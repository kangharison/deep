# [C3] I/O Passthru: Upstreaming a Flexible and Efficient I/O Path in Linux

- **학회/연도:** FAST 2024
- **저자:** Kanchan Joshi, Anuj Gupta, Jens Axboe, Keith Busch, et al. (Samsung, Meta, Western Digital)
- **분류:** CPU Initiated I/O

## 핵심 요약 (1~2문장)
Linux 커널의 기존 블록 레이어를 우회하면서도 커널 내에서 동작하는 새로운 I/O 경로인 **io_passthru**를 설계하고 업스트림. NVMe 명령어를 블록 레이어 변환 없이 직접 전달하여 SPDK에 근접하는 성능을 달성하면서도 커널의 안전성, 스케줄링, 멀티테넌시 이점을 유지.

## 읽기 전 질문
- 블록 레이어를 우회하면서 커널 내에서 어떻게 안전성을 보장하는가?
- io_passthru의 성능은 SPDK 대비 어느 수준이며, 어떤 오버헤드가 남아있는가?
- 기존 NVMe ioctl 인터페이스 대비 io_passthru의 구체적 장점은 무엇인가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **기존 커널 I/O 경로의 문제:**
  - 블록 레이어(bio → request → dispatch)가 범용성을 위해 설계되어 **NVMe 전용 명령어/기능을 활용 불가**
  - 블록 레이어 변환 과정에서 CPU 오버헤드 발생
  - NVMe의 고급 기능(ZNS, KV-SSD, FDP 등)이 블록 인터페이스에 매핑되지 않음
- **기존 대안들의 한계:**
  - **SPDK:** 커널 바이패스로 고성능이지만, 커널의 보호/격리/스케줄링 상실, 운영 복잡
  - **NVMe ioctl:** NVMe 명령을 직접 보낼 수 있지만, 동기식이고 느림 (매 요청마다 ioctl syscall)
  - **io_uring + ioctl:** 비동기화 가능하지만 여전히 블록 레이어 경유
- **목표:** 커널 내에서 동작하면서 블록 레이어를 우회하는 **유연하고 효율적인 I/O 경로**

### 2. 제안 방법 (Approach)
- **io_uring에 새로운 opcode 추가:** `IORING_OP_URING_CMD`
  - NVMe 명령(passthru command)을 io_uring의 SQE로 직접 제출
  - 블록 레이어의 bio/request 변환을 건너뛰고, NVMe 드라이버에 직접 도달
- **char device 기반 경로 활용:** `/dev/ng0n1` (NVMe generic character device)
  - 기존 block device (`/dev/nvme0n1`) 대신 char device를 통해 passthru
  - 블록 레이어를 완전히 우회
- **Big SQE/Big CQE 지원:** 큰 NVMe 명령(예: 메타데이터 포함)을 담기 위해 io_uring의 SQE/CQE 크기 확장
- **Linux 커널 업스트림:** 실제로 Linux 6.x 커널에 머지됨

### 3. 핵심 아키텍처/설계

```
기존 경로 vs io_passthru 경로:

┌──────────────────┐          ┌──────────────────┐
│   Application    │          │   Application    │
│                  │          │                  │
│  io_uring SQE    │          │  io_uring SQE    │
│  (READ/WRITE)    │          │  (URING_CMD)     │
└───────┬──────────┘          └───────┬──────────┘
        │                             │
   ┌────▼────────────┐          ┌─────▼──────────────┐
   │  io_uring Core   │          │  io_uring Core      │
   └────┬────────────┘          └─────┬──────────────┘
        │                             │
   ┌────▼────────────┐          ┌─────▼──────────────┐
   │  Block Layer     │          │  SKIP Block Layer!  │
   │  ┌────────────┐ │          │                     │
   │  │bio → req   │ │          │  /dev/ng0n1         │
   │  │merge/sched │ │          │  (char device)      │
   │  │plugging    │ │          │                     │
   │  └────────────┘ │          └─────┬──────────────┘
   └────┬────────────┘                │
        │                             │
   ┌────▼────────────┐          ┌─────▼──────────────┐
   │  NVMe Driver     │          │  NVMe Driver        │
   │  (block path)    │          │  (char/passthru     │
   │                  │          │   path)              │
   └────┬────────────┘          └─────┬──────────────┘
        │                             │
   ┌────▼────────────────────────────▼───────────────┐
   │              NVMe SSD Hardware                   │
   └──────────────────────────────────────────────────┘

성능 비교 스펙트럼:

  NVMe ioctl    io_uring     io_passthru      SPDK
  (동기, 느림)  (블록레이어) (블록레이어 우회) (커널 우회)
  ├─────────────┼────────────┼─────────────────┤
  낮은 성능                                    최고 성능
  커널 보호 ✓   커널 보호 ✓  커널 보호 ✓       커널 보호 ✗

io_passthru가 지원하는 NVMe 고급 기능:
  ┌──────────────────────────────────┐
  │ ✓ ZNS (Zoned Namespace)         │
  │ ✓ KV-SSD (Key-Value commands)   │
  │ ✓ FDP (Flexible Data Placement) │
  │ ✓ NVMe 벤더 전용 명령어          │
  │ ✓ Copy, Verify 등 NVMe 명령     │
  └──────────────────────────────────┘
```

### 4. 실험 결과 (Key Results)
- **IOPS 성능 (4KB random read):**
  - 기존 io_uring (블록 경로): 기준선
  - io_passthru: 기존 io_uring 대비 **~20-30% IOPS 향상**
  - SPDK 대비: ~5-15% 낮은 수준 (커널 내 최소 오버헤드는 존재)
- **레이턴시:**
  - io_passthru는 블록 레이어 경유 시 발생하는 bio 할당/변환/merge/스케줄링 레이턴시 제거
  - 평균 레이턴시 기존 대비 개선, 특히 **tail latency(P99) 감소**
- **CPU 효율성:**
  - 블록 레이어 우회로 인해 CPU 사이클 절약
  - perf 프로파일: bio 관련 함수들(bio_alloc, blk_mq_submit_bio 등)이 호출되지 않음
- **유연성:**
  - ZNS SSD에서 zone append 명령을 직접 전달하여 기존 블록 인터페이스 대비 효율적
  - NVMe copy 명령 등 블록 레이어에 매핑 없는 명령어 사용 가능
- **호환성:**
  - 기존 커널 인프라(cgroups, 네임스페이스 등)와 호환
  - 멀티테넌트 환경에서 I/O 격리 유지

### 5. 한계점 및 향후 연구
- **파일시스템 통합 미지원:** 현재 raw device 접근만 가능, ext4/xfs 등과의 통합 미완성
- **블록 레이어 기능 상실:** I/O 스케줄링, merge, plugging 등의 최적화를 사용 불가
  - 이 기능들이 필요한 워크로드에서는 오히려 성능 저하 가능
- **NVMe 전용:** 다른 스토리지 프로토콜(SCSI, virtio 등)에는 적용 불가
- **애플리케이션 수정 필요:** 기존 POSIX I/O API 사용 앱은 코드 수정 없이 사용 불가
- **SPDK 대비 여전히 성능 격차:** 커널 내 동작으로 인한 최소 오버헤드(syscall, 커널 공간 전환)는 제거 불가
- **향후 연구:**
  - 파일시스템 레벨에서의 passthru 통합
  - 더 많은 NVMe 고급 기능 지원
  - io_uring과의 더 깊은 통합 (예: fixed buffer + passthru 조합)

## 다른 논문과의 관계
- 선행 연구: io_uring 기본 설계 (Jens Axboe), NVMe ioctl 인터페이스
- 후속 연구: NVMe FDP 지원, 파일시스템 passthru 확장
- 비교 대상: SPDK [P7] (커널 바이패스 vs 커널 내 바이패스), io_uring 기본 경로
- [C1]과 [C2]에서 드러난 커널 I/O 오버헤드 문제의 **커널 내 해결책**
- [C4]의 SPDK 극한 성능과 비교하면, io_passthru는 "커널 포기 없이 도달 가능한 최선"

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - 기존 블록 경로 vs passthru 경로 아키텍처 비교 다이어그램
  - IOPS/레이턴시 비교 그래프 (io_uring vs passthru vs SPDK)
  - CPU 프로파일 flame graph (블록 레이어 함수 제거 확인)
- 핵심 수치/데이터: 블록 레이어 우회로 ~20-30% IOPS 향상, SPDK 대비 ~90% 성능
- "커널을 포기하지 않고도 성능을 높일 수 있다"는 메시지의 핵심 근거
- Linux 업스트림 사례로서 오픈소스 기여 과정 설명에도 활용 가능

## 메모
- Linux 6.x에 실제 머지된 코드이므로 ~/sources/linux에서 관련 코드 확인 가능
- `drivers/nvme/core/ioctl.c`에서 passthru 구현 확인
- fio에도 io_uring_cmd ioengine이 추가됨 → 벤치마크 재현 가능
