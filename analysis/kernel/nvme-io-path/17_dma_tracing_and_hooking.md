# 17. Device→Host DMA 추적/후킹 방법 총정리

## 0. 문제 정의: 왜 Device→Host DMA 추적이 어려운가

### 0.1 두 방향의 근본적 차이

Host→Device와 Device→Host는 하드웨어 레벨에서 완전히 다른 메커니즘을 사용한다.

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Host→Device (MMIO Write/Read)                    │
│                                                                     │
│   CPU ─── MMU ─── [Virtual Address] ─── Page Table ─── Physical    │
│    │       │            │                   │              │        │
│    │    ┌──┘         ┌──┘                ┌──┘           ┌──┘        │
│    │    │PTE를       │ioremap()된        │PTE를         │PCIe       │
│    │    │non-present │가상 주소로        │조작하면      │BAR 영역   │
│    │    │로 설정     │CPU가 접근         │page fault    │에 도달    │
│    │    │가능!       │                   │발생!         │           │
│    │    └──────── mmiotrace가 이것을 이용 ──────────┘   │           │
│                                                         ▼           │
│                                              ┌──────────────┐       │
│                                              │ NVMe Device  │       │
│                                              │ (BAR 레지스터)│       │
│                                              └──────────────┘       │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                    Device→Host (DMA Write/Read)                     │
│                                                                     │
│                              ┌────── CPU의 MMU를 완전히 우회!       │
│                              │                                      │
│   NVMe    PCIe    (IOMMU)   Host                                    │
│   Device ──TLP──→ ──?──→  Physical Memory                          │
│    │               │          │                                      │
│    │            ┌──┘       ┌──┘                                      │
│    │            │IOMMU가   │물리 메모리에                            │
│    │            │있으면    │직접 쓰기                                │
│    │            │주소 변환 │→ CPU page fault 발생 안함!              │
│    │            │수행      │→ mmiotrace 방식 사용 불가!              │
│    │            │          │                                         │
│    │            │IOMMU 없으면: 물리 주소 그대로 사용                 │
│    │            │IOMMU 있으면: IOVA → PA 변환 후 접근                │
│    └────────────┴──────────┘                                         │
│                                                                      │
│   ★ CPU의 page table을 거치지 않으므로 page fault hook 불가          │
│   ★ 디바이스가 "알아서" 메모리에 접근하므로 소프트웨어 개입 어려움   │
└──────────────────────────────────────────────────────────────────────┘
```

### 0.2 NVMe에서 Device→Host DMA가 발생하는 상황들

NVMe 디바이스가 호스트 메모리에 DMA로 접근하는 경우는 크게 세 가지다.

```
1. Submission Queue 읽기 (Device ← Host Memory)
   - 호스트가 Doorbell을 쓰면, 디바이스가 SQ에서 명령어를 DMA read
   - 방향: Device reads from Host → DMA_FROM_DEVICE 관점

2. Completion Queue 쓰기 (Device → Host Memory)
   - 명령어 처리 완료 후 CQE를 호스트 메모리에 DMA write
   - 방향: Device writes to Host → DMA_TO_DEVICE 관점

3. 데이터 전송 (양방향)
   - Read 명령: 디바이스가 데이터를 호스트 메모리에 DMA write
   - Write 명령: 디바이스가 호스트 메모리에서 데이터를 DMA read
   - PRP/SGL 리스트 자체도 DMA read로 가져옴

4. MSI-X 인터럽트
   - 디바이스가 MSI-X 테이블에 지정된 주소에 DMA write하여 인터럽트 발생
```

---

## 1. mmiotrace 리뷰 (Host→Device 방향, 기존 방식)

### 1.1 kmmio.c의 page fault 기반 메커니즘

mmiotrace는 CPU가 ioremap()된 MMIO 영역에 접근할 때 이를 가로채는 메커니즘이다. 핵심 원리는 CPU의 MMU page table을 조작하는 것이다.

소스: `arch/x86/mm/kmmio.c`

```c
// kmmio_fault_page 구조체: 추적 대상 페이지 정보
struct kmmio_fault_page {
    struct list_head list;
    struct kmmio_fault_page *release_next;
    unsigned long addr;           // 추적 대상 가상 주소
    pteval_t old_presence;        // 원래 PTE의 present 비트 상태
    bool armed;                   // 현재 "무장"(트랩 설정) 상태인지
    int count;                    // 이 페이지를 참조하는 probe 수
    bool scheduled_for_release;
};
```

**동작 순서:**

```
1단계: arm_kmmio_fault_page() → PTE의 present 비트를 clear
       ┌──────────────────────────────────────┐
       │ clear_page_presence(f, true)          │
       │   → clear_pte_presence(pte, true)     │
       │     → pte_clear(&init_mm, 0, pte)     │
       │   → flush_tlb_one_kernel(f->addr)     │
       └──────────────────────────────────────┘

2단계: CPU가 해당 주소에 접근 → page fault 발생
       ┌──────────────────────────────────────┐
       │ do_page_fault() → kmmio_handler()     │
       │   → pre_handler 콜백 호출             │
       │   → PTE 복원 (present 비트 set)       │
       │   → TF 플래그 설정 (single-step)      │
       └──────────────────────────────────────┘

3단계: 실제 MMIO 접근 수행 (1개 명령어만)

4단계: single-step trap → post_kmmio_handler()
       ┌──────────────────────────────────────┐
       │ post_kmmio_handler()                  │
       │   → post_handler 콜백 호출            │
       │   → PTE 다시 clear (재무장)           │
       │   → TF 플래그 복원                    │
       └──────────────────────────────────────┘
```

kmmio_handler() 코드의 핵심 부분:

```c
int kmmio_handler(struct pt_regs *regs, unsigned long addr)
{
    // ... page fault가 kmmio에 의한 것인지 확인
    faultpage = get_kmmio_fault_page(page_base);
    if (!faultpage)
        goto no_kmmio;

    ctx->fpage = faultpage;
    ctx->probe = get_kmmio_probe(page_base);

    // pre_handler 호출 (접근 전 정보 기록)
    if (ctx->probe && ctx->probe->pre_handler)
        ctx->probe->pre_handler(ctx->probe, regs, addr);

    // single-step 모드 활성화
    regs->flags |= X86_EFLAGS_TF;
    regs->flags &= ~X86_EFLAGS_IF;

    // PTE 복원하여 실제 접근 허용
    disarm_kmmio_fault_page(ctx->fpage);

    return 1; // fault handled
}
```

### 1.2 mmio-mod.c의 상위 레이어

`arch/x86/mm/mmio-mod.c`에서는 ioremap/iounmap을 후킹하여 자동으로 MMIO 영역을 추적한다.

```c
// pre() 콜백: MMIO 접근 전 정보 수집
static void pre(struct kmmio_probe *p, struct pt_regs *regs, unsigned long addr)
{
    const enum reason_type type = get_ins_type(instptr);  // 명령어 분석

    switch (type) {
    case REG_READ:
        my_trace->opcode = MMIO_READ;
        my_trace->width = get_ins_mem_width(instptr);     // 접근 크기
        break;
    case REG_WRITE:
        my_trace->opcode = MMIO_WRITE;
        my_trace->width = get_ins_mem_width(instptr);
        my_trace->value = get_ins_reg_val(instptr, regs); // 기록할 값
        break;
    // ...
    }
}

// post() 콜백: MMIO 접근 후 결과 수집
static void post(struct kmmio_probe *p, unsigned long condition, struct pt_regs *regs)
{
    switch (my_reason->type) {
    case REG_READ:
        my_trace->value = get_ins_reg_val(my_reason->ip, regs);  // 읽은 값
        break;
    }
    mmio_trace_rw(my_trace);  // trace 버퍼에 기록
}
```

### 1.3 사용법

```bash
# mmiotrace 활성화
mount -t debugfs none /sys/kernel/debug
echo mmiotrace > /sys/kernel/debug/tracing/current_tracer

# 추적 시작 (이후 발생하는 모든 ioremap/MMIO 접근이 기록됨)
cat /sys/kernel/debug/tracing/trace_pipe

# 추적 결과 예시:
# MAP 4 0xf0060000 0xf0060fff 0xe0b4e000
# R 4 1234.5678 0xf0060064 0x00000001 0x0  1
# W 4 1234.5679 0xf0060064 0x00000002 0x0  1
```

### 1.4 왜 Device→Host DMA에는 적용 불가능한가

mmiotrace의 핵심은 **CPU의 MMU page table을 조작**하는 것이다. 그런데 DMA는 CPU의 MMU를 전혀 거치지 않는다.

```
mmiotrace가 작동하는 이유:
  CPU → MMU → [PTE를 non-present로 설정] → page fault!
  → 소프트웨어가 가로챌 수 있음

DMA에서 작동하지 않는 이유:
  Device → PCIe bus → (IOMMU) → 물리 메모리
  → CPU의 page table과 완전히 무관
  → CPU page fault 발생하지 않음
  → 소프트웨어 개입 불가
```

따라서 Device→Host DMA를 추적하려면 완전히 다른 접근 방식이 필요하다.

---

## 2. 방법 1: IOMMU Fault 기반 Hooking

### 2.1 원리

IOMMU는 디바이스의 DMA 주소를 물리 주소로 변환하는 하드웨어이다. IOMMU page table에서 특정 매핑을 제거하면, 디바이스가 해당 주소로 DMA할 때 IOMMU fault가 발생한다.

```
┌─────────────────────────────────────────────────────────────┐
│                    IOMMU Fault 기반 Hooking                  │
│                                                              │
│  1. 정상 상태:                                               │
│     Device ──DMA──→ IOMMU [IOVA→PA 매핑 있음] → 물리 메모리 │
│                                                              │
│  2. 매핑 제거 후:                                            │
│     Device ──DMA──→ IOMMU [매핑 없음!] → FAULT 발생!        │
│                         │                                    │
│                         ▼                                    │
│                    Fault Handler                             │
│                    - 접근 주소, 디바이스 ID, R/W 기록        │
│                    - 매핑 복구                               │
│                    - DMA 재시도 (가능한 경우)                │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 커널 코드 분석

#### Intel VT-d DMAR Fault Handler

소스: `drivers/iommu/intel/dmar.c`

```c
irqreturn_t dmar_fault(int irq, void *dev_id)
{
    struct intel_iommu *iommu = dev_id;
    u32 fault_status;

    fault_status = readl(iommu->reg + DMAR_FSTS_REG);

    fault_index = dma_fsts_fault_record_index(fault_status);
    reg = cap_fault_reg_offset(iommu->cap);

    while (1) {
        u8 fault_reason;
        u16 source_id;
        u64 guest_addr;

        data = readl(iommu->reg + reg + fault_index * PRIMARY_FAULT_REG_LEN + 12);
        if (!(data & DMA_FRCD_F))
            break;

        fault_reason = dma_frcd_fault_reason(data);
        type = dma_frcd_type(data);
        source_id = dma_frcd_source_id(data);
        guest_addr = dmar_readq(iommu->reg + reg + fault_index * PRIMARY_FAULT_REG_LEN);

        // fault 기록 및 출력
        dmar_fault_do_one(iommu, type, fault_reason,
                          pasid_present ? pasid : IOMMU_PASID_INVALID,
                          source_id, guest_addr);
        // ...
    }
}
```

Fault 원인 문자열 (DMA remap 관련):

```c
static const char *dma_remap_fault_reasons[] = {
    "Software",
    "Present bit in root entry is clear",
    "Present bit in context entry is clear",
    "Invalid context entry",
    "Access beyond MGAW",
    "PTE Write access is not set",   // ← Write 보호를 이용할 수 있음
    "PTE Read access is not set",    // ← Read 보호를 이용할 수 있음
    "Next page table ptr is invalid",
    // ...
};
```

#### I/O Page Fault (IOPF) 프레임워크

소스: `drivers/iommu/io-pgfault.c`

IOPF는 PRI(Page Request Interface)를 지원하는 디바이스를 위한 프레임워크이다. PRI를 지원하면 디바이스가 IOMMU fault 발생 시 자동으로 재시도할 수 있다.

```c
int iommu_report_device_fault(struct device *dev, struct iopf_fault *evt)
{
    struct iommu_attach_handle *attach_handle;

    // 적절한 fault handler 찾기
    attach_handle = find_fault_handler(dev, evt);
    if (!attach_handle)
        goto err_bad_iopf;

    // fault 파라미터 가져오기
    iopf_param = iopf_get_dev_fault_param(dev);

    // partial fault인 경우 (그룹의 마지막이 아닌 경우) 대기
    if (!(fault->prm.flags & IOMMU_FAULT_PAGE_REQUEST_LAST_PAGE)) {
        ret = report_partial_fault(iopf_param, fault);
        return ret;
    }

    // 마지막 fault → 그룹 할당 후 도메인의 iopf_handler 호출
    group = iopf_group_alloc(iopf_param, evt, &abort_group);
    group->attach_handle = attach_handle;

    if (group->attach_handle->domain->iopf_handler(group))
        goto err_abort;

    return 0;
}
```

### 2.3 실제 구현 방법 (개념적)

IOMMU fault를 이용한 DMA 추적의 이론적 구현:

```c
/*
 * IOMMU fault 기반 DMA 추적 - 개념적 구현
 * (실제 커널 API로는 직접 구현이 제한적)
 */

// 1. 특정 IOVA 범위의 매핑을 제거
iommu_unmap(domain, target_iova, size);

// 2. fault handler 등록 (iommufd 인터페이스 사용)
//    - IOPF 프레임워크를 통해 등록
//    - 또는 직접 DMAR fault 로그 모니터링

// 3. fault 발생 시 정보 수집
//    - fault 주소 (어디에 DMA하려 했는지)
//    - source ID (어떤 디바이스인지)
//    - fault type (Read인지 Write인지)
//    - PASID (어떤 프로세스 컨텍스트인지)

// 4. 매핑 복구
iommu_map(domain, target_iova, phys_addr, size, prot);

// 5. 재시도 (PRI 지원 디바이스만 가능)
//    PRI 미지원 디바이스: DMA 실패로 종료됨
```

### 2.4 한계

```
┌──────────────────────────────────────────────────────────────┐
│ IOMMU Fault 기반 Hooking의 한계                              │
│                                                              │
│ 1. DMA 실패 가능성                                           │
│    - 대부분의 NVMe 디바이스는 PRI를 지원하지 않음            │
│    - fault 발생 시 DMA가 그냥 실패하고 에러 보고             │
│    - 디바이스가 복구 불가능한 상태에 빠질 수 있음            │
│                                                              │
│ 2. 성능 영향                                                 │
│    - 매번 fault → handler → 복구 → 재시도 사이클             │
│    - 수 마이크로초~밀리초 단위 지연                          │
│    - 프로덕션 환경에서 사용 불가                             │
│                                                              │
│ 3. IOMMU 하드웨어 의존                                       │
│    - 모든 시스템에 IOMMU가 있는 것은 아님                    │
│    - recoverable fault 지원 여부가 하드웨어마다 다름          │
│    - Intel VT-d의 Primary Fault는 non-recoverable             │
│                                                              │
│ 4. 정보 제한                                                 │
│    - fault 시점의 주소/디바이스만 알 수 있음                  │
│    - 실제 전송 데이터는 확인 불가                             │
│    - DMA 크기 정보가 정확하지 않을 수 있음                   │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. 방법 2: IOMMU Dirty Page Tracking

### 3.1 원리

IOMMU 하드웨어가 DMA write가 발생한 페이지를 dirty bit로 표시하는 기능이다. 원래 Live Migration(VM 마이그레이션)을 위해 설계되었지만, DMA write 추적에도 활용할 수 있다.

```
┌──────────────────────────────────────────────────────────────┐
│                IOMMU Dirty Page Tracking                      │
│                                                              │
│  IOMMU Page Table Entry:                                     │
│  ┌─────┬─────┬───────┬──────────────────────────┐            │
│  │ ... │ D   │ R/W   │ Physical Address          │            │
│  │     │(dirty)│     │                           │            │
│  └─────┴──┬──┴───────┴──────────────────────────┘            │
│           │                                                   │
│           ▼                                                   │
│  디바이스가 DMA Write 수행 시:                                │
│  IOMMU가 자동으로 D(dirty) 비트를 1로 설정                   │
│                                                              │
│  소프트웨어가 주기적으로:                                    │
│  1. dirty bitmap 읽기                                        │
│  2. "어떤 페이지에 DMA write가 있었는지" 확인                │
│  3. dirty 비트 클리어                                        │
│  4. 반복                                                     │
└──────────────────────────────────────────────────────────────┘
```

### 3.2 커널 코드 분석

소스: `drivers/iommu/iommufd/hw_pagetable.c`

#### Dirty Tracking 활성화

```c
int iommufd_hwpt_set_dirty_tracking(struct iommufd_ucmd *ucmd)
{
    struct iommu_hwpt_set_dirty_tracking *cmd = ucmd->cmd;
    struct iommufd_hwpt_paging *hwpt_paging;
    bool enable;

    hwpt_paging = iommufd_get_hwpt_paging(ucmd, cmd->hwpt_id);

    enable = cmd->flags & IOMMU_HWPT_DIRTY_TRACKING_ENABLE;

    // IOAS의 IOMMU 도메인에 dirty tracking 설정
    rc = iopt_set_dirty_tracking(&ioas->iopt,
                                 hwpt_paging->common.domain, enable);
    return rc;
}
```

#### Dirty Bitmap 읽기

```c
int iommufd_hwpt_get_dirty_bitmap(struct iommufd_ucmd *ucmd)
{
    struct iommu_hwpt_get_dirty_bitmap *cmd = ucmd->cmd;
    struct iommufd_hwpt_paging *hwpt_paging;

    hwpt_paging = iommufd_get_hwpt_paging(ucmd, cmd->hwpt_id);

    // dirty 데이터 읽기 및 클리어
    rc = iopt_read_and_clear_dirty_data(
        &ioas->iopt, hwpt_paging->common.domain, cmd->flags, cmd);

    return rc;
}
```

#### Intel VT-d Dirty Tracking 구현

소스: `drivers/iommu/intel/iommu.c`

```c
static int intel_iommu_set_dirty_tracking(struct iommu_domain *domain, bool enable)
{
    struct dmar_domain *dmar_domain = to_dmar_domain(domain);

    if (dmar_domain->dirty_tracking == enable)
        return 0;

    // 각 디바이스에 대해 dirty tracking 설정
    ret = device_set_dirty_tracking(&dmar_domain->devices, enable);
    if (ret)
        return ret;

    // 중첩(nested) 도메인의 경우 부모 도메인도 설정
    ret = parent_domain_set_dirty_tracking(dmar_domain, enable);

    dmar_domain->dirty_tracking = enable;
    return 0;
}

static int device_set_dirty_tracking(struct list_head *devices, bool enable)
{
    struct device_domain_info *info;
    list_for_each_entry(info, devices, link) {
        // PASID 레벨에서 dirty tracking 설정
        ret = intel_pasid_setup_dirty_tracking(info->iommu, info->dev,
                                               IOMMU_NO_PASID, enable);
    }
    return ret;
}
```

### 3.3 사용 방법 (iommufd 인터페이스)

```c
/* 유저스페이스에서 iommufd를 통한 dirty tracking 사용 예시 */
#include <linux/iommufd.h>
#include <sys/ioctl.h>

int fd = open("/dev/iommu", O_RDWR);

// 1. Dirty tracking 활성화
struct iommu_hwpt_set_dirty_tracking set_dirty = {
    .size = sizeof(set_dirty),
    .hwpt_id = hwpt_id,
    .flags = IOMMU_HWPT_DIRTY_TRACKING_ENABLE,
};
ioctl(fd, IOMMU_HWPT_SET_DIRTY_TRACKING, &set_dirty);

// 2. 주기적으로 dirty bitmap 읽기
struct iommu_hwpt_get_dirty_bitmap get_dirty = {
    .size = sizeof(get_dirty),
    .hwpt_id = hwpt_id,
    .iova = start_iova,
    .length = region_size,
    .page_size = 4096,
    .data = (uintptr_t)bitmap_buffer,
    .flags = 0,  // 또는 IOMMU_HWPT_GET_DIRTY_BITMAP_NO_CLEAR
};
ioctl(fd, IOMMU_HWPT_GET_DIRTY_BITMAP, &get_dirty);

// 3. bitmap 분석
for (int i = 0; i < bitmap_size; i++) {
    if (bitmap_buffer[i / 8] & (1 << (i % 8))) {
        printf("Page at IOVA 0x%lx was written by DMA\n",
               start_iova + i * 4096);
    }
}
```

### 3.4 한계와 장점

```
장점:
  ✓ DMA를 전혀 방해하지 않음 (성능 영향 최소)
  ✓ 하드웨어가 자동으로 dirty 비트 설정 (오버헤드 거의 없음)
  ✓ 프로덕션 환경에서도 사용 가능

한계:
  ✗ 페이지 단위(4KB) 해상도 → 바이트 단위 추적 불가
  ✗ "언제" DMA가 발생했는지 모름 → 타이밍 정보 없음
  ✗ Write만 추적 가능 → DMA Read는 감지 불가
  ✗ 하드웨어 지원 필요 (Intel 3세대 이상 VT-d, AMD-Vi)
  ✗ iommufd 인터페이스 필요 → VFIO/KVM 환경에서 주로 사용
```

---

## 4. 방법 3: DMA Debug 프레임워크 (CONFIG_DMA_API_DEBUG)

### 4.1 원리

커널의 DMA API(dma_map_sg, dma_alloc_coherent 등)를 호출할 때마다 매핑 정보를 해시 테이블에 기록하는 디버깅 인프라이다. DMA 매핑의 생명주기(map → use → unmap)를 추적하고, 오용을 감지한다.

### 4.2 커널 코드 분석

소스: `kernel/dma/debug.c`

```c
// DMA 디버그 엔트리: 각 DMA 매핑을 추적
struct dma_debug_entry {
    struct list_head list;
    struct device    *dev;        // 디바이스
    u64              dev_addr;    // DMA 주소 (IOVA)
    u64              size;        // 매핑 크기
    int              type;        // single, sg, coherent, noncoherent, phy
    int              direction;   // DMA_TO_DEVICE, DMA_FROM_DEVICE, 등
    int              sg_call_ents;
    int              sg_mapped_ents;
    phys_addr_t      paddr;       // 물리 주소
    enum map_err_types map_err_type;
#ifdef CONFIG_STACKTRACE
    unsigned int     stack_len;
    unsigned long    stack_entries[DMA_DEBUG_STACKTRACE_ENTRIES]; // 호출 스택
#endif
};

// 해시 테이블로 관리 (16384개 버킷)
static struct hash_bucket dma_entry_hash[HASH_SIZE];  // HASH_SIZE = 16384
```

타입 분류:

```c
enum {
    dma_debug_single,       // dma_map_single()
    dma_debug_sg,           // dma_map_sg()
    dma_debug_coherent,     // dma_alloc_coherent()
    dma_debug_noncoherent,  // dma_alloc_noncoherent()
    dma_debug_phy,          // dma_map_phys()
};
```

### 4.3 DMA API에서의 debug 호출 위치

소스: `kernel/dma/mapping.c`

모든 DMA API 함수에서 debug 함수가 호출된다:

```c
dma_addr_t dma_map_phys(struct device *dev, phys_addr_t phys, size_t size, ...)
{
    // ... 실제 매핑 수행 ...
    trace_dma_map_phys(dev, phys, addr, size, dir, attrs);   // ftrace
    debug_dma_map_phys(dev, phys, size, dir, addr, attrs);   // DMA debug
    return addr;
}

void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, ...)
{
    // ... 실제 할당 수행 ...
    trace_dma_alloc(dev, cpu_addr, *dma_handle, size, ...);
    debug_dma_alloc_coherent(dev, size, *dma_handle, cpu_addr, attrs);
    return cpu_addr;
}
```

### 4.4 활성화 및 사용법

```bash
# 1. 커널 빌드 시 설정
CONFIG_DMA_API_DEBUG=y

# 2. 부트 파라미터 (필요시)
# 기본적으로 CONFIG_DMA_API_DEBUG=y이면 자동 활성화
# 비활성화하려면:
dma_debug=0

# 3. debugfs 인터페이스 확인
mount -t debugfs none /sys/kernel/debug

# 현재 활성 DMA 매핑 덤프
cat /sys/kernel/debug/dma-api/dump

# 에러 카운트
cat /sys/kernel/debug/dma-api/error_count

# 모든 에러 표시 활성화
echo 1 > /sys/kernel/debug/dma-api/all_errors

# 특정 드라이버만 필터링
echo "nvme" > /sys/kernel/debug/dma-api/driver_filter

# 통계 정보
cat /sys/kernel/debug/dma-api/num_errors
cat /sys/kernel/debug/dma-api/num_free_entries
```

### 4.5 NVMe에서 얻을 수 있는 정보

```bash
# NVMe 드라이버의 DMA 매핑 정보 확시
cat /sys/kernel/debug/dma-api/dump | grep nvme

# 출력 예시:
# nvme 0000:01:00.0 coherent  dev_addr=0x00000000ff800000 size=16384
#   → SQ/CQ coherent DMA 할당 (dma_alloc_coherent)
# nvme 0000:01:00.0 sg        dev_addr=0x0000000100000000 size=4096 dir=FROM_DEVICE
#   → 데이터 전송용 streaming DMA 매핑 (dma_map_sg)
```

```
얻을 수 있는 정보:
  ✓ 어떤 디바이스가 DMA 매핑을 요청했는지
  ✓ DMA 주소(IOVA)와 물리 주소
  ✓ 매핑 크기
  ✓ 방향 (TO_DEVICE, FROM_DEVICE, BIDIRECTIONAL)
  ✓ 타입 (coherent, streaming, etc.)
  ✓ 호출 스택 (CONFIG_STACKTRACE 필요)

얻을 수 없는 정보:
  ✗ 실제 DMA가 "언제" 수행되었는지 (매핑 시점만 알 수 있음)
  ✗ DMA로 전송된 데이터 내용
  ✗ DMA 접근 패턴 (몇 번 접근했는지)
```

---

## 5. 방법 4: ftrace/kprobe로 DMA 관련 함수 추적

### 5.1 DMA tracepoint 인프라

소스: `include/trace/events/dma.h`, `kernel/dma/mapping.c`

커널은 `#define CREATE_TRACE_POINTS` / `#include <trace/events/dma.h>`를 통해 DMA 관련 tracepoint를 정의한다.

사용 가능한 tracepoint 목록:

```
dma:dma_map_phys          - 물리 주소 DMA 매핑
dma:dma_unmap_phys        - 물리 주소 DMA 매핑 해제
dma:dma_map_sg            - scatter-gather DMA 매핑
dma:dma_map_sg_err        - scatter-gather DMA 매핑 에러
dma:dma_unmap_sg          - scatter-gather DMA 매핑 해제
dma:dma_alloc             - coherent DMA 메모리 할당
dma:dma_alloc_pages       - DMA 페이지 할당
dma:dma_alloc_sgt         - scatter-gather table 할당
dma:dma_free              - coherent DMA 메모리 해제
dma:dma_free_pages        - DMA 페이지 해제
dma:dma_free_sgt          - scatter-gather table 해제
dma:dma_sync_single_for_cpu    - CPU용 동기화
dma:dma_sync_single_for_device - 디바이스용 동기화
dma:dma_sync_sg_for_cpu        - SG CPU용 동기화
dma:dma_sync_sg_for_device     - SG 디바이스용 동기화
```

tracepoint의 출력 형식 (dma_map_phys 예시):

```c
// include/trace/events/dma.h
DECLARE_EVENT_CLASS(dma_map,
    TP_PROTO(struct device *dev, phys_addr_t phys_addr, dma_addr_t dma_addr,
             size_t size, enum dma_data_direction dir, unsigned long attrs),
    // ...
    TP_printk("%s dir=%s dma_addr=%llx size=%zu phys_addr=%llx attrs=%s",
        __get_str(device),
        decode_dma_data_direction(__entry->dir),
        __entry->dma_addr,
        __entry->size,
        __entry->phys_addr,
        decode_dma_attrs(__entry->attrs))
);
```

### 5.2 trace-cmd로 DMA 매핑 추적

```bash
# 모든 DMA 이벤트 추적
trace-cmd record -e dma

# DMA 매핑만 추적
trace-cmd record -e dma:dma_map_phys -e dma:dma_map_sg

# function_graph로 DMA 매핑 함수 내부 추적
trace-cmd record -p function_graph -g dma_map_sg_attrs -g dma_alloc_attrs

# NVMe 디바이스만 필터링
trace-cmd record -e dma:dma_map_sg \
    -f 'device == "0000:01:00.0"'

# 결과 확인
trace-cmd report
```

### 5.3 bpftrace로 DMA 추적

```bash
# DMA 매핑 시점의 주소/크기/방향 출력
bpftrace -e '
tracepoint:dma:dma_map_phys {
    printf("%-20s phys=0x%llx dma=0x%llx size=%zu dir=%s\n",
           str(args.device),
           args.phys_addr,
           args.dma_addr,
           args.size,
           args.dir == 0 ? "BIDIR" :
           args.dir == 1 ? "TO_DEV" :
           args.dir == 2 ? "FROM_DEV" : "NONE");
}
'

# NVMe 드라이버의 DMA 매핑 추적 (kprobe 사용)
bpftrace -e '
kprobe:nvme_map_data {
    printf("nvme_map_data called, comm=%s pid=%d\n", comm, pid);
}

kretprobe:nvme_map_data {
    printf("nvme_map_data returned %d\n", retval);
}
'

# NVMe I/O 완료 시점 추적
bpftrace -e '
kprobe:nvme_irq {
    printf("nvme_irq on CPU %d\n", cpu);
}

kprobe:nvme_handle_cqe {
    printf("nvme_handle_cqe called\n");
}
'

# DMA 주소 범위 히스토그램
bpftrace -e '
tracepoint:dma:dma_map_phys /str(args.device) == "0000:01:00.0"/ {
    @addr_hist = lhist(args.dma_addr >> 20, 0, 1024, 16);
    @size_hist = hist(args.size);
}
'
```

### 5.4 NVMe I/O 레이턴시 측정 bpftrace 스크립트

```bash
#!/usr/bin/env bpftrace
/*
 * nvme_dma_latency.bt - NVMe DMA 매핑부터 I/O 완료까지 레이턴시 측정
 */

kprobe:nvme_queue_rq {
    @start[tid] = nsecs;
}

kprobe:nvme_handle_cqe {
    // CQE 처리 시점에서의 레이턴시
    printf("CQE processed at %llu ns\n", nsecs);
}

tracepoint:dma:dma_map_sg /str(args.device) == "0000:01:00.0"/ {
    @dma_map_time[tid] = nsecs;
    printf("DMA map: nents=%d dma_addr[0]=%llx\n",
           args.full_nents, args.dma_addr);
}

tracepoint:dma:dma_unmap_sg /str(args.device) == "0000:01:00.0"/ {
    $duration = nsecs - @dma_map_time[tid];
    @dma_duration_us = hist($duration / 1000);
    delete(@dma_map_time[tid]);
}
```

### 5.5 NVMe DMA 관련 함수 추적 (function_graph)

```bash
# NVMe I/O 제출부터 완료까지의 함수 호출 체인
trace-cmd record -p function_graph \
    -g nvme_queue_rq \
    -g nvme_irq \
    -g nvme_poll \
    -l 'dma_map_*' \
    -l 'dma_unmap_*' \
    -l 'dma_alloc_*'

# 결과 예시:
#  nvme_queue_rq() {
#    nvme_map_data() {
#      dma_map_sg_attrs() {
#        __dma_map_sg_attrs() {
#          iommu_dma_map_sg();
#        }
#      }
#    }
#    nvme_submit_cmd();
#  }
```

---

## 6. 방법 5: blktrace / block tracepoints

### 6.1 blktrace 사용법

```bash
# blktrace 시작 (백그라운드)
blktrace -d /dev/nvme0n1 -o trace &

# I/O 부하 발생
fio --name=test --filename=/dev/nvme0n1 --rw=randread --bs=4k \
    --numjobs=1 --iodepth=32 --runtime=10

# blktrace 종료
kill %1

# 결과 파싱
blkparse -i trace -o result.txt

# 또는 실시간 모니터링
blktrace -d /dev/nvme0n1 -o - | blkparse -i -
```

### 6.2 이벤트 종류와 의미

```
블록 I/O 이벤트 흐름:

  Q (Queue)      - bio가 request queue에 추가됨
      ↓
  G (Get request)- request 구조체 할당됨
      ↓
  I (Insert)     - I/O 스케줄러에 삽입됨
      ↓
  D (Dispatch)   - 드라이버로 디스패치됨 (NVMe SQ에 제출)
      ↓
  C (Complete)   - I/O 완료 (NVMe CQ에서 처리됨)
```

```bash
# blkparse 출력 예시:
#  8,0   1    1  0.000000000  1234  Q   R 0 + 8 [fio]
#  8,0   1    2  0.000001200  1234  G   R 0 + 8 [fio]
#  8,0   1    3  0.000002500  1234  I   R 0 + 8 [fio]
#  8,0   1    4  0.000005000  1234  D   R 0 + 8 [fio]
#  8,0   0    1  0.000050000     0  C   R 0 + 8 [0]
#
# 형식: device CPU seq timestamp PID event R/W sector + size [process]
```

```bash
# trace-cmd으로 block tracepoints 사용
trace-cmd record -e block:block_rq_insert \
                 -e block:block_rq_issue \
                 -e block:block_rq_complete \
                 -e block:block_bio_queue

# 특정 디바이스만 필터링
trace-cmd record -e block:block_rq_complete \
    -f 'dev == 0x10300'   # major:minor를 hex로
```

### 6.3 bcc/bpftrace로 block 레이어 추적

```bash
# biolatency (BCC 도구)
biolatency-bpfcc -D

# biosnoop (각 I/O의 상세 정보)
biosnoop-bpfcc -d nvme0n1

# 커스텀 bpftrace: NVMe 요청의 D→C 레이턴시
bpftrace -e '
tracepoint:block:block_rq_issue /args.dev >> 20 == 259/ {
    @start[args.sector] = nsecs;
}

tracepoint:block:block_rq_complete /args.dev >> 20 == 259/ {
    $lat = nsecs - @start[args.sector];
    @lat_us = hist($lat / 1000);
    delete(@start[args.sector]);
}
'
```

### 6.4 한계

```
blktrace/block tracepoints의 한계:
  ✗ 블록 레이어 수준의 정보만 제공
  ✗ 실제 DMA 주소/물리 주소 정보 없음
  ✗ DMA 전송 데이터 내용 확인 불가
  ✗ 디바이스가 실제로 어떤 메모리 주소에 접근하는지 알 수 없음
  ✗ NVMe 프로토콜 레벨의 세부사항 (SQE/CQE 내용) 확인 불가

  → 블록 레이어 "위에서" I/O 흐름을 관찰하는 것이므로,
    "디바이스가 호스트 메모리의 어디에 DMA하는가"라는 질문에는 답할 수 없음
```

---

## 7. 방법 6: QEMU 코드 수정 (가장 강력)

### 7.1 원리

QEMU에서 에뮬레이션되는 NVMe 디바이스는 모든 DMA를 `pci_dma_read()`, `pci_dma_write()`, `nvme_addr_read()` 같은 함수를 통해 수행한다. 이 함수들에 로깅을 추가하면 **모든 Device→Host DMA를 100% 가로챌 수 있다**.

```
┌──────────────────────────────────────────────────────────────┐
│                  QEMU 기반 DMA 추적                          │
│                                                              │
│  QEMU NVMe 에뮬레이션:                                      │
│                                                              │
│  nvme_process_sq()                                           │
│    └─ nvme_addr_read(addr, &cmd)  ← SQ에서 명령어 읽기      │
│         │                            (Device reads Host)     │
│         ▼ 여기에 로깅 추가!                                  │
│                                                              │
│  nvme_post_cqes()                                            │
│    └─ pci_dma_write(addr, &cqe)   ← CQ에 완료 엔트리 쓰기   │
│         │                            (Device writes Host)    │
│         ▼ 여기에 로깅 추가!                                  │
│                                                              │
│  nvme_map_prp()                                              │
│    └─ nvme_addr_read(prp2, prp_list) ← PRP 리스트 읽기      │
│         │                                                    │
│         ▼ 여기에 로깅 추가!                                  │
│                                                              │
│  데이터 전송 (Read/Write)                                    │
│    └─ dma_memory_read/write()      ← 실제 데이터 전송       │
│         │                                                    │
│         ▼ 여기에 로깅 추가!                                  │
└──────────────────────────────────────────────────────────────┘
```

### 7.2 QEMU ctrl.c 코드 분석

소스: `hw/nvme/ctrl.c`

#### SQ Fetch (Device reads Host memory)

```c
static void nvme_process_sq(void *opaque)
{
    NvmeSQueue *sq = opaque;
    NvmeCtrl *n = sq->ctrl;
    hwaddr addr;
    NvmeCmd cmd;

    while (!(nvme_sq_empty(sq) || QTAILQ_EMPTY(&sq->req_list))) {
        addr = sq->dma_addr + (sq->head << NVME_SQES);

        // ★ Device가 Host memory에서 SQ 엔트리를 DMA read
        if (nvme_addr_read(n, addr, (void *)&cmd, sizeof(cmd))) {
            trace_pci_nvme_err_addr_read(addr);
            stl_le_p(&n->bar.csts, NVME_CSTS_FAILED);
            break;
        }
        // ... 명령어 처리
    }
}
```

#### CQ Write (Device writes Host memory)

```c
static void nvme_post_cqes(void *opaque)
{
    NvmeCQueue *cq = opaque;
    NvmeCtrl *n = cq->ctrl;
    NvmeRequest *req, *next;

    QTAILQ_FOREACH_SAFE(req, &cq->req_list, entry, next) {
        hwaddr addr;

        req->cqe.status = cpu_to_le16((req->status << 1) | cq->phase);
        req->cqe.sq_id = cpu_to_le16(sq->sqid);
        req->cqe.sq_head = cpu_to_le16(sq->head);
        addr = cq->dma_addr + (cq->tail << NVME_CQES);

        // ★ Device가 Host memory에 CQE를 DMA write
        ret = pci_dma_write(PCI_DEVICE(n), addr, (void *)&req->cqe,
                            sizeof(req->cqe));
        // ...
    }
}
```

#### PRP List Fetch

```c
static uint16_t nvme_map_prp(NvmeCtrl *n, NvmeSg *sg, uint64_t prp1,
                              uint64_t prp2, uint32_t len)
{
    // ★ PRP 리스트를 DMA read로 가져옴
    ret = nvme_addr_read(n, prp2, (void *)prp_list, prp_trans);
    // ... PRP 엔트리별로 반복하며 추가 PRP 리스트도 읽음
}
```

### 7.3 DMA 추적 코드 구현 예시

```c
/* hw/nvme/ctrl.c에 추가할 DMA 추적 코드 */

/* 추적 레벨 제어 */
static int nvme_dma_trace_level = 0;  /* 0=off, 1=summary, 2=detail, 3=data */

static void nvme_trace_dma(const char *op, hwaddr addr,
                           uint64_t size, const void *data)
{
    if (!nvme_dma_trace_level) return;

    fprintf(stderr, "[NVMe DMA] %s addr=0x%"PRIx64" size=%"PRIu64,
            op, (uint64_t)addr, size);

    if (nvme_dma_trace_level >= 3 && data && size <= 64) {
        fprintf(stderr, " data=");
        const uint8_t *p = data;
        for (uint64_t i = 0; i < size && i < 64; i++)
            fprintf(stderr, "%02x", p[i]);
    }
    fprintf(stderr, "\n");
}

/* nvme_process_sq()에 추가 */
static void nvme_process_sq(void *opaque)
{
    // ...
    if (nvme_addr_read(n, addr, (void *)&cmd, sizeof(cmd))) {
        // error handling
    }
    nvme_trace_dma("SQ_FETCH", addr, sizeof(cmd), &cmd);
    // NVMe 명령어 세부사항 출력
    if (nvme_dma_trace_level >= 2) {
        fprintf(stderr, "  [SQE] opcode=0x%02x cid=%u nsid=%u\n",
                cmd.opcode, le16_to_cpu(cmd.cid), le32_to_cpu(cmd.nsid));
    }
    // ...
}

/* nvme_post_cqes()에 추가 */
static void nvme_post_cqes(void *opaque)
{
    // ...
    ret = pci_dma_write(PCI_DEVICE(n), addr, (void *)&req->cqe,
                        sizeof(req->cqe));
    nvme_trace_dma("CQ_WRITE", addr, sizeof(req->cqe), &req->cqe);
    if (nvme_dma_trace_level >= 2) {
        fprintf(stderr, "  [CQE] sqid=%u cid=%u status=0x%04x\n",
                le16_to_cpu(req->cqe.sq_id),
                le16_to_cpu(req->cqe.cid),
                le16_to_cpu(req->cqe.status) >> 1);
    }
    // ...
}
```

### 7.4 QEMU trace-events 활용

QEMU 자체에도 빌트인 trace 시스템이 있다. NVMe 관련 trace-events가 이미 정의되어 있다.

소스: `hw/nvme/trace-events`

```
# 이미 정의된 주요 trace-events:
pci_nvme_dma_read(uint64_t prp1, uint64_t prp2)
pci_nvme_map_addr(uint64_t addr, uint64_t len)
pci_nvme_map_prp(uint64_t trans_len, uint32_t len, uint64_t prp1, uint64_t prp2, int num_prps)
pci_nvme_io_cmd(uint16_t cid, uint32_t nsid, uint16_t sqid, uint8_t opcode, const char *opname)
pci_nvme_rw_cb(uint16_t cid, const char *blkname)
```

QEMU trace-events 사용법:

```bash
# QEMU 실행 시 trace 활성화
qemu-system-x86_64 \
    -trace "pci_nvme_*" \
    -trace "pci_nvme_map_addr" \
    -trace "pci_nvme_map_prp" \
    -trace "pci_nvme_dma_read" \
    ...

# 또는 trace 이벤트를 파일에 기록
qemu-system-x86_64 \
    -trace events=/tmp/trace-events.txt \
    -trace file=/tmp/trace.log \
    ...

# /tmp/trace-events.txt 내용:
# pci_nvme_map_addr
# pci_nvme_map_prp
# pci_nvme_io_cmd
# pci_nvme_rw_cb

# 런타임에 trace 켜기/끄기 (QMP 사용)
# (qemu) trace-event pci_nvme_map_addr on
# (qemu) trace-event pci_nvme_map_addr off
```

### 7.5 장점

```
QEMU 기반 DMA 추적의 장점:
  ✓ 모든 DMA 방향 추적 가능 (Device→Host, Host→Device 모두)
  ✓ 정확한 DMA 주소, 크기 정보
  ✓ 전송 데이터 내용까지 확인 가능 (바이트 레벨)
  ✓ NVMe 프로토콜 수준의 의미있는 정보
    (어떤 명령어의 DMA인지, SQ/CQ/데이터인지)
  ✓ 시스템을 중단하지 않고 추적 가능
  ✓ IOMMU 유무와 관계없이 동작

한계:
  ✗ QEMU 환경에서만 사용 가능 (실제 하드웨어 불가)
  ✗ 에뮬레이션 오버헤드로 인한 타이밍 부정확
  ✗ 실제 NVMe 디바이스와 동작이 다를 수 있음
```

---

## 8. 방법 7: perf + PMU 이벤트

### 8.1 IOMMU PMU 이벤트

Intel VT-d IOMMU는 Performance Monitoring Unit(PMU)를 통해 DMA 트래픽 통계를 제공한다.

```bash
# 사용 가능한 IOMMU PMU 이벤트 확인
perf list | grep -i iommu

# IOMMU PMU 이벤트 예시 (하드웨어 의존적):
# dmar0/iommu_clocks/
# dmar0/iommu_requests/

# perf stat으로 IOMMU 이벤트 모니터링
perf stat -e 'dmar0/iommu_clocks/' \
          -e 'dmar0/iommu_requests/' \
          -I 1000 \
          -- sleep 10

# 특정 프로세스의 DMA 관련 카운터
perf stat -e 'dmar0/iommu_clocks/' -p $(pgrep fio) -- sleep 5
```

### 8.2 PCIe PMU (Uncore)

Intel 프로세서의 Uncore PMU에서 PCIe 트래픽 카운터를 읽을 수 있다.

```bash
# Uncore PMU 이벤트 확인
perf list | grep -i uncore | grep -i pci

# PCIe 트래픽 카운터 (플랫폼 의존적)
# 예: Intel Skylake 이후
perf stat -e 'uncore_iio_0/event=0x83,umask=0x01/' \  # PCIe inbound reads
          -e 'uncore_iio_0/event=0x83,umask=0x04/' \  # PCIe inbound writes
          -I 1000 -- sleep 10

# 결과 예시:
# 1.001 sec  1,234,567  uncore_iio_0/event=0x83,umask=0x01/
# 1.001 sec    456,789  uncore_iio_0/event=0x83,umask=0x04/
```

### 8.3 한계

```
PMU 기반 추적의 한계:
  ✗ 통계 정보만 제공 (카운터, 합계)
  ✗ 개별 DMA 트랜잭션 추적 불가
  ✗ DMA 주소/데이터 내용 확인 불가
  ✗ 하드웨어/플랫폼별로 사용 가능한 이벤트가 다름
  ✗ Uncore PMU는 문서화가 부족한 경우가 많음

  → "DMA 트래픽 양"을 측정하는 데는 유용하지만,
    개별 DMA 트랜잭션을 추적하는 용도로는 부적합
```

---

## 9. 방법 8: /dev/mem으로 DMA 영역 직접 관찰

### 9.1 원리

NVMe의 SQ/CQ는 `dma_alloc_coherent()`로 할당된 메모리 영역이다. 이 영역의 물리 주소를 알고 있으면 `/dev/mem`을 통해 직접 읽어서 디바이스가 어떤 데이터를 썼는지 관찰할 수 있다.

### 9.2 CQ 물리 주소 확인

```bash
# NVMe CQ의 물리 주소 확인 방법 1: DMA debug 프레임워크
cat /sys/kernel/debug/dma-api/dump | grep nvme | grep coherent

# 방법 2: /proc/iomem에서 NVMe 관련 영역 확인
cat /proc/iomem | grep -i nvme

# 방법 3: bpftrace로 dma_alloc_coherent 시점 포착
bpftrace -e '
kretprobe:dma_alloc_coherent {
    printf("dma_alloc_coherent returned vaddr=%p, comm=%s\n",
           retval, comm);
}
'
```

### 9.3 /dev/mem으로 CQ 관찰

```c
/* cq_monitor.c - NVMe CQ DMA 영역 직접 관찰 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

struct nvme_cqe {
    uint32_t result;
    uint32_t rsvd;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t cid;
    uint16_t status;
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <phys_addr> <queue_depth>\n", argv[0]);
        return 1;
    }

    uint64_t phys_addr = strtoull(argv[1], NULL, 0);
    int depth = atoi(argv[2]);

    int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    size_t size = depth * sizeof(struct nvme_cqe);
    void *map = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, phys_addr);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    struct nvme_cqe *cq = (struct nvme_cqe *)map;

    /* 폴링하면서 CQ 변경 감지 */
    uint16_t prev_status[depth];
    memset(prev_status, 0, sizeof(prev_status));

    while (1) {
        for (int i = 0; i < depth; i++) {
            if (cq[i].status != prev_status[i]) {
                printf("[CQE %d] sqid=%u cid=%u status=0x%04x sq_head=%u\n",
                       i, cq[i].sq_id, cq[i].cid,
                       cq[i].status >> 1, cq[i].sq_head);
                prev_status[i] = cq[i].status;
            }
        }
        usleep(100);  /* 100us 간격 폴링 */
    }

    munmap(map, size);
    close(fd);
    return 0;
}
```

### 9.4 보안 제한

```bash
# CONFIG_STRICT_DEVMEM이 활성화된 경우 (대부분의 배포판):
# /dev/mem은 PCI/MMIO 영역만 접근 가능하고 일반 RAM은 접근 불가

# 확인:
grep STRICT_DEVMEM /boot/config-$(uname -r)
# CONFIG_STRICT_DEVMEM=y        ← RAM 접근 불가
# CONFIG_IO_STRICT_DEVMEM=y     ← 드라이버가 claim한 I/O도 불가

# 커널 부트 파라미터로 비활성화 (보안 위험!):
# iomem=relaxed

# 또는 CONFIG_STRICT_DEVMEM=n으로 커널 재빌드
```

---

## 10. 방법 9: Custom Kernel Module

### 10.1 원리

직접 커널 모듈을 작성하여 DMA 영역을 모니터링한다. `dma_alloc_coherent()`로 할당한 영역의 커널 가상 주소는 이미 매핑되어 있으므로, 직접 읽어서 변경을 감지할 수 있다.

### 10.2 구현 골격 코드

```c
/* dma_monitor.c - NVMe CQ/SQ DMA 영역 모니터링 커널 모듈 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/nvme.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("NVMe DMA Region Monitor");

static char *pci_slot = "0000:01:00.0";
module_param(pci_slot, charp, 0444);

static unsigned long cq_vaddr = 0;
module_param(cq_vaddr, ulong, 0444);
MODULE_PARM_DESC(cq_vaddr, "Virtual address of CQ DMA region");

static int cq_depth = 1024;
module_param(cq_depth, int, 0444);

static int poll_interval_us = 100;
module_param(poll_interval_us, int, 0644);

static struct task_struct *monitor_thread;
static u8 *prev_snapshot;

/* CQE 변경 감지 및 로깅 */
static void check_cq_changes(void *cq_base)
{
    struct nvme_completion *cq = cq_base;
    struct nvme_completion *prev = (struct nvme_completion *)prev_snapshot;
    int i;

    for (i = 0; i < cq_depth; i++) {
        if (cq[i].status != prev[i].status ||
            cq[i].command_id != prev[i].command_id) {
            pr_info("[DMA Monitor] CQE[%d] changed: "
                    "sqid=%u cid=%u status=0x%04x result=0x%x\n",
                    i, le16_to_cpu(cq[i].sq_id),
                    le16_to_cpu(cq[i].command_id),
                    le16_to_cpu(cq[i].status) >> 1,
                    le32_to_cpu(cq[i].result.u32));
            prev[i] = cq[i];
        }
    }
}

static int monitor_fn(void *data)
{
    void *cq_base = (void *)cq_vaddr;

    pr_info("DMA Monitor: started, monitoring CQ at %px\n", cq_base);

    while (!kthread_should_stop()) {
        check_cq_changes(cq_base);
        usleep_range(poll_interval_us, poll_interval_us + 10);
    }

    pr_info("DMA Monitor: stopped\n");
    return 0;
}

static int __init dma_monitor_init(void)
{
    if (!cq_vaddr) {
        pr_err("DMA Monitor: cq_vaddr parameter required\n");
        return -EINVAL;
    }

    prev_snapshot = kzalloc(cq_depth * sizeof(struct nvme_completion),
                            GFP_KERNEL);
    if (!prev_snapshot)
        return -ENOMEM;

    monitor_thread = kthread_run(monitor_fn, NULL, "dma_monitor");
    if (IS_ERR(monitor_thread)) {
        kfree(prev_snapshot);
        return PTR_ERR(monitor_thread);
    }

    return 0;
}

static void __exit dma_monitor_exit(void)
{
    if (monitor_thread)
        kthread_stop(monitor_thread);
    kfree(prev_snapshot);
}

module_init(dma_monitor_init);
module_exit(dma_monitor_exit);
```

```bash
# 빌드
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# CQ의 가상 주소를 먼저 확인 (bpftrace 등으로)
# 그 후 모듈 로드
insmod dma_monitor.ko cq_vaddr=0xffff888123456000 cq_depth=1024

# 모니터링 결과 확인
dmesg -w | grep "DMA Monitor"

# 언로드
rmmod dma_monitor
```

### 10.3 더 발전된 접근: NVMe 드라이버 패치

NVMe 드라이버 자체를 패치하여 DMA 영역의 변경을 감시하는 방법도 있다. `nvme_handle_cqe()` 함수에서 CQE를 처리하기 직전에 로깅을 추가하면, 디바이스가 쓴 CQE의 정확한 내용을 확인할 수 있다.

```c
/* drivers/nvme/host/pci.c 패치 예시 */
static inline void nvme_handle_cqe(struct nvme_queue *nvmeq, u16 idx)
{
    struct nvme_completion *cqe = &nvmeq->cqes[idx];

    /* ★ DMA로 쓰여진 CQE 내용 로깅 */
    pr_debug("CQE[%u] DMA write detected: "
             "sqid=%u cid=%u status=0x%04x result=0x%x\n",
             idx, le16_to_cpu(cqe->sq_id),
             le16_to_cpu(cqe->command_id),
             le16_to_cpu(cqe->status) >> 1,
             le32_to_cpu(cqe->result.u32));

    // ... 원래 처리 코드
}
```

---

## 11. 종합 비교표

```
┌─────────────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ 방법            │ 추적 방향│ 실시간   │데이터내용│DMA 방해  │ 난이도   │ HW 요구  │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 1. IOMMU Fault  │ 양방향   │ ○       │ ✗        │ ★★★    │ ★★★★ │ IOMMU    │
│    Hooking      │          │(fault시) │          │(DMA실패) │          │+PRI      │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 2. IOMMU Dirty  │ Write만  │ △       │ ✗        │ 없음     │ ★★★  │ IOMMU    │
│    Tracking     │          │(주기적) │          │          │          │(최신)    │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 3. DMA Debug    │ 매핑정보 │ ○       │ ✗        │ 없음     │ ★       │ 없음     │
│    Framework    │ (양방향) │          │          │(약간     │          │          │
│                 │          │          │          │오버헤드) │          │          │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 4. ftrace/      │ 매핑정보 │ ○       │ ✗        │ 없음     │ ★★     │ 없음     │
│    kprobe/BPF   │ + 함수   │          │          │(약간     │          │          │
│                 │          │          │          │오버헤드) │          │          │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 5. blktrace     │ 블록I/O  │ ○       │ ✗        │ 없음     │ ★       │ 없음     │
│                 │ (간접)   │          │          │          │          │          │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 6. QEMU 수정    │ 양방향   │ ○       │ ○        │ 없음     │ ★★     │ QEMU     │
│                 │ (완벽)   │          │(완벽)    │          │          │ 환경     │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 7. perf/PMU     │ 양방향   │ ○       │ ✗        │ 없음     │ ★★     │ PMU      │
│                 │ (통계)   │          │          │          │          │ 지원     │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 8. /dev/mem     │ 양방향   │ △       │ ○        │ 없음     │ ★★     │ 없음     │
│                 │ (폴링)   │(폴링)   │(읽기가능)│          │          │(DEVMEM)  │
├─────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 9. Custom       │ 양방향   │ ○       │ ○        │ 없음     │ ★★★  │ 없음     │
│    Kernel Module│ (폴링)   │          │(읽기가능)│          │          │          │
└─────────────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘

범례:
  ○ = 가능/지원    △ = 부분적    ✗ = 불가/미지원
  ★ = 난이도 (많을수록 어려움)
```

---

## 12. NVMe DMA 추적 실전 가이드

### 12.1 목적별 추천 방법

#### "DMA 매핑만 확인하고 싶다" → DMA Debug + bpftrace

```bash
# 1. DMA Debug 활성화 (커널 빌드 시)
CONFIG_DMA_API_DEBUG=y

# 2. 현재 활성 DMA 매핑 확인
cat /sys/kernel/debug/dma-api/dump | grep nvme

# 3. 실시간 DMA 매핑 이벤트 추적
bpftrace -e '
tracepoint:dma:dma_map_phys /str(args.device) =~ "nvme"/ {
    printf("MAP  phys=0x%llx dma=0x%llx size=%zu dir=%d\n",
           args.phys_addr, args.dma_addr, args.size, args.dir);
}
tracepoint:dma:dma_unmap_phys /str(args.device) =~ "nvme"/ {
    printf("UMAP dma=0x%llx size=%zu dir=%d\n",
           args.addr, args.size, args.dir);
}
'
```

#### "I/O 레이턴시를 분석하고 싶다" → blktrace + bpftrace

```bash
# 1. blktrace로 전체 I/O 흐름 기록
blktrace -d /dev/nvme0n1 -w 10 -o trace

# 2. bpftrace로 세부 레이턴시 분석
bpftrace -e '
kprobe:nvme_queue_rq {
    @submit[tid] = nsecs;
}

tracepoint:block:block_rq_complete /args.dev >> 20 == 259/ {
    if (@submit[tid]) {
        @latency_us = hist((nsecs - @submit[tid]) / 1000);
        delete(@submit[tid]);
    }
}
'

# 3. 결과를 종합하여 병목 구간 식별
blkparse -i trace -d trace.bin
btt -i trace.bin    # Q2Q, Q2C, D2C 등 구간별 레이턴시
```

#### "디바이스가 정확히 어디에 DMA하는지 알고 싶다" → QEMU 수정

```bash
# 1. QEMU NVMe 디바이스에 DMA 로깅 추가 (위의 7.3 참고)

# 2. QEMU 빌드 및 실행
cd ~/sources/qemu && mkdir build && cd build
../configure --target-list=x86_64-softmmu
make -j$(nproc)

# 3. QEMU 실행 (NVMe 디바이스 포함)
./qemu-system-x86_64 \
    -drive file=test.qcow2,if=none,id=nvm \
    -device nvme,serial=deadbeef,drive=nvm \
    -trace "pci_nvme_*" \
    -nographic

# 4. 게스트에서 I/O 발생 → stderr에 DMA 로그 출력
```

#### "프로덕션 환경에서 모니터링" → ftrace + perf

```bash
# 1. 경량 ftrace 추적 (오버헤드 최소)
echo 1 > /sys/kernel/debug/tracing/events/dma/dma_map_phys/enable
echo 'device ~ "nvme*"' > /sys/kernel/debug/tracing/events/dma/dma_map_phys/filter
cat /sys/kernel/debug/tracing/trace_pipe

# 2. perf로 DMA 관련 통계 수집
perf stat -e 'dmar0/iommu_requests/' -I 5000 -- sleep 60

# 3. 이상 감지 시 상세 추적으로 전환
trace-cmd record -e dma -e block -p 10 -- sleep 30
```

#### "IOMMU 보안 분석" → IOMMU fault injection

```bash
# 1. IOMMU fault 로그 모니터링
dmesg -w | grep -i "DMAR\|IOMMU\|fault"

# 2. IOMMU 그룹 정보 확인
for d in /sys/kernel/iommu_groups/*/devices/*; do
    echo "Group $(basename $(dirname $(dirname $d))): $(basename $d)"
done

# 3. 특정 디바이스의 IOMMU 도메인 정보
cat /sys/kernel/debug/iommu/intel/dmar*/domain_translation_struct
```

---

## 13. mmiotrace의 IOMMU 버전: 구현 가능성 분석

### 13.1 핵심 질문

mmiotrace처럼 "DMA를 가로채고 → 기록하고 → 통과시키는" 메커니즘을 IOMMU로 구현할 수 있는가?

```
mmiotrace 모델:
  CPU ─→ [PTE non-present] ─→ page fault ─→ 기록 ─→ PTE 복원 ─→ single-step ─→ 재무장

이것의 IOMMU 버전:
  Device ─→ [IOMMU 매핑 제거] ─→ IOMMU fault ─→ 기록 ─→ 매핑 복구 ─→ DMA 재시도 ─→ 재제거
                                                                          ↑
                                                              여기가 핵심 문제!
```

### 13.2 PRI (Page Request Interface)

PRI는 PCIe 규격의 일부로, 디바이스가 IOMMU page fault 발생 시 자동으로 재시도할 수 있게 해주는 메커니즘이다.

```
PRI 동작 흐름:
  1. Device가 DMA 시도
  2. IOMMU에서 매핑 없음 → fault
  3. IOMMU가 Device에 "Page Request" 전송
  4. Device가 DMA를 중지하고 대기
  5. 소프트웨어가 매핑 설정
  6. 소프트웨어가 "Page Response" 전송
  7. Device가 DMA 재시도
```

커널에서의 PRI 지원 (`drivers/iommu/io-pgfault.c`):

```c
// IOPF 큐 할당 - PRI를 지원하는 디바이스를 위한 큐
struct iopf_queue *iopf_queue_alloc(const char *name)
{
    queue->wq = alloc_workqueue("iopf_queue/%s", WQ_UNBOUND, 0, name);
    INIT_LIST_HEAD(&queue->devices);
    return queue;
}

// 디바이스를 IOPF 큐에 추가
int iopf_queue_add_device(struct iopf_queue *queue, struct device *dev)
{
    // page_response 콜백이 있어야 함
    if (!ops->page_response)
        return -ENODEV;

    fault_param = kzalloc(sizeof(*fault_param), GFP_KERNEL);
    INIT_LIST_HEAD(&fault_param->faults);
    INIT_LIST_HEAD(&fault_param->partial);
    // ...
}

// Fault 보고 및 처리
int iommu_report_device_fault(struct device *dev, struct iopf_fault *evt)
{
    // fault handler 찾기
    attach_handle = find_fault_handler(dev, evt);

    // fault 그룹 할당 및 handler 호출
    group = iopf_group_alloc(iopf_param, evt, &abort_group);
    group->attach_handle->domain->iopf_handler(group);
}

// 응답 전송 (매핑 복구 후)
void iopf_group_response(struct iopf_group *group,
                         enum iommu_page_response_code status)
{
    ops->page_response(dev, &group->last_fault, &resp);
}
```

### 13.3 ATS (Address Translation Services)

ATS는 디바이스가 IOMMU에 주소 변환을 요청할 수 있게 해주는 PCIe 기능이다. 디바이스가 변환 결과를 캐시하면(IOTLB), IOMMU를 우회하여 직접 물리 주소로 DMA할 수 있다.

```
ATS가 DMA 추적에 미치는 영향:
  - ATS가 활성화되면 디바이스가 IOTLB에 캐시된 변환을 사용
  - IOMMU page table 변경만으로는 DMA를 가로챌 수 없음
  - ATS invalidation을 먼저 수행해야 함

  → DMA 추적 시 ATS invalidation을 포함해야 하므로
    성능 영향이 더 커짐
```

### 13.4 SVA (Shared Virtual Addressing)

SVA는 디바이스가 프로세스의 가상 주소 공간을 직접 사용할 수 있게 해주는 기능이다. PASID(Process Address Space ID)를 통해 여러 프로세스의 주소 공간을 동시에 사용할 수 있다.

```
SVA + PRI 조합:
  - 디바이스가 프로세스 가상 주소로 DMA
  - IOMMU가 프로세스 page table을 사용하여 주소 변환
  - page fault 발생 시 PRI를 통해 재시도 가능
  - ★ 이 조합이 "IOMMU 기반 DMA trace"에 가장 가까운 메커니즘
```

### 13.5 "IOMMU 기반 DMA trace" 이론적 구현

```
구현 가능성 분석:

┌──────────────────────────────────────────────────────────────┐
│  IOMMU DMA Trace (이론적 구현)                               │
│                                                              │
│  전제 조건:                                                  │
│  - IOMMU가 PRI를 지원                                       │
│  - 디바이스가 PRI를 지원                                     │
│  - PCIe ATS 비활성화 (또는 invalidation 처리)               │
│                                                              │
│  동작:                                                       │
│  1. 대상 IOVA 범위의 IOMMU PTE에서 R/W 권한 제거            │
│  2. 디바이스가 해당 범위에 DMA 시도                          │
│  3. IOMMU fault 발생                                         │
│  4. PRI: 디바이스가 대기 상태로 진입                         │
│  5. fault handler에서:                                       │
│     - 접근 주소, R/W, 디바이스 ID 기록                       │
│     - IOMMU PTE에 권한 복구                                  │
│     - Page Response 전송 → 디바이스 DMA 재시도               │
│  6. 성공적으로 DMA 완료                                      │
│  7. 다시 PTE 권한 제거 (다음 DMA를 위해)                     │
│                                                              │
│  결과: mmiotrace와 유사한 "트랩-기록-통과" 동작              │
│                                                              │
│  현실적 한계:                                                │
│  ✗ 대부분의 NVMe 디바이스는 PRI를 지원하지 않음              │
│  ✗ PRI를 지원해도 모든 종류의 DMA에 적용되지 않을 수 있음    │
│  ✗ 엄청난 성능 저하 (매 DMA마다 fault 발생)                  │
│  ✗ 타이밍 변화로 인해 race condition 관련 버그 추적이 어려움 │
│  ✗ 커널에 이 목적의 인프라가 아직 없음                       │
│                                                              │
│  현재 커널 상태:                                             │
│  - io-pgfault.c: IOPF 프레임워크 존재 (SVA 용도)           │
│  - iommufd: dirty tracking 지원 (Live Migration 용도)       │
│  - PRI 지원: Intel VT-d, AMD-Vi에서 부분적 지원             │
│  → "범용 DMA trace"로서의 인프라는 아직 없음                 │
└──────────────────────────────────────────────────────────────┘
```

### 13.6 현재 커널에서의 지원 상태 요약

```
┌─────────────────────────────────────────────────────────────────┐
│                   현재 커널 IOPF 인프라 상태                     │
│                                                                 │
│  io-pgfault.c                                                   │
│    ├── iopf_queue_alloc()          ✓ IOPF 큐 관리              │
│    ├── iopf_queue_add_device()     ✓ 디바이스 등록              │
│    ├── iommu_report_device_fault() ✓ fault 보고                 │
│    ├── iopf_group_response()       ✓ 응답 전송                  │
│    └── iopf_queue_flush_dev()      ✓ 큐 플러시                  │
│                                                                 │
│  용도: 현재는 주로 SVA (Shared Virtual Addressing)용            │
│  - 프로세스 주소 공간을 디바이스와 공유할 때                    │
│  - 디바이스 DMA가 page fault를 일으키면                         │
│  - 커널이 해당 페이지를 할당/매핑한 후 응답                     │
│                                                                 │
│  "범용 DMA trace"로 활용하려면:                                 │
│  - IOMMU 도메인에서 의도적으로 매핑을 제거하는 코드 필요        │
│  - fault handler에서 trace 정보를 수집하는 로직 필요            │
│  - 기존 IOPF 인프라를 확장하면 이론적으로 가능                  │
│  - 하지만 PRI 미지원 디바이스에서는 사용 불가                   │
│                                                                 │
│  iommufd dirty tracking                                         │
│    ├── iommufd_hwpt_set_dirty_tracking()  ✓ dirty tracking 설정 │
│    ├── iommufd_hwpt_get_dirty_bitmap()    ✓ dirty bitmap 읽기   │
│    └── Intel/AMD 하드웨어 구현             ✓ 동작 확인           │
│                                                                 │
│  용도: Live Migration에서 dirty page 추적                       │
│  - DMA를 방해하지 않고 "어떤 페이지가 수정되었는지" 확인        │
│  - 개별 DMA 트랜잭션 추적은 아니지만 write 패턴 파악에 유용     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 부록 A: DMA 관련 debugfs/sysfs 인터페이스 정리

```
/sys/kernel/debug/dma-api/
├── dump              - 현재 활성 DMA 매핑 전체 덤프
├── error_count       - 감지된 DMA API 오용 횟수
├── all_errors        - 모든 에러 표시 (0/1)
├── num_errors        - 표시할 에러 수
├── min_free_entries  - 최소 여유 엔트리 수
├── num_free_entries  - 현재 여유 엔트리 수
├── nr_total_entries  - 전체 엔트리 수
├── driver_filter     - 특정 드라이버만 필터링
└── filter            - 필터 활성화/비활성화

/sys/kernel/iommu_groups/
├── <group_id>/
│   ├── devices/      - 그룹에 속한 디바이스들
│   ├── type           - 도메인 타입 (DMA, identity, etc.)
│   └── reserved_regions - 예약된 IOVA 영역

/sys/kernel/debug/iommu/ (Intel VT-d)
├── intel/
│   ├── dmar0/
│   │   ├── domain_translation_struct  - IOMMU 페이지 테이블 덤프
│   │   ├── ecap                       - Extended Capability
│   │   └── cap                        - Capability
│   └── ir_translation_struct         - Interrupt Remapping 테이블
```

## 부록 B: 빠른 참조 - 용도별 명령어

```bash
# ═══════════════════════════════════════════════════════════
# 1. NVMe DMA 매핑 현황 빠른 확인
# ═══════════════════════════════════════════════════════════
cat /sys/kernel/debug/dma-api/dump 2>/dev/null | grep -c nvme
# 또는
bpftrace -e 'tracepoint:dma:dma_map_phys { @[str(args.device)] = count(); }' -c 'sleep 5'

# ═══════════════════════════════════════════════════════════
# 2. DMA 방향별 통계
# ═══════════════════════════════════════════════════════════
bpftrace -e '
tracepoint:dma:dma_map_phys /str(args.device) =~ "nvme"/ {
    @dir[args.dir] = count();
}
END { print(@dir); }
' -c 'sleep 10'

# ═══════════════════════════════════════════════════════════
# 3. NVMe I/O 레이턴시 히스토그램
# ═══════════════════════════════════════════════════════════
bpftrace -e '
tracepoint:block:block_rq_issue /args.dev >> 20 == 259/ {
    @start[args.sector] = nsecs;
}
tracepoint:block:block_rq_complete /args.dev >> 20 == 259 && @start[args.sector]/ {
    @us = hist((nsecs - @start[args.sector]) / 1000);
    delete(@start[args.sector]);
}
' -c 'fio --name=t --filename=/dev/nvme0n1 --rw=randread --bs=4k --runtime=5 --direct=1'

# ═══════════════════════════════════════════════════════════
# 4. IOMMU fault 실시간 모니터링
# ═══════════════════════════════════════════════════════════
dmesg -w | grep -E "DMAR|IOMMU|fault"

# ═══════════════════════════════════════════════════════════
# 5. QEMU NVMe trace 활성화
# ═══════════════════════════════════════════════════════════
qemu-system-x86_64 \
    -trace "pci_nvme_map_*" \
    -trace "pci_nvme_io_cmd" \
    -trace "pci_nvme_rw_cb" \
    -drive file=disk.qcow2,if=none,id=d0 \
    -device nvme,drive=d0,serial=1234 \
    -nographic
```
