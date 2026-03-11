# [C2] What Modern NVMe Storage Can Do, And How To Exploit It

- **학회/연도:** VLDB 2023
- **저자:** Gabriel Haas, Viktor Leis
- **분류:** CPU Initiated I/O

## 핵심 요약 (1~2문장)
최신 NVMe SSD(Samsung 990 Pro 등)의 실제 성능을 DBMS 관점에서 체계적으로 측정하고, 기존 Linux I/O 스택(read/pread, libaio, io_uring)과 SPDK의 효율성을 비교 분석. **CPU 코어의 절반 이상이 I/O 처리에 소모**되는 현실을 실증하며, DBMS 설계 시 I/O 스택 선택이 핵심 설계 결정임을 주장.

## 읽기 전 질문
- 최신 NVMe SSD의 실측 성능은 스펙시트와 얼마나 차이가 나는가?
- DBMS에서 I/O 스택의 CPU 오버헤드는 쿼리 처리 대비 얼마나 큰가?
- io_uring과 SPDK 사이의 현실적 트레이드오프는 무엇인가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 최신 NVMe SSD는 단일 디바이스로 **1M+ IOPS**, **수 마이크로초 레이턴시** 달성
- 그러나 DBMS가 이 성능을 실제로 활용하지 못하고 있음
- **핵심 문제:** 소프트웨어 I/O 스택의 CPU 오버헤드가 너무 커서 **CPU가 I/O 처리 자체에 대부분의 사이클을 소비**
- 기존 DBMS 연구는 I/O를 "느린 것"으로 가정하고 설계 → 이 가정이 더 이상 유효하지 않음
- DBMS 설계자가 어떤 I/O 인터페이스를 선택해야 하는지에 대한 실용적 가이드라인 부재

### 2. 제안 방법 (Approach)
- **마이크로 벤치마크:** 각 I/O 인터페이스의 순수 성능 측정
  - pread/pwrite (동기 I/O)
  - libaio (Linux AIO)
  - io_uring (다양한 모드: 기본, SQPOLL, IOPOLL, registered buffers)
  - SPDK (유저스페이스 NVMe 드라이버)
- **DBMS 통합 실험:** 실제 DBMS(LeanStore 기반)에 각 I/O 백엔드를 통합하여 TPC-C 등 워크로드 실행
- **CPU 프로파일링:** perf를 활용하여 I/O 경로의 CPU 사이클 breakdown 분석
- 하드웨어: Samsung 990 Pro NVMe SSD, 최신 Intel/AMD 플랫폼

### 3. 핵심 아키텍처/설계

```
┌─────────────────────────────────────────────────────┐
│                    DBMS (LeanStore)                  │
│  ┌──────────┐  ┌──────────┐  ┌──────┐  ┌─────────┐ │
│  │Buffer    │  │Page      │  │Query │  │Recovery │ │
│  │Manager   │  │Eviction  │  │Exec  │  │/WAL     │ │
│  └────┬─────┘  └────┬─────┘  └──────┘  └────┬────┘ │
│       │              │                        │      │
│  ┌────▼──────────────▼────────────────────────▼────┐ │
│  │           I/O Backend (교체 가능)                │ │
│  ├──────┬──────────┬──────────────┬────────────────┤ │
│  │pread │  libaio  │   io_uring   │     SPDK       │ │
│  │      │          │   ┌────────┐ │  ┌──────────┐  │ │
│  │1 req │  batch   │   │SQPOLL  │ │  │Polling   │  │ │
│  │= 1   │  submit  │   │IOPOLL  │ │  │Userspace │  │ │
│  │syscall│         │   │Reg.Buf │ │  │Driver    │  │ │
│  └──┬───┘└────┬────┘   └───┬────┘ │  └────┬─────┘  │ │
└─────┼─────────┼────────────┼──────┴───────┼────────┘ │
      │         │            │              │           │
 ─────▼─────────▼────────────▼──────────────┼───────── │
│          Linux Kernel                     │          │
│  VFS → Block Layer → NVMe Driver          │          │
└───────────────────────────────────────────┼──────────┘
                                            │
                  ┌─────────────────────────▼──────┐
                  │     NVMe SSD (PCIe Gen4)       │
                  │  Samsung 990 Pro 등             │
                  │  Spec: ~1M IOPS random read    │
                  └────────────────────────────────┘

CPU 사이클 분포 (4KB random read, 커널 I/O 스택):
  ┌──────────────────────────────────────────┐
  │ ████████████████░░░░░░░░░░░░░░░░░░░░░░░ │
  │ I/O 스택 오버헤드  │  실제 데이터 처리    │
  │     (~50%+)        │     (~50%-)          │
  └──────────────────────────────────────────┘
```

### 4. 실험 결과 (Key Results)
- **I/O 인터페이스별 최대 IOPS (단일 코어, 4KB random read):**
  - pread: ~100K IOPS (syscall 오버헤드 지배적)
  - libaio: ~300-400K IOPS (batch 처리 효과)
  - io_uring (기본): ~400K IOPS
  - io_uring (SQPOLL+IOPOLL): ~600-800K IOPS
  - SPDK: ~1M+ IOPS (디바이스 한계에 도달)
- **CPU 효율성:**
  - pread: I/O 요청당 ~10,000+ CPU 사이클
  - io_uring: I/O 요청당 ~2,000-5,000 CPU 사이클
  - SPDK: I/O 요청당 ~200-500 CPU 사이클
  - **커널 I/O 스택 사용 시 CPU 코어의 약 50%가 I/O 처리에 소모**
- **DBMS 통합 실험 (TPC-C 등):**
  - SPDK 백엔드가 pread 대비 최대 **2-4배 높은 트랜잭션 처리량**
  - I/O 바운드 워크로드에서 I/O 스택 선택이 전체 성능의 핵심 요소
  - io_uring은 SPDK에 근접하지만, CPU 효율에서 여전히 격차
- **레이턴시:**
  - SPDK: 4KB read 기준 ~5-8us
  - io_uring (IOPOLL): ~10-15us
  - pread: ~15-25us

### 5. 한계점 및 향후 연구
- **SPDK 통합의 실용적 어려움:** 파일시스템 우회로 인해 기존 도구(ls, cp 등) 사용 불가, 운영 복잡성 증가
- **단일 SSD 실험 위주:** 다수 SSD RAID/스트라이핑 환경 미검증
- **특정 DBMS(LeanStore)에 한정:** 다른 DBMS(PostgreSQL, MySQL 등)에 대한 일반화 필요
- **io_uring의 발전 속도:** 논문 작성 시점 이후 io_uring이 계속 개선되고 있어, 격차가 줄어들 가능성
- **향후 연구 방향:**
  - CXL 기반 스토리지에서의 I/O 스택 재평가
  - DBMS 아키텍처 자체를 고속 I/O에 맞게 재설계
  - io_uring과 SPDK의 하이브리드 접근법

## 다른 논문과의 관계
- 선행 연구: LeanStore (Viktor Leis 그룹), SPDK 원본 논문
- 후속 연구: I/O-aware DBMS 설계, NVMe-native 데이터베이스
- 비교 대상: [C1] (순수 I/O 벤치마크), [C4] (SPDK 극한 성능)
- [C1]이 I/O 스택 자체 성능 비교라면, [C2]는 **DBMS에 미치는 영향**에 초점
- [C3]의 io_passthru는 [C2]에서 지적한 커널 I/O 오버헤드의 한 해결 방향

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - I/O 인터페이스별 IOPS 비교 그래프
  - CPU 사이클 breakdown 차트 (I/O 처리 vs 데이터 처리)
  - DBMS throughput vs I/O 백엔드 비교표
- 핵심 수치/데이터: CPU 코어 절반이 I/O 처리에 소모, SPDK가 pread 대비 10배+ CPU 효율
- "DBMS가 NVMe 성능을 충분히 활용하지 못하는 이유"를 설명할 때 최적의 인용 자료

## 메모
- 이 논문의 저자 Viktor Leis는 LeanStore, Umbra 등 고성능 DBMS 연구의 핵심 인물
- DBMS 수업이나 스토리지 시스템 발표에서 "왜 I/O 스택이 중요한가"를 설명할 때 강력한 근거
