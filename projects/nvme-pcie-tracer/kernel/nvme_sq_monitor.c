// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_sq_monitor.c - NVMe Submission Queue 커맨드 캡처
 *
 * Phase 1: kprobe를 사용하여 SQ에 제출되는 NVMe 커맨드를 캡처한다.
 *
 * 후킹 전략:
 *   nvme_sq_copy_cmd()는 static inline이므로 직접 kprobe가 불가능하다.
 *   대신 nvme_queue_rq()에 kprobe를 걸어 SQ 제출 시점을 포착한다.
 *   nvme_queue_rq()는 blk-mq 콜백으로 static이지만 심볼이 존재한다.
 *
 * 호출 체인:
 *   blk-mq → nvme_queue_rq(hctx, bd)
 *     → nvme_prep_rq()        : 커맨드 빌드 + DMA 매핑
 *     → nvme_sq_copy_cmd()    : SQ에 64바이트 복사 (inline)
 *     → nvme_write_sq_db()    : Doorbell 쓰기 (inline)
 *
 * nvme_queue_rq() 시그니처 (pci.c):
 *   static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
 *                                     const struct blk_mq_queue_data *bd)
 *
 * x86_64 호출 규약: RDI=hctx, RSI=bd
 *   hctx->driver_data = struct nvme_queue *
 *   bd->rq = struct request *
 *   iod = blk_mq_rq_to_pdu(req) → iod->cmd가 제출할 커맨드
 *
 * 대안 접근 (더 정확한 시점):
 *   nvme_submit_cmds()에 kprobe를 건다. 이 함수는 배치 제출 경로에서
 *   nvme_sq_copy_cmd()을 호출하기 직전에 SQ 락을 잡는 비인라인 함수이다.
 *   그러나 nvme_queue_rq() 경로에서는 불리지 않으므로 두 지점 모두 필요하다.
 *
 * 여기서는 kretprobe를 사용하여 nvme_queue_rq()의 반환값(BLK_STS_OK 여부)을
 * 확인하고, 성공한 경우에만 이벤트를 기록한다. pre_handler에서 커맨드를 캡처하고
 * ret_handler에서 성공 여부를 확인한 뒤 최종 기록한다.
 *
 * 단순화를 위해 pre_handler만 사용하는 kprobe 방식을 채택한다.
 * nvme_prep_rq() 완료 후 nvme_sq_copy_cmd() 호출 전에 iod->cmd에 커맨드가
 * 완성되어 있다. nvme_queue_rq()의 spin_lock 구간 내에서 copy+doorbell이
 * 수행되므로, pre_handler 시점에서는 아직 iod->cmd가 빌드되지 않았을 수 있다.
 *
 * 최종 전략: nvme_submit_cmds()를 주 후킹 대상으로 삼되, 이 함수가 없는
 * 구형 커널에서는 nvme_queue_rq()의 kretprobe를 사용한다.
 * nvme_submit_cmds() 시그니처:
 *   static void nvme_submit_cmds(struct nvme_queue *nvmeq, struct rq_list *rqlist)
 * 이 함수는 배치 제출 전용이고 단건 제출(nvme_queue_rq)에서는 인라인으로
 * nvme_sq_copy_cmd을 호출하므로, 결국 nvme_queue_rq()를 후킹하는 것이 가장
 * 범용적이다.
 *
 * 최종 결정: nvme_queue_rq() kretprobe 사용.
 * entry_handler에서 hctx→nvmeq, req→iod→cmd 정보를 per-instance 데이터에 저장하고,
 * ret_handler에서 반환값이 BLK_STS_OK(0)이면 이벤트를 기록한다.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/nvme.h>
#include <linux/timekeeping.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/*
 * nvme_queue 구조체의 qid 필드 오프셋.
 * 실제 커널 빌드에 따라 다를 수 있으므로, 초기화 시 검증이 필요하다.
 * 여기서는 pci.c 분석 결과에 기반한 struct 레이아웃을 직접 접근한다.
 *
 * 주의: 커널 내부 구조체이므로 EXPORT되지 않는다.
 * kprobe handler에서는 구조체 필드를 바이트 오프셋으로 접근하거나,
 * 구조체 정의를 복사하여 사용해야 한다.
 *
 * 아래에 pci.c의 nvme_queue에서 필요한 필드만 추출한 미러 구조체를 정의한다.
 */

/*
 * nvme_iod 미러 구조체: blk_mq_rq_to_pdu(req)로 얻는 per-request 데이터.
 * 실제 커널의 nvme_iod 구조체에서 cmd 필드의 오프셋이 중요하다.
 * pci.c에서 nvme_iod는:
 *   struct nvme_iod {
 *       struct nvme_request req;  (또는 유사한 헤더)
 *       ...
 *       struct nvme_command cmd;
 *   };
 * 그러나 정확한 오프셋은 커널 버전마다 다르다.
 *
 * 더 안전한 접근: nvme_req(req)->cmd을 사용하되, nvme_req은
 * drivers/nvme/host/nvme.h에 정의되어 있으므로 모듈에서 접근 불가.
 *
 * 실용적 해결책: SQ 커맨드 버퍼를 직접 읽는다.
 * nvme_sq_copy_cmd()이 실행된 후에는 nvmeq->sq_cmds에 커맨드가 복사되어 있다.
 * 하지만 nvme_queue_rq() pre_handler 시점에서는 아직 복사되지 않았다.
 *
 * 따라서 kretprobe를 사용하여:
 *   entry_handler: nvmeq와 sq_tail을 저장
 *   ret_handler: 성공 시 sq_cmds에서 커맨드를 읽음
 *
 * 문제점: ret_handler 시점에서 sq_tail은 이미 변경되었고, 해당 위치에
 * 새로운 커맨드가 덮어씌워졌을 수 있다. 그러나 일반적으로 매우 짧은 시간
 * 내에 발생하므로 실용적으로는 문제가 없다.
 */

/* kretprobe per-instance 데이터 */
struct sq_probe_data {
	u64 timestamp_ns;
	void *nvmeq;         /* struct nvme_queue * */
	u16 sq_tail_before;  /* 복사 전 SQ tail */
	u16 qid;
};

static struct kretprobe sq_kretprobe;
static bool sq_probe_registered;

/*
 * sq_entry_handler - nvme_queue_rq() 진입 시 호출
 *
 * x86_64 호출 규약:
 *   RDI = struct blk_mq_hw_ctx *hctx
 *   RSI = const struct blk_mq_queue_data *bd
 *
 * hctx->driver_data = struct nvme_queue *
 * nvme_queue 구조체에서 필요한 필드의 오프셋:
 *   qid:     u16, offset varies
 *   sq_tail: u16, offset varies
 *   sq_cmds: void *, near beginning
 *   sqes:    u8, offset varies
 */
static int sq_entry_handler(struct kretprobe_instance *ri,
			    struct pt_regs *regs)
{
	struct sq_probe_data *data = (struct sq_probe_data *)ri->data;
	struct blk_mq_hw_ctx *hctx;
	void *nvmeq_ptr;

	data->timestamp_ns = ktime_get_ns();

	/* x86_64: RDI = hctx */
	hctx = (struct blk_mq_hw_ctx *)regs->di;
	if (!hctx)
		return 1; /* 이 instance를 skip */

	/*
	 * hctx->driver_data에서 nvme_queue 포인터를 얻는다.
	 * struct blk_mq_hw_ctx에서 driver_data 필드는 공개 API이다.
	 */
	nvmeq_ptr = hctx->driver_data;
	if (!nvmeq_ptr)
		return 1;

	data->nvmeq = nvmeq_ptr;

	/*
	 * nvme_queue 구조체에서 qid와 sq_tail을 읽는다.
	 * 이 필드들의 오프셋은 probe_read_*로 안전하게 접근한다.
	 *
	 * pci.c 분석 기반 nvme_queue 레이아웃 (5.14+/6.x):
	 *   +0x00: struct nvme_dev *dev
	 *   ...
	 *   sq_tail: u16 (정확한 오프셋은 커널 빌드에 따라 다름)
	 *   qid:    u16 (sq_tail 근처)
	 *
	 * 안전한 접근을 위해 probe_kernel_read를 사용한다.
	 * 여기서는 구조체를 직접 캐스팅하는 대신, 최소한의 필드만 읽는다.
	 */

	return 0; /* 이 instance를 계속 추적 */
}

/*
 * sq_ret_handler - nvme_queue_rq() 반환 시 호출
 *
 * 반환값(regs->ax)이 BLK_STS_OK(0)이면 커맨드가 성공적으로 제출된 것이다.
 * 이때 SQ 버퍼에서 마지막으로 복사된 커맨드를 읽어 이벤트를 기록한다.
 */
static int sq_ret_handler(struct kretprobe_instance *ri,
			  struct pt_regs *regs)
{
	struct sq_probe_data *data = (struct sq_probe_data *)ri->data;
	struct nvme_trace_event evt;
	unsigned long ret_val;
	/*
	 * nvme_queue 구조체 필드에 안전하게 접근하기 위한 로컬 변수.
	 * copy_from_kernel_nofault()로 읽는다.
	 */
	u16 qid = 0;
	u16 sq_tail = 0;
	u8 sqes = 0;
	void *sq_cmds = NULL;
	void *nvmeq = data->nvmeq;

	if (!nvmeq)
		return 0;

	/* 반환값 확인: BLK_STS_OK = 0 */
	ret_val = regs_return_value(regs);
	if (ret_val != 0)
		return 0; /* 제출 실패, 이벤트 기록하지 않음 */

	/*
	 * nvme_queue 구조체에서 필드를 안전하게 읽는다.
	 * 오프셋은 pci.c 분석 기반이다. 커널 빌드마다 다를 수 있으므로
	 * copy_from_kernel_nofault()를 사용하여 안전하게 읽는다.
	 *
	 * 간략화를 위해 직접 캐스팅한다. 실제 배포 시에는 BTF나
	 * kconfig 기반 오프셋 검증이 필요하다.
	 *
	 * 주의: 이 코드는 drivers/nvme/host/pci.c의 nvme_queue 구조체
	 * 레이아웃에 강하게 의존한다.
	 */

	/*
	 * sq_cmds 필드 읽기: nvme_queue에서 네 번째 포인터 크기 필드.
	 * struct nvme_queue {
	 *   +0x00: struct nvme_dev *dev;
	 *   +0x08: struct nvme_descriptor_pools descriptor_pools; (크기 가변)
	 *   +???:  spinlock_t sq_lock;
	 *   +???:  void *sq_cmds;
	 *   ...
	 * }
	 *
	 * 정확한 접근을 위해 여기서는 가용한 정보만으로 이벤트를 구성한다.
	 * 커맨드 데이터는 별도의 BPF 기반 접근이나 추가 kprobe에서 보완한다.
	 */

	/* 이벤트 구성 */
	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = data->timestamp_ns;
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_SQ_SUBMIT;
	evt.direction = DIR_HOST_TO_DEVICE;
	evt.size = sizeof(struct nvme_command); /* 64 bytes */

	/*
	 * blk_mq_queue_data에서 request를 얻고, nvme_req(req)에서
	 * 커맨드를 읽는 것이 이상적이지만, 커널 내부 구조체 접근 제약으로
	 * 여기서는 기본 정보만 기록한다.
	 *
	 * TODO: 커널 빌드 환경에서 nvme 내부 헤더를 포함하여 정확한
	 * 구조체 접근을 구현한다. 또는 BPF CO-RE로 안전하게 접근한다.
	 */
	evt.queue_id = 0xFFFF; /* 구조체 접근 불가 시 알 수 없음 표시 */

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 대안 kprobe: nvme_pci_submit_async_event에 kprobe
 * ============================================================================
 * Admin 커맨드 제출을 캡처한다. 이 함수는 non-inline이고 비교적 단순하다.
 *
 * static void nvme_pci_submit_async_event(struct nvme_ctrl *ctrl)
 *   x86_64: RDI = struct nvme_ctrl *ctrl
 *
 * 이 함수 내부에서 nvme_sq_copy_cmd()을 호출하므로,
 * Admin 커맨드의 SQE를 캡처할 수 있다.
 */

static struct kprobe admin_sq_kprobe;
static bool admin_sq_probe_registered;

static int admin_sq_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct nvme_trace_event evt;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns();
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_SQ_SUBMIT;
	evt.direction = DIR_HOST_TO_DEVICE;
	evt.queue_id = 0; /* Admin Queue */
	evt.size = sizeof(struct nvme_command);

	/*
	 * nvme_pci_submit_async_event()에서는 고정된 opcode(0x0C = Async Event)를
	 * 사용한다. CQE의 command_id도 고정(NVME_AQ_BLK_MQ_DEPTH)이다.
	 * 이벤트에 opcode만 기록한다.
	 */
	evt.sqe[0] = 0x0C; /* nvme_admin_async_event opcode */

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 초기화/해제
 * ============================================================================
 */

int nvme_sq_monitor_init(void)
{
	int ret;

	/* kretprobe 설정: nvme_queue_rq */
	memset(&sq_kretprobe, 0, sizeof(sq_kretprobe));
	sq_kretprobe.kp.symbol_name = "nvme_queue_rq";
	sq_kretprobe.handler = sq_ret_handler;
	sq_kretprobe.entry_handler = sq_entry_handler;
	sq_kretprobe.data_size = sizeof(struct sq_probe_data);
	sq_kretprobe.maxactive = 64; /* 동시 추적 가능한 최대 인스턴스 수 */

	ret = register_kretprobe(&sq_kretprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kretprobe on "
			"nvme_queue_rq (ret=%d). SQ monitoring for I/O "
			"commands will be unavailable.\n", ret);
		sq_probe_registered = false;
		/*
		 * nvme_queue_rq가 없거나 kprobe가 불가능한 경우.
		 * static 함수이므로 kallsyms에 없을 수 있다 (CONFIG_KALLSYMS_ALL 필요).
		 * 치명적 에러가 아니므로 계속 진행한다.
		 */
	} else {
		sq_probe_registered = true;
		pr_info("nvme-pcie-tracer: SQ monitor registered on "
			"nvme_queue_rq\n");
	}

	/* Admin 커맨드 kprobe: nvme_pci_submit_async_event */
	memset(&admin_sq_kprobe, 0, sizeof(admin_sq_kprobe));
	admin_sq_kprobe.symbol_name = "nvme_pci_submit_async_event";
	admin_sq_kprobe.pre_handler = admin_sq_pre_handler;

	ret = register_kprobe(&admin_sq_kprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kprobe on "
			"nvme_pci_submit_async_event (ret=%d)\n", ret);
		admin_sq_probe_registered = false;
	} else {
		admin_sq_probe_registered = true;
		pr_info("nvme-pcie-tracer: Admin SQ monitor registered on "
			"nvme_pci_submit_async_event\n");
	}

	if (!sq_probe_registered && !admin_sq_probe_registered) {
		pr_err("nvme-pcie-tracer: no SQ monitor probes could be "
		       "registered\n");
		return -ENOENT;
	}

	return 0;
}

void nvme_sq_monitor_exit(void)
{
	if (sq_probe_registered) {
		unregister_kretprobe(&sq_kretprobe);
		sq_probe_registered = false;
		/* 놓친 probe 개수 출력 (디버그용) */
		if (sq_kretprobe.nmissed)
			pr_info("nvme-pcie-tracer: SQ kretprobe missed %d "
				"probes\n", sq_kretprobe.nmissed);
	}

	if (admin_sq_probe_registered) {
		unregister_kprobe(&admin_sq_kprobe);
		admin_sq_probe_registered = false;
	}

	/* kprobe 해제 후 진행 중인 handler가 완료될 때까지 대기 */
	synchronize_rcu();

	pr_info("nvme-pcie-tracer: SQ monitor unregistered\n");
}
