# 06 — Attention 메커니즘 + FlashAttention + KV 캐시 저장 심층 분석

관련 문서: [[01-overview]] [[02-paged-attention-block-manager]] [[04-inference-executor]] [[05-tensor-parallelism-ipc]]

---

## 1. Attention 수식 복습

```
Attention(Q, K, V) = softmax( QK^T / sqrt(d_k) ) * V
```

**변수 정의**:
- `Q`: Query 행렬, shape `(seq_len, d_k)`
- `K`: Key 행렬, shape `(context_len, d_k)`
- `V`: Value 행렬, shape `(context_len, d_v)`
- `d_k`: head_dim (쿼리/키 차원, 보통 128)
- `sqrt(d_k)`: 스케일링 인수 — QK^T의 분산이 d_k에 비례해 커지는 것을 보정

**메모리 복잡도 문제**:
- 표준 구현: QK^T 행렬 `(seq_len, context_len)` 전체를 GPU 메모리에 저장 → O(seq_len²)
- FlashAttention: 타일(tile) 단위로 분할 처리 → GPU HBM 접근을 줄여 실질적으로 O(seq_len) 수준의 메모리 사용

---

## 2. Attention 레이어 구조

파일: `mini-vllm-study/minivllm/models/layers/attention.py`

```
QwenAttention.forward(x)
  │
  ├─ QKVParallelLinear(x)             → qkv 텐서 (통합 출력)
  │   Q = qkv[:, :num_heads * head_dim]
  │   K = qkv[:, num_heads*head_dim : (num_heads+num_kv_heads)*head_dim]
  │   V = qkv[:, (num_heads+num_kv_heads)*head_dim:]
  │
  ├─ RotaryEmbedding.forward(positions, Q, K)  → Q_rope, K_rope
  │
  ├─ Attention.forward(Q_rope, K_rope, V)
  │     ├─ backend.store_kv_cache(K, V, k_cache, v_cache, slot_mapping)
  │     └─ FlashAttention / NPUAttention / Fallback
  │
  └─ RowParallelLinear(attn_out)      → output (all-reduce 포함)
```

---

## 3. KV 캐시 저장 — slot_mapping 인덱싱

파일: `mini-vllm-study/minivllm/models/layers/attention_backend.py`

### 물리 슬롯 주소 계산

```
slot = block_id * block_size + intra_block_offset

예시:
block_size = 64
토큰 위치 100 → block_id = 100 // 64 = 1, offset = 100 % 64 = 36
slot = 1 * 64 + 36 = 100
```

### KV 캐시 저장 코드

```python
# 개념적 구현 (실제는 Triton 커널 또는 인덱스 연산으로 가속)
# slot_mapping: shape=(num_tokens,), 각 토큰의 물리 슬롯 번호
# k_cache: shape=(num_blocks, block_size, num_kv_heads, head_dim)
# k: shape=(num_tokens, num_kv_heads, head_dim)

# k_cache를 (num_blocks*block_size, num_kv_heads, head_dim)으로 평탄화
k_cache_flat = k_cache.view(-1, num_kv_heads, head_dim)
# slot_mapping이 가리키는 위치에 k를 기록
k_cache_flat[slot_mapping] = k  # 인덱스 scatter 연산
```

### 음수 슬롯: 프리픽스 캐시 히트

`slot_mapping`의 값이 음수(-1)이면 해당 토큰의 KV는 이미 캐시에 있어 저장을 건너뛴다. `store_kv_cache()` 내부에서 양수 슬롯만 필터링하여 처리.

### KV 캐시 텐서 레이아웃

```
k_cache: (num_blocks, block_size, num_kv_heads, head_dim)

예: num_blocks=512, block_size=64, num_kv_heads=4, head_dim=128
총 메모리: 512 * 64 * 4 * 128 * 2 bytes(bf16) = 256 MB (레이어당)
```

---

## 4. RoPE (Rotary Position Embedding)

파일: `mini-vllm-study/minivllm/models/layers/rotary_embedding.py`

### 핵심 공식

```
역주파수:
  θ_i = 1 / base^(2i/d),  i = 0, 1, ..., d/2 - 1
  base: Llama=10000, Qwen2=1000000, Qwen3=1000000

위치 m에서의 주파수:
  freqs[m, i] = m * θ_i   (shape: max_pos × d/2)

cos/sin 캐시:
  cos_cache[m, i] = cos(m * θ_i)
  sin_cache[m, i] = sin(m * θ_i)

회전 변환 (복소수 곱셈의 실수 구현):
  x = [x₁, x₂]  (head_dim을 절반으로 분할)
  out = [x₁·cos(mθ) - x₂·sin(mθ),   x₂·cos(mθ) + x₁·sin(mθ)]
       ↑ Re(z * e^{imθ})              ↑ Im(z * e^{imθ})
```

이는 복소수 z = x₁ + i·x₂에 e^{imθ}를 곱하는 것과 동일하다.

### rotate_half 구현

```python
# mini-vllm-study/minivllm/models/layers/rotary_embedding.py:154-164
def apply_rotary_emb(x, cos, sin):
    x1, x2 = torch.chunk(x.float(), 2, dim=-1)
    # x를 앞 절반(x₁)과 뒤 절반(x₂)으로 분리
    y1 = x1 * cos - x2 * sin  # 실수부 회전
    y2 = x2 * cos + x1 * sin  # 허수부 회전
    return torch.cat((y1, y2), dim=-1).to(x.dtype)
    # float32로 계산 후 원본 dtype으로 다운캐스트 (수치 안정성)
```

### RoPE의 상대 위치 인코딩 특성

Q와 K에 동일한 회전을 적용하면, 내적 Q^T K에서 절대 위치가 아닌 **상대 위치 차이**만 남는다:

```
Q_m^T K_n = (q * e^{imθ})^T (k * e^{inθ}) = q^T k * e^{i(n-m)θ}
```

위치 m의 쿼리와 위치 n의 키 사이의 어텐션 점수가 (n-m)에만 의존 → 외삽(extrapolation) 능력 향상.

### cos/sin 캐시 사전 계산

```python
# mini-vllm-study/minivllm/models/layers/rotary_embedding.py:270-290
t = torch.arange(max_position_embeddings, dtype=torch.float)  # [0, 1, ..., max_pos-1]
freqs = torch.einsum("i,j -> ij", t, inv_freq)                # (max_pos, d/2)
cos = freqs.cos()  # (max_pos, d/2)
sin = freqs.sin()  # (max_pos, d/2)
self.register_buffer("cos_cache", cos, persistent=False)
self.register_buffer("sin_cache", sin, persistent=False)
# persistent=False: state_dict에 포함 안 됨, 초기화 시 재계산
```

`forward`에서는 `positions` 인덱스로 조회만 하면 됨:

```python
cos = self.cos_cache[positions]  # 조회, 인덱스 = 토큰 위치
sin = self.sin_cache[positions]
cos = cos.unsqueeze(1)  # head 차원 추가 → 브로드캐스트
```

### NTK-Aware 스케일링 (동적 외삽)

학습 최대 길이보다 긴 시퀀스 처리를 위한 base 주파수 확장:

```
새 base = base * factor^(d / (d-2))

이론적 근거: NTK(Neural Tangent Kernel) 이론에서
고주파 성분을 보존하면서 저주파 성분의 외삽 능력 향상.
factor=4이면 학습 max_pos의 ~4배까지 외삽 가능.
```

### Qwen2/3 RoPE 파라미터

```
Qwen2-7B: base=1,000,000 (Llama의 10,000보다 100배 큼)
→ 더 넓은 주파수 범위 → 더 긴 컨텍스트 외삽 능력
→ max_position_embeddings = 131,072 (128K 컨텍스트)
```

---

## 5. GQA (Grouped Query Attention)

파일: `mini-vllm-study/minivllm/models/layers/attention.py:273-309`

### 기본 개념

```
MHA (Multi-Head Attention): num_kv_heads == num_heads
MQA (Multi-Query Attention): num_kv_heads == 1
GQA (Grouped Query Attention): 1 < num_kv_heads < num_heads

Qwen2-7B 예시:
  num_heads    = 28
  num_kv_heads = 4
  그룹 크기    = 28 / 4 = 7  (7개 Q 헤드가 1개 KV 헤드 공유)
```

```
ASCII 다이어그램 (num_heads=8, num_kv_heads=2):

Q:  [h0 h1 h2 h3 h4 h5 h6 h7]   (8 Q 헤드)
K:  [k0        k1       ]         (2 KV 헤드)
V:  [v0        v1       ]

그룹 0: h0 h1 h2 h3 → k0, v0
그룹 1: h4 h5 h6 h7 → k1, v1
```

### KV 캐시 메모리 절감 효과

```
MHA: KV 캐시 크기 = 2 * num_heads * head_dim * max_seq * num_layers
GQA: KV 캐시 크기 = 2 * num_kv_heads * head_dim * max_seq * num_layers

Qwen2-7B 비율: 4/28 ≈ 14%  (86% 절감)
```

### FlashAttention의 GQA 내부 처리

CUDA FlashAttention(`flash_attn_varlen_func`, `flash_attn_with_kvcache`)은 GQA를 네이티브 지원한다. 함수 내부에서 `num_heads != num_kv_heads`를 감지하여 자동으로 KV 헤드를 반복(repeat) 처리하므로 외부에서 `repeat_interleave`를 호출할 필요가 없다.

NPU FlashAttention은 GQA를 **지원하지 않아** `num_heads == num_kv_heads`일 때만 NPU FA 경로를 사용:

```python
# mini-vllm-study/minivllm/models/layers/attention.py:529
_npu_fa_safe = self.num_heads == self.num_kv_heads  # GQA 모델에서 False
```

---

## 6. FlashAttention — Prefill 경로 (CUDA)

파일: `mini-vllm-study/minivllm/models/layers/attention.py:733-788`

### flash_attn_varlen_func

```python
attn_out = flash_attn_varlen_func(
    q,                          # shape: (total_tokens, num_heads, head_dim)
    k,                          # shape: (total_tokens, num_kv_heads, head_dim)
    v,                          # shape: (total_tokens, num_kv_heads, head_dim)
    cu_seqlens_q=cu_seqlens_q,  # 누적 시퀀스 길이: [0, L0, L0+L1, ...]  int32
    cu_seqlens_k=cu_seqlens_k,  # K의 누적 시퀀스 길이 (프리픽스 캐시 시 다를 수 있음)
    max_seqlen_q=max_seqlen_q,  # 배치 내 Q 최대 길이 (커널 최적화 힌트)
    max_seqlen_k=max_seqlen_k,  # 배치 내 K 최대 길이
    softmax_scale=self.scale,   # 1/sqrt(head_dim)
    causal=True,                # 인과적 마스크: 미래 토큰 attend 금지
    block_table=block_tables,   # 프리픽스 캐시 히트 시 블록 테이블
)
# 출력: shape (total_tokens, num_heads, head_dim)
```

### cu_seqlens (누적 시퀀스 길이)

패딩 없이 여러 시퀀스를 하나의 텐서에 연결(packing)하여 처리:

```
배치: [시퀀스A(5토큰), 시퀀스B(3토큰), 시퀀스C(7토큰)]
q 텐서: shape (15, num_heads, head_dim)  ← 연결됨, 패딩 없음

cu_seqlens_q = [0, 5, 8, 15]  (int32)
               ↑  ↑  ↑  ↑
               |  A끝 B끝 C끝
               시작

max_seqlen_q = 7  (가장 긴 시퀀스 길이)
```

### 프리픽스 캐시 히트 시 K/V 교체

```python
# mini-vllm-study/minivllm/models/layers/attention.py:740-741
if context.block_tables is not None:
    k, v = k_cache, v_cache  # K/V를 전체 캐시 텐서로 교체
# 이 경우 cu_seqlens_k가 q와 달라질 수 있음
```

---

## 7. FlashAttention — Decode 경로 (CUDA)

파일: `mini-vllm-study/minivllm/models/layers/attention.py:793-815`

### flash_attn_with_kvcache

```python
attn_out = flash_attn_with_kvcache(
    q.unsqueeze(1),             # (N, H, D) → (N, 1, H, D): 시퀀스 길이 차원 추가
    k_cache,                    # (num_blocks, block_size, num_kv_heads, head_dim)
    v_cache,                    # (num_blocks, block_size, num_kv_heads, head_dim)
    cache_seqlens=context.context_lens,  # (N,): 각 시퀀스의 현재 KV 캐시 길이
    block_table=context.block_tables,    # (N, max_blocks): 시퀀스별 블록 ID 목록
    softmax_scale=self.scale,
    causal=True,
)
# 출력: shape (N, 1, num_heads, head_dim) → squeeze(1) → (N, num_heads, head_dim)
```

### block_table (PagedAttention 블록 참조)

```
block_table: shape (num_seqs, max_blocks_per_seq)

예시 (block_size=64):
시퀀스0 (200 토큰, 4블록): [15, 23, 7, 42]
시퀀스1 (80  토큰, 2블록): [8,  31, -1, -1]  ← -1은 미사용

cache_seqlens: [200, 80]  ← 각 시퀀스의 실제 캐시 길이
```

FlashAttention이 내부적으로 `block_table`을 따라 KV 캐시 물리 블록에서 K/V를 읽어와 어텐션 계산.

---

## 8. NPU FlashAttention

파일: `mini-vllm-study/minivllm/models/layers/npu_flash_attention.py`
파일: `mini-vllm-study/minivllm/models/layers/attention_backend.py`

### BNSD vs BSND 레이아웃

```
CUDA FlashAttention 입력: BSND (Batch, Seq, Num_heads, Dim_head)
NPU FlashAttention 입력:  BNSD (Batch, Num_heads, Seq, Dim_head)

변환:
  BSND → BNSD: tensor.transpose(1, 2).contiguous()
  BNSD → BSND: tensor.transpose(1, 2).contiguous()
```

### NPU 백엔드 우선순위

```
Priority 1: NPU 통합 추론 API (npu_fused_infer_attention_score)
  - Prefill/Decode 단일 함수 처리
  - GQA 미지원 (num_heads != num_kv_heads이면 건너뜀)
  - CUDA Graph warmup 패스 건너뜀 (block_tables 모두 -1이면)

Priority 2a: NPU Prefill (npu_fusion_attention)
  - BSND 레이아웃 입력
  - causal 마스크: triu(diagonal=1) 상삼각 True 마스크
  - 패킹된 배치를 패딩된 4D 텐서로 변환 후 처리

Priority 2b: NPU Decode (npu_incre_flash_attention)
  - BNSD 레이아웃 (q.unsqueeze(2))
  - block_table로 PagedAttention
  - GQA 지원 (num_heads, num_key_value_heads 파라미터)

Fallback: StandardAttentionBackend (PyTorch SDPA)
```

### NPU causal 마스크

파일: `mini-vllm-study/minivllm/models/layers/npu_flash_attention.py:90-133`

```python
# 상삼각 True 마스크: (i, j) where j > i → True (미래 토큰 차단)
mask = torch.triu(
    torch.ones((size, size), dtype=torch.bool, device=device),
    diagonal=1,
)
```

**캐싱 전략**: 2의 거듭제곱 크기로 반올림하여 캐시:

```
요청 크기 3000 → max(3000, 2048) = 3000
(3000 - 1).bit_length() = 12 → 1 << 12 = 4096
→ 4096×4096 마스크 생성 후 캐시
→ 2049~4096 범위의 모든 요청이 이 마스크 재사용
```

캐시 키는 `torch.device` 객체 → NPU 디바이스별 독립 캐시.

### Sparse Mode (NPU FA2)

```
SPARSE_MODE 환경변수: NPU_FA2_SPARSE_MODE (기본값: 3)
  2 = TOP_LEFT_ALIGNED_CAUSAL_MASK_MODE
      표준 causal: 각 쿼리가 자신과 이전 토큰만 attend
  3 = DOWN_RIGHT_ALIGNED_CAUSAL_MASK_MODE
      패킹된 시퀀스에 유리한 정렬 방식 (NPU 기본값)
```

---

## 9. 표준 PyTorch 폴백 어텐션 (_fallback_attention)

파일: `mini-vllm-study/minivllm/models/layers/attention.py:851-910`

CPU, XPU, MPS 등 FlashAttention 미지원 환경의 기본 경로.

### Prefill 경로

```python
# cum_seqlens_q로 배치 내 각 시퀀스를 순차 처리
for i in range(batch_size):
    q_seq = q[cum_seqlens_q[i]:cum_seqlens_q[i+1]]  # (seqlen_q, H, D)
    k_seq = k[cum_seqlens_k[i]:cum_seqlens_k[i+1]]  # (seqlen_k, kv_H, D)
    v_seq = v[cum_seqlens_k[i]:cum_seqlens_k[i+1]]

    k_seq, v_seq = self._repeat_kv_heads(k_seq, v_seq)  # GQA → MHA 변환
    out_seq = self._compute_attention_weights(q_seq, k_seq, v_seq)
    outputs.append(out_seq)

return torch.cat(outputs, dim=0)
```

### Decode 경로

PageAttention 또는 BufferedPageAttention 사용:

```python
# PageAttention: 매 호출마다 gather 버퍼 새로 할당
# BufferedPageAttention: 사전 할당 버퍼 재사용 (고처리량 최적화)
return self._page_attn(q, self.k_cache, self.v_cache, context)
```

### _repeat_kv_heads (GQA → MHA 변환)

FlashAttention 미사용 시 GQA의 KV 헤드를 Q 헤드 수만큼 명시적으로 반복:

```python
# num_kv_heads=4, num_heads=28 → 각 KV 헤드를 7번 반복
groups = num_heads // num_kv_heads  # = 7
k = k.repeat_interleave(groups, dim=2)  # (1, seqlen, kv_H, D) → (1, seqlen, H, D)
v = v.repeat_interleave(groups, dim=2)
```

---

## 10. CUDA Graph와 Attention

### Decode에서 CUDA Graph 적용 가능 이유

Decode 단계에서 각 시퀀스는 정확히 1개 토큰을 입력으로 받는다:

```
Decode Q shape: (batch_size, 1, num_heads, head_dim)
```

배치 크기가 고정되어 있으면 모든 텐서 shape이 정적 → CUDA Graph 캡처 가능.

mini-vLLM은 배치 크기 1, 2, 4, 8, ... `max_num_seqs`까지의 그래프를 미리 캡처:

```python
# mini-vllm-study/minivllm/engine/inference_executor.py
for batch_size in [1, 2, 4, 8, ..., max_num_seqs]:
    # 각 배치 크기별 CUDA Graph 캡처
```

실행 시에는 실제 배치 크기보다 크거나 같은 가장 작은 그래프를 선택하고, 정적 입력 버퍼를 in-place로 갱신한 후 replay.

### Prefill에서 CUDA Graph 불가 이유

프롬프트 길이가 매 요청마다 다름 → 텐서 shape이 가변 → CUDA Graph 캡처 불가. `enforce_eager=True`이면 Decode에서도 그래프 없이 eager 실행.

---

## 11. Attention 백엔드 초기화 흐름

파일: `mini-vllm-study/minivllm/models/layers/attention.py:326-357`

```
Attention.__init__()
  │
  ├─ MINIVLLM_USE_NPU_FA == "1" AND torch_npu 설치됨?
  │     YES → self.backend = NPUAttentionBackend()
  │
  ├─ flash_attn 설치됨 (flash_attn_varlen_func 임포트 성공)?
  │     YES → self.backend = FlashAttentionBackend()
  │
  └─ 그 외:
        self.backend = StandardAttentionBackend()

  ├─ use_buffered_page_attention?
  │     YES → self._page_attn = BufferedPageAttention()
  └─     NO  → self._page_attn = PageAttention()
```

### KV 캐시 텐서는 나중에 등록됨

```python
# __init__ 시점에는 빈 텐서
self.k_cache: torch.Tensor = torch.tensor([])
self.v_cache: torch.Tensor = torch.tensor([])
self._cache_initialized: bool = False
```

`ModelRunner.allocate_kv_cache()` 호출 후 각 레이어별로 할당된 슬라이스가 `attention.k_cache`, `attention.v_cache`에 직접 대입된다.

---

## 12. Attention forward 전체 흐름 요약

```
Attention.forward(q, k, v)
  │
  ├─ context = get_context()   ← contextvars에서 추론 컨텍스트 읽기
  │
  ├─ [KV 캐시 초기화됨 AND slot_mapping 있음]
  │     backend.store_kv_cache(k, v, k_cache, v_cache, slot_mapping)
  │     → 음수 슬롯 건너뜀 (프리픽스 캐시 히트)
  │     → 양수 슬롯에 K/V 기록
  │
  ├─ [NPU 백엔드 AND GQA 아님 AND warmup 아님]
  │   ├─ Priority 1: unified_inference(npu_fused_infer_attention_score)
  │   │     BNSD 변환 → unified API → 4D→3D squeeze
  │   ├─ Priority 2a (is_prefill): npu_fusion_attention
  │   │     packed 3D → padded 4D BNSD → causal mask → 결과 언패킹
  │   └─ Priority 2b (!is_prefill): npu_incre_flash_attention
  │         q.unsqueeze(2) BNSD → block_table PageAttention
  │
  ├─ [CUDA FlashAttention 사용 가능]
  │   ├─ is_prefill:
  │   │     flash_attn_varlen_func(q, k, v, cu_seqlens, max_seqlen, causal=True)
  │   └─ !is_prefill:
  │         flash_attn_with_kvcache(q.unsqueeze(1), k_cache, v_cache,
  │                                  cache_seqlens, block_table)
  │
  └─ [폴백]
        _fallback_attention(q, k, v, context)
          is_prefill: 배치 분리 → _repeat_kv_heads → _compute_attention_weights
          !is_prefill: _page_attn(PageAttention/BufferedPageAttention)
```

---

## 13. 코드 참조 색인

| 컴포넌트 | 파일 | 핵심 행 |
|----------|------|---------|
| Attention 클래스 + forward | `mini-vllm-study/minivllm/models/layers/attention.py` | 212-825 |
| FlashAttention Prefill 경로 | `mini-vllm-study/minivllm/models/layers/attention.py` | 733-788 |
| FlashAttention Decode 경로 | `mini-vllm-study/minivllm/models/layers/attention.py` | 793-815 |
| NPU 통합 추론 경로 | `mini-vllm-study/minivllm/models/layers/attention.py` | 548-603 |
| NPU Prefill/Decode 분기 | `mini-vllm-study/minivllm/models/layers/attention.py` | 613-728 |
| AttentionBackend 추상 클래스 | `mini-vllm-study/minivllm/models/layers/attention_backend.py` | 1-80 |
| store_kvcache_kernel (Triton) | `mini-vllm-study/minivllm/models/layers/attention_backend.py` | - |
| NPU FlashAttention 래퍼 | `mini-vllm-study/minivllm/models/layers/npu_flash_attention.py` | 1-268 |
| NPU causal 마스크 캐싱 | `mini-vllm-study/minivllm/models/layers/npu_flash_attention.py` | 90-133 |
| RoPE 구현 | `mini-vllm-study/minivllm/models/layers/rotary_embedding.py` | 1-426 |
| apply_rotary_emb | `mini-vllm-study/minivllm/models/layers/rotary_embedding.py` | 92-164 |
| RotaryEmbedding.__init__ | `mini-vllm-study/minivllm/models/layers/rotary_embedding.py` | 203-291 |
| RotaryEmbedding.forward | `mini-vllm-study/minivllm/models/layers/rotary_embedding.py` | 308-344 |
