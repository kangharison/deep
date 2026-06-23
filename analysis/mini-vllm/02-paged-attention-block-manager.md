# PagedAttention & BlockManager 심층 분석

관련 문서: [[README]] | [[01-architecture-overview]]

---

## 1. PagedAttention이란

전통적인 LLM 서빙에서 KV 캐시는 시퀀스 최대 길이(`max_model_len`)에 맞춰 **연속된 메모리 블록**을 미리 예약한다. 시퀀스가 실제로 그 길이에 도달하지 않아도 메모리는 예약되므로 내부 단편화(internal fragmentation)가 심하다. 또한 시퀀스 사이에 메모리를 공유할 방법이 없어 동일한 프롬프트를 반복 처리하면 KV를 매번 재계산해야 한다.

PagedAttention(SOSP'23, vLLM 논문)은 **OS의 가상 메모리 페이징**에서 영감을 받아 KV 캐시를 고정 크기 **블록**으로 쪼갠다. 논리 주소(시퀀스 토큰 번호)와 물리 주소(KV 캐시 텐서의 슬롯)를 **block_table**로 간접 참조해, 필요한 만큼만 블록을 동적으로 할당하고 해제한다. 이로 인해:

- **메모리 단편화 거의 없음**: 블록 경계에서만 발생하는 내부 단편화 ≤ block_size−1 토큰
- **KV 캐시 공유 가능**: 동일 프리픽스를 가진 시퀀스가 같은 물리 블록을 참조 카운트로 공유
- **유연한 할당**: 가변 길이 시퀀스를 블록 풀에서 동적으로 병렬 처리

mini-vLLM의 PagedAttention 구현 핵심은 `mini-vllm-study/minivllm/engine/block_manager.py`의 `BlockManager` 클래스다.

---

## 2. Block 클래스 구조

`Block`은 KV 캐시 물리 블록 하나의 **메타데이터**만 관리하는 경량 객체다. 실제 K/V 텐서 값은 `InferenceExecutor.kv_cache` 텐서에 저장되고, `Block`은 그 주소(block_id)만 알고 있다.

```python
# mini-vllm-study/minivllm/engine/block_manager.py:70
class Block:
    block_id:   int        # 블록 풀 내 고유 인덱스 [0, num_blocks)
                           # kv_cache 텐서의 3번째 차원 인덱스와 1:1 동일

    ref_count:  int        # 이 블록을 참조하는 시퀀스 수
                           # 0: 미사용(free), 1: 단독 사용, ≥2: 프리픽스 캐시 공유
                           # 해제 시 ref_count--, 0이 되면 free 풀로 반환

    hash:       int        # 블록 내 token_ids의 체인 xxhash 값
                           # -1: 미계산(partial block 또는 reset 상태)
                           # 양의 정수: 블록이 꽉 찼을 때 계산된 해시
                           # hash_to_block_id 딕셔너리의 키로 사용

    token_ids:  list[int]  # 이 블록에 담긴 토큰 ID 복사본
                           # 프리픽스 캐시 히트 확인 시 해시 충돌 검증에 사용
                           # (해시 같아도 token_ids가 다르면 히트 아님)
```

블록 크기(block_size)는 기본값 64이며 반드시 64의 배수여야 한다. 이유는 FlashAttention 커널이 내부적으로 64 토큰 단위의 타일링을 사용하기 때문이다(`mini-vllm-study/minivllm/config.py`의 `kvcache_block_size % 64 == 0` 검증).

---

## 3. KV 캐시 텐서 형태

`InferenceExecutor._allocate_kv_cache()`에서 단 한 번 할당되는 KV 캐시 텐서의 형상은 다음과 같다.

```
kv_cache: (2, num_layers, num_blocks, block_size, num_kv_heads, head_dim)
            │   │           │           │           │             │
            │   │           │           │           │             └─ 헤드 당 차원 (예: 128)
            │   │           │           │           └─ KV 헤드 수 (GQA시 < num_heads)
            │   │           │           └─ 블록당 토큰 수 (기본 64)
            │   │           └─ 전체 블록 수 (GPU 메모리에서 자동 계산)
            │   └─ Transformer 레이어 수 (예: Qwen3-0.6B = 28)
            └─ 0=K 캐시, 1=V 캐시

예시 (Qwen3-0.6B, TP=1, block_size=64):
  num_kv_heads = 8 (GQA)
  head_dim     = 128
  num_layers   = 28
  num_blocks   = (GPU 가용 메모리 × 0.9) / (2 × 28 × 64 × 8 × 128 × dtype_bytes)

kv_cache[0, l, b, t, h, d] = 레이어 l, 블록 b, 블록 내 t번째 토큰, 헤드 h의 Key 벡터 d번째 요소
kv_cache[1, l, b, t, h, d] = 위와 동일하지만 Value
```

각 어텐션 레이어는 `_assign_kv_cache_to_layers()`에서 자신의 슬라이스를 주입받는다.

```python
# 레이어 l의 K 캐시 슬라이스:
layer.k_cache = kv_cache[0, l, :, :, :, :]  # shape: (num_blocks, block_size, num_kv_heads, head_dim)
layer.v_cache = kv_cache[1, l, :, :, :, :]  # shape: 동일
```

**슬롯(slot) 공식** — 토큰이 KV 캐시의 어디에 저장되는지 결정하는 핵심 인덱스:

```
slot = block_id * block_size + intra_block_offset

예시:
  block_id = 7, block_size = 64, offset = 3
  → slot = 7 * 64 + 3 = 451

즉 kv_cache[0, l, 7, 3, :, :] 에 이 토큰의 Key가 저장됨
```

`slot_mapping`은 현재 처리 중인 모든 토큰의 슬롯 번호를 나열한 1D 텐서로, Attention 레이어의 KV 쓰기 연산에서 사용된다(`mini-vllm-study/minivllm/utils/context.py`).

---

## 4. BlockManager 핵심 자료구조

```python
# mini-vllm-study/minivllm/engine/block_manager.py:173
class BlockManager:
    block_size:        int           # 블록당 토큰 수 (기본 64, 불변)
    num_blocks:        int           # 전체 블록 풀 크기 (불변)
    blocks:            list[Block]   # blocks[i].block_id == i (1:1 불변 관계)

    free_block_ids:    deque[int]    # 할당 가능한 블록 ID FIFO 큐
    _free_set:         set[int]      # free_block_ids 보조 집합 (O(1) 멤버십)
    used_block_ids:    set[int]      # 현재 할당된 블록 ID 집합

    hash_to_block_id:  dict[int,int] # {체인 해시 → block_id} (프리픽스 캐시 인덱스)
    stats:             dict          # 총 할당 수, 캐시 히트/미스 카운터
```

### free_block_ids + _free_set 이중 구조의 이유 (Lazy Tombstone)

deque 하나만 쓰면 중간 원소 삭제가 O(n)이다. set 하나만 쓰면 FIFO 순서를 보장할 수 없다. 두 자료구조를 함께 쓰는 **Lazy Tombstone** 방식은 다음과 같다.

```
할당 시 (_allocate_block):
  _free_set.remove(block_id)   ← O(1) 즉시 제거 (멤버십 검사에 사용)
  (deque에서는 제거 안 함 → lazy)

해제 시 (_deallocate_block):
  free_block_ids.append(block_id)  ← deque 오른쪽에 추가
  _free_set.add(block_id)           ← set에도 추가

꺼낼 때 (_get_any_free_block):
  while True:
    block_id = free_block_ids[0]          ← peek
    if block_id in _free_set: return it   ← 유효한 free 블록
    free_block_ids.popleft()              ← tombstone → 제거
    (이미 할당된 블록: deque에는 있지만 set에 없음)
```

이 방식의 장점: 각 block_id는 deque에 최대 1번 들어가므로 총 작업량 O(num_blocks)로 분할상환 O(1).

### hash_to_block_id: 프리픽스 캐시 인덱스

```
{체인 해시 정수 → block_id 정수}

등록 시점:
  allocate()에서 cache miss: hash_to_block_id[hash] = block_id
  may_append()에서 블록 완성: hash_to_block_id[hash] = block_id

조회 시점:
  allocate()에서 cache hit 검사

제거 시점:
  _allocate_block(reset=True)에서 기존 블록에 새 데이터를 덮어쓸 때
  (해당 block_id의 기존 해시 매핑을 제거)

주의: _deallocate_block()은 hash_to_block_id를 건드리지 않는다.
  → ref_count=0으로 free 풀에 돌아간 블록도 해시는 캐시에 남아 "부활" 가능
```

---

## 5. 블록 할당 흐름 (allocate)

`Scheduler._schedule_prefill()`이 `block_manager.allocate(sequence)`를 호출하는 전체 흐름:

```
mini-vllm-study/minivllm/engine/block_manager.py:514

allocate(sequence):
  검증: block_table이 비어 있는지, 충분한 free 블록이 있는지

  hash_prev = -1   ← 체인 해시 누적값 초기화

  for i in range(sequence.num_blocks):
      token_ids = sequence.block(i)   ← i번째 블록의 토큰 ID 추출

      [단계 1] 해시 계산 (꽉 찬 블록만)
        if len(token_ids) == block_size:
            hash_prev = compute_hash(token_ids, hash_prev)
            # xxhash64(hash_prev_8bytes || token_ids_bytes) → 체인 해시
        else:
            hash_prev = -1  ← 부분 블록: 해시 불가 → 캐시 불가

      [단계 2] 프리픽스 캐시 조회
        if hash_prev != -1:
            block_id = hash_to_block_id.get(hash_prev, -1)

      [단계 3] 캐시 히트 판정 (4조건 모두 충족해야 히트)
        cache_hit = (
            hash_prev != -1                           # 꽉 찬 블록
            and block_id != -1                        # 해시 매핑 존재
            and 0 <= block_id < len(blocks)           # 유효한 범위
            and blocks[block_id].token_ids == token_ids  # 해시 충돌 방지: 직접 비교
        )

      [단계 4A] 캐시 미스: 새 블록 할당
        block_id = _get_any_free_block()  ← lazy tombstone 방식으로 O(1)
        block = _allocate_block(block_id)  ← reset=True
        stats["cache_misses"] += 1
        if hash_prev != -1:
            block.update(hash_prev, token_ids)   ← 해시/토큰 기록
            hash_to_block_id[hash_prev] = block_id  ← 캐시 등록

      [단계 4B] 캐시 히트: 블록 공유
        stats["cache_hits"] += 1
        sequence.num_cached_tokens += block_size  ← GPU 재연산 건너뜀
        if block_id in used_block_ids:
            blocks[block_id].ref_count += 1  ← 이미 used → 공유 카운트 증가
        else:
            block = _allocate_block(block_id, reset=False)  ← 부활 (캐시 유지)

      sequence.block_table.append(block_id)  ← 논리→물리 매핑 기록
```

**4조건 히트 검사가 필요한 이유**: xxhash는 비암호화 해시로 해시 충돌(다른 토큰인데 해시 같음)이 드물지만 가능하다. 마지막 `token_ids == token_ids` 직접 비교가 충돌을 방지하는 안전망이다.

---

## 6. Prefix Caching 해시 체인

동일한 토큰 시퀀스라도 **시퀀스 내 위치(앞에 오는 접두사)가 다르면 다른 해시**가 되어야 한다. 그렇지 않으면 다른 문맥의 KV를 잘못 공유하는 버그가 생긴다. 이를 위해 **체인 해시(chained hash)**를 사용한다.

```
시퀀스 예시 (block_size = 64):
  tokens = [t0, t1, ..., t63,  t64, ..., t127,  t128, ..., t191]
              └── Block 0 ──┘  └─── Block 1 ──┘  └─── Block 2 ──┘

체인 해시 계산:
  H0 = xxhash64(
         prefix = -1,          ← "이전 블록 없음"
         data   = tokens[0:64]
       )
  H1 = xxhash64(
         prefix = H0,          ← H0를 리틀엔디언 8바이트로 직렬화
         data   = tokens[64:128]
       )
  H2 = xxhash64(
         prefix = H1,
         data   = tokens[128:192]
       )

hash_to_block_id: { H0 → block_id_0, H1 → block_id_1, H2 → block_id_2 }
```

같은 `tokens[64:128]`이 다른 접두사 뒤에 오는 두 시퀀스의 H1을 비교해 보면:

```
시퀀스 A: 앞 블록 해시 = H0_A → H1_A = xxhash(H0_A_bytes || tokens[64:128])
시퀀스 B: 앞 블록 해시 = H0_B → H1_B = xxhash(H0_B_bytes || tokens[64:128])

H0_A ≠ H0_B → H1_A ≠ H1_B → 독립적으로 캐시됨 (오염 없음)
```

반대로 시퀀스 A와 B가 완전히 동일한 프리픽스를 공유하면:

```
시퀀스 A와 B: tokens[0:128] 동일
  H0_A = H0_B → H1_A = H1_B → 동일 block_id → KV 캐시 공유 (프리픽스 캐시 히트)
```

구현: `mini-vllm-study/minivllm/engine/block_manager.py:291`

---

## 7. 블록 해제 흐름 (deallocate)

시퀀스가 완료(`FINISHED`)되거나 선점(`preempt`)될 때 `block_manager.deallocate(sequence)`가 호출된다.

```python
# mini-vllm-study/minivllm/engine/block_manager.py:639

deallocate(sequence):
  for block_id in reversed(sequence.block_table):
      # reversed: 최신 블록(뒤)부터 해제 → 오래된 블록(앞) 더 오래 캐시에 남음
      block = blocks[block_id]
      block.ref_count -= 1
      if block.ref_count == 0:
          _deallocate_block(block_id)  ← free 풀 반환 (hash_to_block_id는 건드리지 않음)
      # ref_count > 0: 다른 시퀀스가 공유 중 → 해제 안 함

  sequence.num_cached_tokens = 0
  sequence.block_table.clear()
```

`_deallocate_block()`이 `hash_to_block_id`를 건드리지 않는 이유: free 풀로 돌아간 블록도 해시 매핑을 유지하면, 이후 동일한 프리픽스를 가진 시퀀스가 할당 요청을 할 때 "부활(resurrect)"로 재사용할 수 있다. 블록이 진짜 새 데이터로 덮어씌워질 때(`_allocate_block(reset=True)`)에야 비로소 기존 해시 매핑이 제거된다.

---

## 8. Decode 중 블록 추가 (may_append)

Decode는 토큰을 1개씩 추가한다. 블록이 꽉 차는 경계에서만 새 블록이 필요하다. `Scheduler._schedule_decode()`는 `can_append()` → `may_append()` 순서로 처리한다.

### 8-1. can_append: 블록 경계 검사

```python
# mini-vllm-study/minivllm/engine/block_manager.py:680

can_append(sequence) -> bool:
  needs_new_block = len(sequence) % block_size == 0
  # 현재 길이가 block_size 배수 → 다음 토큰에 새 블록 필요
  # 예: block_size=64, len=64 → 64%64=0 → 새 블록 필요
  #     block_size=64, len=65 → 65%64=1 → 현재 블록에 추가 가능

  required = 1 if needs_new_block else 0
  return len(_free_set) >= required
```

### 8-2. may_append: 세 가지 경우

```python
# mini-vllm-study/minivllm/engine/block_manager.py:726

may_append(sequence):
  last_block = blocks[block_table[-1]]

  [경우 1] len(seq) % block_size == 1: 새 블록 시작
    # 방금 토큰 추가 후 새 블록의 첫 슬롯이 채워진 상태
    # 이전 블록은 이미 가득 차 해시가 확정됐어야 함 (검증)
    block_id = _get_any_free_block()
    _allocate_block(block_id)          ← reset=True
    block_table.append(block_id)

  [경우 2] len(seq) % block_size == 0: 현재 블록 완성
    # 방금 토큰으로 블록이 꽉 참 → 해시 계산 후 캐시 등록
    token_ids = sequence.block(num_blocks - 1)
    prefix = blocks[block_table[-2]].hash if len(block_table) > 1 else -1
    hash_val = compute_hash(token_ids, prefix)  ← 체인 해시
    last_block.update(hash_val, token_ids)
    hash_to_block_id[hash_val] = last_block.block_id

  [경우 3] 그 외 (부분 블록 중간)
    # 아무것도 하지 않음
    # 불변 조건 검증: last_block.hash == -1 이어야 함
```

경우 2와 경우 1의 순서 관계: 블록 완성(경우 2, `len%block_size==0`)은 경우 1(`len%block_size==1`)보다 **한 스텝 먼저** 발생한다. 즉 `len=64`일 때 경우 2로 해시를 확정하고, `len=65`일 때 경우 1로 새 블록을 할당한다.

### 8-3. Copy-on-Write (CoW)

마지막 블록이 `ref_count >= 2`(프리픽스 캐시로 공유 중)일 때 이 시퀀스가 새 토큰을 쓰면 다른 시퀀스의 KV를 오염시킬 수 있다. mini-vLLM은 `Scheduler._schedule_decode()`에서 이 상황을 처리한다:

```
현재 last_block.ref_count >= 2?
  → 새 블록을 할당하고 마지막 블록의 내용을 복사 (Copy-on-Write)
  → 원본 블록의 ref_count를 줄여 다른 시퀀스와의 공유를 분리

구현 위치: mini-vllm-study/minivllm/engine/scheduler.py의 _schedule_decode 내부
           (BlockManager.may_append와 협력)
```

---

## 9. 선점(Preempt) 시 블록 처리

메모리 부족으로 실행 중인 시퀀스를 선점해야 할 때, `Scheduler.preempt()`가 다음 단계를 수행한다:

```python
# mini-vllm-study/minivllm/engine/scheduler.py:693

preempt(sequence):
  1. sequence.status = WAITING
  2. block_manager.deallocate(sequence)
     → ref_count 감소, 0이면 free 풀 반환
     → hash_to_block_id는 유지됨! (해시 매핑 보존)
  3. waiting.appendleft(sequence)
     → 큐 맨 앞에 삽입 (다음 사이클에서 우선 재시도)
```

**선점 후 재스케줄 시 프리픽스 캐시 히트 가능성**:

```
선점 전:
  시퀀스 S: tokens[0:256], block_table=[7, 2, 15, 31], 해시 H0,H1,H2,H3

선점 시:
  block_manager.deallocate(S)
  → blocks[31].ref_count-- → 0 → _deallocate_block(31) → free 풀 반환
  → blocks[15].ref_count-- → 0 → _deallocate_block(15)
  → ...
  → hash_to_block_id: {H0→7, H1→2, H2→15, H3→31} 여전히 존재!

재스케줄 시:
  block_manager.allocate(S)
  → H0: hash_to_block_id에 있음 → block_id=7 확인
         blocks[7].ref_count가 0이고 free_set에 있음
         → reset=False로 "부활" 할당 (token_ids/hash 유지)
         → 캐시 히트! num_cached_tokens += 64
  → H1, H2, H3도 동일하게 캐시 히트 (블록이 다른 시퀀스에 재활용되지 않았다면)
```

선점된 시퀀스가 나중에 재스케줄되면 이전 프리픽스 KV를 재사용할 수 있어 재계산 비용이 줄어든다. 단, 그 사이에 다른 시퀀스가 해당 블록을 덮어쓴 경우에는 cache miss로 처음부터 재계산한다.

---

## 10. 블록 상태 전이 다이어그램

```
                     BlockManager.__init__()
                          │
                          ▼
              ┌───────────────────────┐
              │   미할당 (free)        │
              │   ref_count = 0       │
              │   hash = -1           │
              │   free_set에 있음     │
              └──────────┬────────────┘
                         │ _allocate_block(reset=True)
                         │ - _free_set.remove()
                         │ - used_block_ids.add()
                         │ - block.reset() → ref=1, hash=-1, tokens=[]
                         ▼
              ┌───────────────────────┐
              │   단독 할당 (used)     │
              │   ref_count = 1       │
              │   hash = -1 (partial) │
              │   used_block_ids에 있음│
              └──────────┬────────────┘
                         │ block.update(hash, tokens) [may_append 또는 allocate]
                         │ hash_to_block_id[hash] = block_id
                         ▼
              ┌───────────────────────┐
              │   해시 확정 (used)     │
              │   ref_count = 1       │
              │   hash ≠ -1           │
              │   hash_to_block_id 등록│
              └──────────┬────────────┘
                         │ 다른 시퀀스가 같은 해시로 allocate → ref_count++
                         ▼
              ┌───────────────────────┐
              │   공유 중 (used)       │
              │   ref_count ≥ 2       │
              │   프리픽스 캐시 공유   │
              └──────────┬────────────┘
                         │ 각 시퀀스 deallocate → ref_count--
                         │ 마지막 참조가 해제되면 ref_count = 0
                         ▼
              ┌───────────────────────┐
              │   캐시 유지 free       │
              │   ref_count = 0       │
              │   hash ≠ -1 (유지)    │
              │   free_set에 있음     │
              │   hash_to_block_id 유지│ ← "부활" 가능 상태
              └──────────┬────────────┘
                         │ _allocate_block(reset=False) → 부활 (캐시 히트)
                         │ OR _allocate_block(reset=True) → 덮어쓰기 (캐시 미스)
                         └───────────────────────────────┘
                               ↑ 위 상태들로 돌아감
```

---

## 11. 코드 참조 요약

| 항목 | 파일 및 라인 |
|------|------------|
| Block 클래스 정의 | `mini-vllm-study/minivllm/engine/block_manager.py:70` |
| BlockManager 초기화 | `mini-vllm-study/minivllm/engine/block_manager.py:227` |
| compute_hash (체인 xxhash) | `mini-vllm-study/minivllm/engine/block_manager.py:291` |
| _get_any_free_block (lazy tombstone) | `mini-vllm-study/minivllm/engine/block_manager.py:344` |
| _allocate_block | `mini-vllm-study/minivllm/engine/block_manager.py:387` |
| _deallocate_block | `mini-vllm-study/minivllm/engine/block_manager.py:448` |
| allocate (프리픽스 캐시 + 블록 할당) | `mini-vllm-study/minivllm/engine/block_manager.py:514` |
| deallocate (ref_count 감소 + 해제) | `mini-vllm-study/minivllm/engine/block_manager.py:639` |
| can_append (블록 경계 검사) | `mini-vllm-study/minivllm/engine/block_manager.py:680` |
| may_append (새 블록 할당/해시 확정) | `mini-vllm-study/minivllm/engine/block_manager.py:726` |
| preempt (선점 → waiting 복귀) | `mini-vllm-study/minivllm/engine/scheduler.py:693` |
| KV 캐시 텐서 형상 | `mini-vllm-study/minivllm/engine/inference_executor.py` |
| slot_mapping 구성 | `mini-vllm-study/minivllm/utils/context.py` |

---

## 관련 문서

- [[01-architecture-overview]] — 전체 아키텍처 개요 및 핵심 개념
- [[README]] — 문서 인덱스 및 학습 권장 순서
