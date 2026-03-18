# [S5] IMPRESS: An Importance-Informed Multi-Tier Prefix KV Storage System for Large Language Model Inference

- **학회/연도:** FAST 2025
- **저자:** (연구팀 미상)
- **분류:** LLM 추론/서빙 스토리지

## 핵심 요약 (1~2문장)
LLM 추론에서 Prefix KV Cache를 중요도(importance) 기반으로 다계층 스토리지(GPU HBM, Host DRAM, NVMe SSD)에 분배하여 관리하는 시스템. 자주 재사용되고 중요한 KV Cache는 빠른 계층에, 그렇지 않은 것은 느린 계층에 배치하여 비용-성능 균형을 최적화한다.

## 읽기 전 질문
- KV Cache의 "중요도"를 어떻게 정의하고 측정하는가?
- Prefix KV Cache란 무엇이며, 왜 다계층 관리가 필요한가?
- 기존 LRU 등 전통적 캐시 교체 정책과의 차이는?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- Prefix KV Cache: 공통 시스템 프롬프트, 대화 히스토리 등 여러 요청이 공유하는 KV Cache prefix
- 모든 prefix를 GPU HBM에 유지할 수 없음 → 다계층 저장 필요
- 단순 LRU는 prefix의 "재사용 가치"를 반영하지 못함

### 2. 제안 방법 (Approach)

**중요도(Importance) 기반 계층 배치:**
```
높은 중요도                               낮은 중요도
  │                                          │
  ▼                                          ▼
┌──────────┐    ┌──────────┐    ┌──────────┐
│ GPU HBM  │    │Host DRAM │    │NVMe SSD  │
│ (빠름,   │    │ (중간,   │    │ (느림,   │
│  비쌈,   │    │  보통,   │    │  쌈,     │
│  작음)   │    │  중간)   │    │  큼)     │
└──────────┘    └──────────┘    └──────────┘
```

**중요도 결정 요인:**
- 재사용 빈도: 많은 요청이 공유하는 prefix일수록 중요
- 길이: 긴 prefix일수록 재연산 비용이 높으므로 저장 가치 높음
- 최근성: 활성 대화의 KV Cache가 더 중요

### 3. 실험 결과
- FAST 2025에서 발표 — Mooncake과 함께 KV Cache 스토리지 관리의 양대 논문
- 중요도 기반 배치가 LRU 대비 hit rate와 서빙 성능 모두 향상

### 4. 한계점 및 향후 연구
- 중요도 예측의 정확도에 성능이 의존
- 계층 간 데이터 이동(promotion/demotion) 오버헤드
- 워크로드 패턴 변화에 대한 적응 속도

## 다른 논문과의 관계
- 보완: [S1] Mooncake (KVCache-centric 서빙의 구체적 스토리지 관리 기법)
- 보완: [S2] CachedAttention (멀티턴 대화 특화)
- I/O 기반: [S3] NVMe KV 오프로드 (NVMe 계층의 I/O 특성 이해)
- gpu-ssd 연결: NVMe 계층에서 GPU로의 KV Cache 로딩에 GDS/BaM 활용 가능

## 발표 자료 활용 포인트
- 전통적 스토리지 계층화(Hot/Warm/Cold) 개념이 KV Cache에 직접 적용되는 사례
- "캐시의 캐시를 관리한다"는 메타 레벨의 스토리지 문제

## 메모
- 스토리지 커뮤니티의 전통적 주제(계층화, 캐시 교체 정책)가 LLM 시대에 새로운 형태로 부활
- FAST 2025에서 Mooncake + IMPRESS가 동시 발표 → KV Cache 스토리지가 정식 연구 분야로 자리잡음
