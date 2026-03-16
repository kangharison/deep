/*
 * nvme_trace_reader.c - Read and display NVMe PCIe trace events
 *
 * Reads trace events from the debugfs relay channel provided by the
 * nvme-pcie-tracer kernel module and displays them in human-readable format.
 *
 * Usage:
 *   nvme_trace_reader [-f] [-o file] [-q qid] [-t type] [-n] [-c ncpus]
 *
 * Options:
 *   -f          Follow mode (like tail -f)
 *   -o file     Save output to file
 *   -q qid      Filter by queue ID (-1 = all)
 *   -t type     Filter by event type name (e.g., SUBMIT, COMPLETE, IRQ)
 *   -n          No color output
 *   -c ncpus    Number of CPUs to monitor (default: auto-detect)
 *
 * Copyright (c) 2026
 * License: GPL-2.0
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <getopt.h>
#include <sys/stat.h>
#include <time.h>

#include "nvme_trace_decoder.h"

#define DEBUGFS_BASE   "/sys/kernel/debug/nvme-pcie-tracer"
#define TRACE_PREFIX   "trace"
#define MAX_CPUS       256
#define READ_BUF_SIZE  (64 * 1024)   /* 64KB read buffer */
#define FMT_BUF_SIZE   2048

static volatile int running = 1;

/* ================================================================
 * Configuration
 * ================================================================ */

struct reader_config {
	int     follow_mode;
	char   *output_file;
	int     filter_qid;       /* -1 = all queues */
	char   *filter_type;      /* NULL = all types */
	int     use_color;
	int     ncpus;
	FILE   *out_fp;
};

/* ================================================================
 * Signal handler
 * ================================================================ */

static void sig_handler(int sig)
{
	(void)sig;
	running = 0;
}

/* ================================================================
 * Helpers
 * ================================================================ */

static int detect_ncpus(void)
{
	long n = sysconf(_SC_NPROCESSORS_ONLN);
	if (n < 1) n = 1;
	if (n > MAX_CPUS) n = MAX_CPUS;
	return (int)n;
}

static int match_event_type_filter(const struct nvme_trace_event *evt,
				   const char *filter)
{
	if (!filter)
		return 1; /* no filter = match all */

	const char *evtname = decode_event_type(evt->event_type);

	/* Case-insensitive substring match */
	if (strcasestr(evtname, filter))
		return 1;

	/* Also check common aliases */
	if (strcasecmp(filter, "SUBMIT") == 0 && evt->event_type == EVT_SQ_SUBMIT)
		return 1;
	if (strcasecmp(filter, "COMPLETE") == 0 && evt->event_type == EVT_CQ_COMPLETE)
		return 1;
	if (strcasecmp(filter, "IRQ") == 0 && evt->event_type == EVT_IRQ_FIRED)
		return 1;
	if (strcasecmp(filter, "DOORBELL") == 0 &&
	    (evt->event_type == EVT_DOORBELL_SQ_TAIL ||
	     evt->event_type == EVT_DOORBELL_CQ_HEAD))
		return 1;
	if (strcasecmp(filter, "MMIO") == 0 &&
	    (evt->event_type == EVT_MMIO_READ ||
	     evt->event_type == EVT_MMIO_WRITE))
		return 1;
	if (strcasecmp(filter, "DMA") == 0 &&
	    (evt->event_type == EVT_DMA_MAP ||
	     evt->event_type == EVT_DMA_UNMAP))
		return 1;

	return 0;
}

static int apply_filters(const struct nvme_trace_event *evt,
			  const struct reader_config *cfg)
{
	if (cfg->filter_qid >= 0 && (int)evt->queue_id != cfg->filter_qid)
		return 0;
	if (!match_event_type_filter(evt, cfg->filter_type))
		return 0;
	return 1;
}

/* ================================================================
 * Statistics
 * ================================================================ */

struct trace_stats {
	uint64_t total_events;
	uint64_t filtered_events;
	uint64_t submit_events;
	uint64_t complete_events;
	uint64_t irq_events;
	uint64_t doorbell_events;
	uint64_t mmio_events;
	uint64_t dma_events;
	uint64_t other_events;
	uint64_t read_errors;
};

static void update_stats(struct trace_stats *stats,
			  const struct nvme_trace_event *evt)
{
	stats->total_events++;
	switch (evt->event_type) {
	case EVT_SQ_SUBMIT:        stats->submit_events++; break;
	case EVT_CQ_COMPLETE:      stats->complete_events++; break;
	case EVT_IRQ_FIRED:        stats->irq_events++; break;
	case EVT_DOORBELL_SQ_TAIL:
	case EVT_DOORBELL_CQ_HEAD: stats->doorbell_events++; break;
	case EVT_MMIO_READ:
	case EVT_MMIO_WRITE:       stats->mmio_events++; break;
	case EVT_DMA_MAP:
	case EVT_DMA_UNMAP:        stats->dma_events++; break;
	default:                   stats->other_events++; break;
	}
}

static void print_stats(const struct trace_stats *stats, FILE *fp)
{
	fprintf(fp, "\n--- Trace Statistics ---\n");
	fprintf(fp, "Total events:    %lu\n", (unsigned long)stats->total_events);
	fprintf(fp, "Displayed:       %lu\n", (unsigned long)stats->filtered_events);
	fprintf(fp, "  SQ Submit:     %lu\n", (unsigned long)stats->submit_events);
	fprintf(fp, "  CQ Complete:   %lu\n", (unsigned long)stats->complete_events);
	fprintf(fp, "  IRQ:           %lu\n", (unsigned long)stats->irq_events);
	fprintf(fp, "  Doorbell:      %lu\n", (unsigned long)stats->doorbell_events);
	fprintf(fp, "  MMIO:          %lu\n", (unsigned long)stats->mmio_events);
	fprintf(fp, "  DMA:           %lu\n", (unsigned long)stats->dma_events);
	fprintf(fp, "  Other:         %lu\n", (unsigned long)stats->other_events);
	fprintf(fp, "Read errors:     %lu\n", (unsigned long)stats->read_errors);
}

/* ================================================================
 * Main reader loop
 * ================================================================ */

static void process_buffer(const uint8_t *buf, ssize_t len,
			   struct reader_config *cfg,
			   struct trace_stats *stats)
{
	char fmtbuf[FMT_BUF_SIZE];
	ssize_t offset = 0;

	while (offset + (ssize_t)TRACE_EVENT_SIZE <= len) {
		const struct nvme_trace_event *evt =
			(const struct nvme_trace_event *)(buf + offset);
		offset += TRACE_EVENT_SIZE;

		/* Sanity: skip events with zero timestamp (padding) */
		if (evt->timestamp_ns == 0)
			continue;

		update_stats(stats, evt);

		if (!apply_filters(evt, cfg))
			continue;

		stats->filtered_events++;

		int n = format_event(evt, fmtbuf, sizeof(fmtbuf), cfg->use_color);
		if (n > 0) {
			fprintf(cfg->out_fp, "%s\n", fmtbuf);
			if (!cfg->follow_mode)
				fflush(cfg->out_fp);
		}
	}
}

static int open_trace_files(int *fds, int ncpus)
{
	int opened = 0;
	char path[512];

	for (int i = 0; i < ncpus; i++) {
		snprintf(path, sizeof(path), "%s/%s%d", DEBUGFS_BASE, TRACE_PREFIX, i);
		fds[i] = open(path, O_RDONLY | O_NONBLOCK);
		if (fds[i] >= 0)
			opened++;
		else
			fds[i] = -1;
	}
	return opened;
}

static void close_trace_files(int *fds, int ncpus)
{
	for (int i = 0; i < ncpus; i++) {
		if (fds[i] >= 0)
			close(fds[i]);
	}
}

static int reader_loop(struct reader_config *cfg)
{
	int fds[MAX_CPUS];
	struct pollfd pfds[MAX_CPUS];
	uint8_t readbuf[READ_BUF_SIZE];
	struct trace_stats stats = {0};
	int nfds = 0;

	memset(fds, -1, sizeof(fds));

	int opened = open_trace_files(fds, cfg->ncpus);
	if (opened == 0) {
		fprintf(stderr, "Error: Could not open any trace files at %s/\n",
			DEBUGFS_BASE);
		fprintf(stderr, "Make sure the nvme-pcie-tracer kernel module is loaded.\n");
		return 1;
	}

	fprintf(stderr, "Opened %d/%d trace channels\n", opened, cfg->ncpus);
	if (cfg->follow_mode)
		fprintf(stderr, "Follow mode enabled. Press Ctrl+C to stop.\n");

	/* Set up poll descriptors */
	for (int i = 0; i < cfg->ncpus; i++) {
		if (fds[i] >= 0) {
			pfds[nfds].fd = fds[i];
			pfds[nfds].events = POLLIN;
			nfds++;
		}
	}

	/* Print header */
	fprintf(cfg->out_fp,
		"%-20s %-4s %-5s %-6s %-10s %s\n",
		"[Timestamp]", "CPU", "Dir", "Queue", "Event", "Details");
	fprintf(cfg->out_fp,
		"%.78s\n",
		"------------------------------------------------------------------------------");
	fflush(cfg->out_fp);

	/* Main loop */
	while (running) {
		int ret = poll(pfds, nfds, cfg->follow_mode ? 100 : 0);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("poll");
			break;
		}

		int got_data = 0;
		for (int i = 0; i < nfds; i++) {
			if (pfds[i].revents & POLLIN) {
				ssize_t n = read(pfds[i].fd, readbuf, sizeof(readbuf));
				if (n > 0) {
					process_buffer(readbuf, n, cfg, &stats);
					got_data = 1;
				} else if (n < 0 && errno != EAGAIN) {
					stats.read_errors++;
				}
			}
		}

		/* In non-follow mode, exit when no more data */
		if (!cfg->follow_mode && !got_data)
			break;

		if (cfg->follow_mode && got_data)
			fflush(cfg->out_fp);
	}

	print_stats(&stats, stderr);
	close_trace_files(fds, cfg->ncpus);
	return 0;
}

/* ================================================================
 * Usage and main
 * ================================================================ */

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [OPTIONS]\n"
		"\n"
		"Read and display NVMe PCIe trace events from debugfs relay channel.\n"
		"\n"
		"Options:\n"
		"  -f          Follow mode (continuous reading, like tail -f)\n"
		"  -o FILE     Save output to file (default: stdout)\n"
		"  -q QID      Filter by queue ID (-1 = all, default: -1)\n"
		"  -t TYPE     Filter by event type (SUBMIT, COMPLETE, IRQ, DOORBELL, MMIO, DMA)\n"
		"  -n          No color output\n"
		"  -c NCPUS    Number of CPU trace channels to open (default: auto-detect)\n"
		"  -h          Show this help\n"
		"\n"
		"Examples:\n"
		"  %s -f                    # Follow all events with color\n"
		"  %s -f -q 1 -t SUBMIT    # Follow SQ submits on queue 1\n"
		"  %s -o trace.log -n      # Save all events to file, no color\n"
		"\n"
		"The kernel module must be loaded first:\n"
		"  sudo insmod nvme_pcie_tracer.ko target_pci=\"0000:03:00.0\"\n"
		"  echo 1 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled\n",
		prog, prog, prog, prog);
}

int main(int argc, char *argv[])
{
	struct reader_config cfg = {
		.follow_mode = 0,
		.output_file = NULL,
		.filter_qid  = -1,
		.filter_type = NULL,
		.use_color   = isatty(STDOUT_FILENO),
		.ncpus       = 0,
		.out_fp      = stdout,
	};

	int opt;
	while ((opt = getopt(argc, argv, "fo:q:t:nc:h")) != -1) {
		switch (opt) {
		case 'f':
			cfg.follow_mode = 1;
			break;
		case 'o':
			cfg.output_file = optarg;
			cfg.use_color = 0;  /* no color for file output */
			break;
		case 'q':
			cfg.filter_qid = atoi(optarg);
			break;
		case 't':
			cfg.filter_type = optarg;
			break;
		case 'n':
			cfg.use_color = 0;
			break;
		case 'c':
			cfg.ncpus = atoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (cfg.ncpus <= 0)
		cfg.ncpus = detect_ncpus();

	/* Open output file if specified */
	if (cfg.output_file) {
		cfg.out_fp = fopen(cfg.output_file, "w");
		if (!cfg.out_fp) {
			fprintf(stderr, "Error: cannot open output file '%s': %s\n",
				cfg.output_file, strerror(errno));
			return 1;
		}
	}

	/* Install signal handlers */
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	fprintf(stderr, "nvme-pcie-tracer reader starting...\n");
	fprintf(stderr, "  CPUs: %d, QID filter: %d, Type filter: %s\n",
		cfg.ncpus,
		cfg.filter_qid,
		cfg.filter_type ? cfg.filter_type : "(all)");

	int ret = reader_loop(&cfg);

	if (cfg.output_file && cfg.out_fp)
		fclose(cfg.out_fp);

	return ret;
}
