# NVMe IO/Admin Queue 초기화 — request_queue · tagset · hctx 생성과 연결 구조

> 베이스: `linux-study/drivers/nvme/host/` + `linux-study/block/`
> 라인 번호는 본 트리(`linux-study/`) 기준.
> 관련 문서: [[nvme-tagset-blkmq-flow]] · [[nvme-queue-mgmt-design]] · [[io-lifecycle-no-scheduler]]

---

## 0. 핵심 자료구조 정의 — 용어 정리

분석 전, 이 문서에서 다루는 자료구조의 위치와 필드를 먼저 확정한다.

| 구조체 | 정의 위치 | 핵심 연결 필드 |
|---|---|---|
| `struct nvme_dev` | `pci.c:294` | `.ctrl`(315), `.queues[]`(295), `.tagset`(296, IO), `.admin_tagset`(297) |
| `struct nvme_queue` | `pci.c:365` | `.sq_cmds`(369), `.cqes`(372), `.sq_dma_addr`(373), `.cq_dma_addr`(374), `.q_db`(375, doorbell 레지스터), `.qid`(381) |
| `struct nvme_ctrl` | `nvme.h:334` | `.admin_q`(342), `.connect_q`(343), `.tagset`(348), `.admin_tagset`(349), `.queue_count`(372), `.sqsize`(387) |
| `struct blk_mq_tag_set` | `include/linux/blk-mq.h:534` | `.ops`(535), `.map[HCTX_MAX_TYPES]`(536), `.nr_maps`(537), `.nr_hw_queues`(538), `.queue_depth`(539), `.cmd_size`(541), `.tags`(547, `blk_mq_tags**`) |
| `struct blk_mq_hw_ctx` | `include/linux/blk-mq.h:322` | `.driver_data`(377), `.ctx_map`(383), `.tags`(422), `.queue_num`(433), `.ctxs[]`(402), `.type`(398) |
| `struct blk_mq_tags` | `include/linux/blk-mq.h:774` | `.nr_tags`(775), `.bitmap_tags`(779, `sbitmap_queue`), `.breserved_tags`(780), `.rqs[]`(782), `.static_rqs[]`(783) |
| `struct blk_mq_ctx` (SW queue) | `block/blk-mq.h:19` | `.rq_lists[HCTX_MAX_TYPES]`, `.cpu`, `.index_hw[]`, `.hctxs[HCTX_MAX_TYPES]` |

`HCTX_MAX_TYPES` = 3: `HCTX_TYPE_DEFAULT` / `HCTX_TYPE_READ` / `HCTX_TYPE_POLL`  
(`include/linux/blk-mq.h:488`)

---

## 1. 전체 초기화 순서 개요

```
nvme_probe() (pci.c:3712)
 │
 ├─ [1] nvme_pci_alloc_dev() + nvme_init_ctrl()     ← dev/ctrl 할당
 │       dev->queues = kcalloc(...)                 ← nvme_queue 배열 선할당 (SQ/CQ ring용)
 │
 ├─ [2] nvme_pci_enable()
 │       nvme_pci_configure_admin_queue()           ← Admin NVMe SQ/CQ ring 생성 (qid=0)
 │       BAR 레지스터(AQA/ASQ/ACQ) 기록             ← 디바이스에 admin 큐 위치 알림
 │
 ├─ [3] nvme_alloc_admin_tag_set()                  ← Admin blk_mq_tag_set 할당
 │       blk_mq_alloc_queue() → ctrl->admin_q       ← Admin request_queue 생성
 │
 ├─ [4] nvme_init_ctrl_finish()                     ← admin_q로 Identify·Set Features 전송
 │       nvme_setup_io_queues()
 │         nvme_set_queue_count()                   ← 디바이스와 IO 큐 수 협상 (admin 명령)
 │         nvme_setup_irqs()                        ← MSI-X 인터럽트 설정
 │         nvme_create_io_queues()                  ← IO NVMe SQ/CQ ring 생성 (qid=1..N)
 │
 ├─ [5] nvme_alloc_io_tag_set()                     ← IO blk_mq_tag_set 할당
 │
 └─ [6] nvme_scan_work → nvme_alloc_ns()
         blk_mq_alloc_disk()                        ← NS별 request_queue 생성
         (모든 NS가 동일 IO tagset 공유)
```

핵심 순서 규칙:
- **NVMe SQ/CQ ring(nvme_queue) 은 tagset 보다 먼저 만들어진다.** tagset의 `nr_hw_queues`는 `online_queues`로 결정되므로, 물리 큐가 먼저 존재해야 한다.
- **Admin 큐는 IO 큐 협상의 전제 조건이다.** IO 큐 수를 협상하는 `Set Features` 명령 자체가 admin 큐를 통해 전송된다.

---

## 2. Admin Queue 생성 경로 — 상세

### 2.1 Admin NVMe SQ/CQ ring 생성 (`pci.c:2300`)

```
nvme_pci_enable()
 └─ nvme_pci_configure_admin_queue(dev)               pci.c:2300
      ├─ nvme_alloc_queue(dev, 0, NVME_AQ_DEPTH)      pci.c:2348
      │    ├─ nvmeq = &dev->queues[0]                 pci.c:2103
      │    ├─ nvmeq->sq_cmds = dma_alloc_coherent()   pci.c:2112  ← SQ ring (64B×AQ_DEPTH)
      │    ├─ nvmeq->cqes    = dma_alloc_coherent()   pci.c:2117  ← CQ ring
      │    ├─ nvmeq->qid     = 0                      pci.c:2124  ← qid 0 = admin
      │    └─ ctrl->queue_count = 1                   pci.c:2127
      │
      └─ BAR 레지스터 기록                             pci.c:2358~2360
           writel(AQA, dev->bar + NVME_REG_AQA)       ← 큐 깊이 설정
           writeq(ASQ, dev->bar + NVME_REG_ASQ)       ← SQ 물리 주소
           writeq(ACQ, dev->bar + NVME_REG_ACQ)       ← CQ 물리 주소
```

이 시점에서 `dev->queues[0]`에 DMA-coherent SQ/CQ ring이 존재하고,
디바이스는 해당 물리 주소를 알고 있다. blk-mq는 아직 개입하지 않았다.

### 2.2 Admin blk_mq_tag_set + request_queue 생성 (`core.c:4865`)

```
nvme_alloc_admin_tag_set(ctrl, &dev->admin_tagset,
                         &nvme_mq_admin_ops,
                         sizeof(struct nvme_iod))      core.c:4865

  set->ops          = nvme_mq_admin_ops               core.c:4871
  set->queue_depth  = NVME_AQ_MQ_TAG_DEPTH (= 30)    core.c:4872
  set->numa_node    = ctrl->numa_node                  core.c:4876
  set->cmd_size     = sizeof(struct nvme_iod)          core.c:4879
  set->driver_data  = ctrl                             core.c:4880
  set->nr_hw_queues = 1                                core.c:4881  ← admin은 단일 HW 큐
  set->timeout      = NVME_ADMIN_TIMEOUT               core.c:4882
  set->reserved_tags = 2 (fabrics) 또는 0 (PCIe)      core.c:4873~4875

  blk_mq_alloc_tag_set(set)                            core.c:4883
   └─ (§4에서 상세 설명)

  ctrl->admin_q = blk_mq_alloc_queue(set, NULL, NULL) core.c:4894
   └─ blk_mq_init_allocated_queue(set, q)             blk-mq.c:4651
        → request_queue 완성 (§4.3 참조)

  ctrl->admin_tagset = set                             core.c:4908
```

### 2.3 Admin hctx 초기화 — `nvme_mq_admin_ops` 연결

`nvme_mq_admin_ops` 구조체 (`pci.c:2241`):

```c
.queue_rq     = nvme_queue_rq         // pci.c:1405 — NVMe 명령 SQ ring에 삽입
.complete     = nvme_pci_complete_rq  // 완료 콜백
.init_hctx    = nvme_admin_init_hctx  // pci.c:648 — hctx↔nvmeq 연결
.init_request = nvme_pci_init_request // 요청 초기화
.timeout      = nvme_timeout
// queue_rqs / commit_rqs / map_queues / poll 없음
// — admin은 단일 큐이므로 배치·폴링·CPU 매핑 불필요
```

hctx 초기화 체인 (`blk-mq.c:4013` → `pci.c:648`):

```
blk_mq_init_hctx()                          blk-mq.c:4013
 ├─ hctx->tags      = set->tags[0]          blk-mq.c:4025
 ├─ hctx->queue_num = 0                     blk-mq.c:4023
 └─ ops->init_hctx(hctx, driver_data, 0)   blk-mq.c:4027
      → nvme_admin_init_hctx()              pci.c:648
           → nvme_init_hctx_common(hctx, data, 0)  pci.c:644
                hctx->driver_data = &dev->queues[0]
                // admin hctx[0] ↔ NVMe qid 0 (Admin SQ/CQ ring)
```

---

## 3. IO Queue 생성 경로 — 상세

### 3.1 IO NVMe SQ/CQ ring 생성 (`pci.c:2909`)

```
nvme_setup_io_queues(dev)                              pci.c:2909
 ├─ nvme_set_queue_count(&dev->ctrl, &nr_io_queues)   pci.c:2943
 │    → admin_q로 "Set Features (Number of Queues)" 전송
 │    → 디바이스가 지원 가능한 큐 수를 반환
 │
 ├─ nvme_setup_irqs(dev, nr_io_queues, ...)            pci.c:2997~3005
 │    → pci_alloc_irq_vectors_affinity()으로 MSI-X 인터럽트 할당
 │    → dev->max_qid 확정
 │
 └─ nvme_create_io_queues(dev)                         pci.c:3019 → pci.c:2378
      for i = (online_queues) .. (max_qid):
       ├─ nvme_alloc_queue(dev, i, q_depth)            pci.c:2384
       │    → dev->queues[i] SQ/CQ ring dma_alloc_coherent
       │    → ctrl->queue_count++
       └─ nvme_create_queue(nvmeq, i, polled)          pci.c:2401 → pci.c:2189
            ├─ adapter_alloc_cq()                      pci.c:2206  ← Create CQ admin 명령
            ├─ adapter_alloc_sq()                      pci.c:2210  ← Create SQ admin 명령
            ├─ nvme_init_queue()                       pci.c:2221  ← online_queues++
            └─ queue_request_irq()                     pci.c:2223  ← MSI-X 연결
```

### 3.2 IO blk_mq_tag_set 생성 (`core.c:4938`)

```
if (dev->online_queues > 1):                           pci.c:3776
  nvme_alloc_io_tag_set(&dev->ctrl, &dev->tagset,
                        &nvme_mq_ops,
                        nvme_pci_nr_maps(dev),
                        sizeof(struct nvme_iod))        pci.c:3777 → core.c:4938

  set->ops          = nvme_mq_ops                      core.c:4945
  set->queue_depth  = min(sqsize, BLK_MQ_MAX_DEPTH-1) core.c:4946
  set->reserved_tags = 0 (PCIe) / 32 (SHARED_TAGS)    core.c:4951~4952
  set->numa_node    = ctrl->numa_node                  core.c:4956
  set->flags        = BLK_MQ_F_SHOULD_MERGE | ...      core.c:4957
  set->cmd_size     = sizeof(struct nvme_iod)          core.c:4959
  set->driver_data  = ctrl                             core.c:4960
  set->nr_hw_queues = ctrl->queue_count - 1            core.c:4961  ← admin 큐 제외
  set->timeout      = NVME_IO_TIMEOUT                  core.c:4962
  set->nr_maps      = nvme_pci_nr_maps(dev)            core.c:4963  ← 1~3

  blk_mq_alloc_tag_set(set)                            core.c:4964
  ctrl->tagset = set                                   core.c:4980
```

`nvme_pci_nr_maps(dev)` (`pci.c:2279`):
- poll 큐 있으면 3 (DEFAULT + READ + POLL)
- 기본 1 또는 2

### 3.3 `nvme_mq_ops` — IO 전용 추가 ops

`nvme_mq_ops` 구조체 (`pci.c:2249`):

```c
.queue_rq     = nvme_queue_rq         // 단건 제출
.queue_rqs    = nvme_queue_rqs        // 배치 제출 (admin엔 없음)
.complete     = nvme_pci_complete_rq
.commit_rqs   = nvme_commit_rqs       // doorbell flush (admin엔 없음)
.init_hctx    = nvme_init_hctx        // pci.c:655
.init_request = nvme_pci_init_request
.map_queues   = nvme_pci_map_queues   // IRQ affinity 기반 CPU 매핑 (admin엔 없음)
.timeout      = nvme_timeout
.poll         = nvme_poll             // 폴링 큐 (admin엔 없음)
```

### 3.4 IO hctx — `+1 오프셋` 규칙

```
blk_mq_init_hctx()                             blk-mq.c:4013
 ├─ hctx->tags      = set->tags[hctx_idx]      blk-mq.c:4025
 ├─ hctx->queue_num = hctx_idx                 blk-mq.c:4023
 └─ ops->init_hctx(hctx, data, hctx_idx)       blk-mq.c:4027
      → nvme_init_hctx()                        pci.c:655
           → nvme_init_hctx_common(hctx, data,
                                   hctx_idx + 1)  pci.c:658   ← +1 오프셋!
                hctx->driver_data = &dev->queues[hctx_idx + 1]
                // blk-mq hctx[0] → NVMe qid 1 (IO 큐 첫 번째)
                // blk-mq hctx[i] → NVMe qid i+1
                // qid 0은 admin 전용 → IO hctx는 항상 +1
```

### 3.5 Namespace disk → IO tagset 연결 (`core.c:4135`)

```
nvme_scan_work → nvme_alloc_ns(ctrl, info)            core.c:4135
 ├─ disk = blk_mq_alloc_disk(ctrl->tagset, &lim, ns)  core.c:4153
 │         → blk_mq_alloc_queue(set, ...)             blk-mq.c:4497
 │              ns마다 새로운 request_queue 생성
 │              tagset은 ctrl->tagset으로 공유
 │
 ├─ ns->queue = disk->queue                            core.c:4160
 └─ nvme_init_ns_head(ns, info)                        core.c:4164

// NS0, NS1, NS2 ... 모두 동일 ctrl->tagset 공유
// 두 번째 큐가 attach되는 순간:
blk_mq_add_queue_tag_set(set, q)                       blk-mq.c:4367~4372
 → BLK_MQ_F_TAG_QUEUE_SHARED 플래그 설정              ← 태그를 NS들이 나눠씀
```

---

## 4. blk-mq 레이어 내부 — tagset · hctx · 요청 풀 초기화

### 4.1 `blk_mq_alloc_tag_set()` (`blk-mq.c:4864`) — 단계별 동작

```
1. 입력 검증
   nr_hw_queues, queue_depth, ops->queue_rq 필수 체크    blk-mq.c:4870~4878

2. queue_depth > BLK_MQ_MAX_DEPTH 면 클램프              blk-mq.c:4883

3. nr_maps 미설정 시 1로 보정                             blk-mq.c:4889

4. nr_maps==1 && nr_hw_queues > nr_cpu_ids
   → nr_cpu_ids로 제한                                   blk-mq.c:4906

5. BLK_MQ_F_BLOCKING 이면 srcu 할당                      blk-mq.c:4909

6. set->tags = kcalloc(nr_hw_queues, sizeof(blk_mq_tags*))  blk-mq.c:4924
   ← hw큐 수만큼 태그 포인터 배열 할당

7. 각 map에 mq_map 배열 할당, nr_queues 설정             blk-mq.c:4930~4936
   ← map[DEFAULT/READ/POLL] 각각의 CPU→hwq 매핑 테이블

8. blk_mq_update_queue_map(set)                          blk-mq.c:4939
   ← CPU→hwq 매핑 테이블 채우기 (§4.2 참조)

9. blk_mq_alloc_set_map_and_rqs(set)                     blk-mq.c:4941
   ← tags[] + request pool 실제 할당 (§4.4 참조)

10. tag_list 초기화 (queue들을 연결할 링크드리스트)        blk-mq.c:4945~4946
```

### 4.2 CPU→hctx 매핑: `blk_mq_update_queue_map()` (`blk-mq.c:4780`)

```
set->ops->map_queues 있으면 (NVMe IO: nvme_pci_map_queues):
  각 map 클리어 후 ops->map_queues(set) 호출              blk-mq.c:4807~4810
  → nvme_pci_map_queues() (pci.c:681)
       DEFAULT 타입: blk_mq_map_hw_queues()              pci.c:702
                     (IRQ affinity CPU 마스크 기반)
       READ 타입:    blk_mq_map_hw_queues()
       POLL 타입:    blk_mq_map_queues()                 pci.c:704

set->ops->map_queues 없으면 (Admin):
  blk_mq_map_queues(&set->map[DEFAULT])                   blk-mq.c:4813
  ← round-robin 균등 분배 (CPU 수 / hw큐 수)
```

결과: `set->map[type].mq_map[cpu_id]` = 해당 CPU에 배정된 hctx 인덱스

### 4.3 `blk_mq_init_allocated_queue()` (`blk-mq.c:4651`) — request_queue 초기화

```
q->mq_ops   = set->ops                   blk-mq.c:4655   ← ops 연결
q->tag_set  = set                        blk-mq.c:4661   ← request_queue→tagset 역참조

blk_mq_alloc_ctxs(q)                     blk-mq.c:4663
 ← per-CPU blk_mq_ctx (SW queue) 배열 할당
   각 CPU마다 하나의 ctx, rq_lists[3] 보유

blk_mq_realloc_hw_ctxs(set, q)           blk-mq.c:4672
 └─ blk_mq_alloc_and_init_hctx(set,q,i)  ← 각 hctx 생성+초기화 (§4.5)
    q->queue_hw_ctx[] 배열에 등록

q->nr_requests = set->queue_depth        blk-mq.c:4686

blk_mq_init_cpu_queues(q, nr_hw_queues)  blk-mq.c:4689
 ← 각 per-CPU ctx의 기본 필드 초기화

blk_mq_map_swqueue(q)                    blk-mq.c:4690
 ← SW(ctx) ↔ HW(hctx) 큐 최종 연결 (§4.6)

blk_mq_add_queue_tag_set(set, q)         blk-mq.c:4691
 ← set->tag_list에 이 request_queue 추가
```

### 4.4 요청 풀 할당: `blk_mq_alloc_set_map_and_rqs()` (`blk-mq.c:4750`)

```
blk_mq_alloc_set_map_and_rqs()           blk-mq.c:4750
 └─ __blk_mq_alloc_rq_maps()             blk-mq.c:4713
      └─ hw큐별 __blk_mq_alloc_map_and_rqs()  blk-mq.c:4726
           → set->tags[i] = blk_mq_alloc_map_and_rqs()   blk-mq.c:4167

                blk_mq_alloc_rq_map()    blk-mq.c:4145 → 3566
                 → blk_mq_init_tags()    blk-mq.c:3577
                      rqs[], static_rqs[] 포인터 배열 할당
                      sbitmap_queue_init_node(&tags->bitmap_tags)   blk-mq-tag.c:550
                      sbitmap_queue_init_node(&tags->breserved_tags) blk-mq-tag.c:573
                      ← 태그 인덱스를 비트맵으로 관리

                blk_mq_alloc_rqs()       blk-mq.c:4149 → 3617
                 rq_size = round_up(sizeof(request) + cmd_size, cacheline)  blk-mq.c:3632
                 // request 구조체 바로 뒤에 nvme_iod 인라인 배치
                 // blk_mq_rq_to_pdu(rq)로 nvme_iod 포인터 얻음
                 → ops->init_request = nvme_pci_init_request 호출  blk-mq.c:3676
                      prp_list_pool 할당 등 nvme_iod 초기화
```

### 4.5 hctx 생성: `blk_mq_alloc_and_init_hctx()` (`blk-mq.c:4535`)

```
1. unused_hctx_list에서 재사용 가능 hctx 탐색           blk-mq.c:4542~4551

2. 없으면 blk_mq_alloc_hctx()                           blk-mq.c:4554 → 4047
    kzalloc(sizeof(*hctx))
    cpumask_alloc() (이 hctx 담당 CPU 마스크)
    ctxs = kcalloc(nr_possible_cpus, sizeof(*hctx->ctxs))
    sbitmap_init_node(&hctx->ctx_map)    ← SW큐 pending 비트맵
    hctx->queue    = q
    hctx->flags    = set->flags
    hctx->numa_node = NUMA_NO_NODE (set->numa_node에서 선택됨)

3. blk_mq_init_hctx()                                   blk-mq.c:4558 → 4013
    hctx->fq       = flush_queue 할당                   blk-mq.c:4019
    hctx->queue_num = hctx_idx                          blk-mq.c:4023
    hctx->tags     = set->tags[hctx_idx]                blk-mq.c:4025  ← hctx↔tags 연결
    ops->init_hctx(hctx, set->driver_data, hctx_idx)   blk-mq.c:4027
    ← driver가 hctx->driver_data에 nvme_queue 포인터 바인딩
```

### 4.6 SW↔HW 큐 매핑: `blk_mq_map_swqueue()` (`blk-mq.c:4192`)

```
각 CPU i, 각 타입 j (DEFAULT/READ/POLL):
  hctx_idx = set->map[j].mq_map[i]           blk-mq.c:4220
  hctx      = q->queue_hw_ctx[hctx_idx]      blk-mq.c:4233
  ctx       = per_cpu_ptr(q->queue_ctx, i)

  ctx->hctxs[j]     = hctx                   blk-mq.c:4234
  cpumask_set_cpu(i, hctx->cpumask)          blk-mq.c:4243
  ctx->index_hw[j]  = hctx->nr_ctx
  hctx->ctxs[hctx->nr_ctx++] = ctx           blk-mq.c:4245~4246
  // hctx가 자신에 매핑된 SW큐(ctx) 목록 보유

매핑된 SW큐 없는 hctx → tags 해제 후 비활성             blk-mq.c:4267~4276
매핑된 hctx → hctx->tags = set->tags[i]               blk-mq.c:4279
              sbitmap_resize(&hctx->ctx_map, hctx->nr_ctx)  blk-mq.c:4287
```

---

## 5. 핵심 연결 관계 다이어그램

### 5.1 Admin Queue 연결 구조

```
nvme_dev (pci.c:294)
 │
 ├─ .ctrl ──────────────────────────────────────────► nvme_ctrl (nvme.h:334)
 │                                                      │
 ├─ .admin_tagset ─────────────────────────────────►   ├─ .admin_tagset ─► blk_mq_tag_set [ADMIN]
 │   (pci.c:297)                                        │                     .ops          = nvme_mq_admin_ops
 │                                                      │                     .nr_hw_queues = 1
 │                                                      │                     .queue_depth  = 30
 │                                                      │                     .nr_maps      = 1
 │                                                      │                     .tags[0] ──────► blk_mq_tags
 │                                                      │                                        .bitmap_tags (sbitmap_queue)
 │                                                      │                                        .static_rqs[] (request + nvme_iod)
 │                                                      │
 └─ .queues[0] ─────────────────────────────────────   ├─ .admin_q ────────► request_queue [ADMIN]
     (nvme_queue, Admin SQ/CQ ring)                     │                      .tag_set ──────► [ADMIN blk_mq_tag_set]
       .sq_cmds (DMA SQ ring)                           │                      .queue_hw_ctx[0] ──► blk_mq_hw_ctx [0]
       .cqes    (DMA CQ ring)                           │                                              .tags       ──► tags[0]
       .q_db    (doorbell MMIO)                         │                                              .queue_num  = 0
       .qid     = 0                                     │                                              .driver_data ──► queues[0]
                    ◄───────────────────────────────────┘                                                                (Admin NVMe SQ/CQ)
```

### 5.2 IO Queue 연결 구조

```
nvme_dev (pci.c:294)
 │
 ├─ .ctrl ──────────────────────────────────────────► nvme_ctrl (nvme.h:334)
 │                                                      │
 ├─ .tagset (IO) ──────────────────────────────────►   ├─ .tagset ──────────► blk_mq_tag_set [IO]
 │   (pci.c:296)                                        │                       .ops          = nvme_mq_ops
 │                                                      │                       .nr_hw_queues = queue_count - 1
 │                                                      │                       .queue_depth  = min(sqsize, MAX)
 │                                                      │                       .nr_maps      = 1~3
 │                                                      │                       .map[DEFAULT].mq_map[cpu] → hwq idx
 │                                                      │                       .tags[0..N-1]
 │                                                      │                          [i] → blk_mq_tags
 │                                                      │                                  .bitmap_tags
 │                                                      │                                  .static_rqs[]
 │
 ├─ .queues[1] ──────────────────────────────────────── hctx[0].driver_data
 │   (IO NVMe SQ/CQ qid=1)
 ├─ .queues[2] ──────────────────────────────────────── hctx[1].driver_data
 │   (IO NVMe SQ/CQ qid=2)                                     (+1 오프셋: hctx[i] → queues[i+1])
 └─ .queues[N] ──────────────────────────────────────── hctx[N-1].driver_data

NS0 → request_queue [NS0]  ─── .tag_set ──────► [IO blk_mq_tag_set] (공유)
NS1 → request_queue [NS1]  ─── .tag_set ──────►       │
NS2 → request_queue [NS2]  ─── .tag_set ──────►       │  (BLK_MQ_F_TAG_QUEUE_SHARED)
                                                        │
request_queue [NS0]                              blk_mq_tag_set [IO]
 .queue_hw_ctx[0..N-1]:                           .tags[i] ─────────────────────────────►
   [0] → blk_mq_hw_ctx                                                              blk_mq_tags
           .tags        ──► tags[0]                                                   .nr_tags
           .queue_num   = 0                                                           .bitmap_tags
           .driver_data ──► queues[1]  (NVMe qid 1)                                  .breserved_tags
           .ctxs[]      ──► blk_mq_ctx (per-CPU SW queue)                            .static_rqs[]
                              .hctxs[DEFAULT] ──► [이 hctx]                          .rqs[]
                              .rq_lists[]      (pending 요청 리스트)
   [1] → blk_mq_hw_ctx
           .driver_data ──► queues[2]  (NVMe qid 2)
   ...
   [N-1] → blk_mq_hw_ctx
            .driver_data ──► queues[N]  (NVMe qid N)
```

### 5.3 IO 요청 흐름 (제출 경로 요약)

```
유저스페이스 read/write syscall
       │
       ▼
  request_queue (NS의 disk->queue)
       │  blk_mq_submit_bio()
       ▼
  blk_mq_ctx (per-CPU SW queue)
  ctx = get_cpu_ptr(q->queue_ctx)
  ctx->rq_lists[type]에 request 추가
       │  blk_mq_run_hw_queue()
       ▼
  blk_mq_hw_ctx (hctx)
  hctx = ctx->hctxs[type]
  sbitmap_queue 에서 태그(인덱스) 할당
       │  hctx->queue_num → set->tags[queue_num]
       ▼
  ops->queue_rq(hctx, bd) = nvme_queue_rq()  (pci.c:1405)
  nvmeq = hctx->driver_data                  (dev->queues[hctx->queue_num + 1])
  iod   = blk_mq_rq_to_pdu(req)             (request 뒤 inline nvme_iod)
  nvme_sq_copy_cmd() → SQ ring에 NVMe 명령 복사
  write_sq_db()      → doorbell MMIO 기록 → 디바이스에 제출 알림
       │
       ▼  (비동기)
  MSI-X 인터럽트 발생 → nvme_irq() → nvme_process_cq()
  CQ ring 순회 → blk_mq_complete_request()
  ops->complete = nvme_pci_complete_rq()
  sbitmap_queue에서 태그 반납
```

---

## 6. Admin vs IO Queue 비교표

| 항목 | Admin Queue | IO Queue |
|---|---|---|
| **tag_set 인스턴스** | `dev->admin_tagset` (pci.c:297) | `dev->tagset` (pci.c:296) |
| **ctrl 포인터** | `ctrl->admin_tagset`, `ctrl->admin_q` | `ctrl->tagset` + NS별 request_queue |
| **ops** | `nvme_mq_admin_ops` | `nvme_mq_ops` (+queue_rqs/commit_rqs/map_queues/poll) |
| **nr_hw_queues** | **1** (core.c:4881) | `ctrl->queue_count - 1` (core.c:4961) |
| **queue_depth** | `NVME_AQ_MQ_TAG_DEPTH` = 30 (core.c:4872) | `min(sqsize, BLK_MQ_MAX_DEPTH-1)` (core.c:4946) |
| **nr_maps** | 1 | 1~3 (`nvme_pci_nr_maps()`) |
| **reserved_tags** | PCIe: 0 / Fabrics: 2 (core.c:4873~4875) | PCIe: 0 / SHARED_TAGS quirk: 32 (core.c:4951~4952) |
| **CPU 매핑** | `blk_mq_map_queues` (균등 round-robin) | `nvme_pci_map_queues` → IRQ affinity |
| **배치·폴링** | 없음 | `queue_rqs` 배치 + poll 큐 |
| **request_queue 생성** | `blk_mq_alloc_queue()` 1개 → `ctrl->admin_q` (core.c:4894) | NS마다 `blk_mq_alloc_disk()` → 모두 같은 tagset 공유 (core.c:4153) |
| **hctx→NVMe 큐 오프셋** | hctx[0] → `queues[0]` (qid 0) | hctx[i] → `queues[i+1]` (**+1**, pci.c:658) |
| **SQ/CQ ring 생성 시점** | `nvme_pci_configure_admin_queue()` (pci.c:2300), tagset **이전** | `nvme_create_io_queues()` (pci.c:2378), tagset **이전** |
| **타임아웃** | `NVME_ADMIN_TIMEOUT` | `NVME_IO_TIMEOUT` |
| **할당 시점** | probe 초기, IO 큐 협상 전 | `online_queues > 1` 확인 후 (pci.c:3776) |
| **생명주기** | 리셋 후에도 유지·재사용 | 리셋 시 재생성 |

---

## 7. Admin이 별도 tagset을 쓰는 이유

### 7.1 부트스트랩 순서 의존성

IO 큐 수는 `nvme_set_queue_count()`로 **디바이스와 admin 명령으로 협상**해야 알 수 있다 (pci.c:2943). admin tag_set/queue가 먼저 존재해야 `Identify`, `Set Features` 등의 admin 명령을 보낼 수 있다. 즉, admin 큐 없이는 IO tagset의 `nr_hw_queues`를 결정조차 할 수 없다.

### 7.2 서로 다른 ops와 제약

admin은 `nr_hw_queues=1`, 폴링/배치/다중맵 없음. IO는 CPU별 다중 큐 + IRQ affinity + poll 큐. 구조적으로 같은 tagset에 묶을 수 없다.

### 7.3 생명주기 분리

리셋 시 IO 큐는 재생성되지만 (`nvme_reset_work` → `nvme_setup_io_queues`), admin 큐는 유지·재사용된다. IO 큐가 0개인 degraded 상태에서도 admin 큐만으로 펌웨어 복구·재시도가 가능하도록 설계되어 있다 (pci.c:3423~3429).

### 7.4 hctx 인덱스와 NVMe qid 매핑 일관성

blk-mq hctx 인덱스는 각 tagset에서 0부터 시작한다. 별도 tagset으로 분리함으로써:
- admin: hctx[0] → NVMe qid 0 (오프셋 +0)
- IO: hctx[i] → NVMe qid i+1 (오프셋 +1, `nvme_init_hctx_common`, pci.c:629)

로 인덱스 변환 규칙이 일관되게 유지된다. 만약 같은 tagset이라면 admin(hctx[0]=qid 0)과 IO(hctx[1..N]=qid 1..N)가 섞여 오프셋 계산이 복잡해진다.

---

## 8. 주요 함수 인덱스

| 함수명 | 파일:라인 | 역할 |
|---|---|---|
| `nvme_probe` | `pci.c:3712` | PCI 드라이버 entry point |
| `nvme_pci_configure_admin_queue` | `pci.c:2300` | Admin SQ/CQ ring 생성 + BAR 기록 |
| `nvme_alloc_queue` | `pci.c:2100` | nvme_queue (SQ/CQ ring) DMA 할당 |
| `nvme_create_queue` | `pci.c:2189` | Create CQ/SQ 명령 + IRQ 연결 |
| `nvme_alloc_admin_tag_set` | `core.c:4865` | Admin blk_mq_tag_set + request_queue 생성 |
| `nvme_setup_io_queues` | `pci.c:2909` | IO 큐 협상 + ring 생성 |
| `nvme_alloc_io_tag_set` | `core.c:4938` | IO blk_mq_tag_set 생성 |
| `nvme_alloc_ns` | `core.c:4135` | Namespace disk + request_queue 생성 |
| `nvme_init_hctx_common` | `pci.c:629` | hctx→nvme_queue 포인터 바인딩 (+1 오프셋) |
| `blk_mq_alloc_tag_set` | `blk-mq.c:4864` | blk_mq_tag_set 전체 초기화 |
| `blk_mq_update_queue_map` | `blk-mq.c:4780` | CPU→hctx 매핑 테이블 갱신 |
| `blk_mq_alloc_set_map_and_rqs` | `blk-mq.c:4750` | tags[] + request pool 할당 |
| `blk_mq_init_allocated_queue` | `blk-mq.c:4651` | request_queue를 blk-mq 큐로 초기화 |
| `blk_mq_alloc_and_init_hctx` | `blk-mq.c:4535` | blk_mq_hw_ctx 생성 및 초기화 |
| `blk_mq_map_swqueue` | `blk-mq.c:4192` | SW(ctx) ↔ HW(hctx) 큐 최종 연결 |
| `blk_mq_add_queue_tag_set` | `blk-mq.c:4367` | request_queue를 tagset에 등록 |
| `nvme_pci_map_queues` | `pci.c:681` | IRQ affinity 기반 CPU→hwq 매핑 |
| `nvme_queue_rq` | `pci.c:1405` | IO 제출: SQ ring 복사 + doorbell |
