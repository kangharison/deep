# [T2] PCcheck: Persistent Concurrent Checkpointing for ML

- **학회/연도:** ASPLOS 2025
- **저자:** Foteini Strati, Michal Friedman, Ana Klimovic (ETH Zurich)
- **분류:** 트레이닝 스토리지 최적화
- **소스코드:** https://github.com/eth-easl/pccheck

## 핵심 요약 (1~2문장)
ML 트레이닝의 체크포인트를 연산과 완전히 병렬(concurrent)로 수행하면서 SSD 및 Persistent Memory(PMEM) 모두 지원하는 체크포인트 시스템. 10 iteration마다 체크포인트를 저장하면서도 트레이닝 throughput에 **3% 미만의 오버헤드**만 발생시켜, 빈번한 체크포인트를 통한 빠른 장애 복구를 실현한다.

## 읽기 전 질문
- "Persistent concurrent"가 [T1] DataStates-LLM의 "lazy asynchronous"와 어떻게 다른가?
- 3% 미만의 오버헤드로 어떻게 빈번한 체크포인트가 가능한가?
- SSD와 PMEM 두 가지 저장 매체에서의 동작 차이는?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 대규모 ML 모델 트레이닝: 수천 가속기에서 수일~수주 실행
- 장애 확률 증가: 규모↑ → 장애 빈도↑, spot VM 사용 → 선점 빈도↑
- 표준 접근: 주기적으로 트레이닝 일시정지 → 체크포인트 저장 → 재개
- 핵심 트레이드오프: **체크포인트 빈도 ↑ = 복구 시간 ↓ but 트레이닝 오버헤드 ↑**
- PCcheck의 목표: 이 트레이드오프를 깨고, 빈번한 체크포인트 + 최소 오버헤드

### 2. 핵심 메커니즘: Concurrent Checkpointing

**[T1] DataStates-LLM과의 비교:**

| 특성 | DataStates-LLM [T1] | PCcheck [T2] |
|------|---------------------|-------------|
| **접근** | Lazy Asynchronous | Persistent Concurrent |
| **GPU→Host** | F/B pass 중 비동기 복사 | 연산과 동시에 진행 |
| **저장 매체** | Lustre PFS | SSD + PMEM |
| **오버헤드** | iteration 영향 최소화 | **<3%** |
| **빈도** | 가변 | **매 10 iteration** 가능 |
| **일관성** | 2-phase commit | 버전 관리 기반 |
| **분산** | 최대 128노드/512 GPU | 단일~분산 모두 |
| **핵심 차별점** | throughput 최대화 | 빈도 극대화 |

### 3. 지원 저장 매체

**SSD:**
- 표준 NVMe SSD에 체크포인트 저장
- 비동기 I/O로 트레이닝과 중첩
- 로컬 SSD → 네트워크 I/O 불필요

**Persistent Memory (PMEM):**
- Intel Optane PMEM 등 byte-addressable 비휘발성 메모리
- DRAM 수준의 지연시간으로 체크포인트 저장
- SSD 대비 훨씬 빠른 체크포인트 저장/로딩

### 4. 실험 결과

**핵심 수치:**
- 체크포인트 빈도: **매 10 iteration** (매우 빈번)
- 트레이닝 throughput 오버헤드: **<3%** (사실상 무시 가능)
- 단일 머신 + 분산 환경 모두에서 검증

**의미:**
- 10 iteration = 수십 초~수 분마다 체크포인트
- 장애 발생 시 최대 10 iteration만 재연산 → 복구 시간 극소화
- 기존 방식은 수백~수천 iteration마다 체크포인트 → 복구 시 수시간 재연산 가능

### 5. 한계점 및 향후 연구
- PMEM은 Intel이 Optane 제품 중단 → 범용성에 의문
- SSD 모드에서 체크포인트 크기가 매우 큰 모델의 성능은 추가 검증 필요
- CXL-attached 메모리 등 차세대 매체로의 확장 가능성

## 다른 논문과의 관계
- 발전: [T1] DataStates-LLM의 비동기 접근을 빈도 측면에서 극대화
- 동기 부여: [W2] 장애율 40%, [W3] 체크포인트 실측
- 비교: [T4] LMStor (계층적 체크포인트로 복구 시간 최소화 — 다른 접근)
- gpu-ssd 연결: 로컬 NVMe SSD에 직접 체크포인트 → GPU-initiated I/O로 더 가속 가능

## 발표 자료 활용 포인트
- "매 10 iteration, 3% 미만 오버헤드" — 체크포인트 최적화의 현재 최고 수준
- T1(throughput 최대화)과 T2(빈도 극대화)의 비교: 상호 보완적 설계 철학
- ASPLOS 2025 + ETH Zurich — 학술적 신뢰도 높음

## 메모
- T1과 T2는 같은 문제의 다른 면을 공격: T1은 "체크포인트를 더 빨리", T2는 "체크포인트를 더 자주"
- PMEM의 미래가 불확실하지만, CXL-attached memory가 유사한 역할을 할 수 있음
- 오픈소스: https://github.com/eth-easl/pccheck
