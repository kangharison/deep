# [P6] ZeRO-Infinity: Breaking the GPU Memory Wall with Heterogeneous System Offload

- **학회/연도:** SC 2021
- **저자:** Rajbhandari et al.
- **분류:** 선행 연구 (Pre-2023)

## 핵심 요약 (1~2문장)
GPU, CPU, NVMe를 통합한 이기종 오프로드로 조 단위 파라미터 모델 학습 가능. DeepSpeed에 통합됨.

## 읽기 전 질문
- ZeRO 시리즈(ZeRO-1/2/3)에서 Infinity로의 핵심 발전은 무엇인가?
- NVMe 오프로드 시 학습 throughput을 유지하기 위한 핵심 기술은?
- 조(trillion) 단위 파라미터 모델을 단일 클러스터에서 학습할 수 있는 이유는?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 초대규모 모델(GPT-3: 175B, 그 이상) 학습 시 GPU 메모리가 절대적으로 부족
- ZeRO-3도 모델 상태(파라미터, 그래디언트, 옵티마이저 상태)를 GPU 간 분산하지만, GPU 메모리 총량에 제한
- 모델 병렬화(MP)는 통신 오버헤드가 크고 프로그래밍이 복잡
- **GPU 메모리 벽(Memory Wall)**: GPU 수를 늘려도 모델 크기 확장에 한계

### 2. 제안 방법 (Approach)
- **ZeRO-Infinity**: GPU, CPU 메모리, NVMe SSD를 모두 활용하는 이기종(heterogeneous) 메모리 시스템
- ZeRO-3의 파티셔닝을 CPU 메모리와 NVMe SSD까지 확장
- 5가지 핵심 기술:
  1. **Infinity offload engine**: NVMe로의 비동기 오프로드
  2. **Memory-centric tiling**: 대규모 연산을 GPU 메모리에 맞는 타일로 분할
  3. **Bandwidth-centric partitioning**: 통신 대역폭 최적화를 위한 파티셔닝
  4. **Overlap-centric design**: 연산/통신/I/O 오버랩
  5. **Ease-of-use**: 데이터 병렬화 수준의 간단한 사용성

```
+--------------------------------------------------+
|              ZeRO-Infinity 메모리 계층             |
|                                                    |
|  +----------+   +----------+   +----------+       |
|  |  GPU 0   |   |  GPU 1   |   |  GPU N   |       |
|  | (HBM)    |   | (HBM)    |   | (HBM)    |       |
|  | 활성 파라 |   | 활성 파라 |   | 활성 파라 |       |
|  +----+-----+   +----+-----+   +----+-----+       |
|       |              |              |              |
|  +----v--------------v--------------v-----+        |
|  |           CPU DRAM (각 노드)            |        |
|  |  파라미터 + 그래디언트 일부 저장         |        |
|  +----+-------------------------------+---+        |
|       |                               |            |
|  +----v-------------------------------v---+        |
|  |           NVMe SSD (각 노드)            |        |
|  |  옵티마이저 상태 + 파라미터 전체 저장    |        |
|  |  (TB급 용량 활용)                       |        |
|  +----------------------------------------+        |
+--------------------------------------------------+
```

### 3. 핵심 아키텍처/설계
- **Infinity Offload Engine**:
  - 옵티마이저 상태를 NVMe SSD에 저장 (가장 큰 메모리 소비 요소)
  - 파라미터와 그래디언트는 CPU 메모리 또는 NVMe에 계층적으로 배치
  - 비동기 읽기/쓰기로 I/O 레이턴시를 연산으로 숨김
- **Memory-Centric Tiling**:
  - 단일 레이어의 파라미터도 GPU 메모리에 안 들어갈 수 있음
  - 파라미터를 타일 단위로 분할하여 순차적으로 GPU에 로드/연산/언로드
- **Bandwidth-Centric Partitioning**:
  - 각 디바이스의 대역폭에 비례하여 데이터 파티셔닝
  - NVMe 대역폭이 낮으므로 자주 접근하는 데이터는 상위 계층에 배치
- **Prefetching & Overlapping**:
  - Forward/backward 순서를 예측하여 다음에 필요한 데이터를 미리 로드
  - GPU 연산, NVMe I/O, 노드 간 통신을 동시에 파이프라인

### 4. 실험 결과 (Key Results)
- **조(trillion) 단위 모델 학습 가능**: 32개 GPU(8노드)에서 **32조 파라미터** 모델 학습 시연
- GPU만 사용하는 경우 대비 **500x 이상 큰 모델** 학습 가능
- 학습 throughput: NVMe 오프로드를 사용해도 GPU만 사용하는 최적 설정 대비 **약 50% 이상의 throughput 유지**
- NVMe SSD 대역폭 활용: 노드당 다수의 NVMe SSD를 RAID-0로 묶어 집합 대역폭 확보
- DeepSpeed 라이브러리에 통합되어 **코드 변경 최소화**로 사용 가능

### 5. 한계점 및 향후 연구
- **NVMe 대역폭 의존**: 노드당 NVMe SSD 수와 대역폭이 성능을 좌우
- **학습 속도 저하**: 오프로드로 인한 throughput 감소는 피할 수 없음 (GPU 전용 대비)
- **SSD 수명**: 대규모 학습의 반복적 쓰기로 SSD 마모 우려
- **단일 프레임워크 의존**: DeepSpeed/PyTorch 생태계에 강하게 결합
- **추론(inference)에는 미적용**: 학습 전용 최적화
- 향후: Fuyou [O4]에서 GPU-SSD P2P DMA 활용, MLP-Offload [O8]에서 추론 오프로드

## 다른 논문과의 관계
- 선행 연구: ZeRO-1/2/3 (DeepSpeed), L2L (Layer-to-Layer 오프로드)
- 후속 연구: Fuyou [O4] (GPU-SSD 직접 경로), MLP-Offload [O8] (추론 오프로드)
- 비교 대상: FlashNeuron [P5] (단일 GPU, SSD P2P DMA 특화)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: 메모리 계층 다이어그램, 모델 크기 vs GPU 수 스케일링 그래프, throughput 비교 테이블
- 핵심 수치/데이터: 32조 파라미터 모델 학습, GPU 대비 500x 모델 크기 확장, throughput 50%+ 유지
- **활용 포인트**: NVMe SSD를 학습 메모리 계층에 통합한 대표적 산업 시스템으로, FlashNeuron과의 접근 방식 차이(P2P DMA vs CPU 경유 오프로드) 비교에 활용

## 메모
