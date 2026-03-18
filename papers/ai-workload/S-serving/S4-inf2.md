# [S4] INF²: High-Throughput Generative Inference of Large Language Models using Near-Storage Processing

- **학회/연도:** arXiv 2025 (2502.09921)
- **저자:** (KAIST 등)
- **분류:** LLM 추론/서빙 스토리지

## 핵심 요약 (1~2문장)
CSD(Computational Storage Device)를 활용하여 LLM 추론의 Self-Attention 연산을 NVMe SSD 측에서 수행하는 Near-Storage Processing 시스템. KV Cache를 SSD로 이동시키는 대신 SSD 내부에서 Attention을 계산하여 PCIe 데이터 전송량을 극적으로 줄인다.

## 읽기 전 질문
- Attention 연산의 어떤 특성이 Near-Storage Processing에 적합한가?
- CSD의 연산 능력으로 Attention을 실시간 처리할 수 있는가?
- GPU에서 모든 연산을 하는 것 대비 실제 throughput 향상은?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- LLM 추론에서 KV Cache가 GPU HBM을 초과 → SSD로 오프로드 필요
- 기존 오프로드: SSD에서 KV Cache를 읽어 GPU로 전송 → GPU에서 Attention 연산
- 문제: KV Cache 전체를 GPU로 전송하는 PCIe 대역폭이 병목

### 2. 제안 방법 (Approach)

```
기존 KV Cache 오프로드:
  SSD ──[KV Cache 전체 전송]──→ GPU ──[Attention 연산]──→ 결과

INF² (Attention-near-Storage):
  SSD+가속기 ──[내부에서 Attention 연산]──→ 결과만 GPU로 전송
  (전송량: KV Cache 전체 → Attention 결과 벡터만)
```

**핵심 혁신: Attention-Near-Storage**
- Self-Attention: `softmax(Q·K^T/√d)·V` 연산에서 K, V가 KV Cache
- Q(Query)만 GPU→CSD로 전송 (작음: 1개 토큰 × head_dim)
- K, V는 CSD 내부에 이미 있음 (SSD에 저장된 KV Cache)
- Attention 결과만 CSD→GPU로 반환 (작음: 1개 토큰 × head_dim)
- **데이터 이동량: O(seq_len × d) → O(d)** — 시퀀스 길이에 무관

**구현:**
- Off-the-shelf CSD 컴포넌트로 실제 시스템 구현 (시뮬레이션이 아님)
- CSD 내부에 맞춤 Attention 가속기 설계
- 여러 CSD를 병렬 연결하여 KV Cache를 분산 저장

### 3. 실험 결과
- 기존 KV Cache 오프로드 대비 throughput 대폭 향상
- 특히 long-context(긴 시퀀스)에서 효과 극대화 — KV Cache가 클수록 전송 절약 효과 ↑
- 실제 하드웨어에서 검증

### 4. 한계점 및 향후 연구
- CSD의 Attention 가속기 설계가 특정 모델 아키텍처에 최적화
- MHA/GQA/MQA 등 다양한 Attention 변형에 대한 범용성 필요
- CSD 간 KV Cache 파티셔닝 전략이 성능에 영향

## 다른 논문과의 관계
- 유사 방향: [T3] Smart-Infinity (Near-Storage for Training), [G1] BeaconGNN (In-Storage GNN)
- 보완: [S3] NVMe KV 오프로드 (INF²가 해결하려는 I/O 병목의 실측)
- gpu-ssd 연결: BaM [I1]이 "GPU→SSD I/O"를 최적화했다면, INF²는 "I/O 자체를 없애는" 접근

## 발표 자료 활용 포인트
- "데이터를 이동시키지 말고, 연산을 데이터로" — Near-Data Processing의 극치
- KV Cache 전송량 O(seq_len × d) → O(d) 절감이 매우 직관적
- [T3] Smart-Infinity(트레이닝)와 INF²(추론)를 함께 보여주면 Near-Storage AI의 양면 제시

## 메모
- BaM 계열(GPU→SSD 직접 접근)과 INF² 계열(SSD에서 직접 연산)은 같은 문제(데이터 이동 병목)의 두 가지 접근
  - BaM: I/O 경로를 최적화 (CPU 우회)
  - INF²: I/O 자체를 제거 (연산을 데이터 측으로)
- 장기적으로 두 접근의 하이브리드가 나올 가능성
