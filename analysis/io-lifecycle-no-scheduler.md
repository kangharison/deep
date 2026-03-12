# Linux NVMe I/O 생애주기 완전 분석 (No I/O Scheduler)

> Application → VFS → Block Layer → blk-mq → NVMe Driver → Hardware → Completion
> 이 문서는 I/O 스케줄러가 없는(none) 상태에서의 전체 I/O 경로를 코드 레벨로 추적한다.

---

## 목차

1. [전체 아키텍처 개요](#1-전체-아키텍처-개요)
2. [큐 매핑 관계](#2-큐-매핑-관계)
3. [I/O 제출 경로: submit_bio() 진입](#3-io-제출-경로-submit_bio-진입)
4. [blk-mq 진입: blk_mq_submit_bio()](#4-blk-mq-진입-blk_mq_submit_bio)
5. [Request 할당과 Tag](#5-request-할당과-tag)
6. [Bio → Request 변환](#6-bio--request-변환)
7. [Bio Merge 메커니즘](#7-bio-merge-메커니즘)
8. [Plug 메커니즘: 배치 최적화](#8-plug-메커니즘-배치-최적화)
9. [Direct Dispatch: 최저 지연 경로](#9-direct-dispatch-최저-지연-경로)
10. [NVMe Driver: nvme_queue_rq()](#10-nvme-driver-nvme_queue_rq)
11. [NVMe 커맨드 구성: nvme_setup_cmd()](#11-nvme-커맨드-구성-nvme_setup_cmd)
12. [DMA 매핑: PRP/SGL 구성](#12-dma-매핑-prpsgl-구성)
13. [SQ Doorbell과 배치 최적화](#13-sq-doorbell과-배치-최적화)
14. [인터럽트와 완료 경로](#14-인터럽트와-완료-경로)
15. [Request 상태 머신](#15-request-상태-머신)
16. [에러 처리와 재시도](#16-에러-처리와-재시도)
17. [Tag 반환과 Request 해제](#17-tag-반환과-request-해제)
18. [전체 Call Chain 요약](#18-전체-call-chain-요약)

---

## 1. 전체 아키텍처 개요

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application                               │
│                    (read/write syscall)                           │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     VFS (Virtual File System)                    │
│                   ext4_file_write_iter() 등                      │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Page Cache / Direct I/O                       │
│              bio 구조체 생성 (섹터, 길이, 페이지 목록)            │
└──────────────────────────┬──────────────────────────────────────┘
                           │ submit_bio(bio)
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Block Layer (blk-core.c)                       │
│              유효성 검사, 파티션 리맵, flush 최적화               │
└──────────────────────────┬──────────────────────────────────────┘
                           │ blk_mq_submit_bio(bio)
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                      blk-mq (blk-mq.c)                          │
│                                                                  │
│  ┌─────────┐   ┌──────────────┐   ┌───────────────────────┐    │
│  │  Merge   │──▶│ Tag 할당     │──▶│ bio→request 변환       │    │
│  │  시도    │   │ (sbitmap)    │   │ blk_mq_bio_to_request │    │
│  └─────────┘   └──────────────┘   └───────────┬───────────┘    │
│                                                │                 │
│                    ┌───────────────────────────┤                 │
│                    │                           │                 │
│              ┌─────▼─────┐            ┌────────▼────────┐       │
│              │  Plug에    │            │  Direct Dispatch │       │
│              │  버퍼링    │            │  (최저 지연)     │       │
│              └─────┬─────┘            └────────┬────────┘       │
│                    │                           │                 │
│                    └───────────┬───────────────┘                 │
│                                │                                 │
│                    ┌───────────▼───────────┐                    │
│                    │   queue_rq() 콜백     │                    │
│                    │   = nvme_queue_rq()   │                    │
│                    └───────────┬───────────┘                    │
└────────────────────────────────┼────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    NVMe PCIe Driver (pci.c)                      │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐    │
│  │ nvme_setup_  │  │ nvme_map_    │  │ nvme_sq_copy_cmd() │    │
│  │ cmd()        │──▶│ data()       │──▶│ + SQ Doorbell      │    │
│  │ (커맨드 구성) │  │ (PRP/SGL)   │  │ (MMIO 기록)        │    │
│  └──────────────┘  └──────────────┘  └────────┬───────────┘    │
└────────────────────────────────────────────────┼────────────────┘
                                                 │
                                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    NVMe Controller (Hardware)                    │
│                                                                  │
│  SQ Tail Doorbell ──▶ SQ에서 SQE Fetch ──▶ Flash/DRAM 접근     │
│                                              │                   │
│                                              ▼                   │
│                              CQE를 CQ에 기록 + MSI-X 인터럽트   │
└──────────────────────────────────────┬──────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Completion Path (역순)                        │
│                                                                  │
│  nvme_irq() → nvme_handle_cqe() → nvme_complete_rq()           │
│  → blk_mq_end_request() → bio_endio() → 파일시스템 완료        │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. 큐 매핑 관계

### 2.1 전체 큐 구조 (No Scheduler)

```
CPU 0 ──▶ Software Queue (ctx) ──┐
CPU 1 ──▶ Software Queue (ctx) ──┤──▶ Hardware Queue (hctx 0) ──▶ NVMe I/O Queue 1 (SQ1/CQ1)
CPU 2 ──▶ Software Queue (ctx) ──┘
CPU 3 ──▶ Software Queue (ctx) ──┐
CPU 4 ──▶ Software Queue (ctx) ──┤──▶ Hardware Queue (hctx 1) ──▶ NVMe I/O Queue 2 (SQ2/CQ2)
CPU 5 ──▶ Software Queue (ctx) ──┘
...
                                       Hardware Queue (hctx N) ──▶ NVMe I/O Queue N+1

별도:  Admin Queue (hctx) ──▶ NVMe Admin Queue (SQ0/CQ0)
```

### 2.2 CPU → hctx 매핑 코드

NVMe 드라이버가 `blk_mq_alloc_tag_set()`을 호출하면, blk-mq가 CPU를 하드웨어 큐에 매핑한다.

```c
/* drivers/nvme/host/pci.c - nvme_setup_irqs()에서 결정되는 큐 수 */
/* 이상적인 경우: CPU 수 == NVMe I/O 큐 수 (1:1 매핑) */

/* include/linux/blk-mq.h */
struct blk_mq_tag_set {
    unsigned int nr_hw_queues;   /* NVMe I/O 큐 수 (nvme_setup_io_queues에서 결정) */
    struct blk_mq_queue_map map[HCTX_MAX_TYPES]; /* CPU→hctx 매핑 테이블 */
    /* map[0] = DEFAULT (일반 read/write)
     * map[1] = READ    (read 전용 큐, NVMe의 write_queues 파라미터 사용 시)
     * map[2] = POLL    (폴링 큐, NVMe의 poll_queues 파라미터 사용 시) */
};

struct blk_mq_queue_map {
    unsigned int *mq_map;     /* mq_map[cpu_id] = hctx_index */
    unsigned int nr_queues;   /* 이 타입의 큐 수 */
    unsigned int queue_offset; /* 큐 번호 오프셋 */
};
```

**매핑 결정 과정:**

```c
/* block/blk-mq.c */
static struct blk_mq_hw_ctx *blk_mq_map_queue(blk_opf_t cmd_flags,
                                                struct blk_mq_ctx *ctx)
{
    /* cmd_flags에서 큐 타입 결정 */
    enum hctx_type type = cyclic ? HCTX_TYPE_DEFAULT :
                          (cmd_flags & REQ_POLLED) ? HCTX_TYPE_POLL :
                          ((cmd_flags & REQ_OP_MASK) == REQ_OP_READ) ? HCTX_TYPE_READ :
                          HCTX_TYPE_DEFAULT;

    /* CPU 번호로 hctx 인덱스 조회 */
    return ctx->hctxs[type];
    /* 내부적으로: set->map[type].mq_map[cpu_id] → hctx index */
}
```

**NVMe에서의 큐 타입 분배 예시:**

```
총 8개 NVMe I/O 큐, write_queues=2, poll_queues=1인 경우:

  DEFAULT(write) 큐: 2개 (hctx 0-1)  → NVMe QID 1-2
  READ 큐:           5개 (hctx 2-6)  → NVMe QID 3-7
  POLL 큐:           1개 (hctx 7)    → NVMe QID 8
```

### 2.3 hctx와 NVMe Queue의 연결

```c
/* drivers/nvme/host/pci.c */
static int nvme_init_hctx(struct blk_mq_hw_ctx *hctx, void *data,
                          unsigned int hctx_idx)
{
    struct nvme_dev *dev = to_nvme_dev(data);
    struct nvme_queue *nvmeq = &dev->queues[hctx_idx + 1]; /* +1: QID 0은 Admin */

    /* ★ 핵심 연결: hctx->driver_data에 NVMe 큐 포인터 저장 */
    hctx->driver_data = nvmeq;
    return 0;
}

/* nvme_queue_rq()에서 사용: */
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx, ...)
{
    struct nvme_queue *nvmeq = hctx->driver_data;  /* hctx → NVMe 큐 */
    /* 이 nvmeq가 실제 하드웨어 SQ/CQ를 가리킨다 */
}
```

### 2.4 Namespace와 Queue의 관계

```
NVMe Controller (nvme_ctrl)
  ├── tagset (blk_mq_tag_set)          ← 모든 NS가 공유
  │     ├── hctx[0] → nvme_queue[1]
  │     ├── hctx[1] → nvme_queue[2]
  │     └── hctx[N] → nvme_queue[N+1]
  │
  ├── Namespace 1 (nvme_ns)
  │     ├── queue (request_queue) ──── tagset을 참조
  │     └── disk  (gendisk: /dev/nvme0n1)
  │
  └── Namespace 2 (nvme_ns)
        ├── queue (request_queue) ──── 같은 tagset을 참조
        └── disk  (gendisk: /dev/nvme0n2)

★ 핵심: 같은 컨트롤러의 모든 NS는 같은 tagset(= 같은 NVMe I/O 큐들)을 공유한다.
  따라서 NS1과 NS2의 I/O는 같은 물리적 SQ/CQ를 사용한다.
  태그 공간도 공유하므로, NS1이 태그를 많이 사용하면 NS2가 사용할 수 있는 태그가 줄어든다.
```

---

## 3. I/O 제출 경로: submit_bio() 진입

### 3.1 파일시스템에서 bio 생성

파일시스템(ext4 등)이 I/O를 수행할 때 `bio` 구조체를 생성하여 `submit_bio()`를 호출한다.

```c
/* bio의 핵심 필드 */
struct bio {
    struct block_device *bi_bdev;     /* 대상 블록 디바이스 (/dev/nvme0n1) */
    blk_opf_t           bi_opf;      /* 연산 타입 + 플래그 (REQ_OP_READ | REQ_SYNC 등) */
    struct bvec_iter    bi_iter;      /* 현재 위치 (섹터, 남은 크기) */
    struct bio_vec     *bi_io_vec;    /* 데이터 페이지 배열 [{page, offset, len}, ...] */
    unsigned short      bi_vcnt;      /* bi_io_vec 엔트리 수 */
    bio_end_io_t       *bi_end_io;    /* 완료 콜백 (파일시스템으로 돌아감) */
    void               *bi_private;   /* 완료 콜백에 전달할 private 데이터 */
};
```

### 3.2 submit_bio() → blk-mq 까지의 호출 체인

```c
/* block/blk-core.c:1244 */
void submit_bio(struct bio *bio)
{
    /* I/O 통계: read면 PGPGIN, write면 PGPGOUT 카운터 증가 */
    if (bio_op(bio) == REQ_OP_READ)
        task_io_account_read(bio->bi_iter.bi_size);
    else if (bio_op(bio) == REQ_OP_WRITE)
        count_vm_events(PGPGOUT, bio_sectors(bio));

    bio_set_ioprio(bio);         /* 프로세스의 I/O 우선순위를 bio에 설정 */
    submit_bio_noacct(bio);      /* 실제 제출 시작 */
}
```

### 3.3 유효성 검사 단계

```c
/* block/blk-core.c:1064 */
void submit_bio_noacct(struct bio *bio)
{
    struct block_device *bdev = bio->bi_bdev;
    struct request_queue *q = bdev_get_queue(bdev);

    /* 검사 1: NOWAIT 요청인데 디바이스가 NOWAIT를 지원하지 않으면 거부 */
    if ((bio->bi_opf & REQ_NOWAIT) && !bdev_nowait(bdev))
        goto not_supported;

    /* 검사 2: fault injection (테스트용) */
    if (should_fail_bio(bio))
        goto end_io;

    /* 검사 3: 읽기 전용 디바이스에 쓰기 시도 */
    bio_check_ro(bio);

    /* 검사 4: 디바이스 끝(End Of Device)을 넘는 I/O */
    if (unlikely(bio_check_eod(bio)))
        goto end_io;

    /* 검사 5: 파티션 리맵 - 파티션 내 오프셋을 디스크 전체 오프셋으로 변환 */
    if (bdev_is_partition(bdev))
        blk_partition_remap(bio);

    /* 검사 6: flush 최적화 - write cache가 없으면 PREFLUSH/FUA 플래그 제거 */
    if (op_is_flush(bio->bi_opf) && !bdev_write_cache(bdev)) {
        bio->bi_opf &= ~(REQ_PREFLUSH | REQ_FUA);
        if (!bio_sectors(bio)) {
            /* 데이터 없는 순수 flush → 즉시 완료 */
            status = BLK_STS_OK;
            goto end_io;
        }
    }

    /* 검사 7: 연산 타입별 추가 검증 (atomic write 크기 등) */
    /* ... */

    submit_bio_noacct_nocheck(bio, false);  /* 검증 통과, 다음 단계로 */
}
```

### 3.4 blk-mq 디바이스 라우팅

```c
/* block/blk-core.c:986 */
void submit_bio_noacct_nocheck(struct bio *bio, bool split)
{
    blk_cgroup_bio_start(bio);  /* cgroup v2 I/O 계정 */

    if (current->bio_list) {
        /* 재귀 방지: stacked device(DM/MD)에서 하위 디바이스로 bio를 제출할 때
         * 무한 재귀를 방지하기 위해 리스트에 추가만 하고 나중에 처리 */
        bio_list_add(&current->bio_list[0], bio);
    } else if (!bdev_test_flag(bio->bi_bdev, BD_HAS_SUBMIT_BIO)) {
        /* ★ NVMe 경로: blk-mq 기반 디바이스 (자체 submit_bio 콜백이 없음) */
        __submit_bio_noacct_mq(bio);
    } else {
        /* Device-Mapper, MD RAID 경로: 자체 submit_bio 콜백이 있음 */
        __submit_bio_noacct(bio);
    }
}
```

### 3.5 Plug 초기화와 blk-mq 진입

```c
/* block/blk-core.c:827 */
static void __submit_bio(struct bio *bio)
{
    struct blk_plug plug;

    blk_start_plug(&plug);  /* ★ Plug 시작: 이후 제출되는 요청들을 배치로 모은다 */

    if (!bdev_test_flag(bio->bi_bdev, BD_HAS_SUBMIT_BIO)) {
        /* ★★★ NVMe 메인 경로: bio를 blk-mq로 전달 */
        blk_mq_submit_bio(bio);
    }

    blk_finish_plug(&plug);  /* ★ Plug 종료: 모인 요청들을 한꺼번에 드라이버에 전달 */
}
```

> **핵심 포인트:** `blk_start_plug()`에서 `blk_finish_plug()` 사이에 제출되는 모든 요청은
> plug 버퍼에 모였다가 `blk_finish_plug()` 시점에 한꺼번에 드라이버로 전달된다.
> 이를 통해 NVMe SQ Tail Doorbell 기록 횟수를 줄여 성능을 최적화한다.

---

## 4. blk-mq 진입: blk_mq_submit_bio()

이 함수는 블록 레이어의 **핵심 허브**로, bio를 request로 변환하고 디스패치 경로를 결정한다.

```c
/* block/blk-mq.c:3697 */
void blk_mq_submit_bio(struct bio *bio)
{
    struct request_queue *q = bdev_get_queue(bio->bi_bdev);
    struct blk_plug *plug = current->plug;  /* 현재 스레드의 plug (있으면 배치 모드) */
    struct request *rq;
    unsigned int nr_segs;

    /* ──── 단계 1: 캐시된 request 확인 (plug에 미리 할당된 것) ──── */
    rq = blk_mq_peek_cached_request(plug, q, bio->bi_opf);

    /* ──── 단계 2: 큐 진입 (q_usage_counter 증가) ──── */
    if (!rq) {
        if (unlikely(bio_queue_enter(bio)))
            return;  /* 큐가 프리즈 상태면 대기 또는 실패 */
    }

    /* ──── 단계 3: bio 분할 (디바이스 제한에 맞게) ──── */
    bio = __bio_split_to_limits(bio, &q->limits, &nr_segs);
    /* max_hw_sectors, max_segments 등의 제한을 초과하면 bio를 분할한다.
     * 예: 256KB bio인데 max_hw_sectors가 128KB이면 두 개로 분할 */

    /* ──── 단계 4: Merge 시도 ──── */
    if (blk_mq_attempt_bio_merge(q, bio, nr_segs))
        goto queue_exit;  /* 기존 request에 merge 성공 → 새 request 불필요 */

    /* ──── 단계 5: Request 할당 + Tag 획득 ──── */
    if (rq) {
        blk_mq_use_cached_rq(rq, plug, bio);  /* 캐시된 request 재사용 */
    } else {
        rq = blk_mq_get_new_requests(q, plug, bio);  /* 새 request + tag 할당 */
        if (unlikely(!rq))
            goto queue_exit;
    }

    /* ──── 단계 6: bio 정보를 request에 복사 ──── */
    blk_mq_bio_to_request(rq, bio, nr_segs);

    /* ──── 단계 7: Flush 처리 (REQ_PREFLUSH/FUA) ──── */
    if (op_is_flush(bio->bi_opf) && blk_insert_flush(rq))
        return;

    /* ──── 단계 8: 디스패치 경로 결정 (★ 가장 중요한 분기점) ──── */
    if (plug) {
        /* ★ 경로 A: Plug 배치 (가장 일반적) */
        blk_add_rq_to_plug(plug, rq);
        return;
    }

    hctx = rq->mq_hctx;
    if ((rq->rq_flags & RQF_USE_SCHED) ||
        (hctx->dispatch_busy && (q->nr_hw_queues == 1 || !is_sync))) {
        /* 경로 B: 스케줄러 또는 디스패치 큐 경유 (none 스케줄러에서는 거의 안 탐) */
        blk_mq_insert_request(rq, 0);
        blk_mq_run_hw_queue(hctx, true);
    } else {
        /* ★ 경로 C: Direct Dispatch (최저 지연!) */
        blk_mq_try_issue_directly(hctx, rq);
    }
}
```

### 디스패치 경로 결정 로직 상세

```
blk_mq_submit_bio() 디스패치 경로 결정:

  plug 존재?
    ├── YES → 경로 A: blk_add_rq_to_plug()
    │           └── plug 버퍼에 추가, blk_finish_plug()에서 일괄 제출
    │
    └── NO
         ├── I/O 스케줄러 사용 중? (RQF_USE_SCHED)
         │    ├── YES → 경로 B: 스케줄러 큐에 삽입
         │    └── NO
         │         ├── hctx->dispatch_busy && (단일큐 || 비동기)?
         │         │    ├── YES → 경로 B: dispatch 큐에 삽입
         │         │    └── NO → ★ 경로 C: blk_mq_try_issue_directly()
         │
         └── (none 스케줄러 + plug 없음 + dispatch 한가) → 경로 C

★ No Scheduler + Plug 있는 일반적인 경우:
  대부분 경로 A (plug 배치)를 타고, blk_finish_plug()에서 경로 C로 전환
```

---

## 5. Request 할당과 Tag

### 5.1 Tag의 의미

Tag는 blk-mq에서 각 I/O 요청에 부여하는 고유 번호이다.

```
Tag의 역할:
  1. 동시 진행 중인 I/O를 식별하는 인덱스 (0 ~ queue_depth-1)
  2. NVMe command_id의 하위 12비트로 인코딩 → CQE에서 요청을 찾는 키
  3. tags->rqs[tag] 배열에서 O(1)로 request 포인터를 조회

NVMe Command ID 구성:
  ┌──────────┬───────────────┐
  │ gen (4b) │   tag (12b)   │
  └──────────┴───────────────┘
  비트 15-12   비트 11-0

  gen: generation counter (stale completion 감지용)
  tag: blk-mq 태그 번호 (sbitmap에서 할당)
```

### 5.2 Tag 할당 과정

```c
/* block/blk-mq.c:660 - request 할당의 핵심 */
static struct request *__blk_mq_alloc_requests(struct blk_mq_alloc_data *data)
{
    struct request_queue *q = data->q;
    unsigned int tag;

    /* 단계 1: 현재 CPU의 소프트웨어 큐 컨텍스트를 가져옴 */
    data->ctx = blk_mq_get_ctx(q);

    /* 단계 2: CPU + 연산 타입으로 하드웨어 큐(hctx) 결정 */
    data->hctx = blk_mq_map_queue(data->cmd_flags, data->ctx);

    /* 단계 3: No scheduler이므로 직접 드라이버 태그 할당 */
    /* (스케줄러가 있으면 먼저 스케줄러 태그를 할당하고, 디스패치 시 드라이버 태그를 할당) */
    blk_mq_tag_busy(data->hctx);  /* hctx를 'busy' 상태로 표시 */

    /* 단계 4: sbitmap에서 태그 비트 할당 */
    tag = blk_mq_get_tag(data);
    if (tag == BLK_MQ_NO_TAG) {
        /* 태그 없음 → NOWAIT이면 즉시 실패, 아니면 sleep하며 대기 */
        ...
    }

    /* 단계 5: 태그에 해당하는 request 구조체 초기화 */
    rq = blk_mq_rq_ctx_init(data, tags, tag);
    return rq;
}
```

### 5.3 sbitmap: 태그 비트맵 관리

```c
/* block/blk-mq-tag.c:238 */
unsigned int blk_mq_get_tag(struct blk_mq_alloc_data *data)
{
    struct blk_mq_tags *tags = blk_mq_tags_from_data(data);
    struct sbitmap_queue *bt;
    unsigned int tag_offset;

    /* 예약 태그와 일반 태그를 별도 비트맵으로 관리 */
    if (data->flags & BLK_MQ_REQ_RESERVED) {
        bt = &tags->breserved_tags;    /* 예약 태그 비트맵 */
        tag_offset = 0;
    } else {
        bt = &tags->bitmap_tags;       /* 일반 태그 비트맵 */
        tag_offset = tags->nr_reserved_tags;
    }

    /* sbitmap에서 빈 비트 찾기 (CPU 캐시라인 친화적) */
    tag = __blk_mq_get_tag(data, bt);
    if (tag != BLK_MQ_NO_TAG)
        goto found_tag;

    /* 태그 고갈 시: sleep하며 대기 */
    do {
        blk_mq_run_hw_queue(data->hctx, false);  /* 완료 처리 촉진 */
        tag = __blk_mq_get_tag(data, bt);
        if (tag != BLK_MQ_NO_TAG) break;
        io_schedule();  /* CPU 양보하며 대기 → 완료된 I/O가 태그를 반환하면 깨어남 */
    } while (1);

found_tag:
    return tag + tag_offset;
}
```

```
태그 공간 레이아웃:
┌──────────────────┬────────────────────────────────────┐
│  Reserved Tags   │          Normal Tags                │
│  (0 ~ R-1)       │  (R ~ queue_depth-1)               │
│  breserved_tags  │  bitmap_tags                        │
│                  │                                      │
│  Admin 커맨드용  │  일반 I/O 요청용                    │
│  (Identify 등)   │  (Read, Write 등)                   │
└──────────────────┴────────────────────────────────────┘
                    ←────── queue_depth (예: 1024) ──────→

sbitmap 내부:
  Word 0: [bit0 bit1 ... bit63]   ← 64비트 단위로 관리
  Word 1: [bit0 bit1 ... bit63]
  ...
  각 비트가 하나의 태그에 대응. 1=사용중, 0=사용가능
  CPU별로 다른 word를 우선 검색하여 캐시라인 경합을 줄임
```

---

## 6. Bio → Request 변환

```c
/* block/blk-mq.c:3157 */
static void blk_mq_bio_to_request(struct request *rq, struct bio *bio,
                                   unsigned int nr_segs)
{
    /* bio의 핵심 정보를 request로 복사 */
    rq->bio = rq->biotail = bio;            /* bio 체인의 시작/끝 */
    rq->__sector = bio->bi_iter.bi_sector;  /* 시작 섹터 */
    rq->__data_len = bio->bi_iter.bi_size;  /* 총 데이터 크기 (바이트) */
    rq->nr_phys_segments = nr_segs;         /* 물리 세그먼트 수 (SG 엔트리 수) */

    /* I/O 통계 시작 */
    blk_account_io_start(rq);
}
```

```
변환 후 request 구조:

struct request {
    __sector = 1000           ← bio->bi_iter.bi_sector
    __data_len = 4096         ← bio->bi_iter.bi_size
    bio ──┐                   ← bio 체인 (merge 시 여러 bio)
          ▼
    ┌──────────┐    ┌──────────┐
    │  bio #1  │───▶│  bio #2  │───▶ NULL
    │  4KB     │    │  4KB     │
    └──────────┘    └──────────┘
    biotail ─────────────────────┘

    tag = 42                  ← sbitmap에서 할당
    mq_hctx ──▶ hctx[3]      ← CPU 기반 매핑
    state = MQ_RQ_IDLE        ← 아직 하드웨어에 제출 안 됨

    /* PDU 영역 (request 뒤에 바로 이어짐) */
    ┌─────────────────────────────┐
    │ struct nvme_request         │  ← nvme_req(rq)로 접근
    │   cmd, result, status, ...  │
    ├─────────────────────────────┤
    │ struct nvme_iod             │  ← pci.c 전용
    │   cmd (64B NVMe SQE)        │
    │   sg_table, PRP list 등     │
    └─────────────────────────────┘
}
```

---

## 7. Bio Merge 메커니즘

### 7.1 Merge란?

연속된 섹터를 접근하는 여러 bio를 하나의 request로 합치는 최적화.
하나의 큰 NVMe 커맨드가 여러 개의 작은 커맨드보다 효율적이다.

```
Merge 전:                    Merge 후:
  bio A: sector 100, 4KB      request: sector 100, 12KB
  bio B: sector 108, 4KB        bio A → bio B → bio C (체인)
  bio C: sector 116, 4KB
  → 3개의 NVMe 커맨드         → 1개의 NVMe 커맨드
```

### 7.2 Merge 시도 코드

```c
/* block/blk-mq.c:3570 */
static bool blk_mq_attempt_bio_merge(struct request_queue *q,
                                     struct bio *bio, unsigned int nr_segs)
{
    if (blk_queue_nomerges(q) || !bio_mergeable(bio))
        return false;  /* merge 비활성화 또는 불가능한 bio */

    /* 시도 1: Plug 내의 request와 merge */
    if (blk_attempt_plug_merge(q, bio, nr_segs))
        return true;

    /* 시도 2: 스케줄러 큐의 request와 merge (none 스케줄러에서는 보통 실패) */
    if (blk_mq_sched_bio_merge(q, bio, nr_segs))
        return true;

    return false;
}
```

### 7.3 Plug 내 Merge

```c
/* block/blk-mq.c */
bool blk_attempt_plug_merge(struct request_queue *q, struct bio *bio,
                            unsigned int nr_segs)
{
    struct blk_plug *plug = current->plug;
    struct request *rq;

    if (!plug || rq_list_empty(&plug->mq_list))
        return false;

    /* plug 리스트의 마지막 request부터 역순으로 검색 */
    rq_list_for_each(&plug->mq_list, rq) {
        if (rq->q == q) {
            /* back merge: bio가 request 뒤에 이어지는가? */
            if (blk_rq_pos(rq) + blk_rq_sectors(rq) == bio->bi_iter.bi_sector) {
                /* 크기 제한 확인 (max_hw_sectors, max_segments) */
                if (blk_rq_merge_ok(rq, bio) &&
                    req_attempt_back_merge(rq, bio, nr_segs))
                    return true;
            }
            /* front merge: bio가 request 앞에 오는가? */
            if (bio->bi_iter.bi_sector + bio_sectors(bio) == blk_rq_pos(rq)) {
                if (blk_rq_merge_ok(rq, bio) &&
                    req_attempt_front_merge(rq, bio, nr_segs))
                    return true;
            }
        }
    }
    return false;
}
```

### 7.4 Merge 조건

```
Merge가 가능하려면:
  1. 같은 request_queue (같은 디바이스)
  2. 같은 연산 타입 (둘 다 READ 또는 둘 다 WRITE)
  3. 섹터가 연속 (back merge: rq 끝 == bio 시작, front merge: bio 끝 == rq 시작)
  4. merge 후 크기가 max_hw_sectors를 초과하지 않음
  5. merge 후 세그먼트 수가 max_segments를 초과하지 않음
  6. 암호화 키가 동일 (fscrypt/dm-crypt)
  7. write hint가 동일

NVMe에서의 일반적인 제한:
  max_hw_sectors = MDTS (보통 128KB~1MB)
  max_segments = 하드웨어 의존 (보통 256~512)
```

---

## 8. Plug 메커니즘: 배치 최적화

### 8.1 Plug의 목적

```
Plug 없이 (매번 즉시 제출):
  bio 1 → request → SQ 기록 → Doorbell ┐
  bio 2 → request → SQ 기록 → Doorbell ├── Doorbell 3회 (MMIO 3회)
  bio 3 → request → SQ 기록 → Doorbell ┘

Plug 사용 시 (배치):
  bio 1 → request → plug에 저장 ┐
  bio 2 → request → plug에 저장 ├── blk_finish_plug() 시점에:
  bio 3 → request → plug에 저장 ┘    SQ 기록 3회 + Doorbell 1회

★ MMIO Doorbell 기록은 PCIe uncacheable 접근으로 매우 비싸다 (~수백 ns).
  배치로 줄이면 성능이 크게 향상된다.
```

### 8.2 Plug에 요청 추가

```c
/* block/blk-mq.c:1720 */
static void blk_add_rq_to_plug(struct blk_plug *plug, struct request *rq)
{
    struct request *last = rq_list_peek(&plug->mq_list);

    /* plug가 꽉 찼거나 (BLK_MAX_REQUEST_COUNT, 보통 32)
     * 누적 크기가 BLK_PLUG_FLUSH_SIZE (128KB)를 넘으면 즉시 flush */
    if (plug->rq_count >= blk_plug_max_rq_count(plug) ||
        (!blk_queue_nomerges(rq->q) &&
         blk_rq_bytes(last) >= BLK_PLUG_FLUSH_SIZE)) {
        blk_mq_flush_plug_list(plug, false);
    }

    /* 여러 큐에 걸친 요청이 있는지 추적 */
    if (!plug->multiple_queues && last && last->q != rq->q)
        plug->multiple_queues = true;

    /* plug 리스트 끝에 request 추가 */
    rq_list_add_tail(&plug->mq_list, rq);
    plug->rq_count++;
}
```

### 8.3 Plug Flush (일괄 제출)

```c
/* block/blk-mq.c:3489 */
void blk_mq_flush_plug_list(struct blk_plug *plug, bool from_schedule)
{
    /* plug에 모인 요청들을 한꺼번에 드라이버에 전달 */

    if (!plug->has_elevator && !from_schedule) {
        if (plug->multiple_queues) {
            /* 여러 hctx에 걸친 요청들 → hctx별로 분류해서 각각 dispatch */
            blk_mq_dispatch_multiple_queue_requests(&plug->mq_list);
        } else {
            /* 단일 hctx → 직접 dispatch */
            blk_mq_dispatch_queue_requests(&plug->mq_list, depth);
        }
    }
}

/* 내부적으로 각 request에 대해:
 * blk_mq_dispatch_rq_list() → q->mq_ops->queue_rq(hctx, &bd) 호출
 * bd.last = true인 마지막 요청에서만 NVMe Doorbell을 기록한다 */
```

### 8.4 batch dispatch에서 bd.last의 역할

```c
/* block/blk-mq.c:~2506 - blk_mq_dispatch_rq_list() 내부 */
while (!rq_list_empty(list)) {
    rq = rq_list_pop(list);
    bd.rq = rq;
    bd.last = rq_list_empty(list);  /* ★ 마지막 요청에만 true */

    ret = q->mq_ops->queue_rq(hctx, &bd);  /* = nvme_queue_rq() */
}

/* nvme_queue_rq() 내부: */
nvme_write_sq_db(nvmeq, bd->last);
/* bd->last가 true일 때만 SQ Tail Doorbell을 기록한다.
 * 10개 요청이 배치되면:
 *   SQ 기록 10회 + Doorbell 기록 1회
 *   (Doorbell 없이는 9회의 MMIO 절약!) */
```

---

## 9. Direct Dispatch: 최저 지연 경로

plug가 없고 스케줄러도 없을 때, 가장 빠른 경로이다.

```c
/* block/blk-mq.c:3266 */
static void blk_mq_try_issue_directly(struct blk_mq_hw_ctx *hctx,
                                       struct request *rq)
{
    /* 전제 조건 확인 */
    if (blk_mq_hctx_stopped(hctx) || blk_queue_quiesced(rq->q)) {
        /* 큐가 정지 상태 → 나중에 처리하도록 큐에 삽입 */
        blk_mq_insert_request(rq, 0);
        blk_mq_run_hw_queue(hctx, false);
        return;
    }

    if (!blk_mq_get_budget_and_tag(rq)) {
        /* 디스패치 예산/태그 부족 → 큐에 삽입하고 나중에 재시도 */
        blk_mq_insert_request(rq, 0);
        blk_mq_run_hw_queue(hctx, true);
        return;
    }

    /* ★ 모든 조건 충족: 즉시 드라이버에 전달! */
    ret = __blk_mq_issue_directly(hctx, rq, true);

    switch (ret) {
    case BLK_STS_OK:
        break;  /* 성공 */
    case BLK_STS_RESOURCE:
    case BLK_STS_DEV_RESOURCE:
        /* 드라이버가 리소스 부족으로 거부 → 나중에 재시도 */
        blk_mq_request_bypass_insert(rq, 0);
        blk_mq_run_hw_queue(hctx, false);
        break;
    default:
        /* 치명적 에러 → 즉시 에러 완료 */
        blk_mq_end_request(rq, ret);
        break;
    }
}
```

```c
/* block/blk-mq.c:3194 - 실제 드라이버 콜백 호출 */
static blk_status_t __blk_mq_issue_directly(struct blk_mq_hw_ctx *hctx,
                                             struct request *rq, bool last)
{
    struct blk_mq_queue_data bd = {
        .rq = rq,
        .last = last,  /* direct dispatch에서는 항상 true (배치가 아니므로) */
    };

    /* ★★★ 여기서 NVMe 드라이버의 nvme_queue_rq()가 호출된다! ★★★ */
    ret = q->mq_ops->queue_rq(hctx, &bd);

    /* dispatch_busy EWMA 업데이트: 드라이버가 바쁜지 추적 */
    blk_mq_update_dispatch_busy(hctx, ret != BLK_STS_OK);

    return ret;
}
```

---

## 10. NVMe Driver: nvme_queue_rq()

blk-mq가 호출하는 NVMe 드라이버의 I/O 제출 콜백이다.

```c
/* drivers/nvme/host/pci.c:1675 */
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
                                   const struct blk_mq_queue_data *bd)
{
    struct nvme_queue *nvmeq = hctx->driver_data;  /* hctx → NVMe 하드웨어 큐 */
    struct nvme_dev *dev = nvmeq->dev;
    struct request *req = bd->rq;
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);  /* request의 PDU 영역 */

    /* ──── 검사 1: 큐가 활성화 상태인지 ──── */
    if (unlikely(!test_bit(NVMEQ_ENABLED, &nvmeq->flags)))
        return BLK_STS_IOERR;

    /* ──── 검사 2: 컨트롤러가 I/O 받을 준비가 됐는지 ──── */
    if (unlikely(!nvme_check_ready(&dev->ctrl, req, true)))
        return nvme_fail_nonready_command(&dev->ctrl, req);

    /* ──── 단계 1: NVMe 커맨드 구성 + DMA 매핑 ──── */
    ret = nvme_prep_rq(req);
    /*  내부에서:
     *  1. nvme_setup_cmd()  → bio의 opcode를 NVMe 커맨드로 변환
     *  2. nvme_map_data()   → bio의 페이지들을 PRP/SGL로 매핑
     *  3. nvme_start_request() → blk_mq_start_request() 호출 (IN_FLIGHT로 전이)
     */
    if (unlikely(ret))
        return ret;

    /* ──── 단계 2: SQ에 커맨드 기록 + Doorbell ──── */
    spin_lock(&nvmeq->sq_lock);

    /* 64바이트 SQE를 SQ 링 버퍼의 tail 위치에 복사 */
    nvme_sq_copy_cmd(nvmeq, &iod->cmd);

    /* bd->last가 true일 때만 SQ Tail Doorbell 레지스터에 MMIO 기록 */
    nvme_write_sq_db(nvmeq, bd->last);

    spin_unlock(&nvmeq->sq_lock);

    return BLK_STS_OK;
}
```

### nvme_prep_rq() 상세

```c
/* drivers/nvme/host/pci.c:1598 */
static blk_status_t nvme_prep_rq(struct request *req)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);

    /* IOD 초기화 */
    iod->flags = 0;
    iod->nr_descriptors = 0;

    /* ★ 단계 1: NVMe 커맨드 구성 (core.c) */
    ret = nvme_setup_cmd(req->q->queuedata, req);

    /* ★ 단계 2: 데이터 DMA 매핑 */
    if (blk_rq_nr_phys_segments(req)) {
        ret = nvme_map_data(req);  /* bio 페이지 → PRP 리스트 또는 SGL */
    }

    /* ★ 단계 3: 메타데이터 DMA 매핑 (T10-PI) */
    if (blk_integrity_rq(req)) {
        ret = nvme_map_metadata(req);
    }

    /* ★ 단계 4: request를 IN_FLIGHT 상태로 전이 */
    nvme_start_request(req);
    /*  내부에서 blk_mq_start_request(rq) 호출:
     *  - rq->state = MQ_RQ_IN_FLIGHT
     *  - tags->rqs[rq->tag] = rq  (완료 시 조회용)
     *  - blk_add_timer(rq)  (타임아웃 타이머 시작, 기본 30초)
     */

    return BLK_STS_OK;
}
```

---

## 11. NVMe 커맨드 구성: nvme_setup_cmd()

```c
/* drivers/nvme/host/core.c */
blk_status_t nvme_setup_cmd(struct nvme_ns *ns, struct request *req)
{
    struct nvme_command *cmd = &blk_mq_rq_to_pdu(req)->cmd;

    /* 커맨드 구조체 초기화 (64바이트) */
    memset(cmd, 0, sizeof(*cmd));

    /* command_id 설정: generation counter + blk-mq tag */
    cmd->common.command_id = nvme_cid(req);
    /*  nvme_cid(req) = (genctr << 12) | req->tag
     *  예: genctr=5, tag=42 → CID = 0x502A
     *  CQE에서 이 CID가 돌아오면 tag=42로 request를 찾는다 */

    /* ★ 연산 타입에 따라 NVMe 커맨드 필드 채우기 */
    switch (req_op(req)) {
    case REQ_OP_READ:
    case REQ_OP_WRITE:
        /* NVMe Read/Write 커맨드 구성 */
        cmd->rw.opcode = (req_op(req) == REQ_OP_READ) ?
                          nvme_cmd_read : nvme_cmd_write;
        cmd->rw.nsid = cpu_to_le32(ns->head->ns_id);
        cmd->rw.slba = cpu_to_le64(nvme_sect_to_lba(ns->head,
                                    blk_rq_pos(req)));
        cmd->rw.length = cpu_to_le16((blk_rq_bytes(req) >> ns->head->lba_shift) - 1);
        /* length는 0-based: 1블록이면 0, 8블록이면 7 */

        /* FUA(Force Unit Access) 플래그 설정 */
        if (req->cmd_flags & REQ_FUA)
            cmd->rw.control = cpu_to_le16(NVME_RW_FUA);

        /* DSM 힌트 (sequential, random 등) */
        cmd->rw.dsmgmt = cpu_to_le32(nvme_setup_dsm(req));
        break;

    case REQ_OP_FLUSH:
        /* NVMe Flush 커맨드 */
        cmd->common.opcode = nvme_cmd_flush;
        cmd->common.nsid = cpu_to_le32(ns->head->ns_id);
        break;

    case REQ_OP_DISCARD:
        /* NVMe Dataset Management (TRIM) 커맨드 */
        ret = nvme_setup_discard(ns, req, cmd);
        break;

    case REQ_OP_WRITE_ZEROES:
        /* NVMe Write Zeroes 커맨드 */
        ret = nvme_setup_write_zeroes(ns, req, cmd);
        break;
    }

    /* nvme_request에 커맨드 포인터 저장 */
    nvme_req(req)->cmd = cmd;
    return ret;
}
```

```
NVMe Read/Write SQE (64바이트) 레이아웃:

바이트  필드              예시 값
─────  ──────────────    ──────────────
0      opcode            0x02 (Read) / 0x01 (Write)
1      flags             0x00
2-3    command_id        0x502A (gen=5, tag=42)
4-7    nsid              0x00000001 (NS 1)
8-15   rsvd
16-23  mptr              메타데이터 PRP (PI 사용 시)
24-31  dptr.prp1         첫 번째 데이터 페이지의 물리 주소
32-39  dptr.prp2         두 번째 페이지 또는 PRP 리스트 주소
40-47  slba              시작 LBA (예: 0x00001000)
48-49  length            블록 수 - 1 (예: 7 = 8블록 = 32KB)
50-51  control           FUA 등 제어 플래그
52-55  dsmgmt            DSM 힌트 (sequential/random)
56-59  reftag            PI reference tag
60-61  apptag            PI application tag
62-63  appmask           PI application mask
```

---

## 12. DMA 매핑: PRP/SGL 구성

### 12.1 PRP (Physical Region Page) 방식

NVMe PCIe 드라이버는 기본적으로 PRP 방식을 사용한다.

```c
/* drivers/nvme/host/pci.c - nvme_map_data() → nvme_pci_setup_data_prp() */

/*
 * PRP 구성 규칙:
 *   1. PRP1: 첫 번째 페이지의 물리 주소 (페이지 내 오프셋 포함 가능)
 *   2. PRP2: 두 번째 페이지 주소 또는 PRP 리스트의 주소
 *
 * 케이스별 PRP 구성:
 *
 * Case 1: 1 페이지 이내 (≤4KB)
 *   PRP1 = page0_phys + offset
 *   PRP2 = 0 (사용 안 함)
 *
 * Case 2: 정확히 2 페이지 (≤8KB, 페이지 정렬 시)
 *   PRP1 = page0_phys
 *   PRP2 = page1_phys
 *
 * Case 3: 3 페이지 이상 (>8KB)
 *   PRP1 = page0_phys
 *   PRP2 = PRP_list_phys  (PRP 리스트 페이지의 물리 주소)
 *
 *   PRP 리스트 (DMA 메모리에 할당):
 *   ┌──────────────────┐
 *   │ page1_phys       │
 *   │ page2_phys       │
 *   │ page3_phys       │
 *   │ ...              │
 *   │ pageN_phys       │
 *   └──────────────────┘
 *
 *   PRP 리스트가 한 페이지(4KB)에 안 들어가면 체인:
 *   ┌──────────────────┐     ┌──────────────────┐
 *   │ page1_phys       │     │ page512_phys     │
 *   │ page2_phys       │     │ page513_phys     │
 *   │ ...              │     │ ...              │
 *   │ page510_phys     │     │ pageN_phys       │
 *   │ next_list_phys ──│────▶│                  │
 *   └──────────────────┘     └──────────────────┘
 *   (마지막 엔트리가 다음 PRP 리스트를 가리킴)
 */
```

### 12.2 SGL vs PRP 선택

```c
/* drivers/nvme/host/pci.c */
/* sgl_threshold 모듈 파라미터 (기본값: SGL_THRESHOLD = 32KB)
 * 데이터 크기가 sgl_threshold 이상이고 컨트롤러가 SGL을 지원하면 SGL 사용 */

static blk_status_t nvme_map_data(struct request *req)
{
    /* bio의 페이지들을 DMA 매핑 (scatter-gather list 생성) */
    blk_rq_map_sg(req->q, req, iod->sgt.sgl);
    /* 이 함수는 blk-mq가 제공하며, bio_vec 배열을 scatterlist로 변환한다 */

    if (데이터크기 >= sgl_threshold && nvme_ctrl_sgl_supported(ctrl))
        nvme_pci_setup_data_sgl(req);  /* SGL 방식 */
    else
        nvme_pci_setup_data_prp(req);  /* PRP 방식 (기본) */
}
```

---

## 13. SQ Doorbell과 배치 최적화

### 13.1 SQ에 커맨드 복사

```c
/* drivers/nvme/host/pci.c */
static inline void nvme_sq_copy_cmd(struct nvme_queue *nvmeq,
                                     struct nvme_command *cmd)
{
    /* SQ 링 버퍼의 현재 tail 위치에 64바이트 커맨드 복사 */
    memcpy(nvmeq->sq_cmds + (nvmeq->sq_tail << nvmeq->sqe_shift),
           cmd, sizeof(*cmd));

    /* tail 포인터 전진 (circular) */
    nvmeq->sq_tail = (nvmeq->sq_tail + 1) & (nvmeq->q_depth - 1);
}
```

### 13.2 Doorbell 기록 최적화

```c
/* drivers/nvme/host/pci.c */
static inline void nvme_write_sq_db(struct nvme_queue *nvmeq, bool write_sq)
{
    if (!write_sq) {
        /* bd.last = false: 배치의 중간 요청 → Doorbell 기록 생략! */
        /* SQ에 커맨드만 복사하고 컨트롤러에는 아직 알리지 않음 */
        return;
    }

    /* Shadow Doorbell Buffer 최적화 (NVMe 1.3+) */
    if (nvme_dbbuf_need_event(nvmeq->sq_tail,
                              nvmeq->last_sq_tail,
                              nvmeq->dbbuf_sq_ei)) {
        /* Event Index 알고리즘: 컨트롤러가 "이 값 이후에 알려줘"라고 한 경우에만 기록 */
        nvmeq->last_sq_tail = nvmeq->sq_tail;
        /* ★ 실제 MMIO Doorbell 기록 */
        writel(nvmeq->sq_tail, nvmeq->q_db);
    }
}
```

```
배치 제출 흐름 (3개 요청):

시간 →
  req1: nvme_sq_copy_cmd() → nvme_write_sq_db(last=false) → DB 생략
  req2: nvme_sq_copy_cmd() → nvme_write_sq_db(last=false) → DB 생략
  req3: nvme_sq_copy_cmd() → nvme_write_sq_db(last=true)  → ★ DB 기록!

SQ 링 버퍼:
  [cmd1][cmd2][cmd3][...empty...]
                     ↑
              SQ Tail Doorbell = 3 (MMIO writel)

컨트롤러는 이 Doorbell을 보고 3개 커맨드를 한꺼번에 fetch한다.
```

---

## 14. 인터럽트와 완료 경로

### 14.1 MSI-X 인터럽트

```
NVMe는 각 CQ에 대해 별도의 MSI-X 인터럽트 벡터를 할당한다.
따라서 I/O 큐 N의 완료는 해당 큐 전용 인터럽트로 통지된다.

  NVMe CQ 0 (Admin)  → MSI-X 벡터 0 → CPU X → nvme_irq()
  NVMe CQ 1 (I/O #1) → MSI-X 벡터 1 → CPU 0 → nvme_irq()
  NVMe CQ 2 (I/O #2) → MSI-X 벡터 2 → CPU 1 → nvme_irq()
  ...
```

### 14.2 인터럽트 처리 체인

```c
/* drivers/nvme/host/pci.c:1995 */
static irqreturn_t nvme_irq(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;
    DEFINE_IO_COMP_BATCH(iob);  /* 배치 완료 컨테이너 */

    if (nvme_poll_cq(nvmeq, &iob)) {
        /* CQE가 있었다면 배치 완료 처리 */
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}
```

### 14.3 CQ 폴링

```c
/* drivers/nvme/host/pci.c:1955 */
static inline bool nvme_poll_cq(struct nvme_queue *nvmeq,
                                 struct io_comp_batch *iob)
{
    bool found = false;

    while (nvme_cqe_pending(nvmeq)) {  /* Phase Tag로 새 CQE 확인 */
        found = true;
        dma_rmb();  /* ★ 메모리 배리어: Phase Tag 읽기와 CQE 데이터 읽기 순서 보장 */
        nvme_handle_cqe(nvmeq, iob, nvmeq->cq_head);
        nvme_update_cq_head(nvmeq);  /* CQ head 전진 */
    }

    if (found)
        nvme_ring_cq_doorbell(nvmeq);  /* CQ Head Doorbell 기록 → 엔트리 반환 */
    return found;
}
```

### 14.4 Phase Tag 메커니즘

```
CQ 링 버퍼에서 새로운 CQE를 구분하는 방법:

  호스트의 expected_phase = 1 (초기값)
  컨트롤러가 CQE를 쓸 때 status 필드의 Phase bit = 1
  호스트가 CQE를 읽고 Phase bit == expected_phase이면 새 CQE

  CQ가 한 바퀴 돌면(wrap-around) expected_phase가 반전:
  1 → 0 → 1 → 0 ...

  이를 통해 인터럽트 없이도 새 CQE를 감지할 수 있다 (폴링 모드).

  ┌──────┬──────┬──────┬──────┬──────┐
  │CQE 0 │CQE 1 │CQE 2 │CQE 3 │CQE 4 │
  │P=0   │P=0   │P=1   │P=1   │P=1   │
  └──────┴──────┴──────┴──────┴──────┘
                  ↑
              cq_head (expected_phase=1)
              CQE 2의 P=1 == expected → 새 CQE!
              CQE 0의 P=0 ≠ expected → 이전 라운드의 CQE (무시)
```

### 14.5 CQE 처리

```c
/* drivers/nvme/host/pci.c:1883 */
static inline void nvme_handle_cqe(struct nvme_queue *nvmeq,
                                    struct io_comp_batch *iob, u16 idx)
{
    struct nvme_completion *cqe = &nvmeq->cqes[idx];
    __u16 command_id = READ_ONCE(cqe->command_id);

    /* ★ command_id에서 tag를 추출하여 request를 찾는다 */
    struct request *req = nvme_find_rq(nvme_queue_tagset(nvmeq), command_id);
    /*  내부: tag = command_id & 0xFFF
     *        rq = tags->rqs[tag]  → O(1) 조회! */

    /* CQE의 status와 result를 nvme_request에 저장 */
    if (!nvme_try_complete_req(req, cqe->status, cqe->result)) {
        /* 같은 CPU에서 완료 가능 → 배치에 추가하거나 직접 완료 */
        if (!blk_mq_add_to_batch(req, iob, ...))
            nvme_pci_complete_rq(req);  /* 즉시 완료 */
    }
}
```

### 14.6 완료 처리 체인

```c
/* drivers/nvme/host/pci.c */
static void nvme_pci_complete_rq(struct request *req)
{
    nvme_pci_unmap_rq(req);   /* DMA 매핑 해제 (PRP/SGL 메모리 반환) */
    nvme_complete_rq(req);    /* NVMe 코어 완료 처리 */
}

/* drivers/nvme/host/core.c */
void nvme_complete_rq(struct request *req)
{
    nvme_cleanup_cmd(req);    /* 커맨드 관련 리소스 정리 */

    /* 상태에 따른 처리 결정 */
    switch (nvme_decide_disposition(req)) {
    case COMPLETE:
        nvme_end_req(req);     /* ★ 정상 완료 */
        return;
    case RETRY:
        nvme_retry_req(req);   /* 일시적 에러 → 재시도 */
        return;
    case FAILOVER:
        nvme_failover_req(req); /* 멀티패스 → 다른 경로로 재시도 */
        return;
    }
}

/* nvme_end_req() → blk_mq_end_request() → blk_mq_free_request() */
```

---

## 15. Request 상태 머신

```
Request의 3가지 상태:

  MQ_RQ_IDLE ──────────▶ MQ_RQ_IN_FLIGHT ──────────▶ MQ_RQ_COMPLETE
   (할당됨)    start_rq()    (하드웨어 처리중)  complete()    (완료)
                                    │
                                    │ timeout (30s)
                                    ▼
                              nvme_timeout()
                              ├── BLK_EH_DONE: 에러 완료
                              ├── BLK_EH_RESET_TIMER: 타이머 재시작
                              └── 컨트롤러 리셋

상태 전이 코드:

1. IDLE → IN_FLIGHT:
   blk_mq_start_request(rq)  [nvme_start_request()에서 호출]
     WRITE_ONCE(rq->state, MQ_RQ_IN_FLIGHT);
     blk_add_timer(rq);  /* 타임아웃 타이머 시작 */
     tags->rqs[rq->tag] = rq;  /* 완료 시 조회용 등록 */

2. IN_FLIGHT → COMPLETE:
   blk_mq_complete_request(rq)  [nvme_try_complete_req()에서 호출]
     if (cmpxchg(&rq->state, MQ_RQ_IN_FLIGHT, MQ_RQ_COMPLETE))
       /* 상태 전이 성공 → 완료 처리 진행 */
       /* cmpxchg를 사용하는 이유: timeout 핸들러와의 경합 방지
        * 인터럽트와 타이머가 동시에 같은 request를 완료하려 할 수 있다 */

3. COMPLETE → (해제):
   __blk_mq_end_request(rq)
     blk_mq_put_tag()  /* 태그 반환 */
     blk_mq_free_request()  /* request 구조체 해제 */
```

---

## 16. 에러 처리와 재시도

### 16.1 nvme_decide_disposition()

```c
/* drivers/nvme/host/core.c */
static inline enum nvme_disposition nvme_decide_disposition(struct request *req)
{
    /* 취소된 요청 → 즉시 완료 */
    if (unlikely(nvme_req(req)->flags & NVME_REQ_CANCELLED))
        return COMPLETE;

    /* 성공 → 완료 */
    if (nvme_req(req)->status == 0)
        return COMPLETE;

    /* 경로 에러 (멀티패스) → 페일오버 */
    if (blk_noretry_request(req) || ...) {
        if (nvme_is_path_error(nvme_req(req)->status) || ...)
            return FAILOVER;
    }

    /* 재시도 가능? */
    if (nvme_req(req)->retries > 0 && !(status & NVME_STATUS_DNR)) {
        /* DNR(Do Not Retry) 비트가 없고 재시도 횟수가 남았으면 재시도 */
        return RETRY;
    }

    return COMPLETE;  /* 재시도 불가 → 에러 상태로 완료 */
}
```

### 16.2 타임아웃 처리

```c
/* drivers/nvme/host/pci.c */
static enum blk_eh_timer_return nvme_timeout(struct request *req)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
    struct nvme_queue *nvmeq = req->mq_hctx->driver_data;

    /* 단계 1: 완료가 진행 중인지 확인 (CQ 폴링) */
    if (nvme_poll_cq(nvmeq, NULL)) {
        /* CQE가 있었다 → 완료가 곧 처리될 것 → 타이머 재시작 */
        return BLK_EH_RESET_TIMER;
    }

    /* 단계 2: Abort 커맨드 시도 */
    if (atomic_dec_if_positive(&dev->ctrl.abort_limit) >= 0) {
        /* NVMe Abort 커맨드를 Admin Queue로 보내서 해당 커맨드 취소 시도 */
        nvme_submit_abort_cmd(req);
        return BLK_EH_RESET_TIMER;
    }

    /* 단계 3: Abort도 실패 → 컨트롤러 리셋 */
    nvme_dev_disable(dev, false);
    nvme_reset_ctrl(&dev->ctrl);
    return BLK_EH_DONE;

    /* 에스컬레이션 전략:
     * 1차: CQ 폴링 (비용 최소)
     * 2차: Abort 커맨드 (개별 요청 취소)
     * 3차: 컨트롤러 리셋 (전체 I/O 취소, 최후의 수단)
     */
}
```

---

## 17. Tag 반환과 Request 해제

### 17.1 완료에서 해제까지

```c
/* block/blk-mq.c */
void __blk_mq_end_request(struct request *rq, blk_status_t error)
{
    /* QoS 통계 업데이트 */
    if (rq->rq_flags & RQF_STATS)
        blk_stat_add(rq, blk_time_get_ns());

    /* bio 완료 콜백 호출 */
    blk_update_request(rq, error, blk_rq_bytes(rq));
    /*  내부에서:
     *  bio->bi_status = error;
     *  bio_endio(bio);  → 파일시스템의 완료 콜백 호출
     *  (bio 체인이 있으면 모든 bio에 대해 호출) */

    /* q_usage_counter 감소 (큐 프리즈 감지용) */
    if (req_ref_put_and_test(rq))
        __blk_mq_free_request(rq);
}

void __blk_mq_free_request(struct request *rq)
{
    struct blk_mq_hw_ctx *hctx = rq->mq_hctx;
    struct blk_mq_ctx *ctx = rq->mq_ctx;

    /* ★ 태그 반환: sbitmap에서 해당 비트를 해제 */
    blk_mq_put_tag(hctx->tags, ctx, rq->tag);
    /* sbitmap_queue_clear()가 호출되어:
     * 1. 비트맵에서 태그 비트를 0으로 설정
     * 2. 대기 중인 io_schedule() 스레드를 깨움
     * ctx->cpu를 전달하여 같은 CPU의 비트를 우선 재사용 (캐시 친화적) */

    /* q_usage_counter 감소 */
    blk_queue_exit(rq->q);
}
```

### 17.2 배치 태그 반환

```c
/* block/blk-mq.c - 인터럽트에서 여러 CQE를 처리한 경우 */
void blk_mq_end_request_batch(struct io_comp_batch *iob)
{
    int tags[TAG_COMP_BATCH], nr_tags = 0;
    struct blk_mq_hw_ctx *cur_hctx = NULL;

    while ((rq = rq_list_pop(&iob->req_list)) != NULL) {
        /* 같은 hctx의 태그들을 모아서 배치 반환 */
        if (rq->mq_hctx != cur_hctx) {
            if (nr_tags)
                blk_mq_flush_tag_batch(cur_hctx, tags, nr_tags);
            cur_hctx = rq->mq_hctx;
            nr_tags = 0;
        }
        tags[nr_tags++] = rq->tag;
    }

    /* 마지막 배치 반환 */
    if (nr_tags)
        blk_mq_flush_tag_batch(cur_hctx, tags, nr_tags);
}

/* blk_mq_flush_tag_batch()는 여러 태그를 한 번에 sbitmap에 반환하여
 * 개별 반환 대비 lock 경합과 wake-up 횟수를 줄인다 */
```

---

## 18. 전체 Call Chain 요약

```
═══════════════════════════════════════════════════════════════════
                    I/O 제출 경로 (Submit Path)
═══════════════════════════════════════════════════════════════════

submit_bio(bio)                                    [blk-core.c]
  │
  ├─ I/O 통계, ioprio 설정
  │
  └─▶ submit_bio_noacct(bio)                       [blk-core.c]
       │
       ├─ 유효성 검사 (RO, EOD, 파티션 리맵, flush 최적화)
       │
       └─▶ submit_bio_noacct_nocheck(bio)          [blk-core.c]
            │
            ├─ cgroup 계정
            │
            └─▶ __submit_bio(bio)                  [blk-core.c]
                 │
                 ├─ blk_start_plug(&plug)          ← Plug 시작
                 │
                 └─▶ blk_mq_submit_bio(bio)        [blk-mq.c]  ★ 핵심
                      │
                      ├─ bio_queue_enter()          ← q_usage_counter++
                      ├─ __bio_split_to_limits()    ← 디바이스 제한에 맞게 분할
                      ├─ blk_mq_attempt_bio_merge() ← Merge 시도
                      │   ├─ blk_attempt_plug_merge()  (plug 내 merge)
                      │   └─ 성공 시 return (새 request 불필요)
                      │
                      ├─ blk_mq_get_new_requests()  ← Request + Tag 할당
                      │   ├─ blk_mq_get_ctx()       (CPU → sw queue)
                      │   ├─ blk_mq_map_queue()     (sw queue → hw queue)
                      │   └─ blk_mq_get_tag()       (sbitmap에서 태그 비트 할당)
                      │
                      ├─ blk_mq_bio_to_request()    ← bio 정보 → request 복사
                      │
                      └─ 디스패치 경로 결정:
                          │
                          ├─[Plug 있음]─▶ blk_add_rq_to_plug()
                          │                  │
                          │    blk_finish_plug() 시점에:
                          │                  │
                          │    blk_mq_flush_plug_list()
                          │      └─▶ blk_mq_dispatch_rq_list()  [blk-mq.c]
                          │            │
                          │            └─ for each request:
                          │                q->mq_ops->queue_rq(hctx, &bd)
                          │                        ║
                          │                        ▼
                          │               nvme_queue_rq()  ★
                          │
                          └─[Plug 없음]─▶ blk_mq_try_issue_directly()
                                           │
                                           └─▶ __blk_mq_issue_directly()
                                                 │
                                                 └─ q->mq_ops->queue_rq(hctx, &bd)
                                                            ║
                                                            ▼
                                                   nvme_queue_rq()  ★

═══════════════════════════════════════════════════════════════════
                    NVMe 드라이버 제출 (pci.c)
═══════════════════════════════════════════════════════════════════

nvme_queue_rq(hctx, bd)                            [pci.c]
  │
  ├─ nvme_check_ready()                ← 컨트롤러 상태 확인
  │
  └─▶ nvme_prep_rq(req)
       │
       ├─ nvme_setup_cmd(ns, req)      [core.c]
       │   ├─ cmd->command_id = nvme_cid(req)  ← (gen << 12) | tag
       │   └─ READ/WRITE/FLUSH/DISCARD → NVMe opcode + 파라미터
       │
       ├─ nvme_map_data(req)           ← bio pages → PRP/SGL (DMA 매핑)
       │
       └─ nvme_start_request(req)
            └─ blk_mq_start_request()  [blk-mq.c]
                ├─ rq->state = MQ_RQ_IN_FLIGHT
                ├─ tags->rqs[tag] = rq  ← 완료 시 조회용
                └─ blk_add_timer(rq)    ← 30초 타임아웃

  spin_lock(&nvmeq->sq_lock)
  ├─ nvme_sq_copy_cmd()    ← 64B SQE를 SQ[tail]에 memcpy
  ├─ nvme_write_sq_db()    ← bd->last==true일 때만 MMIO Doorbell 기록
  spin_unlock(&nvmeq->sq_lock)

═══════════════════════════════════════════════════════════════════
          하드웨어 처리 (NVMe Controller)
═══════════════════════════════════════════════════════════════════

  SQ Tail Doorbell 감지 → SQ에서 SQE DMA fetch
  → 내부 명령 처리 (Flash read/write)
  → CQ에 16B CQE DMA write (status + command_id + result)
  → MSI-X 인터럽트 발생

═══════════════════════════════════════════════════════════════════
                    완료 경로 (Completion Path)
═══════════════════════════════════════════════════════════════════

[MSI-X 인터럽트]
  │
  └─▶ nvme_irq(irq, nvmeq)                        [pci.c]
       │
       └─▶ nvme_poll_cq(nvmeq, &iob)
            │
            while (nvme_cqe_pending(nvmeq)):       ← Phase Tag 확인
              │
              ├─ dma_rmb()                         ← 메모리 배리어
              │
              └─▶ nvme_handle_cqe(nvmeq, iob, head)
                   │
                   ├─ cqe->command_id              ← CQE에서 CID 읽기
                   ├─ tag = command_id & 0xFFF      ← 하위 12비트 = tag
                   ├─ rq = tags->rqs[tag]           ← O(1) request 조회
                   │
                   └─▶ nvme_try_complete_req(req, status, result)
                        │
                        ├─ nvme_req(rq)->genctr++   ← gen counter 증가
                        ├─ nvme_req(rq)->status = status >> 1
                        ├─ nvme_req(rq)->result = result
                        │
                        └─ blk_mq_complete_request_remote(req)
                             │
                             └─ (같은 CPU이면 직접, 다른 CPU이면 IPI)

            nvme_ring_cq_doorbell(nvmeq)            ← CQ Head Doorbell 기록

       └─▶ nvme_pci_complete_batch(&iob)           ← 배치 완료 처리
            │
            for each request in batch:
              │
              ├─ nvme_pci_unmap_rq(req)             ← DMA 매핑 해제
              │
              └─▶ nvme_complete_rq(req)            [core.c]
                   │
                   ├─ nvme_cleanup_cmd(req)
                   │
                   └─ switch(nvme_decide_disposition):
                       │
                       ├─ COMPLETE ──▶ nvme_end_req(req)
                       │                └─ blk_mq_end_request(req, status)
                       │                    └─ blk_update_request()
                       │                        └─ bio_endio(bio)  ★ 파일시스템 복귀
                       │                    └─ __blk_mq_free_request()
                       │                        ├─ blk_mq_put_tag()  ← sbitmap 비트 해제
                       │                        └─ blk_queue_exit()  ← q_usage_counter--
                       │
                       ├─ RETRY ────▶ nvme_retry_req(req)
                       │               └─ blk_mq_requeue_request()
                       │                   └─ (request를 큐에 다시 삽입)
                       │
                       └─ FAILOVER ─▶ nvme_failover_req(req)
                                       └─ (다른 경로의 큐에 재삽입)
```

---

## 부록: SPDK와의 비교

```
┌────────────────────┬──────────────────────┬───────────────────────┐
│ 항목               │ 커널 NVMe 드라이버   │ SPDK                  │
├────────────────────┼──────────────────────┼───────────────────────┤
│ I/O 제출           │ syscall → VFS →      │ 유저스페이스에서 직접  │
│                    │ blk-mq → nvme_queue_rq│ SQ에 기록             │
├────────────────────┼──────────────────────┼───────────────────────┤
│ 태그 관리          │ blk-mq sbitmap       │ 직접 관리 (없거나     │
│                    │ (커널 동기화 필요)    │ 단순 카운터)          │
├────────────────────┼──────────────────────┼───────────────────────┤
│ 인터럽트           │ MSI-X 인터럽트       │ 폴링 (busy-wait)      │
│                    │ → softirq/IPI        │ → 지연 최소화         │
├────────────────────┼──────────────────────┼───────────────────────┤
│ 완료 조회          │ tags->rqs[tag]       │ 유저 배열 직접 접근   │
│                    │ (O(1), 커널 내부)     │ (O(1), 유저스페이스)  │
├────────────────────┼──────────────────────┼───────────────────────┤
│ 컨텍스트 스위치    │ 있음 (syscall,       │ 없음 (전용 코어)      │
│                    │  인터럽트)            │                       │
├────────────────────┼──────────────────────┼───────────────────────┤
│ Doorbell 최적화    │ bd.last + Shadow DB  │ 배치 + Shadow DB      │
├────────────────────┼──────────────────────┼───────────────────────┤
│ DMA 주소 지정      │ PRP (기본), SGL      │ SGL (기본), PRP       │
├────────────────────┼──────────────────────┼───────────────────────┤
│ 큐 공유            │ NS간 tagset 공유     │ 큐 독점 (경합 없음)   │
├────────────────────┼──────────────────────┼───────────────────────┤
│ 지연 시간          │ ~10-20µs (4KB read)  │ ~2-5µs (4KB read)     │
├────────────────────┼──────────────────────┼───────────────────────┤
│ CPU 효율           │ 높음 (idle 시 CPU 0%)│ 낮음 (폴링으로 100%)  │
└────────────────────┴──────────────────────┴───────────────────────┘
```

---

## 소스 파일 위치

| 파일 | 역할 |
|------|------|
| `block/blk-core.c` | bio 제출 진입점 (submit_bio) |
| `block/blk-mq.c` | blk-mq 코어 (request 할당, 디스패치, 완료) |
| `block/blk-mq-tag.c` | 태그 비트맵 관리 (sbitmap) |
| `include/linux/blk-mq.h` | blk-mq 자료구조 (request, tag_set, blk_mq_ops) |
| `include/linux/blkdev.h` | 블록 디바이스 자료구조 (request_queue, gendisk) |
| `drivers/nvme/host/pci.c` | NVMe PCIe 드라이버 (probe, queue_rq, irq) |
| `drivers/nvme/host/core.c` | NVMe 코어 (setup_cmd, complete_rq) |
| `drivers/nvme/host/nvme.h` | NVMe 내부 자료구조 (nvme_ctrl, nvme_ns) |
| `include/linux/nvme.h` | NVMe 스펙 자료구조 (커맨드, CQE, Identify) |
