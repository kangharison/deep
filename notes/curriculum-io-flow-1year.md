# PCIe HW → Application 전체 I/O 스택 완전 정복 커리큘럼 (24개월)

## 커리큘럼 개요

**목표**: PCIe 물리 계층(PHY)부터 Application(fio, LLM 추론/학습)까지 **전체 I/O 스택의 모든 레이어**를 코드/스펙 레벨로 이해한다. C언어 기본 지식에서 출발하여, 24개월 후 Linux Storage Stack 시니어 개발자 수준에 도달하며, 추가로 LLM 워크로드의 I/O 특성까지 분석할 수 있는 능력을 갖춘다.

**핵심 철학**: Bottom-Up. 하드웨어(PCIe PHY → TLP → MMIO → DMA)를 먼저 이해한 뒤, 그 위의 소프트웨어 계층(NVMe Driver → Block Layer → VFS → Application)을 쌓아간다. "왜 이렇게 만들었는가"는 항상 아래 계층의 제약에서 답을 찾을 수 있다.

**기간**: 24개월 (2026-04 ~ 2028-03)
**방식**: 스펙/이론 → 소스코드 라인바이라인 리딩 → QEMU 실험/검증 → 코드 수정 → 업스트림 기여

**소스코드 버전 기준**:
- Linux Kernel: `sources/linux` (현재 트리)
- QEMU: `sources/qemu` (현재 트리)
- SPDK: `sources/spdk` (비교 분석용)
- DPDK: `sources/dpdk` (비교 분석용)
- BaM: `sources/bam` (GPU-Storage)
- fio: 별도 클론 (ioengine 코드 분석용)

**레벨 정의**:

```
Level 0 (입문)  : C 기본, OS 개념 학습 ← 여기서 시작
Level 1 (초급)  : 커널 빌드/부팅, GDB/ftrace 사용 가능
Level 2 (중급)  : I/O 경로 전체를 함수 단위로 설명 가능
Level 3 (중상급): 커널 코드 수정 → 빌드 → 테스트 가능
Level 4 (고급)  : SPDK/GPU-Storage 아키텍처 설계 가능
Level 5 (시니어): 업스트림 패치 제출, 아키텍처 리뷰, 성능 최적화 주도 ← 최종 목표
Level 5+ (확장) : LLM I/O 워크로드 분석 및 스토리지 최적화 설계 가능
```

---

## 전체 I/O 스택 레이어 맵 (Bottom-Up)

이 커리큘럼이 다루는 전체 레이어를 한눈에 보여준다. **아래(HW)에서 위(App)로** 올라가며 학습한다.

```
═══════════════════════════════════════════════════════════════════════════════
 Layer 7: APPLICATION (fio, LLM 추론/학습, 데이터베이스)          ← Phase 1, 7B
═══════════════════════════════════════════════════════════════════════════════
  fio:
    ioengine 선택 (sync/libaio/io_uring/pvsync2)
    engines/sync.c   → write()           → syscall
    engines/libaio.c → io_submit()       → syscall
    engines/io_uring.c → io_uring_enter() → syscall

  LLM (확장):
    PyTorch DataLoader → mmap/read → Page Cache or Direct I/O
    Checkpoint Save    → torch.save() → write() + fsync()
    KV Cache Offload   → GPU→CPU→SSD (cuFile or mmap)
    Model Loading      → mmap() → demand paging → readahead → bio
───────────────────────────────────────────────────────────────────────────────
 Layer 6: SYSCALL / io_uring                                      ← Phase 3
───────────────────────────────────────────────────────────────────────────────
  SYSCALL instruction → entry_SYSCALL_64 → do_syscall_64()
  → sys_write() / sys_io_submit() / sys_io_uring_enter()
  io_uring: SQ/CQ 공유 메모리, SQPOLL, IOPOLL
───────────────────────────────────────────────────────────────────────────────
 Layer 5: VFS + FILESYSTEM                                        ← Phase 3
───────────────────────────────────────────────────────────────────────────────
  vfs_write() → new_sync_write() → f_op->write_iter()
  ext4_file_write_iter() → iomap_dio_rw() → submit_bio()
  blkdev_write_iter() → __blkdev_direct_IO() → submit_bio()
  Page Cache: generic_perform_write(), writeback, fsync
───────────────────────────────────────────────────────────────────────────────
 Layer 4: BLOCK LAYER (blk-mq) + bio                             ← Phase 3
───────────────────────────────────────────────────────────────────────────────
  submit_bio() → blk_mq_submit_bio()
  → blk_mq_get_request() (sbitmap tag)
  → blk_mq_bio_to_request() (bio → request 변환)
  → blk_mq_dispatch_rq_list() → q->mq_ops->queue_rq()
  I/O 스케줄러: none, mq-deadline, kyber
───────────────────────────────────────────────────────────────────────────────
 Layer 3: NVMe DRIVER (커널)                                      ← Phase 4
───────────────────────────────────────────────────────────────────────────────
  nvme_queue_rq() → nvme_setup_cmd() → nvme_map_data()
  → nvme_submit_cmd() → writel(sq->tail, q->q_db)  ← MMIO doorbell
  Completion: nvme_irq() → nvme_process_cq() → blk_mq_end_request()
───────────────────────────────────────────────────────────────────────────────
 Layer 2: PCIe TRANSACTION LAYER (TLP)                            ← Phase 0 (핵심)
───────────────────────────────────────────────────────────────────────────────
  CPU writel() → Write Combining Buffer → Root Complex
  → TLP 생성: Memory Write (3DW/4DW header + payload) → doorbell
  DMA: Controller → TLP Memory Read → Root Complex → DRAM fetch
       Controller → TLP Memory Write → Root Complex → DRAM store
  MSI-X: Controller → TLP Memory Write → LAPIC address → interrupt
  Configuration: TLP Config Read/Write → BAR/Capability 설정
───────────────────────────────────────────────────────────────────────────────
 Layer 1: PCIe DATA LINK LAYER (DLLP)                             ← Phase 0
───────────────────────────────────────────────────────────────────────────────
  TLP framing: sequence number + LCRC 추가
  ACK/NAK DLLP: 신뢰성 보장 (TLP 재전송)
  Flow Control DLLP: credit 기반 흐름 제어
    Posted (P): Memory Write, MSI-X → credit 소비만, ACK 불필요
    Non-Posted (NP): Memory Read → 요청+응답 쌍
    Completion (Cpl): Non-Posted에 대한 응답
───────────────────────────────────────────────────────────────────────────────
 Layer 0: PCIe PHYSICAL LAYER (PHY)                               ← Phase 0
───────────────────────────────────────────────────────────────────────────────
  SerDes: 직렬↔병렬 변환
  Encoding: Gen1/2 = 8b/10b, Gen3+ = 128b/130b
  Lane: x1, x4 (NVMe 일반), x8, x16 (GPU)
  Speed: Gen3=8GT/s, Gen4=16GT/s, Gen5=32GT/s, Gen6=64GT/s(PAM4)
  Link Training: LTSSM (Detect→Polling→Configuration→L0)
  Electrical: differential signaling, eye diagram, equalization
═══════════════════════════════════════════════════════════════════════════════

 QEMU NVMe Emulation (전 레이어 대응):                           ← Phase 5
   Layer 3 ↔ QEMU hw/nvme/ctrl.c: nvme_mmio_write(), nvme_process_sq()
   Layer 2 ↔ QEMU memory_region MMIO trap → flatview dispatch
   Layer 1~0 ↔ QEMU에서는 에뮬레이션 (물리 계층 없음)

 SPDK (커널 bypass):                                              ← Phase 6
   Layer 7 직접 → Layer 2 (TLP) — Layer 3~6 전체 건너뜀
   spdk_nvme_ns_cmd_write() → memcpy(SQ) → writel(doorbell)
   syscall 0회, context switch 0회, interrupt 0회

 GPU-Storage (BaM / GDS):                                        ← Phase 7A
   GPU 커널 → PCIe P2P TLP → NVMe Controller (CPU bypass)
```

---

## 전체 타임라인 조감도

```
Month   1   2   3 │ 4   5   6 │ 7   8   9 │10  11  12 │13  14  15 │16  17  18 │19  20  21 │22  23  24
Phase ──────────── │────────── │────────── │────────── │────────── │────────── │────────── │──────────
  0: HW기초       │           │           │           │           │           │           │
     PCIe PHY     │           │           │           │           │           │           │
     PCIe TLP     │           │           │           │           │           │           │
     C/OS기초     │           │           │           │           │           │           │
                   │ 1: App    │           │           │           │           │           │
                   │    fio    │           │           │           │           │           │
                   │    내부   │           │           │           │           │           │
                   │ 2: 환경   │           │           │           │           │           │
                   │    구축   │           │           │           │           │           │
                   │           │ 3: Syscall│           │           │           │           │
                   │           │    VFS    │           │           │           │           │
                   │           │    Bio    │           │           │           │           │
                   │           │    blk-mq │           │           │           │           │
                   │           │           │ 4: NVMe   │           │           │           │
                   │           │           │    Driver  │           │           │           │
                   │           │           │    Submit  │           │           │           │
                   │           │           │    Compl   │           │           │           │
                   │           │           │           │ 5: QEMU   │           │           │
                   │           │           │           │    NVMe   │           │           │
                   │           │           │           │  io_uring │           │           │
                   │           │           │           │  Writeback│           │           │
                   │           │           │           │           │ 6: SPDK   │           │
                   │           │           │           │           │    DPDK   │           │
                   │           │           │           │           │    커널   │           │
                   │           │           │           │           │    수정   │           │
                   │           │           │           │           │           │ 7A: GPU   │
                   │           │           │           │           │           │     Stor  │
                   │           │           │           │           │           │ 7B: LLM   │
                   │           │           │           │           │           │     I/O   │
                   │           │           │           │           │           │           │ 8: 시니어
                   │           │           │           │           │           │           │    성능
                   │           │           │           │           │           │           │    실전
Level  ──0──  ──0──  ──0──  ──1──  ──1──  ──1──  ──2──  ──2──  ──2──  ──2──  ──2──  ──2──  ──3──  ──3──  ──3──  ──3──  ──4──  ──4──  ──4──  ──4──  ──4──  ──5──  ──5──  ──5──
```

---

## 디테일 목차

```
Phase 0: HW 기초 — PCIe 물리 계층부터 C/OS까지 (Month 1~3)
├── Ch 0: PCIe Physical Layer (PHY) — 전기 신호부터 시작     [Week 1~2]
├── Ch 1: PCIe Data Link Layer — 신뢰성과 흐름 제어          [Week 3~4]
├── Ch 2: PCIe Transaction Layer (TLP) — MMIO/DMA/MSI-X      [Week 5~6]
├── Ch 3: PCIe 토폴로지, Configuration Space, BAR            [Week 7~8]
├── Ch 4: C 언어 실전 & 커널 자료구조                         [Week 9~10]
├── Ch 5: CPU 아키텍처, 메모리, DMA, IOMMU                   [Week 11~12]
└── Ch 6: 인터럽트(MSI-X), 동기화, Polling vs Interrupt      [Week 13]

Phase 1: Application Layer — fio 완전 해부 (Month 4)
├── Ch 7: fio 아키텍처 & 내부 구조                            [Week 14~15]
└── Ch 8: ioengine별 코드 분석 (sync/libaio/io_uring)        [Week 16~17]

Phase 2: 개발 환경 구축 & 디버깅 (Month 5~6)
├── Ch 9: 커널 빌드, QEMU NVMe 부팅, rootfs                  [Week 18~20]
└── Ch 10: GDB, ftrace, kprobe, bpftrace 워크플로            [Week 21~22]

Phase 3: 커널 I/O 경로 — Syscall → Block Layer (Month 7~9)
├── Ch 11: x86 시스템 콜 진입 & VFS                           [Week 23~27]
├── Ch 12: bio — 블록 I/O의 기본 단위                         [Week 28~30]
└── Ch 13: blk-mq — Multi-Queue Block Layer                  [Week 31~36]

Phase 4: NVMe Driver 심층 분석 (Month 10~12)
├── Ch 14: NVMe PCIe 드라이버 초기화                          [Week 37~40]
├── Ch 15: nvme_queue_rq() — I/O 제출 경로                    [Week 41~44]
└── Ch 16: NVMe Completion — MSI-X → CQ → request 완료       [Week 45~48]

Phase 5: QEMU NVMe & 고급 I/O (Month 13~15)
├── Ch 17: QEMU NVMe 컨트롤러 구조 & MMIO                    [Week 49~52]
├── Ch 18: QEMU NVMe I/O 처리 (Doorbell→DMA→Completion)      [Week 53~56]
├── Ch 19: io_uring 심층 분석                                  [Week 57~59]
└── Ch 20: Buffered I/O, Page Cache, Writeback, fsync        [Week 60~62]

Phase 6: SPDK/DPDK & 커널 수정 (Month 16~18)
├── Ch 21: SPDK 아키텍처 & NVMe 드라이버 (커널 1:1 비교)     [Week 63~68]
├── Ch 22: DPDK EAL, hugepage, VFIO                          [Week 69~72]
├── Ch 23: 커널 수정 실습 (I/O 통계, 스케줄러, QEMU 확장)    [Week 73~78]

Phase 7A: GPU-Storage I/O (Month 19~20)
├── Ch 24: GPUDirect Storage & cuFile API                     [Week 79~82]
└── Ch 25: BaM — GPU-Initiated I/O                           [Week 83~86]

Phase 7B: LLM I/O Flow 분석 (Month 21) ★ 확장 섹션
├── Ch 26: LLM 추론의 I/O 패턴 — 모델 로딩, KV Cache         [Week 87~88]
└── Ch 27: LLM 학습의 I/O 패턴 — Checkpoint, DataLoader      [Week 89~90]

Phase 8: 시니어 실전 — 성능 엔지니어링 & 종합 (Month 22~24)
├── Ch 28: 체계적 벤치마킹 (fio sweep, perf, BPF)            [Week 91~94]
├── Ch 29: NVMe 고급 주제 (NVMe-oF, ZNS, CMB)               [Week 95~96]
└── Ch 30: 업스트림 기여, 아키텍처 설계, 포트폴리오           [Week 97~100]
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 0: HW 기초 — PCIe 물리 계층부터 C/OS까지 (Month 1~3)
## ═══════════════════════════════════════════════════════════════

> **왜 PCIe HW부터 시작하는가?**
> NVMe SSD는 PCIe 디바이스다. writel(doorbell)이 "무엇을 하는 것인지" — 전기 신호가 SerDes를 통해 직렬화되고, 8b/10b로 인코딩되어, TLP 패킷으로 Switch를 거쳐 Endpoint에 도달하는 과정을 이해해야, 이후 모든 소프트웨어 계층의 "왜"에 답할 수 있다.

### Chapter 0: PCIe Physical Layer (PHY) — 전기 신호에서 시작 (Week 1~2)

**목표**: PCIe의 가장 밑바닥 — 전기 신호, SerDes, 인코딩, Lane, Link Training을 이해한다. "PCIe x4 Gen4 NVMe SSD"가 정확히 무엇을 의미하는지 물리적으로 설명할 수 있다.

**도달 수준**: "PCIe Gen4 x4의 이론적 대역폭이 왜 ~7.88 GB/s인가"를 encoding overhead까지 포함하여 계산할 수 있다.

**참고 자료**: PCI Express Base Specification (Revision 5.0 이상), Mindshare PCI Express Technology 3.0

```
Week 1: PCIe 전기 신호와 직렬 통신
───────────────────────────────────
1. 왜 직렬인가 (Parallel → Serial 전환의 역사)
   - PCI: 32/64-bit 병렬 버스, 33/66 MHz
     → 클럭 skew, crosstalk 문제로 주파수 한계
   - PCIe: 고속 직렬 point-to-point 링크
     → 각 Lane이 독립적 SerDes, GHz 대역 동작

2. Differential Signaling (차동 신호)
   - D+, D- 두 선의 전압차로 0/1 구분
   - 장점: 공통 모드 노이즈 제거 (common-mode rejection)
   - NVMe SSD의 PCIe 커넥터: TX+/TX-, RX+/RX- 쌍
   - 전압 swing: ~800mV~1200mV (Gen3 기준)

3. SerDes (Serializer/Deserializer)
   - Serializer: 병렬 데이터 → 직렬 비트 스트림
   - Deserializer: 직렬 비트 스트림 → 병렬 데이터
   - CDR (Clock Data Recovery): 데이터 스트림에서 클럭 추출
     → 별도 클럭 라인 없음 (embedded clock)

4. Lane과 Link
   - Lane: TX 1쌍 + RX 1쌍 = 양방향 1 Lane
   - Link: 여러 Lane 묶음 (x1, x2, x4, x8, x16)
   - NVMe SSD: 보통 x4 (M.2 폼팩터)
   - GPU: 보통 x16

5. PCIe 세대별 속도

   ┌──────┬──────────┬────────────┬────────────┬─────────────────┐
   │ Gen  │ Data Rate│ Encoding   │ 유효 대역폭│ x4 Link 대역폭  │
   │      │ (per Lane│            │ (per Lane) │ (양방향 합산)    │
   │      │  방향)   │            │            │                 │
   ├──────┼──────────┼────────────┼────────────┼─────────────────┤
   │ 1.0  │ 2.5 GT/s │ 8b/10b     │ 250 MB/s   │ 2.0 GB/s        │
   │ 2.0  │ 5.0 GT/s │ 8b/10b     │ 500 MB/s   │ 4.0 GB/s        │
   │ 3.0  │ 8.0 GT/s │ 128b/130b  │ ~985 MB/s  │ ~7.88 GB/s      │
   │ 4.0  │ 16 GT/s  │ 128b/130b  │ ~1969 MB/s │ ~15.75 GB/s     │
   │ 5.0  │ 32 GT/s  │ 128b/130b  │ ~3938 MB/s │ ~31.51 GB/s     │
   │ 6.0  │ 64 GT/s  │ PAM4+FEC   │ ~7877 MB/s │ ~63.02 GB/s     │
   └──────┴──────────┴────────────┴────────────┴─────────────────┘

   대역폭 계산 예시 (Gen4 x4, 단방향):
   16 GT/s × 4 Lanes × (128/130) = 15.754 GB/s ÷ 8 bits = ~1.969 GB/s per direction
   → 양방향 합산 ≈ 3.938 GB/s × 2 ≈ ... (실제로는 각 방향 독립)
   → 단방향 x4 = ~7.877 GB/s

6. Encoding
   - 8b/10b (Gen1, Gen2):
     → 8비트 데이터를 10비트로 인코딩
     → DC balance 유지, 충분한 bit transition 보장 (CDR용)
     → overhead: 20% (유효 대역폭 = raw × 0.8)
     → Running Disparity (RD) 추적: +1/-1 교대

   - 128b/130b (Gen3, Gen4, Gen5):
     → 128비트 데이터 + 2비트 sync header
     → overhead: 1.54% (유효 대역폭 = raw × 128/130)
     → Scrambling: LFSR로 데이터 스크램블 (DC balance용)

   - PAM4 (Gen6):
     → NRZ(2-level) 대신 4-level signaling
     → 1 symbol = 2 bits → 같은 baud rate에서 2배 데이터
     → SNR 감소 → FEC (Forward Error Correction) 필수

Week 2: Link Training & LTSSM
──────────────────────────────
1. LTSSM (Link Training and Status State Machine)
   → PCIe 링크가 UP되기까지의 상태 머신

   Detect → Polling → Configuration → L0 (정상 동작)
   │                                     │
   ├→ Recovery (에러 시)                  │
   ├→ L0s (저전력, 빠른 복귀 ~1μs)       │
   ├→ L1 (저전력, 느린 복귀 ~10μs)       │
   └→ L2/L3 (전원 차단)                  │

   - Detect: 상대편 존재 감지 (전기적)
   - Polling: bit lock, symbol lock, lane polarity 확인
   - Configuration: lane 수 협상, link width 결정, lane reversal
   - L0: 정상 데이터 전송 상태

2. 속도 협상 (Speed Negotiation)
   - 초기: Gen1 (2.5 GT/s)로 link up
   - Changed Speed bit → Recovery → 더 높은 speed 시도
   - 양쪽이 지원하는 최대 속도로 수렴
   - Equalization (Gen3+): TX/RX EQ 계수 조정
     → 고속에서 ISI (Inter-Symbol Interference) 보상

3. Eye Diagram
   - oscilloscope로 측정하는 신호 품질 지표
   - eye opening: 높이(voltage margin) × 너비(timing margin)
   - Gen4+: eye opening이 매우 작음 → equalization 필수
   - BER (Bit Error Rate) 목표: < 10^-12

4. PCIe 전원 관리 (I/O latency에 미치는 영향)
   - ASPM (Active State Power Management):
     L0s: 짧은 idle → 복귀 ~1μs
     L1: 긴 idle → 복귀 ~10~50μs
   → NVMe SSD 응답 latency에 영향!
   → 고성능 환경에서는 ASPM 비활성화 일반적

5. PCIe 에러 처리
   - Correctable Error: LCRC 에러 → 재전송으로 복구
   - Uncorrectable Non-Fatal: poisoned TLP → 해당 트랜잭션만 실패
   - Uncorrectable Fatal: link failure → 전체 리셋 필요
   - AER (Advanced Error Reporting): PCIe capability로 에러 로깅

실습:
 □ lspci -vvv에서 LnkSta (Link Status): Speed, Width 확인
 □ lspci에서 LnkCap: 디바이스가 지원하는 최대 Speed/Width 확인
 □ ASPM 상태 확인: lspci -vvv | grep ASPM
 □ PCIe Gen4 x4 NVMe의 이론 대역폭 직접 계산
 □ dmesg에서 PCIe link training 관련 메시지 분석
```

### Chapter 1: PCIe Data Link Layer — 신뢰성과 흐름 제어 (Week 3~4)

**목표**: Physical Layer 위에서 TLP의 신뢰성(ACK/NAK)과 흐름 제어(Credit)를 보장하는 Data Link Layer를 이해한다.

**도달 수준**: "NVMe doorbell write가 PCIe 계층에서 어떻게 신뢰성 있게 전달되는가"를 ACK/NAK + Credit 매커니즘으로 설명할 수 있다.

```
Week 3: DLLP와 TLP Framing
──────────────────────────
1. TLP (Transaction Layer Packet) vs DLLP (Data Link Layer Packet)
   - TLP: 상위 계층 데이터 전달 (Memory Read/Write, Config, etc.)
   - DLLP: 링크 관리 전용 (ACK, NAK, Flow Control)
   - TLP는 DLLP로 감싸져서 전송됨

2. TLP Framing (Data Link Layer가 하는 일)
   → TLP 앞에 Sequence Number (12-bit) 추가
   → TLP 뒤에 LCRC (32-bit CRC) 추가
   → Physical Layer로 전달

   [Seq#(2B)] [TLP Header + Data] [LCRC(4B)]

3. ACK/NAK 프로토콜
   - 수신측: LCRC 검증
     → OK → ACK DLLP 전송 (해당 Seq# 이하 모두 확인)
     → Fail → NAK DLLP 전송 → 송신측 재전송
   - Replay Buffer: 송신측이 ACK 받을 때까지 TLP 보관
   - Replay Timer: timeout → 자동 재전송

4. NVMe I/O와의 관계
   - doorbell writel() → Memory Write TLP → Seq# + LCRC 추가
   - DMA Read → Memory Read TLP + Completion TLP
   - TLP 손실 시: 자동 재전송 → SW 레벨에서는 투명

Week 4: Flow Control — Credit 기반 흐름 제어
──────────────────────────────────────────
1. 왜 흐름 제어가 필요한가
   - 수신측 버퍼 overflow 방지
   - PCIe는 하드웨어 레벨에서 credit 기반 흐름 제어

2. Credit 종류 (6개 독립 credit pool)
   ┌──────────────┬───────────────┬──────────────────────────┐
   │ 카테고리     │ Header Credit │ Data Credit              │
   ├──────────────┼───────────────┼──────────────────────────┤
   │ Posted (P)   │ PH            │ PD (Memory Write, MSI-X) │
   │ Non-Posted   │ NPH           │ NPD (Memory Read)        │
   │ Completion   │ CplH          │ CplD (Read Completion)   │
   └──────────────┴───────────────┴──────────────────────────┘

3. Credit 동작
   - 초기: InitFC1/InitFC2 DLLP로 credit 값 교환 (link training 시)
   - 송신: TLP 전송 시 credit 소비 (consumed)
   - 수신: TLP 처리 완료 시 UpdateFC DLLP로 credit 반환
   - credit이 0이면 TLP 전송 불가 → 대기 (backpressure)

4. Posted vs Non-Posted와 NVMe I/O
   - Doorbell Write (writel): Posted Memory Write
     → credit 소비만, 응답 대기 없음 → CPU는 즉시 리턴
     → fire-and-forget: CPU 입장에서 가장 빠른 PCIe 트랜잭션
   - DMA Read (NVMe Controller → Host Memory):
     Non-Posted Memory Read → Completion으로 데이터 반환
     → latency가 존재 (round-trip)

5. Flow Control과 NVMe 성능
   - credit 부족 → TLP 대기 → I/O 지연
   - deep queue depth → 동시 outstanding TLP 증가 → credit 압박
   - PCIe switch hop → 추가 credit pool → 추가 latency
   - Root Complex ↔ NVMe Controller 사이 hop 수가 latency에 영향

실습:
 □ lspci -vvv에서 DevCap: MaxPayload, MaxReadReq 확인
 □ Posted(doorbell) vs Non-Posted(DMA Read) 동작 차이 다이어그램 그리기
 □ setpci로 Max Payload Size 읽기 → 이론적 TLP 효율 계산
 □ PCIe switch가 있는 토폴로지 vs 직결의 credit 차이 분석
```

### Chapter 2: PCIe Transaction Layer (TLP) — MMIO, DMA, MSI-X (Week 5~6)

**목표**: TLP의 종류, 구조, 라우팅을 이해한다. NVMe I/O에서 발생하는 모든 PCIe 트랜잭션(doorbell MMIO write, DMA read/write, MSI-X interrupt)을 TLP 레벨에서 설명할 수 있다.

**도달 수준**: "4KB NVMe write에서 발생하는 TLP의 종류와 개수를 열거할 수 있다."

```
Week 5: TLP 구조와 종류
─────────────────────────
1. TLP 공통 헤더 (3DW 또는 4DW)
   ┌─────────────────────────────────────────────────┐
   │ Byte 0-3 (DW0):                                 │
   │   Fmt(2) + Type(5) = TLP 종류                    │
   │   TC(3): Traffic Class (QoS)                     │
   │   TD(1): TLP Digest 유무                         │
   │   EP(1): Error Poisoned                          │
   │   Attr(2): ordering, snoop 속성                  │
   │   Length(10): payload 크기 (DW 단위, 0=1024DW)   │
   ├─────────────────────────────────────────────────┤
   │ Byte 4-7 (DW1):                                 │
   │   Requester ID(16): Bus:Dev:Func                 │
   │   Tag(8): 트랜잭션 ID                            │
   │   Last/First DW BE(8): byte enable               │
   ├─────────────────────────────────────────────────┤
   │ Byte 8-11 (DW2): Address[31:2] (3DW) 또는       │
   │ Byte 8-15 (DW2-3): Address[63:2] (4DW)          │
   └─────────────────────────────────────────────────┘

2. 주요 TLP 종류와 NVMe 대응

   ┌─────────────────┬─────┬──────────────────────────────────────┐
   │ TLP Type        │ P/NP│ NVMe에서의 용도                      │
   ├─────────────────┼─────┼──────────────────────────────────────┤
   │ Memory Write    │ P   │ Doorbell write (writel)              │
   │ (MWr)           │     │ DMA Write (Controller→Host Memory)  │
   │                 │     │ MSI-X interrupt (special MWr)        │
   │                 │     │ CQE DMA Write (완료 항목 기록)       │
   ├─────────────────┼─────┼──────────────────────────────────────┤
   │ Memory Read     │ NP  │ DMA Read (Controller가 SQ/데이터 읽기)│
   │ (MRd)           │     │                                      │
   ├─────────────────┼─────┼──────────────────────────────────────┤
   │ Completion      │ Cpl │ MRd에 대한 데이터 응답               │
   │ (CplD)          │     │                                      │
   ├─────────────────┼─────┼──────────────────────────────────────┤
   │ Config Read/    │ NP  │ PCIe enumeration (lspci)             │
   │ Write (CfgRd/Wr)│    │ BAR 설정, Capability 접근            │
   └─────────────────┴─────┴──────────────────────────────────────┘

3. NVMe 4KB Write에서 발생하는 TLP 시퀀스 (상세)

   CPU (Host)                              NVMe Controller
    │                                          │
    │ [1] MWr TLP: doorbell write              │
    │     Addr=BAR0+0x1008 (SQ1 tail DB)       │
    │     Payload=4B (new tail value)           │
    │ ──── Posted, 3DW header ───────────────→ │
    │                                          │
    │ [2] MRd TLP: SQ entry 읽기               │
    │     Addr=SQ DMA addr + tail*64           │
    │     Length=64B (1 NVMe command)           │
    │ ←── Non-Posted, 4DW header ──────────── │
    │                                          │
    │ [3] CplD TLP: SQ entry 데이터            │
    │     64B payload (NVMe command)            │
    │ ──── Completion ─────────────────────→   │
    │                                          │
    │ [4] MRd TLP: PRP로 데이터 읽기           │
    │     Addr=PRP1 (호스트 메모리의 4KB 버퍼)  │
    │     Length=4096B (MPS에 따라 분할 가능)    │
    │ ←── Non-Posted ─────────────────────── │
    │                                          │
    │ [5] CplD TLP(들): 4KB 데이터             │
    │     MaxPayload에 따라 분할               │
    │     (MPS=256B → 16개 CplD TLP)           │
    │     (MPS=512B → 8개 CplD TLP)            │
    │ ──── Completion(s) ──────────────────→   │
    │                                          │
    │ ... (Controller가 Flash에 쓰기)          │
    │                                          │
    │ [6] MWr TLP: CQE 기록                    │
    │     Addr=CQ DMA addr + head*16           │
    │     Payload=16B (CQ Entry)               │
    │ ←── Posted ─────────────────────────── │
    │                                          │
    │ [7] MWr TLP: MSI-X interrupt             │
    │     Addr=LAPIC MSI address               │
    │     Payload=4B (MSI data/vector)          │
    │ ←── Posted ─────────────────────────── │
    │                                          │

   총 TLP 수 (MPS=256B 기준):
   MWr: 1(doorbell) + 1(CQE) + 1(MSI-X) = 3
   MRd: 1(SQ entry) + 1(data fetch) = 2
   CplD: 1(SQ entry) + 16(4KB data) = 17
   합계: ~22 TLP for 1 NVMe 4KB Write

Week 6: TLP 라우팅과 Ordering
────────────────────────────
1. TLP 라우팅 방법
   - Address-based: Memory TLP → 주소로 라우팅
   - ID-based: Config/Completion TLP → BDF(Bus:Dev:Func)로 라우팅
   - Implicit: Message TLP → 묵시적 라우팅

2. PCIe Ordering Rules (중요!)
   ┌───────────────────────────────────────────────────┐
   │ Producer-Consumer 모델 보장을 위한 순서 규칙:     │
   │                                                   │
   │ Posted는 Posted를 추월할 수 없다 (같은 방향)      │
   │ → CQE MWr 후 MSI-X MWr: MSI-X가 도착하면         │
   │   CQE도 이미 도착 보장 ← NVMe 동작의 핵심!        │
   │                                                   │
   │ Read Completion은 Write를 추월할 수 없다          │
   │ (Relaxed Ordering 비활성화 시)                     │
   └───────────────────────────────────────────────────┘

   → 이 규칙 덕분에 NVMe 드라이버가 MSI-X를 받으면
     CQE가 메모리에 있다고 확신할 수 있다
   → Relaxed Ordering (RO): GPU-Storage P2P DMA에서 중요

3. MaxPayloadSize (MPS)와 MaxReadRequestSize (MRRS)
   - MPS: 하나의 TLP에 실을 수 있는 최대 payload
     128B, 256B, 512B, 1024B, 2048B, 4096B
   - MRRS: 하나의 Memory Read Request가 요청할 수 있는 최대 크기
   - 시스템의 MPS = 트리 내 모든 디바이스의 min(MaxPayload)
   - MPS가 작으면 → 더 많은 TLP → 더 많은 header overhead

4. TLP Performance 계산

   4KB 데이터 전송 시 TLP 효율:
   MPS=128B: 4096/128 = 32 TLP, header overhead = 32×(12+4)B = 512B
             효율 = 4096/(4096+512) = 88.9%
   MPS=256B: 4096/256 = 16 TLP, overhead = 16×16B = 256B
             효율 = 4096/(4096+256) = 94.1%
   MPS=512B: 4096/512 = 8 TLP, overhead = 8×16B = 128B
             효율 = 4096/(4096+128) = 97.0%

5. Relaxed Ordering과 No Snoop
   - RO: Posted write가 다른 Posted write를 추월 가능
     → GPU P2P DMA에서 성능 향상, 단 순서 보장 약화
   - No Snoop: CPU cache snoop 생략 → DMA latency 감소

실습:
 □ setpci로 MPS, MRRS 값 읽기 & 변경
 □ 4KB NVMe write의 전체 TLP 시퀀스 다이어그램 그리기
 □ MPS 128B vs 512B에서의 TLP 수/효율 계산
 □ PCIe ordering rule이 NVMe CQE 신뢰성을 보장하는 과정 설명
```

### Chapter 3: PCIe 토폴로지, Configuration Space, BAR (Week 7~8)

**목표**: PCIe 시스템 구조(Root Complex → Switch → Endpoint)와 Configuration Space, BAR를 이해한다.

**도달 수준**: lspci -vvv 출력의 모든 필드를 해석할 수 있다. NVMe BAR0의 의미와 MMIO 매핑을 설명할 수 있다.

```
Week 7: PCIe 토폴로지
─────────────────────
1. PCIe 트리 구조
   ┌──────────────────────────────┐
   │         CPU (iMC)           │
   │     ┌────────┐              │
   │     │  Root  │              │
   │     │Complex │              │
   │     └──┬──┬──┘              │
   │        │  │                 │
   └────────┼──┼─────────────────┘
            │  │
   ┌────────┘  └────────┐
   │                    │
   ▼                    ▼
 ┌──────────┐     ┌──────────┐
 │ Endpoint │     │ Switch   │
 │ (NVMe)   │     │  ┌───┐  │
 │ x4 Gen4  │     │  │ UP│  │   UP = Upstream Port
 └──────────┘     │  └─┬─┘  │   DP = Downstream Port
                  │  ┌─┴─┐  │
                  │ DP  DP  │
                  └──┬──┬──┘
                     │  │
                     ▼  ▼
               ┌────┐  ┌────┐
               │NVMe│  │NVMe│
               │ #2 │  │ #3 │
               └────┘  └────┘

2. BDF (Bus:Device:Function) — 디바이스 주소
   - Bus: 0~255, Device: 0~31, Function: 0~7
   - 예: 0000:03:00.0 → Domain 0, Bus 3, Device 0, Function 0
   - Switch 아래의 디바이스는 secondary bus number를 받음

3. lspci -tv: 트리 구조 확인
   → NVMe가 Root Complex 직결인지, Switch 아래인지 확인
   → GPU와 NVMe가 같은 Root Port 하위인지 (P2P DMA 영향)

Week 8: Configuration Space & BAR
──────────────────────────────────
1. PCIe Configuration Space 구조
   ┌─────────────────────────────────────────┐
   │ Offset 0x00-0x3F: Type 0 Header (64B)  │
   │   00-01: Vendor ID                      │
   │   02-03: Device ID                      │
   │   04-05: Command Register               │
   │   06-07: Status Register                │
   │   08: Revision ID                       │
   │   09-0B: Class Code (0x010802 = NVMe)   │
   │   10-27: BAR0 ~ BAR5                    │
   │   2C-2D: Subsystem Vendor ID            │
   │   34: Capabilities Pointer              │
   ├─────────────────────────────────────────┤
   │ Extended Config Space (0x100~0xFFF):    │
   │   PCIe Capability, MSI-X, AER, etc.     │
   └─────────────────────────────────────────┘

2. BAR (Base Address Register)
   - BAR: 디바이스의 메모리 영역을 시스템 주소 공간에 매핑
   - NVMe BAR0: 컨트롤러 레지스터 (CAP, CC, CSTS, Doorbell 등)
     → 보통 16KB~64KB 크기
   - BAR 할당 과정:
     BIOS/UEFI가 BAR에 0xFFFFFFFF 기록 → 디바이스가 크기 반환
     → BIOS가 실제 주소 할당 → BAR에 기록

3. NVMe BAR0 레지스터 맵
   ┌──────────┬──────────┬────────────────────────────────┐
   │ Offset   │ Name     │ Description                    │
   ├──────────┼──────────┼────────────────────────────────┤
   │ 0x00     │ CAP      │ Controller Capabilities (64b)  │
   │ 0x08     │ VS       │ Version                        │
   │ 0x0C     │ INTMS    │ Interrupt Mask Set             │
   │ 0x10     │ INTMC    │ Interrupt Mask Clear           │
   │ 0x14     │ CC       │ Controller Configuration       │
   │ 0x1C     │ CSTS     │ Controller Status              │
   │ 0x24     │ AQA      │ Admin Queue Attributes         │
   │ 0x28     │ ASQ      │ Admin SQ Base Address (64b)    │
   │ 0x30     │ ACQ      │ Admin CQ Base Address (64b)    │
   │ 0x1000   │ SQ0TDBL  │ Admin SQ Tail Doorbell         │
   │ 0x1004   │ CQ0HDBL  │ Admin CQ Head Doorbell         │
   │ 0x1008   │ SQ1TDBL  │ I/O SQ1 Tail Doorbell          │
   │ 0x100C   │ CQ1HDBL  │ I/O CQ1 Head Doorbell          │
   │ ...      │ ...      │ (큐 수만큼 반복)               │
   └──────────┴──────────┴────────────────────────────────┘

   Doorbell Stride = 2^(CAP.DSTRD+2) bytes (보통 4B)

4. MMIO 매핑 (커널)
   - pci_resource_start(pdev, 0): BAR0 물리 주소 획득
   - ioremap(phys_addr, size): 물리 주소 → 커널 가상 주소 매핑
   - writel(value, addr): MMIO 쓰기 → PCIe Memory Write TLP 발생
   - readl(addr): MMIO 읽기 → PCIe Memory Read + Completion TLP

5. MSI-X Capability
   - MSI-X Table: BAR 내 또는 별도 BAR에 매핑
   - 각 entry: Message Address(8B) + Message Data(4B) + Vector Control(4B)
   - Message Address = LAPIC 주소 (0xFEExxxxx)
   - Message Data = 인터럽트 벡터 번호
   - NVMe: per-queue MSI-X vector (큐별 독립 인터럽트)

실습:
 □ lspci -vvv로 NVMe 디바이스의 모든 필드 해석
 □ BAR0 주소 확인 → /proc/iomem에서 매핑 확인
 □ setpci로 config space 직접 읽기/쓰기
 □ MSI-X Table 위치와 vector 수 확인
 □ NVMe BAR0 레지스터 맵과 NVMe spec 대조
```

### Chapter 4: C 언어 실전 & 커널 자료구조 (Week 9~10)

**목표**: 커널 코드를 읽는 데 필요한 C 패턴(포인터, 구조체, 매크로, 함수 포인터)과 빌드 시스템을 완벽히 익힌다.

**도달 수준**: 커널 소스의 아무 파일이나 열어서 "이 코드가 뭘 하는지" 5분 내에 파악 가능.

```
Week 9: C 포인터, 함수 포인터, 콜백
────────────────────────────────────
1. 포인터 산술, 이중 포인터, void 포인터
   - container_of() 매크로: include/linux/kernel.h
     → 구조체 멤버 포인터에서 구조체 시작 주소를 역산
     → 커널 전체에서 수천 번 사용되는 핵심 패턴

2. 구조체 패딩과 메모리 레이아웃
   - sizeof, offsetof, __packed, __aligned
   - NVMe 명령 구조체가 64바이트로 정렬되는 이유

3. 함수 포인터 & Operations 패턴
   - struct file_operations: .read_iter, .write_iter
   - struct blk_mq_ops: .queue_rq, .complete, .init_hctx
   → C의 OOP: "인터페이스"를 함수 포인터 테이블로 구현
   → 이 패턴을 이해하면 커널 코드의 50%는 읽을 수 있다

4. 콜백: bio->bi_end_io, timer_setup() + callback

Week 10: 매크로, 커널 자료구조, 빌드 시스템
────────────────────────────────────────────
1. 전처리기 매크로: #define, ##, likely()/unlikely(), BUG_ON()
2. 커널 연결 리스트: include/linux/list.h
   - list_add(), list_del(), list_for_each_entry()
   - bio 체인, request 리스트, NVMe queue 관리에 모두 사용
3. RB-Tree, XArray (page cache 인덱싱)
4. Makefile, Kconfig, obj-y/obj-m
5. GDB 기초, git blame, git format-patch

실습:
 □ container_of() 직접 구현 및 테스트
 □ struct bio의 메모리 레이아웃 그리기
 □ list_head로 간단한 큐 구현
 □ 커널 직접 빌드 (make -j$(nproc))
 □ drivers/nvme/host/Makefile 분석
```

### Chapter 5: CPU 아키텍처, 메모리, DMA, IOMMU (Week 11~12)

**목표**: CPU, 메모리, DMA, IOMMU 등 하드웨어 기초와 프로세스/메모리 관리 등 OS 핵심을 이해한다.

```
Week 11: CPU & 메모리 아키텍처
──────────────────────────────
1. x86_64 실행 모드: Ring 0/3, SYSCALL/SYSRET
2. 캐시 계층: L1/L2/L3, cache line(64B), MESI 프로토콜
   - write combining: MMIO doorbell write 최적화와의 관계
3. TLB, 가상 메모리, 4-level page table
4. hugepage: TLB miss 감소 → SPDK에서 핵심 활용

Week 12: DMA & IOMMU
─────────────────────
1. DMA: CPU 개입 없이 디바이스↔메모리 직접 전송
   - dma_alloc_coherent(): NVMe SQ/CQ DMA 메모리 할당
   - dma_map_sg(): scatter-gather DMA 매핑
2. IOMMU (Intel VT-d / AMD-Vi)
   - 디바이스의 DMA 주소 → 물리 주소 변환
   - QEMU에서의 vIOMMU
3. 커널 가상 주소 vs 물리 주소 vs DMA 주소의 관계

실습:
 □ perf stat으로 cache miss rate 측정
 □ /proc/iomem으로 물리 메모리 맵 확인
 □ dmesg에서 IOMMU/DMA 관련 메시지 분석
 □ 가상→물리→DMA 주소 변환 다이어그램 그리기
```

### Chapter 6: 인터럽트(MSI-X), 동기화, Polling vs Interrupt (Week 13)

**목표**: MSI-X 인터럽트, 커널 동기화, Polling vs Interrupt 트레이드오프를 이해한다.

```
Week 13: 인터럽트 & 동기화
──────────────────────────
1. MSI-X: PCIe TLP(Memory Write)로 인터럽트 전달
   - MSI-X Table → LAPIC → IDT → 인터럽트 핸들러
   - Top-half (hardirq) vs Bottom-half (softirq, workqueue)

2. Polling vs Interrupt
   - Interrupt: latency 낮지만 overhead (context switch, cache pollution)
   - Polling: overhead 없지만 CPU 점유
   - Hybrid: interrupt coalescing, busy polling
   → NVMe/SPDK 성능 차이의 핵심 요인

3. 커널 동기화
   - spinlock, mutex, RCU, atomic, per-CPU 변수
   - blk-mq per-CPU queue: lock contention 감소의 설계 이유

실습:
 □ /proc/interrupts에서 NVMe MSI-X 벡터 확인
 □ spin_lock의 어셈블리 분석
 □ Polling vs Interrupt latency 차이 개념 다이어그램
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 1: Application Layer — fio 완전 해부 (Month 4)
## ═══════════════════════════════════════════════════════════════

> **왜 fio부터 상세하게 보는가?**
> fio는 스토리지 벤치마킹의 de facto 표준이다. fio의 내부 구조(job, thread, ioengine, io_u)를 이해하면, "Application이 어떻게 I/O를 생성하는가"를 코드 레벨로 알 수 있다. 이후 커널 I/O 경로를 공부할 때 "위에서 뭐가 내려오는지"를 정확히 알고 시작할 수 있다.

### Chapter 7: fio 아키텍처 & 내부 구조 (Week 14~15)

**목표**: fio의 전체 아키텍처 — job/thread 모델, io_u 구조체, 이벤트 루프를 코드 레벨로 이해한다.

**도달 수준**: fio의 모든 주요 파라미터가 내부적으로 어떤 코드 경로를 타는지 설명할 수 있다.

**소스**: fio 소스코드 (git clone https://github.com/axboe/fio)

```
Week 14: fio 전체 아키텍처
──────────────────────────
1. fio 빌드 & 디렉토리 구조
   fio/
   ├── fio.c              ← main(), 전체 진입점
   ├── fio.h              ← 핵심 구조체 선언
   ├── init.c             ← job 파싱, 옵션 처리
   ├── backend.c          ← 워커 스레드 메인 루프
   ├── ioengines.c        ← ioengine 추상화 계층
   ├── io_u.c             ← I/O unit 관리
   ├── stat.c             ← 통계 수집/리포팅
   ├── engines/           ← ioengine 구현체들
   │   ├── sync.c         ← pread/pwrite
   │   ├── libaio.c       ← Linux AIO (io_submit)
   │   ├── io_uring.c     ← io_uring
   │   ├── sg.c           ← SCSI generic
   │   └── ...
   └── os/                ← OS 추상화

2. fio 실행 흐름 (main → I/O 완료)

   main() (fio.c)
     → parse_options() (init.c)           ← job 파일/CLI 파싱
     → fio_backend() (backend.c)          ← 워커 스레드 생성
       → thread_main() (backend.c)        ← 각 job의 메인 루프
         → td_io_queue() (ioengines.c)    ← I/O 제출
         → td_io_getevents() (ioengines.c)← I/O 완료 대기
         → io_completed() (io_u.c)        ← 통계 갱신

3. 핵심 구조체

   struct thread_data (td):
   ─────────────────────────
   - 하나의 fio job = 하나의 thread_data
   - td->o: struct thread_options (사용자 옵션)
     .ioengine, .bs, .iodepth, .rw, .direct, .numjobs 등
   - td->io_ops: struct ioengine_ops (ioengine 함수 테이블)
   - td->files[]: 타겟 파일/디바이스
   - td->io_u_freelist: 사용 가능한 io_u pool
   - td->io_u_all: 전체 io_u 배열

   struct io_u:
   ────────────
   - fio의 I/O 기본 단위 (커널의 bio/request에 대응)
   - io_u->buf: I/O 데이터 버퍼 (page-aligned)
   - io_u->offset: 파일 내 오프셋
   - io_u->buflen: I/O 크기 (= bs)
   - io_u->ddir: 방향 (DDIR_READ, DDIR_WRITE, DDIR_TRIM)
   - io_u->start_time: 제출 시각 (latency 계산용)
   - io_u->issue_time: 커널에 제출된 시각
   - io_u->file: 타겟 파일

   io_u 생명주기:
   freelist → fill(offset, size, ddir) → queue(제출) → getevents(완료)
   → io_completed(통계) → freelist

4. iodepth와 io_u pool
   - iodepth=N → io_u를 N개 사전 할당
   - N개까지 동시에 "in-flight" 가능
   - N+1번째 제출 시 → io_u 없음 → getevents로 완료 대기

5. fio 파라미터 → 코드 매핑 표

   ┌───────────────┬──────────────────────────────────────────────────┐
   │ 파라미터      │ 코드에서의 역할                                  │
   ├───────────────┼──────────────────────────────────────────────────┤
   │ ioengine=X    │ engines/X.c의 ioengine_ops 등록                 │
   │ bs=4k         │ td->o.bs[DDIR_READ/WRITE] = 4096               │
   │ iodepth=32    │ io_u pool 크기 = 32                             │
   │ direct=1      │ open() 시 O_DIRECT 플래그 → page cache bypass   │
   │ rw=randwrite  │ io_u->ddir=WRITE, offset=random                 │
   │ numjobs=4     │ thread_data 4개 생성 (fork/pthread)             │
   │ size=1G       │ job당 I/O 총량 = 1GB                            │
   │ runtime=60    │ 60초 후 자동 종료                                │
   │ filename=/dev │ td->files[0] = /dev/nvme0n1                     │
   │ ramp_time=5   │ 5초간 통계 무시 (warm-up)                       │
   │ group_reporting│ 같은 group의 job 결과 합산                     │
   └───────────────┴──────────────────────────────────────────────────┘

Week 15: fio 워커 루프 & 통계 수집
──────────────────────────────────
1. thread_main() (backend.c) 메인 루프 상세:

   while (!td->terminate) {
     // 1. io_u 획득
     io_u = get_io_u(td);           ← freelist에서 pop
     if (!io_u) break;              ← iodepth 다 참

     // 2. offset/size/ddir 결정
     io_u_set(td, io_u);            ← rw 패턴에 따라 설정

     // 3. I/O 제출
     ret = td_io_queue(td, io_u);   ← ioengine->queue()
     // ret = FIO_Q_COMPLETED (동기), FIO_Q_QUEUED (비동기), FIO_Q_BUSY

     // 4. 완료 처리
     if (queued >= iodepth || need_events) {
       td_io_getevents(td, min, max, timeout);  ← ioengine->getevents()
       for (각 완료된 io_u) {
         io_completed(td, io_u);    ← latency 계산, 통계 갱신
         put_io_u(td, io_u);        ← freelist에 반환
       }
     }
   }

2. latency 계산 (stat.c)
   - slat (submission latency):
     io_u->issue_time - io_u->start_time
     = fio가 io_u를 만든 시점 ~ 커널에 제출한 시점
     → 커널 진입 overhead 측정

   - clat (completion latency):
     completion_time - io_u->issue_time
     = 커널에 제출한 시점 ~ 완료 통지받은 시점
     → 디바이스 처리 시간 + 커널 완료 경로

   - lat (total latency):
     completion_time - io_u->start_time
     = slat + clat

3. fio 출력 해석
   write: IOPS=125k, BW=489MiB/s (512MB/s)
     slat (usec): min=1, max=50, avg=2.50, stdev=1.20
     clat (usec): min=5, max=5000, avg=25.00, stdev=15.00
      lat (usec): min=7, max=5010, avg=27.50, stdev=15.50
     clat percentiles (usec):
      | 1.00th=[   8], 5.00th=[  10], 50.00th=[  22],
      | 95.00th=[  53], 99.00th=[  95], 99.90th=[ 200],
      | 99.99th=[ 500]

   → p99=95μs: 99%의 I/O가 95μs 이내 완료
   → p99.9=200μs: tail latency = 200μs (1000건 중 1건)

실습:
 □ fio 소스를 직접 빌드 (./configure && make)
 □ GDB로 thread_main() → td_io_queue() step through
 □ io_u 구조체의 필드 값을 GDB로 확인
 □ fio --name=test --ioengine=sync --bs=4k --rw=write --size=100M 실행 후 결과 해석
 □ slat vs clat vs lat의 의미를 실제 수치로 확인
```

### Chapter 8: ioengine별 코드 분석 (Week 16~17)

**목표**: sync, libaio, io_uring 세 가지 ioengine의 내부를 코드 레벨로 분석하고, 커널에 어떤 syscall이 호출되는지 정확히 이해한다.

**도달 수준**: 각 ioengine이 커널의 어떤 진입점을 사용하는지, I/O 제출/완료 흐름이 어떻게 다른지 비교 설명할 수 있다.

```
Week 16: sync & libaio ioengine
────────────────────────────────
1. engines/sync.c — 가장 단순한 ioengine

   static struct ioengine_ops ioengine = {
     .name = "sync",
     .queue = fio_syncio_queue,      ← I/O 제출
     // getevents 없음 → 동기 I/O (queue에서 즉시 완료)
   };

   fio_syncio_queue():
     switch (io_u->ddir) {
       case DDIR_READ:  ret = pread(f->fd, buf, len, offset);
       case DDIR_WRITE: ret = pwrite(f->fd, buf, len, offset);
       case DDIR_SYNC:  ret = fsync(f->fd);
     }
     return FIO_Q_COMPLETED;  ← 동기: 제출 = 완료

   → 커널 경로: pwrite() → sys_pwrite64() → vfs_write() → ...
   → iodepth 의미 없음 (항상 1개씩 동기 처리)
   → 가장 느리지만 가장 예측 가능

2. engines/libaio.c — Linux AIO (비동기)

   static struct ioengine_ops ioengine = {
     .name = "libaio",
     .init = fio_libaio_init,        ← io_setup() 호출
     .queue = fio_libaio_queue,      ← iocb 준비 (제출 보류)
     .commit = fio_libaio_commit,    ← io_submit() 일괄 제출
     .getevents = fio_libaio_getevents, ← io_getevents() 완료 대기
     .cleanup = fio_libaio_cleanup,  ← io_destroy()
   };

   흐름:
   (1) init: io_setup(iodepth, &ctx) → AIO context 생성
   (2) queue: struct iocb 준비 → 내부 배열에 추가 (아직 커널에 안 보냄)
   (3) commit: io_submit(ctx, count, iocbs[]) → 커널에 일괄 제출
       → 커널: sys_io_submit() → aio_write() → vfs_write() → submit_bio()
   (4) getevents: io_getevents(ctx, min, max, events, timeout)
       → 커널에서 완료된 I/O 목록 반환

   → iodepth=32: 최대 32개 동시 in-flight
   → Direct I/O 필수 (O_DIRECT), Buffered I/O 미지원
   → commit()에서 일괄 제출 → syscall 횟수 감소 (batching)

   ┌──────────────────────────────────────────────────────┐
   │ libaio 시퀀스 다이어그램                             │
   │                                                      │
   │  fio (userspace)         kernel                      │
   │   │                        │                         │
   │   │ io_setup(32, &ctx)     │                         │
   │   │───────────────────────→│ aio_ctx 할당            │
   │   │                        │                         │
   │   │ queue(io_u[0])         │ (내부 보관)             │
   │   │ queue(io_u[1])         │ (내부 보관)             │
   │   │ ...                    │                         │
   │   │ commit()               │                         │
   │   │ io_submit(ctx, N, ..)  │                         │
   │   │───────────────────────→│ N개 I/O 한번에 제출     │
   │   │                        │ → submit_bio() × N      │
   │   │                        │                         │
   │   │ io_getevents(min=1,..) │                         │
   │   │───────────────────────→│ 완료 대기               │
   │   │                        │ ← bio_endio() callback  │
   │   │←───────────────────────│ 완료된 events 반환      │
   │   │                        │                         │
   └──────────────────────────────────────────────────────┘

Week 17: io_uring ioengine & 세 engine 비교
──────────────────────────────────────────
1. engines/io_uring.c — 최신 고성능 비동기 I/O

   static struct ioengine_ops ioengine = {
     .name = "io_uring",
     .init = fio_ioring_init,         ← io_uring_setup()
     .queue = fio_ioring_queue,       ← SQE 작성
     .commit = fio_ioring_commit,     ← io_uring_enter()
     .getevents = fio_ioring_getevents, ← CQE 읽기
   };

   핵심 차이점 (vs libaio):
   - SQ/CQ가 mmap으로 유저↔커널 공유
     → SQE 작성 시 커널 진입 불필요 (유저가 직접 SQ에 기록)
   - SQPOLL 모드: io_uring_enter()조차 불필요
     → 커널 스레드가 SQ를 자동 polling → syscall 0회
   - IOPOLL: CQ도 polling → 인터럽트 불필요
     → io_uring_enter(IORING_ENTER_GETEVENTS | IORING_ENTER_EXT_ARG)

   io_uring 메모리 레이아웃:
   ┌─────────────────────────────────────────┐
   │  유저 공간 (fio)                        │
   │  ┌─────────────┐  ┌─────────────┐      │
   │  │ SQ Ring     │  │ CQ Ring     │      │
   │  │ head, tail  │  │ head, tail  │      │
   │  │ array[]     │  │ cqes[]      │      │
   │  └──────┬──────┘  └──────┬──────┘      │
   │         │ mmap           │ mmap         │
   │  ┌──────┴──────┐  ┌──────┴──────┐      │
   │  │ SQE Array   │  │ CQE Array   │      │
   │  │ (유저 직접  │  │ (커널이     │      │
   │  │  기록)      │  │  기록)      │      │
   │  └─────────────┘  └─────────────┘      │
   │                                         │
   ├─────────────────────────────────────────┤
   │  커널 공간                              │
   │  io_ring_ctx: 같은 메모리를 커널도 접근  │
   │  → SQE 읽기, CQE 쓰기                   │
   └─────────────────────────────────────────┘

2. 세 ioengine 비교 총괄표

   ┌─────────────────┬───────────┬──────────────┬──────────────────┐
   │ 항목            │ sync      │ libaio       │ io_uring         │
   ├─────────────────┼───────────┼──────────────┼──────────────────┤
   │ I/O 모델        │ 동기      │ 비동기       │ 비동기           │
   │ syscall (제출)  │ pwrite()  │ io_submit()  │ io_uring_enter() │
   │                 │ 매 I/O    │ batch 가능   │ 또는 SQPOLL(0회) │
   │ syscall (완료)  │ 없음(동기)│ io_getevents │ CQ 직접 읽기     │
   │ iodepth         │ 1 (사실상)│ 자유 설정    │ 자유 설정        │
   │ Direct I/O 필수 │ 아니오    │ 예           │ 아니오           │
   │ Buffered I/O    │ 가능      │ 불가         │ 가능             │
   │ Polling (IOPOLL)│ 불가      │ 불가         │ 가능             │
   │ SQPOLL          │ 불가      │ 불가         │ 가능 (syscall 0) │
   │ 커널 진입점     │ sys_pwrite│ sys_io_submit│ sys_io_uring_enter│
   │ VFS 경유        │ 예        │ 예           │ 예               │
   │ 전형적 용도     │ 단순 테스트│ 프로덕션 DB │ 최신 고성능      │
   └─────────────────┴───────────┴──────────────┴──────────────────┘

3. strace로 확인

   # sync
   strace fio --ioengine=sync --bs=4k --rw=write --direct=1 --size=1M \
     --filename=/dev/nvme0n1 --name=test
   → pwrite64(3, ..., 4096, 0) = 4096
   → pwrite64(3, ..., 4096, 4096) = 4096
   → ...

   # libaio
   → io_setup(32, ...) = 0
   → io_submit(ctx, 32, ...) = 32
   → io_getevents(ctx, 1, 32, ...) = 32
   → ...

   # io_uring
   → io_uring_setup(32, ...) = 3
   → mmap(NULL, ..., 3, 0) = 0x7f...  (SQ ring)
   → mmap(NULL, ..., 3, 0x10000000) = 0x7f...  (SQEs)
   → io_uring_enter(3, 32, 1, IORING_ENTER_GETEVENTS) = 32
   → ...

실습:
 □ fio를 세 ioengine으로 동일 workload 실행 → IOPS/latency 비교
 □ strace로 각 ioengine의 syscall 패턴 캡처 → 비교표 작성
 □ io_uring SQPOLL 모드에서 io_sq_thread CPU 사용률 확인
 □ GDB로 engines/io_uring.c의 fio_ioring_queue() step through
 □ iodepth={1,4,16,64} sweep → 각 ioengine의 확장성 비교
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 2: 개발 환경 구축 & 디버깅 (Month 5~6)
## ═══════════════════════════════════════════════════════════════

### Chapter 9: 커널 빌드, QEMU NVMe 부팅, rootfs (Week 18~20)

**목표**: QEMU + GDB로 커널 코드를 한 줄씩 따라갈 수 있는 환경을 완성한다.

```
Week 18: 커널 빌드 설정
───────────────────────
1. sources/linux/Makefile → 빌드 진입점
2. 필수 CONFIG:
   CONFIG_DEBUG_INFO=y, CONFIG_GDB_SCRIPTS=y
   CONFIG_BLK_DEV_NVME=y, CONFIG_NVME_CORE=y
   CONFIG_DEBUG_FS=y, CONFIG_BLK_DEV_IO_TRACE=y
   CONFIG_FTRACE=y, CONFIG_KPROBES=y, CONFIG_IO_URING=y

3. QEMU NVMe 디바이스 옵션:
   qemu-system-x86_64 -drive file=test.qcow2,if=none,id=nvm \
     -device nvme,serial=deadbeef,drive=nvm,id=nvme0,max_ioqpairs=4

Week 19: rootfs 준비 & fio 설치
───────────────────────────────
1. debootstrap/buildroot로 최소 rootfs
2. fio static build 포함
3. 부팅 → NVMe 인식 확인 (lsblk, nvme list)

Week 20: GDB 연결 & 기본 디버깅
────────────────────────────────
1. qemu -s -S → gdb vmlinux → target remote :1234
2. break nvme_queue_rq → continue → I/O 시 중단 확인
```

### Chapter 10: GDB, ftrace, kprobe, bpftrace 워크플로 (Week 21~22)

```
Week 21: ftrace & kprobe
─────────────────────────
1. trace-cmd record -p function_graph -g nvme_queue_rq
2. printk 기반 디버깅 → dmesg 확인
3. kprobe/kretprobe 동적 트레이싱

Week 22: bpftrace (고급)
─────────────────────────
1. bpftrace -e 'kprobe:nvme_queue_rq { @start[tid] = nsecs; }'
2. biolatency, biosnoop 도구

실습:
 □ QEMU에서 fio 4K random write → GDB로 nvme_queue_rq() step
 □ ftrace function_graph로 전체 호출 체인 캡처
 □ kprobe로 nvme_queue_rq의 인자 트레이싱
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 3: 커널 I/O 경로 — Syscall → Block Layer (Month 7~9)
## ═══════════════════════════════════════════════════════════════

### Chapter 11: x86 시스템 콜 진입 & VFS (Week 23~27)

**목표**: 유저 공간의 write()가 커널에 진입하여 VFS → 파일시스템 → bio 생성까지의 경로를 라인바이라인으로 이해한다.

```
Week 23: syscall 진입 메커니즘
────────────────────────────
1. glibc write() → SYSCALL instruction
2. arch/x86/entry/entry_64.S: entry_SYSCALL_64
   → swapgs, RSP 전환, pt_regs 저장, do_syscall_64() 호출
3. syscall dispatch: sys_call_table[nr](regs)

Week 24: syscall 복귀 & 세 가지 I/O syscall 비교
────────────────────────────────────────────────
1. sys_write(), sys_io_submit(), sys_io_uring_enter() 비교
2. SPDK와의 비교: syscall 자체가 없다 (vfio + mmap)

Week 25: VFS — fd에서 file_operations까지
─────────────────────────────────────────
1. ksys_write() → fdget_pos() → vfs_write()
2. f_op->write_iter(): 어떤 함수가 불리는지는 파일 종류에 따라 결정
3. block device vs ext4 file_operations 비교

Week 26: 블록 디바이스 & ext4 경로
──────────────────────────────────
1. blkdev_write_iter() → __blkdev_direct_IO() → bio 생성 → submit_bio()
2. ext4_file_write_iter() → iomap_dio_rw() → submit_bio()
3. Direct I/O vs Buffered I/O 분기점

Week 27: Page Cache 개요
────────────────────────
1. generic_perform_write() → pagecache_get_page() → set_page_dirty()
2. Dirty 페이지 관리, balance_dirty_pages()
3. Direct I/O가 Page Cache를 bypass하는 이유와 메커니즘

실습:
 □ GDB로 ksys_write() → vfs_write() → blkdev_write_iter() step
 □ strace로 fio의 syscall 패턴 비교 (sync vs libaio vs io_uring)
 □ ftrace로 ext4_dio_write_iter → submit_bio 호출 체인 캡처
```

### Chapter 12: bio — 블록 I/O의 기본 단위 (Week 28~30)

**목표**: struct bio의 생성부터 소멸까지 전체 생명주기를 이해한다.

```
Week 28: struct bio 구조체 해부
─────────────────────────────
1. bi_iter (bvec_iter): sector, size, idx
2. bi_io_vec (bio_vec 배열): page, offset, len → scatter-gather
3. bi_end_io: completion callback
4. bi_opf: REQ_OP_READ/WRITE + flags

Week 29: bio 할당과 페이지 매핑
──────────────────────────────
1. bio_alloc_bioset() → mempool_alloc()
2. bio_add_page() → bio_vec 추가
3. bio_split() + bio_chain(): max_sectors 초과 시 분할

Week 30: bio 제출과 완료
────────────────────────
1. submit_bio() → submit_bio_noacct() → blk_mq_submit_bio()
2. bio_endio() → callback 호출
3. bio chaining: child 완료 → parent callback

실습:
 □ GDB로 bio_alloc → bio_add_page에서 bio_vec 값 확인
 □ 4KB vs 128KB write 시 bio_vec 개수 차이 확인
 □ bio_endio에 printk 추가하여 완료 시점 확인
```

### Chapter 13: blk-mq — Multi-Queue Block Layer (Week 31~36)

**목표**: blk-mq의 전체 아키텍처를 라인바이라인으로 추적한다.

```
Week 31: blk-mq 아키텍처와 자료구조
────────────────────────────────────
1. struct blk_mq_hw_ctx (hctx), blk_mq_ctx (ctx), blk_mq_ops
2. CPU → Software Queue → Hardware Queue 매핑
3. SPDK 비교: SPDK에는 블록 레이어가 없다 → overhead = 0

Week 32: request 할당과 bio→request 변환
───────────────────────────────────────
1. blk_mq_submit_bio() → __blk_mq_alloc_requests()
2. sbitmap tag 할당 + request 획득
3. request 병합 (merge), plug/unplug batching

Week 33: dispatch — request를 드라이버에 전달
────────────────────────────────────────────
1. blk_mq_run_hw_queue() → blk_mq_dispatch_rq_list()
2. q->mq_ops->queue_rq() → nvme_queue_rq()

Week 34: tag & sbitmap 심층
───────────────────────────
1. sbitmap: multi-word bitmap, lock-free allocation
2. tag = request 배열 index = NVMe command ID

Week 35: I/O 스케줄러 (none / mq-deadline / kyber)
──────────────────────────────────────────────────
1. none: NVMe 기본, 바로 dispatch
2. mq-deadline: read starvation 방지
3. kyber: token 기반 rate limiting

Week 36: Request 완료 경로
─────────────────────────
1. blk_mq_end_request() → blk_update_request() → bio_endio()
2. tag 해제, request 반환
3. softirq vs direct completion

실습:
 □ GDB로 blk_mq_submit_bio() 전체 step through
 □ request tag 값과 NVMe command ID 일치 확인
 □ none/mq-deadline 스케줄러 전환 → fio 성능 비교
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 4: NVMe Driver 심층 분석 (Month 10~12)
## ═══════════════════════════════════════════════════════════════

### Chapter 14: NVMe PCIe 드라이버 초기화 (Week 37~40)

**목표**: NVMe 디바이스의 probe → queue 생성 → namespace 등록 전체를 라인바이라인 분석한다.

```
Week 37: PCIe 디바이스 발견과 드라이버 매칭
──────────────────────────────────────────
1. nvme_pci_driver: .id_table, .probe = nvme_probe
2. PCIe 열거 → PCI class code 비교 → nvme_probe() 호출

Week 38: nvme_probe() 전체 분석
───────────────────────────────
1. pci_enable_device_mem() → pci_set_master() → nvme_map_bar()
2. BAR0 ioremap → CAP/CC/CSTS/AQA/ASQ/ACQ 레지스터 접근

Week 39: 리셋과 Admin Queue 생성
───────────────────────────────
1. nvme_reset_work() → nvme_pci_configure_admin_queue()
2. dma_alloc_coherent(): SQ/CQ DMA 메모리 할당
3. CC.EN=1 → CSTS.RDY=1: 컨트롤러 활성화

Week 40: I/O Queue 생성과 Namespace 등록
───────────────────────────────────────
1. nvme_setup_io_queues() → min(CPU 수, 디바이스 최대 큐)
2. nvme_dev_add() → blk_mq_alloc_tag_set()
3. nvme_scan_work() → device_add_disk() → /dev/nvme0n1 등장!

실습:
 □ GDB로 nvme_probe() 전체 step (QEMU 부팅 시)
 □ BAR0 레지스터 값 GDB로 읽기
 □ I/O Queue 수를 1개로 제한 → 성능 변화 측정
```

### Chapter 15: nvme_queue_rq() — I/O 제출 경로 (Week 41~44)

**목표**: nvme_queue_rq()가 NVMe 명령을 조립 → SQ 기록 → Doorbell까지의 전체 과정을 instruction 수준으로 이해한다.

**역할**: **전체 커리큘럼의 핵심 중 핵심. 소프트웨어가 하드웨어에 명령을 보내는 정확한 순간.**

```
Week 41: nvme_queue_rq() 전체 분석
─────────────────────────────────
1. nvme_setup_cmd(req): NVMe 명령 조립
2. nvme_map_data(dev, req): DMA 매핑
3. nvme_submit_cmd(nvmeq, cmd): SQ 기록 + doorbell

Week 42: NVMe 명령 조립 — nvme_setup_rw()
──────────────────────────────────────────
1. opcode, nsid, slba, length 설정
2. NVMe Command 64B 구조: 커널 구조체 ↔ NVMe spec 대응

Week 43: DMA 매핑 — nvme_map_data()
───────────────────────────────────
1. sg_init_table() → blk_rq_map_sg() → dma_map_sg()
2. PRP 리스트: ≤4KB → PRP1만, ≤8KB → PRP1+PRP2, >8KB → PRP List
3. 이 DMA 주소가 PCIe TLP의 Memory Read 주소가 된다 (Phase 0 연결)

Week 44: SQ 기록과 Doorbell — nvme_submit_cmd()
───────────────────────────────────────────────
1. memcpy(&sq_cmds[tail], cmd, 64)
2. writel(tail, q_db) → PCIe Memory Write TLP (Phase 0 Ch2 연결!)
3. Shadow Doorbell (DBBUF): MMIO 횟수 감소 최적화

   전체 연결:
   fio io_u → syscall → VFS → bio → blk-mq request → nvme_queue_rq()
   → memcpy(SQ) → writel(doorbell) → PCIe MWr TLP → NVMe Controller
   → MRd TLP(SQ fetch) → MRd TLP(data fetch) → Flash Write
   → MWr TLP(CQE) → MWr TLP(MSI-X) → CPU interrupt → nvme_irq()

실습:
 □ GDB로 nvme_queue_rq() 전체 step — 모든 변수 값 확인
 □ PRP 리스트의 DMA 주소와 사용자 버퍼 물리 주소 비교
 □ SQ 메모리 GDB 덤프: x/16x nvmeq->sq_cmds
```

### Chapter 16: NVMe Completion — MSI-X → CQ → request 완료 (Week 45~48)

**목표**: 완료 역방향 경로 전체를 추적한다.

```
Week 45: 인터럽트 핸들러와 CQ 처리
─────────────────────────────────
1. nvme_irq() → nvme_process_cq()
2. CQE: command_id(=tag), status → nvme_handle_cqe()
3. blk_mq_tag_to_rq(tag) → nvme_pci_complete_rq()

Week 46: MSI-X 인터럽트 상세
───────────────────────────
1. MSI-X TLP (Memory Write) → LAPIC → IDT → nvme_irq()
2. Interrupt Coalescing: aggregation time + threshold
3. PCIe ordering 보장: CQE MWr 먼저 → MSI-X MWr 나중 (Phase 0 연결)

Week 47: Polling Mode (io_poll)
──────────────────────────────
1. nvme_poll(): 인터럽트 없이 CQ 직접 polling
2. io_uring IOPOLL → blk_mq_poll() → nvme_poll()
3. SPDK 비교: SPDK는 항상 polling (dedicated core)

Week 48: Completion → Application 통지
──────────────────────────────────────
1. blk_mq_end_request() → bio_endio() → dio_complete()
2. → complete()/wakeup → fio io_completed()
3. io_uring: CQE 기록 → eventfd 또는 polling

실습:
 □ GDB로 nvme_irq() → nvme_process_cq() 전체 step
 □ polling vs interrupt mode fio 성능 비교
 □ interrupt coalescing 설정 변경 → latency 변화 측정
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 5: QEMU NVMe & 고급 I/O (Month 13~15)
## ═══════════════════════════════════════════════════════════════

### Chapter 17: QEMU NVMe 컨트롤러 구조 (Week 49~52)

```
Week 49: QEMU 디바이스 모델 기초
   QOM, nvme_class_init(), nvme_realize()
Week 50: BAR 등록과 MMIO 핸들러
   nvme_mmio_read/write(), doorbell → nvme_process_db()
Week 51: Admin Queue 생성 (QEMU 측)
   CC.EN=1 → nvme_start_ctrl() → CSTS.RDY=1
Week 52: QEMU NVMe 자료구조
   NvmeCtrl, NvmeSQueue, NvmeCQueue
```

### Chapter 18: QEMU NVMe I/O 처리 (Week 53~56)

```
Week 53: Doorbell → SQ 명령 읽기
   nvme_process_db() → nvme_process_sq() → nvme_addr_read()
Week 54: NVMe 명령 처리 (Read/Write)
   nvme_io_cmd() → nvme_rw() → nvme_map_prp() → dma_blk_read/write()
Week 55: QEMU 블록 백엔드
   BlockDriverState chain → file-posix.c → 호스트 pwritev()
Week 56: CQ 기록과 MSI-X
   nvme_post_cqes() → msix_notify() → 커널 nvme_irq()

실습:
 □ QEMU nvme_process_sq()에 printf → 매 명령 로그
 □ QEMU에 에러 주입 (NVME_SC_INTERNAL) → 커널 반응 확인
```

### Chapter 19: io_uring 심층 분석 (Week 57~59)

```
Week 57: io_uring 초기화와 공유 메모리
   io_uring_setup() → SQ/CQ ring buffer → mmap
Week 58: 제출 경로
   io_uring_enter() → io_submit_sqes() → vfs_iocb_iter_write()
Week 59: 완료와 polling
   IOPOLL: io_do_iopoll() → blk_mq_poll() → nvme_poll()
   SQPOLL: io_sq_thread() → syscall 없이 SQ 자동 polling

실습:
 □ io_uring C 프로그램 작성: 4KB direct write → NVMe
 □ IOPOLL vs interrupt latency 비교
 □ SQPOLL CPU 사용률 확인
```

### Chapter 20: Buffered I/O, Page Cache, Writeback, fsync (Week 60~62)

```
Week 60: Page Cache 쓰기
   generic_perform_write() → set_page_dirty()
Week 61: Writeback 메커니즘
   wb_workfn() → do_writepages() → submit_bio()
   트리거: dirty_writeback_centisecs, dirty_ratio, fsync
Week 62: fsync와 데이터 안전성
   vfs_fsync() → ext4_sync_file() → NVMe Flush 명령

실습:
 □ buffered write 후 /proc/meminfo Dirty 값 관찰
 □ fsync() → NVMe flush 경로 ftrace
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 6: SPDK/DPDK & 커널 수정 (Month 16~18)
## ═══════════════════════════════════════════════════════════════

### Chapter 21: SPDK 아키텍처 & NVMe 드라이버 (Week 63~68)

**목표**: SPDK가 커널을 bypass하여 디바이스에 접근하는 전체 구조를 커널 경로와 1:1 비교한다.

```
Week 63~64: SPDK 빌드 & 환경 구축
   hugepage 설정, PCI unbind, vfio-pci, hello_world

Week 65~66: SPDK NVMe 드라이버 구조
   spdk_nvme_probe() → BAR mmap (커널 ioremap 대응)
   spdk_nvme_ctrlr_alloc_io_qpair() → hugepage SQ/CQ

Week 67~68: SPDK I/O 제출/완료 (커널 비교)

   커널 경로:
     write() → syscall → VFS → Block → NVMe driver → writel(doorbell)
     함수 호출 ~20개, context switch 있음

   SPDK 경로:
     spdk_nvme_ns_cmd_write() → memcpy(SQ) → writel(doorbell)
     함수 호출 ~5개, context switch 0

   ┌────────────────────┬──────────────────┬──────────────────┐
   │ 항목               │ 커널 NVMe        │ SPDK             │
   ├────────────────────┼──────────────────┼──────────────────┤
   │ syscall            │ 매 I/O마다       │ 없음 (0)         │
   │ context switch     │ user↔kernel      │ 없음 (0)         │
   │ interrupt          │ MSI-X per I/O    │ 없음 (polling)   │
   │ DMA 매핑           │ 매 I/O마다       │ 사전 매핑 (0)    │
   │ CPU 사용           │ 필요 시          │ dedicated core   │
   │ 멀티테넌시         │ 커널 보호        │ app이 관리       │
   └────────────────────┴──────────────────┴──────────────────┘

실습:
 □ SPDK perf → fio(커널) 성능 비교
 □ spdk_nvme_ns_cmd_write()와 nvme_queue_rq() 코드 나란히 비교
```

### Chapter 22: DPDK EAL, hugepage, VFIO (Week 69~72)

```
Week 69~70: DPDK EAL
   rte_eal_init(): hugepage, PCI scan, mempool
   VFIO: 커널 드라이버 unbind → 유저공간 BAR mmap

Week 71~72: DPDK → SPDK 연결
   커널 kmalloc vs DPDK rte_malloc vs hugepage pool
   DMA 매핑이 trivial해지는 이유

실습:
 □ hugepage 할당 확인: /proc/meminfo
 □ VFIO PCI unbind/bind 수동 실행
```

### Chapter 23: 커널 수정 실습 (Week 73~78)

> **여기서 "읽는 사람"에서 "고치는 사람"으로 전환된다.**

```
Week 73~74: I/O 통계 수집 모듈
   nvme_queue_rq()에 ktime_get_ns() 타임스탬프 추가
   debugfs로 per-queue latency 통계 노출

Week 75~76: 커스텀 I/O 스케줄러
   struct elevator_mq_ops skeleton
   block/my-iosched.c 작성 → Kconfig/Makefile 수정

Week 77~78: QEMU NVMe 확장
   Vendor Specific 명령 (opcode 0xC0)
   I/O latency 시뮬레이션 + 에러 주입
   커스텀 커널 + 커스텀 QEMU + fio 통합 테스트

실습:
 □ 수정 → 빌드 → QEMU 부팅 사이클 확립
 □ fio random read/write 시 debugfs latency 분포 확인
 □ 에러 주입 → 커널 에러 핸들링 경로 추적
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 7A: GPU-Storage I/O (Month 19~20)
## ═══════════════════════════════════════════════════════════════

### Chapter 24: GPUDirect Storage & cuFile API (Week 79~82)

```
Week 79~80: GPU 메모리 & GPUDirect Storage 아키텍처
   CPU 경유: SSD → CPU Memory → GPU Memory (double copy, 대역폭 낭비)
   GDS 직접: SSD → [PCIe P2P] → GPU Memory (CPU bypass)
   nvidia-fs 커널 모듈: NVMe DMA 주소를 GPU BAR 주소로 설정
   PRP/SGL에 GPU 물리 주소 → P2P DMA

Week 81~82: cuFile API & 성능
   cuFileRead(), cuFileWrite()
   CPU 경유 vs GDS: 대역폭 2~3배, latency 감소, CPU 유휴
   한계: small I/O 이점 적음, P2P 지원 필요
```

### Chapter 25: BaM — GPU-Initiated I/O (Week 83~86)

```
Week 83~84: BaM 아키텍처
   GDS: CPU가 I/O 발행, 데이터만 GPU로
   BaM: GPU가 직접 I/O 발행 (GPU-initiated)
   GPU 커널에서 NVMe doorbell에 직접 MMIO write
   CMB 활용: SQ를 NVMe CMB에 배치

Week 85~86: GPU-Storage 미래 방향
   Computational Storage, CXL, NVMe CMB/PMR
   SmartNIC + GPU + NVMe 삼자 직접 통신

실습:
 □ sources/bam/ 코드 분석 (analysis/bam/ 참조)
 □ GDS vs BaM 비교표 작성
 □ papers/gpu-ssd/ 관련 논문 읽기
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 7B: LLM I/O Flow 분석 (Month 21) ★ 확장 섹션
## ═══════════════════════════════════════════════════════════════

> **왜 LLM의 I/O를 보는가?**
> LLM은 현대 GPU 워크로드의 핵심이다. 모델 크기가 수십~수백 GB에 달하면서, 모델 로딩, checkpoint 저장, KV cache offload 등 스토리지 I/O가 LLM 성능의 주요 병목 중 하나가 되었다. 지금까지 배운 PCIe HW → NVMe Driver → Block Layer → Application의 전체 스택 지식을 LLM 워크로드에 적용하여 "LLM에서 I/O가 어떻게 발생하고, 어떻게 최적화할 수 있는가"를 분석한다.

### Chapter 26: LLM 추론의 I/O 패턴 (Week 87~88)

**목표**: LLM 추론(inference) 과정에서 발생하는 I/O — 모델 로딩, KV cache offload, speculative decoding — 의 경로를 분석한다.

**도달 수준**: "70B 모델 로딩이 왜 N초 걸리는가"를 PCIe 대역폭 + NVMe I/O 크기 + 커널 경로 overhead로 설명할 수 있다.

```
Week 87: LLM 모델 로딩 I/O — 가장 큰 I/O 이벤트
────────────────────────────────────────────────
1. 모델 파일 형식과 크기
   ┌──────────────────┬──────────────┬──────────────┐
   │ 모델             │ 파라미터 수  │ FP16 크기    │
   ├──────────────────┼──────────────┼──────────────┤
   │ Llama-2 7B       │ 7B           │ ~13 GB       │
   │ Llama-2 70B      │ 70B          │ ~130 GB      │
   │ Llama-3 405B     │ 405B         │ ~750 GB      │
   │ Mixtral 8x7B     │ 47B (MoE)    │ ~87 GB       │
   └──────────────────┴──────────────┴──────────────┘

   파일 형식:
   - PyTorch (.bin/.pt): pickle + raw tensor 바이너리
   - SafeTensors (.safetensors): zero-copy mmap 가능, 헤더에 tensor 오프셋
   - GGUF: llama.cpp 포맷, quantized (Q4_0, Q8_0 등)

2. 모델 로딩 경로 #1: torch.load() (전통적)

   torch.load("model.bin")
     → Python pickle.load() → 메타데이터 역직렬화
     → storage = torch.UntypedStorage.from_file(filename, ...)
       → 내부: open() → mmap() 또는 read()
     → tensor.to("cuda:0")
       → cudaMemcpy(gpu_ptr, cpu_ptr, size, H2D)
       → 실제: DMA via PCIe (CPU Memory → GPU Memory)

   I/O 경로:
   read(fd, buf, 13GB)
     → sys_read() → vfs_read() → ext4_file_read_iter()
     → Buffered: readahead → bio 생성 → blk-mq → NVMe
     → 데이터: SSD → CPU Memory (NVMe DMA)
     → 그 후: CPU Memory → GPU Memory (cudaMemcpy, PCIe H2D)
     → double copy 발생!

   시간 추정 (NVMe Gen4 x4, 순차 읽기 ~6 GB/s):
   13 GB ÷ 6 GB/s ≈ 2.2초 (SSD→CPU)
   13 GB ÷ 25 GB/s (PCIe Gen4 x16 H2D) ≈ 0.5초 (CPU→GPU)
   합계 ≈ 2.7초 (7B 모델, 이론적 최소)
   실제: Python overhead + 파일시스템 + pickle 역직렬화 → 5~15초

3. 모델 로딩 경로 #2: mmap + lazy loading (SafeTensors)

   safetensors.torch.load_file("model.safetensors")
     → fd = open(file)
     → ptr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0)
     → tensor에 접근할 때 page fault 발생
       → do_page_fault() → filemap_fault() → readahead → bio → NVMe
     → 실제 I/O는 접근 시점에 발생 (demand paging)

   장점:
   - 초기 로딩 시간 ≈ 0 (mmap 자체는 즉시)
   - 메모리 압박 시 page evict 가능 (file-backed)
   - 여러 프로세스가 같은 파일 → 페이지 공유

   단점:
   - page fault overhead (4KB 단위, 각각 I/O 발생)
   - readahead가 효과적이지 않으면 random-like I/O
   - GPU로 복사 시 여전히 CPU 경유 필요

4. 모델 로딩 경로 #3: GDS 직접 로딩 (최적)

   cuFileRead(fd, gpu_buf, size, offset)
     → nvidia-fs: NVMe DMA 주소를 GPU BAR로 설정
     → SSD → [PCIe P2P] → GPU Memory (CPU bypass)

   장점:
   - CPU 메모리 경유 없음 → 대역폭 최적
   - CPU 부하 최소

   한계:
   - NVIDIA GPU + 호환 NVMe + P2P DMA 지원 필요
   - 프레임워크 수준에서 아직 광범위하지 않음

5. 모델 로딩 최적화 기법
   - Tensor Parallelism 로딩: 각 GPU가 자기 shard만 읽기
     → N GPU: 총 읽기량 동일, 그러나 병렬 NVMe 접근
   - Pre-sharded 파일: 미리 GPU별로 파일 분할 저장
   - NVMe RAID 또는 다중 SSD: 대역폭 선형 확장
   - madvise(MADV_SEQUENTIAL): readahead 힌트 → 순차 prefetch 극대화

Week 88: KV Cache Offload & Speculative Decoding I/O
────────────────────────────────────────────────────
1. KV Cache란?
   - Transformer의 self-attention에서 과거 토큰의 Key/Value 캐시
   - 시퀀스 길이 ∝ KV cache 크기
   - 예: Llama-2 7B, seq_len=4096, batch=1:
     KV cache ≈ 2 × 32 layers × 32 heads × 128 dim × 4096 seq × 2B(FP16)
             ≈ ~2 GB

   - 긴 시퀀스(32K, 128K): KV cache 수십~수백 GB → GPU 메모리 부족

2. KV Cache Offload (GPU → CPU → SSD)

   vLLM PagedAttention:
   - KV cache를 "page" 단위로 관리
   - GPU 메모리 부족 시: GPU page → CPU 메모리로 evict
   - CPU 메모리도 부족 시: CPU → SSD로 swap

   I/O 경로 (GPU → CPU → SSD):
   cudaMemcpy(cpu_buf, gpu_kv_page, size, D2H)  ← PCIe DMA (GPU→CPU)
   write(ssd_fd, cpu_buf, size)                   ← 커널 I/O 경로 전체

   I/O 경로 (SSD → CPU → GPU, 복원):
   read(ssd_fd, cpu_buf, size)                    ← 커널 I/O 경로
   cudaMemcpy(gpu_buf, cpu_buf, size, H2D)         ← PCIe DMA (CPU→GPU)

   성능 영향:
   - SSD swap latency: ~100μs (NVMe 4KB read) ~ ~1ms (큰 KV page)
   - GPU 연산 중 I/O가 critical path에 있으면 → 추론 latency 급증
   - 최적화: prefetch (다음 토큰 생성 전 KV page 미리 로드)

3. KV Cache Offload with GDS (이상적)

   evict: GPU KV page → cuFileWrite → SSD (CPU bypass)
   restore: cuFileRead → GPU (CPU bypass)
   → double copy 제거, CPU 부하 제거
   → 현재 연구 단계, 프레임워크 통합 진행 중

4. Speculative Decoding I/O 패턴
   - Draft 모델(작은 모델)이 N 토큰 예측 → 큰 모델이 검증
   - Draft 모델이 별도 SSD에서 로드될 수 있음
   - I/O 패턴: 작은 모델(~1-3GB) 로딩 + KV cache 관리

5. 전체 LLM 추론 I/O 타임라인

   ┌────────────────────────────────────────────────────────────┐
   │ Phase          │ I/O 유형          │ 크기        │ 경로    │
   ├────────────────┼───────────────────┼─────────────┼─────────┤
   │ 모델 로딩      │ Sequential Read   │ 13~750 GB   │ SSD→CPU │
   │                │                   │             │ →GPU    │
   │ Prefill (첫 토│ 없음 (GPU 연산)   │ -           │ -       │
   │ 큰 처리)       │                   │             │         │
   │ Decode (토큰   │ KV evict/restore  │ 수 MB~GB    │ GPU↔SSD │
   │ 생성 루프)     │ (메모리 부족 시)  │ per page    │         │
   │ 응답 완료      │ 없음              │ -           │ -       │
   └────────────────┴───────────────────┴─────────────┴─────────┘

실습:
 □ PyTorch에서 safetensors 모델 로딩 시간 측정 (mmap vs read)
 □ strace로 모델 로딩의 syscall 패턴 캡처 → I/O 크기/횟수 분석
 □ KV cache 크기 계산: 주어진 모델/시퀀스 길이에서 필요한 메모리
 □ NVMe 대역폭과 모델 로딩 시간의 관계 계산
```

### Chapter 27: LLM 학습의 I/O 패턴 (Week 89~90)

**목표**: LLM 학습(training) 과정에서 발생하는 I/O — DataLoader, Checkpoint, Gradient Accumulation — 를 분석한다.

**도달 수준**: "학습 중 I/O 병목이 어디서 발생하는가"를 스토리지 스택 레벨에서 진단할 수 있다.

```
Week 89: DataLoader I/O — 학습 데이터 읽기
──────────────────────────────────────────
1. PyTorch DataLoader 구조
   ┌──────────────────────────────────────────────┐
   │ Main Process                                 │
   │   DataLoader(num_workers=8, prefetch_factor=2)│
   │     → Worker Process #0~#7 (fork)            │
   │     → 각 Worker: Dataset.__getitem__(idx)    │
   │       → 파일 read (토큰, 이미지 등)           │
   │       → 전처리 (tokenize, augment)            │
   │     → Queue → Main Process → GPU transfer    │
   └──────────────────────────────────────────────┘

2. 데이터 읽기 패턴
   - 텍스트 학습 데이터: 큰 파일(수 GB~TB), 순차 접근
     → mmap + random offset (shuffling)
     → 실제 I/O: random read with readahead
   - 이미지 학습 데이터: 수백만 개 작은 파일 (JPEG ~100KB)
     → metadata read + data read per image
     → I/O 패턴: random small read (IOPS bound)
   - 토큰화된 데이터 (Memory-mapped):
     → np.memmap("tokens.bin", dtype=np.uint16, mode='r')
     → page fault → demand paging → bio → NVMe
     → 순차: readahead 효과적
     → 셔플: random access → readahead 비효율

3. DataLoader I/O 최적화

   ┌──────────────────────┬────────────────────────────────────┐
   │ 기법                 │ I/O 스택에서의 효과                │
   ├──────────────────────┼────────────────────────────────────┤
   │ num_workers 증가     │ 병렬 read() syscall → 블록 레이어  │
   │                      │ 큐 깊이 증가 → NVMe 병렬성 활용    │
   │ prefetch_factor 증가 │ 미리 읽기 → GPU 연산과 I/O 겹침    │
   │ WebDataset/TFRecord  │ 작은 파일 → tar/record로 묶기      │
   │ (대용량 순차 파일)   │ → 순차 I/O → 대역폭 극대화        │
   │ DALI (NVIDIA)        │ GPU에서 직접 디코딩, GDS 활용 가능 │
   │ tmpfs/RAM disk       │ 핫 데이터를 메모리에 → I/O 제거    │
   │ NVMe RAID 0          │ 대역폭 선형 확장                   │
   └──────────────────────┴────────────────────────────────────┘

4. I/O와 GPU 연산의 파이프라이닝
   - 이상적: I/O (batch N+1) || GPU 연산 (batch N)
   - I/O > GPU: I/O bound → 스토리지 최적화 필요
   - I/O < GPU: compute bound → I/O는 문제 아님
   - 진단: nvidia-smi → GPU utilization
     GPU util < 90% + 높은 I/O wait → I/O bound

Week 90: Checkpoint I/O — 학습 상태 저장
───────────────────────────────────────
1. Checkpoint 구성요소
   - 모델 가중치: 수십~수백 GB
   - 옵티마이저 상태: Adam → 모델 크기의 2~3배 (momentum, variance)
   - 학습 메타데이터: step, lr, rng 상태
   - 예: Llama-2 70B 학습:
     모델 70B params × 2B (FP16) = 140 GB
     옵티마이저 (Adam): 70B × 4B (FP32) × 3 = 840 GB
     총 checkpoint ≈ ~1 TB per save

2. Checkpoint 저장 경로

   torch.save(state_dict, "checkpoint.pt")
     → pickle.dump() → 직렬화
     → write(fd, serialized_data, size) → 커널 I/O 경로
     → fsync(fd) → NVMe flush

   I/O 특성:
   - 매우 큰 순차 쓰기 (수백 GB~1 TB)
   - write() + fsync() → Buffered I/O + writeback flush
   - 학습 중단 방지를 위해 비동기 저장 필요

3. Checkpoint 최적화 기법

   a) 비동기 Checkpoint (PyTorch DCP)
      → 별도 스레드에서 저장 → 학습 중단 없음
      → CPU 메모리에 snapshot 복사 → 백그라운드 write

   b) 분산 Checkpoint
      → 각 GPU가 자기 shard만 저장 (Data Parallel)
      → 8 GPU × 8 NVMe: 병렬 쓰기 → 대역폭 8배
      → 1 TB ÷ (8 × 3 GB/s) ≈ 42초

   c) Incremental Checkpoint
      → 변경된 부분만 저장 (diff 기반)
      → 1 TB 전체 vs 100 GB delta → 10배 감소

   d) Checkpoint to GDS
      → GPU Memory → SSD 직접 저장 (CPU bypass)
      → CPU 메모리 복사 제거

4. Checkpoint I/O 경로 상세

   동기 저장:
   GPU Memory → cudaMemcpy(D2H) → CPU Memory → write() → Page Cache
   → writeback → bio → blk-mq → NVMe DMA → SSD Flash
   → fsync() → NVMe Flush Command

   비동기 저장:
   GPU Memory → cudaMemcpy(D2H) → CPU snapshot buffer
   학습 계속 ────────────────────────────────────────────→
   (백그라운드) CPU buffer → write() → ... → SSD

5. Gradient Checkpoint (재계산 기법) ← I/O와 직접 관련 없지만 메모리 절약
   - activation을 저장하지 않고 역전파 시 재계산
   - GPU 메모리 절약 → KV cache offload 필요성 감소

6. LLM 학습 전체 I/O 프로파일

   ┌──────────────────┬──────────┬──────────┬───────────┬──────────┐
   │ 이벤트           │ I/O 방향 │ 크기     │ 빈도      │ 패턴     │
   ├──────────────────┼──────────┼──────────┼───────────┼──────────┤
   │ DataLoader       │ Read     │ MB 단위  │ 매 batch  │ 순차/랜덤│
   │ (학습 데이터)    │          │ per batch│ (~ms 주기)│          │
   ├──────────────────┼──────────┼──────────┼───────────┼──────────┤
   │ Checkpoint       │ Write    │ 수백 GB  │ N step    │ 대용량   │
   │ (모델 저장)      │          │ ~1 TB    │ 마다      │ 순차 쓰기│
   ├──────────────────┼──────────┼──────────┼───────────┼──────────┤
   │ Checkpoint       │ Read     │ 수백 GB  │ 학습 시작 │ 대용량   │
   │ (모델 복원)      │          │ ~1 TB    │ /재개 시  │ 순차 읽기│
   ├──────────────────┼──────────┼──────────┼───────────┼──────────┤
   │ Logging/Metrics  │ Write    │ KB 단위  │ 매 step   │ append   │
   ├──────────────────┼──────────┼──────────┼───────────┼──────────┤
   │ Evaluation       │ Read     │ 수 GB    │ 주기적    │ 순차     │
   │ (평가 데이터)    │          │          │           │          │
   └──────────────────┴──────────┴──────────┴───────────┴──────────┘

실습:
 □ PyTorch checkpoint 저장 시간 측정 (1GB, 10GB, 100GB 모델)
 □ strace로 checkpoint 저장의 syscall 패턴 분석
 □ iostat/blktrace로 checkpoint 저장 시 NVMe I/O 모니터링
 □ fsync 포함/미포함 시 저장 시간 차이 측정
 □ num_workers 변경에 따른 DataLoader throughput 변화 측정
 □ LLM 학습 시나리오의 총 I/O 대역폭 요구량 계산:
   예: 8 GPU, Llama 70B, 1000 step checkpoint 간격
   DataLoader: 8 workers × 100 MB/s ≈ 800 MB/s read
   Checkpoint: ~1 TB per 1000 steps, 비동기 → 평균 ~30 MB/s write
   → NVMe 1개(6 GB/s)로 충분? → 분석
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 8: 시니어 실전 — 성능 엔지니어링 & 종합 (Month 22~24)
## ═══════════════════════════════════════════════════════════════

### Chapter 28: 체계적 벤치마킹 (Week 91~94)

**목표**: fio sweep, perf flamegraph, BPF/bpftrace를 사용한 체계적 성능 분석 방법론을 습득한다.

```
Week 91: fio 마스터 클래스
─────────────────────────
1. 파라미터 완전 이해: ioengine, direct, iodepth, numjobs, bs, rw
2. 결과 해석: IOPS, bandwidth, slat/clat/lat, percentiles
3. 실험 설계: 변수 하나만 변경, warm-up, 반복 측정

Week 92: perf & flamegraph
─────────────────────────
1. perf record -g -a -- fio ... → perf report
2. FlameGraph로 CPU 시간 소비 시각화
3. perf stat: IPC, cache miss, branch miss

Week 93: BPF/bpftrace 실전
─────────────────────────
1. biolatency, biosnoop
2. 커스텀 bpftrace: NVMe doorbell 빈도, per-queue latency

Week 94: 성능 분석 케이스 스터디
──────────────────────────────
1. iodepth sweep: IOPS vs latency 트레이드오프
2. Direct I/O vs Buffered I/O 원인 분석
3. 커널 NVMe vs SPDK 정량 비교
4. io_uring polling vs interrupt latency 분석
5. LLM 모델 로딩 시간 분석: mmap vs read vs GDS

실습:
 □ fio sweep: bs=4K, iodepth={1,4,16,64,128} → IOPS/latency 그래프
 □ perf flamegraph로 I/O 경로 CPU 분석
 □ 커널 vs SPDK 동일 워크로드 비교 보고서
```

### Chapter 29: NVMe 고급 주제 (Week 95~96)

```
Week 95: NVMe over Fabrics, ZNS
   NVMe-oF: RDMA/TCP 원격 접근, transport 추상화
   ZNS: sequential write zone, FTL 부담 감소

Week 96: CMB, NVMe 2.0+
   CMB: SQ를 디바이스 메모리에 → DMA Read 불필요
   PMR, I/O Determinism, Multipath, Copy Command
```

### Chapter 30: 업스트림 기여, 아키텍처 설계, 포트폴리오 (Week 97~100)

```
Week 97~98: 커널 패치 제출
   coding-style, checkpatch.pl, get_maintainer.pl, git send-email
   실제 기여: NVMe 에러 메시지 개선, 리팩토링, 작은 버그 수정

Week 99: 아키텍처 설계
   "NVMe SSD에 QoS 추가" → 설계 문서
   "GPU-NVMe 직접 I/O를 커널에 추가" → 설계 문서
   trade-off 분석: 성능 vs 복잡도 vs 호환성

Week 100: 종합 프로젝트 & 포트폴리오
   End-to-End 분석 문서: fio write() 한 건의 전체 경로
     ioengine → syscall → VFS → bio → blk-mq → NVMe SQ
     → PCIe TLP(doorbell) → DMA TLP(data) → MSI-X TLP(completion)
     → nvme_irq() → bio_endio() → fio io_completed()
   포트폴리오: I/O 통계 모듈, 스케줄러, QEMU 확장, 패치 이력
```

---

## 월별 요약표

| 월 | Phase | Chapter | 핵심 내용 | Level |
|:---:|:---:|:---:|:---|:---:|
| 1 | 0 | Ch0~1 | PCIe PHY (SerDes, encoding, LTSSM), Data Link (ACK/NAK, Credit) | 0 |
| 2 | 0 | Ch2~3 | PCIe TLP (MMIO, DMA, MSI-X), Configuration Space, BAR | 0 |
| 3 | 0 | Ch4~6 | C/커널 자료구조, CPU/메모리/DMA/IOMMU, 인터럽트/동기화 | 0 |
| 4 | 1 | Ch7~8 | fio 아키텍처, io_u, ioengine (sync/libaio/io_uring) 코드 분석 | 0→1 |
| 5 | 2 | Ch9 | 커널 빌드, QEMU NVMe 부팅, rootfs | 1 |
| 6 | 2 | Ch10 | GDB, ftrace, kprobe, bpftrace 워크플로 | 1 |
| 7 | 3 | Ch11 전반 | syscall 진입, VFS, fd→f_op | 1→2 |
| 8 | 3 | Ch11 후반~Ch12 | ext4/blkdev 경로, bio 구조체 | 2 |
| 9 | 3 | Ch13 | blk-mq: request, tag, dispatch, 스케줄러 | 2 |
| 10 | 4 | Ch14 | NVMe probe, 초기화, queue 생성 | 2 |
| 11 | 4 | Ch15 | nvme_queue_rq(), DMA, doorbell | 2 |
| 12 | 4 | Ch16 | NVMe CQ, MSI-X, polling, completion | 2→3 |
| 13 | 5 | Ch17~18 전반 | QEMU NVMe 컨트롤러, MMIO, I/O 처리 | 3 |
| 14 | 5 | Ch18 후반~Ch19 | QEMU completion, io_uring 심층 | 3 |
| 15 | 5 | Ch20 | Page cache, writeback, fsync | 3 |
| 16 | 6 | Ch21 | SPDK 아키텍처, NVMe 드라이버 (커널 비교) | 3→4 |
| 17 | 6 | Ch22 | DPDK EAL, hugepage, VFIO | 4 |
| 18 | 6 | Ch23 | 커널 수정: I/O 통계, 스케줄러, QEMU 확장 | 4 |
| 19 | 7A | Ch24 | GPUDirect Storage, cuFile API | 4 |
| 20 | 7A | Ch25 | BaM GPU-initiated I/O, 미래 방향 | 4 |
| 21 | 7B | Ch26~27 | LLM 추론 I/O (모델 로딩, KV cache), 학습 I/O (DataLoader, checkpoint) | 4→5 |
| 22 | 8 | Ch28 | 체계적 벤치마킹 (fio, perf, BPF) | 5 |
| 23 | 8 | Ch29 | NVMe-oF, ZNS, CMB, 최신 스펙 | 5 |
| 24 | 8 | Ch30 | 업스트림 기여, 아키텍처 설계, 포트폴리오 | 5 |

---

## 주당 학습 권장 시간

| 활동 | Phase 0~2 | Phase 3~5 | Phase 6~7 | Phase 8 |
|:---|:---:|:---:|:---:|:---:|
| 스펙/이론 학습 | 5~6시간 | 3~4시간 | 2~3시간 | 2시간 |
| 소스코드 리딩 | 5~7시간 | 6~8시간 | 5~6시간 | 3~4시간 |
| GDB/ftrace/perf 실습 | 3~4시간 | 3~4시간 | 3~4시간 | 4~5시간 |
| 코드 수정 & 실험 | - | 2~3시간 | 4~6시간 | 4~6시간 |
| 노트 정리 & 문서화 | 2~3시간 | 2~3시간 | 2~3시간 | 3~4시간 |
| **합계** | **~18시간/주** | **~18시간/주** | **~18시간/주** | **~18시간/주** |

---

## 각 Phase의 전제 조건과 연결 관계

```
Phase 0 (PCIe HW + C/OS 기초)
  │  "writel()이 뭘 하는 건지" 물리적으로 이해
  ▼
Phase 1 (Application: fio)
  │  "위에서 뭐가 내려오는지" 정확히 이해
  ▼
Phase 2 (환경 구축)
  │  QEMU + GDB로 코드를 한 줄씩 따라갈 수 있는 환경
  ▼
Phase 3 (Syscall → Block Layer)
  │  bio/request 모르면 NVMe 이해 불가
  ▼
Phase 4 (NVMe Driver)
  │  커널 드라이버가 Phase 0의 PCIe TLP을 어떻게 발생시키는지 연결
  ▼
Phase 5 (QEMU NVMe + io_uring + Writeback)
  │  디바이스 에뮬레이션 + 고급 I/O 경로
  ▼
Phase 6 (SPDK/DPDK + 커널 수정)        ← 커널 경로를 알아야 비교 가능
  │  유저공간 I/O + "읽기→쓰기" 전환
  ▼
Phase 7A (GPU-Storage)                  ← NVMe + PCIe P2P TLP 이해 필수
  │  GDS, BaM, GPU-initiated I/O
  ▼
Phase 7B (LLM I/O) ★                   ← 전체 스택 + GPU 지식 필요
  │  모델 로딩, KV cache, checkpoint의 I/O 분석
  ▼
Phase 8 (시니어 실전)                   ← 모든 지식 통합
     성능 엔지니어링, 업스트림 기여, 포트폴리오
```

---

## 학습 완료 후 달성 능력 (시니어 체크리스트)

### Level 5 — 시니어 스토리지 개발자

1. **PCIe HW 이해**: writel(doorbell)이 물리적으로 어떤 TLP으로 변환되어, 어떤 encoding으로 SerDes를 통해 전송되는지 설명할 수 있다
2. **End-to-End 코드 이해**: fio write() 한 번이 ioengine → syscall → VFS → bio → blk-mq → NVMe SQ → PCIe TLP → NVMe Controller까지 어떤 함수를 몇 번 호출하는지, 변수 값까지 포함하여 설명할 수 있다
3. **성능 디버깅**: I/O 성능 문제가 Application/VFS/Block/NVMe/PCIe 중 어디서 발생하는지 perf/BPF/ftrace로 5분 내에 특정할 수 있다
4. **커널 수정**: 블록 레이어에 커스텀 스케줄러를 추가하거나, NVMe 드라이버에 새 기능을 구현할 수 있다
5. **SPDK 비교**: 커널 NVMe와 SPDK의 성능 차이를 코드 레벨에서 정량적으로 설명할 수 있다
6. **GPU-Storage**: GDS/BaM 아키텍처를 설명하고 GPU-NVMe 직접 I/O 시스템을 설계할 수 있다
7. **업스트림 기여**: Linux 커널에 패치를 제출하고 코드 리뷰를 수행할 수 있다
8. **아키텍처 설계**: "NVMe SSD에 QoS를 추가하라" 같은 요구사항에 대해 설계 문서를 작성하고 trade-off를 분석할 수 있다
9. **fio 마스터**: fio의 모든 ioengine과 파라미터를 내부 코드 레벨에서 이해하고, 체계적 벤치마킹을 설계할 수 있다
10. **교육**: 후배 개발자에게 I/O 경로를 가르치고 "왜 이렇게 해야 하는가"를 설명할 수 있다

### Level 5+ — LLM I/O 확장 역량

11. **LLM I/O 분석**: LLM 추론/학습에서 발생하는 I/O 패턴을 스토리지 스택 레벨에서 분석하고, 병목을 진단할 수 있다
12. **LLM 스토리지 최적화**: 모델 로딩(mmap vs read vs GDS), KV cache offload, checkpoint 저장 전략을 설계하고 성능을 예측할 수 있다
