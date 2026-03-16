// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_doorbell_tracer.c - NVMe Doorbell 쓰기 캡처
 *
 * Phase 1: kprobe를 사용하여 SQ Tail Doorbell과 CQ Head Doorbell 쓰기를 캡처한다.
 *
 * 후킹 전략:
 *   nvme_write_sq_db()는 static inline이므로 직접 kprobe가 불가능하다.
 *   nvme_ring_cq_doorbell()도 static inline이다.
 *
 *   SQ Tail Doorbell:
 *     nvme_write_sq_db()는 nvme_queue_rq()와 nvme_commit_rqs()에서 호출된다.
 *     nvme_commit_rqs()는 blk-mq 콜백(blk_mq_ops.commit_rqs)으로 non-inline이다.
 *     시그니처: static void nvme_commit_rqs(struct blk_mq_hw_ctx *hctx)
 *     x86_64: RDI=hctx
 *
 *   CQ Head Doorbell:
 *     nvme_ring_cq_doorbell()는 nvme_poll_cq()에서 호출된다.
 *     nvme_poll_cq()도 static inline이므로 직접 kprobe 불가.
 *     nvme_irq()에 kprobe를 걸어 CQ Doorbell 시점을 간접 추적한다.
 *     nvme_irq() kretprobe를 사용하면, IRQ 반환 시점에서
 *     CQ Head Doorbell이 이미 쓰여진 후이다.
 *
 *   대안: writel() 자체에 kprobe를 건다면 모든 Doorbell 쓰기를 캡처할 수 있지만,
 *   writel은 인라인 매크로/아키텍처 함수이므로 불가능하다.
 *
 * 이 모듈에서는:
 *   1. nvme_commit_rqs()에 kprobe → SQ Tail Doorbell 강제 쓰기 캡처
 *   2. nvme_irq()에 kretprobe → CQ Head Doorbell 쓰기 간접 캡처
 *      (IRQ 반환 시 IRQ_HANDLED이면 CQ Doorbell이 쓰여졌다는 의미)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/blk-mq.h>
#include <linux/timekeeping.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/*
 * ============================================================================
 * SQ Tail Doorbell: nvme_commit_rqs() kprobe
 * ============================================================================
 * nvme_commit_rqs()는 blk-mq가 배치 제출 후 Doorbell을 강제 쓰기할 때 호출된다.
 * 이 함수 내에서 nvme_write_sq_db(nvmeq, true)가 호출된다.
 *
 * static void nvme_commit_rqs(struct blk_mq_hw_ctx *hctx)
 * x86_64: RDI = struct blk_mq_hw_ctx *hctx
 *         hctx->driver_data = struct nvme_queue *
 */

static struct kprobe commit_rqs_kprobe;
static bool commit_rqs_probe_registered;

static int commit_rqs_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct nvme_trace_event evt;
	struct blk_mq_hw_ctx *hctx;

	hctx = (struct blk_mq_hw_ctx *)regs->di;
	if (!hctx || !hctx->driver_data)
		return 0;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns();
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_DOORBELL_SQ_TAIL;
	evt.direction = DIR_HOST_TO_DEVICE;
	evt.size = 4; /* Doorbell 레지스터는 4바이트 쓰기 */

	/*
	 * nvme_queue에서 qid와 sq_tail을 읽어야 하지만,
	 * 커널 내부 구조체 접근 제약으로 기본 정보만 기록한다.
	 *
	 * 향후: nvme_queue 오프셋을 모듈 파라미터나 BTF로 설정하여
	 * 정확한 값을 기록할 수 있다.
	 */

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * CQ Head Doorbell: nvme_irq() kretprobe
 * ============================================================================
 * nvme_irq() 반환값이 IRQ_HANDLED이면, nvme_poll_cq()가 CQE를 처리하고
 * nvme_ring_cq_doorbell()로 CQ Head Doorbell을 썼다는 의미이다.
 *
 * static irqreturn_t nvme_irq(int irq, void *data)
 * x86_64: RDI=irq, RSI=data(struct nvme_queue *)
 * 반환값(RAX): IRQ_HANDLED=1, IRQ_NONE=0
 */

/* kretprobe per-instance 데이터 */
struct doorbell_cq_data {
	u64 timestamp_ns;
	u32 irq_num;
};

static struct kretprobe cq_doorbell_kretprobe;
static bool cq_doorbell_probe_registered;

static int cq_doorbell_entry_handler(struct kretprobe_instance *ri,
				     struct pt_regs *regs)
{
	struct doorbell_cq_data *data = (struct doorbell_cq_data *)ri->data;

	data->timestamp_ns = ktime_get_ns();
	data->irq_num = (u32)regs->di;
	return 0;
}

static int cq_doorbell_ret_handler(struct kretprobe_instance *ri,
				   struct pt_regs *regs)
{
	struct doorbell_cq_data *data = (struct doorbell_cq_data *)ri->data;
	struct nvme_trace_event evt;
	unsigned long ret_val;

	ret_val = regs_return_value(regs);

	/* IRQ_HANDLED(1)이면 CQE가 처리되었고, CQ Head Doorbell이 쓰여졌다 */
	if (ret_val != IRQ_HANDLED)
		return 0;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns(); /* Doorbell 쓰기 완료 후 시점 */
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_DOORBELL_CQ_HEAD;
	evt.direction = DIR_HOST_TO_DEVICE;
	evt.size = 4; /* Doorbell 레지스터는 4바이트 쓰기 */
	evt.irq.irq_num = data->irq_num;

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 초기화/해제
 * ============================================================================
 */

int nvme_doorbell_tracer_init(void)
{
	int ret;

	/* SQ Tail Doorbell: nvme_commit_rqs kprobe */
	memset(&commit_rqs_kprobe, 0, sizeof(commit_rqs_kprobe));
	commit_rqs_kprobe.symbol_name = "nvme_commit_rqs";
	commit_rqs_kprobe.pre_handler = commit_rqs_pre_handler;

	ret = register_kprobe(&commit_rqs_kprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kprobe on "
			"nvme_commit_rqs (ret=%d). SQ Tail Doorbell "
			"tracing unavailable.\n", ret);
		commit_rqs_probe_registered = false;
	} else {
		commit_rqs_probe_registered = true;
		pr_info("nvme-pcie-tracer: Doorbell tracer registered on "
			"nvme_commit_rqs (SQ Tail)\n");
	}

	/* CQ Head Doorbell: nvme_irq kretprobe */
	memset(&cq_doorbell_kretprobe, 0, sizeof(cq_doorbell_kretprobe));
	cq_doorbell_kretprobe.kp.symbol_name = "nvme_irq";
	cq_doorbell_kretprobe.handler = cq_doorbell_ret_handler;
	cq_doorbell_kretprobe.entry_handler = cq_doorbell_entry_handler;
	cq_doorbell_kretprobe.data_size = sizeof(struct doorbell_cq_data);
	cq_doorbell_kretprobe.maxactive = 64;

	ret = register_kretprobe(&cq_doorbell_kretprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kretprobe on "
			"nvme_irq for CQ Head Doorbell (ret=%d)\n", ret);
		cq_doorbell_probe_registered = false;
	} else {
		cq_doorbell_probe_registered = true;
		pr_info("nvme-pcie-tracer: Doorbell tracer registered on "
			"nvme_irq kretprobe (CQ Head)\n");
	}

	if (!commit_rqs_probe_registered && !cq_doorbell_probe_registered) {
		pr_err("nvme-pcie-tracer: no Doorbell probes could be "
		       "registered\n");
		return -ENOENT;
	}

	return 0;
}

void nvme_doorbell_tracer_exit(void)
{
	if (commit_rqs_probe_registered) {
		unregister_kprobe(&commit_rqs_kprobe);
		commit_rqs_probe_registered = false;
	}

	if (cq_doorbell_probe_registered) {
		unregister_kretprobe(&cq_doorbell_kretprobe);
		cq_doorbell_probe_registered = false;
		if (cq_doorbell_kretprobe.nmissed)
			pr_info("nvme-pcie-tracer: CQ Doorbell kretprobe "
				"missed %d probes\n",
				cq_doorbell_kretprobe.nmissed);
	}

	synchronize_rcu();
	pr_info("nvme-pcie-tracer: Doorbell tracer unregistered\n");
}
