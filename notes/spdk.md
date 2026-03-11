# SPDK 구조 분석

## 전체 아키텍처

```
┌─────────────────────────────────────────────────────────┐
│                    Application                          │
│              (examples/nvme/hello_world)                │
├─────────────────────────────────────────────────────────┤
│              Event Framework (lib/event/)               │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                │
│  │Reactor 0│  │Reactor 1│  │Reactor 2│  ... (코어당 1개)│
│  │(Polling)│  │(Polling)│  │(Polling)│                 │
│  └────┬────┘  └────┬────┘  └────┬────┘                 │
├───────┼─────────────┼───────────┼───────────────────────┤
│       │    Thread Layer (lib/thread/)                   │
│  ┌────┴────┐  ┌────┴────┐  ┌────┴────┐                │
│  │ Thread  │  │ Thread  │  │ Thread  │                 │
│  │+Pollers │  │+Pollers │  │+Pollers │                 │
│  └────┬────┘  └────┬────┘  └────┬────┘                 │
├───────┼─────────────┼───────────┼───────────────────────┤
│       │      Bdev Layer (lib/bdev/)                     │
│  ┌────┴─────────────┴───────────┴────┐                 │
│  │  I/O Channel (스레드별 독립 채널)   │                 │
│  │  QoS, 파티션, RAID, ...            │                 │
│  └────────────────┬──────────────────┘                 │
├───────────────────┼─────────────────────────────────────┤
│            NVMe Driver (lib/nvme/)                      │
│  ┌────────────────┴──────────────────┐                 │
│  │  Controller (nvme_ctrlr.c)        │                 │
│  │  ├─ Admin QP (관리 명령)           │                 │
│  │  └─ I/O QP[] (데이터 명령)         │                 │
│  │     ├─ Submission Queue (SQ)      │                 │
│  │     └─ Completion Queue (CQ)      │                 │
│  └────────────────┬──────────────────┘                 │
├───────────────────┼─────────────────────────────────────┤
│       DPDK Env Layer (lib/env_dpdk/)                   │
│  ┌────────────────┴──────────────────┐                 │
│  │  Huge Page 메모리 관리             │                 │
│  │  PCI 장치 스캔 (UIO/VFIO)         │                 │
│  │  CPU 코어 피닝                     │                 │
│  └────────────────┬──────────────────┘                 │
├───────────────────┼─────────────────────────────────────┤
│              Hardware (NVMe SSD)                        │
│  ┌────────────────┴──────────────────┐                 │
│  │     PCIe BAR + Doorbell 레지스터    │                │
│  └───────────────────────────────────┘                 │
└─────────────────────────────────────────────────────────┘
```

## 핵심 개념 3가지

### 1. Reactor (Polling Loop) — 인터럽트 없음!

커널 NVMe 드라이버는 **인터럽트(MSI-X)** 로 완료를 감지하지만,
SPDK는 **busy-polling**:

```c
// lib/event/reactor.c — 핵심 루프
while (g_reactor_state == SPDK_REACTOR_STATE_RUNNING) {
    event_queue_run_batch(reactor);        // 이벤트 큐 처리
    FOREACH thread in reactor->threads:
        spdk_thread_poll(thread, 0, tsc);  // 모든 poller 실행
}
```

- CPU 코어 1개 = Reactor 1개 (고정 피닝)
- 무한루프로 계속 돌면서 완료 확인 → **context switch 제로**
- CPU 100% 사용하지만, **레이턴시가 극도로 낮음**

### 2. NVMe Completion Polling — Hot Path

```c
// lib/nvme/nvme_pcie_common.c — 실제 완료 처리
while (1) {
    cpl = &pqpair->cpl[pqpair->cq_head];

    // Phase bit로 새 완료 확인 (인터럽트 없이!)
    if (cpl->status.p != pqpair->flags.phase)
        break;  // 새 완료 없음

    __builtin_prefetch(&pqpair->tr[next_cpl->cid]);  // 프리페치
    nvme_pcie_qpair_complete_tracker(qpair, tr, cpl); // 콜백 호출

    if (++num_completions == max_completions)
        break;
}
nvme_pcie_qpair_ring_cq_doorbell(qpair);  // CQ 도어벨
```

### 3. I/O Channel — 락 프리 설계

```
  Thread 0              Thread 1              Thread 2
  ┌──────────┐          ┌──────────┐          ┌──────────┐
  │I/O Chan 0│          │I/O Chan 1│          │I/O Chan 2│
  │(QP 0)    │          │(QP 1)    │          │(QP 2)    │
  └─────┬────┘          └─────┬────┘          └─────┬────┘
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              │
                     NVMe Controller
```

- 스레드마다 **독립된 Queue Pair** → **Lock 불필요**
- 커널은 blk-mq에서 hw queue를 공유할 수 있어 lock 필요

## 커널 NVMe vs SPDK 비교

| 항목 | Linux Kernel | SPDK |
|------|-------------|------|
| **I/O 모델** | 인터럽트 (MSI-X) | Busy-polling |
| **메모리** | 커널이 DMA 관리 | Huge Page 사전 할당 |
| **스레드** | OS 스케줄러 | 코어 고정 (preemption 없음) |
| **Lock** | blk-mq lock 존재 | Lock-free (스레드별 채널) |
| **시스템콜** | read/write/io_uring | 없음 (유저스페이스 직접) |
| **레이턴시** | ~10μs | ~1-2μs |
| **CPU 사용** | 유휴 시 절약 | 항상 100% |

## 주요 소스 파일

| 파일 | 역할 |
|------|------|
| `lib/event/reactor.c` | Reactor 폴링 루프 |
| `lib/thread/thread.c` | 스레드/Poller 관리 |
| `lib/nvme/nvme.c` | NVMe 드라이버 진입점 |
| `lib/nvme/nvme_ctrlr.c` | 컨트롤러 관리 (160KB!) |
| `lib/nvme/nvme_pcie_common.c` | PCIe 완료 처리 (핫패스) |
| `lib/bdev/bdev.c` | Block Device 추상화 (323KB!) |
| `lib/env_dpdk/init.c` | DPDK 초기화 |

## 요약

SPDK의 핵심 철학: **커널을 우회(bypass)하고, 유저스페이스에서 직접 하드웨어를 제어**하여
인터럽트, context switch, 시스템콜 오버헤드를 모두 제거한다.
대신 CPU 코어를 전용으로 할당하여 busy-polling으로 극한의 저레이턴시를 달성한다.
