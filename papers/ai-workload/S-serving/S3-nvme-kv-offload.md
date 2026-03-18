# [S3] An I/O Characterizing Study of Offloading LLM Models and KV Caches to NVMe SSD ★필독

- **학회/연도:** CHEOPS 2025 (EuroSys 워크숍)
- **저자:** (VU Amsterdam, @Large Research)
- **분류:** LLM 추론/서빙 스토리지

## 핵심 요약 (1~2문장)
LLM 모델 파라미터와 KV Cache를 NVMe SSD로 오프로드할 때 발생하는 I/O 패턴을 실측 분석한 논문. GPU-Storage 연구([I1] BaM, [G1] GDS)와 LLM 추론의 접점을 직접적으로 보여주는 핵심 연구.

## 읽기 전 질문
- LLM 모델과 KV Cache를 NVMe로 오프로드할 때 I/O 크기/패턴은 어떻게 되는가?
- NVMe SSD의 어떤 성능 특성(순차/랜덤, 읽기/쓰기)이 LLM 오프로드에서 중요한가?
- GPU-initiated I/O(BaM)가 이 시나리오에서 기존 CPU-mediated I/O 대비 얼마나 유리한가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- LLM 크기 증가(70B, 175B, 405B 파라미터)로 GPU HBM 용량 초과 일상화
- KV Cache: 컨텍스트 길이 증가(128K~1M 토큰)로 수십 GB까지 팽창
- NVMe SSD로의 오프로드가 현실적 대안이지만, I/O 특성에 대한 실측 데이터 부족

### 2. I/O 특성 분석

**모델 파라미터 오프로드:**
```
특성: 대용량 순차 읽기
크기: 수십~수백 GB (모델 전체)
빈도: 모델 로딩 시 1회 (콜드스타트)
패턴: 순차 읽기 지배적
대역폭 요구: 높음 (빠른 콜드스타트를 위해)
```

**KV Cache 오프로드:**
```
특성: 중간 크기 읽기/쓰기 혼재
크기: 레이어당 수 MB ~ 수십 MB
빈도: 토큰 생성마다 (매우 빈번)
패턴: 순차적이지만 레이어별로 분산
대역폭 요구: 높음 + 저지연 요구
읽기/쓰기 비율: 혼합 (evict 시 쓰기, prefetch 시 읽기)
```

**핵심 발견:**
- KV Cache 오프로드의 I/O 크기는 전통적 "작은 랜덤 I/O"도 "대용량 순차 I/O"도 아닌 **중간 영역**
- NVMe SSD의 QD(Queue Depth)와 I/O 크기에 따른 대역폭 변화가 성능에 직접 영향
- PCIe 대역폭이 병목이 되는 경우가 많음 — Gen4 vs Gen5의 차이가 실질적

### 3. GPU-Storage 연구와의 연결

```
                    LLM 추론 시 KV Cache 흐름

GPU HBM ◄──────────────────────────────────────► NVMe SSD
   │                                                  │
   │  [활성 KV Cache]                    [오프로드된 KV Cache]
   │  - 현재 생성 중인 토큰              - Evict된 이전 레이어
   │  - 최근 어텐션 대상                 - Prefill 캐시
   │                                                  │
   │           데이터 이동 방식 선택:                   │
   │                                                  │
   ├── (1) CPU-mediated: GPU→CPU→SSD (기존)           │
   ├── (2) GDS P2P DMA: GPU↔SSD 직접 (CPU 우회)      │
   └── (3) GPU-initiated: GPU가 NVMe 큐 직접 조작     │
                    (BaM 방식, 최저 지연)
```

### 4. 한계점 및 향후 연구
- 특정 NVMe SSD 모델에서의 실측 — 일반화에 한계
- KV Cache 압축(INT8, FP8) 적용 시 I/O 특성 변화 미분석
- GPU-initiated I/O(BaM 방식)와의 직접 비교 실험은 미수행

## 다른 논문과의 관계
- 직접 연결: BaM [I1] (GPU-initiated I/O), GDS [G1] (P2P DMA) — 이 논문의 I/O를 처리하는 기술
- 워크로드 제공: [S1] Mooncake, [S2] CachedAttention — 이런 시스템들이 발생시키는 I/O의 실측
- 보완: [S4] INF² — NVMe 측에서 연산하여 I/O 자체를 줄이는 접근

## 발표 자료 활용 포인트
- **gpu-ssd 논문들과의 연결고리로 가장 중요한 논문**
- KV Cache I/O 패턴 다이어그램 → BaM/GDS의 실제 사용 사례로 활용
- "LLM 추론이 스토리지 문제가 되었다"는 메시지의 실증적 근거

## 메모
- 이 논문이 gpu-ssd 연구와 ai-workload 연구를 잇는 **브릿지 논문**
- BaM [I1]이 "GPU에서 NVMe로 직접 I/O"를 제안했고, 이 논문이 "LLM KV Cache가 그 기술이 필요한 실제 워크로드"임을 보여줌
- 향후 BaM + KV Cache 오프로드를 결합한 연구가 나올 가능성 높음
