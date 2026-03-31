# GIDS 학습 파이프라인 End-to-End 분석

## 1. 전체 실행 타임라인

```
시간 ──────────────────────────────────────────────────────────────────►

[Phase 0: 데이터 준비 (오프라인, 1회)]
│
├── 그래프 데이터 로드 (dataloader.py)
│   IGB260MDGLDataset.process()
│   → CSC 형식으로 그래프 구조 로드
│   → feature는 mmap (메모리에 로드하지 않음)
│
├── Feature를 SSD에 기록 (tensor_write.py)
│   np.load("node_feat.npy") → torch.tensor → GIDS.store_tensor()
│   → write_feature_kernel2 → BaM → SSD에 STRIPE로 분산 저장
│
└── PageRank 노드 리스트 생성 (page_rank_node_list_gen.py)
    compute_pagerank(g) → top 60% indices → torch.save()


[Phase 1: 초기화 (~수 초)]
│
├── GIDS 초기화
│   GIDS() → Controllers → Page Cache → Range → Array
│
├── CPU Buffer 설정
│   cpu_backing_buffer() → set_cpu_buffer()
│   → SSD에서 핫 노드 feature를 CPU 피닝 메모리로 복사
│
├── DataLoader 생성
│   GIDS_DGLDataLoader(g, train_nid, sampler, GIDS=GIDS_Loader)
│
└── 모델 초기화
    SAGE(in_dim=1024, h_dim=256, out_dim=19, num_layers=3)
    → GPU에 로드


[Phase 2: Warm-up (~100 iterations)]
│
├── 캐시 워밍업
│   첫 100 배치 동안:
│   - Page Cache에 자주 접근되는 페이지 로드
│   - CPU Buffer 히트율 안정화
│   - SSD I/O 패턴 안정화
│
└── 측정하지 않음 (cold start 제외)


[Phase 3: 학습 (반복)]
│
│  for epoch in range(3):
│    for step in range(num_batches):
│      │
│      │ ┌─── 단일 배치 처리 ─────────────────────────────────┐
│      │ │                                                    │
│      │ │ [3a] DGL Sampling (~5-10ms)                        │
│      │ │   NeighborSampler.sample(g, seed_nodes)            │
│      │ │   → 2-hop: 각 시드에서 10개, 15개 이웃 선택       │
│      │ │   → blocks (메시지 흐름 서브그래프) 생성           │
│      │ │   → input_nodes: feature가 필요한 노드 목록       │
│      │ │                                                    │
│      │ │ [3b] GIDS Feature Read (~1-10ms)                   │
│      │ │   BAM_FS.read_feature(out, idx, n, dim, ...)       │
│      │ │   → CUDA 커널 런치                                │
│      │ │   → 각 warp: CPU buffer check → Page Cache →       │
│      │ │     (miss시) SSD read                              │
│      │ │   → batch_inputs 완성 (GPU 텐서)                  │
│      │ │                                                    │
│      │ │ [3c] Block Transfer (~1-5ms)                       │
│      │ │   blocks = [b.to(device) for b in blocks]          │
│      │ │   batch_labels = labels.to(device)                 │
│      │ │                                                    │
│      │ │ [3d] Forward Pass (~2-5ms)                         │
│      │ │   pred = model(blocks, batch_inputs)               │
│      │ │   → Layer 0: SAGE(input → hidden)                  │
│      │ │   → Layer 1: SAGE(hidden → hidden)                 │
│      │ │   → Layer 2: SAGE(hidden → output)                 │
│      │ │                                                    │
│      │ │ [3e] Loss & Backward (~3-8ms)                      │
│      │ │   loss = CrossEntropy(pred, labels)                │
│      │ │   loss.backward()                                  │
│      │ │   optimizer.step()                                 │
│      │ │                                                    │
│      │ │ 총 배치 시간: ~12-38ms                             │
│      │ └────────────────────────────────────────────────────┘
│      │
│      └── 반복
│
└── 에폭 완료 후 validation/test
```

## 2. 단일 배치 데이터 흐름 상세

```
seed_nodes = [42, 1337, 999, ...]  (batch_size=1024)
│
│  NeighborSampler (fan_out=[10, 15])
│
├── Layer 1 (2-hop): 각 시드에서 15개 이웃 선택
│   → 중간 노드: ~15,000개 (중복 제거 후)
│
├── Layer 0 (1-hop): 각 중간 노드에서 10개 이웃 선택
│   → input_nodes: ~50,000개 (중복 제거 후)
│
│   blocks[0]: input_nodes(~50K) → 중간 노드(~15K)
│   blocks[1]: 중간 노드(~15K) → seed_nodes(1024)
│
│  GIDS Feature Read
│
├── input_nodes (~50,000개)의 feature 읽기
│   │
│   │ 메모리 접근 분포 (예시):
│   │
│   │  TLB Hit:        ~5%   (~2,500 노드)  ~10ns each
│   │  Page Cache Hit: ~25%  (~12,500 노드) ~100ns each
│   │  CPU Buffer Hit: ~30%  (~15,000 노드) ~1-5μs each
│   │  SSD Read:       ~40%  (~20,000 노드) ~10-20μs each
│   │
│   │ SSD I/O 상세:
│   │  20,000 노드 × 4KB = 80 MB 읽기
│   │  4 SSD × ~5 GB/s = 20 GB/s 합산
│   │  이론적 SSD 시간: 80MB / 20GB/s = 4ms
│   │
│   └── batch_inputs = tensor(50000, 1024)  (200 MB GPU 메모리)
│
│  GNN 연산
│
├── blocks[0]: SAGE Layer 0
│   input: (50000, 1024) → output: (15000, 256)
│   연산: message passing + aggregation + linear
│
├── blocks[1]: SAGE Layer 1
│   input: (15000, 256) → output: (1024, 256)
│   연산: message passing + aggregation + linear
│
├── Linear: (1024, 256) → (1024, 19)  [num_classes]
│
├── CrossEntropyLoss(pred, labels)
│
└── backward() + optimizer.step()
```

## 3. 성능 측정 코드 구조

```python
# homogenous_train.py :: track_acc_GIDS()

warmup = 100
measure_iters = 100

for step, (input_nodes, seeds, blocks, ret) in enumerate(dataloader):

    if step < warmup:
        # 워밍업: 측정 안 함
        continue

    if step == warmup:
        dataloader.print_timer()  # 타이머 리셋

    # ── 전송 시간 측정 ──
    transfer_start = time.time()
    blocks = [b.to(device) for b in blocks]
    batch_labels = blocks[-1].dstdata['label'].to(device)
    transfer_time = time.time() - transfer_start

    # ── 학습 시간 측정 ──
    train_start = time.time()
    batch_inputs = ret
    batch_pred = model(blocks, batch_inputs)
    loss = loss_fn(batch_pred, batch_labels)
    loss.backward()
    optimizer.step()
    optimizer.zero_grad()
    train_time = time.time() - train_start

    # ── E2E 시간 측정 ──
    e2e_time = time.time() - e2e_start

    if step == warmup + measure_iters:
        dataloader.print_stats()  # GIDS 통계 출력
        break

# 출력 예시:
# GIDS Time: 2.5s
# Kernel Time: 1.8s
# Total Access: 5,000,000
# CPU Access: 1,500,000 (30%)
# Page Cache Hit Rate: 65%
# Transfer Time: 0.3s
# Train Time: 1.2s
# E2E Time: 4.0s
```

## 4. 데이터셋별 실행 예시

```bash
# ── IGB260M Full (동종 그래프, SAGE) ──
python homogenous_train.py \
  --dataset_size full \
  --path /data/igb/ \
  --model_type sage \
  --num_layers 3 \
  --batch_size 1024 \
  --fan_out "10,15,25" \
  --emb_size 1024 \
  --GIDS \
  --num_ssd 4 \
  --ssd_list 0,1,2,3 \
  --page_size 8 \
  --cache_size 8 \
  --cache_dim 1024 \
  --cpu_buffer \
  --cpu_buffer_percent 0.2 \
  --pin_file /data/igb/pagerank_full.pt \
  --window_buffer \
  --wb_size 6

# 예상 데이터 크기:
#   노드 수: 269,346,000
#   Feature: 269M × 1024 × 4B = ~1.03 TB
#   그래프: CSC 형식 ~수십 GB
#   캐시: 8 GB GPU VRAM
#   CPU Buffer: 20% × 1.03 TB = ~206 GB


# ── IGBH Full (이종 그래프, RSAGE) ──
python heterogeneous_train.py \
  --dataset_size full \
  --path /data/igb/ \
  --model_type rsage \
  --num_layers 2 \
  --batch_size 1024 \
  --fan_out "15,10" \
  --emb_size 1024 \
  --GIDS \
  --num_ssd 4 \
  --ssd_list 0,1,2,3 \
  --page_size 8 \
  --cache_size 8 \
  --cache_dim 1024 \
  --heterograph_map "paper:0,author:269346174,fos:546567174,institute:547280174"


# ── Baseline 비교 (동일 데이터셋, mmap) ──
python homogenous_train_baseline.py \
  --dataset_size full \
  --path /data/igb/ \
  --model_type sage \
  --num_layers 3 \
  --in_memory 0 \
  --sample_type NHS \
  --fan_out "10,15,25"
```

## 5. 학습 정확도 검증

```
GIDS는 데이터 접근 방식만 변경 (알고리즘 불변):

┌────────────────────────┬──────────────┬────────────────┐
│ 방법                    │ 접근 방식     │ 정확도          │
├────────────────────────┼──────────────┼────────────────┤
│ Standard DGL           │ CPU→GPU copy │ Baseline       │
│ UVA                    │ Host DRAM    │ = Baseline     │
│ GIDS                   │ SSD→GPU P2P  │ = Baseline     │
│ GIDS + CPU Buffer      │ 위 + 피닝    │ = Baseline     │
│ GIDS + Window Buffer   │ 위 + 프리페치│ = Baseline     │
│ GIDS + Accumulator     │ 위 + 누적    │ = Baseline     │
└────────────────────────┴──────────────┴────────────────┘

모든 최적화가 투명(transparent): 읽는 데이터는 동일
→ 학습 수렴, 최종 정확도 동일
→ 속도만 다름
```

## 6. 에러 처리 및 디버깅

```
GIDS_unit_test.py로 기본 동작 검증:

# 동종 fetch 테스트
ret = GIDS_Loader.fetch_test(1, 1024)
assert ret.shape == (1, 1024)

# 이종 fetch 테스트
ret = GIDS_Loader.fetch_hetero_test(1, 1024)
assert ret.shape == (1, 1024)

디버깅 커널:
  read_kernel<<<1, 1>>>        # 단일 스레드로 값 출력
  seq_read_kernel<<<1, 32>>>   # 순차 읽기 + 출력

통계 출력:
  GIDS_Loader.print_stats()
  → Page Cache 히트율, 미스율
  → 커널 실행 시간
  → CPU 버퍼 히트 수
  → 총 접근 수
```
