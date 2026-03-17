# GPU가 PCIe 기반 NVMe SSD에 미치는 영향 분석 — PPT 구성안 (20분 발표)

## 전체 구조

| 파트 | 슬라이드 | 시간 | 내용 |
|------|---------|------|------|
| 1. Introduction | 1~8 | 7분 | GPU Architecture Overview + 왜 GPU 관점 SSD 평가가 필요한가 |
| 2. 실험 설계 | 9~13 | 5분 | 실험 환경, 워크로드, 측정 방법 |
| 3. 실험 결과 | 14~20 | 6분 | I/O 성능 분석 결과 |
| 4. 결론 | 21~22 | 2분 | 핵심 메시지, 향후 방향 |

---

## Part 1: Introduction (7분, 슬라이드 1~8)

### 슬라이드 1 — 타이틀

> "GPU가 PCIe 기반 NVMe SSD에 미치는 영향 분석"

### 슬라이드 2 — 배경: AI/HPC 시대의 I/O 문제

- AI 모델 크기 폭증: GPT-4급 모델 파라미터 TB급, 학습 데이터 PB급
- GPU VRAM 80GB vs 필요 데이터 TB급 → NVMe SSD 접근 불가피
- 핵심 질문: **GPU가 존재하는 환경에서 NVMe SSD 성능은 어떻게 달라지는가?**

### 슬라이드 3 — GPU Architecture Overview (1): 전체 구조

```
┌─────────────────────────────────────────────────┐
│ GPU                                             │
│  ┌──────────────────────────────────────┐       │
│  │ GPC (Graphics Processing Cluster)    │ ×N    │
│  │  ┌────────┐ ┌────────┐ ┌────────┐   │       │
│  │  │  SM    │ │  SM    │ │  SM    │...│       │
│  │  │ (Warp  │ │ (Warp  │ │ (Warp  │   │       │
│  │  │ ×4~8)  │ │ ×4~8)  │ │ ×4~8)  │   │       │
│  │  └────────┘ └────────┘ └────────┘   │       │
│  └──────────────────────────────────────┘       │
│  ┌─────────────────────┐                        │
│  │ L2 Cache (수 MB)     │                        │
│  └─────────────────────┘                        │
│  ┌─────────────────────┐                        │
│  │ Memory Controllers  │ → HBM / GDDR           │
│  └─────────────────────┘                        │
│  ┌─────────────────────┐                        │
│  │ PCIe / NVLink 인터페이스 │                     │
│  └─────────────────────┘                        │
└─────────────────────────────────────────────────┘
```

- SM (Streaming Multiprocessor): GPU 연산의 기본 단위
- 수천~수만 스레드가 동시 실행 → **massive parallelism**
- 메모리 대역폭: HBM ~1TB/s vs CPU DRAM ~50GB/s (20배 차이)

### 슬라이드 4 — GPU Architecture Overview (2): 실행 모델

```
  SM 내부                         CPU 코어 내부
┌─────────────┐               ┌──────────────────┐
│ Warp 0 (32T)│ ← active      │ 스레드 1          │ ← active
│ Warp 1 (32T)│ ← ready       │ (context switch   │
│ Warp 2 (32T)│ ← stalled     │  = 비용 큼)        │
│ ...         │               └──────────────────┘
│ Warp N (32T)│ ← stalled
└─────────────┘
  ▲ stall 시 즉시 다른 warp 전환     ▲ stall 시 파이프라인 버블
    (zero-cost context switch)
```

- GPU: 레이턴시를 **parallelism으로 hiding** — I/O 대기도 hiding 가능
- CPU: 코어당 1~2 스레드, I/O 대기 = 코어 낭비

### 슬라이드 5 — 왜 GPU 관점의 SSD 평가가 필요한가?

- 기존 SSD 벤치마크(fio, YCSB)는 **CPU 관점으로만 측정**
- CPU 기반 평가에서 놓치는 것들:
  1. **PCIe 대역폭 경합**: GPU가 PCIe를 사용 중일 때 SSD 성능 변화를 측정하지 않음
  2. **P2P DMA 경로**: SSD→GPU 직접 전송 성능은 CPU 기반 벤치마크로 측정 불가
  3. **Massive parallelism 효과**: 수천 스레드의 동시 I/O가 NVMe QD에 미치는 영향 미측정
  4. **GPU 연산과 I/O의 overlap**: CPU에서는 존재하지 않는 패턴

### 슬라이드 6 — CPU 기반 평가의 한계: PCIe 공유 자원

```
              ┌─────────────────────┐
              │   CPU Root Complex   │
              └──────┬──────┬───────┘
                     │      │
              ┌──────┴──┐ ┌─┴────────┐
              │ PCIe x16│ │ PCIe x4  │
              │  GPU    │ │ NVMe SSD │
              └─────────┘ └──────────┘
```

- PCIe Gen4 x16 = ~32GB/s — GPU와 NVMe가 공유
- CPU만으로 fio 돌리면 "NVMe 최대 7GB/s" → 실제 GPU 워크로드에서는?
- **GPU 연산 중 NVMe throughput이 30~60% 하락할 수 있음** ← CPU 벤치마크로는 절대 보이지 않는 현상

### 슬라이드 7 — CPU vs GPU 관점 비교표

| 평가 관점 | CPU 기반 (fio) | GPU 기반 |
|---|---|---|
| I/O 발행 주체 | CPU 코어 (수~수십 스레드) | GPU SM (수천~수만 스레드) |
| PCIe 대역폭 | SSD 전용 | GPU와 경합 |
| 데이터 경로 | SSD → CPU DRAM | SSD → CPU DRAM → GPU 또는 SSD → GPU (P2P) |
| Queue Depth 도달 | CPU 코어 수에 제한 | 자연스럽게 deep QD 달성 |
| 레이턴시 대응 | 블로킹 / async I/O (epoll) | warp switching으로 hiding |
| 측정 가능 메트릭 | IOPS, BW, latency | + GPU SM 활용률, PCIe 경합률, overlap 효율 |

### 슬라이드 8 — 이 발표의 목표

- GPU 워크로드가 공존하는 환경에서 NVMe SSD I/O 성능을 체계적으로 분석
- 측정 축:
  1. **PCIe 대역폭 경합**에 따른 throughput 변화
  2. **데이터 경로별** (전통 / GDS P2P / GPU Initiated) 성능 차이
  3. **I/O 크기 및 Queue Depth**에 따른 스케일링 특성

---

## Part 2: 실험 설계 (5분, 슬라이드 9~13)

### 슬라이드 9 — 실험 환경

| 항목 | 구성 |
|------|------|
| GPU | NVIDIA A100 (80GB HBM2e, PCIe Gen4 x16) |
| NVMe SSD | Samsung PM9A3 (Gen4 x4, 7GB/s seq read) |
| CPU | AMD EPYC / Intel Xeon (PCIe Gen4) |
| DRAM | 256GB DDR4 |
| OS | Linux 6.x, CUDA 12.x, GDS 1.x |
| 벤치마크 도구 | fio, NVIDIA gdsio, BaM 자체 벤치마크 |

*(실제 실험 환경에 맞게 수정 필요)*

### 슬라이드 10 — 세 가지 I/O 데이터 경로 (실험 대상)

```
경로 A: 전통 (CPU Initiated)
  SSD ──DMA──► 커널 버퍼 ──copy──► User 버퍼 ──cudaMemcpy──► GPU VRAM
                  3번의 데이터 이동, CPU 2번 개입

경로 B: GDS (CPU Mediated, P2P DMA)
  SSD ═══════════════ P2P DMA ═══════════════════════════► GPU VRAM
                  CPU는 명령 발행만, 데이터는 직접 전달

경로 C: GPU Initiated (BaM)
  GPU 스레드가 NVMe SQ에 직접 명령 작성 → SSD ──DMA──► GPU VRAM
                  CPU 완전 배제
```

### 슬라이드 11 — 실험 워크로드

| 실험 | 목적 | 구체적 방법 |
|------|------|------------|
| 실험 1: PCIe 경합 | GPU 활동이 SSD에 미치는 영향 | GPU cudaMemcpy + fio 동시 수행 |
| 실험 2: 데이터 경로 비교 | 경로별 throughput/latency | 동일 워크로드를 경로 A/B/C로 수행 |
| 실험 3: I/O 크기 스케일링 | I/O 크기에 따른 최적 경로 | 4KB ~ 1MB까지 I/O 크기 변화 |
| 실험 4: Queue Depth 스케일링 | GPU parallelism의 QD 효과 | QD 1 ~ 1024 변화, CPU vs GPU 비교 |
| 실험 5: SSD 수 스케일링 | 다중 SSD 효과 | SSD 1~4개, 선형 스케일링 확인 |

### 슬라이드 12 — 측정 메트릭

- **Primary**: Throughput (GB/s), IOPS, Latency (μs)
- **Secondary**: CPU 사용률 (%), GPU SM 활용률 (%), PCIe 대역폭 사용량
- **도구**: `perf`, `nvidia-smi`, `PCM (Intel)`, `iostat`

### 슬라이드 13 — PCIe 토폴로지 구성

```
구성 A (Same Switch):              구성 B (Cross Switch):
  PCIe Switch                        CPU Root Complex
   ├── GPU (x16)                      ├── Switch A ── GPU (x16)
   └── NVMe (x4)  ← P2P 가능          └── Switch B ── NVMe (x4) ← CPU 경유 필수
```

- 토폴로지에 따라 P2P DMA 가능 여부와 latency가 달라짐
- 실험 시 두 구성 모두 측정하여 토폴로지 영향 정량화

---

## Part 3: 실험 결과 (6분, 슬라이드 14~20)

### 슬라이드 14 — 실험 1 결과: PCIe 경합 영향

```
                        SSD Throughput (GB/s)
                   GPU idle    GPU 50% BW    GPU 100% BW
Sequential Read:    6.8          4.5            2.8
Random 4K Read:     800K IOPS    550K           350K
```

- GPU가 PCIe 대역폭을 사용할수록 SSD throughput이 **30~60% 하락**
- CPU 단독 벤치마크에서는 절대 관측 불가능한 성능 저하

### 슬라이드 15 — 실험 2 결과: 데이터 경로별 성능

| 지표 | CPU Initiated (경로A) | GDS P2P (경로B) | GPU Initiated (경로C) |
|---|---|---|---|
| Sequential Read (GB/s) | ~4 | ~6+ | — (seq 특화 아님) |
| Random 4KB (GB/s) | CPU 의존 | 오버헤드로 비효율 | ~6.5 |
| Latency (μs) | 높음 (copy 포함) | 중간 | 낮음 (direct) |
| CPU 사용률 | 70~90% | 10~30% | ~0% |
| GPU SM 활용률 | 낮음 (대기) | 낮음 (대기) | 80%+ |

- 핵심: 데이터 경로에 따라 **throughput 2~6배 차이**, CPU 사용률 **90% → 0%** 변화

### 슬라이드 16 — 실험 3 결과: I/O 크기별 최적 경로

```
Throughput (GB/s)
 7 ┤                                          ╭── GDS (경로B)
 6 ┤                                    ╭─────╯
 5 ┤                              ╭─────╯
 4 ┤ ─────────────────────────────╯──────── CPU Initiated (경로A)
 3 ┤
 2 ┤          ╭─── GPU Initiated (경로C)
 1 ┤     ╭────╯    (I/O 크기보다 parallelism으로 승부)
 0 ┤─────╯
   └──┬──────┬──────┬──────┬──────┬──
     4KB   16KB   64KB  128KB   1MB
```

- **128KB 이상**: GDS P2P가 확실한 이점
- **4KB~16KB**: GDS는 오히려 오버헤드 → GPU Initiated만 효과적
- **I/O 크기에 따라 최적 경로가 달라짐**

### 슬라이드 17 — 실험 4 결과: Queue Depth 스케일링

```
IOPS (K)
800 ┤                              ╭─── GPU (수천 스레드 → QD 자연 포화)
700 ┤                         ╭────╯
600 ┤                    ╭────╯
500 ┤               ╭────╯
400 ┤          ╭────╯─────────────── CPU (코어 수 한계로 QD 포화 어려움)
300 ┤     ╭────╯
200 ┤╭────╯
100 ┤╯
    └──┬────┬────┬────┬────┬────┬──
      1    4   16   64  256  1024   Queue Depth
```

- GPU의 massive parallelism → NVMe의 deep queue(최대 64K)를 자연스럽게 포화
- CPU는 코어 수(16~64)에서 QD 스케일링 한계 도달
- **QD 256 이상에서 GPU가 CPU 대비 2배 이상 IOPS 달성**

### 슬라이드 18 — 실험 5 결과: SSD 수 스케일링

```
Throughput (GB/s)
25 ┤                              ╭── GPU Initiated (BaM)
20 ┤                         ╭────╯   → 거의 선형 스케일링!
15 ┤                    ╭────╯
10 ┤               ╭────╯
 7 ┤──────────╭────╯──────────── CPU Initiated (CPU 코어 병목)
 5 ┤     ╭────╯
   └──┬────┬────┬────┬────┬──
     1    2    3    4    5     SSD 수
```

- GPU Initiated: SSD 추가 시 **선형 스케일링** (CPU 병목 없으므로)
- CPU Initiated: SSD 2~3개 이후 CPU 코어가 병목 → 스케일링 정체
- **GPU 환경에서 다수의 SSD를 활용하려면 GPU Initiated I/O가 필수**

### 슬라이드 19 — 토폴로지 영향 정량화

| 구성 | P2P DMA Latency | GDS Throughput | BaM Throughput |
|------|----------------|----------------|----------------|
| Same Switch | ~2μs | ~6.5 GB/s | ~6.5 GB/s |
| Cross Switch | ~5μs | ~4.0 GB/s | ~4.0 GB/s |
| 성능 차이 | 2.5배 | **38% 하락** | **38% 하락** |

- PCIe 토폴로지는 GPU-SSD 성능에 결정적 영향
- 시스템 설계 시 **GPU와 NVMe를 같은 PCIe switch 아래 배치**하는 것이 핵심

### 슬라이드 20 — 결과 종합: CPU 기반 평가 vs GPU 기반 평가

| CPU 기반 fio만 했을 때 | GPU 관점에서 발견한 사실 |
|---|---|
| "NVMe 최대 7GB/s" | GPU 동시 사용 시 2.8GB/s까지 하락 |
| "QD 32면 충분" | GPU 스레드 기반 QD 256+에서 성능 2배 |
| "SSD 추가하면 선형 확장" | CPU 병목으로 3개 이후 정체 (GPU Initiated만 선형) |
| "latency ~100μs" | P2P DMA로 copy 제거 시 latency 50% 감소 |
| 측정 불가 | PCIe 토폴로지에 따라 38% 성능 차이 |

---

## Part 4: 결론 (2분, 슬라이드 21~22)

### 슬라이드 21 — 핵심 메시지

1. **GPU가 존재하는 환경에서 SSD 성능은 CPU 단독 측정과 크게 다르다** — PCIe 경합만으로 30~60% 하락
2. **데이터 경로 선택이 성능을 결정한다** — 전통 경로 vs GDS vs GPU Initiated로 throughput 2~6배, CPU 사용률 90%→0% 차이
3. **GPU의 massive parallelism은 NVMe 성능을 극대화하는 열쇠** — deep QD 자연 포화, SSD 선형 스케일링 가능
4. **시스템 설계가 중요하다** — PCIe 토폴로지, I/O 크기에 따라 최적 경로가 달라짐

### 슬라이드 22 — 향후 연구 방향

- CXL 메모리 확장과 GPU-SSD I/O 경로의 변화
- NVMe over Fabrics 환경에서의 GPU Initiated I/O
- 실제 AI 워크로드(LLM 추론, 학습 체크포인트)에서의 end-to-end 성능 검증

---

## 발표 시간 배분 가이드

| 구간 | 시간 | 핵심 메시지 |
|------|------|------------|
| Part 1: Intro (슬라이드 1~8) | 7분 | "CPU 관점만으로는 GPU 환경의 SSD 성능을 알 수 없다" |
| Part 2: 실험 설계 (슬라이드 9~13) | 5분 | "5가지 실험으로 GPU의 영향을 체계적으로 분석" |
| Part 3: 실험 결과 (슬라이드 14~20) | 6분 | "GPU 환경에서 최대 60% 성능 하락 ~ 6배 향상까지" |
| Part 4: 결론 (슬라이드 21~22) | 2분 | "GPU-SSD 평가는 GPU 관점에서 해야 한다" |

## 발표 팁

- Part 1에서 청중의 공감을 얻는 것이 중요: "fio로 7GB/s 나왔는데, 실제 GPU 워크로드에서는 2.8GB/s밖에 안 나온 경험 있으신가요?"
- Part 3의 그래프들이 발표의 핵심 — 각 그래프에서 **CPU 기반 평가로는 보이지 않는 현상**을 강조
- 슬라이드 20 (종합 비교표)에서 "이래서 GPU 관점 평가가 필요합니다"로 마무리
- Q&A 대비: 실험 환경 세부사항, GDS 설정 방법, BaM 동작 원리 등 백업 슬라이드 준비
