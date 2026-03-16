# io_submit() → nvme_queue_rq() 전체 코드 플로우

## 전체 호출 체인 요약

```
User Space (libaio)
━━━━━━━━━━━━━━━━━━━
  io_submit(ctx_id, nr, iocbpp)
      │
━━━━━━│━━━━━━━━━━━━━━━━━━━━━━━ syscall ━━━━━━━━━━━━━━━━━━━━━━━━━
      │
Kernel Space
      │
      ▼
  ① SYSCALL_DEFINE3(io_submit)               fs/aio.c:2082
      │
      ▼
  ② io_submit_one(ctx, user_iocb)            fs/aio.c:2023
      │  copy_from_user(&iocb, ...)
      │
      ▼
  ③ __io_submit_one(ctx, &iocb, req)         fs/aio.c:1969
      │  fget(iocb.aio_fildes)  →  struct file
      │  switch(aio_lio_opcode)
      │
      ▼
  ④ aio_read(&req->rw, iocb)                 fs/aio.c:1581
      │  aio_prep_rw()   →  kiocb 설정
      │  file->f_op->read_iter(req, &iter)
      │
      ▼
  ⑤ blkdev_read_iter(iocb, to)               block/fops.c:812
      │  if (IOCB_DIRECT)
      │
      ▼
  ⑥ blkdev_direct_IO(iocb, to)               block/fops.c:396
      │  async kiocb → __blkdev_direct_IO_async()
      │
      ▼
  ⑦ __blkdev_direct_IO_async(iocb, iter)     block/fops.c:322
      │  bio_alloc_bioset()   →  struct bio 할당
      │  submit_bio(bio)
      │
      ▼
  ⑧ submit_bio(bio)                           block/blk-core.c:1244
      │  bio_set_ioprio(bio)
      │  submit_bio_noacct(bio)
      │
      ▼
  ⑨ blk_mq_submit_bio(bio)                   block/blk-mq.c:3697
      │  blk_mq_get_new_requests()  →  struct request 할당
      │  blk_mq_bio_to_request()
      │  디스패치 경로 분기
      │
      ▼
  ⑩ blk_mq_try_issue_directly(hctx, rq)      block/blk-mq.c:3266
      │
      ▼
  ⑪ __blk_mq_issue_directly(hctx, rq)        block/blk-mq.c:3194
      │  q->mq_ops->queue_rq(hctx, &bd)
      │
      ▼
  ⑫ nvme_queue_rq(hctx, bd)                  drivers/nvme/host/pci.c:1675
      │  nvme_prep_rq(req)
      │  │  nvme_setup_cmd()     →  NVMe SQE 구성
      │  │  nvme_map_data()      →  PRP/SGL DMA 매핑
      │  │  nvme_start_request() →  blk_mq_start_request()
      │  │                          → blk_add_timer()  ★ timeout 시작
      │  │
      │  nvme_sq_copy_cmd()      →  SQ tail에 64B command 복사
      │  nvme_write_sq_db()      →  Doorbell register write
      │
      ▼
  NVMe SSD가 SQ에서 command fetch → 실행 → CQ에 완료 기록
```

---

## 단계별 상세 분석

### ① SYSCALL_DEFINE3(io_submit) — fs/aio.c:2082

```c
SYSCALL_DEFINE3(io_submit, aio_context_t, ctx_id, long, nr,
                struct iocb __user * __user *, iocbpp)
{
    struct kioctx *ctx;
    struct blk_plug plug;

    ctx = lookup_ioctx(ctx_id);         // aio_context 조회

    if (nr > AIO_PLUG_THRESHOLD)
        blk_start_plug(&plug);          // IO batching 시작

    for (i = 0; i < nr; i++) {
        get_user(user_iocb, iocbpp + i);
        io_submit_one(ctx, user_iocb);  // ②로 진입
    }

    if (nr > AIO_PLUG_THRESHOLD)
        blk_finish_plug(&plug);         // 모은 IO 한꺼번에 전송
}
```

| 항목 | 값 |
|------|-----|
| 소스 파일 | `fs/aio.c:2082` |
| user → kernel 전환 | `copy_from_user` (②에서 수행) |
| batch 최적화 | `blk_plug` — 여러 IO를 모아서 한 번에 디스패치 |
| timeout 개입 가능성 | **없음** — IO 자체에 대한 처리 아직 안 시작 |

---

### ② io_submit_one() — fs/aio.c:2023

```c
static int io_submit_one(struct kioctx *ctx, struct iocb __user *user_iocb)
{
    struct iocb iocb;

    copy_from_user(&iocb, user_iocb, sizeof(iocb));  // ★ user iocb → kernel 복사

    // reserved 필드 체크 (비어있어야 함)
    if (iocb.aio_reserved2)
        return -EINVAL;

    req = aio_get_req(ctx);              // aio_kiocb 할당

    __io_submit_one(ctx, &iocb, req);    // ③으로 진입
}
```

**user에서 전달되는 struct iocb 구조:**

```c
struct iocb {
    __u64   aio_data;          // user callback data (io_getevents에서 반환)
    __u32   aio_key;           // kernel이 설정 (KIOCB_KEY)
    __u32   aio_rw_flags;      // RWF_DSYNC, RWF_HIPRI 등
    __u16   aio_lio_opcode;    // IOCB_CMD_PREAD / IOCB_CMD_PWRITE
    __s16   aio_reqprio;       // ★ IO 우선순위 (ioprio)
    __u32   aio_fildes;        // file descriptor
    __u64   aio_buf;           // buffer address
    __u64   aio_nbytes;        // size
    __s64   aio_offset;        // file offset
    __u64   aio_reserved2;     // ★ must be 0 — 여기를 쓰면 -EINVAL
    __u32   aio_flags;         // IOCB_FLAG_RESFD | IOCB_FLAG_IOPRIO
    __u32   aio_resfd;         // eventfd for notification
};
```

| 항목 | 값 |
|------|-----|
| user 데이터 복사 | `copy_from_user()` — 64바이트 iocb 구조체 |
| timeout 필드 | **없음** — `aio_reserved2`는 0이어야 함 |
| ioprio 전달 | `aio_reqprio` + `IOCB_FLAG_IOPRIO` 플래그로 가능 |
| timeout 개입 가능성 | **불가** — timeout 필드 자체가 struct iocb에 없음 |

---

### ③ __io_submit_one() — fs/aio.c:1969

```c
static int __io_submit_one(struct kioctx *ctx, const struct iocb *iocb,
                           struct iocb __user *user_iocb, struct aio_kiocb *req)
{
    req->ki_filp = fget(iocb->aio_fildes);  // fd → struct file

    // eventfd 설정 (IOCB_FLAG_RESFD)
    if (iocb->aio_flags & IOCB_FLAG_RESFD)
        req->ki_eventfd = eventfd_ctx_fdget(iocb->aio_resfd);

    switch (iocb->aio_lio_opcode) {
    case IOCB_CMD_PREAD:
        return aio_read(&req->rw, iocb);     // ④로 진입
    case IOCB_CMD_PWRITE:
        return aio_write(&req->rw, iocb);
    }
}
```

| 항목 | 값 |
|------|-----|
| fd → file 변환 | `fget()` — NVMe block device의 struct file 획득 |
| opcode 분기 | `IOCB_CMD_PREAD(0)` / `IOCB_CMD_PWRITE(1)` |
| timeout 개입 가능성 | **없음** |

---

### ④ aio_read() / aio_prep_rw() — fs/aio.c:1510, 1581

```c
static int aio_prep_rw(struct kiocb *req, const struct iocb *iocb, int rw_type)
{
    req->ki_complete = aio_complete_rw;       // 완료 콜백 등록
    req->ki_pos = iocb->aio_offset;           // file offset
    req->ki_flags = req->ki_filp->f_iocb_flags | IOCB_AIO_RW;

    // ★ ioprio 전달 경로
    if (iocb->aio_flags & IOCB_FLAG_IOPRIO) {
        ioprio_check_cap(iocb->aio_reqprio);
        req->ki_ioprio = iocb->aio_reqprio;   // user가 설정한 ioprio
    } else {
        req->ki_ioprio = get_current_ioprio(); // thread의 기본 ioprio
    }
}

static int aio_read(struct kiocb *req, const struct iocb *iocb)
{
    aio_prep_rw(req, iocb, READ);

    file->f_op->read_iter(req, &iter);        // ⑤ VFS 콜백
    // block device인 경우: blkdev_read_iter()
}
```

| 항목 | 값 |
|------|-----|
| ioprio 전달 | `iocb->aio_reqprio` → `kiocb->ki_ioprio` ★ 전달됨 |
| timeout 전달 | **없음** — kiocb에 timeout 필드 없음 |
| VFS 콜백 | `file->f_op->read_iter` = `blkdev_read_iter` |

**struct kiocb (kernel I/O control block) — VFS 레벨 IO 제어 구조체:**

```c
struct kiocb {
    struct file     *ki_filp;        // 대상 file
    loff_t           ki_pos;         // offset
    void           (*ki_complete)(); // 완료 콜백
    int              ki_flags;       // IOCB_DIRECT, IOCB_AIO_RW 등
    u16              ki_ioprio;      // ★ IO 우선순위 — 여기까지 전달됨
    // ... timeout 필드 없음!
};
```

---

### ⑤⑥ blkdev_read_iter() → blkdev_direct_IO() — block/fops.c:812, 396

```c
static ssize_t blkdev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    if (iocb->ki_flags & IOCB_DIRECT) {
        // O_DIRECT로 열었으므로 page cache 우회
        ret = blkdev_direct_IO(iocb, to);    // ⑥
    }
}

static ssize_t blkdev_direct_IO(struct kiocb *iocb, struct iov_iter *iter)
{
    // libaio + async → 이 경로
    return __blkdev_direct_IO_async(iocb, iter, bdev, nr_pages);  // ⑦
}
```

| 항목 | 값 |
|------|-----|
| O_DIRECT 체크 | `IOCB_DIRECT` 플래그 — libaio는 반드시 O_DIRECT |
| 경로 선택 | async kiocb → `__blkdev_direct_IO_async()` |
| timeout 개입 가능성 | **없음** |

---

### ⑦ __blkdev_direct_IO_async() — block/fops.c:322

```c
static ssize_t __blkdev_direct_IO_async(struct kiocb *iocb,
                                         struct iov_iter *iter,
                                         struct block_device *bdev,
                                         unsigned int nr_pages)
{
    // ★★ struct bio 할당 — 이것이 block layer의 IO 단위
    bio = bio_alloc_bioset(bdev, nr_pages, opf, GFP_KERNEL, &blkdev_dio_pool);

    dio = container_of(bio, struct blkdev_dio, bio);
    dio->iocb = iocb;                                    // kiocb 연결

    bio->bi_iter.bi_sector = pos >> SECTOR_SHIFT;         // LBA 설정
    bio->bi_end_io = blkdev_bio_end_io_async;             // 완료 콜백
    bio->bi_ioprio = iocb->ki_ioprio;                     // ★ ioprio 전달!

    submit_bio(bio);                                      // ⑧로 진입
    return -EIOCBQUEUED;                                  // "비동기 처리 중"
}
```

**kiocb → bio 전달되는 필드들:**

| kiocb 필드 | bio 필드 | 전달 여부 |
|-----------|----------|:---------:|
| `ki_pos` | `bi_iter.bi_sector` | O |
| `ki_ioprio` | `bi_ioprio` | **O** |
| `ki_write_stream` | `bi_write_stream` | O |
| `ki_flags (IOCB_NOWAIT)` | `bi_opf \|= REQ_NOWAIT` | O |
| `ki_flags (IOCB_HIPRI)` | `bi_opf \|= REQ_POLLED` | O |
| timeout??? | ??? | **X — 전달 경로 없음!** |

---

### ⑧ submit_bio() — block/blk-core.c:1244

```c
void submit_bio(struct bio *bio)
{
    if (bio_op(bio) == REQ_OP_READ)
        task_io_account_read(bio->bi_iter.bi_size);  // IO 통계

    bio_set_ioprio(bio);        // ★ thread의 ioprio가 bio에 없으면 여기서 설정
    submit_bio_noacct(bio);     // 디바이스 드라이버로 전달
}
```

```c
// bio_set_ioprio() 내부:
static inline void bio_set_ioprio(struct bio *bio)
{
    if (!bio->bi_ioprio)                    // ⑦에서 이미 설정했으면 skip
        bio->bi_ioprio = get_current_ioprio();  // 아니면 thread ioprio 사용
}
```

| 항목 | 값 |
|------|-----|
| ioprio 최종 결정 | `bio->bi_ioprio` (④에서 user가 설정한 값 또는 thread 기본값) |
| timeout 개입 가능성 | **없음** — bio에 timeout 필드 없음 |

---

### ⑨ blk_mq_submit_bio() — block/blk-mq.c:3697

```c
void blk_mq_submit_bio(struct bio *bio)
{
    struct request_queue *q = bdev_get_queue(bio->bi_bdev);
    struct blk_plug *plug = current->plug;
    struct request *rq;

    // 1. bio 분할 (max_sectors 초과 시)
    bio = __bio_split_to_limits(bio, &q->limits, &nr_segs);

    // 2. 병합 시도 (인접 LBA의 기존 request에 합치기)
    if (blk_mq_attempt_bio_merge(q, bio, nr_segs))
        return;  // 병합 성공 → 새 request 불필요

    // 3. ★★ request 할당 (tag = NVMe Command ID)
    rq = blk_mq_get_new_requests(q, plug, bio);

    // 4. bio → request 변환
    blk_mq_bio_to_request(rq, bio, nr_segs);

    // 5. ★★★ 디스패치 경로 분기
    if (plug) {
        // 경로 A: plug 리스트에 추가 (blk_finish_plug 시 일괄 전송)
        blk_add_rq_to_plug(plug, rq);
        return;
    }

    if (스케줄러 사용 || hctx busy) {
        // 경로 B: SW 큐 → 비동기 디스패치
        blk_mq_insert_request(rq);
        blk_mq_run_hw_queue(hctx, true);
    } else {
        // 경로 C: ★ 직접 디스패치 (NVMe 최단 경로)
        blk_mq_try_issue_directly(hctx, rq);  // ⑩
    }
}
```

**bio → request 전달되는 필드들:**

| bio 필드 | request 필드 | 설명 |
|----------|-------------|------|
| `bi_iter.bi_sector` | `__sector` | LBA |
| `bi_iter.bi_size` | `__data_len` | 크기 |
| `bi_ioprio` | `ioprio` | **IO 우선순위 ★ 전달됨** |
| `bi_opf` | `cmd_flags` | REQ_OP_READ 등 |
| — | `timeout` | **0 (아직 미설정!)** |

| 항목 | 값 |
|------|-----|
| request 할당 | `blk_mq_get_new_requests()` — tag(NVMe CID) 포함 |
| timeout 상태 | `rq->timeout = 0` (초기값) |
| timeout 개입 가능성 | **이론적 가능** — rq 할당 직후 timeout을 설정하면 반영 가능하나, 현재 코드에 그런 경로 없음 |

---

### ⑩⑪ blk_mq_try_issue_directly() → __blk_mq_issue_directly() — block/blk-mq.c:3194

```c
static blk_status_t __blk_mq_issue_directly(struct blk_mq_hw_ctx *hctx,
                                             struct request *rq, bool last)
{
    struct blk_mq_queue_data bd = { .rq = rq, .last = last };

    // ★★★ NVMe 드라이버 콜백 호출
    ret = q->mq_ops->queue_rq(hctx, &bd);    // ⑫ nvme_queue_rq()
}
```

| 항목 | 값 |
|------|-----|
| 드라이버 콜백 | `blk_mq_ops.queue_rq` = `nvme_queue_rq` |
| timeout 개입 가능성 | **없음** — 단순 콜백 호출 |

---

### ⑫ nvme_queue_rq() — drivers/nvme/host/pci.c:1675

```c
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
                                   const struct blk_mq_queue_data *bd)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    struct request *req = bd->rq;

    // 큐 활성화 체크
    if (!test_bit(NVMEQ_ENABLED, &nvmeq->flags))
        return BLK_STS_IOERR;

    // ★★ 요청 준비 (커맨드 빌드 + DMA 매핑 + 타이머 시작)
    ret = nvme_prep_rq(req);

    // ★★★ SQ에 커맨드 복사 + Doorbell
    spin_lock(&nvmeq->sq_lock);
    nvme_sq_copy_cmd(nvmeq, &iod->cmd);     // SQ[tail]에 64B 쓰기
    nvme_write_sq_db(nvmeq, bd->last);       // Doorbell register write
    spin_unlock(&nvmeq->sq_lock);

    return BLK_STS_OK;
}
```

#### nvme_prep_rq() 내부 — pci.c:1598

```c
static blk_status_t nvme_prep_rq(struct request *req)
{
    // 1. NVMe 커맨드 구성 (opcode, NSID, LBA, NLB 등)
    ret = nvme_setup_cmd(req->q->queuedata, req);
         │
         │  // nvme_setup_cmd() 내부 (core.c:973):
         │  req->timeout = NVME_IO_TIMEOUT;    // ★★★ 30초 하드코딩!
         │  // opcode 설정, LBA/NLB 설정, CDW12/CDW13 설정

    // 2. DMA 매핑 (PRP/SGL 리스트 구성)
    ret = nvme_map_data(req);

    // 3. ★ blk-mq에 요청 시작 알림 → 타이머 시작
    nvme_start_request(req);
         │
         │  // blk_mq_start_request() → blk_add_timer(req)
         │  // blk_add_timer() 내부 (blk-timeout.c:128):
         │  if (!req->timeout)
         │      req->timeout = q->rq_timeout;    // 이미 30초로 설정됨
         │  req->deadline = jiffies + req->timeout;
         │  // ★ 이 순간부터 timeout 감시 시작
}
```

| 항목 | 값 |
|------|-----|
| timeout 설정 위치 | `nvme_setup_cmd()` — `req->timeout = NVME_IO_TIMEOUT` |
| NVME_IO_TIMEOUT | `nvme_io_timeout * HZ` = 30초 (module_param) |
| 타이머 시작 | `blk_add_timer(req)` — `deadline = jiffies + 30초` |
| timeout 개입 가능성 | **★ 가능!** — `nvme_setup_cmd()` 직후, `blk_add_timer()` 직전에 `req->timeout`을 변경하면 반영됨 |

---

## 데이터 구조체 변환 전체 흐름

```
User Space          Kernel (AIO)        Kernel (Block)       Kernel (NVMe)
━━━━━━━━━━         ━━━━━━━━━━━━        ━━━━━━━━━━━━━━       ━━━━━━━━━━━━━

struct iocb    →    struct kiocb    →    struct bio     →    struct request    →    NVMe SQE
(user, 64B)         (VFS 레벨)           (block 레벨)        (blk-mq 레벨)          (64B HW)

aio_lio_opcode      ki_flags             bi_opf              cmd_flags              opcode
aio_fildes    →     ki_filp        →     bi_bdev       →     q                →     NSID
aio_offset          ki_pos               bi_sector           __sector               SLBA
aio_nbytes          (iter count)         bi_size             __data_len             NLB
aio_reqprio   →     ki_ioprio      →     bi_ioprio     →     ioprio                 CDW13(DSM)
aio_buf             (iter)               (bvec/pages)        (sg_table)             PRP/SGL

없음                없음                 없음                timeout = 30초          없음
                                                             ★ 하드코딩!
```

---

## timeout 개입 가능한 지점 분석

### 전체 경로에서 timeout과 관련된 코드 위치

| 단계 | 함수 | timeout 관련 코드 | 개입 가능성 |
|------|------|------------------|:-----------:|
| ① | `io_submit()` | 없음 | X |
| ② | `io_submit_one()` | `aio_reserved2 != 0 → EINVAL` 체크 | **△** (이 필드를 timeout으로 재정의 가능하나 ABI 변경) |
| ③ | `__io_submit_one()` | 없음 | X |
| ④ | `aio_prep_rw()` | `ki_ioprio` 설정만 있음 | **△** (kiocb에 timeout 필드 추가 가능) |
| ⑤ | `blkdev_read_iter()` | 없음 | X |
| ⑥ | `blkdev_direct_IO()` | 없음 | X |
| ⑦ | `__blkdev_direct_IO_async()` | `bio->bi_ioprio = iocb->ki_ioprio` | **△** (bio에 timeout 필드 추가 가능) |
| ⑧ | `submit_bio()` | `bio_set_ioprio()` | X |
| ⑨ | `blk_mq_submit_bio()` | `rq->timeout = 0` (초기) | **△** (rq 할당 직후 설정 가능) |
| ⑩ | `blk_mq_try_issue_directly()` | 없음 | X |
| ⑪ | `__blk_mq_issue_directly()` | 없음 | X |
| ⑫-a | `nvme_setup_cmd()` | **`req->timeout = NVME_IO_TIMEOUT`** | **★ 핵심 개입 지점** |
| ⑫-b | `blk_add_timer()` | `if (!req->timeout) req->timeout = q->rq_timeout` | **★ 최종 결정 지점** |

### 구체적 개입 방안 3가지

#### 방안 1: struct iocb 확장 (User → Kernel 전달)

```c
// ★ 현재 코드:
struct iocb {
    ...
    __u64   aio_reserved2;     // must be 0 → EINVAL
    __u32   aio_flags;
    __u32   aio_resfd;
};

// ★ 제안: aio_reserved2를 timeout으로 재정의
struct iocb {
    ...
    __u32   aio_timeout_ms;    // per-IO timeout (ms), 0=default
    __u32   aio_originator;    // 0=CPU, 1=GPU, 2=DPU
    __u32   aio_flags;         // IOCB_FLAG_TIMEOUT 추가
    __u32   aio_resfd;
};

// io_submit_one()에서 기존 호환성 유지:
if (iocb.aio_reserved2 && !(iocb.aio_flags & IOCB_FLAG_TIMEOUT))
    return -EINVAL;  // 기존 동작 유지
```

**전달 체인:**
```
iocb.aio_timeout_ms
  → ② io_submit_one()에서 읽음
    → ④ aio_prep_rw(): kiocb에 새 필드로 복사
      → ⑦ __blkdev_direct_IO_async(): bio에 새 필드로 복사
        → ⑨ blk_mq_submit_bio(): rq->timeout에 설정
          → ⑫-b blk_add_timer(): 해당 timeout으로 타이머 시작
```

#### 방안 2: ioprio 경로 활용 (기존 인프라 재사용)

```c
// ioprio는 이미 iocb → kiocb → bio → request 전체 경로가 있음
// ioprio의 hint 비트를 활용하여 timeout class를 인코딩

// 현재 ioprio 구조 (16비트):
//   bits 15:13 = class (RT/BE/IDLE)
//   bits 12:0  = priority level
//   hint는 별도 매크로로 확장 가능

// nvme_setup_cmd()에서 ioprio 기반 timeout 결정:
static void nvme_set_timeout_from_ioprio(struct request *req)
{
    u16 ioprio = req->ioprio;

    if (IOPRIO_PRIO_CLASS(ioprio) == IOPRIO_CLASS_RT)
        req->timeout = msecs_to_jiffies(100);     // RT: 100ms
    else if (IOPRIO_PRIO_CLASS(ioprio) == IOPRIO_CLASS_IDLE)
        req->timeout = msecs_to_jiffies(60000);   // IDLE: 60초
    else
        req->timeout = NVME_IO_TIMEOUT;            // BE: 30초 (기본)
}
```

**장점**: struct iocb / kiocb / bio 수정 불필요
**단점**: ioprio의 원래 의미(스케줄링 우선순위)와 혼용

#### 방안 3: NVMe 드라이버 직접 수정 (nvme_setup_cmd)

```c
// drivers/nvme/host/core.c — nvme_setup_cmd() 수정

// 현재:
req->timeout = NVME_IO_TIMEOUT;  // 30초 하드코딩

// 제안: bio의 ioprio 또는 새 필드를 기반으로 timeout 차별화
if (req->bio && req->bio->bi_ioprio) {
    switch (IOPRIO_PRIO_CLASS(req->bio->bi_ioprio)) {
    case IOPRIO_CLASS_RT:
        req->timeout = msecs_to_jiffies(100);   // GPU inference: 100ms
        break;
    case IOPRIO_CLASS_IDLE:
        req->timeout = msecs_to_jiffies(60000); // Background: 60s
        break;
    default:
        req->timeout = NVME_IO_TIMEOUT;          // Normal: 30s
    }
} else {
    req->timeout = NVME_IO_TIMEOUT;
}
```

---

## timeout이 발동되면 어떻게 되는가

```
blk_add_timer(req)
    │  deadline = jiffies + req->timeout
    │
    │  ─── req->timeout 시간 경과 ───
    │
    ▼
blk_mq_timeout_work()                    block/blk-mq.c:2094
    │
    ▼
blk_mq_check_expired(rq)                 block/blk-mq.c:2030
    │  if (time_after_eq(jiffies, rq->deadline))
    │      // ★ 만료됨!
    │
    ▼
blk_mq_rq_timed_out(rq)                  block/blk-mq.c:1979
    │
    ▼
q->mq_ops->timeout(req)                  ★ NVMe 드라이버 콜백
    │
    ▼
nvme_timeout(req)                         drivers/nvme/host/pci.c:2294
    │
    ├─ 1단계: CQ 폴링 (인터럽트 놓쳤나 확인)
    │   nvme_poll_irqdisable(nvmeq)
    │   if (req 이미 완료됨) → return BLK_EH_DONE
    │
    ├─ 2단계: Abort 커맨드 전송 (해당 IO만 취소 시도)
    │   nvme_alloc_request(ADMIN_Q)
    │   cmd.abort.sqid = nvmeq->qid
    │   cmd.abort.cid = req->tag     ← Command ID로 특정 IO 지정
    │   nvme_submit_sync_cmd()
    │   return BLK_EH_RESET_TIMER    ← Abort 결과 대기
    │
    └─ 3단계: Abort 후에도 미완료 → 컨트롤러 전체 리셋!
        nvme_dev_disable(dev, false)
        nvme_reset_ctrl(&dev->ctrl)
        return BLK_EH_DONE
```

**per-thread timeout이 짧으면 (예: 100ms):**
- GPU thread의 IO가 100ms 내 미완료 → `nvme_timeout()` 발동
- **1단계**: CQ 폴링으로 완료 확인 시도
- **2단계**: Abort 커맨드 전송 → SSD에 해당 IO만 취소 요청
- **3단계(위험!)**: Abort 실패 시 **컨트롤러 전체 리셋** → 다른 thread의 IO도 전부 영향!

```
★ 주의: per-thread timeout을 너무 짧게 설정하면
  → 정상 IO도 timeout 처리됨
  → Abort 폭주
  → 최악의 경우 컨트롤러 리셋
  → 모든 thread의 IO가 실패

따라서 특허에서는:
  timeout을 "abort 트리거"가 아닌
  "SSD 내부 스케줄링 hint"로 사용하는 것이 핵심 차별점
```

---

## 전체 요약표

| 단계 | 구조체 | 파일 | 핵심 동작 | ioprio | timeout |
|:----:|--------|------|-----------|:------:|:-------:|
| ① | `struct iocb` (user) | fs/aio.c | syscall 진입 | 설정 가능 | **없음** |
| ② | `struct iocb` (kernel copy) | fs/aio.c | user→kernel 복사 | 전달 | **없음** |
| ③ | `struct aio_kiocb` | fs/aio.c | fd→file, opcode 분기 | — | **없음** |
| ④ | `struct kiocb` | fs/aio.c | VFS IO 제어블록 | **전달됨** | **없음** |
| ⑤⑥ | `struct kiocb` | block/fops.c | O_DIRECT 경로 선택 | — | **없음** |
| ⑦ | `struct bio` | block/fops.c | **block IO 단위 생성** | **전달됨** | **없음** |
| ⑧ | `struct bio` | block/blk-core.c | ioprio 최종 결정 | **최종값** | **없음** |
| ⑨ | `struct request` | block/blk-mq.c | **HW 디스패치 단위** | **전달됨** | **0 (미설정)** |
| ⑩⑪ | `struct request` | block/blk-mq.c | 드라이버 콜백 호출 | — | — |
| ⑫ | `struct nvme_command` (SQE) | nvme/pci.c | **30초 하드코딩!** | CDW13 | **30초 고정** |
