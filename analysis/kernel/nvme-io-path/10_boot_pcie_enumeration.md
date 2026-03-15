# 부트 시퀀스: PCIe 서브시스템 초기화와 버스 스캔

> **소스 기준**: Linux kernel (drivers/pci/probe.c, drivers/acpi/pci_root.c, arch/x86/pci/acpi.c, drivers/pci/setup-bus.c, drivers/pci/setup-res.c, drivers/pci/access.c, arch/x86/pci/mmconfig-shared.c)

이 문서는 리눅스 커널이 부팅하면서 PCIe 서브시스템을 초기화하고, 버스를 스캔하여 NVMe SSD 같은 디바이스를 발견하고 등록하기까지의 전체 과정을 코드 레벨에서 추적한다.

---

## 전체 흐름 ASCII Diagram

```
 ┌─────────────────────────────────────────────────────────────────────┐
 │                    start_kernel()                                   │
 │                         │                                           │
 │                    setup_arch()                                     │
 │                         │                                           │
 │              ┌──────────┴──────────┐                                │
 │              │                     │                                │
 │     pci_mmcfg_early_init()   pci_direct_init()                     │
 │     ┌────────┴────────┐       (Legacy I/O 0xCF8)                   │
 │     │                 │                                             │
 │  pci_mmcfg_          acpi_table_parse                               │
 │  check_hostbridge()   (ACPI_SIG_MCFG)                              │
 │     │                 │                                             │
 │     │           pci_parse_mcfg()                                    │
 │     │                 │                                             │
 │     │           pci_mmconfig_add()                                  │
 │     │                 │                                             │
 │     └─────┬───────────┘                                             │
 │           │                                                         │
 │     __pci_mmcfg_init()                                              │
 │           │                                                         │
 │     pci_mmcfg_arch_init()  → ioremap() ECAM 영역                   │
 │                                                                     │
 │                    do_initcalls()                                    │
 │                         │                                           │
 │   ┌─────────────────────┼───────────────────────┐                   │
 │   │                     │                       │                   │
 │ postcore_initcall   subsys_initcall        arch_initcall            │
 │ pcibus_class_init() acpi_pci_root_init()  pci_acpi_init()           │
 │                         │                                           │
 │              acpi_scan_add_handler()                                 │
 │                         │                                           │
 │              ─── ACPI 디바이스 스캔 시 ───                           │
 │                         │                                           │
 │              acpi_pci_root_add()                                    │
 │                         │                                           │
 │              ┌──────────┴──────────┐                                │
 │              │                     │                                │
 │     negotiate_os_control()  pci_acpi_scan_root()                    │
 │     (BIOS↔OS 제어권 협상)         │                                 │
 │                          acpi_pci_root_create()                     │
 │                                   │                                 │
 │                    ┌──────────────┴──────────────┐                  │
 │                    │                             │                  │
 │           pci_create_root_bus()        pci_scan_child_bus()         │
 │           (Host Bridge + Bus 0)               │                    │
 │                                    ┌──────────┴──────────┐         │
 │                                    │                     │         │
 │                              pci_scan_slot()    pci_scan_bridge_   │
 │                              (slot 0~31)        extend()           │
 │                                    │            (재귀 스캔)         │
 │                                    │                               │
 │                         pci_scan_single_device()                   │
 │                                    │                               │
 │                    ┌───────────────┴───────────────┐               │
 │                    │                               │               │
 │              pci_scan_device()           pci_device_add()          │
 │              ┌─────┴──────┐             ┌─────────┴────────┐      │
 │              │            │             │                  │       │
 │     Vendor ID 읽기  pci_setup_device() pci_init_         device_  │
 │     (Config 0x00)   ┌─────┴─────┐     capabilities()    add()    │
 │                     │           │     (MSI/MSI-X/       (sysfs)  │
 │              pci_read_bases()  Class Code               AER 등)   │
 │              (BAR 파싱)        읽기                                │
 │                                                                    │
 │                    ─── 리소스 할당 ───                              │
 │                                                                    │
 │           pci_assign_unassigned_root_bus_resources()                │
 │                         │                                          │
 │              __pci_bus_assign_resources()                           │
 │                         │                                          │
 │              pbus_assign_resources_sorted()                        │
 │                         │                                          │
 │              pci_assign_resource() → pci_update_resource()         │
 │              (물리 주소 할당)         (BAR에 주소 기록)              │
 │                                                                    │
 │                    ─── 디바이스 등록 ───                            │
 │                                                                    │
 │              pci_bus_add_devices()                                  │
 │                         │                                          │
 │              pci_bus_add_device()                                   │
 │              ┌──────────┴──────────┐                               │
 │              │                     │                               │
 │     pci_create_sysfs_dev_files()  device_initial_probe()           │
 │     (/sys/bus/pci/devices/         (드라이버 매칭 시작)             │
 │      0000:01:00.0 생성)                                            │
 │                                                                    │
 │              ═══ NVMe 드라이버 probe() 호출 ═══                    │
 └─────────────────────────────────────────────────────────────────────┘
```

---

# Phase 1: 커널 부트 → PCIe 서브시스템 초기화

## 1.1 커널 부트 시퀀스에서 PCI 초기화 위치

리눅스 커널의 부팅 과정은 `start_kernel()`에서 시작하여 여러 단계의 초기화를 거친다. PCI 서브시스템의 초기화는 이 과정에서 여러 시점에 걸쳐 분산되어 있다.

### initcall 레벨 개요

커널은 `do_initcalls()`를 통해 0~7 레벨의 초기화 함수를 순서대로 실행한다. PCI 관련 초기화는 다음 레벨들에 분포한다:

```
 Level 0  early_initcall     : (PCI에 직접 해당 없음)
 Level 1  core_initcall      : (PCI에 직접 해당 없음)
 Level 2  postcore_initcall  : pcibus_class_init()  ← PCI 버스 클래스 등록
 Level 3  arch_initcall      : pci_acpi_init()      ← ACPI IRQ 라우팅 설정
 Level 4  subsys_initcall    : acpi_pci_root_init()  ← PCI Root Bridge 핸들러 등록
                                pci_subsys_init()    ← Legacy PCI 프로빙
 Level 5  fs_initcall        : pcibios_assign_resources() (일부 아키텍처)
 Level 6  device_initcall    : (PCI 드라이버들)
 Level 7  late_initcall      : pci_mmcfg_late_init() ← ECAM 후기 초기화
```

### setup_arch() 단계: 가장 이른 PCI 초기화

`start_kernel()` → `setup_arch()` 과정에서 ECAM(Enhanced Configuration Access Mechanism)이 먼저 초기화된다. 이것은 `do_initcalls()` 이전에 일어나는 매우 이른 시점의 초기화다.

```
start_kernel()
  └─ setup_arch()           // arch/x86/kernel/setup.c
       ├─ ...
       ├─ pci_mmcfg_early_init()    // ECAM/MCFG 파싱 (아래 1.2에서 상세)
       └─ pci_direct_init()          // Legacy I/O port 방식 Config Space 접근 설정
```

### postcore_initcall: PCI 버스 클래스 등록

```c
// drivers/pci/probe.c:107-111
static const struct class pcibus_class = {
    .name       = "pci_bus",
    .dev_release    = &release_pcibus_dev,
    .dev_groups = pcibus_groups,
};

static int __init pcibus_class_init(void)
{
    return class_register(&pcibus_class);
}
postcore_initcall(pcibus_class_init);
```

이 함수는 `/sys/class/pci_bus/` 디렉토리를 생성하는 sysfs 클래스를 등록한다. 이후 발견되는 모든 PCI 버스가 이 클래스 아래에 등록된다.

### subsys_initcall: ACPI PCI Root Bridge 핸들러 등록

```c
// drivers/acpi/pci_root.c:1061-1068
void __init acpi_pci_root_init(void)
{
    if (acpi_pci_disabled)
        return;

    pci_acpi_crs_quirks();
    acpi_scan_add_handler_with_hotplug(&pci_root_handler, "pci_root");
}
```

이 시점에서 `pci_root_handler`가 ACPI 서브시스템에 등록된다. 이후 ACPI가 디바이스 트리를 스캔할 때 `PNP0A03` (PCI Host Bridge) 또는 `PNP0A08` (PCIe Host Bridge) 디바이스를 만나면 `acpi_pci_root_add()`가 호출된다.

핸들러 구조체:

```c
// drivers/acpi/pci_root.c:44-57
static const struct acpi_device_id root_device_ids[] = {
    {"PNP0A03", 0},    // PCI Host Bridge
    {"", 0},
};

static struct acpi_scan_handler pci_root_handler = {
    .ids = root_device_ids,
    .attach = acpi_pci_root_add,      // ← 핵심: Root Bridge 발견 시 호출
    .detach = acpi_pci_root_remove,
    .hotplug = {
        .enabled = true,
        .scan_dependent = acpi_pci_root_scan_dependent,
    },
};
```

`PNP0A03`은 ACPI 네임스페이스에서 PCI/PCIe Host Bridge를 식별하는 HID(Hardware ID)다. 이 ID를 가진 ACPI 디바이스가 발견되면 `acpi_pci_root_add()`가 attach 콜백으로 호출되어 PCI 버스 스캔이 시작된다.

---

## 1.2 ACPI MCFG 테이블 파싱 → ECAM 초기화

PCIe 디바이스의 Configuration Space에 접근하려면 ECAM(Enhanced Configuration Access Mechanism)이 필요하다. 전통적인 PCI는 I/O 포트 0xCF8/0xCFC를 통해 256바이트의 Config Space에 접근했지만, PCIe는 디바이스당 4KB의 확장된 Config Space를 제공하며, 이를 위해 MMIO 기반의 ECAM을 사용한다.

### ECAM 주소 계산 공식

```
ECAM 주소 = ECAM_Base + (Bus << 20) + (Device << 15) + (Function << 12) + Register
```

각 Bus/Device/Function 조합이 4KB(= 2^12) 공간을 차지하므로:
- 1개 Function = 4KB
- 1개 Device (8 Functions) = 32KB
- 1개 Bus (32 Devices) = 1MB (= 2^20)

### pci_mmcfg_early_init(): ECAM 초기화 진입점

```c
// arch/x86/pci/mmconfig-shared.c:715-728
void __init pci_mmcfg_early_init(void)
{
    // ① pci_probe 플래그에 PCI_PROBE_MMCONF가 설정되어 있는지 확인
    //    이 플래그는 커널 커맨드라인 "pci=nommconf"로 비활성화 가능
    if (pci_probe & PCI_PROBE_MMCONF) {

        // ② 먼저 알려진 호스트 브릿지 칩셋의 레지스터에서 직접 ECAM 주소 추출 시도
        //    Intel E7520, 945G, AMD Fam10h, NVIDIA MCP55 등
        if (pci_mmcfg_check_hostbridge())
            known_bridge = 1;
        else
            // ③ 칩셋별 탐지 실패 → ACPI MCFG 테이블에서 ECAM 정보 파싱
            acpi_table_parse(ACPI_SIG_MCFG, pci_parse_mcfg);

        // ④ 파싱된 ECAM 정보를 검증하고 아키텍처별 초기화 수행
        __pci_mmcfg_init(1);

        set_apei_filter();
    }
}
```

**라인별 분석:**

- **known_bridge 판정 (line 720-721)**: `pci_mmcfg_check_hostbridge()`는 Legacy I/O 포트(0xCF8/0xCFC)를 사용하여 Bus 0, Device 0의 Vendor/Device ID를 읽고, 알려진 칩셋(Intel E7520, 945G, AMD Fam10h, NVIDIA MCP55)인지 판별한다. 해당 칩셋이면 칩셋 고유 레지스터에서 ECAM base 주소를 직접 추출한다.
- **MCFG 파싱 (line 723)**: `acpi_table_parse()`는 ACPI 테이블 중 "MCFG" 시그니처를 가진 테이블을 찾아 `pci_parse_mcfg()` 콜백을 호출한다.

### pci_parse_mcfg(): MCFG 테이블 파싱

```c
// arch/x86/pci/mmconfig-shared.c:617-659
static int __init pci_parse_mcfg(struct acpi_table_header *header)
{
    struct acpi_table_mcfg *mcfg;
    struct acpi_mcfg_allocation *cfg_table, *cfg;
    unsigned long i;
    int entries;

    if (!header)
        return -EINVAL;

    // MCFG 테이블 헤더를 캐스팅
    mcfg = (struct acpi_table_mcfg *)header;

    // MCFG 엔트리 개수 계산:
    //   테이블 전체 길이에서 고정 헤더 크기를 빼면
    //   가변 길이의 allocation 엔트리 배열이 남는다
    free_all_mmcfg();
    entries = 0;
    i = header->length - sizeof(struct acpi_table_mcfg);
    while (i >= sizeof(struct acpi_mcfg_allocation)) {
        entries++;
        i -= sizeof(struct acpi_mcfg_allocation);
    }
    if (entries == 0) {
        pr_err("MCFG has no entries\n");
        return -ENODEV;
    }

    // MCFG 헤더 바로 뒤에 allocation 엔트리 배열이 위치
    cfg_table = (struct acpi_mcfg_allocation *) &mcfg[1];
    for (i = 0; i < entries; i++) {
        cfg = &cfg_table[i];
        // 유효성 검사 (4GB 이상 주소 처리 등)
        if (!acpi_mcfg_valid_entry(mcfg, cfg)) {
            free_all_mmcfg();
            return -ENODEV;
        }
        // 각 MCFG 엔트리를 ECAM 영역으로 등록
        if (pci_mmconfig_add(cfg->pci_segment, cfg->start_bus_number,
                   cfg->end_bus_number, cfg->address) == NULL) {
            pr_warn("no memory for MCFG entries\n");
            free_all_mmcfg();
            return -ENOMEM;
        }
    }

    return 0;
}
```

MCFG 테이블의 각 엔트리(`struct acpi_mcfg_allocation`)는 다음 정보를 담고 있다:
- `address`: ECAM 영역의 물리 base 주소
- `pci_segment`: PCI Segment(Domain) 번호
- `start_bus_number` / `end_bus_number`: 이 ECAM 영역이 담당하는 버스 범위

### pci_mmconfig_add(): ECAM 영역 등록

```c
// arch/x86/pci/mmconfig-shared.c:100-117
struct pci_mmcfg_region *__init pci_mmconfig_add(int segment, int start,
                         int end, u64 addr)
{
    struct pci_mmcfg_region *new;

    // pci_mmcfg_region 구조체 할당 및 초기화
    new = pci_mmconfig_alloc(segment, start, end, addr);
    if (!new)
        return NULL;

    // 정렬된 리스트(pci_mmcfg_list)에 삽입
    mutex_lock(&pci_mmcfg_lock);
    list_add_sorted(new);
    mutex_unlock(&pci_mmcfg_lock);

    pr_info("ECAM %pR (base %#lx) for domain %04x [bus %02x-%02x]\n",
        &new->res, (unsigned long)addr, segment, start, end);

    return new;
}
```

`pci_mmconfig_alloc()`은 리소스 정보를 다음과 같이 계산한다:

```c
// arch/x86/pci/mmconfig-shared.c:71-98
static struct pci_mmcfg_region *pci_mmconfig_alloc(int segment, int start,
                           int end, u64 addr)
{
    struct pci_mmcfg_region *new;
    struct resource *res;

    if (addr == 0)
        return NULL;

    new = kzalloc(sizeof(*new), GFP_KERNEL);
    if (!new)
        return NULL;

    new->address = addr;         // ECAM 물리 base 주소
    new->segment = segment;      // PCI Domain 번호
    new->start_bus = start;      // 시작 버스 번호
    new->end_bus = end;          // 끝 버스 번호

    res = &new->res;
    // 리소스 시작 = base + (start_bus * 1MB)
    res->start = addr + PCI_MMCFG_BUS_OFFSET(start);
    // 리소스 끝 = base + ((end_bus + 1) * 1MB) - 1
    res->end = addr + PCI_MMCFG_BUS_OFFSET(end + 1) - 1;
    res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
    snprintf(new->name, PCI_MMCFG_RESOURCE_NAME_LEN,
         "PCI ECAM %04x [bus %02x-%02x]", segment, start, end);
    res->name = new->name;

    return new;
}
```

예시: segment=0, start=0, end=255, addr=0xE0000000 이면:
- `res->start` = 0xE0000000
- `res->end` = 0xEFFFFFFF (256 buses * 1MB each = 256MB)

### __pci_mmcfg_init(): ECAM 활성화

```c
// arch/x86/pci/mmconfig-shared.c:687-711
static void __init __pci_mmcfg_init(int early)
{
    // ① 등록된 ECAM 영역이 실제로 예약된 메모리인지 검증
    //    E820 맵, ACPI 리소스 등을 확인하여 유효하지 않으면 제거
    pci_mmcfg_reject_broken(early);
    if (list_empty(&pci_mmcfg_list))
        return;

    // ② pcibios_last_bus 업데이트
    if (pcibios_last_bus < 0) {
        const struct pci_mmcfg_region *cfg;
        list_for_each_entry(cfg, &pci_mmcfg_list, list) {
            if (cfg->segment)
                break;
            pcibios_last_bus = cfg->end_bus;
        }
    }

    // ③ 아키텍처별 초기화 (x86에서는 ioremap 수행)
    //    성공하면 pci_probe에 PCI_PROBE_MMCONF 플래그 설정
    if (pci_mmcfg_arch_init())
        pci_probe = (pci_probe & ~PCI_PROBE_MASK) | PCI_PROBE_MMCONF;
    else {
        free_all_mmcfg();
        pci_mmcfg_arch_init_failed = true;
    }
}
```

`pci_mmcfg_arch_init()` (x86에서는 `arch/x86/pci/mmconfig_64.c` 또는 `mmconfig_32.c`)이 ECAM 영역을 `ioremap()`으로 가상 주소에 매핑한다. 이후부터 커널은 ECAM을 통해 PCIe Extended Configuration Space(4KB)에 접근할 수 있게 된다.

### Config Space 접근 메커니즘: access.c

ECAM이 초기화된 후, Config Space 접근은 다음 계층을 통해 이루어진다:

```c
// drivers/pci/access.c:35-55 (매크로로 생성)
// PCI_OP_READ 매크로가 pci_bus_read_config_dword() 등을 생성한다:
#define PCI_OP_READ(size, type, len) \
int noinline pci_bus_read_config_##size \
    (struct pci_bus *bus, unsigned int devfn, int pos, type *value) \
{                                                                   \
    unsigned long flags;                                            \
    u32 data = 0;                                                   \
    int res;                                                        \
                                                                    \
    if (PCI_##size##_BAD)                                           \
        return PCIBIOS_BAD_REGISTER_NUMBER;                         \
                                                                    \
    pci_lock_config(flags);                                         \
    res = bus->ops->read(bus, devfn, pos, len, &data);              \
    if (res)                                                        \
        PCI_SET_ERROR_RESPONSE(value);                              \
    else                                                            \
        *value = (type)data;                                        \
    pci_unlock_config(flags);                                       \
                                                                    \
    return res;                                                     \
}
```

**핵심 동작:**
1. `pci_lock_config(flags)`: 글로벌 스핀락 `pci_lock`을 잡는다 (`CONFIG_PCI_LOCKLESS_CONFIG`가 아닌 경우).
2. `bus->ops->read()`: 버스에 설정된 operations의 `read` 콜백을 호출한다. ECAM의 경우 `pci_generic_config_read()`가 된다.
3. `pci_unlock_config(flags)`: 스핀락을 해제한다.

ECAM 기반의 generic read는 매우 단순하다:

```c
// drivers/pci/access.c:88-106
int pci_generic_config_read(struct pci_bus *bus, unsigned int devfn,
                int where, int size, u32 *val)
{
    void __iomem *addr;

    // map_bus 콜백으로 ECAM 가상 주소 계산
    // addr = ecam_base + (bus << 20) + (devfn << 12) + where
    addr = bus->ops->map_bus(bus, devfn, where);
    if (!addr)
        return PCIBIOS_DEVICE_NOT_FOUND;

    // MMIO 읽기: 크기에 따라 readb/readw/readl 사용
    if (size == 1)
        *val = readb(addr);
    else if (size == 2)
        *val = readw(addr);
    else
        *val = readl(addr);

    return PCIBIOS_SUCCESSFUL;
}
```

이것이 PCIe Config Space 접근의 최하위 계층이다. CPU가 ECAM 영역의 가상 주소에 대해 메모리 읽기/쓰기를 수행하면, Memory Controller(또는 Root Complex)가 이를 PCIe Configuration Transaction으로 변환하여 대상 디바이스에 전달한다.

---

## 1.3 PCI Host Bridge 등록

ECAM이 초기화된 후, ACPI 서브시스템이 디바이스 트리를 스캔하면서 PCI Host Bridge(`PNP0A03` 또는 `PNP0A08`)를 발견하면 `acpi_pci_root_add()`가 호출된다.

### acpi_pci_root_add(): Root Bridge 초기화의 핵심

```c
// drivers/acpi/pci_root.c:639-771
static int acpi_pci_root_add(struct acpi_device *device,
                 const struct acpi_device_id *not_used)
{
    unsigned long long segment, bus;
    acpi_status status;
    int result;
    struct acpi_pci_root *root;
    acpi_handle handle = device->handle;
    int no_aspm = 0;
    bool hotadd = system_state == SYSTEM_RUNNING;

    // ① acpi_pci_root 구조체 할당
    root = kzalloc(sizeof(struct acpi_pci_root), GFP_KERNEL);
    if (!root)
        return -ENOMEM;

    // ② PCI Segment(Domain) 번호 추출
    //    ACPI _SEG 메서드를 평가하여 Segment 번호를 얻는다.
    //    대부분의 시스템에서 0이다.
    segment = 0;
    status = acpi_evaluate_integer(handle, METHOD_NAME__SEG, NULL, &segment);
    if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
        dev_err(&device->dev,  "can't evaluate _SEG\n");
        result = -ENODEV;
        goto end;
    }

    // ③ 버스 번호 범위 추출
    //    먼저 _CRS(Current Resource Settings)에서 버스 번호 범위를 얻으려 시도
    //    실패하면 _BBN(Base Bus Number)에서 시작 번호만 얻는다
    root->secondary.flags = IORESOURCE_BUS;
    status = try_get_root_bridge_busnr(handle, &root->secondary);
    if (ACPI_FAILURE(status)) {
        root->secondary.end = 0xFF;
        dev_warn(&device->dev,
             FW_BUG "no secondary bus range in _CRS\n");
        status = acpi_evaluate_integer(handle, METHOD_NAME__BBN,
                           NULL, &bus);
        if (ACPI_SUCCESS(status))
            root->secondary.start = bus;
        else if (status == AE_NOT_FOUND)
            root->secondary.start = 0;
        else {
            dev_err(&device->dev, "can't evaluate _BBN\n");
            result = -ENODEV;
            goto end;
        }
    }

    // ④ root 구조체 초기화
    root->device = device;
    root->segment = segment & 0xFFFF;
    device->driver_data = root;

    // ⑤ MCFG base 주소 얻기 (이 Host Bridge 고유의 ECAM 주소)
    root->mcfg_addr = acpi_pci_root_get_mcfg_addr(handle);

    // ⑥ Bridge 타입 판별
    //    PNP0A08 → PCIe, ACPI0016 → CXL
    acpi_hid = acpi_device_hid(root->device);
    if (strcmp(acpi_hid, "PNP0A08") == 0)
        root->bridge_type = ACPI_BRIDGE_TYPE_PCIE;
    else if (strcmp(acpi_hid, "ACPI0016") == 0)
        root->bridge_type = ACPI_BRIDGE_TYPE_CXL;

    // ⑦ OS와 BIOS 간 제어권 협상 (_OSC 메서드)
    //    AER, Hotplug, PME, DPC 등의 제어를 누가 할지 결정
    negotiate_os_control(root, &no_aspm);

    // ⑧ Root Bridge 스캔 → 하위 버스 & 디바이스 탐색
    root->bus = pci_acpi_scan_root(root);
    if (!root->bus) {
        result = -ENODEV;
        goto remove_dmar;
    }

    // ⑨ 디바이스 등록 (sysfs, uevent 등)
    pci_lock_rescan_remove();
    pci_bus_add_devices(root->bus);
    pci_unlock_rescan_remove();
    return 1;

remove_dmar:
    if (hotadd)
        dmar_device_remove(handle);
end:
    kfree(root);
    return result;
}
```

**단계별 흐름:**

1. **Segment 추출 (②)**: ACPI `_SEG` 메서드로 PCI Domain 번호를 얻는다. 대부분의 단일 소켓 시스템에서는 0이다.

2. **버스 범위 추출 (③)**: `try_get_root_bridge_busnr()`은 ACPI `_CRS`를 파싱하여 `ACPI_BUS_NUMBER_RANGE` 타입의 리소스에서 버스 번호 범위를 추출한다. 일반적으로 Host Bridge 0은 Bus 0~255를 담당한다.

3. **_OSC 협상 (⑦)**: `negotiate_os_control()`은 ACPI `_OSC`(Operating System Capabilities) 메서드를 통해 BIOS와 OS 간에 PCIe 기능의 제어권을 협상한다. OS가 AER, Hotplug, PME, DPC 등을 직접 제어할 수 있는지 BIOS에 문의하고, BIOS가 허용한 기능만 OS가 관리한다.

4. **Root 스캔 (⑧)**: `pci_acpi_scan_root()`이 실제 버스 스캔을 시작한다.

### pci_acpi_scan_root(): x86에서의 Root 스캔

```c
// arch/x86/pci/acpi.c:533-589
struct pci_bus *pci_acpi_scan_root(struct acpi_pci_root *root)
{
    int domain = root->segment;
    int busnum = root->secondary.start;
    int node = pci_acpi_root_get_node(root);
    struct pci_bus *bus;

    // Segment 무시 quirk 처리
    if (pci_ignore_seg)
        root->segment = domain = 0;

    // 다중 도메인 미지원 시스템에서 도메인 ≠ 0이면 스킵
    if (domain && !pci_domains_supported) {
        pr_warn("pci_bus %04x:%02x: ignored\n", domain, busnum);
        return NULL;
    }

    // 이미 스캔된 버스인지 확인
    bus = pci_find_bus(domain, busnum);
    if (bus) {
        // 이미 있으면 sysdata만 업데이트
        struct pci_sysdata sd = {
            .domain = domain,
            .node = node,
            .companion = root->device
        };
        memcpy(bus->sysdata, &sd, sizeof(sd));
    } else {
        // 새 버스 → pci_root_info 할당 및 acpi_pci_root_create() 호출
        struct pci_root_info *info;
        info = kzalloc(sizeof(*info), GFP_KERNEL);
        if (!info)
            return NULL;

        info->sd.domain = domain;
        info->sd.node = node;
        info->sd.companion = root->device;
        // ★ 여기서 Root Bus 생성 + 하위 버스 스캔이 시작된다
        bus = acpi_pci_root_create(root, &acpi_pci_root_ops,
                       &info->common, &info->sd);
    }

    // PCIe MPS/MRRS 설정 최적화
    if (bus) {
        struct pci_bus *child;
        list_for_each_entry(child, &bus->children, node)
            pcie_bus_configure_settings(child);
    }

    return bus;
}
```

### acpi_pci_root_create(): Root Bus 생성 + 버스 스캔

```c
// drivers/acpi/pci_root.c:996-1059
struct pci_bus *acpi_pci_root_create(struct acpi_pci_root *root,
                     struct acpi_pci_root_ops *ops,
                     struct acpi_pci_root_info *info,
                     void *sysdata)
{
    int ret, busnum = root->secondary.start;
    struct acpi_device *device = root->device;
    struct pci_bus *bus;
    struct pci_host_bridge *host_bridge;

    // ① info 구조체 초기화
    info->root = root;
    info->bridge = device;
    info->ops = ops;
    INIT_LIST_HEAD(&info->resources);

    // ② MCFG 맵 설정 (x86에서는 setup_mcfg_map())
    //    이 Host Bridge의 ECAM 정보를 MMCONFIG 서브시스템에 등록
    if (ops->init_info && ops->init_info(info))
        goto out_release_info;

    // ③ Host Bridge의 I/O 및 Memory 리소스를 ACPI _CRS에서 추출
    if (ops->prepare_resources)
        ret = ops->prepare_resources(info);
    else
        ret = acpi_pci_probe_root_resources(info);
    if (ret < 0)
        goto out_release_info;

    // ④ 리소스를 커널 리소스 트리에 등록
    pci_acpi_root_add_resources(info);
    pci_add_resource(&info->resources, &root->secondary);

    // ⑤ ★ Root Bus 생성
    bus = pci_create_root_bus(NULL, busnum, ops->pci_ops,
                  sysdata, &info->resources);
    if (!bus)
        goto out_release_info;

    // ⑥ Host Bridge 속성 설정 (_OSC 협상 결과 반영)
    host_bridge = to_pci_host_bridge(bus->bridge);
    if (!(root->osc_control_set & OSC_PCI_EXPRESS_NATIVE_HP_CONTROL))
        host_bridge->native_pcie_hotplug = 0;
    if (!(root->osc_control_set & OSC_PCI_EXPRESS_AER_CONTROL))
        host_bridge->native_aer = 0;
    if (!(root->osc_control_set & OSC_PCI_EXPRESS_PME_CONTROL))
        host_bridge->native_pme = 0;
    if (!(root->osc_control_set & OSC_PCI_EXPRESS_LTR_CONTROL))
        host_bridge->native_ltr = 0;
    if (!(root->osc_control_set & OSC_PCI_EXPRESS_DPC_CONTROL))
        host_bridge->native_dpc = 0;

    // ⑦ ★★ 하위 버스 스캔 시작 → Phase 2로 진입
    pci_scan_child_bus(bus);

    return bus;
}
```

### pci_create_root_bus(): Host Bridge + Bus 0 생성

```c
// drivers/pci/probe.c:3263-3290
struct pci_bus *pci_create_root_bus(struct device *parent, int bus,
        struct pci_ops *ops, void *sysdata, struct list_head *resources)
{
    int error;
    struct pci_host_bridge *bridge;

    // ① Host Bridge 구조체 할당
    bridge = pci_alloc_host_bridge(0);
    if (!bridge)
        return NULL;

    bridge->dev.parent = parent;

    // ② 리소스(I/O window, Memory window, Bus range)를
    //    bridge->windows로 이동
    list_splice_init(resources, &bridge->windows);
    bridge->sysdata = sysdata;
    bridge->busnr = bus;
    bridge->ops = ops;          // Config Space 접근 operations

    // ③ Host Bridge 등록 (sysfs, device tree 등)
    error = pci_register_host_bridge(bridge);
    if (error < 0)
        goto err_out;

    return bridge->bus;         // 생성된 root PCI bus 반환
}
```

`pci_register_host_bridge()` 내부에서 다음이 일어난다:

```c
// drivers/pci/probe.c:988-1160 (핵심 부분 요약)
static int pci_register_host_bridge(struct pci_host_bridge *bridge)
{
    struct pci_bus *bus;

    // Bus 구조체 할당
    bus = pci_alloc_bus(NULL);
    bridge->bus = bus;
    bus->sysdata = bridge->sysdata;
    bus->ops = bridge->ops;
    bus->number = bridge->busnr;     // Bus 0

    // 중복 검사
    b = pci_find_bus(pci_domain_nr(bus), bridge->busnr);
    if (b) return -EEXIST;

    // dev_set_name: "pci0000:00" 형태의 이름 설정
    dev_set_name(&bridge->dev, "pci%04x:%02x",
                 pci_domain_nr(bus), bridge->busnr);

    // device_add: sysfs에 Host Bridge 디바이스 등록
    device_add(&bridge->dev);

    // Bus 디바이스 등록
    bus->bridge = get_device(&bridge->dev);
    bus->dev.class = &pcibus_class;
    dev_set_name(&bus->dev, "%04x:%02x",
                 pci_domain_nr(bus), bus->number);
    device_register(&bus->dev);

    // 리소스 윈도우를 Bus에 등록
    resource_list_for_each_entry_safe(window, n, &resources) {
        if (res->flags & IORESOURCE_BUS)
            pci_bus_insert_busn_res(bus, bus->number, res->end);
        else
            pci_bus_add_resource(bus, res);
    }

    // root_buses 글로벌 리스트에 추가
    list_add_tail(&bus->node, &pci_root_buses);

    return 0;
}
```

이 시점에서 `/sys/devices/pci0000:00/` 디렉토리가 생성되고, PCI Bus 0이 커널에 등록된다.

---

# Phase 2: PCIe 버스 스캔 (Enumeration)

## 2.1 버스 스캔 시작

`acpi_pci_root_create()`에서 `pci_scan_child_bus(bus)`가 호출되면 Bus 0부터 재귀적인 버스 스캔이 시작된다.

```c
// drivers/pci/probe.c:3237-3241
unsigned int pci_scan_child_bus(struct pci_bus *bus)
{
    return pci_scan_child_bus_extend(bus, 0);
}
```

### pci_scan_child_bus_extend(): 버스 스캔의 실제 구현

```c
// drivers/pci/probe.c:3114-3228
static unsigned int pci_scan_child_bus_extend(struct pci_bus *bus,
                          unsigned int available_buses)
{
    unsigned int used_buses, normal_bridges = 0, hotplug_bridges = 0;
    unsigned int start = bus->busn_res.start;
    unsigned int devnr, cmax, max = start;
    struct pci_dev *dev;

    dev_dbg(&bus->dev, "scanning bus\n");

    // ★ Phase 2.2: 모든 슬롯 스캔 (Device 0 ~ 31)
    for (devnr = 0; devnr < PCI_MAX_NR_DEVS; devnr++)
        pci_scan_slot(bus, PCI_DEVFN(devnr, 0));

    // SR-IOV를 위한 버스 번호 예약
    used_buses = pci_iov_bus_range(bus);
    max += used_buses;

    // 아키텍처별 Bus fixup
    if (!bus->is_added) {
        pcibios_fixup_bus(bus);
        bus->is_added = 1;
    }

    // Bridge 개수 카운트 (일반 / 핫플러그)
    for_each_pci_bridge(dev, bus) {
        if (dev->is_hotplug_bridge)
            hotplug_bridges++;
        else
            normal_bridges++;
    }

    // ★ Phase 2.5: 이미 설정된 Bridge들을 먼저 스캔 (pass 0)
    for_each_pci_bridge(dev, bus) {
        cmax = max;
        max = pci_scan_bridge_extend(bus, dev, max, 0, 0);
        used_buses++;
        if (max - cmax > 1)
            used_buses += max - cmax - 1;
    }

    // 재설정이 필요한 Bridge들을 스캔 (pass 1)
    for_each_pci_bridge(dev, bus) {
        unsigned int buses = 0;

        if (!hotplug_bridges && normal_bridges == 1) {
            buses = available_buses;
        } else if (dev->is_hotplug_bridge) {
            buses = available_buses / hotplug_bridges;
            buses = min(buses, available_buses - used_buses + 1);
        }

        cmax = max;
        max = pci_scan_bridge_extend(bus, dev, cmax, buses, 1);
        if (max - cmax > 1)
            used_buses += max - cmax - 1;
    }

    return max;
}
```

**핵심 알고리즘:**

이 함수는 3단계로 동작한다:

1. **슬롯 스캔**: `for (devnr = 0; devnr < 32; devnr++)` 루프로 현재 버스의 모든 슬롯(Device 0~31)을 스캔한다. 각 슬롯에서 디바이스가 발견되면 `pci_dev` 구조체가 생성된다.

2. **Bridge Pass 0**: 이미 BIOS가 설정해둔 Bridge들을 먼저 처리한다. 이 Bridge 아래의 하위 버스를 재귀적으로 스캔한다.

3. **Bridge Pass 1**: BIOS가 설정하지 않은(또는 재설정이 필요한) Bridge들을 처리한다. 이때 새 버스 번호를 할당한다.

---

## 2.2 디바이스 발견 (Device Discovery)

### pci_scan_slot(): 슬롯 스캔

```c
// drivers/pci/probe.c:2898-2931
int pci_scan_slot(struct pci_bus *bus, int devfn)
{
    struct pci_dev *dev;
    int fn = 0, nr = 0;

    // PCIe Downstream Port는 Device 0만 가진다 (최적화)
    if (only_one_child(bus) && (devfn > 0))
        return 0;

    // Function 0부터 스캔 시작
    do {
        dev = pci_scan_single_device(bus, devfn + fn);
        if (dev) {
            if (!pci_dev_is_added(dev))
                nr++;               // 새로 발견된 디바이스 카운트
            if (fn > 0)
                dev->multifunction = 1;  // Function 0이 아니면 멀티펑션
        } else if (fn == 0) {
            // Function 0이 없으면 → 이 슬롯에 디바이스 없음
            // (하이퍼바이저 환경 예외 있음)
            if (!hypervisor_isolated_pci_functions())
                break;
        }
        fn = next_fn(bus, dev, fn);  // 다음 Function 번호 결정
    } while (fn >= 0);

    // ASPM 초기화
    if (bus->self && nr)
        pcie_aspm_init_link_state(bus->self);

    return nr;
}
```

**PCIe 최적화 - `only_one_child()`:**

```c
// drivers/pci/probe.c:2865-2885
static int only_one_child(struct pci_bus *bus)
{
    struct pci_dev *bridge = bus->self;

    if (pci_has_flag(PCI_SCAN_ALL_PCIE_DEVS))
        return 0;

    // PCIe Downstream Port 아래에는 Device 0만 존재한다
    // (PCIe spec r3.1, sec 7.3.1)
    if (bridge && pci_is_pcie(bridge) && pcie_downstream_port(bridge))
        return 1;

    return 0;
}
```

PCIe 스위치의 Downstream Port나 Root Port 아래에는 항상 Device 0만 존재하므로, Device 1~31을 스캔하지 않아 시간을 절약한다.

**Function 번호 결정 - `next_fn()`:**

```c
// drivers/pci/probe.c:2851-2863
static int next_fn(struct pci_bus *bus, struct pci_dev *dev, int fn)
{
    // ARI(Alternative Routing-ID Interpretation) 활성화 시
    // Function 번호가 0~255까지 확장 가능
    if (pci_ari_enabled(bus))
        return next_ari_fn(bus, dev, fn);

    // 일반 PCI: Function 0~7
    if (fn >= 7)
        return -ENODEV;
    // 멀티펑션 디바이스가 아니면 Function 0만 존재
    if (dev && !dev->multifunction)
        return -ENODEV;

    return fn + 1;
}
```

### pci_scan_single_device(): 개별 디바이스 스캔

```c
// drivers/pci/probe.c:2810-2828
struct pci_dev *pci_scan_single_device(struct pci_bus *bus, int devfn)
{
    struct pci_dev *dev;

    // 이미 발견된 디바이스인지 확인
    dev = pci_get_slot(bus, devfn);
    if (dev) {
        pci_dev_put(dev);
        return dev;     // 이미 있으면 그대로 반환
    }

    // 새 디바이스 탐색
    dev = pci_scan_device(bus, devfn);
    if (!dev)
        return NULL;

    // 발견된 디바이스를 시스템에 등록
    pci_device_add(dev, bus);

    return dev;
}
```

### pci_scan_device(): Config Space 읽기로 디바이스 존재 확인

```c
// drivers/pci/probe.c:2620-2652
static struct pci_dev *pci_scan_device(struct pci_bus *bus, int devfn)
{
    struct pci_dev *dev;
    u32 l;

    // 전원 제어 디바이스가 필요한 경우 처리 (Device Tree 기반 시스템)
    if (pci_pwrctrl_create_device(bus, devfn))
        return NULL;

    // ★★★ 핵심: Config Space offset 0x00에서 Vendor ID + Device ID 읽기
    //          타임아웃 60초 (Configuration Request Retry Status 대응)
    if (!pci_bus_read_dev_vendor_id(bus, devfn, &l, 60*1000))
        return NULL;    // 0xFFFFFFFF → 디바이스 없음

    // 디바이스 발견! pci_dev 구조체 할당
    dev = pci_alloc_dev(bus);
    if (!dev)
        return NULL;

    dev->devfn = devfn;
    dev->vendor = l & 0xffff;            // 하위 16비트: Vendor ID
    dev->device = (l >> 16) & 0xffff;    // 상위 16비트: Device ID

    // 디바이스 헤더 파싱 (BAR, Class Code, Capabilities 등)
    if (pci_setup_device(dev)) {
        pci_bus_put(dev->bus);
        kfree(dev);
        return NULL;
    }

    return dev;
}
```

**라인별 분석:**

1. **Vendor ID 읽기**: `pci_bus_read_dev_vendor_id()`가 Config Space의 첫 4바이트(offset 0x00)를 읽는다. 이 4바이트에는 Vendor ID(하위 16비트)와 Device ID(상위 16비트)가 들어있다.

2. **디바이스 부재 판정**: 해당 Bus/Device/Function에 디바이스가 없으면, PCIe 스펙에 의해 모든 비트가 1인 값(0xFFFFFFFF)이 반환된다. 또한 0x00000000, 0x0000FFFF, 0xFFFF0000도 "없음"으로 처리된다.

3. **RRS(Request Retry Status) 처리**: PCIe 디바이스가 아직 초기화 중이면 Configuration Request Retry Status를 반환할 수 있다. 이 경우 최대 60초까지 지수 백오프로 재시도한다.

### pci_bus_read_dev_vendor_id(): Vendor ID 읽기 상세

```c
// drivers/pci/probe.c:2530-2564
bool pci_bus_generic_read_dev_vendor_id(struct pci_bus *bus, int devfn,
                                        u32 *l, int timeout)
{
    // Config Space offset 0x00에서 4바이트 읽기
    //   → ECAM: readl(ecam_base + (bus<<20) + (devfn<<12) + 0)
    //   → Legacy: outl(0xCF8, ...) + inl(0xCFC)
    if (pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, l))
        return false;

    // 디바이스 없음 판정
    if (PCI_POSSIBLE_ERROR(*l) || *l == 0x00000000 ||
        *l == 0x0000ffff || *l == 0xffff0000)
        return false;

    // RRS 응답이면 재시도
    if (pci_bus_rrs_vendor_id(*l))
        return pci_bus_wait_rrs(bus, devfn, l, timeout);

    return true;
}
```

NVMe SSD의 경우, 여기서 읽히는 값의 예시:
- Samsung 980 PRO: Vendor=0x144D, Device=0xA809 → `l = 0xA809144D`
- Intel P4510: Vendor=0x8086, Device=0x0A54 → `l = 0x0A548086`

### pci_alloc_dev(): pci_dev 구조체 할당

```c
// drivers/pci/probe.c:2461-2484
struct pci_dev *pci_alloc_dev(struct pci_bus *bus)
{
    struct pci_dev *dev;

    dev = kzalloc(sizeof(struct pci_dev), GFP_KERNEL);
    if (!dev)
        return NULL;

    // bus_list: 이 디바이스가 속한 버스의 디바이스 리스트에 연결될 노드
    INIT_LIST_HEAD(&dev->bus_list);
    dev->dev.type = &pci_dev_type;
    // bus 참조 카운트 증가
    dev->bus = pci_bus_get(bus);
    // 드라이버 exclusive 리소스 초기화
    dev->driver_exclusive_resource = (struct resource) {
        .name = "PCI Exclusive",
        .start = 0,
        .end = -1,
    };

    spin_lock_init(&dev->pcie_cap_lock);
    return dev;
}
```

---

## 2.3 디바이스 헤더 파싱

`pci_scan_device()`에서 Vendor/Device ID를 확인한 후, `pci_setup_device()`가 호출되어 디바이스의 상세 정보를 Config Space에서 읽어온다.

### pci_setup_device(): 디바이스 정보 파싱의 핵심

```c
// drivers/pci/probe.c:2018-2194
int pci_setup_device(struct pci_dev *dev)
{
    u32 class;
    u16 cmd;
    u8 hdr_type;
    int err, pos = 0;
    struct pci_bus_region region;
    struct resource *res;

    // ① Header Type 읽기 (Config Space offset 0x0E)
    hdr_type = pci_hdr_type(dev);

    dev->sysdata = dev->bus->sysdata;
    dev->dev.parent = dev->bus->bridge;
    dev->dev.bus = &pci_bus_type;
    // Header Type의 하위 7비트가 실제 타입 (0=일반, 1=Bridge, 2=CardBus)
    dev->hdr_type = FIELD_GET(PCI_HEADER_TYPE_MASK, hdr_type);
    // 비트 7이 멀티펑션 플래그
    dev->multifunction = FIELD_GET(PCI_HEADER_TYPE_MFD, hdr_type);
    dev->error_state = pci_channel_io_normal;

    // ② PCIe Capability 탐색 및 설정
    set_pcie_port_type(dev);

    // ③ Device Tree / ACPI 노드 연결
    pci_set_of_node(dev);
    pci_set_acpi_fwnode(dev);
    pci_dev_assign_slot(dev);

    // 기본 DMA 마스크 32비트
    dev->dma_mask = 0xffffffff;

    // ④ 디바이스 이름 설정: "0000:01:00.0" 형태
    dev_set_name(&dev->dev, "%04x:%02x:%02x.%d",
                 pci_domain_nr(dev->bus),
                 dev->bus->number,
                 PCI_SLOT(dev->devfn),
                 PCI_FUNC(dev->devfn));

    // ⑤ Class Code + Revision 읽기 (Config Space offset 0x08)
    //    NVMe 디바이스의 경우:
    //      class = 0x01080200
    //        Base Class = 0x01 (Mass Storage Controller)
    //        Sub Class  = 0x08 (Non-Volatile Memory Controller)
    //        Prog IF    = 0x02 (NVM Express)
    //        Revision   = 0x00
    class = pci_class(dev);
    dev->revision = class & 0xff;          // 하위 8비트: Revision
    dev->class = class >> 8;               // 상위 24비트: Class Code

    // Config Space 크기 결정 (256B or 4KB)
    dev->cfg_size = pci_cfg_space_size(dev);

    // Thunderbolt, Untrusted, Removable 속성 설정
    set_pcie_thunderbolt(dev);
    set_pcie_untrusted(dev);

    // PCIe Supported Speeds 읽기
    if (pci_is_pcie(dev))
        dev->supported_speeds = pcie_get_supported_speeds(dev);

    dev->current_state = PCI_UNKNOWN;

    // Early fixup (BAR 프로빙 전)
    pci_fixup_device(pci_fixup_early, dev);
    pci_set_removable(dev);

    // 디바이스 정보 출력
    pci_info(dev, "[%04x:%04x] type %02x class %#08x %s\n",
         dev->vendor, dev->device, dev->hdr_type, dev->class,
         pci_type_str(dev));

    // ⑥ Header Type에 따라 분기
    class = dev->class >> 8;

    switch (dev->hdr_type) {
    case PCI_HEADER_TYPE_NORMAL:           // Type 0: 일반 디바이스 (NVMe 등)
        if (class == PCI_CLASS_BRIDGE_PCI)
            goto bad;
        pci_read_irq(dev);                 // INTx 인터럽트 정보 읽기
        pci_read_bases(dev, PCI_STD_NUM_BARS, PCI_ROM_ADDRESS);  // ★ BAR 파싱
        pci_subsystem_ids(dev, &dev->subsystem_vendor,
                          &dev->subsystem_device);
        break;

    case PCI_HEADER_TYPE_BRIDGE:           // Type 1: PCI-to-PCI Bridge
        pci_read_irq(dev);
        pci_read_bases(dev, 2, PCI_ROM_ADDRESS1);  // Bridge는 BAR 2개
        pci_read_bridge_windows(dev);       // Bridge Window 읽기
        set_pcie_hotplug_bridge(dev);       // Hotplug 지원 확인
        break;

    case PCI_HEADER_TYPE_CARDBUS:          // Type 2: CardBus Bridge
        pci_read_irq(dev);
        pci_read_bases(dev, 1, 0);
        break;
    }

    return 0;
}
```

### set_pcie_port_type(): PCIe Capability 파싱

```c
// drivers/pci/probe.c:1651-1717
void set_pcie_port_type(struct pci_dev *pdev)
{
    int pos;
    u16 reg16;
    u32 reg32;

    // PCIe Capability 찾기 (Capability ID = 0x10)
    pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
    if (!pos)
        return;     // PCIe 디바이스가 아님

    pdev->pcie_cap = pos;       // PCIe Capability 오프셋 저장

    // PCIe Flags 레지스터 읽기
    pci_read_config_word(pdev, pos + PCI_EXP_FLAGS, &reg16);
    pdev->pcie_flags_reg = reg16;
    // 이 레지스터에서 디바이스 타입을 추출할 수 있다:
    //   Root Port, Upstream Port, Downstream Port, Endpoint 등

    // Root Port이면 RRS(Request Retry Status) Software Visibility 활성화
    if (pci_pcie_type(pdev) == PCI_EXP_TYPE_ROOT_PORT)
        pci_enable_rrs_sv(pdev);

    // Device Capabilities 레지스터 읽기
    pci_read_config_dword(pdev, pos + PCI_EXP_DEVCAP, &pdev->devcap);
    // Max Payload Size Supported 추출
    pdev->pcie_mpss = FIELD_GET(PCI_EXP_DEVCAP_PAYLOAD, pdev->devcap);

    // Link Capabilities 읽기
    pcie_capability_read_dword(pdev, PCI_EXP_LNKCAP, &reg32);
    if (reg32 & PCI_EXP_LNKCAP_DLLLARC)
        pdev->link_active_reporting = 1;
}
```

### BAR 파싱: pci_read_bases()

BAR(Base Address Register) 파싱은 디바이스가 필요로 하는 메모리/IO 영역의 크기와 타입을 알아내는 과정이다. NVMe 디바이스는 보통 BAR0에 64비트 Memory-Mapped I/O 영역을 가진다.

```c
// drivers/pci/probe.c:345-392
static __always_inline void pci_read_bases(struct pci_dev *dev,
                       unsigned int howmany, int rom)
{
    u32 rombar, stdbars[PCI_STD_NUM_BARS];
    unsigned int pos, reg;
    u16 orig_cmd;

    // Broken BAR를 가진 디바이스는 스킵
    if (dev->non_compliant_bars)
        return;
    // SR-IOV VF의 BAR은 RO Zero
    if (dev->is_virtfn)
        return;

    // ★ BAR sizing을 위해 디코딩을 일시적으로 비활성화
    //    이유: BAR에 0xFFFFFFFF를 쓸 때 다른 디바이스와 주소 충돌 방지
    if (!dev->mmio_always_on) {
        pci_read_config_word(dev, PCI_COMMAND, &orig_cmd);
        if (orig_cmd & PCI_COMMAND_DECODE_ENABLE) {
            pci_write_config_word(dev, PCI_COMMAND,
                orig_cmd & ~PCI_COMMAND_DECODE_ENABLE);
        }
    }

    // ★★ 모든 표준 BAR의 크기를 한 번에 측정
    //     각 BAR에 0xFFFFFFFF를 쓰고 다시 읽어 마스크 값을 얻는다
    __pci_size_stdbars(dev, howmany, PCI_BASE_ADDRESS_0, stdbars);
    if (rom)
        __pci_size_rom(dev, rom, &rombar);

    // 디코딩 복원
    if (!dev->mmio_always_on &&
        (orig_cmd & PCI_COMMAND_DECODE_ENABLE))
        pci_write_config_word(dev, PCI_COMMAND, orig_cmd);

    // 각 BAR 해석
    for (pos = 0; pos < howmany; pos++) {
        struct resource *res = &dev->resource[pos];
        reg = PCI_BASE_ADDRESS_0 + (pos << 2);
        // __pci_read_base()가 BAR 값과 마스크를 해석하여
        // resource 구조체를 채운다
        pos += __pci_read_base(dev, pci_bar_unknown,
                       res, reg, &stdbars[pos]);
    }

    // ROM BAR 처리
    if (rom) {
        struct resource *res = &dev->resource[PCI_ROM_RESOURCE];
        dev->rom_base_reg = rom;
        res->flags = IORESOURCE_MEM | IORESOURCE_PREFETCH |
                IORESOURCE_READONLY | IORESOURCE_SIZEALIGN;
        __pci_read_base(dev, pci_bar_mem32, res, rom, &rombar);
    }
}
```

### __pci_size_stdbars(): BAR 크기 측정 (Write-Read-Back 방식)

```c
// drivers/pci/probe.c:184-196
static void __pci_size_bars(struct pci_dev *dev, int count,
                unsigned int pos, u32 *sizes, bool rom)
{
    u32 orig, mask = rom ? PCI_ROM_ADDRESS_MASK : ~0;
    int i;

    for (i = 0; i < count; i++, pos += 4, sizes++) {
        // ① 현재 BAR 값 저장
        pci_read_config_dword(dev, pos, &orig);
        // ② BAR에 0xFFFFFFFF (또는 ROM 마스크) 기록
        pci_write_config_dword(dev, pos, mask);
        // ③ BAR에서 마스크 값 읽기 → 디바이스가 구현한 비트만 1로 남음
        pci_read_config_dword(dev, pos, sizes);
        // ④ 원래 값 복원
        pci_write_config_dword(dev, pos, orig);
    }
}
```

**BAR Sizing의 원리:**

PCI 스펙에 의해 BAR의 크기를 알아내는 방법이 정해져 있다:
1. BAR에 0xFFFFFFFF를 쓴다.
2. BAR을 다시 읽는다.
3. 디바이스가 구현한 주소 비트만 1로 남고, 하위의 "don't care" 비트는 0이 된다.
4. 가장 낮은 1 비트의 위치가 BAR의 크기를 결정한다.

예시 (NVMe BAR0, 16KB 크기의 Memory BAR):
```
Write: 0xFFFFFFFF
Read:  0xFFFFC004
       ││││└─── 비트 2: Memory BAR 표시
       ││││     비트 1-0: 64비트 Memory 타입
       └┘└┘──── 비트 14 이상이 1 → 크기 = 2^14 = 16KB
```

### __pci_read_base(): BAR 값 해석

```c
// drivers/pci/probe.c:219-343
int __pci_read_base(struct pci_dev *dev, enum pci_bar_type type,
            struct resource *res, unsigned int pos, u32 *sizes)
{
    u32 l = 0, sz;
    u64 l64, sz64, mask64;
    struct pci_bus_region region, inverted_region;

    res->name = pci_name(dev);

    // BAR의 현재 값 읽기
    pci_read_config_dword(dev, pos, &l);
    sz = sizes[0];    // 이전에 측정한 마스크 값

    // 에러 체크: 모든 비트가 1이면 디바이스 오류
    if (PCI_POSSIBLE_ERROR(sz))
        sz = 0;
    if (PCI_POSSIBLE_ERROR(l))
        l = 0;

    if (type == pci_bar_unknown) {
        // BAR 타입 판별 (Memory vs I/O, 32비트 vs 64비트)
        res->flags = decode_bar(dev, l);
        res->flags |= IORESOURCE_SIZEALIGN;

        if (res->flags & IORESOURCE_IO) {
            l64 = l & PCI_BASE_ADDRESS_IO_MASK;
            sz64 = sz & PCI_BASE_ADDRESS_IO_MASK;
            mask64 = PCI_BASE_ADDRESS_IO_MASK & (u32)IO_SPACE_LIMIT;
        } else {
            l64 = l & PCI_BASE_ADDRESS_MEM_MASK;
            sz64 = sz & PCI_BASE_ADDRESS_MEM_MASK;
            mask64 = (u32)PCI_BASE_ADDRESS_MEM_MASK;
        }
    }

    // 64비트 BAR이면 상위 32비트도 읽기
    if (res->flags & IORESOURCE_MEM_64) {
        pci_read_config_dword(dev, pos + 4, &l);
        sz = sizes[1];
        l64 |= ((u64)l << 32);
        sz64 |= ((u64)sz << 32);
        mask64 |= ((u64)~0 << 32);
    }

    if (!sz64)
        goto fail;    // BAR이 구현되지 않음

    // 크기 계산: 마스크에서 가장 낮은 set bit 추출
    sz64 = pci_size(l64, sz64, mask64);

    // Bus Address → Resource Address 변환
    region.start = l64;
    region.end = l64 + sz64 - 1;
    pcibios_bus_to_resource(dev->bus, res, &region);

    // 결과 출력
    pci_info(dev, "%s %pR\n", res_name, res);
    // 64비트 BAR이면 1 반환 (다음 BAR 슬롯을 건너뛰기 위해)
    return (res->flags & IORESOURCE_MEM_64) ? 1 : 0;
}
```

### decode_bar(): BAR 타입 판별

```c
// drivers/pci/probe.c:135-166
static inline unsigned long decode_bar(struct pci_dev *dev, u32 bar)
{
    u32 mem_type;
    unsigned long flags;

    // 비트 0: 0=Memory, 1=I/O
    if ((bar & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
        flags = bar & ~PCI_BASE_ADDRESS_IO_MASK;
        flags |= IORESOURCE_IO;
        return flags;
    }

    // Memory BAR
    flags = bar & ~PCI_BASE_ADDRESS_MEM_MASK;
    flags |= IORESOURCE_MEM;
    // 비트 3: Prefetchable
    if (flags & PCI_BASE_ADDRESS_MEM_PREFETCH)
        flags |= IORESOURCE_PREFETCH;

    // 비트 2:1 = Memory Type
    mem_type = bar & PCI_BASE_ADDRESS_MEM_TYPE_MASK;
    switch (mem_type) {
    case PCI_BASE_ADDRESS_MEM_TYPE_32:     // 00: 32비트 주소
        break;
    case PCI_BASE_ADDRESS_MEM_TYPE_1M:     // 01: 1MB 이하 (레거시)
        break;
    case PCI_BASE_ADDRESS_MEM_TYPE_64:     // 10: 64비트 주소 ← NVMe
        flags |= IORESOURCE_MEM_64;
        break;
    }
    return flags;
}
```

### pci_size(): BAR 크기 계산

```c
// drivers/pci/probe.c:113-133
static u64 pci_size(u64 base, u64 maxbase, u64 mask)
{
    // maxbase: BAR에 ~0 쓰고 읽은 값 (마스크)
    // mask: 유효 비트 마스크
    u64 size = mask & maxbase;

    if (!size)
        return 0;

    // 가장 낮은 set bit를 찾아 크기 결정
    // 예: size = 0xFFFFC000 → size & ~(size-1) = 0x00004000 = 16KB
    size = size & ~(size-1);

    // base == maxbase인 경우 (이미 all-ones로 프로그래밍된 경우) 검증
    if (base == maxbase && ((base | (size - 1)) & mask) != mask)
        return 0;

    return size;
}
```

### Capability 체인 파싱: pci_init_capabilities()

`pci_device_add()` 과정에서 `pci_init_capabilities()`가 호출되어 디바이스의 모든 PCI/PCIe Capability를 파싱하고 초기화한다.

```c
// drivers/pci/probe.c:2682-2712
static void pci_init_capabilities(struct pci_dev *dev)
{
    pci_ea_init(dev);            // Enhanced Allocation
    pci_msi_init(dev);           // MSI Capability → 오프셋 저장 + 비활성화
    pci_msix_init(dev);          // MSI-X Capability → 오프셋 저장 + 비활성화

    // PCIe/PCI-X Capability 저장 버퍼 할당
    pci_allocate_cap_save_buffers(dev);

    pci_imm_ready_init(dev);     // Immediate Readiness
    pci_pm_init(dev);            // Power Management Capability
    pci_vpd_init(dev);           // Vital Product Data
    pci_configure_ari(dev);      // Alternative Routing-ID Forwarding
    pci_iov_init(dev);           // SR-IOV (Single Root I/O Virtualization)
    pci_ats_init(dev);           // Address Translation Services
    pci_pri_init(dev);           // Page Request Interface
    pci_pasid_init(dev);         // Process Address Space ID
    pci_acs_init(dev);           // Access Control Services
    pci_ptm_init(dev);           // Precision Time Measurement
    pci_aer_init(dev);           // Advanced Error Reporting
    pci_dpc_init(dev);           // Downstream Port Containment
    pci_rcec_init(dev);          // Root Complex Event Collector
    pci_doe_init(dev);           // Data Object Exchange
    pci_tph_init(dev);           // TLP Processing Hints
    pci_rebar_init(dev);         // Resizable BAR
    pci_dev3_init(dev);          // Device 3 Capabilities
    pci_ide_init(dev);           // Link Integrity and Data Encryption

    pcie_report_downtraining(dev);
    pci_init_reset_methods(dev);
}
```

NVMe 디바이스에서 특히 중요한 Capability:
- **MSI/MSI-X**: NVMe는 MSI-X를 사용하여 Completion Queue별로 인터럽트를 할당한다.
- **PCIe Capability**: Link Speed/Width, Max Payload Size 정보를 담고 있다.
- **PM Capability**: D0~D3 전원 상태 전환을 지원한다.
- **AER**: PCIe 에러 리포팅을 위해 사용된다.

---

## 2.4 BAR 리소스 할당 (Resource Assignment)

디바이스가 발견되고 BAR 크기가 파악된 후, 실제 물리 주소를 BAR에 할당하는 과정이 필요하다. 이 과정은 부트 시에는 `acpi_pci_root_add()`에서 `pci_assign_unassigned_root_bus_resources()`로, 또는 `pci_host_probe()`에서 수행된다.

BIOS가 이미 할당한 BAR 주소가 유효하면 그대로 사용하고, 그렇지 않으면 커널이 새로 할당한다.

### pci_assign_resource(): 개별 리소스 할당

```c
// drivers/pci/setup-res.c:324-371
int pci_assign_resource(struct pci_dev *dev, int resno)
{
    struct resource *res = pci_resource_n(dev, resno);
    resource_size_t align, size;
    int ret;

    // 고정 리소스(IORESOURCE_PCI_FIXED)는 변경하지 않음
    if (res->flags & IORESOURCE_PCI_FIXED)
        return 0;

    // UNSET 플래그 설정 (할당 진행 중)
    res->flags |= IORESOURCE_UNSET;

    // 정렬 요구사항 계산
    align = pci_resource_alignment(dev, res);
    if (!align) {
        pci_info(dev, "%s: can't assign; bogus alignment\n", res_name);
        return -EINVAL;
    }

    size = resource_size(res);

    // ★ 실제 주소 할당 시도
    ret = _pci_assign_resource(dev, resno, size, align);

    // 실패 시 → BIOS가 남긴 주소로 되돌리기 시도
    if (ret < 0) {
        pci_info(dev, "%s: can't assign; no space\n", res_name);
        ret = pci_revert_fw_address(res, dev, resno, size);
    }

    if (ret < 0) {
        pci_info(dev, "%s: failed to assign\n", res_name);
        return ret;
    }

    // 할당 성공
    res->flags &= ~IORESOURCE_UNSET;
    res->flags &= ~IORESOURCE_STARTALIGN;

    pci_info(dev, "%s %pR: assigned\n", res_name, res);

    // ★ Config Space의 BAR 레지스터에 할당된 주소 기록
    if (resno < PCI_BRIDGE_RESOURCES)
        pci_update_resource(dev, resno);

    return 0;
}
```

### __pci_assign_resource(): 버스 리소스 풀에서 할당

```c
// drivers/pci/setup-res.c:260-306
static int __pci_assign_resource(struct pci_bus *bus, struct pci_dev *dev,
        int resno, resource_size_t size, resource_size_t align)
{
    struct resource *res = pci_resource_n(dev, resno);
    resource_size_t min;
    int ret;

    // I/O면 PCIBIOS_MIN_IO, Memory면 PCIBIOS_MIN_MEM부터 할당
    min = (res->flags & IORESOURCE_IO) ? PCIBIOS_MIN_IO : PCIBIOS_MIN_MEM;

    // ① 첫 번째 시도: Prefetchable + 64비트 정확 매칭
    ret = pci_bus_alloc_resource(bus, res, size, align, min,
                     IORESOURCE_PREFETCH | IORESOURCE_MEM_64,
                     pcibios_align_resource, dev);
    if (ret == 0)
        return 0;

    // ② 두 번째 시도: 64비트 Prefetchable → 32비트 Prefetchable 윈도우 허용
    if ((res->flags & (IORESOURCE_PREFETCH | IORESOURCE_MEM_64)) ==
         (IORESOURCE_PREFETCH | IORESOURCE_MEM_64)) {
        ret = pci_bus_alloc_resource(bus, res, size, align, min,
                         IORESOURCE_PREFETCH,
                         pcibios_align_resource, dev);
        if (ret == 0)
            return 0;
    }

    // ③ 세 번째 시도: Non-prefetchable 윈도우에서 할당
    if (res->flags & (IORESOURCE_PREFETCH | IORESOURCE_MEM_64))
        ret = pci_bus_alloc_resource(bus, res, size, align, min, 0,
                         pcibios_align_resource, dev);

    return ret;
}
```

### pci_update_resource(): BAR 레지스터에 주소 기록

할당된 물리 주소를 Config Space의 BAR 레지스터에 실제로 기록하는 함수다.

```c
// drivers/pci/setup-res.c:25-124
static void pci_std_update_resource(struct pci_dev *dev, int resno)
{
    struct pci_bus_region region;
    bool disable;
    u16 cmd;
    u32 new, check, mask;
    int reg;
    struct resource *res = pci_resource_n(dev, resno);

    // VF BAR은 RO Zero → 기록 불가
    if (dev->is_virtfn)
        return;
    // 미구현 BAR 스킵
    if (!res->flags)
        return;
    // 미할당 BAR 스킵
    if (res->flags & IORESOURCE_UNSET)
        return;
    // 고정 BAR 스킵
    if (res->flags & IORESOURCE_PCI_FIXED)
        return;

    // Resource Address → Bus Address 변환
    pcibios_resource_to_bus(dev->bus, &region, res);
    new = region.start;

    // BAR 타입에 따른 마스크 및 하위 비트 설정
    if (res->flags & IORESOURCE_IO) {
        mask = (u32)PCI_BASE_ADDRESS_IO_MASK;
        new |= res->flags & ~PCI_BASE_ADDRESS_IO_MASK;
    } else {
        mask = (u32)PCI_BASE_ADDRESS_MEM_MASK;
        new |= res->flags & ~PCI_BASE_ADDRESS_MEM_MASK;
    }

    // BAR 레지스터 오프셋 계산
    reg = PCI_BASE_ADDRESS_0 + 4 * resno;

    // ★ 64비트 BAR의 경우 원자적 업데이트가 불가능하므로
    //    Memory 디코딩을 일시적으로 비활성화
    disable = (res->flags & IORESOURCE_MEM_64) && !dev->mmio_always_on;
    if (disable) {
        pci_read_config_word(dev, PCI_COMMAND, &cmd);
        pci_write_config_word(dev, PCI_COMMAND,
                      cmd & ~PCI_COMMAND_MEMORY);
    }

    // ★ BAR 하위 32비트 기록
    pci_write_config_dword(dev, reg, new);
    pci_read_config_dword(dev, reg, &check);
    if ((new ^ check) & mask) {
        pci_err(dev, "BAR: error updating (%#010x != %#010x)\n",
            new, check);
    }

    // ★ 64비트 BAR이면 상위 32비트도 기록
    if (res->flags & IORESOURCE_MEM_64) {
        new = region.start >> 16 >> 16;
        pci_write_config_dword(dev, reg + 4, new);
        pci_read_config_dword(dev, reg + 4, &check);
        if (check != new) {
            pci_err(dev, "BAR: error updating high (%#010x != %#010x)\n",
                new, check);
        }
    }

    // 디코딩 복원
    if (disable)
        pci_write_config_word(dev, PCI_COMMAND, cmd);
}
```

---

## 2.5 PCIe Bridge 처리 (재귀 스캔)

Bus 스캔 중 Header Type 1(PCI-to-PCI Bridge)인 디바이스를 만나면, 그 Bridge 아래의 하위 버스를 재귀적으로 스캔해야 한다.

### pci_scan_bridge_extend(): Bridge 스캔

```c
// drivers/pci/probe.c:1376-1602 (핵심 로직 요약)
static int pci_scan_bridge_extend(struct pci_bus *bus, struct pci_dev *dev,
                  int max, unsigned int available_buses,
                  int pass)
{
    struct pci_bus *child;
    u32 buses;
    u8 primary, secondary, subordinate;

    // Bridge의 Config Space에서 버스 번호 읽기
    //   PCI_PRIMARY_BUS (offset 0x18): 상위 버스 번호
    //   Secondary Bus (offset 0x19): 하위 버스 시작 번호
    //   Subordinate Bus (offset 0x1A): 하위 버스 최대 번호
    pci_read_config_dword(dev, PCI_PRIMARY_BUS, &buses);
    primary = buses & 0xFF;
    secondary = (buses >> 8) & 0xFF;
    subordinate = (buses >> 16) & 0xFF;

    // ─── Pass 0: BIOS가 이미 설정한 Bridge ───
    if ((secondary || subordinate) && !broken) {
        if (pass)
            goto out;   // Pass 1에서는 스킵

        // 하위 버스가 이미 존재하는지 확인
        child = pci_find_bus(pci_domain_nr(bus), secondary);
        if (!child) {
            // ★ 새 PCI Bus 생성
            child = pci_add_new_bus(bus, dev, secondary);
            if (!child) goto out;
            child->primary = primary;
            pci_bus_insert_busn_res(child, secondary, subordinate);
            child->bridge_ctl = bctl;
        }

        // ★★ 재귀: 하위 버스 스캔
        cmax = pci_scan_child_bus_extend(child, subordinate - secondary);
    }

    // ─── Pass 1: 새로 설정해야 하는 Bridge ───
    else {
        if (!pass) goto out;  // Pass 0에서는 스킵

        // 새 버스 번호 할당
        next_busnr = max + 1;

        child = pci_add_new_bus(bus, dev, next_busnr);
        if (!child) goto out;
        max++;

        // Bridge의 Primary/Secondary/Subordinate 레지스터에 기록
        buses = ((unsigned int)(child->primary)        <<  0)
              | ((unsigned int)(child->busn_res.start)  <<  8)
              | ((unsigned int)(child->busn_res.end)    << 16);
        pci_write_config_dword(dev, PCI_PRIMARY_BUS, buses);

        // ★★ 재귀: 하위 버스 스캔
        max = pci_scan_child_bus_extend(child, available_buses);

        // Subordinate 번호를 최종값으로 업데이트
        pci_bus_update_busn_res_end(child, max);
        pci_write_config_byte(dev, PCI_SUBORDINATE_BUS, max);
    }

    return max;
}
```

### pci_add_new_bus(): 새 하위 버스 생성

```c
// drivers/pci/probe.c:1286-1298
struct pci_bus *pci_add_new_bus(struct pci_bus *parent, struct pci_dev *dev,
                int busnr)
{
    struct pci_bus *child;

    // 하위 버스 구조체 할당 및 초기화
    child = pci_alloc_child_bus(parent, dev, busnr);
    if (child) {
        // 부모 버스의 children 리스트에 추가
        down_write(&pci_bus_sem);
        list_add_tail(&child->node, &parent->children);
        up_write(&pci_bus_sem);
    }
    return child;
}
```

`pci_alloc_child_bus()` 내부에서:

```c
// drivers/pci/probe.c:1201-1284 (핵심 부분)
static struct pci_bus *pci_alloc_child_bus(struct pci_bus *parent,
                       struct pci_dev *bridge, int busnr)
{
    struct pci_bus *child;

    child = pci_alloc_bus(parent);
    child->parent = parent;
    child->sysdata = parent->sysdata;     // sysdata 상속
    child->bus_flags = parent->bus_flags;  // Bus flags 상속
    child->ops = parent->ops;             // Config Space ops 상속

    // 버스 번호 설정
    child->number = child->busn_res.start = busnr;
    child->primary = parent->busn_res.start;
    child->busn_res.end = 0xff;

    child->self = bridge;                 // 이 버스의 상위 Bridge
    child->bridge = get_device(&bridge->dev);

    // Bridge의 resource를 하위 버스의 window로 설정
    for (i = 0; i < PCI_BRIDGE_RESOURCE_NUM; i++) {
        child->resource[i] = &bridge->resource[PCI_BRIDGE_RESOURCES+i];
        child->resource[i]->name = child->name;
    }
    bridge->subordinate = child;          // Bridge → 하위 버스 포인터

    // 디바이스 등록
    device_register(&child->dev);

    return child;
}
```

### 재귀 스캔 시각화

일반적인 NVMe SSD가 장착된 시스템의 토폴로지 예시:

```
Bus 0 (Root Bus)
├── Device 00:00.0 - Host Bridge (CPU내장)
├── Device 00:01.0 - PCIe Root Port (Bridge)
│   └── Bus 1
│       └── Device 01:00.0 - NVMe SSD ★
├── Device 00:02.0 - 내장 그래픽
├── Device 00:1c.0 - PCIe Root Port (Bridge)
│   └── Bus 2
│       └── Device 02:00.0 - 네트워크 카드
└── Device 00:1f.0 - ISA Bridge (LPC)
```

스캔 과정:
1. Bus 0에서 Device 0~31을 스캔 → 00:00.0, 00:01.0, 00:02.0, 00:1c.0, 00:1f.0 발견
2. 00:01.0이 Bridge → Pass 0에서 Bus 1 생성 → Bus 1 재귀 스캔 → 01:00.0 (NVMe) 발견
3. 00:1c.0이 Bridge → Pass 0에서 Bus 2 생성 → Bus 2 재귀 스캔 → 02:00.0 발견

---

## 2.6 디바이스 등록 (sysfs)

모든 버스 스캔이 완료되고 리소스가 할당된 후, `acpi_pci_root_add()`에서 `pci_bus_add_devices()`가 호출되어 발견된 모든 디바이스를 sysfs에 등록하고 드라이버 프로빙을 시작한다.

### pci_bus_add_devices(): 디바이스 일괄 등록

```c
// drivers/pci/bus.c:397-418
void pci_bus_add_devices(const struct pci_bus *bus)
{
    struct pci_dev *dev;
    struct pci_bus *child;

    // ① 현재 버스의 모든 디바이스를 등록
    list_for_each_entry(dev, &bus->devices, bus_list) {
        if (pci_dev_is_added(dev))
            continue;               // 이미 등록된 디바이스는 스킵
        pci_bus_add_device(dev);    // 개별 디바이스 등록
    }

    // ② 하위 버스의 디바이스도 재귀적으로 등록
    list_for_each_entry(dev, &bus->devices, bus_list) {
        if (!pci_dev_is_added(dev))
            continue;
        child = dev->subordinate;
        if (child)
            pci_bus_add_devices(child);  // 재귀
    }
}
```

### pci_bus_add_device(): 개별 디바이스 등록

```c
// drivers/pci/bus.c:344-388
void pci_bus_add_device(struct pci_dev *dev)
{
    // ① 아키텍처별 디바이스 추가 처리
    pcibios_bus_add_device(dev);

    // ② Final fixup 적용
    pci_fixup_device(pci_fixup_final, dev);

    // ③ sysfs 파일 생성
    //    /sys/bus/pci/devices/0000:01:00.0/ 아래에:
    //    - config (Config Space 접근)
    //    - resource (리소스 정보)
    //    - vendor, device, class 등
    pci_create_sysfs_dev_files(dev);

    // ④ /proc/bus/pci/ 엔트리 생성
    pci_proc_attach_device(dev);

    // ⑤ D3 전원 관리 업데이트
    pci_bridge_d3_update(dev);

    // ⑥ Config Space 상태 저장 (에러 복구용)
    pci_save_state(dev);

    // ⑦ ★★ 드라이버 바인딩 허용 + 초기 프로빙 시작
    pci_dev_allow_binding(dev);
    device_initial_probe(&dev->dev);

    // ⑧ 등록 완료 표시
    pci_dev_assign_added(dev);
}
```

`device_initial_probe()`가 호출되면, PCI 버스의 `match` 콜백이 등록된 PCI 드라이버들의 `id_table`과 디바이스의 Vendor/Device/Class를 비교한다. NVMe 디바이스(Class 0x010802)가 `nvme` 드라이버의 `id_table`과 매칭되면, `nvme_probe()`가 호출되어 NVMe 컨트롤러 초기화가 시작된다.

### sysfs 디렉토리 구조

디바이스 등록 후 생성되는 sysfs 구조:

```
/sys/bus/pci/devices/0000:01:00.0/
├── class          → 0x010802
├── vendor         → 0x144d
├── device         → 0xa809
├── config         → (Config Space 256 or 4096 bytes)
├── resource       → (BAR 리소스 정보)
├── resource0      → (BAR0 mmap 가능)
├── irq            → (할당된 인터럽트 번호)
├── enable         → (디바이스 활성화 상태)
├── driver/        → (바인딩된 드라이버 심볼릭 링크)
├── msi_irqs/      → (MSI/MSI-X 인터럽트 정보)
└── ...
```

또한 uevent가 발생하여 udev/systemd에 알림이 전달된다. udev 규칙에 따라 `/dev/nvme0` 같은 디바이스 노드가 생성될 수 있다.

---

## 요약: NVMe SSD가 발견되는 전체 과정

```
1. BIOS POST 완료 → 커널 로드
2. start_kernel() → setup_arch()
3. pci_mmcfg_early_init() → MCFG 파싱 → ECAM ioremap
4. do_initcalls()
   ├── postcore: pcibus_class_init() → /sys/class/pci_bus/ 생성
   ├── subsys: acpi_pci_root_init() → PCI Root Handler 등록
   └── ACPI 스캔 → PNP0A03 발견 → acpi_pci_root_add() 호출
5. acpi_pci_root_add()
   ├── _SEG, _CRS, _BBN으로 Segment/Bus 정보 추출
   ├── negotiate_os_control() → _OSC 협상
   └── pci_acpi_scan_root() → acpi_pci_root_create()
6. acpi_pci_root_create()
   ├── pci_create_root_bus() → Host Bridge + Bus 0 생성
   └── pci_scan_child_bus() → 재귀 스캔 시작
7. Bus 0 스캔:
   ├── Slot 0: Host Bridge 발견
   ├── Slot 1: Root Port (Bridge) 발견 → Bus 1 생성 → 재귀
   │   └── Bus 1, Slot 0: NVMe SSD 발견! ★
   │       ├── Vendor ID: 0x144D (Samsung)
   │       ├── Class: 0x010802 (NVMe)
   │       ├── BAR0: 64-bit MMIO, 16KB
   │       ├── MSI-X Capability 발견
   │       └── pci_dev 구조체 생성 + 등록
   └── (나머지 슬롯들...)
8. pci_assign_unassigned_root_bus_resources()
   └── NVMe BAR0에 물리 주소 할당 (예: 0xFB000000)
9. pci_bus_add_devices()
   ├── sysfs 생성: /sys/bus/pci/devices/0000:01:00.0/
   ├── uevent 발생
   └── device_initial_probe() → nvme 드라이버 매칭 → nvme_probe() 호출
```

이 시점부터 NVMe 드라이버가 BAR0을 ioremap()하여 NVMe 레지스터에 접근하고, Admin Queue를 생성하며, Identify 명령으로 컨트롤러와 네임스페이스 정보를 읽어오는 Phase 3가 시작된다.
