/*
 * nvme_trace_decoder.c - NVMe command/completion decoder for nvme-pcie-tracer
 *
 * Decodes NVMe SQE, CQE, MMIO register, and doorbell events into
 * human-readable text output.
 *
 * Copyright (c) 2026
 * License: GPL-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "nvme_trace_decoder.h"

/* ----------------------------------------------------------------
 * NVMe I/O opcode table
 * ---------------------------------------------------------------- */
static const struct opcode_entry io_opcodes[] = {
	{ 0x00, "Flush" },
	{ 0x01, "Write" },
	{ 0x02, "Read" },
	{ 0x04, "Write Uncorrectable" },
	{ 0x05, "Compare" },
	{ 0x08, "Write Zeroes" },
	{ 0x09, "Dataset Management" },
	{ 0x0c, "Verify" },
	{ 0x0d, "Reservation Register" },
	{ 0x0e, "Reservation Report" },
	{ 0x11, "Reservation Acquire" },
	{ 0x12, "IO Management Recv" },
	{ 0x15, "Reservation Release" },
	{ 0x79, "Zone Mgmt Send" },
	{ 0x7a, "Zone Mgmt Recv" },
	{ 0x7d, "Zone Append" },
	{ 0, NULL },
};

/* ----------------------------------------------------------------
 * NVMe Admin opcode table
 * ---------------------------------------------------------------- */
static const struct opcode_entry admin_opcodes[] = {
	{ 0x00, "Delete I/O SQ" },
	{ 0x01, "Create I/O SQ" },
	{ 0x02, "Get Log Page" },
	{ 0x04, "Delete I/O CQ" },
	{ 0x05, "Create I/O CQ" },
	{ 0x06, "Identify" },
	{ 0x08, "Abort" },
	{ 0x09, "Set Features" },
	{ 0x0a, "Get Features" },
	{ 0x0c, "Async Event Request" },
	{ 0x0d, "NS Management" },
	{ 0x10, "Firmware Commit" },
	{ 0x11, "Firmware Download" },
	{ 0x14, "Device Self-Test" },
	{ 0x15, "NS Attachment" },
	{ 0x18, "Keep Alive" },
	{ 0x19, "Directive Send" },
	{ 0x1a, "Directive Recv" },
	{ 0x1c, "Virtualization Mgmt" },
	{ 0x1d, "NVMe-MI Send" },
	{ 0x1e, "NVMe-MI Recv" },
	{ 0x7c, "Doorbell Buffer Config" },
	{ 0x80, "Format NVM" },
	{ 0x81, "Security Send" },
	{ 0x82, "Security Recv" },
	{ 0x84, "Sanitize" },
	{ 0x86, "Get LBA Status" },
	{ 0, NULL },
};

/* ----------------------------------------------------------------
 * NVMe status code table (Generic Command Status)
 * ---------------------------------------------------------------- */
static const struct status_entry status_codes[] = {
	{ 0x0000, "SUCCESS" },
	{ 0x0001, "INVALID_OPCODE" },
	{ 0x0002, "INVALID_FIELD" },
	{ 0x0003, "CMDID_CONFLICT" },
	{ 0x0004, "DATA_XFER_ERROR" },
	{ 0x0005, "POWER_LOSS" },
	{ 0x0006, "INTERNAL" },
	{ 0x0007, "ABORT_REQ" },
	{ 0x0008, "ABORT_QUEUE" },
	{ 0x0009, "FUSED_FAIL" },
	{ 0x000a, "FUSED_MISSING" },
	{ 0x000b, "INVALID_NS" },
	{ 0x000c, "CMD_SEQ_ERROR" },
	{ 0, NULL },
};

/* ----------------------------------------------------------------
 * BAR0 register offset table
 * ---------------------------------------------------------------- */
static const struct reg_entry bar0_regs[] = {
	{ 0x0000, "CAP",    8 },
	{ 0x0008, "VS",     4 },
	{ 0x000c, "INTMS",  4 },
	{ 0x0010, "INTMC",  4 },
	{ 0x0014, "CC",     4 },
	{ 0x001c, "CSTS",   4 },
	{ 0x0020, "NSSR",   4 },
	{ 0x0024, "AQA",    4 },
	{ 0x0028, "ASQ",    8 },
	{ 0x0030, "ACQ",    8 },
	{ 0x0038, "CMBLOC", 4 },
	{ 0x003c, "CMBSZ",  4 },
	{ 0x0050, "CMBMSC", 8 },
	{ 0x0068, "CRTO",   4 },
	{ 0, NULL, 0 },
};

/* ----------------------------------------------------------------
 * Event type name table
 * ---------------------------------------------------------------- */
static const struct event_type_entry event_types[] = {
	{ EVT_MMIO_READ,        "MMIO_READ" },
	{ EVT_MMIO_WRITE,       "MMIO_WRITE" },
	{ EVT_DOORBELL_SQ_TAIL, "SQ_TAIL" },
	{ EVT_DOORBELL_CQ_HEAD, "CQ_HEAD" },
	{ EVT_CONFIG_READ,      "CFG_READ" },
	{ EVT_CONFIG_WRITE,     "CFG_WRITE" },
	{ EVT_SQ_SUBMIT,        "SUBMIT" },
	{ EVT_CQ_COMPLETE,      "COMPLETE" },
	{ EVT_IRQ_FIRED,        "MSI-X" },
	{ EVT_SQ_HEAD_UPDATE,   "SQ_HEAD" },
	{ EVT_DMA_MAP,          "DMA_MAP" },
	{ EVT_DMA_UNMAP,        "DMA_UNMAP" },
	{ EVT_QUEUE_CREATE,     "Q_CREATE" },
	{ EVT_QUEUE_DELETE,     "Q_DELETE" },
	{ EVT_RESET,            "RESET" },
	{ 0, NULL },
};

/* ================================================================
 * Lookup helpers
 * ================================================================ */

const char *decode_io_opcode(uint8_t opcode)
{
	for (const struct opcode_entry *e = io_opcodes; e->name; e++) {
		if (e->opcode == opcode)
			return e->name;
	}
	return "Unknown";
}

const char *decode_admin_opcode(uint8_t opcode)
{
	for (const struct opcode_entry *e = admin_opcodes; e->name; e++) {
		if (e->opcode == opcode)
			return e->name;
	}
	return "Unknown";
}

const char *decode_opcode(uint32_t qid, uint8_t opcode)
{
	if (qid == 0)
		return decode_admin_opcode(opcode);
	return decode_io_opcode(opcode);
}

const char *decode_status(uint16_t status_field)
{
	/* status_field bits [15:1] contain status, bit [0] is phase */
	uint16_t sc = (status_field >> 1) & 0x7ff;
	uint8_t sct = (sc >> 8) & 0x7;
	uint8_t code = sc & 0xff;

	if (sct != 0)
		return "VENDOR/MEDIA_ERROR";

	for (const struct status_entry *e = status_codes; e->name; e++) {
		if (e->code == code)
			return e->name;
	}
	return "UNKNOWN_STATUS";
}

const char *decode_bar0_reg(uint64_t offset)
{
	/* Check if it's a doorbell register (>= 0x1000) */
	if (offset >= 0x1000)
		return "DOORBELL";

	for (const struct reg_entry *e = bar0_regs; e->name; e++) {
		if (e->offset == offset)
			return e->name;
	}
	return "UNKNOWN_REG";
}

int decode_doorbell_qid(uint64_t offset, uint32_t db_stride)
{
	if (offset < 0x1000)
		return -1;
	/*
	 * Doorbell layout: BAR0 + 0x1000 + (2 * qid * db_stride)  = SQ Tail
	 *                  BAR0 + 0x1000 + (2 * qid + 1) * db_stride = CQ Head
	 * db_stride = 4 << dstrd (typically 4 bytes for dstrd=0)
	 */
	if (db_stride == 0)
		db_stride = 4;
	uint64_t idx = (offset - 0x1000) / db_stride;
	return (int)(idx / 2);
}

const char *decode_event_type(uint16_t type)
{
	for (const struct event_type_entry *e = event_types; e->name; e++) {
		if (e->type == type)
			return e->name;
	}
	return "UNKNOWN";
}

const char *decode_direction(uint16_t dir)
{
	switch (dir) {
	case DIR_HOST_TO_DEVICE: return "H\xe2\x86\x92D";
	case DIR_DEVICE_TO_HOST: return "D\xe2\x86\x92H";
	case DIR_INTERNAL:       return "INT ";
	default:                 return "??? ";
	}
}

const char *decode_dma_direction(uint32_t dir)
{
	switch (dir) {
	case 0: return "BIDIRECTIONAL";
	case 1: return "TO_DEVICE";
	case 2: return "FROM_DEVICE";
	case 3: return "NONE";
	default: return "UNKNOWN";
	}
}

/* ================================================================
 * Formatted event output
 * ================================================================ */

/* ANSI color codes */
#define COLOR_RESET  "\033[0m"
#define COLOR_BLUE   "\033[1;34m"   /* H->D */
#define COLOR_GREEN  "\033[1;32m"   /* D->H */
#define COLOR_YELLOW "\033[1;33m"   /* Internal */
#define COLOR_RED    "\033[1;31m"   /* Error */
#define COLOR_CYAN   "\033[0;36m"   /* Latency */

static const char *dir_color(uint16_t direction)
{
	switch (direction) {
	case DIR_HOST_TO_DEVICE: return COLOR_BLUE;
	case DIR_DEVICE_TO_HOST: return COLOR_GREEN;
	case DIR_INTERNAL:       return COLOR_YELLOW;
	default:                 return "";
	}
}

/*
 * Format a timestamp in seconds.nanoseconds from a nanosecond value.
 */
static void format_timestamp(uint64_t ts_ns, char *buf, size_t buflen)
{
	uint64_t sec = ts_ns / 1000000000ULL;
	uint64_t nsec = ts_ns % 1000000000ULL;
	snprintf(buf, buflen, "%5lu.%09lu", (unsigned long)sec, (unsigned long)nsec);
}

int format_sq_submit(const struct nvme_trace_event *evt, char *buf,
		     size_t buflen, int use_color)
{
	char ts[32];
	format_timestamp(evt->timestamp_ns, ts, sizeof(ts));

	uint8_t opcode = evt->cmd_bytes[0]; /* common.opcode is first byte */
	uint32_t nsid;
	memcpy(&nsid, &evt->cmd_bytes[4], 4); /* common.nsid at offset 4 */
	uint16_t cid;
	memcpy(&cid, &evt->cmd_bytes[2], 2); /* common.command_id at offset 2 */

	const char *opname = decode_opcode(evt->queue_id, opcode);
	const char *c_on = use_color ? COLOR_BLUE : "";
	const char *c_off = use_color ? COLOR_RESET : "";

	int n = snprintf(buf, buflen,
		"%s[%s] C%02u  H%sD  SQ[%u]  SUBMIT    opcode=%s(0x%02x) nsid=%u cid=%u%s",
		c_on, ts, evt->cpu,
		use_color ? "\xe2\x86\x92" : "->",
		evt->queue_id, opname, opcode, nsid, cid, c_off);

	/* For Read/Write, add SLBA, NLB, PRP1 */
	if (opcode == 0x01 || opcode == 0x02) {
		uint64_t slba;
		uint16_t nlb;
		uint64_t prp1;
		memcpy(&slba, &evt->cmd_bytes[40], 8); /* rw.slba at offset 40 */
		memcpy(&nlb, &evt->cmd_bytes[48], 2);  /* rw.length at offset 48 */
		memcpy(&prp1, &evt->cmd_bytes[24], 8); /* dptr.prp1 at offset 24 */
		uint32_t kb = ((uint32_t)nlb + 1) * 512 / 1024;
		if (kb == 0) kb = 1;
		n += snprintf(buf + n, buflen - n,
			"\n                                              slba=0x%08lx nlb=%u (%uKB)"
			"\n                                              prp1=0x%016lx",
			(unsigned long)slba, nlb, kb, (unsigned long)prp1);
	}

	return n;
}

int format_cq_complete(const struct nvme_trace_event *evt, char *buf,
		       size_t buflen, int use_color)
{
	char ts[32];
	format_timestamp(evt->timestamp_ns, ts, sizeof(ts));

	uint16_t sq_head, sq_id, command_id, status;
	memcpy(&sq_head, &evt->cqe_bytes[8], 2);
	memcpy(&sq_id, &evt->cqe_bytes[10], 2);
	memcpy(&command_id, &evt->cqe_bytes[12], 2);
	memcpy(&status, &evt->cqe_bytes[14], 2);

	uint8_t phase = status & 0x1;
	const char *status_str = decode_status(status);
	uint16_t sc = (status >> 1) & 0x7ff;

	const char *c_on = use_color ? COLOR_GREEN : "";
	const char *c_off = use_color ? COLOR_RESET : "";
	const char *c_err = "";
	if (sc != 0 && use_color)
		c_on = COLOR_RED;

	return snprintf(buf, buflen,
		"%s[%s] C%02u  D%sH  CQ[%u]  COMPLETE  cid=%u status=%s(0x%04x)"
		"\n                                              sqhd=%u sqid=%u phase=%u%s",
		c_on, ts, evt->cpu,
		use_color ? "\xe2\x86\x92" : "->",
		evt->queue_id, command_id, status_str, sc,
		sq_head, sq_id, phase, c_off);
}

int format_doorbell(const struct nvme_trace_event *evt, char *buf,
		    size_t buflen, int use_color)
{
	char ts[32];
	format_timestamp(evt->timestamp_ns, ts, sizeof(ts));

	const char *c_on = use_color ? dir_color(evt->direction) : "";
	const char *c_off = use_color ? COLOR_RESET : "";
	const char *dir_str = (evt->direction == DIR_HOST_TO_DEVICE) ?
		(use_color ? "H\xe2\x86\x92D" : "H->D") :
		(use_color ? "D\xe2\x86\x92H" : "D->H");

	const char *db_type = (evt->event_type == EVT_DOORBELL_SQ_TAIL) ?
		"SQ_TAIL" : "CQ_HEAD";
	const char *val_name = (evt->event_type == EVT_DOORBELL_SQ_TAIL) ?
		"tail" : "head";

	return snprintf(buf, buflen,
		"%s[%s] C%02u  %s  DB[%u]  %s     %s=%u (old=%u)%s",
		c_on, ts, evt->cpu, dir_str,
		evt->queue_id, db_type, val_name,
		evt->db.new_value, evt->db.old_value, c_off);
}

int format_irq(const struct nvme_trace_event *evt, char *buf,
	       size_t buflen, int use_color)
{
	char ts[32];
	format_timestamp(evt->timestamp_ns, ts, sizeof(ts));

	const char *c_on = use_color ? COLOR_GREEN : "";
	const char *c_off = use_color ? COLOR_RESET : "";

	return snprintf(buf, buflen,
		"%s[%s] C%02u  D%sH  IRQ    MSI-X     vector=%u irq=%u%s",
		c_on, ts, evt->cpu,
		use_color ? "\xe2\x86\x92" : "->",
		evt->irq_info.vector, evt->irq_info.irq_num, c_off);
}

int format_mmio(const struct nvme_trace_event *evt, char *buf,
		size_t buflen, int use_color)
{
	char ts[32];
	format_timestamp(evt->timestamp_ns, ts, sizeof(ts));

	const char *c_on = use_color ? COLOR_BLUE : "";
	const char *c_off = use_color ? COLOR_RESET : "";
	const char *rw = (evt->event_type == EVT_MMIO_READ) ? "READ" : "WRITE";
	const char *regname = decode_bar0_reg(evt->mmio_info.bar_offset);

	return snprintf(buf, buflen,
		"%s[%s] C%02u  H%sD  MMIO   %-9s %s(0x%04lx) = 0x%lx%s",
		c_on, ts, evt->cpu,
		use_color ? "\xe2\x86\x92" : "->",
		rw, regname, (unsigned long)evt->mmio_info.bar_offset,
		(unsigned long)evt->mmio_info.value, c_off);
}

int format_dma(const struct nvme_trace_event *evt, char *buf,
	       size_t buflen, int use_color)
{
	char ts[32];
	format_timestamp(evt->timestamp_ns, ts, sizeof(ts));

	const char *c_on = use_color ? COLOR_CYAN : "";
	const char *c_off = use_color ? COLOR_RESET : "";
	const char *map_str = (evt->event_type == EVT_DMA_MAP) ? "DMA_MAP" : "DMA_UNMAP";
	const char *dir = decode_dma_direction(evt->dma_info.dma_direction);

	return snprintf(buf, buflen,
		"%s[%s] C%02u  D%sH  DMA    %-9s iova=0x%lx phys=0x%lx len=%u dir=%s nents=%u%s",
		c_on, ts, evt->cpu,
		use_color ? "\xe2\x86\x92" : "->",
		map_str, (unsigned long)evt->dma_info.iova,
		(unsigned long)evt->dma_info.phys_addr,
		evt->dma_info.length, dir, evt->dma_info.nents, c_off);
}

int format_event(const struct nvme_trace_event *evt, char *buf,
		 size_t buflen, int use_color)
{
	switch (evt->event_type) {
	case EVT_SQ_SUBMIT:
		return format_sq_submit(evt, buf, buflen, use_color);
	case EVT_CQ_COMPLETE:
		return format_cq_complete(evt, buf, buflen, use_color);
	case EVT_DOORBELL_SQ_TAIL:
	case EVT_DOORBELL_CQ_HEAD:
		return format_doorbell(evt, buf, buflen, use_color);
	case EVT_IRQ_FIRED:
		return format_irq(evt, buf, buflen, use_color);
	case EVT_MMIO_READ:
	case EVT_MMIO_WRITE:
		return format_mmio(evt, buf, buflen, use_color);
	case EVT_DMA_MAP:
	case EVT_DMA_UNMAP:
		return format_dma(evt, buf, buflen, use_color);
	default: {
		char ts[32];
		format_timestamp(evt->timestamp_ns, ts, sizeof(ts));
		return snprintf(buf, buflen,
			"[%s] C%02u  %s  Q[%u]  %s",
			ts, evt->cpu,
			decode_direction(evt->direction),
			evt->queue_id,
			decode_event_type(evt->event_type));
	}
	}
}
