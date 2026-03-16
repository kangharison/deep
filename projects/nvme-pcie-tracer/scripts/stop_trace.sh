#!/bin/bash
#
# stop_trace.sh - Stop NVMe PCIe tracing
#
# Stops the reader process, disables tracing, shows summary statistics,
# and unloads the kernel module.
#
# Usage: sudo ./stop_trace.sh
#

set -euo pipefail

DEBUGFS_BASE="/sys/kernel/debug/nvme-pcie-tracer"
PID_FILE="/tmp/nvme_trace_reader.pid"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

# ---- Root check ----
if [ "$(id -u)" -ne 0 ]; then
    error "This script must be run as root"
    exit 1
fi

echo "============================================"
echo "  Stopping nvme-pcie-tracer"
echo "============================================"
echo ""

# ---- Stop background reader ----
if [ -f "$PID_FILE" ]; then
    READER_PID=$(cat "$PID_FILE")
    if kill -0 "$READER_PID" 2>/dev/null; then
        info "Stopping reader process (PID $READER_PID)..."
        kill -TERM "$READER_PID" 2>/dev/null || true
        sleep 1
        if kill -0 "$READER_PID" 2>/dev/null; then
            warn "Reader still running, sending SIGKILL..."
            kill -9 "$READER_PID" 2>/dev/null || true
        fi
        info "Reader stopped"
    else
        info "Reader process (PID $READER_PID) not running"
    fi
    rm -f "$PID_FILE"
fi

# Also kill any remaining reader processes
pkill -f "nvme_trace_reader" 2>/dev/null || true

# ---- Disable tracing ----
if [ -d "$DEBUGFS_BASE" ]; then
    info "Disabling tracing..."
    echo 0 > "${DEBUGFS_BASE}/enabled" 2>/dev/null || true

    # ---- Show statistics ----
    echo ""
    echo "--- Trace Statistics ---"
    if [ -f "${DEBUGFS_BASE}/stats" ]; then
        cat "${DEBUGFS_BASE}/stats"
    else
        warn "No statistics file found"
    fi
    echo ""
else
    warn "debugfs directory not found (module may not be loaded)"
fi

# ---- Unload kernel module ----
if lsmod | grep -q nvme_pcie_tracer; then
    info "Unloading nvme_pcie_tracer module..."
    rmmod nvme_pcie_tracer
    info "Module unloaded"
else
    info "nvme_pcie_tracer module not loaded"
fi

# ---- Check for saved trace files ----
echo ""
TRACE_FILES=$(ls -t /tmp/nvme_trace_*.log 2>/dev/null | head -5)
if [ -n "$TRACE_FILES" ]; then
    echo "Recent trace files:"
    for f in $TRACE_FILES; do
        SIZE=$(du -h "$f" | awk '{print $1}')
        LINES=$(wc -l < "$f")
        echo "  $f  (${SIZE}, ${LINES} lines)"
    done
    echo ""
    echo "To analyze:"
    LATEST=$(echo "$TRACE_FILES" | head -1)
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    echo "  python3 ${SCRIPT_DIR}/analyze.py $LATEST"
fi

echo ""
info "Tracing stopped"
