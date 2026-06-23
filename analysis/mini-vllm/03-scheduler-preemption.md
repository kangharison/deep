# 03. Scheduler + 2단계 스케줄링 + 선점 정책 심층 분석

관련 문서: [[01-overview]] [[02-block-manager-prefix-cache]] [[04-inference-executor-cuda-graph]]

---

## 1. 스케줄러 역할

`mini-vllm-study/minivllm/engine/scheduler.py`

Scheduler는 LLM 추론 엔진의 "제어 센터"다. 두 가지 핵심 책임을 진다.

1. **어떤 시퀀스를 이번 스텝에서 실행할지 결정** — `schedule()` 메서드가 LLMEngine.step()에서 매 스텝 호출된다.
2. **GPU 메모리(KV 캐시 블록)를 가용 범위 안에서 최대한 활용** — BlockManager에 블록 할당/해제를 위임하고, 압박 시 선점(preemption)으로 공간을 확보한다.

### Prefill / Decode 2단계 정책

LLM 추론은 두 단계로 나뉜다.

```
Prefill 단계
  - 입력: 프롬프트 전체 토큰 (수십~수천 토큰)
  - 목적: 초기 KV 캐시 구축
  - 특성: GPU compute-bound (행렬 연산이 많아 GPU 연산이 병목)
  - 결과: 첫 번째 생성 토큰 1개 반환

Decode 단계
  - 입력: 이전 스텝에서 생성된 토큰 1개
  - 목적: 다음 토큰 1개 생성 (KV 캐시 재사용)
  - 특성: GPU memory-bound (KV 캐시 대역폭이 병목)
  - 결과: 다음 토큰 1개 반환
```

mini-vLLM은 **Prefill과 Decode를 같은 스텝에서 혼합하지 않는다**. Prefill이 있으면 그 스텝은 Prefill 전용, Prefill이 없으면 Decode 전용이다. 이 단순화는 구현 복잡도를 낮추되 처리량은 실용적 범위 내에서 유지한다.

### 큐 관리

```
waiting (deque)  ←── add()로 새 요청 투입
                      preempt()로 선점 시 앞에 재삽입
running (deque)  ←── _schedule_prefill()이 waiting에서 이동
                 ──→ postprocess()가 완료 시 remove()
```

### 블록 할당 조율

Scheduler는 BlockManager를 통해 블록 수명을 제어하지만, 물리 블록 상세(해시, 참조 카운트 등)는 BlockManager 내부에 캡슐화된다. Scheduler는 아래 API만 사용한다.

| API | 호출 위치 | 목적 |
|-----|----------|------|
| `can_allocate(seq)` | `_schedule_prefill()` | Prefill 전 블록 가용성 확인 |
| `allocate(seq)` | `_schedule_prefill()` | Prefill 시 블록 할당 |
| `can_append(seq)` | `_schedule_decode()` | Decode 전 블록 추가 가능 여부 |
| `may_append(seq)` | `_schedule_decode()` | Decode 시 필요 시 새 블록 할당 |
| `deallocate(seq)` | `preempt()`, `postprocess()` | 블록 해제 |
| `get_num_free_blocks()` | 디버그 로그 | 잔여 블록 수 조회 |

---

## 2. 핵심 자료구조

`mini-vllm-study/minivllm/engine/scheduler.py:193`

### `waiting: deque[Sequence]` — 대기 큐

```
성격:   FIFO 대기열. 프롬프트가 있지만 아직 Prefill이 실행되지 않은 시퀀스들.
설정자: add() → append()  (오른쪽 삽입, FIFO 보장)
        preempt() → appendleft()  (왼쪽 삽입, 선점된 시퀀스가 우선 재시도)
읽는 자: _schedule_prefill() → popleft()  (왼쪽 제거, FIFO 순서로 꺼냄)
         is_finished()가 len(waiting)==0 체크
상태:   모든 원소의 status == SequenceStatus.WAITING
```

### `running: deque[Sequence]` — 실행 큐

```
성격:   현재 Decode 중인 시퀀스들. 순서: 왼쪽이 오래된(oldest) 시퀀스.
설정자: _schedule_prefill() → append()  (오른쪽에 추가)
읽는 자: _schedule_decode() → popleft() 꺼내기, extendleft() 복원
         _schedule_decode() 선점 → pop()  (오른쪽, 가장 최신 시퀀스 희생)
제거:   postprocess() → remove()  (완료 시)
상태:   모든 원소의 status == SequenceStatus.RUNNING
```

### `block_manager: BlockManager` — 블록 할당 위임

```
설정자: __init__()에서 Config.num_kvcache_blocks, Config.kvcache_block_size로 단 1회 생성
읽는 자: _schedule_prefill, _schedule_decode, preempt, postprocess — 4개 메서드에서 사용
내부 상태: free_block_ids, hash_to_block_id, 블록별 ref_count
           (Scheduler는 이 내부 상태에 직접 접근하지 않음)
```

### `max_num_seqs`, `max_num_batched_tokens` — 배치 한계

```
max_num_seqs: int
  - 한 스텝에서 동시에 처리할 수 있는 최대 시퀀스 수
  - Prefill/Decode 모두 num_seqs 카운터로 이 값을 체크한다
  - 클수록 GPU 병렬도 증가하지만 KV 캐시 소모 증가

max_num_batched_tokens: int
  - Prefill 배치 내 실제 계산 토큰 수 상한 (프리픽스 캐시 히트 토큰 제외)
  - 이 값을 초과하면 waiting 큐의 다음 시퀀스를 배치에 포함하지 않는다
  - Decode에서는 시퀀스당 토큰 1개이므로 이 한도를 체크하지 않는다
```

---

## 3. `schedule()` 흐름

`mini-vllm-study/minivllm/engine/scheduler.py:344`

```
schedule()
  ├─ is_finished() → RuntimeError  # 빈 상태에서 호출 방어
  │
  ├─ Phase 1: _schedule_prefill()
  │     ├─ waiting이 비어 있으면 → []
  │     └─ 있으면 → [seq1, seq2, ...] (is_prefill=True 반환)
  │               (이미 running 큐로 이동 + 블록 할당 완료 상태)
  │
  └─ Phase 2: _schedule_decode()  ← Prefill 결과가 [] 일 때만 실행
        ├─ running이 비어 있으면 → []
        └─ 있으면 → [seq1, seq2, ...] (is_prefill=False 반환)

반환 타입: tuple[list[Sequence], bool]
  - bool=True  → 이번 스텝은 Prefill (LLMEngine이 _prefill_forward 경로 실행)
  - bool=False → 이번 스텝은 Decode (LLMEngine이 _decode_forward 경로 실행)
```

**Prefill 우선인 이유:**

- Prefill은 compute-bound이므로 GPU 행렬 연산 집약적이다. 전용 스텝으로 분리하면 GPU 가동률이 최대화된다.
- Prefill+Decode 혼합 배치를 지원하려면 FlashAttention의 두 커널(`varlen_func` + `with_kvcache`)을 한 forward에서 모두 호출해야 하는데, mini-vLLM은 이를 지원하지 않는다 (단순화 설계).
- Prefill이 있으면 해당 스텝에서 모든 실행 비용을 Prefill에 집중해 TTFT(Time To First Token)를 줄인다.

---

## 4. Prefill 스케줄링 (`_schedule_prefill()`)

`mini-vllm-study/minivllm/engine/scheduler.py:445`

```python
while self.waiting and num_seqs < self.max_num_seqs:
    sequence = self.waiting[0]           # peek (제거 안 함)

    if num_batched_tokens + len(sequence) > self.max_num_batched_tokens
       or not self.block_manager.can_allocate(sequence):
        break                            # 조건 불충족: 이후도 시도 안 함

    num_seqs += 1
    self.block_manager.allocate(sequence)

    # 캐시 히트 토큰 제외 후 실제 연산 토큰만 카운트
    num_batched_tokens += len(sequence) - sequence.num_cached_tokens

    sequence.status = SequenceStatus.RUNNING
    self.waiting.popleft()
    self.running.append(sequence)
    scheduled_sequences.append(sequence)
```

**`num_batched_tokens += len(seq) - seq.num_cached_tokens` — 캐시 토큰 제외 이유:**

- `allocate()` 호출 후 `seq.num_cached_tokens`가 갱신된다 (프리픽스 캐시 히트 수).
- 이미 KV가 준비된 토큰은 GPU 재연산이 불필요하므로 연산량 카운터에서 제외한다.
- 예: 512 토큰 시퀀스에서 256 토큰이 캐시 히트면 실제 연산은 256 토큰만 필요하다.
- 이 값은 `max_num_batched_tokens` 상한 검사에 사용되므로, 제외하지 않으면 캐시 효과를 활용하지 못한다.

**조건 불충족 시 즉시 break (FIFO 유지):**

- waiting 큐는 FIFO이므로 앞 시퀀스가 조건 실패 = 뒤 시퀀스도 비슷하거나 더 나쁜 조건이다.
- 순서를 건너뛰면 공정성(fairness)이 무너져 앞 시퀀스가 영구 기아 상태에 빠질 수 있다.
- 따라서 첫 번째 실패 시점에서 루프를 종료한다.

**`can_allocate(seq)` 체크:**

- 시퀀스가 필요로 하는 블록 수와 현재 free 블록 수를 비교한다.
- 필요 블록 수 = `ceil(len(seq) / block_size)` (프리픽스 캐시 히트 블록 제외).

**`block_manager.allocate(seq)` → `WAITING → RUNNING`:**

- 내부에서 프리픽스 해시 조회 → 히트 시 기존 블록 참조(ref_count 증가) → 미스 시 새 블록 할당.
- 할당 후 `seq.num_cached_tokens`와 `seq.block_table`이 갱신된다.
- 상태 전이: `sequence.status = SequenceStatus.RUNNING`

---

## 5. Decode 스케줄링 (`_schedule_decode()`)

`mini-vllm-study/minivllm/engine/scheduler.py:571`

```python
while self.running and num_seqs < self.max_num_seqs:
    sequence = self.running.popleft()     # 가장 오래된 시퀀스 꺼내기

    while not self.block_manager.can_append(sequence):
        if self.running:
            preempted_seq = self.running.pop()   # 가장 최신 시퀀스 희생
            self.preempt(preempted_seq)
        else:
            self.preempt(sequence)               # 유일한 시퀀스 자신을 선점
            break
    else:
        # break 없이 while 탈출 = can_append()가 True가 됨
        num_seqs += 1
        self.block_manager.may_append(sequence)
        scheduled_sequences.append(sequence)

# 선발된 시퀀스를 running 큐 앞에 원래 순서로 복원
if scheduled_sequences:
    self.running.extendleft(reversed(scheduled_sequences))
```

**Python `while-else` 의미론:**

```
while <조건>:
    ...
    break     ← 이 경로: else 블록 실행 안 됨
else:
    ...       ← 이 경로: break 없이 <조건>이 False가 되어 종료 시 실행
```

여기서:
- `while not self.block_manager.can_append(sequence):` — can_append()가 False(블록 부족)인 동안 반복
- `else` — can_append()가 True가 되어(블록 확보) while 조건이 False가 된 경우 실행
- `break` — 현재 시퀀스 자신을 선점한 경우 (else 블록 건너뜀 → 이 시퀀스는 선발되지 않음)

**`running.extendleft(reversed(scheduled_sequences))` — deque 순서 복원 이유:**

popleft()로 꺼낸 시퀀스들은 Decode 후에도 running 큐에 남아 있어야 한다. 복원 방법:

```
scheduled = [A, B, C]  (A가 가장 오래된 순서로 꺼낸 것)
reversed(scheduled) = [C, B, A]
extendleft([C,B,A]):
  appendleft(C)  → deque = [C, ...]
  appendleft(B)  → deque = [B, C, ...]
  appendleft(A)  → deque = [A, B, C, ...]  ← 원래 순서 복원
```

`extendleft`는 이터러블 원소를 하나씩 왼쪽에 삽입하므로 이터러블 순서가 역전된다. 따라서 `reversed()`로 미리 역전해 두면 최종 deque 순서가 원래와 같아진다.

---

## 6. 선점(Preemption) 정책

`mini-vllm-study/minivllm/engine/scheduler.py:693`

```python
def preempt(self, sequence: Sequence) -> None:
    logger.info("Preempting sequence %d due to memory constraints.", sequence.seq_id)

    sequence.status = SequenceStatus.WAITING      # RUNNING → WAITING
    self.block_manager.deallocate(sequence)       # KV 블록 해제 (해시는 보존)
    self.waiting.appendleft(sequence)             # 큐 앞에 재삽입 (우선 재스케줄)
```

**가장 최근 시퀀스(running.pop())를 희생 대상으로 삼는 이유:**

```
running = [seq_old, seq_mid, seq_new]
                                ↑
                              pop() 희생
```

- `running` 큐는 왼쪽이 오래된 시퀀스다. 오래된 시퀀스일수록 더 많은 KV 블록을 점유하고, 더 오래 실행된 만큼 이미 많은 컴퓨팅 비용이 투입되었다.
- 가장 최신 시퀀스는 아직 생성 토큰이 적어 재계산 비용이 낮다.
- 오래된 시퀀스를 살리면 전반적인 forward progress가 보장된다 (기아 방지).

**선점 후 캐시 히트 가능성 (hash 보존):**

`deallocate(seq)` 내부에서 블록의 ref_count를 감소시키지만, `hash_to_block_id` 매핑(프리픽스 캐시 인덱스)은 유지된다. 다른 시퀀스에 의해 블록이 덮어씌워지지 않는 한, 선점된 시퀀스가 재스케줄될 때 `allocate()`에서 동일 프리픽스를 찾아 캐시 히트로 처리된다. 즉 선점 → 재스케줄 시 전부 재계산(full recomputation)이 아닌 부분 히트(partial hit)가 가능하다.

---

## 7. `postprocess()`

`mini-vllm-study/minivllm/engine/scheduler.py:763`

```python
def postprocess(self, sequences: list[Sequence], token_ids: list[int]) -> None:
    for sequence, token_id in zip(sequences, token_ids, strict=True):
        sequence.append_token(token_id)

        is_eos = (token_id == self.eos) and not sequence.ignore_eos
        is_max_len = sequence.num_completion_tokens >= sequence.max_tokens

        if is_eos or is_max_len:
            sequence.status = SequenceStatus.FINISHED
            self.block_manager.deallocate(sequence)
            self.running.remove(sequence)
```

**`zip(..., strict=True)`:**
- 두 리스트의 길이가 다르면 `ValueError` 발생.
- GPU가 반환한 `token_ids` 수와 처리한 `sequences` 수의 불일치를 즉시 감지하는 방어 코드다.
- Python 3.10+의 기능.

**EOS 체크 조건:**
```
is_eos = (token_id == self.eos) AND (not seq.ignore_eos)
```
- `ignore_eos=True`이면 EOS 토큰을 받아도 생성을 계속한다 (강제로 더 긴 출력이 필요한 경우).

**max_tokens 체크:**
```
is_max_len = seq.num_completion_tokens >= seq.max_tokens
```
- `num_completion_tokens`는 `append_token()` 후 갱신되므로, 이 라인이 `append_token()` 뒤에 위치함에 주의.

**완료 시: `FINISHED` → `deallocate()` → `running.remove()` (O(n) 허용 근거):**
- `deque.remove()`는 O(n) 선형 탐색이다.
- 허용 근거: (1) running 큐 내 시퀀스 수는 max_num_seqs로 제한되어 n이 작다, (2) 완료 시퀀스는 매 스텝 드물어 실제 호출 빈도가 낮다.
- 순서: `deallocate()` 먼저, `remove()` 나중 — 블록을 먼저 해제해야 다음 시퀀스가 즉시 이 블록을 사용할 수 있다.

---

## 8. 처리량 영향 분석

### Prefill은 compute-bound

- 긴 프롬프트 = GPU 행렬 곱 연산 집약적 → GPU SM(Streaming Multiprocessor) 포화.
- 배치 내 여러 시퀀스를 묶을수록 행렬 크기가 커져 GPU 효율 증가.
- 그러나 `max_num_batched_tokens`를 초과하면 메모리 부족이나 연산 시간 초과.

### Decode는 memory-bound

- 시퀀스당 토큰 1개만 처리 → 행렬 크기가 작아 GPU SM 가동률 낮음.
- 병목은 KV 캐시 읽기 대역폭 (DRAM bandwidth).
- KV 캐시 크기가 클수록 더 많은 시퀀스를 동시에 Decode 가능하지만, GPU VRAM이 제한.

### 선점 발생 시 비용

```
선점 비용 = preempted_seq가 지금까지 받은 Prefill 비용이 낭비될 수 있음
           (프리픽스 캐시 히트 시 일부 복구 가능)
```

- 선점 빈도가 높으면 처리량이 급감한다.
- 선점을 줄이려면: `num_kvcache_blocks` 증가, `max_num_seqs` 감소, `device_memory_utilization` 조정.

---

## 9. ASCII 상태 전이도

```
                           add()
                             |
                             v
                        ┌─────────┐
                        │ WAITING │
                        └────┬────┘
                             │ _schedule_prefill()
                             │ (can_allocate() 통과 후)
                             │ block_manager.allocate()
                             v
                        ┌─────────┐
                        │ RUNNING │
                        └────┬────┘
                             │
           ┌─────────────────┼─────────────────────┐
           │                 │                     │
   preempt() ←              │ postprocess()        │ postprocess()
   (can_append 실패 시)      │ token_id == eos      │ num_completion_tokens
           │                 │ AND not ignore_eos   │ >= max_tokens
           v                 v                     v
      ┌─────────┐       ┌──────────┐
      │ WAITING │       │ FINISHED │
      │ (앞에   │       └──────────┘
      │  재삽입)│         block_manager.deallocate()
      └─────────┘         running.remove()

- WAITING → RUNNING: block_manager.allocate() + SequenceStatus.RUNNING
- RUNNING → WAITING: preempt() → block_manager.deallocate() + SequenceStatus.WAITING + waiting.appendleft()
- RUNNING → FINISHED: postprocess() → block_manager.deallocate() + SequenceStatus.FINISHED + running.remove()
```

---

## 10. 코드 참조 요약

| 위치 | 내용 |
|------|------|
| `mini-vllm-study/minivllm/engine/scheduler.py:155` | `class Scheduler` 정의 |
| `mini-vllm-study/minivllm/engine/scheduler.py:193` | `__init__`: 파라미터 초기화, BlockManager 생성, 큐 초기화 |
| `mini-vllm-study/minivllm/engine/scheduler.py:281` | `is_finished()`: 종료 조건 판정 |
| `mini-vllm-study/minivllm/engine/scheduler.py:307` | `add()`: 대기 큐 투입 |
| `mini-vllm-study/minivllm/engine/scheduler.py:344` | `schedule()`: 2단계 스케줄링 진입점 |
| `mini-vllm-study/minivllm/engine/scheduler.py:445` | `_schedule_prefill()`: Prefill 대상 선발 |
| `mini-vllm-study/minivllm/engine/scheduler.py:571` | `_schedule_decode()`: Decode 대상 선발 + 선점 처리 |
| `mini-vllm-study/minivllm/engine/scheduler.py:693` | `preempt()`: KV 블록 해제 + waiting 복귀 |
| `mini-vllm-study/minivllm/engine/scheduler.py:763` | `postprocess()`: 토큰 반영 + 완료 처리 |
| `mini-vllm-study/minivllm/engine/block_manager.py` | BlockManager: 블록 할당/해제 세부 로직 |
| `mini-vllm-study/minivllm/engine/sequence.py` | Sequence, SequenceStatus 정의 |
| `mini-vllm-study/minivllm/engine/llm_engine.py` | LLMEngine.step(): schedule() 호출 컨텍스트 |
