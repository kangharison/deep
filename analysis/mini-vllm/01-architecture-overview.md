# mini-vLLM 전체 아키텍처 개요

관련 문서: [[README]] | [[02-paged-attention-block-manager]]

---

## 1. mini-vLLM이란

mini-vLLM은 vLLM의 핵심 아이디어(PagedAttention, KV 캐시 관리, 2단계 스케줄링, Tensor Parallelism, CUDA Graph)를 약 9,700줄의 읽기 쉬운 Python 코드로 재구현한 경량 LLM 추론 엔진이다. PyTorch + HuggingFace Transformers를 기반으로 하며 Qwen2/3, OPT, GPT2 모델을 지원한다. 성능보다 "코드를 읽으면서 LLM 서빙의 전체 원리를 이해하는 것"을 목적으로 설계되어, 각 서브시스템의 경계가 명확하고 파일 단위로 역할이 분리되어 있다.

---

## 2. 전체 데이터 흐름

사용자 요청이 텍스트 출력으로 변환되는 전체 흐름은 다음과 같다.

```
[사용자]
    │
    │ prompts = ["Hello, world!"]
    │ params  = SamplingParams(temperature=0.7, max_tokens=100)
    ▼
LLM.generate(prompts, params)                   mini-vllm-study/minivllm/llm.py:134
    │  (LLMEngine을 상속하므로 LLMEngine.generate()가 실행됨)
    ▼
LLMEngine.generate()                            mini-vllm-study/minivllm/engine/llm_engine.py:548
    │
    ├─ for prompt in prompts:
    │      add_request(prompt, sp)              → tokenize → Sequence 생성 → waiting 큐 투입
    │
    └─ while not is_finished():
           ┌─ Scheduler.schedule()              mini-vllm-study/minivllm/engine/scheduler.py:344
           │      → Prefill 우선: waiting 큐에서 시퀀스 선발 + KV 블록 할당
           │      → 없으면 Decode: running 큐에서 선발 (블록 부족 시 preempt)
           │      → (sequences, is_prefill) 반환
           │
           ├─ ModelRunner.call("run", seqs, is_prefill)
           │                                    mini-vllm-study/minivllm/engine/model_runner.py
           │      → TP worker 프로세스에 신호 전달 (events[i].set())
           │      → InferenceExecutor.execute_batch()
           │                                    mini-vllm-study/minivllm/engine/inference_executor.py
           │           → 입력 텐서 구성 (slot_mapping, block_tables, cu_seqlens)
           │           → model.forward() [embed → N×(attn+mlp) → lm_head]
           │           → CUDA Graph replay (enforce_eager=False 시)
           │           → Sampler: logits → token_ids
           │      → token_ids 반환
           │
           └─ Scheduler.postprocess(seqs, token_ids)
                  → 각 seq에 토큰 추가, EOS/max_tokens 완료 판정, 블록 해제
    │
    ├─ sorted_outputs = sorted(outputs, by seq_id)   # 입력 순서 복원
    └─ tokenizer.batch_decode(sorted_outputs)         # 토큰 ID → 텍스트
    │
    ▼
[{"text": "...", "token_ids": [...]}]  × len(prompts)
```

핵심 루프 불변 조건: 한 스텝은 Prefill **또는** Decode 중 하나만 실행한다. 혼합 배치는 지원하지 않는다 (`mini-vllm-study/minivllm/engine/scheduler.py:378`).

---

## 3. 시스템 계층 구조

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer 1: 사용자 인터페이스                                        │
│                                                                  │
│    LLM(config)                                                   │
│    └─ generate(prompts, SamplingParams) → list[{"text", ...}]   │
│    mini-vllm-study/minivllm/llm.py                              │
├─────────────────────────────────────────────────────────────────┤
│  Layer 2: 엔진 오케스트레이션                                      │
│                                                                  │
│    LLMEngine          ← 루프 총괄, tokenize, 결과 집계             │
│    Scheduler          ← Prefill/Decode 2단계 스케줄링              │
│    BlockManager       ← KV 캐시 블록 풀 (PagedAttention)          │
│    Sequence           ← 요청 1개의 상태 단위                       │
│    mini-vllm-study/minivllm/engine/                              │
├─────────────────────────────────────────────────────────────────┤
│  Layer 3: 멀티프로세스 실행 조율                                    │
│                                                                  │
│    ModelRunner        ← Rank 0 / worker 분기, IPC 신호            │
│    DistributedManager ← NCCL/HCCL broadcast/all-reduce/gather   │
│    InferenceExecutor  ← KV 캐시 텐서, CUDA Graph, Sampler 호출   │
│    mini-vllm-study/minivllm/engine/                              │
├─────────────────────────────────────────────────────────────────┤
│  Layer 4: 모델 구현                                               │
│                                                                  │
│    QwenBase / Qwen2 / Qwen3   ← Qwen 계열 (공유 기반 클래스)      │
│    OPT / GPT2                 ← OPT, GPT-2                      │
│    ModelManager / registry    ← 모델 로드, 가중치 주입            │
│    mini-vllm-study/minivllm/models/                              │
├─────────────────────────────────────────────────────────────────┤
│  Layer 5: 신경망 레이어                                            │
│                                                                  │
│    Attention + AttentionBackend  ← KV 캐시 저장 + FlashAttention  │
│    ColumnParallelLinear /        ← Tensor Parallelism (TP)       │
│    RowParallelLinear                                             │
│    RotaryEmbedding               ← RoPE 위치 인코딩              │
│    RMSNorm / LayerNorm           ← 정규화                        │
│    SiluAndMul / GELU             ← 활성화 함수                   │
│    VocabParallelEmbedding /      ← 임베딩 + LM 헤드              │
│    ParallelLMHead                                                │
│    mini-vllm-study/minivllm/models/layers/                       │
├─────────────────────────────────────────────────────────────────┤
│  Layer 6: 샘플링                                                  │
│                                                                  │
│    Sampler                   ← logits → 토큰 ID (파이프라인 진입)  │
│    functional                ← penalties/temperature/top-k/p    │
│    SamplingConfig            ← 내부 파라미터 정규화               │
│    Mirostat                  ← 미연결 확장 샘플러                  │
│    mini-vllm-study/minivllm/sampling/                            │
├─────────────────────────────────────────────────────────────────┤
│  Layer 7: 디바이스 추상화 / 유틸                                    │
│                                                                  │
│    device.py     ← CUDA/NPU/XPU/MPS/CPU 감지, 메모리 조회        │
│    context.py    ← contextvars 기반 추론 컨텍스트 (is_prefill 등) │
│    loader.py     ← safetensors 가중치 로드                       │
│    logger_utils  ← rank-aware 로거                              │
│    random_utils  ← 재현 가능 시드                                │
│    mini-vllm-study/minivllm/utils/                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. 핵심 개념 7가지

### 4-1. Prefill / Decode 2단계

LLM 추론은 두 단계로 나뉜다.

```
Prefill (1회):                    Decode (N회):
  입력: 프롬프트 전체 토큰            입력: 직전에 생성된 토큰 1개
  출력: KV 캐시 + 첫 토큰           출력: 다음 토큰 1개
  특성: compute-bound              특성: memory-bound
        (행렬 곱 위주)                    (KV 캐시 로드 위주)
  텐서 형상: 가변 길이 packed         텐서 형상: [batch_size, 1]
  커널: flash_attn_varlen_func     커널: flash_attn_with_kvcache
```

Prefill은 "프롬프트를 이해하는 단계", Decode는 "단어를 한 글자씩 생성하는 단계"로 이해하면 된다. 두 단계는 어텐션 커널, 텐서 형상, KV 캐시 접근 방식이 모두 다르므로 mini-vLLM은 한 스텝에서 둘을 혼합하지 않는다(`mini-vllm-study/minivllm/engine/scheduler.py:378–389`).

### 4-2. KV 캐시 (Key-Value Cache)

Transformer 어텐션에서 각 토큰의 Key/Value 벡터는 이후 모든 토큰 생성에서 재사용된다. KV 캐시는 이 벡터를 GPU 메모리에 보관해 재계산을 피하는 기법이다. Decode 단계의 속도는 KV 캐시에서 데이터를 얼마나 빨리 읽어오느냐(메모리 대역폭)에 달려 있다.

```
레이어 l, 시퀀스 s의 KV 캐시:
  K[l][s] = [k_t0, k_t1, ..., k_t_{len-1}]   shape: (len, num_kv_heads, head_dim)
  V[l][s] = [v_t0, v_t1, ..., v_t_{len-1}]   shape: (len, num_kv_heads, head_dim)
```

추론이 길어질수록 KV 캐시 크기가 선형으로 증가하며, 이것이 LLM 서빙 메모리의 주요 소비원이다.

### 4-3. PagedAttention (블록식 KV 캐시)

기존 방식은 시퀀스 최대 길이에 맞춰 KV 캐시를 연속 공간으로 미리 예약해 내부 단편화(internal fragmentation)가 심했다. PagedAttention은 OS의 페이징에서 영감을 받아 KV 캐시를 **고정 크기 블록**(mini-vLLM 기본 64토큰)으로 쪼개 관리한다.

```
논리 뷰 (시퀀스):           물리 뷰 (KV 캐시 텐서):
  token 0..63   →  block_table[0] = 7  →  kv_cache[:, :, 7, :, :, :]
  token 64..127 →  block_table[1] = 2  →  kv_cache[:, :, 2, :, :, :]
  token 128..   →  block_table[2] = 15 →  kv_cache[:, :,15, :, :, :]

슬롯 공식:
  slot = block_id * block_size + intra_block_offset
         (예: block_id=7, offset=3, block_size=64 → slot=451)
```

블록 단위 관리로 메모리 단편화가 거의 없고, 길이가 다른 시퀀스들을 같은 블록 풀에서 공유할 수 있다. 자세한 구현은 [[02-paged-attention-block-manager]] 참조.

### 4-4. Prefix Caching (xxhash)

여러 요청이 동일한 프롬프트 접두사를 공유하면(예: 시스템 프롬프트), 해당 부분의 KV 계산을 한 번만 하고 블록을 공유한다. mini-vLLM은 블록이 block_size 토큰으로 꽉 찼을 때 xxhash64로 블록 내용의 **체인 해시**를 계산하여 `hash_to_block_id` 딕셔너리에 등록한다.

```
체인 해시 (Chained Hash):
  H0 = xxhash64(초기값=-1 || tokens[0:64])
  H1 = xxhash64(H0_8bytes  || tokens[64:128])
  H2 = xxhash64(H1_8bytes  || tokens[128:192])
  ...

→ 동일 토큰이라도 시퀀스 내 위치가 다르면 해시가 달라져 오염 없이 독립 캐시됨
```

할당 시 캐시 히트하면 `ref_count++`로 블록을 공유하고, 해당 블록의 토큰 수 만큼 `num_cached_tokens`를 올려 Prefill 연산을 건너뛴다. 구현: `mini-vllm-study/minivllm/engine/block_manager.py:514`.

### 4-5. CUDA Graph

Decode 단계는 매 스텝 같은 연산을 반복한다. GPU 커널을 매번 CPU에서 런칭하면 커널 발사(launch) 오버헤드가 누적된다. CUDA Graph는 일련의 GPU 연산을 **정적 그래프**로 한 번 녹화(capture)해두고, 이후 스텝에서는 graph.replay()로 재실행해 CPU 오버헤드를 제거한다.

```
사전 캡처 (배치 크기별):
  [batch=1]: capture → graph_1
  [batch=2]: capture → graph_2
  [batch=4]: capture → graph_4
  ...

실행 시 (Decode):
  target_batch = 입력 시퀀스 수에 맞는 최소 그래프 배치 크기
  graph_vars["input_ids"][:target_batch].copy_(actual_input)  # in-place 업데이트
  graphs[target_batch].replay()                               # 그래프 재실행
```

`enforce_eager=True`로 비활성화하면 매 스텝 일반 eager 실행을 한다(디버깅·NPU용). 구현: `mini-vllm-study/minivllm/engine/inference_executor.py`.

### 4-6. Tensor Parallelism (텐서 병렬)

모델이 너무 커서 단일 GPU에 올릴 수 없을 때, 가중치 행렬을 여러 GPU에 분산한다. mini-vLLM은 열/행 방향 분할을 사용한다.

```
ColumnParallelLinear (열 방향 분할):
  원본 W: [in, out]  → GPU 0: W[:, :out/2], GPU 1: W[:, out/2:]
  각 GPU가 독립 연산 후 출력을 그대로 사용 (all-gather는 선택적)

RowParallelLinear (행 방향 분할, all-reduce 필요):
  원본 W: [in, out]  → GPU 0: W[:in/2, :], GPU 1: W[in/2:, :]
  입력을 나눠 연산 후 all-reduce로 결과를 합산

TP=4인 MLP 예시 (SwiGLU):
  gate/up_proj: ColumnParallel [hidden, intermediate]  → 각 GPU: [hidden, intermediate/4]
  down_proj:    RowParallel [intermediate, hidden]     → 각 GPU: [intermediate/4, hidden]
                                                          → all-reduce → [hidden]
```

TP 통신 백엔드: CUDA→NCCL, NPU→HCCL, CPU/XPU/MPS→Gloo. 구현: `mini-vllm-study/minivllm/models/layers/linear.py`, `mini-vllm-study/minivllm/engine/distributed_manager.py`.

### 4-7. FlashAttention

표준 어텐션은 N×N의 어텐션 행렬 전체를 VRAM에 저장하므로 시퀀스 길이에 대해 O(N²) 메모리를 쓴다. FlashAttention은 어텐션 행렬을 블록 단위로 나눠 계산하고 intermediate 결과를 VRAM 대신 SRAM(레지스터)에 누적해 메모리 대역폭을 크게 절약한다.

```
Prefill 커널: flash_attn_varlen_func
  입력: query/key/value (가변 길이 packed sequence)
        cu_seqlens_q, cu_seqlens_k (시퀀스 경계 오프셋)
  특징: 새 K/V를 KV 캐시에 저장하면서 동시에 어텐션 계산

Decode 커널: flash_attn_with_kvcache
  입력: query (배치×1×헤드×dim)
        kv_cache (사전 계산된 K/V 텐서 전체)
        block_table (물리 블록 매핑)
  특징: 기존 KV 캐시를 조회해 PagedAttention과 통합됨
```

구현: `mini-vllm-study/minivllm/models/layers/attention_backend.py`.

---

## 5. 파일별 역할 표

### engine/ (8개 파일)

| 파일 | 역할 |
|------|------|
| `engine/llm_engine.py` | 전체 추론 루프 오케스트레이터 — add_request, step, generate |
| `engine/scheduler.py` | Prefill/Decode 2단계 스케줄러 — 큐 관리, 선점(preempt) |
| `engine/block_manager.py` | KV 캐시 블록 풀 — PagedAttention, Prefix Caching, CoW |
| `engine/sequence.py` | 요청 1개의 상태 단위 — token_ids, block_table, status |
| `engine/inference_executor.py` | GPU 실행 엔진 — KV 캐시 텐서, CUDA Graph, Sampler 호출 |
| `engine/model_runner.py` | Rank 0 / worker 분기 — IPC 신호, forward 총괄 |
| `engine/distributed_manager.py` | TP 집합 통신 — broadcast, all-reduce, all-gather |
| `engine/__init__.py` | 패키지 공개 API 재수출 |

### models/ (8개 파일)

| 파일 | 역할 |
|------|------|
| `models/qwen_base.py` | Qwen2/3 공통 기반 클래스 — Transformer 블록, RMSNorm, RoPE |
| `models/qwen2.py` | Qwen2 전용 설정 오버라이드 |
| `models/qwen3.py` | Qwen3 전용 설정 오버라이드 (QK-Norm 추가) |
| `models/opt.py` | OPT 모델 구현 — Post-LN, LearnedPositionalEmbedding |
| `models/gpt2.py` | GPT-2 모델 구현 — Conv1D 가중치 형태, GELU |
| `models/manager.py` | 모델 로드·가중치 주입·TP 분배 — ModelManager |
| `models/registry.py` | 모델명→클래스 매핑 레지스트리 |
| `models/__init__.py` | 패키지 공개 API |

### models/layers/ (11개 파일)

| 파일 | 역할 |
|------|------|
| `layers/attention.py` | Attention 레이어 — KV 캐시 저장 + FlashAttention 위임 |
| `layers/attention_backend.py` | FlashAttention 커널 래퍼 — Prefill(varlen) vs Decode(kvcache) |
| `layers/attention_gather.py` | TP 환경 어텐션 출력 gather 처리 |
| `layers/linear.py` | ColumnParallelLinear / RowParallelLinear / Linear |
| `layers/rotary_embedding.py` | RoPE (Rotary Position Embedding) — CUDA/NPU 커널 분기 |
| `layers/embed_head.py` | VocabParallelEmbedding / ParallelLMHead |
| `layers/layernorm.py` | RMSNorm / LayerNorm — NPU 융합 커널 분기 |
| `layers/page_attention.py` | Paged Attention 유틸리티 (슬롯 계산 보조) |
| `layers/activation.py` | SiluAndMul (SwiGLU), GELU, ReLU |
| `layers/npu_flash_attention.py` | NPU 전용 FlashAttention 커널 래퍼 |
| `layers/__init__.py` | 레이어 공개 API |

### sampling/ (6개 파일)

| 파일 | 역할 |
|------|------|
| `sampling/sampler.py` | Sampler — logits → 토큰 ID (파이프라인 진입점) |
| `sampling/functional.py` | penalties/temperature/top-k/top-p/min-p/multinomial |
| `sampling/config.py` | SamplingConfig — 내부 파라미터 정규화 |
| `sampling/mirostat.py` | Mirostat 알고리즘 (확장 샘플러, 현재 미연결) |
| `sampling/__init__.py` | 패키지 공개 API |
| `sampling_params.py` | 사용자 노출 SamplingParams 데이터클래스 |

### utils/ (7개 파일)

| 파일 | 역할 |
|------|------|
| `utils/device.py` | CUDA/NPU/XPU/MPS/CPU 감지 — 메모리 조회, DeviceGraph |
| `utils/context.py` | contextvars 기반 추론 컨텍스트 — is_prefill, slot_mapping 등 |
| `utils/loader.py` | safetensors 가중치 로드 — 샤드 병합, TP 분배 |
| `utils/logger_utils.py` | rank-aware Python 표준 로거 팩토리 |
| `utils/random_utils.py` | 재현 가능 시드 설정 (CUDA/NPU/CPU) |
| `utils/example_utils.py` | 예제 스크립트 공용 유틸 |
| `utils/__init__.py` | 패키지 공개 API |

### 루트 (4개 파일)

| 파일 | 역할 |
|------|------|
| `llm.py` | LLM 사용자 인터페이스 클래스 (LLMEngine 상속, thin wrapper) |
| `config.py` | Config 데이터클래스 — 엔진 전역 설정, HF config 자동 로드 |
| `sampling_params.py` | SamplingParams 사용자 노출 파라미터 |
| `__init__.py` | 패키지 공개 API (LLM, Config, SamplingParams) |

---

## 6. 멀티프로세스 / 텐서 병렬 모델 상세

TP > 1인 경우 Rank 0 메인 프로세스가 Rank 1..N-1 worker 프로세스를 spawn하고, 매 스텝 mp.Event 기반 신호로 동기화한다.

```
Rank 0 (메인 프로세스)                     Rank 1..N-1 (worker)
─────────────────────────────────────      ──────────────────────────────────
LLMEngine.step()                           ModelRunner.loop()  ← 무한 대기
  scheduler.schedule()                       while True:
  model_runner.call("run", seqs, ...)          event.wait()
    broadcast_data(seqs, is_prefill)             broadcast_data() 수신
    event[i].set()  ──────────────────────→      (공유메모리 + mp.Event)
    model.forward()                              model.forward()
      ColumnParallelLinear: 독립 연산              ColumnParallelLinear: 독립 연산
      RowParallelLinear: all-reduce ←──────────→ RowParallelLinear: all-reduce
    synchronize()                                synchronize()
    Sampler(logits) → token_ids                  (샘플링은 Rank 0만)
    return token_ids                             event.clear()
  scheduler.postprocess()
```

핵심 설계:
- Rank 0만 스케줄링·샘플링을 담당하고, forward 결과를 worker와 합산한다
- spawn 방식(fork 아님): CUDA는 fork 후 자식에서 재초기화 불가능하므로 spawn 필수 (`mini-vllm-study/minivllm/engine/llm_engine.py:207`)
- MASTER_PORT 자동 탐지: `socket.bind(("127.0.0.1", 0))`로 OS에서 빈 포트 배정 (`mini-vllm-study/minivllm/engine/llm_engine.py:194`)

---

## 7. 핵심 자료구조 4각형

mini-vLLM의 데이터 흐름을 이해하는 가장 중요한 연결고리는 다음 4개 자료구조다.

```
Sequence (논리 상태)          Block (블록 메타)
  seq_id                        block_id  ──┐
  status: WAITING/RUNNING/       ref_count   │ 인덱스 동일
          FINISHED              hash        │
  token_ids[]                  token_ids[] │
  num_prompt_tokens                        │
  num_cached_tokens            kv_cache 텐서 (물리 메모리)
  block_table[]: [7, 2, 15] ──→ kv_cache[:, :, 7, ...]   ← K/V 값 실제 저장
                                kv_cache[:, :, 2, ...]
                                kv_cache[:, :,15, ...]

Context (contextvars)
  is_prefill: bool
  slot_mapping: [7*64+0, 7*64+1, ..., 15*64+3]  ← KV 쓸 위치
  block_tables: [[7, 2, 15], ...]               ← Decode 시 K/V 읽을 위치
  cu_seqlens_q / cu_seqlens_k                   ← Prefill 시 시퀀스 경계
```

`Sequence.block_table` → `Block.block_id` → `kv_cache` 인덱스 → `Context.slot_mapping` 이 네 개가 "KV 캐시의 어디에 이 토큰의 K/V를 쓰고 읽을지"를 결정하는 핵심 체인이다. 상세 해설은 [[02-paged-attention-block-manager]] 참조.

---

## 관련 문서

- [[02-paged-attention-block-manager]] — PagedAttention 및 BlockManager 심층 분석
- [[README]] — 문서 인덱스 및 학습 권장 순서
