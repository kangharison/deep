/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nvme_trace_event.h - NVMe PCIe Tracer 공통 헤더
 *
 * 트레이스 이벤트 구조체, 이벤트 타입 열거형, NVMe opcode 디코딩 헬퍼를 정의한다.
 * 모든 트레이서 모듈과 trace_buffer가 이 헤더를 공유한다.
 */
#ifndef _NVME_TRACE_EVENT_H
#define _NVME_TRACE_EVENT_H

#include <linux/types.h>

/*
 * ============================================================================
 * 트레이스 방향 열거형
 * ============================================================================
 */
enum trace_direction {
	DIR_HOST_TO_DEVICE = 0,
	DIR_DEVICE_TO_HOST = 1,
	DIR_INTERNAL       = 2,
};

/*
 * ============================================================================
 * 트레이스 이벤트 타입 열거형
 * ============================================================================
 * Host → Device (0x01xx):
 *   MMIO_READ, MMIO_WRITE, DOORBELL_SQ_TAIL, DOORBELL_CQ_HEAD,
 *   CONFIG_READ, CONFIG_WRITE, SQ_SUBMIT
 *
 * Device → Host (0x02xx):
 *   CQ_COMPLETE, IRQ_FIRED, SQ_HEAD_UPDATE, DMA_MAP, DMA_UNMAP
 *
 * Internal (0x03xx):
 *   QUEUE_CREATE, QUEUE_DELETE, RESET
 */
enum trace_event_type {
	/* Host → Device */
	EVT_MMIO_READ        = 0x0100,
	EVT_MMIO_WRITE       = 0x0101,
	EVT_DOORBELL_SQ_TAIL = 0x0102,
	EVT_DOORBELL_CQ_HEAD = 0x0103,
	EVT_CONFIG_READ      = 0x0104,
	EVT_CONFIG_WRITE     = 0x0105,
	EVT_SQ_SUBMIT        = 0x0110,

	/* Device → Host */
	EVT_CQ_COMPLETE      = 0x0200,
	EVT_IRQ_FIRED        = 0x0201,
	EVT_SQ_HEAD_UPDATE   = 0x0202,
	EVT_DMA_MAP          = 0x0210,
	EVT_DMA_UNMAP        = 0x0211,

	/* Internal */
	EVT_QUEUE_CREATE     = 0x0300,
	EVT_QUEUE_DELETE     = 0x0301,
	EVT_RESET            = 0x0302,
};

/*
 * ============================================================================
 * NVMe BAR0 레지스터 오프셋
 * ============================================================================
 */
enum nvme_bar0_reg {
	REG_CAP      = 0x00,   /* Controller Capabilities (64-bit) */
	REG_VS       = 0x08,   /* Version */
	REG_INTMS    = 0x0C,   /* Interrupt Mask Set */
	REG_INTMC    = 0x10,   /* Interrupt Mask Clear */
	REG_CC       = 0x14,   /* Controller Configuration */
	REG_CSTS     = 0x1C,   /* Controller Status */
	REG_NSSR     = 0x20,   /* NVM Subsystem Reset */
	REG_AQA      = 0x24,   /* Admin Queue Attributes */
	REG_ASQ      = 0x28,   /* Admin Submission Queue Base Address (64-bit) */
	REG_ACQ      = 0x30,   /* Admin Completion Queue Base Address (64-bit) */
	REG_DOORBELL = 0x1000, /* Doorbell 레지스터 시작 */
};

/*
 * ============================================================================
 * 핵심 트레이스 이벤트 구조체
 * ============================================================================
 * 크기: 고정 헤더(40B) + union(최대 64B) = 104바이트 (패딩 포함 128B)
 *
 * relay_write()를 통해 per-CPU 링 버퍼에 기록된다.
 * 유저스페이스 reader가 debugfs relay 채널에서 이 구조체를 읽어 디코딩한다.
 */
struct nvme_trace_event {
	/* === 공통 헤더 (40바이트) === */
	u64 timestamp_ns;       /* ktime_get_ns() */
	u32 cpu;                /* smp_processor_id() */
	u16 event_type;         /* enum trace_event_type */
	u16 direction;          /* enum trace_direction */
	u32 queue_id;           /* NVMe 큐 번호 (0=Admin, 1~N=I/O) */
	u64 address;            /* MMIO 주소 또는 DMA 주소 */
	u32 size;               /* 접근 크기 (바이트) */
	u32 flags;              /* 이벤트별 플래그 */

	/* === 이벤트별 상세 정보 (최대 64바이트) === */
	union {
		/* SQ_SUBMIT: 64바이트 NVMe 커맨드 전체 (SQE raw copy) */
		u8 sqe[64];

		/* CQ_COMPLETE: 16바이트 CQE + 메타 정보 */
		struct {
			u8  cqe[16];            /* nvme_completion raw copy */
			u16 sq_head_before;
			u16 sq_head_after;
			u32 batch_count;
			u8  _pad[40];
		} completion;

		/* MMIO_READ/MMIO_WRITE: 레지스터 접근 정보 */
		struct {
			u64 bar_offset;
			u64 value;
			u32 reg_id;             /* enum nvme_bar0_reg */
			u32 width;              /* 접근 크기: 4 또는 8 바이트 */
			u8  _pad[40];
		} mmio;

		/* DOORBELL_SQ_TAIL/DOORBELL_CQ_HEAD */
		struct {
			u32 new_value;
			u32 old_value;
			u32 shadow_db;
			u32 event_idx;
			u8  mmio_skipped;       /* Shadow DB로 MMIO 생략 여부 */
			u8  _pad[47];
		} doorbell;

		/* DMA_MAP/DMA_UNMAP */
		struct {
			u64 iova;
			u64 phys_addr;
			u32 length;
			u32 dma_direction;      /* enum dma_data_direction */
			u32 nents;
			u8  _pad[32];
		} dma;

		/* IRQ_FIRED */
		struct {
			u32 vector;
			u32 irq_num;
			u32 cqes_processed;
			u8  irq_result;         /* IRQ_HANDLED or IRQ_NONE */
			u8  _pad[51];
		} irq;

		/* CONFIG_READ/CONFIG_WRITE */
		struct {
			u16 bus;
			u8  dev;
			u8  func;
			u16 offset;
			u8  _pad1[2];
			u32 value;
			u8  width;
			u8  _pad2[47];
		} config;

		/* 원시 데이터 (최대 64바이트) */
		u8 raw[64];
	};
} __attribute__((packed));

/* 구조체 크기 검증: 헤더(40) + union(64) = 104 바이트 */
_Static_assert(sizeof(struct nvme_trace_event) == 104,
	"nvme_trace_event size mismatch");

/*
 * ============================================================================
 * NVMe Opcode 디코딩 헬퍼
 * ============================================================================
 * I/O 커맨드와 Admin 커맨드의 opcode를 사람이 읽을 수 있는 문자열로 변환한다.
 */

/* NVMe I/O 커맨드 opcode → 문자열 */
static inline const char *nvme_trace_io_opcode_str(u8 opcode)
{
	switch (opcode) {
	case 0x00: return "Flush";
	case 0x01: return "Write";
	case 0x02: return "Read";
	case 0x04: return "Write Uncorrectable";
	case 0x05: return "Compare";
	case 0x08: return "Write Zeroes";
	case 0x09: return "Dataset Management";
	case 0x0C: return "Verify";
	case 0x0D: return "Reservation Register";
	case 0x0E: return "Reservation Report";
	case 0x11: return "Reservation Acquire";
	case 0x12: return "IO Management Receive";
	case 0x15: return "Reservation Release";
	case 0x19: return "IO Management Send";
	case 0x79: return "Zone Management Send";
	case 0x7A: return "Zone Management Receive";
	case 0x7D: return "Zone Append";
	default:   return "Unknown IO Cmd";
	}
}

/* NVMe Admin 커맨드 opcode → 문자열 */
static inline const char *nvme_trace_admin_opcode_str(u8 opcode)
{
	switch (opcode) {
	case 0x00: return "Delete I/O SQ";
	case 0x01: return "Create I/O SQ";
	case 0x02: return "Get Log Page";
	case 0x04: return "Delete I/O CQ";
	case 0x05: return "Create I/O CQ";
	case 0x06: return "Identify";
	case 0x08: return "Abort";
	case 0x09: return "Set Features";
	case 0x0A: return "Get Features";
	case 0x0C: return "Async Event Request";
	case 0x0D: return "Namespace Management";
	case 0x10: return "Firmware Commit";
	case 0x11: return "Firmware Image Download";
	case 0x14: return "Device Self-test";
	case 0x15: return "Namespace Attach";
	case 0x18: return "Keep Alive";
	case 0x19: return "Directive Send";
	case 0x1A: return "Directive Receive";
	case 0x1C: return "Virtualization Management";
	case 0x1D: return "NVMe-MI Send";
	case 0x1E: return "NVMe-MI Receive";
	case 0x7C: return "Doorbell Buffer Config";
	case 0x80: return "Format NVM";
	case 0x81: return "Security Send";
	case 0x82: return "Security Receive";
	case 0x84: return "Sanitize";
	case 0x86: return "Get LBA Status";
	default:   return "Unknown Admin Cmd";
	}
}

/*
 * nvme_trace_opcode_str - 큐 ID에 따라 적절한 opcode 문자열을 반환한다
 * @qid: 큐 ID (0=Admin, 1~N=I/O)
 * @opcode: NVMe 커맨드 opcode
 *
 * Admin 큐(qid=0)이면 Admin opcode, I/O 큐이면 I/O opcode로 디코딩한다.
 */
static inline const char *nvme_trace_opcode_str(u16 qid, u8 opcode)
{
	return qid ? nvme_trace_io_opcode_str(opcode)
		   : nvme_trace_admin_opcode_str(opcode);
}

/* NVMe 상태 코드 → 문자열 (CQE의 status 필드, Phase bit 제외) */
static inline const char *nvme_trace_status_str(u16 status)
{
	/* status에서 Phase bit(비트0) 제거 후 상위 비트 사용 */
	u16 sc = (status >> 1) & 0xFF;
	u16 sct = (status >> 9) & 0x7;

	if (sct == 0 && sc == 0) return "SUCCESS";
	if (sct == 0 && sc == 0x02) return "Invalid Field";
	if (sct == 0 && sc == 0x04) return "Data Transfer Error";
	if (sct == 0 && sc == 0x05) return "Aborted (Power Loss)";
	if (sct == 0 && sc == 0x06) return "Internal Error";
	if (sct == 0 && sc == 0x07) return "Abort Requested";
	if (sct == 0 && sc == 0x0B) return "Namespace Not Ready";
	if (sct == 1) return "Command Specific Error";
	if (sct == 2) return "Media/Data Integrity Error";
	if (sct == 3) return "Path Related Status";
	return "Unknown Error";
}

/* BAR0 레지스터 오프셋 → 레지스터 이름 문자열 */
static inline const char *nvme_trace_bar0_reg_str(u32 offset)
{
	switch (offset) {
	case 0x00: return "CAP";
	case 0x08: return "VS";
	case 0x0C: return "INTMS";
	case 0x10: return "INTMC";
	case 0x14: return "CC";
	case 0x1C: return "CSTS";
	case 0x20: return "NSSR";
	case 0x24: return "AQA";
	case 0x28: return "ASQ";
	case 0x30: return "ACQ";
	case 0x38: return "CMBLOC";
	case 0x3C: return "CMBSZ";
	case 0x40: return "BPINFO";
	case 0x44: return "BPRSEL";
	case 0x48: return "BPMBL";
	default:
		if (offset >= 0x1000)
			return "Doorbell";
		return "Unknown";
	}
}

/* 이벤트 타입 → 문자열 */
static inline const char *nvme_trace_event_type_str(u16 event_type)
{
	switch (event_type) {
	case EVT_MMIO_READ:        return "MMIO_READ";
	case EVT_MMIO_WRITE:       return "MMIO_WRITE";
	case EVT_DOORBELL_SQ_TAIL: return "DOORBELL_SQ_TAIL";
	case EVT_DOORBELL_CQ_HEAD: return "DOORBELL_CQ_HEAD";
	case EVT_CONFIG_READ:      return "CONFIG_READ";
	case EVT_CONFIG_WRITE:     return "CONFIG_WRITE";
	case EVT_SQ_SUBMIT:        return "SQ_SUBMIT";
	case EVT_CQ_COMPLETE:      return "CQ_COMPLETE";
	case EVT_IRQ_FIRED:        return "IRQ_FIRED";
	case EVT_SQ_HEAD_UPDATE:   return "SQ_HEAD_UPDATE";
	case EVT_DMA_MAP:          return "DMA_MAP";
	case EVT_DMA_UNMAP:        return "DMA_UNMAP";
	case EVT_QUEUE_CREATE:     return "QUEUE_CREATE";
	case EVT_QUEUE_DELETE:     return "QUEUE_DELETE";
	case EVT_RESET:            return "RESET";
	default:                   return "UNKNOWN";
	}
}

/* 방향 → 문자열 */
static inline const char *nvme_trace_direction_str(u16 direction)
{
	switch (direction) {
	case DIR_HOST_TO_DEVICE: return "H->D";
	case DIR_DEVICE_TO_HOST: return "D->H";
	case DIR_INTERNAL:       return "INTL";
	default:                 return "????";
	}
}

#endif /* _NVME_TRACE_EVENT_H */
