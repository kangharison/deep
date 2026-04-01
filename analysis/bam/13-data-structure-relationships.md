# BaM Block Benchmark 자료구조 연결 관계도

## 1. 전체 자료구조 계층 (소유 관계)

```
Settings (stack, Host)
│ 커맨드라인 파라미터
│ cudaDevice, numPages, pageSize, queueDepth, numQueues, ...
│
│
│  main.cu에서 생성 순서: ① → ② → ③ → ④ 커널 런치
│
│
├──① Controller** ctrls[n_ctrls]          ← std::vector (Host)
│   │
│   └── Controller (Host + Device copy)
│       │
│       ├── nvm_ctrl_t* ctrl ─────────────────────────── (Host, mmap)
│       │   │ BAR0 메모리 매핑 핸들
│       │   │
│       │   ├── mm_ptr: volatile void*     ← NVMe BAR0 (PCIe MMIO)
│       │   ├── mm_size: size_t
│       │   ├── page_size: size_t          ← MPS (Memory Page Size)
│       │   ├── dstrd: uint8_t             ← Doorbell Stride
│       │   ├── timeout: uint64_t
│       │   └── max_qs: uint32_t           ← MQES
│       │
│       ├── nvm_aq_ref aq_ref ────────────────────────── (Host, opaque)
│       │   │ Admin Queue RPC 핸들
│       │   └── → struct local_admin { acq, asq, timeout }
│       │
│       ├── DmaPtr aq_mem ────────────────────────────── (Host DMA)
│       │   │ Admin Queue 메모리 (3 pages)
│       │   └── nvm_dma_t { vaddr, ioaddrs[], page_size, n_ioaddrs }
│       │
│       ├── nvm_ctrl_info info ───────────────────────── (Host)
│       │   │ Identify Controller 결과
│       │   └── model_no, serial_no, fw_rev, max_transfer_size, ...
│       │
│       ├── nvm_ns_info ns ───────────────────────────── (Host)
│       │   │ Identify Namespace 결과
│       │   └── ns_id, size, lba_data_size, metadata_size
│       │
│       ├── n_qps: uint16_t
│       ├── blk_size, blk_size_log: uint32_t
│       │
│       ├── access_counter ─── simt::atomic<uint64_t>  (Device)
│       ├── queue_counter ──── simt::atomic<uint64_t>  (Device)
│       │
│       ├──② QueuePair** h_qps[n_qps] ──────────────── (Host array)
│       │   │
│       │   └── QueuePair (Host 생성 → Device 복사)
│       │       │
│       │       ├── pageSize: uint32_t
│       │       ├── block_size: uint32_t
│       │       ├── block_size_log: uint32_t
│       │       ├── block_size_minus_1: uint32_t
│       │       ├── nvmNamespace: uint32_t
│       │       ├── qp_id: uint16_t
│       │       │
│       │       ├── sq: nvm_queue_t ─────────── (중요! 아래 상세)
│       │       ├── cq: nvm_queue_t ─────────── (중요! 아래 상세)
│       │       │
│       │       ├── sq_mem: DmaPtr ──────────── GPU VRAM (SQ 데이터)
│       │       │   └── nvm_dma_t
│       │       │       ├── vaddr → GPU 메모리 (nvm_cmd_t 배열)
│       │       │       └── ioaddrs[] → DMA 주소 (SSD가 접근)
│       │       │
│       │       ├── cq_mem: DmaPtr ──────────── GPU VRAM (CQ 데이터)
│       │       │   └── nvm_dma_t
│       │       │       ├── vaddr → GPU 메모리 (nvm_cpl_t 배열)
│       │       │       └── ioaddrs[] → DMA 주소
│       │       │
│       │       ├── sq_tickets: BufferPtr ───── GPU VRAM
│       │       │   └── padded_struct[qs]       슬롯 라운드로빈
│       │       │
│       │       ├── sq_tail_mark: BufferPtr ─── GPU VRAM
│       │       │   └── padded_struct[qs]       커맨드 준비 표시
│       │       │
│       │       ├── sq_cid: BufferPtr ────────── GPU VRAM
│       │       │   └── padded_struct[65536]    CID 할당 추적
│       │       │
│       │       ├── cq_head_mark: BufferPtr ─── GPU VRAM
│       │       │   └── padded_struct[qs]       완료 위치 표시
│       │       │
│       │       └── cq_pos_locks: BufferPtr ─── GPU VRAM
│       │           └── padded_struct[qs]       CQ 위치 잠금
│       │
│       ├── QueuePair* d_qps ────────────────── Device 메모리
│       │   └── h_qps[]의 GPU 복사본 (cudaMemcpy)
│       │
│       └── d_ctrl_ptr: void* ───────────────── Device 메모리
│           └── Controller 자체의 GPU 복사본
│
│
├──③ page_cache_t h_pc (Host)
│   │
│   ├── page_cache_d_t pdt ──────────────────── (Host 구조체, Device에 복사)
│   │   │
│   │   │  ★ GPU 커널이 실제로 접근하는 핵심 디스크립터
│   │   │
│   │   ├── base_addr: uint8_t* ─────────── → pages_dma.vaddr (GPU VRAM)
│   │   │   캐시 데이터 시작 주소
│   │   │
│   │   ├── page_size: uint64_t
│   │   ├── page_size_minus_1: uint64_t      비트 마스크
│   │   ├── page_size_log: uint64_t          시프트 연산용
│   │   ├── n_pages: uint64_t
│   │   ├── n_pages_minus_1: uint64_t
│   │   │
│   │   ├── cache_pages: cache_page_t* ──── → cache_pages_buf (GPU VRAM)
│   │   │   │ 페이지 메타데이터 배열 [n_pages]
│   │   │   │
│   │   │   └── cache_page_t (32B 정렬)
│   │   │       ├── page_take_lock: atomic<uint32_t>
│   │   │       └── page_translation: uint32_t
│   │   │
│   │   ├── page_ticket: padded_struct_pc* ── → page_ticket_buf (GPU)
│   │   │   티켓 기반 페이지 할당 카운터
│   │   │
│   │   ├── prp1: uint64_t* ─────────────── → prp1_buf (GPU VRAM)
│   │   │   각 캐시 페이지의 PRP1 DMA 주소 [n_pages]
│   │   │
│   │   ├── prp2: uint64_t* ─────────────── → prp2_buf (GPU VRAM)
│   │   │   PRP2 DMA 주소 (멀티페이지 시) [n_pages]
│   │   │
│   │   ├── ranges: pages_t* ────────────── → ranges_buf (GPU VRAM)
│   │   │   │ range별 페이지 배열 포인터 [n_ranges]
│   │   │   └── → data_page_t[] 배열들
│   │   │
│   │   ├── ranges_page_starts: uint64_t* ── → ranges_page_starts_buf
│   │   │   각 range의 시작 LBA [n_ranges]
│   │   │
│   │   ├── ranges_dists: data_dist_t* ───── → ranges_dists_buf
│   │   │   각 range의 분산 모드 [n_ranges] (REPLICATE/STRIPE)
│   │   │
│   │   ├── n_ranges, n_ranges_bits, n_ranges_mask
│   │   ├── ctrl_page_size, n_blocks_per_page
│   │   ├── range_cap, prps: bool
│   │   │
│   │   ├── ctrl_counter: atomic<uint64_t>* ── → ctrl_counter_buf (GPU)
│   │   │   컨트롤러 라운드로빈 선택 카운터
│   │   │
│   │   ├── q_head: atomic<uint64_t>* ────── → q_head_buf (GPU)
│   │   ├── q_tail: atomic<uint64_t>* ────── → q_tail_buf (GPU)
│   │   ├── q_lock: atomic<uint64_t>* ────── → q_lock_buf (GPU)
│   │   ├── extra_reads: atomic<uint64_t>* ── → extra_reads_buf (GPU)
│   │   │
│   │   ├── d_ctrls: Controller** ────────── → d_ctrls_buff (GPU)
│   │   │   └── [ctrl[0]->d_ctrl_ptr, ctrl[1]->d_ctrl_ptr, ...]
│   │   │       디바이스 컨트롤러 포인터 배열
│   │   │
│   │   └── n_ctrls: uint64_t
│   │
│   ├── d_pc_ptr: page_cache_d_t* ──────── → pc_buff (GPU VRAM)
│   │   pdt의 Device 복사본 포인터
│   │
│   ├── pages_dma: DmaPtr ──────────────── GPU VRAM (DMA 매핑됨)
│   │   │ 실제 캐시 데이터 버퍼
│   │   └── 크기: n_pages × page_size (예: 1024 × 4096 = 4MB)
│   │       SSD가 P2P DMA로 여기에 데이터를 쓴다
│   │
│   └── (기타 BufferPtr들: 위 pdt 필드의 GPU 버퍼)
│
│
│  ★ 아래는 page_cache_t 생성 시 내부적으로 만들어지는 구조:
│
├── range_t<T> (page_cache에 등록)
│   │
│   ├── range_d_t<T> (Device side)
│   │   ├── pages: data_page_t* ──── GPU VRAM
│   │   │   │ 페이지 상태 배열
│   │   │   └── data_page_t (32B 정렬)
│   │   │       ├── state: atomic<uint32_t>
│   │   │       │   bit 31: VALID  (0x80000000)
│   │   │       │   bit 30: BUSY   (0x40000000)
│   │   │       │   bit 29: DIRTY  (0x20000000)
│   │   │       │   bits 0-28: 참조 카운트
│   │   │       └── offset: uint32_t
│   │   │
│   │   ├── page_start: uint64_t
│   │   ├── dist: data_dist_t (STRIPE/REPLICATE)
│   │   └── range_id: uint64_t
│   │
│   └── d_range_ptr: void* → Device 복사본
│
│
└── array_t<T> (range 위에 구축)
    │
    ├── array_d_t<T> (Device side)
    │   └── → range_d_t 참조
    │
    └── d_array_ptr: void* → Device 복사본
```

---

## 2. nvm_queue_t 내부 구조 상세 (핵심)

```
nvm_queue_t (약 512 bytes, 캐시라인 정렬)
│
│  모든 필드가 32바이트 패딩으로 분리되어 있다
│  → false sharing 방지 (GPU warp 간 캐시라인 충돌 제거)
│
├── [Cacheline 0] head_lock: atomic<uint32_t> + pad[28]
│   GPU 스레드가 head를 이동할 때 획득하는 락
│
├── [Cacheline 1] tail_lock: atomic<uint32_t> + pad[28]
│   GPU 스레드가 tail을 이동할 때 획득하는 락
│
├── [Cacheline 2] head: atomic<uint32_t> + pad[28]
│   큐의 head 위치 (device scope)
│
├── [Cacheline 3] tail: atomic<uint32_t> + pad[28]
│   큐의 tail 위치 (device scope)
│
├── [Cacheline 4] tail_copy: atomic<uint32_t> + pad[28]
│   tail의 복사본 (system scope — CPU에서도 볼 수 있음)
│
├── [Cacheline 5] head_copy: atomic<uint32_t> + pad[28]
│   head의 복사본 (system scope)
│
├── [Cacheline 6] in_ticket: atomic<uint32_t> + pad[28]
│   sq_enqueue 진입 티켓 (각 스레드가 atomic으로 획득)
│
├── [Cacheline 7] cid_ticket: atomic<uint32_t> + pad[28]
│   CID(Command ID) 할당 티켓
│
├── [비패딩 필드들]
│   ├── no: uint16_t              큐 번호
│   ├── qs: uint32_t              큐 크기 (엔트리 수)
│   ├── qs_minus_1: uint32_t      비트 마스크
│   ├── qs_log2: uint32_t         시프트 연산용
│   ├── es: uint32_t              엔트리 크기 (SQ:64B, CQ:16B)
│   ├── phase: uint8_t            현재 phase bit
│   ├── last: uint16_t            마지막 처리 위치
│   ├── local: uint8_t            로컬 메모리 여부
│   │
│   ├── vaddr: volatile void* ──── → sq_mem 또는 cq_mem의 GPU VRAM 주소
│   │   GPU 스레드가 커맨드/완료 엔트리를 읽고 쓰는 주소
│   │
│   ├── ioaddr: uint64_t ──── SSD가 보는 DMA 주소
│   │   SSD 컨트롤러가 이 주소로 DMA 전송
│   │
│   └── db: volatile uint32_t* ──── → BAR0 Doorbell 레지스터
│       cudaHostGetDevicePointer로 획득한 GPU 포인터
│       *(sq.db) = tail  → SSD에 "새 커맨드" 알림
│       *(cq.db) = head  → SSD에 "처리 완료" 알림
│
│
│  동기화 배열들 (QueuePair에서 할당, sq/cq에 연결):
│
├── tickets: padded_struct* ──── → sq_tickets (GPU VRAM)
│   │ [qs] 엔트리, 각 32바이트
│   │ tickets[pos].val == round_id 일 때 해당 슬롯 사용 가능
│   └── 역할: 같은 슬롯의 다른 라운드 스레드가 겹치지 않게 보장
│
├── tail_mark: padded_struct* ── → sq_tail_mark (GPU VRAM)
│   │ [qs] 엔트리
│   │ tail_mark[pos] = LOCKED  → "이 슬롯에 커맨드 복사 완료"
│   └── 역할: move_tail()이 연속 완료 슬롯만큼 tail 전진
│
├── head_mark: padded_struct* ── → cq_head_mark (GPU VRAM)
│   │ [qs] 엔트리
│   │ head_mark[pos] = LOCKED  → "이 슬롯의 완료 처리됨"
│   │ head_mark[pos] = UNLOCKED → "슬롯 재사용 가능"
│   └── 역할: cq_dequeue가 SQ 슬롯 해제 시 사용
│
├── cid: padded_struct* ──────── → sq_cid (GPU VRAM)
│   │ [65536] 엔트리
│   └── 역할: Command ID 할당/해제 추적
│
└── pos_locks: padded_struct* ── → cq_pos_locks (GPU VRAM)
    │ [qs] 엔트리
    └── 역할: CQ dequeue 시 위치별 상호 배제
```

---

## 3. 포인터 연결 관계도 (화살표 = "참조한다")

```
┌──────────────────────────────────────────────────────────────────┐
│                          HOST MEMORY                              │
│                                                                  │
│  Settings ──(파라미터)──► Controller[] ──(초기화)──► page_cache_t │
│                               │                         │        │
│                               │                         │        │
│              nvm_ctrl_t ◄─────┤                         │        │
│              (BAR0 mmap)      │                         │        │
│                               │                         │        │
│              nvm_aq_ref ◄─────┤                         │        │
│                               │                         │        │
│              DmaPtr aq_mem ◄──┤                         │        │
│                               │                         │        │
│              QueuePair[] ◄────┤                         │        │
│              (h_qps)          │                         │        │
│                               │                         │        │
└───────────────────────────────┼─────────────────────────┼────────┘
                                │ cudaMemcpy              │ cudaMemcpy
════════════════════════════════╪═════════════════════════╪═════════
                                ▼                         ▼
┌──────────────────────────────────────────────────────────────────┐
│                        GPU DEVICE MEMORY                          │
│                                                                  │
│  ┌─ Controller copy (d_ctrl_ptr) ──────────────────────────────┐ │
│  │  access_counter: atomic                                     │ │
│  │  queue_counter: atomic                                      │ │
│  │  n_qps, blk_size, blk_size_log                              │ │
│  │  │                                                          │ │
│  │  └── d_qps: QueuePair* ──────────────────────────────────┐  │ │
│  │       │                                                   │  │ │
│  │       ├── QueuePair[0]                                    │  │ │
│  │       │   ├── sq: nvm_queue_t ──┐                         │  │ │
│  │       │   └── cq: nvm_queue_t ──┤                         │  │ │
│  │       ├── QueuePair[1]          │                         │  │ │
│  │       │   ├── sq ───────────────┤                         │  │ │
│  │       │   └── cq ───────────────┤                         │  │ │
│  │       └── ...                   │                         │  │ │
│  │                                 │                         │  │ │
│  └─────────────────────────────────┼─────────────────────────┘  │ │
│                                    │                             │ │
│                                    ▼                             │ │
│  ┌─ SQ/CQ 관련 메모리 ────────────────────────────────────────┐ │ │
│  │                                                             │ │ │
│  │  sq.vaddr ──► ┌─────────────────────────────┐  (DMA 매핑)  │ │ │
│  │               │ nvm_cmd_t[0] (64B)          │◄── SSD가     │ │ │
│  │               │ nvm_cmd_t[1] (64B)          │    DMA로     │ │ │
│  │               │ ...                         │    읽어감    │ │ │
│  │               │ nvm_cmd_t[qs-1]             │              │ │ │
│  │               └─────────────────────────────┘              │ │ │
│  │                                                             │ │ │
│  │  cq.vaddr ──► ┌─────────────────────────────┐  (DMA 매핑)  │ │ │
│  │               │ nvm_cpl_t[0] (16B)          │◄── SSD가     │ │ │
│  │               │ nvm_cpl_t[1] (16B)          │    DMA로     │ │ │
│  │               │ ...                         │    써넣음    │ │ │
│  │               │ nvm_cpl_t[qs-1]             │              │ │ │
│  │               └─────────────────────────────┘              │ │ │
│  │                                                             │ │ │
│  │  sq.db ───────► BAR0::SQ Tail Doorbell (PCIe MMIO) ────────┼─┤ │
│  │  cq.db ───────► BAR0::CQ Head Doorbell (PCIe MMIO) ────────┼─┤ │
│  │                                                             │ │ │
│  │  ┌─ 동기화 배열들 (모두 padded_struct[], 32B 정렬) ───────┐ │ │ │
│  │  │                                                        │ │ │ │
│  │  │  sq.tickets[qs] ◄── sq_tickets BufferPtr               │ │ │ │
│  │  │  sq.tail_mark[qs] ◄── sq_tail_mark BufferPtr           │ │ │ │
│  │  │  sq.cid[65536] ◄── sq_cid BufferPtr                    │ │ │ │
│  │  │  cq.head_mark[qs] ◄── cq_head_mark BufferPtr           │ │ │ │
│  │  │  cq.pos_locks[qs] ◄── cq_pos_locks BufferPtr           │ │ │ │
│  │  │                                                        │ │ │ │
│  │  └────────────────────────────────────────────────────────┘ │ │ │
│  └─────────────────────────────────────────────────────────────┘ │ │
│                                                                  │ │
│                                                                  │ │
│  ┌─ page_cache_d_t (d_pc_ptr) ─────────────────────────────────┐│ │
│  │                                                              ││ │
│  │  base_addr ───► ┌─────────────────────────┐  pages_dma      ││ │
│  │                 │ 캐시 페이지 데이터 버퍼  │  (GPU VRAM,     ││ │
│  │                 │ Page[0]: 4096B          │   DMA 매핑)     ││ │
│  │                 │ Page[1]: 4096B          │                 ││ │
│  │                 │ ...                     │  ◄── SSD가       ││ │
│  │                 │ Page[n_pages-1]         │     P2P DMA로   ││ │
│  │                 └─────────────────────────┘     데이터 기록  ││ │
│  │                                                              ││ │
│  │  cache_pages ──► ┌───────────────────────┐  cache_pages_buf  ││ │
│  │                  │ cache_page_t[0] (32B) │                  ││ │
│  │                  │  page_take_lock       │                  ││ │
│  │                  │  page_translation     │                  ││ │
│  │                  │ cache_page_t[1]       │                  ││ │
│  │                  │ ...                   │                  ││ │
│  │                  └───────────────────────┘                  ││ │
│  │                                                              ││ │
│  │  prp1[n_pages] ──► 각 페이지의 PRP1 DMA 주소               ││ │
│  │  prp2[n_pages] ──► 각 페이지의 PRP2 DMA 주소               ││ │
│  │                    NVMe 커맨드에 이 주소가 들어감             ││ │
│  │                                                              ││ │
│  │  ranges[] ──────► ┌──────────────────────┐  range별          ││ │
│  │                   │ range[0].pages ──►   │  data_page_t[]   ││ │
│  │                   │   data_page_t[0]    │                  ││ │
│  │                   │     state: atomic   │                  ││ │
│  │                   │     offset          │                  ││ │
│  │                   │   data_page_t[1]    │                  ││ │
│  │                   │   ...               │                  ││ │
│  │                   └──────────────────────┘                  ││ │
│  │                                                              ││ │
│  │  ranges_page_starts[] ──► 각 range의 시작 LBA               ││ │
│  │  ranges_dists[] ────────► STRIPE / REPLICATE                ││ │
│  │                                                              ││ │
│  │  d_ctrls ───────► ┌────────────────────┐                    ││ │
│  │                   │ Controller*[0] ────┼──► Controller copy │◄┘ │
│  │                   │ Controller*[1] ────┼──► Controller copy │   │
│  │                   │ ...               │                    │   │
│  │                   └────────────────────┘                    │   │
│  │                                                              │   │
│  │  ctrl_counter: atomic ← 컨트롤러 라운드로빈 선택            │   │
│  │  page_ticket: atomic  ← 캐시 페이지 할당 티켓               │   │
│  │  q_head, q_tail, q_lock: atomics ← 캐시 큐 관리             │   │
│  │  extra_reads: atomic ← 추가 읽기 카운터                     │   │
│  │                                                              │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘


                    PCIe Bus
═══════════════════════╪════════════════════════════
                       │
┌──────────────────────▼──────────────────────────────────────────────┐
│                      NVMe SSD                                       │
│                                                                     │
│  SQ Tail Doorbell ◄── sq.db 쓰기 (GPU)                             │
│  CQ Head Doorbell ◄── cq.db 쓰기 (GPU)                             │
│                                                                     │
│  SSD 컨트롤러:                                                      │
│  1. sq.vaddr의 DMA 주소에서 nvm_cmd_t 읽기 (P2P DMA Read)          │
│  2. cmd.PRP1 주소 = prp1[page_idx] = 캐시 페이지 DMA 주소          │
│  3. Flash에서 데이터 읽기                                           │
│  4. PRP1 주소로 데이터 쓰기 (P2P DMA Write → GPU pages_dma)        │
│  5. cq.vaddr의 DMA 주소에 완료 엔트리 쓰기 (P2P DMA Write)        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 4. 런타임 참조 체인 (Read I/O 1회)

```
GPU 커널 시작
│
├── page_cache_d_t* pc  (커널 인자)
│   │
│   ├──[1] 컨트롤러 선택
│   │   ctrl_idx = pc->ctrl_counter->fetch_add(1) % pc->n_ctrls
│   │   Controller* ctrl = pc->d_ctrls[ctrl_idx]
│   │                       ~~~~~~~~~~~~~~~~
│   │                       page_cache_d_t.d_ctrls → Controller copy
│   │
│   ├──[2] 큐 선택
│   │   queue_idx = smid % ctrl->n_qps
│   │   QueuePair* qp = &ctrl->d_qps[queue_idx]
│   │                     ~~~~~~~~~~~~~~~~~
│   │                     Controller.d_qps → QueuePair[]
│   │
│   ├──[3] PRP 주소 획득
│   │   prp1 = pc->prp1[page_idx]
│   │          ~~~~~~~~~~~~
│   │          page_cache_d_t.prp1 → DMA 주소 배열
│   │   → NVMe cmd의 PRP1에 이 주소 설정
│   │   → SSD가 이 주소에 데이터를 DMA Write
│   │
│   ├──[4] NVMe 커맨드 구성 (nvm_cmd_t, 64B)
│   │   cmd.opcode = NVM_IO_READ
│   │   cmd.nsid = qp->nvmNamespace
│   │   cmd.prp1 = prp1
│   │   cmd.slba = 계산된 LBA
│   │   cmd.nlb = n_blocks
│   │
│   ├──[5] SQ Enqueue
│   │   ticket = qp->sq.in_ticket.fetch_add(1)
│   │                   ~~~~~~~~~~~~
│   │                   nvm_queue_t.in_ticket (atomic)
│   │
│   │   pos = ticket & qp->sq.qs_minus_1
│   │
│   │   while (qp->sq.tickets[pos].val != round_id)
│   │          ~~~~~~~~~~~~~~~~~~
│   │          nvm_queue_t → padded_struct[] (GPU VRAM)
│   │     __nanosleep()
│   │
│   │   memcpy(qp->sq.vaddr + pos*64, &cmd, 64)
│   │          ~~~~~~~~~~~~
│   │          nvm_queue_t.vaddr → sq_mem (GPU VRAM, DMA 매핑)
│   │
│   │   qp->sq.tail_mark[pos].val = LOCKED
│   │
│   │   *(qp->sq.db) = new_tail
│   │     ~~~~~~~~~~
│   │     nvm_queue_t.db → BAR0 Doorbell (PCIe MMIO)
│   │
│   ├──[6] CQ Poll
│   │   cpl = qp->cq.vaddr[loc]
│   │         ~~~~~~~~~~~~
│   │         nvm_queue_t.vaddr → cq_mem (GPU VRAM, DMA 매핑)
│   │         SSD가 여기에 완료 엔트리를 P2P DMA Write
│   │
│   │   phase 확인: cpl.dword[3] & 0x10000
│   │   cid 확인:   cpl.dword[3] & 0xFFFF
│   │
│   ├──[7] CQ Dequeue
│   │   *(qp->cq.db) = new_head
│   │     ~~~~~~~~~~
│   │     nvm_queue_t.db → BAR0 CQ Doorbell
│   │
│   │   qp->sq.head_mark[pos].val = UNLOCKED
│   │   → SQ 슬롯 재사용 허용
│   │
│   └──[8] 데이터 접근
│       data = pc->base_addr + page_idx * pc->page_size
│              ~~~~~~~~~~~~~~
│              page_cache_d_t.base_addr → pages_dma (GPU VRAM)
│              SSD가 P2P DMA로 여기에 데이터를 써놓았음
```

---

## 5. 자료구조 크기 요약

```
┌──────────────────────┬──────────┬─────────────────────────────────┐
│ 구조체                │ 크기      │ 인스턴스 수 (예: 4SSD, 128QP)   │
├──────────────────────┼──────────┼─────────────────────────────────┤
│ Settings             │ ~100B    │ 1                               │
│ Controller           │ ~1KB     │ 4 (Host) + 4 (Device copy)      │
│ nvm_ctrl_t           │ ~48B     │ 4 (Host only)                   │
│ QueuePair            │ ~1.2KB   │ 4×128 = 512 (Host+Device)       │
│ nvm_queue_t          │ ~512B    │ 512×2 = 1024 (SQ+CQ)           │
│ padded_struct        │ 32B      │ qs×5 배열 × 512 QP = 매우 많음  │
│ page_cache_t         │ ~300B    │ 1 (Host)                        │
│ page_cache_d_t       │ ~256B    │ 1 (Device)                      │
│ cache_page_t         │ 32B      │ n_pages (예: 1024)               │
│ data_page_t          │ 32B      │ range당 가변                     │
│ nvm_cmd_t            │ 64B      │ SQ 엔트리 수 (큐 크기)          │
│ nvm_cpl_t            │ 16B      │ CQ 엔트리 수 (큐 크기)          │
│ pages_dma 데이터     │ 가변      │ 1 (n_pages × page_size)         │
├──────────────────────┼──────────┼─────────────────────────────────┤
│ GPU VRAM 총 사용량 (예시):                                        │
│ 캐시 데이터: 1024 × 4096 = 4 MB                                  │
│ SQ/CQ × 512 QP: ~수십 MB                                         │
│ 동기화 배열: ~수 MB                                               │
│ 메타데이터: ~수 KB                                                │
└───────────────────────────────────────────────────────────────────┘
```

---

## 6. 타입 별칭 정리

```
┌──────────────────┬──────────────────────────────────────────────┐
│ 별칭              │ 실제 타입                                     │
├──────────────────┼──────────────────────────────────────────────┤
│ DmaPtr           │ std::shared_ptr<nvm_dma_t>                   │
│ BufferPtr        │ std::shared_ptr<void> (GPU cudaMalloc 래핑)  │
│ nvm_aq_ref       │ struct nvm_admin_reference*                   │
│ nvm_cmd_t        │ struct { uint32_t dword[16] } (64B)          │
│ nvm_cpl_t        │ struct { uint32_t dword[4] } (16B)           │
│ padded_struct    │ __align__(32) { atomic<uint32_t> val }       │
│ cache_page_t     │ __align__(32) { atomic<uint32_t>, uint32_t } │
│ data_page_t      │ __align__(32) { atomic<uint32_t>, uint32_t } │
│ pages_t          │ data_page_t* (range별 페이지 배열 포인터)     │
│ data_dist_t      │ enum { REPLICATE=0, STRIPE=1 }               │
└──────────────────┴──────────────────────────────────────────────┘
```
