# GIDS Feature Read 전체 경로 분석

## 1. Python에서 CUDA 커널까지

```
GIDS_DGLDataLoader.__iter__()
│
└── _PrefetchingIter.__next__()
    │
    └── GIDS_Loader.fetch_feature(dim, it, device)
        │ GIDS.py:323-528
        │
        ├── batch = next(dataloader_it)
        │   → DGL이 sampling 수행
        │   → (input_nodes, seeds, blocks) 반환
        │
        │  input_nodes 예시 (homogeneous):
        │  tensor([42, 1337, 99999, 7, ...], device='cuda:0')
        │  → 이 노드들의 feature가 필요하다
        │
        ├── [Window Buffering 초기 채움]
        │   if self.window_buffering_flag and self.wb_init:
        │     self.fill_wb(it, self.wb_size)
        │     self.wb_init = False
        │
        ├── num_nodes = len(input_nodes)  (또는 dict values 합)
        │
        ├── return_torch_buffer = torch.zeros(
        │       num_nodes, dim,
        │       dtype=torch.float32,
        │       device=self.gids_device)
        │   → GPU 메모리에 결과 텐서 할당
        │
        ├── out_ptr = return_torch_buffer.data_ptr()  (GPU 주소)
        │   idx_ptr = input_nodes.data_ptr()          (GPU 주소)
        │
        └── self.BAM_FS.read_feature(
                out_ptr,        # uint64_t → float* (GPU)
                idx_ptr,        # uint64_t → int64_t* (GPU)
                num_nodes,      # int64_t
                dim,            # int (1024)
                cache_dim,      # int (1024)
                key_off         # uint64_t (0 for homogeneous)
            )
```

## 2. C++ read_feature() 메서드

```
gids_nvme.cu :: BAM_Feature_Store<float>::read_feature()
│ Lines 156-191
│
├── 포인터 캐스팅
│   float* out = (float*)i_ptr
│   int64_t* index = (int64_t*)i_index_ptr
│
├── 그리드/블록 크기 계산
│   blkSize = 128                          (4 warps)
│   n_warp = blkSize / 32 = 4             (warp당 노드 1개)
│   g_size = (num_index + n_warp - 1) / n_warp
│
│   예: num_index = 10000 노드
│       g_size = (10000 + 3) / 4 = 2500 블록
│       총 스레드 = 2500 × 128 = 320,000
│       활성 warp = 10000 (나머지는 early return)
│
├── 타이머 시작
│   gettimeofday(&st, NULL)
│
├── CUDA 커널 런치 (분기)
│   │
│   ├── [CPU 버퍼 없는 경우]
│   │   read_feature_kernel<float><<<g_size, blkSize>>>(
│   │       a->d_array_ptr,    # array_d_t<float>*
│   │       out,               # 출력 텐서 (GPU)
│   │       index,             # 노드 인덱스 배열 (GPU)
│   │       dim,               # feature 차원 (1024)
│   │       num_index,         # 노드 수
│   │       cache_dim,         # 캐시 차원 (1024)
│   │       key_off            # 키 오프셋 (0)
│   │   )
│   │
│   └── [CPU 버퍼 있는 경우]
│       read_feature_kernel_with_cpu_backing_memory<float>
│       <<<g_size, blkSize>>>(
│           a->d_array_ptr,    # array_d_t<float>*
│           out,               # 출력 텐서
│           index,             # 노드 인덱스
│           dim,               # feature 차원
│           num_index,         # 노드 수
│           cache_dim,         # 캐시 차원
│           key_off,           # 키 오프셋
│           h_range->d_range_ptr,  # range 메타데이터 (GPU)
│           CPU_buffer,        # GIDS_CPU_buffer 구조체
│           seq_flag,          # 순차 버퍼 플래그
│           d_cpu_access       # CPU 접근 카운터 (GPU)
│       )
│
├── cudaDeviceSynchronize()
│
├── 타이머 종료
│   elapsed = (et - st) in microseconds
│   kernel_time += elapsed
│   total_access += num_index
│
└── 반환 → Python으로 돌아감
    return_torch_buffer에 결과가 채워져 있음
```

## 3. CUDA 커널 상세: read_feature_kernel

```
gids_kernel.cu :: read_feature_kernel<float>  (Lines 3-27)

__global__ __launch_bounds__(64, 32)
void read_feature_kernel(
    array_d_t<float> *dr,        // BaM device array
    float *out_tensor_ptr,        // 출력 버퍼
    int64_t *index_ptr,           // 노드 인덱스
    int dim,                      // feature 차원 (1024)
    int64_t num_idx,              // 총 노드 수
    int cache_dim,                // 캐시 차원
    uint64_t key_off              // 노드 타입 오프셋
)
{
    // Warp 단위 작업 분배
    int warp_id = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
    int lane_id = threadIdx.x % 32;

    if (warp_id < num_idx) {
        // 이 warp가 담당할 노드의 인덱스
        int64_t row_index = index_ptr[warp_id] + key_off;

        // BaM array에서 포인터 획득
        bam_ptr<float> ptr = dr->get_bam_ptr();

        // 32개 lane이 분담하여 1024차원 읽기
        // lane 0: dim 0, 32, 64, ...
        // lane 1: dim 1, 33, 65, ...
        // ...
        for (int j = lane_id; j < dim; j += 32) {
            float val = ptr.read(row_index * cache_dim + j);
            //          ~~~~~~~~
            //          이 한 줄이 BaM의 전체 I/O 경로를 실행:
            //          TLB → Page Cache → (Miss시) NVMe SQ/CQ → SSD

            out_tensor_ptr[warp_id * dim + j] = val;
        }
    }
}
```

## 4. ptr.read() 내부 경로 (BaM page_cache)

```
bam_ptr<float>::read(uint64_t addr)
│
│  addr = row_index * cache_dim + j
│  예: 노드 42, dim offset 5 → addr = 42 * 1024 + 5 = 43013
│
├─[1] 페이지 번호 계산
│  page_id = addr * sizeof(float) / page_size
│  page_offset = (addr * sizeof(float)) % page_size
│
├─[2] TLB (Thread-Local Buffer) 조회
│  tlb_entry = tlb.entries[page_id % TLB_SIZE]
│  if (tlb_entry.global_id == page_id && tlb_entry.state == VALID):
│    → TLB Hit! 캐시된 페이지 포인터에서 즉시 읽기
│    return *(tlb_entry.page->data + page_offset)
│
├─[3] Page Cache Hash Table 조회
│  hash_idx = hash(page_id) % num_pages
│  data_page = page_cache.lookup(page_id)
│
│  ┌── Cache Hit ───────────────────────────────┐
│  │ data_page.state에 VALID 비트 설정됨        │
│  │ → 참조 카운트 증가 (atomic)                │
│  │ → TLB 엔트리 업데이트                      │
│  │ → data 포인터에서 값 읽기                  │
│  │ → 참조 카운트 감소                         │
│  │ return value                               │
│  └────────────────────────────────────────────┘
│
│  ┌── Cache Miss ──────────────────────────────┐
│  │                                            │
│  ├─[4] 빈 페이지 슬롯 확보                   │
│  │  page_cache에서 INVALID 슬롯 찾기          │
│  │  없으면 LRU 교체 (참조카운트 0인 페이지)   │
│  │  page.state = BUSY (다른 스레드 접근 차단)  │
│  │                                            │
│  ├─[5] NVMe Read 커맨드 구성                  │
│  │  range_t로 논리 주소 → 물리 주소 변환       │
│  │                                            │
│  │  STRIPE 모드:                               │
│  │    ssd_idx = page_id % num_ssd              │
│  │    lba = (page_id / num_ssd) * sectors      │
│  │                                            │
│  │  NVMe 커맨드:                               │
│  │  ┌──────────────────────────────────────┐  │
│  │  │ Opcode:  NVM_IO_READ                 │  │
│  │  │ NSID:    1                           │  │
│  │  │ PRP1:    cache_page DMA addr (GPU)   │  │
│  │  │ SLBA:    계산된 LBA                  │  │
│  │  │ NLB:     page_size / block_size      │  │
│  │  └──────────────────────────────────────┘  │
│  │                                            │
│  ├─[6] BaM Parallel Queue로 I/O 실행          │
│  │  ┌────────────────────────────────────┐    │
│  │  │ sq_enqueue(&sq, &cmd)              │    │
│  │  │ │                                  │    │
│  │  │ ├── ticket = in_ticket.fetch_add(1)│    │
│  │  │ ├── pos = ticket & qs_minus_1      │    │
│  │  │ ├── wait tickets[pos] == round_id  │    │
│  │  │ ├── copy cmd → sq.vaddr[pos]       │    │
│  │  │ ├── tail_mark[pos] = LOCKED        │    │
│  │  │ ├── move_tail()                    │    │
│  │  │ ├── *(sq.db) = new_tail ──── ★    │    │
│  │  │ │    GPU → PCIe → SSD Doorbell     │    │
│  │  │ └── wait head_mark[pos] unlocked   │    │
│  │  └────────────────────────────────────┘    │
│  │                                            │
│  │  SSD가 PCIe P2P DMA로:                     │
│  │  1. GPU의 SQ에서 커맨드 읽기               │
│  │  2. Flash에서 데이터 읽기                   │
│  │  3. GPU의 캐시 페이지에 데이터 쓰기 (P2P)  │
│  │  4. GPU의 CQ에 완료 엔트리 쓰기            │
│  │                                            │
│  │  ┌────────────────────────────────────┐    │
│  │  │ cq_poll(&cq, cid)                 │    │
│  │  │ → CQ 엔트리를 반복 확인            │    │
│  │  │ → phase tag로 새 완료 구분          │    │
│  │  │ → 내 CID 발견 시 반환              │    │
│  │  │                                    │    │
│  │  │ cq_dequeue(&cq)                    │    │
│  │  │ → CQ head 전진                     │    │
│  │  │ → *(cq.db) = new_head ──── ★      │    │
│  │  │   GPU → PCIe → SSD CQ Doorbell    │    │
│  │  └────────────────────────────────────┘    │
│  │                                            │
│  ├─[7] 페이지 상태 업데이트                    │
│  │  page.state = VALID                         │
│  │  page.offset = page_id                      │
│  │  TLB 엔트리 업데이트                        │
│  │                                            │
│  └─[8] 캐시 페이지에서 값 읽어서 반환          │
│     return *(page.data + page_offset)          │
│                                                │
│  └────────────────────────────────────────────┘
│
└── 값이 out_tensor_ptr[warp_id * dim + j]에 저장됨
```

## 5. CPU Backing Buffer가 있는 경우의 분기

```
gids_kernel.cu :: read_feature_kernel_with_cpu_backing_memory (Lines 29-86)

각 warp:
│
├── row_index = index_ptr[warp_id] + key_off
│
├── CPU 버퍼 체크
│   cpu_idx = d_range->check_cpu_buffer(row_index)
│   │
│   │  range_d_t::check_cpu_buffer():
│   │  → range 메타데이터에서 row_index가 CPU 버퍼에
│   │    매핑되어 있는지 확인
│   │  → 매핑됨: cpu_idx (CPU 버퍼 내 인덱스) 반환
│   │  → 매핑 안됨: -1 반환
│   │
│   ├── [cpu_idx >= 0: CPU 버퍼에 있음]
│   │   │
│   │   │  CPU 피닝 메모리에서 직접 읽기
│   │   │  (GPU → PCIe → Host DRAM, ~1-5μs)
│   │   │
│   │   │  if (cpu_seq):
│   │   │    base = cpu_idx * cpu_buffer_dim
│   │   │  else:
│   │   │    base = cpu_idx * cpu_buffer_dim
│   │   │
│   │   for (j = lane_id; j < dim; j += 32):
│   │     out[warp_id * dim + j] =
│   │       CPU_buffer.device_cpu_buffer[base + j]
│   │       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
│   │       cudaHostGetDevicePointer로 획득한 포인터
│   │       GPU가 PCIe를 통해 호스트 메모리 직접 읽기
│   │
│   │   // 통계 카운팅
│   │   if (lane_id == 0):
│   │     atomicAdd(d_cpu_access, 1)
│   │
│   │   return  ← SSD 접근 없이 완료
│   │
│   └── [cpu_idx < 0: CPU 버퍼에 없음]
│       │
│       └── SSD 경로 (위의 ptr.read() 경로와 동일)
│           bam_ptr<float> ptr = dr->get_bam_ptr()
│           for (j = lane_id; j < dim; j += 32):
│             val = ptr.read(row_index * cache_dim + j)
│             out[warp_id * dim + j] = val
```

## 6. Heterogeneous Read (다중 스트림)

```
gids_nvme.cu :: read_feature_hetero()  (Lines 194-254)

num_iter = 노드 타입 수 (예: paper, author, fos)

for (int i = 0; i < num_iter; i++):
│
├── cudaStreamCreate(&stream[i])
│
├── 해당 노드 타입의 파라미터:
│   out_ptr = i_ptr_list[i]           (GPU 출력 버퍼)
│   idx_ptr = i_index_ptr_list[i]     (GPU 인덱스)
│   n_idx = num_index[i]              (노드 수)
│   key = key_off[i]                  (SSD 내 오프셋)
│
└── read_feature_kernel<<<grid, 128, 0, stream[i]>>>(
        a->d_array_ptr, out_ptr, idx_ptr,
        dim, n_idx, cache_dim, key)
    │
    │ 각 스트림이 독립적으로 실행
    │ GPU가 여러 노드 타입을 동시에 처리
    │
    └── 스트림 i: paper 노드 feature 읽기 (key_off=0)
        스트림 j: author 노드 feature 읽기 (key_off=269M)
        스트림 k: fos 노드 feature 읽기 (key_off=546M)
        → 동시 실행으로 SSD 대역폭 극대화

cudaDeviceSynchronize()  ← 모든 스트림 완료 대기
```

## 7. Merged Read (Storage Accumulator와 함께)

```
gids_nvme.cu :: read_feature_merged()  (Lines 259-318)

누적된 여러 배치를 한 번에 읽기:

num_iter = 누적된 배치 수

for (int i = 0; i < num_iter; i++):
│
├── cudaStreamCreate(&stream[i])
│
└── 배치 i의 read_feature_kernel<<<..., stream[i]>>>
    // 각 배치가 별도 스트림에서 실행

효과:
  배치 1개: 500 I/O → SSD 대역폭 일부만 사용
  배치 3개 누적: 1500 I/O → SSD 대역폭 포화

┌────────┐ ┌────────┐ ┌────────┐
│Batch 0 │ │Batch 1 │ │Batch 2 │
│Stream 0│ │Stream 1│ │Stream 2│
└───┬────┘ └───┬────┘ └───┬────┘
    │          │          │
    ├──────────┴──────────┤
    │  동시 실행          │
    │  NVMe SQ에 더 많은  │
    │  커맨드가 쌓임      │
    │  → SSD 내부 병렬성  │
    │     최대 활용       │
    └─────────────────────┘
```

## 8. 성능 측정 포인트

```
┌──────────────────────────────────────────────────────────┐
│ 측정 지표               │ 위치                          │
├─────────────────────────┼───────────────────────────────┤
│ kernel_time             │ gids_nvme.cu:188              │
│ (CUDA 커널 실행 시간)    │ gettimeofday 기반             │
├─────────────────────────┼───────────────────────────────┤
│ total_access            │ gids_nvme.cu:189              │
│ (총 노드 접근 수)        │ 누적 카운터                   │
├─────────────────────────┼───────────────────────────────┤
│ cpu_access_count        │ d_cpu_access (GPU atomic)      │
│ (CPU 버퍼 히트 수)       │ cudaMemcpy로 읽기             │
├─────────────────────────┼───────────────────────────────┤
│ GIDS_time               │ GIDS.py:498                   │
│ (Python 레벨 전체 시간)  │ time.time() 기반              │
├─────────────────────────┼───────────────────────────────┤
│ WB_time                 │ GIDS.py:312                   │
│ (Window Buffering 시간)  │ time.time() 기반              │
├─────────────────────────┼───────────────────────────────┤
│ page_cache stats        │ h_pc->print_reset_stats()     │
│ (히트율, 미스율 등)      │ BaM 내장 통계                 │
├─────────────────────────┼───────────────────────────────┤
│ array stats             │ a->print_reset_stats()        │
│ (배열 접근 통계)         │ BaM 내장 통계                 │
└──────────────────────────────────────────────────────────┘
```
