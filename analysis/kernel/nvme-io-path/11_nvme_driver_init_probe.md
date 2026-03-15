# NVMe 드라이버 초기화 및 Probe 시퀀스 완전 분석

> 소스: Linux Kernel `drivers/nvme/host/pci.c`, `core.c`, `nvme.h`, `drivers/pci/pci-driver.c`

## 전체 초기화 흐름 개요

```
 ┌─────────────────────────────────────────────────────────────────┐
 │                    모듈 로드 (insmod nvme.ko)                    │
 └──────────────────────────┬──────────────────────────────────────┘
                            │
                            ▼
                    nvme_init()
                            │
                    pci_register_driver(&nvme_driver)
                            │
                    __pci_register_driver()
                            │
                    driver_register()
                            │
                    bus_add_driver() → driver_attach()
                            │
              ┌─────────────┴─────────────┐
              │  각 PCI 디바이스에 대해:    │
              │  pci_bus_match()           │
              │  → class code 0x010802?   │
              └─────────────┬─────────────┘
                            │ YES
                            ▼
                    pci_device_probe()
                            │
                    __pci_device_probe()
                            │
                    pci_call_probe()
                            │
                    nvme_probe()  ◀── 여기서 모든 것이 시작
                            │
         ┌──────────────────┼──────────────────┐
         ▼                  ▼                  ▼
  nvme_pci_alloc_dev  nvme_dev_map    nvme_pci_enable
  (nvme_dev 할당)     (BAR0 ioremap)  (PCI 활성화+Admin Q)
                                               │
                            ┌──────────────────┘
                            ▼
                nvme_pci_configure_admin_queue
                            │
                  ┌─────────┼──────────┐
                  ▼         ▼          ▼
            CC.EN=0    DMA alloc    CC.EN=1
           (disable)   (SQ/CQ)    (enable)
                                       │
                            ┌──────────┘
                            ▼
                   nvme_setup_io_queues
                            │
              ┌─────────────┼──────────────┐
              ▼             ▼              ▼
        Set Num Queues  Create CQ/SQ   IRQ 등록
        (Admin cmd)     (Admin cmd)
                                           │
                            ┌──────────────┘
                            ▼
                  nvme_alloc_io_tag_set
                            │
                  nvme_start_ctrl
                            │
                  nvme_scan_work
                            │
                  nvme_alloc_ns
                            │
                  device_add_disk
                            │
                            ▼
                    /dev/nvme0n1 탄생!
```

---

## Phase 3: PCI 드라이버 등록과 NVMe 매칭

### 3.1 struct pci_driver nvme_driver

```c
// drivers/nvme/host/pci.c:4964

static struct pci_driver nvme_driver = {
    .name       = "nvme",             // 드라이버 이름. /sys/bus/pci/drivers/nvme에 노출
    .id_table   = nvme_id_table,      // 지원하는 PCI 디바이스 ID 목록
    .probe      = nvme_probe,         // 디바이스 발견 시 호출되는 초기화 함수
    .remove     = nvme_remove,        // 디바이스 제거 시 호출되는 정리 함수
    .shutdown   = nvme_shutdown,      // 시스템 셧다운/재부팅 시 호출
    .driver     = {
        .probe_type = PROBE_PREFER_ASYNCHRONOUS,  // 여러 NVMe를 병렬 probe
#ifdef CONFIG_PM_SLEEP
        .pm     = &nvme_dev_pm_ops,               // suspend/resume 콜백
#endif
    },
    .sriov_configure = pci_sriov_configure_simple, // SR-IOV VF 설정
    .err_handler     = &nvme_err_handler,          // PCIe AER 에러 복구 핸들러
};
```

**핵심 포인트:**
- `PROBE_PREFER_ASYNCHRONOUS`: 여러 NVMe SSD가 있으면 각각의 probe를 비동기로 수행하여 부팅 속도를 향상시킨다.
- `err_handler`: PCIe Advanced Error Reporting(AER) 에러 발생 시 복구 절차를 정의한다.

### 3.2 nvme_id_table: PCI 디바이스 매칭 테이블

```c
// drivers/nvme/host/pci.c:4762

static const struct pci_device_id nvme_id_table[] = {
    // --- 벤더별 특수 항목 (quirks 포함) ---
    { PCI_VDEVICE(INTEL, 0x0953),       // Intel 750/P3500/P3600/P3700
        .driver_data = NVME_QUIRK_STRIPE_SIZE |
                       NVME_QUIRK_DEALLOCATE_ZEROES, },
    { PCI_VDEVICE(INTEL, 0x0a53),       // Intel P3520
        .driver_data = NVME_QUIRK_STRIPE_SIZE |
                       NVME_QUIRK_DEALLOCATE_ZEROES, },
    { PCI_VDEVICE(INTEL, 0x5845),       // QEMU 에뮬레이션 NVMe
        .driver_data = NVME_QUIRK_IDENTIFY_CNS |
                       NVME_QUIRK_DISABLE_WRITE_ZEROES |
                       NVME_QUIRK_BOGUS_NID, },
    // ... 삼성, Apple 등 벤더별 quirk 항목들 ...

    // --- 핵심: 모든 NVMe 디바이스를 잡는 와일드카드 항목 ---
    { PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_EXPRESS, 0xffffff) },
    //                  ^^^^^^^^^^^^^^^^^^^^^^^^^
    //                  PCI Class Code = 01:08:02
    //                  01 = Mass Storage Controller
    //                  08 = Non-Volatile Memory Controller
    //                  02 = NVM Express
    //
    // 0xffffff = class_mask (3바이트 전체 비교)
    // → PCI class가 정확히 0x010802인 모든 디바이스가 매칭

    { 0, }  // 종료 마커
};
MODULE_DEVICE_TABLE(pci, nvme_id_table);
```

**매칭 로직:**
1. PCI 서브시스템이 디바이스를 스캔하면 `id_table`을 순서대로 검색한다.
2. 먼저 벤더별 항목(VID:DID)과 비교하여 quirks를 적용한다.
3. 벤더별 항목에 없으면 마지막 `PCI_CLASS_STORAGE_EXPRESS` 항목에서 class code만으로 매칭한다.
4. 따라서 모든 NVMe 디바이스(class=01:08:02)가 이 드라이버에 바인딩된다.

### 3.3 module_init(nvme_init): 드라이버 등록

```c
// drivers/nvme/host/pci.c:4987

static int __init nvme_init(void)
{
    // 컴파일 타임 구조체 크기 검증
    BUILD_BUG_ON(sizeof(struct nvme_create_cq) != 64);
        // Create CQ 커맨드가 정확히 64바이트인지 확인
        // NVMe 스펙: SQ Entry Size = 64 바이트 (필수)
    BUILD_BUG_ON(sizeof(struct nvme_create_sq) != 64);
    BUILD_BUG_ON(sizeof(struct nvme_delete_queue) != 64);
    BUILD_BUG_ON(IRQ_AFFINITY_MAX_SETS < 2);
        // 읽기/쓰기 큐에 별도의 IRQ affinity 세트를 할당하기 위해 최소 2셋 필요

    // PCI 서브시스템에 NVMe 드라이버를 등록한다
    return pci_register_driver(&nvme_driver);
        // 이 호출이 반환된 후부터 NVMe 디바이스 발견 시 nvme_probe()가 호출된다
}

module_init(nvme_init);   // insmod 또는 커널 부팅 시 nvme_init() 실행
module_exit(nvme_exit);   // rmmod 시 nvme_exit() 실행
```

### 3.4 pci_register_driver → driver_register 체인

```c
// drivers/pci/pci-driver.c:1433

int __pci_register_driver(struct pci_driver *drv, struct module *owner,
                          const char *mod_name)
{
    // pci_driver의 내부 driver 필드를 초기화한다
    drv->driver.name = drv->name;       // "nvme"
    drv->driver.bus = &pci_bus_type;    // PCI 버스에 등록
    drv->driver.owner = owner;          // THIS_MODULE
    drv->driver.mod_name = mod_name;    // "nvme"
    drv->driver.groups = drv->groups;
    drv->driver.dev_groups = drv->dev_groups;

    // 동적 ID 리스트 초기화 (런타임에 new_id를 추가할 수 있음)
    spin_lock_init(&drv->dynids.lock);
    INIT_LIST_HEAD(&drv->dynids.list);

    // 커널 디바이스 모델에 드라이버를 등록
    return driver_register(&drv->driver);
    // → bus_add_driver()
    //   → driver_attach()
    //     → bus_for_each_dev()  ← 현재 시스템의 모든 PCI 디바이스를 순회
    //       → __driver_attach()
    //         → driver_match_device() → pci_bus_match()  ← 매칭 확인
    //         → driver_probe_device() → pci_device_probe()  ← probe 호출
}
```

### 3.5 pci_bus_match: NVMe 디바이스 매칭

```c
// drivers/pci/pci-driver.c:1504

static int pci_bus_match(struct device *dev, const struct device_driver *drv)
{
    struct pci_dev *pci_dev = to_pci_dev(dev);     // generic device → pci_dev
    struct pci_driver *pci_drv;
    const struct pci_device_id *found_id;

    // VFIO 등으로 binding이 금지된 디바이스는 매칭하지 않음
    if (pci_dev_binding_disallowed(pci_dev))
        return 0;

    pci_drv = (struct pci_driver *)to_pci_driver(drv);

    // nvme_id_table에서 이 디바이스와 매칭되는 항목을 찾는다
    found_id = pci_match_device(pci_drv, pci_dev);
    //          ^^^^^^^^^^^^^^^^
    // VID:DID 비교 → class code 비교 → dynamic ID 비교 순서
    // NVMe의 경우 class=0x010802이면 nvme_id_table의 마지막 항목과 매칭

    if (found_id)
        return 1;    // 매칭 성공 → probe 호출 대상

    return 0;
}
```

### 3.6 pci_device_probe → nvme_probe() 호출

```c
// drivers/pci/pci-driver.c:435

static int pci_device_probe(struct device *dev)
{
    int error;
    struct pci_dev *pci_dev = to_pci_dev(dev);
    struct pci_driver *drv = to_pci_driver(dev->driver);

    // SR-IOV VF의 자동 probe 확인
    if (!pci_device_can_probe(pci_dev))
        return -ENODEV;

    // 디바이스에 INTx IRQ 번호 할당
    pci_assign_irq(pci_dev);

    // 아키텍처별 IRQ 리소스 할당
    error = pcibios_alloc_irq(pci_dev);
    if (error < 0)
        return error;

    // 참조 카운트 증가 (probe 중 디바이스가 사라지지 않도록)
    pci_dev_get(pci_dev);

    // __pci_device_probe()에서 실제 드라이버의 probe 호출
    error = __pci_device_probe(drv, pci_dev);
    if (error) {
        pcibios_free_irq(pci_dev);
        pci_dev_put(pci_dev);
    }

    return error;
}

// drivers/pci/pci-driver.c:407
static int __pci_device_probe(struct pci_driver *drv, struct pci_dev *pci_dev)
{
    const struct pci_device_id *id;
    int error = 0;

    if (drv->probe) {             // nvme_driver.probe = nvme_probe
        error = -ENODEV;

        // id_table에서 매칭 항목을 찾는다
        id = pci_match_device(drv, pci_dev);
        if (id)
            // ★★★ nvme_probe(pci_dev, id) 호출! ★★★
            error = pci_call_probe(drv, pci_dev, id);
            // pci_call_probe()는 NUMA 최적화를 수행한 후
            // drv->probe(pci_dev, id)를 호출한다
    }
    return error;
}
```

**pci_bus_match → pci_device_probe 호출 체인 정리:**

```
driver_attach()
  └→ bus_for_each_dev(pci_bus_type, ..., __driver_attach)
       └→ __driver_attach(dev, drv)
            ├→ driver_match_device(drv, dev)
            │    └→ pci_bus_type.match = pci_bus_match()
            │         └→ pci_match_device(nvme_driver, pci_dev)
            │              └→ class 0x010802 매칭 → return 1
            └→ driver_probe_device(drv, dev)
                 └→ really_probe(dev, drv)
                      └→ pci_bus_type.probe = pci_device_probe()
                           └→ __pci_device_probe()
                                └→ pci_call_probe()
                                     └→ nvme_probe(pdev, id)  ★
```

---

## Phase 4: nvme_probe() 완전 분석

### 4.1 nvme_probe() 전체 흐름

```c
// drivers/nvme/host/pci.c:4368

static int nvme_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct nvme_dev *dev;
    int result = -ENOMEM;
```

#### 1단계: nvme_dev 할당 및 기본 초기화

```c
    dev = nvme_pci_alloc_dev(pdev, id);
    if (IS_ERR(dev))
        return PTR_ERR(dev);
```

`nvme_pci_alloc_dev()` 내부:

```c
// drivers/nvme/host/pci.c:4273

static struct nvme_dev *nvme_pci_alloc_dev(struct pci_dev *pdev,
        const struct pci_device_id *id)
{
    unsigned long quirks = id->driver_data;
        // id_table에서 매칭된 항목의 quirks를 가져온다
        // 와일드카드 매칭(PCI_CLASS_STORAGE_EXPRESS)이면 quirks=0
    int node = dev_to_node(&pdev->dev);
        // NUMA 노드 번호. 이 디바이스가 연결된 CPU 소켓
    struct nvme_dev *dev;
    int ret = -ENOMEM;

    // nvme_dev 할당. NUMA 노드를 지정하여 로컬 메모리에 할당
    // struct_size()로 flexible array member(descriptor_pools)를 포함한 크기 계산
    dev = kzalloc_node(struct_size(dev, descriptor_pools, nr_node_ids),
            GFP_KERNEL, node);
    if (!dev)
        return ERR_PTR(-ENOMEM);

    // 리셋 워크 함수를 nvme_reset_work로 설정
    INIT_WORK(&dev->ctrl.reset_work, nvme_reset_work);
    mutex_init(&dev->shutdown_lock);

    // 모듈 파라미터에서 큐 설정 복사
    dev->nr_write_queues = write_queues;    // 쓰기 전용 큐 수 (기본 0)
    dev->nr_poll_queues = poll_queues;      // 폴링 큐 수 (기본 0)
    dev->nr_allocated_queues = nvme_max_io_queues(dev) + 1;
        // CPU 수 + write_queues + poll_queues + 1(Admin)

    // 큐 배열 할당: queues[0]=Admin, queues[1..N]=I/O
    dev->queues = kcalloc_node(dev->nr_allocated_queues,
            sizeof(struct nvme_queue), GFP_KERNEL, node);
    if (!dev->queues)
        goto out_free_dev;

    dev->dev = get_device(&pdev->dev);   // PCI 디바이스 참조

    // 벤더/보드 조합별 추가 quirk 체크
    quirks |= check_vendor_combination_bug(pdev);
        // 삼성 SSD + Dell 노트북 등 특정 조합에서 발생하는 버그 우회

    // nvme_ctrl 구조체 초기화 (core.c에 정의된 공통 컨트롤러)
    ret = nvme_init_ctrl(&dev->ctrl, &pdev->dev, &nvme_pci_ctrl_ops, quirks);
    if (ret)
        goto out_put_device;

    // DMA 주소 비트 폭 설정
    if (dev->ctrl.quirks & NVME_QUIRK_DMA_ADDRESS_BITS_48)
        dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(48));
    else
        dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
        // 대부분의 NVMe는 64비트 DMA 주소 사용 가능

    dma_set_min_align_mask(&pdev->dev, NVME_CTRL_PAGE_SIZE - 1);
        // PRP 정렬 요구사항: 4KB 페이지 경계
    dma_set_max_seg_size(&pdev->dev, 0xffffffff);
        // 단일 세그먼트 최대 크기 = 4GB

    // 최대 전송 크기 설정
    dev->ctrl.max_hw_sectors = min_t(u32,
            NVME_MAX_BYTES >> SECTOR_SHIFT,     // 8MB / 512 = 16384섹터
            dma_opt_mapping_size(&pdev->dev) >> 9);
    dev->ctrl.max_segments = NVME_MAX_SEGS;     // 4096/16 = 256개 SGL 세그먼트
    dev->ctrl.max_integrity_segments = 1;

    return dev;
    // ... 에러 처리 생략 ...
}
```

#### nvme_init_ctrl(): 공통 컨트롤러 구조체 초기화

```c
// drivers/nvme/host/core.c:5987

int nvme_init_ctrl(struct nvme_ctrl *ctrl, struct device *dev,
        const struct nvme_ctrl_ops *ops, unsigned long quirks)
{
    int ret;

    WRITE_ONCE(ctrl->state, NVME_CTRL_NEW);   // 초기 상태 = NEW
    ctrl->passthru_err_log_enabled = false;
    clear_bit(NVME_CTRL_FAILFAST_EXPIRED, &ctrl->flags);
    spin_lock_init(&ctrl->lock);
    mutex_init(&ctrl->namespaces_lock);

    ret = init_srcu_struct(&ctrl->srcu);    // SRCU 초기화 (네임스페이스 RCU 보호)
    if (ret)
        return ret;

    mutex_init(&ctrl->scan_lock);
    INIT_LIST_HEAD(&ctrl->namespaces);      // 네임스페이스 리스트 초기화
    xa_init(&ctrl->cels);                   // Command Effects Log 캐시
    ctrl->dev = dev;                        // PCI 디바이스 참조
    ctrl->ops = ops;                        // nvme_pci_ctrl_ops (MMIO read/write 콜백)
    ctrl->quirks = quirks;                  // 하드웨어 quirks 비트맵
    ctrl->numa_node = NUMA_NO_NODE;

    // 워크 함수 등록
    INIT_WORK(&ctrl->scan_work, nvme_scan_work);      // 네임스페이스 스캔
    INIT_WORK(&ctrl->async_event_work, nvme_async_event_work);  // AEN
    INIT_WORK(&ctrl->fw_act_work, nvme_fw_act_work);  // 펌웨어 활성화
    INIT_WORK(&ctrl->delete_work, nvme_delete_ctrl_work);
    init_waitqueue_head(&ctrl->state_wq);

    INIT_DELAYED_WORK(&ctrl->ka_work, nvme_keep_alive_work);   // Keep Alive
    INIT_DELAYED_WORK(&ctrl->failfast_work, nvme_failfast_work);

    // Keep Alive 커맨드 미리 구성
    memset(&ctrl->ka_cmd, 0, sizeof(ctrl->ka_cmd));
    ctrl->ka_cmd.common.opcode = nvme_admin_keep_alive;
    ctrl->ka_last_check_time = jiffies;

    // Discard(TRIM) 범위 저장용 페이지 할당
    ctrl->discard_page = alloc_page(GFP_KERNEL);
    if (!ctrl->discard_page) { ret = -ENOMEM; goto out; }

    // 컨트롤러 인스턴스 번호 할당 (nvme0, nvme1, ...)
    ret = ida_alloc(&nvme_instance_ida, GFP_KERNEL);
    if (ret < 0) goto out;
    ctrl->instance = ret;

    // NVMe-oF 인증 초기화
    ret = nvme_auth_init_ctrl(ctrl);
    if (ret) goto out_release_instance;

    // 멀티패스 초기화
    nvme_mpath_init_ctrl(ctrl);

    // 디바이스 모델에 컨트롤러 등록 준비
    device_initialize(&ctrl->ctrl_device);
    ctrl->device = &ctrl->ctrl_device;
    ctrl->device->devt = MKDEV(MAJOR(nvme_ctrl_base_chr_devt), ctrl->instance);
        // /dev/nvme0 캐릭터 디바이스의 major:minor 번호
    ctrl->device->class = &nvme_class;
    ctrl->device->parent = ctrl->dev;

    if (ops->dev_attr_groups)
        ctrl->device->groups = ops->dev_attr_groups;
    else
        ctrl->device->groups = nvme_dev_attr_groups;
    ctrl->device->release = nvme_free_ctrl;
    dev_set_drvdata(ctrl->device, ctrl);

    return ret;
    // ... 에러 처리 ...
}
```

#### 2단계~4단계: 컨트롤러 등록, BAR 매핑, 메모리풀

```c
    // nvme_probe() 계속:

    // 2단계: nvme_core에 컨트롤러 등록 → /dev/nvme0 캐릭터 디바이스 생성
    result = nvme_add_ctrl(&dev->ctrl);
    if (result) goto out_put_ctrl;

    // 3단계: PCIe BAR0 MMIO 매핑
    result = nvme_dev_map(dev);
    if (result) goto out_uninit_ctrl;

    // 4단계: PRP DMA 벡터 메모리풀 할당
    result = nvme_pci_alloc_iod_mempool(dev);
    if (result) goto out_dev_unmap;
```

#### nvme_dev_map(): BAR0 MMIO 매핑

```c
// drivers/nvme/host/pci.c:4175

static int nvme_dev_map(struct nvme_dev *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);

    // PCI 메모리 리전을 "nvme" 이름으로 예약
    if (pci_request_mem_regions(pdev, "nvme"))
        return -ENODEV;
        // BAR0 리소스를 다른 드라이버가 사용하지 못하도록 예약
        // /proc/iomem에 "nvme"로 표시됨

    // BAR0의 최소 필수 영역을 ioremap
    if (nvme_remap_bar(dev, NVME_REG_DBS + 4096))
        goto release;
        // NVME_REG_DBS = 0x1000 (Doorbell 시작)
        // + 4096 = Admin 큐의 Doorbell까지 포함 → 최소 0x2000 바이트 매핑
        // I/O 큐 수에 따라 나중에 nvme_remap_bar()로 확장

    return 0;
  release:
    pci_release_mem_regions(pdev);
    return -ENODEV;
}
```

#### nvme_remap_bar(): BAR0 ioremap 핵심

```c
// drivers/nvme/host/pci.c:2813

static int nvme_remap_bar(struct nvme_dev *dev, unsigned long size)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);

    // 이미 충분히 매핑되어 있으면 건너뛴다
    if (size <= dev->bar_mapped_size)
        return 0;

    // 요청 크기가 실제 BAR 크기를 초과하면 실패
    if (size > pci_resource_len(pdev, 0))
        return -ENOMEM;

    // 기존 매핑이 있으면 해제
    if (dev->bar)
        iounmap(dev->bar);

    // BAR0을 새 크기로 ioremap
    dev->bar = ioremap(pci_resource_start(pdev, 0), size);
        // ★★★ 핵심: PCI BAR0의 물리 주소를 커널 가상 주소에 매핑
        // pci_resource_start(): BIOS가 PCI enumeration 시 할당한 BAR 물리 주소
        // ioremap(): 이 물리 주소를 커널이 접근할 수 있는 가상 주소에 매핑
    if (!dev->bar) {
        dev->bar_mapped_size = 0;
        return -ENOMEM;
    }
    dev->bar_mapped_size = size;

    // ★★★ Doorbell 시작 주소 계산 ★★★
    dev->dbs = dev->bar + NVME_REG_DBS;
        // NVME_REG_DBS = 0x1000 (NVMe 스펙 기준)
        // dev->bar: NVMe 컨트롤러 레지스터의 시작 (CAP, VS, CC, CSTS 등)
        // dev->dbs: Doorbell 레지스터의 시작 (SQ0 Tail Doorbell)
        //
        // BAR0 레이아웃:
        // +0x0000: CAP (Controller Capabilities)
        // +0x0008: VS (Version)
        // +0x0014: CC (Controller Configuration)
        // +0x001C: CSTS (Controller Status)
        // +0x0024: AQA (Admin Queue Attributes)
        // +0x0028: ASQ (Admin SQ Base Address)
        // +0x0030: ACQ (Admin CQ Base Address)
        // +0x1000: SQ0 Tail Doorbell ← dev->dbs가 여기를 가리킴
        // +0x1000 + stride: CQ0 Head Doorbell
        // +0x1000 + 2*stride: SQ1 Tail Doorbell
        // ...

    return 0;
}
```

#### 5단계~14단계: PCI 활성화부터 블록 디바이스 생성까지

```c
    // nvme_probe() 계속:

    // 5단계: PCI 디바이스 활성화 + Admin Queue 설정
    result = nvme_pci_enable(dev);
    if (result) goto out_release_iod_mempool;

    // 6단계: Admin 큐용 blk-mq 태그셋 생성
    result = nvme_alloc_admin_tag_set(&dev->ctrl, &dev->admin_tagset,
                &nvme_mq_admin_ops, sizeof(struct nvme_iod));
    if (result) goto out_disable;

    // 7단계: 컨트롤러 상태를 CONNECTING으로 전환
    if (!nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_CONNECTING)) {
        result = -EBUSY;
        goto out_disable;
    }

    // 8단계: Identify Controller 실행 + 기능 협상
    result = nvme_init_ctrl_finish(&dev->ctrl, false);
        // Identify Controller 커맨드를 Admin 큐로 전송
        // MDTS, ONCS, CNTLID, SN, MN, FR 등 컨트롤러 정보 수집
        // 전력 상태(APST), 온도 임계값 등 설정
    if (result) goto out_disable;

    // 메타데이터 SGL 지원 확인
    if (nvme_ctrl_meta_sgl_supported(&dev->ctrl))
        dev->ctrl.max_integrity_segments = NVME_MAX_META_SEGS;
    else
        dev->ctrl.max_integrity_segments = 1;

    // 9단계: Shadow Doorbell Buffer DMA 메모리 할당
    nvme_dbbuf_dma_alloc(dev);

    // 10단계: HMB(Host Memory Buffer) 설정
    result = nvme_setup_host_mem(dev);
    if (result < 0) goto out_disable;

    nvme_update_attrs(dev);

    // 11단계: I/O 큐 생성 (상세 분석은 4.2절에서)
    result = nvme_setup_io_queues(dev);
    if (result) goto out_disable;

    // 12단계: I/O 큐용 blk-mq 태그셋 생성
    if (dev->online_queues > 1) {
        nvme_alloc_io_tag_set(&dev->ctrl, &dev->tagset, &nvme_mq_ops,
                nvme_pci_nr_maps(dev), sizeof(struct nvme_iod));
        nvme_dbbuf_set(dev);
    }

    if (!dev->ctrl.tagset)
        dev_warn(dev->ctrl.device, "IO queues not created\n");

    // 13단계: 컨트롤러 상태를 LIVE로 전환 → I/O 수신 가능
    if (!nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_LIVE)) {
        result = -ENODEV;
        goto out_disable;
    }

    pci_set_drvdata(pdev, dev);   // PCI 디바이스에 nvme_dev 포인터 저장

    // 14단계: 컨트롤러 시작 → 네임스페이스 스캔
    nvme_start_ctrl(&dev->ctrl);
    nvme_put_ctrl(&dev->ctrl);
    flush_work(&dev->ctrl.scan_work);   // 스캔 완료 대기
    return 0;

    // 에러 시 역순으로 정리
out_disable:
    nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_DELETING);
    nvme_dev_disable(dev, true);
    nvme_free_host_mem(dev);
    nvme_dev_remove_admin(dev);
    nvme_dbbuf_dma_free(dev);
    nvme_free_queues(dev, 0);
out_release_iod_mempool:
    mempool_destroy(dev->dmavec_mempool);
out_dev_unmap:
    nvme_dev_unmap(dev);
out_uninit_ctrl:
    nvme_uninit_ctrl(&dev->ctrl);
out_put_ctrl:
    nvme_put_ctrl(&dev->ctrl);
    return result;
}
```

### 4.2 nvme_reset_work() (리셋 시 재초기화 경로)

`nvme_probe()`가 최초 초기화 경로라면, `nvme_reset_work()`는 리셋 후 재초기화 경로이다. 두 경로는 구조가 매우 유사하다.

```c
// drivers/nvme/host/pci.c:3981

static void nvme_reset_work(struct work_struct *work)
{
    struct nvme_dev *dev =
        container_of(work, struct nvme_dev, ctrl.reset_work);
    bool was_suspend = !!(dev->ctrl.ctrl_config & NVME_CC_SHN_NORMAL);
        // 이전 상태가 shutdown이었는지 확인
    int result;

    // 상태 검증: RESETTING 상태에서만 리셋 수행 가능
    if (nvme_ctrl_state(&dev->ctrl) != NVME_CTRL_RESETTING) {
        dev_warn(dev->ctrl.device, "ctrl state %d is not RESETTING\n",
                 dev->ctrl.state);
        result = -ENODEV;
        goto out;
    }

    // 이미 Enable 상태이면 먼저 disable
    if (dev->ctrl.ctrl_config & NVME_CC_ENABLE)
        nvme_dev_disable(dev, false);
    nvme_sync_queues(&dev->ctrl);

    // PCI 재활성화 + Admin Queue 재설정
    mutex_lock(&dev->shutdown_lock);
    result = nvme_pci_enable(dev);
    if (result) goto out_unlock;
    nvme_unquiesce_admin_queue(&dev->ctrl);
    mutex_unlock(&dev->shutdown_lock);

    // 이하 nvme_probe()와 유사한 흐름:
    // CONNECTING 상태 전환 → Identify → Shadow Doorbell → HMB → I/O 큐 → LIVE
    // ... (생략, nvme_probe()와 동일 구조) ...

    nvme_start_ctrl(&dev->ctrl);
    return;

 out_unlock:
    mutex_unlock(&dev->shutdown_lock);
 out:
    // 리셋 실패 시 컨트롤러를 DEAD로 마킹
    nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_DELETING);
    nvme_dev_disable(dev, true);
    nvme_sync_queues(&dev->ctrl);
    nvme_mark_namespaces_dead(&dev->ctrl);
    nvme_unquiesce_io_queues(&dev->ctrl);
    nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_DEAD);
}
```

#### nvme_pci_enable(): PCI 디바이스 활성화 + Admin Queue 설정

```c
// drivers/nvme/host/pci.c:3750

static int nvme_pci_enable(struct nvme_dev *dev)
{
    int result = -ENOMEM;
    struct pci_dev *pdev = to_pci_dev(dev->dev);
    unsigned int flags = PCI_IRQ_ALL_TYPES;
        // MSI-X, MSI, INTx 모두 허용

    // ★ PCI 디바이스 메모리 리소스 활성화
    if (pci_enable_device_mem(pdev))
        return result;
        // PCI 설정 공간의 Command 레지스터에서 Memory Space 비트를 설정한다
        // 이후 BAR0 MMIO 접근이 가능해진다

    // ★ PCI Bus Master 활성화
    pci_set_master(pdev);
        // Command 레지스터의 Bus Master Enable 비트를 설정한다
        // 이것이 있어야 NVMe 컨트롤러가 DMA로 호스트 메모리에 접근할 수 있다
        // (CQ 엔트리 쓰기, SQ 커맨드 fetch, 데이터 전송 등)

    // 디바이스 상태 레지스터 읽기 테스트
    if (readl(dev->bar + NVME_REG_CSTS) == -1) {
        // 0xFFFFFFFF = PCI 디바이스가 응답하지 않음 (링크 다운 등)
        result = -ENODEV;
        goto disable;
    }

    // ★ 인터럽트 벡터 1개 할당 (초기 설정용)
    if (dev->ctrl.quirks & NVME_QUIRK_BROKEN_MSI)
        flags &= ~PCI_IRQ_MSI;       // MSI가 고장난 디바이스는 MSI 제외
    result = pci_alloc_irq_vectors(pdev, 1, 1, flags);
        // 최소 1개, 최대 1개의 인터럽트 벡터를 할당한다
        // Admin 큐용으로 먼저 1개만 할당하고,
        // I/O 큐 설정 시(nvme_setup_io_queues) 필요한 만큼 재할당한다
    if (result < 0)
        goto disable;

    // ★★★ CAP 레지스터 읽기: 컨트롤러 능력 파악 ★★★
    dev->ctrl.cap = lo_hi_readq(dev->bar + NVME_REG_CAP);
        // lo_hi_readq: 64비트 MMIO 읽기 (32비트 2회: low → high)
        // CAP 레지스터 필드:
        //   MQES: Maximum Queue Entries Supported (0-based)
        //   CQR: Contiguous Queues Required
        //   AMS: Arbitration Mechanism Supported
        //   TO: Timeout (500ms 단위)
        //   DSTRD: Doorbell Stride (4바이트 단위 간격)
        //   MPSMIN/MPSMAX: 메모리 페이지 크기 범위
        //   CSS: Command Set Supported

    // ★ 큐 깊이 결정
    dev->q_depth = min_t(u32, NVME_CAP_MQES(dev->ctrl.cap) + 1,
                io_queue_depth);
        // CAP.MQES+1 (디바이스 최대)과 io_queue_depth (모듈 파라미터) 중 작은 값
        // 기본 io_queue_depth = 1024

    // ★ Doorbell Stride 계산
    dev->db_stride = 1 << NVME_CAP_STRIDE(dev->ctrl.cap);
        // CAP.DSTRD 필드: Doorbell 레지스터 간격 = 4 << DSTRD 바이트
        // DSTRD=0이면 db_stride=1, 실제 간격 = 4*1 = 4바이트
        // 대부분의 NVMe SSD는 DSTRD=0

    // ★★★ Doorbell 시작 주소 설정 ★★★
    dev->dbs = dev->bar + 4096;
        // BAR0 + 0x1000 = SQ0 Tail Doorbell
        // NVMe 스펙: Doorbell 영역은 BAR0 오프셋 0x1000부터 시작
        // 이 주소에 writel()로 SQ Tail 값을 쓰면 디바이스에 새 커맨드를 알림

    // SQ Entry Size 설정
    if (dev->ctrl.quirks & NVME_QUIRK_128_BYTES_SQES)
        dev->io_sqes = 7;        // Apple: 128바이트 SQE (2^7)
    else
        dev->io_sqes = NVME_NVM_IOSQES;  // 표준: 64바이트 SQE (2^6)

    // 특수 디바이스의 큐 깊이 오버라이드
    if (dev->ctrl.quirks & NVME_QUIRK_QDEPTH_ONE) {
        dev->q_depth = 2;        // 큐 깊이를 2로 강제 (최소값)
    } else if (pdev->vendor == PCI_VENDOR_ID_SAMSUNG &&
               (pdev->device == 0xa821 || pdev->device == 0xa822) &&
               NVME_CAP_MQES(dev->ctrl.cap) == 0) {
        dev->q_depth = 64;       // 삼성 PM1725: MQES 버그 우회
    }

    // Shared Tags quirk: Admin 큐 태그 공간 확보
    if ((dev->ctrl.quirks & NVME_QUIRK_SHARED_TAGS) &&
        (dev->q_depth < (NVME_AQ_DEPTH + 2))) {
        dev->q_depth = NVME_AQ_DEPTH + 2;
    }

    dev->ctrl.sqsize = dev->q_depth - 1;   // 0-based 큐 크기

    // CMB(Controller Memory Buffer) 매핑 시도
    nvme_map_cmb(dev);

    // PCI 설정 저장 (에러 복구 시 복원용)
    pci_save_state(pdev);

    // ★ Admin Queue 설정 (다음 섹션에서 상세 분석)
    result = nvme_pci_configure_admin_queue(dev);
    if (result)
        goto free_irq;
    return result;

 free_irq:
    pci_free_irq_vectors(pdev);
 disable:
    pci_disable_device(pdev);
    return result;
}
```

#### nvme_pci_configure_admin_queue(): Admin Queue 설정

NVMe 스펙에 정의된 Admin Queue 설정 절차를 정확히 따른다.

```
Admin Queue 설정이 특별한 이유:
┌────────────────────────────────────────────────────────────┐
│  I/O 큐는 Admin 커맨드(Create CQ/SQ)로 생성한다.           │
│  그런데 Admin 큐가 없으면 Admin 커맨드를 보낼 수 없다.      │
│  → 닭과 달걀 문제!                                         │
│                                                            │
│  해결: Admin 큐는 MMIO 레지스터(AQA/ASQ/ACQ)로 직접 설정    │
│  → Admin 커맨드 전송 가능해짐                               │
│  → Admin 커맨드로 I/O 큐를 생성                             │
└────────────────────────────────────────────────────────────┘
```

```c
// drivers/nvme/host/pci.c:2851

static int nvme_pci_configure_admin_queue(struct nvme_dev *dev)
{
    int result;
    u32 aqa;
    struct nvme_queue *nvmeq;

    // 1. BAR0 매핑 크기가 Admin 큐 Doorbell까지 포함하는지 확인/확장
    result = nvme_remap_bar(dev, db_bar_size(dev, 0));
        // db_bar_size(dev, 0): qid=0(Admin)의 Doorbell까지 필요한 BAR 크기
    if (result < 0)
        return result;

    // 2. NVMe 서브시스템 리셋 지원 확인
    dev->subsystem = readl(dev->bar + NVME_REG_VS) >= NVME_VS(1, 1, 0) ?
                NVME_CAP_NSSRC(dev->ctrl.cap) : 0;
        // NVMe 1.1+ 이고 CAP.NSSRC=1이면 서브시스템 리셋 가능

    // 3. NVM Subsystem Reset Occurred 비트 클리어
    if (dev->subsystem &&
        (readl(dev->bar + NVME_REG_CSTS) & NVME_CSTS_NSSRO))
        writel(NVME_CSTS_NSSRO, dev->bar + NVME_REG_CSTS);

    // ★★★ 4. 컨트롤러 비활성화 (CC.EN = 0) ★★★
    result = nvme_disable_ctrl(&dev->ctrl, false);
        // BIOS가 이미 활성화해 놓았을 수 있으므로 먼저 비활성화한다
        // CC.EN=0을 쓰고 CSTS.RDY=0이 될 때까지 대기
    if (result < 0) {
        struct pci_dev *pdev = to_pci_dev(dev->dev);

        // NVMe 리셋이 실패하면 PCIe FLR(Function Level Reset)로 더 강한 리셋 시도
        result = pcie_reset_flr(pdev, false);
        if (result < 0)
            return result;

        pci_restore_state(pdev);
        result = nvme_disable_ctrl(&dev->ctrl, false);
        if (result < 0)
            return result;
    }

    // ★★★ 5. Admin 큐 DMA 메모리 할당 ★★★
    result = nvme_alloc_queue(dev, 0, NVME_AQ_DEPTH);
        // qid=0, depth=NVME_AQ_DEPTH(보통 32)
        // SQ 버퍼 + CQ 버퍼를 DMA coherent 메모리로 할당
        // 상세 분석은 아래 nvme_alloc_queue 참조
    if (result)
        return result;

    dev->ctrl.numa_node = dev_to_node(dev->dev);

    nvmeq = &dev->queues[0];    // Admin 큐

    // ★★★ 6. AQA 레지스터 설정 ★★★
    aqa = nvmeq->q_depth - 1;   // ASQS (Admin SQ Size, 0-based)
    aqa |= aqa << 16;           // ACQS (Admin CQ Size, 0-based) = ASQS
        // AQA 레지스터 포맷:
        // [31:16] ACQS: Admin CQ Size (0-based, 최대 4095)
        // [15:0]  ASQS: Admin SQ Size (0-based, 최대 4095)
        // NVME_AQ_DEPTH=32이면 aqa = 31 | (31 << 16) = 0x001F001F
    writel(aqa, dev->bar + NVME_REG_AQA);

    // ★★★ 7. ASQ/ACQ 레지스터에 DMA 주소 설정 ★★★
    lo_hi_writeq(nvmeq->sq_dma_addr, dev->bar + NVME_REG_ASQ);
        // ASQ: Admin Submission Queue Base Address
        // DMA 할당으로 받은 SQ 버퍼의 물리(DMA) 주소를 컨트롤러에 알린다
        // 컨트롤러는 이 주소에서 Admin 커맨드를 DMA로 fetch한다
    lo_hi_writeq(nvmeq->cq_dma_addr, dev->bar + NVME_REG_ACQ);
        // ACQ: Admin Completion Queue Base Address
        // 컨트롤러가 완료 엔트리를 이 주소에 DMA로 쓴다

    // ★★★ 8. 컨트롤러 활성화 (CC.EN = 1) ★★★
    result = nvme_enable_ctrl(&dev->ctrl);
        // CC 레지스터 구성 + CC.EN=1 + CSTS.RDY=1 대기
        // 상세 분석은 아래 nvme_enable_ctrl 참조
    if (result)
        return result;

    // 9. Admin 큐 인터럽트 벡터 설정
    nvmeq->cq_vector = 0;    // Admin 큐는 항상 벡터 0

    // 10. 큐 상태 초기화
    nvme_init_queue(nvmeq, 0);

    // 11. IRQ 핸들러 등록
    result = queue_request_irq(nvmeq);
        // pci_request_irq()로 MSI-X/MSI 인터럽트 핸들러를 등록한다
        // 핸들러: nvme_irq() (또는 threaded: nvme_irq_check + nvme_irq)
        // 인터럽트 이름: "nvme0q0"
    if (result) {
        dev->online_queues--;
        return result;
    }

    // 12. 큐 활성화 플래그 설정
    set_bit(NVMEQ_ENABLED, &nvmeq->flags);
    return result;
}
```

#### nvme_disable_ctrl(): CC.EN=0으로 컨트롤러 비활성화

```c
// drivers/nvme/host/core.c:3202

int nvme_disable_ctrl(struct nvme_ctrl *ctrl, bool shutdown)
{
    int ret;

    ctrl->ctrl_config &= ~NVME_CC_SHN_MASK;   // Shutdown 통지 비트 클리어

    if (shutdown)
        ctrl->ctrl_config |= NVME_CC_SHN_NORMAL;  // 정상 셧다운 통지
    else
        ctrl->ctrl_config &= ~NVME_CC_ENABLE;     // CC.EN = 0 (리셋)

    // CC 레지스터에 쓰기 (MMIO writel)
    ret = ctrl->ops->reg_write32(ctrl, NVME_REG_CC, ctrl->ctrl_config);
        // nvme_pci_reg_write32: writel(val, dev->bar + off)
    if (ret)
        return ret;

    if (shutdown) {
        // 셧다운: CSTS.SHST가 Complete가 될 때까지 대기
        return nvme_wait_ready(ctrl, NVME_CSTS_SHST_MASK,
                               NVME_CSTS_SHST_CMPLT,
                               ctrl->shutdown_timeout, "shutdown");
    }

    // 리셋 지연이 필요한 quirky 디바이스
    if (ctrl->quirks & NVME_QUIRK_DELAY_BEFORE_CHK_RDY)
        msleep(NVME_QUIRK_DELAY_AMOUNT);

    // ★ CSTS.RDY = 0이 될 때까지 폴링
    return nvme_wait_ready(ctrl, NVME_CSTS_RDY, 0,
                           (NVME_CAP_TIMEOUT(ctrl->cap) + 1) / 2, "reset");
        // CAP.TO * 500ms 이내에 RDY=0이 되어야 한다
        // 타임아웃 초과 시 -ENODEV 반환
}
```

#### nvme_enable_ctrl(): CC.EN=1로 컨트롤러 활성화

```c
// drivers/nvme/host/core.c:3245

int nvme_enable_ctrl(struct nvme_ctrl *ctrl)
{
    unsigned dev_page_min;
    u32 timeout;
    int ret;

    // CAP 레지스터 다시 읽기 (disable 후 변경될 수 있음)
    ret = ctrl->ops->reg_read64(ctrl, NVME_REG_CAP, &ctrl->cap);
    if (ret) return ret;

    // 최소 메모리 페이지 크기 호환성 확인
    dev_page_min = NVME_CAP_MPSMIN(ctrl->cap) + 12;
        // MPSMIN=0이면 dev_page_min=12 → 최소 4KB (2^12)
    if (NVME_CTRL_PAGE_SHIFT < dev_page_min) {
        // 호스트 페이지(4KB)보다 디바이스 최소 페이지가 크면 지원 불가
        return -ENODEV;
    }

    // ★★★ CC(Controller Configuration) 레지스터 구성 ★★★
    //
    // CSS: Command Set Selection
    if (NVME_CAP_CSS(ctrl->cap) & NVME_CAP_CSS_CSI)
        ctrl->ctrl_config = NVME_CC_CSS_CSI;   // I/O Command Set 선택 (NVMe 2.0+)
    else
        ctrl->ctrl_config = NVME_CC_CSS_NVM;   // NVM Command Set (기본)

    ctrl->ctrl_config &= ~NVME_CC_CRIME;       // Controller Ready Independent of Media 비활성화

    // MPS: Memory Page Size (호스트 페이지 크기)
    ctrl->ctrl_config |= (NVME_CTRL_PAGE_SHIFT - 12) << NVME_CC_MPS_SHIFT;
        // NVME_CTRL_PAGE_SHIFT=12이면 MPS=0 → 4KB 페이지

    // AMS: Arbitration Mechanism Selection = Round Robin
    ctrl->ctrl_config |= NVME_CC_AMS_RR | NVME_CC_SHN_NONE;

    // IOSQES/IOCQES: I/O SQ/CQ Entry Size
    ctrl->ctrl_config |= NVME_CC_IOSQES | NVME_CC_IOCQES;
        // IOSQES = 6 (2^6 = 64바이트)
        // IOCQES = 4 (2^4 = 16바이트)

    // CC 레지스터에 설정 쓰기 (EN 비트는 아직 0)
    ret = ctrl->ops->reg_write32(ctrl, NVME_REG_CC, ctrl->ctrl_config);
    if (ret) return ret;

    // CAP이 CC 쓰기 후 변경될 수 있으므로 다시 읽기
    ret = ctrl->ops->reg_read64(ctrl, NVME_REG_CAP, &ctrl->cap);
    if (ret) return ret;

    // 타임아웃 계산
    timeout = NVME_CAP_TIMEOUT(ctrl->cap);
        // CAP.TO: 500ms 단위의 타임아웃
    if (ctrl->cap & NVME_CAP_CRMS_CRWMS) {
        // CRTO(Controller Ready Timeout) 레지스터가 있으면 더 정확한 타임아웃 사용
        u32 crto, ready_timeout;
        ret = ctrl->ops->reg_read32(ctrl, NVME_REG_CRTO, &crto);
        if (ret) return ret;
        ready_timeout = NVME_CRTO_CRWMT(crto);
        if (ready_timeout >= timeout)
            timeout = ready_timeout;
    }

    // ★★★ CC.EN = 1: 컨트롤러 활성화! ★★★
    ctrl->ctrl_config |= NVME_CC_ENABLE;
    ret = ctrl->ops->reg_write32(ctrl, NVME_REG_CC, ctrl->ctrl_config);
        // 이 순간 NVMe 컨트롤러가 내부 초기화를 시작한다
        // 펌웨어 로드, NAND 매핑 테이블 읽기 등
    if (ret) return ret;

    // ★ CSTS.RDY = 1이 될 때까지 폴링
    return nvme_wait_ready(ctrl, NVME_CSTS_RDY, NVME_CSTS_RDY,
                           (timeout + 1) / 2, "initialisation");
        // RDY=1이 되면 Admin 큐가 사용 가능하다
        // 일반적으로 수백 ms ~ 수 초 소요
}
```

**CC 레지스터 비트 필드 요약:**

```
CC (Controller Configuration) 레지스터 (offset 0x14):
 ┌────────────────────────────────────────────────────────┐
 │ [31:24] 예약                                           │
 │ [23:20] IOCQES = 4 (CQ Entry Size = 2^4 = 16바이트)   │
 │ [19:16] IOSQES = 6 (SQ Entry Size = 2^6 = 64바이트)   │
 │ [15:14] SHN = 00 (Shutdown Notification: 없음)         │
 │ [13:11] AMS = 000 (Arbitration: Round Robin)           │
 │ [10:7]  MPS = 0 (Memory Page Size: 2^(12+0) = 4KB)    │
 │ [6:4]   CSS = NVM 또는 CSI                              │
 │ [3:1]   예약                                            │
 │ [0]     EN = 1 (Controller Enable)  ★                   │
 └────────────────────────────────────────────────────────┘
```

#### nvme_alloc_queue(): SQ/CQ DMA 메모리 할당

```c
// drivers/nvme/host/pci.c:2581

static int nvme_alloc_queue(struct nvme_dev *dev, int qid, int depth)
{
    struct nvme_queue *nvmeq = &dev->queues[qid];

    // 이미 할당된 큐이면 건너뛴다 (리셋 시 재호출 방지)
    if (dev->ctrl.queue_count > qid)
        return 0;

    // SQ Entry Size 설정
    nvmeq->sqes = qid ? dev->io_sqes : NVME_ADM_SQES;
        // Admin 큐(qid=0): NVME_ADM_SQES = 6 (64바이트)
        // I/O 큐(qid>0): io_sqes (보통 6, Apple은 7)
    nvmeq->q_depth = depth;

    // ★ CQ 버퍼 DMA 할당
    nvmeq->cqes = dma_alloc_coherent(dev->dev, CQ_SIZE(nvmeq),
                     &nvmeq->cq_dma_addr, GFP_KERNEL);
        // CQ_SIZE = q_depth * sizeof(nvme_completion) = q_depth * 16
        // dma_alloc_coherent(): CPU와 디바이스 모두 접근 가능한 DMA 메모리 할당
        //   - nvmeq->cqes: CPU 가상 주소 (호스트가 CQ 엔트리를 읽는 데 사용)
        //   - nvmeq->cq_dma_addr: DMA 물리 주소 (컨트롤러가 CQ에 쓸 때 사용)
    if (!nvmeq->cqes)
        goto free_nvmeq;

    // ★ SQ 버퍼 DMA 할당
    if (nvme_alloc_sq_cmds(dev, nvmeq, qid))
        goto free_cqdma;
        // CMB가 사용 가능하면 CMB에, 아니면 dma_alloc_coherent로 할당
        // SQ_SIZE = q_depth << sqes = q_depth * 64

    nvmeq->dev = dev;
    spin_lock_init(&nvmeq->sq_lock);      // SQ 접근 보호 스핀락
    spin_lock_init(&nvmeq->cq_poll_lock); // CQ 폴링 보호 스핀락
    nvmeq->cq_head = 0;
    nvmeq->cq_phase = 1;
        // Phase Tag를 1로 초기화. CQ 메모리는 0으로 초기화되어 있으므로
        // 모든 CQE의 phase=0 ≠ cq_phase=1 → "아직 처리 안 됨"으로 판단
        // 컨트롤러가 첫 CQE를 쓸 때 phase=1로 설정 → cq_phase=1과 일치 → "새 완료!"

    // ★★★ Doorbell MMIO 주소 계산 ★★★
    nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];
        // dev->dbs = dev->bar + 0x1000 (Doorbell 영역 시작)
        // 각 큐는 SQ Tail Doorbell + CQ Head Doorbell = 2개의 Doorbell을 사용
        //
        // q_db 계산 예 (db_stride=1일 때):
        //   qid=0: &dbs[0]   → BAR0+0x1000 (Admin SQ Tail DB)
        //   qid=1: &dbs[2]   → BAR0+0x1008 (I/O Q1 SQ Tail DB)
        //   qid=2: &dbs[4]   → BAR0+0x1010 (I/O Q2 SQ Tail DB)
        //
        // 이 주소에 writel(sq_tail, q_db)하면 디바이스에 "새 커맨드 있음" 알림
        // q_db + db_stride에 writel(cq_head, ...)하면 "CQ 소비 완료" 알림

    nvmeq->qid = qid;
    dev->ctrl.queue_count++;

    return 0;

 free_cqdma:
    dma_free_coherent(dev->dev, CQ_SIZE(nvmeq), (void *)nvmeq->cqes,
              nvmeq->cq_dma_addr);
 free_nvmeq:
    return -ENOMEM;
}
```

**DMA 메모리 할당 구조:**

```
  호스트 메모리 (dma_alloc_coherent)
  ┌─────────────────────────────────────────────┐
  │  SQ 버퍼 (sq_cmds)                          │
  │  ┌───────┬───────┬───────┬─── ─ ─ ─ ───┐   │
  │  │ Cmd 0 │ Cmd 1 │ Cmd 2 │   ...        │   │
  │  │ 64B   │ 64B   │ 64B   │              │   │
  │  └───────┴───────┴───────┴─── ─ ─ ─ ───┘   │
  │  CPU가 여기에 NVMe 커맨드를 쓴다             │
  │  컨트롤러가 DMA로 fetch한다                  │
  │                                              │
  │  DMA 주소: sq_dma_addr (ASQ 레지스터에 설정)  │
  └─────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────┐
  │  CQ 버퍼 (cqes)                              │
  │  ┌───────┬───────┬───────┬─── ─ ─ ─ ───┐   │
  │  │ CQE 0 │ CQE 1 │ CQE 2 │   ...       │   │
  │  │ 16B   │ 16B   │ 16B   │              │   │
  │  └───────┴───────┴───────┴─── ─ ─ ─ ───┘   │
  │  컨트롤러가 DMA로 여기에 완료 결과를 쓴다     │
  │  CPU가 여기서 읽고 처리한다                   │
  │                                              │
  │  DMA 주소: cq_dma_addr (ACQ 레지스터에 설정)  │
  └─────────────────────────────────────────────┘
```

#### nvme_init_queue(): 큐 상태 초기화

```c
// drivers/nvme/host/pci.c:2657

static void nvme_init_queue(struct nvme_queue *nvmeq, u16 qid)
{
    struct nvme_dev *dev = nvmeq->dev;

    // SQ/CQ 포인터를 처음으로 리셋
    nvmeq->sq_tail = 0;        // 다음 커맨드를 쓸 SQ 슬롯 = 0
    nvmeq->last_sq_tail = 0;   // 마지막으로 Doorbell에 기록한 SQ Tail = 0
    nvmeq->cq_head = 0;        // 다음 읽을 CQE 슬롯 = 0
    nvmeq->cq_phase = 1;       // 예상 phase = 1

    // Doorbell MMIO 주소 재계산 (BAR remap 후 주소가 바뀔 수 있으므로)
    nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];

    // ★ CQ 버퍼를 0으로 클리어
    memset((void *)nvmeq->cqes, 0, CQ_SIZE(nvmeq));
        // 모든 CQE의 status.phase = 0이 됨
        // cq_phase = 1이므로, phase 불일치 = "미처리 엔트리 없음"
        // 컨트롤러가 새 완료를 쓸 때 phase=1을 설정하면 일치 → "새 완료!"

    // Shadow Doorbell 초기화
    nvme_dbbuf_init(dev, nvmeq, qid);

    dev->online_queues++;

    // ★ Write Memory Barrier
    wmb();
        // 이 배리어는 위의 모든 초기화 작업이 메모리에 커밋된 후에야
        // 인터럽트 핸들러가 이 큐를 볼 수 있도록 보장한다.
        // 배리어 없으면: IRQ 핸들러가 아직 초기화 안 된 cq_head/cq_phase를 읽을 수 있음
}
```

#### nvme_setup_io_queues(): I/O 큐 생성

```c
// drivers/nvme/host/pci.c:3502

static int nvme_setup_io_queues(struct nvme_dev *dev)
{
    struct nvme_queue *adminq = &dev->queues[0];
    struct pci_dev *pdev = to_pci_dev(dev->dev);
    unsigned int nr_io_queues;
    unsigned long size;
    int result;

    // 모듈 파라미터를 snapshot (리셋 간 안정성)
    dev->nr_write_queues = write_queues;
    dev->nr_poll_queues = poll_queues;

    // ★ 1단계: 컨트롤러에 I/O 큐 수 요청 (Set Features: Number of Queues)
    nr_io_queues = dev->nr_allocated_queues - 1;
        // 요청할 I/O 큐 수 = 할당된 큐 수 - 1 (Admin 제외)
    result = nvme_set_queue_count(&dev->ctrl, &nr_io_queues);
        // Admin 커맨드 "Set Features - Number of Queues"를 전송
        // 컨트롤러가 지원 가능한 큐 수를 응답
        // nr_io_queues가 실제 할당 가능한 수로 갱신됨
    if (result < 0)
        return result;

    if (nr_io_queues == 0)
        return 0;   // I/O 큐 없이 Admin만으로 동작 (관리 전용)

    // shutdown_lock 획득
    result = nvme_setup_io_queues_trylock(dev);
    if (result) return result;

    // Admin 큐의 기존 IRQ 해제
    if (test_and_clear_bit(NVMEQ_ENABLED, &adminq->flags))
        pci_free_irq(pdev, 0, adminq);

    // CMB에 SQ 배치 가능한지 확인
    if (dev->cmb_use_sqes) {
        result = nvme_cmb_qdepth(dev, nr_io_queues, sizeof(struct nvme_command));
        if (result > 0) {
            dev->q_depth = result;
            dev->ctrl.sqsize = result - 1;
        } else {
            dev->cmb_use_sqes = false;
        }
    }

    // ★ 2단계: BAR 매핑 크기 확장 (모든 I/O 큐의 Doorbell까지)
    do {
        size = db_bar_size(dev, nr_io_queues);
            // 필요한 BAR 크기 = 0x1000 + (nr_io_queues + 1) * 2 * 4 * db_stride
        result = nvme_remap_bar(dev, size);
        if (!result)
            break;
        if (!--nr_io_queues) {
            result = -ENOMEM;
            goto out_unlock;
        }
    } while (1);
    // BAR 크기가 부족하면 큐 수를 줄이면서 재시도
    adminq->q_db = dev->dbs;   // Admin 큐 Doorbell 주소 갱신

 retry:
    // Admin 큐 IRQ 해제
    if (test_and_clear_bit(NVMEQ_ENABLED, &adminq->flags))
        pci_free_irq(pdev, 0, adminq);

    pci_free_irq_vectors(pdev);   // 기존 벡터 전부 해제

    // ★ 3단계: MSI-X 인터럽트 벡터 할당
    result = nvme_setup_irqs(dev, nr_io_queues);
        // pci_alloc_irq_vectors_affinity()로 I/O 큐 수만큼 벡터 할당
        // CPU affinity를 고려하여 각 벡터를 CPU에 분배
        // default(읽기+쓰기) 큐와 read-only 큐에 별도의 affinity 세트 사용
    if (result <= 0) {
        result = -EIO;
        goto out_unlock;
    }

    dev->num_vecs = result;
    result = max(result - 1, 1);
    dev->max_qid = result + dev->io_queues[HCTX_TYPE_POLL];
        // 폴링 큐는 인터럽트 벡터가 불필요하므로 추가

    // Admin 큐에 새 IRQ 핸들러 등록
    result = queue_request_irq(adminq);
    if (result) goto out_unlock;
    set_bit(NVMEQ_ENABLED, &adminq->flags);
    mutex_unlock(&dev->shutdown_lock);

    // ★ 4단계: I/O 큐 생성
    result = nvme_create_io_queues(dev);
    if (result || dev->online_queues < 2)
        return result;

    // 일부 큐 생성 실패 시 큐 수 줄여서 재시도
    if (dev->online_queues - 1 < dev->max_qid) {
        nr_io_queues = dev->online_queues - 1;
        nvme_delete_io_queues(dev);
        result = nvme_setup_io_queues_trylock(dev);
        if (result) return result;
        nvme_suspend_io_queues(dev);
        goto retry;
    }

    dev_info(dev->ctrl.device, "%d/%d/%d default/read/poll queues\n",
             dev->io_queues[HCTX_TYPE_DEFAULT],
             dev->io_queues[HCTX_TYPE_READ],
             dev->io_queues[HCTX_TYPE_POLL]);
    return 0;

out_unlock:
    mutex_unlock(&dev->shutdown_lock);
    return result;
}
```

#### nvme_create_io_queues(): I/O 큐 실제 생성

```c
// drivers/nvme/host/pci.c:2941

static int nvme_create_io_queues(struct nvme_dev *dev)
{
    unsigned i, max, rw_queues;
    int ret = 0;

    // ★ 1단계: 모든 I/O 큐의 DMA 메모리 할당
    for (i = dev->ctrl.queue_count; i <= dev->max_qid; i++) {
        if (nvme_alloc_queue(dev, i, dev->q_depth)) {
            ret = -ENOMEM;
            break;
        }
    }

    max = min(dev->max_qid, dev->ctrl.queue_count - 1);

    // 폴링 큐와 인터럽트 큐 구분
    if (max != 1 && dev->io_queues[HCTX_TYPE_POLL]) {
        rw_queues = dev->io_queues[HCTX_TYPE_DEFAULT] +
                    dev->io_queues[HCTX_TYPE_READ];
    } else {
        rw_queues = max;
    }

    // ★ 2단계: 각 큐에 대해 Admin 커맨드로 CQ/SQ 생성
    for (i = dev->online_queues; i <= max; i++) {
        bool polled = i > rw_queues;   // rw_queues 이후는 폴링 큐

        ret = nvme_create_queue(&dev->queues[i], i, polled);
            // 아래에서 상세 분석
        if (ret)
            break;
    }

    // 일부 큐 생성 실패해도 나머지로 동작 가능
    return ret >= 0 ? 0 : ret;
}
```

#### nvme_create_queue(): 단일 I/O 큐 생성 (Admin 커맨드 사용)

```c
// drivers/nvme/host/pci.c:2702

static int nvme_create_queue(struct nvme_queue *nvmeq, int qid, bool polled)
{
    struct nvme_dev *dev = nvmeq->dev;
    int result;
    u16 vector = 0;

    clear_bit(NVMEQ_DELETE_ERROR, &nvmeq->flags);

    // 인터럽트 벡터 결정
    if (!polled)
        vector = dev->num_vecs == 1 ? 0 : qid;
            // 벡터가 1개뿐이면 모든 큐가 벡터 0 공유
            // 여러 개이면 큐 ID가 벡터 번호가 됨
    else
        set_bit(NVMEQ_POLLED, &nvmeq->flags);
            // 폴링 큐: 인터럽트 없음 (io_uring의 IOPOLL 등에서 사용)

    // ★ Admin 커맨드로 CQ 생성 (Create I/O Completion Queue)
    result = adapter_alloc_cq(dev, qid, nvmeq, vector);
        // NVMe Admin 커맨드 opcode=0x05 (Create I/O CQ)
        // CQ DMA 주소, 크기, 인터럽트 벡터를 컨트롤러에 전달
        // CQ가 먼저 생성되어야 SQ가 CQ를 참조할 수 있다
    if (result)
        return result;

    // ★ Admin 커맨드로 SQ 생성 (Create I/O Submission Queue)
    result = adapter_alloc_sq(dev, qid, nvmeq);
        // NVMe Admin 커맨드 opcode=0x01 (Create I/O SQ)
        // SQ DMA 주소, 크기, 연결할 CQ ID를 컨트롤러에 전달
    if (result < 0)
        return result;
    if (result)
        goto release_cq;

    nvmeq->cq_vector = vector;

    // 큐 초기화 + IRQ 등록
    result = nvme_setup_io_queues_trylock(dev);
    if (result) return result;

    nvme_init_queue(nvmeq, qid);
        // sq_tail=0, cq_head=0, cq_phase=1, CQ 메모리 클리어
        // q_db 주소 계산

    if (!polled) {
        result = queue_request_irq(nvmeq);
            // "nvme0q{qid}" 이름으로 IRQ 핸들러 등록
        if (result < 0)
            goto release_sq;
    }

    set_bit(NVMEQ_ENABLED, &nvmeq->flags);
    mutex_unlock(&dev->shutdown_lock);
    return result;

release_sq:
    dev->online_queues--;
    mutex_unlock(&dev->shutdown_lock);
    adapter_delete_sq(dev, qid);
release_cq:
    adapter_delete_cq(dev, qid);
    return result;
}
```

#### nvme_alloc_io_tag_set() + nvme_start_ctrl() → 블록 디바이스 생성

```c
// drivers/nvme/host/core.c:5750

int nvme_alloc_io_tag_set(struct nvme_ctrl *ctrl, struct blk_mq_tag_set *set,
        const struct blk_mq_ops *ops, unsigned int nr_maps,
        unsigned int cmd_size)
{
    int ret;

    memset(set, 0, sizeof(*set));
    set->ops = ops;                    // nvme_mq_ops (queue_rq, complete 등)
    set->queue_depth = min_t(unsigned, ctrl->sqsize, BLK_MQ_MAX_DEPTH - 1);
        // 큐 깊이: 보통 1023 또는 그 이하
    if (ctrl->quirks & NVME_QUIRK_SHARED_TAGS)
        set->reserved_tags = NVME_AQ_DEPTH;    // Admin 태그 공간 예약
    else if (ctrl->ops->flags & NVME_F_FABRICS)
        set->reserved_tags = 1;                // Fabrics 연결용

    set->numa_node = ctrl->numa_node;
    if (ctrl->ops->flags & NVME_F_BLOCKING)
        set->flags |= BLK_MQ_F_BLOCKING;
    set->cmd_size = cmd_size;          // sizeof(nvme_iod) = 요청당 private data
    set->driver_data = ctrl;
    set->nr_hw_queues = ctrl->queue_count - 1;
        // ★ HW 큐 수 = I/O 큐 수 = 보통 CPU 코어 수
        // 각 CPU가 자기 전용 HW 큐를 가지므로 락 경합 최소화
    set->timeout = NVME_IO_TIMEOUT;    // 30초
    set->nr_maps = nr_maps;            // 큐 타입 맵 수 (default, read, poll)

    // ★★★ blk-mq tag_set 할당 → 블록 레이어와 NVMe 연결 ★★★
    ret = blk_mq_alloc_tag_set(set);
    if (ret) return ret;

    ctrl->tagset = set;    // 컨트롤러의 I/O 태그셋으로 등록
    return 0;
}
```

```c
// drivers/nvme/host/core.c:5856

void nvme_start_ctrl(struct nvme_ctrl *ctrl)
{
    // AEN(Async Event Notification) 활성화
    nvme_enable_aen(ctrl);

    if (ctrl->queue_count > 1) {
        // ★ 네임스페이스 스캔 시작
        nvme_queue_scan(ctrl);
            // nvme_wq에 scan_work를 큐잉
            // → nvme_scan_work() 비동기 실행
            //   → nvme_scan_ns_list() 또는 nvme_scan_ns_sequential()
            //     → nvme_alloc_ns()  ← 네임스페이스 생성
            //       → blk_mq_alloc_disk()  ← gendisk 생성
            //       → device_add_disk()    ← /dev/nvme0n1 생성!

        // I/O 큐 활성화
        nvme_unquiesce_io_queues(ctrl);

        // 멀티패스 업데이트
        nvme_mpath_update(ctrl);
    }

    nvme_change_uevent(ctrl, "NVME_EVENT=connected");
    set_bit(NVME_CTRL_STARTED_ONCE, &ctrl->flags);
}
```

#### nvme_alloc_ns() → device_add_disk(): 블록 디바이스 최종 생성

```c
// drivers/nvme/host/core.c:4809

static void nvme_alloc_ns(struct nvme_ctrl *ctrl, struct nvme_ns_info *info)
{
    struct queue_limits lim = { };
    struct nvme_ns *ns;
    struct gendisk *disk;
    int node = ctrl->numa_node;

    // nvme_ns 구조체 할당
    ns = kzalloc_node(sizeof(*ns), GFP_KERNEL, node);
    if (!ns) return;

    // P2P DMA 지원 확인
    if (ctrl->ops->supports_pci_p2pdma &&
        ctrl->ops->supports_pci_p2pdma(ctrl))
        lim.features |= BLK_FEAT_PCI_P2PDMA;

    // ★★★ gendisk + request_queue 생성 ★★★
    disk = blk_mq_alloc_disk(ctrl->tagset, &lim, ns);
        // ctrl->tagset: 위에서 할당한 I/O tag_set
        // 이 tag_set의 HW 큐가 이 디스크의 request_queue에 연결된다
        // 같은 컨트롤러의 모든 네임스페이스가 같은 tag_set을 공유!
    if (IS_ERR(disk))
        goto out_free_ns;

    disk->fops = &nvme_bdev_ops;   // 블록 디바이스 파일 오퍼레이션
    disk->private_data = ns;

    ns->disk = disk;
    ns->queue = disk->queue;
    ns->ctrl = ctrl;
    kref_init(&ns->kref);

    // 네임스페이스 헤드 초기화
    if (nvme_init_ns_head(ns, info))
        goto out_cleanup_disk;

    // ★ 디스크 이름 설정
    if (nvme_ns_head_multipath(ns->head)) {
        sprintf(disk->disk_name, "nvme%dc%dn%d", ...);  // 멀티패스
    } else {
        sprintf(disk->disk_name, "nvme%dn%d",
                ctrl->instance, ns->head->instance);
        // 예: "nvme0n1" (컨트롤러 0, 네임스페이스 1)
    }

    // 네임스페이스 정보(크기, 섹터 크기 등) 설정
    if (nvme_update_ns_info(ns, info))
        goto out_unlink_ns;

    // 네임스페이스를 컨트롤러 리스트에 추가
    mutex_lock(&ctrl->namespaces_lock);
    nvme_ns_add_to_ctrl_list(ns);
    mutex_unlock(&ctrl->namespaces_lock);

    nvme_get_ctrl(ctrl);

    // ★★★ 블록 디바이스를 시스템에 최종 등록 ★★★
    if (device_add_disk(ctrl->device, ns->disk, nvme_ns_attr_groups))
        goto out_cleanup_ns_from_list;
        // 이 순간:
        //   - /dev/nvme0n1 블록 디바이스 노드가 생성된다
        //   - /sys/block/nvme0n1/ sysfs 엔트리가 생성된다
        //   - udev가 이벤트를 받아 파일시스템 마운트 등을 수행할 수 있다
        //   - 사용자 공간에서 I/O를 보낼 수 있다!

    // 멀티패스 디스크 추가
    nvme_mpath_add_disk(ns, info->anagrpid);
}
```

---

## Phase 5: BAR0의 E2E(End-to-End) 여정

PCIe BAR0 물리 주소가 어떻게 Doorbell writel()까지 이어지는지 추적한다.

### 5.1 전체 흐름

```
 ┌────────────────────────────────────────────────────────────────┐
 │ 1. BIOS/UEFI: PCI Enumeration                                 │
 │    - PCIe Configuration Space의 BAR0 레지스터에 물리 주소 할당  │
 │    - 예: BAR0 = 0xFB000000 (256MB 메모리 영역)                 │
 └───────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
 ┌────────────────────────────────────────────────────────────────┐
 │ 2. Linux PCI Enumeration (커널 부팅 시)                        │
 │    - pci_scan_root_bus() → pci_scan_bridge() → pci_scan_slot() │
 │    - struct pci_dev 생성, resource[0]에 BAR0 물리 주소 저장     │
 │    - pci_resource_start(pdev, 0) = 0xFB000000                  │
 │    - pci_resource_len(pdev, 0)   = BAR 크기                    │
 └───────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
 ┌────────────────────────────────────────────────────────────────┐
 │ 3. nvme_dev_map() → nvme_remap_bar()                           │
 │    - pci_request_mem_regions(): BAR0 리소스 예약                │
 │    - ioremap(pci_resource_start(pdev,0), size)                 │
 │      물리 0xFB000000 → 커널 가상 주소 0xFFFF_AAAA_BBBB_0000    │
 │    - dev->bar = 0xFFFF_AAAA_BBBB_0000  (가상 주소)             │
 └───────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
 ┌────────────────────────────────────────────────────────────────┐
 │ 4. dev->dbs 계산                                               │
 │    - dev->dbs = dev->bar + 0x1000                              │
 │    - dev->dbs = 0xFFFF_AAAA_BBBB_1000  (Doorbell 영역 시작)    │
 └───────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
 ┌────────────────────────────────────────────────────────────────┐
 │ 5. nvme_alloc_queue() → q_db 계산                              │
 │    - nvmeq->q_db = &dev->dbs[qid * 2 * db_stride]             │
 │                                                                │
 │    qid=0 (Admin): q_db = dbs + 0     = 0xFFFF_AAAA_BBBB_1000  │
 │    qid=1 (I/O 1): q_db = dbs + 8     = 0xFFFF_AAAA_BBBB_1008  │
 │    qid=2 (I/O 2): q_db = dbs + 16    = 0xFFFF_AAAA_BBBB_1010  │
 │    ...                                                         │
 │    (db_stride=1, sizeof(u32)=4이므로 &dbs[2] = dbs + 8)        │
 └───────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
 ┌────────────────────────────────────────────────────────────────┐
 │ 6. I/O 제출 시: writel(sq_tail, nvmeq->q_db)                   │
 │    - nvme_write_sq_db() 함수에서 호출                           │
 │    - writel()은 MMIO 쓰기: 커널 가상 주소 → MMU → 물리 주소    │
 │    - PCIe 버스를 통해 NVMe 컨트롤러의 Doorbell 레지스터에 도달  │
 │    - 컨트롤러가 SQ에서 새 커맨드를 fetch하기 시작               │
 └────────────────────────────────────────────────────────────────┘
```

### 5.2 BAR0 레이아웃 상세

```
 BAR0 (dev->bar)
 ┌──────────────────────────────────────────────────────────────┐
 │  +0x0000  CAP    (8B)  Controller Capabilities               │
 │  +0x0008  VS     (4B)  Version                               │
 │  +0x000C  INTMS  (4B)  Interrupt Mask Set                    │
 │  +0x0010  INTMC  (4B)  Interrupt Mask Clear                  │
 │  +0x0014  CC     (4B)  Controller Configuration  ★           │
 │  +0x001C  CSTS   (4B)  Controller Status          ★          │
 │  +0x0020  NSSR   (4B)  NVM Subsystem Reset                   │
 │  +0x0024  AQA    (4B)  Admin Queue Attributes     ★          │
 │  +0x0028  ASQ    (8B)  Admin SQ Base Address      ★          │
 │  +0x0030  ACQ    (8B)  Admin CQ Base Address      ★          │
 │  +0x0038  CMBLOC (4B)  CMB Location                          │
 │  +0x003C  CMBSZ  (4B)  CMB Size                             │
 │  ...                                                         │
 │  +0x0E00  BPMBL  (8B)  Boot Partition Memory Buffer          │
 │  +0x0E08~0x0FFF        예약 영역                              │
 ├──────────────────────────────────────────────────────────────┤
 │  +0x1000  (dev->dbs 시작)                                    │
 │  +0x1000  SQ0 Tail Doorbell (Admin SQ)   ← queues[0].q_db   │
 │  +0x1004  CQ0 Head Doorbell (Admin CQ)                       │
 │  +0x1008  SQ1 Tail Doorbell (I/O Q1)     ← queues[1].q_db   │
 │  +0x100C  CQ1 Head Doorbell (I/O Q1)                         │
 │  +0x1010  SQ2 Tail Doorbell (I/O Q2)     ← queues[2].q_db   │
 │  +0x1014  CQ2 Head Doorbell (I/O Q2)                         │
 │  ...      (큐 수만큼 반복)                                    │
 └──────────────────────────────────────────────────────────────┘

 Doorbell 주소 계산 공식:
   SQ y Tail DB = BAR0 + 0x1000 + (2y     * (4 << CAP.DSTRD))
   CQ y Head DB = BAR0 + 0x1000 + (2y + 1 * (4 << CAP.DSTRD))

 코드에서의 계산:
   q_db = &dev->dbs[qid * 2 * dev->db_stride]
   // dev->dbs = dev->bar + 0x1000   (u32 __iomem * 타입)
   // db_stride = 1 << CAP.DSTRD     (보통 1)
   // &dbs[qid*2*1] = dbs + qid*2*4 = BAR0 + 0x1000 + qid*8
```

### 5.3 Doorbell 쓰기의 물리적 경로

```
 writel(sq_tail, nvmeq->q_db)
     │
     ▼
 CPU: Store 명령 → Write Combining Buffer (비캐시 MMIO)
     │
     ▼
 MMU: 가상 주소 → 물리 주소 변환 (ioremap이 설정한 PTE)
     │         uncacheable, write-through 속성
     ▼
 메모리 컨트롤러 → PCIe Root Complex
     │
     ▼
 PCIe Transaction Layer Packet (TLP): Memory Write Request
     │   Address: BAR0 물리주소 + 0x1000 + qid*8
     │   Data: sq_tail (4바이트)
     │   Type: MWr (Memory Write, Posted → 응답 불필요)
     ▼
 PCIe 스위치 (있는 경우) → NVMe 컨트롤러
     │
     ▼
 NVMe 컨트롤러 내부 레지스터에 sq_tail 값이 기록됨
     │
     ▼
 컨트롤러의 커맨드 fetch 엔진이 SQ에서 새 커맨드를 DMA로 읽어감
```

### 5.4 SPDK와의 BAR 접근 비교

```
 커널 NVMe 드라이버:
   pci_enable_device_mem() → ioremap() → dev->bar
   → readl()/writel() (커널 공간 MMIO)
   → 인터럽트 기반 완료 처리

 SPDK:
   spdk_pci_device_map_bar() → mmap(/sys/bus/pci/.../resource0)
   → 사용자 공간에서 직접 MMIO 접근
   → 폴링 기반 완료 처리 (인터럽트 없음)

 핵심 차이:
   커널: ioremap + writel (커널 공간, 특권 모드)
   SPDK: mmap + volatile 포인터 (사용자 공간, UIO/VFIO)
   둘 다 같은 물리 주소(BAR0)에 접근하지만 경로가 다르다!
```

---

## 핵심 자료구조 관계도

```
 struct nvme_dev (pci.c 전용)
 ┌─────────────────────────────────────────────────┐
 │  bar          ──→ BAR0 MMIO 가상 주소            │
 │  dbs          ──→ bar + 0x1000 (Doorbell 시작)   │
 │  db_stride    ──→ Doorbell 간격 (보통 1)          │
 │  q_depth      ──→ I/O 큐 깊이 (보통 1024)        │
 │  queues[]     ──→ nvme_queue 배열 (Admin + I/O)   │
 │  tagset       ──→ I/O용 blk_mq_tag_set           │
 │  admin_tagset ──→ Admin용 blk_mq_tag_set          │
 │  ctrl         ──→ nvme_ctrl (core.c 공용)         │
 │  dbbuf_dbs    ──→ Shadow Doorbell 버퍼            │
 └─────────────────────────────────────────────────┘
          │
          │ contains
          ▼
 struct nvme_ctrl (core.c, 모든 transport 공용)
 ┌─────────────────────────────────────────────────┐
 │  state        ──→ NEW→CONNECTING→LIVE→...        │
 │  ops          ──→ nvme_pci_ctrl_ops              │
 │  cap          ──→ CAP 레지스터 값                  │
 │  ctrl_config  ──→ CC 레지스터 값 (소프트웨어 카피)  │
 │  tagset       ──→ I/O tag_set 포인터              │
 │  admin_q      ──→ Admin request_queue             │
 │  namespaces   ──→ nvme_ns 리스트                   │
 │  scan_work    ──→ 네임스페이스 스캔 워크            │
 │  reset_work   ──→ nvme_reset_work                 │
 │  instance     ──→ 컨트롤러 번호 (0, 1, ...)        │
 └─────────────────────────────────────────────────┘
          │
          │ namespaces list
          ▼
 struct nvme_ns (core.c)
 ┌─────────────────────────────────────────────────┐
 │  disk         ──→ gendisk (/dev/nvme0n1)         │
 │  queue        ──→ request_queue                   │
 │  ctrl         ──→ 소속 nvme_ctrl                  │
 │  head         ──→ nvme_ns_head (멀티패스 공유)     │
 └─────────────────────────────────────────────────┘

 struct nvme_queue (pci.c 전용, per-queue)
 ┌─────────────────────────────────────────────────┐
 │  sq_cmds      ──→ SQ 커맨드 버퍼 (DMA)           │
 │  cqes         ──→ CQ 엔트리 배열 (DMA)            │
 │  sq_dma_addr  ──→ SQ DMA 물리 주소                │
 │  cq_dma_addr  ──→ CQ DMA 물리 주소                │
 │  q_db         ──→ SQ Tail Doorbell MMIO 주소      │
 │  q_depth      ──→ 큐 깊이                         │
 │  sq_tail      ──→ 현재 SQ Tail 포인터              │
 │  cq_head      ──→ 현재 CQ Head 포인터              │
 │  cq_phase     ──→ 현재 Phase Tag (0 or 1)          │
 │  cq_vector    ──→ MSI-X 인터럽트 벡터 번호         │
 │  qid          ──→ 큐 ID (0=Admin)                  │
 │  sq_lock      ──→ SQ 접근 스핀락                   │
 └─────────────────────────────────────────────────┘
```

---

## 초기화 시퀀스 타임라인 요약

```
시간 →

1. module_init
   │  nvme_init()
   │  pci_register_driver(&nvme_driver)
   │
2. PCI 매칭
   │  pci_bus_match() → class=0x010802 확인
   │  pci_device_probe()
   │
3. nvme_probe() 시작
   │  ├─ nvme_pci_alloc_dev()        nvme_dev 메모리 할당
   │  │   └─ nvme_init_ctrl()        워크, 뮤텍스 초기화
   │  ├─ nvme_add_ctrl()             /dev/nvme0 chardev 생성
   │  ├─ nvme_dev_map()              BAR0 ioremap
   │  │   └─ nvme_remap_bar()        dev->bar, dev->dbs 설정
   │  ├─ nvme_pci_enable()
   │  │   ├─ pci_enable_device_mem() PCI 메모리 활성화
   │  │   ├─ pci_set_master()        Bus Master Enable
   │  │   ├─ pci_alloc_irq_vectors() IRQ 1개 할당
   │  │   ├─ CAP 읽기                MQES, DSTRD, TO 파악
   │  │   └─ nvme_pci_configure_admin_queue()
   │  │       ├─ nvme_disable_ctrl()    CC.EN=0 → CSTS.RDY=0 대기
   │  │       ├─ nvme_alloc_queue(0)    Admin SQ/CQ DMA 할당
   │  │       ├─ AQA/ASQ/ACQ 레지스터   DMA 주소를 컨트롤러에 전달
   │  │       ├─ nvme_enable_ctrl()     CC 구성 + CC.EN=1 → CSTS.RDY=1 대기
   │  │       ├─ nvme_init_queue()      SQ/CQ 포인터 리셋
   │  │       └─ queue_request_irq()    Admin IRQ 핸들러 등록
   │  │
   │  │  ← 이 시점부터 Admin 커맨드 전송 가능
   │  │
   │  ├─ nvme_init_ctrl_finish()     Identify Controller 실행
   │  ├─ nvme_setup_io_queues()
   │  │   ├─ nvme_set_queue_count()  I/O 큐 수 협상 (Admin cmd)
   │  │   ├─ nvme_remap_bar()        BAR 확장 (모든 Doorbell까지)
   │  │   ├─ nvme_setup_irqs()       MSI-X 벡터 할당
   │  │   └─ nvme_create_io_queues()
   │  │       ├─ nvme_alloc_queue(1..N)  각 I/O 큐 DMA 할당
   │  │       └─ nvme_create_queue()
   │  │           ├─ adapter_alloc_cq()  Create CQ (Admin cmd)
   │  │           ├─ adapter_alloc_sq()  Create SQ (Admin cmd)
   │  │           ├─ nvme_init_queue()   큐 상태 초기화
   │  │           └─ queue_request_irq() I/O IRQ 핸들러 등록
   │  │
   │  │  ← 이 시점부터 I/O 큐 사용 가능
   │  │
   │  ├─ nvme_alloc_io_tag_set()     blk-mq I/O tag_set 생성
   │  ├─ NVME_CTRL_LIVE 전환         I/O 수신 가능 상태
   │  └─ nvme_start_ctrl()
   │      └─ nvme_queue_scan()       → nvme_scan_work()
   │          └─ nvme_alloc_ns()     → device_add_disk()
   │
4. /dev/nvme0n1 사용 가능!
```
