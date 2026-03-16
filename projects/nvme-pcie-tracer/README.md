# nvme-pcie-tracer

NVMe PCIe-Level Bidirectional Command Tracer for Linux.

Captures and decodes all Host-to-Device and Device-to-Host communication on the NVMe PCIe interface using kernel kprobes, relay buffers, and BPF. Designed for physical x86_64 machines running CentOS/Rocky Linux 9.

## Requirements

- CentOS/Rocky Linux 9 (kernel 5.14+)
- `kernel-devel` package matching running kernel
- `gcc`, `make`
- `bpftrace` (for BPF-based tracing, optional)
- `python3` with `matplotlib` (for plot generation, optional)

## Quick Start

```bash
# 1. Setup environment (install deps, check config)
sudo scripts/setup.sh

# 2. Build userspace tools
make user

# 3a. Use bpftrace (no kernel module needed)
sudo bpftrace bpf/nvme_tracer.bt

# 3b. Or use kernel module + reader
sudo scripts/start_trace.sh -r          # auto-detect NVMe device
sudo scripts/start_trace.sh -r 0000:03:00.0  # specify device

# 4. Stop tracing
sudo scripts/stop_trace.sh

# 5. Analyze saved trace
python3 scripts/analyze.py /tmp/nvme_trace_*.log
```

## Architecture

```
+---------------------+     +------------------+
| nvme_trace_reader   |     | analyze.py       |
| (read debugfs/relay)|     | (latency, IOPS)  |
+---------+-----------+     +--------+---------+
          |                          |
==========|==========================|===============  (userspace/kernel)
          | relay channel            | text/CSV
==========|==========================|===============
          |
+---------+--------------------------------------------------+
| nvme-pcie-tracer kernel module                             |
|                                                            |
|  nvme_sq_monitor  --> kprobe: nvme_queue_rq                |
|  nvme_cq_monitor  --> kprobe: nvme_irq (CQ scan)          |
|  nvme_irq_hooker  --> kprobe: nvme_irq                    |
|  nvme_mmio_tracer --> kprobe/mmiotrace                     |
|  nvme_iommu_tracer-> kprobe: dma_map_sg_attrs             |
|  trace_buffer     --> relay per-CPU ring buffers           |
+------------------------------------------------------------+
          |
    PCIe Bus / NVMe SSD
```

## Components

### Kernel Module (`kernel/`)

The kernel module hooks into NVMe driver internals via kprobes to capture:

| Event | Direction | Hook Point | Data Captured |
|-------|-----------|------------|---------------|
| SQ Command Submit | H->D | `nvme_queue_rq` | Full 64-byte SQE |
| CQ Completion | D->H | `nvme_irq` | 16-byte CQE, status, phase |
| SQ Tail Doorbell | H->D | `nvme_write_sq_db` | qid, tail value |
| CQ Head Doorbell | H->D | `nvme_poll_cq` | qid, head value |
| MSI-X Interrupt | D->H | `nvme_irq` | vector, IRQ number |
| DMA Mapping | H->D | `dma_map_sg_attrs` | IOVA, phys addr, size |
| MMIO Register | H->D | driver reg functions | offset, value |

### Userspace Reader (`user/`)

`nvme_trace_reader` reads per-CPU relay buffers from debugfs and displays events with color-coded output.

### BPF Scripts (`bpf/`)

- `nvme_tracer.bt` - Full lifecycle tracer using bpftrace (no kernel module needed)
- `nvme_latency.bt` - Focused latency analysis with periodic histograms

### Scripts (`scripts/`)

- `setup.sh` - Install dependencies and check environment
- `start_trace.sh` - Load module and start tracing
- `stop_trace.sh` - Stop tracing and show statistics
- `analyze.py` - Parse trace output and generate statistics/plots

## Module Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `target_pci` | (required) | PCI BDF of target NVMe device (e.g., `0000:03:00.0`) |
| `buffer_size_kb` | 4096 | Per-CPU relay buffer size in KB |

## Output Format

```
[Timestamp]        CPU  Dir   Queue  Event      Details
------------------------------------------------------------------------------
[12345.678901234] C03  H->D  SQ[1]  SUBMIT    opcode=Read(0x02) nsid=1 cid=42
                                               slba=0x00001000 nlb=7 (4KB)
                                               prp1=0x00000000FE000000
[12345.678902100] C03  H->D  DB[1]  SQ_TAIL   tail=15 (old=14)
[12345.679100500] C03  D->H  CQ[1]  COMPLETE  cid=42 status=SUCCESS(0x0000)
                                               sqhd=15 sqid=1 phase=1
[12345.679101200] C03  D->H  IRQ    MSI-X     vector=5 irq=87
[12345.679105000] C03  H->D  DB[1]  CQ_HEAD   head=22 (old=21)
```

Color coding (terminal only):
- Blue: Host-to-Device (H->D)
- Green: Device-to-Host (D->H)
- Red: Error status
- Yellow: Internal events

## Reader Options

```
nvme_trace_reader [OPTIONS]
  -f          Follow mode (continuous, like tail -f)
  -o FILE     Save output to file
  -q QID      Filter by queue ID
  -t TYPE     Filter by event type (SUBMIT, COMPLETE, IRQ, DOORBELL, MMIO, DMA)
  -n          Disable color output
  -c NCPUS    Number of CPU channels to monitor
```

## Analysis Script

```bash
# Basic analysis
python3 scripts/analyze.py trace.log

# With plots
python3 scripts/analyze.py trace.log -o ./plots/

# Export to CSV
python3 scripts/analyze.py trace.log --csv stats.csv

# Text-only (no matplotlib needed)
python3 scripts/analyze.py trace.log --text
```

Output includes:
- Per-queue latency statistics (avg, min, max, p50, p99)
- Per-opcode latency breakdown
- IOPS and bandwidth per queue
- Latency histograms (text or matplotlib)
- Event timeline plot
- Per-queue latency box plots

## Event Types

```
Host -> Device (0x01xx):
  0x0100  MMIO_READ           BAR0 register read
  0x0101  MMIO_WRITE          BAR0 register write
  0x0102  DOORBELL_SQ_TAIL    SQ Tail Doorbell write
  0x0103  DOORBELL_CQ_HEAD    CQ Head Doorbell write
  0x0110  SQ_SUBMIT           Command submitted to SQ

Device -> Host (0x02xx):
  0x0200  CQ_COMPLETE         Completion entry in CQ
  0x0201  IRQ_FIRED           MSI-X interrupt
  0x0210  DMA_MAP             DMA mapping created
  0x0211  DMA_UNMAP           DMA mapping released
```

## PCIe-Level Detection (Page Fault 기반)

kprobe 방식은 커널 함수 호출을 잡는 **간접적** 방법이다. Device→Host DMA 접근을 **PCIe 레벨**에서 직접 탐지하려면 page fault 메커니즘을 사용한다.

자세한 설계: [`docs/01_pcie_fault_based_detection.md`](docs/01_pcie_fault_based_detection.md)

### 접근법 비교

| 접근법 | 파일 | 탐지 수준 | 투명성 | 하드웨어 요구 |
|--------|------|-----------|--------|---------------|
| kprobe (기존) | `nvme_*_monitor.c` | 커널 함수 | 투명 | 없음 |
| **CQ 폴링** | `nvme_cq_poll_tracer.c` | **PCIe DMA Write** | 투명 | 없음 |
| **IOMMU Fault** | `nvme_iommu_fault_tracer.c` | **PCIe TLP** | 파괴적 | VT-d |
| IOMMU A/D Bit | (Phase 3) | PCIe DMA | 투명 | VT-d Scalable |
| IOMMU PRI | (Phase 4) | PCIe TLP | 투명 | VT-d + ATS/PRI |

```
탐지 정밀도:

  kprobe (간접)     IOMMU Fault (직접)     CQ 폴링 (직접)
  ┌────────┐        ┌────────┐             ┌────────┐
  │nvme_irq│ ←IRQ─ │IOMMU   │ ←PCIe TLP─ │CQ 메모리│ ← DMA Write
  │  호출  │        │Fault   │             │Phase변화│
  └────────┘        └────────┘             └────────┘
  t3 (늦음)         t1 (정확)              t0 (가장 빠름)
```

### Quick Start: CQ 폴링 (Phase 1)

```bash
# nvme_queue 오프셋 확인
pahole -C nvme_queue /sys/kernel/btf/vmlinux

# 모듈 로드 (오프셋은 커널 버전에 따라 다름)
sudo insmod kernel/nvme_cq_poll_tracer.ko \
  target_qid=1 \
  off_cqes=XX off_q_depth=XX off_cq_head=XX \
  off_qid=XX off_cq_phase=XX

# I/O 발생시켜 nvme_queue 정보 캡처
dd if=/dev/nvme0n1 of=/dev/null bs=4k count=1

# 캡처 확인
cat /sys/kernel/debug/nvme-cq-poll/info_captured

# 폴링 시작 (debugfs 또는 코드에서)
```

### Quick Start: IOMMU Fault (Phase 2)

```bash
# IOMMU 활성화 확인
dmesg | grep -i "IOMMU\|DMAR"

# 모듈 로드 (관찰 모드)
sudo insmod kernel/nvme_iommu_fault_tracer.ko \
  target_pci='0000:03:00.0'

# Fault 로그 확인
cat /sys/kernel/debug/nvme-iommu-fault/fault_log
cat /sys/kernel/debug/nvme-iommu-fault/iommu_info
```

## Performance Impact

With kprobe-based tracing only (no mmiotrace):
- Per-event overhead: ~200ns
- Expected IOPS reduction: 3-8%
- Per-CPU relay buffers are lock-free

mmiotrace (PTE fault based) adds ~5-10us per MMIO access and is intended for initialization debugging only.

## License

GPL-2.0
