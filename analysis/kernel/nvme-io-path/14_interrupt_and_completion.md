# NVMe I/O 완료와 인터럽트 서브시스템 심층 분석

> 커널 소스: `drivers/nvme/host/pci.c`, `block/blk-mq.c`, `kernel/softirq.c`, `kernel/irq/manage.c`
> 분석 대상: NVMe I/O 완료가 하드웨어 인터럽트부터 유저 프로세스 반환까지 어떻게 처리되는지

---

## 1. 인터럽트 개요: NVMe I/O 완료의 시작점

NVMe I/O가 완료되려면 디바이스가 호스트에게 "처리가 끝났다"고 알려야 한다. 이 알림 메커니즘이 바로 인터럽트이다. NVMe에서는 MSI-X 인터럽트가 기본이며, 각 I/O 큐마다 별도의 인터럽트 벡터를 할당하여 CPU별 독립 완료 처리가 가능하다.

### 1.1 전체 완료 경로 개요

NVMe I/O 완료는 4단계를 거친다:

1. **하드웨어 인터럽트 (Top Half)**: NVMe 컨트롤러가 MSI-X로 CPU에 인터럽트 발생
2. **인터럽트 핸들러**: `nvme_irq()`가 CQ를 순회하며 CQE를 처리
3. **블록 레이어 완료**: `blk_mq_complete_request()`가 적절한 CPU에서 완료 처리
4. **애플리케이션 반환**: `bio_endio()` → 유저 프로세스 깨우기 (wake_up)

```
 ┌─────────────────────────────────────────────────────────────────────┐
 │                    NVMe I/O 완료 전체 경로                          │
 └─────────────────────────────────────────────────────────────────────┘

 NVMe Controller                     Host CPU
 ┌──────────────┐                   ┌──────────────────────────────────┐
 │              │  ① CQE를 CQ에    │                                  │
 │  NVMe SSD    │───DMA Write──────>│  Host Memory (CQ 버퍼)           │
 │              │                   │  [CQE: cmd_id, status, result]   │
 │              │  ② MSI-X TLP     │                                  │
 │              │───Memory Write───>│  LAPIC (인터럽트 수신)            │
 └──────────────┘                   └───────────┬──────────────────────┘
                                                │
                                    ③ CPU가 인터럽트 수신
                                                │
                                                ▼
                                    ┌──────────────────────┐
                                    │  IDT → irq handler   │
                                    │  (hardirq context)    │
                                    └───────────┬──────────┘
                                                │
                                    ④ nvme_irq() 호출
                                                │
                                                ▼
                                    ┌──────────────────────┐
                                    │  nvme_poll_cq()      │
                                    │  ├─ Phase bit 확인   │
                                    │  ├─ dma_rmb()        │
                                    │  ├─ nvme_handle_cqe()│
                                    │  └─ CQ Doorbell 울림 │
                                    └───────────┬──────────┘
                                                │
                                    ⑤ blk_mq_complete_request()
                                                │
                                    ┌───────────┴──────────────┐
                                    │                          │
                              같은 CPU?               다른 CPU 필요?
                                    │                          │
                                    ▼                          ▼
                          ┌──────────────┐          ┌──────────────────┐
                          │ 직접 완료    │          │ IPI 또는 softirq │
                          │ complete()   │          │ → 대상 CPU에서   │
                          │ 호출         │          │   complete() 호출│
                          └──────┬───────┘          └────────┬─────────┘
                                 │                           │
                                 └───────────┬───────────────┘
                                             │
                                    ⑥ nvme_pci_complete_rq()
                                             │
                                             ▼
                                    ┌──────────────────────┐
                                    │  nvme_complete_rq()  │
                                    │  → nvme_end_req()    │
                                    │  → blk_mq_end_request()
                                    │  → blk_update_request()
                                    │  → bio_endio()       │
                                    │  → wake_up()         │
                                    └──────────────────────┘
```

---

## 2. MSI vs MSI-X vs Legacy INTx

### 2.1 Legacy INTx (PCI 시대의 유산)

Legacy INTx는 PCI 시대의 인터럽트 방식이다. 물리적 인터럽트 라인 (INTA#~INTD#)을 사용하며, 레벨 트리거 방식으로 동작한다.

**핵심 특징:**
- **공유 인터럽트 라인**: 여러 디바이스가 같은 IRQ 라인을 공유한다. 인터럽트가 발생하면 커널은 해당 라인에 등록된 모든 핸들러를 순서대로 호출해야 한다.
- **레벨 트리거**: 인터럽트 라인이 "low"로 유지되는 동안 계속 인터럽트가 발생한다. 핸들러가 디바이스의 인터럽트 원인을 해제해야 라인이 "high"로 돌아간다.
- **대역폭 제한**: 디바이스당 최대 1개의 인터럽트 벡터만 사용 가능하다.
- **NVMe에서의 사용**: 커널은 MSI-X/MSI를 사용할 수 없을 때만 INTx로 fallback한다.

### 2.2 MSI (Message Signaled Interrupts)

MSI는 PCIe Memory Write TLP(Transaction Layer Packet)으로 인터럽트를 전달하는 방식이다. 물리적 인터럽트 라인이 필요 없다.

**핵심 특징:**
- **에지 트리거 의미**: Memory Write TLP 자체가 인터럽트 신호이므로, 라인을 유지할 필요가 없다.
- **벡터 수**: 디바이스당 1~32개의 벡터를 할당할 수 있다.
- **인터럽트 공유 불필요**: 각 벡터가 고유한 인터럽트이므로 핸들러 체인을 순회할 필요가 없다.
- **한계**: 벡터 수가 32개로 제한되어, 많은 큐를 가진 NVMe 디바이스에서는 부족할 수 있다.

### 2.3 MSI-X (Extended Message Signaled Interrupts)

MSI-X는 MSI의 확장 버전으로, NVMe가 기본적으로 사용하는 인터럽트 방식이다.

**핵심 특징:**
- **테이블 기반**: PCIe 디바이스의 BAR에 MSI-X 테이블이 존재한다. 각 엔트리에 Message Address와 Message Data가 저장된다.
- **벡터 최대 2048개**: 각 벡터를 서로 다른 CPU에 할당할 수 있다.
- **per-queue 인터럽트**: NVMe는 각 I/O 큐마다 별도의 MSI-X 벡터를 할당한다. 큐 N의 CQE가 준비되면 벡터 N이 발생하고, 해당 벡터가 바인딩된 CPU에서만 인터럽트가 처리된다.
- **CPU별 독립 처리**: 서로 다른 큐의 인터럽트가 서로 다른 CPU에서 동시에 처리되므로 lock contention이 없다.

### 2.4 NVMe가 MSI-X를 선호하는 이유

NVMe 스펙은 각 I/O 큐마다 독립적인 SQ/CQ 쌍을 가진다. 이 구조에 MSI-X가 완벽히 맞는 이유:

1. **per-CPU 큐 = per-CPU 인터럽트**: CPU 0의 I/O는 큐 1 → MSI-X 벡터 1 → CPU 0에서 완료. 다른 CPU를 방해하지 않는다.
2. **Lock-free 완료**: 각 CPU가 자기 큐의 CQ만 처리하므로 lock이 필요 없다.
3. **캐시 지역성**: I/O를 제출한 CPU에서 완료가 처리되므로 캐시 미스가 최소화된다.
4. **스케일링**: CPU 수에 비례하여 인터럽트 처리 성능이 선형으로 증가한다.

### 2.5 MSI-X가 실제로 동작하는 원리: Memory Write TLP

MSI-X 인터럽트는 실제로 PCIe Memory Write TLP이다. 디바이스가 특정 메모리 주소에 데이터를 쓰면, 이것이 CPU의 Local APIC(Advanced Programmable Interrupt Controller)에 도달하여 인터럽트로 변환된다.

```
 MSI-X 인터럽트 전달 원리
 ========================

 NVMe Controller                PCIe 버스                    CPU
 ┌──────────────┐         ┌────────────────┐         ┌──────────────────┐
 │              │         │                │         │                  │
 │ MSI-X Table  │         │  Memory Write  │         │  LAPIC           │
 │ ┌──────────┐ │         │  TLP           │         │  ┌────────────┐  │
 │ │Entry 0   │ │         │                │         │  │0xFEE00000  │  │
 │ │Addr:FEE..│─┼────────>│  Addr: FEE...  │────────>│  │(LAPIC MMIO)│  │
 │ │Data:0x41 │ │         │  Data: 0x41    │         │  │            │  │
 │ ├──────────┤ │         │                │         │  │→ IDT 조회  │  │
 │ │Entry 1   │ │         │                │         │  │→ 벡터 0x41 │  │
 │ │Addr:FEE..│ │         │                │         │  │→ nvme_irq()│  │
 │ ├──────────┤ │         │                │         │  └────────────┘  │
 │ │Entry 2   │ │         │                │         │                  │
 │ │...       │ │         │                │         │  인터럽트 발생!   │
 │ └──────────┘ │         │                │         │                  │
 └──────────────┘         └────────────────┘         └──────────────────┘

 MSI-X Table Entry 구조:
 ┌──────────────────────────────────────────────┐
 │ Offset 0x00: Message Address (64-bit)        │
 │   = 0xFEE0_0000 + (Destination APIC ID << 12)│
 │   → LAPIC의 MMIO 주소 범위                    │
 │                                               │
 │ Offset 0x08: Message Data (32-bit)            │
 │   = 인터럽트 벡터 번호 + 기타 제어 비트        │
 │   → IDT에서 핸들러를 찾는 데 사용              │
 │                                               │
 │ Offset 0x0C: Vector Control (32-bit)          │
 │   = Mask bit: 1이면 이 벡터 비활성화           │
 └──────────────────────────────────────────────┘
```

**Message Address (0xFEE0_xxxx)** 는 x86의 LAPIC MMIO 영역이다. PCIe Root Complex가 이 주소에 대한 Memory Write를 감지하면, 이를 메모리에 쓰는 대신 대상 CPU의 LAPIC에 인터럽트로 전달한다.

**Message Data** 의 하위 8비트가 인터럽트 벡터 번호이다. CPU는 이 벡터 번호로 IDT(Interrupt Descriptor Table)를 조회하여 `nvme_irq()` 핸들러를 찾는다.

---

## 3. MSI-X 초기화 과정 (NVMe)

NVMe 드라이버가 MSI-X를 설정하는 과정은 크게 3단계로 나뉜다: 벡터 할당, 큐-벡터 매핑, IRQ 핸들러 등록.

### 3.1 pci_alloc_irq_vectors_affinity() — 벡터 할당

NVMe 드라이버는 `nvme_setup_irqs()`에서 `pci_alloc_irq_vectors_affinity()`를 호출하여 MSI-X 벡터를 할당한다.

```c
/* drivers/pci/msi/api.c:252 */
int pci_alloc_irq_vectors_affinity(struct pci_dev *dev, unsigned int min_vecs,
                                   unsigned int max_vecs, unsigned int flags,
                                   struct irq_affinity *affd)
{
    struct irq_affinity msi_default_affd = {0};
    int nvecs = -ENOSPC;

    if (flags & PCI_IRQ_AFFINITY) {
        if (!affd)
            affd = &msi_default_affd;
    } else {
        if (WARN_ON(affd))
            affd = NULL;
    }

    /* 우선순위: MSI-X → MSI → INTx */
    if (flags & PCI_IRQ_MSIX) {
        nvecs = __pci_enable_msix_range(dev, NULL, min_vecs, max_vecs,
                                        affd, flags);
        if (nvecs > 0)
            return nvecs;  /* MSI-X 성공하면 바로 반환 */
    }

    if (flags & PCI_IRQ_MSI) {
        nvecs = __pci_enable_msi_range(dev, min_vecs, max_vecs, affd);
        if (nvecs > 0)
            return nvecs;  /* MSI 성공하면 반환 */
    }

    /* MSI-X도 MSI도 안 되면 INTx fallback */
    if (flags & PCI_IRQ_INTX) {
        if (min_vecs == 1 && dev->irq) {
            if (affd)
                irq_create_affinity_masks(1, affd);
            pci_intx(dev, 1);
            return 1;
        }
    }

    return nvecs;
}
```

**핵심 동작:**
- `PCI_IRQ_ALL_TYPES` 플래그는 `PCI_IRQ_MSIX | PCI_IRQ_MSI | PCI_IRQ_INTX`이다. NVMe 드라이버는 이 플래그를 사용하여 MSI-X를 우선 시도하고, 실패 시 MSI, 그리고 최후에 INTx로 fallback한다.
- `PCI_IRQ_AFFINITY` 플래그가 설정되면 커널이 자동으로 각 벡터를 적절한 CPU에 바인딩한다.
- `__pci_enable_msix_range()`는 PCIe 디바이스의 MSI-X Capability를 읽어 테이블 크기를 확인하고, 요청된 범위 내에서 가능한 최대 벡터를 할당한다.

NVMe 드라이버에서의 첫 호출 (probe 시):

```c
/* drivers/nvme/host/pci.c:3774 */
/* 초기에는 Admin 큐용으로 벡터 1개만 할당 */
result = pci_alloc_irq_vectors(pdev, 1, 1, flags);
```

이후 I/O 큐 설정 시 `nvme_setup_irqs()`에서 실제로 필요한 수만큼 재할당한다.

### 3.2 nvme_setup_irqs() / nvme_calc_irq_sets() — 큐-벡터 매핑 계산

```c
/* drivers/nvme/host/pci.c:3437 */
static int nvme_setup_irqs(struct nvme_dev *dev, unsigned int nr_io_queues)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);
    struct irq_affinity affd = {
        .pre_vectors  = 1,              /* Admin 큐용 벡터 1개 예약 */
        .calc_sets    = nvme_calc_irq_sets, /* 읽기/쓰기 큐 벡터 수 계산 콜백 */
        .priv         = dev,
    };
    unsigned int irq_queues, poll_queues;
    unsigned int flags = PCI_IRQ_ALL_TYPES | PCI_IRQ_AFFINITY;

    /* 폴링 큐는 인터럽트가 필요 없으므로 제외 */
    poll_queues = min(dev->nr_poll_queues, nr_io_queues - 1);
    dev->io_queues[HCTX_TYPE_POLL] = poll_queues;

    /* 기본값 초기화: calc_sets에서 업데이트됨 */
    dev->io_queues[HCTX_TYPE_DEFAULT] = 1;
    dev->io_queues[HCTX_TYPE_READ] = 0;

    /* 필요한 인터럽트 수 = 1(Admin) + I/O 큐 수 - 폴링 큐 수 */
    irq_queues = 1;
    if (!(dev->ctrl.quirks & NVME_QUIRK_SINGLE_VECTOR))
        irq_queues += (nr_io_queues - poll_queues);

    /* Apple 등 일부 디바이스는 MSI가 깨져 있어서 MSI-X만 사용 */
    if (dev->ctrl.quirks & NVME_QUIRK_BROKEN_MSI)
        flags &= ~PCI_IRQ_MSI;

    return pci_alloc_irq_vectors_affinity(pdev, 1, irq_queues, flags, &affd);
}
```

**라인별 해설:**

| 라인 | 설명 |
|------|------|
| `pre_vectors = 1` | MSI-X 벡터 0번은 Admin 큐 전용이다. I/O 큐 벡터는 1번부터 시작한다. |
| `calc_sets = nvme_calc_irq_sets` | 커널의 IRQ affinity 프레임워크가 벡터 할당 시 이 콜백을 호출하여, 읽기 큐와 쓰기 큐에 각각 몇 개의 벡터를 배분할지 결정한다. |
| `poll_queues` | `nvme.poll_queues` 커널 파라미터로 설정된 폴링 전용 큐 수이다. 이 큐들은 인터럽트 없이 CPU가 직접 CQ를 확인한다. |
| `irq_queues` | 실제로 인터럽트가 필요한 큐 수이다. 폴링 큐는 제외한다. |

`nvme_calc_irq_sets()` 콜백:

```c
/* drivers/nvme/host/pci.c:3396 */
static void nvme_calc_irq_sets(struct irq_affinity *affd, unsigned int nrirqs)
{
    struct nvme_dev *dev = affd->priv;
    unsigned int nr_read_queues, nr_write_queues = dev->nr_write_queues;

    if (!nrirqs) {
        nrirqs = 1;
        nr_read_queues = 0;
    } else if (nrirqs == 1 || !nr_write_queues) {
        /* 벡터가 1개뿐이거나 write_queues 파라미터가 없으면 읽기/쓰기 통합 */
        nr_read_queues = 0;
    } else if (nr_write_queues >= nrirqs) {
        /* 쓰기 큐가 너무 많으면 읽기 큐에 최소 1개 보장 */
        nr_read_queues = 1;
    } else {
        nr_read_queues = nrirqs - nr_write_queues;
    }

    /* DEFAULT(쓰기) 큐와 READ 큐에 벡터 배분 */
    dev->io_queues[HCTX_TYPE_DEFAULT] = nrirqs - nr_read_queues;
    affd->set_size[HCTX_TYPE_DEFAULT] = nrirqs - nr_read_queues;
    dev->io_queues[HCTX_TYPE_READ] = nr_read_queues;
    affd->set_size[HCTX_TYPE_READ] = nr_read_queues;
    affd->nr_sets = nr_read_queues ? 2 : 1;
}
```

이 함수는 `nr_write_queues` 모듈 파라미터에 따라 인터럽트 벡터를 읽기 큐와 쓰기(default) 큐로 분리한다. 이 분리를 통해 읽기와 쓰기 워크로드가 서로 다른 CPU에서 처리되어 간섭을 줄일 수 있다.

**예시: 16 CPU 시스템, nr_write_queues=4:**

```
 IRQ 벡터 할당 예시 (16 CPU, write_queues=4)
 ============================================

 벡터 0:  Admin 큐                    (pre_vectors=1)
 벡터 1:  I/O 쓰기 큐 1 → CPU 0      ┐
 벡터 2:  I/O 쓰기 큐 2 → CPU 1      │ HCTX_TYPE_DEFAULT (4개)
 벡터 3:  I/O 쓰기 큐 3 → CPU 2      │
 벡터 4:  I/O 쓰기 큐 4 → CPU 3      ┘
 벡터 5:  I/O 읽기 큐 1 → CPU 4      ┐
 벡터 6:  I/O 읽기 큐 2 → CPU 5      │
 벡터 7:  I/O 읽기 큐 3 → CPU 6      │ HCTX_TYPE_READ (12개)
 ...                                  │
 벡터 16: I/O 읽기 큐 12 → CPU 15    ┘
```

### 3.3 IRQ 핸들러 등록

벡터가 할당된 후, 각 큐에 인터럽트 핸들러를 등록한다.

```c
/* drivers/nvme/host/pci.c:2635 */
static int queue_request_irq(struct nvme_queue *nvmeq)
{
    struct pci_dev *pdev = to_pci_dev(nvmeq->dev->dev);
    int nr = nvmeq->dev->ctrl.instance;  /* nvme0, nvme1 등의 인스턴스 번호 */

    if (use_threaded_interrupts) {
        /* 스레드 모드: hardirq에서는 CQE 확인만, 실제 처리는 커널 스레드에서 */
        return pci_request_irq(pdev, nvmeq->cq_vector,
                nvme_irq_check,   /* hardirq 핸들러: CQE 존재 확인만 */
                nvme_irq,         /* thread 핸들러: 실제 CQ 처리 */
                nvmeq, "nvme%dq%d", nr, nvmeq->qid);
    } else {
        /* 일반 모드 (기본): hardirq에서 CQ 처리까지 전부 수행 */
        return pci_request_irq(pdev, nvmeq->cq_vector,
                nvme_irq,         /* hardirq 핸들러: CQ 처리 전체 */
                NULL,             /* thread 핸들러 없음 */
                nvmeq, "nvme%dq%d", nr, nvmeq->qid);
    }
}
```

`pci_request_irq()`는 내부적으로 `request_threaded_irq()`를 호출한다:

```c
/* drivers/pci/irq.c:37 */
int pci_request_irq(struct pci_dev *dev, unsigned int nr, irq_handler_t handler,
                    irq_handler_t thread_fn, void *dev_id, const char *fmt, ...)
{
    /* ... */
    unsigned long irqflags = IRQF_SHARED;  /* MSI-X도 IRQF_SHARED로 등록 */

    if (!handler)
        irqflags |= IRQF_ONESHOT;  /* thread만 있으면 ONESHOT 자동 설정 */

    /* pci_irq_vector()로 디바이스 벡터 번호 → Linux IRQ 번호 변환 */
    ret = request_threaded_irq(pci_irq_vector(dev, nr), handler, thread_fn,
                               irqflags, devname, dev_id);
    /* ... */
}
```

**IRQF_SHARED 플래그**: MSI-X는 원래 공유될 필요가 없지만, `pci_request_irq()`가 항상 `IRQF_SHARED`를 설정한다. 이는 호환성과 안전을 위한 방어적 설계이다. 실제로 NVMe MSI-X 벡터는 하나의 큐에만 사용되므로 공유되는 경우는 거의 없다.

**스레드 인터럽트 vs 일반 인터럽트:**

```
 일반 모드 (기본):
 ┌─────────┐     ┌──────────────┐
 │ MSI-X   │────>│  nvme_irq()  │  ← hardirq context
 │ 인터럽트│     │  CQ 전체 처리│  ← 가장 빠름, 다른 인터럽트 지연 가능
 └─────────┘     └──────────────┘

 스레드 모드 (use_threaded_interrupts=1):
 ┌─────────┐     ┌────────────────┐     ┌──────────────┐
 │ MSI-X   │────>│ nvme_irq_check │────>│  nvme_irq()  │  ← 커널 스레드
 │ 인터럽트│     │ (CQE 확인만)   │     │  CQ 전체 처리│  ← preemptible
 └─────────┘     │ IRQ_WAKE_THREAD│     └──────────────┘
                 └────────────────┘
                   hardirq (초단시간)
```

스레드 모드는 RT(Real-Time) 커널이나 인터럽트 지연이 문제가 되는 환경에서 사용된다. CQ 처리를 프로세스 컨텍스트로 옮겨서 다른 하드웨어 인터럽트를 지연시키지 않는다.

### 3.4 큐 생성과 IRQ 등록의 전체 흐름

```c
/* drivers/nvme/host/pci.c:2702 */
static int nvme_create_queue(struct nvme_queue *nvmeq, int qid, bool polled)
{
    /* ... */
    u16 vector = 0;

    /* 폴링 큐가 아니면 큐 ID를 벡터 번호로 사용 (1:1 매핑) */
    if (!polled)
        vector = dev->num_vecs == 1 ? 0 : qid;
    else
        set_bit(NVMEQ_POLLED, &nvmeq->flags);  /* 폴링 큐 표시 */

    result = adapter_alloc_cq(dev, qid, nvmeq, vector);  /* CQ 생성 (Admin 커맨드) */
    result = adapter_alloc_sq(dev, qid, nvmeq);           /* SQ 생성 (Admin 커맨드) */

    nvmeq->cq_vector = vector;

    nvme_init_queue(nvmeq, qid);  /* 큐 상태 초기화 */
    if (!polled) {
        result = queue_request_irq(nvmeq);  /* IRQ 핸들러 등록 (폴링 큐는 건너뜀) */
    }

    set_bit(NVMEQ_ENABLED, &nvmeq->flags);
    /* ... */
}
```

**핵심 포인트: 큐 ID = MSI-X 벡터 번호**

벡터가 1개뿐인 특수한 경우를 제외하면, 큐 ID가 곧 MSI-X 벡터 번호이다 (`vector = qid`). 이 1:1 매핑이 per-queue 인터럽트의 기반이다. NVMe 컨트롤러는 CQ를 생성할 때 `Create I/O Completion Queue` Admin 커맨드의 IV(Interrupt Vector) 필드에 이 벡터 번호를 전달받아, 해당 CQ에 완료가 발생하면 그 벡터로 인터럽트를 발생시킨다.

---

## 4. 인터럽트 처리 경로 (Top Half)

### 4.1 하드웨어 인터럽트 발생

NVMe 컨트롤러가 커맨드를 처리하면 다음 순서로 인터럽트가 발생한다:

```
 NVMe 컨트롤러의 커맨드 완료 → 인터럽트 발생 과정
 ================================================

 1. NVMe 컨트롤러가 커맨드 처리 완료
    │
 2. CQ에 CQE(16바이트)를 DMA로 기록
    │  ┌──────────────────────────────────┐
    │  │ CQE (Completion Queue Entry)     │
    │  │  command_specific: 결과 데이터    │
    │  │  sq_head: SQ에서 처리된 위치      │
    │  │  sq_id: 대응하는 SQ ID            │
    │  │  command_id: 원래 커맨드의 tag    │
    │  │  status: 완료 상태 + Phase bit   │
    │  └──────────────────────────────────┘
    │
 3. MSI-X 인터럽트 발생 (Memory Write TLP)
    │  → CQ에 연결된 MSI-X 벡터 번호의 테이블 엔트리 읽기
    │  → 해당 엔트리의 Message Address에 Message Data를 PCIe Write
    │
 4. PCIe Root Complex가 Message Address(0xFEE...)를 인식
    │  → 해당 CPU의 LAPIC에 인터럽트 전달
    │
 5. CPU가 현재 실행 중인 코드를 중단
    │  → 레지스터를 스택에 저장 (context save)
    │  → IDT에서 벡터에 해당하는 핸들러 조회
    │  → irq_enter() → generic_handle_irq()
    │  → nvme_irq() 호출
```

### 4.2 nvme_irq() — Top Half 핸들러 상세 분석

```c
/* drivers/nvme/host/pci.c:1995 */
static irqreturn_t nvme_irq(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;      /* queue_request_irq에서 전달한 dev_id */
    DEFINE_IO_COMP_BATCH(iob);            /* 스택에 배치 완료 구조체 할당 */

    if (nvme_poll_cq(nvmeq, &iob)) {      /* CQ를 순회하며 모든 CQE 처리 */
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob); /* 배치 완료: 여러 요청을 한 번에 처리 */
        return IRQ_HANDLED;               /* "이 인터럽트를 내가 처리했음" */
    }
    return IRQ_NONE;  /* "이 인터럽트는 내 것이 아님" (공유 인터럽트 시) */
}
```

**라인별 해설:**

1. **`struct nvme_queue *nvmeq = data`**: `queue_request_irq()`에서 `pci_request_irq()`에 전달한 `dev_id`가 그대로 돌아온다. 이를 통해 어떤 큐의 인터럽트인지 즉시 알 수 있다.

2. **`DEFINE_IO_COMP_BATCH(iob)`**: 배치 완료를 위한 구조체이다. 여러 CQE를 처리할 때 개별적으로 `complete()`를 호출하지 않고, 모아서 한 번에 처리하면 오버헤드가 줄어든다.

3. **`nvme_poll_cq(nvmeq, &iob)`**: CQ를 순회하며 모든 대기 중인 CQE를 처리한다. 반환값이 `true`이면 하나 이상의 CQE를 처리한 것이다.

4. **`nvme_pci_complete_batch(&iob)`**: 배치로 모은 완료들을 한 번에 처리한다. DMA unmap과 `blk_mq_end_request()`를 일괄 수행한다.

5. **`IRQ_HANDLED` / `IRQ_NONE`**: 공유 인터럽트 환경에서 커널이 어떤 디바이스가 인터럽트를 발생시켰는지 판단하는 데 사용한다. `IRQ_NONE`을 반환하면 커널이 다음 핸들러를 호출한다.

### 4.3 nvme_poll_cq() — CQ 순회의 핵심

```c
/* drivers/nvme/host/pci.c:1955 */
static inline bool nvme_poll_cq(struct nvme_queue *nvmeq,
                                struct io_comp_batch *iob)
{
    bool found = false;

    while (nvme_cqe_pending(nvmeq)) {    /* Phase bit으로 새 CQE 확인 */
        found = true;
        dma_rmb();                        /* DMA 읽기 배리어: Phase 이후 나머지 필드 읽기 보장 */
        nvme_handle_cqe(nvmeq, iob, nvmeq->cq_head);  /* CQE 처리 */
        nvme_update_cq_head(nvmeq);       /* CQ Head 전진 (필요시 Phase 토글) */
    }

    if (found)
        nvme_ring_cq_doorbell(nvmeq);     /* 디바이스에 "여기까지 읽었다" 알림 */
    return found;
}
```

**Phase bit 확인:**

```c
/* drivers/nvme/host/pci.c:1838 */
static inline bool nvme_cqe_pending(struct nvme_queue *nvmeq)
{
    struct nvme_completion *hcqe = &nvmeq->cqes[nvmeq->cq_head];
    /* CQE의 status 비트0(Phase)이 호스트의 cq_phase와 일치하면 새 CQE */
    return (le16_to_cpu(READ_ONCE(hcqe->status)) & 1) == nvmeq->cq_phase;
}
```

Phase bit의 동작:
```
 Phase Bit 동작 원리
 ====================

 CQ 초기 상태: phase=1, 모든 CQE의 phase=0 (memset 0)
 → 불일치 → CQE가 없음

 첫 번째 라운드 (phase=1):
 ┌─────┬─────┬─────┬─────┬─────┐
 │CQE 0│CQE 1│CQE 2│CQE 3│ ... │  CQ (depth=N)
 │ P=1 │ P=1 │ P=0 │ P=0 │     │
 └──┬──┴──┬──┴─────┴─────┴─────┘
    │     │
    │     └─ head=2: phase=0 ≠ cq_phase=1 → 처리할 CQE 없음
    └─ head=0: phase=1 == cq_phase=1 → 새 CQE! 처리 후 head 전진

 두 번째 라운드 (head가 0으로 돌아오면 phase 토글: phase=0):
 ┌─────┬─────┬─────┬─────┬─────┐
 │CQE 0│CQE 1│CQE 2│CQE 3│ ... │
 │ P=0 │ P=1 │ P=1 │ P=1 │     │
 └──┬──┴─────┴─────┴─────┴─────┘
    │
    └─ head=0: phase=0 == cq_phase=0 → 새 CQE!

 → Phase 토글 덕분에 CQ 전체를 0으로 초기화하지 않아도
   새 CQE와 이전 라운드의 CQE를 구분할 수 있다.
```

**dma_rmb() 배리어의 역할:**

디바이스가 CQE를 DMA로 쓸 때, Phase bit가 먼저 보이고 나머지 필드(command_id, status 등)가 나중에 보일 수 있다. `dma_rmb()`는 Phase bit 확인 후 나머지 필드를 읽기 전에 DMA 쓰기가 완료되었음을 보장한다. 이 배리어가 없으면 호스트가 아직 기록되지 않은 command_id를 읽어 엉뚱한 요청을 완료시킬 수 있다.

### 4.4 nvme_handle_cqe() — CQE에서 요청 완료

```c
/* drivers/nvme/host/pci.c:1883 */
static inline void nvme_handle_cqe(struct nvme_queue *nvmeq,
                                   struct io_comp_batch *iob, u16 idx)
{
    struct nvme_completion *cqe = &nvmeq->cqes[idx];
    __u16 command_id = READ_ONCE(cqe->command_id);
    struct request *req;

    /* AEN(비동기 이벤트)은 별도 처리 */
    if (unlikely(nvme_is_aen_req(nvmeq->qid, command_id))) {
        nvme_complete_async_event(&nvmeq->dev->ctrl,
                cqe->status, &cqe->result);
        return;
    }

    /* command_id(=blk-mq tag)로 원래 request 찾기 */
    req = nvme_find_rq(nvme_queue_tagset(nvmeq), command_id);
    if (unlikely(!req)) {
        dev_warn(/* "invalid id completed" */);
        return;
    }

    trace_nvme_sq(req, cqe->sq_head, nvmeq->sq_tail);

    /* 완료 처리 시도: IPI/softirq로 전달되면 true, 아니면 false */
    if (!nvme_try_complete_req(req, cqe->status, cqe->result) &&
        !blk_mq_add_to_batch(req, iob,
                             nvme_req(req)->status != NVME_SC_SUCCESS,
                             nvme_pci_complete_batch))
        nvme_pci_complete_rq(req);  /* 배치에도 못 넣으면 즉시 완료 */
}
```

**완료 경로 분기 로직:**

```
 nvme_handle_cqe() 완료 경로 분기
 =================================

 nvme_try_complete_req(req, status, result)
     │
     ├─ true:  완료가 IPI로 다른 CPU에 전달됨 → 여기서 끝
     │
     └─ false: 현재 CPU에서 완료 처리 계속
               │
               blk_mq_add_to_batch(req, iob, ...)
               │
               ├─ true (non-NULL iob):  배치에 추가됨 → 나중에 일괄 완료
               │                        (nvme_irq에서 nvme_pci_complete_batch 호출)
               │
               └─ false (NULL iob 또는 에러):
                         │
                         nvme_pci_complete_rq(req)  → 즉시 완료
```

### 4.5 nvme_irq_check() — 스레드 인터럽트의 Top Half

`use_threaded_interrupts=1`일 때 hardirq에서 실행되는 최소 핸들러:

```c
/* drivers/nvme/host/pci.c:2014 */
static irqreturn_t nvme_irq_check(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;

    if (nvme_cqe_pending(nvmeq))
        return IRQ_WAKE_THREAD;  /* CQE가 있으면 커널 스레드를 깨움 */
    return IRQ_NONE;
}
```

이 핸들러는 CQE가 있는지 Phase bit만 확인하고 즉시 리턴한다. 실제 CQ 처리(`nvme_irq()`)는 커널 스레드(irq/N-nvme0qM)에서 실행된다. hardirq 시간을 극도로 줄여 다른 인터럽트에 대한 응답성을 보장한다.

---

## 5. 완료 처리 경로 (Bottom Half)

### 5.1 blk_mq_complete_request()의 완료 방식 결정

nvme_handle_cqe()에서 `nvme_try_complete_req()`가 호출되면, 내부적으로 `blk_mq_complete_request_remote()`가 호출되어 완료 처리를 어디서 할지 결정한다.

```c
/* block/blk-mq.c:1580 */
bool blk_mq_complete_request_remote(struct request *rq)
{
    WRITE_ONCE(rq->state, MQ_RQ_COMPLETE);  /* 상태를 COMPLETE로 전환 */

    /*
     * 조건 1: hctx에 ctx가 하나뿐이고 현재 CPU가 그 ctx의 CPU이면 → 로컬 완료
     * 조건 2: 폴링 요청이면 → 로컬 완료 (폴링 CPU에서 직접 처리)
     */
    if ((rq->mq_hctx->nr_ctx == 1 &&
         rq->mq_ctx->cpu == raw_smp_processor_id()) ||
         rq->cmd_flags & REQ_POLLED)
        return false;  /* 호출자가 직접 complete() 호출 */

    /* IPI가 필요한지 확인 */
    if (blk_mq_complete_need_ipi(rq)) {
        blk_mq_complete_send_ipi(rq);  /* IPI로 제출 CPU에 완료 전달 */
        return true;
    }

    /* 단일 hw 큐이면 softirq로 처리 */
    if (rq->q->nr_hw_queues == 1) {
        blk_mq_raise_softirq(rq);
        return true;
    }

    return false;  /* 그 외: 호출자가 직접 complete() */
}
```

그리고 `blk_mq_complete_request()`가 최종 결정을 내린다:

```c
/* block/blk-mq.c:1626 */
void blk_mq_complete_request(struct request *rq)
{
    if (!blk_mq_complete_request_remote(rq))
        rq->q->mq_ops->complete(rq);  /* NVMe: nvme_pci_complete_rq() */
}
```

### 5.2 3가지 완료 방식 비교

```
 ┌───────────────────────────────────────────────────────────────────┐
 │              blk_mq_complete_request() 완료 방식 결정            │
 └───────────────────────────────────────────────────────────────────┘

 방식 ①: 로컬 직접 완료 (가장 빠름)
 ───────────────────────────────
 조건: 인터럽트 CPU == 제출 CPU, 또는 per-CPU 큐, 또는 폴링
 경로: nvme_irq() → blk_mq_complete_request() → nvme_pci_complete_rq()
 특징: 컨텍스트 전환 없음, 캐시 최적

 ┌──────────┐    직접 호출     ┌──────────────────────┐
 │ CPU 0    │ ──────────────> │ nvme_pci_complete_rq()│
 │ nvme_irq │                 │ (같은 CPU에서 즉시)   │
 └──────────┘                 └──────────────────────┘

 방식 ②: IPI를 통한 원격 완료
 ───────────────────────────
 조건: QUEUE_FLAG_SAME_COMP 설정 + 인터럽트 CPU ≠ 제출 CPU
 경로: nvme_irq() → blk_mq_complete_send_ipi()
       → [IPI] → __blk_mq_complete_request_remote()
       → raise_softirq(BLOCK_SOFTIRQ) → blk_done_softirq()
 특징: 제출 CPU에서 완료하여 캐시 지역성 확보, IPI 오버헤드 발생

 ┌──────────┐    IPI          ┌──────────┐    softirq    ┌──────────┐
 │ CPU 0    │ ──────────────> │ CPU 3    │ ────────────> │ complete │
 │ nvme_irq │                 │ (제출CPU)│               │ 처리     │
 └──────────┘                 └──────────┘               └──────────┘

 방식 ③: softirq를 통한 지연 완료
 ──────────────────────────────
 조건: 단일 hw 큐 (nr_hw_queues == 1)
 경로: nvme_irq() → blk_mq_raise_softirq()
       → raise_softirq(BLOCK_SOFTIRQ)
       → blk_done_softirq() → nvme_pci_complete_rq()
 특징: 인터럽트 컨텍스트를 빨리 벗어남, 약간의 지연

 ┌──────────┐    per-CPU list   ┌──────────┐
 │ nvme_irq │ ────────────────> │ softirq  │
 │ (hardirq)│ raise_softirq()  │ 처리     │
 └──────────┘                   └──────────┘
```

### 5.3 IPI 필요 여부 판단: blk_mq_complete_need_ipi()

```c
/* block/blk-mq.c:1521 */
static inline bool blk_mq_complete_need_ipi(struct request *rq)
{
    int cpu = raw_smp_processor_id();  /* 현재 CPU (인터럽트 처리 중인 CPU) */

    /* SMP가 아니거나 SAME_COMP 플래그가 없으면 IPI 불필요 */
    if (!IS_ENABLED(CONFIG_SMP) ||
        !test_bit(QUEUE_FLAG_SAME_COMP, &rq->q->queue_flags))
        return false;

    /*
     * force_irqthreads가 켜져 있으면 IPI로 보내면 ksoftirqd가 깨어나
     * 오히려 성능이 나빠질 수 있다. 다른 캐시 도메인에서 완료하는 게 나을 수 있다.
     */
    if (force_irqthreads())
        return false;

    /* 같은 CPU이거나, 같은 캐시 도메인+같은 용량이면 → IPI 불필요 */
    if (cpu == rq->mq_ctx->cpu ||
        (!test_bit(QUEUE_FLAG_SAME_FORCE, &rq->q->queue_flags) &&
         cpus_share_cache(cpu, rq->mq_ctx->cpu) &&
         cpus_equal_capacity(cpu, rq->mq_ctx->cpu)))
        return false;

    /* 오프라인 CPU에는 IPI 보내지 않음 */
    return cpu_online(rq->mq_ctx->cpu);
}
```

**캐시 도메인 개념:**

같은 L2/L3 캐시를 공유하는 CPU들은 같은 캐시 도메인에 속한다. 예를 들어 하이퍼스레딩으로 2개의 논리 CPU가 같은 물리 코어를 공유하면, CPU 0에서 인터럽트를 받아 CPU 1(같은 코어)에서 완료해도 캐시 미스가 발생하지 않는다. 이 경우 IPI를 보내는 오버헤드가 더 크므로 로컬에서 완료한다.

### 5.4 IPI를 통한 완료 전달

```c
/* block/blk-mq.c:1548 */
static void blk_mq_complete_send_ipi(struct request *rq)
{
    unsigned int cpu;

    cpu = rq->mq_ctx->cpu;  /* I/O를 제출한 CPU */
    /* per-CPU 완료 리스트에 추가하고 IPI 전송 */
    if (llist_add(&rq->ipi_list, &per_cpu(blk_cpu_done, cpu)))
        smp_call_function_single_async(cpu, &per_cpu(blk_cpu_csd, cpu));
}
```

IPI를 받은 대상 CPU에서 실행되는 콜백:

```c
/* block/blk-mq.c:1509 */
static void __blk_mq_complete_request_remote(void *data)
{
    __raise_softirq_irqoff(BLOCK_SOFTIRQ);  /* 대상 CPU에서 softirq 발생 */
}
```

IPI 콜백은 단지 `BLOCK_SOFTIRQ`를 raise할 뿐이다. 실제 완료 처리는 softirq에서 이루어진다.

### 5.5 softirq 기반 완료: BLOCK_SOFTIRQ

**softirq 등록 (부팅 시):**

```c
/* block/blk-mq.c:6008 */
static int __init blk_mq_init(void)
{
    int i;

    /* per-CPU 완료 리스트 초기화 */
    for_each_possible_cpu(i)
        init_llist_head(&per_cpu(blk_cpu_done, i));

    /* per-CPU CSD(Cross-CPU call data) 초기화 - IPI 완료용 */
    for_each_possible_cpu(i)
        INIT_CSD(&per_cpu(blk_cpu_csd, i),
                 __blk_mq_complete_request_remote, NULL);

    /* BLOCK_SOFTIRQ에 blk_done_softirq 핸들러 등록 */
    open_softirq(BLOCK_SOFTIRQ, blk_done_softirq);

    /* CPU hotplug 콜백 등록 */
    cpuhp_setup_state_nocalls(CPUHP_BLOCK_SOFTIRQ_DEAD,
                              "block/softirq:dead", NULL,
                              blk_softirq_cpu_dead);
    /* ... */
}
subsys_initcall(blk_mq_init);  /* 드라이버보다 먼저 실행 */
```

**softirq 발생:**

```c
/* block/blk-mq.c:1557 */
static void blk_mq_raise_softirq(struct request *rq)
{
    struct llist_head *list;

    preempt_disable();
    list = this_cpu_ptr(&blk_cpu_done);
    if (llist_add(&rq->ipi_list, list))   /* per-CPU 리스트에 추가 */
        raise_softirq(BLOCK_SOFTIRQ);     /* softirq 플래그 설정 */
    preempt_enable();
}
```

**softirq 처리:**

```c
/* block/blk-mq.c:1498 */
static __latent_entropy void blk_done_softirq(void)
{
    blk_complete_reqs(this_cpu_ptr(&blk_cpu_done));
}
```

`blk_complete_reqs()`는 per-CPU 리스트의 모든 요청에 대해 `rq->q->mq_ops->complete(rq)`를 호출한다. NVMe의 경우 이것이 `nvme_pci_complete_rq()`이다.

**softirq 실행 시점:**

```c
/* kernel/softirq.c:713 */
static inline void __irq_exit_rcu(void)
{
    /* ... */
    preempt_count_sub(HARDIRQ_OFFSET);
    if (!in_interrupt() && local_softirq_pending())
        invoke_softirq();  /* hardirq 리턴 시 pending softirq 실행 */
    /* ... */
}
```

softirq는 두 가지 시점에 실행된다:
1. **hardirq 리턴 시**: `irq_exit()`에서 `invoke_softirq()`를 호출. 가장 일반적이고 가장 빠른 경로이다.
2. **ksoftirqd 스레드**: softirq 처리가 너무 오래 걸리거나(`MAX_SOFTIRQ_TIME` 초과, `MAX_SOFTIRQ_RESTART` 도달), 프로세스 컨텍스트에서 `raise_softirq()`가 호출되면 ksoftirqd가 깨어난다.

```
 softirq 실행 타이밍
 ====================

 경로 1: hardirq → softirq (가장 빠름, 대부분의 경우)
 ┌─────────┐   ┌──────────────────┐   ┌──────────────┐
 │ hardirq │──>│ irq_exit_rcu()   │──>│ invoke_softirq│
 │ 완료    │   │ pending 확인     │   │ → __do_softirq│
 └─────────┘   └──────────────────┘   │ → handle_softirqs
                                       │ → blk_done_softirq
                                       └──────────────┘

 경로 2: ksoftirqd (지연 처리)
 ┌──────────────┐   ┌─────────────────┐
 │ raise_softirq│──>│ wakeup_softirqd │
 │ (프로세스    │   │ → ksoftirqd 스레드
 │  컨텍스트)   │   │ → handle_softirqs
 └──────────────┘   │ → blk_done_softirq
                    └─────────────────┘
```

### 5.6 handle_softirqs() — softirq 디스패치

```c
/* kernel/softirq.c:579 */
static void handle_softirqs(bool ksirqd)
{
    unsigned long end = jiffies + MAX_SOFTIRQ_TIME;
    int max_restart = MAX_SOFTIRQ_RESTART;
    __u32 pending;

    pending = local_softirq_pending();  /* pending 비트맵 읽기 */

    /* ... */

restart:
    set_softirq_pending(0);  /* pending 비트맵 클리어 */
    local_irq_enable();       /* 인터럽트 재활성화 (softirq 중에도 hardirq 가능) */

    h = softirq_vec;

    while ((softirq_bit = ffs(pending))) {  /* 비트맵에서 set된 비트 찾기 */
        h += softirq_bit - 1;
        vec_nr = h - softirq_vec;

        h->action();  /* BLOCK_SOFTIRQ이면 blk_done_softirq() 호출 */

        h++;
        pending >>= softirq_bit;
    }

    local_irq_disable();

    pending = local_softirq_pending();
    if (pending) {
        /* 처리할 softirq가 더 있으면 재시도 (시간/횟수 제한) */
        if (time_before(jiffies, end) && !need_resched() && --max_restart)
            goto restart;
        wakeup_softirqd();  /* 제한 초과 시 ksoftirqd에게 위임 */
    }
    /* ... */
}
```

softirq 비트맵에서 `BLOCK_SOFTIRQ`(=4)가 set되어 있으면 `blk_done_softirq()`가 호출된다. softirq의 우선순위는 비트 번호 순이다:

```
 softirq 우선순위 (include/linux/interrupt.h)
 =============================================
 0: HI_SOFTIRQ         (고우선순위 tasklet)
 1: TIMER_SOFTIRQ      (타이머)
 2: NET_TX_SOFTIRQ     (네트워크 송신)
 3: NET_RX_SOFTIRQ     (네트워크 수신)
 4: BLOCK_SOFTIRQ      ★ (블록 I/O 완료)
 5: IRQ_POLL_SOFTIRQ   (IRQ 폴링)
 6: TASKLET_SOFTIRQ    (일반 tasklet)
 7: SCHED_SOFTIRQ      (스케줄러)
 8: HRTIMER_SOFTIRQ    (고해상도 타이머)
 9: RCU_SOFTIRQ        (RCU)
```

### 5.7 nvme_pci_complete_rq() → bio_endio() — 최종 완료

인터럽트에서 시작된 완료 처리가 최종적으로 도달하는 곳:

```c
/* drivers/nvme/host/pci.c:1812 */
static void nvme_pci_complete_rq(struct request *req)
{
    nvme_pci_unmap_rq(req);    /* DMA 매핑 해제 (IOMMU 엔트리 반환 등) */
    nvme_complete_rq(req);     /* NVMe core의 완료 처리 */
}

/* drivers/nvme/host/core.c:639 */
void nvme_complete_rq(struct request *req)
{
    struct nvme_ctrl *ctrl = nvme_req(req)->ctrl;

    trace_nvme_complete_rq(req);  /* ftrace 추적 */
    nvme_cleanup_cmd(req);        /* discard 등 특수 페이로드 정리 */

    /* Keep-Alive 타이머 업데이트 */
    if (ctrl->kas && req->deadline - req->timeout >= ctrl->ka_last_check_time)
        ctrl->comp_seen = true;

    switch (nvme_decide_disposition(req)) {
    case COMPLETE:
        nvme_end_req(req);         /* 정상 완료 → blk_mq_end_request() */
        return;
    case RETRY:
        nvme_retry_req(req);       /* 재시도 가능한 에러 → 재큐잉 */
        return;
    case FAILOVER:
        nvme_failover_req(req);    /* 멀티패스: 다른 경로로 전환 */
        return;
    case AUTHENTICATE:
        /* NVMe-oF 인증 실패 → 인증 후 재시도 */
        /* ... */
        return;
    }
}

/* drivers/nvme/host/core.c:610 */
void nvme_end_req(struct request *req)
{
    blk_status_t status = nvme_error_status(nvme_req(req)->status);
    __nvme_end_req(req);
    blk_mq_end_request(req, status);  /* ★ 블록 레이어 최종 완료 */
}
```

`blk_mq_end_request()`에서 `blk_update_request()`가 호출되면, 이것이 `bio_endio()`를 통해 파일시스템이나 직접 I/O 콜백을 호출한다. 최종적으로 유저 프로세스가 깨어난다 (`wake_up()`).

```c
/* block/blk-mq.c:1392 */
void blk_mq_end_request(struct request *rq, blk_status_t error)
{
    /* bio 체인의 모든 bio에 bio_endio() 호출 */
    if (blk_update_request(rq, error, blk_rq_bytes(rq)))
        BUG();
    /* 통계 업데이트 + tag 반환 + request 해제 */
    __blk_mq_end_request(rq, error);
}
```

**전체 완료 호출 체인 요약:**

```
 하드웨어 인터럽트
   → nvme_irq()
     → nvme_poll_cq()
       → nvme_handle_cqe()
         → nvme_try_complete_req()
           → blk_mq_complete_request_remote()
             [로컬/IPI/softirq 분기]
               → nvme_pci_complete_rq()     ← mq_ops->complete
                 → nvme_pci_unmap_rq()      ← DMA 해제
                 → nvme_complete_rq()       ← NVMe core
                   → nvme_end_req()
                     → blk_mq_end_request()
                       → blk_update_request()
                         → bio_endio()      ← 파일시스템 콜백
                           → wake_up()      ← 유저 프로세스 깨우기
                       → __blk_mq_end_request()
                         → blk_mq_free_request()  ← tag + request 해제
```

---

## 6. Polling 모드 (인터럽트 없는 완료)

### 6.1 io_uring IOPOLL

NVMe의 폴링 모드는 인터럽트를 사용하지 않고 CPU가 직접 CQ를 확인하는 방식이다. 인터럽트 오버헤드(IDT 조회, 컨텍스트 저장/복원, softirq 스케줄링)를 완전히 제거하여 초저지연을 달성한다.

**io_uring에서의 폴링 활성화:**

```c
/* 유저 코드 */
struct io_uring_params p = {
    .flags = IORING_SETUP_IOPOLL,  /* 폴링 모드 활성화 */
};
io_uring_setup(queue_depth, &p);
```

**폴링 경로:**

```
 io_uring IOPOLL 전체 경로
 ==========================

 유저 프로세스                      커널                           NVMe
 ┌──────────┐                   ┌──────────────────┐         ┌──────────┐
 │          │  io_uring_enter() │                  │  SQ DB  │          │
 │ io_uring │ ────────────────> │  I/O 제출        │ ──────> │ 커맨드   │
 │ 제출     │                   │  (REQ_POLLED 설정)│         │ 처리     │
 │          │                   │                  │         │          │
 │          │  io_uring_enter() │                  │         │  CQE를   │
 │ 폴링     │ ────────────────> │  io_iopoll_check │         │  CQ에    │
 │ (완료    │                   │  ├─ io_do_iopoll │         │  DMA     │
 │  대기)   │                   │  │  └─ nvme_poll()│<────────│  기록    │
 │          │                   │  │     └─ CQ 확인 │         │          │
 │          │                   │  │                │         │ (인터럽트│
 │          │ <──── CQE 반환 ── │  └─ 완료 처리    │         │  없음!)  │
 └──────────┘                   └──────────────────┘         └──────────┘

 인터럽트 vs 폴링 타이밍 비교:

 인터럽트 모드:
 ─────────┬───────────────────────────────┬──────
          │← 인터럽트 지연 (IDT, ctx save │
 CQE 도착 │   softirq 스케줄링 등) ──────>│ 완료

 폴링 모드:
 ─────────┬──────┬──────
          │ 폴링 │
 CQE 도착 │ 확인 │ 즉시 완료
```

**커널 내부 폴링 경로:**

```c
/* io_uring/io_uring.c:1586 */
static int io_iopoll_check(struct io_ring_ctx *ctx, unsigned int min_events)
{
    unsigned int nr_events = 0;

    do {
        int ret = 0;

        /* iopoll_list가 비어있으면 task_work 처리 */
        if (wq_list_empty(&ctx->iopoll_list) || io_task_work_pending(ctx)) {
            /* ... task_work 처리 ... */
        }

        /* 실제 폴링: 블록 레이어의 blk_poll()을 통해 nvme_poll() 호출 */
        ret = io_do_iopoll(ctx, !min_events);
        if (unlikely(ret < 0))
            return ret;

        /* 시그널 처리 / 리스케줄 확인 */
        if (task_sigpending(current))
            return -EINTR;
        if (need_resched())
            break;

        nr_events += ret;
    } while (nr_events < min_events);  /* 최소 이벤트 수를 채울 때까지 반복 */

    return 0;
}
```

### 6.2 nvme_poll() — NVMe 폴링 핸들러

```c
/* drivers/nvme/host/pci.c:2047 */
static int nvme_poll(struct blk_mq_hw_ctx *hctx, struct io_comp_batch *iob)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    bool found;

    /* 빠른 확인: CQE가 없으면 즉시 리턴 (lock 획득 없이) */
    if (!nvme_cqe_pending(nvmeq))
        return 0;

    /* CQE가 있으면 lock을 잡고 처리 */
    spin_lock(&nvmeq->cq_poll_lock);
    found = nvme_poll_cq(nvmeq, iob);
    spin_unlock(&nvmeq->cq_poll_lock);

    return found;
}
```

**핵심 포인트:**

1. **인터럽트와 같은 처리 로직**: `nvme_poll_cq()`는 인터럽트 핸들러(`nvme_irq()`)와 폴링 모드 모두에서 사용되는 동일한 함수이다. CQ를 순회하며 CQE를 처리하는 로직은 완전히 동일하다.

2. **cq_poll_lock**: 폴링 큐는 인터럽트가 없어서 여러 컨텍스트에서 동시에 폴링할 수 있다(예: 타임아웃 핸들러 + 정상 폴링). 이를 방지하기 위해 spin_lock을 사용한다. 인터럽트 큐에서는 인터럽트 핸들러가 serialized되므로 이 lock이 필요 없다.

3. **빠른 경로 최적화**: `nvme_cqe_pending()`을 lock 없이 먼저 확인하여, CQE가 없을 때 lock 오버헤드를 제거한다.

### 6.3 SPDK와의 비교: 폴링의 극한

커널의 폴링 모드는 SPDK의 처리 모델과 본질적으로 같다:

```
 커널 폴링 vs SPDK 비교
 =======================

 커널 IOPOLL:
 ┌──────────────────────────────────────────────┐
 │ io_uring_enter()                              │
 │   └─ io_iopoll_check()                        │
 │       └─ io_do_iopoll()                       │
 │           └─ blk_poll()                       │
 │               └─ nvme_poll()                  │
 │                   └─ nvme_poll_cq()           │ ← 블록 레이어, VFS,
 │                                               │   스케줄러 경유
 │ 장점: 기존 커널 인프라 활용 (파일시스템 등)    │
 │ 단점: 시스템콜 + 블록레이어 오버헤드 존재      │
 └──────────────────────────────────────────────┘

 SPDK:
 ┌──────────────────────────────────────────────┐
 │ spdk_nvme_qpair_process_completions()         │
 │   └─ 직접 CQ 메모리 확인 (Phase bit)          │
 │   └─ 콜백 호출                                │
 │                                               │
 │ 장점: 커널 바이패스, 시스템콜 없음             │
 │ 단점: 커널 파일시스템 사용 불가                │
 └──────────────────────────────────────────────┘
```

### 6.4 장단점 비교

**장점:**
- 인터럽트 오버헤드 완전 제거 (IDT 조회, 레지스터 저장/복원, softirq 스케줄링 등)
- 레이턴시 감소: 일반적으로 1~3μs 개선 (고성능 NVMe SSD에서는 전체 레이턴시의 10~30%)
- SPDK에 근접한 성능을 커널에서 달성 가능

**단점:**
- CPU 100% 점유: 폴링 루프가 계속 돌므로 해당 CPU를 다른 작업에 사용할 수 없다
- 전력 소비 증가: C-state 진입 불가
- QD가 낮으면 대부분의 폴링이 헛돈다 (CQE가 없는데 계속 확인)

### 6.5 Hybrid Polling

Hybrid Polling은 인터럽트 모드와 폴링 모드의 장점을 결합한다.

**동작 원리:**
1. I/O 제출 직후에는 sleep (CPU를 다른 작업에 양보)
2. 예상 완료 시점이 가까워지면 busy-poll 시작
3. 예상 완료 시간은 이전 I/O들의 평균 레이턴시를 기반으로 계산

```
 Hybrid Polling 타이밍
 =====================

 시간 →
 ┌─────┬──────────────────┬────────┬──────┐
 │ 제출│  sleep (CPU 양보) │  poll  │ 완료 │
 └─────┴──────────────────┴────────┴──────┘
                           ↑
                     예상 완료 시점의
                     약 50% 지점에서
                     busy-poll 시작

 vs 순수 폴링:
 ┌─────┬──────────────────────────────────┐
 │ 제출│  busy-poll (CPU 100% 점유) → 완료│
 └─────┴──────────────────────────────────┘

 vs 인터럽트:
 ┌─────┬──────────────────┬───────┬───────┐
 │ 제출│  sleep            │IRQ   │ 완료  │
 └─────┴──────────────────┴───────┴───────┘
                             ↑
                       인터럽트 지연
```

Hybrid Polling은 `/sys/block/nvmeXnY/queue/io_poll_delay`로 제어할 수 있다:
- `-1`: 클래식 폴링 (busy-wait 전용)
- `0`: hybrid polling (커널이 자동으로 최적 sleep 시간 계산)
- `>0`: 고정 delay (μs 단위)

---

## 7. IRQ Affinity와 NUMA

### 7.1 IRQ CPU 바인딩

MSI-X 벡터가 할당될 때, 커널의 IRQ affinity 프레임워크가 각 벡터를 적절한 CPU에 자동 바인딩한다. `PCI_IRQ_AFFINITY` 플래그를 사용하면 이 자동 바인딩이 활성화된다.

**확인/설정 방법:**

```bash
# IRQ 번호 확인
cat /proc/interrupts | grep nvme

# 특정 IRQ의 CPU affinity 확인 (비트마스크)
cat /proc/irq/123/smp_affinity

# 특정 CPU에 바인딩 (예: CPU 3)
echo 8 > /proc/irq/123/smp_affinity  # 비트3 = 0x8

# effective affinity 확인 (실제로 적용된 값)
cat /proc/irq/123/effective_affinity
```

**managed_irq:**

NVMe 드라이버가 `PCI_IRQ_AFFINITY`를 사용하면, 할당된 IRQ는 "managed IRQ"가 된다. managed IRQ의 특징:

1. 커널이 자동으로 CPU를 할당하며, 수동 변경이 제한된다.
2. CPU가 오프라인되면 IRQ가 자동으로 다른 CPU로 마이그레이션된다.
3. CPU가 다시 온라인되면 원래 CPU로 복귀한다.
4. NUMA 토폴로지를 고려하여 같은 NUMA 노드의 CPU에 우선 배치한다.

### 7.2 NUMA 노드 고려

NVMe SSD가 연결된 PCIe 슬롯은 특정 NUMA 노드에 속한다. 최적의 성능을 위해서는 I/O를 처리하는 CPU가 NVMe SSD와 같은 NUMA 노드에 있어야 한다.

```
 NUMA 토폴로지와 NVMe IRQ
 =========================

 ┌─────────────────────────────────────────────────┐
 │  NUMA Node 0                                    │
 │  ┌──────────┐  ┌──────────┐                     │
 │  │  CPU 0   │  │  CPU 1   │                     │
 │  │  CPU 2   │  │  CPU 3   │                     │
 │  └────┬─────┘  └────┬─────┘                     │
 │       │              │                           │
 │       └──────┬───────┘                           │
 │              │ (로컬 메모리)                      │
 │       ┌──────┴───────┐                           │
 │       │   DRAM       │                           │
 │       └──────┬───────┘                           │
 │              │                                   │
 │       ┌──────┴───────┐                           │
 │       │  PCIe Root   │                           │
 │       │  Complex 0   │                           │
 │       └──────┬───────┘                           │
 │              │                                   │
 │       ┌──────┴───────┐                           │
 │       │  NVMe SSD    │  ← 이 SSD의 IRQ는        │
 │       │  (/dev/nvme0)│    CPU 0~3에 바인딩       │
 │       └──────────────┘    해야 최적!              │
 └─────────────────────────────────────────────────┘

 ┌─────────────────────────────────────────────────┐
 │  NUMA Node 1                                    │
 │  ┌──────────┐  ┌──────────┐                     │
 │  │  CPU 4   │  │  CPU 5   │                     │
 │  │  CPU 6   │  │  CPU 7   │                     │
 │  └──────────┘  └──────────┘                     │
 │       │                                          │
 │  다른 PCIe Root Complex → 다른 NVMe SSD          │
 └─────────────────────────────────────────────────┘
```

**원격 NUMA 노드 접근의 페널티:**

CPU 4(Node 1)이 Node 0의 NVMe SSD에 I/O를 수행하면:
1. DMA 버퍼가 Node 1의 메모리에 있으면 → PCIe TLP가 Node 0 → 인터커넥트 → Node 1으로 이동 (추가 레이턴시)
2. 인터럽트가 CPU 4에 도착해도, CQ 버퍼가 Node 0에 있으면 원격 메모리 접근 발생
3. 일반적으로 10~30%의 레이턴시 증가

**NUMA 노드 확인:**

```bash
# NVMe 디바이스의 NUMA 노드 확인
cat /sys/class/nvme/nvme0/device/numa_node

# 해당 노드의 CPU 목록
cat /sys/devices/system/node/node0/cpulist
```

### 7.3 Queue-CPU 매핑

이상적인 구성에서는 각 I/O 큐가 정확히 하나의 CPU에 매핑되어, 해당 CPU에서만 SQ에 커맨드를 제출하고 CQ의 완료를 처리한다.

```
 이상적인 Queue-CPU 1:1 매핑 (4 CPU 시스템)
 ============================================

 CPU 0 ──── I/O Queue 1 ──── MSI-X Vector 1 ──── CPU 0
 CPU 1 ──── I/O Queue 2 ──── MSI-X Vector 2 ──── CPU 1
 CPU 2 ──── I/O Queue 3 ──── MSI-X Vector 3 ──── CPU 2
 CPU 3 ──── I/O Queue 4 ──── MSI-X Vector 4 ──── CPU 3

 Admin Queue ── MSI-X Vector 0 ── CPU 0 (또는 임의 CPU)

 이 매핑이 Lock-Free I/O를 가능하게 한다:
 - CPU 0은 Queue 1의 SQ만 쓴다 → SQ에 lock 불필요
 - Queue 1의 CQ 인터럽트는 CPU 0에만 도착 → CQ에 lock 불필요
 - 서로 다른 CPU가 서로 다른 큐를 사용 → 간섭 없음
```

**큐 수가 CPU 수보다 적을 때:**

MSI-X 벡터 수가 CPU 수보다 적으면, 하나의 큐에 여러 CPU가 매핑된다. 이 경우 SQ 접근에 lock이 필요할 수 있다.

```
 벡터 부족 시 매핑 (8 CPU, 4 벡터)
 ==================================

 CPU 0, 4 ──── I/O Queue 1 ──── MSI-X Vector 1 ──── CPU 0
 CPU 1, 5 ──── I/O Queue 2 ──── MSI-X Vector 2 ──── CPU 1
 CPU 2, 6 ──── I/O Queue 3 ──── MSI-X Vector 3 ──── CPU 2
 CPU 3, 7 ──── I/O Queue 4 ──── MSI-X Vector 4 ──── CPU 3

 → CPU 0과 CPU 4가 같은 SQ를 공유 → sq_lock 필요
 → 인터럽트는 CPU 0에만 도착 → CQ 처리는 CPU 0에서만
```

---

## 8. 인터럽트 Coalescing

### 8.1 NVMe 인터럽트 병합

NVMe 스펙은 Set Features 커맨드(Feature ID: 0x08, `NVME_FEAT_IRQ_COALESCE`)를 통해 인터럽트 병합을 지원한다. 여러 완료를 모아서 하나의 인터럽트로 통합하면 CPU 오버헤드를 줄일 수 있다.

```
 인터럽트 Coalescing 동작
 ========================

 Coalescing OFF (기본):
 CQE 1  CQE 2  CQE 3  CQE 4
   ↓      ↓      ↓      ↓
  IRQ    IRQ    IRQ    IRQ     ← 4번의 인터럽트

 Coalescing ON (Threshold=4, Time=100μs):
 CQE 1  CQE 2  CQE 3  CQE 4
   │      │      │      ↓
   │      │      │     IRQ     ← 1번의 인터럽트 (4개 CQE 누적)
   └──────┴──────┴──────┘

 또는 Time 만료 시:
 CQE 1  CQE 2   (100μs 경과)
   │      │          ↓
   │      │         IRQ        ← Time 만료로 인터럽트 (2개 CQE만)
   └──────┴──────────┘
```

**Set Features 파라미터:**

```
 NVME_FEAT_IRQ_COALESCE (Feature ID: 0x08)
 ==========================================

 CDW11 (command dword 11):
 ┌────────────────────────────────────────┐
 │ Bits 15:8  │ Aggregation Time (TIME)   │
 │            │ 단위: 100μs               │
 │            │ 0 = 즉시 인터럽트          │
 │            │ 예: 10 = 1ms              │
 ├────────────┼────────────────────────────┤
 │ Bits 7:0   │ Aggregation Threshold (THR)│
 │            │ CQE 수 - 1                │
 │            │ 0 = CQE 1개마다 인터럽트   │
 │            │ 예: 3 = CQE 4개 모이면     │
 └────────────┴────────────────────────────┘

 인터럽트 발생 조건 (OR):
 - CQE 수가 (THR + 1)에 도달
 - Aggregation Time이 TIME × 100μs를 초과
 둘 중 하나라도 만족하면 인터럽트 발생
```

**리눅스 커널에서의 인터럽트 Coalescing:**

현재 리눅스 NVMe 드라이버는 인터럽트 coalescing을 자동으로 설정하지 않는다. 필요하면 `nvme-cli` 도구를 사용하여 수동으로 설정할 수 있다:

```bash
# 인터럽트 coalescing 설정
nvme set-feature /dev/nvme0 -f 0x08 -v 0x0A03
# TIME=10 (1ms), THR=3 (4개 CQE마다)

# 특정 인터럽트 벡터에 대해 coalescing 비활성화
nvme set-feature /dev/nvme0 -f 0x09 -v <vector>
# Feature 0x09: Interrupt Vector Configuration
# IV(벡터 번호) + CD(Coalescing Disable) 비트
```

### 8.2 Coalescing의 트레이드오프

```
 인터럽트 빈도 vs 레이턴시 트레이드오프
 =======================================

 Coalescing 강함 (THR 높음, TIME 길음):
 ┌────────────────────────────────┐
 │ + 인터럽트 수 감소 → CPU 절약  │
 │ + 처리량(throughput) 향상 가능  │
 │ - 개별 I/O 레이턴시 증가        │
 │ - tail latency 악화            │
 │ 적합: 대량 순차 I/O, 백업      │
 └────────────────────────────────┘

 Coalescing 약함/없음 (기본):
 ┌────────────────────────────────┐
 │ + 최소 레이턴시                 │
 │ + 일관된 응답 시간              │
 │ - 인터럽트 수 많음 → CPU 사용↑  │
 │ 적합: 데이터베이스, OLTP        │
 └────────────────────────────────┘
```

---

## 9. 성능 영향 비교

### 9.1 3가지 모드 비교표

| 항목 | 인터럽트 모드 | 폴링 모드 | 하이브리드 폴링 |
|------|-------------|----------|---------------|
| **레이턴시** | 중간 (~10-15μs) | 최저 (~7-10μs) | 낮음 (~8-12μs) |
| **CPU 사용률** | 낮음 (I/O 없으면 idle) | 최대 (100% busy-wait) | 중간 (필요 시만 폴링) |
| **처리량 (IOPS)** | 높음 | 최고 (인터럽트 오버헤드 없음) | 높음 |
| **전력 소비** | 낮음 (C-state 가능) | 최대 (C-state 불가) | 중간 |
| **CPU 확장성** | 좋음 | 제한적 (CPU 전용) | 좋음 |
| **적합한 QD** | QD 전체 | 높은 QD (QD≥16) | 낮은 QD (QD 1~8) |
| **적합한 워크로드** | 범용, 혼합 워크로드 | 초저지연 요구, 전용 스토리지 서버 | 지연 민감 + CPU 절약 |
| **커널 설정** | 기본 | `nvme.poll_queues=N` | `io_poll_delay=0` |
| **유저 인터페이스** | read/write, io_uring | io_uring + IOPOLL | io_uring + IOPOLL |

### 9.2 워크로드별 권장 모드

```
 워크로드별 최적 완료 모드 선택 가이드
 =====================================

 ┌─────────────────────┬────────────────────┬─────────────────┐
 │ 워크로드             │ 권장 모드          │ 이유            │
 ├─────────────────────┼────────────────────┼─────────────────┤
 │ 데이터베이스 (OLTP)  │ 인터럽트 또는      │ QD 낮음, 혼합   │
 │                     │ 하이브리드          │ 워크로드        │
 ├─────────────────────┼────────────────────┼─────────────────┤
 │ 키-밸류 스토어      │ 폴링               │ 초저지연 필수,  │
 │ (Redis, RocksDB)    │                    │ 전용 CPU 가능   │
 ├─────────────────────┼────────────────────┼─────────────────┤
 │ 로그 수집/분석      │ 인터럽트 +          │ 순차 I/O,       │
 │                     │ coalescing          │ CPU 절약 중요   │
 ├─────────────────────┼────────────────────┼─────────────────┤
 │ 가상화 (VM I/O)     │ 인터럽트            │ 여러 VM이       │
 │                     │                    │ CPU 공유        │
 ├─────────────────────┼────────────────────┼─────────────────┤
 │ AI/ML 학습 데이터   │ 폴링 또는           │ 데이터 로딩     │
 │ 파이프라인          │ 인터럽트            │ 병목 제거 필요  │
 ├─────────────────────┼────────────────────┼─────────────────┤
 │ 범용 서버           │ 인터럽트 (기본)     │ 안정성, 효율성  │
 └─────────────────────┴────────────────────┴─────────────────┘
```

### 9.3 레이턴시 분해

4K 랜덤 읽기 기준으로 각 단계의 레이턴시를 대략적으로 분해하면:

```
 인터럽트 모드 레이턴시 분해 (~12μs 총)
 =======================================

 ┌──────────────────────────────────────────────────────┐
 │ 단계                             │ 예상 시간         │
 ├──────────────────────────────────┼───────────────────┤
 │ ① SQ Doorbell MMIO Write        │ ~0.5μs            │
 │ ② PCIe TLP 전달 (호스트→SSD)     │ ~0.3μs            │
 │ ③ SSD 내부 처리 (NAND 읽기 등)   │ ~8-10μs           │
 │ ④ CQE DMA Write (SSD→호스트)     │ ~0.3μs            │
 │ ⑤ MSI-X 인터럽트 전달            │ ~0.1μs            │
 │ ⑥ IDT 조회 + 컨텍스트 저장       │ ~0.3μs            │
 │ ⑦ nvme_irq() CQ 처리            │ ~0.2μs            │
 │ ⑧ softirq/완료 처리              │ ~0.3μs            │
 │ ⑨ bio_endio + wake_up           │ ~0.2μs            │
 ├──────────────────────────────────┼───────────────────┤
 │ 총 인터럽트 오버헤드 (⑤~⑨)       │ ~1.1μs            │
 └──────────────────────────────────┴───────────────────┘

 폴링 모드 레이턴시 분해 (~9μs 총)
 ==================================

 ┌──────────────────────────────────────────────────────┐
 │ 단계                             │ 예상 시간         │
 ├──────────────────────────────────┼───────────────────┤
 │ ①~④ 동일                         │ ~9.1μs            │
 │ ⑤ 폴링으로 CQE 감지              │ ~0.05μs           │
 │ ⑥ nvme_poll() CQ 처리            │ ~0.2μs            │
 │ ⑦ 완료 처리                      │ ~0.2μs            │
 ├──────────────────────────────────┼───────────────────┤
 │ 총 완료 오버헤드 (⑤~⑦)            │ ~0.45μs           │
 │ 인터럽트 대비 절약                │ ~0.65μs           │
 └──────────────────────────────────┴───────────────────┘
```

### 9.4 fio를 사용한 벤치마크 방법

```bash
# 인터럽트 모드 (기본)
fio --name=irq_test --ioengine=io_uring \
    --filename=/dev/nvme0n1 --direct=1 \
    --rw=randread --bs=4k --iodepth=1 \
    --numjobs=1 --time_based --runtime=30

# 폴링 모드
# 먼저: modprobe nvme poll_queues=4
fio --name=poll_test --ioengine=io_uring \
    --filename=/dev/nvme0n1 --direct=1 \
    --rw=randread --bs=4k --iodepth=1 \
    --numjobs=1 --time_based --runtime=30 \
    --hipri=1  # io_uring IOPOLL 활성화

# 주요 비교 지표:
# - slat (submission latency)
# - clat (completion latency)  ← 이것이 핵심
# - lat (total latency)
# - iops
# - cpu usr/sys
```

---

## 10. 핵심 정리

### 10.1 인터럽트 → 완료의 전체 시퀀스

```
 시간 →
 ─────────────────────────────────────────────────────────────────

 NVMe SSD:  ┌──────┐  ┌──────┐  ┌───────┐
            │ 처리 │  │ CQE  │  │ MSI-X │
            │      │  │ DMA  │  │ TLP   │
            └──────┘  └──────┘  └───┬───┘
                                    │
 CPU:                               │  ┌─────────────────────┐
                                    └─>│ nvme_irq()          │
                                       │ ├─ nvme_poll_cq()   │
                                       │ │  ├─ Phase 확인    │
                                       │ │  ├─ dma_rmb()     │
                                       │ │  ├─ handle_cqe()  │
                                       │ │  └─ CQ Doorbell   │
                                       │ └─ complete_batch() │
                                       └──────────┬──────────┘
                                                  │
                                       ┌──────────┴──────────┐
                                       │ complete_rq()       │
                                       │ ├─ DMA unmap        │
                                       │ ├─ nvme_complete_rq │
                                       │ │  └─ end_req       │
                                       │ │     └─ end_request│
                                       │ │        └─ bio_endio
                                       │ └─ free_request     │
                                       └─────────────────────┘
```

### 10.2 커널 vs SPDK 완료 처리 비교

| 측면 | 리눅스 커널 NVMe | SPDK |
|------|-----------------|------|
| 인터럽트 | MSI-X 기반 (기본) | 없음 (폴링 전용) |
| CQ 처리 | `nvme_poll_cq()` | `spdk_nvme_qpair_process_completions()` |
| 완료 전달 | blk-mq → bio_endio | 콜백 직접 호출 |
| CPU 사용 | I/O 없으면 idle | 항상 100% (reactor) |
| 레이어 수 | 많음 (VFS → blk → NVMe) | 최소 (직접 접근) |
| 레이턴시 | ~10-15μs (인터럽트) | ~7-9μs |
| 레이턴시 | ~7-10μs (폴링) | ~7-9μs |

### 10.3 주요 소스 파일 참조

| 파일 | 역할 |
|------|------|
| `drivers/nvme/host/pci.c` | NVMe PCIe 드라이버: `nvme_irq()`, `nvme_poll()`, `nvme_setup_irqs()` |
| `drivers/nvme/host/core.c` | NVMe core: `nvme_complete_rq()`, `nvme_end_req()` |
| `drivers/nvme/host/nvme.h` | `nvme_try_complete_req()` 인라인 함수 |
| `block/blk-mq.c` | blk-mq: `blk_mq_complete_request()`, `blk_done_softirq()` |
| `kernel/softirq.c` | softirq 디스패치: `handle_softirqs()`, `raise_softirq()` |
| `kernel/irq/manage.c` | IRQ 관리: `request_threaded_irq()`, `free_irq()` |
| `drivers/pci/irq.c` | PCI IRQ: `pci_request_irq()` |
| `drivers/pci/msi/api.c` | MSI-X 할당: `pci_alloc_irq_vectors_affinity()` |
| `include/linux/interrupt.h` | IRQF 플래그, softirq 번호 정의 |
| `io_uring/io_uring.c` | io_uring 폴링: `io_iopoll_check()` |
