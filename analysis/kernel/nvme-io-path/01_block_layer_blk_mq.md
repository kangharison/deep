# Block Layer (blk-mq) 심층 분석

> 소스 기준: Linux 커널 6.x (~/sources/linux)
> 핵심 파일: `block/blk-mq.c`, `block/blk-core.c`, `block/bio.c`, `block/blk-merge.c`, `block/blk-mq-tag.c`, `block/blk-mq-sched.c`

---

## 1. Block Layer 전체 아키텍처

### 1.1 전통적 Block Layer vs blk-mq (Multi-Queue)

전통적인 블록 레이어(single-queue)는 디바이스당 하나의 request queue를 가지며, 모든 CPU가 이 하나의 큐를 공유하면서 spinlock 경합이 발생했다. HDD 시대에는 디바이스 자체가 순차 접근에 최적화되어 있어서 단일 큐로 충분했지만, NVMe SSD가 등장하면서 상황이 완전히 바뀌었다.

NVMe SSD는 하드웨어 레벨에서 최대 65535개의 I/O 큐를 지원하고, 각 큐에서 독립적으로 수십만~수백만 IOPS를 처리할 수 있다. 그런데 커널의 단일 큐 블록 레이어가 이 성능을 전부 활용하지 못하는 병목이 되었다. 모든 CPU가 하나의 큐 lock을 놓고 경합하기 때문이다.

이 문제를 해결하기 위해 Jens Axboe가 2013년에 blk-mq(Block Multi-Queue)를 개발했다. blk-mq의 핵심 아이디어는 다음과 같다.

| 구분 | 전통적 Block Layer | blk-mq (Multi-Queue) |
|------|-------------------|---------------------|
| 큐 구조 | 디바이스당 1개 request queue | per-CPU Software Queue + per-device Hardware Queue |
| Lock 경합 | 모든 CPU가 하나의 lock 경합 | CPU별 lock으로 경합 최소화 |
| 태그 관리 | 없음 (순차 번호) | sbitmap 기반 확장 가능한 태그 할당 |
| NVMe 활용 | 하드웨어 큐 1개만 활용 | CPU 수만큼 NVMe I/O 큐 활용 |
| 최대 IOPS | ~100K (lock 병목) | 수백만 IOPS |
| 도입 시기 | Linux 2.x~ | Linux 3.13 (2014년) |

### 1.2 blk-mq 등장 배경

NVMe 디바이스는 내부적으로 최대 65535개의 Submission Queue(SQ)와 Completion Queue(CQ) 쌍을 가질 수 있다. 일반적으로 온라인 CPU 코어 수만큼 I/O 큐 쌍을 생성하여 각 CPU가 전용 큐를 사용하게 한다. 이렇게 하면 CPU 간 lock 경합이 전혀 없다.

blk-mq는 이러한 하드웨어 특성을 소프트웨어에서 최대한 활용하기 위해 설계되었다.

### 1.3 Software Queue → Hardware Queue 매핑

blk-mq의 큐 계층 구조는 다음과 같다.

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Application Layer                            │
│              read() / write() / io_uring_enter()                    │
└──────────────────────────────┬──────────────────────────────────────┘
                               │
                               v
┌─────────────────────────────────────────────────────────────────────┐
│                    VFS / File System Layer                           │
│                   ext4, xfs, btrfs, ...                              │
│                   bio 생성 → submit_bio()                            │
└──────────────────────────────┬──────────────────────────────────────┘
                               │
                               v
┌─────────────────────────────────────────────────────────────────────┐
│                    Block Layer (blk-mq)                              │
│                                                                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐               │
│  │ SW Queue │ │ SW Queue │ │ SW Queue │ │ SW Queue │  (per-CPU)    │
│  │  (ctx)   │ │  (ctx)   │ │  (ctx)   │ │  (ctx)   │               │
│  │  CPU 0   │ │  CPU 1   │ │  CPU 2   │ │  CPU 3   │               │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘               │
│       │             │             │             │                     │
│       │    ┌────────┘             │    ┌────────┘                    │
│       │    │                      │    │                             │
│       v    v                      v    v                             │
│  ┌──────────────┐           ┌──────────────┐                        │
│  │  HW Queue 0  │           │  HW Queue 1  │     (per-NVMe-SQ)     │
│  │   (hctx)     │           │   (hctx)     │                        │
│  │  tags/sbitmap │           │  tags/sbitmap │                        │
│  └──────┬───────┘           └──────┬───────┘                        │
│         │                          │                                 │
└─────────┼──────────────────────────┼────────────────────────────────┘
          │                          │
          v                          v
┌──────────────────┐      ┌──────────────────┐
│  NVMe I/O Queue  │      │  NVMe I/O Queue  │
│   SQ/CQ Pair 1   │      │   SQ/CQ Pair 2   │
│  (nvme_queue[1]) │      │  (nvme_queue[2]) │
│                  │      │                  │
│  ┌────────────┐  │      │  ┌────────────┐  │
│  │ Submission │  │      │  │ Submission │  │
│  │   Queue    │  │      │  │   Queue    │  │
│  │ (64B SQE)  │  │      │  │ (64B SQE)  │  │
│  └────────────┘  │      │  └────────────┘  │
│  ┌────────────┐  │      │  ┌────────────┐  │
│  │ Completion │  │      │  │ Completion │  │
│  │   Queue    │  │      │  │   Queue    │  │
│  │ (16B CQE)  │  │      │  │ (16B CQE)  │  │
│  └────────────┘  │      │  └────────────┘  │
└──────────────────┘      └──────────────────┘
```

**매핑 관계 정리:**
- **Software Queue (blk_mq_ctx)**: CPU당 1개. I/O를 제출한 CPU를 추적하고, request의 임시 대기 장소 역할을 한다.
- **Hardware Queue (blk_mq_hw_ctx)**: NVMe I/O 큐당 1개. 보통 CPU 코어 수만큼 생성한다.
- **CPU→HW Queue 매핑**: `blk_mq_queue_map`의 `mq_map[cpu]`로 결정된다. 보통 1:1이지만, NVMe 큐 수가 CPU 수보다 적으면 N:1 매핑이 된다.

---

## 2. 핵심 데이터 구조 상세

### 2.1 struct bio

`struct bio`는 블록 I/O의 기본 단위이다. 파일시스템이 디스크에서 데이터를 읽거나 쓸 때, 각 I/O 요청을 bio로 표현한다.

**정의 위치:** `include/linux/blk_types.h` (line 210)

```c
struct bio {
    struct bio          *bi_next;       /* 다음 bio (request queue에서 체이닝) */
    struct block_device *bi_bdev;       /* 대상 블록 디바이스 (예: /dev/nvme0n1) */
    blk_opf_t           bi_opf;         /* 하위 8비트: REQ_OP (READ/WRITE 등)
                                           상위 24비트: 플래그 (REQ_SYNC 등) */
    unsigned short      bi_flags;       /* BIO_CLONED, BIO_CHAIN 등 내부 플래그 */
    unsigned short      bi_ioprio;      /* I/O 우선순위 */
    enum rw_hint        bi_write_hint;  /* 데이터 수명 힌트 (NVMe Directive에 매핑) */
    blk_status_t        bi_status;      /* 완료 상태: BLK_STS_OK, BLK_STS_IOERR 등 */

    atomic_t        __bi_remaining;     /* bio_chain에서 남은 하위 bio 수 */
    struct bvec_iter    bi_iter;        /* ★ 핵심: 현재 I/O 위치와 크기 */

    bio_end_io_t    *bi_end_io;         /* ★ 완료 콜백 함수 포인터 */
    void            *bi_private;        /* 콜백에서 사용할 private 데이터 */

    unsigned short  bi_vcnt;            /* bi_io_vec 배열의 유효 항목 수 */
    unsigned short  bi_max_vecs;        /* bi_io_vec 배열의 최대 크기 */
    atomic_t        __bi_cnt;           /* 참조 카운트 */
    struct bio_vec  *bi_io_vec;         /* ★ 물리 페이지 목록 (scatter-gather) */
    struct bio_set  *bi_pool;           /* 이 bio를 할당한 bio_set */
};
```

**bi_iter (struct bvec_iter)의 핵심 필드:**

| 필드 | 타입 | 설명 |
|------|------|------|
| `bi_sector` | `sector_t` | I/O 시작 섹터 번호 (512바이트 단위). 예: bi_sector=8이면 디스크 4096바이트 오프셋 |
| `bi_size` | `unsigned int` | 남은 I/O 크기 (바이트). 완료될 때마다 감소한다 |
| `bi_idx` | `unsigned int` | 현재 처리 중인 bio_vec 인덱스 |
| `bi_bvec_done` | `unsigned int` | 현재 bio_vec에서 이미 완료된 바이트 수 |

**bio_vec: 물리 메모리 세그먼트**

```c
struct bio_vec {
    struct page *bv_page;       /* 물리 페이지 포인터 */
    unsigned int bv_len;        /* 이 세그먼트의 길이 (바이트) */
    unsigned int bv_offset;     /* 페이지 내 시작 오프셋 */
};
```

하나의 bio는 여러 bio_vec를 가질 수 있다. 이는 scatter-gather DMA에 직접 매핑된다. NVMe 드라이버는 bio_vec 배열을 순회하여 PRP(Physical Region Page) 리스트나 SGL(Scatter-Gather List)을 구성한다.

**bio 생명주기:**

```
  bio_alloc()          bio_add_page()         submit_bio()
  ───────────> [ALLOC] ──────────────> [FILL] ──────────────> [SUBMIT]
                                                                  │
                                                                  v
                                                           blk_mq_submit_bio()
                                                                  │
                                                                  v
                                                           nvme_queue_rq()
                                                                  │
    ┌─────────────────────────────────────────────────────────────┘
    │                     (하드웨어 처리)
    v
  nvme_irq() → blk_mq_complete_request()
    │
    v
  bio_endio(bio)       bi_end_io 콜백 호출     bio_put() / 자동 해제
  ───────────> [COMPLETE] ──────────────> [CALLBACK] ──────────> [FREE]
```

### 2.2 struct request

`struct request`는 블록 드라이버에게 전달되는 I/O 요청 단위이다. bio가 상위 레이어(파일시스템)의 I/O 표현이라면, request는 하위 레이어(드라이버)의 I/O 표현이다.

**정의 위치:** `include/linux/blk-mq.h` (line 177)

**핵심 필드들:**

```c
struct request {
    /* === 첫 번째 캐시라인 (자주 함께 접근되는 필드들) === */
    struct request_queue *q;        /* 소속 request_queue */
    struct blk_mq_ctx *mq_ctx;     /* 소프트웨어 큐 (제출 CPU 추적) */
    struct blk_mq_hw_ctx *mq_hctx; /* 하드웨어 큐 (NVMe I/O 큐에 대응) */

    blk_opf_t cmd_flags;           /* REQ_OP_READ/WRITE + 플래그 */
    req_flags_t rq_flags;          /* 내부 상태 플래그 (RQF_STARTED 등) */

    int tag;                       /* ★ driver tag = NVMe Command ID */
    int internal_tag;              /* scheduler tag (스케줄러 사용 시) */
    unsigned int timeout;          /* 타임아웃 (jiffies) */

    unsigned int __data_len;       /* 전체 데이터 크기 (바이트) */
    sector_t __sector;             /* 시작 섹터 번호 */

    /* === bio 체인 === */
    struct bio *bio;               /* 첫 번째 bio */
    struct bio *biotail;           /* 마지막 bio */

    /* === 상태 관리 === */
    enum mq_rq_state state;        /* IDLE → IN_FLIGHT → COMPLETE */
    atomic_t ref;                  /* 참조 카운트 (타임아웃/완료 경쟁 방지) */
    unsigned long deadline;        /* 타임아웃 만료 시각 */

    /* === 완료 콜백 === */
    rq_end_io_fn *end_io;         /* passthrough 요청의 완료 콜백 */
    void *end_io_data;            /* 콜백 데이터 */
};
```

**bio와 request의 관계:**

하나의 request에 여러 bio가 병합(merge)될 수 있다. 인접한 섹터에 대한 I/O 요청들이 하나의 request로 합쳐지면, NVMe command 수가 줄어 성능이 향상된다.

```
  request (rq)
  ├── bio → bio_vec[0], bio_vec[1]
  ├── bio → bio_vec[0]
  └── bio → bio_vec[0], bio_vec[1], bio_vec[2]
      ↑
      biotail

  rq->bio 는 체인의 첫 번째, rq->biotail은 마지막
  각 bio는 bi_next로 연결됨
  rq->__data_len = 모든 bio의 크기 합
  rq->__sector = 첫 번째 bio의 시작 섹터
```

**request 메모리 레이아웃 (NVMe):**

```
  ┌──────────────────────┐  offset 0
  │   struct request     │  blk-mq가 관리하는 공통 필드
  ├──────────────────────┤  sizeof(struct request)
  │ struct nvme_request  │  NVMe 공통 필드 (cmd, result, status)
  │ = nvme_req(rq)       │  blk_mq_rq_to_pdu(rq)로 접근
  ├──────────────────────┤
  │ struct nvme_iod      │  NVMe PCIe 전용 (sg list, PRP 등)
  │ (나머지 cmd_size)    │  DMA 매핑 정보 저장
  └──────────────────────┘
```

**request 상태 머신:**

```
  MQ_RQ_IDLE ──blk_mq_start_request()──> MQ_RQ_IN_FLIGHT
                                              │
                                    (NVMe 컨트롤러 처리 중)
                                    (타임아웃 타이머 동작 중)
                                              │
                              blk_mq_complete_request()
                                              │
                                              v
                                       MQ_RQ_COMPLETE
                                              │
                                   blk_mq_end_request()
                                   (tag 반환, request 해제)
                                              │
                                              v
                                        MQ_RQ_IDLE (재사용 가능)
```

### 2.3 struct request_queue

블록 디바이스당 하나 존재하는 큐 관리 구조체이다. NVMe 네임스페이스마다 하나의 request_queue가 있다.

**정의 위치:** `include/linux/blkdev.h`

**주요 필드:**

| 필드 | 설명 |
|------|------|
| `mq_ops` | blk_mq_ops 포인터 (NVMe: &nvme_mq_ops) |
| `queue_hw_ctx[]` | hctx 배열 (NVMe I/O 큐 수만큼) |
| `queue_ctx` | per-cpu ctx (소프트웨어 큐) |
| `tag_set` | blk_mq_tag_set 포인터 |
| `elevator` | I/O 스케줄러 (none이면 NULL) |
| `nr_hw_queues` | 하드웨어 큐 수 |
| `nr_requests` | 큐당 최대 request 수 (기본 128) |
| `queue_flags` | QUEUE_FLAG_* (DYING, QUIESCED 등) |
| `q_usage_counter` | percpu_ref (freeze/drain 메커니즘) |
| `mq_freeze_wq` | freeze 대기 큐 |
| `limits` | queue_limits (max_sectors, max_segments 등) |

**request_queue 초기화 경로 (NVMe):**
```
nvme_alloc_ns()
  → blk_mq_alloc_disk()
    → __blk_mq_alloc_disk()
      → blk_mq_init_queue_data()
        → blk_alloc_queue()     ← block/blk-core.c line 542
```

### 2.4 struct blk_mq_tag_set / blk_mq_hw_ctx / blk_mq_ctx

#### blk_mq_tag_set — 드라이버와 blk-mq의 브릿지

NVMe 드라이버가 이 구조체를 생성하고 blk_mq_alloc_tag_set()을 호출하면, blk-mq가 태그 풀, request 배열, 하드웨어 큐 등을 준비한다.

**정의 위치:** `include/linux/blk-mq.h` (line 659)

```c
struct blk_mq_tag_set {
    const struct blk_mq_ops *ops;       /* 드라이버 콜백 (queue_rq 등) */
    struct blk_mq_queue_map map[HCTX_MAX_TYPES]; /* CPU→hctx 매핑 */
    unsigned int nr_maps;               /* 사용할 map 수 (1~3) */
    unsigned int nr_hw_queues;          /* 하드웨어 큐 수 (모든 타입 합) */
    unsigned int queue_depth;           /* 큐당 태그 수 (NVMe: 보통 1023) */
    unsigned int reserved_tags;         /* 예약 태그 수 */
    unsigned int cmd_size;              /* request PDU 크기 (NVMe: sizeof(nvme_iod)) */
    int numa_node;                      /* NUMA 노드 */
    unsigned int timeout;               /* 타임아웃 (jiffies) */
    unsigned int flags;                 /* BLK_MQ_F_* 플래그 */
    void *driver_data;                  /* NVMe: nvme_dev* */
    struct blk_mq_tags **tags;          /* 큐별 태그 세트 배열 */
    struct blk_mq_tags *shared_tags;    /* 공유 태그 (설정된 경우) */
    struct list_head tag_list;          /* 이 tag_set을 공유하는 request_queue들 */
};
```

tag_set은 여러 request_queue(= NVMe 네임스페이스)가 공유할 수 있다. 하나의 NVMe 디바이스에 네임스페이스 2개가 있으면, 두 네임스페이스의 request_queue가 같은 tag_set을 사용한다.

#### blk_mq_hw_ctx (hctx) — 하드웨어 큐 컨텍스트

**정의 위치:** `include/linux/blk-mq.h` (line 433)

NVMe에서 각 hctx는 하나의 NVMe I/O Submission Queue에 대응한다.

```c
struct blk_mq_hw_ctx {
    spinlock_t lock;                /* dispatch 리스트 보호 lock */
    struct list_head dispatch;      /* 재시도 대기 request 리스트 */
    unsigned long state;            /* BLK_MQ_S_* (STOPPED, INACTIVE 등) */

    struct delayed_work run_work;   /* 비동기 큐 실행용 work */
    cpumask_var_t cpumask;          /* 이 hctx를 사용하는 CPU들 */

    void *driver_data;              /* NVMe: nvme_queue* */
    struct sbitmap ctx_map;         /* pending 소프트웨어 큐 비트맵 */

    struct blk_mq_tags *tags;       /* driver tag (Command ID) */
    struct blk_mq_tags *sched_tags; /* scheduler tag (스케줄러 사용 시) */

    unsigned int dispatch_busy;     /* busy 상태 EWMA 추적값 */
    unsigned short type;            /* HCTX_TYPE_DEFAULT/READ/POLL */
    unsigned short nr_ctx;          /* 매핑된 소프트웨어 큐 수 */
    struct blk_mq_ctx **ctxs;      /* 매핑된 소프트웨어 큐 배열 */

    struct blk_mq_ctx *dispatch_from; /* 스케줄러 없을 때 디스패치 시작 ctx */
    wait_queue_entry_t dispatch_wait;  /* tag 대기 큐 */

    unsigned int queue_num;         /* 이 하드웨어 큐의 인덱스 */
    atomic_t nr_active;             /* 활성 request 수 (공유 태그 시) */
};
```

#### blk_mq_ctx (ctx) — 소프트웨어 큐 (per-CPU)

**정의 위치:** `block/blk-mq.h` (line 19)

```c
struct blk_mq_ctx {
    spinlock_t lock;                            /* rq_lists 보호 */
    struct list_head rq_lists[HCTX_MAX_TYPES];  /* 타입별 request 대기 리스트 */

    unsigned int cpu;                           /* 이 ctx의 CPU 번호 */
    unsigned short index_hw[HCTX_MAX_TYPES];    /* 이 ctx의 hctx 내 인덱스 */
    struct blk_mq_hw_ctx *hctxs[HCTX_MAX_TYPES]; /* 타입별 연결된 hctx */

    struct request_queue *queue;                /* 소속 request_queue */
};
```

**Software Context와 Hardware Context의 관계:**

```
  CPU 0 ──> ctx[0] ─┐
  CPU 1 ──> ctx[1] ─┤──> hctx[0] (NVMe SQ/CQ pair 1)
                     │
  CPU 2 ──> ctx[2] ─┤
  CPU 3 ──> ctx[3] ─┘──> hctx[1] (NVMe SQ/CQ pair 2)

  (CPU 수 > NVMe 큐 수일 때 N:1 매핑 예시)
```

보통 NVMe에서는 CPU 수만큼 I/O 큐를 생성하여 1:1 매핑을 한다. 이 경우 각 CPU는 전용 NVMe 큐를 가지므로 lock 경합이 전혀 없다.

---

## 3. I/O 제출 경로 (submit_bio → 드라이버)

### 3.1 submit_bio() 진입

전체 호출 체인은 다음과 같다.

```
  애플리케이션: read() / write() / io_uring_enter()
       │
       v
  VFS → 파일시스템 (ext4, xfs 등)
       │
       v
  submit_bio(bio)                         ← blk-core.c line 1244
       │  task_io_account_read/write()     (I/O 통계)
       │  bio_set_ioprio()                 (I/O 우선순위 설정)
       v
  submit_bio_noacct(bio)                  ← blk-core.c line 1064
       │  유효성 검사 9종:
       │    - REQ_NOWAIT 지원 확인
       │    - fault injection 검사
       │    - 읽기 전용 검사 (bio_check_ro)
       │    - EOD(End Of Device) 검사 (bio_check_eod)
       │    - 파티션 리매핑 (blk_partition_remap)
       │    - flush 필터링 (write cache 없으면 flush 제거)
       │    - 오퍼레이션 유형별 검증 (DISCARD, WRITE_ZEROES 등)
       │    - atomic write 크기 검증
       │    - cgroup I/O throttling (blk_throtl_bio)
       v
  submit_bio_noacct_nocheck(bio)          ← blk-core.c line 986
       │  blk_cgroup_bio_start()           (cgroup 등록)
       │  trace_block_bio_queue()          (트레이싱)
       │  재귀 방지 체크 (current->bio_list)
       v
  __submit_bio_noacct_mq(bio)             ← blk-core.c line 957
       │  (blk-mq 디바이스용 재귀 방지 루프)
       │  current->bio_list 설정
       v
  __submit_bio(bio)                       ← blk-core.c line 827
       │  blk_crypto_bio_prep()            (암호화 전처리)
       │  blk_start_plug() / blk_finish_plug()
       v
  blk_mq_submit_bio(bio)                 ← blk-mq.c line 3697 (★ 핵심)
```

#### submit_bio() — 최상위 진입점

**위치:** `block/blk-core.c` line 1244

```c
void submit_bio(struct bio *bio)
{
    // READ이면 태스크별 I/O 통계 업데이트 + PGPGIN 카운터 증가
    if (bio_op(bio) == REQ_OP_READ) {
        task_io_account_read(bio->bi_iter.bi_size);
        count_vm_events(PGPGIN, bio_sectors(bio));
    } else if (bio_op(bio) == REQ_OP_WRITE) {
        count_vm_events(PGPGOUT, bio_sectors(bio));
    }

    bio_set_ioprio(bio);      // I/O 우선순위 결정 (nice 값 기반)
    submit_bio_noacct(bio);   // 유효성 검사 후 제출
}
```

#### __submit_bio() — 실제 디스패치

**위치:** `block/blk-core.c` line 827

```c
static void __submit_bio(struct bio *bio)
{
    struct blk_plug plug;

    if (unlikely(!blk_crypto_bio_prep(&bio)))
        return;

    blk_start_plug(&plug);    // I/O 배치를 위한 plug 시작

    if (!bdev_test_flag(bio->bi_bdev, BD_HAS_SUBMIT_BIO)) {
        // ★ NVMe를 포함한 대부분의 블록 디바이스는 이 경로
        blk_mq_submit_bio(bio);
    } else if (likely(bio_queue_enter(bio) == 0)) {
        // device-mapper, md RAID 등 자체 submit_bio가 있는 디바이스
        struct gendisk *disk = bio->bi_bdev->bd_disk;
        disk->fops->submit_bio(bio);
        blk_queue_exit(disk->queue);
    }

    blk_finish_plug(&plug);   // 축적된 I/O를 일괄 제출
}
```

### 3.2 blk_mq_submit_bio() 상세

이 함수가 blk-mq의 핵심이다. bio가 블록 레이어에 진입하여 NVMe 드라이버까지 도달하는 전체 과정을 관장한다.

**위치:** `block/blk-mq.c` line 3697

```c
void blk_mq_submit_bio(struct bio *bio)
{
    struct request_queue *q = bdev_get_queue(bio->bi_bdev);
    // ① request_queue를 구한다 (NVMe 네임스페이스의 큐)

    struct blk_plug *plug = current->plug;
    // ② 현재 태스크의 plug 확인 (blk_start_plug()으로 설정됨)

    const int is_sync = op_is_sync(bio->bi_opf);
    struct blk_mq_hw_ctx *hctx;
    unsigned int nr_segs;
    struct request *rq;
    blk_status_t ret;

    // ③ plug에 캐시된 request가 있으면 재사용 시도
    rq = blk_mq_peek_cached_request(plug, q, bio->bi_opf);
    // 캐시된 request는 이전 제출에서 미리 할당해둔 것으로,
    // blk_queue_enter()를 다시 호출하지 않아도 된다 (빠른 경로)

    // ④ 큐 진입 (q_usage_counter 증가)
    if (!rq) {
        if (unlikely(bio_queue_enter(bio)))
            return;  // 큐가 frozen 상태이면 실패
    }

    // ⑤ 정렬(alignment) 검사 - logical block size에 맞는지
    if (unlikely(bio_unaligned(bio, q))) {
        bio_io_error(bio);
        goto queue_exit;
    }

    // ⑥ 폴링 지원 검사 (REQ_POLLED bio인데 폴링 큐가 없으면 에러)
    if ((bio->bi_opf & REQ_POLLED) && !blk_mq_can_poll(q)) {
        bio->bi_status = BLK_STS_NOTSUPP;
        bio_endio(bio);
        goto queue_exit;
    }

    // ⑦ bio 분할: 큐의 제한(max_sectors, max_segments)에 맞게 bio를 나눔
    // 예: 1MB 요청인데 max_sectors가 512KB이면 두 개로 분할됨
    bio = __bio_split_to_limits(bio, &q->limits, &nr_segs);
    if (!bio)
        goto queue_exit;

    // ⑧ 데이터 무결성(T10-PI) 준비
    if (!bio_integrity_prep(bio))
        goto queue_exit;

    // ⑨ ★ 병합(merge) 시도
    // plug 리스트와 스케줄러에서 이 bio를 합칠 수 있는 기존 request를 찾는다
    // 병합 성공하면 새 request 할당 없이 종료 (큰 성능 이점!)
    if (blk_mq_attempt_bio_merge(q, bio, nr_segs))
        goto queue_exit;

    // ⑩ ★ request 할당 및 bio→request 변환
new_request:
    if (rq) {
        blk_mq_use_cached_rq(rq, plug, bio);  // 캐시된 request 재사용
    } else {
        rq = blk_mq_get_new_requests(q, plug, bio);
        // 내부에서 __blk_mq_alloc_requests() 호출:
        //   1) blk_mq_get_ctx(q)    → 현재 CPU의 소프트웨어 큐
        //   2) blk_mq_map_queue()   → 적절한 하드웨어 큐 선택
        //   3) blk_mq_get_tag()     → sbitmap에서 태그 할당
        //   4) blk_mq_rq_ctx_init() → request 초기화
        if (unlikely(!rq)) {
            if (bio->bi_opf & REQ_NOWAIT)
                bio_wouldblock_error(bio);
            goto queue_exit;
        }
    }

    // ⑪ bio 정보를 request에 복사
    blk_mq_bio_to_request(rq, bio, nr_segs);
    // rq->__sector = bio->bi_iter.bi_sector
    // rq->__data_len = bio->bi_iter.bi_size
    // rq->bio = rq->biotail = bio
    // blk_account_io_start() → I/O 통계 시작

    // ⑫ flush 처리 (REQ_PREFLUSH, REQ_FUA 등)
    if (op_is_flush(bio->bi_opf) && blk_insert_flush(rq))
        return;

    // ⑬ ★★★ 디스패치 경로 선택 ★★★
    if (plug) {
        // 경로 A: Plug이 있는 경우 (가장 일반적)
        // request를 plug 리스트에 추가하고 즉시 반환
        // blk_finish_plug() 시 한꺼번에 디스패치됨
        blk_add_rq_to_plug(plug, rq);
        return;
    }

    hctx = rq->mq_hctx;
    if ((rq->rq_flags & RQF_USE_SCHED) ||
        (hctx->dispatch_busy && (q->nr_hw_queues == 1 || !is_sync))) {
        // 경로 B: 스케줄러 사용 중이거나 디바이스가 busy
        // 소프트웨어 큐/스케줄러에 삽입 + 비동기 실행
        blk_mq_insert_request(rq, 0);
        blk_mq_run_hw_queue(hctx, true);
    } else {
        // 경로 C: 직접 디스패치 (★ NVMe 최단 경로!)
        // 스케줄러 없고 디바이스가 idle하면 바로 드라이버 호출
        blk_mq_run_dispatch_ops(q,
            blk_mq_try_issue_directly(hctx, rq));
    }
    return;

queue_exit:
    if (!rq)
        blk_queue_exit(q);
}
```

**디스패치 경로 선택 로직 요약:**

```
  blk_mq_submit_bio()에서 디스패치 경로 결정:

  ┌──────────────────────────────────────┐
  │  plug이 있는가?                       │
  │  (blk_start_plug()으로 설정됨)        │
  └────────────┬──────────┬──────────────┘
               │ Yes      │ No
               v          v
  ┌──────────────┐  ┌──────────────────────────┐
  │  경로 A:     │  │  스케줄러 사용 중이거나    │
  │  Plug에 추가 │  │  hctx가 busy인가?         │
  │  (나중에     │  └─────────┬─────────┬──────┘
  │   한꺼번에)  │            │ Yes     │ No
  └──────────────┘            v         v
                   ┌──────────────┐ ┌──────────────┐
                   │  경로 B:     │ │  경로 C:     │
                   │  큐에 삽입   │ │  직접 디스패치│
                   │  + 비동기    │ │  (★최단경로) │
                   │  실행        │ │  → nvme_     │
                   └──────────────┘ │  queue_rq()  │
                                    └──────────────┘
```

### 3.3 Request Merge

병합(merge)은 인접한 섹터에 대한 I/O 요청들을 하나의 request로 합치는 최적화이다. 병합이 성공하면 NVMe command 수가 줄어 성능이 향상된다.

**병합 시도 함수:** `blk_mq_attempt_bio_merge()` (blk-mq.c line 3570)

```c
static bool blk_mq_attempt_bio_merge(struct request_queue *q,
                                     struct bio *bio, unsigned int nr_segs)
{
    if (!blk_queue_nomerges(q) && bio_mergeable(bio)) {
        // 1단계: plug 리스트에서 병합 대상 검색
        if (blk_attempt_plug_merge(q, bio, nr_segs))
            return true;
        // 2단계: 스케줄러의 request 풀에서 검색
        if (blk_mq_sched_bio_merge(q, bio, nr_segs))
            return true;
    }
    return false;
}
```

**병합의 종류:**

```
  Back Merge (가장 흔함):
  ┌──────────────────┐     ┌──────────────────┐
  │ 기존 request     │     │ 새 bio           │
  │ sector: 0~99     │  +  │ sector: 100~199  │
  └──────────────────┘     └──────────────────┘
           ↓
  ┌──────────────────────────────────────────┐
  │ 병합된 request                            │
  │ sector: 0~199                             │
  └──────────────────────────────────────────┘

  Front Merge:
  ┌──────────────────┐     ┌──────────────────┐
  │ 새 bio           │     │ 기존 request     │
  │ sector: 0~99     │  +  │ sector: 100~199  │
  └──────────────────┘     └──────────────────┘
           ↓
  ┌──────────────────────────────────────────┐
  │ 병합된 request                            │
  │ sector: 0~199                             │
  └──────────────────────────────────────────┘
```

**병합 가능 조건 (blk_rq_merge_ok):** (`block/blk-merge.c` line 878)

```c
bool blk_rq_merge_ok(struct request *rq, struct bio *bio)
{
    if (!rq_mergeable(rq) || !bio_mergeable(bio))
        return false;               // RQF_NOMERGE_FLAGS 체크
    if (req_op(rq) != bio_op(bio))
        return false;               // 오퍼레이션 타입 일치해야 함
    if (!blk_cgroup_mergeable(rq, bio))
        return false;               // cgroup이 같아야 함
    if (blk_integrity_merge_bio(rq->q, rq, bio) == false)
        return false;               // integrity 호환성
    if (!bio_crypt_rq_ctx_compatible(rq, bio))
        return false;               // 암호화 키 일치
    if (rq->bio->bi_write_hint != bio->bi_write_hint)
        return false;               // write hint 일치
    if (rq->bio->bi_ioprio != bio->bi_ioprio)
        return false;               // I/O 우선순위 일치
    // ... (추가 검사들)
    return true;
}
```

### 3.4 blk_mq_dispatch_rq_list() — 드라이버로 request 전달

이 함수가 blk-mq와 NVMe 드라이버를 연결하는 핵심 지점이다. request 리스트의 각 항목에 대해 `q->mq_ops->queue_rq()`를 호출한다.

**위치:** `block/blk-mq.c` line 2502

```c
bool blk_mq_dispatch_rq_list(struct blk_mq_hw_ctx *hctx,
                              struct list_head *list, bool get_budget)
{
    enum prep_dispatch prep;
    struct request_queue *q = hctx->queue;
    struct request *rq;
    int queued;
    blk_status_t ret = BLK_STS_OK;
    bool needs_resource = false;

    if (list_empty(list))
        return false;

    queued = 0;
    do {
        struct blk_mq_queue_data bd;

        rq = list_first_entry(list, struct request, queuelist);
        WARN_ON_ONCE(hctx != rq->mq_hctx);

        // dispatch budget과 driver tag를 확보한다
        // NVMe는 budget을 사용하지 않지만 driver tag는 필요
        prep = blk_mq_prep_dispatch_rq(rq, get_budget);
        if (prep != PREP_DISPATCH_OK)
            break;

        list_del_init(&rq->queuelist);

        bd.rq = rq;
        bd.last = list_empty(list);
        // ★ bd.last: NVMe가 SQ Doorbell write 여부를 결정하는 플래그
        // last=true일 때만 Doorbell을 눌러 여러 command를 한번의 MMIO로 전달

        // ★★★ 여기서 NVMe 드라이버의 nvme_queue_rq()가 호출된다! ★★★
        ret = q->mq_ops->queue_rq(hctx, &bd);

        switch (ret) {
        case BLK_STS_OK:
            queued++;
            break;
        case BLK_STS_RESOURCE:
            // 블록 레이어 리소스 부족 (tag 등)
            needs_resource = true;
            fallthrough;
        case BLK_STS_DEV_RESOURCE:
            // 디바이스 리소스 부족 (NVMe SQ 가득 참)
            // request를 dispatch 리스트에 다시 넣고 나중에 재시도
            blk_mq_handle_dev_resource(rq, list);
            goto out;
        default:
            // 복구 불가능한 에러. request를 에러로 즉시 완료
            blk_mq_end_request(rq, ret);
        }
    } while (!list_empty(list));

out:
    // 리스트를 다 처리하지 못했으면 commit_rqs() 호출
    // → NVMe: SQ Doorbell write (이미 SQ에 기록된 command 처리)
    if (!list_empty(list) || ret != BLK_STS_OK)
        blk_mq_commit_rqs(hctx, queued, false);

    // 미처리 request는 hctx->dispatch에 다시 넣는다
    if (!list_empty(list)) {
        spin_lock(&hctx->lock);
        list_splice_tail_init(list, &hctx->dispatch);
        spin_unlock(&hctx->lock);

        // dispatch_busy EWMA 갱신 (busy=true)
        blk_mq_update_dispatch_busy(hctx, true);
        return false;
    }

    // 모든 request 성공적으로 전달됨
    blk_mq_update_dispatch_busy(hctx, false);
    return true;
}
```

---

## 4. Plug/Unplug 메커니즘

### 4.1 개요

Plug(플러그)란 배수구의 마개와 같은 개념이다. I/O 요청을 잠시 모아두었다가 한꺼번에 내보내는 배치(batching) 메커니즘이다.

**왜 필요한가?**

1. **NVMe Doorbell write 최소화**: 10개의 command를 개별 제출하면 Doorbell을 10번 써야 하지만, 모아서 제출하면 1번만 쓰면 된다. Doorbell write는 PCIe MMIO 연산으로 비용이 크다.
2. **병합(merge) 기회 증가**: 연속된 섹터의 I/O를 모아두면 하나의 큰 request로 합칠 수 있다.
3. **queue_rqs() 배치 인터페이스 활용**: NVMe의 `nvme_queue_rqs()`는 여러 request를 한 번에 처리하여 lock 획득/해제를 최소화한다.

### 4.2 blk_start_plug() / blk_finish_plug()

```c
// 플러그 시작 — I/O 축적 시작
void blk_start_plug(struct blk_plug *plug)
{
    // current->plug에 plug 구조체를 설정
    // 이후 submit_bio() 호출 시 request가 plug->mq_list에 추가됨
}

// 플러그 종료 — 축적된 I/O를 한꺼번에 제출
void blk_finish_plug(struct blk_plug *plug)
{
    // plug->mq_list에 모인 request들을 blk_mq_flush_plug_list()로 디스패치
}
```

**파일시스템에서의 사용 예:**

```c
// ext4 readahead 예시
void ext4_readahead(struct readahead_control *rac)
{
    struct blk_plug plug;
    blk_start_plug(&plug);    // 플러그 시작

    // 여러 페이지에 대해 반복적으로 submit_bio() 호출
    // → 각 bio가 request로 변환되어 plug->mq_list에 축적됨
    mpage_readahead(rac, ext4_get_block);

    blk_finish_plug(&plug);   // 플러그 종료 → 한꺼번에 NVMe에 전달
}
```

### 4.3 blk_add_rq_to_plug() — Plug 리스트에 request 추가

**위치:** `block/blk-mq.c` line 1720

```c
static void blk_add_rq_to_plug(struct blk_plug *plug, struct request *rq)
{
    struct request *last = rq_list_peek(&plug->mq_list);

    if (!plug->rq_count) {
        trace_block_plug(rq->q);
    } else if (plug->rq_count >= blk_plug_max_rq_count(plug) ||
               (!blk_queue_nomerges(rq->q) &&
                blk_rq_bytes(last) >= BLK_PLUG_FLUSH_SIZE)) {
        // plug이 가득 찼으면 자동 flush
        blk_mq_flush_plug_list(plug, false);
        last = NULL;
        trace_block_plug(rq->q);
    }

    // 여러 큐의 request가 섞여있는지 추적
    if (!plug->multiple_queues && last && last->q != rq->q)
        plug->multiple_queues = true;

    // 스케줄러 태그를 가진 request가 있는지 추적
    if (!plug->has_elevator && (rq->rq_flags & RQF_SCHED_TAGS))
        plug->has_elevator = true;

    // plug 리스트 끝에 request 추가
    rq_list_add_tail(&plug->mq_list, rq);
    plug->rq_count++;
}
```

### 4.4 Plug/Unplug 흐름도

```
  blk_start_plug(&plug)
       │
       │  current->plug = &plug
       │  plug->mq_list = 빈 리스트
       │  plug->rq_count = 0
       │
       v
  submit_bio(bio_1) ──> blk_mq_submit_bio()
       │                    │ 병합 실패 → request 할당
       │                    │ blk_add_rq_to_plug(plug, rq_1)
       │                    │ plug->rq_count = 1
       v
  submit_bio(bio_2) ──> blk_mq_submit_bio()
       │                    │ plug 리스트에서 rq_1과 병합 시도
       │                    │ 병합 실패 → request 할당
       │                    │ blk_add_rq_to_plug(plug, rq_2)
       │                    │ plug->rq_count = 2
       v
  submit_bio(bio_3) ──> blk_mq_submit_bio()
       │                    │ plug 리스트에서 rq_2와 back merge 성공!
       │                    │ → 새 request 할당 없음 (rq_2에 bio_3 추가)
       │                    │ plug->rq_count = 2 (변화 없음)
       v
  blk_finish_plug(&plug)
       │
       │  blk_mq_flush_plug_list(plug)
       │      │
       │      │ queue_rqs가 있으면 (NVMe):
       │      │   nvme_queue_rqs(&rq_list) → 배치 제출
       │      │     rq_1: SQ에 command 기록 (Doorbell 안 누름)
       │      │     rq_2: SQ에 command 기록 + Doorbell write
       │      │     (마지막 request에서만 Doorbell 1회!)
       │      │
       │      │ queue_rqs가 없으면:
       │      │   각 request에 대해 queue_rq() 개별 호출
       │
       v
  current->plug = NULL
```

---

## 5. I/O 스케줄러 연동

### 5.1 스케줄러 종류

Linux blk-mq는 다음 4가지 I/O 스케줄러를 지원한다.

| 스케줄러 | 특징 | NVMe에서의 적합성 |
|---------|------|------------------|
| **none** | 스케줄러 없음. FIFO 순서로 직접 디스패치 | ★★★ NVMe에 가장 적합. 최소 오버헤드 |
| **mq-deadline** | deadline 기반. 읽기 우선, starvation 방지 | ★★ HDD/SATA에 적합. NVMe에서는 불필요한 오버헤드 |
| **bfq** | Budget Fair Queueing. 프로세스별 공정 분배 | ★ 데스크탑 환경에 적합. 서버에서는 오버헤드 큼 |
| **kyber** | latency-aware. 읽기/쓰기별 큐 깊이 자동 조절 | ★★ NVMe에서 레이턴시 최적화 시 유용 |

### 5.2 스케줄러 없을 때 vs 있을 때의 경로 차이

**스케줄러 없음 (none) — NVMe의 기본 설정:**

```
  blk_mq_submit_bio()
    │
    ├── tag 할당: driver tag를 직접 할당 (rq->tag)
    │   (sbitmap에서 비트 하나 확보 = NVMe Command ID)
    │
    ├── 디스패치:
    │   ├── plug 있음 → plug 리스트에 추가
    │   ├── dispatch_busy가 낮음 → blk_mq_try_issue_directly()
    │   │                          → __blk_mq_issue_directly()
    │   │                          → nvme_queue_rq() [★ 직접 호출!]
    │   └── dispatch_busy가 높음 → ctx->rq_lists[]에 삽입
    │                              → blk_mq_run_hw_queue()
    │                              → blk_mq_flush_busy_ctxs()
    │                              → blk_mq_dispatch_rq_list()
    │                              → nvme_queue_rq()
    │
    (request 할당 시 이미 driver tag가 있으므로 추가 할당 불필요)
```

**스케줄러 있음 (예: mq-deadline):**

```
  blk_mq_submit_bio()
    │
    ├── tag 할당: scheduler tag를 할당 (rq->internal_tag)
    │   (driver tag는 나중에 디스패치 시점에 할당)
    │
    ├── 디스패치:
    │   ├── plug 있음 → plug 리스트에 추가
    │   └── plug 없음 → blk_mq_insert_request()
    │                    → elevator->ops.insert_requests()
    │                    (스케줄러의 내부 큐에 삽입)
    │                    → blk_mq_run_hw_queue()
    │
    │   blk_mq_sched_dispatch_requests(hctx)
    │     ├── hctx->dispatch 리스트 먼저 처리 (재시도 request)
    │     └── __blk_mq_do_dispatch_sched()
    │           │ elevator->ops.dispatch_request(hctx) 호출
    │           │ → 스케줄러가 request를 선택하여 반환
    │           │
    │           │ blk_mq_get_driver_tag(rq)
    │           │ → 이 시점에서 driver tag 할당 (NVMe Command ID)
    │           │
    │           └── blk_mq_dispatch_rq_list()
    │                → nvme_queue_rq()
    │
    (driver tag 할당이 2단계로 분리: 먼저 scheduler tag, 나중에 driver tag)
```

### 5.3 blk_mq_sched_dispatch_requests()

스케줄러 연동의 핵심 디스패치 함수이다.

**위치:** `block/blk-mq-sched.c`

이 함수의 디스패치 우선순위는 다음과 같다.

1. `hctx->dispatch` 리스트 (이전에 BUSY로 재시도 대기 중인 request)
2. 스케줄러의 `dispatch_request()` (스케줄러가 최적 순서로 선택)
3. 스케줄러 없으면 `blk_mq_flush_busy_ctxs()` (소프트웨어 큐에서 수집)

---

## 6. Tag 시스템

### 6.1 Tag의 의미

blk-mq에서 "태그(tag)"는 진행 중인 I/O request를 식별하는 고유 정수 번호이다. NVMe 드라이버에서 이 태그는 NVMe 커맨드의 `command_id` 필드로 직접 사용된다.

```
  Tag 할당:                    NVMe 제출:                    NVMe 완료:
  ┌─────────────┐             ┌─────────────┐              ┌────────────┐
  │ sbitmap에서  │             │ SQE 구성:   │              │ CQE 수신:  │
  │ 비트 5 확보  │──tag=5──>  │ command_id=5│──디바이스──>  │ cid=5      │
  │             │             │ opcode=READ │              │ status=OK  │
  └─────────────┘             │ slba=100    │              └────┬───────┘
                              │ nlb=8       │                   │
                              └─────────────┘                   v
                                                        tags->rqs[5]로
                                                        원래 request 찾기
```

### 6.2 blk_mq_tags 구조

```
  struct blk_mq_tags {
      unsigned int nr_tags;           /* 전체 태그 수 (= queue_depth) */
      unsigned int nr_reserved_tags;  /* 예약 태그 수 */

      struct sbitmap_queue bitmap_tags;     /* 일반 태그 비트맵 */
      struct sbitmap_queue breserved_tags;  /* 예약 태그 비트맵 */

      struct request **rqs;           /* tag → request 매핑 배열 */
      struct request **static_rqs;    /* 미리 할당된 request 배열 */
  };
```

**태그 공간 레이아웃:**

```
  태그 번호: [0 ... nr_reserved_tags-1] [nr_reserved_tags ... nr_tags-1]
             └── 예약 태그 영역 ──┘    └── 일반 태그 영역 ────────────┘
             (breserved_tags 비트맵)     (bitmap_tags 비트맵)

  NVMe 예시 (queue_depth=1024, reserved_tags=0):
    태그 범위: 0 ~ 1023
    모두 일반 태그로 사용
    동시에 최대 1024개의 NVMe command가 진행 가능
```

### 6.3 Tag 할당: blk_mq_get_tag()

**위치:** `block/blk-mq-tag.c` line 238

이 함수는 sbitmap에서 빈 비트를 찾아 태그를 할당한다. NVMe에서 이 태그가 곧 Command ID가 된다.

```c
unsigned int blk_mq_get_tag(struct blk_mq_alloc_data *data)
{
    struct blk_mq_tags *tags = blk_mq_tags_from_data(data);
    struct sbitmap_queue *bt;
    unsigned int tag_offset;
    int tag;

    // 예약 태그와 일반 태그 구분
    if (data->flags & BLK_MQ_REQ_RESERVED) {
        bt = &tags->breserved_tags;
        tag_offset = 0;
    } else {
        bt = &tags->bitmap_tags;
        tag_offset = tags->nr_reserved_tags;
    }

    // 1차 시도: sbitmap에서 빈 비트 찾기
    tag = __blk_mq_get_tag(data, bt);
    if (tag != BLK_MQ_NO_TAG)
        goto found_tag;

    // NOWAIT이면 즉시 실패 (io_uring 비동기 I/O)
    if (data->flags & BLK_MQ_REQ_NOWAIT)
        return BLK_MQ_NO_TAG;

    // 태그 대기 루프: 빈 태그가 나올 때까지 sleep
    ws = bt_wait_ptr(bt, data->hctx);
    do {
        // HW 큐를 돌려서 완료된 I/O의 태그를 회수
        blk_mq_run_hw_queue(data->hctx, false);

        // 재시도
        tag = __blk_mq_get_tag(data, bt);
        if (tag != BLK_MQ_NO_TAG)
            break;

        // sleep 준비 + 마지막 시도
        sbitmap_prepare_to_wait(bt, ws, &wait, TASK_UNINTERRUPTIBLE);
        tag = __blk_mq_get_tag(data, bt);
        if (tag != BLK_MQ_NO_TAG)
            break;

        // CPU를 양보하고 sleep
        // 다른 I/O가 완료되어 blk_mq_put_tag()이 호출되면 깨어남
        io_schedule();

        sbitmap_finish_wait(bt, ws, &wait);

        // sleep에서 깨어난 후 CPU 매핑이 변경되었을 수 있음
        data->ctx = blk_mq_get_ctx(data->q);
        data->hctx = blk_mq_map_queue(data->cmd_flags, data->ctx);
        // ...
    } while (1);

found_tag:
    return tag + tag_offset;
}
```

**핵심 포인트:**

- 태그가 모두 소진되면 `io_schedule()`로 프로세스가 sleep한다. 이는 NVMe 큐가 가득 찼다는 의미이다.
- SPDK는 커널 블록 레이어를 거치지 않으므로 이 태그 시스템이 없다. 유저 공간에서 직접 SQ tail/head를 비교하여 여유 슬롯을 판단한다.
- sbitmap은 CPU별로 비트맵을 분할하여 캐시 라인 경합을 최소화한다.

### 6.4 Tag 해제: blk_mq_put_tag()

**위치:** `block/blk-mq-tag.c` line 379

```c
void blk_mq_put_tag(struct blk_mq_tags *tags, struct blk_mq_ctx *ctx,
                    unsigned int tag)
{
    if (!blk_mq_tag_is_reserved(tags, tag)) {
        const int real_tag = tag - tags->nr_reserved_tags;
        BUG_ON(real_tag >= tags->nr_tags);
        sbitmap_queue_clear(&tags->bitmap_tags, real_tag, ctx->cpu);
        // sbitmap에서 비트를 clear하면 대기 중인 프로세스가 깨어남
    } else {
        sbitmap_queue_clear(&tags->breserved_tags, tag, ctx->cpu);
    }
}
```

### 6.5 Shared Tag vs Per-HW-Queue Tag

- **Per-HW-Queue Tag (기본)**: 각 hctx가 독립적인 태그 풀을 가진다. NVMe에서 각 I/O 큐가 독립적인 Command ID 공간을 사용하는 것에 대응한다.
- **Shared Tag**: `BLK_MQ_F_TAG_QUEUE_SHARED` 플래그 사용 시. 여러 request_queue(네임스페이스)가 하나의 태그 풀을 공유한다. `hctx_may_queue()`로 한 큐가 태그를 독점하지 못하게 공정성을 보장한다.

---

## 7. 완료 경로 (Completion)

### 7.1 전체 완료 흐름

```
  ┌─────────────────────────────────────────────────┐
  │              NVMe 컨트롤러 (하드웨어)             │
  │  command 처리 완료 → CQE를 CQ에 기록             │
  │  → MSI-X 인터럽트 발생                           │
  └──────────────────────┬──────────────────────────┘
                         │
                         v
  nvme_irq() → nvme_poll_cq() → nvme_handle_cqe()
                         │
                         │  CQE에서 command_id(= tag) 추출
                         │  tags->rqs[command_id]로 request 찾기
                         │
                         v
  ┌─────────────────────────────────────────────────┐
  │ blk_mq_complete_request(rq)  ← blk-mq.c:1626   │
  │                                                   │
  │   ┌─ blk_mq_complete_request_remote(rq)          │
  │   │   WRITE_ONCE(rq->state, MQ_RQ_COMPLETE)     │
  │   │                                               │
  │   │   완료 처리 CPU 결정:                          │
  │   │   ├── hctx에 ctx 1개 + 현재 CPU → 로컬 완료  │
  │   │   ├── polled I/O → 로컬 완료                  │
  │   │   ├── IPI 필요 → 제출 CPU로 IPI 전송          │
  │   │   ├── 단일 hw 큐 → softirq로 완료             │
  │   │   └── 그 외 → 로컬 완료                       │
  │   │                                               │
  │   └── 로컬 완료 시:                                │
  │       q->mq_ops->complete(rq)                     │
  │       = nvme_pci_complete_rq(rq)                  │
  └──────────────────────┬──────────────────────────┘
                         │
                         v
  nvme_pci_complete_rq() → nvme_complete_rq()
                         │
                         v
  ┌─────────────────────────────────────────────────┐
  │ blk_mq_end_request(rq, status)  ← blk-mq.c:1392│
  │                                                   │
  │   ① blk_update_request(rq, error, blk_rq_bytes)  │
  │      │  bio 체인을 순회하며 각 bio에 대해:        │
  │      │  ├── error이면 bio->bi_status에 설정       │
  │      │  ├── bio_advance()로 완료된 바이트 반영     │
  │      │  └── bio_endio(bio) 호출                   │
  │      │       → bi_end_io 콜백 실행                │
  │      │       → 파일시스템에 I/O 완료 통보          │
  │      │       → (fio: aio_complete로 완료 알림)    │
  │      │                                             │
  │   ② __blk_mq_end_request(rq, error)               │
  │      │  통계 업데이트 (레이턴시, IOPS)             │
  │      │  blk_mq_finish_request()                   │
  │      │  blk_mq_free_request()                     │
  │      │    ├── WRITE_ONCE(rq->state, MQ_RQ_IDLE)  │
  │      │    ├── blk_mq_put_tag() → tag 반환         │
  │      │    │   (대기 중인 프로세스 깨움)            │
  │      │    └── blk_queue_exit() → q_usage_counter--│
  │      │                                             │
  └─────────────────────────────────────────────────┘
```

### 7.2 blk_mq_complete_request() — 완료 진입점

**위치:** `block/blk-mq.c` line 1626

```c
void blk_mq_complete_request(struct request *rq)
{
    // 원격 완료가 불필요하면 현재 CPU에서 직접 complete 콜백 호출
    if (!blk_mq_complete_request_remote(rq))
        rq->q->mq_ops->complete(rq);
    // NVMe에서 complete = nvme_pci_complete_rq()
}
```

### 7.3 Softirq vs Direct Completion

완료 처리가 발생하는 컨텍스트는 두 가지이다.

**Direct Completion (직접 완료):**
- 인터럽트 핸들러 또는 폴링 컨텍스트에서 직접 `mq_ops->complete(rq)` 호출
- 조건: hctx에 ctx가 1개뿐이고 현재 CPU가 그 ctx의 CPU이거나, polled I/O인 경우
- 장점: softirq 지연 없이 즉시 완료
- NVMe에서 CPU별 전용 큐를 사용하면 대부분 이 경로

**Softirq Completion (소프트 인터럽트 완료):**
- `per_cpu(blk_cpu_done)` 리스트에 request를 추가하고 `BLOCK_SOFTIRQ`를 발생
- softirq 핸들러 `blk_done_softirq()`에서 `blk_complete_reqs()` 호출
- 조건: 단일 hw 큐이거나, IPI로 다른 CPU에 위임하는 경우

**IPI Completion (프로세서간 인터럽트):**
- I/O를 제출한 CPU와 인터럽트가 발생한 CPU가 다를 때
- `QUEUE_FLAG_SAME_COMP` 플래그가 설정되면 제출 CPU에서 완료하려 시도
- 캐시 지역성(cache locality)을 높이기 위함

### 7.4 blk_mq_end_request() — 최종 완료

**위치:** `block/blk-mq.c` line 1392

```c
void blk_mq_end_request(struct request *rq, blk_status_t error)
{
    // 모든 바이트를 완료 처리한다
    // 내부에서 bio 체인을 순회하며 bio_endio() 호출
    if (blk_update_request(rq, error, blk_rq_bytes(rq)))
        BUG();  // 전체 완료인데 남은 데이터가 있으면 버그

    // 통계 업데이트 + tag 반환 + request 해제
    __blk_mq_end_request(rq, error);
}
```

**blk_update_request() 내부:**

```c
bool blk_update_request(struct request *req, blk_status_t error,
                        unsigned int nr_bytes)
{
    // 에러 로깅 (복구 불가능한 에러일 때)
    if (unlikely(error && !blk_rq_is_passthrough(req) && !quiet))
        blk_print_req_error(req, error);

    // I/O 완료 바이트 통계
    blk_account_io_completion(req, nr_bytes);

    // bio 체인 순회
    while (req->bio) {
        struct bio *bio = req->bio;
        unsigned bio_bytes = min(bio->bi_iter.bi_size, nr_bytes);

        if (unlikely(error))
            bio->bi_status = error;  // 에러 전파

        if (bio_bytes == bio->bi_iter.bi_size)
            req->bio = bio->bi_next;  // 이 bio는 완전히 완료

        bio_advance(bio, bio_bytes);

        if (!bio->bi_iter.bi_size) {
            // bio의 모든 바이트가 완료되었으면 완료 콜백 호출
            bio_endio(bio);
            // → bi_end_io() 호출
            // → 파일시스템/io_uring에 완료 통보
        }

        nr_bytes -= bio_bytes;
        if (!nr_bytes)
            break;
    }
    // ...
}
```

### 7.5 배치 완료: blk_mq_end_request_batch()

NVMe 인터럽트 핸들러에서 여러 CQE를 한 번에 처리할 때, `io_comp_batch`에 모아둔 request들을 한꺼번에 완료한다.

**위치:** `block/blk-mq.c` line 1430

```c
void blk_mq_end_request_batch(struct io_comp_batch *iob)
{
    int tags[TAG_COMP_BATCH], nr_tags = 0;  // TAG_COMP_BATCH = 32
    struct blk_mq_hw_ctx *cur_hctx = NULL;

    while ((rq = rq_list_pop(&iob->req_list)) != NULL) {
        prefetch(rq->bio);          // 다음 처리를 위해 미리 캐시에 로드

        blk_complete_request(rq);   // bio_endio() 호출
        __blk_mq_end_request_acct(rq, now);  // 통계 업데이트

        // tag를 배치로 모아서 한꺼번에 반환
        if (nr_tags == TAG_COMP_BATCH || cur_hctx != rq->mq_hctx) {
            if (cur_hctx)
                blk_mq_flush_tag_batch(cur_hctx, tags, nr_tags);
            nr_tags = 0;
            cur_hctx = rq->mq_hctx;
        }
        tags[nr_tags++] = rq->tag;
    }

    if (nr_tags)
        blk_mq_flush_tag_batch(cur_hctx, tags, nr_tags);
    // blk_mq_put_tags()로 여러 태그를 한번에 sbitmap에 반환
    // percpu_ref_put_many()로 q_usage_counter도 배치 감소
}
```

배치 완료의 이점:
- sbitmap 비트 clear 연산을 32개씩 모아서 처리하여 lock 횟수 감소
- percpu_ref 업데이트 횟수 감소
- 동일 hctx의 태그를 모아서 한꺼번에 반환

---

## 부록: 주요 함수 요약 테이블

| 함수 | 파일:라인 | 역할 |
|------|----------|------|
| `submit_bio()` | blk-core.c:1244 | 모든 블록 I/O의 최상위 진입점 |
| `submit_bio_noacct()` | blk-core.c:1064 | bio 유효성 검사 (파티션 리매핑, throttling 등) |
| `__submit_bio()` | blk-core.c:827 | 실제 디스패치 (plug 시작/종료 포함) |
| `blk_mq_submit_bio()` | blk-mq.c:3697 | ★ blk-mq 핵심: bio→request 변환, 디스패치 경로 선택 |
| `blk_mq_get_tag()` | blk-mq-tag.c:238 | 태그 할당 (sbitmap에서 비트 확보 = NVMe Command ID) |
| `blk_mq_bio_to_request()` | blk-mq.c:3157 | bio 정보를 request에 복사 |
| `blk_mq_try_issue_directly()` | blk-mq.c:3266 | 직접 디스패치 시도 (NVMe 최단 경로) |
| `__blk_mq_issue_directly()` | blk-mq.c:3194 | queue_rq() 콜백 호출 (NVMe: nvme_queue_rq) |
| `blk_mq_dispatch_rq_list()` | blk-mq.c:2502 | request 리스트를 드라이버에 디스패치 |
| `blk_add_rq_to_plug()` | blk-mq.c:1720 | plug 리스트에 request 추가 |
| `blk_mq_flush_plug_list()` | blk-mq.c:3489 | plug 리스트의 request 일괄 디스패치 |
| `blk_mq_insert_request()` | blk-mq.c:3079 | request를 적절한 큐에 삽입 |
| `blk_mq_run_hw_queue()` | blk-mq.c:2768 | 하드웨어 큐 실행 (대기 request 디스패치) |
| `blk_mq_start_request()` | blk-mq.c:1659 | request를 IN_FLIGHT로 전환 + 타임아웃 시작 |
| `blk_mq_complete_request()` | blk-mq.c:1626 | ★ I/O 완료 진입점 (NVMe CQE 처리 후) |
| `blk_mq_end_request()` | blk-mq.c:1392 | bio_endio() + tag 반환 + request 해제 |
| `blk_mq_end_request_batch()` | blk-mq.c:1430 | 배치 완료 (여러 request 한꺼번에 처리) |
| `blk_update_request()` | blk-mq.c:1135 | bio 체인 순회하며 bio_endio() 호출 |
| `blk_mq_put_tag()` | blk-mq-tag.c:379 | 태그 반환 (sbitmap 비트 clear) |
| `blk_mq_timeout_work()` | blk-mq.c:2075 | 타임아웃 워크 (NVMe: nvme_timeout 호출) |
| `blk_mq_attempt_bio_merge()` | blk-mq.c:3570 | bio 병합 시도 (plug + 스케줄러) |
| `blk_alloc_queue()` | blk-core.c:542 | request_queue 할당 및 초기화 |

---

## 부록: SPDK와의 비교

| 항목 | 커널 blk-mq | SPDK |
|------|------------|------|
| 큐 접근 | 커널 공간에서 blk-mq를 통해 | 유저 공간에서 VFIO로 직접 매핑 |
| 태그 관리 | sbitmap 기반 자동 관리 | 유저가 직접 Command ID 관리 |
| I/O 스케줄링 | none/mq-deadline/bfq/kyber 선택 | 없음 (애플리케이션이 직접) |
| 타임아웃 | 자동 (30초 기본) | 없음 (애플리케이션이 직접 구현) |
| 완료 알림 | 인터럽트 (IRQ) + softirq | 폴링 (polling) 전용 |
| Context switch | sleep/wakeup 가능 | CPU 점유 (busy polling) |
| 메모리 보호 | 커널 공간 격리 | 유저 공간 (IOMMU 의존) |
| 공유/격리 | 여러 프로세스가 안전하게 공유 | 하나의 프로세스가 디바이스 독점 |
| 오버헤드 | 시스템 콜 + 컨텍스트 전환 | 없음 (직접 접근) |
| 최소 레이턴시 | ~10μs (인터럽트 기반) | ~1-2μs (폴링 기반) |
