# 04. InferenceExecutor + CUDA Graph + Prefill/Decode 실행 경로 심층 분석

관련 문서: [[01-overview]] [[02-block-manager-prefix-cache]] [[03-scheduler-preemption]]

---

## 1. InferenceExecutor 역할

`mini-vllm-study/minivllm/engine/inference_executor.py`

InferenceExecutor는 mini-vLLM 추론 파이프라인에서 실제 GPU/NPU 연산을 책임지는 핵심 실행 엔진이다. 세 가지 주요 역할을 수행한다.

### (1) KV 캐시 할당 및 레이어 배분

GPU VRAM에 단일 연속 텐서를 할당하고, 각 Attention 레이어의 `k_cache`/`v_cache` 속성에 레이어별 슬라이스(뷰)를 주입한다. Attention 레이어가 이 뷰에 직접 쓰므로 복사 없이 KV가 누적된다.

### (2) Prefill/Decode 입력 준비 및 컨텍스트 전파

FlashAttention 커널이 필요로 하는 보조 텐서(`slot_mapping`, `block_tables`, `cu_seqlens`, `context_lens`)를 매 스텝 구성하고, `contextvars` 기반 Context에 담아 Attention 레이어에 전달한다. Attention 레이어는 forward 중에 `get_context()`로 이 정보를 읽는다.

### (3) CUDA/NPU Graph 캡처·리플레이

Decode 단계 GPU 연산 시퀀스를 정적 그래프로 한 번 캡처해두고, 매 스텝 `replay()`로 재실행해 CPU 커널 제출 오버헤드를 제거한다.

### 초기화 체인

```
ModelRunner.__init__()
  → InferenceExecutor.__init__()
       → _init_attributes()   : 그래프 컨테이너 + Decode 사전 할당 버퍼 생성
  → InferenceExecutor.initialize()
       → _allocate_kv_cache() : KV 캐시 텐서 할당
       → _initialize_sampler(): Sampler 초기화
       → _optimize_model()    : eval 모드 + gradient 비활성화 + NPU 최적화
       → _warmup_model()      : 더미 데이터로 CUDA 커널 예열
  → InferenceExecutor.capture_device_graphs()
       → _capture_graph_for_batch_size() × N  : 배치 크기별 Graph 캡처
```

---

## 2. KV 캐시 텐서 할당 (`initialize()` → `_allocate_kv_cache()`)

`mini-vllm-study/minivllm/engine/inference_executor.py:429`

### KV 캐시 텐서 형상

```python
kv_cache = torch.empty(
    (2, num_layers, num_blocks, block_size, num_kv_heads, head_dim),
    device=device,
    dtype=dtype,
)
```

| 차원 | 의미 | 예시 값 |
|------|------|---------|
| `2` | 0=K 캐시, 1=V 캐시 | 항상 2 |
| `num_layers` | 트랜스포머 레이어 수 | 28 (Qwen2-7B) |
| `num_blocks` | 전체 블록 풀 크기 | 자동 계산 또는 설정 |
| `block_size` | 블록당 토큰 수 | 64 (기본값) |
| `num_kv_heads` | KV 헤드 수 (GQA 시 Q 헤드보다 작음) | 8 (Qwen2-7B GQA) |
| `head_dim` | 헤드 차원 = hidden_size / num_attention_heads | 128 |

**블록당 바이트 계산:**
```
bytes_per_block = 2 × num_layers × block_size × num_kv_heads × head_dim × dtype.itemsize
```

예: bfloat16, 28 레이어, block_size=64, 8 KV heads, head_dim=128:
```
= 2 × 28 × 64 × 8 × 128 × 2 bytes = 7,340,032 bytes ≈ 7 MB/block
```

### `_assign_kv_cache_to_layers()` — 레이어별 슬라이스 주입

`mini-vllm-study/minivllm/engine/inference_executor.py:599`

```python
layer_id = 0
for module in self.model.modules():
    if hasattr(module, "k_cache") and hasattr(module, "v_cache"):
        module.k_cache = self.kv_cache[0, layer_id]   # K 슬라이스
        module.v_cache = self.kv_cache[1, layer_id]   # V 슬라이스
        layer_id += 1
```

- `k_cache` 슬라이스 shape: `(num_blocks, block_size, num_kv_heads, head_dim)`
- `v_cache` 슬라이스 shape: 동일
- 슬라이스는 기존 텐서의 **뷰(view)**이므로 복사 없이 원본 메모리를 공유한다.
- Attention 레이어가 `k_cache`에 쓰면 `kv_cache[0, layer_id]`도 동시에 갱신된다.

### 블록 수 자동 계산 (`_calculate_num_kv_blocks()`)

`mini-vllm-study/minivllm/engine/inference_executor.py:499`

```
가용 메모리 계산 (CUDA/NPU):
  available = total × utilization − used − peak + current

블록 수:
  num_blocks = available ÷ bytes_per_block

상한 적용:
  max_total_blocks = max_num_seqs × ceil(max_model_len / block_size)
  num_blocks = min(num_blocks, max_total_blocks)
```

| 변수 | 의미 |
|------|------|
| `total` | GPU 전체 VRAM |
| `utilization` | `device_memory_utilization` 설정값 (기본 0.9) |
| `used` | 현재 사용 중인 메모리 (모델 가중치 포함) |
| `peak` | warmup 전까지 피크 메모리 (activations 포함) |
| `current` | peak 계산 시 중복 차감 보정 |

CPU/MPS는 unified memory 구조이므로 `free × utilization`으로 단순 계산한다.

---

## 3. 실행 컨텍스트 (`utils/context.py`)

`mini-vllm-study/minivllm/utils/context.py`

### `contextvars.ContextVar` — 스레드/코루틴 격리 안전

Python 표준 라이브러리 `contextvars.ContextVar`는 스레드별 독립 값을 보장한다. asyncio 코루틴 간에도 격리되므로 멀티스레드 TP(Tensor Parallel) worker와 async 서버 모두에서 안전하다.

```python
_CONTEXT_VAR: ContextVar[Context | None] = ContextVar("inference_context", default=None)
```

### `set_context(ctx)` → `model.forward()` → `reset_context(token)`

```
InferenceExecutor._prepare_prefill/decode_input()
  → set_context(is_prefill=..., slot_mapping=..., block_tables=..., ...)
  → _execute_model()
       → model.forward()
            └─ Attention.forward()
                 └─ get_context()  ← is_prefill, slot_mapping 등 읽기
  → (완료 후) finally: reset_context()
```

`reset_context()`는 `execute_batch()`의 `finally` 블록에서 호출되므로 예외 발생 시에도 컨텍스트 누수가 없다.

### Context 데이터클래스 필드

`mini-vllm-study/minivllm/utils/context.py:68`

| 필드 | dtype/shape | Prefill | Decode |
|------|------------|---------|--------|
| `is_prefill` | bool | True | False |
| `max_seqlen_q` | int | 최대 쿼리 길이 | 0 (미사용) |
| `max_seqlen_k` | int | 최대 키 길이 | 0 (미사용) |
| `cum_seqlens_q` | int32 `[B+1]` | `[0, l0, l0+l1, ...]` | None |
| `cum_seqlens_k` | int32 `[B+1]` | 캐시 포함 누적 길이 | None |
| `slot_mapping` | int32 `[T]` or `[B]` | 미캐시 토큰별 슬롯 | 시퀀스별 슬롯 1개 |
| `context_lens` | int32 `[B]` | None | 시퀀스별 현재 길이 |
| `block_tables` | int32 `[B, max_blocks]` | 프리픽스 블록 참조용 | 전체 블록 ID 테이블 |

---

## 4. Prefill 실행 경로 (`_prepare_prefill_input()`)

`mini-vllm-study/minivllm/engine/inference_executor.py:831`

### 입력 구성 원리

Prefill은 여러 시퀀스의 프롬프트 토큰을 패딩 없이 1D로 연결(packed)하여 forward에 전달한다. 프리픽스 캐시 히트 토큰(`num_cached_tokens`)은 건너뛰고 미캐시 토큰만 포함한다.

```
시퀀스 A (총 10 토큰, 4 캐시 히트) → 미캐시 토큰 6개
시퀀스 B (총  8 토큰, 0 캐시 히트) → 미캐시 토큰 8개
시퀀스 C (총  5 토큰, 5 캐시 히트) → 미캐시 토큰 0개 (전부 히트)

input_ids shape: [6 + 8 + 0] = [14]
```

### `slot_mapping` 계산

```python
for i in range(seq.num_cached_blocks, seq.num_blocks):
    start = seq.block_table[i] * self.block_size
    if i != seq.num_blocks - 1:
        end = start + self.block_size           # 중간 블록: 전체
    else:
        end = start + seq.last_block_num_tokens  # 마지막 블록: 실제 토큰 수만큼
    slot_mapping.extend(range(start, end))
```

```
슬롯 번호 = block_id × block_size + offset_within_block

예: block_id=3, block_size=64, offset=7 → slot = 3×64+7 = 199
```

### `cu_seqlens_q/k`: 누적 길이 배열

```
시퀀스 A: 미캐시 길이=6, 전체 길이=10
시퀀스 B: 미캐시 길이=8, 전체 길이=8

cum_seqlens_q = [0, 6, 14]     # 쿼리 (미캐시 토큰 기준)
cum_seqlens_k = [0, 10, 18]    # 키 (캐시 포함 전체 길이 기준)
```

`cum_seqlens_q`는 FlashAttention의 `flash_attn_varlen_func`에 전달되어, 패킹된 텐서를 시퀀스별로 분리하는 데 사용된다.

### logits 추출: cumsum으로 마지막 토큰 위치 계산

`mini-vllm-study/minivllm/engine/inference_executor.py:781`

```python
if prefill and logits.size(0) > len(sequences):
    seq_lengths = [len(seq) for seq in sequences]      # 전체 토큰 수
    last_indices = (
        torch.cumsum(torch.tensor(seq_lengths, device=logits.device), dim=0)
        - 1  # 0-base: cumsum 마지막 값 - 1 = 마지막 토큰 절대 위치
    )
    logits = logits[last_indices]   # [num_seqs, vocab_size]
```

예시:
```
seq_lengths = [6, 8]                 # 미캐시 토큰 수
cumsum      = [6, 14]
last_indices= [5, 13]               # 각 시퀀스 마지막 토큰의 절대 인덱스
logits      = all_logits[[5, 13]]   # shape: [2, vocab_size]
```

---

## 5. Decode 실행 경로 (`_prepare_decode_input()`)

`mini-vllm-study/minivllm/engine/inference_executor.py:941`

### 입력 구성 원리

Decode는 각 시퀀스의 **마지막 생성 토큰 1개**만 입력으로 받는다. 배치 크기 = 시퀀스 수.

```
시퀀스 A (현재 길이 15) → last_token 1개, position=14, context_len=15
시퀀스 B (현재 길이  9) → last_token 1개, position=8,  context_len=9

input_ids shape: [2]  (시퀀스당 1 토큰)
```

### `block_tables` 구성

각 시퀀스의 `block_table`(물리 블록 ID 목록)을 2D 텐서로 변환한다. 짧은 시퀀스의 빈 슬롯은 `-1`(sentinel)로 패딩.

```
시퀀스 A block_table = [3, 7, 12]      (3개 블록 사용)
시퀀스 B block_table = [5]             (1개 블록 사용)

block_tables = [[3,  7, 12],
                [5, -1, -1]]           (shape: [2, 3])
```

### `context_lens`: 각 시퀀스의 현재 총 토큰 수

```python
self._decode_context_lens_cpu[i] = len(seq)  # = prompt 길이 + 생성 토큰 수
```

flash_attn_with_kvcache가 이 값을 참조해 KV 캐시에서 몇 토큰까지 유효한지 알아낸다.

### Decode slot_mapping 계산

```python
slot = seq.block_table[-1] * self.block_size + seq.last_block_num_tokens - 1
```

```
= 마지막 블록의 첫 슬롯 절대 번호 + 현재 블록 내 오프셋
= block_id × block_size + (현재 블록 내 토큰 수 - 1)

예: block_id=12, block_size=64, last_block_num_tokens=5
  → slot = 12×64 + 5 - 1 = 768 + 4 = 772
```

이 슬롯에 이번 Decode 토큰의 K/V가 저장된다.

---

## 6. CUDA Graph 캡처 (`capture_device_graphs()`)

`mini-vllm-study/minivllm/engine/inference_executor.py:1361`

### 사전 캡처 배치 크기 목록

```python
self.graph_bs = [1, 2, 4, 8]
self.graph_bs.extend(range(12, 16, 4))    # [12]
self.graph_bs.extend(range(16, min(max_batch_size, 512) + 1, 16))
# 예: max_batch_size=64 → [16, 32, 48, 64]
# 최종: [1, 2, 4, 8, 12, 16, 32, 48, 64]
```

소규모 배치에서는 2의 거듭제곱 밀도로, 중간 이상 크기에서는 16 단위로 캡처한다. 실행 시 `batch_size` 이상인 가장 작은 캡처 크기를 선택해 패딩 실행한다.

### 정적 입력 버퍼 (`graph_vars`)

`mini-vllm-study/minivllm/engine/inference_executor.py:1412`

```python
self.graph_vars = {
    "input_ids"  : torch.zeros(max_bs, dtype=torch.long,  device=device),
    "positions"  : torch.zeros(max_bs, dtype=torch.long,  device=device),
    "slot_mapping": torch.full((max_bs,), -1, dtype=torch.int32, device=device),
    "context_lens": torch.zeros(max_bs, dtype=torch.int32, device=device),
    "block_tables": torch.zeros(max_bs, max_blocks, dtype=torch.int32, device=device),
    "outputs"    : torch.zeros(max_bs, hidden_size, device=device),
}
```

모든 캡처 크기가 이 버퍼를 공유한다. 배치 크기 bs의 그래프는 `graph_vars["input_ids"][:bs]` 슬라이스를 사용한다.

### 캡처 절차 (`_capture_graph_for_batch_size()`)

`mini-vllm-study/minivllm/engine/inference_executor.py:1450`

```python
# 1. 공유 버퍼의 배치 크기 슬라이스 추출 (뷰, 복사 아님)
input_ids     = self.graph_vars["input_ids"][:batch_size]
positions     = self.graph_vars["positions"][:batch_size]
slot_mapping  = self.graph_vars["slot_mapping"][:batch_size]
context_lens  = self.graph_vars["context_lens"][:batch_size]
block_tables  = self.graph_vars["block_tables"][:batch_size]
outputs       = self.graph_vars["outputs"][:batch_size]

# 2. 어텐션 레이어가 읽을 컨텍스트 설정
set_context(is_prefill=False, slot_mapping=..., context_lens=..., block_tables=...)

# 3. Warmup forward (커널 초기화, 그래프 밖에서 한 번 실행)
outputs.copy_(self.model(input_ids=input_ids, positions=positions)[0])

# 4. Graph 캡처 시작
graph = DeviceGraph()
with DeviceGraphContext(graph):
    outputs.copy_(self.model(input_ids=input_ids, positions=positions)[0])
    # 이 forward 연산 시퀀스가 graph에 기록됨
    # outputs의 메모리 주소도 고정됨

# 5. 완료 대기 + 컨텍스트 초기화
self.graphs[batch_size] = graph
synchronize(self.device)
reset_context()
```

**중요**: 캡처 시 사용한 텐서의 **메모리 주소**가 graph에 고정된다. replay() 시 반드시 같은 버퍼(graph_vars)를 사용해 in-place 업데이트해야 한다.

### 실행 시: 버퍼 in-place 갱신 → `graph.replay()`

`mini-vllm-study/minivllm/engine/inference_executor.py:1214`

```python
batch_size = input_ids.size(0)
graph_bs = next((bs for bs in self.graph_bs if bs >= batch_size), None)
graph = self.graphs[graph_bs]

# in-place 업데이트 (같은 주소, 새 값)
vars_dict["input_ids"][:batch_size]  = input_ids
vars_dict["positions"][:batch_size]  = positions
vars_dict["slot_mapping"].fill_(-1)
vars_dict["slot_mapping"][:batch_size] = ctx.slot_mapping
vars_dict["context_lens"].zero_()
vars_dict["context_lens"][:batch_size] = ctx.context_lens
vars_dict["block_tables"][:batch_size, :max_blocks] = ctx.block_tables

# 캡처된 GPU 연산 시퀀스 재실행 (CPU 커널 제출 오버헤드 없음)
graph.replay()

# 출력은 graph_bs 크기이므로 실제 배치 크기로 잘라 logits 계산
return self.model.compute_logits(vars_dict["outputs"][:batch_size])
```

### `enforce_eager=True`: CUDA Graph 완전 비활성화

```python
if (
    not is_prefill
    and not self.enforce_eager          ← True이면 Graph 비사용
    and supports_cuda_graph()
    and self.graphs
):
    ...
return self._execute_eager(input_ids, positions)  # 매 스텝 즉시 실행
```

`enforce_eager=True`이면 `capture_device_graphs()`도 조기 반환하므로 `self.graphs`가 빈 딕셔너리다. 디버깅/개발 시 유용하나 Decode 단계 속도가 느리다.

---

## 7. CPU 스테이징 버퍼 — Decode H2D 전송 최적화

`mini-vllm-study/minivllm/engine/inference_executor.py:342`

매 Decode 스텝마다 `torch.tensor(list)` 호출 시 발생하는 비용:
1. Python 리스트 → CPU 메모리 할당
2. H2D(Host-to-Device) 전송 N회 (원소별)

이를 줄이기 위해 초기화 시 CPU와 디바이스 버퍼를 한 번 할당하고, 스텝마다 CPU 버퍼를 채운 뒤 `copy_(non_blocking=True)` 한 번으로 H2D 전송을 완료한다.

```
초기화 (1회):
  _decode_input_ids_cpu    = torch.zeros(max_seqs, dtype=torch.long)       # CPU
  _decode_input_ids        = torch.zeros(max_seqs, dtype=torch.long,
                                          device=cuda)                     # GPU

매 Decode 스텝:
  # CPU에서 값 채우기 (H2D 없음)
  for i, seq in enumerate(sequences):
      _decode_input_ids_cpu[i] = seq.last_token

  # 일괄 H2D 복사 (non_blocking → 비동기)
  _decode_input_ids[:batch_size].copy_(
      _decode_input_ids_cpu[:batch_size], non_blocking=True
  )
```

`non_blocking=True`: H2D 복사를 CUDA 스트림에 넣고 CPU는 즉시 반환한다. 다음 CUDA 연산이 이 복사를 자동으로 기다린다.

### 블록 테이블 사전 할당 버퍼

```python
max_blocks = (max_model_len + block_size - 1) // block_size  # 시퀀스당 최대 블록 수

_decode_block_tables_cpu = torch.full((max_seqs, max_blocks), -1, dtype=torch.int32)
_decode_block_tables     = torch.full((max_seqs, max_blocks), -1, dtype=torch.int32,
                                       device=device)
```

배치 크기나 블록 수가 사전 할당 버퍼를 초과하면 `_prepare_block_tables()`로 동적 할당 fallback.

---

## 8. `execute_batch()` 전체 흐름

`mini-vllm-study/minivllm/engine/inference_executor.py:743`

```
execute_batch(sequences, prefill)
  │
  ├─ 1. _prepare_batch_input(sequences, prefill)
  │       ├─ if prefill: _prepare_prefill_input()
  │       │     → input_ids(패킹), positions, slot_mapping, cu_seqlens
  │       │     → set_context(is_prefill=True, ...)
  │       └─ else: _prepare_decode_input()
  │             → 사전 할당 버퍼에 값 채우기 → copy_(non_blocking)
  │             → set_context(is_prefill=False, ...)
  │       → (input_ids, positions) 반환
  │
  ├─ 2. _execute_model(input_ids, positions, is_prefill)
  │       ├─ if Decode + not enforce_eager + graph exists:
  │       │     _execute_with_cuda_graph()
  │       │       → graph_vars 버퍼 in-place 갱신
  │       │       → graph.replay()
  │       │       → model.compute_logits(outputs[:batch_size])
  │       └─ else:
  │             _execute_eager()
  │               → with torch.inference_mode(): model(input_ids, positions)
  │               → model.compute_logits(hidden_states)
  │       → logits 반환
  │
  ├─ 3. [Prefill 한정] 마지막 토큰 logits 추출
  │       → cu_seqlens_q로 각 시퀀스 마지막 토큰 위치 계산
  │       → logits = all_logits[last_indices]  shape: [num_seqs, vocab_size]
  │
  ├─ 4. _sample_tokens(logits, sequences)
  │       → temperature, top_p, top_k, min_p를 배치 텐서로 변환
  │       → self.sampler(logits, ...) → next_tokens.tolist()
  │
  ├─ 5. _update_metrics(sequences, prefill)
  │       → total_prefill_tokens / total_decode_tokens 갱신
  │
  └─ finally: reset_context()  (정상/예외 모두 보장)

반환: (logits, next_token_ids)
  logits        : shape [num_seqs, vocab_size], GPU 텐서
  next_token_ids: Python 정수 리스트, 길이 = num_seqs
```

---

## 9. Prefill logits 추출 — `cu_seqlens_q` 활용

`mini-vllm-study/minivllm/engine/inference_executor.py:781`

Prefill에서 `model.forward()`는 모든 미캐시 토큰에 대한 hidden state를 반환한다 (shape: `[총 미캐시 토큰 수, vocab_size]`). 다음 토큰 샘플링을 위해서는 각 시퀀스의 **마지막 토큰** logits만 필요하다.

**공식:**

```python
seq_lengths  = [len(seq) for seq in sequences]     # 각 시퀀스 총 토큰 수
# (프리픽스 캐시 히트 제외한 미캐시 토큰 수를 사용하는 것이 정확하나,
#  현재 구현에서는 전체 길이를 사용하고 Prefill logits 크기와 비교해 추출)

last_positions = cumsum([len0, len1, len2]) - 1
             # = [len0-1, len0+len1-1, len0+len1+len2-1]
             # 각 시퀀스 마지막 토큰의 packed 텐서 내 절대 인덱스

logits = all_logits[last_positions]   # shape: [num_seqs, vocab_size]
```

예:
```
시퀀스 길이 = [6, 8]
cumsum      = [6, 14]
last_positions = [5, 13]
logits = all_logits[[5, 13]]   # 마지막 토큰 logits만 추출
```

---

## 10. 코드 참조 요약

| 위치 | 내용 |
|------|------|
| `mini-vllm-study/minivllm/engine/inference_executor.py:109` | `class InferenceExecutor` 정의 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:177` | `__init__`: dtype 결정, 사전 할당 버퍼 생성 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:297` | `_init_attributes()`: 그래프 컨테이너 + Decode 버퍼 초기화 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:397` | `initialize()`: KV 캐시 + 샘플러 + 최적화 + warmup |
| `mini-vllm-study/minivllm/engine/inference_executor.py:429` | `_allocate_kv_cache()`: KV 캐시 텐서 할당 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:499` | `_calculate_num_kv_blocks()`: 가용 메모리 기반 블록 수 계산 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:599` | `_assign_kv_cache_to_layers()`: 레이어별 슬라이스 주입 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:743` | `execute_batch()`: 배치 추론 진입점 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:831` | `_prepare_prefill_input()`: Prefill 입력 텐서 구성 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:941` | `_prepare_decode_input()`: Decode 입력 텐서 구성 (사전 할당 버퍼) |
| `mini-vllm-study/minivllm/engine/inference_executor.py:1136` | `_execute_model()`: CUDA Graph vs Eager 분기 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:1214` | `_execute_with_cuda_graph()`: Graph replay 실행 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:1282` | `_sample_tokens()`: logits → 다음 토큰 샘플링 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:1361` | `capture_device_graphs()`: 배치 크기별 Graph 캡처 |
| `mini-vllm-study/minivllm/engine/inference_executor.py:1450` | `_capture_graph_for_batch_size()`: 단일 배치 크기 Graph 캡처 |
| `mini-vllm-study/minivllm/utils/context.py:68` | `Context` 데이터클래스 정의 |
| `mini-vllm-study/minivllm/utils/context.py:176` | `get_context()`, `set_context()`, `reset_context()` |
