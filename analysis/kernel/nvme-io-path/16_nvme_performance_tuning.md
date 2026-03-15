# NVMe 성능 튜닝 실전 가이드 (커널 내부 기반)

이 문서는 Linux 커널 소스 코드 분석을 바탕으로 NVMe 스토리지의 성능을 최적화하는 실전 방법을 정리한다.
각 튜닝 항목마다 해당 커널 코드의 위치와 동작 원리를 설명하고, 즉시 적용 가능한 명령어를 제시한다.

---

## 1. NVMe 성능에 영향을 주는 커널 파라미터

### 1.1 I/O 스케줄러 선택

NVMe 디바이스의 I/O 스케줄러는 성능에 직접적인 영향을 미친다.
블록 레이어에서 스케줄러 유무에 따라 I/O 경로가 완전히 달라진다.

#### 스케줄러별 동작 원리

**none (스케줄러 없음) — NVMe에 최적**

스케줄러가 없으면 `blk_mq_submit_bio()`에서 가장 빠른 경로를 탄다.
커널 코드 `block/blk-mq.c`의 디스패치 분기점에서 확인할 수 있다:

```c
/* block/blk-mq.c: blk_mq_submit_bio() 내부 */
if ((rq->rq_flags & RQF_USE_SCHED) ||
    (hctx->dispatch_busy && (q->nr_hw_queues == 1 || !is_sync))) {
    /* 경로 B: 스케줄러 사용 → 소프트웨어 큐에 삽입 후 비동기 디스패치 */
    blk_mq_insert_request(rq, 0);
    blk_mq_run_hw_queue(hctx, true);
} else {
    /* 경로 C: 직접 디스패치 (★ NVMe 최단 경로!) */
    blk_mq_run_dispatch_ops(q, blk_mq_try_issue_directly(hctx, rq));
}
```

`none` 스케줄러일 때 `RQF_USE_SCHED` 플래그가 설정되지 않으므로, `blk_mq_try_issue_directly()` → `__blk_mq_issue_directly()` → `nvme_queue_rq()`로 바로 호출된다.
이것이 NVMe에서 달성 가능한 가장 낮은 소프트웨어 레이턴시 경로이다.

NVMe SSD는 내부에 자체 스케줄러(FTL)를 가지고 있어 호스트 측 스케줄링이 불필요하다.
하드웨어 큐가 CPU 수만큼 있으므로 소프트웨어 스케줄링의 이점이 거의 없다.

**mq-deadline — 읽기 우선, 기한 기반**

`block/mq-deadline.c`에 구현되어 있다. 핵심 파라미터:

```c
/* block/mq-deadline.c */
static const int read_expire = HZ / 2;   /* 읽기 최대 대기: 500ms */
static const int write_expire = 5 * HZ;  /* 쓰기 최대 대기: 5초 */
static const int writes_starved = 2;     /* 읽기가 쓰기를 최대 2번 기아시킬 수 있음 */
static const int fifo_batch = 16;        /* 연속 요청 16개를 하나의 배치로 처리 */
static const int prio_aging_expire = 10 * HZ;  /* 우선순위 에이징: 10초 */
```

mq-deadline은 읽기/쓰기 각각 RB 트리(섹터 순서)와 FIFO 리스트(시간 순서)를 유지한다.
3단계 우선순위(`DD_RT_PRIO`, `DD_BE_PRIO`, `DD_IDLE_PRIO`)를 지원한다.
NVMe에서 사용하면 읽기 레이턴시가 안정적이지만, 최대 IOPS는 `none` 대비 5~15% 감소한다.

**bfq (Budget Fair Queueing) — 공정 큐잉, 데스크탑 적합**

`block/bfq-iosched.c`에 구현되어 있다.
BFQ는 시간 슬라이스 대신 "예산(budget, 섹터 단위)"을 프로세스에 할당한다.
B-WF2Q+ 스케줄러로 가중치 기반 공정 대역폭 분배를 수행한다.

특징:
- 인터랙티브 애플리케이션과 소프트 리얼타임 애플리케이션을 자동 감지하여 우선순위를 높임 (weight-raising)
- cgroup 기반 계층적 스케줄링 지원
- NVMe에서 사용하면 공정성은 보장되지만 IOPS가 크게 떨어진다 (오버헤드가 큼)
- 데스크탑 환경에서 반응성이 중요할 때만 고려

**kyber — 레이턴시 기반 적응형 스로틀링**

`block/kyber-iosched.c`에 구현되어 있다 (Facebook 개발).
큐 깊이를 동적으로 조절하여 목표 레이턴시를 유지하는 방식이다:

```c
/* block/kyber-iosched.c */
static const unsigned int kyber_depth[] = {
    [KYBER_READ] = 256,      /* 읽기 최대 큐 깊이 */
    [KYBER_WRITE] = 128,     /* 쓰기 최대 큐 깊이 */
    [KYBER_DISCARD] = 64,    /* TRIM 최대 큐 깊이 */
    [KYBER_OTHER] = 16,      /* 기타 최대 큐 깊이 */
};

static const u64 kyber_latency_targets[] = {
    [KYBER_READ] = 2ULL * NSEC_PER_MSEC,     /* 읽기 목표: 2ms */
    [KYBER_WRITE] = 10ULL * NSEC_PER_MSEC,   /* 쓰기 목표: 10ms */
    [KYBER_DISCARD] = 5ULL * NSEC_PER_SEC,   /* TRIM 목표: 5초 */
};
```

레이턴시가 목표를 초과하면 자동으로 큐 깊이를 줄여 스로틀링한다.
NVMe에서 혼합 워크로드의 읽기 레이턴시를 안정화하는 데 효과적이다.

#### 스케줄러 변경 방법

```bash
# 현재 스케줄러 확인 (대괄호 안이 현재 설정)
cat /sys/block/nvme0n1/queue/scheduler
# 출력 예: [none] mq-deadline kyber bfq

# NVMe 최적 성능: none 설정
echo none > /sys/block/nvme0n1/queue/scheduler

# 영구 설정: udev 규칙
echo 'ACTION=="add|change", KERNEL=="nvme[0-9]*n[0-9]*", ATTR{queue/scheduler}="none"' \
    > /etc/udev/rules.d/60-nvme-scheduler.rules

# 모든 NVMe 디바이스에 일괄 적용
for dev in /sys/block/nvme*; do
    echo none > "$dev/queue/scheduler"
done
```

#### NVMe에서 스케줄러별 성능 비교 (참고 수치)

```
워크로드: 4K Random Read, iodepth=128, numjobs=4

스케줄러       IOPS        평균 레이턴시    P99 레이턴시
------------------------------------------------------
none          ~800K       ~640μs          ~1.2ms
mq-deadline   ~720K       ~710μs          ~1.5ms
kyber         ~700K       ~730μs          ~1.3ms
bfq           ~500K       ~1.0ms          ~2.5ms
```

### 1.2 큐 깊이 (Queue Depth)

큐 깊이는 NVMe 디바이스에 동시에 전송할 수 있는 커맨드 수를 결정한다.

#### io_queue_depth 모듈 파라미터

`drivers/nvme/host/pci.c`에 정의되어 있다:

```c
/* drivers/nvme/host/pci.c */
#define NVME_PCI_MIN_QUEUE_SIZE 2
#define NVME_PCI_MAX_QUEUE_SIZE 4095

static unsigned int io_queue_depth = 1024;   /* 기본값 */
module_param_cb(io_queue_depth, &io_queue_depth_ops, &io_queue_depth, 0644);
MODULE_PARM_DESC(io_queue_depth, "set io queue depth, should >= 2 and < 4096");

static int io_queue_depth_set(const char *val, const struct kernel_param *kp)
{
    return param_set_uint_minmax(val, kp, NVME_PCI_MIN_QUEUE_SIZE,
            NVME_PCI_MAX_QUEUE_SIZE);
}
```

기본값 1024는 대부분의 워크로드에 적합하다.
NVMe 스펙상 최대 큐 깊이는 65536이지만, 커널 드라이버는 4095로 제한한다 (0-based 인덱싱).
실제 사용 가능한 큐 깊이는 컨트롤러의 MQES(Maximum Queue Entries Supported) 값으로 결정된다.

#### blk-mq에서의 큐 깊이 처리

`block/blk-mq.c`의 `blk_mq_alloc_tag_set()`에서 큐 깊이를 설정한다:

```c
/* block/blk-mq.c */
/* set->queue_depth = 각 큐의 깊이 (보통 1023, NVMe 컨트롤러의 MQES에 의존) */
if (set->queue_depth > BLK_MQ_MAX_DEPTH) {
    set->queue_depth = BLK_MQ_MAX_DEPTH;
}

/* 메모리 부족 시 queue_depth를 절반으로 줄여서 재시도 */
do {
    /* tag 할당 시도... */
    set->queue_depth >>= 1;
    if (set->queue_depth < set->reserved_tags + BLK_MQ_TAG_MIN) {
        break;
    }
} while (set->queue_depth);
```

메모리가 부족하면 자동으로 큐 깊이를 줄인다.
`dmesg`에서 "reduced tag depth" 메시지로 확인 가능하다.

#### nr_requests 파라미터

```bash
# 현재 큐 깊이 확인
cat /sys/block/nvme0n1/queue/nr_requests
# 출력 예: 1023

# 큐 깊이 변경 (스케줄러가 none이 아닐 때만 의미 있음)
echo 256 > /sys/block/nvme0n1/queue/nr_requests
```

`nr_requests`는 블록 레이어 레벨의 큐 깊이이고, `io_queue_depth`는 NVMe 드라이버 레벨의 큐 깊이이다.
실제 적용되는 값은 둘 중 작은 값이다.

#### 최적 큐 깊이 결정 방법

```bash
# 큐 깊이별 성능 측정 스크립트
for depth in 1 2 4 8 16 32 64 128 256 512 1024; do
    echo "=== Queue Depth: $depth ==="
    fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
        --iodepth=$depth --numjobs=1 --direct=1 --runtime=10 \
        --group_reporting --name=qd_test 2>&1 | grep -E "IOPS|lat.*avg"
done
```

일반적인 경향:
- 큐 깊이 1~4: 디바이스가 유휴 상태, IOPS 낮음 (디바이스 내부 병렬성 활용 못함)
- 큐 깊이 8~64: IOPS가 급격히 증가 (디바이스의 내부 채널/다이 병렬성 활용)
- 큐 깊이 64~256: IOPS 포화 시작, 레이턴시 점진적 증가
- 큐 깊이 256+: IOPS 거의 변화 없음, 레이턴시만 증가, 메모리 낭비

```
                     IOPS
   1M ┤                          ┌─────────────────
      │                         ╱
      │                        ╱
 500K ┤                  ┌────╱
      │                 ╱
      │               ╱
 100K ┤         ┌────╱
      │        ╱
      │      ╱
  10K ┤ ────╱
      │
      └──┬──┬──┬──┬───┬───┬───┬───┬───┬────
         1  2  4  8  16  32  64 128 256 512  → Queue Depth

   Latency
   10ms ┤                              ╱────
        │                            ╱
    1ms ┤                    ┌──────╱
        │              ┌────╱
  100μs ┤ ─────────────╱
        │
   10μs ┤
        └──┬──┬──┬──┬───┬───┬───┬───┬───┬────
           1  2  4  8  16  32  64 128 256 512  → Queue Depth
```

#### 모듈 파라미터 변경 방법

```bash
# 현재 값 확인
cat /sys/module/nvme/parameters/io_queue_depth
# 출력: 1024

# 런타임 변경 (새로 probe되는 디바이스에만 적용)
echo 256 > /sys/module/nvme/parameters/io_queue_depth

# 부팅 시 커널 파라미터로 설정
# /etc/default/grub의 GRUB_CMDLINE_LINUX에 추가:
#   nvme.io_queue_depth=256

# modprobe 설정으로 영구 적용
echo "options nvme io_queue_depth=256" > /etc/modprobe.d/nvme.conf
```

### 1.3 하드웨어 큐 수

#### write_queues 모듈 파라미터

`drivers/nvme/host/pci.c`에서 읽기/쓰기 큐를 분리할 수 있다:

```c
/* drivers/nvme/host/pci.c */
/*
 * write_queues: 쓰기 전용 큐 개수. blk-mq의 HCTX_TYPE_READ 맵에 해당한다.
 * 0이면 읽기/쓰기가 같은 큐셋을 공유한다 (기본 동작).
 * 쓰기 전용 큐를 분리하면 읽기 지연시간이 쓰기에 의해 늘어나는 것을 방지할 수 있다.
 */
static unsigned int write_queues;
module_param_cb(write_queues, &io_queue_count_ops, &write_queues, 0644);
```

큐 매핑은 `nvme_pci_map_queues()`에서 수행된다:

```c
/* drivers/nvme/host/pci.c: nvme_pci_map_queues() */
/* 세 가지 큐 타입:
 * - HCTX_TYPE_DEFAULT: 기본 큐 (읽기+쓰기 공유, 또는 write_queues 설정 시 쓰기 전용)
 * - HCTX_TYPE_READ: 읽기 전용 큐 (write_queues > 0일 때만)
 * - HCTX_TYPE_POLL: 폴링 큐 (인터럽트 없음)
 */
```

```bash
# 쓰기 전용 큐 2개 설정 (혼합 워크로드에서 읽기 보호)
echo "options nvme write_queues=2" > /etc/modprobe.d/nvme.conf

# 적용 확인
cat /sys/block/nvme0n1/queue/nr_hw_queues
```

#### poll_queues 모듈 파라미터

```c
/* drivers/nvme/host/pci.c */
/*
 * poll_queues: 폴링 전용 큐 개수. 인터럽트를 사용하지 않고 CPU가 직접 CQ를 폴링한다.
 * 초저지연(ultra-low latency) I/O에 사용되며, io_uring의 IORING_SETUP_IOPOLL과 함께 쓴다.
 * SPDK가 모든 큐에서 폴링을 사용하는 것과 유사한 개념이다.
 */
static unsigned int poll_queues;
module_param_cb(poll_queues, &io_queue_count_ops, &poll_queues, 0644);
```

폴링 큐는 인터럽트가 필요 없으므로 IRQ 벡터를 할당하지 않는다:

```c
/* drivers/nvme/host/pci.c: nvme_setup_irqs() */
poll_queues = min(dev->nr_poll_queues, nr_io_queues - 1);
dev->io_queues[HCTX_TYPE_POLL] = poll_queues;

/* 폴링 큐를 제외한 큐에만 인터럽트 벡터 할당 */
irq_queues += (nr_io_queues - poll_queues);
```

```bash
# 폴링 큐 4개 설정
echo "options nvme poll_queues=4" > /etc/modprobe.d/nvme.conf

# 또는 커널 파라미터로
# nvme.poll_queues=4
```

#### CPU 수와 큐 수의 관계

`nvme_max_io_queues()`에서 최대 I/O 큐 수를 결정한다:

```c
/* drivers/nvme/host/pci.c */
static unsigned int nvme_max_io_queues(struct nvme_dev *dev)
{
    return blk_mq_num_possible_queues(0) + dev->nr_write_queues +
        dev->nr_poll_queues;
}
```

기본적으로 가능한 CPU 수만큼 I/O 큐를 생성하려고 시도한다.
실제 생성되는 큐 수는 `min(호스트 요청 수, 컨트롤러 지원 수, MSI-X 벡터 수)`로 결정된다.

```bash
# 현재 큐 수 확인
cat /sys/block/nvme0n1/queue/nr_hw_queues

# CPU 수 확인
nproc

# 큐 매핑 확인 (어떤 CPU가 어떤 큐를 사용하는지)
ls /sys/block/nvme0n1/mq/
# 0/  1/  2/  3/ ...

# 각 큐의 CPU 매핑
for q in /sys/block/nvme0n1/mq/*/cpu_list; do
    echo "$(dirname $q | xargs basename): $(cat $q)"
done
```

#### NUMA 노드 고려

NVMe 디바이스가 연결된 NUMA 노드와 I/O를 발생시키는 CPU의 NUMA 노드가 다르면 크로스 NUMA 메모리 접근이 발생하여 레이턴시가 증가한다.

```bash
# NVMe 디바이스의 NUMA 노드 확인
cat /sys/block/nvme0n1/device/numa_node

# CPU별 NUMA 노드 확인
lscpu | grep NUMA

# NUMA 토폴로지 확인
numactl --hardware

# 특정 NUMA 노드에서만 fio 실행
numactl --cpunodebind=0 --membind=0 \
    fio --filename=/dev/nvme0n1 --rw=randread --bs=4k \
    --ioengine=io_uring --iodepth=32 --numjobs=4 --direct=1 --name=test
```

---

## 2. IRQ Affinity 튜닝

### 2.1 NVMe 인터럽트 확인

```bash
# NVMe 인터럽트 확인
grep nvme /proc/interrupts
#            CPU0  CPU1  CPU2  CPU3
#  34:      1234     0     0     0  IR-PCI-MSI ...  nvme0q0    (Admin 큐)
#  35:      5678     0     0     0  IR-PCI-MSI ...  nvme0q1    (I/O 큐 1)
#  36:         0  5432     0     0  IR-PCI-MSI ...  nvme0q2    (I/O 큐 2)
#  37:         0     0  4321     0  IR-PCI-MSI ...  nvme0q3    (I/O 큐 3)
#  38:         0     0     0  3210  IR-PCI-MSI ...  nvme0q4    (I/O 큐 4)
```

`nvme0q0`은 Admin 큐로 관리 명령 전용이다.
`nvme0q1` ~ `nvme0qN`이 실제 I/O를 처리하는 큐이다.
각 큐의 인터럽트는 기본적으로 `pci_alloc_irq_vectors_affinity()`로 CPU에 분배된다.

### 2.2 IRQ Affinity 설정

커널은 `nvme_setup_irqs()`에서 `irq_affinity` 구조체를 사용하여 자동 affinity를 설정한다:

```c
/* drivers/nvme/host/pci.c: nvme_setup_irqs() */
struct irq_affinity affd = {
    .pre_vectors = 1,               /* Admin 큐용 벡터 1개 예약 */
    .calc_sets = nvme_calc_irq_sets, /* 읽기/쓰기 큐 벡터 수 계산 */
    .priv = dev,
};
unsigned int flags = PCI_IRQ_ALL_TYPES | PCI_IRQ_AFFINITY;

return pci_alloc_irq_vectors_affinity(pdev, 1, irq_queues, flags, &affd);
```

`PCI_IRQ_AFFINITY` 플래그로 커널이 자동으로 NUMA-aware IRQ 분배를 수행한다.

```bash
# 특정 IRQ의 affinity 확인 (비트마스크)
cat /proc/irq/35/smp_affinity
# 출력: 01  (CPU 0에 바인딩)

# affinity를 CPU 2로 변경
echo 4 > /proc/irq/35/smp_affinity  # 비트마스크: 0100 = CPU 2

# CPU 리스트 형식으로 확인/설정
cat /proc/irq/35/smp_affinity_list
echo "2" > /proc/irq/35/smp_affinity_list

# CPU 0,2에 바인딩
echo "0,2" > /proc/irq/35/smp_affinity_list
```

### 2.3 Managed IRQ vs Manual Affinity

커널은 기본적으로 "managed IRQ"를 사용한다.
managed IRQ는 커널이 자동으로 affinity를 관리하며, 사용자가 변경할 수 없다.

```bash
# managed IRQ인지 확인
cat /proc/irq/35/effective_affinity_list

# managed IRQ가 아닌 경우에만 수동 설정 가능
# managed IRQ를 사용하면 CPU hotplug 시에도 안전하게 동작한다
```

managed IRQ를 비활성화하려면 커널 파라미터 `pci=nomsi` 또는 특정 드라이버 옵션이 필요하다.
그러나 일반적으로 managed IRQ의 자동 분배가 최적에 가깝다.

### 2.4 NUMA-aware IRQ 배치

```
  NUMA Node 0                    NUMA Node 1
  ┌─────────────────────┐       ┌─────────────────────┐
  │ CPU 0  CPU 1        │       │ CPU 4  CPU 5        │
  │ CPU 2  CPU 3        │       │ CPU 6  CPU 7        │
  │                     │       │                     │
  │ Memory (DDR)        │       │ Memory (DDR)        │
  │                     │       │                     │
  │ PCIe Root Complex   │       │ PCIe Root Complex   │
  │   └── NVMe0 ★       │       │   └── NVMe1         │
  └─────────────────────┘       └─────────────────────┘

  ★ NVMe0의 IRQ는 Node 0의 CPU (0-3)에 바인딩해야 최적
    크로스 NUMA 접근 시 추가 레이턴시: ~100ns (QPI/UPI 전송)
```

```bash
# NVMe 디바이스의 NUMA 노드 확인
cat /sys/class/nvme/nvme0/device/numa_node
# 출력: 0

# 해당 NUMA 노드의 CPU 목록
cat /sys/devices/system/node/node0/cpulist
# 출력: 0-3

# NVMe0의 모든 I/O 큐 IRQ를 Node 0 CPU에 바인딩하는 스크립트
#!/bin/bash
NVME_DEV="nvme0"
NUMA_NODE=$(cat /sys/class/nvme/$NVME_DEV/device/numa_node)
CPULIST=$(cat /sys/devices/system/node/node$NUMA_NODE/cpulist)

for irq in $(grep "$NVME_DEV" /proc/interrupts | awk '{print $1}' | tr -d ':'); do
    echo "$CPULIST" > /proc/irq/$irq/smp_affinity_list 2>/dev/null
done
```

### 2.5 irqbalance 데몬과의 상호작용

`irqbalance`는 주기적으로 IRQ를 재분배하므로 수동 설정과 충돌할 수 있다.

```bash
# irqbalance 상태 확인
systemctl status irqbalance

# 방법 1: irqbalance에서 NVMe IRQ 제외
# /etc/sysconfig/irqbalance 또는 /etc/default/irqbalance에 추가:
IRQBALANCE_BANNED_IRQS="35 36 37 38"

# 방법 2: irqbalance 완전 비활성화 (NVMe 전용 서버)
systemctl stop irqbalance
systemctl disable irqbalance

# 방법 3: irqbalance를 NUMA hint 모드로만 사용
# irqbalance --hintpolicy=exact
```

---

## 3. I/O 인터페이스별 성능

### 3.1 fio 벤치마크 설정

#### ioengine 선택: libaio vs io_uring

```bash
# libaio: 전통적인 Linux AIO (submit + getevents 2회 시스템콜)
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=libaio \
    --iodepth=128 --numjobs=4 --direct=1 --runtime=30 \
    --group_reporting --name=libaio_test

# io_uring: 최신 비동기 I/O (공유 메모리 링, 시스템콜 최소화)
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=128 --numjobs=4 --direct=1 --runtime=30 \
    --group_reporting --name=iouring_test
```

io_uring이 libaio보다 빠른 이유:
- SQ/CQ 링을 커널과 사용자 공간이 공유 → 데이터 복사 최소화
- `IORING_ENTER_GETEVENTS`와 제출을 하나의 시스템콜로 처리 가능
- Fixed buffers/files로 반복적인 setup 오버헤드 제거
- SQPOLL 모드로 시스템콜 자체를 제거 가능

#### direct=1이 필수인 이유

`direct=1`(O_DIRECT)은 페이지 캐시를 우회하여 NVMe 디바이스 성능을 직접 측정한다.
`direct=0`이면 I/O가 페이지 캐시에 흡수되어 실제 디바이스 성능이 아닌 메모리 성능을 측정하게 된다.

#### iodepth와 numjobs 설정

```bash
# iodepth: 하나의 job이 동시에 제출하는 I/O 수
# numjobs: 병렬로 실행되는 fio 프로세스/스레드 수
# 총 outstanding I/O = iodepth × numjobs

# 케이스 1: 단일 스레드 고 큐 깊이
fio --iodepth=256 --numjobs=1  # 총 256 outstanding I/O

# 케이스 2: 다중 스레드 적당한 큐 깊이
fio --iodepth=32 --numjobs=8   # 총 256 outstanding I/O, CPU 분산

# 케이스 2가 보통 더 나은 이유:
# - blk-mq의 여러 하드웨어 큐를 활용 (큐 간 lock 경합 없음)
# - 여러 CPU의 캐시를 활용
# - NUMA 노드 분산 가능
```

권장: `numjobs`는 NVMe가 연결된 NUMA 노드의 CPU 코어 수 이하로 설정하고, `iodepth`는 디바이스가 IOPS 포화에 도달하는 최소값으로 설정한다.

#### 실전 fio 명령어 예시

```bash
# 기본 성능 프로파일링 (4가지 패턴)
fio --filename=/dev/nvme0n1 --direct=1 --ioengine=io_uring \
    --group_reporting --runtime=60 --time_based \
    --stonewall \
    --name=seq_read --rw=read --bs=128k --iodepth=32 --numjobs=4 \
    --name=seq_write --rw=write --bs=128k --iodepth=32 --numjobs=4 \
    --name=rand_read --rw=randread --bs=4k --iodepth=128 --numjobs=4 \
    --name=rand_write --rw=randwrite --bs=4k --iodepth=128 --numjobs=4
```

### 3.2 io_uring 최적화

#### IORING_SETUP_SQPOLL: 커널 스레드가 SQ 폴링

SQPOLL 모드에서는 커널 스레드(`io_uring-sq`)가 SQ를 지속적으로 폴링하여 새 엔트리를 가져간다.
애플리케이션은 SQ에 엔트리를 넣기만 하면 시스템콜 없이 I/O가 제출된다.

```bash
# SQPOLL 모드 fio
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=64 --numjobs=1 --direct=1 --sqthread_poll=1 \
    --name=sqpoll_test

# SQPOLL 스레드 확인
ps -eLf | grep io_uring-sq
```

성능 영향:
- 시스템콜 오버헤드 제거 (submit 경로에서 ~200ns 절감)
- CPU 1코어를 전담으로 사용하므로 CPU 효율성은 떨어짐
- 초고성능이 필요한 단일 디바이스 시나리오에 적합

#### IORING_SETUP_IOPOLL: NVMe 폴링 모드

IOPOLL은 NVMe의 `poll_queues`와 연동하여 인터럽트 없이 I/O 완료를 감지한다.
커널 코드에서 `nvme_poll()` 함수가 호출된다:

```c
/* drivers/nvme/host/pci.c: nvme_poll() */
static int nvme_poll(struct blk_mq_hw_ctx *hctx, struct io_comp_batch *iob)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    bool found;

    if (!nvme_cqe_pending(nvmeq))
        return 0;

    spin_lock(&nvmeq->cq_poll_lock);
    found = nvme_poll_cq(nvmeq, iob);
    spin_unlock(&nvmeq->cq_poll_lock);

    return found;
}
```

```bash
# IOPOLL 사용을 위해 poll_queues 먼저 설정
echo "options nvme poll_queues=4" > /etc/modprobe.d/nvme.conf
modprobe -r nvme && modprobe nvme  # 드라이버 재로드 (주의: 마운트 해제 필요)

# IOPOLL 모드 fio
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=16 --numjobs=1 --direct=1 --hipri=1 \
    --name=iopoll_test
```

#### SQPOLL + IOPOLL 동시 사용

```bash
# 최소 레이턴시 설정: SQPOLL + IOPOLL
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=1 --numjobs=1 --direct=1 --hipri=1 --sqthread_poll=1 \
    --name=ultra_low_latency
```

이 설정은 제출(SQPOLL)과 완료(IOPOLL) 모두에서 시스템콜과 인터럽트를 제거한다.
SPDK의 전체 폴링 모델과 가장 유사한 커널 I/O 경로이다.

#### Fixed Buffers와 Fixed Files

```c
// io_uring에서 Fixed Buffer 사용 예시 (C 코드)
struct iovec iovecs[BUFFER_COUNT];
for (int i = 0; i < BUFFER_COUNT; i++) {
    iovecs[i].iov_base = aligned_alloc(4096, BUFFER_SIZE);
    iovecs[i].iov_len = BUFFER_SIZE;
}

// 버퍼 등록: 커널이 페이지를 pin하고 DMA 매핑을 미리 수행
io_uring_register_buffers(&ring, iovecs, BUFFER_COUNT);

// 파일 등록: 매번 fd lookup하지 않고 인덱스로 접근
int fds[] = {fd};
io_uring_register_files(&ring, fds, 1);
```

Fixed Buffer의 성능 효과:
- 매 I/O마다 `get_user_pages()` + DMA 매핑 불필요 → ~500ns 절감
- 특히 작은 I/O(4K)에서 전체 레이턴시 대비 비율이 크므로 효과가 크다

```bash
# fio에서 Fixed Buffer 사용
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=64 --numjobs=4 --direct=1 --fixedbufs=1 \
    --registerfiles=1 --name=fixed_buf_test
```

### 3.3 Polling 모드 상세

#### 인터럽트 vs 폴링 코드 경로 비교

```
인터럽트 경로 (기본):
  디바이스 CQE 기록 → MSI-X 인터럽트 발생
  → CPU 인터럽트 수신 (~1-3μs)
  → nvme_irq() 호출
  → nvme_poll_cq() → nvme_handle_cqe()
  → nvme_complete_rq() → 상위 레이어 완료 콜백
  총 오버헤드: ~5-10μs (인터럽트 처리 포함)

폴링 경로 (poll_queues 설정 시):
  디바이스 CQE 기록
  → io_uring이 nvme_poll() 반복 호출
  → nvme_cqe_pending() → Phase Tag 확인
  → nvme_poll_cq() → nvme_handle_cqe()
  → nvme_complete_rq()
  총 오버헤드: ~1-3μs (인터럽트 없음)
```

```
  ┌──────────────────────────────────────────────────────┐
  │              레이턴시 비교 (4K Random Read)             │
  ├──────────────────────────────────────────────────────┤
  │                                                      │
  │  인터럽트 모드:  ├────────┤ ~8-12μs                     │
  │                 ├──┤ HW   ├──┤ IRQ  ├──┤ SW           │
  │                                                      │
  │  폴링 모드:     ├──────┤ ~4-7μs                        │
  │                 ├──┤ HW  ├─┤ Poll                     │
  │                                                      │
  │  개선: 약 30-50% 레이턴시 감소                           │
  └──────────────────────────────────────────────────────┘
```

#### poll_queues 설정 가이드

```bash
# poll_queues 수 결정 기준:
# - 폴링 I/O를 사용할 스레드 수와 같거나 약간 많게
# - 총 큐 수(인터럽트 + 폴링)가 CPU 수를 초과하지 않도록
# - 인터럽트 큐를 최소 1개 이상 남겨야 함

# 예: 16 CPU 시스템에서 폴링 4개, 인터럽트 12개
echo "options nvme poll_queues=4" > /etc/modprobe.d/nvme.conf

# 적용 후 확인
dmesg | grep "nvme.*queues"
# 예: nvme nvme0: 12/0/4 default/read/poll queues
```

---

## 4. 메모리/DMA 관련 튜닝

### 4.1 Hugepage 활용 (TLB Miss 감소)

NVMe I/O에서 DMA 매핑 시 `get_user_pages()`로 사용자 버퍼의 물리 주소를 확인한다.
Hugepage(2MB/1GB)를 사용하면 페이지 테이블 엔트리 수가 줄어 TLB miss가 감소한다.

```bash
# Hugepage 설정
echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
# 2GB의 hugepage 풀 생성 (1024 × 2MB)

# 또는 커널 파라미터로 부팅 시 설정
# hugepages=1024 hugepagesz=2M

# Transparent Hugepage (THP) 활성화
echo always > /sys/kernel/mm/transparent_hugepage/enabled

# fio에서 hugepage 사용
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=128 --numjobs=4 --direct=1 --hugepage-size=2m \
    --name=hugepage_test
```

### 4.2 IOMMU Passthrough 모드

IOMMU가 활성화되면 모든 DMA 요청이 IOMMU 페이지 테이블을 거친다.
NVMe처럼 신뢰할 수 있는 디바이스에는 passthrough 모드로 오버헤드를 제거할 수 있다.

```bash
# IOMMU 상태 확인
dmesg | grep -i iommu

# Passthrough 모드 설정 (커널 파라미터)
# Intel: intel_iommu=on iommu=pt
# AMD:   amd_iommu=on iommu=pt

# /etc/default/grub의 GRUB_CMDLINE_LINUX에 추가:
GRUB_CMDLINE_LINUX="... iommu=pt"

# 적용
grub-mkconfig -o /boot/grub/grub.cfg
```

IOMMU passthrough의 효과:
- DMA 매핑 시 IOMMU 페이지 테이블 walk 제거 → ~100-200ns 절감
- 특히 작은 I/O(4K)가 많은 워크로드에서 효과가 크다
- 보안 측면에서 DMA 공격에 노출될 수 있으므로, 신뢰할 수 있는 환경에서만 사용

### 4.3 SWIOTLB 비활성화 조건

SWIOTLB(Software I/O Translation Lookaside Buffer)는 DMA 가능 메모리 범위를 벗어나는 버퍼를 위해 바운스 버퍼를 사용한다.
64비트 시스템에서 NVMe는 모든 물리 메모리에 DMA가 가능하므로 SWIOTLB가 불필요한 경우가 많다.

```bash
# SWIOTLB 사용 여부 확인
dmesg | grep swiotlb

# SWIOTLB 비활성화 (64비트 시스템, IOMMU 없을 때)
# 커널 파라미터: swiotlb=0

# 주의: 일부 시스템에서는 SWIOTLB가 필수일 수 있음
# - 32비트 DMA만 지원하는 구형 디바이스가 있는 경우
# - kdump 커널에서 제한된 메모리를 사용하는 경우
```

### 4.4 NUMA Locality

```bash
# NVMe 디바이스의 NUMA 노드와 같은 노드의 메모리 사용 강제
numactl --cpunodebind=0 --membind=0 -- \
    fio --filename=/dev/nvme0n1 --rw=randread --bs=4k \
    --ioengine=io_uring --iodepth=128 --numjobs=4 --direct=1 --name=test

# NUMA 통계 확인
numastat -c

# 크로스 NUMA 메모리 접근 모니터링
perf stat -e node-loads,node-load-misses -a -- sleep 5
```

---

## 5. NVMe 디바이스 설정 (Set Features)

NVMe 스펙은 `Set Features` 명령(opcode 0x09)으로 디바이스 동작을 세밀하게 조정할 수 있다.
커널에서 `nvme_set_features()` 함수로 구현되어 있다:

```c
/* drivers/nvme/host/core.c */
int nvme_set_features(struct nvme_ctrl *dev, unsigned int fid,
                      unsigned int dword11, void *buffer, size_t buflen,
                      void *result)
{
    return nvme_features(dev, nvme_admin_set_features, fid, dword11, buffer,
                         buflen, result);
}
```

### 5.1 Interrupt Coalescing (FID 0x08)

```c
/* include/linux/nvme.h */
NVME_FEAT_IRQ_COALESCE = 0x08,  /* 인터럽트 병합: 여러 완료를 모아서 한 번에 인터럽트 */
```

인터럽트 코얼레싱은 여러 CQE가 누적될 때까지 인터럽트 발생을 지연시킨다.
인터럽트 수를 줄여 CPU 오버헤드를 감소시키지만, 개별 I/O 레이턴시는 증가한다.

```bash
# 현재 인터럽트 코얼레싱 설정 확인
nvme get-feature /dev/nvme0 -f 0x08

# 인터럽트 코얼레싱 설정
# dword11 = (TIME[15:8] | THR[7:0])
# THR: 인터럽트 발생 전 누적할 CQE 수 - 1 (0 = 즉시 인터럽트)
# TIME: 최대 대기 시간 (100μs 단위)
nvme set-feature /dev/nvme0 -f 0x08 -v 0x0A07
# TIME=10 (1ms), THR=7 (8개 CQE 누적 후 인터럽트)

# 대역폭 워크로드에서 권장 (레이턴시 민감하지 않을 때):
nvme set-feature /dev/nvme0 -f 0x08 -v 0x1410
# TIME=20 (2ms), THR=16 (17개 CQE 누적)

# 레이턴시 민감 워크로드에서 권장 (비활성화):
nvme set-feature /dev/nvme0 -f 0x08 -v 0x0000
# TIME=0, THR=0 → 즉시 인터럽트
```

### 5.2 Arbitration (FID 0x01)

```c
/* include/linux/nvme.h */
NVME_FEAT_ARBITRATION = 0x01,  /* 중재 메커니즘 설정 (High/Medium/Low 가중치) */
```

NVMe 컨트롤러가 여러 SQ에서 커맨드를 가져올 때의 우선순위를 설정한다.

```bash
# Arbitration 설정 확인
nvme get-feature /dev/nvme0 -f 0x01

# dword11 구성:
# [31:24] HPW - High Priority Weight
# [23:16] MPW - Medium Priority Weight
# [15:8]  LPW - Low Priority Weight
# [2:0]   AB  - Arbitration Burst (2^N 커맨드)

# 기본값 복원 (Round Robin)
nvme set-feature /dev/nvme0 -f 0x01 -v 0x00000007
# AB=7: 각 큐에서 최대 128개 커맨드를 연속 처리
```

### 5.3 Power Management (APST)

```c
/* include/linux/nvme.h */
NVME_FEAT_POWER_MGMT = 0x02,  /* 전력 관리: 전력 상태 선택 */
NVME_FEAT_AUTO_PST   = 0x0c,  /* 자동 전력 상태 전환 */
```

커널은 `nvme_configure_apst()`에서 APST(Autonomous Power State Transition)를 자동 설정한다:

```c
/* drivers/nvme/host/core.c */
ret = nvme_set_features(ctrl, NVME_FEAT_AUTO_PST, apste,
                        table, sizeof(*table), NULL);
```

APST는 유휴 시 자동으로 저전력 상태로 전환한다.
서버 환경에서는 전환 레이턴시 때문에 성능이 불안정해질 수 있다.

```bash
# 현재 전력 상태 확인
nvme get-feature /dev/nvme0 -f 0x02

# 전력 상태를 최대 성능(PS0)으로 고정
nvme set-feature /dev/nvme0 -f 0x02 -v 0

# APST 비활성화 (서버 환경 권장)
nvme set-feature /dev/nvme0 -f 0x0c -v 0

# 또는 커널 파라미터로 비활성화
# nvme_core.default_ps_max_latency_us=0
```

### 5.4 Temperature Threshold (FID 0x04)

```bash
# 온도 임계값 확인
nvme get-feature /dev/nvme0 -f 0x04

# 온도 임계값 설정 (켈빈 단위)
# 70°C = 343K
nvme set-feature /dev/nvme0 -f 0x04 -v 343

# 현재 온도 확인
nvme smart-log /dev/nvme0 | grep temperature
```

### 5.5 Volatile Write Cache (FID 0x06)

```bash
# 쓰기 캐시 상태 확인
nvme get-feature /dev/nvme0 -f 0x06

# 쓰기 캐시 활성화 (성능 우선, 전원 손실 시 데이터 유실 가능)
nvme set-feature /dev/nvme0 -f 0x06 -v 1

# 쓰기 캐시 비활성화 (데이터 안전성 우선)
nvme set-feature /dev/nvme0 -f 0x06 -v 0
```

---

## 6. 커널 설정

### 6.1 필수 커널 컴파일 옵션

```
# NVMe 드라이버 기본 (필수)
CONFIG_BLK_DEV_NVME=y      # 또는 =m (모듈)
# drivers/nvme/host/Kconfig:
# "NVM Express block device" - PCI & BLOCK 의존

# NVMe 멀티패스 지원 (다중 경로 SSD)
CONFIG_NVME_MULTIPATH=y
# 여러 PCIe 포트로 연결된 NVMe에 대해 경로 자동 전환
# /dev/nvme0n1, /dev/nvme1n1 대신 /dev/nvme0c0n1 형태로 통합

# I/O 폴링 지원 (필수: poll_queues, io_uring IOPOLL)
CONFIG_BLK_DEV_IO_POLL=y
# 이 옵션이 없으면 폴링 관련 기능이 전혀 동작하지 않음

# io_uring 지원
CONFIG_IO_URING=y

# BFQ 스케줄러 (데스크탑용으로 빌드할 때)
CONFIG_MQ_IOSCHED_DEADLINE=y
CONFIG_MQ_IOSCHED_KYBER=y
CONFIG_MQ_IOSCHED_BFQ=y

# NVMe 하드웨어 모니터링 (온도/전력 등)
CONFIG_NVME_HWMON=y
```

### 6.2 Preempt vs No-preempt 커널

커널의 Preemption 모델은 NVMe I/O 레이턴시에 영향을 미친다:

```
CONFIG_PREEMPT_NONE=y      # 서버용: 최소 컨텍스트 스위칭, 최대 처리량
CONFIG_PREEMPT_VOLUNTARY=y  # 균형: 적절한 반응성 + 처리량
CONFIG_PREEMPT=y           # 데스크탑용: 최소 레이턴시, 처리량 약간 감소
CONFIG_PREEMPT_RT=y        # 실시간: 결정론적 레이턴시, 처리량 크게 감소
```

NVMe 서버 워크로드에서는 `CONFIG_PREEMPT_NONE`이 가장 높은 IOPS를 달성한다.
Preemption이 활성화되면 `nvme_queue_rq()` 실행 중에도 선점될 수 있어 SQ 접근의 `spin_lock` 대기 시간이 증가할 수 있다.

```bash
# 현재 커널의 preemption 모델 확인
cat /boot/config-$(uname -r) | grep PREEMPT
# 또는
uname -v | grep -i preempt
```

### 6.3 기타 커널 파라미터

```bash
# 추가 성능 최적화 커널 파라미터 (/etc/default/grub)
GRUB_CMDLINE_LINUX="... \
    iommu=pt \
    intel_idle.max_cstate=0 \
    processor.max_cstate=0 \
    idle=poll \
    nosoftlockup \
    nohz_full=2-7 \
    rcu_nocbs=2-7 \
    isolcpus=2-7"

# intel_idle.max_cstate=0: CPU가 깊은 절전 상태에 들어가지 않음
#   → 인터럽트 응답 시간 안정화 (C6 wake-up ~100μs 제거)
# idle=poll: CPU가 유휴 시에도 폴링 (극한 레이턴시 환경)
# nohz_full: 지정된 CPU에서 타이머 인터럽트 최소화
# isolcpus: 지정된 CPU를 일반 스케줄링에서 제외 (전용 I/O용)
```

---

## 7. 실전 벤치마크 시나리오

### 7.1 최대 IOPS (4K Random Read)

```bash
# 목표: 디바이스의 최대 랜덤 읽기 IOPS 측정
# 핵심: 충분한 큐 깊이 + 다중 CPU 활용 + 폴링 없음 (인터럽트 모드)
fio --filename=/dev/nvme0n1 \
    --rw=randread \
    --bs=4k \
    --ioengine=io_uring \
    --iodepth=128 \
    --numjobs=4 \
    --direct=1 \
    --group_reporting \
    --runtime=60 \
    --time_based \
    --name=max_iops_test

# 최적화 체크리스트:
# 1. echo none > /sys/block/nvme0n1/queue/scheduler
# 2. numactl --cpunodebind=<NVMe NUMA node>
# 3. iodepth × numjobs ≥ 디바이스 최대 outstanding I/O
# 4. direct=1 (필수)
```

### 7.2 최소 레이턴시 (4K Random Read, Polling)

```bash
# 목표: 가능한 가장 낮은 4K 읽기 레이턴시
# 핵심: QD=1 + 폴링 + SQPOLL + 단일 스레드
# 사전 설정: nvme.poll_queues=4 (모듈 파라미터)

fio --filename=/dev/nvme0n1 \
    --rw=randread \
    --bs=4k \
    --ioengine=io_uring \
    --iodepth=1 \
    --numjobs=1 \
    --direct=1 \
    --hipri=1 \
    --sqthread_poll=1 \
    --fixedbufs=1 \
    --registerfiles=1 \
    --runtime=60 \
    --time_based \
    --name=min_latency_test

# 추가 최적화:
# 1. CPU C-state 비활성화: intel_idle.max_cstate=0
# 2. CPU frequency 고정: cpupower frequency-set -g performance
# 3. irqbalance 중지
# 4. taskset으로 NVMe NUMA 노드 CPU에 바인딩
taskset -c 0 fio ...  # NVMe가 NUMA 0에 연결된 경우
```

### 7.3 최대 대역폭 (128K Sequential Read)

```bash
# 목표: 최대 순차 읽기 대역폭 (GB/s)
# 핵심: 큰 블록 크기 + 적당한 큐 깊이 + 다중 스레드

fio --filename=/dev/nvme0n1 \
    --rw=read \
    --bs=128k \
    --ioengine=io_uring \
    --iodepth=32 \
    --numjobs=4 \
    --direct=1 \
    --group_reporting \
    --runtime=60 \
    --time_based \
    --name=max_bw_test

# PCIe 대역폭 한계 참고:
# PCIe 3.0 x4: ~3.5 GB/s
# PCIe 4.0 x4: ~7.0 GB/s
# PCIe 5.0 x4: ~14.0 GB/s
```

### 7.4 Mixed Workload (70/30 Read/Write)

```bash
# 목표: 읽기 70% / 쓰기 30% 혼합 워크로드 성능 측정
# 데이터베이스, 웹서버 등 실제 워크로드와 유사

fio --filename=/dev/nvme0n1 \
    --rw=randrw \
    --rwmixread=70 \
    --bs=4k \
    --ioengine=io_uring \
    --iodepth=64 \
    --numjobs=4 \
    --direct=1 \
    --group_reporting \
    --runtime=60 \
    --time_based \
    --name=mixed_workload

# write_queues를 사용한 읽기/쓰기 분리 테스트
# 먼저: echo "options nvme write_queues=2" > /etc/modprobe.d/nvme.conf
# write_queues를 설정하면 쓰기 I/O가 읽기 큐를 방해하지 않음
```

### 7.5 Steady State 테스트 (SSD 프리컨디셔닝 포함)

```bash
# SSD는 초기에는 빠르지만 GC(Garbage Collection)가 시작되면 느려진다
# Steady State에서의 성능이 실제 운영 환경의 성능이다

# Step 1: 전체 디바이스 2회 순차 쓰기 (프리컨디셔닝)
fio --filename=/dev/nvme0n1 --rw=write --bs=128k \
    --ioengine=io_uring --iodepth=32 --numjobs=1 --direct=1 \
    --size=100% --loops=2 --name=precondition

# Step 2: 30분간 Random Write (Steady State 진입)
fio --filename=/dev/nvme0n1 --rw=randwrite --bs=4k \
    --ioengine=io_uring --iodepth=64 --numjobs=4 --direct=1 \
    --runtime=1800 --time_based --name=steady_state_write

# Step 3: 실제 벤치마크
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k \
    --ioengine=io_uring --iodepth=128 --numjobs=4 --direct=1 \
    --runtime=60 --time_based --name=steady_state_read
```

---

## 8. 성능 디버깅

### 8.1 blktrace / blkparse

blktrace는 블록 레이어의 이벤트를 추적하여 I/O가 어디에서 지연되는지 분석한다.

```bash
# blktrace 실행 (10초간 추적)
blktrace -d /dev/nvme0n1 -o trace -w 10 &
# 동시에 워크로드 실행
fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=32 --numjobs=1 --direct=1 --runtime=10 --name=trace_test

# blkparse로 분석
blkparse -i trace -o trace.txt

# 주요 이벤트 코드:
# Q - 큐에 삽입 (bio 생성)
# G - request 할당 (tag 확보)
# I - 스케줄러에 삽입
# D - 드라이버에 디스패치 (nvme_queue_rq 호출)
# C - 완료 (nvme_irq 또는 nvme_poll)
#
# Q→D 지연: 소프트웨어 오버헤드 (스케줄러, plug 등)
# D→C 지연: 하드웨어 처리 시간 + 인터럽트 지연

# btt (Block Trace Timing) 분석
btt -i trace.blktrace.0 -l trace.lat

# 레이턴시 히스토그램
blkparse -i trace | awk '/C/{print $8}' | sort -n | \
    awk '{sum+=$1; count++} END {print "avg:", sum/count, "ms"}'
```

### 8.2 bpftrace로 레이턴시 측정

```bash
# NVMe I/O 레이턴시 히스토그램 (커널 진입부터 완료까지)
bpftrace -e '
kprobe:nvme_queue_rq {
    @start[arg1] = nsecs;
}
kprobe:nvme_pci_complete_rq {
    $s = @start[arg0];
    if ($s > 0) {
        @latency_us = hist((nsecs - $s) / 1000);
        delete(@start[arg0]);
    }
}
interval:s:10 { exit(); }
'

# blk-mq 디스패치 경로별 카운트
bpftrace -e '
kprobe:blk_mq_try_issue_directly { @direct = count(); }
kprobe:blk_add_rq_to_plug { @plug = count(); }
kprobe:blk_mq_insert_request { @sched = count(); }
interval:s:5 {
    printf("direct: %lld, plug: %lld, sched: %lld\n",
           @direct, @plug, @sched);
    clear(@direct); clear(@plug); clear(@sched);
}
'

# NVMe 큐별 I/O 분포 확인
bpftrace -e '
kprobe:nvme_queue_rq {
    $hctx = (struct blk_mq_hw_ctx *)arg0;
    @queue_dist = hist($hctx->queue_num);
}
interval:s:10 { exit(); }
'

# nvme_queue_rq 내부 시간 분석 (DMA 매핑 + SQ 복사)
bpftrace -e '
kprobe:nvme_prep_rq { @prep_start[tid] = nsecs; }
kretprobe:nvme_prep_rq {
    if (@prep_start[tid] > 0) {
        @prep_ns = hist(nsecs - @prep_start[tid]);
        delete(@prep_start[tid]);
    }
}
interval:s:10 { exit(); }
'
```

### 8.3 perf로 CPU 병목 분석

```bash
# NVMe I/O 경로의 CPU 프로파일링
perf record -g -a -F 99 -- \
    fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=128 --numjobs=4 --direct=1 --runtime=10 --name=perf_test

# 결과 분석
perf report --sort=dso,symbol

# NVMe 관련 함수만 필터링
perf report --sort=symbol | grep -E "nvme|blk_mq|io_uring"

# 핫스팟 함수 예시:
# nvme_queue_rq         - I/O 제출 (SQ 복사 + Doorbell)
# nvme_irq              - 인터럽트 핸들러
# nvme_poll_cq          - CQ 폴링
# blk_mq_submit_bio     - bio→request 변환
# sbitmap_get            - tag 할당

# lock contention 분석
perf lock record -- \
    fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=128 --numjobs=4 --direct=1 --runtime=10 --name=lock_test
perf lock report

# 캐시 미스 분석
perf stat -e cache-misses,cache-references,instructions,cycles \
    fio --filename=/dev/nvme0n1 --rw=randread --bs=4k --ioengine=io_uring \
    --iodepth=128 --numjobs=4 --direct=1 --runtime=10 --name=cache_test
```

### 8.4 /sys/block/nvme0n1/stat 해석

```bash
# I/O 통계 확인
cat /sys/block/nvme0n1/stat
# 출력 형식:
# read_ios read_merges read_sectors read_ticks
# write_ios write_merges write_sectors write_ticks
# in_flight io_ticks time_in_queue
# discard_ios discard_merges discard_sectors discard_ticks
# flush_ios flush_ticks

# 필드 설명 스크립트
cat /sys/block/nvme0n1/stat | awk '{
    printf "읽기 완료 I/O:      %s\n", $1
    printf "읽기 병합:          %s\n", $2
    printf "읽기 섹터:          %s (%.2f GB)\n", $3, $3*512/1024/1024/1024
    printf "읽기 소요시간:      %s ms\n", $4
    printf "쓰기 완료 I/O:      %s\n", $5
    printf "쓰기 병합:          %s\n", $6
    printf "쓰기 섹터:          %s (%.2f GB)\n", $7, $7*512/1024/1024/1024
    printf "쓰기 소요시간:      %s ms\n", $8
    printf "진행 중 I/O:        %s\n", $9
    printf "I/O 활성 시간:      %s ms\n", $10
    printf "큐 대기 시간:       %s ms\n", $11
}'

# 1초 간격 IOPS/BW 모니터링 스크립트
while true; do
    read r1 _ _ _ w1 _ _ _ _ _ _ < /sys/block/nvme0n1/stat
    sleep 1
    read r2 _ _ _ w2 _ _ _ _ _ _ < /sys/block/nvme0n1/stat
    echo "Read IOPS: $((r2-r1)), Write IOPS: $((w2-w1))"
done
```

### 8.5 nvme smart-log 확인

```bash
# SMART 로그 전체 확인
nvme smart-log /dev/nvme0

# 주요 필드:
# critical_warning        - 0이 아니면 주의 필요
# temperature             - 현재 온도 (켈빈)
# available_spare         - 여유 블록 비율 (%)
# data_units_read         - 총 읽기 (1단위 = 512KB × 1000)
# data_units_written      - 총 쓰기
# host_read_commands      - 총 읽기 커맨드 수
# host_write_commands     - 총 쓰기 커맨드 수
# power_on_hours          - 전원 ON 시간
# media_errors            - 미디어 에러 수
# percentage_used         - 수명 소모율 (100% = 보증 수명 도달)

# 성능 저하 감지 포인트:
# 1. temperature가 70°C 이상 → 서멀 스로틀링 가능
# 2. available_spare < available_spare_threshold → 블록 부족
# 3. percentage_used > 90% → 교체 시기
# 4. media_errors 증가 → 디바이스 불량

# 에러 로그 확인
nvme error-log /dev/nvme0

# 펌웨어 로그 확인
nvme fw-log /dev/nvme0
```

---

## 9. 커널 코드에서 성능에 영향을 주는 핫패스

### 9.1 nvme_queue_rq()의 각 단계별 비용

`nvme_queue_rq()`는 NVMe I/O의 핵심 제출 함수이다 (`drivers/nvme/host/pci.c`).
각 단계의 상대적 비용을 분석한다:

```c
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
                                  const struct blk_mq_queue_data *bd)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    struct nvme_dev *dev = nvmeq->dev;
    struct request *req = bd->rq;
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
    blk_status_t ret;

    /* [비용: ~5ns] 큐 활성화 상태 확인 */
    if (unlikely(!test_bit(NVMEQ_ENABLED, &nvmeq->flags)))
        return BLK_STS_IOERR;

    /* [비용: ~5ns] 컨트롤러 준비 상태 확인 */
    if (unlikely(!nvme_check_ready(&dev->ctrl, req, true)))
        return nvme_fail_nonready_command(&dev->ctrl, req);

    /* [비용: ~200-500ns] ★ 요청 준비 (가장 비싼 단계)
     * - NVMe 커맨드 빌드 (opcode, LBA, 길이 설정)
     * - DMA 매핑: get_user_pages() + dma_map_sg()
     * - PRP/SGL 리스트 구성
     * 이 단계에서 IOMMU 활성화 시 추가 ~100ns */
    ret = nvme_prep_rq(req);
    if (unlikely(ret))
        return ret;

    /* [비용: ~50-100ns] SQ에 커맨드 복사 + Doorbell
     * spin_lock: 같은 큐에 여러 CPU가 접근할 때 경합
     * nvme_sq_copy_cmd: 64바이트 memcpy (L1 캐시 히트 시 ~10ns)
     * nvme_write_sq_db: MMIO write (~30-50ns, PCIe posted write) */
    spin_lock(&nvmeq->sq_lock);
    nvme_sq_copy_cmd(nvmeq, &iod->cmd);
    nvme_write_sq_db(nvmeq, bd->last);
    spin_unlock(&nvmeq->sq_lock);
    return BLK_STS_OK;
}
```

```
  nvme_queue_rq() 시간 분석 (대략적 수치):
  ┌────────────────────────────────────────────────────────────┐
  │                                                            │
  │  ├── 상태 확인 ──┤├───── nvme_prep_rq() ─────┤├── SQ+DB ──┤│
  │  │     ~10ns     ││      ~200-500ns          ││  ~50-100ns ││
  │  │               ││                          ││            ││
  │  │ test_bit      ││ cmd build + DMA map      ││ spinlock   ││
  │  │ nvme_check    ││ PRP/SGL list             ││ memcpy 64B ││
  │  │               ││ get_user_pages           ││ MMIO write ││
  │  │               ││ dma_map_sg               ││            ││
  │                                                            │
  │  총: ~260-610ns (소프트웨어 제출 비용)                        │
  └────────────────────────────────────────────────────────────┘
```

### 9.2 nvme_queue_rqs() 배치 제출의 최적화

```c
/* drivers/nvme/host/pci.c */
static void nvme_queue_rqs(struct rq_list *rqlist)
{
    struct rq_list submit_list = { };
    struct nvme_queue *nvmeq = NULL;
    struct request *req;

    while ((req = rq_list_pop(rqlist))) {
        /* 같은 큐의 요청들을 모은다 */
        if (nvmeq && nvmeq != req->mq_hctx->driver_data)
            nvme_submit_cmds(nvmeq, &submit_list);
        nvmeq = req->mq_hctx->driver_data;
        /* ... */
    }
    /* 모인 요청들을 한 번에 제출 */
    if (nvmeq)
        nvme_submit_cmds(nvmeq, &submit_list);
}

static void nvme_submit_cmds(struct nvme_queue *nvmeq, struct rq_list *rqlist)
{
    spin_lock(&nvmeq->sq_lock);          /* ★ 한 번만 lock */
    while ((req = rq_list_pop(rqlist)))
        nvme_sq_copy_cmd(nvmeq, &iod->cmd);  /* N개 커맨드 복사 */
    nvme_write_sq_db(nvmeq, true);       /* ★ Doorbell 한 번만 */
    spin_unlock(&nvmeq->sq_lock);
}
```

배치 제출의 성능 이점:
- `spin_lock` 획득/해제: N회 → 1회
- Doorbell MMIO write: N회 → 1회 (~30-50ns × (N-1) 절감)
- 총 절감: N개 요청 배치 시 `~(N-1) × 80ns`

### 9.3 blk_mq_submit_bio()의 merge/plug 오버헤드

```c
/* block/blk-mq.c: blk_mq_submit_bio() 내부 핫패스 */

/* [비용: ~50-100ns] 병합 시도 */
if (blk_mq_attempt_bio_merge(q, bio, nr_segs))
    goto queue_exit;  /* 병합 성공 시 여기서 종료 (새 request 불필요) */

/* [비용: ~100-200ns] request 할당 + tag 확보 */
rq = blk_mq_get_new_requests(q, plug, bio);
/* 내부에서 sbitmap_get()으로 빈 tag를 찾음.
 * tag가 부족하면 대기 → 이 대기 시간이 레이턴시를 크게 증가시킬 수 있음 */

/* [비용: ~20-50ns] bio→request 변환 */
blk_mq_bio_to_request(rq, bio, nr_segs);

/* [비용: ~10-20ns] plug 리스트에 추가 (plug이 있을 때) */
blk_add_rq_to_plug(plug, rq);
/* plug이 가득 차면 (~BLK_MAX_REQUEST_COUNT=32) 자동 flush → 배치 디스패치 */
```

#### plug의 성능 영향

```
  Plug 없는 경우 (I/O 하나씩):
  ┌─────────────────────────────────────────────────┐
  │ bio → request → nvme_queue_rq() → doorbell      │
  │ bio → request → nvme_queue_rq() → doorbell      │ 매번 doorbell
  │ bio → request → nvme_queue_rq() → doorbell      │
  └─────────────────────────────────────────────────┘

  Plug 있는 경우 (배치):
  ┌─────────────────────────────────────────────────┐
  │ bio → request → plug 리스트 추가                  │
  │ bio → request → plug 리스트 추가                  │ doorbell 안 울림
  │ bio → request → plug 리스트 추가                  │
  │ ──── blk_finish_plug() ────                      │
  │ nvme_queue_rqs(전체 리스트) → doorbell 한 번       │ ★ 배치 처리
  └─────────────────────────────────────────────────┘
```

### 9.4 인터럽트 vs 폴링 코드 경로 차이

```c
/* 인터럽트 경로 */
static irqreturn_t nvme_irq(int irq, void *data)    /* ~50ns 진입 오버헤드 */
{
    struct nvme_queue *nvmeq = data;
    DEFINE_IO_COMP_BATCH(iob);

    if (nvme_poll_cq(nvmeq, &iob)) {                /* ~20ns per CQE */
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob);           /* 배치 완료 */
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

/* 폴링 경로 */
static int nvme_poll(struct blk_mq_hw_ctx *hctx, struct io_comp_batch *iob)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    bool found;

    if (!nvme_cqe_pending(nvmeq))                    /* ~5ns Phase 확인 */
        return 0;

    spin_lock(&nvmeq->cq_poll_lock);                 /* ~10ns */
    found = nvme_poll_cq(nvmeq, iob);                /* ~20ns per CQE */
    spin_unlock(&nvmeq->cq_poll_lock);

    return found;
}
```

인터럽트 경로 추가 비용:
- MSI-X 인터럽트 전달: ~1-3μs (CPU에 도달하기까지)
- 인터럽트 핸들러 진입: ~50-100ns (레지스터 저장, 스택 전환)
- 인터럽트 핸들러 퇴장: ~50-100ns (레지스터 복원)
- 총 오버헤드: ~1.1-3.2μs

폴링 경로 추가 비용:
- `nvme_cqe_pending()` 반복 호출: 각 ~5ns (캐시 히트 시)
- 완료까지 평균 폴링 횟수에 따라 CPU 소비 증가
- 하지만 레이턴시 자체는 인터럽트보다 ~1-3μs 낮음

### 9.5 Lock Contention 포인트

NVMe 핫패스에서 lock이 사용되는 지점과 경합 가능성:

```
  Lock 1: nvmeq->sq_lock (spinlock)
  ├── 위치: nvme_queue_rq(), nvme_submit_cmds()
  ├── 보호 대상: SQ tail 포인터, SQ 엔트리 쓰기, Doorbell
  ├── 경합 조건: 같은 큐에 여러 CPU가 동시에 I/O 제출
  └── 완화: blk-mq가 CPU별로 큐를 매핑하므로 보통 경합 없음
            (nr_hw_queues >= CPU 수일 때)

  Lock 2: nvmeq->cq_poll_lock (spinlock)
  ├── 위치: nvme_poll(), nvme_poll_irqdisable()
  ├── 보호 대상: CQ head 포인터, CQ 엔트리 읽기
  ├── 경합 조건: 폴링 큐에서 여러 CPU가 동시에 완료 폴링
  └── 완화: 폴링 큐는 보통 1:1 CPU 매핑

  Lock 3: sbitmap (per-word atomic)
  ├── 위치: blk_mq_get_tag() → sbitmap_get()
  ├── 보호 대상: tag(command ID) 할당/해제
  ├── 경합 조건: 큐 깊이가 낮고 I/O가 많을 때
  └── 완화: sbitmap은 word 단위로 분산되어 경합 최소화
```

```bash
# lock contention 실시간 모니터링
perf lock record -a -- sleep 10
perf lock report

# 또는 bpftrace로 sq_lock 대기 시간 측정
bpftrace -e '
kprobe:nvme_queue_rq {
    @sq_lock_start[tid] = nsecs;
}
kretprobe:nvme_queue_rq {
    if (@sq_lock_start[tid]) {
        @total_ns = hist(nsecs - @sq_lock_start[tid]);
        delete(@sq_lock_start[tid]);
    }
}
interval:s:10 { exit(); }
'
```

---

## 10. 종합 튜닝 체크리스트

서버 환경에서 NVMe 최대 성능을 위한 체크리스트:

```bash
#!/bin/bash
# nvme_tune.sh - NVMe 성능 튜닝 스크립트

NVME_DEV="nvme0n1"

echo "=== NVMe Performance Tuning ==="

# 1. I/O 스케줄러: none
echo none > /sys/block/$NVME_DEV/queue/scheduler
echo "[1] Scheduler: $(cat /sys/block/$NVME_DEV/queue/scheduler)"

# 2. readahead 비활성화 (랜덤 I/O 워크로드)
echo 0 > /sys/block/$NVME_DEV/queue/read_ahead_kb
echo "[2] Read ahead: $(cat /sys/block/$NVME_DEV/queue/read_ahead_kb) KB"

# 3. 큐 정보 확인
echo "[3] HW Queues: $(cat /sys/block/$NVME_DEV/queue/nr_hw_queues)"
echo "[3] Queue Depth: $(cat /sys/block/$NVME_DEV/queue/nr_requests)"

# 4. rq_affinity 설정 (2 = 제출한 CPU에서 완료 처리)
echo 2 > /sys/block/$NVME_DEV/queue/rq_affinity
echo "[4] RQ Affinity: $(cat /sys/block/$NVME_DEV/queue/rq_affinity)"

# 5. nomerges 설정 (순수 랜덤 I/O일 때)
# echo 2 > /sys/block/$NVME_DEV/queue/nomerges
echo "[5] Nomerges: $(cat /sys/block/$NVME_DEV/queue/nomerges)"

# 6. CPU performance governor
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo performance > "$cpu" 2>/dev/null
done
echo "[6] CPU Governor: performance"

# 7. NUMA 정보
NUMA_NODE=$(cat /sys/block/$NVME_DEV/device/numa_node)
echo "[7] NUMA Node: $NUMA_NODE"

# 8. irqbalance 중지
systemctl stop irqbalance 2>/dev/null
echo "[8] irqbalance: stopped"

# 9. NVMe APST 비활성화
nvme set-feature /dev/${NVME_DEV%n*} -f 0x0c -v 0 2>/dev/null
echo "[9] APST: disabled"

# 10. 인터럽트 코얼레싱 비활성화 (저레이턴시)
nvme set-feature /dev/${NVME_DEV%n*} -f 0x08 -v 0x0000 2>/dev/null
echo "[10] IRQ Coalescing: disabled"

echo "=== Tuning Complete ==="
```

### 커널 부트 파라미터 (최대 성능)

```
# /etc/default/grub
GRUB_CMDLINE_LINUX="iommu=pt \
    intel_idle.max_cstate=0 \
    processor.max_cstate=0 \
    nvme.io_queue_depth=1024 \
    nvme.poll_queues=4 \
    nvme_core.default_ps_max_latency_us=0 \
    transparent_hugepage=always"
```

### 모듈 파라미터 (성능 프로파일별)

```bash
# /etc/modprobe.d/nvme.conf

# 프로파일 1: 최대 IOPS (기본 설정이 대부분 최적)
options nvme io_queue_depth=1024

# 프로파일 2: 최소 레이턴시
options nvme io_queue_depth=256 poll_queues=4

# 프로파일 3: 혼합 워크로드 (읽기 보호)
options nvme io_queue_depth=1024 write_queues=2

# 프로파일 4: 초저지연 (SPDK에 가까운 설정)
options nvme io_queue_depth=64 poll_queues=8 use_threaded_interrupts=0
```

---

## 부록: 커널 소스 참조 위치

| 항목 | 파일 | 핵심 함수/변수 |
|------|------|---------------|
| NVMe 모듈 파라미터 | `drivers/nvme/host/pci.c:125-210` | `io_queue_depth`, `write_queues`, `poll_queues` |
| I/O 제출 (핫패스) | `drivers/nvme/host/pci.c:1675` | `nvme_queue_rq()` |
| 배치 제출 | `drivers/nvme/host/pci.c:1763` | `nvme_queue_rqs()` |
| 인터럽트 핸들러 | `drivers/nvme/host/pci.c:1995` | `nvme_irq()` |
| CQ 폴링 | `drivers/nvme/host/pci.c:1955` | `nvme_poll_cq()` |
| 블록 폴링 콜백 | `drivers/nvme/host/pci.c:2047` | `nvme_poll()` |
| 큐 매핑 | `drivers/nvme/host/pci.c:763` | `nvme_pci_map_queues()` |
| IRQ 설정 | `drivers/nvme/host/pci.c:3437` | `nvme_setup_irqs()` |
| I/O 큐 설정 | `drivers/nvme/host/pci.c:3502` | `nvme_setup_io_queues()` |
| blk-mq 진입점 | `block/blk-mq.c:3700` | `blk_mq_submit_bio()` |
| 직접 디스패치 | `block/blk-mq.c:3194` | `__blk_mq_issue_directly()` |
| tag set 할당 | `block/blk-mq.c:5532` | `blk_mq_alloc_tag_set()` |
| Set Features | `drivers/nvme/host/core.c:2100` | `nvme_set_features()` |
| APST 설정 | `drivers/nvme/host/core.c:3519` | `nvme_configure_apst()` |
| mq-deadline | `block/mq-deadline.c:30-39` | `read_expire`, `write_expire` |
| kyber 스케줄러 | `block/kyber-iosched.c:60-74` | `kyber_depth[]`, `kyber_latency_targets[]` |
| BFQ 스케줄러 | `block/bfq-iosched.c` | B-WF2Q+ 스케줄러 |
| NVMe Feature ID | `include/linux/nvme.h:1716-1727` | `NVME_FEAT_*` |
| blk-mq ops | `drivers/nvme/host/pci.c:2777` | `nvme_mq_ops` |
