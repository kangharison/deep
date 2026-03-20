# Linux Kernel NVMe vs BaM GPU-Initiated: SQ 매핑 비교

## 1. 전체 비교 도식

```
┌═══════════════════════════════════════════════════════════════════════════════════════════════════════════┐
║                                                                                                         ║
║          Linux Kernel NVMe Driver                        BaM GPU-Initiated I/O                          ║
║          (CPU 기반, blk-mq)                              (GPU 기반, lock-free)                           ║
║                                                                                                         ║
║   ┌─────────────────────────────────┐          ┌──────────────────────────────────────┐                 ║
║   │         Application             │          │          CUDA Kernel                  │                 ║
║   │      (read/write syscall)       │          │    (수만~수십만 GPU 스레드)            │                 ║
║   └──────────────┬──────────────────┘          └──────────────────┬───────────────────┘                 ║
║                  │                                                │                                     ║
║                  ▼                                                ▼                                     ║
║   ┌─────────────────────────────────┐          ┌──────────────────────────────────────┐                 ║
║   │    VFS → 파일시스템 → Block      │          │         (커널 스택 전체 우회)          │                 ║
║   │    Layer (blk-mq)               │          │     GPU 스레드가 직접 NVMe 큐 조작    │                 ║
║   └──────────────┬──────────────────┘          └──────────────────┬───────────────────┘                 ║
║                  │                                                │                                     ║
║                  ▼                                                ▼                                     ║
║                                                                                                         ║
║   ┌─ CPU 코어 → SQ 정적 매핑 ──────┐          ┌─ Warp → QP 동적 매핑 ───────────────┐                 ║
║   │                                 │          │                                      │                 ║
║   │  Core 0 ──→ SQ 0 (전용)        │          │  Warp A (SM3)  ──┐                   │                 ║
║   │  Core 1 ──→ SQ 1 (전용)        │          │  Warp B (SM100)──┤ fetch_add(1)%N    │                 ║
║   │  Core 2 ──→ SQ 2 (전용)        │          │  Warp C (SM3)  ──┤ → 동적 라운드로빈 │                 ║
║   │  Core 3 ──→ SQ 3 (전용)        │          │  Warp D (SM50) ──┤                   │                 ║
║   │  ...                            │          │  ...            ──┘                   │                 ║
║   │  Core N ──→ SQ N (전용)        │          │       │                               │                 ║
║   │                                 │          │       ▼                               │                 ║
║   │  ★ 1:1 고정 바인딩             │          │  QP[0], QP[1], ... QP[N-1]           │                 ║
║   │  ★ 경합 없음 (lock-free 불필요)│          │  ★ N:1 동적 공유                     │                 ║
║   │                                 │          │  ★ Lamport Ticket으로 경합 해결      │                 ║
║   └─────────────────────────────────┘          └──────────────────────────────────────┘                 ║
║                  │                                                │                                     ║
║                  ▼                                                ▼                                     ║
║   ┌─────────────────────────────────┐          ┌──────────────────────────────────────┐                 ║
║   │         NVMe SSD                │          │             NVMe SSD                  │                 ║
║   │   SQ/CQ: Host DRAM에 위치      │          │   SQ/CQ: GPU VRAM에 위치 ★           │                 ║
║   │   도어벨: CPU MMIO write        │          │   도어벨: GPU PTX st.mmio ★          │                 ║
║   │   데이터: Host DRAM에 DMA       │          │   데이터: GPU VRAM에 P2P DMA ★       │                 ║
║   └─────────────────────────────────┘          └──────────────────────────────────────┘                 ║
║                                                                                                         ║
╚═══════════════════════════════════════════════════════════════════════════════════════════════════════════╝
```

## 2. SQ 매핑 구조 상세 비교

### Linux Kernel: CPU Core → SQ 정적 1:1 매핑

```
                        Linux blk-mq NVMe Driver
┌───────────────────────────────────────────────────────────────┐
│                                                               │
│   CPU Core 0        CPU Core 1        CPU Core 2       ...   │
│   ┌──────────┐     ┌──────────┐     ┌──────────┐            │
│   │ Thread   │     │ Thread   │     │ Thread   │            │
│   │ Thread   │     │ Thread   │     │ Thread   │            │
│   │ Thread   │     │ Thread   │     │ Thread   │            │
│   └────┬─────┘     └────┬─────┘     └────┬─────┘            │
│        │                │                │                    │
│  ┌─────▼─────┐    ┌─────▼─────┐    ┌─────▼─────┐           │
│  │ hctx 0    │    │ hctx 1    │    │ hctx 2    │           │
│  │(HW queue) │    │(HW queue) │    │(HW queue) │           │
│  └─────┬─────┘    └─────┬─────┘    └─────┬─────┘           │
│        │                │                │                    │
│   ─────┼────────────────┼────────────────┼─── blk-mq ───    │
│        │                │                │   CPU→hctx 매핑   │
│        │ 1:1 고정       │ 1:1 고정       │ 1:1 고정         │
│        ▼                ▼                ▼                    │
│  ┌──────────┐     ┌──────────┐     ┌──────────┐            │
│  │  SQ #1   │     │  SQ #2   │     │  SQ #3   │            │
│  │ ┌──────┐ │     │ ┌──────┐ │     │ ┌──────┐ │            │
│  │ │cmd   │ │     │ │cmd   │ │     │ │cmd   │ │            │
│  │ │cmd   │ │     │ │cmd   │ │     │ │cmd   │ │            │
│  │ │...   │ │     │ │...   │ │     │ │...   │ │            │
│  │ └──────┘ │     │ └──────┘ │     │ └──────┘ │            │
│  │  Host    │     │  Host    │     │  Host    │            │
│  │  DRAM    │     │  DRAM    │     │  DRAM    │            │
│  └────┬─────┘     └────┬─────┘     └────┬─────┘            │
│       │                │                │                    │
│  SQ DB 0          SQ DB 1          SQ DB 2                  │
│  (CPU MMIO)       (CPU MMIO)       (CPU MMIO)               │
│       │                │                │                    │
└───────┼────────────────┼────────────────┼────────────────────┘
        └────────────────┼────────────────┘
                         ▼
                   ┌───────────┐
                   │ NVMe SSD  │
                   │Controller │
                   └───────────┘

  특징:
  • blk-mq가 부팅 시 CPU 코어 수만큼 hctx(HW queue) 생성
  • 각 hctx가 1개 SQ에 1:1 바인딩 → 코어 전용 큐
  • 같은 코어의 thread들은 같은 SQ를 사용하되,
    blk-mq 내부에서 순차적으로 처리 (spin_lock 또는 lock-free)
  • 다른 코어의 thread는 절대 같은 SQ에 접근하지 않음 → 크로스코어 경합 없음
  • SQ 메모리: Host DRAM (DMA coherent 할당)
  • 도어벨: CPU가 MMIO write (writel)
```

### BaM GPU-Initiated: Warp → QP 동적 N:1 매핑

```
                        BaM GPU-Initiated I/O
┌───────────────────────────────────────────────────────────────┐
│                         GPU                                   │
│                                                               │
│   SM 0               SM 1               SM 187               │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│   │Sched0→WarpA  │  │Sched0→WarpE  │  │Sched0→WarpX  │      │
│   │Sched1→WarpB  │  │Sched1→WarpF  │  │Sched1→WarpY  │      │
│   │Sched2→WarpC  │  │Sched2→WarpG  │  │Sched2→WarpZ  │      │
│   │Sched3→WarpD  │  │Sched3→WarpH  │  │Sched3→WarpW  │      │
│   │ +60 warps... │  │ +60 warps... │  │ +60 warps... │      │
│   └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
│          │                 │                  │               │
│   ───────┼─────────────────┼──────────────────┼────────────  │
│          │                 │                  │               │
│          └────────┬────────┴──────────────────┘               │
│                   │                                           │
│                   ▼                                           │
│          ┌────────────────────┐                               │
│          │  queue_counter     │  ← Controller 전역 atomic 1개│
│          │  fetch_add(1)      │                               │
│          │  % n_qps           │                               │
│          └────────┬───────────┘                               │
│                   │  동적 라운드로빈                           │
│      ┌────────┬───┼───┬────────┐                              │
│      ▼        ▼   ▼   ▼        ▼                              │
│   ┌──────┐┌──────┐┌──────┐┌──────┐                           │
│   │QP #1 ││QP #2 ││QP #3 ││QP #N │                           │
│   │┌─SQ─┐││┌─SQ─┐││┌─SQ─┐││┌─SQ─┐│                          │
│   ││cmd │││ │cmd │││ │cmd │││ │cmd ││                          │
│   ││cmd ││││cmd ││││cmd ││││cmd ││                          │
│   ││... │││ │... │││ │... │││ │... ││                          │
│   │└────┘││└────┘││└────┘││└────┘│                           │
│   │┌─CQ─┐││┌─CQ─┐││┌─CQ─┐││┌─CQ─┐│                          │
│   ││cpl ││││cpl ││││cpl ││││cpl ││                          │
│   │└────┘││└────┘││└────┘││└────┘│                           │
│   │ GPU  ││ GPU  ││ GPU  ││ GPU  │                           │
│   │ VRAM ││ VRAM ││ VRAM ││ VRAM │                           │
│   └──┬───┘└──┬───┘└──┬───┘└──┬───┘                           │
│      │       │       │       │                                │
│  SQ DB 1  SQ DB 2  SQ DB 3  SQ DB N                          │
│  (GPU PTX st.mmio)                                            │
│      │       │       │       │                                │
└──────┼───────┼───────┼───────┼────────────────────────────────┘
       └───────┼───────┼───────┘
               ▼       ▼
         ┌───────────────┐
         │   NVMe SSD    │
         │  Controller   │  ← PCIe P2P로 GPU VRAM 직접 접근
         └───────────────┘

  특징:
  • 모든 SM의 warp가 글로벌 atomic counter로 QP를 동적 배정
  • 같은 SM의 warp도 다른 QP에 갈 수 있음 (SM 고정 바인딩 아님)
  • 여러 warp가 같은 SQ를 공유 → Lamport Ticket으로 lock-free 동기화
  • SQ/CQ 메모리: GPU VRAM (PCIe P2P DMA)
  • 도어벨: GPU가 PTX st.mmio로 PCIe MMIO write
  • 도어벨 배칭: 연속 slot을 모아 1회 doorbell로 통합
```

## 3. 경합(Contention) 해결 방식 비교

```
┌═══════════════════════════════════════════════════════════════════════════════════════┐
║                                                                                       ║
║   Linux Kernel (경합 회피)                    BaM GPU (경합 허용 + lock-free 해결)    ║
║                                                                                       ║
║   Core 0 ─→ SQ 0 (전용)                     Warp A ──┐                               ║
║   Core 1 ─→ SQ 1 (전용)                     Warp B ──┤──→ QP[3] (공유)              ║
║   Core 2 ─→ SQ 2 (전용)                     Warp C ──┘                               ║
║                                                 │                                     ║
║   같은 SQ에 2개 코어가                          │                                     ║
║   접근하는 일이 없음                         ticket = fetch_add(1)                    ║
║                                              slot = ticket % qs                       ║
║   ┌──────────┐                               ┌──────────────────────┐                 ║
║   │          │                               │  Warp A → slot[10]   │                 ║
║   │ Lock     │                               │  Warp B → slot[11]   │                 ║
║   │ 불필요   │                               │  Warp C → slot[12]   │                 ║
║   │          │                               │                      │                 ║
║   └──────────┘                               │  3개 모두 LOCKED 후  │                 ║
║                                              │  대표 1개가 doorbell │                 ║
║                                              │  1회로 3개 통합 전달 │                 ║
║                                              └──────────────────────┘                 ║
║                                                                                       ║
║   ✓ 단순하고 빠름                            ✓ 수만 warp 동시 I/O 가능              ║
║   ✗ CPU 코어 수에 비례 (수십 개)             ✓ 도어벨 배칭으로 PCIe 효율↑            ║
║                                              ✗ atomic 오버헤드 존재                   ║
║                                                                                       ║
╚═══════════════════════════════════════════════════════════════════════════════════════╝
```

## 4. SQ 자원 수명(Lifecycle) 비교

```
┌═══════════════════════════════════════════════════════════════════════════════════════┐
║                                                                                       ║
║   Linux Kernel                                BaM GPU-Initiated                       ║
║                                                                                       ║
║   SQ slot 할당:                               SQ slot 할당:                           ║
║     sq->tail++ (단일 코어 전용)                 in_ticket.fetch_add(1) (atomic)        ║
║     → 경합 없음                                 → ticket → slot 결정                  ║
║                                                 → tickets[slot] 대기 (generation)     ║
║                                                                                       ║
║   커맨드 복사:                                 커맨드 복사:                            ║
║     memcpy 64B                                  ulonglong4 × 2 (32B × 2)             ║
║                                                                                       ║
║   도어벨:                                      도어벨:                                ║
║     writel(tail, sq->db)                        tail_mark[slot] = LOCKED              ║
║     → 즉시 1개 커맨드 통지                      → move_tail()로 연속 slot 수집         ║
║                                                 → st.mmio 1회로 다수 커맨드 통지       ║
║                                                                                       ║
║   SQ slot 해제:                                SQ slot 해제:                           ║
║     CQ 인터럽트 → IRQ handler                   cq_poll(CID) → spin-wait              ║
║     → blk_mq_end_request()                      → cq_dequeue() → move_head_cq()      ║
║     → sq head 갱신                               → SQ Head Ptr 읽기                   ║
║                                                  → tickets[slot].fetch_add(1)         ║
║                                                  (generation 증가 → 재사용 허용)      ║
║                                                                                       ║
║   CID 관리:                                    CID 관리:                              ║
║     tag 기반 (blk-mq tags)                      65536 atomic 풀 (get_cid/put_cid)     ║
║     → hctx당 tag set                            → 전역 풀, spin-retry                 ║
║                                                                                       ║
║   완료 통지:                                   완료 통지:                              ║
║     MSI-X 인터럽트 → IRQ handler                인터럽트 없음 ★                       ║
║     → softirq → completion                      GPU 스레드가 CQ를 직접 polling        ║
║                                                  (phase bit 확인, nanosleep backoff)  ║
║                                                                                       ║
╚═══════════════════════════════════════════════════════════════════════════════════════╝
```

## 5. 핵심 차이 요약 테이블

| 항목 | Linux Kernel NVMe | BaM GPU-Initiated |
|------|:-:|:-:|
| **I/O 발행 주체** | CPU 스레드 | GPU warp (32 threads) |
| **매핑 방식** | 정적 1:1 (코어:SQ) | 동적 N:1 (warp:QP 라운드로빈) |
| **매핑 시점** | 부팅 시 (blk-mq 초기화) | 커널 실행 시 매번 (fetch_add) |
| **경합 해결** | 경합 자체를 회피 (전용 큐) | Lamport Ticket (lock-free) |
| **SQ/CQ 위치** | Host DRAM | GPU VRAM |
| **도어벨 방식** | CPU writel (MMIO) | GPU st.mmio (PTX) |
| **도어벨 빈도** | 커맨드당 1회 | 배칭 — 다수 커맨드에 1회 |
| **완료 통지** | MSI-X 인터럽트 | Polling (인터럽트 없음) |
| **Latency hiding** | 없음 (context switch) | Warp switching (자연스러운 hiding) |
| **I/O 경로** | syscall→VFS→FS→blk-mq→NVMe | GPU 커널 → NVMe 큐 직접 |
| **CPU 개입** | 필수 (전체 경로) | 초기화만 (런타임 불필요) |
| **스케일링 한계** | CPU 코어 수 (수십 개) | GPU warp 수 (수만 개) |

## 6. Blackwell 188 SM + SSD 1개 환경 비교

```
┌═══════════════════════════════════════════════════════════════════════════════════════┐
║                                                                                       ║
║   같은 SSD를 Linux Kernel vs BaM으로 접근할 때:                                       ║
║                                                                                       ║
║   Linux (예: 16코어 CPU)              BaM (Blackwell 188 SM)                          ║
║                                                                                       ║
║   동시 I/O 발행자:  16               동시 I/O 발행자:  최대 12,032 warp              ║
║   SQ 수:            16 (코어당 1개)   QP 수:            16~128 (SSD 한계)             ║
║   SQ당 발행자:      1 (전용)          QP당 발행자:      ~94~752 warp (공유)           ║
║   경합:             없음              경합:             ticket으로 해결               ║
║   도어벨 횟수:      커맨드당 1회      도어벨 횟수:      배칭으로 수십분의 1           ║
║   CPU 사용:         100%              CPU 사용:         0% (초기화 후)                ║
║   데이터 경로:      SSD→DRAM→GPU      데이터 경로:      SSD→GPU (직접 P2P)            ║
║                     (2-hop)                              (1-hop) ★                    ║
║                                                                                       ║
╚═══════════════════════════════════════════════════════════════════════════════════════╝
```
