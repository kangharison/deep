# 05 — Tensor Parallelism + 멀티프로세스 IPC 심층 분석

관련 문서: [[01-overview]] [[02-paged-attention-block-manager]] [[03-scheduler]] [[04-inference-executor]] [[06-attention-flashattention]]

---

## 1. Tensor Parallelism 기본 원리

Tensor Parallelism(TP)은 단일 GPU 메모리로 담기 어려운 대형 LLM 가중치를 여러 GPU에 분산하는 기법이다. 모델의 Linear 레이어(행렬 곱)를 가중치의 **열 방향(출력 차원)** 또는 **행 방향(입력 차원)**으로 잘라 각 GPU에 할당하고, 집합 통신(collective communication)으로 결과를 합산한다.

### Column Parallel (출력 차원 분할)

```
W: (hidden=4096, ffn=16384)   →   N=4 GPU로 열 방향 분할
GPU0: W[:, 0:4096]
GPU1: W[:, 4096:8192]
GPU2: W[:, 8192:12288]
GPU3: W[:, 12288:16384]

입력 x: (batch, 4096) — 모든 GPU에 동일하게 복제됨
각 GPU 출력: (batch, 4096)  ← 자신의 열 샤드만 계산
```

- 모든 GPU가 동일한 입력 x를 받아 자신의 열 샤드에 대한 행렬곱을 계산
- 결과는 각 GPU가 전체 출력의 1/N 부분만 가짐
- 다음 RowParallel 레이어에 직접 연결하면 중간 통신을 생략할 수 있음

### Row Parallel (입력 차원 분할 + all-reduce)

```
W: (hidden=4096, ffn=16384)   →   N=4 GPU로 행 방향 분할
GPU0: W[0:1024, :]
GPU1: W[1024:2048, :]
GPU2: W[2048:3072, :]
GPU3: W[3072:4096, :]

각 GPU 입력: (batch, 1024)  ← ColumnParallel 출력 샤드
각 GPU 부분합: (batch, 16384)
all-reduce(SUM): sum across GPUs → (batch, 16384)  ← 전체 올바른 결과
```

- 각 GPU의 입력이 전체 입력의 1/N 샤드이고, 가중치도 그에 맞게 잘려 있음
- 각 GPU에서 부분 합산(partial sum)을 계산한 후 all-reduce로 합침
- all-reduce 완료 후 모든 GPU가 동일한 전체 결과를 가짐

### 표준 MLP 블록에서의 TP 통신 흐름

```
x (공유 입력)
    ↓
ColumnParallelLinear(gate_proj)    ← 열 분할, 입력 공유, 통신 없음
    ↓
ColumnParallelLinear(up_proj)      ← 열 분할, 입력 공유, 통신 없음
    ↓
activation(SiLU/SwiGLU)           ← 각 GPU 독립 실행
    ↓
RowParallelLinear(down_proj)       ← 행 분할, all-reduce로 결과 합산
    ↓
y (합산된 전체 출력, 모든 GPU 동일)
```

---

## 2. mini-vLLM의 Linear 레이어 종류

파일: `mini-vllm-study/minivllm/models/layers/linear.py`

### 클래스 계층 구조

```
LinearBase (nn.Module)
├── ReplicatedLinear          — 모든 GPU 동일 가중치, 통신 없음
├── ColumnParallelLinear      — 출력 차원 분할
│   ├── MergedColumnParallelLinear  — gate+up 등 복수 출력 병합
│   └── QKVParallelLinear     — Q/K/V 통합 Column Parallel
└── RowParallelLinear         — 입력 차원 분할 + all-reduce
```

### ColumnParallelLinear

- `output_size`를 `tp_size`로 나눠 각 GPU가 `output_size/tp_size` 출력 행 담당
- `tp_dim=0`: `weight_loader`에서 `loaded_weight.narrow(dim=0, start=tp_rank*shard_size, length=shard_size)`로 자신의 슬라이스만 로드
- `forward()`: `F.linear(x, self.weight, self.bias)` — 통신 없음, 결과는 부분 출력 샤드

```python
# mini-vllm-study/minivllm/models/layers/linear.py:303-305
tp_size = get_tensor_parallel_world_size()
super().__init__(input_size, divide(output_size, tp_size), bias, 0)
# divide(): output_size가 tp_size로 나누어떨어지는지 검증
```

### RowParallelLinear

- `input_size`를 `tp_size`로 나눠 각 GPU가 `input_size/tp_size` 입력 피처 처리
- `tp_dim=1`: `weight_loader`에서 입력 축(dim=1)으로 슬라이싱
- `forward()`: 부분 합산 → `dist.all_reduce(y)` → 모든 GPU에 전체 결과

```python
# mini-vllm-study/minivllm/models/layers/linear.py:621-632
def forward(self, x: torch.Tensor) -> torch.Tensor:
    y = F.linear(x, self.weight, self.bias if self.tp_rank == 0 else None)
    # bias는 rank 0에만 로드했으므로 rank 0만 더함 (중복 덧셈 방지)
    if self.tp_size > 1:
        dist.all_reduce(y)  # NCCL/HCCL SUM 집합 통신
    return y
```

### QKVParallelLinear

GQA(Grouped Query Attention) 지원을 위해 Q/K/V 헤드 수가 다를 수 있음:

```
출력 레이아웃: [Q 블록 | K 블록 | V 블록]
Q 블록: [0,              num_heads * head_size)
K 블록: [num_heads * head_size,  (num_heads + num_kv_heads) * head_size)
V 블록: [(num_heads + num_kv_heads) * head_size,  (num_heads + 2*num_kv_heads) * head_size)

각 블록에서 tp_rank번째 청크만 로드:
loaded_weight.chunk(tp_size, dim=0)[tp_rank]
```

`weight_loader`는 `loaded_shard_id='q'|'k'|'v'`로 어느 프로젝션인지 지정받아 올바른 오프셋에 복사.

### MergedColumnParallelLinear

SwiGLU MLP의 `gate_proj`와 `up_proj`를 하나의 행렬로 병합:

```python
# mini-vllm-study/minivllm/models/layers/linear.py:404-415
shard_offset = sum(self.output_sizes[:loaded_shard_id]) // self.tp_size
shard_size   = self.output_sizes[loaded_shard_id] // self.tp_size
param_data   = param_data.narrow(self.tp_dim, shard_offset, shard_size)
loaded_weight = loaded_weight.chunk(self.tp_size, self.tp_dim)[self.tp_rank]
param_data.copy_(loaded_weight)
```

### VocabParallelEmbedding / ParallelLMHead

`mini-vllm-study/minivllm/models/layers/embed_head.py`에 구현:
- `VocabParallelEmbedding`: vocab 차원을 tp_size로 분할
- `ParallelLMHead`: 최종 logit 출력 분할 → all-gather로 전체 vocab logit 집결

---

## 3. Attention에서의 TP 분할

파일: `mini-vllm-study/minivllm/models/qwen_base.py`

```
Q 헤드: num_heads / tp_size 개씩 각 GPU
K 헤드: num_kv_heads / tp_size 개씩 각 GPU  (GQA)
V 헤드: num_kv_heads / tp_size 개씩 각 GPU  (GQA)
output proj: RowParallelLinear → all-reduce
```

### GQA에서 TP 분할

```
Qwen2-7B: num_heads=28, num_kv_heads=4, tp_size=4

GPU0: Q[0:7],   K[0:1],   V[0:1]
GPU1: Q[7:14],  K[1:2],   V[1:2]
GPU2: Q[14:21], K[2:3],   V[2:3]
GPU3: Q[21:28], K[3:4],   V[3:4]
```

`num_kv_heads=4, tp_size=4` → 각 GPU가 kv_head 1개 담당. `num_kv_heads`가 `tp_size`로 정확히 나누어떨어져야 함.

FlashAttention은 GQA를 내부적으로 처리하므로 외부에서 `repeat_interleave` 불필요.

---

## 4. 프로세스 시작 흐름

파일: `mini-vllm-study/minivllm/engine/llm_engine.py:124-264`

```
LLMEngine.__init__(config)
  │
  ├─ [TP > 1] 빈 포트 자동 탐지
  │     sock.bind(("127.0.0.1", 0))  → OS가 빈 포트 배정
  │     os.environ["MASTER_PORT"] = str(port)
  │
  ├─ ctx = mp.get_context("spawn")
  │   CUDA는 fork 후 GPU 상태 손상 가능 → spawn으로 깨끗한 Python 인터프리터 시작
  │
  ├─ for i in range(1, tensor_parallel_size):
  │     event = ctx.Event()          # 공유 메모리 IPC 이벤트
  │     p = ctx.Process(
  │           target=ModelRunner,    # 클래스 = callable → __init__이 자식에서 실행
  │           args=(config, i, event)
  │         )
  │     p.start()
  │     self.ps.append(p)
  │     self.events.append(event)
  │
  └─ ModelRunner(config, rank=0, self.events)
       # Rank 0은 메인 프로세스에서 직접 생성
       # 무한 루프 없이 __init__ 완료 후 call() 대기
```

### spawn 방식을 쓰는 이유

`fork`는 부모 프로세스의 메모리를 복사하는데, CUDA context는 이미 초기화된 상태이고 이를 자식 프로세스에서 재사용하면 undefined behavior가 발생한다. `spawn`은 새 Python 인터프리터를 시작하므로 CUDA 초기화가 자식 프로세스에서 독립적으로 이루어진다.

---

## 5. ModelRunner 초기화 및 worker 루프

파일: `mini-vllm-study/minivllm/engine/model_runner.py`

### Rank에 따른 실행 경로 분기

```
ModelRunner.__init__(config, rank, event)
  │
  ├─ 환경변수 설정: RANK, LOCAL_RANK, WORLD_SIZE
  │
  ├─ ModelManager.initialize()        ← 모델 가중치 로드 + TP 분할
  ├─ DistributedManager.initialize()  ← NCCL/HCCL 프로세스 그룹 초기화
  ├─ InferenceExecutor()              ← KV 캐시 할당
  ├─ [rank==0] capture_device_graphs() ← CUDA Graph 캡처
  │
  ├─ [world_size > 1] distributed_manager.synchronize()  ← 모든 rank barrier
  │
  └─ [rank != 0] _worker_loop()  ← Rank>0만 진입, 여기서 블록됨
```

**핵심**: `mp.Process(target=ModelRunner, args=(config, i, event))`에서 `target`에 클래스를 전달하면, 자식 프로세스에서 `ModelRunner(config, i, event)`가 호출된다. 즉 `__init__`이 자식 프로세스의 메인 루틴이 된다.

### Rank 0 vs Rank>0 역할

| 항목 | Rank 0 | Rank>0 |
|------|--------|--------|
| `__init__` 완료 후 | `call()` 대기 (LLMEngine이 제어) | `_worker_loop()`에서 무한 대기 |
| 스케줄링 | 담당 | 담당 안 함 |
| 샘플링 | 담당 (next_tokens 반환) | 담당 안 함 (None 반환) |
| forward 실행 | 동일하게 실행 | 동일하게 실행 |
| CUDA Graph 캡처 | 수행 | 수행 안 함 |

### _worker_loop() 구조

파일: `mini-vllm-study/minivllm/engine/model_runner.py:298-408`

```python
def _worker_loop(self) -> None:
    while True:
        # Rank 0이 broadcast_data(cmd)로 보낸 명령을 수신
        cmd = self.distributed_manager.broadcast_data(None, src=0)

        if not isinstance(cmd, list | tuple) or len(cmd) != 3:
            continue  # 잘못된 형식 → 무시하고 다음 명령 대기

        method_name, args, kwargs = cmd

        if method_name == "exit":
            break  # 루프 탈출 → finally에서 exit() 호출

        method = getattr(self, method_name)
        method(*args, **kwargs)  # 로컬 실행, 반환값 버림
```

---

## 6. `call()` → `broadcast_data()` 프로토콜

파일: `mini-vllm-study/minivllm/engine/model_runner.py:410-475`

```
LLMEngine.step()
  ↓
ModelRunner.call("run", sequences, is_prefill)   [Rank 0]
  │
  ├─ [world_size > 1] broadcast_data(("run", (sequences, is_prefill), {}))
  │     → Rank>0 worker들이 수신 → getattr(self, "run")(sequences, is_prefill)
  │
  └─ self.run(sequences, is_prefill)   [Rank 0 로컬 실행]
         → next_tokens 반환           [Rank 0만]

Rank>0 _worker_loop():
  cmd = broadcast_data(None, src=0)   [수신 대기]
  method = getattr(self, cmd[0])      ["run" 메서드 조회]
  method(*cmd[1], **cmd[2])           [로컬 실행]
  return None                         [반환값 버림]
```

**반환값이 Rank 0만 의미 있는 이유**: RowParallelLinear의 all-reduce 덕분에 모든 GPU의 forward 결과가 동일하지만, 샘플링(Sampler)은 Rank 0에서만 수행하고 그 결과를 LLMEngine에 반환한다.

---

## 7. `broadcast_data()` 5단계 IPC 프로토콜

파일: `mini-vllm-study/minivllm/engine/distributed_manager.py:400-528`

```
┌─────────────────────────────────────────────────────────────────────┐
│ 단계 1: 직렬화 (Rank 0만)                                             │
│   data_bytes = pickle.dumps(data)                                    │
│   data_size  = torch.tensor([len(data_bytes)], dtype=torch.long)     │
│   Rank>0: data_size = torch.tensor([0])  ← 더미                      │
├─────────────────────────────────────────────────────────────────────┤
│ 단계 2: 크기 브로드캐스트 (전체)                                         │
│   data_size = data_size.cuda()  (NCCL 요구사항)                       │
│   dist.broadcast(data_size, src=0)                                   │
│   → 모든 rank에서 data_size == len(data_bytes)                        │
├─────────────────────────────────────────────────────────────────────┤
│ 단계 3: 버퍼 준비                                                       │
│   Rank 0:  data_tensor = frombuffer(data_bytes, uint8).clone()       │
│            .clone() 이유: frombuffer는 원본 메모리 공유 → GC 위험        │
│   Rank>0:  data_tensor = torch.empty(data_bytes_len, dtype=uint8)    │
├─────────────────────────────────────────────────────────────────────┤
│ 단계 4: 데이터 브로드캐스트 (전체)                                        │
│   data_tensor = data_tensor.cuda()                                   │
│   dist.broadcast(data_tensor, src=0)                                 │
│   → 모든 rank의 data_tensor가 동일한 바이트열 보유                        │
├─────────────────────────────────────────────────────────────────────┤
│ 단계 5: 역직렬화 (Rank>0만)                                              │
│   data = pickle.loads(data_tensor.cpu().numpy().tobytes())           │
│   .cpu()   : GPU/NPU → CPU (pickle.loads는 CPU 메모리 필요)             │
│   .numpy() : Tensor → ndarray (가능하면 zero-copy)                    │
│   .tobytes(): ndarray → bytes                                        │
│   pickle.loads(): bytes → 파이썬 객체 복원                               │
└─────────────────────────────────────────────────────────────────────┘
```

### 크기를 먼저 브로드캐스트하는 이유

`dist.broadcast(tensor, src=0)`는 **모든 rank가 동일한 shape/dtype의 텐서를 미리 할당**해야 한다. 직렬화된 데이터의 크기가 매 스텝마다 다르므로, 먼저 크기(1개 정수 텐서)를 공유한 후 수신측이 `torch.empty(size)` 버퍼를 할당한다.

### `.clone()`이 필요한 이유

```python
# mini-vllm-study/minivllm/engine/distributed_manager.py:493-501
data_tensor = torch.frombuffer(data_bytes, dtype=torch.uint8).clone()
```

`torch.frombuffer(data_bytes, ...)`는 `data_bytes` 파이썬 bytes 객체의 메모리를 직접 참조하는 zero-copy 뷰를 생성한다. `broadcast()` 호출 중에 `data_bytes`가 GC에 의해 해제되면 텐서가 댕글링 포인터가 된다. `.clone()`으로 독립된 메모리 복사본을 만들어 이 위험을 제거한다.

---

## 8. torch.distributed 백엔드

파일: `mini-vllm-study/minivllm/engine/distributed_manager.py:187-248`
파일: `mini-vllm-study/minivllm/utils/device.py`

| 백엔드 | 하드웨어 | 특징 |
|--------|----------|------|
| NCCL | NVIDIA CUDA GPU | GPU-to-GPU 직접 통신 (NVLink/PCIe), 가장 고성능 |
| HCCL | 화웨이 Ascend NPU | Ascend 전용, 포트 16666 사용 |
| CCL | Intel XPU | Intel GPU 전용 |
| Gloo | CPU/범용 | CPU 텐서 통신, 테스트/개발 환경 폴백 |

백엔드 선택 로직:

```python
# mini-vllm-study/minivllm/utils/device.py
def get_distributed_backend() -> str:
    # 환경변수 MINIVLLM_TP_BACKEND 우선
    # 없으면 현재 디바이스 타입으로 자동 결정:
    #   CUDA → "nccl"
    #   NPU  → "hccl"
    #   XPU  → "ccl"
    #   기타 → "gloo"
```

`_setup_distributed_backend()` 에서 init_process_group 전에 반드시 디바이스를 먼저 설정해야 함:

```python
if self.backend == "hccl":
    torch.npu.set_device(self.rank)   # NPU rank 번호 설정
elif self.backend == "nccl":
    torch.cuda.set_device(self.rank)  # GPU rank 번호 설정
dist.init_process_group(backend=self.backend, rank=self.rank, world_size=self.world_size)
```

### 분산 백엔드 검증

초기화 후 all-reduce 통신 테스트로 정상 동작 확인:

```python
# mini-vllm-study/minivllm/engine/distributed_manager.py:299-329
tensor = torch.ones(1) * self.rank      # 각 rank가 자신의 번호를 값으로
tensor = tensor.cuda()
dist.all_reduce(tensor, op=dist.ReduceOp.SUM)
expected = sum(range(self.world_size))  # 0+1+2+...+(N-1)
assert abs(tensor.item() - expected) < 1e-6
```

---

## 9. HCCL 포트 16666 충돌 문제

파일: `mini-vllm-study/minivllm/engine/distributed_manager.py:250-270`

**원인**: HCCL은 랑데부(rendezvous)에 포트 16666을 고정 사용한다. 이전 세션이 비정상 종료되면 포트가 `TIME_WAIT` 또는 `CLOSE_WAIT` 상태로 남아 재사용이 불가능하다. 에러 메시지에 `"error code is 7"` 포함.

**해결책**:
1. Docker 컨테이너 재시작 또는 30초 대기 (포트 TIME_WAIT 해제)
2. `MINIVLLM_TP_BACKEND=gloo` 설정 (CPU 기반 통신, 성능 저하)
3. TP=1 설정 (단일 디바이스, 분산 비활성화)

**mini-vLLM의 포트 충돌 예방책**: `LLMEngine.__init__`에서 빈 포트를 동적으로 탐지:

```python
# mini-vllm-study/minivllm/engine/llm_engine.py:194-205
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.bind(("127.0.0.1", 0))      # OS가 빈 포트를 자동 배정
    port = sock.getsockname()[1]      # 배정된 포트 번호 추출
os.environ["MASTER_PORT"] = str(port) # 소켓 close 후 환경변수 등록
```

---

## 10. Sequence 직렬화 최적화 (`__getstate__` / `__setstate__`)

파일: `mini-vllm-study/minivllm/engine/sequence.py:593-791`

### 문제 상황

Rank 0이 `broadcast_data(sequences)`로 Sequence 목록을 pickle 직렬화할 때, Decode 단계에서 각 시퀀스의 `token_ids`가 O(seq_len)이다. 긴 대화 컨텍스트에서 매 Decode 스텝마다 수천 개의 토큰 ID를 직렬화·전송하면 IPC 비용이 선형으로 증가한다.

### 최적화 아이디어

Decode 단계 worker는 실제로 **마지막 토큰 1개**만 필요하다. 나머지 토큰의 KV는 이미 `k_cache/v_cache`에 저장되어 있어 재전송이 불필요하다.

### `__getstate__` 구현

```python
# mini-vllm-study/minivllm/engine/sequence.py:630-658
return (
    self.num_tokens,
    self.num_prompt_tokens,
    self.num_cached_tokens,
    self.block_table,
    self.block_size,
    # 핵심 최적화:
    self.token_ids if self.num_completion_tokens == 0  # Prefill: 전체 전송
    else self.last_token,                               # Decode:  1개만 전송
    self.temperature,
    self.top_p,
    self.top_k,
    self.min_p,
    self.max_tokens,
    self.ignore_eos,
)
```

- **Prefill** (`num_completion_tokens == 0`): `token_ids` 전체 리스트 전송
  - 이유: Prefill forward는 프롬프트 토큰 전체가 입력으로 필요
- **Decode** (`num_completion_tokens > 0`): `last_token` int 1개만 전송
  - 이유: Decode forward는 현재 스텝의 새 토큰 1개만 입력으로 사용

### `__setstate__` 구현

```python
# mini-vllm-study/minivllm/engine/sequence.py:753-762
if isinstance(token_data, int):
    # Decode 최적화 경로: 마지막 토큰만 수신
    self.token_ids = [0] * self.num_tokens   # 0으로 패딩 (플레이스홀더)
    self.token_ids[-1] = token_data          # 마지막 위치에 실제 토큰 설정
    self.last_token = token_data
```

Worker는 `num_tokens` 크기의 0 패딩 리스트를 로컬에서 생성하고 마지막 원소만 실제 토큰 ID로 채운다. IPC로 O(num_tokens) 데이터를 전송하는 것보다 로컬 O(num_tokens) 메모리 할당이 훨씬 저렴하다.

### IPC 비용 비교

| 단계 | 기존 방식 | 최적화 방식 |
|------|-----------|-------------|
| Prefill | O(prompt_len) | O(prompt_len) — 동일 |
| Decode | O(total_tokens) 증가 | O(1) 고정 |

---

## 11. 멀티프로세스 초기화 동기화 타임라인

```
시간 →
Rank 0 메인 프로세스:
  ├─ spawn Rank1, Rank2, ...
  ├─ ModelRunner.__init__(rank=0)
  │     ModelManager.initialize()       [모델 로드]
  │     DistributedManager.initialize() [NCCL init_process_group]
  │     InferenceExecutor.initialize()  [KV 캐시 할당]
  │     capture_device_graphs()         [CUDA Graph 캡처]
  └─ synchronize() ←──────────────────── barrier (모든 rank 대기)
         ↓ (barrier 통과 후)
  call("run", seqs, is_prefill) ...

Rank 1, 2, ... worker 프로세스 (spawn):
  ├─ ModelRunner.__init__(rank=i)
  │     ModelManager.initialize()       [모델 로드]
  │     DistributedManager.initialize() [NCCL init_process_group]
  │     InferenceExecutor.initialize()  [KV 캐시 할당]
  │     (CUDA Graph 캡처 없음)
  └─ synchronize() ←──────────────────── barrier
         ↓ (barrier 통과 후)
  _worker_loop() → broadcast_data(None) 대기 ...
```

---

## 12. 코드 참조 색인

| 컴포넌트 | 파일 | 핵심 행 |
|----------|------|---------|
| LLMEngine + 멀티프로세스 spawn | `mini-vllm-study/minivllm/engine/llm_engine.py` | 185–234 |
| ModelRunner 초기화 + worker 루프 | `mini-vllm-study/minivllm/engine/model_runner.py` | 102–205, 298–408 |
| DistributedManager 초기화 | `mini-vllm-study/minivllm/engine/distributed_manager.py` | 140–186 |
| broadcast_data 5단계 프로토콜 | `mini-vllm-study/minivllm/engine/distributed_manager.py` | 400–528 |
| HCCL 오류코드 7 처리 | `mini-vllm-study/minivllm/engine/distributed_manager.py` | 250–270 |
| ColumnParallelLinear | `mini-vllm-study/minivllm/models/layers/linear.py` | 278–344 |
| RowParallelLinear + all-reduce | `mini-vllm-study/minivllm/models/layers/linear.py` | 546–632 |
| QKVParallelLinear | `mini-vllm-study/minivllm/models/layers/linear.py` | 419–543 |
| MergedColumnParallelLinear | `mini-vllm-study/minivllm/models/layers/linear.py` | 347–416 |
| Sequence __getstate__/__setstate__ | `mini-vllm-study/minivllm/engine/sequence.py` | 593–791 |
