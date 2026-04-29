# parallelink Architecture

> **문서 목적**: parallelink 시스템의 전체 아키텍처를 사용자 관점부터 모듈 내부 동작까지 한 문서에서 추적 가능하게 정리한다. 본 문서는 `parallelink/CLAUDE.md`의 도메인 지식과 `parallelink/work.md`의 시행착오 로그와 짝을 이루는 **설계 기준 문서**다.
>
> **참조 프로젝트**:
> - `spdk` — userspace NVMe driver, CUSE 통합, lockless trace
> - `bam (libnvm)` — GPU-orchestrated NVMe (BaM 본체)
> - `nvme-cli` — 외부 사용자가 사용하는 표준 도구 (호환 대상)
> - `fio` — 외부 사용자가 사용하는 IO benchmark (호환 대상)
> - `poseidonos` — Samsung NVMe-oF target, SPDK wrapper / event scheduler / telemetry / cli 패턴
> - `air` — Analytics In Real-time, lockless circular buffer 기반 profiler
> - `trident` — POS 테스트 framework (multi-host orchestration)

---

## 0. 배경 / 목표

### 0.1 배경

기존 GPU-orchestrated NVMe 솔루션(BaM 등)은 **단일 프로세스가 GPU context, NVMe controller, IO queue, persistent kernel을 모두 보유**하는 구조다. 결과적으로:

- 벤치마크 한 번에 controller init + queue create + GPU kernel launch 비용 모두 발생 (cold start)
- nvme-cli/smartctl 같은 외부 운영 도구가 GPU가 점유한 SSD에 접근 불가
- 한 프로세스 종료 시 GPU/SSD 자원 모두 회수 — 다음 실행 시 재초기화

### 0.2 parallelink 목표

1. **자원 영속화**: NVMe controller(BAR, admin queue, IO queue pool), CUDA context, BaM controller object를 daemon 프로세스로 분리하여 외부 클라이언트(fio 등) 라이프사이클과 무관하게 유지.
2. **외부 도구 호환**: stock `nvme-cli`, `smartctl`, custom ioctl tool, `fio`(자체 ioengine)가 패치 없이 동작.
3. **GPU-direct IO 보존**: 데이터 IO는 무조건 GPU persistent kernel을 거쳐 PCIe P2P DMA 수행. 호스트 CPU 우회.
4. **단일 진입점**: 모든 외부 통신을 `/dev/plink*` (CUSE) 한 곳으로 통일. 별도 socket 운용 안 함.
5. **모듈 분리**: SPDK처럼 control plane / data plane / lifecycle 책임을 분명히 분리하여 향후 확장(추가 트랜스포트, 멀티 SSD)에 대응.

### 0.3 비목표

- **file system 가속 아님**: raw block device 접근 시나리오만 다룸. ublk/NBD/file system 노출은 도입 안 함.
- **VM 게스트 노출 아님**: vhost-user-blk 등 VM 통합 없음. host process만 client.
- **per-IO 단위 fio queue 의미 부여 안 함**: fio io_u는 워크로드 디스크립터 전달용 trigger로만 쓰임. 실제 IO는 GPU 자율 생성.

---

## 1. 외부 인터페이스 — 사용자 시점

### 1.1 외부 도구 호환성 매트릭스

| 도구 / 명령 | 채널 | 처리 결과 | 비고 |
|-------------|------|----------|------|
| `nvme list` | `/dev/plink0` ioctl | ✅ resource cache 응답 | stock nvme-cli |
| `nvme id-ctrl /dev/plink0` | `NVME_IOCTL_ADMIN_CMD` | ✅ admin worker → BaM admin | |
| `nvme id-ns /dev/plink0n1` | `NVME_IOCTL_ADMIN_CMD` | ✅ ns_table 캐시 + admin | |
| `nvme smart-log /dev/plink0` | `NVME_IOCTL_ADMIN_CMD` GET_LOG_PAGE | ✅ 실시간 SSD 조회 | |
| `nvme format /dev/plink0n1` | `NVME_IOCTL_ADMIN_CMD` FORMAT | ✅ admin (단, 진행 중 IO drain 후) | |
| `nvme fw-download/-commit` | `NVME_IOCTL_ADMIN_CMD` | ✅ admin | |
| `nvme reset /dev/plink0` | `NVME_IOCTL_RESET` | ✅ controller reset | |
| `nvme rescan /dev/plink0` | `NVME_IOCTL_RESCAN` | ✅ ns 재발견 | |
| `smartctl -a /dev/plink0` | ioctl | ✅ | |
| Custom ioctl tool (vendor opcode) | `NVME_IOCTL_ADMIN_CMD` | ✅ ADMIN passthrough | vendor-specific도 통과 |
| `fio --ioengine=parallelink` | `/dev/plink0n1` ioctl + shm_open | ✅ GPU 자율 IO | 자체 ioengine 필수 |
| `dd if=/dev/plink0n1` | `NVME_IOCTL_SUBMIT_IO` | ❌ ENOTSUP | data IO는 GPU 경유 강제 |
| `fio --ioengine=io_uring --filename=/dev/plink0n1` | (해당 없음) | ❌ | IO ioctl 거부됨 |
| `plink-stat`, `plink-top` (자체 도구) | `/dev/plink0` `PLINK_IOC_QUERY_*` | ✅ resource cache + GPU stats 합성 | parallelink 자체 도구 |

### 1.2 사용자 명령 흐름 예시

```bash
# 1) 시스템 부팅 — daemon은 systemd로 영속 시작
systemctl start plink-daemon

# 2) 일상 관리 — 기존 도구 그대로
nvme list                               # /dev/plink0n1 인식
nvme id-ctrl /dev/plink0                # SSD 정보
nvme smart-log /dev/plink0              # SMART
nvme format /dev/plink0n1 --lbaf=0      # 포맷 (in-flight drain)
nvme fw-download /dev/plink0 -f fw.bin  # 펌웨어 업데이트
smartctl -a /dev/plink0                 # 다른 도구

# 3) Custom ioctl tool — vendor-specific 명령
my-vendor-tool --device=/dev/plink0 --opcode=0xC0 ...

# 4) 성능 측정 — 자체 ioengine
fio --name=test --ioengine=parallelink \
    --filename=/dev/plink0n1 \
    --rw=randread --bs=4k --iodepth=128 --numjobs=4 \
    --runtime=60 --time_based

# 5) 모니터링 — 자체 도구
plink-stat                              # 현재 상태 단발 출력
plink-top                               # 실시간 ncurses dashboard
```

---

## 2. User View Sequence Diagram (Ladder)

### 2.1 시나리오 A — Admin 명령 (nvme-cli)

`nvme id-ctrl /dev/plink0` 또는 `nvme smart-log /dev/plink0`.

```
nvme-cli         CUSE module       daemon                BaM/admin worker      NVMe SSD
(user)           (kernel)          (CUSE thread)         (admin queue)         (HW)

   │ open(/dev/plink0)                                      │                  │
   │────────────────►│                                      │                  │
   │                 │ FUSE_OPEN ─────►│ session register   │                  │
   │◄────────fd──────│◄────────────────│                    │                  │
   │                 │                  │                    │                  │
   │ ioctl(NVME_IOCTL_ADMIN_CMD,        │                    │                  │
   │       Identify CNS=0x01)           │                    │                  │
   │────────────────►│ FUSE_IOCTL ────►│ decode admin cmd   │                  │
   │                 │                  │ enqueue io_msg     │                  │
   │                 │                  │───────────────────►│ build SQE       │
   │                 │                  │                    │ admin SQ doorbell│─►
   │                 │                  │                    │                  │ Identify
   │                 │                  │                    │                  │ DMA write 4KB
   │                 │                  │                    │ CQE arrival     │◄─
   │                 │                  │ admin worker cb   │◄───────────────────
   │                 │                  │ result copy +     │                    │
   │                 │◄─────────────────│ fuse_reply_ioctl  │                    │
   │◄────FUSE_REPLY──│                  │                    │                    │
   │ id-ctrl 출력    │                  │                    │                    │
   │ close(fd)       │                  │                    │                    │
```

**소요**: 단일 admin 명령당 1 round-trip, ms 수준 (admin queue + SSD 응답 시간).

### 2.2 시나리오 B — IO 워크로드 (fio)

`fio --ioengine=parallelink --filename=/dev/plink0n1 --rw=randread --bs=4k --iodepth=128 --runtime=60`.

```
fio                CUSE/daemon                  GPU kernel                 SSD
(user)             (CUSE + session mgr)         (persistent kernel,        (NVMe)
                                                 fio session 단위 launch)

  │ td_init() → plink_init()                                                  │
  │ open(/dev/plink0n1)                                                       │
  │──────────────────►│                                                       │
  │◄────fd────────────│                                                       │
  │ ioctl(PLINK_IOC_OPEN_SESSION)                                             │
  │──────────────────►│ session_table 등록                                    │
  │◄─session_id───────│                                                       │
  │                                                                            │
  │ ioctl(PLINK_IOC_START_WORKLOAD,                                           │
  │       {rw,bs,iodepth,nsid,runtime,...})                                   │
  │──────────────────►│ ① IO queue pool LEASE qids[N] (이미 영속 생성됨)     │
  │                   │ ② shm_open + mmap + cudaHostRegister                   │
  │                   │    (stats, control, workload_desc 영역)               │
  │                   │ ③ workload_desc cudaMemcpyHostToDevice                │
  │                   │ ④ ★ cudaLaunchKernel(plink_kernel,                    │
  │                   │      <<<n_blocks, threads>>>, stream_sid,             │
  │                   │      desc, qids, stats, ctrl)                         │
  │                   │──────────────────────────────►│ kernel start          │
  │◄─shm_path 회신────│                                │                       │
  │                                                    │                       │
  │ shm_open(shm_path) + mmap → stats, control         │                       │
  │                                                    │                       │
  │                                                    │ workload_desc fetch   │
  │                                                    │ → constant cache      │
  │                                                    │                       │
  │                                                    │┌─per-warp loop───┐   │
  │                                                    ││ curand→LBA      │   │
  │                                                    ││ CID alloc       │   │
  │                                                    ││ PRP build       │   │
  │ td_io_queue() → plink_queue() (no-op,              ││ SQE build       │   │
  │   FIO_Q_QUEUED)                                    ││ SQ enqueue      │   │
  │                                                    ││ doorbell write ─┼─►│ DMA + CQE
  │ td_io_getevents() → plink_getevents():             ││ CQ phase poll   │◄─┤
  │   read stats->io_completed                         ││ stats atomicAdd │   │
  │   delta = now - prev                               ││ → host pinned   │   │
  │   fake events 합성                                 ││ CID free        │   │
  │                                                    ││ check stop_req  │   │
  │ (fio main loop 반복)                               │└─────────────────┘   │
  │ ⋮                                                   │                       │
  │ (runtime 60s, 수백만 IO)                            │ ⋮                     │
  │ ⋮                                                   │                       │
  │                                                    │                       │
  │ td_cleanup() → plink_cleanup():                    │                       │
  │ control->stop_request = 1 (mmap write)             │                       │
  │                                                    │ stop_req 감지         │
  │                                                    │ (single-thread        │
  │                                                    │  + nanosleep poll)    │
  │                                                    │ drain inflight        │
  │                                                    │ ────────────────────◄┤
  │                                                    │ kernel exit           │
  │                                                                            │
  │ ioctl(PLINK_IOC_STOP_WORKLOAD)                                            │
  │──────────────────►│ cudaStreamSynchronize (kernel join)                   │
  │                   │ qids 반납 (LEASED→IDLE)                                │
  │                   │ shm_unlink, cudaFree                                   │
  │◄─final stats──────│                                                       │
  │ munmap, close(fd) / fio result 출력                                       │
```

**소요**:
- 시작: ioctl + cudaLaunchKernel (수십 ms)
- runtime: hot path 모두 GPU 자율, fio↔daemon 통신 0 (stats는 mmap 직접 read)
- 종료: stream sync + 정리 (수십 ms)

### 2.3 시나리오 C — 혼용 (fio 진행 중 nvme-cli)

```
[fio session 진행 중 — 위 2.2의 GPU kernel이 IO 자율 실행 중]
                                                                                
  ⋮                                       │                              │
  (kernel은 IO queue 1,2,3,4 사용 중)     │                              │
                                          │                              │ 
nvme smart-log /dev/plink0                │                              │
(다른 터미널, user B)                     │                              │
                                          │                              │
   ioctl(NVME_IOCTL_ADMIN_CMD,            │                              │
         GET_LOG_PAGE LID=0x02)           │                              │
   ─────────────────►│ CUSE thread        │                              │
                     │ admin worker로 enqueue                             │
                     │  io_msg ring (fio IO와 다른 채널)                  │
                     │  Get Log Page SQE 빌드                             │
                     │  ★ admin SQ에 enqueue (IO queue와 별도)            │─►│ SMART log
                     │  doorbell ring                                     │  │ 읽기
                     │  CQE 폴링 ◄────────────────────────────────────────│◄─│
                     │  result host buf 복사                              │
   ◄── ioctl 응답 (SMART data)            │                              │
   smart-log 출력                          │                              │
                                          │ fio IO 계속 진행 (방해 없음)  │
                                                                          │ io 계속...
```

**핵심**:
- Admin queue와 IO queue가 물리적으로 분리 → 간섭 없음
- daemon CUSE thread 하나가 admin/parallelink ioctl 모두 받지만 routing 분기:
  - NVMe 표준 ioctl → io_msg ring → admin worker → admin SQ
  - PLINK_IOC_* → session manager → cudaLaunchKernel
- GPU kernel은 IO queue만 사용, admin과 무관

---

## 3. 전체 아키텍처 Block Diagram

### 3.1 영속성 표기

- **[P]** = daemon 영속 (daemon process 시작 ~ 종료)
- **[S]** = fio 세션 단위 (fio start ~ fio cleanup)

### 3.2 전체 다이어그램

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                      외부 사용자 도구 (Stock, 무수정)                          ║
║                                                                              ║
║   ┌──────────┐  ┌──────────┐  ┌─────────────┐  ┌────────────────────────┐    ║
║   │ nvme-cli │  │ smartctl │  │ custom      │  │ fio                    │    ║
║   │          │  │          │  │ ioctl tool  │  │ --ioengine=parallelink │    ║
║   └────┬─────┘  └────┬─────┘  └─────┬───────┘  └──────────┬─────────────┘    ║
║        │             │              │                     │                  ║
║        │  ioctl(NVME_IOCTL_ADMIN_CMD/RESET/RESCAN/...)    │                  ║
║        │  open + ioctl + mmap (PLINK_IOC_*)               │                  ║
║        └─────────────┴──────────────┴─────────────────────┘                  ║
║                              ★ 모든 진입을 /dev/plink* 로 통일                ║
║                              (socket 없음, 표준 file API)                     ║
╚══════════════════════════════│═══════════════════════════════════════════════╝
                               ▼
              ┌─────────────────────────────────────────┐
              │        Linux Kernel — CUSE module       │
              │        /dev/plink0   (controller)  [P]  │
              │        /dev/plink0n1 (namespace)   [P]  │
              └─────────────────┬───────────────────────┘
                                │ FUSE protocol
╔═══════════════════════════════│══════════════════════════════════════════════╗
║                               ▼                                              ║
║                plink-daemon (single long-running process, systemd)           ║
║                                                                              ║
║  ┌───────────────────── ⑧ Control Path Module ─────────────────────────┐   ║
║  │ ┌──────────────────────────────────────────────────────────────┐    │   ║
║  │ │ CUSE thread (libfuse3 epoll loop)                       [P]  │    │   ║
║  │ │  ─ ioctl 디스패처:                                            │    │   ║
║  │ │    NVMe 표준: ADMIN_CMD/RESET/RESCAN/BLK*GET                 │    │   ║
║  │ │    parallelink 자체:                                          │    │   ║
║  │ │      PLINK_IOC_OPEN_SESSION                                   │    │   ║
║  │ │      PLINK_IOC_START_WORKLOAD → shm_path, qids[] 회신         │    │   ║
║  │ │      PLINK_IOC_STOP_WORKLOAD                                  │    │   ║
║  │ │      PLINK_IOC_QUERY_INVENTORY/SESSION/SMART/STATS            │    │   ║
║  │ │    거부: SUBMIT_IO/IO_CMD → ENOTSUP                           │    │   ║
║  │ └──────────────────────────────┬───────────────────────────────┘    │   ║
║  │                                ▼                                     │   ║
║  │ ┌──────────────────────────────────────────────────────────────┐    │   ║
║  │ │ io_msg ring (lock-free MPSC, SPDK nvme_io_msg.c 차용)   [P]  │    │   ║
║  │ │  producer: CUSE thread / session mgr                         │    │   ║
║  │ │  consumer: admin worker thread                               │    │   ║
║  │ └──────────────────────────────┬───────────────────────────────┘    │   ║
║  │                                ▼                                     │   ║
║  │ ┌──────────────────────────────────────────────────────────────┐    │   ║
║  │ │ session manager  (RPC handler가 호출)                   [S]  │    │   ║
║  │ │  START_WORKLOAD: qid LEASE / shm 생성 / cudaLaunchKernel     │    │   ║
║  │ │  STOP_WORKLOAD : ctrl→stop / streamSync / qid 반납 / shm 정리 │    │   ║
║  │ └──────────────────────────────────────────────────────────────┘    │   ║
║  └────────────────────────────────────────────────────────────────────┘    ║
║                                                                              ║
║  ┌──────────────── Admin / Worker Module ───────────────────────────────┐   ║
║  │ ┌──────────────────────────────────────────────────────────────┐    │   ║
║  │ │ admin worker thread (BaM admin queue 단독)               [P] │    │   ║
║  │ │  - Identify Ctrl/NS, Get/Set Features, Get Log Page          │    │   ║
║  │ │  - Format / FW Download / Sanitize                           │    │   ║
║  │ │  - AER 항상 1개 outstanding                                  │    │   ║
║  │ │  - boot 시 IO queue pool (Create IO SQ/CQ × N) 생성          │    │   ║
║  │ └──────────────────────────────────────────────────────────────┘    │   ║
║  │ ┌──────────────────────────────────────────────────────────────┐    │   ║
║  │ │ GPU launcher / supervisor thread                         [P] │    │   ║
║  │ │  - boot: cuInit / cudaSetDevice / BaM controller init        │    │   ║
║  │ │  - cudaError 감시, kernel crash 시 session 강제 정리         │    │   ║
║  │ │  - heartbeat 추적 (fio session liveness)                     │    │   ║
║  │ └──────────────────────────────────────────────────────────────┘    │   ║
║  └────────────────────────────────────────────────────────────────────┘    ║
║                                                                              ║
║  ┌──────────────── Resource / Cache Module ─────────────────────────────┐   ║
║  │  ┌────────────────────┬────────────────────┬─────────────────────┐  │   ║
║  │  │ ctrlr_info         │ ns_table[nsid]     │ feature_cache       │  │   ║
║  │  │ (Identify Ctrl)    │ size/lba_size/pi   │ num_queues, AEC     │  │   ║
║  │  ├────────────────────┼────────────────────┼─────────────────────┤  │   ║
║  │  │ io_queue_pool[qid] │ session_table[sid] │ log_page_cache      │  │   ║
║  │  │ {sq,cq,doorbell,   │ {wd, stats fd,     │ SMART, error log    │  │   ║
║  │  │  state}            │  ctrl fd, qids[]}  │                     │  │   ║
║  │  └────────────────────┴────────────────────┴─────────────────────┘  │   ║
║  └────────────────────────────────────────────────────────────────────┘    ║
║                                                                              ║
║  ┌──────────────── ⑦ Logger Module ─────────────────────────────────────┐   ║
║  │   - log level: ERR/WARN/INFO/DEBUG/TRACE                            │   ║
║  │   - 출력 sink: stderr / syslog / file rotation                       │   ║
║  │   - 컴포넌트별 분류 (cuse, admin, session, gpu, kernel, ...)         │   ║
║  │   - host: spdk_log 류 매크로                                         │   ║
║  │   - device: GPU log ring (atomic add → CPU drain)                    │   ║
║  │   - per-session log 채널 분리                                        │   ║
║  │   - AIR 차용 (lockless circular buffer + per-thread)                 │   ║
║  └────────────────────────────────────────────────────────────────────┘    ║
║                                                                              ║
║  ┌──────────────── ⑨ Monitoring Module ─────────────────────────────────┐   ║
║  │   - PLINK_IOC_QUERY_* ioctl handler                                 │   ║
║  │   - resource cache + GPU stats memory + SSD SMART 합성              │   ║
║  │   - per-session stats snapshot                                      │   ║
║  │   - plink-stat (단발 조회) / plink-top (실시간 ncurses)              │   ║
║  │   - (선택) prometheus exporter                                       │   ║
║  └────────────────────────────────────────────────────────────────────┘    ║
║                                                                              ║
║  ┌──── Shared memory (host-pinned, GPU mapped, session 단위 [S]) ──────┐   ║
║  │   ┌──────────────────┬────────────────────────────────────────┐    │   ║
║  │   │ stats[sid]       │ GPU atomic write → CPU(fio/monitor) read│    │   ║
║  │   │ control[sid]     │ CPU write stop_request → GPU read       │    │   ║
║  │   │ workload_desc    │ GPU constant cache (kernel 시작 시 fetch)│    │   ║
║  │   │  (GPU memory)    │                                         │    │   ║
║  │   │ log_ring[sid]    │ GPU producer → CPU drain (logger)       │    │   ║
║  │   └──────────────────┴────────────────────────────────────────┘    │   ║
║  │     ↑ shm_open path를 ioctl 응답으로 fio에 회신                      │   ║
║  │     ↑ fio: shm_open + mmap → 같은 메모리 zero-copy 접근              │   ║
║  └────────────────────────────────────────────────────────────────────┘    ║
╚══════════════════════════════│═══════════════════════════════════════════════╝
                               │ CUDA driver (cudaLaunchKernel)
                               ▼
╔══════════════════════════════════════════════════════════════════════════════╗
║                              GPU (CUDA device)                               ║
║                                                                              ║
║  ┌──────── plink_kernel (fio session 단위 launch) [S] ──────────────────┐   ║
║  │                                                                      │   ║
║  │  ┌──────────────────────────────────────────────────────────────┐   │   ║
║  │  │ Init: workload_desc fetch / qid 분배 / curand seed setup     │   │   ║
║  │  └──────────────────────────────────────────────────────────────┘   │   ║
║  │                                                                      │   ║
║  │  ┌──── ① Queue Update Manager (warp) ──────────────────────────┐    │   ║
║  │  │  - SQ tail / CQ head 원자적 진행                            │    │   ║
║  │  │  - 여러 warp 같은 큐 공유 시 atomicCAS / 티켓 lock          │    │   ║
║  │  │  - sq_tail wrap-around 처리                                 │    │   ║
║  │  │  - shadow doorbell 갱신 (옵션, NVMe 1.3+)                   │    │   ║
║  │  │  - doorbell write 시점/주기 제어 (batching 결정)            │    │   ║
║  │  └──────────────┬─────────────────────┬─────────────────────────┘    │   ║
║  │                 │                     │                              │   ║
║  │                 ▼                     ▼                              │   ║
║  │  ┌──── ② CID Manager (warp) ─┐  ┌── ③ PRP/SGL Builder (warp) ─┐    │   ║
║  │  │  - CID 할당 (큐당 free    │  │  - DMA buffer 주소 →         │    │   ║
║  │  │    bitmap, atomic find)   │  │    PRP1/PRP2 / PRP list      │    │   ║
║  │  │  - 발급된 CID → in-flight │  │  - 4KB page 경계 정렬        │    │   ║
║  │  │    tracker 매핑           │  │    (bs > page 시 list)       │    │   ║
║  │  │  - completion 시 CID 해제 │  │  - SGL 모드 분기 (옵션)      │    │   ║
║  │  │  - inflight depth 관리    │  │  - bounce buffer 회피         │    │   ║
║  │  │    (iodepth 제한)         │  │    (GPU memory P2P 우선)      │    │   ║
║  │  └──────────────┬────────────┘  └─────────────┬────────────────┘    │   ║
║  │                 │                              │                     │   ║
║  │                 └──────────────┬───────────────┘                     │   ║
║  │                                ▼                                     │   ║
║  │  ┌──── ④ Command Submitter (warp) ──────────────────────────────┐   │   ║
║  │  │  - SQE 64B 빌드:                                             │   │   ║
║  │  │      opcode (READ/WRITE/FLUSH/...)                           │   │   ║
║  │  │      cid (← CID Manager)                                     │   │   ║
║  │  │      nsid                                                    │   │   ║
║  │  │      prp1/prp2 (← PRP Builder)                               │   │   ║
║  │  │      slba (← curand or sequential)                           │   │   ║
║  │  │      nlb (← workload_desc.bs / ns->lba_size - 1)             │   │   ║
║  │  │      fua, prinfo, dsm 등 plain bits                          │   │   ║
║  │  │  - SQ[tail] = SQE  (GPU memory write)                        │   │   ║
║  │  │  - Queue Update Manager에게 tail++ 요청                      │   │   ║
║  │  │  - doorbell write → SSD BAR (★ PCIe P2P, host 우회)          │   │   ║
║  │  │  - submit_tsc 기록 (latency 측정용, GPU clock)               │   │   ║
║  │  └──────────────────────────────┬───────────────────────────────┘   │   ║
║  │                                 ▼                                    │   ║
║  │  ┌──── ⑤ Completion Checker (warp) ─────────────────────────────┐   │   ║
║  │  │  - CQ phase bit polling (★ GPU memory, PCIe traffic 0)       │   │   ║
║  │  │  - CQE 16B 파싱:                                             │   │   ║
║  │  │      cid 추출 → CID Manager에 release                        │   │   ║
║  │  │      sct/sc 검사 → 성공/에러 분기                            │   │   ║
║  │  │      sq_head_ptr 업데이트                                    │   │   ║
║  │  │  - latency = now_tsc - submit_tsc                            │   │   ║
║  │  │  - CQ head 진행 후 doorbell write                             │   │   ║
║  │  │  - 통계 emit:                                                │   │   ║
║  │  │      atomicAdd_system(&stats->io_completed, 1)               │   │   ║
║  │  │      atomicAdd_system(&stats->bytes_*, bs)                   │   │   ║
║  │  │      atomicAdd_system(&stats->lat_sum_ns, lat)               │   │   ║
║  │  │      atomicAdd_system(&stats->lat_buckets[bucket], 1)        │   │   ║
║  │  └──────────────────────────────┬───────────────────────────────┘   │   ║
║  │                                 ▼                                    │   ║
║  │  ┌──── ⑥ Workload Generator (warp) ─────────────────────────────┐   │   ║
║  │  │  - workload_desc 정책에 따라 next_lba 결정                   │   │   ║
║  │  │    RANDREAD: curand → lba_offset + (rand % lba_count)        │   │   ║
║  │  │    SEQ: per-warp counter += bs/lba_size                      │   │   ║
║  │  │    ZIPF: 캐시된 zipf table lookup                            │   │   ║
║  │  │  - rwmix (RANDRW) 비율 결정                                  │   │   ║
║  │  │  - 종료 조건 평가:                                           │   │   ║
║  │  │      control->stop_request (single-thread + nanosleep)       │   │   ║
║  │  │      runtime_ns 초과                                         │   │   ║
║  │  │      io_count_limit 도달                                     │   │   ║
║  │  └──────────────────────────────────────────────────────────────┘   │   ║
║  │                                                                      │   ║
║  │  ┌──── Drain & Exit ────────────────────────────────────────────┐   │   ║
║  │  │  - submit 중지, inflight 모두 완료까지 폴링                  │   │   ║
║  │  │  - 마지막 stats flush                                        │   │   ║
║  │  │  - logger 마지막 메시지 emit                                 │   │   ║
║  │  │  - kernel 자체 종료 → daemon이 cudaStreamSynchronize         │   │   ║
║  │  └──────────────────────────────────────────────────────────────┘   │   ║
║  └────────────────────────────────────────────────────────────────────┘    ║
║                                                                              ║
║  GPU memory                                                                  ║
║   - SQ ring × N    (BaM 관리)        - workload_desc 사본                    ║
║   - CQ ring × N    (BaM 관리)        - DMA buffer (read/write 데이터)        ║
║   - CID free bitmap × N              - log ring (device → host)              ║
║                                                                              ║
╚════════════════════════════════│═════════════════════════════════════════════╝
                                 │ PCIe P2P DMA (BaM/libnvm 직결)
                                 ▼
                  ┌──────────────────────────────────┐
                  │         NVMe SSD                 │
                  │   Admin queue                    │
                  │   IO queue pool ×N (영속)         │
                  │     ↑ GPU doorbell (BAR)          │
                  │     ↑ DMA read/write              │
                  └──────────────────────────────────┘
```

### 3.3 모듈 번호 정의

본 문서에서 사용하는 모듈 번호 (사용자가 우선 정의한 9개):

| # | 모듈 | 위치 |
|---|------|------|
| ① | Queue Update Manager | GPU kernel |
| ② | CID Manager | GPU kernel |
| ③ | PRP/SGL Builder | GPU kernel |
| ④ | Command Submitter | GPU kernel |
| ⑤ | Completion Checker | GPU kernel |
| ⑥ | Workload Generator | GPU kernel |
| ⑦ | Logger | daemon + GPU |
| ⑧ | Control Path (CUSE thread + io_msg + session mgr) | daemon |
| ⑨ | Monitoring (plink-stat / plink-top + ioctl handler) | daemon + 외부 도구 |

추가 보조 모듈 (daemon 내부):
- Admin / Worker Module — admin worker thread + GPU launcher/supervisor
- Resource / Cache Module — ctrlr_info / ns_table / io_queue_pool / session_table

---

## 4. 자원 라이프사이클 (영속화 경계)

### 4.1 자원별 owner와 영속성

| 자원 | 위치 | 영속성 | owner |
|------|------|-------|-------|
| daemon process | OS | systemd 시작~종료 | systemd |
| CUDA context | daemon process | [P] daemon 시작~종료 | daemon GPU launcher thread |
| BaM controller object (BAR mmap, admin queue) | daemon | [P] | admin worker thread |
| IO queue pool (SQ/CQ × N, doorbell) | daemon + SSD | [P] (boot 시 1회 Create IO SQ/CQ) | admin worker thread + io_queue_pool |
| ns 메타테이블 (Identify NS 캐시) | daemon | [P] (boot 시 채움) | resource cache module |
| CUSE device `/dev/plink*` | kernel + daemon | [P] | CUSE thread |
| Persistent kernel | GPU | **[S] cudaLaunchKernel ~ kernel exit** | session manager |
| workload_desc / stats / control 메모리 | host-pinned + GPU mapped | [S] | session manager |
| leased qids[] | session_table | [S] | session manager |
| CUDA stream | daemon | [S] (session당 1개) | session manager |

### 4.2 라이프사이클 다이어그램

```
┌──────────────────────────────────────────────────────────────┐
│  daemon boot                                                 │
│   ├─ CUDA context             [P] ──────────────────┐        │
│   ├─ BaM controller (BAR)     [P] ──────────────────┤        │
│   ├─ Admin queue              [P] ──────────────────┤        │
│   ├─ IO queue pool (×N)       [P] ──────────────────┤        │
│   ├─ ns_table 캐시            [P] ──────────────────┤        │
│   └─ CUSE device (/dev/plink) [P] ──────────────────┤        │
│                                                     │        │
│   fio_1 start                                       │        │
│    ├─ session_1               [S] ─┐                │        │
│    ├─ workload_desc (GPU)     [S] ─┤                │        │
│    ├─ stats/ctrl (host-pin)   [S] ─┤                │        │
│    ├─ qids [3,5,7] LEASE      [S] ─┤                │        │
│    └─ kernel_1 launch         [S] ─┤                │        │
│   fio_1 end                        │ <─ kernel exit │        │
│    └─ qids 반환, shm 정리          │                │        │
│                                                     │        │
│   fio_2 start  (kernel은 새로 launch)               │        │
│    └─ ...                                           │        │
│                                                     │        │
│  daemon shutdown                                    ▼        │
│   ├─ Admin Delete IO SQ/CQ × N                               │
│   ├─ BaM controller destroy                                  │
│   └─ CUDA context destroy                                    │
└──────────────────────────────────────────────────────────────┘
```

### 4.3 Linux nvme driver와의 대응

| Linux nvme driver | parallelink daemon | 영속 단위 |
|-------------------|-------------------|----------|
| `struct nvme_ctrl` | ctrlr_info + BaM controller | daemon 시작~종료 |
| `struct nvme_ns` | ns_table[nsid] | controller 라이프사이클 |
| Admin queue | BaM admin queue + admin worker thread | 영속 |
| IO queue (per-CPU) | io_queue_pool[qid] (LEASE 단위 사용) | pool은 영속, lease는 fio 세션 |
| `nvme_completion_poll_cb` | persistent kernel CQ polling | fio 세션 |
| AER (Async Event Request) | admin worker가 항상 1개 outstanding | 영속 |
| `/sys/block/nvmeXnY` | ns_table[].size_in_blocks, CUSE BLKGETSIZE 응답 | controller 라이프사이클 |

---

## 5. 모듈별 상세

### 5.1 ① Queue Update Manager (GPU kernel)

**책임**:
- `sq_tail` / `cq_head` 원자적 진행
- 여러 warp가 같은 큐 공유 시 동기화 (atomicCAS 기반 티켓 lock 또는 atomic exchange)
- `sq_tail` wrap-around 처리 (큐 깊이 mask)
- shadow doorbell 갱신 (NVMe 1.3+ 옵션)
- doorbell write 시점/주기 제어 — submit batching 또는 즉시 write 결정

**관련 BaM API**: `nvm_queue_t::sq_tail` 직접 manipulation, `nvm_queue_submit`은 사용 안 하고 자체 구현

**참조**:
- SPDK `lib/nvme/nvme_pcie_common.c::nvme_pcie_qpair_submit_tracker` — doorbell write 패턴
- BaM `include/queue.h` — host 측 큐 관리
- POS `src/io_submit_interface/` — submit interface 추상화

### 5.2 ② CID Manager (GPU kernel)

**책임**:
- CID 할당: 큐당 free bitmap (atomic find-and-set)
- 발급된 CID → in-flight tracker(submit_tsc 등) 매핑
- completion 시 CID 해제 (atomic clear)
- inflight depth 관리 (`workload_desc.iodepth` 제한)

**자료구조**:
```c
__device__ struct cid_pool {
    uint32_t free_bitmap[N_WORDS];   /* 큐 깊이만큼 비트 */
    struct {
        uint64_t submit_tsc;
        uint64_t lba;                 /* completion 후 stats용 */
        uint16_t bs;
    } tracker[QUEUE_DEPTH];
};
```

**참조**:
- SPDK `nvme_qpair.c::nvme_qpair_init` — CID pool 초기화 패턴
- BaM `nvm_aq.cu` — admin CID 관리 (작은 사례)

### 5.3 ③ PRP/SGL Builder (GPU kernel)

**책임**:
- DMA buffer 주소 → PRP1/PRP2 / PRP list
- 4KB page 경계 정렬 검증
- bs > page size 시 PRP list 빌드
- SGL 모드 분기 (옵션, NVMe 1.2+)
- bounce buffer 회피 (GPU memory P2P 우선)

**핵심 제약**:
- BaM의 P2P DMA는 GPU BAR1 메모리 또는 host pinned 메모리만 가능
- IOMMU 비활성 또는 GPU BAR가 IOVA로 해석되어야 함

**참조**:
- SPDK `nvme_pcie_common.c::nvme_pcie_prp_list_append` — PRP 빌드 표준
- BaM `include/buffer.h::nvm_buffer_t`, `device.cu::nvm_dma_map`

### 5.4 ④ Command Submitter (GPU kernel)

**책임**:
- SQE 64B 빌드 (opcode/cid/nsid/prp/slba/nlb/fua/prinfo/dsm)
- SQ[tail]에 SQE write (GPU memory write)
- Queue Update Manager에게 tail++ 요청
- doorbell write → SSD BAR (PCIe P2P, host 우회)
- submit_tsc 기록 (CID Manager의 tracker에)

**SQE 레이아웃**:
- Bytes 0-3: cdw0 (opcode | fuse | psdt | cid)
- Bytes 4-7: nsid
- Bytes 8-15: reserved or metadata pointer
- Bytes 16-23: PRP1
- Bytes 24-31: PRP2
- Bytes 32-39: cdw10-11 (slba lower/upper)
- Bytes 40-43: cdw12 (nlb 0-based)
- Bytes 44-63: cdw13-15 (FUA, PRINFO, DSM 등)

**참조**:
- NVMe Base Specification 1.x Section 6 (NVM Command Set)
- BaM `include/cmd.h::nvm_cmd_*` — host 측 SQE 빌더 (GPU에서 inline 차용)

### 5.5 ⑤ Completion Checker (GPU kernel)

**책임**:
- CQ phase bit polling (GPU memory만 access, PCIe traffic 0)
- CQE 16B 파싱: cid, sct, sc, sq_head_ptr, sq_id
- CID Manager에 release
- latency = `now_tsc - tracker[cid].submit_tsc`
- CQ head 진행 후 doorbell write (CQ doorbell)
- 통계 emit: atomicAdd_system to host-pinned stats

**CQE 레이아웃**:
- DW0: command-specific result
- DW1: reserved
- DW2: sq_head_ptr (16) | sq_id (16)
- DW3: cid (16) | phase_tag (1) | status (15: sct + sc + dnr + crd + m)

**참조**:
- SPDK `nvme_pcie_common.c::nvme_pcie_qpair_process_completions`
- BaM `nvm_aq.cu::nvm_aq_consume` (admin 케이스)

### 5.6 ⑥ Workload Generator (GPU kernel)

**책임**:
- workload_desc 정책에 따라 next LBA 결정
  - RANDREAD/RANDWRITE: curand → `lba_offset + (rand % lba_count)`
  - SEQ: per-warp counter += `bs / lba_size`
  - ZIPF: pre-computed zipf table lookup
  - RANDRW: rwmix 비율로 read/write 결정
- 종료 조건 평가:
  - control->stop_request (single-thread + `__nanosleep` backoff)
  - elapsed_ns > workload_desc.runtime_ns
  - io_completed >= workload_desc.io_count_limit

**참조**:
- fio 옵션 매핑: `td->o.td_ddir`, `td->o.bs`, `td->o.iodepth`, `td->o.rwmix`, `td->o.random_distribution`, `td->o.rand_seed`

### 5.7 ⑦ Logger (daemon + GPU)

**Host side (daemon)**:
- log level: ERR / WARN / INFO / DEBUG / TRACE
- 출력 sink: stderr / syslog / file rotation
- 컴포넌트별 분류 태그 (cuse, admin, session, gpu, kernel, ...)
- API: `PLINK_LOG(level, component, fmt, args...)` 매크로

**Device side (GPU kernel)**:
- printf 대안 — fmt string은 보낼 수 없으므로 `fmt_id + args`로 인코딩
- per-session circular log ring (host-pinned)
- GPU producer (atomic head++), CPU drain thread
- format render는 host에서

**자료구조**:
```c
struct gpu_log_entry {
    uint64_t tsc;
    uint8_t  level;
    uint16_t fmt_id;     /* 사전 등록된 fmt string ID */
    uint64_t args[4];
};

struct gpu_log_ring {
    _Atomic uint64_t head;        /* GPU writer */
    _Atomic uint64_t tail;        /* CPU reader */
    uint64_t         seq[N_ENTRIES];
    struct gpu_log_entry entries[N_ENTRIES];
};
```

**참조**:
- **AIR 전체** (`air/src/`) — lockless circular buffer + per-thread + sampling policy + telemetry 출력
- POS `src/logger/` — host 측 로그 매크로 + 컴포넌트별 분류
- POS `src/trace/` + AIR 통합 — device → host 데이터 전달
- SPDK `lib/log/` + `lib/trace/` — 비교용

### 5.8 ⑧ Control Path (CUSE + io_msg + session mgr)

**구성**:
1. **CUSE thread** (libfuse3 epoll loop)
   - `/dev/plink0` 등록, 모든 ioctl 받기
   - NVMe 표준 ioctl → io_msg ring → admin worker
   - parallelink ioctl → session manager 직접 호출
2. **io_msg ring** (lock-free MPSC)
   - producer: CUSE thread, session manager
   - consumer: admin worker thread
   - SPDK `lib/nvme/nvme_io_msg.c` 코드 차용
3. **session manager**
   - `START_WORKLOAD`: qid LEASE → shm 생성 → workload_desc cudaMemcpy → cudaLaunchKernel
   - `STOP_WORKLOAD`: ctrl->stop=1 → cudaStreamSynchronize → qid 반납 → shm 정리
   - heartbeat 추적 (fio 죽었을 때 강제 정리)

**참조**:
- SPDK `lib/nvme/nvme_cuse.c` — CUSE ioctl 디스패치 패턴 (이미 한국어 주석 완비)
- SPDK `lib/nvme/nvme_io_msg.c` — lock-free 메시지 채널
- POS `src/cli/` + `src/network/` — 외부 인터페이스 (gRPC 비교용, parallelink는 CUSE 채택)
- POS `src/event_scheduler/` — lock-free event 처리 패턴

### 5.9 ⑨ Monitoring (plink-stat + plink-top + ioctl handler)

**daemon 측 (ioctl handler)**:
- `PLINK_IOC_QUERY_INVENTORY`: 전체 스냅샷 (controller + queue pool + sessions)
- `PLINK_IOC_QUERY_SESSION`: 특정 sid 상세
- `PLINK_IOC_QUERY_QUEUE_POOL`: 큐 상태
- `PLINK_IOC_QUERY_SMART`: SSD SMART (캐시)

**외부 도구**:
- **`plink-stat`** — 단발 조회, `nvidia-smi` 1회 출력 스타일
- **`plink-top`** — 실시간 ncurses dashboard (1초 polling)
- **(선택) prometheus exporter** — HTTP `/metrics` endpoint

**데이터 출처**:
- daemon resource cache (ctrlr/ns/qpool/session)
- GPU stats memory (host-pinned에 atomic update된 값들)
- SSD SMART (admin queue 캐시)
- GPU 일반 정보 (NVML library — 외부 도구 측에서 별도 query)

**참조**:
- POS `src/telemetry/` — 통계 수집/노출 인프라
- Trident `lib/prometheus.py` — Prometheus exporter 패턴
- Trident `lib/profiler.py` — 프로파일링 데이터 수집

---

## 6. 데이터 흐름 / 메모리 레이아웃

### 6.1 메모리 위치 결정 표

| 자원 | 위치 | GPU access 빈도 | PCIe 영향 |
|------|------|---------------|----------|
| workload_desc | GPU constant memory | 시작 시 1회 fetch | 0 (최초만) |
| stats (write) | host-pinned, GPU mapped | warp atomic, write only | 1 write/IO (PCIe write 효율 좋음) |
| stats (CPU read) | 동일 영역 | CPU만 | 0 |
| control->stop_request | host-pinned, GPU mapped | low rate (1ms+ backoff) | ~1 KB/s |
| doorbell | SSD BAR | every IO submit/complete | P2P (host 우회) |
| CQ ring | GPU memory | every poll | 0 (GPU local) |
| SQ ring | GPU memory | every IO write | 0 (GPU local) |
| log_ring | host-pinned | low rate (debug 시만) | 무시 가능 |
| DMA buffer (read/write) | GPU memory | every IO | P2P (host 우회) |

### 6.2 PCIe 대역폭 보호 원칙

GPU PCIe Gen4 x16 = 32 GB/s 단방향. SSD P2P + host memory access 모두 공유. 잘못된 polling이 SSD bandwidth 잠식 가능.

**원칙**:
1. 핫패스(SQE/CQE/doorbell)는 GPU memory와 SSD BAR P2P만 사용
2. host-pinned polling은 single-thread + `__nanosleep(1000)` 이상 backoff
3. stats는 GPU → host-pinned `atomicAdd_system`(write — read보다 효율 좋음)
4. workload_desc는 시작 시 1회 fetch 후 GPU memory에 캐시

**잘못된 패턴 vs 올바른 패턴**:
```
❌ BAD:
  while (*host_flag != 1) { /* spin */ }    // 수백 MB/s PCIe traffic

✅ GOOD:
  if (threadIdx.x == 0) {
      s_flag = atomicLoad_system(host_flag);
      __nanosleep(1000);                     // 1μs backoff
  }
  __syncthreads();
  if (s_flag) break;                         // < 50 MB/s
```

---

## 7. CUSE Interface 정의

### 7.1 ioctl number 정의

```c
/* /dev/plink* magic byte = 'P' (0x50) */
#define PLINK_IOC_MAGIC 'P'

/* parallelink 자체 ioctl */
#define PLINK_IOC_OPEN_SESSION       _IOWR(PLINK_IOC_MAGIC, 0x10, struct plink_session_open)
#define PLINK_IOC_START_WORKLOAD     _IOWR(PLINK_IOC_MAGIC, 0x11, struct plink_workload_start)
#define PLINK_IOC_STOP_WORKLOAD      _IOW (PLINK_IOC_MAGIC, 0x12, struct plink_workload_stop)
#define PLINK_IOC_QUERY_INVENTORY    _IOR (PLINK_IOC_MAGIC, 0x20, struct plink_inventory)
#define PLINK_IOC_QUERY_SESSION      _IOWR(PLINK_IOC_MAGIC, 0x21, struct plink_session_stat)
#define PLINK_IOC_QUERY_QUEUE_POOL   _IOR (PLINK_IOC_MAGIC, 0x22, struct plink_queue_pool_stat)
#define PLINK_IOC_QUERY_SMART        _IOR (PLINK_IOC_MAGIC, 0x23, struct plink_smart_snapshot)
#define PLINK_IOC_GET_TRANSPORT      _IOR (PLINK_IOC_MAGIC, 0x30, struct plink_transport_id)

/* NVMe 표준 ioctl은 그대로 (linux/nvme_ioctl.h) */
/* NVME_IOCTL_ADMIN_CMD, NVME_IOCTL_RESET, NVME_IOCTL_RESCAN, ... */
```

### 7.2 핵심 자료구조

```c
struct plink_workload_desc {
    /* 패턴 */
    uint8_t  rw;                    /* 0=READ 1=WRITE 2=RANDREAD 3=RANDWRITE 4=RANDRW */
    uint32_t bs;                    /* block size */
    uint8_t  rwmixread;             /* RANDRW 시 read % */
    uint8_t  fua;
    uint8_t  use_data_pattern;

    /* LBA range */
    uint32_t nsid;
    uint64_t lba_offset;
    uint64_t lba_count;

    /* 동시성 */
    uint32_t iodepth;
    uint32_t n_queues;
    uint32_t warps_per_queue;

    /* 종료 조건 */
    uint64_t runtime_ns;            /* 0 = unlimited */
    uint64_t io_count_limit;        /* 0 = unlimited */

    /* 재현성 */
    uint64_t random_seed;
    uint8_t  norandommap;
};

struct plink_workload_start {
    uint32_t                      session_id;     /* OPEN 시 발급된 값 */
    struct plink_workload_desc    desc;
    /* 응답 */
    char                          stats_shm_path[64];
    char                          control_shm_path[64];
    uint16_t                      qids[16];        /* leased queue IDs */
    uint16_t                      qid_count;
};

struct plink_stats {  /* shm_open + mmap, host-pinned, GPU atomic write */
    /* 누적 */
    _Atomic uint64_t io_completed;
    _Atomic uint64_t io_errors;
    _Atomic uint64_t bytes_read;
    _Atomic uint64_t bytes_written;

    /* latency */
    _Atomic uint64_t lat_sum_ns;
    _Atomic uint64_t lat_min_ns;
    _Atomic uint64_t lat_max_ns;
    _Atomic uint64_t lat_buckets[64];   /* log scale */

    /* 진행 */
    _Atomic uint64_t io_inflight;
    _Atomic uint64_t start_tsc;
    _Atomic uint64_t last_update_tsc;
    _Atomic uint8_t  state;             /* IDLE/RUNNING/STOPPING/DONE/ERROR */
};

struct plink_control {
    _Atomic uint8_t stop_request;
};
```

---

## 8. 외부 프로젝트 참조 매핑

### 8.1 모듈별 차용 매핑

| parallelink 모듈 | 차용 대상 | 핵심 자산 |
|----------------|---------|---------|
| ⑧ Control Path - CUSE thread | SPDK `lib/nvme/nvme_cuse.c` | libfuse3 ioctl 디스패치, BLKGETSIZE 등 호환 |
| ⑧ Control Path - io_msg ring | SPDK `lib/nvme/nvme_io_msg.c` | lock-free MPSC ring (CUSE → admin worker) |
| Admin Worker | SPDK `lib/nvme/nvme_ctrlr.c::nvme_ctrlr_process_init` | controller bring-up 상태머신, BaM wrapper에 동일 패턴 |
| Admin Worker | POS `src/spdk_wrapper/` | SPDK 위에 wrapper layer 만든 사례 (BaM wrapper 만들 때 참조) |
| Resource Cache | POS `src/master_context/`, `src/resource_manager/` | singleton 영속 자원 관리 |
| ① Queue Update Manager | SPDK `lib/nvme/nvme_pcie_common.c` | doorbell ring 패턴, shadow doorbell |
| ② CID Manager | SPDK `lib/nvme/nvme_qpair.c::nvme_qpair_init` | CID pool 초기화 |
| ② CID Manager | BaM `nvm_aq.cu` | GPU 측 admin CID 관리 (작은 사례) |
| ③ PRP/SGL Builder | SPDK `nvme_pcie_common.c::nvme_pcie_prp_list_append` | PRP 빌드 표준 |
| ③ PRP/SGL Builder | BaM `include/buffer.h::nvm_buffer_t` | GPU DMA buffer 매핑 |
| ④ Command Submitter | NVMe Spec Section 6 | SQE 비트필드 정의 |
| ④ Command Submitter | BaM `include/cmd.h::nvm_cmd_*` | host 측 SQE 빌더 (GPU에서 inline 차용) |
| ⑤ Completion Checker | SPDK `nvme_pcie_common.c::nvme_pcie_qpair_process_completions` | CQ phase bit polling 패턴 |
| ⑥ Workload Generator | fio ioengine 인터페이스 | option mapping (rw/bs/iodepth/random_distribution) |
| ⑦ Logger | **AIR 전체 (`air/src/`)** | lockless circular buffer + per-thread + sampling + telemetry |
| ⑦ Logger | POS `src/logger/`, `src/trace/` | 컴포넌트별 분류, AIR 통합 사례 |
| ⑨ Monitoring | POS `src/telemetry/` | 통계 수집/노출 인프라 |
| ⑨ Monitoring | Trident `lib/prometheus.py`, `lib/profiler.py` | Prometheus exporter, 프로파일러 |
| 테스트 framework | Trident `composable/`, `pos.py` | 시나리오 기반 테스트, multi-host |
| 테스트 framework | POS `src/wbt/` (white box test) | 디버깅 ioctl 패턴 |

### 8.2 차용 우선순위

**Phase 1 (구현 시작 시 즉시 필요)**:
- SPDK `nvme_cuse.c` → ⑧ CUSE thread 코드 베이스
- SPDK `nvme_io_msg.c` → ⑧ ring 그대로 차용
- BaM `nvm_aq.cu`, `include/queue.h` → BaM admin queue wrapper 참고

**Phase 2 (GPU kernel 코드 작성 시)**:
- SPDK `nvme_pcie_common.c` → ① ~ ⑤ 모듈의 표준 패턴
- BaM `include/cmd.h`, `include/buffer.h` → GPU 측 SQE/PRP 빌드

**Phase 3 (성숙도 향상 시)**:
- AIR 전체 → ⑦ Logger 정식 도입
- POS `src/telemetry/`, Trident → ⑨ Monitoring 강화
- POS `src/spdk_wrapper/`, `src/event_scheduler/` → 코드 품질 개선

---

## 9. 다음 구현 단계

### 9.1 권장 순서

1. **Resource cache 자료구조 정의** (`daemon/src/resource.c`)
   - ctrlr_info, ns_table, io_queue_pool, session_table 헤더
   - 다른 모듈이 모두 참조하므로 자료구조 먼저 확정
2. **CUSE skeleton + ioctl number 정의** (`daemon/src/cuse.c`, `include/plink_ioctl.h`)
   - 표준 ADMIN_CMD/RESET/RESCAN 디스패처만 먼저
   - parallelink 자체 ioctl은 stub (-ENOSYS)
3. **Admin worker + io_msg ring** (`daemon/src/admin.c`, `daemon/src/io_msg.c`)
   - SPDK `nvme_io_msg.c` 코드 차용
   - BaM admin queue wrapper
4. **Session manager + workload slot 메모리 setup** (`daemon/src/session.c`)
   - shm_open + cudaHostRegister
   - cudaLaunchKernel stub (실제 kernel은 다음 단계)
5. **GPU kernel 골격** (`kernel/plink_kernel.cu`)
   - workload_desc fetch + dummy IO loop (실제 SQE 빌드 없이)
6. **GPU 모듈 ④+①+②+③+⑤ 한 묶음** — 강결합이라 같이 작업
7. **⑥ Workload Generator** — 정책 다양화 (curand random, sequential, zipf)
8. **fio ioengine** (`engine/parallelink.c`) — init/queue/getevents/cleanup + RPC client
9. **⑦ Logger + ⑨ Monitoring** — AIR 통합, plink-stat/plink-top
10. **Trident 차용 테스트 시나리오** — composable scenario, CI 통합

### 9.2 각 단계 work.md 기록 규칙

`parallelink/CLAUDE.md`의 시행착오 로그 규칙 그대로:
- 작업 시작 시 엔트리 먼저 만들고 시도마다 실시간 추가
- 실패/롤백/삽질 모두 기록
- 에러 메시지 원문 그대로 코드 블록에
- 결과만 적기 금지

---

## 10. 부록

### 10.1 용어

| 용어 | 의미 |
|------|------|
| **fio external ioengine** | fio가 dlopen으로 로드하는 .so 플러그인 |
| **persistent kernel** | cudaLaunchKernel 후 join 안 하고 GPU에 상주하는 kernel |
| **CUDA stream** | GPU 명령 큐 (단일 process 안에서 여러 개 가능) |
| **PCIe P2P DMA** | GPU↔SSD 직접 데이터 전송 (host memory 우회) |
| **CUSE** | Character device in USErspace (FUSE 변형) |
| **CID** | Command Identifier (NVMe SQE의 16-bit cmd ID) |
| **SQ / CQ** | Submission Queue / Completion Queue |
| **SQE / CQE** | Submission/Completion Queue Entry (64B / 16B) |
| **PRP / SGL** | Physical Region Page / Scatter-Gather List (DMA 주소 표현) |
| **doorbell** | SSD BAR의 MMIO write로 SQ tail / CQ head 통지 |
| **AER** | Async Event Request (NVMe device-initiated event) |
| **NQN** | NVMe Qualified Name (Fabrics 식별자) |
| **BaM / libnvm** | 유저스페이스 NVMe driver (extern/bam) |
| **shadow doorbell** | NVMe 1.3+ MMIO 비용 절감 (host pinned에 doorbell mirror) |
| **LEASE / IDLE** | parallelink IO queue pool 상태 |
| **session_id (sid)** | parallelink 세션 식별자 (fio 인스턴스마다 발급) |

### 10.2 약어

- **POS**: PoseidonOS
- **AIR**: Analytics In Real-time
- **SPDK**: Storage Performance Development Kit
- **BaM**: Big accelerator Memory (libnvm 기반)
- **NVMe-oF**: NVMe over Fabrics
- **CUDA**: Compute Unified Device Architecture
- **CUSE**: Character device in USErspace
- **FUSE**: Filesystem in USErspace
- **MPSC**: Multi-Producer Single-Consumer (ring buffer 패턴)
- **SIMT**: Single Instruction Multiple Threads (CUDA 실행 모델)
- **TLP**: Transaction Layer Packet (PCIe)
- **MMIO**: Memory-Mapped I/O

### 10.3 관련 문서

- `parallelink/CLAUDE.md` — 도메인 지식 + 주석 작업 규칙
- `parallelink/work.md` — 시행착오 로그 (구현 진행)
- `parallelink/docs/design.md` — 초기 설계 (본 문서로 대체 또는 통합)
- `bam-study/CLAUDE.md` — BaM 아키텍처 참고
- `fio-study/CLAUDE.md` — fio ioengine 모델 참고
- `spdk-study/lib/nvme/nvme_cuse.c` — CUSE 차용 소스 (한국어 주석 완비)
- `spdk-study/lib/nvme/nvme_io_msg.c` — io_msg ring 차용 소스 (한국어 주석 완비)

---

**작성일**: 2026-04-29
**상태**: 초안 — parallelink 코드 작업 진행하면서 갱신
**다음 갱신 예정**: 모듈별 구현 시작 시 각 모듈 섹션을 코드 위치로 보강
