# NVMe PCIe 드라이버 내부 구조 완전 분석

> 분석 대상: Linux Kernel NVMe PCIe Host Driver
> 주요 파일: `drivers/nvme/host/pci.c`, `drivers/nvme/host/core.c`, `drivers/nvme/host/nvme.h`
> 스펙 헤더: `include/linux/nvme.h`
> ioctl 인터페이스: `drivers/nvme/host/ioctl.c`


## 1. NVMe 드라이버 아키텍처 개요

### 1.1 NVMe 스펙과 Linux 드라이버의 대응 관계

NVMe(Non-Volatile Memory Express) 스펙은 PCIe 버스를 통해 SSD와 통신하기 위한 프로토콜이다. 이 스펙이 정의하는 핵심 개념과 Linux 커널 코드의 대응 관계는 다음과 같다.

| NVMe 스펙 개념 | Linux 커널 구현 | 파일 |
|---|---|---|
| NVMe Controller | `struct nvme_ctrl` + `struct nvme_dev` | nvme.h, pci.c |
| Namespace | `struct nvme_ns` + `struct nvme_ns_head` | nvme.h |
| Submission Queue (SQ) | `nvme_queue->sq_cmds` (DMA coherent 메모리) | pci.c |
| Completion Queue (CQ) | `nvme_queue->cqes` (DMA coherent 메모리) | pci.c |
| Doorbell Register | `nvme_queue->q_db` (MMIO 주소) | pci.c |
| Admin Queue | `nvme_dev->queues[0]` | pci.c |
| I/O Queue | `nvme_dev->queues[1..N]` | pci.c |
| Command (64바이트) | `struct nvme_command` (union) | include/linux/nvme.h |
| Completion Entry (16바이트) | `struct nvme_completion` | include/linux/nvme.h |
| BAR0 레지스터 | `nvme_dev->bar` (ioremap된 MMIO) | pci.c |
| PRP (Physical Region Page) | `nvme_pci_setup_data_prp()` | pci.c |
| SGL (Scatter Gather List) | `nvme_pci_setup_data_sgl()` | pci.c |
| Shadow Doorbell | `nvme_queue->dbbuf_sq_db/dbbuf_cq_db` | pci.c |


### 1.2 core.c vs pci.c 분리 구조

Linux NVMe 드라이버는 transport-independent 계층과 transport-specific 계층으로 분리되어 있다. 이 설계 덕분에 PCIe, RDMA, TCP, Fibre Channel 등 다양한 전송 방식을 하나의 코어 코드로 지원한다.

```
┌─────────────────────────────────────────────────────────┐
│                    사용자 공간                            │
│            (fio, dd, nvme-cli, io_uring)                 │
├─────────────────────────────────────────────────────────┤
│                   VFS / 파일시스템                        │
├─────────────────────────────────────────────────────────┤
│               Block Layer (blk-mq)                      │
│         request_queue ←→ blk_mq_tag_set                 │
├─────────────────────────────────────────────────────────┤
│              NVMe Core (core.c)                         │
│     nvme_ctrl, nvme_ns, nvme_setup_cmd()                │
│     Identify, Feature 설정, 네임스페이스 관리             │
│                                                         │
│     struct nvme_ctrl_ops  ← 전송 드라이버 콜백 인터페이스  │
├──────────┬──────────┬──────────┬────────────────────────┤
│  PCIe    │  RDMA    │  TCP     │  FC                    │
│ (pci.c)  │(rdma.c)  │ (tcp.c)  │ (fc.c)                │
│ MMIO     │ RDMA CM  │ TCP sock │ Fibre Channel          │
│ MSI-X    │ IB verbs │ sendmsg  │ FC Exchange            │
└──────────┴──────────┴──────────┴────────────────────────┘
```

**core.c의 역할 (transport-independent)**:
- `nvme_setup_cmd()`: block request를 NVMe 커맨드로 변환 (opcode, LBA, 길이 설정)
- `nvme_complete_rq()`: 완료된 요청의 에러 처리 및 재시도 로직
- `nvme_init_ctrl_finish()`: Identify Controller 커맨드 발행, 기능 협상
- 네임스페이스 스캔/관리, 전원 관리, sysfs 인터페이스
- `struct nvme_ctrl_ops`를 통해 전송 드라이버에 위임

**pci.c의 역할 (PCIe transport-specific)**:
- PCIe BAR0 MMIO 레지스터 접근 (`readl`/`writel`)
- DMA coherent 메모리 할당 (SQ/CQ 버퍼)
- MSI-X 인터럽트 핸들러 등록 및 처리
- `nvme_queue_rq()`: SQ에 커맨드 복사 + Doorbell 쓰기
- `nvme_irq()`/`nvme_poll()`: CQ에서 완료 엔트리 수확
- PRP/SGL DMA 매핑


### 1.3 nvme_dev, nvme_ctrl, nvme_ns 관계도

```
┌─────────────────────────────────────────────────────────────┐
│                    struct pci_dev                            │
│                  (PCI 서브시스템 디바이스)                     │
└──────────────┬──────────────────────────────────────────────┘
               │ pci_set_drvdata()
               ▼
┌─────────────────────────────────────────────────────────────┐
│                   struct nvme_dev                            │
│              (PCIe 전송 전용 디바이스 구조체)                  │
│                                                             │
│  bar ─────────→ BAR0 MMIO 매핑 (컨트롤러 레지스터)           │
│  dbs ─────────→ Doorbell 레지스터 시작 (BAR0 + 0x1000)      │
│  queues[] ────→ nvme_queue 배열 (아래 참조)                  │
│  tagset ──────→ blk_mq_tag_set (I/O 큐용)                  │
│  admin_tagset → blk_mq_tag_set (Admin 큐용)                │
│  dbbuf_dbs ──→ Shadow Doorbell 버퍼                         │
│                                                             │
│  ┌──────────────────────────────────────────────────┐       │
│  │          struct nvme_ctrl  (공통 컨트롤러)         │       │
│  │                                                  │       │
│  │  ops ──→ nvme_pci_ctrl_ops (전송 콜백)            │       │
│  │  admin_q → request_queue (Admin 커맨드용)          │       │
│  │  tagset ──→ &nvme_dev.tagset                     │       │
│  │  namespaces → list of nvme_ns                    │       │
│  │  state: LIVE / RESETTING / CONNECTING / ...      │       │
│  │  cap: CAP 레지스터 값 (MQES, DSTRD 등)            │       │
│  └──────────────┬───────────────────────────────────┘       │
└─────────────────┼───────────────────────────────────────────┘
                  │ namespaces 리스트
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    struct nvme_ns                            │
│                (네임스페이스 = 논리 디스크)                    │
│                                                             │
│  ctrl ────→ 소속 nvme_ctrl                                  │
│  queue ───→ request_queue (이 NS의 I/O 큐)                  │
│  disk ────→ gendisk → /dev/nvme0n1                          │
│  head ────→ nvme_ns_head (멀티패스 앵커)                     │
│                                                             │
│  ┌──────────────────────────────────────────────┐           │
│  │         struct nvme_ns_head                   │           │
│  │  ns_id: NSID (1, 2, ...)                     │           │
│  │  lba_shift: LBA 크기 (9=512B, 12=4KB)        │           │
│  │  disk → /dev/nvme0c0n1 (멀티패스 디바이스)     │           │
│  └──────────────────────────────────────────────┘           │
└─────────────────────────────────────────────────────────────┘
```

**queues[] 배열 구조**:
```
queues[0]  ─→ Admin Queue      (QID=0, Identify/Feature/Queue 관리)
queues[1]  ─→ I/O Queue #1     (QID=1, 데이터 읽기/쓰기)
queues[2]  ─→ I/O Queue #2     (QID=2)
  ...
queues[N]  ─→ I/O Queue #N     (QID=N)
queues[N+1]─→ Poll Queue #1    (인터럽트 없음, CPU 직접 폴링)
```


## 2. 드라이버 초기화 경로

### 2.1 nvme_probe() 라인바이라인 분석

`nvme_probe()`는 커널이 PCI 디바이스를 발견했을 때 호출되는 진입점이다. PCI ID 테이블(`nvme_id_table`)의 class 매칭(NVMe class code `0x010802`)에 의해 모든 NVMe PCIe 디바이스에서 호출된다.

```c
static int nvme_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct nvme_dev *dev;
    int result = -ENOMEM;

    /* ── 1단계: nvme_dev 구조체 할당 ── */
    /* NUMA 노드 인지 할당으로 메모리 접근 지연 최소화 */
    /* 내부에서 nvme_init_ctrl()을 호출하여 nvme_ctrl도 초기화 */
    /* quirks 설정, DMA 마스크(64비트) 설정, 큐 배열 할당 */
    dev = nvme_pci_alloc_dev(pdev, id);
    if (IS_ERR(dev))
        return PTR_ERR(dev);

    /* ── 2단계: nvme_core에 컨트롤러 등록 ── */
    /* /dev/nvme0 캐릭터 디바이스 생성 (ioctl 인터페이스) */
    /* sysfs 엔트리 생성 (/sys/class/nvme/nvme0) */
    result = nvme_add_ctrl(&dev->ctrl);
    if (result)
        goto out_put_ctrl;

    /* ── 3단계: PCIe BAR0 MMIO 매핑 ── */
    /* pci_request_mem_regions(): BAR 영역을 이 드라이버 전용으로 예약 */
    /* ioremap(): BAR0 물리 주소를 커널 가상 주소로 매핑 */
    /* 최소 매핑 크기: 0x1000 (레지스터) + 4096 (최소 Doorbell 영역) */
    result = nvme_dev_map(dev);
    if (result)
        goto out_uninit_ctrl;

    /* ── 4단계: PRP DMA 벡터 메모리풀 할당 ── */
    /* PRP 리스트의 DMA unmap을 위한 매핑 정보 저장용 */
    result = nvme_pci_alloc_iod_mempool(dev);
    if (result)
        goto out_dev_unmap;

    /* ── 5단계: PCI 디바이스 활성화 + Admin Queue 설정 ── */
    /* pci_enable_device_mem(): PCI 명령 레지스터의 Memory Space 비트 활성화 */
    /* pci_set_master(): 버스 마스터 활성화 (DMA 가능) */
    /* CAP 레지스터 읽기 → 큐 깊이, Doorbell 간격 결정 */
    /* nvme_pci_configure_admin_queue(): 아래 2.2절 참조 */
    result = nvme_pci_enable(dev);
    if (result)
        goto out_release_iod_mempool;

    /* ── 6단계: Admin 큐용 blk-mq 태그셋 생성 ── */
    /* blk_mq_alloc_tag_set()으로 Admin 큐의 요청 관리 구조 생성 */
    /* blk_mq_init_queue()로 Admin request_queue 생성 */
    /* 이 시점부터 Admin 커맨드(Identify, Feature 등) 발행 가능 */
    result = nvme_alloc_admin_tag_set(&dev->ctrl, &dev->admin_tagset,
                &nvme_mq_admin_ops, sizeof(struct nvme_iod));
    if (result)
        goto out_disable;

    /* ── 7단계: 상태를 CONNECTING으로 전환 ── */
    if (!nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_CONNECTING)) {
        result = -EBUSY;
        goto out_disable;
    }

    /* ── 8단계: Identify Controller + 기능 협상 ── */
    /* Admin 큐로 Identify Controller 커맨드를 전송하여 디바이스 정보 수집 */
    /* MDTS(Maximum Data Transfer Size), ONCS(Optional NVM Commands) 등 확인 */
    /* Set Features로 Number of Queues, 온도 임계값 등 설정 */
    result = nvme_init_ctrl_finish(&dev->ctrl, false);
    if (result)
        goto out_disable;

    /* ── 9단계: Shadow Doorbell Buffer 할당 ── */
    /* 컨트롤러가 OACS.DBBUF을 지원하면 DMA coherent 메모리 할당 */
    nvme_dbbuf_dma_alloc(dev);

    /* ── 10단계: HMB 설정 (DRAM-less SSD용) ── */
    result = nvme_setup_host_mem(dev);
    if (result < 0)
        goto out_disable;

    /* ── 11단계: I/O 큐 생성 ── */
    /* Set Number of Queues → MSI-X 벡터 할당 → Create CQ/SQ */
    result = nvme_setup_io_queues(dev);
    if (result)
        goto out_disable;

    /* ── 12단계: I/O 큐용 blk-mq 태그셋 생성 ── */
    if (dev->online_queues > 1) {
        nvme_alloc_io_tag_set(&dev->ctrl, &dev->tagset, &nvme_mq_ops,
                nvme_pci_nr_maps(dev), sizeof(struct nvme_iod));
        nvme_dbbuf_set(dev);  /* Shadow Doorbell 활성화 커맨드 전송 */
    }

    /* ── 13단계: 컨트롤러 상태를 LIVE로 전환 ── */
    if (!nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_LIVE)) {
        result = -ENODEV;
        goto out_disable;
    }

    pci_set_drvdata(pdev, dev);

    /* ── 14단계: 컨트롤러 시작 → 네임스페이스 스캔 ── */
    /* nvme_start_ctrl() 내부에서 scan_work를 스케줄링 */
    /* 스캔 워크가 각 네임스페이스에 대해 gendisk를 생성하고 add_disk() 호출 */
    /* 이 시점부터 /dev/nvme0n1 등이 사용자에게 보인다 */
    nvme_start_ctrl(&dev->ctrl);
    nvme_put_ctrl(&dev->ctrl);
    flush_work(&dev->ctrl.scan_work);
    return 0;

    /* 에러 시 역순 정리 (goto chain) */
out_disable:
    nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_DELETING);
    nvme_dev_disable(dev, true);
    /* ... 리소스 해제 ... */
}
```


### 2.2 Admin Queue 생성: nvme_pci_configure_admin_queue()

Admin Queue는 NVMe 드라이버 초기화의 핵심 단계이다. I/O Queue는 Admin 커맨드(Create I/O CQ/SQ)로 생성하는데, Admin Queue 자체는 BAR0 레지스터에 직접 설정해야 한다. "닭이 먼저냐 달걀이 먼저냐" 문제의 해결책이다.

```
NVMe 스펙 초기화 순서:
  1. CAP 레지스터 읽기 (컨트롤러 능력 확인)
  2. CC.EN = 0 (컨트롤러 비활성화)
  3. CSTS.RDY = 0 대기 (비활성화 완료 확인)
  4. AQA 레지스터에 Admin 큐 깊이 설정
  5. ASQ 레지스터에 Admin SQ의 DMA 물리 주소 설정
  6. ACQ 레지스터에 Admin CQ의 DMA 물리 주소 설정
  7. CC.EN = 1 (컨트롤러 활성화)
  8. CSTS.RDY = 1 대기 (활성화 완료 확인)
```

코드 흐름:

```c
static int nvme_pci_configure_admin_queue(struct nvme_dev *dev)
{
    /* BAR0 매핑 확장 (Doorbell 영역 포함) */
    result = nvme_remap_bar(dev, db_bar_size(dev, 0));

    /* 컨트롤러 비활성화: CC.EN = 0, CSTS.RDY = 0 대기 */
    result = nvme_disable_ctrl(&dev->ctrl, false);

    /* Admin 큐 DMA 메모리 할당 */
    /* CQ: 32 * 16바이트 = 512바이트 (dma_alloc_coherent) */
    /* SQ: 32 * 64바이트 = 2048바이트 (dma_alloc_coherent) */
    result = nvme_alloc_queue(dev, 0, NVME_AQ_DEPTH);  /* depth=32 */

    nvmeq = &dev->queues[0];

    /* AQA: Admin Queue Attributes (SQ 깊이 | CQ 깊이) */
    /* 0-based이므로 31 | (31 << 16) */
    aqa = nvmeq->q_depth - 1;
    aqa |= aqa << 16;
    writel(aqa, dev->bar + NVME_REG_AQA);

    /* ASQ: Admin SQ 물리 주소를 BAR0 + 0x28에 기록 */
    lo_hi_writeq(nvmeq->sq_dma_addr, dev->bar + NVME_REG_ASQ);

    /* ACQ: Admin CQ 물리 주소를 BAR0 + 0x30에 기록 */
    lo_hi_writeq(nvmeq->cq_dma_addr, dev->bar + NVME_REG_ACQ);

    /* 컨트롤러 활성화: CC.EN = 1, CSTS.RDY = 1 대기 */
    result = nvme_enable_ctrl(&dev->ctrl);

    /* Admin 큐 초기화 (head/tail 리셋, phase=1, Doorbell 주소 설정) */
    nvmeq->cq_vector = 0;
    nvme_init_queue(nvmeq, 0);

    /* MSI-X 벡터 0번에 인터럽트 핸들러 등록 */
    result = queue_request_irq(nvmeq);

    set_bit(NVMEQ_ENABLED, &nvmeq->flags);
    return result;
}
```


### 2.3 I/O Queue 생성: nvme_create_io_queues()

Admin Queue가 준비되면 I/O Queue를 생성한다. I/O Queue는 Admin 커맨드를 통해 생성한다.

**큐 개수 결정 로직**:

```c
/* nvme_max_io_queues()에서 최대 I/O 큐 수를 결정 */
static unsigned int nvme_max_io_queues(struct nvme_dev *dev)
{
    if (dev->ctrl.quirks & NVME_QUIRK_SHARED_TAGS)
        return 1;  /* Apple quirk: 태그 공유 문제로 1개만 */

    /* 가능한 CPU 수 + 쓰기 전용 큐 + 폴링 큐 */
    return blk_mq_num_possible_queues(0) + dev->nr_write_queues
         + dev->nr_poll_queues;
}
```

기본적으로 CPU 코어 수만큼 I/O 큐를 생성한다. 예를 들어 8코어 시스템에서는 8개의 I/O 큐를 요청한다. 하지만 실제 큐 수는 다음 제약에 의해 줄어들 수 있다:

1. **컨트롤러 제한**: Set Number of Queues 커맨드로 디바이스와 협상한 결과
2. **MSI-X 벡터 수**: 할당 가능한 인터럽트 벡터 수
3. **BAR 크기**: Doorbell 영역이 충분해야 함

```c
/* nvme_setup_io_queues() 핵심 흐름 */
static int nvme_setup_io_queues(struct nvme_dev *dev)
{
    nr_io_queues = dev->nr_allocated_queues - 1;

    /* Set Number of Queues: 디바이스와 큐 수 협상 */
    /* 요청: nr_io_queues개, 결과: 디바이스가 지원하는 최대 수 */
    result = nvme_set_queue_count(&dev->ctrl, &nr_io_queues);

    /* BAR 매핑 확장 (Doorbell 영역 포함) */
    size = db_bar_size(dev, nr_io_queues);
    result = nvme_remap_bar(dev, size);

    /* MSI-X 벡터 할당 (CPU affinity 고려) */
    result = nvme_setup_irqs(dev, nr_io_queues);
    dev->num_vecs = result;

    /* Admin 큐 인터럽트 재등록 */
    result = queue_request_irq(adminq);

    /* 각 I/O 큐 생성 */
    result = nvme_create_io_queues(dev);
}

static int nvme_create_io_queues(struct nvme_dev *dev)
{
    /* 1단계: 각 큐의 DMA 메모리 할당 */
    for (i = dev->ctrl.queue_count; i <= dev->max_qid; i++) {
        nvme_alloc_queue(dev, i, dev->q_depth);
    }

    /* 2단계: Admin 커맨드로 디바이스에 큐 생성 */
    for (i = dev->online_queues; i <= max; i++) {
        bool polled = i > rw_queues;
        nvme_create_queue(&dev->queues[i], i, polled);
    }
}
```

각 큐의 생성 순서 (`nvme_create_queue()`):

```
1. adapter_alloc_cq()    → Create I/O Completion Queue (opcode 0x05)
2. adapter_alloc_sq()    → Create I/O Submission Queue (opcode 0x01)
3. nvme_init_queue()     → 큐 상태 초기화 (head/tail/phase)
4. queue_request_irq()   → MSI-X 인터럽트 핸들러 등록 (폴링 큐 제외)
5. set_bit(NVMEQ_ENABLED) → 큐 활성화
```


### 2.4 blk_mq_tag_set 등록

blk-mq 태그셋은 블록 레이어와 NVMe 드라이버를 연결하는 핵심 구조체이다.

```c
/* I/O 큐용 태그셋 생성 (nvme_alloc_io_tag_set 내부) */
struct blk_mq_tag_set tagset = {
    .ops        = &nvme_mq_ops,           /* ★ 핵심 콜백 */
    .nr_hw_queues = online_queues - 1,    /* I/O 큐 수 */
    .queue_depth  = dev->q_depth,         /* 큐 깊이 (기본 1024) */
    .cmd_size     = sizeof(struct nvme_iod), /* 요청당 private data 크기 */
    .nr_maps      = nvme_pci_nr_maps(dev),  /* 큐 맵 수 (default/read/poll) */
    .numa_node    = dev->ctrl.numa_node,
};

/* blk_mq_ops가 블록 레이어의 I/O 요청을 NVMe 드라이버로 연결 */
static const struct blk_mq_ops nvme_mq_ops = {
    .queue_rq     = nvme_queue_rq,         /* ★ I/O 제출 (가장 중요) */
    .queue_rqs    = nvme_queue_rqs,         /* 배치 I/O 제출 */
    .complete     = nvme_pci_complete_rq,   /* 완료 처리 */
    .commit_rqs   = nvme_commit_rqs,        /* 지연된 Doorbell flush */
    .init_hctx    = nvme_init_hctx,         /* hctx ↔ nvme_queue 연결 */
    .init_request = nvme_pci_init_request,  /* 요청 초기화 */
    .map_queues   = nvme_pci_map_queues,    /* CPU → 큐 매핑 */
    .timeout      = nvme_timeout,           /* 타임아웃 핸들러 */
    .poll         = nvme_poll,              /* 폴링 콜백 */
};
```


## 3. NVMe Queue Pair 구조 상세

### 3.1 struct nvme_queue

`struct nvme_queue`는 NVMe의 SQ+CQ 쌍을 하나로 관리하는 구조체이다. 각 필드의 역할을 상세히 분석한다.

```c
struct nvme_queue {
    struct nvme_dev *dev;
    /* 이 큐가 속한 NVMe 디바이스에 대한 포인터.
     * DMA 매핑, Doorbell 주소 계산 등에서 디바이스 정보가 필요할 때 사용한다. */

    struct nvme_descriptor_pools descriptor_pools;
    /* PRP 리스트/SGL 디스크립터 할당을 위한 DMA 풀.
     * NUMA 노드에 최적화되어 있어 로컬 메모리에서 할당된다.
     * large: 4KB 페이지 크기 (큰 PRP 리스트)
     * small: 256바이트 크기 (작은 I/O 최적화) */

    spinlock_t sq_lock;
    /* SQ 접근을 보호하는 스핀락.
     * 여러 CPU가 같은 큐에 동시에 커맨드를 제출할 수 있으므로 필요하다.
     * nvme_queue_rq()에서 SQ에 커맨드를 복사하고 Doorbell을 울릴 때 획득한다. */

    void *sq_cmds;
    /* SQ 커맨드 버퍼 포인터.
     * 64바이트 NVMe 커맨드 * q_depth개가 연속 배치된 DMA coherent 메모리이다.
     * 또는 CMB(Controller Memory Buffer)에 위치할 수 있다.
     * 호스트가 여기에 커맨드를 쓰면 디바이스가 DMA로 fetch한다. */

    spinlock_t cq_poll_lock ____cacheline_aligned_in_smp;
    /* 폴링 모드에서 CQ 접근을 보호하는 스핀락.
     * ____cacheline_aligned_in_smp: 별도 캐시라인에 배치하여
     * sq_lock과의 false sharing을 방지한다.
     * 인터럽트 모드에서는 사용되지 않는다. */

    struct nvme_completion *cqes;
    /* CQ 엔트리 배열 (DMA coherent 메모리).
     * 16바이트 nvme_completion * q_depth개.
     * 디바이스가 커맨드 완료 시 이 메모리에 DMA로 결과를 쓴다.
     * 호스트는 phase bit를 확인하여 새 완료를 감지한다. */

    dma_addr_t sq_dma_addr;
    /* SQ 버퍼의 DMA(버스) 주소.
     * 디바이스는 이 주소로 SQ에서 커맨드를 fetch한다.
     * Create I/O SQ 커맨드의 PRP1 필드에 이 값이 들어간다. */

    dma_addr_t cq_dma_addr;
    /* CQ 버퍼의 DMA(버스) 주소.
     * 디바이스는 이 주소에 완료 엔트리를 DMA로 쓴다.
     * Create I/O CQ 커맨드의 PRP1 필드에 이 값이 들어간다. */

    u32 __iomem *q_db;
    /* 이 큐의 SQ Tail Doorbell MMIO 주소.
     * BAR0 + 0x1000 + (2 * qid * stride) 위치.
     * q_db + dev->db_stride가 CQ Head Doorbell 주소이다.
     * writel(value, q_db)로 디바이스에 "새 커맨드가 있다"고 알린다. */

    u32 q_depth;
    /* 이 큐의 깊이 (엔트리 수).
     * Admin 큐: 32 (NVME_AQ_DEPTH)
     * I/O 큐: 모듈 파라미터 io_queue_depth (기본 1024) */

    u16 cq_vector;
    /* 이 큐에 할당된 MSI-X 인터럽트 벡터 번호.
     * pci_request_irq()에서 이 벡터에 nvme_irq() 핸들러를 등록한다.
     * 폴링 큐는 이 값이 의미 없다. */

    u16 sq_tail;
    /* SQ Tail 포인터: 다음 커맨드를 쓸 위치.
     * nvme_sq_copy_cmd()에서 커맨드를 sq_cmds[sq_tail]에 복사하고 증가시킨다.
     * q_depth에 도달하면 0으로 wrap한다 (링 버퍼). */

    u16 last_sq_tail;
    /* 마지막으로 Doorbell에 기록한 SQ Tail 값.
     * 배치 최적화에 사용된다: sq_tail과 같으면 Doorbell을 쓰지 않아도 된다.
     * nvme_write_sq_db()에서 비교하여 불필요한 MMIO를 방지한다. */

    u16 cq_head;
    /* CQ Head 포인터: 다음 읽을 완료 엔트리 위치.
     * nvme_poll_cq()에서 CQE를 처리할 때마다 증가시킨다.
     * 처리 완료 후 CQ Head Doorbell에 기록하여 디바이스에 알린다. */

    u16 qid;
    /* 큐 ID. 0은 Admin Queue, 1~N은 I/O Queue.
     * NVMe 스펙의 SQID/CQID에 해당한다. */

    u8 cq_phase;
    /* 현재 기대하는 CQ Phase 비트.
     * CQ가 초기화될 때 1로 시작한다.
     * CQ가 한 바퀴 돌 때마다 토글된다 (1→0→1→...).
     * CQE의 status 필드 최하위 비트와 비교하여 새 완료인지 판단한다. */

    u8 sqes;
    /* SQ Entry Size의 log2 값.
     * 보통 6 (= 2^6 = 64바이트). Apple 디바이스는 7 (128바이트).
     * SQ_SIZE 매크로에서 SQ 버퍼의 바이트 크기를 계산할 때 사용된다. */

    unsigned long flags;
    /* 큐 상태 비트 플래그:
     * NVMEQ_ENABLED(0): 큐가 활성화되어 I/O를 받을 수 있는 상태
     * NVMEQ_SQ_CMB(1):  SQ가 CMB에 있음
     * NVMEQ_DELETE_ERROR(2): 큐 삭제 중 에러 발생
     * NVMEQ_POLLED(3):  폴링 모드 (인터럽트 없음) */

    __le32 *dbbuf_sq_db;    /* Shadow SQ Tail Doorbell (호스트 메모리) */
    __le32 *dbbuf_cq_db;    /* Shadow CQ Head Doorbell (호스트 메모리) */
    __le32 *dbbuf_sq_ei;    /* SQ Event Index (컨트롤러가 설정) */
    __le32 *dbbuf_cq_ei;    /* CQ Event Index (컨트롤러가 설정) */
    /* Shadow Doorbell은 NVMe 1.3에서 도입된 최적화 기능이다.
     * 호스트가 일반 메모리에 Doorbell 값을 쓰고, Event Index와 비교하여
     * 실제 MMIO 쓰기가 필요한지 판단한다. 불필요한 MMIO를 줄여 성능을 향상시킨다. */

    struct completion delete_done;
    /* 큐 삭제 완료를 동기적으로 기다리기 위한 completion 구조체.
     * Delete SQ/CQ 커맨드는 비동기로 실행되므로 이것으로 대기한다. */
};
```


### 3.2 SQ/CQ 메모리 레이아웃

SQ와 CQ는 물리적으로 연속된 DMA coherent 메모리에 배치된 링 버퍼이다.

**SQ (Submission Queue) 메모리 레이아웃**:

```
sq_cmds 시작 주소 (sq_dma_addr)
│
▼
┌──────────────────┬──────────────────┬──── ─ ─ ──┬──────────────────┐
│  SQE[0]  64바이트 │  SQE[1]  64바이트 │   ...    │ SQE[q_depth-1]  │
│ opcode, nsid,    │                  │          │                  │
│ slba, nlb,       │                  │          │                  │
│ PRP1, PRP2       │                  │          │                  │
└──────────────────┴──────────────────┴──── ─ ─ ──┴──────────────────┘
│←────── SQ 전체 크기: q_depth << sqes (예: 1024 * 64 = 64KB) ────────→│
```

**CQ (Completion Queue) 메모리 레이아웃**:

```
cqes 시작 주소 (cq_dma_addr)
│
▼
┌──────────────────┬──────────────────┬──── ─ ─ ──┬──────────────────┐
│ CQE[0]  16바이트  │ CQE[1]  16바이트  │   ...    │ CQE[q_depth-1]  │
│ result,          │                  │          │                  │
│ sq_head, sq_id,  │                  │          │                  │
│ command_id,      │                  │          │                  │
│ status + phase   │                  │          │                  │
└──────────────────┴──────────────────┴──── ─ ─ ──┴──────────────────┘
│←────── CQ 전체 크기: q_depth * 16 (예: 1024 * 16 = 16KB) ──────────→│
```

**SQ/CQ 링 버퍼 동작 다이어그램**:

커맨드 제출 전:
```
         SQ (q_depth = 8, 예시)
         ┌───┬───┬───┬───┬───┬───┬───┬───┐
  인덱스: │ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │
         └───┴───┴───┴───┴───┴───┴───┴───┘
               ▲               ▲
               │               │
          SQ Head            SQ Tail
          (디바이스 관리)      (호스트 관리)
                              sq_tail = 4

         CQ (q_depth = 8, 예시)
         ┌───┬───┬───┬───┬───┬───┬───┬───┐
  인덱스: │ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │
         └───┴───┴───┴───┴───┴───┴───┴───┘
               ▲
               │
            CQ Head
            (호스트 관리)
            cq_head = 1, cq_phase = 1
```

커맨드 제출 후 (2개 제출):
```
         SQ
         ┌───┬───┬───┬───┬───┬───┬───┬───┐
  인덱스: │ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │
         └───┴───┴───┴───┴───┴─▲─┴───┴───┘
               ▲               │
               │         새 커맨드 2개
          SQ Head        SQ Tail = 6
          (변경 없음)
                         Doorbell에 6을 기록!
                         writel(6, q_db)
```

완료 처리 후:
```
         CQ
         ┌───┬───┬───┬───┬───┬───┬───┬───┐
  인덱스: │ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │
         └───┴─▲─┴───┴─▲─┴───┴───┴───┴───┘
               │       │
            이전위치  새 CQ Head = 3
            (처리됨)  CQE[1],[2]를 처리함

     디바이스가 CQE[1], CQE[2]에 완료 결과를 DMA로 기록
     → 인터럽트 발생
     → nvme_irq() → nvme_poll_cq()
     → CQE[1] 처리: phase 확인 → command_id로 request 찾기 → 완료
     → CQE[2] 처리: 동일
     → CQ Head Doorbell에 3을 기록
       writel(3, q_db + db_stride)
```

Phase bit를 사용한 CQ 유효성 판별:
```
초기 상태 (cq_phase = 1):
  CQ 메모리를 0으로 클리어 → 모든 CQE의 phase = 0
  cq_phase = 1이므로 모든 CQE는 "처리 안 된" 상태

디바이스가 CQE를 쓸 때:
  status 필드의 최하위 비트에 현재 phase(1)를 설정
  → 호스트가 읽으면 phase == cq_phase → "새 완료!"

CQ가 한 바퀴 돌면:
  cq_head가 0으로 되돌아가고 cq_phase를 토글 (1→0)
  이제 phase가 0인 CQE가 "새 완료"가 됨
  → 이전 라운드에서 남은 phase=1 CQE를 새 것으로 오인하지 않음
```


## 4. I/O 제출 경로 (nvme_queue_rq) 완전 분석

### 4.1 nvme_queue_rq() 라인바이라인

`nvme_queue_rq()`는 블록 레이어와 NVMe 하드웨어 사이의 브릿지이다. 애플리케이션의 read()/write() 시스템 콜이 최종적으로 이 함수에 도달하여 NVMe SQ에 커맨드로 기록된다.

**전체 호출 체인**:
```
애플리케이션: write(fd, buf, 4096)
  → VFS: vfs_write()
    → 파일시스템: ext4_file_write_iter() → ext4_writepages()
      → Block Layer: submit_bio() → blk_mq_submit_bio()
        → blk_mq_dispatch_rq_list() 또는 blk_mq_try_issue_directly()
          → ★ nvme_queue_rq() ★
            → nvme_prep_rq()
              → nvme_setup_cmd()    : request → NVMe 커맨드 변환
              → nvme_map_data()     : DMA 매핑 (PRP/SGL 설정)
              → nvme_start_request(): 타이머 시작
            → nvme_sq_copy_cmd()    : SQ에 커맨드 복사
            → nvme_write_sq_db()    : Doorbell 쓰기
```

```c
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
                                  const struct blk_mq_queue_data *bd)
{
    /* hctx->driver_data는 nvme_init_hctx()에서 설정한 nvme_queue 포인터 */
    struct nvme_queue *nvmeq = hctx->driver_data;
    struct nvme_dev *dev = nvmeq->dev;
    struct request *req = bd->rq;       /* 블록 레이어 요청 */
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);  /* 요청의 NVMe private data */
    blk_status_t ret;

    /* ── 검사 1: 큐가 활성화 상태인지 ── */
    /* 셧다운/리셋 중에는 NVMEQ_ENABLED가 해제된다 */
    if (unlikely(!test_bit(NVMEQ_ENABLED, &nvmeq->flags)))
        return BLK_STS_IOERR;

    /* ── 검사 2: 컨트롤러가 준비되었는지 ── */
    /* LIVE 또는 CONNECTING 상태에서만 I/O를 받는다 */
    if (unlikely(!nvme_check_ready(&dev->ctrl, req, true)))
        return nvme_fail_nonready_command(&dev->ctrl, req);

    /* ── 요청 준비 (핵심 단계) ── */
    /* 1. nvme_setup_cmd(): block op → NVMe opcode 변환 */
    /* 2. nvme_map_data():  DMA 매핑 (PRP/SGL) */
    /* 3. nvme_start_request(): 타임아웃 타이머 시작 */
    ret = nvme_prep_rq(req);
    if (unlikely(ret))
        return ret;

    /* ── SQ에 커맨드 쓰기 + Doorbell (critical section) ── */
    spin_lock(&nvmeq->sq_lock);

    /* sq_cmds의 sq_tail 위치에 64바이트 커맨드를 memcpy */
    nvme_sq_copy_cmd(nvmeq, &iod->cmd);

    /* Doorbell 쓰기 (조건부)
     * bd->last == true: 이 배치의 마지막 요청 → 즉시 Doorbell
     * bd->last == false: 아직 더 있음 → Doorbell 지연 (배치 최적화) */
    nvme_write_sq_db(nvmeq, bd->last);

    spin_unlock(&nvmeq->sq_lock);
    return BLK_STS_OK;
}
```


### 4.1.1 nvme_prep_rq() 상세

```c
static blk_status_t nvme_prep_rq(struct request *req)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
    blk_status_t ret;

    /* I/O 디스크립터 초기화 */
    iod->flags = 0;                /* DMA 상태 플래그 클리어 */
    iod->nr_descriptors = 0;       /* PRP/SGL 디스크립터 수 클리어 */
    iod->total_len = 0;            /* 매핑된 총 바이트 수 클리어 */
    iod->meta_total_len = 0;       /* 메타데이터 매핑 길이 클리어 */
    iod->nr_dma_vecs = 0;          /* DMA 벡터 수 클리어 */

    /* ── 단계 1: NVMe 커맨드 빌드 ── */
    /* nvme_setup_cmd()는 core.c에 있는 공통 함수이다.
     * block request의 종류(READ/WRITE/DISCARD/FLUSH 등)를 분석하여
     * NVMe 커맨드의 opcode, nsid, slba, nlb 등을 설정한다.
     *
     * 예: READ 요청
     *   cmd->rw.opcode = nvme_cmd_read (0x02)
     *   cmd->rw.nsid   = namespace ID
     *   cmd->rw.slba   = starting LBA
     *   cmd->rw.length = number of LBAs - 1 (0-based)
     */
    ret = nvme_setup_cmd(req->q->queuedata, req);
    if (ret)
        return ret;

    /* ── 단계 2: 데이터 DMA 매핑 ── */
    /* 물리 세그먼트가 있으면 (데이터 전송이 필요한 요청) */
    if (blk_rq_nr_phys_segments(req)) {
        ret = nvme_map_data(req);   /* PRP 또는 SGL 설정 (4.2절 참조) */
        if (ret)
            goto out_free_cmd;
    }

    /* ── 단계 3: 메타데이터 DMA 매핑 ── */
    /* T10-PI(Protection Information) 등 무결성 데이터가 있으면 */
    if (blk_integrity_rq(req)) {
        ret = nvme_map_metadata(req);
        if (ret)
            goto out_unmap_data;
    }

    /* ── 단계 4: 요청 시작 알림 ── */
    /* blk_mq_start_request()를 호출하여:
     * - 요청 상태를 MQ_RQ_IN_FLIGHT로 전환
     * - 타임아웃 타이머 시작 (기본 30초)
     * - command_id에 generation counter 인코딩 */
    nvme_start_request(req);
    return BLK_STS_OK;

out_unmap_data:
    if (blk_rq_nr_phys_segments(req))
        nvme_unmap_data(req);
out_free_cmd:
    nvme_cleanup_cmd(req);
    return ret;
}
```


### 4.1.2 nvme_sq_copy_cmd() 상세

```c
static inline void nvme_sq_copy_cmd(struct nvme_queue *nvmeq,
                                    struct nvme_command *cmd)
{
    /* SQ 버퍼의 현재 Tail 위치에 64바이트 커맨드를 복사 */
    /* sq_cmds + (sq_tail << sqes) = sq_cmds + sq_tail * 64 */
    memcpy(nvmeq->sq_cmds + (nvmeq->sq_tail << nvmeq->sqes),
           absolute_pointer(cmd), sizeof(*cmd));

    /* SQ Tail을 다음 위치로 전진 (링 버퍼 wrap) */
    if (++nvmeq->sq_tail == nvmeq->q_depth)
        nvmeq->sq_tail = 0;
}
```


### 4.2 PRP (Physical Region Page) vs SGL (Scatter Gather List)

NVMe는 데이터 전송 주소를 지정하는 두 가지 방식을 지원한다.

**PRP (Physical Region Page)**:
- NVMe 1.0부터 지원하는 기본 방식
- 4KB 페이지 정렬 제약이 있다 (첫 PRP만 비정렬 허용)
- 커맨드의 PRP1/PRP2 필드를 사용
- 2페이지 이내: PRP1, PRP2에 직접 주소
- 2페이지 초과: PRP2가 PRP 리스트 페이지를 가리킴

```
데이터가 작은 경우 (≤ 2페이지):
  ┌─────────────┐
  │ NVMe 커맨드  │
  │ PRP1 ───────┼──→ 첫 번째 데이터 페이지
  │ PRP2 ───────┼──→ 두 번째 데이터 페이지 (또는 0)
  └─────────────┘

데이터가 큰 경우 (> 2페이지):
  ┌─────────────┐
  │ NVMe 커맨드  │
  │ PRP1 ───────┼──→ 첫 번째 데이터 페이지
  │ PRP2 ───────┼──→ PRP 리스트 페이지
  └─────────────┘     │
                      ▼
  ┌──────────┬──────────┬──── ─ ─ ──┬──────────┬──────────────┐
  │ PRP[0]   │ PRP[1]   │  ...      │ PRP[510] │ 다음 리스트링크│
  │ →데이터  │ →데이터  │           │ →데이터  │ →다음 PRP 리스트│
  └──────────┴──────────┴──── ─ ─ ──┴──────────┴──────────────┘
    PRP 리스트 1개에 최대 511개 PRP 엔트리 (마지막 1개는 링크)
    4KB 페이지 / 8바이트 PRP 엔트리 - 1 = 511개
```

**SGL (Scatter Gather List)**:
- NVMe 1.1+에서 선택적 지원
- 각 엔트리에 주소+길이가 있어 유연하다
- 비정렬, 가변 길이 세그먼트를 지원
- 디바이스가 전송 길이를 검증할 수 있다

```
단일 세그먼트:
  ┌─────────────┐
  │ NVMe 커맨드  │
  │ SGL ────────┼──→ Data Descriptor: {addr, length, type=DATA}
  └─────────────┘     → 데이터 버퍼

다중 세그먼트:
  ┌─────────────┐
  │ NVMe 커맨드  │
  │ SGL ────────┼──→ Segment Descriptor: {addr→SGL리스트, entries, type=LAST_SEG}
  └─────────────┘     │
                      ▼
  ┌───────────────┬───────────────┬──── ─ ─ ──┬───────────────┐
  │ Data Desc[0]  │ Data Desc[1]  │  ...      │ Data Desc[N]  │
  │{addr,len,DATA}│{addr,len,DATA}│           │{addr,len,DATA}│
  └───────────────┴───────────────┴──── ─ ─ ──┴───────────────┘
```


### 4.2.1 nvme_map_data()에서의 PRP/SGL 선택 로직

```c
static blk_status_t nvme_map_data(struct request *req)
{
    enum nvme_use_sgl use_sgl = nvme_pci_use_sgls(dev, req);

    /* ── 최적화: 단일 세그먼트이면 빠른 경로 ── */
    /* 대부분의 4KB I/O가 이 경로를 탄다 */
    if (blk_rq_nr_phys_segments(req) == 1) {
        ret = nvme_pci_setup_data_simple(req, use_sgl);
        if (ret != BLK_STS_AGAIN)
            return ret;
    }

    /* DMA 이터레이터 시작 */
    blk_rq_dma_map_iter_start(req, dev->dev, &iod->dma_state, &iter);

    /* ── SGL vs PRP 최종 결정 ── */
    if (use_sgl == SGL_FORCED ||
        (use_sgl == SGL_SUPPORTED &&
         sgl_threshold && nvme_pci_avg_seg_size(req) >= sgl_threshold))
        return nvme_pci_setup_data_sgl(req, &iter);  /* SGL 경로 */
    return nvme_pci_setup_data_prp(req, &iter);       /* PRP 경로 */
}
```

SGL 선택 기준 (`nvme_pci_use_sgls()`):
- Admin 큐(qid=0): 항상 PRP (SGL 미지원)
- 컨트롤러가 SGL 미지원: PRP만 사용
- SGL 강제 조건 (SGL_FORCED):
  - 페이지 갭이 있는 경우 (PRP로 표현 불가)
  - 사용자 커맨드 (SGL이 길이 검증을 제공)
  - 다중 무결성 세그먼트
- SGL 선택적 사용 (SGL_SUPPORTED):
  - 평균 세그먼트 크기가 `sgl_threshold`(기본 32KB) 이상이면 SGL
  - 작은 세그먼트가 많으면 PRP (오버헤드가 적음)


### 4.3 Doorbell 쓰기 상세

#### nvme_write_sq_db()

Doorbell은 호스트가 디바이스에게 "새 커맨드가 SQ에 있다"고 알리는 메커니즘이다. MMIO writel()로 BAR0의 Doorbell 레지스터에 SQ Tail 값을 쓴다.

```c
static inline void nvme_write_sq_db(struct nvme_queue *nvmeq, bool write_sq)
{
    /* ── 배치 최적화: Doorbell 쓰기 지연 ── */
    if (!write_sq) {
        /* bd->last가 false이면 아직 더 커맨드가 올 수 있으므로 대기 */
        u16 next_tail = nvmeq->sq_tail + 1;
        if (next_tail == nvmeq->q_depth)
            next_tail = 0;
        /* 단, wrap이 발생하면 반드시 써야 한다 (SQ가 가득 찰 수 있음) */
        if (next_tail != nvmeq->last_sq_tail)
            return;  /* Doorbell 쓰기 건너뛰기! */
    }

    /* ── Shadow Doorbell 최적화 ── */
    /* Shadow DB가 있으면 먼저 호스트 메모리에 쓰고,
     * Event Index를 확인하여 실제 MMIO가 필요한지 판단한다 */
    if (nvme_dbbuf_update_and_check_event(nvmeq->sq_tail,
            nvmeq->dbbuf_sq_db, nvmeq->dbbuf_sq_ei))
        writel(nvmeq->sq_tail, nvmeq->q_db);  /* 실제 MMIO 쓰기 */

    nvmeq->last_sq_tail = nvmeq->sq_tail;
}
```


#### Shadow Doorbell: nvme_dbbuf_update_and_check_event()

Shadow Doorbell은 NVMe 1.3에서 도입된 중요한 최적화이다. MMIO writel()은 PCIe 트랜잭션을 발생시키므로 비용이 크다(수백 ns). Shadow Doorbell은 호스트 메모리에 Doorbell 값을 쓰고, 컨트롤러가 설정한 Event Index와 비교하여 실제 MMIO가 필요한 경우에만 writel()을 수행한다.

```c
static bool nvme_dbbuf_update_and_check_event(u16 value, __le32 *dbbuf_db,
                                              volatile __le32 *dbbuf_ei)
{
    if (dbbuf_db) {
        u16 old_value, event_idx;

        /* 1. 큐 메모리 쓰기가 완료된 후 shadow DB를 업데이트해야 한다 */
        wmb();

        /* 2. Shadow Doorbell에 새 값을 쓴다 (일반 메모리 쓰기 = 매우 빠름) */
        old_value = le32_to_cpu(*dbbuf_db);
        *dbbuf_db = cpu_to_le32(value);

        /* 3. Shadow DB 업데이트 후 Event Index를 읽는 순서를 보장 */
        mb();

        /* 4. 컨트롤러가 설정한 Event Index를 읽는다 */
        event_idx = le32_to_cpu(*dbbuf_ei);

        /* 5. Event Index 기준으로 MMIO가 필요한지 판단 */
        /* "new_idx가 event_idx를 지나쳤는가?" */
        /* 컨트롤러가 아직 이전 값을 처리 중이면 MMIO 불필요 */
        if (!nvme_dbbuf_need_event(event_idx, value, old_value))
            return false;  /* MMIO 불필요! → writel() 건너뜀 */
    }

    return true;  /* MMIO 필요 → writel() 수행 */
}
```

```
Shadow Doorbell 동작 흐름:

일반 Doorbell (Shadow DB 없음):
  커맨드 제출 → writel(tail, q_db) → PCIe 트랜잭션
  커맨드 제출 → writel(tail, q_db) → PCIe 트랜잭션
  커맨드 제출 → writel(tail, q_db) → PCIe 트랜잭션
  매번 수백 ns의 MMIO 비용!

Shadow Doorbell:
  커맨드 제출 → *dbbuf_sq_db = tail (메모리 쓰기, ~수 ns)
               → event_idx 확인 → MMIO 불필요! (컨트롤러가 아직 이전 꺼 처리 중)
  커맨드 제출 → *dbbuf_sq_db = tail
               → event_idx 확인 → MMIO 불필요!
  커맨드 제출 → *dbbuf_sq_db = tail
               → event_idx 확인 → MMIO 필요! → writel(tail, q_db)
  MMIO 횟수가 크게 줄어듦!
```


## 5. I/O 완료 경로 (Completion) 완전 분석

### 5.1 인터럽트 처리

디바이스가 커맨드를 처리하면 CQ에 완료 엔트리(CQE)를 DMA로 기록하고 인터럽트를 발생시킨다.

**완료 경로 전체 흐름**:
```
디바이스: 커맨드 처리 완료
  → CQ 메모리에 CQE를 DMA로 기록 (16바이트)
  → MSI-X 인터럽트 발생
  → CPU가 인터럽트 수신
    → 커널 IRQ 프레임워크
      → nvme_irq() (hardirq 컨텍스트)
        → nvme_poll_cq() : CQ 순회
          → nvme_cqe_pending() : phase bit 확인
          → dma_rmb() : 메모리 배리어
          → nvme_handle_cqe() : CQE 처리
            → nvme_find_rq() : command_id → request 매칭
            → nvme_try_complete_req() : 상태 설정
            → blk_mq_add_to_batch() 또는 nvme_pci_complete_rq()
          → nvme_update_cq_head() : CQ Head 전진 + phase 토글
        → nvme_ring_cq_doorbell() : CQ Head Doorbell 기록
      → nvme_pci_complete_batch() : 배치 완료 처리
```


### 5.1.1 nvme_irq() - 인터럽트 핸들러

```c
static irqreturn_t nvme_irq(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;
    DEFINE_IO_COMP_BATCH(iob);  /* 배치 완료를 위한 스택 변수 */

    /* CQ를 순회하며 대기 중인 모든 CQE를 처리 */
    if (nvme_poll_cq(nvmeq, &iob)) {
        /* 배치로 모은 완료 요청들을 한 번에 처리 */
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;  /* 이 큐에 처리할 CQE 없음 (인터럽트 공유 시) */
}
```

스레드 인터럽트 모드 (`use_threaded_interrupts=1`):
```c
/* hardirq: CQE 존재 여부만 빠르게 확인 */
static irqreturn_t nvme_irq_check(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;
    if (nvme_cqe_pending(nvmeq))
        return IRQ_WAKE_THREAD;  /* 스레드를 깨워서 실제 처리 */
    return IRQ_NONE;
}
/* thread: nvme_irq()가 실행되어 실제 CQ 처리 수행 */
```

폴링 모드 (`poll_queues > 0`):
```c
/* blk-mq의 .poll 콜백. io_uring IOPOLL에서 호출됨 */
static int nvme_poll(struct blk_mq_hw_ctx *hctx, struct io_comp_batch *iob)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    bool found;

    if (!nvme_cqe_pending(nvmeq))
        return 0;  /* 완료 없음 */

    spin_lock(&nvmeq->cq_poll_lock);
    found = nvme_poll_cq(nvmeq, iob);
    spin_unlock(&nvmeq->cq_poll_lock);
    return found;
}
```


### 5.2 nvme_poll_cq() / nvme_handle_cqe() 라인바이라인

#### nvme_cqe_pending() - CQ에 새 완료가 있는지 확인

```c
static inline bool nvme_cqe_pending(struct nvme_queue *nvmeq)
{
    struct nvme_completion *hcqe = &nvmeq->cqes[nvmeq->cq_head];

    /* CQE의 status 필드 최하위 비트(phase)와 현재 cq_phase를 비교 */
    /* 일치하면 새로운 완료 엔트리가 있다는 뜻 */
    /* READ_ONCE: 디바이스가 DMA로 쓴 값을 fresh하게 읽기 위해 */
    return (le16_to_cpu(READ_ONCE(hcqe->status)) & 1) == nvmeq->cq_phase;
}
```


#### nvme_poll_cq() - CQ 순회 루프

```c
static inline bool nvme_poll_cq(struct nvme_queue *nvmeq,
                                struct io_comp_batch *iob)
{
    bool found = false;

    /* CQ에 처리할 CQE가 있는 동안 반복 */
    while (nvme_cqe_pending(nvmeq)) {
        found = true;

        /* ── DMA 읽기 메모리 배리어 ── */
        /* Phase bit 확인과 CQE의 다른 필드 읽기 사이에 필요하다.
         * Phase가 유효하면 CQE 전체가 DMA로 기록 완료되었다는 뜻이지만,
         * CPU의 메모리 재정렬로 다른 필드가 stale할 수 있다.
         * dma_rmb()로 이를 방지한다. */
        dma_rmb();

        /* 현재 CQ Head 위치의 CQE를 처리 */
        nvme_handle_cqe(nvmeq, iob, nvmeq->cq_head);

        /* CQ Head를 다음 엔트리로 전진 */
        nvme_update_cq_head(nvmeq);
    }

    /* 처리한 CQE가 있으면 CQ Head Doorbell을 울려 디바이스에 알림 */
    if (found)
        nvme_ring_cq_doorbell(nvmeq);
    return found;
}
```


#### nvme_handle_cqe() - 개별 CQE 처리

```c
static inline void nvme_handle_cqe(struct nvme_queue *nvmeq,
                                   struct io_comp_batch *iob, u16 idx)
{
    struct nvme_completion *cqe = &nvmeq->cqes[idx];
    __u16 command_id = READ_ONCE(cqe->command_id);
    struct request *req;

    /* ── AEN(Asynchronous Event Notification) 특별 처리 ── */
    /* AEN은 컨트롤러가 비동기 이벤트를 알리는 메커니즘이다.
     * 일반 blk-mq request가 아니라 특별한 command_id를 사용한다. */
    if (unlikely(nvme_is_aen_req(nvmeq->qid, command_id))) {
        nvme_complete_async_event(&nvmeq->dev->ctrl,
                cqe->status, &cqe->result);
        return;
    }

    /* ── command_id로 원래 request를 찾는다 ── */
    /* nvme_find_rq():
     * 1. command_id에서 tag 추출 (하위 12비트)
     * 2. blk_mq_tag_to_rq()로 tag → request 포인터 조회
     * 3. generation counter 검증 (stale completion 감지) */
    req = nvme_find_rq(nvme_queue_tagset(nvmeq), command_id);
    if (unlikely(!req)) {
        dev_warn(..., "invalid id %d completed on queue %d\n",
                 command_id, le16_to_cpu(cqe->sq_id));
        return;
    }

    /* SQ head 트레이스 (디버깅/성능 분석용) */
    trace_nvme_sq(req, cqe->sq_head, nvmeq->sq_tail);

    /* ── 요청 완료 처리 ── */
    /* nvme_try_complete_req(): CQE의 status와 result를 nvme_request에 저장
     * blk_mq_add_to_batch(): 배치 완료 리스트에 추가 (성능 최적화)
     *   → 나중에 nvme_pci_complete_batch()에서 일괄 처리
     * nvme_pci_complete_rq(): 배치에 추가 실패 시 즉시 완료 */
    if (!nvme_try_complete_req(req, cqe->status, cqe->result) &&
        !blk_mq_add_to_batch(req, iob,
                nvme_req(req)->status != NVME_SC_SUCCESS,
                nvme_pci_complete_batch))
        nvme_pci_complete_rq(req);
}
```


#### nvme_update_cq_head() - CQ Head 전진 + Phase 토글

```c
static inline void nvme_update_cq_head(struct nvme_queue *nvmeq)
{
    u32 tmp = nvmeq->cq_head + 1;

    if (tmp == nvmeq->q_depth) {
        /* CQ가 한 바퀴 돌았음 → head를 0으로 리셋 */
        nvmeq->cq_head = 0;
        /* Phase를 토글: 다음 라운드의 유효 CQE를 구분하기 위해 */
        nvmeq->cq_phase ^= 1;
    } else {
        nvmeq->cq_head = tmp;
    }
}
```


#### nvme_ring_cq_doorbell() - CQ Head Doorbell 기록

```c
static inline void nvme_ring_cq_doorbell(struct nvme_queue *nvmeq)
{
    u16 head = nvmeq->cq_head;

    /* Shadow Doorbell 최적화 (SQ Doorbell과 동일한 로직) */
    if (nvme_dbbuf_update_and_check_event(head, nvmeq->dbbuf_cq_db,
                                          nvmeq->dbbuf_cq_ei))
        writel(head, nvmeq->q_db + nvmeq->dev->db_stride);
        /* CQ Head Doorbell 위치 = SQ Tail Doorbell + stride */
}
```


### 5.2.1 완료 처리 후 DMA unmap

```c
static void nvme_pci_complete_rq(struct request *req)
{
    /* DMA 매핑 해제 (데이터 + 메타데이터) */
    nvme_pci_unmap_rq(req);
    /* blk-mq에 요청 완료 통지 → 블록 레이어로 반환 */
    nvme_complete_rq(req);
}

static __always_inline void nvme_pci_unmap_rq(struct request *req)
{
    if (blk_integrity_rq(req))
        nvme_unmap_metadata(req);  /* 메타데이터 DMA 해제 */
    if (blk_rq_nr_phys_segments(req))
        nvme_unmap_data(req);      /* 데이터 DMA 해제 + PRP/SGL 풀 반환 */
}
```


### 5.3 I/O 제출/완료 전체 ASCII 다이어그램

```
┌──────────────────────────────────────────────────────────────────┐
│                        I/O 제출 경로                             │
│                                                                  │
│  애플리케이션: write(fd, buf, 4096)                               │
│       │                                                          │
│       ▼                                                          │
│  Block Layer: blk_mq_submit_bio()                                │
│       │                                                          │
│       ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐     │
│  │ nvme_queue_rq(hctx, bd)                                 │     │
│  │   │                                                     │     │
│  │   ├─ nvme_prep_rq(req)                                  │     │
│  │   │    ├─ nvme_setup_cmd()  : opcode/LBA/길이 설정      │     │
│  │   │    ├─ nvme_map_data()   : PRP/SGL DMA 매핑          │     │
│  │   │    └─ nvme_start_request() : 타이머 시작             │     │
│  │   │                                                     │     │
│  │   ├─ spin_lock(&sq_lock)                                │     │
│  │   ├─ nvme_sq_copy_cmd() : SQ[tail]에 64B 커맨드 복사    │     │
│  │   ├─ nvme_write_sq_db() : Doorbell 쓰기                 │     │
│  │   └─ spin_unlock(&sq_lock)                              │     │
│  └─────────────────────────────────────────────────────────┘     │
│       │                                                          │
│       ▼                                                          │
│  ┌─────────────────────┐     ┌─────────────────────┐            │
│  │   호스트 메모리       │     │   BAR0 레지스터      │            │
│  │ ┌─────────────────┐ │     │ ┌─────────────────┐ │            │
│  │ │   SQ 버퍼        │◄┼─────┤ │ SQ Tail Doorbell│ │            │
│  │ │ [cmd0][cmd1]... │ │     │ │ writel(tail)    │ │            │
│  │ └─────────────────┘ │     │ └─────────────────┘ │            │
│  └──────────┬──────────┘     └─────────────────────┘            │
│             │ 디바이스가 DMA로 커맨드 fetch                       │
│             ▼                                                    │
│  ┌─────────────────────┐                                        │
│  │   NVMe 컨트롤러      │                                        │
│  │   커맨드 실행 중...   │                                        │
│  └──────────┬──────────┘                                        │
└─────────────┼────────────────────────────────────────────────────┘
              │
              │ 완료
              ▼
┌──────────────────────────────────────────────────────────────────┐
│                        I/O 완료 경로                             │
│                                                                  │
│  NVMe 컨트롤러: CQ에 CQE를 DMA로 기록 → MSI-X 인터럽트           │
│       │                                                          │
│       ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐     │
│  │ nvme_irq(irq, nvmeq)                                   │     │
│  │   │                                                     │     │
│  │   └─ nvme_poll_cq(nvmeq, &iob)                         │     │
│  │       │                                                 │     │
│  │       ├─ while (nvme_cqe_pending()) {                   │     │
│  │       │    dma_rmb();               : 메모리 배리어      │     │
│  │       │    nvme_handle_cqe()        : CQE → request     │     │
│  │       │      nvme_find_rq()         : CID → request     │     │
│  │       │      nvme_try_complete_req(): 상태 저장          │     │
│  │       │      blk_mq_add_to_batch() : 배치에 추가        │     │
│  │       │    nvme_update_cq_head()    : head++, phase 토글│     │
│  │       │  }                                              │     │
│  │       └─ nvme_ring_cq_doorbell()    : CQ Head DB 기록   │     │
│  │                                                         │     │
│  │   nvme_pci_complete_batch(&iob)     : 배치 완료         │     │
│  │     nvme_pci_unmap_rq()             : DMA unmap         │     │
│  │     nvme_complete_rq()              : blk-mq에 통지     │     │
│  └─────────────────────────────────────────────────────────┘     │
│       │                                                          │
│       ▼                                                          │
│  Block Layer → 파일시스템 → VFS → 애플리케이션에 결과 반환         │
└──────────────────────────────────────────────────────────────────┘
```


## 6. nvme-cli ioctl 경로 분석

nvme-cli 도구는 ioctl 시스템 콜을 통해 NVMe 커맨드를 직접 전송한다. 블록 I/O 스택을 거치지 않고 "passthrough" 방식으로 동작한다.

### 6.1 ioctl 진입점

```
사용자 공간: nvme admin-passthru /dev/nvme0 --opcode=0x06 ...
  → ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd)
    → 커널: nvme_dev_ioctl()  [/dev/nvme0 캐릭터 디바이스]
      → nvme_ctrl_ioctl()
        → nvme_user_cmd()

사용자 공간: nvme io-passthru /dev/nvme0n1 --opcode=0x02 ...
  → ioctl(fd, NVME_IOCTL_IO_CMD, &cmd)
    → 커널: nvme_ioctl()  [/dev/nvme0n1 블록 디바이스]
      → nvme_ns_ioctl()
        → nvme_user_cmd()
```

ioctl 분기 테이블:
```c
/* 블록 디바이스 ioctl (/dev/nvme0n1) */
static int nvme_ns_ioctl(struct nvme_ns *ns, unsigned int cmd,
        void __user *argp, unsigned int flags, bool open_for_write)
{
    switch (cmd) {
    case NVME_IOCTL_ID:          /* 네임스페이스 ID 반환 */
        return ns->head->ns_id;
    case NVME_IOCTL_IO_CMD:      /* I/O passthrough (32비트 결과) */
        return nvme_user_cmd(ns->ctrl, ns, argp, flags, open_for_write);
    case NVME_IOCTL_SUBMIT_IO:   /* 레거시 I/O 제출 */
        return nvme_submit_io(ns, argp);
    case NVME_IOCTL_IO64_CMD:    /* I/O passthrough (64비트 결과) */
        return nvme_user_cmd64(ns->ctrl, ns, argp, flags, open_for_write);
    }
}
```


### 6.2 nvme_user_cmd() 상세

```c
static int nvme_user_cmd(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
        struct nvme_passthru_cmd __user *ucmd, unsigned int flags,
        bool open_for_write)
{
    struct nvme_passthru_cmd cmd;
    struct nvme_command c;

    /* ── 사용자 공간에서 커맨드 구조체를 복사 ── */
    if (copy_from_user(&cmd, ucmd, sizeof(cmd)))
        return -EFAULT;

    /* ── NVMe 커맨드 구성 ── */
    /* 사용자가 지정한 opcode, nsid, CDW10~15를 그대로 커맨드에 넣는다 */
    memset(&c, 0, sizeof(c));
    c.common.opcode = cmd.opcode;
    c.common.nsid = cpu_to_le32(cmd.nsid);
    c.common.cdw10 = cpu_to_le32(cmd.cdw10);
    c.common.cdw11 = cpu_to_le32(cmd.cdw11);
    /* ... cdw2~cdw15 모두 설정 ... */

    /* ── 권한 확인 ── */
    /* 비특권 사용자는 안전한 커맨드만 허용 (nvme_cmd_allowed) */
    if (!nvme_cmd_allowed(ns, &c, 0, open_for_write))
        return -EACCES;

    /* ── 커맨드 제출 (동기적) ── */
    /* ns가 있으면 ns->queue (I/O 큐), 없으면 ctrl->admin_q (Admin 큐) */
    status = nvme_submit_user_cmd(ns ? ns->queue : ctrl->admin_q, &c,
            cmd.addr, cmd.data_len,
            nvme_to_user_ptr(cmd.metadata), cmd.metadata_len,
            &result, timeout, 0);

    /* 결과를 사용자 공간에 반환 */
    if (status >= 0)
        put_user(result, &ucmd->result);

    return status;
}
```


### 6.3 nvme_submit_user_cmd() - 핵심 실행 함수

```c
static int nvme_submit_user_cmd(struct request_queue *q,
        struct nvme_command *cmd, u64 ubuffer, unsigned bufflen,
        void __user *meta_buffer, unsigned meta_len,
        u64 *result, unsigned timeout, unsigned int flags)
{
    struct request *req;
    struct bio *bio;
    int ret;

    /* ── 1. request 직접 할당 ── */
    /* blk_mq_alloc_request()로 블록 레이어에서 request를 할당한다.
     * 일반 I/O와 달리 bio 제출 → 스케줄러 경로를 거치지 않고
     * 직접 request를 할당하여 제출한다.
     * NVME_REQ_USERCMD 플래그를 설정하여 passthrough임을 표시한다. */
    req = nvme_alloc_user_request(q, cmd, 0, 0);

    req->timeout = timeout;

    /* ── 2. 사용자 버퍼를 bio로 매핑 ── */
    if (ubuffer && bufflen) {
        /* blk_rq_map_user_io():
         * 사용자 공간 버퍼의 물리 페이지를 bio에 매핑한다.
         * get_user_pages()로 사용자 메모리를 pin하고
         * bio_add_page()로 페이지를 bio에 추가한다.
         * 결과적으로 req->bio가 설정된다. */
        ret = nvme_map_user_request(req, ubuffer, bufflen,
                meta_buffer, meta_len, NULL, flags);
    }

    bio = req->bio;

    /* ── 3. 동기적 실행 (핵심!) ── */
    /* nvme_execute_rq() → blk_execute_rq():
     * 요청을 큐에 제출하고 완료될 때까지 현재 스레드를 블로킹한다.
     *
     * 핵심 경로:
     *   blk_execute_rq()
     *     → blk_mq_insert_request()  (I/O 스케줄러 바이패스!)
     *     → blk_mq_run_hw_queue()
     *       → nvme_queue_rq()         (일반 I/O와 같은 경로)
     *         → SQ에 커맨드 복사 → Doorbell
     *     → wait_for_completion()      (현재 스레드 대기)
     *
     * 인터럽트/완료:
     *     nvme_irq() → nvme_handle_cqe() → blk_mq_complete_request()
     *     → 대기 중인 스레드를 깨운다
     */
    effects = nvme_passthru_start(ctrl, ns, cmd->common.opcode);
    ret = nvme_execute_rq(req, false);

    if (result)
        *result = le64_to_cpu(nvme_req(req)->result.u64);

    /* ── 4. 정리 ── */
    if (bio)
        blk_rq_unmap_user(bio);  /* 사용자 메모리 unpin */
    blk_mq_free_request(req);     /* request 반환 */

    if (effects)
        nvme_passthru_end(ctrl, ns, effects, cmd, ret);
    return ret;
}
```


### 6.4 Admin 명령 vs I/O 명령 경로 차이

| 특성 | Admin 명령 | I/O 명령 |
|---|---|---|
| 큐 | `ctrl->admin_q` (QID=0) | `ns->queue` (QID=1~N) |
| 디바이스 | `/dev/nvme0` (chardev) | `/dev/nvme0n1` (blkdev) |
| 태그셋 | `dev->admin_tagset` | `dev->tagset` |
| 큐 깊이 | 32 (NVME_AQ_DEPTH) | 1024 (기본값) |
| 타임아웃 | 60초 (admin_timeout) | 30초 (nvme_io_timeout) |
| 용도 | Identify, Feature, Queue 관리 | Read, Write, Compare |
| ioctl | `NVME_IOCTL_ADMIN_CMD` | `NVME_IOCTL_IO_CMD` |
| 인터럽트 벡터 | MSI-X 0번 | MSI-X 1~N번 |

두 경로 모두 최종적으로 같은 `nvme_queue_rq()` → `nvme_sq_copy_cmd()` → Doorbell 경로를 탄다. 차이는 어떤 큐(Admin vs I/O)에 커맨드가 들어가는지이다.


### 6.5 io_uring passthrough 경로

io_uring은 nvme-cli보다 효율적인 비동기 passthrough를 지원한다.

```c
/* io_uring의 NVME_URING_CMD_IO 처리 */
static int nvme_uring_cmd_io(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
        struct io_uring_cmd *ioucmd, unsigned int issue_flags, bool vec)
{
    /* NVMe 커맨드 구성 (ioctl과 동일) */
    c.common.opcode = READ_ONCE(cmd->opcode);
    /* ... */

    /* request 할당 */
    req = nvme_alloc_user_request(q, &c, rq_flags, blk_flags);

    /* 사용자 버퍼 매핑 */
    nvme_map_user_request(req, d.addr, d.data_len, ...);

    /* ── 비동기 실행 (핵심 차이!) ── */
    /* blk_execute_rq()가 아닌 blk_execute_rq_nowait() 사용 */
    /* 완료 시 end_io 콜백이 호출됨 → io_uring CQE로 결과 전달 */
    req->end_io = nvme_uring_cmd_end_io;
    blk_execute_rq_nowait(req, false);
    return -EIOCBQUEUED;  /* "비동기로 처리 중" */
}
```

ioctl vs io_uring passthrough 비교:
- ioctl: `blk_execute_rq()` → 블로킹 대기 → 스레드당 1개씩 직렬 처리
- io_uring: `blk_execute_rq_nowait()` → 즉시 반환 → 다수 커맨드 병렬 실행


## 7. 에러 처리 및 리셋

### 7.1 nvme_timeout() - 타임아웃 처리

blk-mq는 요청이 지정 시간(기본 30초) 내에 완료되지 않으면 `nvme_timeout()`을 호출한다.

**타임아웃 처리 전략 (단계적 에스컬레이션)**:

```
레벨 1: 인터럽트 놓침 확인
  → CQ를 직접 폴링하여 이미 완료되었는지 확인
  → 완료되었으면 BLK_EH_DONE (문제 해결)

레벨 2: Abort 커맨드 전송 (I/O 큐만)
  → Admin 큐로 Abort 커맨드를 보내 해당 커맨드만 취소
  → 타이머 재시작 (BLK_EH_RESET_TIMER)

레벨 3: 컨트롤러 리셋 (Abort 실패 또는 Admin 큐 타임아웃)
  → nvme_dev_disable() → nvme_reset_work() 스케줄
  → 모든 진행 중 요청을 강제 완료
```

```c
static enum blk_eh_timer_return nvme_timeout(struct request *req)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
    struct nvme_queue *nvmeq = req->mq_hctx->driver_data;
    struct nvme_dev *dev = nvmeq->dev;
    u32 csts = readl(dev->bar + NVME_REG_CSTS);

    /* PCI 디바이스 연결 해제 확인 */
    if (pci_dev_is_disconnected(pdev))
        nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_DELETING);
    if (nvme_state_terminal(&dev->ctrl))
        goto disable;

    /* 컨트롤러 치명적 에러(CSTS.CFS) 확인 → 즉시 리셋 */
    if (nvme_should_reset(dev, csts)) {
        nvme_warn_reset(dev, csts);
        goto disable;
    }

    /* ── 레벨 1: 인터럽트 놓침 확인 ── */
    if (test_bit(NVMEQ_POLLED, &nvmeq->flags))
        nvme_poll(req->mq_hctx, NULL);
    else
        nvme_poll_irqdisable(nvmeq);

    /* 이미 완료되었으면 문제 없음 */
    if (blk_mq_rq_state(req) != MQ_RQ_IN_FLIGHT) {
        dev_warn(..., "completion polled\n");
        return BLK_EH_DONE;
    }

    /* ── 레벨 3 조건: Admin 큐이거나 이미 Abort한 커맨드 ── */
    if (!nvmeq->qid || (iod->flags & IOD_ABORTED)) {
        dev_warn(..., "reset controller\n");
        nvme_req(req)->flags |= NVME_REQ_CANCELLED;
        goto disable;
    }

    /* ── 레벨 2: Abort 커맨드 전송 ── */
    if (atomic_dec_return(&dev->ctrl.abort_limit) < 0) {
        atomic_inc(&dev->ctrl.abort_limit);
        return BLK_EH_RESET_TIMER;
    }
    iod->flags |= IOD_ABORTED;

    /* Admin 큐로 Abort 커맨드 전송 */
    cmd.abort.opcode = nvme_admin_abort_cmd;
    cmd.abort.cid = nvme_cid(req);
    cmd.abort.sqid = cpu_to_le16(nvmeq->qid);

    abort_req = blk_mq_alloc_request(dev->ctrl.admin_q, ...);
    blk_execute_rq_nowait(abort_req, false);

    return BLK_EH_RESET_TIMER;  /* Abort 완료를 기다림 */

disable:
    /* 컨트롤러 리셋 스케줄 */
    nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_RESETTING);
    nvme_dev_disable(dev, false);
    nvme_try_sched_reset(&dev->ctrl);
    return BLK_EH_DONE;
}
```


### 7.2 nvme_reset_work() - 컨트롤러 리셋

리셋은 비동기 워크큐(`nvme_reset_wq`)에서 실행된다. 전체 컨트롤러를 재초기화하는 무거운 작업이다.

```c
static void nvme_reset_work(struct work_struct *work)
{
    struct nvme_dev *dev = container_of(work, struct nvme_dev, ctrl.reset_work);

    /* 상태가 RESETTING이 아니면 중단 */
    if (nvme_ctrl_state(&dev->ctrl) != NVME_CTRL_RESETTING)
        goto out;

    /* ── 1단계: 현재 컨트롤러 비활성화 ── */
    if (dev->ctrl.ctrl_config & NVME_CC_ENABLE)
        nvme_dev_disable(dev, false);
    nvme_sync_queues(&dev->ctrl);

    /* ── 2단계: PCI 디바이스 재활성화 + Admin Queue 재설정 ── */
    result = nvme_pci_enable(dev);

    /* ── 3단계: CONNECTING 상태로 전환 ── */
    nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_CONNECTING);

    /* ── 4단계: Identify Controller + 기능 재협상 ── */
    result = nvme_init_ctrl_finish(&dev->ctrl, was_suspend);

    /* ── 5단계: Shadow Doorbell + HMB 재설정 ── */
    nvme_dbbuf_dma_alloc(dev);
    nvme_setup_host_mem(dev);

    /* ── 6단계: I/O 큐 재생성 ── */
    result = nvme_setup_io_queues(dev);

    /* ── 7단계: blk-mq 큐 수 업데이트 ── */
    if (dev->online_queues > 1) {
        nvme_dbbuf_set(dev);
        nvme_unquiesce_io_queues(&dev->ctrl);
        nvme_wait_freeze(&dev->ctrl);
        nvme_pci_update_nr_queues(dev);
        nvme_unfreeze(&dev->ctrl);
    }

    /* ── 8단계: LIVE 상태로 전환 ── */
    nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_LIVE);
    nvme_start_ctrl(&dev->ctrl);
}
```

리셋 전후 상태 전이:
```
LIVE → RESETTING → nvme_dev_disable() → nvme_pci_enable()
                                        → nvme_init_ctrl_finish()
                                        → nvme_setup_io_queues()
                 → CONNECTING → LIVE
```


### 7.3 nvme_dev_disable() - 디바이스 비활성화

셧다운과 리셋 두 경로에서 사용되는 디바이스 비활성화 함수이다.

```c
static void nvme_dev_disable(struct nvme_dev *dev, bool shutdown)
{
    bool dead = nvme_pci_ctrl_is_dead(dev);

    /* 1. I/O 큐 동결 (새 I/O 차단) */
    if (state == NVME_CTRL_LIVE || state == NVME_CTRL_RESETTING) {
        nvme_start_freeze(&dev->ctrl);
        /* 안전한 셧다운이면 진행 중 I/O 완료 대기 */
        if (!dead && shutdown)
            nvme_wait_freeze_timeout(&dev->ctrl, NVME_IO_TIMEOUT);
    }

    /* 2. I/O 큐 quiesce (dispatch 중지) */
    nvme_quiesce_io_queues(&dev->ctrl);
    nvme_quiesce_admin_queue(&dev->ctrl);

    /* 3. I/O 큐 삭제 (SQ 먼저, 그다음 CQ) */
    if (!dead && dev->ctrl.queue_count > 0)
        nvme_delete_io_queues(dev);

    /* 4. 컨트롤러 비활성화 또는 셧다운 */
    if (shutdown)
        nvme_disable_ctrl(&dev->ctrl, true);   /* CC.SHN 설정 */
    else
        nvme_disable_ctrl(&dev->ctrl, false);  /* CC.EN=0 */

    /* 5. 인터럽트 해제 + 큐 일시정지 */
    nvme_suspend_io_queues(dev);
    nvme_suspend_queue(dev, 0);  /* Admin 큐도 */

    /* 6. 남은 CQE 수확 (놓친 완료 처리) */
    nvme_reap_pending_cqes(dev);

    /* 7. 미완료 요청 강제 취소 */
    nvme_cancel_tagset(&dev->ctrl);
    nvme_cancel_admin_tagset(&dev->ctrl);

    /* 8. IRQ 벡터 해제 */
    pci_free_irq_vectors(pdev);
}
```

셧다운 vs 리셋 비교:
```
shutdown=true (nvme_remove, 시스템 종료):
  CC.SHN = NORMAL → 디바이스가 내부 캐시 flush
  → CSTS.SHST = COMPLETE 대기

shutdown=false (리셋):
  CC.EN = 0 → 디바이스 즉시 중지 (flush 없음)
  → 더 빠르지만 데이터 손실 가능성
```


### 7.4 상태 머신 전체 다이어그램

```
                     ┌─────┐
                     │ NEW │
                     └──┬──┘
                        │ nvme_probe() → nvme_pci_enable()
                        ▼
                  ┌────────────┐
           ┌──────┤ CONNECTING │◄────────────────┐
           │      └──────┬─────┘                  │
           │             │ nvme_init_ctrl_finish() │
           │             │ + nvme_setup_io_queues()│
           │             ▼                         │
           │         ┌──────┐                      │
           │    ┌────┤ LIVE │◄───────┐             │
           │    │    └──┬───┘        │             │
           │    │       │            │             │
           │    │       │ 에러/      │ 리셋 성공   │
           │    │       │ 타임아웃   │             │
           │    │       ▼            │             │
           │    │  ┌──────────┐      │             │
           │    │  │ RESETTING │──────┘             │
           │    │  └──────────┘  nvme_reset_work()  │
           │    │       │        → CONNECTING ──────┘
           │    │       │ 리셋 실패
           │    │       ▼
           │    │  ┌──────────┐
           │    ├──┤ DELETING │
           │    │  └──────────┘
           │    │       │
           │    │       ▼
           │    │ ┌──────────────┐
           │    ├─┤DELETING_NOIO│
           │    │ └──────────────┘
           │    │       │
           │    │       ▼
           │    │   ┌──────┐
           │    └──→│ DEAD │
           │        └──────┘
           │
           └─→ (에러 시 DELETING → DEAD)
```
