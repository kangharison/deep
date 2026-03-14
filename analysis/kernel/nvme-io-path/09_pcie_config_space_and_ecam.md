# PCIe Config Space와 ECAM: BAR MMIO와는 다른 또 하나의 접근 경로

> Config Space는 BAR와는 **완전히 별개의 메커니즘**이다. BAR는 디바이스가 자신의 레지스터를 노출하는 것이고, Config Space는 PCIe 표준이 **모든 디바이스에 공통으로 부여하는 관리 영역**이다. 이 둘을 혼동하는 것이 PCIe를 이해하는 데 가장 큰 장벽이다.

---

## 1. 핵심 개념: 두 가지 MMIO 영역의 구분

```
  호스트 물리 주소 공간
  ┌──────────────────────────────────────────────────────────────────┐
  │                                                                  │
  │   RAM (0x0000_0000 ~ ...)                                       │
  │                                                                  │
  ├──────────────────────────────────────────────────────────────────┤
  │                                                                  │
  │   ① ECAM 영역 (Config Space MMIO)         예: 0xB000_0000      │
  │   ┌────────────────────────────────────────────────────┐        │
  │   │ Bus 0, Dev 0, Func 0의 Config Space (4KB)          │        │
  │   │ Bus 0, Dev 0, Func 1의 Config Space (4KB)          │        │
  │   │ ...                                                 │        │
  │   │ Bus 0, Dev 31, Func 7의 Config Space (4KB)         │        │
  │   │ Bus 1, Dev 0, Func 0의 Config Space (4KB)  ← NVMe │        │
  │   │ ...                                                 │        │
  │   │ Bus 255, Dev 31, Func 7의 Config Space (4KB)       │        │
  │   └────────────────────────────────────────────────────┘        │
  │   총 크기: 256 buses × 32 devs × 8 funcs × 4KB = 256MB         │
  │                                                                  │
  ├──────────────────────────────────────────────────────────────────┤
  │                                                                  │
  │   ② BAR MMIO 영역들 (디바이스별 레지스터)                       │
  │   ┌──────────────────┐                                          │
  │   │ NVMe BAR0         │  예: 0xFB00_0000 (컨트롤러 레지스터)    │
  │   │ (16KB)            │  CC, CSTS, Doorbell 등                  │
  │   └──────────────────┘                                          │
  │   ┌──────────────────┐                                          │
  │   │ GPU BAR0          │  예: 0xFC00_0000 (VRAM/MMIO)            │
  │   └──────────────────┘                                          │
  │                                                                  │
  └──────────────────────────────────────────────────────────────────┘

  ① ECAM: BIOS/펌웨어가 고정 위치에 배치, 모든 디바이스의 Config Space를 한 곳에 모음
  ② BAR:  각 디바이스가 자신의 레지스터를 개별적으로 노출
```

**핵심 차이**:

| 구분 | Config Space (ECAM) | BAR MMIO |
|------|-------------------|----------|
| 용도 | 디바이스 식별, 설정, BAR 주소 할당 | 디바이스 고유 레지스터 접근 |
| 크기 | 디바이스당 4KB (고정) | 디바이스마다 다름 (NVMe: ~16KB) |
| 주소 결정 | BIOS가 ECAM base를 결정 (ACPI MCFG) | BIOS가 BAR에 주소를 기록 |
| 접근 주체 | 주로 OS의 PCI 서브시스템 (드라이버 초기화) | 디바이스 드라이버 (런타임) |
| 접근 빈도 | 초기화/설정 시 (드물게) | I/O마다 (매우 빈번) |
| 내용 | Vendor/Device ID, BAR 값, Capability 등 | NVMe의 경우 CC, CSTS, Doorbell 등 |
| PCIe TLP | Config Read/Write TLP | Memory Read/Write TLP |

---

## 2. Config Space 구조: 표준 256바이트 + Extended 4KB

### 2.1 레거시 PCI Config Space (0x00 ~ 0xFF, 256바이트)

```
  오프셋   크기   레지스터                  설명
  ┌──────────────────────────────────────────────────────────────────┐
  │ 0x00    2B    Vendor ID                제조사 식별 (NVMe SSD: 삼성=0x144D)  │
  │ 0x02    2B    Device ID                제품 식별                              │
  │ 0x04    2B    Command                  디바이스 활성화 (I/O, Memory, Bus Master)│
  │ 0x06    2B    Status                   에러, 인터럽트 상태                    │
  │ 0x08    1B    Revision ID              하드웨어 리비전                        │
  │ 0x09    3B    Class Code               디바이스 유형 (NVMe=0x010802)          │
  │ 0x0C    1B    Cache Line Size                                                │
  │ 0x0D    1B    Latency Timer                                                  │
  │ 0x0E    1B    Header Type              0=일반, 1=브리지, 2=CardBus           │
  │ 0x0F    1B    BIST                     Built-In Self Test                    │
  │                                                                              │
  │ 0x10    4B    ★ BAR0 (low 32bit)      ← NVMe 컨트롤러 레지스터 주소!       │
  │ 0x14    4B    ★ BAR1 (high 32bit)     ← BAR0이 64비트이면 여기에 상위 32bit │
  │ 0x18    4B    BAR2                                                           │
  │ 0x1C    4B    BAR3                                                           │
  │ 0x20    4B    BAR4                                                           │
  │ 0x24    4B    BAR5                                                           │
  │                                                                              │
  │ 0x28    4B    CardBus CIS Pointer                                            │
  │ 0x2C    2B    Subsystem Vendor ID                                            │
  │ 0x2E    2B    Subsystem ID                                                   │
  │ 0x30    4B    Expansion ROM Base                                             │
  │ 0x34    1B    ★ Capabilities Pointer  ← Capability 체인 시작 오프셋         │
  │ 0x3C    1B    Interrupt Line                                                 │
  │ 0x3D    1B    Interrupt Pin                                                  │
  │ 0x3E    1B    Min Grant                                                      │
  │ 0x3F    1B    Max Latency                                                    │
  │                                                                              │
  │ 0x40~   ??    Capability 구조체 체인   MSI, MSI-X, Power Mgmt, PCIe 등      │
  │ ~0xFF                                                                        │
  └──────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 PCIe Extended Config Space (0x100 ~ 0xFFF, 3840바이트 추가)

```
  ┌──────────────────────────────────────────────────────────────────┐
  │ 0x100~  Extended Capability 체인                                 │
  │         ┌──────────────────────────────────────────────┐        │
  │         │ AER (Advanced Error Reporting)                │        │
  │         │ - 0x100: AER Capability Header                │        │
  │         │ - 0x104: Uncorrectable Error Status           │        │
  │         │ - 0x108: Uncorrectable Error Mask             │        │
  │         │ - ...                                         │        │
  │         └──────────────────────────────────────────────┘        │
  │         ┌──────────────────────────────────────────────┐        │
  │         │ Serial Number                                 │        │
  │         └──────────────────────────────────────────────┘        │
  │         ┌──────────────────────────────────────────────┐        │
  │         │ L1 PM Substates                               │        │
  │         └──────────────────────────────────────────────┘        │
  │         ┌──────────────────────────────────────────────┐        │
  │         │ Secondary PCI Express                         │        │
  │         └──────────────────────────────────────────────┘        │
  │         ...                                                      │
  │ ~0xFFF                                                           │
  └──────────────────────────────────────────────────────────────────┘

  각 Extended Capability의 헤더 (4바이트):
  ┌─────────────────┬────────────────┬──────────────────┐
  │ Capability ID    │ Version        │ Next Offset      │
  │ [15:0]           │ [19:16]        │ [31:20]          │
  └─────────────────┴────────────────┴──────────────────┘
  → Next Offset으로 체인을 따라가며 원하는 Capability를 찾는다
```

### 2.3 Config Space와 BAR의 관계

```
  Config Space (0x10~0x27)에 BAR 값이 저장되어 있다.
  BAR 값이 곧 MMIO 영역의 물리 주소이다.

  ┌─────────────────────────────┐
  │ Config Space                │
  │                             │
  │ offset 0x10: BAR0           │──── 값: 0xFB000004 (하위 비트=속성)
  │              ↓              │         물리 주소: 0xFB000000
  │              ↓              │
  │ offset 0x14: BAR1           │──── 값: 0x00000000 (64비트 BAR의 상위)
  └─────────────────────────────┘
                 │
                 │  이 값이 가리키는 물리 주소에
                 │  NVMe 컨트롤러 레지스터가 매핑된다
                 │
                 ▼
  ┌─────────────────────────────┐
  │ 물리 주소 0xFB000000        │
  │ NVMe BAR0 MMIO 영역        │
  │  0x0000: CAP                │
  │  0x0014: CC                 │
  │  0x1000: Doorbell           │
  └─────────────────────────────┘

  즉: Config Space에서 BAR 주소를 읽은 후 → 그 주소를 ioremap() → BAR MMIO 접근
      Config Space 접근과 BAR MMIO 접근은 완전히 별개의 메커니즘!
```

---

## 3. Config Space 접근 방법의 진화

### 3.1 레거시 PCI: I/O 포트 (0xCF8 / 0xCFC)

초기 PCI에서는 Config Space에 접근하기 위해 **x86 I/O 포트**를 사용했다. MMIO가 아니라 IN/OUT 명령어다.

```
  ┌──────────────────────────────────────────────────────────────┐
  │  방법 1: PCI Configuration Mechanism Type 1 (레거시)         │
  │                                                              │
  │  I/O 포트 0xCF8 = Config Address Register                   │
  │  I/O 포트 0xCFC = Config Data Register                      │
  │                                                              │
  │  접근 절차:                                                  │
  │  1) 0xCF8에 대상 디바이스 + 레지스터 오프셋을 쓴다          │
  │  2) 0xCFC에서 데이터를 읽거나 쓴다                          │
  │                                                              │
  │  Config Address 레지스터 포맷 (32비트):                      │
  │  ┌──┬────────┬─────┬──────┬──────────┬──┐                  │
  │  │31│30:24   │23:16│15:11 │10:8      │7:2│1:0               │
  │  │EN│Reserved│Bus  │Device│Function  │Reg│ 00               │
  │  └──┴────────┴─────┴──────┴──────────┴──┘                  │
  │  EN=1: Config 접근 활성화                                    │
  │                                                              │
  │  예: NVMe (Bus=1, Dev=0, Func=0)의 Vendor ID (reg=0x00) 읽기│
  │  outl(0x80010000, 0xCF8)  ← Bus=1, Dev=0, Func=0, Reg=0    │
  │  val = inl(0xCFC)         ← Vendor ID + Device ID 반환      │
  └──────────────────────────────────────────────────────────────┘
```

**Linux 커널 코드** (`arch/x86/pci/direct.c`):
```c
#define PCI_CONF1_ADDRESS(bus, devfn, reg) \
    (0x80000000 | ((reg & 0xF00) << 16) | (bus << 16) \
    | (devfn << 8) | (reg & 0xFC))

static int pci_conf1_read(unsigned int seg, unsigned int bus,
                          unsigned int devfn, int reg, int len, u32 *value)
{
    raw_spin_lock_irqsave(&pci_config_lock, flags);

    /* ① 0xCF8에 대상 주소를 쓴다 (Bus:Dev:Func:Reg) */
    outl(PCI_CONF1_ADDRESS(bus, devfn, reg), 0xCF8);

    /* ② 0xCFC에서 데이터를 읽는다 */
    switch (len) {
    case 1: *value = inb(0xCFC + (reg & 3)); break;
    case 2: *value = inw(0xCFC + (reg & 2)); break;
    case 4: *value = inl(0xCFC);             break;
    }

    raw_spin_unlock_irqrestore(&pci_config_lock, flags);
    return 0;
}
```

**문제점**:
- 0x00~0xFF (256바이트)만 접근 가능 → Extended Config Space (0x100~0xFFF) 접근 불가
- I/O 포트는 직렬화 필요 (spin_lock) → 성능 병목
- x86 전용 메커니즘

### 3.2 PCIe ECAM: Config Space도 MMIO로! (현대 방식)

PCIe에서는 Config Space도 **메모리 매핑 (MMIO)**으로 접근한다. 이를 **ECAM (Enhanced Configuration Access Mechanism)**이라 한다.

```
  ┌──────────────────────────────────────────────────────────────┐
  │  방법 2: PCIe ECAM (Enhanced Configuration Access Mechanism) │
  │                                                              │
  │  BIOS가 ACPI MCFG 테이블에 ECAM base 주소를 기록            │
  │  OS가 이 주소를 ioremap()하면 모든 디바이스의 Config Space에 │
  │  일반 메모리 접근(readl/writel)으로 접근할 수 있다            │
  │                                                              │
  │  ECAM 주소 계산:                                             │
  │  addr = ECAM_base + (Bus << 20) + (Device << 15)             │
  │                    + (Function << 12) + Register_offset       │
  │                                                              │
  │  ┌──────────────────────────────────────────────────┐        │
  │  │ ECAM_base + Bus[7:0] │ Dev[4:0]│Func[2:0]│Reg[11:0]│     │
  │  │           │ [27:20]   │[19:15]  │[14:12]  │[11:0]   │     │
  │  └──────────────────────────────────────────────────┘        │
  │                                                              │
  │  각 Function당 4KB (2^12 = 4096바이트)                       │
  │  각 Bus당: 32 devices × 8 functions × 4KB = 1MB (2^20)      │
  │  256 buses: 256 × 1MB = 256MB (ECAM 전체 크기)              │
  │                                                              │
  │  예: NVMe (Bus=1, Dev=0, Func=0)의 BAR0 (reg=0x10) 읽기     │
  │  addr = ECAM_base + (1 << 20) + (0 << 15) + (0 << 12) + 0x10│
  │       = ECAM_base + 0x100010                                 │
  │  val = readl(ecam_virt + 0x100010)  ← 일반 MMIO 읽기!       │
  └──────────────────────────────────────────────────────────────┘
```

**Linux 커널 코드** (`arch/x86/pci/mmconfig_64.c`):
```c
/* PCI_MMCFG_BUS_OFFSET = bus << 20 (각 bus당 1MB) */
#define PCI_MMCFG_BUS_OFFSET(bus)  ((bus) << 20)

/* ECAM 영역에서 특정 디바이스의 Config Space 시작 주소를 계산 */
static char __iomem *pci_dev_base(unsigned int seg, unsigned int bus,
                                  unsigned int devfn)
{
    struct pci_mmcfg_region *cfg = pci_mmconfig_lookup(seg, bus);

    if (cfg && cfg->virt)
        /* cfg->virt = ioremap(ECAM_base_phys)
         * PCI_MMCFG_BUS_OFFSET(bus) = bus << 20 (1MB per bus)
         * devfn << 12 = 4KB per function
         * 결과: 해당 디바이스의 4KB Config Space 시작 가상 주소 */
        return cfg->virt + (PCI_MMCFG_BUS_OFFSET(bus) | (devfn << 12));
    return NULL;
}

/* Config Space 읽기 - 그냥 MMIO readl! */
static int pci_mmcfg_read(unsigned int seg, unsigned int bus,
                          unsigned int devfn, int reg, int len, u32 *value)
{
    char __iomem *addr;

    addr = pci_dev_base(seg, bus, devfn);
    /* addr + reg: 해당 디바이스의 Config Space 내 특정 레지스터 주소
     * 그냥 mmio_config_readl()로 읽는다! (= readl()과 유사)
     * lock 불필요! (I/O 포트 방식과 달리) */
    switch (len) {
    case 1: *value = mmio_config_readb(addr + reg); break;
    case 2: *value = mmio_config_readw(addr + reg); break;
    case 4: *value = mmio_config_readl(addr + reg); break;
    }
    return 0;
}
```

### 3.3 ECAM 초기화 과정

```
  부팅 시:
  ┌─────────────────────────────────────────────────────────────┐
  │ BIOS/UEFI                                                   │
  │                                                             │
  │ 1. ECAM 영역의 물리 주소를 결정 (예: 0xB0000000)            │
  │ 2. ACPI MCFG 테이블에 이 주소를 기록:                       │
  │    - Base Address: 0xB0000000                               │
  │    - PCI Segment Group: 0                                   │
  │    - Start Bus: 0                                           │
  │    - End Bus: 255                                           │
  │ 3. PCIe Root Complex의 ECAM 디코더에도 이 주소를 설정       │
  └──────────────┬──────────────────────────────────────────────┘
                 │
                 ▼
  ┌─────────────────────────────────────────────────────────────┐
  │ Linux 커널                                                  │
  │                                                             │
  │ 1. ACPI MCFG 테이블 파싱:                                   │
  │    pci_mmcfg_late_init()                                    │
  │      → acpi_sfi_table_parse(ACPI_SIG_MCFG, ...)            │
  │      → ECAM base = 0xB0000000, buses 0~255                 │
  │                                                             │
  │ 2. ECAM 영역을 ioremap():                                   │
  │    mcfg_ioremap(cfg)                                        │
  │      → ioremap(0xB0000000, 256MB)                           │
  │      → cfg->virt = 커널 가상 주소                           │
  │                                                             │
  │ 3. raw_pci_ext_ops = &pci_mmcfg;                            │
  │    → 이후 pci_read_config_dword() 등이 ECAM MMIO를 사용     │
  └─────────────────────────────────────────────────────────────┘
```

---

## 4. QEMU에서의 ECAM 에뮬레이션

### 4.1 QEMU PCIe Host Bridge가 ECAM을 제공하는 방식

```c
/* hw/pci/pcie_host.c */

/* ECAM MMIO 핸들러 정의 */
static const MemoryRegionOps pcie_mmcfg_ops = {
    .read = pcie_mmcfg_data_read,    /* Guest가 ECAM에서 읽을 때 */
    .write = pcie_mmcfg_data_write,  /* Guest가 ECAM에 쓸 때 */
    .endianness = DEVICE_LITTLE_ENDIAN,
};

/* ECAM MemoryRegion 초기화 */
static void pcie_host_init(Object *obj)
{
    PCIExpressHost *e = PCIE_HOST_BRIDGE(obj);

    /* ECAM용 MemoryRegion을 생성.
     * 크기: PCIE_MMCFG_SIZE_MAX = 256MB (1 << 28)
     * 이 MemoryRegion에 접근하면 pcie_mmcfg_ops 핸들러가 호출된다 */
    memory_region_init_io(&e->mmio, OBJECT(e), &pcie_mmcfg_ops, e,
                          "pcie-mmcfg-mmio", PCIE_MMCFG_SIZE_MAX);
}

/* ECAM을 Guest 물리 주소 공간에 매핑 */
void pcie_host_mmcfg_map(PCIExpressHost *e, hwaddr addr, uint32_t size)
{
    e->base_addr = addr;  /* 예: 0xB0000000 */
    /* Guest의 시스템 메모리 공간에 ECAM MemoryRegion을 추가.
     * Guest가 addr(0xB0000000) ~ addr+size에 접근하면
     * QEMU의 pcie_mmcfg_data_read/write가 호출된다 */
    memory_region_add_subregion(get_system_memory(), e->base_addr, &e->mmio);
}
```

### 4.2 Guest가 ECAM에 접근하면 일어나는 일

```
  Guest 커널: pci_read_config_dword(pdev, PCI_VENDOR_ID, &val)
  │
  │  내부적으로 ECAM 방식이면:
  │  → mmio_config_readl(ecam_virt + bus<<20 + devfn<<12 + 0x00)
  │  → readl(가상 주소)
  │
  ▼
  Guest CPU: MOV reg, [ECAM 가상 주소]
  │
  ▼ MMU: 가상→물리 변환 → ECAM 물리 주소 (예: 0xB0100000)
  │
  ▼ KVM: EPT violation (이 물리 주소는 RAM이 아님!)
  │       VM-Exit → QEMU로 제어권 전달
  │
  ▼ QEMU: address_space_rw()
  │       → FlatView에서 이 물리 주소가 어느 MemoryRegion인지 검색
  │       → "pcie-mmcfg-mmio" MemoryRegion에 매칭!
  │       → 오프셋 = 물리주소 - ECAM_base = 0xB0100000 - 0xB0000000 = 0x100000
  │
  ▼ QEMU: pcie_mmcfg_data_read(opaque, mmcfg_addr=0x100000, len=4)
```

### 4.3 ECAM 주소 → Bus:Dev:Func:Reg 디코딩 (QEMU)

```c
/* include/hw/pci/pcie_host.h */
#define PCIE_MMCFG_BUS_BIT       20     /* Bus 번호: bit [27:20] */
#define PCIE_MMCFG_DEVFN_BIT     12     /* DevFn: bit [19:12] */
#define PCIE_MMCFG_CONFOFFSET_MASK 0xfff /* 레지스터 오프셋: bit [11:0] */

#define PCIE_MMCFG_BUS(addr)        (((addr) >> 20) & 0xFF)
#define PCIE_MMCFG_DEVFN(addr)      (((addr) >> 12) & 0xFF)
#define PCIE_MMCFG_CONFOFFSET(addr) ((addr) & 0xFFF)

/* hw/pci/pcie_host.c */
static uint64_t pcie_mmcfg_data_read(void *opaque, hwaddr mmcfg_addr,
                                     unsigned len)
{
    PCIExpressHost *e = opaque;
    PCIBus *s = e->pci.bus;

    /* ① ECAM 오프셋에서 Bus:DevFn을 추출 */
    /* mmcfg_addr = 0x100000 이면:
     *   Bus  = (0x100000 >> 20) & 0xFF = 1
     *   DevFn = (0x100000 >> 12) & 0xFF = 0  (Dev=0, Func=0)
     *   → Bus 1, Dev 0, Func 0 = NVMe SSD! */
    PCIDevice *pci_dev = pcie_dev_find_by_mmcfg_addr(s, mmcfg_addr);
    if (!pci_dev) return ~0x0;  /* 디바이스 없으면 0xFFFFFFFF 반환 */

    /* ② 레지스터 오프셋 추출 */
    uint32_t addr = PCIE_MMCFG_CONFOFFSET(mmcfg_addr);
    /* mmcfg_addr = 0x100010 이면: addr = 0x010 = BAR0 */

    uint32_t limit = pci_config_size(pci_dev);  /* 256 또는 4096 */

    /* ③ 해당 디바이스의 Config Space 배열에서 값을 읽어 반환 */
    return pci_host_config_read_common(pci_dev, addr, limit, len);
    /* 내부적으로: pci_dev->config[addr] 배열에서 값을 읽는다.
     * 이 배열이 디바이스의 Config Space를 소프트웨어적으로 에뮬레이션한다. */
}

static void pcie_mmcfg_data_write(void *opaque, hwaddr mmcfg_addr,
                                  uint64_t val, unsigned len)
{
    /* 같은 방식으로 Bus:DevFn:Reg를 추출하고 */
    PCIDevice *pci_dev = pcie_dev_find_by_mmcfg_addr(s, mmcfg_addr);
    uint32_t addr = PCIE_MMCFG_CONFOFFSET(mmcfg_addr);

    /* Config Space에 값을 쓴다.
     * 특수 레지스터(BAR, Command 등)는 write 콜백이 트리거된다 */
    pci_host_config_write_common(pci_dev, addr, limit, val, len);
}
```

### 4.4 QEMU에서 Config Space가 저장되는 위치

```c
/* include/hw/pci/pci_device.h */
struct PCIDevice {
    ...
    /* ★ Config Space는 이 배열에 저장된다 ★ */
    uint8_t *config;        /* 현재 Config Space 값 (4096 bytes) */

    /* config_read/config_write 콜백:
     * 특정 레지스터에 쓸 때 추가 동작이 필요한 경우 사용
     * 예: BAR에 쓰면 MemoryRegion 매핑이 변경됨
     * 예: Command 레지스터에 쓰면 I/O/Memory 활성화 상태 변경 */
    PCIConfigReadFunc *config_read;
    PCIConfigWriteFunc *config_write;

    uint8_t *wmask;         /* Write mask (쓰기 가능 비트 표시) */
    uint8_t *cmask;         /* Config Space 변경 마스크 */
    uint8_t *w1cmask;       /* Write-1-to-clear 마스크 */
    uint8_t *used;          /* 사용 중인 바이트 표시 */
    ...
};

/* NVMe 디바이스 초기화 시 Config Space 설정 (hw/nvme/ctrl.c) */
static void nvme_init_pci(NvmeCtrl *n, PCIDevice *pci_dev)
{
    /* Vendor ID, Device ID 등은 QEMU 인프라가 자동 설정하거나
     * 디바이스 realize 시 명시적으로 설정한다 */
    pci_config_set_vendor_id(pci_dev->config, PCI_VENDOR_ID_INTEL);
    pci_config_set_device_id(pci_dev->config, 0x5845);
    pci_config_set_class(pci_dev->config, PCI_CLASS_STORAGE_EXPRESS);
    /* → config[0x00] = 0x86, config[0x01] = 0x80 (Intel)
     *   config[0x02] = 0x45, config[0x03] = 0x58
     *   config[0x09] = 0x02, config[0x0A] = 0x08, config[0x0B] = 0x01 */
}
```

---

## 5. BAR 주소 할당 과정 (Config Space 경유)

### 5.1 BIOS/Guest OS가 BAR에 주소를 할당하는 메커니즘

```
  ┌─────────────────────────────────────────────────────────────────┐
  │ BAR 크기 결정 (BAR Sizing)                                      │
  │                                                                 │
  │ 1. OS가 BAR0 (Config Space 0x10)에 0xFFFFFFFF를 쓴다           │
  │    → ECAM: writel(0xFFFFFFFF, ecam + bus<<20 + devfn<<12 + 0x10)│
  │    → QEMU: pci_default_write_config() → BAR에 0xFFFFFFFF 기록  │
  │                                                                 │
  │ 2. OS가 BAR0를 다시 읽는다                                      │
  │    → QEMU: pci_default_read_config() → BAR 값 반환             │
  │    → 반환값 예: 0xFFFFC004                                      │
  │       하위 비트: 0x4 = 64비트 BAR, Memory Space                 │
  │       상위 비트: 0xFFFFC000 → 하위 14비트가 0 → 크기 = 2^14 = 16KB│
  │                                                                 │
  │ 3. OS가 BAR0에 실제 주소를 쓴다                                  │
  │    → writel(0xFB000004, ecam + ... + 0x10)                      │
  │    → QEMU: BAR0 = 0xFB000000 (속성 비트 제외)                   │
  │    → QEMU: NVMe의 MemoryRegion(n->iomem)을                     │
  │            Guest 물리 주소 0xFB000000에 매핑!                     │
  │    → 이후 Guest가 0xFB000000에 접근하면 nvme_mmio_ops 호출      │
  └─────────────────────────────────────────────────────────────────┘
```

### 5.2 BAR 쓰기 시 QEMU 내부 동작

```
  Guest가 Config Space의 BAR0에 주소를 쓸 때:

  pci_host_config_write_common(pci_dev, addr=0x10, val=0xFB000004)
    → pci_dev->config_write(pci_dev, 0x10, 0xFB000004, 4)
      → pci_default_write_config()
        → pci_update_mappings()
          │
          │ "BAR0의 값이 바뀌었다"
          │ "이전 주소에서 MemoryRegion을 제거하고"
          │ "새 주소에 MemoryRegion을 추가한다"
          │
          ├── memory_region_del_subregion(old_addr, &n->iomem)  /* 이전 매핑 제거 */
          └── memory_region_add_subregion(new_addr, &n->iomem)  /* 새 매핑 추가 */
                                          ~~~~~~~~~~
                                          0xFB000000

  결과:
  이후 Guest가 물리 주소 0xFB000000 + offset에 접근하면
  → QEMU의 nvme_mmio_read() / nvme_mmio_write()가 호출된다!
```

---

## 6. 전체 연결 관계 정리

```
  ┌──────────────────────────────────────────────────────────────────┐
  │                         Guest 커널                               │
  │                                                                  │
  │  ┌─────────────────────────────────────────────────┐            │
  │  │ PCI 서브시스템 (drivers/pci/)                    │            │
  │  │                                                  │            │
  │  │ ① pci_read_config_dword(pdev, PCI_VENDOR_ID)    │            │
  │  │    → ECAM MMIO: readl(ecam_virt + offset)       │            │
  │  │    → Config Space에서 Vendor ID 읽기            │            │
  │  │                                                  │            │
  │  │ ② pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0)│           │
  │  │    → BAR0 값 읽기 → 물리 주소 0xFB000000        │            │
  │  └─────────────────┬───────────────────────────────┘            │
  │                     │                                            │
  │  ┌─────────────────▼───────────────────────────────┐            │
  │  │ NVMe 드라이버 (drivers/nvme/host/pci.c)          │            │
  │  │                                                  │            │
  │  │ ③ ioremap(pci_resource_start(pdev, 0), size)    │            │
  │  │    = ioremap(0xFB000000, 16384)                 │            │
  │  │    → dev->bar = 커널 가상 주소                  │            │
  │  │                                                  │            │
  │  │ ④ writel(aqa, dev->bar + NVME_REG_AQA)          │            │
  │  │    → BAR0 MMIO: 물리 주소 0xFB000024에 쓰기     │            │
  │  │                                                  │            │
  │  │ ⑤ writel(tail, nvmeq->q_db)                     │            │
  │  │    → BAR0 MMIO: 물리 주소 0xFB001008에 쓰기     │            │
  │  └─────────────────────────────────────────────────┘            │
  └─────────────────┬────────────────┬──────────────────────────────┘
                    │                │
         ┌──────────┘                └──────────┐
         │ ① ECAM 접근                          │ ④⑤ BAR0 접근
         │ (Config Space)                        │ (NVMe 레지스터)
         ▼                                       ▼
  ┌──────────────────┐                   ┌───────────────────┐
  │ KVM: VM-Exit     │                   │ KVM: VM-Exit      │
  │ EPT violation    │                   │ EPT violation     │
  │ 주소: 0xB01xxxxx │                   │ 주소: 0xFB00xxxx  │
  └────────┬─────────┘                   └────────┬──────────┘
           │                                       │
           ▼                                       ▼
  ┌──────────────────────────────────────────────────────────────────┐
  │                          QEMU                                    │
  │                                                                  │
  │  FlatView에서 물리 주소로 MemoryRegion을 찾는다                  │
  │                                                                  │
  │  ┌────────────────────────┐     ┌────────────────────────┐      │
  │  │ MemoryRegion:           │     │ MemoryRegion:           │      │
  │  │ "pcie-mmcfg-mmio"      │     │ "nvme-bar0" (n->iomem)  │      │
  │  │ 주소: 0xB0000000       │     │ 주소: 0xFB000000        │      │
  │  │ 크기: 256MB            │     │ 크기: 16KB              │      │
  │  │ ops: pcie_mmcfg_ops    │     │ ops: nvme_mmio_ops      │      │
  │  │                        │     │                          │      │
  │  │ .read → Bus:DevFn:Reg  │     │ .read → nvme_mmio_read  │      │
  │  │   디코딩 → pci_dev의   │     │ .write→ nvme_mmio_write │      │
  │  │   config[] 배열 접근   │     │   → Doorbell 처리       │      │
  │  │ .write→ BAR 쓰기 시    │     │   → 레지스터 처리       │      │
  │  │   MemoryRegion 재매핑  │     │                          │      │
  │  └────────────────────────┘     └────────────────────────┘      │
  └──────────────────────────────────────────────────────────────────┘
```

---

## 7. Capability 체인과 Extended Capability

### 7.1 Capability 체인 (0x40~0xFF 내)

```
  Config Space 0x34: Capabilities Pointer = 0x40 (첫 번째 Capability 위치)

  0x40: ┌──────────────┬──────────────┐
        │ Cap ID = 0x05│ Next = 0x60  │ ← Power Management Capability
        │ (PM)         │              │
        ├──────────────┴──────────────┤
        │ PM 관련 레지스터들...        │
        └─────────────────────────────┘

  0x60: ┌──────────────┬──────────────┐
        │ Cap ID = 0x11│ Next = 0x70  │ ← MSI-X Capability
        │ (MSI-X)      │              │
        ├──────────────┴──────────────┤
        │ MSI-X Control               │
        │ Table Offset/BIR            │  ← MSI-X Table이 어느 BAR에 있는지
        │ PBA Offset/BIR              │
        └─────────────────────────────┘

  0x70: ┌──────────────┬──────────────┐
        │ Cap ID = 0x10│ Next = 0x00  │ ← PCI Express Capability
        │ (PCIe)       │ (체인 끝)    │
        ├──────────────┴──────────────┤
        │ PCIe Capability Register     │ ← Type (Endpoint/Root Port 등)
        │ Device Capability            │ ← Max Payload, Phantom Functions
        │ Device Control/Status        │ ← Max Read Request Size
        │ Link Capability              │ ← 지원 속도, 레인 수
        │ Link Control/Status          │ ← 현재 속도, 현재 레인 수
        └─────────────────────────────┘

  Guest가 lspci -v로 보는 정보가 바로 이 Capability 체인에서 나온다!
```

### 7.2 Extended Capability (0x100~0xFFF)

Extended Capability는 **ECAM을 통해서만 접근 가능**하다 (레거시 I/O 포트로는 불가).

```
  0x100: ┌──────────────────────────────────────────┐
         │ Cap ID = 0x0001 (AER)                     │
         │ Version = 2                               │
         │ Next = 0x148                              │
         ├──────────────────────────────────────────┤
         │ Uncorrectable Error Status                │
         │ Uncorrectable Error Mask                  │
         │ Correctable Error Status                  │
         │ ...                                       │
         └──────────────────────────────────────────┘

  0x148: ┌──────────────────────────────────────────┐
         │ Cap ID = 0x0019 (Secondary PCIe)          │
         │ Next = 0x000 (끝)                         │
         ├──────────────────────────────────────────┤
         │ Link Control 3                            │
         │ Lane Error Status                         │
         └──────────────────────────────────────────┘
```

---

## 8. 실제 시스템에서 Config Space 확인하기

```bash
# ① lspci로 디바이스 목록 확인
$ lspci -nn
01:00.0 Non-Volatile memory controller [0108]: Samsung [144d:a808]

# ② Config Space 전체 덤프 (ECAM 접근)
$ lspci -s 01:00.0 -xxx       # 256바이트 (레거시)
$ lspci -s 01:00.0 -xxxx      # 4096바이트 (Extended 포함)

# ③ Vendor ID, Device ID 확인
$ setpci -s 01:00.0 0x00.W    # Vendor ID (2바이트)
144d                           # Samsung

$ setpci -s 01:00.0 0x02.W    # Device ID (2바이트)
a808                           # 970 EVO Plus

# ④ BAR0 값 확인
$ setpci -s 01:00.0 0x10.L    # BAR0 (4바이트)
fb000004                       # 물리 주소 0xFB000000, 64비트 BAR

# ⑤ Capability 체인 확인
$ lspci -s 01:00.0 -vvv
  Capabilities: [40] Power Management version 3
  Capabilities: [50] MSI: Enable- Count=1/32 Maskable- 64bit+
  Capabilities: [70] Express (v2) Endpoint, MSI 00
    LnkCap: Speed 8GT/s, Width x4  ← PCIe Gen3 x4
    LnkSta: Speed 8GT/s, Width x4  ← 현재 링크 상태
  Capabilities: [b0] MSI-X: Enable+ Count=33 Masked-
  Capabilities: [100 v2] Advanced Error Reporting
  Capabilities: [148 v1] Secondary PCI Express

# ⑥ ECAM 영역 확인
$ cat /proc/iomem | grep -i ecam
  b0000000-bfffffff : PCI ECAM 0000 [bus 00-ff]    # ← ECAM 256MB
```

---

## 9. 핵심 정리: Config Space vs BAR MMIO 접근 경로

```
  ┌───────────────────────────────────────────────────────────────────┐
  │                    두 가지 MMIO 접근 경로                        │
  │                                                                   │
  │  경로 A: Config Space 접근 (ECAM)                                │
  │  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                        │
  │  용도: 디바이스 식별, BAR 주소 설정, Capability 읽기              │
  │  시점: 드라이버 초기화 시 (드물게)                                │
  │  방법: pci_read_config_dword() → readl(ecam_virt + offset)       │
  │  주소: ECAM_base + (Bus<<20 | DevFn<<12 | Reg)                   │
  │  QEMU: pcie_mmcfg_data_read/write → pci_dev->config[] 배열      │
  │                                                                   │
  │  경로 B: BAR MMIO 접근                                           │
  │  ~~~~~~~~~~~~~~~~~~~~~~~~                                        │
  │  용도: 디바이스 레지스터 읽기/쓰기, Doorbell                     │
  │  시점: I/O마다 (매우 빈번)                                       │
  │  방법: readl/writel(dev->bar + offset)                            │
  │  주소: BAR0_base + Register_offset                                │
  │  QEMU: nvme_mmio_read/write → 디바이스 로직 실행                 │
  │                                                                   │
  │  관계: Config Space의 BAR 레지스터에 저장된 주소가               │
  │        BAR MMIO 영역의 시작 물리 주소이다.                       │
  │        Config Space를 통해 BAR 주소를 설정한 후에야              │
  │        BAR MMIO로 디바이스 레지스터에 접근할 수 있다.            │
  └───────────────────────────────────────────────────────────────────┘
```

---

## 10. PCIe TLP 관점에서의 차이

```
  Config Space 접근 (ECAM):
  ┌──────────────────────────────────────────────────────┐
  │ CPU: readl(ecam_virt + 0x100010)                     │
  │   → 물리 주소 0xB0100010 = ECAM 영역                 │
  │   → PCIe Root Complex가 이것을 Config TLP로 변환!    │
  │                                                      │
  │ PCIe TLP:                                            │
  │   Type: CfgRd0 또는 CfgRd1 (Configuration Read)     │
  │   Bus: 1, Device: 0, Function: 0                     │
  │   Register: 0x10                                     │
  │                                                      │
  │ ★ ECAM에 대한 Memory Read가 Config TLP로 변환되는   │
  │   것은 PCIe Root Complex의 하드웨어 기능이다!        │
  └──────────────────────────────────────────────────────┘

  BAR MMIO 접근:
  ┌──────────────────────────────────────────────────────┐
  │ CPU: writel(tail, dev->bar + 0x1008)                 │
  │   → 물리 주소 0xFB001008 = BAR0 영역                 │
  │   → PCIe Root Complex가 Memory Write TLP 생성        │
  │                                                      │
  │ PCIe TLP:                                            │
  │   Type: MWr (Memory Write)                           │
  │   Address: 0xFB001008                                │
  │   Data: tail value                                   │
  │                                                      │
  │ ★ 일반 Memory Write TLP. 디바이스가 BAR 범위의      │
  │   주소를 자신의 것으로 인식하여 처리한다.             │
  └──────────────────────────────────────────────────────┘

  핵심 차이:
  - ECAM 접근은 Root Complex가 Memory TLP → Config TLP로 변환한다
  - BAR 접근은 그대로 Memory TLP로 디바이스에 전달된다
  - QEMU에서는 둘 다 MemoryRegion의 MMIO 핸들러로 처리되지만,
    실제 하드웨어에서는 TLP 유형이 다르다
```

---

## 11. 소스코드 위치 정리

| 파일 | 내용 |
|------|------|
| **Linux 커널** | |
| `arch/x86/pci/direct.c` | 레거시 PCI Config 접근 (I/O 포트 0xCF8/0xCFC) |
| `arch/x86/pci/mmconfig_64.c` | ECAM MMIO Config 접근 (pci_mmcfg_read/write) |
| `arch/x86/pci/mmconfig-shared.c` | ECAM 영역 초기화 (ACPI MCFG 파싱) |
| `arch/x86/include/asm/pci_x86.h:193` | `PCI_MMCFG_BUS_OFFSET(bus) = bus << 20` |
| `drivers/pci/access.c` | pci_read_config_dword() 등 상위 API |
| `drivers/pci/probe.c` | PCI 버스 스캔, BAR sizing |
| **QEMU** | |
| `hw/pci/pcie_host.c` | ECAM MemoryRegion 생성, pcie_mmcfg_ops |
| `include/hw/pci/pcie_host.h` | ECAM 주소 디코딩 매크로 (PCIE_MMCFG_BUS 등) |
| `hw/pci/pci.c` | pci_default_read/write_config, pci_update_mappings |
| `hw/pci/pci_host.c` | pci_host_config_read/write_common |
| `hw/nvme/ctrl.c` | nvme_init_pci() - NVMe Config Space 초기화 |
