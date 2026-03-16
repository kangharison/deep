// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_mmio_tracer.c - NVMe BAR0 레지스터 접근 캡처
 *
 * Phase 2: kprobe를 사용하여 NVMe 컨트롤러의 BAR0 MMIO 레지스터 접근을 캡처한다.
 *
 * 후킹 대상 (pci.c):
 *
 *   static int nvme_pci_reg_read32(struct nvme_ctrl *ctrl, u32 off, u32 *val)
 *   {
 *       *val = readl(to_nvme_dev(ctrl)->bar + off);
 *       return 0;
 *   }
 *   x86_64: RDI=ctrl, RSI=off, RDX=val
 *
 *   static int nvme_pci_reg_write32(struct nvme_ctrl *ctrl, u32 off, u32 val)
 *   {
 *       writel(val, to_nvme_dev(ctrl)->bar + off);
 *       return 0;
 *   }
 *   x86_64: RDI=ctrl, RSI=off, RDX=val
 *
 *   static int nvme_pci_reg_read64(struct nvme_ctrl *ctrl, u32 off, u64 *val)
 *   {
 *       *val = lo_hi_readq(to_nvme_dev(ctrl)->bar + off);
 *       return 0;
 *   }
 *   x86_64: RDI=ctrl, RSI=off, RDX=val
 *
 * 이 함수들은 nvme_ctrl_ops 함수 테이블에 등록되어 있으며 non-inline이다.
 * NVMe core (nvme_core 모듈)에서 nvme_enable_ctrl(), nvme_disable_ctrl(),
 * nvme_wait_ready() 등에서 호출된다.
 *
 * 캡처되는 레지스터:
 *   CAP(0x00), VS(0x08), CC(0x14), CSTS(0x1C), NSSR(0x20),
 *   AQA(0x24), ASQ(0x28), ACQ(0x30) 등
 *
 * Doorbell 레지스터(0x1000+)는 이 경로를 거치지 않고 writel()로 직접 접근한다.
 * Doorbell은 nvme_doorbell_tracer.c에서 캡처한다.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/timekeeping.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/*
 * ============================================================================
 * nvme_pci_reg_read32 kprobe + kretprobe
 * ============================================================================
 * pre_handler에서 오프셋을 저장하고,
 * ret_handler에서 읽힌 값을 기록한다.
 */

struct mmio_read32_data {
	u64 timestamp_ns;
	u32 offset;
	u32 __user *val_ptr; /* 결과가 저장될 주소 */
};

static struct kretprobe read32_kretprobe;
static bool read32_probe_registered;

static int read32_entry_handler(struct kretprobe_instance *ri,
				struct pt_regs *regs)
{
	struct mmio_read32_data *data = (struct mmio_read32_data *)ri->data;

	data->timestamp_ns = ktime_get_ns();
	data->offset = (u32)regs->si;
	data->val_ptr = (u32 *)regs->dx;
	return 0;
}

static int read32_ret_handler(struct kretprobe_instance *ri,
			      struct pt_regs *regs)
{
	struct mmio_read32_data *data = (struct mmio_read32_data *)ri->data;
	struct nvme_trace_event evt;
	u32 val = 0;

	/* 반환값이 0(성공)이 아니면 무시 */
	if (regs_return_value(regs) != 0)
		return 0;

	/* val_ptr에서 읽힌 값을 안전하게 가져온다 */
	if (data->val_ptr) {
		if (copy_from_kernel_nofault(&val, data->val_ptr, sizeof(val)))
			val = 0xDEADBEEF; /* 읽기 실패 표시 */
	}

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = data->timestamp_ns;
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_MMIO_READ;
	evt.direction = DIR_HOST_TO_DEVICE;
	evt.size = 4;
	evt.address = data->offset;
	evt.mmio.bar_offset = data->offset;
	evt.mmio.value = val;
	evt.mmio.reg_id = data->offset;
	evt.mmio.width = 4;

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * nvme_pci_reg_write32 kprobe
 * ============================================================================
 * 쓰기 값은 인자에 있으므로 pre_handler만으로 충분하다.
 */

static struct kprobe write32_kprobe;
static bool write32_probe_registered;

static int write32_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct nvme_trace_event evt;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns();
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_MMIO_WRITE;
	evt.direction = DIR_HOST_TO_DEVICE;
	evt.size = 4;

	/* x86_64: RSI=off, RDX=val */
	evt.address = (u32)regs->si;
	evt.mmio.bar_offset = (u32)regs->si;
	evt.mmio.value = (u32)regs->dx;
	evt.mmio.reg_id = (u32)regs->si;
	evt.mmio.width = 4;

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * nvme_pci_reg_read64 kprobe + kretprobe
 * ============================================================================
 * 64비트 레지스터 읽기 (CAP, ASQ, ACQ 등).
 */

struct mmio_read64_data {
	u64 timestamp_ns;
	u32 offset;
	u64 *val_ptr;
};

static struct kretprobe read64_kretprobe;
static bool read64_probe_registered;

static int read64_entry_handler(struct kretprobe_instance *ri,
				struct pt_regs *regs)
{
	struct mmio_read64_data *data = (struct mmio_read64_data *)ri->data;

	data->timestamp_ns = ktime_get_ns();
	data->offset = (u32)regs->si;
	data->val_ptr = (u64 *)regs->dx;
	return 0;
}

static int read64_ret_handler(struct kretprobe_instance *ri,
			      struct pt_regs *regs)
{
	struct mmio_read64_data *data = (struct mmio_read64_data *)ri->data;
	struct nvme_trace_event evt;
	u64 val = 0;

	if (regs_return_value(regs) != 0)
		return 0;

	if (data->val_ptr) {
		if (copy_from_kernel_nofault(&val, data->val_ptr, sizeof(val)))
			val = 0xDEADBEEFDEADBEEFULL;
	}

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = data->timestamp_ns;
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_MMIO_READ;
	evt.direction = DIR_HOST_TO_DEVICE;
	evt.size = 8;
	evt.address = data->offset;
	evt.mmio.bar_offset = data->offset;
	evt.mmio.value = val;
	evt.mmio.reg_id = data->offset;
	evt.mmio.width = 8;

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 초기화/해제
 * ============================================================================
 */

int nvme_mmio_tracer_init(void)
{
	int ret;

	/* nvme_pci_reg_read32 kretprobe */
	memset(&read32_kretprobe, 0, sizeof(read32_kretprobe));
	read32_kretprobe.kp.symbol_name = "nvme_pci_reg_read32";
	read32_kretprobe.handler = read32_ret_handler;
	read32_kretprobe.entry_handler = read32_entry_handler;
	read32_kretprobe.data_size = sizeof(struct mmio_read32_data);
	read32_kretprobe.maxactive = 16;

	ret = register_kretprobe(&read32_kretprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kretprobe on "
			"nvme_pci_reg_read32 (ret=%d)\n", ret);
		read32_probe_registered = false;
	} else {
		read32_probe_registered = true;
		pr_info("nvme-pcie-tracer: MMIO tracer registered on "
			"nvme_pci_reg_read32\n");
	}

	/* nvme_pci_reg_write32 kprobe */
	memset(&write32_kprobe, 0, sizeof(write32_kprobe));
	write32_kprobe.symbol_name = "nvme_pci_reg_write32";
	write32_kprobe.pre_handler = write32_pre_handler;

	ret = register_kprobe(&write32_kprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kprobe on "
			"nvme_pci_reg_write32 (ret=%d)\n", ret);
		write32_probe_registered = false;
	} else {
		write32_probe_registered = true;
		pr_info("nvme-pcie-tracer: MMIO tracer registered on "
			"nvme_pci_reg_write32\n");
	}

	/* nvme_pci_reg_read64 kretprobe */
	memset(&read64_kretprobe, 0, sizeof(read64_kretprobe));
	read64_kretprobe.kp.symbol_name = "nvme_pci_reg_read64";
	read64_kretprobe.handler = read64_ret_handler;
	read64_kretprobe.entry_handler = read64_entry_handler;
	read64_kretprobe.data_size = sizeof(struct mmio_read64_data);
	read64_kretprobe.maxactive = 16;

	ret = register_kretprobe(&read64_kretprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kretprobe on "
			"nvme_pci_reg_read64 (ret=%d)\n", ret);
		read64_probe_registered = false;
	} else {
		read64_probe_registered = true;
		pr_info("nvme-pcie-tracer: MMIO tracer registered on "
			"nvme_pci_reg_read64\n");
	}

	if (!read32_probe_registered && !write32_probe_registered &&
	    !read64_probe_registered) {
		pr_err("nvme-pcie-tracer: no MMIO probes could be "
		       "registered\n");
		return -ENOENT;
	}

	return 0;
}

void nvme_mmio_tracer_exit(void)
{
	if (read32_probe_registered) {
		unregister_kretprobe(&read32_kretprobe);
		read32_probe_registered = false;
		if (read32_kretprobe.nmissed)
			pr_info("nvme-pcie-tracer: reg_read32 kretprobe "
				"missed %d probes\n",
				read32_kretprobe.nmissed);
	}

	if (write32_probe_registered) {
		unregister_kprobe(&write32_kprobe);
		write32_probe_registered = false;
	}

	if (read64_probe_registered) {
		unregister_kretprobe(&read64_kretprobe);
		read64_probe_registered = false;
		if (read64_kretprobe.nmissed)
			pr_info("nvme-pcie-tracer: reg_read64 kretprobe "
				"missed %d probes\n",
				read64_kretprobe.nmissed);
	}

	synchronize_rcu();
	pr_info("nvme-pcie-tracer: MMIO tracer unregistered\n");
}
