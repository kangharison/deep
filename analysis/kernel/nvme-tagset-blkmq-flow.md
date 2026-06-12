# NVMe IO tagset — 생성 동작과 blk-mq 연결 완성까지의 Flow

> 베이스: `linux-study/drivers/nvme/host/` + `linux-study/block/` (라인 번호는 본 트리 기준)
> 관련 문서: [[nvme-queue-mgmt-design]] — 큐 관리 설계 (§5 blk-mq 전파, §9 NS–SQ 바인딩)
> 구성: §1 `nvme_pci_alloc_tag_set` 분석 → §2 전체 3-Phase flow + block diagram → §3 Phase 2 심화

---

## 1. `nvme_pci_alloc_tag_set(dev)` — IO tagset 생성 동작

### 1.0 함수명에 대한 주의

`nvme_pci_alloc_tag_set()`은 **v6.1 시절의 함수명**이다 (구 `nvme_dev_add()`를 대체하며 도입). 이후 transport별 중복을 없애기 위해 **공용 함수 `nvme_alloc_io_tag_set()`(core.c:4938)으로 통합**되었고, 현재 트리에서 PCIe 드라이버는 이를 두 곳에서 호출한다. 하는 일은 동일하다 — *"컨트롤러의 IO 큐 전체를 대표하는 blk-mq tagset 하나를 만들어 등록한다."*

```c
/* pci.c:3777 (nvme_probe) / pci.c:3136 (nvme_pci_update_nr_queues) */
nvme_alloc_io_tag_set(&dev->ctrl, &dev->tagset, &nvme_mq_ops,
                      nvme_pci_nr_maps(dev), sizeof(struct nvme_iod));
```

(v6.1 원본 `nvme_pci_alloc_tag_set()`은 같은 내용을 pci.c에서 직접 수행했다. 현 공용 버전에는 fabrics 분기(connect_q 생성, core.c:4968~4978)가 추가됐지만 PCIe 경로에서는 타지 않으므로 동작 동일.)

### 1.1 호출 시점 — "NVMe 큐가 다 만들어진 뒤, 디스크가 생기기 전"

```
nvme_probe() (pci.c:3712)
  ├─ nvme_pci_enable()           ← q_depth 결정, ctrl.sqsize = q_depth - 1 (pci.c:3223)
  ├─ nvme_setup_io_queues()      ← 큐 개수 협상 + MSI-X + NVMe SQ/CQ 전부 생성
  ├─ if (dev->online_queues > 1)             ← IO 큐가 1개라도 있을 때만
  │     nvme_alloc_io_tag_set(...)   ★ 여기  ← blk-mq에 토폴로지를 "고정"하는 순간
  │     nvme_dbbuf_set(dev)
  └─ nvme_start_ctrl() → scan_work → NS마다 blk_mq_alloc_disk(ctrl->tagset, ...)
```

순서가 중요하다. **하드웨어 큐(NVMe SQ/CQ)가 먼저 존재해야** tagset을 만들 수 있다 — tagset의 `nr_hw_queues`가 그 시점의 `ctrl->queue_count`에서 계산되기 때문. 그리고 tagset이 있어야 namespace 디스크(core.c:4153)를 만들 수 있다. 즉 **tagset은 "NVMe 큐 세계"와 "블록 디바이스 세계"를 잇는 접점**이다.

두 번째 호출지 `nvme_pci_update_nr_queues()`(pci.c:3133)는 리셋 경로용: tagset이 아직 없으면 새로 만들고, 이미 있으면 `blk_mq_update_nr_hw_queues()`로 **개수만 갱신**한다 (tagset 재생성 아님).

### 1.2 인자 4개의 의미

| 인자 | 값 | 의미 |
|------|----|----|
| `set` | `&dev->tagset` | 컨트롤러당 1개. **모든 NS가 이걸 공유** (core.c:4153) |
| `ops` | `&nvme_mq_ops` (pci.c:2249) | blk-mq→드라이버 콜백 묶음: `queue_rq`, `queue_rqs`, `commit_rqs`, `complete`, `init_hctx`, `init_request`, `map_queues`, `timeout`, `poll` |
| `nr_maps` | `nvme_pci_nr_maps(dev)` (pci.c:3124) | CPU→hctx 매핑 테이블 개수. poll 큐 있으면 **3**(default/read/poll), read 큐만 있으면 **2**, 기본 **1**. `dev->io_queues[HCTX_TYPE_*]`는 `write_queues`/`poll_queues` 모듈 파라미터로 채워짐 (pci.c:2851~2881) |
| `cmd_size` | `sizeof(struct nvme_iod)` (pci.c:431) | request마다 덧붙는 드라이버 전용 PDU 크기 — NVMe 명령(`nvme_command`), DMA 디스크립터, 메타데이터 DMA 상태 등 |

### 1.3 본체 동작 — 필드를 채우고 `blk_mq_alloc_tag_set()` 호출 (core.c:4944~4964)

```c
memset(set, 0, sizeof(*set));
set->ops          = ops;
set->queue_depth  = min_t(unsigned, ctrl->sqsize, BLK_MQ_MAX_DEPTH - 1);  /* ① */
if (ctrl->quirks & NVME_QUIRK_SHARED_TAGS)
        set->reserved_tags = NVME_AQ_DEPTH;        /* ② Apple quirk: 32개 예약 */
else if (ctrl->ops->flags & NVME_F_FABRICS)
        set->reserved_tags = 1;                    /* ② fabrics connect용 — PCIe는 0 */
set->numa_node    = ctrl->numa_node;               /* ③ 태그/request 메모리 NUMA 배치 */
set->cmd_size     = cmd_size;                      /* ④ request + nvme_iod 한 덩어리 */
set->driver_data  = ctrl;
set->nr_hw_queues = ctrl->queue_count - 1;         /* ⑤ */
set->timeout      = NVME_IO_TIMEOUT;               /* ⑥ 기본 30초 → .timeout 콜백 */
set->nr_maps      = nr_maps;
ret = blk_mq_alloc_tag_set(set);                   /* ⑦ 실제 할당 — §3 심화 */
...
ctrl->tagset = set;                                /* ⑧ 등록 — 이후 NS 생성이 참조 */
```

**① `queue_depth` = 태그 수 = in-flight 상한.** `ctrl->sqsize`는 **0-based** 값이다 (`dev->q_depth - 1`, pci.c:3223). 그래서 태그 수가 SQ 슬롯 수보다 항상 1 작다 — SQ는 `sq_tail + 1 == cq_head`이면 full로 판정해야 해서 슬롯 하나를 비워둬야 하는데, 태그를 depth-1개만 발급하면 **blk-mq 차원에서 SQ 오버런이 구조적으로 불가능**해진다. 모든 IO 큐가 이 **단일 depth를 공유**한다 ([[nvme-queue-mgmt-design]] §5.1-(3)의 "균일 tag depth" 제약이 만들어지는 지점).

**⑤ `nr_hw_queues` = hctx 개수.** `ctrl->queue_count`는 `nvme_alloc_queue()`가 큐를 만들 때마다 증가시키는 카운터(pci.c:2127)로 **admin 큐를 포함**하므로 `-1`. 이 숫자가 곧 "blk-mq가 아는 IO 큐 개수"로 굳는다 — §5.1-(1)의 캐싱 지점.

**② `reserved_tags`:** 일반 태그 풀에서 떼어 특수 용도로 예약하는 수. PCIe 일반 컨트롤러는 0. Apple 컨트롤러는 admin/IO가 태그 공간을 공유하는 quirk 때문에 앞 32개(`NVME_AQ_DEPTH`)를 비워둔다.

### 1.4 호출 이후 — 이 tagset이 지배하는 것들

- `ctrl->tagset = set` 등록 → 스캔에서 **모든 NS가** `blk_mq_alloc_disk(ctrl->tagset, ...)`(core.c:4153)로 디스크를 만든다. NS별 큐 구분이 없는 근본 원인 (설계 문서 §9.1).
- quiesce/cancel이 **tagset 단위**로 동작: `nvme_quiesce_io_queues` = `blk_mq_quiesce_tagset(ctrl->tagset)` (core.c:5299), `nvme_cancel_tagset` (core.c:539). 한 NS만 멈출 수 없는 이유.
- `queue_depth`·`cmd_size`는 **여기서 고정** — 런타임 변경은 tagset 재할당이 필요하다 (§5.5). 갱신 가능한 것은 `nr_hw_queues`뿐 (`blk_mq_update_nr_hw_queues`).
- 직후 `nvme_dbbuf_set(dev)`: doorbell buffer config (가상화 환경에서 MMIO doorbell 대신 공유 메모리 사용) — tagset과는 독립적인 후속 작업.

### 1.5 한 줄 요약

> `nvme_pci_alloc_tag_set`(현 `nvme_alloc_io_tag_set`)은 **협상이 끝난 NVMe 큐 토폴로지(개수·depth)를 blk-mq의 정적 구조(태그 풀·request 풀·CPU 매핑)로 "복사해 고정"하는 함수**다. 이후 큐 토폴로지를 바꾸려면 이 고정을 `blk_mq_update_nr_hw_queues`로 다시 풀어야 하며, 여기서 굳는 세 가정(hctx 개수, hctx↔qid 선형 매핑, 균일 depth)이 큐 관리 설계(§5)가 싸우는 대상이다.

---

## 2. 전체 Flow — request_queue/hctx 완성까지

### 2.0 큰 그림: 3단계로 나뉜다

```
Phase 1. NVMe 큐 세계        nvme_setup_io_queues()       — 하드웨어에 SQ/CQ 실체 생성
Phase 2. tagset (설계도)      nvme_alloc_io_tag_set()      — 태그풀·request풀·CPU맵 고정
Phase 3. 블록 디바이스 세계    NS 스캔 → blk_mq_alloc_disk() — request_queue·ctx·hctx 실체화 + 바인딩
```

핵심: **hctx는 Phase 2(tagset 생성)가 아니라 Phase 3(NS 디스크 생성)에서 만들어진다.** tagset은 "hctx를 몇 개, 어떤 depth로, 어느 CPU에 매핑해서 만들 것인가"라는 설계도이고, NS가 생길 때마다 그 설계도대로 hctx 세트가 한 벌씩 찍혀 나온다.

### 2.1 Phase 1 — NVMe SQ/CQ 생성 (`nvme_setup_io_queues`, pci.c:2909)

```
nvme_setup_io_queues()
  ├─ nvme_set_queue_count()        // Set Features(0x07)로 컨트롤러와 큐 개수 협상
  ├─ nvme_setup_irqs()             // MSI-X 벡터 할당 + IRQ affinity 분배
  │     └─ dev->io_queues[HCTX_TYPE_DEFAULT/READ/POLL] 확정 (pci.c:2851~2881)
  │        ← write_queues/poll_queues 모듈 파라미터가 여기서 반영됨
  └─ nvme_create_io_queues()       // pci.c:3019
        ├─ nvme_alloc_queue(dev, i, dev->q_depth)     // pci.c:2103
        │     ├─ SQ/CQ DMA 버퍼 할당 (dma_alloc_coherent)
        │     ├─ nvmeq->q_db = doorbell 위치 계산 (pci.c:2125)
        │     └─ dev->ctrl.queue_count++              // pci.c:2127 ★ Phase 2의 nr_hw_queues 재료
        └─ nvme_create_queue(nvmeq, qid, polled)      // pci.c:2189
              ├─ adapter_alloc_cq()  → Create I/O CQ admin 명령 (pci.c:1719)
              ├─ adapter_alloc_sq()  → Create I/O SQ admin 명령 (pci.c:1742, cqid=qid 1:1)
              ├─ nvme_init_queue()   → cq_head=0, cq_phase=1 초기화
              └─ queue_request_irq() → MSI-X 핸들러 등록 (polled 큐 제외)
```

이 시점 결과물: `dev->queues[1..N]`이 모두 **하드웨어에 실재하는 큐**가 됨 (`NVMEQ_ENABLED`). 하지만 blk-mq는 아직 아무것도 모른다.

### 2.2 Phase 2 — tagset 생성 (`blk_mq_alloc_tag_set`, blk-mq.c:4864) — 요약

```
blk_mq_alloc_tag_set(set)                              // blk-mq.c:4864
  ├─ ① 검증/보정: depth, nr_maps, kdump 절삭 등
  ├─ ② set->tags = kcalloc(nr_hw_queues개 포인터 배열)   // blk-mq.c:4924
  ├─ ③ set->map[0..nr_maps-1].mq_map = CPU수 크기 배열   // blk-mq.c:4930~4937
  ├─ ④ blk_mq_update_queue_map(set)                     // blk-mq.c:4939
  │      └─ ops->map_queues = nvme_pci_map_queues()     // pci.c:681 — CPU→hctx 변환표 작성
  └─ ⑤ blk_mq_alloc_set_map_and_rqs(set)                // blk-mq.c:4750
       └─ hctx_idx마다 태그풀(sbitmap) + request풀(request+nvme_iod) 사전 할당
```

→ **상세 동작은 §3 심화 참조.**

이 시점 결과물: `dev->tagset.tags[0..N-1]` (hctx별 태그풀+request풀), `set->map[type].mq_map[cpu]` (CPU→hctx 변환표). **아직 hctx도 ctx도 request_queue도 없다.**

### 2.3 Phase 3 — NS 스캔: request_queue·ctx·hctx 실체화와 바인딩

`nvme_start_ctrl()` → scan_work가 Identify로 NSID를 열거하고, NS마다:

```
nvme_alloc_ns(ctrl, info)                               // core.c:4135
  └─ disk = blk_mq_alloc_disk(ctrl->tagset, &lim, ns)   // core.c:4153 ★ 공유 tagset 기반
       └─ __blk_mq_alloc_disk()                         // blk-mq.c:4490
            └─ blk_mq_alloc_queue(set, ...)             // blk-mq.c:4437
                 └─ blk_mq_init_allocated_queue(set, q) // blk-mq.c:4651 ★★ 조립 본체
                      ├─ q->mq_ops = set->ops; q->tag_set = set       // 4655, 4661
                      │
                      ├─ ⓐ blk_mq_alloc_ctxs(q)                       // 4663 (정의 4381)
                      │     → q->queue_ctx = per-CPU 할당
                      │       CPU마다 struct blk_mq_ctx 1개 (SW 큐: 플러그 리스트, hctxs[] 포인터)
                      │
                      ├─ ⓑ blk_mq_realloc_hw_ctxs(set, q)             // 4672 (정의 4639)
                      │     → hctx_idx = 0..nr_hw_queues-1 마다:
                      │       blk_mq_alloc_and_init_hctx()            // 4535
                      │         └─ blk_mq_init_hctx()                 // 4013
                      │              ├─ hctx->tags = set->tags[hctx_idx]   ← Phase 2 태그풀 연결
                      │              └─ ops->init_hctx(hctx, driver_data, hctx_idx)  // 4027
                      │                   = nvme_init_hctx (pci.c:655)
                      │                     └─ nvmeq = &dev->queues[hctx_idx + 1]   // pci.c:633
                      │                        hctx->driver_data = nvmeq            // pci.c:644
                      │                        ★★★ blk-mq 세계와 NVMe 큐 세계가 여기서 결합
                      │
                      ├─ q->nr_requests = set->queue_depth             // 4686
                      ├─ ⓒ blk_mq_init_cpu_queues(q, nr_hw_queues)    // 4689 (정의 4108)
                      │     → ctx마다 소속 CPU 번호·기본 hctx 연결 준비
                      ├─ ⓓ blk_mq_map_swqueue(q)                      // 4690 (정의 4192)
                      │     → for_each_possible_cpu(i):
                      │         hctx_idx = set->map[type].mq_map[i]    // Phase 2의 변환표 조회
                      │         ctx->hctxs[type] = q->queue_hw_ctx[hctx_idx]  // 4234
                      │         hctx->ctxs[hctx->nr_ctx++] = ctx       // 역방향 등록 (4246)
                      │       ★ "CPU→hctx" 매핑이 NS의 ctx에 실제로 박히는 순간
                      └─ ⓔ blk_mq_add_queue_tag_set(set, q)           // 4691 (정의 4359)
                            → q를 set->tag_list에 등록
                              ★ blk_mq_update_nr_hw_queues/quiesce_tagset이
                                "이 tagset의 모든 NS"를 순회할 수 있는 근거 (§5.7 함정 1)
```

NS가 2개면 이 블록 전체가 **2번** 실행된다 — request_queue·ctx·hctx는 NS마다 한 벌씩 생기고, tags와 nvme_queue만 공유된다.

### 2.4 Block Diagram — 전체 연결 관계

CPU 4개, IO 큐 4개(default 4), NS 2개 가정:

```
══════════════════ NS별 사본 (request_queue 단위) ═══════════════════╗
                                                                      ║
 /dev/nvme0n1 (gendisk)              /dev/nvme0n2 (gendisk)           ║
   │                                   │                              ║
 request_queue q1                    request_queue q2                 ║
   │ q->tag_set ────────┐              │ q->tag_set ───────┐          ║
   │                    │              │                   │          ║
   │ q->queue_ctx (per-CPU)            │ q->queue_ctx (per-CPU)       ║
   │ ┌──────┬──────┬──────┬──────┐     │ ┌──────┬──────┬──────┬──────┐║
   │ │ctx_0 │ctx_1 │ctx_2 │ctx_3 │     │ │ctx_0 │ctx_1 │ctx_2 │ctx_3 │║ ← SW큐
   │ └──┬───┴──┬───┴──┬───┴──┬───┘     │ └──┬───┴──┬───┴──┬───┴──┬───┘║   (CPU당 1개)
   │    │ctx->hctxs[type]              │    │ctx->hctxs[type]         ║
   │    ▼      ▼      ▼      ▼         │    ▼      ▼      ▼      ▼    ║
   │ ┌──────┬──────┬──────┬──────┐     │ ┌──────┬──────┬──────┬──────┐║
   └─│hctx_0│hctx_1│hctx_2│hctx_3│     └─│hctx_0│hctx_1│hctx_2│hctx_3│║ ← HW큐 문맥
     └─┬──┬─┴─┬──┬─┴─┬──┬─┴─┬──┬─┘       └─┬──┬─┴─┬──┬─┴─┬──┬─┴─┬──┬─┘║   (NS×큐수만큼!)
═══════│══│═══│══│═══│══│═══│══│═══════════│══│═══│══│═══│══│═══│══│══╝
       │.tags │  │   │  │   │  │           │  │   │  │   │  │   │  │
       │  │.driver_data │   │  │           │  │   │  │   │  │   │  │
══════ ▼══▼═══▼══▼═ 컨트롤러 공유 (tagset 단위, NS와 무관) ═▼══▼═══▼══▼══╗
                                                                       ║
  struct blk_mq_tag_set dev->tagset                                    ║
   ├─ map[DEFAULT].mq_map[cpu] = hctx_idx   ← nvme_pci_map_queues가 작성║
   ├─ tag_list = { q1, q2 }                 ← freeze가 전 NS를 멈추는 근거║
   └─ tags[0]    tags[1]    tags[2]    tags[3]                         ║
      (sbitmap   (sbitmap   (sbitmap   (sbitmap      ← 태그풀+request풀 ║
      +req풀)    +req풀)    +req풀)    +req풀)         n1·n2가 경쟁 공유 ║
        │           │          │          │                            ║
════════│═══════════│══════════│══════════│════════════════════════════╝
        ▼           ▼          ▼          ▼        (hctx_idx + 1 = qid)
══════ 드라이버 큐 세계 (struct nvme_queue, dev->queues[]) ═════════════╗
                                                                       ║
  queues[0]   queues[1]   queues[2]   queues[3]   queues[4]            ║
  (admin)     (IO qid1)   (IO qid2)   (IO qid3)   (IO qid4)            ║
   │            │ sq_cmds/cqes (DMA버퍼), q_db (doorbell), cq_vector    ║
═══│════════════│══════════════════════════════════════════════════════╝
   ▼            ▼            ▼           ▼           ▼
══ 하드웨어 (NVMe 컨트롤러) ════════════════════════════════════════════
  Admin SQ/CQ   SQ1→CQ1     SQ2→CQ2    SQ3→CQ3    SQ4→CQ4   (1:1 고정)
```

### 2.5 개수 관계 요약표

| 객체 | 단위 | 개수 (NS=2, IO큐=4, CPU=4) | 만들어지는 곳 |
|------|------|------|------|
| `nvme_queue` | 컨트롤러 | 5 (admin 1 + IO 4) | Phase 1, `nvme_alloc_queue` |
| `blk_mq_tag_set` | 컨트롤러 | 1 (`dev->tagset`) | Phase 2 |
| `blk_mq_tags` (태그풀) | tagset × hctx_idx | 4 | Phase 2, `blk_mq_alloc_rq_map` |
| `request` 풀 | tags당 depth개 | 4 × depth | Phase 2, `blk_mq_alloc_rqs` |
| `request_queue` | **NS마다** | 2 | Phase 3, `blk_mq_alloc_queue` |
| `blk_mq_ctx` (SW큐) | NS × CPU | 8 | Phase 3-ⓐ |
| `blk_mq_hw_ctx` | **NS × IO큐** | 8 | Phase 3-ⓑ |

### 2.6 헷갈리기 쉬운 포인트 2개

1. **hctx는 NS마다 따로다.** nvme0n1의 hctx_0과 nvme0n2의 hctx_0은 별개 객체다. 하지만 둘 다 `hctx->tags = set->tags[0]`, `hctx->driver_data = &dev->queues[1]`로 **같은 태그풀·같은 SQ를 가리킨다**. 그래서 n1과 n2의 IO는 같은 SQ에서 섞이고, 같은 태그풀을 두고 경쟁한다 — NS–SQ 바인딩([[nvme-queue-mgmt-design]] §9)이 공유 tagset 위에서 불가능한 구조적 이유.
2. **연결 정보의 원본은 tagset의 `mq_map`이고, ctx의 `hctxs[]`는 사본이다.** `blk_mq_update_nr_hw_queues`가 하는 "재매핑"이란 ⓑ~ⓓ(realloc_hw_ctxs → map_swqueue)를 freeze 하에 다시 실행해 이 사본들을 갱신하는 것이다 (§5.3).

### 2.7 검증: IO 한 건이 이 그림을 타고 내려가는 경로

```
write() → submit_bio
  → __bio_queue_enter(q)                    // q_usage_counter +1 (freeze 게이트)
  → ctx = per_cpu_ptr(q->queue_ctx, 현재CPU) // 내 CPU의 SW큐
  → hctx = ctx->hctxs[HCTX_TYPE_DEFAULT]    // Phase 3-ⓓ에서 박힌 사본
  → blk_mq_get_tag(hctx->tags)              // Phase 2의 sbitmap에서 태그 = request 획득
  → nvme_queue_rq(hctx, ...)                // pci.c:1405
       nvmeq = hctx->driver_data            // Phase 3-ⓑ에서 박힌 nvme_queue
  → SQE 작성(command_id = genctr<<12 | tag) → sq_tail++ → doorbell(nvmeq->q_db)
  → (완료) IRQ → nvme_handle_cqe → tags에서 tag로 request 복원 → bio 종료, ref −1
```

경로의 각 화살표가 위 다이어그램의 연결선 하나씩에 대응한다 — **ctx 선택은 CPU가, hctx 선택은 mq_map이, SQ 선택은 driver_data가 결정하며, NS는 어디에도 관여하지 않는다.**

---

## 3. Phase 2 심화 — `blk_mq_alloc_tag_set()` 내부 동작 (blk-mq.c:4864)

Phase 2는 5단계로 나뉜다. 각 단계가 만드는 산출물을 기준으로 따라간다.

### 3.1 ①검증·보정 단계 (blk-mq.c:4870~4907)

```c
if (!set->nr_hw_queues) return -EINVAL;                       // 큐 0개 금지
if (!set->queue_depth)  return -EINVAL;
if (set->queue_depth < set->reserved_tags + BLK_MQ_TAG_MIN)   // 예약분+최소(1)보다 작으면 거부
        return -EINVAL;
if (!set->ops->queue_rq) return -EINVAL;                      // 제출 콜백은 필수

if (set->queue_depth > BLK_MQ_MAX_DEPTH) {                    // 상한 10240 (blk-mq.h:714)
        pr_info("blk-mq: reduced tag depth to %u\n", ...);
        set->queue_depth = BLK_MQ_MAX_DEPTH;                  // 에러가 아니라 절삭
}
if (!set->nr_maps) set->nr_maps = 1;                          // 맵 최소 1개 (DEFAULT)
else if (set->nr_maps > HCTX_MAX_TYPES) return -EINVAL;       // 최대 3 (DEFAULT/READ/POLL)

if (is_kdump_kernel())                                        // crashdump 커널이면
        set->queue_depth = min(64U, set->queue_depth);        // 메모리 절약: 태그 64개로 제한

if (set->nr_maps == 1 && set->nr_hw_queues > nr_cpu_ids)      // 맵이 1개뿐이면
        set->nr_hw_queues = nr_cpu_ids;                       // CPU 수 초과 hctx는 무의미 → 절삭
```

주목: **depth/개수 위반은 대부분 에러가 아니라 "조용한 절삭"**이다. 드라이버가 요청한 값이 그대로 반영됐다고 가정하면 안 되고, 호출 후 `set->queue_depth`/`set->nr_hw_queues`를 다시 읽어야 정확하다. (NVMe는 sqsize ≤ MQES+1 ≤ 65536이라 10240 절삭에 걸릴 수 있다 — depth 4096 이상 컨트롤러에서 실제 발생 가능.)

마지막 줄의 의미: 맵이 1개(DEFAULT만)면 한 CPU가 두 hctx를 쓸 일이 없으므로 CPU 수보다 많은 hctx는 영원히 안 쓰인다. NVMe처럼 poll 맵이 있으면(nr_maps 3) 같은 CPU가 default용 hctx와 poll용 hctx를 **둘 다** 가질 수 있어 hctx 수 > CPU 수가 정상이다.

### 3.2 ②인프라 할당 (blk-mq.c:4909~4937)

```c
if (set->flags & BLK_MQ_F_BLOCKING) { set->srcu = ...; }      // 4909: .queue_rq가 sleep 가능한
                                                              // transport(NVMe-TCP 등)만. PCIe는 비할당
init_srcu_struct(&set->tags_srcu);                            // 4917: 태그풀 해제 시 RCU 유예용
init_rwsem(&set->update_nr_hwq_lock);                         // 4921: ★ update_nr_hw_queues가 잡는
                                                              //        rwsem (설계 문서 §5.3의 그 락)
set->tags = kcalloc_node(set->nr_hw_queues,                   // 4924: 태그풀 "포인터 배열"만
                         sizeof(struct blk_mq_tags *), ...);  //       (실체는 ⑤에서)
for (i = 0; i < set->nr_maps; i++)                            // 4930~4937
        set->map[i].mq_map = kcalloc_node(nr_cpu_ids, ...);   // 맵마다 CPU수 크기 변환표 그릇
```

여기까지는 그릇만 만든다. NUMA 주의점: `set->tags` 배열과 `mq_map`은 `set->numa_node`(= 컨트롤러가 꽂힌 노드)에 할당되지만, **개별 태그풀은 ⑤에서 "그 큐를 쓸 CPU의 노드"에 따로 배치**된다.

### 3.3 ④CPU→hctx 변환표 작성 — `blk_mq_update_queue_map` → `nvme_pci_map_queues` (pci.c:681)

산출물: `set->map[type].mq_map[cpu] = hctx_idx`. **이 표가 시스템의 모든 후속 매핑(Phase 3-ⓓ)의 원본**이다.

```c
static void nvme_pci_map_queues(struct blk_mq_tag_set *set)
{
        offset = queue_irq_offset(dev);          // pci.c:672 — MSI-X 벡터가 2개 이상이면 1
                                                 //   (벡터 0 = admin 전용이므로 IO는 1부터)
        for (i = 0, qoff = 0; i < set->nr_maps; i++) {
                map->nr_queues = dev->io_queues[i];   // Phase 1에서 확정한 타입별 큐 수
                map->queue_offset = qoff;             // 이 타입의 hctx 시작 번호
                if (i != HCTX_TYPE_POLL && offset)
                        blk_mq_map_hw_queues(map, dev->dev, offset);  // ⓐ IRQ affinity 복사
                else
                        blk_mq_map_queues(map);                       // ⓑ 균등 분배
                qoff   += map->nr_queues;        // 다음 타입은 hctx 번호 이어서 시작
                offset += map->nr_queues;        //   → poll 큐가 hctx 뒷번호에 배치되는 이유
        }
}
```

**ⓐ `blk_mq_map_hw_queues` (blk-mq-cpumap.c:108) — DEFAULT/READ 맵의 핵심:**

```c
for (queue = 0; queue < qmap->nr_queues; queue++) {
        mask = dev->bus->irq_get_affinity(dev, queue + offset);  // PCI 버스에서
                                                                 // 그 MSI-X 벡터의 affinity 마스크 조회
        for_each_cpu(cpu, mask)
                qmap->mq_map[cpu] = qmap->queue_offset + queue;  // 마스크의 CPU들 → 이 hctx
}
```

중요한 통찰: **blk-mq는 매핑을 "계산"하지 않는다. Phase 1의 `nvme_setup_irqs()`가 `pci_alloc_irq_vectors_affinity()`로 정해둔 MSI-X affinity를 그대로 복사한다.** 인과가 IRQ → 큐 매핑 방향이다. 이렇게 해야 "CPU X가 제출한 IO의 완료 인터럽트가 CPU X로 돌아온다"가 보장된다 — 완료 처리의 캐시 지역성 확보 + 크로스 CPU IPI 회피. (`irq_get_affinity` 콜백이 없는 버스면 ⓑ로 fallback, blk-mq-cpumap.c:115~116.)

**ⓑ `blk_mq_map_queues` (blk-mq-cpumap.c:59) — POLL 맵용:**

```c
masks = group_cpus_evenly(qmap->nr_queues, &nr_masks);  // CPU들을 큐 수만큼 균등 그룹화
                                                        //   (NUMA/패키지 토폴로지 고려)
for (queue = 0; queue < qmap->nr_queues; queue++)
        for_each_cpu(cpu, &masks[queue % nr_masks])
                qmap->mq_map[cpu] = qmap->queue_offset + queue;
```

poll 큐는 IRQ가 없으므로(affinity가 존재하지 않음) 토폴로지 기반 균등 분배만 한다.

**hctx 번호 배치 결과** (예: default 2, read 2, poll 2 → nr_hw_queues 6):

```
hctx_idx:    0        1        2       3       4       5
타입:        DEFAULT  DEFAULT  READ    READ    POLL    POLL
NVMe qid:    1        2        3       4       5       6     (qid = hctx_idx + 1)
IRQ 벡터:    1        2        3       4       (없음 — polled)
```

같은 CPU가 `map[DEFAULT]`에서는 hctx 0, `map[READ]`에서는 hctx 2, `map[POLL]`에서는 hctx 4를 가리킬 수 있다 — IO 종류에 따라 다른 SQ로 가는 메커니즘.

### 3.4 ⑤태그풀 + request풀 사전 할당 — `blk_mq_alloc_set_map_and_rqs` (blk-mq.c:4750)

**(1) 메모리 부족 시 depth 반감 재시도 루프** (blk-mq.c:4756~4766):

```c
depth = set->queue_depth;
do {
        err = __blk_mq_alloc_rq_maps(set);       // 전체 hctx의 태그+request 할당 시도
        if (!err) break;
        set->queue_depth >>= 1;                  // 실패 → depth 절반으로 재시도
        if (set->queue_depth < set->reserved_tags + BLK_MQ_TAG_MIN) { err = -ENOMEM; break; }
} while (set->queue_depth);
if (depth != set->queue_depth)
        pr_info("blk-mq: reduced tag depth (%u -> %u)\n", ...);   // 4774
```

depth × nr_hw_queues × rq_size는 수십 MB가 될 수 있어, 메모리 압박 시 **요청 depth의 1/2, 1/4...로 자동 후퇴**한다. dmesg에 "reduced tag depth"가 보이면 in-flight 상한이 조용히 줄어든 것 — 성능 디버깅 시 체크 포인트.

**(2) hctx_idx마다** `__blk_mq_alloc_map_and_rqs()` (blk-mq.c:4145) = 태그풀 + request풀:

```c
tags = blk_mq_alloc_rq_map(set, hctx_idx, depth, set->reserved_tags);  // 4145
ret  = blk_mq_alloc_rqs(set, tags, hctx_idx, depth);                   // 4149
```

**(3) 태그풀 — `blk_mq_alloc_rq_map` (blk-mq.c:3566):**

```c
int node = blk_mq_get_hctx_node(set, hctx_idx);   // ★ mq_map 역조회(blk-mq-cpumap.c:87)로
                                                  //   "이 큐를 쓸 CPU들의 NUMA 노드"를 찾음
tags = blk_mq_init_tags(nr_tags, reserved_tags, set->flags, node);  // blk-mq-tag.c:550
tags->rqs        = kcalloc_node(nr_tags, sizeof(struct request *), ...);  // 3581
tags->static_rqs = kcalloc_node(nr_tags, sizeof(struct request *), ...);  // 3587
```

`blk_mq_init_tags`(blk-mq-tag.c:550)가 만드는 `struct blk_mq_tags`(blk-mq.h:774):

| 필드 | 역할 |
|------|------|
| `bitmap_tags` (sbitmap_queue) | **일반 태그 할당자.** 캐시라인 단위로 쪼개진 비트맵 + per-CPU 할당 힌트(`alloc_hint`)로 CPU 간 같은 워드 경쟁 최소화 + 태그 고갈 시 잠들 대기열(`wait queue`) 내장 |
| `breserved_tags` | 예약 태그 전용 sbitmap (PCIe NVMe는 reserved 0 → 빈 풀) |
| `static_rqs[]` | **사전 할당된 request 원본.** `static_rqs[tag]`는 영구 고정 — 태그 번호가 곧 request 인덱스 |
| `rqs[]` | **현재 그 태그로 비행 중인 request 포인터.** IO 스케줄러 경유 시 static과 다른 객체일 수 있음. 완료 경로(`blk_mq_tag_to_rq`, blk-mq.h:794)와 `blk_mq_tagset_busy_iter`(cancel)가 보는 배열 |
| `page_list` | request풀이 들어있는 페이지 목록 (해제용) |

**(4) request풀 — `blk_mq_alloc_rqs` (blk-mq.c:3617):**

```c
rq_size = round_up(sizeof(struct request) + set->cmd_size,   // 3632
                   cache_line_size());
/* → [ struct request | struct nvme_iod ] 가 캐시라인 정렬된 한 덩어리.
 *   blk_mq_rq_to_pdu(rq) = (void *)rq + sizeof(*rq) 산술이 성립하는 근거 */

for (i = 0; i < depth; ) {
        /* order 4(64KB)부터 시도, 실패하면 order를 낮춰가며 페이지 청크 확보 (3637~3655)
         *  — depth가 커도 단일 거대 할당을 요구하지 않아 단편화에 강함 */
        page = alloc_pages_node(node, GFP_NOIO | __GFP_ZERO, this_order);
        page->private = this_order;               // 해제 시 order 기억
        list_add_tail(&page->lru, &tags->page_list);
        entries_per_page = order_to_size(this_order) / rq_size;   // 3669
        for (j = 0; j < to_do; j++) {
                tags->static_rqs[i] = rq;                          // 3675 태그번호=인덱스 고정
                blk_mq_init_request(set, rq, hctx_idx, node);      // 3676 (정의 3602)
                /*  └─ ops->init_request = nvme_pci_init_request (pci.c:661):
                 *       nvme_req(req)->ctrl = ctrl;
                 *       nvme_req(req)->cmd  = &iod->cmd;   ← SQE 버퍼를 iod 안에 영구 연결
                 *     이후 rq->state = MQ_RQ_IDLE (3613) */
                p += rq_size; i++;
        }
}
```

메모리 규모 감각 (hctx 1개당): `depth × rq_size` + 포인터 배열 2개(`depth × 8B × 2`) + sbitmap. rq_size는 `sizeof(struct request)`(수백 B) + `sizeof(struct nvme_iod)`(pci.c:431 — `nvme_command` 64B, 디스크립터 포인터 배열, DMA 상태 포함 수백 B)를 캐시라인으로 올림한 값. depth 1023, IO 큐 8개면 대략 수 MB~십수 MB 수준이 **부팅 시 한 번에** 잡힌다. **fast path(IO 제출)에서 request·SQE 메모리 할당이 0회**인 것은 전부 이 사전 할당 덕분이다.

**(5) 마무리** (blk-mq.c:4945~4946): `set->tag_list` 초기화 — Phase 3-ⓔ에서 NS들의 request_queue가 이 리스트에 매달린다.

### 3.5 Phase 2가 의도적으로 하지 않는 것

| 안 하는 것 | 하는 시점 |
|-----------|----------|
| hctx 생성 | Phase 3-ⓑ (`blk_mq_realloc_hw_ctxs`) — NS마다 |
| ctx(SW큐) 생성 | Phase 3-ⓐ (`blk_mq_alloc_ctxs`) — NS마다 |
| `ctx->hctxs[]` 매핑 적용 | Phase 3-ⓓ (`blk_mq_map_swqueue`) — mq_map의 "사본 배포" |
| hctx↔nvme_queue 결합 | Phase 3-ⓑ 안의 `.init_hctx` (pci.c:644) |
| NVMe SQ/CQ·doorbell·DMA | Phase 1에서 이미 완료 — Phase 2는 하드웨어를 전혀 건드리지 않음 |

### 3.6 Phase 2와 태그의 정체 — command_id와의 관계

Phase 2에서 만든 sbitmap 태그는 NVMe `command_id`의 하위 12비트가 된다 (genctr 인코딩, nvme.h:645 — [[nvme-queue-mgmt-design]] 부록 E.2):

```
command_id(16비트) = | genctr(4비트) | tag(12비트) |
                                       └─ Phase 2의 sbitmap이 발급
완료 시: CQE.command_id → tag 추출 → tags->rqs[tag] 로 request 복원 (blk-mq.h:794)
```

`BLK_MQ_UNIQUE_TAG_BITS = 16`(blk-mq.h:806), `BLK_MQ_MAX_DEPTH = 10240` — 태그가 12비트(4096)를 넘으면 genctr 인코딩과 충돌하므로 NVMe의 실효 depth 상한은 4096이다 (설계 문서 §3.1 정보 구조체의 `mqes` 항목 참조).

### 3.7 Phase 2 요약

> `blk_mq_alloc_tag_set()`은 (a) 드라이버 요청값을 **검증이 아니라 보정**하고(절삭·반감), (b) IRQ affinity를 **복사**해 CPU→hctx 변환표를 만들고(`mq_map`), (c) hctx별 태그풀(sbitmap)과 request+iod 덩어리 풀을 **NUMA-로컬로 사전 할당**한다. hctx라는 객체는 하나도 만들지 않는다 — Phase 3가 NS마다 이 설계도를 실체화한다.

---

## 4. Phase 3 심화 — NS 스캔에서 request_queue·ctx·hctx 완성까지

Phase 3는 두 세계에 걸쳐 있다: **NVMe 측**(스캔으로 NSID를 발견하고 NS 객체를 만드는 core.c)과 **blk-mq 측**(그 NS의 request_queue/ctx/hctx를 조립하는 blk-mq.c). 순서대로 따라간다.

### 4.0 진입 — 누가 Phase 3를 트리거하는가 (스캔 경로)

```
nvme_start_ctrl() (core.c:5012)                    // probe 마지막 단계
  └─ nvme_queue_scan(ctrl)
       └─ if (state == LIVE && ctrl->tagset)        // core.c:163 ★ tagset 존재가 선행 조건
            queue_work(nvme_wq, &ctrl->scan_work)   // core.c:164

nvme_scan_work() (core.c:4519)                      // nvme_wq 워크 — 프로세스 컨텍스트, sleep 가능
  └─ nvme_scan_ns_list() (core.c:4427)              // Identify(CNS 0x02) Active NSID List 순회
       └─ NSID마다 nvme_scan_ns() (core.c:4332)
            ├─ 이미 있는 NS → revalidate (크기/포맷 변경 반영)
            └─ 새 NSID → nvme_alloc_ns() (core.c:4135)   ★ Phase 3 본체
```

두 가지 함의:
- **Phase 2 → Phase 3 순서는 core.c:163의 `ctrl->tagset` 검사로 강제**된다. tagset 없이는 스캔 자체가 큐잉되지 않는다.
- AEN(Namespace Attribute Changed)도 같은 `scan_work`를 큐잉한다 → **런타임 attach/detach도 부팅 스캔과 완전히 같은 경로**를 탄다. `nvme attach-ns`로 NS를 붙이면 그 순간 아래 전체 조립이 실행된다.

### 4.1 NVMe 측 — `nvme_alloc_ns()` (core.c:4135) 단계별

```
nvme_alloc_ns(ctrl, info)
  ├─ ① ns = kzalloc_node(..., ctrl->numa_node)              // 4143 NS 객체
  ├─ ② disk = blk_mq_alloc_disk(ctrl->tagset, &lim, ns)     // 4153 ★ blk-mq 조립 (→ §4.2)
  │       실패 시 ns만 해제하고 조용히 return — NS 하나의 실패가
  │       컨트롤러 전체를 죽이지 않는다 (에러 격리)
  ├─ ③ disk->fops = &nvme_bdev_ops; ns->queue = disk->queue // 4156~4160
  ├─ ④ nvme_init_ns_head(ns, info)                          // 4164
  │       NGUID/EUI64/UUID로 subsystem 전역에서 ns_head 탐색/생성
  │       (멀티패스: 같은 NS를 보는 여러 ctrl 경로가 head 하나를 공유)
  ├─ ⑤ disk_name 결정                                        // 4178~4188
  │       멀티패스 멤버: "nvme0c0n1" + GENHD_FL_HIDDEN (head 디스크 뒤로 숨음)
  │       일반:        "nvme0n1" (ctrl->instance + head->instance)
  ├─ ⑥ nvme_update_ns_info(ns, info)                        // 4190
  │       Identify NS 결과 → capacity, LBA 크기, queue_limits 적용
  ├─ ⑦ ctrl->namespaces 리스트 등록                          // 4193~4204
  │       namespaces_lock + NVME_CTRL_FROZEN 검사(4198):
  │       리셋이 큐를 freeze한 뒤에는 새 NS 등록 거부 → 스캔↔리셋 데드락 회피
  │       list 등록 후 synchronize_srcu(4204) — srcu 리더(설계 문서 §9.1)와의 정합
  ├─ ⑧ device_add_disk()                                    // 4207
  │       ★ /dev/nvme0n1 이 유저스페이스에 노출되는 순간 (udev 이벤트, 파티션 스캔)
  └─ ⑨ nvme_add_ns_cdev(ns)                                 // 4211
          /dev/ng0n1 캐릭터 디바이스 — io_uring passthrough 진입점 (ioctl.c:673)
```

실패 시 `out_*` 라벨 체인이 **등록의 정확한 역순**으로 해제한다 (4218~4247) — list_del_rcu + synchronize_srcu → ns_head 참조 반납 → put_disk → kfree.

### 4.2 `blk_mq_alloc_disk` → `blk_mq_alloc_queue` (blk-mq.c:4437)

```c
lim->features |= BLK_FEAT_IO_STAT | BLK_FEAT_NOWAIT;       // 4446
if (set->nr_maps > HCTX_TYPE_POLL)                          // 4447 — poll 맵이 있으면
        lim->features |= BLK_FEAT_POLL;                     //   이 NS는 IOPOLL 지원으로 표시
q = blk_alloc_queue(lim, set->numa_node);                   // 4450 — q 골격 + q_usage_counter
q->queuedata = queuedata;                                   // 4453 — ns 포인터 (완료 경로에서 회수)
ret = blk_mq_init_allocated_queue(set, q);                  // 4454 ★ 조립 본체 (§4.3)
```

- Phase 2의 `nr_maps`(=poll 큐 유무)가 **NS의 기능 플래그(`BLK_FEAT_POLL`)로 전파**되는 지점. `poll_queues=0`이면 모든 NS에서 io_uring IOPOLL이 불가능해지는 인과가 여기다.
- `blk_alloc_queue`가 만드는 `q_usage_counter`(percpu_ref)가 §5.7 freeze 게이트의 그 카운터다 — request_queue가 생기는 순간부터 freeze 가능.
- `__blk_mq_alloc_disk`(4490)는 q 위에 gendisk 껍데기(`__alloc_disk_node`, 4501)를 씌울 뿐이다.

### 4.3 `blk_mq_init_allocated_queue` (blk-mq.c:4651) — 5개 서브스텝 내부

#### ⓐ `blk_mq_alloc_ctxs` (4381) — per-CPU SW큐 생성

```c
ctxs->queue_ctx = alloc_percpu(struct blk_mq_ctx);          // 4390 — CPU마다 1개, 캐시라인 정렬
```

`struct blk_mq_ctx`(block/blk-mq.h:19)의 실체:

```c
struct blk_mq_ctx {
        struct {
                spinlock_t       lock;
                struct list_head rq_lists[HCTX_MAX_TYPES];  // ★ 타입별(DEFAULT/READ/POLL) 적재 리스트
        } ____cacheline_aligned_in_smp;
        unsigned int     cpu;                               // 소속 CPU 번호
        unsigned short   index_hw[HCTX_MAX_TYPES];          // hctx->ctxs[] 안에서 내 위치 (역참조)
        struct blk_mq_hw_ctx *hctxs[HCTX_MAX_TYPES];        // ★ 타입별 담당 hctx — 제출 fast path가 읽는 곳
        struct request_queue *queue;
};
```

`rq_lists`가 **타입별 3개**인 이유: 같은 CPU에서 나온 IO라도 read/poll 여부에 따라 다른 hctx로 가므로, 적재 단계부터 분리해야 dispatch 시 섞이지 않는다. per-CPU 구조라 제출 적재에 CPU 간 락 경합이 없다.

#### ⓑ `blk_mq_realloc_hw_ctxs` (4639 → 4569) — hctx 실체화

```c
/* (1) q->queue_hw_ctx 포인터 배열 확보 — RCU 교체 (4578~4592) */
new_hctxs = kcalloc_node(set->nr_hw_queues, ...);
rcu_assign_pointer(q->queue_hw_ctx, new_hctxs);
kfree_rcu_mightsleep(hctxs);     // 옛 배열을 읽는 중인 컨텍스트의 UAF 방지
                                 // (첫 생성 때는 NULL이라 그냥 새 배열)

/* (2) hctx_idx = 0..nr_hw_queues-1 마다 (4595~4615) */
int node = blk_mq_get_hctx_node(set, i);      // ★ mq_map 역조회 — hctx 메모리도
                                              //   "그 큐를 쓸 CPU의 NUMA 노드"에 배치 (Phase 2와 동일 원리)
hctxs[i] = blk_mq_alloc_and_init_hctx(set, q, i, node);   // 4535
```

`blk_mq_alloc_and_init_hctx`(4535)는 2단 구성:

**1단 — 객체 확보**: 먼저 `q->unused_hctx_list`에서 같은 노드의 죽은 hctx 재사용 시도(4542~4551 — `update_nr_hw_queues` 반복 시 할당 비용 절감), 없으면 `blk_mq_alloc_hctx`(4048)로 신규 할당:

| 필드 (4048~4094) | 역할 |
|------|------|
| `cpumask` | 이 hctx를 쓰는 CPU 집합 (ⓓ에서 채움) |
| `run_work` | 비동기 dispatch 워커 (`blk_mq_run_work_fn`) |
| `dispatch` + `lock` | 디스패치 실패분 보관 리스트 (BLK_STS_RESOURCE 재시도용) |
| `ctxs[]` | 매핑된 ctx 역참조 배열 — **nr_cpu_ids 크기로 선할당**(4080, 런타임 재할당 회피) |
| `ctx_map` (sbitmap) | "어느 ctx에 일감이 쌓였나" 비트맵(4085) — dispatch가 빈 ctx를 안 뒤지게 |
| `dispatch_wait` | 태그 고갈 시 잠들 대기 엔트리(4091) |

**2단 — `blk_mq_init_hctx`(4013)**: 자원 연결 3종.

```c
hctx->fq = blk_alloc_flush_queue(hctx->numa_node, set->cmd_size, gfp);  // 4019
/* ★ flush/FUA 전용 request가 hctx마다 1개 별도 존재. cmd_size 포함이므로
 *   flush_rq에도 nvme_iod가 붙는다. 일반 태그풀 밖의 예약 request —
 *   태그 고갈 상태에서도 flush는 진행 가능해야 하므로 */
hctx->tags = set->tags[hctx_idx];                                       // 4025 ★ Phase 2 태그풀 연결
set->ops->init_hctx(hctx, set->driver_data, hctx_idx);                  // 4027 → nvme_init_hctx
blk_mq_init_request(set, hctx->fq->flush_rq, hctx_idx, ...);            // 4031 flush_rq도 iod 초기화
```

NVMe 콜백 `nvme_init_hctx`(pci.c:655) → `nvme_init_hctx_common`(pci.c:629):

```c
struct nvme_queue *nvmeq = &dev->queues[qid];        // pci.c:633  qid = hctx_idx + 1 (선형 가정)
tags = qid ? dev->tagset.tags[qid - 1] : ...;        // pci.c:637
WARN_ON(tags != hctx->tags);                         // pci.c:638  선형 가정 정합성 자가검증
pools = nvme_setup_descriptor_pools(dev, hctx->numa_node);  // pci.c:639 PRP/SGL dma_pool
hctx->driver_data = nvmeq;                           // pci.c:644 ★★★ 두 세계의 결합점
```

**실패 시 폴백 사다리**(4606~4626): 새 노드 할당 실패 → 옛 노드로 재시도(4611) → 그래도 실패면 만들어진 데까지만 `q->nr_hw_queues`로 유지(4620~4626). hctx 생성 실패는 치명상이 아니라 **축소 운영**으로 흡수된다. 마지막으로 cpuhp(CPU hotplug) 콜백 등록(4644~4648) — CPU가 꺼질 때 그 hctx의 잔여 작업을 다른 CPU로 옮기는 핸들러.

#### ⓒ `blk_mq_init_cpu_queues` (4108) — ctx 초기화 + hctx 노드 보정

```c
for_each_possible_cpu(i) {
        __ctx->cpu = i;  rq_lists 초기화;  __ctx->queue = q;     // 4119~4124
        for (j = 0; j < set->nr_maps; j++) {                     // 4130~4134
                hctx = blk_mq_map_queue_type(q, j, i);
                if (nr_hw_queues > 1 && hctx->numa_node == NUMA_NO_NODE)
                        hctx->numa_node = cpu_to_node(i);        // 첫 매핑 CPU의 노드로 보정
        }
}
```

#### ⓓ `blk_mq_map_swqueue` (4192) — mq_map "사본 배포" (3라운드)

**R1 — 리셋**(4200~4204): 전 hctx의 `cpumask`/`nr_ctx` 클리어. 재진입(remap) 대비 멱등성 확보.

**R2 — 정방향+역방향 배선**(4211~4258): `for_each_possible_cpu × nr_maps`:

```c
hctx_idx = set->map[j].mq_map[i];                    // 4220 Phase 2 변환표 조회
ctx->hctxs[j] = blk_mq_map_queue_type(q, j, i);      // 4234 ★ 정방향 — 제출 경로가 읽는 포인터
cpumask_set_cpu(i, hctx->cpumask);                   // 4243 역방향 ① 이 hctx의 CPU 집합
ctx->index_hw[hctx->type] = hctx->nr_ctx;            // 4245 역방향 ② ctx_map 비트 위치
hctx->ctxs[hctx->nr_ctx++] = ctx;                    // 4246 역방향 ③ dispatch 순회용
```

엣지 케이스 두 개가 중요하다:
- `map[j].nr_queues == 0`이면 그 타입은 **DEFAULT hctx로 폴백**(4215~4218) — read 맵이 비면 read IO도 default 큐로 간다.
- 태그 없는 hctx를 만나면 lazy 할당 시도, 실패 시 `mq_map[i] = 0`으로 **hctx 0에 강제 폴백**(4222~4231). 그래서 hctx 0의 태그는 절대 회수하지 않는다(4268~4271 주석 "Never unmap queue 0").

**R3 — 마무리**(4260~4307): `nr_ctx == 0`인 hctx(매핑된 CPU가 없음)는 태그 반납 후 비활성(4267~4277), `sbitmap_resize(&hctx->ctx_map, nr_ctx)`(4287 — 매핑 수만큼만), isolated CPU를 cpumask에서 제거(4295~4300, nohz_full 환경 보호), 라운드로빈 시작점 `next_cpu` 초기화(4305).

#### ⓔ `blk_mq_add_queue_tag_set` (4359) — tagset 가입과 **shared 전환**

```c
if (!list_empty(&set->tag_list) &&                   // 4367 ★ 이미 다른 NS가 등록돼 있으면
    !(set->flags & BLK_MQ_F_TAG_QUEUE_SHARED)) {
        set->flags |= BLK_MQ_F_TAG_QUEUE_SHARED;     // "태그 공유 모드" 진입
        blk_mq_update_tag_set_shared(set, true);     // 기존 NS들의 hctx에도 소급 적용
}
list_add_tail_rcu(&q->tag_set_list, &set->tag_list); // 4375 tagset의 NS 목록에 합류
```

**두 번째 NS가 등록되는 순간이 분수령**이다. `TAG_QUEUE_SHARED`가 켜지면 태그 할당이 `active_queues` 기반 공정 분배 모드로 바뀐다 — hctx당 태그를 통째로 쓰지 못하고 활성 큐 수로 나눈 몫(`hctx_may_queue`)까지만 가져간다. 즉:
- NS 1개: 태그 1023개를 혼자 씀 (검사 오버헤드도 없음)
- NS 2개 (둘 다 활성): 각자 ~511개 상한 — **NS를 붙이기만 해도 기존 NS의 실효 depth가 줄 수 있다.**
이것이 공유 tagset 모델에서 NS 간 간섭의 공식 메커니즘이고, [[nvme-queue-mgmt-design]] §9.6 per-NS tagset 분리가 해소하는 대상 중 하나다.

### 4.4 타임라인 요약 — NS 1개가 온라인되기까지

```
scan_work(프로세스 컨텍스트)
   │ Identify NSID List → 새 NSID 발견
   ▼
nvme_alloc_ns ──② blk_mq_alloc_disk ──> blk_mq_init_allocated_queue
   │                                       ├ ⓐ ctx percpu 할당      (그릇)
   │                                       ├ ⓑ hctx 할당 + init_hctx (★ nvmeq 결합)
   │                                       ├ ⓒ ctx 초기화/노드 보정
   │                                       ├ ⓓ mq_map → ctx->hctxs 배선 (★ 사본 배포)
   │                                       └ ⓔ tag_list 가입        (★ shared 전환 판정)
   ├─④ ns_head 연결(멀티패스 정체성)
   ├─⑥ Identify NS → capacity/limits
   ├─⑦ ctrl->namespaces 등록 (srcu)
   ├─⑧ device_add_disk  ──→ /dev/nvme0n1 노출, udev, 파티션 스캔
   └─⑨ cdev 등록        ──→ /dev/ng0n1 (uring passthrough)
```

이 순서의 의미: **blk-mq 배선(ⓐ~ⓔ)이 끝난 뒤에야 디스크가 노출**되므로, 유저스페이스의 첫 IO가 도착하는 시점에는 ctx→hctx→nvmeq 경로가 항상 완성돼 있다. 반쯤 조립된 큐로 IO가 들어올 창이 구조적으로 없다.

### 4.5 Phase 3 종료 시점의 불변식 (invariants)

| 불변식 | 성립 근거 | 깨지는 조건 |
|--------|----------|------------|
| 모든 NS의 hctx_i ↔ `dev->queues[i+1]` 동일 결합 | `nvme_init_hctx`의 `hctx_idx+1` (pci.c:658) | 설계 문서 §5.6 indirection 도입 시 |
| `q->nr_requests == set->queue_depth` (전 NS 동일) | blk-mq.c:4686 | tagset 재할당 없이는 불변 |
| NS ≥ 2 → `TAG_QUEUE_SHARED` (태그 공정 분배) | blk-mq.c:4367~4374 | NS가 1개로 줄어도 해제됨 |
| hctx마다 flush_rq 1개 (태그풀 밖, iod 포함) | blk-mq.c:4019/4031 | — |
| hctx 0은 항상 태그 보유 (폴백 보장) | blk-mq.c:4268~4273 | — |
| ctx의 `hctxs[]`는 mq_map의 사본 | blk-mq.c:4234 | remap(ⓑ~ⓓ 재실행) 전까지 |

`blk_mq_update_nr_hw_queues`(설계 문서 §5.3)가 하는 "재매핑"의 정체가 이제 정확해진다: **freeze 하에서 tag_list의 모든 NS에 대해 ⓑ(realloc_hw_ctxs)~ⓓ(map_swqueue)를 다시 실행해 위 사본들을 새 토폴로지로 갱신하는 것**이다.

### 4.6 Phase 3 요약

> Phase 3는 스캔이 NSID를 발견할 때마다 (a) NVMe 측에서 NS 객체·ns_head·디바이스 노드를 만들고, (b) blk-mq 측에서 tagset 설계도를 실체화한다 — per-CPU ctx를 깔고, hctx를 NUMA-로컬로 만들어 `init_hctx`로 `nvme_queue`에 결합하고, mq_map을 ctx에 사본 배포하고, tag_list에 가입시킨다. **두 번째 NS부터는 태그 공유 모드가 켜져 NS 간 태그 경쟁이 시작**되며, 모든 NS가 같은 hctx_idx→qid 선형 결합을 복제하므로 NS와 SQ의 분리는 이 구조 안에서는 발생할 수 없다.
