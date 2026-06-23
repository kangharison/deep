# mini-vLLM 분석 문서 인덱스

## mini-vLLM이란

mini-vLLM은 [jianzhnie](https://github.com/jianzhnie/mini-vllm)이 공개한 경량 LLM 추론 엔진으로, 산업용 고성능 서빙 시스템인 vLLM의 핵심 아이디어를 "읽을 수 있는 코드"로 밑바닥부터 재구현한 교육용 프로젝트다. Python 3.10–3.12, PyTorch, HuggingFace Transformers 위에서 동작하며, 가독성을 최우선으로 하여 PagedAttention, Prefix Caching, Tensor Parallelism, CUDA Graph, FlashAttention 같은 LLM 서빙 핵심 기법을 약 9,700줄의 코드에 압축해 담았다.

스터디 저장소는 원본 코드에 모든 함수·구조체·실행 라인에 한국어 주석을 추가하여 "주석만 읽어도 LLM 추론 엔진의 전체 아키텍처를 이해할 수 있는" 수준을 목표로 한다.

## 참조

- 스터디 저장소: <https://github.com/kangharison/mini-vllm-study>
- 원본 저장소: <https://github.com/jianzhnie/mini-vllm>
- 라이선스: Apache 2.0
- 지원 모델: Qwen2, Qwen3, OPT, GPT2

## 문서 목록

| 파일 | 설명 |
|------|------|
| [[01-architecture-overview]] | mini-vLLM 전체 아키텍처 개요 — 계층 구조, 실행 흐름, 핵심 개념 7가지, 43개 파일 역할 표 |
| [[02-paged-attention-block-manager]] | PagedAttention & BlockManager 심층 분석 — 블록 구조, KV 캐시 텐서 형태, 프리픽스 캐싱, CoW |

## mini-vLLM 학습 권장 순서

### 1단계: 개념 정립 (이론)
1. [[01-architecture-overview]] "핵심 개념 7가지" 섹션 읽기
   - Prefill / Decode 2단계 이해
   - KV 캐시와 PagedAttention 개념
   - CUDA Graph, Tensor Parallelism, FlashAttention 개요

### 2단계: 최상위 흐름 (코드 입문)
2. `mini-vllm-study/minivllm/llm.py` — 사용자 진입점 (30줄, 이해 쉬움)
3. `mini-vllm-study/minivllm/engine/llm_engine.py` — 전체 루프 오케스트레이션
4. `mini-vllm-study/minivllm/engine/scheduler.py` — Prefill/Decode 스케줄링 정책

### 3단계: 메모리 관리 (핵심)
5. [[02-paged-attention-block-manager]] 문서 정독
6. `mini-vllm-study/minivllm/engine/block_manager.py` — BlockManager 구현
7. `mini-vllm-study/minivllm/engine/sequence.py` — Sequence 상태 단위

### 4단계: GPU 실행 계층
8. `mini-vllm-study/minivllm/engine/inference_executor.py` — KV 캐시 텐서, CUDA Graph
9. `mini-vllm-study/minivllm/models/layers/attention.py` — 어텐션 + KV 캐시 저장
10. `mini-vllm-study/minivllm/models/layers/attention_backend.py` — FlashAttention 커널

### 5단계: 모델·샘플링·분산 (완성)
11. `mini-vllm-study/minivllm/models/qwen_base.py` — 모델 구현 대표 예시
12. `mini-vllm-study/minivllm/sampling/functional.py` — temperature/top-k/top-p 파이프라인
13. `mini-vllm-study/minivllm/engine/distributed_manager.py` — TP 통신
14. `mini-vllm-study/minivllm/engine/model_runner.py` — Rank 0 / worker 분기
