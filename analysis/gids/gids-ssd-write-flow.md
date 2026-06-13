# GIDS SSD 데이터 기록 플로우 분석

> 참조: [[gids-complete-architecture]] §11, [[gids-workload-analysis]] §5.1
> 코드: `gids-study/evaluation/write_data.sh`, `write_data_full.sh`, `tensor_write.py`
> 핵심 연결: `GIDS_Setup/GIDS/GIDS.py` `heterograph_map` ↔ 셸 스크립트 `--loffset`

---

## 1. 왜 SSD에 미리 기록해야 하는가

GIDS는 NVMe SSD의 raw block 영역을 파일시스템 없이 BaM이 직접 LBA(Logical Block Address)로 읽는다.
따라서 학습 전 1회, **numpy `.npy` feature 파일을 지정된 LBA 오프셋에 기록해두는 전처리가 반드시 필요**하다.

```
전처리 (1회):  node_feat.npy → SSD LBA 레이아웃 기록
학습 (반복):   GPU → BaM bam_ptr.read(node_id) → LBA 계산 → NVMe DMA
```

LBA 레이아웃이 학습 스크립트의 `heterograph_map` 오프셋과 정확히 일치해야
올바른 feature를 읽을 수 있다. 불일치 시 다른 노드의 feature를 읽게 되어 학습이 조용히 망가진다.

---

## 2. 두 가지 기록 도구

| 도구 | 위치 | 경유 경로 | 특징 |
|------|------|-----------|------|
| `nvm-readwrite_stripe-bench` | `bam/build/bin/` | BaM Controller → libnvm → NVMe | 고속 (큐 깊이 1024, 128 큐) |
| `tensor_write.py` | `evaluation/` | GIDS Python API → gids_nvme.cu → write_feature_kernel | GIDS 코드 경로 검증용 |

실제 대용량 기록은 BaM 네이티브 바이너리(`nvm-readwrite_stripe-bench`)를 사용한다.
`tensor_write.py`는 GIDS 쓰기 경로(`store_tensor`)의 정합성을 확인하거나
소규모 테스트에 쓰인다.

---

## 3. LBA 레이아웃 원칙

### 3.1 핵심 공식

```
ntype의 SSD 시작 주소(byte) = cumulative_node_count × page_size

여기서:
  cumulative_node_count = 이 ntype 앞에 오는 모든 ntype의 노드 수 합계
  page_size = 4096 bytes (= 1 feature vector, float32 × 1024)
```

즉 **feature 한 벡터 = NVMe 1 페이지 = 4096 bytes** 이므로,
`node_id × 4096`이 그대로 해당 노드 feature의 byte offset이 된다 (ntype offset 포함 시).

### 3.2 왜 `page_size = 4096 = 1 feature vector`인가

```
feature dim = 1024 elements × sizeof(float32) = 1024 × 4 = 4096 bytes
NVMe 최소 읽기 단위 = 4096 bytes (4KB sector)

→ 노드 1개의 feature = NVMe 1 sector = BaM 1 page
→ node_id → LBA 변환이 단순 shift(<<12)로 가능
→ GPU warp 1개가 node 1개의 feature를 1회 page read로 가져올 수 있음
```

---

## 4. write_data.sh — 단일 SSD (4 ntype)

IGBH full의 paper/author/fos/institute 4개 ntype을 **1개 SSD**에 순서대로 기록한다.

```bash
# paper (loffset 없음 = 0번지부터)
nvm-readwrite_stripe-bench --input paper/node_feat.npy \
  --access_type 1 --queue_depth 1024 --num_queues 128 --threads 102400 \
  --n_ctrls 1

# author (paper 269,346,174개 이후)
nvm-readwrite_stripe-bench --input author/node_feat.npy \
  --n_ctrls 1 --loffset $((269346174 * 4096))

# fos (paper+author 누적 이후)
nvm-readwrite_stripe-bench --input fos/node_feat.npy \
  --n_ctrls 1 --loffset $((546567057 * 4096))

# institute (paper+author+fos 누적 이후)
nvm-readwrite_stripe-bench --input institute/node_feat.npy \
  --n_ctrls 1 --loffset $((547280017 * 4096))
```

### 레이아웃 테이블 (단일 SSD)

```
SSD 논리 주소공간 (단위: node index = 4096 bytes/node)
┌─────────────────────────────────────────────────────────┐
│ 0             269,346,174                               │  ← paper
│               269M nodes × 4096 = ~1,035 GB            │
├─────────────────────────────────────────────────────────┤
│ 269,346,174   546,567,057                               │  ← author
│               277,220,883 nodes × 4096 = ~1,069 GB     │
├──────────────────┤
│ 546,567,057   547,280,017                               │  ← fos
│               712,960 nodes × 4096 = ~2.7 GB           │
├──────┤
│ 547,280,017   547,280,017 + |institute|                 │  ← institute
│               노드 수는 소규모 (수십만 추정)            │
└──────────────────────────────────────────────────────────┘
총 용량: ~2.1 TB (4 ntype 합계)
```

### loffset ↔ heterograph_map 대응

```python
# GIDS_Setup/GIDS/GIDS.py의 heterograph_map (node index 단위)
heterograph_map = {
    'full': {
        'paper':     0,          # loffset = 0 × 4096 = 0 bytes
        'author':    269346174,  # loffset = 269346174 × 4096
        'fos':       546567057,  # loffset = 546567057 × 4096
        'institute': 547280017,  # loffset = 547280017 × 4096
    }
}
```

`heterograph_map`의 값이 write_data.sh의 `--loffset` 계산 전(×4096 전) 값과 **1:1 동일**하다.
이것이 "왜 author offset이 269,346,174인가"에 대한 답이다 — paper 노드 수가 그대로 author의 시작 node index가 된다.

---

## 5. write_data_full.sh — 멀티 SSD 스트라이핑 (6 ntype)

IGBH Massive(large/full + journal/conference)를 **2개 SSD Controller** 스트라이핑으로 기록한다.

```bash
# paper (ioffset=0: Controller0 페이지 0부터)
nvm-readwrite_stripe-bench --input paper/node_feat.npy \
  --n_ctrls 2 --pages $((1024*1024)) --ioffset 0

# author (loffset = paper 이후, ioffset=0 유지)
nvm-readwrite_stripe-bench --input author/node_feat.npy \
  --n_ctrls 2 --loffset $((269346174*4096)) --ioffset 0

# fos (ioffset=128: Controller 경계 정렬 조정)
nvm-readwrite_stripe-bench --input fos/node_feat.npy \
  --n_ctrls 2 --loffset $((546567057*4096)) --ioffset 128

# institute
nvm-readwrite_stripe-bench --input institute/node_feat.npy \
  --n_ctrls 2 --loffset $((547280017*4096)) --ioffset 128

# journal (fos 직후 소형 ntype)
nvm-readwrite_stripe-bench --input journal/node_feat.npy \
  --n_ctrls 2 --loffset $((546593975*4096)) --ioffset 128

# conference
nvm-readwrite_stripe-bench --input conference/node_feat.npy \
  --n_ctrls 2 --loffset $((546643027*4096)) --ioffset 128
```

### 6 ntype 레이아웃 (논리 node index 기준, 정렬 순)

```
node index offset    ntype         노드 수(추정)
─────────────────────────────────────────────────────
0                    paper         269,346,174
269,346,174          author        277,220,883
546,567,057          fos           26,918     ← 소형
546,593,975          journal       49,052     ← 소형
546,643,027          conference    636,990    ← 소형
547,280,017          institute     소규모
```

> **주의**: fos 노드 수가 write_data.sh(4 ntype, 712,960개)와 다르다.
> write_data_full.sh는 "large" 크기 변형을 대상으로 하거나, journal/conference 포함 시
> fos 할당 슬롯이 달라지는 버전으로 추정된다. 실제 실행 환경의 dataset_size를 확인해야 한다.

### STRIPE 모드 동작 원리

```
n_ctrls = 2 (SSD 0, SSD 1)
page_i → ctrls[i % 2]로 라운드로빈

실제 물리 배치:
  page 0 → SSD0, page 1 → SSD1, page 2 → SSD0, page 3 → SSD1, ...

ioffset=128 의미:
  소형 ntype의 기록 시작 페이지가 Controller 내 128 페이지 오프셋에서 시작.
  대형 ntype(paper/author)이 이미 차지한 공간의 끝 맞춤 역할.
  → Controller 간 데이터가 균형 있게 분산되도록 조정.

--pages $((1024*1024)) = 1M pages = 4 GB 단위로 기록.
  대용량 feature를 한 번에 다 쓰지 않고 4GB 청크로 나눠 기록.
```

---

## 6. tensor_write.py — GIDS Python API 경유

GIDS의 쓰기 경로(`store_tensor`)를 직접 검증하기 위한 파이썬 스크립트다.

```python
# 동작 순서
GIDS_Loader = GIDS.GIDS(
    page_size=args.page_size,  # 4096
    off=args.offset,            # SSD byte offset
    num_ele=args.num_ele,       # 총 element 수
    num_ssd=args.num_ssd,
    cache_size=args.cache_size,
    cache_dim=args.cache_dim,   # 1024 (feature dim)
    ssd_list=gids_ssd_list
)
emb = np.load(args.path)                      # .npy → host numpy
emb_tensor = torch.tensor(emb).to('cuda:0')  # GPU로 이동
GIDS_Loader.store_tensor(emb_tensor, 0)       # logical index 0부터 기록
```

### 호출 체인

```
tensor_write.py
  └─ GIDS.GIDS.store_tensor(tensor, start_idx)       [GIDS_Setup/GIDS/GIDS.py]
       └─ BAM_Feature_Store_float.store_tensor(...)   [pybind11]
            └─ BAM_Feature_Store<float>::store_tensor [gids_nvme.cu]
                 └─ write_feature_kernel2<<<...>>>     [gids_kernel.cu]
                      └─ bam_ptr<float>.write(idx, val) → NVMe SQE 제출
```

`write_feature_kernel2` 내부:
- block당 1 node, warp당 feature 32개 처리 (1024 / 32 = 32 iteration)
- GPU HBM의 `emb_tensor` → BaM `bam_ptr.write()` → SSD DMA

---

## 7. 읽기-쓰기 레이아웃 일관성 보장

학습 시 feature를 올바르게 읽으려면 쓰기 레이아웃과 읽기 오프셋이 정확히 대응해야 한다.

```
기록 시 (write_data.sh):
  ntype별 --loffset = cumulative_node_count × 4096 (bytes)

읽기 시 (heterograph_map):
  ntype별 offset = cumulative_node_count (node index)

GIDS 내부에서:
  read_feature_hetero() 호출 시:
    byte_offset = heterograph_map[ntype] × page_size
               = heterograph_map[ntype] × 4096
  → write_data.sh의 --loffset과 동일한 값

검증:
  paper  : 0 × 4096 = 0                ✓ (loffset 없음 = 0)
  author : 269346174 × 4096            ✓
  fos    : 546567057 × 4096            ✓
  institute: 547280017 × 4096          ✓
```

---

## 8. 실행 전제 조건

```bash
# 1. BaM 커널 모듈 삽입 (libnvm 제공)
sudo insmod bam/module/libnvm.ko

# 2. NVMe 디바이스 확인
ls /dev/libnvm*    # libnvm0, libnvm1, ...

# 3. NVMe SSD 용량 확인
#    IGB full (4 ntype): ~2.1 TB → 3 TB NVMe 이상 권장
#    IGBH full (6 ntype): ~2.25 TB → 2 × 2 TB NVMe (striping)

# 4. 데이터 기록 (1회)
cd gids-study/evaluation
./write_data.sh          # 단일 SSD
# 또는
./write_data_full.sh     # 멀티 SSD

# 5. 학습 실행
./run_GIDS_IGBH.sh
```

---

*참조: [[gids-complete-architecture]] §11 Feature 쓰기 플로우*
*참조: [[gids-workload-analysis]] §5.1 Feature 스토리지 용량*
*코드: `gids-study/gids_module/gids_kernel.cu` `write_feature_kernel2`, `gids_nvme.cu` `BAM_Feature_Store::store_tensor`*
