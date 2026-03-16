# Per-IO Timeout via struct iocb 확장 제안

## 목표

libaio를 사용하는 multi-thread 환경에서, 각 thread가 io_submit() 시 iocb에 per-IO timeout을 설정하여 커널 NVMe 드라이버의 nvme_timeout()까지 차별적으로 동작하게 한다.

```
Thread A (GPU inference):  iocb.aio_timeout_ms = 100     (100ms)
Thread B (CPU backup):     iocb.aio_timeout_ms = 30000   (30초)
```

---

## 현재 상태: timeout 전달 경로가 없음

### struct iocb 현재 레이아웃 (include/uapi/linux/aio_abi.h)

```c
struct iocb {                                    // 64 bytes 총
    __u64   aio_data;           // [0]   user callback data
    __u32   aio_key;            // [8]   커널이 설정
    __kernel_rwf_t aio_rw_flags;// [12]  RWF_DSYNC, RWF_HIPRI 등
    __u16   aio_lio_opcode;     // [16]  IOCB_CMD_PREAD / PWRITE
    __s16   aio_reqprio;        // [18]  IO 우선순위 (ioprio)
    __u32   aio_fildes;         // [20]  file descriptor
    __u64   aio_buf;            // [24]  buffer address
    __u64   aio_nbytes;         // [32]  size
    __s64   aio_offset;         // [40]  file offset
    __u64   aio_reserved2;      // [48]  ★ 8바이트 미사용! (0 강제)
    __u32   aio_flags;          // [56]  IOCB_FLAG_RESFD | IOCB_FLAG_IOPRIO
    __u32   aio_resfd;          // [60]  eventfd
};
```

### 현재 필드별 전달 현황

```
struct iocb         struct kiocb       struct bio          struct request      NVMe SQE
(user, UAPI)        (VFS)              (block)             (blk-mq)            (HW, 64B)
──────────────      ──────────         ──────────          ──────────          ──────────
aio_data         →  (aio_kiocb에)       —                   —                   —
aio_lio_opcode   →  (switch 분기)       bi_opf           →  cmd_flags        →  opcode
aio_reqprio      →  ki_ioprio        →  bi_ioprio        →  ioprio           →  CDW13
aio_fildes       →  ki_filp          →  bi_bdev          →  q                →  NSID
aio_buf          →  (iov_iter)       →  (bvec/pages)     →  (sg_table)       →  PRP/SGL
aio_nbytes       →  (iov_iter)       →  bi_iter.bi_size  →  __data_len       →  NLB
aio_offset       →  ki_pos           →  bi_iter.bi_sector→  __sector         →  SLBA
aio_rw_flags     →  ki_flags         →  bi_opf bits      →  cmd_flags        →  FUA 등
aio_reserved2    →  ✗ EINVAL if !=0     ✗ 필드 없음         ✗ timeout=30초      ✗ 없음
                    ^^^^^^^^^^^^^^^^    ^^^^^^^^^^^^^       ^^^^^^^^^^^^^^^
                    전달 안 됨           전달 안 됨           하드코딩!
```

### 커널에서 timeout이 30초로 하드코딩되는 위치

```c
// drivers/nvme/host/core.c:973 — nvme_setup_cmd() 내부
req->timeout = NVME_IO_TIMEOUT;    // nvme_io_timeout * HZ = 30 * HZ

// block/blk-timeout.c:137-138 — blk_add_timer() 내부
if (!req->timeout)
    req->timeout = q->rq_timeout;  // 위에서 이미 30초로 설정됨
req->deadline = jiffies + req->timeout;
// → 이 순간부터 30초 카운트다운 시작
```

---

## 제안: 6개 지점 수정으로 전체 경로 연결

### 수정 지점 총괄

| 수정 | 파일 | 구조체/함수 | 변경 내용 |
|:----:|------|-----------|----------|
| ① | `include/uapi/linux/aio_abi.h` | `struct iocb` | `aio_reserved2` → timeout + originator 분할 |
| ② | `fs/aio.c:2034` | `io_submit_one()` | reserved2 != 0 체크 로직 수정 |
| ③ | `include/linux/fs.h` + `fs/aio.c:1510` | `struct kiocb` + `aio_prep_rw()` | `ki_timeout_ms`, `ki_originator_type` 추가 |
| ④ | `include/linux/blk_types.h` + `block/fops.c:322` | `struct bio` + `__blkdev_direct_IO_async()` | `bi_timeout_ms`, `bi_originator_type` 추가 |
| ⑤ | `block/blk-mq.c:3697` | `blk_mq_submit_bio()` | `bio->bi_timeout_ms` → `rq->timeout` 반영 |
| ⑥ | `drivers/nvme/host/core.c:973` | `nvme_setup_cmd()` | 하드코딩 제거, 기존 timeout 유지 |

---

### 수정 ①: struct iocb (UAPI 헤더)

**파일**: `include/uapi/linux/aio_abi.h`

```c
// ── 現行 ──
__u64   aio_reserved2;   /* TODO: use this for a (struct sigevent *) */

// ── 提案 ──
__u32   aio_timeout_ms;       // per-IO timeout (ms). 0 = default (NVME_IO_TIMEOUT)
__u8    aio_originator_type;  // 0=unspec, 1=CPU, 2=GPU, 3=DPU, 4=FPGA
__u8    aio_latency_class;    // 0=default, 1=ultra-low(<50μs), 2=low(<200μs), 3=relaxed
__u16   aio_reserved3;        // 향후 확장용 (must be 0)
```

```c
// aio_flags に新フラグ追加:
#define IOCB_FLAG_RESFD     (1 << 0)   // 기존
#define IOCB_FLAG_IOPRIO    (1 << 1)   // 기존
#define IOCB_FLAG_TIMEOUT   (1 << 2)   // ★ 신규: timeout/originator 필드 활성화
```

**메모리 레이아웃 비교:**

```
현재 aio_reserved2 (offset 48, 8바이트):
┌────────────────────────────────────────────────────────────────┐
│  0x00  0x00  0x00  0x00  0x00  0x00  0x00  0x00               │
│  ──────────────── must be all zeros ──────────────────         │
└────────────────────────────────────────────────────────────────┘

제안 (같은 8바이트):
┌──────────────────┬─────┬─────┬──────────┐
│ aio_timeout_ms   │ OT  │ LC  │ reserved │
│ (4 bytes, LE)    │(1B) │(1B) │ (2B, =0) │
│ 예: 0x64000000   │0x02 │0x01 │ 0x0000   │
│    = 100 ms      │=GPU │=UL  │          │
└──────────────────┴─────┴─────┴──────────┘
```

**하위 호환성:**
- `IOCB_FLAG_TIMEOUT`이 설정되지 않으면 기존과 동일하게 `aio_reserved2 != 0` → `-EINVAL`
- `IOCB_FLAG_TIMEOUT`이 설정되면 새 필드로 해석

---

### 수정 ②: io_submit_one() — reserved2 체크 로직

**파일**: `fs/aio.c:2034`

```c
// ── 現行 ──
if (unlikely(iocb.aio_reserved2)) {
    pr_debug("EINVAL: reserve field set\n");
    return -EINVAL;
}

// ── 提案 ──
if (unlikely(iocb.aio_reserved2)) {
    if (!(iocb.aio_flags & IOCB_FLAG_TIMEOUT)) {
        // 기존 동작 유지: FLAG 없이 reserved2 사용 → 거부
        pr_debug("EINVAL: reserve field set without IOCB_FLAG_TIMEOUT\n");
        return -EINVAL;
    }
    // IOCB_FLAG_TIMEOUT이 있으면 reserved3만 체크
    if (iocb.aio_reserved3) {
        pr_debug("EINVAL: reserved3 field set\n");
        return -EINVAL;
    }
}
```

---

### 수정 ③: struct kiocb + aio_prep_rw()

**파일**: `include/linux/fs.h:381`

```c
// ── 現行 ──
struct kiocb {
    struct file     *ki_filp;
    loff_t           ki_pos;
    void           (*ki_complete)(struct kiocb *iocb, long ret);
    void            *private;
    int              ki_flags;
    u16              ki_ioprio;
    u8               ki_write_stream;
    struct wait_page_queue *ki_waitq;
};

// ── 提案 ──
struct kiocb {
    struct file     *ki_filp;
    loff_t           ki_pos;
    void           (*ki_complete)(struct kiocb *iocb, long ret);
    void            *private;
    int              ki_flags;
    u16              ki_ioprio;
    u8               ki_write_stream;
    u32              ki_timeout_ms;        // ★ 신규
    u8               ki_originator_type;   // ★ 신규
    u8               ki_latency_class;     // ★ 신규
    struct wait_page_queue *ki_waitq;
};
```

**파일**: `fs/aio.c:1510` — `aio_prep_rw()`

```c
static int aio_prep_rw(struct kiocb *req, const struct iocb *iocb, int rw_type)
{
    req->ki_complete = aio_complete_rw;
    req->ki_pos = iocb->aio_offset;
    req->ki_flags = req->ki_filp->f_iocb_flags | IOCB_AIO_RW;

    // 기존: ioprio 전달
    if (iocb->aio_flags & IOCB_FLAG_IOPRIO) {
        ioprio_check_cap(iocb->aio_reqprio);
        req->ki_ioprio = iocb->aio_reqprio;
    } else {
        req->ki_ioprio = get_current_ioprio();
    }

    // ★ 신규: timeout + originator 전달
    if (iocb->aio_flags & IOCB_FLAG_TIMEOUT) {
        req->ki_timeout_ms      = iocb->aio_timeout_ms;
        req->ki_originator_type = iocb->aio_originator_type;
        req->ki_latency_class   = iocb->aio_latency_class;
    } else {
        req->ki_timeout_ms      = 0;   // default (NVME_IO_TIMEOUT 사용)
        req->ki_originator_type = 0;   // unspecified
        req->ki_latency_class   = 0;   // default
    }

    ret = kiocb_set_rw_flags(req, iocb->aio_rw_flags, rw_type);
    ...
}
```

---

### 수정 ④: struct bio + __blkdev_direct_IO_async()

**파일**: `include/linux/blk_types.h:210`

```c
// ── 提案: struct bio에 필드 추가 ──
struct bio {
    struct bio          *bi_next;
    struct block_device *bi_bdev;
    blk_opf_t            bi_opf;
    unsigned short       bi_flags;
    unsigned short       bi_ioprio;
    enum rw_hint         bi_write_hint;
    u8                   bi_write_stream;
    blk_status_t         bi_status;
    u32                  bi_timeout_ms;       // ★ 신규
    u8                   bi_originator_type;  // ★ 신규
    u8                   bi_latency_class;    // ★ 신규
    ...
};
```

**파일**: `block/fops.c:322` — `__blkdev_direct_IO_async()`

```c
static ssize_t __blkdev_direct_IO_async(struct kiocb *iocb,
                                         struct iov_iter *iter,
                                         struct block_device *bdev,
                                         unsigned int nr_pages)
{
    bio = bio_alloc_bioset(bdev, nr_pages, opf, GFP_KERNEL, &blkdev_dio_pool);

    bio->bi_iter.bi_sector  = pos >> SECTOR_SHIFT;
    bio->bi_end_io          = blkdev_bio_end_io_async;
    bio->bi_ioprio          = iocb->ki_ioprio;           // 기존

    // ★ 신규: timeout + originator 전달
    bio->bi_timeout_ms      = iocb->ki_timeout_ms;
    bio->bi_originator_type = iocb->ki_originator_type;
    bio->bi_latency_class   = iocb->ki_latency_class;

    submit_bio(bio);
    return -EIOCBQUEUED;
}
```

---

### 수정 ⑤: blk_mq_submit_bio() — bio→request timeout 반영

**파일**: `block/blk-mq.c:3697`

```c
void blk_mq_submit_bio(struct bio *bio)
{
    ...
    rq = blk_mq_get_new_requests(q, plug, bio);
    blk_mq_bio_to_request(rq, bio, nr_segs);

    // ★ 신규: bio의 per-IO timeout을 request에 반영
    if (bio->bi_timeout_ms)
        rq->timeout = msecs_to_jiffies(bio->bi_timeout_ms);

    // 디스패치 경로 분기...
}
```

---

### 수정 ⑥: nvme_setup_cmd() — 하드코딩 제거

**파일**: `drivers/nvme/host/core.c:973`

```c
// ── 現行 ──
req->timeout = NVME_IO_TIMEOUT;   // 항상 30초 하드코딩

// ── 提案 ──
if (!req->timeout)
    req->timeout = NVME_IO_TIMEOUT;   // user가 설정 안 했으면 기본값
// user가 수정⑤에서 설정했으면 그 값이 유지됨
```

---

## 전체 데이터 플로우

### Thread A (GPU, timeout=100ms)

```
User Space
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

struct iocb cb;
cb.aio_lio_opcode     = IOCB_CMD_PREAD;
cb.aio_fildes         = fd;
cb.aio_buf            = (uint64_t)buf;
cb.aio_nbytes         = 4096;
cb.aio_offset         = offset;
cb.aio_flags          = IOCB_FLAG_TIMEOUT;       // ★
cb.aio_timeout_ms     = 100;                     // ★ 100ms
cb.aio_originator_type = 2;                      // ★ GPU
cb.aio_latency_class   = 1;                      // ★ ultra-low
io_submit(ctx, 1, &cbp);

━━━━━━━━━━━━━━━━━━━━━━ syscall boundary ━━━━━━━━━━━━━━━━━━

Kernel Space
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

fs/aio.c — SYSCALL_DEFINE3(io_submit)
│
├─ io_submit_one()
│    copy_from_user(&iocb, ...)
│    // iocb.aio_flags & IOCB_FLAG_TIMEOUT → reserved2 사용 허가
│
├─ __io_submit_one()
│    fget(iocb.aio_fildes) → struct file (NVMe block device)
│
├─ aio_prep_rw()                                     ← 수정③
│    kiocb.ki_timeout_ms      = 100                  ★
│    kiocb.ki_originator_type = 2 (GPU)              ★
│    kiocb.ki_latency_class   = 1 (ultra-low)        ★
│    kiocb.ki_ioprio          = get_current_ioprio()
│
├─ aio_read()
│    file->f_op->read_iter(kiocb, &iter)
│
│  block/fops.c — blkdev_read_iter()
│  │
│  ├─ blkdev_direct_IO()
│  │
│  ├─ __blkdev_direct_IO_async()                     ← 수정④
│  │    bio = bio_alloc_bioset(...)
│  │    bio->bi_ioprio          = kiocb->ki_ioprio
│  │    bio->bi_timeout_ms      = 100                ★
│  │    bio->bi_originator_type = 2 (GPU)            ★
│  │    bio->bi_latency_class   = 1 (ultra-low)      ★
│  │    submit_bio(bio)
│  │
│  block/blk-core.c — submit_bio()
│  │    bio_set_ioprio(bio)
│  │    submit_bio_noacct(bio)
│  │
│  block/blk-mq.c — blk_mq_submit_bio()             ← 수정⑤
│  │    rq = blk_mq_get_new_requests(...)
│  │    blk_mq_bio_to_request(rq, bio)
│  │    rq->timeout = msecs_to_jiffies(100)          ★ 100ms!
│  │
│  │    blk_mq_try_issue_directly(hctx, rq)
│  │      → __blk_mq_issue_directly()
│  │        → q->mq_ops->queue_rq(hctx, &bd)
│  │
│  drivers/nvme/host/pci.c — nvme_queue_rq()
│       │
│       ├─ nvme_prep_rq(req)
│       │    nvme_setup_cmd()                        ← 수정⑥
│       │      // req->timeout already = 100ms → skip 하드코딩
│       │      // NVMe SQE 구성 (opcode, LBA, NLB, PRP/SGL)
│       │
│       │    nvme_map_data()
│       │      // DMA 매핑 (물리 주소 → PRP/SGL 리스트)
│       │
│       │    nvme_start_request(req)
│       │      → blk_mq_start_request(req)
│       │        → blk_add_timer(req)
│       │          req->deadline = jiffies + 100ms   ★ 타이머 시작!
│       │          req->state = MQ_RQ_IN_FLIGHT
│       │
│       ├─ spin_lock(&nvmeq->sq_lock)
│       ├─ nvme_sq_copy_cmd(nvmeq, &iod->cmd)
│       │    // SQ[tail]에 64바이트 NVMe command 복사
│       ├─ nvme_write_sq_db(nvmeq, bd->last)
│       │    // ★ Doorbell register write → SSD가 command fetch 시작
│       └─ spin_unlock(&nvmeq->sq_lock)
│
│  ═══════════════════════════════════════════════════════
│  NVMe SSD가 command를 실행 중...
│  ═══════════════════════════════════════════════════════
│
│  Case 1: 100ms 이내 완료
│  ─────────────────────────
│    SSD → CQ에 completion 기록 → MSI-X 인터럽트
│    → nvme_irq() → nvme_process_cq()
│    → blk_mq_complete_request(req)
│    → blk_mq_end_request(req)  // 타이머 자동 취소
│    → bio->bi_end_io() = blkdev_bio_end_io_async()
│    → kiocb->ki_complete() = aio_complete_rw()
│    → io_getevents()에서 user에게 반환
│
│  Case 2: 100ms 초과 — nvme_timeout() 발동!
│  ────────────────────────────────────────────
│    blk_mq_timeout_work()
│      → blk_mq_check_expired(rq)
│        // jiffies >= rq->deadline (100ms 초과)
│      → blk_mq_rq_timed_out(rq)
│        → q->mq_ops->timeout(req) = nvme_timeout()
│
│    nvme_timeout(req)   [drivers/nvme/host/pci.c:2294]
│    │
│    ├─ 1단계: CQ 폴링 (인터럽트 놓쳤나?)
│    │   nvme_poll_irqdisable(nvmeq)
│    │   if (이미 완료됨)
│    │       return BLK_EH_DONE          // 정상 완료
│    │
│    ├─ 2단계: Abort 커맨드 전송
│    │   cmd.abort.opcode = nvme_admin_abort_cmd
│    │   cmd.abort.sqid   = nvmeq->qid   // 해당 SQ
│    │   cmd.abort.cid    = req->tag      // 해당 Command ID
│    │   nvme_submit_sync_cmd(admin_q, &cmd)
│    │   return BLK_EH_RESET_TIMER       // Abort 결과 대기
│    │
│    └─ 3단계: Abort 실패 시
│        nvme_dev_disable(dev, false)
│        nvme_reset_ctrl(&dev->ctrl)     // ★ 컨트롤러 전체 리셋!
│        return BLK_EH_DONE
```

### Thread B (CPU, timeout=30000ms) — 동시 실행

```
같은 NVMe device, 같은 코드 경로를 타지만:

iocb.aio_timeout_ms = 30000   →   rq->timeout = 30초   →   deadline = jiffies + 30초

→ Thread A의 100ms timeout과 완전히 독립
→ Thread A가 timeout되어 abort 되어도 Thread B의 IO는 영향 없음
   (Abort는 Command ID 단위로 동작하므로)
```

---

## 전달 추적 요약표

```
구조체 변환 체인과 timeout 전달:

struct iocb (user, UAPI)       ← 수정①
│  aio_timeout_ms     = 100
│  aio_originator_type = 2 (GPU)
│  aio_latency_class   = 1 (ultra-low)
│  aio_flags           = IOCB_FLAG_TIMEOUT
│
│  copy_from_user()    ← 수정②: reserved2 체크 로직 수정
▼
struct kiocb (VFS)             ← 수정③
│  ki_timeout_ms       = 100        ★ iocb에서 복사
│  ki_originator_type  = 2          ★ iocb에서 복사
│  ki_latency_class    = 1          ★ iocb에서 복사
│  ki_ioprio           = (thread의 ioprio)
│
│  file->f_op->read_iter()
▼
struct bio (block layer)       ← 수정④
│  bi_timeout_ms       = 100        ★ kiocb에서 복사
│  bi_originator_type  = 2          ★ kiocb에서 복사
│  bi_latency_class    = 1          ★ kiocb에서 복사
│  bi_ioprio           = ki_ioprio
│
│  submit_bio() → blk_mq_submit_bio()
▼
struct request (blk-mq)        ← 수정⑤
│  timeout             = msecs_to_jiffies(100)   ★ bio에서 변환
│  ioprio              = bi_ioprio
│
│  q->mq_ops->queue_rq()
▼
nvme_setup_cmd()               ← 수정⑥
│  // 기존: req->timeout = NVME_IO_TIMEOUT (30초 하드코딩)
│  // 제안: if (!req->timeout) req->timeout = NVME_IO_TIMEOUT
│  // → 이미 100ms로 설정되어 있으므로 유지됨
│
▼
blk_add_timer(req)             ← 기존 코드 (수정 불필요)
│  // req->timeout이 이미 100ms
│  // if (!req->timeout) req->timeout = q->rq_timeout;  ← 실행 안 됨
│  req->deadline = jiffies + req->timeout;   // jiffies + 100ms
│
▼
NVMe SQ에 command 복사 → Doorbell → SSD 실행
│
▼
100ms 초과 시: nvme_timeout() → Abort → (최악) Controller Reset
```

---

## 각 구조체 필드 전달 총괄

| 필드 | iocb (user) | kiocb (VFS) | bio (block) | request (blk-mq) | NVMe SQE (HW) |
|------|:-----------:|:-----------:|:-----------:|:----------------:|:-------------:|
| opcode | `aio_lio_opcode` | (switch 분기) | `bi_opf` | `cmd_flags` | `CDW0.OPC` |
| fd/device | `aio_fildes` | `ki_filp` | `bi_bdev` | `q` | `CDW1.NSID` |
| offset | `aio_offset` | `ki_pos` | `bi_sector` | `__sector` | `CDW10-11.SLBA` |
| size | `aio_nbytes` | (iter count) | `bi_size` | `__data_len` | `CDW12.NLB` |
| buffer | `aio_buf` | (iov_iter) | (bvec) | (sg_table) | `CDW6-9.PRP` |
| ioprio | `aio_reqprio` | `ki_ioprio` | `bi_ioprio` | `ioprio` | `CDW13.DSM` |
| **timeout** | **`aio_timeout_ms`** | **`ki_timeout_ms`** | **`bi_timeout_ms`** | **`timeout`** | — |
| **originator** | **`aio_originator_type`** | **`ki_originator_type`** | **`bi_originator_type`** | — | **`CDW12 hint`** |
| **latency class** | **`aio_latency_class`** | **`ki_latency_class`** | **`bi_latency_class`** | — | **`CDW13 hint`** |

---

## User Space 사용 예제 (완전한 코드)

```c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <libaio.h>

/* 제안된 새 플래그/필드 (커널 수정 후 사용 가능) */
#define IOCB_FLAG_TIMEOUT   (1 << 2)

/* originator type */
#define AIO_ORIG_UNSPEC     0
#define AIO_ORIG_CPU        1
#define AIO_ORIG_GPU        2
#define AIO_ORIG_DPU        3

/* latency class */
#define AIO_LAT_DEFAULT     0
#define AIO_LAT_ULTRA_LOW   1   /* < 50μs target */
#define AIO_LAT_LOW         2   /* < 200μs target */
#define AIO_LAT_RELAXED     3   /* throughput 우선 */

struct thread_cfg {
    int         thread_id;
    const char *dev_path;
    uint32_t    timeout_ms;
    uint8_t     originator;
    uint8_t     latency_class;
    int         io_count;
};

static void setup_iocb_with_timeout(struct iocb *cb, int fd,
                                     void *buf, size_t len, off_t offset,
                                     uint32_t timeout_ms,
                                     uint8_t originator,
                                     uint8_t latency_class)
{
    memset(cb, 0, sizeof(*cb));
    cb->aio_lio_opcode = IOCB_CMD_PREAD;
    cb->aio_fildes     = fd;
    cb->aio_buf        = (uint64_t)buf;
    cb->aio_nbytes     = len;
    cb->aio_offset     = offset;

    /* ★ 제안된 per-IO timeout 설정 */
    cb->aio_flags = IOCB_FLAG_TIMEOUT;

    /* aio_reserved2 영역을 timeout + originator로 사용 */
    /* 현재 커널에서는 -EINVAL, 수정된 커널에서만 동작 */
    uint64_t reserved2 = 0;
    reserved2 |= (uint64_t)timeout_ms;                    /* [31:0]  timeout */
    reserved2 |= (uint64_t)originator << 32;              /* [39:32] originator */
    reserved2 |= (uint64_t)latency_class << 40;           /* [47:40] latency class */
    /* [63:48] reserved3 = 0 */
    cb->aio_reserved2 = reserved2;
}

static void *io_thread(void *arg)
{
    struct thread_cfg *cfg = (struct thread_cfg *)arg;
    io_context_t ctx = 0;
    struct io_event events[32];
    void *buf;

    io_setup(32, &ctx);

    int fd = open(cfg->dev_path, O_RDONLY | O_DIRECT);

    posix_memalign(&buf, 4096, 4096);

    printf("[Thread %d] %s: timeout=%ums, originator=%u, latency_class=%u\n",
           cfg->thread_id,
           cfg->originator == AIO_ORIG_GPU ? "GPU" : "CPU",
           cfg->timeout_ms, cfg->originator, cfg->latency_class);

    for (int i = 0; i < cfg->io_count; i++) {
        struct iocb cb;
        struct iocb *cbp = &cb;
        off_t offset = (rand() % 1000000) * 4096;

        setup_iocb_with_timeout(&cb, fd, buf, 4096, offset,
                                cfg->timeout_ms,
                                cfg->originator,
                                cfg->latency_class);

        io_submit(ctx, 1, &cbp);

        struct timespec ts = { .tv_sec = cfg->timeout_ms / 1000 + 1 };
        int ret = io_getevents(ctx, 1, 1, events, &ts);

        if (ret == 0)
            printf("[Thread %d] IO %d: io_getevents timeout\n",
                   cfg->thread_id, i);
    }

    free(buf);
    close(fd);
    io_destroy(ctx);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <nvme_device>\n", argv[0]);
        return 1;
    }

    struct thread_cfg cfgs[2] = {
        {
            .thread_id     = 0,
            .dev_path      = argv[1],
            .timeout_ms    = 100,              /* GPU: 100ms timeout */
            .originator    = AIO_ORIG_GPU,
            .latency_class = AIO_LAT_ULTRA_LOW,
            .io_count      = 100,
        },
        {
            .thread_id     = 1,
            .dev_path      = argv[1],
            .timeout_ms    = 30000,            /* CPU: 30초 timeout */
            .originator    = AIO_ORIG_CPU,
            .latency_class = AIO_LAT_RELAXED,
            .io_count      = 100,
        },
    };

    pthread_t tids[2];
    for (int i = 0; i < 2; i++)
        pthread_create(&tids[i], NULL, io_thread, &cfgs[i]);
    for (int i = 0; i < 2; i++)
        pthread_join(tids[i], NULL);

    return 0;
}
```

---

## 특허 관점에서의 의의

### 이 제안이 해결하는 문제

```
현재 Linux:

  io_submit() 호출 시 per-IO timeout 설정 불가
  → 모든 IO가 동일한 30초 timeout
  → GPU burst IO (< 4KB random read)도 30초 기다려야 timeout 감지
  → CPU background IO도 30초만에 abort (불필요하게 짧을 수도)
  → 이종 프로세서 환경에서 IO 특성에 맞는 timeout 불가능

제안:

  struct iocb의 aio_reserved2를 활용하여
  per-IO timeout + originator type + latency class를
  user space에서 커널 NVMe 드라이버까지 전달

  → GPU thread: 100ms timeout으로 빠른 실패 감지
  → CPU thread: 30초 timeout으로 안정적 처리
  → 같은 NVMe device, 같은 code path, 다른 timeout
```

### NVMe SSD 내부 스케줄링과의 연계 (특허 확장)

```
수정⑤에서 rq->timeout만 설정하는 것이 아니라,
originator_type과 latency_class를 NVMe command의
CDW12/CDW13에도 인코딩하면:

User Space                  Kernel                      NVMe SSD
──────────                  ──────                      ────────
iocb.originator=GPU    →    bio.bi_originator=GPU   →   CDW12.DTYPE=Originator
iocb.latency_class=UL  →    bio.bi_latency_class=UL →   CDW13.DSM=UltraLow
iocb.timeout_ms=100    →    rq->timeout=100ms       →   (SSD 내부 스케줄링 hint)

→ 커널 측: 100ms timeout으로 abort 제어
→ SSD 측: GPU+UltraLow hint로 내부 우선 스케줄링
→ 양쪽 모두에서 latency 보장
```
