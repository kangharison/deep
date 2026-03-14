# fio I/O 엔진과 커널 I/O 인터페이스 심층 분석

> **분석 대상**: Linux Kernel의 I/O 시스템 콜 경로 (sync, libaio, io_uring, ioctl)
> **소스 기준**: ~/sources/linux (최신 커널)
> **핵심 파일**: `fs/read_write.c`, `fs/aio.c`, `io_uring/io_uring.c`, `io_uring/rw.c`, `io_uring/uring_cmd.c`, `block/fops.c`, `block/blk-core.c`, `drivers/nvme/host/ioctl.c`

---

## 1. fio I/O 엔진 개요

fio(Flexible I/O tester)는 Linux에서 가장 널리 사용되는 스토리지 벤치마크 도구이다. fio의 핵심 설계 철학은 다양한 I/O 엔진(ioengine)을 플러그인 방식으로 지원하여, 동일한 워크로드를 서로 다른 커널 I/O 경로로 제출할 수 있게 하는 것이다.

### 1.1 주요 I/O 엔진과 커널 진입 경로

```
┌─────────────────────────────────────────────────────────────────────┐
│                         fio (유저스페이스)                           │
├──────────┬───────────┬────────────┬──────────────┬──────────────────┤
│  sync    │  psync    │  libaio    │  io_uring    │  io_uring_cmd    │
│ (동기)   │ (위치지정)│ (Linux AIO)│ (최신 비동기)│ (NVMe passthru)  │
└────┬─────┴─────┬─────┴─────┬──────┴──────┬───────┴────────┬─────────┘
     │           │           │             │                │
     v           v           v             v                v
  read()     pread()   io_submit()   io_uring_enter()  io_uring_enter()
  write()    pwrite()  io_getevents() (SQE 처리)       (URING_CMD SQE)
     │           │           │             │                │
     v           v           v             v                v
  ksys_read()   do_preadv()  aio_read()   io_read()    io_uring_cmd()
     │           │           │             │                │
     v           v           v             v                v
  vfs_read()    vfs_read()   │             │           nvme_uring_cmd()
     │           │           │             │                │
     v           v           v             v                v
  blkdev_read_iter()────────────────────────┘           nvme_alloc_user
     │                                                  _request()
     v                                                      │
  blkdev_direct_IO() ← Direct I/O                          v
     │                                              blk_execute_rq
     v                                              _nowait()
  submit_bio() ──────────────────────────────────────────────┘
     │
     v
  blk_mq_submit_bio() → nvme_queue_rq() → NVMe SQ doorbell
```

### 1.2 각 엔진의 장단점 비교 (NVMe 기준)

**sync 엔진 (read/write)**:
- 장점: 구현이 가장 단순하고 디버깅이 쉬움
- 단점: I/O 하나당 시스템 콜 하나, 완료까지 프로세스가 블로킹됨
- NVMe에서: 큐 깊이 1로 제한되어 NVMe의 병렬 처리 능력을 전혀 활용 못함

**psync 엔진 (pread/pwrite)**:
- 장점: 파일 오프셋을 명시적으로 지정하므로 멀티스레드에서 안전
- 단점: sync와 동일하게 블로킹, 큐 깊이 1
- NVMe에서: sync와 성능 차이 거의 없음

**libaio 엔진 (io_submit/io_getevents)**:
- 장점: 비동기 I/O로 높은 큐 깊이 가능, 배치 제출 가능
- 단점: O_DIRECT 필수, 시스템 콜 오버헤드 여전히 존재, 메타데이터 I/O에 비동기 불가
- NVMe에서: 높은 IOPS 달성 가능하지만 io_uring보다는 열등

**io_uring 엔진 (io_uring_enter)**:
- 장점: 커널-유저 공유 메모리 링으로 시스템 콜 최소화, 폴링 모드 지원, 배치 제출/완료
- 단점: 커널 5.1+ 필요, 설정이 복잡
- NVMe에서: 최고 성능을 달성할 수 있는 I/O 인터페이스, IOPOLL과 결합하면 초저지연 가능

**io_uring_cmd 엔진 (NVMe passthrough)**:
- 장점: NVMe 커맨드를 직접 구성하여 블록 레이어를 우회, 가장 낮은 레이턴시
- 단점: NVMe 전용, 커널 5.19+ 필요
- NVMe에서: NVMe 스펙 레벨의 최적화가 가능한 유일한 fio 엔진

---

## 2. Sync I/O 경로 (read/write 시스템 콜)

### 2.1 시스템 콜 진입

fio의 sync 엔진은 표준 POSIX read()/write() 시스템 콜을 사용한다. 커널 내부 호출 체인은 다음과 같다.

```
유저스페이스: read(fd, buf, count)
    │
    v
커널 진입: SYSCALL_DEFINE3(read, ...)     ← fs/read_write.c:722
    │
    v
ksys_read(fd, buf, count)                 ← fs/read_write.c:704
    │  fd → file 변환
    │  file position 관리 (f_pos)
    v
vfs_read(file, buf, count, &pos)          ← fs/read_write.c:552
    │  1) FMODE_READ 권한 확인
    │  2) access_ok(buf, count) — 유저 버퍼 유효성 검증
    │  3) rw_verify_area() — 보안 검사 (SELinux 등)
    │  4) count > MAX_RW_COUNT이면 제한
    v
file->f_op->read_iter() 호출              ← 블록 디바이스: blkdev_read_iter()
```

**소스코드 분석 - ksys_read()** (`fs/read_write.c:704`):

```c
ssize_t ksys_read(unsigned int fd, char __user *buf, size_t count)
{
    CLASS(fd_pos, f)(fd);           // fd 번호로 file 구조체 획득
    ssize_t ret = -EBADF;

    if (!fd_empty(f)) {
        loff_t pos, *ppos = file_ppos(fd_file(f));
        if (ppos) {
            pos = *ppos;            // 현재 파일 위치를 로컬 변수에 복사
            ppos = &pos;
        }
        ret = vfs_read(fd_file(f), buf, count, ppos);
        if (ret >= 0 && ppos)
            fd_file(f)->f_pos = pos; // 읽은 만큼 파일 위치 갱신
    }
    return ret;
}
```

**소스코드 분석 - vfs_read()** (`fs/read_write.c:552`):

```c
ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    ssize_t ret;

    if (!(file->f_mode & FMODE_READ))       // 읽기 권한 없으면 -EBADF
        return -EBADF;
    if (!(file->f_mode & FMODE_CAN_READ))   // 읽기 불가 파일이면 -EINVAL
        return -EINVAL;
    if (unlikely(!access_ok(buf, count)))    // 유저 버퍼 주소 유효성 검사
        return -EFAULT;

    ret = rw_verify_area(READ, file, pos, count);  // LSM 보안 체크
    if (ret)
        return ret;
    if (count > MAX_RW_COUNT)
        count = MAX_RW_COUNT;

    if (file->f_op->read)
        ret = file->f_op->read(file, buf, count, pos);   // 레거시 read
    else if (file->f_op->read_iter)
        ret = new_sync_read(file, buf, count, pos);       // 현대적 read_iter
    else
        ret = -EINVAL;

    if (ret > 0) {
        fsnotify_access(file);        // inotify/fanotify 알림
        add_rchar(current, ret);      // 프로세스 I/O 통계 갱신
    }
    inc_syscr(current);               // 시스템 콜 읽기 횟수 증가
    return ret;
}
```

블록 디바이스는 `f_op->read`가 없고 `f_op->read_iter`만 있으므로 `new_sync_read()`를 거쳐 `blkdev_read_iter()`가 호출된다.

### 2.2 블록 디바이스 read_iter — blkdev_read_iter()

블록 디바이스에 대한 읽기 요청이 도달하는 핵심 함수이다. `block/fops.c`에 정의되어 있다.

**소스코드 분석 - blkdev_read_iter()** (`block/fops.c:812`):

```c
static ssize_t blkdev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    struct inode *bd_inode = bdev_file_inode(iocb->ki_filp);
    struct block_device *bdev = I_BDEV(iocb->ki_filp->f_mapping->host);
    loff_t size = bdev_nr_bytes(bdev);   // 디바이스 전체 크기
    loff_t pos = iocb->ki_pos;
    size_t shorted = 0;
    ssize_t ret = 0;
    size_t count;

    // 디바이스 끝을 넘어가는 요청이면 잘라냄
    if (unlikely(pos + iov_iter_count(to) > size)) {
        if (pos >= size)
            return 0;
        size -= pos;
        shorted = iov_iter_count(to) - size;
        iov_iter_truncate(to, size);
    }

    count = iov_iter_count(to);
    if (!count)
        goto reexpand;

    if (iocb->ki_flags & IOCB_DIRECT) {
        // ★ Direct I/O 경로: 페이지 캐시를 우회하여 디바이스에 직접 I/O
        ret = kiocb_write_and_wait(iocb, count);  // 기존 dirty 페이지 flush
        if (ret < 0)
            goto reexpand;
        file_accessed(iocb->ki_filp);  // atime 갱신

        ret = blkdev_direct_IO(iocb, to);  // ← 핵심: Direct I/O 실행
        if (ret > 0) {
            iocb->ki_pos += ret;
            count -= ret;
        }
        ...
    }

    // ★ Buffered I/O 경로: 페이지 캐시를 경유
    inode_lock_shared(bd_inode);
    ret = filemap_read(iocb, to, ret);   // 페이지 캐시에서 읽기
    inode_unlock_shared(bd_inode);

reexpand:
    if (unlikely(shorted))
        iov_iter_reexpand(to, iov_iter_count(to) + shorted);
    return ret;
}
```

여기서 `IOCB_DIRECT` 플래그의 유무에 따라 Direct I/O와 Buffered I/O로 분기한다. fio에서 `direct=1`을 설정하면 파일을 O_DIRECT로 열기 때문에 `IOCB_DIRECT`가 설정된다.

### 2.3 Direct I/O 경로 — blkdev_direct_IO()

Direct I/O는 페이지 캐시를 우회하여 유저 버퍼에서 직접 디바이스로 데이터를 전송한다. NVMe 벤치마크에서는 거의 항상 Direct I/O를 사용한다.

**소스코드 분석 - blkdev_direct_IO()** (`block/fops.c:396`):

```c
static ssize_t blkdev_direct_IO(struct kiocb *iocb, struct iov_iter *iter)
{
    struct block_device *bdev = I_BDEV(iocb->ki_filp->f_mapping->host);
    unsigned int nr_pages;

    if (!iov_iter_count(iter))
        return 0;

    // 정렬 검사: 위치와 크기가 논리 블록 크기의 배수여야 함
    if (blkdev_dio_invalid(bdev, iocb, iter))
        return -EINVAL;

    nr_pages = bio_iov_vecs_to_alloc(iter, BIO_MAX_VECS + 1);

    if (likely(nr_pages <= BIO_MAX_VECS &&
               !(iocb->ki_flags & IOCB_HAS_METADATA))) {
        if (is_sync_kiocb(iocb))
            // ★ 동기 I/O: 단일 bio, 완료까지 대기
            return __blkdev_direct_IO_simple(iocb, iter, bdev, nr_pages);
        // ★ 비동기 I/O: 단일 bio, 즉시 반환 (libaio/io_uring에서 사용)
        return __blkdev_direct_IO_async(iocb, iter, bdev, nr_pages);
    }
    // ★ 대용량 I/O: 여러 bio로 분할하여 제출
    return __blkdev_direct_IO(iocb, iter, bdev, bio_max_segs(nr_pages));
}
```

이 함수는 세 가지 경로로 분기한다:

| 조건 | 함수 | 설명 |
|------|------|------|
| 동기 + 단일 bio | `__blkdev_direct_IO_simple()` | sync 엔진에서 사용. 스택에 bio 할당, 완료 대기 |
| 비동기 + 단일 bio | `__blkdev_direct_IO_async()` | libaio/io_uring에서 사용. 즉시 반환 |
| 대용량 (여러 bio 필요) | `__blkdev_direct_IO()` | 데이터가 BIO_MAX_VECS 페이지를 초과할 때 |

### 2.4 __blkdev_direct_IO_simple() 라인바이라인 분석

동기 Direct I/O의 핵심 함수이다. fio sync/psync 엔진이 블록 디바이스에 Direct I/O를 수행할 때 이 함수를 거친다.

**소스코드 분석** (`block/fops.c:55`):

```c
static ssize_t __blkdev_direct_IO_simple(struct kiocb *iocb,
        struct iov_iter *iter, struct block_device *bdev,
        unsigned int nr_pages)
{
    struct bio_vec inline_vecs[DIO_INLINE_BIO_VECS], *vecs;
    // DIO_INLINE_BIO_VECS = 4: 4페이지(16KB) 이하면 스택에 bio_vec 할당
    // 힙 할당을 피하여 성능 최적화

    loff_t pos = iocb->ki_pos;
    bool should_dirty = false;
    struct bio bio;          // ★ 스택에 bio 구조체를 직접 할당! (힙 할당 비용 절약)
    ssize_t ret;

    if (nr_pages <= DIO_INLINE_BIO_VECS)
        vecs = inline_vecs;  // 스택 사용
    else {
        vecs = kmalloc_array(nr_pages, sizeof(struct bio_vec), GFP_KERNEL);
        if (!vecs)
            return -ENOMEM;
    }

    // bio 초기화: 디바이스, bio_vec 배열, 오퍼레이션 설정
    if (iov_iter_rw(iter) == READ) {
        bio_init(&bio, bdev, vecs, nr_pages, REQ_OP_READ);
        if (user_backed_iter(iter))
            should_dirty = true;   // 읽기 완료 후 유저 페이지를 dirty로 마킹
    } else {
        bio_init(&bio, bdev, vecs, nr_pages, dio_bio_write_op(iocb));
        // dio_bio_write_op(): REQ_OP_WRITE | REQ_SYNC | REQ_IDLE
        // O_DSYNC이면 REQ_FUA 추가 (Force Unit Access)
    }

    bio.bi_iter.bi_sector = pos >> SECTOR_SHIFT;  // 바이트 오프셋 → 섹터 번호 변환
    bio.bi_write_hint = file_inode(iocb->ki_filp)->i_write_hint;
    bio.bi_ioprio = iocb->ki_ioprio;

    if (iocb->ki_flags & IOCB_ATOMIC)
        bio.bi_opf |= REQ_ATOMIC;  // atomic write 지원

    // 유저 버퍼의 물리 페이지를 bio에 매핑
    ret = blkdev_iov_iter_get_pages(&bio, iter, bdev);
    if (unlikely(ret))
        goto out;
    ret = bio.bi_iter.bi_size;  // 실제 매핑된 바이트 수

    if (iov_iter_rw(iter) == WRITE)
        task_io_account_write(ret);  // /proc/[pid]/io 통계 갱신

    if (iocb->ki_flags & IOCB_NOWAIT)
        bio.bi_opf |= REQ_NOWAIT;

    // ★★★ 핵심: bio를 제출하고 완료를 기다림
    submit_bio_wait(&bio);
    // submit_bio_wait()는 내부적으로:
    // 1) submit_bio(&bio) 호출 — 블록 레이어에 bio 전달
    // 2) blk_io_schedule() 반복 — I/O 완료 인터럽트까지 슬립
    // → 이것이 sync 엔진이 블로킹되는 이유

    bio_release_pages(&bio, should_dirty);
    if (unlikely(bio.bi_status))
        ret = blk_status_to_errno(bio.bi_status);

out:
    if (vecs != inline_vecs)
        kfree(vecs);
    bio_uninit(&bio);
    return ret;
}
```

**핵심 포인트**: sync 엔진은 `submit_bio_wait()`를 호출하여 I/O가 완료될 때까지 현재 태스크를 슬립시킨다. NVMe의 병렬 처리 능력(수만 개의 큐 깊이)을 전혀 활용할 수 없다.

### 2.5 __blkdev_direct_IO_async() — 비동기 Direct I/O

libaio나 io_uring에서 블록 디바이스에 Direct I/O를 수행할 때 호출되는 함수이다.

**소스코드 분석** (`block/fops.c:322`):

```c
static ssize_t __blkdev_direct_IO_async(struct kiocb *iocb,
                    struct iov_iter *iter,
                    struct block_device *bdev,
                    unsigned int nr_pages)
{
    bool is_read = iov_iter_rw(iter) == READ;
    blk_opf_t opf = is_read ? REQ_OP_READ : dio_bio_write_op(iocb);
    struct blkdev_dio *dio;
    struct bio *bio;
    loff_t pos = iocb->ki_pos;
    int ret = 0;

    // ★ bio를 힙에서 할당 (blkdev_dio_pool bioset 사용)
    // blkdev_dio 구조체가 bio 안에 임베딩됨
    bio = bio_alloc_bioset(bdev, nr_pages, opf, GFP_KERNEL, &blkdev_dio_pool);
    dio = container_of(bio, struct blkdev_dio, bio);

    dio->flags = 0;
    dio->iocb = iocb;  // 완료 시 iocb->ki_complete() 호출을 위해 저장

    bio->bi_iter.bi_sector = pos >> SECTOR_SHIFT;
    bio->bi_end_io = blkdev_bio_end_io_async;  // ★ 비동기 완료 콜백 등록
    bio->bi_ioprio = iocb->ki_ioprio;

    ...
    dio->size = bio->bi_iter.bi_size;

    if (iocb->ki_flags & IOCB_HIPRI) {
        bio->bi_opf |= REQ_POLLED;   // ★ 폴링 모드: 인터럽트 대신 폴링으로 완료 확인
        submit_bio(bio);
        WRITE_ONCE(iocb->private, bio);  // 나중에 폴링할 수 있도록 bio 포인터 저장
    } else {
        submit_bio(bio);  // ★ bio를 블록 레이어에 제출하고 즉시 반환
    }

    return -EIOCBQUEUED;  // "비동기로 큐잉되었음" — 나중에 완료 콜백이 호출됨
}
```

**완료 콜백 - blkdev_bio_end_io_async()** (`block/fops.c:294`):

```c
static void blkdev_bio_end_io_async(struct bio *bio)
{
    struct blkdev_dio *dio = container_of(bio, struct blkdev_dio, bio);
    struct kiocb *iocb = dio->iocb;
    ssize_t ret;

    WRITE_ONCE(iocb->private, NULL);

    if (likely(!bio->bi_status)) {
        ret = dio->size;
        iocb->ki_pos += ret;
    } else {
        ret = blk_status_to_errno(bio->bi_status);
    }

    // ★ 비동기 완료: 호출자(libaio의 aio_complete_rw 또는 io_uring의 io_complete_rw)에게 통지
    iocb->ki_complete(iocb, ret);

    ...
    bio_put(bio);
}
```

이 콜백은 NVMe 인터럽트 핸들러 → softirq → blk_mq_complete_request() 체인에서 호출된다.

### 2.6 Buffered I/O 경로

O_DIRECT 없이 파일을 열면 Buffered I/O 경로를 탄다. 읽기는 페이지 캐시에서, 쓰기는 페이지 캐시에 기록 후 나중에 writeback 된다.

**읽기 경로**:
```
blkdev_read_iter()
  → filemap_read()           ← 페이지 캐시에서 데이터 읽기
    → (캐시 미스 시)
    → blkdev_readahead()      ← readahead 트리거
      → mpage_readahead()     ← 여러 페이지를 한 bio로 모음
        → submit_bio()        ← 블록 레이어에 전달
```

**쓰기 경로**:
```
blkdev_write_iter()
  → blkdev_buffered_write()
    → iomap_file_buffered_write()  ← 페이지 캐시에 데이터 기록
      → (dirty 페이지 생성)
      → (나중에 writeback daemon이 flush)
      → blkdev_writepages()
        → submit_bio()             ← dirty 페이지를 디스크로 기록
```

Buffered I/O에서 실제 디스크 I/O가 발생하는 시점은 다음과 같다:
- **읽기**: 페이지 캐시 미스 시 readahead가 트리거되어 submit_bio() 호출
- **쓰기**: dirty 페이지가 일정 비율 이상 쌓이면 커널의 writeback daemon(kworker)이 blkdev_writepages()를 호출

---

## 3. Linux AIO (libaio) 경로

Linux AIO(Asynchronous I/O)는 커널 2.5에서 도입된 비동기 I/O 인터페이스이다. fio의 `libaio` 엔진이 이 인터페이스를 사용한다. 소스코드는 `fs/aio.c`에 위치한다.

### 3.1 io_setup() — AIO 컨텍스트 생성

유저 프로그램이 AIO를 사용하려면 먼저 `io_setup()`으로 AIO 컨텍스트를 생성해야 한다.

```
유저: io_setup(nr_events, &ctx_id)
    │
    v
SYSCALL_DEFINE2(io_setup, ...)          ← fs/aio.c:1382
    │
    v
ioctx_alloc(nr_events)
    │  1) kioctx 구조체 할당 (slab 캐시)
    │  2) aio_setup_ring() — 완료 이벤트 링 버퍼 생성
    │     - nr_events 크기의 io_event 배열을 mmap 가능한 페이지에 할당
    │     - 유저스페이스와 공유되는 aio_ring 구조체 초기화
    │  3) percpu 참조 카운터 초기화
    v
ctx_id 반환 (유저스페이스 핸들)
```

**aio_ring 구조체** (`fs/aio.c:56`):

```c
struct aio_ring {
    unsigned    id;              // 커널 내부 인덱스
    unsigned    nr;              // io_event 배열 크기
    unsigned    head;            // 유저가 읽기 위치 (유저가 갱신)
    unsigned    tail;            // 커널이 쓰기 위치 (커널이 갱신)
    unsigned    magic;           // 0xa10a10a1
    unsigned    compat_features;
    unsigned    incompat_features;
    unsigned    header_length;
    struct io_event io_events[]; // ★ 완료 이벤트가 저장되는 가변 길이 배열
};
```

이 링 버퍼는 커널과 유저스페이스 사이에 mmap으로 공유된다. 커널이 tail을 갱신하고, 유저가 head를 갱신하는 단방향 링 버퍼이다.

### 3.2 io_submit() — I/O 요청 제출

fio libaio 엔진의 핵심 시스템 콜이다. 여러 개의 iocb(I/O Control Block)를 한 번에 제출할 수 있다.

**소스코드 분석 - SYSCALL_DEFINE3(io_submit)** (`fs/aio.c:2082`):

```c
SYSCALL_DEFINE3(io_submit, aio_context_t, ctx_id, long, nr,
        struct iocb __user * __user *, iocbpp)
{
    struct kioctx *ctx;
    long ret = 0;
    int i = 0;
    struct blk_plug plug;

    if (unlikely(nr < 0))
        return -EINVAL;

    ctx = lookup_ioctx(ctx_id);  // AIO 컨텍스트 조회
    if (unlikely(!ctx))
        return -EINVAL;

    if (nr > ctx->nr_events)
        nr = ctx->nr_events;

    // ★ AIO_PLUG_THRESHOLD(2)개 이상이면 blk_plug 시작
    // 여러 bio를 모아서 한 번에 디스패치하여 성능 향상
    if (nr > AIO_PLUG_THRESHOLD)
        blk_start_plug(&plug);

    for (i = 0; i < nr; i++) {
        struct iocb __user *user_iocb;

        if (unlikely(get_user(user_iocb, iocbpp + i))) {
            ret = -EFAULT;
            break;
        }

        ret = io_submit_one(ctx, user_iocb, false);  // ★ iocb 하나씩 제출
        if (ret)
            break;
    }

    if (nr > AIO_PLUG_THRESHOLD)
        blk_finish_plug(&plug);   // 플러그 해제 → 모인 bio 일괄 디스패치

    percpu_ref_put(&ctx->users);
    return i ? i : ret;  // 성공적으로 제출한 iocb 수 반환
}
```

**io_submit_one() → __io_submit_one() 핵심 분기** (`fs/aio.c:2002`):

```c
switch (iocb->aio_lio_opcode) {
case IOCB_CMD_PREAD:
    return aio_read(&req->rw, iocb, false, compat);   // 비동기 읽기
case IOCB_CMD_PWRITE:
    return aio_write(&req->rw, iocb, false, compat);  // 비동기 쓰기
case IOCB_CMD_PREADV:
    return aio_read(&req->rw, iocb, true, compat);    // scatter 읽기
case IOCB_CMD_PWRITEV:
    return aio_write(&req->rw, iocb, true, compat);   // gather 쓰기
case IOCB_CMD_FSYNC:
    return aio_fsync(&req->fsync, iocb, false);
case IOCB_CMD_FDSYNC:
    return aio_fsync(&req->fsync, iocb, true);
case IOCB_CMD_POLL:
    return aio_poll(req, iocb);
}
```

**aio_read() 상세** (`fs/aio.c:1581`):

```c
static int aio_read(struct kiocb *req, const struct iocb *iocb,
                    bool vectored, bool compat)
{
    struct iovec inline_vecs[UIO_FASTIOV], *iovec = inline_vecs;
    struct iov_iter iter;
    struct file *file;
    int ret;

    ret = aio_prep_rw(req, iocb, READ);  // kiocb 필드 설정
    if (ret)
        return ret;

    file = req->ki_filp;
    if (unlikely(!(file->f_mode & FMODE_READ)))
        return -EBADF;
    if (unlikely(!file->f_op->read_iter))
        return -EINVAL;

    ret = aio_setup_rw(ITER_DEST, iocb, &iovec, vectored, compat, &iter);
    if (ret < 0)
        return ret;

    ret = rw_verify_area(READ, file, &req->ki_pos, iov_iter_count(&iter));
    if (!ret)
        // ★ 결국 file->f_op->read_iter()를 호출
        // 블록 디바이스: blkdev_read_iter() → blkdev_direct_IO()
        //   → __blkdev_direct_IO_async() → submit_bio()
        aio_rw_done(req, file->f_op->read_iter(req, &iter));

    kfree(iovec);
    return ret;
}
```

**aio_prep_rw()에서 중요한 설정** (`fs/aio.c:1510`):

```c
static int aio_prep_rw(struct kiocb *req, const struct iocb *iocb, int rw_type)
{
    req->ki_complete = aio_complete_rw;  // ★ 비동기 완료 콜백 등록
    req->ki_pos = iocb->aio_offset;
    req->ki_flags = req->ki_filp->f_iocb_flags | IOCB_AIO_RW;

    // ★ libaio는 폴링을 지원하지 않음
    req->ki_flags &= ~IOCB_HIPRI;  // "no one is going to poll for this I/O"

    return 0;
}
```

주목할 점: `req->ki_flags &= ~IOCB_HIPRI` 라인은 libaio가 폴링 모드를 지원하지 않음을 명시한다. 이것이 io_uring 대비 libaio의 근본적 한계이다.

### 3.3 비동기 완료 — aio_complete_rw()

블록 디바이스의 I/O가 완료되면 인터럽트 → softirq → bio->bi_end_io() 체인을 거쳐 `blkdev_bio_end_io_async()`가 호출되고, 이 함수가 `iocb->ki_complete()`를 호출한다. libaio의 경우 `ki_complete`는 `aio_complete_rw()`이다.

```c
static void aio_complete_rw(struct kiocb *kiocb, long res)
{
    struct aio_kiocb *iocb = container_of(kiocb, struct aio_kiocb, rw);

    if (!list_empty_careful(&iocb->ki_list))
        aio_remove_iocb(iocb);     // 활성 요청 목록에서 제거

    iocb->ki_res.res = res;        // 결과(바이트 수 또는 에러) 저장
    iocb->ki_res.res2 = 0;
    iocb_put(iocb);                // → aio_complete() 호출
}
```

**aio_complete()** (`fs/aio.c:1121`)는 완료 이벤트를 링 버퍼에 기록한다:

```c
static void aio_complete(struct aio_kiocb *iocb)
{
    struct kioctx *ctx = iocb->ki_ctx;
    struct aio_ring *ring;
    struct io_event *event;
    unsigned tail;

    spin_lock_irqsave(&ctx->completion_lock, flags);

    tail = ctx->tail;
    // io_event 배열에서 tail 위치에 완료 결과 기록
    event = ev_page + pos % AIO_EVENTS_PER_PAGE;
    *event = iocb->ki_res;     // ★ 완료 이벤트를 링에 기록

    smp_wmb();  // 이벤트 데이터가 먼저 보이고 나서 tail 갱신
    ctx->tail = tail;

    ring = folio_address(ctx->ring_folios[0]);
    ring->tail = tail;          // ★ 유저가 볼 수 있는 tail 포인터 갱신

    spin_unlock_irqrestore(&ctx->completion_lock, flags);

    if (waitqueue_active(&ctx->wait))
        wake_up(&ctx->wait);    // io_getevents()에서 대기 중인 태스크 깨움
}
```

### 3.4 io_getevents() — 완료 이벤트 수집

`io_getevents()`는 링 버퍼에서 완료된 이벤트를 수집한다.

```c
SYSCALL_DEFINE5(io_getevents, aio_context_t, ctx_id,
        long, min_nr, long, nr,
        struct io_event __user *, events,
        struct __kernel_timespec __user *, timeout)
{
    ...
    ret = do_io_getevents(ctx_id, min_nr, nr, events, timeout ? &ts : NULL);
    ...
}

static long do_io_getevents(aio_context_t ctx_id, long min_nr, long nr,
                            struct io_event __user *events, struct timespec64 *ts)
{
    ktime_t until = ts ? timespec64_to_ktime(*ts) : KTIME_MAX;
    struct kioctx *ioctx = lookup_ioctx(ctx_id);
    long ret = -EINVAL;

    if (likely(ioctx)) {
        if (likely(min_nr <= nr && min_nr >= 0))
            ret = read_events(ioctx, min_nr, nr, events, until);
            // ★ 링 버퍼의 head~tail 사이 이벤트를 유저 버퍼에 복사
            // min_nr 이상 이벤트가 있을 때까지 대기 (또는 타임아웃)
        percpu_ref_put(&ioctx->users);
    }
    return ret;
}
```

### 3.5 libaio 전체 흐름 요약

```
┌──────────────────────────────────────────────────────────────┐
│                    유저스페이스 (fio libaio)                   │
│                                                              │
│  1) io_setup(128, &ctx)     → AIO 컨텍스트 생성 (QD=128)     │
│  2) io_submit(ctx, nr, iocbs[])  → nr개의 I/O 비동기 제출    │
│  3) io_getevents(ctx, 1, 128, events, timeout) → 완료 수집   │
│  4) 2~3 반복                                                 │
│  5) io_destroy(ctx)         → 정리                           │
└──────────────┬───────────────────────────────────┬────────────┘
               │ io_submit()                       │ io_getevents()
               v                                   v
┌──────────────────────────────────┐  ┌─────────────────────────┐
│ io_submit_one()                  │  │ read_events()           │
│   → aio_read()/aio_write()      │  │   → aio_ring.head/tail  │
│     → file->f_op->read_iter()   │  │   → copy_to_user()      │
│       → blkdev_direct_IO()      │  └──────────┬──────────────┘
│         → submit_bio()          │             │
└──────────────┬───────────────────┘             │
               v                                 │
┌──────────────────────────────────┐             │
│ NVMe 드라이버                     │             │
│   → nvme_queue_rq()             │             │
│   → SQ doorbell                 │             │
│   ... 하드웨어 처리 ...          │             │
│   → CQ 인터럽트                  │             │
│   → nvme_irq()                  │             │
│     → blk_mq_complete_request() │             │
│       → bio->bi_end_io()        │             │
│         → aio_complete_rw()     │             │
│           → aio_complete()      │─ 이벤트 ───→│
│             → ring.tail 갱신    │  ring 기록   │
│             → wake_up()         │             │
└──────────────────────────────────┘             │
                                                 v
                                    유저가 이벤트를 읽음
```

---

## 4. io_uring 경로 (최신 비동기 I/O)

io_uring은 Linux 5.1(2019년)에 Jens Axboe가 도입한 차세대 비동기 I/O 인터페이스이다. libaio의 모든 한계를 극복하도록 설계되었으며, NVMe와의 조합에서 최고의 성능을 발휘한다.

### 4.1 io_uring 핵심 설계

```
┌─────────────────────────────────────────────────────────────────┐
│                유저스페이스 (공유 메모리 영역)                    │
│                                                                 │
│  ┌─────────────────────┐      ┌──────────────────────┐         │
│  │ SQ (Submission Q)   │      │ CQ (Completion Q)    │         │
│  │ ┌───┬───┬───┬───┐   │      │ ┌───┬───┬───┬───┐    │         │
│  │ │SQE│SQE│SQE│   │   │      │ │CQE│CQE│   │   │    │         │
│  │ └───┴───┴───┴───┘   │      │ └───┴───┴───┴───┘    │         │
│  │ head ──→     ←── tail│      │ head ──→     ←── tail│         │
│  │ (커널읽기) (유저쓰기)│      │ (유저읽기) (커널쓰기)│         │
│  └─────────────────────┘      └──────────────────────┘         │
│                                                                 │
│  SQE: opcode, fd, offset, addr, len, flags, user_data          │
│  CQE: user_data, res, flags                                    │
└────────────────────────────┬────────────────────────────────────┘
                             │ mmap 공유
┌────────────────────────────┴────────────────────────────────────┐
│                커널스페이스 (io_ring_ctx)                        │
│                                                                 │
│  io_uring_enter() 시스템 콜로 배치 제출/완료 수집                │
│  또는 SQPOLL 모드에서 커널 스레드가 자동 제출                    │
└─────────────────────────────────────────────────────────────────┘
```

**io_uring이 libaio보다 우수한 이유**:

1. **시스템 콜 최소화**: SQ/CQ가 mmap으로 공유되므로 SQE 작성과 CQE 읽기에 시스템 콜이 불필요. `SQPOLL` 모드에서는 제출까지 시스템 콜 없이 가능
2. **Buffered I/O 지원**: libaio는 O_DIRECT만 비동기 가능하지만, io_uring은 Buffered I/O도 비동기 가능 (io-wq 워커 스레드 활용)
3. **폴링 모드 (IOPOLL)**: NVMe CQ를 인터럽트 없이 직접 폴링하여 초저지연 달성
4. **NVMe passthrough**: `IORING_OP_URING_CMD`로 NVMe 커맨드를 직접 전송, 블록 레이어 우회 가능
5. **고정 버퍼/파일**: `IORING_REGISTER_BUFFERS`로 DMA 매핑을 미리 설정하여 I/O마다 매핑 오버헤드 제거

### 4.2 io_uring_setup() — 링 생성

```
유저: io_uring_setup(entries, &params)
    │
    v
SYSCALL_DEFINE2(io_uring_setup, ...)         ← io_uring/io_uring.c:3748
    │
    v
io_uring_allowed()          // 권한 확인 (sysctl_io_uring_disabled)
    │
    v
io_uring_setup(entries, params)              ← io_uring/io_uring.c:3709
    │
    v
io_uring_create(&config)
    │  1) io_ring_ctx 구조체 할당 및 초기화
    │  2) SQ/CQ 링 버퍼용 페이지 할당
    │  3) SQE 배열 할당 (entries 크기)
    │  4) CQE 배열 할당 (보통 entries의 2배)
    │  5) 파일 디스크립터 생성 (io_uring_fops)
    │  6) SQPOLL 설정 시 커널 스레드 생성
    v
fd 반환 + params에 sq_off, cq_off 정보 기록
(유저는 이 오프셋으로 mmap하여 SQ/CQ에 접근)
```

`params` 구조체에서 반환되는 `sq_off`와 `cq_off`는 유저스페이스가 mmap으로 SQ/CQ 링에 접근하기 위한 오프셋 정보를 담고 있다.

### 4.3 io_uring_enter() — SQE 처리

**소스코드 분석 - SYSCALL_DEFINE6(io_uring_enter)** (`io_uring/io_uring.c:3224`):

```c
SYSCALL_DEFINE6(io_uring_enter, unsigned int, fd, u32, to_submit,
        u32, min_complete, u32, flags, const void __user *, argp,
        size_t, argsz)
{
    struct io_ring_ctx *ctx;
    struct file *file;
    long ret;

    // ★ REGISTERED_RING 최적화: fd lookup 없이 태스크의 등록된 링에서 직접 접근
    if (flags & IORING_ENTER_REGISTERED_RING) {
        struct io_uring_task *tctx = current->io_uring;
        file = tctx->registered_rings[fd];
    } else {
        file = fget(fd);
    }

    ctx = file->private_data;

    // ★ SQPOLL 모드: 커널 스레드가 SQ를 자동으로 폴링하여 제출
    if (ctx->flags & IORING_SETUP_SQPOLL) {
        if (flags & IORING_ENTER_SQ_WAKEUP)
            wake_up(&ctx->sq_data->wait);  // SQPOLL 스레드 깨우기
        ret = to_submit;
    } else if (to_submit) {
        // ★ 일반 모드: 직접 SQE를 처리
        mutex_lock(&ctx->uring_lock);
        ret = io_submit_sqes(ctx, to_submit);  // ← SQE를 하나씩 꺼내서 처리
        ...
        mutex_unlock(&ctx->uring_lock);
    }

    // ★ IOPOLL 모드: 완료를 폴링으로 확인
    if (flags & IORING_ENTER_GETEVENTS) {
        if (ctx->syscall_iopoll) {
            mutex_lock(&ctx->uring_lock);
            ret2 = io_iopoll_check(ctx, min_complete);  // NVMe CQ를 직접 폴링
            mutex_unlock(&ctx->uring_lock);
        } else {
            // 인터럽트 기반 완료 대기
            ret2 = io_cqring_wait(ctx, min_complete, ...);
        }
    }
    ...
}
```

**io_submit_sqes()는 내부적으로 각 SQE의 opcode에 따라 적절한 핸들러를 호출한다**:

| opcode | 핸들러 | 설명 |
|--------|--------|------|
| `IORING_OP_READ` | `io_read()` | 파일/블록 디바이스 읽기 |
| `IORING_OP_WRITE` | `io_write()` | 파일/블록 디바이스 쓰기 |
| `IORING_OP_READ_FIXED` | `io_read_fixed()` | 등록된 버퍼로 읽기 |
| `IORING_OP_WRITE_FIXED` | `io_write_fixed()` | 등록된 버퍼로 쓰기 |
| `IORING_OP_URING_CMD` | `io_uring_cmd()` | NVMe passthrough 등 |

### 4.4 io_read() — io_uring 파일 읽기 경로

**소스코드 분석 - __io_read()** (`io_uring/rw.c:910`):

```c
static int __io_read(struct io_kiocb *req, struct io_br_sel *sel,
                     unsigned int issue_flags)
{
    bool force_nonblock = issue_flags & IO_URING_F_NONBLOCK;
    struct io_rw *rw = io_kiocb_to_cmd(req, struct io_rw);
    struct kiocb *kiocb = &rw->kiocb;
    ssize_t ret;

    // 버퍼 임포트 (등록 버퍼 또는 일반 유저 버퍼)
    ...

    ret = io_rw_init_file(req, FMODE_READ, READ);  // kiocb 완료 콜백 설정

    if (force_nonblock) {
        if (unlikely(!io_file_supports_nowait(req, EPOLLIN)))
            return -EAGAIN;   // NOWAIT 불가 → io-wq 워커에서 재시도
        kiocb->ki_flags |= IOCB_NOWAIT;
    }

    // ★ 결국 file->f_op->read_iter()를 호출
    // 블록 디바이스: blkdev_read_iter() → blkdev_direct_IO()
    //   → __blkdev_direct_IO_async() → submit_bio()
    ret = io_iter_do_read(rw, &io->iter);

    if (ret == -EIOCBQUEUED)
        return IOU_ISSUE_SKIP_COMPLETE;  // 비동기로 큐잉됨 → 나중에 CQE 생성
    ...
}
```

io_uring의 파일 I/O 경로는 결국 libaio와 같은 `file->f_op->read_iter()` → `blkdev_read_iter()` → `blkdev_direct_IO()`로 합류한다. 차이점은 io_uring이 SQ/CQ 공유 메모리를 통해 시스템 콜 오버헤드를 줄이고, IOPOLL 모드를 지원한다는 것이다.

### 4.5 NVMe Passthrough — io_uring_cmd() → nvme_uring_cmd()

io_uring의 가장 강력한 기능 중 하나는 `IORING_OP_URING_CMD`를 통한 디바이스 특화 커맨드 전송이다. NVMe의 경우 블록 레이어를 완전히 우회하여 NVMe 커맨드를 직접 전송할 수 있다.

**io_uring_cmd() 핸들러** (`io_uring/uring_cmd.c:230`):

```c
int io_uring_cmd(struct io_kiocb *req, unsigned int issue_flags)
{
    struct io_uring_cmd *ioucmd = io_kiocb_to_cmd(req, struct io_uring_cmd);
    struct file *file = req->file;

    if (!file->f_op->uring_cmd)     // 디바이스가 uring_cmd를 지원하는지 확인
        return -EOPNOTSUPP;

    ret = security_uring_cmd(ioucmd); // LSM 보안 검사

    // ★ 디바이스 드라이버의 uring_cmd 핸들러 호출
    // NVMe: nvme_ns_chr_uring_cmd()
    ret = file->f_op->uring_cmd(ioucmd, issue_flags);
    ...
}
```

**NVMe uring_cmd 핸들러** (`drivers/nvme/host/ioctl.c:439`):

```c
static int nvme_uring_cmd_io(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
        struct io_uring_cmd *ioucmd, unsigned int issue_flags, bool vec)
{
    struct nvme_uring_cmd_pdu *pdu = nvme_uring_cmd_pdu(ioucmd);
    const struct nvme_uring_cmd *cmd = io_uring_sqe_cmd(ioucmd->sqe);

    // ★ SQE에 임베딩된 NVMe 커맨드를 추출
    c.common.opcode = READ_ONCE(cmd->opcode);   // NVMe opcode (Read=0x02, Write=0x01 등)
    c.common.nsid = cpu_to_le32(cmd->nsid);
    c.common.cdw10 = cpu_to_le32(READ_ONCE(cmd->cdw10));
    c.common.cdw11 = cpu_to_le32(READ_ONCE(cmd->cdw11));
    c.common.cdw12 = cpu_to_le32(READ_ONCE(cmd->cdw12));
    // ... cdw2~cdw15 모두 복사

    // 권한 검사
    if (!nvme_cmd_allowed(ns, &c, 0, ioucmd->file->f_mode & FMODE_WRITE))
        return -EACCES;

    d.addr = READ_ONCE(cmd->addr);
    d.data_len = READ_ONCE(cmd->data_len);
    d.timeout_ms = READ_ONCE(cmd->timeout_ms);

    // 고정 버퍼 사용 시 DMA 매핑 생략 (사전 등록)
    if (d.data_len && (ioucmd->flags & IORING_URING_CMD_FIXED)) {
        ret = io_uring_cmd_import_fixed(d.addr, d.data_len, ddir, &iter, ioucmd, issue_flags);
        map_iter = &iter;
    }

    if (issue_flags & IO_URING_F_IOPOLL)
        rq_flags |= REQ_POLLED;     // ★ 폴링 모드 설정

    // ★ blk-mq request를 직접 할당 (블록 레이어의 bio 경로를 우회!)
    req = nvme_alloc_user_request(q, &c, rq_flags, blk_flags);

    if (d.data_len) {
        ret = nvme_map_user_request(req, d.addr, d.data_len, ...);
    }

    pdu->bio = req->bio;
    pdu->req = req;
    req->end_io_data = ioucmd;
    req->end_io = nvme_uring_cmd_end_io;  // ★ 완료 콜백 등록

    blk_execute_rq_nowait(req, false);     // ★ request를 즉시 디스패치 (스케줄러 우회)
    return -EIOCBQUEUED;
}
```

**핵심 차이점**: io_uring_cmd 경로는 `submit_bio()`를 호출하지 않는다. 대신 `nvme_alloc_user_request()`로 blk-mq request를 직접 할당하고 `blk_execute_rq_nowait()`로 즉시 디스패치한다. 이는 bio → request 변환, I/O 머지, I/O 스케줄러 등 블록 레이어의 모든 오버헤드를 우회한다.

### 4.6 완료 처리 — CQE 생성

io_uring의 I/O 완료는 세 가지 경로로 처리된다:

**경로 1: 인터럽트 기반 (일반)**
```
NVMe CQ 인터럽트 → nvme_irq() → nvme_process_cq()
  → blk_mq_complete_request()
    → bio->bi_end_io() → blkdev_bio_end_io_async()
      → iocb->ki_complete() → io_complete_rw()
        → io_req_task_complete() → CQE 생성 및 ring.tail 갱신
```

**경로 2: IOPOLL (폴링)**
```
io_uring_enter(IORING_ENTER_GETEVENTS) → io_iopoll_check()
  → io_do_iopoll()
    → iocb_bio_iopoll() → bio_poll() → blk_mq_poll()
      → nvme_poll() — NVMe CQ를 직접 읽어서 완료 확인
    → (완료 확인 시) req->iopoll_completed = 1
    → CQE 생성
```

**경로 3: io_uring_cmd 완료**
```
NVMe 완료 → nvme_uring_cmd_end_io()
  → io_uring_cmd_do_in_task_lazy(ioucmd, nvme_uring_task_cb)
    → nvme_uring_task_cb()
      → io_uring_cmd_done32() → CQE 생성
```

### 4.7 Polled I/O (IORING_SETUP_IOPOLL)

IOPOLL은 NVMe에서 초저지연을 달성하기 위한 핵심 기능이다. 인터럽트를 사용하지 않고 CPU가 직접 NVMe CQ를 폴링하여 완료를 확인한다.

**io_iopoll_check()** (`io_uring/io_uring.c:1586`):

```c
static int io_iopoll_check(struct io_ring_ctx *ctx, unsigned int min_events)
{
    unsigned int nr_events = 0;

    // 이미 완료된 이벤트가 있으면 즉시 반환
    if (io_cqring_events(ctx))
        return 0;

    do {
        int ret = 0;

        // ★ 미완료 I/O 목록을 순회하며 폴링
        ret = io_do_iopoll(ctx, !min_events);
        if (unlikely(ret < 0))
            return ret;

        if (task_sigpending(current))
            return -EINTR;
        if (need_resched())
            break;

        nr_events += ret;
    } while (nr_events < min_events);   // min_events만큼 완료될 때까지 반복

    return 0;
}
```

IOPOLL의 성능 이점:
- **인터럽트 제거**: MSI-X 인터럽트 처리 오버헤드(수 마이크로초) 제거
- **컨텍스트 스위칭 제거**: softirq/workqueue 전환 없이 동일 CPU에서 완료 처리
- **캐시 효율**: 데이터와 메타데이터가 동일 CPU 캐시에 유지
- NVMe 4K 랜덤 읽기에서 인터럽트 방식 대비 20~30% 지연 시간 감소

---

## 5. 블록 디바이스 Direct I/O vs Buffered I/O

### 5.1 두 경로의 ASCII 비교 다이어그램

```
                      ┌──────────────────┐
                      │ 유저 애플리케이션 │
                      │  (fio 등)        │
                      └────────┬─────────┘
                               │
                    ┌──────────┴──────────┐
                    │                     │
              O_DIRECT 플래그            일반 open()
              (direct=1)                (direct=0)
                    │                     │
                    v                     v
          ┌─────────────────┐   ┌──────────────────┐
          │  Direct I/O     │   │  Buffered I/O    │
          │  경로           │   │  경로            │
          └────────┬────────┘   └────────┬─────────┘
                   │                     │
                   │                     v
                   │            ┌──────────────────┐
                   │            │ 페이지 캐시       │
                   │            │ (Page Cache)     │
                   │            │                  │
                   │            │ 읽기: 캐시히트   │
                   │            │  → 즉시 반환     │
                   │            │ 읽기: 캐시미스   │
                   │            │  → readahead ────┼──┐
                   │            │                  │  │
                   │            │ 쓰기: 페이지에   │  │
                   │            │  기록 후 반환    │  │
                   │            │  → dirty 마킹    │  │
                   │            │  → writeback ────┼──┤
                   │            └──────────────────┘  │
                   │                                  │
                   │  bio 생성                bio 생성│
                   │  (유저 버퍼 → bio_vec)   (페이지 캐시 → bio_vec)
                   │                                  │
                   v                                  v
          ┌──────────────────────────────────────────────┐
          │              submit_bio()                     │
          │  → submit_bio_noacct()    (유효성 검사)       │
          │    → submit_bio_noacct_nocheck()              │
          │      → __submit_bio_noacct_mq()              │
          │        → __submit_bio()                      │
          │          → blk_mq_submit_bio()               │
          │            → nvme_queue_rq()                  │
          │              → NVMe SQ doorbell              │
          └──────────────────────────────────────────────┘
```

### 5.2 Direct I/O의 특성

- **페이지 캐시 우회**: 유저 버퍼의 물리 페이지를 직접 DMA에 사용
- **정렬 요구**: 오프셋과 크기가 논리 블록 크기(보통 512B 또는 4KB)의 배수여야 함
- **즉각적 I/O**: `submit_bio()` 호출 시 즉시 디바이스로 전달
- **제로 카피 가능**: 유저 버퍼 → DMA → NVMe 디바이스 (중간 복사 없음)

### 5.3 Buffered I/O의 특성

- **페이지 캐시 경유**: 읽기 시 캐시 히트하면 디스크 I/O 없음
- **쓰기 지연**: dirty 페이지가 쌓이다가 writeback daemon이 일괄 flush
- **readahead**: 순차 읽기 패턴 감지 시 미리 읽어와 캐시에 적재
- **데이터 복사**: 유저 버퍼 ↔ 페이지 캐시 사이에 `copy_to_user()`/`copy_from_user()` 발생

### 5.4 NVMe 벤치마크에서 Direct I/O를 사용하는 이유

1. **순수 디바이스 성능 측정**: 페이지 캐시 히트는 메모리 속도를 측정하는 것이지 NVMe 성능이 아님
2. **일관된 지연 시간**: Buffered I/O는 writeback 시점에 따라 지연 시간이 크게 변동
3. **높은 큐 깊이**: Direct I/O의 비동기 변형(`__blkdev_direct_IO_async`)은 여러 I/O를 동시에 디바이스에 전달 가능
4. **메모리 절약**: 대용량 I/O 테스트 시 페이지 캐시가 시스템 메모리를 소진하는 것을 방지
5. **실제 워크로드 반영**: 데이터베이스(MySQL/PostgreSQL의 O_DIRECT 모드)나 스토리지 시스템은 주로 Direct I/O 사용

---

## 6. nvme-cli 경로 (ioctl)

nvme-cli는 NVMe 관리 유틸리티로, NVMe 스펙에 정의된 Admin/IO 커맨드를 직접 구성하여 전송한다. 블록 레이어를 완전히 우회하는 경로이다.

### 6.1 ioctl 기반 I/O 커맨드

```
유저: nvme read /dev/nvme0n1 --start-block=0 --block-count=7
    │
    v
nvme-cli가 NVMe Read 커맨드 구조체를 직접 구성
    │  opcode = 0x02 (Read)
    │  nsid = 네임스페이스 ID
    │  cdw10/cdw11 = 시작 LBA
    │  cdw12 = 블록 수 - 1
    │  data_addr = 유저 버퍼 주소
    v
ioctl(fd, NVME_IOCTL_IO_CMD, &cmd)
    │
    v
blkdev_ioctl()                        ← block/ioctl.c
    │
    v
nvme_ioctl()                          ← drivers/nvme/host/ioctl.c
    │
    v
nvme_user_cmd()
    │  1) NVMe 커맨드 구조체 복사 (copy_from_user)
    │  2) nvme_alloc_user_request() — blk-mq request 할당
    │  3) nvme_map_user_request() — 유저 버퍼를 DMA 매핑
    │  4) blk_execute_rq() — request 제출 및 완료 대기 (동기)
    v
nvme_queue_rq() → SQ doorbell → 하드웨어 → CQ 인터럽트 → 완료
```

### 6.2 ioctl 경로의 특성

**블록 레이어를 우회하는 이유**:
- Admin 커맨드(Identify, Get Log Page, Format 등)는 블록 I/O가 아니므로 블록 레이어를 거칠 수 없음
- Vendor Specific 커맨드는 커널 블록 레이어가 알 수 없는 커맨드
- NVMe 스펙 레벨의 세밀한 제어가 필요 (예: 특정 CDW 값 설정)

**ioctl vs io_uring_cmd 비교**:
- ioctl: 동기식 (`blk_execute_rq()`로 완료 대기), 단일 커맨드만 제출 가능
- io_uring_cmd: 비동기식 (`blk_execute_rq_nowait()`), 배치 제출 가능, 폴링 지원

---

## 7. 모든 경로의 통합 비교표

| 특성 | sync (read/write) | libaio (io_submit) | io_uring (io_uring_enter) | io_uring_cmd (passthrough) | nvme-cli (ioctl) |
|------|-------|--------|----------|-------------|----------|
| **시스템 콜** | read()/write() | io_submit()/io_getevents() | io_uring_enter() (또는 0회) | io_uring_enter() (또는 0회) | ioctl() |
| **I/O당 시스템 콜 수** | 1회 | 제출 1회 + 수집 1회 (배치 가능) | 0~1회 (배치, SQPOLL) | 0~1회 (배치, SQPOLL) | 1회 |
| **비동기 여부** | 동기 (블로킹) | 비동기 | 비동기 | 비동기 | 동기 (블로킹) |
| **최대 큐 깊이** | 1 (스레드당) | 수천 | 수천 | 수천 | 1 |
| **I/O 스케줄러 경유** | 예 | 예 | 예 (파일 I/O) | 아니오 | 아니오 |
| **블록 레이어 경유** | 예 (submit_bio) | 예 (submit_bio) | 예 (submit_bio) | 아니오 (blk_execute_rq_nowait) | 아니오 (blk_execute_rq) |
| **배치 제출** | 불가 | 가능 (blk_plug) | 가능 (SQ 링) | 가능 (SQ 링) | 불가 |
| **폴링 모드** | 불가 | 불가 (IOCB_HIPRI 강제 해제) | 가능 (IOPOLL) | 가능 (IOPOLL) | 불가 |
| **Direct I/O 필수** | 아니오 | 예 (비동기 필수조건) | 아니오 | 해당 없음 | 해당 없음 |
| **고정 버퍼** | 불가 | 불가 | 가능 (REGISTER_BUFFERS) | 가능 (URING_CMD_FIXED) | 불가 |
| **NVMe 커맨드 직접 구성** | 불가 | 불가 | 불가 (파일 I/O) | 가능 | 가능 |
| **적합한 용도** | 간단한 테스트, 디버깅 | 높은 IOPS 벤치마크 | 최고 성능 벤치마크, 프로덕션 | NVMe 최적화, 초저지연 | NVMe 관리, 진단 |
| **커널 버전 요구** | 모든 버전 | 2.5+ | 5.1+ | 5.19+ | 모든 버전 |
| **fio 엔진명** | sync, psync | libaio | io_uring | io_uring_cmd | (fio 아님) |

---

## 8. 전체 경로 ASCII 다이어그램

모든 I/O 경로가 최종적으로 NVMe 하드웨어에 도달하는 과정을 하나의 통합 다이어그램으로 나타낸다.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          유저스페이스 애플리케이션                            │
│                                                                             │
│  fio sync     fio psync    fio libaio    fio io_uring   fio io_uring_cmd   │
│  nvme-cli     dd           database      SPDK(비교용)                       │
└──┬──────────┬───────────┬────────────┬──────────────┬──────────────────┬────┘
   │          │           │            │              │                  │
   │read()    │pread()    │io_submit() │io_uring      │io_uring          │ioctl()
   │write()   │pwrite()   │            │_enter()      │_enter()          │
   │          │           │            │              │(URING_CMD)       │
   v          v           v            v              v                  v
┌─────────────────────────────────────────────────────────────────────────────┐
│                        시스템 콜 레이어                                      │
│                                                                             │
│  ksys_read()  ksys_pread64()  sys_io_submit()  sys_io_uring_enter()  ioctl()│
└──┬──────────┬───────────┬────────────┬──────────────┬──────────────────┬────┘
   │          │           │            │              │                  │
   v          v           v            v              │                  │
┌──────────────────────────────────────────────┐      │                  │
│              VFS 레이어                       │      │                  │
│                                              │      │                  │
│  vfs_read() / vfs_write()                    │      │                  │
│    → file->f_op->read_iter()                 │      │                  │
│    → file->f_op->write_iter()                │      │                  │
│                                              │      │                  │
│  ┌────────────────────────────────────────┐   │      │                  │
│  │ blkdev_read_iter() / blkdev_write_iter()│  │      │                  │
│  │                                         │  │      │                  │
│  │   IOCB_DIRECT?─────┐                   │  │      │                  │
│  │   │ Yes             │ No                │  │      │                  │
│  │   v                 v                   │  │      │                  │
│  │  blkdev_         filemap_read()         │  │      │                  │
│  │  direct_IO()     blkdev_buffered_write()│  │      │                  │
│  │   │                 │                   │  │      │                  │
│  │   │              ┌──┴──────────┐        │  │      │                  │
│  │   │              │ 페이지 캐시  │        │  │      │                  │
│  │   │              │ (읽기: 캐시히트→반환) │  │      │                  │
│  │   │              │ (쓰기: dirty→나중에)  │  │      │                  │
│  │   │              └──┬──────────┘        │  │      │                  │
│  │   │                 │ (writeback/       │  │      │                  │
│  │   │                 │  readahead 시)    │  │      │                  │
│  └───┼─────────────────┼──────────────────┘  │      │                  │
└──────┼─────────────────┼─────────────────────┘      │                  │
       │                 │                             │                  │
       │  ┌──────────────┘                             │                  │
       │  │    bio 생성                                │                  │
       v  v                                            │                  │
┌──────────────────────────────────────────┐           │                  │
│          submit_bio()                     │           │                  │
│            ← blk-core.c:1244             │           │                  │
│                                          │           │                  │
│  1) bio_set_ioprio() — I/O 우선순위 설정  │           │                  │
│  2) submit_bio_noacct() — 유효성 검사     │           │                  │
│     - bio_check_ro() — 읽기 전용 검사    │           │                  │
│     - bio_check_eod() — 디바이스 끝 검사 │           │                  │
│     - blk_partition_remap() — 파티션 리매핑│          │                  │
│     - blk_throtl_bio() — cgroup 스로틀링  │           │                  │
│  3) submit_bio_noacct_nocheck()          │           │                  │
│     - blk_cgroup_bio_start()             │           │                  │
│     - trace_block_bio_queue()            │           │                  │
│  4) __submit_bio_noacct_mq() — NVMe 경로 │           │                  │
│     - __submit_bio()                     │           │                  │
│       - blk_start_plug()                 │           │                  │
│       - blk_mq_submit_bio()  ←────────────────┐     │                  │
│       - blk_finish_plug()                │     │     │                  │
└──────────────────────────────────────────┘     │     │                  │
                                                 │     │                  │
┌────────────────────────────────────────────────┼─────┼──────────────────┤
│              blk-mq 레이어                      │     │                  │
│                                                │     │                  │
│  blk_mq_submit_bio()                          │     │                  │
│    1) bio → request 변환                       │     │                  │
│    2) I/O 머지 시도 (plug list, 스케줄러)      │     │                  │
│    3) 스케줄러 (mq-deadline, kyber, none 등)   │     │                  │
│    4) blk_mq_dispatch_rq_list()               │     │                  │
│       → .queue_rq() 콜백                ←──────┘     │                  │
│                                                      │                  │
│  blk_execute_rq_nowait()  ←──────────────────────────┤                  │
│    - request를 직접 디스패치 (스케줄러 우회)           │  io_uring_cmd     │
│    → .queue_rq() 콜백                                │                  │
│                                                      │                  │
│  blk_execute_rq()  ←────────────────────────────────────────────────────┤
│    - request를 디스패치하고 완료까지 대기              │  ioctl           │
│    → .queue_rq() 콜백                                │                  │
│                                                      │                  │
└──────────────────────────┬───────────────────────────┘                  │
                           │ .queue_rq()                                  │
                           v                                              │
┌─────────────────────────────────────────────────────────────────────────┘
│                   NVMe 드라이버 (drivers/nvme/host/)
│
│  nvme_queue_rq()
│    1) NVMe SQE(Submission Queue Entry) 구성
│       - opcode, NSID, LBA, 블록 수 등
│       - PRP/SGL — DMA 주소 목록
│    2) SQ tail 포인터에 SQE 기록
│    3) ★ SQ doorbell 레지스터에 새 tail 값 기록
│       → PCIe MMIO write → NVMe 컨트롤러가 SQE를 fetch
│
│  ───────────── NVMe 컨트롤러가 I/O 실행 ─────────────
│
│  완료 경로 (인터럽트 방식):
│    NVMe CQ에 CQE 기록 → MSI-X 인터럽트 발생
│    → nvme_irq() → nvme_process_cq()
│      → nvme_handle_cqe()
│        → nvme_end_req() → blk_mq_end_request()
│          → blk_mq_complete_request()
│            → bio->bi_end_io() 또는 req->end_io()
│              → 유저에게 완료 통지
│
│  완료 경로 (폴링 방식 — io_uring IOPOLL):
│    io_uring_enter(GETEVENTS) → io_iopoll_check()
│      → io_do_iopoll() → blk_mq_poll()
│        → nvme_poll() — CQ를 직접 읽어서 완료 확인
│          → nvme_handle_cqe()
│            → blk_mq_end_request()
│              → CQE를 io_uring CQ 링에 기록
│
└─────────────────────────────────────────────────────────────────────────┘
```

### 8.1 경로별 커널 함수 호출 깊이 비교

각 I/O 경로가 NVMe doorbell에 도달하기까지 거치는 커널 함수의 대략적인 깊이를 비교한다:

```
sync read (Direct I/O):
  read() → ksys_read() → vfs_read() → blkdev_read_iter()
  → blkdev_direct_IO() → __blkdev_direct_IO_simple()
  → submit_bio_wait() → submit_bio() → submit_bio_noacct()
  → submit_bio_noacct_nocheck() → __submit_bio_noacct_mq()
  → __submit_bio() → blk_mq_submit_bio() → nvme_queue_rq()
  ≈ 13단계

libaio read (Direct I/O):
  io_submit() → io_submit_one() → __io_submit_one()
  → aio_read() → blkdev_read_iter()
  → blkdev_direct_IO() → __blkdev_direct_IO_async()
  → submit_bio() → ... → nvme_queue_rq()
  ≈ 13단계 (async 완료는 별도)

io_uring read (Direct I/O):
  io_uring_enter() → io_submit_sqes() → io_read()
  → __io_read() → io_iter_do_read() → blkdev_read_iter()
  → blkdev_direct_IO() → __blkdev_direct_IO_async()
  → submit_bio() → ... → nvme_queue_rq()
  ≈ 14단계 (but 시스템 콜 수 최소)

io_uring_cmd (NVMe passthrough):
  io_uring_enter() → io_submit_sqes() → io_uring_cmd()
  → nvme_ns_chr_uring_cmd() → nvme_uring_cmd_io()
  → nvme_alloc_user_request() → blk_execute_rq_nowait()
  → nvme_queue_rq()
  ≈ 8단계 (블록 레이어 우회!)

ioctl:
  ioctl() → blkdev_ioctl() → nvme_ioctl()
  → nvme_user_cmd() → blk_execute_rq()
  → nvme_queue_rq()
  ≈ 6단계 (but 동기식)
```

io_uring_cmd가 함수 호출 깊이가 가장 얕으면서도 비동기 + 배치 + 폴링을 지원하므로, NVMe에서 이론적 최고 성능을 달성할 수 있다.

### 8.2 fio 벤치마크 권장 설정

NVMe SSD의 최대 성능을 측정하기 위한 fio 설정 예시:

```ini
# 최대 IOPS 측정 (io_uring, Direct I/O, IOPOLL)
[global]
ioengine=io_uring
direct=1
hipri=1              # IOPOLL 활성화
sqthread_poll=1      # SQPOLL 커널 스레드 활성화
fixedbufs=1          # 고정 버퍼 등록
registerfiles=1      # 파일 디스크립터 등록
bs=4k
rw=randread
iodepth=128
numjobs=1
filename=/dev/nvme0n1

# NVMe passthrough 최대 성능
[global]
ioengine=io_uring_cmd
cmd_type=nvme
direct=1
hipri=1
fixedbufs=1
bs=4k
rw=randread
iodepth=128
numjobs=1
filename=/dev/ng0n1   # NVMe character device (/dev/ngXnY)
```

`/dev/nvme0n1`(블록 디바이스)과 `/dev/ng0n1`(NVMe 캐릭터 디바이스)의 차이:
- `/dev/nvme0n1`: 블록 레이어를 거치는 일반 I/O
- `/dev/ng0n1`: io_uring_cmd로 NVMe 커맨드를 직접 전송하는 passthrough I/O

---

## 요약

### 핵심 인사이트

1. **모든 일반 I/O 경로는 결국 submit_bio()로 합류한다**: sync, libaio, io_uring(파일 I/O) 모두 `blkdev_read_iter()` → `blkdev_direct_IO()` → `submit_bio()` → `blk_mq_submit_bio()` → `nvme_queue_rq()` 체인을 거친다.

2. **io_uring_cmd와 ioctl은 블록 레이어를 우회한다**: `submit_bio()` 대신 `blk_execute_rq_nowait()`/`blk_execute_rq()`로 request를 직접 디스패치한다. bio → request 변환, I/O 머지, I/O 스케줄러 등의 오버헤드가 없다.

3. **성능 차이의 근본 원인은 시스템 콜 오버헤드와 폴링 지원 여부이다**:
   - sync: I/O당 시스템 콜 1회, 큐 깊이 1, 인터럽트 기반
   - libaio: 배치 가능하지만 여전히 시스템 콜 필요, 폴링 불가
   - io_uring: 시스템 콜 최소화(SQPOLL), 폴링 가능(IOPOLL)
   - io_uring_cmd: 블록 레이어까지 우회, 폴링 가능

4. **NVMe 성능 극대화 공식**: `io_uring_cmd` + `IOPOLL` + `SQPOLL` + `고정 버퍼` + `Direct I/O` = NVMe 하드웨어의 이론적 최대 성능에 근접

### notes/ 폴더 정리용 요약

`notes/blk-mq.md`에 추가할 내용:
- fio 엔진별 커널 진입 경로 차이
- Direct I/O의 세 가지 변형: `__blkdev_direct_IO_simple`, `__blkdev_direct_IO_async`, `__blkdev_direct_IO`
- libaio의 IOCB_HIPRI 강제 해제로 인한 폴링 미지원
- io_uring_cmd가 `blk_execute_rq_nowait()`로 블록 레이어를 우회하는 메커니즘
