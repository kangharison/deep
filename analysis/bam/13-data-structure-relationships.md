# BaM Block Benchmark 자료구조 연결 관계도

## 1. main.cu에서의 생성 순서와 인자 전달

```
main(argc, argv)
│
├─[1] Settings settings
│     settings.parseArguments(argc, argv)
│     ┌─────────────────────────────────────────────────────────────┐
│     │ Settings (stack, settings.h:27-63)                          │
│     │                                                             │
│     │  cudaDevice:     uint32_t    = 0       ─┬─► Controller()    │
│     │  nvmNamespace:   uint32_t    = 1       ─┤   인자로 전달     │
│     │  queueDepth:     size_t      = 16      ─┤                   │
│     │  numQueues:      size_t      = 1       ─┘                   │
│     │  numReqs:        size_t      = 1       ─── 커널 인자        │
│     │  numPages:       size_t      = 1024    ─┬─► page_cache_t()  │
│     │  pageSize:       size_t      = 4096    ─┤   인자로 전달     │
│     │  n_ctrls:        uint32_t    = 1       ─┘                   │
│     │  numThreads:     size_t      = 1024    ─── grid/block 계산  │
│     │  blkSize:        size_t      = 64      ─── block size       │
│     │  numBlks:        uint64_t    = 2097152 ─── LBA 범위         │
│     │  random:         bool        = true    ─── 커널 선택        │
│     │  accessType:     uint64_t    = 0(READ) ─── 커널 인자        │
│     │  ratio:          uint64_t    = 100     ─── read/write 비율  │
│     └─────────────────────────────────────────────────────────────┘
│
├─[2] cudaSetDevice(settings.cudaDevice)
│
├─[3] Controller 배열 생성 (main.cu:254-256)
│     std::vector<Controller*> ctrls(settings.n_ctrls)
│     for i = 0..n_ctrls:
│       ctrls[i] = new Controller(
│           ctrls_paths[i],          // "/dev/libnvm0"
│           settings.nvmNamespace,   // 1
│           settings.cudaDevice,     // 0
│           settings.queueDepth,     // 16
│           settings.numQueues       // 1
│       )
│
├─[4] page_cache_t 생성 (main.cu:289)
│     page_cache_t h_pc(
│         page_size,               // settings.pageSize (4096)
│         n_pages,                 // settings.numPages (1024)
│         settings.cudaDevice,     // 0
│         ctrls[0][0],             // 첫 번째 Controller 참조
│         (uint64_t) 64,           // max_ranges
│         ctrls                    // 전체 Controller 벡터
│     )
│
├─[5] d_pc 포인터 획득 (main.cu:293)
│     page_cache_d_t* d_pc = (page_cache_d_t*)(h_pc.d_pc_ptr)
│
└─[6] 커널 런치 (main.cu:343-346)
      random_access_kernel<<<g_size, b_size>>>(
          h_pc.pdt.d_ctrls,        // Controller** (Device)
          d_pc,                     // page_cache_d_t* (Device)
          page_size,                // uint32_t
          n_threads,                // uint32_t
          d_req_count,              // unsigned long long* (Device)
          settings.n_ctrls,         // uint32_t
          d_assignment,             // uint64_t* (Device, random용)
          settings.numReqs,         // uint64_t
          settings.accessType,      // uint32_t
          d_access_assignment       // uint8_t* (Device)
      )
```

---

## 2. 모든 자료구조의 필드-필드 연결 (완전판)

아래 다이어그램에서:
- `───►` = "이 포인터가 저 메모리를 가리킨다"
- `═══►` = "이 값이 저 값에서 복사/파생되었다"
- `(H)` = Host 메모리, `(D)` = Device(GPU) 메모리, `(BAR0)` = PCIe MMIO

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                            ║
║   Controller  (ctrl.h:39-78)                                               ║
║   위치: Host에 생성, Device에 복사본 (d_ctrl_ptr)                          ║
║                                                                            ║
║   ┌────────────────────────────────────────────────────────────────────┐    ║
║   │ access_counter: atomic<uint64_t>  (D)  ← GPU 커널이 I/O마다 증가 │    ║
║   │                                                                    │    ║
║   │ ctrl: nvm_ctrl_t* ──────────────────────────────────────────────┐  │    ║
║   │                                                                 │  │    ║
║   │ aq_ref: nvm_aq_ref ──────────────────────────────────────────┐  │  │    ║
║   │                                                              │  │  │    ║
║   │ aq_mem: DmaPtr ────────────────────────────────────────────┐ │  │  │    ║
║   │                                                            │ │  │  │    ║
║   │ info: nvm_ctrl_info ──────────────────────────────────┐    │ │  │  │    ║
║   │                                                       │    │ │  │  │    ║
║   │ ns: nvm_ns_info ──────────────────────────────────┐   │    │ │  │  │    ║
║   │                                                   │   │    │ │  │  │    ║
║   │ n_sqs:  uint16_t  ═► min(n_sqs,n_cqs,numQueues)  │   │    │ │  │  │    ║
║   │ n_cqs:  uint16_t      = n_qps                    │   │    │ │  │  │    ║
║   │ n_qps:  uint16_t ─────────────────────────────┐   │   │    │ │  │  │    ║
║   │                                               │   │   │    │ │  │  │    ║
║   │ deviceId: uint32_t  ═► settings.cudaDevice    │   │   │    │ │  │  │    ║
║   │                                               │   │   │    │ │  │  │    ║
║   │ h_qps: QueuePair** ─────────────────────────┐ │   │   │    │ │  │  │    ║
║   │                                             │ │   │   │    │ │  │  │    ║
║   │ d_qps: QueuePair*  ═► cudaMemcpy(h_qps) ─┐ │ │   │   │    │ │  │  │    ║
║   │                                           │ │ │   │   │    │ │  │  │    ║
║   │ queue_counter: atomic<uint64_t>  (D)      │ │ │   │   │    │ │  │  │    ║
║   │                                           │ │ │   │   │    │ │  │  │    ║
║   │ page_size: uint32_t  ═► ctrl->page_size   │ │ │   │   │    │ │  │  │    ║
║   │ blk_size:  uint32_t  ═► ns.lba_data_size  │ │ │   │   │    │ │  │  │    ║
║   │ blk_size_log: uint32_t ═► log2(blk_size)  │ │ │   │   │    │ │  │  │    ║
║   │                                           │ │ │   │   │    │ │  │  │    ║
║   │ d_ctrl_ptr: void* ─► Device의 자기 복사본 │ │ │   │   │    │ │  │  │    ║
║   │ d_ctrl_buff: BufferPtr ─► 위 메모리 관리  │ │ │   │   │    │ │  │  │    ║
║   └───────────────────────────────────────────┼─┼─┼───┼───┼────┼─┼──┼──┘    ║
║                                               │ │ │   │   │    │ │  │       ║
╚═══════════════════════════════════════════════╪═╪═╪═══╪═══╪════╪═╪══╪═══════╝
                                                │ │ │   │   │    │ │  │
    ┌───────────────────────────────────────────┘ │ │   │   │    │ │  │
    │   ┌─────────────────────────────────────────┘ │   │   │    │ │  │
    │   │   ┌───────────────────────────────────────┘   │   │    │ │  │
    │   │   │                                           │   │    │ │  │
    ▼   ▼   ▼                                           │   │    │ │  │
╔═══════════════════════════════════════════════════════╪═══╪════╪═╪══╪════╗
║                                                       │   │    │ │  │    ║
║   QueuePair  (queue.h:27-223)                         │   │    │ │  │    ║
║   위치: Host에 생성, d_qps[]로 Device에 복사           │   │    │ │  │    ║
║                                                       │   │    │ │  │    ║
║   h_qps[0], h_qps[1], ..., h_qps[n_qps-1]            │   │    │ │  │    ║
║   ┌───────────────────────────────────────────────┐   │   │    │ │  │    ║
║   │                                               │   │   │    │ │  │    ║
║   │ pageSize:        uint32_t ═══════════════════════════════════╪══╪═►  ║
║   │                  ctrl->page_size에서 복사      │   │   │    │ │  │    ║
║   │                                               │   │   │    │ │  │    ║
║   │ block_size:      uint32_t ═══════════════════════════►│    │ │  │    ║
║   │                  ns.lba_data_size에서 복사 ────┤   │   │    │ │  │    ║
║   │ block_size_log:  uint32_t                     │   │   │    │ │  │    ║
║   │                  log2(block_size)              │   │   │    │ │  │    ║
║   │ block_size_minus_1: uint32_t                  │   │   │    │ │  │    ║
║   │                  block_size - 1               │   │   │    │ │  │    ║
║   │                                               │   │   │    │ │  │    ║
║   │ nvmNamespace:    uint32_t ═══════════════════════════════════╪══╪═►  ║
║   │                  ns.ns_id에서 복사             │   │   │    │ │  │    ║
║   │                                               │   │   │    │ │  │    ║
║   │ qp_id:           uint16_t (1, 2, 3, ...)      │   │   │    │ │  │    ║
║   │                                               │   │   │    │ │  │    ║
║   │ sq: nvm_queue_t ──────────────────────── (상세: §3) │   │    │ │  │    ║
║   │ cq: nvm_queue_t ──────────────────────── (상세: §3) │   │    │ │  │    ║
║   │                                               │   │   │    │ │  │    ║
║   │ sq_mem: DmaPtr ───────────────────────────┐   │   │   │    │ │  │    ║
║   │ cq_mem: DmaPtr ─────────────────────────┐ │   │   │   │    │ │  │    ║
║   │ prp_mem: DmaPtr ──────────────────────┐  │ │   │   │   │    │ │  │    ║
║   │                                       │  │ │   │   │   │    │ │  │    ║
║   │ sq_tickets:   BufferPtr ────────────┐ │  │ │   │   │   │    │ │  │    ║
║   │ sq_tail_mark: BufferPtr ──────────┐ │ │  │ │   │   │   │    │ │  │    ║
║   │ sq_cid:       BufferPtr ────────┐ │ │ │  │ │   │   │   │    │ │  │    ║
║   │ cq_head_mark: BufferPtr ──────┐ │ │ │ │  │ │   │   │   │    │ │  │    ║
║   │ cq_pos_locks: BufferPtr ────┐ │ │ │ │ │  │ │   │   │   │    │ │  │    ║
║   └─────────────────────────────┼─┼─┼─┼─┼─┼──┼─┼───┘   │   │    │ │  │    ║
║                                 │ │ │ │ │ │  │ │        │   │    │ │  │    ║
╚═════════════════════════════════╪═╪═╪═╪═╪═╪══╪═╪════════╪═══╪════╪═╪══╪════╝
                                  │ │ │ │ │ │  │ │        │   │    │ │  │
    ┌─────────────────────────────┘ │ │ │ │ │  │ │        │   │    │ │  │
    │ ┌─────────────────────────────┘ │ │ │ │  │ │        │   │    │ │  │
    │ │ ┌─────────────────────────────┘ │ │ │  │ │        │   │    │ │  │
    │ │ │ ┌─────────────────────────────┘ │ │  │ │        │   │    │ │  │
    │ │ │ │ ┌─────────────────────────────┘ │  │ │        │   │    │ │  │
    ▼ ▼ ▼ ▼ ▼                              ▼  ▼ ▼        ▼   ▼    ▼ ▼  ▼
```

---

## 3. nvm_queue_t 필드별 연결 상세 (nvm_types.h:428-661)

```
nvm_queue_t (SQ용과 CQ용 2개가 QueuePair에 존재)
│
│  ★ 각 필드가 어디에서 값을 받고, 어디를 가리키는지 표시
│
│  ═══════════════════ 패딩된 Atomic 필드들 ═══════════════════
│  (각 32바이트 캐시라인 단위로 분리)
│
├── head_lock: atomic<uint32_t, thread_scope_device>   (line 436)
│   pad0[28]                                           (line 445)
│   ├── 초기값: 0 (UNLOCKED)
│   ├── 사용: sq_enqueue()에서 tail doorbell 쓰기 전 획득
│   │         cq_dequeue()에서 head doorbell 쓰기 전 획득
│   └── 범위: GPU 스레드 간 동기화 (device scope)
│
├── tail_lock: atomic<uint32_t, thread_scope_device>   (line 451)
│   pad1[28]                                           (line 452)
│   └── 사용: (예약됨, 현재 미사용 — head_lock이 통합 관리)
│
├── head: atomic<uint32_t, thread_scope_device>        (line 460)
│   pad2[28]                                           (line 461)
│   ├── 초기값: 0
│   ├── SQ.head: SSD가 처리 완료한 위치 (cq_dequeue에서 갱신)
│   ├── CQ.head: GPU가 처리 완료한 위치 (cq_dequeue에서 갱신)
│   └── *(cq.db) = head  ← 이 값이 CQ Doorbell에 쓰인다
│
├── tail: atomic<uint32_t, thread_scope_device>        (line 468)
│   pad3[28]                                           (line 469)
│   ├── 초기값: 0
│   ├── SQ.tail: GPU가 커맨드를 넣은 마지막 위치
│   ├── CQ.tail: SSD가 완료를 넣은 마지막 위치
│   └── *(sq.db) = tail  ← 이 값이 SQ Doorbell에 쓰인다
│
├── tail_copy: atomic<uint32_t, thread_scope_system>   (line 478)
│   pad4[28]                                           (line 479)
│   ├── scope = system → CPU에서도 가시적
│   └── Admin Queue 등 CPU 측에서 tail 확인용
│
├── head_copy: atomic<uint32_t, thread_scope_system>   (line 486)
│   pad5[28]                                           (line 487)
│   └── CPU에서 head 확인용
│
├── in_ticket: atomic<uint32_t, thread_scope_device>   (line 506)
│   pad6[28]                                           (line 507)
│   ├── 초기값: 0
│   ├── sq_enqueue()에서: ticket = in_ticket.fetch_add(1)
│   │   pos = ticket & qs_minus_1     ← 큐 내 슬롯 위치
│   │   round = ticket >> qs_log2     ← 라운드 번호
│   └── 매 I/O마다 1씩 증가 (전체 I/O 카운터 역할)
│
├── cid_ticket: atomic<uint32_t, thread_scope_device>  (line 517)
│   ├── Command ID 할당용 티켓
│   └── get_cid()에서 사용
│
│  ═══════════ 동기화 배열 포인터들 ═══════════
│  (QueuePair의 BufferPtr가 할당한 GPU 메모리를 가리킴)
│
├── tickets: padded_struct* ────────────────────────── (line 526)
│   │
│   │  ┌── QueuePair.sq_tickets (BufferPtr)가 할당
│   │  │   cudaMalloc(qs × sizeof(padded_struct))
│   │  │   = qs × 32 bytes
│   │  │
│   │  └──► padded_struct tickets[qs]  (GPU VRAM)
│   │       ┌───────────────────────────────────────┐
│   │       │ tickets[0].val: atomic<uint32_t> = 0  │
│   │       │ tickets[1].val: atomic<uint32_t> = 0  │
│   │       │ ...                                   │
│   │       │ tickets[qs-1].val = 0                 │
│   │       └───────────────────────────────────────┘
│   │       각 32B 정렬 (false sharing 방지)
│   │
│   │  동작: sq_enqueue()에서
│   │    pos = ticket & qs_minus_1
│   │    round = ticket >> qs_log2
│   │    while (tickets[pos].val != round):  ← 내 차례 대기
│   │      __nanosleep(ns)
│   │    ... 커맨드 복사 ...
│   │    tickets[pos].val.fetch_add(1)       ← 다음 라운드 허용
│   │
│   └── 연결: in_ticket의 ticket 값으로 pos 계산 → tickets[pos] 접근
│
├── head_mark: padded_struct* ──────────────────────── (line 532)
│   │
│   │  ┌── QueuePair.cq_head_mark (BufferPtr)가 할당
│   │  └──► padded_struct head_mark[qs]  (GPU VRAM)
│   │
│   │  동작: cq_dequeue()에서
│   │    head_mark[pos].val = LOCKED     ← "이 SQ 슬롯 완료됨"
│   │    move_head_cq()에서:
│   │      연속 LOCKED 슬롯만큼 head 전진
│   │      head_mark[pos].val = UNLOCKED ← "슬롯 재사용 가능"
│   │    sq_enqueue()에서:
│   │      while (head_mark[pos] == LOCKED): wait ← 재사용 대기
│   │
│   └── 연결: SQ의 head_mark이지만, CQ dequeue가 값을 설정함
│            SQ enqueue가 값을 확인함 (SQ↔CQ 동기화 매개체)
│
├── tail_mark: padded_struct* ──────────────────────── (line 538)
│   │
│   │  ┌── QueuePair.sq_tail_mark (BufferPtr)가 할당
│   │  └──► padded_struct tail_mark[qs]  (GPU VRAM)
│   │
│   │  동작: sq_enqueue()에서
│   │    [커맨드 복사 후]
│   │    tail_mark[pos].val = LOCKED     ← "이 슬롯에 cmd 준비됨"
│   │    move_tail()에서:
│   │      연속 LOCKED 슬롯만큼 tail 전진
│   │      tail_mark[pos].val = UNLOCKED
│   │    → tail이 전진하면 *(sq.db) = tail 로 SSD에 통보
│   │
│   └── 연결: in_ticket → pos 계산 → tail_mark[pos] 설정
│            → move_tail() → sq.tail 갱신 → sq.db doorbell
│
├── cid: padded_struct* ────────────────────────────── (line 544)
│   │
│   │  ┌── QueuePair.sq_cid (BufferPtr)가 할당
│   │  │   cudaMalloc(65536 × sizeof(padded_struct))
│   │  └──► padded_struct cid[65536]  (GPU VRAM)
│   │
│   │  동작: get_cid()에서
│   │    cid_slot = cid_ticket.fetch_add(1) & 0xFFFF
│   │    while (cid[cid_slot].val != expected): wait
│   │    return cid_slot  ← 이 값이 NVMe cmd의 CID 필드에 들어감
│   │
│   │  put_cid()에서:
│   │    cid[cid_slot].val.fetch_add(1) ← 다음 사용 허용
│   │
│   └── 연결: cid_ticket → cid_slot → cid[cid_slot]
│            반환된 cid_slot → nvm_cmd_t.dword[0] bits[31:16]
│            SSD가 완료 시 nvm_cpl_t.dword[3] bits[15:0]에 동일 CID 반환
│
├── pos_locks: padded_struct* ──────────────────────── (line 550)
│   │
│   │  ┌── QueuePair.cq_pos_locks (BufferPtr)가 할당
│   │  └──► padded_struct pos_locks[qs]  (GPU VRAM)
│   │
│   │  동작: cq_dequeue()에서
│   │    while (!pos_locks[pos].val.CAS(UNLOCKED, LOCKED)): wait
│   │    ... 완료 처리 ...
│   │    pos_locks[pos].val = UNLOCKED
│   │
│   └── 연결: cq_poll()이 반환한 pos → pos_locks[pos]로 상호 배제
│
├── clean_cid: uint16_t* ──────────────────────────── (line 557)
│   └── CID 정리용 배열
│
│  ═══════════ 스칼라 필드들 ═══════════
│
├── qs:           uint32_t ═► nvm_admin_cq/sq_create()의 결과  (line 597)
│   └── 큐 엔트리 수 (예: 1024)
│
├── qs_minus_1:   uint32_t = qs - 1                            (line 566)
│   └── pos = ticket & qs_minus_1 (모듈러 연산 대체)
│
├── qs_log2:      uint32_t = log2(qs)                          (line 574)
│   └── round = ticket >> qs_log2
│
├── no:           uint16_t ═► qp_id (큐 번호)                 (line 582)
├── es:           uint16_t (SQ: 64, CQ: 16)                   (line 590)
│   └── SQ는 nvm_cmd_t(64B) 크기, CQ는 nvm_cpl_t(16B) 크기
│
├── phase:        int8_t = 1 (초기값)                          (line 618)
│   └── CQ에서: 큐가 한 바퀴 돌 때마다 0↔1 토글
│       cq_poll()에서 expected_phase와 비교하여 새 엔트리 판별
│
├── local:        int8_t                                       (line 625)
│   └── 1 = GPU 로컬 메모리, 0 = 원격
│
├── last:         uint32_t                                     (line 632)
│   └── 마지막 doorbell에 쓴 값
│
│  ═══════════ 핵심 포인터 3개 ═══════════
│
├── db: volatile uint32_t* ──────────────────────────── (line 646)
│   │
│   │  ┌── 값의 출처:
│   │  │   nvm_admin_sq_create() / nvm_admin_cq_create()가
│   │  │   BAR0에서 doorbell 오프셋 계산 후 설정
│   │  │
│   │  │   이후 cudaHostGetDevicePointer(&devPtr, db, 0)로
│   │  │   GPU에서 접근 가능한 포인터로 변환
│   │  │
│   │  └──► NVMe BAR0 Doorbell Register  (BAR0, PCIe MMIO)
│   │       ┌─────────────────────────────────────────────┐
│   │       │ BAR0 + 0x1000 + (2*qp_id) * (4 << dstrd)   │ SQ Tail DB
│   │       │ BAR0 + 0x1000 + (2*qp_id+1) * (4 << dstrd) │ CQ Head DB
│   │       └─────────────────────────────────────────────┘
│   │
│   │  사용:
│   │    *(sq.db) = sq.tail   → SSD에 "새 커맨드 tail까지 있다" 알림
│   │    *(cq.db) = cq.head   → SSD에 "head까지 처리했다" 알림
│   │
│   └── 연결: sq.tail → *(sq.db), cq.head → *(cq.db)
│            db 주소 = BAR0(mmap) → cudaHostRegister → cudaHostGetDevicePointer
│            즉 nvm_ctrl_t.mm_ptr 기반
│
├── vaddr: volatile void* ───────────────────────────── (line 653)
│   │
│   │  ┌── 값의 출처:
│   │  │   QueuePair 생성자에서:
│   │  │   sq_mem = createDma(ctrl, size, cudaDevice)
│   │  │   → cudaMalloc()으로 GPU VRAM 할당
│   │  │   → ioctl(NVM_MAP_DEVICE_MEMORY)로 DMA 주소 획득
│   │  │   sq.vaddr = sq_mem->vaddr (GPU 가상 주소)
│   │  │
│   │  └──► GPU VRAM의 큐 데이터 영역
│   │       ┌─────────────────────────────────────────────┐
│   │       │ SQ의 경우: nvm_cmd_t[qs] 배열  (각 64B)    │
│   │       │  ┌──────────────────────────────────────┐   │
│   │       │  │ cmd[0]: dword[0..15]  ← 64바이트     │   │
│   │       │  │   dword[0]: opcode(8b) + flags + CID │   │
│   │       │  │   dword[1]: NSID                     │   │
│   │       │  │   dword[6-7]: PRP1 (64-bit)          │   │
│   │       │  │   dword[8-9]: PRP2 (64-bit)          │   │
│   │       │  │   dword[10-11]: Starting LBA (64-bit)│   │
│   │       │  │   dword[12]: NLB (bits 15:0)         │   │
│   │       │  ├──────────────────────────────────────┤   │
│   │       │  │ cmd[1]: ...                          │   │
│   │       │  │ ...                                  │   │
│   │       │  │ cmd[qs-1]: ...                       │   │
│   │       │  └──────────────────────────────────────┘   │
│   │       │                                             │
│   │       │ CQ의 경우: nvm_cpl_t[qs] 배열  (각 16B)    │
│   │       │  ┌──────────────────────────────────────┐   │
│   │       │  │ cpl[0]: dword[0..3]  ← 16바이트     │   │
│   │       │  │   dword[0]: Command Specific         │   │
│   │       │  │   dword[1]: Reserved                 │   │
│   │       │  │   dword[2]: SQ Head Ptr + SQ ID      │   │
│   │       │  │   dword[3]: Status + Phase + CID     │   │
│   │       │  │     bits[15:0] = CID ← sq의 cmd CID  │   │
│   │       │  │     bit[16] = Phase Tag               │   │
│   │       │  │     bits[31:17] = Status              │   │
│   │       │  ├──────────────────────────────────────┤   │
│   │       │  │ cpl[1]: ...                          │   │
│   │       │  └──────────────────────────────────────┘   │
│   │       └─────────────────────────────────────────────┘
│   │
│   │  사용:
│   │    sq_enqueue: memcpy(sq.vaddr + pos*64, &cmd, 64) ← 커맨드 복사
│   │    cq_poll:    cpl = *(cq.vaddr + loc*16)          ← 완료 읽기
│   │
│   └── 연결: vaddr = DmaPtr.vaddr = cudaMalloc() 결과
│            SSD는 ioaddr(DMA 주소)로 이 메모리에 접근
│
└── ioaddr: uint64_t ────────────────────────────────── (line 660)
    │
    │  ┌── 값의 출처:
    │  │   createDma() → ioctl(NVM_MAP_DEVICE_MEMORY)
    │  │   커널 모듈이 nvidia_p2p_dma_map_pages()로 획득한 DMA 주소
    │  │   nvm_dma_t.ioaddrs[0]에서 복사
    │  │
    │  └──► PCIe Bus Address (물리 주소 공간)
    │       SSD 컨트롤러가 이 주소로:
    │       - SQ에서 커맨드를 DMA Read (SQ ioaddr)
    │       - CQ에 완료 엔트리를 DMA Write (CQ ioaddr)
    │
    │  사용:
    │    nvm_admin_cq_create(aq_ref, &cq, qp_id, cq_mem, ...)
    │    → cq_mem->ioaddrs[0]이 SSD에 등록됨
    │    → SSD 펌웨어가 이 주소로 DMA 전송
    │
    └── 연결: ioaddr = nvm_dma_t.ioaddrs[0]
             vaddr와 ioaddr는 같은 GPU 메모리의 두 가지 주소:
             vaddr = GPU가 보는 가상 주소
             ioaddr = SSD가 보는 PCIe DMA 주소
```

---

## 4. page_cache_d_t 필드별 연결 상세 (page_cache.h:498-548)

```
page_cache_d_t  (GPU 커널이 접근하는 핵심 구조체)
│
│  page_cache_t에서 pdt 멤버로 Host에 존재
│  d_pc_ptr = cudaMemcpy(pdt → GPU) 로 Device에 복사
│
├── base_addr: uint8_t* ────────────────────────────── (line 500)
│   │
│   │  ┌── 값의 출처:
│   │  │   page_cache_t 생성자에서:
│   │  │   pages_dma = createDma(ctrl, n_pages * page_size, cudaDevice)
│   │  │   pdt.base_addr = (uint8_t*) pages_dma->vaddr
│   │  │
│   │  └──► GPU VRAM: 캐시 데이터 버퍼 (DMA 매핑됨)
│   │       ┌─────────────────────────────────────────────────────┐
│   │       │ offset 0:                  Page 0 데이터 (4096B)    │
│   │       │ offset page_size:          Page 1 데이터 (4096B)    │
│   │       │ offset page_size*2:        Page 2 데이터 (4096B)    │
│   │       │ ...                                                 │
│   │       │ offset page_size*(n-1):    Page n-1 데이터 (4096B)  │
│   │       └─────────────────────────────────────────────────────┘
│   │       총 크기: n_pages × page_size (예: 1024 × 4096 = 4MB)
│   │
│   │  사용:
│   │    data = base_addr + cache_page.page_translation * page_size
│   │    → 캐시 페이지의 실제 데이터 위치
│   │
│   │  SSD와의 관계:
│   │    pages_dma->ioaddrs[i] = 이 버퍼 내 페이지 i의 DMA 주소
│   │    NVMe cmd.PRP1 = pages_dma->ioaddrs[page_idx]
│   │    SSD가 이 DMA 주소로 P2P Write → base_addr 영역에 데이터 착륙
│   │
│   └── 연결: base_addr ←═ pages_dma.vaddr
│            pages_dma.ioaddrs[] ←═ prp1[] 배열에 복사
│            prp1[i] → NVMe cmd → SSD DMA → base_addr + i*page_size
│
├── page_size:         uint64_t ═► Settings.pageSize          (line 501)
├── page_size_minus_1: uint64_t = page_size - 1               (line 502)
├── page_size_log:     uint64_t = log2(page_size)             (line 503)
├── n_pages:           uint64_t ═► Settings.numPages          (line 504)
├── n_pages_minus_1:   uint64_t = n_pages - 1                 (line 505)
│
├── cache_pages: cache_page_t* ─────────────────────── (line 506)
│   │
│   │  ┌── 값의 출처:
│   │  │   cache_pages_buf = allocate(n_pages * sizeof(cache_page_t))
│   │  │   pdt.cache_pages = (cache_page_t*) cache_pages_buf.get()
│   │  │
│   │  └──► GPU VRAM: 페이지 메타데이터 배열
│   │       ┌─────────────────────────────────────────────────┐
│   │       │ cache_page_t[0]:  (32B 정렬)                    │
│   │       │   page_take_lock: atomic<uint32_t>              │
│   │       │     └── 페이지 교체 시 CAS로 획득하는 락        │
│   │       │   page_translation: uint32_t                    │
│   │       │     └── 이 캐시 슬롯이 매핑된 논리 페이지 번호  │
│   │       │         base_addr + page_translation * page_size│
│   │       │         = 실제 데이터 위치                       │
│   │       │   pad[24]: 패딩                                 │
│   │       ├─────────────────────────────────────────────────┤
│   │       │ cache_page_t[1]: ...                            │
│   │       │ ...                                             │
│   │       │ cache_page_t[n_pages-1]: ...                    │
│   │       └─────────────────────────────────────────────────┘
│   │
│   │  사용: find_slot()에서
│   │    slot = page_ticket.fetch_add(1) & n_pages_minus_1
│   │    cache_pages[slot].page_take_lock.CAS(old, new)
│   │    cache_pages[slot].page_translation = 새 논리 페이지 ID
│   │
│   └── 연결: find_slot() → cache_pages[slot] → page_translation
│            → base_addr + page_translation * page_size = 데이터
│            → prp1[page_translation] = SSD에 줄 DMA 주소
│
├── page_ticket: padded_struct_pc* ─────────────────── (line 510)
│   │  atomic<uint32_t> (단일 카운터)
│   │  └── 캐시 슬롯 할당: slot = page_ticket.fetch_add(1) % n_pages
│   └── 연결: page_ticket → slot 번호 → cache_pages[slot]
│
├── prp1: uint64_t* ────────────────────────────────── (line 511)
│   │
│   │  ┌── 값의 출처:
│   │  │   prp1_buf = allocate(n_pages * sizeof(uint64_t))
│   │  │   for i in 0..n_pages:
│   │  │     prp1[i] = pages_dma->ioaddrs[i]
│   │  │   즉, 각 캐시 페이지의 DMA 주소를 미리 계산해둔 테이블
│   │  │
│   │  └──► GPU VRAM: DMA 주소 테이블 [n_pages]
│   │       ┌────────────────────────────────────────┐
│   │       │ prp1[0] = pages_dma->ioaddrs[0]        │
│   │       │ prp1[1] = pages_dma->ioaddrs[1]        │
│   │       │ ...                                    │
│   │       │ prp1[n-1] = pages_dma->ioaddrs[n-1]   │
│   │       └────────────────────────────────────────┘
│   │
│   │  사용: read_data() / write_data()에서
│   │    nvm_cmd_data_ptr(&cmd, prp1[pc_entry], prp2[pc_entry])
│   │    → NVMe 커맨드의 PRP1 필드에 이 DMA 주소 설정
│   │    → SSD가 이 주소로 데이터를 DMA Write (Read 명령 시)
│   │    → SSD가 이 주소에서 데이터를 DMA Read (Write 명령 시)
│   │
│   └── 연결: prp1[i] ←═ pages_dma.ioaddrs[i]
│            prp1[i] → nvm_cmd_t.dword[6-7] (PRP1)
│            SSD DMA 대상 주소 = base_addr + i*page_size (같은 물리 메모리)
│
├── prp2: uint64_t* ════► prp1과 동일 구조                (line 512)
│   └── 멀티 페이지 전송 시 두 번째 PRP (보통 단일 페이지면 0)
│
├── ctrl_page_size: uint64_t ═► nvm_ctrl_t.page_size      (line 514)
├── range_cap:      uint64_t = 64 (max ranges)            (line 515)
│
├── ranges: pages_t* ──────────────────────────────── (line 517)
│   │
│   │  ┌── pages_t = data_page_t*
│   │  └──► GPU VRAM: range별 data_page_t 배열 포인터 [n_ranges]
│   │       ┌────────────────────────────────────────────┐
│   │       │ ranges[0] ──► data_page_t[] (range 0의 페이지 상태) │
│   │       │ ranges[1] ──► data_page_t[] (range 1의 페이지 상태) │
│   │       │ ...                                        │
│   │       └────────────────────────────────────────────┘
│   │
│   │  data_page_t 내부:
│   │    state: atomic<uint32_t>
│   │      bit 31 (0x80000000): VALID  ← 데이터가 최신
│   │      bit 30 (0x40000000): BUSY   ← I/O 진행 중
│   │      bit 29 (0x20000000): DIRTY  ← 수정됨 (writeback 필요)
│   │      bits 0-28: 참조 카운트       ← 현재 읽는 스레드 수
│   │    offset: uint32_t
│   │      └── 이 data_page가 매핑된 캐시 슬롯 번호
│   │          = cache_pages[offset] 와 대응
│   │
│   └── 연결: ranges[range_id] → data_page_t[page_in_range]
│            data_page_t.offset → cache_pages[offset]
│            → base_addr + offset * page_size = 실제 데이터
│
├── h_ranges: pages_t* ═► ranges의 Host 측 복사본    (line 518)
│
├── n_ranges:      uint64_t                           (line 519)
├── n_ranges_bits: uint64_t = log2(n_ranges)          (line 520)
├── n_ranges_mask: uint64_t = n_ranges - 1            (line 521)
│
├── n_cachelines_for_states: uint64_t                 (line 522)
│   └── = n_pages / STATES_PER_CACHELINE
│
├── ranges_page_starts: uint64_t* ─────────────────── (line 524)
│   │  └──► [n_ranges] 각 range의 시작 LBA
│   │       STRIPE 모드에서:
│   │         backing_page = page_start + page_offset / n_ctrls
│   │         backing_ctrl = page_offset % n_ctrls
│   └── 연결: range의 page_start → 물리 SSD의 LBA 계산에 사용
│
├── ranges_dists: data_dist_t* ────────────────────── (line 525)
│   │  └──► [n_ranges] 각 range의 분산 모드
│   │       REPLICATE(0): 모든 SSD에 같은 데이터
│   │       STRIPE(1): 페이지를 SSD에 순환 분산
│   └── 연결: backing_ctrl 계산에 사용
│            STRIPE: ctrl = page_offset % n_ctrls
│
├── ctrl_counter: atomic<uint64_t>* ───────────────── (line 526)
│   │  └──► 단일 atomic 카운터 (GPU VRAM)
│   │  사용: GPU 커널에서
│   │    ctrl_idx = ctrl_counter->fetch_add(1) % n_ctrls
│   │    → 컨트롤러를 라운드로빈으로 선택
│   │    lane 0이 대표로 수행 후 __shfl_sync로 warp 전체에 broadcast
│   └── 연결: ctrl_counter → ctrl_idx → d_ctrls[ctrl_idx]
│
├── q_head:     atomic<uint64_t>* (GPU)               (line 528)
├── q_tail:     atomic<uint64_t>* (GPU)               (line 529)
├── q_lock:     atomic<uint64_t>* (GPU)               (line 530)
├── extra_reads: atomic<uint64_t>* (GPU)              (line 531)
│   └── 캐시 내부 큐 관리용 (eviction 등)
│
├── d_ctrls: Controller** ─────────────────────────── (line 533)
│   │
│   │  ┌── 값의 출처:
│   │  │   d_ctrls_buff = allocate(n_ctrls * sizeof(Controller*))
│   │  │   for i in 0..n_ctrls:
│   │  │     d_ctrls[i] = ctrls[i]->d_ctrl_ptr
│   │  │   즉, 각 Controller의 Device 복사본 포인터 배열
│   │  │
│   │  └──► GPU VRAM: Controller 포인터 배열 [n_ctrls]
│   │       ┌────────────────────────────────────────────────────┐
│   │       │ d_ctrls[0] ──► Controller copy 0 (GPU VRAM)       │
│   │       │                 ├── d_qps ──► QueuePair[0..127]   │
│   │       │                 ├── n_qps = 128                   │
│   │       │                 ├── blk_size, blk_size_log        │
│   │       │                 └── access_counter, queue_counter │
│   │       │                                                    │
│   │       │ d_ctrls[1] ──► Controller copy 1 (GPU VRAM)       │
│   │       │                 └── ...                           │
│   │       │ ...                                                │
│   │       └────────────────────────────────────────────────────┘
│   │
│   │  사용: GPU 커널에서
│   │    ctrl = d_ctrls[ctrl_idx]
│   │    qp = &ctrl->d_qps[queue_idx]
│   │    read_data(pc, qp, lba, n_blocks, pc_entry)
│   │
│   └── 연결: ctrl_counter → ctrl_idx → d_ctrls[ctrl_idx]
│            → Controller.d_qps[queue_idx] → QueuePair
│            → qp.sq (SQ에 커맨드 enqueue)
│            → qp.cq (CQ에서 완료 poll)
│
├── n_ctrls: uint64_t ═► ctrls.size()                (line 534)
├── prps:    bool                                     (line 535)
└── n_blocks_per_page: uint64_t                       (line 537)
    └── = page_size / block_size
```

---

## 5. range_d_t 필드별 연결 (page_cache.h:892-983)

```
range_d_t<T>  (Device측 range 디스크립터)
│
│  page_cache에 등록되어 ranges[] 배열에서 참조됨
│  array_d_t.d_ranges[]에서도 참조됨
│
├── index_start:      uint64_t                        (line 894)
│   └── 이 range가 담당하는 논리 요소의 시작 인덱스
│
├── count:            uint64_t                        (line 895)
│   └── 이 range의 총 요소 수
│
├── range_id:         uint64_t                        (line 896)
│   └── page_cache 내 고유 ID (ranges[] 인덱스)
│
├── page_start_offset: uint64_t                       (line 897)
├── page_size:        uint64_t                        (line 898)
├── page_start:       uint64_t                        (line 899)
│   └── 이 range의 시작 LBA (ranges_page_starts[range_id]와 동일)
│
├── page_count:       uint64_t                        (line 900)
├── n_elems_per_page: size_t                          (line 901)
│   └── = page_size / sizeof(T) (예: 4096/4 = 1024 float)
│
├── dist:             data_dist_t                     (line 902)
│   └── STRIPE(1) 또는 REPLICATE(0)
│
├── src:              uint8_t*                        (line 903)
│   └── = page_cache_d_t.base_addr (캐시 데이터 버퍼)
│
├── access_cnt:  atomic<uint64_t> (D)                 (line 905)
├── miss_cnt:    atomic<uint64_t> (D)                 (line 906)
├── hit_cnt:     atomic<uint64_t> (D)                 (line 907)
├── read_io_cnt: atomic<uint64_t> (D)                 (line 908)
│   └── 통계 카운터들 (print_reset_stats()에서 출력)
│
├── pages: pages_t ═► data_page_t* ──────────────── (line 911)
│   │  이 range의 페이지 상태 배열
│   │  = page_cache_d_t.ranges[range_id]와 동일
│   │
│   └── 연결: pages[page_in_range].state = VALID/BUSY/DIRTY + ref_cnt
│            pages[page_in_range].offset = cache_pages 슬롯 번호
│
├── cache: page_cache_d_t ═► 전체 복사 ─────────── (line 916)
│   │  ★ range_d_t가 page_cache_d_t 전체를 복사로 가지고 있다!
│   │  (포인터가 아니라 값 복사)
│   │
│   └── 연결: range 내에서 cache.base_addr, cache.cache_pages 등
│            직접 접근 가능 (간접 참조 불필요)
│
│  ═══════════ 주요 메서드와 필드 사용 ═══════════
│
├── get_backing_page(i): (line 920)
│   │  page_offset = page_start_offset + i
│   │  if (dist == STRIPE):
│   │    return page_start + page_offset / n_ctrls
│   │  else:
│   │    return page_start + page_offset
│   └── 반환값 → NVMe cmd의 Starting LBA로 사용
│
├── get_backing_ctrl(i): (line 923)
│   │  if (dist == STRIPE):
│   │    return page_offset % n_ctrls   ← SSD 번호
│   │  else:
│   │    return ALL_CTRLS (0xFFFF...)
│   └── 반환값 → d_ctrls[ctrl] 선택에 사용
│
├── get_page(i): (line 929)
│   │  return i / n_elems_per_page
│   └── 요소 인덱스 → 페이지 번호 변환
│
├── get_subindex(i): (line 932)
│   │  return i % n_elems_per_page
│   └── 페이지 내 오프셋
│
└── acquire_page(pg, count, write, ctrl, queue): (line 944)
    │  1. pages[pg].state를 atomic CAS로 BUSY 설정
    │  2. Cache Miss → find_slot()으로 캐시 슬롯 확보
    │  3. NVMe Read 커맨드 발행:
    │     lba = get_backing_page(pg)
    │     ctrl_idx = get_backing_ctrl(pg)
    │     qp = d_ctrls[ctrl_idx]->d_qps[queue]
    │     read_data(cache, qp, lba, n_blocks, slot)
    │  4. 완료 후 pages[pg].state = VALID + count
    │  5. 데이터 주소 반환: src + slot * page_size
    │
    └── 연결 체인:
         pg → get_backing_page → lba
         pg → get_backing_ctrl → d_ctrls[ctrl]
         d_ctrls[ctrl] → d_qps[queue] → QueuePair
         QueuePair.sq → sq_enqueue(cmd{lba, prp1[slot]})
         → *(sq.db) = tail → SSD Doorbell
         → SSD DMA Write → base_addr + slot * page_size
         → cq_poll → 완료
```

---

## 6. array_d_t 필드별 연결 (page_cache.h:1238-1725)

```
array_d_t<T>  (GPU에서 접근하는 가상 배열)
│
├── n_elems:      uint64_t                            (line 1240)
├── start_offset: uint64_t                            (line 1241)
├── n_ranges:     uint64_t                            (line 1242)
│
├── src:          uint8_t* ═► page_cache_d_t.base_addr (line 1243)
│
├── d_ranges: range_d_t<T>* ──────────────────────── (line 1245)
│   │  └──► GPU VRAM: range 디스크립터 배열 [n_ranges]
│   │       각 range_d_t가 page_cache_d_t 전체 복사를 가짐
│   │
│   └── 연결: array[i] 접근 시:
│            1. find_range(i) → 어느 range에 속하는지
│            2. range.get_page(i) → 페이지 번호
│            3. range.acquire_page(pg) → 캐시 히트/미스 처리
│            4. src + offset → 데이터 반환
│
│  ═══════════ 핵심 메서드: operator[] ═══════════
│
├── operator[](i): (line 1621-1635)
│   │
│   │  T operator[](size_t i) const {
│   │    r = find_range(i);
│   │    page = d_ranges[r].get_page(i);
│   │    gaddr = d_ranges[r].get_global_address(page);
│   │    subindex = d_ranges[r].get_subindex(i);
│   │
│   │    // 페이지 획득 (캐시 히트 또는 SSD에서 로드)
│   │    addr = acquire_page(i, page_, start, end, r);
│   │    T* typedAddr = (T*)addr;
│   │
│   │    val = typedAddr[subindex];  ← 실제 데이터 읽기
│   │
│   │    release_page(page_, r, i);
│   │    return val;
│   │  }
│   │
│   └── 전체 참조 체인:
│        array_d_t[i]
│        → find_range(i) → d_ranges[r]
│        → d_ranges[r].get_page(i) → page 번호
│        → d_ranges[r].acquire_page(page, ...)
│           → pages[page].state 확인
│              ├── VALID (Cache Hit):
│              │   pages[page].offset → cache slot
│              │   src + slot * page_size → 데이터 주소
│              │
│              └── INVALID (Cache Miss):
│                  → find_slot() → cache_pages[slot]
│                  → get_backing_page() → LBA
│                  → get_backing_ctrl() → SSD 번호
│                  → d_ctrls[ctrl]→d_qps[q] → QueuePair
│                  → read_data(pc, qp, lba, n_blocks, slot)
│                     → nvm_cmd_t { opcode=READ,
│                                   prp1=prp1[slot],
│                                   slba=lba,
│                                   nlb=n_blocks }
│                     → sq_enqueue(sq, cmd)
│                        → sq.vaddr[pos] = cmd
│                        → *(sq.db) = tail
│                     → [SSD 처리]
│                        → SSD DMA Read: sq.ioaddr에서 cmd 가져감
│                        → SSD Flash Read
│                        → SSD DMA Write: prp1[slot]로 데이터 전송
│                           = base_addr + slot * page_size
│                        → SSD DMA Write: cq.ioaddr에 완료 기록
│                     → cq_poll(cq, cid)
│                        → cq.vaddr[loc].dword[3]에서 CID+phase 확인
│                     → cq_dequeue(cq)
│                        → *(cq.db) = head
│                  → pages[page].state = VALID
│                  → src + slot * page_size → 데이터 주소
│        → typedAddr[subindex] → 값 반환
```

---

## 7. 전체 구조의 한 눈 요약

```
  ┌─────────────────────────────────────────────────────────────────┐
  │                    참조 방향 요약                                │
  │                                                                 │
  │  커널 인자:  d_ctrls (Controller**),  d_pc (page_cache_d_t*)   │
  │               │                         │                      │
  │               │                         ├── base_addr ──► 캐시 데이터
  │               │                         ├── cache_pages ──► 메타데이터
  │               │                         ├── prp1[] ──► DMA 주소 테이블
  │               │                         ├── ranges[] ──► data_page_t[]
  │               │                         └── d_ctrls ──┐      │
  │               │                                       │      │
  │               └─────────────────── = ──────────────────┘      │
  │               │                                               │
  │               ▼                                               │
  │         Controller (Device)                                    │
  │               │                                               │
  │               ├── d_qps[] ──► QueuePair[]                     │
  │               │                    │                           │
  │               │                    ├── sq: nvm_queue_t         │
  │               │                    │   ├── vaddr ──► SQ 데이터 (GPU, DMA)
  │               │                    │   ├── db ──► BAR0 Doorbell│
  │               │                    │   ├── tickets[] ──► 동기화│
  │               │                    │   ├── tail_mark[] ──► 동기화
  │               │                    │   └── head_mark[] ──► 동기화
  │               │                    │                           │
  │               │                    └── cq: nvm_queue_t         │
  │               │                        ├── vaddr ──► CQ 데이터 (GPU, DMA)
  │               │                        ├── db ──► BAR0 Doorbell│
  │               │                        └── pos_locks[] ──► 동기화
  │               │                                               │
  │               └── access_counter, queue_counter (통계)         │
  │                                                                │
  │                                                                │
  │  데이터 흐름:                                                   │
  │                                                                │
  │  GPU thread → array_d_t[i]                                     │
  │    → range_d_t.acquire_page()                                  │
  │      → [Miss] read_data(pc, qp, lba, slot)                    │
  │        → cmd.PRP1 = prp1[slot]    ← base_addr의 DMA 주소      │
  │        → sq_enqueue → *(sq.db)    ← BAR0 Doorbell              │
  │        → SSD DMA Write → base_addr + slot*page_size            │
  │        → cq_poll → *(cq.db)      ← BAR0 Doorbell              │
  │      → [Hit] base_addr + slot*page_size                        │
  │    → data[subindex]                                            │
  │                                                                │
  └─────────────────────────────────────────────────────────────────┘
```
