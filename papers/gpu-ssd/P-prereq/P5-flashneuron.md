# [P5] FlashNeuron: SSD-Enabled Large-Batch Training of Very Deep Neural Networks

- **학회/연도:** FAST 2021
- **저자:** Bae et al.
- **분류:** 선행 연구 (Pre-2023)

## 핵심 요약 (1~2문장)
NVMe SSD를 DNN 학습의 백업 스토어로 사용하는 최초 시스템. GPU-SSD 간 P2P DMA를 활용한 텐서 오프로드.

## 읽기 전 질문
- GPU-SSD 간 P2P DMA를 활용한 텐서 오프로드 시 학습 throughput 저하는 얼마나 되는가?
- 어떤 텐서를 오프로드할지 결정하는 정책은 무엇인가?
- CPU 메모리 오프로드 대비 SSD 오프로드의 장점과 단점은?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 대규모 DNN 학습 시 GPU 메모리 부족이 심각한 병목
  - 배치 크기를 줄이면 학습 수렴 속도와 최종 정확도에 악영향
  - 모델 크기 자체가 GPU 메모리를 초과하는 경우도 증가
- 기존 해결책의 한계:
  - **CPU 메모리 오프로드**: CPU DRAM 용량도 제한적 (보통 GPU 메모리의 2~4배)
  - **모델 병렬화**: 통신 오버헤드와 프로그래밍 복잡도
  - **Gradient checkpointing**: 재계산 오버헤드로 학습 시간 증가

### 2. 제안 방법 (Approach)
- **FlashNeuron**: NVMe SSD를 DNN 학습의 텐서 오프로드 대상으로 활용하는 최초 시스템
- GPU → SSD로 비활성 텐서를 P2P DMA로 직접 전송 (CPU 메모리 경유 없음)
- 학습 반복의 forward pass에서 생성된 activation 텐서를 SSD로 오프로드, backward pass에서 필요할 때 다시 로드
- 오프로드 비용 모델 기반으로 어떤 텐서를 오프로드할지 자동 결정

```
Forward Pass:
  Layer 1 → Layer 2 → ... → Layer N → Loss
    |          |                |
    v          v                v
  [SSD에 오프로드]  [SSD에 오프로드]  [GPU에 유지]
    (P2P DMA)       (P2P DMA)

Backward Pass:
  ∂Loss/∂N → ... → ∂Loss/∂2 → ∂Loss/∂1
                       ^          ^
                       |          |
                   [SSD에서 로드]  [SSD에서 로드]
                    (P2P DMA)      (P2P DMA)
```

### 3. 핵심 아키텍처/설계
- **P2P DMA 경로**: GDRCopy/GPUDirect를 활용하여 GPU↔SSD 간 CPU 경유 없이 직접 전송
- **오프로드 스케줄러**:
  - 프로파일링 단계에서 각 레이어의 텐서 크기와 생존 시간 분석
  - 비용 모델: 오프로드/로드 시간 vs 메모리 절약량의 트레이드오프
  - 오프로드 효과가 큰 텐서(큰 크기, 긴 비활성 구간)를 우선 선택
- **연산-전송 오버랩**: 텐서 오프로드/로드를 GPU 연산과 파이프라인하여 오버헤드 숨김
- **압축**: 오프로드 전 텐서를 GPU에서 압축하여 SSD I/O 양 감소
- **메모리 관리**: GPU 메모리 풀을 관리하여 오프로드된 공간을 다른 텐서에 재사용

### 4. 실험 결과 (Key Results)
- **배치 크기 확장**: GPU 메모리 한계 대비 **최대 4~5x 큰 배치** 사용 가능
- VGGNet, ResNet 등에서 CPU 오프로드 대비 **유사하거나 더 나은 성능**
- P2P DMA로 CPU 메모리 병목 회피: CPU DRAM 대역폭을 소모하지 않음
- 학습 throughput 저하: 오프로드 없는 경우 대비 **약 10~20% 감소** (오버랩 최적화 적용 시)
- 압축으로 SSD I/O 양 **30~50% 감소**
- SSD 수명: 일반적 학습 시나리오에서 엔터프라이즈 SSD 기준 수년 이상 사용 가능

### 5. 한계점 및 향후 연구
- **SSD 대역폭 한계**: NVMe SSD(~3.5GB/s)는 HBM(~900GB/s) 대비 매우 느림 → 오프로드 양에 한계
- **P2P DMA 호환성**: 모든 GPU-SSD 조합에서 P2P DMA가 지원되지 않을 수 있음 (PCIe 토폴로지 의존)
- **단일 GPU 전용**: 분산 학습 환경에서의 확장은 다루지 않음
- **SSD 수명 우려**: 대규모 학습 시 쓰기 양이 많아 SSD 마모 가속 가능
- **정적 오프로드 결정**: 프로파일링 기반 정적 결정으로 동적 변화에 대응 부족
- 향후: SSDTrain [O1], Fuyou [O4]에서 분산 환경과 더 정교한 스케줄링으로 발전

## 다른 논문과의 관계
- 선행 연구: vDNN (CPU 오프로드), GPUDirect/GDRCopy (P2P DMA 인프라)
- 후속 연구: SSDTrain [O1], TERAIO [O2], Fuyou [O4] - SSD 오프로드의 발전
- 비교 대상: ZeRO-Infinity [P6] (CPU+NVMe 통합 오프로드, 분산 학습)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: P2P DMA 데이터 경로도, 배치 크기별 throughput 비교, 오프로드 스케줄링 알고리즘
- 핵심 수치/데이터: 배치 크기 4~5x 확장, throughput 10~20% 감소, 압축으로 I/O 30~50% 절감
- **활용 포인트**: GPU-SSD P2P DMA를 DNN 학습에 적용한 최초 연구로, SSD 오프로드 계열 논문의 출발점

## 메모
