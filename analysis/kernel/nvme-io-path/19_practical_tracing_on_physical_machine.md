# NVMe 실전 트레이싱 가이드 (물리 x86_64 머신)

> 대상 환경: CentOS/Rocky Linux 9, Intel/AMD x86_64 CPU, 64GB DRAM, NVMe SSD
> 모든 명령어는 root 권한으로 실행한다.

---

## 1. 환경 확인 및 준비

### 1.1 하드웨어 확인

```bash
# CPU 정보 (Intel vs AMD 판별)
lscpu | grep -E "Model name|Vendor|Thread|Core|Socket"

# IOMMU 지원 확인 (Intel VT-d 또는 AMD-Vi)
dmesg | grep -iE "DMAR|IOMMU|AMD-Vi"

# NVMe 디바이스 목록
nvme list
lspci -vvv | grep -i nvme

# NVMe 컨트롤러 상세 정보 (BAR, IRQ, NUMA 노드)
lspci -vvv -s $(lspci | grep -i nvme | awk '{print $1}')

# 메모리 확인
free -h
cat /proc/meminfo | head -5

# 커널 버전 확인
uname -r
cat /etc/os-release
```

### 1.2 필수 패키지 설치

```bash
# EPEL 저장소 활성화
dnf install -y epel-release

# 기본 트레이싱 도구
dnf install -y bpftrace perf blktrace iowatcher

# 커널 디버그 심볼 (bpftrace/perf에 필수)
dnf install -y kernel-debuginfo-$(uname -r) kernel-debuginfo-common-$(uname -m)-$(uname -r)
# CentOS/Rocky 9에서는 debuginfo-install 사용 가능
debuginfo-install -y kernel-$(uname -r)

# 커널 헤더 및 개발 패키지
dnf install -y kernel-devel-$(uname -r) kernel-headers-$(uname -r)

# bcc-tools (BPF 기반 도구 모음)
dnf install -y bcc-tools

# nvme-cli (NVMe 관리 도구)
dnf install -y nvme-cli

# trace-cmd (ftrace 프론트엔드)
dnf install -y trace-cmd
```

### 1.3 IOMMU 활성화 (부트 파라미터)

```bash
# 현재 IOMMU 상태 확인
cat /proc/cmdline | tr ' ' '\n' | grep -i iommu

# Intel CPU인 경우
grubby --update-kernel=ALL --args="intel_iommu=on iommu=pt"

# AMD CPU인 경우
grubby --update-kernel=ALL --args="amd_iommu=on iommu=pt"

# DMA debug 활성화 (부트 파라미터에 추가)
grubby --update-kernel=ALL --args="dma_debug=1"

# 재부팅 후 확인
reboot

# 재부팅 후 IOMMU 활성화 확인
dmesg | grep -i "IOMMU enabled"
dmesg | grep -i "DMAR"
```

### 1.4 debugfs 마운트 확인

```bash
# debugfs 마운트 상태 확인
mount | grep debugfs

# 마운트되지 않은 경우 수동 마운트
mount -t debugfs none /sys/kernel/debug

# tracefs 확인 (ftrace 사용에 필요)
mount | grep tracefs
ls /sys/kernel/debug/tracing/
```

### 1.5 NVMe 모듈 파라미터 확인

커널 소스(`drivers/nvme/host/pci.c`)에 정의된 모듈 파라미터들의 현재 값을 확인한다.

```bash
# 현재 로드된 NVMe 모듈 파라미터 전체 조회
cat /sys/module/nvme/parameters/* 2>/dev/null
for f in /sys/module/nvme_core/parameters/*; do
    echo "$(basename $f) = $(cat $f)"
done
for f in /sys/module/nvme/parameters/*; do
    echo "$(basename $f) = $(cat $f)"
done

# 주요 파라미터 개별 확인
# io_queue_depth: I/O 큐 깊이 (기본 1024, 범위 2~4095)
cat /sys/module/nvme/parameters/io_queue_depth

# use_threaded_interrupts: 스레드 인터럽트 사용 여부 (기본 0)
cat /sys/module/nvme/parameters/use_threaded_interrupts

# use_cmb_sqes: CMB에 SQ 엔트리 배치 여부 (기본 Y)
cat /sys/module/nvme/parameters/use_cmb_sqes

# poll_queues: 폴링 전용 큐 수 (기본 0)
cat /sys/module/nvme/parameters/poll_queues

# write_queues: 쓰기 전용 큐 수 (기본 0, 읽기/쓰기 공유)
cat /sys/module/nvme/parameters/write_queues

# sgl_threshold: SGL 사용 임계값 (기본 32768 바이트)
cat /sys/module/nvme/parameters/sgl_threshold
```

---

## 2. mmiotrace (Host → Device MMIO 캡처)

mmiotrace는 커널의 `arch/x86/mm/mmio-mod.c`에 구현되어 있다. CPU가 디바이스 BAR 영역에 수행하는 MMIO 읽기/쓰기를 page fault 기반으로 캡처한다. NVMe Doorbell 쓰기(SQ Tail, CQ Head)를 직접 관찰할 수 있는 유일한 방법이다.

### 2.1 BAR0 주소 확인

```bash
# NVMe 디바이스의 PCI BDF 확인
NVME_BDF=$(lspci | grep -i nvme | head -1 | awk '{print $1}')
echo "NVMe BDF: $NVME_BDF"

# BAR0 주소 확인 (NVMe 레지스터 맵)
lspci -vvv -s $NVME_BDF | grep "Region 0"
# 출력 예: Region 0: Memory at fb000000 (64-bit, non-prefetchable) [size=16K]

# sysfs에서 리소스 확인
cat /sys/bus/pci/devices/0000:$NVME_BDF/resource
# 첫 번째 줄이 BAR0: 시작주소 끝주소 플래그
```

### 2.2 mmiotrace 활성화 및 캡처

**주의: mmiotrace는 시스템을 단일 프로세서 모드로 전환한다(`enter_uniprocessor`). 성능이 극도로 저하되므로 프로덕션 환경에서 사용하지 않는다.**

```bash
# 1) ftrace를 mmiotrace 트레이서로 설정
echo mmiotrace > /sys/kernel/debug/tracing/current_tracer

# 2) 트레이스 버퍼 크기 증가 (MB 단위)
echo 64000 > /sys/kernel/debug/tracing/buffer_size_kb

# 3) 트레이싱 시작 (이 시점부터 모든 MMIO 접근 캡처)
echo 1 > /sys/kernel/debug/tracing/tracing_on

# 4) 짧은 I/O 워크로드 실행 (Doorbell 쓰기가 발생함)
dd if=/dev/nvme0n1 of=/dev/null bs=4k count=10 iflag=direct

# 5) 트레이싱 중지
echo 0 > /sys/kernel/debug/tracing/tracing_on

# 6) 결과 저장
cat /sys/kernel/debug/tracing/trace > /tmp/mmiotrace_nvme.log

# 7) 트레이서 원복 (반드시 수행)
echo nop > /sys/kernel/debug/tracing/current_tracer
```

### 2.3 mmiotrace 출력 해석

```bash
# 로그 확인
head -50 /tmp/mmiotrace_nvme.log

# 형식: W 4 0xfb001000 0x00000001 kworker/0:1H-1234 123.456789
# W = Write (R = Read)
# 4 = 크기 (바이트)
# 0xfb001000 = 물리 주소 (BAR0 + offset)
# 0x00000001 = 기록된 값
```

### 2.4 NVMe Doorbell 식별

NVMe 스펙에서 Doorbell 레지스터의 오프셋은 `0x1000 + (큐번호 * 2 * doorbell_stride)`이다.

```bash
# BAR0 기준 Doorbell 오프셋 필터링
BAR0_ADDR="0xfb000000"  # lspci에서 확인한 BAR0 주소로 교체
grep "^W" /tmp/mmiotrace_nvme.log | awk -v bar="$BAR0_ADDR" '
{
    addr = strtonum($3)
    base = strtonum(bar)
    offset = addr - base
    if (offset >= 0x1000) {
        queue = int((offset - 0x1000) / 8)
        is_sq = ((offset - 0x1000) % 8 == 0)
        printf "Addr=%s Offset=0x%04x Queue=%d Type=%s Value=%s\n",
            $3, offset, queue, (is_sq ? "SQ_Tail" : "CQ_Head"), $4
    }
}' | head -30

# CAP, CC, CSTS 등 컨트롤러 레지스터 접근 확인
grep -E "^[WR]" /tmp/mmiotrace_nvme.log | awk -v bar="$BAR0_ADDR" '
{
    addr = strtonum($3)
    base = strtonum(bar)
    offset = addr - base
    if (offset < 0x1000) {
        printf "Addr=%s Offset=0x%04x Value=%s (%s)\n",
            $3, offset, $4, $1
    }
}' | head -20
```

### 2.5 mmiotrace 주의사항

- `enable_mmiotrace()`는 `enter_uniprocessor()`를 호출하여 CPU 1개만 남기고 나머지를 오프라인 처리한다.
- 트레이싱 중 시스템 응답이 매우 느려진다.
- SSH 세션이 끊길 수 있으므로 `screen` 또는 `tmux`를 사용한다.
- 캡처 시간을 최소한으로 유지한다 (수 초 이내 권장).
- `nommiotrace` 모듈 파라미터로 실제 트레이싱 없이 인프라만 테스트할 수 있다.
- Secure Boot가 활성화된 경우 mmiotrace가 동작하지 않을 수 있다.

---

## 3. IOMMU Fault 로그 (Device → Host DMA)

IOMMU(Intel VT-d / AMD-Vi)는 디바이스의 DMA 접근을 가상→물리 주소 변환하고, 잘못된 접근을 차단하여 fault를 발생시킨다.

### 3.1 Intel VT-d 환경

```bash
# DMAR(DMA Remapping) 초기화 확인
dmesg | grep -i "DMAR"

# IOMMU 그룹 확인
find /sys/kernel/iommu_groups/ -type l | sort -t/ -k5 -n | while read link; do
    dev=$(basename $(readlink $link))
    group=$(echo $link | sed 's|.*iommu_groups/\([0-9]*\)/.*|\1|')
    echo "Group $group: $dev $(lspci -s $dev 2>/dev/null | cut -d: -f3-)"
done | grep -i nvme

# VT-d fault 로그 실시간 모니터링
dmesg -wH | grep -iE "DMAR|fault|IOMMU"

# DMAR 통계 확인
cat /sys/kernel/debug/iommu/intel/dmar*/stats 2>/dev/null

# IOMMU 도메인 정보
ls /sys/kernel/debug/iommu/intel/ 2>/dev/null
```

### 3.2 AMD-Vi 환경

```bash
# AMD IOMMU 초기화 확인
dmesg | grep -i "AMD-Vi"

# AMD IOMMU event 로그 모니터링
dmesg -wH | grep -iE "AMD-Vi|IO_PAGE_FAULT"

# IOMMU 그룹 및 매핑 확인 (Intel/AMD 공통)
for d in /sys/kernel/iommu_groups/*/devices/*; do
    dev=$(basename $d)
    group=$(echo $d | sed 's|.*iommu_groups/\([0-9]*\)/.*|\1|')
    echo "Group $group: $dev"
done | grep -i $(lspci | grep -i nvme | awk '{print $1}')
```

### 3.3 IOMMU Fault 유발 (디버깅/학습 목적)

정상 환경에서는 IOMMU fault가 발생하지 않는다. 학습 목적으로 fault 로그 형태를 확인하려면 다음 방법을 사용한다.

```bash
# passthrough 모드에서 strict 모드로 변경하여 DMA 해제 지연 제거
echo strict > /sys/kernel/debug/iommu/intel/dmar0/iotlb_flush_policy 2>/dev/null

# IOMMU 동작 확인용 ftrace 이벤트
echo 1 > /sys/kernel/debug/tracing/events/iommu/enable
cat /sys/kernel/debug/tracing/trace_pipe &
TRACE_PID=$!

# I/O 실행하여 IOMMU 이벤트 발생
dd if=/dev/nvme0n1 of=/dev/null bs=4k count=100 iflag=direct

# 트레이스 중지
kill $TRACE_PID 2>/dev/null
echo 0 > /sys/kernel/debug/tracing/events/iommu/enable
```

### 3.4 IOMMU 이벤트 ftrace 포인트

```bash
# 사용 가능한 IOMMU 관련 트레이스 이벤트 확인
ls /sys/kernel/debug/tracing/events/iommu/ 2>/dev/null

# 전체 IOMMU 이벤트 활성화
echo 1 > /sys/kernel/debug/tracing/events/iommu/map/enable
echo 1 > /sys/kernel/debug/tracing/events/iommu/unmap/enable

# 트레이스 확인
cat /sys/kernel/debug/tracing/trace | head -30

# 정리
echo 0 > /sys/kernel/debug/tracing/events/iommu/map/enable
echo 0 > /sys/kernel/debug/tracing/events/iommu/unmap/enable
```

---

## 4. DMA Debug

커널의 `kernel/dma/debug.c`는 DMA API 오용을 감지하는 디버깅 인프라다. `PREALLOC_DMA_DEBUG_ENTRIES`(65536개)를 사전 할당하고, DMA map/unmap 쌍의 올바른 사용을 검증한다.

### 4.1 DMA Debug 활성화

```bash
# 부트 파라미터로 활성화 (재부팅 필요)
grubby --update-kernel=ALL --args="dma_debug=1"

# 활성화 확인 (재부팅 후)
dmesg | grep "DMA-API"
# 정상이면: "DMA-API: debugging enabled by kernel config"

# 비활성화 확인
cat /sys/kernel/debug/dma-api/disabled
# 0 = 활성, 1 = 비활성
```

### 4.2 DMA Debug 정보 조회

```bash
# DMA 디버그 전체 통계
cat /sys/kernel/debug/dma-api/num_errors
cat /sys/kernel/debug/dma-api/num_free_entries
cat /sys/kernel/debug/dma-api/min_free_entries

# 드라이버별 DMA 사용 통계
cat /sys/kernel/debug/dma-api/driver_filter

# 특정 드라이버만 필터링
echo nvme > /sys/kernel/debug/dma-api/driver_filter

# 에러 발생 시 dmesg에서 확인
dmesg | grep "DMA-API"
# 예: "DMA-API: nvme 0000:03:00.0: device driver tries to free DMA memory it has not allocated"
```

### 4.3 NVMe DMA 매핑 상태 확인

```bash
# NVMe 디바이스의 DMA 매핑 정보
NVME_BDF=$(lspci | grep -i nvme | head -1 | awk '{print $1}')
cat /sys/bus/pci/devices/0000:$NVME_BDF/dma_map_*  2>/dev/null

# 전체 IOMMU DMA 매핑 덤프 (debugfs)
cat /sys/kernel/debug/iommu/intel/dmar*/mappings 2>/dev/null | head -50

# DMA 할당 상태 확인 (각 디바이스별)
for dev in /sys/bus/pci/devices/*/; do
    if [ -f "$dev/driver" ]; then
        driver=$(basename $(readlink $dev/driver))
        if [ "$driver" = "nvme" ]; then
            echo "=== $(basename $dev) ==="
            cat $dev/numa_node 2>/dev/null
            cat $dev/dma_mask_bits 2>/dev/null
        fi
    fi
done
```

---

## 5. bpftrace 실전 스크립트

bpftrace는 eBPF 기반으로 커널 함수를 동적으로 트레이싱한다. NVMe I/O 경로의 핵심 함수들을 관찰할 수 있다.

### 5.1 nvme_queue_rq 트레이싱 (I/O 제출 경로)

```bash
# nvme_queue_rq 호출 빈도 및 호출자 추적
bpftrace -e '
kprobe:nvme_queue_rq
{
    @count[comm] = count();
    @stack[kstack] = count();
}

interval:s:5 { print(@count); clear(@count); }
' 2>/dev/null

# nvme_queue_rq 인자 분석 (hctx, rq)
bpftrace -e '
kprobe:nvme_queue_rq
{
    $hctx = (struct blk_mq_hw_ctx *)arg0;
    printf("cpu=%d queue_num=%d comm=%s\n",
        cpu, $hctx->queue_num, comm);
}
' 2>/dev/null
```

### 5.2 DMA map/unmap 트레이싱

```bash
# dma_map_sg_attrs 호출 추적 (scatter-gather DMA 매핑)
bpftrace -e '
kprobe:dma_map_sg_attrs
{
    $dev = (struct device *)arg0;
    $nents = (int)arg2;
    printf("dma_map_sg: nents=%d comm=%s\n", $nents, comm);
    @sg_count = hist($nents);
}

interval:s:10 { print(@sg_count); }
'

# DMA 매핑 주소 범위 확인
bpftrace -e '
kretprobe:dma_map_page_attrs
/retval != 0/
{
    printf("dma_addr=0x%lx comm=%s\n", retval, comm);
    @dma_addrs = hist(retval);
}
'
```

### 5.3 NVMe 인터럽트(Completion) 트레이싱

```bash
# nvme_irq 호출 빈도 (큐별 인터럽트 분포)
bpftrace -e '
kprobe:nvme_irq
{
    @irq_count[cpu] = count();
}

interval:s:3 {
    print(@irq_count);
    clear(@irq_count);
}
'

# NVMe completion 처리 추적
bpftrace -e '
kprobe:nvme_pci_complete_rq
{
    @comp[comm, cpu] = count();
}

interval:s:5 { print(@comp); clear(@comp); }
'
```

### 5.4 NVMe I/O 레이턴시 측정

```bash
# 제출~완료 레이턴시 (nvme_queue_rq → nvme_pci_complete_rq)
bpftrace -e '
kprobe:nvme_queue_rq
{
    @start[tid] = nsecs;
}

kprobe:nvme_pci_complete_rq
/@start[tid]/
{
    $lat = nsecs - @start[tid];
    @usec_hist = hist($lat / 1000);
    @avg_ns = avg($lat);
    delete(@start[tid]);
}

interval:s:10 {
    printf("\n--- Latency Distribution (usec) ---\n");
    print(@usec_hist);
    printf("Average: %lld ns\n", @avg_ns);
    clear(@usec_hist);
    clear(@avg_ns);
}
'
```

### 5.5 종합 NVMe 트레이싱 스크립트

아래 내용을 `/tmp/nvme_trace.bt`로 저장하고 실행한다.

```bash
cat > /tmp/nvme_trace.bt << 'BTEOF'
/* NVMe I/O 경로 종합 트레이싱 */

BEGIN
{
    printf("NVMe Comprehensive Tracing Started...\n");
    printf("Ctrl+C to stop and show results\n\n");
}

/* 1. blk-mq 레이어: I/O 요청 시작 */
tracepoint:block:block_rq_issue
/args->rwbs[0] != 0/
{
    @rq_issue[args->rwbs] = count();
}

/* 2. NVMe 드라이버: 큐에 제출 */
kprobe:nvme_queue_rq
{
    @submit_start[tid] = nsecs;
    @submit_cpu[cpu] = count();
}

/* 3. NVMe 완료 */
kprobe:nvme_pci_complete_rq
{
    if (@submit_start[tid]) {
        $lat = nsecs - @submit_start[tid];
        @latency_us = hist($lat / 1000);
        delete(@submit_start[tid]);
    }
    @complete_cpu[cpu] = count();
}

/* 4. NVMe 인터럽트 */
kprobe:nvme_irq
{
    @irq_cpu[cpu] = count();
}

/* 5. DMA 매핑 */
kprobe:dma_map_sg_attrs
{
    @dma_maps = count();
}

END
{
    printf("\n\n=== Block Layer I/O Types ===\n");
    print(@rq_issue);

    printf("\n=== Submit CPU Distribution ===\n");
    print(@submit_cpu);

    printf("\n=== Complete CPU Distribution ===\n");
    print(@complete_cpu);

    printf("\n=== IRQ CPU Distribution ===\n");
    print(@irq_cpu);

    printf("\n=== I/O Latency (usec) ===\n");
    print(@latency_us);

    printf("\n=== DMA Mapping Count ===\n");
    print(@dma_maps);
}
BTEOF

# 실행
bpftrace /tmp/nvme_trace.bt
```

---

## 6. ftrace function_graph

ftrace의 function_graph 트레이서는 함수 호출/반환 체인을 트리 구조로 보여준다. NVMe I/O 경로의 전체 호출 흐름을 파악하는 데 가장 효과적이다.

### 6.1 NVMe 함수 호출 체인 추적

```bash
# 현재 트레이서 초기화
echo nop > /sys/kernel/debug/tracing/current_tracer

# 트레이스할 함수 필터 설정 (NVMe + blk-mq)
echo 'nvme_*' > /sys/kernel/debug/tracing/set_ftrace_filter
echo 'blk_mq_*' >> /sys/kernel/debug/tracing/set_ftrace_filter

# function_graph 트레이서 설정
echo function_graph > /sys/kernel/debug/tracing/current_tracer

# 그래프 깊이 제한 (너무 깊으면 오버헤드 증가)
echo 10 > /sys/kernel/debug/tracing/max_graph_depth

# 버퍼 크기 설정
echo 16384 > /sys/kernel/debug/tracing/buffer_size_kb

# 트레이싱 시작
echo 1 > /sys/kernel/debug/tracing/tracing_on

# I/O 실행
dd if=/dev/nvme0n1 of=/dev/null bs=4k count=10 iflag=direct

# 트레이싱 중지 및 결과 저장
echo 0 > /sys/kernel/debug/tracing/tracing_on
cat /sys/kernel/debug/tracing/trace > /tmp/ftrace_nvme_graph.log

# 결과 확인
head -100 /tmp/ftrace_nvme_graph.log

# 정리
echo nop > /sys/kernel/debug/tracing/current_tracer
echo > /sys/kernel/debug/tracing/set_ftrace_filter
```

### 6.2 trace-cmd를 이용한 간편 사용법

```bash
# NVMe 함수 호출 그래프 기록
trace-cmd record -p function_graph \
    -g nvme_queue_rq \
    -g blk_mq_start_request \
    --max-graph-depth 8 \
    -- dd if=/dev/nvme0n1 of=/dev/null bs=4k count=100 iflag=direct

# 결과 출력
trace-cmd report > /tmp/trace_report.txt
head -200 /tmp/trace_report.txt

# 특정 함수만 필터링하여 기록
trace-cmd record -p function_graph \
    -l 'nvme_*' -l 'blk_mq_*' \
    --max-graph-depth 6 \
    -- fio --name=test --filename=/dev/nvme0n1 --rw=randread \
       --bs=4k --numjobs=1 --iodepth=1 --runtime=3 --time_based \
       --ioengine=libaio --direct=1

trace-cmd report | head -300
```

### 6.3 blk-mq 이벤트 추적

```bash
# blk-mq 관련 트레이스 이벤트 전체 활성화
echo 1 > /sys/kernel/debug/tracing/events/block/enable

# 특정 이벤트만 활성화
echo 1 > /sys/kernel/debug/tracing/events/block/block_rq_issue/enable
echo 1 > /sys/kernel/debug/tracing/events/block/block_rq_complete/enable
echo 1 > /sys/kernel/debug/tracing/events/block/block_rq_insert/enable

# NVMe 이벤트
echo 1 > /sys/kernel/debug/tracing/events/nvme/enable 2>/dev/null

echo > /sys/kernel/debug/tracing/trace
echo 1 > /sys/kernel/debug/tracing/tracing_on

dd if=/dev/nvme0n1 of=/dev/null bs=4k count=10 iflag=direct

echo 0 > /sys/kernel/debug/tracing/tracing_on
cat /sys/kernel/debug/tracing/trace > /tmp/blkmq_events.log
head -50 /tmp/blkmq_events.log

# 정리
echo 0 > /sys/kernel/debug/tracing/events/block/enable
echo 0 > /sys/kernel/debug/tracing/events/nvme/enable 2>/dev/null
```

---

## 7. blktrace

blktrace는 블록 I/O 레이어의 요청 흐름을 상세히 추적한다. I/O가 어느 단계(큐잉, 머지, 디스패치, 완료)에서 시간을 소비하는지 파악할 수 있다.

### 7.1 blktrace 실행

```bash
# 기본 블록 트레이스 수집 (10초)
blktrace -d /dev/nvme0n1 -o /tmp/nvme_blktrace -w 10 &
BLKTRACE_PID=$!

# 동시에 워크로드 실행
fio --name=test --filename=/dev/nvme0n1 --rw=randread \
    --bs=4k --numjobs=4 --iodepth=32 --runtime=8 --time_based \
    --ioengine=libaio --direct=1

# blktrace 종료 대기
wait $BLKTRACE_PID

# 결과 파싱
blkparse -i /tmp/nvme_blktrace -o /tmp/nvme_blkparse.txt
```

### 7.2 blkparse 출력 해석

```bash
# 요약 통계 보기
blkparse -i /tmp/nvme_blktrace -s -o /dev/null

# 주요 이벤트 코드:
# Q = Queued (bio가 블록 레이어에 도착)
# G = Get request (request 구조체 할당)
# I = Inserted (엘리베이터/스케줄러 큐에 삽입)
# D = Dispatched (드라이버에 전달)
# C = Completed (완료)
# M = Merged (기존 요청과 합병)

# Q→D 지연 (소프트웨어 큐잉 시간)
# D→C 지연 (디바이스 처리 시간)

# 특정 이벤트만 필터링
blkparse -i /tmp/nvme_blktrace -a issue -a complete -o /tmp/nvme_issue_complete.txt
head -30 /tmp/nvme_issue_complete.txt

# 레이턴시 통계 (btt 도구 사용)
btt -i /tmp/nvme_blktrace.blktrace.0 -o /tmp/nvme_btt
cat /tmp/nvme_btt.avg
```

### 7.3 iowatcher 시각화

```bash
# blktrace 데이터로 SVG 그래프 생성
iowatcher -t /tmp/nvme_blktrace -o /tmp/nvme_iowatcher.svg

# 특정 비교 (두 워크로드 비교)
# 첫 번째 워크로드
blktrace -d /dev/nvme0n1 -o /tmp/trace_read -w 5 &
fio --name=read --filename=/dev/nvme0n1 --rw=randread --bs=4k \
    --iodepth=32 --runtime=4 --ioengine=libaio --direct=1
wait

# 두 번째 워크로드
blktrace -d /dev/nvme0n1 -o /tmp/trace_write -w 5 &
fio --name=write --filename=/dev/nvme0n1 --rw=randwrite --bs=4k \
    --iodepth=32 --runtime=4 --ioengine=libaio --direct=1
wait

# 비교 그래프 생성
iowatcher -t /tmp/trace_read -t /tmp/trace_write \
    -l "Random Read" -l "Random Write" \
    -o /tmp/nvme_compare.svg
```

---

## 8. perf

perf는 CPU 성능 카운터와 소프트웨어 이벤트를 기반으로 NVMe I/O 경로의 CPU 오버헤드를 분석한다.

### 8.1 NVMe I/O 경로 CPU 오버헤드 프로파일링

```bash
# NVMe 관련 함수의 CPU 사용률 측정
perf record -ag -F 999 -- \
    fio --name=test --filename=/dev/nvme0n1 --rw=randread \
    --bs=4k --numjobs=4 --iodepth=32 --runtime=10 --time_based \
    --ioengine=libaio --direct=1

# 결과 보기 (함수별 CPU 점유율)
perf report --stdio --sort=dso,symbol | head -60

# NVMe/블록 관련 함수만 필터
perf report --stdio | grep -E "nvme_|blk_mq_|bio_|__blk" | head -30
```

### 8.2 인터럽트 분석

```bash
# 하드웨어 인터럽트 (NVMe MSI-X) 오버헤드 측정
perf stat -e irq:irq_handler_entry,irq:irq_handler_exit -a -- sleep 5

# NVMe 인터럽트 상세 추적
perf record -e irq:irq_handler_entry -a --filter "irq_name ~ nvme*" -- sleep 5
perf report --stdio

# softirq 오버헤드 (blk_mq의 softirq 완료 처리)
perf stat -e irq:softirq_entry,irq:softirq_exit -a -- sleep 5
```

### 8.3 NVMe 함수별 호출 횟수 및 시간

```bash
# 특정 함수의 호출 빈도와 소요 시간
perf probe --add nvme_queue_rq
perf probe --add nvme_pci_complete_rq

perf stat -e probe:nvme_queue_rq,probe:nvme_pci_complete_rq -a -- \
    fio --name=test --filename=/dev/nvme0n1 --rw=randread \
    --bs=4k --iodepth=32 --runtime=5 --ioengine=libaio --direct=1

# probe 정리
perf probe --del nvme_queue_rq
perf probe --del nvme_pci_complete_rq
```

### 8.4 cache miss 분석 (NVMe 구조체 접근 패턴)

```bash
# L1/L2/LLC cache miss 측정
perf stat -e cache-misses,cache-references,L1-dcache-load-misses,LLC-load-misses -a -- \
    fio --name=test --filename=/dev/nvme0n1 --rw=randread \
    --bs=4k --iodepth=32 --runtime=10 --ioengine=libaio --direct=1

# NUMA 관련 메모리 접근 패턴
perf stat -e node-loads,node-load-misses,node-stores,node-store-misses -a -- sleep 5
```

---

## 9. /proc /sys 인터페이스

재부팅이나 추가 도구 없이 즉시 확인할 수 있는 정보들이다.

### 9.1 NVMe 디바이스 상태

```bash
# NVMe 컨트롤러 정보
nvme id-ctrl /dev/nvme0 | head -30
nvme id-ctrl /dev/nvme0 -H  # Human-readable

# NVMe 네임스페이스 정보
nvme id-ns /dev/nvme0n1

# SMART / 헬스 정보
nvme smart-log /dev/nvme0

# 펌웨어 로그
nvme fw-log /dev/nvme0

# NVMe 큐 정보 (sysfs)
ls /sys/class/nvme/nvme0/
cat /sys/class/nvme/nvme0/transport
cat /sys/class/nvme/nvme0/model
cat /sys/class/nvme/nvme0/queue_count

# I/O 큐별 상태
ls /sys/class/nvme/nvme0/nvme0n1/queue/
cat /sys/class/nvme/nvme0/nvme0n1/queue/nr_requests
cat /sys/class/nvme/nvme0/nvme0n1/queue/scheduler
cat /sys/class/nvme/nvme0/nvme0n1/queue/hw_sector_size
```

### 9.2 인터럽트 통계

```bash
# NVMe 관련 인터럽트 카운트 (CPU별)
grep nvme /proc/interrupts

# 인터럽트 친화도(affinity) 확인
NVME_IRQS=$(grep nvme /proc/interrupts | awk '{print $1}' | tr -d ':')
for irq in $NVME_IRQS; do
    echo "IRQ $irq: affinity=$(cat /proc/irq/$irq/smp_affinity_list) node=$(cat /proc/irq/$irq/node 2>/dev/null)"
done

# 인터럽트 친화도 변경 (특정 CPU에 바인딩)
# echo 0 > /proc/irq/<IRQ_NUM>/smp_affinity_list

# softirq 통계
cat /proc/softirqs | head -5
```

### 9.3 IOMMU 정보

```bash
# IOMMU 그룹 구조
ls -la /sys/kernel/iommu_groups/

# NVMe 디바이스의 IOMMU 그룹
NVME_BDF=$(lspci | grep -i nvme | head -1 | awk '{print $1}')
readlink /sys/bus/pci/devices/0000:$NVME_BDF/iommu_group

# IOMMU 도메인 타입
cat /sys/bus/pci/devices/0000:$NVME_BDF/iommu_group/type

# DMA 마스크
cat /sys/bus/pci/devices/0000:$NVME_BDF/dma_mask_bits 2>/dev/null
cat /sys/bus/pci/devices/0000:$NVME_BDF/consistent_dma_mask_bits 2>/dev/null

# NUMA 노드
cat /sys/bus/pci/devices/0000:$NVME_BDF/numa_node
```

### 9.4 블록 디바이스 상태

```bash
# I/O 통계 (실시간)
iostat -xdm /dev/nvme0n1 1 5

# 디스크 통계 (커널 카운터)
cat /sys/block/nvme0n1/stat
# 필드 순서: read_ios read_merges read_sectors read_ticks
#            write_ios write_merges write_sectors write_ticks
#            in_flight io_ticks time_in_queue

# I/O 스케줄러 (NVMe는 보통 none)
cat /sys/block/nvme0n1/queue/scheduler

# 블록 디바이스 큐 파라미터
echo "nr_requests: $(cat /sys/block/nvme0n1/queue/nr_requests)"
echo "max_sectors_kb: $(cat /sys/block/nvme0n1/queue/max_sectors_kb)"
echo "read_ahead_kb: $(cat /sys/block/nvme0n1/queue/read_ahead_kb)"
echo "nomerges: $(cat /sys/block/nvme0n1/queue/nomerges)"
echo "io_poll: $(cat /sys/block/nvme0n1/queue/io_poll)"
```

---

## 10. Intel vs AMD 차이

### 10.1 IOMMU 구현 차이

| 항목 | Intel VT-d | AMD-Vi |
|------|-----------|--------|
| 부트 파라미터 | `intel_iommu=on` | `amd_iommu=on` |
| 커널 소스 | `drivers/iommu/intel/` | `drivers/iommu/amd/` |
| debugfs 경로 | `/sys/kernel/debug/iommu/intel/` | `/sys/kernel/debug/iommu/amd/` |
| Fault 로그 키워드 | `DMAR:.*fault` | `AMD-Vi:.*IO_PAGE_FAULT` |
| IOTLB flush | Posted Interrupts 지원 | 별도 메커니즘 |
| Interrupt Remapping | IR (Interrupt Remapping) | GAbits (Guest Address) |

### 10.2 IOMMU 확인 명령어 차이

```bash
# ===== Intel VT-d =====
# DMAR 테이블 확인
dmesg | grep "DMAR:"
dmesg | grep "DMAR-IR"  # Interrupt Remapping

# VT-d 상세 정보
cat /sys/kernel/debug/iommu/intel/dmar0/cap 2>/dev/null
cat /sys/kernel/debug/iommu/intel/dmar0/ecap 2>/dev/null

# Intel fault 로그
dmesg | grep -E "DMAR.*fault|DMAR.*error"

# ===== AMD-Vi =====
# AMD IOMMU 이벤트 로그
dmesg | grep "AMD-Vi:"
dmesg | grep "AMD-Vi.*Completion-Wait"

# AMD IOMMU 기능 확인
dmesg | grep "AMD-Vi: Features"

# AMD fault 로그
dmesg | grep "AMD-Vi.*IO_PAGE_FAULT"
```

### 10.3 PCIe/NVMe 관련 차이

```bash
# PCIe ACS(Access Control Services) 확인 (IOMMU 격리에 영향)
lspci -vvv -s $(lspci | grep -i nvme | awk '{print $1}') | grep -A5 "Access Control"

# ASPM(Active State Power Management) 상태 — Intel/AMD 구현이 다름
lspci -vvv -s $(lspci | grep -i nvme | awk '{print $1}') | grep -i "ASPM\|LnkCtl"

# CPU별 PCIe 토폴로지 확인
lstopo-no-graphics 2>/dev/null || lspci -tv
```

---

## 11. CentOS/Rocky Linux 특화 설정

### 11.1 SELinux 설정

```bash
# SELinux가 트레이싱을 차단할 수 있음
getenforce

# 임시 비활성화 (reboot하면 원복)
setenforce 0

# 영구 비활성화 (비추천, 필요시에만)
# sed -i 's/SELINUX=enforcing/SELINUX=permissive/' /etc/selinux/config

# SELinux 관련 거부 로그 확인
ausearch -m avc --start recent 2>/dev/null | tail -10

# bpftrace 사용 시 SELinux boolean 설정
setsebool -P bpf_unpriv_allowed 1 2>/dev/null
```

### 11.2 kernel-debuginfo 설치

CentOS/Rocky 9에서 bpftrace와 perf는 debuginfo 패키지가 없으면 커널 심볼을 해석하지 못한다.

```bash
# debuginfo 저장소 활성화
dnf config-manager --set-enabled *-debuginfo 2>/dev/null

# CentOS Stream 9
dnf install -y --enablerepo=*debuginfo kernel-debuginfo-$(uname -r)

# Rocky Linux 9
dnf install -y --enablerepo=devel kernel-debuginfo-$(uname -r)

# 설치 확인
rpm -qa | grep kernel-debuginfo

# BTF(BPF Type Format) 지원 확인 (최신 커널은 별도 debuginfo 없이 가능)
ls /sys/kernel/btf/vmlinux
bpftrace -e 'BEGIN { printf("BTF available\n"); exit(); }'

# debuginfo 없이 BTF만으로 동작 확인
# BTF가 있으면 대부분의 bpftrace 스크립트가 debuginfo 없이 동작한다
```

### 11.3 Secure Boot 관련

```bash
# Secure Boot 상태 확인
mokutil --sb-state

# Secure Boot가 활성화되어 있으면 unsigned 커널 모듈 로드 불가
# mmiotrace, kprobes 등이 제한될 수 있음

# lockdown 모드 확인
cat /sys/kernel/security/lockdown
# [none] integrity confidentiality
# integrity 이상이면 kprobes, ftrace 일부 기능 제한

# Secure Boot 환경에서의 대안
# 1) MOK(Machine Owner Key) 등록으로 커스텀 모듈 서명
# 2) lockdown=none 부트 파라미터 추가 (보안 위험 있음)
grubby --update-kernel=ALL --args="lockdown=none"

# kprobes 사용 가능 여부 확인
cat /sys/kernel/debug/kprobes/enabled 2>/dev/null
# 1 = 사용 가능, 없거나 0 = lockdown에 의해 차단됨
```

### 11.4 firewalld와 트레이싱

```bash
# firewalld가 네트워크 기반 트레이스 전송을 차단할 수 있음 (보통 로컬에서는 문제 없음)
# 원격 blktrace 사용 시
firewall-cmd --add-port=8462/tcp --permanent  # blktrace 기본 포트
firewall-cmd --reload
```

---

## 12. 목적별 추천 방법 요약표

### 12.1 "무엇을 알고 싶은가"에 따른 도구 선택

| 분석 목적 | 1순위 도구 | 2순위 도구 | 난이도 |
|-----------|-----------|-----------|--------|
| NVMe Doorbell 쓰기 관찰 | mmiotrace | - | 상 |
| I/O 제출~완료 레이턴시 | bpftrace | blktrace/btt | 중 |
| 함수 호출 체인 파악 | ftrace function_graph | trace-cmd | 중 |
| CPU 오버헤드 분석 | perf record/report | bpftrace | 중 |
| 인터럽트 분포 확인 | /proc/interrupts | bpftrace | 하 |
| DMA 매핑 검증 | DMA debug | bpftrace | 중 |
| IOMMU 동작 확인 | dmesg + iommu ftrace | /sys/kernel/debug/iommu | 중 |
| 블록 I/O 흐름 추적 | blktrace + blkparse | ftrace block events | 중 |
| I/O 패턴 시각화 | iowatcher | perf | 하 |
| 디바이스 상태/SMART | nvme-cli | /sys/class/nvme/ | 하 |
| 큐 깊이/스케줄러 확인 | /sys/block/ | nvme-cli | 하 |
| PCIe 토폴로지 | lspci -tv | lstopo | 하 |
| 캐시 미스 분석 | perf stat | - | 중 |

### 12.2 시나리오별 추천 조합

```
[시나리오 1] "I/O 레이턴시가 높은 원인을 찾고 싶다"
  1. iostat -xdm /dev/nvme0n1 1     → 전체 상황 파악
  2. blktrace → btt                  → Q2D(큐잉) vs D2C(디바이스) 분리
  3. bpftrace 레이턴시 히스토그램     → 분포 확인
  4. perf record                     → CPU 병목 확인

[시나리오 2] "NVMe 하드웨어 동작을 이해하고 싶다"
  1. lspci -vvv                      → BAR, MSI-X, ASPM 확인
  2. mmiotrace                       → Doorbell 쓰기 패턴
  3. /proc/interrupts                → 인터럽트 분포
  4. nvme id-ctrl / smart-log        → 컨트롤러 상태

[시나리오 3] "커널 코드 흐름을 추적하고 싶다"
  1. ftrace function_graph           → 호출 체인 전체 보기
  2. bpftrace kprobe                 → 특정 함수 인자/반환값
  3. perf probe                      → 동적 트레이스포인트 추가

[시나리오 4] "DMA/IOMMU 관련 문제를 디버깅하고 싶다"
  1. dmesg | grep DMAR/AMD-Vi        → fault 여부 확인
  2. DMA debug 활성화                → API 오용 검출
  3. IOMMU ftrace events             → map/unmap 추적
  4. bpftrace dma_map_sg             → 매핑 패턴 분석
```

### 12.3 성능 영향도

| 도구 | CPU 오버헤드 | I/O 영향 | 시스템 안정성 위험 |
|------|------------|---------|-----------------|
| /proc, /sys 읽기 | 무시 | 없음 | 없음 |
| iostat, nvme-cli | 무시 | 없음 | 없음 |
| perf stat | 매우 낮음 | 없음 | 없음 |
| perf record | 낮음 | 없음 | 없음 |
| blktrace | 낮음~중간 | 약간 | 없음 |
| ftrace events | 낮음 | 없음 | 없음 |
| ftrace function_graph | 중간 | 약간 | 없음 |
| bpftrace | 낮음~중간 | 거의 없음 | 없음 |
| DMA debug | 중간 | 약간 | 없음 |
| mmiotrace | **매우 높음** | **심각** | **단일 CPU 모드** |

### 12.4 빠른 참조: 원라이너 모음

```bash
# NVMe 전체 상태 한눈에 보기
echo "=== NVMe ===" && nvme list && echo "=== IRQ ===" && grep nvme /proc/interrupts | wc -l && echo "queues" && echo "=== Scheduler ===" && cat /sys/block/nvme0n1/queue/scheduler && echo "=== Queue Depth ===" && cat /sys/module/nvme/parameters/io_queue_depth

# 실시간 I/O 모니터링
watch -n1 'cat /sys/block/nvme0n1/stat'

# NVMe 인터럽트 초당 변화량
while true; do grep nvme /proc/interrupts; sleep 1; echo "---"; done

# 가장 빠른 레이턴시 측정 (bpftrace 원라이너)
bpftrace -e 'kprobe:nvme_queue_rq { @s[tid]=nsecs; } kprobe:nvme_pci_complete_rq /@s[tid]/ { @us=hist((nsecs-@s[tid])/1000); delete(@s[tid]); }'

# blk-mq 큐 매핑 확인
cat /sys/block/nvme0n1/mq/*/cpu_list 2>/dev/null

# NUMA-aware NVMe 확인
for d in /sys/class/nvme/nvme*/; do echo "$(basename $d): numa=$(cat $d/numa_node 2>/dev/null) model=$(cat $d/model 2>/dev/null)"; done
```

---

## 참고: 커널 소스 코드 위치

| 파일 | 역할 |
|------|------|
| `drivers/nvme/host/pci.c` | NVMe PCIe 드라이버 (module_param, Doorbell 쓰기, 인터럽트 처리) |
| `arch/x86/mm/mmio-mod.c` | mmiotrace 구현 (enable/disable, page fault 기반 MMIO 캡처) |
| `kernel/dma/debug.c` | DMA debug 프레임워크 (65536 엔트리 사전 할당, map/unmap 검증) |
| `block/blk-mq.c` | blk-mq 프레임워크 (멀티큐 블록 레이어) |
| `kernel/trace/trace.c` | ftrace 핵심 구현 |
| `drivers/iommu/intel/iommu.c` | Intel VT-d IOMMU 드라이버 |
| `drivers/iommu/amd/iommu.c` | AMD-Vi IOMMU 드라이버 |
