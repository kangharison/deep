# GIDS 캐시 메커니즘 & CPU Backing Buffer 분석

## 1. Page Cache 구조 (BaM 기반)

```
page_cache_t (page_cache_backup.h에서 정의)

┌─────────────────────────────────── GPU VRAM ───────────────────────┐
│                                                                    │
│  ┌── Page Cache 메타데이터 ──────────────────────────────────────┐ │
│  │                                                               │ │
│  │  data_page_t pages[n_pages]     (정렬: 32바이트)              │ │
│  │  ┌──────────────────────────────────────────┐                 │ │
│  │  │ pages[0]:                                │                 │ │
│  │  │   state:  atomic<uint32_t>               │                 │ │
│  │  │           ┌─ bit 31: VALID (0x80000000)  │                 │ │
│  │  │           ├─ bit 30: BUSY  (0x40000000)  │                 │ │
│  │  │           ├─ bit 29: DIRTY (0x20000000)  │                 │ │
│  │  │           └─ bits 28-0: 참조 카운트       │                 │ │
│  │  │   offset: uint32_t (이 페이지의 논리 ID)  │                 │ │
│  │  │   prefetch_count: atomic<uint8_t>         │                 │ │
│  │  │   prefetch_counter: atomic<uint8_t>       │                 │ │
│  │  ├──────────────────────────────────────────┤                 │ │
│  │  │ pages[1]: ...                            │                 │ │
│  │  │ ...                                      │                 │ │
│  │  │ pages[2097151]: ...                      │                 │ │
│  │  └──────────────────────────────────────────┘                 │ │
│  │                                                               │ │
│  │  상태 전이도:                                                 │ │
│  │  INVALID ──acquire──► BUSY ──load──► VALID                    │ │
│  │     ▲                                  │                      │ │
│  │     └──────────evict (ref_cnt=0)───────┘                      │ │
│  │                                                               │ │
│  └───────────────────────────────────────────────────────────────┘ │
│                                                                    │
│  ┌── 실제 데이터 버퍼 ──────────────────────────────────────────┐ │
│  │  char data[n_pages × page_size]                               │ │
│  │                                                               │ │
│  │  ┌─────────┐┌─────────┐┌─────────┐     ┌─────────┐          │ │
│  │  │ Page 0  ││ Page 1  ││ Page 2  │ ... │ Page N  │          │ │
│  │  │ 4096B   ││ 4096B   ││ 4096B   │     │ 4096B   │          │ │
│  │  └─────────┘└─────────┘└─────────┘     └─────────┘          │ │
│  │                                                               │ │
│  │  총 크기: 2,097,152 × 4096 = 8 GB (cache_size 설정에 따라)   │ │
│  └───────────────────────────────────────────────────────────────┘ │
│                                                                    │
│  ┌── 해시 테이블 (페이지 룩업) ─────────────────────────────────┐ │
│  │  page_id → 캐시 슬롯 인덱스 매핑                             │ │
│  │  해시 충돌: 체이닝 또는 오픈 어드레싱                         │ │
│  └───────────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

## 2. TLB (Thread-Level Buffer)

```
page_cache_backup.h :: tlb<T, n, scope, loc>

GPU 스레드/워프별 로컬 캐시 (페이지 캐시 앞단의 빠른 조회)

┌── 각 워프의 TLB ─────────────────────────┐
│                                          │
│  tlb_entry entries[n]                    │
│  ┌───────────────────────────────────┐   │
│  │ entry[0]:                         │   │
│  │   global_id: uint64_t (페이지 ID) │   │
│  │   state: atomic<uint32_t>         │   │
│  │   page: data_page_t* (직접 포인터)│   │
│  ├───────────────────────────────────┤   │
│  │ entry[1]: ...                     │   │
│  │ ...                               │   │
│  └───────────────────────────────────┘   │
│                                          │
│  조회 순서:                              │
│  1. TLB에서 page_id 매칭 → O(1) 즉시    │
│  2. Page Cache 해시에서 조회 → O(1) amort│
│  3. Cache Miss → SSD I/O → O(10μs)      │
│                                          │
│  acquire(page_id):                       │
│    if entries[page_id % n].global_id     │
│       == page_id:                        │
│      state.fetch_add(1)  // 참조++       │
│      return page pointer  // TLB Hit!    │
│    else:                                 │
│      // TLB Miss → Page Cache로 이동    │
│      release old entry                   │
│      load new entry from page cache      │
│                                          │
└──────────────────────────────────────────┘
```

## 3. 캐시 페이지 생명주기

```
시나리오: 노드 42의 feature가 처음 접근됨

[1] bam_ptr.read(42 * 1024 + 0)
    │
    ├── page_id = (42 * 1024 * 4) / 4096 = 42
    │
    ├── TLB 조회 → Miss
    │
    ├── Page Cache 해시 조회 → Miss
    │
    ├── 빈 슬롯 찾기
    │   │
    │   ├── INVALID 상태 슬롯 있으면 → 사용
    │   │
    │   └── 없으면 → Eviction
    │       for slot in pages:
    │         expected = VALID | 0 (ref_cnt = 0)
    │         if slot.state.CAS(expected, BUSY):
    │           → 이 슬롯 교체 대상
    │           (참조 카운트가 0인 VALID 페이지만 교체 가능)
    │
    ├── 슬롯 확보: pages[slot].state = BUSY
    │
    ├── NVMe Read 실행
    │   STRIPE: SSD idx = 42 % 4 = 2 → SSD2에서 읽기
    │   LBA = (42 / 4) * sectors_per_page
    │
    │   sq_enqueue → Doorbell → SSD DMA → cq_poll
    │   → 데이터가 data[slot × 4096] 위치에 로드됨
    │
    ├── pages[slot].state = VALID  (BUSY → VALID)
    │   pages[slot].offset = 42
    │   해시 테이블 업데이트: hash[42] → slot
    │
    └── TLB 엔트리 업데이트
        tlb.entries[42 % n] = { global_id=42, page=&pages[slot] }

[2] 같은 배치 내 다른 워프가 노드 42 접근 시
    → TLB Hit 또는 Page Cache Hit → SSD 접근 없이 즉시 반환

[3] 다른 배치에서 노드 42 재접근 시
    → Page Cache에 아직 있으면 → Cache Hit
    → Evict 되었으면 → Cache Miss → SSD 재읽기

[4] Eviction 조건
    → 참조 카운트가 0이어야 함 (아무도 읽고 있지 않음)
    → DIRTY 플래그가 있으면 Write-back 후 교체
      (GIDS는 read 위주이므로 DIRTY는 드묾)
```

## 4. CPU Backing Buffer 구조

```
┌─────────────────── CPU Pinned Memory ────────────────────────────┐
│                                                                  │
│  GIDS_CPU_buffer<float>:                                         │
│  │                                                               │
│  ├── cpu_buffer (TYPE*):                                         │
│  │   cudaHostAlloc(&cpu_buffer, size, cudaHostAllocMapped)       │
│  │   호스트 메모리에 할당, 페이지 고정 (swap 불가)               │
│  │                                                               │
│  ├── device_cpu_buffer (TYPE*):                                  │
│  │   cudaHostGetDevicePointer(&device_cpu_buffer, cpu_buffer, 0) │
│  │   GPU에서 직접 접근 가능한 포인터                              │
│  │                                                               │
│  ├── cpu_buffer_dim (uint64_t):                                  │
│  │   feature 차원 (1024)                                         │
│  │                                                               │
│  └── cpu_buffer_len (uint64_t):                                  │
│      저장된 노드 수                                               │
│                                                                  │
│  메모리 레이아웃:                                                 │
│  ┌────────────────────────────────────────────────────┐          │
│  │ Node A: [f0, f1, f2, ..., f1023]  (4096 bytes)    │          │
│  │ Node B: [f0, f1, f2, ..., f1023]  (4096 bytes)    │          │
│  │ Node C: [f0, f1, f2, ..., f1023]  (4096 bytes)    │          │
│  │ ...                                                │          │
│  │ Node N: [f0, f1, f2, ..., f1023]  (4096 bytes)    │          │
│  └────────────────────────────────────────────────────┘          │
│                                                                  │
│  크기 계산:                                                       │
│  cpu_buffer_percent = 0.2 (20%)                                   │
│  total_nodes = 269,346,000 (IGB-Full)                             │
│  num_pinned = 269,346,000 × 0.2 = 53,869,200                     │
│  buffer_size = 53,869,200 × 1024 × 4 = ~206 GB                  │
│  (실제로는 호스트 메모리 크기에 맞춰 조절)                        │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## 5. CPU Buffer 노드 선별 (PageRank)

```
page_rank_node_list_gen.py :: compute_pagerank()

입력: DGL 그래프 g
출력: PageRank 상위 노드 인덱스 텐서

알고리즘:
  N = g.num_nodes()
  pv = torch.ones(N) / N          ← 초기값: 균등 분포

  for k in range(K=20):            ← 20회 반복
    pv_new = zeros(N)
    for each edge (src → dst):
      pv_new[dst] += pv[src] / out_degree[src]
    pv = (1 - DAMP) / N + DAMP * pv_new

  top_indices = pv.topk(N * 0.6)   ← 상위 60% 선택
  torch.save(top_indices, output_file)

왜 PageRank?
  ┌──────────────────────────────────────────────────────┐
  │ GNN에서 허브 노드(높은 degree)는 많은 미니배치에서    │
  │ 반복적으로 샘플링된다.                               │
  │                                                      │
  │ PageRank 점수 ∝ 노드의 "중요도" ≈ 접근 빈도          │
  │                                                      │
  │ 상위 20%를 CPU 버퍼에 두면:                           │
  │ - GPU Cache 미스 시에도 SSD 접근 불필요               │
  │ - PCIe를 통한 호스트 메모리 접근 (~1-5μs)            │
  │ - SSD 접근 (~10-20μs)보다 5-10배 빠름                │
  └──────────────────────────────────────────────────────┘
```

## 6. CPU Buffer 설정 플로우

```
[1] 버퍼 할당
    GIDS_Loader.cpu_backing_buffer(dim=1024, length=N)
    │
    └── gids_nvme.cu :: cpu_backing_buffer(dim, len):
        CPU_buffer.cpu_buffer_dim = dim
        CPU_buffer.cpu_buffer_len = len
        cudaHostAlloc(&CPU_buffer.cpu_buffer,
                      len * dim * sizeof(float),
                      cudaHostAllocMapped)
        cudaHostGetDevicePointer(&CPU_buffer.device_cpu_buffer,
                                 CPU_buffer.cpu_buffer, 0)
        cpu_buffer_flag = true

[2] 노드 매핑 설정
    pr_tensor = torch.load("pagerank_nodes.pt")  ← 노드 ID 리스트
    GIDS_Loader.set_cpu_buffer(pr_tensor, N)
    │
    └── gids_nvme.cu :: set_cpu_buffer(idx_buffer, num):
        int64_t* idx_ptr = (int64_t*)idx_buffer

        [2a] range 메타데이터에 매핑 등록
             set_cpu_buffer_kernel<<<(num+127)/128, 128>>>(
                 h_range->d_range_ptr, idx_ptr, num)
             │
             │ 각 스레드:
             │   node_id = idx_ptr[threadIdx]
             │   d_range->set_cpu_buffer(node_id, threadIdx)
             │   → "node_id는 CPU 버퍼의 인덱스 threadIdx에 있다"
             │   → 이후 check_cpu_buffer(node_id) → threadIdx 반환

        [2b] SSD에서 데이터 복사
             set_cpu_buffer_data_kernel<<<num, 128>>>(
                 a->d_array_ptr,
                 CPU_buffer.device_cpu_buffer,
                 idx_ptr, dim, num)
             │
             │ 각 블록 (노드 1개):
             │   node_id = idx_ptr[blockIdx.x]
             │   bam_ptr ptr = dr->get_bam_ptr()
             │   for (j = threadIdx.x; j < dim; j += 128):
             │     val = ptr.read(node_id * dim + j)  ← SSD에서 읽기
             │     device_cpu_buffer[blockIdx.x * dim + j] = val
             │                                          ↑
             │                               CPU 피닝 메모리에 쓰기
             │                               (GPU가 PCIe로 호스트에 쓰기)

        seq_flag = false

[3] 런타임 동작
    read_feature_kernel_with_cpu_backing_memory 에서:

    cpu_idx = d_range->check_cpu_buffer(row_index)
    if (cpu_idx >= 0):
      // CPU 버퍼에서 읽기 (~1-5μs)
      for j in lane_id..dim step 32:
        out[warp_id*dim+j] = device_cpu_buffer[cpu_idx*dim+j]
    else:
      // SSD에서 읽기 (~10-20μs)
      ptr.read(...)
```

## 7. 3단계 메모리 히트 확률

```
접근 경로와 지연:

Request ──► TLB ──Hit──► ~10ns (GPU 레지스터/L1)
             │
             Miss
             │
             ▼
         Page Cache ──Hit──► ~100ns (GPU VRAM)
             │
             Miss
             │
             ▼
         CPU Buffer ──Hit──► ~1-5μs (PCIe → Host DRAM)
             │
             Miss (또는 CPU 버퍼 없음)
             │
             ▼
         NVMe SSD ──────────► ~10-20μs (PCIe P2P DMA)


실험 결과 (논문 기반):
┌───────────────────────────────────────────────────────┐
│ 데이터셋      │ 캐시 8GB │ CPU 20% │ 전체 히트율     │
├───────────────┼──────────┼─────────┼─────────────────┤
│ papers100M    │ ~15%     │ ~45%    │ ~60% (SSD 40%)  │
│ MAG240M       │ ~5%      │ ~30%    │ ~35% (SSD 65%)  │
│ IGB-Full      │ ~3%      │ ~25%    │ ~28% (SSD 72%)  │
└───────────────────────────────────────────────────────┘

SSD 접근이 적을수록 → 전체 latency 감소 → 학습 속도 향상
```
