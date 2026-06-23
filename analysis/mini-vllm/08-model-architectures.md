# 08 지원 모델 아키텍처 비교 분석

> 관련 문서: [[01-architecture-overview]] [[07-sampling-pipeline]] [[05-kv-cache-paged-attention]]
> 코드 기준: `mini-vllm-study/minivllm/models/`

---

## 1. 지원 모델 전체 비교 표

| 항목 | Qwen2 / Qwen2.5 | Qwen3 | OPT | GPT-2 |
|------|-----------------|-------|-----|-------|
| Normalization | RMSNorm | RMSNorm | LayerNorm | LayerNorm |
| MLP 활성화 | SwiGLU (SiLU + multiply) | SwiGLU | ReLU | GELU (erf 기반) |
| 위치 인코딩 | RoPE (θ=1,000,000) | RoPE (θ=10,000 + rope_scaling) | Learned PE (offset+2) | Learned PE (offset 없음) |
| Attention 종류 | GQA | GQA + QK-Norm | MHA | MHA |
| QKV bias | True | False | True | True (Conv1D) |
| Pre/Post Norm | Pre-Norm | Pre-Norm | 선택 가능 (`do_layer_norm_before`) | Pre-Norm |
| 사전학습 컨텍스트 | 최대 128K | 32K+ (rope_scaling으로 확장) | 2K | 1K |
| HF 아키텍처 이름 | `Qwen2ForCausalLM` | `Qwen3ForCausalLM` | `OPTForCausalLM` | `GPT2LMHeadModel` |

**GQA (Grouped Query Attention)**: `num_kv_heads < num_heads`. K/V 헤드 수를 줄여 KV 캐시 크기와 어텐션 대역폭을 절약. Qwen2/3만 해당.

**MHA (Multi-Head Attention)**: `num_kv_heads == num_heads`. 전통적 방식. OPT/GPT-2 해당.

---

## 2. Transformer 블록 공통 구조 (Pre-Norm)

모든 지원 모델은 Pre-Norm 방식의 Transformer 블록을 사용한다 (OPT는 선택 가능).

```
입력 x
  │
  ├─ Pre-Norm (RMSNorm 또는 LayerNorm)
  │       ↓
  │  Self-Attention
  │   ├─ QKV 프로젝션 (GQA 또는 MHA)
  │   ├─ RoPE 또는 Learned PE
  │   ├─ FlashAttention (KV 캐시 저장 + 어텐션 계산)
  │   └─ 출력 프로젝션
  │
x = x + attn_out          ← 잔차 연결 1
  │
  ├─ Pre-Norm
  │       ↓
  │  MLP / FFN
  │   ├─ SwiGLU: [gate_proj, up_proj] → silu(gate) * up → down_proj
  │   └─ 또는 FC-ReLU-FC / FC-GELU-FC
  │
x = x + mlp_out           ← 잔차 연결 2
  │
출력 x
```

**Pre-Norm 장점**: 학습 안정성 향상. 깊은 모델에서 gradient 소실 문제 완화.

---

## 3. QwenBase 아키텍처 (공통 베이스)

### 3.1 상속 계층

```
QwenAttention    ─┐
QwenMLP          ─┤  (qwen_base.py — 실제 구현)
QwenDecoderLayer ─┤
QwenModel        ─┤
QwenForCausalLM  ─┘
        ↑
Qwen2Attention   (default_qkv_bias=True,  default_rope_theta=1000000)
Qwen2DecoderLayer (attention_cls=Qwen2Attention)
Qwen2Model        (decoder_layer_cls=Qwen2DecoderLayer)
Qwen2ForCausalLM  (model_cls=Qwen2Model)
        또는
Qwen3Attention   (default_qkv_bias=False, default_rope_theta=10000)
Qwen3DecoderLayer (attention_cls=Qwen3Attention)
Qwen3Model        (decoder_layer_cls=Qwen3DecoderLayer)
Qwen3ForCausalLM  (model_cls=Qwen3Model)
```

Qwen2/3은 클래스 변수 재정의만으로 분기. 모든 forward 로직은 `qwen_base.py`에 집중.

### 3.2 모듈 구성

```
QwenForCausalLM
  └─ QwenModel
       ├─ embed_tokens: VocabParallelEmbedding [vocab_size, hidden_size]
       ├─ layers: nn.ModuleList([QwenDecoderLayer × num_hidden_layers])
       │    └─ QwenDecoderLayer
       │         ├─ input_layernorm: RMSNorm(hidden_size)
       │         ├─ self_attn: QwenAttention
       │         │    ├─ qkv_proj: QKVParallelLinear
       │         │    ├─ o_proj: RowParallelLinear
       │         │    ├─ rotary_emb: get_rope(...)
       │         │    └─ attn: Attention (PagedAttention 래퍼)
       │         ├─ post_attention_layernorm: RMSNorm(hidden_size)
       │         └─ mlp: QwenMLP
       │              ├─ gate_up_proj: MergedColumnParallelLinear
       │              ├─ act_fn: SiluAndMul
       │              └─ down_proj: RowParallelLinear
       └─ norm: RMSNorm(hidden_size)  ← 최종 정규화
  └─ lm_head: ParallelLMHead [hidden_size, vocab_size]
```

### 3.3 Tensor Parallelism (TP) 설계

```
입력 [total_tokens, hidden_size]
  │
  ├─ QKV proj (ColumnParallelLinear)
  │     각 GPU rank: Q/K/V를 num_heads//tp_size 헤드씩 담당
  │     all-reduce: 없음 (ColumnParallel은 독립 계산)
  │
  ├─ FlashAttention (각 rank 독립 실행)
  │
  ├─ out_proj (RowParallelLinear)
  │     각 rank의 부분 출력을 all-reduce → hidden_size 복원
  │
  ├─ gate_up_proj (MergedColumnParallelLinear)
  │     intermediate_size 축 분할 (각 rank가 일부 뉴런 담당)
  │     all-reduce: 없음
  │
  └─ down_proj (RowParallelLinear)
        각 rank의 부분 출력을 all-reduce → hidden_size 복원
```

TP 시 레이어당 all-reduce 2회 (out_proj, down_proj). ColumnParallel → RowParallel → all-reduce가 반복 패턴.

**코드 위치**: `mini-vllm-study/minivllm/models/qwen_base.py`

---

## 4. Qwen3 차이점: QK-Norm

**활성화 조건**: `qkv_bias=False` (Qwen3 기본값).

```python
# QwenAttention.__init__()
if not self.qkv_bias:           # Qwen3: qkv_bias=False → QK-Norm 활성화
    self.q_norm = RMSNorm(self.head_dim, eps=rms_norm_eps)
    self.k_norm = RMSNorm(self.head_dim, eps=rms_norm_eps)
else:                            # Qwen2: qkv_bias=True → QK-Norm 없음
    self.q_norm = None
    self.k_norm = None
```

**forward()에서 적용 위치**:
```python
q, k, v = qkv.split(...)
q = q.view(total_tokens, self.num_heads, self.head_dim)
k = k.view(total_tokens, self.num_kv_heads, self.head_dim)

if self.q_norm is not None:
    q, _ = self.q_norm(q)   # RoPE 이전에 Q 정규화
    k, _ = self.k_norm(k)   # RoPE 이전에 K 정규화

q, k = self.rotary_emb(positions, q, k)   # RoPE 적용
```

**QK-Norm이 필요한 이유**: bias 없을 때 Q/K 벡터의 스케일이 발산할 수 있음. RMSNorm으로 헤드 차원 정규화 → 어텐션 스코어 안정화, FP16 수렴 개선.

**RoPE theta 차이**:
- Qwen2: `default_rope_theta = 1000000` — 긴 컨텍스트(최대 128K) 지원
- Qwen3: `default_rope_theta = 10000` — 기본 GPT 스타일, `rope_scaling`으로 컨텍스트 확장

**코드 위치**:
- `mini-vllm-study/minivllm/models/qwen_base.py:301-312` — QK-Norm 초기화
- `mini-vllm-study/minivllm/models/qwen_base.py:374-416` — QK-Norm forward 적용
- `mini-vllm-study/minivllm/models/qwen3.py:66-94` — Qwen3Attention 클래스 변수

---

## 5. OPT 아키텍처 (`models/opt.py`)

### 5.1 특징

- **Learned Positional Embedding** (`OPTLearnedPositionalEmbedding`): RoPE 미사용. 위치 인덱스로 고정 크기 임베딩 테이블 조회.
  - **offset=2**: `positions + 2` 인덱스로 조회. OPT 고유 설계 (HuggingFace 원본과 호환).
- **MHA (Multi-Head Attention)**: `num_kv_heads == num_heads` (GQA 아님).
- **Pre/Post Norm 선택**: `do_layer_norm_before` 플래그로 Pre-Norm(기본) 또는 Post-Norm 선택 가능.
- **GELU 비사용, ReLU 사용**: FFN 활성화가 ReLU.
- **선택적 레이어**:
  - `project_in/project_out`: 임베딩 차원과 hidden 차원이 다를 때 투영 레이어 (일부 OPT 변형)
  - `final_layer_norm`: Post-Norm 시 마지막 정규화

### 5.2 OPT 모듈 구성

```
OPTForCausalLM
  └─ OPTModel
       └─ OPTDecoder
            ├─ embed_tokens: VocabParallelEmbedding
            ├─ embed_positions: OPTLearnedPositionalEmbedding (offset=2)
            ├─ (project_in: ColumnParallel — 선택적)
            ├─ layers: [OPTDecoderLayer × num_hidden_layers]
            │    └─ OPTDecoderLayer
            │         ├─ self_attn_layer_norm / final_layer_norm: nn.LayerNorm
            │         ├─ self_attn: OPTAttention (MHA, RoPE 없음)
            │         │    ├─ qkv_proj: QKVParallelLinear (num_kv_heads=num_heads)
            │         │    └─ out_proj: RowParallelLinear
            │         ├─ fc1: ColumnParallelLinear → ReLU
            │         └─ fc2: RowParallelLinear
            ├─ (final_layer_norm: nn.LayerNorm — 선택적)
            └─ (project_out: nn.Linear — 선택적)
  └─ lm_head: ParallelLMHead
```

**코드 위치**: `mini-vllm-study/minivllm/models/opt.py`

---

## 6. GPT-2 아키텍처 (`models/gpt2.py`)

### 6.1 특징

- **Learned Positional Embedding**: `nn.Embedding(n_positions, n_embd)`. offset 없음.
- **임베딩 공유(Tied Weights)**: 입력 임베딩(`embed_tokens.weight`)과 LM 헤드(`lm_head`)가 동일 가중치를 공유.
- **GeLU 활성화**: OpenAI GELU = `0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))`.
- **MHA**: `num_kv_heads == num_heads`.

### 6.2 HuggingFace Conv1D 전치 문제

HuggingFace GPT-2는 가중치를 `Conv1D` 형식으로 저장:
- Conv1D weight shape: `(in_features, out_features)` → PyTorch `nn.Linear`: `(out_features, in_features)`
- `load_weights()`에서 전치(`contiguous().T`)가 필요.

```python
# GPT-2 weight 로딩 시
if "c_attn" in name or "c_proj" in name or "c_fc" in name:
    # Conv1D (in, out) → Linear (out, in) 전치
    weight = weight.contiguous().T
```

### 6.3 c_attn 분리 (QKV 샤드)

HuggingFace GPT-2는 Q/K/V를 `c_attn` 하나의 가중치에 합쳐 저장:
```
c_attn.weight: [n_embd, 3 * n_embd]  (Conv1D 형식)
전치 후: [3 * n_embd, n_embd]
분할: Q = [:n_embd, :], K = [n_embd:2*n_embd, :], V = [2*n_embd:, :]
```

mini-vLLM의 `QKVParallelLinear`는 Q/K/V를 각각의 슬라이스로 기대하므로 이 분리 처리가 필요.

### 6.4 GPT-2 모듈 구성

```
GPT2ForCausalLM
  └─ GPT2Model
       ├─ embed_tokens: VocabParallelEmbedding (lm_head와 가중치 공유)
       ├─ embed_positions: nn.Embedding(n_positions, n_embd)
       ├─ h: [GPT2Block × n_layer]
       │    └─ GPT2Block
       │         ├─ ln_1: nn.LayerNorm (Pre-Norm, Attention 전)
       │         ├─ attn: GPT2Attention
       │         │    ├─ qkv_proj: QKVParallelLinear (c_attn에서 분리)
       │         │    └─ out_proj: RowParallelLinear (c_proj)
       │         ├─ ln_2: nn.LayerNorm (Pre-Norm, MLP 전)
       │         └─ mlp: GPT2MLP
       │              ├─ fc1: ColumnParallelLinear (c_fc) → GELU
       │              └─ fc2: RowParallelLinear (c_proj)
       └─ ln_f: nn.LayerNorm (최종 정규화)
  └─ lm_head: ParallelLMHead (embed_tokens.weight 공유)
```

**코드 위치**: `mini-vllm-study/minivllm/models/gpt2.py`

---

## 7. 가중치 로딩 (`models/manager.py` + `utils/loader.py`)

### 7.1 로딩 순서

```
ModelManager._load_model()
  → AutoConfig.from_pretrained(model_path)       # HF config 로드
  → create_model(hf_config)                       # 모델 인스턴스화 (가중치 미로드)
  → load_model(model, model_path)                 # 가중치 로드
      → safetensors 파일 탐색 (model.safetensors.index.json 우선)
      → .bin 파일 폴백
      → weight_loader 속성으로 파라미터별 커스텀 로딩
  → model.to(device, dtype)                       # GPU 이동 및 dtype 변환
```

### 7.2 `weight_loader` 속성

각 파라미터는 `weight_loader` 속성으로 커스텀 로딩 함수를 가진다:

```python
param.weight_loader(param, loaded_weight, shard_id)
```

- `ColumnParallelLinear`: 출력 차원 슬라이스 `[out_start:out_end, :]`
- `RowParallelLinear`: 입력 차원 슬라이스 `[:, in_start:in_end]`
- `QKVParallelLinear`: Q/K/V 각각의 샤드 슬라이스

### 7.3 `packed_modules_mapping` (병합 가중치 처리)

HF 체크포인트에서 분리된 가중치를 mini-vLLM의 병합 파라미터로 매핑:

```python
packed_modules_mapping = {
    "qkv_proj": ["q_proj", "k_proj", "v_proj"],   # QKV 병합
    "gate_up_proj": ["gate_proj", "up_proj"],        # SwiGLU 게이트+업 병합
}
```

HF 체크포인트의 `q_proj.weight`, `k_proj.weight`, `v_proj.weight` → 내부 `qkv_proj.weight`로 합산.

---

## 8. 모델 레지스트리 (`models/registry.py`)

```python
SUPPORTED_MODELS = {
    "Qwen2ForCausalLM": Qwen2ForCausalLM,
    "Qwen3ForCausalLM": Qwen3ForCausalLM,
    "OPTForCausalLM":   OPTForCausalLM,
    "GPT2LMHeadModel":  GPT2ForCausalLM,  # HF 이름 ≠ 내부 클래스 이름
}

TYPE_TO_ARCH = {   # model_type 폴백 조회
    "qwen2": "Qwen2ForCausalLM",
    "qwen3": "Qwen3ForCausalLM",
    "opt":   "OPTForCausalLM",
    "gpt2":  "GPT2LMHeadModel",
}
```

**아키텍처 감지 우선순위**:
1. `hf_config.architectures` 리스트에서 `SUPPORTED_MODELS` 키 매칭
2. 실패 시 `hf_config.model_type`(소문자)으로 `TYPE_TO_ARCH` 조회

**새 모델 추가 방법**:
1. `models/new_model.py` 구현 (forward/set_kv_cache/compute_logits 인터페이스 필수)
2. `SUPPORTED_MODELS`에 HF 아키텍처 이름 추가
3. `TYPE_TO_ARCH`에 `model_type` 폴백 추가

**코드 위치**: `mini-vllm-study/minivllm/models/registry.py`

---

## 9. KV 캐시 주입 방식

**주입 시점**: 모델 로드 완료 후 `InferenceExecutor._assign_kv_cache_to_layers()`에서 1회 실행.

**주입 방식**:
```python
for layer_idx, layer in enumerate(model.model.layers):
    layer.self_attn.attn.k_cache = kv_cache[:, :, layer_idx, 0, ...]  # K 캐시 슬라이스
    layer.self_attn.attn.v_cache = kv_cache[:, :, layer_idx, 1, ...]  # V 캐시 슬라이스
```

- `kv_cache` 전체 텐서: `[num_blocks, block_size, num_layers, 2, num_kv_heads, head_dim]`
- 레이어별 슬라이스를 각 `Attention` 인스턴스에 주입
- Attention 레이어가 `self.k_cache`, `self.v_cache`로 보유하고 forward 시 직접 접근

**이점**: 레이어가 KV 캐시 전체를 알 필요 없음. 자신의 레이어 인덱스 슬라이스만 보유.

---

## 10. rope_theta 추출 헬퍼 (`_resolve_rope_theta`)

HuggingFace 체크포인트마다 `rope_theta` 저장 위치가 다름:

```python
def _resolve_rope_theta(config, default):
    if hasattr(config, "rope_theta"):        # 1순위: config.rope_theta 직접
        return config.rope_theta
    rope_params = getattr(config, "rope_parameters", None) or {}
    if "rope_theta" in rope_params:          # 2순위: rope_parameters.rope_theta
        return rope_params["rope_theta"]
    rope_scaling = getattr(config, "rope_scaling", None) or {}
    if "rope_theta" in rope_scaling:         # 3순위: rope_scaling.rope_theta
        return rope_scaling["rope_theta"]
    return default                           # 폴백: 서브클래스 기본값
```

**코드 위치**: `mini-vllm-study/minivllm/models/qwen_base.py:91-117`

`_resolve_rope_scaling`도 유사하게 `rope_scaling` 딕셔너리에서 비관련 키(`rope_theta`, `rope_type` 등)를 필터링해 순수 스케일링 파라미터(`factor`, `type`, `low_freq_factor` 등)만 추출.

---

## 11. 모델 표준 인터페이스

모든 지원 모델이 구현해야 하는 공통 인터페이스:

```python
class AnyForCausalLM(nn.Module):
    def forward(self, input_ids: Tensor, positions: Tensor) -> Tensor:
        """input_ids [total_tokens], positions [total_tokens] → hidden_states"""
        ...

    def compute_logits(self, hidden_states: Tensor) -> Tensor:
        """hidden_states [total_tokens, hidden_size] → logits [total_tokens, vocab_size]"""
        ...

    def set_kv_cache(self, kv_cache: Tensor) -> None:
        """레이어별 KV 캐시 텐서 주입"""
        ...

    def load_weights(self, weights: Iterable[tuple[str, Tensor]]) -> None:
        """HuggingFace 가중치 이름 매핑 및 로딩"""
        ...
```

`InferenceExecutor`는 이 인터페이스만 의존하므로 새 모델 추가 시 위 메서드 구현 + 레지스트리 등록만 하면 됨.

---

## 12. 모델별 코드 참조

| 파일 | 행 수 | 주요 내용 |
|------|-------|---------|
| `mini-vllm-study/minivllm/models/qwen_base.py` | 994 | QwenAttention (GQA+QK-Norm+RoPE), QwenMLP (SwiGLU), QwenDecoderLayer, QwenModel, QwenForCausalLM |
| `mini-vllm-study/minivllm/models/qwen2.py` | 146 | Qwen2 클래스 변수 재정의 (qkv_bias=True, theta=1000000) |
| `mini-vllm-study/minivllm/models/qwen3.py` | 158 | Qwen3 클래스 변수 재정의 (qkv_bias=False, QK-Norm 활성화) |
| `mini-vllm-study/minivllm/models/opt.py` | 821 | OPTLearnedPositionalEmbedding(+2), OPTAttention (MHA, ReLU FFN), Pre/Post-Norm 선택 |
| `mini-vllm-study/minivllm/models/gpt2.py` | 735 | GPT2Attention (MHA, Conv1D 전치), GPT2MLP (GELU), 임베딩 공유, c_attn QKV 분리 |
| `mini-vllm-study/minivllm/models/registry.py` | 169 | SUPPORTED_MODELS, TYPE_TO_ARCH, create_model() 팩토리 |
| `mini-vllm-study/minivllm/models/manager.py` | 555 | ModelManager: 초기화/로드/검증/cleanup 생명주기 관리 |
