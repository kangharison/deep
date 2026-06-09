# NVMe Namespace(NVM Set)별 SQ 격리 커스텀 드라이버 — 설계

> 기준 코드: `sources/linux/drivers/nvme/host/{pci.c, core.c, ioctl.c, multipath.c}`, blk-mq(`block/blk-mq.c`)
> 기준 스펙: NVMe Base Spec — Create I/O SQ/CQ, NVM Sets, Identify, Arbitration(WRR)
> 작성: 2026-06-09 · 상태: 설계 확정(코드 착수 전)

**한 줄 요약**: NVMe IO SQ 를 **NVM Set(=namespace) 단위로 분리**해 namespace 별 전용 tagset 에 묶어 격리한다. 큐 생성은 **ioctl 로 NVMSETID 를 실은 Create I/O SQ** 를 발행하는 커스텀 드라이버가 담당하며, blk-mq 데이터패스(`nvme_queue_rq`)는 그대로 재사용한다. 이후 thread 격리는 cgroup 으로 얹는다.

---

## 1. 목표와 범위

- **목표**: namespace 별로 IO 큐(SQ)를 분리하고 tagset/driver SQ 를 모두 분리 → noisy-neighbor 격리. 추후 application thread 를 특정 namespace 큐에만 접근시키는 QoS 격리.
- **구현 형태**: 커널 측 커스텀 드라이버(mainline `nvme-pci` fork/확장). `create io queue` 등은 **ioctl 로 전달**.
- **분할 키**: namespace 직접이 아니라 **NVM Set ID**. namespace 는 NVM Set 에 소속(`Identify Namespace` 의 NVMSETID).
- **데이터패스**: blk-mq 제출/완료 경로 100% 재사용. 신규 작업은 "tagset 다중화 + 그룹별 매핑 + ns 바인딩 + ioctl 제어면"뿐.
- **범위 밖**: 유저스페이스(SPDK/VFIO) poll-mode.

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
| 2 | **공용 overflow 없음**: ns 는 한 그룹에만 전속(전용 ∪ 기본 중 하나) | CID 충돌·커스텀 엔진·정합성 모호성 제거 |
| 3 | **blk-mq 데이터패스 재사용** | `nvme_queue_rq` 무수정, tagset 다중화만 추가 |
| 4 | **tagset 소유 = 영속 default + 일시 per-set** | probe 의 `ctrl->tagset` 을 기본 그룹으로 재활용, 침습 최소 |
| 5 | **provisioning = queue-first** | tagset 이 ns 스캔 전에 준비 → 바인딩 즉시(mainline 동일 패턴) |
| 6 | **분할 키 = NVMSETID**, Create I/O SQ CDW12 로 전달 | NVMe 표준 경로 |
| 7 | **thread 격리 = cpuset cgroup** (후속) | 커스텀 제출 엔진 불필요 |

> **버린 대안**: ① 공용 tagset + thread 소유 전용 SQ(passthrough) → 커스텀 제출/완료/CID/DMA 엔진 + 정합성 위험으로 폐기. ② probe 단일 tagset 제거 후 전량 ioctl 할당 → `ctrl->tagset` 재활용(결정 4)이 더 단순.

```
nvme_dev
 ├─ admin queue (qid 0)
 ├─ ctrl->tagset  ............... 기본 그룹(NVMSETID 0). 영속. probe 생성.
 ├─ group[1].tagset (NVMSETID 1)  per-set. 일시. ioctl 로 생성.
 └─ group[2].tagset (NVMSETID 2)  per-set. 일시.

 ns(NVMSETID 0) → ctrl->tagset      |  ns(NVMSETID k) → group[k].tagset
```

---

## 5. tagset 소유 모델 — 영속 default + 일시 per-set

- **`ctrl->tagset` = 기본 그룹 = NVMSETID 0.** probe(`pci.c:3136/3777`, `nvme_alloc_io_tag_set`)에서 생성되어 **영속**. NVM Set 미소속 ns 의 home, 그리고 "원복된 기본 상태"의 home.
- **`group[k].tagset` (k≥1) = per-set 그룹.** `create io queue (NVMSETID=k)` ioctl 로 생성되고, 전체 큐 삭제(원복) 시 `free_tag_set` 으로 **소멸하는 일시 오버레이**.
- **`ns->tagset` 은 실체 소유가 아니라 selector** — `nvmset_id ? group[id].tagset : ctrl->tagset`.

```c
/* nvme_alloc_ns, core.c:4153 (변경점) */
struct blk_mq_tag_set *ts =
        ns->nvmset_id ? &group[ns->nvmset_id].tagset   /* per-set (일시) */
                      : ctrl->tagset;                    /* default (영속) */
disk = blk_mq_alloc_disk(ts, &lim, ns);
```

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

## 8. 제어 경로 — ioctl + Create I/O SQ (NVMSETID)

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

### 8.3 tagset 매핑 콜백
- 그룹 전용 `.map_queues` 는 **그 Set 의 SQ들만**으로 CPU→hctx 맵 생성 → 다른 Set SQ 로 누수 없음.
- `.init_hctx` 에서 `hctx->driver_data = group.sqs[hctx_idx]` 로 hctx↔SQ 결선.
- `nr_hw_queues > 온라인 CPU` → 일부 hctx 유휴 또는 poll 용.

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

**채택: queue-first** — IO 큐(그룹 tagset)를 먼저, ns 를 나중에. mainline 자연 순서(`ctrl->tagset` 먼저 → ns 스캔)를 tagset N개로 일반화. ns 스캔 시 tagset 준비됨 → 바인딩 즉시, deferred add_disk 불필요.

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
3. 1단계 상세: `nvmset_group` 필드 확정, ioctl 인터페이스, 그룹 전용 `.map_queues`/`.init_hctx`, `nvme_queue_tagset()` 재작성.

---

## 부록 — 참조

- **NVMe spec**: Create I/O Submission/Completion Queue (CDW10~12, NVMSETID/QPRIO) · NVM Sets, Identify NVM Set List(CNS 04h), Identify Namespace 의 NVMSETID · Arbitration: WRR with Urgent Priority Class(`CAP.AMS`/`CC.AMS`)
- **Linux**: `drivers/nvme/host/{pci.c, core.c, ioctl.c, multipath.c}` · blk-mq(`block/blk-mq.c`, `struct blk_mq_tag_set`)
- **관련 노트**: [[nvme-driver.md]], [[sq-mapping-comparison.md]], [[io_submit-to-nvme_queue_rq-flow.md]], [[per-io-timeout-iocb-proposal.md]]
