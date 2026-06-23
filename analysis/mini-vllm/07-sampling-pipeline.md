# 07 샘플링 파이프라인 — 심층 분석

> 관련 문서: [[01-architecture-overview]] [[08-model-architectures]]
> 코드 기준: `mini-vllm-study/minivllm/sampling/`

---

## 1. 샘플링 파이프라인 전체 흐름

```
model.forward() → logits [num_seqs, vocab_size]
  │
  ├─ (1) Penalties 블록 (prev_tokens 있을 때만)
  │       ├─ repetition penalty   — 이전 토큰 logit에 배율 적용
  │       ├─ frequency penalty    — 등장 횟수에 비례 차감
  │       └─ presence penalty     — 등장 여부 이진 차감
  │
  ├─ (2) Avoid Top-K              — 상위 K개 토큰 제거 (워터마킹/제약)
  │
  ├─ (3) Temperature Scaling      — logits / temperature
  │
  ├─ (4) Typical Filtering        — 정보량이 엔트로피에 가까운 토큰만 유지
  │
  ├─ (5) Top-K Filtering          — 상위 K개 외 -inf 마스킹
  │
  ├─ (6) Top-P (Nucleus) Filtering — 누적 확률 p 이내만 유지
  │
  ├─ (7) Min-P Filtering          — max_prob × min_p 미만 제거
  │
  └─ (8) 최종 샘플링
          ├─ temperature == 0  →  argmax (Greedy, 결정론적)
          └─ temperature > 0   →  softmax → torch.multinomial (확률적)
```

**Greedy 경로 (temperature=0)**: step (3)에서 temperature 적용 후 step (8)에서 argmax. softmax/multinomial 생략.

**배치 처리**: 각 시퀀스가 다른 temperature/top_k/top_p를 가질 수 있다. `InferenceExecutor`가 각 `Sequence.sampling_params`에서 배치 텐서를 구성해 `Sampler.forward()`에 전달한다.

**코드 위치**:
- `mini-vllm-study/minivllm/sampling/sampler.py:110-273` — `Sampler.forward()`: 8단계 파이프라인 조율자
- `mini-vllm-study/minivllm/sampling/functional.py` — 개별 순수 함수 구현

---

## 2. Temperature Scaling

**공식**:
```
scaled_logits = logits / T
```

- `T`: temperature 파라미터 (float 스칼라 또는 `[num_seqs]` 텐서)
- 배치 내 시퀀스별 다른 T: `temp.unsqueeze(1)` → `[num_seqs, 1]`로 브로드캐스팅

**Temperature 효과 대비**:

| T 값 | 분포 형태 | 동작 |
|------|---------|------|
| T → 0 | 최고값 집중 | greedy에 수렴 (Sampler에서 argmax로 대체) |
| T = 1 | 원본 모델 분포 | 변환 없음 (단락 반환) |
| T > 1 | 평탄화 | 다양성 증가, 품질 감소 |

**수치 안정성**: `MIN_TEMPERATURE = 1e-8`로 `clamp_min` 처리 (0 나누기 방지).

**코드 위치**: `mini-vllm-study/minivllm/sampling/functional.py:133-196`

```python
temp = temp.clamp_min(MIN_TEMPERATURE)   # 0 나누기 방지
if temp.dim() == 1:
    temp = temp.unsqueeze(1)             # [num_seqs] → [num_seqs, 1] (브로드캐스팅)
return logits / temp
```

---

## 3. Top-K 필터링

**알고리즘**: 상위 K개 토큰 외 나머지를 `-inf`로 마스킹.

```
logits [num_seqs, vocab_size]
  → torch.topk(logits, max_k)         # max_k = 배치 내 최대 k (단일 GPU 커널 호출)
  → k_range < top_k[i] 마스크 생성    # [num_seqs, max_k] bool 텐서
  → scatter_로 vocab 공간에 역매핑    # [num_seqs, vocab_size] bool 마스크
  → masked_fill_(-inf)                # 상위 k 밖의 토큰 제거
```

**배치 최적화 핵심**: 시퀀스마다 `topk` 호출하지 않고 `max_k` 기준 **단일 `torch.topk` 호출**로 벡터화. 이후 `k_range < top_k.unsqueeze(1)` 브로드캐스팅으로 시퀀스별 유효 k 위치 마스크 생성.

```python
max_k = top_k[active_mask].max().item()        # 활성 시퀀스 중 최대 k
_, top_k_indices = torch.topk(logits, max_k)   # [num_seqs, max_k] — GPU 커널 1회
k_range = torch.arange(max_k).unsqueeze(0)     # [1, max_k]
keep_top_k_mask = k_range < top_k.unsqueeze(1) # [num_seqs, max_k] bool
mask = torch.zeros_like(logits, dtype=torch.bool)
mask.scatter_(1, top_k_indices, keep_top_k_mask)  # vocab 공간으로 역매핑
logits.masked_fill_(~mask, filter_value)
```

**비활성 조건**: `top_k <= 0` 또는 `top_k >= vocab_size`이면 원본 반환.

**코드 위치**: `mini-vllm-study/minivllm/sampling/functional.py:199-302`

---

## 4. Top-P (Nucleus) 필터링

**아이디어**: 누적 확률이 p를 처음 초과하는 지점까지의 토큰만 유지. "핵심(nucleus)" 토큰 집합.

**공식 (Holtzman et al., 2020)**:
```
sorted_probs = sort(softmax(logits), descending=True)
cumsum = cumsum(sorted_probs)
remove_mask = cumsum > p
```

**핵심 트릭 — 오른쪽 시프트**:
```python
sorted_indices_to_remove[..., 1:] = sorted_indices_to_remove[..., :-1].clone()
sorted_indices_to_remove[..., 0] = 0   # 첫 번째(가장 높은 확률) 토큰은 항상 유지
```

- cumsum이 p를 **막 초과한 토큰 자체**는 유지해야 한다.
  - "누적합이 p를 넘게 만든 마지막 토큰"을 포함해야 p 이상의 확률 커버리지가 보장됨
  - 시프트하면 "p 초과 직후 토큰부터" 제거 → 올바른 nucleus 선택

```
before: [False, False, True,  True, ...]  (3번째 토큰에서 cumsum이 p 초과)
after:  [False, False, False, True, ...]  (3번째 토큰은 유지, 4번째부터 제거)
```

**코드 위치**: `mini-vllm-study/minivllm/sampling/functional.py:305-405`

`@compiled` 적용 이유: softmax, sort, cumsum, scatter 등 GPU 친화 연산의 연속 → `torch.compile` 커널 퓨전 효과가 뚜렷함.

---

## 5. Min-P 필터링

**동적 임계값 방식**:
```
threshold[i] = min_p × max_prob[i]
```
- `max_prob[i]`: 시퀀스 i의 가장 높은 확률 토큰의 확률 값
- `threshold` 미만 토큰 → `-inf` 마스킹

**Top-P와의 차이**:

| 방식 | 임계값 기준 | 특징 |
|------|---------|------|
| Top-P | 절대적 누적 확률 p | 분포 형태와 무관한 고정 커버리지 |
| Min-P | 최대 확률 대비 상대 비율 min_p | 분포가 집중적일 때 더 엄격, 넓을 때 더 느슨 |

**예시**: min_p=0.05, 최고 확률 토큰의 확률=0.4이면 threshold=0.02. 확률 2% 미만 토큰 제거.

```python
top_prob, _ = torch.max(probs, dim=-1, keepdim=True)   # [num_seqs, 1]
scaled_min_p = min_p.unsqueeze(1) * top_prob            # [num_seqs, 1]
tokens_to_remove = probs < scaled_min_p                 # [num_seqs, vocab_size]
logits.masked_fill_(tokens_to_remove, filter_value)
```

**코드 위치**: `mini-vllm-study/minivllm/sampling/functional.py:408-479`

---

## 6. `torch.multinomial` 샘플링

**과정**:
```
필터링된 logits
  → softmax(logits)         # 확률 분포로 변환 (-inf → ≈ 0)
  → NaN 처리 (nan_to_num)   # 전부 -inf였으면 NaN → 0
  → 확률 합 0 방어           # 균등 분포 폴백
  → 재정규화 (probs / sum)
  → torch.multinomial(probs, 1, replacement=True)   # 1개 무작위 추출
  → [num_seqs, 1] → squeeze(1) → [num_seqs]
```

**방어 처리 상세**:
- `torch.isnan(probs).any()`: NaN 있으면 `nan_to_num`으로 0 대체
- `zero_sum_mask`: 확률 합 = 0인 시퀀스 → 균등 분포(`1/vocab_size`)로 폴백
  - 발생 상황: top_k/top_p/min_p가 모든 토큰을 제거한 극단적 경우

**재현성**: `generator` 인자로 `torch.Generator`를 전달하면 동일 시드에서 동일 결과.

**동일 프롬프트 반복 시 다른 결과**: multinomial 샘플링은 확률에 비례한 무작위 추출이므로 창의적/다양한 생성 가능.

**코드 위치**: `mini-vllm-study/minivllm/sampling/functional.py:865-933`

---

## 7. Typical Sampling (Typical Filtering)

**원리 (Meister et al., 2023)**: "너무 당연한 토큰(매우 높은 확률)"과 "너무 엉뚱한 토큰(매우 낮은 확률)"을 모두 배제하고, 정보량이 평균(엔트로피)에 가까운 "전형적인" 토큰만 선택.

**수식**:
```
H = -Σ p(x) * log p(x)           # 분포의 엔트로피
조건: | -log p(x) - H | ≤ tau * H
    ≡ | log p(x) + H | ≤ tau * H  (shifted_info의 절댓값 ≤ tau * H)
```

- `tau` (= `typical_p`): 허용 편차 비율. 작을수록 엔트로피에 가까운 토큰만 통과.
- `tau >= 1.0`이면 비활성 (모든 토큰 통과).

**최소 1개 보장**: 극단적 tau에서 모든 토큰이 제거될 수 있으므로 `|shifted_info|`가 가장 작은 토큰(가장 전형적인 토큰) 1개를 강제로 유지.

**파이프라인 위치**: temperature 이후, top_k 이전 (단계 4).

**코드 위치**: `mini-vllm-study/minivllm/sampling/functional.py:482-571`

---

## 8. 페널티 3종 비교

| 페널티 | 공식 | 기반 | 비활성 조건 |
|--------|------|------|------------|
| `repetition_penalty` | `logit > 0 → /penalty`, `logit < 0 → *penalty` | 곱셈 (비선형) | `penalty == 1.0` |
| `frequency_penalty` | `logit -= count(token) * alpha` | 등장 횟수 선형 비례 | `alpha == 0.0` |
| `presence_penalty` | `logit -= penalty` (1회 이상 등장 시) | 이진 유무 | `penalty == 0.0` |

**repetition vs frequency 차이**:
- repetition: 비율 기반 (원래 확률 순위 보존)
- frequency: 누산 기반 (많이 등장할수록 강하게 억제)

**Avoid Top-K** (`apply_top_token_restriction`): Top-K와 반대. 상위 K개를 **제거**. 워터마킹이나 탐색 다양성 강제에 사용.

**코드 위치**:
- `mini-vllm-study/minivllm/sampling/functional.py:702-749` — `apply_repetition_penalty`
- `mini-vllm-study/minivllm/sampling/functional.py:752-817` — `apply_frequency_penalty`
- `mini-vllm-study/minivllm/sampling/functional.py:820-862` — `apply_presence_penalty`
- `mini-vllm-study/minivllm/sampling/functional.py:574-612` — `apply_top_token_restriction`

---

## 9. Mirostat 샘플링 (현재 미연결)

`mini-vllm-study/minivllm/sampling/mirostat.py`에 구현되어 있으나 `Sampler`와 연결되지 않음 (독립 실험용).

**공통 원리 (Basu et al., 2020)**:
```
surprise(i) = -log p(token_i)      # i번째 선택 토큰의 정보량(놀라움)
target_surprise = log(τ)            # 목표 놀라움 (τ = target_perplexity)
error = surprise - target_surprise  # 실제 vs 목표 차이
```
→ `error`에 비례해 temperature 또는 mu를 갱신.

### V1 (`MirostatSampler`)

**동적 임계값**:
```
cutoff = 1 - 1 / (temperature × 10)
```
temperature가 높을수록 cutoff ↑ → 더 많은 토큰 허용.

**갱신 수식**:
```
temperature += learning_rate × error
temperature = clamp(temperature, min=0.1, max=max_temperature)
```
- `error > 0` (너무 놀라운 토큰): temperature ↑ → 더 넓은 분포
- `error < 0` (너무 당연한 토큰): temperature ↓ → 더 집중된 분포

**코드 위치**: `mini-vllm-study/minivllm/sampling/mirostat.py:159-267`

### V2 (`MirostatV2Sampler`)

**mu 도입**: temperature를 직접 갱신하지 않고 `mu`(추정 복잡도)를 갱신 후 temperature를 파생.

**동적 임계값**:
```
cutoff = 1 - 1 / mu               # V1의 temperature*10 대신 mu 사용
```

**갱신 수식**:
```python
mu -= learning_rate × error        # mu 직접 갱신
mu = clamp(mu, min=1.0, max=100.0)
temperature = tau / mu             # temperature는 mu로부터 파생
```
- mu ↑ → temperature ↓ (집중). mu ↓ → temperature ↑ (다양성).
- `learning_rate=0.1` (V1=1.0보다 10배 느림) → 더 안정적 수렴.
- `mu = 1` → `temperature = tau` (집중). `mu → ∞` → `temperature → 0` (greedy).

**V2가 V1보다 안정적인 이유**: temperature 변화가 mu를 통해 간접적으로 완충됨. mu가 실제 복잡도를 더 안정적으로 추적.

**코드 위치**: `mini-vllm-study/minivllm/sampling/mirostat.py:370-491`

**시퀀스 간 상태 초기화**: `reset()` 호출 필수. V1은 `temperature=1.0`, V2는 `temperature=1.0, mu=target_perplexity`.

---

## 10. 배치 처리 — 시퀀스별 다른 파라미터

```
InferenceExecutor.execute_batch()
  temperatures = torch.tensor([seq.sampling_params.temperature for seq in seqs])
  top_ks       = torch.tensor([seq.sampling_params.top_k       for seq in seqs])
  top_ps       = torch.tensor([seq.sampling_params.top_p       for seq in seqs])
  min_ps       = torch.tensor([seq.sampling_params.min_p       for seq in seqs])
  →  Sampler.forward(logits, temperatures=..., top_ks=..., top_ps=..., min_ps=...)
```

각 함수(apply_temperature, apply_top_k 등)는 배치 텐서를 받아 **벡터 연산**으로 시퀀스별 독립 적용. 루프 없음.

---

## 11. SamplingParams vs SamplingConfig 이원화

| 항목 | `SamplingParams` | `SamplingConfig` |
|------|-----------------|-----------------|
| 위치 | `minivllm/sampling_params.py` | `minivllm/sampling/config.py` |
| 역할 | 사용자 공개 API | Sampler 내부 기본값 |
| 노출 파라미터 | temperature, top_p, top_k, min_p, max_tokens, ignore_eos | temperature, top_k, top_p, min_p, typical_p, repetition_penalty, frequency_penalty, presence_penalty, avoid_top_k, seed |
| 추가 파라미터 | max_tokens, ignore_eos | typical_p, repetition/frequency/presence penalty, avoid_top_k, seed |
| 이원화 이유 | 공개 API 안정성 유지 | 내부 구현 독립 변경 가능 |

**실제 배치 처리에서 우선순위**: `Sampler.forward(temperatures=...)` 인자로 전달된 배치 텐서 > `SamplingConfig` 스칼라 폴백.

**코드 위치**:
- `mini-vllm-study/minivllm/sampling_params.py` — SamplingParams (66-197행)
- `mini-vllm-study/minivllm/sampling/config.py` — SamplingConfig (56-199행)

---

## 12. `torch.compile` 적용 (`@compiled` 데코레이터)

```python
def compiled(fn):
    if compile_ops:
        return torch.compile(fn)
    return fn
```

**적용 대상**:
- `apply_top_p`: softmax, sort, cumsum, scatter — GPU 친화 연산 연속
- `apply_presence_penalty`: 정적 연산 위주

**비적용 대상**:
- `apply_top_k`, `apply_temperature`: Python 동적 제어흐름 포함 (조건 분기, `isinstance` 체크)

**비활성화 조건** (`compile_ops = False`):
- NPU(`torch.npu.is_available()`) 환경: NPU 백엔드가 dynamo 트레이싱 미지원
- CUDA 없는 CPU-only 환경: compile 이득이 적음
- `torch._dynamo` import 실패 (구버전 torch)

**코드 위치**: `mini-vllm-study/minivllm/sampling/functional.py:94-130`
