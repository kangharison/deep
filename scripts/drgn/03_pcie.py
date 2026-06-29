#!/usr/bin/env drgn
# -*- coding: utf-8 -*-
#
# [한국어] 03_pcie.py — NVMe 컨트롤러의 PCIe(struct pci_dev) 실시간 관찰
#
# === 이 스크립트의 역할 ===
# nvme0n1 을 떠받치는 PCIe 함수(struct pci_dev)를 찾아, 커널이 그 디바이스에 대해
# 캐싱하고 있는 PCIe 메타데이터 — 벤더/디바이스 ID, 클래스, BAR(리소스) 영역,
# IRQ/MSI-X 상태, PCIe capability 위치, 전력 상태 — 를 메모리에서 직접 읽는다.
#
# === 따라가는 경로 ===
#   struct nvme_dev .dev (struct device *)  ──container_of──►  struct pci_dev *
#       (nvme_dev.dev 는 &pdev->dev 를 가리킨다. to_pci_dev() 와 동일한 복원)
#
# === 읽을 수 있는 것 / 없는 것 ===
#   읽기 O : pci_dev 구조체에 커널이 '캐싱'한 값들(vendor/device/class/resource[]/irq/
#            msix_enabled/pcie_cap 오프셋/current_state 등) — 전부 호스트 RAM.
#   읽기 X : 라이브 PCIe config space 레지스터(BAR0 실제 내용, 링크 상태 LNKSTA 등).
#            이건 config 사이클(MMCONFIG MMIO 또는 0xCF8/0xCFC 포트)로만 접근 가능 →
#            /proc/kcore(RAM)로는 안 읽힌다. 캐싱된 미러값으로 갈음한다.
#
# 실행: sudo -E drgn 03_pcie.py

from drgn import Object, cast, container_of, FaultError, program_from_kernel
from drgn.helpers.linux.block import for_each_disk, disk_name
from drgn.helpers.linux.pci import pci_name, for_each_pci_dev

try:
    prog
except NameError:
    prog = program_from_kernel()

TARGET = b"nvme0n1"

# [한국어] struct resource.flags 의 주요 비트(include/linux/ioport.h). BAR 종류 판별용.
IORESOURCE_MEM = 0x00000200
IORESOURCE_IO = 0x00000100
IORESOURCE_MEM_64 = 0x00100000
IORESOURCE_PREFETCH = 0x00002000


def find_disk(name):
    for disk in for_each_disk(prog):
        if disk_name(disk) == name:
            return disk
    return None


def get_pci_dev(disk):
    """[한국어] gendisk → nvme_ns → nvme_ctrl → nvme_dev → pci_dev 복원."""
    ns = cast("struct nvme_ns *", disk.private_data)
    ctrl = ns.ctrl
    nvme_dev = container_of(ctrl, "struct nvme_dev", "ctrl")
    # [한국어] nvme_dev.dev 는 struct device*. pci_dev 안의 .dev 멤버를 가리키므로 역복원.
    pdev = container_of(nvme_dev.dev, "struct pci_dev", "dev")
    return nvme_dev, pdev


def decode_res_flags(flags):
    t = []
    if flags & IORESOURCE_MEM:
        t.append("MEM")
    if flags & IORESOURCE_IO:
        t.append("IO")
    if flags & IORESOURCE_MEM_64:
        t.append("64bit")
    if flags & IORESOURCE_PREFETCH:
        t.append("prefetch")
    return ",".join(t) if t else "-"


disk = find_disk(TARGET)
if disk is None:
    print(f"[ERR] {TARGET.decode()} 없음")
else:
    nvme_dev, pdev = get_pci_dev(disk)
    name = pci_name(pdev).decode()                 # 예: "0000:00:0e.0"
    vendor = int(pdev.vendor)                       # u16, 예: 0x80ee(VirtualBox), 0x1b36(QEMU)
    device = int(pdev.device)                       # u16
    klass = int(pdev["class"])                      # u24<<8; 0x010802 = NVMe(=Mass storage/NVM/NVMe)
    revision = int(pdev.revision) if hasattr(pdev, "revision") else -1
    subv = int(pdev.subsystem_vendor)
    subd = int(pdev.subsystem_device)
    irq = int(pdev.irq)

    print(f"PCIe 함수: {name}")
    print(f"  vendor:device = {vendor:#06x}:{device:#06x}  (rev {revision:#x})")
    print(f"  subsystem     = {subv:#06x}:{subd:#06x}")
    print(f"  class         = {klass:#08x}  (0x010802 = NVMe SSD)")
    print(f"  legacy IRQ    = {irq}")
    # [한국어] msi/msix 활성 여부: NVMe 는 보통 MSI-X 로 큐별 인터럽트를 받는다.
    msix = int(pdev.msix_enabled) if hasattr(pdev, "msix_enabled") else -1
    msi = int(pdev.msi_enabled) if hasattr(pdev, "msi_enabled") else -1
    print(f"  msi_enabled={msi}  msix_enabled={msix}")
    # [한국어] pcie_cap: PCI Express capability 구조체의 config space 내 오프셋(u8).
    #  0 이 아니면 PCIe 디바이스. msix_cap/msi_cap 도 각 capability 오프셋.
    print(f"  cap offsets   : pcie={int(pdev.pcie_cap):#x} "
          f"msix={int(pdev.msix_cap):#x} msi={int(pdev.msi_cap):#x}")
    # [한국어] current_state: enum pci_power_state (PCI_D0=0 이면 완전 가동).
    print(f"  power state   : {str(pdev.current_state)}")

    # [한국어] BAR 영역(resource[0..5]): NVMe 는 BAR0 가 컨트롤러 레지스터(MMIO).
    #  start/end 는 호스트 물리 주소 공간상의 MMIO 윈도우. 이 주소를 ioremap 한 게 nvme_dev.bar.
    print("\n  BAR(resource[0..5]):")
    for i in range(6):
        r = pdev.resource[i]
        start = int(r.start)
        end = int(r.end)
        flags = int(r.flags)
        if start == 0 and end == 0:
            continue
        size = end - start + 1
        print(f"    BAR{i}: {start:#012x}-{end:#012x} size={size:#x} "
              f"[{decode_res_flags(flags)}]")

    print("\n  ── nvme_dev 쪽 연결 ──")
    print(f"    nvme_dev.bar(ioremap된 BAR0 가상주소) = {hex(nvme_dev.bar.value_())}")
    print(f"    nvme_dev.dbs(도어벨 베이스)           = {hex(nvme_dev.dbs.value_())}")

    print("\n[안내] BAR0 가 가리키는 실제 컨트롤러 레지스터(CAP/CC/CSTS/AQA 등)와")
    print("       라이브 PCIe config(LNKSTA 링크속도 등)는 MMIO/config 사이클이라 메모리로는 못 읽는다.")
    print("       위 값들은 커널이 RAM 에 캐싱한 메타데이터다.")

    # [한국어] (보너스) 전체 PCIe 디바이스 열거 — for_each_pci_dev 헬퍼 데모.
    print("\n  (참고) 시스템 PCIe 디바이스 일부:")
    cnt = 0
    for d in for_each_pci_dev(prog):
        print(f"    {pci_name(d).decode()}  {int(d.vendor):#06x}:{int(d.device):#06x}")
        cnt += 1
        if cnt >= 8:
            print("    ...")
            break
