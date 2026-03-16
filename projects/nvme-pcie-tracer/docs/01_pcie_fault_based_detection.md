# PCIe 레벨 Device→Host 접근 탐지: Page Fault 기반 설계

## 1. 문제 정의

기존 nvme-pcie-tracer는 kprobe로 커널 함수(`nvme_irq`, `dma_map_sg_attrs` 등)를 후킹해서 Device→Host 접근을 **간접적으로 추론**한다. 하지만 이것은 "커널 드라이버가 실행된 시점"을 잡는 것이지, "디바이스가 실제로 PCIe TLP를 보내서 호스트 메모리에 접근한 시점"을 잡는 게 아니다.

```
기존 방식 (kprobe):
  NVMe 디바이스 --DMA Write--> CQ 메모리 (호스트 RAM)
                                    |
                                    v
                              MSI-X 인터럽트
                                    |
                                    v
                              nvme_irq() ← kprobe가 여기를 잡음
                                    |
  문제: DMA Write 시점과 kprobe 시점 사이에 갭이 있음
        실제 PCIe 트랜잭션은 보이지 않음

원하는 방식 (Page Fault):
  NVMe 디바이스 --DMA Write--> IOMMU PTE에 접근 권한 없음
                                    |
                                    v
                              IOMMU Fault 발생 ← 실제 PCIe 접근 시점!
                                    |
                                    v
                              Fault Handler에서 기록
```

## 2. kmmio vs IOMMU: 같은 원리, 다른 제약

### kmmio (CPU → Device MMIO 추적) - 이미 작동하는 모델

```
┌─────────────────────────────────────────────────────────────────┐
│ kmmio의 PTE Fault 메커니즘 (arch/x86/mm/kmmio.c)              │
│                                                                 │
│  1. register_kmmio_probe(BAR0 영역)                            │
│     → BAR0에 매핑된 CPU PTE의 Present 비트 제거                │
│     → TLB flush                                                 │
│                                                                 │
│  2. CPU가 readl(bar + 0x1C) 실행 (CSTS 읽기)                  │
│     → PTE Present=0이므로 #PF(Page Fault) 발생                 │
│                                                                 │
│  3. kmmio_handler() 실행                                       │
│     → pre_handler(): 접근 전 정보 기록 (주소, 명령어)          │
│     → PTE Present 비트 복원 (disarm)                           │
│     → EFLAGS.TF=1 설정 (Single-Step 모드)                     │
│     → return → CPU가 readl() 재실행 (이번엔 성공)             │
│                                                                 │
│  4. Single-Step 완료 → #DB(Debug Trap) 발생                   │
│     → post_handler(): 접근 후 값 기록                          │
│     → PTE Present 비트 다시 제거 (re-arm)                      │
│     → EFLAGS.TF=0                                              │
│                                                                 │
│  핵심: CPU에는 Single-Step(TF flag)이 있어서                   │
│        "한 번 접근 허용 → 다시 잠금"이 가능하다                │
└─────────────────────────────────────────────────────────────────┘
```

### IOMMU (Device → Host DMA 추적) - 핵심 차이점

```
┌─────────────────────────────────────────────────────────────────┐
│ IOMMU Page Fault 메커니즘                                      │
│                                                                 │
│  1. IOMMU PTE에서 Read/Write 비트 제거                         │
│     → IOTLB flush                                               │
│                                                                 │
│  2. NVMe 디바이스가 DMA Read(SQ Fetch) 시도                    │
│     → IOMMU PTE에 접근 권한 없음                               │
│     → IOMMU Fault 발생!                                        │
│                                                                 │
│  3. ★ 여기서 문제 ★                                           │
│     CPU의 kmmio는:  fault → single-step → 재실행 → 성공       │
│     IOMMU는:        fault → DMA 트랜잭션 중단(abort)           │
│                                                                 │
│     IOMMU에는 "Single-Step" 개념이 없다!                       │
│     DMA가 abort되면 디바이스는 PCIe Completion에서              │
│     UR(Unsupported Request) 또는 CA(Completer Abort)를 받는다  │
│     → NVMe 컨트롤러가 에러 상태에 빠질 수 있음                │
└─────────────────────────────────────────────────────────────────┘
```

**결론**: IOMMU fault를 kmmio처럼 "투명하게 모든 접근을 잡는" 용도로 쓸 수는 없다. 하지만 다음과 같은 **실용적 접근법**이 가능하다.


## 3. 실현 가능한 접근법들

### 접근법 비교표

| 접근법 | 탐지 대상 | 투명성 | 정밀도 | 위험도 | 하드웨어 요구 |
|--------|-----------|--------|--------|--------|---------------|
| A. IOMMU Fault (의도적 유발) | SQ/CQ/Data DMA | 파괴적 | 접근 시점 정확 | 높음 | VT-d |
| B. IOMMU PRI (Page Request) | SQ/CQ/Data DMA | 투명 | 접근 시점 정확 | 없음 | VT-d + ATS/PRI |
| C. IOMMU Access/Dirty Bit | SQ/CQ/Data DMA | 투명 | 폴링 주기 의존 | 없음 | VT-d Scalable Mode |
| D. CQ 메모리 직접 폴링 | CQ DMA Write | 투명 | 폴링 주기 의존 | 없음 | 없음 |
| E. Unmap-Window 기법 | SQ/CQ/Data DMA | 반파괴적 | 윈도우 내 | 중간 | VT-d |


## 4. 접근법 A: IOMMU Fault 의도적 유발

### 원리

NVMe 디바이스의 DMA 매핑(IOVA)을 의도적으로 해제하여 IOMMU fault를 유발한다. Fault handler에서 "어떤 디바이스가 어떤 주소에 접근 시도했는지" 기록한다.

### 아키텍처

```
+---------------------------+
| NVMe Controller           |
| (PCIe Function)           |
+------+--------------------+
       | DMA Read/Write (PCIe TLP)
       v
+------+--------------------+
| IOMMU (Intel VT-d)        |
| +------------------------+ |
| | IOMMU Page Table       | |
| |  IOVA → HPA 매핑      | |
| |  ┌──────────────────┐  | |
| |  │ SQ IOVA: R=0 W=0 │  | |  ← 접근 권한 제거!
| |  │ CQ IOVA: R=0 W=0 │  | |
| |  │ Data:    R=0 W=0 │  | |
| |  └──────────────────┘  | |
| +------------------------+ |
|     ↓ Fault!               |
| +------------------------+ |
| | Fault Status Register  | |
| | - Source ID (BDF)      | |
| | - Fault Address (IOVA) | |
| | - Fault Reason         | |
| | - Read/Write           | |
| +------------------------+ |
+------+--------------------+
       | MSI → CPU
       v
+------+--------------------+
| dmar_fault() IRQ handler  |
| → 커스텀 fault handler    |
| → 이벤트 로깅             |
+---------------------------+
```

### Intel VT-d Fault 레코드 구조

```
Fault Record (16 bytes, DMAR_FSTS_REG 0x34):
+--------+------+------------------------------------------------+
| Offset | Bits | 필드                                           |
+--------+------+------------------------------------------------+
|  +0    | 63:12| Fault Address (IOVA, 4KB 정렬)                 |
|  +0    | 11:0 | Reserved                                       |
|  +8    | 31   | Fault (F) - valid bit                          |
|  +8    | 30   | Type (T) - 0=Write, 1=Read                    |
|  +8    | 29   | PASID Present                                  |
|  +8    | 27:20| Fault Reason                                   |
|  +8    | 19:0 | Source ID (Bus:Dev:Func)                       |
+--------+------+------------------------------------------------+

Fault Reason 코드:
  0x05: "PTE Write access is not set"   ← CQ Write, Data DMA Write
  0x06: "PTE Read access is not set"    ← SQ Fetch (DMA Read)
  0x01: "Present bit in root entry is clear"
  0x02: "Present bit in context entry is clear"
```

### 커널 모듈 구현

```c
// nvme_iommu_fault_tracer.c - IOMMU Fault 기반 DMA 접근 탐지

#include <linux/iommu.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>

/*
 * IOMMU Fault Handler 등록
 *
 * iommu_set_fault_handler()로 디바이스의 IOMMU 도메인에
 * 커스텀 fault handler를 등록한다.
 *
 * 커널 코드 (drivers/iommu/iommu.c):
 *   void iommu_set_fault_handler(struct iommu_domain *domain,
 *                                iommu_fault_handler_t handler,
 *                                void *token)
 *
 * Fault handler 시그니처:
 *   int handler(struct iommu_domain *domain, struct device *dev,
 *               unsigned long iova, int flags, void *token)
 *
 * 반환값:
 *   -ENOSYS: 핸들러가 처리하지 않음 (기본 처리로 넘김)
 *   0: 핸들러가 처리 완료
 */

struct iommu_fault_ctx {
    struct pci_dev *pdev;
    struct iommu_domain *domain;

    /* 모니터링 대상 IOVA 영역 */
    struct {
        dma_addr_t sq_iova;    /* SQ DMA 주소 */
        dma_addr_t cq_iova;    /* CQ DMA 주소 */
        size_t sq_size;
        size_t cq_size;
    } targets;

    /* 통계 */
    atomic64_t fault_count;
    atomic64_t sq_fetch_count;     /* SQ Read fault (디바이스가 SQ 읽음) */
    atomic64_t cq_write_count;     /* CQ Write fault (디바이스가 CQ 씀) */
    atomic64_t data_dma_count;     /* Data DMA fault */
};

/*
 * IOMMU Fault Handler
 *
 * IOMMU가 DMA fault를 감지하면 이 함수가 호출된다.
 * dmar_fault() IRQ handler → report_iommu_fault() → 이 핸들러
 *
 * 호출 체인 (drivers/iommu/intel/dmar.c):
 *   dmar_fault()                          ← IOMMU MSI IRQ
 *     → dmar_fault_do_one()
 *       → intel_iommu_dev_fault_handler() ← Intel 드라이버
 *         → report_iommu_fault()          ← 범용 API
 *           → domain->handler()           ← 우리의 핸들러!
 */
static int nvme_iommu_fault_handler(struct iommu_domain *domain,
                                     struct device *dev,
                                     unsigned long iova,
                                     int flags, void *token)
{
    struct iommu_fault_ctx *ctx = token;
    u64 ts = ktime_get_ns();
    int is_write = !(flags & IOMMU_FAULT_READ);

    atomic64_inc(&ctx->fault_count);

    /*
     * IOVA 범위로 접근 유형 분류:
     *
     * SQ Fetch (DMA Read):
     *   디바이스가 SQ 메모리를 읽으러 옴 → Fault Reason 0x06
     *   flags에 IOMMU_FAULT_READ가 설정됨
     *
     * CQ Write (DMA Write):
     *   디바이스가 CQ에 완료 엔트리를 쓰러 옴 → Fault Reason 0x05
     *   flags에 IOMMU_FAULT_WRITE가 설정됨
     */
    if (iova >= ctx->targets.sq_iova &&
        iova < ctx->targets.sq_iova + ctx->targets.sq_size) {
        atomic64_inc(&ctx->sq_fetch_count);
        pr_info("nvme-fault: [%llu] SQ FETCH detected! "
                "IOVA=0x%lx dev=%s read=%d\n",
                ts, iova, dev_name(dev), !is_write);
    } else if (iova >= ctx->targets.cq_iova &&
               iova < ctx->targets.cq_iova + ctx->targets.cq_size) {
        atomic64_inc(&ctx->cq_write_count);
        pr_info("nvme-fault: [%llu] CQ WRITE detected! "
                "IOVA=0x%lx dev=%s write=%d\n",
                ts, iova, dev_name(dev), is_write);
    } else {
        atomic64_inc(&ctx->data_dma_count);
        pr_info("nvme-fault: [%llu] DATA DMA detected! "
                "IOVA=0x%lx dev=%s %s\n",
                ts, iova, dev_name(dev),
                is_write ? "WRITE" : "READ");
    }

    return -ENOSYS; /* 기본 fault 처리 계속 */
}
```

### 치명적 한계

```
★ 경고: 이 방식은 NVMe 디바이스를 죽인다! ★

DMA가 abort되면 NVMe 컨트롤러는:
  1. PCIe Completion에서 UR(Unsupported Request) 수신
  2. 컨트롤러 내부 에러 → CSTS.CFS=1 (Controller Fatal Status)
  3. 호스트가 컨트롤러 리셋 필요

→ 이 방식은 "탐지 1회 후 디바이스 리셋" 패턴으로만 사용 가능
→ 교육/실험 목적으로는 유효 (실제 PCIe fault가 발생하는 것을 확인)
```


## 5. 접근법 B: IOMMU PRI (Page Request Interface)

### 원리

PRI는 IOMMU의 "Single-Step" 등가물이다. 디바이스가 IOMMU fault를 만나면 DMA를 abort하지 않고, **Page Request 메시지**를 보내서 OS에게 "이 페이지 매핑해줘"라고 요청한다. OS가 매핑을 해주고 **Page Response**를 보내면 디바이스가 DMA를 **재시도**한다.

```
kmmio의 Single-Step과 PRI 비교:

kmmio (CPU):
  Page Fault → pre_handler → PTE 복원 → TF=1 → 1명령 실행 → #DB → re-arm
                                         ^^^^^^^^^^^^^^^^^
                                         "한 번만 허용"

PRI (Device):
  IOMMU Fault → Page Request → OS: 매핑 복원 → Page Response → DMA 재시도
                                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                "한 번만 허용" (그 뒤에 다시 제거 가능)
```

### PRI 동작 흐름

```
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│  1. IOMMU PTE에서 Present/R/W 제거                                 │
│     → IOTLB flush                                                   │
│                                                                     │
│  2. NVMe 디바이스 DMA 시도                                         │
│     → IOMMU translation fail                                        │
│     → 디바이스가 ATS/PRI 지원하면:                                 │
│                                                                     │
│  3. 디바이스 → IOMMU: Page Request (PCIe 메시지)                   │
│     ┌─────────────────────────────────────────┐                     │
│     │ Page Request Descriptor (PRQ Entry)     │                     │
│     │  - Request ID (Bus:Dev:Func)            │                     │
│     │  - PASID (있으면)                        │                     │
│     │  - Faulting Address                      │                     │
│     │  - Read/Write/Execute 요청              │                     │
│     │  - Last Page In Group (LPIG)            │                     │
│     │  - Page Request Group Index             │                     │
│     └─────────────────────────────────────────┘                     │
│                                                                     │
│  4. IOMMU → CPU: PRQ 인터럽트 (MSI)                               │
│     커널: prq_event_thread() (drivers/iommu/intel/prq.c)          │
│       → intel_prq_report()                                         │
│         → iommu_report_device_fault()                              │
│           → domain->iopf_handler()  ← 우리의 핸들러!              │
│                                                                     │
│  5. 핸들러:                                                        │
│     a) 접근 정보 로깅 (IOVA, R/W, timestamp)                      │
│     b) IOMMU PTE 복원 (매핑 재설정)                               │
│     c) IOTLB flush                                                  │
│     d) iopf_group_response(IOMMU_PAGE_RESP_SUCCESS)                │
│                                                                     │
│  6. IOMMU → 디바이스: Page Response                                │
│     → 디바이스 DMA 재시도 → 성공!                                  │
│                                                                     │
│  7. (선택) DMA 완료 후 다시 PTE 제거 → 다음 접근도 잡을 수 있음   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### PRI 요구사항

```
하드웨어 요구사항:
  ① IOMMU: Intel VT-d with PRQ (Page Request Queue) 지원
     → ECAP 레지스터의 PRS(Page Request Support) 비트 확인
     → 최신 Intel CPU (Sapphire Rapids, Alder Lake 이후) 대부분 지원

  ② NVMe 디바이스: ATS(Address Translation Service) + PRI 지원
     → PCIe Extended Capability에 ATS/PRI Capability 필요
     → lspci -vvv | grep -E "ATS|PRI|PASID"
     → ★ 대부분의 NVMe SSD는 ATS/PRI를 지원하지 않음 ★

  ③ 커널 설정:
     → CONFIG_INTEL_IOMMU_SVM=y (Shared Virtual Memory)
     → CONFIG_PCI_PRI=y
     → CONFIG_PCI_ATS=y

확인 방법:
  # IOMMU 쪽 지원 확인
  dmesg | grep -i "IOMMU.*PRS\|Page Request"

  # 디바이스 쪽 확인
  lspci -s <BDF> -vvv | grep -E "PageReq|PRI"

  # PRI capability가 없으면 이 방식은 불가
```

### 커널 코드 분석: PRQ 핸들러

```c
/*
 * drivers/iommu/intel/prq.c - prq_event_thread()
 *
 * IOMMU가 Page Request를 받으면 PRQ(Page Request Queue)에 넣고
 * MSI 인터럽트로 CPU에 알린다. prq_event_thread()가 PRQ를 읽어
 * iommu_report_device_fault()로 보고한다.
 *
 * PRQ Entry 구조 (struct page_req_dsc):
 *   - rid: Request ID (BDF)
 *   - addr: 요청된 주소 (>> 12)
 *   - rd_req/wr_req: 읽기/쓰기 요청
 *   - lpig: Last Page In Group
 *   - prg_index: Page Request Group 인덱스
 */

/* 우리가 등록할 IOPF 핸들러 */
static int nvme_iopf_handler(struct iopf_group *group)
{
    struct iommu_fault *fault = &group->last_fault.fault;
    u64 ts = ktime_get_ns();

    pr_info("nvme-pri: [%llu] Page Request! "
            "addr=0x%llx %s%s pasid=%d\n",
            ts,
            fault->prm.addr,
            fault->prm.flags & IOMMU_FAULT_PERM_READ ? "READ " : "",
            fault->prm.flags & IOMMU_FAULT_PERM_WRITE ? "WRITE " : "",
            fault->prm.pasid);

    /*
     * 여기서:
     * 1. 접근 정보 로깅
     * 2. IOMMU 매핑 복원 (필요시)
     * 3. Page Response 전송 → 디바이스 DMA 재시도
     */
    iopf_group_response(group, IOMMU_PAGE_RESP_SUCCESS);
    iopf_free_group(group);
    return 0;
}
```

### PRI의 현실적 한계

```
★ 대부분의 NVMe SSD는 ATS/PRI를 지원하지 않음 ★

NVMe SSD는 전통적으로 IOMMU bypass (identity mapping) 또는
단순 DMA 매핑을 사용한다. ATS/PRI는 주로:
  - GPU (NVIDIA, AMD)
  - SmartNIC (Mellanox, Intel E810)
  - 일부 최신 CXL 디바이스
에서 지원된다.

확인: lspci -s <BDF> -vvv 에서 "PageReq" capability가 없으면 불가.
```


## 6. 접근법 C: IOMMU Access/Dirty Bit 추적

### 원리

Intel VT-d의 First-Level Page Table (Scalable Mode)에는 CPU 페이지 테이블과 동일하게 **Access bit(A, bit 5)**과 **Dirty bit(D, bit 6)**이 있다. 하드웨어가 DMA 접근 시 이 비트들을 자동으로 설정한다.

```
First-Level PTE (Scalable Mode):
+------+------+------+------+------+------+------+------+
| 63:12       | 11:7 |  6   |  5   |  4   |  3:1 |  0   |
| HPA         | Rsvd |Dirty |Access| Rsvd | R/W  |Prsnt |
+------+------+------+------+------+------+------+------+

DMA_FL_PTE_PRESENT = BIT(0)   ← 매핑 존재
DMA_FL_PTE_ACCESS  = BIT(5)   ← 하드웨어가 접근 시 설정
DMA_FL_PTE_DIRTY   = BIT(6)   ← 하드웨어가 쓰기 시 설정
```

### 동작 흐름

```
┌─────────────────────────────────────────────────────────────────────┐
│ Access/Dirty Bit 기반 DMA 접근 추적                                │
│                                                                     │
│  1. 초기 상태: IOMMU PTE에 Present=1, R=1, W=1, A=0, D=0         │
│     → 정상 DMA 가능하지만 Access/Dirty는 클리어                    │
│                                                                     │
│  2. NVMe 디바이스가 DMA Read (SQ Fetch)                            │
│     → IOMMU 하드웨어가 자동으로 Access bit 설정                   │
│     → PTE: Present=1, R=1, W=1, A=1, D=0                          │
│                                                                     │
│  3. NVMe 디바이스가 DMA Write (CQ Write)                           │
│     → IOMMU 하드웨어가 Access + Dirty bit 설정                    │
│     → PTE: Present=1, R=1, W=1, A=1, D=1                          │
│                                                                     │
│  4. 커널 스레드가 주기적으로 PTE를 읽음                            │
│     → A=1이면: "이 페이지가 디바이스에 의해 읽혔다"               │
│     → D=1이면: "이 페이지가 디바이스에 의해 쓰여졌다"             │
│     → A/D 비트 클리어 → IOTLB flush → 다음 접근 감지 준비        │
│                                                                     │
│  장점: DMA가 중단되지 않음! 완전히 투명!                          │
│  단점: 폴링 주기만큼의 시간 해상도 (μs~ms)                       │
│        First-Level Paging + Scalable Mode 필요                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 요구사항

```
하드웨어 요구사항:
  ① Intel VT-d ECAP 레지스터:
     → SSADS (Scalable-mode Second-stage Access/Dirty Support) 또는
     → SLADS (Second-level Access/Dirty Support)
     → dmesg | grep -i "access\|dirty\|scalable"

  ② IOMMU가 Scalable Mode로 동작 중:
     → 커널 부트 파라미터: intel_iommu=on,sm_on
     → dmesg | grep -i "Scalable Mode"

  ③ 커널 설정:
     → CONFIG_INTEL_IOMMU=y (기본)
```

### 구현 골격

```c
// nvme_iommu_ad_tracer.c - Access/Dirty bit 기반 DMA 접근 추적

#include <linux/iommu.h>

/*
 * IOMMU 내부 PTE에 직접 접근하는 것은 드라이버 내부 API이므로,
 * 실제 구현에서는 다음 중 하나를 사용한다:
 *
 * 방법 1: Intel VT-d 내부 함수 직접 호출 (커널 수정 필요)
 *   → pfn_to_dma_pte() 등으로 PTE 포인터 획득
 *   → A/D 비트 읽기/클리어
 *
 * 방법 2: iommu_dirty_bitmap API (커널 6.6+)
 *   → VFIO dirty page tracking 용으로 추가된 API
 *   → struct iommu_dirty_ops의 read_and_clear_dirty()
 *
 * 방법 3: /dev/iommu 또는 VFIO를 통한 유저스페이스 접근
 *   → VFIO_IOMMU_DIRTY_PAGES ioctl
 */

/*
 * 방법 2의 커널 API (drivers/iommu/iommu.c):
 *
 * int iommu_read_and_clear_dirty(struct iommu_domain *domain,
 *                                 unsigned long iova, size_t size,
 *                                 unsigned long flags,
 *                                 struct iommu_dirty_bitmap *dirty)
 *
 * Intel VT-d 구현 (drivers/iommu/intel/iommu.c):
 *   intel_iommu_read_and_clear_dirty()
 *     → 1st level PTE의 Dirty bit 읽고 클리어
 *     → IOTLB flush
 *
 * 이 API는 VFIO live migration 용이지만,
 * 우리 목적에도 그대로 사용 가능하다.
 */

struct ad_monitor_ctx {
    struct iommu_domain *domain;
    struct task_struct *poll_thread;

    /* 모니터링 대상 */
    dma_addr_t sq_iova;
    dma_addr_t cq_iova;
    size_t sq_size;
    size_t cq_size;

    /* 설정 */
    unsigned int poll_interval_us;  /* 폴링 주기 (기본 100μs) */
    bool running;
};

/*
 * 폴링 스레드: Access/Dirty 비트를 주기적으로 확인
 */
static int ad_poll_thread(void *data)
{
    struct ad_monitor_ctx *ctx = data;
    struct iommu_dirty_bitmap dirty_bitmap;
    /* bitmap 초기화 생략 */

    while (!kthread_should_stop() && ctx->running) {
        u64 ts = ktime_get_ns();

        /*
         * SQ 영역의 Dirty/Access 비트 확인
         * Access bit이 설정되어 있으면 → 디바이스가 SQ를 읽었음 (SQ Fetch)
         */
        iommu_read_and_clear_dirty(ctx->domain,
                                    ctx->sq_iova, ctx->sq_size,
                                    0, &dirty_bitmap);
        /* dirty_bitmap에서 어떤 페이지가 accessed/dirty인지 확인 */

        /*
         * CQ 영역의 Dirty 비트 확인
         * Dirty bit이 설정되어 있으면 → 디바이스가 CQ에 썼음 (CQ Write)
         */
        iommu_read_and_clear_dirty(ctx->domain,
                                    ctx->cq_iova, ctx->cq_size,
                                    0, &dirty_bitmap);

        usleep_range(ctx->poll_interval_us,
                     ctx->poll_interval_us + 10);
    }

    return 0;
}
```

### 현실적 한계

```
★ iommu_read_and_clear_dirty()는 VFIO 도메인 전용 ★

현재 커널에서 이 API는 IOMMU_DOMAIN_UNMANAGED 또는
IOMMU_HWPT_DIRTY_TRACKING이 활성화된 도메인에서만 동작한다.
일반 DMA 도메인(NVMe가 사용하는)에서는 dirty tracking이
기본 비활성화이다.

활성화 방법:
  1. VFIO passthrough로 NVMe를 사용 (vfio-pci 드라이버 바인딩)
  2. 또는 커널 패치로 DMA 도메인에서도 dirty tracking 활성화
  3. 또는 Intel VT-d 내부 함수를 직접 호출 (해킹)
```


## 7. 접근법 D: CQ 메모리 직접 폴링 (가장 실용적)

### 원리

NVMe CQ는 호스트 메모리에 있고, 디바이스가 DMA Write로 CQE를 기록한다. CQE의 **Phase bit**이 뒤집히면 → 디바이스가 이 메모리에 DMA Write를 했다는 확실한 증거이다.

```
CQ Memory Layout:
┌──────────────────────────────────────────────────────────┐
│ CQE[0]: status=0x0001 (phase=1) ← 완료됨               │
│ CQE[1]: status=0x0001 (phase=1) ← 완료됨               │
│ CQE[2]: status=0x0000 (phase=0) ← 아직 비어있음        │
│ CQE[3]: status=0x0000 (phase=0)                         │
│ ...                                                      │
└──────────────────────────────────────────────────────────┘
         ↑
  폴링 스레드가 CQE[2]의 phase를 계속 확인
  phase가 0→1로 바뀌면 = 디바이스가 DMA Write로 CQE를 기록한 것!

이것이 진짜 PCIe 레벨 탐지:
  - kprobe는 nvme_irq() 시점을 잡지만, CQ 폴링은 DMA Write 자체를 잡음
  - 인터럽트보다 먼저 CQE 도착을 감지할 수 있음 (인터럽트 지연 전)
```

### 구현

```c
// nvme_cq_poll_tracer.c - CQ 메모리 직접 폴링으로 Device DMA Write 탐지

/*
 * 핵심 아이디어:
 * NVMe CQ 메모리는 DMA-coherent로 할당되어 CPU에서 직접 읽을 수 있다.
 * dma_alloc_coherent()로 할당된 메모리이므로 CPU와 디바이스가
 * 동일한 물리 메모리를 본다 (캐시 일관성 보장).
 *
 * CQE의 Phase bit 변화 = 디바이스가 PCIe TLP(Memory Write)를 보냄
 *
 * 이것은 kprobe가 아니라 실제 메모리 내용 변화를 관찰하는 것이므로
 * PCIe 레벨 접근의 직접적 증거이다.
 */

struct cq_poll_ctx {
    /* NVMe 큐 정보 (모듈 파라미터 또는 debugfs에서 설정) */
    volatile struct nvme_completion *cqes; /* CQ 메모리 가상 주소 */
    u32 q_depth;
    u16 expected_phase;   /* 다음에 올 CQE의 예상 phase */
    u16 poll_head;        /* 폴링 위치 */
    u16 qid;

    /* 타이밍 */
    u64 last_empty_ts;    /* 마지막으로 CQE가 없었던 시각 */
    u64 first_seen_ts;    /* 새 CQE를 처음 발견한 시각 */

    /* 폴링 설정 */
    unsigned int poll_interval_ns; /* 폴링 주기 (기본 1μs) */
    struct task_struct *poll_thread;
    bool running;
};

/*
 * phase_matches - CQE가 유효한지 Phase bit으로 확인
 *
 * NVMe CQE status 필드의 bit 0이 Phase bit이다.
 * 컨트롤러가 CQ를 한 바퀴 돌 때마다 Phase가 토글된다.
 * expected_phase와 일치하면 → 새로운 CQE가 도착한 것.
 *
 * READ_ONCE(): 컴파일러 최적화 방지, 디바이스 DMA를 반드시 읽음
 * le16_to_cpu(): NVMe는 리틀 엔디안
 */
static inline bool phase_matches(volatile struct nvme_completion *cqe,
                                  u16 expected_phase)
{
    return (le16_to_cpu(READ_ONCE(cqe->status)) & 1) == expected_phase;
}

/*
 * CQ 폴링 스레드
 *
 * 이 스레드는 kthread_run()으로 생성하고, CPU affinity를 설정하여
 * NVMe 큐의 인터럽트 CPU와 다른 CPU에서 실행한다.
 *
 * 폴링 주기에 따라 CPU 사용률이 달라진다:
 *   1μs:   ~100% CPU (busy poll) - 최고 정밀도
 *   10μs:  ~50% CPU             - 좋은 타협점
 *   100μs: ~5% CPU              - 대부분의 I/O 캡처 가능
 *   1ms:   ~0.5% CPU            - 높은 레이턴시 I/O만
 */
static int cq_poll_thread(void *data)
{
    struct cq_poll_ctx *ctx = data;

    pr_info("nvme-cq-poll: 폴링 시작 qid=%u depth=%u interval=%uns\n",
            ctx->qid, ctx->q_depth, ctx->poll_interval_ns);

    while (!kthread_should_stop() && ctx->running) {
        volatile struct nvme_completion *cqe;

        cqe = &ctx->cqes[ctx->poll_head];

        if (phase_matches(cqe, ctx->expected_phase)) {
            /*
             * ★ 새 CQE 발견! ★
             * 이 시점이 "디바이스가 호스트 메모리에 DMA Write를 완료한"
             * 직후이다. kprobe보다 정확한 타이밍.
             */
            u64 detect_ts = ktime_get_ns();
            u16 status = le16_to_cpu(READ_ONCE(cqe->status));
            u16 sq_head = le16_to_cpu(READ_ONCE(cqe->sq_head));
            u16 sq_id = le16_to_cpu(READ_ONCE(cqe->sq_id));
            u16 cid = READ_ONCE(cqe->command_id);

            pr_info("nvme-cq-poll: [%llu] CQE DETECTED! "
                    "qid=%u idx=%u cid=%u sqhd=%u sqid=%u "
                    "status=0x%04x (sc=0x%02x)\n",
                    detect_ts, ctx->qid, ctx->poll_head,
                    cid, sq_head, sq_id,
                    status >> 1, (status >> 1) & 0xFF);

            /* SQ Head 변화 = SQ Fetch도 발생했다는 간접 증거 */
            pr_info("nvme-cq-poll: [%llu]   → SQ Fetch 확인: "
                    "sqhd=%u (디바이스가 SQ[%u]까지 읽음)\n",
                    detect_ts, sq_head, sq_id);

            /* 다음 CQE로 이동 */
            if (++ctx->poll_head == ctx->q_depth) {
                ctx->poll_head = 0;
                ctx->expected_phase ^= 1;
            }
        } else {
            /* CQE 미도착 → 짧은 대기 */
            if (ctx->poll_interval_ns < 1000)
                cpu_relax(); /* busy wait (~10ns) */
            else
                ndelay(ctx->poll_interval_ns);
        }
    }

    return 0;
}
```

### CQ 메모리 주소 획득 방법

```
문제: CQ 메모리(cqes 포인터)는 nvme_queue 구조체 내부에 있고,
      이 구조체는 NVMe 드라이버 private이다.

해결법 3가지:

방법 1: kprobe로 nvme_queue 정보 추출 (하이브리드)
  nvme_irq()에 kprobe를 걸어 nvme_queue 포인터를 얻고,
  cqes, q_depth, cq_head, cq_phase 등을 복사한다.
  → "최초 1회만 kprobe 사용, 이후는 순수 폴링"

방법 2: BTF(BPF Type Format)로 구조체 오프셋 계산
  /sys/kernel/btf/vmlinux에서 nvme_queue의 필드 오프셋을 읽어
  바이트 단위로 접근한다.
  → 커널 버전 무관, 안전

방법 3: DMA 주소에서 역추적
  /sys/kernel/debug/dma-buf/ 또는 iomem 정보에서
  NVMe CQ의 물리 주소를 찾아 ioremap()한다.
  → 위험하지만 완전히 드라이버 독립적
```


## 8. 접근법 E: IOMMU Unmap-Window 기법

### 원리

짧은 시간 동안만 IOMMU 매핑을 해제하여, 그 "창(window)" 안에서 디바이스 접근이 있었는지 확인한다. 접근이 있었으면 IOMMU fault가 기록되고, 즉시 매핑을 복원한다.

```
시간축:
  t0          t1          t2          t3
  │           │           │           │
  ├───────────┼───────────┼───────────┤
  │ 매핑 정상 │ UNMAP     │ 매핑 복원 │ 결과 확인
  │           │ (10-100μs)│           │
  │           │ ↑ fault?  │           │
  │           └───────────┘           │

  t1에서 unmap → t2에서 remap
  사이에 디바이스가 DMA 시도하면 → fault 기록됨
  사이에 디바이스가 DMA 안 하면 → fault 없음

  이것을 반복하면 통계적으로 "디바이스가 언제 접근하는지" 프로파일 가능
```

### 구현

```c
// nvme_iommu_window_tracer.c - Unmap-Window 기법

struct window_ctx {
    struct iommu_domain *domain;
    dma_addr_t target_iova;
    phys_addr_t target_phys;
    size_t target_size;
    int target_prot;  /* IOMMU_READ | IOMMU_WRITE */

    /* 결과 */
    atomic_t fault_detected;
    u64 fault_iova;
    int fault_flags;
};

/*
 * 한 번의 "probe window" 실행
 *
 * 1. IOMMU 매핑 해제
 * 2. 짧은 대기 (디바이스 접근 기회)
 * 3. IOMMU 매핑 복원
 * 4. Fault 발생 여부 확인
 *
 * 주의: 이 윈도우 동안 디바이스 DMA가 실패하므로
 *       I/O 에러가 발생할 수 있다.
 *       SQ/CQ의 경우 NVMe 컨트롤러가 에러 상태에 빠질 수 있다.
 *
 * 안전한 대상: Data DMA 버퍼 (실패해도 해당 I/O만 재시도)
 * 위험한 대상: SQ/CQ (컨트롤러 에러 가능)
 */
static int probe_window(struct window_ctx *ctx,
                         unsigned int window_us)
{
    size_t unmapped;
    int ret;

    atomic_set(&ctx->fault_detected, 0);

    /* Step 1: IOMMU 매핑 해제 → 디바이스 DMA가 fail하게 됨 */
    unmapped = iommu_unmap(ctx->domain,
                           ctx->target_iova,
                           ctx->target_size);
    if (unmapped != ctx->target_size) {
        pr_warn("nvme-window: partial unmap %zu/%zu\n",
                unmapped, ctx->target_size);
    }

    /* Step 2: 윈도우 대기 */
    usleep_range(window_us, window_us + 1);

    /* Step 3: 매핑 복원 */
    ret = iommu_map(ctx->domain,
                    ctx->target_iova,
                    ctx->target_phys,
                    ctx->target_size,
                    ctx->target_prot,
                    GFP_KERNEL);
    if (ret) {
        pr_err("nvme-window: CRITICAL - failed to restore "
               "IOMMU mapping! ret=%d\n", ret);
        return ret;
    }

    /* Step 4: 결과 확인 */
    if (atomic_read(&ctx->fault_detected)) {
        pr_info("nvme-window: DMA ACCESS DETECTED during "
                "window! iova=0x%llx flags=0x%x\n",
                ctx->fault_iova, ctx->fault_flags);
        return 1;  /* 접근 감지됨 */
    }

    return 0;  /* 윈도우 동안 접근 없음 */
}
```


## 9. 권장 구현 순서

### Phase 1: CQ 폴링 (안전, 즉시 구현 가능)

```
목표: 디바이스의 CQ DMA Write를 인터럽트보다 먼저 탐지

구현:
  1. kprobe로 nvme_queue 정보 1회 추출
  2. CQ 메모리 폴링 스레드 구동
  3. Phase bit 변화로 DMA Write 탐지
  4. 타임스탬프 + CQE 내용 기록

장점: 안전, NVMe 동작에 영향 없음, 높은 정밀도
파일: kernel/nvme_cq_poll_tracer.c
```

### Phase 2: IOMMU Fault Handler 등록 (교육 목적)

```
목표: IOMMU fault 메커니즘 이해, fault 정보 로깅

구현:
  1. iommu_get_domain_for_dev()로 도메인 획득
  2. iommu_set_fault_handler()로 커스텀 핸들러 등록
  3. (선택) 의도적으로 매핑 해제하여 fault 유발
  4. Fault 정보 (IOVA, Source ID, Read/Write) 기록

장점: 실제 IOMMU fault 동작 확인
위험: 의도적 unmap 시 디바이스 에러 가능
파일: kernel/nvme_iommu_fault_tracer.c
```

### Phase 3: IOMMU Access/Dirty Bit (고급)

```
목표: 투명하게 디바이스 DMA 접근 추적

구현:
  1. Scalable Mode VT-d 확인
  2. Intel VT-d 내부 PTE에 접근하여 A/D 비트 읽기
  3. 주기적 폴링으로 접근 페이지 식별
  4. IOTLB flush 후 A/D 비트 리셋

장점: 완전히 투명, 디바이스에 영향 없음
제약: Intel Scalable Mode VT-d 필요, 커널 수정 가능성
파일: kernel/nvme_iommu_ad_tracer.c
```

### Phase 4: PRI (하드웨어 의존)

```
목표: IOMMU의 "Single-Step" 등가물로 투명 추적

구현: PRI 지원 확인 후 IOPF 핸들러 등록
제약: NVMe SSD의 ATS/PRI 지원이 필수 (대부분 미지원)
```


## 10. 접근법별 코드 경로 요약

```
접근법 A (IOMMU Fault):
  iommu_unmap()
    → intel_iommu_unmap()                [drivers/iommu/intel/iommu.c]
      → __domain_mapping()               IOMMU PTE 삭제
      → iommu_iotlb_sync()              IOTLB flush
  디바이스 DMA 시도 → IOMMU fault
    → dmar_fault() IRQ                   [drivers/iommu/intel/dmar.c]
      → dmar_fault_do_one()
        → report_iommu_fault()           [drivers/iommu/iommu.c]
          → domain->handler()            우리의 핸들러

접근법 B (PRI):
  IOMMU PTE에서 Present 제거
  디바이스 DMA 시도 → ATS miss → Page Request
    → prq_event_thread()                 [drivers/iommu/intel/prq.c]
      → intel_prq_report()
        → iommu_report_device_fault()    [drivers/iommu/io-pgfault.c]
          → domain->iopf_handler()       우리의 핸들러
  iopf_group_response(SUCCESS)           → 디바이스 DMA 재시도

접근법 C (Access/Dirty):
  intel_iommu_read_and_clear_dirty()     [drivers/iommu/intel/iommu.c]
    → 1st-level PTE의 A/D 비트 읽기
    → qi_flush_dev_iotlb()               IOTLB flush

접근법 D (CQ 폴링):
  NVMe CQ 메모리: dma_alloc_coherent()로 할당됨
    → CPU에서 직접 읽기 가능 (캐시 일관성)
    → Phase bit 변화 감시
    → 별도 커널 인프라 불필요
```


## 11. kmmio 원리를 IOMMU에 적용: 개념 비교

```
┌───────────────┬─────────────────────────┬──────────────────────────┐
│               │ kmmio (CPU→Device)      │ IOMMU (Device→Host)     │
├───────────────┼─────────────────────────┼──────────────────────────┤
│ 페이지 테이블 │ CPU PTE                 │ IOMMU PTE               │
│ Present 비트  │ PTE bit 0               │ DMA_PTE_READ/WRITE      │
│               │                         │ (2nd-level) 또는        │
│               │                         │ DMA_FL_PTE_PRESENT      │
│               │                         │ (1st-level)             │
│ Fault 발생    │ CPU #PF                 │ IOMMU DMAR Fault        │
│ Fault 핸들러  │ kmmio_handler()         │ dmar_fault()            │
│ Single-Step   │ EFLAGS.TF=1 → #DB      │ ★ 없음 ★              │
│               │                         │ (PRI가 있으면 가능)     │
│ Re-arm        │ PTE Present 다시 제거   │ IOMMU PTE 다시 제거     │
│ TLB Flush     │ flush_tlb_one_kernel()  │ qi_flush_iotlb()        │
│ Access Bit    │ PTE bit 5 (Accessed)    │ FL_PTE bit 5 (Accessed) │
│ Dirty Bit     │ PTE bit 6 (Dirty)       │ FL_PTE bit 6 (Dirty)    │
│ 컨텍스트 저장 │ per-CPU kmmio_ctx       │ fault_record per-IOMMU  │
│ RCU 보호      │ rcu_read_lock_sched()   │ 불필요 (HW 큐)         │
├───────────────┼─────────────────────────┼──────────────────────────┤
│ 핵심 차이     │ CPU는 Single-Step으로   │ IOMMU는 DMA를 abort    │
│               │ 1명령만 실행 후 재차단  │ 시키고 끝. 재시도 없음  │
│               │ → 투명한 추적 가능     │ → PRI 없으면 파괴적    │
└───────────────┴─────────────────────────┴──────────────────────────┘
```
