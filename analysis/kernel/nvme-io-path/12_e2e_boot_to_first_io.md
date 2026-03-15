# 전원 ON에서 첫 번째 NVMe I/O까지: 커널 부트 → I/O 완료 통합 분석

> **Capstone Document** — 블로그 시리즈 최종 정리 문서
> 소스코드 기준: ~/sources/linux (커널 6.x 계열)
> 핵심 파일: `drivers/nvme/host/pci.c`, `drivers/pci/probe.c`, `drivers/pci/pci-driver.c`, `block/blk-mq.c`

---

## 1. 전체 타임라인: 전원 ON → 첫 번째 NVMe I/O 완료

컴퓨터 전원 버튼을 누른 순간부터 `fio`가 첫 4KB 데이터를 읽어오는 순간까지, 소프트웨어 스택에서 일어나는 모든 일을 하나의 타임라인으로 정리한다.

```
╔══════════════════════════════════════════════════════════════════════╗
║                    전원 ON → 첫 번째 NVMe I/O 완료                    ║
╠══════════════════════════════════════════════════════════════════════╣
║                                                                      ║
║  [전원 ON] ─────────────────────────────────────────────────────────  ║
║      │                                                               ║
║      ▼ Phase 0: BIOS/UEFI (펌웨어)                                   ║
║      │  • CPU Reset Vector → UEFI 코드 시작                          ║
║      │  • PCIe Root Complex 초기화                                    ║
║      │  • PCIe 버스 스캔 (Bus:Dev:Func 열거)                          ║
║      │  • Config Space 읽기 → Vendor/Device ID 확인                   ║
║      │  • BAR sizing → 물리 주소 할당 (예: BAR0 = 0xFB000000)         ║
║      │  • ACPI MCFG 테이블에 ECAM base 주소 기록                      ║
║      │  • ACPI 테이블 구성 (DSDT/SSDT에 PCI 호스트 브리지 정의)        ║
║      │                                                               ║
║      ▼ Phase 1: 커널 초기화 (start_kernel)                            ║
║      │  • GRUB → 커널 로드 → start_kernel() 진입                     ║
║      │  • setup_arch() → ACPI 파싱 → MCFG 테이블 발견                 ║
║      │  • ECAM 영역 ioremap → Config Space MMIO 접근 가능             ║
║      │  • PCI 서브시스템 초기화 (pci_subsys_init)                      ║
║      │                                                               ║
║      ▼ Phase 2: PCIe 버스 스캔 (Enumeration)                         ║
║      │  • pci_scan_child_bus() → 모든 슬롯 순회                       ║
║      │  • Config Space에서 Vendor ID 읽기 → 디바이스 발견              ║
║      │  • pci_setup_device() → BAR 값/class code 파싱                 ║
║      │  • pci_init_capabilities() → MSI-X, PM 등 케이퍼빌리티 초기화  ║
║      │  • pci_device_add() → sysfs 등록 → uevent 발생                 ║
║      │                                                               ║
║      ▼ Phase 3: NVMe 드라이버 로드 & 매칭                             ║
║      │  • module_init(nvme_init) 실행                                 ║
║      │  • pci_register_driver(&nvme_driver)                           ║
║      │  • driver_attach() → 이미 등록된 디바이스와 매칭 시도            ║
║      │  • pci_bus_match(): class code 0x010802 확인 → 매치!           ║
║      │  • pci_device_probe() → nvme_probe() 호출                      ║
║      │                                                               ║
║      ▼ Phase 4: NVMe 컨트롤러 초기화                                  ║
║      │  • nvme_dev_map(): BAR0 ioremap → MMIO 접근 가능               ║
║      │  • nvme_pci_enable(): PCI 활성화 + Bus Master Enable           ║
║      │  • CAP 레지스터 읽기 → 큐 깊이, Doorbell stride 결정            ║
║      │  • dev->dbs = dev->bar + 4096 (Doorbell 시작 주소)              ║
║      │  • Admin Queue DMA 메모리 할당 (SQ/CQ 버퍼)                    ║
║      │  • AQA/ASQ/ACQ 레지스터에 Admin Queue 주소 기록                 ║
║      │  • CC.EN=1 → 컨트롤러 활성화 (CSTS.RDY 대기)                   ║
║      │  • Identify Controller 커맨드 (Admin Queue 경유)                ║
║      │  • Set Number of Queues → I/O 큐 수 협상                       ║
║      │  • Create I/O CQ → Create I/O SQ (Admin 커맨드)                ║
║      │  • blk_mq_alloc_tag_set() → request_queue → gendisk            ║
║      │  • device_add_disk() → /dev/nvme0n1 블록 디바이스 생성!         ║
║      │                                                               ║
║      ▼ Phase 5: 첫 번째 I/O                                          ║
║      │  • fio/dd → read() syscall → VFS → block layer                 ║
║      │  • blk_mq_submit_bio(): bio → request 변환 + tag 할당          ║
║      │  • nvme_queue_rq(): NVMe Read 커맨드 빌드                      ║
║      │  • nvme_sq_copy_cmd(): SQ[tail]에 64바이트 SQE 복사             ║
║      │  • nvme_write_sq_db(): writel(tail, q_db) → Doorbell!          ║
║      │  • ─── PCIe Memory Write TLP → 컨트롤러 ───                    ║
║      │  • 컨트롤러: SQ fetch → NAND 읽기 → CQ에 CQE 기록              ║
║      │  • ─── MSI-X 인터럽트 ───                                      ║
║      │  • nvme_irq() → nvme_poll_cq() → Phase bit 확인                ║
║      │  • nvme_handle_cqe() → blk_mq_complete_request()               ║
║      │  • bio_endio() → 유저 버퍼에 데이터 전달                        ║
║      │                                                               ║
║  [첫 번째 I/O 완료! read() 리턴] ───────────────────────────────────  ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝
```

### 시간 축 감각

| Phase | 대략적 소요 시간 | 설명 |
|-------|-----------------|------|
| Phase 0 | 수 초 | BIOS POST + PCIe 열거 |
| Phase 1 | 수백 ms | 커널 부트 초기 |
| Phase 2 | 수십 ms | PCI 서브시스템 초기화 |
| Phase 3 | 수 ms | 드라이버 매칭 + probe 진입 |
| Phase 4 | 수백 ms ~ 수 초 | 컨트롤러 Enable + 큐 생성 (CSTS.RDY 대기가 가장 김) |
| Phase 5 | 수십 μs | 첫 I/O 왕복 (NVMe 4KB Read 레이턴시) |

---

## 2. Phase 0: BIOS/UEFI — 하드웨어 세계의 시작

커널이 시작되기 전, BIOS/UEFI 펌웨어가 하드웨어를 초기화한다. 이 단계는 커널 소스코드에 없지만, 커널이 물려받는 모든 것의 기반이다.

### 2.1 PCIe 토폴로지 스캔

UEFI는 PCIe Root Complex를 통해 모든 버스를 순회한다:

```
CPU ─── Root Complex (Bus 0)
            │
            ├── Slot 0: GPU (Bus 1)
            ├── Slot 1: NVMe SSD (Bus 2, Dev 0, Func 0)
            │           Vendor: 0x144D (Samsung)
            │           Device: 0xA808
            │           Class:  0x010802 (NVMe)
            └── Slot 2: NIC (Bus 3)
```

UEFI가 각 디바이스의 Config Space를 읽어 존재 여부를 확인한다. Config Space 접근은 처음에는 Legacy I/O 포트(0xCF8/0xCFC)로 시작하여, MCFG(Memory Mapped Configuration)가 설정되면 ECAM 메모리 매핑으로 전환된다.

### 2.2 BAR 주소 할당

NVMe 디바이스의 BAR0에 물리 주소를 할당한다:

```
BAR Sizing 과정 (UEFI):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
1. BAR0 레지스터에 0xFFFFFFFF 기록
2. BAR0 값 읽기 → 0xFFFF4000 반환
   (하위 비트가 0 → 이 디바이스는 16KB 이상의 메모리 필요)
3. ~(0xFFFF4000) + 1 = 0xC000 = 48KB 크기
4. 48KB 경계에 맞춰 물리 주소 할당: BAR0 = 0xFB000000
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

이 주소가 나중에 커널의 ioremap()으로 매핑되어
NVMe 컨트롤러 레지스터(CAP, CC, CSTS, Doorbell 등)에
접근하는 데 사용된다.
```

### 2.3 ACPI 테이블 구성

UEFI는 커널에 전달할 ACPI 테이블을 구성한다:

```
ACPI 테이블 중 PCI 관련:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
• MCFG (Memory Mapped Configuration):
  - ECAM base address = 0xB0000000
  - Segment 0, Bus 0~255
  - 커널이 이 주소를 ioremap하여 Config Space에 MMIO 접근

• DSDT/SSDT:
  - PCI Host Bridge 디바이스 정의 (\_SB.PCI0)
  - _CRS 메서드: 버스 번호 범위, I/O 포트 범위, MMIO 범위
  - _BBN: Base Bus Number
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 3. Phase 1: 커널 초기화 — PCI 서브시스템 준비

### 3.1 부팅 초기 호출 체인

```
start_kernel()                               // init/main.c
  │
  ├── setup_arch(&command_line)              // arch/x86/kernel/setup.c
  │     ├── acpi_boot_init()                 // ACPI 테이블 파싱 시작
  │     │     └── acpi_table_parse(ACPI_SIG_MCFG, ...)
  │     │           └── MCFG 테이블에서 ECAM base 주소 추출
  │     │
  │     └── x86_init.pci.init()              // PCI 초기 설정
  │           └── pci_acpi_init()            // ACPI 기반 PCI 설정
  │
  ├── ... (메모리, 스케줄러, 파일시스템 등 초기화) ...
  │
  └── do_initcalls()                         // 드라이버 초기화 단계
        │
        ├── [subsys_initcall]
        │     └── pci_subsys_init()          // PCI 서브시스템 활성화
        │           └── pcibios_init()
        │
        ├── [subsys_initcall]
        │     └── acpi_pci_root_init()       // ACPI PCI Root Bridge 드라이버 등록
        │           └── acpi_bus_register_driver(&acpi_pci_root_driver)
        │
        └── [device_initcall / module_init]
              └── nvme_init()                // NVMe 드라이버 등록 (Phase 3)
```

### 3.2 ECAM 매핑의 의미

ECAM(Enhanced Configuration Access Mechanism)은 PCIe Config Space를 메모리 주소로 매핑하는 표준이다:

```
ECAM 주소 계산:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
ECAM_base + (Bus << 20) + (Device << 15) + (Function << 12) + Register

예: Bus 2, Dev 0, Func 0, Register 0 (Vendor ID):
  0xB0000000 + (2 << 20) + (0 << 15) + (0 << 12) + 0
= 0xB0200000

이 주소를 readl()하면 NVMe SSD의 Vendor ID를 읽을 수 있다.
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

                    ┌───────────────────────────────────┐
                    │         ECAM 메모리 영역            │
                    │  0xB0000000 ~ 0xBFFFFFFF (256MB)   │
                    │                                     │
                    │  Bus 0: 0xB0000000 ~ 0xB00FFFFF    │
                    │  Bus 1: 0xB0100000 ~ 0xB01FFFFF    │
  NVMe SSD ──────→ │  Bus 2: 0xB0200000 ~ 0xB02FFFFF    │
                    │  ...                                │
                    │  Bus 255: 0xBFF00000 ~ 0xBFFFFFFF  │
                    └───────────────────────────────────┘
```

이 ECAM 매핑이 있어야 Phase 2의 PCIe 버스 스캔이 가능하다. 커널은 ACPI MCFG 테이블에서 ECAM base를 읽고, `ioremap()`으로 이 물리 주소를 가상 주소에 매핑한다.

---

## 4. Phase 2: PCIe Enumeration — 디바이스 발견과 등록

### 4.1 전체 호출 체인

ACPI PCI Root Bridge가 발견되면 버스 스캔이 시작된다:

```
acpi_pci_root_add()                          // drivers/acpi/pci_root.c
  │
  └── pci_acpi_scan_root()                   // arch/x86/pci/acpi.c
        │
        └── pci_scan_root_bus_bridge()       // drivers/pci/probe.c
              │
              └── pci_scan_child_bus()       // drivers/pci/probe.c:3237
                    │
                    └── pci_scan_child_bus_extend()  // probe.c:3114
                          │
                          │  /* 버스의 모든 슬롯(0~31)을 순회 */
                          │  for (devnr = 0; devnr < 32; devnr++)
                          │
                          └── pci_scan_slot(bus, PCI_DEVFN(devnr, 0))  // probe.c:2898
                                │
                                │  /* 슬롯의 모든 Function(0~7)을 순회 */
                                │  do {
                                │
                                └── pci_scan_single_device(bus, devfn)  // probe.c:2810
                                      │
                                      ├── pci_scan_device(bus, devfn)   // probe.c:2620
                                      │     │
                                      │     ├── pci_bus_read_dev_vendor_id()
                                      │     │     └── Config Space offset 0 읽기
                                      │     │         → 0xA808144D? 디바이스 존재!
                                      │     │         → 0xFFFFFFFF? 빈 슬롯, 스킵
                                      │     │
                                      │     ├── pci_alloc_dev(bus)
                                      │     │     └── struct pci_dev 할당
                                      │     │         dev->vendor = 0x144D (Samsung)
                                      │     │         dev->device = 0xA808
                                      │     │
                                      │     └── pci_setup_device(dev)   // probe.c:2018
                                      │           │
                                      │           ├── pci_hdr_type() → Header Type 읽기
                                      │           ├── dev->class 읽기 → 0x010802 (NVMe)
                                      │           ├── dev_set_name() → "0000:02:00.0"
                                      │           │
                                      │           └── (Header Type 0x00인 경우)
                                      │                 ├── pci_read_bases(dev, 6)
                                      │                 │     └── BAR0~BAR5 읽기
                                      │                 │         BAR0 = 0xFB000000 (MMIO, 64-bit)
                                      │                 │         → pci_dev->resource[0] 설정
                                      │                 │
                                      │                 ├── pci_read_irq(dev)
                                      │                 │     └── Interrupt Line/Pin 읽기
                                      │                 │
                                      │                 └── pci_read_config_word(PCI_SUBSYSTEM_*)
                                      │
                                      └── pci_device_add(dev, bus)
                                            │
                                            ├── pci_init_capabilities(dev) // probe.c:2682
                                            │     ├── pci_msi_init(dev)    // MSI 초기 설정
                                            │     ├── pci_msix_init(dev)   // MSI-X 초기 설정
                                            │     ├── pci_pm_init(dev)     // 전원 관리
                                            │     ├── pci_iov_init(dev)    // SR-IOV
                                            │     ├── pci_ats_init(dev)    // ATS
                                            │     └── pci_aer_init(dev)    // AER
                                            │
                                            └── device_add(&dev->dev)
                                                  └── sysfs 등록
                                                      → /sys/bus/pci/devices/0000:02:00.0/
                                                      → kobject_uevent() → udev에 알림
```

### 4.2 pci_setup_device()의 핵심 동작

`drivers/pci/probe.c:2018`에 위치한 이 함수는 Config Space를 파싱하여 `pci_dev` 구조체를 채운다:

```c
// probe.c:2018 (간략화)
int pci_setup_device(struct pci_dev *dev)
{
    hdr_type = pci_hdr_type(dev);          // Config Space 0x0E
    dev->hdr_type = hdr_type & 0x7F;

    class = pci_class(dev);                // Config Space 0x08~0x0B
    dev->revision = class & 0xff;          // Revision ID
    dev->class = class >> 8;               // Class Code (예: 0x010802)

    // 디바이스 이름 설정: "DDDD:BB:SS.F" 형식
    dev_set_name(&dev->dev, "%04x:%02x:%02x.%d",
        pci_domain_nr(dev->bus),            // Domain (보통 0000)
        dev->bus->number,                   // Bus (예: 02)
        PCI_SLOT(dev->devfn),               // Slot (예: 00)
        PCI_FUNC(dev->devfn));              // Function (예: 0)

    switch (dev->hdr_type) {
    case PCI_HEADER_TYPE_NORMAL:            // Type 0 (NVMe는 이것)
        pci_read_bases(dev, 6, PCI_ROM_ADDRESS);  // BAR0~5 + ROM
        pci_read_irq(dev);                          // IRQ 정보
        break;
    // ...
    }
}
```

### 4.3 BAR 파싱 결과

```
pci_dev->resource[0] (BAR0):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  .start = 0xFB000000        물리 시작 주소
  .end   = 0xFB00BFFF        물리 끝 주소 (48KB)
  .flags = IORESOURCE_MEM    메모리 매핑 타입
           | IORESOURCE_MEM_64  64비트 BAR
           | IORESOURCE_PREFETCH  프리페치 가능
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

이 BAR0 영역에 NVMe 컨트롤러의 모든 레지스터가 매핑된다:
  +0x0000: CAP   (Controller Capabilities)
  +0x0008: VS    (Version)
  +0x000C: INTMS (Interrupt Mask Set)
  +0x0010: INTMC (Interrupt Mask Clear)
  +0x0014: CC    (Controller Configuration)
  +0x001C: CSTS  (Controller Status)
  +0x0024: AQA   (Admin Queue Attributes)
  +0x0028: ASQ   (Admin SQ Base Address)
  +0x0030: ACQ   (Admin CQ Base Address)
  +0x1000: SQ0 Tail Doorbell  (Admin SQ)
  +0x1004: CQ0 Head Doorbell  (Admin CQ)
  +0x1008: SQ1 Tail Doorbell  (I/O SQ 1)
  +0x100C: CQ1 Head Doorbell  (I/O CQ 1)
  ...
```

### 4.4 이 시점의 데이터 구조 상태

```
Phase 2 완료 시점:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  struct pci_dev {
      .vendor    = 0x144D        // Samsung
      .device    = 0xA808        // NVMe SSD 모델
      .class     = 0x010802      // Mass Storage → NVM → NVMe
      .devfn     = 0x00          // Bus 2, Dev 0, Func 0
      .resource[0] = {           // BAR0
          .start = 0xFB000000
          .end   = 0xFB00BFFF
      }
      .driver    = NULL          // ← 아직 드라이버 미연결!
  }

  sysfs:
    /sys/bus/pci/devices/0000:02:00.0/
      ├── vendor       → "0x144d"
      ├── device       → "0xa808"
      ├── class        → "0x010802"
      ├── resource     → BAR 주소 목록
      └── driver/      → (없음 — 아직 매칭 전)
```

---

## 5. Phase 3: NVMe 드라이버 등록과 매칭

### 5.1 드라이버 등록 체인

NVMe 드라이버는 `module_init` 또는 빌트인 initcall로 등록된다:

```
nvme_init()                                  // pci.c:4987
  │
  │  /* 컴파일 타임 검증: NVMe 커맨드 구조체가 정확히 64바이트인지 확인 */
  │  BUILD_BUG_ON(sizeof(struct nvme_create_cq) != 64);
  │  BUILD_BUG_ON(sizeof(struct nvme_create_sq) != 64);
  │
  └── pci_register_driver(&nvme_driver)      // PCI 서브시스템에 드라이버 등록
        │
        └── __pci_register_driver()
              │
              └── driver_register(&drv->driver)
                    │
                    └── bus_add_driver(drv)
                          │
                          ├── driver_attach(drv)  // 이미 등록된 디바이스와 매칭
                          │     │
                          │     └── bus_for_each_dev(bus, NULL, drv, __driver_attach)
                          │           │
                          │           │  /* 등록된 모든 PCI 디바이스를 순회 */
                          │           │  for each pci_dev:
                          │           │
                          │           └── __driver_attach(dev, drv)
                          │                 │
                          │                 ├── driver_match_device(drv, dev)
                          │                 │     └── pci_bus_match(dev, drv)
                          │                 │           └── pci_match_device(drv, dev)
                          │                 │                 │
                          │                 │                 ├── nvme_id_table 검색
                          │                 │                 │   (Vendor/Device별 quirk 매칭)
                          │                 │                 │
                          │                 │                 └── PCI_ANY_ID 와일드카드 매칭
                          │                 │                     class = 0x010802
                          │                 │                     class_mask = 0xFFFFFF
                          │                 │                     → 매치 성공!
                          │                 │
                          │                 └── driver_probe_device(drv, dev)
                          │                       └── really_probe(dev, drv)
                          │                             └── call_driver_probe(dev, drv)
                          │                                   └── pci_device_probe(dev)
                          │
                          └── (sysfs에 드라이버 등록)
                                → /sys/bus/pci/drivers/nvme/
```

### 5.2 nvme_id_table — 디바이스 매칭 규칙

`pci.c:4762`에 정의된 ID 테이블은 두 종류의 매칭을 한다:

```c
// pci.c:4762 (발췌)
static const struct pci_device_id nvme_id_table[] = {
    // 1) Vendor+Device 특정 매칭 (quirk 적용):
    { PCI_VDEVICE(INTEL, 0x0953),       // Intel 750/P3500
      .driver_data = NVME_QUIRK_STRIPE_SIZE | NVME_QUIRK_DEALLOCATE_ZEROES },
    { PCI_VDEVICE(INTEL, 0x5845),       // QEMU 에뮬레이션
      .driver_data = NVME_QUIRK_IDENTIFY_CNS | NVME_QUIRK_BOGUS_NID },

    // 2) Class Code 와일드카드 매칭 (모든 NVMe 디바이스):
    { PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_EXPRESS, 0xFFFFFF) },
    // PCI_CLASS_STORAGE_EXPRESS = 0x010802 = Mass Storage / NVM / NVMe
    { 0, }  // 종료 마커
};
```

매칭 순서: Vendor+Device 특정 항목을 먼저 확인하고, 없으면 Class Code 와일드카드로 매칭한다. 특정 매칭이 되면 해당 SSD에 필요한 quirk(하드웨어 버그 우회)가 적용된다.

### 5.3 pci_device_probe() — 드라이버와 디바이스의 만남

```
pci_device_probe(dev)                        // pci-driver.c:435
  │
  ├── pci_assign_irq(pci_dev)               // IRQ 번호 할당
  │
  ├── pci_dev_get(pci_dev)                  // 참조 카운트 증가
  │
  └── __pci_device_probe(drv, pci_dev)      // pci-driver.c:407
        │
        ├── pci_match_device(drv, pci_dev)  // ID 테이블에서 매칭 항목 찾기
        │     → id 반환 (class=0x010802 매치)
        │
        └── pci_call_probe(drv, pci_dev, id)
              │
              └── local_pci_probe()
                    │
                    └── drv->probe(pci_dev, id)
                          │
                          └── ★ nvme_probe(pdev, id) ★  // Phase 4 시작!
```

---

## 6. Phase 4: NVMe 컨트롤러 초기화 — nvme_probe()에서 /dev/nvme0n1까지

### 6.1 nvme_probe() 전체 흐름

`pci.c:4368`에 정의된 `nvme_probe()`는 NVMe 드라이버의 진입점이다. 이 함수 하나가 완료되면 사용자는 `/dev/nvme0n1`으로 I/O를 수행할 수 있다.

```
nvme_probe(pdev, id)                         // pci.c:4368
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 1단계: nvme_dev 구조체 할당                               │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_pci_alloc_dev(pdev, id)
  │     ├── struct nvme_dev 할당 (kzalloc)
  │     ├── queues 배열 할당 (큐 수 = CPU 수 + 1)
  │     ├── dev->dev = &pdev->dev
  │     └── nvme_init_ctrl(&dev->ctrl, ...)
  │           └── ctrl->ops = &nvme_pci_ctrl_ops
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 2단계: nvme_core에 컨트롤러 등록                          │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_add_ctrl(&dev->ctrl)
  │     └── /dev/nvme0 캐릭터 디바이스 생성
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 3단계: BAR0 MMIO 매핑                                    │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_dev_map(dev)                      // pci.c:4175
  │     ├── pci_request_mem_regions(pdev, "nvme")
  │     │     └── BAR0 메모리 영역 예약 (다른 드라이버 접근 방지)
  │     └── nvme_remap_bar(dev, NVME_REG_DBS + 4096)
  │           └── ioremap(pci_resource_start(pdev, 0), size)
  │                 → dev->bar = ioremap(0xFB000000, ...)
  │                 → 이제 readl(dev->bar + NVME_REG_CAP) 가능!
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 4단계: DMA 메모리풀 할당                                  │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_pci_alloc_iod_mempool(dev)
  │     └── PRP/SGL 디스크립터용 DMA 벡터 메모리풀
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 5단계: PCI 디바이스 활성화 + Admin Queue 설정              │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_pci_enable(dev)                   // pci.c:3750
  │     │
  │     ├── pci_enable_device_mem(pdev)
  │     │     └── Config Space Command 레지스터: Memory Space Enable = 1
  │     │         → 이제 BAR0 MMIO 접근이 실제 동작함
  │     │
  │     ├── pci_set_master(pdev)
  │     │     └── Config Space Command 레지스터: Bus Master Enable = 1
  │     │         → 디바이스가 DMA(Memory Read/Write TLP) 가능
  │     │
  │     ├── pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES)
  │     │     └── 초기 MSI-X 벡터 1개 할당 (Admin Queue용)
  │     │
  │     ├── CAP 레지스터 읽기
  │     │     dev->ctrl.cap = lo_hi_readq(dev->bar + NVME_REG_CAP)
  │     │     ├── MQES: Maximum Queue Entries Supported
  │     │     ├── DSTRD: Doorbell Stride (보통 0 → 4바이트 간격)
  │     │     ├── MPSMIN/MPSMAX: 메모리 페이지 크기 범위
  │     │     └── TO: Timeout (CSTS.RDY 대기 시간)
  │     │
  │     ├── dev->q_depth = min(MQES + 1, io_queue_depth)
  │     ├── dev->db_stride = 1 << DSTRD      // 보통 1 (4바이트 단위)
  │     ├── dev->dbs = dev->bar + 4096        // Doorbell 시작 주소!
  │     │
  │     │  ┌───────────────────────────────────────────────┐
  │     │  │ Admin Queue 설정 (컨트롤러와 첫 통신 준비)      │
  │     │  └───────────────────────────────────────────────┘
  │     └── nvme_pci_configure_admin_queue(dev) // pci.c:2851
  │           │
  │           ├── nvme_disable_ctrl()          // CC.EN=0 (혹시 켜져있으면 끔)
  │           │
  │           ├── nvme_alloc_queue(dev, 0, NVME_AQ_DEPTH)  // qid=0, depth=32
  │           │     ├── SQ 버퍼: dma_alloc_coherent(depth * 64)
  │           │     │     → nvmeq->sq_cmds, nvmeq->sq_dma_addr
  │           │     └── CQ 버퍼: dma_alloc_coherent(depth * 16)
  │           │           → nvmeq->cqes, nvmeq->cq_dma_addr
  │           │
  │           ├── AQA 레지스터 설정
  │           │     aqa = (depth-1) | ((depth-1) << 16)
  │           │     writel(aqa, dev->bar + NVME_REG_AQA)
  │           │     // SQ 크기 31, CQ 크기 31 (0-based)
  │           │
  │           ├── ASQ/ACQ 레지스터 설정
  │           │     lo_hi_writeq(sq_dma_addr, dev->bar + NVME_REG_ASQ)
  │           │     lo_hi_writeq(cq_dma_addr, dev->bar + NVME_REG_ACQ)
  │           │     // 컨트롤러에게 Admin SQ/CQ의 DMA 주소를 알려줌
  │           │
  │           ├── nvme_enable_ctrl()           // CC.EN=1!
  │           │     ├── writel(CC값, dev->bar + NVME_REG_CC)
  │           │     │     CC.IOSQES = 6 (64바이트)
  │           │     │     CC.IOCQES = 4 (16바이트)
  │           │     │     CC.MPS = 0 (4KB 페이지)
  │           │     │     CC.EN = 1 ← 컨트롤러 시작!
  │           │     │
  │           │     └── CSTS.RDY=1 대기 (폴링)
  │           │           while (!(readl(bar+CSTS) & NVME_CSTS_RDY))
  │           │               msleep(100);
  │           │           // 이 대기가 Phase 4에서 가장 오래 걸린다
  │           │
  │           ├── nvme_init_queue(nvmeq, 0)    // 큐 포인터 초기화
  │           │     ├── nvmeq->sq_tail = 0
  │           │     ├── nvmeq->cq_head = 0
  │           │     ├── nvmeq->cq_phase = 1
  │           │     └── nvmeq->q_db = &dev->dbs[0]
  │           │
  │           └── queue_request_irq(nvmeq)     // Admin Queue 인터럽트 등록
  │                 └── pci_request_irq(pdev, 0, nvme_irq, ...)
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 6단계: Admin 큐 blk-mq 태그셋 생성                       │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_alloc_admin_tag_set(&dev->ctrl, &dev->admin_tagset, ...)
  │     └── Admin 커맨드 발행을 위한 blk-mq 인프라 구축
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 7단계: Identify Controller                               │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_init_ctrl_finish(&dev->ctrl, false)
  │     ├── Identify Controller 커맨드 발행 (Admin Queue 경유)
  │     │     → 모델명, 시리얼, FW 버전, MDTS, ONCS 등 확인
  │     ├── MDTS → max_hw_sectors 설정
  │     └── 기능 협상 (ONCS 비트 확인)
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 8~9단계: Shadow Doorbell + HMB                           │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_dbbuf_dma_alloc(dev)              // Shadow DB 버퍼 할당
  ├── nvme_setup_host_mem(dev)               // HMB 설정 (DRAM-less SSD용)
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 10단계: I/O 큐 생성                                      │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_setup_io_queues(dev)              // pci.c:3502
  │     │
  │     ├── nvme_set_queue_count(&dev->ctrl, &nr_io_queues)
  │     │     └── Set Number of Queues Admin 커맨드
  │     │         → 디바이스와 I/O 큐 수 협상
  │     │         → 예: 요청 8개, 디바이스 지원 4개 → 4개로 결정
  │     │
  │     ├── nvme_remap_bar(dev, db_bar_size)
  │     │     └── Doorbell 영역이 늘어나므로 BAR 매핑 확장
  │     │
  │     ├── nvme_setup_irqs(dev, nr_io_queues)
  │     │     └── pci_alloc_irq_vectors_affinity()
  │     │           → MSI-X 벡터 할당 (큐당 1개, CPU affinity 설정)
  │     │
  │     └── nvme_create_io_queues(dev)
  │           │
  │           │  for (qid = 1; qid <= nr_io_queues; qid++):
  │           │
  │           ├── nvme_alloc_queue(dev, qid, depth)
  │           │     ├── SQ 버퍼 DMA 할당
  │           │     └── CQ 버퍼 DMA 할당
  │           │
  │           ├── Create I/O CQ (Admin 커맨드 opcode=0x05)
  │           │     → CQ DMA 주소, 큐 깊이, 인터럽트 벡터 전달
  │           │
  │           ├── Create I/O SQ (Admin 커맨드 opcode=0x01)
  │           │     → SQ DMA 주소, 큐 깊이, 연결될 CQ ID 전달
  │           │
  │           └── nvme_init_queue(nvmeq, qid)
  │                 ├── sq_tail = 0, cq_head = 0, cq_phase = 1
  │                 └── q_db = &dev->dbs[qid * 2 * db_stride]
  │                       // 각 큐의 Doorbell 주소 계산!
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 11단계: I/O 큐 blk-mq 태그셋 생성                        │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_alloc_io_tag_set(&dev->ctrl, &dev->tagset, &nvme_mq_ops, ...)
  │     │
  │     ├── blk_mq_alloc_tag_set(&dev->tagset)
  │     │     ├── tagset->ops = &nvme_mq_ops
  │     │     │     .queue_rq  = nvme_queue_rq      // ★ I/O 제출 콜백
  │     │     │     .queue_rqs = nvme_queue_rqs     // 배치 제출
  │     │     │     .commit_rqs = nvme_commit_rqs   // Doorbell flush
  │     │     │     .complete  = nvme_pci_complete_rq
  │     │     │     .poll      = nvme_poll
  │     │     │
  │     │     └── tag 비트맵 할당 (sbitmap)
  │     │           → 최대 동시 I/O 수 결정 (큐 깊이 - 1)
  │     │
  │     └── nvme_dbbuf_set(dev)              // Shadow Doorbell 활성화
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ 12단계: 컨트롤러 LIVE + 네임스페이스 스캔                  │
  │  └─────────────────────────────────────────────────────────┘
  ├── nvme_change_ctrl_state(NVME_CTRL_LIVE) // I/O 가능 상태!
  │
  └── nvme_start_ctrl(&dev->ctrl)
        │
        └── nvme_queue_scan(&dev->ctrl)
              └── nvme_scan_work()            // (워크큐에서 비동기)
                    │
                    ├── Identify Namespace (Admin 커맨드)
                    │     → 네임스페이스 크기, LBA 크기 등 확인
                    │
                    └── nvme_alloc_ns() 또는 nvme_validate_or_alloc_ns()
                          │
                          ├── blk_mq_init_queue()
                          │     └── request_queue 생성
                          │
                          ├── device_add_disk()
                          │     └── disk 등록 → /dev/nvme0n1 생성!
                          │
                          └── ★ 사용자 I/O 가능 ★
```

### 6.2 Admin Queue를 통한 컨트롤러 초기화 시퀀스 다이어그램

```
     호스트 (커널)                          NVMe 컨트롤러
     ━━━━━━━━━━                          ━━━━━━━━━━━━━
         │                                     │
         │  ① CC.EN=0 (Disable)                 │
         │────────── writel(CC) ──────────────→│
         │                                     │ 컨트롤러 리셋
         │                                     │
         │  ② AQA/ASQ/ACQ 레지스터 설정          │
         │────────── writel(AQA) ─────────────→│
         │────────── writeq(ASQ) ─────────────→│ Admin SQ DMA 주소 저장
         │────────── writeq(ACQ) ─────────────→│ Admin CQ DMA 주소 저장
         │                                     │
         │  ③ CC.EN=1 (Enable)                  │
         │────────── writel(CC) ──────────────→│
         │                                     │ 초기화 시작...
         │  ④ CSTS.RDY=1 대기                    │
         │←──────── readl(CSTS) ──────────────│ (폴링)
         │              ...                    │
         │←──────── readl(CSTS) ──────────────│ RDY=1!
         │                                     │
         │  ⑤ Identify Controller               │
         │──── SQ[0]에 커맨드 복사 ───────────→│
         │──── Doorbell(SQ0 Tail) ────────────→│
         │                                     │ 4KB 데이터를 DMA로 전송
         │←───── DMA Write (4KB) ─────────────│
         │←───── CQ[0]에 CQE 기록 ────────────│
         │←───── MSI-X 인터럽트 ───────────────│
         │                                     │
         │  ⑥ Set Number of Queues              │
         │──── (같은 패턴) ───────────────────→│
         │←───── 완료 ────────────────────────│
         │                                     │
         │  ⑦ Create I/O CQ 1                   │
         │──── (같은 패턴) ───────────────────→│ CQ1 생성
         │←───── 완료 ────────────────────────│
         │                                     │
         │  ⑧ Create I/O SQ 1                   │
         │──── (같은 패턴) ───────────────────→│ SQ1 생성
         │←───── 완료 ────────────────────────│
         │                                     │
         │  (추가 I/O 큐가 있으면 ⑦⑧ 반복)       │
         │                                     │
         │  ★ 이제 I/O Queue를 통한 데이터 전송 가능! ★
         │                                     │
```

---

## 7. Phase 5: 첫 번째 I/O — fio에서 데이터 반환까지

### 7.1 사용자 공간 → 커널 진입

```bash
# 사용자가 실행하는 명령:
fio --filename=/dev/nvme0n1 --rw=read --bs=4k --numjobs=1 \
    --iodepth=1 --ioengine=sync --direct=1 --size=4k
```

이 명령은 하나의 4KB Direct I/O Read를 수행한다. 내부적으로 `read()` 시스템 콜이 호출된다.

```
유저 공간                              커널 공간
━━━━━━━━                              ━━━━━━━
fio process
  │
  └── read(fd, buf, 4096)
        │
        │  ═══ syscall 경계 (SYSCALL_DEFINE3) ═══
        │
        └── ksys_read(fd, buf, 4096)         // fs/read_write.c
              │
              └── vfs_read(file, buf, 4096, &pos)
                    │
                    └── file->f_op->read_iter()
                          │
                          └── blkdev_read_iter()  // block/fops.c
                                │
                                └── blkdev_direct_IO()
                                      │
                                      └── __blkdev_direct_IO()
                                            │
                                            ├── bio 할당 및 초기화
                                            │   bio->bi_bdev = bdev      // NVMe 블록 디바이스
                                            │   bio->bi_iter.bi_sector = sector
                                            │   bio->bi_opf = REQ_OP_READ
                                            │   bio_add_page(bio, page, 4096, 0)
                                            │
                                            └── submit_bio(bio)
                                                  │
                                                  └── submit_bio_noacct(bio)
                                                        │
                                                        └── __submit_bio(bio)
                                                              │
                                                              └── ★ blk_mq_submit_bio(bio) ★
```

### 7.2 blk-mq 계층: bio → request → NVMe 드라이버

`block/blk-mq.c:3697`의 `blk_mq_submit_bio()`가 블록 레이어의 핵심이다:

```
blk_mq_submit_bio(bio)                      // blk-mq.c:3697
  │
  ├── q = bdev_get_queue(bio->bi_bdev)      // NVMe 네임스페이스의 request_queue
  ├── plug = current->plug                   // 현재 태스크의 plug
  │
  ├── [유효성 검사]
  │     ├── bio_queue_enter(bio)             // 큐 사용 카운터 증가
  │     ├── bio_unaligned() 체크             // 정렬 확인
  │     └── __bio_split_to_limits()          // max_sectors 초과 시 분할
  │
  ├── [병합 시도]
  │     └── blk_mq_attempt_bio_merge()       // 기존 request에 합칠 수 있나?
  │           → 실패 (첫 I/O이므로 병합 대상 없음)
  │
  ├── [request 할당]
  │     └── blk_mq_get_new_requests(q, plug, bio)
  │           │
  │           ├── __blk_mq_alloc_requests()
  │           │     ├── blk_mq_get_tag()     // sbitmap에서 tag 할당
  │           │     │     → tag = 0 (첫 번째 I/O)
  │           │     │     → 이 tag가 NVMe command_id가 된다!
  │           │     │
  │           │     └── blk_mq_rq_ctx_init() // request 초기화
  │           │           rq->tag = 0
  │           │           rq->mq_hctx = hctx  // CPU에 매핑된 HW 큐
  │           │
  │           └── return rq
  │
  ├── blk_mq_bio_to_request(rq, bio, nr_segs)
  │     └── bio 정보를 request에 복사
  │         rq->__sector = bio->bi_iter.bi_sector
  │         rq->__data_len = 4096
  │
  └── [디스패치 경로 선택]                    // blk-mq.c:3813
        │
        ├── if (plug) → blk_add_rq_to_plug()  // 경로 A: plug에 적재
        │     (fio sync의 경우 plug이 있으면 이 경로)
        │
        ├── if (스케줄러 or busy) → blk_mq_insert_request()
        │     (스케줄러가 있으면 이 경로)       // 경로 B
        │
        └── else → blk_mq_try_issue_directly() // 경로 C: 직접 디스패치
              │                                 // NVMe + none 스케줄러 + idle
              └── __blk_mq_issue_directly()
                    │
                    └── hctx->ops->queue_rq(hctx, &bd)
                          │
                          └── ★ nvme_queue_rq(hctx, bd) ★
```

### 7.3 nvme_queue_rq() — SQ에 커맨드 기록 + Doorbell

`pci.c:1675`의 `nvme_queue_rq()`는 blk-mq와 NVMe 하드웨어 사이의 브릿지다:

```
nvme_queue_rq(hctx, bd)                      // pci.c:1675
  │
  ├── nvmeq = hctx->driver_data             // 이 CPU에 매핑된 NVMe 큐
  ├── req = bd->rq                           // 처리할 request
  ├── iod = blk_mq_rq_to_pdu(req)           // request에 내장된 nvme_iod
  │
  ├── [큐 상태 확인]
  │     ├── test_bit(NVMEQ_ENABLED, &nvmeq->flags)  // 큐 활성화?
  │     └── nvme_check_ready(&dev->ctrl, req, true)  // 컨트롤러 LIVE?
  │
  ├── nvme_prep_rq(req)                      // 요청 준비
  │     │
  │     ├── nvme_setup_cmd(ns, req, &iod->cmd)
  │     │     │
  │     │     └── NVMe Read 커맨드 빌드:
  │     │           iod->cmd.rw.opcode = nvme_cmd_read  // 0x02
  │     │           iod->cmd.rw.nsid = ns->head->ns_id  // 1
  │     │           iod->cmd.rw.slba = sector >> (ns->lba_shift - 9)
  │     │           iod->cmd.rw.length = (bytes >> ns->lba_shift) - 1
  │     │           iod->cmd.rw.command_id = req->tag    // 0
  │     │
  │     └── nvme_map_data(dev, req, &iod->cmd)
  │           │
  │           └── DMA 매핑 + PRP 설정:
  │                 ├── dma_map_sg(dev->dev, sg, nents, DMA_FROM_DEVICE)
  │                 │     → 물리 페이지 → DMA 주소 변환
  │                 │     → IOMMU가 있으면 IOVA 할당
  │                 │
  │                 └── PRP1/PRP2 설정:
  │                       iod->cmd.rw.dptr.prp1 = dma_addr  // 데이터 DMA 주소
  │                       iod->cmd.rw.dptr.prp2 = 0         // 4KB면 PRP2 불필요
  │
  │  ┌─────────────────────────────────────────────────────────┐
  │  │ ★ Critical Section: SQ에 커맨드 복사 + Doorbell 울림 ★   │
  │  └─────────────────────────────────────────────────────────┘
  │
  ├── spin_lock(&nvmeq->sq_lock)             // SQ 접근 보호
  │
  ├── nvme_sq_copy_cmd(nvmeq, &iod->cmd)
  │     │
  │     │  /* SQ의 tail 위치에 64바이트 NVMe 커맨드를 복사 */
  │     └── memcpy(nvmeq->sq_cmds + (nvmeq->sq_tail << nvmeq->sqes),
  │                &iod->cmd, sizeof(iod->cmd));
  │         nvmeq->sq_tail++;                // tail 전진
  │         if (sq_tail == q_depth) sq_tail = 0;  // wrap around
  │
  │         복사되는 64바이트 NVMe SQE:
  │         ┌────────────────────────────────────────┐
  │         │ Byte  0: opcode = 0x02 (Read)          │
  │         │ Byte  1: flags                         │
  │         │ Byte  2-3: command_id = 0              │
  │         │ Byte  4-7: nsid = 1                    │
  │         │ Byte  8-15: (reserved)                 │
  │         │ Byte 16-23: MPTR (metadata)            │
  │         │ Byte 24-31: PRP1 = dma_addr            │
  │         │ Byte 32-39: PRP2 = 0                   │
  │         │ Byte 40-47: SLBA (시작 LBA)            │
  │         │ Byte 48-49: NLB (블록 수 - 1) = 0      │
  │         │ Byte 50-63: (나머지 필드)               │
  │         └────────────────────────────────────────┘
  │
  ├── nvme_write_sq_db(nvmeq, bd->last)
  │     │
  │     │  /* bd->last=true이면 즉시 Doorbell 울림 */
  │     │  /* bd->last=false이면 지연 (배치 최적화) */
  │     │
  │     └── writel(nvmeq->sq_tail, nvmeq->q_db)
  │           │
  │           │  이 writel()이 실행되는 순간:
  │           │
  │           │  CPU → 가상주소(dev->bar + 0x1008)에 쓰기
  │           │      → MMU가 물리주소(0xFB001008)로 변환
  │           │      → UC(Uncacheable) 메모리이므로 캐시 우회
  │           │      → PCIe Root Complex가 Memory Write TLP 생성
  │           │      → PCIe 링크를 통해 NVMe 컨트롤러에 전달
  │           │      → 컨트롤러가 SQ Tail Doorbell 값 수신
  │           │      → "아, 새 커맨드가 있구나!" → SQ fetch 시작
  │           │
  │           ▼
  │    ═══ PCIe 링크를 넘어 디바이스 세계로 ═══
  │
  └── spin_unlock(&nvmeq->sq_lock)
```

### 7.4 NVMe 컨트롤러 내부 동작 (개념적)

```
NVMe 컨트롤러 (펌웨어/하드웨어):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

① Doorbell 수신
   SQ1 Tail = 1 수신 → "새 커맨드 0개→1개로 증가"

② SQ Fetch (DMA Read)
   PCIe Memory Read TLP → 호스트 메모리의 SQ[0] 에서 64바이트 읽기
   → NVMe Read 커맨드 파싱

③ 커맨드 실행
   ├── SLBA로 FTL(Flash Translation Layer) 조회
   │     → 논리 주소 → 물리 NAND 주소 변환
   ├── NAND Flash에서 4KB 데이터 읽기
   │     → 내부 버퍼에 저장
   └── ECC 체크 → 정상

④ 데이터 전송 (DMA Write)
   PCIe Memory Write TLP → PRP1 주소(호스트 메모리)에 4KB 기록
   → 호스트의 DMA 버퍼에 데이터 도착

⑤ CQE 기록 (DMA Write)
   PCIe Memory Write TLP → CQ[0]에 16바이트 CQE 기록
   ┌────────────────────────────────────────┐
   │ Byte 0-3: Command Specific (DW0)       │
   │ Byte 4-7: Reserved (DW1)               │
   │ Byte 8-9: SQ Head Pointer = 1          │
   │ Byte 10-11: SQ Identifier = 1          │
   │ Byte 12-13: Command ID = 0             │
   │ Byte 14-15: Status = 0x0001            │
   │             Phase bit = 1, Status = 0   │
   │             (성공!)                      │
   └────────────────────────────────────────┘

⑥ MSI-X 인터럽트 전송
   PCIe Memory Write TLP → MSI-X 테이블의 주소에 기록
   → CPU의 LAPIC에 인터럽트 벡터 전달
   → 해당 CPU에 인터럽트 발생
```

### 7.5 인터럽트 → 완료 처리 → 사용자 반환

```
CPU: 인터럽트 수신 (MSI-X 벡터)
  │
  └── 커널 IRQ 프레임워크
        │
        └── nvme_irq(irq, data)              // pci.c:1995
              │
              ├── nvmeq = data               // 인터럽트에 연결된 NVMe 큐
              ├── DEFINE_IO_COMP_BATCH(iob)   // 배치 완료 구조체
              │
              └── nvme_poll_cq(nvmeq, &iob)  // pci.c:1955
                    │
                    │  /* CQ에서 처리할 CQE가 있는지 Phase bit로 확인 */
                    ├── nvme_cqe_pending(nvmeq)
                    │     │
                    │     │  cqe = &nvmeq->cqes[nvmeq->cq_head]
                    │     │  return (le16_to_cpu(cqe->status) & 1) == nvmeq->cq_phase
                    │     │
                    │     │  Phase bit 설명:
                    │     │  ┌─────────────────────────────────────────┐
                    │     │  │ 초기 상태: cq_phase = 1                  │
                    │     │  │ CQE의 Phase bit = 1 → 유효한 새 CQE!     │
                    │     │  │ CQ가 한 바퀴 돌면 cq_phase = 0으로 토글  │
                    │     │  │ → 이전 라운드의 CQE(phase=1)와 구분      │
                    │     │  └─────────────────────────────────────────┘
                    │     └── true 반환 (새 CQE 있음!)
                    │
                    ├── dma_rmb()
                    │     └── DMA 읽기 메모리 배리어
                    │         Phase bit 확인 후 CQE 나머지 필드를 안전하게 읽기 위해
                    │
                    ├── nvme_handle_cqe(nvmeq, &iob, cq_head)  // pci.c:1883
                    │     │
                    │     ├── cqe = &nvmeq->cqes[idx]
                    │     ├── command_id = cqe->command_id     // 0
                    │     │
                    │     ├── req = nvme_find_rq(tagset, command_id)
                    │     │     └── tag 0으로 request 찾기
                    │     │         (sbitmap tag → request 매핑)
                    │     │
                    │     └── nvme_pci_complete_rq(req)
                    │           │
                    │           ├── nvme_unmap_data(dev, req)
                    │           │     └── DMA 매핑 해제
                    │           │
                    │           └── nvme_complete_rq(req)
                    │                 │
                    │                 └── blk_mq_end_request(req, status)
                    │                       │
                    │                       ├── blk_mq_free_request(req)
                    │                       │     └── tag 반환 (sbitmap 비트 해제)
                    │                       │
                    │                       └── bio_endio(bio)
                    │                             │
                    │                             └── bio->bi_end_io(bio)
                    │                                   │
                    │                                   └── blkdev_bio_end_io()
                    │                                         │
                    │                                         └── 대기 중인
                    │                                             태스크 깨움
                    │
                    ├── nvme_update_cq_head(nvmeq)  // pci.c:1924
                    │     ├── cq_head++
                    │     └── if (cq_head == q_depth) { cq_head=0; cq_phase^=1; }
                    │
                    └── nvme_ring_cq_doorbell(nvmeq)
                          └── writel(cq_head, nvmeq->q_db + nvmeq->dev->db_stride)
                                → 컨트롤러에게 "CQE를 처리했으니 이 슬롯 재사용 가능" 알림

  그리고...

  fio 프로세스가 깨어남
    └── read() 시스템 콜 리턴
          └── 4096바이트 데이터가 유저 버퍼에!
              ★ 첫 번째 I/O 완료! ★
```

---

## 8. 데이터 구조의 생성과 연결 타임라인

각 Phase에서 어떤 구조체가 생성되고 서로 어떻게 연결되는지 추적한다.

```
Phase 2: PCIe Enumeration
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ┌─────────────┐
  │  pci_dev     │ ← pci_alloc_dev()
  │  .vendor     │ = 0x144D
  │  .device     │ = 0xA808
  │  .class      │ = 0x010802
  │  .resource[0]│ = {0xFB000000, 0xFB00BFFF}
  │  .driver     │ = NULL ← 아직!
  └─────────────┘


Phase 3: Driver Matching
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ┌─────────────┐        ┌──────────────┐
  │  pci_dev     │───────→│  pci_driver   │
  │  .driver     │ = drv  │  .name="nvme" │
  └─────────────┘        │  .probe=      │
                          │   nvme_probe  │
                          └──────────────┘


Phase 4: NVMe Initialization
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ┌─────────────┐     ┌──────────────────────────────────┐
  │  pci_dev     │────→│  nvme_dev                         │
  └─────────────┘     │  .bar = ioremap(0xFB000000)       │
                       │  .dbs = bar + 0x1000              │
                       │  .q_depth = 1024                  │
                       │  .db_stride = 1                   │
                       │                                    │
                       │  .queues[0] (Admin Queue)          │
                       │    ├── .sq_cmds → DMA 버퍼          │
                       │    ├── .cqes → DMA 버퍼             │
                       │    ├── .q_db = &dbs[0]             │
                       │    └── .qid = 0                    │
                       │                                    │
                       │  .queues[1] (I/O Queue 1)          │
                       │    ├── .sq_cmds → DMA 버퍼          │
                       │    ├── .cqes → DMA 버퍼             │
                       │    ├── .q_db = &dbs[2*stride]      │
                       │    └── .qid = 1                    │
                       │                                    │
                       │  .ctrl ──→ nvme_ctrl               │
                       │             └── .tagset            │
                       │                                    │
                       │  .tagset ──→ blk_mq_tag_set        │
                       │               ├── .ops = nvme_mq_ops│
                       │               └── .nr_hw_queues     │
                       └──────────────────────────────────┘
                                         │
                                         ▼
                       ┌──────────────────────────────────┐
                       │  request_queue                     │
                       │    └── .tag_set = &dev->tagset     │
                       └──────────────────────────────────┘
                                         │
                                         ▼
                       ┌──────────────────────────────────┐
                       │  gendisk                           │
                       │    .disk_name = "nvme0n1"          │
                       │    .queue = request_queue           │
                       │    → /dev/nvme0n1                   │
                       └──────────────────────────────────┘


Phase 5: I/O 처리 중 생성되는 구조체
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ┌─────────┐      ┌────────────────┐      ┌──────────────┐
  │  bio     │─────→│  request       │─────→│  nvme_iod     │
  │ .bi_bdev │      │  .tag = 0      │      │ .cmd (64B)    │
  │ .bi_sector│     │  .mq_hctx      │      │   .opcode     │
  │ .bi_size │      │  → nvme_queue  │      │   .command_id │
  └─────────┘      └────────────────┘      │   .slba       │
                                            │   .prp1       │
                                            └──────────────┘
                                                   │
                                                   ▼ (memcpy)
                                            ┌──────────────┐
                                            │ SQ[tail]      │
                                            │ 64바이트 SQE   │
                                            │ (DMA 메모리)   │
                                            └──────────────┘
```

---

## 9. 주소 공간의 변화 타임라인

BAR0 물리 주소가 결정되고, 매핑되고, 사용되기까지의 전체 여정:

```
Phase 0 (BIOS/UEFI):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  BIOS가 BAR sizing으로 NVMe SSD에 필요한 메모리 크기를 알아내고
  물리 주소를 할당한다:
    BAR0 = 0xFB000000 (48KB 영역)
    Config Space에 기록 → 디바이스가 이 주소에 레지스터를 노출


Phase 2 (PCIe Enumeration):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  pci_setup_device() → pci_read_bases()에서 BAR0 값을 읽는다:
    pci_dev->resource[0].start = 0xFB000000
    pci_dev->resource[0].end   = 0xFB00BFFF

  이 시점에서는 물리 주소만 알고 있다.
  커널 코드에서 직접 접근할 수는 없다 (가상 주소 매핑 필요).


Phase 4-Step 3 (nvme_dev_map):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  nvme_dev_map() → ioremap(0xFB000000, size)
    → 커널 가상 주소 할당: dev->bar = 0xFFFF_8880_FB00_0000 (예시)
    → 페이지 테이블에 UC(Uncacheable) 매핑 생성:
      가상 주소 0xFFFF_8880_FB00_0000 → 물리 주소 0xFB000000
      캐시 비활성화 (매 접근마다 PCIe 트랜잭션 발생)

  ┌─────────────────────────────────────────────────────────┐
  │ 가상 주소                     물리 주소                   │
  │ dev->bar                      0xFB000000                │
  │ dev->bar + 0x00 (CAP)     →   0xFB000000                │
  │ dev->bar + 0x14 (CC)      →   0xFB000014                │
  │ dev->bar + 0x1C (CSTS)    →   0xFB00001C                │
  │ dev->bar + 0x24 (AQA)     →   0xFB000024                │
  │ dev->bar + 0x28 (ASQ)     →   0xFB000028                │
  │ dev->bar + 0x30 (ACQ)     →   0xFB000030                │
  └─────────────────────────────────────────────────────────┘


Phase 4-Step 5 (nvme_pci_enable):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  dev->dbs = dev->bar + 4096   (Doorbell 시작)

  ┌─────────────────────────────────────────────────────────┐
  │ dev->dbs + 0x00 → SQ0 Tail DB  (Admin)  → 0xFB001000   │
  │ dev->dbs + 0x04 → CQ0 Head DB  (Admin)  → 0xFB001004   │
  │ dev->dbs + 0x08 → SQ1 Tail DB  (I/O 1)  → 0xFB001008   │
  │ dev->dbs + 0x0C → CQ1 Head DB  (I/O 1)  → 0xFB00100C   │
  │ dev->dbs + 0x10 → SQ2 Tail DB  (I/O 2)  → 0xFB001010   │
  │ dev->dbs + 0x14 → CQ2 Head DB  (I/O 2)  → 0xFB001014   │
  │ ...                                                      │
  └─────────────────────────────────────────────────────────┘


Phase 4-Step 10 (nvme_create_io_queues):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  각 큐에 Doorbell 주소를 연결:
    nvmeq->q_db = &dev->dbs[qid * 2 * db_stride]

  예 (db_stride=1):
    queues[0].q_db = dev->dbs + 0  = bar + 0x1000  (Admin SQ Tail)
    queues[1].q_db = dev->dbs + 2  = bar + 0x1008  (I/O Q1 SQ Tail)
    queues[2].q_db = dev->dbs + 4  = bar + 0x1010  (I/O Q2 SQ Tail)

  CQ Head Doorbell은 q_db + db_stride 위치:
    queues[1] CQ Head DB = bar + 0x100C


Phase 5 (첫 I/O):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  writel(sq_tail, nvmeq->q_db)

  CPU 코드:
    writel(1, 0xFFFF_8880_FB00_1008)

  하드웨어 수준:
    → MMU: 가상 → 물리 변환 (UC 영역)
    → CPU가 물리 주소 0xFB001008에 4바이트 쓰기 시작
    → Root Complex가 PCIe Memory Write TLP 생성:
        ┌─────────────────────────────────┐
        │ TLP Header:                      │
        │   Type: MWr (Memory Write)       │
        │   Address: 0xFB001008            │
        │   Length: 1 DW (4바이트)          │
        │ Data: 0x00000001 (sq_tail=1)     │
        └─────────────────────────────────┘
    → PCIe 링크를 통해 NVMe 컨트롤러에 도달
    → 컨트롤러: "SQ1의 새 tail = 1, 처리할 커맨드 있음!"
```

---

## 10. SPDK와의 비교: 같은 목적, 다른 경로

커널 NVMe 드라이버와 SPDK는 동일한 NVMe 하드웨어를 제어하지만 경로가 완전히 다르다:

```
                커널 NVMe 드라이버                    SPDK
                ━━━━━━━━━━━━━━                    ━━━━
Phase 0-2       BIOS + 커널 PCI 서브시스템           BIOS (동일)
Phase 3         module_init + pci_register_driver    spdk_nvme_probe()
BAR 매핑         ioremap() (커널 공간)               mmap(/dev/uio0) (유저 공간)
DMA 할당         dma_alloc_coherent()                spdk_dma_zmalloc() (hugepage)
큐 생성          Admin 커맨드 (커널에서)               Admin 커맨드 (유저에서)
I/O 제출         blk-mq → nvme_queue_rq()           spdk_nvme_ns_cmd_read()
                 → 컨텍스트 스위칭 필요               → 직접 SQ에 쓰기
Doorbell         writel() (커널 MMIO)                *(volatile u32*)mmio_addr
완료 처리        인터럽트 → nvme_irq()               폴링 → spdk_nvme_qpair_
                                                     process_completions()
스케줄링         blk-mq + I/O 스케줄러               애플리케이션이 직접 관리
보호             커널 주소 공간 격리                   없음 (유저 공간)
```

핵심 차이:
- **커널 드라이버**: syscall → VFS → block layer → NVMe → 인터럽트 (수 μs 오버헤드)
- **SPDK**: 유저 공간에서 직접 SQ 쓰기 → Doorbell → 폴링 (수백 ns 오버헤드)
- SPDK가 빠른 이유: syscall 없음, 인터럽트 없음, 컨텍스트 스위칭 없음, 락 최소화

---

## 11. 디버깅과 관찰 가이드

### 11.1 각 Phase를 관찰하는 방법

```
Phase 2 (PCIe Enumeration):
  $ dmesg | grep "pci 0000"
  $ lspci -vvv -s 00:02.0
  $ cat /sys/bus/pci/devices/0000:02:00.0/resource

Phase 3 (Driver Matching):
  $ ls -la /sys/bus/pci/devices/0000:02:00.0/driver
  $ dmesg | grep nvme

Phase 4 (NVMe Init):
  $ dmesg | grep "nvme nvme0"
  → "pci function 0000:02:00.0"
  → "4/0/0 default/read/poll queues"
  $ cat /sys/class/nvme/nvme0/transport
  $ nvme list

Phase 5 (I/O):
  $ sudo bpftrace -e 'kprobe:nvme_queue_rq { printf("nvme_queue_rq called\n"); }'
  $ sudo perf trace -e 'nvme:*' -- dd if=/dev/nvme0n1 of=/dev/null bs=4k count=1
```

### 11.2 ftrace로 첫 I/O 추적하기

```bash
# 함수 추적 설정
echo function_graph > /sys/kernel/debug/tracing/current_tracer
echo blk_mq_submit_bio nvme_queue_rq nvme_irq > \
  /sys/kernel/debug/tracing/set_graph_function
echo 1 > /sys/kernel/debug/tracing/tracing_on

# I/O 실행
dd if=/dev/nvme0n1 of=/dev/null bs=4k count=1

# 결과 확인
cat /sys/kernel/debug/tracing/trace
```

예상 출력:
```
  dd-1234  [003]  blk_mq_submit_bio() {
  dd-1234  [003]    blk_mq_get_new_requests();
  dd-1234  [003]    blk_mq_bio_to_request();
  dd-1234  [003]    blk_mq_try_issue_directly() {
  dd-1234  [003]      nvme_queue_rq() {
  dd-1234  [003]        nvme_setup_cmd();
  dd-1234  [003]        nvme_map_data();
  dd-1234  [003]        nvme_sq_copy_cmd();
  dd-1234  [003]        nvme_write_sq_db();
  dd-1234  [003]      } /* nvme_queue_rq */
  dd-1234  [003]    } /* blk_mq_try_issue_directly */
  dd-1234  [003]  } /* blk_mq_submit_bio */
  ...
  <idle>-0  [003]  nvme_irq() {
  <idle>-0  [003]    nvme_poll_cq() {
  <idle>-0  [003]      nvme_handle_cqe() {
  <idle>-0  [003]        nvme_pci_complete_rq();
  <idle>-0  [003]      }
  <idle>-0  [003]    }
  <idle>-0  [003]  }
```

---

## 12. 블로그 포스트 구성 제안

이 분석 자료를 블로그 시리즈로 발행하기 위한 구성안:

### Part 1: PCIe 기초와 Config Space
- **대상 독자**: 하드웨어 인터페이스를 처음 접하는 개발자
- **핵심 내용**: Config Space의 구조, BAR의 의미, ECAM 매핑
- **참고 문서**: `09_pcie_config_space_and_ecam.md`, `05_pcie_bar_mmio_connection.md`
- **코드 위치**: `drivers/pci/probe.c` (pci_setup_device, pci_read_bases)
- **실습**: `lspci -vvv`로 자신의 NVMe SSD Config Space 읽어보기

### Part 2: 커널 부트와 PCIe Enumeration
- **대상 독자**: 커널 부팅 과정에 관심 있는 개발자
- **핵심 내용**: start_kernel → ACPI → PCI 스캔 → 디바이스 등록
- **참고 문서**: 이 문서(12)의 Phase 1~2
- **코드 위치**: `drivers/pci/probe.c` (pci_scan_child_bus, pci_scan_slot, pci_scan_device)
- **실습**: `dmesg | grep pci`로 부팅 시 PCIe 열거 로그 확인

### Part 3: NVMe 드라이버 초기화 — probe()에서 /dev/nvme0n1까지
- **대상 독자**: 디바이스 드라이버 개발에 관심 있는 개발자
- **핵심 내용**: driver-device 매칭, nvme_probe(), Admin Queue, I/O Queue
- **참고 문서**: `02_nvme_driver_internals.md`, 이 문서(12)의 Phase 3~4
- **코드 위치**: `drivers/nvme/host/pci.c` (nvme_probe, nvme_pci_enable, nvme_pci_configure_admin_queue)
- **실습**: QEMU + GDB로 nvme_probe()에 브레이크포인트 걸기

### Part 4: I/O 경로 — Application → Doorbell → Completion
- **대상 독자**: 스토리지 성능에 관심 있는 개발자
- **핵심 내용**: read() → blk-mq → nvme_queue_rq → 인터럽트 → 완료
- **참고 문서**: `00_overview_and_full_path.md`, `01_block_layer_blk_mq.md`, `03_fio_and_io_interfaces.md`, `04_pcie_doorbell_and_completion.md`
- **코드 위치**: `block/blk-mq.c` (blk_mq_submit_bio), `drivers/nvme/host/pci.c` (nvme_queue_rq, nvme_irq)
- **실습**: fio로 I/O 실행하며 bpftrace/perf로 함수 호출 추적

### Part 5: QEMU로 보는 디바이스 측 동작
- **대상 독자**: "디바이스가 뭘 하는 건지" 궁금한 개발자
- **핵심 내용**: QEMU NVMe 에뮬레이션으로 디바이스 측 로직 이해
- **참고 문서**: `06_qemu_nvme_device_emulation.md`, `07_end_to_end_app_to_device.md`, `08_qemu_pcie_nvme_connection.md`
- **실습**: QEMU 빌드 + NVMe 디바이스 연결 + GDB로 디바이스 코드 디버깅

---

## 13. 전체 문서 인덱스

현재까지 작성된 분석 문서 목록과 각 문서가 다루는 Phase 매핑:

| # | 파일명 | 제목 | 관련 Phase |
|---|--------|------|-----------|
| 00 | `00_overview_and_full_path.md` | NVMe I/O 경로 전체 분석: fio에서 SSD Doorbell까지 | Phase 5 (전체 I/O 경로) |
| 01 | `01_block_layer_blk_mq.md` | Block Layer (blk-mq) 심층 분석 | Phase 5 (blk-mq 계층) |
| 02 | `02_nvme_driver_internals.md` | NVMe PCIe 드라이버 내부 구조 완전 분석 | Phase 3~4 (드라이버 구조) |
| 03 | `03_fio_and_io_interfaces.md` | fio I/O 엔진과 커널 I/O 인터페이스 심층 분석 | Phase 5 (유저→커널 진입) |
| 04 | `04_pcie_doorbell_and_completion.md` | NVMe PCIe Doorbell과 Completion 메커니즘 | Phase 5 (Doorbell/인터럽트) |
| 05 | `05_pcie_bar_mmio_connection.md` | PCIe BAR와 MMIO: 레지스터가 커널 코드와 연결되는 원리 | Phase 0~4 (BAR/MMIO) |
| 06 | `06_qemu_nvme_device_emulation.md` | QEMU NVMe 디바이스 에뮬레이션 심층 분석 | Phase 5 (디바이스 측) |
| 07 | `07_end_to_end_app_to_device.md` | End-to-End NVMe I/O 완전 추적 | Phase 5 (양방향 추적) |
| 08 | `08_qemu_pcie_nvme_connection.md` | QEMU PCIe ↔ NVMe 에뮬레이션 연결점 | Phase 2~4 (에뮬레이션) |
| 09 | `09_pcie_config_space_and_ecam.md` | PCIe Config Space와 ECAM | Phase 0~2 (Config Space) |
| 12 | `12_e2e_boot_to_first_io.md` | **전원 ON에서 첫 번째 NVMe I/O까지 (이 문서)** | **Phase 0~5 전체** |

### 문서 간 관계도

```
                        ┌──────────────────────┐
                        │  12. Boot to First IO │ ← ★ 이 문서 (Capstone)
                        │  (전체 타임라인)        │
                        └──────────┬───────────┘
                                   │
            ┌──────────────────────┼──────────────────────┐
            │                      │                      │
            ▼                      ▼                      ▼
  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
  │  Phase 0~2       │  │  Phase 3~4       │  │  Phase 5         │
  │  HW/PCIe 기반    │  │  드라이버 초기화  │  │  I/O 경로         │
  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘
           │                    │                     │
     ┌─────┴─────┐        ┌────┴────┐         ┌──────┼──────┐
     ▼           ▼        ▼         ▼         ▼      ▼      ▼
  ┌─────┐   ┌─────┐   ┌─────┐  ┌─────┐   ┌─────┐┌─────┐┌─────┐
  │ 09  │   │ 05  │   │ 02  │  │ 08  │   │ 00  ││ 01  ││ 03  │
  │ECAM │   │BAR/ │   │NVMe │  │QEMU │   │전체 ││blk- ││fio/ │
  │     │   │MMIO │   │내부 │  │PCIe │   │경로 ││mq   ││I/O  │
  └─────┘   └─────┘   └─────┘  └─────┘   └─────┘└─────┘└─────┘
                                                     │
                                               ┌─────┼─────┐
                                               ▼     ▼     ▼
                                            ┌─────┐┌─────┐┌─────┐
                                            │ 04  ││ 06  ││ 07  │
                                            │Door-││QEMU ││E2E  │
                                            │bell ││NVMe ││추적 │
                                            └─────┘└─────┘└─────┘
```

---

## 14. 핵심 소스코드 위치 요약

이 문서에서 다룬 모든 핵심 함수의 소스 위치:

### PCIe 서브시스템 (`drivers/pci/`)
| 함수 | 파일:라인 | Phase | 역할 |
|------|----------|-------|------|
| `pci_scan_child_bus()` | probe.c:3237 | 2 | 버스의 모든 슬롯 스캔 |
| `pci_scan_slot()` | probe.c:2898 | 2 | 슬롯의 모든 Function 스캔 |
| `pci_scan_single_device()` | probe.c:2810 | 2 | 단일 디바이스 스캔 |
| `pci_scan_device()` | probe.c:2620 | 2 | Vendor ID 읽기 + pci_dev 생성 |
| `pci_setup_device()` | probe.c:2018 | 2 | Config Space 파싱 (BAR, class) |
| `pci_init_capabilities()` | probe.c:2682 | 2 | MSI-X, PM, SR-IOV 등 초기화 |
| `pci_device_probe()` | pci-driver.c:435 | 3 | 드라이버→디바이스 probe 호출 |
| `__pci_device_probe()` | pci-driver.c:407 | 3 | ID 매칭 + probe 콜백 호출 |

### NVMe 드라이버 (`drivers/nvme/host/pci.c`)
| 함수 | 라인 | Phase | 역할 |
|------|------|-------|------|
| `nvme_init()` | 4987 | 3 | 모듈 초기화, pci_register_driver |
| `nvme_probe()` | 4368 | 4 | 디바이스 probe 진입점 |
| `nvme_dev_map()` | 4175 | 4 | BAR0 ioremap |
| `nvme_pci_enable()` | 3750 | 4 | PCI 활성화 + CAP 읽기 |
| `nvme_pci_configure_admin_queue()` | 2851 | 4 | Admin Queue 설정 + CC.EN=1 |
| `nvme_setup_io_queues()` | 3502 | 4 | I/O 큐 전체 설정 |
| `nvme_queue_rq()` | 1675 | 5 | blk-mq I/O 제출 콜백 (핵심!) |
| `nvme_irq()` | 1995 | 5 | 인터럽트 핸들러 |
| `nvme_poll_cq()` | 1955 | 5 | CQ 순회 + CQE 처리 |
| `nvme_handle_cqe()` | 1883 | 5 | 개별 CQE 처리 + 요청 완료 |
| `nvme_reset_work()` | 3981 | (리셋) | 컨트롤러 리셋 워크 함수 |

### Block Layer (`block/blk-mq.c`)
| 함수 | 라인 | Phase | 역할 |
|------|------|-------|------|
| `blk_mq_submit_bio()` | 3697 | 5 | bio → request 변환 + 디스패치 |

---

## 15. 마무리: "한 줄 요약"으로 보는 전체 흐름

```
전원 ON
  → BIOS가 PCIe 버스를 스캔하고 NVMe SSD의 BAR0에 물리 주소를 할당한다
  → 커널이 ACPI 테이블을 파싱하여 ECAM 매핑을 설정한다
  → 커널이 PCIe 버스를 다시 스캔하여 pci_dev 구조체를 생성한다
  → NVMe 드라이버가 class code 0x010802로 매칭되어 nvme_probe()가 호출된다
  → BAR0를 ioremap하고, Admin Queue를 설정하고, CC.EN=1로 컨트롤러를 켠다
  → Identify Controller → I/O Queue 생성 → /dev/nvme0n1 블록 디바이스 등록
  → fio가 read() → blk_mq_submit_bio() → nvme_queue_rq() → SQ에 커맨드 + Doorbell
  → 컨트롤러가 NAND에서 데이터를 읽어 DMA로 호스트에 전달
  → MSI-X 인터럽트 → nvme_irq() → CQE 처리 → bio_endio() → read() 리턴
첫 번째 4KB 데이터가 유저 버퍼에 도착!
```

이 모든 과정이 전원 버튼을 누른 후 수 초 안에 준비되고, 첫 I/O는 수십 마이크로초 만에 완료된다. 수천 줄의 커널 코드와 수십 개의 하드웨어 메커니즘이 하나의 4KB 데이터 읽기를 위해 협력하는 것이다.
