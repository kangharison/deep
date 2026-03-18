# [T1] DataStates-LLM: Lazy Asynchronous Checkpointing for Large Language Models

- **학회/연도:** HPDC 2024
- **저자:** Avinash Maurya et al. (Argonne National Laboratory)
- **분류:** 트레이닝 스토리지 최적화

## 핵심 요약 (1~2문장)
LLM 트레이닝의 체크포인트 I/O 오버헤드를 줄이기 위해, State Provider 추상화를 도입하고 모델 파라미터의 불변성(forward/backward pass 중 변하지 않음)을 활용한 lazy 비동기 체크포인트 아키텍처. DeepSpeed/Megatron과 통합되어 체크포인트 I/O를 트레이닝과 중첩(overlap)시킨다.

## 읽기 전 질문
- "Lazy" 체크포인트란 무엇이며, 기존 동기 체크포인트와 어떻게 다른가?
- State Provider 추상화가 체크포인트 시스템 설계에 주는 이점은?
- 비동기 체크포인트의 일관성(consistency)은 어떻게 보장하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 동기 체크포인트: 모든 GPU가 트레이닝을 멈추고 파라미터+옵티마이저 상태를 디스크에 기록 → GPU idle
- LLM 규모에서 체크포인트 크기: 수 TB → 쓰기에 수 분 소요
- 장애 빈도가 높아([W2] 참조) 체크포인트 주기를 줄이고 싶지만, 매번 GPU가 멈추므로 비용 증가

### 2. 제안 방법 (Approach)

```
기존 동기 체크포인트:
  Step N ──[학습]──→ STOP ──[체크포인트 쓰기]──→ Step N+1 ──[학습]──→
                     ↑ GPU idle (수 분)

DataStates-LLM (비동기):
  Step N ──[학습]──→ Step N+1 ──[학습]──→ Step N+2 ──→ ...
                │
                └──[백그라운드: 스냅샷 → 스토리지 쓰기]──→ 완료
                    (GPU 메모리 → CPU 메모리 → 디스크, 비차단)
```

**핵심 설계:**
- **State Provider**: 체크포인트 대상(모델 파라미터, 옵티마이저 상태, 학습률 스케줄러 등)을 추상화. 각 상태의 불변 구간을 활용하여 lazy 복사
- **Lazy snapshot**: Forward/backward pass 중 파라미터는 변하지 않으므로, 이 구간에 GPU→CPU 메모리 복사를 비차단으로 수행
- **파이프라인**: GPU→CPU 복사와 CPU→디스크 쓰기를 파이프라인화

### 3. 실험 결과
- DeepSpeed + Megatron-LM 기반 LLM 트레이닝에서 검증
- 체크포인트 오버헤드를 **최소 수십 배** 절감 (동기 대비)
- 트레이닝 throughput 손실 최소화

### 4. 한계점 및 향후 연구
- CPU 메모리가 충분해야 스냅샷 버퍼링 가능
- 매우 빈번한 체크포인트(매 step)는 GPU→CPU 대역폭 병목 가능
- 멀티노드 분산 체크포인트의 글로벌 일관성 보장 복잡도

## 다른 논문과의 관계
- 동기 부여: [W2] 장애 빈도, [W3] 체크포인트 실측
- 비교/발전: [T2] PCcheck (ASPLOS 2025, persistent memory 활용), [T4] LMStor (계층적 접근)
- gpu-ssd 연결: GPU→SSD 직접 체크포인트에 GDS P2P DMA 활용 가능성

## 발표 자료 활용 포인트
- 동기 vs 비동기 체크포인트 비교 다이어그램
- "lazy"의 의미: 파라미터가 변하지 않는 구간을 활용

## 메모
- 오픈소스: https://github.com/datastates/datastates-llm
- SPDK의 비동기 I/O 철학과 유사: 블로킹을 최소화하고 I/O를 백그라운드로 이동
