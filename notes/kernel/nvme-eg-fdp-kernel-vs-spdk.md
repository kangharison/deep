# NVMe Endurance Group / NVM Set / FDP — Linux 커널 vs SPDK 처리 비교

> 작성일: 2026-06-17
> 분석 대상 코드:
> - 커널: `linux-study/drivers/nvme/host/` (최신 트리, `kmalloc_obj`·`bi_write_stream` 사용 → ~6.17/6.18대)
> - SPDK: `spdk-study/` (v26.01.0)
> 관련: [[nvme-driver]], [[spdk]], `deep/analysis/lighter/lighter-design.md`
> 스펙 출처: NVMe Base Spec 2.x, **TP4146 Flexible Data Placement(FDP)**

---

## 0. 한 줄 결론

- **Endurance Group(EG)** 은 양쪽 모두 **거의 전적으로 FDP(Flexible Data Placement)를 위해서만** 다뤄진다. EG의 마모/수명 회계 자체는 디바이스 내부 + 유저스페이스(nvme-cli/identify) 몫이다.
- **NVM Set** 은 커널·SPDK 모두 **스펙 구조체/식별·NSID 유일성 판단 용도일 뿐**, I/O 라우팅·격리·QoS 스케줄링 로직은 **없다**.
- **커널**은 "블록 레이어 write stream → FDP placement id 자동 매핑"에 그치고 EG 로그는 유저스페이스에 위임한다. **SPDK**는 유저스페이스 라이브러리답게 FDP/EG 전체(로그 조회·RUH 설정·placement 지정)를 앱이 직접 제어하도록 **풀스택으로 노출**한다.

---

## 1. 개념 계층 (스펙)

```
 NVM Subsystem
 ├─ Controller(s)            ← host 접근 경로 (SQ/CQ, CNTLID)
 └─ Endurance Group(s)       ← ★ 매체/마모 관리 도메인 (ENDGID).  FDP의 스코프
       └─ NVM Set(s)         ← 격리(QoS) 도메인 (NVMSETID). Set→EG = N:1
             └─ Namespace(s) ← host 노출 LBA 공간 (NSID).   ns→Set = N:1
```

- 매체 트리: Namespace ⊂ NVM Set ⊂ Endurance Group ⊂ Subsystem.
- Controller↔Namespace 만 다대다(attach), 나머지는 부모 1개 트리.
- **FDP**: EG 안에 **Reclaim Unit Handle(RUH)** 들이 있고, write 명령이 **Placement Identifier(PID/dspec)** 로 특정 RUH(=Reclaim Unit)에 데이터를 배치 → 호스트가 GC/write-amplification을 제어. DTYPE=0x2(Data Placement Directive)로 활성화.

---

## 2. 커널 드라이버 처리

### 2.1. Endurance Group ID 추적 — FDP 조회 키로만 사용

- ns 정보에 `endgid` 저장: `struct nvme_ns_info { ... u16 endgid; ... }` — `linux-study/drivers/nvme/host/core.c:41`
- Identify Namespace의 ENDGID 읽기: `info->endgid = le16_to_cpu(id->endgid)` — `core.c:1637`, `core.c:1678`
- 용도는 **FDP feature/config 조회의 LSI(Log Specific Identifier)** 뿐:
  - `nvme_get_features(ctrl, NVME_FEAT_FDP, info->endgid, ...)` — `core.c:2313`
  - `nvme_get_log_lsi(ctrl, 0, NVME_LOG_FDP_CONFIGS, ..., info->endgid)` — `core.c:2231`

### 2.2. FDP 데이터 경로 — write stream → placement id 자동 매핑

EG의 FDP 구성에서 placement id 목록(plid)을 만들어 ns_head에 저장:
- `struct nvme_ns_head { ... u16 nr_plids; u16 *plids; ... }` — `nvme.h:555`
- `nvme_query_fdp_info()` / `nvme_query_fdp_granularity()` — `core.c:2295 / 2222` (RUH status를 I/O Mgmt Receive로 읽어 plid 채움)

write 명령에 placement directive 부착 (**커널에서 EG가 I/O 경로에 관여하는 유일한 지점**):
```c
// core.c:1005-1013  nvme_setup_rw()
if (op == nvme_cmd_write && ns->head->nr_plids) {
    u16 write_stream = req->bio->bi_write_stream;       // 블록 레이어 write stream
    if (WARN_ON_ONCE(write_stream > ns->head->nr_plids)) return BLK_STS_INVAL;
    if (write_stream) {
        dsmgmt |= ns->head->plids[write_stream - 1] << 16;  // DSPEC = placement id
        control |= NVME_RW_DTYPE_DPLCMT;                    // DTYPE = Directive: Placement
    }
}
```
→ 블록 레이어 `bi_write_stream` 을 FDP reclaim unit(plid)에 매핑. write hint/stream 인프라를 재활용.

### 2.3. EG endurance 로그 — 정의만, 커널 미사용

- `struct nvme_endurance_group_log` (512B) — `include/linux/nvme.h:670`
- 로그 ID `NVME_LOG_ENDURANCE_GROUP = 0x09` — `include/linux/nvme.h:1414`
- 호스트 드라이버에서의 등장은 **크기 검증뿐**: `BUILD_BUG_ON(sizeof(struct nvme_endurance_group_log) != 512)` — `core.c:5385`
- → EG 마모/수명 통계는 커널이 능동적으로 읽지 않음. `nvme-cli` 가 passthrough(get-log 0x09)로 가져감.

### 2.4. NVM Set — NSID 유일성 판단에만

```c
// nvme.h:944-947  nvme_is_unique_nsid()
return head->shared ||
       (ctrl->oacs & NVME_CTRL_OACS_NS_MNGT_SUPP) ||
       (ctrl->subsys->cmic & NVME_CTRL_CMIC_ANA) ||
       (ctrl->ctratt & NVME_CTRL_CTRATT_NVM_SETS);   // NVM Sets 지원 → NSID 전역 유일로 간주
```
- NSID naming / 멀티패스 head 매칭 결정용. **NVMSETID 자체는 드라이버가 읽지도, I/O 라우팅에 쓰지도 않음.**

---

## 3. SPDK 처리 (v26.01)

커널보다 **훨씬 완전한 TP4146 풀스택**. 유저스페이스 라이브러리라 앱이 직접 제어.

### 3.1. 스펙 정의 — `spdk-study/include/spdk/nvme_spec.h`

| 항목 | 위치 |
|------|------|
| 로그 ID: Endurance Group Information(0x09) | `nvme_spec.h:5269` |
| 로그 ID: Endurance Group Event(0x0F) | `nvme_spec.h:5293` |
| 로그 ID: FDP Configurations(0x20)/RUH Usage(0x21)/Statistics(0x22)/Events(0x23) | `nvme_spec.h:5328-5341` |
| `struct spdk_nvme_fdp_ruhs_desc` (PID=placement id, write dspec과 매칭) | `nvme_spec.h:3017` |
| `struct spdk_nvme_fdp_cfg_descriptor` / `..._ruh_descriptor` | `nvme_spec.h:5871 / 5857` |
| `enum ..._mgmt_recv_mo / send_mo / ruh_type` | `nvme_spec.h:3061 / 3071 / 5841` |
| 명령 CDW12 [23:20] **DTYPE** (0x3=Data Placement) | `nvme_spec.h:2106` |

### 3.2. 컨트롤러/명령 레이어 — `lib/nvme/nvme_ctrlr_cmd.c`

- `spdk_nvme_ctrlr_cmd_get_log_page_ext()` — `nvme_ctrlr_cmd.c:918`
  EG Info 로그(0x09)·FDP 로그를 가져오는 범용 경로. **LSI에 `endgid` 를 실어** EG 단위 조회.

### 3.3. Namespace I/O 명령 레이어 — `lib/nvme/nvme_ns_cmd.c`

FDP 핵심 공개 API 2개:

| API | opcode | 위치 |
|-----|--------|------|
| `spdk_nvme_ns_cmd_io_mgmt_recv` (RUHS: Reclaim Unit Handle Status 조회) | 0x12 | `nvme_ns_cmd.c:2661` |
| `spdk_nvme_ns_cmd_io_mgmt_send` (RUH 설정) | 0x1D | `nvme_ns_cmd.c:2717` |

- write 경로는 호출자 지정 **`cdw13`(=dspec, placement handle)** 를 그대로 명령에 실음 — `nvme_ns_cmd.c:439`.

### 3.4. bdev 추상화 — `include/spdk/bdev.h` / `module/bdev/nvme/bdev_nvme.c`

FDP가 bdev 레벨까지 노출:
- `ctratt.fdps`(지원 비트, `bdev.h:478`), `dtype`(`:508`), `dspec`(`:536`)
- 변환: `bio->ext_opts.io_flags = flags | SPDK_NVME_IO_FLAGS_DIRECTIVE(cdw12.write.dtype)` — `bdev_nvme.c:11579`

### 3.5. 앱/툴

- **identify** `app/spdk_nvme_identify/identify.c`
  - FDP 4종 로그 덤프 — `identify.c:520 get_fdp_cfg_log_page`, EG 키 `nsdata->endgid << 16` (`:536`)
  - FDP feature가 **Endurance Group scope** 임을 처리 — `identify.c:242`
- **fio 플러그인** `app/fio/nvme/fio_plugin.c` (FDP가 실제 I/O에 실리는 곳)
  - write placement 적용: `io_u->dtype`/`io_u->dspec` → `ext_opts` — `fio_plugin.c:1215-1219`
  - RUHS 페치: `spdk_fio_fdp_fetch_ruhs()` → `io_mgmt_recv` — `fio_plugin.c:1612, 1649`

---

## 4. 비교 표

| 항목 | 커널 (`linux-study/drivers/nvme/host`) | SPDK (`spdk-study`) |
|------|----------------------------------------|---------------------|
| FDP 스펙 구조체 | `include/linux/nvme.h`에 일부 | **TP4146 전부** (`nvme_spec.h`) |
| EG ID(`endgid`) 추적 | ns_info에 저장, FDP 조회 키로만 | identify가 EG 키로 로그 조회 |
| EG Info 로그(0x09) | **구조체만 정의, 커널 미사용**(nvme-cli 몫) | get_log + identify 덤프 |
| FDP placement write | `bi_write_stream`→plid **자동 매핑** (`core.c:1005`) | 앱이 **dtype·dspec 직접 지정** (fio_plugin) |
| I/O Mgmt Send/Recv | 내부용(`nvme_query_fdp_info`) | **공개 API 2개** (`io_mgmt_recv/send`) |
| NVM Set | NSID 유일성 비트만 (`nvme.h:947`) | 스펙 구조체/identify만 |
| NVM Set 기반 스케줄링 | 없음 | 없음 |

---

## 5. 프로파일링(lighter) 함의

- **커널 경로**: I/O 명령에 직접 실리는 식별자는 여전히 **NSID + qid** 뿐. per-EG/per-Set 집계는 **Identify(NSID→NVMSETID/ENDGID) 매핑 테이블을 1회 구축**해 역참조해야 함. 단 **FDP plid(write stream)/DTYPE 비트는 데이터 경로에 실리므로**(`core.c:1005`), `nvme_setup_cmd`/`nvme_sq` 단계에서 디코딩하면 EG/placement 관점 분해 가능.
- **SPDK 경로**: SPDK는 커널 블록/NVMe 드라이버를 우회(유저스페이스 폴링) → `block:*`/`nvme:*` tracepoint가 **안 잡힘**. SPDK 프로파일은 `spdk_nvme_ns_cmd_io_mgmt_*` / `bdev_nvme.c:11579` 등 **유저스페이스 함수 uprobe/USDT** 가 필요 → lighter의 커널 훅과 다른 별도 트랙. 자세한 설계는 `lighter-design.md` §3.5/§17 참조.
