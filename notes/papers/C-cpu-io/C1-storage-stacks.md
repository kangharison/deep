# [C1] Performance Characterization of Modern Storage Stacks: POSIX I/O, libaio, SPDK, and io_uring

- **학회/연도:** CHEOPS 2023
- **저자:** Dennis Didulo, Niklas Ueter, Sebastian Duda, et al.
- **분류:** CPU Initiated I/O

## 핵심 요약 (1~2문장)
fio 벤치마크를 활용하여 POSIX I/O(sync), libaio, io_uring, SPDK 네 가지 I/O 스택의 throughput, latency, CPU 효율성을 체계적으로 비교 분석한 논문. 최신 NVMe SSD 환경에서 SPDK가 io_uring 대비 최대 13배 높은 CPU 효율성을 보이며, 커널 바이패스의 실질적 이점을 정량적으로 실증.

## 읽기 전 질문
- io_uring의 커널 내 폴링 모드(SQPOLL)는 SPDK의 유저스페이스 폴링과 비교해 얼마나 격차가 있는가?
- 각 I/O 스택의 CPU 오버헤드 breakdown(syscall, context switch, interrupt 등)은 어떻게 되는가?
- 멀티코어 환경에서 I/O 스택별 스케일링 특성은 어떻게 다른가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- NVMe SSD의 성능이 급격히 향상(수백만 IOPS, 수 마이크로초 레이턴시)되면서, **소프트웨어 I/O 스택 자체가 병목**이 되고 있음
- 기존 연구들은 개별 I/O 인터페이스를 분석하거나 특정 워크로드에 한정된 비교만 수행
- POSIX sync I/O, libaio, io_uring, SPDK를 **동일한 조건**에서 **공정하게** 비교한 연구가 부족
- 특히 **CPU 효율성**(IOPS per CPU cycle)을 중심으로 한 체계적 분석이 필요

### 2. 제안 방법 (Approach)
- **fio 벤치마크**를 공통 도구로 사용하여 네 가지 I/O 스택을 동일 하드웨어에서 비교
- 측정 지표:
  - **Throughput** (IOPS, bandwidth)
  - **Latency** (평균, 99th percentile)
  - **CPU 효율성** (IOPS per CPU cycle)
- 워크로드 변수:
  - I/O 크기 (4KB, 8KB, 16KB, ...)
  - Queue depth (1, 2, 4, 8, ..., 128)
  - Read/Write 비율
  - Random/Sequential 패턴
- io_uring의 다양한 모드 비교: 기본 모드, SQPOLL 모드, IOPOLL 모드, SQPOLL+IOPOLL 모드

### 3. 핵심 아키텍처/설계

```
┌──────────────────────────────────────────────────────────┐
│                    Application (fio)                      │
├──────────┬──────────┬──────────────────┬─────────────────┤
│ POSIX    │ libaio   │    io_uring      │     SPDK        │
│ read/    │ io_submit│                  │                 │
│ write    │ io_getev │  ┌────────────┐  │  ┌───────────┐  │
│          │          │  │ SQ ──► CQ  │  │  │ User-space│  │
│          │          │  │ (shared    │  │  │ NVMe      │  │
│          │          │  │  ring buf) │  │  │ Driver    │  │
│          │          │  └────────────┘  │  └───────────┘  │
├──────────┴──────────┤      │           │       │         │
│   syscall 경계      │  syscall (or     │  NO syscall     │
│                     │  SQPOLL 커널     │                 │
│   VFS → Block Layer │  스레드)         │  직접 PCIe      │
│   → NVMe Driver     │      │           │  Doorbell       │
│                     │  Block Layer     │                 │
│                     │  → NVMe Driver   │                 │
├─────────────────────┴──────────────────┴─────────────────┤
│                    NVMe SSD Hardware                      │
└──────────────────────────────────────────────────────────┘

CPU 오버헤드 비교:
  POSIX sync:  syscall + VFS + block layer + interrupt + context switch
  libaio:      syscall + block layer + interrupt (batch 처리)
  io_uring:    ring buffer + block layer + interrupt/poll
  SPDK:        polling only (NO syscall, NO interrupt, NO context switch)
```

### 4. 실험 결과 (Key Results)
- **IOPS (4KB random read, high queue depth):**
  - POSIX sync: 가장 낮은 성능 (syscall 오버헤드로 인해 CPU 병목)
  - libaio: POSIX 대비 상당한 개선, batch submission으로 syscall 횟수 감소
  - io_uring (기본): libaio와 유사하거나 약간 높은 IOPS
  - io_uring (SQPOLL+IOPOLL): 기본 io_uring 대비 눈에 띄는 개선
  - SPDK: **최고 IOPS 달성**, 디바이스 한계에 근접
- **CPU 효율성 (핵심 결과):**
  - SPDK는 io_uring 대비 **최대 13배** 높은 CPU 효율성 (IOPS/CPU cycle)
  - io_uring SQPOLL 모드도 기본 모드 대비 향상되지만, SPDK와는 여전히 큰 격차
  - 커널 경유 스택(POSIX, libaio, io_uring)은 모두 커널 오버헤드가 상당
- **Latency:**
  - 낮은 queue depth에서 SPDK가 가장 낮은 레이턴시
  - POSIX sync는 단일 I/O 기준으로도 syscall 오버헤드로 인해 높은 레이턴시
  - io_uring 폴링 모드는 interrupt 기반 대비 tail latency 개선
- **Queue Depth 영향:**
  - 모든 스택에서 queue depth 증가 시 throughput 증가
  - SPDK는 낮은 queue depth에서도 이미 높은 성능
  - 커널 스택들은 높은 queue depth가 필요해야 디바이스 성능에 근접

### 5. 한계점 및 향후 연구
- **단일 디바이스 실험:** 다수 NVMe SSD 환경에서의 스케일링 미검증
- **fio 의존성:** fio의 I/O 엔진 구현이 각 스택의 최적 사용법과 다를 수 있음
- **SPDK의 한계 미논의:** SPDK의 실용적 단점(파일시스템 부재, 운영 복잡성, 멀티프로세스 공유 어려움)에 대한 심도 있는 분석 부족
- **실제 애플리케이션 워크로드 부재:** 합성 벤치마크만 사용, 실제 DB/스토리지 워크로드 미적용
- **io_uring 최적화 여지:** io_uring의 고급 기능(fixed files, registered buffers 등) 활용 미흡

## 다른 논문과의 관계
- 선행 연구: SPDK [P7], io_uring 관련 초기 벤치마크 연구
- 후속 연구: io_uring 최적화, 커널 바이패스 하이브리드 연구
- 비교 대상: [C2] (DBMS 관점에서 유사 비교), [C4] (SPDK 극한 성능)
- [C2]가 DBMS 워크로드 중심이라면, [C1]은 순수 I/O 스택 성능에 집중

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - IOPS vs Queue Depth 그래프 (4개 스택 비교)
  - CPU 효율성 비교 차트 (SPDK 13x 우위)
  - Latency CDF 비교
- 핵심 수치/데이터: SPDK가 io_uring 대비 13배 효율적
- 발표 시 "왜 커널 바이패스가 필요한가?"에 대한 정량적 근거로 활용

## 메모
- fio ioengine별 설정 차이를 이해하면 실험 재현 가능
- perf를 사용한 CPU 프로파일링과 결합하면 더 깊은 분석 가능
