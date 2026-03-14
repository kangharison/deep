# QEMU NVMe 디바이스 에뮬레이션 심층 분석

이 문서는 QEMU의 NVMe 컨트롤러 에뮬레이션 코드를 디바이스(하드웨어) 관점에서 분석한다. Guest OS 커널의 NVMe 드라이버가 "실제 하드웨어"라고 믿는 것의 정체가 바로 이 코드다.

소스 위치: `~/sources/qemu/hw/nvme/ctrl.c` (~9500줄), `hw/nvme/nvme.h`, `hw/nvme/ns.c`, `hw/nvme/subsys.c`, `include/block/nvme.h`

---

## 1. QEMU NVMe 디바이스 에뮬레이션 아키텍처

### 1.1 전체 구조 다이어그램

```
 ┌─────────────────────────────────────────────────────────┐
 │                    Guest OS (Linux)                     │
 │                                                         │
 │  ┌──────────────┐   ┌──────────────┐   ┌─────────────┐ │
 │  │   fio/app    │   │  filesystem  │   │  /dev/nvme0  │ │
 │  └──────┬───────┘   └──────┬───────┘   └──────┬──────┘ │
 │         └──────────────────┼──────────────────┘        │
 │                    ┌───────┴───────┐                    │
 │                    │   Block Layer │                    │
 │                    │   (blk-mq)    │                    │
 │                    └───────┬───────┘                    │
 │                    ┌───────┴───────┐                    │
 │                    │  NVMe Driver  │                    │
 │                    │  (nvme-core)  │                    │
 │                    └───────┬───────┘                    │
 │                            │                            │
 │  ┌─────────────────────────┴─────────────────────────┐  │
 │  │  writel(val, doorbell_addr)  ← MMIO Write to BAR0 │  │
 │  │  readl(bar0 + offset)       ← MMIO Read from BAR0 │  │
 │  └─────────────────────────┬─────────────────────────┘  │
 │                            │                            │
 ├════════════════════════════╪════════════════════════════╡
 │                     KVM / QEMU                         │
 │                            │                            │
 │  ┌─────────────────────────┴─────────────────────────┐  │
 │  │         VM-Exit  (EPT violation / MMIO trap)       │  │
 │  │              KVM → QEMU userspace                  │  │
 │  └─────────────────────────┬─────────────────────────┘  │
 │                            │                            │
 │  ┌─────────────────────────┴─────────────────────────┐  │
 │  │          QEMU NVMe Device Emulation               │  │
 │  │                                                     │  │
 │  │  nvme_mmio_write()                                 │  │
 │  │    ├─ addr < sizeof(NvmeBar) → nvme_write_bar()   │  │
 │  │    │    ├─ CC 쓰기 → nvme_start_ctrl() 컨트롤러ON │  │
 │  │    │    ├─ AQA/ASQ/ACQ 쓰기 → Admin Queue 설정    │  │
 │  │    │    └─ INTMS/INTMC → 인터럽트 마스크 설정      │  │
 │  │    └─ addr >= 0x1000 → nvme_process_db()          │  │
 │  │         ├─ SQ Doorbell → sq->tail 갱신            │  │
 │  │         │    → nvme_process_sq() → 명령어 fetch    │  │
 │  │         └─ CQ Doorbell → cq->head 갱신            │  │
 │  │              → CQ 슬롯 해제, 인터럽트 해제         │  │
 │  │                                                     │  │
 │  │  nvme_process_sq()                                 │  │
 │  │    ├─ DMA read: guest SQ에서 NvmeCmd 64B 읽기     │  │
 │  │    ├─ nvme_io_cmd() / nvme_admin_cmd()             │  │
 │  │    │    ├─ nvme_read() → blk_aio_preadv()          │  │
 │  │    │    ├─ nvme_write() → blk_aio_pwritev()        │  │
 │  │    │    └─ nvme_create_cq/sq(), nvme_identify()... │  │
 │  │    └─ Completion → nvme_post_cqes()                │  │
 │  │         ├─ DMA write: CQE 16B를 guest CQ에 기록   │  │
 │  │         └─ msix_notify() → guest 인터럽트 발생     │  │
 │  └─────────────────────────┬─────────────────────────┘  │
 │                            │                            │
 │  ┌─────────────────────────┴─────────────────────────┐  │
 │  │         QEMU Block Layer (blk_aio_*)               │  │
 │  │         → Host Filesystem / Block Device           │  │
 │  │         → 실제 qcow2 이미지 / raw 파일 I/O        │  │
 │  └───────────────────────────────────────────────────┘  │
 └─────────────────────────────────────────────────────────┘
```

### 1.2 PCIe 디바이스 에뮬레이션의 기본 원리

QEMU에서 NVMe SSD는 **가상 PCIe 디바이스**로 에뮬레이션된다. 핵심 메커니즘은 다음과 같다.

**MMIO Trap 원리:**
1. QEMU가 `memory_region_init_io(&n->iomem, ..., &nvme_mmio_ops, n, "nvme", bar_size)`로 MMIO 영역을 등록한다.
2. 이 MemoryRegion이 PCIe BAR0에 매핑된다: `pci_register_bar(pci_dev, 0, ..., &n->bar0)`
3. Guest OS가 BAR0 주소에 `writel()`을 실행하면, 해당 물리 주소가 EPT(Extended Page Table)에 매핑되어 있지 않으므로 **VM-Exit**가 발생한다.
4. KVM이 이 MMIO 접근을 QEMU 유저스페이스로 전달한다.
5. QEMU의 메모리 서브시스템이 등록된 `nvme_mmio_ops`의 `.read` 또는 `.write` 핸들러를 호출한다.

**MemoryRegionOps 등록 (ctrl.c 8535-8543행):**
```c
static const MemoryRegionOps nvme_mmio_ops = {
    .read = nvme_mmio_read,     // Guest가 BAR0 읽을 때
    .write = nvme_mmio_write,   // Guest가 BAR0에 쓸 때
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 2,   // 최소 2바이트 접근
        .max_access_size = 8,   // 최대 8바이트 접근 (64비트 레지스터용)
    },
};
```

### 1.3 NvmeCtrl, NvmeNamespace, NvmeSubsystem 클래스 관계도

```
                      ┌──────────────────┐
                      │  NvmeSubsystem   │
                      │  (nvme-subsys)   │
                      │                  │
                      │  subnqn[256]     │
                      │  serial          │
                      │  endgrp          │ ─── NvmeEnduranceGroup (FDP)
                      │  ctrls[256]  ────┼──┐
                      │  namespaces[257] │  │
                      └────────┬─────────┘  │
                               │            │
              ┌────────────────┘            │
              │ (1:N)                       │ (subsys→ctrls[i])
              ▼                             │
     ┌────────────────────┐                 │
     │    NvmeCtrl        │ ◄───────────────┘
     │    (nvme)          │
     │                    │
     │  PCIDevice         │ ← QEMU PCIe 디바이스 상속
     │  bar0 (MemReg)     │ ← BAR0 전체 (MMIO + MSI-X)
     │  iomem (MemReg)    │ ← MMIO 핸들러 영역
     │  bar (NvmeBar)     │ ← 레지스터 상태 (CAP, VS, CC, CSTS, AQA, ASQ, ACQ ...)
     │  params            │ ← QEMU 커맨드라인 파라미터
     │  sq[N], cq[N]      │ ← I/O SQ/CQ 포인터 배열
     │  admin_sq, admin_cq│ ← Admin 큐 (인라인)
     │  namespaces[257]   │ ← 이 컨트롤러에 attach된 NS
     │  id_ctrl           │ ← Identify Controller 데이터
     │  dbbuf_dbs/eis     │ ← Shadow Doorbell 주소
     └───────┬────────────┘
             │ (1:N)
             ▼
     ┌────────────────────┐
     │   NvmeNamespace    │
     │   (nvme-ns)        │
     │                    │
     │  DeviceState       │ ← QEMU 디바이스 상속
     │  blkconf (BlockConf)│← 백엔드 블록 디바이스 설정
     │  size              │ ← 네임스페이스 크기
     │  id_ns (NvmeIdNs)  │ ← Identify Namespace 데이터
     │  lbaf (NvmeLBAF)   │ ← 현재 LBA 포맷
     │  lbasz             │ ← LBA 크기 (바이트)
     │  params            │ ← nsid, zoned, ms, pi 등
     │  zone_array[]      │ ← ZNS 존 배열 (zoned일 때)
     │  ctrl              │ ← private NS이면 특정 컨트롤러
     │  subsys            │ ← 소속 서브시스템
     └────────────────────┘
```

**핵심 관계:**
- `NvmeSubsystem`은 최대 256개의 컨트롤러와 256개의 네임스페이스를 관리한다.
- `NvmeCtrl`는 `PCIDevice`를 상속하는 가상 PCIe 디바이스다.
- `NvmeNamespace`는 실제 블록 스토리지(qcow2 파일 등)를 NVMe 네임스페이스로 노출한다.
- `shared=true`인 네임스페이스는 여러 컨트롤러에 attach될 수 있다.

---

## 2. PCIe BAR/레지스터 에뮬레이션 (Host와의 연결고리!)

### 2.1 BAR0 MMIO 영역 등록

QEMU는 `nvme_init_pci()` (ctrl.c 8940행)에서 BAR0를 설정한다.

```c
// BAR0 전체 크기 계산: NvmeBar(레지스터) + 도어벨 + MSI-X 테이블/PBA
bar_size = nvme_mbar_size(n->params.max_ioqpairs + 1,
                          nr_vectors, &msix_table_offset, &msix_pba_offset);

// BAR0 컨테이너 MemoryRegion 생성
memory_region_init(&n->bar0, OBJECT(n), "nvme-bar0", bar_size);

// MMIO 핸들러가 붙은 서브영역 (오프셋 0부터 MSI-X 테이블 시작 전까지)
memory_region_init_io(&n->iomem, OBJECT(n), &nvme_mmio_ops, n, "nvme",
                      msix_table_offset);
memory_region_add_subregion(&n->bar0, 0, &n->iomem);

// PCIe BAR0으로 등록 (64비트 메모리 BAR)
pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY |
                 PCI_BASE_ADDRESS_MEM_TYPE_64, &n->bar0);
```

BAR0의 메모리 레이아웃:
```
 BAR0 오프셋
 ┌─────────────────────────────┐ 0x0000
 │   NvmeBar 레지스터          │
 │   (CAP, VS, CC, CSTS, ...)  │
 │   sizeof(NvmeBar) ≈ 0x1000  │
 ├─────────────────────────────┤ 0x1000
 │   Doorbell 레지스터 영역     │
 │   SQ0 Tail DB (0x1000)      │
 │   CQ0 Head DB (0x1004)      │
 │   SQ1 Tail DB (0x1008)      │
 │   CQ1 Head DB (0x100C)      │
 │   ...                       │
 │   각 4바이트, SQ/CQ 쌍마다 8B│
 ├─────────────────────────────┤ (4K 정렬)
 │   MSI-X Table               │
 ├─────────────────────────────┤ (4K 정렬)
 │   MSI-X PBA                 │
 └─────────────────────────────┘
```

### 2.2 nvme_mmio_read() 함수 분석

Guest가 BAR0에서 레지스터를 읽을 때 호출된다 (ctrl.c 8323행).

```c
static uint64_t nvme_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    NvmeCtrl *n = (NvmeCtrl *)opaque;
    uint8_t *ptr = (uint8_t *)&n->bar;   // NvmeBar 구조체를 바이트 배열로 접근

    // 정렬 검사: NVMe 스펙상 32비트 정렬 필수
    if (unlikely(addr & (sizeof(uint32_t) - 1))) { ... }
    if (unlikely(size < sizeof(uint32_t))) { ... }

    // 범위 검사: BAR 레지스터 영역을 넘어서면 0 반환
    if (addr > sizeof(n->bar) - size) { return 0; }

    // VF offline 체크 (SR-IOV)
    if (pci_is_vf(PCI_DEVICE(n)) && !nvme_sctrl(n)->scs && addr != NVME_REG_CSTS) {
        return 0;
    }

    // PMRSTS 읽기 시 PMR 동기화 (데이터 영속성 보장)
    if (addr == NVME_REG_PMRSTS && ...) {
        memory_region_msync(&n->pmr.dev->mr, 0, n->pmr.dev->size);
    }

    // n->bar 구조체에서 해당 오프셋의 값을 리틀 엔디안으로 읽어서 반환
    return ldn_le_p(ptr + addr, size);
}
```

**핵심:** Guest가 `readl(bar0 + NVME_REG_CSTS)` 하면, QEMU는 `n->bar.csts`의 현재 값을 그대로 반환한다. 이 값은 `nvme_write_bar()`에서 CC 레지스터 처리 시 갱신된다.

### 2.3 nvme_mmio_write() 함수 라인바이라인 분석

Guest가 BAR0에 쓸 때 호출되는 최상위 핸들러 (ctrl.c 8515행).

```c
static void nvme_mmio_write(void *opaque, hwaddr addr, uint64_t data,
                            unsigned size)
{
    NvmeCtrl *n = (NvmeCtrl *)opaque;

    trace_pci_nvme_mmio_write(addr, data, size);
    // → 디버그 트레이싱: "addr=0x1008, data=5, size=4" 같은 로그

    // SR-IOV VF가 offline이면 CSTS 외 모든 쓰기 무시
    if (pci_is_vf(PCI_DEVICE(n)) && !nvme_sctrl(n)->scs &&
        addr != NVME_REG_CSTS) {
        return;
    }

    if (addr < sizeof(n->bar)) {
        // ★ 레지스터 영역 쓰기 (CAP, CC, AQA, ASQ, ACQ 등)
        nvme_write_bar(n, addr, data, size);
    } else {
        // ★ 도어벨 영역 쓰기 (오프셋 0x1000 이상)
        nvme_process_db(n, addr, data);
    }
}
```

**결정적 분기점:**
- `addr < sizeof(NvmeBar)` → 컨트롤러 레지스터 쓰기 (CC, AQA 등)
- `addr >= sizeof(NvmeBar)` (= 0x1000 이상) → 도어벨 쓰기 → 명령어 처리 시작!

### 2.4 nvme_write_bar() 함수 라인바이라인 분석

컨트롤러 설정 레지스터에 대한 쓰기를 처리한다 (ctrl.c 8085행).

```c
static void nvme_write_bar(NvmeCtrl *n, hwaddr offset, uint64_t data,
                           unsigned size)
{
    PCIDevice *pci = PCI_DEVICE(n);
    uint64_t cap = ldq_le_p(&n->bar.cap);     // 현재 CAP 레지스터 값
    uint32_t cc = ldl_le_p(&n->bar.cc);        // 현재 CC (Controller Configuration)
    uint32_t intms = ldl_le_p(&n->bar.intms);  // 현재 인터럽트 마스크
    uint32_t csts = ldl_le_p(&n->bar.csts);    // 현재 CSTS (Controller Status)

    switch (offset) {

    case NVME_REG_INTMS:    // 오프셋 0x0C: Interrupt Mask Set
        // Guest가 인터럽트를 마스킹하려고 비트를 세팅
        intms |= data;
        stl_le_p(&n->bar.intms, intms);
        n->bar.intmc = n->bar.intms;
        nvme_irq_check(n);  // 마스킹 변경 후 인터럽트 상태 재평가
        break;

    case NVME_REG_INTMC:    // 오프셋 0x10: Interrupt Mask Clear
        intms &= ~data;     // 해당 비트 클리어
        stl_le_p(&n->bar.intms, intms);
        n->bar.intmc = n->bar.intms;
        nvme_irq_check(n);
        break;

    case NVME_REG_CC:       // ★ 오프셋 0x14: Controller Configuration
        stl_le_p(&n->bar.cc, data);  // 새 CC 값 저장

        // ── Shutdown Notification 처리 ──
        if (NVME_CC_SHN(data) && !(NVME_CC_SHN(cc))) {
            // SHN 비트가 0→1: 셧다운 시작
            nvme_ctrl_shutdown(n);   // 모든 NS flush
            csts |= NVME_CSTS_SHST_COMPLETE;  // 셧다운 완료 표시
        } else if (!NVME_CC_SHN(data) && NVME_CC_SHN(cc)) {
            // SHN 비트가 1→0: 셧다운 해제
            csts &= ~(CSTS_SHST_MASK << CSTS_SHST_SHIFT);
        }

        // ── Enable/Disable 처리 ──
        if (NVME_CC_EN(data) && !NVME_CC_EN(cc)) {
            // ★ EN 비트가 0→1: 컨트롤러 활성화!
            if (unlikely(nvme_start_ctrl(n))) {
                csts = NVME_CSTS_FAILED;  // 시작 실패
            } else {
                csts = NVME_CSTS_READY;   // 시작 성공 → CSTS.RDY=1
            }
        } else if (!NVME_CC_EN(data) && NVME_CC_EN(cc)) {
            // EN 비트가 1→0: 컨트롤러 비활성화
            nvme_ctrl_reset(n, NVME_RESET_CONTROLLER);
            break;
        }

        stl_le_p(&n->bar.csts, csts);  // CSTS 업데이트
        break;

    case NVME_REG_CSTS:     // 오프셋 0x1C: Controller Status (읽기 전용)
        // 스펙: CSTS는 Read-Only. 쓰기 시도 시 에러 로그
        break;

    case NVME_REG_AQA:      // 오프셋 0x24: Admin Queue Attributes
        // ASQS(Admin SQ Size)와 ACQS(Admin CQ Size) 설정
        stl_le_p(&n->bar.aqa, data);
        break;

    case NVME_REG_ASQ:      // 오프셋 0x28: Admin SQ Base Address (하위 32비트)
        stn_le_p(&n->bar.asq, size, data);
        break;

    case NVME_REG_ASQ + 4:  // 오프셋 0x2C: Admin SQ Base Address (상위 32비트)
        stl_le_p((uint8_t *)&n->bar.asq + 4, data);
        break;

    case NVME_REG_ACQ:      // 오프셋 0x30: Admin CQ Base Address (하위 32비트)
        stn_le_p(&n->bar.acq, size, data);
        break;

    case NVME_REG_ACQ + 4:  // 오프셋 0x34: Admin CQ Base Address (상위 32비트)
        stl_le_p((uint8_t *)&n->bar.acq + 4, data);
        break;

    // ... CMB, PMR 관련 레지스터 처리 생략 ...
    }
}
```

**컨트롤러 시작 시퀀스 (Guest 드라이버 관점):**
1. `writel(aqa_val, bar0 + 0x24)` → AQA 설정 (Admin SQ/CQ 크기)
2. `writeq(asq_dma, bar0 + 0x28)` → ASQ 베이스 주소 설정
3. `writeq(acq_dma, bar0 + 0x30)` → ACQ 베이스 주소 설정
4. `writel(cc_val, bar0 + 0x14)` → CC.EN=1 → `nvme_start_ctrl()` 호출!
5. Guest가 `readl(bar0 + 0x1C)` 폴링 → CSTS.RDY=1 확인

### 2.5 nvme_start_ctrl(): 컨트롤러 활성화

CC.EN이 0→1로 바뀔 때 호출된다 (ctrl.c 7979행).

```c
static int nvme_start_ctrl(NvmeCtrl *n)
{
    uint32_t cc = ldl_le_p(&n->bar.cc);
    uint32_t aqa = ldl_le_p(&n->bar.aqa);
    uint64_t asq = ldq_le_p(&n->bar.asq);   // Guest가 설정한 Admin SQ 물리주소
    uint64_t acq = ldq_le_p(&n->bar.acq);   // Guest가 설정한 Admin CQ 물리주소
    uint32_t page_bits = NVME_CC_MPS(cc) + 12;
    uint32_t page_size = 1 << page_bits;

    // 다양한 유효성 검사: 이미 큐가 있는지, 주소 정렬, CSS 지원, 페이지 크기 등
    if (unlikely(asq & (page_size - 1))) { return -1; }  // 페이지 정렬 필수
    if (unlikely(acq & (page_size - 1))) { return -1; }

    n->page_bits = page_bits;
    n->page_size = page_size;
    n->max_prp_ents = n->page_size / sizeof(uint64_t);

    // ★ Admin CQ 초기화: guest 메모리의 ACQ를 DMA 주소로 사용
    nvme_init_cq(&n->admin_cq, n, acq, 0, 0, NVME_AQA_ACQS(aqa) + 1, 1);

    // ★ Admin SQ 초기화: guest 메모리의 ASQ를 DMA 주소로 사용
    nvme_init_sq(&n->admin_sq, n, asq, 0, 0, NVME_AQA_ASQS(aqa) + 1);

    // 네임스페이스 attach
    for (int i = 1; i <= NVME_MAX_NAMESPACES; i++) {
        NvmeNamespace *ns = nvme_subsys_ns(n->subsys, i);
        if (ns && nvme_csi_supported(n, ns->csi) && !ns->params.detached) {
            nvme_attach_ns(n, ns);
        }
    }
    return 0;  // 성공 → CSTS.RDY=1이 된다
}
```

---

## 3. Doorbell 처리 (Guest writel → QEMU 처리)

### 3.1 전체 흐름

```
Guest 커널: writel(new_tail, bar0 + 0x1008)    ← SQ1 Tail Doorbell에 5 쓰기
    │
    ▼
KVM:        VM-Exit (EPT violation, MMIO write)
    │
    ▼
QEMU:       nvme_mmio_write(opaque, addr=0x1008, data=5, size=4)
    │
    ├─ addr(0x1008) >= sizeof(NvmeBar) → 도어벨 영역!
    │
    ▼
            nvme_process_db(n, addr=0x1008, val=5)
    │
    ├─ (0x1008 - 0x1000) >> 2 = 2, (2 & 1) = 0 → SQ Doorbell!
    ├─ qid = (0x1008 - 0x1000) >> 3 = 1  → SQ1
    ├─ sq->tail = 5
    │
    ▼
            qemu_bh_schedule(sq->bh)  → 곧 nvme_process_sq(sq) 호출
    │
    ▼
            nvme_process_sq(sq):
              while (sq->head != sq->tail) {
                  // SQ에서 명령어 fetch (DMA read)
                  addr = sq->dma_addr + (sq->head << 6);  // SQE = 64바이트
                  nvme_addr_read(n, addr, &cmd, 64);
                  sq->head++;
                  // 명령어 디스패치
                  status = nvme_io_cmd(n, req);
              }
```

### 3.2 nvme_process_db() 라인바이라인 분석

도어벨 쓰기를 처리하는 핵심 함수 (ctrl.c 8369행).

```c
static void nvme_process_db(NvmeCtrl *n, hwaddr addr, int val)
{
    PCIDevice *pci = PCI_DEVICE(n);
    uint32_t qid;

    // ── 정렬 검사 ──
    // 도어벨은 4바이트 정렬이어야 한다 (NVMe 스펙)
    if (unlikely(addr & ((1 << 2) - 1))) {
        // 비정렬 쓰기 → 무시하고 로그
        return;
    }

    // ── SQ vs CQ 도어벨 구분 ──
    // 오프셋 0x1000부터 시작, 4바이트 단위로 SQ/CQ가 교대:
    //   0x1000: SQ0 Tail    (bit 1 of ((addr-0x1000)>>2) = 0)
    //   0x1004: CQ0 Head    (bit 1 of ((addr-0x1000)>>2) = 1)
    //   0x1008: SQ1 Tail    (bit 1 = 0)
    //   0x100C: CQ1 Head    (bit 1 = 1)

    if (((addr - 0x1000) >> 2) & 1) {
        // ═══════════════════════════════════════════
        // ★ CQ Head Doorbell 쓰기
        // ═══════════════════════════════════════════
        // Guest가 CQE를 소비한 후 Head를 업데이트
        uint16_t new_head = val & 0xffff;
        NvmeCQueue *cq;

        // qid 계산: (addr - 0x1004) / 8
        qid = (addr - (0x1000 + (1 << 2))) >> 3;

        // CQ 존재 여부 확인
        if (unlikely(nvme_check_cqid(n, qid))) {
            // 존재하지 않는 CQ에 대한 도어벨 → AER 발생 가능
            if (n->outstanding_aers) {
                nvme_enqueue_event(n, NVME_AER_TYPE_ERROR,
                                   NVME_AER_INFO_ERR_INVALID_DB_REGISTER,
                                   NVME_LOG_ERROR_INFO);
            }
            return;
        }

        cq = n->cq[qid];

        // new_head 범위 검사
        if (unlikely(new_head >= cq->size)) {
            // 큐 크기를 초과하는 head 값 → 무시 + AER
            if (n->outstanding_aers) {
                nvme_enqueue_event(n, NVME_AER_TYPE_ERROR,
                                   NVME_AER_INFO_ERR_INVALID_DB_VALUE,
                                   NVME_LOG_ERROR_INFO);
            }
            return;
        }

        // ★ CQ가 이전에 가득 차 있었다면, 이제 공간이 생겼으므로
        //   대기 중인 CQE를 포스팅할 수 있다
        if (nvme_cq_full(cq)) {
            qemu_bh_schedule(cq->bh);  // nvme_post_cqes() 스케줄
        }

        cq->head = new_head;  // ★ CQ Head 갱신 → 슬롯 재사용 가능

        // Shadow Doorbell: Admin CQ인 경우 db_addr에도 반영
        if (!qid && n->dbbuf_enabled) {
            stl_le_pci_dma(pci, cq->db_addr, cq->head, MEMTXATTRS_UNSPECIFIED);
        }

        // CQ가 비었으면 (tail == head) 인터럽트 해제
        if (cq->tail == cq->head) {
            if (cq->irq_enabled) {
                n->cq_pending--;
            }
            nvme_irq_deassert(n, cq);
        }

    } else {
        // ═══════════════════════════════════════════
        // ★ SQ Tail Doorbell 쓰기
        // ═══════════════════════════════════════════
        // Guest가 새 명령어를 SQ에 넣은 후 Tail을 업데이트
        uint16_t new_tail = val & 0xffff;
        NvmeSQueue *sq;

        // qid 계산: (addr - 0x1000) / 8
        qid = (addr - 0x1000) >> 3;

        // SQ 존재 여부 확인
        if (unlikely(nvme_check_sqid(n, qid))) {
            if (n->outstanding_aers) {
                nvme_enqueue_event(n, NVME_AER_TYPE_ERROR,
                                   NVME_AER_INFO_ERR_INVALID_DB_REGISTER,
                                   NVME_LOG_ERROR_INFO);
            }
            return;
        }

        sq = n->sq[qid];

        // new_tail 범위 검사
        if (unlikely(new_tail >= sq->size)) {
            if (n->outstanding_aers) {
                nvme_enqueue_event(n, NVME_AER_TYPE_ERROR,
                                   NVME_AER_INFO_ERR_INVALID_DB_VALUE,
                                   NVME_LOG_ERROR_INFO);
            }
            return;
        }

        sq->tail = new_tail;  // ★ SQ Tail 갱신

        // Shadow Doorbell: Admin SQ인 경우 동기화
        if (!qid && n->dbbuf_enabled) {
            stl_le_pci_dma(pci, sq->db_addr, sq->tail, MEMTXATTRS_UNSPECIFIED);
        }

        // ★ 핵심: SQ 처리 함수 스케줄 → 명령어 fetch & 실행
        qemu_bh_schedule(sq->bh);
        // sq->bh는 nvme_init_sq()에서 nvme_process_sq에 바인딩됨
    }
}
```

### 3.3 Shadow Doorbell 에뮬레이션

Shadow Doorbell Buffer는 NVMe 1.3의 최적화 기능으로, VM-Exit 횟수를 줄인다.

**원리:**
- 호스트(Guest)가 Shadow Doorbell Buffer(메모리)에 도어벨 값을 쓰고, QEMU가 `ioeventfd`를 통해 이를 감지한다.
- Admin 명령 `NVME_ADM_CMD_DBBUF_CONFIG`로 Shadow DB 주소를 설정한다.
- 이후 I/O 큐의 도어벨은 `ioeventfd`로 처리되어, 매번 VM-Exit 없이 QEMU에 통지된다.

```c
// nvme_init_sq()에서 Shadow DB 설정 (ctrl.c 4878행):
if (n->dbbuf_enabled) {
    sq->db_addr = n->dbbuf_dbs + (sqid << 3);    // Shadow DB 주소
    sq->ei_addr = n->dbbuf_eis + (sqid << 3);    // EventIdx 주소

    if (n->params.ioeventfd && sq->sqid != 0) {
        // ioeventfd 등록: 해당 도어벨 오프셋에 쓰기가 오면 eventfd 시그널
        if (!nvme_init_sq_ioeventfd(sq)) {
            sq->ioeventfd_enabled = true;
        }
    }
}
```

```c
// ioeventfd 핸들러 (ctrl.c 4765행):
static void nvme_sq_notifier(EventNotifier *e)
{
    NvmeSQueue *sq = container_of(e, NvmeSQueue, notifier);
    if (!event_notifier_test_and_clear(e)) { return; }
    nvme_process_sq(sq);  // VM-Exit 없이 바로 SQ 처리
}
```

---

## 4. NVMe 커맨드 처리 흐름

### 4.1 SQ에서 명령어 Fetch: nvme_process_sq()

도어벨 처리 후 호출되어 SQ에서 명령어를 읽고 실행한다 (ctrl.c 7777행).

```c
static void nvme_process_sq(void *opaque)
{
    NvmeSQueue *sq = opaque;
    NvmeCtrl *n = sq->ctrl;
    NvmeCQueue *cq = n->cq[sq->cqid];  // 이 SQ에 연결된 CQ

    uint16_t status;
    hwaddr addr;
    NvmeCmd cmd;
    NvmeRequest *req;

    // Shadow DB 모드: DMA로 최신 tail 값 읽기
    if (n->dbbuf_enabled) {
        nvme_update_sq_tail(sq);
    }

    // ★ 메인 루프: SQ에 처리할 명령어가 있고, 사용 가능한 request 슬롯이 있는 동안 반복
    while (!(nvme_sq_empty(sq) || QTAILQ_EMPTY(&sq->req_list))) {

        // ── SQE DMA Read ──
        // Guest 메모리에서 SQ Entry 64바이트를 읽는다
        addr = sq->dma_addr + (sq->head << NVME_SQES);
        //                      head * 64 (NVME_SQES=6, 1<<6=64)
        if (nvme_addr_read(n, addr, (void *)&cmd, sizeof(cmd))) {
            // DMA 읽기 실패 → 치명적 에러 → CSTS.CFS=1
            stl_le_p(&n->bar.csts, NVME_CSTS_FAILED);
            break;
        }

        // Atomic write 검사 (생략)
        // ...

        // ── SQ Head 증가 ──
        nvme_inc_sq_head(sq);
        // sq->head = (sq->head + 1) % sq->size;

        // ── Request 슬롯 할당 ──
        req = QTAILQ_FIRST(&sq->req_list);     // 빈 request 가져오기
        QTAILQ_REMOVE(&sq->req_list, req, entry);
        QTAILQ_INSERT_TAIL(&sq->out_req_list, req, entry);  // 진행 중 목록에 추가
        nvme_req_clear(req);                     // request 초기화
        req->cqe.cid = cmd.cid;                  // CID 복사 (응답에 사용)
        memcpy(&req->cmd, &cmd, sizeof(NvmeCmd)); // 명령어 전체 복사

        // ★ 명령어 디스패치 ──
        // sqid가 0이면 Admin 명령, 아니면 I/O 명령
        status = sq->sqid ? nvme_io_cmd(n, req) :
            nvme_admin_cmd(n, req);

        // 즉시 완료되는 명령어는 바로 CQE 생성
        if (status != NVME_NO_COMPLETE) {
            req->status = status;
            nvme_enqueue_req_completion(cq, req);
        }
        // NVME_NO_COMPLETE: 비동기 I/O 진행 중 → 나중에 콜백에서 완료 처리

        // Shadow DB 모드: EventIdx 갱신 & tail 재확인
        if (n->dbbuf_enabled) {
            nvme_update_sq_eventidx(sq);
            nvme_update_sq_tail(sq);
        }
    }
}
```

### 4.2 I/O 명령어 디스패치: nvme_io_cmd()

```c
static uint16_t nvme_io_cmd(NvmeCtrl *n, NvmeRequest *req)
{
    NvmeNamespace *ns;
    uint32_t nsid = le32_to_cpu(req->cmd.nsid);

    // Flush는 broadcast NSID 가능 → 별도 처리
    if (req->cmd.opcode == NVME_CMD_FLUSH) {
        return nvme_flush(n, req);
    }

    // NSID 유효성 검사
    if (!nvme_nsid_valid(n, nsid) || nsid == NVME_NSID_BROADCAST) {
        return NVME_INVALID_NSID | NVME_DNR;
    }

    ns = nvme_ns(n, nsid);
    if (unlikely(!ns)) { return NVME_INVALID_FIELD | NVME_DNR; }

    req->ns = ns;

    // Namespace의 Command Set에 따라 분기
    switch (ns->csi) {
    case NVME_CSI_NVM:
        return nvme_io_cmd_nvm(n, req);   // NVM Command Set
    case NVME_CSI_ZONED:
        return nvme_io_cmd_zoned(n, req); // Zoned Command Set
    }
}
```

NVM Command Set 내부 디스패치:
```c
static uint16_t __nvme_io_cmd_nvm(NvmeCtrl *n, NvmeRequest *req)
{
    switch (req->cmd.opcode) {
    case NVME_CMD_WRITE:        return nvme_write(n, req);
    case NVME_CMD_READ:         return nvme_read(n, req);
    case NVME_CMD_COMPARE:      return nvme_compare(n, req);
    case NVME_CMD_WRITE_ZEROES: return nvme_write_zeroes(n, req);
    case NVME_CMD_DSM:          return nvme_dsm(n, req);
    case NVME_CMD_VERIFY:       return nvme_verify(n, req);
    case NVME_CMD_COPY:         return nvme_copy(n, req);
    case NVME_CMD_IO_MGMT_RECV: return nvme_io_mgmt_recv(n, req);
    case NVME_CMD_IO_MGMT_SEND: return nvme_io_mgmt_send(n, req);
    }
}
```

### 4.3 nvme_read() 라인바이라인 분석

NVMe Read 명령어 처리 (ctrl.c 3607행).

```c
static uint16_t nvme_read(NvmeCtrl *n, NvmeRequest *req)
{
    NvmeRwCmd *rw = (NvmeRwCmd *)&req->cmd;   // 64B SQE를 NvmeRwCmd로 캐스팅
    NvmeNamespace *ns = req->ns;

    // ── SQE에서 파라미터 추출 ──
    uint64_t slba = le64_to_cpu(rw->slba);    // Start LBA
    uint32_t nlb = (uint32_t)le16_to_cpu(rw->nlb) + 1;  // Number of LBAs (0-based → +1)
    uint8_t prinfo = NVME_RW_PRINFO(le16_to_cpu(rw->control));
    uint64_t data_size = nvme_l2b(ns, nlb);   // LBA 개수 → 바이트 변환
    //                    = nlb << ns->lbaf.ds  (예: nlb * 4096)
    uint64_t mapped_size = data_size;
    uint64_t data_offset;
    BlockBackend *blk = ns->blkconf.blk;      // 백엔드 블록 디바이스
    uint16_t status;

    // Extended LBA + Metadata 크기 조정
    if (nvme_ns_ext(ns) && !(NVME_ID_CTRL_CTRATT_MEM(n->id_ctrl.ctratt))) {
        mapped_size += nvme_m2b(ns, nlb);
        // PI + PRACT일 때 metadata 제외 가능
    }

    // ── MDTS 검사 ──
    // 한 번에 전송할 수 있는 최대 데이터 크기 체크
    status = nvme_check_mdts(n, mapped_size);
    if (status) { goto invalid; }

    // ── LBA 범위 검사 ──
    status = nvme_check_bounds(ns, slba, nlb);
    if (status) { goto invalid; }

    // ── Zoned NS인 경우 존 상태 검사 ──
    if (ns->params.zoned) {
        status = nvme_check_zone_read(ns, slba, nlb);
        if (status) { goto invalid; }
    }

    // ── DULBE 검사 (Deallocated/Unwritten Logical Block Error) ──
    if (NVME_ERR_REC_DULBE(ns->features.err_rec)) {
        status = nvme_check_dulbe(ns, slba, nlb);
        if (status) { goto invalid; }
    }

    // ── Protection Information이 있으면 DIF 경로 ──
    if (NVME_ID_NS_DPS_TYPE(ns->id_ns.dps)) {
        return nvme_dif_rw(n, req);
    }

    // ── PRP/SGL로 Guest 메모리 매핑 ──
    status = nvme_map_data(n, nlb, req);
    // → nvme_map_dptr() → nvme_map_prp() 또는 nvme_map_sgl()
    // → Guest의 DMA 주소를 scatter-gather 리스트로 변환
    if (status) { goto invalid; }

    // ── 실제 I/O 수행 ──
    data_offset = nvme_l2b(ns, slba);  // LBA → 바이트 오프셋

    block_acct_start(blk_get_stats(blk), &req->acct, data_size, BLOCK_ACCT_READ);

    // ★ QEMU 블록 레이어를 통한 비동기 읽기
    nvme_blk_read(blk, data_offset, BDRV_SECTOR_SIZE, nvme_rw_cb, req);
    // → 내부적으로:
    //   if (req->sg.flags & NVME_SG_DMA) {
    //       req->aiocb = dma_blk_read(blk, &req->sg.qsg, offset, ...);
    //   } else {
    //       req->aiocb = blk_aio_preadv(blk, offset, &req->sg.iov, ...);
    //   }
    // → Host의 파일/블록 디바이스에서 데이터를 읽어서 Guest 메모리에 DMA로 전달

    return NVME_NO_COMPLETE;  // 비동기 → 나중에 nvme_rw_cb()에서 완료 처리

invalid:
    block_acct_invalid(blk_get_stats(blk), BLOCK_ACCT_READ);
    return status | NVME_DNR;
}
```

### 4.4 nvme_write() / nvme_do_write() 라인바이라인 분석

```c
static inline uint16_t nvme_write(NvmeCtrl *n, NvmeRequest *req)
{
    return nvme_do_write(n, req, false, false);
    //                          append=false, wrz(write_zeroes)=false
}

static uint16_t nvme_do_write(NvmeCtrl *n, NvmeRequest *req, bool append,
                              bool wrz)
{
    NvmeRwCmd *rw = (NvmeRwCmd *)&req->cmd;
    NvmeNamespace *ns = req->ns;
    uint64_t slba = le64_to_cpu(rw->slba);
    uint32_t nlb = (uint32_t)le16_to_cpu(rw->nlb) + 1;
    uint64_t data_size = nvme_l2b(ns, nlb);
    uint64_t data_offset;
    NvmeZone *zone;
    BlockBackend *blk = ns->blkconf.blk;
    uint16_t status;

    // ── MDTS 검사 (write_zeroes가 아닌 경우만) ──
    if (!wrz) {
        status = nvme_check_mdts(n, mapped_size);
        if (status) { goto invalid; }
    }

    // ── LBA 범위 검사 ──
    status = nvme_check_bounds(ns, slba, nlb);
    if (status) { goto invalid; }

    // ── Zoned NS 처리 ──
    if (ns->params.zoned) {
        zone = nvme_get_zone_by_slba(ns, slba);

        if (append) {
            // Zone Append: ZSLBA에서 시작하되, 실제 쓰기는 write pointer 위치
            slba = zone->w_ptr;
            rw->slba = cpu_to_le64(slba);
            // 응답에 실제 쓰기 위치를 반환
            res->slba = cpu_to_le64(slba);
        }

        status = nvme_check_zone_write(ns, zone, slba, nlb);
        if (status) { goto invalid; }

        status = nvme_zrm_auto(n, ns, zone);  // 자동 존 열기
        if (status) { goto invalid; }

        if (!(zone->d.za & NVME_ZA_ZRWA_VALID)) {
            zone->w_ptr += nlb;  // write pointer 전진
        }
    } else if (ns->endgrp && ns->endgrp->fdp.enabled) {
        // FDP (Flexible Data Placement) 처리
        nvme_do_write_fdp(n, req, slba, nlb);
    }

    data_offset = nvme_l2b(ns, slba);

    if (!wrz) {
        // ── PRP/SGL 매핑 → Guest 메모리에서 쓸 데이터 위치 확보 ──
        status = nvme_map_data(n, nlb, req);
        if (status) { goto invalid; }

        block_acct_start(blk_get_stats(blk), &req->acct, data_size, BLOCK_ACCT_WRITE);

        // ★ 비동기 쓰기: Guest 메모리의 데이터 → Host 파일/블록 디바이스
        nvme_blk_write(blk, data_offset, BDRV_SECTOR_SIZE, nvme_rw_cb, req);
    } else {
        // Write Zeroes: 영역을 0으로 채우기
        req->aiocb = blk_aio_pwrite_zeroes(blk, data_offset, data_size,
                                           BDRV_REQ_MAY_UNMAP, nvme_rw_cb, req);
    }

    return NVME_NO_COMPLETE;

invalid:
    return status | NVME_DNR;
}
```

### 4.5 PRP/SGL 디코딩: nvme_map_prp()

Guest 물리 주소를 호스트 메모리로 매핑하는 핵심 함수 (ctrl.c 888행).

```c
static uint16_t nvme_map_prp(NvmeCtrl *n, NvmeSg *sg, uint64_t prp1,
                             uint64_t prp2, uint32_t len)
{
    // prp1: 첫 번째 PRP 엔트리 (Guest 물리 주소)
    // prp2: 두 번째 PRP 엔트리 또는 PRP List의 주소

    // 첫 번째 PRP 엔트리에서 전송할 양 계산 (페이지 경계까지)
    hwaddr trans_len = n->page_size - (prp1 % n->page_size);
    trans_len = MIN(len, trans_len);

    // SG 리스트 초기화 (DMA vs CMB/PMR)
    nvme_sg_init(n, sg, nvme_addr_is_dma(n, prp1));

    // 첫 번째 PRP 매핑
    status = nvme_map_addr(n, sg, prp1, trans_len);
    len -= trans_len;

    if (len) {
        if (len > n->page_size) {
            // 다수의 페이지 → PRP List 따라가기
            // prp2는 PRP List를 가리킴
            // DMA로 PRP List를 읽어서 각 엔트리를 매핑
            nents = (n->page_size - (prp2 & (n->page_size - 1))) >> 3;
            nvme_addr_read(n, prp2, prp_list, nents * 8);

            while (len != 0) {
                uint64_t prp_ent = le64_to_cpu(prp_list[i]);
                // PRP List의 마지막 엔트리가 다음 PRP List를 가리킬 수 있음
                // (chaining)
                trans_len = MIN(len, n->page_size);
                status = nvme_map_addr(n, sg, prp_ent, trans_len);
                len -= trans_len;
                i++;
            }
        } else {
            // 2페이지 이하 → prp2가 직접 두 번째 페이지를 가리킴
            status = nvme_map_addr(n, sg, prp2, len);
        }
    }
    return NVME_SUCCESS;
}
```

### 4.6 Admin 명령어 처리: nvme_admin_cmd()

```c
static uint16_t nvme_admin_cmd(NvmeCtrl *n, NvmeRequest *req)
{
    // Admin 명령은 SGL 사용 불가 (NVMe over PCIe)
    if (NVME_CMD_FLAGS_PSDT(req->cmd.flags) != NVME_PSDT_PRP) {
        return NVME_INVALID_FIELD | NVME_DNR;
    }

    switch (req->cmd.opcode) {
    case NVME_ADM_CMD_DELETE_SQ:     return nvme_del_sq(n, req);
    case NVME_ADM_CMD_CREATE_SQ:     return nvme_create_sq(n, req);
    case NVME_ADM_CMD_GET_LOG_PAGE:  return nvme_get_log(n, req);
    case NVME_ADM_CMD_DELETE_CQ:     return nvme_del_cq(n, req);
    case NVME_ADM_CMD_CREATE_CQ:     return nvme_create_cq(n, req);
    case NVME_ADM_CMD_IDENTIFY:      return nvme_identify(n, req);
    case NVME_ADM_CMD_ABORT:         return nvme_abort(n, req);
    case NVME_ADM_CMD_SET_FEATURES:  return nvme_set_feature(n, req);
    case NVME_ADM_CMD_GET_FEATURES:  return nvme_get_feature(n, req);
    case NVME_ADM_CMD_ASYNC_EV_REQ:  return nvme_aer(n, req);
    case NVME_ADM_CMD_NS_ATTACHMENT: return nvme_ns_attachment(n, req);
    case NVME_ADM_CMD_DBBUF_CONFIG:  return nvme_dbbuf_config(n, req);
    case NVME_ADM_CMD_FORMAT_NVM:    return nvme_format(n, req);
    // ... DIRECTIVE, VIRT_MNGMT, SECURITY ...
    }
}
```

**Create I/O CQ (nvme_create_cq, ctrl.c 5590행):**
```c
static uint16_t nvme_create_cq(NvmeCtrl *n, NvmeRequest *req)
{
    NvmeCreateCq *c = (NvmeCreateCq *)&req->cmd;
    uint16_t cqid = le16_to_cpu(c->cqid);
    uint16_t vector = le16_to_cpu(c->irq_vector);
    uint16_t qsize = le16_to_cpu(c->qsize);      // 0-based
    uint64_t prp1 = le64_to_cpu(c->prp1);        // CQ의 Guest 물리 주소

    // 유효성 검사: cqid 범위, 중복, 큐 크기, 주소 정렬, 인터럽트 벡터
    if (unlikely(!cqid || cqid > n->conf_ioqpairs || n->cq[cqid] != NULL)) { ... }
    if (unlikely(!qsize || qsize > NVME_CAP_MQES(ldq_le_p(&n->bar.cap)))) { ... }
    if (unlikely(prp1 & (n->page_size - 1))) { ... }

    cq = g_malloc0(sizeof(*cq));
    nvme_init_cq(cq, n, prp1, cqid, vector, qsize + 1, NVME_CQ_FLAGS_IEN(qflags));
    // → cq->dma_addr = prp1 (Guest 물리 주소)
    // → cq->phase = 1 (초기 phase bit)
    // → cq->bh = qemu_bh_new(nvme_post_cqes, cq)

    n->qs_created = true;
    return NVME_SUCCESS;
}
```

**Create I/O SQ (nvme_create_sq, ctrl.c 4895행):**
```c
static uint16_t nvme_create_sq(NvmeCtrl *n, NvmeRequest *req)
{
    NvmeCreateSq *c = (NvmeCreateSq *)&req->cmd;
    uint16_t sqid = le16_to_cpu(c->sqid);
    uint16_t cqid = le16_to_cpu(c->cqid);  // 이 SQ와 연결할 CQ
    uint16_t qsize = le16_to_cpu(c->qsize);
    uint64_t prp1 = le64_to_cpu(c->prp1);  // SQ의 Guest 물리 주소

    // 유효성 검사
    if (unlikely(!cqid || nvme_check_cqid(n, cqid))) { ... }  // CQ 존재 필수
    if (unlikely(!sqid || sqid > n->conf_ioqpairs || n->sq[sqid] != NULL)) { ... }

    sq = g_malloc0(sizeof(*sq));
    nvme_init_sq(sq, n, prp1, sqid, cqid, qsize + 1);
    // → sq->dma_addr = prp1
    // → sq->io_req = g_new0(NvmeRequest, sq->size)  ← request 풀 할당
    // → sq->bh = qemu_bh_new(nvme_process_sq, sq)
    return NVME_SUCCESS;
}
```

---

## 5. Completion 처리 (CQE 생성 → 인터럽트)

### 5.1 Completion 전체 흐름

```
nvme_read()/nvme_write()
    → blk_aio_preadv() / blk_aio_pwritev()  (비동기 I/O 시작)
    → Host I/O 완료
    → 콜백: nvme_rw_cb() → nvme_rw_complete_cb()
        → nvme_enqueue_req_completion(cq, req)
            → QTAILQ_INSERT_TAIL(&cq->req_list, req)
            → qemu_bh_schedule(cq->bh)
                → nvme_post_cqes(cq)
                    → Guest 메모리에 CQE 16바이트 DMA write
                    → Phase bit 토글
                    → msix_notify() → Guest 인터럽트!
```

### 5.2 nvme_rw_complete_cb(): I/O 완료 콜백

```c
void nvme_rw_complete_cb(void *opaque, int ret)
{
    NvmeRequest *req = opaque;
    NvmeNamespace *ns = req->ns;
    BlockBackend *blk = ns->blkconf.blk;
    BlockAcctStats *stats = blk_get_stats(blk);

    if (ret) {
        // I/O 에러 → 상태 코드 설정
        block_acct_failed(stats, &req->acct);
        switch (req->cmd.opcode) {
        case NVME_CMD_READ:  req->status = NVME_UNRECOVERED_READ; break;
        case NVME_CMD_WRITE: req->status = NVME_WRITE_FAULT; break;
        default:             req->status = NVME_INTERNAL_DEV_ERROR; break;
        }
    } else {
        block_acct_done(stats, &req->acct);  // 통계 업데이트
    }

    // Zoned NS: write pointer 최종 갱신
    if (ns->params.zoned && nvme_is_write(req)) {
        nvme_finalize_zoned_write(ns, req);
    }

    // ★ CQ에 완료 엔트리 추가
    nvme_enqueue_req_completion(nvme_cq(req), req);
}
```

### 5.3 nvme_enqueue_req_completion(): CQ에 요청 등록

```c
static void nvme_enqueue_req_completion(NvmeCQueue *cq, NvmeRequest *req)
{
    assert(cq->cqid == req->sq->cqid);

    // request를 SQ의 out_req_list에서 제거하고 CQ의 req_list에 추가
    QTAILQ_REMOVE(&req->sq->out_req_list, req, entry);
    QTAILQ_INSERT_TAIL(&cq->req_list, req, entry);

    // ★ CQE 포스팅 BH 스케줄
    qemu_bh_schedule(cq->bh);  // → nvme_post_cqes() 호출
}
```

### 5.4 nvme_post_cqes() 라인바이라인 분석

CQ에 대기 중인 CQE들을 Guest 메모리에 기록하고 인터럽트를 발생시킨다 (ctrl.c 1498행).

```c
static void nvme_post_cqes(void *opaque)
{
    NvmeCQueue *cq = opaque;
    NvmeCtrl *n = cq->ctrl;
    NvmeRequest *req, *next;
    bool pending = cq->head != cq->tail;  // 이전에 미처리 CQE가 있었는지
    int ret;

    QTAILQ_FOREACH_SAFE(req, &cq->req_list, entry, next) {
        NvmeSQueue *sq;
        hwaddr addr;

        // Shadow DB 모드: EventIdx 갱신 및 최신 head 읽기
        if (n->dbbuf_enabled) {
            nvme_update_cq_eventidx(cq);
            nvme_update_cq_head(cq);
        }

        // CQ가 가득 찼으면 더 이상 포스팅 불가 → 중단
        if (nvme_cq_full(cq)) {
            break;
            // Guest가 CQ Head Doorbell을 업데이트하면 공간이 생기고
            // 그때 다시 nvme_post_cqes()가 호출됨
        }

        sq = req->sq;

        // ── CQE 구성 ──
        // status: [15:1] = 실제 상태 코드, [0] = phase bit
        req->cqe.status = cpu_to_le16((req->status << 1) | cq->phase);
        req->cqe.sq_id = cpu_to_le16(sq->sqid);    // 어느 SQ에서 온 명령인지
        req->cqe.sq_head = cpu_to_le16(sq->head);  // 현재 SQ head (flow control)

        // ── Guest 메모리에 CQE 16바이트 DMA Write ──
        addr = cq->dma_addr + (cq->tail << NVME_CQES);
        //                     tail * 16 (NVME_CQES=4, 1<<4=16)
        ret = pci_dma_write(PCI_DEVICE(n), addr, (void *)&req->cqe,
                            sizeof(req->cqe));
        if (ret) {
            // DMA 쓰기 실패 → 치명적 에러
            stl_le_p(&n->bar.csts, NVME_CSTS_FAILED);
            break;
        }

        QTAILQ_REMOVE(&cq->req_list, req, entry);

        // ── CQ Tail 증가 + Phase bit 토글 ──
        nvme_inc_cq_tail(cq);
        // cq->tail++; if (cq->tail >= cq->size) { cq->tail = 0; cq->phase = !cq->phase; }
        //
        // Phase bit는 CQ가 한 바퀴 돌 때마다 토글된다.
        // Guest 드라이버는 이 phase bit를 보고 새 CQE인지 구분한다.

        nvme_sg_unmap(&req->sg);  // SG 리스트 해제

        // 해당 SQ에 빈 request 슬롯이 생기고 아직 처리할 명령이 있으면 SQ 재개
        if (QTAILQ_EMPTY(&sq->req_list) && !nvme_sq_empty(sq)) {
            qemu_bh_schedule(sq->bh);
        }

        // Request를 다시 SQ의 free list(req_list)에 반환
        QTAILQ_INSERT_TAIL(&sq->req_list, req, entry);
    }

    // ── 인터럽트 발생 ──
    if (cq->tail != cq->head) {
        // CQ에 새 CQE가 있으면 인터럽트
        if (cq->irq_enabled && !pending) {
            n->cq_pending++;
        }
        nvme_irq_assert(n, cq);
        // → msix_enabled일 때: msix_notify(pci, cq->vector)
        //   Guest의 MSI-X 인터럽트 핸들러 실행 → CQE 처리
        // → PIN 인터럽트: pci_irq_assert()
    }
}
```

### 5.5 인터럽트 발생: nvme_irq_assert()

```c
static void nvme_irq_assert(NvmeCtrl *n, NvmeCQueue *cq)
{
    PCIDevice *pci = PCI_DEVICE(n);

    if (cq->irq_enabled) {
        if (msix_enabled(pci)) {
            // ★ MSI-X 인터럽트: 대부분의 경우 이 경로
            msix_notify(pci, cq->vector);
            // → QEMU가 KVM에 인터럽트 주입 요청
            // → Guest의 MSI-X 핸들러(nvme_irq()) 실행
        } else {
            // Legacy INTx 인터럽트
            n->irq_status |= 1 << cq->vector;
            nvme_irq_check(n);
        }
    }
}
```

---

## 6. NVMe Queue 구조 (디바이스 측)

### 6.1 NvmeSQueue 구조체

```c
typedef struct NvmeSQueue {
    struct NvmeCtrl *ctrl;       // 소속 컨트롤러
    uint16_t    sqid;            // SQ ID (0=Admin, 1~N=I/O)
    uint16_t    cqid;            // 연결된 CQ의 ID
    uint32_t    head;            // SQ Head (디바이스가 관리) - 다음 fetch할 위치
    uint32_t    tail;            // SQ Tail (호스트가 Doorbell로 업데이트)
    uint32_t    size;            // 큐 엔트리 수 (depth)
    uint64_t    dma_addr;        // ★ Guest 물리 주소 (SQ 메모리의 DMA 주소)
    uint64_t    db_addr;         // Shadow Doorbell 주소 (dbbuf 모드)
    uint64_t    ei_addr;         // EventIdx 주소 (dbbuf 모드)
    QEMUBH      *bh;             // Bottom Half → nvme_process_sq()에 바인딩
    EventNotifier notifier;      // ioeventfd notifier
    bool        ioeventfd_enabled;
    NvmeRequest *io_req;         // ★ Request 풀 (size개 할당)
    QTAILQ_HEAD(, NvmeRequest) req_list;     // 빈 request 리스트
    QTAILQ_HEAD(, NvmeRequest) out_req_list; // 진행 중 request 리스트
    QTAILQ_ENTRY(NvmeSQueue) entry;
} NvmeSQueue;
```

### 6.2 NvmeCQueue 구조체

```c
typedef struct NvmeCQueue {
    struct NvmeCtrl *ctrl;
    uint8_t     phase;           // ★ Phase bit (0 또는 1, CQ wrap 시 토글)
    uint16_t    cqid;            // CQ ID
    uint16_t    irq_enabled;     // 인터럽트 활성화 여부
    uint32_t    head;            // CQ Head (호스트가 Doorbell로 업데이트)
    uint32_t    tail;            // CQ Tail (디바이스가 관리) - 다음 CQE 기록 위치
    uint32_t    vector;          // MSI-X 인터럽트 벡터 번호
    uint32_t    size;            // 큐 엔트리 수
    uint64_t    dma_addr;        // ★ Guest 물리 주소 (CQ 메모리의 DMA 주소)
    uint64_t    db_addr;         // Shadow Doorbell 주소
    uint64_t    ei_addr;         // EventIdx 주소
    QEMUBH      *bh;             // Bottom Half → nvme_post_cqes()에 바인딩
    EventNotifier notifier;
    bool        ioeventfd_enabled;
    QTAILQ_HEAD(, NvmeSQueue) sq_list;   // 이 CQ에 연결된 SQ 리스트
    QTAILQ_HEAD(, NvmeRequest) req_list; // 포스팅 대기 중인 완료 요청
} NvmeCQueue;
```

### 6.3 NvmeRequest 구조체

```c
typedef struct NvmeRequest {
    struct NvmeSQueue       *sq;       // 소속 SQ
    struct NvmeNamespace    *ns;       // 대상 네임스페이스
    BlockAIOCB              *aiocb;    // QEMU 블록 레이어 비동기 콜백
    uint16_t                status;    // 완료 상태 코드
    void                    *opaque;   // 추가 컨텍스트 (compare 등에서 사용)
    NvmeCqe                 cqe;       // ★ 이 request의 CQE (16바이트)
    NvmeCmd                 cmd;       // ★ 원본 SQE 복사본 (64바이트)
    BlockAcctCookie         acct;      // 블록 통계용
    NvmeSg                  sg;        // Scatter-Gather 리스트
    bool                    atomic_write;
    QTAILQ_ENTRY(NvmeRequest)entry;
} NvmeRequest;
```

### 6.4 SQ/CQ 링 버퍼 다이어그램 (디바이스 관점)

```
               ★ SQ (Submission Queue) - Guest 메모리

  Guest 물리 주소: sq->dma_addr
  ┌──────────────────────────────────────────────────┐
  │ [0] │ [1] │ [2] │ [3] │ [4] │ [5] │ [6] │ [7]  │
  │ SQE │ SQE │ SQE │     │     │     │ SQE │ SQE  │
  │64B  │64B  │64B  │     │     │     │64B  │64B   │
  └──────────────────────────────────────────────────┘
                  ↑                         ↑
                  │                         │
               sq->head=2               sq->tail=7
               (디바이스)               (호스트가 Doorbell로 알림)

  디바이스 동작:
  1. sq->head != sq->tail 이면 처리할 명령이 있음
  2. sq->dma_addr + (head * 64) 에서 DMA read로 SQE를 가져옴
  3. head를 하나 증가시킴 (ring wrap 처리)
  4. 명령어 실행


               ★ CQ (Completion Queue) - Guest 메모리

  Guest 물리 주소: cq->dma_addr
  ┌──────────────────────────────────────────────────┐
  │ [0] │ [1] │ [2] │ [3] │ [4] │ [5] │ [6] │ [7]  │
  │ CQE │ CQE │ CQE │     │     │     │     │      │
  │16B  │16B  │16B  │     │     │     │     │      │
  │P=1  │P=1  │P=1  │     │     │     │     │      │
  └──────────────────────────────────────────────────┘
            ↑              ↑
            │              │
         cq->head=1     cq->tail=3
         (호스트가       (디바이스가
          Doorbell로      CQE 기록 후
          알림)           증가)

  디바이스 동작:
  1. 명령 완료 → cq->dma_addr + (tail * 16)에 DMA write로 CQE 기록
  2. CQE의 status[0] = cq->phase (현재 phase bit)
  3. tail 증가 (ring wrap 시 phase 토글)
  4. MSI-X 인터럽트 발생

  호스트 동작:
  1. 인터럽트 수신 → CQ 폴링
  2. CQE의 phase bit가 기대값과 일치하면 새 CQE
  3. 처리 완료 후 Head Doorbell 업데이트 → 슬롯 재사용
```

### 6.5 CQ Full / SQ Empty 판별

```c
// CQ가 가득 찬지 확인 (tail+1 == head이면 full)
static uint8_t nvme_cq_full(NvmeCQueue *cq)
{
    return (cq->tail + 1) % cq->size == cq->head;
}

// SQ가 비었는지 확인 (head == tail이면 empty)
static uint8_t nvme_sq_empty(NvmeSQueue *sq)
{
    return sq->head == sq->tail;
}

// CQ Tail 증가 + Phase bit 토글
static void nvme_inc_cq_tail(NvmeCQueue *cq)
{
    cq->tail++;
    if (cq->tail >= cq->size) {
        cq->tail = 0;
        cq->phase = !cq->phase;  // ★ 한 바퀴 돌면 phase 반전
    }
}

// SQ Head 증가
static void nvme_inc_sq_head(NvmeSQueue *sq)
{
    sq->head = (sq->head + 1) % sq->size;
}
```

---

## 7. Namespace 에뮬레이션

### 7.1 NvmeNamespace 핵심 필드

```c
typedef struct NvmeNamespace {
    DeviceState  parent_obj;        // QEMU 디바이스 모델 상속
    BlockConf    blkconf;           // ★ 백엔드 블록 디바이스 설정
    int64_t      size;              // 네임스페이스 총 크기 (바이트)
    int64_t      moff;              // 메타데이터 오프셋 (데이터 영역 이후)
    NvmeIdNs     id_ns;             // ★ Identify Namespace 데이터 (4KB)
    NvmeLBAF     lbaf;              // 현재 LBA 포맷 {ds, ms}
    unsigned int nlbaf;             // 지원하는 LBA 포맷 수
    size_t       lbasz;             // LBA 크기 (바이트) = 1 << lbaf.ds
    uint8_t      csi;               // Command Set Identifier (NVM/Zoned)
    NvmeNamespaceParams params;     // QEMU 설정 파라미터
    NvmeSubsystem *subsys;
    NvmeCtrl *ctrl;                 // private NS이면 특정 컨트롤러
} NvmeNamespace;
```

### 7.2 LBA 포맷과 크기 변환

```c
// LBA 수 → 바이트 변환
static inline size_t nvme_l2b(NvmeNamespace *ns, uint64_t lba)
{
    return lba << ns->lbaf.ds;  // 예: lba * 4096 (ds=12)
}

// 메타데이터 크기 계산
static inline size_t nvme_m2b(NvmeNamespace *ns, uint64_t lba)
{
    return ns->lbaf.ms * lba;  // 예: lba * 8 (ms=8)
}

// 메타데이터 오프셋 (이미지 파일 내에서)
static inline int64_t nvme_moff(NvmeNamespace *ns, uint64_t lba)
{
    return ns->moff + nvme_m2b(ns, lba);
}
```

### 7.3 Namespace 초기화: nvme_ns_init_format()

```c
void nvme_ns_init_format(NvmeNamespace *ns)
{
    NvmeIdNs *id_ns = &ns->id_ns;

    // 현재 LBA 포맷 결정
    ns->lbaf = id_ns->lbaf[NVME_ID_NS_FLBAS_INDEX(id_ns->flbas)];
    ns->lbasz = 1 << ns->lbaf.ds;  // 예: 1 << 12 = 4096

    // 네임스페이스 크기 계산 (LBA 단위)
    int64_t nlbas = ns->size / (ns->lbasz + ns->lbaf.ms);
    id_ns->nsze = cpu_to_le64(nlbas);
    id_ns->ncap = id_ns->nsze;  // thin provisioning 없음
    id_ns->nuse = id_ns->ncap;

    // 메타데이터 영역 시작 오프셋 (데이터 영역 바로 뒤)
    ns->moff = nlbas << ns->lbaf.ds;
}
```

### 7.4 백엔드 스토리지 연결

Namespace는 QEMU의 BlockBackend를 통해 호스트의 실제 파일/블록 디바이스와 연결된다.

```
QEMU 커맨드라인:
  -drive file=nvme0.qcow2,if=none,id=drive0
  -device nvme-ns,drive=drive0,bus=nvme0,nsid=1

내부 연결:
  NvmeNamespace.blkconf.blk → BlockBackend → BlockDriverState (qcow2/raw)
                                              → Host Filesystem I/O
```

I/O 명령에서의 사용:
```c
// nvme_read()에서:
BlockBackend *blk = ns->blkconf.blk;
uint64_t offset = nvme_l2b(ns, slba);   // LBA → 파일 내 바이트 오프셋

// Guest 메모리에서 준비한 SG 리스트를 사용하여 Host 파일에서 읽기
nvme_blk_read(blk, offset, BDRV_SECTOR_SIZE, nvme_rw_cb, req);
// → blk_aio_preadv(blk, offset, &req->sg.iov, 0, callback, req)
// → Host 파일에서 offset 위치의 데이터를 읽어서
//   DMA를 통해 Guest 메모리에 전달
```

### 7.5 기본 LBA 포맷 테이블

```c
// ns.c 117행 - 8개의 기본 LBA 포맷
static const NvmeLBAF defaults[16] = {
    [0] = { .ds =  9           },   //  512B, 메타데이터 없음
    [1] = { .ds =  9, .ms =  8 },   //  512B + 8B 메타데이터
    [2] = { .ds =  9, .ms = 16 },   //  512B + 16B 메타데이터
    [3] = { .ds =  9, .ms = 64 },   //  512B + 64B 메타데이터
    [4] = { .ds = 12           },   // 4096B, 메타데이터 없음 ← 일반적
    [5] = { .ds = 12, .ms =  8 },   // 4096B + 8B 메타데이터
    [6] = { .ds = 12, .ms = 16 },   // 4096B + 16B 메타데이터
    [7] = { .ds = 12, .ms = 64 },   // 4096B + 64B 메타데이터
};
```

---

## 부록: NVMe 스펙 레지스터 오프셋 맵 (include/block/nvme.h)

```c
typedef struct QEMU_PACKED NvmeBar {
    uint64_t    cap;     // 0x00: Controller Capabilities (RO)
    uint32_t    vs;      // 0x08: Version (RO)
    uint32_t    intms;   // 0x0C: Interrupt Mask Set (RW1S)
    uint32_t    intmc;   // 0x10: Interrupt Mask Clear (RW1C)
    uint32_t    cc;      // 0x14: Controller Configuration (RW)
    uint8_t     rsvd24[4];
    uint32_t    csts;    // 0x1C: Controller Status (RO)
    uint32_t    nssr;    // 0x20: NVM Subsystem Reset (RW)
    uint32_t    aqa;     // 0x24: Admin Queue Attributes (RW)
    uint64_t    asq;     // 0x28: Admin SQ Base Address (RW)
    uint64_t    acq;     // 0x30: Admin CQ Base Address (RW)
    uint32_t    cmbloc;  // 0x38: CMB Location (RO)
    uint32_t    cmbsz;   // 0x3C: CMB Size (RO)
    // ... PMR 관련 레지스터 ...
} NvmeBar;
```

NvmeCmd (SQE) 구조 (64바이트):
```c
typedef struct QEMU_PACKED NvmeCmd {
    uint8_t     opcode;    // [0]     명령어 종류
    uint8_t     flags;     // [1]     PSDT(PRP/SGL), FUSE
    uint16_t    cid;       // [2:3]   Command ID
    uint32_t    nsid;      // [4:7]   Namespace ID
    uint64_t    res1;      // [8:15]  Reserved
    uint64_t    mptr;      // [16:23] Metadata Pointer
    NvmeCmdDptr dptr;      // [24:39] Data Pointer (PRP1/PRP2 또는 SGL)
    uint32_t    cdw10;     // [40:43] Command Dword 10
    uint32_t    cdw11;     // [44:47]
    uint32_t    cdw12;     // [48:51]
    uint32_t    cdw13;     // [52:55]
    uint32_t    cdw14;     // [56:59]
    uint32_t    cdw15;     // [60:63]
} NvmeCmd;
```

NvmeCqe (CQE) 구조 (16바이트):
```c
typedef struct QEMU_PACKED NvmeCqe {
    uint32_t    result;    // [0:3]   Command Specific
    uint32_t    dw1;       // [4:7]   Reserved / Command Specific
    uint16_t    sq_head;   // [8:9]   SQ Head Pointer (flow control)
    uint16_t    sq_id;     // [10:11] SQ Identifier
    uint16_t    cid;       // [12:13] Command ID (어떤 SQE의 응답인지)
    uint16_t    status;    // [14:15] Status ([15:1]=코드, [0]=Phase)
} NvmeCqe;
```

NvmeRwCmd (Read/Write SQE) 구조:
```c
typedef struct QEMU_PACKED NvmeRwCmd {
    uint8_t     opcode;    // 0x01=Write, 0x02=Read
    uint8_t     flags;
    uint16_t    cid;
    uint32_t    nsid;
    uint32_t    cdw2, cdw3;
    uint64_t    mptr;      // Metadata Pointer
    NvmeCmdDptr dptr;      // PRP1/PRP2
    uint64_t    slba;      // ★ Start LBA
    uint16_t    nlb;       // ★ Number of Logical Blocks (0-based)
    uint16_t    control;   // FUA, LR, PRINFO 등
    uint8_t     dsmgmt;
    uint8_t     rsvd;
    uint16_t    dspec;     // Directive Specific (FDP PID)
    uint32_t    reftag;
    uint16_t    apptag;
    uint16_t    appmask;
} NvmeRwCmd;
```

---

## 요약: QEMU NVMe 에뮬레이션의 핵심 동작 시퀀스

```
[Guest NVMe 드라이버]
    │
    ├─ 1. CC.EN=1 쓰기 → nvme_write_bar() → nvme_start_ctrl()
    │      → Admin SQ/CQ 초기화, CSTS.RDY=1 설정
    │
    ├─ 2. Create I/O CQ (Admin 명령) → nvme_create_cq()
    │      → NvmeCQueue 할당, Guest CQ 주소(prp1)를 dma_addr로 저장
    │
    ├─ 3. Create I/O SQ (Admin 명령) → nvme_create_sq()
    │      → NvmeSQueue 할당, Guest SQ 주소(prp1)를 dma_addr로 저장
    │      → NvmeRequest 풀(size개) 할당
    │
    ├─ 4. SQ에 Read/Write SQE 기록 (Guest 메모리에 직접)
    │
    ├─ 5. SQ Tail Doorbell 쓰기 → VM-Exit → nvme_process_db()
    │      → sq->tail 갱신 → nvme_process_sq() 스케줄
    │
    ├─ 6. nvme_process_sq(): SQE DMA read → 명령어 디스패치
    │      → nvme_read()/nvme_write() → PRP 디코딩 → blk_aio_preadv/pwritev
    │
    ├─ 7. Host I/O 완료 → nvme_rw_complete_cb()
    │      → nvme_enqueue_req_completion() → nvme_post_cqes()
    │      → CQE DMA write (Guest CQ에 16B 기록) + Phase bit
    │      → msix_notify() → Guest 인터럽트
    │
    └─ 8. Guest 인터럽트 핸들러: CQE 확인, CQ Head Doorbell 쓰기
           → nvme_process_db() → cq->head 갱신, 인터럽트 해제
```

이 전체 과정이 Guest OS 입장에서는 "실제 NVMe SSD 하드웨어와 통신하는 것"과 동일하게 보인다. QEMU가 NVMe 스펙의 레지스터, 도어벨, SQ/CQ 프로토콜을 소프트웨어로 충실히 구현하고 있기 때문이다.
