#!/bin/bash
#
# setup.sh - Environment setup for nvme-pcie-tracer
#
# Installs dependencies, checks system configuration, and prepares
# the environment for NVMe PCIe tracing on CentOS/Rocky Linux 9.
#
# Usage: sudo ./setup.sh
#

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        error "This script must be run as root"
        exit 1
    fi
}

check_distro() {
    info "Checking distribution..."
    if [ -f /etc/redhat-release ]; then
        cat /etc/redhat-release
    elif [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$PRETTY_NAME"
    else
        warn "Unknown distribution"
    fi
    echo "Kernel: $(uname -r)"
    echo "Arch:   $(uname -m)"
}

install_dependencies() {
    info "Installing build dependencies..."

    # Core build tools
    dnf install -y gcc make elfutils-libelf-devel

    # Kernel development headers
    local kver
    kver=$(uname -r)
    if ! rpm -q "kernel-devel-${kver}" &>/dev/null; then
        info "Installing kernel-devel for ${kver}..."
        dnf install -y "kernel-devel-${kver}" || {
            warn "Could not install kernel-devel-${kver}, trying generic..."
            dnf install -y kernel-devel
        }
    else
        info "kernel-devel-${kver} already installed"
    fi

    # BPF tools (optional but recommended)
    info "Installing BPF tools..."
    dnf install -y bpftrace bcc-tools || {
        warn "BPF tools installation failed (optional)"
    }

    # Python analysis tools (optional)
    info "Installing Python analysis tools..."
    dnf install -y python3 python3-pip || true
    pip3 install matplotlib numpy 2>/dev/null || {
        warn "matplotlib/numpy installation failed (optional, text histograms will be used)"
    }

    info "Dependencies installed successfully"
}

mount_debugfs() {
    info "Checking debugfs..."
    if mountpoint -q /sys/kernel/debug 2>/dev/null; then
        info "debugfs already mounted at /sys/kernel/debug"
    else
        info "Mounting debugfs..."
        mount -t debugfs none /sys/kernel/debug
        info "debugfs mounted"
    fi
}

check_iommu() {
    info "Checking IOMMU status..."

    if [ -d /sys/class/iommu ]; then
        local iommu_devs
        iommu_devs=$(ls /sys/class/iommu/ 2>/dev/null | wc -l)
        if [ "$iommu_devs" -gt 0 ]; then
            info "IOMMU is active (${iommu_devs} IOMMU groups)"
            if dmesg | grep -qi "DMAR.*IOMMU enabled\|AMD-Vi.*enabled" 2>/dev/null; then
                info "IOMMU type: $(dmesg | grep -oi 'DMAR\|AMD-Vi' | head -1)"
            fi
        else
            warn "IOMMU not active"
        fi
    else
        warn "IOMMU not available"
    fi

    # Check kernel command line for IOMMU settings
    if grep -q "iommu=pt" /proc/cmdline; then
        info "IOMMU passthrough mode (iommu=pt)"
    elif grep -q "iommu=on" /proc/cmdline; then
        info "IOMMU translation mode (iommu=on)"
    fi
}

check_nvme_devices() {
    info "Checking NVMe devices..."

    if ! lsmod | grep -q nvme; then
        warn "NVMe kernel module not loaded"
        return
    fi

    echo ""
    echo "NVMe PCIe devices:"
    echo "-------------------------------------------"
    lspci -d ::0108 -v 2>/dev/null | grep -E "^[0-9a-f]|NVMe|Memory|IRQ" | head -20 || {
        warn "No NVMe devices found via lspci"
        return
    }

    echo ""
    echo "NVMe block devices:"
    echo "-------------------------------------------"
    if command -v nvme &>/dev/null; then
        nvme list 2>/dev/null || true
    else
        ls -la /dev/nvme* 2>/dev/null || echo "  (none)"
    fi

    echo ""
    echo "NVMe PCI BDFs (for target_pci parameter):"
    echo "-------------------------------------------"
    lspci -d ::0108 -D 2>/dev/null | while read -r line; do
        bdf=$(echo "$line" | awk '{print $1}')
        desc=$(echo "$line" | cut -d: -f4-)
        echo "  $bdf  -$desc"
    done
}

check_kernel_config() {
    info "Checking kernel configuration..."

    local kconfig="/boot/config-$(uname -r)"
    if [ ! -f "$kconfig" ]; then
        kconfig="/proc/config.gz"
        if [ ! -f "$kconfig" ]; then
            warn "Cannot find kernel config"
            return
        fi
    fi

    local check_configs=(
        "CONFIG_KPROBES"
        "CONFIG_RELAY"
        "CONFIG_DEBUG_FS"
        "CONFIG_BPF"
        "CONFIG_BPF_SYSCALL"
        "CONFIG_FTRACE"
        "CONFIG_MMIOTRACE"
    )

    for cfg in "${check_configs[@]}"; do
        if grep -q "^${cfg}=y" "$kconfig" 2>/dev/null; then
            info "  $cfg = y"
        elif zcat "$kconfig" 2>/dev/null | grep -q "^${cfg}=y"; then
            info "  $cfg = y"
        else
            warn "  $cfg not set or not found"
        fi
    done
}

build_project() {
    local project_dir
    project_dir="$(cd "$(dirname "$0")/.." && pwd)"

    info "Building nvme-pcie-tracer..."

    if [ -f "${project_dir}/Makefile" ]; then
        make -C "$project_dir" clean 2>/dev/null || true
        make -C "$project_dir" all
        info "Build complete"
    else
        warn "Top-level Makefile not found at ${project_dir}/Makefile"
        # Try building user tools only
        if [ -f "${project_dir}/user/Makefile" ]; then
            make -C "${project_dir}/user" all
            info "User tools built"
        fi
    fi
}

# ---- Main ----

echo "============================================"
echo "  nvme-pcie-tracer Environment Setup"
echo "  Target: CentOS/Rocky Linux 9"
echo "============================================"
echo ""

check_root
check_distro
echo ""
install_dependencies
echo ""
mount_debugfs
echo ""
check_iommu
echo ""
check_nvme_devices
echo ""
check_kernel_config
echo ""
build_project
echo ""

info "Setup complete!"
echo ""
echo "Next steps:"
echo "  1. Load the kernel module:"
echo "     sudo insmod kernel/nvme_pcie_tracer.ko target_pci=\"<BDF>\""
echo "  2. Enable tracing:"
echo "     echo 1 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled"
echo "  3. Start the reader:"
echo "     sudo ./user/nvme_trace_reader -f"
echo ""
echo "Or use the convenience scripts:"
echo "  sudo ./scripts/start_trace.sh [PCI_BDF]"
echo "  sudo ./scripts/stop_trace.sh"
