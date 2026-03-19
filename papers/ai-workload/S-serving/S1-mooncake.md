# [S1] Mooncake: A KVCache-centric Disaggregated Architecture for LLM Serving ★필독

- **학회/연도:** FAST 2025 (**Best Paper Award**)
- **저자:** Ruoyu Qin, Zheming Li, Weiran He, Jialei Cui (Moonshot AI), Feng Ren, Mingxing Zhang, Yongwei Wu, Weimin Zheng (Tsinghua Univ.), Xinran Xu (Moonshot AI)
- **분류:** LLM 추론/서빙 스토리지
- **소스코드:** https://github.com/kvcache-ai/Mooncake
- **프로덕션:** Kimi 서비스의 기본 서빙 플랫폼, 수천 노드, **일일 1,000억+ 토큰** 처리

## 핵심 요약 (1~2문장)
LLM 챗봇 서빙에서 KV Cache를 핵심 자원으로 취급하여 Prefill/Decoding을 분리하고, GPU/CPU DRAM/SSD의 유휴 자원을 KV Cache 분산 캐시로 활용하는 disaggregated 아키텍처. 실제 워크로드에서 vLLM 대비 long-context 시 **최대 525% 처리량 향상**, 프로덕션에서 **75% 추가 요청 처리**를 달성하며 FAST 2025 Best Paper를 수상했다.

## 읽기 전 질문
- Prefill/Decoding 분리(disaggregation)가 왜 long-context LLM에서 중요한가?
- KV Cache를 "저장하고 재사용"하는 것이 "재연산"보다 효율적인 조건은?
- 과부하(overload) 상황에서 요청을 어떻게 관리하는가?

## 주요 내용 정리

### 1. 시스템 아키텍처

```
┌──────────────────────────────────────────────────────────────────────┐
│                         Conductor (글로벌 스케줄러)                    │
│  - Prefix 매칭 (해시 기반)                                            │
│  - TTFT 예측 기반 인스턴스 선택                                        │
│  - 과부하 시 예측 기반 조기 거부                                       │
└────────────┬─────────────────────┬───────────────────────────────────┘
             │                     │
     ┌───────▼──────┐      ┌──────▼───────┐
     │ Prefill Pool │      │ Decoding Pool│
     │              │      │              │
     │ 입력 토큰    │──KV──│ 출력 토큰    │
     │ 병렬 처리    │Cache │ 자동회귀 생성│
     │ Chunked PP   │전송  │ Continuous   │
     │              │      │ Batching     │
     └──────┬───────┘      └──────┬───────┘
            │                      │
     ┌──────▼──────────────────────▼───────┐
     │         KVCache Pool (분산 캐시)     │
     │                                     │
     │  ┌──────┐  ┌──────────┐  ┌───────┐ │
     │  │ GPU  │  │CPU DRAM  │  │  SSD  │ │
     │  │ HBM  │  │(RDMA)    │  │(Cold) │ │
     │  │(Hot) │  │(Warm)    │  │       │ │
     │  └──────┘  └──────────┘  └───────┘ │
     │                                     │
     │  블록 크기: 512 토큰/블록             │
     │  중복제거: prefix 해시 기반           │
     │  교체정책: LRU/LFU/커스텀            │
     │  전송: GPUDirect RDMA (최대 800Gbps) │
     └─────────────────────────────────────┘
```

### 2. KVCache Pool 상세

**블록 관리:**
- 블록 크기: **512 토큰/블록**
- 각 블록에 해시 값 부여 (자신의 해시 + prefix 해시로 중복 제거)
- 교체 정책: LRU가 Kimi 워크로드의 시간적 지역성에 가장 적합

**캐시 Hit Rate (Table 1, 23,608 요청 트레이스):**

| 캐시 용량 (블록 수) | Hit Rate |
|:------------------:|:--------:|
| 1,000 | 30% |
| 10,000 | 40% |
| 50,000 | **50%** |
| 그 이상 | 미미한 개선 |

- 50%의 블록이 한 번도 사용되지 않음
- Hot 블록은 수만 번 접근 → 극단적 skew 분포

**전송 메커니즘: Messenger Service**
- GPUDirect RDMA 기반 노드 간 KVCache 전송
- 최대 **800 Gbps** 대역폭
- 토폴로지 인식 경로 선택: NIC별 preferred/secondary 리스트

### 3. Prefill-Decoding Disaggregation

**4단계 요청 처리 흐름:**
1. **KV Cache 재사용**: 원격 CPU 메모리에서 prefix cache를 prefill 노드 GPU로 로딩
2. **증분 Prefill**: 캐시되지 않은 새 토큰만 prefill 수행 → 새 KV Cache 생성
3. **KV Cache 전송**: Messenger가 비동기적으로 decoding 노드로 전송
4. **Decoding**: KV Cache 도착 후 continuous batching에 합류

**Chunked Pipeline Parallelism (CPP):**
- 입력 토큰을 청크로 분할 (threshold: ~1000 토큰)
- 여러 prefill 노드가 동시에 다른 청크 처리
- 파이프라인 단계 경계에서만 cross-node 통신 → 네트워크 부담 최소화

**Layer-wise Prefill:**
- KV Cache 로딩/저장을 레이어별 비동기 수행
- 연산과 데이터 전송 중첩 → long-context 요청에서 효과적

### 4. Conductor 스케줄러 (Algorithm 1)

**KV Cache 인식 스케줄링:**
1. 입력 토큰을 블록 단위로 해시
2. 각 prefill 인스턴스의 캐시된 블록과 비교
3. **TTFT = T_queue + T_prefill + T_transfer** (캐시 재사용 시 transfer 추가)
4. TTFT를 최소화하는 인스턴스 선택 (캐시 히트 vs 전송 비용 트레이드오프)

**Hot-Spot Migration:**
- 자주 접근되는 캐시 블록을 여러 노드에 자동 복제
- Heuristic 기반 복제 → 정밀 예측 불필요

### 5. 과부하 처리: 예측 기반 조기 거부

**문제:** Prefill 완료 후 Decoding 할당 실패 → 낭비된 Prefill 연산
**해결:** 요청 도착 시 Prefill/Decoding 풀의 부하를 예측하여 처리 불가능한 요청을 **미리 거부**

| 전략 | 거부된 요청 수 |
|------|:------------:|
| 기준선 (사후 거부) | 4,183 |
| 조기 거부 | 3,771 |
| **예측 기반 조기 거부** | **3,589** (13.9% 개선) |

### 6. 실험 환경

| 항목 | 사양 |
|------|------|
| **GPU** | 8× NVIDIA A800-SXM4-80GB/노드 |
| **네트워크** | RDMA, 800 Gbps |
| **클러스터** | 8~20 노드 |
| **모델** | LLaMA2-70B 아키텍처 (더미 모델, Kimi 보호 목적) |
| **기준선** | vLLM (continuous batching + PagedAttention) |

**실제 워크로드 트레이스 (23,608 요청):**
- 평균 입력: **7,590 토큰**, 평균 출력: 182 토큰
- 입력:출력 비율: **~720:1** (극단적 long-context)
- 50% 캐시 재사용 가능

### 7. 성능 결과 (상세)

#### vLLM 대비 처리량 향상

| 워크로드 | 프롬프트 길이 | 향상 |
|---------|:----------:|:----:|
| ArXiv (짧은 컨텍스트) | ~8K | 20% |
| L-Eval (prefix 재사용 높음) | ~19K | 40% |
| 시뮬레이션 16K | 16K | 50% |
| 시뮬레이션 32K | 32K | ~150% |
| 시뮬레이션 64K | 64K | ~300% |
| 시뮬레이션 128K | 128K | **525%** |

**핵심: 컨텍스트가 길수록 이점이 폭발적으로 증가** — KV Cache 재사용의 가치가 높아지고, vLLM은 coupled 설계로 대형 요청을 단일 처리해야 함

#### 프로덕션 실 워크로드 (Figure 13)
- Mooncake [10P+10D] vs vLLM [20M]
- TTFT SLO 만족률: 둘 다 ~100%
- TBT SLO 만족률: Mooncake **100%** vs vLLM **57%**
- **75% 더 많은 요청 처리 가능** (SLO 준수하면서)

#### SLO 설정
- TTFT: P90 ≤ 30초
- TBT: P90 ≤ 0.1초/토큰

### 8. 한계점 및 향후 연구
- 수동 threshold 조정 필요 (캐시-밸런싱 결정 기준)
- 정적 Prefill/Decoding 인스턴스 비율
- 요청 레벨 출력 길이 예측 미구현 (비용 높고 정확도 낮아 보류)
- 향후: 이기종 가속기(GDDR/LPDDR), Process-in-Memory, 연산자 레벨 disaggregation

## 다른 논문과의 관계
- 보완: [S2] CachedAttention (멀티턴 KV 재사용의 선행 연구)
- 보완: [S5] IMPRESS (중요도 기반 다계층 KV 관리)
- I/O 실측: [S3] NVMe KV 오프로드 (Mooncake이 발생시키는 I/O의 특성)
- gpu-ssd 연결: KV Cache를 NVMe→GPU로 로딩 시 BaM/GDS의 P2P DMA가 이상적 기술
- vLLM 통합: Mooncake+vLLM 통합 로드맵 공개 (P/D disaggregation + 글로벌 KVCache)

## 발표 자료 활용 포인트
- **FAST 2025 Best Paper** — 스토리지 최고 학회 최고 논문
- "더 많은 스토리지, 더 적은 연산" — 스토리지 연구자에게 강력한 메시지
- 128K 컨텍스트에서 525% 향상 그래프
- "일일 1,000억 토큰" 프로덕션 배포 규모

## 메모
- FAST(스토리지 학회)에서 LLM 서빙이 Best Paper — 패러다임 전환의 신호
- KV Cache 관리 = 전통적 스토리지 계층화/캐싱 문제의 재림 → 스토리지 연구자의 새로운 영역
- Mooncake 오픈소스 + 트레이스 공개: 후속 연구의 기반 플랫폼
- 입력:출력 = 720:1 → Prefill이 지배적 → KV Cache 재사용의 가치가 압도적
