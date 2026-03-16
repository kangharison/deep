#!/usr/bin/env python3
"""
analyze.py - NVMe PCIe trace analysis tool

Reads trace output from nvme_trace_reader (text format) and calculates:
  - Per-queue IOPS and bandwidth
  - Submit-to-complete latency distribution (avg, min, max, p50, p99)
  - Per-opcode latency breakdown
  - Event timeline

Optionally generates matplotlib plots if available.

Usage:
    python3 analyze.py <trace_file> [OPTIONS]

Options:
    -o DIR      Output directory for plots (default: current directory)
    --text      Text-only output (no plots even if matplotlib is available)
    --summary   Print summary only (no per-event output)
    --csv FILE  Export statistics to CSV
"""

import sys
import os
import re
import argparse
from collections import defaultdict
import math

# Try to import matplotlib
try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# ----------------------------------------------------------------
# Trace event parser
# ----------------------------------------------------------------

# Regex patterns for parsing reader output lines
RE_SUBMIT = re.compile(
    r'\[(\d+\.\d+)\]\s+C(\d+)\s+H[-=]>D\s+SQ\[(\d+)\]\s+SUBMIT\s+'
    r'opcode=(\S+)\(0x([0-9a-fA-F]+)\)\s+nsid=(\d+)\s+cid=(\d+)'
)
RE_SUBMIT_RW = re.compile(
    r'slba=0x([0-9a-fA-F]+)\s+nlb=(\d+)\s+\((\d+)KB\)'
)

RE_COMPLETE = re.compile(
    r'\[(\d+\.\d+)\]\s+C(\d+)\s+D[-=]>H\s+CQ\[(\d+)\]\s+COMPLETE\s+'
    r'cid=(\d+)\s+status=(\S+)\(0x([0-9a-fA-F]+)\)'
)
RE_COMPLETE_DETAIL = re.compile(
    r'sqhd=(\d+)\s+sqid=(\d+)\s+phase=(\d+)'
)

RE_DOORBELL = re.compile(
    r'\[(\d+\.\d+)\]\s+C(\d+)\s+\S+\s+DB\[(\d+)\]\s+(SQ_TAIL|CQ_HEAD)\s+'
    r'(?:tail|head)=(\d+)\s+\(old=(\d+)\)'
)

RE_IRQ = re.compile(
    r'\[(\d+\.\d+)\]\s+C(\d+)\s+D[-=]>H\s+IRQ\s+MSI-X\s+'
    r'vector=(\d+)\s+irq=(\d+)'
)

RE_DMA = re.compile(
    r'\[(\d+\.\d+)\]\s+C(\d+)\s+\S+\s+DMA\s+(DMA_MAP|DMA_UNMAP)\s+'
    r'iova=0x([0-9a-fA-F]+).*?len=(\d+)\s+dir=(\S+)'
)


class TraceEvent:
    """Parsed trace event."""
    __slots__ = ['timestamp', 'cpu', 'event_type', 'queue_id',
                 'opcode', 'opcode_hex', 'nsid', 'cid',
                 'slba', 'nlb', 'kb',
                 'status', 'status_hex',
                 'sqhd', 'sqid', 'phase',
                 'db_type', 'new_val', 'old_val',
                 'vector', 'irq',
                 'iova', 'dma_len', 'dma_dir']

    def __init__(self):
        for attr in self.__slots__:
            setattr(self, attr, None)


def parse_trace_file(filepath):
    """Parse a trace output file and return list of TraceEvent objects."""
    events = []

    with open(filepath, 'r') as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        line = lines[i].rstrip()

        # SQ Submit
        m = RE_SUBMIT.search(line)
        if m:
            evt = TraceEvent()
            evt.event_type = 'SUBMIT'
            evt.timestamp = float(m.group(1))
            evt.cpu = int(m.group(2))
            evt.queue_id = int(m.group(3))
            evt.opcode = m.group(4)
            evt.opcode_hex = int(m.group(5), 16)
            evt.nsid = int(m.group(6))
            evt.cid = int(m.group(7))

            # Check next lines for SLBA/NLB details
            for j in range(1, 3):
                if i + j < len(lines):
                    mr = RE_SUBMIT_RW.search(lines[i + j])
                    if mr:
                        evt.slba = int(mr.group(1), 16)
                        evt.nlb = int(mr.group(2))
                        evt.kb = int(mr.group(3))
                        break

            events.append(evt)
            i += 1
            continue

        # CQ Complete
        m = RE_COMPLETE.search(line)
        if m:
            evt = TraceEvent()
            evt.event_type = 'COMPLETE'
            evt.timestamp = float(m.group(1))
            evt.cpu = int(m.group(2))
            evt.queue_id = int(m.group(3))
            evt.cid = int(m.group(4))
            evt.status = m.group(5)
            evt.status_hex = int(m.group(6), 16)

            # Check next line for sqhd/sqid/phase
            if i + 1 < len(lines):
                md = RE_COMPLETE_DETAIL.search(lines[i + 1])
                if md:
                    evt.sqhd = int(md.group(1))
                    evt.sqid = int(md.group(2))
                    evt.phase = int(md.group(3))

            events.append(evt)
            i += 1
            continue

        # Doorbell
        m = RE_DOORBELL.search(line)
        if m:
            evt = TraceEvent()
            evt.event_type = 'DOORBELL'
            evt.timestamp = float(m.group(1))
            evt.cpu = int(m.group(2))
            evt.queue_id = int(m.group(3))
            evt.db_type = m.group(4)
            evt.new_val = int(m.group(5))
            evt.old_val = int(m.group(6))
            events.append(evt)
            i += 1
            continue

        # IRQ
        m = RE_IRQ.search(line)
        if m:
            evt = TraceEvent()
            evt.event_type = 'IRQ'
            evt.timestamp = float(m.group(1))
            evt.cpu = int(m.group(2))
            evt.vector = int(m.group(3))
            evt.irq = int(m.group(4))
            events.append(evt)
            i += 1
            continue

        # DMA
        m = RE_DMA.search(line)
        if m:
            evt = TraceEvent()
            evt.event_type = 'DMA'
            evt.timestamp = float(m.group(1))
            evt.cpu = int(m.group(2))
            evt.db_type = m.group(3)
            evt.iova = int(m.group(4), 16)
            evt.dma_len = int(m.group(5))
            evt.dma_dir = m.group(6)
            events.append(evt)
            i += 1
            continue

        i += 1

    return events


# ----------------------------------------------------------------
# Analysis
# ----------------------------------------------------------------

class TraceAnalyzer:
    """Analyze parsed trace events."""

    def __init__(self, events):
        self.events = events
        self.submit_map = {}         # (qid, cid) -> TraceEvent
        self.latencies = defaultdict(list)         # qid -> [latency_us]
        self.opcode_latencies = defaultdict(list)  # opcode_hex -> [latency_us]
        self.queue_io_count = defaultdict(int)
        self.queue_bytes = defaultdict(int)
        self.opcode_counts = defaultdict(int)
        self.error_count = 0
        self.irq_count = 0
        self.doorbell_count = 0
        self.dma_count = 0

    def analyze(self):
        """Run all analysis passes."""
        self._calculate_latencies()
        self._calculate_stats()

    def _calculate_latencies(self):
        """Match submit/complete pairs by (qid, cid) and compute latency."""
        for evt in self.events:
            if evt.event_type == 'SUBMIT':
                key = (evt.queue_id, evt.cid)
                self.submit_map[key] = evt
                self.opcode_counts[evt.opcode_hex] = self.opcode_counts.get(evt.opcode_hex, 0) + 1
            elif evt.event_type == 'COMPLETE':
                key = (evt.queue_id, evt.cid)
                if key in self.submit_map:
                    sub = self.submit_map.pop(key)
                    latency_us = (evt.timestamp - sub.timestamp) * 1e6
                    if latency_us > 0:
                        self.latencies[evt.queue_id].append(latency_us)
                        if sub.opcode_hex is not None:
                            self.opcode_latencies[sub.opcode_hex].append(latency_us)
                    self.queue_io_count[evt.queue_id] += 1
                    if sub.kb:
                        self.queue_bytes[evt.queue_id] += sub.kb * 1024
                if evt.status_hex and evt.status_hex != 0:
                    self.error_count += 1

    def _calculate_stats(self):
        """Count IRQ, doorbell, DMA events."""
        for evt in self.events:
            if evt.event_type == 'IRQ':
                self.irq_count += 1
            elif evt.event_type == 'DOORBELL':
                self.doorbell_count += 1
            elif evt.event_type == 'DMA':
                self.dma_count += 1

    def get_time_range(self):
        """Return (start, end) timestamps."""
        timestamps = [e.timestamp for e in self.events if e.timestamp is not None]
        if not timestamps:
            return 0, 0
        return min(timestamps), max(timestamps)

    def print_summary(self, fp=sys.stdout):
        """Print analysis summary."""
        start, end = self.get_time_range()
        duration = end - start if end > start else 0

        fp.write("\n")
        fp.write("=" * 70 + "\n")
        fp.write("  NVMe PCIe Trace Analysis Summary\n")
        fp.write("=" * 70 + "\n\n")

        fp.write(f"Duration:        {duration:.6f} seconds\n")
        fp.write(f"Total events:    {len(self.events)}\n")
        fp.write(f"  Submits:       {sum(self.queue_io_count.values())}\n")
        fp.write(f"  Completions:   {sum(len(v) for v in self.latencies.values())}\n")
        fp.write(f"  IRQs:          {self.irq_count}\n")
        fp.write(f"  Doorbells:     {self.doorbell_count}\n")
        fp.write(f"  DMA ops:       {self.dma_count}\n")
        fp.write(f"  Errors:        {self.error_count}\n")
        fp.write(f"  Unmatched:     {len(self.submit_map)}\n\n")

    def print_latency_stats(self, fp=sys.stdout):
        """Print per-queue latency statistics."""
        fp.write("-" * 70 + "\n")
        fp.write("  Per-Queue Latency Statistics (microseconds)\n")
        fp.write("-" * 70 + "\n")
        fp.write(f"{'QID':>4s}  {'Count':>8s}  {'Avg':>10s}  {'Min':>10s}  "
                 f"{'Max':>10s}  {'P50':>10s}  {'P99':>10s}\n")
        fp.write("-" * 70 + "\n")

        for qid in sorted(self.latencies.keys()):
            lats = sorted(self.latencies[qid])
            n = len(lats)
            if n == 0:
                continue
            avg = sum(lats) / n
            p50 = lats[int(n * 0.50)]
            p99 = lats[min(int(n * 0.99), n - 1)]
            fp.write(f"{qid:4d}  {n:8d}  {avg:10.1f}  {lats[0]:10.1f}  "
                     f"{lats[-1]:10.1f}  {p50:10.1f}  {p99:10.1f}\n")
        fp.write("\n")

    def print_opcode_stats(self, fp=sys.stdout):
        """Print per-opcode latency statistics."""
        opcode_names = {
            0x00: "Flush", 0x01: "Write", 0x02: "Read",
            0x04: "WriteUncor", 0x05: "Compare",
            0x08: "WriteZeroes", 0x09: "DSM",
        }

        fp.write("-" * 70 + "\n")
        fp.write("  Per-Opcode Latency Statistics (microseconds)\n")
        fp.write("-" * 70 + "\n")
        fp.write(f"{'Opcode':>12s}  {'Count':>8s}  {'Avg':>10s}  {'Min':>10s}  "
                 f"{'Max':>10s}  {'P50':>10s}  {'P99':>10s}\n")
        fp.write("-" * 70 + "\n")

        for op in sorted(self.opcode_latencies.keys()):
            lats = sorted(self.opcode_latencies[op])
            n = len(lats)
            if n == 0:
                continue
            avg = sum(lats) / n
            p50 = lats[int(n * 0.50)]
            p99 = lats[min(int(n * 0.99), n - 1)]
            name = opcode_names.get(op, f"0x{op:02x}")
            fp.write(f"{name:>12s}  {n:8d}  {avg:10.1f}  {lats[0]:10.1f}  "
                     f"{lats[-1]:10.1f}  {p50:10.1f}  {p99:10.1f}\n")
        fp.write("\n")

    def print_iops_bandwidth(self, fp=sys.stdout):
        """Print per-queue IOPS and bandwidth."""
        start, end = self.get_time_range()
        duration = end - start if end > start else 1.0

        fp.write("-" * 70 + "\n")
        fp.write("  Per-Queue IOPS and Bandwidth\n")
        fp.write("-" * 70 + "\n")
        fp.write(f"{'QID':>4s}  {'I/Os':>8s}  {'IOPS':>10s}  {'BW (MB/s)':>12s}\n")
        fp.write("-" * 70 + "\n")

        total_ios = 0
        total_bytes = 0
        for qid in sorted(self.queue_io_count.keys()):
            ios = self.queue_io_count[qid]
            bw_bytes = self.queue_bytes.get(qid, 0)
            iops = ios / duration
            bw_mbps = bw_bytes / duration / (1024 * 1024)
            fp.write(f"{qid:4d}  {ios:8d}  {iops:10.1f}  {bw_mbps:12.2f}\n")
            total_ios += ios
            total_bytes += bw_bytes

        fp.write("-" * 70 + "\n")
        fp.write(f"{'Total':>4s}  {total_ios:8d}  {total_ios/duration:10.1f}  "
                 f"{total_bytes/duration/(1024*1024):12.2f}\n\n")

    def print_text_histogram(self, data, title, unit="us", bins=20, fp=sys.stdout):
        """Print ASCII histogram."""
        if not data:
            return

        data_sorted = sorted(data)
        lo = data_sorted[0]
        hi = data_sorted[-1]

        if hi == lo:
            fp.write(f"{title}: all values = {lo:.1f} {unit}\n")
            return

        fp.write(f"\n{title}\n")
        fp.write(f"  Range: {lo:.1f} - {hi:.1f} {unit}, N={len(data)}\n\n")

        # Use log-scale bins for latency
        if lo > 0:
            log_lo = math.log10(lo)
            log_hi = math.log10(hi)
            bin_edges = [10 ** (log_lo + i * (log_hi - log_lo) / bins)
                         for i in range(bins + 1)]
        else:
            step = (hi - lo) / bins
            bin_edges = [lo + i * step for i in range(bins + 1)]

        counts = [0] * bins
        for v in data:
            for b in range(bins):
                if v <= bin_edges[b + 1] or b == bins - 1:
                    counts[b] += 1
                    break

        max_count = max(counts) if counts else 1
        bar_width = 40

        for b in range(bins):
            if counts[b] == 0:
                continue
            lo_edge = bin_edges[b]
            hi_edge = bin_edges[b + 1]
            bar_len = int(counts[b] / max_count * bar_width)
            bar = '#' * bar_len
            fp.write(f"  [{lo_edge:10.1f}, {hi_edge:10.1f}) "
                     f"{counts[b]:6d} |{bar}\n")
        fp.write("\n")

    def plot_latency_histogram(self, output_dir='.'):
        """Generate latency histogram plot with matplotlib."""
        if not HAS_MATPLOTLIB:
            return

        all_lats = []
        for lats in self.latencies.values():
            all_lats.extend(lats)

        if not all_lats:
            return

        fig, ax = plt.subplots(figsize=(10, 6))
        ax.hist(all_lats, bins=50, edgecolor='black', alpha=0.7)
        ax.set_xlabel('Latency (us)')
        ax.set_ylabel('Count')
        ax.set_title('NVMe I/O Latency Distribution')
        ax.set_xscale('log')

        avg = sum(all_lats) / len(all_lats)
        ax.axvline(avg, color='red', linestyle='--', label=f'avg={avg:.1f}us')
        ax.legend()

        path = os.path.join(output_dir, 'latency_hist.png')
        plt.savefig(path, dpi=150, bbox_inches='tight')
        plt.close()
        print(f"  Saved: {path}")

    def plot_timeline(self, output_dir='.'):
        """Generate event timeline plot."""
        if not HAS_MATPLOTLIB:
            return

        submits = [(e.timestamp, e.queue_id) for e in self.events
                    if e.event_type == 'SUBMIT']
        completes = [(e.timestamp, e.queue_id) for e in self.events
                      if e.event_type == 'COMPLETE']
        irqs = [(e.timestamp, 0) for e in self.events
                if e.event_type == 'IRQ']

        if not submits and not completes:
            return

        fig, ax = plt.subplots(figsize=(14, 6))

        if submits:
            ts, qids = zip(*submits)
            ax.scatter(ts, qids, c='blue', s=2, alpha=0.5, label='Submit')
        if completes:
            ts, qids = zip(*completes)
            ax.scatter(ts, qids, c='green', s=2, alpha=0.5, label='Complete')
        if irqs:
            ts, _ = zip(*irqs)
            for t in ts[:1000]:  # limit IRQ markers
                ax.axvline(t, color='red', alpha=0.1, linewidth=0.5)

        ax.set_xlabel('Time (s)')
        ax.set_ylabel('Queue ID')
        ax.set_title('NVMe Event Timeline')
        ax.legend()

        path = os.path.join(output_dir, 'timeline.png')
        plt.savefig(path, dpi=150, bbox_inches='tight')
        plt.close()
        print(f"  Saved: {path}")

    def plot_queue_heatmap(self, output_dir='.'):
        """Generate per-queue latency heatmap."""
        if not HAS_MATPLOTLIB:
            return

        if not self.latencies:
            return

        fig, ax = plt.subplots(figsize=(10, 6))

        qids = sorted(self.latencies.keys())
        data = [self.latencies[q] for q in qids]

        ax.boxplot(data, labels=[str(q) for q in qids], whis=[1, 99])
        ax.set_xlabel('Queue ID')
        ax.set_ylabel('Latency (us)')
        ax.set_title('Per-Queue Latency Distribution')
        ax.set_yscale('log')

        path = os.path.join(output_dir, 'queue_latency.png')
        plt.savefig(path, dpi=150, bbox_inches='tight')
        plt.close()
        print(f"  Saved: {path}")

    def export_csv(self, filepath):
        """Export statistics to CSV."""
        with open(filepath, 'w') as f:
            f.write("qid,count,avg_us,min_us,max_us,p50_us,p99_us\n")
            for qid in sorted(self.latencies.keys()):
                lats = sorted(self.latencies[qid])
                n = len(lats)
                if n == 0:
                    continue
                avg = sum(lats) / n
                p50 = lats[int(n * 0.50)]
                p99 = lats[min(int(n * 0.99), n - 1)]
                f.write(f"{qid},{n},{avg:.1f},{lats[0]:.1f},"
                        f"{lats[-1]:.1f},{p50:.1f},{p99:.1f}\n")
        print(f"  CSV exported: {filepath}")


# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='NVMe PCIe Trace Analyzer')
    parser.add_argument('trace_file',
                        help='Trace output file from nvme_trace_reader')
    parser.add_argument('-o', '--output-dir', default='.',
                        help='Output directory for plots')
    parser.add_argument('--text', action='store_true',
                        help='Text-only output (no plots)')
    parser.add_argument('--summary', action='store_true',
                        help='Print summary only')
    parser.add_argument('--csv', default=None,
                        help='Export statistics to CSV file')
    args = parser.parse_args()

    if not os.path.exists(args.trace_file):
        print(f"Error: file not found: {args.trace_file}", file=sys.stderr)
        sys.exit(1)

    # Parse
    print(f"Parsing {args.trace_file}...")
    events = parse_trace_file(args.trace_file)
    print(f"Parsed {len(events)} events")

    if not events:
        print("No events found. Check the trace file format.")
        sys.exit(1)

    # Analyze
    analyzer = TraceAnalyzer(events)
    analyzer.analyze()

    # Output
    analyzer.print_summary()
    analyzer.print_latency_stats()
    analyzer.print_opcode_stats()
    analyzer.print_iops_bandwidth()

    # Text histograms
    all_lats = []
    for lats in analyzer.latencies.values():
        all_lats.extend(lats)
    analyzer.print_text_histogram(all_lats, "Overall Latency Histogram")

    for qid in sorted(analyzer.latencies.keys()):
        analyzer.print_text_histogram(
            analyzer.latencies[qid],
            f"Queue {qid} Latency Histogram")

    # Plots
    if not args.text and HAS_MATPLOTLIB:
        os.makedirs(args.output_dir, exist_ok=True)
        print("\nGenerating plots...")
        analyzer.plot_latency_histogram(args.output_dir)
        analyzer.plot_timeline(args.output_dir)
        analyzer.plot_queue_heatmap(args.output_dir)
    elif not args.text:
        print("\nmatplotlib not available, skipping plots."
              " Install with: pip3 install matplotlib")

    # CSV export
    if args.csv:
        analyzer.export_csv(args.csv)


if __name__ == '__main__':
    main()
