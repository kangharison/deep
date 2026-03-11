# [I1] GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture ★필독

- **학회/연도:** ASPLOS 2023
- **저자:** Zaid Qureshi, Vikram Sharma Mailthody, Isaac Gelado et al. (UIUC, NVIDIA, IBM)
- **분류:** GPU Initiated I/O

## 핵심 요약 (1~2문장)
GPU Initiated I/O의 원조 논문. GPU 커널이 CPU 개입 없이 NVMe SQ에 직접 커맨드를 작성하고 doorbell을 울려 SSD I/O를 수행하는 아키텍처 제안. GPUDirect RDMA를 활용하여 NVMe 큐와 I/O 버퍼를 GPU VRAM에 배치하고, 소프트웨어 캐시와 코어 어피니티 기반 큐 관리로 높은 처리량과 낮은 지연시간을 동시에 달성.

## 읽기 전 질문
- GPU 스레드가 NVMe 커맨드를 직접 발행할 때 동기화 문제(수천 개 스레드가 동시에 같은 큐에 접근)는 어떻게 해결하는가?
- CPU를 완전히 우회하면 보안/격리 문제는 없는가? 커널 NVMe 드라이버와 어떻게 공존하는가?
- GPU의 on-demand fine-grained I/O가 기존 coarse-grained DMA 전송 대비 실제로 어떤 워크로드에서 이점이 있는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **GPU 메모리 용량 한계**: GPU VRAM(수십 GB)으로는 대규모 그래프 분석, 추천 시스템 등의 데이터셋(수백 GB~TB)을 담을 수 없음
- **기존 접근법의 비효율성**:
  - **CPU-mediated I/O**: GPU가 데이터를 필요로 할 때마다 CPU에게 요청 → CPU가 스토리지에서 읽어 GPU로 전송. GPU 커널 중단/재시작 오버헤드, CPU-GPU 간 동기화 지연
  - **Coarse-grained prefetch (EMOGI 등)**: 전체 데이터를 미리 GPU로 전송. 실제 접근하지 않는 데이터도 전송하여 대역폭 낭비 (읽기 증폭)
  - **NVIDIA GDS**: CPU가 I/O를 orchestrate하고 DMA로 GPU 메모리에 전달. 여전히 CPU 개입 필요, fine-grained random access에 부적합
- **핵심 문제**: 불규칙 접근 패턴(irregular access)의 워크로드에서 GPU가 필요한 데이터만 on-demand로 스토리지에서 가져올 방법이 없음

### 2. 제안 방법 (Approach)
- **GPU-Initiated I/O**: GPU 커널 스레드가 직접 NVMe SSD에 I/O 커맨드를 발행
- **핵심 아이디어 3가지**:
  1. **NVMe 큐를 GPU BAR 영역에 매핑**: GPUDirect RDMA API를 사용하여 NVMe SQ/CQ와 PRP 버퍼를 GPU VRAM에 할당. SSD 컨트롤러가 PCIe P2P로 GPU 메모리에 직접 DMA
  2. **소프트웨어 관리 캐시**: GPU VRAM에 LRU 기반 소프트웨어 캐시를 구현하여 반복 접근 시 SSD I/O 회피
  3. **SM 어피니티 기반 큐 할당**: GPU의 각 SM(Streaming Multiprocessor)에 전용 NVMe I/O 큐를 할당하여 lock-free 큐 접근 실현
- **SPDK와의 유사점**: 커널 NVMe 드라이버를 우회하고 유저스페이스(여기서는 GPU)에서 직접 NVMe 큐를 조작하는 개념은 SPDK의 polled-mode 드라이버와 동일한 철학

### 3. 핵심 아키텍처/설계

```
┌─────────────────────────────────────────────────────────┐
│                    GPU (CUDA Kernel)                     │
│                                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ SM 0     │  │ SM 1     │  │ SM N     │   Warp 단위  │
│  │ Thread   │  │ Thread   │  │ Thread   │   I/O 요청   │
│  │ Block    │  │ Block    │  │ Block    │              │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘              │
│       │              │              │                    │
│       ▼              ▼              ▼                    │
│  ┌─────────────────────────────────────────────┐        │
│  │         Software Cache (GPU VRAM)            │        │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐          │        │
│  │  │Page │ │Page │ │Page │ │Page │ ...       │        │
│  │  │Cache│ │Cache│ │Cache│ │Cache│           │        │
│  │  └─────┘ └─────┘ └─────┘ └─────┘          │        │
│  │  Hit → 즉시 반환 / Miss → NVMe I/O 발행    │        │
│  └──────────────────┬──────────────────────────┘        │
│                     │ Cache Miss                         │
│       ┌─────────────┼─────────────┐                     │
│       ▼             ▼             ▼                     │
│  ┌────────┐   ┌────────┐   ┌────────┐                  │
│  │ NVMe   │   │ NVMe   │   │ NVMe   │  SM별 전용 큐   │
│  │ SQ 0   │   │ SQ 1   │   │ SQ N   │  (GPU VRAM에    │
│  │        │   │        │   │        │   위치)          │
│  └───┬────┘   └───┬────┘   └───┬────┘                  │
│      │            │            │                        │
│  GPU BAR 영역 (GPUDirect RDMA로 SSD에 노출)            │
└──────┼────────────┼────────────┼────────────────────────┘
       │            │            │
       ▼            ▼            ▼
  ┌─────────────────────────────────────┐
  │        PCIe Bus (P2P DMA)           │
  └─────────────────┬───────────────────┘
                    │
                    ▼
  ┌─────────────────────────────────────┐
  │          NVMe SSD Controller        │
  │                                     │
  │  1. SQ에서 커맨드 fetch (GPU VRAM)  │
  │  2. 데이터 read from NAND          │
  │  3. 데이터를 PRP이 가리키는         │
  │     GPU VRAM 주소로 DMA 전송       │
  │  4. CQ에 완료 항목 기록            │
  └─────────────────────────────────────┘
```

**I/O 발행 상세 흐름 (GPU Thread → SSD → 완료)**:
```
GPU Thread (Warp)                    NVMe SSD Controller
      │                                     │
      │ 1. Cache miss 발생                  │
      │ 2. SQ tail 슬롯 atomic 할당        │
      │    (atomicAdd on tail pointer)      │
      │ 3. NVMe Read Command 작성:         │
      │    - opcode: 0x02 (Read)            │
      │    - NSID, LBA, num blocks          │
      │    - PRP1 → GPU VRAM buffer addr    │
      │ 4. Doorbell 레지스터에 tail 기록     │
      │    (MMIO write to SSD BAR)          │
      │─────────doorbell ring──────────────→│
      │                                     │ 5. SQ에서 cmd fetch
      │                                     │    (PCIe P2P → GPU VRAM)
      │                                     │ 6. NAND에서 데이터 읽기
      │                                     │ 7. PRP 주소로 DMA 전송
      │                                     │    (PCIe P2P → GPU VRAM)
      │                                     │ 8. CQ에 완료 기록
      │◀────────CQ entry written────────────│
      │ 9. CQ polling (GPU thread가         │
      │    phase bit 확인하며 spin)         │
      │ 10. 데이터 사용 가능               │
      │ 11. CQ head doorbell 갱신          │
      │                                     │
```

**핵심 설계 세부사항**:
- **큐 할당 전략**: SM당 1개 SQ/CQ 쌍. GPU 스레드는 자신의 SM ID를 기반으로 큐를 선택 → lock-free. NVMe 스펙상 최대 65,535개 큐 지원하므로 충분
- **Doorbell batching**: 같은 SM의 여러 warp가 동시에 I/O 발행 시 doorbell write를 배치하여 PCIe MMIO 오버헤드 절감
- **Polling 기반 완료 확인**: 인터럽트 대신 GPU 스레드가 CQ의 phase bit를 polling. GPU는 본래 latency hiding에 강하므로 다른 warp가 실행되는 동안 polling warp는 대기
- **캐시 설계**: 페이지 단위(4KB~) 캐시, 해시 테이블 기반 lookup, LRU 교체. Atomic 연산으로 동시성 제어

### 4. 실험 결과 (Key Results)
- **실험 환경**: NVIDIA A100 GPU, Intel Optane P5800X / Samsung PM9A3 NVMe SSD, PCIe Gen4
- **그래프 BFS 워크로드**:
  - CPU-mediated 방식(EMOGI) 대비 **최대 2~9배 성능 향상** (불규칙 접근 패턴에서)
  - GDS 대비 **최대 3~5배** 빠름 (fine-grained random read에서)
- **처리량**:
  - 단일 SSD에서 **~6.5 GB/s** random read 대역폭 달성 (SSD 최대 성능에 근접)
  - 다중 SSD(4개) 구성에서 **~25 GB/s** 달성, 선형 스케일링 확인
- **지연시간**: GPU-initiated I/O의 단일 요청 지연시간은 ~12-20us (NVMe SSD 하드웨어 지연 + PCIe P2P 지연)
- **캐시 효과**: 소프트웨어 캐시 적용 시 BFS의 경우 **40~70% hit rate** 달성, I/O 양 대폭 절감
- **읽기 증폭 비교**: EMOGI(전체 prefetch)는 실제 필요 데이터의 **3~10배**를 읽는 반면, BaM은 필요한 데이터만 읽어 I/O 효율 극대화

### 5. 한계점 및 향후 연구
- **동기식(synchronous) I/O 모델**: GPU 스레드가 I/O 완료를 polling하며 대기 → SM 자원 점유. 비동기 모델이면 더 효율적 (→ AGILE [I3]에서 해결 시도)
- **파일시스템 미지원**: raw block device 접근만 가능. 파일 추상화 없음 (→ GoFS [I4], GeminiFS [I5]에서 해결)
- **Write 지원 제한**: 주로 read 워크로드에 최적화. Write consistency, journaling 등 미고려
- **보안/격리 부재**: GPU가 NVMe 큐를 직접 조작하므로 다른 프로세스/VM과의 격리 메커니즘 없음
- **SSD 호환성**: 모든 NVMe SSD가 GPU VRAM으로의 P2P DMA를 지원하지는 않음 (CMB 필요 여부, PCIe ACS 설정 등)
- **PCIe 토폴로지 의존**: GPU와 SSD가 같은 PCIe switch 아래에 있어야 최적 성능. NUMA-aware 배치 필요

## 다른 논문과의 관계
- 선행 연구: GPUfs [P1] (GPU 파일시스템 개념), EMOGI [P4] (coarse-grained GPU 스토리지 접근), FlashGPU [P3] (GPU-SSD 통합), SPDK [P7] (유저스페이스 NVMe 드라이버 개념 차용)
- 후속 연구: GIDS [I2] (GNN에 적용), AGILE [I3] (비동기화), GoFS [I4] (파일시스템 추가), GeminiFS [I5] (companion FS), Phoenix [I6] (I/O 스택 리팩토링)
- 비교 대상: GDS [G1] (CPU-mediated vs GPU-initiated), EMOGI [P4] (coarse vs fine-grained)
- 개념적 기반: SPDK의 polled-mode NVMe 드라이버를 GPU 환경으로 이식한 것으로 이해 가능

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure 1: 기존 GPU 스토리지 접근 방식 vs BaM 비교 다이어그램
  - Figure 4: BaM 시스템 아키텍처 전체도 (NVMe 큐가 GPU VRAM에 위치하는 구조)
  - Figure 8: BFS 성능 비교 (BaM vs EMOGI vs GDS)
  - Table 1: 다양한 그래프 데이터셋에서의 성능 수치
- 핵심 수치/데이터:
  - EMOGI 대비 최대 9배 성능 향상
  - 단일 SSD ~6.5GB/s, 4 SSD ~25GB/s 선형 스케일링
  - 읽기 증폭 3~10배 절감

## 메모
- BaM의 핵심 통찰: GPU는 수천 개의 스레드를 동시에 실행하므로, I/O 지연시간을 thread-level parallelism으로 숨길 수 있음. CPU와 달리 polling이 자연스러움
- SPDK와의 차이: SPDK는 CPU 코어당 1개 큐를 할당하여 lock-free를 달성. BaM은 SM당 1개 큐를 할당하여 동일한 전략을 GPU에 적용
- NVMe 스펙과의 관계: NVMe 1.3+ 스펙의 CMB(Controller Memory Buffer)와 관련. 단 BaM은 CMB 대신 GPU VRAM에 큐를 배치하는 역방향 접근
