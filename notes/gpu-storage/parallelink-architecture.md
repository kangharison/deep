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

#### 5.1.1 동시성 모델 — SQ는 MPSC, CQ는 SPMC

NVMe 큐는 양 끝에서 *비대칭* 동시성 패턴을 갖는다:

```
   Producer 측                          Consumer 측
   ────────────                          ────────────
   GPU warps (여러 개)  ─→  SQ  ─→     NVMe HW (1개)        ← MPSC
   NVMe HW (1개)        ─→  CQ  ─→     GPU warps (여러 개)  ← SPMC
```

단순한 `atomicAdd`만으로 SQ tail을 진행하면 **doorbell이 미완성 SQE를 publish 할 위험**이 발생한다 — producer A가 slot 5 예약 후 SQE write를 끝내기 전에 producer B가 slot 6 예약·write·doorbell을 먼저 끝내면, NVMe가 slot 5의 쓰레기 SQE를 읽어간다.

#### 5.1.2 두 가지 구현 옵션

##### 옵션 A — Direct MPSC (rte_ring 식 이중 카운터)

```c
__device__ struct queue_state {
    _Atomic uint32_t sq_reserve_tail;   /* 예약 tail (atomicAdd 점) */
    _Atomic uint32_t sq_publish_tail;   /* publish tail (doorbell 대상) */
    /* publish_tail ≤ reserve_tail 불변, doorbell은 publish_tail만 가능 */
};

/* Enqueue 4단계 */
uint32_t ticket = atomicAdd(&q->sq_reserve_tail, 1);    /* 예약 */
SQ[ticket % depth] = build_sqe(...);                    /* SQE write */
__threadfence();
while (atomicLoad(&q->sq_publish_tail) != ticket) { }   /* 자기 차례 spin */
atomicStore(&q->sq_publish_tail, ticket + 1);           /* publish */
if (last_in_batch) *q->sq_db_bar = ticket + 1;          /* doorbell */
```

##### 옵션 B — vSQ + Submitter (io_uring SQPOLL 패턴)

MPSC 자체를 *회피*: producer 별 전용 vSQ(SPSC), 단일 submitter가 모든 vSQ를 sweep하여 real SQ로 옮김.

```c
__device__ struct vsq {              /* per-warp 전용 ring */
    _Atomic uint32_t head;           /* submitter consumer */
    _Atomic uint32_t tail;           /* producer */
    struct nvme_sqe entries[VSQ_DEPTH];
};
__device__ struct vsq* g_vsqs[MAX_WARPS];

/* producer (worker warp) — SPSC push, atomic 1회만 */
__device__ void plink_submit(uint32_t warp_id, struct nvme_sqe sqe) {
    struct vsq* v = g_vsqs[warp_id];
    uint32_t t = v->tail;                              /* relaxed load */
    while (t - v->head >= VSQ_DEPTH) /* 가득 차면 spin */;
    v->entries[t % VSQ_DEPTH] = sqe;
    __threadfence();
    atomicStore_release(&v->tail, t + 1);              /* publish to submitter */
}

/* submitter — 단일 thread가 real SQ에 SPSC write */
__device__ void submitter_loop(...) {
    uint32_t lt = real_sq_tail;
    for (;;) {
        bool any = false;
        for (int w = 0; w < num_warps; w++) {
            uint32_t h = vsqs[w].head, t = atomicLoad(&vsqs[w].tail);
            while (h != t) {
                real_sq[lt % depth] = vsqs[w].entries[h % VSQ_DEPTH];
                lt++; h++; any = true;
            }
            atomicStore_release(&vsqs[w].head, h);
        }
        if (any) {
            __threadfence_system();
            real_sq_tail = lt;
            *sq_db_bar = lt % depth;                   /* doorbell 1회/sweep */
        }
    }
}
```

**효과**: 모든 채널이 SPSC가 되어 atomic CAS·ABA·doorbell ordering 문제가 사라지고, doorbell이 sweep 단위로 자연 batching된다.

#### 5.1.3 CPU 마이크로벤치마크 (`/tmp/sq_bench.cpp`)

AMD Ryzen 5 5600G (4 cores), iters=20K/producer, SQ depth 8K, vSQ depth 64.

##### 시나리오 ① UNIFORM (모든 producer 빈틈없이 enqueue)

```
prods   Direct(MOPS)   vSQ(MOPS)   vSQ/Direct
──────────────────────────────────────────────
1       170.6          50.9        0.30×
2        34.0         205.9        6.06×
4        12.6         135.6       10.73×
6 *       0.01        171.1     13822×       ← Direct catastrophic collapse
```

*6 producers는 별도 측정 — Direct가 corecount 초과 시 lock-step starvation으로 50초 소요.*

##### 시나리오 ② SKEWED (1/4 heavy + 3/4 light, 8:1)

```
prods   Direct(MOPS)   vSQ(MOPS)   vSQ/Direct
──────────────────────────────────────────────
2       25.3           84.3        3.34×
4       39.1           68.7        1.76×
```

heavy producer 가 적어 contention 일부 완화 → 격차 작음. 그래도 vSQ 우위 유지.

##### 시나리오 ③ BURST (32 enqueue 후 짧은 idle)

```
prods   Direct(MOPS)   vSQ(MOPS)   vSQ/Direct
──────────────────────────────────────────────
2       13.7           16.8        1.23×
4       18.0           17.4        0.97×
```

burst 사이 idle이 throughput을 dominate → 차이 작음. 다만 producer-perceived latency는 vSQ가 fire-and-forget이라 응답성 우위 (throughput 측정에 안 잡힘).

#### 5.1.4 핵심 발견

```
┌────────────────────────────────────────────────────────────────────┐
│ 1. Direct MPSC는 producers > cores 시 *catastrophic collapse*      │
│    예: 1→2→4→6 producers : 170→34→12→0.01 MOPS                     │
│    원인: predecessor publish 대기 spin이 lock-step starvation       │
│    GPU 등가: warp 수가 SM의 atomic 처리율 한계 넘으면 동일 패턴      │
│                                                                    │
│ 2. vSQ+Submitter는 producer 수에 *둔감*                            │
│    51 → 206 → 136 → 171 MOPS (변동 ±50%, 평균 plateau)             │
│    SPSC 채널만 존재 → contention 자체가 없음                        │
│                                                                    │
│ 3. 단일 producer에선 vSQ가 *손해*                                   │
│    Direct 170 vs vSQ 51 MOPS                                       │
│    submitter copy step의 indirection 비용                           │
│                                                                    │
│ 4. burst·QoS 워크로드는 throughput보다 latency 측면에서 vSQ 우위     │
└────────────────────────────────────────────────────────────────────┘
```

#### 5.1.5 parallelink 적용 정책

```
┌────────────────────────────────────────────────────────────────┐
│ workload_desc 설정              | 권장 패턴                        │
├────────────────────────────────────────────────────────────────┤
│ warps_per_queue == 1           | Direct (vSQ submitter overhead 회피)│
│   (per-warp 큐, contention 0)   |                                  │
├────────────────────────────────────────────────────────────────┤
│ warps_per_queue 2~4            | Direct OK, contention 측정 권장   │
├────────────────────────────────────────────────────────────────┤
│ warps_per_queue ≥ 8            | vSQ + Submitter ★                 │
│   (큐당 다수 warp 공유)          |                                   │
├────────────────────────────────────────────────────────────────┤
│ QoS · priority · rate limit 필요 | vSQ — submitter가 정책 일관 적용 │
├────────────────────────────────────────────────────────────────┤
│ debug · per-warp tracing 필요   | vSQ — submitter가 자연 통합 지점  │
└────────────────────────────────────────────────────────────────┘
```

##### parallelink kernel 내 배치

`plink_kernel` 안에서 **block 0의 warp 0를 submitter로 박는다**:
- 별도 kernel launch 불필요 (overhead 0)
- nvme_state·queue_state·g_vsqs 가 같은 kernel context
- supervisor (§5.1 GPU launcher) 가 cudaError 감시 시 자연스럽게 같이 잡힘

```cuda
__global__ void plink_kernel(...) {
    if (blockIdx.x == 0 && warp_id == 0) {
        submitter_loop(...);            /* 영속 — kernel 종료까지 sweep */
        return;
    }
    /* 그 외 warps = worker = vSQ producer */
    worker_loop(my_warp_id, ...);
}
```

#### 5.1.6 vSQ depth 사이즈

```
   vSQ depth = 32~64 권장
   
   너무 작으면 (≤ 16): producer가 자주 stall (submitter 따라잡기 전 가득 참)
   너무 크면 (≥ 256) : per-warp 메모리 비용 ↑, sweep 1회당 spike 처리량 변동 ↑
   
   메모리 비용:
     vSQ entry = 64 B (NVMe SQE)
     1024 warps × 64 entries × 64 B = 4 MiB (무시 가능)
```

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

#### 5.3.1 DMA Buffer Pool Allocator 설계

③ PRP/SGL Builder가 사용할 **DMA-able buffer slot 발급기**는 parallelink의 *direct=1 워크로드 특성*과 *GPU SIMT 동시성*에 맞춰 BaM 기본 구조를 변형한다. 풀 자체는 init 시 cudaMalloc + nvidia_p2p_get_pages + IOMMU 등록을 한 번에 끝내고 *변하지 않는다* — 런타임에 동적으로 결정되는 것은 *어느 슬롯을 점유할지*(인덱스) 뿐이다. Linux 드라이버의 `dma_alloc_coherent`가 *주소*를 반환하는 인터페이스를 parallelink는 *인덱스* 인터페이스로 평행 이동시킨 형태다.

##### 5.3.1.1 두 후보 비교

```
┌─[A] BaM 본체 방식: CLOCK hand + take_lock atomicCAS]───────────────┐
│   목적: LBA→slot 캐시 (read 재사용 시 hit)                           │
│   동작: page_ticket fetch_add → slot CAS → refcount 검사 → eviction │
│   atomic 수: alloc 1건당 2~3회 (+ retry)                            │
│   부수 비용: page_states / page_addresses / range_id 매핑 갱신       │
│   적합: cache 의미가 있는 워크로드 (재접근 LBA 분포)                   │
└────────────────────────────────────────────────────────────────────┘

┌─[B] free-stack + per-thread Magazine 방식 (parallelink 채택)────────┐
│   목적: 익명 buffer 풀 — direct=1 처럼 재사용 의미가 없는 워크로드      │
│   동작: 스레드 로컬 캐시에서 pop → 비면 글로벌 stack에서 BATCH 충전     │
│   atomic 수: BATCH(=32) 회당 1회                                    │
│   부수 비용: per-warp shared memory cache 만 추가                     │
│   적합: parallelink fio 워크로드 (direct=1, alloc-on-submit)         │
└────────────────────────────────────────────────────────────────────┘
```

direct=1 워크로드에서는 cache hit이 본질적으로 0%이므로, A의 *eviction · LBA 매핑 · refcount* 인프라는 100% 잉여 비용이다. parallelink는 B를 채택하되, BaM의 page_cache 구조 자체(cudaMalloc 영역, prp1[]/prp2[] 사전 계산 배열)는 *그대로 재사용*하고 **발급 메커니즘만 free-stack + Magazine 으로 교체**한다.

##### 5.3.1.2 자료구조

```c
/* per-class buffer pool — class 별로 인스턴스화 */
struct buf_pool_d {
    /* BaM page_cache에서 그대로 차용 — init 시 한 번 채워짐 */
    void*       cache_pages_base;   /* cudaMalloc 영역 시작 */
    uint64_t    page_size;          /* class 사이즈 (4K/8K/16K/128K) */
    uint64_t*   prp1;               /* per-slot IOVA */
    uint64_t*   prp2;               /* per-slot PRP list IOVA */
    uint32_t    n_slots;

    /* free-stack 발급기 — parallelink 추가분 */
    int*        free_indices;       /* 비어있는 슬롯 인덱스 LIFO */
    _Atomic uint32_t free_top;
};

/* per-thread (또는 per-warp shared) Magazine 캐시 */
struct buf_local_cache {
    int      items[64];             /* LIFO 슬롯 인덱스 */
    int      count;
    struct buf_pool_d* pool;
};

/* Multi-class wrapper — bs 다양성 흡수 */
struct buf_multi_d {
    struct buf_pool_d* pools[NUM_CLASSES];   /* 4K/8K/16K/128K */
    /* pick_class(bs)는 비트 연산 한 번 — log4(bs/4K) ⌈⌉ */
};

/* 사용자 핸들 — 호출자에게 노출되는 형태 */
struct buf_handle {
    uint8_t   cls;
    uint32_t  slot_idx;
    uint64_t  prp1;     /* pools[cls]->prp1[slot_idx] 그대로 복사 */
    uint64_t  prp2;
    void*     dev_ptr;  /* cache_pages_base + slot_idx * page_size */
};
```

##### 5.3.1.3 alloc / free 핫패스 (Magazine 패턴)

```c
__device__ struct buf_handle plink_buf_alloc(struct buf_multi_d* mp,
                                              struct buf_local_cache* lc,
                                              uint32_t bs)
{
    int cls = pick_class(bs);                       /* 비트 연산 1회 */
    struct buf_local_cache* c = &lc[cls];

    if (c->count == 0) {                            /* 캐시 빔 → 글로벌 refill */
        struct buf_pool_d* p = mp->pools[cls];
        int top = atomicSub(&p->free_top, BATCH);   /* 글로벌 atomic 1회 */
        if (top < BATCH) {
            atomicAdd(&p->free_top, BATCH);         /* 롤백 */
            return (struct buf_handle){.cls = 0xFF};/* 풀 고갈 */
        }
        for (int i = 0; i < BATCH; i++)
            c->items[i] = p->free_indices[top - 1 - i];
        c->count = BATCH;
    }

    int idx = c->items[--c->count];                 /* LIFO pop, atomic 0회 */
    struct buf_pool_d* p = mp->pools[cls];
    return (struct buf_handle){
        .cls      = cls,
        .slot_idx = idx,
        .prp1     = p->prp1[idx],                   /* 정적 배열 인덱싱 */
        .prp2     = p->prp2[idx],
        .dev_ptr  = (char*)p->cache_pages_base + (uint64_t)idx * p->page_size,
    };
}

__device__ void plink_buf_free(struct buf_multi_d* mp,
                                struct buf_local_cache* lc,
                                struct buf_handle h)
{
    struct buf_local_cache* c = &lc[h.cls];
    if (c->count >= 64) {                           /* 캐시 가득 → 글로벌 flush */
        struct buf_pool_d* p = mp->pools[h.cls];
        int pos = atomicAdd(&p->free_top, BATCH);   /* 글로벌 atomic 1회 */
        for (int i = 0; i < BATCH; i++)
            p->free_indices[pos + i] = c->items[c->count - BATCH + i];
        c->count -= BATCH;
    }
    c->items[c->count++] = h.slot_idx;              /* LIFO push, atomic 0회 */
}
```

핵심: **글로벌 atomic은 BATCH(32) 회의 alloc/free당 1회씩**. 핫패스의 atomic 비용이 32× 분산됨.

##### 5.3.1.4 Multi-class 사이즈 클래스

`bs={4K, 8K, 16K, 128K}` (parallelink 워크로드 가정)에 대해 **4-class geometric pool**을 운영한다:

```
   class[0]  4 KB    pool_4K   (free-stack + Magazine)
   class[1]  8 KB    pool_8K
   class[2] 16 KB    pool_16K
   class[3]128 KB    pool_128K

   pick_class(bs):
     bs ≤  4K → 0
     bs ≤  8K → 1
     bs ≤ 16K → 2
     else    → 3
```

장점:
- 각 class 내부에서 슬롯이 *물리적으로 연속* → PRP entry 1~2개로 끝 (PRP list page 불필요)
- I/O 한 건 = 슬롯 한 개 → atomic op 분산이 class 단위로도 일어남
- 사이즈가 bs와 정확히 일치하면 내부 단편화 0; round-up이 발생해도 *최악 75%, 평균 ~30%*

##### 5.3.1.5 CPU 측정 — 성능 벤치마크 (단일 bs)

`/tmp/bam_bench2.cpp` (AMD Ryzen 5 5600G, g++ -O2, 4 cores, std::atomic을 GPU atomic 1:1 모사):

```
bs=128KB (slots/IO=32), iodepth=1024 :
threads   ClockHand(ms)   FreeStack-naive(ms)   Magazine(ms)
─────────────────────────────────────────────────────────────
1         0.26            0.17                  0.07
8         5.28            8.37                  0.27
32        21.34           33.45                 1.28
64        42.49           65.73                 1.69       ← 25× 빨라짐
```

**관찰**:
- naive free-stack 만으로는 single hot cache line(`free_top`) 때문에 multi-thread 시 clock hand보다 *오히려 느려짐* (1.2~1.6×)
- Magazine 패턴(per-thread cache + BATCH refill)을 얹어야 비로소 압도적 우위
- "atomic op 횟수 적음" ≠ "빠름" — 진짜 변수는 *hot cache line 분산 정도* 와 *atomic amortization*

##### 5.3.1.6 CPU 측정 — Mixed bs (direct=1)

`/tmp/bam_bench3.cpp`, bs round-robin {4, 8, 16, 128}KB:

```
threads   ClockSingle(ms)   MagSingle(ms)   MagMulti(ms)
──────────────────────────────────────────────────────────
8         1.68              0.19            0.18
32        6.77              0.74            0.71
64       13.46              1.26            1.12       ← Multi가 Clock의 12×
```

**관찰**:
- ClockSingle(현 BaM 그대로 4K 풀에 N슬롯 individual alloc): atomic ops가 ~25K/thread → 병목
- MagSingle (4K 단일 풀 + bulk_get + Magazine): 10× 빠름
- MagMulti (4-class Magazine): 추가 ~10% + PRP 간소화 + class별 contention 분산
- direct=1 에서 hit/eviction 비용은 100% 잉여 → cache 의미를 버리는 것이 명백한 win

##### 5.3.1.7 풀 사이징 가이드

direct=1 + iodepth × numjobs 큰 워크로드의 peak in-flight:

```
   peak slots = numjobs × iodepth × 1.5 (헤더 50%)
   
   예) numjobs=64, iodepth=1024 → 98,304 slots / class
   
   class별 메모리 (균등 분포 가정):
     pool_4K   : 24,576 × 4KB    =  96 MiB
     pool_8K   : 24,576 × 8KB    = 192 MiB
     pool_16K  : 24,576 × 16KB   = 384 MiB
     pool_128K : 24,576 × 128KB  = 3072 MiB
     ────────────────────────────────────────
     총                            ≈ 3.7 GiB
```

워크로드 분포가 작은 bs 위주면 큰 class 풀을 줄여 메모리 절약. 분포 어긋날 시를 대비해 **위쪽 class spill** 메커니즘(요청보다 큰 class에서 발급)을 추가한다.

##### 5.3.1.8 BaM 본체와의 공존 정책

parallelink는 BaM의 page_cache 영역을 **두 영역으로 분할**해 동시에 운영한다:

```
   ┌─[영역 A] BaM 캐시 의미 슬롯 (있다면)─┐
   │   slot[0..M-1]                       │
   │   clock hand + take_lock + LBA 매핑   │
   │   read-mostly + 재사용 LBA 분포 워크로드 │
   └────────────────────────────────────────┘

   ┌─[영역 B] parallelink 익명 풀 (기본)─┐
   │   slot[M..N-1]                      │
   │   free-stack + per-thread Magazine  │
   │   direct=1 / alloc-on-submit         │
   │   range_id = ANONYMOUS 표시          │
   │   영역 A의 clock hand가 evict 후보에서 제외 │
   └─────────────────────────────────────────┘
```

cache_pages[]·prp1[]·prp2[] 배열은 공유. 발급 메커니즘만 영역별로 분리해 두 의미론을 동시에 지원.

**참조 (allocator 학술/산업 출처)**:
- Bonwick (1994) "The Slab Allocator", USENIX Summer
- Bonwick & Adams (2001) "Magazines and Vmem", USENIX
- Berger et al. (2000) "Hoard: A Scalable Memory Allocator", ASPLOS-IX
- Ghemawat (2007) Google "TCMalloc: Thread-Caching Malloc"
- Corbató (1968) "A Paging Experiment with the Multics System" (CLOCK 알고리즘 원전)
- Qureshi et al. (2023) "BaM: GPU-Initiated On-Demand High-Throughput Storage Access", ASPLOS

##### 5.3.1.9 Pool 고갈 처리 — Spill / Buddy / Lazy Carve

워크로드 분포가 사이징과 어긋나면 한 class 풀이 비고 다른 class 는 남는다. 세 가지 처리 방식:

```
┌─[A] One-way upward spill (§6.3.4 의 단순 안전장치)──────────────┐
│   class[k] 비면 class[k+1] 에서 1슬롯 사용 (메모리 일부 낭비)     │
│   장점: 추가 atomic 0회, 코드 거의 그대로                         │
│   단점: 8KB 요청에 16KB 슬롯 — 50% 낭비, 역방향 불가             │
└──────────────────────────────────────────────────────────────────┘

┌─[B] 진짜 Buddy allocator (split/merge 양방향)──────────────────┐
│   class[k] 비면 class[k+1] 1슬롯 split                           │
│   class[k] 둘 다 free + buddy면 coalesce → class[k+1]            │
│   장점: 메모리 100% 활용                                          │
│   단점: GPU lock-free split/merge 매우 어려움                     │
│         cascade 시 multi-step atomic → 사실상 spinlock 필요        │
│         ABA 위험, 구현 라인 수 ~500+                              │
└──────────────────────────────────────────────────────────────────┘

┌─[C] Lazy carve (★ 권장 절충)─────────────────────────────────┐
│   하나의 backing region 에 4KB 단위 IOVA 사전 등록                │
│   class별 free-stack 은 *시작 인덱스* 만 저장                       │
│   고갈 시 큰 class 에서 1 슬롯 빌려와 N 개 작은 인덱스로 분할 push  │
│   coalesce 안 함 — 단편화 누적 → fio 세션 종료 시 reset 으로 청소   │
│   장점: B의 90% 이득, A 정도의 구현 복잡도                        │
│   단점: 장기 실행 시 단편화 누적 (compactor 별도 필요)              │
└──────────────────────────────────────────────────────────────────┘
```

##### 5.3.1.10 Lazy Carve 자료구조 + 코드

핵심: 모든 class가 *같은 backing memory* 위의 *다른 인덱싱 스킴*. "128KB slot s" = 4KB 페이지 [s, s+31] 32개. 4KB 단위 IOVA 테이블만 있으면 어느 class 든 PRP 빌드 가능.

```c
struct buf_pool_unified_d {
    void*     backing_base;             /* 단일 cudaMalloc */
    uint64_t* prp1_4k;                  /* 4KB 단위 IOVA 테이블 (전역) */
    uint32_t  n_pages_4k;

    /* class별 free-stack — 시작 4KB 인덱스만 보관 */
    struct {
        _Atomic uint32_t top;
        uint32_t*       indices;
        uint32_t        capacity;
        uint32_t        unit_4k;        /* 1=4K, 2=8K, 4=16K, 32=128K */
    } cls[NUM_CLASSES];

    int carve_donor[NUM_CLASSES];       /* 어느 class 에서 빌릴지 (보통 가장 큰) */
};

__device__ uint32_t plink_alloc_with_carve(struct buf_pool_unified_d* p, int cls)
{
    uint32_t idx = stack_pop(&p->cls[cls]);
    if (idx != INVALID) return idx;                /* 정상 경로 */

    /* 고갈 — donor class 에서 1 슬롯 carve */
    int donor = p->carve_donor[cls];
    uint32_t big = stack_pop(&p->cls[donor]);
    if (big == INVALID) return INVALID;            /* 진짜 고갈 */

    uint32_t units = p->cls[donor].unit_4k / p->cls[cls].unit_4k;  /* e.g., 32/2=16 */
    uint32_t step  = p->cls[cls].unit_4k;
    for (uint32_t i = 1; i < units; i++)
        stack_push(&p->cls[cls], big + i * step);  /* 나머지 N-1 개 push */
    return big;                                    /* 첫 번째는 그대로 사용 */
}

__device__ void plink_free_no_coalesce(struct buf_pool_unified_d* p,
                                       int cls, uint32_t idx) {
    stack_push(&p->cls[cls], idx);                  /* 원래 class 로 단순 push */
    /* coalesce 시도 안 함 — atomic 0회 추가, 단순함 유지 */
}
```

##### 5.3.1.11 단편화 처리 정책

```
   parallelink 워크로드 특성: fio 세션 단위로 시작·종료 명확
   
   Phase 1 (단편화 없음): 첫 fio 세션 — 모든 class 가 init 분배 그대로
   Phase 2 (carve 발생): 워크로드 분포가 어긋난 class에서 lazy carve 발생
   Phase 3 (단편화 누적): 큰 class 가 점차 작은 조각들의 묶음으로 변함
   
   대응: fio 세션 종료 시 (PLINK_IOC_STOP_WORKLOAD)
         → 모든 class free-stack reset
         → init 분배로 재초기화
         → 다음 세션은 Phase 1 부터 시작
   
   장기 daemon (세션이 끊임없이 이어짐) 의 경우:
         → CPU background compactor 스레드 (idle 시점에 합병)
         → GPU lock-free buddy 보다 안전하고 단순
```

##### 5.3.1.12 carve 비용 분석

```
   정상 alloc:        atomic 1회 (BATCH refill 시만, Magazine 효과)
   carve 이벤트:      donor pop 1회 + N-1 push 회 = atomic N회
   
   carve 빈도:
     - 워크로드 분포 정확하면 거의 0
     - 분포 어긋남 시 한 번 발생 후 *N-1 회는 정상 path*
     - 평균적으로 (1 + N-1)/N = 1 atomic per slot — Magazine 과 동급
```

##### 5.3.1.13 단계적 적용 권장

```
   Phase 1 (MVP):       [A] one-way upward spill
                        → 워크로드 프로파일링 → 풀 분배 결정
                        → spill rate 카운터 노출

   Phase 2 (운영):      [C] lazy carve
                        → backing region 단일화
                        → carve event count / class 별 모니터링
                        → fio 세션 종료 시 reset 정책

   Phase 3 (장기 daemon): [C] + CPU background compactor
                          → idle 감지 시 free-stack 정리
                          → GPU 측 lock-free buddy 는 검토만
```

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

#### 5.7.1 Host 측 라이브러리 선택지

핫패스 오버헤드를 GPU 수준으로 맞추려면 *deferred formatting + lock-free async* 가 필수다. parallelink CPU 측 로깅은 다음 중 하나를 채택한다:

```
┌──────────────────────────────────────────────────────────────────┐
│ 라이브러리      | per-call 오버헤드  | 특징                          │
├──────────────────────────────────────────────────────────────────┤
│ NanoLog       | ~7 ns            | Binary 로그, 후처리 디코딩       │
│  (Stanford)   |                   | format string pointer만 저장   │
├──────────────────────────────────────────────────────────────────┤
│ Quill         | ~8 ns            | Async, lock-free SPSC ring     │
│               |                   | 헤더-only 가능                  │
├──────────────────────────────────────────────────────────────────┤
│ fmtlog        | ~10 ns           | fmt::format 기반                │
│               |                   | 컴파일 타임 포맷 검증            │
├──────────────────────────────────────────────────────────────────┤
│ spdlog (async)| ~100~200 ns      | 사실상 표준, 풍부한 sink         │
└──────────────────────────────────────────────────────────────────┘
```

저오버헤드의 핵심 패턴 3가지:
- **Deferred formatting** — 핫패스에선 raw 인자만 저장, 포맷팅은 별도 스레드
- **Lock-free per-thread ring buffer** — 스레드 간 contention 0
- **Binary log format** — 디스크 I/O 최소화, 디코딩은 별도 프로세스

parallelink는 NanoLog 또는 Quill을 우선 검토하되, AIR(`air/src/`) 의 lockless circular buffer 패턴을 차용해 host/device 통합 logger를 구성한다.

#### 5.7.2 Device-side Logger 구현 상세

GPU printf의 한계 (NVIDIA 공식 명시):
- per-call ~수천~10K 사이클 (핫패스 timing 왜곡)
- device-side circular buffer (기본 1 MiB) — 넘치면 *조용히 손실*
- 모든 출력은 kernel 완료 또는 `cudaDeviceSynchronize()` 시점에 flush
- 워프 내 thread별 출력이 직렬화 (divergent printf)

→ **디버깅 일회성에만 사용**, 성능/이벤트 로깅에는 부적합.

##### 두 영역 비교 — 콘텐츠를 어디에 둘 것인가

```
┌─[host-mapped pinned (cudaHostAllocMapped)]─────────────────────────┐
│ gpu_log() 한 번:                                                   │
│   atomicAdd → PCIe write (host counter)         ~수백 ns           │
│   memory write 4회 → PCIe writes                ~PCIe BW 제한       │
│   __threadfence_system()                        ~수백 cycle         │
│   ─ 총: ~300~500 ns/엔트리                                         │
│ 장점: CPU 즉시 가시 (tail -f 가능)                                   │
│ 단점: PCIe BW (32 GB/s)가 로깅 throughput 천장                       │
└────────────────────────────────────────────────────────────────────┘

┌─[device-local + CPU pull (parallelink 채택)]───────────────────────┐
│ gpu_log() 한 번:                                                   │
│   atomicAdd → GPU L2/HBM (local)                ~30~80 cycle        │
│   memory write 4회 → coalesced (local)          ~수십 cycle         │
│   __threadfence() (device only)                 ~수 cycle           │
│   ─ 총: ~50~150 ns/엔트리 (3~5× 빠름)                               │
│ 장점: aggregate write BW가 GPU HBM 수준                              │
│ 단점: CPU 가시화는 cudaMemcpy 시점까지 지연 (post-mortem 측정에 무관) │
└────────────────────────────────────────────────────────────────────┘
```

벤치마크가 주 용도이므로 **device-local 채택**. 콘텐츠는 GPU 메모리에, 회수는 CPU pull 스레드가 cudaMemcpy로 batch 처리.

##### 자료구조 (5.7의 `gpu_log_ring` 보강)

```c
struct plink_log_entry {       /* 32 bytes, 캐시라인 1/2 */
    uint64_t ts_ns;            /* GPU globaltimer (ns) */
    uint32_t tid;              /* global thread id */
    uint16_t fmt_id;           /* 사전 등록 fmt string ID */
    uint16_t event;            /* SUBMIT/COMPLETE/ALLOC/EVICT/... */
    uint64_t arg0;             /* lba, prp, slot_idx 등 */
    uint64_t arg1;
};

/* GPU 측 글로벌 — cudaMalloc */
__device__ struct plink_log_entry* g_log_buf;
__device__ uint32_t*               g_log_top;       /* atomic counter */
constexpr uint32_t LOG_CAP_LOG2  = 24;              /* 16 M entries = 512 MiB */
constexpr uint32_t LOG_MASK      = (1u << LOG_CAP_LOG2) - 1;
```

##### 핫패스 — `printk()` 일대일 대응

```cuda
__device__ inline void plink_log(uint16_t event, uint64_t a, uint64_t b) {
    uint32_t idx = atomicAdd(g_log_top, 1u);
    struct plink_log_entry& e = g_log_buf[idx & LOG_MASK];
    e.ts_ns = gpu_ns();              /* globaltimer (아래 5.7.3) */
    e.tid   = threadIdx.x + blockIdx.x * blockDim.x;
    e.fmt_id = 0;
    e.event = event;
    e.arg0  = a;
    e.arg1  = b;
    /* threadfence 불필요 — 같은 device 내부에서 끝 */
}

/* 사용 — Linux driver의 printk() 한 줄 등가 */
plink_log(EVT_SUBMIT, lba, prp);
plink_log(EVT_COMPLETE, lba, latency_ns);
```

##### Per-warp shared memory 캐시 (선택 — 추가 최적화)

글로벌 atomic을 워프 단위로 amortize:

```cuda
__shared__ struct plink_log_entry warp_buf[16];
__shared__ uint32_t warp_top;

/* 워프 내부 ++ 인덱싱, 가득 차면 lane 0이 글로벌 atomicAdd로 batch flush */
```

→ 글로벌 atomic이 16~32 thread당 1회로 amortize. ③ buffer pool과 동일한 Magazine 트릭.

##### CPU pull 스레드 (dmesg/journald 대응)

```c
void* plink_log_puller(void*) {
    cudaStream_t stream;
    cudaStreamCreate(&stream);
    uint32_t local_head = 0;
    FILE* out = fopen("/var/log/plink_gpu.log", "w");

    while (running) {
        uint32_t cur;
        cudaMemcpyAsync(&cur, d_log_top, 4,
                        cudaMemcpyDeviceToHost, stream);
        cudaStreamSynchronize(stream);

        if (cur > local_head) {
            uint32_t n = cur - local_head;
            /* ring wrap 처리 후 cudaMemcpyAsync로 batch 회수 */
            /* 호스트 측에서 fmt_id → 등록된 fmt string 으로 render */
            local_head = cur;
        } else {
            usleep(1000);   /* 1 ms backoff */
        }
    }
    fclose(out);
    return NULL;
}
```

Pull 주기 결정:
- **kernel 끝에 한 번만**: 핫패스 오버헤드 0, 측정값 왜곡 0 — *벤치마크 권장*
- **1 ms 폴링**: 거의 실시간, plink-top과 호환
- **100 μs 폴링**: 진짜 실시간 dmesg 수준 — cudaMemcpy 빈도 ↑

##### Hybrid (실시간성 필요 시): 카운터만 host-mapped

```
   콘텐츠 = device-local
   카운터 = host-mapped pinned
   
   gpu_log():
     idx = atomicAdd(g_log_top_dev, 1)         /* GPU 로컬, 빠름 */
     g_log_buf[idx] = ...                      /* GPU 로컬 */
     if ((idx & 1023) == 0)
         *h_log_top_mapped = idx;              /* PCIe 1024-entry당 1회 */
   
   CPU 리더:
     cur = *h_log_top_mapped;                  /* 그냥 메모리 read */
     if (cur > local_head) cudaMemcpyAsync(...)
```

PCIe traffic을 1024× 줄이면서 실시간성 유지.

#### 5.7.3 CPU/GPU Clock Synchronization

GPU `clock64()` 의 함정:
- SM별 카운터, 다른 SM 간 직접 비교 불가
- DVFS로 frequency 변동 → ns 환산 오차
- kernel launch 시 임의값에서 시작

**parallelink는 무조건 `globaltimer` 사용** — PTX `mov.u64 %0, %%globaltimer;`로 읽는 GPU 전역 단조 ns 카운터:

```cuda
__device__ inline uint64_t gpu_ns() {
    uint64_t t;
    asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(t));
    return t;   /* ns 단위, DVFS 무관, 모든 SM 공통 */
}
```

##### Affine fit 으로 host clock과 alignment

원리: `T_host = a × T_gpu + b`. 두 점에서 `(T_host, T_gpu)` 동시 샘플 → `a, b` 계산. 기울기 `a` 는 거의 1.0 (둘 다 ns), `b` 만 구해도 sub-microsecond 가능.

##### Ping-pong cross-stamping (NTP 방식 차용)

```c
int64_t plink_calibrate_clock(uint64_t* d_gpu_t) {
    int64_t best_offset = 0;
    int64_t min_rtt = INT64_MAX;
    for (int i = 0; i < 100; i++) {
        struct timespec t1, t2;
        uint64_t gpu_t;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        sync_kernel<<<1,1>>>(d_gpu_t);
        cudaMemcpy(&gpu_t, d_gpu_t, 8, cudaMemcpyDeviceToHost);
        clock_gettime(CLOCK_MONOTONIC, &t2);

        int64_t h1 = t1.tv_sec*1000000000LL + t1.tv_nsec;
        int64_t h2 = t2.tv_sec*1000000000LL + t2.tv_nsec;
        int64_t rtt = h2 - h1;
        if (rtt < min_rtt) {              /* 최소 RTT의 점이 가장 정확 */
            min_rtt = rtt;
            best_offset = (h1 + h2)/2 - (int64_t)gpu_t;
        }
    }
    return best_offset;
}
```

100회 중 *최소 RTT* 의 점을 채택 → noise 제거. 일반 PCIe 시스템에서 **±200 ns 이내** 가능.

##### Drift 보정 (긴 측정)

GPU globaltimer ↔ host CLOCK_MONOTONIC 은 별개 oscillator. ~1 ppm 수준 drift 누적:
- 1 초 측정 → ~1 μs 오차
- 100 초 측정 → ~100 μs 오차

→ **측정 시작·끝(필요 시 중간)에 calibrate 두 번**. 두 점 affine fit으로 slope 계산. 긴 측정엔 piecewise linear.

##### parallelink 측정 워크플로

```
1. plink-bench start:
     sync_t0 = plink_calibrate_clock()
2. fio 워크로드 실행 (kernel 안에서 plink_log() 호출, ts_ns = gpu_ns())
3. plink-bench end:
     sync_t1 = plink_calibrate_clock()
     slope, offset 계산 (두 점 affine)
4. 후처리:
     모든 GPU 엔트리 ts_ns를 host time 으로 변환
     CPU 로그(NanoLog 등)와 merge sort → 단일 timeline
5. 분석:
     SUBMIT (GPU) → COMPLETE (GPU) latency
     SUBMIT (GPU) ↔ NVMe doorbell write (CPU 측 trace) 정합성 검증
```

##### 참고: Nsight Systems 내부도 동일 원리

CUPTI activity callback이 kernel start/end 시점을 GPU clock으로 받고, NVPM_GET_TIMESTAMP 로 globaltimer 와 host clock 동시 샘플. parallelink가 손으로 하는 것과 같은 affine 변환을 사용해 timeline 표시.

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

### 6.3 Buffer Pool 메모리 사이징

§5.3.1 의 Multi-class Magazine 풀이 워크로드 *peak in-flight*를 stall 없이 흡수하려면 다음 공식을 사용한다.

```
   peak_slots(class c) = numjobs × iodepth × P(c) × headroom

   numjobs  : fio 동시 잡 수
   iodepth  : 잡당 outstanding I/O 깊이
   P(c)     : 워크로드 분포에서 class c 비율 (워크로드 프로파일링으로 측정)
   headroom : 1.25 ~ 1.5 (eviction·jitter 흡수, direct=1 도 burst 보호 필요)
```

#### 6.3.1 예시 — `bs={4K,8K,16K,128K}` 균등, numjobs=64, iodepth=1024

```
   per-class slot 수: 64 × 1024 × 0.25 × 1.5 ≈ 24,576 slots
   
   ┌─────────┬────────────┬────────────┬─────────────┐
   │ class    │ slot_size  │ slots       │ memory      │
   ├─────────┼────────────┼────────────┼─────────────┤
   │   4 KB  │    4 KB    │  24,576    │   96 MiB    │
   │   8 KB  │    8 KB    │  24,576    │  192 MiB    │
   │  16 KB  │   16 KB    │  24,576    │  384 MiB    │
   │ 128 KB  │  128 KB    │  24,576    │ 3072 MiB    │
   ├─────────┼────────────┼────────────┼─────────────┤
   │ 합계                                  ≈ 3.7 GiB    │
   └─────────────────────────────────────────────────┘
```

GPU 메모리 budget 안에 들어와야 함 — 예: A100 40GB / H100 80GB 여유, 24GB급 카드는 iodepth 또는 큰 class 풀을 줄여 적응.

#### 6.3.2 예시 — 작은 I/O 위주 분포 (4K:60%, 8K:25%, 16K:10%, 128K:5%)

```
   ┌─────────┬────────────┬────────────┬─────────────┐
   │ class    │ slots       │ memory                  │
   ├─────────┼────────────┼─────────────────────────┤
   │   4 KB  │   58,982    │   230 MiB                │
   │   8 KB  │   24,576    │   192 MiB                │
   │  16 KB  │    9,830    │   154 MiB                │
   │ 128 KB  │    4,915    │   614 MiB                │
   ├─────────┼────────────┼─────────────────────────┤
   │ 합계                       ≈ 1.2 GiB              │
   └─────────────────────────────────────────────────┘
```

작은 I/O 위주면 압박 ↓. 워크로드 프로파일링 후 분배 비율 결정.

#### 6.3.3 SPDK vtophys 트리와의 비교 (설계 결정 근거)

```
                 SPDK 데몬                   parallelink
   ─────────────────────────  ──────────────────────────────
   메모리 풀 구성   동적 (DPDK가         정적 (init 시 한 번
                  hugepage 추가/삭제)    cudaMalloc + IOMMU)
   PRP 주소 획득   spdk_vtophys()         prp1[slot_idx]
                  2-level radix tree    배열 인덱싱 1번
                  시프트 2 + 접근 2      접근 1
   왜 가능한가     hugepage 변동 가능     모든 페이지 fix &
                  → 동적 매핑 필요        사전 IOMMU 등록
```

parallelink는 BaM의 정적 사전 등록 모델을 그대로 계승 — *vtophys 트리 불필요*. 대신 *인덱스 발급기* 만 동적이며, 그 동적성을 Magazine으로 amortize한다.

#### 6.3.4 Spill 메커니즘 (분포 어긋날 때 안전장치)

```c
__device__ struct buf_handle plink_buf_alloc_with_spill(...) {
    int cls = pick_class(bs);
    /* 일반 경로 */
    ... try alloc from class[cls] ...
    if (got) return handle;

    /* spill: 한 단계 위 class로 (메모리 일부 낭비 감수) */
    for (int c = cls + 1; c < NUM_CLASSES; c++) {
        ... try alloc from class[c] ...
        if (got) return handle_with_class(c, ...);
    }
    return INVALID;   /* 호출자가 backpressure */
}
```

운영 중 spill rate를 카운터로 노출 → 풀 사이즈 재조정 신호.

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

## 10. 모듈별 Pseudocode

> 의사코드 수준이지 실제 컴파일 가능한 코드는 아니다. 각 모듈의 **핵심 알고리즘과 동기화 포인트**만 노출. 실제 구현 시 BaM API, CUDA atomic intrinsic, libfuse3 콜백 시그니처 등으로 구체화한다.

### 10.1 ① Queue Update Manager (GPU)

```c
__device__ struct queue_state {
    uint32_t sq_tail_reserve;  /* 다음 SQ slot 예약 카운터 */
    uint32_t sq_tail_publish;  /* 실제 doorbell write된 tail */
    uint32_t cq_head;
    uint32_t cq_phase;         /* 0/1 phase tag */
    uint32_t depth;
    uint32_t shadow_sq_db;     /* NVMe 1.3+ 옵션 */
    uint32_t shadow_cq_db;
    volatile uint32_t *sq_db_bar;  /* SSD BAR doorbell 주소 (P2P) */
    volatile uint32_t *cq_db_bar;
};

/* SQ slot 예약 — 여러 warp 공유 시 atomic 진행 */
__device__ uint32_t queue_acquire_sq_slot(queue_state *q) {
    uint32_t slot = atomicAdd(&q->sq_tail_reserve, 1);
    /* 호출자가 slot % depth로 wrap, full 검사는 cid_alloc 단계에서 */
    return slot % q->depth;
}

/* SQE write 완료 후 doorbell 발행 — 순서 보존 */
__device__ void queue_publish_sq(queue_state *q, uint32_t my_slot) {
    uint32_t my_tail = (my_slot + 1) % q->depth;

    /* 내 차례 올 때까지 wait — sq_tail_publish가 my_slot에 도달해야 함.
     * 이래야 SQE write 순서가 doorbell 순서와 일치. */
    while (atomicCAS(&q->sq_tail_publish, my_slot, my_tail) != my_slot) {
        __nanosleep(50);
    }

    /* doorbell write — SSD BAR P2P (host 우회) */
    *(q->sq_db_bar) = my_tail;

    if (shadow_db_enabled)
        atomicExch(&q->shadow_sq_db, my_tail);
}

/* CQ head 진행 + doorbell */
__device__ void queue_advance_cq(queue_state *q, uint32_t new_head, uint32_t new_phase) {
    q->cq_head = new_head;
    q->cq_phase = new_phase;
    *(q->cq_db_bar) = new_head;
}
```

### 10.2 ② CID Manager (GPU)

```c
__device__ struct cid_pool {
    uint32_t free_bitmap[N_WORDS];   /* depth 비트, 1=free 0=in-use */
    struct {
        uint64_t submit_tsc;
        uint64_t lba;
        uint16_t bs;
        uint8_t  rw_op;
    } tracker[QUEUE_DEPTH];
};

/* find-and-set lowest free bit, atomic */
__device__ int cid_alloc(cid_pool *p) {
    for (int w = 0; w < N_WORDS; w++) {
        uint32_t old = p->free_bitmap[w];
        while (old != 0) {
            int bit = __ffs(old) - 1;          /* free=1, 가장 낮은 bit */
            uint32_t mask = 1u << bit;
            uint32_t prev = atomicCAS(&p->free_bitmap[w], old, old & ~mask);
            if (prev == old) return w * 32 + bit;  /* 할당 성공 */
            old = prev;                         /* 경쟁 — 재시도 */
        }
    }
    return -1;  /* depth 초과 — 호출자가 inflight drain 후 재시도 */
}

__device__ void cid_free(cid_pool *p, int cid) {
    uint32_t mask = 1u << (cid % 32);
    atomicOr(&p->free_bitmap[cid / 32], mask);
}

__device__ void cid_record(cid_pool *p, int cid,
                           uint64_t lba, uint16_t bs, uint8_t op) {
    p->tracker[cid].submit_tsc = clock64();
    p->tracker[cid].lba = lba;
    p->tracker[cid].bs = bs;
    p->tracker[cid].rw_op = op;
}
```

### 10.3 ③ PRP/SGL Builder (GPU)

```c
/* PRP 빌드 — bs와 page 경계에 따라 PRP1, PRP2, (옵션) PRP list */
__device__ int build_prp(uint64_t buf_iova, uint32_t bs, uint32_t page_size,
                         uint64_t *prp_list_buf, uint64_t prp_list_iova,
                         uint64_t *out_prp1, uint64_t *out_prp2) {
    uint32_t offset = buf_iova & (page_size - 1);
    *out_prp1 = buf_iova;

    /* Case 1: 단일 페이지 내 — PRP1만 */
    if (bs <= page_size - offset) {
        *out_prp2 = 0;
        return 0;
    }

    uint32_t remain = bs - (page_size - offset);

    /* Case 2: 두 페이지 — PRP2가 두 번째 페이지 base */
    if (remain <= page_size) {
        *out_prp2 = (buf_iova + page_size - offset) & ~(uint64_t)(page_size - 1);
        return 0;
    }

    /* Case 3: PRP list 필요 */
    uint32_t n_pages = (remain + page_size - 1) / page_size;
    uint64_t cur = (buf_iova + page_size - offset) & ~(uint64_t)(page_size - 1);
    for (uint32_t i = 0; i < n_pages; i++) {
        prp_list_buf[i] = cur;
        cur += page_size;
    }
    *out_prp2 = prp_list_iova;
    return n_pages;
}
```

### 10.4 ④ Command Submitter (GPU)

```c
/* IO 한 개 submit — Workload Generator가 결정한 lba/op로 */
__device__ bool submit_io(plink_workload_desc *desc,
                         queue_state *q, cid_pool *cidp,
                         nvme_sqe *sq_ring, void *dma_buf,
                         uint64_t lba, uint8_t rw_op) {

    /* 1. CID 할당 (inflight 한도 확인) */
    int cid = cid_alloc(cidp);
    if (cid < 0) return false;          /* iodepth 초과 — 호출자가 backoff */

    /* 2. PRP 빌드 */
    uint64_t prp1, prp2;
    build_prp(virt_to_iova(dma_buf), desc->bs, desc->page_size,
              cidp->prp_list[cid], cidp->prp_list_iova[cid],
              &prp1, &prp2);

    /* 3. SQ slot 예약 + SQE 빌드 */
    uint32_t slot = queue_acquire_sq_slot(q);
    nvme_sqe *sqe = &sq_ring[slot];

    sqe->cdw0  = (rw_op & 0xFF) | ((cid & 0xFFFF) << 16);
    sqe->nsid  = desc->nsid;
    sqe->mptr  = 0;
    sqe->prp1  = prp1;
    sqe->prp2  = prp2;
    sqe->cdw10 = (uint32_t)(lba & 0xFFFFFFFF);            /* SLBA lower */
    sqe->cdw11 = (uint32_t)(lba >> 32);                    /* SLBA upper */
    sqe->cdw12 = (desc->bs / desc->lba_size) - 1;          /* NLB 0-based */
    sqe->cdw13 = desc->fua ? (1u << 30) : 0;
    sqe->cdw14 = 0;
    sqe->cdw15 = 0;

    /* 4. submit_tsc 기록 (latency 측정 시작) */
    cid_record(cidp, cid, lba, desc->bs, rw_op);

    /* 5. doorbell publish — SSD BAR P2P */
    queue_publish_sq(q, slot);

    /* 6. 통계 inflight 증가 */
    atomicAdd_system(&stats->io_inflight, 1);
    return true;
}
```

### 10.5 ⑤ Completion Checker (GPU)

```c
__device__ void process_completions(queue_state *q, cid_pool *cidp,
                                    nvme_cqe *cq_ring,
                                    plink_stats *stats) {
    uint32_t head  = q->cq_head;
    uint32_t phase = q->cq_phase;
    uint32_t consumed = 0;

    while (true) {
        nvme_cqe *cqe = &cq_ring[head];

        /* phase bit polling — GPU memory only, PCIe traffic 0 */
        uint16_t status_phase = cqe->status_phase;
        if ((status_phase & 0x1) != phase) break;     /* 새 CQE 없음 */
        __threadfence();                              /* 다른 필드 read 보장 */

        /* CQE 파싱 */
        uint16_t cid    = cqe->cid;
        uint16_t status = (status_phase >> 1) & 0x7FFF;
        uint8_t  sct    = (status >> 8) & 0x7;
        uint8_t  sc     = status & 0xFF;

        /* latency 계산 */
        uint64_t lat_ns = (clock64() - cidp->tracker[cid].submit_tsc)
                          * NS_PER_TICK;

        /* 통계 emit (host-pinned atomic write) */
        if (sct == 0 && sc == 0) {
            atomicAdd_system(&stats->io_completed, 1);
            uint16_t bs = cidp->tracker[cid].bs;
            if (cidp->tracker[cid].rw_op == NVME_OPC_READ)
                atomicAdd_system(&stats->bytes_read, bs);
            else
                atomicAdd_system(&stats->bytes_written, bs);
        } else {
            atomicAdd_system(&stats->io_errors, 1);
        }
        atomicAdd_system(&stats->lat_sum_ns, lat_ns);
        atomicMin_system(&stats->lat_min_ns, lat_ns);
        atomicMax_system(&stats->lat_max_ns, lat_ns);
        atomicAdd_system(&stats->lat_buckets[lat_to_bucket(lat_ns)], 1);
        atomicSub_system(&stats->io_inflight, 1);

        /* CID 해제 — 다음 submit 가능 */
        cid_free(cidp, cid);

        /* CQ head 진행, wrap 시 phase flip */
        head = (head + 1) % q->depth;
        if (head == 0) phase ^= 1;
        consumed++;
    }

    /* CQ doorbell 갱신 — batching 가능 (consumed > 0일 때만) */
    if (consumed > 0)
        queue_advance_cq(q, head, phase);
}
```

### 10.6 ⑥ Workload Generator (GPU)

```c
__device__ uint64_t next_lba(plink_workload_desc *desc, curandState *rng,
                             uint64_t *seq_counter) {
    switch (desc->rw) {
    case READ:
    case WRITE:
        /* sequential */
        uint64_t lba = desc->lba_offset
                     + (*seq_counter * desc->bs / desc->lba_size);
        if (lba + desc->bs / desc->lba_size
            > desc->lba_offset + desc->lba_count)
            *seq_counter = 0;                /* wrap */
        else
            (*seq_counter)++;
        return lba;
    case RANDREAD:
    case RANDWRITE:
    case RANDRW:
        return desc->lba_offset
             + (uint64_t)(curand_uniform(rng) * desc->lba_count);
    }
}

__device__ uint8_t next_op(plink_workload_desc *desc, curandState *rng) {
    if (desc->rw == RANDRW)
        return (curand_uniform(rng) * 100 < desc->rwmixread)
               ? NVME_OPC_READ : NVME_OPC_WRITE;
    return (desc->rw == READ || desc->rw == RANDREAD)
           ? NVME_OPC_READ : NVME_OPC_WRITE;
}

__device__ bool should_stop(plink_workload_desc *desc, plink_control *ctrl,
                           plink_stats *stats, uint64_t start_tsc) {
    /* control->stop_request — single thread + nanosleep으로 PCIe 절약 */
    __shared__ uint8_t s_stop;
    if (threadIdx.x == 0) {
        s_stop = atomicLoad_system(&ctrl->stop_request);
        __nanosleep(1000);                   /* 1μs backoff */
    }
    __syncthreads();
    if (s_stop) return true;

    if (desc->runtime_ns > 0
        && (clock64() - start_tsc) * NS_PER_TICK > desc->runtime_ns)
        return true;

    if (desc->io_count_limit > 0
        && atomicLoad_system(&stats->io_completed) >= desc->io_count_limit)
        return true;

    return false;
}

/* 메인 worker loop — 각 warp가 자기 큐를 잡고 자율 동작 */
__device__ void worker_loop(plink_workload_desc *desc, queue_state *q,
                           cid_pool *cidp, void *dma_buf,
                           plink_stats *stats, plink_control *ctrl) {
    curandState rng;
    curand_init(desc->random_seed + warp_id(), 0, 0, &rng);
    uint64_t seq_counter = 0;
    uint64_t start_tsc = clock64();

    while (!should_stop(desc, ctrl, stats, start_tsc)) {
        /* submit phase */
        while (atomicLoad(&stats->io_inflight) < desc->iodepth) {
            uint64_t lba = next_lba(desc, &rng, &seq_counter);
            uint8_t  op  = next_op(desc, &rng);
            if (!submit_io(desc, q, cidp, sq_ring, dma_buf, lba, op))
                break;                       /* CID pool 고갈 — completion 처리 우선 */
        }

        /* completion phase */
        process_completions(q, cidp, cq_ring, stats);
    }

    /* drain — outstanding 모두 완료까지 */
    while (atomicLoad(&stats->io_inflight) > 0)
        process_completions(q, cidp, cq_ring, stats);
}
```

### 10.7 ⑦ Logger (host + GPU)

#### Host side

```c
/* daemon/include/plink_log.h */
#define PLINK_LOG(level, comp, fmt, ...) do {                          \
    if ((level) <= plink_log_get_level((comp)))                        \
        plink_log_emit((level), (comp), __FILE__, __LINE__, (fmt),     \
                       ##__VA_ARGS__);                                 \
} while (0)

void plink_log_emit(int level, const char *comp, const char *file,
                   int line, const char *fmt, ...) {
    char buf[1024];
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    int n = snprintf(buf, sizeof(buf),
                     "[%lu.%06lu] [%s] [%s:%d] ",
                     ts.tv_sec, ts.tv_nsec / 1000,
                     level_str(level), comp, line);
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    va_end(ap);

    if (g_sink_mask & SINK_STDERR) fputs(buf, stderr);
    if (g_sink_mask & SINK_SYSLOG) syslog(level, "%s", buf);
    if (g_sink_mask & SINK_FILE)   write(g_log_fd, buf, strlen(buf));
}
```

#### Device side (GPU log ring)

```c
__device__ void GPU_LOG(int level, int fmt_id,
                       uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    /* 한 entry 슬롯 예약 */
    uint64_t slot = atomicAdd_system(&g_log_ring->head, 1);

    /* drop policy: ring full 시 drop (블로킹 안 함) */
    if (slot - atomicLoad_system(&g_log_ring->tail) >= N_LOG_ENTRIES)
        return;

    gpu_log_entry *e = &g_log_ring->entries[slot % N_LOG_ENTRIES];
    e->tsc     = clock64();
    e->level   = level;
    e->fmt_id  = fmt_id;
    e->args[0] = a0; e->args[1] = a1; e->args[2] = a2; e->args[3] = a3;

    __threadfence_system();
    /* visibility marker — CPU drain은 seq[slot]==slot 확인 후 read */
    g_log_ring->seq[slot % N_LOG_ENTRIES] = slot;
}

/* host-side drain thread */
void *log_drain_thread(void *arg) {
    plink_session *sess = arg;
    while (sess->state == RUNNING) {
        uint64_t head = atomic_load(&sess->log_ring->head);
        while (sess->log_ring->tail < head) {
            uint64_t i = sess->log_ring->tail % N_LOG_ENTRIES;
            /* GPU의 visibility 확인 — entry write 완료 안 됐으면 wait */
            if (sess->log_ring->seq[i] != sess->log_ring->tail)
                break;

            gpu_log_entry e = sess->log_ring->entries[i];
            const char *fmt = lookup_fmt(e.fmt_id);
            char rendered[512];
            snprintf(rendered, sizeof(rendered), fmt,
                    e.args[0], e.args[1], e.args[2], e.args[3]);
            PLINK_LOG(e.level, "kernel",
                     "[gpu tsc=%lu] %s", e.tsc, rendered);
            sess->log_ring->tail++;
        }
        usleep(10000);                       /* 10ms backoff */
    }
    return NULL;
}
```

### 10.8 ⑧ Control Path (CUSE thread + io_msg ring + session manager)

#### CUSE ioctl 디스패처

```c
static void plink_cuse_ioctl(fuse_req_t req, int cmd, void *arg, ...) {
    switch (cmd) {
    /* NVMe 표준 — io_msg ring으로 위임 (admin worker가 처리) */
    case NVME_IOCTL_ADMIN_CMD:
    case NVME_IOCTL_ADMIN64_CMD: {
        admin_msg *msg = malloc(sizeof(*msg));
        msg->req = req;
        msg->cmd = *(struct nvme_passthru_cmd *)arg;
        if (!io_msg_enqueue(&g_admin_ring, msg)) {
            fuse_reply_err(req, EAGAIN);     /* ring full */
            free(msg);
        }
        return;                              /* admin worker가 fuse_reply */
    }
    case NVME_IOCTL_RESET:    handle_ctrlr_reset(req); return;
    case NVME_IOCTL_RESCAN:   handle_ctrlr_rescan(req); return;

    /* parallelink 자체 */
    case PLINK_IOC_OPEN_SESSION:    handle_open_session(req, arg); return;
    case PLINK_IOC_START_WORKLOAD:  handle_start_workload(req, arg); return;
    case PLINK_IOC_STOP_WORKLOAD:   handle_stop_workload(req, arg); return;
    case PLINK_IOC_QUERY_INVENTORY:
    case PLINK_IOC_QUERY_SESSION:
    case PLINK_IOC_QUERY_QUEUE_POOL:
    case PLINK_IOC_QUERY_SMART:     handle_query(req, cmd, arg); return;

    /* 거부 */
    case NVME_IOCTL_SUBMIT_IO:
    case NVME_IOCTL_IO_CMD:
    case NVME_IOCTL_IO64_CMD:       fuse_reply_err(req, ENOTSUP); return;

    default:                        fuse_reply_err(req, ENOTTY); return;
    }
}
```

#### io_msg ring (lock-free MPSC)

```c
struct io_msg_ring {
    _Atomic uint64_t prod_head;
    _Atomic uint64_t prod_tail;
    _Atomic uint64_t cons_head;
    void            *entries[RING_SIZE];
    uint64_t         mask;                   /* RING_SIZE - 1 */
};

bool io_msg_enqueue(io_msg_ring *r, void *msg) {
    uint64_t ph, pn, ct;
    do {
        ph = atomic_load(&r->prod_head);
        ct = atomic_load(&r->cons_head);
        if (ph - ct >= RING_SIZE) return false;     /* full */
        pn = ph + 1;
    } while (!atomic_compare_exchange_weak(&r->prod_head, &ph, pn));

    r->entries[ph & r->mask] = msg;
    /* 다른 producer가 prod_tail을 ph로 진행시킬 때까지 wait — order 보존 */
    while (atomic_load(&r->prod_tail) != ph) cpu_relax();
    atomic_store(&r->prod_tail, pn);
    return true;
}

void *io_msg_dequeue(io_msg_ring *r) {
    uint64_t ch = atomic_load(&r->cons_head);
    uint64_t pt = atomic_load(&r->prod_tail);
    if (ch == pt) return NULL;                      /* empty */
    void *msg = r->entries[ch & r->mask];
    atomic_store(&r->cons_head, ch + 1);
    return msg;
}
```

#### admin worker thread

```c
void *admin_worker_thread(void *arg) {
    while (g_running) {
        admin_msg *msg = io_msg_dequeue(&g_admin_ring);
        if (!msg) { usleep(100); continue; }

        /* NVMe admin SQE 빌드 */
        nvm_admin_cmd ac;
        ac.opcode = msg->cmd.opcode;
        ac.nsid   = msg->cmd.nsid;
        ac.cdw10  = msg->cmd.cdw10;
        /* ... (cdw11~cdw15) ... */

        /* DMA buffer 준비 */
        void *dma = NULL;
        if (msg->cmd.data_len > 0) {
            dma = nvm_admin_alloc_dma(msg->cmd.data_len);
            if (is_write_op(msg->cmd.opcode))
                memcpy(dma, (void*)msg->cmd.addr, msg->cmd.data_len);
        }

        /* BaM admin queue submit + wait */
        nvm_cpl cpl;
        int rc = nvm_admin_submit_sync(&g_ctrlr, &ac, dma, &cpl);

        /* 결과 user buffer로 복사 */
        if (rc == 0 && dma && is_read_op(msg->cmd.opcode))
            memcpy((void*)msg->cmd.addr, dma, msg->cmd.data_len);
        msg->cmd.result = cpl.cdw0;

        /* fuse reply */
        fuse_reply_ioctl(msg->req, rc, &msg->cmd, sizeof(msg->cmd));
        nvm_admin_free_dma(dma);
        free(msg);
    }
    return NULL;
}
```

#### session manager — START_WORKLOAD

```c
int handle_start_workload(fuse_req_t req, void *arg) {
    plink_workload_start *in = arg;
    plink_session *sess = session_alloc();

    /* ① IO queue pool LEASE */
    if (io_queue_pool_lease(in->desc.n_queues, sess->qids) < 0) {
        fuse_reply_err(req, EAGAIN);
        return -1;
    }
    sess->n_qids = in->desc.n_queues;

    /* ② shared memory 생성 (host-pinned + GPU mapped) */
    snprintf(sess->stats_path, 64, "/plink_stats_%u", sess->id);
    snprintf(sess->ctrl_path,  64, "/plink_ctrl_%u",  sess->id);

    int sfd = shm_open(sess->stats_path, O_CREAT|O_RDWR, 0660);
    ftruncate(sfd, sizeof(plink_stats));
    sess->stats = mmap(NULL, sizeof(plink_stats),
                       PROT_READ|PROT_WRITE, MAP_SHARED, sfd, 0);
    cudaHostRegister(sess->stats, sizeof(plink_stats),
                     cudaHostRegisterMapped);
    /* ctrl도 동일 */

    /* ③ workload_desc to GPU */
    cudaMalloc(&sess->desc_dev, sizeof(plink_workload_desc));
    cudaMemcpy(sess->desc_dev, &in->desc, sizeof(plink_workload_desc),
               cudaMemcpyHostToDevice);

    /* ④ cudaLaunchKernel */
    cudaStreamCreate(&sess->stream);
    int n_blocks = sess->n_qids;
    int threads  = WARP_SIZE * in->desc.warps_per_queue;
    plink_kernel<<<n_blocks, threads, 0, sess->stream>>>(
        sess->desc_dev, sess->qids, sess->n_qids,
        sess->stats, sess->ctrl);

    /* ⑤ 응답 — shm path 회신 (fio가 shm_open + mmap) */
    in->session_id = sess->id;
    strcpy(in->stats_shm_path,   sess->stats_path);
    strcpy(in->control_shm_path, sess->ctrl_path);
    memcpy(in->qids, sess->qids, sess->n_qids * sizeof(uint16_t));
    in->qid_count = sess->n_qids;
    fuse_reply_ioctl(req, 0, in, sizeof(*in));
    return 0;
}
```

#### session manager — STOP_WORKLOAD

```c
int handle_stop_workload(fuse_req_t req, void *arg) {
    plink_workload_stop *in = arg;
    plink_session *sess = session_lookup(in->session_id);
    if (!sess) { fuse_reply_err(req, ENOENT); return -1; }

    /* ① stop signal — kernel이 다음 polling tick에 감지 */
    atomic_store(&sess->ctrl->stop_request, 1);

    /* ② kernel exit 대기 */
    cudaStreamSynchronize(sess->stream);
    cudaStreamDestroy(sess->stream);

    /* ③ qid pool 반납 */
    io_queue_pool_release(sess->qids, sess->n_qids);

    /* ④ shared memory 정리 */
    cudaHostUnregister(sess->stats);
    munmap(sess->stats, sizeof(plink_stats));
    shm_unlink(sess->stats_path);
    /* ctrl도 동일 */
    cudaFree(sess->desc_dev);

    /* ⑤ session_table 제거 */
    session_free(sess);

    fuse_reply_ioctl(req, 0, NULL, 0);
    return 0;
}
```

### 10.9 ⑨ Monitoring (ioctl handler + plink-stat)

#### daemon ioctl handler

```c
int handle_query_inventory(fuse_req_t req, void *arg) {
    plink_inventory *out = arg;

    /* controller — resource cache */
    out->ctrlr.state    = g_ctrlr_info.state;
    out->ctrlr.capacity = g_ctrlr_info.capacity;
    strncpy(out->ctrlr.model, g_ctrlr_info.model, sizeof(out->ctrlr.model));

    /* namespaces */
    out->n_namespaces = 0;
    for (int i = 1; i <= g_ctrlr_info.num_ns; i++) {
        if (g_ns_table[i].active)
            out->namespaces[out->n_namespaces++] = (struct ns_summary){
                .nsid     = i,
                .blocks   = g_ns_table[i].blocks,
                .lba_size = g_ns_table[i].lba_size,
            };
    }

    /* IO queue pool */
    out->qpool.total  = g_qpool.total;
    out->qpool.idle   = atomic_load(&g_qpool.idle_count);
    out->qpool.leased = atomic_load(&g_qpool.leased_count);

    /* sessions — 각 세션의 host-pinned stats 직접 read */
    pthread_rwlock_rdlock(&g_sessions.lock);
    out->n_sessions = 0;
    for (int sid = 0; sid < MAX_SESSIONS; sid++) {
        plink_session *s = g_sessions.tbl[sid];
        if (!s || s->state != RUNNING) continue;

        plink_stats *st = s->stats;
        out->sessions[out->n_sessions++] = (struct session_summary){
            .sid             = s->id,
            .pid             = s->client_pid,
            .rw              = s->desc.rw,
            .bs              = s->desc.bs,
            .iodepth         = s->desc.iodepth,
            .io_completed    = atomic_load(&st->io_completed),
            .bytes_read      = atomic_load(&st->bytes_read),
            .bytes_written   = atomic_load(&st->bytes_written),
            .lat_avg_ns      = atomic_load(&st->lat_sum_ns)
                             / max(1, atomic_load(&st->io_completed)),
            .lat_p99_ns      = histogram_p99(st->lat_buckets),
            .runtime_sec     = (now_ns() - s->start_tsc_ns) / 1e9,
        };
    }
    pthread_rwlock_unlock(&g_sessions.lock);

    /* SMART (cached) */
    memcpy(&out->smart, &g_log_page_cache.smart, sizeof(out->smart));

    fuse_reply_ioctl(req, 0, out, sizeof(*out));
    return 0;
}
```

#### plink-stat (외부 도구)

```c
int main(int argc, char **argv) {
    int fd = open("/dev/plink0", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    plink_inventory inv;
    if (ioctl(fd, PLINK_IOC_QUERY_INVENTORY, &inv) < 0) {
        perror("ioctl"); return 1;
    }
    close(fd);

    /* GPU 일반 정보 — NVML로 별도 조회 */
    nvmlInit();
    nvmlDevice_t gpu;
    nvmlDeviceGetHandleByIndex(0, &gpu);
    nvmlMemory_t      mem;  nvmlDeviceGetMemoryInfo(gpu, &mem);
    nvmlUtilization_t util; nvmlDeviceGetUtilizationRates(gpu, &util);

    /* 출력 */
    printf("Controller %s — %s (%s)\n",
           inv.ctrlr.dev_path, inv.ctrlr.model, state_str(inv.ctrlr.state));
    printf("  Capacity: %.2f TB    Namespaces: %d\n",
           inv.ctrlr.capacity / 1e12, inv.n_namespaces);

    printf("\nIO Queue Pool (%d total)\n", inv.qpool.total);
    printf("  IDLE: %d  LEASED: %d\n",
           inv.qpool.idle, inv.qpool.leased);

    printf("\nActive Sessions\n");
    printf("  SID  PID    WORKLOAD                   IOPS    BW       LAT(avg/p99)\n");
    for (int i = 0; i < inv.n_sessions; i++) {
        struct session_summary *s = &inv.sessions[i];
        double iops = s->io_completed / s->runtime_sec;
        double bw   = (s->bytes_read + s->bytes_written) / s->runtime_sec;
        printf("  %-3u  %-5d  %s bs=%u qd=%u  %.2fM   %.2f GB/s   %luμs/%luμs\n",
               s->sid, s->pid,
               rw_str(s->rw), s->bs, s->iodepth,
               iops / 1e6, bw / 1e9,
               s->lat_avg_ns / 1000, s->lat_p99_ns / 1000);
    }

    printf("\nGPU (NVML)\n");
    printf("  device 0  SM %u%%  Mem %.1f / %.1f GB\n",
           util.gpu, mem.used / 1e9, mem.total / 1e9);

    nvmlShutdown();
    return 0;
}
```

### 10.10 GPU kernel 통합 진입점

```c
__global__ void plink_kernel(plink_workload_desc *desc,
                             uint16_t *qids, uint32_t n_qids,
                             plink_stats *stats, plink_control *ctrl) {
    /* block 1개당 큐 1개 담당 — desc.warps_per_queue × WARP_SIZE threads */
    uint32_t my_qid = qids[blockIdx.x % n_qids];
    queue_state *q  = &g_queues[my_qid];
    cid_pool    *cp = &g_cidpools[my_qid];

    /* DMA buffer는 BaM이 미리 매핑한 GPU memory pool에서 per-thread 할당 */
    void *dma_buf = bam_dma_buf_alloc(desc->bs);

    /* 메인 worker loop (모듈 ⑥) */
    worker_loop(desc, q, cp, dma_buf, stats, ctrl);

    /* drain 후 자체 종료 → daemon이 cudaStreamSynchronize로 join */
    bam_dma_buf_free(dma_buf);
}
```

---

## 11. 부록

### 11.1 용어 정의

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
| **CLOCK algorithm** | Corbató 1968 (MULTICS) — 페이지 교체 알고리즘. BaM의 page_cache가 GPU 변형 사용 |
| **Slab allocator** | Bonwick 1994. 고정 크기 객체 풀 + 사전 할당. Linux SLUB·FreeBSD UMA |
| **Magazine allocator** | Bonwick & Adams 2001. per-CPU LIFO 캐시 + 글로벌 풀 batch refill/flush. DPDK rte_mempool 사용 |
| **Size-class allocator** | jemalloc·tcmalloc 표준. 사이즈를 기하급수 단계로 분류해 단편화 회피 |
| **free-stack** | 인덱스 발급용 LIFO. atomic counter 1개로 alloc/free 가능, naive는 hot cache line 병목 |
| **globaltimer** | GPU 전역 ns 단조 카운터 (PTX `%globaltimer`). DVFS 무관, SM 간 일관 |
| **clock64()** | SM 단위 cycle counter. SM별 phase 다르고 DVFS 영향 — 로깅 timestamp엔 부적합 |
| **affine fit clock sync** | `T_host = a × T_gpu + b` 두 점 측정. ping-pong RTT 최소값 채택 시 sub-μs 정밀 |
| **NanoLog / Quill** | sub-10ns CPU 로거. deferred formatting + lock-free async + binary 로그 |
| **deferred formatting** | 핫패스에선 raw 인자만 ring에 저장, 포맷팅은 별도 스레드에서 수행 |

### 11.2 약어

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

### 11.3 관련 문서

- `parallelink/CLAUDE.md` — 도메인 지식 + 주석 작업 규칙
- `parallelink/work.md` — 시행착오 로그 (구현 진행)
- `parallelink/docs/design.md` — 초기 설계 (본 문서로 대체 또는 통합)
- `bam-study/CLAUDE.md` — BaM 아키텍처 참고
- `fio-study/CLAUDE.md` — fio ioengine 모델 참고
- `spdk-study/lib/nvme/nvme_cuse.c` — CUSE 차용 소스 (한국어 주석 완비)
- `spdk-study/lib/nvme/nvme_io_msg.c` — io_msg ring 차용 소스 (한국어 주석 완비)

### 11.4 참고문헌 (Allocator / Logger / Clock sync)

**Allocator 계열**:
- Corbató, F. J. (1968). "A Paging Experiment with the Multics System." MIT Project MAC Report. *(CLOCK 원전)*
- Bonwick, J. (1994). "The Slab Allocator: An Object-Caching Kernel Memory Allocator." USENIX Summer.
- Bonwick, J. & Adams, J. (2001). "Magazines and Vmem: Extending the Slab Allocator to Many CPUs and Arbitrary Resources." USENIX. *(Magazine 패턴 원전)*
- Berger, E. et al. (2000). "Hoard: A Scalable Memory Allocator for Multithreaded Applications." ASPLOS-IX.
- Ghemawat, S. (2007). "TCMalloc: Thread-Caching Malloc." Google design notes.

**GPU-storage**:
- Qureshi, Z. et al. (2023). "GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture." ASPLOS 2023. *(BaM 본 논문)*

**Logger / Clock sync**:
- NanoLog (Stanford) — github: PlatformLab/NanoLog. ~7 ns per call, binary log + deferred decode.
- Quill — github: odygrd/quill. Lock-free async, ~8 ns per call.
- AIR (Analytics In Real-time) — Samsung POS 내부, lockless circular buffer 기반.
- NVIDIA CUDA C++ Programming Guide — `printf` 동작 명세, `globaltimer` PTX 정의.
- Nsight Systems Documentation — CUPTI activity callback + clock sync 메커니즘.

### 11.5 본 문서 변경 이력

| 일자 | 갱신 내용 |
|------|---------|
| 2026-04-29 | 초안 작성 — 섹션 0~11 골격 |
| 2026-04-29 | §5.3.1 DMA Buffer Pool Allocator 설계 추가 (free-stack + Magazine + Multi-class, CPU 측정 결과 포함) |
| 2026-04-29 | §5.7.1~5.7.3 Logger 상세 — host 라이브러리 후보, device-side ring buffer + CPU pull, globaltimer + ping-pong calibration |
| 2026-04-29 | §6.3 Buffer Pool 사이징 가이드, SPDK vtophys 비교, spill 메커니즘 |
| 2026-04-29 | §11.1 용어 보강 (CLOCK / Slab / Magazine / Size-class / globaltimer / NanoLog / affine fit), §11.4 참고문헌 추가 |
| 2026-04-29 | §5.1.1~5.1.6 SQ 동시성 모델 (MPSC/SPMC) + Direct rte_ring vs vSQ+Submitter 비교 + CPU 마이크로벤치마크 3 시나리오(UNIFORM/SKEWED/BURST) + warps_per_queue 별 적용 정책 |
| 2026-04-29 | §5.3.1.9~5.3.1.13 Pool 고갈 처리 — Spill/Buddy/Lazy Carve 비교, lazy carve 자료구조·코드, 단편화 처리 정책, 단계적 적용 |

---

**작성일**: 2026-04-29
**상태**: §5.1.1~6·§5.3.1·§5.7.1~3·§6.3 보충 (큐 동시성·allocator·logger·clock-sync·고갈처리 설계 결정 반영)
**다음 갱신 예정**: 모듈별 구현 시작 시 각 모듈 섹션을 코드 위치로 보강
