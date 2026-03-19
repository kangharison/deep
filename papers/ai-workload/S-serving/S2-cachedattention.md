# [S2] CachedAttention: Cost-Efficient Large Language Model Serving for Multi-turn Conversations

- **학회/연도:** ATC 2024
- **저자:** Bin Gao, Puru Sharma, Qingxuan Kang, Djordje Jevdjic (NUS), Zhuomin He (SJTU), Junbo Deng, Xingkun Yang, Zhou Yu, Pengfei Zuo (Huawei Cloud)
- **분류:** LLM 추론/서빙 스토리지

## 핵심 요약 (1~2문장)
멀티턴 대화에서 이전 턴의 KV Cache를 GPU HBM → Host DRAM → SSD의 3계층 스토리지에 저장하고 재사용하여 LLM 서빙 비용을 줄이는 시스템. Positional encoding 디커플링으로 컨텍스트 오버플로 시에도 캐시를 유효하게 유지하며, TTFT를 **최대 87% 절감**, prefilling throughput을 **최대 7.8× 향상**, 추론 비용을 **최대 70% 절감**한다.

## 읽기 전 질문
- 멀티턴 대화에서 KV Cache 재연산의 실제 비용은 얼마나 되는가?
- 3계층 스토리지에서 KV Cache를 GPU 연산과 어떻게 중첩하여 로딩하는가?
- 컨텍스트 윈도우 오버플로 시 저장된 KV Cache가 왜 무효화되고, 어떻게 해결하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)

**ShareGPT 데이터셋 분석:**
- **73%의 대화가 멀티턴**
- 새 턴의 prefilling 비용 중 **최대 99%가 히스토리 토큰의 KV Cache 재연산**
- LLaMA-65B 기준: 2K 토큰 prefilling ~360ms, 5GB KV Cache 로딩 ~192ms

**HBM 한계:**
- 80GB GPU HBM이 KV Cache로 **14초 만에** 가득 참 (LLaMA-65B)
- 현재 시스템(vLLM 등): 세션 비활성화 시 KV Cache 폐기 → 재활성화 시 전체 재연산

### 2. AttentionStore: 3계층 KV 캐싱 시스템

| 계층 | 매체 | 용량 | 대역폭 | 역할 |
|------|------|------|--------|------|
| **L1** | GPU HBM | 80GB/GPU | ~900 GB/s | 활성 추론 |
| **L2** | Host DRAM | 128GB | ~26 GB/s (PCIe Gen4) | 빠른 접근 버퍼 |
| **L3** | SSD | **10TB** | ~5 GB/s | 영구 저장 |

**핵심:** 대부분의 KV Cache는 SSD에 저장 — DRAM+SSD를 활용해 용량을 100× 확장

### 3. Layer-wise Pre-loading (레이어별 사전 로딩)

```
기존: KV Cache 전체 로딩 완료 후 연산 시작
     [===== KV 로딩 =====][===== 연산 =====]

CachedAttention: 레이어별 중첩
     Layer 1 KV 로딩 → Layer 1 연산
          Layer 2 KV 로딩 → Layer 2 연산
               Layer 3 KV 로딩 → Layer 3 연산
                    ...
     (로딩과 연산이 파이프라인으로 중첩)
```

- Read Stream: 다음 레이어 KV Cache를 현재 레이어 연산 중 미리 로딩
- 15-layer 버퍼: prefilling 시간 **최대 61% 절감**
- 비동기 저장: prefilling/decoding 중 KV Cache를 레이어별 비동기 쓰기 → 13~15% 추가 개선

### 4. 스케줄러 인식 Fetching/Eviction

**Prefetching (Disk → DRAM):**
- Look-ahead window: `L_pw = C_mem / S_kv`
- 스케줄러 큐를 모니터링하여 다음 실행될 작업의 KV Cache를 디스크에서 미리 로딩

**Eviction (DRAM → Disk):**
- 스케줄러 큐 내 작업의 KV Cache는 eviction 면제
- 큐 뒤쪽(실행 먼 순서)의 KV Cache부터 eviction

**성능 비교 (128GB DRAM + 10TB SSD):**

| 정책 | 전체 Hit Rate | DRAM Hit Rate | 속도 향상 |
|------|:----------:|:----------:|:--------:|
| LRU | 58% | 0.6% | 기준선 |
| FIFO | 48% | 0.5% | — |
| **CachedAttention** | **86%** | **99.6%** | **2.7×** |

**핵심:** 스케줄러 정보를 활용하면 99.6%의 hit가 DRAM에서 발생 → SSD 접근 거의 불필요

### 5. Positional Encoding 디커플링

**문제:** 컨텍스트 윈도우 오버플로(예: 4K 윈도우에 8K 토큰) 시 truncation 필요
- 기존: 절대적 위치 인코딩이 KV에 내재 → truncation으로 위치가 바뀌면 KV 무효화
- 디커플링 없이 직접 truncation(NKVT): PPL >1600, 정확도 29.6% (**실패**)

**해결:** Relative Positional Encoding(RPE) 기반 모델에서:
1. KV Cache 저장 시 위치 인코딩 **적용 전** 상태로 저장
2. 로딩 시 truncation 후의 **새 위치 인코딩** 적용
3. 캐시 유효성 유지 + truncation 자유도 확보

**정확도 검증:**
- Perplexity: 차이 <0.02 (WikiText-2, C4, PTB)
- MMLU: 52.3% (기준선과 동일)
- LongEval: 68.0% (기준선과 동일)
- **지원 모델:** LLaMA, Falcon, Mistral 등 RPE 기반 모든 모델

### 6. 실험 결과 (상세)

**환경:** 4× A100 80GB, 128GB DRAM, 10TB SSD, ShareGPT 9K 세션, Poisson λ=1.0

#### TTFT 절감

| 모델 | TTFT 절감 |
|------|:--------:|
| LLaMA-13B | 85% |
| LLaMA-65B | 61% |
| LLaMA-70B | **87%** |
| Falcon-40B | 86% |

#### Prefilling Throughput 향상

| 모델 | 향상 |
|------|:----:|
| LLaMA-13B | 6.8× |
| LLaMA-65B | 2.6× |
| LLaMA-70B | **7.8×** |
| Falcon-40B | 7.2× |

#### End-to-End 추론 비용 절감

| 모델 | GPU 시간 가속 | 비용 절감 |
|------|:----------:|:--------:|
| LLaMA-13B | 4.0× | **70%** |
| LLaMA-65B | 1.9× | 43% |
| LLaMA-70B | 3.3× | 66% |
| Falcon-40B | 3.4× | 68% |

(비용: A100 $5/hr, DRAM $0.0088/hr/GB, SSD $0.000082/hr/GB)

#### 세션 도착률 민감도
- 0.5 sessions/sec: 82% hit rate, 858K tok/s
- 2.0 sessions/sec: 77% hit rate, 681K tok/s (부하 2배에도 안정적)

### 7. 한계점 및 향후 연구
- APE(절대적 위치 인코딩) 모델 비지원 (GPT-2 등)
- KV Cache 크기가 모델 크기에 비례 → 65B에서 효과 상대적 저하 (SSD 슬롯 부족)
- RPE 디커플링의 오버헤드는 작지만 zero는 아님

## 다른 논문과의 관계
- 선행: Mooncake [S1]에 영향 (KV Cache 재사용 개념의 체계화)
- I/O 실측: [S3] NVMe KV 오프로드 (SSD 계층의 실제 I/O 특성)
- 보완: [S5] IMPRESS (중요도 기반으로 어떤 KV를 어느 계층에 놓을지)
- gpu-ssd 연결: SSD→GPU KV Cache 로딩에 GDS P2P DMA 적용 → PCIe 오버헤드 절감

## 발표 자료 활용 포인트
- "멀티턴 prefilling의 99%가 히스토리 재연산" — 문제 규모 직관적
- 3계층 캐싱 + 스케줄러 인식 정책의 효과: LRU 58% vs CA 86% (DRAM 99.6%)
- RPE 디커플링의 NKVT vs CA 정확도 비교: PPL 1600 vs <5.5 — 드라마틱한 차이
- 비용 절감 70% (GPU 비용 기준)

## 메모
- "스케줄러 정보가 캐시 정책의 핵심"이라는 통찰 → 전통적 캐시에서도 적용 가능한 원리
- Positional encoding 디커플링은 KV Cache 영구 저장의 선결 조건 → 이 기법 없이는 캐시 무효화 빈발
- Mooncake이 시스템 레벨(disaggregation), CachedAttention이 메커니즘 레벨(계층 캐싱+RPE) → 상호 보완적
