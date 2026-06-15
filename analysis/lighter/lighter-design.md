# lighter 설계 문서 — eBPF 기반 NVMe I/O 경로 per-PID/TID 레이턴시 프로파일러 (TUI)

> 작성일: 2026-06-15
> 대상 프로젝트 트리(코드): `company/lighter/`
> 레퍼런스: [[air]] (TUI·데이터 모델), [[nvme-pcie-tracer]] (커널 훅·이벤트 모델)
> 관련 분석: `deep/analysis/kernel/` (blk-mq/NVMe 드라이버 경로)

---

## 1. 개요

### 1.1. 한 줄 정의

**lighter** 는 NVMe 디바이스에 I/O를 발생시키는 **프로세스(PID)/스레드(TID) 단위**로, **애플리케이션 syscall부터 NVMe 드라이버·디바이스까지의 I/O 경로 각 계층(stage)에서 발생하는 레이턴시를 eBPF로 측정**하고, 어느 계층이 **병목(bottleneck)** 인지를 **실시간 TUI**로 보여 주는 경량 프로파일러다.

### 1.2. 동기

기존 도구의 한계:

| 도구 | 측정 대상 | 한계 |
|------|----------|------|
| `fio` 자체 리포트 | 애플리케이션 관점 end-to-end latency | 어느 **계층**이 느린지 분해 불가 |
| `iostat`/`biolatency` | 블록 레이어 큐~완료 | syscall~블록 진입, per-PID/TID 분해 부족 |
| `blktrace`/`blkparse` | 블록 레이어 이벤트 타임라인 | 드라이버/디바이스 분해 약함, 실시간 집계 TUI 아님 |
| [[nvme-pcie-tracer]] | submit→complete (NVMe 계층) | per-PID/TID 귀속 없음, 커널 모듈 위주, end-to-end 분해 아님 |
| [[air]] | 애플리케이션 내부 계측점 | 소스 계측(`airlog()`) 필요 → **무수정 바이너리(fio/nvme-cli) 프로파일 불가** |

lighter 는 위 빈틈을 메운다:

- **무수정(no-instrumentation)**: 커널 tracepoint/kprobe만 사용 → fio·nvme-cli·임의의 custom app을 **재빌드 없이** 프로파일.
- **계층별 분해(stage breakdown)**: end-to-end 한 숫자가 아니라 syscall→blk-mq→driver→device→completion 각 구간 latency.
- **per-PID/TID 귀속**: 어느 프로세스/스레드의 I/O가 어디서 느린지.
- **실시간 TUI**: [[air]] 의 `air_tui` 같은 트리형 실시간 화면 + 병목 하이라이트.

### 1.3. 두 레퍼런스 프로젝트와의 관계

```
   air (TUI/데이터 모델)              nvme-pcie-tracer (커널 훅/이벤트)
   ─────────────────────              ────────────────────────────────
   · Node 타입(Latency/Hist/...)      · nvme:nvme_sq / nvme_complete_rq
   · Period vs Cumulation 통계        · request* / (qid,cid) 상관키
   · JSON 스트리밍 → ncurses TUI      · opcode/slba/nlb 디코딩
   · play/pause/interval/fold 제어    · 레이턴시 히스토그램(bpftrace)
            │                                      │
            └──────────────┬───────────────────────┘
                           ▼
                       l i g h t e r
        (eBPF 무수정 계측 + 계층 분해 + per-PID/TID + 실시간 TUI)
```

- **air 에서 가져오는 것**: 데이터 모델(노드/필터/Period·Cumulation 통계), JSON 스트리밍 → TUI 트리 렌더링 구조, 실시간 제어 UX.
- **air 와 다른 점**: air 는 애플리케이션 소스에 `airlog()` 를 삽입해야 한다. lighter 는 **소스 계측이 없다** — 모든 계측점을 커널 eBPF 훅으로 대체한다.
- **nvme-pcie-tracer 에서 가져오는 것**: NVMe tracepoint 훅 포인트, `request` 포인터·`(qid,cid)` 상관키, opcode/SLBA/NLB 디코딩 로직, 레이턴시 측정 패턴.
- **nvme-pcie-tracer 와 다른 점**: 커널 모듈/IOMMU fault 같은 파괴적·저수준 기법 대신 **순수 eBPF(CO-RE)** 로 비파괴 계측하고, **계층 분해 + per-PID/TID 집계 + TUI** 를 추가한다.

---

## 2. 요구사항

### 2.1. 기능 요구사항 (Functional)

- **FR-1 (대상 애플리케이션)**: 아래 I/O 경로를 모두 프로파일할 수 있어야 한다.
  - Custom app — `libaio`(`io_submit`/`io_getevents`), 동기 `read`/`write`/`pread`/`pwrite`, `ioctl` passthrough.
  - `nvme-cli` — `ioctl(NVME_IOCTL_IO_CMD/ADMIN_CMD)` 패스스루.
  - `fio` — libaio/io_uring/psync/sync ioengine.
  - (확장) `io_uring` 경로.
- **FR-2 (계층 분해)**: 한 I/O에 대해 §4의 stage들의 구간 latency를 측정.
- **FR-3 (per-PID/TID 귀속)**: 모든 통계를 (PID, TID) 단위로 분리. I/O를 **발행한** 스레드 기준으로 귀속(완료는 IRQ 컨텍스트에서 일어나므로 발행 시점 정보를 저장해 둔다 — §6.3).
- **FR-4 (필터)**: target PID/TID, target 디바이스(`/dev/nvmeXnY` 또는 BDF/devt), opcode(Read/Write/Flush/...) 로 필터.
- **FR-5 (통계)**: 각 (PID,TID,stage,opcode) 별 avg/median/min/max/p99 + 히스토그램 + sample count. air 와 동일하게 **Period**(최근 interval)와 **Cumulation**(누적) 두 축.
- **FR-6 (병목 식별)**: 각 PID/TID의 I/O에서 가장 큰 비중을 차지하는 stage를 자동 산출·하이라이트(§9).
- **FR-7 (TUI)**: 터미널 실시간 트리 뷰 + 제어(play/pause, streaming interval 변경, fold/unfold, PID 필터, quit). air `air_tui` UX 차용.
- **FR-8 (스냅샷/내보내기)**: 현재 화면을 JSON/CSV로 덤프(사후 분석용).

### 2.2. 비기능 요구사항 (Non-functional)

- **NFR-1 (저오버헤드)**: 고 IOPS(수백만 IOPS)에서도 영향 최소화. **in-kernel 집계 + 샘플링**으로 per-event 유저스페이스 전송을 피한다(§6.5). 목표: kprobe 대비 tracepoint 우선, per-event 오버헤드 < ~수백 ns, IOPS 영향 한 자릿수 %.
- **NFR-2 (무수정)**: 대상 바이너리 재빌드 불필요.
- **NFR-3 (이식성)**: libbpf **CO-RE(Compile Once – Run Everywhere)** + BTF로 커널 버전 간 이식. 타깃: Linux 5.8+ (ring buffer, BTF). 레퍼런스 환경은 nvme-pcie-tracer 와 동일하게 Rocky/CentOS 9 (5.14+).
- **NFR-4 (권한)**: `CAP_BPF`+`CAP_PERFMON`(또는 root). 커널 모듈 불필요.
- **NFR-5 (안정성)**: 대상/커널을 손상시키지 않음(IOMMU fault 같은 파괴적 기법 배제).

### 2.3. 비목표 (Non-goals)

- 디바이스 내부(컨트롤러 펌웨어) 분해 — device latency는 "doorbell→completion" 한 덩어리로만 측정(블랙박스).
- PCIe TLP 레벨 직접 가로채기(그건 [[nvme-pcie-tracer]] 의 IOMMU fault 영역).
- GUI / 원격 대시보드(초기 범위 밖, JSON 스트림으로 확장 가능).

---

## 3. 배경: NVMe I/O 경로 전 계층

lighter 가 계측하는 "애플리케이션 → 드라이버" 경로를 경로별로 정리한다. 각 ▶ 는 **eBPF 훅 후보 지점**.

### 3.1. libaio (비동기) 경로

```
[유저스페이스]
  app: io_submit(2) ──────────────▶ ① sys_enter_io_submit            (tid 캡처: T0)
        │
[커널]  ▼
  io_submit_one() → aio_read/aio_write → call_read/write_iter
        │
        ▼  blkdev_direct_IO / iomap dio → submit_bio
  block layer:
    ▶ ② block:block_bio_queue        (bio가 큐에 들어옴: T1)
    blk_mq_get_request (tag 할당 — 여기서 tag 고갈 시 대기!)
    ▶ ③ block:block_rq_insert        (request가 SW큐/스케줄러에 삽입: T2)
    blk_mq_run_hw_queue → dispatch
    ▶ ④ block:block_rq_issue         (request가 드라이버로 dispatch: T3)
        │
        ▼
  nvme driver:
    nvme_queue_rq() — NVMe command 빌드(SQE)
    ▶ ⑤ nvme:nvme_setup_cmd          (SQE 작성: T4)
    nvme_submit_cmd → SQ에 기록 → doorbell write(MMIO)
    ▶ ⑥ nvme:nvme_sq                 (SQ 제출+doorbell: T5)   ← submit 시각
        │
========│=== PCIe ===  doorbell ──▶ NVMe SSD ──DMA──▶ CQ write + MSI-X
        │
  device 처리 (블랙박스)
        ▼
    ▶ ⑦ nvme_irq() (kprobe)          (인터럽트 진입: T6, optional)
    blk_mq_complete_request → softirq
    ▶ ⑧ nvme:nvme_complete_rq        (request 완료: T7)   ← complete 시각
    ▶ ⑨ block:block_rq_complete      (블록 완료: T7')
    bio_endio → dio 완료 → aio 완료 큐에 적재
        │
[유저스페이스] ▼
  app: io_getevents(2) ────────────▶ ⑩ sys_exit_io_getevents          (T8)
```

### 3.2. ioctl passthrough 경로 (nvme-cli, custom ioctl)

```
  app: ioctl(fd, NVME_IOCTL_IO_CMD, &cmd) ─▶ ① sys_enter_ioctl (tid: T0)
        │
  nvme_ioctl → nvme_user_cmd → nvme_submit_user_cmd
        │  blk_mq_alloc_request → (동기) blk_execute_rq
        ▼
  ▶ ③ block:block_rq_insert / ④ block:block_rq_issue
  nvme_queue_rq → ⑤ nvme_setup_cmd → ⑥ nvme:nvme_sq (T5)
        │  device …  ⑧ nvme:nvme_complete_rq (T7)
        ▼  blk_execute_rq 깨어남 → 결과 복사
  ioctl 반환 ─────────────────────────▶ ⑩ sys_exit_ioctl (T8)
```

> 주: passthrough는 보통 **동기** — 제출 스레드가 완료까지 블록되므로 (PID,TID) 귀속이 자연스럽다. libaio/io_uring은 제출/완료 스레드가 같을 수도 다를 수도 있다 → request* 상관키로 발행 스레드 정보를 운반(§6.3).

### 3.3. io_uring 경로 (확장)

```
  app: io_uring_enter(2) ─▶ ① sys_enter_io_uring_enter
  ▶ io_uring:io_uring_submit_sqe (T0')
  io_read/io_write → submit_bio → (이하 §3.1 블록/드라이버 동일)
  ▶ io_uring:io_uring_complete (T8')  ← CQE 완성
```

### 3.4. 동기 read/write/pread/pwrite 경로

```
  app: pwrite(2) ─▶ ① sys_enter_pwrite64 (T0) → vfs_write → ...submit_bio
       (이하 블록/드라이버 동일) ... → 완료 시 제출 스레드 깨어남
  pwrite 반환 ─────▶ ⑩ sys_exit_pwrite64 (T8)
```

---

## 4. 레이턴시 스테이지 모델 (핵심)

lighter 는 위 타임스탬프 T0…T8 사이의 구간을 **stage** 로 정의한다. 이는 [[air]] 의 `LAT_IO_PATH` 노드가 `AIR_0~AIR_1`, `AIR_1~AIR_2` 식으로 **구간(range) latency** 를 표현하는 방식과 동일한 모델이다.

| Stage | 구간 | 시각 | 의미 / 무엇을 진단하나 |
|-------|------|------|----------------------|
| **S1 USER_SUBMIT** | T0→T2 | sys_enter → block_rq_insert | syscall 진입~블록 진입. VFS·DIO 셋업, **tag 할당 대기(큐 깊이 포화)** 가 여기서 드러남 |
| **S2 BLK_QUEUE** | T2→T3 | block_rq_insert → block_rq_issue | 블록 레이어 큐잉·스케줄러(mq-deadline/none) 대기·plug merge 시간 |
| **S3 DRV_SUBMIT** | T3→T5 | block_rq_issue → nvme:nvme_sq | 드라이버에서 SQE 빌드 + SQ 기록 + **doorbell(MMIO write)** |
| **S4 DEVICE** | T5→T7 | nvme_sq → nvme_complete_rq | **디바이스 처리 시간**(doorbell→completion). 순수 SSD/컨트롤러 latency (블랙박스) |
| **S5 CPL_PROC** | T7→T7' | complete_rq → block_rq_complete | 완료 인터럽트 처리·softirq·bio_endio |
| **S6 USER_RETURN** | T7'→T8 | block_rq_complete → sys_exit | 완료~유저 반환. **aio 완료 큐 적체, io_getevents 폴링 지연** 이 드러남 |
| **E2E END_TO_END** | T0→T8 | sys_enter → sys_exit | 애플리케이션 관점 총 latency (= ΣS1..S6 + 비계측 잔차) |

추가 보조 노드(air 노드 타입 차용):

| Node | air 타입 | 측정 | 용도 |
|------|---------|------|------|
| `Q_NVME_SQ` | Queue | SQ depth (제출−완료 in-flight 카운트) | 큐 깊이 포화 진단 |
| `PERF_DEV` | Performance | opcode별 IOPS/BW | 워크로드 특성 |
| `HIST_S4_DEVICE` | Histogram | device latency 분포 | tail latency 진단 |
| `CNT_ERROR` | Count | NVMe status != 0 | 에러율 |

> **병목 정의**: 한 I/O에서 `max(S1..S6)` 가 그 I/O의 병목 stage. PID/TID 집계 수준에서는 §9의 가중 방식으로 "지배 stage" 를 산출한다.

---

## 5. 전체 아키텍처

[[air]] 와 동일한 3계층(수집 → 집계/스트리밍 → TUI) 구조를 따른다. 차이는 수집 계층이 `airlog()` 소스 계측이 아니라 **eBPF 커널 프로그램** 이라는 점이다.

```
+=========================================================================+
|                         lighter 전체 구조                                |
+=========================================================================+

 [유저스페이스]
 ┌──────────────────────────┐        ┌──────────────────────────────┐
 │  lighter_tui (ncurses)   │        │  lighter snapshot (JSON/CSV)  │
 │  · 트리: PID→TID→stage    │◀──┐    │  사후 분석용 덤프              │
 │  · 병목 하이라이트         │   │    └──────────────────────────────┘
 │  · play/pause/interval    │   │ /tmp/lighter_<ts>_<pid>.json
 └───────────┬──────────────┘   │ (또는 unix socket / shm)
             │ 키 이벤트          │ 스트리밍(N초 주기)
             ▼                   │
 ┌─────────────────────────────────────────────────────────────────┐
 │  lighter_collector  (libbpf 데몬)                                  │
 │  · BPF 로드/attach (CO-RE)     · ring buffer 폴링                  │
 │  · per-(pid,tid,opcode) 집계   · Period/Cumulation 통계            │
 │  · 히스토그램·percentile        · /proc로 comm(프로세스명) 보강      │
 │  · JSON 스트리밍 출력           · CLI 명령 수신(필터/interval)       │
 └───────────┬──────────────────────────────────▲────────────────────┘
   bpf_map   │ ring buffer / map read            │ map update(필터·제어)
 ════════════╪═══════════════════════════════════╪══════════════════════
 [커널]      ▼                                   │
 ┌─────────────────────────────────────────────────────────────────┐
 │  eBPF 프로그램들 (CO-RE, BTF)                                      │
 │                                                                   │
 │  tp/syscalls:sys_enter_io_submit,ioctl,pwrite64,read,write  → S1  │
 │  tp/block:block_bio_queue / block_rq_insert / block_rq_issue → S1,S2│
 │  tp/nvme:nvme_setup_cmd / nvme_sq        → S3, submit 시각         │
 │  tp/nvme:nvme_complete_rq                → S4 종료, complete 시각   │
 │  tp/block:block_rq_complete             → S5                       │
 │  tp/syscalls:sys_exit_*                  → S6                       │
 │  (옵션) kprobe:nvme_irq, tp/io_uring:*                            │
 │                                                                   │
 │  BPF maps:                                                         │
 │   · inflight  HASH<request*, io_ctx>     상관키 기반 stage 타임스탬프│
 │   · tid_stack HASH<tid, ts>              syscall 진입 시각          │
 │   · stats     HASH<agg_key, lat_stats>   in-kernel 집계(히스토그램) │
 │   · events    RINGBUF                    (옵션) per-event 상세      │
 │   · filters   ARRAY/HASH                 target pid/dev/opcode      │
 └───────────────────────────────────────────────────────────────────┘
             │ tracepoint/kprobe attach
 ════════════▼═══════════════════════════════════════════════════════
 [I/O 경로]  syscall → blk-mq → nvme driver → PCIe → NVMe SSD
             (fio / nvme-cli / custom app — 무수정)
```

기술 스택:
- **eBPF**: libbpf + CO-RE, BTF(`/sys/kernel/btf/vmlinux`). 작성은 C(`*.bpf.c`) + `bpftool gen skeleton`.
- **Collector**: C 또는 Rust(libbpf-rs). 초기엔 C(레퍼런스 nvme-pcie-tracer 와 일관).
- **TUI**: ncurses(C) — air `air_tui` 와 동일 계열. (Rust 선택 시 `ratatui`.)

---

## 6. eBPF 프로브 설계 (핵심)

### 6.1. 훅 포인트 표

가능하면 **tracepoint**(안정 ABI, 저오버헤드)를 쓰고, 없는 지점만 **kprobe/fentry**.

| # | 훅 | 종류 | 캡처 | stage 기여 |
|---|----|------|------|-----------|
| ① | `syscalls:sys_enter_io_submit` / `sys_enter_io_uring_enter` / `sys_enter_ioctl` / `sys_enter_{read,write,pread64,pwrite64}` | tracepoint | tid, ts=T0, args(fd) | S1 시작 |
| ② | `block:block_bio_queue` | tracepoint | bio, dev, ts=T1 | (보조) |
| ③ | `block:block_rq_insert` | tracepoint | request*, dev, sector, ts=T2 | S1 끝/S2 시작 |
| ④ | `block:block_rq_issue` | tracepoint | request*, ts=T3 | S2 끝/S3 시작 |
| ⑤ | `nvme:nvme_setup_cmd` | tracepoint | request*, qid, opcode, nsid, ts=T4 | S3 중간 |
| ⑥ | `nvme:nvme_sq` | tracepoint | qid, cid, opcode, nsid, cdw10.., ts=T5 | S3 끝/S4 시작, **submit** |
| ⑦ | `nvme_irq` | kprobe(옵션) | ts=T6 | S4/S5 경계 보조 |
| ⑧ | `nvme:nvme_complete_rq` | tracepoint | qid, cid, status, result, ts=T7 | S4 끝, **complete** |
| ⑨ | `block:block_rq_complete` | tracepoint | request*, ts=T7' | S5 끝 |
| ⑩ | `syscalls:sys_exit_io_getevents` / `sys_exit_*` | tracepoint | tid, ts=T8 | S6 끝 |

> **검증 필요**: `nvme:nvme_sq`/`nvme_complete_rq` tracepoint 인자에는 `request*` 가 직접 노출되지 않고 `(qid, cid)` 만 노출될 수 있다. 반면 block tracepoint는 `request*` 를 준다. 따라서 **두 상관키 영역**(블록=`request*`, NVMe=`(qid,cid)`)을 잇는 다리가 필요하다(§6.2). `nvme:nvme_setup_cmd` 가 `request*` 와 곧 발급될 `cid` 를 동시에 볼 수 있는지 BTF로 확인하여 다리로 사용. 불가 시 `kprobe:nvme_queue_rq`(인자 `request*`)에서 `nvme_request`의 cid를 읽어 매핑.

### 6.2. 상관관계 키 (correlation) 전략

하나의 I/O를 전 계층에서 추적하려면 단계마다 키가 바뀌는 문제를 풀어야 한다.

```
 syscall 영역        블록/드라이버 영역        NVMe 펌웨어 영역
 ───────────         ──────────────────        ─────────────────
   tid           →      request*           →      (qid, cid)
 (T0 저장)          (T2..T3..T7' 저장)         (T5 submit, T7 complete)

 다리1: tid → request*
   block_rq_insert 시 bpf_get_current_pid_tgid()==원 제출 tid (동기/DIO 제출 경로에서 유효).
   → tid_stack[tid]=T0 를 inflight[request*] 로 이관.
 다리2: request* → (qid,cid)
   nvme_setup_cmd/nvme_queue_rq 에서 request* 와 cid 동시 관측 → cid_of[request*]=cid,
   rq_of[(qid,cid)]=request* 양방향 매핑.
```

핵심 자료구조:

```c
// io_ctx: 한 I/O의 stage별 타임스탬프 + 귀속 정보
struct io_ctx {
    u64 ts[8];        // ts[0]=T0 ... ts[7]=T8 (0=미관측)
    u32 pid;          // 발행 프로세스 (tgid)
    u32 tid;          // 발행 스레드
    u32 dev;          // devt (major:minor) — 디바이스 필터/귀속용
    u16 qid;          // NVMe submission queue id
    u16 cid;          // NVMe command id
    u8  opcode;       // nvme opcode (0x01=Write,0x02=Read,0x00=Flush,..)
    u8  nsid;         // namespace
    u32 nlb;          // 전송 블록 수 (BW 계산)
    char comm[16];    // 발행 스레드 comm (TASK_COMM_LEN)
};
```

> **귀속 원칙(FR-3)**: `pid/tid/comm` 은 **반드시 제출 시점**(① 또는 ③, `bpf_get_current_pid_tgid`/`bpf_get_current_comm`)에 채운다. 완료(⑧⑨)는 IRQ/softirq 컨텍스트라 current task가 무의미하므로, `inflight[request*]` 에 저장해 둔 발행 정보를 그대로 사용한다. 이것이 air 에는 없는 lighter 의 핵심 — air 는 발행 스레드에서 `airlog()` 를 부르므로 귀속이 자동이지만, lighter 는 커널에서 명시적으로 운반해야 한다.

### 6.3. 흐름 (의사코드)

```c
// ① syscall 진입: 발행 스레드 시각·정보 임시 저장
tp/sys_enter_io_submit (또는 pwrite64/ioctl/...) {
    if (!pid_allowed(tgid)) return;          // filters 맵으로 조기 컷
    u64 id = bpf_get_current_pid_tgid();
    struct tinfo *t = tid_stack.reserve(id);
    t->t0 = bpf_ktime_get_ns();
    bpf_get_current_comm(t->comm, ...);
}

// ③ block_rq_insert: tid→request* 다리, io_ctx 생성
tp/block_rq_insert {
    if (!dev_allowed(args->dev)) return;     // 디바이스 필터
    u64 id = bpf_get_current_pid_tgid();
    struct tinfo *t = tid_stack.lookup(id);  // 같은 스레드가 제출 중
    struct io_ctx c = {};
    c.pid = id>>32; c.tid = (u32)id; c.dev = args->dev;
    c.ts[0] = t ? t->t0 : 0;                 // T0 이관(없으면 미관측)
    c.ts[2] = bpf_ktime_get_ns();            // T2
    __builtin_memcpy(c.comm, t->comm, 16);
    inflight.update(args->rq, &c);           // 상관키 = request*
}

// ④ block_rq_issue → c.ts[3]=T3
// ⑤ nvme_setup_cmd: request*↔(qid,cid) 다리 + opcode/nlb
// ⑥ nvme_sq:        c.ts[5]=T5 (submit), inflight_count[qid]++ (SQ depth)
// ⑧ nvme_complete_rq (qid,cid):
tp/nvme_complete_rq {
    struct io_ctx *c = lookup_by_qid_cid(qid, cid);  // 다리2 역방향
    if (!c) return;
    c->ts[7] = bpf_ktime_get_ns();           // T7
    if (status) errors.increment(c->pid,c->tid,status);
    // 여기서 S1..S4 구간이 모두 확정되면 부분 집계 가능
}

// ⑨ block_rq_complete → c->ts[7']  ;  ⑩ sys_exit → c->ts[8]=T8
// 마지막 관측 지점에서 stage delta 계산 후 stats 집계 + (옵션) ringbuf 방출
finalize(io_ctx *c) {
    for (s in S1..S6) {
        if (both endpoints observed)
            stats_update(agg_key(c->pid,c->tid,c->opcode,s), delta(c,s));
    }
    inflight.delete(rq); rq_of.delete(qid,cid);
}
```

### 6.4. 집계 키 & 통계 맵

```c
struct agg_key {       // 집계 차원 (air 의 Node×Filter×Index 에 대응)
    u32 pid;
    u32 tid;
    u8  opcode;        // air Filter: AIR_IOtype 대응
    u8  stage;         // S1..S6,E2E — air Filter: AIR_Range 대응
};
struct lat_stats {     // air 의 Latency 노드 통계에 대응
    u64 count, sum, min, max;
    u64 slots[64];     // log2 히스토그램 버킷 (bpftrace hist() 방식)
    // median/p99 는 slots 로부터 유저스페이스에서 근사
};
```

- **median/p99**: in-kernel 에서는 log2 히스토그램 슬롯만 누적(저비용). percentile 은 collector 가 슬롯에서 보간으로 근사 — air TUI가 median 을 표시하는 것과 동일 목적.
- **Period vs Cumulation**: collector 가 매 interval마다 stats 맵을 읽고 **델타(Period)** 와 **누적(Cumulation)** 을 따로 유지. (in-kernel은 단조 누적만, 차분은 유저스페이스에서 — air 의 streaming interval 모델과 동일.)

### 6.5. per-event vs in-kernel 집계 (오버헤드 결정)

| 모드 | 메커니즘 | 오버헤드 | 용도 |
|------|---------|---------|------|
| **집계(기본)** | in-kernel HASH 히스토그램, 유저스페이스는 interval마다 map dump | 매우 낮음(전송 없음) | 실시간 TUI 통계 (NFR-1) |
| **이벤트(상세)** | RINGBUF 로 io_ctx 방출 | 높음(고 IOPS시) | 디버깅·타임라인·스냅샷, **샘플링**(1/N, air NodeSamplingRatio 차용) 적용 |

> 기본은 집계 모드. nvme-pcie-tracer 의 per-event relay 방식은 고 IOPS에서 비싸므로(README의 성능 영향 표 참조), lighter 는 **biolatency류 in-kernel 히스토그램**을 1순위로 한다. 상세 이벤트는 옵션 + 샘플링.

---

## 7. Collector(데몬) 설계

### 7.1. 책임

1. BPF skeleton 로드·attach, BTF로 CO-RE 재배치.
2. `filters` 맵에 target PID/dev/opcode 기록(TUI/CLI에서 변경 가능).
3. 매 `streaming_interval` 초마다 `stats` 맵 스냅샷 → Period/Cumulation 계산.
4. PID→comm, 디바이스 devt→`/dev/nvmeXnY` 이름 보강(`/proc`, `/sys`).
5. JSON 스트림 출력(파일 `/tmp/lighter_<ts>_<pid>.json` 또는 unix socket/shm) — TUI가 소비.
6. TUI/CLI 제어 명령 수신(interval 변경, pid 필터, pause).

### 7.2. 데이터 모델 (air 노드 모델 매핑)

| air 개념 | lighter 대응 |
|----------|-------------|
| Node (LAT_IO_PATH) | stage 집합 S1..S6, E2E |
| Filter (AIR_IOtype) | opcode (Read/Write/Flush/...) |
| Filter (AIR_Range) | stage 인덱스 |
| NodeIndex | (pid,tid) 조합 |
| airlog() 계측점 | eBPF 훅(자동) |
| Period / Cumulation | interval 델타 / 누적 |
| StreamingInterval | collector interval |
| NodeSamplingRatio | 이벤트 모드 샘플링 비율 |

### 7.3. JSON 스트리밍 포맷(예)

air 의 `/tmp/air_*.json` 을 본떠, 한 interval = 한 JSON object.

```json
{
  "schema": "lighter/v1",
  "timestamp": "2026-06-15T11:12:02",
  "interval_s": 3,
  "device": "/dev/nvme0n1",
  "pids": [{
    "pid": 27019, "comm": "fio",
    "tids": [{
      "tid": 27025, "comm": "fio",
      "opcodes": [{
        "opcode": "Read",
        "e2e":    {"period": {"avg_ns":118000,"median_ns":95000,"max_ns":2100000,"p99_ns":410000,"samples":12044},
                   "cumulation": {"avg_ns":121000,"...":"..."}},
        "stages": {
          "S1_USER_SUBMIT": {"period":{"avg_ns":2400,"median_ns":1900,"max_ns":18000,"samples":12044}},
          "S2_BLK_QUEUE":   {"period":{"avg_ns":1200,"...":"..."}},
          "S3_DRV_SUBMIT":  {"period":{"avg_ns":900,"...":"..."}},
          "S4_DEVICE":      {"period":{"avg_ns":98000,"median_ns":82000,"max_ns":2000000,"samples":12044}},
          "S5_CPL_PROC":    {"period":{"avg_ns":1500,"...":"..."}},
          "S6_USER_RETURN": {"period":{"avg_ns":13000,"median_ns":4000,"max_ns":900000,"samples":12044}}
        },
        "bottleneck": {"stage":"S4_DEVICE","share_pct":83.1},
        "sq_depth": {"avg":28.0,"max":32},
        "perf": {"iops":4_100_000,"bw_bytes":16_800_000_000},
        "errors": 0
      }]
    }]
  }]
}
```

---

## 8. TUI 설계

[[air]] `air_tui` 의 트리·제어 UX를 거의 그대로 차용하되, "노드 트리" 대신 **PID→TID→opcode→stage 분해 트리** 를 보여 주고 **병목 stage 를 색으로 하이라이트** 한다.

### 8.1. 화면 레이아웃(목업)

```
 lighter  mode:[online] status:[play] interval:[3s] dev:[/dev/nvme0n1] ts:2026-06-15 11:12:02
 keys {↑/↓}move {→/←}(un)fold {space}play/pause {1-9}interval {f}filter-pid {s}sort {d}dump {q}quit
 ───────────────────────────────────────────────────────────────────────────────────────────────
 (*) Top
 ( ) +PID 27019  fio                                       iops:4.1M  bw:16.8GB/s
 ( )[O]++TID 27025  fio
        Read   e2e avg:118us med:95us p99:410us max:2.1ms  (12044 smp)   ███ bottleneck: S4 DEVICE 83%
          S1 USER_SUBMIT  avg:2.4us  med:1.9us  max:18us    ▏  2.0%
          S2 BLK_QUEUE    avg:1.2us  med:1.0us  max:9us     ▏  1.0%
          S3 DRV_SUBMIT   avg:0.9us  med:0.8us  max:5us     ▏  0.8%
          S4 DEVICE       avg:98us   med:82us   max:2.0ms   █████████████████  83.1%   ◀ BOTTLENECK
          S5 CPL_PROC     avg:1.5us  med:1.1us  max:21us    ▏  1.3%
          S6 USER_RETURN  avg:13us   med:4us    max:900us   ██  11.0%
          SQ depth avg:28 max:32   errors:0
 ( )[O]++TID 27026  fio
        Write  e2e avg:240us ...                            ███ bottleneck: S1 USER_SUBMIT 47%  ◀ tag 고갈 의심
          S1 USER_SUBMIT  avg:112us  ...                    █████████  47%  ◀ BOTTLENECK
          ...
 ( ) +PID 30142  nvme                                       iops:1   (passthrough)
```

- **막대(█)** = 해당 stage가 e2e에서 차지하는 비중(share). 가장 큰 stage에 `◀ BOTTLENECK` 마커 + 강조색.
- **Period/Cumulation 토글**: `c` 키로 전환(air의 Period/Cumulation 두 통계 차용).
- **정렬**: `s` 로 PID/TID를 e2e avg 또는 IOPS 기준 정렬(병목 큰 순).

### 8.2. 제어 키 (air 차용 + 확장)

| 키 | 동작 | 비고 |
|----|------|------|
| ↑/↓ | 커서 이동 | air |
| →/← | fold/unfold | air |
| space | play/pause | air |
| 1-9 | streaming interval(초) | air, collector에 명령 |
| f | PID 필터 입력 | lighter 확장 → filters 맵 갱신 |
| o / x | 프로파일 시작/중지(attach/detach) | air i/o/x 차용 |
| s | 정렬 기준 변경 | lighter 확장 |
| c | Period↔Cumulation | air |
| d | 현재 스냅샷 JSON/CSV 덤프 | FR-8 |
| q/esc | 종료 | air |

### 8.3. 오프라인 모드

air TUI 처럼 `--file lighter_xxx.json` 으로 과거 스트림을 재생(offline). collector 없이 TUI만 실행.

---

## 9. 병목 식별 알고리즘

### 9.1. I/O 단위

한 완료 I/O에 대해 `bottleneck_stage = argmax_s (S_s)`, `share = S_s / e2e`.

### 9.2. 집계(PID/TID/opcode) 단위

interval 내 모든 샘플의 stage별 평균 비중으로 지배 stage 산출:

```
share_s = mean(S_s) / mean(e2e)          # 각 stage 평균 비중
bottleneck = argmax_s share_s
```

추가 진단 규칙(휴리스틱, TUI에 힌트 표시):

| 패턴 | 의심 원인 |
|------|----------|
| S1 큼 + SQ depth 포화(=max) | **블록 tag/큐 깊이 고갈** → iodepth↑ 또는 nr_requests↑ |
| S2 큼 | I/O 스케줄러 지연 → `none` 스케줄러 검토 |
| S4 큼 + p99≫median | **디바이스 tail latency** (GC, 열, write cliff) |
| S6 큼 | **완료 수확 지연** → io_getevents 폴링 주기/배치, 완료측 CPU 경합 |
| S5 큼 | IRQ/softirq 경합, CPU affinity |
| E2E ≫ ΣS1..S6 | 계측 누락 구간(스케줄 대기 등) — 잔차 표시 |

---

## 10. 설정 / 필터

- **대상 디바이스**: `--dev /dev/nvme0n1` → devt(major:minor)로 변환해 `filters` 맵에 기록. 또는 NVMe controller 전체.
- **대상 PID/TID**: `--pid`, `--comm fio`(comm 매칭). 미지정 시 해당 디바이스에 I/O하는 모든 PID 자동 탐지(air의 target_detector 역할 — `src/target_detector/`).
- **opcode 필터**: `--op read,write`.
- **모드**: `--aggregate`(기본) / `--events --sample 1000`.
- **출력**: `--json /tmp/...` / `--socket`.
- 컴파일타임 설정은 불필요(air 와 차이) — eBPF는 런타임 attach. 단, air 식 `.cfg` 로 노드/필터 on-off 프리셋을 줄 수는 있음(선택).

---

## 11. 구현 로드맵

- [ ] **Phase 0 — 골격**: libbpf skeleton 빌드 파이프라인, BTF/CO-RE 확인, `nvme:nvme_sq`+`nvme:nvme_complete_rq` 만으로 **S4(device) per-(qid,cid) 히스토그램**(= nvme-pcie-tracer `nvme_latency.bt` 의 libbpf 이식). 검증 기준: `fio` device latency가 fio 리포트와 ±오차 내 일치.
- [ ] **Phase 1 — per-PID/TID 귀속**: `block_rq_insert` 에서 발행 tid 캡처 + request* 상관키 + 다리1/다리2 구현. 검증: `fio --name=A` vs `B` 두 잡이 PID별로 분리.
- [ ] **Phase 2 — 전 계층 stage 분해**: syscall enter/exit + block insert/issue/complete 추가 → S1..S6 완성. 검증: ΣS ≈ E2E.
- [ ] **Phase 3 — Collector 집계/스트리밍**: Period/Cumulation, percentile 근사, JSON 출력.
- [ ] **Phase 4 — TUI**: ncurses 트리 + 병목 하이라이트 + 제어키. air `tool/tui/` 구조 참고.
- [ ] **Phase 5 — 경로 확장**: io_uring tracepoint, ioctl passthrough 정밀화, 동기 read/write.
- [ ] **Phase 6 — 진단 휴리스틱(§9.2) + 스냅샷/오프라인 모드**.

검증용 워크로드: `fio`(libaio/io_uring, iodepth 스윕), `nvme-cli`(`nvme read`), custom libaio 미니 앱.

---

## 12. 기술 선택 근거

| 항목 | 선택 | 이유 / 대안 |
|------|------|------------|
| eBPF 로더 | **libbpf + CO-RE** | 커널 버전 이식(NFR-3). 대안 BCC는 런타임 clang 의존·무거움 |
| 프로브 우선순위 | **tracepoint > fentry > kprobe** | 안정 ABI·저오버헤드. kprobe(`nvme_irq`)는 보조 |
| 집계 위치 | **in-kernel 히스토그램** | per-event 전송 회피(NFR-1). 대안 ringbuf는 상세/샘플링용 |
| Collector/TUI 언어 | **C(ncurses)** 1차 | 레퍼런스(air/nvme-pcie-tracer)와 일관. 2차로 Rust(libbpf-rs+ratatui) 검토 |
| 상관키 | request*(블록) + (qid,cid)(NVMe) | tracepoint가 노출하는 식별자에 맞춤(§6.2) |
| device latency | doorbell→completion 블랙박스 | PCIe TLP 직접 계측은 비목표(=nvme-pcie-tracer 영역) |

---

## 13. 리스크 / 한계

1. **tracepoint 인자 가용성**: `nvme:*` tracepoint가 `request*`를 노출하지 않으면 다리2를 kprobe(`nvme_queue_rq`)로 구현해야 함 → BTF로 사전 검증 필요(§6.1 주).
2. **상관키 누수**: 에러/취소/타임아웃 I/O는 complete가 안 올 수 있음 → `inflight` 맵에 TTL/주기적 GC 필요(맵 가득 참 방지).
3. **제출≠완료 스레드**(io_uring SQPOLL, libaio 별도 완료 스레드): 귀속은 "발행 스레드" 고정(§6.3)으로 일관성 유지하되, TUI에 명시.
4. **S1 경계 모호성**: syscall→block 사이 경로가 파일시스템/DIO에 따라 다름 → 초기엔 DIO(O_DIRECT) 경로 우선 지원, 버퍼드 I/O는 별도(페이지캐시·writeback 비동기성으로 귀속 난해).
5. **고 IOPS 오버헤드**: 다수 tracepoint 동시 attach 시 누적 비용 → 집계 모드·필터 조기 컷·샘플링으로 완화. 실측 벤치 필수(NFR-1).
6. **권한/보안**: CAP_BPF/PERFMON 필요. 컨테이너 환경 제약 문서화.

---

## 14. 참고 자료 / 코드 참조

### 14.1. 레퍼런스 프로젝트 (코드 위치)

- [[air]] — 실시간 프로파일러(데이터·TUI 모델)
  - 노드 데이터 모델: `air/src/data_structure/NodeData.h`, `air/src/meta/NodeMeta.h`
  - 처리/스트리밍: `air/src/process/`, `air/src/stream/Stream.cpp`, `air/src/output/OutputManager.cpp`
  - 대상 탐지: `air/src/target_detector/Detector.cpp`
  - TUI: `air/tool/tui/` (`Viewer.cpp`, `Book.cpp`, `KeyEventHandler.cpp`, `Player.cpp`)
  - CLI: `air/tool/cli/`
  - 노드 타입·config 문법: `air/docs/user_guide.md`
- [[nvme-pcie-tracer]] — eBPF/커널 NVMe 트레이서(훅·이벤트 모델)
  - 레이턴시 bpftrace: `deep/projects/nvme-pcie-tracer/bpf/nvme_latency.bt`
  - 전체 트레이서: `deep/projects/nvme-pcie-tracer/bpf/nvme_tracer.bt`
  - 이벤트 구조/타입: `deep/projects/nvme-pcie-tracer/kernel/nvme_trace_event.h`
  - NVMe 커맨드 디코딩: `deep/projects/nvme-pcie-tracer/user/nvme_trace_decoder.c`
  - 설계: `deep/projects/nvme-pcie-tracer/docs/01_pcie_fault_based_detection.md`

### 14.2. 커널 경로 분석

- blk-mq/NVMe 드라이버 경로: `deep/analysis/kernel/`
- 관련 노트: `deep/notes/kernel/nvme-driver.md`

### 14.3. 외부 (확인 권장)

- libbpf CO-RE: https://nakryiko.com/posts/bpf-portability-and-co-re/
- `tracepoint:nvme:*` 정의: 커널 `include/trace/events/nvme.h`
- biolatency(BCC/libbpf-tools): in-kernel 히스토그램 집계 패턴 레퍼런스

---

## 15. 용어

- **stage**: I/O 경로의 한 구간(S1..S6). air의 latency range에 대응.
- **상관키(correlation key)**: 한 I/O를 계층 간에 잇는 식별자(tid → request* → (qid,cid)).
- **귀속(attribution)**: 통계를 발행 PID/TID로 묶는 것.
- **Period / Cumulation**: 최근 interval / 누적 통계(air 차용).
- **SQ/CQ**: NVMe Submission/Completion Queue. **cid/qid**: command id / queue id.
- **CO-RE**: Compile Once – Run Everywhere (libbpf+BTF 이식성).
- **doorbell**: SQ tail을 디바이스에 알리는 MMIO 레지스터 쓰기.
