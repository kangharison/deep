# nvme-pcie-tracer

NVMe PCIe-Level Bidirectional Command Tracer for Linux.

NVMe 디바이스와 호스트 간의 모든 통신을 **PCIe 레벨**에서 탐지하고 기록한다. 핵심 목표는 디바이스가 호스트 메모리에 DMA 접근하는 순간을 **IOMMU page fault**로 직접 잡는 것이다.

## 핵심 아이디어: IOMMU Page Fault로 PCIe DMA 접근 탐지

```
CQ 폴링 (간접):
  NVMe SSD --DMA Write--> CQ 메모리 변화 ← 호스트 CPU가 "값이 바뀌었네?" → 결과를 보는 것

IOMMU Page Fault (직접):
  NVMe SSD --DMA Write 시도--> IOMMU PTE에 Write 권한 없음
                                    |
                                    v
                              IOMMU Fault! ← PCIe TLP 자체를 IOMMU가 차단
                                    |
                                    v
                              dmar_fault() IRQ → Fault Record에서 정보 추출
                              - Fault Address: CQ의 IOVA
                              - Source ID: NVMe 디바이스 BDF
                              - Reason 0x05: "PTE Write access is not set"
                              - Type: DMA Write
```

## 탐지 방식 비교

| 방식 | 모듈 | 탐지 수준 | 특성 | 하드웨어 요구 | 상태 |
|------|------|-----------|------|---------------|------|
| **★ IOMMU Fault** | `nvme_iommu_fault_tracer.c` | **PCIe TLP 직접 차단** | DMA abort (파괴적) | Intel VT-d | **핵심 구현** |
| CQ 메모리 폴링 | `nvme_cq_poll_tracer.c` | 메모리 값 변화 관찰 | 비파괴, 간접적 | 없음 | 보조 |
| IOMMU Access/Dirty Bit | (미구현) | IOMMU PTE A/D 비트 | 비파괴, 투명 | VT-d Scalable Mode | 계획 |
| IOMMU PRI | (미구현) | PCIe TLP + 자동 재시도 | 비파괴, 투명 | VT-d + ATS/PRI | 계획 |
| kprobe (보조) | `nvme_sq_monitor.c` 등 | 커널 함수 호출 시점 | 비파괴, 간접적 | 없음 | Host→Device 보조 |

```
탐지 수준 비교:

  kprobe:       "nvme_irq()가 호출되었다" → 커널 함수 시점 (간접)
  CQ 폴링:      "CQ 메모리 값이 바뀌었다" → 결과 관찰 (간접)
  IOMMU Fault:  "디바이스가 IOVA에 DMA Write를 시도했다" → PCIe TLP 자체 (직접!)

  IOMMU Fault만이 진짜 PCIe 레벨 탐지이다.
  대가: 해당 DMA는 abort되고 NVMe 컨트롤러가 에러 상태에 빠진다.
```

## 아키텍처

```
+=====================================================================+
|                  nvme-pcie-tracer 전체 구조                          |
+=====================================================================+

  [ 유저스페이스 ]
  +---------------------------+     +------------------------+
  | nvme_trace_reader         |     | analyze.py             |
  |  - debugfs/relay 읽기     |     |  - 레이턴시 분석       |
  +------------+--------------+     +-----------+------------+
               | debugfs                        | JSON/CSV
  =============|================================|================
  [ 커널스페이스 ]
               v
  +----------------------------------------------------------+
  | trace_buffer.c  (per-CPU relay 링 버퍼, lock-free)       |
  +------+----------+----------+----------+----------+-------+
         ^          ^          ^          ^          ^
         |          |          |          |          |
  ┌──────┴──┐ ┌────┴────┐ ┌───┴────┐ ┌───┴───┐ ┌───┴────────┐
  │ CQ Poll │ │ IOMMU   │ │ SQ Mon │ │ IRQ   │ │ DMA/MMIO   │
  │ Tracer  │ │ Fault   │ │ (kprobe│ │Hooker │ │ Tracer     │
  │         │ │ Tracer  │ │  보조) │ │(kprobe│ │ (kprobe    │
  │ Phase비트│ │ IOMMU   │ │       │ │  보조)│ │  보조)     │
  │ 직접관찰│ │ PTE조작 │ │       │ │       │ │            │
  └────┬────┘ └────┬────┘ └───┬───┘ └───┬───┘ └─────┬──────┘
       │           │          │         │            │
  ═════╪═══════════╪══════════╪═════════╪════════════╪════════
       v           v          v         v            v
  ┌─────────┐ ┌─────────┐ ┌──────┐ ┌──────┐ ┌───────────┐
  │CQ Memory│ │ IOMMU   │ │ SQ   │ │MSI-X │ │BAR0 MMIO  │
  │(Host RAM│ │Page Tbl │ │Memory│ │ IRQ  │ │DMA Map    │
  │DMA coh.)│ │(VT-d)   │ │      │ │      │ │           │
  └────┬────┘ └────┬────┘ └──┬───┘ └──┬───┘ └─────┬─────┘
       │           │         │        │            │
       v           v         v        v            v
  +----------------------------------------------------------+
  |              PCIe Bus (TLP Transactions)                  |
  +----------------------------------------------------------+
                           |
                           v
  +----------------------------------------------------------+
  |                  NVMe SSD Controller                      |
  +----------------------------------------------------------+
```

## 프로젝트 구조

```
nvme-pcie-tracer/
├── README.md                              ← 이 문서
├── docs/
│   └── 01_pcie_fault_based_detection.md   ← 설계 문서 (5가지 접근법 상세)
├── kernel/                                ← 커널 모듈
│   ├── Makefile
│   ├── nvme_trace_event.h                 ← 공통 이벤트 구조체/타입
│   ├── trace_buffer.c/h                   ← per-CPU relay 링 버퍼
│   │
│   │  ★ PCIe 레벨 직접 탐지 (Phase 1-2) ★
│   ├── nvme_cq_poll_tracer.c              ← CQ 메모리 폴링 (DMA Write 탐지)
│   ├── nvme_iommu_fault_tracer.c          ← IOMMU Fault (PCIe TLP 탐지)
│   │
│   │  보조 모듈 (kprobe, Host→Device 방향)
│   ├── nvme_sq_monitor.c                  ← SQ 커맨드 캡처
│   ├── nvme_doorbell_tracer.c             ← Doorbell 쓰기 캡처
│   ├── nvme_irq_hooker.c                 ← MSI-X 인터럽트 타이밍
│   ├── nvme_mmio_tracer.c                 ← BAR0 레지스터 R/W
│   ├── nvme_dma_tracer.c                  ← DMA map/unmap 추적
│   └── nvme_pcie_tracer.c                 ← 통합 모듈 (init/exit)
├── user/                                  ← 유저스페이스 도구
│   ├── nvme_trace_reader.c                ← relay 버퍼 읽기/표시
│   ├── nvme_trace_decoder.c/h             ← NVMe 커맨드 디코딩
│   └── Makefile
├── bpf/                                   ← BPF 스크립트 (대안)
│   ├── nvme_tracer.bt                     ← bpftrace 전체 트레이서
│   └── nvme_latency.bt                    ← 레이턴시 히스토그램
└── scripts/
    ├── setup.sh                           ← 환경 설정
    ├── start_trace.sh                     ← 트레이싱 시작
    ├── stop_trace.sh                      ← 트레이싱 중지
    └── analyze.py                         ← 분석/시각화
```

## Requirements

- CentOS/Rocky Linux 9 (kernel 5.14+), x86_64 물리 머신
- `kernel-devel` package matching running kernel
- `gcc`, `make`
- Intel VT-d IOMMU (IOMMU Fault 사용 시: `intel_iommu=on`)
- `bpftrace` (BPF 스크립트, optional)
- `python3` with `matplotlib` (시각화, optional)

## Quick Start: IOMMU Fault로 CQ DMA Write 탐지

```bash
# 0. 전제 조건
#    - Intel VT-d IOMMU 활성화: 부트 파라미터에 intel_iommu=on iommu.passthrough=0
#    - 실험용 NVMe (fault 시 컨트롤러 에러 발생, 데이터 손실 가능)
dmesg | grep -i "IOMMU\|DMAR"       # IOMMU 활성 확인
lspci -d ::0108                       # NVMe BDF 확인

# 1. nvme_queue 구조체 오프셋 확인
pahole -C nvme_queue /sys/kernel/btf/vmlinux
# → cq_dma_addr, q_depth, qid 오프셋 메모

# 2. 커널 모듈 빌드
cd kernel && make

# 3. 모듈 로드 (오프셋은 pahole 결과로 대체)
sudo insmod nvme_iommu_fault_tracer.ko \
  target_pci='0000:03:00.0' \
  target_qid=1 \
  off_cq_dma_addr=XX off_q_depth=XX off_qid=XX

# 4. 상태 확인
cat /sys/kernel/debug/nvme-iommu-fault/status

# 5. I/O 발생 → CQ IOVA 자동 캡처
dd if=/dev/nvme0n1 of=/dev/null bs=4k count=1

# 6. CQ IOVA 캡처 확인
cat /sys/kernel/debug/nvme-iommu-fault/status
# → "CQ Captured: YES, CQ IOVA: 0x..."

# 7. ★ IOMMU Fault 유발 (CQ Write 권한 제거) ★
echo arm > /sys/kernel/debug/nvme-iommu-fault/control
# → CQ IOVA가 Read-Only로 변경됨
# → 다음 디바이스 DMA Write → IOMMU fault!

# 8. I/O 발생 → 디바이스가 CQ에 DMA Write 시도 → IOMMU FAULT!
dd if=/dev/nvme0n1 of=/dev/null bs=4k count=1
# → 이 I/O는 실패함 (DMA abort)

# 9. Fault 로그 확인
cat /sys/kernel/debug/nvme-iommu-fault/log
# → IOVA=0x..., Source=03:00.0, Type=Write, Reason=0x05
#    "PTE Write access is not set" → CQ DMA Write가 잡혔다!

# 10. 복원
echo disarm > /sys/kernel/debug/nvme-iommu-fault/control
echo 1 > /sys/class/nvme/nvme0/reset_controller
```

### 보조: CQ 메모리 폴링 (비파괴, 간접 관찰)

```bash
# CQ 폴링은 "메모리 값 변화를 CPU가 읽어서 확인"하는 것이지
# 디바이스의 DMA 접근을 직접 잡는 것이 아니다.
# IOMMU fault와 비교/보조 용도로 사용.

sudo insmod nvme_cq_poll_tracer.ko \
  target_qid=1 \
  off_cqes=XX off_q_depth=XX off_cq_head=XX \
  off_qid=XX off_cq_phase=XX
```

### BPF 스크립트 (커널 모듈 불필요)

```bash
sudo bpftrace bpf/nvme_tracer.bt       # 전체 라이프사이클 트레이서
sudo bpftrace bpf/nvme_latency.bt      # 레이턴시 히스토그램
```

## 이벤트 타입

```
Device → Host (0x02xx) ★ PCIe 직접 탐지 대상 ★
  0x0200  CQ_COMPLETE     CQ에 완료 엔트리 도착 (CQ 폴링으로 탐지)
  0x0201  IRQ_FIRED       MSI-X 인터럽트 발생
  0x0210  DMA_MAP         DMA 매핑 생성 / IOMMU fault
  0x0211  DMA_UNMAP       DMA 매핑 해제

Host → Device (0x01xx) - kprobe 보조
  0x0100  MMIO_READ       BAR0 레지스터 읽기
  0x0101  MMIO_WRITE      BAR0 레지스터 쓰기
  0x0102  DOORBELL_SQ_TAIL  SQ Tail Doorbell 쓰기
  0x0103  DOORBELL_CQ_HEAD  CQ Head Doorbell 쓰기
  0x0110  SQ_SUBMIT       SQ에 커맨드 제출
```

## 출력 형식

```
[Timestamp]        CPU  Dir   Queue  Event      Details
------------------------------------------------------------------------------
[12345.678901234] C03  H->D  SQ[1]  SUBMIT    opcode=Read(0x02) nsid=1 cid=42
                                               slba=0x00001000 nlb=7 (4KB)
[12345.678902100] C03  H->D  DB[1]  SQ_TAIL   tail=15 (old=14)
[12345.679100500] C03  D->H  CQ[1]  COMPLETE  cid=42 status=SUCCESS(0x0000)
                                               sqhd=15 sqid=1 phase=1
                                               [DETECTED_BY_POLL, BEFORE_IRQ]
[12345.679101200] C03  D->H  IRQ    MSI-X     vector=5 irq=87
[12345.679105000] C03  H->D  DB[1]  CQ_HEAD   head=22 (old=21)
```

## 성능 영향

| 모듈 | 메커니즘 | per-event 오버헤드 | IOPS 영향 |
|------|----------|-------------------|-----------|
| CQ 폴링 | 메모리 폴링 | ~10ns (cpu_relax) | CPU 1코어 점유 |
| IOMMU Fault | HW fault | ~5-10μs | 파괴적 (실험용) |
| SQ/IRQ/MMIO (kprobe) | kprobe | ~200ns | 3-8% |
| trace_buffer | relay_write | ~50ns | <0.5% |

## 구현 로드맵

- [x] **Phase 1: IOMMU Fault** (`nvme_iommu_fault_tracer.c`) — 핵심
  - dmar_fault_do_one() kprobe로 모든 IOMMU fault 캡처
  - CQ IOVA의 IOMMU Write 권한 제거로 DMA Write fault 유발
  - Fault Record에서 IOVA, BDF, Reason, Read/Write 추출
  - debugfs arm/disarm 인터페이스
- [x] 보조: CQ 메모리 폴링 (`nvme_cq_poll_tracer.c`) — 비파괴 간접 관찰
- [ ] Phase 2: IOMMU Access/Dirty Bit 추적 (VT-d Scalable Mode) — 비파괴 직접 탐지
- [ ] Phase 3: IOMMU PRI (Page Request Interface) — 비파괴 + 자동 재시도
- [x] 보조: Host→Device kprobe 모듈들 (SQ, Doorbell, MMIO, DMA, IRQ)

자세한 설계: [`docs/01_pcie_fault_based_detection.md`](docs/01_pcie_fault_based_detection.md)

## License

GPL-2.0
