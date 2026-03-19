# [S5] IMPRESS: An Importance-Informed Multi-Tier Prefix KV Storage System for Large Language Model Inference

- **학회/연도:** FAST 2025, pp. 187-201
- **저자:** Weijian Chen, Shuibing He, Haoyang Qu, Ruidong Zhang, Siling Yang, Ping Chen, Gang Chen (Zhejiang Univ.), Yi Zheng, Baoxing Huai (Huawei Cloud)
- **분류:** LLM 추론/서빙 스토리지

## 핵심 요약 (1~2문장)
LLM 추론에서 prefix KV Cache를 NVMe SSD에 저장할 때, 전체를 로딩하는 대신 **중요한(important) KV만 선택적으로 로딩**하여 디스크 I/O 지연을 줄이는 다계층 스토리지 시스템. 어텐션 헤드 간 중요 토큰 인덱스의 유사성을 활용한 I/O 효율적 식별 알고리즘으로, TTFT를 기존 대비 **최대 2.8× 절감**하면서 추론 정확도를 유지한다.

## 읽기 전 질문
- KV Cache의 "중요도"를 어떻게 정의하고 효율적으로 측정하는가?
- 전체 KV Cache 대신 일부만 로딩해도 추론 정확도가 유지되는 이유는?
- Mooncake, CachedAttention과의 차별점은?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- Prefix KV Cache: 시스템 프롬프트, 공통 컨텍스트 등 여러 요청이 공유하는 prefix의 KV Cache
- CPU DRAM 부족 시 NVMe SSD에 저장 → 재사용 시 디스크에서 로딩 필요
- **핵심 문제: 디스크 I/O 지연이 높아 prefix KV 재사용이 오히려 TTFT를 악화시킬 수 있음**
- 모든 prefix KV를 로딩하면 I/O 지연 > 재연산 시간인 경우 발생

### 2. 핵심 관찰: 중요한 KV만 있으면 된다

**Attention의 희소성:**
- Self-Attention에서 대부분의 가중치가 소수의 "중요한" 토큰에 집중
- 나머지 토큰의 KV는 결과에 미미한 영향 → 로딩하지 않아도 정확도 유지
- **어텐션 헤드 간 중요 토큰 인덱스의 높은 유사성** → 효율적 식별 가능

### 3. I/O 효율적 중요 KV 식별 알고리즘

**기존 방법의 문제:**
- 모든 헤드의 모든 KV에 대해 중요도 점수 계산 → 계산 비용 높음
- 전체 KV를 읽어야 중요도를 알 수 있음 → I/O 절감 목적 모순

**IMPRESS의 해결:**
1. 소수 헤드의 KV만 로딩하여 중요 토큰 인덱스 추출 (대표 샘플링)
2. 헤드 간 유사성을 활용하여 전체 헤드의 중요 인덱스 추론
3. 추론된 인덱스에 해당하는 KV만 선택적으로 NVMe에서 로딩
4. **로딩량: 전체 KV Cache의 일부만** → I/O 시간 대폭 절감

### 4. 다계층 스토리지 관리

```
┌──────────┐    ┌──────────┐    ┌──────────┐
│ GPU HBM  │    │CPU DRAM  │    │NVMe SSD  │
│ (활성)   │    │(캐시)    │    │(영구)    │
└────┬─────┘    └────┬─────┘    └────┬─────┘
     │               │               │
     │  중요 KV만     │  중요도 기반  │  전체 prefix
     │  로딩          │  캐싱         │  KV 저장
     │               │               │
     └───────────────┴───────────────┘
         중요도 기반 promotion/demotion
```

**캐싱 정책:**
- 높은 중요도 + 높은 재사용 빈도 → DRAM 우선 배치
- 낮은 중요도 → NVMe에 유지 (로딩 대상에서 제외 가능)
- 동적 조정: 워크로드 패턴 변화에 적응

### 5. 실험 결과

**핵심 성능:**
- TTFT: 기존 대비 **최대 2.8× 절감**
- 추론 정확도: 기존과 **동등 수준** 유지
- FAST 2025에서 Mooncake과 함께 발표

### 6. 한계점 및 향후 연구
- 중요도 예측 정확도에 성능 의존
- 모델 아키텍처(헤드 수, attention 패턴)에 따른 일반화 검증 필요
- 동적 워크로드에서 중요도 패턴 변화에 대한 적응 속도

## 다른 논문과의 관계

**Mooncake [S1] vs CachedAttention [S2] vs IMPRESS [S5] 비교:**

| 특성 | Mooncake | CachedAttention | IMPRESS |
|------|---------|----------------|---------|
| **핵심 아이디어** | KV Cache로 연산 대체 | 멀티턴 KV 재사용 | 중요 KV만 선택적 로딩 |
| **캐싱 단위** | 블록 (512 토큰) | 세션 전체 | 토큰 단위 (중요도) |
| **스토리지 계층** | GPU/DRAM/SSD | GPU/DRAM/SSD | GPU/DRAM/SSD |
| **I/O 절감 방법** | prefix hit → 로딩 | 히스토리 hit → 로딩 | **일부만 로딩** |
| **주요 시나리오** | 공통 시스템 프롬프트 | 멀티턴 대화 | SSD에서의 prefix 로딩 |
| **규모** | 프로덕션 (수천 노드) | 연구 프로토타입 | 연구 프로토타입 |

→ **3편이 상호 보완적**: Mooncake이 캐시 아키텍처, CachedAttention이 멀티턴 메커니즘, IMPRESS가 SSD 계층 I/O 최적화

## 발표 자료 활용 포인트
- "전체 KV 대신 중요한 것만 로딩" — 직관적이고 임팩트 있는 메시지
- Attention 희소성 + 헤드 간 유사성 → 효율적 I/O의 근거
- FAST 2025에서 Mooncake + IMPRESS 동시 발표 → KV Cache 스토리지가 정식 연구 분야

## 메모
- "전부 읽지 말고 필요한 것만 읽는다" — 전통적 스토리지의 selective I/O, prefetching과 동일한 원리
- Attention 희소성은 KV Cache 압축(INT8, FP8)과도 연결되는 관찰
- Mooncake + CachedAttention + IMPRESS를 조합하면: 분산 캐싱 + 멀티턴 재사용 + 선택적 SSD 로딩
