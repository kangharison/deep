# GPU-NVMe 데이터 전송 최적화의 역사: GDS/GPU-Initiated I/O 이전의 노력들

## 왜 이 문서를 만들었나?

GPU와 NVMe Storage 간 데이터를 주고받는 데 **거대한 오버헤드**가 있었다.
이 오버헤드를 극복하기 위해 GDS(GPUDirect Storage)나 GPU-Initiated I/O 같은
기술이 나오기 **이전에** 어떤 시도들이 있었는지를 정리한다.

---

## 1. 원래의 문제: "바운스 버퍼" 경로

전통적인 CPU-GPU-NVMe 시스템에서 GPU가 SSD 데이터를 읽는 과정:

```
NVMe SSD ──DMA──► 커널 버퍼(Page Cache) ──CPU copy──► User 버퍼 ──cudaMemcpy──► GPU VRAM
                   (1차 복사)                (2차 복사)              (3차 전송)
```

**문제점 3가지:**
1. **데이터가 CPU 메모리를 2번 거침** — 불필요한 복사로 대역폭 낭비
2. **CPU가 데이터 전달에 묶임** — 연산에 써야 할 CPU 사이클이 I/O에 소모
3. **GPU 커널이 멈춰야 함** — 데이터 필요 시 GPU 커널 중단 → CPU에 요청 → 대기 → 재시작

이 문제가 얼마나 심각했나?
- [C2] 측정: **CPU 코어의 절반 이상이 I/O 처리에 소모**
- [I1] BaM 측정: coarse-grained prefetch 시 실제 필요 데이터의 **3~10배를 읽음** (읽기 증폭)

---

## 2. 접근법 분류

GDS/GPU-initiated I/O 이전의 최적화 시도를 **5가지 방향**으로 분류:

```
                        GPU-NVMe 오버헤드 문제
                              │
          ┌───────────┬───────┴───────┬──────────────┬──────────────┐
          ▼           ▼               ▼              ▼              ▼
     ① CPU I/O    ② GPU에서     ③ 하드웨어     ④ 메모리 계층   ⑤ P2P DMA
     스택 최적화   직접 접근 시도  아키텍처 제안    통합/오프로드    (GDS 직전)
     (SW 효율↑)   (API 제공)     (HW 변경)      (계층적 관리)    (복사 제거)
```

---

## 3. 각 접근법 상세

### ① CPU I/O 스택 최적화 — "CPU가 하는 일이라도 빠르게 하자"

GPU 문제를 직접 풀기보다, **CPU가 I/O를 처리하는 속도 자체를 극한까지 끌어올리는** 접근.

| 기술 | 핵심 아이디어 | 효과 | 한계 |
|------|-------------|------|------|
| **SPDK** [P7, C4] | 커널 우회, 유저스페이스 NVMe 드라이버, 폴링 모드 | 코어당 10M+ IOPS, 시스템 120M IOPS | CPU 코어 전용 할당 필요, 파일시스템 없음 |
| **io_uring** [C1] | 커널 내 비동기 I/O, 링 버퍼 기반 배치 제출 | libaio 대비 개선, SQPOLL 모드로 syscall 감소 | 커널 오버헤드 여전, SPDK 대비 13배 비효율 |
| **io_passthru** [C3] | 커널 내에서 블록 레이어 우회, NVMe 명령 직접 전달 | io_uring 대비 20~30% IOPS 향상, SPDK의 ~90% | 파일시스템 미통합, raw device만 |

```
성능 스펙트럼:
  POSIX sync    libaio    io_uring    io_passthru    SPDK
  ├──────────────┼──────────┼───────────┼──────────────┤
  ~100K IOPS/core                                 ~10M IOPS/core
                                                  (100x 차이)
```

**이 접근의 근본 한계:**
- CPU I/O가 아무리 빨라져도, **GPU가 데이터를 기다리는 구조 자체는 변하지 않음**
- GPU 커널 중단 → CPU 요청 → SSD 읽기 → CPU 메모리 → GPU 복사의 **왕복 구조**가 남아있음
- CPU 코어를 I/O에 전용으로 쓰면, 그 코어는 다른 일을 못함

### ② GPU에서 스토리지 직접 접근 시도 — "GPU 코드에서 파일을 읽게 하자"

GPU 프로그래머가 GPU 커널 내에서 직접 데이터를 요청할 수 있는 **API를 제공**하는 접근.

| 논문 | 연도 | 핵심 아이디어 | 효과 | 한계 |
|------|------|-------------|------|------|
| **GPUfs** [P1] | 2014 | GPU 커널에서 gopen/gread 호출 가능, CPU 데몬이 실제 I/O 수행 | 프로그래밍 편의, external sort 4x 향상 | **여전히 CPU가 실제 I/O 수행**, CPU 병목 |
| **EMOGI** [P4] | 2021 | GPU가 호스트 메모리를 zero-copy로 PCIe 통해 직접 접근 | UVM 대비 2~8x 향상, PCIe 대역폭 80~90% 활용 | **호스트 DRAM까지만**, SSD 데이터는 불가 |

```
GPUfs (2014):
  GPU 커널 ──요청──► CPU 데몬 ──► File System ──► SSD
                     ▲ 여전히 CPU가 병목

EMOGI (2021):
  GPU 스레드 ──PCIe zero-copy──► CPU DRAM (호스트 메모리)
  수천 스레드가 동시 요청 → PCIe 대역폭 포화시킴
  ▲ 호스트 DRAM에 데이터가 있어야 함 (SSD 직접 접근 불가)
```

**EMOGI의 핵심 통찰** (BaM의 직접적 선행):
- GPU의 수천 개 스레드가 동시에 PCIe 요청을 발생시키면, 개별 요청은 느려도 **집합적으로 대역폭을 포화**시킬 수 있다
- 이 통찰이 BaM으로 이어짐: "호스트 메모리까지 됐으니, SSD까지 확장하자"

### ③ 하드웨어 아키텍처 제안 — "GPU 칩 자체를 바꾸자"

GPU-NVMe 간 직접 경로를 **하드웨어 수준에서 새로 설계**하는 접근.

| 논문 | 연도 | 핵심 아이디어 | 효과 | 한계 |
|------|------|-------------|------|------|
| **NVMMU** [P2] | 2015 | GPU 내에 주소변환 HW 유닛 추가, GPU 가상주소 → SSD 물리주소 직접 매핑 | CPU 경유 대비 2x 전송 속도, CPU 사용률 감소 | **시뮬레이션만**, GPU 칩 수정 필요 |
| **FlashGPU** [P3] | 2019 | 3D NAND 플래시를 GPU 패키지 내부(인터포저)에 직접 배치 | PCIe SSD 대비 레이턴시 4~10x 감소, TB급 확장 | **시뮬레이션만**, 제조 복잡, 비용↑ |

```
NVMMU (2015):
  GPU Core ──► NVMMU (새 HW) ──► NVM/SSD
                 주소변환 + DMA 엔진

FlashGPU (2019):
  ┌─────── GPU Package ───────┐
  │ SM │ SM │ ... │ HBM │ Flash │  ← 같은 패키지 안에!
  └─────── NoC ───────────────┘
  PCIe 오버헤드 완전 제거
```

**이 접근의 의의:**
- GPU-SSD 직접 접근의 **이상적 형태**를 보여줌
- 하지만 **상용 GPU를 바꿔야 하므로** 단기 실현 불가
- BaM은 **기존 상용 하드웨어**에서 소프트웨어적으로 유사한 목표를 달성

### ④ 메모리 계층 통합 / 오프로드 — "GPU-CPU-SSD를 하나의 메모리로 관리하자"

GPU 메모리 부족 문제를 **SSD를 메모리 계층의 일부로 편입**시켜 해결하는 접근.

| 논문 | 연도 | 핵심 아이디어 | 효과 | 한계 |
|------|------|-------------|------|------|
| **FlashNeuron** [P5] | 2021 | DNN 학습 시 activation을 GPU→SSD로 P2P DMA 오프로드 | 배치 4~5x 확장, throughput 10~20% 감소만 | 단일 GPU, 정적 결정, SSD 수명 |
| **ZeRO-Infinity** [P6] | 2021 | GPU+CPU+NVMe 3계층 통합 오프로드 (DeepSpeed) | 32조 파라미터 모델 학습, 500x 모델 확장 | throughput ~50%, NVMe 대역폭 의존 |
| **G10** [O6] | 2023 | GPU VRAM+SSD 통합 주소 공간, 텐서 단위 proactive migration | UVM 대비 3~5x, 메모리 4~16배 확장 | GPU MMU 확장 필요 (HW 변경) |
| **HetCache** [O7] | 2023 | GPU VRAM/CPU DRAM/NVMe SSD 3-tier 캐시 | 쿼리 성능 2~5x, 데이터 10~50배 확장 | 쿼리 워크로드 전용 |

```
메모리 계층 관점:
  ┌─────────┐
  │ GPU VRAM│ ~80GB, ~1TB/s    ← 핫 데이터
  ├─────────┤
  │ CPU DRAM│ ~512GB, ~50GB/s  ← 웜 데이터
  ├─────────┤
  │ NVMe SSD│ ~TB급, ~7GB/s   ← 콜드 데이터
  └─────────┘

핵심 기법: 데이터를 적절한 계층에 배치하고,
필요할 때 미리 상위로 이동 (prefetch/migration)
```

**이 접근의 한계:**
- 결국 **CPU가 데이터 이동을 오케스트레이션**해야 함
- DNN처럼 접근 패턴이 예측 가능한 워크로드에서만 효과적
- **불규칙 접근 패턴** (그래프 분석, 추천 시스템)에서는 예측 실패 → 성능 붕괴

### ⑤ P2P DMA (GDS 직전 단계) — "CPU 메모리를 거치지 말자"

바운스 버퍼를 제거하고 **NVMe→GPU 직접 DMA** 경로를 만드는 접근.
GDS가 여기에 해당하며, BaM 직전의 마지막 단계.

| 논문 | 연도 | 핵심 아이디어 | 효과 | 한계 |
|------|------|-------------|------|------|
| **GDS** [G1] | 2022 | NVIDIA cuFile API로 NVMe→GPU P2P DMA | 바운스 버퍼 대비 최대 2x throughput | **여전히 CPU가 I/O 발행**, cuFile API 전용 |
| **SPIN** [G2] | 2023 | P2P DMA를 Linux 커널에 투명 통합, POSIX API 유지 | 바운스 버퍼 대비 최대 6x 대역폭 | PCIe NTB HW 필요, NVIDIA 독점 드라이버 한계 |

```
전통 경로:        SSD ──► CPU RAM ──► GPU RAM  (2번 복사)
GDS/SPIN:        SSD ═══════════════► GPU RAM  (P2P DMA, 1번)
                         ▲ 하지만 CPU가 "읽어라" 명령을 내림
```

**GDS/SPIN도 해결 못한 것:**
- CPU가 I/O를 발행하는 구조는 동일 → **fine-grained random I/O에 부적합**
- GPU가 4KB 데이터 하나를 원할 때마다 CPU에게 부탁해야 함
- GPU 커널 중단 → CPU 통신 → I/O 발행 → 완료 대기의 **round-trip 오버헤드**

---

## 4. 종합: 각 접근법이 해결한 것과 남긴 것

```
┌────────────────────┬─────────────────────────┬──────────────────────────┐
│     접근법          │    해결한 문제            │    남은 문제              │
├────────────────────┼─────────────────────────┼──────────────────────────┤
│ ① CPU I/O 최적화   │ CPU 측 SW 오버헤드 제거   │ GPU 입장에서 여전히 느림   │
│   (SPDK, io_uring) │ (코어당 100x IOPS 향상)  │ CPU↔GPU 통신 왕복 필요    │
├────────────────────┼─────────────────────────┼──────────────────────────┤
│ ② GPU 파일 API     │ GPU 프로그래밍 편의 향상  │ CPU가 실제 I/O (병목)     │
│   (GPUfs, EMOGI)   │ 호스트 메모리 직접 접근   │ SSD 접근 불가 (EMOGI)     │
├────────────────────┼─────────────────────────┼──────────────────────────┤
│ ③ HW 아키텍처      │ 이상적 GPU-SSD 직접 경로  │ 상용화 불가 (시뮬레이션)   │
│   (NVMMU, FlashGPU)│ PCIe 오버헤드 제거       │ GPU 칩 수정 필요           │
├────────────────────┼─────────────────────────┼──────────────────────────┤
│ ④ 메모리 오프로드   │ GPU 메모리 용량 확장     │ CPU가 오케스트레이션       │
│   (FlashNeuron,    │ 대규모 모델 학습 가능    │ 불규칙 접근에 취약         │
│    ZeRO-Infinity)  │                         │ 예측 가능한 워크로드만     │
├────────────────────┼─────────────────────────┼──────────────────────────┤
│ ⑤ P2P DMA          │ 바운스 버퍼(복사) 제거    │ CPU가 I/O 발행            │
│   (GDS, SPIN)      │ throughput 2~6x 향상    │ fine-grained random 부적합│
└────────────────────┴─────────────────────────┴──────────────────────────┘

모든 접근법의 공통 한계: "CPU가 중간에 끼어있다"
```

---

## 5. 왜 GPU-Initiated I/O (BaM)가 필요했나?

위 모든 시도가 **부분적으로만** 문제를 해결했다. 근본 원인은:

> **GPU가 데이터를 원하는 시점에, CPU의 개입 없이,
> 필요한 만큼만 SSD에서 직접 가져올 수 없다.**

BaM [I1, ASPLOS 2023]이 이 모든 한계를 돌파:

```
BaM 이전 (모든 기존 접근):
  GPU: "데이터 필요해" ──► CPU: "알겠어, 읽어줄게" ──► SSD ──► CPU ──► GPU
                          ▲ CPU가 중재 (병목)

BaM 이후:
  GPU 스레드: NVMe SQ에 직접 커맨드 작성 → doorbell → SSD → GPU VRAM으로 직접 DMA
  ▲ CPU 완전 제외, SPDK가 CPU에서 한 것을 GPU에서 함
```

BaM이 가능했던 이유 = **선행 연구들의 통찰을 조합**:
- GPUfs [P1] → GPU에서 스토리지 접근 가능하다는 개념 증명
- EMOGI [P4] → GPU 스레드의 집합적 대역폭 활용 가능성
- SPDK [P7] → 유저스페이스에서 NVMe 큐 직접 조작 가능
- NVMMU/FlashGPU [P2, P3] → GPU-SSD 직접 접근의 이상적 형태 (목표)
- GDS [G1] → P2P DMA 인프라 (GPUDirect RDMA/BAR1 매핑)

---

## 6. 기술 발전 타임라인

```
2014  GPUfs ──── GPU에서 파일 API (CPU가 실제 I/O)
   │
2015  NVMMU ──── GPU-SSD 직접 접근 HW 제안 (시뮬레이션)
   │
2017  SPDK ───── CPU I/O 극한 최적화 (유저스페이스 NVMe 드라이버)
   │
2019  FlashGPU ─ GPU 패키지 내부에 플래시 배치 (시뮬레이션)
   │
2021  EMOGI ──── GPU→호스트 메모리 zero-copy (PCIe 대역폭 활용 통찰)
   │  FlashNeuron GPU→SSD P2P DMA 텐서 오프로드
   │  ZeRO-Infinity GPU+CPU+NVMe 3계층 통합 메모리
   │
2022  GDS 측정 ─ NVMe→GPU P2P DMA, 바운스 버퍼 제거 (CPU 발행)
   │
2023  io_uring, io_passthru ─ 커널 I/O 스택 최적화 한계 도달
   │  SPIN ──── P2P DMA의 OS 투명 통합
   │  SPDK 120M IOPS ── CPU I/O의 절대 상한
   │  G10 ───── GPU+SSD 통합 주소 공간 (HW 변경)
   │  HetCache ─ 3-tier 이기종 캐시
   │
   ▼
2023  ★ BaM ──── GPU가 NVMe SQ에 직접 커맨드 발행
                  (기존의 모든 한계를 소프트웨어적으로 돌파)
```

---

## 7. 핵심 교훈

1. **CPU I/O 최적화의 한계**: SPDK로 코어당 10M IOPS를 달성해도, GPU 관점에서는 CPU와의 왕복이 병목
2. **복사 제거만으로는 부족**: GDS가 바운스 버퍼를 제거해도, I/O 발행 주체가 CPU이면 fine-grained random I/O에 부적합
3. **HW 변경 없이 달성 가능**: NVMMU/FlashGPU의 이상을 BaM이 기존 상용 HW + 소프트웨어로 실현
4. **GPU의 massive parallelism이 핵심**: 수천 스레드가 동시에 I/O를 발행하면 레이턴시를 hiding할 수 있다는 EMOGI의 통찰이 BaM의 핵심

---

## 관련 논문 목록 (이 문서에서 다룬 논문)

| ID | 논문 | 분류 | 이 문서에서의 역할 |
|----|------|------|-------------------|
| P1 | GPUfs (ASPLOS 2014) | ② GPU 파일 API | GPU 스토리지 접근의 시작 |
| P2 | NVMMU (PACT 2015) | ③ HW 아키텍처 | GPU-SSD 직접 접근 HW 제안 |
| P3 | FlashGPU (DAC 2019) | ③ HW 아키텍처 | GPU 내부 플래시 배치 |
| P4 | EMOGI (VLDB 2021) | ② GPU 직접 접근 | BaM의 직접적 동기 |
| P5 | FlashNeuron (FAST 2021) | ④ 메모리 오프로드 | GPU-SSD P2P DMA 텐서 오프로드 |
| P6 | ZeRO-Infinity (SC 2021) | ④ 메모리 오프로드 | GPU+CPU+NVMe 통합 |
| P7 | SPDK (CloudCom 2017) | ① CPU I/O 최적화 | BaM의 철학적 원류 |
| C1 | Storage Stacks (CHEOPS 2023) | ① CPU I/O 최적화 | I/O 스택 비교 |
| C2 | Modern NVMe (VLDB 2023) | ① CPU I/O 최적화 | CPU I/O 오버헤드 정량화 |
| C3 | I/O Passthru (FAST 2024) | ① CPU I/O 최적화 | 커널 내 최적화 한계 |
| C4 | 120M IOPS (SPDK 2023) | ① CPU I/O 최적화 | CPU I/O의 절대 상한 |
| G1 | GDS 성능 측정 (NAS 2022) | ⑤ P2P DMA | 바운스 버퍼 제거 효과 |
| G2 | SPIN (ATC 2023) | ⑤ P2P DMA | P2P DMA OS 통합 |
| O6 | G10 (MICRO 2023) | ④ 메모리 오프로드 | 통합 주소 공간 |
| O7 | HetCache (CIDR 2023) | ④ 메모리 오프로드 | 3-tier 이기종 캐시 |
| I1 | **BaM** (ASPLOS 2023) | **결론** | **모든 한계를 돌파** |
