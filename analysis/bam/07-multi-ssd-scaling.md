# 07. Multi-SSD 지원 분석

## 1. 개요

BaM은 여러 NVMe SSD를 동시에 사용하여 대역폭을 확장하는 Multi-SSD 아키텍처를 지원한다. GPU 스레드들이 여러 컨트롤러의 큐를 병렬로 사용하며, 데이터 분산 방식으로 STRIPE(스트라이핑)과 REPLICATE(복제)를 제공한다.

```
┌──────────────────────────────────────────────────────────────┐
│                         GPU                                  │
│                                                              │
│   ┌──────────────────────────────────────────────────────┐  │
│   │              page_cache_d_t (소프트웨어 캐시)           │  │
│   │                                                      │  │
│   │  d_ctrls[0] ──▶ Controller 0 (d_qps[0..N])          │  │
│   │  d_ctrls[1] ──▶ Controller 1 (d_qps[0..N])          │  │
│   │  d_ctrls[2] ──▶ Controller 2 (d_qps[0..N])          │  │
│   │  ...                                                 │  │
│   │  n_ctrls = SSD 수                                    │  │
│   │                                                      │  │
│   │  ranges_dists[range_id] = STRIPE 또는 REPLICATE       │  │
│   └──────────────────────────────────────────────────────┘  │
│       │              │              │                        │
│       ▼              ▼              ▼                        │
│   QueuePair      QueuePair      QueuePair                   │
│   SQ/CQ (GPU)    SQ/CQ (GPU)    SQ/CQ (GPU)                │
│       │              │              │                        │
└───────┼──────────────┼──────────────┼────────────────────────┘
        │              │              │      PCIe P2P
        ▼              ▼              ▼
   ┌─────────┐   ┌─────────┐   ┌─────────┐
   │ NVMe 0  │   │ NVMe 1  │   │ NVMe 2  │
   │ SSD     │   │ SSD     │   │ SSD     │
   └─────────┘   └─────────┘   └─────────┘
```


## 2. 데이터 분산 방식

### 2.1 열거형 정의

```c
// include/page_cache.h:42
enum data_dist_t { REPLICATE = 0, STRIPE = 1 };
```

### 2.2 STRIPE 모드

데이터 페이지를 SSD들에 라운드 로빈으로 분산한다. 대역폭 확장에 효과적이다.

```
데이터 페이지:  [P0] [P1] [P2] [P3] [P4] [P5] [P6] [P7] ...

3개 SSD STRIPE:
  SSD 0:  P0  P3  P6  ...     (page_offset % n_ctrls == 0)
  SSD 1:  P1  P4  P7  ...     (page_offset % n_ctrls == 1)
  SSD 2:  P2  P5  ...         (page_offset % n_ctrls == 2)
```

#### get_backing_ctrl_() (include/page_cache.h:571)
페이지 오프셋으로부터 담당 SSD 컨트롤러를 결정한다.
```c
__device__
uint64_t get_backing_ctrl_(const size_t page_offset, const uint64_t n_ctrls,
                           const data_dist_t dist) {
    if (dist == STRIPE)
        ctrl = page_offset % n_ctrls;     // 라운드 로빈 분산
    else if (dist == REPLICATE)
        ctrl = ALL_CTRLS;                 // 모든 SSD에 존재
    return ctrl;
}
```

#### get_backing_page_() (include/page_cache.h:558)
페이지 오프셋으로부터 해당 SSD 내의 LBA 주소를 계산한다.
```c
__device__
uint64_t get_backing_page_(const uint64_t page_start, const size_t page_offset,
                           const uint64_t n_ctrls, const data_dist_t dist) {
    uint64_t page = page_start;
    if (dist == STRIPE)
        page += page_offset / n_ctrls;   // SSD 내 페이지 인덱스
    else if (dist == REPLICATE)
        page += page_offset;              // 전체 오프셋 그대로
    return page;
}
```

### 2.3 REPLICATE 모드

모든 SSD에 동일한 데이터의 복사본을 유지한다. 읽기 대역폭은 SSD 수에 비례하여 증가하고, 쓰기 시에는 모든 SSD에 동일 데이터를 기록한다.

```
데이터 페이지:  [P0] [P1] [P2] [P3] ...

3개 SSD REPLICATE:
  SSD 0:  P0  P1  P2  P3  ...  (전체 복사본)
  SSD 1:  P0  P1  P2  P3  ...  (전체 복사본)
  SSD 2:  P0  P1  P2  P3  ...  (전체 복사본)

읽기: 아무 SSD에서나 읽으면 됨 (ctrl = ALL_CTRLS → 랜덤 선택 가능)
쓰기: 모든 SSD에 동일하게 기록해야 함
```


## 3. Multi-SSD 초기화

### 3.1 여러 Controller 생성

벤치마크 코드에서 여러 NVMe 컨트롤러를 개별적으로 초기화한다.

```c
// benchmarks/array/main.cu:93
std::vector<Controller*> ctrls(settings.n_ctrls);
for (size_t i = 0; i < settings.n_ctrls; i++)
    ctrls[i] = new Controller(sam_ctrls_paths[i], settings.nvmNamespace,
                               settings.cudaDevice, settings.queueDepth,
                               settings.numQueues);
```

각 Controller 객체는 독립적으로 초기화된다:
- 각자의 `/dev/libnvmN` 캐릭터 디바이스를 열고
- 각자의 BAR0을 mmap하고
- 각자의 Admin Queue를 생성하여 컨트롤러를 리셋하고
- 각자의 I/O Queue Pair를 생성한다

### 3.2 page_cache_t에 여러 Controller 등록

```c
// benchmarks/array/main.cu:123
page_cache_t h_pc(page_size, n_pages, settings.cudaDevice,
                  ctrls[0][0],         // 기준 컨트롤러 (PRP 크기 등 결정)
                  (uint64_t) 64,       // 최대 range 수
                  ctrls);              // 전체 컨트롤러 벡터
```

### 3.3 page_cache_t 생성자의 Multi-Controller 설정

```c
// include/page_cache.h:697
page_cache_t::page_cache_t(..., const std::vector<Controller*>& ctrls) {
    pdt.n_ctrls = ctrls.size();

    // GPU 메모리에 Controller 포인터 배열 할당
    d_ctrls_buff = createBuffer(pdt.n_ctrls * sizeof(Controller*), cudaDevice);
    pdt.d_ctrls = (Controller**) d_ctrls_buff.get();

    // 각 컨트롤러의 GPU 디바이스 포인터를 배열에 복사
    for (size_t k = 0; k < pdt.n_ctrls; k++)
        cudaMemcpy(pdt.d_ctrls + k, &(ctrls[k]->d_ctrl_ptr),
                   sizeof(Controller*), cudaMemcpyHostToDevice);
}
```

GPU에서 접근하는 구조:
```
GPU 메모리:
page_cache_d_t:
  d_ctrls ──▶ [Controller* 0] [Controller* 1] [Controller* 2] ...
                    │                │                │
                    ▼                ▼                ▼
              Controller 0     Controller 1     Controller 2
              ├── d_qps[0]    ├── d_qps[0]    ├── d_qps[0]
              ├── d_qps[1]    ├── d_qps[1]    ├── d_qps[1]
              └── ...         └── ...         └── ...
```


## 4. GPU 스레드 → Queue 할당

### 4.1 SM 기반 큐 선택

GPU 커널에서 I/O를 발행할 때, 현재 GPU SM(Streaming Multiprocessor)의 ID를 기반으로 Queue Pair를 선택한다.

```c
// include/page_cache.h:598 (__flush 커널에서)
uint32_t smid = get_smid();
Controller* c = pc->d_ctrls[ctrl_id];
uint32_t queue = smid % (c->n_qps);
write_data(pc, (c->d_qps) + queue, ...);
```

`get_smid()`는 PTX 인라인 어셈블리로 현재 SM의 하드웨어 ID를 읽는다:
```c
// include/nvm_util.h
__device__ uint32_t get_smid() {
    uint32_t ret;
    asm("mov.u32 %0, %smid;" : "=r"(ret));
    return ret;
}
```

### 4.2 큐 할당 전략

```
SM 0  → queue = 0 % n_qps → QueuePair 0
SM 1  → queue = 1 % n_qps → QueuePair 1
SM 2  → queue = 2 % n_qps → QueuePair 2
...
SM K  → queue = K % n_qps → QueuePair (K % n_qps)
```

같은 SM에서 실행되는 여러 warp들은 동일한 QueuePair를 공유한다. 이 경우 lock-free parallel queue 메커니즘(04-parallel-queue.md 참조)이 경합을 관리한다.


## 5. I/O 발행 시 컨트롤러 선택

### 5.1 읽기 경로 (STRIPE)

STRIPE 모드에서 읽기 요청은 페이지 오프셋에 따라 단일 컨트롤러로 라우팅된다.

```c
// page_cache의 acquire_page 내부 흐름 (간략화):
uint64_t ctrl_id = get_backing_ctrl_(page_offset, n_ctrls, dist);
uint64_t lba_page = get_backing_page_(page_start, page_offset, n_ctrls, dist);
Controller* c = pc->d_ctrls[ctrl_id];
uint32_t queue = smid % (c->n_qps);
read_data(pc, (c->d_qps) + queue, lba_page * n_blocks_per_page, n_blocks_per_page, cache_slot);
```

### 5.2 읽기 경로 (REPLICATE)

REPLICATE 모드에서 `get_backing_ctrl_()`는 `ALL_CTRLS`(0xffffffffffffffff)를 반환한다. 읽기 시에는 임의의 컨트롤러를 선택하면 되므로, 실제 구현에서는 특정 로직에 따라 하나를 선택한다.

### 5.3 쓰기 경로 (__flush 커널)

```c
// include/page_cache.h:584 - __flush 커널
if (ctrl == ALL_CTRLS) {
    // REPLICATE: 모든 컨트롤러에 동일 데이터를 기록
    for (ctrl = 0; ctrl < pc->n_ctrls; ctrl++) {
        Controller* c = pc->d_ctrls[ctrl];
        uint32_t queue = smid % (c->n_qps);
        write_data(pc, (c->d_qps) + queue, index * n_blocks_per_page,
                   n_blocks_per_page, page);
    }
} else {
    // STRIPE: 해당 컨트롤러에만 기록
    Controller* c = pc->d_ctrls[ctrl];
    uint32_t queue = smid % (c->n_qps);
    write_data(pc, (c->d_qps) + queue, index * n_blocks_per_page,
               n_blocks_per_page, page);
}
```


## 6. range_t와 데이터 분산

### 6.1 range_t 생성

```c
// benchmarks/array/main.cu:132
range_t<uint64_t> h_range(
    (uint64_t)0,            // index_start
    (uint64_t)n_elems,      // count (요소 수)
    (uint64_t)0,            // page_start (SSD 시작 페이지)
    (uint64_t)(t_size/page_size), // page_count
    (uint64_t)0,            // page_start_offset
    (uint64_t)page_size,    // page_size
    &h_pc,                  // page_cache 참조
    settings.cudaDevice,
    /* dist = REPLICATE (기본값) */
);
```

### 6.2 range_d_t의 데이터 분산 관련 필드

```c
// include/page_cache.h:892
template <typename T>
struct range_d_t {
    uint64_t    page_start;     // SSD 시작 페이지 번호
    uint64_t    page_count;     // 총 페이지 수
    data_dist_t dist;           // 분산 방식 (STRIPE 또는 REPLICATE)
    page_cache_d_t cache;       // 페이지 캐시 참조 (n_ctrls, d_ctrls 포함)
    // ...
};
```

### 6.3 range_d_t의 백킹 스토리지 계산

```c
// range_d_t::get_backing_page()
__device__
uint64_t get_backing_page(const size_t i) const {
    return get_backing_page_(page_start, i, cache.n_ctrls, dist);
}

// range_d_t::get_backing_ctrl()
__device__
uint64_t get_backing_ctrl(const size_t i) const {
    return get_backing_ctrl_(i, cache.n_ctrls, dist);
}
```


## 7. 대역폭 스케일링

### 7.1 STRIPE 모드 대역폭

```
N개 SSD STRIPE:
  이론적 최대 읽기 대역폭 = N × 단일 SSD 대역폭
  이론적 최대 쓰기 대역폭 = N × 단일 SSD 대역폭

예: Samsung 970 EVO 3.5GB/s × 4 SSD = 14 GB/s
```

STRIPE에서 각 SSD는 전체 데이터의 1/N만 담당하므로, 충분한 GPU 스레드가 동시에 서로 다른 SSD에 요청을 발행하면 대역폭이 선형으로 증가한다.

### 7.2 REPLICATE 모드 대역폭

```
N개 SSD REPLICATE:
  이론적 최대 읽기 대역폭 = N × 단일 SSD 대역폭
  이론적 최대 쓰기 대역폭 = 단일 SSD 대역폭 (모든 SSD에 쓰기)

읽기가 주된 워크로드에 적합
```

### 7.3 병목 요인

- **PCIe 대역폭**: PCIe Gen3 x16은 약 16GB/s, Gen4 x16은 약 32GB/s이다. SSD 수가 늘어도 PCIe 루트 복합체의 총 대역폭을 초과할 수 없다
- **GPU 메모리 대역폭**: GPU 캐시 히트율이 낮으면 실제 SSD I/O량이 증가하여 SSD 대역폭이 병목이 된다
- **큐 깊이**: 각 SSD가 충분한 큐 깊이(outstanding commands)를 확보해야 IOPS가 극대화된다


## 8. 커널 모듈의 Multi-SSD 지원

### 8.1 GPU P2P 매핑의 모든 컨트롤러 등록

커널 모듈의 `map_gpu_memory()` (module/map.c:395)에서 GPU 메모리에 대한 P2P DMA 매핑을 생성할 때, 모듈에 등록된 **모든** NVMe 컨트롤러에 대해 매핑을 생성한다.

```c
// module/map.c:446 - 모든 컨트롤러에 대한 P2P 매핑 루프
while (element != NULL) {
    ctrl = container_of(element, struct ctrl, list);
    nvidia_p2p_dma_map_pages(ctrl->pdev, gd->pages, gd->mappings + (j++));
    element = list_next(element);
}
```

이 설계 덕분에 페이지 캐시의 GPU 메모리가 어떤 NVMe SSD에서든 직접 DMA로 접근 가능하다. 모든 SSD → GPU 직접 전송 경로가 초기화 시점에 한 번에 설정된다.

### 8.2 max_num_ctrls 파라미터

```c
// module/map.c:41
uint32_t max_num_ctrls = 64;

// module/map.c:413
gd->mappings = kmalloc(sizeof(nvidia_p2p_dma_mapping_t*) * max_num_ctrls, GFP_KERNEL);
```
GPU P2P 매핑 배열은 `max_num_ctrls`(기본 64)개까지 지원한다. 모듈 파라미터로 변경 가능하다.


## 9. 벤치마크 구조

### 9.1 array 벤치마크 (benchmarks/array/main.cu)

순차/랜덤 접근 패턴으로 Multi-SSD 대역폭을 측정한다.

```c
// 순차 접근 커널
__global__
void sequential_access_kernel(array_d_t<uint64_t>* dr, uint64_t n_reqs,
                              unsigned long long* req_count, uint64_t reqs_per_thread) {
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n_reqs) {
        for (size_t i = 0; i < reqs_per_thread; i++)
            req_count += (*dr)[(tid)];  // operator[] → 캐시 확인 → 미스 시 SSD I/O
    }
}

// 랜덤 접근 커널
__global__
void random_access_kernel(array_d_t<uint64_t>* dr, uint64_t n_reqs,
                          unsigned long long* req_count, uint64_t* assignment,
                          uint64_t reqs_per_thread) {
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n_reqs) {
        for (size_t i = 0; i < reqs_per_thread; i++)
            req_count += (*dr)[(assignment[tid])];  // 랜덤 인덱스
    }
}
```

### 9.2 벤치마크 초기화 흐름

```
1. Controller 생성 (n_ctrls개)
   for i in 0..n_ctrls:
     ctrls[i] = new Controller(path[i], ns, cudaDevice, queueDepth, numQueues)

2. page_cache_t 생성 (공유 캐시)
   h_pc = page_cache_t(page_size, n_pages, cudaDevice, ctrls[0], max_range, ctrls)

3. range_t 생성 (데이터 범위)
   h_range = range_t<uint64_t>(0, n_elems, 0, t_size/page_size, 0, page_size, &h_pc, cudaDevice)

4. array_t 생성 (GPU 접근 인터페이스)
   a = array_t<uint64_t>(n_elems, 0, vr, cudaDevice)

5. GPU 커널 실행
   sequential/random_access_kernel<<<g_size, b_size>>>(a.d_array_ptr, ...)

6. 성능 측정
   elapsed time, IOPS, effective bandwidth 출력
```

### 9.3 성능 통계

```c
// Controller::print_reset_stats() - SSD 접근 카운터 출력
cudaMemcpy(&access_counter, d_ctrl_ptr, sizeof(...), cudaMemcpyDeviceToHost);
printf("#SSDAccesses: %llu\n", access_counter);

// range_d_t 필드 - 접근 패턴 통계
simt::atomic<uint64_t> access_cnt;   // 총 접근 수
simt::atomic<uint64_t> miss_cnt;     // 캐시 미스 수
simt::atomic<uint64_t> hit_cnt;      // 캐시 히트 수
simt::atomic<uint64_t> read_io_cnt;  // 실제 SSD 읽기 I/O 수
```


## 10. 관련 파일 요약

| 파일 경로 | 역할 |
|-----------|------|
| `include/page_cache.h` | Multi-SSD 핵심: data_dist_t, get_backing_ctrl_, get_backing_page_, __flush, page_cache_t/d_t |
| `include/ctrl.h` | Controller 구조체, Multi-Controller 초기화, GPU 복사 |
| `include/queue.h` | QueuePair 구조체, I/O 큐 생성 |
| `include/buffer.h` | createDma (GPU/호스트), getDeviceMemory (64KB 정렬) |
| `module/map.c` | map_gpu_memory: 모든 컨트롤러에 P2P DMA 매핑 |
| `module/pci.c` | ctrl_list: 전역 컨트롤러 리스트, max_num_ctrls 파라미터 |
| `benchmarks/array/main.cu` | Multi-SSD 벤치마크 예제 |
