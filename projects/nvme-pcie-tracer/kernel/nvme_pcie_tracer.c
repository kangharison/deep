// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_pcie_tracer.c - NVMe PCIe-Level Bidirectional Command Tracer (메인 모듈)
 *
 * 모든 서브 트레이서를 통합하여 단일 커널 모듈로 제공한다.
 * module_init()에서 trace_buffer를 초기화하고 각 서브 트레이서의 kprobe를 등록한다.
 * module_exit()에서 모든 kprobe를 해제하고 trace_buffer를 정리한다.
 *
 * 사용법:
 *   sudo insmod nvme_pcie_tracer.ko [target_dev=nvme0] [buffer_size_kb=256]
 *   echo 1 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled
 *   cat /sys/kernel/debug/nvme-pcie-tracer/trace0  # CPU 0 이벤트 읽기
 *   echo 0 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled
 *   cat /sys/kernel/debug/nvme-pcie-tracer/stats
 *   sudo rmmod nvme_pcie_tracer
 *
 * debugfs 인터페이스:
 *   /sys/kernel/debug/nvme-pcie-tracer/
 *   ├── enabled        # 트레이싱 on/off (echo 1/0)
 *   ├── trace0..N      # per-CPU 릴레이 버퍼 (이벤트 읽기용)
 *   ├── stats          # 통계: total events, dropped
 *   └── status         # 현재 상태: 각 서브 트레이서 등록 여부
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/*
 * ============================================================================
 * 모듈 파라미터
 * ============================================================================
 */

/* 타겟 NVMe 디바이스 (예: "nvme0"). 빈 문자열이면 모든 NVMe 디바이스를 추적한다. */
static char *target_dev = "";
module_param(target_dev, charp, 0444);
MODULE_PARM_DESC(target_dev, "Target NVMe device name (e.g., nvme0). Empty=all");

/* per-CPU 버퍼 크기 (KB) */
static unsigned int buffer_size_kb = 256;
module_param(buffer_size_kb, uint, 0444);
MODULE_PARM_DESC(buffer_size_kb, "Per-CPU trace buffer size in KB (default: 256)");

/* 각 서브 트레이서 활성화 플래그 */
static int enable_sq = 1;
module_param(enable_sq, int, 0444);
MODULE_PARM_DESC(enable_sq, "Enable SQ submission monitoring (default: 1)");

static int enable_cq = 1;
module_param(enable_cq, int, 0444);
MODULE_PARM_DESC(enable_cq, "Enable CQ completion monitoring (default: 1)");

static int enable_doorbell = 1;
module_param(enable_doorbell, int, 0444);
MODULE_PARM_DESC(enable_doorbell, "Enable doorbell write tracing (default: 1)");

static int enable_irq = 1;
module_param(enable_irq, int, 0444);
MODULE_PARM_DESC(enable_irq, "Enable IRQ hooking (default: 1)");

static int enable_mmio = 1;
module_param(enable_mmio, int, 0444);
MODULE_PARM_DESC(enable_mmio, "Enable MMIO register access tracing (default: 1)");

static int enable_dma = 0;
module_param(enable_dma, int, 0444);
MODULE_PARM_DESC(enable_dma, "Enable DMA mapping tracing (default: 0, high overhead)");

/*
 * ============================================================================
 * 서브 트레이서 init/exit 선언
 * ============================================================================
 */

/* nvme_sq_monitor.c */
extern int nvme_sq_monitor_init(void);
extern void nvme_sq_monitor_exit(void);

/* nvme_cq_monitor.c */
extern int nvme_cq_monitor_init(void);
extern void nvme_cq_monitor_exit(void);

/* nvme_doorbell_tracer.c */
extern int nvme_doorbell_tracer_init(void);
extern void nvme_doorbell_tracer_exit(void);

/* nvme_irq_hooker.c */
extern int nvme_irq_hooker_init(void);
extern void nvme_irq_hooker_exit(void);

/* nvme_mmio_tracer.c */
extern int nvme_mmio_tracer_init(void);
extern void nvme_mmio_tracer_exit(void);

/* nvme_dma_tracer.c */
extern int nvme_dma_tracer_init(void);
extern void nvme_dma_tracer_exit(void);

/*
 * ============================================================================
 * 등록 상태 추적
 * ============================================================================
 */
static DEFINE_MUTEX(tracer_mutex);

static bool sq_initialized;
static bool cq_initialized;
static bool doorbell_initialized;
static bool irq_initialized;
static bool mmio_initialized;
static bool dma_initialized;

/*
 * ============================================================================
 * debugfs status 파일
 * ============================================================================
 * 각 서브 트레이서의 등록 상태를 표시한다.
 */

static ssize_t status_read(struct file *file, char __user *buf,
			    size_t count, loff_t *ppos)
{
	char tmp[512];
	int len;

	len = snprintf(tmp, sizeof(tmp),
		"nvme-pcie-tracer status:\n"
		"  target_dev:     %s\n"
		"  buffer_size_kb: %u\n"
		"  tracing:        %s\n"
		"\n"
		"Sub-tracers:\n"
		"  SQ monitor:     %s (enable_sq=%d)\n"
		"  CQ monitor:     %s (enable_cq=%d)\n"
		"  Doorbell:       %s (enable_doorbell=%d)\n"
		"  IRQ hooker:     %s (enable_irq=%d)\n"
		"  MMIO tracer:    %s (enable_mmio=%d)\n"
		"  DMA tracer:     %s (enable_dma=%d)\n",
		target_dev[0] ? target_dev : "(all)",
		buffer_size_kb,
		trace_buffer_is_enabled() ? "ACTIVE" : "STOPPED",
		sq_initialized ? "REGISTERED" : "NOT REGISTERED", enable_sq,
		cq_initialized ? "REGISTERED" : "NOT REGISTERED", enable_cq,
		doorbell_initialized ? "REGISTERED" : "NOT REGISTERED", enable_doorbell,
		irq_initialized ? "REGISTERED" : "NOT REGISTERED", enable_irq,
		mmio_initialized ? "REGISTERED" : "NOT REGISTERED", enable_mmio,
		dma_initialized ? "REGISTERED" : "NOT REGISTERED", enable_dma);

	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct file_operations status_fops = {
	.owner = THIS_MODULE,
	.read  = status_read,
};

/*
 * ============================================================================
 * 모듈 초기화
 * ============================================================================
 * 순서:
 *   1. trace_buffer 초기화 (debugfs + relay 채널)
 *   2. 각 서브 트레이서 초기화 (kprobe 등록)
 *   3. debugfs status 파일 생성
 *
 * 서브 트레이서 초기화 실패는 경고만 출력하고 계속 진행한다.
 * trace_buffer 초기화 실패만 치명적 에러로 처리한다.
 */
static int __init nvme_pcie_tracer_init(void)
{
	int ret;
	struct dentry *debugfs_dir;

	pr_info("nvme-pcie-tracer: initializing (target=%s, buf=%u KB)\n",
		target_dev[0] ? target_dev : "all", buffer_size_kb);

	mutex_lock(&tracer_mutex);

	/* 1. trace_buffer 초기화 */
	ret = trace_buffer_init(buffer_size_kb);
	if (ret) {
		pr_err("nvme-pcie-tracer: trace buffer init failed "
		       "(ret=%d)\n", ret);
		mutex_unlock(&tracer_mutex);
		return ret;
	}

	/* debugfs status 파일 생성 */
	debugfs_dir = trace_buffer_get_debugfs_dir();
	if (debugfs_dir) {
		debugfs_create_file("status", 0444, debugfs_dir,
				    NULL, &status_fops);
	}

	/* 2. Phase 1 서브 트레이서 초기화 */

	if (enable_sq) {
		ret = nvme_sq_monitor_init();
		if (ret) {
			pr_warn("nvme-pcie-tracer: SQ monitor init failed "
				"(ret=%d), continuing without it\n", ret);
		} else {
			sq_initialized = true;
		}
	}

	if (enable_cq) {
		ret = nvme_cq_monitor_init();
		if (ret) {
			pr_warn("nvme-pcie-tracer: CQ monitor init failed "
				"(ret=%d), continuing without it\n", ret);
		} else {
			cq_initialized = true;
		}
	}

	if (enable_doorbell) {
		ret = nvme_doorbell_tracer_init();
		if (ret) {
			pr_warn("nvme-pcie-tracer: Doorbell tracer init "
				"failed (ret=%d), continuing without it\n",
				ret);
		} else {
			doorbell_initialized = true;
		}
	}

	/* 3. Phase 2 서브 트레이서 초기화 */

	if (enable_irq) {
		ret = nvme_irq_hooker_init();
		if (ret) {
			pr_warn("nvme-pcie-tracer: IRQ hooker init failed "
				"(ret=%d), continuing without it\n", ret);
		} else {
			irq_initialized = true;
		}
	}

	if (enable_mmio) {
		ret = nvme_mmio_tracer_init();
		if (ret) {
			pr_warn("nvme-pcie-tracer: MMIO tracer init failed "
				"(ret=%d), continuing without it\n", ret);
		} else {
			mmio_initialized = true;
		}
	}

	if (enable_dma) {
		ret = nvme_dma_tracer_init();
		if (ret) {
			pr_warn("nvme-pcie-tracer: DMA tracer init failed "
				"(ret=%d), continuing without it\n", ret);
		} else {
			dma_initialized = true;
		}
	}

	mutex_unlock(&tracer_mutex);

	pr_info("nvme-pcie-tracer: initialized successfully. "
		"Sub-tracers: SQ=%s CQ=%s DB=%s IRQ=%s MMIO=%s DMA=%s\n",
		sq_initialized ? "OK" : "FAIL",
		cq_initialized ? "OK" : "FAIL",
		doorbell_initialized ? "OK" : "FAIL",
		irq_initialized ? "OK" : "FAIL",
		mmio_initialized ? "OK" : "FAIL",
		dma_initialized ? "OK" : "FAIL");

	pr_info("nvme-pcie-tracer: use 'echo 1 > "
		"/sys/kernel/debug/nvme-pcie-tracer/enabled' to start "
		"tracing\n");

	return 0;
}

/*
 * ============================================================================
 * 모듈 해제
 * ============================================================================
 * 순서:
 *   1. 트레이싱 비활성화
 *   2. 각 서브 트레이서 해제 (kprobe 해제)
 *   3. RCU 동기화 (진행 중인 kprobe handler 완료 대기)
 *   4. trace_buffer 파괴
 */
static void __exit nvme_pcie_tracer_exit(void)
{
	pr_info("nvme-pcie-tracer: shutting down...\n");

	mutex_lock(&tracer_mutex);

	/* 1. 트레이싱 비활성화 (새 이벤트 기록 중지) */
	trace_buffer_set_enabled(false);

	/* 2. 서브 트레이서 해제 (역순) */
	if (dma_initialized) {
		nvme_dma_tracer_exit();
		dma_initialized = false;
	}

	if (mmio_initialized) {
		nvme_mmio_tracer_exit();
		mmio_initialized = false;
	}

	if (irq_initialized) {
		nvme_irq_hooker_exit();
		irq_initialized = false;
	}

	if (doorbell_initialized) {
		nvme_doorbell_tracer_exit();
		doorbell_initialized = false;
	}

	if (cq_initialized) {
		nvme_cq_monitor_exit();
		cq_initialized = false;
	}

	if (sq_initialized) {
		nvme_sq_monitor_exit();
		sq_initialized = false;
	}

	/* 3. 최종 RCU 동기화: 모든 kprobe handler 완료 보장 */
	synchronize_rcu();

	/* 4. trace_buffer 파괴 */
	trace_buffer_destroy();

	mutex_unlock(&tracer_mutex);

	pr_info("nvme-pcie-tracer: unloaded successfully\n");
}

module_init(nvme_pcie_tracer_init);
module_exit(nvme_pcie_tracer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NVMe PCIe Tracer Project");
MODULE_DESCRIPTION("NVMe PCIe-Level Bidirectional Command Tracer");
MODULE_VERSION("0.1");
