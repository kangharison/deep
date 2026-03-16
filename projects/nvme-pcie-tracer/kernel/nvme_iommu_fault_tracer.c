// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_iommu_fault_tracer.c - IOMMU Page Fault로 PCIe DMA 접근 직접 탐지
 *
 * ============================================================================
 * 목적
 * ============================================================================
 *
 * NVMe 디바이스가 호스트 메모리(SQ, CQ, Data)에 DMA 접근하는 순간을
 * IOMMU page fault로 직접 탐지한다.
 *
 * CQ 폴링은 "호스트가 메모리를 읽어서 값이 바뀌었는지 확인"하는 것이고,
 * IOMMU fault는 "디바이스가 PCIe TLP를 보내는 순간 IOMMU가 차단"하는 것이다.
 * 후자만이 진짜 PCIe 레벨 탐지이다.
 *
 * ============================================================================
 * 동작 흐름
 * ============================================================================
 *
 *   ┌─────────────────────────────────────────────────────────────────┐
 *   │ 1. 모듈 로드                                                   │
 *   │    → dmar_fault()에 kprobe 등록 (fault 감지 준비)             │
 *   │    → nvme_irq()에 kprobe 등록 (CQ IOVA 추출용)               │
 *   │                                                                 │
 *   │ 2. I/O 발생 → nvme_irq() kprobe가 CQ IOVA/크기 추출         │
 *   │                                                                 │
 *   │ 3. debugfs에서 "arm" 명령                                     │
 *   │    → CQ IOVA의 IOMMU 매핑에서 Write 권한 제거                │
 *   │    → IOTLB flush                                               │
 *   │                                                                 │
 *   │ 4. NVMe 디바이스가 CQ에 DMA Write 시도                       │
 *   │    → IOMMU가 "PTE Write access is not set" fault 발생        │
 *   │    → PCIe Completion에 UR 반환 → DMA 실패                    │
 *   │                                                                 │
 *   │ 5. dmar_fault() IRQ handler 호출                              │
 *   │    → 우리의 kprobe가 fault 정보 캡처:                        │
 *   │      - Fault Address (IOVA) ← CQ의 DMA 주소                  │
 *   │      - Source ID (BDF)      ← NVMe 디바이스의 BDF             │
 *   │      - Fault Reason 0x05    ← "PTE Write access is not set"  │
 *   │      - Type = Write         ← DMA Write (CQ 기록)            │
 *   │                                                                 │
 *   │ 6. 즉시 IOMMU 매핑 복원 (Write 권한 재설정)                  │
 *   │                                                                 │
 *   │ ★ 결과: "디바이스가 CQ IOVA에 DMA Write를 시도했다"를        │
 *   │   PCIe 레벨에서 확인! 하지만 해당 DMA는 실패함.              │
 *   │   NVMe 컨트롤러 리셋이 필요할 수 있음.                       │
 *   └─────────────────────────────────────────────────────────────────┘
 *
 * ============================================================================
 * Fault 탐지 방법: dmar_fault() kprobe
 * ============================================================================
 *
 * iommu_set_fault_handler()는 DMA 도메인(cookie_type=DMA_IOVA)에서
 * 동작하지 않는다. Intel VT-d의 dmar_fault() IRQ handler를 직접
 * kprobe로 후킹하면 어떤 도메인에서든 fault를 잡을 수 있다.
 *
 * dmar_fault() 내부에서 Fault Record를 읽는 위치:
 *
 *   Fault Record Base = iommu->reg + cap_fault_reg_offset(iommu->cap)
 *   각 record는 16바이트:
 *     +0  [63:12] Fault Address (IOVA)
 *     +8  [15:0]  Source ID (BDF)
 *     +12 [7:0]   Fault Reason
 *     +12 [30]    Type (0=Write, 1=Read)
 *     +12 [31]    Valid bit
 *
 *   Fault Reason:
 *     0x05 = "PTE Write access is not set"  ← CQ Write 차단
 *     0x06 = "PTE Read access is not set"   ← SQ Fetch 차단
 *     0x01 = "Present bit in root entry is clear"
 *     0x02 = "Present bit in context entry is clear"
 *
 * ============================================================================
 * Fault 유발 방법: IOMMU PTE 권한 조작
 * ============================================================================
 *
 * Intel VT-d 2nd-Level PTE:
 *   bit 0: Read permission   (DMA_PTE_READ)
 *   bit 1: Write permission  (DMA_PTE_WRITE)
 *
 * CQ 영역의 Write 권한만 제거하면:
 *   → 디바이스의 DMA Read(있다면)는 통과
 *   → 디바이스의 DMA Write(CQ 기록)는 fault
 *   → Fault Reason = 0x05 "PTE Write access is not set"
 *
 * API:
 *   iommu_unmap(domain, cq_iova, cq_size)    → 매핑 완전 제거
 *   iommu_map(domain, cq_iova, cq_phys, cq_size, IOMMU_READ, GFP_KERNEL)
 *     → Read-Only로 재매핑 (Write만 차단)
 *   또는 완전 unmap 후 fault 확인 후 remap
 *
 * ============================================================================
 * ★ 경고: 이 모듈은 NVMe 디바이스를 죽입니다 ★
 * ============================================================================
 *
 * IOMMU fault = DMA abort = PCIe UR Completion
 * → NVMe 컨트롤러 CSTS.CFS=1 (Controller Fatal Status)
 * → nvme_reset_work() → 컨트롤러 리셋 필요
 *
 * 사용 대상:
 *   - 실험용 NVMe (데이터 손실 가능)
 *   - 또는 nvme_core.multipath=N 상태에서 여분의 NVMe
 *
 * 복구:
 *   echo 1 > /sys/class/nvme/nvmeX/reset_controller
 *   또는 rmmod nvme && modprobe nvme
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/iommu.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/timekeeping.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/*
 * ============================================================================
 * 모듈 파라미터
 * ============================================================================
 */

static char *target_pci = "";
module_param(target_pci, charp, 0444);
MODULE_PARM_DESC(target_pci,
	"Target NVMe PCI BDF (e.g., '0000:03:00.0')");

static int target_qid = 1;
module_param(target_qid, int, 0444);
MODULE_PARM_DESC(target_qid,
	"Target NVMe queue ID for CQ fault (default=1)");

/* nvme_queue 오프셋 (CQ IOVA 추출용) */
static int off_cq_dma_addr = -1;
static int off_q_depth = -1;
static int off_qid = -1;
module_param(off_cq_dma_addr, int, 0444);
module_param(off_q_depth, int, 0444);
module_param(off_qid, int, 0444);
MODULE_PARM_DESC(off_cq_dma_addr,
	"Offset of cq_dma_addr in nvme_queue (pahole로 확인)");

/*
 * ============================================================================
 * Fault 로그
 * ============================================================================
 */

#define MAX_FAULT_LOG 256

struct dmar_fault_record {
	u64  timestamp_ns;
	u64  fault_addr;       /* IOVA (4KB 정렬) */
	u16  source_id;        /* BDF: bus[15:8] devfn[7:0] */
	u8   fault_reason;     /* 0x05=Write denied, 0x06=Read denied 등 */
	u8   fault_type;       /* 0=DMA Write, 1=DMA Read */
	u32  pasid;
	bool pasid_present;
	char classification[16]; /* CQ_WRITE / SQ_FETCH / DATA / UNKNOWN */
};

struct fault_ctx {
	struct pci_dev *pdev;
	struct iommu_domain *domain;
	u16 target_bdf;         /* 타겟 NVMe의 BDF (Source ID 필터링용) */

	/* CQ 정보 (kprobe로 추출) */
	dma_addr_t cq_iova;     /* CQ DMA 주소 */
	phys_addr_t cq_phys;    /* CQ 물리 주소 (remap용) */
	size_t cq_size;          /* CQ 크기 (바이트) */
	u32 cq_q_depth;
	bool cq_info_captured;

	/* Fault 상태 */
	bool armed;              /* IOMMU PTE 조작 중 */

	/* Fault 로그 */
	struct dmar_fault_record log[MAX_FAULT_LOG];
	unsigned int log_head;
	unsigned int log_count;
	spinlock_t log_lock;

	/* 통계 */
	atomic64_t total_faults;
	atomic64_t target_faults; /* 타겟 NVMe에서 온 fault만 */

	/* debugfs */
	struct dentry *debugfs_dir;
};

static struct fault_ctx fctx;

/*
 * ============================================================================
 * Part 1: dmar_fault() kprobe - IOMMU Fault Record 직접 읽기
 * ============================================================================
 *
 * dmar_fault()는 IOMMU MSI IRQ handler이다.
 * 시그니처: irqreturn_t dmar_fault(int irq, void *dev_id)
 *   x86_64: RDI=irq, RSI=dev_id(struct intel_iommu *)
 *
 * struct intel_iommu의 첫 번째 필드가 reg(void __iomem *)이다.
 * iommu->reg + DMAR_FSTS_REG(0x34)에서 Fault Status를 읽고,
 * Fault Record에서 IOVA, Source ID, Reason을 추출한다.
 *
 * dmar_fault()에 kprobe를 걸면:
 *   - 어떤 도메인이든 상관없이 모든 IOMMU fault를 잡음
 *   - Fault Record의 원시 정보에 접근 가능
 *   - DMA 도메인에서도 동작 (iommu_set_fault_handler() 제약 없음)
 *
 * 그러나 dmar_fault() 진입 시점에는 아직 fault record를 처리하기 전이므로,
 * kretprobe를 사용하여 dmar_fault() 실행 후에 로그를 확인하거나,
 * dmar_fault_do_one()을 직접 후킹하는 것이 더 정확하다.
 *
 * dmar_fault_do_one() 시그니처:
 *   static int dmar_fault_do_one(struct intel_iommu *iommu, int type,
 *       u8 fault_reason, u32 pasid, u16 source_id, unsigned long long addr)
 *   x86_64: RDI=iommu, RSI=type, RDX=fault_reason,
 *           RCX=pasid, R8=source_id, R9=addr
 *
 * → dmar_fault_do_one()을 후킹하면 파싱된 정보를 바로 얻는다!
 */

static struct kprobe fault_kprobe;
static bool fault_kprobe_registered;

/*
 * dmar_fault_do_one_handler - IOMMU fault 정보 캡처
 *
 * dmar_fault_do_one()이 호출될 때 이미 fault record가 파싱되어
 * 함수 인자로 전달된다. 우리는 이 인자를 읽기만 하면 된다.
 *
 * 인자 (x86_64 calling convention):
 *   RDI: struct intel_iommu *iommu  (사용하지 않음)
 *   RSI: int type                   0=Write fault, 1=Read fault
 *   RDX: u8 fault_reason            0x05=Write denied, 0x06=Read denied
 *   RCX: u32 pasid
 *   R8:  u16 source_id              BDF (bus:dev.func)
 *   R9:  unsigned long long addr    Fault IOVA
 */
static int dmar_fault_do_one_handler(struct kprobe *p, struct pt_regs *regs)
{
	u64 ts = ktime_get_ns();
	int type = (int)regs->si;               /* 0=Write, 1=Read */
	u8 fault_reason = (u8)regs->dx;
	u32 pasid = (u32)regs->cx;
	u16 source_id = (u16)regs->r8;
	u64 fault_addr = (u64)regs->r9;

	struct dmar_fault_record *rec;
	unsigned long irq_flags;
	const char *classification = "UNKNOWN";
	bool is_target;

	atomic64_inc(&fctx.total_faults);

	/* 타겟 NVMe 디바이스의 BDF와 비교 */
	is_target = (source_id == fctx.target_bdf);
	if (is_target)
		atomic64_inc(&fctx.target_faults);

	/* IOVA 범위로 접근 유형 분류 */
	if (is_target && fctx.cq_info_captured) {
		if (fault_addr >= fctx.cq_iova &&
		    fault_addr < fctx.cq_iova + fctx.cq_size) {
			if (type == 0)
				classification = "CQ_WRITE";
			else
				classification = "CQ_READ";
		} else {
			classification = "DATA_DMA";
		}
	}

	/* 로그 기록 */
	spin_lock_irqsave(&fctx.log_lock, irq_flags);

	rec = &fctx.log[fctx.log_head % MAX_FAULT_LOG];
	rec->timestamp_ns = ts;
	rec->fault_addr = fault_addr;
	rec->source_id = source_id;
	rec->fault_reason = fault_reason;
	rec->fault_type = (u8)type;
	rec->pasid = pasid;
	rec->pasid_present = (pasid != 0);
	strscpy(rec->classification, classification,
		sizeof(rec->classification));
	fctx.log_head++;
	if (fctx.log_count < MAX_FAULT_LOG)
		fctx.log_count++;

	spin_unlock_irqrestore(&fctx.log_lock, irq_flags);

	/* trace_buffer에 기록 */
	{
		struct nvme_trace_event evt;

		memset(&evt, 0, sizeof(evt));
		evt.timestamp_ns = ts;
		evt.cpu = smp_processor_id();
		evt.event_type = (type == 0) ? EVT_DMA_MAP : EVT_DMA_UNMAP;
		evt.direction = DIR_DEVICE_TO_HOST;
		evt.address = fault_addr;
		evt.flags = 0x10; /* DETECTED_BY_IOMMU_FAULT */
		evt.dma.iova = fault_addr;
		evt.dma.dma_direction = type; /* 0=Write, 1=Read */

		trace_buffer_write(&evt);
	}

	/*
	 * Fault 로그 출력
	 *
	 * 형식: dmar_fault_do_one()과 동일한 정보지만
	 * 우리의 분류(CQ_WRITE 등)가 추가됨
	 */
	pr_info("nvme-iommu-fault: [%llu] ★ IOMMU FAULT ★\n"
		"  Source: %04x:%02x:%02x.%x %s\n"
		"  IOVA:   0x%016llx\n"
		"  Type:   DMA %s\n"
		"  Reason: 0x%02x (%s)\n"
		"  Class:  %s\n",
		ts,
		0, source_id >> 8,
		PCI_SLOT(source_id & 0xFF),
		PCI_FUNC(source_id & 0xFF),
		is_target ? "[TARGET]" : "",
		fault_addr,
		type == 0 ? "Write" : "Read",
		fault_reason,
		fault_reason == 0x05 ? "PTE Write access not set" :
		fault_reason == 0x06 ? "PTE Read access not set" :
		fault_reason == 0x01 ? "Root entry not present" :
		fault_reason == 0x02 ? "Context entry not present" :
		"other",
		classification);

	return 0;
}

/*
 * ============================================================================
 * Part 2: CQ IOVA 추출 (nvme_irq kprobe)
 * ============================================================================
 *
 * CQ의 DMA 주소(IOVA)를 알아야 IOMMU 매핑을 조작할 수 있다.
 * nvme_irq()의 두 번째 인자(nvme_queue *)에서 cq_dma_addr를 읽는다.
 */

static struct kprobe info_kprobe;
static bool info_kprobe_registered;

static int nvme_info_handler(struct kprobe *p, struct pt_regs *regs)
{
	void *nvmeq = (void *)regs->si;
	u16 qid;
	dma_addr_t cq_dma;
	u32 q_depth;

	if (fctx.cq_info_captured || !nvmeq)
		return 0;

	if (off_qid < 0 || off_cq_dma_addr < 0 || off_q_depth < 0)
		return 0;

	if (copy_from_kernel_nofault(&qid, nvmeq + off_qid, sizeof(qid)))
		return 0;

	if (qid != target_qid)
		return 0;

	if (copy_from_kernel_nofault(&cq_dma, nvmeq + off_cq_dma_addr,
				     sizeof(cq_dma)))
		return 0;
	if (copy_from_kernel_nofault(&q_depth, nvmeq + off_q_depth,
				     sizeof(q_depth)))
		return 0;

	if (cq_dma == 0 || q_depth == 0 || q_depth > 65536)
		return 0;

	fctx.cq_iova = cq_dma;
	fctx.cq_q_depth = q_depth;
	fctx.cq_size = q_depth * 16; /* CQE = 16 bytes */

	/*
	 * CQ의 물리 주소를 IOMMU 변환으로 추정
	 * dma_alloc_coherent()는 1:1 매핑이 아닐 수 있으므로
	 * iommu_iova_to_phys()로 변환한다.
	 */
	if (fctx.domain) {
		fctx.cq_phys = iommu_iova_to_phys(fctx.domain, cq_dma);
	}

	smp_wmb();
	fctx.cq_info_captured = true;

	pr_info("nvme-iommu-fault: CQ info captured for qid=%u\n"
		"  cq_iova=0x%llx cq_phys=0x%llx\n"
		"  q_depth=%u cq_size=%zu\n"
		"  → echo arm > /sys/kernel/debug/nvme-iommu-fault/control\n",
		qid, (u64)cq_dma, (u64)fctx.cq_phys,
		q_depth, fctx.cq_size);

	return 0;
}

/*
 * ============================================================================
 * Part 3: IOMMU PTE 조작 (arm/disarm)
 * ============================================================================
 *
 * arm:   CQ IOVA의 Write 권한 제거 → 디바이스 DMA Write 시 fault
 * disarm: CQ IOVA의 매핑 복원 → 정상 DMA 가능
 *
 * 방법:
 *   1. iommu_unmap(domain, cq_iova, cq_size) → 매핑 완전 제거
 *   2. iommu_map(domain, cq_iova, cq_phys, cq_size,
 *                IOMMU_READ, GFP_KERNEL)
 *      → Read-Only로 재매핑
 *      → 디바이스의 DMA Write → "PTE Write access is not set" (0x05)
 *      → 디바이스의 DMA Read는 통과
 *
 *   또는 완전히 unmap하면:
 *      → "Present bit in context entry is clear" (0x02) 또는
 *        "PTE Read/Write access is not set"
 *
 * IOTLB Flush:
 *   iommu_unmap()과 iommu_map() 내부에서 자동으로 IOTLB flush가
 *   수행되므로, 별도 flush는 불필요하다.
 */

static int arm_cq_fault(void)
{
	size_t unmapped;
	int ret;

	if (!fctx.cq_info_captured) {
		pr_err("nvme-iommu-fault: CQ info not captured yet. "
		       "Generate I/O first:\n"
		       "  dd if=/dev/nvmeXnY of=/dev/null bs=4k count=1\n");
		return -EAGAIN;
	}

	if (fctx.armed) {
		pr_warn("nvme-iommu-fault: already armed\n");
		return -EBUSY;
	}

	if (!fctx.domain) {
		pr_err("nvme-iommu-fault: no IOMMU domain\n");
		return -ENODEV;
	}

	pr_warn("nvme-iommu-fault: ★ ARMING ★\n"
		"  Removing Write permission for CQ IOVA 0x%llx (size=%zu)\n"
		"  Next device DMA Write to CQ will trigger IOMMU fault!\n"
		"  NVMe controller WILL enter fatal error state!\n",
		(u64)fctx.cq_iova, fctx.cq_size);

	/*
	 * Step 1: 기존 매핑 제거
	 */
	unmapped = iommu_unmap(fctx.domain, fctx.cq_iova, fctx.cq_size);
	if (unmapped != fctx.cq_size) {
		pr_err("nvme-iommu-fault: iommu_unmap partial: %zu/%zu\n"
		       "  IOMMU가 이 도메인을 관리하지 않거나\n"
		       "  identity mapping(passthrough)일 수 있음\n",
		       unmapped, fctx.cq_size);
		/*
		 * identity mapping인 경우 iommu_unmap()이 0을 반환한다.
		 * 이 경우 IOMMU가 DMA 주소를 직접 통과시키므로
		 * fault를 유발할 수 없다.
		 *
		 * 해결: intel_iommu=on,strict 부트 파라미터로
		 * 강제 IOMMU translation 모드를 사용해야 한다.
		 */
		if (unmapped == 0) {
			pr_err("nvme-iommu-fault: IOMMU passthrough mode!\n"
			       "  Boot with: intel_iommu=on iommu.passthrough=0\n");
			return -ENOTSUP;
		}
	}

	/*
	 * Step 2: Read-Only로 재매핑
	 *
	 * IOMMU_READ만 설정하면:
	 *   VT-d PTE: bit0(R)=1, bit1(W)=0
	 *   → DMA Read는 통과, DMA Write는 fault
	 *   → Fault Reason = 0x05 "PTE Write access is not set"
	 *
	 * 이렇게 하면 SQ Fetch(DMA Read)는 계속 동작하지만
	 * CQ Write(DMA Write)만 fault가 발생한다.
	 */
	ret = iommu_map(fctx.domain, fctx.cq_iova, fctx.cq_phys,
			fctx.cq_size, IOMMU_READ, GFP_KERNEL);
	if (ret) {
		pr_err("nvme-iommu-fault: iommu_map(READ-ONLY) failed: %d\n"
		       "  Attempting full remap to recover...\n", ret);
		/* 복구 시도 */
		iommu_map(fctx.domain, fctx.cq_iova, fctx.cq_phys,
			  fctx.cq_size, IOMMU_READ | IOMMU_WRITE,
			  GFP_KERNEL);
		return ret;
	}

	fctx.armed = true;

	pr_warn("nvme-iommu-fault: ★ ARMED ★\n"
		"  CQ IOVA 0x%llx is now READ-ONLY in IOMMU\n"
		"  Waiting for device DMA Write fault...\n"
		"  Generate I/O: dd if=/dev/nvmeXnY of=/dev/null bs=4k count=1\n"
		"  Then check: cat /sys/kernel/debug/nvme-iommu-fault/log\n");

	return 0;
}

static int disarm_cq_fault(void)
{
	size_t unmapped;
	int ret;

	if (!fctx.armed) {
		pr_info("nvme-iommu-fault: not armed\n");
		return 0;
	}

	if (!fctx.domain)
		return -ENODEV;

	pr_info("nvme-iommu-fault: disarming - restoring Write permission\n");

	/* Read-Only 매핑 제거 */
	unmapped = iommu_unmap(fctx.domain, fctx.cq_iova, fctx.cq_size);

	/* Read+Write로 재매핑 */
	ret = iommu_map(fctx.domain, fctx.cq_iova, fctx.cq_phys,
			fctx.cq_size, IOMMU_READ | IOMMU_WRITE,
			GFP_KERNEL);
	if (ret) {
		pr_err("nvme-iommu-fault: ★ CRITICAL ★ "
		       "Failed to restore IOMMU mapping! ret=%d\n"
		       "  NVMe device may be permanently broken!\n"
		       "  Try: echo 1 > /sys/class/nvme/nvmeX/"
		       "reset_controller\n", ret);
		return ret;
	}

	fctx.armed = false;

	pr_info("nvme-iommu-fault: disarmed - CQ IOVA 0x%llx restored (R+W)\n"
		"  NVMe controller may need reset:\n"
		"    echo 1 > /sys/class/nvme/nvmeX/reset_controller\n",
		(u64)fctx.cq_iova);

	return 0;
}

/*
 * ============================================================================
 * Part 4: debugfs 인터페이스
 * ============================================================================
 */

/* control: arm / disarm / info */
static ssize_t control_write(struct file *file, const char __user *buf,
			     size_t count, loff_t *ppos)
{
	char cmd[16];
	size_t len = min(count, sizeof(cmd) - 1);

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;
	cmd[len] = '\0';

	/* 개행 제거 */
	if (len > 0 && cmd[len - 1] == '\n')
		cmd[len - 1] = '\0';

	if (strcmp(cmd, "arm") == 0)
		arm_cq_fault();
	else if (strcmp(cmd, "disarm") == 0)
		disarm_cq_fault();
	else
		pr_err("nvme-iommu-fault: unknown command '%s' "
		       "(use: arm, disarm)\n", cmd);

	return count;
}

static const struct file_operations control_fops = {
	.owner = THIS_MODULE,
	.write = control_write,
};

/* log: Fault Record 이력 */
static int log_show(struct seq_file *m, void *v)
{
	unsigned long flags;
	unsigned int i, start, count;

	seq_printf(m, "IOMMU Fault Log (target=%s BDF=0x%04x)\n",
		   target_pci, fctx.target_bdf);
	seq_printf(m, "Total: %lld  Target: %lld  Armed: %s\n\n",
		   atomic64_read(&fctx.total_faults),
		   atomic64_read(&fctx.target_faults),
		   fctx.armed ? "YES" : "no");

	seq_printf(m, "%-18s %-18s %-12s %-6s %-8s %s\n",
		   "Timestamp(ns)", "IOVA", "Source(BDF)", "R/W",
		   "Reason", "Class");
	seq_puts(m, "-----------------------------------------------"
		 "-----------------------------------------------\n");

	spin_lock_irqsave(&fctx.log_lock, flags);

	count = min(fctx.log_count, (unsigned int)MAX_FAULT_LOG);
	start = (fctx.log_head >= count) ? (fctx.log_head - count) : 0;

	for (i = 0; i < count; i++) {
		struct dmar_fault_record *r;

		r = &fctx.log[(start + i) % MAX_FAULT_LOG];
		seq_printf(m, "%-18llu 0x%016llx %02x:%02x.%x      %-6s 0x%02x     %s\n",
			   r->timestamp_ns, r->fault_addr,
			   r->source_id >> 8,
			   PCI_SLOT(r->source_id & 0xFF),
			   PCI_FUNC(r->source_id & 0xFF),
			   r->fault_type == 0 ? "Write" : "Read",
			   r->fault_reason,
			   r->classification);
	}

	spin_unlock_irqrestore(&fctx.log_lock, flags);
	return 0;
}

static int log_open(struct inode *inode, struct file *file)
{
	return single_open(file, log_show, NULL);
}

static const struct file_operations log_fops = {
	.owner = THIS_MODULE,
	.open = log_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/* status: 현재 상태 요약 */
static int status_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Target PCI:     %s\n", target_pci);
	seq_printf(m, "Target BDF:     0x%04x (%02x:%02x.%x)\n",
		   fctx.target_bdf,
		   fctx.target_bdf >> 8,
		   PCI_SLOT(fctx.target_bdf & 0xFF),
		   PCI_FUNC(fctx.target_bdf & 0xFF));
	seq_printf(m, "Target QID:     %d\n", target_qid);
	seq_printf(m, "IOMMU Domain:   %s (type=%u)\n",
		   fctx.domain ? "found" : "NONE",
		   fctx.domain ? fctx.domain->type : 0);
	seq_printf(m, "CQ Captured:    %s\n",
		   fctx.cq_info_captured ? "YES" : "no");

	if (fctx.cq_info_captured) {
		seq_printf(m, "  CQ IOVA:      0x%llx\n",
			   (u64)fctx.cq_iova);
		seq_printf(m, "  CQ Phys:      0x%llx\n",
			   (u64)fctx.cq_phys);
		seq_printf(m, "  CQ Size:      %zu (%u entries x 16B)\n",
			   fctx.cq_size, fctx.cq_q_depth);
	}

	seq_printf(m, "Armed:          %s\n",
		   fctx.armed ? "★ YES - CQ Write will fault! ★" : "no");
	seq_printf(m, "Total Faults:   %lld\n",
		   atomic64_read(&fctx.total_faults));
	seq_printf(m, "Target Faults:  %lld\n",
		   atomic64_read(&fctx.target_faults));

	if (!fctx.cq_info_captured)
		seq_puts(m, "\nNext: Generate I/O to capture CQ info:\n"
			 "  dd if=/dev/nvmeXnY of=/dev/null bs=4k count=1\n");
	else if (!fctx.armed)
		seq_puts(m, "\nNext: Arm the fault:\n"
			 "  echo arm > /sys/kernel/debug/"
			 "nvme-iommu-fault/control\n");
	else
		seq_puts(m, "\nNext: Generate I/O to trigger fault:\n"
			 "  dd if=/dev/nvmeXnY of=/dev/null bs=4k count=1\n"
			 "Then: cat /sys/kernel/debug/"
			 "nvme-iommu-fault/log\n");

	return 0;
}

static int status_open(struct inode *inode, struct file *file)
{
	return single_open(file, status_show, NULL);
}

static const struct file_operations status_fops = {
	.owner = THIS_MODULE,
	.open = status_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
 * ============================================================================
 * Part 5: 초기화 / 해제
 * ============================================================================
 */

int nvme_iommu_fault_tracer_init(void)
{
	struct pci_dev *pdev;
	struct iommu_domain *domain;
	int ret;

	memset(&fctx, 0, sizeof(fctx));
	spin_lock_init(&fctx.log_lock);

	/* 파라미터 검증 */
	if (!target_pci || !target_pci[0]) {
		pr_err("nvme-iommu-fault: target_pci required!\n"
		       "  Usage: insmod nvme_iommu_fault_tracer.ko \\\n"
		       "    target_pci='0000:03:00.0' \\\n"
		       "    off_cq_dma_addr=XX off_q_depth=XX off_qid=XX\n"
		       "  Find NVMe: lspci -d ::0108\n"
		       "  Find offsets: pahole -C nvme_queue "
		       "/sys/kernel/btf/vmlinux\n");
		return -EINVAL;
	}

	if (off_cq_dma_addr < 0 || off_q_depth < 0 || off_qid < 0) {
		pr_err("nvme-iommu-fault: nvme_queue offsets required!\n"
		       "  off_cq_dma_addr, off_q_depth, off_qid\n"
		       "  Use: pahole -C nvme_queue "
		       "/sys/kernel/btf/vmlinux\n");
		return -EINVAL;
	}

	/*
	 * Step 1: PCI 디바이스 찾기
	 */
	pdev = pci_get_domain_bus_and_slot(
		0,
		simple_strtoul(target_pci + 5, NULL, 16),
		PCI_DEVFN(
			simple_strtoul(target_pci + 8, NULL, 16),
			simple_strtoul(target_pci + 11, NULL, 16)
		)
	);
	if (!pdev) {
		pr_err("nvme-iommu-fault: PCI device %s not found\n",
		       target_pci);
		return -ENODEV;
	}
	fctx.pdev = pdev;
	fctx.target_bdf = (pdev->bus->number << 8) | pdev->devfn;

	pr_info("nvme-iommu-fault: target %s [%04x:%04x] BDF=0x%04x\n",
		pci_name(pdev), pdev->vendor, pdev->device,
		fctx.target_bdf);

	/*
	 * Step 2: IOMMU 도메인 획득
	 */
	domain = iommu_get_domain_for_dev(&pdev->dev);
	if (!domain) {
		pr_err("nvme-iommu-fault: no IOMMU domain for %s\n"
		       "  Boot with: intel_iommu=on iommu.passthrough=0\n",
		       pci_name(pdev));
		pci_dev_put(pdev);
		return -ENODEV;
	}
	fctx.domain = domain;

	pr_info("nvme-iommu-fault: IOMMU domain found (type=%u)\n",
		domain->type);

	/*
	 * Step 3: dmar_fault_do_one()에 kprobe 등록
	 *
	 * 이것이 핵심: 모든 IOMMU fault를 잡는다.
	 * DMA 도메인이든 VFIO 도메인이든 상관없이 동작한다.
	 */
	memset(&fault_kprobe, 0, sizeof(fault_kprobe));
	fault_kprobe.symbol_name = "dmar_fault_do_one";
	fault_kprobe.pre_handler = dmar_fault_do_one_handler;

	ret = register_kprobe(&fault_kprobe);
	if (ret < 0) {
		pr_err("nvme-iommu-fault: kprobe on dmar_fault_do_one "
		       "failed (ret=%d)\n"
		       "  Is Intel IOMMU enabled?\n"
		       "  dmesg | grep -i DMAR\n", ret);
		pci_dev_put(pdev);
		return ret;
	}
	fault_kprobe_registered = true;

	pr_info("nvme-iommu-fault: kprobe registered on "
		"dmar_fault_do_one()\n");

	/*
	 * Step 4: nvme_irq()에 kprobe 등록 (CQ IOVA 추출)
	 */
	memset(&info_kprobe, 0, sizeof(info_kprobe));
	info_kprobe.symbol_name = "nvme_irq";
	info_kprobe.pre_handler = nvme_info_handler;

	ret = register_kprobe(&info_kprobe);
	if (ret < 0) {
		pr_warn("nvme-iommu-fault: kprobe on nvme_irq failed "
			"(ret=%d). CQ IOVA auto-detection unavailable.\n",
			ret);
		info_kprobe_registered = false;
	} else {
		info_kprobe_registered = true;
		pr_info("nvme-iommu-fault: kprobe registered on nvme_irq "
			"(CQ IOVA extraction)\n");
	}

	/*
	 * Step 5: debugfs 생성
	 */
	fctx.debugfs_dir = debugfs_create_dir("nvme-iommu-fault", NULL);
	debugfs_create_file("control", 0200, fctx.debugfs_dir,
			    NULL, &control_fops);
	debugfs_create_file("log", 0444, fctx.debugfs_dir,
			    NULL, &log_fops);
	debugfs_create_file("status", 0444, fctx.debugfs_dir,
			    NULL, &status_fops);

	pr_info("nvme-iommu-fault: ========================================\n"
		"  Module loaded. Usage:\n"
		"  1. cat /sys/kernel/debug/nvme-iommu-fault/status\n"
		"  2. dd if=/dev/nvmeXnY of=/dev/null bs=4k count=1\n"
		"     (captures CQ IOVA)\n"
		"  3. echo arm > /sys/kernel/debug/nvme-iommu-fault/control\n"
		"     (removes CQ Write permission → arms fault)\n"
		"  4. dd if=/dev/nvmeXnY of=/dev/null bs=4k count=1\n"
		"     (triggers IOMMU fault on CQ DMA Write)\n"
		"  5. cat /sys/kernel/debug/nvme-iommu-fault/log\n"
		"     (shows captured fault: IOVA, BDF, reason)\n"
		"  6. echo disarm > /sys/kernel/debug/nvme-iommu-fault/"
		"control\n"
		"  7. echo 1 > /sys/class/nvme/nvmeX/reset_controller\n"
		"=========================================================\n");

	return 0;
}

void nvme_iommu_fault_tracer_exit(void)
{
	/* 먼저 disarm */
	if (fctx.armed)
		disarm_cq_fault();

	/* kprobe 해제 */
	if (fault_kprobe_registered) {
		unregister_kprobe(&fault_kprobe);
		fault_kprobe_registered = false;
	}

	if (info_kprobe_registered) {
		unregister_kprobe(&info_kprobe);
		info_kprobe_registered = false;
	}

	debugfs_remove_recursive(fctx.debugfs_dir);

	if (fctx.pdev)
		pci_dev_put(fctx.pdev);

	synchronize_rcu();

	pr_info("nvme-iommu-fault: unloaded. total=%lld target=%lld\n",
		atomic64_read(&fctx.total_faults),
		atomic64_read(&fctx.target_faults));
}
