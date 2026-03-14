# QEMU PCIe ↔ NVMe 에뮬레이션 연결점 심층 분석

> QEMU 소스: `~/sources/qemu/hw/nvme/ctrl.c`, `hw/nvme/nvme.h`, `include/block/nvme.h`
> PCIe 인프라: `hw/pci/pci.c`, `hw/pci/msix.c`, `include/hw/pci/pci_device.h`

## 목차
1. QEMU PCIe 디바이스 에뮬레이션 기초
2. BAR 등록과 MMIO Trap 메커니즘
3. DMA 에뮬레이션
4. MSI-X 인터럽트 에뮬레이션
5. CMB (Controller Memory Buffer) 에뮬레이션
6. PCIe Config Space 에뮬레이션
7. 전체 연결 다이어그램
8. Linux 커널 NVMe 드라이버 코드와의 1:1 대응

---

## 1. QEMU PCIe 디바이스 에뮬레이션 기초

### 1.1 QOM (QEMU Object Model) 타입 계층

QEMU의 모든 디바이스는 QOM이라는 객체 시스템을 통해 정의된다. NVMe 컨트롤러의 타입 계층은 다음과 같다:

```
TYPE_OBJECT
  └── TYPE_DEVICE (DeviceState)
        └── TYPE_PCI_DEVICE (PCIDevice)
              └── TYPE_NVME ("nvme") (NvmeCtrl)
```

`ctrl.c` 파일 맨 아래에 타입 등록 코드가 있다:

```c
/* ctrl.c:9542 */
static const TypeInfo nvme_info = {
    .name          = TYPE_NVME,          /* "nvme" 문자열 */
    .parent        = TYPE_PCI_DEVICE,    /* PCIDevice를 상속 */
    .instance_size = sizeof(NvmeCtrl),   /* 인스턴스 크기 */
    .instance_init = nvme_instance_init, /* 인스턴스 초기화 콜백 */
    .class_init    = nvme_class_init,    /* 클래스 초기화 콜백 */
    .interfaces = (const InterfaceInfo[]) {
        { INTERFACE_PCIE_DEVICE },       /* PCIe 디바이스 인터페이스 구현 */
        { }
    },
};

/* ctrl.c:9560 */
static void nvme_register_types(void)
{
    type_register_static(&nvme_info);
    type_register_static(&nvme_bus_info);
}
type_init(nvme_register_types)   /* QEMU 부팅 시 자동 호출 */
```

**핵심 포인트:**
- `TYPE_PCI_DEVICE`를 상속하므로 PCIe 디바이스의 모든 인프라(Config Space, BAR, 인터럽트)를 자동으로 물려받는다.
- `INTERFACE_PCIE_DEVICE`를 선언하여 PCIe 엔드포인트임을 명시한다.
- `type_init()` 매크로가 QEMU 시작 시 `nvme_register_types()`를 호출하여 타입을 등록한다.

### 1.2 NvmeCtrl 구조체: PCIDevice 임베딩

`nvme.h`에서 NvmeCtrl 구조체의 첫 번째 필드가 핵심이다:

```c
/* nvme.h:577 */
typedef struct NvmeCtrl {
    PCIDevice    parent_obj;    /* ★ PCIDevice를 첫 번째 필드로 임베딩 */
    MemoryRegion bar0;          /* BAR0 컨테이너 MemoryRegion */
    MemoryRegion iomem;         /* MMIO 핸들러가 연결된 MemoryRegion */
    NvmeBar      bar;           /* NVMe 레지스터 값 저장 (소프트웨어 shadow) */
    NvmeParams   params;        /* 사용자 설정 파라미터 */
    NvmeBus      bus;           /* NVMe 버스 (네임스페이스 연결용) */

    /* ... SQ/CQ 배열, CMB, PMR 등 ... */

    struct {
        MemoryRegion mem;       /* CMB MemoryRegion */
        uint8_t      *buf;      /* CMB 버퍼 (malloc 메모리) */
        bool         cmse;      /* CMB Space Enable */
        hwaddr       cba;       /* CMB Base Address */
    } cmb;

    NvmeSQueue      **sq;       /* Submission Queue 배열 */
    NvmeCQueue      **cq;       /* Completion Queue 배열 */
    NvmeSQueue      admin_sq;   /* Admin Submission Queue */
    NvmeCQueue      admin_cq;   /* Admin Completion Queue */
    /* ... */
} NvmeCtrl;
```

C 언어에서 구조체의 첫 번째 필드 주소는 구조체 자체의 주소와 동일하다. 따라서 `NvmeCtrl *`를 `PCIDevice *`로 캐스팅할 수 있고, 반대로도 가능하다. 이것이 QOM의 "상속" 구현 원리이다.

```
NvmeCtrl 메모리 레이아웃:
┌──────────────────────────┐ ← NvmeCtrl * == PCIDevice *
│  PCIDevice parent_obj    │
│  ┌────────────────────┐  │
│  │ DeviceState        │  │
│  │ config[256+4096]   │  │  ← PCI Config Space
│  │ io_regions[7]      │  │  ← BAR 정보
│  │ msix_table         │  │  ← MSI-X 테이블
│  │ ...                │  │
│  └────────────────────┘  │
├──────────────────────────┤
│  MemoryRegion bar0       │  ← BAR0 컨테이너
│  MemoryRegion iomem      │  ← MMIO ops 연결
│  NvmeBar      bar        │  ← 레지스터 shadow copy
│  NvmeSQueue **sq         │  ← SQ 포인터 배열
│  NvmeCQueue **cq         │  ← CQ 포인터 배열
│  struct cmb { ... }      │  ← CMB 관련
│  ...                     │
└──────────────────────────┘
```

### 1.3 클래스 초기화: nvme_class_init()

```c
/* ctrl.c:9510 */
static void nvme_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(oc);

    pc->realize = nvme_realize;          /* ★ 디바이스 실체화 콜백 */
    pc->config_write = nvme_pci_write_config;  /* PCI Config 쓰기 핸들러 */
    pc->config_read = nvme_pci_read_config;    /* PCI Config 읽기 핸들러 */
    pc->exit = nvme_exit;                /* 디바이스 제거 콜백 */
    pc->class_id = PCI_CLASS_STORAGE_EXPRESS;   /* NVMe 클래스 코드 */
    pc->revision = 2;                    /* PCIe revision */

    set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
    dc->desc = "Non-Volatile Memory Express";
    device_class_set_props(dc, nvme_props);    /* QEMU 명령줄 속성 */
    dc->vmsd = &nvme_vmstate;
    device_class_set_legacy_reset(dc, nvme_pci_reset);
}
```

**`pc->realize = nvme_realize`가 가장 중요하다.** QEMU가 `-device nvme,...` 명령줄 옵션을 처리할 때 최종적으로 이 함수를 호출하여 디바이스를 "실체화"한다.

### 1.4 디바이스 생성 전체 흐름

```
QEMU 명령줄: -device nvme,drive=...,serial=...
  │
  ▼
qdev_device_add()
  │
  ▼
object_new(TYPE_NVME)
  ├── 메모리 할당: g_malloc0(sizeof(NvmeCtrl))
  ├── object_init_with_type() → 부모 타입부터 순서대로:
  │     TYPE_OBJECT → init
  │     TYPE_DEVICE → init
  │     TYPE_PCI_DEVICE → init (PCI Config Space 초기화)
  │     TYPE_NVME → nvme_instance_init()
  │
  ▼
device_set_realized(true)
  │
  ▼
pci_qdev_realize()           ← PCIDeviceClass의 공통 realize
  ├── pci_dev 할당 및 초기화
  ├── Config Space 기본값 설정
  │
  ▼
pc->realize(pci_dev, errp)   ← nvme_realize() 호출!
  ├── nvme_check_params()    ← 파라미터 검증
  ├── qbus_init()            ← NVMe 버스 생성
  ├── nvme_init_subsys()     ← 서브시스템 초기화
  ├── nvme_init_state()      ← 내부 상태 초기화
  ├── nvme_init_pci()        ← ★★★ BAR 등록, MSI-X 설정
  ├── nvme_init_ctrl()       ← 컨트롤러 ID 설정, CAP 레지스터 초기화
  └── nvme_ns_setup()        ← 네임스페이스 설정
```

---

## 2. BAR 등록과 MMIO Trap 메커니즘 (핵심!)

### 2.1 NVMe가 BAR0를 등록하는 과정

`nvme_init_pci()` 함수가 BAR0 등록의 핵심이다. 두 가지 모드가 있다:

#### 모드 A: MSI-X 전용 BAR 사용 (msix_exclusive_bar=true)

```c
/* ctrl.c:8968-8974 */
if (n->params.msix_exclusive_bar && !pci_is_vf(pci_dev)) {
    /* BAR0 크기 = Doorbell 영역까지만 */
    bar_size = nvme_mbar_size(n->params.max_ioqpairs + 1, 0, NULL, NULL);

    /* ① MemoryRegion 초기화: nvme_mmio_ops를 MMIO 핸들러로 등록 */
    memory_region_init_io(&n->iomem,       /* MemoryRegion 포인터 */
                          OBJECT(n),        /* 소유자 객체 */
                          &nvme_mmio_ops,   /* ★ read/write 콜백 구조체 */
                          n,                /* opaque: 콜백에 전달될 포인터 */
                          "nvme",           /* 이름 (디버깅용) */
                          bar_size);        /* 영역 크기 */

    /* ② BAR0으로 등록 */
    pci_register_bar(pci_dev, 0,           /* BAR 번호 = 0 */
                     PCI_BASE_ADDRESS_SPACE_MEMORY |  /* Memory Space */
                     PCI_BASE_ADDRESS_MEM_TYPE_64,    /* 64-bit BAR */
                     &n->iomem);           /* 연결할 MemoryRegion */

    /* ③ MSI-X를 별도의 BAR4에 설정 */
    ret = msix_init_exclusive_bar(pci_dev, n->params.msix_qsize, 4, errp);
}
```

#### 모드 B: BAR0에 MMIO + MSI-X 공존 (기본 모드)

```c
/* ctrl.c:8993-9003 */
else {
    /* BAR0 컨테이너: MMIO + MSI-X 테이블 + PBA를 모두 포함 */
    memory_region_init(&n->bar0, OBJECT(n), "nvme-bar0", bar_size);

    /* MMIO 영역: BAR0의 앞부분 (0 ~ msix_table_offset) */
    memory_region_init_io(&n->iomem, OBJECT(n), &nvme_mmio_ops, n, "nvme",
                          msix_table_offset);

    /* MMIO를 BAR0 컨테이너의 서브리전으로 추가 */
    memory_region_add_subregion(&n->bar0, 0, &n->iomem);

    /* BAR0 등록 */
    pci_register_bar(pci_dev, 0,
                     PCI_BASE_ADDRESS_SPACE_MEMORY |
                     PCI_BASE_ADDRESS_MEM_TYPE_64,
                     &n->bar0);

    /* MSI-X 테이블과 PBA를 같은 BAR0 내에 배치 */
    ret = msix_init(pci_dev, nr_vectors,
                    &n->bar0, 0, msix_table_offset,   /* MSI-X 테이블 위치 */
                    &n->bar0, 0, msix_pba_offset,     /* PBA 위치 */
                    0, errp);
}
```

BAR0 내부 레이아웃 (모드 B):

```
BAR0 (bar_size 크기, 예: 128KB)
┌─────────────────────────────────────────────────────────────┐
│  offset 0x0000: NVMe 레지스터 (CAP, VS, CC, CSTS, AQA...) │ ← nvme_mmio_ops
│  offset 0x1000: SQ0 Tail Doorbell                          │ ← nvme_mmio_ops
│  offset 0x1004: CQ0 Head Doorbell                          │ ← nvme_mmio_ops
│  offset 0x1008: SQ1 Tail Doorbell                          │
│  ...                                                        │
│  offset 0xXXXX: MSI-X 테이블                               │ ← msix_table_mmio
│  offset 0xYYYY: MSI-X PBA                                  │ ← msix_pba_mmio
└─────────────────────────────────────────────────────────────┘
```

### 2.2 nvme_mmio_ops: MMIO 핸들러 등록

```c
/* ctrl.c:8535 */
static const MemoryRegionOps nvme_mmio_ops = {
    .read = nvme_mmio_read,     /* Guest가 BAR0 읽을 때 호출 */
    .write = nvme_mmio_write,   /* Guest가 BAR0 쓸 때 호출 */
    .endianness = DEVICE_LITTLE_ENDIAN,  /* NVMe는 Little Endian */
    .impl = {
        .min_access_size = 2,   /* 최소 2바이트 접근 허용 */
        .max_access_size = 8,   /* 최대 8바이트 접근 허용 (64-bit CAP 등) */
    },
};
```

`MemoryRegionOps` 구조체 정의 (`include/system/memory.h:300`):

```c
struct MemoryRegionOps {
    /* @addr는 MemoryRegion 시작점 기준 상대 오프셋, @size는 바이트 */
    uint64_t (*read)(void *opaque, hwaddr addr, unsigned size);
    void (*write)(void *opaque, hwaddr addr, uint64_t data, unsigned size);

    /* 속성 포함 버전 (선택) */
    MemTxResult (*read_with_attrs)(void *opaque, hwaddr addr, uint64_t *data,
                                   unsigned size, MemTxAttrs attrs);
    MemTxResult (*write_with_attrs)(void *opaque, hwaddr addr, uint64_t data,
                                    unsigned size, MemTxAttrs attrs);

    enum device_endian endianness;
    /* ... */
};
```

### 2.3 pci_register_bar() 내부 동작

`hw/pci/pci.c:1497`에서 BAR 등록이 수행된다:

```c
void pci_register_bar(PCIDevice *pci_dev, int region_num,
                      uint8_t type, MemoryRegion *memory)
{
    PCIIORegion *r;
    uint64_t wmask;
    pcibus_t size = memory_region_size(memory);

    /* BAR 정보를 io_regions 배열에 저장 */
    r = &pci_dev->io_regions[region_num];
    r->size = size;
    r->type = type;
    r->memory = memory;    /* ★ MemoryRegion 포인터 저장 */

    /* Memory 타입이면 PCI 버스의 address_space_mem에 연결 */
    r->address_space = type & PCI_BASE_ADDRESS_SPACE_IO
                        ? pci_get_bus(pci_dev)->address_space_io
                        : pci_get_bus(pci_dev)->address_space_mem;

    /* 초기 상태: 아직 매핑되지 않음 */
    r->addr = PCI_BAR_UNMAPPED;

    /* BAR 레지스터에 Write Mask 설정 (Guest가 주소를 쓸 수 있게) */
    wmask = ~(size - 1);
    addr = pci_bar(pci_dev, region_num);
    pci_set_long(pci_dev->config + addr, type);

    /* 64-bit BAR인 경우 두 개의 32-bit 레지스터 사용 */
    if (!(r->type & PCI_BASE_ADDRESS_SPACE_IO) &&
        r->type & PCI_BASE_ADDRESS_MEM_TYPE_64) {
        pci_set_quad(pci_dev->wmask + addr, wmask);
        pci_set_quad(pci_dev->cmask + addr, ~0ULL);
    }
}
```

**Guest OS가 BAR 주소를 프로그래밍하면** (`pci_update_mappings()`를 통해) `r->memory` MemoryRegion이 `r->address_space`의 해당 주소에 `memory_region_add_subregion()`으로 매핑된다. 이 시점부터 Guest가 그 주소에 접근하면 MMIO trap이 발생한다.

### 2.4 Guest의 writel()이 QEMU 핸들러에 도달하는 전체 경로

```
┌─ Guest VM ────────────────────────────────────────────────────────┐
│                                                                    │
│  Linux 커널 NVMe 드라이버:                                        │
│    writel(val, dev->bar + 0x1000)   /* SQ0 Tail Doorbell 쓰기 */  │
│      │                                                             │
│      ▼                                                             │
│  x86 CPU (Guest mode):                                             │
│    MOV [MMIO_ADDR], val             /* 메모리 기록 명령 실행 */    │
│      │                                                             │
│      ▼                                                             │
│  MMU: EPT (Extended Page Table) 조회                               │
│    ├── MMIO 주소는 RAM이 아니므로 EPT 엔트리가 없음               │
│    └── EPT Violation 발생!                                        │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
         │ VM-Exit (EXIT_REASON_EPT_VIOLATION)
         ▼
┌─ KVM (호스트 커널) ──────────────────────────────────────────────┐
│                                                                    │
│  handle_ept_violation()                                            │
│    ├── GPA (Guest Physical Address) 추출                          │
│    ├── kvm_mmu_page_fault()                                        │
│    │     └── MMIO 영역임을 확인 (memslot에 없는 주소)             │
│    └── MMIO 접근을 userspace(QEMU)로 전달                         │
│          ├── run->exit_reason = KVM_EXIT_MMIO                      │
│          ├── run->mmio.phys_addr = GPA                             │
│          ├── run->mmio.data = val                                  │
│          ├── run->mmio.len = 4                                     │
│          └── run->mmio.is_write = 1                                │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
         │ ioctl(KVM_RUN) 리턴
         ▼
┌─ QEMU (userspace) ───────────────────────────────────────────────┐
│                                                                    │
│  kvm_cpu_exec()                                                    │
│    └── kvm_handle_io() 또는 kvm_handle_mmio()                     │
│          │                                                         │
│          ▼                                                         │
│  address_space_rw(address_space, GPA, attrs, data, len, is_write)  │
│    │                                                               │
│    ▼                                                               │
│  flatview_write() / flatview_read()                                │
│    ├── flatview에서 GPA에 해당하는 MemoryRegionSection 검색       │
│    │   (FlatView는 모든 MemoryRegion을 평면화한 lookup 테이블)    │
│    ├── section->mr = &n->iomem  (NVMe의 MMIO MemoryRegion 발견)  │
│    │                                                               │
│    ▼                                                               │
│  memory_region_dispatch_write(mr, offset, data, size, attrs)       │
│    │                                                               │
│    ▼                                                               │
│  mr->ops->write(mr->opaque, offset, data, size)                   │
│    │                                                               │
│    ▼                                                               │
│  nvme_mmio_write(n, addr, data, size)  ← ★★★ NVMe 핸들러 도착!  │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

#### EPT (Extended Page Table)에서 MMIO가 trap되는 원리

```
Guest Virtual Address
        │
        ▼
   Guest Page Table  (Guest CR3 기반)
        │ Guest Physical Address (GPA) 변환
        ▼
   EPT (Extended Page Table)  (VMCS의 EPTP 기반)
        │ Host Physical Address (HPA) 변환
        ▼
   ┌──────────────────────────────────────┐
   │ RAM 영역:  EPT 엔트리 존재          │ → 정상 메모리 접근
   │ MMIO 영역: EPT 엔트리 없음          │ → EPT Violation → VM-Exit
   └──────────────────────────────────────┘

KVM은 QEMU의 memory_region_add_subregion()으로 등록된 RAM 영역만
KVM_SET_USER_MEMORY_REGION ioctl로 EPT에 매핑한다.
MMIO MemoryRegion (memory_region_init_io로 생성)은 의도적으로
EPT에 매핑하지 않아서 접근 시 반드시 trap이 발생한다.
```

#### MemoryRegion FlatView Dispatch 원리

```
QEMU 메모리 시스템 계층 구조:

AddressSpace "pci-bus-memory"
  └── MemoryRegion "system" (container)
        ├── MemoryRegion "ram" (0x0 ~ 0x7FFFFFFF, RAM)
        ├── MemoryRegion "pci-hole" (container)
        │     ├── MemoryRegion "nvme-bar0" (0xFE000000, container)
        │     │     ├── MemoryRegion "nvme" (0x0, nvme_mmio_ops)
        │     │     └── MemoryRegion "msix-table" (0xXXXX)
        │     └── MemoryRegion "nvme-cmb" (0xFE100000, nvme_cmb_ops)
        └── ...

이 트리 구조를 "Flatten"하면:

FlatView:
  [0x00000000 ~ 0x7FFFFFFF] → ram MemoryRegion
  [0xFE000000 ~ 0xFE00XXXX] → nvme (nvme_mmio_ops) ← BAR0 MMIO
  [0xFE00XXXX ~ 0xFE00YYYY] → msix-table
  [0xFE100000 ~ 0xFE1ZZZZZ] → nvme-cmb (nvme_cmb_ops)

GPA가 들어오면 FlatView에서 이진 탐색으로 해당 MemoryRegion을 O(log n)에 찾는다.
```

### 2.5 nvme_mmio_read() 상세 분석

Guest가 NVMe BAR0의 레지스터를 읽을 때 호출되는 함수이다:

```c
/* ctrl.c:8323 */
static uint64_t nvme_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    NvmeCtrl *n = (NvmeCtrl *)opaque;
    /* bar 구조체를 바이트 배열로 캐스팅 - 오프셋으로 직접 접근하기 위해 */
    uint8_t *ptr = (uint8_t *)&n->bar;

    trace_pci_nvme_mmio_read(addr, size);

    /* 정렬 검사: NVMe 스펙상 32-bit 정렬 필수 */
    if (unlikely(addr & (sizeof(uint32_t) - 1))) {
        NVME_GUEST_ERR(pci_nvme_ub_mmiord_misaligned32,
                       "MMIO read not 32-bit aligned,"
                       " offset=0x%"PRIx64"", addr);
        /* 스펙상 무시해야 하지만, 현재는 허용 */
    } else if (unlikely(size < sizeof(uint32_t))) {
        NVME_GUEST_ERR(pci_nvme_ub_mmiord_toosmall,
                       "MMIO read smaller than 32-bits,"
                       " offset=0x%"PRIx64"", addr);
    }

    /* 범위 검사: bar 구조체를 넘어서는 읽기 방지 */
    if (addr > sizeof(n->bar) - size) {
        NVME_GUEST_ERR(pci_nvme_ub_mmiord_invalid_ofs,
                       "MMIO read beyond last register,"
                       " offset=0x%"PRIx64", returning 0", addr);
        return 0;
    }

    /* SR-IOV VF 오프라인 체크 */
    if (pci_is_vf(PCI_DEVICE(n)) && !nvme_sctrl(n)->scs &&
        addr != NVME_REG_CSTS) {
        return 0;
    }

    /* PMR 관련: 영속 매체 동기화 */
    if (addr == NVME_REG_PMRSTS &&
        (NVME_PMRCAP_PMRWBM(ldl_le_p(&n->bar.pmrcap)) & 0x02)) {
        memory_region_msync(&n->pmr.dev->mr, 0, n->pmr.dev->size);
    }

    /* ★ 핵심: bar 구조체에서 오프셋 위치의 값을 Little Endian으로 읽어 반환 */
    return ldn_le_p(ptr + addr, size);
}
```

**n->bar 구조체가 곧 레지스터 파일이다.** QEMU는 NVMe 레지스터를 별도 하드웨어 로직이 아니라 `NvmeBar` 구조체에 소프트웨어적으로 저장한다. Guest가 레지스터를 읽으면 단순히 이 구조체의 해당 오프셋 값을 반환한다.

```
NvmeBar 구조체 (include/block/nvme.h:6):
┌──────────┬────────┬─────────────────────────────────┐
│ Offset   │ 크기   │ 필드                            │
├──────────┼────────┼─────────────────────────────────┤
│ 0x00     │ 8      │ cap   (Controller Capabilities) │
│ 0x08     │ 4      │ vs    (Version)                 │
│ 0x0C     │ 4      │ intms (Interrupt Mask Set)      │
│ 0x10     │ 4      │ intmc (Interrupt Mask Clear)    │
│ 0x14     │ 4      │ cc    (Controller Configuration)│
│ 0x18     │ 4      │ rsvd                            │
│ 0x1C     │ 4      │ csts  (Controller Status)       │
│ 0x20     │ 4      │ nssr  (NVM Subsystem Reset)     │
│ 0x24     │ 4      │ aqa   (Admin Queue Attributes)  │
│ 0x28     │ 8      │ asq   (Admin SQ Base Address)   │
│ 0x30     │ 8      │ acq   (Admin CQ Base Address)   │
│ 0x38     │ 4      │ cmbloc(CMB Location)            │
│ 0x3C     │ 4      │ cmbsz (CMB Size)                │
│ ...      │ ...    │ ...                             │
│ 0x1000   │ 4      │ SQ0 Tail Doorbell (가변)        │
│ 0x1004   │ 4      │ CQ0 Head Doorbell (가변)        │
│ ...      │        │ (Doorbell은 bar 구조체 밖)      │
└──────────┴────────┴─────────────────────────────────┘
```

### 2.6 nvme_mmio_write() 상세 분석

Guest가 BAR0에 쓸 때 호출되는 최상위 분배 함수이다:

```c
/* ctrl.c:8515 */
static void nvme_mmio_write(void *opaque, hwaddr addr, uint64_t data,
                            unsigned size)
{
    NvmeCtrl *n = (NvmeCtrl *)opaque;

    trace_pci_nvme_mmio_write(addr, data, size);

    /* VF 오프라인 체크 */
    if (pci_is_vf(PCI_DEVICE(n)) && !nvme_sctrl(n)->scs &&
        addr != NVME_REG_CSTS) {
        return;
    }

    /* ★ 핵심 분기: 주소가 bar 구조체 범위 내면 레지스터, 밖이면 Doorbell */
    if (addr < sizeof(n->bar)) {
        nvme_write_bar(n, addr, data, size);   /* 레지스터 쓰기 */
    } else {
        nvme_process_db(n, addr, data);        /* Doorbell 쓰기 */
    }
}
```

**NVMe BAR0의 0x1000 이후는 모두 Doorbell 레지스터이다.** `sizeof(NvmeBar)`는 4096(0x1000)이므로, 이 경계를 기준으로 레지스터 쓰기와 Doorbell 쓰기를 분리한다.

### 2.7 nvme_write_bar() 상세 분석: 레지스터별 처리

```c
/* ctrl.c:8085 */
static void nvme_write_bar(NvmeCtrl *n, hwaddr offset, uint64_t data,
                           unsigned size)
{
    PCIDevice *pci = PCI_DEVICE(n);
    uint64_t cap = ldq_le_p(&n->bar.cap);
    uint32_t cc = ldl_le_p(&n->bar.cc);      /* 현재 CC 값 (쓰기 전) */
    uint32_t csts = ldl_le_p(&n->bar.csts);   /* 현재 CSTS 값 */

    /* 정렬/크기 검사 (생략) ... */

    switch (offset) {
    /*─── INTMS (0x0C): Interrupt Mask Set ───*/
    case NVME_REG_INTMS:
        if (unlikely(msix_enabled(pci))) {
            /* MSI-X 사용 시 INTMS/INTMC는 무의미 */
        }
        intms |= data;                    /* Set 비트 OR */
        stl_le_p(&n->bar.intms, intms);
        n->bar.intmc = n->bar.intms;
        nvme_irq_check(n);                /* 인터럽트 상태 재확인 */
        break;

    /*─── INTMC (0x10): Interrupt Mask Clear ───*/
    case NVME_REG_INTMC:
        intms &= ~data;                   /* Clear 비트 AND NOT */
        stl_le_p(&n->bar.intms, intms);
        n->bar.intmc = n->bar.intms;
        nvme_irq_check(n);
        break;

    /*─── CC (0x14): Controller Configuration ─── ★ 가장 중요 ★ ───*/
    case NVME_REG_CC:
        stl_le_p(&n->bar.cc, data);       /* 먼저 CC 값 저장 */

        /* Shutdown 처리: SHN 비트가 0→비0 전환 */
        if (NVME_CC_SHN(data) && !(NVME_CC_SHN(cc))) {
            nvme_ctrl_shutdown(n);         /* 컨트롤러 셧다운 */
            csts &= ~(CSTS_SHST_MASK << CSTS_SHST_SHIFT);
            csts |= NVME_CSTS_SHST_COMPLETE;  /* SHST=완료 */
        } else if (!NVME_CC_SHN(data) && NVME_CC_SHN(cc)) {
            csts &= ~(CSTS_SHST_MASK << CSTS_SHST_SHIFT);
        }

        /* Enable 처리: EN 비트가 0→1 전환 */
        if (NVME_CC_EN(data) && !NVME_CC_EN(cc)) {
            if (unlikely(nvme_start_ctrl(n))) {
                csts = NVME_CSTS_FAILED;   /* 시작 실패 */
            } else {
                csts = NVME_CSTS_READY;    /* CSTS.RDY = 1 */
            }
        /* Disable 처리: EN 비트가 1→0 전환 */
        } else if (!NVME_CC_EN(data) && NVME_CC_EN(cc)) {
            nvme_ctrl_reset(n, NVME_RESET_CONTROLLER);
            break;
        }

        stl_le_p(&n->bar.csts, csts);
        break;

    /*─── CSTS (0x1C): Controller Status ─── 읽기 전용 ───*/
    case NVME_REG_CSTS:
        /* Guest가 CSTS에 쓰면 에러 (읽기 전용 레지스터) */
        break;

    /*─── AQA (0x24): Admin Queue Attributes ───*/
    case NVME_REG_AQA:
        stl_le_p(&n->bar.aqa, data);      /* Admin SQ/CQ 크기 저장 */
        break;

    /*─── ASQ (0x28): Admin SQ Base Address ───*/
    case NVME_REG_ASQ:
        stn_le_p(&n->bar.asq, size, data);
        break;
    case NVME_REG_ASQ + 4:                 /* 64-bit 주소의 상위 32-bit */
        stl_le_p((uint8_t *)&n->bar.asq + 4, data);
        break;

    /*─── ACQ (0x30): Admin CQ Base Address ───*/
    case NVME_REG_ACQ:
        stn_le_p(&n->bar.acq, size, data);
        break;
    case NVME_REG_ACQ + 4:
        stl_le_p((uint8_t *)&n->bar.acq + 4, data);
        break;

    /*─── CMBMSC (0x50): CMB Memory Space Control ───*/
    case NVME_REG_CMBMSC:
        if (!NVME_CAP_CMBS(cap)) return;
        stn_le_p(&n->bar.cmbmsc, size, data);
        n->cmb.cmse = false;
        if (NVME_CMBMSC_CRE(data)) {
            nvme_cmb_enable_regs(n);
            if (NVME_CMBMSC_CMSE(data)) {
                uint64_t cmbmsc = ldq_le_p(&n->bar.cmbmsc);
                hwaddr cba = NVME_CMBMSC_CBA(cmbmsc) << CMBMSC_CBA_SHIFT;
                n->cmb.cba = cba;
                n->cmb.cmse = true;
            }
        }
        return;

    /* ... PMRCTL, PMRMSCL 등 생략 ... */

    default:
        NVME_GUEST_ERR(pci_nvme_ub_mmiowr_invalid,
                       "invalid MMIO write,"
                       " offset=0x%"PRIx64", data=%"PRIx64"",
                       offset, data);
        break;
    }
}
```

### 2.8 nvme_process_db() 상세 분석: Doorbell 처리

Guest가 BAR0의 0x1000 이후 주소에 쓰면 Doorbell 처리가 된다. 이것이 NVMe I/O의 시작점이다.

```c
/* ctrl.c:8369 */
static void nvme_process_db(NvmeCtrl *n, hwaddr addr, int val)
{
    PCIDevice *pci = PCI_DEVICE(n);
    uint32_t qid;

    /* 정렬 검사 */
    if (unlikely(addr & ((1 << 2) - 1))) {
        return;  /* 4-byte 정렬 필수 */
    }

    /*
     * Doorbell 주소 레이아웃 (NVMe 스펙 Figure 46):
     *
     * 0x1000 + (2y    ) * (4 << CAP.DSTRD) = SQy Tail Doorbell
     * 0x1000 + (2y + 1) * (4 << CAP.DSTRD) = CQy Head Doorbell
     *
     * QEMU에서 DSTRD=0이므로:
     * 0x1000 + 8*y     = SQy Tail Doorbell
     * 0x1000 + 8*y + 4 = CQy Head Doorbell
     */

    if (((addr - 0x1000) >> 2) & 1) {
        /*═══════════════════════════════════════════════*/
        /* Completion Queue Head Doorbell 쓰기           */
        /*═══════════════════════════════════════════════*/
        uint16_t new_head = val & 0xffff;
        NvmeCQueue *cq;

        /* QID 계산: (addr - 0x1004) / 8 */
        qid = (addr - (0x1000 + (1 << 2))) >> 3;

        /* 유효성 검사 */
        if (unlikely(nvme_check_cqid(n, qid))) {
            /* 존재하지 않는 큐 → 에러 이벤트 */
            if (n->outstanding_aers) {
                nvme_enqueue_event(n, NVME_AER_TYPE_ERROR,
                                   NVME_AER_INFO_ERR_INVALID_DB_REGISTER,
                                   NVME_LOG_ERROR_INFO);
            }
            return;
        }

        cq = n->cq[qid];
        if (unlikely(new_head >= cq->size)) {
            /* 범위 초과 → 에러 */
            return;
        }

        /* CQ가 이전에 꽉 찼었으면, head가 전진하므로 대기 중인 CQE 전송 스케줄 */
        if (nvme_cq_full(cq)) {
            qemu_bh_schedule(cq->bh);   /* → nvme_post_cqes() 호출 예약 */
        }

        cq->head = new_head;

        /* CQ가 비었으면 인터럽트 해제 */
        if (cq->tail == cq->head) {
            if (cq->irq_enabled) {
                n->cq_pending--;
            }
            nvme_irq_deassert(n, cq);
        }

    } else {
        /*═══════════════════════════════════════════════*/
        /* Submission Queue Tail Doorbell 쓰기           */
        /*═══════════════════════════════════════════════*/
        uint16_t new_tail = val & 0xffff;
        NvmeSQueue *sq;

        /* QID 계산: (addr - 0x1000) / 8 */
        qid = (addr - 0x1000) >> 3;

        if (unlikely(nvme_check_sqid(n, qid))) {
            return;
        }

        sq = n->sq[qid];
        if (unlikely(new_tail >= sq->size)) {
            return;
        }

        sq->tail = new_tail;

        /* ★★★ SQ 처리 Bottom Half 스케줄 → nvme_process_sq() 호출 */
        qemu_bh_schedule(sq->bh);
    }
}
```

**핵심:** SQ Tail Doorbell 쓰기가 들어오면 `qemu_bh_schedule(sq->bh)`로 Bottom Half를 스케줄하고, 이것이 `nvme_process_sq()`를 호출하여 실제 SQ에서 명령을 가져와 처리한다.

---

## 3. DMA 에뮬레이션 (디바이스가 Guest 메모리 접근)

### 3.1 실제 하드웨어 vs QEMU 에뮬레이션 비교

```
┌─ 실제 하드웨어 ─────────────────────────────────────────────┐
│                                                               │
│  NVMe SSD 컨트롤러:                                          │
│    "SQ에서 명령 읽어와야 해"                                  │
│    → PCIe Bus Master DMA: SQ의 물리 주소로 Read TLP 전송     │
│    → Root Complex가 호스트 RAM에서 데이터 가져다줌            │
│    → 컨트롤러 내부 SRAM에 64바이트 SQE 수신                  │
│                                                               │
│  [SSD Controller] ──DMA Read──→ [PCIe Bus] ──→ [Host RAM]    │
│                                                               │
└───────────────────────────────────────────────────────────────┘

┌─ QEMU 에뮬레이션 ───────────────────────────────────────────┐
│                                                               │
│  nvme_process_sq():                                           │
│    "SQ에서 명령 읽어와야 해"                                  │
│    → pci_dma_read(PCI_DEVICE(n), sq->dma_addr + offset,      │
│                   &cmd, sizeof(cmd))                          │
│    → dma_memory_rw(pci_get_address_space(dev), ...)           │
│    → address_space_rw(as, guest_phys_addr, ...)               │
│    → Guest 물리 주소 → QEMU 프로세스의 가상 주소로 변환      │
│    → memcpy()로 데이터 복사                                   │
│                                                               │
│  결과: QEMU 프로세스 메모리 내에서 포인터 연산으로 해결       │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

### 3.2 DMA 함수 호출 체인

```c
/* include/hw/pci/pci_device.h:276 */
static inline MemTxResult pci_dma_read(PCIDevice *dev, dma_addr_t addr,
                                       void *buf, dma_addr_t len)
{
    return pci_dma_rw(dev, addr, buf, len,
                      DMA_DIRECTION_TO_DEVICE, MEMTXATTRS_UNSPECIFIED);
}

/* include/hw/pci/pci_device.h:256 */
static inline MemTxResult pci_dma_rw(PCIDevice *dev, dma_addr_t addr,
                                     void *buf, dma_addr_t len,
                                     DMADirection dir, MemTxAttrs attrs)
{
    /* pci_get_address_space()로 디바이스의 AddressSpace 획득 */
    return dma_memory_rw(pci_get_address_space(dev), addr, buf, len,
                         dir, attrs);
}

/* 최종: address_space_rw() →
 *   flatview_lookup() → Guest 물리 주소에 대응하는 RAMBlock 찾기 →
 *   qemu_map_ram_ptr() → Host 가상 주소 획득 →
 *   memcpy() → 데이터 복사
 */
```

```
pci_dma_read(dev, gpa, buf, len)
  └── pci_dma_rw(dev, gpa, buf, len, TO_DEVICE, ...)
        └── dma_memory_rw(address_space, gpa, buf, len, ...)
              └── address_space_rw(as, gpa, attrs, buf, len, is_write)
                    └── flatview_rw(fv, gpa, ...)
                          └── flatview_translate(fv, gpa, &xlat, &len, ...)
                                ├── MemoryRegionSection 검색
                                └── xlat = gpa - section.offset_in_as +
                                           section.offset_in_region
                          └── address_space_read_cached / memcpy()
```

### 3.3 SQ Fetch: nvme_process_sq() 상세

NVMe 컨트롤러가 SQ에서 명령을 가져오는 과정이다:

```c
/* ctrl.c:7777 */
static void nvme_process_sq(void *opaque)
{
    NvmeSQueue *sq = opaque;
    NvmeCtrl *n = sq->ctrl;
    NvmeCQueue *cq = n->cq[sq->cqid];

    uint16_t status;
    hwaddr addr;
    NvmeCmd cmd;
    NvmeRequest *req;

    /* Shadow Doorbell 사용 시 tail 업데이트 */
    if (n->dbbuf_enabled) {
        nvme_update_sq_tail(sq);
    }

    /* SQ에 처리할 명령이 있고, 빈 Request 슬롯이 있는 동안 반복 */
    while (!(nvme_sq_empty(sq) || QTAILQ_EMPTY(&sq->req_list))) {

        /* ★ SQE 물리 주소 계산: SQ 시작 + head * 64바이트 */
        addr = sq->dma_addr + (sq->head << NVME_SQES);
        /*                      ^^^^^^^^
         * NVME_SQES = 6, 즉 1 << 6 = 64바이트 = SQE 크기
         * sq->head << 6 = sq->head * 64                       */

        /* ★ Guest 메모리에서 SQE 64바이트를 DMA로 읽기 */
        if (nvme_addr_read(n, addr, (void *)&cmd, sizeof(cmd))) {
            /* 읽기 실패 → CFS (Controller Fatal Status) */
            stl_le_p(&n->bar.csts, NVME_CSTS_FAILED);
            break;
        }

        /* SQ head 증가 (원형 큐) */
        nvme_inc_sq_head(sq);

        /* 빈 Request 슬롯 획득 */
        req = QTAILQ_FIRST(&sq->req_list);
        QTAILQ_REMOVE(&sq->req_list, req, entry);
        QTAILQ_INSERT_TAIL(&sq->out_req_list, req, entry);

        /* 요청 초기화 및 명령 복사 */
        nvme_req_clear(req);
        req->cqe.cid = cmd.cid;      /* Command ID 저장 (CQE에서 사용) */
        memcpy(&req->cmd, &cmd, sizeof(NvmeCmd));

        /* ★ 명령 실행: Admin 큐면 admin_cmd, I/O 큐면 io_cmd */
        status = sq->sqid ? nvme_io_cmd(n, req) :
                            nvme_admin_cmd(n, req);

        /* 즉시 완료되는 명령이면 바로 CQ에 등록 */
        if (status != NVME_NO_COMPLETE) {
            req->status = status;
            nvme_enqueue_req_completion(cq, req);
        }
    }
}
```

`nvme_addr_read()`는 CMB/PMR 주소이면 직접 memcpy하고, 일반 주소이면 `pci_dma_read()`를 호출한다:

```c
/* ctrl.c:573 */
static int nvme_addr_read(NvmeCtrl *n, hwaddr addr, void *buf, int size)
{
    hwaddr hi = addr + size - 1;
    if (hi < addr) return 1;  /* 오버플로우 검사 */

    /* CMB 영역이면 직접 메모리 복사 (DMA 불필요) */
    if (n->bar.cmbsz && nvme_addr_is_cmb(n, addr) && nvme_addr_is_cmb(n, hi)) {
        memcpy(buf, nvme_addr_to_cmb(n, addr), size);
        return 0;
    }

    /* PMR 영역이면 직접 메모리 복사 */
    if (nvme_addr_is_pmr(n, addr) && nvme_addr_is_pmr(n, hi)) {
        memcpy(buf, nvme_addr_to_pmr(n, addr), size);
        return 0;
    }

    /* ★ 일반 Guest RAM → PCIe DMA 읽기 */
    return pci_dma_read(PCI_DEVICE(n), addr, buf, size);
}
```

### 3.4 CQ Post: nvme_post_cqes() 상세

명령 처리가 완료되면 CQE를 Guest 메모리의 CQ에 기록한다:

```c
/* ctrl.c:1498 */
static void nvme_post_cqes(void *opaque)
{
    NvmeCQueue *cq = opaque;
    NvmeCtrl *n = cq->ctrl;
    NvmeRequest *req, *next;
    bool pending = cq->head != cq->tail;

    /* CQ의 대기 목록에서 완료된 요청을 순서대로 처리 */
    QTAILQ_FOREACH_SAFE(req, &cq->req_list, entry, next) {
        NvmeSQueue *sq;
        hwaddr addr;

        /* CQ가 꽉 찼으면 중단 (Guest가 CQ Head Doorbell을 쓸 때까지 대기) */
        if (nvme_cq_full(cq)) {
            break;
        }

        sq = req->sq;

        /* CQE 구성 */
        req->cqe.status = cpu_to_le16((req->status << 1) | cq->phase);
        req->cqe.sq_id = cpu_to_le16(sq->sqid);
        req->cqe.sq_head = cpu_to_le16(sq->head);

        /* ★ CQE 물리 주소 계산: CQ 시작 + tail * 16바이트 */
        addr = cq->dma_addr + (cq->tail << NVME_CQES);
        /*                      ^^^^^^^^
         * NVME_CQES = 4, 즉 1 << 4 = 16바이트 = CQE 크기 */

        /* ★ Guest 메모리에 CQE 16바이트를 DMA로 기록 */
        ret = pci_dma_write(PCI_DEVICE(n), addr,
                            (void *)&req->cqe, sizeof(req->cqe));
        if (ret) {
            stl_le_p(&n->bar.csts, NVME_CSTS_FAILED);
            break;
        }

        /* CQ에서 제거 */
        QTAILQ_REMOVE(&cq->req_list, req, entry);

        /* CQ tail 증가 (원형 큐, phase bit 반전 포함) */
        nvme_inc_cq_tail(cq);

        /* SG 매핑 해제 */
        nvme_sg_unmap(&req->sg);

        /* Request를 SQ의 free 목록으로 반환 */
        QTAILQ_INSERT_TAIL(&sq->req_list, req, entry);
    }

    /* CQ에 새 엔트리가 추가되었으면 인터럽트 발생 */
    if (cq->tail != cq->head) {
        if (cq->irq_enabled && !pending) {
            n->cq_pending++;
        }
        nvme_irq_assert(n, cq);   /* ★ MSI-X 인터럽트 발생! */
    }
}
```

### 3.5 DMA 흐름 ASCII 다이어그램

```
                    QEMU Process Memory
                    ┌─────────────────────────────────────┐
                    │                                     │
  Guest RAM         │   NvmeCtrl *n                       │
  (mmap된 영역)     │   ┌─────────┐                       │
  ┌──────────┐      │   │ NvmeCmd │ ← nvme_addr_read()   │
  │ SQ Entry │──DMA─┼──→│  cmd    │   copies SQE here    │
  │ (64B)    │ Read │   └─────────┘                       │
  └──────────┘      │        │                            │
                    │        │ nvme_io_cmd()               │
                    │        ▼                             │
                    │   [명령 처리]                        │
                    │        │                             │
                    │        │ nvme_map_prp() / nvme_map_sgl()
                    │        ▼                             │
  ┌──────────┐      │   ┌─────────┐                       │
  │ Data     │←─DMA─┼───│ 데이터  │ pci_dma_read/write   │
  │ Buffer   │ R/W  │   │ 처리    │                       │
  └──────────┘      │   └─────────┘                       │
                    │        │                             │
                    │        │ nvme_enqueue_req_completion()│
                    │        ▼                             │
  ┌──────────┐      │   ┌─────────┐                       │
  │ CQ Entry │←─DMA─┼───│ NvmeCqe │ pci_dma_write()      │
  │ (16B)    │Write │   │  cqe    │   writes CQE here    │
  └──────────┘      │   └─────────┘                       │
                    │        │                             │
                    │        │ msix_notify()               │
                    │        ▼                             │
                    │   [MSI-X 인터럽트 → Guest]          │
                    │                                     │
                    └─────────────────────────────────────┘
```

### 3.6 PRP (Physical Region Page) 매핑: nvme_map_prp()

NVMe 명령의 데이터 전송 주소를 지정하는 PRP 디코딩 과정이다:

```c
/* ctrl.c:888 */
static uint16_t nvme_map_prp(NvmeCtrl *n, NvmeSg *sg, uint64_t prp1,
                             uint64_t prp2, uint32_t len)
{
    /* PRP1의 페이지 내 오프셋을 고려한 첫 번째 전송 길이 */
    hwaddr trans_len = n->page_size - (prp1 % n->page_size);
    trans_len = MIN(len, trans_len);
    int num_prps = (len >> n->page_bits) + 1;

    /* scatter-gather 리스트 초기화 */
    nvme_sg_init(n, sg, nvme_addr_is_dma(n, prp1));

    /* ★ PRP1: 첫 번째 페이지 매핑 */
    status = nvme_map_addr(n, sg, prp1, trans_len);
    len -= trans_len;

    if (len) {
        if (len > n->page_size) {
            /* ★ PRP2가 PRP List를 가리키는 경우 */
            /* PRP List에서 페이지 주소들을 차례로 읽어 매핑 */
            g_autofree uint64_t *prp_list = g_new(uint64_t, n->max_prp_ents);

            nents = (n->page_size - (prp2 & (n->page_size - 1))) >> 3;
            prp_trans = MIN(n->max_prp_ents, nents) * sizeof(uint64_t);

            /* Guest 메모리에서 PRP List 읽기 */
            ret = nvme_addr_read(n, prp2, (void *)prp_list, prp_trans);

            while (len != 0) {
                uint64_t prp_ent = le64_to_cpu(prp_list[i]);

                /* PRP List의 마지막 엔트리가 다음 PRP List를 가리킬 수 있음 */
                if (i == nents - 1 && len > n->page_size) {
                    /* 체인된 PRP List 읽기 */
                    ret = nvme_addr_read(n, prp_ent, (void *)prp_list,
                                         prp_trans);
                    prp_ent = le64_to_cpu(prp_list[i]);
                }

                /* 각 PRP 엔트리를 scatter-gather에 추가 */
                trans_len = MIN(len, n->page_size);
                status = nvme_map_addr(n, sg, prp_ent, trans_len);
                len -= trans_len;
                i++;
            }
        } else {
            /* ★ PRP2가 직접 두 번째 페이지를 가리키는 경우 */
            status = nvme_map_addr(n, sg, prp2, len);
        }
    }
    return NVME_SUCCESS;
}
```

PRP 구조 다이어그램:

```
Case 1: 데이터가 2 페이지 이하
┌───────────────────────────────────┐
│ NVMe Command (SQE)               │
│  PRP1 = 0x1000_2800 ─────────┐   │ (페이지 내 오프셋 0x800)
│  PRP2 = 0x1000_3000 ───────┐ │   │
└────────────────────────────┼─┼───┘
                             │ │
    Guest Physical Memory    │ │
    ┌────────────────────┐   │ │
    │ Page @ 0x1000_2000 │   │ │
    │  ...               │   │ │
    │  offset 0x800 ◄────┼───┘ │ 전송 시작점
    │  ...데이터...       │     │ trans_len = 0x800 (페이지 끝까지)
    ├────────────────────┤     │
    │ Page @ 0x1000_3000 │◄────┘ PRP2 = 두 번째 페이지
    │  ...데이터...       │      나머지 len 만큼
    └────────────────────┘

Case 2: 데이터가 2 페이지 초과 → PRP List
┌───────────────────────────────────┐
│ NVMe Command (SQE)               │
│  PRP1 = 0x1000_2800 ─────────────┼──→ 첫 페이지 (오프셋 포함)
│  PRP2 = 0x2000_0000 ─────────┐   │
└──────────────────────────────┼───┘
                               │
    PRP List @ 0x2000_0000     │
    ┌──────────────────────┐◄──┘
    │ Entry[0] = 0x3000_0000 ──→ Data Page 2
    │ Entry[1] = 0x4000_0000 ──→ Data Page 3
    │ Entry[2] = 0x5000_0000 ──→ Data Page 4
    │ ...                      │
    │ Entry[N-1] = 0x6000_0000 ──→ 다음 PRP List (체인)
    └──────────────────────────┘          또는 마지막 Data Page
```

### 3.7 SGL (Scatter Gather List) 매핑: nvme_map_sgl()

SGL은 PRP보다 유연한 데이터 주소 지정 방식이다:

```c
/* ctrl.c:1050 */
static uint16_t nvme_map_sgl(NvmeCtrl *n, NvmeSg *sg, NvmeSglDescriptor sgl,
                             size_t len, NvmeCmd *cmd)
{
    /* 256개씩 청크로 읽어 메모리 낭비 방지 */
    #define SEG_CHUNK_SIZE 256
    NvmeSglDescriptor segment[SEG_CHUNK_SIZE];

    sgld = &sgl;
    addr = le64_to_cpu(sgl.addr);
    nvme_sg_init(n, sg, nvme_addr_is_dma(n, addr));

    /* 단일 Data Block이면 바로 매핑 */
    if (NVME_SGL_TYPE(sgl.type) == NVME_SGL_DESCR_TYPE_DATA_BLOCK) {
        status = nvme_map_sgl_data(n, sg, sgld, 1, &len, cmd);
        goto out;
    }

    /* Segment/Last Segment 체인을 따라가며 Data Block들을 매핑 */
    for (;;) {
        seg_len = le32_to_cpu(sgld->len);
        nsgld = seg_len / sizeof(NvmeSglDescriptor);

        /* 256개씩 청크로 읽어 처리 */
        while (nsgld > SEG_CHUNK_SIZE) {
            nvme_addr_read(n, addr, segment, sizeof(segment));
            nvme_map_sgl_data(n, sg, segment, SEG_CHUNK_SIZE, &len, cmd);
            nsgld -= SEG_CHUNK_SIZE;
            addr += SEG_CHUNK_SIZE * sizeof(NvmeSglDescriptor);
        }
        /* ... 나머지 디스크립터 처리 ... */
    }
}
```

---

## 4. MSI-X 인터럽트 에뮬레이션

### 4.1 MSI-X 초기화

`nvme_init_pci()`에서 MSI-X를 설정한다:

```c
/* ctrl.c:8962 - PCIe capability 등록 */
pcie_endpoint_cap_init(pci_dev, 0x80);    /* PCIe Endpoint Capability */

/* ctrl.c:8974 또는 9001 - MSI-X 초기화 */
/* 모드 A: 전용 BAR */
msix_init_exclusive_bar(pci_dev, n->params.msix_qsize, 4, errp);
/*                                ^^^^^^^^^^^^^^^^^^    ^
 *                                벡터 수 (보통 65)    BAR 번호 */

/* 모드 B: BAR0 내 공존 */
msix_init(pci_dev, nr_vectors,
          &n->bar0, 0, msix_table_offset,  /* MSI-X 테이블: BAR0 내 */
          &n->bar0, 0, msix_pba_offset,    /* PBA: BAR0 내 */
          0, errp);
```

MSI-X 테이블 구조:

```
MSI-X Table (각 벡터당 16바이트):
┌──────────────────────────────────────────┐
│ Vector 0:                                │
│   [0x00] Message Address Low  (32-bit)   │ ← LAPIC 주소
│   [0x04] Message Address High (32-bit)   │
│   [0x08] Message Data         (32-bit)   │ ← 인터럽트 벡터 번호
│   [0x0C] Vector Control       (32-bit)   │ ← Mask bit
├──────────────────────────────────────────┤
│ Vector 1:                                │
│   ...                                    │
├──────────────────────────────────────────┤
│ ...                                      │
└──────────────────────────────────────────┘

PBA (Pending Bit Array):
  각 벡터당 1비트, 마스크 상태에서 pending된 인터럽트 표시
```

### 4.2 인터럽트 발생 과정: nvme_irq_assert()

```c
/* ctrl.c:674 */
static void nvme_irq_assert(NvmeCtrl *n, NvmeCQueue *cq)
{
    PCIDevice *pci = PCI_DEVICE(n);

    if (cq->irq_enabled) {
        if (msix_enabled(pci)) {
            /* ★ MSI-X 모드: 해당 CQ의 벡터로 인터럽트 발생 */
            trace_pci_nvme_irq_msix(cq->vector);
            msix_notify(pci, cq->vector);
        } else {
            /* INTx 모드: Legacy PCI 인터럽트 */
            assert(cq->vector < 32);
            n->irq_status |= 1 << cq->vector;
            nvme_irq_check(n);
        }
    }
}
```

### 4.3 msix_notify() 내부 동작

```c
/* hw/pci/msix.c:540 */
void msix_notify(PCIDevice *dev, unsigned vector)
{
    MSIMessage msg;

    assert(vector < dev->msix_entries_nr);

    /* 사용되지 않는 벡터면 무시 */
    if (!dev->msix_entry_used[vector]) {
        return;
    }

    /* 마스크된 벡터면 Pending으로 기록 */
    if (msix_is_masked(dev, vector)) {
        msix_set_pending(dev, vector);
        return;
    }

    /* MSI-X 테이블에서 Message Address/Data 읽기 */
    msg = msix_get_message(dev, vector);

    /* ★ MSI 메시지 전송 → KVM으로 전달 */
    msi_send_message(dev, msg);
}
```

### 4.4 MSI-X 인터럽트 전달 전체 경로

```
nvme_post_cqes()
  │ CQE를 Guest 메모리에 기록 완료
  │
  ▼
nvme_irq_assert(n, cq)
  │
  ▼
msix_notify(pci_dev, cq->vector)
  │ MSI-X 테이블에서 msg 구성:
  │   msg.address = 0xFEE0_0000 + (APIC_ID << 12)  ← LAPIC 주소
  │   msg.data    = vector_number                    ← 인터럽트 벡터
  │
  ▼
msi_send_message(dev, msg)
  │
  ▼
kvm_irqchip_send_msi(kvm_state, msg)
  │ KVM ioctl: KVM_SIGNAL_MSI
  │
  ▼
┌─ KVM (호스트 커널) ──────────────────────────────┐
│                                                    │
│  kvm_set_msi():                                    │
│    msg.address → LAPIC의 destination 결정          │
│    msg.data    → 인터럽트 벡터 결정                │
│    vlapic_deliver_msi() →                          │
│      가상 LAPIC에 인터럽트 inject                  │
│                                                    │
│  다음 VM-Entry 시:                                 │
│    VMCS의 VM-entry interruption-information에      │
│    인터럽트 벡터 설정                              │
│                                                    │
└────────────────────────────────────────────────────┘
         │ VM-Entry
         ▼
┌─ Guest VM ───────────────────────────────────────┐
│                                                    │
│  CPU가 인터럽트 수신                               │
│    → IDT에서 핸들러 검색                           │
│    → nvme_irq() 실행 (Linux 커널 NVMe 드라이버)   │
│      → nvme_process_cq_io()                        │
│      → 완료된 I/O 처리                             │
│                                                    │
└────────────────────────────────────────────────────┘
```

**MSI-X의 핵심 원리:**

실제 하드웨어에서 MSI-X는 PCIe Memory Write TLP이다. 디바이스가 특정 주소(LAPIC 주소 영역 `0xFEE0_xxxx`)에 데이터(인터럽트 벡터 번호)를 쓰면, Root Complex가 이를 인터럽트로 변환한다. QEMU에서는 이 "메모리 쓰기"를 직접 수행하는 대신, KVM의 `KVM_SIGNAL_MSI` ioctl을 호출하여 가상 LAPIC에 직접 인터럽트를 전달한다.

---

## 5. CMB (Controller Memory Buffer) 에뮬레이션

### 5.1 CMB 개념

CMB는 NVMe 컨트롤러 내부에 위치하는 메모리 버퍼로, SQ를 이 버퍼에 배치하면 호스트가 SQ에 명령을 쓸 때 PCIe Memory Write TLP만 발생하고, 컨트롤러가 SQ를 읽을 때 DMA가 불필요하다 (로컬 메모리 접근).

```
CMB 없이:
  Host → writel(SQE, host_ram)           → PCIe DMA 필요 없음
  SSD  → DMA Read(host_ram) → SQE 획득  → PCIe DMA 필요!

CMB 사용:
  Host → writel(SQE, cmb_bar)           → PCIe Write TLP
  SSD  → 내부 메모리 읽기 → SQE 획득   → DMA 불필요!
```

### 5.2 QEMU CMB 초기화: nvme_init_cmb()

```c
/* ctrl.c:8795 */
static void nvme_init_cmb(NvmeCtrl *n, PCIDevice *pci_dev)
{
    uint64_t cmb_size = n->params.cmb_size_mb * MiB;
    uint64_t cap = ldq_le_p(&n->bar.cap);

    /* CMB 버퍼 할당 (QEMU 프로세스 메모리) */
    n->cmb.buf = g_malloc0(cmb_size);

    /* CMB용 MemoryRegion 생성 (nvme_cmb_ops 핸들러 연결) */
    memory_region_init_io(&n->cmb.mem, OBJECT(n), &nvme_cmb_ops, n,
                          "nvme-cmb", cmb_size);

    /* BAR2에 CMB 등록 (64-bit prefetchable) */
    pci_register_bar(pci_dev, NVME_CMB_BIR,
                     PCI_BASE_ADDRESS_SPACE_MEMORY |
                     PCI_BASE_ADDRESS_MEM_TYPE_64 |
                     PCI_BASE_ADDRESS_MEM_PREFETCH, &n->cmb.mem);

    /* CAP 레지스터에 CMB 지원 플래그 설정 */
    NVME_CAP_SET_CMBS(cap, 1);
    stq_le_p(&n->bar.cap, cap);
}
```

### 5.3 CMB MMIO 핸들러

```c
/* ctrl.c:8545-8565 */
static void nvme_cmb_write(void *opaque, hwaddr addr, uint64_t data,
                           unsigned size)
{
    NvmeCtrl *n = (NvmeCtrl *)opaque;
    /* Guest가 CMB BAR에 쓰면 → QEMU의 cmb.buf에 저장 */
    stn_le_p(&n->cmb.buf[addr], size, data);
}

static uint64_t nvme_cmb_read(void *opaque, hwaddr addr, unsigned size)
{
    NvmeCtrl *n = (NvmeCtrl *)opaque;
    /* Guest가 CMB BAR을 읽으면 → cmb.buf에서 반환 */
    return ldn_le_p(&n->cmb.buf[addr], size);
}

static const MemoryRegionOps nvme_cmb_ops = {
    .read = nvme_cmb_read,
    .write = nvme_cmb_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,   /* CMB는 1바이트 접근도 허용 */
        .max_access_size = 8,
    },
};
```

### 5.4 CMB 주소 판별과 직접 접근

nvme_addr_read()에서 CMB 주소이면 DMA 대신 직접 memcpy한다:

```c
/* ctrl.c:517 */
static bool nvme_addr_is_cmb(NvmeCtrl *n, hwaddr addr)
{
    hwaddr hi, lo;
    if (!n->cmb.cmse) return false;

    lo = n->params.legacy_cmb ? n->cmb.mem.addr : n->cmb.cba;
    hi = lo + int128_get64(n->cmb.mem.size) - 1;
    return addr >= lo && addr <= hi;
}

static inline void *nvme_addr_to_cmb(NvmeCtrl *n, hwaddr addr)
{
    hwaddr base = n->params.legacy_cmb ? n->cmb.mem.addr : n->cmb.cba;
    return &n->cmb.buf[addr - base];  /* 직접 포인터 반환 */
}
```

CMB 전체 아키텍처:

```
Guest VM                            QEMU Process
┌────────────────────┐              ┌────────────────────────┐
│ NVMe 드라이버:     │              │ NvmeCtrl:              │
│                    │              │                        │
│ SQ in CMB:         │   MMIO       │ n->cmb.buf:            │
│ writel(SQE, cmb)───┼─── trap ────→│ [SQE data stored]     │
│                    │              │        │                │
│                    │              │        │ nvme_addr_read │
│                    │              │        │ → memcpy       │
│                    │              │        ▼                │
│                    │              │ nvme_process_sq():      │
│                    │              │   직접 cmb.buf에서 읽기 │
│                    │              │   (DMA 불필요!)         │
└────────────────────┘              └────────────────────────┘
```

---

## 6. PCIe Config Space 에뮬레이션

### 6.1 Config Space 설정: nvme_init_pci()

```c
/* ctrl.c:8940 */
static bool nvme_init_pci(NvmeCtrl *n, PCIDevice *pci_dev, Error **errp)
{
    uint8_t *pci_conf = pci_dev->config;  /* 256+4096 바이트 Config Space */

    /* 인터럽트 핀 설정 (VF는 INTx 미사용) */
    pci_conf[PCI_INTERRUPT_PIN] = pci_is_vf(pci_dev) ? 0 : 1;

    /* Programming Interface: 0x02 = NVMe */
    pci_config_set_prog_interface(pci_conf, 0x2);

    /* Vendor/Device ID 설정 */
    if (n->params.use_intel_id) {
        pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_INTEL);
        pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_INTEL_NVME);
    } else {
        pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_REDHAT);
        pci_config_set_device_id(pci_conf, PCI_DEVICE_ID_REDHAT_NVME);
    }

    /* Class Code: 0x010802 = Mass Storage / NVMe */
    pci_config_set_class(pci_conf, PCI_CLASS_STORAGE_EXPRESS);

    /* Power Management Capability (offset 0x60) */
    nvme_add_pm_capability(pci_dev, 0x60);

    /* ★ PCIe Endpoint Capability (offset 0x80) */
    pcie_endpoint_cap_init(pci_dev, 0x80);

    /* Function Level Reset Capability */
    pcie_cap_flr_init(pci_dev);

    /* SR-IOV인 경우 ARI (Alternative Routing-ID Interpretation) */
    if (n->params.sriov_max_vfs) {
        pcie_ari_init(pci_dev, 0x100);
    }

    /* ... BAR 등록, MSI-X 초기화 (위에서 설명) ... */
}
```

### 6.2 Config Space 레이아웃

```
PCI Configuration Space (256 bytes, Type 0 Header):
┌──────────┬──────────┬──────────────────────────────────────┐
│ Offset   │ 크기     │ 내용                                 │
├──────────┼──────────┼──────────────────────────────────────┤
│ 0x00     │ 2        │ Vendor ID (0x1B36=Red Hat / 0x8086=Intel)
│ 0x02     │ 2        │ Device ID                            │
│ 0x04     │ 2        │ Command (Bus Master, Memory Space)   │
│ 0x06     │ 2        │ Status                               │
│ 0x08     │ 1        │ Revision ID = 2                      │
│ 0x09     │ 3        │ Class Code = 0x010802 (NVMe)         │
│ 0x10     │ 8        │ BAR0 (64-bit, NVMe MMIO + Doorbell)  │
│ 0x18     │ 8        │ BAR2 (64-bit, CMB, 설정된 경우)      │
│ 0x20     │ 8        │ BAR4 (64-bit, MSI-X, exclusive 모드) │
│ 0x3C     │ 1        │ Interrupt Line                       │
│ 0x3D     │ 1        │ Interrupt Pin = 1                    │
├──────────┼──────────┼──────────────────────────────────────┤
│ 0x60     │ 8        │ Power Management Capability          │
│ 0x80     │ 60+      │ PCI Express Capability               │
│          │          │  ├── Device Capabilities              │
│          │          │  ├── Device Control/Status            │
│          │          │  └── Link Capabilities/Control/Status │
│ varies   │ varies   │ MSI-X Capability                     │
└──────────┴──────────┴──────────────────────────────────────┘

PCIe Extended Configuration Space (4096 bytes):
┌──────────┬──────────┬──────────────────────────────────────┐
│ 0x100    │ varies   │ ARI Capability (SR-IOV 시)           │
│ 0x120    │ varies   │ SR-IOV Capability (SR-IOV 시)        │
│ varies   │ varies   │ DOE Capability (SPDM 시)             │
└──────────┴──────────┴──────────────────────────────────────┘
```

### 6.3 QEMU가 자동 관리하는 부분 vs NVMe가 설정하는 부분

```
QEMU PCI 인프라가 자동 관리:
  ├── Config Space 읽기/쓰기 에뮬레이션 (pci_default_read/write_config)
  ├── BAR 주소 프로그래밍 및 MemoryRegion 매핑/언매핑
  ├── Command 레지스터 (Bus Master Enable 등)
  ├── Status 레지스터
  └── Capability List 관리

NVMe가 명시적으로 설정:
  ├── Vendor ID, Device ID (pci_config_set_vendor_id 등)
  ├── Class Code (pci_config_set_class)
  ├── Programming Interface (pci_config_set_prog_interface)
  ├── Interrupt Pin
  ├── PCIe Capability 등록 (pcie_endpoint_cap_init)
  ├── MSI-X 초기화 (msix_init)
  └── BAR 크기 및 MemoryRegion 연결 (pci_register_bar)
```

---

## 7. 전체 연결 다이어그램: NVMe Read I/O 완전한 콜 체인

하나의 NVMe Read 명령이 Guest `writel()`부터 Guest 인터럽트까지 QEMU 내부에서 거치는 모든 함수를 추적한다.

### 7.1 Sequence Diagram

```
Guest (Linux NVMe Driver)          QEMU (NVMe Emulation)         Guest (IRQ)
═══════════════════════         ═══════════════════════════     ═════════════
        │                                  │                         │
   [1] nvme_submit_cmd()                   │                         │
        │ SQ에 SQE 기록                    │                         │
        │ wmb()                            │                         │
        │                                  │                         │
   [2] writel(sq->tail,                    │                         │
        dev->bar + SQ_TDBL) ─── VM-Exit ──→│                         │
        │                          nvme_mmio_write(addr=0x1008,      │
        │                                          data=new_tail)    │
        │                            │                               │
        │                            ▼                               │
        │                     [3] nvme_process_db(n, 0x1008, tail)   │
        │                            │ qid = (0x1008 - 0x1000)/8 = 1│
        │                            │ sq->tail = new_tail           │
        │                            │                               │
        │                            ▼                               │
        │                     [4] qemu_bh_schedule(sq->bh)           │
        │                            │ Bottom Half 예약              │
        │                     ───── VM-Entry ────→│                  │
        │                                         │ (Guest 계속 실행)│
        │                            │                               │
        │ (QEMU main loop에서       │                               │
        │  BH 실행)                  │                               │
        │                            ▼                               │
        │                     [5] nvme_process_sq(sq)                │
        │                            │                               │
        │                            │ addr = sq->dma_addr           │
        │                            │        + (sq->head << 6)      │
        │                            │                               │
        │                            ▼                               │
        │                     [6] nvme_addr_read(n, addr, &cmd, 64)  │
        │                            │ → pci_dma_read()              │
        │                            │ → Guest RAM에서 SQE 복사      │
        │                            │                               │
        │                            ▼                               │
        │                     [7] nvme_io_cmd(n, req)                │
        │                            │                               │
        │                            ▼                               │
        │                     [8] nvme_read(n, req)                  │
        │                            │                               │
        │                            ▼                               │
        │                     [9] nvme_map_dptr(n, &sg, len, &cmd)   │
        │                            │ → nvme_map_prp() 또는         │
        │                            │   nvme_map_sgl()              │
        │                            │ Guest 물리 주소 목록 수집     │
        │                            │                               │
        │                            ▼                               │
        │                    [10] blk_aio_preadv()                   │
        │                            │ QEMU 블록 레이어에 읽기 요청  │
        │                            │ (비동기: 호스트 디스크/이미지  │
        │                            │  에서 데이터 읽기)            │
        │                            │                               │
        │                     ~~~ 비동기 I/O 진행 중 ~~~             │
        │                            │                               │
        │                            ▼                               │
        │                    [11] nvme_rw_complete_cb(req, ret)       │
        │                            │ 읽기 완료 콜백                │
        │                            │ → DMA로 데이터를 Guest 메모리 │
        │                            │   에 기록                     │
        │                            │                               │
        │                            ▼                               │
        │                    [12] nvme_enqueue_req_completion(cq,req) │
        │                            │ req->status = NVME_SUCCESS    │
        │                            │ req를 CQ의 req_list에 추가    │
        │                            │                               │
        │                            ▼                               │
        │                    [13] qemu_bh_schedule(cq->bh)           │
        │                            │                               │
        │                            ▼                               │
        │                    [14] nvme_post_cqes(cq)                 │
        │                            │ CQE 구성:                     │
        │                            │   status, sq_id, sq_head, cid │
        │                            │                               │
        │                            ▼                               │
        │                    [15] pci_dma_write(dev, cq_addr,        │
        │                                       &cqe, 16)            │
        │                            │ Guest CQ에 CQE 16바이트 기록  │
        │                            │                               │
        │                            ▼                               │
        │                    [16] nvme_irq_assert(n, cq)             │
        │                            │                               │
        │                            ▼                               │
        │                    [17] msix_notify(pci_dev, cq->vector)   │
        │                            │ → msix_get_message()          │
        │                            │ → msi_send_message()          │
        │                            │ → KVM_SIGNAL_MSI ioctl        │
        │                            │                               │
        │                            │           ┌───────────────────┘
        │                            │           │
        │                            │           ▼
        │                            │    [18] Guest CPU 인터럽트 수신
        │                            │           │
        │                            │           ▼
        │                            │    [19] nvme_irq() 실행
        │                            │           │
        │                            │           ▼
        │                            │    [20] nvme_irqhandler()
        │                            │           │ nvme_process_cq()
        │                            │           │ CQE 읽기 및 처리
        │                            │           │
        │                            │           ▼
        │                            │    [21] writel(cq->head,
        │                            │          dev->bar+CQ_HDBL)
        │                            │           │
        │                            │    ──VM-Exit──→
        │                            │                │
        │                            │           nvme_process_db()
        │                            │           cq->head 갱신
        │                            │           인터럽트 해제
        ▼                            ▼                ▼
```

### 7.2 함수 호출 체인 요약 (번호순)

```
[Guest] nvme_submit_cmd()          ← SQE를 SQ에 기록
[Guest] writel(tail, SQ_TDBL)      ← Doorbell 쓰기
  ─── VM-Exit ───
[QEMU]  nvme_mmio_write()         ← MMIO 핸들러 진입
[QEMU]    nvme_process_db()       ← Doorbell 처리, SQ tail 갱신
[QEMU]      qemu_bh_schedule()    ← BH 스케줄
[QEMU]  nvme_process_sq()         ← SQ에서 SQE fetch
[QEMU]    nvme_addr_read()        ← DMA: Guest RAM → cmd
[QEMU]      pci_dma_read()
[QEMU]    nvme_io_cmd()           ← 명령 디스패치
[QEMU]      nvme_read()           ← Read 명령 처리
[QEMU]        nvme_map_prp()      ← PRP 디코딩 → SG 리스트
[QEMU]        blk_aio_preadv()    ← 비동기 블록 I/O 시작
  ~~~ 비동기 ~~~
[QEMU]  nvme_rw_complete_cb()     ← I/O 완료 콜백
[QEMU]    nvme_enqueue_req_completion() ← CQ에 완료 등록
[QEMU]      qemu_bh_schedule()    ← CQ BH 스케줄
[QEMU]  nvme_post_cqes()          ← CQE를 Guest CQ에 기록
[QEMU]    pci_dma_write()         ← DMA: cqe → Guest RAM
[QEMU]    nvme_irq_assert()       ← 인터럽트 발생
[QEMU]      msix_notify()
[QEMU]        msi_send_message()
[QEMU]          KVM_SIGNAL_MSI
  ─── VM-Entry ───
[Guest] nvme_irq()                ← 인터럽트 핸들러
[Guest]   nvme_process_cq()       ← CQE 처리
[Guest]   writel(head, CQ_HDBL)   ← CQ Head Doorbell
  ─── VM-Exit ───
[QEMU]  nvme_process_db()         ← CQ head 갱신, IRQ 해제
```

---

## 8. Linux 커널 NVMe 드라이버 코드와의 1:1 대응

### 8.1 대응 테이블

| 단계 | Linux 커널 NVMe 드라이버 (`drivers/nvme/host/pci.c`) | QEMU NVMe 에뮬레이터 (`hw/nvme/ctrl.c`) | 설명 |
|------|------|------|------|
| BAR 매핑 | `pci_ioremap_bar(pdev, 0)` → `dev->bar` | `memory_region_init_io(&n->iomem, ..., &nvme_mmio_ops)` | Guest가 ioremap하면 EPT에 MMIO로 등록됨. QEMU에서는 MemoryRegion으로 MMIO 핸들러 등록 |
| CAP 읽기 | `readq(dev->bar + NVME_REG_CAP)` | `nvme_mmio_read(opaque, 0x00, 8)` → `ldn_le_p(ptr+0, 8)` | Guest의 readq가 VM-Exit → QEMU가 bar.cap 반환 |
| CC 쓰기 | `writel(cc, dev->bar + NVME_REG_CC)` | `nvme_write_bar(n, 0x14, data, 4)` → `case NVME_REG_CC:` | CC.EN=1이면 nvme_start_ctrl() 호출 |
| AQA 설정 | `writel(aqa, dev->bar + NVME_REG_AQA)` | `nvme_write_bar(n, 0x24, data, 4)` → `stl_le_p(&n->bar.aqa, data)` | Admin Queue 크기 설정 |
| ASQ 설정 | `writeq(asq, dev->bar + NVME_REG_ASQ)` | `nvme_write_bar(n, 0x28, data, 8)` → `stn_le_p(&n->bar.asq, size, data)` | Admin SQ 물리 주소 설정 |
| ACQ 설정 | `writeq(acq, dev->bar + NVME_REG_ACQ)` | `nvme_write_bar(n, 0x30, data, 8)` → `stn_le_p(&n->bar.acq, size, data)` | Admin CQ 물리 주소 설정 |
| 컨트롤러 Enable | `writel(CC \| CC_ENABLE, dev->bar + CC)` | `nvme_write_bar(n, 0x14, ...)` → `NVME_CC_EN(data)` → `nvme_start_ctrl(n)` | CC.EN 0→1 전환 시 컨트롤러 시작 |
| CSTS.RDY 폴링 | `readl(dev->bar + NVME_REG_CSTS) & CSTS_RDY` | `nvme_mmio_read(opaque, 0x1C, 4)` → `n->bar.csts` 반환 | QEMU는 nvme_start_ctrl 성공 시 즉시 RDY=1 설정 |
| SQ에 명령 기록 | `memcpy(sq->sq_cmds + sq->sq_tail * 64, cmd, 64)` | (Guest RAM에 직접 기록, QEMU 개입 없음) | SQ는 Guest RAM에 있으므로 일반 메모리 쓰기 |
| SQ Doorbell | `writel(sq->sq_tail, q_db)` | `nvme_mmio_write()` → `nvme_process_db()` → `qemu_bh_schedule(sq->bh)` | Doorbell 쓰기가 MMIO trap → QEMU가 SQ 처리 시작 |
| SQE Fetch | (하드웨어가 DMA로 자동 수행) | `nvme_process_sq()` → `nvme_addr_read(n, addr, &cmd, 64)` → `pci_dma_read()` | QEMU가 Guest RAM에서 SQE를 읽어옴 |
| 데이터 DMA | (하드웨어 DMA 엔진) | `nvme_map_prp()` → `pci_dma_read/write()` | PRP 디코딩 후 Guest RAM과 데이터 교환 |
| CQE 기록 | (하드웨어가 DMA로 CQ에 기록) | `nvme_post_cqes()` → `pci_dma_write(dev, cq_addr, &cqe, 16)` | QEMU가 Guest CQ에 CQE를 기록 |
| 인터럽트 | (하드웨어가 MSI-X TLP 전송) | `nvme_irq_assert()` → `msix_notify()` → `KVM_SIGNAL_MSI` | QEMU가 KVM을 통해 Guest에 인터럽트 전달 |
| IRQ 핸들러 | `nvme_irq()` → `nvme_process_cq()` | (Guest 내부 실행, QEMU 개입 없음) | Guest CPU가 인터럽트 핸들러 실행 |
| CQ Doorbell | `writel(cq->head, q_db + 4)` | `nvme_process_db()` → `cq->head = new_head` → `nvme_irq_deassert()` | Guest가 CQE 처리 완료 알림 |

### 8.2 대응 관계 ASCII 다이어그램

```
┌─ Linux 커널 NVMe 드라이버 ─────────┐    ┌─ QEMU NVMe 에뮬레이터 ──────────┐
│                                      │    │                                  │
│  nvme_probe()                        │    │  nvme_realize()                   │
│    pci_enable_device()               │←──→│    nvme_init_pci()               │
│    pci_ioremap_bar(0)                │    │      memory_region_init_io()     │
│    pci_set_master()                  │    │      pci_register_bar()          │
│    request_irq(msix)                 │    │      msix_init()                 │
│                                      │    │                                  │
│  nvme_init_ctrl_finish()             │    │                                  │
│    writel(AQA)  ─── MMIO ───────────→│    │    nvme_write_bar(AQA)           │
│    writeq(ASQ)  ─── MMIO ───────────→│    │    nvme_write_bar(ASQ)           │
│    writeq(ACQ)  ─── MMIO ───────────→│    │    nvme_write_bar(ACQ)           │
│    writel(CC|EN) ── MMIO ───────────→│    │    nvme_write_bar(CC)            │
│                                      │    │      nvme_start_ctrl()           │
│    readl(CSTS) ──── MMIO ───────────→│    │    nvme_mmio_read(CSTS) → RDY   │
│                                      │    │                                  │
│  nvme_submit_cmd():                  │    │                                  │
│    sq->cmds[tail] = cmd              │    │    (Guest RAM 직접 접근)         │
│    writel(tail, q_db) ─ MMIO ───────→│    │    nvme_process_db()             │
│                                      │    │      qemu_bh_schedule(sq->bh)   │
│                                      │    │                                  │
│                                      │    │  nvme_process_sq():              │
│    (SQE in Guest RAM) ←── DMA ───────│←───│    nvme_addr_read(sq_addr, 64)  │
│                                      │    │    nvme_io_cmd() → nvme_read()  │
│    (Data buffer) ←──── DMA ──────────│←───│    nvme_map_prp()               │
│                                      │    │    blk_aio_preadv()             │
│                                      │    │                                  │
│                                      │    │  nvme_rw_complete_cb():          │
│    (CQE in Guest CQ) ←─ DMA ────────│←───│    pci_dma_write(cq_addr, 16)   │
│                                      │    │                                  │
│  nvme_irq(): ←────── MSI-X ─────────│←───│    nvme_irq_assert()             │
│    nvme_process_cq()                 │    │      msix_notify()               │
│    writel(head, q_db+4) ─ MMIO ─────→│    │    nvme_process_db(CQ_HDBL)     │
│                                      │    │      nvme_irq_deassert()         │
│                                      │    │                                  │
└──────────────────────────────────────┘    └──────────────────────────────────┘
```

### 8.3 핵심 통찰: "MMIO Trap이 모든 것을 연결한다"

Linux 커널 NVMe 드라이버는 **실제 하드웨어를 대상으로 작성된 코드**를 아무런 수정 없이 실행한다. 드라이버는 자신이 가상 환경에서 실행되고 있다는 것을 전혀 모른다. 이 투명성을 가능하게 하는 것이 **MMIO Trap 메커니즘**이다:

1. **Guest의 `writel()`** → x86 MOV 명령 실행
2. **EPT Violation** → MMIO 주소는 RAM이 아니므로 하드웨어가 trap
3. **VM-Exit** → KVM이 QEMU에 제어 전달
4. **MemoryRegion Dispatch** → QEMU가 주소로 해당 디바이스 찾기
5. **nvme_mmio_write()** → NVMe 에뮬레이터 코드 실행
6. **DMA** → Guest RAM 접근으로 SQE fetch, CQE 기록
7. **MSI-X** → KVM을 통해 Guest에 인터럽트 전달
8. **VM-Entry** → Guest 계속 실행

이 전체 과정에서 Guest 드라이버 코드는 **단 한 바이트도 수정되지 않는다.** 하드웨어 가상화(VT-x/EPT)와 QEMU의 디바이스 에뮬레이션이 완벽하게 협력하여 투명한 NVMe 디바이스를 제공한다.

---

## 부록 A: 관련 소스 파일 위치

| 파일 | 역할 |
|------|------|
| `hw/nvme/ctrl.c` | NVMe 컨트롤러 에뮬레이션 (9566줄) |
| `hw/nvme/nvme.h` | NvmeCtrl, NvmeSQueue, NvmeCQueue 구조체 정의 |
| `include/block/nvme.h` | NvmeBar, NVMe 레지스터 오프셋/매크로 정의 |
| `hw/pci/pci.c` | PCI 디바이스 공통 인프라, pci_register_bar() |
| `hw/pci/msix.c` | MSI-X 에뮬레이션, msix_notify() |
| `include/hw/pci/pci_device.h` | pci_dma_read/write 인라인 함수 |
| `include/system/memory.h` | MemoryRegion, MemoryRegionOps 정의 |
| `system/memory.c` | memory_region_init_io(), flatview dispatch |

## 부록 B: QEMU NVMe 디바이스 생성 명령줄 예시

```bash
# 기본 NVMe 디바이스
qemu-system-x86_64 \
  -drive file=disk.qcow2,id=nvm,if=none \
  -device nvme,serial=deadbeef,drive=nvm

# CMB 활성화 (256MB)
qemu-system-x86_64 \
  -drive file=disk.qcow2,id=nvm,if=none \
  -device nvme,serial=deadbeef,drive=nvm,cmb_size_mb=256

# MSI-X 전용 BAR, I/O 큐 16개
qemu-system-x86_64 \
  -drive file=disk.qcow2,id=nvm,if=none \
  -device nvme,serial=deadbeef,drive=nvm,\
          msix_exclusive_bar=on,max_ioqpairs=16
```
