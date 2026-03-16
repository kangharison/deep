#
# nvme-pcie-tracer top-level Makefile
#
# Builds kernel module and userspace tools.
#

.PHONY: all kernel user bpf clean install help

all: kernel user

kernel:
	@if [ -d kernel ] && [ -f kernel/Makefile ]; then \
		echo "=== Building kernel module ==="; \
		$(MAKE) -C kernel; \
	else \
		echo "Kernel module source not yet available (see 00_project_design.md)"; \
	fi

user:
	@echo "=== Building userspace tools ==="
	$(MAKE) -C user

bpf:
	@echo "=== BPF scripts do not require compilation ==="
	@echo "Use: sudo bpftrace bpf/nvme_tracer.bt"
	@echo "Use: sudo bpftrace bpf/nvme_latency.bt"

clean:
	@if [ -d kernel ] && [ -f kernel/Makefile ]; then \
		$(MAKE) -C kernel clean 2>/dev/null || true; \
	fi
	$(MAKE) -C user clean

install:
	@echo "=== Installing nvme-pcie-tracer ==="
	install -d /usr/local/bin
	install -m 755 user/nvme_trace_reader /usr/local/bin/ 2>/dev/null || \
		echo "Build user tools first: make user"
	install -d /usr/local/share/nvme-pcie-tracer/bpf
	install -m 644 bpf/*.bt /usr/local/share/nvme-pcie-tracer/bpf/
	install -d /usr/local/share/nvme-pcie-tracer/scripts
	install -m 755 scripts/*.sh /usr/local/share/nvme-pcie-tracer/scripts/
	install -m 755 scripts/analyze.py /usr/local/share/nvme-pcie-tracer/scripts/
	@if [ -f kernel/nvme_pcie_tracer.ko ]; then \
		install -d /lib/modules/$$(uname -r)/extra; \
		install -m 644 kernel/nvme_pcie_tracer.ko /lib/modules/$$(uname -r)/extra/; \
		depmod -a; \
		echo "Kernel module installed"; \
	fi
	@echo "Installation complete"

help:
	@echo "nvme-pcie-tracer build targets:"
	@echo ""
	@echo "  make all      - Build kernel module + userspace tools"
	@echo "  make kernel   - Build kernel module only"
	@echo "  make user     - Build userspace tools only"
	@echo "  make bpf      - Show BPF usage info"
	@echo "  make clean    - Clean build artifacts"
	@echo "  make install  - Install to system"
	@echo "  make help     - Show this help"
	@echo ""
	@echo "Quick start:"
	@echo "  make user"
	@echo "  sudo scripts/setup.sh"
	@echo "  sudo scripts/start_trace.sh -r"
