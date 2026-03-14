# NVMe I/O 경로 전체 분석: fio/nvme-cli에서 SSD Doorbell까지

> 소스코드 기준: ~/sources/linux (커널 6.x 계열)
> 분석 대상 파일:
> - `drivers/nvme/host/pci.c` — NVMe PCIe 전송 드라이버
> - `drivers/nvme/host/core.c` — NVMe 코어 (커맨드 빌드, 동기 실행)
> - `drivers/nvme/host/nvme.h` — NVMe 호스트 내부 헤더
> - `include/linux/nvme.h` — NVMe 스펙 정의 (커맨드/완료 구조체)
> - `drivers/nvme/host/ioctl.c` — NVMe ioctl 인터페이스 (nvme-cli 경로)
> - `block/blk-mq.c` — blk-mq Multi-Queue Block Layer
> - `block/blk-core.c` — Block Layer 코어 (submit_bio)

---

## 1. 전체 I/O 경로 개요

### 1.1 전체 아키텍처 ASCII 다이어그램

```
 ┌──────────────────────────────────────────────────────────────────────────┐
 │                        사용자 공간 (User Space)                         │
 │                                                                         │
 │   ┌─────────┐        ┌──────────┐        ┌─────────────┐               │
 │   │   fio   │        │ nvme-cli │        │  일반 앱    │               │
 │   │(ioengine│        │(passthru)│        │(read/write) │               │
 │   └────┬────┘        └────┬─────┘        └──────┬──────┘               │
 │        │                  │                      │                      │
 │   io_uring /         ioctl(fd,                read() /                  │
 │   libaio /        NVME_IOCTL_*)              write()                    │
 │   pread/pwrite                                                          │
 └────────┼──────────────────┼──────────────────────┼──────────────────────┘
          │                  │                      │
 ─ ─ ─ ─ ┼ ─ ─ ─ ─ syscall ─┼─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┼ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
          │                  │                      │
 ┌────────┼──────────────────┼──────────────────────┼──────────────────────┐
 │        │      커널 공간   │ (Kernel Space)        │                      │
 │        │                  │                      │                      │
 │        │                  │               ┌──────▼──────┐               │
 │        │                  │               │     VFS     │               │
 │        │                  │               │ vfs_read()  │               │
 │        │                  │               │ vfs_write() │               │
 │        │                  │               └──────┬──────┘               │
 │        │                  │                      │                      │
 │        │                  │               ┌──────▼──────┐               │
 │        │                  │               │ 파일시스템  │               │
 │        │                  │               │ (ext4/xfs)  │               │
 │        │                  │               │  또는       │               │
 │        │                  │               │ Direct I/O  │               │
 │        │                  │               └──────┬──────┘               │
 │        │                  │                      │                      │
 │        ▼                  │                      ▼                      │
 │  ┌───────────┐     ┌──────▼──────┐       ┌─────────────┐               │
 │  │ io_uring  │     │ nvme_dev_   │       │ submit_bio()│               │
 │  │ / libaio  │     │   ioctl()   │       │  (블록코어) │               │
 │  └─────┬─────┘     └──────┬──────┘       └──────┬──────┘               │
 │        │                  │                      │                      │
 │        │           ┌──────▼──────┐               │                      │
 │        │           │nvme_user_   │               │                      │
 │        │           │   cmd()     │               │                      │
 │        │           └──────┬──────┘               │                      │
 │        │                  │                      │                      │
 │        │           ┌──────▼──────────┐           │                      │
 │        │           │nvme_submit_     │           │                      │
 │        │           │  user_cmd()     │           │                      │
 │        │           │                 │           │                      │
 │        │           │blk_mq_alloc_   │           │                      │
 │        │           │  request()     │           │                      │
 │        │           │(스케줄러 우회) │           │                      │
 │        │           └───────┬────────┘           │                      │
 │        │                   │                     │                      │
 │ ┌──────▼───────────────────┼─────────────────────▼──────────────────┐   │
 │ │              Block Layer (blk-mq)                                 │   │
 │ │                                                                   │   │
 │ │  blk_mq_submit_bio()                                             │   │
 │ │    → bio → request 변환                                          │   │
 │ │    → I/O 병합 (merge) 시도                                       │   │
 │ │    → plug 리스트 또는 스케줄러 큐에 삽입                          │   │
 │ │                                                                   │   │
 │ │  blk_mq_dispatch_rq_list()                                       │   │
 │ │    → 드라이버 tag 확보                                            │   │
 │ │    → q->mq_ops->queue_rq() 호출  ────────────────────┐           │   │
 │ │                                                       │           │   │
 │ └───────────────────────────────────────────────────────┼───────────┘   │
 │                                                         │               │
 │ ┌───────────────────────────────────────────────────────▼───────────┐   │
 │ │              NVMe PCIe Driver (pci.c)                             │   │
 │ │                                                                   │   │
 │ │  nvme_queue_rq(hctx, bd)                                         │   │
 │ │    ├─ nvme_prep_rq(req)                                          │   │
 │ │    │    ├─ nvme_setup_cmd()  ← NVMe 커맨드 빌드                  │   │
 │ │    │    ├─ nvme_map_data()   ← DMA 매핑 (PRP/SGL)               │   │
 │ │    │    └─ nvme_start_request() ← 타이머 시작                    │   │
 │ │    │                                                              │   │
 │ │    ├─ spin_lock(&nvmeq->sq_lock)                                 │   │
 │ │    ├─ nvme_sq_copy_cmd()    ← SQ에 64바이트 커맨드 복사          │   │
 │ │    ├─ nvme_write_sq_db()    ← SQ Tail Doorbell 쓰기             │   │
 │ │    └─ spin_unlock(&nvmeq->sq_lock)                               │   │
 │ │                                                                   │   │
 │ └──────────────────────────────────────────────────┬────────────────┘   │
 │                                                    │                    │
 └────────────────────────────────────────────────────┼────────────────────┘
                                                      │
                                                      │  writel()
                                                      │  (MMIO 쓰기)
                                                      ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                         PCIe Bus                                       │
 │                                                                        │
 │   BAR0 + 0x1000 + (2*qid*stride)  ← SQ Tail Doorbell 레지스터        │
 │                                                                        │
 └───────────────────────────────────────────┬────────────────────────────┘
                                             │
                                             ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                    NVMe SSD 컨트롤러                                   │
 │                                                                        │
 │   1. Doorbell 값 읽음 → 새 커맨드가 SQ에 있음을 인지                  │
 │   2. PCIe DMA로 호스트 메모리의 SQ에서 커맨드(64B) 페치               │
 │   3. PRP/SGL이 가리키는 호스트 메모리에서 데이터 DMA                   │
 │   4. NAND Flash에 데이터 기록 (Write) 또는 NAND에서 읽기 (Read)       │
 │   5. CQ에 완료 엔트리(16B)를 DMA로 기록                               │
 │   6. MSI-X 인터럽트 발생                                               │
 │                                                                        │
 └────────────────────────────────────────────────────────────────────────┘
```

### 1.2 각 계층의 역할 요약

| 계층 | 파일 | 핵심 역할 |
|------|------|-----------|
| **사용자 공간** | fio, nvme-cli | I/O 요청 생성, 시스템 콜 호출 |
| **VFS** | fs/read_write.c | 파일 → 블록 디바이스 매핑, 권한 검사 |
| **파일시스템** | fs/ext4/, fs/xfs/ | 논리 파일 오프셋 → LBA 변환, 페이지 캐시 관리 |
| **Block Core** | block/blk-core.c | submit_bio() 진입점, bio 유효성 검사, 파티션 리매핑 |
| **blk-mq** | block/blk-mq.c | bio → request 변환, I/O 병합, 스케줄러 연동, 드라이버 디스패치 |
| **NVMe Core** | drivers/nvme/host/core.c | NVMe 커맨드 빌드 (nvme_setup_cmd), 동기/비동기 커맨드 실행 |
| **NVMe PCIe** | drivers/nvme/host/pci.c | SQ/CQ 관리, DMA 매핑, Doorbell 쓰기, 인터럽트 처리 |
| **NVMe ioctl** | drivers/nvme/host/ioctl.c | passthrough 커맨드 처리 (nvme-cli 등 사용자 도구) |
| **하드웨어** | NVMe SSD | PCIe BAR0 레지스터, SQ/CQ DMA, NAND 연산 |

---

## 2. 두 가지 I/O 경로 비교

### 2.1 fio 경로 (Block I/O Path)

일반적인 파일시스템 I/O 또는 블록 디바이스 직접 I/O 경로이다. fio는 다양한 ioengine(libaio, io_uring, sync, psync 등)을 통해 이 경로를 사용한다.

```
fio → read()/write()/io_submit()/io_uring_enter()
  │
  ▼
  VFS: vfs_read()/vfs_write() 또는 io_uring 직접 경로
  │
  ▼
  파일시스템 (ext4/xfs) 또는 Direct I/O
  │   - 페이지 캐시 경유: 버퍼링된 I/O (write-back)
  │   - Direct I/O: 캐시 우회, bio를 직접 생성
  │
  ▼
  submit_bio(bio)                    ← block/blk-core.c:1244
  │
  ├─ submit_bio_noacct()             ← 유효성 검사 (크기, 읽기전용, 파티션)
  │   └─ submit_bio_noacct_nocheck() ← cgroup 회계, 트레이싱
  │       └─ __submit_bio_noacct_mq()← blk-mq 전용 재귀 방지 루프
  │           └─ __submit_bio()      ← 실제 디스패치 시작
  │
  ▼
  blk_mq_submit_bio(bio)             ← block/blk-mq.c:3697
  │
  ├─ __bio_split_to_limits()         ← bio를 큐 제한(max_sectors 등)에 맞게 분할
  ├─ 병합 시도: 기존 request에 bio를 합칠 수 있는지 확인
  ├─ bio → request 변환 (blk_mq_get_new_requests)
  │   └─ blk_mq_alloc_request_hctx() → tag 할당
  │
  ├─ [경로 A: plug 있음]
  │   blk_add_rq_to_plug() → plug 리스트에 추가
  │   (나중에 blk_flush_plug() → blk_mq_dispatch_rq_list())
  │
  ├─ [경로 B: 스케줄러 있음]
  │   blk_mq_sched_insert_request() → 스케줄러 큐에 삽입
  │   → blk_mq_run_hw_queue() → blk_mq_dispatch_rq_list()
  │
  └─ [경로 C: 직접 디스패치 (최단 경로)]
      blk_mq_try_issue_directly() → blk_mq_dispatch_rq_list()
  │
  ▼
  blk_mq_dispatch_rq_list()          ← block/blk-mq.c:2502
  │
  ├─ blk_mq_prep_dispatch_rq()      ← driver tag 확보
  ├─ bd.rq = rq, bd.last = 마지막 요청인지 표시
  └─ q->mq_ops->queue_rq(hctx, &bd) ← ★ NVMe 드라이버 진입점!
  │
  ▼
  nvme_queue_rq(hctx, bd)            ← drivers/nvme/host/pci.c:1675
  │
  ├─ nvme_prep_rq(req)
  │   ├─ nvme_setup_cmd()            ← NVMe 커맨드 빌드 (opcode, LBA, 길이)
  │   ├─ nvme_map_data()             ← 데이터 DMA 매핑 (PRP/SGL 구성)
  │   ├─ nvme_map_metadata()         ← 메타데이터 DMA 매핑 (필요 시)
  │   └─ nvme_start_request()        ← blk-mq에 요청 시작 알림 (타이머 시작)
  │
  ├─ spin_lock(&nvmeq->sq_lock)
  ├─ nvme_sq_copy_cmd(nvmeq, &iod->cmd)  ← SQ Tail에 64B 커맨드 복사
  ├─ nvme_write_sq_db(nvmeq, bd->last)    ← SQ Tail Doorbell 쓰기
  │   └─ writel(sq_tail, nvmeq->q_db)    ← MMIO로 컨트롤러에 통보
  └─ spin_unlock(&nvmeq->sq_lock)
```

**핵심 포인트:**
- bio(Block I/O)가 request로 변환되면서 인접한 I/O 병합(merge)이 가능하다.
- I/O 스케줄러(none/mq-deadline/bfq/kyber)가 요청 순서를 최적화할 수 있다.
- plug 메커니즘으로 여러 request를 모아서 한 번에 디스패치한다.
- `bd->last` 플래그로 Doorbell 쓰기를 배치 최적화한다.

### 2.2 nvme-cli 경로 (Passthrough/ioctl Path)

nvme-cli 등의 관리 도구가 NVMe 커맨드를 직접 구성하여 전달하는 경로이다. Block Layer의 I/O 스케줄러와 병합 로직을 우회한다.

```
nvme-cli → open("/dev/nvme0") 또는 open("/dev/nvme0n1")
  │
  ▼
  ioctl(fd, NVME_IOCTL_IO_CMD, &cmd)   또는
  ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd)
  │
  ▼
  nvme_dev_ioctl()                       ← drivers/nvme/host/ioctl.c:840
  │
  ├─ NVME_IOCTL_ADMIN_CMD → nvme_user_cmd(ctrl, NULL, ...)
  ├─ NVME_IOCTL_IO_CMD    → nvme_dev_user_cmd(ctrl, ...)
  └─ NVME_IOCTL_ADMIN64_CMD → nvme_user_cmd64(ctrl, NULL, ...)
  │
  ▼
  nvme_user_cmd(ctrl, ns, ucmd)          ← ioctl.c:281
  │
  ├─ copy_from_user(&cmd, ucmd, ...)     ← 사용자 공간에서 커맨드 구조체 복사
  ├─ NVMe 커맨드 구조체 수동 구성:
  │   c.common.opcode = cmd.opcode
  │   c.common.nsid   = cpu_to_le32(cmd.nsid)
  │   c.common.cdw10  = cpu_to_le32(cmd.cdw10)
  │   ... (cdw2~cdw15 모두 설정)
  ├─ nvme_cmd_allowed() 권한 검사
  │
  ▼
  nvme_submit_user_cmd(q, &c, ...)       ← ioctl.c:162
  │
  ├─ nvme_alloc_user_request(q, cmd, 0, 0)
  │   └─ blk_mq_alloc_request()          ← ★ request 직접 할당 (bio 없이!)
  │      └─ NVME_REQ_USERCMD 플래그 설정
  │
  ├─ nvme_map_user_request()              ← 사용자 버퍼 → 커널 bio 매핑
  │   └─ blk_rq_map_user_io()
  │
  ├─ nvme_execute_rq(req, false)          ← 동기 실행 (완료까지 대기)
  │   └─ blk_execute_rq()
  │       └─ blk_mq_insert_request() → blk_mq_run_hw_queue()
  │           └─ blk_mq_dispatch_rq_list()
  │               └─ nvme_queue_rq()     ← ★ 동일한 드라이버 진입점
  │                   ├─ nvme_sq_copy_cmd()
  │                   └─ nvme_write_sq_db()
  │                       └─ writel() → Doorbell
  │
  └─ 완료 대기 → 결과 반환
      └─ put_user(result, &ucmd->result)  ← 결과를 사용자 공간에 복사
```

**핵심 포인트:**
- 사용자가 NVMe 커맨드의 모든 필드(opcode, cdw10~cdw15)를 직접 구성한다.
- `blk_mq_alloc_request()`로 request를 직접 할당하므로 bio → request 변환이 없다.
- I/O 스케줄러를 거치지 않고 `blk_execute_rq()`로 직접 디스패치한다.
- 동기 실행이므로 커맨드가 완료될 때까지 호출 스레드가 블로킹된다.
- 최종적으로 동일한 `nvme_queue_rq()`를 통해 하드웨어에 도달한다.

### 2.3 두 경로의 핵심 차이 요약

| 특성 | fio (Block I/O) | nvme-cli (Passthrough) |
|------|-----------------|----------------------|
| **진입점** | submit_bio() | ioctl() |
| **커맨드 구성** | 커널이 자동 (nvme_setup_cmd) | 사용자가 직접 (cdw0~cdw15) |
| **bio → request 변환** | 있음 | 없음 (request 직접 할당) |
| **I/O 병합** | 가능 (인접 bio 합침) | 불가능 |
| **I/O 스케줄러** | 경유 (none/mq-deadline 등) | 우회 |
| **실행 방식** | 비동기 (콜백으로 완료 통보) | 동기 (완료까지 블로킹) |
| **Doorbell 배치** | bd->last로 배치 가능 | 단일 커맨드씩 |
| **최종 드라이버 함수** | nvme_queue_rq() | nvme_queue_rq() (동일) |

---

## 3. 핵심 데이터 구조 관계도

### 3.1 I/O 경로의 데이터 구조 전체 관계

```
                    사용자 공간의 I/O 요청
                           │
                           ▼
 ┌─────────────────────────────────────────────────┐
 │  struct bio                                      │
 │  ┌─────────────────────────────────────────────┐ │
 │  │ bi_bdev      → 블록 디바이스                │ │
 │  │ bi_opf       → 연산 타입 (READ/WRITE/FLUSH) │ │
 │  │ bi_iter.bi_sector → 시작 섹터 (512B 단위)   │ │
 │  │ bi_iter.bi_size   → 전송 크기 (바이트)      │ │
 │  │ bi_io_vec[]  → 데이터 페이지 배열 (bvec)    │ │
 │  │ bi_end_io    → 완료 콜백 함수                │ │
 │  └─────────────────────────────────────────────┘ │
 │  역할: 디스크의 연속 섹터 범위에 대한 I/O 요청   │
 │  하나의 bio는 하나의 연속 LBA 범위를 나타낸다     │
 └───────────────────────┬─────────────────────────┘
                         │  blk_mq_submit_bio()에서 변환
                         ▼
 ┌─────────────────────────────────────────────────┐
 │  struct request                                  │
 │  ┌─────────────────────────────────────────────┐ │
 │  │ __sector     → 시작 섹터                     │ │
 │  │ __data_len   → 전송 크기                     │ │
 │  │ bio          → bio 체인 (여러 bio 병합 가능) │ │
 │  │ tag          → blk-mq 태그 (= command_id)   │ │
 │  │ mq_hctx      → 할당된 하드웨어 큐            │ │
 │  │ rq_flags     → 요청 플래그                   │ │
 │  │ timeout      → 타임아웃 (jiffies)            │ │
 │  │ end_io       → 완료 콜백                     │ │
 │  │                                              │ │
 │  │ ┌──────────────────────────────────────────┐ │ │
 │  │ │ PDU 영역 (blk_mq_rq_to_pdu로 접근)      │ │ │
 │  │ │ struct nvme_iod (I/O Descriptor)         │ │ │
 │  │ │  ├─ struct nvme_request req              │ │ │
 │  │ │  │    ├─ ctrl → nvme_ctrl 포인터         │ │ │
 │  │ │  │    ├─ cmd  → &iod->cmd 포인터         │ │ │
 │  │ │  │    ├─ status → 완료 상태              │ │ │
 │  │ │  │    └─ result → 완료 결과값            │ │ │
 │  │ │  ├─ struct nvme_command cmd (64바이트)    │ │ │
 │  │ │  ├─ flags (DMA 매핑 상태)                │ │ │
 │  │ │  ├─ nr_descriptors                       │ │ │
 │  │ │  ├─ descriptors[] (PRP/SGL 페이지들)     │ │ │
 │  │ │  └─ meta_dma (메타데이터 DMA 주소)       │ │ │
 │  │ └──────────────────────────────────────────┘ │ │
 │  └─────────────────────────────────────────────┘ │
 │  역할: blk-mq의 I/O 단위. 여러 bio가 병합될 수   │
 │  있고, tag(=command_id)로 완료 시 식별된다        │
 └───────────────────────┬─────────────────────────┘
                         │  nvme_prep_rq()에서 변환
                         ▼
 ┌─────────────────────────────────────────────────┐
 │  struct nvme_command (64바이트, SQE)              │
 │  (= iod->cmd, nvme_sq_copy_cmd로 SQ에 복사됨)   │
 │  ┌─────────────────────────────────────────────┐ │
 │  │ union {                                      │ │
 │  │   nvme_rw_command rw;     ← Read/Write      │ │
 │  │   nvme_common_command common; ← 범용         │ │
 │  │   nvme_dsm_cmd dsm;      ← TRIM             │ │
 │  │   nvme_identify identify; ← Identify         │ │
 │  │   ... (기타 커맨드 타입들)                    │ │
 │  │ }                                            │ │
 │  └─────────────────────────────────────────────┘ │
 │  역할: SSD 컨트롤러가 이해하는 하드웨어 커맨드    │
 │  SQ(Submission Queue)에 복사되는 실제 데이터      │
 └───────────────────────┬─────────────────────────┘
                         │  nvme_sq_copy_cmd()로 복사
                         ▼
 ┌─────────────────────────────────────────────────┐
 │  NVMe Queue Pair (SQ + CQ)                       │
 │  = struct nvme_queue                              │
 │                                                   │
 │  ┌─── Submission Queue (SQ) ──────────────────┐  │
 │  │  sq_cmds: DMA coherent 메모리               │  │
 │  │  ┌────┬────┬────┬────┬─ ─ ─┬────┐          │  │
 │  │  │SQE0│SQE1│SQE2│SQE3│     │SQEn│  64B씩   │  │
 │  │  └────┴────┴────┴────┴─ ─ ─┴────┘          │  │
 │  │       ↑                                      │  │
 │  │    sq_tail (호스트가 관리, 다음 쓸 위치)     │  │
 │  │                                              │  │
 │  │  sq_dma_addr: 디바이스가 이 주소로 SQE 페치  │  │
 │  │  q_db: SQ Tail Doorbell MMIO 주소            │  │
 │  └──────────────────────────────────────────────┘  │
 │                                                    │
 │  ┌─── Completion Queue (CQ) ──────────────────┐   │
 │  │  cqes: DMA coherent 메모리                  │   │
 │  │  ┌────┬────┬────┬────┬─ ─ ─┬────┐          │   │
 │  │  │CQE0│CQE1│CQE2│CQE3│     │CQEn│  16B씩  │   │
 │  │  └────┴────┴────┴────┴─ ─ ─┴────┘          │   │
 │  │       ↑                                      │   │
 │  │    cq_head (호스트가 관리, 다음 읽을 위치)   │   │
 │  │    cq_phase (유효성 판별 비트, CQ 한 바퀴마다 │   │
 │  │             토글: 0→1→0→...)                 │   │
 │  │                                              │   │
 │  │  cq_dma_addr: 디바이스가 여기에 CQE를 DMA    │   │
 │  │  q_db + stride: CQ Head Doorbell MMIO 주소   │   │
 │  └──────────────────────────────────────────────┘   │
 │                                                     │
 │  qid: 큐 ID (0=Admin, 1~N=I/O)                     │
 │  cq_vector: MSI-X 인터럽트 벡터 번호                │
 └─────────────────────────────────────────────────────┘
```

### 3.2 데이터 구조 변환 흐름 요약

```
  bio                request + nvme_iod           nvme_command (SQE)
 ┌──────┐           ┌──────────────┐             ┌──────────────┐
 │sector│──변환──▶  │__sector      │──빌드──▶    │slba          │
 │size  │           │__data_len    │             │length        │
 │bvec[]│           │bio chain     │             │dptr (PRP/SGL)│
 │opf   │           │tag(cmd_id)   │             │opcode        │
 └──────┘           │iod.cmd ──────╋─────────▶   │command_id    │
                    └──────────────┘             └──────┬───────┘
                                                        │
                                    SQ에 복사 ◀─────────┘
                                                        │
                                                        ▼
                                              ┌──────────────────┐
  nvme_completion (CQE)                       │   SSD 컨트롤러    │
 ┌──────────────────┐                         │   커맨드 처리     │
 │command_id        │◀───── DMA ──────────────│                  │
 │status + phase    │                         └──────────────────┘
 │sq_head           │
 │result            │
 └──────┬───────────┘
        │
        │ command_id로 request를 찾음
        ▼
   blk_mq_complete_request(req)
   → 상위 계층 콜백 호출
   → bio->bi_end_io() → 사용자에게 완료 통보
```

### 3.3 struct nvme_queue 필드 상세

`struct nvme_queue`는 `drivers/nvme/host/pci.c:355`에 정의되어 있다.

```c
struct nvme_queue {
    struct nvme_dev *dev;             // 소속 디바이스
    spinlock_t sq_lock;               // SQ 접근 보호 (커맨드 복사 + Doorbell)
    void *sq_cmds;                    // SQ 버퍼 (64B * q_depth)
    spinlock_t cq_poll_lock;          // CQ 폴링 보호 (별도 캐시라인)
    struct nvme_completion *cqes;     // CQ 엔트리 배열 (16B * q_depth)
    dma_addr_t sq_dma_addr;           // SQ DMA 물리 주소
    dma_addr_t cq_dma_addr;           // CQ DMA 물리 주소
    u32 __iomem *q_db;                // SQ Tail Doorbell MMIO 주소
    u32 q_depth;                      // 큐 깊이 (엔트리 수)
    u16 cq_vector;                    // MSI-X 벡터 번호
    u16 sq_tail;                      // SQ Tail (다음 쓸 위치)
    u16 last_sq_tail;                 // 마지막 Doorbell에 쓴 값
    u16 cq_head;                      // CQ Head (다음 읽을 위치)
    u16 qid;                          // 큐 ID (0=Admin)
    u8 cq_phase;                      // CQ Phase 비트 (유효성 판별)
    u8 sqes;                          // SQ Entry Size log2 (보통 6=64B)
    // Shadow Doorbell 포인터들
    __le32 *dbbuf_sq_db;              // SQ Shadow DB
    __le32 *dbbuf_cq_db;              // CQ Shadow DB
    __le32 *dbbuf_sq_ei;              // SQ Event Index
    __le32 *dbbuf_cq_ei;              // CQ Event Index
};
```

---

## 4. NVMe 명령어 구조 (nvme_command)

### 4.1 SQE (Submission Queue Entry) 64바이트 레이아웃

NVMe 커맨드는 항상 64바이트 고정 크기이다. `include/linux/nvme.h`에 정의된 `struct nvme_command`는 모든 커맨드 타입을 하나의 union으로 표현한다.

```
  바이트 오프셋    크기    필드명         설명
 ┌─────────────┬───────┬────────────┬───────────────────────────────────┐
 │  [0]        │  1B   │ opcode     │ 명령 코드 (0x01=Write, 0x02=Read) │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [1]        │  1B   │ flags      │ FUSE(비트1:0) + PSDT(비트7:6)     │
 │             │       │            │ PSDT: 0=PRP, 1=SGL(메타+데이터),  │
 │             │       │            │       2=SGL(데이터만)              │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [3:2]      │  2B   │ command_id │ 명령 식별자 (blk-mq tag)          │
 │             │       │            │ CQE에서 이 값으로 원래 요청 매칭   │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [7:4]      │  4B   │ nsid       │ 대상 Namespace ID                 │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [15:8]     │  8B   │ cdw2, cdw3 │ 명령별 예약 영역                  │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [23:16]    │  8B   │ metadata   │ 메타데이터 포인터 (MPTR)          │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [31:24]    │  8B   │ dptr.prp1  │ PRP Entry 1 / SGL 디스크립터      │
 │             │       │ (또는 sgl) │ 첫 번째 데이터 페이지 물리 주소    │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [39:32]    │  8B   │ dptr.prp2  │ PRP Entry 2 또는 PRP List 주소    │
 │             │       │            │ (데이터가 2페이지 이상일 때)       │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [43:40]    │  4B   │ cdw10      │ R/W: slba[31:0] (시작 LBA 하위)   │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [47:44]    │  4B   │ cdw11      │ R/W: slba[63:32] (시작 LBA 상위)  │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [51:48]    │  4B   │ cdw12      │ R/W: length(15:0) + control(31:16)│
 │             │       │            │ length: 전송 블록 수 - 1 (0 기반) │
 │             │       │            │ control: FUA, LR 등 제어 플래그    │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [55:52]    │  4B   │ cdw13      │ R/W: dsmgmt (접근 빈도/지연 힌트) │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [59:56]    │  4B   │ cdw14      │ R/W: reftag (참조 태그, T10-PI)   │
 ├─────────────┼───────┼────────────┼───────────────────────────────────┤
 │  [63:60]    │  4B   │ cdw15      │ R/W: lbat(15:0) + lbatm(31:16)   │
 │             │       │            │ Application Tag + Mask (T10-PI)   │
 └─────────────┴───────┴────────────┴───────────────────────────────────┘
```

### 4.2 nvme_rw_command 구조체 (include/linux/nvme.h:1406)

```c
struct nvme_rw_command {
    __u8   opcode;       // 0x01(Write) 또는 0x02(Read)
    __u8   flags;        // FUSE + PSDT
    __u16  command_id;   // blk-mq tag → CQE에서 매칭
    __le32 nsid;         // 대상 Namespace ID
    __le32 cdw2;         // 예약
    __le32 cdw3;         // 예약
    __le64 metadata;     // 메타데이터 포인터 (MPTR)
    union nvme_data_ptr dptr;  // 데이터 포인터 (PRP1/PRP2 또는 SGL)
    __le64 slba;         // CDW10-11: Starting LBA
    __le16 length;       // CDW12[15:0]: 블록 수 - 1 (0 기반)
    __le16 control;      // CDW12[31:16]: FUA, LR 등
    __le32 dsmgmt;       // CDW13: 접근 패턴 힌트
    __le32 reftag;       // CDW14: 참조 태그 (T10-PI)
    __le16 lbat;         // CDW15[15:0]: Application Tag
    __le16 lbatm;        // CDW15[31:16]: Application Tag Mask
};
```

**Read 명령 예시** (4KB 읽기, LBA 1000번에서):
```
opcode     = 0x02 (Read)
nsid       = 1
slba       = 1000
length     = 0 (1블록 - 1 = 0, 즉 1블록 전송)
dptr.prp1  = 0x12345000 (호스트 메모리 DMA 주소)
dptr.prp2  = 0 (한 페이지면 PRP2 불필요)
control    = 0 (FUA 없음)
command_id = 42 (blk-mq가 할당한 tag)
```

### 4.3 union nvme_data_ptr — PRP vs SGL

데이터 포인터(`dptr`)는 두 가지 방식 중 하나를 사용한다.

```c
union nvme_data_ptr {
    struct {
        __le64 prp1;    // PRP Entry 1: 첫 번째 데이터 페이지 물리 주소
        __le64 prp2;    // PRP Entry 2: 두 번째 페이지 또는 PRP List 주소
    };
    struct nvme_sgl_desc sgl;    // SGL 디스크립터
};
```

**PRP 방식 (기본):**
```
데이터 < 4KB:
  PRP1 → 데이터 페이지
  PRP2 = 0 (미사용)

데이터 4KB ~ 8KB:
  PRP1 → 첫 번째 4KB 페이지
  PRP2 → 두 번째 4KB 페이지

데이터 > 8KB:
  PRP1 → 첫 번째 4KB 페이지
  PRP2 → PRP List 시작 주소
         ┌────────────────────────┐
         │ PRP List (4KB 페이지): │
         │ [0] → 데이터 페이지 2  │
         │ [1] → 데이터 페이지 3  │
         │ ...                    │
         │ [510] → 데이터 페이지  │
         │ [511] → 다음 PRP List  │  ← 체인 링크
         └────────────────────────┘
```

**SGL 방식 (선택적, 더 유연):**
```
SGL은 임의 주소와 길이를 지정할 수 있어 PRP보다 유연하다.
sgl_threshold(기본 32KB) 이상의 평균 세그먼트 크기일 때 사용한다.
SPDK는 컨트롤러가 지원하면 항상 SGL을 사용한다.
```

---

## 5. Doorbell 메커니즘 상세

### 5.1 Doorbell 레지스터 위치와 레이아웃

NVMe Doorbell 레지스터는 PCIe BAR0의 오프셋 0x1000부터 시작한다. 각 큐 쌍(SQ+CQ)은 두 개의 Doorbell 레지스터를 가진다.

```
BAR0 메모리 맵:
  0x0000  ┌──────────────────────────────┐
          │ CAP (8B)                     │  컨트롤러 능력
  0x0008  │ VS (4B)                      │  버전
  0x000C  │ INTMS, INTMC (8B)            │  인터럽트 마스크
  0x0014  │ CC (4B)                      │  컨트롤러 구성
  0x001C  │ CSTS (4B)                    │  컨트롤러 상태
  0x0024  │ AQA, ASQ, ACQ               │  Admin Queue 설정
          │ ...                          │
  0x1000  ├──────────────────────────────┤  ← Doorbell 시작
          │ SQ 0 Tail Doorbell (4B)      │  Admin SQ Tail
  +stride │ CQ 0 Head Doorbell (4B)      │  Admin CQ Head
  +stride │ SQ 1 Tail Doorbell (4B)      │  I/O SQ 1 Tail
  +stride │ CQ 1 Head Doorbell (4B)      │  I/O CQ 1 Head
  +stride │ SQ 2 Tail Doorbell (4B)      │  I/O SQ 2 Tail
  +stride │ CQ 2 Head Doorbell (4B)      │  I/O CQ 2 Head
          │ ...                          │
          └──────────────────────────────┘

stride = 4 << CAP.DSTRD  (보통 4바이트, DSTRD=0)
```

**Doorbell 주소 계산 공식:**
```
SQ y Tail Doorbell = BAR0 + 0x1000 + (2 * y) * stride
CQ y Head Doorbell = BAR0 + 0x1000 + (2 * y + 1) * stride
```

커널 코드에서의 구현 (`pci.c:315~323`):
```c
// SQ Tail Doorbell 인덱스 (stride 단위)
static inline unsigned int sq_idx(unsigned int qid, u32 stride)
{
    return qid * 2 * stride;
}

// CQ Head Doorbell 인덱스 (stride 단위)
static inline unsigned int cq_idx(unsigned int qid, u32 stride)
{
    return (qid * 2 + 1) * stride;
}
```

`nvme_queue->q_db`는 해당 큐의 SQ Tail Doorbell을 가리키고, `q_db + dev->db_stride`가 CQ Head Doorbell이다.

### 5.2 SQ Tail Doorbell 쓰기 — nvme_write_sq_db()

I/O 제출의 마지막 단계는 SQ Tail Doorbell에 새 tail 값을 쓰는 것이다. 이것이 컨트롤러에게 "새 커맨드가 있다"고 알리는 유일한 방법이다.

```c
// pci.c:808
static inline void nvme_write_sq_db(struct nvme_queue *nvmeq, bool write_sq)
{
    // 배치 최적화: write_sq=false이면 Doorbell 쓰기를 지연시킨다
    if (!write_sq) {
        u16 next_tail = nvmeq->sq_tail + 1;
        if (next_tail == nvmeq->q_depth)
            next_tail = 0;
        // 아직 wrap하지 않았으면 Doorbell 쓰기 생략
        if (next_tail != nvmeq->last_sq_tail)
            return;
    }

    // Shadow Doorbell: MMIO가 정말 필요한지 확인
    if (nvme_dbbuf_update_and_check_event(nvmeq->sq_tail,
            nvmeq->dbbuf_sq_db, nvmeq->dbbuf_sq_ei))
        writel(nvmeq->sq_tail, nvmeq->q_db);  // ★ MMIO 쓰기!
    nvmeq->last_sq_tail = nvmeq->sq_tail;
}
```

**동작 흐름도:**
```
nvme_write_sq_db(nvmeq, write_sq) 호출
  │
  ├── write_sq == false?
  │     ├── YES: 배치 모드
  │     │     └── next_tail이 큐를 wrap하지 않으면 → return (지연)
  │     │         (Doorbell 안 울림, 다음 커맨드에서 같이 처리)
  │     └── NO: 즉시 쓰기 모드 (계속 진행)
  │
  ├── Shadow Doorbell 활성화됨?
  │     ├── YES: nvme_dbbuf_update_and_check_event() 호출
  │     │     ├── shadow DB에 새 값 쓰기 (일반 메모리 쓰기, 빠름)
  │     │     ├── Event Index 확인
  │     │     ├── MMIO 필요? → writel() 수행
  │     │     └── MMIO 불필요? → return (shadow만으로 충분)
  │     │
  │     └── NO: 무조건 writel() 수행
  │
  └── last_sq_tail = sq_tail 업데이트
```

### 5.3 CQ Head Doorbell 쓰기 — nvme_ring_cq_doorbell()

CQ의 완료 엔트리를 처리한 후, 호스트는 CQ Head Doorbell을 울려서 "여기까지 읽었다"고 컨트롤러에게 알린다. 컨트롤러는 이 정보로 CQ의 남은 공간을 파악한다.

```c
// pci.c:1852
static inline void nvme_ring_cq_doorbell(struct nvme_queue *nvmeq)
{
    u16 head = nvmeq->cq_head;

    if (nvme_dbbuf_update_and_check_event(head, nvmeq->dbbuf_cq_db,
                                          nvmeq->dbbuf_cq_ei))
        writel(head, nvmeq->q_db + nvmeq->dev->db_stride);
        //                        ^^^^^^^^^^^^^^^^^^^^^^^^
        //         SQ Tail DB 다음 위치 = CQ Head DB
}
```

### 5.4 Shadow Doorbell 최적화 — nvme_dbbuf_update_and_check_event()

NVMe 1.3에서 도입된 Shadow Doorbell Buffer는 불필요한 MMIO 쓰기를 줄여 성능을 향상시킨다.

MMIO writel()은 PCIe Posted Write 트랜잭션을 발생시키며, CPU 파이프라인을 잠시 멈출 수 있다. Shadow Doorbell을 사용하면 대부분의 경우 일반 메모리 쓰기만으로 충분하고, 컨트롤러가 "지금 MMIO가 필요하다"고 알릴 때만 실제 MMIO를 수행한다.

```c
// pci.c:592
static bool nvme_dbbuf_update_and_check_event(u16 value, __le32 *dbbuf_db,
                                              volatile __le32 *dbbuf_ei)
{
    if (dbbuf_db) {
        u16 old_value, event_idx;

        // 1. wmb(): SQ/CQ 메모리 쓰기가 완료된 후 shadow DB를 업데이트
        //    (커맨드 데이터가 메모리에 기록된 후에 Doorbell 값이 갱신되어야 함)
        wmb();

        // 2. shadow DB에 새 값 쓰기 (일반 메모리 쓰기 = 빠름)
        old_value = le32_to_cpu(*dbbuf_db);
        *dbbuf_db = cpu_to_le32(value);

        // 3. mb(): shadow DB 업데이트 후 event index를 읽는 순서 보장
        //    컨트롤러도 event index 업데이트와 shadow DB 읽기 사이에 배리어가 필요
        mb();

        // 4. event index를 읽어서 MMIO가 필요한지 판단
        //    컨트롤러가 설정한 event_idx: "이 값에 도달하면 MMIO로 알려줘"
        event_idx = le32_to_cpu(*dbbuf_ei);
        if (!nvme_dbbuf_need_event(event_idx, value, old_value))
            return false;  // MMIO 불필요! shadow 업데이트만으로 충분
    }

    return true;  // MMIO writel() 필요 (또는 shadow DB 미지원)
}
```

**Shadow Doorbell 동작 원리 ASCII 다이어그램:**
```
   호스트 메모리                          컨트롤러
  ┌────────────────┐                    ┌──────────────┐
  │ Shadow DB      │◀─── DMA 읽기 ─────│              │
  │ (dbbuf_dbs)    │                    │  NVMe 컨트롤 │
  │                │                    │  러 펌웨어    │
  │ Event Index    │──── DMA 쓰기 ────▶│              │
  │ (dbbuf_eis)    │                    │              │
  └────────────────┘                    └──────┬───────┘
                                               │
  호스트 동작:                                  │
  1. shadow DB에 새 tail 값 쓰기 (빠름)        │
  2. event index 확인                          │
  3. event 조건 충족? → writel() (MMIO)        │
     조건 미충족?   → 생략 (절약!)             │
                                               ▼
                                        BAR0 Doorbell
                                        (MMIO 레지스터)
```

---

## 6. Completion 경로 (인터럽트 → 완료)

### 6.1 전체 완료 경로 다이어그램

```
  NVMe SSD 컨트롤러
  │
  │  1. 커맨드 처리 완료
  │  2. CQ에 완료 엔트리(CQE) DMA 쓰기 (16바이트)
  │  3. MSI-X 인터럽트 발생
  │
  ▼
  CPU 인터럽트 벡터 (해당 큐에 바인딩된 CPU)
  │
  ├── [일반 모드] ─────────────────────────────────────┐
  │   nvme_irq() (hardirq context)                     │
  │   │                                                 │
  │   ├── DEFINE_IO_COMP_BATCH(iob)  ← 배치 완료 변수  │
  │   ├── nvme_poll_cq(nvmeq, &iob) ← CQ 순회          │
  │   │   │                                             │
  │   │   ├── while (nvme_cqe_pending(nvmeq))           │
  │   │   │     Phase Tag 비교: CQE.status[0] == cq_phase?
  │   │   │     │                                       │
  │   │   │     ├── dma_rmb()  ← DMA 읽기 배리어       │
  │   │   │     │   (Phase 확인 후 CQE 나머지 필드를    │
  │   │   │     │    안전하게 읽기 위한 순서 보장)       │
  │   │   │     │                                       │
  │   │   │     ├── nvme_handle_cqe()  ← 개별 CQE 처리 │
  │   │   │     │   ├── command_id = CQE.command_id     │
  │   │   │     │   ├── req = nvme_find_rq(tagset, cid) │
  │   │   │     │   │   (tag로 원래 request를 찾음)     │
  │   │   │     │   ├── nvme_try_complete_req()         │
  │   │   │     │   │   (status, result를 req에 저장)   │
  │   │   │     │   └── blk_mq_add_to_batch() 또는     │
  │   │   │     │       nvme_pci_complete_rq()          │
  │   │   │     │                                       │
  │   │   │     └── nvme_update_cq_head()               │
  │   │   │         cq_head++, wrap 시 phase ^= 1      │
  │   │   │                                             │
  │   │   └── nvme_ring_cq_doorbell()                   │
  │   │       (CQ Head DB 쓰기 → 컨트롤러에 알림)       │
  │   │                                                 │
  │   └── nvme_pci_complete_batch(&iob)                 │
  │       (배치로 모은 완료들을 한 번에 처리)             │
  │       └── 각 req: nvme_pci_unmap_rq() + blk-mq 완료│
  │                                                     │
  ├── [스레드 인터럽트 모드] ───────────────────────────┤
  │   nvme_irq_check() (hardirq, 최소 작업만)           │
  │   └── CQE 있으면 IRQ_WAKE_THREAD                   │
  │       → nvme_irq() (thread context에서 실행)        │
  │                                                     │
  └── [폴링 모드] ─────────────────────────────────────┤
      nvme_poll() (io_uring IOPOLL에서 호출)            │
      └── nvme_poll_cq(nvmeq, iob)                     │
          (인터럽트 없이 CPU가 직접 CQ를 확인)           │
```

### 6.2 CQE (Completion Queue Entry) 구조체

CQE는 16바이트 고정 크기이며, 컨트롤러가 DMA로 호스트 메모리에 기록한다.

```c
// include/linux/nvme.h:2711
struct nvme_completion {
    union nvme_result result;  // [7:0]  명령별 결과값 (u16/u32/u64)
    __le16 sq_head;            // [9:8]  SQ에서 컨트롤러가 처리한 위치
    __le16 sq_id;              // [11:10] 이 CQE를 생성한 SQ ID
    __u16  command_id;         // [13:12] 완료된 명령의 Command ID
    __le16 status;             // [15:14] 상태 코드 + Phase Tag
};
```

```
CQE 바이트 레이아웃 (16바이트):
 ┌─────────┬──────┬──────────────────────────────────────────┐
 │ [7:0]   │  8B  │ Command Specific (result)                 │
 │         │      │ - Read/Write: 미사용 (0)                  │
 │         │      │ - Admin 명령: 반환값 포함                  │
 ├─────────┼──────┼──────────────────────────────────────────┤
 │ [9:8]   │  2B  │ SQ Head Pointer (SQHD)                   │
 │         │      │ 컨트롤러가 SQ에서 처리한 마지막 위치       │
 │         │      │ 호스트는 이 값으로 SQ 공간을 재활용        │
 ├─────────┼──────┼──────────────────────────────────────────┤
 │ [11:10] │  2B  │ SQ Identifier (SQID)                     │
 │         │      │ 이 완료가 어느 SQ에서 온 것인지            │
 ├─────────┼──────┼──────────────────────────────────────────┤
 │ [13:12] │  2B  │ Command Identifier (CID)                 │
 │         │      │ SQE 제출 시 호스트가 부여한 값             │
 │         │      │ = blk-mq tag (request를 찾는 키)          │
 ├─────────┼──────┼──────────────────────────────────────────┤
 │ [15:14] │  2B  │ Status Field                             │
 │         │      │ 비트 0: Phase Tag (P)                     │
 │         │      │ 비트 8:1: Status Code (SC)                │
 │         │      │ 비트 11:9: Status Code Type (SCT)         │
 │         │      │ 비트 14: More (M) - 추가 상태 있음        │
 │         │      │ 비트 15: Do Not Retry (DNR)               │
 └─────────┴──────┴──────────────────────────────────────────┘
```

### 6.3 Phase Tag 동작 원리

Phase Tag는 CQ에서 새 엔트리와 이전 엔트리를 구분하는 핵심 메커니즘이다.

```
초기 상태: cq_phase = 1, CQ 모든 엔트리의 phase = 0

라운드 1 (phase=1):
  CQ: [P=1][P=1][P=1][P=0][P=0]...
       ↑         ↑
     완료됨    cq_head    ← P==cq_phase(1)이면 새 엔트리

라운드 2 (phase=0, CQ wrap 후):
  CQ: [P=0][P=0][P=1][P=1][P=1]...
       ↑         ↑
     완료됨    cq_head    ← P==cq_phase(0)이면 새 엔트리

phase 토글 시점:
  cq_head이 q_depth에 도달하면 → cq_head = 0, cq_phase ^= 1
```

```c
// pci.c:1838 - CQE가 있는지 확인
static inline bool nvme_cqe_pending(struct nvme_queue *nvmeq)
{
    struct nvme_completion *hcqe = &nvmeq->cqes[nvmeq->cq_head];
    return (le16_to_cpu(READ_ONCE(hcqe->status)) & 1) == nvmeq->cq_phase;
    //                                              ^        ^^^^^^^^^
    //                                    Phase 비트(LSB)   호스트의 현재 phase
}
```

---

## 7. 핵심 함수 코드 주석 분석

### 7.1 nvme_queue_rq() — I/O 제출의 핵심 (pci.c:1675)

이 함수는 blk-mq가 NVMe 드라이버에 I/O 요청을 전달할 때 호출하는 가장 중요한 콜백이다. `blk_mq_ops.queue_rq`에 등록되어 있다.

```c
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
                         const struct blk_mq_queue_data *bd)
{
    // hctx->driver_data는 init_hctx에서 설정한 nvme_queue 포인터
    struct nvme_queue *nvmeq = hctx->driver_data;
    struct nvme_dev *dev = nvmeq->dev;
    struct request *req = bd->rq;       // 처리할 블록 요청
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);  // request 뒤의 PDU 영역
    blk_status_t ret;

    /*
     * [1단계] 큐 활성화 상태 확인
     * 컨트롤러 셧다운/리셋 중에는 큐가 비활성화된다.
     * NVMEQ_ENABLED 비트가 꺼져 있으면 즉시 에러를 반환한다.
     */
    if (unlikely(!test_bit(NVMEQ_ENABLED, &nvmeq->flags)))
        return BLK_STS_IOERR;

    /*
     * [2단계] 컨트롤러 상태 확인
     * 컨트롤러가 LIVE 또는 CONNECTING 상태가 아니면
     * 요청을 실패시키거나 재큐잉한다.
     */
    if (unlikely(!nvme_check_ready(&dev->ctrl, req, true)))
        return nvme_fail_nonready_command(&dev->ctrl, req);

    /*
     * [3단계] 요청 준비 (nvme_prep_rq)
     * - nvme_setup_cmd(): block 요청 → NVMe 커맨드 변환
     *   (opcode, nsid, slba, length, command_id 설정)
     * - nvme_map_data(): scatter-gather → PRP/SGL DMA 매핑
     * - nvme_start_request(): blk-mq에 요청 시작 알림 (타임아웃 타이머)
     */
    ret = nvme_prep_rq(req);
    if (unlikely(ret))
        return ret;

    /*
     * [4단계] SQ에 커맨드 복사 + Doorbell 쓰기 (critical section)
     *
     * sq_lock으로 보호하는 이유:
     * 같은 큐에 여러 CPU가 동시에 커맨드를 제출할 수 있다.
     * SQ tail 포인터와 Doorbell 쓰기는 원자적이어야 한다.
     *
     * bd->last:
     * blk-mq가 설정하는 "이 배치의 마지막 요청" 플래그.
     * - true: 즉시 Doorbell을 울린다
     * - false: Doorbell 쓰기를 지연시킨다 (배치 최적화)
     *          → 여러 커맨드를 한 번의 Doorbell로 알림
     */
    spin_lock(&nvmeq->sq_lock);
    nvme_sq_copy_cmd(nvmeq, &iod->cmd);    // SQ[tail]에 64B 복사, tail++
    nvme_write_sq_db(nvmeq, bd->last);      // Doorbell 쓰기 (조건부)
    spin_unlock(&nvmeq->sq_lock);
    return BLK_STS_OK;
}
```

**SPDK와의 비교:**
```
커널 NVMe:
  blk-mq → nvme_queue_rq() → nvme_sq_copy_cmd() → writel(doorbell)
  - 커널 컨텍스트에서 실행
  - spinlock으로 동기화
  - blk-mq의 태그 관리, 타임아웃, 에러 복구 지원

SPDK:
  spdk_nvme_ns_cmd_read() → nvme_pcie_qpair_submit_request()
  - 사용자 공간에서 직접 실행
  - 락 없음 (단일 스레드 모델)
  - 사용자가 직접 qpair을 mmio_remap으로 접근
```

### 7.2 nvme_sq_copy_cmd() — SQ에 커맨드 복사 (pci.c:835)

```c
/*
 * NVMe 커맨드(64바이트)를 SQ의 현재 Tail 위치에 복사하고
 * Tail 포인터를 전진시킨다.
 *
 * SQ는 링 버퍼이므로 Tail이 q_depth에 도달하면 0으로 wrap한다.
 *
 * 메모리 레이아웃:
 *   sq_cmds[0]              ← SQ Entry 0 (64바이트)
 *   sq_cmds[64]             ← SQ Entry 1
 *   ...
 *   sq_cmds[sq_tail << 6]   ← 현재 쓸 위치 (sqes=6일 때)
 */
static inline void nvme_sq_copy_cmd(struct nvme_queue *nvmeq,
                                    struct nvme_command *cmd)
{
    // sq_cmds 버퍼의 sq_tail 번째 슬롯에 64바이트 커맨드를 복사한다.
    // sq_tail << sqes = sq_tail * 64 (sqes=6일 때)
    memcpy(nvmeq->sq_cmds + (nvmeq->sq_tail << nvmeq->sqes),
           absolute_pointer(cmd), sizeof(*cmd));

    // tail 포인터 전진 + wrap-around
    if (++nvmeq->sq_tail == nvmeq->q_depth)
        nvmeq->sq_tail = 0;
}
```

```
SQ 링 버퍼 동작:
  Before:
    [CMD][CMD][ ← tail ][   ][   ][   ]
                 ↑
              여기에 새 커맨드 복사

  After:
    [CMD][CMD][NEW][ ← tail ][   ][   ]
                     ↑
                  tail이 전진

  Wrap-around (tail이 끝에 도달):
    [   ][   ][CMD][CMD][CMD][NEW ← tail]
     ↑
    tail = 0으로 리셋
```

### 7.3 nvme_write_sq_db() — Doorbell 쓰기 (pci.c:808)

```c
/*
 * SQ에 커맨드를 복사한 후 Doorbell을 울려서 디바이스에 알리는 함수.
 *
 * 배치 최적화:
 * - write_sq=false이면 Doorbell 쓰기를 지연시킨다
 * - 단, 다음 커맨드가 큐를 wrap하면 반드시 쓴다
 *
 * Shadow Doorbell 최적화:
 * - shadow DB를 업데이트하고 Event Index를 확인하여
 *   MMIO가 필요한 경우에만 writel()을 수행한다
 */
static inline void nvme_write_sq_db(struct nvme_queue *nvmeq, bool write_sq)
{
    /*
     * write_sq == false: 배치의 중간 커맨드
     * Doorbell 쓰기를 지연시킬 수 있는지 확인한다.
     *
     * 지연 조건: next_tail이 last_sq_tail을 따라잡지 않으면 안전
     * 따라잡으면: SQ가 한 바퀴 도는 것이므로 반드시 Doorbell을 써야 한다
     */
    if (!write_sq) {
        u16 next_tail = nvmeq->sq_tail + 1;

        if (next_tail == nvmeq->q_depth)
            next_tail = 0;            // wrap-around 처리
        if (next_tail != nvmeq->last_sq_tail)
            return;                    // 아직 안전 → Doorbell 지연
    }

    /*
     * nvme_dbbuf_update_and_check_event():
     *   1. shadow DB에 sq_tail 값을 쓴다 (일반 메모리 쓰기)
     *   2. event index를 읽어서 MMIO 필요 여부를 판단한다
     *   3. true 반환: MMIO writel() 필요
     *      false 반환: shadow만으로 충분 (MMIO 생략)
     *
     * shadow DB가 없는 경우 (dbbuf_sq_db == NULL):
     *   항상 true를 반환하여 무조건 writel()을 수행한다
     */
    if (nvme_dbbuf_update_and_check_event(nvmeq->sq_tail,
            nvmeq->dbbuf_sq_db, nvmeq->dbbuf_sq_ei))
        writel(nvmeq->sq_tail, nvmeq->q_db);
        // ★ writel(): MMIO 쓰기
        // nvmeq->q_db = BAR0 + 0x1000 + (2 * qid * stride)
        // PCIe Posted Write로 컨트롤러에 새 tail 값 전달

    nvmeq->last_sq_tail = nvmeq->sq_tail;
    // 다음 배치 Doorbell 지연 판단을 위해 기록
}
```

### 7.4 nvme_irq() — 인터럽트 핸들러 (pci.c:1995)

```c
/*
 * MSI-X 인터럽트가 발생하면 커널이 호출하는 NVMe 인터럽트 핸들러.
 * hardirq 컨텍스트에서 실행된다 (스레드 모드가 아닌 경우).
 *
 * 각 I/O 큐는 보통 별도의 MSI-X 벡터를 가지며,
 * 그 벡터는 특정 CPU에 바인딩된다.
 *
 * 전체 인터럽트 경로:
 *   SSD → MSI-X 인터럽트 → CPU → 커널 IRQ 프레임워크 → nvme_irq()
 */
static irqreturn_t nvme_irq(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;  // 인터럽트 등록 시 전달한 큐 포인터
    DEFINE_IO_COMP_BATCH(iob);        // 배치 완료를 위한 스택 변수

    /*
     * nvme_poll_cq(): CQ를 순회하며 모든 대기 CQE를 처리한다.
     * 반환값: true이면 하나 이상의 CQE를 처리했음
     */
    if (nvme_poll_cq(nvmeq, &iob)) {
        /*
         * 배치로 모은 완료들이 있으면 한 번에 처리한다.
         * 배치 처리의 이점:
         * - DMA unmap을 한 번에 모아서 처리 (IOMMU 효율)
         * - blk-mq 완료 통지를 일괄 수행
         */
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob);
        return IRQ_HANDLED;  // 인터럽트를 처리했음
    }
    return IRQ_NONE;  // 이 큐에 처리할 CQE가 없었음
                       // (공유 인터럽트일 때 다른 핸들러에 양보)
}
```

### 7.5 nvme_poll_cq() — CQ 순회 (pci.c:1955)

```c
/*
 * CQ를 순회하며 모든 대기 중인 완료 엔트리를 처리한다.
 * 인터럽트 핸들러(nvme_irq)와 폴링 경로(nvme_poll) 모두에서 호출된다.
 *
 * SPDK의 spdk_nvme_qpair_process_completions()와 동일한 역할이다.
 * 차이점: SPDK는 이 함수를 반복적으로 폴링하고,
 *         커널은 인터럽트가 트리거할 때 호출한다 (poll 큐 제외).
 */
static inline bool nvme_poll_cq(struct nvme_queue *nvmeq,
                                struct io_comp_batch *iob)
{
    bool found = false;

    /*
     * CQ를 순회: Phase Tag로 새 CQE가 있는지 반복 확인
     */
    while (nvme_cqe_pending(nvmeq)) {
        found = true;

        /*
         * dma_rmb(): DMA 읽기 메모리 배리어
         *
         * Phase가 유효하다는 것은 CQE 전체가 DMA로 기록 완료되었다는 뜻이다.
         * 하지만 CPU의 메모리 순서 재배치로 인해 Phase를 먼저 읽고
         * CQE의 다른 필드(command_id, status 등)를 나중에 읽을 때
         * 아직 이전 값이 보일 수 있다.
         *
         * dma_rmb()는 이 배리어 이전의 읽기(Phase)가 완료된 후에만
         * 이후의 읽기(나머지 CQE 필드)가 수행되도록 보장한다.
         *
         * ARM/RISC-V에서는 실제 배리어 명령어로 변환되고,
         * x86에서는 컴파일러 배리어만 수행 (하드웨어가 순서 보장).
         */
        dma_rmb();

        /* 개별 CQE 처리: command_id로 request를 찾아 완료 */
        nvme_handle_cqe(nvmeq, iob, nvmeq->cq_head);

        /* CQ Head 전진 + wrap 시 Phase 토글 */
        nvme_update_cq_head(nvmeq);
    }

    /* 하나 이상의 CQE를 처리했으면 CQ Head Doorbell을 울린다 */
    if (found)
        nvme_ring_cq_doorbell(nvmeq);

    return found;
}
```

### 7.6 nvme_handle_cqe() — 개별 CQE 처리 (pci.c:1883)

```c
/*
 * CQ의 개별 완료 엔트리(CQE)를 처리하는 함수.
 *
 * CQE에서 command_id를 읽어 원래 요청(struct request)을 찾고 완료시킨다.
 * command_id는 SQE 제출 시 blk-mq가 할당한 tag 값과 동일하다.
 *
 * command_id → request 매핑은 blk-mq의 tag 시스템을 사용한다.
 * tag은 0부터 q_depth-1까지의 정수이고, tag → request 배열이 미리 할당되어 있다.
 */
static inline void nvme_handle_cqe(struct nvme_queue *nvmeq,
                                   struct io_comp_batch *iob, u16 idx)
{
    struct nvme_completion *cqe = &nvmeq->cqes[idx];  // CQ[idx]의 CQE
    __u16 command_id = READ_ONCE(cqe->command_id);     // CID 읽기
    struct request *req;

    /*
     * AEN(Asynchronous Event Notification) 특별 처리
     * AEN은 컨트롤러가 비동기 이벤트(에러, 상태 변화)를 알리기 위해 사용한다.
     * AEN 요청은 struct request를 할당하지 않으므로 별도로 처리한다.
     */
    if (unlikely(nvme_is_aen_req(nvmeq->qid, command_id))) {
        nvme_complete_async_event(&nvmeq->dev->ctrl,
                cqe->status, &cqe->result);
        return;
    }

    /*
     * command_id(= blk-mq tag)로 원래 request를 찾는다.
     * nvme_find_rq()는 tag → request 매핑 테이블을 조회한다.
     */
    req = nvme_find_rq(nvme_queue_tagset(nvmeq), command_id);
    if (unlikely(!req)) {
        dev_warn(nvmeq->dev->ctrl.device,
            "invalid id %d completed on queue %d\n",
            command_id, le16_to_cpu(cqe->sq_id));
        return;
    }

    /* 트레이스포인트: 디버깅/성능 분석 도구에서 사용 */
    trace_nvme_sq(req, cqe->sq_head, nvmeq->sq_tail);

    /*
     * 요청 완료 처리 (3단계 시도):
     *
     * 1) nvme_try_complete_req(): CQE의 status/result를 request에 저장
     *    특수 완료(재시도, 에러 경로 등)가 필요하면 true를 반환하고 여기서 끝남
     *
     * 2) blk_mq_add_to_batch(): 배치 완료 리스트에 추가
     *    iob(io_comp_batch)에 request를 모아두고 나중에 한 번에 완료
     *    성공하면 true 반환
     *
     * 3) nvme_pci_complete_rq(): 즉시 완료
     *    배치에 추가할 수 없으면 바로 DMA unmap + blk-mq 완료
     */
    if (!nvme_try_complete_req(req, cqe->status, cqe->result) &&
        !blk_mq_add_to_batch(req, iob,
                     nvme_req(req)->status != NVME_SC_SUCCESS,
                     nvme_pci_complete_batch))
        nvme_pci_complete_rq(req);
}
```

### 7.7 nvme_update_cq_head() — CQ Head 전진 (pci.c:1924)

```c
/*
 * CQ Head를 다음 엔트리로 전진시킨다.
 * CQ가 한 바퀴 돌면 (head가 q_depth에 도달하면):
 * - head를 0으로 리셋 (wrap-around)
 * - phase를 토글 (0↔1)
 *
 * Phase 토글은 다음 라운드의 유효 CQE를 구분하기 위한 것이다.
 * 컨트롤러도 CQ를 한 바퀴 돌 때마다 Phase를 토글하므로,
 * 호스트의 cq_phase와 CQE의 phase가 일치하면 새 엔트리이다.
 */
static inline void nvme_update_cq_head(struct nvme_queue *nvmeq)
{
    u32 tmp = nvmeq->cq_head + 1;

    if (tmp == nvmeq->q_depth) {
        nvmeq->cq_head = 0;       // wrap-around
        nvmeq->cq_phase ^= 1;     // phase 토글!
    } else {
        nvmeq->cq_head = tmp;     // 단순 전진
    }
}
```

### 7.8 nvme_prep_rq() — 요청 준비 (pci.c:1598)

```c
/*
 * blk-mq 요청을 NVMe 커맨드로 변환하는 핵심 준비 함수.
 * nvme_queue_rq()에서 SQ에 복사하기 전에 호출된다.
 */
static blk_status_t nvme_prep_rq(struct request *req)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
    blk_status_t ret;

    /* [1] I/O 디스크립터 초기화 */
    iod->flags = 0;
    iod->nr_descriptors = 0;
    iod->total_len = 0;
    iod->meta_total_len = 0;
    iod->nr_dma_vecs = 0;

    /* [2] block 요청 → NVMe 커맨드 변환
     * nvme_setup_cmd()는 core.c에 있으며, req_op(req)에 따라:
     *   REQ_OP_READ  → nvme_setup_rw(nvme_cmd_read)
     *   REQ_OP_WRITE → nvme_setup_rw(nvme_cmd_write)
     *   REQ_OP_FLUSH → nvme_setup_flush()
     *   REQ_OP_DISCARD → nvme_setup_discard()
     *   등
     * opcode, nsid, slba, length, command_id를 설정한다.
     */
    ret = nvme_setup_cmd(req->q->queuedata, req);
    if (ret)
        return ret;

    /* [3] 데이터가 있으면 DMA 매핑 (PRP/SGL 설정)
     * blk_rq_nr_phys_segments()는 scatter-gather 세그먼트 수를 반환한다.
     * 0이면 데이터 없는 커맨드(Flush 등)이므로 DMA 매핑 불필요.
     */
    if (blk_rq_nr_phys_segments(req)) {
        ret = nvme_map_data(req);
        if (ret)
            goto out_free_cmd;
    }

    /* [4] 메타데이터(T10-PI 등)가 있으면 메타데이터 DMA 매핑 */
    if (blk_integrity_rq(req)) {
        ret = nvme_map_metadata(req);
        if (ret)
            goto out_unmap_data;
    }

    /* [5] blk-mq에 요청 시작 알림
     * - 요청의 타임아웃 타이머를 시작한다
     * - 요청 상태를 MQ_RQ_IN_FLIGHT로 전환한다
     * - 이후부터 타임아웃 핸들러가 이 요청을 감시한다
     */
    nvme_start_request(req);
    return BLK_STS_OK;

out_unmap_data:
    if (blk_rq_nr_phys_segments(req))
        nvme_unmap_data(req);
out_free_cmd:
    nvme_cleanup_cmd(req);
    return ret;
}
```

### 7.9 nvme_commit_rqs() — 지연된 Doorbell flush (pci.c:849)

```c
/*
 * blk-mq 콜백: blk_mq_ops.commit_rqs
 *
 * blk-mq가 배치 제출 후 "이제 정말 디바이스에 알려라"라고 할 때 호출된다.
 * nvme_queue_rq()에서 bd->last=false로 Doorbell을 지연시킨 경우,
 * 이 함수가 최종적으로 Doorbell을 울린다.
 *
 * 호출 시점:
 * - blk_mq_dispatch_rq_list()에서 리소스 부족으로 중단될 때
 * - plug flush 후 아직 Doorbell을 안 울렸을 때
 */
static void nvme_commit_rqs(struct blk_mq_hw_ctx *hctx)
{
    struct nvme_queue *nvmeq = hctx->driver_data;

    spin_lock(&nvmeq->sq_lock);
    // sq_tail과 last_sq_tail이 다르면 = 아직 Doorbell을 안 울린 커맨드가 있다
    if (nvmeq->sq_tail != nvmeq->last_sq_tail)
        nvme_write_sq_db(nvmeq, true);  // 강제 Doorbell 쓰기
    spin_unlock(&nvmeq->sq_lock);
}
```

### 7.10 nvme_pci_complete_rq() — 단일 요청 완료 (pci.c:1812)

```c
/*
 * 단일 요청 완료: DMA unmap 후 blk-mq에 완료 통지
 */
static void nvme_pci_complete_rq(struct request *req)
{
    // DMA 매핑 해제 (메타데이터 + 데이터)
    nvme_pci_unmap_rq(req);
    // blk-mq에 요청 완료를 알린다
    // → blk-mq가 tag를 해제하고 상위 계층 콜백을 호출
    // → 최종적으로 bio->bi_end_io() → 사용자에게 완료 통보
    nvme_complete_rq(req);
}

static __always_inline void nvme_pci_unmap_rq(struct request *req)
{
    if (blk_integrity_rq(req))
        nvme_unmap_metadata(req);    // 메타데이터 DMA 해제
    if (blk_rq_nr_phys_segments(req))
        nvme_unmap_data(req);        // 데이터 DMA 해제
}
```

---

## 8. 배치 제출 최적화 — nvme_queue_rqs() (pci.c:1763)

blk-mq는 여러 요청을 한 번에 드라이버에 전달하는 `queue_rqs` 콜백을 지원한다. NVMe 드라이버는 이를 활용하여 같은 큐의 요청들을 모아 한 번의 Doorbell로 처리한다.

```c
/*
 * blk-mq 배치 I/O 제출 콜백 (blk_mq_ops.queue_rqs)
 *
 * nvme_queue_rq()를 개별 호출하는 것보다 효율적인 이유:
 * 1. 같은 큐의 요청들을 모아서 한 번의 Doorbell로 처리
 * 2. spinlock 획득/해제 횟수를 줄임
 * 3. MMIO writel() 횟수를 줄임
 */
static void nvme_queue_rqs(struct rq_list *rqlist)
{
    struct rq_list submit_list = { };    // 제출할 요청 리스트
    struct rq_list requeue_list = { };   // 실패한 요청 리스트
    struct nvme_queue *nvmeq = NULL;
    struct request *req;

    while ((req = rq_list_pop(rqlist))) {
        // 큐가 바뀌면 이전 큐의 요청들을 먼저 제출
        if (nvmeq && nvmeq != req->mq_hctx->driver_data)
            nvme_submit_cmds(nvmeq, &submit_list);
        nvmeq = req->mq_hctx->driver_data;

        // 요청 준비 성공하면 submit_list에, 실패하면 requeue_list에
        if (nvme_prep_rq_batch(nvmeq, req))
            rq_list_add_tail(&submit_list, req);
        else
            rq_list_add_tail(&requeue_list, req);
    }

    // 마지막 큐의 남은 요청 제출
    if (nvmeq)
        nvme_submit_cmds(nvmeq, &submit_list);

    *rqlist = requeue_list;  // 실패한 요청은 blk-mq에 반환
}

/*
 * 준비된 커맨드 리스트를 한꺼번에 SQ에 복사하고 Doorbell을 한 번만 울린다.
 */
static void nvme_submit_cmds(struct nvme_queue *nvmeq, struct rq_list *rqlist)
{
    struct request *req;

    if (rq_list_empty(rqlist))
        return;

    spin_lock(&nvmeq->sq_lock);       // 한 번만 락
    while ((req = rq_list_pop(rqlist))) {
        struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
        nvme_sq_copy_cmd(nvmeq, &iod->cmd);  // 모든 커맨드를 SQ에 복사
    }
    nvme_write_sq_db(nvmeq, true);     // Doorbell 한 번만!
    spin_unlock(&nvmeq->sq_lock);      // 한 번만 언락
}
```

**배치 최적화 효과:**
```
개별 제출 (nvme_queue_rq × N):
  lock → copy → doorbell → unlock    ×N번
  = N번의 lock/unlock + N번의 MMIO writel()

배치 제출 (nvme_queue_rqs):
  lock → copy × N → doorbell → unlock
  = 1번의 lock/unlock + 1번의 MMIO writel()
```

---

## 9. blk-mq 계층의 핵심 동작

### 9.1 submit_bio() → blk_mq_submit_bio() 경로

`block/blk-core.c`의 `submit_bio()`가 모든 블록 I/O의 최상위 진입점이다.

```
submit_bio(bio)                         ← blk-core.c:1244
  │
  └─ submit_bio_noacct(bio)             ← 유효성 검사
       │ - 파티션 리매핑 (파티션 오프셋 → 실제 디바이스 오프셋)
       │ - 크기/정렬 검사
       │ - 읽기 전용 디바이스에 쓰기 방지
       │
       └─ submit_bio_noacct_nocheck()   ← cgroup 회계, 트레이싱
            │
            └─ __submit_bio_noacct_mq() ← blk-mq 전용 (NVMe가 이 경로)
                 │  재귀 방지 루프:
                 │  bio_list에 bio를 넣고 하나씩 꺼내서 처리
                 │
                 └─ __submit_bio()      ← 실제 디스패치
                      │
                      └─ blk_mq_submit_bio(bio)  ← blk-mq.c:3697
```

### 9.2 blk_mq_submit_bio()의 핵심 동작

```
blk_mq_submit_bio(bio)
  │
  ├─ [1] bio 분할: __bio_split_to_limits()
  │   큐의 max_sectors, max_segments 제한에 맞게 분할
  │
  ├─ [2] 병합 시도
  │   기존 request에 이 bio를 합칠 수 있는지 확인
  │   └─ plug 리스트, 소프트웨어 큐, 스케줄러 큐에서 검색
  │
  ├─ [3] 새 request 할당 (병합 실패 시)
  │   blk_mq_get_new_requests() → tag 할당 + request 초기화
  │
  ├─ [4] 디스패치 경로 선택
  │   ├─ plug 있음 → blk_add_rq_to_plug()
  │   │   나중에 blk_flush_plug()에서 한꺼번에 디스패치
  │   │
  │   ├─ 스케줄러 있음 → blk_mq_sched_insert_request()
  │   │   스케줄러(mq-deadline 등)가 순서를 최적화
  │   │
  │   └─ 직접 디스패치 → blk_mq_try_issue_directly()
  │       스케줄러 없고 plug 없으면 즉시 드라이버에 전달
  │
  └─ 최종적으로 blk_mq_dispatch_rq_list() 호출
       → q->mq_ops->queue_rq(hctx, &bd)
       → nvme_queue_rq()
```

### 9.3 blk_mq_dispatch_rq_list() (blk-mq.c:2502)

이 함수가 request 리스트를 NVMe 드라이버에 실제로 전달하는 마지막 blk-mq 단계이다.

```c
bool blk_mq_dispatch_rq_list(struct blk_mq_hw_ctx *hctx,
                             struct list_head *list, bool get_budget)
{
    struct request_queue *q = hctx->queue;
    struct request *rq;
    int queued;
    blk_status_t ret = BLK_STS_OK;

    queued = 0;
    do {
        struct blk_mq_queue_data bd;

        rq = list_first_entry(list, struct request, queuelist);

        // driver tag 확보 (command_id로 사용됨)
        prep = blk_mq_prep_dispatch_rq(rq, get_budget);
        if (prep != PREP_DISPATCH_OK)
            break;

        list_del_init(&rq->queuelist);

        bd.rq = rq;
        bd.last = list_empty(list);  // ★ 마지막 요청이면 true
        // NVMe는 이 값으로 Doorbell write 여부를 결정

        // ★★★ 드라이버 호출! nvme_queue_rq()로 진입
        ret = q->mq_ops->queue_rq(hctx, &bd);

        switch (ret) {
        case BLK_STS_OK:
            queued++;
            break;
        case BLK_STS_RESOURCE:
        case BLK_STS_DEV_RESOURCE:
            // 리소스 부족: 나중에 재시도
            blk_mq_handle_dev_resource(rq, list);
            goto out;
        default:
            // 에러: 요청을 에러로 완료
            blk_mq_end_request(rq, ret);
        }
    } while (!list_empty(list));

out:
    // 리스트에 남은 요청이 있으면 지연된 Doorbell을 flush
    if (!list_empty(list) || ret != BLK_STS_OK)
        blk_mq_commit_rqs(hctx, queued, false);
    // → nvme_commit_rqs() 호출 → 남은 Doorbell 쓰기
    ...
}
```

---

## 10. ioctl 경로 상세 — nvme-cli가 커맨드를 보내는 방법

### 10.1 nvme_dev_ioctl() (ioctl.c:840)

```c
long nvme_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct nvme_ctrl *ctrl = file->private_data;
    void __user *argp = (void __user *)arg;

    switch (cmd) {
    case NVME_IOCTL_ADMIN_CMD:
        // Admin 커맨드 (Identify, Get Log Page 등)
        return nvme_user_cmd(ctrl, NULL, argp, 0, open_for_write);
    case NVME_IOCTL_ADMIN64_CMD:
        // 64비트 결과를 지원하는 Admin 커맨드
        return nvme_user_cmd64(ctrl, NULL, argp, 0, open_for_write);
    case NVME_IOCTL_IO_CMD:
        // I/O 커맨드 (passthrough Read/Write)
        return nvme_dev_user_cmd(ctrl, argp, open_for_write);
    case NVME_IOCTL_RESET:
        return nvme_reset_ctrl_sync(ctrl);
    ...
    }
}
```

### 10.2 nvme_user_cmd() → nvme_submit_user_cmd() 흐름

```c
static int nvme_user_cmd(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
                         struct nvme_passthru_cmd __user *ucmd, ...)
{
    struct nvme_passthru_cmd cmd;
    struct nvme_command c;

    // [1] 사용자 공간에서 커맨드 구조체 복사
    copy_from_user(&cmd, ucmd, sizeof(cmd));

    // [2] NVMe 커맨드 수동 구성 (사용자가 지정한 값 그대로)
    memset(&c, 0, sizeof(c));
    c.common.opcode = cmd.opcode;
    c.common.nsid   = cpu_to_le32(cmd.nsid);
    c.common.cdw2[0] = cpu_to_le32(cmd.cdw2);
    c.common.cdw2[1] = cpu_to_le32(cmd.cdw3);
    c.common.cdw10  = cpu_to_le32(cmd.cdw10);
    // ... cdw11 ~ cdw15

    // [3] 권한 검사
    nvme_cmd_allowed(ns, &c, 0, open_for_write);

    // [4] 동기 실행
    status = nvme_submit_user_cmd(ns ? ns->queue : ctrl->admin_q,
                                  &c, cmd.addr, cmd.data_len, ...);
    // → blk_mq_alloc_request() → nvme_execute_rq()
    // → blk_execute_rq() → nvme_queue_rq() → Doorbell

    // [5] 결과를 사용자 공간에 복사
    put_user(result, &ucmd->result);
    return status;
}
```

### 10.3 fio 경로와 ioctl 경로의 request 생성 차이

```
fio 경로:
  bio 생성 (submit_bio)
  → blk_mq_submit_bio()에서 bio → request 변환
  → tag 할당 + nvme_setup_cmd()로 커맨드 자동 빌드
  → 병합 가능, 스케줄러 경유

ioctl 경로:
  blk_mq_alloc_request()로 request 직접 할당
  → 사용자가 커맨드를 직접 구성 (opcode, cdw10~15)
  → 병합 불가, 스케줄러 우회
  → blk_execute_rq()로 동기 실행
```

---

## 11. 추가 참고: 폴링 모드

### 11.1 nvme_poll() — io_uring 폴링 (pci.c:2047)

```c
/*
 * io_uring의 IORING_SETUP_IOPOLL 등에서 호출된다.
 * 인터럽트 없이 CPU가 직접 CQ를 확인하므로 인터럽트 지연을 완전히 제거한다.
 * SPDK의 전체 I/O 모델(사용자 공간 폴링)과 개념적으로 유사하다.
 */
static int nvme_poll(struct blk_mq_hw_ctx *hctx, struct io_comp_batch *iob)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    bool found;

    // 먼저 CQE가 있는지 빠르게 확인 (lock 없이)
    if (!nvme_cqe_pending(nvmeq))
        return 0;

    // CQE가 있으면 lock을 잡고 처리
    spin_lock(&nvmeq->cq_poll_lock);
    found = nvme_poll_cq(nvmeq, iob);
    spin_unlock(&nvmeq->cq_poll_lock);

    return found;
}
```

폴링 큐는 MSI-X 인터럽트가 없으므로(`NVMEQ_POLLED` 플래그), CPU가 주기적으로 `nvme_poll()`을 호출해야 한다. 인터럽트 지연(~1us)을 제거할 수 있어 초저지연 I/O에 유리하지만, CPU 사이클을 소모하는 트레이드오프가 있다.

---

## 12. 전체 경로 요약 — 한 장에 보는 I/O 라이프사이클

```
 ┌─────────────────────────────────────────────────────────────────────┐
 │  ① 제출 (Submission)                                                │
 │                                                                     │
 │  [APP] write(fd, buf, 4096)                                        │
 │    → [VFS] vfs_write()                                             │
 │      → [FS] ext4_file_write_iter() → submit_bio()                 │
 │        → [blk-mq] blk_mq_submit_bio()                             │
 │          → bio → request 변환, tag 할당                             │
 │            → blk_mq_dispatch_rq_list()                             │
 │              → nvme_queue_rq()                                     │
 │                → nvme_setup_cmd(): opcode=0x01, slba, length 설정  │
 │                → nvme_map_data(): PRP1/PRP2 설정 (DMA 주소)        │
 │                → nvme_sq_copy_cmd(): SQ[tail]에 64B 복사           │
 │                → nvme_write_sq_db(): writel(tail, doorbell)        │
 │                                                                     │
 └──────────────────────────────────┬──────────────────────────────────┘
                                    │ PCIe Posted Write (MMIO)
                                    ▼
 ┌─────────────────────────────────────────────────────────────────────┐
 │  ② 처리 (Processing by SSD)                                        │
 │                                                                     │
 │  [SSD] Doorbell 읽음 → 새 커맨드 인지                              │
 │    → PCIe DMA Read: SQ에서 커맨드(64B) 페치                        │
 │    → PCIe DMA Read: PRP1이 가리키는 호스트 메모리에서 4KB 읽기      │
 │    → FTL 변환: LBA → PPA (물리 페이지 주소)                        │
 │    → NAND Flash에 프로그램                                          │
 │    → PCIe DMA Write: CQ에 완료 엔트리(16B) 기록                    │
 │    → MSI-X 인터럽트 발생                                            │
 │                                                                     │
 └──────────────────────────────────┬──────────────────────────────────┘
                                    │ MSI-X 인터럽트
                                    ▼
 ┌─────────────────────────────────────────────────────────────────────┐
 │  ③ 완료 (Completion)                                                │
 │                                                                     │
 │  [CPU] 인터럽트 수신 → nvme_irq()                                  │
 │    → nvme_poll_cq(): CQE.phase == cq_phase 확인                   │
 │      → dma_rmb(): 메모리 배리어                                     │
 │      → nvme_handle_cqe(): command_id로 request 찾기                │
 │        → nvme_try_complete_req(): status/result 저장               │
 │      → nvme_update_cq_head(): head++, wrap 시 phase 토글          │
 │    → nvme_ring_cq_doorbell(): CQ Head DB 쓰기                     │
 │    → nvme_pci_complete_rq():                                       │
 │      → nvme_pci_unmap_rq(): DMA 매핑 해제                          │
 │      → nvme_complete_rq() → blk_mq_end_request()                  │
 │        → bio->bi_end_io() → 사용자에게 완료 통보                    │
 │                                                                     │
 └─────────────────────────────────────────────────────────────────────┘
```

---

## 부록: 소스코드 위치 정리

| 함수/구조체 | 파일 | 라인 |
|-------------|------|------|
| `nvme_queue_rq()` | `drivers/nvme/host/pci.c` | 1675 |
| `nvme_prep_rq()` | `drivers/nvme/host/pci.c` | 1598 |
| `nvme_sq_copy_cmd()` | `drivers/nvme/host/pci.c` | 835 |
| `nvme_write_sq_db()` | `drivers/nvme/host/pci.c` | 808 |
| `nvme_irq()` | `drivers/nvme/host/pci.c` | 1995 |
| `nvme_poll_cq()` | `drivers/nvme/host/pci.c` | 1955 |
| `nvme_handle_cqe()` | `drivers/nvme/host/pci.c` | 1883 |
| `nvme_update_cq_head()` | `drivers/nvme/host/pci.c` | 1924 |
| `nvme_ring_cq_doorbell()` | `drivers/nvme/host/pci.c` | 1852 |
| `nvme_commit_rqs()` | `drivers/nvme/host/pci.c` | 849 |
| `nvme_queue_rqs()` | `drivers/nvme/host/pci.c` | 1763 |
| `nvme_dbbuf_update_and_check_event()` | `drivers/nvme/host/pci.c` | 592 |
| `struct nvme_queue` | `drivers/nvme/host/pci.c` | 355 |
| `struct nvme_iod` | `drivers/nvme/host/pci.c` | 424 |
| `struct nvme_dev` | `drivers/nvme/host/pci.c` | 243 |
| `nvme_setup_cmd()` | `drivers/nvme/host/core.c` | 1427 |
| `struct nvme_rw_command` | `include/linux/nvme.h` | 1406 |
| `struct nvme_completion` | `include/linux/nvme.h` | 2711 |
| `struct nvme_command` | `include/linux/nvme.h` | 2396 |
| `union nvme_data_ptr` | `include/linux/nvme.h` | 1317 |
| `submit_bio()` | `block/blk-core.c` | 1244 |
| `blk_mq_submit_bio()` | `block/blk-mq.c` | 3697 |
| `blk_mq_dispatch_rq_list()` | `block/blk-mq.c` | 2502 |
| `nvme_dev_ioctl()` | `drivers/nvme/host/ioctl.c` | 840 |
| `nvme_user_cmd()` | `drivers/nvme/host/ioctl.c` | 281 |
| `nvme_submit_user_cmd()` | `drivers/nvme/host/ioctl.c` | 162 |
