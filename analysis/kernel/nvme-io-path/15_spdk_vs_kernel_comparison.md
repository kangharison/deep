# 15. SPDK vs 커널 NVMe 드라이버 심층 비교

## 1. 아키텍처 비교 개요

커널 NVMe 드라이버와 SPDK NVMe 드라이버는 같은 NVMe 스펙에 기반하지만, I/O 경로의 설계 철학이 근본적으로 다르다.
커널 드라이버는 범용 운영체제의 일부로서 보호, 추상화, 공유를 중시하고, SPDK는 극한의 성능을 위해 이 모든 것을 제거한다.

### 1.1 커널 NVMe I/O 경로

```
Application (fio, dd, ...)
    │
    ▼
┌──────────────┐
│  syscall     │  read()/write()/io_uring_enter()
│  (커널 진입)  │  ← context switch 발생
└──────┬───────┘
       ▼
┌──────────────┐
│     VFS      │  파일시스템 추상화 (ext4, xfs, ...)
│              │  페이지 캐시 확인, inode 탐색
└──────┬───────┘
       ▼
┌──────────────┐
│  Block Layer │  bio → request 변환
│  (blk-mq)    │  I/O 스케줄러 (mq-deadline, kyber, none)
│              │  request merge, 태그 할당
└──────┬───────┘
       ▼
┌──────────────┐
│  NVMe Driver │  nvme_queue_rq()
│  (pci.c)     │  DMA 매핑 (PRP/SGL 구성)
│              │  SQ에 커맨드 복사
└──────┬───────┘
       ▼
┌──────────────┐
│  Doorbell    │  writel(sq_tail, doorbell_addr)
│  Write       │  MMIO를 통해 디바이스에 알림
└──────────────┘
```

### 1.2 SPDK NVMe I/O 경로

```
Application (SPDK app, fio + spdk ioengine)
    │
    ▼
┌──────────────┐
│  SPDK API    │  spdk_nvme_ns_cmd_read()
│  (nvme_ns_   │  NVMe 커맨드 빌드 (직접)
│   cmd.c)     │  syscall 0개!
└──────┬───────┘
       ▼
┌──────────────┐
│  SPDK PCIe   │  nvme_pcie_qpair_submit_request()
│  Transport   │  tracker 할당, PRP/SGL 설정
│              │  SQ에 커맨드 복사 (SSE2 최적화)
└──────┬───────┘
       ▼
┌──────────────┐
│  Doorbell    │  spdk_mmio_write_4(sq_tdbl, sq_tail)
│  Write       │  유저스페이스에서 직접 MMIO!
└──────────────┘
```

### 1.3 두 경로 비교 (나란히)

```
커널 경로                              SPDK 경로
═══════════════════                    ═══════════════════
Application                            Application
    │                                      │
    ▼                                      │
syscall (context switch!)                  │  ← 없음!
    │                                      │
    ▼                                      │
VFS (파일시스템)                            │  ← 없음!
    │                                      │
    ▼                                      │
Block Layer (blk-mq)                       │  ← 없음!
  - I/O 스케줄러                           │
  - request merge                          │
  - 태그 할당                              │
    │                                      │
    ▼                                      ▼
NVMe Driver                            SPDK NVMe
  - nvme_queue_rq()                      - spdk_nvme_ns_cmd_read()
  - DMA 매핑                             - PRP/SGL 설정
  - spin_lock 필요                       - lock-free!
    │                                      │
    ▼                                      ▼
SQ에 커맨드 복사                        SQ에 커맨드 복사
    │                                      │
    ▼                                      ▼
Doorbell Write                          Doorbell Write
(ioremap된 MMIO)                        (mmap된 MMIO)
    │                                      │
    ▼                                      ▼
 ┌─ 완료 대기 ─┐                       ┌─ 완료 대기 ─┐
 │ MSI-X 인터럽트│                       │ Polling     │
 │ → hardirq   │                       │ (busy-wait) │
 │ → softirq   │                       │ CPU 점유!   │
 │ → bio_endio │                       │ → 콜백 호출 │
 └─────────────┘                       └─────────────┘

함수 호출 깊이: ~20개                   함수 호출 깊이: ~5개
context switch: 1회 이상                context switch: 0회
lock 획득: 여러 개                      lock 획득: 0개
인터럽트: MSI-X 1회                     인터럽트: 0개
```


## 2. SPDK가 커널을 우회하는 원리

SPDK의 핵심 아이디어는 간단하다: NVMe 디바이스를 커널에서 빼내서 유저스페이스에서 직접 제어한다.
이를 위해 세 가지 기술이 필요하다: (1) 디바이스 바인딩 해제, (2) BAR MMIO 접근, (3) DMA 메모리 관리.

### 2.1 UIO / VFIO를 통한 디바이스 분리

NVMe 디바이스를 SPDK에서 사용하려면 먼저 커널 NVMe 드라이버에서 분리(unbind)해야 한다.

```
# 1단계: 커널 드라이버에서 언바인드
echo "0000:04:00.0" > /sys/bus/pci/drivers/nvme/unbind

# 2단계: VFIO 또는 UIO 드라이버에 바인드
# SPDK의 setup.sh가 이 과정을 자동화한다
```

**VFIO (Virtual Function I/O)**:
- IOMMU를 활용하여 안전한 유저스페이스 DMA를 제공한다.
- IOMMU가 DMA 주소를 변환하므로, 유저스페이스 프로세스가 다른 메모리 영역에 접근할 수 없다.
- `/dev/vfio/` 디바이스 파일을 통해 접근한다.
- 프로덕션 환경에서 권장되는 방식이다.

**UIO (Userspace I/O)**:
- IOMMU 없이 디바이스에 직접 접근한다.
- 모든 물리 메모리에 DMA 가능하므로 보안상 위험하다.
- IOMMU가 없는 환경에서만 사용한다.
- `uio_pci_generic` 커널 모듈을 사용한다.

SPDK의 `lib/env_dpdk/pci.c`에서 DPDK의 PCI 드라이버 프레임워크를 통해 디바이스를 관리한다:

```c
// SPDK: lib/env_dpdk/pci.c
// DPDK의 rte_pci를 통해 PCI 디바이스를 열거하고 바인딩한다
static int
map_bar_rte(struct spdk_pci_device *device, uint32_t bar,
            void **mapped_addr, uint64_t *phys_addr, uint64_t *size)
{
    struct rte_mem_resource *res;
    // DPDK가 이미 mmap한 BAR 리소스를 가져온다
    res = dpdk_pci_device_get_mem_resource(device->dev_handle, bar);
    *mapped_addr = res->addr;      // 유저스페이스 가상 주소
    *phys_addr = (uint64_t)res->phys_addr;  // 물리 주소
    *size = (uint64_t)res->len;
    return 0;
}
```

### 2.2 BAR MMIO를 유저스페이스에서 접근

NVMe 컨트롤러의 레지스터는 PCIe BAR0에 매핑된다.
커널에서는 `ioremap()`으로, SPDK에서는 `mmap()`으로 이 영역에 접근한다.

**커널 방식** (drivers/nvme/host/pci.c):
```c
// 커널: nvme_dev_map() 내부에서 호출
// ioremap()을 통해 BAR0를 커널 가상 주소 공간에 매핑
dev->bar = ioremap(pci_resource_start(pdev, 0),
                   pci_resource_len(pdev, 0));
// 이후 writel()로 Doorbell 쓰기
writel(nvmeq->sq_tail, nvmeq->q_db);
```

**SPDK 방식** (lib/nvme/nvme_pcie.c):
```c
// SPDK: nvme_pcie_ctrlr_allocate_bars()
static int
nvme_pcie_ctrlr_allocate_bars(struct nvme_pcie_ctrlr *pctrlr)
{
    void *addr = NULL;
    uint64_t phys_addr = 0, size = 0;

    // spdk_pci_device_map_bar()는 내부적으로 VFIO/UIO의 mmap()을 호출
    // → BAR0가 유저스페이스 가상 주소에 매핑됨
    rc = spdk_pci_device_map_bar(pctrlr->devhandle, 0, &addr,
                                 &phys_addr, &size);

    // NVMe 레지스터에 직접 접근 가능!
    pctrlr->regs = (volatile struct spdk_nvme_registers *)addr;
    pctrlr->regs_size = size;

    // Doorbell 베이스 주소 계산
    pctrlr->doorbell_base = (volatile uint32_t *)&pctrlr->regs->doorbell[0].sq_tdbl;

    return 0;
}
```

**유저스페이스에서 Doorbell 쓰기** (lib/nvme/nvme_pcie_internal.h):
```c
// SPDK: nvme_pcie_qpair_ring_sq_doorbell()
static inline void
nvme_pcie_qpair_ring_sq_doorbell(struct spdk_nvme_qpair *qpair)
{
    struct nvme_pcie_qpair *pqpair = nvme_pcie_qpair(qpair);
    struct nvme_pcie_ctrlr *pctrlr = nvme_pcie_ctrlr(qpair->ctrlr);

    // spdk_mmio_write_4()로 유저스페이스에서 직접 MMIO 쓰기!
    // 커널의 writel()과 동일한 효과
    spdk_wmb();
    g_thread_mmio_ctrlr = pctrlr;
    spdk_mmio_write_4(pqpair->sq_tdbl, pqpair->sq_tail);
    g_thread_mmio_ctrlr = NULL;
}
```

핵심 차이:
- 커널: `ioremap()` + `writel()` → 커널 코드에서만 실행 가능
- SPDK: `mmap()` + `spdk_mmio_write_4()` → 유저스페이스에서 직접 실행, syscall 불필요

### 2.3 DMA 메모리 관리

NVMe I/O에서 데이터 버퍼는 DMA로 디바이스에 전달된다.
커널과 SPDK는 DMA 메모리를 완전히 다른 방식으로 관리한다.

**커널 방식**:
```c
// 커널이 관리하는 DMA 매핑
// 1. 페이지 캐시의 페이지를 DMA 매핑
dma_addr = dma_map_page(dev, page, offset, len, DMA_TO_DEVICE);

// 2. 또는 coherent DMA 버퍼 할당
buf = dma_alloc_coherent(dev, size, &dma_addr, GFP_KERNEL);

// 커널의 DMA 서브시스템이 IOMMU 매핑을 자동으로 관리
// 요청마다 dma_map/dma_unmap 호출 필요 (오버헤드)
```

**SPDK 방식**:
```c
// SPDK: hugepage 기반 메모리 할당
// 1. 초기화 시 hugepage를 할당하고 IOMMU에 등록
buf = spdk_dma_zmalloc(size, alignment, NULL);
// 내부: rte_malloc() → hugepage에서 할당
// hugepage는 SPDK 초기화 시 한 번만 IOMMU에 등록됨

// 2. 가상주소 → 물리주소 변환 (매우 빠름)
phys_addr = spdk_vtophys(buf, &size);
// hugepage 기반이므로 단순 테이블 룩업으로 변환
```

```c
// SPDK: lib/nvme/nvme_pcie_common.c
static inline uint64_t
nvme_pcie_vtophys(struct spdk_nvme_ctrlr *ctrlr, const void *buf, uint64_t *size)
{
    if (spdk_likely(ctrlr->trid.trtype == SPDK_NVME_TRANSPORT_PCIE)) {
        // spdk_vtophys(): hugepage 테이블에서 물리 주소를 찾는다
        // 4KB 페이지 기반 커널과 달리, 2MB hugepage이므로
        // 변환 테이블이 매우 작고 캐시 친화적이다
        return spdk_vtophys(buf, size);
    }
    return (uint64_t)(uintptr_t)buf;  // VFIO-user: IOVA=VA 모드
}
```

**SPDK의 VFIO DMA 등록** (lib/env_dpdk/memory.c):
```c
// VFIO IOMMU에 DMA 영역 등록
struct spdk_vfio_dma_map {
    struct vfio_iommu_type1_dma_map map;
    TAILQ_ENTRY(spdk_vfio_dma_map) tailq;
};
// hugepage 할당 시 한 번만 IOMMU에 등록하면
// 이후 I/O에서는 추가 매핑/해제 없이 DMA 가능
```

핵심 차이:
| 항목 | 커널 | SPDK |
|------|------|------|
| 메모리 할당 | `dma_alloc_coherent()` / 페이지 캐시 | `spdk_dma_zmalloc()` (hugepage) |
| DMA 매핑 | 매 I/O마다 `dma_map`/`dma_unmap` | 초기화 시 한 번만 등록 |
| 주소 변환 | IOMMU 페이지 테이블 (4KB 단위) | hugepage 테이블 (2MB 단위) |
| TLB 효율 | 4KB 페이지 → TLB miss 잦음 | 2MB hugepage → TLB miss 극소 |


## 3. 코드 레벨 비교: I/O 제출

### 3.1 커널 경로 (Review)

fio에서 4KB 읽기를 시작하면:

```
fio                                     ← 유저스페이스
 └→ read() / io_uring_enter()
     └→ [syscall] ─────────────────────── ← 커널 진입 (context switch)
         └→ VFS: vfs_read()
             └→ ext4_file_read_iter() (또는 direct I/O)
                 └→ submit_bio()
                     └→ blk_mq_submit_bio()
                         └→ blk_mq_sched_dispatch_requests()
                             └→ blk_mq_dispatch_rq_list()
                                 └→ nvme_queue_rq()      ← NVMe 드라이버 진입
                                     ├→ nvme_prep_rq()    (DMA 매핑)
                                     ├→ spin_lock()       ← lock 획득!
                                     ├→ nvme_sq_copy_cmd() (SQ에 복사)
                                     ├→ nvme_write_sq_db() (Doorbell)
                                     └→ spin_unlock()

함수 호출 체인 깊이: ~20단계
```

커널 `nvme_queue_rq()` (drivers/nvme/host/pci.c:1675):
```c
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
                                  const struct blk_mq_queue_data *bd)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    struct request *req = bd->rq;
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);

    // 1. 큐 상태 확인
    if (unlikely(!test_bit(NVMEQ_ENABLED, &nvmeq->flags)))
        return BLK_STS_IOERR;

    // 2. 컨트롤러 상태 확인
    if (unlikely(!nvme_check_ready(&dev->ctrl, req, true)))
        return nvme_fail_nonready_command(&dev->ctrl, req);

    // 3. NVMe 커맨드 빌드 + DMA 매핑 (PRP/SGL)
    ret = nvme_prep_rq(req);  // 내부에서 dma_map_sg() 호출

    // 4. SQ에 커맨드 복사 + Doorbell (lock 보호)
    spin_lock(&nvmeq->sq_lock);           // ← lock 필요!
    nvme_sq_copy_cmd(nvmeq, &iod->cmd);   // SQ Tail에 64바이트 복사
    nvme_write_sq_db(nvmeq, bd->last);    // Doorbell 쓰기
    spin_unlock(&nvmeq->sq_lock);
    return BLK_STS_OK;
}
```

### 3.2 SPDK 경로

SPDK 애플리케이션에서 4KB 읽기를 시작하면:

```
SPDK Application                         ← 유저스페이스 (항상!)
 └→ spdk_nvme_ns_cmd_read()              (nvme_ns_cmd.c)
     └→ _nvme_ns_cmd_rw()                (NVMe 커맨드 구성)
         └→ nvme_qpair_submit_request()  (nvme_qpair.c)
             └→ nvme_pcie_qpair_submit_request()  (nvme_pcie_common.c)
                 ├→ TAILQ_FIRST(&free_tr)          (tracker 할당, lock 없음!)
                 ├→ PRP/SGL 구성                   (vtophys 변환)
                 └→ nvme_pcie_qpair_submit_tracker()
                     ├→ nvme_pcie_copy_command()   (SSE2 최적화 복사)
                     └→ nvme_pcie_qpair_ring_sq_doorbell()
                         └→ spdk_mmio_write_4()    (Doorbell 쓰기)

함수 호출 체인 깊이: ~5단계
syscall: 0개
context switch: 0회
lock: 0개 (I/O qpair는 단일 스레드 전용)
```

`spdk_nvme_ns_cmd_read()` (lib/nvme/nvme_ns_cmd.c:642):
```c
int
spdk_nvme_ns_cmd_read(struct spdk_nvme_ns *ns, struct spdk_nvme_qpair *qpair,
                      void *buffer, uint64_t lba, uint32_t lba_count,
                      spdk_nvme_cmd_cb cb_fn, void *cb_arg, uint32_t io_flags)
{
    struct nvme_request *req;
    struct nvme_payload payload;

    payload = NVME_PAYLOAD_CONTIG(buffer, NULL);

    // NVMe Read 커맨드를 직접 빌드
    req = _nvme_ns_cmd_rw(ns, qpair, &payload, 0, 0,
                          lba, lba_count, cb_fn, cb_arg,
                          SPDK_NVME_OPC_READ, io_flags, ...);

    // 트랜스포트 레이어에 제출 (PCIe의 경우 nvme_pcie_qpair_submit_request)
    return nvme_qpair_submit_request(qpair, req);
}
```

### 3.3 핵심 함수 비교 (코드 분석)

**커널 `nvme_queue_rq()` vs SPDK `nvme_pcie_qpair_submit_request()`**:

두 함수 모두 같은 일을 한다: NVMe SQ에 커맨드를 넣고 Doorbell을 울린다.
하지만 SPDK는 블록 레이어, 스케줄러, 인터럽트, lock이 모두 없다.

```c
// SPDK: nvme_pcie_qpair_submit_request() (nvme_pcie_common.c:1672)
int
nvme_pcie_qpair_submit_request(struct spdk_nvme_qpair *qpair,
                               struct nvme_request *req)
{
    struct nvme_tracker *tr;
    struct nvme_pcie_qpair *pqpair = nvme_pcie_qpair(qpair);

    // 1. free tracker 리스트에서 하나 가져옴 (TAILQ, lock 없음!)
    tr = TAILQ_FIRST(&pqpair->free_tr);
    if (tr == NULL) {
        return -EAGAIN;  // 큐 가득 참
    }

    TAILQ_REMOVE(&pqpair->free_tr, tr, tq_list);
    TAILQ_INSERT_TAIL(&pqpair->outstanding_tr, tr, tq_list);
    tr->req = req;
    req->cmd.cid = tr->cid;  // Command ID = tracker index

    // 2. PRP/SGL 구성 (DMA 매핑이 아닌 단순 vtophys 변환)
    if (req->payload_size != 0) {
        rc = g_nvme_pcie_build_req_table[payload_type][sgl_supported](
                qpair, req, tr, dword_aligned);
    }

    // 3. SQ에 커맨드 복사 + Doorbell
    nvme_pcie_qpair_submit_tracker(qpair, tr);

    return 0;
}
```

**커맨드 복사 최적화 비교**:

```c
// 커널: 일반적인 memcpy 기반 복사
static inline void nvme_sq_copy_cmd(struct nvme_queue *nvmeq,
                                     struct nvme_command *cmd)
{
    memcpy(nvmeq->sq_cmds + (nvmeq->sq_tail << nvmeq->sqe_shift),
           cmd, sizeof(*cmd));
    // 64바이트 NVMe 커맨드를 SQ Tail 위치에 복사
}

// SPDK: SSE2 스트리밍 스토어 최적화
static inline void
nvme_pcie_copy_command(struct spdk_nvme_cmd *dst,
                       const struct spdk_nvme_cmd *src)
{
#if defined(__SSE2__)
    __m128i *d128 = (__m128i *)dst;
    const __m128i *s128 = (const __m128i *)src;
    // 16바이트 × 4회 = 64바이트를 non-temporal store로 복사
    // CPU 캐시를 오염시키지 않고 직접 메모리에 쓴다
    _mm_stream_si128(&d128[0], _mm_load_si128(&s128[0]));
    _mm_stream_si128(&d128[1], _mm_load_si128(&s128[1]));
    _mm_stream_si128(&d128[2], _mm_load_si128(&s128[2]));
    _mm_stream_si128(&d128[3], _mm_load_si128(&s128[3]));
#else
    *dst = *src;
#endif
}
```

SPDK는 `_mm_stream_si128` (non-temporal store)를 사용하여 SQ 커맨드 복사 시 CPU 캐시를 오염시키지 않는다.
SQ 메모리는 쓰기 전용이므로, 캐시에 올릴 필요가 없기 때문이다.

**Lock 비교**:

```c
// 커널: spin_lock 필요
// 같은 큐에 여러 CPU가 동시에 커맨드를 제출할 수 있으므로
spin_lock(&nvmeq->sq_lock);
nvme_sq_copy_cmd(nvmeq, &iod->cmd);
nvme_write_sq_db(nvmeq, bd->last);
spin_unlock(&nvmeq->sq_lock);

// SPDK: lock 불필요
// qpair는 단일 스레드 전용으로 설계됨
// "The SPDK NVMe driver provides no synchronization for qpair accesses -
//  the application must ensure only a single thread submits I/O to a
//  qpair." (hello_world.c 주석)
nvme_pcie_qpair_submit_tracker(qpair, tr);  // lock 없이 직접 호출
```


## 4. 코드 레벨 비교: I/O 완료

### 4.1 커널 완료 경로

```
NVMe 디바이스가 CQE를 DMA로 CQ에 기록
    │
    ▼
MSI-X 인터럽트 발생
    │
    ▼
CPU 인터럽트 핸들러 진입 (hardirq)
    │
    ▼
nvme_irq() ─── drivers/nvme/host/pci.c:1995
    │
    ▼
nvme_poll_cq()
    ├→ nvme_cqe_pending(): phase bit 확인
    ├→ dma_rmb(): DMA 읽기 배리어
    ├→ nvme_handle_cqe(): CQE 처리
    │   └→ blk_mq_complete_request_remote()
    │       └→ nvme_pci_complete_rq()
    │           └→ nvme_unmap_data() (DMA unmap)
    │           └→ nvme_complete_rq()
    │               └→ blk_mq_end_request()
    │                   └→ bio_endio() → 유저스페이스에 완료 알림
    └→ nvme_ring_cq_doorbell(): CQ Head Doorbell 쓰기
```

커널 `nvme_irq()` (drivers/nvme/host/pci.c:1995):
```c
static irqreturn_t nvme_irq(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;
    DEFINE_IO_COMP_BATCH(iob);

    if (nvme_poll_cq(nvmeq, &iob)) {
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob);  // 배치 완료 처리
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}
```

### 4.2 SPDK 완료 경로

```
NVMe 디바이스가 CQE를 DMA로 CQ에 기록
    │
    ▼
(인터럽트 없음! 애플리케이션이 명시적으로 폴링)
    │
    ▼
spdk_nvme_qpair_process_completions()
    │
    ▼
nvme_pcie_qpair_process_completions() ─── nvme_pcie_common.c:865
    │
    ├→ while (1) {
    │      cpl = &pqpair->cpl[pqpair->cq_head];
    │      // phase bit 확인: 새 CQE인지 판단
    │      if (cpl->status.p != pqpair->flags.phase)
    │          break;  // 더 이상 새 CQE 없음
    │
    │      // 다음 CQE prefetch (성능 최적화)
    │      if (next_is_valid)
    │          __builtin_prefetch(&pqpair->tr[next_cpl->cid]);
    │
    │      // tracker 찾아서 완료 처리
    │      tr = &pqpair->tr[cpl->cid];
    │      nvme_pcie_qpair_complete_tracker(qpair, tr, cpl, true);
    │          └→ nvme_complete_request(tr->cb_fn, tr->cb_arg, ...)
    │              └→ 사용자 콜백 직접 호출!  ← bio_endio() 대신
    │  }
    │
    └→ nvme_pcie_qpair_ring_cq_doorbell()  // CQ Head Doorbell
```

SPDK `nvme_pcie_qpair_process_completions()` (nvme_pcie_common.c:865):
```c
int32_t
nvme_pcie_qpair_process_completions(struct spdk_nvme_qpair *qpair,
                                    uint32_t max_completions)
{
    struct nvme_pcie_qpair *pqpair = nvme_pcie_qpair(qpair);
    uint32_t num_completions = 0;

    while (1) {
        cpl = &pqpair->cpl[pqpair->cq_head];

        // Phase bit로 새 CQE 확인 (커널의 nvme_cqe_pending()과 동일 원리)
        if (cpl->status.p != pqpair->flags.phase)
            break;

        // 다음 CQE prefetch → L1 캐시에 미리 로드
        next_cpl = &pqpair->cpl[next_cq_head];
        next_is_valid = (next_cpl->status.p == next_phase);
        if (next_is_valid)
            __builtin_prefetch(&pqpair->tr[next_cpl->cid]);

        // CQ head 업데이트 + phase 토글
        if (++pqpair->cq_head == pqpair->num_entries) {
            pqpair->cq_head = 0;
            pqpair->flags.phase = !pqpair->flags.phase;
        }

        // Tracker에서 원래 요청 찾아 완료 콜백 호출
        tr = &pqpair->tr[cpl->cid];
        nvme_pcie_qpair_complete_tracker(qpair, tr, cpl, true);

        if (++num_completions == max_completions)
            break;
    }

    if (num_completions > 0)
        nvme_pcie_qpair_ring_cq_doorbell(qpair);  // CQ Doorbell

    // delay_cmd_submit 모드: 완료 폴링 시점에 SQ Doorbell도 같이 울림
    // → SQ Doorbell과 CQ Doorbell을 한 번에 처리하여 MMIO 횟수 절약
    if (pqpair->flags.delay_cmd_submit) {
        if (pqpair->last_sq_tail != pqpair->sq_tail) {
            nvme_pcie_qpair_ring_sq_doorbell(qpair);
            pqpair->last_sq_tail = pqpair->sq_tail;
        }
    }

    return num_completions;
}
```

### 4.3 완료 경로 비교 요약

```
                    커널                              SPDK
────────────────────────────────────────────────────────────────
트리거          MSI-X 인터럽트                     애플리케이션 polling
핸들러          nvme_irq() (hardirq)               process_completions()
CQE 확인        nvme_cqe_pending()                 phase bit 비교
메모리 배리어    dma_rmb()                          spdk_mb() (arch별)
DMA 해제        nvme_unmap_data() 필요             불필요 (hugepage 상주)
완료 알림       bio_endio() → softirq              콜백 직접 호출
context switch  있음 (hardirq→softirq→user)        없음 (같은 스레드)
인터럽트 횟수   CQE당 또는 coalescing 후 1회        0회
배치 처리       io_comp_batch                      max_completions 파라미터
```


## 5. SPDK 초기화 과정

### 5.1 SPDK 디바이스 발견 및 초기화

SPDK의 초기화는 `spdk_nvme_probe()`에서 시작한다.
이 함수는 PCIe 버스를 스캔하여 NVMe 디바이스를 찾고, 사용자가 등록한 콜백으로 디바이스를 초기화한다.

```
spdk_nvme_probe()                          ← 사용자 호출
    │
    ├→ nvme_pcie_ctrlr_scan()              ← PCIe 디바이스 열거
    │   └→ spdk_pci_enumerate()             (DPDK의 PCI 프레임워크 사용)
    │       └→ pcie_nvme_enum_cb()          (NVMe 클래스 디바이스 필터)
    │           └→ nvme_ctrlr_probe()       (probe 콜백 호출)
    │
    ├→ nvme_pcie_ctrlr_construct()         ← 컨트롤러 생성
    │   ├→ spdk_pci_device_claim()          (디바이스 독점)
    │   ├→ spdk_zmalloc()                   (hugepage에서 할당)
    │   ├→ nvme_pcie_ctrlr_allocate_bars()  (BAR0 mmap)
    │   │   └→ spdk_pci_device_map_bar()    (VFIO mmap)
    │   ├→ PCI busmaster enable             (cmd_reg |= 0x404)
    │   └→ nvme_pcie_ctrlr_construct_admin_qpair()
    │       └→ nvme_pcie_qpair_construct()  (Admin SQ/CQ 할당)
    │
    └→ spdk_nvme_ctrlr_process_init()      ← 컨트롤러 초기화
        ├→ Controller Enable (CC.EN = 1)
        ├→ Identify Controller
        ├→ Set Number of Queues
        └→ Identify Namespace
```

### 5.2 SPDK I/O 큐 생성

```c
// 사용자 코드 (hello_world.c)
// I/O qpair 할당: 내부적으로 Create I/O CQ + Create I/O SQ 커맨드를 보냄
ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ns_entry->ctrlr, NULL, 0);
```

내부 흐름:
```
spdk_nvme_ctrlr_alloc_io_qpair()
    │
    ├→ nvme_pcie_ctrlr_create_io_qpair()    (nvme_pcie_common.c:1073)
    │   ├→ spdk_zmalloc(sizeof(nvme_pcie_qpair))  (hugepage 할당)
    │   ├→ nvme_pcie_qpair_construct()
    │   │   ├→ SQ 메모리 할당 (spdk_zmalloc, DMA 가능한 hugepage)
    │   │   ├→ CQ 메모리 할당 (spdk_zmalloc)
    │   │   ├→ vtophys로 bus address 계산
    │   │   ├→ Doorbell 주소 계산:
    │   │   │   sq_tdbl = doorbell_base + (2*qid+0) * stride
    │   │   │   cq_hdbl = doorbell_base + (2*qid+1) * stride
    │   │   └→ Tracker 배열 할당 (4KB 정렬, PRP 리스트 내장)
    │   └→ return qpair
    │
    └→ _nvme_pcie_ctrlr_create_io_qpair()
        ├→ Create I/O CQ (Admin 커맨드)
        └→ Create I/O SQ (Admin 커맨드, CQ 생성 완료 후)
```

### 5.3 커널 vs SPDK 초기화 비교

```
                    커널 nvme_probe()              SPDK spdk_nvme_probe()
────────────────────────────────────────────────────────────────────────
디바이스 발견      PCI 서브시스템 자동              spdk_pci_enumerate()
BAR 매핑          ioremap()                       mmap() via VFIO/UIO
DMA 풀 준비       dma_pool_create()               hugepage 사전 할당
Admin 큐 생성     nvme_alloc_admin_tag_set()       nvme_pcie_ctrlr_construct_admin_qpair()
컨트롤러 Enable   nvme_pci_enable()                CC.EN = 1 직접 쓰기
Identify          nvme_init_ctrl_finish()          nvme_ctrlr_process_init()
I/O 큐 생성      nvme_setup_io_queues()           spdk_nvme_ctrlr_alloc_io_qpair()
블록 디바이스     /dev/nvme0n1 생성                없음 (bdev 레이어 별도)
```

핵심 차이: 커널은 `nvme_probe()` 완료 시 블록 디바이스(`/dev/nvme0n1`)가 생성되어 파일시스템 마운트가 가능하지만, SPDK는 블록 디바이스를 생성하지 않고 API를 통해서만 접근한다.


## 6. 성능 차이와 원인

### 6.1 대표적인 성능 수치

4KB Random Read 기준 (단일 NVMe SSD, 단일 CPU 코어):

```
                    커널 NVMe              SPDK
────────────────────────────────────────────────────
레이턴시 (avg)      ~10-15 μs              ~2-3 μs
IOPS (per core)     ~400-500K              ~1.5-2M
CPU 효율            I/O당 ~30K 사이클       I/O당 ~5K 사이클
```

### 6.2 레이턴시 분해

```
커널 경로 레이턴시 분해 (~12 μs 기준):
┌────────────────────────────────────┬──────────┐
│ 구간                              │ 소요시간  │
├────────────────────────────────────┼──────────┤
│ syscall + VFS 오버헤드             │ ~1-2 μs  │
│ Block Layer (blk-mq, 스케줄러)     │ ~1-2 μs  │
│ NVMe 드라이버 (DMA 매핑, lock)     │ ~1 μs    │
│ Doorbell → 디바이스 처리 → CQE     │ ~5-7 μs  │ ← 하드웨어 시간
│ 인터럽트 + softirq + bio_endio     │ ~2-3 μs  │
│ 유저스페이스 복귀                   │ ~0.5 μs  │
├────────────────────────────────────┼──────────┤
│ 합계                              │ ~10-15 μs │
└────────────────────────────────────┴──────────┘

SPDK 경로 레이턴시 분해 (~2.5 μs 기준):
┌────────────────────────────────────┬──────────┐
│ 구간                              │ 소요시간  │
├────────────────────────────────────┼──────────┤
│ SPDK API + PRP 구성               │ ~0.3 μs  │
│ SQ 커맨드 복사 + Doorbell          │ ~0.2 μs  │
│ Doorbell → 디바이스 처리 → CQE     │ ~1.5-2 μs│ ← 하드웨어 시간
│ Polling + 콜백 호출               │ ~0.2 μs  │
├────────────────────────────────────┼──────────┤
│ 합계                              │ ~2-3 μs  │
└────────────────────────────────────┴──────────┘
```

### 6.3 성능 차이 원인 상세 분석

**1. syscall 오버헤드 제거**:
- 커널: `read()`/`write()` → syscall entry → 커널 모드 전환 → KPTI 오버헤드
- SPDK: 함수 호출만으로 I/O 제출, syscall 0개
- 절감: I/O당 ~0.5-1 μs

**2. context switch 제거**:
- 커널: 유저→커널 전환 시 레지스터 저장/복원, 페이지 테이블 전환 (KPTI)
- SPDK: 같은 유저스페이스 스레드에서 전체 I/O 경로 실행
- 절감: I/O당 ~0.5-1 μs

**3. lock contention 제거**:
- 커널: `spin_lock(&nvmeq->sq_lock)` → 여러 CPU가 같은 큐에 접근 가능
- SPDK: qpair는 단일 스레드 전용, lock 불필요
- 절감: contention 시 수 μs, 평균 ~0.1-0.5 μs

**4. 인터럽트 오버헤드 제거 (Polling)**:
- 커널: MSI-X 인터럽트 → IDT 조회 → 핸들러 호출 → softirq 스케줄링
- SPDK: CQ를 직접 polling → phase bit 확인 → 즉시 콜백
- 절감: I/O당 ~1-3 μs (인터럽트 자체 + softirq 지연)

**5. Block Layer / 스케줄러 우회**:
- 커널: bio → request 변환, merge 시도, 스케줄러 결정, 태그 할당
- SPDK: 없음. 애플리케이션이 직접 NVMe 커맨드 빌드
- 절감: I/O당 ~1-2 μs

**6. hugepage → TLB miss 감소**:
- 커널: 4KB 페이지 → I/O 버퍼 접근 시 TLB miss 빈번
- SPDK: 2MB hugepage → 같은 메모리 범위를 더 적은 TLB 엔트리로 커버
- DMA 주소 변환도 hugepage 테이블에서 단순 룩업
- 절감: I/O당 ~0.1-0.5 μs (워크로드에 따라 크게 다름)

**7. DMA 매핑 오버헤드 제거**:
- 커널: 매 I/O마다 `dma_map_sg()` / `dma_unmap_sg()` 호출
- SPDK: hugepage를 초기화 시 한 번만 IOMMU에 등록, I/O 시에는 `spdk_vtophys()` 단순 룩업만
- 절감: I/O당 ~0.2-0.5 μs

### 6.4 IOPS 확장성

```
코어 수 증가에 따른 IOPS 확장성:

커널 NVMe:
1 core:  ~500K IOPS
4 cores: ~1.5M IOPS  (확장 효율 ~75%)  ← lock contention 영향
8 cores: ~2.5M IOPS  (확장 효율 ~63%)

SPDK:
1 core:  ~1.5M IOPS
4 cores: ~5.8M IOPS  (확장 효율 ~97%)  ← lock-free
8 cores: ~11M IOPS   (확장 효율 ~92%)  ← NVMe SSD 자체 한계에 수렴
```

SPDK의 확장성이 우수한 이유: 코어 간 공유 자원이 없다.
각 코어가 전용 qpair를 가지고, lock 없이 독립적으로 I/O를 처리한다.


## 7. 트레이드오프

| 특성 | 커널 NVMe | SPDK |
|------|-----------|------|
| **커널 보호** | 완전한 메모리 보호, fault isolation | 유저스페이스에서 하드웨어 직접 접근, 버그 시 시스템 크래시 위험 |
| **파일시스템 지원** | ext4, xfs, btrfs 등 모두 사용 가능 | 파일시스템 사용 불가 (blobstore 자체 구현 또는 raw block) |
| **멀티테넌시** | 여러 프로세스가 동시에 같은 디바이스 사용 가능 | 디바이스를 한 프로세스가 독점, 다른 프로세스 접근 불가 |
| **CPU 사용** | I/O 없으면 CPU idle (인터럽트 기반) | I/O 없어도 CPU 100% 사용 (polling 루프) |
| **개발 복잡도** | 표준 POSIX API (read/write) | SPDK 전용 API, 비동기 프로그래밍 모델 필수 |
| **디버깅** | 표준 커널 도구 (perf, ftrace, blktrace) | SPDK 자체 trace, gdb 사용 가능하나 생태계 작음 |
| **핫플러그** | 커널이 자동 처리 | 애플리케이션이 직접 처리해야 함 |
| **전력 효율** | 인터럽트 기반, CPU가 idle 가능 | 폴링으로 CPU가 항상 활성, 전력 소모 큼 |
| **레이턴시** | ~10-15 μs | ~2-3 μs |
| **IOPS/core** | ~400-500K | ~1.5-2M |
| **표준 도구 호환** | ls, dd, mkfs 등 모두 작동 | 표준 도구 사용 불가 |
| **NUMA 지원** | 커널이 자동 최적화 | 애플리케이션이 직접 NUMA 관리 |
| **에러 처리** | 커널의 robust한 에러 복구 | 애플리케이션 책임 |


## 8. SPDK 아키텍처 상세

### 8.1 Reactor 모델 (이벤트 루프)

SPDK의 핵심 실행 모델은 Reactor 패턴이다.
각 CPU 코어에서 무한 루프를 돌며 이벤트를 처리한다.

```
┌─────────────────────────────────────────────────┐
│                  SPDK Reactor                    │
│                (1 core 당 1개)                    │
│                                                  │
│   while (true) {                                 │
│       // 1. 등록된 poller 실행                     │
│       TAILQ_FOREACH(poller, &reactor->pollers) { │
│           poller->fn(poller->arg);               │
│       }                                          │
│                                                  │
│       // 2. 메시지 큐 확인                         │
│       spdk_ring_dequeue(reactor->events, ...);   │
│                                                  │
│       // 3. NVMe completion polling              │
│       spdk_nvme_qpair_process_completions(...);  │
│   }                                              │
│                                                  │
│   ← context switch 없음, sleep 없음              │
│   ← CPU 100% 점유                                │
└─────────────────────────────────────────────────┘
```

### 8.2 Thread 모델 (lcore 당 1 스레드)

```
┌──────────────────────────────────────────────────┐
│                                                   │
│   DPDK lcore 0        lcore 1        lcore 2     │
│   ┌──────────┐   ┌──────────┐   ┌──────────┐    │
│   │ Reactor  │   │ Reactor  │   │ Reactor  │    │
│   │          │   │          │   │          │    │
│   │ Thread 0 │   │ Thread 1 │   │ Thread 2 │    │
│   │          │   │          │   │          │    │
│   │ qpair 0  │   │ qpair 1  │   │ qpair 2  │    │
│   │ qpair 3  │   │ qpair 4  │   │ qpair 5  │    │
│   └──────────┘   └──────────┘   └──────────┘    │
│                                                   │
│   각 lcore는 pthread에 고정 (CPU affinity)         │
│   각 qpair는 하나의 lcore에만 소속                  │
│   → 코어 간 동기화 불필요                           │
│                                                   │
└──────────────────────────────────────────────────┘
```

DPDK의 lcore (logical core) 개념:
- 각 lcore는 하나의 pthread로 구현된다.
- `pthread_setaffinity_np()`로 특정 CPU에 고정(pinning)된다.
- 스케줄러에 의해 다른 CPU로 마이그레이션되지 않는다.
- 각 lcore에서 하나의 SPDK Reactor가 실행된다.

### 8.3 Bdev 레이어 (블록 디바이스 추상화)

SPDK는 커널의 Block Layer를 대체하는 자체 bdev 레이어를 제공한다.

```
┌─────────────────────────────────────────────┐
│               SPDK Application               │
│         (커스텀 앱, vhost, iSCSI 등)          │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│            SPDK Bdev Layer                   │
│     (커널의 Block Layer에 해당)               │
│                                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐    │
│  │ NVMe     │ │ Null     │ │ Malloc   │    │
│  │ bdev     │ │ bdev     │ │ bdev     │    │
│  │          │ │ (테스트) │ │ (RAM)    │    │
│  └────┬─────┘ └──────────┘ └──────────┘    │
│       │                                      │
│  ┌────┴─────────────────────┐               │
│  │ RAID, Crypto, QoS 등     │               │
│  │ (bdev module 스택)       │               │
│  └──────────────────────────┘               │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│         SPDK NVMe Driver                     │
│    spdk_nvme_ns_cmd_read/write()             │
│    nvme_pcie_qpair_submit_request()          │
└─────────────────────────────────────────────┘
```

### 8.4 NVMe bdev vs 커널 Block Device

```
커널 구조:                              SPDK 구조:
──────────                              ──────────
/dev/nvme0n1                            bdev "NVMe0n1"
    │                                       │
    ▼                                       ▼
struct gendisk                          struct spdk_bdev
    │                                       │
    ▼                                       ▼
struct request_queue                    struct spdk_bdev_channel
(blk-mq, I/O 스케줄러)                  (per-thread, lock-free)
    │                                       │
    ▼                                       ▼
struct nvme_ns                          struct spdk_nvme_ns
    │                                       │
    ▼                                       ▼
struct nvme_queue                       struct spdk_nvme_qpair
(NVMe SQ/CQ pair)                      (NVMe SQ/CQ pair)
    │                                       │
    ▼                                       ▼
PCIe BAR0 (ioremap)                     PCIe BAR0 (mmap)
```


## 9. 언제 무엇을 선택할지

### 9.1 커널 NVMe를 선택해야 할 때

**범용 서버**:
- 웹 서버, 데이터베이스 서버 등 일반적인 워크로드
- 파일시스템(ext4, xfs)이 필요한 경우
- 여러 애플리케이션이 같은 스토리지를 공유할 때
- 관리 편의성이 중요할 때 (표준 리눅스 도구 사용)

**파일시스템이 필요한 경우**:
- 로그 파일 저장
- 설정 파일 관리
- 전통적인 POSIX 호환 애플리케이션

**개발/테스트 환경**:
- 빠른 프로토타이핑
- 표준 도구로 디버깅 (perf, blktrace, iostat)
- 복잡한 SPDK 프로그래밍 모델이 불필요할 때

**전력 효율이 중요한 경우**:
- 인터럽트 기반이므로 I/O가 없으면 CPU가 idle 상태
- 노트북, 모바일, 저전력 서버

**커널의 io_uring + 폴링 모드 고려**:
- io_uring의 `IORING_SETUP_IOPOLL`을 사용하면 커널 내에서도 폴링 기반 I/O 가능
- SPDK 수준은 아니지만 인터럽트 오버헤드를 크게 줄일 수 있음
- 파일시스템과 함께 사용 가능하다는 장점

### 9.2 SPDK를 선택해야 할 때

**고성능 스토리지 시스템**:
- Ceph OSD (BlueStore + SPDK 백엔드)
- 분산 스토리지 시스템 (예: MinIO의 DirectIO 대안)
- NVMe-oF 타겟 (SPDK nvmf_tgt)
- 고성능 KV 스토어

**전용 스토리지 어플라이언스**:
- NVMe SSD를 전용으로 사용하는 장비
- 스토리지 전용 노드 (컴퓨트와 분리된 아키텍처)
- 네트워크 스토리지 게이트웨이

**극한의 레이턴시가 필요한 경우**:
- 고빈도 트레이딩 (HFT)
- 실시간 데이터 처리
- 마이크로초 단위 레이턴시가 비즈니스 가치를 좌우할 때

**CPU 코어당 최대 IOPS가 필요한 경우**:
- NVMe SSD의 성능을 100% 활용해야 할 때
- 여러 NVMe SSD를 소수의 CPU 코어로 구동해야 할 때

### 9.3 하이브리드 접근

실제 환경에서는 두 가지를 함께 사용하는 것도 가능하다:

```
┌─────────────────────────────────────────┐
│              서버 시스템                   │
│                                          │
│  ┌─────────────┐  ┌──────────────────┐  │
│  │ NVMe SSD #1 │  │  NVMe SSD #2,3   │  │
│  │ (커널 NVMe) │  │  (SPDK)          │  │
│  │             │  │                  │  │
│  │ /dev/nvme0n1│  │  SPDK bdev       │  │
│  │ OS 부팅     │  │  데이터 스토리지  │  │
│  │ 로그 저장   │  │  고성능 I/O      │  │
│  └─────────────┘  └──────────────────┘  │
│                                          │
│  OS/로그용 SSD는 커널 드라이버            │
│  데이터 경로 SSD는 SPDK                   │
└─────────────────────────────────────────┘
```


## 10. SPDK 주요 데이터 구조 정리

### 10.1 nvme_pcie_ctrlr (컨트롤러)

```c
// lib/nvme/nvme_pcie_internal.h
struct nvme_pcie_ctrlr {
    struct spdk_nvme_ctrlr ctrlr;       // 공통 컨트롤러 구조체

    volatile struct spdk_nvme_registers *regs;  // BAR0 MMIO 매핑 주소
    uint64_t regs_size;                         // BAR0 크기

    struct {
        void *bar_va;                   // CMB 가상 주소
        uint64_t bar_pa;                // CMB 물리 주소
        uint64_t size;                  // CMB 크기
        uint64_t current_offset;        // 현재 할당 오프셋
    } cmb;

    uint32_t doorbell_stride_u32;       // Doorbell 간격 (uint32 단위)
    struct spdk_pci_device *devhandle;  // PCI 디바이스 핸들
    volatile uint32_t *doorbell_base;   // Doorbell 레지스터 시작 주소
};

// 커널 대응: struct nvme_dev (drivers/nvme/host/pci.c)
```

### 10.2 nvme_pcie_qpair (큐 페어)

```c
// lib/nvme/nvme_pcie_internal.h
struct nvme_pcie_qpair {
    // === 핫 데이터 (같은 캐시라인에 배치) ===
    volatile uint32_t *sq_tdbl;          // SQ Tail Doorbell 주소
    volatile uint32_t *cq_hdbl;          // CQ Head Doorbell 주소
    struct spdk_nvme_cmd *cmd;           // SQ 커맨드 배열 (DMA 가능)
    struct spdk_nvme_cpl *cpl;           // CQ 완료 배열 (DMA 가능)

    TAILQ_HEAD(, nvme_tracker) free_tr;        // 사용 가능한 tracker
    TAILQ_HEAD(, nvme_tracker) outstanding_tr;  // 진행 중인 tracker
    struct nvme_tracker *tr;                    // tracker 배열

    uint16_t num_entries;               // 큐 엔트리 수
    uint16_t sq_tail;                   // SQ tail 포인터
    uint16_t cq_head;                   // CQ head 포인터
    struct {
        uint8_t phase : 1;             // 현재 CQ phase
        uint8_t delay_cmd_submit : 1;  // Doorbell 지연 여부
    } flags;

    // === 콜드 데이터 ===
    struct spdk_nvme_qpair qpair;       // 공통 qpair 구조체
    uint64_t cmd_bus_addr;              // SQ 물리 주소
    uint64_t cpl_bus_addr;              // CQ 물리 주소
};

// 커널 대응: struct nvme_queue (drivers/nvme/host/pci.c)
```

### 10.3 nvme_tracker (요청 추적)

```c
// lib/nvme/nvme_pcie_internal.h
struct nvme_tracker {
    TAILQ_ENTRY(nvme_tracker) tq_list;  // free/outstanding 리스트 링크
    struct nvme_request *req;            // 원본 요청 참조
    uint16_t cid;                        // Command ID (CQ에서 매칭용)

    spdk_nvme_cmd_cb cb_fn;             // 완료 콜백 함수
    void *cb_arg;                        // 콜백 인자

    uint64_t prp_sgl_bus_addr;          // PRP/SGL 리스트의 물리 주소

    union {
        uint64_t prp[NVME_MAX_PRP_LIST_ENTRIES];       // PRP 리스트 (503개)
        struct spdk_nvme_sgl_descriptor sgl[NVME_MAX_SGL_DESCRIPTORS];  // SGL (250개)
    } u;
};
// sizeof(nvme_tracker) == 4096 (정확히 1 페이지)
// PRP 리스트가 페이지 경계를 넘지 않도록 보장

// 커널 대응: struct nvme_iod (request의 private data)
```


## 11. 핵심 코드 매핑 요약

아래 표는 동일한 기능을 수행하는 커널과 SPDK의 함수/구조체를 대응시킨 것이다.

### 11.1 I/O 제출 경로

| 기능 | 커널 함수 | SPDK 함수 | 소스 위치 |
|------|-----------|-----------|-----------|
| I/O 제출 진입 | `nvme_queue_rq()` | `spdk_nvme_ns_cmd_read()` | pci.c / nvme_ns_cmd.c |
| NVMe 커맨드 빌드 | `nvme_setup_cmd()` | `_nvme_ns_cmd_rw()` | core.c / nvme_ns_cmd.c |
| DMA 매핑 | `nvme_map_data()` | `nvme_pcie_qpair_build_contig_request()` | pci.c / nvme_pcie_common.c |
| SQ 커맨드 복사 | `nvme_sq_copy_cmd()` | `nvme_pcie_copy_command()` | pci.c / nvme_pcie_common.c |
| Doorbell 쓰기 | `nvme_write_sq_db()` | `nvme_pcie_qpair_ring_sq_doorbell()` | pci.c / nvme_pcie_internal.h |
| SQ Lock | `spin_lock(&nvmeq->sq_lock)` | 없음 (single-thread) | pci.c / - |

### 11.2 I/O 완료 경로

| 기능 | 커널 함수 | SPDK 함수 | 소스 위치 |
|------|-----------|-----------|-----------|
| 완료 트리거 | `nvme_irq()` | `process_completions()` (polling) | pci.c / nvme_pcie_common.c |
| CQ 폴링 | `nvme_poll_cq()` | `while` loop in `process_completions()` | pci.c / nvme_pcie_common.c |
| CQE 처리 | `nvme_handle_cqe()` | `nvme_pcie_qpair_complete_tracker()` | pci.c / nvme_pcie_common.c |
| 요청 완료 | `blk_mq_complete_request()` | `nvme_complete_request()` → 콜백 | core.c / nvme_internal.h |
| CQ Doorbell | `nvme_ring_cq_doorbell()` | `nvme_pcie_qpair_ring_cq_doorbell()` | pci.c / nvme_pcie_internal.h |

### 11.3 초기화 경로

| 기능 | 커널 함수 | SPDK 함수 | 소스 위치 |
|------|-----------|-----------|-----------|
| PCI probe | `nvme_probe()` | `nvme_pcie_ctrlr_scan()` | pci.c / nvme_pcie.c |
| 컨트롤러 생성 | `nvme_pci_alloc_dev()` | `nvme_pcie_ctrlr_construct()` | pci.c / nvme_pcie.c |
| BAR 매핑 | `nvme_dev_map()` | `nvme_pcie_ctrlr_allocate_bars()` | pci.c / nvme_pcie.c |
| Admin 큐 생성 | `nvme_alloc_admin_tag_set()` | `nvme_pcie_ctrlr_construct_admin_qpair()` | pci.c / nvme_pcie_common.c |
| I/O 큐 생성 | `nvme_setup_io_queues()` | `nvme_pcie_ctrlr_create_io_qpair()` | pci.c / nvme_pcie_common.c |

### 11.4 데이터 구조 대응

| 기능 | 커널 구조체 | SPDK 구조체 |
|------|-------------|-------------|
| NVMe 컨트롤러 | `struct nvme_dev` | `struct nvme_pcie_ctrlr` |
| NVMe 큐 | `struct nvme_queue` | `struct nvme_pcie_qpair` |
| I/O 요청 메타데이터 | `struct nvme_iod` | `struct nvme_tracker` |
| NVMe 네임스페이스 | `struct nvme_ns` | `struct spdk_nvme_ns` |
| SQ 커맨드 | `struct nvme_command` | `struct spdk_nvme_cmd` |
| CQ 완료 엔트리 | `struct nvme_completion` | `struct spdk_nvme_cpl` |
| NVMe 레지스터 | `struct nvme_bar` | `struct spdk_nvme_registers` |


## 12. SPDK Hello World 예제 분석

SPDK의 `examples/nvme/hello_world/hello_world.c`는 SPDK NVMe 드라이버의 전체 사용 패턴을 보여준다.

```c
// 1. 환경 초기화 (DPDK EAL, hugepage, VFIO)
spdk_env_init(&opts);

// 2. NVMe 디바이스 발견 및 연결
spdk_nvme_probe(&trid, NULL, probe_cb, attach_cb, NULL);
// probe_cb:  디바이스를 발견했을 때 연결 여부 결정
// attach_cb: 연결 성공 시 네임스페이스 등록

// 3. I/O qpair 할당 (각 네임스페이스별)
ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);

// 4. DMA 버퍼 할당 (hugepage에서)
sequence.buf = spdk_zmalloc(0x1000, 0x1000, NULL,
                            SPDK_ENV_NUMA_ID_ANY, SPDK_MALLOC_DMA);

// 5. I/O 제출 (비동기, 콜백 등록)
spdk_nvme_ns_cmd_write(ns, qpair, buf, 0, 1, write_complete, &sequence, 0);

// 6. 완료 대기 (polling)
while (!sequence.is_completed) {
    spdk_nvme_qpair_process_completions(qpair, 0);
    // ↑ CQ를 직접 확인하고, 완료된 요청의 콜백을 호출
}

// 7. 정리
spdk_nvme_ctrlr_free_io_qpair(qpair);
spdk_nvme_detach(ctrlr);
```

이 예제에서 볼 수 있는 SPDK의 핵심 특징:
- **완전한 비동기**: I/O 제출과 완료가 분리되어 있다.
- **명시적 폴링**: 애플리케이션이 `process_completions()`를 직접 호출해야 한다.
- **콜백 기반**: 완료 시 등록된 콜백 함수가 호출된다.
- **DMA 메모리 명시적 관리**: `spdk_zmalloc()`으로 DMA 가능한 버퍼를 직접 할당한다.


## 13. 결론

커널 NVMe 드라이버와 SPDK는 같은 NVMe 하드웨어를 제어하지만, 설계 목표가 완전히 다르다.

**커널 NVMe 드라이버**는 운영체제의 일부로서 보호, 공유, 호환성을 제공한다.
syscall, VFS, Block Layer, 인터럽트 등 여러 추상화 계층을 거치며, 이 과정에서 레이턴시와 CPU 오버헤드가 발생한다.
하지만 파일시스템 지원, 멀티프로세스 공유, 표준 도구 호환성 등 범용 환경에서 필수적인 기능을 제공한다.

**SPDK**는 이 모든 추상화 계층을 제거하고, 유저스페이스에서 NVMe 하드웨어를 직접 제어한다.
VFIO/UIO로 디바이스를 커널에서 분리하고, mmap으로 BAR0에 직접 접근하며, hugepage로 DMA 메모리를 관리한다.
그 결과 레이턴시는 3~5배 감소하고, IOPS는 3~4배 증가한다.
대신 파일시스템 사용 불가, 디바이스 독점, CPU 100% 점유 등의 트레이드오프가 있다.

두 드라이버의 I/O 경로를 코드 레벨에서 비교하면, NVMe SQ/CQ의 조작 방식은 거의 동일하다.
커맨드를 SQ에 복사하고, Doorbell을 울리고, CQ에서 phase bit를 확인하는 로직은 같다.
차이는 그 "위아래"에 있다: 커널은 syscall/VFS/Block Layer를 거쳐야 NVMe 드라이버에 도달하고, 완료는 인터럽트를 통해 전달된다.
SPDK는 이 모든 것 없이, 애플리케이션에서 곧바로 NVMe 하드웨어에 접근한다.
