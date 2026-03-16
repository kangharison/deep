// SPDX-License-Identifier: GPL-2.0
/*
 * nvme_dma_tracer.c - NVMe DMA 매핑 캡처
 *
 * Phase 2: kprobe를 사용하여 NVMe 디바이스의 DMA 매핑/해제를 캡처한다.
 *
 * 후킹 대상:
 *
 *   1. dma_map_sg_attrs() - DMA scatter-gather 매핑
 *      int dma_map_sg_attrs(struct device *dev, struct scatterlist *sg,
 *                           int nents, enum dma_data_direction dir,
 *                           unsigned long attrs)
 *      x86_64: RDI=dev, RSI=sg, RDX=nents, RCX=dir, R8=attrs
 *
 *   2. dma_unmap_sg_attrs() - DMA scatter-gather 해제
 *      void dma_unmap_sg_attrs(struct device *dev, struct scatterlist *sg,
 *                              int nents, enum dma_data_direction dir,
 *                              unsigned long attrs)
 *      x86_64: RDI=dev, RSI=sg, RDX=nents, RCX=dir, R8=attrs
 *
 * NVMe 디바이스 필터링:
 *   dma_map_sg_attrs()는 모든 디바이스의 DMA 매핑에서 호출된다.
 *   NVMe 디바이스만 필터링하기 위해 dev->driver->name을 확인한다.
 *
 * 오버헤드 주의:
 *   dma_map_sg_attrs()는 매우 빈번하게 호출되므로 (모든 디바이스에서),
 *   필터링 비용도 무시할 수 없다. 기본적으로 비활성화(enable_dma=0)로 설정한다.
 *
 * kretprobe 사용:
 *   dma_map_sg_attrs()의 반환값은 매핑된 세그먼트 수이다.
 *   반환 후에야 sg_dma_address()/sg_dma_len()으로 IOVA를 알 수 있다.
 *   entry_handler에서 인자를 저장하고, ret_handler에서 결과를 기록한다.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/timekeeping.h>
#include <linux/string.h>

#include "nvme_trace_event.h"
#include "trace_buffer.h"

/* 타겟 디바이스 이름 (모듈 파라미터로 설정 가능) */
static char *target_driver = "nvme";

/*
 * is_nvme_device - DMA 요청이 NVMe 디바이스에서 온 것인지 확인
 * @dev: DMA 매핑을 요청한 디바이스
 *
 * dev->driver->name == "nvme" 인지 확인한다.
 * 안전하게 NULL 체크를 수행한다.
 */
static inline bool is_nvme_device(struct device *dev)
{
	if (!dev || !dev->driver || !dev->driver->name)
		return false;
	return strcmp(dev->driver->name, target_driver) == 0;
}

/*
 * ============================================================================
 * dma_map_sg_attrs kretprobe
 * ============================================================================
 */

struct dma_map_data {
	u64 timestamp_ns;
	struct device *dev;
	struct scatterlist *sg;
	int nents;
	int direction;
};

static struct kretprobe dma_map_kretprobe;
static bool dma_map_probe_registered;

static int dma_map_entry_handler(struct kretprobe_instance *ri,
				 struct pt_regs *regs)
{
	struct dma_map_data *data = (struct dma_map_data *)ri->data;
	struct device *dev;

	dev = (struct device *)regs->di;

	/* NVMe 디바이스가 아니면 이 instance를 건너뛴다 */
	if (!is_nvme_device(dev))
		return 1; /* skip */

	data->timestamp_ns = ktime_get_ns();
	data->dev = dev;
	data->sg = (struct scatterlist *)regs->si;
	data->nents = (int)regs->dx;
	data->direction = (int)regs->cx;
	return 0;
}

static int dma_map_ret_handler(struct kretprobe_instance *ri,
			       struct pt_regs *regs)
{
	struct dma_map_data *data = (struct dma_map_data *)ri->data;
	struct nvme_trace_event evt;
	int mapped_nents;

	mapped_nents = (int)regs_return_value(regs);

	/* 매핑 실패 시 (반환값 <= 0) 무시 */
	if (mapped_nents <= 0)
		return 0;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = data->timestamp_ns;
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_DMA_MAP;
	evt.direction = DIR_DEVICE_TO_HOST; /* DMA는 디바이스→호스트 방향 */
	evt.dma.nents = mapped_nents;
	evt.dma.dma_direction = data->direction;

	/*
	 * 첫 번째 scatter-gather 엔트리의 DMA 주소와 길이를 기록한다.
	 * 매핑 후에는 sg_dma_address()/sg_dma_len()으로 IOVA를 얻을 수 있다.
	 */
	if (data->sg && mapped_nents > 0) {
		evt.dma.iova = sg_dma_address(data->sg);
		evt.dma.length = sg_dma_len(data->sg);
		evt.dma.phys_addr = sg_phys(data->sg);
		evt.address = evt.dma.iova;
		evt.size = evt.dma.length;
	}

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * dma_unmap_sg_attrs kprobe
 * ============================================================================
 * unmap은 반환값이 없으므로 pre_handler만 사용한다.
 */

static struct kprobe dma_unmap_kprobe;
static bool dma_unmap_probe_registered;

static int dma_unmap_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct device *dev;
	struct scatterlist *sg;
	struct nvme_trace_event evt;

	dev = (struct device *)regs->di;

	/* NVMe 디바이스가 아니면 무시 */
	if (!is_nvme_device(dev))
		return 0;

	sg = (struct scatterlist *)regs->si;

	memset(&evt, 0, sizeof(evt));
	evt.timestamp_ns = ktime_get_ns();
	evt.cpu = smp_processor_id();
	evt.event_type = EVT_DMA_UNMAP;
	evt.direction = DIR_DEVICE_TO_HOST;
	evt.dma.nents = (int)regs->dx;
	evt.dma.dma_direction = (int)regs->cx;

	if (sg) {
		evt.dma.iova = sg_dma_address(sg);
		evt.dma.length = sg_dma_len(sg);
		evt.address = evt.dma.iova;
		evt.size = evt.dma.length;
	}

	trace_buffer_write(&evt);
	return 0;
}

/*
 * ============================================================================
 * 초기화/해제
 * ============================================================================
 */

int nvme_dma_tracer_init(void)
{
	int ret;

	/* dma_map_sg_attrs kretprobe */
	memset(&dma_map_kretprobe, 0, sizeof(dma_map_kretprobe));
	dma_map_kretprobe.kp.symbol_name = "dma_map_sg_attrs";
	dma_map_kretprobe.handler = dma_map_ret_handler;
	dma_map_kretprobe.entry_handler = dma_map_entry_handler;
	dma_map_kretprobe.data_size = sizeof(struct dma_map_data);
	dma_map_kretprobe.maxactive = 128;

	ret = register_kretprobe(&dma_map_kretprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kretprobe on "
			"dma_map_sg_attrs (ret=%d). DMA map tracing "
			"unavailable.\n", ret);
		dma_map_probe_registered = false;
	} else {
		dma_map_probe_registered = true;
		pr_info("nvme-pcie-tracer: DMA tracer registered on "
			"dma_map_sg_attrs\n");
	}

	/* dma_unmap_sg_attrs kprobe */
	memset(&dma_unmap_kprobe, 0, sizeof(dma_unmap_kprobe));
	dma_unmap_kprobe.symbol_name = "dma_unmap_sg_attrs";
	dma_unmap_kprobe.pre_handler = dma_unmap_pre_handler;

	ret = register_kprobe(&dma_unmap_kprobe);
	if (ret < 0) {
		pr_warn("nvme-pcie-tracer: failed to register kprobe on "
			"dma_unmap_sg_attrs (ret=%d). DMA unmap tracing "
			"unavailable.\n", ret);
		dma_unmap_probe_registered = false;
	} else {
		dma_unmap_probe_registered = true;
		pr_info("nvme-pcie-tracer: DMA tracer registered on "
			"dma_unmap_sg_attrs\n");
	}

	if (!dma_map_probe_registered && !dma_unmap_probe_registered) {
		pr_err("nvme-pcie-tracer: no DMA probes could be "
		       "registered\n");
		return -ENOENT;
	}

	return 0;
}

void nvme_dma_tracer_exit(void)
{
	if (dma_map_probe_registered) {
		unregister_kretprobe(&dma_map_kretprobe);
		dma_map_probe_registered = false;
		if (dma_map_kretprobe.nmissed)
			pr_info("nvme-pcie-tracer: DMA map kretprobe "
				"missed %d probes\n",
				dma_map_kretprobe.nmissed);
	}

	if (dma_unmap_probe_registered) {
		unregister_kprobe(&dma_unmap_kprobe);
		dma_unmap_probe_registered = false;
	}

	synchronize_rcu();
	pr_info("nvme-pcie-tracer: DMA tracer unregistered\n");
}
