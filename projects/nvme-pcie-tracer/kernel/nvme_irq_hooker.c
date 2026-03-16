// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_irq_hooker.c - NVMe MSI-X 인터럽트 캡처
 *
 * Phase 2: kprobe + kretprobe를 사용하여 NVMe MSI-X 인터럽트를 캡처한다.
 *
 * 후킹 대상:
 *   nvme_irq() - NVMe 인터럽트 핸들러 (hardirq context)
 *
 *   static irqreturn_t nvme_irq(int irq, void *data)
 *   x86_64: RDI=irq, RSI=data(struct nvme_queue *)
 *
 *   nvme_queue에서 추출하는 정보:
 *     - qid:       큐 ID (0=Admin, 1~N=I/O)
 *     - cq_vector: MSI-X 벡터 번호
 *
 *   kretprobe로 반환값을 확인하여:
 *     - IRQ_HANDLED: 이 큐에 처리할 CQE가 있었음 (정상)
 *     - IRQ_NONE:    이 큐에 CQE가 없었음 (spurious interrupt)
 *
 * IRQ 통계:
 *   - 총 IRQ 수
 *   - IRQ_HANDLED 수 (유효한 인터럽트)
 *   - IRQ_NONE 수 (spurious 인터럽트)
 *   - CPU별 IRQ 분포
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/interrupt.h>
#include <linux/timekeeping.h>
#include <linux/atomic.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/* IRQ 통계 카운터 */
static atomic64_t irq_total = ATOMIC64_INIT(0);
static atomic64_t irq_handled = ATOMIC64_INIT(0);
static atomic64_t irq_none = ATOMIC64_INIT(0);

/* kretprobe per-instance 데이터 */
struct irq_probe_data {
	u64 timestamp_ns;       /* IRQ 진입 시각 */
	u32 irq_num;            /* IRQ 번호 */
	void *nvmeq;            /* struct nvme_queue * */
};

static struct kretprobe irq_kretprobe;
static bool irq_probe_registered;

/*
 * irq_entry_handler - nvme_irq() 진입 시 호출
 *
 * IRQ 발생 시각, IRQ 번호, nvme_queue 포인터를 저장한다.
 */
static int irq_entry_handler(struct kretprobe_instance *ri,
			     struct pt_regs *regs)
{
	struct irq_probe_data *data = (struct irq_probe_data *)ri->data;
	struct nvme_trace_event evt;

	data->timestamp_ns = ktime_get_ns();
	data->irq_num = (u32)regs->di;
	data->nvmeq = (void *)regs->si;

	atomic64_inc(&irq_total);

	/* IRQ 진입 이벤트 기록 */
	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = data->timestamp_ns;
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_IRQ_FIRED;
	evt.direction = DIR_DEVICE_TO_HOST;
	evt.irq.irq_num = data->irq_num;

	/*
	 * nvme_queue에서 qid와 cq_vector를 읽으려면 구조체 내부 접근이 필요하다.
	 * 안전한 접근을 위해 기본 IRQ 정보만 기록한다.
	 */

	trace_buffer_write(&evt);
	return 0;
}

/*
 * irq_ret_handler - nvme_irq() 반환 시 호출
 *
 * 반환값으로 IRQ_HANDLED/IRQ_NONE을 구분하여 통계를 업데이트한다.
 * IRQ_NONE이면 spurious interrupt로 카운트한다.
 */
static int irq_ret_handler(struct kretprobe_instance *ri,
			   struct pt_regs *regs)
{
	struct irq_probe_data *data = (struct irq_probe_data *)ri->data;
	unsigned long ret_val;
	u64 duration_ns;

	ret_val = regs_return_value(regs);
	duration_ns = ktime_get_ns() - data->timestamp_ns;

	if (ret_val == IRQ_HANDLED) {
		atomic64_inc(&irq_handled);
	} else {
		atomic64_inc(&irq_none);
	}

	/*
	 * IRQ 처리 시간이 비정상적으로 긴 경우 경고.
	 * 일반적으로 NVMe IRQ handler는 수 마이크로초 내에 완료되어야 한다.
	 * 100us 이상이면 문제가 있을 수 있다.
	 */
	if (duration_ns > 100000) { /* 100us */
		pr_debug("nvme-pcie-tracer: long IRQ handler: irq=%u "
			 "duration=%llu ns, result=%s\n",
			 data->irq_num, duration_ns,
			 ret_val == IRQ_HANDLED ? "HANDLED" : "NONE");
	}

	return 0;
}

/*
 * ============================================================================
 * nvme_irq_check() kprobe - 스레드 인터럽트 모드 지원
 * ============================================================================
 * use_threaded_interrupts=1일 때 사용되는 hardirq 핸들러.
 * CQE 유무만 확인하고 스레드를 깨운다.
 *
 * static irqreturn_t nvme_irq_check(int irq, void *data)
 */
static struct kprobe irq_check_kprobe;
static bool irq_check_probe_registered;

static int irq_check_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct nvme_trace_event evt;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns();
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_IRQ_FIRED;
	evt.direction = DIR_DEVICE_TO_HOST;
	evt.irq.irq_num = (u32)regs->di;
	evt.flags = 0x01; /* threaded IRQ check 표시 */

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 초기화/해제
 * ============================================================================
 */

int nvme_irq_hooker_init(void)
{
	int ret;

	/* nvme_irq kretprobe */
	memset(&irq_kretprobe, 0, sizeof(irq_kretprobe));
	irq_kretprobe.kp.symbol_name = "nvme_irq";
	irq_kretprobe.handler = irq_ret_handler;
	irq_kretprobe.entry_handler = irq_entry_handler;
	irq_kretprobe.data_size = sizeof(struct irq_probe_data);
	irq_kretprobe.maxactive = 128; /* 동시 IRQ 처리 가능 수 */

	ret = register_kretprobe(&irq_kretprobe);
	if (ret < 0) {
		pr_err("nvme-pcie-tracer: failed to register kretprobe on "
		       "nvme_irq (ret=%d). IRQ hooking unavailable.\n", ret);
		irq_probe_registered = false;
		return ret;
	}
	irq_probe_registered = true;
	pr_info("nvme-pcie-tracer: IRQ hooker registered on nvme_irq\n");

	/* nvme_irq_check kprobe (optional) */
	memset(&irq_check_kprobe, 0, sizeof(irq_check_kprobe));
	irq_check_kprobe.symbol_name = "nvme_irq_check";
	irq_check_kprobe.pre_handler = irq_check_pre_handler;

	ret = register_kprobe(&irq_check_kprobe);
	if (ret < 0) {
		pr_info("nvme-pcie-tracer: nvme_irq_check kprobe not "
			"available (ret=%d). Threaded IRQ check tracing "
			"disabled.\n", ret);
		irq_check_probe_registered = false;
		/* 치명적이지 않음: 스레드 인터럽트 미사용 시 정상 */
	} else {
		irq_check_probe_registered = true;
		pr_info("nvme-pcie-tracer: IRQ check hooker registered on "
			"nvme_irq_check\n");
	}

	return 0;
}

void nvme_irq_hooker_exit(void)
{
	if (irq_probe_registered) {
		unregister_kretprobe(&irq_kretprobe);
		irq_probe_registered = false;
		if (irq_kretprobe.nmissed)
			pr_info("nvme-pcie-tracer: IRQ kretprobe missed "
				"%d probes\n", irq_kretprobe.nmissed);
	}

	if (irq_check_probe_registered) {
		unregister_kprobe(&irq_check_kprobe);
		irq_check_probe_registered = false;
	}

	synchronize_rcu();

	pr_info("nvme-pcie-tracer: IRQ hooker unregistered "
		"(total=%lld, handled=%lld, none=%lld)\n",
		(long long)atomic64_read(&irq_total),
		(long long)atomic64_read(&irq_handled),
		(long long)atomic64_read(&irq_none));
}
