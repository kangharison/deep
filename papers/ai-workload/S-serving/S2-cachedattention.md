# [S2] Cost-Efficient Large Language Model Serving for Multi-turn Conversations with CachedAttention

- **학회/연도:** ATC 2024
- **저자:** Bin Gao et al.
- **분류:** LLM 추론/서빙 스토리지

## 핵심 요약 (1~2문장)
멀티턴 대화에서 이전 턴의 KV Cache를 저장하고 재사용하여 LLM 서빙 비용을 줄이는 시스템. 대화 히스토리의 KV Cache를 GPU HBM → Host DRAM → SSD로 계층적으로 관리하여, 이전 턴의 컨텍스트를 재연산 없이 로딩한다.

## 읽기 전 질문
- 멀티턴 대화에서 KV Cache 재사용의 잠재적 절감량은 어느 정도인가?
- KV Cache의 계층적 저장에서 DRAM↔SSD 간 이동 정책은?
- 모델 병렬화(TP/PP) 환경에서 KV Cache 재사용의 복잡도는?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 멀티턴 대화: 사용자가 여러 번 메시지를 주고받음 → 매 턴마다 전체 대화 히스토리를 Prefill
- 대화 길이가 길어질수록 Prefill 비용이 급증
- 이전 턴의 KV Cache는 이미 계산된 것이므로 저장/재사용하면 연산 절약

### 2. 제안 방법 (Approach)

```
턴 1: "안녕하세요" → Prefill → KV Cache 생성 → 응답 + KV Cache 저장
턴 2: "날씨 알려줘" → 저장된 KV Cache 로딩 + 새 토큰만 Prefill → 응답
턴 3: "내일은?" → 저장된 KV Cache 로딩 + 새 토큰만 Prefill → 응답
      (매 턴마다 전체 재연산 불필요)
```

**KV Cache 계층 관리:**
- 활성 요청: GPU HBM
- 대기 중 대화: Host DRAM (빠른 복원)
- 오래된 대화: NVMe SSD (용량 확보)
- 대화 재개 시: SSD → DRAM → GPU로 프리페치

### 3. 실험 결과
- 멀티턴 대화 시나리오에서 Prefill 비용 대폭 절감
- 대화 길이가 길수록 절감 효과 증가
- ATC 2024에서 발표

### 4. 한계점 및 향후 연구
- KV Cache 크기가 대화 길이에 비례하여 증가 → 스토리지 부담
- 대화 간 시간 간격이 길면 저장된 KV Cache의 가치 하락
- KV Cache 정밀도(FP16/FP8/INT8)와 정확도 트레이드오프

## 다른 논문과의 관계
- 보완: [S1] Mooncake (시스템 프롬프트 공유), [S5] IMPRESS (중요도 기반 관리)
- I/O 실측: [S3] NVMe KV 오프로드 (이 시스템이 사용하는 SSD 계층의 I/O 특성)
- gpu-ssd 연결: SSD→GPU KV Cache 로딩에 GDS P2P DMA 적용 가능

## 발표 자료 활용 포인트
- 멀티턴 대화의 KV Cache 재사용 개념이 직관적이고 설명하기 좋음
- 대화 길이별 Prefill 비용 절감 그래프

## 메모
- KV Cache 재사용은 웹 캐싱의 "If-Not-Modified" 패턴과 유사한 발상
- Mooncake과 함께 "KV Cache를 스토리지 자원으로 관리"하는 트렌드의 핵심 논문
