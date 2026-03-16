#!/bin/bash
#
# start_trace.sh - Start NVMe PCIe tracing
#
# Loads the kernel module, enables tracing, and optionally starts
# the userspace reader in the background.
#
# Usage:
#   sudo ./start_trace.sh [OPTIONS] [PCI_BDF]
#
# Options:
#   -b SIZE    Per-CPU buffer size in KB (default: 4096)
#   -q QID     Filter specific queue ID
#   -r         Start reader in foreground
#   -R         Start reader in background (output to /tmp/nvme_trace.log)
#   -o FILE    Reader output file (implies -R)
#   -m MODE    Tracing mode: all, sq, cq, irq, doorbell (default: all)
#   -h         Show help
#
# If PCI_BDF is "auto" or not specified, auto-detects first NVMe device.
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
MODULE_PATH="${PROJECT_DIR}/kernel/nvme_pcie_tracer.ko"
READER_PATH="${PROJECT_DIR}/user/nvme_trace_reader"
DEBUGFS_BASE="/sys/kernel/debug/nvme-pcie-tracer"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

# Defaults
BUFFER_SIZE=4096
FILTER_QID=-1
START_READER=0
READER_BG=0
OUTPUT_FILE=""
TRACE_MODE="all"
PCI_BDF="auto"

usage() {
    echo "Usage: $0 [OPTIONS] [PCI_BDF]"
    echo ""
    echo "Options:"
    echo "  -b SIZE    Per-CPU buffer size in KB (default: 4096)"
    echo "  -q QID     Filter specific queue ID"
    echo "  -r         Start reader in foreground"
    echo "  -R         Start reader in background"
    echo "  -o FILE    Reader output file (implies background reader)"
    echo "  -m MODE    Tracing mode: all, sq, cq, irq, doorbell (default: all)"
    echo "  -h         Show this help"
    echo ""
    echo "PCI_BDF format: 0000:03:00.0 (use 'auto' for auto-detection)"
    exit 0
}

while getopts "b:q:rRo:m:h" opt; do
    case $opt in
        b) BUFFER_SIZE="$OPTARG" ;;
        q) FILTER_QID="$OPTARG" ;;
        r) START_READER=1; READER_BG=0 ;;
        R) START_READER=1; READER_BG=1 ;;
        o) OUTPUT_FILE="$OPTARG"; START_READER=1; READER_BG=1 ;;
        m) TRACE_MODE="$OPTARG" ;;
        h) usage ;;
        *) usage ;;
    esac
done
shift $((OPTIND-1))

# PCI BDF from positional argument
if [ $# -ge 1 ]; then
    PCI_BDF="$1"
fi

# ---- Root check ----
if [ "$(id -u)" -ne 0 ]; then
    error "This script must be run as root"
fi

# ---- Auto-detect NVMe device ----
if [ "$PCI_BDF" = "auto" ]; then
    info "Auto-detecting NVMe device..."
    PCI_BDF=$(lspci -D -d ::0108 | head -1 | awk '{print $1}')
    if [ -z "$PCI_BDF" ]; then
        error "No NVMe device found. Specify PCI BDF manually."
    fi
    info "Detected NVMe device: $PCI_BDF"
fi

# ---- Validate PCI BDF ----
if ! lspci -s "${PCI_BDF#*:}" &>/dev/null; then
    error "PCI device $PCI_BDF not found"
fi

# ---- Check if module already loaded ----
if lsmod | grep -q nvme_pcie_tracer; then
    warn "nvme_pcie_tracer module already loaded, unloading first..."
    echo 0 > "${DEBUGFS_BASE}/enabled" 2>/dev/null || true
    rmmod nvme_pcie_tracer 2>/dev/null || true
    sleep 1
fi

# ---- Ensure debugfs is mounted ----
if ! mountpoint -q /sys/kernel/debug 2>/dev/null; then
    info "Mounting debugfs..."
    mount -t debugfs none /sys/kernel/debug
fi

# ---- Load kernel module ----
info "Loading nvme_pcie_tracer module..."
info "  target_pci=$PCI_BDF"
info "  buffer_size_kb=$BUFFER_SIZE"

if [ ! -f "$MODULE_PATH" ]; then
    warn "Module not found at $MODULE_PATH"
    info "Trying modprobe..."
    modprobe nvme_pcie_tracer target_pci="$PCI_BDF" buffer_size_kb="$BUFFER_SIZE" || \
        error "Failed to load kernel module. Build it first with: make -C kernel"
else
    insmod "$MODULE_PATH" target_pci="$PCI_BDF" buffer_size_kb="$BUFFER_SIZE" || \
        error "Failed to load kernel module"
fi

info "Module loaded successfully"

# ---- Configure filters ----
if [ "$FILTER_QID" -ge 0 ] 2>/dev/null; then
    info "Setting queue filter: qid=$FILTER_QID"
    echo "$FILTER_QID" > "${DEBUGFS_BASE}/filter_qid" 2>/dev/null || true
fi

# ---- Enable tracing ----
info "Enabling tracing (mode: $TRACE_MODE)..."
echo 1 > "${DEBUGFS_BASE}/enabled"
info "Tracing enabled"

# ---- Show module info ----
echo ""
echo "Tracing active:"
echo "  Device:      $PCI_BDF"
echo "  Buffer:      ${BUFFER_SIZE}KB per CPU"
echo "  Mode:        $TRACE_MODE"
echo "  debugfs:     $DEBUGFS_BASE"
if [ -f "${DEBUGFS_BASE}/stats" ]; then
    echo "  Stats:       $(cat "${DEBUGFS_BASE}/stats" 2>/dev/null)"
fi
echo ""

# ---- Start reader if requested ----
if [ "$START_READER" -eq 1 ]; then
    if [ ! -x "$READER_PATH" ]; then
        warn "Reader not found at $READER_PATH, building..."
        make -C "${PROJECT_DIR}/user" 2>/dev/null || {
            error "Failed to build reader"
        }
    fi

    READER_ARGS="-f"
    if [ "$FILTER_QID" -ge 0 ] 2>/dev/null; then
        READER_ARGS="$READER_ARGS -q $FILTER_QID"
    fi

    if [ "$READER_BG" -eq 1 ]; then
        if [ -z "$OUTPUT_FILE" ]; then
            OUTPUT_FILE="/tmp/nvme_trace_$(date +%Y%m%d_%H%M%S).log"
        fi
        READER_ARGS="$READER_ARGS -o $OUTPUT_FILE -n"
        info "Starting reader in background..."
        info "  Output: $OUTPUT_FILE"
        nohup "$READER_PATH" $READER_ARGS &>/dev/null &
        READER_PID=$!
        echo "$READER_PID" > /tmp/nvme_trace_reader.pid
        info "Reader PID: $READER_PID"
    else
        info "Starting reader in foreground (Ctrl+C to stop)..."
        "$READER_PATH" $READER_ARGS
    fi
else
    echo "To read trace events:"
    echo "  sudo ${READER_PATH} -f"
    echo ""
    echo "Or use bpftrace (no kernel module needed):"
    echo "  sudo bpftrace ${PROJECT_DIR}/bpf/nvme_tracer.bt"
    echo ""
    echo "To stop tracing:"
    echo "  sudo ${SCRIPT_DIR}/stop_trace.sh"
fi
