// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_iommu_fault_tracer.c - IOMMU Fault 기반 PCIe DMA 접근 탐지
 *
 * ============================================================================
 * 목적
 * ============================================================================
 *
 * NVMe 디바이스의 DMA 접근을 IOMMU page fault 메커니즘으로 탐지한다.
 * kprobe가 커널 함수 호출을 잡는 "간접적" 방법이라면,
 * IOMMU fault는 실제 PCIe TLP(Transaction Layer Packet)가
 * IOMMU를 통과하는 시점을 잡는 "직접적" 방법이다.
 *
 * ============================================================================
 * 동작 원리
 * ============================================================================
 *
 * 1. NVMe 디바이스의 IOMMU 도메인을 획득한다.
 * 2. iommu_set_fault_handler()로 커스텀 fault handler를 등록한다.
 * 3. 정상 동작 중 IOMMU fault는 발생하지 않는다.
 * 4. 의도적으로 IOMMU 매핑을 조작하면 fault를 유발할 수 있다.
 *
 * ============================================================================
 * IOMMU Fault 경로 (Intel VT-d)
 * ============================================================================
 *
 * 디바이스가 DMA 접근 시도
 *   → PCIe TLP (Memory Read/Write)
 *     → IOMMU가 IOVA → HPA 변환 시도
 *       → IOMMU PTE에 접근 권한 없음 (R=0 또는 W=0)
 *         → IOMMU Fault Record 생성
 *           → IOMMU MSI 인터럽트 → CPU
 *             → dmar_fault() IRQ handler    [drivers/iommu/intel/dmar.c]
 *               → dmar_fault_do_one()
 *
 * dmar_fault_do_one()은 기본적으로 dmesg에 경고만 출력한다:
 *   "DMAR: DRHD: handling fault status reg 2"
 *   "DMAR: [DMA Read/Write] Request device [03:00.0] ..."
 *
 * report_iommu_fault()가 호출되면 우리의 handler가 실행된다.
 *
 * ============================================================================
 * Intel VT-d Fault Record 구조
 * ============================================================================
 *
 * Fault Recording Register (16 bytes per record):
 *
 *   +0 [63:12] Fault Address (IOVA, 4KB 정렬)
 *   +0 [11:0]  Reserved
 *   +8 [31]    Fault (F) - valid bit
 *   +8 [30]    Type (T) - 0=DMA Write fault, 1=DMA Read fault
 *   +8 [29]    PASID Present (PP)
 *   +8 [27:20] Fault Reason
 *   +8 [19:8]  Source ID [bus:dev.func]
 *   +8 [7:0]   PASID value (lower bits)
 *
 * Fault Reason 코드:
 *   0x01: "Present bit in root entry is clear"
 *   0x02: "Present bit in context entry is clear"
 *   0x05: "PTE Write access is not set"    ← CQ Write, Data DMA Write
 *   0x06: "PTE Read access is not set"     ← SQ Fetch (DMA Read)
 *   0x07: "Next page table ptr is invalid"
 *
 * ============================================================================
 * 주의사항
 * ============================================================================
 *
 * ★ IOMMU fault는 DMA를 abort시킨다! ★
 *
 * CPU의 kmmio(PTE fault)는 Single-Step으로 접근을 허용한 후 다시 차단하지만,
 * IOMMU fault는 해당 DMA 트랜잭션을 중단(abort)시킨다.
 * PCIe Completion에 UR(Unsupported Request)가 반환되어
 * NVMe 컨트롤러가 에러 상태(CSTS.CFS=1)에 빠질 수 있다.
 *
 * 따라서 이 모듈은:
 *   - 교육/실험 목적으로 IOMMU fault 메커니즘을 이해하는 용도
 *   - 의도적 fault 유발은 별도 디바이스(사용하지 않는 NVMe)에서만
 *   - 정상 운영 중에는 fault handler 등록만 하고 관찰만 한다
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/iommu.h>
#include <linux/dma-mapping.h>
#include <linux/timekeeping.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/* 모듈 파라미터 */
static char *target_pci = "";
module_param(target_pci, charp, 0444);
MODULE_PARM_DESC(target_pci,
	"Target NVMe PCI device BDF (e.g., '0000:03:00.0')");

/*
 * 의도적 fault 유발 모드
 * 0: 관찰만 (안전)
 * 1: 지정된 IOVA 영역의 매핑을 해제하여 fault 유발 (위험!)
 */
static int provoke_fault;
module_param(provoke_fault, int, 0644);
MODULE_PARM_DESC(provoke_fault,
	"0=observe only (safe), 1=provoke IOMMU fault (DANGEROUS!)");

/*
 * ============================================================================
 * IOMMU Fault 컨텍스트
 * ============================================================================
 */

#define MAX_FAULT_LOG 256

struct iommu_fault_entry {
	u64 timestamp_ns;
	unsigned long iova;       /* Fault 주소 */
	int flags;                /* IOMMU_FAULT_READ/WRITE */
	char dev_name[32];        /* 디바이스 이름 */
	char classification[32];  /* SQ_FETCH / CQ_WRITE / DATA_DMA / UNKNOWN */
};

struct iommu_fault_ctx {
	struct pci_dev *pdev;
	struct iommu_domain *domain;

	/* Fault 로그 (ring buffer) */
	struct iommu_fault_entry log[MAX_FAULT_LOG];
	unsigned int log_head;
	unsigned int log_count;
	spinlock_t log_lock;

	/* NVMe DMA 주소 범위 (분류용, kprobe로 추출) */
	struct {
		dma_addr_t sq_iova;
		size_t sq_size;
		dma_addr_t cq_iova;
		size_t cq_size;
	} nvme_ranges;
	bool ranges_known;

	/* 통계 */
	atomic64_t total_faults;
	atomic64_t read_faults;   /* DMA Read fault (SQ Fetch 등) */
	atomic64_t write_faults;  /* DMA Write fault (CQ Write 등) */

	/* debugfs */
	struct dentry *debugfs_dir;
};

static struct iommu_fault_ctx fault_ctx;

/*
 * ============================================================================
 * IOMMU Fault Handler
 * ============================================================================
 *
 * iommu_set_fault_handler()로 등록하는 콜백이다.
 *
 * 이 핸들러가 호출되는 경로 (Intel VT-d):
 *
 *   dmar_fault() [drivers/iommu/intel/dmar.c:1899]
 *     IOMMU MSI IRQ handler. Fault Status Register(DMAR_FSTS_REG)를 읽고
 *     Fault Record에서 정보를 추출한다.
 *
 *   → dmar_fault_do_one() [dmar.c:1806]
 *       pr_err("DMAR: [DMA %s] ... fault addr %llx [fault reason %s] ...")
 *       Fault 정보를 dmesg에 출력한다.
 *
 *   현재 커널(6.x)에서는 Intel VT-d의 non-recoverable fault에 대해
 *   report_iommu_fault()가 호출되지 않는 경우가 있다.
 *   이 경우 dmesg의 DMAR 에러 메시지를 파싱해야 한다.
 *
 *   확실한 호출을 위해서는 커널 패치가 필요할 수 있다:
 *   dmar_fault_do_one()에서 report_iommu_fault() 호출 추가.
 *
 * 시그니처:
 *   int handler(struct iommu_domain *domain, struct device *dev,
 *               unsigned long iova, int flags, void *token)
 *
 * flags:
 *   IOMMU_FAULT_READ  (1): DMA Read fault
 *   IOMMU_FAULT_WRITE (2): DMA Write fault
 *
 * 반환값:
 *   -ENOSYS: 핸들러가 처리하지 않음
 *   0: 처리 완료
 */
static int nvme_iommu_fault_handler(struct iommu_domain *domain,
				    struct device *dev,
				    unsigned long iova,
				    int flags, void *token)
{
	struct iommu_fault_ctx *ctx = token;
	struct iommu_fault_entry *entry;
	u64 ts = ktime_get_ns();
	unsigned long irq_flags;
	const char *classification = "UNKNOWN";

	atomic64_inc(&ctx->total_faults);

	if (flags & IOMMU_FAULT_READ)
		atomic64_inc(&ctx->read_faults);
	if (flags & IOMMU_FAULT_WRITE)
		atomic64_inc(&ctx->write_faults);

	/*
	 * IOVA 범위로 접근 유형 분류
	 *
	 * NVMe DMA 매핑:
	 *   SQ: dma_alloc_coherent() → sq_dma_addr
	 *        디바이스가 DMA Read로 커맨드를 가져감 (SQ Fetch)
	 *   CQ: dma_alloc_coherent() → cq_dma_addr
	 *        디바이스가 DMA Write로 완료 엔트리를 기록함 (CQ Write)
	 *   Data: dma_map_sg() → scatter-gather IOVA
	 *        데이터 읽기/쓰기 DMA
	 */
	if (ctx->ranges_known) {
		if (iova >= ctx->nvme_ranges.sq_iova &&
		    iova < ctx->nvme_ranges.sq_iova +
			    ctx->nvme_ranges.sq_size) {
			classification = "SQ_FETCH";
		} else if (iova >= ctx->nvme_ranges.cq_iova &&
			   iova < ctx->nvme_ranges.cq_iova +
				   ctx->nvme_ranges.cq_size) {
			classification = "CQ_WRITE";
		} else {
			classification = "DATA_DMA";
		}
	}

	/* Fault 로그에 기록 */
	spin_lock_irqsave(&ctx->log_lock, irq_flags);

	entry = &ctx->log[ctx->log_head % MAX_FAULT_LOG];
	entry->timestamp_ns = ts;
	entry->iova = iova;
	entry->flags = flags;
	strscpy(entry->dev_name, dev_name(dev), sizeof(entry->dev_name));
	strscpy(entry->classification, classification,
		sizeof(entry->classification));
	ctx->log_head++;
	if (ctx->log_count < MAX_FAULT_LOG)
		ctx->log_count++;

	spin_unlock_irqrestore(&ctx->log_lock, irq_flags);

	/* trace_buffer에도 기록 */
	{
		struct nvme_trace_event evt;

		memset(&evt, 0, sizeof(evt));
		evt.timestamp_ns = ts;
		evt.cpu = smp_processor_id();

		if (flags & IOMMU_FAULT_READ) {
			evt.event_type = EVT_DMA_MAP; /* SQ Fetch 등 */
		} else {
			evt.event_type = EVT_DMA_UNMAP; /* CQ Write 등 */
		}

		evt.direction = DIR_DEVICE_TO_HOST;
		evt.address = iova;
		evt.flags = 0x10; /* DETECTED_BY_IOMMU_FAULT */
		evt.dma.iova = iova;
		evt.dma.dma_direction = (flags & IOMMU_FAULT_WRITE) ? 1 : 0;

		trace_buffer_write(&evt);
	}

	pr_info("nvme-iommu-fault: [%llu] FAULT! "
		"iova=0x%lx dev=%s %s → %s\n",
		ts, iova, dev_name(dev),
		(flags & IOMMU_FAULT_READ) ? "DMA_READ" :
		(flags & IOMMU_FAULT_WRITE) ? "DMA_WRITE" : "??",
		classification);

	return -ENOSYS; /* 기본 fault 처리로 넘김 */
}

/*
 * ============================================================================
 * debugfs: Fault 로그 조회
 * ============================================================================
 */

static int fault_log_show(struct seq_file *m, void *v)
{
	unsigned long flags;
	unsigned int i, start, count;

	seq_printf(m, "IOMMU Fault Log (target=%s)\n", target_pci);
	seq_printf(m, "Total faults: %lld (read=%lld, write=%lld)\n\n",
		   atomic64_read(&fault_ctx.total_faults),
		   atomic64_read(&fault_ctx.read_faults),
		   atomic64_read(&fault_ctx.write_faults));

	seq_printf(m, "%-20s %-18s %-12s %-16s %s\n",
		   "Timestamp(ns)", "IOVA", "Type", "Classification",
		   "Device");
	seq_puts(m, "------------------------------------------------------"
		 "-----------------------------------\n");

	spin_lock_irqsave(&fault_ctx.log_lock, flags);

	count = fault_ctx.log_count;
	if (count > MAX_FAULT_LOG)
		count = MAX_FAULT_LOG;

	start = (fault_ctx.log_head >= count) ?
		(fault_ctx.log_head - count) : 0;

	for (i = 0; i < count; i++) {
		struct iommu_fault_entry *e;

		e = &fault_ctx.log[(start + i) % MAX_FAULT_LOG];
		seq_printf(m, "%-20llu 0x%016lx %-12s %-16s %s\n",
			   e->timestamp_ns, e->iova,
			   (e->flags & IOMMU_FAULT_READ) ? "DMA_READ" :
			   (e->flags & IOMMU_FAULT_WRITE) ? "DMA_WRITE" :
			   "UNKNOWN",
			   e->classification, e->dev_name);
	}

	spin_unlock_irqrestore(&fault_ctx.log_lock, flags);

	return 0;
}

static int fault_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, fault_log_show, NULL);
}

static const struct file_operations fault_log_fops = {
	.owner = THIS_MODULE,
	.open = fault_log_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
 * ============================================================================
 * debugfs: IOMMU 정보 조회
 * ============================================================================
 */

static int iommu_info_show(struct seq_file *m, void *v)
{
	struct iommu_domain *domain = fault_ctx.domain;

	seq_printf(m, "Target PCI Device: %s\n", target_pci);
	seq_printf(m, "Device: %s\n",
		   fault_ctx.pdev ? pci_name(fault_ctx.pdev) : "none");

	if (domain) {
		seq_printf(m, "IOMMU Domain Type: %u\n", domain->type);
		seq_printf(m, "Page Size Bitmap: 0x%lx\n",
			   domain->pgsize_bitmap);
		seq_printf(m, "Geometry:\n");
		seq_printf(m, "  Aperture Start: 0x%llx\n",
			   domain->geometry.aperture_start);
		seq_printf(m, "  Aperture End:   0x%llx\n",
			   domain->geometry.aperture_end);
	} else {
		seq_puts(m, "IOMMU Domain: not found\n");
	}

	if (fault_ctx.ranges_known) {
		seq_printf(m, "\nNVMe DMA Ranges:\n");
		seq_printf(m, "  SQ IOVA: 0x%llx (size=%zu)\n",
			   (u64)fault_ctx.nvme_ranges.sq_iova,
			   fault_ctx.nvme_ranges.sq_size);
		seq_printf(m, "  CQ IOVA: 0x%llx (size=%zu)\n",
			   (u64)fault_ctx.nvme_ranges.cq_iova,
			   fault_ctx.nvme_ranges.cq_size);
	}

	return 0;
}

static int iommu_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, iommu_info_show, NULL);
}

static const struct file_operations iommu_info_fops = {
	.owner = THIS_MODULE,
	.open = iommu_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
 * ============================================================================
 * 초기화 / 해제
 * ============================================================================
 */

int nvme_iommu_fault_tracer_init(void)
{
	struct pci_dev *pdev;
	struct iommu_domain *domain;

	memset(&fault_ctx, 0, sizeof(fault_ctx));
	spin_lock_init(&fault_ctx.log_lock);

	if (!target_pci || !target_pci[0]) {
		pr_err("nvme-iommu-fault: target_pci not specified!\n"
		       "  Usage: insmod ... target_pci='0000:03:00.0'\n"
		       "  Find NVMe: lspci -d ::0108\n");
		return -EINVAL;
	}

	/*
	 * Step 1: PCI 디바이스 찾기
	 */
	pdev = pci_get_domain_bus_and_slot(
		0, /* domain */
		simple_strtoul(target_pci + 5, NULL, 16), /* bus */
		PCI_DEVFN(
			simple_strtoul(target_pci + 8, NULL, 16), /* dev */
			simple_strtoul(target_pci + 11, NULL, 16) /* func */
		)
	);

	if (!pdev) {
		pr_err("nvme-iommu-fault: PCI device %s not found\n",
		       target_pci);
		return -ENODEV;
	}

	fault_ctx.pdev = pdev;
	pr_info("nvme-iommu-fault: found PCI device %s [%04x:%04x]\n",
		pci_name(pdev), pdev->vendor, pdev->device);

	/*
	 * Step 2: IOMMU 도메인 획득
	 *
	 * iommu_get_domain_for_dev() (drivers/iommu/iommu.c):
	 *   dev->iommu_group->domain을 반환한다.
	 *   IOMMU가 비활성화되어 있으면 NULL을 반환한다.
	 *
	 * IOMMU가 필요한 이유:
	 *   커널 부트 파라미터에 intel_iommu=on이 있어야 한다.
	 *   (또는 DMAR ACPI 테이블이 있고 커널이 자동 활성화)
	 */
	domain = iommu_get_domain_for_dev(&pdev->dev);
	if (!domain) {
		pr_err("nvme-iommu-fault: no IOMMU domain for %s\n"
		       "  Is IOMMU enabled? Check:\n"
		       "    dmesg | grep -i iommu\n"
		       "    cat /proc/cmdline | grep intel_iommu\n"
		       "  Enable: add 'intel_iommu=on' to kernel cmdline\n",
		       pci_name(pdev));
		pci_dev_put(pdev);
		return -ENODEV;
	}

	fault_ctx.domain = domain;
	pr_info("nvme-iommu-fault: IOMMU domain found (type=%u)\n",
		domain->type);

	/*
	 * Step 3: Fault Handler 등록
	 *
	 * iommu_set_fault_handler() (drivers/iommu/iommu.c):
	 *   domain->handler = handler
	 *   domain->handler_token = token
	 *
	 * 제약사항:
	 *   - domain->cookie_type이 IOMMU_COOKIE_NONE이어야 등록 가능
	 *   - DMA-IOMMU가 관리하는 도메인에서는 cookie_type이
	 *     IOMMU_COOKIE_DMA_IOVA이므로 등록이 실패할 수 있다!
	 *   - 이 경우 커널 패치가 필요하거나 다른 방법을 사용해야 한다
	 *
	 * 대안:
	 *   1. VFIO passthrough 도메인 사용 (cookie_type=NONE)
	 *   2. dmar_fault() 자체를 kprobe로 후킹
	 *   3. dmesg의 DMAR 에러 메시지를 파싱
	 */
	iommu_set_fault_handler(domain, nvme_iommu_fault_handler,
				&fault_ctx);
	pr_info("nvme-iommu-fault: fault handler registered\n");

	/*
	 * Step 4: debugfs 인터페이스 생성
	 */
	fault_ctx.debugfs_dir = debugfs_create_dir("nvme-iommu-fault", NULL);
	debugfs_create_file("fault_log", 0444, fault_ctx.debugfs_dir,
			    NULL, &fault_log_fops);
	debugfs_create_file("iommu_info", 0444, fault_ctx.debugfs_dir,
			    NULL, &iommu_info_fops);

	pr_info("nvme-iommu-fault: module loaded for %s\n"
		"  debugfs: /sys/kernel/debug/nvme-iommu-fault/\n"
		"  fault_log: cat /sys/kernel/debug/nvme-iommu-fault/"
		"fault_log\n"
		"  iommu_info: cat /sys/kernel/debug/nvme-iommu-fault/"
		"iommu_info\n",
		target_pci);

	return 0;
}

void nvme_iommu_fault_tracer_exit(void)
{
	/*
	 * Fault handler 해제
	 *
	 * iommu_set_fault_handler()는 unregister API가 없다.
	 * domain->handler를 NULL로 설정하는 것도 안전하지 않다
	 * (경쟁 조건). 모듈 해제 전에 synchronize_rcu()로 대기한다.
	 *
	 * 실제로는 도메인이 살아있는 한 handler도 유효해야 하므로,
	 * 모듈을 해제하기 전에 디바이스를 사용하지 않는 상태여야 한다.
	 */

	debugfs_remove_recursive(fault_ctx.debugfs_dir);

	if (fault_ctx.pdev)
		pci_dev_put(fault_ctx.pdev);

	synchronize_rcu();

	pr_info("nvme-iommu-fault: unloaded. total_faults=%lld\n",
		atomic64_read(&fault_ctx.total_faults));
}

/*
 * ============================================================================
 * 대안: dmar_fault()를 kprobe로 직접 후킹
 * ============================================================================
 *
 * iommu_set_fault_handler()가 DMA 도메인에서 동작하지 않는 경우,
 * Intel VT-d의 dmar_fault() IRQ handler를 kprobe로 후킹하여
 * IOMMU fault 정보를 직접 읽을 수 있다.
 *
 * dmar_fault()는 IOMMU의 Fault Status/Record 레지스터를
 * 직접 읽으므로, 가장 원시적인 fault 정보에 접근할 수 있다.
 *
 * 구현 골격:
 *
 * static struct kretprobe dmar_fault_kretprobe;
 *
 * static int dmar_fault_entry(struct kretprobe_instance *ri,
 *                              struct pt_regs *regs)
 * {
 *     // RDI=irq, RSI=dev_id(struct intel_iommu *)
 *     struct intel_iommu *iommu = (void *)regs->si;
 *
 *     // IOMMU의 Fault Status Register 읽기
 *     // → 어떤 디바이스(BDF)가 어떤 주소(IOVA)에 접근 시도했는지 확인
 *     // → Source ID로 NVMe 디바이스인지 필터링
 *     // → Fault Address로 SQ/CQ/Data 분류
 *
 *     // 주의: intel_iommu 구조체도 내부 구조체이므로
 *     //       레지스터 오프셋을 알아야 한다.
 *     //       iommu->reg + DMAR_FSTS_REG(0x34)
 *     return 0;
 * }
 *
 * 이 방법은 iommu_set_fault_handler()보다 더 low-level이지만,
 * DMA 도메인에서도 동작한다.
 */
