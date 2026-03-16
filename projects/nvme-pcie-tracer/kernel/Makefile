# SPDX-License-Identifier: GPL-2.0
#
# Makefile - nvme-pcie-tracer 커널 모듈 빌드
#
# 사용법:
#   make                    # 현재 커널 대상 빌드
#   make KDIR=/path/to/kernel  # 지정 커널 대상 빌드
#   make clean              # 빌드 산출물 삭제
#
# CentOS/Rocky Linux 9 빌드 사전 조건:
#   sudo dnf install -y kernel-devel-$(uname -r) gcc make elfutils-libelf-devel

# 단일 .ko 모듈로 빌드: 모든 .c 파일을 하나의 nvme_pcie_tracer_mod.ko에 링크한다.
# kbuild 규칙: obj-m 이름과 소스 파일 이름이 같으면 -objs를 무시하므로,
# 모듈명을 nvme_pcie_tracer_mod로 한다 (메인 소스: nvme_pcie_tracer.c).
obj-m := nvme_pcie_tracer_mod.o

nvme_pcie_tracer_mod-objs := \
	nvme_pcie_tracer.o \
	trace_buffer.o \
	nvme_sq_monitor.o \
	nvme_cq_monitor.o \
	nvme_doorbell_tracer.o \
	nvme_irq_hooker.o \
	nvme_mmio_tracer.o \
	nvme_dma_tracer.o

# 컴파일 플래그
ccflags-y := -DDEBUG -g -Wall -Wno-unused-function

# 커널 빌드 디렉토리 (배포판 기본 경로)
KDIR ?= /lib/modules/$(shell uname -r)/build

# 현재 디렉토리
PWD := $(shell pwd)

# 기본 타겟: 커널 모듈 빌드
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 클린
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 설치 (선택)
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# 모듈 로드/언로드 편의 타겟
load:
	sudo insmod nvme_pcie_tracer_mod.ko

unload:
	sudo rmmod nvme_pcie_tracer_mod

reload: unload load

# 트레이싱 제어 편의 타겟
start:
	echo 1 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled

stop:
	echo 0 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled

status:
	@cat /sys/kernel/debug/nvme-pcie-tracer/status 2>/dev/null || echo "Module not loaded"

stats:
	@cat /sys/kernel/debug/nvme-pcie-tracer/stats 2>/dev/null || echo "Module not loaded"

.PHONY: all clean install load unload reload start stop status stats
