# [I8] GPU Graph Processing on CXL-Based Microsecond-Latency External Memory

- **학회/연도:** 2024 (arXiv / workshop)
- **저자:** Qureshi et al. (BaM 저자 그룹과 관련)
- **분류:** GPU Initiated I/O + CXL Memory

## 핵심 요약 (1~2문장)
BaM의 GPU-initiated I/O 아키텍처를 CXL(Compute Express Link) 기반 마이크로초 지연시간 외부 메모리로 확장한 연구. NVMe SSD(~10us 지연) 대신 CXL 메모리(~1us 이하 지연)를 사용할 때의 GPU 그래프 처리 성능을 분석하고, BaM과 EMOGI 접근법의 CXL 환경에서의 트레이드오프를 정량적으로 비교.

## 읽기 전 질문
- CXL 메모리의 지연시간이 NVMe SSD보다 10배 이상 짧을 때, BaM의 fine-grained 접근이 여전히 EMOGI의 coarse-grained 접근보다 유리한가?
- CXL 메모리는 바이트 단위 접근이 가능한데, NVMe의 블록 단위 접근과 어떤 차이를 만드는가?
- GPU-CXL 간 PCIe/CXL 인터커넥트의 대역폭 특성은 GPU-NVMe와 어떻게 다른가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **NVMe SSD의 지연시간 한계**: BaM은 NVMe SSD 기반으로 단일 I/O 요청 지연시간이 ~10-20us. 매우 작은 random access에서는 이 지연이 병목
- **CXL 메모리의 등장**: CXL 2.0/3.0 기반 메모리 확장기(memory expander)는 DRAM급 지연시간(~200-500ns)을 제공하면서도 호스트 DRAM보다 대용량(TB급) 확장 가능
- **접근 방식 재평가 필요**: NVMe에서는 I/O 지연시간이 길어서 coalescing/batching이 중요했지만, CXL 메모리에서는 접근 전략이 달라져야 할 수 있음
- **GPU에서 CXL 메모리 접근 경로**: GPU → PCIe → CXL 스위치 → CXL 메모리. PCIe P2P와는 다른 경로 특성

### 2. 제안 방법 (Approach)
- **BaM을 CXL 환경에 이식**: BaM의 GPU-initiated 접근 모델을 CXL 메모리에 적용
  - NVMe SQ/CQ 대신, GPU가 CXL 메모리에 직접 load/store (바이트 단위 접근 가능)
  - 또는 CXL 메모리를 NVMe 에뮬레이션 모드로 사용
- **EMOGI와의 비교 실험**: CXL 환경에서 fine-grained(BaM 스타일) vs coarse-grained(EMOGI 스타일) 접근의 성능 재비교
- **하이브리드 접근**: GPU VRAM 캐시 + CXL 메모리 백킹 스토어 구성으로 메모리 계층 최적화

### 3. 핵심 아키텍처/설계

```
NVMe 기반 BaM:                    CXL 기반 확장:
┌─────────┐                       ┌─────────┐
│  GPU    │                       │  GPU    │
│  VRAM   │                       │  VRAM   │
│  (Cache)│                       │  (Cache)│
└────┬────┘                       └────┬────┘
     │ PCIe P2P                        │ PCIe/CXL
     │ ~10-20us 지연                   │ ~0.5-1us 지연
┌────▼────┐                       ┌────▼────────┐
│  NVMe   │                       │  CXL Memory │
│  SSD    │                       │  Expander   │
│  (TB)   │                       │  (TB)       │
│ 블록단위│                       │ 바이트단위  │
└─────────┘                       └─────────────┘

메모리 계층 비교:
┌────────────────────────────────────────────────────┐
│ 계층        용량      지연시간    접근 단위         │
│ GPU VRAM    ~80GB     ~ns        바이트            │
│ Host DRAM   ~256GB    ~100ns     바이트            │
│ CXL Memory  ~TB      ~200-500ns 바이트            │
│ NVMe SSD    ~TB      ~10-20us   4KB 블록          │
└────────────────────────────────────────────────────┘
```

**CXL 환경에서의 GPU 접근 방식 변화**:
- NVMe: GPU가 NVMe 커맨드를 작성하고 doorbell을 울려야 함 (프로토콜 오버헤드)
- CXL: GPU가 load/store 명령으로 직접 접근 가능 (UVA와 유사하지만 더 넓은 주소 공간)
- 결과: CXL에서는 BaM의 NVMe 커맨드 오버헤드가 사라지므로, 매우 작은 random access에서 특히 유리

### 4. 실험 결과 (Key Results)
- **CXL vs NVMe SSD 성능 비교** (GPU 그래프 BFS):
  - CXL 메모리 사용 시 BaM 대비 **3~10배** 빠른 그래프 순회 (지연시간 차이에 비례)
  - EMOGI 방식도 CXL에서 성능 향상, 하지만 BaM 스타일 fine-grained 접근의 이점이 CXL에서 더 두드러짐
- **접근 패턴별 분석**:
  - 높은 locality 워크로드: EMOGI와 BaM 성능 차이 감소 (CXL 지연이 짧아서 coarse 접근도 괜찮)
  - 불규칙 접근 워크로드: BaM 스타일이 여전히 우위 (읽기 증폭 차이가 관건)
- **대역폭 활용**: CXL 메모리의 읽기 대역폭(~50-100 GB/s)을 GPU가 효율적으로 활용 가능 확인
- **캐시 효과**: CXL의 낮은 지연시간 덕에 캐시 miss penalty가 크게 줄어, 더 작은 캐시로도 충분한 성능

### 5. 한계점 및 향후 연구
- **CXL 하드웨어 미성숙**: 실험 환경이 에뮬레이션 또는 초기 하드웨어 기반. 상용 CXL 메모리의 실제 성능은 다를 수 있음
- **GPU-CXL 직접 연결**: 현재 GPU는 CXL 인터페이스를 직접 지원하지 않음. PCIe를 통한 우회 접근 필요
- **비용**: CXL 메모리는 NVMe SSD보다 GB당 비용이 높음. 비용 대비 성능 분석 필요
- **Write 워크로드**: CXL 메모리의 write 특성(persist 보장 등)은 별도 분석 필요
- **NVIDIA GPU의 CXL 지원 로드맵**: 향후 GPU가 CXL.mem을 네이티브 지원하면 아키텍처가 크게 변할 수 있음

## 다른 논문과의 관계
- 선행 연구: BaM [I1] (NVMe 기반 원본), EMOGI [P4] (coarse-grained 비교 대상)
- 후속 연구: CXL 기반 GPU 메모리 확장 연구의 선구적 실험
- 비교 대상: BaM [I1] (NVMe vs CXL에서의 동일 접근법 비교), GMT [I9] (메모리 계층 관리와 관련)
- 관련: CXL 컨소시엄의 메모리 풀링/공유 연구

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure: NVMe vs CXL 기반 GPU 그래프 처리 성능 비교
  - Figure: 메모리 계층별 지연시간/대역폭 특성 다이어그램
  - Table: BaM(NVMe) vs BaM(CXL) vs EMOGI(CXL) 성능 비교표
- 핵심 수치/데이터:
  - CXL 메모리로 전환 시 BaM 3~10배 성능 향상
  - CXL 접근 지연시간 ~200-500ns vs NVMe ~10-20us
  - GPU 그래프 처리에서의 메모리 계층별 성능 차이

## 메모
- CXL이 보편화되면 BaM의 NVMe 직접 접근은 "CXL 메모리 직접 접근"으로 자연 진화할 가능성
- 그러나 CXL 메모리는 비용이 높으므로, NVMe SSD + CXL 메모리 + GPU VRAM의 3계층 구조가 현실적 (→ GMT [I9]와 연결)
- GPU의 CXL 네이티브 지원이 이 분야의 game changer가 될 것
