# lighter 설계 문서 — eBPF 기반 NVMe I/O 경로 per-PID/TID 레이턴시 프로파일러 (TUI)

> 작성일: 2026-06-15
> 대상 프로젝트 트리(코드): `company/lighter/`
> 관련: [[nvme-pcie-tracer]] (선행 eBPF/커널 NVMe 트레이서), `deep/analysis/kernel/` (blk-mq/NVMe 드라이버 경로)

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

lighter 가 메우는 빈틈:

- **무수정(no-instrumentation)**: 커널 tracepoint/kprobe만 사용 → fio·nvme-cli·임의의 custom app을 **재빌드 없이** 프로파일.
- **계층별 분해(stage breakdown)**: end-to-end 한 숫자가 아니라 syscall→blk-mq→driver→device→completion 각 구간 latency.
- **per-PID/TID 귀속**: 어느 프로세스/스레드의 I/O가 어디서 느린지.
- **실시간 TUI**: 터미널에서 트리형 실시간 화면 + 병목 하이라이트.

### 1.3. 비파괴·순수 eBPF 원칙

선행 [[nvme-pcie-tracer]] 는 커널 모듈/IOMMU page-fault 같은 저수준·파괴적 기법까지 다뤘다. lighter 는 그와 달리 **순수 eBPF(CO-RE)** 만으로 비파괴 계측하고, 그 위에 **계층 분해 + per-PID/TID 집계 + 실시간 TUI** 를 얹는 것을 목표로 한다.

- nvme-pcie-tracer 에서 재사용하는 것: NVMe tracepoint 훅 포인트, `request` 포인터·`(qid,cid)` 상관키, opcode/SLBA/NLB 디코딩 로직, 레이턴시 측정 패턴.
- nvme-pcie-tracer 와 다른 점: 커널 모듈/IOMMU fault 같은 파괴적 기법을 배제하고, 계층 분해·per-PID/TID 집계·TUI 를 추가한다.

---

## 2. 요구사항

### 2.1. 기능 요구사항 (Functional)

- **FR-1 (대상 애플리케이션)**: 아래 I/O 경로를 모두 프로파일할 수 있어야 한다.
  - Custom app — `libaio`(`io_submit`/`io_getevents`), 동기 `read`/`write`/`pread`/`pwrite`, `ioctl` passthrough.
  - `nvme-cli` — `ioctl(NVME_IOCTL_IO_CMD/ADMIN_CMD)` 패스스루.
  - `fio` — libaio/io_uring/psync/sync ioengine.
  - (확장) `io_uring` 경로.
- **FR-2 (계층 분해)**: 한 I/O에 대해 §4의 stage들의 구간 latency를 측정.
- **FR-3 (per-PID/TID 귀속)**: 모든 통계를 (PID, TID) 단위로 분리. I/O를 **발행한** 스레드 기준으로 귀속(완료는 IRQ 컨텍스트에서 일어나므로 발행 시점 정보를 저장해 둔다 — §6.2).
- **FR-4 (필터)**: target PID/TID, target 디바이스(`/dev/nvmeXnY` 또는 BDF/devt), opcode(Read/Write/Flush/...) 로 필터.
- **FR-5 (통계)**: 각 (PID,TID,stage,opcode) 별 avg/median/min/max/p99 + 히스토그램 + sample count. **Period**(최근 interval)와 **Cumulation**(누적) 두 축을 함께 제공.
- **FR-6 (병목 식별)**: 각 PID/TID의 I/O에서 가장 큰 비중을 차지하는 stage를 자동 산출·하이라이트(§9).
- **FR-7 (TUI)**: 터미널 실시간 트리 뷰 + 제어(play/pause, streaming interval 변경, fold/unfold, PID 필터, quit).
- **FR-8 (스냅샷/내보내기)**: 현재 화면을 JSON/CSV로 덤프(사후 분석용).

### 2.2. 비기능 요구사항 (Non-functional)

- **NFR-1 (저오버헤드)**: 고 IOPS(수백만 IOPS)에서도 영향 최소화. **in-kernel 집계 + 샘플링**으로 per-event 유저스페이스 전송을 피한다(§6.6). 목표: kprobe 대비 tracepoint 우선, per-event 오버헤드 < ~수백 ns, IOPS 영향 한 자릿수 %. 기준 워크로드 기반 정량 계산은 §16.
- **NFR-2 (무수정)**: 대상 바이너리 재빌드 불필요.
- **NFR-3 (이식성)**: libbpf **CO-RE(Compile Once – Run Everywhere)** + BTF로 커널 버전 간 이식. 타깃: Linux 5.8+ (ring buffer, BTF). 레퍼런스 환경은 Rocky/CentOS 9 (5.14+).
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

> 주: passthrough는 보통 **동기** — 제출 스레드가 완료까지 블록되므로 (PID,TID) 귀속이 자연스럽다. libaio/io_uring은 제출/완료 스레드가 같을 수도 다를 수도 있다 → request* 상관키로 발행 스레드 정보를 운반(§6.2).

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

### 3.5. SPDK 유저스페이스 경로 (커널 우회) — 별도 트랙

**중요**: SPDK 애플리케이션은 커널 블록/NVMe 드라이버를 **우회**한다. VFIO/uio로 NVMe를 유저스페이스에 바인딩하고, 폴링 모드 드라이버가 유저스페이스에서 직접 SQ에 기록·doorbell write·CQ 폴링을 한다. 따라서 §3.1~3.4의 **커널 훅(`syscalls:*`, `block:*`, `nvme:*`)이 전혀 발화하지 않는다.** SPDK 대상은 §6의 커널 eBPF와 다른 **USDT/uprobe 트랙**으로 계측한다(설계는 §17).

```
[유저스페이스 — SPDK 프로세스 내부]
  app → spdk_bdev_write / spdk_nvme_ns_cmd_write   ▶ U1 submit (T0)
        │  (커널 진입 없음 — 라이브러리 함수 호출)
        ▼
  nvme_pcie qpair: SQE 빌드 → SQ 기록 → doorbell(MMIO write, mmap된 BAR)
        ▶ U2 doorbell (T1)
========│=== PCIe ===  doorbell ──▶ NVMe SSD ──DMA──▶ CQ write (IRQ 없음)
        ▼
  device 처리 (블랙박스)
        ▼
  spdk_nvme_qpair_process_completions() 폴링 루프가 CQ phase 비트 관찰
        ▶ U3 completion 감지 (T2)
  완료 콜백(cb_fn) 호출 → app
        ▶ U4 callback (T3)
```

stage 모델이 커널과 다르다: syscall/blk-mq 단계가 없고 `U_SUBMIT(T0→T1) / U_DEVICE(T1→T2) / U_CPL(T2→T3)` 로 단순화된다. 상관키는 커널의 `request*` 대신 **명령 컨텍스트 포인터(`nvme_request*`/`bdev_io*`)** 또는 `(qpair, cid)`. per-PID/TID 대신 **per-SPDK-thread(reactor)/per-qpair** 가 자연 차원이다.

---

## 4. 레이턴시 스테이지 모델 (핵심)

lighter 는 위 타임스탬프 T0…T8 사이의 구간을 **stage** 로 정의한다. 한 I/O의 latency를 단일 end-to-end 값이 아니라 구간(range) 단위로 분해하는 것이 이 도구의 정수다.

| Stage | 구간 | 시각 | 의미 / 무엇을 진단하나 |
|-------|------|------|----------------------|
| **S1 USER_SUBMIT** | T0→T2 | sys_enter → block_rq_insert | syscall 진입~블록 진입. VFS·DIO 셋업, **tag 할당 대기(큐 깊이 포화)** 가 여기서 드러남 |
| **S2 BLK_QUEUE** | T2→T3 | block_rq_insert → block_rq_issue | 블록 레이어 큐잉·스케줄러(mq-deadline/none) 대기·plug merge 시간 |
| **S3 DRV_SUBMIT** | T3→T5 | block_rq_issue → nvme:nvme_sq | 드라이버에서 SQE 빌드 + SQ 기록 + **doorbell(MMIO write)** |
| **S4 DEVICE** | T5→T7 | nvme_sq → nvme_complete_rq | **디바이스 처리 시간**(doorbell→completion). 순수 SSD/컨트롤러 latency (블랙박스) |
| **S5 CPL_PROC** | T7→T7' | complete_rq → block_rq_complete | 완료 인터럽트 처리·softirq·bio_endio |
| **S6 USER_RETURN** | T7'→T8 | block_rq_complete → sys_exit | 완료~유저 반환. **aio 완료 큐 적체, io_getevents 폴링 지연** 이 드러남 |
| **E2E END_TO_END** | T0→T8 | sys_enter → sys_exit | 애플리케이션 관점 총 latency (= ΣS1..S6 + 비계측 잔차) |

추가 보조 지표:

| 지표 | 종류 | 측정 | 용도 |
|------|------|------|------|
| `SQ_DEPTH` | Queue depth | 큐별 제출−완료 in-flight 카운트 (§4.1) | 큐 깊이 포화 진단 |
| `PERF` | Performance | opcode별 IOPS/BW | 워크로드 특성 |
| `HIST_S4` | Histogram | device latency 분포 | tail latency 진단 |
| `ERR_CNT` | Count | NVMe status != 0 | 에러율 |

> **병목 정의**: 한 I/O에서 `max(S1..S6)` 가 그 I/O의 병목 stage. PID/TID 집계 수준에서는 §9의 가중 방식으로 "지배 stage" 를 산출한다.

### 4.1. per-hardware-queue(hctx) 차원

blk-mq 는 CPU(또는 CPU 그룹)마다 **hardware context(hctx)** 를 두고, 각 hctx는 tag 집합(sbitmap)으로 in-flight request를 관리한다. NVMe PCIe 드라이버에서 hctx는 **NVMe Submission Queue(SQ)** 와 거의 1:1로 매핑된다(`hctx->queue_num` ≈ `nvme_queue->qid`, 폴링 큐 제외). 따라서 **`qid` 를 집계의 1급 차원으로** 추가하면 다음을 얻는다:

- **큐별 현재 in-flight 개수(depth gauge)**: 각 SQ에 몇 개의 커맨드가 발행되었으나 아직 완료되지 않았는지. 큐 깊이 포화·핫큐(hot queue) 탐지.
- **큐별 latency**: 동일한 stage(특히 S4 DEVICE, E2E)를 `qid` 로 쪼갠 히스토그램. 특정 큐만 느린 경우(IRQ affinity 편중, 컨트롤러 큐 불균형, CPU 경합)를 드러냄.

두 측정 모두 이미 확보한 식별자(`io_ctx.qid`, §6.2)로 공짜에 가깝게 파생되며, in-flight depth는 §6.5의 per-queue gauge 맵으로 직접 관리한다. hctx tag(sbitmap) 점유율을 직접 읽는 정밀 방식과 submit/complete 카운팅으로 추정하는 경량 방식의 트레이드오프는 §6.5 참조.

> **PID/TID 차원과의 관계**: `qid` 는 PID/TID와 **직교(orthogonal)** 하는 차원이다. 한 스레드의 I/O가 여러 큐에 흩어지거나(여러 CPU에서 제출), 한 큐를 여러 스레드가 공유할 수 있다. TUI는 PID→TID→stage 트리(§8.1)와 별도로 **큐 중심 뷰**(§8.2 `v` 토글)를 제공한다.

---

## 5. 전체 아키텍처

수집(eBPF) → 집계/스트리밍(collector) → 표시(TUI) 의 3계층 구조다.

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
- **Collector**: C 또는 Rust(libbpf-rs). 초기엔 C.
- **TUI**: ncurses(C). (Rust 선택 시 `ratatui`.)

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

> **귀속 원칙(FR-3)**: `pid/tid/comm` 은 **반드시 제출 시점**(① 또는 ③, `bpf_get_current_pid_tgid`/`bpf_get_current_comm`)에 채운다. 완료(⑧⑨)는 IRQ/softirq 컨텍스트라 current task가 무의미하므로, `inflight[request*]` 에 저장해 둔 발행 정보를 그대로 사용한다. 즉 발행 스레드 정보를 커널 맵으로 명시적으로 운반하는 것이 lighter 귀속 로직의 핵심이다.

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
// ⑥ nvme_sq:        c.ts[5]=T5 (submit), c.qid=qid, qdepth[qid]++ (§6.5 SQ depth)
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
            stats_update(agg_key(c->pid,c->tid,c->qid,c->opcode,s), delta(c,s));
    }
    qdepth[c->qid]--;                        // §6.5 in-flight gauge 감소
    inflight.delete(rq); rq_of.delete(qid,cid);
}
```

### 6.4. 집계 키 & 통계 맵

```c
struct agg_key {       // 집계 차원
    u32 pid;
    u32 tid;
    u16 qid;           // NVMe SQ id = blk-mq hctx (§4.1). 0xFFFF = 전체 합산
    u8  opcode;        // Read/Write/Flush/...
    u8  stage;         // S1..S6, E2E
};
struct lat_stats {     // 구간 latency 통계
    u64 count, sum, min, max;
    u64 slots[64];     // log2 히스토그램 버킷
    // median/p99 는 slots 로부터 유저스페이스에서 근사
};
```

- **median/p99**: in-kernel 에서는 log2 히스토그램 슬롯만 누적(저비용). percentile 은 collector 가 슬롯에서 보간으로 근사.
- **Period vs Cumulation**: collector 가 매 interval마다 stats 맵을 읽고 **델타(Period)** 와 **누적(Cumulation)** 을 따로 유지. (in-kernel은 단조 누적만, 차분은 유저스페이스에서.)

> **qid 차원 비용**: `qid` 는 이미 `io_ctx` 에 있으므로 `agg_key` 에 추가해도 추가 훅이 필요 없다. 다만 차원 곱(pid×tid×qid×opcode×stage)으로 stats 맵 엔트리 수가 늘 수 있어, collector는 큐 차원을 **요청 시(on-demand)** 펼치거나 `qid=0xFFFF`(전체 합산) 엔트리를 기본으로 두고 드릴다운할 때만 큐별로 분해하는 전략을 쓴다.

### 6.5. per-queue in-flight depth gauge & hctx tag 상태

각 SQ(=hctx)의 현재 in-flight 개수를 두 방식으로 얻을 수 있다.

```c
// per-queue in-flight gauge (경량·기본)
// key=qid, value=현재 미완료 커맨드 수. submit에서 +1, complete에서 -1.
struct { __uint(type, BPF_MAP_TYPE_PERCPU_HASH); ... } qdepth SEC(".maps");

// ⑥ nvme_sq:           __sync_fetch_and_add(&qdepth[qid], 1);  cur = qdepth[qid];
//                       qdepth_max[qid] = max(qdepth_max[qid], cur);   // 최대치 추적
// ⑧ nvme_complete_rq:  __sync_fetch_and_sub(&qdepth[qid], 1);
```

| 방식 | 메커니즘 | 정확도 | 비용 | 비고 |
|------|---------|--------|------|------|
| **submit/complete 카운팅(기본)** | nvme_sq +1 / nvme_complete_rq −1 per qid | 드라이버가 SQ에 넣은 시점 기준 in-flight | 거의 0 | percpu 합산 필요. 음수 방지(드롭/리셋 시 GC) |
| **hctx tag(sbitmap) 직접 읽기(정밀)** | `hctx->tags->bitmap_tags` 점유 비트 수 / `hctx->nr_active` 를 kprobe·iter로 관측 | blk-mq tag 점유 실측(드라이버 진입 전 tag 할당분 포함) | 중(구조체 워크·BTF 의존) | `nr_active`(atomic) 읽기가 가장 싸고, 비트맵 순회는 `bpf_for`/iter 필요. 커널 버전별 필드 차이 큼 → CO-RE relocation·검증 필수 |

> 기본은 카운팅 방식(견고·이식적). tag 점유 실측이 필요하면(예: tag 고갈로 S1이 큰지 확인) 정밀 방식을 옵션으로 켠다. 둘을 함께 쓰면 "tag는 다 찼는데(=정밀) 디바이스에 발행된 건 적다(=카운팅)" 같은 **tag 병목 vs 디바이스 병목** 구분이 가능하다.

### 6.6. per-event vs in-kernel 집계 (오버헤드 결정)

| 모드 | 메커니즘 | 오버헤드 | 용도 |
|------|---------|---------|------|
| **집계(기본)** | in-kernel HASH 히스토그램, 유저스페이스는 interval마다 map dump | 매우 낮음(전송 없음) | 실시간 TUI 통계 (NFR-1) |
| **이벤트(상세)** | RINGBUF 로 io_ctx 방출 | 높음(고 IOPS시) | 디버깅·타임라인·스냅샷, **샘플링**(1/N) 적용 |

> 기본은 집계 모드. per-event 전송은 고 IOPS에서 비싸므로(선행 [[nvme-pcie-tracer]] 의 relay 방식 참조), lighter 는 **biolatency류 in-kernel 히스토그램**을 1순위로 한다. 상세 이벤트는 옵션 + 샘플링.

---

## 7. Collector(데몬) 설계

### 7.1. 책임

1. BPF skeleton 로드·attach, BTF로 CO-RE 재배치.
2. `filters` 맵에 target PID/dev/opcode 기록(TUI/CLI에서 변경 가능).
3. 매 `streaming_interval` 초마다 `stats` 맵 스냅샷 → Period/Cumulation 계산.
4. PID→comm, 디바이스 devt→`/dev/nvmeXnY` 이름 보강(`/proc`, `/sys`).
5. JSON 스트림 출력(파일 `/tmp/lighter_<ts>_<pid>.json` 또는 unix socket/shm) — TUI가 소비.
6. TUI/CLI 제어 명령 수신(interval 변경, pid 필터, pause).

### 7.2. 데이터 모델

집계 차원과 통계 축:

- **차원**: `(PID, TID, opcode, stage)`. opcode = Read/Write/Flush/..., stage = S1..S6/E2E.
- **통계**: 각 차원마다 count / avg / median / min / max / p99 / 히스토그램.
- **시간 축**: `Period`(최근 streaming interval 구간) 와 `Cumulation`(시작 이후 누적) 두 벌.
- **보조 지표**: SQ depth(avg/max), IOPS/BW, error count — opcode 단위로 함께 유지.

### 7.3. JSON 스트리밍 포맷(예)

한 interval = 한 JSON object 로 출력하여 TUI(또는 사후 분석 스크립트)가 소비한다.

```json
{
  "schema": "lighter/v1",
  "timestamp": "2026-06-15T11:12:02",
  "interval_s": 3,
  "device": "/dev/nvme0n1",
  "queues": [
    {"qid":1, "cpu":0, "inflight":31, "tag_total":32, "tag_allocated":31,
     "s4":{"avg_ns":92000,"median_ns":80000,"p99_ns":380000,"samples":3300}, "iops":1100000,
     "owners":[{"pid":27019,"tid":27025,"share_pct":100.0}]},
    {"qid":2, "cpu":1, "inflight":32, "tag_total":32, "tag_allocated":32,
     "s4":{"avg_ns":140000,"median_ns":96000,"p99_ns":2000000,"samples":3000}, "iops":1000000,
     "hot":true, "owners":[{"pid":27019,"tid":27026,"share_pct":100.0}]}
  ],
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

터미널 실시간 트리 화면으로, **PID→TID→opcode→stage 분해 트리** 를 보여 주고 **병목 stage 를 색으로 하이라이트** 한다.

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
- **Period/Cumulation 토글**: `c` 키로 전환.
- **정렬**: `s` 로 PID/TID를 e2e avg 또는 IOPS 기준 정렬(병목 큰 순).

### 8.2. 큐 중심 뷰 (`v` 토글, §4.1)

`v` 키로 PID 트리 ↔ 큐(hctx/SQ) 트리를 전환한다. 큐별 현재 in-flight depth와 큐별 latency를 한눈에 비교해 **큐 불균형·핫큐**를 진단한다.

```
 lighter  view:[QUEUE] status:[play] interval:[3s] dev:[/dev/nvme0n1]  ts:2026-06-15 11:12:02
 keys {v}view {↑/↓}move {→/←}(un)fold {space}play/pause {s}sort {q}quit
 ───────────────────────────────────────────────────────────────────────────────────────────
 (*) Top  (queues: 8, total in-flight: 213/256)
 ( )++Q1  cpu0   inflight: 31/32  ████████████████████  S4 avg:92us  med:80us  p99:380us  iops:1.1M
        owners: fio[27025] 100%
 ( )++Q2  cpu1   inflight: 32/32  █████████████████████ FULL  S4 avg:140us med:96us p99:2.0ms iops:1.0M  ◀ HOT
        owners: fio[27026] 100%
 ( )++Q3  cpu2   inflight:  4/32  ███                   S4 avg:88us  med:79us  p99:120us  iops:140K
        owners: nvme[30142] 60%, app[30201] 40%
 ...
        tag(sbitmap) 점유(정밀, §6.5): Q2 = 32/32 (allocated) vs 발행 28 → tag 대기 4 존재 → S1 병목 의심
```

- **inflight: 사용/총** = `qdepth[qid]` / 큐 tag 수. `FULL`/`◀ HOT`(latency 또는 depth가 평균보다 현저히 큼) 마커.
- 정밀 모드(§6.5)를 켜면 tag 점유 실측을 함께 표시해 **tag 대기 vs 디바이스 지연**을 분리.
- `owners` = 그 큐에 I/O를 넣은 PID/TID 비중(큐↔스레드 다대다 관계 §4.1 가시화).

### 8.3. 제어 키

| 키 | 동작 |
|----|------|
| ↑/↓ | 커서 이동 |
| →/← | fold/unfold |
| v | PID 트리 ↔ 큐 트리 뷰 전환 (§8.2) |
| space | play/pause |
| 1-9 | streaming interval(초) → collector에 명령 |
| f | PID 필터 입력 → filters 맵 갱신 |
| o / x | 프로파일 시작/중지(attach/detach) |
| s | 정렬 기준 변경 |
| c | Period↔Cumulation 전환 |
| d | 현재 스냅샷 JSON/CSV 덤프 (FR-8) |
| q/esc | 종료 |

### 8.4. 오프라인 모드

`--file lighter_xxx.json` 으로 과거 스트림을 재생(offline). collector 없이 TUI만 실행.

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
| 특정 큐만 depth FULL·latency↑ | **큐 불균형/핫큐** → IRQ affinity 편중, CPU 핫스팟, 큐 매핑 쏠림 (§8.2 큐 뷰) |
| tag 점유(정밀) ≫ 발행 in-flight(카운팅) | **tag 대기** → S1 병목과 일치, iodepth 대비 tag 부족 (§6.5) |

---

## 10. 설정 / 필터

- **대상 디바이스**: `--dev /dev/nvme0n1` → devt(major:minor)로 변환해 `filters` 맵에 기록. 또는 NVMe controller 전체.
- **대상 PID/TID**: `--pid`, `--comm fio`(comm 매칭). 미지정 시 해당 디바이스에 I/O하는 모든 PID 자동 탐지.
- **opcode 필터**: `--op read,write`.
- **모드**: `--aggregate`(기본) / `--events --sample 1000`.
- **출력**: `--json /tmp/...` / `--socket`.
- 컴파일타임 설정은 불필요 — eBPF는 런타임 attach. 선택적으로 노드/필터 on-off 프리셋을 `.cfg` 로 줄 수 있음.

---

## 11. 구현 로드맵

- [ ] **Phase 0 — 골격**: libbpf skeleton 빌드 파이프라인, BTF/CO-RE 확인, `nvme:nvme_sq`+`nvme:nvme_complete_rq` 만으로 **S4(device) per-(qid,cid) 히스토그램**. 검증 기준: `fio` device latency가 fio 리포트와 ±오차 내 일치.
- [ ] **Phase 1 — per-PID/TID 귀속 + per-queue 차원**: `block_rq_insert` 에서 발행 tid 캡처 + request* 상관키 + 다리1/다리2 구현. `agg_key` 에 `qid` 추가 + `qdepth` gauge(§6.5). 검증: `fio --name=A` vs `B` 두 잡이 PID별로 분리, 큐별 in-flight/latency가 큐 수만큼 나뉘어 표시.
- [ ] **Phase 2 — 전 계층 stage 분해**: syscall enter/exit + block insert/issue/complete 추가 → S1..S6 완성. 검증: ΣS ≈ E2E.
- [ ] **Phase 3 — Collector 집계/스트리밍**: Period/Cumulation, percentile 근사, JSON 출력.
- [ ] **Phase 4 — TUI**: ncurses 트리 + 병목 하이라이트 + 제어키.
- [ ] **Phase 5 — 경로 확장**: io_uring tracepoint, ioctl passthrough 정밀화, 동기 read/write.
- [ ] **Phase 6 — 진단 휴리스틱(§9.2) + 스냅샷/오프라인 모드 + hctx tag 정밀 모드(§6.5)**. 큐 중심 TUI 뷰(§8.2) 완성.
- [ ] **Phase 7 — SPDK USDT 계측 모드(§17)**: 커널 우회 경로 별도 트랙. 심볼/USDT 탐지 → U_SUBMIT/U_CPL 2점 latency → per-qpair·FDP placement 차원 → TUI 통합.

검증용 워크로드: `fio`(libaio/io_uring, iodepth 스윕), `nvme-cli`(`nvme read`), custom libaio 미니 앱.

---

## 12. 기술 선택 근거

| 항목 | 선택 | 이유 / 대안 |
|------|------|------------|
| eBPF 로더 | **libbpf + CO-RE** | 커널 버전 이식(NFR-3). 대안 BCC는 런타임 clang 의존·무거움 |
| 프로브 우선순위 | **tracepoint > fentry > kprobe** | 안정 ABI·저오버헤드. kprobe(`nvme_irq`)는 보조 |
| 집계 위치 | **in-kernel 히스토그램** | per-event 전송 회피(NFR-1). 대안 ringbuf는 상세/샘플링용 |
| Collector/TUI 언어 | **C(ncurses)** 1차 | 2차로 Rust(libbpf-rs+ratatui) 검토 |
| 상관키 | request*(블록) + (qid,cid)(NVMe) | tracepoint가 노출하는 식별자에 맞춤(§6.2) |
| device latency | doorbell→completion 블랙박스 | PCIe TLP 직접 계측은 비목표 |

---

## 13. 리스크 / 한계

1. **tracepoint 인자 가용성**: `nvme:*` tracepoint가 `request*`를 노출하지 않으면 다리2를 kprobe(`nvme_queue_rq`)로 구현해야 함 → BTF로 사전 검증 필요(§6.1 주).
2. **상관키 누수**: 에러/취소/타임아웃 I/O는 complete가 안 올 수 있음 → `inflight` 맵에 TTL/주기적 GC 필요(맵 가득 참 방지).
3. **제출≠완료 스레드**(io_uring SQPOLL, libaio 별도 완료 스레드): 귀속은 "발행 스레드" 고정(§6.2)으로 일관성 유지하되, TUI에 명시.
4. **S1 경계 모호성**: syscall→block 사이 경로가 파일시스템/DIO에 따라 다름 → 초기엔 DIO(O_DIRECT) 경로 우선 지원, 버퍼드 I/O는 별도(페이지캐시·writeback 비동기성으로 귀속 난해).
5. **고 IOPS 오버헤드**: 다수 tracepoint 동시 attach 시 누적 비용 → 집계 모드·필터 조기 컷·샘플링으로 완화. 실측 벤치 필수(NFR-1).
6. **권한/보안**: CAP_BPF/PERFMON 필요. 컨테이너 환경 제약 문서화.

---

## 14. 참고 자료 / 코드 참조

### 14.1. 선행 프로젝트 (코드 위치)

- [[nvme-pcie-tracer]] — eBPF/커널 NVMe 트레이서(훅·이벤트 모델 재사용)
  - 레이턴시 bpftrace: `deep/projects/nvme-pcie-tracer/bpf/nvme_latency.bt`
  - 전체 트레이서: `deep/projects/nvme-pcie-tracer/bpf/nvme_tracer.bt`
  - 이벤트 구조/타입: `deep/projects/nvme-pcie-tracer/kernel/nvme_trace_event.h`
  - NVMe 커맨드 디코딩: `deep/projects/nvme-pcie-tracer/user/nvme_trace_decoder.c`
  - 설계: `deep/projects/nvme-pcie-tracer/docs/01_pcie_fault_based_detection.md`

### 14.2. 커널 경로 분석

- blk-mq/NVMe 드라이버 경로: `deep/analysis/kernel/`
- 관련 노트: `deep/notes/kernel/nvme-driver.md`
- EG/FDP 커널 vs SPDK 처리 비교: [[nvme-eg-fdp-kernel-vs-spdk]] (`deep/notes/kernel/nvme-eg-fdp-kernel-vs-spdk.md`) — §3.5/§17 SPDK 트랙의 코드 근거

### 14.3. 외부 (확인 권장)

- libbpf CO-RE: https://nakryiko.com/posts/bpf-portability-and-co-re/
- `tracepoint:nvme:*` 정의: 커널 `include/trace/events/nvme.h`
- biolatency(BCC/libbpf-tools): in-kernel 히스토그램 집계 패턴 레퍼런스

---

## 15. 용어

- **stage**: I/O 경로의 한 구간(S1..S6).
- **상관키(correlation key)**: 한 I/O를 계층 간에 잇는 식별자(tid → request* → (qid,cid)).
- **귀속(attribution)**: 통계를 발행 PID/TID로 묶는 것.
- **Period / Cumulation**: 최근 interval / 누적 통계.
- **SQ/CQ**: NVMe Submission/Completion Queue. **cid/qid**: command id / queue id.
- **CO-RE**: Compile Once – Run Everywhere (libbpf+BTF 이식성).
- **doorbell**: SQ tail을 디바이스에 알리는 MMIO 레지스터 쓰기.

---

## 16. 오버헤드 예산 — 기준 워크로드 계산

> 아래 수치는 **모델 기반 추정**이다(전제·계산식 명시). 실측은 §16.6의 `bpftool prog show` 방법으로 교정한다. 절대값보다 **자릿수·상대 비교**로 읽을 것.

### 16.1. 기준 워크로드

```
fio --numjobs=16 --iodepth=512 --bs=4k --direct=1 --ioengine=libaio --rw=randread
```

| 파라미터 | 값 | 오버헤드에 주는 함의 |
|---------|----|--------------------|
| numjobs | 16 | I/O 발행 스레드 16개 → 보통 16 코어에 분산. BPF 훅은 **발행/완료가 일어난 그 코어**에서 실행 → fio I/O 경로 CPU와 직접 경합 |
| iodepth | 512 | 스레드당 512 in-flight |
| **동시 in-flight** | **16 × 512 = 8192** | `inflight`/bridge 맵이 동시에 ~8192 live 엔트리 보유 → **맵 사이징 필수**(§16.5) |
| bs / direct | 4k / O_DIRECT | 페이지캐시 우회 → S1 귀속 단순(설계 가정과 일치). 4k → IOPS가 높아 per-IO 비용이 그대로 증폭 |
| libaio | 배치 | `io_submit`/`io_getevents` **syscall 훅(S1 진입, S6 종료)은 배치당 1회로 amortize**. block/nvme 6개 훅은 **I/O당 고정** |

이 QD(8192 outstanding)는 단일 SSD 큐 용량을 크게 초과하므로, 보통 **device-bound**(코어에 유휴 여유 존재)다. 단, 초고속 디바이스나 다중 SSD로 코어가 포화되면 **CPU-bound**로 바뀐다 — 두 레짐을 §16.4에서 모두 계산한다.

### 16.2. I/O 1건당 발화 훅 & 비용 (full 모드, Phase 2)

| 훅 | I/O당 발화 | 추정 비용 | 작업 |
|----|-----------|----------|------|
| `sys_enter_io_submit` (S1) | 배치당 1회† | ~150ns | ktime, comm, tid_stack update |
| `block_rq_insert` | 1 | ~200ns | tid_stack lookup + io_ctx(64B) 생성 + inflight.update |
| `block_rq_issue` | 1 | ~120ns | inflight lookup + ts |
| `nvme_setup_cmd` | 1 | ~180ns | request*↔(qid,cid) bridge 2맵 update |
| `nvme_sq` | 1 | ~150ns | lookup + ts + qdepth atomic inc |
| `nvme_complete_rq` | 1 | ~150ns | (qid,cid) lookup + ts |
| `block_rq_complete` | 1 | ~120ns | lookup + ts |
| `sys_exit_io_getevents` + finalize (S6) | 배치당 1회† | ~500ns | 6 delta 계산 + 6 히스토그램 update + 3 맵 delete |
| **합계 (배치 amortize 가정)** | | **≈ 1.0–1.2µs/IO** | syscall 2훅이 배치로 희석된 경우 |
| **합계 (batch=1, 최악)** | | **≈ 1.5–1.7µs/IO** | fio 기본 `iodepth_batch_submit=1` |

† fio 기본은 제출 배치=1이지만 완료(`io_getevents`)는 다건 reap이 흔하다 → S6는 거의 amortize, S1은 워크로드 설정에 따라 per-IO일 수 있음. 보수적으로 **full ≈ 1.5µs/IO** 로 둔다.

### 16.3. 모드별 I/O당 추가 CPU

| 모드 | 발화 훅 수 | I/O당 CPU |
|------|----------|----------|
| **Phase 0 — S4(device)만** | 2 | **~0.4µs** |
| **Phase 1 — +per-PID/TID +per-queue** | ~5 | **~0.8µs** |
| **Phase 2 — full 전 계층 분해** | ~8 | **~1.5µs** |
| (tracepoint에 request* 없어 kprobe 폴백 시, §6.1) | +1~2 | +0.4–0.8µs |

### 16.4. IOPS / CPU 영향 계산

추가 CPU 코어 수 = (I/O당 추가 CPU) × (완료 IOPS). 디바이스 등급별로:

| | Phase 0 (0.4µs) | Phase 1 (0.8µs) | Full (1.5µs) |
|---|---|---|---|
| **1.0M IOPS (mainstream Gen4 NVMe)** | 0.40 코어 (16코어의 2.5%) | 0.80 코어 (5.0%) | 1.50 코어 (9.4%) |
| **3.0M IOPS (top-tier/다중 SSD)** | 1.20 코어 (7.5%) | 2.40 코어 (15%) | 4.50 코어 (28%) |

이 "추가 코어"가 IOPS를 깎는지는 레짐에 달림:

- **device-bound (이 워크로드의 일반적 경우, 코어 유휴 여유 O)**: 추가 CPU는 유휴 헤드룸이 흡수 → **IOPS 거의 불변**, 대신 CPU 사용률이 위 표만큼 상승. Phase 0/1은 사실상 공짜.
- **CPU-bound (코어 포화, 매우 빠른 디바이스/다중 SSD)**: 처리량 비례식
  ```
  IOPS_new / IOPS_old ≈ C_base / (C_base + C_added)
  C_base = 포화 코어의 IO당 커널 경로 CPU (제출+완료) ≈ 2.5µs/IO 가정
  ```
  → Full(1.5µs): 2.5/(2.5+1.5) = **0.63 → IOPS 약 −37%**
  → Phase 1(0.8µs): 2.5/3.3 = 0.76 → **−24%**
  → Phase 0(0.4µs): 2.5/2.9 = 0.86 → **−14%**

> 결론: 이 워크로드가 **device-bound면 Phase 0/1은 IOPS 영향 거의 없음**(CPU만 +0.4~0.8코어). **CPU-bound로 몰린 경우에만 full이 −30%대로 아파짐** → full은 진단용 단기 세션으로.

### 16.5. 메모리 / 맵 사이징 (8192 in-flight 때문에 중요)

동시 8192 I/O가 살아 있으므로 상관키 맵을 **넉넉히** 잡아야 한다(가득 차면 update 실패 → 샘플 유실).

| 맵 | max_entries 권장 | 엔트리 | 대략 메모리 |
|----|-----------------|--------|-----------|
| `inflight` HASH<request*, io_ctx> | ≥ 16384 (8192의 2×) | 64B | ~1.0MB |
| `rq_of` / `cid_of` bridge | ≥ 16384 | 8~16B | ~0.3MB |
| `tid_stack` HASH<tid, tinfo> | ≥ 64 (스레드 16) | ~32B | 수 KB |
| `qdepth`/`qdepth_max` PERCPU | ≥ 256 (qid) | 8B × ncpu | 수십 KB |
| `stats` HASH<agg_key, lat_stats> | pid16×tid16×qid×op×stage | ~560B | 보통 수 MB 이하 (스레드당 큐 국소성으로 실제 카디널리티 작음) |

- **이벤트 모드 금지 수준**: full 이벤트(샘플링 없음) = ~1M~3M events/s × ~48B = **48–144MB/s** ring buffer + 유저스페이스 소비 → 이 IOPS에서 **반드시 1/1000 이상 샘플링**(→ 1k~3k events/s로 무해).
- 8192 엔트리 해시는 캐시 풋프린트(~1MB+)가 있어 per-op 비용을 약간(수~수십 ns) 끌어올림 — §16.2 추정에 이미 보수적으로 반영.

### 16.6. 실측 교정 방법

```bash
# 각 BPF 프로그램의 누적 실행시간/횟수 → IO당 ns 실측
bpftool prog show           # run_time_ns, run_cnt 확인 (kernel.bpf_stats_enabled=1 필요)
# per-IO ns ≈ Σ(run_time_ns) / 완료 IO 수
```

검증 절차: 프로파일러 (off / Phase0 / Phase1 / full) × 위 fio 워크로드를 돌려 (1) 완료 IOPS, (2) p99 latency, (3) `mpstat` CPU%, (4) `bpftool` IO당 ns를 비교해 §16.3 표를 교정한다.

---

## 17. SPDK 경로 계측 모드 (USDT/uprobe, 별도 트랙)

§3.5에서 본 대로 SPDK는 커널 I/O 스택을 우회하므로, lighter의 커널 eBPF 훅(§6)으로는 **한 건도 잡히지 않는다.** SPDK 대상을 프로파일하려면 **유저스페이스 프로브(USDT 또는 uprobe)** 를 SPDK 프로세스에 거는 별도 모드가 필요하다. 본 절은 그 설계를 정리한다. (관련 코드 분석: [[nvme-eg-fdp-kernel-vs-spdk]], `spdk-study/lib/nvme/`.)

### 17.1. 왜 별도 트랙인가

| | 커널 경로 (§6) | SPDK 경로 (이 절) |
|---|---|---|
| 프로브 종류 | tracepoint/kprobe (커널) | **USDT/uprobe** (유저스페이스 바이너리) |
| 발화 컨텍스트 | syscall·blk-mq·IRQ | SPDK reactor 스레드(폴링 루프) |
| 귀속 차원 | PID/TID | **reactor 스레드 / qpair** |
| 상관키 | `request*` → `(qid,cid)` | `nvme_request*`/`bdev_io*` → `(qpair,cid)` |
| stage | S1..S6 (6단계) | U_SUBMIT/U_DEVICE/U_CPL (3단계, §3.5) |
| 완료 검출 | IRQ 시각 | **폴링 시각**(완료 발생≠검출, 폴링 지연 포함) |

### 17.2. 프로브 후보

SPDK는 자체 trace 프레임워크(`SPDK_TPOINT`, `lib/trace`)와 USDT(`SPDK_DTRACE_PROBE`)를 제공한다. 둘 중 USDT가 외부 BPF로 붙이기 좋다. USDT 미빌드/미배치 시 **함수 심볼 uprobe** 로 대체한다.

| 단계 | uprobe 심볼(예) | 캡처 |
|------|----------------|------|
| U_SUBMIT | `spdk_nvme_ns_cmd_read/write` (`lib/nvme/nvme_ns_cmd.c`) 또는 `bdev_nvme_*` (`module/bdev/nvme/bdev_nvme.c`) 진입 | T0, qpair, opcode, lba, len, cb_arg(상관키) |
| U_DOORBELL | `nvme_pcie_qpair_ring_sq_doorbell` 류 | T1 (옵션) |
| U_CPL | `spdk_nvme_qpair_process_completions` 내부 완료 처리 / 완료 콜백 진입 | T2/T3, (qpair,cid) |
| FDP | `spdk_nvme_ns_cmd_io_mgmt_recv/send` (`nvme_ns_cmd.c:2661/2717`) | placement(dspec)·RUH 차원 |

> USDT가 가장 안정적(심볼·인라인 변화에 둔감)이나 SPDK 빌드 옵션에 의존. uprobe는 무빌드지만 **인라인·LTO·심볼 스트립**에 취약 → 빌드된 SPDK의 심볼 유무를 시작 시 점검하고 가능한 지점만 자동 선택한다.

### 17.3. 상관키·귀속

- 상관키: submit에서 `cb_arg`(또는 `bdev_io*`/`nvme_request*` 포인터)를 키로 `inflight_u[ctx]=T0` 저장 → 완료 콜백/process_completions에서 같은 포인터로 매칭. SPDK는 보통 단일 reactor가 한 qpair를 소유 → 락 불필요.
- 귀속: PID/TID 대신 **reactor(스레드 이름, `spdk_thread`)** 와 **qpair** 를 차원으로. `bpf_get_current_pid_tgid()` 로 reactor 스레드 TID는 그대로 얻되, "I/O 발행 thread = 완료 폴링 thread" 가 동일(폴링 모델)이라 귀속이 오히려 단순.

### 17.4. 한계·주의

1. **폴링 지연이 U_CPL에 포함**: 완료는 디바이스가 CQ에 쓴 순간이 아니라 reactor가 폴링으로 관측한 순간 측정 → U_DEVICE에 폴링 배치 지연이 섞인다. CQ phase 비트 시각을 직접 보려면 더 깊은 프로브 필요(정확도 vs 복잡도 트레이드오프).
2. **오버헤드**: uprobe는 커널 tracepoint보다 비싸다(유저→커널 트랩). 고 IOPS SPDK(코어당 수백만 IOPS)에서 per-I/O uprobe는 부담이 큼 → **USDT + 샘플링** 필수. (§16의 커널 추정보다 per-event 비용이 높다고 가정.)
3. **빌드 의존**: USDT 프로브 위치/심볼은 SPDK 버전에 따라 다름 → CO-RE 같은 자동 이식이 없으므로 **버전별 프로브 맵**을 둔다.
4. **TUI 통합**: 커널 트리(PID→TID→stage)와 SPDK 트리(reactor→qpair→U_stage)를 동일 TUI에서 별 섹션으로 표시. 데이터 모델(§7.2)은 차원만 교체해 재사용.

### 17.5. 로드맵 위치

본 트랙은 핵심(커널 경로) 이후의 **확장 Phase**다. Phase 5(경로 확장)에 이어 **Phase 7 — SPDK USDT 모드**로 둔다: ① 심볼/USDT 탐지 → ② U_SUBMIT/U_CPL 2점 latency(MVP) → ③ per-qpair·FDP placement 차원 → ④ TUI 통합.
