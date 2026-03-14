# GPU-Storage I/O 논문 실험 지표 총정리 (41편)

GPU-Storage I/O 관련 41편 논문의 실험 섹션에서 측정하는 지표를 체계적으로 분류·정리.

---

## 1차 지표 — 거의 모든 논문이 측정

| 지표 | 단위 | 설명 | 사용 논문 |
|------|------|------|----------|
| **Throughput / Bandwidth** | GB/s, MB/s | 데이터 전송 대역폭 | 거의 전체 (C1~C4, G1~G8, I1~I9, O1~O8, P1~P7) |
| **Latency** | us, ms | I/O 요청~완료 지연시간 (평균 + P99 tail) | C1, C2, C3, G1, G4, G5, I1, I3, I6 |
| **IOPS** | ops/sec | 초당 I/O 처리 횟수 (주로 4KB random read) | C1, C2, C3, C4, P7, N1, N2, N5 |
| **CPU 사용률 / CPU 효율** | IOPS/cycle, % | CPU가 I/O에 소모하는 자원 | C1, C2, C4, G1, G2, G6, G8, I6 |

### 대표 수치

```
Throughput:
  SPDK 단일 코어       → 10M+ IOPS (4KB) [C4]
  BaM 단일 SSD        → ~6.5 GB/s random read [I1]
  BaM 4 SSD           → ~25 GB/s (선형 스케일) [I1]
  GDS sequential read  → legacy 대비 최대 2x [G1]

Latency:
  SPDK 4KB read        → ~5-8us [C2]
  io_uring IOPOLL      → ~10-15us [C2]
  pread (sync)         → ~15-25us [C2]
  BaM GPU-initiated    → ~12-20us [I1]

IOPS:
  pread 단일 코어      → ~100K [C2]
  io_uring SQPOLL      → ~600-800K [C2]
  SPDK 단일 코어       → ~1M+ [C2]
  SPDK 시스템 전체     → 120M (12~14코어) [C4]

CPU 효율:
  pread               → ~10,000+ cycles/IO [C2]
  io_uring            → ~2,000-5,000 cycles/IO [C2]
  SPDK               → ~200-500 cycles/IO [C2]
  커널 I/O 사용 시    → CPU 코어 절반 이상 I/O에 소모 [C2]
```

---

## 2차 지표 — 분류별로 다름

| 지표 | 단위 | 설명 | 사용 논문 |
|------|------|------|----------|
| **GPU SM 활용률** | % | I/O 대기 중 SM 유휴 비율 | I1, I3 |
| **스케일링 (선형성)** | 배수 | SSD/GPU 수 증가 시 성능 비례 여부 | I1, I4, G3, G5, G7, C4 |
| **Cache Hit Rate** | % | GPU VRAM 소프트웨어 캐시 적중률 | I1, I2, I9 |
| **읽기 증폭** | 배수 | 실제 필요 데이터 대비 읽은 총량 | I1 |
| **PCIe 토폴로지 영향** | — | same switch vs cross switch 성능 차이 | G5, I1, I7, G3 |
| **Queue Depth 영향** | — | QD 증가에 따른 throughput/latency 변화 | C1, C2, G5, G6, G8 |
| **I/O 크기별 성능** | — | 4KB~4MB 구간에서 성능 변화 커브 | C1, G1, G5, G6, G8 |

### 대표 수치

```
GPU SM 활용률:
  BaM (동기식 polling) → ~30-40% [I3]
  AGILE (비동기)       → 80%+ [I3]

Cache Hit Rate:
  BaM BFS 워크로드     → 40~70% [I1]
  GIDS GNN 워크로드    → degree-aware 캐시로 10~20%만으로 효과적 [I2]
  GMT tiering          → 전체 데이터의 10~20%만 VRAM에 유지해도 대부분의 성능 [I9]

읽기 증폭:
  EMOGI (coarse-grained) → 필요 데이터의 3~10배를 읽음 [I1]
  BaM (fine-grained)     → 필요한 데이터만 읽음 (1x) [I1]

I/O 크기 임계점:
  GDS 이점 본격화       → 128KB 이상 [G5]
  GDS 최대 대역폭       → 1MB+ [G5, G6, G8]
  소규모 I/O (4KB~64KB) → GDS 오버헤드로 이점 제한적 [G1, G6]

스케일링:
  SPDK 코어 수 vs IOPS → 거의 선형 (14코어 → 120M IOPS) [C4]
  BaM SSD 수 vs BW     → 거의 선형 (4 SSD → ~25 GB/s) [I1]
  GoFS 8 SSD           → ~50 GB/s aggregate, 선형 스케일 [I4]
```

---

## 3차 지표 — AI/ML 워크로드 전용

| 지표 | 단위 | 설명 | 사용 논문 |
|------|------|------|----------|
| **학습 Throughput** | samples/sec, tokens/sec | E2E 학습 처리량 | O1~O8, P5, P6, I2 |
| **학습 정확도** | loss, accuracy | 오프로드가 수렴에 영향 없음을 확인 | I2, O1, O4, O5 |
| **GPU 메모리 절약률** | %, 배수 | 오프로드로 줄어든 VRAM 사용량 | P5, P6, O1, O6 |
| **배치 크기 확장** | 배수 | 오프로드 덕에 늘릴 수 있는 배치 크기 | P5, O5 |
| **SSD 쓰기량 / 수명** | TBW, DWPD | SSD 마모 속도와 수명 영향 | O2 |
| **압축 효과** | % | activation 압축 후 I/O 감소율 | P5, O5 |
| **Recomputation 대비 속도** | 배수, % | gradient checkpointing 대비 학습 시간 | O1, O2 |
| **쿼리 Latency / QPS** | ms, queries/sec | 검색/DB 워크로드의 응답 시간 | G4, O7 |

### 대표 수치

```
학습 가속:
  GIDS vs DGL CPU       → 최대 582배 가속 [I2]
  SSDTrain vs recomp.   → 2.5x 속도 향상 [O1]
  Fuyou vs ZeRO-Inf.    → 1.5~2.5x 학습 속도 향상 [O4]
  MLP-Offload vs ZeRO   → 1.5~3x throughput 향상 [O8]

메모리 절약:
  FlashNeuron           → 배치 4~5x 확장, throughput 10~20% 감소 [P5]
  ZeRO-Infinity         → GPU 대비 500x 모델 크기 확장 [P6]
  SSDTrain              → GPU 메모리 47% 절감 [O1]
  G10                   → GPU 메모리 4~16배 확장 [O6]

SSD 수명:
  TERAIO                → SSD 쓰기량 40~60% 감소, 수명 2~3배 연장 [O2]
  FlashNeuron 압축      → SSD I/O 양 30~50% 감소 [P5]

쿼리 성능:
  ESPN (GDS 검색)       → 쿼리 latency 3.9배 감소 [G4]
  HetCache (3-tier)     → 쿼리 성능 2~5x 향상 [O7]
```

---

## 4차 지표 — 특수 / 일부 논문만

| 지표 | 단위 | 설명 | 사용 논문 |
|------|------|------|----------|
| **Phony buffer 메모리 낭비** | GB | GDS가 할당하는 가짜 CPU 메모리 | I6 |
| **데이터 이동 오버헤드** | % | promote/demote의 연산 성능 영향 | I9 |
| **Coroutine 컨텍스트 스위칭** | ns | GPU coroutine 전환 비용 | I3 |
| **하드웨어 비용 절감** | % | DRAM→SSD 대체 시 비용 절감 | O5, O7 |
| **NVMe 대역폭 활용률** | % | 이론 대역폭 대비 실측 비율 | P4, O3, O6 |
| **데드락 발생 여부** | Y/N | GPU I/O polling 시 교착 상태 | I3 |
| **파일시스템 오버헤드** | % | raw block 대비 FS 추상화 비용 | I4, I5 |
| **네트워크 스토리지 성능** | GB/s, % | 원격 NVMe의 로컬 대비 성능 비율 | G3, G7, I6 |

### 대표 수치

```
Phoenix phony buffer   → 수 GB 메모리 절약, 10~30% latency 감소 [I6]
GMT 데이터 이동        → 연산 성능 영향 5% 미만 [I9]
AGILE coroutine        → 수백 ns 전환 비용 (I/O 10~20us 대비 무시 가능) [I3]
LoHan 비용 절감        → 하드웨어 비용 50~70% 절감 [O5]
HetCache 비용 절감     → 동일 성능 대비 40~60% 비용 절감 [O7]
GoFS FS 오버헤드       → raw block 대비 5~10% [I4]
GeminiFS FS 오버헤드   → raw block 대비 5~15% [I5]
원격 GDS (G7)          → 로컬 NVMe 성능의 ~90% 이상 달성 [G7]
```

---

## 지표별 논문 분포 매트릭스

```
                     P (선행)  C (CPU IO)  G (GDS)  I (GPU-init)  O (오프로드)  N (산업)
──────────────────────────────────────────────────────────────────────────────────────
Throughput/BW         ●●●●      ●●●●       ●●●●      ●●●●●        ●●●●●        ●●
Latency               ○         ●●●        ●●        ●●●          ○            ○
IOPS                  ●         ●●●●       ○         ○            ○            ●●
CPU 효율              ○         ●●●●       ●●●       ●            ○            ○
SM 활용률             ○         ○          ○         ●●           ○            ○
Cache Hit Rate        ○         ○          ○         ●●●          ○            ○
학습 Throughput       ●●        ○          ○         ●            ●●●●●        ○
메모리 절약           ●●        ○          ○         ○            ●●●●         ○
SSD 수명              ○         ○          ○         ○            ●            ○
스케일링              ○         ●          ●●        ●●●          ○            ○
I/O 크기별            ○         ●●         ●●●●      ○            ○            ○
PCIe 토폴로지         ○         ○          ●●        ●●           ○            ○
비용 효율             ○         ○          ○         ○            ●●           ○
──────────────────────────────────────────────────────────────────────────────────────
● = 해당 분류에서 주요 지표로 사용    ○ = 일부만 사용하거나 미사용
```

---

## 핵심 관찰

1. **Throughput/Bandwidth는 유일한 "공통 화폐"** — 모든 분류에서 측정
2. **Latency는 의외로 일부만 측정** — 오프로드(O) 계열은 E2E 학습 시간으로 대체
3. **IOPS는 CPU I/O(C) 논문에 집중** — GPU-initiated 쪽은 BW 중심으로 보고
4. **GPU SM 활용률은 AGILE [I3]이 처음 제기** — BaM의 동기식 한계를 드러내는 핵심 지표
5. **SSD 수명은 TERAIO [O2]만 본격적으로 다룸** — 다른 논문들은 한계점에서 언급만
6. **비용($/성능)은 LoHan [O5]과 HetCache [O7]만 정량적으로 분석**
7. **I/O 크기별 성능은 GDS(G) 논문의 핵심** — GDS의 효과가 I/O 크기에 크게 의존하기 때문
8. **학습 정확도는 "변하지 않음"을 보이는 것이 목적** — 성능 지표가 아닌 정당성 검증

---

## 분류별 "가장 중요한 단일 지표"

| 분류 | 가장 중요한 지표 | 이유 |
|------|----------------|------|
| P (선행 연구) | Throughput | GPU-SSD 직접 경로의 가능성 증명 |
| C (CPU I/O) | **CPU 효율 (IOPS/cycle)** | CPU I/O 스택의 오버헤드를 정량화하는 핵심 |
| G (GDS) | **I/O 크기별 Throughput** | GDS의 효과가 I/O 크기에 극도로 의존 |
| I (GPU-initiated) | **SM 활용률 + 스케일링** | GPU 자원 효율과 SSD 확장성이 핵심 |
| O (오프로드) | **학습 Throughput + 메모리 절약** | 성능 저하 최소화하면서 메모리 확장 |
| N (산업) | **IOPS + 호환성** | SSD 벤더의 GPU-initiated I/O 지원 수준 |
