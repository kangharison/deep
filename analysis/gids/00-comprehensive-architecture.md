# GIDS 전체 아키텍처 A-Z

## 1. GIDS가 해결하는 문제

```
기존 GNN 학습의 병목:

  ┌─────────┐    sampling     ┌──────────┐   PCIe copy   ┌─────────┐
  │ CPU     │ ──────────────► │ CPU      │ ────────────► │ GPU     │
  │ (Graph  │   이웃 탐색     │ (Feature │  느린 전송    │ (GNN    │
  │  구조)  │                 │  Gather) │               │  연산)  │
  └─────────┘                 └──────────┘               └─────────┘
       │                           │
       │                    ★ 여기가 병목 ★
       │                    CPU random access 느림
       │                    Feature 수백GB → DRAM 초과
       ▼
  ogbn-papers100M: Feature ~60GB
  MAG240M:         Feature ~380GB
  IGB-Full:        Feature ~1TB+
```

```
GIDS의 해결 방식:

  ┌──────────────────── GPU ────────────────────┐
  │                                             │
  │  Sampling ──► Feature Gather ──► GNN Train  │
  │  (DGL)        (GIDS/BaM)         (PyTorch)  │
  │                    │                         │
  │              ┌─────▼──────┐                  │
  │              │ GPU Cache  │ Hit → 즉시 반환  │
  │              │ (VRAM)     │                  │
  │              └─────┬──────┘                  │
  │                    │ Miss                    │
  └────────────────────┼────────────────────────┘
                       │ PCIe P2P (CPU 우회)
                  ┌────▼─────┐
                  │ NVMe SSD │ Feature Table
                  │ (1~6개)  │ 노드ID순 저장
                  └──────────┘
```

## 2. 시스템 레이어 구조

```
┌─────────────────────────────────────────────────────────────────────────┐
│ Layer 5: Python Training Script                                         │
│   evaluation/homogenous_train.py, heterogeneous_train.py                │
│   - argparse 설정 → 데이터셋 로드 → 학습 루프                          │
│   - model(blocks, batch_inputs) → loss → backward → step               │
├─────────────────────────────────────────────────────────────────────────┤
│ Layer 4: GIDS Python API                                                │
│   GIDS_Setup/GIDS/GIDS.py                                              │
│   - GIDS 클래스: 초기화, fetch_feature(), window_buffering()            │
│   - GIDS_DGLDataLoader: DGL DataLoader 래핑                            │
│   - _PrefetchingIter: 배치마다 GIDS로 feature fetch                    │
├─────────────────────────────────────────────────────────────────────────┤
│ Layer 3: C++/CUDA Module (PyBind11)                                     │
│   gids_module/gids_nvme.cu + gids_kernel.cu                            │
│   - BAM_Feature_Store<T>: Controller 초기화, read_feature(), store()     │
│   - CUDA Kernels: read_feature_kernel, write_feature_kernel             │
│   - GIDS_Controllers: NVMe 컨트롤러 관리                               │
├─────────────────────────────────────────────────────────────────────────┤
│ Layer 2: BaM Library (libnvm)                                           │
│   bam/ (외부 의존성)                                                    │
│   - Controller, QueuePair, page_cache_t, range_t, array_t              │
│   - nvm_parallel_queue.h: Lock-free GPU parallel queue                  │
│   - sq_enqueue, cq_poll, cq_dequeue                                    │
├─────────────────────────────────────────────────────────────────────────┤
│ Layer 1: BaM Kernel Module                                              │
│   bam/module/ (libnvm_helper.ko)                                        │
│   - /dev/libnvm0~5: char device                                         │
│   - BAR0 mmap, GPU DMA mapping (nvidia_p2p)                             │
├─────────────────────────────────────────────────────────────────────────┤
│ Layer 0: Hardware                                                       │
│   GPU ◄──── PCIe P2P ────► NVMe SSD (1~6개)                            │
└─────────────────────────────────────────────────────────────────────────┘
```

## 3. 데이터 준비 (SSD에 Feature 기록)

```
evaluation/tensor_write.py

  NumPy 파일 (.npy)
       │
       ▼
  np.load(path) → torch.tensor() → .to(cuda)
       │
       ▼
  GIDS_Loader.store_tensor(emb_tensor, offset=0)
       │
       ▼ (gids_nvme.cu :: store_tensor)
  write_feature_kernel2<<<num_nodes, 128>>>
       │
       │ 각 블록(=노드 1개)이 feature 벡터를 SSD에 기록
       │ GPU 스레드 → array_t → page_cache → NVMe SQ → SSD
       ▼
  h_pc->flush_cache()  ← 캐시 플러시로 SSD에 확실히 기록

  SSD 레이아웃 (STRIPE 모드):
  ┌─────────────────────────────────────────┐
  │ Page 0 → SSD0, Page 1 → SSD1, ...      │
  │ 노드 ID 순서로 feature 벡터가 연속 배치  │
  │ 노드 ID × feature_dim × sizeof(float)   │
  │ = 오프셋으로 바로 계산 가능               │
  └─────────────────────────────────────────┘
```

## 4. GIDS 초기화 전체 흐름

```
Python: GIDS(page_size=8, num_ssd=4, cache_size=8, ...)
│
├─[1] BAM_Feature_Store 생성
│     gids_nvme.cu → BAM_Feature_Store<float> 또는 <int64_t>
│
├─[2] GIDS_Controllers 초기화
│     │
│     └─ init_GIDS_controllers(num_ctrls=4, q_depth=1024,
│                               num_q=128, ssd_list=[0,1,2,3])
│        │
│        └─ for each SSD:
│           Controller("/dev/libnvm{i}", nvmNamespace=1,
│                      cudaDevice=0, queueDepth=1024, numQueues=128)
│           │
│           ├── open("/dev/libnvm{i}")        ← BaM 커널 모듈
│           ├── mmap(BAR0)                    ← 레지스터 접근
│           ├── cudaHostRegister(BAR0, IoMemory)
│           ├── Admin Queue 생성
│           ├── I/O QueuePair × 128개 생성
│           │   각 QP: cudaMalloc(SQ/CQ) → ioctl(MAP_DEVICE) → Admin Create
│           └── cudaMemcpy(QP → GPU)
│
├─[3] BAM_Feature_Store::init_controllers(...)
│     │
│     ├── page_cache_t 생성
│     │   h_pc = page_cache_t(pageSize, n_pages, cudaDevice,
│     │                       ctrls[0], 64, ctrls)
│     │   - GPU VRAM에 캐시 페이지 풀 할당
│     │   - n_pages = cache_size_GB × 1024³ / pageSize
│     │   - 예: 8GB / 4096B = 2,097,152 페이지
│     │
│     ├── range_t<float> 생성
│     │   h_range = range_t<float>(0, numElems, 0, n_pages,
│     │                           0, pageSize, h_pc, ctrls[0], STRIPE)
│     │   - STRIPE: 페이지 단위로 SSD들에 분산
│     │   - 논리 주소 → 물리 SSD/LBA 매핑 담당
│     │
│     └── array_t<float> 생성
│         a = array_t<float>(numElems, 0, vr, cudaDevice)
│         - GPU에서 접근 가능한 가상 배열 추상화
│         - array[i] 접근 시 내부적으로 range → page_cache → SSD
│
├─[4] CPU Backing Buffer (선택적)
│     cpu_backing_buffer(dim=1024, len=num_pinned_nodes)
│     │
│     ├── cudaHostAlloc(cpu_buffer, size, cudaHostAllocMapped)
│     │   피닝된 호스트 메모리 할당
│     │
│     └── cudaHostGetDevicePointer(&device_cpu_buffer, cpu_buffer, 0)
│         GPU에서 직접 접근 가능한 포인터 획득
│
└─[5] CPU Buffer에 핫 노드 로드 (선택적)
      set_cpu_buffer(pagerank_tensor, num_nodes)
      │
      ├── set_cpu_buffer_kernel<<<...>>>
      │   range 메타데이터에 "이 노드는 CPU 버퍼에 있다" 기록
      │
      └── set_cpu_buffer_data_kernel<<<num_nodes, 128>>>
          SSD → array → CPU 버퍼로 핫 노드 feature 복사
```

## 5. Feature Read 경로 (핵심)

```
Python 학습 루프에서:
  for input_nodes, seeds, blocks, ret in GIDS_DGLDataLoader:
      batch_inputs = ret    ← 이미 GPU에 있는 feature 텐서
                                            │
                                            │ 내부적으로:
                                            ▼
GIDS.py :: fetch_feature(dim, it, device)
│
├── it = next(dataloader_it)
│   → (input_nodes, seeds, blocks)
│   → DGL이 sampling 수행 (이웃 노드 선택)
│
├── input_nodes에서 feature가 필요한 노드 ID 추출
│   Homogeneous: input_nodes (1D tensor)
│   Heterogeneous: input_nodes[ntype] (dict of tensors)
│
├── return_torch_buffer = torch.zeros(num_nodes, dim, device=cuda)
│   결과를 받을 GPU 텐서 할당
│
└── BAM_FS.read_feature(out_ptr, index_ptr, num_index,
│                        dim, cache_dim, key_off)
    │
    ▼ (gids_nvme.cu)
    read_feature() C++ 메서드
    │
    ├── grid = (num_index + n_warp - 1) / n_warp
    │   block = 128 (4 warps)
    │   n_warp = block / 32 = 4
    │
    └── CUDA Kernel 런치 ──────────────────────────────────┐
        │                                                   │
        │  CPU 버퍼 없음:                                   │
        │  read_feature_kernel<<<grid, 128>>>               │
        │                                                   │
        │  CPU 버퍼 있음:                                   │
        │  read_feature_kernel_with_cpu_backing_memory      │
        │  <<<grid, 128>>>                                  │
        │                                                   │
        ▼                                                   │
   ┌────────────────────────────────────────────────────┐   │
   │ GPU Kernel 내부 (gids_kernel.cu)                   │   │
   │                                                    │   │
   │ 각 warp(32 스레드)가 노드 1개의 feature를 담당     │   │
   │                                                    │   │
   │ warp_id = (blockIdx.x * blockDim.x + threadIdx.x)  │   │
   │           / 32                                      │   │
   │ lane_id = threadIdx.x % 32                          │   │
   │                                                    │   │
   │ if (warp_id < num_idx):                            │   │
   │   row_index = index_ptr[warp_id] + key_off         │   │
   │                                                    │   │
   │   ┌─── CPU 버퍼 체크 (있는 경우) ───┐              │   │
   │   │ if CPU_buffer에 있는 노드인가?   │              │   │
   │   │   → CPU 피닝 메모리에서 직접 읽기 │              │   │
   │   │   for (j = lane_id; j < dim;     │              │   │
   │   │        j += 32):                 │              │   │
   │   │     out[warp_id*dim+j] =         │              │   │
   │   │       CPU_buffer[cpu_idx*dim+j]  │              │   │
   │   │   atomicAdd(d_cpu_access, 1)     │              │   │
   │   │   return                         │              │   │
   │   └──────────────────────────────────┘              │   │
   │                                                    │   │
   │   ┌─── SSD에서 읽기 (BaM 경로) ───────────────┐    │   │
   │   │ bam_ptr<T> ptr = dr->get_bam_ptr()        │    │   │
   │   │                                            │    │   │
   │   │ for (j = lane_id; j < dim; j += 32):      │    │   │
   │   │   val = ptr.read(row_index * cache_dim + j)│    │   │
   │   │   out_tensor_ptr[warp_id * dim + j] = val  │    │   │
   │   │                                            │    │   │
   │   │ ┌─── ptr.read() 내부 ──────────────────┐  │    │   │
   │   │ │ 1. TLB lookup: 페이지 캐시 히트?     │  │    │   │
   │   │ │    → Hit: GPU VRAM에서 즉시 반환      │  │    │   │
   │   │ │    → Miss: 아래 진행                  │  │    │   │
   │   │ │                                       │  │    │   │
   │   │ │ 2. Page Cache에서 빈 슬롯 확보       │  │    │   │
   │   │ │    (eviction 필요 시 LRU 교체)        │  │    │   │
   │   │ │                                       │  │    │   │
   │   │ │ 3. NVMe Read 커맨드 발행             │  │    │   │
   │   │ │    range → 논리주소를 물리SSD/LBA 변환│  │    │   │
   │   │ │    STRIPE: page_id % num_ssd → SSD 선택│  │    │   │
   │   │ │                                       │  │    │   │
   │   │ │ 4. BaM I/O 실행                      │  │    │   │
   │   │ │    sq_enqueue(cmd) → *sq.db = tail    │  │    │   │
   │   │ │    (SSD Doorbell)                     │  │    │   │
   │   │ │                                       │  │    │   │
   │   │ │ 5. SSD가 PCIe P2P DMA로              │  │    │   │
   │   │ │    GPU VRAM (캐시 페이지)에 데이터 기록│  │    │   │
   │   │ │                                       │  │    │   │
   │   │ │ 6. cq_poll() → 완료 확인             │  │    │   │
   │   │ │                                       │  │    │   │
   │   │ │ 7. 캐시 페이지에서 값 읽어서 반환    │  │    │   │
   │   │ └───────────────────────────────────────┘  │    │   │
   │   └────────────────────────────────────────────┘    │   │
   └─────────────────────────────────────────────────────┘   │
                                                             │
   결과: return_torch_buffer에 모든 노드의 feature가 채워짐  │
   → batch_inputs로 반환 → model(blocks, batch_inputs)       │
   ──────────────────────────────────────────────────────────┘
```

## 6. 데이터 흐름 타임라인 (단일 배치)

```
시간 ─────────────────────────────────────────────────────────────────►

DGL Sampler          GIDS Python          CUDA Kernel         NVMe SSD
    │                    │                    │                    │
    │  sample(seed)      │                    │                    │
    │  이웃 노드 탐색    │                    │                    │
    │  │                 │                    │                    │
    │  blocks 생성       │                    │                    │
    │  (subgraph)        │                    │                    │
    │  ──────────────►   │                    │                    │
    │                    │                    │                    │
    │                fetch_feature()          │                    │
    │                    │                    │                    │
    │                    │  read_feature()    │                    │
    │                    │  ──────────────►   │                    │
    │                    │                    │                    │
    │                    │             Warp 0: node[0]             │
    │                    │               │ TLB Miss               │
    │                    │               │ sq_enqueue()            │
    │                    │               │ *sq.db ─────────────►  │
    │                    │               │              DMA Read   │
    │                    │               │              Flash 읽기 │
    │                    │               │  ◄── PCIe P2P DMA ──  │
    │                    │               │ cq_poll() 완료         │
    │                    │               │ out[0] = data          │
    │                    │                                        │
    │                    │             Warp 1: node[1]             │
    │                    │               │ TLB Hit!               │
    │                    │               │ → 캐시에서 즉시 반환    │
    │                    │               │ out[1] = cached_data   │
    │                    │                    │                    │
    │                    │             Warp 2: node[2]             │
    │                    │               │ CPU 버퍼 Hit!          │
    │                    │               │ → 피닝 메모리에서 읽기  │
    │                    │               │ out[2] = cpu_data      │
    │                    │                    │                    │
    │                    │             ...수천 warp 병렬...        │
    │                    │                    │                    │
    │                    │  ◄── 커널 완료 ──  │                    │
    │                    │                    │                    │
    │                return (blocks, features)                     │
    │                    │                    │                    │
    ▼                    ▼                    ▼                    ▼

이후: model(blocks, features) → loss → backward → step
```

## 7. 메모리 계층 구조

```
┌─────────────────────────────────────────────────────────────────┐
│                    GIDS 메모리 계층                              │
│                                                                 │
│  ┌─── GPU VRAM ──────────────────────────────────────────────┐  │
│  │                                                           │  │
│  │  ┌─ Page Cache ───────────────────────────────────┐       │  │
│  │  │ 크기: cache_size (기본 8GB)                     │       │  │
│  │  │ 페이지: 4KB (page_size × 512B)                  │       │  │
│  │  │ 엔트리 수: cache_size / page_size               │       │  │
│  │  │ 교체: LRU 기반                                  │       │  │
│  │  │ ★ Cache Hit → ~100ns (GPU VRAM 접근 속도)       │       │  │
│  │  └────────────────────────────────────────────────┘       │  │
│  │                                                           │  │
│  │  ┌─ Output Buffer ─────────────┐  ┌─ SQ/CQ (BaM) ─────┐ │  │
│  │  │ return_torch_buffer         │  │ GPU VRAM에 위치     │ │  │
│  │  │ (batch_size × dim × 4B)    │  │ Doorbell은 BAR0     │ │  │
│  │  └─────────────────────────────┘  └─────────────────────┘ │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌─── CPU Pinned Memory (선택적) ────────────────────────────┐  │
│  │ 크기: cpu_buffer_percent × total_nodes × dim × 4B          │  │
│  │ 내용: PageRank 상위 노드의 feature                         │  │
│  │ 접근: cudaHostGetDevicePointer로 GPU에서 직접 읽기         │  │
│  │ ★ CPU Buffer Hit → ~1-5μs (PCIe를 통한 호스트 메모리)     │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌─── NVMe SSD (1~6개, STRIPE) ─────────────────────────────┐  │
│  │ Feature Table: 노드ID순, page 단위로 SSD에 분산           │  │
│  │ Page 0 → SSD0, Page 1 → SSD1, Page 2 → SSD2, ...        │  │
│  │ ★ SSD Read → ~10-20μs (NVMe latency)                     │  │
│  │ ★ 4 SSD 대역폭 합산: ~20+ GB/s                           │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                 │
│  접근 우선순위:                                                 │
│  GPU Cache (Hit) > CPU Buffer (Hit) > SSD (Miss)                │
│  ~100ns            ~1-5μs             ~10-20μs                  │
└─────────────────────────────────────────────────────────────────┘
```

## 8. GIDS 최적화 기법 3가지

### 8.1 CPU Backing Buffer
```
목적: 자주 접근되는 "허브 노드"를 CPU 피닝 메모리에 캐싱

준비:
  page_rank_node_list_gen.py
  → PageRank 상위 20% 노드 ID 추출
  → torch.save(indices, "pin_file.pt")

런타임:
  GIDS_Loader.cpu_backing_buffer(dim=1024, length=N)
  GIDS_Loader.set_cpu_buffer(pagerank_tensor, N)

커널 내부:
  if (range에서 CPU 버퍼 노드로 표시됨):
    CPU 피닝 메모리에서 직접 읽기 (SSD 우회)
    atomicAdd(cpu_access_count, 1)  ← 통계 수집
```

### 8.2 Window Buffering
```
목적: 다음 몇 배치의 feature를 미리 캐시에 로드

원리:
  현재 배치의 sampling 결과에서 다음 wb_size개 배치를 미리 확인
  해당 노드들의 페이지를 page_cache에 프리페치

  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
  │ Batch 0  │ │ Batch 1  │ │ Batch 2  │ │ Batch 3  │
  │ (현재)   │ │ (프리페치)│ │ (프리페치)│ │ (프리페치)│
  └──────────┘ └──────────┘ └──────────┘ └──────────┘
       ▲              ▲           ▲           ▲
       │              └───────────┴───────────┘
       │              Window Buffer (wb_size=3)
       │
  이 배치 처리하는 동안 다음 3개 배치 프리페치

코드:
  set_window_buffering_kernel<<<num_pages, 32>>>
  → page_cache의 prefetch_counter 설정
  → 백그라운드에서 SSD 읽기 시작
```

### 8.3 Storage Access Accumulator
```
목적: 여러 배치의 I/O 요청을 합쳐서 SSD 대역폭 극대화

문제: 배치가 작으면 SSD에 보내는 I/O도 적음 → 대역폭 낭비
해결: required_accesses 이상이 될 때까지 배치를 누적

공식:
  required = (p × BW × 1024 / page_size × (L_ssd + L_system) × num_ssd) / (1-p)

  p = 캐시 미스 비율 (추정)
  BW = SSD 피크 대역폭 (GB/s)
  L_ssd = SSD 지연 (μs)
  L_system = 시스템 지연 (μs)

동작:
  ┌──────────┐ ┌──────────┐ ┌──────────┐
  │ Batch 0  │ │ Batch 1  │ │ Batch 2  │ ← 누적
  │ 500 nodes│ │ 500 nodes│ │ 500 nodes│
  └────┬─────┘ └────┬─────┘ └────┬─────┘
       └─────────────┴─────────────┘
                     │
                     ▼
       read_feature_merged<<<...>>>
       1500 nodes를 한 번에 읽기
       → SSD 대역폭 최대 활용
```

## 9. Heterogeneous Graph 처리

```
이종 그래프: 여러 종류의 노드와 엣지

IGB260M-Hetero 예시:
  노드: paper, author, institute, fos, journal, conference
  엣지: paper→cites→paper, author→writes→paper, ...

SSD에서의 Feature 배치:
  ┌──────────────────────────────────────────────────┐
  │ Offset 0            │ Offset 269M        │ ...   │
  │ paper features      │ author features    │ fos   │
  │ (269M × 1024 × 4B) │ (277M × 1024 × 4B)│ ...   │
  └──────────────────────────────────────────────────┘

key_offset 딕셔너리:
  { 'paper': 0, 'author': 269346000, 'fos': 546567000, ... }

read_feature_hetero() 동작:
  for each node_type:
    stream[i] = cudaStreamCreate()  ← 노드 타입별 별도 스트림
    read_feature_kernel<<<..., stream[i]>>>(
        ..., key_off=key_offset[ntype])
  → 여러 노드 타입을 동시에 읽기 (스트림 병렬)
```

## 10. BaM 원본과 GIDS의 차이

```
┌──────────────────┬──────────────────────┬───────────────────────────┐
│ 항목              │ BaM (원본)            │ GIDS (확장)               │
├──────────────────┼──────────────────────┼───────────────────────────┤
│ 목적              │ 범용 GPU-NVMe I/O    │ GNN Feature Fetching 특화 │
│ API               │ C++/CUDA only        │ + Python/PyBind11 바인딩  │
│ 프레임워크        │ 없음 (standalone)     │ DGL + PyTorch 통합        │
│ 캐시              │ page_cache_t         │ + CPU Backing Buffer      │
│                   │                      │ + Window Buffering        │
│                   │                      │ + Storage Accumulator     │
│ 데이터 분산       │ STRIPE/REPLICATE     │ STRIPE (고정)             │
│ I/O 패턴          │ 임의 read/write      │ Feature Gather (read 중심)│
│ 이종 그래프       │ 미지원               │ 노드 타입별 offset 지원   │
│ 배치 I/O          │ 개별 스레드 I/O      │ 배치 누적 + 멀티스트림    │
│ 핫 노드 최적화    │ 없음                 │ PageRank 기반 CPU 피닝    │
│ 학습 통합         │ 없음                 │ DataLoader Iterator 래핑  │
└──────────────────┴──────────────────────┴───────────────────────────┘
```

## 11. 전체 실행 예시

```bash
# 1. BaM 커널 모듈 로드
sudo insmod libnvm_helper.ko max_num_ctrls=6

# 2. Feature를 SSD에 기록
python tensor_write.py \
  --path /data/igb/full/node_feat.npy \
  --num_ssd 4 --ssd_list 0,1,2,3 \
  --page_size 8 --num_ele 300000000000

# 3. PageRank 기반 CPU 버퍼 노드 선별
python page_rank_node_list_gen.py \
  --dataset_size full --path /data/igb/

# 4. GIDS로 GNN 학습
python homogenous_train.py \
  --dataset_size full --path /data/igb/ \
  --model_type sage --num_layers 3 \
  --batch_size 1024 --fan_out "10,15,25" \
  --GIDS --num_ssd 4 --ssd_list 0,1,2,3 \
  --cache_size 8 --page_size 8 \
  --cpu_buffer --cpu_buffer_percent 0.2 \
  --pin_file pagerank_nodes.pt \
  --window_buffer --wb_size 6 \
  --accumulator
```
