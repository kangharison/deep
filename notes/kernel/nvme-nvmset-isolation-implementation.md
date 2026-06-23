# NVMe NVM Set별 SQ/tagset 격리 드라이버 — 구현/결정 기록

> 설계 정본: [[nvme-namespace-sq-isolation-driver]] (이 문서는 그 설계를 **실제 코드로 구현**하며 내린 단순화·결정·포팅 기록)
> 대상 코드: `linux-study/drivers/nvme/host/{pci.c, core.c, ioctl.c, nvme.h, nvmset.h}`, `linux-study/include/linux/nvme.h`
> 저장소: `github.com/kangharison/linux-study`
> - `master` — kernel **7.0.0** 기반 구현
> - `nvme-nvmset-6.1.4` — kernel **6.1.4** 포팅 (root 커밋 1d51944)
> 작성: 2026-06-23 · 상태: 1단계 구현 완료(빌드 미검증), 런타임 게이트(NVMSETID)로 동작

**한 줄 요약**: NVMe IO SQ 를 NVM Set(NVMSETID) 단위로 분리해 namespace 별 전용 blk-mq tagset 에 묶어 격리한다. 큐 생성은 private ioctl `CSQ_CREATE`(NVMSETID 를 실은 Create I/O SQ 발행)가 담당하고, blk-mq 데이터패스(`nvme_queue_rq`/완료)는 무수정 재사용한다. 격리 사용 여부는 **컴파일 옵션 없이 "CSQ_CREATE 에 NVMSETID 를 주는지"로 런타임 결정**.

---

## 1. 설계(정본) 대비 단순화

설계 §29 의 정본(2계층 endurance_group)이 아니라 **단일 계층 `nvmset_group` 모델**로 구현했다. 학습/개인용 단순화이며, 핵심 3가지만 살렸다:

| 구현함 | 생략함(추후) |
|--------|--------------|
| per-set tagset 격리 (`nvmset_group`) | endurance_group 2계층 (`nvmset_entry`) |
| NVMSETID Create I/O SQ (CDW12) | State D/P all-or-nothing 강제 |
| 완료 디먹스 재작성 (`nvmeq->tags` 캐시) | reset Option B (per-set teardown 후 baseline) |
| selector + lazy per-set tagset | 동적 grow/shrink, deferred_ns 리스트 |
| CSQ_CREATE/DELETE ioctl | 비연속 qid(gap), WRR 큐 예산 정책 |

기본 그룹(`ctrl->tagset`)과 per-set 그룹의 **공존을 허용**한다(엄격한 D/P 전이 미강제). teardown 은 driver detach(`nvme_remove`)에서 일괄.

---

## 2. 변경 파일 맵

| 파일 | 변경 |
|------|------|
| `drivers/nvme/host/nvmset.h` (신규) | private ioctl UAPI: `NVME_IOCTL_CSQ_CREATE/DELETE` + 인자 구조체 |
| `include/linux/nvme.h` | `struct nvme_create_sq` 의 CDW12 를 `nvmsetid` 필드로 명명 |
| `drivers/nvme/host/nvme.h` | `nvme_ctrl_ops` 에 `ns_tagset`/`nvmset_ctl` 콜백, `nvme_ns_head.nvmset_id` |
| `drivers/nvme/host/core.c` | NVMSETID 채집 + `nvme_alloc_ns` selector 훅 + `ns_head` 보관 |
| `drivers/nvme/host/ioctl.c` | `nvme_dev_ioctl` 에서 `CSQ_*` 를 `nvmset_ctl` 콜백으로 디스패치 |
| `drivers/nvme/host/pci.c` | `nvmset_group`, dev/queue 확장, 완료 디먹스, per-set 분기, CSQ 핸들러, lazy tagset, probe/remove 통합 |

> **레이어 분리 원칙**(설계 §19.1): core(`core.c`)는 NVMSETID 를 *모르고* `ns_tagset` 콜백만 호출 → 콜백 NULL 이면 mainline 동작 100% 보존. 모든 NVM Set 지식은 PCIe 모듈(`pci.c`)에 격리. 콜백은 PCIe 컨트롤러에서만 등록되고 비-PCIe(tcp/rdma/fc) 트랜스포트는 NULL → mainline.

---

## 3. 핵심 동작 흐름

```
[ioctl] CSQ_CREATE(nvmset_id=k, qid=N) × M
   └ nvmset_create_hw_queue(qid=N): 하드웨어 SQ/CQ 생성(Create I/O SQ CDW12=k)
   └ group[k].sqids[nr++] = N        : 기록만 (tagset/맵 아직 X = queue-first)
        ... 모든 큐 생성 후 ...
[admin] ns create & attach(NVMSET k)  → AEN
[scan]  nvme_alloc_ns → nvme_pci_ns_tagset(nvmset_id=k):
           g = find_group(k)
           if !g->tagset_ready: nvmset_build_tagset(g)  ★ sqids[]로 tagset 구성
           return &g->tagset → blk_mq_alloc_disk → /dev/nvme0nX (격리 완성)

[제출] write(/dev/nvme0nX) → bio → blk-mq → ns->queue(=group[k].tagset)
        → CPU→hctx(그룹 내)→ g->sqids[j] SQ → nvme_queue_rq (무수정)
[완료] CQE → nvme_queue_tagset(nvmeq)=nvmeq->tags → nvme_find_rq (O(1) 역매핑)
```

**2단계 격리**: ① `rq->q(ns)` → 묶인 tagset → 그룹 SQ 집합(어느 CPU 든 무조건 이 그룹). ② CPU → hctx → 그룹 내 SQ 하나(그룹 밖 못 나감).

---

## 4. 주요 구현 결정 (의사결정 기록)

### 4.1 완료 디먹스 — `nvmeq->tags` 캐시 (설계 §15, 1단계 핵심 난점)
mainline `nvme_queue_tagset()` 은 `dev->tagset.tags[qid-1]` 로 **단일 tagset 가정**. 다중 per-set tagset 에선 전역 qid 로 로컬 `tags[]` 인덱싱 → 엉뚱한 풀 조회로 완료가 조용히 깨진다.
→ `struct nvme_queue` 에 `struct blk_mq_tags *tags;` 추가. `init_hctx` 가 `nvmeq->tags = hctx->tags`(그 hctx 의 올바른 풀)를 캐시하고, `nvme_queue_tagset()` 은 `return nvmeq->tags;`(qid 산술 제거). 제출에서 tag 꺼낸 풀 == 완료 조회 풀 → 항상 일치.

### 4.2 ops 재사용 — 별도 `nvmset_mq_ops` 만들지 않음
초기엔 per-set 전용 `nvmset_mq_ops` 를 뒀으나, **"path 다양화로 혼란"** 회피를 위해 mainline `nvme_mq_ops` 를 그대로 재사용하도록 변경. per-set 에서 본질적으로 달라야 하는 2곳을 **공유 콜백 내 분기**로 흡수:
- `nvme_init_hctx`: `set==&dev->tagset` 이면 mainline(`hctx_idx+1==전역 qid`), 아니면 per-set(`g->sqids[hctx->queue_num]`).
- `nvme_pci_map_queues`: `set!=&dev->tagset` 이면 `g->nr_sqs` 만큼 라운드로빈.

> `.map_queues` 를 NULL 로 둬 blk-mq 기본(`blk_mq_map_queues`)에 맡기는 것도 가능하지만, ops 재사용 시 `nvme_mq_ops.map_queues=nvme_pci_map_queues` 가 호출되므로 거기에 per-set 분기를 둠. `const struct blk_mq_ops` 는 런타임에 한 필드만 못 바꾸므로, mainline 자신도 `nvme_mq_ops`/`nvme_mq_admin_ops` 를 따로 두는 것과 같은 관용.

### 4.3 컴파일 옵션 제거 — 항상 컴파일
`#ifdef CONFIG_NVME_NVMSET_ISOLATION` 가드와 Kconfig 항목을 **전부 제거**. 기능 사용 여부는 컴파일이 아니라 **런타임(NVMSETID 유무)**으로만 결정. (비-PCIe 트랜스포트는 콜백 미등록 → mainline.)

### 4.4 qid = 유저 입력 + 연속 생성 정책
`CSQ_CREATE` 가 생성할 IO 큐의 `qid` 를 유저가 지정(`c.qid`). 검증: `1 <= qid < nr_allocated_queues`, `qid < num_vecs`, 미사용(`!NVMEQ_ENABLED`).
- **연속(gap 없음) 정책**: `qid > dev->ctrl.queue_count` 면 거부. mainline 큐 회계(`dev->queues[0..queue_count-1]` 연속 유효 가정)와 reset 경로(`nvme_reap_pending_cqes` 가 NULL CQ 접근)가 깨지는 것을 방지. 새 큐는 다음 슬롯(`qid==queue_count`)이나 해제된 슬롯 재사용(`qid<queue_count`)만.
- 비연속/gap 시나리오는 추후(placeholder 채우기 또는 reset 경로 gap-safe 가드).

---

## 5. 7.0.0 vs 6.1.4 포팅 차이

`master`(7.0.0)와 `nvme-nvmset-6.1.4` 브랜치의 동일 로직을 각 커널 API 에 맞춘 차이:

| 영역 | 7.0.0 | 6.1.4 |
|------|-------|-------|
| IO tagset `driver_data` | `&dev->ctrl` (ctrl) | **`dev`** (nvme_dev) — `init_hctx`/`init_request` 가 dev 직접 캐스팅 |
| `nvme_suspend_queue` | `(dev, qid)` | **`(nvmeq)`** |
| descriptor pool | per-hctx `nvme_setup_descriptor_pools()` | per-dev `prp_page_pool` → `init_hctx` 더 단순(풀 셋업 없음) |
| `blk_mq_alloc_disk` | `(tagset, &lim, ns)` 3-인자 | **`(tagset, ns)`** 2-인자 |
| `nvme_ctrl_ops` 끝 | `.get_virt_boundary` | `.supports_pci_p2pdma` 뒤에 콜백 추가 |
| init_hctx 헬퍼 | `nvme_init_hctx_common`(admin+io 공유) | `nvme_init_hctx`/`nvme_admin_init_hctx` 분리 |
| tagset flags | `0` (SHOULD_MERGE 제거됨) | **`BLK_MQ_F_SHOULD_MERGE`** |

> 6.1.4 의 `driver_data=dev` 덕분에 per-set `build_tagset` 도 `set->driver_data=dev` 로 두면 `nvme_init_hctx`/`nvme_pci_init_request` 가 그대로 동작. 7.0 은 `&dev->ctrl` 이라 `to_nvme_dev()` 경유.

핵심 로직(per-set tagset, NVMSETID Create SQ, selector, 완료 디먹스, CSQ ioctl)은 양 버전 동일.

---

## 6. 자료구조 (구현 확정형)

```c
/* pci.c */
enum nvmset_state { NVMSET_STATE_D, NVMSET_STATE_P };   /* 판정 캐시 */

struct nvmset_group {                  /* per-set 격리 단위 (k>=1) */
    u16                   nvmset_id;    /* 그룹 NVM Set ID. selector 비교 키 */
    u16                  *sqids;        /* 이 set 의 SQ qid 목록. sqids[로컬hctx]=전역qid */
    unsigned int          nr_sqs;       /* = tagset.nr_hw_queues */
    struct blk_mq_tag_set tagset;       /* 이 set 전용 IO tagset (일시) */
    bool                  tagset_ready; /* lazy 빌드 가드 */
    struct nvme_dev      *dev;          /* init_hctx 에서 sqid→dev->queues[] 해석 */
    struct list_head      entry;        /* dev->group_list 연결 */
};

struct nvme_dev { /* +group_list, nr_groups, nvmset_state, nvmset_lock */ };
struct nvme_queue { /* +tags(완료 디먹스 캐시), group(소속 그룹) */ };
```

---

## 7. UAPI (제어 평면)

대상: 컨트롤러 char dev `/dev/nvmeX`. 권한 `CAP_SYS_ADMIN`.

```c
struct nvme_csq_create {
    __u16 nvmset_id;   /* 0=거부(격리 미사용은 ioctl 불필요), >=1=per-set */
    __u16 qsize;       /* SQ 깊이(0→기본) */
    __u8  qprio;       /* WRR 0=Urgent..3=Low (CAP.AMS 미지원 시 무시) */
    __u8  flags;
    __u16 qid;         /* [입력] 생성할 IO 큐 qid (연속만 허용) */
    __u16 rsvd;
};
struct nvme_csq_delete { __u16 nvmset_id; __u16 qid; __u32 rsvd; };

#define NVME_IOCTL_CSQ_CREATE  _IOWR('N', 0x50, struct nvme_csq_create)
#define NVME_IOCTL_CSQ_DELETE  _IOW ('N', 0x51, struct nvme_csq_delete)
```

사용 예:
```c
int fd = open("/dev/nvme0", O_RDWR);
struct nvme_csq_create c = { .nvmset_id = 1, .qid = 1, .qsize = 1024 };
ioctl(fd, NVME_IOCTL_CSQ_CREATE, &c);     /* NVM Set 1 격리 SQ (필요 수만큼 qid 연속 반복) */
/* → 이후 ns 를 NVM Set 1 로 create&attach → /dev/nvme0n1 이 그 set SQ 에 격리 */
```

전제(capability): 컨트롤러가 NVM Set 지원(QEMU nvme `nvm-set` 등으로 에뮬레이션 가능), MSI-X 벡터 여유(`qid < num_vecs`). 미지원 시 CDW12 NVMSETID 무시 → SW 라우팅만.

---

## 8. github 브랜치 / 커밋

| 브랜치 | base | 내용 |
|--------|------|------|
| `master` | 7.0.0-rc6 fork | 7.0.0 기반 구현 (커밋 `6d82316`) |
| `nvme-nvmset-6.1.4` | v6.1.4 전체 트리 | 6.1.4 포팅 (root 커밋 `1d51944`) |

> 6.1.4 브랜치가 root(orphan) 커밋인 이유: 로컬 클론이 shallow 라 v6.1.4 를 parent 로 둔 브랜치를 github 에 push 불가(v6.1.4 조상 객체가 shallow 경계 아래). 6.1.4 트리를 통째로 담은 단일 커밋으로 둠. 변경분 확인: `git diff v6.1.4 github/nvme-nvmset-6.1.4 -- drivers/nvme/host include/linux/nvme.h`.

---

## 9. 빌드/검증 상태

- 모든 함수 시그니처·구조체·헤더 필드를 각 커널(7.0.0 / v6.1.4) **실제 소스와 대조**해 작성.
- 정적 점검: `#ifdef` 균형(제거 후 0), 중괄호/괄호 균형, nvmset 심볼 전부 정의, 정의 순서(enum→dev→group→queue).
- **전체 커널 빌드(.config)는 이 환경에서 미수행** — 실제 빌드 시 `CONFIG_BLK_DEV_NVME` 로 `drivers/nvme/host` 컴파일 권장.

---

## 10. 추후 시나리오 (미구현)

1. **비연속 qid / gap 허용**: placeholder 큐로 빈칸 채우기 또는 reset 경로 gap-safe 가드.
2. **reset Option B**(설계 §25): reset 시 per-set 오버레이 해체 후 `ctrl->tagset` baseline 복귀.
3. **endurance_group 2계층**(설계 §29 정본): ENDGRP 컨테이너 → nvmset_entry.
4. **State D/P all-or-nothing 강제** + selector pending(rendezvous).
5. **thread 격리**: cpuset cgroup 으로 thread→set SQ CPU 핀(설계 §11).
6. WRR 큐 예산 정책, poll 큐(HIPRI) per-set provision.

---

## 부록 — 참조
- 설계 정본: [[nvme-namespace-sq-isolation-driver]] (§15 완료 디먹스, §19 레이어, §28 헬퍼, §29 정본)
- 관련: [[nvme-driver]], [[sq-mapping-comparison]], [[io_submit-to-nvme_queue_rq-flow]]
- 코드: `linux-study/drivers/nvme/host/{pci.c, core.c, ioctl.c, nvme.h, nvmset.h}`, `include/linux/nvme.h`
- 커널 대조 기준: mainline `v7.0` / stable `v6.1.4` (git.kernel.org)
