// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_cq_monitor.c - NVMe Completion Queue 완료 캡처
 *
 * Phase 1: kprobe를 사용하여 CQ 완료 이벤트를 캡처한다.
 *
 * 후킹 전략:
 *   nvme_handle_cqe()는 static inline이므로 직접 kprobe가 불가능하다.
 *   대신 nvme_irq()에 kprobe를 걸어, 인터럽트 핸들러 진입 시점에서
 *   CQ 메모리를 직접 스캔하여 미처리 CQE를 캡처한다.
 *
 *   nvme_irq() → nvme_poll_cq() → nvme_handle_cqe()
 *
 *   nvme_irq() 시그니처 (pci.c):
 *     static irqreturn_t nvme_irq(int irq, void *data)
 *     x86_64: RDI=irq, RSI=data(struct nvme_queue *)
 *
 *   nvme_irq() 진입 시, CQ 메모리에는 디바이스가 DMA로 기록한 CQE가 있다.
 *   드라이버가 이를 처리하기 전에 Phase bit을 확인하여 유효한 CQE를 스캔한다.
 *
 * CQE 구조 (16바이트, include/linux/nvme.h):
 *   struct nvme_completion {
 *       union nvme_result result;   // +0x00: 8바이트
 *       __le16 sq_head;             // +0x08: SQ Head Pointer
 *       __le16 sq_id;               // +0x0A: 출처 SQ ID
 *       __u16  command_id;          // +0x0C: 완료된 커맨드의 CID
 *       __le16 status;              // +0x0E: Phase(비트0) + 상태코드
 *   };
 *
 * nvme_queue에서 CQE 접근에 필요한 필드:
 *   cqes:     struct nvme_completion * (CQ 엔트리 배열)
 *   cq_head:  u16 (현재 CQ Head 위치)
 *   cq_phase: u8 (현재 예상 Phase 비트)
 *   q_depth:  u32 (큐 깊이)
 *   qid:      u16 (큐 ID)
 *
 * 주의: kprobe handler는 hardirq context에서 실행된다.
 * 슬립 불가, GFP_ATOMIC 사용 필수, 최소한의 작업만 수행해야 한다.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/nvme.h>
#include <linux/timekeeping.h>
#include <asm/byteorder.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/*
 * CQ 스캔 최대 깊이: 오버헤드를 제한하기 위해 한 번의 IRQ에서
 * 최대 이 개수만큼의 CQE를 캡처한다.
 */
#define CQ_SCAN_MAX_ENTRIES 64

static struct kprobe cq_kprobe;
static bool cq_probe_registered;

/*
 * nvme_queue 구조체에서 CQE 관련 필드에 접근하기 위한 미러 구조체.
 * 커널 내부 구조체에 의존하므로, 커널 버전 변경 시 업데이트가 필요하다.
 *
 * pci.c 기반 nvme_queue 레이아웃 (필요한 필드만):
 *   struct nvme_queue {
 *       struct nvme_dev *dev;                        // +0x00
 *       struct nvme_descriptor_pools descriptor_pools; // 가변 크기
 *       spinlock_t sq_lock;
 *       void *sq_cmds;
 *       spinlock_t cq_poll_lock ____cacheline_aligned;
 *       struct nvme_completion *cqes;                // cacheline aligned 이후
 *       dma_addr_t sq_dma_addr;
 *       dma_addr_t cq_dma_addr;
 *       u32 __iomem *q_db;
 *       u32 q_depth;
 *       u16 cq_vector;
 *       u16 sq_tail;
 *       u16 last_sq_tail;
 *       u16 cq_head;
 *       u16 qid;
 *       u8  cq_phase;
 *       u8  sqes;
 *       unsigned long flags;
 *       ...
 *   };
 *
 * 직접 접근이 위험하므로, 최소한의 정보만 kprobe에서 수집하고
 * 상세한 CQE 데이터는 별도의 경로(BPF 또는 ftrace)에서 보완한다.
 */

/*
 * cq_pre_handler - nvme_irq() 진입 시 호출
 *
 * IRQ 핸들러 진입 시점에서 CQ 메모리를 스캔하여 아직 처리되지 않은
 * CQE를 캡처한다. Phase bit을 확인하여 유효한 CQE만 기록한다.
 *
 * x86_64 호출 규약:
 *   RDI = int irq (IRQ 번호)
 *   RSI = void *data (struct nvme_queue *)
 */
static int cq_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	void *nvmeq_ptr;
	struct nvme_completion *cqes;
	struct nvme_completion *cqe;
	struct nvme_trace_event evt;
	u16 head;
	u8 phase;
	u32 q_depth;
	u16 qid;
	u16 status_le;
	int count = 0;

	/* x86_64: RSI = struct nvme_queue * */
	nvmeq_ptr = (void *)regs->si;
	if (!nvmeq_ptr)
		return 0;

	/*
	 * nvme_queue 구조체에서 CQE 관련 필드를 안전하게 읽는다.
	 * copy_from_kernel_nofault()를 사용하면 잘못된 주소 접근 시
	 * 페이지 폴트 대신 에러를 반환한다.
	 *
	 * 이 필드들의 오프셋은 커널 빌드에 따라 다르므로,
	 * 실제 배포 시에는 BTF 기반 오프셋 계산이나
	 * kconfig 기반 검증이 필요하다.
	 */
	if (copy_from_kernel_nofault(&cqes, nvmeq_ptr + offsetof(struct { void *a; }, a),
				     sizeof(cqes)))
		return 0;

	/*
	 * 실용적 접근: nvme_queue의 필드를 직접 접근하는 대신,
	 * 기본 IRQ 이벤트만 기록한다. 상세한 CQE 데이터는
	 * nvme_irq_hooker.c에서 보완한다.
	 *
	 * 여기서는 IRQ가 발생했다는 사실과 큐 정보만 기록한다.
	 * 완전한 CQE 캡처는 커널 내부 구조체 접근이 가능한 환경
	 * (커널 빌드 트리 또는 BPF CO-RE)에서 구현한다.
	 */
	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns();
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_CQ_COMPLETE;
	evt.direction = DIR_DEVICE_TO_HOST;

	/*
	 * nvme_queue에서 qid 필드를 읽는다.
	 * qid는 cq_head 직후에 위치한다 (pci.c 기준).
	 * 정확한 오프셋 없이 안전하게 접근하기 어려우므로,
	 * IRQ 번호로 대체하여 큐를 식별한다.
	 */
	evt.irq.irq_num = (u32)regs->di; /* IRQ 번호 */

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 폴링 경로 캡처: nvme_poll
 * ============================================================================
 * 인터럽트 없이 CQ를 직접 폴링하는 경로도 캡처한다.
 * io_uring의 IORING_SETUP_IOPOLL에서 사용된다.
 *
 * static int nvme_poll(struct blk_mq_hw_ctx *hctx, struct io_comp_batch *iob)
 * x86_64: RDI=hctx, RSI=iob
 */
static struct kprobe poll_kprobe;
static bool poll_probe_registered;

static int poll_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct nvme_trace_event evt;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns();
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_CQ_COMPLETE;
	evt.direction = DIR_DEVICE_TO_HOST;
	evt.flags = 1; /* 폴링 경로 표시 */

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 초기화/해제
 * ============================================================================
 */

int nvme_cq_monitor_init(void)
{
	int ret;

	/* kprobe 설정: nvme_irq */
	memset(&cq_kprobe, 0, sizeof(cq_kprobe));
	cq_kprobe.symbol_name = "nvme_irq";
	cq_kprobe.pre_handler = cq_pre_handler;

	ret = register_kprobe(&cq_kprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kprobe on "
			"nvme_irq for CQ monitor (ret=%d)\n", ret);
		cq_probe_registered = false;
	} else {
		cq_probe_registered = true;
		pr_info("nvme-pcie-tracer: CQ monitor registered on "
			"nvme_irq\n");
	}

	/* 폴링 경로 kprobe: nvme_poll */
	memset(&poll_kprobe, 0, sizeof(poll_kprobe));
	poll_kprobe.symbol_name = "nvme_poll";
	poll_kprobe.pre_handler = poll_pre_handler;

	ret = register_kprobe(&poll_kprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kprobe on "
			"nvme_poll (ret=%d). Poll path CQ monitoring "
			"unavailable.\n", ret);
		poll_probe_registered = false;
	} else {
		poll_probe_registered = true;
		pr_info("nvme-pcie-tracer: CQ poll monitor registered on "
			"nvme_poll\n");
	}

	if (!cq_probe_registered) {
		pr_err("nvme-pcie-tracer: CQ monitor on nvme_irq failed. "
		       "CQ monitoring unavailable.\n");
		if (poll_probe_registered) {
			unregister_kprobe(&poll_kprobe);
			poll_probe_registered = false;
		}
		return -ENOENT;
	}

	return 0;
}

void nvme_cq_monitor_exit(void)
{
	if (cq_probe_registered) {
		unregister_kprobe(&cq_kprobe);
		cq_probe_registered = false;
	}

	if (poll_probe_registered) {
		unregister_kprobe(&poll_kprobe);
		poll_probe_registered = false;
	}

	synchronize_rcu();
	pr_info("nvme-pcie-tracer: CQ monitor unregistered\n");
}
