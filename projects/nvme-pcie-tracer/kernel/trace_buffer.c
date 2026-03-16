// SPDX-License-Identifier: GPL-2.0
/*
 * trace_buffer.c - Per-CPU 링 버퍼 (relay 서브시스템 기반)
 *
 * relay 서브시스템을 사용하여 per-CPU lock-free 링 버퍼를 구현한다.
 * debugfs 아래에 relay 채널 파일을 노출하여 유저스페이스에서 읽을 수 있게 한다.
 *
 * 버퍼 구조:
 *   /sys/kernel/debug/nvme-pcie-tracer/
 *   ├── trace0     (CPU 0 relay 버퍼)
 *   ├── trace1     (CPU 1 relay 버퍼)
 *   ├── ...
 *   └── traceN     (CPU N relay 버퍼)
 *
 * 유저스페이스 reader는 각 CPU의 relay 파일을 poll()+read()로 이벤트를 소비한다.
 */

#include <linux/module.h>
#include <linux/relay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/atomic.h>

#include "trace_buffer.h"
#include "nvme_trace_event.h"

/* relay 채널과 debugfs 디렉토리 */
static struct rchan *trace_chan;
static struct dentry *trace_dir;

/* 트레이싱 활성화 상태 (atomic으로 lock-free 접근) */
static atomic_t tracing_enabled = ATOMIC_INIT(0);

/* 이벤트 통계 카운터 */
static atomic64_t total_events = ATOMIC64_INIT(0);
static atomic64_t dropped_events = ATOMIC64_INIT(0);

/*
 * ============================================================================
 * relay 콜백 함수
 * ============================================================================
 */

/*
 * subbuf_start_handler - sub-buffer 시작 시 호출되는 콜백
 *
 * relay가 새 sub-buffer를 시작할 때 호출된다.
 * 버퍼가 가득 차면 오래된 데이터를 덮어쓰도록 항상 1을 반환한다.
 * 0을 반환하면 relay가 블로킹되어 kprobe handler에서 문제가 된다.
 */
static int subbuf_start_handler(struct rchan_buf *buf, void *subbuf,
				void *prev_subbuf, size_t prev_padding)
{
	if (relay_buf_full(buf)) {
		/* 버퍼 오버플로: 오래된 데이터 위에 덮어쓴다 */
		atomic64_inc(&dropped_events);
	}
	return 1; /* 항상 새 sub-buffer 시작을 허용 */
}

/*
 * create_buf_file_handler - relay 버퍼 파일 생성 콜백
 *
 * relay_open()이 각 CPU별로 호출한다.
 * debugfs에 "trace<cpu>" 파일을 생성한다.
 */
static struct dentry *create_buf_file_handler(const char *filename,
					      struct dentry *parent,
					      umode_t mode,
					      struct rchan_buf *buf,
					      int *is_global)
{
	struct dentry *d;

	d = debugfs_create_file(filename, mode, parent, buf,
				&relay_file_operations);
	return d;
}

/*
 * remove_buf_file_handler - relay 버퍼 파일 제거 콜백
 */
static int remove_buf_file_handler(struct dentry *dentry)
{
	debugfs_remove(dentry);
	return 0;
}

/* relay 콜백 테이블 */
static const struct rchan_callbacks relay_callbacks = {
	.subbuf_start    = subbuf_start_handler,
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};

/*
 * ============================================================================
 * debugfs stats 파일 - 통계 정보 표시
 * ============================================================================
 */

static ssize_t stats_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	char tmp[256];
	int len;

	len = snprintf(tmp, sizeof(tmp),
		"enabled:  %d\n"
		"total:    %lld\n"
		"dropped:  %lld\n",
		atomic_read(&tracing_enabled),
		(long long)atomic64_read(&total_events),
		(long long)atomic64_read(&dropped_events));

	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct file_operations stats_fops = {
	.owner = THIS_MODULE,
	.read  = stats_read,
};

/*
 * ============================================================================
 * enabled 파일 - 트레이싱 on/off 제어
 * ============================================================================
 * echo 1 > /sys/kernel/debug/nvme-pcie-tracer/enabled  (트레이싱 시작)
 * echo 0 > /sys/kernel/debug/nvme-pcie-tracer/enabled  (트레이싱 중지)
 */

static ssize_t enabled_read(struct file *file, char __user *buf,
			     size_t count, loff_t *ppos)
{
	char tmp[4];
	int len;

	len = snprintf(tmp, sizeof(tmp), "%d\n",
		       atomic_read(&tracing_enabled));
	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static ssize_t enabled_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	char tmp[4];
	int val;

	if (count >= sizeof(tmp))
		return -EINVAL;
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
	tmp[count] = '\0';

	if (kstrtoint(tmp, 10, &val))
		return -EINVAL;

	atomic_set(&tracing_enabled, val ? 1 : 0);
	pr_info("nvme-pcie-tracer: tracing %s\n",
		val ? "enabled" : "disabled");
	return count;
}

static const struct file_operations enabled_fops = {
	.owner = THIS_MODULE,
	.read  = enabled_read,
	.write = enabled_write,
};

/*
 * ============================================================================
 * 외부 인터페이스
 * ============================================================================
 */

int trace_buffer_init(unsigned int buf_size_kb)
{
	unsigned int subbuf_size = TRACE_SUBBUF_SIZE;
	unsigned int n_subbufs;

	if (buf_size_kb == 0)
		buf_size_kb = TRACE_DEFAULT_BUF_SIZE_KB;

	/* sub-buffer 개수 계산: 최소 2개 필요 */
	n_subbufs = (buf_size_kb * 1024) / subbuf_size;
	if (n_subbufs < 2)
		n_subbufs = 2;

	/* debugfs 디렉토리 생성 */
	trace_dir = debugfs_create_dir("nvme-pcie-tracer", NULL);
	if (IS_ERR_OR_NULL(trace_dir)) {
		pr_err("nvme-pcie-tracer: failed to create debugfs dir\n");
		return -ENOMEM;
	}

	/* relay 채널 생성: per-CPU 버퍼 자동 할당 */
	trace_chan = relay_open("trace", trace_dir,
				subbuf_size, n_subbufs,
				&relay_callbacks, NULL);
	if (!trace_chan) {
		pr_err("nvme-pcie-tracer: failed to open relay channel "
		       "(subbuf_size=%u, n_subbufs=%u)\n",
		       subbuf_size, n_subbufs);
		debugfs_remove_recursive(trace_dir);
		trace_dir = NULL;
		return -ENOMEM;
	}

	/* debugfs 제어/통계 파일 생성 */
	debugfs_create_file("enabled", 0644, trace_dir, NULL, &enabled_fops);
	debugfs_create_file("stats", 0444, trace_dir, NULL, &stats_fops);

	pr_info("nvme-pcie-tracer: trace buffer initialized "
		"(per-cpu: %u KB, subbuf: %u KB, n_subbufs: %u)\n",
		buf_size_kb, subbuf_size / 1024, n_subbufs);
	return 0;
}

int trace_buffer_write(struct nvme_trace_event *evt)
{
	if (!trace_chan)
		return -ENODEV;

	if (!atomic_read(&tracing_enabled))
		return -EAGAIN;

	/* relay_write: per-CPU 버퍼에 lock-free로 기록 */
	relay_write(trace_chan, evt, sizeof(*evt));
	atomic64_inc(&total_events);
	return 0;
}

void trace_buffer_destroy(void)
{
	if (trace_chan) {
		relay_flush(trace_chan);
		relay_close(trace_chan);
		trace_chan = NULL;
	}

	if (trace_dir) {
		debugfs_remove_recursive(trace_dir);
		trace_dir = NULL;
	}

	pr_info("nvme-pcie-tracer: trace buffer destroyed "
		"(total=%lld, dropped=%lld)\n",
		(long long)atomic64_read(&total_events),
		(long long)atomic64_read(&dropped_events));
}

void trace_buffer_set_enabled(bool enabled)
{
	atomic_set(&tracing_enabled, enabled ? 1 : 0);
}

bool trace_buffer_is_enabled(void)
{
	return atomic_read(&tracing_enabled) != 0;
}

struct dentry *trace_buffer_get_debugfs_dir(void)
{
	return trace_dir;
}
