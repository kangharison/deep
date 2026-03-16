/*
 * nvme_trace_decoder.h - NVMe trace event definitions and decoder API
 *
 * Shared between kernel module (trace_events.h) and userspace tools.
 * This file mirrors the kernel trace_events.h definitions for userspace use.
 *
 * Copyright (c) 2026
 * License: GPL-2.0
 */

#ifndef NVME_TRACE_DECODER_H
#define NVME_TRACE_DECODER_H

#include <stdint.h>

/* ================================================================
 * Direction and event type enumerations
 * Must stay in sync with kernel/trace_events.h
 * ================================================================ */

enum trace_direction {
	DIR_HOST_TO_DEVICE = 0,
	DIR_DEVICE_TO_HOST = 1,
	DIR_INTERNAL       = 2,
};

enum trace_event_type {
	/* Host -> Device (0x01xx) */
	EVT_MMIO_READ        = 0x0100,
	EVT_MMIO_WRITE       = 0x0101,
	EVT_DOORBELL_SQ_TAIL = 0x0102,
	EVT_DOORBELL_CQ_HEAD = 0x0103,
	EVT_CONFIG_READ      = 0x0104,
	EVT_CONFIG_WRITE     = 0x0105,
	EVT_SQ_SUBMIT        = 0x0110,

	/* Device -> Host (0x02xx) */
	EVT_CQ_COMPLETE      = 0x0200,
	EVT_IRQ_FIRED        = 0x0201,
	EVT_SQ_HEAD_UPDATE   = 0x0202,
	EVT_DMA_MAP          = 0x0210,
	EVT_DMA_UNMAP        = 0x0211,

	/* Internal (0x03xx) */
	EVT_QUEUE_CREATE     = 0x0300,
	EVT_QUEUE_DELETE     = 0x0301,
	EVT_RESET            = 0x0302,
};

/* ================================================================
 * Trace event structure (128 bytes, packed)
 * Must match kernel struct nvme_trace_event exactly.
 * ================================================================ */

struct nvme_trace_event {
	/* Common header (40 bytes) */
	uint64_t timestamp_ns;
	uint32_t cpu;
	uint16_t event_type;
	uint16_t direction;
	uint32_t queue_id;
	uint64_t address;
	uint32_t size;
	uint32_t flags;

	/* Event-specific payload (max 64 bytes) */
	union {
		/* SQ Submit: 64-byte NVMe command */
		uint8_t cmd_bytes[64];

		/* CQ Complete: 16-byte CQE + metadata */
		struct {
			uint8_t  cqe_bytes[16];
			uint16_t sq_head_before;
			uint16_t sq_head_after;
			uint32_t batch_count;
		} __attribute__((packed));

		/* MMIO register access */
		struct {
			uint64_t bar_offset;
			uint64_t value;
			uint32_t reg_id;
			uint32_t width;
		} __attribute__((packed)) mmio_info;

		/* Doorbell */
		struct {
			uint32_t new_value;
			uint32_t old_value;
			uint32_t shadow_db;
			uint32_t event_idx;
			uint8_t  mmio_skipped;
		} __attribute__((packed)) db;

		/* DMA mapping */
		struct {
			uint64_t iova;
			uint64_t phys_addr;
			uint32_t length;
			uint32_t dma_direction;
			uint32_t nents;
		} __attribute__((packed)) dma_info;

		/* IRQ */
		struct {
			uint32_t vector;
			uint32_t irq_num;
			uint32_t cqes_processed;
			uint8_t  irq_result;
		} __attribute__((packed)) irq_info;

		/* Config Space */
		struct {
			uint16_t bus;
			uint8_t  dev;
			uint8_t  func;
			uint16_t offset;
			uint32_t cfg_value;
			uint8_t  cfg_width;
		} __attribute__((packed)) config;

		uint8_t raw[64];
	};
} __attribute__((packed));

#define TRACE_EVENT_SIZE  sizeof(struct nvme_trace_event)

/* ================================================================
 * Lookup tables
 * ================================================================ */

struct opcode_entry {
	uint8_t     opcode;
	const char *name;
};

struct status_entry {
	uint16_t    code;
	const char *name;
};

struct reg_entry {
	uint32_t    offset;
	const char *name;
	uint32_t    width;
};

struct event_type_entry {
	uint16_t    type;
	const char *name;
};

/* ================================================================
 * Decoder API
 * ================================================================ */

/* Opcode decoding */
const char *decode_io_opcode(uint8_t opcode);
const char *decode_admin_opcode(uint8_t opcode);
const char *decode_opcode(uint32_t qid, uint8_t opcode);

/* Status decoding */
const char *decode_status(uint16_t status_field);

/* Register/doorbell decoding */
const char *decode_bar0_reg(uint64_t offset);
int         decode_doorbell_qid(uint64_t offset, uint32_t db_stride);

/* Event type/direction decoding */
const char *decode_event_type(uint16_t type);
const char *decode_direction(uint16_t dir);
const char *decode_dma_direction(uint32_t dir);

/* Formatted output (writes to buf, returns bytes written) */
int format_sq_submit(const struct nvme_trace_event *evt, char *buf, size_t buflen, int use_color);
int format_cq_complete(const struct nvme_trace_event *evt, char *buf, size_t buflen, int use_color);
int format_doorbell(const struct nvme_trace_event *evt, char *buf, size_t buflen, int use_color);
int format_irq(const struct nvme_trace_event *evt, char *buf, size_t buflen, int use_color);
int format_mmio(const struct nvme_trace_event *evt, char *buf, size_t buflen, int use_color);
int format_dma(const struct nvme_trace_event *evt, char *buf, size_t buflen, int use_color);
int format_event(const struct nvme_trace_event *evt, char *buf, size_t buflen, int use_color);

#endif /* NVME_TRACE_DECODER_H */
