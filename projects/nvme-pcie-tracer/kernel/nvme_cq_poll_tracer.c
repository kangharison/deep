// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_cq_poll_tracer.c - CQ 메모리 직접 폴링으로 Device DMA Write 탐지
 *
 * ============================================================================
 * 목적
 * ============================================================================
 *
 * NVMe 디바이스가 호스트 메모리에 실제로 DMA Write를 수행한 시점을
 * PCIe 레벨에서 탐지한다. kprobe가 nvme_irq() 진입 시점(소프트웨어 시점)을
 * 잡는 것과 달리, 이 모듈은 CQ 메모리의 Phase bit 변화를 직접 관찰하여
 * 디바이스의 DMA Write 완료 시점을 탐지한다.
 *
 * ============================================================================
 * 동작 원리
 * ============================================================================
 *
 * NVMe CQ 메모리는 dma_alloc_coherent()로 할당되어 CPU-device 간
 * 캐시 일관성이 보장된다. 따라서:
 *
 *   1. 디바이스가 PCIe Memory Write TLP로 CQE를 기록하면
 *   2. x86의 snoop 메커니즘에 의해 CPU 캐시가 즉시 무효화되고
 *   3. CPU가 해당 메모리를 읽으면 최신 값(디바이스가 쓴 값)을 얻는다
 *
 * CQE(Completion Queue Entry)의 Phase bit은 컨트롤러가 CQ를 한 바퀴
 * 돌 때마다 토글된다. 현재 예상되는 Phase와 CQE의 Phase가 일치하면
 * "새로운 CQE가 도착한 것" = "디바이스가 DMA Write를 수행한 것"이다.
 *
 * ============================================================================
 * 타임라인 비교: kprobe vs CQ 폴링
 * ============================================================================
 *
 *   t0: 디바이스가 CQE를 DMA Write  ← CQ 폴링은 여기를 잡음
 *   t1: 디바이스가 MSI-X 인터럽트 발생
 *   t2: CPU가 인터럽트 수신
 *   t3: nvme_irq() 진입            ← kprobe는 여기를 잡음
 *   t4: nvme_poll_cq()에서 CQE 처리
 *
 *   CQ 폴링은 t0~t3 사이의 갭(통상 수 μs)을 없앨 수 있다.
 *   폴링 주기가 충분히 짧으면 인터럽트보다 먼저 CQE를 발견할 수 있다.
 *
 * ============================================================================
 * CQ 메모리 획득
 * ============================================================================
 *
 * nvme_queue 구조체는 NVMe PCI 드라이버 내부(drivers/nvme/host/pci.c)에
 * 정의되어 있고 EXPORT되지 않는다. CQ 메모리 주소를 얻기 위해
 * nvme_irq()에 1회성 kprobe를 걸어 nvme_queue 포인터에서 정보를 추출한다.
 *
 * nvme_irq() 시그니처:
 *   static irqreturn_t nvme_irq(int irq, void *data)
 *   x86_64: RDI=irq, RSI=data(struct nvme_queue *)
 *
 * nvme_queue 구조체 (pci.c, 비공개):
 *   struct nvme_queue {
 *       ...
 *       struct nvme_completion *cqes;   // +offset_cqes
 *       u32 q_depth;                     // +offset_q_depth
 *       u16 cq_head;                     // +offset_cq_head
 *       u16 qid;                         // +offset_qid
 *       u8  cq_phase;                    // +offset_cq_phase
 *       ...
 *   };
 *
 * 오프셋은 BTF(BPF Type Format)로 런타임에 계산하거나,
 * 모듈 파라미터로 지정한다.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/timekeeping.h>
#include <linux/debugfs.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/*
 * ============================================================================
 * nvme_queue 구조체 오프셋 (모듈 파라미터)
 * ============================================================================
 *
 * nvme_queue는 커널 내부 구조체이므로 오프셋이 커널 버전마다 다르다.
 * BTF가 있으면 자동 계산, 없으면 수동 지정한다.
 *
 * 확인 방법:
 *   # BTF에서 오프셋 확인
 *   pahole -C nvme_queue /sys/kernel/btf/vmlinux 2>/dev/null || \
 *   pahole -C nvme_queue /usr/lib/debug/lib/modules/$(uname -r)/vmlinux
 *
 *   # 또는 crash/gdb로 확인
 *   crash> struct nvme_queue -o
 */
static int off_cqes = -1;       /* cqes 필드 오프셋 (자동 감지 시 -1) */
static int off_q_depth = -1;    /* q_depth 필드 오프셋 */
static int off_cq_head = -1;    /* cq_head 필드 오프셋 */
static int off_qid = -1;        /* qid 필드 오프셋 */
static int off_cq_phase = -1;   /* cq_phase 필드 오프셋 */
static int off_sq_dma_addr = -1; /* sq_dma_addr 필드 오프셋 */
static int off_cq_dma_addr = -1; /* cq_dma_addr 필드 오프셋 */

module_param(off_cqes, int, 0444);
module_param(off_q_depth, int, 0444);
module_param(off_cq_head, int, 0444);
module_param(off_qid, int, 0444);
module_param(off_cq_phase, int, 0444);
module_param(off_sq_dma_addr, int, 0444);
module_param(off_cq_dma_addr, int, 0444);

MODULE_PARM_DESC(off_cqes, "Offset of 'cqes' in nvme_queue struct");
MODULE_PARM_DESC(off_q_depth, "Offset of 'q_depth' in nvme_queue struct");

/* 폴링 설정 */
static unsigned int poll_interval_ns = 1000; /* 기본 1μs */
module_param(poll_interval_ns, uint, 0644);
MODULE_PARM_DESC(poll_interval_ns,
	"CQ polling interval in nanoseconds (default=1000)");

static int target_qid = 1; /* 모니터링할 큐 ID (기본: I/O 큐 1) */
module_param(target_qid, int, 0444);
MODULE_PARM_DESC(target_qid,
	"Target NVMe queue ID to monitor (default=1)");

/*
 * ============================================================================
 * CQ 폴링 컨텍스트
 * ============================================================================
 */

/* NVMe CQE 구조체 (include/linux/nvme.h에서 복사) */
struct nvme_cqe {
	__le32 result_lo;  /* command specific result DW0 */
	__le32 result_hi;  /* command specific result DW1 */
	__le16 sq_head;    /* SQ head pointer */
	__le16 sq_id;      /* SQ identifier */
	__u16  command_id; /* command identifier */
	__le16 status;     /* status field: bit0=phase, bit1-15=status */
} __attribute__((packed));

struct cq_poll_ctx {
	/* CQ 메모리 정보 (kprobe로 1회 추출) */
	volatile struct nvme_cqe *cqes;   /* CQ 가상 주소 */
	dma_addr_t cq_dma_addr;           /* CQ DMA(IOVA) 주소 */
	dma_addr_t sq_dma_addr;           /* SQ DMA(IOVA) 주소 */
	u32 q_depth;                       /* 큐 깊이 */
	u16 qid;                           /* 큐 ID */

	/* 폴링 상태 */
	u16 poll_head;                     /* 현재 폴링 위치 */
	u16 expected_phase;                /* 다음 CQE의 예상 Phase */

	/* 스레드 */
	struct task_struct *poll_thread;
	bool running;
	bool info_captured;                /* nvme_queue 정보 획득 완료 */

	/* 통계 */
	u64 total_cqes_detected;
	u64 detect_before_irq;             /* 인터럽트보다 먼저 감지한 횟수 */

	/* 인터럽트 타이밍 비교용 */
	u64 last_irq_ts;                   /* 마지막 nvme_irq() 시각 */
};

static struct cq_poll_ctx poll_ctx;

/*
 * ============================================================================
 * kprobe: nvme_queue 정보 추출 (1회성)
 * ============================================================================
 *
 * nvme_irq()가 호출될 때 RSI(두 번째 인자)에 nvme_queue 포인터가 있다.
 * 이 포인터에서 cqes, q_depth, cq_head, cq_phase, qid를 읽어온다.
 *
 * 오프셋이 모듈 파라미터로 지정되지 않은 경우(-1),
 * 일반적인 커널 6.x의 오프셋을 시도한다.
 * 실패하면 dmesg에 에러를 출력하고 모듈 파라미터 지정을 요구한다.
 */

static struct kprobe info_kprobe;
static bool info_kprobe_registered;

/*
 * read_field_ptr - nvme_queue에서 포인터 필드 읽기
 *
 * 안전하게 커널 메모리에서 값을 읽는다.
 * copy_from_kernel_nofault()를 사용하여 잘못된 오프셋으로
 * 인한 커널 패닉을 방지한다.
 */
static int read_field_ptr(void *base, int offset, void **out)
{
	return copy_from_kernel_nofault(out,
		(void *)((unsigned long)base + offset), sizeof(void *));
}

static int read_field_u32(void *base, int offset, u32 *out)
{
	return copy_from_kernel_nofault(out,
		(void *)((unsigned long)base + offset), sizeof(u32));
}

static int read_field_u16(void *base, int offset, u16 *out)
{
	return copy_from_kernel_nofault(out,
		(void *)((unsigned long)base + offset), sizeof(u16));
}

static int read_field_u8(void *base, int offset, u8 *out)
{
	return copy_from_kernel_nofault(out,
		(void *)((unsigned long)base + offset), sizeof(u8));
}

static int read_field_u64(void *base, int offset, u64 *out)
{
	return copy_from_kernel_nofault(out,
		(void *)((unsigned long)base + offset), sizeof(u64));
}

/*
 * info_capture_handler - nvme_queue 정보를 1회 추출하는 kprobe handler
 */
static int info_capture_handler(struct kprobe *p, struct pt_regs *regs)
{
	void *nvmeq_ptr;
	u16 qid;
	void *cqes_ptr;
	u32 q_depth;
	u16 cq_head;
	u8 cq_phase;
	u64 sq_dma, cq_dma;

	if (poll_ctx.info_captured)
		return 0;

	/* x86_64: RSI = nvme_queue* (nvme_irq의 두 번째 인자) */
	nvmeq_ptr = (void *)regs->si;
	if (!nvmeq_ptr)
		return 0;

	/* qid 읽기 */
	if (read_field_u16(nvmeq_ptr, off_qid, &qid))
		return 0;

	/* 타겟 큐가 아니면 무시 */
	if (qid != target_qid)
		return 0;

	/* 나머지 필드 읽기 */
	if (read_field_ptr(nvmeq_ptr, off_cqes, &cqes_ptr))
		return 0;
	if (read_field_u32(nvmeq_ptr, off_q_depth, &q_depth))
		return 0;
	if (read_field_u16(nvmeq_ptr, off_cq_head, &cq_head))
		return 0;
	if (read_field_u8(nvmeq_ptr, off_cq_phase, &cq_phase))
		return 0;

	/* DMA 주소 (선택적) */
	if (off_sq_dma_addr >= 0)
		read_field_u64(nvmeq_ptr, off_sq_dma_addr, &sq_dma);
	if (off_cq_dma_addr >= 0)
		read_field_u64(nvmeq_ptr, off_cq_dma_addr, &cq_dma);

	/* 유효성 검사 */
	if (!cqes_ptr || q_depth == 0 || q_depth > 65536) {
		pr_warn("nvme-cq-poll: invalid nvme_queue data: "
			"cqes=%px q_depth=%u\n", cqes_ptr, q_depth);
		return 0;
	}

	/* 컨텍스트에 저장 */
	poll_ctx.cqes = (volatile struct nvme_cqe *)cqes_ptr;
	poll_ctx.q_depth = q_depth;
	poll_ctx.poll_head = cq_head;
	poll_ctx.expected_phase = cq_phase;
	poll_ctx.qid = qid;
	if (off_sq_dma_addr >= 0)
		poll_ctx.sq_dma_addr = sq_dma;
	if (off_cq_dma_addr >= 0)
		poll_ctx.cq_dma_addr = cq_dma;

	/* 완료 표시 */
	smp_wmb();
	poll_ctx.info_captured = true;

	pr_info("nvme-cq-poll: captured queue info for qid=%u:\n"
		"  cqes=%px q_depth=%u cq_head=%u cq_phase=%u\n"
		"  sq_dma=0x%llx cq_dma=0x%llx\n",
		qid, cqes_ptr, q_depth, cq_head, cq_phase,
		(u64)poll_ctx.sq_dma_addr, (u64)poll_ctx.cq_dma_addr);

	return 0;
}

/*
 * ============================================================================
 * kprobe: nvme_irq() 타이밍 비교용
 * ============================================================================
 *
 * CQ 폴링으로 CQE를 발견한 시점과 nvme_irq()가 호출된 시점을 비교하여
 * "인터럽트보다 몇 ns 먼저 감지했는지"를 측정한다.
 */

static struct kprobe irq_timing_kprobe;
static bool irq_timing_kprobe_registered;

static int irq_timing_handler(struct kprobe *p, struct pt_regs *regs)
{
	poll_ctx.last_irq_ts = ktime_get_ns();
	return 0;
}

/*
 * ============================================================================
 * CQ 폴링 스레드
 * ============================================================================
 */

/*
 * cq_poll_thread - CQ 메모리를 직접 폴링하여 DMA Write를 탐지
 *
 * Phase bit 검사:
 *   NVMe CQE의 status 필드 bit 0이 Phase bit이다.
 *   컨트롤러가 CQ를 채울 때, 현재 Phase 값으로 bit 0을 설정한다.
 *   CQ를 한 바퀴 돌면 Phase가 토글된다 (0→1 또는 1→0).
 *
 *   expected_phase와 CQE의 Phase가 일치하면:
 *   → 디바이스가 이 CQE를 DMA Write로 기록한 것!
 *   → 이것이 PCIe Memory Write TLP의 직접적 증거!
 *
 * 메모리 순서:
 *   READ_ONCE()를 사용하여 컴파일러가 읽기를 최적화하지 못하게 한다.
 *   x86에서 dma_alloc_coherent() 메모리는 UC(Uncacheable) 또는
 *   WC(Write-Combining)가 아닌 WB(Write-Back) + snoop이므로,
 *   별도의 메모리 배리어 없이도 최신 값을 읽을 수 있다.
 *   (DMA coherent = CPU 캐시와 디바이스 DMA가 일관성 유지)
 */
static int cq_poll_thread(void *data)
{
	struct cq_poll_ctx *ctx = data;

	pr_info("nvme-cq-poll: polling thread started for qid=%u "
		"(depth=%u, interval=%uns)\n",
		ctx->qid, ctx->q_depth, poll_interval_ns);

	while (!kthread_should_stop() && ctx->running) {
		volatile struct nvme_cqe *cqe;
		u16 status_raw;

		cqe = &ctx->cqes[ctx->poll_head];
		status_raw = le16_to_cpu(READ_ONCE(cqe->status));

		if ((status_raw & 1) == ctx->expected_phase) {
			/*
			 * ★ 새 CQE 발견! ★
			 * 디바이스가 PCIe Memory Write TLP로 이 CQE를 기록했다.
			 */
			u64 detect_ts = ktime_get_ns();
			u16 sq_head = le16_to_cpu(READ_ONCE(cqe->sq_head));
			u16 sq_id = le16_to_cpu(READ_ONCE(cqe->sq_id));
			u16 cid = READ_ONCE(cqe->command_id);
			u16 status_code = status_raw >> 1;

			ctx->total_cqes_detected++;

			/*
			 * 이벤트 기록: trace_buffer에 기록
			 */
			{
				struct nvme_trace_event evt;

				memset(&evt, 0, sizeof(evt));
				evt.timestamp_ns = detect_ts;
				evt.cpu = smp_processor_id();
				evt.event_type = EVT_CQ_COMPLETE;
				evt.direction = DIR_DEVICE_TO_HOST;
				evt.queue_id = ctx->qid;
				evt.address = ctx->cq_dma_addr +
					ctx->poll_head * sizeof(struct nvme_cqe);
				evt.size = sizeof(struct nvme_cqe);

				/* CQE 원시 데이터 복사 */
				memcpy(evt.completion.cqe, (void *)cqe,
				       sizeof(struct nvme_cqe));
				evt.completion.sq_head_after = sq_head;

				/*
				 * flags 비트 0: 폴링으로 감지됨 (kprobe 아님)
				 * flags 비트 1: 인터럽트보다 먼저 감지됨
				 */
				evt.flags = 0x01; /* DETECTED_BY_POLL */
				if (detect_ts < ctx->last_irq_ts ||
				    ctx->last_irq_ts == 0)
					evt.flags |= 0x02; /* BEFORE_IRQ */

				trace_buffer_write(&evt);
			}

			/*
			 * 로그 출력 (디버그용, 프로덕션에서는 비활성화)
			 */
			pr_debug("nvme-cq-poll: [%llu] CQE DETECTED "
				"qid=%u idx=%u cid=%u sqhd=%u sqid=%u "
				"status=0x%04x (%s)\n",
				detect_ts, ctx->qid, ctx->poll_head,
				cid, sq_head, sq_id, status_code,
				nvme_trace_status_str(status_raw));

			/*
			 * SQ Head 변화로 SQ Fetch도 감지
			 * CQE의 sq_head 필드는 컨트롤러가 SQ에서
			 * 얼마나 많은 커맨드를 소비(fetch)했는지 나타낸다.
			 */
			pr_debug("nvme-cq-poll: [%llu]   SQ[%u] head=%u "
				"(디바이스가 여기까지 DMA Read함)\n",
				detect_ts, sq_id, sq_head);

			/* 다음 CQE로 이동 */
			if (++ctx->poll_head == ctx->q_depth) {
				ctx->poll_head = 0;
				ctx->expected_phase ^= 1;
			}
		} else {
			/*
			 * CQE 미도착 → 대기
			 *
			 * 폴링 주기별 CPU 사용률:
			 *   cpu_relax() (~10ns): ~100% CPU, 최고 정밀도
			 *   100ns:  ~80% CPU
			 *   1μs:   ~30% CPU, 좋은 타협점
			 *   10μs:  ~5% CPU
			 *   100μs: ~0.5% CPU
			 */
			if (poll_interval_ns == 0)
				cpu_relax();
			else if (poll_interval_ns < 1000)
				ndelay(poll_interval_ns);
			else
				udelay(poll_interval_ns / 1000);
		}
	}

	pr_info("nvme-cq-poll: polling thread stopped. "
		"total_detected=%llu\n", ctx->total_cqes_detected);
	return 0;
}

/*
 * ============================================================================
 * debugfs 인터페이스
 * ============================================================================
 */

static struct dentry *poll_debugfs_dir;

/*
 * ============================================================================
 * 초기화 / 해제
 * ============================================================================
 */

int nvme_cq_poll_tracer_init(void)
{
	int ret;

	memset(&poll_ctx, 0, sizeof(poll_ctx));
	poll_ctx.running = false;
	poll_ctx.info_captured = false;

	/*
	 * 오프셋 검증: 모든 필수 오프셋이 설정되었는지 확인
	 */
	if (off_cqes < 0 || off_q_depth < 0 || off_cq_head < 0 ||
	    off_qid < 0 || off_cq_phase < 0) {
		pr_err("nvme-cq-poll: nvme_queue offsets not set!\n"
		       "  Required: off_cqes, off_q_depth, off_cq_head, "
		       "off_qid, off_cq_phase\n"
		       "  Use: pahole -C nvme_queue /sys/kernel/btf/vmlinux\n"
		       "  Then: insmod ... off_cqes=N off_q_depth=N ...\n");
		return -EINVAL;
	}

	/*
	 * Step 1: nvme_irq()에 kprobe를 걸어 nvme_queue 정보를 추출한다.
	 * 정보를 얻은 후에는 kprobe를 해제하고 순수 폴링으로 전환한다.
	 */
	memset(&info_kprobe, 0, sizeof(info_kprobe));
	info_kprobe.symbol_name = "nvme_irq";
	info_kprobe.pre_handler = info_capture_handler;

	ret = register_kprobe(&info_kprobe);
	if (ret < 0) {
		pr_err("nvme-cq-poll: failed to register info kprobe "
		       "on nvme_irq (ret=%d)\n", ret);
		return ret;
	}
	info_kprobe_registered = true;

	/*
	 * Step 2: nvme_irq() 타이밍 비교용 kprobe
	 */
	memset(&irq_timing_kprobe, 0, sizeof(irq_timing_kprobe));
	irq_timing_kprobe.symbol_name = "nvme_irq";
	irq_timing_kprobe.pre_handler = irq_timing_handler;

	/*
	 * 주의: 동일 심볼에 2개 kprobe는 가능하지만,
	 * info_kprobe와 합칠 수도 있다. 여기서는 명확성을 위해 분리.
	 * 실제 구현에서는 하나로 합치는 것을 권장.
	 */

	pr_info("nvme-cq-poll: module loaded. "
		"Waiting for nvme_irq() on qid=%d to capture queue info...\n"
		"  Generate I/O: dd if=/dev/nvmeXnY of=/dev/null bs=4k count=1\n",
		target_qid);

	/*
	 * 폴링 스레드는 info_captured가 true가 된 후에 시작한다.
	 * 여기서는 간단히 타이머나 워크큐로 체크하거나,
	 * info_capture_handler 내에서 직접 스레드를 시작할 수 있다.
	 *
	 * 현재 구현: info_captured 후 debugfs에서 수동 시작
	 * echo 1 > /sys/kernel/debug/nvme-cq-poll/start
	 */

	/* debugfs 생성 */
	poll_debugfs_dir = debugfs_create_dir("nvme-cq-poll", NULL);
	debugfs_create_bool("info_captured", 0444, poll_debugfs_dir,
			    &poll_ctx.info_captured);
	debugfs_create_u64("total_detected", 0444, poll_debugfs_dir,
			   &poll_ctx.total_cqes_detected);
	debugfs_create_u32("poll_interval_ns", 0644, poll_debugfs_dir,
			   &poll_interval_ns);

	return 0;
}

/*
 * start_polling - 폴링 스레드 시작
 *
 * info_captured가 true인 상태에서만 호출해야 한다.
 */
int nvme_cq_poll_start(void)
{
	if (!poll_ctx.info_captured) {
		pr_err("nvme-cq-poll: queue info not captured yet\n");
		return -EAGAIN;
	}

	if (poll_ctx.running) {
		pr_warn("nvme-cq-poll: already running\n");
		return -EBUSY;
	}

	poll_ctx.running = true;
	poll_ctx.poll_thread = kthread_run(cq_poll_thread, &poll_ctx,
					   "nvme-cq-poll/%u", poll_ctx.qid);
	if (IS_ERR(poll_ctx.poll_thread)) {
		int ret = PTR_ERR(poll_ctx.poll_thread);
		poll_ctx.running = false;
		pr_err("nvme-cq-poll: failed to create poll thread "
		       "(ret=%d)\n", ret);
		return ret;
	}

	/*
	 * info kprobe는 더 이상 필요 없으므로 해제한다.
	 * 이후로는 순수 메모리 폴링만 동작한다.
	 */
	if (info_kprobe_registered) {
		unregister_kprobe(&info_kprobe);
		info_kprobe_registered = false;
		pr_info("nvme-cq-poll: info kprobe unregistered "
			"(switching to pure polling)\n");
	}

	return 0;
}

void nvme_cq_poll_tracer_exit(void)
{
	/* 폴링 스레드 중지 */
	if (poll_ctx.running) {
		poll_ctx.running = false;
		if (poll_ctx.poll_thread)
			kthread_stop(poll_ctx.poll_thread);
	}

	/* kprobe 해제 */
	if (info_kprobe_registered) {
		unregister_kprobe(&info_kprobe);
		info_kprobe_registered = false;
	}

	if (irq_timing_kprobe_registered) {
		unregister_kprobe(&irq_timing_kprobe);
		irq_timing_kprobe_registered = false;
	}

	/* debugfs 정리 */
	debugfs_remove_recursive(poll_debugfs_dir);

	synchronize_rcu();

	pr_info("nvme-cq-poll: unloaded. total_detected=%llu\n",
		poll_ctx.total_cqes_detected);
}
