# [O1] SSDTrain: An Activation Offloading Framework to SSDs for Faster Large Language Model Training

- **학회/연도:** arXiv 2024
- **저자:** Wu et al.
- **분류:** GPU-NVMe 오프로드

## 핵심 요약 (1~2문장)
LLM 학습 시 forward pass에서 생성되는 activation 텐서를 NVMe SSD로 오프로드하여 GPU 메모리 부족 문제를 해결하는 프레임워크. NVIDIA GPUDirect Storage(GDS)를 활용하여 CPU를 우회하는 GPU-SSD 직접 전송으로 오프로드 오버헤드를 최소화하고, tensor deduplication과 forwarding 기법으로 I/O 양을 줄임.

## 읽기 전 질문
- GDS(GPUDirect Storage)를 활용한 GPU-SSD 직접 전송이 CPU 경유 방식 대비 얼마나 빠른가?
- Activation 오프로드 시 어떤 텐서를 SSD로 보내고, 어떤 텐서는 GPU에 남기는 결정 기준은 무엇인가?
- Recomputation(재계산) 방식과 비교했을 때 SSD 오프로드의 학습 속도 이점은 어느 정도인가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- LLM 학습 시 activation 메모리가 모델 크기, 배치 크기, 시퀀스 길이에 비례하여 급증
- GPU VRAM이 부족하여 큰 배치/시퀀스로 학습이 불가능
- 기존 해결책의 한계:
  - **Activation recomputation**: backward pass에서 activation을 다시 계산 → GPU 연산 오버헤드 30~40% 증가
  - **CPU 오프로드**: PCIe 대역폭이 병목, CPU 메모리도 제한적
  - **FlashNeuron**: CPU 경유 SSD 오프로드 → 불필요한 CPU-GPU 복사 오버헤드 발생

### 2. 제안 방법 (Approach)
- **GPUDirect Storage 활용**: GPU VRAM ↔ NVMe SSD 간 DMA 직접 전송 (CPU 메모리 바이패스)
- **Tensor Deduplication**: 동일한 activation 텐서가 여러 레이어에서 중복 저장되는 것을 탐지하고 제거
- **Tensor Forwarding**: 이전 레이어의 출력이 다음 레이어의 입력과 동일할 때, SSD에서 직접 다음 레이어로 전달
- **비동기 파이프라이닝**: forward pass 계산과 SSD 쓰기를 오버랩, backward pass 계산과 SSD 읽기를 오버랩
- **선택적 오프로드**: 텐서 크기와 생존 시간(lifetime)을 기준으로 오프로드할 텐서를 결정하는 정책

### 3. 핵심 아키텍처/설계

```
  Forward Pass                          Backward Pass
  ┌─────────┐                          ┌─────────┐
  │  Layer i │──activation──┐     ┌────│  Layer i │
  └────┬─────┘              │     │    └────┬─────┘
       │              ┌─────▼─────┴───┐     │
       │              │  GDS Engine   │     │
       │              │ (async DMA)   │     │
       │              └─────┬─────────┘     │
       │                    │               │
       │              ┌─────▼─────┐         │
       │              │ NVMe SSD  │         │
       │              │ (저장/복원)│         │
       │              └───────────┘         │
  ┌────▼─────┐                         ┌────▼─────┐
  │ Layer i+1│                         │Layer i-1 │
  └──────────┘                         └──────────┘

  기존 CPU 경유 방식 vs SSDTrain GDS 방식:
  ┌─────┐    PCIe     ┌─────┐    PCIe    ┌─────┐
  │ GPU │◄──────────►│ CPU │◄──────────►│ SSD │  (기존: 2-hop)
  └─────┘            └─────┘            └─────┘

  ┌─────┐         PCIe (GDS)           ┌─────┐
  │ GPU │◄─────────────────────────────►│ SSD │  (SSDTrain: 1-hop)
  └─────┘                              └─────┘
```

- **Deduplication 메커니즘**: PyTorch의 autograd 그래프를 분석하여 동일 텐서 참조를 식별
- **Prefetch 스케줄러**: backward pass에서 필요한 activation을 미리 SSD에서 GPU로 로드

### 4. 실험 결과 (Key Results)
- **환경**: NVIDIA A100 GPU, NVMe SSD (Samsung PM9A3 등)
- GPT-2, LLaMA 등의 모델에서 실험
- Activation recomputation 대비 **최대 2.5x 학습 속도 향상**
- CPU 오프로드(ZeRO-Offload) 대비 **약 1.5~2x 성능 향상**
- GDS 직접 전송으로 CPU 경유 대비 **대역폭 약 40~60% 향상**
- Tensor deduplication으로 **SSD I/O 양 약 25~30% 감소**
- GPU 메모리 사용량을 최대 **47% 절감**하면서도 학습 성능 저하 최소화

### 5. 한계점 및 향후 연구
- NVIDIA GPU + GDS 지원 드라이버가 필수 → AMD GPU 등에서 사용 불가
- SSD 수명(endurance) 문제: 반복적인 대량 쓰기로 인한 SSD 마모
- 단일 노드 환경에 초점 → 분산 학습 시나리오 확장 필요
- NVMe SSD의 대역폭이 GPU HBM 대비 여전히 낮아, 매우 큰 모델에서는 병목 가능
- SSD 성능 변동(thermal throttling 등)에 따른 학습 시간 불안정성

## 다른 논문과의 관계
- 선행 연구: FlashNeuron [P5] (최초 SSD 오프로드, CPU 경유), ZeRO-Infinity [P6] (CPU+NVMe 통합 오프로드)
- 후속 연구: TERAIO [O2] (SSD 수명 인식 오프로드로 발전)
- 비교 대상: [O3] (NVMe I/O 특성 분석), [O5] (LoHan, 메모리+SSD 혼합)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: GPU-SSD 직접 전송 vs CPU 경유 아키텍처 비교도, Tensor deduplication 효과 그래프
- 핵심 수치/데이터: recomputation 대비 2.5x 속도 향상, GPU 메모리 47% 절감, GDS로 대역폭 40~60% 향상

## 메모
