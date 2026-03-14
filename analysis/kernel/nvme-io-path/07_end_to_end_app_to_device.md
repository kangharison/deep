# End-to-End NVMe I/O 완전 추적: Application → Device → 데이터 반환

이 문서는 하나의 Read I/O와 하나의 Write I/O가 애플리케이션에서 시작하여 QEMU NVMe 디바이스에서 실행되고 다시 애플리케이션으로 돌아오는 전체 과정을 함수 단위로 추적하는 최종 가이드이다.

소스코드 위치:
- 커널: `~/sources/linux/`
- QEMU: `~/sources/qemu/`

---

## 1. 전체 아키텍처: Application → Device 완전 연결도

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        User Space (Guest VM)                           │
│                                                                        │
│  ┌──────────┐    pread(fd, buf, 4096, off)                            │
│  │   fio    │──────────────────────────────────┐                      │
│  │ (or app) │    pwrite(fd, buf, 4096, off)    │                      │
│  └──────────┘    io_uring SQE submit           │                      │
│                                                 ▼                      │
├─────────────────────────────────────────────────────────────────────────┤
│                     Kernel Space (Guest VM)                             │
│                                                                        │
│  ┌──────────────┐     ┌─────────────────┐     ┌──────────────────┐    │
│  │   VFS Layer  │────▶│   Block Layer   │────▶│  NVMe Driver     │    │
│  │              │     │   (blk-mq)      │     │  (pci.c)         │    │
│  │ sys_read()   │     │                 │     │                  │    │
│  │ sys_write()  │     │ submit_bio()    │     │ nvme_queue_rq()  │    │
│  │ vfs_read()   │     │ blk_mq_submit  │     │ nvme_sq_copy_cmd │    │
│  │ __blkdev_    │     │   _bio()        │     │ nvme_write_sq_db │    │
│  │  direct_IO() │     │ bio → request   │     │   writel(tail,   │    │
│  │ bio 생성     │     │ tag 할당        │     │     q_db)        │    │
│  └──────────────┘     └─────────────────┘     └────────┬─────────┘    │
│                                                         │ MMIO write   │
│                                                         ▼              │
│                              ┌──────────────────────────────────┐      │
│                              │ PCIe BAR0 Doorbell Register     │      │
│                              │ offset = 0x1000 + (qid*2) * 4  │      │
│                              └──────────────┬───────────────────┘      │
├─────────────────────────────────────────────┼──────────────────────────┤
│                    VM-Exit Boundary          │                          │
│                    (KVM trap on MMIO)        │                          │
├─────────────────────────────────────────────┼──────────────────────────┤
│                     Host Space (QEMU)        ▼                          │
│                                                                        │
│  ┌────────────────────────────────────────────────────────────────┐    │
│  │                    QEMU NVMe Controller                        │    │
│  │                    (hw/nvme/ctrl.c)                             │    │
│  │                                                                │    │
│  │  nvme_mmio_write()                                             │    │
│  │    └─▶ nvme_process_db(): SQ tail 업데이트                     │    │
│  │         └─▶ qemu_bh_schedule(sq->bh)                           │    │
│  │              └─▶ nvme_process_sq(): guest 메모리에서 SQE DMA   │    │
│  │                   └─▶ nvme_io_cmd()                            │    │
│  │                        ├─▶ nvme_read() → nvme_blk_read()      │    │
│  │                        └─▶ nvme_write() → nvme_blk_write()    │    │
│  │                                                                │    │
│  │  [I/O 완료 후]                                                  │    │
│  │  nvme_rw_cb()                                                   │    │
│  │    └─▶ nvme_enqueue_req_completion()                           │    │
│  │         └─▶ nvme_post_cqes(): CQE를 guest CQ에 DMA write      │    │
│  │              └─▶ nvme_irq_assert() → msix_notify()             │    │
│  └────────────────────────────────────┬───────────────────────────┘    │
│                                        │                               │
│  ┌─────────────────────────────────────▼──────────────────────────┐    │
│  │              Host Block I/O                                    │    │
│  │  blk_aio_preadv() / blk_aio_pwritev()                        │    │
│  │  → host filesystem → 실제 디스크/이미지 파일                    │    │
│  └────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────┘
```

### 각 계층 간 인터페이스 요약

| 계층 경계 | 인터페이스 | 데이터 형태 |
|-----------|-----------|-------------|
| App → VFS | syscall (read/write/io_uring) | fd, user buffer, offset, size |
| VFS → Block | `submit_bio()` | `struct bio` (sector, bio_vec) |
| Block → NVMe Driver | `blk_mq_ops.queue_rq` | `struct request` (merged bios, tag) |
| NVMe Driver → Device | MMIO writel (doorbell) | `struct nvme_rw_command` (64B SQE in SQ) |
| Guest → QEMU | KVM VM-Exit (MMIO trap) | doorbell register write |
| QEMU NVMe → Host I/O | `blk_aio_preadv/pwritev` | offset + iovec into backing file |
| Device → Driver (completion) | MSI-X 인터럽트 | CQE (16B) in CQ via DMA |
| Driver → Block | `blk_mq_complete_request()` | `struct request` 완료 |
| Block → VFS → App | `bio_endio()` → syscall return | user buffer에 데이터 존재 |

---

## 2. Read I/O 완전 추적 (fio read → 데이터 반환)

### Phase 1: Application → Kernel 진입

```
fio (libaio engine, 4KB random read)
  │
  │  pread(fd, buf, 4096, offset)
  │  또는 io_uring: io_uring_enter() → SQE {opcode=IORING_OP_READ}
  │
  ▼
sys_read() / sys_pread64()
  │
  ▼
vfs_read()
  │  file->f_op->read_iter = blkdev_read_iter()  (block device의 경우)
  │
  ▼
blkdev_read_iter()
  │
  ▼
filemap_read()  (buffered I/O 경우)
  또는
__blkdev_direct_IO()  (O_DIRECT 경우, fio의 기본 경로)
  │
  │  1. kiocb에서 sector 계산: pos >> 9  (512바이트 섹터 단위)
  │  2. bio_alloc() → struct bio 할당
  │  3. bio->bi_iter.bi_sector = pos >> 9
  │  4. bio->bi_opf = REQ_OP_READ
  │  5. bio_add_page(bio, page, 4096, 0)
  │     → bio_vec {bv_page, bv_len=4096, bv_offset=0}
  │  6. get_user_pages_fast()로 user buffer의 물리 페이지를 pin
  │
  ▼
submit_bio(bio)                          ← blk-core.c:1244
  │  task_io_account_read(4096)
  │  bio_set_ioprio(bio)
  │
  ▼
submit_bio_noacct(bio)                   ← blk-core.c:1064
  │  유효성 검사 (섹터 범위, 읽기전용 체크 등)
  │
  ▼
__submit_bio_noacct_mq(bio)              ← blk-core.c:957
  │
  ▼
__submit_bio(bio)                        ← blk-core.c:827
  │  disk->fops->submit_bio(bio)
  │  또는 blk_mq_submit_bio(bio)
  │
  ▼
```

### Phase 2: Block Layer 통과 (blk-mq)

```
blk_mq_submit_bio(bio)                  ← blk-mq.c:3697
  │
  │  ① bio_queue_enter(bio): request_queue 사용 카운터 증가
  │
  │  ② bio 분할 검사:
  │     __bio_split_to_limits(bio, &q->limits, &nr_segs)
  │     4KB 요청은 max_sectors를 초과하지 않으므로 분할 없음
  │
  │  ③ 병합(merge) 시도:
  │     blk_mq_attempt_bio_merge(q, bio, nr_segs)
  │     plug 리스트와 스케줄러에서 인접 요청과 병합 가능한지 확인
  │     4KB 단독 요청이면 병합 대상 없음 → 실패
  │
  │  ④ request 할당:
  │     blk_mq_get_new_requests(q, plug, bio)
  │       └─ __blk_mq_alloc_requests()
  │            └─ blk_mq_get_tag(): sbitmap에서 빈 비트 확보 = tag 할당
  │               tag = NVMe command ID로 사용됨 (0 ~ queue_depth-1)
  │
  │  ⑤ bio → request 변환:
  │     blk_mq_bio_to_request(rq, bio, nr_segs)
  │       rq->__sector = bio->bi_iter.bi_sector
  │       rq->__data_len = bio->bi_iter.bi_size (= 4096)
  │       rq->bio = bio
  │
  │  ⑥ 디스패치 경로 선택:
  │
  │  [경로 A: plug이 있는 경우 - 가장 일반적]
  │     blk_add_rq_to_plug(plug, rq)
  │     → plug 리스트에 추가, blk_finish_plug()에서 일괄 디스패치
  │     → blk_mq_flush_plug_list() → blk_mq_dispatch_plug_list()
  │
  │  [경로 C: 직접 디스패치 - NVMe 최단 경로]
  │     blk_mq_try_issue_directly(hctx, rq)
  │       └─ __blk_mq_issue_directly(hctx, rq)
  │            └─ hctx->queue->mq_ops->queue_rq(hctx, &bd)
  │               = nvme_queue_rq()  ← ★ NVMe 드라이버 진입점
  │
  ▼
```

**blk-mq의 핵심 역할 정리:**
- `bio` (단일 I/O 요청)를 `request` (NVMe에 제출할 단위)로 변환한다.
- `tag`를 할당하여 NVMe command ID를 부여한다 (완료 시 어떤 요청인지 식별).
- 인접 요청을 병합하여 I/O 효율을 높인다.
- CPU → HW queue 매핑을 관리한다.

### Phase 3: NVMe 드라이버 → SQ 기록 → Doorbell

```
nvme_queue_rq(hctx, bd)                 ← pci.c:1675
  │
  │  nvmeq = hctx->driver_data    (NVMe I/O 큐)
  │  req = bd->rq                  (블록 요청)
  │  iod = blk_mq_rq_to_pdu(req)  (request 뒤에 붙은 nvme_iod)
  │
  │  ① 큐 상태 확인:
  │     test_bit(NVMEQ_ENABLED, &nvmeq->flags)
  │     nvme_check_ready(&dev->ctrl, req, true)
  │
  │  ② nvme_prep_rq(req):                           ← pci.c:1598
  │     │
  │     │  [단계 1] nvme_setup_cmd(ns, req)          ← core.c:1427
  │     │     REQ_OP_READ → nvme_setup_rw(ns, req, cmd, nvme_cmd_read)
  │     │
  │     │     nvme_setup_rw():                        ← core.c:1302
  │     │       cmd->rw.opcode = nvme_cmd_read (0x02)
  │     │       cmd->rw.nsid   = cpu_to_le32(ns_id)     예: 1
  │     │       cmd->rw.slba   = nvme_sect_to_lba(blk_rq_pos(req))
  │     │                        예: sector 8 → LBA 1 (4KB LBA 기준)
  │     │       cmd->rw.length = (4096 >> lba_shift) - 1 = 0
  │     │                        (NVMe 스펙: NLB = 전송 블록 수 - 1)
  │     │       cmd->common.command_id = nvme_cid(req) = tag
  │     │
  │     │  [단계 2] nvme_map_data(req)               ← pci.c:1425
  │     │     단일 세그먼트이므로 → nvme_pci_setup_data_simple(req)
  │     │       dma_map_bvec(): bio_vec의 page를 DMA 주소로 매핑
  │     │       cmd->rw.dptr.prp1 = dma_addr    (PRP Entry 1)
  │     │       4KB 단일 페이지이므로 PRP2는 불필요
  │     │
  │     │  [단계 3] nvme_start_request(req)
  │     │     blk_mq_start_request(req)  → 타임아웃 타이머 시작
  │     │
  │     │  완성된 NVMe SQE (64바이트):
  │     │  ┌──────────────────────────────────────────────┐
  │     │  │ Byte 0     : opcode = 0x02 (Read)           │
  │     │  │ Byte 1     : flags  = 0x00                  │
  │     │  │ Byte 2-3   : command_id = tag               │
  │     │  │ Byte 4-7   : nsid = 1                       │
  │     │  │ Byte 24-31 : metadata = 0                   │
  │     │  │ Byte 32-39 : PRP1 = dma_addr (데이터 버퍼)  │
  │     │  │ Byte 40-47 : PRP2 = 0 (4KB이면 불필요)      │
  │     │  │ Byte 48-55 : SLBA = 시작 LBA                │
  │     │  │ Byte 56-57 : NLB = 0 (1블록 - 1)            │
  │     │  │ Byte 58-63 : control, dsmgmt 등             │
  │     │  └──────────────────────────────────────────────┘
  │
  │  ③ spin_lock(&nvmeq->sq_lock)
  │
  │  ④ nvme_sq_copy_cmd(nvmeq, &iod->cmd)           ← pci.c:835
  │     memcpy(sq_cmds + (sq_tail << sqes), cmd, 64)
  │     SQ 링 버퍼의 tail 위치에 64바이트 커맨드 복사
  │     sq_tail = (sq_tail + 1) % q_depth
  │
  │     SQ 메모리 레이아웃 (예: q_depth=64):
  │     ┌───────────┬───────────┬───────────┬─────┐
  │     │ Entry 0   │ Entry 1   │ Entry 2   │ ... │
  │     │ (64 bytes)│ (64 bytes)│ (64 bytes)│     │
  │     └───────────┴───────────┴───────────┴─────┘
  │                   ▲ sq_tail (새 커맨드 기록 위치)
  │
  │  ⑤ nvme_write_sq_db(nvmeq, bd->last)            ← pci.c:808
  │     │
  │     │  bd->last == true (이 배치의 마지막 요청)인 경우:
  │     │    dbbuf 이벤트 인덱스 체크 후
  │     │    writel(sq_tail, nvmeq->q_db)
  │     │
  │     │  writel()의 의미:
  │     │    q_db = PCIe BAR0 + 0x1000 + (qid * 2) * doorbell_stride
  │     │    이 주소에 sq_tail 값을 MMIO write
  │     │    → CPU가 PCIe 트랜잭션을 생성
  │     │    → VM 환경에서는 KVM이 이 MMIO를 trap → VM-Exit 발생
  │     │
  │  ⑥ spin_unlock(&nvmeq->sq_lock)
  │
  ▼
```

### Phase 4: PCIe → VM-Exit → QEMU NVMe 디바이스

```
[Guest Kernel]
writel(sq_tail, q_db)
  │
  │  이 주소는 PCIe BAR0 매핑 영역이다.
  │  VM 환경에서 BAR0는 QEMU가 등록한 MemoryRegion이므로
  │  KVM이 EPT violation을 감지하고 VM-Exit를 발생시킨다.
  │
  ▼
[KVM]
  │  VM-Exit reason: EPT_VIOLATION (MMIO access)
  │  MMIO 주소, 크기, 데이터를 QEMU로 전달
  │
  ▼
[QEMU - NVMe Controller]

nvme_mmio_write(opaque, addr, data, size)       ← ctrl.c:8515
  │
  │  n = (NvmeCtrl *)opaque
  │  addr = doorbell offset (예: 0x1008 = SQ1 doorbell)
  │
  │  if (addr < sizeof(n->bar)):
  │    nvme_write_bar()  → 컨트롤러 레지스터 쓰기
  │  else:
  │    nvme_process_db(n, addr, data)  → ★ Doorbell 처리
  │
  ▼
nvme_process_db(n, addr, val)                    ← ctrl.c:8369
  │
  │  Doorbell 주소에서 SQ/CQ 구분:
  │    bit 2 of ((addr - 0x1000) >> 2):
  │      0 = Submission Queue doorbell
  │      1 = Completion Queue doorbell
  │
  │  [SQ Doorbell인 경우]
  │  qid = (addr - 0x1000) >> 3
  │  new_tail = val & 0xffff
  │
  │  sq = n->sq[qid]
  │  sq->tail = new_tail       ← SQ tail 업데이트!
  │
  │  qemu_bh_schedule(sq->bh)  ← bottom-half 스케줄링
  │    → QEMU 이벤트 루프에서 nvme_process_sq() 호출 예약
  │
  ▼
nvme_process_sq(opaque)                          ← ctrl.c:7777
  │
  │  sq = opaque
  │  cq = n->cq[sq->cqid]
  │
  │  while (!(sq가 비어있음 || 가용 request 없음)):
  │
  │    ① SQE를 guest 메모리에서 읽기 (DMA read):
  │       addr = sq->dma_addr + (sq->head << NVME_SQES)
  │              NVME_SQES = 6 → 1 << 6 = 64바이트
  │       nvme_addr_read(n, addr, &cmd, sizeof(cmd))
  │         → pci_dma_read(): guest 물리 주소에서 64바이트를 읽음
  │         → 읽은 내용이 NvmeCmd 구조체 (= SQE)
  │
  │    ② SQ head 전진:
  │       nvme_inc_sq_head(sq)
  │
  │    ③ request 할당:
  │       req = QTAILQ_FIRST(&sq->req_list)  (pre-allocated pool)
  │       req->cqe.cid = cmd.cid             (command ID 복사)
  │       memcpy(&req->cmd, &cmd, sizeof(NvmeCmd))
  │
  │    ④ 커맨드 실행:
  │       status = nvme_io_cmd(n, req)        ← ctrl.c:4664
  │
  ▼
```

### Phase 5: QEMU 디바이스 커맨드 실행

```
nvme_io_cmd(n, req)                              ← ctrl.c:4664
  │
  │  nsid = le32_to_cpu(req->cmd.nsid)
  │  ns = nvme_ns(n, nsid)
  │  req->ns = ns
  │
  │  switch (ns->csi):
  │    case NVME_CSI_NVM:
  │      return nvme_io_cmd_nvm(n, req)
  │
  ▼
nvme_io_cmd_nvm(n, req)                          ← ctrl.c:4635
  │
  ▼
__nvme_io_cmd_nvm(n, req)                        ← ctrl.c:4609
  │
  │  switch (req->cmd.opcode):
  │    case NVME_CMD_READ:
  │      return nvme_read(n, req)    ← ★ Read 처리
  │
  ▼
nvme_read(n, req)                                ← ctrl.c:3607
  │
  │  rw = (NvmeRwCmd *)&req->cmd
  │  slba = le64_to_cpu(rw->slba)         예: LBA 1
  │  nlb = le16_to_cpu(rw->nlb) + 1       예: 1 블록
  │  data_size = nvme_l2b(ns, nlb)         예: 4096 바이트
  │  blk = ns->blkconf.blk                 (backing BlockBackend)
  │
  │  ① 유효성 검사:
  │     nvme_check_mdts(n, data_size)    MDTS 초과 체크
  │     nvme_check_bounds(ns, slba, nlb) LBA 범위 체크
  │
  │  ② PRP/SGL 디코딩 → scatter-gather 리스트 구성:
  │     nvme_map_data(n, nlb, req)                   ← ctrl.c:1237
  │       └─ nvme_map_dptr(n, &req->sg, len, &req->cmd)  ← ctrl.c:1187
  │            │  prp1 = le64_to_cpu(cmd->dptr.prp1)
  │            │  prp2 = le64_to_cpu(cmd->dptr.prp2)
  │            │
  │            └─ nvme_map_prp(n, sg, prp1, prp2, len)
  │                 │
  │                 │  PRP1 = guest 물리 주소 (데이터를 쓸 위치)
  │                 │  4KB이면 PRP1 하나로 충분
  │                 │  → QEMUSGList 또는 QEMUIOVector에 매핑
  │                 │
  │                 │  QEMU 내부에서:
  │                 │  guest physical addr → host virtual addr
  │                 │  (address_space_map 또는 dma_memory_map 사용)
  │
  │  ③ 데이터 오프셋 계산:
  │     data_offset = nvme_l2b(ns, slba)
  │     예: LBA 1 × 4096 = 4096 (backing 파일에서의 바이트 오프셋)
  │
  │  ④ 비동기 읽기 시작:
  │     block_acct_start(blk_get_stats(blk), &req->acct, data_size, BLOCK_ACCT_READ)
  │     nvme_blk_read(blk, data_offset, BDRV_SECTOR_SIZE, nvme_rw_cb, req)
  │
  ▼
nvme_blk_read(blk, offset, align, cb, req)       ← ctrl.c:1456
  │
  │  if (req->sg.flags & NVME_SG_DMA):
  │    req->aiocb = dma_blk_read(blk, &req->sg.qsg, offset, align, cb, req)
  │      → guest 메모리의 scatter-gather 리스트를 사용한 DMA 읽기
  │      → 내부적으로: backing 파일에서 데이터를 읽어 guest 물리 메모리에 기록
  │  else:
  │    req->aiocb = blk_aio_preadv(blk, offset, &req->sg.iov, 0, cb, req)
  │      → 일반 iovec 기반 비동기 읽기
  │
  │  ★ 이 시점에서 host의 실제 I/O가 시작된다:
  │     QEMU의 block backend가 backing 파일(qcow2/raw 이미지)에서
  │     offset 위치의 4096 바이트를 비동기로 읽는다.
  │     host의 AIO/io_uring 또는 thread pool을 통해 실행된다.
  │
  │  return NVME_NO_COMPLETE
  │  (비동기 I/O이므로 즉시 완료되지 않음, 콜백이 나중에 호출됨)
  │
  ▼
[Host I/O 완료 후 QEMU 이벤트 루프에서 콜백 호출]
  │
  ▼
nvme_rw_cb(opaque, ret)                          ← ctrl.c:2187
  │
  │  req = opaque
  │  ret = 0 (성공)
  │
  │  메타데이터가 없는 일반 경우:
  │    → nvme_rw_complete_cb(req, 0) 호출
  │
  ▼
nvme_rw_complete_cb(req, ret)
  │
  │  req->status = NVME_SUCCESS (ret == 0인 경우)
  │
  ▼
nvme_enqueue_req_completion(cq, req)             ← ctrl.c:1553
  │
  │  QTAILQ_REMOVE(&req->sq->out_req_list, req, entry)
  │  QTAILQ_INSERT_TAIL(&cq->req_list, req, entry)
  │
  │  qemu_bh_schedule(cq->bh)   ← CQ 처리 bottom-half 스케줄링
  │
  ▼
```

### Phase 6: Completion → 인터럽트 주입

```
nvme_post_cqes(opaque)                           ← ctrl.c:1498
  │
  │  cq = opaque
  │  pending = (cq->head != cq->tail)  이전에 미처리 CQE가 있었는지
  │
  │  QTAILQ_FOREACH_SAFE(req, &cq->req_list, entry, next):
  │
  │    ① CQ가 가득 찼는지 확인:
  │       if (nvme_cq_full(cq)) break;
  │
  │    ② CQE 구성 (16바이트):
  │       req->cqe.status = cpu_to_le16((req->status << 1) | cq->phase)
  │       req->cqe.sq_id = cpu_to_le16(sq->sqid)
  │       req->cqe.sq_head = cpu_to_le16(sq->head)
  │
  │       완성된 CQE (16바이트):
  │       ┌────────────────────────────────────────────────────┐
  │       │ Byte 0-3   : DW0 = command specific result         │
  │       │ Byte 4-7   : DW1 = reserved                       │
  │       │ Byte 8-9   : SQ Head Pointer                      │
  │       │ Byte 10-11 : SQ Identifier                        │
  │       │ Byte 12-13 : Command Identifier (= tag)           │
  │       │ Byte 14-15 : Status Field (상태 + Phase bit)      │
  │       │              bit 0 = Phase Tag (0 또는 1)          │
  │       │              bit 1-15 = Status Code                │
  │       │              성공 = 0x0001 (status=0, phase=1)     │
  │       └────────────────────────────────────────────────────┘
  │
  │    ③ CQE를 guest 메모리에 DMA write:
  │       addr = cq->dma_addr + (cq->tail << NVME_CQES)
  │              NVME_CQES = 4 → 1 << 4 = 16바이트
  │       pci_dma_write(PCI_DEVICE(n), addr, &req->cqe, sizeof(req->cqe))
  │         → guest 물리 메모리의 CQ 영역에 16바이트 CQE 기록
  │
  │    ④ CQ tail 전진:
  │       nvme_inc_cq_tail(cq)
  │       tail이 한 바퀴 돌면 phase 토글 (0↔1)
  │
  │    ⑤ request를 SQ의 free list로 반환:
  │       QTAILQ_INSERT_TAIL(&sq->req_list, req, entry)
  │
  │  ⑥ 인터럽트 발생:
  │     if (cq->tail != cq->head):  처리한 CQE가 있으면
  │       if (cq->irq_enabled && !pending):
  │         n->cq_pending++
  │       nvme_irq_assert(n, cq)                    ← ctrl.c:674
  │
  ▼
nvme_irq_assert(n, cq)
  │
  │  if (msix_enabled(pci)):
  │    msix_notify(pci, cq->vector)
  │      → KVM으로 MSI-X 인터럽트 전달
  │      → KVM이 guest vCPU에 인터럽트 주입
  │      → guest의 IDT에 등록된 인터럽트 핸들러 호출
  │
  ▼
```

### Phase 7: 인터럽트 처리 → Application 반환

```
[Guest Kernel - 인터럽트 컨텍스트]

nvme_irq(irq, data)                             ← pci.c:1995
  │
  │  nvmeq = data  (인터럽트 벡터에 바인딩된 NVMe I/O 큐)
  │  DEFINE_IO_COMP_BATCH(iob)
  │
  ▼
nvme_poll_cq(nvmeq, &iob)                       ← pci.c:1955
  │
  │  while (nvme_cqe_pending(nvmeq)):
  │    │
  │    │  CQE의 Phase bit 확인:
  │    │  cqe = &nvmeq->cqes[cq_head]
  │    │  phase_bit = le16_to_cpu(cqe->status) & 1
  │    │  if (phase_bit != nvmeq->cq_phase) → 새 CQE 없음 → 루프 종료
  │    │
  │    │  dma_rmb()  ← DMA 읽기 메모리 배리어
  │    │  (Phase 확인 후 CQE 나머지 필드를 안전하게 읽기 위해)
  │    │
  │    ▼
  │  nvme_handle_cqe(nvmeq, iob, cq_head)       ← pci.c:1883
  │    │
  │    │  cqe = &nvmeq->cqes[idx]
  │    │  command_id = READ_ONCE(cqe->command_id)
  │    │
  │    │  ① command_id로 원래 request 찾기:
  │    │     req = nvme_find_rq(nvme_queue_tagset(nvmeq), command_id)
  │    │     (tagset의 rqs[command_id]에서 request 포인터를 가져옴)
  │    │
  │    │  ② 완료 처리:
  │    │     nvme_try_complete_req(req, cqe->status, cqe->result)
  │    │       → NVMe 상태를 request에 기록
  │    │
  │    │     blk_mq_add_to_batch(req, iob, ..., nvme_pci_complete_batch)
  │    │       → 배치 완료 리스트에 추가 (성능 최적화)
  │    │       또는 직접: nvme_pci_complete_rq(req)
  │    │
  │    ▼
  │  nvme_update_cq_head(nvmeq)                  ← pci.c:1924
  │    │  cq_head = (cq_head + 1) % q_depth
  │    │  if (wrap around): cq_phase ^= 1  (phase 토글)
  │    │
  │  [루프 반복 → 다음 CQE 처리]
  │
  │  nvme_ring_cq_doorbell(nvmeq)
  │    writel(cq_head, nvmeq->q_db + 1)
  │    → CQ Head Doorbell에 새 head 값 기록
  │    → 디바이스에게 "여기까지 처리했음" 알림
  │
  ▼
[nvme_irq() 복귀]
  │
  │  if (!rq_list_empty(&iob.req_list)):
  │    nvme_pci_complete_batch(&iob)
  │      → 배치로 모은 완료들을 한 번에 처리
  │
  │  return IRQ_HANDLED
  │
  ▼
[배치 완료 처리 또는 개별 완료]

nvme_pci_complete_rq(req)  또는  nvme_pci_complete_batch(iob)
  │
  │  nvme_unmap_data(req): DMA 매핑 해제
  │  nvme_cleanup_cmd(req)
  │
  ▼
blk_mq_complete_request(req)
  │
  ▼
blk_mq_end_request(req, status)
  │
  ▼
__blk_mq_end_request(req, status)
  │
  │  rq_qos_done(q, rq)   ← QoS 추적 종료
  │  blk_mq_free_request(rq) → tag 반환 (sbitmap bit clear)
  │
  ▼
bio_endio(bio)
  │
  │  bio->bi_end_io(bio)
  │    → __blkdev_direct_IO의 콜백
  │    → 완료 카운터 감소 → wake_up() 또는 io_uring CQE 생성
  │
  ▼
[Application Space]

pread() 반환 → buf에 4096바이트 데이터가 존재
  또는
io_uring: CQE가 CQ 링에 기록됨 → io_uring_enter()에서 반환

★ 데이터 흐름 요약:
  host 파일 → QEMU blk_aio_preadv → guest 물리 메모리(PRP1 주소) → user buffer
  (get_user_pages로 pinned된 페이지이므로 추가 복사 없이 직접 접근 가능)
```

---

## 3. Write I/O 완전 추적 (fio write → 디바이스 기록)

Write I/O는 Read I/O와 대부분의 경로가 동일하다. 여기서는 차이점을 중심으로 설명한다.

### Phase 1-2: Application → Block Layer (Read와 동일 구조, 방향만 반대)

```
fio: pwrite(fd, buf, 4096, offset)
  │
  │  차이점:
  │  - bio->bi_opf = REQ_OP_WRITE
  │  - task_io_account_write()가 아닌 count_vm_events(PGPGOUT, ...)
  │  - user buffer의 데이터가 이미 준비되어 있어야 함
  │  - get_user_pages()로 user buffer 페이지를 pin (Read와 동일)
  │
  ▼
submit_bio(bio)                                   ← blk-core.c:1244
  │  bio_op(bio) == REQ_OP_WRITE
  │  count_vm_events(PGPGOUT, bio_sectors(bio))
  │
  ▼
blk_mq_submit_bio(bio)
  │  (Read와 동일한 경로: tag 할당, bio→request 변환, 디스패치)
  │
  ▼
```

### Phase 3: NVMe 드라이버 (Write 커맨드 빌드)

```
nvme_queue_rq(hctx, bd)                          ← pci.c:1675
  │
  │  nvme_prep_rq(req):
  │    nvme_setup_cmd(ns, req)
  │      REQ_OP_WRITE → nvme_setup_rw(ns, req, cmd, nvme_cmd_write)
  │
  │      ★ Read와의 차이점:
  │      cmd->rw.opcode = nvme_cmd_write (0x01)   ← Read는 0x02
  │
  │      REQ_FUA가 설정된 경우:
  │        control |= NVME_RW_FUA
  │        → 디바이스가 volatile cache를 우회하여 미디어에 직접 기록
  │
  │      나머지 (SLBA, NLB, PRP 설정)는 Read와 동일
  │
  │    nvme_map_data(req):
  │      DMA 매핑 방향이 다름:
  │      Read:  DMA_FROM_DEVICE (디바이스 → 메모리)
  │      Write: DMA_TO_DEVICE   (메모리 → 디바이스)
  │      PRP1에는 동일하게 DMA 주소가 설정됨
  │
  │  SQE에 기록:
  │  ┌──────────────────────────────────────────────┐
  │  │ opcode = 0x01 (Write)                        │
  │  │ command_id = tag                              │
  │  │ nsid = 1                                      │
  │  │ PRP1 = dma_addr (데이터를 읽을 위치)          │
  │  │ SLBA = 시작 LBA                               │
  │  │ NLB = 0 (1블록)                               │
  │  │ control = NVME_RW_FUA (FUA인 경우)            │
  │  └──────────────────────────────────────────────┘
  │
  │  nvme_sq_copy_cmd() → nvme_write_sq_db() → writel()
  │
  ▼
```

### Phase 4-5: QEMU 디바이스 (Write 실행)

```
nvme_mmio_write() → nvme_process_db() → nvme_process_sq()
  │  (doorbell 처리는 Read와 완전히 동일)
  │
  ▼
nvme_io_cmd(n, req) → __nvme_io_cmd_nvm(n, req)
  │
  │  case NVME_CMD_WRITE:
  │    return nvme_write(n, req)     ← ctrl.c:3856 (인라인)
  │
  ▼
nvme_write(n, req) → nvme_do_write(n, req, false, false)  ← ctrl.c:3715
  │
  │  slba = le64_to_cpu(rw->slba)
  │  nlb = le16_to_cpu(rw->nlb) + 1
  │  data_size = nvme_l2b(ns, nlb)
  │
  │  ① 유효성 검사 (Read와 동일):
  │     nvme_check_mdts(), nvme_check_bounds()
  │
  │  ② PRP 디코딩:
  │     nvme_map_data(n, nlb, req)  → nvme_map_dptr() → nvme_map_prp()
  │
  │  ③ ★ 데이터 방향의 핵심 차이:
  │     data_offset = nvme_l2b(ns, slba)
  │
  │     nvme_blk_write(blk, data_offset, BDRV_SECTOR_SIZE, nvme_rw_cb, req)
  │       │
  │       │  if (req->sg.flags & NVME_SG_DMA):
  │       │    dma_blk_write(blk, &req->sg.qsg, offset, ...)
  │       │      → guest 물리 메모리(PRP 주소)에서 데이터를 읽어
  │       │        backing 파일에 기록
  │       │  else:
  │       │    blk_aio_pwritev(blk, offset, &req->sg.iov, ...)
  │       │
  │       │  ★ Read는 "파일→guest메모리", Write는 "guest메모리→파일"
  │       │
  │       │  데이터 흐름 비교:
  │       │  Read:  host file ──읽기──▶ guest 메모리(PRP1)
  │       │  Write: guest 메모리(PRP1) ──읽기──▶ host file
  │       │
  ▼
  return NVME_NO_COMPLETE  (비동기)
```

### Phase 5.5: Flush 관련 처리

```
Write 후 데이터 영속성이 필요한 경우, 별도의 Flush 커맨드가 발행된다:

fio에서 fdatasync() 또는 REQ_FUA 설정 시:

1. REQ_FUA가 설정된 Write:
   - NVMe 커맨드의 control 필드에 FUA bit 설정
   - 디바이스는 volatile write cache를 우회하여 직접 미디어에 기록
   - QEMU에서는 사실상 차이 없음 (host I/O 완료 = 데이터 기록 완료)

2. 별도 Flush 커맨드 (fdatasync 호출 시):
   REQ_OP_FLUSH → nvme_setup_flush() → opcode = nvme_cmd_flush (0x00)
   → QEMU: nvme_flush() → blk_aio_flush()
   → host의 fsync() 호출

Flush 커맨드의 QEMU 처리 경로:
  nvme_io_cmd()
    │  req->cmd.opcode == NVME_CMD_FLUSH
    │
    ▼
  nvme_flush(n, req)
    └─ req->aiocb = blk_aio_flush(blk, nvme_misc_cb, req)
       → host backing 파일에 fsync 수행
       → 콜백에서 CQE 생성 → 인터럽트
```

### Phase 6-7: Completion (Read와 동일)

```
nvme_rw_cb() → nvme_enqueue_req_completion() → nvme_post_cqes()
  → pci_dma_write(CQE to guest CQ) → nvme_irq_assert() → msix_notify()

[Guest]
nvme_irq() → nvme_poll_cq() → nvme_handle_cqe()
  → blk_mq_complete_request() → bio_endio()
  → pwrite() 반환 (4096 반환)

★ Write 완료의 의미:
  - QEMU 환경: host 파일에 데이터가 기록됨 (host cache에 있을 수 있음)
  - FUA가 아닌 경우: 실제 미디어까지 도달했다고 보장하지 않음
  - Flush가 필요한 경우 별도 Flush 커맨드를 발행해야 함
```

---

## 4. 각 단계에서의 데이터 변환 추적

하나의 4KB Read I/O가 각 계층을 통과하며 어떤 데이터 구조로 변환되는지 추적한다.

```
┌─────────────────────────────────────────────────────────────────────┐
│ Layer 1: Application                                                │
│                                                                     │
│   fio buffer: user virtual addr 0x7f1234000000                     │
│   size: 4096 bytes                                                  │
│   offset: 0 (파일의 시작)                                           │
│                                                                     │
│   pread(fd, 0x7f1234000000, 4096, 0)                               │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ syscall → VFS
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Layer 2: VFS / Direct I/O                                           │
│                                                                     │
│   struct iov_iter {                                                 │
│     .type = ITER_UBUF                                               │
│     .ubuf = 0x7f1234000000  (user virtual address)                 │
│     .count = 4096                                                   │
│   }                                                                 │
│                                                                     │
│   get_user_pages_fast(0x7f1234000000, 1, 0, &pages)                │
│     → pages[0] = struct page * (물리 페이지 디스크립터)             │
│     → 이 페이지는 pin되어 I/O 완료 전까지 swap out 불가             │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ bio 생성
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Layer 3: Block Layer - bio                                          │
│                                                                     │
│   struct bio {                                                      │
│     .bi_opf     = REQ_OP_READ                                      │
│     .bi_iter = {                                                    │
│       .bi_sector = 0        (512바이트 섹터 단위: offset/512)       │
│       .bi_size   = 4096     (바이트)                                │
│     }                                                               │
│     .bi_io_vec[0] = {       (bio_vec: 물리 페이지 + 오프셋)        │
│       .bv_page   = pages[0] (물리 페이지 디스크립터)                │
│       .bv_len    = 4096     (이 세그먼트의 길이)                    │
│       .bv_offset = 0        (페이지 내 오프셋)                      │
│     }                                                               │
│     .bi_vcnt = 1            (bio_vec 개수)                          │
│   }                                                                 │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ bio → request 변환 + tag 할당
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Layer 4: Block Layer - request                                      │
│                                                                     │
│   struct request {                                                  │
│     .__sector    = 0         (시작 섹터)                             │
│     .__data_len  = 4096      (총 데이터 크기)                       │
│     .tag         = 42        (예시: sbitmap에서 할당된 tag)          │
│     .bio         = &bio      (bio 체인의 첫 번째)                   │
│     .mq_hctx     = hctx      (CPU에 매핑된 HW 큐)                  │
│     .rq_flags    = 0                                                │
│     .cmd_flags   = REQ_OP_READ                                     │
│   }                                                                 │
│                                                                     │
│   nvme_iod = blk_mq_rq_to_pdu(req)  (request 뒤에 붙은 드라이버 데이터) │
│   nvme_iod.cmd = 빌드될 NVMe 커맨드                                │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ nvme_setup_rw() + nvme_map_data()
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Layer 5: NVMe Driver - NVMe Command (SQE, 64 bytes)                │
│                                                                     │
│   struct nvme_rw_command {      (64바이트, SQ에 기록됨)             │
│     .opcode     = 0x02          (NVMe Read)                        │
│     .flags      = 0x00                                              │
│     .command_id = 42            (= request tag)                    │
│     .nsid       = 1                                                 │
│     .metadata   = 0                                                 │
│     .prp1       = 0x1A3F5000   (DMA 주소: guest 물리 주소)         │
│     .prp2       = 0            (4KB이면 불필요)                     │
│     .slba       = 0            (시작 LBA: sector 0 / 8 = LBA 0)   │
│     .length     = 0            (NLB: 1블록 - 1 = 0)               │
│     .control    = 0                                                 │
│     .dsmgmt     = 0                                                 │
│     .reftag     = 0                                                 │
│   }                                                                 │
│                                                                     │
│   DMA 주소 생성 과정:                                               │
│     bio_vec.bv_page → page_to_phys() → 물리 주소                   │
│     → dma_map_page() → DMA 주소 (= guest 물리 주소, QEMU에서)     │
│     → PRP1에 설정                                                   │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ MMIO doorbell write → VM-Exit
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Layer 6: QEMU NVMe Device                                          │
│                                                                     │
│   nvme_process_sq():                                                │
│     nvme_addr_read(n, sq_addr, &cmd, 64)                           │
│       → guest 물리 메모리에서 64바이트 SQE를 읽음                   │
│       → cmd = NvmeCmd 구조체                                        │
│                                                                     │
│   nvme_read():                                                      │
│     nvme_map_dptr() → nvme_map_prp():                              │
│       PRP1 (0x1A3F5000) → guest 물리 주소                          │
│       → address_space_map()으로 host 가상 주소 획득                 │
│       → QEMUSGList 또는 QEMUIOVector에 추가                        │
│                                                                     │
│   dma_blk_read(blk, &qsg, offset=0, ...):                         │
│     offset = nvme_l2b(ns, slba) = LBA 0 × 4096 = 0                │
│     → backing 파일에서 offset 0부터 4096바이트 읽기                 │
│     → 읽은 데이터를 guest 물리 주소 0x1A3F5000에 기록              │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ host file I/O
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Layer 7: Host Block I/O                                             │
│                                                                     │
│   QEMU backing file: /var/lib/libvirt/images/disk.qcow2            │
│   또는 raw: /dev/sda                                                │
│                                                                     │
│   blk_aio_preadv(blk, 0, &iov, 0, cb, req)                        │
│     → host kernel의 AIO 또는 io_uring을 통해                       │
│       backing 파일에서 데이터를 읽어 iov에 기록                     │
│     → iov는 guest 물리 메모리에 매핑된 host 가상 주소를 가리킴     │
│     → 따라서 host에서 읽은 데이터가 직접 guest 메모리에 들어감      │
└─────────────────────────────────────────────────────────────────────┘
```

### 데이터 구조 변환 요약 표

| 계층 | 데이터 구조 | 핵심 필드 | 크기 |
|------|------------|-----------|------|
| Application | user buffer | virtual addr, size | 4096B |
| VFS | `struct iov_iter` | ubuf, count | - |
| Block (bio) | `struct bio` | bi_sector, bi_size, bi_io_vec | - |
| Block (request) | `struct request` | __sector, __data_len, tag | - |
| NVMe Driver | `struct nvme_rw_command` | opcode, slba, nlb, prp1, cid | 64B |
| QEMU | `NvmeCmd` + `QEMUSGList` | 위와 동일 + host 매핑 | 64B |
| Host I/O | `struct iovec` | host vaddr, len | - |

---

## 5. 주소 변환 추적

NVMe I/O에서 가장 혼란스러운 부분은 데이터 버퍼의 주소가 여러 번 변환된다는 것이다. 하나의 4KB 버퍼가 거치는 모든 주소 변환을 추적한다.

### 5.1 주소 변환 전체 흐름

```
┌──────────────────────────────────────────────────────────────────────┐
│                       주소 변환 체인                                  │
│                                                                      │
│  ① User Virtual Address                                             │
│     0x7f1234000000                                                   │
│     (fio의 malloc/mmap으로 할당된 버퍼)                              │
│           │                                                          │
│           │ get_user_pages_fast()                                    │
│           ▼                                                          │
│  ② struct page *                                                    │
│     (커널의 물리 페이지 디스크립터)                                   │
│     page->flags, page->_refcount 등                                  │
│           │                                                          │
│           │ page_to_phys(page)                                       │
│           ▼                                                          │
│  ③ Guest Physical Address                                           │
│     0x1A3F5000                                                       │
│     (guest VM의 물리 주소)                                           │
│           │                                                          │
│           │ dma_map_page() / dma_map_sg()                            │
│           ▼                                                          │
│  ④ DMA Address (= Bus Address)                                     │
│     0x1A3F5000                                                       │
│     (QEMU VM에서는 보통 guest 물리 = DMA 주소)                       │
│     (실제 HW에서는 IOMMU가 변환할 수 있음)                           │
│           │                                                          │
│           │ NVMe 커맨드의 PRP1 필드에 설정                           │
│           ▼                                                          │
│  ⑤ PRP Entry in NVMe Command                                       │
│     cmd->rw.prp1 = 0x1A3F5000                                      │
│     (SQE의 Byte 32-39에 저장됨)                                     │
│           │                                                          │
│           │ [VM-Exit → QEMU]                                         │
│           │ nvme_process_sq()에서 SQE 읽기                           │
│           │ nvme_map_prp()에서 PRP 디코딩                            │
│           ▼                                                          │
│  ⑥ QEMU: Guest Physical → Host Virtual                             │
│     address_space_map(as, 0x1A3F5000, &len, ...)                    │
│     또는 dma_memory_map(as, 0x1A3F5000, &len, ...)                  │
│     → Host Virtual Address: 0x7f9876540000                          │
│     (QEMU 프로세스의 가상 주소 공간에서 guest RAM에 해당하는 영역)   │
│           │                                                          │
│           │ blk_aio_preadv()의 iov에 사용                            │
│           ▼                                                          │
│  ⑦ Host I/O: iovec {base=0x7f9876540000, len=4096}                 │
│     → host kernel이 이 주소에 파일 데이터를 기록                     │
│     → 이 주소가 곧 guest 물리 메모리이므로                           │
│       guest에서 해당 물리 페이지를 읽으면 데이터가 있음              │
└──────────────────────────────────────────────────────────────────────┘
```

### 5.2 주소 변환 요약 표

| 단계 | 주소 유형 | 예시 값 | 변환 함수 | 설명 |
|------|----------|---------|-----------|------|
| ① | User Virtual | 0x7f1234000000 | - | fio 프로세스의 가상 주소 |
| ② | struct page * | (커널 포인터) | get_user_pages_fast() | 물리 페이지를 pin |
| ③ | Guest Physical | 0x1A3F5000 | page_to_phys() | guest의 물리 주소 |
| ④ | DMA Address | 0x1A3F5000 | dma_map_page() | 버스 주소 (VM에서는 ③과 동일) |
| ⑤ | PRP Entry | cmd->prp1 | cpu_to_le64() | NVMe SQE에 기록 |
| ⑥ | Host Virtual | 0x7f9876540000 | address_space_map() | QEMU가 guest RAM 접근 |
| ⑦ | Host iovec | {base, len} | - | host I/O에 사용 |

### 5.3 주소 변환 다이어그램 (ASCII)

```
Guest Process         Guest Kernel          NVMe SQE         QEMU            Host
Memory Space          Physical Memory       (PCIe)           Process         Kernel

┌──────────┐
│ User     │
│ Buffer   │
│ 0x7f12.. │──┐
└──────────┘  │
              │ get_user_pages()
              │ (페이지 테이블 워크)
              ▼
         ┌──────────┐
         │ Physical │
         │ Page     │
         │ 0x1A3F5  │──┐
         │ 000      │  │
         └──────────┘  │
                       │ dma_map_page()
                       │ (IOMMU 또는 identity mapping)
                       ▼
                  ┌──────────┐
                  │ DMA addr │
                  │ 0x1A3F5  │──┐
                  │ 000      │  │
                  └──────────┘  │
                                │ PRP1에 기록
                                ▼
                           ┌──────────┐
                           │ PRP1     │
                           │ 0x1A3F5  │──────┐
                           │ 000      │      │
                           └──────────┘      │
                                             │ address_space_map()
                                             │ (GPA → HVA)
                                             ▼
                                        ┌──────────┐
                                        │ Host VA  │
                                        │ 0x7f98.. │──┐
                                        └──────────┘  │
                                                       │ preadv()
                                                       ▼
                                                  ┌──────────┐
                                                  │ Host     │
                                                  │ File I/O │
                                                  │ offset=0 │
                                                  └──────────┘
```

### 5.4 실제 하드웨어에서의 차이

| 항목 | QEMU VM | 실제 하드웨어 |
|------|---------|--------------|
| Guest Physical → DMA | identity mapping (동일) | IOMMU(VT-d)가 변환할 수 있음 |
| DMA → 실제 메모리 접근 | QEMU가 소프트웨어로 처리 | PCIe 버스를 통해 DDR에 직접 DMA |
| PRP 주소 해석 | address_space_map() | NVMe 컨트롤러의 DMA 엔진이 PCIe TLP 생성 |
| IOMMU 존재 | 없음 (pass-through) | 있으면 DMA주소→물리주소 추가 변환 |

### 5.5 다중 페이지 I/O의 PRP 리스트

4KB보다 큰 I/O에서는 PRP 리스트가 필요하다.

```
예: 16KB Read (4개 페이지)

NVMe SQE:
  PRP1 = 0x1A3F5000  (첫 번째 4KB 페이지)
  PRP2 = 0x2B000000  (PRP 리스트의 DMA 주소를 가리킴)

PRP 리스트 (PRP2가 가리키는 메모리):
  ┌─────────────────────┐
  │ Entry 0: 0x1A3F6000 │ → 두 번째 4KB 페이지
  │ Entry 1: 0x1A3F7000 │ → 세 번째 4KB 페이지
  │ Entry 2: 0x1A3F8000 │ → 네 번째 4KB 페이지
  └─────────────────────┘

커널 (nvme_pci_setup_data_prp):
  - PRP 리스트용 메모리를 dma_pool에서 할당
  - 각 bio_vec의 DMA 주소를 PRP 리스트에 기록
  - PRP2에 PRP 리스트의 DMA 주소를 설정

QEMU (nvme_map_prp):
  - PRP1에서 첫 번째 페이지 매핑
  - PRP2에서 PRP 리스트 읽기 (guest 메모리에서 DMA read)
  - 각 PRP 엔트리를 순회하며 scatter-gather 리스트 구성
```

---

## 6. 타이밍 다이어그램

### 6.1 Read I/O 타이밍

```
시간 →

fio        │ VFS/Bio    │ blk-mq      │ NVMe drv     │ PCIe/KVM    │ QEMU NVMe    │ Host I/O
───────────┼────────────┼─────────────┼──────────────┼─────────────┼──────────────┼──────────
           │            │             │              │             │              │
pread() ──▶│            │             │              │             │              │
           │            │             │              │             │              │
 [블록됨]  │__blkdev_   │             │              │             │              │
           │direct_IO() │             │              │             │              │
           │bio 생성   ─┼──▶          │              │             │              │
           │            │submit_bio() │              │             │              │
           │            │             │              │             │              │
           │            │blk_mq_      │              │             │              │
           │            │submit_bio() │              │             │              │
           │            │  tag 할당   │              │             │              │
           │            │  bio→req    │              │             │              │
           │            │ ────────────┼──▶           │             │              │
           │            │             │nvme_queue_rq │             │              │
           │            │             │ nvme_prep_rq │             │              │
           │            │             │  setup_cmd   │             │              │
           │            │             │  map_data    │             │              │
           │            │             │ sq_copy_cmd  │             │              │
           │            │             │ write_sq_db  │             │              │
           │            │             │  writel() ───┼──▶          │              │
           │            │             │              │ VM-Exit     │              │
           │            │             │              │ ────────────┼──▶           │
           │            │             │              │             │mmio_write()  │
           │            │             │              │             │process_db()  │
           │            │             │              │             │ sq->tail=new │
           │            │             │              │             │ bh_schedule  │
           │            │             │              │             │              │
[pread()   │            │             │              │ VM-Enter    │              │
 에서 대기 │            │             │              │◀────────────┤              │
 또는 다른 │            │             │              │             │              │
 작업 수행]│            │             │              │             │              │
           │            │             │              │             │              │
           │            │             │              │             │[QEMU 이벤트  │
           │            │             │              │             │ 루프에서]     │
           │            │             │              │             │process_sq()  │
           │            │             │              │             │ SQE DMA read │
           │            │             │              │             │ nvme_io_cmd  │
           │            │             │              │             │ nvme_read()  │
           │            │             │              │             │ map_prp()    │
           │            │             │              │             │ blk_read() ──┼──▶
           │            │             │              │             │              │ preadv()
           │            │             │              │             │              │ [host I/O
           │            │             │              │             │              │  진행중]
           │            │             │              │             │              │ ...
           │            │             │              │             │              │ 완료!
           │            │             │              │             │◀─────────────┼─ callback
           │            │             │              │             │nvme_rw_cb()  │
           │            │             │              │             │enqueue_      │
           │            │             │              │             │ completion() │
           │            │             │              │             │post_cqes()   │
           │            │             │              │             │ CQE DMA write│
           │            │             │              │             │ phase bit set│
           │            │             │              │             │irq_assert()  │
           │            │             │              │             │ msix_notify()│
           │            │             │              │ KVM 인터럽트│◀─────────────┤
           │            │             │              │ 주입        │              │
           │            │             │              │────────────▶│              │
           │            │             │◀─────────────┤             │              │
           │            │             │nvme_irq()    │             │              │
           │            │             │ poll_cq()    │             │              │
           │            │             │ handle_cqe() │             │              │
           │            │             │ tag→req 매칭 │             │              │
           │            │             │ unmap_data() │             │              │
           │            │◀────────────┤              │             │              │
           │            │blk_mq_      │              │             │              │
           │            │complete_req │              │             │              │
           │◀───────────┤bio_endio()  │              │             │              │
           │            │wake_up()    │              │             │              │
◀──────────┤            │             │              │             │              │
pread()    │            │             │              │             │              │
리턴       │            │             │              │             │              │
(buf에     │            │             │              │             │              │
 데이터)   │            │             │              │             │              │
```

### 6.2 Write I/O 타이밍

```
시간 →

fio        │ VFS/Bio    │ blk-mq      │ NVMe drv     │ PCIe/KVM    │ QEMU NVMe    │ Host I/O
───────────┼────────────┼─────────────┼──────────────┼─────────────┼──────────────┼──────────
           │            │             │              │             │              │
pwrite()──▶│            │             │              │             │              │
           │            │             │              │             │              │
 [블록됨]  │__blkdev_   │             │              │             │              │
           │direct_IO() │             │              │             │              │
           │bio 생성    │             │              │             │              │
           │(WRITE)  ───┼──▶          │              │             │              │
           │            │submit_bio() │              │             │              │
           │            │             │              │             │              │
           │            │blk_mq_      │              │             │              │
           │            │submit_bio() │              │             │              │
           │            │ ────────────┼──▶           │             │              │
           │            │             │nvme_queue_rq │             │              │
           │            │             │ opcode=WRITE │             │              │
           │            │             │ PRP1=데이터  │             │              │
           │            │             │  버퍼 DMA주소│             │              │
           │            │             │ writel() ────┼──▶          │              │
           │            │             │              │ VM-Exit     │              │
           │            │             │              │─────────────┼──▶           │
           │            │             │              │             │mmio_write()  │
           │            │             │              │             │process_db()  │
           │            │             │              │ VM-Enter    │              │
           │            │             │              │◀────────────┤              │
           │            │             │              │             │              │
           │            │             │              │             │process_sq()  │
           │            │             │              │             │ SQE DMA read │
           │            │             │              │             │nvme_write()  │
           │            │             │              │             │ map_prp():   │
           │            │             │              │             │  PRP→guest   │
           │            │             │              │             │  메모리에서  │
           │            │             │              │             │  데이터 읽기 │
           │            │             │              │             │ blk_write()──┼──▶
           │            │             │              │             │              │ pwritev()
           │            │             │              │             │              │ [데이터를
           │            │             │              │             │              │  host file
           │            │             │              │             │              │  에 기록]
           │            │             │              │             │              │ ...
           │            │             │              │             │              │ 완료!
           │            │             │              │             │◀─────────────┼─callback
           │            │             │              │             │nvme_rw_cb()  │
           │            │             │              │             │post_cqes()   │
           │            │             │              │             │msix_notify() │
           │            │             │              │ 인터럽트    │              │
           │            │             │◀─────────────┤ 주입        │              │
           │            │             │nvme_irq()    │             │              │
           │            │             │ CQE 처리     │             │              │
           │            │◀────────────┤              │             │              │
           │◀───────────┤bio_endio()  │              │             │              │
◀──────────┤wake_up()   │             │              │             │              │
pwrite()   │            │             │              │             │              │
리턴 (4096)│            │             │              │             │              │
```

### 6.3 Read vs Write 데이터 방향 비교

```
★ Read I/O - 데이터 흐름:

  Host File ─────preadv()────▶ QEMU buffer ────DMA write────▶ Guest 물리 메모리
                                                                (PRP1 주소)
                                                                    │
                                                              (= pinned page)
                                                                    │
                                                              User Buffer에
                                                              이미 매핑됨

  데이터 이동: Host file → guest 메모리 → user buffer (zero-copy에 가까움)


★ Write I/O - 데이터 흐름:

  User Buffer ──(이미 pinned)──▶ Guest 물리 메모리 ──DMA read──▶ QEMU buffer
   (PRP1 주소)                                                       │
                                                               pwritev()
                                                                     │
                                                                     ▼
                                                               Host File

  데이터 이동: user buffer → guest 메모리 → host file

★ 핵심: Read에서 "DMA write"는 디바이스가 메모리에 쓰는 것이고,
        Write에서 "DMA read"는 디바이스가 메모리를 읽는 것이다.
        NVMe 관점에서 "Read" = 디바이스→호스트, "Write" = 호스트→디바이스.
```

---

## 7. QEMU와 실제 하드웨어의 차이

### 7.1 아키텍처 비교

```
┌─────────────────────────────────────────────────────────────────┐
│                    QEMU NVMe 에뮬레이션                          │
│                                                                  │
│  Guest NVMe Driver                                               │
│       │ writel(doorbell)                                         │
│       ▼                                                          │
│  [KVM VM-Exit] ─────▶ QEMU 프로세스                             │
│                         │                                        │
│                    nvme_mmio_write()                              │
│                    nvme_process_sq()                              │
│                    nvme_read/write()                              │
│                         │                                        │
│                    blk_aio_preadv/pwritev()                      │
│                         │                                        │
│                    Host Filesystem                                │
│                         │                                        │
│                    Host Block Layer                               │
│                         │                                        │
│                    Host NVMe/SATA/etc.                            │
│                         │                                        │
│                    실제 저장 매체 (HDD/SSD)                       │
└─────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────┐
│                    실제 NVMe SSD                                 │
│                                                                  │
│  Host NVMe Driver                                                │
│       │ writel(doorbell)                                         │
│       ▼                                                          │
│  PCIe Bus (TLP 패킷)                                            │
│       │                                                          │
│       ▼                                                          │
│  ┌──────────────────────────────────────────────┐                │
│  │          NVMe SSD Controller                  │                │
│  │                                               │                │
│  │  ┌────────────┐  ┌───────────────────────┐   │                │
│  │  │ ARM/RISC-V │  │ DMA Engine            │   │                │
│  │  │ Firmware   │  │ (PCIe Requester)      │   │                │
│  │  │ CPU        │  │                       │   │                │
│  │  └────────────┘  └───────────────────────┘   │                │
│  │                                               │                │
│  │  ┌────────────┐  ┌───────────────────────┐   │                │
│  │  │ FTL        │  │ ECC Engine            │   │                │
│  │  │ (Flash     │  │ (BCH/LDPC)            │   │                │
│  │  │ Translation│  │                       │   │                │
│  │  │ Layer)     │  │                       │   │                │
│  │  └────────────┘  └───────────────────────┘   │                │
│  │                                               │                │
│  │  ┌───────────────────────────────────────┐   │                │
│  │  │ NAND Flash Array                      │   │                │
│  │  │ ┌────┐ ┌────┐ ┌────┐ ┌────┐          │   │                │
│  │  │ │Die │ │Die │ │Die │ │Die │ ...       │   │                │
│  │  │ │ 0  │ │ 1  │ │ 2  │ │ 3  │          │   │                │
│  │  │ └────┘ └────┘ └────┘ └────┘          │   │                │
│  │  └───────────────────────────────────────┘   │                │
│  └──────────────────────────────────────────────┘                │
└─────────────────────────────────────────────────────────────────┘
```

### 7.2 상세 차이점

| 항목 | QEMU NVMe | 실제 NVMe SSD |
|------|-----------|---------------|
| **Doorbell 처리** | VM-Exit → 소프트웨어 처리 (수 us) | PCIe MMIO write → HW 즉시 인식 (수십 ns) |
| **SQE Fetch** | pci_dma_read(): guest RAM에서 memcpy | DMA 엔진이 PCIe TLP로 host 메모리에서 fetch |
| **데이터 전송** | host file I/O (수 us ~ 수 ms) | NAND read/write (수 us ~ 수백 us) |
| **CQE 기록** | pci_dma_write(): guest RAM에 memcpy | DMA 엔진이 PCIe TLP로 host 메모리에 write |
| **인터럽트** | KVM에 MSI-X 주입 요청 | PCIe MSI-X: 메모리에 write → LAPIC 인터럽트 |
| **FTL** | 없음 (단순 오프셋 매핑) | L2P 테이블, GC, wear leveling, bad block 관리 |
| **ECC** | 없음 | BCH/LDPC 인코딩/디코딩 |
| **Wear Leveling** | 없음 (이미지 파일은 마모 없음) | NAND 셀 수명 관리 |
| **Write Amplification** | 없음 | GC로 인한 추가 쓰기 |
| **캐시** | host page cache가 대신 작동 | DRAM 캐시 (write buffer) |
| **전원 손실 보호** | 없음 (QEMU 종료 = 데이터 손실 가능) | 커패시터로 DRAM → NAND flush |
| **병렬성** | 순차적 host I/O | 다수 NAND die에 병렬 접근 |
| **레이턴시** | host I/O 레이턴시 + VM-Exit 오버헤드 | NAND 레이턴시 (read ~50us, write ~200us) |

### 7.3 실제 NVMe SSD에서의 Read 경로 (QEMU와 비교)

```
실제 SSD에서 4KB Read가 처리되는 과정:

1. Host CPU: writel(sq_tail, doorbell)
   → PCIe MMIO write TLP 생성 → NVMe 컨트롤러가 수신

2. NVMe 컨트롤러 펌웨어:
   - SQ tail 업데이트 인지
   - DMA 엔진에 SQE fetch 명령
   → PCIe Memory Read TLP 생성 → Host 메모리에서 64B SQE 가져옴

3. 펌웨어가 SQE 파싱:
   - opcode = Read, SLBA, NLB 확인
   - FTL(Flash Translation Layer) 참조:
     Logical LBA → Physical NAND 주소 변환
     (die, plane, block, page 결정)

4. NAND 읽기:
   - 해당 die/plane/block/page에 Read 명령 전송
   - NAND 셀에서 데이터를 페이지 버퍼로 전송 (~25-50us)
   - ECC 디코딩 (BCH/LDPC): 비트 에러 정정
   - 컨트롤러 SRAM/DRAM에 데이터 임시 저장

5. DMA로 Host 메모리에 데이터 전송:
   - PRP1 주소로 PCIe Memory Write TLP 생성
   - 4KB 데이터를 여러 TLP로 분할하여 전송
     (PCIe Gen3: Max Payload Size 128B~256B → 16~32개 TLP)

6. CQE 생성 + 인터럽트:
   - CQ 주소에 16B CQE를 DMA write
   - MSI-X: 지정된 주소에 메모리 write → LAPIC가 인터럽트 발생
   → 커널의 nvme_irq() 호출

QEMU와의 핵심 차이:
- QEMU: "host file read → memcpy to guest RAM" (소프트웨어)
- 실제: "NAND read → PCIe DMA to host RAM" (하드웨어)
- QEMU: FTL 없이 단순 바이트 오프셋으로 접근
- 실제: LBA → PBA 변환이 필수 (FTL의 핵심 역할)
```

### 7.4 QEMU 에뮬레이션의 한계와 유용한 점

**한계:**

1. **성능 측정 불가**: QEMU의 I/O 레이턴시는 실제 SSD와 전혀 다르다. VM-Exit 오버헤드, host I/O 레이턴시가 지배적이므로 성능 벤치마크 결과는 의미가 없다.

2. **FTL 동작 없음**: GC(Garbage Collection), wear leveling, write amplification 등 실제 SSD의 핵심 동작을 관찰할 수 없다.

3. **병렬성 없음**: 실제 SSD는 다수의 NAND die에 병렬 접근하여 높은 대역폭을 달성하지만, QEMU는 순차적 host I/O만 수행한다.

4. **에러 처리 제한**: NAND 비트 에러, uncorrectable error 등 실제 하드웨어 에러 시나리오를 테스트하기 어렵다.

5. **인터럽트 레이턴시**: KVM을 통한 인터럽트 주입은 실제 MSI-X보다 오버헤드가 크다.

**유용한 점:**

1. **프로토콜 검증**: NVMe 스펙의 커맨드 포맷, 큐잉 메커니즘, 완료 처리 등 프로토콜 레벨의 동작을 정확하게 에뮬레이션한다. 드라이버 개발과 디버깅에 이상적이다.

2. **코드 분석 학습**: 드라이버와 디바이스 양쪽 코드를 모두 읽을 수 있어 end-to-end 이해에 최적이다. 실제 SSD 펌웨어는 공개되지 않는다.

3. **기능 테스트**: Namespace 관리, ZNS(Zoned Namespaces), 멀티 큐 등 NVMe 기능을 실제 하드웨어 없이 테스트할 수 있다.

4. **디버깅 용이**: QEMU에 브레이크포인트를 걸어 디바이스의 모든 내부 상태를 관찰할 수 있다. 실제 SSD에서는 불가능한 일이다.

5. **비용 절약**: 다양한 NVMe 설정(큐 수, 네임스페이스 수, 용량 등)을 소프트웨어로 자유롭게 변경할 수 있다.

6. **안전한 실험**: 잘못된 드라이버 코드가 실제 하드웨어를 손상시킬 위험이 없다. 데이터 손실 없이 에러 경로를 테스트할 수 있다.

---

## 8. 전체 함수 호출 체인 요약

### 8.1 Read 전체 체인 (한눈에)

```
[Application]
fio: pread(fd, buf, 4096, offset)

[VFS]
→ sys_pread64()
→ vfs_read()
→ blkdev_read_iter()
→ __blkdev_direct_IO()
  → bio_alloc() + bio_add_page()
  → submit_bio()

[Block Layer]
→ submit_bio_noacct()
→ __submit_bio() / blk_mq_submit_bio()
  → __bio_split_to_limits()
  → blk_mq_get_new_requests() [tag 할당]
  → blk_mq_bio_to_request()
  → blk_mq_try_issue_directly() 또는 plug flush

[NVMe Driver]
→ nvme_queue_rq()
  → nvme_prep_rq()
    → nvme_setup_cmd() → nvme_setup_rw(nvme_cmd_read)
    → nvme_map_data() → dma_map_page() [PRP 설정]
    → nvme_start_request()
  → nvme_sq_copy_cmd() [SQ에 64B 기록]
  → nvme_write_sq_db() → writel(tail, q_db) [Doorbell!]

[VM-Exit → QEMU]
→ nvme_mmio_write(addr=doorbell, data=new_tail)
→ nvme_process_db() → sq->tail = new_tail
→ qemu_bh_schedule(sq->bh)
→ nvme_process_sq()
  → nvme_addr_read() [SQE fetch from guest memory]
  → nvme_io_cmd() → nvme_read()
    → nvme_map_data() → nvme_map_dptr() → nvme_map_prp()
    → nvme_blk_read() → dma_blk_read() / blk_aio_preadv()

[Host I/O]
→ host kernel: preadv() on backing file → 데이터 읽기 완료

[Completion Path - QEMU]
→ nvme_rw_cb() → nvme_rw_complete_cb()
→ nvme_enqueue_req_completion()
→ nvme_post_cqes()
  → pci_dma_write() [CQE를 guest CQ에 기록]
  → nvme_irq_assert() → msix_notify()

[Completion Path - Guest Kernel]
→ nvme_irq()
→ nvme_poll_cq()
  → nvme_cqe_pending() [phase bit 확인]
  → dma_rmb()
  → nvme_handle_cqe()
    → nvme_find_rq(tag) [command_id로 request 찾기]
    → nvme_try_complete_req()
    → blk_mq_add_to_batch() 또는 nvme_pci_complete_rq()
  → nvme_update_cq_head()
→ nvme_ring_cq_doorbell() [CQ Head doorbell]

[Completion Path - Block → App]
→ nvme_pci_complete_rq() / nvme_pci_complete_batch()
  → nvme_unmap_data() [DMA unmap]
→ blk_mq_end_request()
  → __blk_mq_end_request()
    → blk_mq_free_request() [tag 반환]
    → bio_endio() → __blkdev_direct_IO 콜백
→ pread() 반환: buf에 데이터 존재
```

### 8.2 Write 전체 체인 (Read와의 차이점만)

```
차이점 1: opcode
  nvme_setup_rw(ns, req, cmd, nvme_cmd_write)  ← nvme_cmd_read 대신

차이점 2: DMA 방향
  nvme_map_data(): DMA_TO_DEVICE              ← DMA_FROM_DEVICE 대신

차이점 3: QEMU에서 데이터 흐름
  nvme_do_write() → nvme_blk_write()
    → dma_blk_write(): guest 메모리에서 읽어 host 파일에 기록
    (Read는 반대: host 파일에서 읽어 guest 메모리에 기록)

차이점 4: FUA 처리
  control |= NVME_RW_FUA (REQ_FUA일 때)

나머지 경로 (doorbell, SQ/CQ, 인터럽트)는 Read와 동일
```

---

## 9. 핵심 데이터 구조 크기 참조

```
NVMe Submission Queue Entry (SQE):   64 bytes
NVMe Completion Queue Entry (CQE):   16 bytes
struct bio:                          ~200 bytes (커널 버전에 따라 다름)
struct request:                      ~400 bytes (+ nvme_iod PDU)
struct nvme_command:                  64 bytes (union of all command types)
struct nvme_completion:               16 bytes

SQ Entry 레이아웃 (64 bytes):
  ┌──────────────────────────────────────────────────────────────────┐
  │ DW0  (Byte 0-3):   opcode(8) + flags(8) + command_id(16)       │
  │ DW1  (Byte 4-7):   nsid                                         │
  │ DW2  (Byte 8-11):  reserved                                     │
  │ DW3  (Byte 12-15): reserved                                     │
  │ DW4  (Byte 16-19): metadata pointer (low)                       │
  │ DW5  (Byte 20-23): metadata pointer (high)                      │
  │ DW6  (Byte 24-27): PRP1 / SGL (low)                             │
  │ DW7  (Byte 28-31): PRP1 / SGL (high)                            │
  │ DW8  (Byte 32-35): PRP2 / SGL (low)                             │
  │ DW9  (Byte 36-39): PRP2 / SGL (high)                            │
  │ DW10 (Byte 40-43): SLBA (low)  [Read/Write]                     │
  │ DW11 (Byte 44-47): SLBA (high) [Read/Write]                     │
  │ DW12 (Byte 48-51): NLB(16) + 기타 [Read/Write]                  │
  │ DW13 (Byte 52-55): DSM 관리                                     │
  │ DW14 (Byte 56-59): ref tag 등                                    │
  │ DW15 (Byte 60-63): reserved                                     │
  └──────────────────────────────────────────────────────────────────┘

CQ Entry 레이아웃 (16 bytes):
  ┌──────────────────────────────────────────────────────────────────┐
  │ DW0  (Byte 0-3):   Command Specific Result                      │
  │ DW1  (Byte 4-7):   Reserved                                     │
  │ DW2  (Byte 8-11):  SQ Head Pointer(16) + SQ ID(16)             │
  │ DW3  (Byte 12-15): Command ID(16) + Status(15) + Phase(1)      │
  └──────────────────────────────────────────────────────────────────┘
```

---

## 10. 소스 파일 위치 인덱스

| 기능 | 파일 | 핵심 함수 |
|------|------|-----------|
| Syscall → Bio | `block/blk-core.c` | `submit_bio()` |
| Bio → Request + 디스패치 | `block/blk-mq.c` | `blk_mq_submit_bio()` |
| Request → NVMe SQE | `drivers/nvme/host/core.c` | `nvme_setup_cmd()`, `nvme_setup_rw()` |
| DMA 매핑 + PRP 설정 | `drivers/nvme/host/pci.c` | `nvme_map_data()`, `nvme_prep_rq()` |
| SQ 기록 + Doorbell | `drivers/nvme/host/pci.c` | `nvme_queue_rq()`, `nvme_sq_copy_cmd()`, `nvme_write_sq_db()` |
| 인터럽트 처리 | `drivers/nvme/host/pci.c` | `nvme_irq()`, `nvme_poll_cq()`, `nvme_handle_cqe()` |
| QEMU Doorbell 수신 | `hw/nvme/ctrl.c` | `nvme_mmio_write()`, `nvme_process_db()` |
| QEMU SQE 처리 | `hw/nvme/ctrl.c` | `nvme_process_sq()`, `nvme_io_cmd()` |
| QEMU Read/Write | `hw/nvme/ctrl.c` | `nvme_read()`, `nvme_do_write()` |
| QEMU PRP 디코딩 | `hw/nvme/ctrl.c` | `nvme_map_dptr()`, `nvme_map_prp()` |
| QEMU 데이터 I/O | `hw/nvme/ctrl.c` | `nvme_blk_read()`, `nvme_blk_write()` |
| QEMU 완료 처리 | `hw/nvme/ctrl.c` | `nvme_rw_cb()`, `nvme_enqueue_req_completion()`, `nvme_post_cqes()` |
| QEMU 인터럽트 발생 | `hw/nvme/ctrl.c` | `nvme_irq_assert()`, `msix_notify()` |
