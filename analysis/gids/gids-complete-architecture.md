# GIDS 완전 아키텍처 분석 — 한 파일로 전체 이해

> 본 문서는 GIDS(GPU-Initiated Direct Storage) 프로젝트를 **한 파일만 읽고도 전체 동작을 완전히 이해**할 수 있도록 작성되었다.  
> 코드 참조: `gids-study/` (Python/CUDA 전체 주석 완료).  
> 논문: Park et al., "Accelerating Sampling and Aggregation Operations in GNN Frameworks with GPU-Initiated Direct Storage Accesses", VLDB 2024.

---

## 목차

1. [문제 정의: 왜 GIDS가 필요한가](#1-문제-정의)
2. [전체 시스템 레이어 구조](#2-전체-시스템-레이어-구조)
3. [핵심 자료구조 관계도](#3-핵심-자료구조-관계도)
4. [초기화 플로우 (시스템 부트)](#4-초기화-플로우)
5. [학습 1 Iteration 플로우 — 완전 추적](#5-학습-1-iteration-플로우)
6. [3가지 Feature Read 경로 상세](#6-3가지-feature-read-경로)
7. [GPU 커널 내부 분석](#7-gpu-커널-내부-분석)
8. [BaM I/O 경로: GPU → NVMe SSD](#8-bam-io-경로)
9. [Window Buffering & Storage Access Accumulator](#9-window-buffering--storage-access-accumulator)
10. [이종 그래프(Heterogeneous Graph) 지원](#10-이종-그래프-지원)
11. [Feature 쓰기 (SSD 사전 기록) 플로우](#11-feature-쓰기-플로우)
12. [pybind11 바인딩 계층](#12-pybind11-바인딩-계층)
13. [핵심 파라미터 레퍼런스](#13-핵심-파라미터-레퍼런스)
14. [성능 설계 원칙 요약](#14-성능-설계-원칙-요약)

---

## 1. 문제 정의

### 1-1. GNN 학습의 전통적 병목

```
┌───────────────────────────────────────────────────────────────────────┐
│                  기존 GNN Feature Gather 병목 구조                     │
│                                                                       │
│  ① DGL Sampler        ② Feature Gather          ③ GNN Train         │
│  ┌──────────────┐      ┌──────────────────┐      ┌──────────────┐    │
│  │   CPU        │ IDs  │     CPU          │ copy │    GPU       │    │
│  │   (Graph     │ ───► │  (DRAM, ~TB)     │ ───► │  (VRAM 24GB) │   │
│  │   Topology)  │      │  feature[node_id] │      │  GNN Layer   │    │
│  └──────────────┘      └──────────────────┘      └──────────────┘    │
│                              │    ▲                                   │
│                    DRAM 한계 │    │ PCIe 12~16 GB/s (단방향)         │
│                    IGB-Full  │    │ 수백 만 노드 랜덤 access           │
│                    feature=  │    │ → NUMA, cache miss 폭발           │
│                    1.2 TB    │                                        │
│                              │                                        │
│                         ★ 이것이 병목 ★                               │
│                         CPU 랜덤 메모리 접근 + 느린 PCIe copy         │
└───────────────────────────────────────────────────────────────────────┘

실제 측정 (IGB-Full, GraphSAGE 3-hop):
  - Sampling:        ~15 ms / batch
  - Feature Gather:  ~95 ms / batch  ← 병목 (전체의 80%)
  - GNN Forward:     ~12 ms / batch
  - GNN Backward:    ~18 ms / batch
```

### 1-2. GIDS의 해결 방식

```
┌───────────────────────────────────────────────────────────────────────┐
│                  GIDS: GPU-Initiated Direct Storage                    │
│                                                                       │
│  ① DGL Sampler        ② GIDS Feature Gather      ③ GNN Train        │
│  ┌──────────────┐      ┌──────────────────┐        ┌──────────────┐  │
│  │   GPU        │ IDs  │    GPU           │        │    GPU       │  │
│  │   (CSC 그래프 │ ───► │  (BaM 페이지캐시) │ ──────►│  GNN Layer   │  │
│  │   pin memory)│      │  Page Cache Hit  │        │  (동일 VRAM) │  │
│  └──────────────┘      └────────┬─────────┘        └──────────────┘  │
│                                 │ Miss                                 │
│                                 │ GPU가 직접 SSD에 NVMe 명령 발행      │
│                                 │ (CPU/DRAM 우회, P2P DMA)            │
│                                 ▼                                     │
│                        ┌─────────────────┐                           │
│                        │   NVMe SSD(s)   │  Feature Table            │
│                        │   /dev/libnvm0  │  Node_ID 순 저장           │
│                        │   ~ /libnvm5    │  Striping 가능             │
│                        └─────────────────┘                           │
│                                                                       │
│  핵심 차이:                                                            │
│  ① CPU 완전 우회: GPU→SSD 직접 P2P DMA                              │
│  ② 랜덤 Access: GPU 내 병렬 Warp가 각각 비동기 I/O                   │
│  ③ 캐시 재사용: 핫 노드는 GPU 페이지 캐시에 상주                     │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 2. 전체 시스템 레이어 구조

```
┌──────────────────────────────────────────────────────────────────────────┐
│  Layer 5: Python Training Script  (evaluation/*.py)                       │
│                                                                           │
│   GIDS_DGLDataLoader(g, train_nid, sampler, GIDS=gids_loader)             │
│   for batch in loader:                                                    │
│     blocks, input_nodes, feature = batch   ← fetch_feature 결과          │
│     pred = model(blocks, feature)                                         │
│     loss.backward() → optimizer.step()                                    │
├──────────────────────────────────────────────────────────────────────────┤
│  Layer 4: GIDS Python API  (GIDS_Setup/GIDS/GIDS.py)                     │
│                                                                           │
│   ┌─────────────────────────────────────────────────────────────────┐    │
│   │ GIDS_DGLDataLoader (torch.utils.data.DataLoader 서브클래스)      │    │
│   │  ├─ CollateWrapper: DGL sampler.sample() → MFG block            │    │
│   │  └─ _PrefetchingIter: __next__ → GIDS.fetch_feature()          │    │
│   └─────────────────────────────────────────────────────────────────┘    │
│   ┌─────────────────────────────────────────────────────────────────┐    │
│   │ GIDS (메인 엔진)                                                  │    │
│   │  ├─ fetch_feature()   — 경로 분기 (직접/CPU버퍼/윈도우/누적기)   │    │
│   │  ├─ window_buffering()— 프리패치                                 │    │
│   │  ├─ set_cpu_buffer()  — 핫 노드 CPU 고정                        │    │
│   │  └─ store_tensor()    — SSD 데이터 기록 (사전작업)               │    │
│   └─────────────────────────────────────────────────────────────────┘    │
├──────────────────────────────────────────────────────────────────────────┤
│  Layer 3: C++/CUDA Module  (gids_module/)                                 │
│                                                                           │
│   bam_nvme.h  ← 구조체 선언                                               │
│   gids_nvme.cu  ← 호스트 C++ 구현 + pybind11 바인딩                      │
│   │  ├─ GIDS_Controllers::init_GIDS_controllers()                        │
│   │  └─ BAM_Feature_Store<T>::                                           │
│   │       init_controllers / read_feature* / store_tensor / ...          │
│   gids_kernel.cu  ← GPU __global__ 커널 (nvme.cu에 #include됨)           │
│        ├─ read_feature_kernel<T>                                          │
│        ├─ read_feature_kernel_with_cpu_backing_memory<T>                  │
│        ├─ set_cpu_buffer_kernel<T>                                        │
│        ├─ set_cpu_buffer_data_kernel<T>                                   │
│        ├─ set_window_buffering_kernel<T>                                  │
│        └─ write_feature_kernel2<T>                                        │
│                      │                                                    │
│             [pybind11 경계] Python ↔ C++                                  │
├──────────────────────────────────────────────────────────────────────────┤
│  Layer 2: BaM Library  (bam/ 서브모듈)                                    │
│                                                                           │
│   [호스트 측]                                                              │
│   Controller    — /dev/libnvm* 오픈, admin queue, GPU I/O queue 할당      │
│   page_cache_t  — GPU-resident 페이지 캐시 호스트 핸들                    │
│   range_t<T>    — 논리 인덱스 ↔ SSD LBA 매핑 (STRIPE/REPLICATE)          │
│   array_t<T>    — range 위의 배열 추상화 (커널에 d_array_ptr 제공)        │
│                                                                           │
│   [디바이스 측]                                                            │
│   bam_ptr<T>    — GPU 코드에서 SSD를 마치 배열처럼 read/write             │
│   page_cache_d_t— GPU에 상주하는 실제 페이지 캐시 (VRAM)                  │
│   range_d_t<T>  — get_cpu_offset() : CPU 버퍼 hit 판정                   │
│   array_d_t<T>  — seq_read() / seq_write()                               │
│   nvm_queue     — GPU가 직접 SQE 제출 / CQE 폴링하는 NVMe 큐             │
│                                                                           │
│   [핵심 API 흐름]                                                          │
│   bam_ptr.read(idx)                                                       │
│     → array_d_t::seq_read(idx)                                            │
│     → page_cache_d_t::acquire_page(page_id)                               │
│       → Hit: 캐시 내 offset에서 load                                      │
│       → Miss: nvm_queue.sq_enqueue(SQE) → doorbell kick                  │
│               → CQE poll → 완료 → 페이지 캐시에 저장 → load              │
├──────────────────────────────────────────────────────────────────────────┤
│  Layer 1: BaM Kernel Module  (libnvm_helper.ko)                           │
│                                                                           │
│   /dev/libnvm0 ~ /dev/libnvm5  — char device (SSD당 1개)                  │
│   BAR0 MMIO mmap       — doorbell register를 GPU VA에 매핑                │
│   nvidia_p2p_get_pages — GPU DMA 버퍼를 SSD DMA 엔진에 노출              │
│   PCIe P2P routing     — GPU ↔ SSD DMA를 CPU/DRAM 경유 없이 직결         │
├──────────────────────────────────────────────────────────────────────────┤
│  Layer 0: Hardware                                                        │
│                                                                           │
│   GPU VRAM  ◄──── PCIe P2P DMA (CPU 우회) ────►  NVMe SSD(s)             │
│   (Page Cache, SQ/CQ, Doorbell 포함)              (1~6개, Striping)       │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 3. 핵심 자료구조 관계도

```
Python 계층
────────────────────────────────────────────────────────────────
GIDS (GIDS.py)
 ├─ BAM_FS: BAM_Feature_Store_float   → C++ BAM_Feature_Store<float>
 │          (or BAM_Feature_Store_long → C++ BAM_Feature_Store<int64_t>)
 ├─ GIDS_controller: GIDS_Controllers pybind 객체
 ├─ graph_GIDS: BAM_Feature_Store_long  (그래프 토폴로지용, 선택적)
 ├─ window_buffer:      [] (샘플링 배치 큐)
 └─ return_torch_buffer:[] (fetch 완료 feature 텐서 큐)

C++ 계층 (bam_nvme.h / gids_nvme.cu)
────────────────────────────────────────────────────────────────
GIDS_Controllers
 ├─ ctrls_paths[6]: "/dev/libnvm0" .. "/dev/libnvm5"  (고정 상수)
 ├─ ctrls: vector<Controller*>    (BaM Controller 객체 벡터)
 │          └─ Controller         (BaM 호스트 C++ 래퍼)
 │               ├─ admin_queue   (Identify, Set Features)
 │               ├─ d_qps[]       (GPU-resident SQ/CQ 배열, numQueues개)
 │               └─ BAR0 매핑 (doorbell 주소)
 ├─ n_ctrls: uint32           (사용할 SSD 수)
 ├─ queueDepth: uint64        (SQ/CQ 엔트리 수, 기본 1024)
 └─ numQueues: uint64         (큐 쌍 수, 기본 128)

BAM_Feature_Store<TYPE>              ← Python BAM_Feature_Store_float/long
 ├─ ctrls: vector<Controller*>       ← GIDS_Controllers에서 공유 복사
 │
 ├─ h_pc: page_cache_t*              (GPU 페이지 캐시 호스트 핸들)
 │   ├─ d_pc_ptr: page_cache_d_t*   (GPU VRAM 내 실제 캐시 구조체)
 │   │   ├─ cache[n_pages]           (캐시 슬롯 배열, 각 pageSize 바이트)
 │   │   ├─ tags[n_pages]            (page_id ↔ slot 매핑)
 │   │   └─ state[n_pages]           (valid/dirty/locked 비트)
 │   └─ pdt.d_ctrls: Controller**   (커널에서 SSD 접근 시 사용)
 │
 ├─ h_range: range_t<TYPE>*          (논리 idx ↔ SSD LBA 매핑)
 │   ├─ d_range_ptr: range_d_t*     (GPU 측 range 뷰)
 │   │   ├─ get_cpu_offset(row)     (CPU 버퍼 매핑 비트 조회)
 │   │   └─ set_cpu_buffer(row, slot)(CPU 버퍼 슬롯 등록)
 │   └─ STRIPE 매핑: page_i → ctrls[i % n_ctrls]의 SSD
 │
 ├─ a: array_t<TYPE>*                (range 위의 배열 추상화)
 │   └─ d_array_ptr: array_d_t<TYPE>*(모든 커널의 1st 인자)
 │       └─ seq_read(idx) → page_cache → NVMe
 │
 ├─ d_range: range_d_t<TYPE>*        ← h_range->d_range_ptr 캐스팅
 │
 ├─ CPU_buffer: GIDS_CPU_buffer<TYPE>
 │   ├─ cpu_buffer: TYPE*            (cudaHostAlloc, 호스트 주소)
 │   ├─ device_cpu_buffer: TYPE*     (동일 버퍼의 GPU 주소, P2P)
 │   ├─ cpu_buffer_dim: uint64       (feature 차원)
 │   └─ cpu_buffer_len: uint64       (버퍼 노드 수)
 │
 ├─ d_cpu_access: unsigned int*      (GPU atomic 카운터, device)
 │
 ├─ cpu_buffer_flag: bool            (CPU 버퍼 활성 여부)
 └─ seq_flag: bool                   (선형/희소 매핑 모드)

메모리 공간 매핑 (선언 이후 불변)
────────────────────────────────────────────────────────────────
         Host DRAM         GPU VRAM                  SSD
  ┌──────────────┐   ┌──────────────────┐   ┌──────────────────┐
  │ cpu_buffer   │←→ │device_cpu_buffer  │   │  Feature Table   │
  │ (pinned)     │   │ (P2P mapped)     │   │  (노드ID 순)      │
  │              │   │                  │   │                  │
  │              │   │  page_cache      │ ◄─│  STRIPE 배치     │
  │              │   │  (VRAM cache)    │   │  (SSD 0,1,2...)  │
  │              │   │                  │   │                  │
  │              │   │  SQ / CQ        │ ─►│  NVMe I/O        │
  │              │   │  (GPU resident) │   │  명령 처리        │
  └──────────────┘   └──────────────────┘   └──────────────────┘
```

---

## 4. 초기화 플로우

```
학습 스크립트 시작
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│ Step 1: 데이터셋 로드 (dataloader.py)                        │
│                                                             │
│  IGB260MDGLDataset.process()                                │
│  → np.memmap("node_feat.npy", dtype='float32', mode='r')   │
│  → dgl.graph((src, dst)) → .formats('csc')                 │
│  → graph.ndata['feat'] = ... (baseline만, GIDS는 불필요)    │
│  → train_nid = graph.ndata['train_mask'].nonzero()          │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Step 2: GIDS 인스턴스 생성  (GIDS.__init__)                  │
│                                                             │
│  GIDS(page_size=4096,                                       │
│       off=0,           ← SSD 시작 byte offset              │
│       num_ele=300G,    ← feature 원소 수                   │
│       num_ssd=4,                                            │
│       ssd_list=[0,1,2,3],                                   │
│       cache_size=2000, ← GPU 페이지 캐시 MB                │
│       cache_dim=1024,  ← feature 차원                      │
│       ctrl_idx=0)      ← CUDA device index                 │
│                                                             │
│  내부 동작:                                                  │
│  ┌─────────────────────────────────────────────────┐        │
│  │ ① float/int64 바인딩 선택                        │        │
│  │    self.BAM_FS = BAM_Feature_Store_float()       │        │
│  │                                                 │        │
│  │ ② off 정규화                                     │        │
│  │    self.off = ceil(ceil(off/page_size)/num_ssd)  │        │
│  │    → SSD당 페이지 인덱스 기준으로 변환            │        │
│  │                                                 │        │
│  │ ③ Controller 벡터 초기화                         │        │
│  │    GIDS_controller.init_GIDS_controllers(        │        │
│  │        num_ssd=4, q_depth=1024, num_q=128,      │        │
│  │        ssd_list=[0,1,2,3])                      │        │
│  │    ↓                                            │        │
│  │    for i in [0,1,2,3]:                          │        │
│  │      ctrls.push_back(new Controller(            │        │
│  │          "/dev/libnvm{i}", ns=1,                │        │
│  │          cudaDev=0, qDepth=1024, nQueues=128))  │        │
│  │    (각 Controller 생성자: open → admin queue →  │        │
│  │     Identify → GPU-resident SQ/CQ 할당)         │        │
│  │                                                 │        │
│  │ ④ 페이지 캐시 / range / array 초기화            │        │
│  │    BAM_FS.init_controllers(ctrl, ps=4096,       │        │
│  │        off, cache_size, num_ele, num_ssd)       │        │
│  │    ↓                                            │        │
│  │    n_pages = cache_size*MB / page_size           │        │
│  │    h_pc = new page_cache_t(4096, n_pages, ...)  │        │
│  │    h_range = new range_t<float>(               │        │
│  │        0, num_ele, read_off,                    │        │
│  │        num_ele*sizeof(float)/page_size,         │        │
│  │        0, 4096, h_pc, cudaDev, STRIPE)          │        │
│  │    d_range = (range_d_t*)h_range->d_range_ptr   │        │
│  │    vr[0] = h_range                              │        │
│  │    a = new array_t<float>(num_ele, 0, vr, dev)  │        │
│  │    cudaMalloc(&d_cpu_access, sizeof(uint))       │        │
│  └─────────────────────────────────────────────────┘        │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Step 3: Constant CPU Buffer 설정 (선택적)                    │
│                                                             │
│  GIDS.cpu_backing_buffer(dim=1024, length=N_hot)           │
│  ↓                                                          │
│  cudaHostAlloc(size=N_hot*1024*4, flag=Mapped)             │
│    → cpu_buffer  (호스트 주소)                              │
│  cudaHostGetDevicePointer(cpu_buffer)                       │
│    → device_cpu_buffer  (GPU 주소)                         │
│  cpu_buffer_flag = True                                     │
│                                                             │
│  GIDS.set_cpu_buffer(pagerank_hot_nodes, N=N_hot)          │
│  ↓                                                          │
│  GPU로 핫노드 ID 전송 → set_cpu_buffer_kernel 런칭          │
│    → range_d_t::set_cpu_buffer(node_id, slot_idx)          │
│       (range 테이블에 "이 노드는 CPU 버퍼 슬롯 k에 있음" 기록) │
│  → set_cpu_buffer_data_kernel 런칭                          │
│    → 각 핫 노드의 feature를 SSD→CPU 버퍼로 복사            │
│  → seq_flag = False (희소 매핑 모드로 전환)                  │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Step 4: GIDS_DGLDataLoader 생성                             │
│                                                             │
│  loader = GIDS_DGLDataLoader(                               │
│      graph=g, indices=train_nid,                            │
│      graph_sampler=sampler, batch_size=1024,               │
│      dim=1024, GIDS=gids_loader)                           │
│  ↓                                                          │
│  graph.create_formats_() → CSC 포맷 준비                    │
│  graph._graph.pin_memory_() → 그래프 토폴로지 pin          │
│  create_tensorized_dataset(indices, batch_size, ...)        │
│  super().__init__(dataset, collate_fn=CollateWrapper(...))  │
└─────────────────────────────────────────────────────────────┘
```

---

## 5. 학습 1 Iteration 플로우

### 5-1. 완전 호출 체인 (단순 경로)

```
학습 루프: for batch in loader
                │
                ▼
 GIDS_DGLDataLoader.__iter__()
  → _PrefetchingIter(self, super().__iter__(), GIDS_Loader)
                │
                ▼
 _PrefetchingIter.__next__()
  → GIDS.fetch_feature(dim=1024, it=dataloader_it, device='cuda:0')
                │
    ┌───────────┼──────────────────────────────────────────┐
    │           │                                          │
    │  ┌────────▼──────────────────────────────────┐      │
    │  │  내부 iterator: next(it)                   │      │
    │  │   → torch DataLoader 내부 collate          │      │
    │  │   → CollateWrapper.__call__(items)          │      │
    │  │       → recursive_apply(items, .to(device)) │      │
    │  │       → graph_sampler.sample(g, seed_nids)  │      │
    │  │         (DGL NeighborSampler)               │      │
    │  │         → 2-hop MFG(block) 생성            │      │
    │  │         → input_nodes 결정                 │      │
    │  │       → remove_parent_storage_columns(batch)│      │
    │  │   반환: (input_nodes, output_nodes, blocks) │      │
    │  └─────────────────────────────────────────────┘      │
    │                                                        │
    │  window_buffer.append(batch)                           │
    │                                                        │
    │  batch = window_buffer.pop(0)   ← 현재 처리할 배치    │
    │  index = batch[0].to('cuda:0')  ← input_nodes GPU 이동│
    │  return_torch = torch.zeros([len(index), dim])         │
    │                                                        │
    │  BAM_FS.read_feature(                                  │
    │      return_torch.data_ptr(),  ← 출력 버퍼 GPU 주소   │
    │      index.data_ptr(),         ← 노드 ID GPU 주소     │
    │      len(index),               ← 노드 수              │
    │      dim=1024,                                         │
    │      cache_dim=1024,                                   │
    │      key_off=0)                                        │
    │           │                                            │
    └───────────┼────────────────────────────────────────────┘
                │
                ▼
  [pybind11 경계 통과 → C++]
  BAM_Feature_Store<float>::read_feature(...)
   ├─ tensor_ptr = (float*)i_ptr      ← 출력 포인터
   ├─ index_ptr  = (int64_t*)i_index  ← 인덱스 포인터
   ├─ b_size = 128 (blockDim)
   ├─ n_warp = 4   (128/32)
   ├─ g_size = ceil(num_index / 4)    ← gridDim
   │
   ├─ cudaDeviceSynchronize()  (타이머 시작)
   │
   ├─ if cpu_buffer_flag == False:
   │    read_feature_kernel<float>
   │        <<<g_size, 128>>>(a->d_array_ptr, tensor_ptr, index_ptr,
   │                          dim, num_index, cache_dim, key_off)
   │  else:
   │    read_feature_kernel_with_cpu_backing_memory<float>
   │        <<<g_size, 128>>>(a->d_array_ptr, d_range, tensor_ptr,
   │                          index_ptr, dim, num_index, cache_dim,
   │                          CPU_buffer, seq_flag, d_cpu_access, key_off)
   │
   ├─ cudaDeviceSynchronize()  (타이머 종료)
   ├─ cudaMemcpy(&cpu_access_count, d_cpu_access, ...)
   ├─ kernel_time += elapsed_ms
   └─ total_access += num_index
                │
                ▼
  [GPU 커널 실행 — §7 참조]
                │
                ▼
  return_torch (GPU 텐서, [num_nodes, 1024])
  → batch.append(return_torch)
  → 학습 스크립트에 반환:
      (input_nodes, output_nodes, blocks, feature)
```

### 5-2. Sequence 다이어그램

```
학습스크립트   _PrefetchingIter  GIDS.fetch_feature  BAM_FS   GPU커널   BaM/NVMe
    │               │                  │              │         │         │
    │  for batch    │                  │              │         │         │
    │──────────────►│                  │              │         │         │
    │               │  fetch_feature() │              │         │         │
    │               │─────────────────►│              │         │         │
    │               │                  │  next(it)    │         │         │
    │               │                  │─────────────►│(샘플링) │         │
    │               │                  │  (input_nids,blocks)   │         │
    │               │                  │◄─────────────│         │         │
    │               │                  │  read_feature│         │         │
    │               │                  │─────────────►│         │         │
    │               │                  │              │kernel() │         │
    │               │                  │              │────────►│         │
    │               │                  │              │         │ hit?    │
    │               │                  │              │         │──┐      │
    │               │                  │              │         │  │Yes   │
    │               │                  │              │         │◄─┘cache │
    │               │                  │              │         │  load   │
    │               │                  │              │         │ miss?   │
    │               │                  │              │         │──┐      │
    │               │                  │              │         │  │Yes   │
    │               │                  │              │         │  │SQE→  │
    │               │                  │              │         │  │────► │SSD
    │               │                  │              │         │  │◄─── CQE
    │               │                  │              │         │◄─┘      │
    │               │                  │              │ sync    │         │
    │               │                  │              │◄────────│         │
    │               │  (blocks, feat)  │              │         │         │
    │               │◄─────────────────│              │         │         │
    │  batch        │                  │              │         │         │
    │◄──────────────│                  │              │         │         │
    │ model(blocks, │                  │              │         │         │
    │  feat) →loss  │                  │              │         │         │
```

---

## 6. 3가지 Feature Read 경로

### 경로 A: BaM 직접 경로 (기본)

```
조건: cpu_buffer_flag = False  (Constant CPU Buffer 미설정)

read_feature_kernel<T><<<g_size, 128>>>

  각 Warp (32 threads) = 한 노드 담당
  ┌──────────────────────────────────────────┐
  │  row_index = index_ptr[warp_idx] + key_off │
  │                                           │
  │  for tid in [0, 32, 64, ..., dim):        │
  │    elem = ptr.read(row_index*dim + tid)   │
  │    out[warp_idx*dim + tid] = elem         │
  └──────────────────────────────────────────┘
                 │
                 ▼
      bam_ptr.read(logical_idx)
         │
         ▼
      array_d_t::seq_read(logical_idx)
         │
         ▼
      page_cache_d_t::acquire_page(page_id)
         │
    ┌────┴────┐
    │         │
   Hit       Miss
    │         │
    │         ▼
    │    nvm_queue.sq_enqueue(SQE)
    │         → doorbell kick (BAR0 MMIO)
    │         → PCIe P2P DMA (GPU→SSD)
    │         → SSD 처리
    │         → DMA 완료 (SSD→GPU page cache)
    │         → CQE 도착
    │         → cq_poll → cq_dequeue
    │         → page_cache 슬롯 유효화
    │
    ▼
  캐시 슬롯에서 원소 로드 → out_tensor에 기록
```

### 경로 B: Constant CPU Buffer Hybrid 경로

```
조건: cpu_buffer_flag = True  (초기화 시 핫 노드 설정 완료)

read_feature_kernel_with_cpu_backing_memory<T><<<g_size, 128>>>

  각 Warp = 한 노드 담당
  ┌────────────────────────────────────────────────────────┐
  │  row_index = index_ptr[warp_idx] + key_off             │
  │  cpu_off = range_d_t::get_cpu_offset(row_index)        │
  │                                                        │
  │  if seq_flag == True:   ← 선형 매핑 모드              │
  │    if row_index < cpu_buffer_len:                      │
  │      → CPU 버퍼 Hit                                    │
  │        atomicAdd(d_cpu_access, 1)  ← 통계 카운터      │
  │        temp = device_cpu_buffer[row_index*dim + tid]   │
  │        (PCIe를 통해 host-pinned 메모리 직접 읽기)       │
  │    else:                                               │
  │      → SSD 경로 (ptr.read)                             │
  │                                                        │
  │  if seq_flag == False:  ← 희소 매핑 모드              │
  │    if cpu_off & 0x1 == 1:                              │
  │      → CPU 버퍼 Hit                                    │
  │        slot = cpu_off >> 1   ← 슬롯 번호              │
  │        temp = device_cpu_buffer[slot*dim + tid]        │
  │    else:                                               │
  │      → SSD 경로 (ptr.read)                             │
  └────────────────────────────────────────────────────────┘

cpu_off 비트 구조:
  ┌────────────────────────────────────┬───┐
  │  슬롯 인덱스 (31 bits)              │ H │ H=1: CPU hit
  └────────────────────────────────────┴───┘
  bit 0 = hit 여부
  bit 1..31 = CPU 버퍼 내 슬롯 번호

데이터 경로 비교:
  CPU 버퍼 Hit:                     SSD 경로:
  device_cpu_buffer (GPU VA)         ptr.read → page cache → NVMe
    ↑                                  ↑
  PCIe (host → GPU)                  PCIe P2P (SSD → GPU)
  레이턴시: ~수백 ns                  레이턴시: ~수십 μs
```

### 경로 C: Window Buffering (Prefetch)

```
조건: window_buffering_flag = True

fill_wb(it, wb_size):             ← 최초 1회, epoch 시작
  for i in range(wb_size):
    batch = next(it)
    window_buffer.append(batch)
    window_buffering(batch)        ← 프리패치 실행

window_buffering(batch):
  input_tensor = batch[0].to('cuda:0')
  BAM_FS.set_window_buffering(input_tensor.data_ptr(), num_pages, key_off)
    ↓
  set_window_buffering_kernel<T><<<num_pages, 32>>>
    각 Block (= 1 warp, 32 threads):
      if threadIdx.x == 0:         ← warp당 1회만
        page_idx = index_ptr[blockIdx.x] + hash_off
        ptr.set_window_buffer_counter(page_idx * page_size/sizeof(T), 1)
          → BaM 페이지 캐시에 "이 페이지를 미리 확보(fetch)" 신호

효과:
  현재 배치 처리 중 → 다음 wb_size개 배치의 페이지가 미리 캐시에 로드됨
  → 다음 배치의 read_feature 시 대부분 Cache Hit → 레이턴시 감소

  타임라인:
  ┌──────────────────────────────────────────────────────────────────┐
  │ iter N:   [Sampling N] [WB: prefetch N+1, N+2, N+3] [Fetch N]   │
  │                        ├──────────────────────────┤              │
  │                        백그라운드에서 SSD → GPU Cache 로드        │
  │                                                                  │
  │ iter N+1: [Sampling N+1] [WB: prefetch N+4] [Fetch N+1 (Hit!)]  │
  │                                               ↑                  │
  │                                         대부분 Cache Hit          │
  └──────────────────────────────────────────────────────────────────┘
```

---

## 7. GPU 커널 내부 분석

### 7-1. Block/Warp/Thread 매핑

```
커널 런칭: read_feature_kernel<<<g_size, 128>>>

blockDim = 128  (= 4 warps)
gridDim  = ceil(num_nodes / 4)

전역 Thread 배치:

Block 0             Block 1             Block (g_size-1)
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│Warp0│Warp1│W2│W3│ │Warp0│Warp1│W2│W3│ │ ...             │
│node0│node1│n2│n3│ │node4│node5│n6│n7│ │                 │
└─────────────────┘ └─────────────────┘ └─────────────────┘
각 Warp (32 threads)  = 노드 1개 담당
각 Thread (tid 0..31) = feature 차원을 stride-32로 분할

예: dim=1024, warp_idx=5 (node_id=index_ptr[5])
  Thread  0: out[5*1024 +   0] = SSD[node*1024 +   0]
  Thread  1: out[5*1024 +   1] = SSD[node*1024 +   1]
  ...
  Thread 31: out[5*1024 +  31] = SSD[node*1024 +  31]
  (2nd round)
  Thread  0: out[5*1024 +  32] = SSD[node*1024 +  32]
  ...
  Thread 31: out[5*1024 + 1023]= SSD[node*1024 + 1023]
  → 총 ceil(1024/32) = 32 iterations per warp

인덱스 계산:
  bid      = blockIdx.x
  num_warps= blockDim.x / 32  = 4
  warp_id  = threadIdx.x / 32
  idx_idx  = bid * num_warps + warp_id  ← 담당 노드 인덱스
  tid      = threadIdx.x % 32           ← warp 내 레인
  row_index= index_ptr[idx_idx] + key_off
  elem_idx = row_index * cache_dim + tid
```

### 7-2. bam_ptr.read() 내부 동작

```
bam_ptr<T> ptr(dr);   ← 생성자: page_cache 핸들 확보 (저비용)

T val = ptr.read(logical_idx);
         │
         ▼
  page_id  = logical_idx / (page_size / sizeof(T))
  page_off = logical_idx % (page_size / sizeof(T))
         │
         ▼
  page_cache_d_t::acquire_page(page_id)
         │
    ┌────┴────────────────────────────────────────┐
    │ atomicCAS(tag[slot], INVALID, page_id)       │
    │   → Hit: 이미 유효한 슬롯 → 슬롯 잠금 없이 반환 │
    │   → Miss:                                    │
    │     1. 빈 슬롯 탐색 또는 LRU 교체             │
    │     2. nvm_queue.sq_enqueue(SQE):            │
    │        SQE.opc  = NVM_IO_READ (0x02)         │
    │        SQE.nsid = 1                          │
    │        SQE.slba = LBA(page_id, ssd_idx)     │
    │        SQE.nblks= page_size / lba_size       │
    │        SQE.prp1 = GPU DMA 버퍼 PA           │
    │        doorbell[sq_tail] = sq_tail++         │
    │        (PCIe MMIO write → SSD에 I/O 신호)   │
    │     3. cq_poll:                              │
    │        while cq[cq_head].phase != phase:    │
    │          CPU yield / GPU spin               │
    │        cq_head++                            │
    │        → 완료 확인                          │
    │     4. page_cache 슬롯에 데이터 복사됨      │
    └──────────────────────────────────────────────┘
         │
         ▼
  return cache[slot * page_size/sizeof(T) + page_off]
```

### 7-3. CPU 버퍼 설정 커널 상세

```
Step 1: set_cpu_buffer_kernel<<<ceil(N/1024), 1024>>>

  각 thread = 핫 노드 1개 담당
  idx  = threadIdx.x + blockIdx.x * blockDim.x
  if idx < num:
    node_id = idx_ptr[idx]    ← 핫 노드 ID
    slot    = idx             ← CPU 버퍼 슬롯 번호
    range_d_t::set_cpu_buffer(node_id, slot)
      → range 테이블에 기록:
        entry[node_id] = (slot << 1) | 0x1  ← hit=1, slot=slot

Step 2: set_cpu_buffer_data_kernel<<<N, 32>>>

  block = 핫 노드 슬롯 1개 (blockIdx.x = slot)
  bam_ptr<T> ptr(dr);
  if bid < num:
    idx = idx_ptr[bid]        ← 실제 node ID → SSD 위치
    for i in [tid, dim, +blockDim]:
      CPU_buffer[bid*dim + i] = ptr[idx*dim + i]
      ↑ CPU 버퍼(device 주소)    ↑ SSD에서 BaM으로 읽기

결과: CPU_buffer[0..N-1][0..dim-1]에 핫 노드 feature 적재 완료
      range_d_t에 각 노드의 슬롯 매핑 완료
      → 이후 read_feature에서 CPU 버퍼 hit 시 SSD 접근 없이 직접 반환
```

---

## 8. BaM I/O 경로

### 8-1. SSD 접근 전체 경로

```
GPU Warp에서 ptr.read(idx) 호출
          │
          ▼
┌─────────────────────────────────────────────────────────────────┐
│                 GPU Thread (device code)                         │
│                                                                  │
│  ┌──────────────────────────────────────────────────┐           │
│  │  page_cache_d_t (GPU VRAM)                        │           │
│  │  ┌──────────────────────────────────────────┐    │           │
│  │  │  slots[0] = [4096 bytes]  page_id=?      │    │           │
│  │  │  slots[1] = [4096 bytes]  page_id=?      │    │           │
│  │  │  ...                                     │    │           │
│  │  │  slots[n_pages-1] = ...                  │    │           │
│  │  └──────────────────────────────────────────┘    │           │
│  │  tags[]: atomicCAS로 Hit/Miss 판정              │           │
│  └──────────────────────────────────────────────────┘           │
│            │ Miss                                                │
│            ▼                                                     │
│  ┌──────────────────────────────────────────────────┐           │
│  │  nvm_queue (GPU VRAM에 상주하는 SQ/CQ)           │           │
│  │  ┌─────────────────────────────────────────┐     │           │
│  │  │ SQ (Submission Queue):                  │     │           │
│  │  │  SQ[sq_tail] = {                        │     │           │
│  │  │    opc: 0x02 (NVM_IO_READ)              │     │           │
│  │  │    nsid: 1                              │     │           │
│  │  │    slba: <LBA 계산값>                   │     │           │
│  │  │    nblks: page_size/LBA_size            │     │           │
│  │  │    prp1: <GPU DMA buf PA>               │     │           │
│  │  │  }                                      │     │           │
│  │  │  sq_tail++                              │     │           │
│  │  └─────────────────────────────────────────┘     │           │
│  └──────────────────────────────────────────────────┘           │
│            │                                                     │
│      Doorbell Write (PCIe MMIO)                                  │
│            │                                                     │
└────────────┼─────────────────────────────────────────────────────┘
             │  PCIe TLP (doorbell)
             ▼
┌─────────────────────────────────────────────────────────────────┐
│                    NVMe SSD Controller                           │
│                                                                  │
│  SQ fetch → NVMe 명령 파싱 → Internal DMA:                      │
│    NAND → SSD Cache → DMA 엔진                                  │
│    DMA 엔진: GPU DMA 버퍼(prp1)로 P2P 전송                      │
│    CQE 생성: {sq_head, status, phase}                            │
│    CQ에 기록 → MSI-X 인터럽트 또는 Polling 신호                  │
└─────────────────────────────────────────────────────────────────┘
             │  PCIe P2P DMA
             ▼
┌─────────────────────────────────────────────────────────────────┐
│  GPU DMA 버퍼 (GPU VRAM or pinned host)                          │
│  → page_cache 슬롯으로 복사                                      │
│  → CQE 완료 확인 (cq_poll)                                       │
│  → 데이터 반환                                                   │
└─────────────────────────────────────────────────────────────────┘

STRIPE 모드 LBA 계산:
  page_id = logical_idx * sizeof(T) / page_size
  ssd_idx = page_id % n_ctrls         ← 어떤 SSD
  lba_in_ssd = page_id / n_ctrls     ← 그 SSD 내 위치
  lba = (read_off / lba_size) + lba_in_ssd
  → ctrls[ssd_idx]의 큐에 SQE 제출
```

### 8-2. 다중 SSD Striping 레이아웃

```
feature 배열 (논리 순서, node_id 기준):
  [node0: 4KB] [node1: 4KB] [node2: 4KB] [node3: 4KB] ...
      │             │             │             │
      ▼             ▼             ▼             ▼
SSD 0           SSD 1           SSD 2           SSD 3   (4개 SSD)
  page 0          page 1          page 2          page 3
  [node0]         [node1]         [node2]         [node3]
  page 4          page 5          page 6          page 7
  [node4]         [node5]         [node6]         [node7]
  ...

효과:
  - 서로 다른 node_id 접근 → 서로 다른 SSD → 병렬 처리
  - 4개 SSD 이론 대역폭: 4 × 단일 SSD 대역폭
  - 실제: 캐시 miss 시 여러 warp가 각기 다른 SSD에 동시 I/O

Queue 배분 (큐 선택 로직, write_feature_kernel 참조):
  queue = tid % ctrls[ctrl]->n_qps   (128개 큐에 round-robin)
  → 128개 큐 × 4개 SSD = 512개 병렬 I/O 경로
```

---

## 9. Window Buffering & Storage Access Accumulator

### 9-1. Window Buffering 상세 타임라인

```
wb_size = 4 (다음 4배치를 미리 프리패치)

[Epoch 시작 - fill_wb 1회 실행]

  Iterator:  B0  B1  B2  B3  B4  B5  B6  B7  ...
  fill_wb:  [──────────────]  ← B0~B3 샘플링 + WB 프리패치
  window_buffer: [B0, B1, B2, B3]

  WB 커널 실행 (각 Bi에 대해):
    set_window_buffering_kernel → B0의 input_nodes 페이지를 page cache에 로드
    set_window_buffering_kernel → B1의 input_nodes 페이지를 page cache에 로드
    set_window_buffering_kernel → B2의 input_nodes 페이지를 page cache에 로드
    set_window_buffering_kernel → B3의 input_nodes 페이지를 page cache에 로드

[Iteration 1: fetch_feature 호출]

  next_batch = next(it) = B4  ← 샘플링
  window_buffer.append(B4)
  window_buffering(B4)         ← B4 프리패치

  batch = window_buffer.pop(0) = B0   ← 이미 프리패치 완료!
  read_feature(B0.input_nodes)        ← 대부분 Cache Hit

  window_buffer: [B1, B2, B3, B4]

[Iteration 2]
  next = B5 → 프리패치
  pop B1 → read (Cache Hit 기대)

[결과]
  각 iteration에서 "wb_size 배치 앞서서 프리패치"
  → 프리패치 기간 동안 SSD I/O가 GPU 연산과 중첩
  → Cache Hit율 ↑ → Feature Gather 레이턴시 ↓
```

### 9-2. Storage Access Accumulator 원리

```
배경: SSD는 랜덤 소량 I/O보다 대량 병렬 I/O에서 효율적
      mini-batch 하나(≈1024 노드)는 SSD 파이프라인을 충분히 채우기에 부족

Accumulator: 여러 배치를 모아서 한 번에 read_feature_merged 호출

required_accesses 계산:
  공식: accesses = p * (bw * 1024 / page_size) * (l_ssd + l_system) * n_ssd / (1-p)
  - p: 캐시 미스율 (예: 0.7)
  - bw: SSD 대역폭 GB/s (예: 6.4)
  - l_ssd: SSD 레이턴시 μs (예: 100)
  - l_system: 시스템 오버헤드 μs (예: 20)
  - n_ssd: SSD 개수 (예: 4)
  → accesses ≈ 1920 개 (동시에 in-flight해야 할 노드 수)

Accumulator 동작:
  ┌────────────────────────────────────────────────────────────┐
  │ 이미 fetch한 텐서가 버퍼에 있으면 바로 꺼내 반환         │
  │  return_torch_buffer.pop(0) → 즉시 반환                  │
  └────────────────────────────────────────────────────────────┘
  ┌────────────────────────────────────────────────────────────┐
  │ 아니면 누적:                                               │
  │   current_access = 0                                       │
  │   while current_access < required_accesses:                │
  │     batch = next(it)                                       │
  │     current_access += len(batch[0])                        │
  │     window_buffer.append(batch)                            │
  │     num_iter++                                             │
  │                                                            │
  │   [모든 배치의 인덱스/포인터 수집]                         │
  │   read_feature_merged(num_iter, ptr_list, idx_list, ...)   │
  │    → 내부: num_iter개 CUDA 스트림으로 동시 커널 런칭      │
  │    → 모든 배치가 동시에 SSD I/O → 파이프라인 포화         │
  │                                                            │
  │   return_torch_buffer에 num_iter개 텐서 저장               │
  │   첫 번째 pop(0) → 이번 iteration 결과                    │
  │   나머지는 다음 iteration에서 즉시 반환                    │
  └────────────────────────────────────────────────────────────┘

이점:
  ┌─────────────────────────────────────────────────────┐
  │ 단순 경로: iter1→SSD→iter2→SSD→iter3→SSD ...      │
  │             순차 I/O, SSD 파이프라인 비효율          │
  │                                                     │
  │ Accumulator: iter1+2+3 → SSD 동시 I/O → 결과 반환  │
  │              SSD 파이프라인 포화 → 처리량 최대화     │
  └─────────────────────────────────────────────────────┘
```

---

## 10. 이종 그래프 지원

### 10-1. Heterogeneous Graph에서의 Key Offset

```
IGB Heterogeneous (IGBH) 노드 타입 구성:

  타입          노드 수           SSD offset (bytes)
  ──────────    ─────────────     ─────────────────────
  paper         269,346,174       offset[paper]  = 0
  author        277,220,883       offset[author] = 269346174 * 4096
  fos (분야)    712,960           offset[fos]    = (author 끝 이후)
  institute     712,960           offset[inst]   = (fos 끝 이후)

heterograph_map = {
    'paper':    0,
    'author':   269346174 * page_size,
    'fos':      546567057 * page_size,
    'institute':547280017 * page_size,
}

read_feature_kernel에서:
  row_index = index_ptr[warp_idx] + key_off
              ↑ 타입 내 로컬 ID    ↑ 타입 시작 offset
  → 전체 feature 배열에서의 절대 위치로 변환
  → SSD 내 올바른 LBA 계산 가능
```

### 10-2. Hetero 학습 1 Iteration 플로우

```
batch[0] = {
  'paper': tensor([101, 5, 8902, ...]),   ← paper 타입 input_nodes
  'author': tensor([23, 891, ...]),        ← author 타입 input_nodes
  'fos': tensor([]),                       ← 빈 타입 (skip)
  'institute': tensor([12, ...]),
}

read_feature_hetero 호출 시:
  for key, v in batch[0].items():
    key_off = heterograph_map[key]
    stream[i] = new CUDA stream
    read_feature_kernel<<<g_size_i, 128, 0, stream[i]>>>(
        a->d_array_ptr, out_tensor[i], index[i],
        dim, num_index_i, cache_dim, key_off)

  → 4개 타입에 대해 4개 CUDA 스트림에서 동시 커널 실행
  → 서로 다른 SSD 영역을 동시 접근 → 병렬 처리
  → cudaStreamSynchronize 후 결과 반환

반환 구조:
  ret_ten = {
    'paper':    tensor([N_paper, 1024]),
    'author':   tensor([N_author, 1024]),
    'fos':      tensor([0, 1024]),     ← 빈 텐서
    'institute':tensor([N_inst, 1024]),
  }
```

---

## 11. Feature 쓰기 플로우

### 11-1. 학습 전 SSD 사전 기록 (tensor_write.py)

```
write_data.sh 실행:
  python tensor_write.py
    --dataset igbh
    --node_type paper
    --offset 0
    --num_ssd 4
    --ssd_list 0 1 2 3

tensor_write.py 동작:
  1. numpy memmap 로드:
     feat = np.memmap('paper/node_feat.npy', mode='r', dtype='float32')
     feat.shape = (269346174, 1024)

  2. GIDS 초기화 (write 용도):
     gids = GIDS(page_size=4096, off=0, num_ssd=4, ...)

  3. 청크 단위 기록:
     chunk_size = cache_size (페이지 캐시 크기)
     for start in range(0, total_nodes, chunk_size):
       chunk = feat[start:start+chunk_size]
       torch_chunk = torch.from_numpy(chunk).to('cuda:0')
       gids.store_tensor(torch_chunk, offset=start*page_size)

store_tensor 내부 (gids_nvme.cu):
  write_feature_kernel2<<<num_nodes, 128>>>(
      h_pc->pdt.d_ctrls, d_pc, a->d_array_ptr,
      t_ptr, dim, n_ctrls, offset/sizeof(TYPE))

  각 Block = 노드 1개:
    bam_ptr<T> ptr(dr)
    for i in [tid, dim, +blockDim]:
      ptr[row_index*dim + i] = in_tensor[row_index*dim + i + offset]
      ↑ BaM 페이지 캐시에 쓰기 (dirty 마킹)

  cudaDeviceSynchronize()
  h_pc->flush_cache()    ← dirty 페이지 전부 NVMe Write로 플러시
    → write_data(pc, queue, start_lba, n_blocks, tid)
    → NVMe Write SQE 제출 → SSD에 영구 기록

스트라이핑 결과:
  노드 0   → SSD 0 (LBA 0)
  노드 1   → SSD 1 (LBA 0)
  노드 2   → SSD 2 (LBA 0)
  노드 3   → SSD 3 (LBA 0)
  노드 4   → SSD 0 (LBA 1)
  ...
```

---

## 12. pybind11 바인딩 계층

### 12-1. 바인딩 구조

```
gids_nvme.cu의 PYBIND11_MODULE(BAM_Feature_Store, m):

Python 클래스               C++ 클래스                 템플릿
─────────────────────       ─────────────────────────   ────────────
BAM_Feature_Store_float  ↔  BAM_Feature_Store<float>    (feature)
BAM_Feature_Store_long   ↔  BAM_Feature_Store<int64_t>  (label/idx)
GIDS_Controllers         ↔  GIDS_Controllers             (Controller 관리)

Python 호출 → C++ 대응:
  bfs.init_controllers(ctrl, ps, off, cache_size, num_ele, num_ssd)
    → BAM_Feature_Store<float>::init_controllers(...)

  bfs.read_feature(out.data_ptr(), idx.data_ptr(), N, dim, cache_dim, key_off)
    → BAM_Feature_Store<float>::read_feature(uint64_t, uint64_t, int64_t, ...)

  bfs.read_feature_hetero(n, out_list, idx_list, n_list, dim, cd, key_off_list)
    → read_feature_hetero(int, vector<uint64_t>, vector<uint64_t>, ...)

포인터 전달 방식:
  Python: tensor.data_ptr()  → int (Python에서는 정수)
  C++ 수신: uint64_t i_ptr
  C++ 사용: TYPE* tensor_ptr = (TYPE*)i_ptr  ← 포인터로 재해석

타입 선택 (GIDS.__init__):
  if long_type:
    self.BAM_FS = BAM_Feature_Store.BAM_Feature_Store_long()
  else:
    self.BAM_FS = BAM_Feature_Store.BAM_Feature_Store_float()
  → C++ 템플릿이 컴파일 타임에 특수화
  → 런타임 선택은 Python 수준 클래스 선택으로 구현
```

---

## 13. 핵심 파라미터 레퍼런스

| 파라미터 | 위치 | 기본값 | 의미 | 영향 |
|---------|------|--------|------|------|
| `page_size` | GIDS.__init__ | 4096 | BaM I/O 단위 (bytes) | NVMe LBA 정렬, cache 슬롯 크기 |
| `cache_size` | GIDS.__init__ | 10 MB | GPU 페이지 캐시 크기 | Hit율. 클수록 VRAM 소비 ↑ |
| `num_ssd` | GIDS.__init__ | 1 | 스트라이핑 SSD 수 | 대역폭 ×num_ssd (이론) |
| `ssd_list` | GIDS.__init__ | [0..n-1] | 사용할 /dev/libnvm 인덱스 | SSD 선택 순서 = striping 순서 |
| `cache_dim` | GIDS.__init__ | 1024 | feature 차원 | read_feature cache_dim 인자 |
| `num_ele` | GIDS.__init__ | 300G | 전체 feature 원소 수 | range_t n_elems |
| `off` | GIDS.__init__ | 0 | SSD 시작 byte offset | range_t read_off |
| `wb_size` | GIDS.__init__ | 8 | Window Buffer 배치 수 | 프리패치 깊이 |
| `queueDepth` | GIDS_Controllers | 1024 | SQ/CQ 엔트리 수 | 동시 in-flight I/O |
| `numQueues` | GIDS_Controllers | 128 | Controller당 큐 쌍 수 | 큐 경합 감소 |
| `blkSize` | BAM_Feature_Store | 128 | CUDA block 크기 | = 4 warp = 4 노드/블록 |

### 파라미터 조정 가이드

```
SSD 수 늘리기 (대역폭 확대):
  GIDS(num_ssd=4, ssd_list=[0,1,2,3], ...)
  → 이론 4× 대역폭, 실제 ~3× (컨트롤러 오버헤드)

캐시 크기 확대 (Hit율 개선):
  GIDS(cache_size=4000, ...)  ← 4GB VRAM 사용
  → 자주 쓰는 노드(핫 노드) 더 많이 캐시 가능

CPU 버퍼 설정 (핫 노드 최적화):
  gids.cpu_backing_buffer(dim=1024, length=1000000)
  gids.set_cpu_buffer(pagerank_nodes, N=1000000)
  → 상위 100만 노드는 SSD 접근 없이 CPU pinned 메모리에서 직접 제공

Window Buffering + Accumulator 조합:
  GIDS(window_buffer=True, wb_size=8, accumulator_flag=True, ...)
  gids.set_required_storage_access(bw=6.4, l_ssd=100, l_system=20, n_ssd=4, p=0.7)
  → 프리패치 + 대량 병합 I/O로 SSD 파이프라인 최대 활용
```

---

## 14. 성능 설계 원칙 요약

### 14-1. 왜 빠른가

```
① CPU/DRAM 우회
   Feature Gather 시 CPU 개입 없음
   GPU → SSD 직접 P2P DMA (PCIe peer-to-peer)
   CPU NUMA, cache miss, memory bus 경합 없음

② GPU 병렬 랜덤 I/O
   1 Warp = 1 노드 = 1 NVMe I/O 요청
   수천 Warp 동시 실행 = 수천 병렬 SSD I/O
   (기존 CPU: 순차적 랜덤 access)

③ 페이지 캐시 재사용
   GNN의 거듭제곱 샘플링 특성상 핫 노드는 반복 접근
   GPU VRAM 캐시 Hit → 레이턴시 ~ns (SSD: ~100μs)

④ Constant CPU Buffer
   PageRank 상위 노드 → pinned 호스트 메모리 고정
   CPU 버퍼 Hit → PCIe P2P read (~수백 ns) → SSD 접근 완전 회피

⑤ Window Buffering
   현재 배치 처리 중 다음 배치 페이지 선행 로드
   I/O 대기 시간을 GPU 연산 뒤에 숨김 (overlap)

⑥ Storage Access Accumulator
   여러 배치를 묶어 동시 SSD I/O → 파이프라인 포화
   SSD 처리량 × (동시 배치 수) 까지 실효 대역폭 향상
```

### 14-2. 전체 데이터 플로우 요약

```
[사전 작업 (오프라인)]
  numpy feature 파일
    → tensor_write.py → write_feature_kernel2 → BaM → NVMe SSD (STRIPE 저장)
  PageRank 계산
    → page_rank_node_list_gen.py → 핫 노드 리스트 저장

[학습 루프 (온라인, per-epoch, per-iteration)]

  DGL 샘플러 (GPU)            GIDS Feature Fetch          GNN 학습 (GPU)
  ┌─────────────────┐         ┌────────────────────┐      ┌─────────────────┐
  │ seed_nids       │─────►  │ input_nodes         │      │ blocks          │
  │ NeighborSampler │ blocks  │                    │      │ features        │
  │ → MFG(blocks)  │───────► │ ① CPU 버퍼 Hit?   │──►  │ model(blocks,   │
  │ → input_nodes   │         │   → pinned memory  │      │       features) │
  └─────────────────┘         │                    │      │ → loss          │
                               │ ② Page Cache Hit?  │      │ → backward      │
                               │   → VRAM 읽기      │      │ → step          │
                               │                    │      └─────────────────┘
                               │ ③ SSD Miss?        │
                               │   → NVMe SQE 제출  │
                               │   → P2P DMA 완료   │
                               │   → 캐시에 저장     │
                               └────────────────────┘
                                     ↑
                               Window Buffer: 다음 배치 선행 프리패치
                               Accumulator: 여러 배치 묶어 I/O 포화
```

---

*문서 작성일: 2026-06-13*  
*코드 기준: gids-study main 브랜치 (9679422)*  
*관련 문서: [[00-comprehensive-architecture]] [[02-feature-read-flow]] [[04-window-buffering-and-accumulator]]*
