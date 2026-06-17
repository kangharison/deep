# NVMe Namespace(NVM Set)별 SQ 격리 커스텀 드라이버 — 설계

> 기준 코드: `sources/linux/drivers/nvme/host/{pci.c, core.c, ioctl.c, multipath.c}`, blk-mq(`block/blk-mq.c`)
> 기준 스펙: NVMe Base Spec — Create I/O SQ/CQ, NVM Sets, Identify, Arbitration(WRR)
> 작성: 2026-06-09 · 갱신: 2026-06-17 · 상태: 설계 확정 + 코드 구성안(§19~§27) + 보조 함수 전체(§28) + **★ endurance_group 기반 개정 정본(§29)**
>
> **읽는 순서 주의**: 앵커 자료구조·"생성→큐맵 구성" 타이밍은 **§29가 정본**이다(§7·§9·§20·§22~24·§28의 해당 부분 대체). §29 외 절은 격리 원리·격리 검토·디먹스·reset 등 변하지 않는 설계.

**한 줄 요약**: NVMe IO SQ 를 **NVM Set(=namespace) 단위로 분리**해 namespace 별 전용 tagset 에 묶어 격리한다. 큐 생성은 **ioctl 로 NVMSETID 를 실은 Create I/O SQ** 를 발행하는 커스텀 드라이버가 담당하며, blk-mq 데이터패스(`nvme_queue_rq`)는 그대로 재사용한다. 이후 thread 격리는 cgroup 으로 얹는다.

---

## 1. 목표와 범위

- **목표**: namespace 별로 IO 큐(SQ)를 분리하고 tagset/driver SQ 를 모두 분리 → noisy-neighbor 격리. 추후 application thread 를 특정 namespace 큐에만 접근시키는 QoS 격리.
- **구현 형태**: 커널 측 커스텀 드라이버(mainline `nvme-pci` fork/확장).
- **제어 경로 = private path ioctl**: `create io queue` / `delete io queue` 는 **드라이버 전용(private) ioctl 인터페이스**로 전달(표준 nvme passthrough 가 아님). create 시 **NVMSETID 사용 여부를 인자로** 받음.
- **★ NVMSETID all-or-nothing(이진 상태)**: create io queue 는 NVMSETID 를 쓰거나 안 쓰거나 둘 중 하나인데, **쓰기로 했으면 모든 IO 큐가 NVMSETID 를 쓴다(혼용 없음)**. 따라서 시스템은 두 상태 중 하나다:
  - **State D(default)**: NVMSETID 없이 생성 → 모든 ns 가 `ctrl->tagset`(mainline 등가).
  - **State P(partitioned)**: 전부 NVMSETID 로 생성 → 모든 ns 가 자기 set 의 `ns->tagset`. `ctrl->tagset` 은 dormant(ns 0).
- **분할 키**: namespace 직접이 아니라 **NVM Set ID**. namespace 는 NVM Set 에 소속(`Identify Namespace` 의 NVMSETID).
- **데이터패스**: blk-mq 제출/완료 경로 100% 재사용. 신규 작업은 "tagset 다중화 + 그룹별 매핑 + ns 바인딩 + private ioctl 제어면"뿐.
- **범위 밖**: 유저스페이스(SPDK/VFIO) poll-mode.

### 상태 전이 (이진) — 4전이 전체

```
State D (default)                       State P (partitioned)
 모든 ns → ctrl->tagset                  모든 ns → ns->tagset(group[k])
 (mainline 등가)                         ctrl->tagset = dormant(SQ 0)

  ──[private ioctl: NVMSETID io 큐 생성 + ns rescan]──►  (D→P)
  ◄──[nvme_reset_work: device disable → NVMSETID 없이 재생성]── (P→D)
   ⟳ D→D (자기)                              ⟳ P→P (자기)
```

**상태 판정 기준**: "NVMSETID 로 만든 활성 그룹(per-set 큐)이 하나라도 있는가" → 있으면 **P**, 없으면 **D**.

| 전이 | 트리거 | 동작 | 결과 |
|------|--------|------|------|
| **D→D** | (a) NVMSETID 없는 io 큐 create/delete (b) `nvme_reset_work` | (a) `ctrl->tagset` `blk_mq_update_nr_hw_queues`(mainline 큐 리사이즈) (b) mainline native reset(§12.1 Case1) | D 유지 |
| **D→P** | NVMSETID io 큐 첫 생성 + ns rescan | per-set tagset 생성, ns→`ns->tagset` | P |
| **P→P** | (a) 새 NVMSETID 그룹 신설 (b) 기존 그룹 grow/shrink (c) 그룹 삭제(≥1 잔존) | §12 lifecycle [A]/[B]/[C]/[D]. **모두 NVMSETID 사용** | P 유지 |
| **P→D** | (a) `nvme_reset_work` (b) 명시적 전체 teardown 후 비-NVMSETID 재생성 | (a) §12.1 Option B (b) §13 역방향 | D |

**자기 전이(D→D / P→P)에서 지켜야 할 고려사항**:
1. **reset 은 항상 D 로 귀결** — State P 에서 `nvme_reset_work` 가 일어나면 **P→D**(P→P 아님). reset 은 분할을 보존하지 않는다(Option B). 즉 **P→P 트리거에 reset 은 없다**.
2. **all-or-nothing 불변식 유지** — State P 안에서 **비-NVMSETID 큐 생성 시도는 드라이버가 거부**(P→mixed 불가). State D 안에서 NVMSETID 큐를 섞어 만들면 그건 D→P 진입이지 D→D 아님.
3. **★ State P 의 selector fallback 금지(엣지)** — P 에서 `ctrl->tagset` 은 **dormant(SQ 0)**. 그런데 NVMSETID=k 인 ns 가 붙는데 group[k] 가 아직 없으면, selector 의 "fallback → ctrl->tagset" 이 **dormant tagset 에 바인딩 → IO 불가**가 된다. 따라서 **State P 에서는 fallback 을 ctrl->tagset 으로 보내지 말고 pending(rendezvous, 그룹 생성까지 대기)** 해야 한다. 판정식은 "ctrl->tagset 이 active(SQ>0)인가"로 통일:
   ```c
   g = find_group(ns->nvmset_id);
   if (g && g->tagset_ready)        ts = &g->tagset;        /* per-set */
   else if (ctrl_tagset_active())   ts = ctrl->tagset;      /* State D fallback (valid) */
   else                             defer_ns(ns);           /* State P: 그룹 대기 (pending) */
   ```
4. **마지막 그룹 삭제(P)** — 활성 그룹이 0 이 되면 무큐 중간상태. 다음 create 가 NVMSETID 유무로 D/P 를 다시 결정. (reset 없이 P→D 로 가는 경로 = §13 역방향.)
5. **D→D 큐 리사이즈** — State D 에서 NVMSETID 없는 큐 add/remove 는 mainline `nvme_pci_update_nr_queues` 그대로(ctrl->tagset 재매핑). 분할 로직 개입 없음.

---

## 2. 용어

| 용어 | 의미 |
|------|------|
| **SQ / CQ** | Submission/Completion Queue. mainline `struct nvme_queue` 1개가 SQ/CQ 쌍 + 전용 MSI-X 벡터. |
| **Admin Queue (qid 0)** | Create I/O SQ/CQ, Identify, Set Features 등 관리 명령 통로. |
| **tagset** (`blk_mq_tag_set`) | blk-mq 의 tag(in-flight request) 풀 + hctx 집합. mainline 은 IO tagset 1개를 모든 ns 가 공유. **본 설계는 그룹별로 둔다.** |
| **hctx** (`blk_mq_hw_ctx`) | 하드웨어 큐 컨텍스트. `hctx ↔ nvme_queue(SQ)` 매핑. 기본 선택은 제출 CPU 기준. |
| **NVM Set** | 물리/논리 격리된 namespace 집합(QoS·미디어 격리 단위). NVMSETID 로 식별. device 레벨 구성. |
| **NVMSETID** | Create I/O SQ 명령 CDW12. SQ 를 NVM Set 에 연관시키는 컨트롤러 측 격리 hint. |
| **QPRIO** | Create I/O SQ CDW11[2:1]. WRR 중재 우선순위(Urgent/High/Medium/Low). |
| **CID** | Command Identifier. **한 SQ 내 유일**해야 완료 역매핑 가능. blk-mq 에선 `rq->tag` 파생. |

---

## 3. 현재 mainline 구조와 한계 (baseline)

```
nvme_dev (PCIe 컨트롤러 1개)
 ├─ admin_tagset ───────────► admin queue (qid 0)
 └─ tagset (IO용, "1개")  ──► nr_hw_queues = num_possible_cpus()
        hctx0..N               ▲ blk-mq 가 "제출 CPU" 기준으로 hctx 선택
          └ 각 hctx → nvme_queue(qid 1..N) = SQ/CQ + MSI-X

 namespaces: ns1, ns2, ns3 ─ 전부 dev->tagset 공유 (namespace 구분 없음)
```

호출 체인(제출): `submit_bio → blk-mq → ns->queue(=ctrl->tagset) → nvme_queue_rq() → SQ write → doorbell`

**한계**
1. 큐 선택이 **CPU 기준** — namespace/thread 가 큐를 못 고름.
2. **tagset 공유** — 모든 ns 가 같은 tag·SQ 풀 → noisy neighbor.
3. thread 소유권 개념 부재(blk-mq 는 CPU 에 큐를 묶음).

> mainline 은 이미 read/poll/default "타입별"로 큐를 쪼갠다(`HCTX_TYPE_*`). 본 설계는 그 발상을 **NVM Set별 tagset**으로 확장한다.

---

## 4. 설계 개요 — 핵심 결정

| # | 결정 | 이유 |
|---|------|------|
| 1 | **모델 A**: NVM Set별 tagset, ns 1:(1\|N) 바인딩 | "tagset/SQ 모두 분리" 목표에 정확히 부합 |
| 2 | **NVMSETID all-or-nothing(이진 상태 D/P)**: 혼용 없음. 공용 overflow 없음 | CID 충돌·커스텀 엔진·정합성 모호성 제거 (§1 상태 전이) |
| 3 | **blk-mq 데이터패스 재사용** | `nvme_queue_rq` 무수정, tagset 다중화만 추가 |
| 4 | **tagset 소유 = 영속 default + 일시 per-set** | probe 의 `ctrl->tagset` 을 기본 그룹으로 재활용, 침습 최소 |
| 5 | **provisioning = queue-first** | tagset 이 ns 스캔 전에 준비 → 바인딩 즉시(mainline 동일 패턴) |
| 6 | **분할 키 = NVMSETID**, Create I/O SQ CDW12 로 전달 | NVMe 표준 경로 |
| 7 | **thread 격리 = cpuset cgroup** (후속) | 커스텀 제출 엔진 불필요 |

> **버린 대안**: ① 공용 tagset + thread 소유 전용 SQ(passthrough) → 커스텀 제출/완료/CID/DMA 엔진 + 정합성 위험으로 폐기. ② probe 단일 tagset 제거 후 전량 ioctl 할당 → `ctrl->tagset` 재활용(결정 4)이 더 단순.

```
nvme_dev
 ├─ admin queue (qid 0)
 ├─ ctrl->tagset  ............... 기본 tagset. 영속(probe 생성). State D 에서 active / State P 에서 dormant.
 ├─ group[1].tagset (NVMSETID 1)  per-set. 일시(private ioctl 로 생성, reset 시 소멸).
 └─ group[2].tagset (NVMSETID 2)  per-set. 일시.

 State D: 모든 ns → ctrl->tagset            (NVMSETID 미사용)
 State P: 모든 ns → group[k].tagset         (NVMSETID 전면 사용, ctrl->tagset dormant)
```

---

## 5. tagset 소유 모델 — 영속 default + 일시 per-set

- **`ctrl->tagset` = 기본 그룹 = NVMSETID 0.** probe(`pci.c:3136/3777`, `nvme_alloc_io_tag_set`)에서 생성되어 **영속**. NVM Set 미소속 ns 의 home, 그리고 "원복된 기본 상태"의 home.
- **`group[k].tagset` (k≥1) = per-set 그룹.** `create io queue (NVMSETID=k)` ioctl 로 생성되고, 전체 큐 삭제(원복) 시 `free_tag_set` 으로 **소멸하는 일시 오버레이**.
- **`ns->tagset` 은 실체 소유가 아니라 selector** — `nvmset_id ? group[id].tagset : ctrl->tagset`.

```c
/* nvme_alloc_ns, core.c:4153 (변경점) — 상태 인지 selector (§1 자기전이 #3) */
struct nvmset_group *g = find_group(ns->nvmset_id);
if (g && g->tagset_ready)            ts = &g->tagset;     /* per-set (일시) */
else if (ctrl_tagset_active())       ts = ctrl->tagset;  /* State D: 유효한 fallback */
else                                 { defer_ns(ns); return; } /* State P: 그룹 대기(pending) */
disk = blk_mq_alloc_disk(ts, &lim, ns);
```
> 단순화하면 `nvmset_id ? group : ctrl->tagset` 이나, **State P 에서 group 미존재 시 dormant ctrl->tagset 로 떨어지면 IO 불가**(§1 자기전이 #3) → "ctrl->tagset active 여부"로 분기해 pending 처리.

**dormant 주의**: 분할 상태(State1)에서 `ctrl->tagset` 은 활성 ns 0 + SQ 0 으로 dormant 가 된다. 구조체는 살리되 **hctx→nvme_queue 가 삭제된 SQ 를 가리키지 않게 quiesce/정리**한다. 이는 mainline reset 흐름(`nvme_dev_disable` 가 tagset 유지·SQ 만 재생성 후 `blk_mq_update_nr_hw_queues`)과 동일 패턴이라 검증된 길.

---

## 6. 핵심 불변식 (정확성 근거)

1. **SQ 전속성** — 모든 IO SQ 는 정확히 하나의 그룹(tagset)에 속한다. 한 물리 SQ 를 두 tagset 이 공유하지 않는다.
2. **namespace 전속성** — 모든 ns 는 정확히 하나의 그룹(기본 ∪ per-set 중 하나)에 바인딩.
3. **CID 유일성 자동 성립** — (1)에 의해 SQ 는 단일 tagset 의 tag 만 받음 → `rq->tag` 파생 CID 가 그 SQ 안에서 유일. **커스텀 CID 관리 불필요.**
4. **라우팅 전사성** — `request → rq->q(ns) → tagset → hctx → SQ` 가 모호함 없이 결정.
5. **완료 독립성** — 각 SQ 의 CQ 가 자기 MSI-X IRQ 를 가짐 → 그룹 간 완료 경로 독립.

---

## 7. 핵심 자료구조

> **⚠️ §29로 개정됨**: 앵커 구조체가 `nvmset_group`(dev 직접 소유) → **`endurance_group`(ENDGRP 컨테이너) → `nvmset_entry`(NVM Set별 tagset)** 계층으로 바뀌고, SQ 식별이 포인터 배열 `sqs[]` → **`sqid` 목록**으로, tagset 생성 시점이 "첫 ns lazy" → **"rescan 때 sqid 테이블로 일괄 구성"**으로 변경. 이하 §7~§9·§20·§22~§24·§28의 해당 부분은 §29가 정본이다.

```c
/* 개념 스케치 — 실제 필드/타입은 구현 시 확정 */

struct nvmset_group {                 /* per-set 격리 단위 (k>=1). NVMSETID 0 = ctrl->tagset(별도) */
    u16                   nvmset_id;  /* 이 그룹의 NVM Set ID (>=1) */
    struct blk_mq_tag_set tagset;     /* 이 그룹 전용 IO tagset (일시 생성/소멸) */
    struct nvme_queue   **sqs;        /* 이 그룹에 속한 SQ들 (mainline 구조 재사용) */
    unsigned int          nr_sqs;     /* = tagset.nr_hw_queues. CSQ_CREATE 마다 누적 */
    bool                  tagset_ready;/* lazy 생성 가드 */
    enum csq_prio         qprio;      /* (선택) WRR 우선순위 */
    struct list_head      ns_list;    /* 이 Set 에 소속된 namespace들 */
};

struct nvme_dev {                     /* 확장 */
    /* ... 기존 필드 (admin_tagset, dev->tagset=기본그룹 등 그대로) ... */
    struct nvmset_group  *groups;     /* nvmset_id(k>=1) → group */
    unsigned int          nr_groups;
};
```

- `struct nvme_ns` 는 **수정 없이** selector 가 고른 tagset 으로 `request_queue` 생성. (단 nvmset_id 보관 필드 1개는 필요.)
- `struct nvme_queue`(SQ 실체)는 그대로 재사용.

---

## 8. 제어 경로 — private ioctl + Create I/O SQ (NVMSETID)

제어면은 **드라이버 전용(private) ioctl** 이다(표준 nvme passthrough 아님). 최소 동작:
- `CSQ_CREATE {nvmset_id(opt), qsize, qprio}` — io 큐 생성. **nvmset_id 미지정이면 State D(ctrl->tagset), 지정이면 State P(per-set)**. all-or-nothing 이므로 한 세션 내 혼용 금지(드라이버가 거부).
- `CSQ_DELETE {qid|nvmset_id}` — io 큐/그룹 삭제.
- 큐 생애주기는 **반드시 이 private ioctl 경유**(§13 조건 3) — Create/Delete I/O SQ 발행과 tagset 생성/바인딩을 한 트랜잭션으로 묶어 bookkeeping 일관 보장.

### 8.1 큐 예산
- 부팅 시 `nvme_set_queue_count()` 로 IO 큐 총량 N·MSI-X 벡터 수 확인.
- 그룹들에 분배. 정책: 균등 / 가중치(hot set 에 더 많은 SQ) — 추후 택일.
- 제약: `Σ ki ≤ N ≤ 가용 벡터 수` (SQ 마다 CQ 벡터 1개 가정).

### 8.2 SQ 생성 (ioctl → Admin)
```
ioctl(fd, CSQ_CREATE, {nvmset_id, qsize, qprio})
   ▼ 드라이버
   1) CQ DMA 할당 + MSI-X 벡터 확보
   2) Admin: Create I/O Completion Queue (CQID 배정)
   3) SQ DMA 할당
   4) Admin: Create I/O Submission Queue
        ├─ CDW10: QSIZE[31:16] | QID[15:0]
        ├─ CDW11: CQID[31:16] | QPRIO[2:1] | PC[0]
        └─ CDW12: NVMSETID[15:0]  ◄── NVM Set ID 전달
   5) group[nvmset_id].sqs[nr_sqs++] 등록  (tagset 미생성 — 카운트만 누적)
```

> **QPRIO 주의**: WRR 은 `CAP.AMS` 광고 + `CC.AMS` 활성화 시에만 유효. 미지원 시 Round Robin 폴백.

### 8.3 tagset 매핑 콜백 — `.map_queues` / `.init_hctx` (한 쌍)

둘은 제출 경로의 한 쌍이다. **같은 로컬 인덱스 j → `group->sqs[j]` 규약을 공유**해야 제출↔완료가 정합한다.
```
제출: CPU c → mq_map[c]=j → hctx j → group->sqs[j] → SQ write
완료: 그 SQ → nvmeq->tags (§15)
```
- `set->driver_data = group` 으로 둬 두 콜백이 그룹에 접근. `set->nr_maps = 1`(phase1, DEFAULT만).
- `.init_hctx`(§15): 로컬 j → `group->sqs[j]`, `hctx->driver_data=nvmeq`, `nvmeq->tags=hctx->tags`.

**`.map_queues` 함정 — affinity 타일링 깨짐**: 그룹 SQ 는 전역 벡터의 부분집합(비연속 가능)이라, mainline 의 affinity 기반 `blk_mq_map_hw_queues` 를 그대로 쓰면 **그룹 벡터들의 affinity 가 전 CPU 를 못 덮어 일부 CPU 가 미매핑** → 그 CPU 가 ns 에 제출 불가(조용한 버그). mainline 단일 tagset 은 전 큐가 전 CPU 를 타일링하므로 빈틈이 없지만, 그룹 부분집합은 보장 못 함.

**Phase 1 — 라운드로빈으로 전 CPU 커버 보장** (affinity 의존 제거):
```c
static void nvmset_map_queues(struct blk_mq_tag_set *set) {
    struct nvmset_group *g = set->driver_data;
    struct blk_mq_queue_map *map = &set->map[HCTX_TYPE_DEFAULT];
    map->nr_queues    = g->nr_sqs;
    map->queue_offset = 0;
    blk_mq_map_queues(map);   /* CPU 0..nr_sqs-1 라운드로빈 → 전 CPU 커버, affinity 무관 */
}
```
→ 모든 online CPU 가 그룹 SQ 에 매핑됨(§11 "격리 자동" 유지). 비용: 제출-완료 지역성 약화(완료 IRQ 는 SQ 벡터 effective-affinity CPU 에 떨어지고 blk-mq `SAME_COMP` 가 IPI 로 제출 CPU 에 되돌림).

**Phase 2 최적화(후속)**: 그룹 벡터 affinity 를 전 CPU 타일링되게 배정(`group_cpus_evenly(g->nr_sqs)`) 후 affinity 기반 매핑 → 지역성 확보. per-group 벡터 affinity 배정 복잡도로 1단계 미포함.

| | Phase1 라운드로빈 | Phase2 affinity |
|--|------------------|-----------------|
| 전 CPU 커버 | ✅ 보장 | 타일링 배정 필요 |
| 제출-완료 지역성 | 약(IPI) | 강 |
| 복잡도 | 낮음(한 줄) | 높음 |

- **reset(P→D)**: per-set tagset 소멸 → ctrl->tagset 은 mainline `nvme_pci_map_queues` 그대로. per-set `.map_queues` 호출 안 됨.
- **CPU hotplug / grow·shrink**: blk-mq 가 `.map_queues` 재호출(또는 `blk_mq_update_nr_hw_queues`) → 라운드로빈 재계산. affinity 비의존이라 견고.

---

## 9. tagset 크기 확정과 **생성 시점**

`nr_hw_queues = group->nr_sqs` (1 SQ = 1 hw queue = 1 hctx). 따라서 "몇 개 생성됐는지"를 알아야 tagset 을 만들 수 있다. queue-first 라 타이밍은 자연 해결된다 — **각 create 호출은 누적만, 크기는 첫 ns 바인딩에서 확정.**

| tagset | **생성 시점** | 비고 |
|--------|-------------|------|
| `ctrl->tagset` (기본, NVMSETID 0) | **probe** (`pci.c:3136/3777`) | 영속. ns 등장 전부터 존재 |
| `group[k].tagset` (per-set) | **그 set 의 첫 ns 스캔/바인딩** (`nvme_alloc_ns`, `core.c:4153`) | lazy. queue-first 로 SQ 누적 완료 후 |

```
[ioctl] CSQ_CREATE(NVMSETID=k) × N    → SQ 생성 + group[k].nr_sqs++   (tagset 없음, 누적)
[admin] namespace create & attach(k)  → AEN/rescan
[scan]  nvme_alloc_ns (core.c:4153):
           if (!group[k].tagset_ready) {
               nvme_alloc_io_tag_set(&group[k].tagset, /*nr_hw_queues=*/ group[k].nr_sqs, ops);
               group[k].tagset_ready = true;          ★★ per-set tagset 생성 시점 ★★
           }
           blk_mq_alloc_disk(&group[k].tagset) → ns->queue
```

- **그룹당 1회 생성** — 같은 set 의 나머지 ns 는 그 tagset 공유(`tagset_ready` 가드).
- **queue-first 전제 의존** — ns 바인딩 후 SQ 추가 시 tagset 이 작게 잡힘 → `blk_mq_update_nr_hw_queues` grow 필요(§12).
- **대안(명시적 finalize)** — ns 전에 tagset 이 존재해야 한다면, "set k 큐 완료" ioctl 에서 생성. 단 신호 필요.
- **소멸** — 그 set 의 마지막 gendisk 제거 + 큐 삭제 시 `free_tag_set`(§13).

---

## 10. 매핑 & 데이터패스

```
namespace(NSID) ─[Identify Namespace 의 NVMSETID]─► NVM Set ID
   │ selector: nvmset_id ? group[id].tagset : ctrl->tagset
   ▼
tagset ─► hctx ─► SQ 그룹
```

- **제출**: `rq->q(=ns) → 그 ns 의 tagset → (CPU 기준) hctx → 그룹 SQ → nvme_queue_rq()`. mainline 코드 무변경.
- **완료**: 각 SQ 의 CQ 가 자기 IRQ 로 전달. mainline 그대로. **단** qid→tagset 역매핑은 §15 G4 수정 필요.
- **컨트롤러 vs SW**: 컨트롤러는 SQ 를 NSID 에 하드 바인딩하지 않음(명령의 NSID 가 대상 결정). **"ns→자기 그룹 SQ 로만"은 드라이버가 tagset 바인딩으로 SW 강제**, NVMSETID 는 그걸 컨트롤러 미디어 격리와 정렬.

---

## 11. namespace 격리는 cgroup 불필요 / thread 격리

### 핵심: namespace 격리는 자동 (cgroup·앱수정 불요)
라우팅 키가 CPU 가 아니라 **namespace** 라서, 결정이 2단계로 일어난다.

```
1단계 (그룹 결정)  : request → rq->q(=ns) → 바인딩된 tagset → 그 그룹 SQ 집합
                     └ 어느 CPU/thread 에서 제출하든 무조건 이 그룹. (tagset 바인딩이 강제)
2단계 (그룹 내 SQ) : CPU → hctx → 그룹 안의 SQ 하나. (그룹 밖으로 못 나감)
```
- `/dev/nvme0n1` 에 IO → 무조건 그 ns 의 그룹 SQ. blk-mq 기본 `.map_queues` 가 그룹의 `nr_hw_queues` 개 SQ 를 모든 online CPU 에 펼침. SQ<CPU 면 그룹 내 `sq_lock` 경합이 생기나 **그 경합도 그룹 안에 갇힘**.

### cgroup 의 역할 (축소)
| 목적 | 메커니즘 | cgroup |
|------|---------|--------|
| namespace → SQ 그룹 격리 | tagset 바인딩 | **불필요(자동)** |
| 그룹 **내부** thread→특정 SQ 핀 | cpuset (thread 를 SQ 의 CPU 에 핀) | 선택 |
| 앱이 타 namespace 접근 차단 | device 권한 / cgroup device ctrl | 접근 제어 |

thread 격리(후속): `cpuset` 으로 thread 그룹을 해당 set SQ 가 매핑된 CPU 에 핀. "thread 당 물리 SQ 1개" 엄격 보장이 필요할 때만 passthrough add-on 별도 검토(기본 미포함).

---

## 12. 동적 생성/삭제 (Lifecycle)

**가능. 그룹별 tagset 이라 동적 조작이 국소적**(한 set 재구성이 다른 set IO 를 멈추지 않음) — mainline 단일 tagset(전체 stall) 대비 핵심 이점.

| 계층 | primitive (mainline 재사용) |
|------|------|
| SQ/CQ | `adapter_alloc_cq/sq`, `adapter_delete_sq/cq`, `nvme_create_queue` |
| SQ 개수 변경 | `blk_mq_update_nr_hw_queues(&group.tagset, n)` |
| tagset 신설/해제 | `blk_mq_alloc_tag_set` / `blk_mq_free_tag_set` |
| ns attach/detach | `nvme_alloc_ns` / `nvme_ns_remove`, `add_disk`/`del_gendisk` |

```
[A] 그룹 신설 : CQ→SQ(NVMSETID) ··· (첫 ns 때 tagset 생성, §9)
[B] grow      : Create SQ → blk_mq_update_nr_hw_queues   (그 그룹만 quiesce)
[C] shrink    : drain → 축소 → Delete SQ→CQ → 벡터 반납
[D] 그룹 삭제 : 소속 ns 전부 del_gendisk → free_tag_set → SQ/CQ 삭제 → 그룹 free
```

**지킬 규칙**: ① Delete 는 **SQ→CQ 순** + in-flight drain 선행 ② tagset 해제는 묶인 gendisk 전부 제거 후 ③ MSI-X 벡터 회계 ④ **컨트롤러 reset 정책은 아래 §12.1(Option B)** ⑤ 생성/삭제 직렬화(control mutex) + `nvme_scan_work`/reset 조율.

### 12.1 컨트롤러 reset 정책 — Option B: baseline(ctrl->tagset) 복귀

**전제 정정**: `nvme_dev_disable` 가 지우는 것은 **하드웨어 큐(nvme_queue)+MSI-X 벡터**뿐. `blk_mq_tag_set` 객체와 gendisk/request_queue 는 소프트웨어라 **살아남는다**(mainline 도 `ctrl->tagset` 을 free 하지 않고 `blk_mq_update_nr_hw_queues` 로 재매핑, `pci.c:3133-3151`). 따라서 "ctrl->tagset 으로 되돌린다"는 **강제가 아니라 정책 선택**이다.

**채택 = Option B**: reset 은 per-set 오버레이를 걷어내고 baseline 으로 복귀한다. reset 복구가 **검증된 mainline default 경로**를 그대로 타므로 안전하고, 분할은 orchestration 이 `CSQ_CREATE` 로 재적용한다.

**하드 제약**: blk-mq 는 살아있는 request_queue 를 다른 tagset 으로 **re-home 불가**. 따라서 ns 를 per-set→ctrl->tagset 으로 옮기려면 **`del_gendisk` 후 ctrl->tagset 위에 재생성**해야 한다(→ per-set device 노드 블링크 발생, 수용).

두 케이스는 **하나의 경로로 통합**된다 — "per-set 오버레이가 있으면 걷어내고, 나머지는 mainline native":

```
nvme_reset_work:
 [Case 1] ctrl->tagset 만 사용 중  → per-set 오버레이 없음 → 그대로 mainline native reset
 [Case 2] per-set tagset 사용 중   → disable 단계에서 오버레이 해체:
     1) per-set namespace 들 nvme_ns_remove (del_gendisk)         ← re-home 위해
     2) per-set tagset 전부 blk_mq_free_tag_set + nvmset_group[] clear
   ── 재가동(공통) ──
     3) default IO 큐 재생성 (NVMSETID 없이, mainline) → ctrl->tagset blk_mq_update_nr_hw_queues
     4) nvme_scan_work 재스캔 → nvme_alloc_ns:
          selector 가 group 없음 확인 → ctrl->tagset 에 재바인딩 (자동 fallback)
```

**selector 가 fallback 을 흡수**한다(§5). group 을 clear 하면 ns 가 NVMSETID=k 를 보고해도 `find_group(k)==NULL` → ctrl->tagset:
```c
struct nvmset_group *g = find_group(ns->nvmset_id);          /* clear 후 NULL */
struct blk_mq_tag_set *ts = (g && g->tagset_ready) ? &g->tagset : ctrl->tagset;
blk_mq_alloc_disk(ts, &lim, ns);
```
→ 별도 "되돌리기 코드" 거의 불필요. group clear + per-set ns 재스캔이면 자동으로 baseline.

> 주의: reset 후 제거된 per-set namespace 를 다시 올리려면 **reset 경로가 rescan 을 트리거**해야 함(mainline 은 reset 시 ns 를 제거하지 않으므로, 이 제거+재추가는 우리 추가 로직).
> **Option A(분할 보존)** 대안: per-set tagset 유지 + 하드웨어 SQ 를 NVMSETID 로 재발행 + `blk_mq_update_nr_hw_queues` 재바인딩 → device 블링크 없으나 복구 경로 복잡. 본 설계는 단순·안전 우선으로 B 채택.

---

## 13. provisioning 순서 & 왕복 시나리오 (queue-first)

### 13.0 디바이스 namespace vs block-layer gendisk (scan = mirror)
**둘은 별개 객체이고 순서가 있다 — 디바이스가 먼저, block layer 가 그 미러.**

| | 디바이스 namespace | block-layer gendisk |
|--|-------------------|---------------------|
| 정체 | 컨트롤러의 namespace(NSID) | `/dev/nvme0nX` = gendisk + request_queue |
| 생성 주체 | orchestration (Admin 명령) | 커널 드라이버 (AEN 반응) |
| 수단 | Namespace Mgmt Create(0x0D)+Attach(0x15) | `nvme_alloc_ns`→`blk_mq_alloc_disk`→`device_add_disk` |

```
① 디바이스: ns create&attach (Admin, orchestration)
② AEN "NS Attribute Changed" → nvme_handle_aen_notice(core.c:4781)
     → nvme_queue_scan(core.c:158) → queue_work(scan_work)   [비동기]
③ nvme_scan_work(core.c:4519): 가드 state==LIVE && ctrl->tagset 있어야 진행
     → nvme_scan_ns_list(CNS 0x02) → nvme_scan_ns → nvme_alloc_ns(core.c:4135)
④ block layer: Identify Namespace(NVMSETID 읽음) → blk_mq_alloc_disk(tagset) → device_add_disk
```
- **비동기 디커플링**: ①과 ④는 원자적 아님(워크큐). 디바이스 ns 존재 후 gendisk 가 잠시 뒤 등장.
- **scan = reactive mirror(양방향 reconciliation)**: 디바이스에 새 ns→gendisk 생성, 사라진 ns→gendisk 제거.
- **우리 설계 연결**: tagset 바인딩(selector, core.c:4153)은 ④ block-layer 단계에서 일어나며 **디바이스가 준 NVMSETID** 를 읽어 그룹을 정한다. `nvme_scan_work` 가 `!ctrl->tagset` 이면 return 하므로 **queue-first 가 전제**(per-set SQ/tagset 선행).

### 정방향 (NVM Set 분할)
```
1) 전체 ns 삭제       : del_gendisk 전부
2) 전체 IO 큐 삭제    : Delete SQ→CQ, 그룹별 free_tag_set, qid→group 맵 리셋, 벡터 반납
3) create IO queue (NVMSET 1h) : group[1] SQ 누적
4) create IO queue (NVMSET 2h) : group[2] SQ 누적
5) ns create&attach (ENDGRP 1h, NVMSET 1h) : 첫 ns → group[1].tagset 생성 + add_disk
6) ns create&attach (ENDGRP 2h, NVMSET 2h) : group[2].tagset 생성 + add_disk → 격리 완성
```

### 역방향 (기본 상태 원복)
```
1) 전체 ns 삭제
2) 전체 IO 큐 삭제    : per-set tagset free_tag_set, qid 맵 리셋
3) create IO queue (w/o NVM set) : 기본 SQ 재생성 → ctrl->tagset (dormant→active)
4) ns create&attach (set 없음)   : ctrl->tagset 에 바인딩 = mainline 등가
```

### 왕복이 깨지지 않을 4조건
1. **완전 teardown**: 큐 삭제 시 `free_tag_set` 까지(선행: 모든 gendisk `del_gendisk`). 안 풀면 stale tagset 이 qid 재사용과 충돌. ("ns삭제→큐삭제" 순서가 정확히 이 선행조건.)
2. **qid→group 맵 리셋**: 완료 디먹스(`nvme_queue_tagset()` `pci.c:1524`)를 비움. qid 재사용 시 stale 매핑은 완료 오라우팅.
3. **큐 생애주기 드라이버 경유**: create/delete IO queue 가 raw passthrough 가 아니라 드라이버 ioctl 이어야 Create I/O SQ + tagset 생성/바인딩이 한 트랜잭션(bookkeeping 일관).
4. **기본 그룹(NVMSETID 0) = ctrl->tagset**: 역방향 상태의 home. 영속이라 별도 정책 코드 불필요.

> 전제(capability): ns 0개인 NVM Set 에 Create I/O SQ(NVMSETID) 발행 허용 — NVM Set 은 device 레벨 구성이라 원칙 OK, 단 타깃 컨트롤러 검증 차이는 확인.
> 의미 주의: queue-first 라 큐 생성 전까진 `/dev/nvme0nX` 미노출(큐 없으면 IO 도 불가하므로 합당).

---

## 14. 전체 시나리오 격리 검토 (mainline 코드 확인)

**판정: 데이터 경로 격리는 구조적으로 견고.** 모든 IO 가 `ns->queue → tagset` 으로 모이고 request_queue↔tagset 이 1:1 이므로, ns 를 그룹 tagset 에 묶는 순간 ns-X IO 가 ns-Y SQ 로 갈 경로가 없다.

| 시나리오 | 경로 | 격리 | 근거 |
|---------|------|------|------|
| FS/O_DIRECT/raw/io_uring(블록) | bio → blk-mq → `ns->queue` | ✅ | `core.c:4153` `blk_mq_alloc_disk` |
| ns passthrough ioctl(`/dev/nvme0n1`) | `nvme_submit_user_cmd(ns->queue,…)` | ✅ | `ioctl.c:264,317,364` |
| ns char passthrough(`/dev/ng0n1`) | `ns->queue` | ✅ | `ioctl.c:452` |
| poll/HIPRI, discard/flush/zns, bio 병합 | `ns->queue` (병합은 동일 q 내) | ✅ | cross-ns 병합 불가 |
| multipath(mpath head) | `nvme_find_path()` → path `ns->queue` | ✅* | `multipath.c:498,515,517` |
| 컨트롤러 char IO passthrough(`/dev/nvme0`) | **다중 ns 면 커널 -EINVAL 거부** | ✅ | `ioctl.c:819-823` (deprecated) |

### 처리할 갭 (누수 아님 — 예외/생애주기)
- **G2 (NVM Set 미지원 컨트롤러)**: NVMSETID 무시 → 단일 그룹 degrade(=mainline) 또는 SW 라우팅만(하드웨어 미디어 격리 없음). 폴백 필수.
- **G3 (reset/recovery)**: Option B — reset 시 per-set 오버레이 해체 후 ctrl->tagset baseline 복귀(§12.1). per-set ns 는 del_gendisk→재스캔으로 ctrl->tagset 재생성.
- **G5 (multipath/다중 컨트롤러 공유 ns)**: 같은 ns 가 각 컨트롤러에서 동일 NVM Set 그룹에 일관되게 매핑돼야 path 전환 시 격리 유지.
- **G6 (설정 규율)**: 그룹 tagset 간 공유 태그(`BLK_MQ_F_TAG_HCTX_SHARED`) 금지.
- **G7 (poll 예산)**: HIPRI 사용 시 그룹마다 poll 큐 별도 provision.

---

## 15. 완료 디먹스(completion demux) 상세 설계 — 1단계 핵심

### 디먹스란
CQ 에 올라온 완료(CQE)는 `command_id`(CID)만 담는다. 드라이버는 CID 로 **원래 `struct request` 를 역매핑**해 완료시킨다(= demultiplexing). 2단계:
```
CQE(qid, CID) ─① nvme_queue_tagset() (qid→tag 풀) ─② nvme_find_rq()=blk_mq_tag_to_rq() (CID→request)
```
CID 포맷(`nvme.h:650-657`): **`genctr(4b) | tag(12b)`**. tag=blk-mq tag(풀 내 유일), genctr=stale 탐지.

### mainline 이 깨지는 이유
`nvme_queue_tagset()`(`pci.c:1524-1528`): `return dev->tagset.tags[qid-1];` — **단일 tagset 가정**. 다중 tagset 에선 qid 는 전역, 각 그룹 `tags[]` 는 로컬 인덱스라 **엉뚱한 풀** 조회 → 다른 request 완료(정합성 붕괴) 또는 NULL(완료 유실/행). 조용히 깨지는 critical bug.

### 설계: `hctx->tags` 를 nvme_queue 에 캐시
핵심 통찰 — mainline `nvme_init_hctx_common`(`pci.c:638`)에 `WARN_ON(tags != hctx->tags)` 가 있다. 즉 **`hctx->tags` 가 그 hctx 의 올바른 풀**이고 blk-mq 가 init_hctx 전에 채워준다. 다중 tagset 에선 qid 공식은 틀리고 **`hctx->tags` 만 항상 옳다.** → **바인딩 시점에 캐시하고 완료 때 그대로 쓴다**(qid 역산 제거).

```c
/* 1) struct nvme_queue 에 캐시 필드 */
struct blk_mq_tags *tags;   /* set: init_hctx, get: 완료 디먹스. 값=hctx->tags. reset 시 갱신 */

/* 2) per-set 그룹 .init_hctx — 로컬 idx→그룹 SQ + tags 캐시 (set->driver_data=group) */
static int nvmset_init_hctx(struct blk_mq_hw_ctx *hctx, void *data, unsigned int hctx_idx) {
    struct nvmset_group *g = data;
    struct nvme_queue *nvmeq = g->sqs[hctx_idx];   /* 전역 qid 아님 — 그룹 로컬 인덱스 */
    nvmeq->tags = hctx->tags;                      /* ★ 디먹스용 정답 풀 캐시 */
    hctx->driver_data = nvmeq;
    return 0;
}

/* 3) nvme_queue_tagset 재작성 */
static inline struct blk_mq_tags *nvme_queue_tagset(struct nvme_queue *nvmeq) {
    if (!nvmeq->qid) return nvmeq->dev->admin_tagset.tags[0];
    return nvmeq->tags;                            /* qid 산술 제거 */
}
/* 4) 기본 그룹(ctrl->tagset) init_hctx_common 도 nvmeq->tags=hctx->tags 로 통일 */
```

### 제출 ↔ 완료 정합
```
제출 group[k].hctx_j : tag←group[k].tagset.tags[j],  CID=genctr|tag,  SQ write
                       ※ nvmeq->tags == group[k].tagset.tags[j] (init_hctx 캐시)
완료 nvmeq.CQ        : tags=nvme_queue_tagset(nvmeq)=nvmeq->tags  → nvme_find_rq(tags,CID)
```
제출에서 tag 꺼낸 풀 = 완료에서 조회하는 풀 = 동일 `nvmeq->tags` → 항상 정확.

### 캐시 방식 vs qid→group 룩업 테이블
| | 캐시(권장) | 룩업 테이블 |
|--|----------|-----------|
| 완료 비용 | O(1) 역참조 | 매 완료 조회 |
| 상태 동기화 | hctx->tags 재사용, 추가 0 | reset/teardown 마다 갱신 |
| reset 복구 | init_hctx 재실행 시 자동 | 별도 재구축 |

### 엣지
- **admin(qid 0)**: `admin_tagset.tags[0]` 분기 유지.
- **reset(Option B)**: 재바인딩 시 init_hctx 재실행 → tags 자동 갱신(자가복구). 큐 suspend 시 `nvmeq->tags=NULL` 클리어로 stale 방지.
- **poll/HIPRI**: `nvme_poll_cq→nvme_handle_cqe→nvme_queue_tagset` 동일 경로.
- **teardown 순서**: 큐 정지(Delete SQ/CQ) → free_tag_set. 순서 지키면 stale 접근 없음.
- **genctr/CID 포맷 불변** — `nvme_find_rq` stale 탐지 유지.
- **forward 전제**: init_hctx 가 `g->sqs[hctx_idx]` 읽음 → tagset 할당 전 그룹 SQ 존재(queue-first 보장, §9).

---

## 16. 코드 변경 지점 요약 (file:line)

| 위치 | 현재 | 변경 |
|------|------|------|
| `pci.c:3136/3777` `nvme_alloc_io_tag_set` | `dev->tagset` 1개 할당(=기본 그룹) | **유지**(기본 그룹으로 재활용) |
| `core.c:4153` `nvme_alloc_ns` `blk_mq_alloc_disk(ctrl->tagset)` | 모든 ns 가 ctrl->tagset | **selector 로 분기** + per-set tagset **lazy 생성**(§9) |
| **신규** ioctl 핸들러 `CSQ_CREATE/DELETE` | — | Create/Delete I/O SQ 발행 + group SQ 누적 + 매핑(§8) |
| `pci.c:1524-1528` `nvme_queue_tagset()` + `pci.c:629` `init_hctx` | 완료를 `dev->tagset.tags[qid-1]` 단일 디먹스 | **init_hctx 에서 `nvmeq->tags=hctx->tags` 캐시 → 완료는 캐시 반환** (상세 §15, 1단계 핵심) |
| teardown(큐 삭제) | reset 시 큐만 | **그룹 `free_tag_set` + qid 맵 리셋**(§13 조건 1·2) |
| `nvme_reset_work` 복구 | dev->tagset 재매핑 | **Option B: per-set 오버레이 해체 → ctrl->tagset baseline 복귀**(§12.1). Case2 는 per-set ns del_gendisk→재스캔 |

> **1단계 핵심 난점 = 완료 디먹스(§15)**: per-set SQ 완료를 올바른 그룹 tag 풀로 역매핑. qid 산술을 버리고 init_hctx 에서 `hctx->tags` 를 nvme_queue 에 캐시하는 방식으로 재작성.

---

## 17. 엣지/폴백 · 리스크

- **MSI-X 벡터 한계** — 그룹을 잘게 쪼갤수록 그룹당 SQ 빈약(벡터가 상한).
- **CPU > SQ** — 그룹 SQ 가 CPU 보다 적으면 `sq_lock` 경합 → 격리 부분화.
- **NVM Set/WRR 미지원** — 하드웨어 격리 없음(§14 G2). capability 확인 후 폴백.
- **동적 재구성 동기화** — tagset `nr_hw_queues` 갱신 타이밍, in-flight drain.
- **mainline divergence** — fork 리베이스 비용.

---

## 18. 구현 로드맵 / 다음 단계

```
1단계: NVM Set별 tagset + SQ 파티셔닝
       - nvmset_group 도입, ioctl(CSQ_CREATE) + Create I/O SQ(NVMSETID)
       - core.c:4153 selector + lazy tagset 생성
       - ★ 완료 디먹스 재작성: init_hctx 에서 nvmeq->tags=hctx->tags 캐시 (§15, 핵심 난점)
2단계: ns↔Set 자동 바인딩 + 동적 attach/teardown + reset=baseline 복귀(§12.1 Option B)
3단계: thread 격리 (cpuset cgroup)
```

**착수 항목**
1. 타깃 컨트롤러 capability: NVM Set 지원, `CAP.AMS`(WRR), 최대 IO 큐/MSI-X 벡터 수, "ns 0개 set 에 Create I/O SQ" 허용 여부.
2. 큐 예산 분배 정책(균등/가중치) 택일.
3. 1단계 상세: `nvmset_group` 필드 확정, **private ioctl 인터페이스(`CSQ_CREATE/DELETE`)**, 그룹 전용 `.map_queues`(§8.3, 라운드로빈)/`.init_hctx`(§15)/`nvme_queue_tagset()`(§15) — 매핑·디먹스는 설계 완료, 남은 건 ioctl 인터페이스·자료구조 확정·벡터 예산.

---

## 19. 실제 코드 구성 — 파일/모듈 레이아웃

> §1~§18 은 "무엇을/왜"의 설계다. 이하 §19~§27 은 **"실제 코드를 어디에 어떻게 둘지"**(파일 트리, 실 자료구조 정의, UAPI, 레이어 분리, blk-mq ops, 통합 지점, 빌드, 락)를 확정한다. 코드는 골격(skeleton)이며 커널 버전에 따라 시그니처는 미세 조정될 수 있다(기준: 6.x mainline `drivers/nvme/host`).

### 19.1 레이어 분리 원칙 (가장 중요한 결정)

mainline `nvme` 는 **두 모듈**로 갈린다:
- `nvme-core.ko` ← `core.c ioctl.c …` : 트랜스포트 독립. `struct nvme_ctrl`, `struct nvme_ns`, 스캔/바인딩.
- `nvme.ko` ← `pci.c` : PCIe 전용. `struct nvme_dev`, `struct nvme_queue`(SQ 실체), 하드웨어 큐 생성/MSI-X.

`struct nvme_dev`/`struct nvme_queue` 는 **`pci.c` 파일-private**(헤더에 없음). 따라서 NVMSETID·하드웨어 SQ 를 다루는 그룹 로직은 **반드시 PCIe 쪽(`nvme.ko`)** 에 있어야 한다. 반대로 ns→tagset 바인딩(`nvme_alloc_ns`)은 **core 쪽**이다. 둘을 잇는 유일한 깨끗한 통로는 **`nvme_ctrl_ops` 콜백**이다.

```
core.c (nvme-core.ko)                 pci.c / pci-nvmset.c (nvme.ko)
  nvme_alloc_ns()                       struct nvme_dev { groups[] }
    └ selector 훅 호출 ───ops 콜백──►    nvme_pci_ns_tagset()  ← 그룹 선택/lazy 생성
                                         CSQ_CREATE/DELETE 핸들러 ← 하드웨어 SQ
  nvme_dev_ioctl() (ioctl.c)            nvme_pci_nvmset_ctl()  ← ioctl 디스패치 대상
    └ 신규 cmd ──ops 콜백──────────►
```

**원칙**: core 는 NVMSETID 를 *모르고*, "tagset 을 골라 달라"는 콜백만 호출한다. 콜백이 NULL(=mainline/비-PCIe)이면 `ctrl->tagset` 폴백 → **기존 동작 100% 보존**. 모든 NVM Set 지식은 PCIe 모듈에 격리.

### 19.2 파일 트리 (신규/수정)

```
drivers/nvme/host/
├── nvme.h            [수정] nvme_ctrl_ops 콜백 2개, nvme_ns_head.nvmset_id,
│                            nvme_ns_info.nvmset_id 추가
├── core.c            [수정] nvme_alloc_ns selector, nvme_ns_info_from_identify
│                            에서 NVMSETID 채집, defer/rescan
├── ioctl.c           [수정] nvme_dev_ioctl 에 CSQ_CREATE/DELETE case 추가(디스패치만)
├── pci.c             [수정] struct nvme_dev/nvme_queue 확장, nvme_mq_ops 옆에
│                            nvmset_mq_ops, nvme_queue_tagset 재작성, reset(Option B)
├── pci.h             [신규] struct nvme_dev/nvme_queue/nvmset_group 선언을
│                            pci.c ↔ pci-nvmset.c 공유 (모듈화할 경우)
├── pci-nvmset.c      [신규] 그룹 lifecycle 전부: CSQ_CREATE/DELETE 구현,
│                            그룹 tagset alloc/free, 큐 예산, selector 구현
├── Makefile          [수정] nvme-y += pci-nvmset.o (CONFIG 가드)
└── Kconfig           [수정] config NVME_NVMSET_ISOLATION
include/uapi/linux/
└── nvme_ioctl.h      [수정] CSQ_CREATE/DELETE ioctl 번호 + 인자 구조체(UAPI)
```

> **간이안(파일 1개)**: `pci.h`/`pci-nvmset.c` 분리가 부담이면 모든 신규 코드를 `pci.c` 안에 둬도 된다(struct 가 이미 거기 있으므로 추가 헤더 불필요). 모듈화는 리뷰/리베이스 편의를 위한 권장일 뿐. 본 문서는 분리안을 기준으로 기술한다.

### 19.3 빌드 (Makefile / Kconfig)

```makefile
# drivers/nvme/host/Makefile
nvme-y                                  += pci.o
nvme-$(CONFIG_NVME_NVMSET_ISOLATION)    += pci-nvmset.o
```
```kconfig
# drivers/nvme/host/Kconfig
config NVME_NVMSET_ISOLATION
    bool "NVMe per-NVM-Set SQ/tagset isolation (custom)"
    depends on BLK_DEV_NVME
    help
      Create per-NVM-Set I/O submission queues and bind each namespace to a
      dedicated blk-mq tag set for noisy-neighbor isolation. Controlled via
      private CSQ_CREATE/CSQ_DELETE ioctls on the controller char device.
```
CONFIG 미설정 시: `nvme_ctrl_ops.ns_tagset == NULL`, ioctl case 들이 `-ENOTTY` → **빌드/동작이 정확히 mainline**.

---

## 20. 자료구조 — 실제 정의

### 20.1 `pci.h` (신규) — 그룹과 dev/queue 확장

```c
/* drivers/nvme/host/pci.h  (CONFIG_NVME_NVMSET_ISOLATION) */

enum nvmset_state { NVMSET_STATE_D, NVMSET_STATE_P };   /* §1 D/P */

struct nvmset_group {                  /* per-set 격리 단위. NVMSETID k>=1 */
    u16                    nvmset_id;   /* 이 그룹의 NVM Set ID */
    struct blk_mq_tag_set  tagset;      /* 그룹 전용 IO tagset (일시) */
    struct nvme_queue    **sqs;         /* group->sqs[j] = 로컬 j 번째 SQ (mainline nvme_queue) */
    unsigned int           nr_sqs;      /* = tagset.nr_hw_queues. CSQ_CREATE 마다 ++ */
    bool                   tagset_ready;/* lazy 생성 가드 (§9) */
    refcount_t             ns_ref;      /* 이 그룹에 바인딩된 gendisk 수. 0→free 가능 */
    struct list_head       entry;       /* nvme_dev.group_list 연결 */
};

/* mainline struct nvme_dev 에 추가할 필드 (pci.c 정의를 pci.h 로 이전) */
struct nvme_dev {
    /* ... 기존 필드 그대로: queues, tagset, admin_tagset, ctrl, max_qid, ... */
    struct list_head   group_list;      /* nvmset_group 들 */
    unsigned int       nr_groups;
    enum nvmset_state  nvmset_state;    /* 판정 캐시 (§1.35: 활성 그룹>0 ? P : D) */
    struct mutex       nvmset_lock;     /* CSQ_CREATE/DELETE ↔ reset ↔ scan 직렬화(§26) */
    struct list_head   deferred_ns;     /* State P 에서 그룹 미존재로 보류된 ns (§23) */
};

/* mainline struct nvme_queue 에 추가할 필드 (완료 디먹스 §15) */
struct nvme_queue {
    /* ... 기존 필드: dev, sq_cmds, cq, qid, q_depth, cq_vector, ... */
    struct blk_mq_tags *tags;           /* = hctx->tags. set:init_hctx get:완료. NULL=suspend */
    struct nvmset_group *group;         /* 이 SQ 가 속한 그룹(기본그룹이면 NULL) */
};
```

### 20.2 `nvme.h` (수정) — core↔pci 연결 필드/콜백

```c
/* struct nvme_ns_info 에 추가 — 스캔 중 NVMSETID 운반 (core.c 내부 구조체) */
struct nvme_ns_info {
    /* ... nsid, anagrpid, is_shared, ... */
    u16 nvmset_id;                      /* Identify Namespace NVMSETID (§23) */
};

/* struct nvme_ns_head 에 추가 — teardown 시 그룹 식별 */
struct nvme_ns_head {
    /* ... ns_id, ... */
    u16 nvmset_id;
};

/* struct nvme_ctrl_ops 에 추가 — 선택 콜백 (NULL 허용) */
struct nvme_ctrl_ops {
    /* ... 기존 콜백 ... */

    /* ns 에 바인딩할 tagset 선택. NULL 이면 core 가 ctrl->tagset 사용(mainline).
     * *defer=true 반환 시 core 는 이번 스캔에서 disk 생성을 보류(State P pending). */
    struct blk_mq_tag_set *(*ns_tagset)(struct nvme_ctrl *ctrl,
                                        struct nvme_ns_info *info, bool *defer);

    /* 컨트롤러 char dev 의 private ioctl 디스패치(CSQ_CREATE/DELETE). NULL→-ENOTTY */
    int (*nvmset_ctl)(struct nvme_ctrl *ctrl, unsigned int cmd, void __user *arg);
};
```

> `struct nvme_ns` 자체는 **수정 0** — 바인딩된 tagset 정보는 `ns->queue`(request_queue)에 이미 내포된다. 그룹 식별용 `nvmset_id` 만 `ns_head` 에 둔다(§7 "nvmset_id 보관 필드 1개"의 실제 위치).

---

## 21. UAPI — private ioctl 인터페이스

```c
/* include/uapi/linux/nvme_ioctl.h (추가). 'N' 매직, 미사용 번호 0x50/0x51 (충돌 확인 필수) */

struct nvme_csq_create {
    __u16 nvmset_id;   /* 0 = State D(ctrl->tagset). >=1 = State P(그룹 k) */
    __u16 qsize;       /* SQ 깊이(엔트리). 0 → 드라이버 기본 */
    __u8  qprio;       /* WRR: 0=Urgent..3=Low. CAP.AMS 미지원 시 무시(§8.2) */
    __u8  flags;       /* 예약(0) */
    __u16 qid_out;     /* [out] 배정된 IO 큐 qid */
    __u32 rsvd;
};

struct nvme_csq_delete {
    __u16 nvmset_id;   /* 그룹 단위 삭제 시 */
    __u16 qid;         /* 단일 큐 삭제 시(0이면 nvmset_id 그룹 전체) */
    __u32 rsvd;
};

#define NVME_IOCTL_CSQ_CREATE  _IOWR('N', 0x50, struct nvme_csq_create)
#define NVME_IOCTL_CSQ_DELETE  _IOW ('N', 0x51, struct nvme_csq_delete)
```

대상 디바이스: **컨트롤러 char dev `/dev/nvme0`**(ns 가 아니라 컨트롤러 레벨 자원이므로). 권한: `CAP_SYS_ADMIN` 필수(하드웨어 큐 생성).

---

## 22. 제어 경로 구현 (`ioctl.c` 디스패치 → `pci-nvmset.c`)

### 22.1 ioctl.c — 디스패치만 (core, 트랜스포트 독립)

```c
/* drivers/nvme/host/ioctl.c : nvme_dev_ioctl() 의 switch 에 추가 */
case NVME_IOCTL_CSQ_CREATE:
case NVME_IOCTL_CSQ_DELETE:
    if (!capable(CAP_SYS_ADMIN))            return -EACCES;
    if (!ctrl->ops->nvmset_ctl)             return -ENOTTY;  /* 비-PCIe/CONFIG off */
    return ctrl->ops->nvmset_ctl(ctrl, cmd, (void __user *)arg);
```

### 22.2 pci-nvmset.c — 실제 핸들러

```c
/* nvme_ctrl_ops.nvmset_ctl 구현체. ctrl→dev 역참조 후 분기 */
int nvme_pci_nvmset_ctl(struct nvme_ctrl *ctrl, unsigned int cmd, void __user *arg)
{
    struct nvme_dev *dev = to_nvme_dev(ctrl);
    switch (cmd) {
    case NVME_IOCTL_CSQ_CREATE: return nvmset_csq_create(dev, arg);
    case NVME_IOCTL_CSQ_DELETE: return nvmset_csq_delete(dev, arg);
    default:                    return -ENOTTY;
    }
}

static int nvmset_csq_create(struct nvme_dev *dev, void __user *uarg)
{
    struct nvme_csq_create c;
    struct nvmset_group *g;
    struct nvme_queue *nvmeq;
    int qid, ret;

    if (copy_from_user(&c, uarg, sizeof(c)))           return -EFAULT;
    guard(mutex)(&dev->nvmset_lock);                   /* §26 직렬화 */

    /* all-or-nothing 불변식 검사 (§1 자기전이 #2) */
    if (dev->nvmset_state == NVMSET_STATE_P && c.nvmset_id == 0) return -EBUSY; /* P 에 D 큐 금지 */
    if (dev->nvmset_state == NVMSET_STATE_D && c.nvmset_id != 0 && dev->ctrl.tagset /*active*/)
        /* D→P 진입: 기존 ctrl->tagset 큐가 살아있는 채로 섞으면 안 됨 → §13 정방향 teardown 선행 요구 */
        return -EBUSY;

    qid = nvme_alloc_queue_id(dev);                    /* 전역 qid 비트맵에서 할당 */
    if (qid < 0) return qid;

    /* §8.2: CQ DMA+MSI-X → Create I/O CQ → SQ DMA → Create I/O SQ(NVMSETID,QPRIO) */
    ret = nvme_create_queue(/*dev,*/ qid, c.nvmset_id, c.qprio, c.qsize);
    if (ret) { nvme_free_queue_id(dev, qid); return ret; }
    nvmeq = dev->queues[qid];

    if (c.nvmset_id == 0) {
        /* State D: 기본 그룹. ctrl->tagset 의 nr_hw_queues 를 +1 (mainline grow) */
        nvmeq->group = NULL;
        return nvmset_grow_default(dev, c.qid_out = qid, uarg);   /* blk_mq_update_nr_hw_queues */
    }

    /* State P: per-set 그룹에 SQ 누적(tagset 은 아직 안 만듦, §9 queue-first) */
    g = nvmset_group_get_or_create(dev, c.nvmset_id);  /* 없으면 그룹 구조만 신설 */
    if (!g) { /* rollback */ return -ENOMEM; }
    nvmeq->group = g;
    g->sqs[g->nr_sqs++] = nvmeq;
    dev->nvmset_state = NVMSET_STATE_P;

    nvme_queue_scan(&dev->ctrl);                       /* §23: 보류 ns 재시도(D→P) */
    c.qid_out = qid;
    return copy_to_user(uarg, &c, sizeof(c)) ? -EFAULT : 0;
}
```

`nvmset_csq_delete()` 는 §12[C]/[D]·§13 역방향을 그대로 구현: drain → `adapter_delete_sq` → `adapter_delete_cq` → 벡터 반납 → (그룹 마지막이면) `nvmset_group_free()`.

### 22.3 큐 예산 (§8.1)

```c
/* probe 후 1회: 가용 IO 큐/벡터 수 캐시 */
dev->max_io_queues = nvme_set_queue_count(&dev->ctrl, want);   /* mainline */
/* CSQ_CREATE 마다: Σ nr_sqs + 기본그룹 ≤ max_io_queues 검사. 초과 시 -ENOSPC */
```

---

## 23. core 통합 — selector 훅 + NVMSETID 채집 + defer/rescan

### 23.1 NVMSETID 채집 (`core.c : nvme_ns_info_from_identify`)

```c
/* Identify Namespace(CNS 00h) 응답에서 NVMSETID 를 info 로 옮긴다.
 * id->nvmsetid 는 mainline struct nvme_id_ns 에 이미 존재(__le16). */
info->nvmset_id = le16_to_cpu(id->nvmsetid);
```

### 23.2 selector (`core.c : nvme_alloc_ns`, §5/§1 자기전이 #3 의 실제 구현)

```c
/* 기존: disk = blk_mq_alloc_disk(ctrl->tagset, &lim, ns);  를 교체 */
static struct blk_mq_tag_set *
nvme_select_ns_tagset(struct nvme_ctrl *ctrl, struct nvme_ns_info *info, bool *defer)
{
    *defer = false;
    if (ctrl->ops->ns_tagset)
        return ctrl->ops->ns_tagset(ctrl, info, defer);   /* PCIe: 그룹/lazy/defer */
    return ctrl->tagset;                                   /* mainline 폴백 */
}

/* nvme_alloc_ns() 내부 */
bool defer;
struct blk_mq_tag_set *ts = nvme_select_ns_tagset(ctrl, info, &defer);
if (defer) {                          /* State P + 그룹 미존재 → 이번 스캔 보류 */
    nvme_defer_ns(ctrl, info->nsid);  /* dev->deferred_ns 에 nsid 기록 */
    goto out_free_ns;                 /* gendisk 만들지 않음(§1 자기전이 #3 pending) */
}
ns->head->nvmset_id = info->nvmset_id;
disk = blk_mq_alloc_disk(ts, &lim, ns);
```

### 23.3 PCIe selector + lazy tagset (`pci-nvmset.c : nvme_pci_ns_tagset`)

```c
struct blk_mq_tag_set *
nvme_pci_ns_tagset(struct nvme_ctrl *ctrl, struct nvme_ns_info *info, bool *defer)
{
    struct nvme_dev *dev = to_nvme_dev(ctrl);
    struct nvmset_group *g;
    *defer = false;

    if (info->nvmset_id == 0) {                       /* State D 경로 */
        if (nvme_ctrl_tagset_active(ctrl)) return ctrl->tagset;
        *defer = true; return NULL;                   /* P 인데 D ns? → 보류 */
    }
    g = nvmset_find_group(dev, info->nvmset_id);
    if (!g || g->nr_sqs == 0) { *defer = true; return NULL; }   /* 그룹 큐 아직 없음 */
    if (!g->tagset_ready) {                            /* ★ per-set tagset lazy 생성(§9) */
        int ret = nvmset_alloc_tag_set(dev, g);       /* §24.1 */
        if (ret) { *defer = true; return NULL; }
    }
    refcount_inc(&g->ns_ref);
    return &g->tagset;
}
```

### 23.4 defer 재구동

`CSQ_CREATE` 가 새 그룹 SQ 를 누적한 뒤 `nvme_queue_scan(ctrl)` 호출(§22.2) → `nvme_scan_work` 가 NSID 리스트를 재순회 → 보류됐던 ns 가 이번엔 `tagset_ready` 그룹을 만나 정상 `blk_mq_alloc_disk`. **별도 보류 리스트 없이 rescan 만으로도 성립**하나, 대량 ns 환경에서 재순회 비용을 줄이려면 `dev->deferred_ns` 로 타깃 재시도.

---

## 24. blk-mq ops (per-set) — 실제 구현

### 24.1 그룹 tagset 할당 — ⚠️ `nvme_alloc_io_tag_set` 재사용 불가

> **설계 보정(§9 대비)**: §9 의 `nvme_alloc_io_tag_set(&group.tagset, nr_hw_queues=…)` 는 **그대로 못 쓴다.** mainline `nvme_alloc_io_tag_set()` 은 `set->nr_hw_queues` 를 **컨트롤러 전역 `ctrl->queue_count` 기준으로 내부 결정**하고 `ctrl->tagset` 포인터까지 건드린다. per-set 은 `nr_hw_queues = g->nr_sqs`(부분집합)여야 하므로 **`blk_mq_tag_set` 을 직접 채우고 `blk_mq_alloc_tag_set()` 을 호출**한다.

```c
static int nvmset_alloc_tag_set(struct nvme_dev *dev, struct nvmset_group *g)
{
    struct blk_mq_tag_set *set = &g->tagset;

    memset(set, 0, sizeof(*set));
    set->ops          = &nvmset_mq_ops;            /* §24.2 */
    set->nr_hw_queues = g->nr_sqs;                 /* ★ 그룹 SQ 수 = hctx 수 */
    set->nr_maps      = 1;                          /* phase1: DEFAULT 만 (§8.3) */
    set->queue_depth  = dev->ctrl.sqsize + 1;
    set->numa_node    = dev->ctrl.numa_node;
    set->cmd_size     = nvme_pci_cmd_size(dev);    /* nvme_mq_ops 와 동일 계산 재사용 */
    set->timeout      = NVME_IO_TIMEOUT;
    set->flags        = BLK_MQ_F_SHOULD_MERGE;     /* ※ TAG_HCTX_SHARED 금지(§14 G6) */
    set->driver_data  = g;                         /* ★ map_queues/init_hctx 가 그룹 접근 */

    int ret = blk_mq_alloc_tag_set(set);
    if (ret) return ret;
    g->tagset_ready = true;                         /* §9 가드 */
    return 0;
}
```

### 24.2 `nvmset_mq_ops` — `nvme_mq_ops` 복제 + 2개 override

```c
/* pci.c: 기존 nvme_mq_ops 와 대부분 동일, .map_queues/.init_hctx 만 교체 */
static const struct blk_mq_ops nvmset_mq_ops = {
    .queue_rq    = nvme_queue_rq,        /* ← 무수정 재사용(§3 데이터패스) */
    .complete    = nvme_pci_complete_rq, /* ← 무수정 */
    .commit_rqs  = nvme_commit_rqs,      /* ← 무수정 */
    .init_request= nvme_pci_init_request,/* ← 무수정 */
    .timeout     = nvme_timeout,         /* ← 무수정 */
    .poll        = nvme_poll,            /* phase2(poll map) 에서만 */
    .init_hctx   = nvmset_init_hctx,     /* ★ override (§15) */
    .map_queues  = nvmset_map_queues,    /* ★ override (§8.3) */
};
```

### 24.3 `.map_queues` (§8.3 phase1 — 라운드로빈)

```c
static void nvmset_map_queues(struct blk_mq_tag_set *set)
{
    struct nvmset_group *g = set->driver_data;
    struct blk_mq_queue_map *map = &set->map[HCTX_TYPE_DEFAULT];
    map->nr_queues    = g->nr_sqs;
    map->queue_offset = 0;
    blk_mq_map_queues(map);    /* 전 online CPU → g->nr_sqs 라운드로빈(affinity 무관) */
}
```

### 24.4 `.init_hctx` + 완료 디먹스 (§15 — 1단계 핵심)

```c
static int nvmset_init_hctx(struct blk_mq_hw_ctx *hctx, void *data, unsigned int hctx_idx)
{
    struct nvmset_group *g = data;                  /* = set->driver_data */
    struct nvme_queue *nvmeq = g->sqs[hctx_idx];    /* 로컬 idx → 그룹 SQ(전역 qid 아님) */
    nvmeq->tags = hctx->tags;                       /* ★ 완료 디먹스 정답 풀 캐시 */
    hctx->driver_data = nvmeq;
    return 0;
}

/* pci.c: nvme_queue_tagset() 재작성 (기본 그룹 init_hctx 도 nvmeq->tags=hctx->tags 로 통일) */
static inline struct blk_mq_tags *nvme_queue_tagset(struct nvme_queue *nvmeq)
{
    if (!nvmeq->qid) return nvmeq->dev->admin_tagset.tags[0];  /* admin */
    return nvmeq->tags;                                         /* qid 산술 제거 */
}
```

---

## 25. reset 통합 — Option B (§12.1) 실제 위치

`nvme_reset_work()`(pci.c) 의 **disable 단계**와 **re-enable 단계**에 훅을 끼운다:

```c
static void nvme_reset_work(struct work_struct *work)
{
    /* ── disable 직후(하드웨어 큐 죽은 시점) ── */
    nvmset_teardown_overlay(dev);   /* Case2: per-set ns del_gendisk + 그룹 free_tag_set
                                       + group_list clear + nvmset_state=D (§12.1 1·2단계) */

    /* ... mainline: nvme_dev_disable → 재초기화 → default IO 큐 재생성 ... */
    /* nvme_pci_update_nr_queues(): ctrl->tagset blk_mq_update_nr_hw_queues (Case 공통 3단계) */

    /* ── nvme_scan_work 재스캔(공통 4단계) ──
       selector 가 group 없음 확인 → ctrl->tagset 재바인딩(자동 fallback, §12.1) */
}
```

`nvmset_teardown_overlay()`:
```c
static void nvmset_teardown_overlay(struct nvme_dev *dev)
{
    struct nvmset_group *g, *t;
    guard(mutex)(&dev->nvmset_lock);
    if (dev->nvmset_state != NVMSET_STATE_P) return;             /* Case1: no-op */
    list_for_each_entry_safe(g, t, &dev->group_list, entry) {
        nvmset_remove_group_namespaces(dev, g);                 /* del_gendisk (re-home 위해) */
        if (g->tagset_ready) blk_mq_free_tag_set(&g->tagset);
        list_del(&g->entry); kfree(g->sqs); kfree(g);
    }
    dev->nr_groups = 0;
    dev->nvmset_state = NVMSET_STATE_D;                          /* P→D 확정 */
    /* 이후 재스캔에서 selector 가 NULL group → ctrl->tagset 자동 복귀 */
}
```

> reset 은 per-set ns 를 제거하므로, **reset 경로가 명시적으로 `nvme_queue_scan()` 을 트리거**해 제거된 ns 를 ctrl->tagset 위로 재생성해야 한다(§12.1 주의). mainline 은 reset 시 ns 를 제거하지 않으므로 이 재추가가 우리 추가 로직.

---

## 26. 동시성 / 락 순서

| 자원 | 락 | 보호 대상 |
|------|----|-----------|
| 그룹 생성/삭제/teardown | `dev->nvmset_lock`(mutex) | `group_list`, `nr_groups`, `nvmset_state`, `deferred_ns` |
| ns 바인딩(scan) | mainline `namespaces_rwsem`/scan 직렬성 | `ns` 생성. selector 는 `nvmset_lock` 아래에서 그룹 조회 |
| 하드웨어 큐 | mainline 큐 락 | `nvme_queue` SQ 링 |

**락 순서(deadlock 회피)**: `nvmset_lock` → (필요 시) blk-mq freeze. **`nvmset_lock` 을 쥔 채 `nvme_queue_scan()` 의 완료를 기다리지 말 것**(scan_work 가 selector 에서 같은 락을 재취득 → ABBA). CSQ_CREATE 는 `nvme_queue_scan()` 을 **비동기 큐잉만** 하고 락을 즉시 놓는다(§22.2 가 그 순서). reset(`nvmset_lock`) ↔ CSQ(`nvmset_lock`) 는 동일 락으로 상호배제.

---

## 27. 함수 인벤토리 (신규 N / 수정 M) & 1단계 PR 분할

| 심볼 | 파일 | N/M | 역할 | 절 |
|------|------|-----|------|----|
| `nvme_ctrl_ops.ns_tagset` / `.nvmset_ctl` | nvme.h | N | core↔pci 콜백 2개 | §20.2 |
| `nvme_ns_info.nvmset_id` / `nvme_ns_head.nvmset_id` | nvme.h | N | NVMSETID 운반/보관 | §20.2 |
| `nvme_ns_info_from_identify` | core.c | M | NVMSETID 채집 | §23.1 |
| `nvme_select_ns_tagset` / `nvme_alloc_ns` | core.c | N/M | selector + defer | §23.2 |
| `nvme_dev_ioctl` | ioctl.c | M | CSQ_* 디스패치 | §22.1 |
| `struct nvmset_group`, dev/queue 확장 | pci.h | N | 그룹/캐시 필드 | §20.1 |
| `nvme_pci_nvmset_ctl` / `nvmset_csq_create/delete` | pci-nvmset.c | N | 제어 경로 | §22.2 |
| `nvme_pci_ns_tagset` / `nvmset_alloc_tag_set` | pci-nvmset.c | N | selector + lazy tagset | §23.3/§24.1 |
| `nvmset_mq_ops` / `nvmset_map_queues` / `nvmset_init_hctx` | pci.c | N | blk-mq ops | §24 |
| `nvme_queue_tagset` | pci.c | **M(핵심)** | 완료 디먹스 재작성 | §24.4/§15 |
| `nvmset_teardown_overlay` / `nvme_reset_work` | pci.c | N/M | Option B | §25 |
| ioctl 번호/구조체 | uapi/nvme_ioctl.h | N | UAPI | §21 |
| Makefile / Kconfig | — | M | 빌드 가드 | §19.3 |

**1단계 PR 분할(독립 리뷰·이등분 용이)**:
1. **PR-A(무동작 인프라)**: `nvme_ctrl_ops` 콜백 추가 + `nvme_select_ns_tagset` 도입(콜백 NULL→mainline 동일). NVMSETID 채집. **기능 변화 0**, 회귀 테스트 통과 기준선.
2. **PR-B(디먹스 재작성)**: `nvmeq->tags` 캐시 + `nvme_queue_tagset` 재작성 + 기본 그룹 init_hctx 통일. **단일 tagset 에서도 동작 동일**해야 함(§15 자가검증).
3. **PR-C(그룹 lifecycle)**: `pci.h`/`pci-nvmset.c`, UAPI ioctl, `nvmset_mq_ops`, lazy tagset, defer/rescan. State D↔P 전이.
4. **PR-D(reset Option B)**: `nvmset_teardown_overlay` + reset 재스캔.

> PR-A·PR-B 는 NVM Set 없이도 머지 가능(순수 리팩터). 위험은 PR-C·PR-D 에 집중되므로 분리가 곧 안전장치.

---

## 28. 보조 함수 전체 구현 (full helpers)

> §22~§25 에서 이름만 등장한 모든 헬퍼를 완성한다. 작성하며 확정된 **3개 보정점**(앞 절과의 정합):
> 1. **§22.2 의 `nvme_create_queue(qid,nvmset_id,qprio,qsize)` → `nvmset_create_hw_queue()`** 로 명명(아래 §28.3).
> 2. **§20.1 `refcount_t ns_ref` / §23.3 `refcount_inc` 제거** — 그룹 해제는 명시적(CSQ_DELETE/teardown)이고 ns 는 `head->nvmset_id` 로 열거하므로 카운터 불요(queue-first 라 ns 0 그룹 정상, §13). 통계가 필요하면 락 보호 `unsigned int nr_ns` 로만.
> 3. **§22.2 P-branch grow 누락 보강** — 기존 그룹(`tagset_ready`)에 SQ 추가 시 `blk_mq_update_nr_hw_queues()` 호출(P→P [B], §28.5).
>
> 공통 전제: **별도 명시 없으면 모든 헬퍼는 호출자가 `dev->nvmset_lock` 보유**(§26). `to_nvme_dev(ctrl)=container_of(ctrl,struct nvme_dev,ctrl)`.
>
> `struct nvme_dev` 추가 필드(§20.1 보완): `struct ida qid_ida;`(IO qid 1..max_qid), `struct ida vec_ida;`(MSI-X 1..num_vecs-1) — probe 에서 `ida_init`, remove 에서 `ida_destroy`.

### 28.1 그룹 조회 / 생성 / 해제

```c
/* 그룹 검색 — caller holds nvmset_lock */
static struct nvmset_group *nvmset_find_group(struct nvme_dev *dev, u16 id)
{
    struct nvmset_group *g;
    list_for_each_entry(g, &dev->group_list, entry)
        if (g->nvmset_id == id)
            return g;
    return NULL;
}

/* 없으면 그룹 구조만 신설(하드웨어 SQ/tagset 은 아직 X) — caller holds nvmset_lock */
static struct nvmset_group *nvmset_group_get_or_create(struct nvme_dev *dev, u16 id)
{
    struct nvmset_group *g = nvmset_find_group(dev, id);
    if (g)
        return g;                       /* 이미 존재: 그대로 반환(누적 대상) */

    g = kzalloc(sizeof(*g), GFP_KERNEL);
    if (!g)
        return NULL;
    /* SQ 포인터 배열은 그룹이 최대로 가질 수 있는 SQ 수(=전역 max_qid)로 1회 할당 */
    g->sqs = kcalloc(dev->max_qid, sizeof(*g->sqs), GFP_KERNEL);
    if (!g->sqs) {
        kfree(g);
        return NULL;
    }
    g->nvmset_id    = id;
    g->nr_sqs       = 0;
    g->tagset_ready = false;
    INIT_LIST_HEAD(&g->entry);
    list_add_tail(&g->entry, &dev->group_list);
    dev->nr_groups++;
    return g;
}

/* 그룹 완전 해제 — 전제: 소속 ns 전부 nvmset_remove_group_namespaces() 로 제거됨.
 * caller holds nvmset_lock. (§13 조건1: free_tag_set 은 gendisk 제거 후) */
static void nvmset_group_free(struct nvme_dev *dev, struct nvmset_group *g)
{
    int i;

    if (g->tagset_ready)
        blk_mq_free_tag_set(&g->tagset);          /* SW tagset 해제 */
    for (i = g->nr_sqs - 1; i >= 0; i--)          /* SQ→CQ 순, 역순 정리(§12 규칙①) */
        nvmset_destroy_hw_queue(dev, g->sqs[i]);
    list_del(&g->entry);
    dev->nr_groups--;
    kfree(g->sqs);
    kfree(g);
}

/* 이 그룹에 바인딩된 gendisk 전부 제거(re-home/teardown 전 선행) — caller holds nvmset_lock.
 * nvme_ns_remove() 는 rwsem 안에서 못 부르므로 mainline 패턴대로 락을 풀고 호출. */
static void nvmset_remove_group_namespaces(struct nvme_dev *dev, struct nvmset_group *g)
{
    struct nvme_ctrl *ctrl = &dev->ctrl;
    struct nvme_ns *ns, *next;
    LIST_HEAD(rm);

    down_write(&ctrl->namespaces_rwsem);          /* 최신 커널은 srcu — 버전에 맞춤 */
    list_for_each_entry_safe(ns, next, &ctrl->namespaces, list)
        if (ns->head->nvmset_id == g->nvmset_id)
            list_move_tail(&ns->list, &rm);
    up_write(&ctrl->namespaces_rwsem);

    list_for_each_entry_safe(ns, next, &rm, list)
        nvme_ns_remove(ns);                        /* del_gendisk + 정리(mainline) */
}
```

### 28.2 qid / MSI-X 벡터 회계

```c
/* 전역 IO qid 비트맵 할당(1..max_qid). 그룹 간 비연속 가능 → IDA 사용 */
static int nvme_alloc_queue_id(struct nvme_dev *dev)
{
    return ida_alloc_range(&dev->qid_ida, 1, dev->max_qid, GFP_KERNEL);
}
static void nvme_free_queue_id(struct nvme_dev *dev, u16 qid)
{
    ida_free(&dev->qid_ida, qid);
}

/* MSI-X 벡터 1개 확보/반납(admin=0 제외). ★ 실제 affinity 배정은 §17 리스크 — 여기선 풀에서 인덱스만 */
static int nvmset_acquire_vector(struct nvme_dev *dev)
{
    return ida_alloc_range(&dev->vec_ida, 1, dev->num_vecs - 1, GFP_KERNEL);
}
static void nvmset_release_vector(struct nvme_dev *dev, int vector)
{
    if (vector >= 0)
        ida_free(&dev->vec_ida, vector);
}

/* 큐 IRQ 등록(mainline nvme_irq 핸들러 재사용) */
static int nvmset_request_irq(struct nvme_dev *dev, struct nvme_queue *nvmeq)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);
    return pci_request_irq(pdev, nvmeq->cq_vector, nvme_irq, NULL, nvmeq,
                           "nvme%dq%d", dev->ctrl.instance, nvmeq->qid);
}
```

### 28.3 하드웨어 큐 생성 — NVMSETID/QPRIO 주입 (§8.2 실제 코드)

```c
/* QPRIO → Create I/O SQ CDW11 sq_flags 비트. WRR 미지원(CC.AMS=0)이면 URGENT(=0) 무해 */
static u16 nvmset_prio_flag(u8 qprio)
{
    switch (qprio) {
    case 0:  return NVME_SQ_PRIO_URGENT;   /* (0 << 1) */
    case 1:  return NVME_SQ_PRIO_HIGH;     /* (1 << 1) */
    case 2:  return NVME_SQ_PRIO_MEDIUM;   /* (2 << 1) */
    default: return NVME_SQ_PRIO_LOW;      /* (3 << 1) */
    }
}

/* mainline adapter_alloc_sq 확장: CDW11 에 QPRIO, CDW12 에 NVMSETID.
 * ★ struct nvme_create_sq 의 CDW12 예약영역을 fork 에서 nvmsetid 필드로 명명
 *   (mainline: __le32 rsvd12[...]; → fork: __le16 nvmsetid; __le16 rsvd; __u32 rsvd13[3];) */
static int adapter_alloc_sq_nvmset(struct nvme_dev *dev, u16 qid,
                                   struct nvme_queue *nvmeq, u16 nvmset_id, u8 qprio)
{
    struct nvme_command c = { };
    u16 flags = NVME_QUEUE_PHYS_CONTIG | nvmset_prio_flag(qprio);

    c.create_sq.opcode   = nvme_admin_create_sq;
    c.create_sq.prp1     = cpu_to_le64(nvmeq->sq_dma_addr);
    c.create_sq.sqid     = cpu_to_le16(qid);
    c.create_sq.qsize    = cpu_to_le16(nvmeq->q_depth - 1);  /* 0-based */
    c.create_sq.sq_flags = cpu_to_le16(flags);
    c.create_sq.cqid     = cpu_to_le16(qid);                 /* SQ:CQ = 1:1 */
    c.create_sq.nvmsetid = cpu_to_le16(nvmset_id);           /* ◄── CDW12 (§8.2) */
    return nvme_submit_sync_cmd(dev->ctrl.admin_q, &c, NULL, 0);
}

/* §22.2 의 nvme_create_queue(...) 실체. CQ→SQ 순 생성 + 벡터 + IRQ + 등록 */
static int nvmset_create_hw_queue(struct nvme_dev *dev, u16 qid, u16 nvmset_id,
                                  u8 qprio, u16 qsize)
{
    struct nvme_queue *nvmeq;
    int vector, ret;
    u16 depth = qsize ? qsize : dev->q_depth;

    nvmeq = nvme_alloc_queue(dev, qid, depth);    /* nvme_queue + SQ/CQ DMA(mainline) */
    if (!nvmeq)
        return -ENOMEM;

    vector = nvmset_acquire_vector(dev);
    if (vector < 0) { ret = vector; goto free_q; }
    nvmeq->cq_vector = vector;

    ret = adapter_alloc_cq(dev, qid, nvmeq, vector);              /* Create I/O CQ */
    if (ret) goto put_vec;
    ret = adapter_alloc_sq_nvmset(dev, qid, nvmeq, nvmset_id, qprio); /* Create I/O SQ */
    if (ret) goto del_cq;
    ret = nvmset_request_irq(dev, nvmeq);
    if (ret) goto del_sq;

    nvme_init_queue(nvmeq, qid);                  /* doorbell/상태 활성(mainline) */
    dev->queues[qid] = nvmeq;                     /* 전역 큐 테이블 등록 */
    dev->online_queues++;
    return 0;

del_sq:  adapter_delete_sq(dev, qid);
del_cq:  adapter_delete_cq(dev, qid);
put_vec: nvmset_release_vector(dev, vector);
free_q:  nvme_free_queue(nvmeq);
    return ret;
}

/* 단일 하드웨어 큐 파괴(역순) — §12 규칙① SQ→CQ */
static void nvmset_destroy_hw_queue(struct nvme_dev *dev, struct nvme_queue *nvmeq)
{
    u16 qid = nvmeq->qid;

    nvme_suspend_queue(nvmeq);                    /* IRQ/doorbell off, in-flight drain(mainline) */
    nvmeq->tags = NULL;                           /* §15 stale 디먹스 방지 */
    adapter_delete_sq(dev, qid);                  /* SQ 먼저 */
    adapter_delete_cq(dev, qid);                  /* CQ 나중 */
    nvmset_release_vector(dev, nvmeq->cq_vector);
    dev->queues[qid] = NULL;
    dev->online_queues--;
    nvme_free_queue_id(dev, qid);
    nvme_free_queue(nvmeq);
}
```

### 28.4 State 판정 / cmd_size / 예산

```c
/* ctrl->tagset 이 active(State D, SQ>0)인가 — dormant(State P)와 구분(§5/§1 #3) */
static inline bool nvme_ctrl_tagset_active(struct nvme_ctrl *ctrl)
{
    struct nvme_dev *dev = to_nvme_dev(ctrl);
    return ctrl->tagset &&
           dev->nvmset_state == NVMSET_STATE_D &&
           ctrl->tagset->nr_hw_queues > 0;
}

/* per-set tagset 의 cmd_size — mainline ctrl tagset 과 동일 계산 재사용(§24.1) */
static unsigned int nvme_pci_cmd_size(struct nvme_dev *dev)
{
    /* 실제 식은 커널 버전 의존: sizeof(struct nvme_iod) + PRP/SGL 디스크립터 풀.
     * mainline 이 dev->tagset.cmd_size 에 넣는 값을 그대로 쓰면 됨. */
    return sizeof(struct nvme_iod) +
           (NVME_MAX_SEGS * sizeof(struct scatterlist));
}

/* Σ(모든 그룹 nr_sqs) + 신규 1 ≤ max_io_queues 인가 (§8.1) — caller holds nvmset_lock */
static bool nvmset_budget_ok(struct nvme_dev *dev)
{
    struct nvmset_group *g;
    unsigned int used = dev->online_queues;       /* 이미 떠 있는 IO SQ 수(admin 제외) */
    (void)g;
    return used + 1 <= dev->max_io_queues;
}
```

### 28.5 State D grow/shrink + P 그룹 grow/shrink

```c
/* State D: 새 SQ 를 ctrl->tagset 에 흡수(D→D). caller holds nvmset_lock */
static int nvmset_grow_default(struct nvme_dev *dev, u16 qid, void __user *uarg)
{
    struct nvme_csq_create c;
    int ret;

    /* nvmset_create_hw_queue() 가 이미 dev->queues[qid] 등록·online_queues++ 함.
     * ctrl->tagset 의 hw queue 수를 현재 online IO 큐 수로 재매핑(mainline 패턴). */
    ret = blk_mq_update_nr_hw_queues(dev->ctrl.tagset, dev->online_queues - 1);
    if (ret)
        return ret;

    if (copy_from_user(&c, uarg, sizeof(c)))
        return -EFAULT;
    c.qid_out = qid;
    return copy_to_user(uarg, &c, sizeof(c)) ? -EFAULT : 0;
}

/* State D 큐 축소(D→D) */
static int nvmset_shrink_default(struct nvme_dev *dev, u16 qid)
{
    struct nvme_queue *nvmeq = dev->queues[qid];

    if (!nvmeq || nvmeq->group)                   /* per-set 큐를 D 경로로 지우면 안 됨 */
        return -EINVAL;
    nvmset_destroy_hw_queue(dev, nvmeq);
    return blk_mq_update_nr_hw_queues(dev->ctrl.tagset, dev->online_queues - 1);
}

/* 그룹 SQ 로컬 인덱스 검색 */
static int nvmset_sq_index(struct nvmset_group *g, u16 qid)
{
    unsigned int i;
    for (i = 0; i < g->nr_sqs; i++)
        if (g->sqs[i]->qid == qid)
            return i;
    return -1;
}

/* P→P [B] grow: 기존 그룹에 이미 만든 nvmeq 를 편입 후 tagset 확장.
 * (§22.2 csq_create 의 P-branch 끝에서 호출 — 보정점 #3) */
static int nvmset_group_grow(struct nvme_dev *dev, struct nvmset_group *g,
                             struct nvme_queue *nvmeq)
{
    nvmeq->group = g;
    g->sqs[g->nr_sqs++] = nvmeq;
    if (g->tagset_ready)                          /* 이미 ns 바인딩됨 → 큐 확장 */
        return blk_mq_update_nr_hw_queues(&g->tagset, g->nr_sqs);
    return 0;                                     /* 아직 lazy 전: 누적만(§9) */
}

/* P→P [C] shrink: 그룹에서 SQ 하나 제거(라운드로빈 매핑이라 인덱스 스왑 안전) */
static int nvmset_group_shrink(struct nvme_dev *dev, struct nvmset_group *g, u16 qid)
{
    int idx = nvmset_sq_index(g, qid);
    struct nvme_queue *victim;

    if (idx < 0)
        return -ENOENT;
    if (g->nr_sqs == 1 && g->tagset_ready)        /* 마지막 SQ 는 ns 있으면 못 뺌 */
        return -EBUSY;

    swap(g->sqs[idx], g->sqs[g->nr_sqs - 1]);     /* victim 을 끝으로 */
    victim = g->sqs[--g->nr_sqs];
    if (g->tagset_ready)
        blk_mq_update_nr_hw_queues(&g->tagset, g->nr_sqs);  /* freeze→remap→unfreeze */
    nvmset_destroy_hw_queue(dev, victim);
    return 0;
}
```

### 28.6 제어 핸들러 최종본 (§22.2 보강 반영)

```c
static int nvmset_csq_create(struct nvme_dev *dev, void __user *uarg)
{
    struct nvme_csq_create c;
    struct nvmset_group *g;
    struct nvme_queue *nvmeq;
    int qid, ret;

    if (copy_from_user(&c, uarg, sizeof(c)))
        return -EFAULT;
    guard(mutex)(&dev->nvmset_lock);                       /* §26 */

    /* all-or-nothing 불변식(§1 #2) */
    if (dev->nvmset_state == NVMSET_STATE_P && c.nvmset_id == 0)
        return -EBUSY;                                     /* P 에 D 큐 금지 */
    if (dev->nvmset_state == NVMSET_STATE_D && c.nvmset_id != 0 &&
        nvme_ctrl_tagset_active(&dev->ctrl))
        return -EBUSY;                                     /* D active 와 P 혼용 금지(§13 선행 teardown) */
    if (!nvmset_budget_ok(dev))
        return -ENOSPC;                                    /* §8.1 예산 */

    qid = nvme_alloc_queue_id(dev);
    if (qid < 0)
        return qid;

    ret = nvmset_create_hw_queue(dev, qid, c.nvmset_id, c.qprio, c.qsize);
    if (ret) {
        nvme_free_queue_id(dev, qid);
        return ret;
    }
    nvmeq = dev->queues[qid];

    if (c.nvmset_id == 0)                                  /* State D: 기본 그룹 흡수 */
        return nvmset_grow_default(dev, qid, uarg);

    /* State P: 그룹 편입(+필요 시 grow) */
    g = nvmset_group_get_or_create(dev, c.nvmset_id);
    if (!g) {
        nvmset_destroy_hw_queue(dev, nvmeq);              /* rollback */
        return -ENOMEM;
    }
    ret = nvmset_group_grow(dev, g, nvmeq);               /* 누적 or update_nr_hw_queues */
    if (ret) {
        nvmset_destroy_hw_queue(dev, nvmeq);
        return ret;
    }
    dev->nvmset_state = NVMSET_STATE_P;
    nvme_queue_scan(&dev->ctrl);                          /* §23.4 보류 ns 재시도(비동기) */

    c.qid_out = qid;
    return copy_to_user(uarg, &c, sizeof(c)) ? -EFAULT : 0;
}

static int nvmset_csq_delete(struct nvme_dev *dev, void __user *uarg)
{
    struct nvme_csq_delete c;
    struct nvmset_group *g;

    if (copy_from_user(&c, uarg, sizeof(c)))
        return -EFAULT;
    guard(mutex)(&dev->nvmset_lock);

    if (c.nvmset_id == 0)                                  /* State D 큐 축소 */
        return nvmset_shrink_default(dev, c.qid);

    g = nvmset_find_group(dev, c.nvmset_id);
    if (!g)
        return -ENOENT;

    if (c.qid)                                            /* 그룹 내 SQ 1개 축소(§12[C]) */
        return nvmset_group_shrink(dev, g, c.qid);

    /* 그룹 전체 삭제(§12[D]): ns 제거 → tagset/SQ 해제 */
    nvmset_remove_group_namespaces(dev, g);
    nvmset_group_free(dev, g);
    if (list_empty(&dev->group_list))
        dev->nvmset_state = NVMSET_STATE_D;              /* 마지막 그룹 → 무큐 중간상태(§1 #4) */
    return 0;
}
```

### 28.7 lazy tagset + selector (refcount 제거 반영, §23.3 갱신본)

```c
struct blk_mq_tag_set *
nvme_pci_ns_tagset(struct nvme_ctrl *ctrl, struct nvme_ns_info *info, bool *defer)
{
    struct nvme_dev *dev = to_nvme_dev(ctrl);
    struct nvmset_group *g;

    *defer = false;
    guard(mutex)(&dev->nvmset_lock);                      /* 그룹 조회 보호(§26) */

    if (info->nvmset_id == 0) {                           /* State D 경로 */
        if (nvme_ctrl_tagset_active(ctrl))
            return ctrl->tagset;
        *defer = true; return NULL;                       /* P 인데 D ns → 보류 */
    }
    g = nvmset_find_group(dev, info->nvmset_id);
    if (!g || g->nr_sqs == 0) { *defer = true; return NULL; }   /* 그룹 큐 아직 없음 */

    if (!g->tagset_ready) {                                /* ★ per-set tagset lazy 생성(§9/§24.1) */
        if (nvmset_alloc_tag_set(dev, g)) { *defer = true; return NULL; }
    }
    return &g->tagset;                                     /* refcount 불요 — ns 는 head->nvmset_id 로 열거 */
}
```

### 28.8 core 측 defer 보조 (선택적 최적화)

```c
/* dev->deferred_ns 리스트 항목 */
struct nvme_deferred_ns { struct list_head entry; u32 nsid; };

/* 보류 등록(중복 방지). rescan 만으로도 재시도되지만, 대량 ns 에서 타깃 재시도용(§23.4) */
static void nvme_defer_ns(struct nvme_ctrl *ctrl, u32 nsid)
{
    struct nvme_dev *dev = to_nvme_dev(ctrl);
    struct nvme_deferred_ns *d;

    guard(mutex)(&dev->nvmset_lock);
    list_for_each_entry(d, &dev->deferred_ns, entry)
        if (d->nsid == nsid) return;                      /* 이미 보류됨 */
    d = kzalloc(sizeof(*d), GFP_KERNEL);
    if (!d) return;                                       /* OOM 시 rescan 폴백 */
    d->nsid = nsid;
    list_add_tail(&d->entry, &dev->deferred_ns);
}
```

> **함수 완결성 체크**: §22~§25 에서 호출된 미정의 심볼 — `nvmset_find_group`, `nvmset_group_get_or_create`, `nvmset_group_free`, `nvmset_remove_group_namespaces`, `nvme_alloc_queue_id`, `nvme_free_queue_id`, `nvmset_acquire/release_vector`, `nvmset_request_irq`, `nvmset_prio_flag`, `adapter_alloc_sq_nvmset`, `nvmset_create_hw_queue`(=§22 의 `nvme_create_queue`), `nvmset_destroy_hw_queue`, `nvme_ctrl_tagset_active`, `nvme_pci_cmd_size`, `nvmset_budget_ok`, `nvmset_grow_default`, `nvmset_shrink_default`, `nvmset_sq_index`, `nvmset_group_grow`, `nvmset_group_shrink`, `nvmset_csq_create`, `nvmset_csq_delete`, `nvme_pci_ns_tagset`, `nvme_defer_ns` — **전부 정의 완료.** mainline 재사용(무수정): `nvme_alloc_queue`, `nvme_free_queue`, `nvme_init_queue`, `nvme_suspend_queue`, `adapter_alloc_cq`, `adapter_delete_sq/cq`, `nvme_submit_sync_cmd`, `nvme_irq`, `nvme_ns_remove`, `blk_mq_update_nr_hw_queues`, `blk_mq_alloc_tag_set`, `blk_mq_free_tag_set`, `nvme_queue_scan`.

---

## 29. ★ 개정 정본 — endurance_group 기반 모델 (앵커 구조체 + rescan 큐맵 구성)

> 이 절이 앵커 자료구조와 "생성→맵 구성" 타이밍의 **정본**이다. §7·§9·§20.1·§22.2·§23.3·§24.1/§24.3/§24.4(init_hctx)·§28.1/§28.6/§28.7 의 해당 부분을 대체한다. 그대로 유지되는 것: 완료 디먹스 원리(§15), QPRIO/하드웨어 큐 생성(§28.3), qid/벡터 회계(§28.2), reset Option B(§25, 단 순회 단위는 entry).
>
> **핵심 차이 3가지** (기존안 → 개정안):
> 1. 앵커 = **`endurance_group`(ENDGRP별 list) → `nvmset_entry`(NVM Set별 tagset)** 2계층. (기존 `nvmset_group` 단일 계층)
> 2. **CSQ_CREATE = 기록 전용** — 하드웨어 SQ 생성 후 `nvmset_entry.sqids[]` 에 sqid append 만. tagset/큐맵 안 만들고 **rescan 트리거도 안 함**.
> 3. **큐맵 = rescan 때 일괄 구성** — 모든 큐 생성 후 ns attach→rescan(§13) 시, 그 ns 의 NVMSETID 로 `nvmset_entry` 를 찾아 **기록된 sqid 목록으로** tagset(`nr_hw_queues=nr_sqs`)과 `hctx→SQ` 맵을 만든다.

### 29.1 자료구조 (정본 — §7/§20.1 대체)

```c
/* drivers/nvme/host/pci.h (개정) */

enum nvmset_state { NVMSET_STATE_D, NVMSET_STATE_P };       /* §1 D/P 그대로 */

struct nvmset_entry {                  /* NVM Set 1개 = blk-mq tagset 격리 단위 */
    u16                   nvmset_id;    /* 이 set 의 NVM Set ID (>=1) */
    u16                  *sqids;        /* ★ 이 set 에 포함된 SQ id 목록(CSQ_CREATE 순서) */
    unsigned int          nr_sqs;       /* sqids 유효 개수 = tagset.nr_hw_queues */
    struct blk_mq_tag_set tagset;       /* rescan 때 sqids 로 구성(일시 생성/소멸) */
    bool                  tagset_ready; /* lazy 빌드 가드 */
    struct nvme_dev      *dev;          /* init_hctx 에서 dev->queues[sqid] 해석용 back-ptr */
    struct list_head      entry;        /* endurance_group.sets 연결 */
};

struct nvme_endurance_group {           /* ENDGRP 컨테이너 (1 ENDGRP : N NVM Set) */
    u16               endgrp_id;        /* Endurance Group ID */
    struct list_head  sets;             /* nvmset_entry 목록 */
    unsigned int      nr_sets;
    struct list_head  entry;            /* nvme_dev.endgrp_list 연결 */
};

struct nvme_dev {                       /* 확장 (기존 필드 그대로 + 아래) */
    struct list_head   endgrp_list;     /* ★ nvme_endurance_group 목록 */
    unsigned int       nr_endgrps;
    enum nvmset_state  nvmset_state;
    struct mutex       nvmset_lock;     /* CSQ ↔ reset ↔ scan 직렬화(§26) */
    struct ida         qid_ida;         /* IO qid 1..max_qid */
    struct ida         vec_ida;         /* MSI-X 1..num_vecs-1 */
    struct list_head   deferred_ns;     /* 엣지: 큐보다 ns 가 먼저인 경우 보류 */
};

/* struct nvme_queue 확장(§20.1 동일, 이름만): */
struct nvme_queue {
    /* ... 기존 ... */
    struct blk_mq_tags  *tags;          /* = hctx->tags. 완료 디먹스(§15) */
    struct nvmset_entry *set;           /* 이 SQ 가 속한 NVM Set(기본그룹이면 NULL) */
};
```

> **NVM Set ID 유일성**: NVM Set ID 는 컨트롤러(서브시스템) 범위에서 유일하므로, ns→entry 조회는 endgrp 를 가로질러 `nvmset_id` 키 하나로 충분하다(§29.5 `nvmset_find_entry`). `endgrp_id` 는 *배치(컨테이너)* 와 ENDGRP 단위 정책(추후 WRR/예산)에만 쓰인다.

### 29.2 UAPI 개정 — `endgrp_id` 추가 (§21 대체)

```c
struct nvme_csq_create {
    __u16 endgrp_id;   /* 이 SQ 가 속할 Endurance Group (배치 컨테이너) */
    __u16 nvmset_id;   /* 0 = State D(ctrl->tagset). >=1 = State P(해당 NVM Set) */
    __u16 qsize;       /* SQ 깊이(0→기본) */
    __u8  qprio;       /* WRR 우선순위(§28.3) */
    __u8  flags;       /* 예약(0) */
    __u16 qid_out;     /* [out] 배정 qid */
    __u16 rsvd;
};
/* DELETE 는 §21 그대로(+endgrp_id 선택). 번호도 §21 동일. */
```
> `endgrp_id` 는 Identify(NVM Set/Endurance Group List)로 검증 가능하나, **private ioctl 이므로 orchestration 이 직접 지정**하는 편이 단순(§8 "제어면은 드라이버 전용"). 미지정(0) 시 단일 ENDGRP 로 폴백.

### 29.3 CSQ_CREATE = 기록 전용 (§22.2/§28.6 create 대체)

```c
static int nvmset_csq_create(struct nvme_dev *dev, void __user *uarg)
{
    struct nvme_csq_create c;
    struct nvme_endurance_group *eg;
    struct nvmset_entry *se;
    struct nvme_queue *nvmeq;
    int qid, ret;

    if (copy_from_user(&c, uarg, sizeof(c)))                 return -EFAULT;
    guard(mutex)(&dev->nvmset_lock);

    /* all-or-nothing 불변식(§1 #2) */
    if (dev->nvmset_state == NVMSET_STATE_P && c.nvmset_id == 0)              return -EBUSY;
    if (dev->nvmset_state == NVMSET_STATE_D && c.nvmset_id != 0 &&
        nvme_ctrl_tagset_active(&dev->ctrl))                                  return -EBUSY;
    if (!nvmset_budget_ok(dev))                                              return -ENOSPC;

    qid = nvme_alloc_queue_id(dev);
    if (qid < 0)                                             return qid;

    /* 하드웨어 SQ/CQ 는 지금 만든다(§28.3). 단 tagset/맵은 만들지 않음. */
    ret = nvmset_create_hw_queue(dev, qid, c.nvmset_id, c.qprio, c.qsize);
    if (ret) { nvme_free_queue_id(dev, qid); return ret; }
    nvmeq = dev->queues[qid];

    if (c.nvmset_id == 0)                                    /* State D: 기본 그룹 흡수 */
        return nvmset_grow_default(dev, qid, uarg);

    /* ★ State P: 'endurance_group → nvmset_entry' 테이블에 sqid 기록만 */
    eg = nvme_endgrp_get_or_create(dev, c.endgrp_id);
    if (!eg)            { nvmset_destroy_hw_queue(dev, nvmeq); return -ENOMEM; }
    se = nvmset_entry_get_or_create(dev, eg, c.nvmset_id);
    if (!se)            { nvmset_destroy_hw_queue(dev, nvmeq); return -ENOMEM; }

    se->sqids[se->nr_sqs++] = qid;                           /* 기록 */
    nvmeq->set = se;
    dev->nvmset_state = NVMSET_STATE_P;

    /* ★ rescan 트리거 안 함 — provisioning 은 '모든 큐 생성 후' ns attach→rescan(§13).
     *   이미 tagset 이 떠 있던(=ns 가 있던) set 에 한해서만 grow 반영: */
    if (se->tagset_ready)
        blk_mq_update_nr_hw_queues(&se->tagset, se->nr_sqs);  /* P→P [B] 동적 grow */

    c.qid_out = qid;
    return copy_to_user(uarg, &c, sizeof(c)) ? -EFAULT : 0;
}
```

> 기존안과의 차이: ① `nvme_queue_scan()` 호출 제거(정상 흐름은 ns attach 가 rescan 유발). ② SQ 를 포인터 배열이 아니라 **sqid 번호로 기록**. ③ tagset 은 여기서 안 만든다(§29.4 가 만든다).

### 29.4 rescan 큐맵 구성 (정본 — §9·§23.3·§24 init_hctx 대체)

ns attach AEN → `nvme_scan_work` → `nvme_alloc_ns`(core) → selector 콜백에서 **기록 테이블로 tagset+맵을 구성**한다.

```c
/* (pci-nvmset.c) nvme_ctrl_ops.ns_tagset 구현 — rescan 시 호출 */
struct blk_mq_tag_set *
nvme_pci_ns_tagset(struct nvme_ctrl *ctrl, struct nvme_ns_info *info, bool *defer)
{
    struct nvme_dev *dev = to_nvme_dev(ctrl);
    struct nvmset_entry *se;

    *defer = false;
    guard(mutex)(&dev->nvmset_lock);

    if (info->nvmset_id == 0) {                              /* State D */
        if (nvme_ctrl_tagset_active(ctrl)) return ctrl->tagset;
        *defer = true; return NULL;                          /* P 인데 D ns → 보류 */
    }
    se = nvmset_find_entry(dev, info->nvmset_id);            /* 기록 테이블 조회 */
    if (!se || se->nr_sqs == 0) { *defer = true; return NULL; }  /* 큐 기록 없음 → 보류 */

    if (!se->tagset_ready) {                                 /* ★ 최초 ns 때 sqids 로 빌드 */
        if (nvmset_build_tagset(dev, se)) { *defer = true; return NULL; }
    }
    return &se->tagset;                                      /* 이 set 의 모든 ns 가 공유 */
}

/* sqids 목록 → tagset 구성. nr_hw_queues = 기록된 SQ 수 */
static int nvmset_build_tagset(struct nvme_dev *dev, struct nvmset_entry *se)
{
    struct blk_mq_tag_set *set = &se->tagset;

    memset(set, 0, sizeof(*set));
    set->ops          = &nvmset_mq_ops;
    set->nr_hw_queues = se->nr_sqs;                          /* ★ = sqids 개수 */
    set->nr_maps      = 1;                                   /* phase1: DEFAULT */
    set->queue_depth  = dev->ctrl.sqsize + 1;
    set->numa_node    = dev->ctrl.numa_node;
    set->cmd_size     = nvme_pci_cmd_size(dev);
    set->timeout      = NVME_IO_TIMEOUT;
    set->flags        = BLK_MQ_F_SHOULD_MERGE;               /* TAG_HCTX_SHARED 금지(§14 G6) */
    set->driver_data  = se;                                  /* map/init_hctx 가 sqids 접근 */

    if (blk_mq_alloc_tag_set(set))                           /* → map_queues + init_hctx 콜백 */
        return -ENOMEM;
    se->tagset_ready = true;
    return 0;
}

/* .map_queues — 라운드로빈(§8.3) */
static void nvmset_map_queues(struct blk_mq_tag_set *set)
{
    struct nvmset_entry *se = set->driver_data;
    struct blk_mq_queue_map *map = &set->map[HCTX_TYPE_DEFAULT];
    map->nr_queues = se->nr_sqs;  map->queue_offset = 0;
    blk_mq_map_queues(map);
}

/* .init_hctx — ★ 로컬 idx → 기록된 sqid → dev->queues[sqid] 로 nvme_queue 해석 + 디먹스 캐시(§15) */
static int nvmset_init_hctx(struct blk_mq_hw_ctx *hctx, void *data, unsigned int hctx_idx)
{
    struct nvmset_entry *se = data;
    struct nvme_dev *dev = se->dev;
    u16 sqid = se->sqids[hctx_idx];
    struct nvme_queue *nvmeq = dev->queues[sqid];           /* sqid → 실제 SQ */

    nvmeq->tags = hctx->tags;                               /* 완료 디먹스 정답 풀 캐시 */
    hctx->driver_data = nvmeq;
    return 0;
}
```

`nvme_queue_tagset()`(완료, §24.4/§15)는 그대로 — `return nvmeq->tags;`. 즉 **"sqid 목록"은 제출측 맵 구성에만 쓰이고, 완료측은 init_hctx 가 캐시한 `nvmeq->tags` 로 O(1) 역매핑**되어 §15 정합이 유지된다.

> §9 와의 관계: 기존 §9 는 "첫 ns 바인딩에서 lazy 생성"이라 했고, 개정안도 트리거는 동일(첫 ns)하되 **재료가 `g->nr_sqs`(누적 카운트)에서 `se->sqids[]`(명시적 목록)로** 바뀐다. 명시 목록이라 grow/shrink 후에도 어떤 sqid 가 어느 hctx 인덱스인지 추적이 명확하다.

### 29.5 보조 함수 (정본 — §28.1 그룹 관리 대체)

```c
/* ── 조회 ── caller holds nvmset_lock */
static struct nvme_endurance_group *nvme_endgrp_find(struct nvme_dev *dev, u16 id)
{
    struct nvme_endurance_group *eg;
    list_for_each_entry(eg, &dev->endgrp_list, entry)
        if (eg->endgrp_id == id) return eg;
    return NULL;
}
static struct nvmset_entry *nvmset_find_entry(struct nvme_dev *dev, u16 nvmset_id)
{
    struct nvme_endurance_group *eg;
    struct nvmset_entry *se;
    list_for_each_entry(eg, &dev->endgrp_list, entry)
        list_for_each_entry(se, &eg->sets, entry)
            if (se->nvmset_id == nvmset_id) return se;     /* NVM Set ID 유일 */
    return NULL;
}

/* ── 생성 ── caller holds nvmset_lock */
static struct nvme_endurance_group *nvme_endgrp_get_or_create(struct nvme_dev *dev, u16 id)
{
    struct nvme_endurance_group *eg = nvme_endgrp_find(dev, id);
    if (eg) return eg;
    eg = kzalloc(sizeof(*eg), GFP_KERNEL);
    if (!eg) return NULL;
    eg->endgrp_id = id;
    INIT_LIST_HEAD(&eg->sets);
    list_add_tail(&eg->entry, &dev->endgrp_list);
    dev->nr_endgrps++;
    return eg;
}
static struct nvmset_entry *
nvmset_entry_get_or_create(struct nvme_dev *dev, struct nvme_endurance_group *eg, u16 nvmset_id)
{
    struct nvmset_entry *se = nvmset_find_entry(dev, nvmset_id);
    if (se) return se;                                      /* 이미 있으면 누적 대상 */
    se = kzalloc(sizeof(*se), GFP_KERNEL);
    if (!se) return NULL;
    se->sqids = kcalloc(dev->max_qid, sizeof(u16), GFP_KERNEL);  /* 상한 = 전역 qid 수 */
    if (!se->sqids) { kfree(se); return NULL; }
    se->nvmset_id = nvmset_id;
    se->dev = dev;
    list_add_tail(&se->entry, &eg->sets);
    eg->nr_sets++;
    return se;
}

/* ── 해제 ── caller holds nvmset_lock. 전제: 소속 ns 전부 제거됨 */
static void nvmset_entry_free(struct nvme_dev *dev, struct nvme_endurance_group *eg,
                             struct nvmset_entry *se)
{
    int i;
    if (se->tagset_ready) blk_mq_free_tag_set(&se->tagset);          /* §13 조건1 */
    for (i = se->nr_sqs - 1; i >= 0; i--)                            /* SQ→CQ 순(§12 규칙①) */
        nvmset_destroy_hw_queue(dev, dev->queues[se->sqids[i]]);
    list_del(&se->entry);
    eg->nr_sets--;
    kfree(se->sqids);
    kfree(se);
}
static void nvme_endgrp_free(struct nvme_dev *dev, struct nvme_endurance_group *eg)
{
    struct nvmset_entry *se, *t;
    list_for_each_entry_safe(se, t, &eg->sets, entry) {
        nvmset_remove_entry_namespaces(dev, se);                     /* del_gendisk(§28.1 변형) */
        nvmset_entry_free(dev, eg, se);
    }
    list_del(&eg->entry);
    dev->nr_endgrps--;
    kfree(eg);
}

/* 이 NVM Set 에 바인딩된 ns 제거 — §28.1 nvmset_remove_group_namespaces 의 entry 판 */
static void nvmset_remove_entry_namespaces(struct nvme_dev *dev, struct nvmset_entry *se)
{
    struct nvme_ctrl *ctrl = &dev->ctrl;
    struct nvme_ns *ns, *next;  LIST_HEAD(rm);
    down_write(&ctrl->namespaces_rwsem);
    list_for_each_entry_safe(ns, next, &ctrl->namespaces, list)
        if (ns->head->nvmset_id == se->nvmset_id) list_move_tail(&ns->list, &rm);
    up_write(&ctrl->namespaces_rwsem);
    list_for_each_entry_safe(ns, next, &rm, list) nvme_ns_remove(ns);
}
```

`nvmset_csq_delete`(§28.6)·`nvmset_teardown_overlay`(§25)는 위 entry 헬퍼로 치환: 그룹 삭제 = `nvme_endgrp_free`/`nvmset_entry_free`, shrink = `se->sqids[]` 에서 해당 sqid 스왑-제거 후 `blk_mq_update_nr_hw_queues(&se->tagset, --se->nr_sqs)`.

### 29.6 그대로 유지되는 부분 (재사용)

| 유지 | 절 | 비고 |
|------|----|------|
| 완료 디먹스(`nvmeq->tags` 캐시 + `nvme_queue_tagset`) | §15·§24.4 | init_hctx 가 sqid→nvmeq 로 해석만 바뀜 |
| 하드웨어 큐 생성(CDW12 NVMSETID, QPRIO) | §28.3 | `nvmset_create_hw_queue`/`adapter_alloc_sq_nvmset` 동일 |
| qid/MSI-X 회계, State 판정, cmd_size, 예산 | §28.2·§28.4 | 동일 |
| selector 훅·core 통합(NVMSETID 채집·defer) | §23.1·§23.2·§23.4 | `nvme_ns_info.nvmset_id` 채집 동일, entry 조회만 변경 |
| reset Option B | §25 | 순회 단위가 group_list → **endgrp_list→sets** |
| 락 순서 | §26 | 동일(`nvmset_lock`) |

### 29.7 한눈에 보는 흐름 (개정)

```
[ioctl] CSQ_CREATE(endgrp=E, nvmset=k) × N
   └ nvmset_create_hw_queue(qid)            : 하드웨어 SQ/CQ 생성(CDW12=k)
   └ endgrp[E].set[k].sqids[nr++] = qid     : ★ 기록만 (tagset/맵 X, rescan X)
        ... 모든 큐 생성 완료 ...
[admin] ns create & attach (ENDGRP E, NVMSET k)  → AEN
[scan]  nvme_scan_work → nvme_alloc_ns → ns_tagset(info.nvmset_id=k):
           se = find_entry(k)
           if !se->tagset_ready: build_tagset(se)   ★ sqids[] 로 nr_hw_queues 구성
                                                       map_queues(라운드로빈)
                                                       init_hctx: sqid→dev->queues[sqid], tags 캐시
           return &se->tagset → blk_mq_alloc_disk → /dev/nvme0nX
```

---

## 부록 — 참조

- **NVMe spec**: Create I/O Submission/Completion Queue (CDW10~12, NVMSETID/QPRIO) · NVM Sets, Identify NVM Set List(CNS 04h), Identify Namespace 의 NVMSETID · Arbitration: WRR with Urgent Priority Class(`CAP.AMS`/`CC.AMS`)
- **Linux**: `drivers/nvme/host/{pci.c, core.c, ioctl.c, multipath.c}` · blk-mq(`block/blk-mq.c`, `struct blk_mq_tag_set`)
- **관련 노트**: [[nvme-driver.md]], [[sq-mapping-comparison.md]], [[io_submit-to-nvme_queue_rq-flow.md]], [[per-io-timeout-iocb-proposal.md]]
- **코드 구조 분석 노트**: [[nvme-queue-mgmt-design]], [[nvme-queue-init-request-queue-tagset-hctx]], [[nvme-tagset-blkmq-flow]] (deep/analysis/kernel/) — tagset/hctx/SQ 매핑 실코드 근거
