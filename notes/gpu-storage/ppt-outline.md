# GPU가 PCIe 기반 NVMe SSD에 미치는 영향 분석 — PPT 구성안 (15분 발표)

## 전체 구조

| 파트 | 슬라이드 | 시간 | 내용 |
|------|---------|------|------|
| 1. 문제 제기 | 1~4 | 3분 | 왜 이 주제인가, PCIe 공유 자원 문제 |
| 2. 세 가지 모델 비교 | 5~10 | 5분 | **핵심 파트** — 그림 3개 + 정량 비교 |
| 3. PCIe 영향 분석 | 11~15 | 4분 | 대역폭 경합, 토폴로지, I/O 크기, QD |
| 4. 발전 흐름 & 결론 | 16~18 | 3분 | 타임라인, 산업 동향, 핵심 메시지 |

---

## Part 1: 문제 제기 (3분, 슬라이드 1~4)

### 슬라이드 1 — 타이틀

> "GPU가 PCIe 기반 NVMe SSD에 미치는 영향 분석"

### 슬라이드 2 — 왜 이 주제인가?

- AI/HPC 시대: GPU는 연산, NVMe는 데이터 — 둘 다 PCIe에 매달려 있다
- GPU VRAM 80GB vs 모델/데이터 TB급 → SSD 접근 불가피
- 핵심 질문: **GPU가 있으면 NVMe 성능이 어떻게 달라지는가?**

### 슬라이드 3 — PCIe 버스: 공유 자원의 딜레마

- PCIe Gen4 x16 = ~32GB/s, Gen5 = ~64GB/s
- GPU도 x16, NVMe도 x4 — 같은 root complex 아래에서 대역폭 경합

```
              ┌─────────────────────┐
              │   CPU Root Complex   │
              └──────┬──────┬───────┘
                     │      │
              ┌──────┴──┐ ┌─┴────────┐
              │ PCIe x16│ │ PCIe x4  │
              │  GPU    │ │ NVMe SSD │
              └─────────┘ └──────────┘
                ▲ 둘 다 같은 PCIe 대역폭을 공유
```

### 슬라이드 4 — 전통적 데이터 경로의 문제

```
NVMe SSD ──DMA──► 커널 버퍼 ──CPU copy──► User 버퍼 ──cudaMemcpy──► GPU VRAM
              (1차 복사)         (2차 복사)              (3차 전송)
```

- 3번의 데이터 이동, CPU가 2번 개입, PCIe 대역폭 3배 소모
- CPU 코어 절반 이상이 I/O 처리에 소모 [C2]

---

## Part 2: 세 가지 I/O 모델 비교 — 핵심 파트 (5분, 슬라이드 5~10)

### 슬라이드 5 — I/O 모델 Overview (한 장에 세 모델 요약표)

| | CPU Initiated | CPU Mediated (GDS) | GPU Initiated (BaM) |
|---|---|---|---|
| I/O 발행 주체 | CPU | CPU | GPU |
| 데이터 경로 | SSD→CPU RAM→GPU | SSD→GPU (P2P DMA) | SSD→GPU (P2P DMA) |
| CPU 개입 | 전체 과정 | 발행만 | 없음 |
| 적합 I/O 패턴 | 범용 | 대용량 sequential | fine-grained random |

### 슬라이드 6 — 그림 ① CPU Initiated I/O (단독)

```
┌──────┐    ① I/O 요청      ┌──────┐    ② DMA     ┌──────────┐
│ App  │ ──────────────────► │ CPU  │ ◄──────────── │ NVMe SSD │
│(GPU) │    ⑤ cudaMemcpy    │ (커널)│ ③ copy       └──────────┘
│ 대기  │ ◄──────────────── │      │───► User buf
└──────┘                     └──────┘
         ▲ CPU가 전 과정을 중재, GPU는 대기만
```

- CPU 코어 절반이 I/O에 소모 [C2], PCIe 대역폭 3배 낭비

### 슬라이드 7 — 그림 ② CPU Mediated (GDS/P2P DMA)

```
┌──────┐    ① cuFileRead()  ┌──────┐    ② NVMe cmd  ┌──────────┐
│ GPU  │ ◄═══════════════════════════════ P2P DMA ═══ │ NVMe SSD │
│ VRAM │    ③ 직접 도착      │ CPU  │   (발행만)      └──────────┘
└──────┘                     └──────┘
         ▲ 바운스 버퍼 제거! 하지만 CPU가 I/O를 '발행'
```

- throughput 2~6x 향상, CPU 사용률 10~30%로 감소
- 하지만 GPU가 4KB 하나 원할 때마다 CPU에게 부탁해야 함

### 슬라이드 8 — 그림 ③ GPU Initiated I/O (BaM)

```
┌──────────────────────────────────┐
│ GPU                              │
│ ┌──────┐ ┌──────┐ ┌──────┐      │
│ │ 스레드│ │ 스레드│ │ 스레드│ ...  │  ← 수천 스레드가 동시 I/O
│ └──┬───┘ └──┬───┘ └──┬───┘      │
│    │        │        │           │
│    ▼        ▼        ▼           │
│  ┌─────────────────────────┐     │
│  │ NVMe SQ (BAR 매핑)      │     │  ← GPU가 직접 NVMe 큐에 명령 작성
│  └────────────┬────────────┘     │
└───────────────│──────────────────┘
                │ doorbell + P2P DMA
                ▼
          ┌──────────┐
          │ NVMe SSD │ ──DMA──► GPU VRAM
          └──────────┘
     ▲ CPU 완전 제외! GPU가 SPDK처럼 NVMe 큐를 직접 조작
```

- 수천 스레드의 집합적 I/O → 레이턴시 hiding, fine-grained random I/O에 최적

### 슬라이드 9 — 세 모델 성능 비교 (정량 데이터)

| 지표 | CPU Initiated | GDS (CPU Mediated) | BaM (GPU Initiated) |
|---|---|---|---|
| Throughput (seq) | ~4 GB/s | ~6+ GB/s | — (sequential에 특화X) |
| Random 4KB | CPU 의존 | 오버헤드로 비효율 | ~6.5 GB/s (1 SSD) |
| CPU 사용률 | 70~90% | 10~30% | ~0% |
| GPU SM 활용률 | 대기(낮음) | 대기(낮음) | 80%+ (AGILE 비동기) |
| SSD 스케일링 | CPU 코어 수 한계 | CPU 코어 수 한계 | 선형 (4 SSD → 25GB/s) |

### 슬라이드 10 — 왜 세 모델이 공존하는가?

- CPU Initiated: 범용 워크로드, 기존 에코시스템 호환
- GDS: 대용량 sequential (AI 학습 데이터 로딩, 체크포인트)
- GPU Initiated: fine-grained random (그래프 분석, 추천, GNN)
- → 워크로드에 따라 최적 모델이 다르다

---

## Part 3: PCIe 관점의 영향 분석 (4분, 슬라이드 11~15)

### 슬라이드 11 — PCIe 대역폭 경합

- GPU cudaMemcpy + NVMe fio 동시 수행 시 30~60% throughput 하락
- 같은 PCIe switch vs 다른 switch → 성능 차이 20~40%

### 슬라이드 12 — PCIe 토폴로지의 중요성

```
Case A (Same Switch):             Case B (Cross Switch):
  PCIe Switch                       CPU Root Complex
   ├── GPU                           ├── Switch A ── GPU
   └── NVMe  ← P2P 직접!             └── Switch B ── NVMe  ← CPU 경유
```

- Same switch: P2P DMA latency 최소, BaM/GDS 최대 효과
- Cross switch: 모든 경로가 CPU root complex 경유 → 병목

### 슬라이드 13 — I/O 크기별 최적 전략

```
I/O 크기        전통 경로       GDS 경로       개선율
─────────────────────────────────────────────────────
4KB             ~0.5 GB/s       ~0.3 GB/s      오히려 GDS가 느림 (오버헤드)
64KB            ~2 GB/s         ~2.5 GB/s      ~25% 개선
128KB           ~3 GB/s         ~5 GB/s        ~67% 개선  ← GDS 이점 본격화
1MB             ~4 GB/s         ~6+ GB/s       ~50%+ 개선
CPU 사용률      70~90%          10~30%         CPU 해방 효과 극대화
```

- 핵심: GDS 이점은 128KB 이상에서 본격화
- Fine-grained random(4KB)은 GPU Initiated만 효과적

### 슬라이드 14 — Queue Depth와 GPU의 관계

- NVMe의 deep queue (QD 64K)를 활용하려면 massive parallelism 필요
- CPU: 코어 수 한계로 QD 포화 어려움
- GPU: 수천 스레드 → QD 자연스럽게 포화 → NVMe 성능 극대화

### 슬라이드 15 — 메모리 계층 관점

```
┌─────────┐
│ GPU VRAM│ ~80GB, ~1TB/s     ← 핫 데이터
├─────────┤
│ CPU DRAM│ ~512GB, ~50GB/s   ← 웜 데이터
├─────────┤
│ NVMe SSD│ ~TB급, ~7GB/s    ← 콜드 데이터
└─────────┘
```

- GPU가 이 계층을 직접 오케스트레이션할 수 있느냐가 핵심

---

## Part 4: 발전 흐름과 마무리 (3분, 슬라이드 16~18)

### 슬라이드 16 — 기술 발전 타임라인

```
2014 GPUfs → 2015 NVMMU → 2017 SPDK → 2021 EMOGI/GDS
→ 2023 ★BaM → 2024 GIDS → 2025 AGILE/GoFS/NVIDIA SCADA
```

- 각 단계가 해결한 것과 남긴 것을 화살표로 연결

### 슬라이드 17 — 산업 동향 & 미래

- NVIDIA SCADA [N4]: GPU가 스토리지 제어까지 가속 (production 전환)
- Samsung [I7]: GPU Initiated I/O 서베이 — SSD 벤더도 대응 시작
- DapuStor [N3]: 실제 SSD에서 BaM 시연
- NVMe 스펙 진화 [N1]: GPU 접근을 위한 표준 논의

### 슬라이드 18 — 결론 & 핵심 메시지

1. GPU는 PCIe 대역폭 경합, 데이터 경로 비효율, CPU 병목이라는 세 가지 경로로 NVMe에 영향
2. CPU Initiated → CPU Mediated(GDS) → GPU Initiated(BaM)로 진화하며 CPU 개입을 점진적으로 제거
3. 최종 지향점: **GPU가 NVMe를 직접 조작하는 것이 가장 효율적** (CPU 해방 + PCIe 최적 활용 + SSD 선형 스케일링)

---

## 발표 팁

- Part 2의 세 그림 비교가 발표의 하이라이트 — "CPU 개입이 점점 사라진다"는 스토리라인 유지
- Q&A 대비: 정량 비교표(슬라이드 9)와 "세 모델 공존 이유"(슬라이드 10)를 backup 슬라이드로 상세 준비
- 관련 논문 41편의 데이터를 근거로 활용 가능
