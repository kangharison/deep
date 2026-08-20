# CPU 수 초과 큐 생성 + SQ:CQ = N:1 + 동적 큐 관리 — v7.2 기준 방안 탐색

> **베이스라인**: `linux/` = torvalds/linux **v7.2** (tag object `237a1c39e8df`, commit `8d3ae59288f1 "Linux 7.2"`)
> **코드 참조 표기**: 이하 모든 `파일:라인`은 위 v7.2 트리 기준. 예) `linux/drivers/nvme/host/pci.c:2940`
> **선행 문서**: [[nvme-queue-mgmt-design]] (ioctl 기반 큐 관리 설계, Tier A/B 모델, freeze 타임라인, stale CQE 방어)
> **관련 문서**: [[nvme-queue-init-request-queue-tagset-hctx]], [[nvme-tagset-blkmq-flow]], [[io-lifecycle-no-scheduler]], [[drgn-live-blkmq-nvme-pcie-prp]]

---

## 0. 목표와 결론 요약 (TL;DR)

### 0.1 요구 목표

| ID | 목표 | 한 줄 정의 |
|----|------|-----------|
| **G1** | CPU 수 초과 큐 생성 | 디바이스가 `nr_cpus`보다 많은 IO queue pair를 광고하면 드라이버가 **전부** 생성 |
| **G2** | SQ:CQ = N:1 | NVMe 스펙이 허용하는 "여러 SQ가 하나의 CQ를 공유" 지원 |
| **G3** | ctx ≤ hctx + 동적 큐 | blk-mq에서 CPU(ctx) 하나가 여러 hctx에 매핑되는 형태로 갈 경우, 큐를 **런타임에 생성/삭제** 가능하게 |

### 0.2 세 목표의 의존 관계 — 독립 목표가 아니다

```
G2 (N:1) ──enables──> G1 (큐 수 > CPU 수)
   이유: 큐 수 상한이 MSI-X 벡터 수에 묶여 있음 (pci.c:3040 dev->max_qid = num_vecs-1+poll).
        벡터 없이 SQ만 늘리려면 CQ를 공유해야 한다 = N:1.

G3 (ctx ≤ hctx) ──enables──> G1이 "블록 IO에서 실제로 쓰임"
   이유: blk-mq의 mq_map[cpu]는 CPU당 hctx 1개(타입당). 큐가 CPU보다 많으면
        남는 hctx는 nr_ctx==0 → tags 해제 → 죽은 큐가 된다 (blk-mq.c:4237-4245).
```

즉 **G1은 단독 목표가 아니라 G2·G3의 결과**다. 작업 순서도 이 의존 관계를 따라야 한다.

### 0.3 핵심 발견 4가지

| # | 발견 | 임팩트 |
|---|------|--------|
| **F1** | blk-mq의 `nr_hw_queues ≤ nr_cpu_ids` 캡은 **`nr_maps == 1`일 때만** 적용된다 (blk-mq.c:4876, 5119). NVMe의 `nr_maps`는 `nvme_pci_nr_maps()`(pci.c:3159)가 정하는데 **`write_queues`/`poll_queues` 모듈 파라미터가 기본값 0(pci.c:265, 271)이므로 기본 구성에서는 `nr_maps == 1` → 캡이 적용된다.** 반대로 `nvme.write_queues>0`이나 `poll_queues>0`을 주면 `nr_maps ≥ 2`가 되어 **같은 캡이 조용히 사라진다** | 캡은 일관성이 없는 *약한* 장벽이다. 이미 멀티맵 경로에서는 blk-mq가 `nr_hw_queues > nr_cpu_ids`를 허용하고 있으나 아무도 쓰지 않는다. **진짜 병목은 캡이 아니라 매핑 함수(B2)** — 캡을 지워도 §2처럼 실패한다 |
| **F2** | "ctx 하나가 여러 hctx에 속하는 구조"는 **이미 구현되어 있다** — `ctx->hctxs[HCTX_MAX_TYPES]` (blk-mq.h:27). CPU 1개가 default/read/poll hctx 3개에 동시 소속 | ctx ≤ hctx는 신규 개념이 아니라 **기존 축(HCTX_MAX_TYPES=3)의 일반화** 문제다 |
| **F3** | 전체 freeze 없이 hctx 하나만 격리·drain하는 프리미티브가 **이미 있다** — `BLK_MQ_S_INACTIVE` + 태그 반납 + retry (blk-mq.c:3735 / blk-mq-tag.c:227 / blk-mq.c:586) | G3의 "동적 삭제"를 `blk_mq_update_nr_hw_queues`의 전체 freeze 없이 구현할 토대 |
| **F4** | `blk_mq_map_swqueue`가 tags를 **lazy alloc**한다 (blk-mq.c:4191-4200) + 매핑 없는 hctx는 tags를 해제한다 (4237-4245) | "hctx는 존재하지만 `tags == NULL`인 **휴면 상태**"가 blk-mq에 이미 1급 상태로 존재 → 오버프로비저닝 + 동적 활성화의 토대 |

### 0.4 권장 결론

**옵션 1(드라이버 내 SQ fan-out) → 옵션 4(per-hctx 동적 activate/deactivate) → (필요 시) 옵션 2/3(hctx 축 일반화)** 의 단계적 접근.

이유: 옵션 1만으로 G1·G2가 **blk-mq 무수정**으로 달성되고, 완료 경로 정합성(가장 위험한 부분)이 **구조적으로 자동 해결**된다. ctx ≤ hctx(옵션 2/3)는 `struct blk_mq_ctx`를 모든 블록 디바이스에 대해 키우는 침습적 변경이므로, "옵션 1로는 부족하다"가 계측으로 증명된 뒤에 간다.

---

## 1. Vanilla v7.2 제약 인벤토리 — 코드 라인 근거

목표를 막는 지점을 전수 조사했다. NVMe 11곳, blk-mq 7곳.

### 1.1 NVMe 드라이버 측 — CPU 결합 및 1:1 하드코딩

| # | 위치 | 코드 | 무엇을 막는가 |
|---|------|------|--------------|
| **N1** | `linux/drivers/nvme/host/pci.c:2932-2942` | `nvme_max_io_queues()` → `blk_mq_num_possible_queues(0) + nr_write_queues + nr_poll_queues` | 큐 수 상한이 `cpumask_weight(cpu_possible_mask)` 기반 (`blk-mq-cpumap.c:38-41`). **G1 1차 차단** |
| **N2** | `linux/drivers/nvme/host/pci.c:3691` | `dev->nr_allocated_queues = nvme_max_io_queues(dev) + 1;` → `dev->queues = kcalloc_node(...)` | `dev->queues[]` **배열 크기 자체**가 CPU 수. 협상으로 더 받아도 담을 곳이 없다 |
| **N3** | `linux/drivers/nvme/host/core.c:1745-1765` | `q_count = (*count-1) \| ((*count-1) << 16)` 후 `nr_io_queues = min(result & 0xffff, result >> 16) + 1` | Set Features 07h의 **NSQR/NCQR을 같은 값으로 요청**하고 응답도 **min(NSQA, NCQA)**로 뭉갠다 → SQ/CQ 개수 독립 협상 불가. **G2 1차 차단** |
| **N4** | `linux/drivers/nvme/host/pci.c:3036-3040` | `dev->num_vecs = result; result = max(result-1, 1); dev->max_qid = result + io_queues[HCTX_TYPE_POLL];` | 큐 수 상한 = **MSI-X 벡터 수**. 벡터는 IRQ affinity로 CPU에 뿌려지므로 실질적으로 CPU 수 상한. **G1 2차 차단(핵심)** |
| **N5** | `linux/drivers/nvme/host/pci.c:2233` | `vector = dev->num_vecs == 1 ? 0 : qid;` | 벡터 인덱스 == qid 전제 |
| **N6** | `linux/drivers/nvme/host/pci.c:1797` | `c.create_sq.cqid = cpu_to_le16(qid);` | **SQ:CQ 1:1 하드코딩**. 스펙은 임의 cqid 허용. **G2 핵심 지점** |
| **N7** | `linux/drivers/nvme/host/pci.c:2156`, `2191` | `nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];` | SQ 도어벨(`q_db`)과 CQ 도어벨(`q_db + db_stride`, pci.c:1552)을 **한 객체가 소유**. N:1이면 CQ 도어벨의 주인이 모호해짐 |
| **N8** | `linux/drivers/nvme/host/pci.c:1555-1560` | `nvme_queue_tagset()` → `dev->tagset.tags[nvmeq->qid - 1]` | CQE→request 역참조가 "**완료를 수확한 CQ의 qid == 그 명령을 제출한 SQ의 qid**"를 가정. N:1에서 그대로 두면 **엉뚱한 tags에서 command_id를 찾아 오조회/미조회** |
| **N9** | `linux/drivers/nvme/host/pci.c:660-663` | `nvme_init_hctx()` → `nvme_init_hctx_common(hctx, data, hctx_idx + 1)` | hctx 인덱스와 qid의 **선형·무결손 결합** |
| **N10** | `linux/drivers/nvme/host/pci.c:686-711` | `nvme_pci_map_queues()` → `blk_mq_map_hw_queues(map, dev->dev, offset)` | CPU↔큐 매핑을 **벡터 affinity**에서 유도 (`blk-mq-cpumap.c:118-125`). "큐 하나당 벡터 하나" 전제 |
| **N11** | `linux/drivers/nvme/host/pci.c:249-257` | `io_queue_count_set()`: `if (ret != 0 \|\| n > blk_mq_num_possible_queues(0)) return -EINVAL;` | `nvme.write_queues` / `nvme.poll_queues` 모듈 파라미터 **자체가 CPU 수로 상한**. 두 값이 기본 0이라 `nvme_pci_nr_maps()`가 1을 반환 → B1 캡이 켜진다 |

부수 제약 2개:

- `db_bar_size()` (pci.c:2306-2309) + `nvme_remap_bar()` — 도어벨 BAR가 `NVME_REG_DBS + (nr+1)*8*db_stride` 이상이어야 하고, 실패 시 큐 수를 반씩 줄인다 (pci.c:3007-3016). 큐 1024개/stride 0 → 약 12.3 KB로 실무상 여유. **큰 문제 아님**.
- `nvme_cmb_qdepth()` (pci.c:2083-2093) — CMB에 SQ를 두면 큐 수↑ → 큐당 depth↓. CMB SQ 사용 시에만 해당.

### 1.2 blk-mq 측 — 매핑·토폴로지 제약

| # | 위치 | 코드 | 무엇을 막는가 |
|---|------|------|--------------|
| **B1** | `linux/block/blk-mq.c:4876-4877`, `5119-5120` | `if (set->nr_maps == 1 && nr_hw_queues > nr_cpu_ids) nr_hw_queues = nr_cpu_ids;` | 기본 NVMe 구성은 `nr_maps == 1`(pci.c:3159, `write_queues`/`poll_queues` 기본 0)이라 **캡이 적용된다**. 단 `nvme.write_queues>0`이면 `nr_maps ≥ 2`가 되어 캡이 사라진다 → **일관성 없는 약한 장벽** (→ F1) |
| **B2** | `linux/block/blk-mq-cpumap.c:71-74` + `linux/lib/group_cpus.c:254` | `for (queue...) for_each_cpu(cpu, &masks[queue % nr_masks]) qmap->mq_map[cpu] = qmap->queue_offset + queue;` / `numgrps = min_t(unsigned, numcpus, numgrps)` | `mq_map[cpu]`는 **CPU당 값 1개**. `nr_masks ≤ ncpus`이므로 큐 수 > CPU 수면 `queue % nr_masks`가 되감기며 **뒤 번호 큐가 앞 번호를 덮어쓴다** → 앞쪽 큐들은 어떤 CPU도 가리키지 않게 됨. **진짜 병목** |
| **B3** | `linux/block/blk-mq.c:4230-4247` | `if (!hctx->nr_ctx) { if (i) __blk_mq_free_map_and_rqs(set, i); hctx->tags = NULL; continue; }` | B2로 고아가 된 hctx는 **tags 해제 + `tags=NULL`** → 죽은 큐. G1을 넣어도 실제로는 안 쓰인다 |
| **B4** | `linux/block/blk-mq.h:19-32` | `struct list_head rq_lists[HCTX_MAX_TYPES];` / `unsigned short index_hw[HCTX_MAX_TYPES];` / `struct blk_mq_hw_ctx *hctxs[HCTX_MAX_TYPES];` | **ctx 하나가 소속될 수 있는 hctx 수의 상한 = `HCTX_MAX_TYPES`(=3)**. ctx ≤ hctx 일반화의 **구조적 병목** (→ §3.3) |
| **B5** | `linux/block/blk-mq.c:553` + `linux/block/blk-mq.h:109-113` | `data->hctx = blk_mq_map_queue(data->cmd_flags, data->ctx);` → `return ctx->hctxs[blk_mq_get_hctx_type(opf)];` | 라우팅 결정 지점이 **딱 한 곳**. 정책 훅을 넣기엔 좋은 조건 |
| **B6** | `linux/block/blk-mq.c:5106-5204` | `__blk_mq_update_nr_hw_queues()` | 토폴로지 변경 API가 **이것 하나뿐**. tagset의 **모든** request_queue freeze + elevator를 none으로 전환 + **전 hctx 재할당**(`__blk_mq_realloc_hw_ctxs`가 기존 hctx도 전부 exit/realloc, `:5165` → `:4539`) → 큐 하나 추가에 O(전체) 비용. **G3 핵심 장애** |
| **B7** | `linux/block/blk-mq.c:4128-4160`, `4683-4697` | hctx마다 `set->tags[i]` + request pool(`queue_depth`개) | 메모리 O(nr_hw_queues × queue_depth × (sizeof(request)+cmd_size)) (→ §5.2) |

### 1.3 이미 존재하는 재사용 자산

| # | 위치 | 내용 | 활용처 |
|---|------|------|--------|
| **A1** | `linux/block/blk-mq.h:19-32`, `linux/block/blk-mq.c:4183-4207` | ctx→hctx 다대일이 **이미 최대 3개**까지 구현·검증됨 (default/read/poll). `blk_mq_map_swqueue`의 `for (j = 0; j < set->nr_maps; j++)` 루프가 그 구현체 | 옵션 2/3의 뼈대 |
| **A2** | `linux/block/blk-mq.c:3719-3762`, `linux/block/blk-mq-tag.c:224-231`, `linux/block/blk-mq.c:576-587` | `set_bit(BLK_MQ_S_INACTIVE)` → `blk_mq_get_tag()`이 `found_tag:`에서 **태그를 반납하고 `BLK_MQ_NO_TAG`** → `__blk_mq_alloc_requests`가 `msleep(3); goto retry` → `blk_mq_hctx_has_requests()` 0까지 대기 | **전체 freeze 없는 per-hctx drain**. 옵션 4의 심장 |
| **A3** | `linux/block/blk-mq.c:4191-4200` | `if (!set->tags[hctx_idx] && !__blk_mq_alloc_map_and_rqs(set, hctx_idx)) set->map[j].mq_map[i] = 0;` — tags **lazy alloc**, 실패 시 hctx0으로 폴백 | "hctx 존재 + tags==NULL = 휴면"이 1급 상태 → 오버프로비저닝 |
| **A4** | `linux/drivers/nvme/host/pci.c:3042-3047` | 업스트림 주석: *"Should investigate if there's a performance win from allocating more queues than interrupt vectors; it might allow the submission path to scale better, even if the receive path is limited by the number of interrupts."* | **G1+G2가 업스트림에서 이미 인지된 미개척 영역**임을 보여주는 근거. 설계 정당화에 인용 가능 |
| **A5** | `linux/block/blk-mq.c:4684-4691`, `4131-4132` | `BLK_MQ_F_TAG_HCTX_SHARED` → `set->tags[i] = set->shared_tags` (전 hctx가 tags 1개 공유) | command_id 전역 유일화 → N:1 완료 역참조 무수정 해결 (§4.3 선택지 3) |

---

## 2. 왜 지금 그냥 큐를 늘리면 실패하는가 — 실패 시나리오 추적

`nvme_max_io_queues()`만 크게 고쳐서 `nr_hw_queues = 256` (CPU 64개)로 만들었다고 가정하자.

```
① nvme_setup_io_queues()
     nr_io_queues = 256 요청 → nvme_set_queue_count()에서 min(NSQA,NCQA) 통과 (N3)
     ↓
② nvme_setup_irqs(dev, 256)
     pci_alloc_irq_vectors_affinity(pdev, 1, 257, ...)
     → 디바이스 MSI-X 테이블 상한(스펙 최대 2048) 및 커널 벡터 여유에 따라 잘림
     → 보통 result ≈ (CPU 수 + 1) 수준으로 반환
     ↓
③ dev->max_qid = result - 1 + poll               (pci.c:3040)  ← ★ 여기서 다시 CPU 수로 회귀
     → 아무리 요청해도 큐 수는 벡터 수를 못 넘는다.  G1 실패.
```

강제로 ③을 뚫어 hctx 256개를 만들었다고 하자.

```
④ blk_mq_alloc_tag_set()
     기본 구성(write_queues=poll_queues=0)이면 nr_maps=1 → nr_cpu_ids 캡 발동 (B1)
       → nr_hw_queues = 64 로 잘림.  여기서 이미 조용히 실패.
     nvme.write_queues>0 등으로 nr_maps=2 이면 캡 미적용 → hctx 256개 할당 성공.
     아래 ⑤부터는 캡을 통과했다고 가정한 경우.
     ↓
⑤ nvme_pci_map_queues() → blk_mq_map_hw_queues() → (벡터 affinity 없으면) blk_mq_map_queues()
     group_cpus_evenly(256, &nr_masks) → nr_masks = min(64, 256) = 64      (group_cpus.c:254)
     for (queue = 0; queue < 256; queue++)
         for_each_cpu(cpu, &masks[queue % 64]) mq_map[cpu] = queue;         (blk-mq-cpumap.c:71-74)
     → queue=0..63이 쓴 값을 64..127이 덮고, 다시 128..191이 덮고, 192..255가 최종 승자
     → mq_map[]에는 큐 192~255만 남는다.  큐 0~191은 고아.
     ↓
⑥ blk_mq_map_swqueue(): 고아 hctx는 nr_ctx == 0
     → __blk_mq_free_map_and_rqs(set, i);  hctx->tags = NULL;               (blk-mq.c:4237-4245)
     → 죽은 hctx 192개.
     ↓
⑦ 결과: 하드웨어 SQ/CQ는 256쌍 만들어졌지만 블록 IO는 64개만 사용.
        나머지 192개는 DMA 메모리와 벡터만 점유하는 순수 낭비.
```

**핵심**: 큐 수를 늘리는 것과 그 큐를 *쓰는* 것은 완전히 별개다. §1의 N1~N4를 뚫어도 B2/B3에서 전부 무효화된다. 그래서 G1은 반드시 G2(벡터 제약 해소)와 G3(매핑 구조 변경) 중 **최소 하나**를 동반해야 한다.

---

## 3. 설계 방안 4가지

### 3.1 옵션 1 — 드라이버 내 SQ fan-out (blk-mq 무수정)

**아이디어**: blk-mq에는 여전히 hctx = CPU 수만큼만 보여준다. hctx 하나가 **여러 SQ를 소유**하고, `nvme_queue_rq()`에서 정책으로 SQ를 고른다. 그 SQ들은 하나의 CQ를 공유한다(N:1).

```
      blk-mq 관점                  NVMe 하드웨어 관점
  ┌──────────────────┐
  │ ctx(cpu0)        │
  │   └─ hctx0 ──────┼────┬──> SQ 1  ─┐
  │      (tags 1개)  │    ├──> SQ 2  ─┤
  │                  │    ├──> SQ 3  ─┼──> CQ 1 ──> MSI-X vec 1
  │                  │    └──> SQ 4  ─┘
  ├──────────────────┤
  │ ctx(cpu1)        │
  │   └─ hctx1 ──────┼────┬──> SQ 5  ─┐
  │                  │    └──> SQ 8  ─┴──> CQ 2 ──> MSI-X vec 2
  └──────────────────┘
```

**변경 지점**
- `hctx->driver_data`가 단일 `nvmeq`가 아니라 **SQ 집합 디스크립터**를 가리키도록 (`nvme_init_hctx`, pci.c:660)
- `nvme_queue_rq()` / `nvme_queue_rqs()`에서 SQ 선택 정책 호출 (ioprio class / write stream / NS / round-robin)
- `adapter_alloc_sq()` cqid 파라미터화 (N6), IRQ는 CQ owner 1회만 등록 (N5)

**결정적 장점 — 완료 경로가 구조적으로 안전하다**

태그(=command_id)는 **hctx 단위로 유일**하다. 한 hctx가 소유한 SQ들은 모두 같은 `hctx->tags`에서 태그를 받으므로, 그 SQ들의 완료가 공유 CQ에 섞여 들어와도 `nvme_queue_tagset()`(N8)이 반환하는 tags가 **원래부터 맞다**. → **N8 무수정**. 이게 이 옵션의 가장 큰 가치다. (§4.3 비교 참조)

**동적 큐 생성/삭제가 freeze 없이 된다**

blk-mq가 보는 `nr_hw_queues`가 변하지 않으므로 `blk_mq_update_nr_hw_queues()`(B6)를 **아예 부르지 않는다**. SQ 추가 = 해당 hctx의 SQ 집합에 추가 + Create SQ. SQ 삭제 = 라우팅에서 제외 → 그 SQ로 나간 in-flight만 drain → Delete SQ. **다른 namespace의 IO가 멈추지 않는다** ([[nvme-queue-mgmt-design]] §5.7의 함정 1·2를 근본 회피).

**한계**
- blk-mq 관점 큐 수 불변 → per-queue `nr_requests`, 스케줄러 인스턴스, `/sys/block/*/mq/N` 통계가 **분리되지 않는다**
- 태그 depth가 hctx 단위 → SQ를 4개로 쪼개도 **동시 in-flight 총량은 그대로**. 병렬성 확대 목적이면 무의미
- 따라서 적합 용도는 **격리/QoS**: WRR 우선순위별 SQ, NS별 SQ, stream별 SQ, CMB SQ 등

### 3.2 옵션 2 — hctx type 축 확장 (`HCTX_MAX_TYPES` 일반화)

**아이디어**: B4의 `HCTX_MAX_TYPES`를 3 → K로 늘리고, 늘어난 map을 "큐 클래스"로 사용한다. 총 hctx = Σ`map[i].nr_queues`, CPU당 최대 K개.

```
 nr_maps = K = 4 인 경우 (CPU 64개 → hctx 256개)

   ctx(cpu0) ──> hctxs[0] (class0, 큐   0..63 중 cpu0 담당)
             ├─> hctxs[1] (class1, 큐  64..127)
             ├─> hctxs[2] (class2, 큐 128..191)
             └─> hctxs[3] (class3, 큐 192..255)
```

**왜 이게 자연스러운가**: `blk_mq_map_swqueue`(blk-mq.c:4183-4207), CPU hotplug(`blk_mq_cpu_mapped_to_hctx`, blk-mq.c:3771), 스케줄러, sysfs/debugfs가 **전부 이미 type 루프**로 작성되어 있다. 차원만 늘리면 나머지는 그대로 동작한다. **재사용률이 가장 높다.**

**변경 지점**
- `enum hctx_type`에 클래스 추가 + `HCTX_MAX_TYPES` 상향 (`linux/include/linux/blk-mq.h:488-494`)
- 라우팅 선택자 `blk_mq_get_hctx_type(opf)` (`linux/block/blk-mq.h:90-102`)를 정책 훅으로 확장
- NVMe `nvme_pci_map_queues()` (pci.c:686)가 `io_queues[]`를 K개까지 채우도록
- `nvme_calc_irq_sets()` (pci.c:2859)의 `affd->set_size[]`도 K 대응 (`irq_affinity.set_size`는 `nr_sets` 상한 확인 필요)

**비용**
- `HCTX_MAX_TYPES`는 **컴파일 타임 상수**이고 `struct blk_mq_ctx`는 **per-CPU per-request_queue** 객체다. `rq_lists`(16B) + `index_hw`(2B) + `hctxs`(8B) ≈ 26B × K. K=8이면 ctx가 약 200B 늘고, 이는 **모든 블록 디바이스(loop, virtio-blk, SCSI …)** 에 부과된다
- CPU당 hctx 수가 K로 **고정 상한** → 디바이스가 광고하는 임의 개수(예: CPU 64에 큐 300개)를 정확히 못 맞춘다. map별 `nr_queues`는 달라도 되므로 총합은 자유롭지만, "CPU당 최대 K개"는 넘을 수 없다
- 동적 변경은 여전히 `blk_mq_update_nr_hw_queues`(B6) → **G3 미해결**

### 3.3 옵션 3 — ctx당 hctx 그룹 + 런타임 선택자 (문자 그대로의 ctx ≤ hctx)

**아이디어**: `mq_map[cpu]`(단일 값)를 `{base, count}` 그룹으로 확장하고, 제출 시점에 정책이 그룹 안에서 하나를 고른다.

**★ 중요한 발견 — 옵션 3의 자료구조 변경은 옵션 2의 그것과 같다**

CPU 하나가 **같은 타입의** hctx 2개에 속하면 다음이 동시에 깨진다:

```c
/* linux/block/blk-mq.c:2585 (blk_mq_insert_requests) — insert 시 ctx 리스트 선택 */
list_splice_tail_init(list, &ctx->rq_lists[type]);      /* type만으로 인덱싱 */

/* linux/block/blk-mq.c:74 (blk_mq_hctx_mark_pending) — hctx의 pending 비트 위치 */
const int bit = ctx->index_hw[hctx->type];              /* type당 값 1개뿐 */
sbitmap_set_bit(&hctx->ctx_map, bit);

/* linux/block/blk-mq.c:1786 (blk_mq_flush_busy_ctxs) — flush 시 그 리스트를 통째로 수확 */
blk_mq_flush_busy_ctxs(hctx, list);   /* → sbitmap_for_each_set(&hctx->ctx_map, flush_busy_ctx) */
```

같은 type의 hctx A와 B가 ctx C를 공유하면:
1. A로 갈 request와 B로 갈 request가 **같은 `C->rq_lists[type]`** 에 섞인다
2. `C->index_hw[type]`은 값을 하나만 담으므로 A와 B의 `ctx_map` 비트 위치가 **충돌**한다
3. `blk_mq_flush_busy_ctxs(A)`가 **B의 request까지 수확**해 A의 SQ로 보낸다 → 라우팅 붕괴

→ 따라서 **"ctx가 속할 수 있는 hctx 수"의 물리적 상한은 정확히 `HCTX_MAX_TYPES`** 이고, 옵션 3을 하려면 `rq_lists`/`index_hw`/`hctxs`의 두 번째 차원을 늘려야 한다. **= 옵션 2의 변경.**

**결론**: 옵션 2와 3은 대립 안이 아니다. **옵션 2 = 자료구조 일반화, 옵션 3 = 그 위의 선택자 정책화.** 로드맵에서 하나로 묶는다.

옵션 3에서 추가로 필요한 것:
- 선택자 훅: `blk_mq_map_queue()`(blk-mq.h:109)에 정책 인자 — per-task 고정(캐시 지역성 보존), ioprio, cgroup, write hint, round-robin 중 택1
- **주의**: 선택이 매 IO마다 바뀌면 `rq->mq_hctx`가 흩어져 plug batching(`blk_mq_flush_plug_list`)과 `queue_rqs` 배치 제출 효율이 떨어진다. **task 단위 sticky 선택**을 기본값으로 권장

### 3.4 옵션 4 — 동적 큐: hctx 오버프로비저닝 + per-hctx activate/deactivate (G3의 답)

**아이디어**: `blk_mq_update_nr_hw_queues()`(B6)의 전체 freeze를 피하기 위해, **hctx 슬롯을 최대치로 미리 잡아두고** 개별 hctx를 켜고 끈다. A3(lazy tags)와 A2(INACTIVE drain)를 조합하면 blk-mq 기존 의미론 안에서 성립한다.

```
tagset 생성 시:  nr_hw_queues = 디바이스 협상 최대치 (예: 256)
                 → hctx 구조체 256개는 만들되,
                   비활성 hctx는 tags == NULL 휴면 (A3/F4) → request pool 미지불
                   → 선지불 메모리는 hctx 구조체 + ctxs 배열 + ctx_map 뿐 (§5.2)
```

**신규 API 2개 (제안)**

```c
/* 활성화: 휴면 hctx를 라우팅에 편입 */
int blk_mq_activate_hw_queue(struct request_queue *q, unsigned int idx,
                             const struct cpumask *cpus);
/* 비활성화: 라우팅에서 제외 + drain + 자원 반납 */
int blk_mq_deactivate_hw_queue(struct request_queue *q, unsigned int idx);
```

**deactivate 순서 — 이 순서가 정확성의 전부**

```
① set->map[type].mq_map[cpu] 를 생존 hctx로 재지정 (해당 idx를 가리키던 모든 cpu)
② 각 ctx->hctxs[type] 을 WRITE_ONCE 로 생존 hctx로 갱신     ← 실제 라우팅에 쓰이는 포인터 (B5)
③ smp_mb();  set_bit(BLK_MQ_S_INACTIVE, &hctx->state)       ← 기존 notify_offline과 동형 (blk-mq.c:3735-3736)
④ while (blk_mq_hctx_has_requests(hctx)) msleep(5);          ← in-flight drain (blk-mq.c:3677)
⑤ ctx->rq_lists[type] 잔여분을 hctx->dispatch 로 splice 후 run  ← notify_dead 패턴 (blk-mq.c:3795-3824)
⑥ __blk_mq_remove_cpuhp(hctx); 
   __blk_mq_free_map_and_rqs(set, idx);  hctx->tags = NULL;  hctx->nr_ctx = 0;
⑦ NVMe: adapter_delete_sq(sqid) → (CQ refcount 0이면) adapter_delete_cq(cqid)
```

**① ② 가 ③ 보다 먼저여야 하는 이유 (데드락 회피)**

`__blk_mq_alloc_requests`(blk-mq.c:576-587)의 재시도 루프는

```c
tag = blk_mq_get_tag(data);
if (tag == BLK_MQ_NO_TAG) { msleep(3); goto retry; }   /* retry: data->hctx 재계산 */
```

인데, `retry:`에서 다시 `blk_mq_map_queue(...)`(B5)로 hctx를 **재계산**한다. 매핑이 이미 생존 hctx를 가리키고 있어야 재시도가 **수렴**한다. ③을 먼저 하면 재시도가 계속 같은 INACTIVE hctx를 집어 **무한 루프**에 빠진다. (CPU hotplug 경로에서는 "태스크가 offline CPU에서 마이그레이션된다"는 사실이 이 역할을 대신하지만, 우리 경로엔 그런 보장이 없다.)

**stale read는 안전하다**: ②의 갱신을 놓친 CPU가 옛 hctx를 읽어도, 그 hctx는 ③에서 INACTIVE가 되므로 태그 할당이 실패하고 재시도에서 새 매핑을 읽는다. 즉 **②→③ 순서 + 메모리 배리어**만 지키면 lock-free 읽기가 성립한다.

**activate는 역순**: `__blk_mq_alloc_map_and_rqs` → `init_hctx`(드라이버가 nvmeq 바인딩) → `hctx->cpumask`/`nr_ctx`/`ctxs[]`/`index_hw` 세팅 → `sbitmap_resize(&hctx->ctx_map, nr_ctx)` → `__blk_mq_add_cpuhp` → sysfs/debugfs 등록 → `clear_bit(BLK_MQ_S_INACTIVE)`.

**미해결 난점 4가지 (설계 시 반드시 처리)**

| 난점 | 내용 | 대응 |
|------|------|------|
| **엘리베이터** | `hctx->sched_tags`가 hctx별 (`linux/block/blk-mq-sched.c:638-640`). `elevator_tags`는 `nr_hw_queues` 고정 배열(`blk_mq_alloc_sched_tags`, blk-mq-sched.c:504) | 오버프로비저닝 시 sched_tags도 최대치로 선할당(메모리 부담↑) 하거나, activate 시 개별 할당 경로 신설. **1차 범위에서는 `elevator=none` 강제**를 권장 |
| **sysfs/debugfs** | `blk_mq_sysfs_register_hctxs()`(blk-mq-sysfs.c:281), `blk_mq_debugfs_register_hctxs()`는 전체 루프 | 개별 함수 `blk_mq_register_hctx()`(blk-mq-sysfs.c:159), `blk_mq_debugfs_register_hctx()`(blk-mq-debugfs.c:672)가 **이미 존재** → 그대로 호출하면 됨 |
| **`hctx->tags == NULL` 방어** | 휴면 hctx를 참조하는 경로(통계 iteration, `blk_mq_tagset_busy_iter`, poll)가 NULL 역참조 가능 | B3에서 이미 `tags=NULL` 상태가 존재하므로 기존 방어가 어느 정도 있음. `blk_mq_queue_tag_busy_iter`(blk-mq-tag.c) 전수 점검 필요 |
| **q->nr_hw_queues 의미** | 오버프로비저닝하면 `nr_hw_queues`가 "실제 활성 큐 수"와 달라짐 → `/sys/block/*/mq/` 엔트리 수, `blk_mq_poll`의 `q->queue_hw_ctx[cookie]` 인덱싱 | `nr_active_hw_queues` 별도 카운터 도입, sysfs는 활성 것만 노출 |

---

## 4. N:1 SQ:CQ 구현 상세 (옵션과 무관한 공통 작업)

### 4.1 자료구조 — SQ/CQ 분리

현재 `struct nvme_queue`(pci.c:365-394)는 SQ와 CQ를 한 객체에 담고 `qid` 하나로 둘 다 가리킨다.

두 가지 접근:

| 방식 | 내용 | 평가 |
|------|------|------|
| **(a) 최소 변경**: `cqid` 필드 + `NVMEQ_CQ_OWNER` 플래그 추가 | 객체는 그대로, "이 객체가 CQ의 주인인가"를 플래그로 구분. CQ 자원(cqes/cq_dma_addr/cq_head/cq_phase/cq_vector)은 owner만 유효 | 침습 작음. 다만 non-owner 객체의 CQ 필드가 죽은 필드로 남아 혼동 유발 |
| **(b) 정공법**: `struct nvme_sq` / `struct nvme_cq` 분리, SQ가 `struct nvme_cq *cq` 참조 | 소유권이 타입으로 표현됨. 수명주기(CQ refcount)가 자연스러움 | 변경 범위 큼 (pci.c 전반의 `nvmeq` 사용처 ~50곳). 그러나 장기적으로 옳음 |

**권장**: (b). 단 **Phase 1에서 1:1을 유지한 채 리팩터만** 수행해 회귀가 없음을 먼저 증명하고, Phase 2에서 N:1을 켠다. (a)로 시작하면 N7/N8을 계속 우회하게 되어 부채가 쌓인다.

### 4.2 도어벨 분리 (N7)

```c
/* 현재 (pci.c:2156) — 한 객체가 SQ/CQ 도어벨을 모두 소유 */
nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];
/* CQ 도어벨은 q_db + db_stride 로 접근 (pci.c:1546 `nvme_ring_cq_doorbell`, writel은 `:1552`) */
writel(head, nvmeq->q_db + nvmeq->dev->db_stride);

/* N:1 후 */
sq->sq_db = &dev->dbs[sqid * 2 * dev->db_stride];              /* SQ Tail Doorbell */
cq->cq_db = &dev->dbs[cqid * 2 * dev->db_stride + dev->db_stride]; /* CQ Head Doorbell */
```

`dbbuf`(shadow doorbell, pci.c:1544-1550의 `dbbuf_cq_db`/`dbbuf_cq_ei`)도 같은 기준으로 SQ/CQ 소유를 갈라야 한다.

### 4.3 완료 역참조 (N8) — 3가지 선택지 비교

CQE가 공유 CQ에 도착했을 때 `command_id`로 request를 찾아야 하는데, `command_id`(=blk-mq 태그)의 **유일성 범위**가 무엇이냐가 관건이다.

| 선택지 | 방법 | 변경량 | 런타임 비용 | 제약 |
|--------|------|--------|------------|------|
| **(1) hctx 공유 (옵션 1)** | 한 hctx가 SQ들을 소유 → 그 SQ들이 같은 `hctx->tags`를 씀 → `nvme_queue_tagset()`이 원래부터 정답 | **0줄** | 없음 | 옵션 1 구조에서만 성립. 큐별 depth 분리 불가 |
| **(2) `cqe->sq_id`로 역조회** | `tags = dev->tagset.tags[le16_to_cpu(cqe->sq_id) - 1]` | 수 줄 | CQE당 간접 참조 1회 | SQ↔hctx가 1:1이어야 함(옵션 2/3 전제). 삭제 중인 SQ의 stale CQE 방어 필요 → [[nvme-queue-mgmt-design]] 부록 E |
| **(3) `BLK_MQ_F_TAG_HCTX_SHARED`** | tagset 전체가 `set->shared_tags` 하나 (blk-mq.c:4687-4691) → `set->tags[i]`가 전부 동일 객체(4131-4132) → `nvme_queue_tagset()`이 어느 qid로 불려도 같은 tags 반환 | **0줄** (플래그만) | `hctx_may_queue()`(blk-mq.h:403)의 per-hctx 공평 분배 오버헤드 | **depth가 전체 공유** → 큐를 늘려도 동시 in-flight 총량이 안 늘어남. G1의 성능 동기와 정면 충돌 |

**권장**: 옵션 1이면 (1) — 무료. 옵션 2/3이면 (2). (3)은 "큐를 늘리는 이유가 동시성이 아니라 격리"인 경우에만.

### 4.4 인터럽트 (N5, N10)

- `queue_request_irq()`(pci.c:2169)는 **CQ owner 1회만** 호출. 핸들러 `dev_id`는 CQ 객체
- `nvme_create_queue()`의 `vector = qid`(pci.c:2233)를 `vector = cq->vector`로
- `nvme_pci_map_queues()`(pci.c:686)의 `blk_mq_map_hw_queues(map, dev->dev, offset)`는 "큐 index → 벡터 index" 동일 전제(`blk-mq-cpumap.c:119`)이므로 **N:1에서는 쓸 수 없다**. CQ 벡터의 affinity를 그 CQ에 매달린 SQ들에 전파하는 커스텀 매퍼 필요:

```
for each CQ c:
    mask = pci_irq_get_affinity(pdev, c->vector)
    for each SQ s attached to c:
        for_each_cpu(cpu, mask): map->mq_map[cpu] = hctx_index_of(s)     /* ← 여러 SQ가 같은 mask를 두고 경쟁 */
```

여기서 "같은 mask 안의 CPU들에게 여러 SQ를 어떻게 나눠줄 것인가"가 바로 **ctx ≤ hctx 문제 그 자체**로 환원된다. → 옵션 1이면 SQ 선택을 드라이버가 하므로 이 문제가 안 생기고, 옵션 2/3이면 클래스/슬롯 축으로 나눈다.

### 4.5 협상 분리 (N3)

NVMe 스펙 Set Features FID 07h (Number of Queues):
- Command Dword 11: `NSQR`(bits 15:0), `NCQR`(bits 31:16) — **각각 요청 개수 - 1**
- Completion Dword 0: `NSQA`(bits 15:0), `NCQA`(bits 31:16) — **각각 할당 개수 - 1**

즉 스펙은 **처음부터 SQ와 CQ 개수를 독립 협상**하도록 설계돼 있다. vanilla가 둘을 같게 요청하고 min으로 뭉갤 뿐이다 (core.c:1747, 1762).

```c
/* 제안: 분리 버전 신설, 기존 시그니처는 래퍼로 유지 */
int nvme_set_queue_count_split(struct nvme_ctrl *ctrl, int *nr_sq, int *nr_cq)
{
        u32 q_count = (*nr_sq - 1) | ((*nr_cq - 1) << 16);
        u32 result;
        int status = nvme_set_features(ctrl, NVME_FEAT_NUM_QUEUES, q_count,
                                       NULL, 0, &result);
        if (status < 0 || status == NVME_SC_HOST_PATH_ERROR)
                return status;
        if (status > 0) { *nr_sq = *nr_cq = 0; return 0; }
        *nr_sq = min(*nr_sq, (int)(result & 0xffff) + 1);
        *nr_cq = min(*nr_cq, (int)(result >> 16) + 1);
        return 0;
}
```

**주의**: 이 협상은 컨트롤러 리셋 이후 **1회만** 유효하다(스펙: Set Features NUMQ는 리셋 후 한 번만). 따라서 **동적 큐 생성(G3)이 늘릴 수 있는 상한은 부팅 시 협상한 값**이다. → 오버프로비저닝 협상(최대치로 요청해두고 실제 생성은 나중에)이 G3의 전제 조건이다. 이는 옵션 4의 "hctx 슬롯 오버프로비저닝"과 정확히 짝을 이룬다.

---

## 5. 정량 검토

### 5.1 큐 수 상한은 실제로 어디까지인가

| 상한 요소 | 값 | 근거 |
|-----------|-----|------|
| NVMe 스펙 큐 ID | 65535 | Set Features 07h의 NSQR/NCQR이 16비트 |
| MSI-X 테이블 | 2048 | PCIe 스펙 MSI-X Table Size 11비트 |
| **→ CQ 수 실질 상한** | **≤ 2048** | 인터럽트 방식 CQ는 벡터 1개씩 소비 |
| **→ SQ 수 실질 상한** | **65535** (N:1이면 벡터 무관) | ★ N:1의 존재 이유 |
| 도어벨 BAR | `NVME_REG_DBS + (n+1)*8*stride` | pci.c:2306-2309. 큐 1024개/stride 0 → ~12.3 KB, 여유 |
| `dev->queues[]` | 현재 CPU 수 | N2 — 수정 대상 |

**결론**: 하드웨어·스펙상 CPU 수는 어디에도 상한이 아니다. 상한을 만드는 건 오직 "CQ당 벡터 1개 + 벡터를 CPU에 뿌린다"는 **드라이버 정책**이다. N:1은 이 정책을 푸는 열쇠다.

### 5.2 메모리 비용

hctx 하나당 선지불(휴면 상태에서도) 비용:

```
sizeof(struct blk_mq_hw_ctx)                    ≈ 수백 B  (linux/include/linux/blk-mq.h:322)
+ hctx->ctxs = kmalloc_array(nr_cpu_ids, 8)      = 8 × nr_cpu_ids     (blk-mq.c:4050)
+ hctx->ctx_map = sbitmap(nr_cpu_ids)            ≈ nr_cpu_ids/8 + α   (blk-mq.c:4055)
+ blk_flush_queue (fq)                           ≈ sizeof(request)+cmd_size
```

CPU 128 기준 hctx당 대략 **1.5~2 KB**. 256개 오버프로비저닝 → **약 0.5 MB**. 감당 가능.

활성화 시 추가되는 request pool:

```
per-hctx = queue_depth × (sizeof(struct request) + set->cmd_size)
```

NVMe의 `cmd_size = sizeof(struct nvme_iod)`이고 `nvme_iod`는 `nvme_command`(64B) + `descriptors[NVME_MAX_NR_DESCRIPTORS]` + `dma_iova_state` ×2 등을 담는다 (pci.c의 `struct nvme_iod`). 정확한 값은 빌드 후 `pahole`로 확인해야 하지만 **request+iod ≈ 0.5 KB** 수준으로 잡으면:

| 큐 수 | depth | 총 request pool |
|-------|-------|----------------|
| 64 | 1023 | ≈ 32 MB |
| 256 | 1023 | ≈ 131 MB |
| 256 | 128 | ≈ 16 MB |
| 1024 | 128 | ≈ 65 MB |

**시사점**: 큐 수를 크게 늘리려면 **큐당 depth를 낮춰야** 한다. "CPU 수보다 많은 큐"의 목적이 격리/QoS라면 낮은 depth가 오히려 자연스럽다. 목적이 동시성이라면 총 태그 수는 어차피 디바이스 QD에 묶이므로 큐를 늘려도 이득이 없다 — **이 지점이 옵션 선택의 판단 기준**이다.

### 5.3 어떤 목표에 어떤 옵션이 맞는가

| 실제 동기 | 맞는 옵션 | 이유 |
|-----------|----------|------|
| WRR 우선순위별 SQ (NVMe Arbitration) | **옵션 1** | 우선순위는 SQ 속성. hctx를 나눌 이유 없음 |
| Namespace/tenant별 SQ 격리 | **옵션 1** (Tier B 병용) | [[nvme-queue-mgmt-design]] §9의 NS–SQ 바인딩과 결합 |
| 큐별 독립 depth / 독립 스케줄러 / 독립 통계 | **옵션 2+3** | 이것들은 전부 hctx 단위 속성 |
| 제출 경로 스케일링 (락 경합 완화) | **옵션 1** | `sq_lock`이 SQ별이므로 SQ만 나눠도 경합이 준다 (A4의 업스트림 주석이 말하는 바로 그것) |
| 런타임 큐 추가/삭제 무중단 | **옵션 4** | 필수 |
| 디바이스가 광고하는 큐를 전부 "보이게" | **옵션 2+3+4** | blk-mq에 노출하려면 hctx가 있어야 함 |

---

## 6. 권장 로드맵

| Phase | 내용 | 산출물 / 검증 | 위험 |
|-------|------|--------------|------|
| **0** | **계측 먼저.** v7.2에서 `nr_hw_queues > nr_cpus`를 강제 주입해 §2의 실패 시나리오(B2 덮어쓰기, B3 죽은 hctx)를 **실측 확인** | drgn 스크립트로 `set->map[].mq_map[]`, `hctx->nr_ctx`, `hctx->tags` 덤프 | 없음 |
| **1** | **1:1 유지 리팩터**: `struct nvme_sq`/`nvme_cq` 분리 (§4.1(b)), 도어벨 분리 (§4.2). 동작은 완전 동일해야 함 | fio 회귀 무결 + KASAN | 낮음. 범위가 커서 시간 소요 |
| **2** | **협상 분리** (§4.5) + `dev->queues[]` 사이징 해제 (N1/N2) + `max_qid`를 벡터에서 분리 (N4/N5) | `nvme get-feature -f 0x7`로 NSQA≠NCQA 확인 | 중. 리셋 경로 재검증 필요 |
| **3** | **N:1 활성화** (N6) + IRQ 1회 등록 + CQ refcount + 완료 역참조 (§4.3 선택지 (1)) → **옵션 1 구현** | QEMU `max_ioqpairs` 크게 + `smp=8`, SQ 4개 → CQ 1개 매핑 후 fio | 중. stale CQE 방어는 [[nvme-queue-mgmt-design]] 부록 E 준수 |
| **4** | **G1·G2 달성 확인**. 여기까지 **blk-mq 무수정** | 큐 수 > CPU 수 상태에서 IO 정상 + 각 SQ 사용 카운터 확인 | — |
| **5** | **동적 add/remove** — 옵션 4의 activate/deactivate를 **SQ 단위**로 (hctx 개수 불변이므로 freeze 전무) | 생성↔삭제 루프 + fio 동시 부하, KASAN | 중. drain 순서(§3.4 ①②③) 준수가 핵심 |
| **6** | **판단 지점**: 옵션 1로 부족한가? (큐별 depth/스케줄러/통계가 실제로 필요한가?) 계측으로 답한다 | — | — |
| **7** | 필요하다면 **옵션 2 자료구조** (`HCTX_MAX_TYPES` 일반화) + **옵션 3 선택자** + **옵션 4를 hctx 단위로 확장** | blk-mq 단위 테스트, 타 드라이버(virtio-blk/scsi) 회귀 | **높음**. 코어 변경이라 전 블록 디바이스 영향 |

**Phase 4에서 멈출 수 있는지가 이 프로젝트의 비용을 가른다.** Phase 7은 Phase 6의 계측 결과가 요구할 때만 착수한다.

---

## 7. 검증 계획

### 7.1 환경

- QEMU NVMe는 **N:1을 이미 지원한다**: `nvme_init_sq(sq, n, prp1, sqid, cqid, ...)`가 임의 `cqid`를 받고 `nvme_check_cqid()`로 CQ 존재를 검증한다 ([[06_qemu_nvme_device_emulation]] 참조 — 해당 문서 1007-1016행 인용). 완료 시 `n->cq[sq->cqid]`로 라우팅하므로 **호스트 드라이버만 고치면 검증 가능**
- 큐 수 > CPU 수 상황 만들기: `-smp 8 -device nvme,...,max_ioqpairs=64` → 광고 64 vs CPU 8
- 기존 자산 활용: telemetryd의 QEMU 8-queue 표준 스크립트, `deep/scripts/drgn/` (커널 6.17 기준이므로 **v7.2 대상 구조체 오프셋 재확인 필요**)

### 7.2 항목별

| 목표 | 검증 방법 | 통과 기준 |
|------|----------|----------|
| G1 | `nvme_setup_io_queues` 후 `dev->ctrl.queue_count` 덤프 | 광고치(64)까지 생성, CPU 수(8)에 안 묶임 |
| G1 | drgn으로 `set->map[t].mq_map[cpu]` 전수 덤프 | 고아 hctx(=`nr_ctx==0`) **0개** |
| G2 | drgn으로 각 SQ의 `cqid` 덤프 + `nvme get-feature -f 0x7` | SQ 4개가 같은 cqid, NSQA≠NCQA |
| G2 | 서로 다른 SQ로 제출한 IO의 완료가 공유 CQ에서 올바른 request로 회수 | 오조회 0, `invalid id ... completed on queue` 경고 0 (pci.c:1584) |
| G2 | 인터럽트 카운트 | `/proc/interrupts`에서 CQ 개수만큼만 nvme 벡터 존재 |
| G3 | 큐 생성/삭제 중 fio 지속 | **throughput 0 구간 없음** (= freeze 미발생). `blk_mq_update_nr_hw_queues` 호출 0회를 ftrace로 확인 |
| G3 | 삭제 순서 위반 테스트 | ③을 ①②보다 먼저 하면 재시도 무한루프가 재현되는지 (음성 테스트) |
| 안정성 | 생성↔삭제 1000회 루프 + fio + KASAN | UAF/leak 0 |
| 안정성 | 컨트롤러 리셋과 동시 실행 | `shutdown_lock` 경합 정상, hang 없음 |

### 7.3 계측 (Phase 6 판단 근거)

- SQ별 제출 카운터를 `bpftrace`로 (`nvme_queue_rq`의 SQ id) → fan-out 정책이 실제로 분산되는지
- `sq_lock` 경합: `lock_stat` 또는 `perf lock`
- 큐 수 × depth 조합별 IOPS/지연 곡선 → "큐를 늘려서 이득이 있는가"의 직답

---

## 8. 미해결 질문 (설계 확정 전 답이 필요)

1. **큐를 늘리는 진짜 동기가 무엇인가?** — 격리/QoS(→옵션 1로 충분)인가, 제출 경로 스케일링(→옵션 1로 충분)인가, 큐별 독립 depth/스케줄러(→옵션 2/3 필수)인가. §5.3 표의 어느 행인지 확정해야 Phase 7 착수 여부가 정해진다.
2. **대상 컨트롤러가 실제로 NSQA > NCQA를 반환하는가?** — 많은 컨트롤러가 NSQR/NCQR을 같은 값으로만 취급한다. 실물/QEMU에서 먼저 확인.
3. **엘리베이터를 쓸 것인가?** — 옵션 4에서 `sched_tags` 동적 할당은 난도가 높다. `none` 강제로 시작할 수 있는지.
4. **업스트림 지향인가, 사내 포크인가?** — 옵션 2(`HCTX_MAX_TYPES` 상향)는 전 블록 디바이스에 메모리를 부과하므로 업스트림 수용 난도가 매우 높다. 사내 포크라면 부담이 다르다.
5. **동적 큐의 상한은 부팅 시 협상값에 묶인다** (§4.5 주의) — 오버프로비저닝 협상을 기본으로 할 것인지, 아니면 큐 확장 시 컨트롤러 리셋을 감수할 것인지.

---

## 9. 코드 참조 인덱스 (v7.2)

### NVMe
| 대상 | 위치 |
|------|------|
| `nvme_max_io_queues()` | `linux/drivers/nvme/host/pci.c:2932` |
| `nvme_setup_io_queues()` | `linux/drivers/nvme/host/pci.c:2944` |
| `dev->max_qid` 결정 | `linux/drivers/nvme/host/pci.c:3040` |
| 업스트림 "more queues than vectors" 주석 | `linux/drivers/nvme/host/pci.c:3042-3047` |
| `nvme_setup_irqs()` / `nvme_calc_irq_sets()` | `linux/drivers/nvme/host/pci.c:2894` / `2859` |
| `adapter_alloc_cq()` / `adapter_alloc_sq()` | `linux/drivers/nvme/host/pci.c:1750` / `1773` (cqid 하드코딩 `:1797`) |
| `nvme_alloc_queue()` (도어벨 계산) | `linux/drivers/nvme/host/pci.c:2134` (`:2156`) |
| `nvme_create_queue()` (vector=qid) | `linux/drivers/nvme/host/pci.c:2220` (`:2233`) |
| `nvme_queue_tagset()` | `linux/drivers/nvme/host/pci.c:1555` |
| `nvme_handle_cqe()` | `linux/drivers/nvme/host/pci.c:1562` |
| `nvme_init_hctx()` (qid = idx+1) | `linux/drivers/nvme/host/pci.c:660` |
| `nvme_pci_map_queues()` | `linux/drivers/nvme/host/pci.c:686` |
| `dev->nr_allocated_queues` | `linux/drivers/nvme/host/pci.c:3691` |
| `nvme_set_queue_count()` | `linux/drivers/nvme/host/core.c:1745` (min 뭉개기 `:1762`) |
| `io_queue_count_set()` (모듈 파라미터 CPU 상한) | `linux/drivers/nvme/host/pci.c:249` (`:255`) |
| `nvme_pci_nr_maps()` | `linux/drivers/nvme/host/pci.c:3159` |
| `write_queues` / `poll_queues` 기본값 0 | `linux/drivers/nvme/host/pci.c:265` / `271` |
| `nvme_alloc_io_tag_set()` | `linux/drivers/nvme/host/core.c:4990` (`nr_hw_queues` 설정 `:5013`) |

### blk-mq
| 대상 | 위치 |
|------|------|
| `nr_cpu_ids` 캡 (nr_maps==1 한정) | `linux/block/blk-mq.c:4876`, `5119` |
| `blk_mq_map_queues()` (덮어쓰기) | `linux/block/blk-mq-cpumap.c:59-76` |
| `group_cpus_evenly()` 그룹 수 클램프 | `linux/lib/group_cpus.c:254` |
| `blk_mq_map_swqueue()` | `linux/block/blk-mq.c:4162` (lazy tags `:4191`, 죽은 hctx `:4237`) |
| `struct blk_mq_ctx` (HCTX_MAX_TYPES 배열) | `linux/block/blk-mq.h:19-32` |
| `blk_mq_map_queue()` (유일 라우팅 지점) | `linux/block/blk-mq.h:109-113` |
| `__blk_mq_alloc_requests()` (retry 루프) | `linux/block/blk-mq.c:537` (`:553`, `:576-587`) |
| `blk_mq_get_tag()` INACTIVE 처리 | `linux/block/blk-mq-tag.c:224-231` |
| `blk_mq_hctx_has_requests()` | `linux/block/blk-mq.c:3677` |
| `blk_mq_hctx_notify_offline()` | `linux/block/blk-mq.c:3719` |
| `blk_mq_hctx_notify_dead()` | `linux/block/blk-mq.c:3795` |
| `__blk_mq_update_nr_hw_queues()` | `linux/block/blk-mq.c:5106` |
| `__blk_mq_realloc_hw_ctxs()` | `linux/block/blk-mq.c:4539` |
| `blk_mq_hctx_mark_pending()` / `blk_mq_flush_busy_ctxs()` | `linux/block/blk-mq.c:71` / `1786` |
| `BLK_MQ_F_TAG_HCTX_SHARED` / `shared_tags` | `linux/include/linux/blk-mq.h:697`, `linux/block/blk-mq.c:4687` |
| `hctx_may_queue()` | `linux/block/blk-mq.h:403` |
| per-hctx sysfs/debugfs 등록 | `linux/block/blk-mq-sysfs.c:159`, `linux/block/blk-mq-debugfs.c:672` |
| `hctx->sched_tags` 배분 | `linux/block/blk-mq-sched.c:638-640` |
