# Window Buffering & Storage Access Accumulator 분석

## 1. Window Buffering

### 1.1 목적과 원리

```
문제: 배치마다 새로운 노드를 SSD에서 읽으면
      캐시 Cold Start로 인해 첫 접근이 느리다

해결: 다음 N개 배치의 노드들을 미리 페이지 캐시에 프리페치

┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
│ Batch 0  │ │ Batch 1  │ │ Batch 2  │ │ Batch 3  │ │ Batch 4  │
│ 현재처리 │ │ 프리페치 │ │ 프리페치 │ │ 프리페치 │ │ 아직안함 │
└──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘
     ▲              ▲           ▲           ▲
     │              └───────────┴───────────┘
     │              Window Buffer (wb_size=3)
  처리 중          이 노드들의 페이지를
                   미리 캐시에 올려둠
```

### 1.2 초기 채움 (fill_wb)

```
GIDS.py :: fill_wb(it, num)   Lines 304-314

def fill_wb(self, it, num):
    for _ in range(num):
        try:
            batch = next(it)
            self.window_buffer.append(batch)
        except StopIteration:
            break

    # Window Buffer에 담긴 배치들의 노드 프리페치
    for wb_batch in self.window_buffer:
        input_nodes = wb_batch[0]    # 또는 dict
        self.window_buffering(input_nodes)

최초 1회 실행 (wb_init = True일 때)
→ wb_size개 배치를 미리 읽어서 window_buffer 리스트에 저장
→ 각 배치의 노드들에 대해 프리페치 요청
```

### 1.3 Window Buffering 커널

```
GIDS.py :: window_buffering(batch)   Lines 278-300

def window_buffering(self, batch):
    │
    ├── [Homogeneous]
    │   index = batch.to(self.gids_device)
    │   num_pages = len(index) * dim * 4 / page_size
    │   hash_off = 0
    │
    └── [Heterogeneous]
        for ntype in batch:
            index = batch[ntype].to(self.gids_device)
            hash_off = heterograph_map[ntype]  ← 노드 타입 오프셋
            num_pages = ...

    BAM_FS.set_window_buffering(
        index.data_ptr(),    # 노드 인덱스 GPU 포인터
        num_pages,           # 프리페치할 페이지 수
        hash_off             # 오프셋
    )
    │
    └── gids_nvme.cu :: set_window_buffering()  Lines 106-111

        set_window_buffering_kernel<<<num_pages, 32>>>(
            a->d_array_ptr,
            idx,
            num_pages,
            hash_off
        )
        │
        └── gids_kernel.cu :: set_window_buffering_kernel  Lines 115-122

            __global__ void set_window_buffering_kernel(
                array_d_t<T> *dr,
                int64_t id_idx,
                int64_t num_pages,
                int hash_off
            ) {
                int tid = blockIdx.x;
                if (tid < num_pages) {
                    bam_ptr<T> ptr = dr->get_bam_ptr();
                    ptr.set_window_buffer_counter(id_idx + hash_off);
                    //  ^^^^^^^^^^^^^^^^^^^^^^^^^
                    // Page Cache에 "이 페이지 곧 필요하다" 힌트
                    // prefetch_counter를 설정하여
                    // 해당 페이지가 eviction 대상에서 보호됨
                }
            }

효과:
  프리페치된 페이지 → Cache Hit 확률 증가
  단, 실제 SSD I/O를 미리 수행하는 것은 아님 (힌트만 설정)
  Page Cache가 eviction 시 이 페이지들을 우선 유지
```

### 1.4 런타임 동작

```
fetch_feature() 내 Window Buffering 흐름:

[매 배치마다]
│
├── batch = self.window_buffer.pop(0)   ← 미리 저장된 배치 사용
│   (window_buffer가 비어있으면 next(it)로 새로 가져옴)
│
├── 새 배치를 window_buffer 끝에 추가
│   try:
│     new_batch = next(it)
│     self.window_buffer.append(new_batch)
│     self.window_buffering(new_batch)  ← 프리페치
│
└── 현재 batch로 feature read 수행
    BAM_FS.read_feature(...)
    → 프리페치된 페이지 → Cache Hit 가능성 높음


슬라이딩 윈도우:
  Step 0: buffer = [B1, B2, B3]  → B0 처리, B1 프리페치 완료
  Step 1: buffer = [B2, B3, B4]  → B1 처리, B4 프리페치
  Step 2: buffer = [B3, B4, B5]  → B2 처리, B5 프리페치
  ...
```

---

## 2. Storage Access Accumulator

### 2.1 목적과 원리

```
문제:
  작은 배치 → 적은 I/O 요청 → SSD 대역폭 미활용

  예: batch_size=512, 캐시 히트 70%
      실제 SSD 접근 = 512 × 0.3 = ~150개 요청
      SSD 4개 × 대역폭 5.8GB/s = 23.2 GB/s 잠재력
      150 × 4KB = 0.6 MB → 대역폭의 극히 일부만 사용

해결:
  여러 배치를 누적하여 충분한 I/O 요청이 모이면 한 번에 실행
  → SSD 큐 깊이 ↑ → 내부 병렬성 활용 → 대역폭 포화

  ┌────────┐ ┌────────┐ ┌────────┐
  │Batch 0 │ │Batch 1 │ │Batch 2 │  누적
  │ 150 I/O│ │ 180 I/O│ │ 170 I/O│
  └───┬────┘ └───┬────┘ └───┬────┘
      └──────────┴──────────┘
               500 I/O
               ≥ required_accesses?
               → Yes: 한 번에 실행
               → No: 더 누적
```

### 2.2 Required Accesses 공식

```
GIDS.py :: set_required_storage_access()   Lines 317-320

def set_required_storage_access(self, bw, l_ssd, l_system, num_ssd, p):
    self.required_accesses = int(
        (p * bw * 1024 / self.page_size *
         (l_ssd + l_system) * num_ssd) / (1 - p)
    )

변수 설명:
  p          = 캐시 미스 확률 (0~1)
  bw         = SSD 피크 대역폭 (GB/s)  (기본 5.8)
  l_ssd      = SSD 지연 (μs)           (기본 11.0)
  l_system   = 시스템 지연 (μs)         (기본 20.0)
  num_ssd    = SSD 개수                 (기본 4)
  page_size  = 페이지 크기              (기본 4096)

계산 예시:
  p = 0.3 (30% 미스)
  bw = 5.8 GB/s
  l_ssd = 11 μs
  l_system = 20 μs
  num_ssd = 4
  page_size = 4096

  required = (0.3 × 5.8 × 1024 / 4096 × (11 + 20) × 4) / (1 - 0.3)
           = (0.3 × 1.45 × 31 × 4) / 0.7
           = 54.0 / 0.7
           ≈ 77 accesses

의미: SSD 대역폭을 포화시키려면 최소 77개의 I/O가
      동시에 진행되어야 한다
```

### 2.3 Accumulator 동작 흐름

```
GIDS.py :: fetch_feature() 내 accumulator 경로   Lines 341-465

[accumulator_flag = True일 때]

self.accum_nodes = []       # 누적 노드 인덱스
self.accum_batches = []     # 누적 배치 데이터
self.accum_count = 0        # 누적 접근 수

매 호출 시:
│
├── batch = next(it)
│   input_nodes = batch[0]
│
├── 예상 SSD 접근 수 계산
│   prev_cpu = self.prev_cpu_access
│   estimated_miss = len(input_nodes) - estimated_cache_hit
│
├── 누적
│   self.accum_nodes.append(input_nodes)
│   self.accum_batches.append(batch)
│   self.accum_count += estimated_miss
│
├── 충분히 누적되었는가?
│   if self.accum_count >= self.required_accesses:
│   │
│   │  [Heterogeneous 경우]
│   │  ┌──────────────────────────────────────────────────┐
│   │  │ 누적된 배치들을 노드 타입별로 분류                │
│   │  │                                                  │
│   │  │ i_ptr_list = []     # 출력 텐서 포인터 리스트    │
│   │  │ i_index_list = []   # 인덱스 리스트              │
│   │  │ num_index = []      # 각 배치의 노드 수          │
│   │  │ key_off = []        # 각 노드 타입의 오프셋       │
│   │  │                                                  │
│   │  │ for each accumulated batch:                      │
│   │  │   for each node_type:                            │
│   │  │     out_buf = torch.zeros(n, dim, device=cuda)   │
│   │  │     i_ptr_list.append(out_buf.data_ptr())        │
│   │  │     i_index_list.append(nodes.data_ptr())        │
│   │  │     num_index.append(len(nodes))                 │
│   │  │     key_off.append(heterograph_map[ntype])       │
│   │  │                                                  │
│   │  │ BAM_FS.read_feature_merged_hetero(               │
│   │  │     len(i_ptr_list),  # num_iter                 │
│   │  │     i_ptr_list,       # 출력 포인터들            │
│   │  │     i_index_list,     # 인덱스 포인터들          │
│   │  │     num_index,        # 각각의 노드 수           │
│   │  │     dim, cache_dim,                              │
│   │  │     key_off           # 오프셋들                 │
│   │  │ )                                                │
│   │  └──────────────────────────────────────────────────┘
│   │
│   │  [Homogeneous 경우]
│   │  BAM_FS.read_feature_merged(
│   │      num_accumulated,
│   │      i_ptr_list,
│   │      i_index_list,
│   │      num_index,
│   │      dim, cache_dim
│   │  )
│   │
│   │  → gids_nvme.cu에서 각 배치를 별도 CUDA 스트림으로 실행
│   │  → SSD에 동시에 더 많은 I/O 요청 → 대역폭 포화
│   │
│   │  결과 반환:
│   │  for each accumulated batch:
│   │    yield (batch_data, features)
│   │
│   │  카운터 리셋:
│   │  self.accum_count = 0
│   │  self.accum_nodes = []
│   │
│   └── 아직 부족하면 → 다음 호출까지 계속 누적
│
└── CPU 접근 카운터 업데이트
    self.prev_cpu_access = BAM_FS.get_cpu_access_count()
```

### 2.4 Merged Read 실행

```
gids_nvme.cu :: read_feature_merged()   Lines 259-318

void read_feature_merged(
    int num_iter,                    // 누적된 배치 수
    std::vector<uint64_t> i_ptr_list,     // 출력 포인터들
    std::vector<uint64_t> i_index_list,   // 인덱스 포인터들
    std::vector<int64_t> num_index,       // 각 배치의 노드 수
    int dim, int cache_dim
) {
    cudaStream_t stream[num_iter];

    for (int i = 0; i < num_iter; i++) {
        cudaStreamCreate(&stream[i]);

        float* out = (float*)i_ptr_list[i];
        int64_t* index = (int64_t*)i_index_list[i];
        int64_t n_idx = num_index[i];

        int g_size = (n_idx + n_warp - 1) / n_warp;

        // CPU 버퍼 유무에 따라 커널 선택
        if (cpu_buffer_flag) {
            read_feature_kernel_with_cpu_backing_memory
            <<<g_size, 128, 0, stream[i]>>>(...)
        } else {
            read_feature_kernel
            <<<g_size, 128, 0, stream[i]>>>(...)
        }
    }

    cudaDeviceSynchronize();
}

실행 패턴:
  Stream 0: ├──── Batch 0 커널 ────┤
  Stream 1:    ├──── Batch 1 커널 ────┤
  Stream 2:       ├──── Batch 2 커널 ────┤
                  ↑
            GPU가 자동으로 오버랩
            SSD에 더 많은 동시 I/O
```

## 3. Window Buffering vs Storage Accumulator 비교

```
┌───────────────────┬───────────────────────┬────────────────────────┐
│ 항목               │ Window Buffering      │ Storage Accumulator    │
├───────────────────┼───────────────────────┼────────────────────────┤
│ 목적               │ 캐시 히트율 향상       │ SSD 대역폭 포화        │
│ 방식               │ 다음 배치 프리페치     │ I/O 요청 누적 후 실행  │
│ 캐시 영향          │ 프리페치 힌트 설정     │ 직접적 영향 없음       │
│ 추가 메모리         │ 배치 wb_size개 저장    │ 누적 텐서 저장          │
│ 지연 영향          │ 프리페치 오버헤드 소량 │ 첫 배치 결과 지연       │
│ SSD 활용           │ 간접 (히트율↑→미스↓)  │ 직접 (큐 깊이↑)        │
│ 사용 시나리오      │ 항상 유효              │ 작은 배치일 때 효과적   │
│ 인자               │ --window_buffer       │ --accumulator          │
│                    │ --wb_size             │ --bw, --l_ssd, etc.    │
│ 동시 사용          │ 가능 (독립적)          │ 가능 (독립적)          │
└───────────────────┴───────────────────────┴────────────────────────┘
```
