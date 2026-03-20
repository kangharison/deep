# BaM block/main.cu I/O Flow — PPT 장표 구성

**소스코드:** `sources/bam/benchmarks/block/main.cu`
**목적:** GPU-initiated NVMe I/O의 전체 흐름을 발표 자료로 구성

## 장표 1: 전체 프로그램 흐름 (Big Picture)

```
main() [CPU Host]
  │
  ├── Phase 1: 파라미터 파싱
  │   settings.parseArguments(argc, argv)
  │   (blkSize, numThreads, numPages, numQueues, random, ...)
  │
  ├── Phase 2: NVMe 컨트롤러 초기화
  │   Controller("/dev/libnvm0", ns, gpu, qd, nq)
  │   → BAR0 mmap + cudaHostRegister + QP 생성
  │
  ├── Phase 3: GPU 페이지 캐시 생성
  │   page_cache_t(page_size, n_pages, cudaDevice, ctrls)
  │   → GPU VRAM에 n_pages × page_size 할당
  │
  ├── Phase 4: 워크로드 준비
  │   랜덤 모드: assignment[tid] = rand() % n_blocks
  │   MIXED 모드: access_type_assignment[tid] = READ/WRITE
  │
  ├── Phase 5: GPU 커널 실행
  │   random_access_kernel<<<g_size, b_size>>>(...)
  │   또는 sequential_access_kernel<<<...>>>(...)
  │
  ├── Phase 6: cudaDeviceSynchronize() — 완료 대기
  │
  └── Phase 7: 결과 출력
      → Elapsed, IOPS, Bandwidth(GB/s), Cache Hit/Miss

핵심: Phase 1~4는 CPU, Phase 5만 GPU, Phase 6~7은 CPU
```

## 장표 2: Host 초기화 상세 (Phase 2~3)

```
Controller 생성자                 page_cache_t 생성자
┌──────────────────────┐         ┌──────────────────────┐
│ open("/dev/libnvm0") │         │ cudaMalloc(VRAM)     │
│ mmap(BAR0)           │         │  → n_pages × page_size│
│ cudaHostRegister     │         │  → base_addr (VRAM)  │
│  (IoMemory)          │         │                      │
│ Admin: Identify      │         │ PRP 주소 배열 생성   │
│ Admin: Create CQ ×N  │         │  → prp1[n_pages]     │
│ Admin: Create SQ ×N  │         │  → NVMe DMA 주소     │
│ cudaHostGetDevicePtr │         │                      │
│  (도어벨 → GPU ptr)  │         │ 캐시 상태 배열 초기화│
│ cudaMemcpy → GPU     │         │  → INVALID × n_pages │
└──────────────────────┘         └──────────────────────┘

결과: GPU VRAM 메모리 맵
┌─────────────────────────────────────────────────┐
│ d_qps[0..N]: SQ/CQ 메모리 + 도어벨 포인터       │
│ d_ctrl_ptr:  Controller 구조체 복사본            │
│ base_addr:   페이지 캐시 데이터 영역             │
│ prp1[]:      NVMe DMA 주소 배열                  │
│ tickets/marks/cid[]: lock-free 동기화 배열       │
└─────────────────────────────────────────────────┘
```

## 장표 3: CUDA 커널 실행 구성

```
__launch_bounds__(64, 32)
random_access_kernel<<<g_size, b_size>>>(...)

┌─────────────────────────────────────────────┐
│ b_size (blockDim.x) = 64 threads/block      │
│                        = 2 warps/block       │
│                                              │
│ g_size (gridDim.x) = ⌈numThreads / 64⌉     │
│                                              │
│ n_threads = g_size × b_size                  │
│                                              │
│ 예: numThreads=65536                         │
│     → g_size=1024 blocks                     │
│     → 1024 × 64 = 65,536 threads            │
│     → 2,048 warps                            │
│     → 각 thread가 1건 이상의 I/O 수행        │
└─────────────────────────────────────────────┘

커널 파라미터:
┌──────────────────┬──────────────────────────┐
│ ctrls            │ Controller 배열 (GPU ptr)│
│ d_pc             │ 페이지 캐시 (GPU ptr)    │
│ page_size        │ I/O 요청 크기 (4KB)      │
│ n_threads        │ 총 스레드 수             │
│ n_ctrls          │ SSD 수                   │
│ assignment       │ 랜덤 LBA 배열 (GPU ptr)  │
│ reqs_per_thread  │ 스레드당 반복 횟수       │
│ access_type      │ READ/WRITE/MIXED         │
└──────────────────┴──────────────────────────┘
```

## 장표 4: Warp 내부 — Controller/Queue 선택 메커니즘

```
Warp (32 threads: lane 0 ~ lane 31)

┌──────────────── lane 0만 실행 ─────────────────┐
│                                                 │
│  // 순차 커널: ctrl은 라운드로빈, queue는 SM 고정│
│  ctrl = ctrl_counter.fetch_add(1) % n_ctrls    │
│  queue = smid % n_qps                           │
│                                                 │
│  // 랜덤 커널: ctrl, queue 모두 라운드로빈      │
│  ctrl = ctrl_counter.fetch_add(1) % n_ctrls    │
│  queue = queue_counter.fetch_add(1) % n_qps    │
│                                                 │
└─────────────────────┬───────────────────────────┘
                      │
                      ▼
┌──────────────── 전체 warp에 전파 ──────────────┐
│  ctrl  = __shfl_sync(0xFFFFFFFF, ctrl, 0)      │
│  queue = __shfl_sync(0xFFFFFFFF, queue, 0)     │
│                                                 │
│  → 32개 thread 모두 동일한 (ctrl, queue) 사용  │
└─────────────────────────────────────────────────┘

★ 순차 vs 랜덤의 큐 선택 차이:
┌──────────────────┬────────────────────────────┐
│ 순차 커널        │ 랜덤 커널                  │
│ queue = smid%nqps│ queue = fetch_add(1)%nqps  │
│ SM 고정 바인딩   │ 동적 라운드로빈             │
│ → 지역성 ↑      │ → 분산 ↑                   │
└──────────────────┴────────────────────────────┘
```

## 장표 5: 단일 Thread의 NVMe I/O 발행 흐름 (핵심)

```
GPU Thread (tid=N, QP[q] 사용)
  │
  ├── ① LBA 계산
  │   순차: start_block = (tid × req_size) >> block_size_log
  │   랜덤: start_block = (assignment[tid] × req_size) >> ..
  │   n_blocks = req_size >> block_size_log
  │
  ├── ② reqs_per_thread 만큼 반복:
  │   │
  │   └── read_data(pc, qp, start_block, n_blocks, tid)
  │         │
  │         ├── get_cid(&qp->sq)
  │         │   → cid_ticket.fetch_add(1) → CID 할당
  │         │
  │         ├── nvm_cmd_header(&cmd, cid, READ, ns)
  │         │   → cmd.dword[0] = opcode | CID
  │         │
  │         ├── nvm_cmd_data_ptr(&cmd, prp1[tid], prp2)
  │         │   → cmd.dword[6..9] = GPU VRAM DMA 주소
  │         │
  │         ├── nvm_cmd_rw_blks(&cmd, start_block, n_blocks)
  │         │   → cmd.dword[10..11] = 시작 LBA + 블록 수
  │         │
  │         ├── sq_enqueue(&qp->sq, &cmd)
  │         │   → in_ticket.fetch_add(1) → slot 할당
  │         │   → tickets[pos] 대기 (generation 확인)
  │         │   → 64B cmd을 SQ slot에 복사
  │         │   → tail_mark[pos] = LOCKED
  │         │   → move_tail() → doorbell 배칭
  │         │   → asm("st.mmio ... [db], new_tail")
  │         │   ─────── PCIe P2P ──────→ NVMe SSD
  │         │
  │         ├── cq_poll(&qp->cq, cid)
  │         │   → CQ 엔트리 순회하며 CID 매칭 검색
  │         │   → phase bit 확인 (유효한 완료인지)
  │         │   ◄────── SSD가 CQ에 완료 기록 ──────
  │         │
  │         ├── cq_dequeue(&qp->cq, cq_pos, &qp->sq)
  │         │   → head_mark[pos] = LOCKED
  │         │   → move_head_cq() → CQ doorbell 배칭
  │         │   → SQ Head Ptr 읽기 → SQ tickets 해제
  │         │
  │         └── put_cid(&qp->sq, cid)
  │             → cid[id] = UNLOCKED (CID 풀에 반환)
  │
  └── Thread 종료 (모든 반복 완료)
```

## 장표 6: Warp 32개 Thread의 동시 SQ 접근 + 도어벨 배칭

```
시간 →

t0:  fetch_add → ticket=100 → slot[100]에 cmd 복사
t1:  fetch_add → ticket=101 → slot[101]에 cmd 복사
t2:  fetch_add → ticket=102 → slot[102]에 cmd 복사
...
t31: fetch_add → ticket=131 → slot[131]에 cmd 복사

각 thread: tail_mark[pos] = LOCKED

SQ: [... |100|101|102|...|131| ...]
          │   │   │       │
         LOCK LOCK LOCK  LOCK

Thread X (tail_lock 획득):
  move_tail():
    slot[100] LOCKED → 수집, count=1
    slot[101] LOCKED → 수집, count=2
    ...
    slot[131] LOCKED → 수집, count=32

  asm("st.mmio [db], 132")  ← doorbell 1회로 32개 커맨드!
  ──────── PCIe MMIO ─────→ NVMe Controller

★ 32개 커맨드 × 1회 doorbell = PCIe MMIO 오버헤드 1/32
```

## 장표 7: NVMe Controller 처리 + PCIe P2P 데이터 전송

```
NVMe Controller
  │
  ├── SQ Doorbell 수신 (new_tail=132)
  │   → "slot[100]~[131]에 32개 새 커맨드가 있다"
  │
  ├── SQ에서 커맨드 fetch (PCIe P2P → GPU VRAM 읽기)
  │   cmd[100]: READ LBA=1000, PRP=0xGPU_ADDR_0
  │   cmd[101]: READ LBA=2000, PRP=0xGPU_ADDR_1
  │   ...
  │   cmd[131]: READ LBA=9999, PRP=0xGPU_ADDR_31
  │
  ├── NAND Flash에서 데이터 읽기 (내부 처리)
  │
  ├── PRP 주소로 데이터 DMA (PCIe P2P → GPU VRAM 쓰기)
  │   0xGPU_ADDR_0 ← 4KB 데이터 (LBA 1000)
  │   0xGPU_ADDR_1 ← 4KB 데이터 (LBA 2000)
  │   ...
  │   (순서 보장 안 됨 — NVMe 내부 스케줄링에 따라)
  │
  └── CQ에 완료 엔트리 기록 (PCIe P2P → GPU VRAM 쓰기)
      CQ entry: { CID=X, Status=OK, SQ Head Ptr=132 }
      → GPU Thread가 cq_poll()에서 이것을 발견

★ 전체 데이터 경로: SSD ↔ GPU VRAM (Host DRAM 무경유)
★ CPU 개입: 0회 (커널 실행 중)
```

## 장표 8: 벤치마크 성능 측정

```
┌─────────── CUDA Event 기반 타이밍 ──────────┐
│                                               │
│  Event before;                ← cudaEventRecord
│  kernel<<<g_size, b_size>>>(...);
│  Event after;                 ← cudaEventRecord
│  cudaDeviceSynchronize();
│                                               │
│  elapsed = after - before;    (마이크로초)
└───────────────────────────────────────────────┘

측정 지표:
┌──────────────────────────────────────────────┐
│ ios       = n_threads × reqs_per_thread       │
│ data      = ios × page_size                   │
│ iops      = ios / (elapsed / 1,000,000)       │
│ bandwidth = data / elapsed(초) / 1GiB         │
└──────────────────────────────────────────────┘

+ 캐시 통계: h_pc.print_reset_stats()
  → hit count, miss count, hit rate

출력 예:
  Elapsed: 1234567 μs
  Ops: 65536000   Data: 268,435,456,000 bytes
  IOPS: 530,841   Bandwidth: 2.04 GB/s
  Cache Hit: 85.3%
```

## 장표 순서 요약

| 장표 | 제목 | 초점 |
|:----:|------|------|
| 1 | 전체 프로그램 흐름 | Big Picture — 7 Phase 개요 |
| 2 | Host 초기화 | NVMe Controller + Page Cache 생성 |
| 3 | 커널 Launch 구성 | grid/block 차원, 파라미터 |
| 4 | Warp 큐 선택 | 순차 vs 랜덤의 ctrl/queue 배정 차이 |
| 5 | Thread I/O 흐름 | **read_data() 함수 호출 체인 전체** (가장 핵심) |
| 6 | Doorbell 배칭 | 32 thread 동시 접근 → 1회 doorbell |
| 7 | NVMe 측 처리 | SQ fetch → NAND read → P2P DMA → CQ write |
| 8 | 성능 측정 | IOPS, Bandwidth, Cache Hit 계산 |

장표 5(Thread I/O 흐름)와 장표 6(Doorbell 배칭)이 발표의 하이라이트.
이 두 장에서 "GPU가 NVMe 프로토콜을 직접 수행한다"는 것이 가장 선명하게 드러남.
