/* SPDX-License-Identifier: GPL-2.0 */
/*
 * trace_buffer.h - Per-CPU 링 버퍼 (relay 서브시스템 기반)
 *
 * relay_open()으로 debugfs 하위에 per-CPU 채널을 생성하고,
 * relay_write()로 lock-free 이벤트 기록을 수행한다.
 * 유저스페이스에서 /sys/kernel/debug/nvme-pcie-tracer/trace<cpu> 파일을 읽어
 * 이벤트를 소비한다.
 */
#ifndef _TRACE_BUFFER_H
#define _TRACE_BUFFER_H

#include <linux/types.h>
#include "nvme_trace_event.h"

/* 기본 버퍼 설정 (모듈 파라미터로 변경 가능) */
#define TRACE_DEFAULT_BUF_SIZE_KB   256       /* per-CPU 버퍼 크기 (KB) */
#define TRACE_SUBBUF_SIZE           (64 * 1024)  /* sub-buffer 크기 (64KB) */

/*
 * trace_buffer_init - relay 채널과 debugfs 디렉토리를 초기화한다
 * @buf_size_kb: per-CPU 버퍼 크기 (KB 단위). sub-buffer 수를 자동으로 계산한다.
 *
 * 호출 후 /sys/kernel/debug/nvme-pcie-tracer/ 아래에 relay 파일이 생성된다.
 * 반환: 0=성공, 음수=에러
 */
int trace_buffer_init(unsigned int buf_size_kb);

/*
 * trace_buffer_write - 이벤트를 per-CPU relay 채널에 기록한다
 * @evt: 기록할 트레이스 이벤트 구조체 포인터
 *
 * relay_write()는 내부적으로 per-CPU 버퍼를 사용하므로 별도의 락이 필요 없다.
 * kprobe handler 등 preempt-disabled 컨텍스트에서 안전하게 호출 가능하다.
 * 트레이싱이 비활성화 상태이면 바로 반환한다.
 * 반환: 0=성공, -EAGAIN=비활성화 상태, -ENODEV=초기화 안 됨
 */
int trace_buffer_write(struct nvme_trace_event *evt);

/*
 * trace_buffer_destroy - relay 채널과 debugfs 항목을 정리한다
 *
 * 모듈 언로드 시 호출한다.
 * 내부에서 relay_close()와 debugfs_remove_recursive()를 수행한다.
 */
void trace_buffer_destroy(void);

/*
 * trace_buffer_set_enabled - 트레이싱 활성화/비활성화
 * @enabled: true=활성화, false=비활성화
 */
void trace_buffer_set_enabled(bool enabled);

/*
 * trace_buffer_is_enabled - 트레이싱 활성화 상태 반환
 */
bool trace_buffer_is_enabled(void);

/*
 * trace_buffer_get_debugfs_dir - debugfs 디렉토리 dentry 반환
 *
 * 다른 모듈이 이 디렉토리 아래에 추가 debugfs 항목을 만들 때 사용한다.
 */
struct dentry *trace_buffer_get_debugfs_dir(void);

#endif /* _TRACE_BUFFER_H */
