# [S1] Mooncake: Trading More Storage for Less Computation — A KVCache-Centric Architecture for Serving LLM Chatbot ★필독

- **학회/연도:** FAST 2025
- **저자:** (Moonshot AI)
- **분류:** LLM 추론/서빙 스토리지

## 핵심 요약 (1~2문장)
LLM 챗봇 서빙에서 KV Cache를 핵심 자원으로 취급하여, "더 많은 스토리지를 써서 더 적은 연산을 하자"는 철학의 KVCache 중심 서빙 아키텍처. Prefill 단계의 연산을 KV Cache 저장/재사용으로 대체하여 GPU 연산량을 줄이고 서빙 비용을 절감한다.

## 읽기 전 질문
- "KVCache-centric"이란 기존 LLM 서빙 아키텍처와 어떻게 다른가?
- KV Cache를 저장하고 재사용하는 것이 재연산보다 효율적인 조건은?
- Storage 계층 설계(DRAM, NVMe, 원격)는 어떻게 구성되는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- LLM 서빙에서 Prefill(프롬프트 처리) 단계가 GPU 연산의 상당 부분을 차지
- 멀티턴 대화에서 이전 턴의 컨텍스트를 매번 재연산하는 것은 비효율적
- 공통 시스템 프롬프트(system prompt)가 많은 요청에서 반복됨

### 2. 제안 방법 (Approach)

```
기존 서빙:
  요청 1: [시스템 프롬프트 + 사용자 입력] ──→ Prefill(전체 연산) ──→ Decode
  요청 2: [시스템 프롬프트 + 사용자 입력] ──→ Prefill(전체 연산) ──→ Decode
  (같은 시스템 프롬프트도 매번 재연산)

Mooncake (KVCache-centric):
  요청 1: [시스템 프롬프트] ──→ Prefill ──→ KV Cache 저장 ──→ Decode
  요청 2: [시스템 프롬프트] ──→ KV Cache 로딩(저장소에서) ──→ Decode만
  (공통 부분은 재사용, 연산 절약)
```

**KV Cache 스토리지 계층:**
- **Tier 1 — GPU HBM**: 현재 활성 요청의 KV Cache
- **Tier 2 — Host DRAM**: 최근 사용된 KV Cache 캐싱
- **Tier 3 — 분산 스토리지(NVMe 등)**: 재사용 가능성 있는 KV Cache 장기 저장
- 요청 도착 시 KV Cache lookup → hit이면 스토리지에서 로딩, miss면 Prefill 연산

**핵심 트레이드오프:**
- 스토리지 비용(KV Cache 저장) vs 연산 비용(Prefill 재연산)
- KV Cache 재사용률이 높을수록 스토리지 접근 방식이 유리
- 챗봇 시나리오에서 시스템 프롬프트 재사용률이 매우 높음

### 3. 실험 결과
- 실제 챗봇 서빙 환경에서 GPU 연산량 대폭 절감
- Prefill 비용이 지배적인 long-context 시나리오에서 특히 효과적
- FAST 2025에서 발표 — 스토리지 커뮤니티에서 LLM 서빙을 다룬 대표 논문

### 4. 한계점 및 향후 연구
- KV Cache 재사용률이 낮은 워크로드에서는 스토리지 비용만 증가
- KV Cache의 정확한 매칭(prefix matching)이 필요 — 해싱/인덱싱 오버헤드
- 모델 업데이트 시 저장된 KV Cache 무효화 필요

## 다른 논문과의 관계
- 보완: [S2] CachedAttention (멀티턴 특화 KV 재사용)
- 보완: [S5] IMPRESS (중요도 기반 다계층 KV 저장)
- 비교: [S3] NVMe KV 오프로드 (KV Cache의 NVMe I/O 특성 실측)
- gpu-ssd 연결: KV Cache를 NVMe에서 GPU로 로딩할 때 GDS/BaM의 P2P DMA가 이상적

## 발표 자료 활용 포인트
- "더 많은 스토리지, 더 적은 연산" — 스토리지 연구자에게 어필하는 메시지
- FAST 2025(스토리지 최고 학회)에서 LLM 서빙 논문이 채택된 것 자체가 트렌드 시사
- KV Cache 스토리지 계층 다이어그램

## 메모
- 스토리지 커뮤니티(FAST)에서 LLM 서빙을 다루기 시작한 상징적 논문
- "연산을 스토리지로 대체"하는 관점은 기존 "캐싱으로 I/O를 줄인다"의 역발상
- KV Cache 관리가 곧 스토리지 관리 문제가 됨 — 전통적인 캐시 교체/계층화 기법이 직접 적용 가능
