# GPGPU-Sim 메모리 시스템 심층 분석 (Part 2)

> 소스: `sources/gpgpu-sim_distribution/src/gpgpu-sim/` 내 gpu-cache.h/cc, l2cache.h/cc, mem_fetch.h/cc, dram.h/cc, dram_sched.h/cc, shader.cc, icnt_wrapper.h/cc, local_interconnect.h/cc, addrdec.h, hashing.h, abstract_hardware_model.cc
> 이 문서는 GPGPU-Sim의 메모리 계층(캐시, DRAM, 인터커넥트)을 소스 코드 레벨에서 완전히 분석한다. 이 문서만 읽으면 소스 코드를 볼 필요가 없도록 작성하였다.

---

## 9. GPU 캐시 계층 구조 (Cache Hierarchy)

### 9.1 전체 구조 다이어그램

```
 ┌─────────────────────────────────────────────────────────────┐
 │                    SM (Streaming Multiprocessor)             │
 │                                                             │
 │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
 │  │ L1 Inst  │  │ L1 Data  │  │L1 Const  │  │L1 Texture│   │
 │  │ (I-Cache)│  │ (D-Cache)│  │ (C-Cache) │  │(T-Cache) │   │
 │  │ read-only│  │ RW, L1D  │  │ read-only │  │read-only │   │
 │  │          │  │ WB/WE/WT │  │ broadcast │  │ 2D 지역성│   │
 │  └────┬─────┘  └────┬─────┘  └─────┬─────┘  └────┬─────┘   │
 │       │             │              │              │         │
 │       └──────┬──────┘──────┬───────┘──────┬───────┘         │
 │              │   ldst_unit │              │                 │
 │              └─────────────┴──────────────┘                 │
 └─────────────────────────┬───────────────────────────────────┘
                           │  icnt_push() / icnt_pop()
                ┌──────────┴──────────┐
                │   Interconnect NoC   │
                │ (InterSim2 / Xbar)   │
                └──────────┬──────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
 ┌──────┴──────┐  ┌───────┴──────┐  ┌───────┴──────┐
 │ Mem Part 0  │  │ Mem Part 1   │  │ Mem Part N   │
 │ ┌─────────┐ │  │ ┌─────────┐  │  │              │
 │ │Sub Part │ │  │ │Sub Part  │  │  │    ...       │
 │ │┌──────┐ │ │  │ │┌──────┐  │  │  │              │
 │ ││  L2  │ │ │  │ ││  L2  │  │  │  │              │
 │ ││ Cache│ │ │  │ ││ Cache│  │  │  │              │
 │ │└──────┘ │ │  │ │└──────┘  │  │  │              │
 │ └─────────┘ │  │ └─────────┘  │  │              │
 │ ┌─────────┐ │  │ ┌─────────┐  │  │              │
 │ │  DRAM   │ │  │ │  DRAM    │  │  │              │
 │ │ (GDDR/  │ │  │ │ (GDDR/   │  │  │              │
 │ │  HBM)   │ │  │ │  HBM)    │  │  │              │
 │ └─────────┘ │  │ └─────────┘  │  │              │
 └─────────────┘  └──────────────┘  └──────────────┘
```

### 9.2 캐시 블록 상태 (cache_block_state)

GPGPU-Sim의 캐시 블록은 4가지 상태를 가진다:

| 상태 | 값 | 의미 | 퇴거 가능 | write-back 필요 |
|------|---|------|-----------|----------------|
| INVALID | 0 | 빈 슬롯, 데이터 없음 | - | 아니오 |
| RESERVED | 1 | MSHR이 하위 메모리에 요청을 보냈으나 응답 대기 중 | 불가 | 아니오 |
| VALID | 2 | 유효한 클린 데이터 (하위 메모리와 동일) | 가능 | 아니오 |
| MODIFIED | 3 | 수정된 더티 데이터 (하위 메모리와 다름) | 가능 | 예 |

캐시 접근 결과(cache_request_status)는 6가지:

| 상태 | 의미 |
|------|------|
| HIT | 캐시에 데이터가 있고 즉시 반환 가능 |
| HIT_RESERVED | 태그 일치하지만 데이터 미도착 (이전 미스의 응답 대기 중), MSHR에 병합 |
| MISS | 캐시에 데이터 없음, 하위 메모리로 요청 필요 |
| RESERVATION_FAIL | 미스 발생했지만 MSHR 가득 참/캐시 라인 할당 불가, 다음 사이클 재시도 |
| SECTOR_MISS | 라인은 존재하나 요청된 특정 섹터가 INVALID |
| MSHR_HIT | MSHR에 동일 블록 주소 기존 요청 있어 병합됨 (SECTOR_MISS의 특수 케이스) |

### 9.3 tag_array 클래스

`tag_array`는 세트-연관(set-associative) 캐시의 핵심 데이터 구조이다. 캐시 블록 포인터의 1차원 배열 `m_lines[]`를 `세트 × 연관도`로 인덱싱한다.

```
m_lines[set_index * m_assoc + way]  →  cache_block_t*
```

**probe() 함수 — 태그 비교와 교체 후보 선택:**

```cpp
enum cache_request_status tag_array::probe(new_addr_type addr, unsigned &idx,
                                           mem_access_sector_mask_t mask,
                                           bool is_write, ...) const {
  unsigned set_index = m_config.set_index(addr);  // 해싱 함수로 세트 결정
  new_addr_type tag = m_config.tag(addr);          // 블록 주소 = 태그

  for (unsigned way = 0; way < m_config.m_assoc; way++) {
    unsigned index = set_index * m_config.m_assoc + way;
    cache_block_t *line = m_lines[index];

    if (line->m_tag == tag) {                      // 태그 매치!
      if (line->get_status(mask) == RESERVED) return HIT_RESERVED;
      if (line->get_status(mask) == VALID)    return HIT;
      if (line->get_status(mask) == MODIFIED) {
        if (is_write || line->is_readable(mask)) return HIT;
        else return SECTOR_MISS;  // lazy-fetch-on-read에서 부분 쓰기 후 읽기
      }
      if (line->is_valid_line() && line->get_status(mask) == INVALID)
        return SECTOR_MISS;       // 라인 유효, 해당 섹터만 INVALID
    }
    // 교체 후보 선택: LRU는 last_access_time 최소, FIFO는 alloc_time 최소
  }
  if (all_reserved) return RESERVATION_FAIL;
  return MISS;
}
```

핵심 포인트:
- 태그는 전체 블록 주소를 사용한다 (`addr & ~(line_sz - 1)`). 복잡한 해싱 함수 때문에 서로 다른 인덱스가 같은 세트에 매핑될 수 있으므로, 전통적 태그(상위 비트만)로는 부족하다.
- dirty 라인 비율 제한 기능: `m_wr_percent` 설정으로 clean 라인만 우선 퇴거하여 write-back 트래픽을 줄일 수 있다.

**교체 정책:**

| 정책 | 비교 기준 | 코드 |
|------|----------|------|
| LRU | `get_last_access_time()` 최소 | HIT 시 타임스탬프 갱신 |
| FIFO | `get_alloc_time()` 최소 | 할당 시에만 타임스탬프 기록 |

### 9.4 MSHR (Miss Status Holding Register)

MSHR은 캐시 미스를 추적하고, 동일 블록에 대한 중복 미스를 병합(merge)하여 불필요한 메모리 요청을 방지하는 하드웨어 구조이다.

```
MSHR 테이블 구조:
  ┌─────────────────────────────────────────────┐
  │  tr1_hash_map<block_addr, mshr_entry>       │
  │                                             │
  │  block_addr_A → mshr_entry {                │
  │     m_list: [mf1, mf2, mf3]  ← 병합된 요청들│
  │     m_has_atomic: false                     │
  │  }                                          │
  │  block_addr_B → mshr_entry {                │
  │     m_list: [mf4]                           │
  │     m_has_atomic: true                      │
  │  }                                          │
  └─────────────────────────────────────────────┘
  m_current_response: [block_addr_A]  ← 처리 준비된 응답 큐
```

**핵심 연산:**

| 함수 | 동작 | 시점 |
|------|------|------|
| `probe(block_addr)` | 해당 블록에 대한 미스가 이미 추적 중인지 확인 | 미스 발생 시 |
| `full(block_addr)` | 새 요청 수용 가능 여부: 기존 엔트리면 `m_list.size() >= max_merged`, 새 엔트리면 `m_data.size() >= num_entries` | 미스 발생 시 |
| `add(block_addr, mf)` | 요청을 추가하거나 기존 엔트리에 병합. atomic이면 `m_has_atomic = true` | 미스 확정 시 |
| `mark_ready(block_addr)` | 하위 메모리 응답 도착 시 해당 엔트리를 `m_current_response`에 추가 | fill 시 |
| `next_access()` | `m_current_response` 선두 블록의 `m_list`에서 요청 하나를 꺼냄. 리스트가 비면 엔트리 삭제 | writeback 시 |
| `is_read_after_write_pending()` | 같은 블록에 Write 후 Read가 대기 중인지 확인 (RAW 의존성), 있으면 추가 쓰기 거부 | 쓰기 미스 시 |

**MSHR 히트 vs 미스 처리 흐름:**

```
send_read_request(addr, block_addr, cache_index, mf, ...):
  mshr_addr = config.mshr_addr(mf->get_addr())

  if (mshr_hit && mshr_avail):
    → MSHR에 병합 (add), MSHR_HIT 통계 기록
    → 중복 메모리 요청 방지!

  else if (!mshr_hit && mshr_avail && miss_queue에 공간 있음):
    → 새 MSHR 엔트리 생성 (add)
    → extra_mf_fields에 원래 주소/크기/인덱스 저장
    → mf의 주소를 mshr_addr로, 크기를 atom_sz로 변경
    → miss_queue에 추가 → 하위 메모리로 전송

  else: RESERVATION_FAIL
```

### 9.5 섹터 캐시 (sector_cache_block)

NVIDIA Volta 이후 GPU에서 사용하는 캐시 구조. 하나의 캐시 라인(128B)을 4개의 섹터(각 32B)로 분할하여 관리한다.

```
sector_cache_block 구조:
  ┌─────────────────────────────────────────────────────┐
  │ m_tag = 0xFF00  (블록 주소)                          │
  │ m_block_addr = 0xFF00                               │
  │                                                     │
  │ 섹터0 (32B)    섹터1 (32B)    섹터2 (32B)    섹터3 (32B)│
  │ ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐│
  │ │ VALID    │  │ MODIFIED │  │ INVALID  │  │ RESERVED ││
  │ │ readable │  │ readable │  │          │  │ 요청 중  ││
  │ └──────────┘  └──────────┘  └──────────┘  └──────────┘│
  └─────────────────────────────────────────────────────┘
```

**섹터 캐시의 장점:**
- 필요한 섹터만 가져오므로 메모리 대역폭 절약
- 하나의 태그로 4개 섹터를 관리하여 태그 스토리지 절약
- 부분 write-back: MODIFIED 섹터만 write-back

**핵심 메서드:**

| 메서드 | 동작 |
|--------|------|
| `allocate_line()` | 새 라인 할당: init()으로 전체 초기화 후 요청된 섹터만 RESERVED |
| `allocate_sector()` | 기존 유효 라인의 INVALID 섹터를 추가 할당 (SECTOR_MISS 시) |
| `fill(time, sector_mask, byte_mask)` | 특정 섹터만 RESERVED→VALID/MODIFIED로 전이 |
| `is_invalid_line()` | 모든 섹터가 INVALID일 때만 true |
| `is_reserved_line()` | 하나라도 RESERVED면 true (퇴거 불가) |
| `get_dirty_sector_mask()` | MODIFIED 섹터들의 비트마스크 반환 |
| `get_modified_size()` | 수정된 섹터 수 × SECTOR_SIZE(32B) |

### 9.6 cache_config 설정 문자열

캐시는 설정 문자열로 구성된다:

```
"S:64:128:6,L:B:m:W:L,A:32:8,8:0,32"
  │  │  │  │ │ │ │ │ │ │ │  │ │ │ └ 데이터 포트 폭 (32B)
  │  │  │  │ │ │ │ │ │ │ │  │ │ └ 결과 FIFO 크기 (0)
  │  │  │  │ │ │ │ │ │ │ │  │ └ 미스 큐 크기 (8)
  │  │  │  │ │ │ │ │ │ │ │  └ MSHR 최대 병합 (8)
  │  │  │  │ │ │ │ │ │ │ └ MSHR 엔트리 수 (32)
  │  │  │  │ │ │ │ │ │ └ MSHR 타입 (A=ASSOC)
  │  │  │  │ │ │ │ │ └ 세트 인덱스 함수 (L=Linear)
  │  │  │  │ │ │ │ └ 쓰기 할당 정책 (W=WRITE_ALLOCATE)
  │  │  │  │ │ │ └ 할당 정책 (m=ON_MISS)
  │  │  │  │ │ └ 쓰기 정책 (B=WRITE_BACK)
  │  │  │  │ └ 교체 정책 (L=LRU)
  │  │  │  └ 연관도 (6-way)
  │  │  └ 라인 크기 (128B)
  │  └ 세트 수 (64)
  └ 캐시 타입 (S=SECTOR)
```

### 9.7 baseline_cache::cycle() — 캐시 사이클 동작

매 사이클 호출되어 다음을 수행한다:

```cpp
void baseline_cache::cycle() {
  // 1. 미스 큐에서 하위 메모리로 요청 전송
  if (!m_miss_queue.empty()) {
    mem_fetch *mf = m_miss_queue.front();
    if (!m_memport->full(mf->size(), mf->get_is_write())) {
      m_miss_queue.pop_front();
      m_memport->push(mf);     // 인터커넥트 또는 L2_dram_queue로 전송
    }
  }
  // 2. 포트 사용률 통계 샘플링
  m_stats.sample_cache_port_utility(data_port_busy, fill_port_busy);
  // 3. 포트 대역폭 복원 (점유 사이클을 1 감소)
  m_bandwidth_management.replenish_port_bandwidth();
}
```

**baseline_cache::fill() — 하위 메모리 응답 처리:**

```cpp
void baseline_cache::fill(mem_fetch *mf, unsigned time) {
  // SECTOR_ASSOC: 모든 섹터 응답이 도착할 때까지 대기
  if (m_config.m_mshr_type == SECTOR_ASSOC) {
    e->second.pending_read--;
    if (e->second.pending_read > 0) { delete mf; return; }
    // 모든 섹터 도착 → 원본 mf로 교체
  }
  // extra_mf_fields에서 원래 주소/크기/인덱스 복원
  mf->set_data_size(e->second.m_data_size);
  mf->set_addr(e->second.m_addr);

  // ON_MISS: 이미 예약된 인덱스에 fill
  // ON_FILL: 주소 기반으로 라인 할당 후 fill
  m_tag_array->fill(...);

  // MSHR 엔트리를 준비 상태로 표시
  m_mshrs.mark_ready(e->second.m_block_addr, has_atomic);

  // atomic이면 블록을 MODIFIED로 표시
  if (has_atomic) {
    block->set_status(MODIFIED, mf->get_access_sector_mask());
  }
  m_bandwidth_management.use_fill_port(mf);  // fill 포트 대역폭 소비
}
```

### 9.8 data_cache — 읽기/쓰기 정책 조합

`data_cache`는 함수 포인터 기반으로 쓰기 정책을 선택한다. 생성자에서 설정에 따라 4개의 함수 포인터를 바인딩한다:

```
m_rd_hit  → rd_hit_base (항상 동일)
m_rd_miss → rd_miss_base (항상 동일)
m_wr_hit  → 설정에 따라 선택
m_wr_miss → 설정에 따라 선택
```

#### 쓰기 히트(Write-Hit) 정책

| 정책 | 함수 | 동작 | 트레이드오프 |
|------|------|------|------------|
| Write-Back | `wr_hit_wb()` | 블록을 MODIFIED로 표시, LRU 갱신, 하위 메모리에 쓰지 않음 | 대역폭 절약, 퇴거 시 write-back 비용 |
| Write-Through | `wr_hit_wt()` | MODIFIED 표시 + 즉시 하위 메모리에 쓰기 전송 | 데이터 일관성 좋음, 대역폭 소비 큼 |
| Write-Evict | `wr_hit_we()` | 블록 INVALID로 무효화 + 하위 메모리에 쓰기 전송 | 캐시 오염 방지, 쓰기가 많은 경우 적합 |
| Local-WB/Global-WE | `wr_hit_global_we_local_wb()` | 글로벌 메모리→write-evict, 로컬 메모리→write-back | Fermi L1 정책 |

#### 쓰기 미스(Write-Miss) 할당 정책

| 정책 | 함수 | 동작 | 특징 |
|------|------|------|------|
| No Write-Allocate | `wr_miss_no_wa()` | 캐시에 할당하지 않고 바로 하위 메모리로 전송 | 단순, write-through/evict와 함께 사용 |
| Write-Allocate (naive) | `wr_miss_wa_naive()` | 쓰기+읽기 요청 모두 전송. 읽기로 블록을 가져와 캐시에 채움 | GPGPU-Sim 3.x 방식, 대역폭 2배 소비 |
| Fetch-on-Write | `wr_miss_wa_fetch_on_write()` | 읽기 요청만 보내고 fill 시 MODIFIED로 설정. 전체 섹터 쓰기면 읽기도 불필요 | Jouppi 1993, 대역폭 효율적 |
| Lazy-Fetch-on-Read | `wr_miss_wa_lazy_fetch_on_read()` | 쓰기만 하고 바로 MODIFIED로 설정. 나중에 읽기 미스 발생 시 그때 fetch | 쓰기만 하는 패턴에서 대역폭 최소 |

**Fetch-on-Write 상세 흐름:**

```
1. 전체 섹터 쓰기인 경우 (byte_mask.count() == atom_sz):
   → tag_array::access()로 라인 할당
   → 즉시 MODIFIED + fill
   → 읽기 요청 불필요 (모든 바이트를 쓰므로)

2. 부분 쓰기인 경우:
   → tag_array::access()로 라인 할당
   → set_modified_on_fill(true)  → fill 시 MODIFIED로 전이
   → set_byte_mask_on_fill(true) → fill 시 바이트 마스크 적용
   → send_read_request()로 읽기 요청 전송
```

**Lazy-Fetch-on-Read 상세 흐름:**

```
1. tag_array::access()로 라인 할당 (ON_MISS)
2. 즉시 MODIFIED로 설정 (메모리 요청 없이!)
3. m_readable = false (아직 전체 데이터가 없으므로 읽기 불가)
4. 바이트 마스크에 쓴 바이트만 기록
5. 나중에 같은 블록에 읽기 미스 발생 시:
   → probe()가 SECTOR_MISS 반환 (MODIFIED이지만 readable이 false)
   → 그때 읽기 요청을 보내 전체 데이터를 가져옴
   → fill 시 readable = true로 전환
```

### 9.9 포트 경합 (data_port, fill_port)

캐시는 두 개의 독립적인 포트를 가진다:

| 포트 | 용도 | 점유 사이클 계산 |
|------|------|-----------------|
| data_port | HIT 시 데이터 읽기, write-back 시 퇴거 블록 읽기 | HIT: `data_size / port_width`, WB: `modified_size / port_width` |
| fill_port | 하위 메모리 응답 수신 | `atom_sz / port_width` |

매 사이클 `replenish_port_bandwidth()`가 점유 사이클을 1씩 감소시킨다. 점유 사이클이 0이면 해당 포트를 사용할 수 있다.

### 9.10 L1 Instruction Cache

`read_only_cache` 클래스로 구현. 쓰기가 없으므로 write-back/write-through 로직이 불필요하다. 미스 시 `send_read_request()`만 호출한다.

### 9.11 L1 Constant Cache

마찬가지로 `read_only_cache`로 구현. 상수 메모리(`const_space`)와 커널 파라미터(`param_space_kernel`)에 대한 읽기 전용 접근을 처리한다. 워프 내 모든 스레드가 같은 주소에 접근하면 브로드캐스트로 1회 접근만 발생한다.

### 9.12 L1 Texture Cache

`tex_cache` 클래스로 별도 구현. 일반 캐시와 달리 HIT를 절대 반환하지 않는다. 모든 요청이 Fragment FIFO를 통과해야 결과를 받을 수 있다:

```
접근 요청 → Fragment FIFO → [태그 검사]
  HIT  → ROB에 ready 표시
  MISS → Request FIFO → 하위 메모리 → fill → ROB에 ready 표시
  ROB에서 순서대로 → Result FIFO → 상위에 전달
```

이유: 텍스처 필터링(bilinear/trilinear)이 여러 텍셀을 순서대로 가져와야 하므로, FIFO 기반 파이프라인으로 결과 순서를 보장한다.

### 9.13 세트 인덱스 해싱 함수

| 함수 | 설정 문자 | 특징 |
|------|----------|------|
| LINEAR | L | `(addr >> line_sz_log2) & (nset-1)`. 가장 단순 |
| FERMI_HASH | H | Nugteren et al. HPCA 2014. 비트 7-11과 비트 13,14,15,17,19를 XOR. 32/64세트만 지원 |
| BITWISE_XOR | X | 상위 비트와 하위 비트를 XOR. 세트 충돌 감소 |
| IPOLY | P | GF(2) 다항식 기반. 가장 균등한 분산 |
| CUSTOM | C | 사용자 정의 (미구현) |

---

## 10. 메모리 접근 코얼레싱 (Memory Coalescing)

### 10.1 코얼레싱이란?

워프 내 32개 스레드의 메모리 접근을 최소한의 메모리 트랜잭션으로 병합하는 최적화이다. 코얼레싱이 잘 되면 32스레드의 접근이 1개 트랜잭션으로 합쳐지고, 안 되면 최대 32개 트랜잭션이 필요하다.

### 10.2 memory_coalescing_arch() 알고리즘 상세

`abstract_hardware_model.cc`의 `warp_inst_t::memory_coalescing_arch()`에 구현된다.

**세그먼트 크기 결정:**

| 아키텍처 | 조건 | 세그먼트 크기 (1B/2B/4B+) |
|----------|------|--------------------------|
| Fermi/Kepler (CC 2.x-3.x) | L1 사용 | 32B / 64B / 128B |
| Fermi/Kepler (CC 2.x-3.x) | L1 우회 | 32B / 32B / 32B |
| Maxwell+ (CC 4.0+) | 항상 | 32B / 32B / 32B |

**서브워프 처리:**

워프를 `warp_parts`개의 서브워프(기본 2, 각 16스레드)로 나누어 독립적으로 코얼레싱한다.

**알고리즘 핵심:**

```
for each 서브워프:
  1단계: 각 활성 스레드의 주소 → 세그먼트 시작 주소로 매핑
         같은 세그먼트의 스레드들을 transaction_info에 병합
         - chunks: 128B 내 어떤 32B 청크가 사용되는지 (4비트)
         - active: 참여 스레드 마스크 (32비트)
         - bytes: 접근되는 바이트 위치 (128비트)

  2단계: 각 트랜잭션의 크기 축소
         memory_coalescing_arch_reduce_and_send() 호출
```

### 10.3 memory_coalescing_arch_reduce_and_send() — 트랜잭션 크기 축소

128B → 64B → 32B로 단계적 축소. 사용되는 32B 청크의 분포를 분석한다:

```
128B 세그먼트의 4개 32B 청크: q[0] q[1] q[2] q[3]

128B → 64B 축소:
  하위만 사용 (q[0]||q[1] && !q[2]&&!q[3]) → 64B, 주소 유지
  상위만 사용 (!q[0]&&!q[1] && q[2]||q[3]) → 64B, 주소 +64

64B → 32B 축소:
  하위만 사용 (h[0] && !h[1]) → 32B, 주소 유지
  상위만 사용 (!h[0] && h[1]) → 32B, 주소 +32
```

### 10.4 코얼레싱 예시

```
◆ 연속 접근 (완벽 코얼레싱):
  Thread 0: addr 0x1000  Thread 1: addr 0x1004  ...  Thread 31: addr 0x107C
  → 모두 같은 128B 세그먼트 [0x1000, 0x1080)
  → 1개 트랜잭션 (128B)

◆ 스트라이드 접근 (stride=256B):
  Thread 0: addr 0x1000  Thread 1: addr 0x1100  ...
  → 각 스레드가 다른 128B 세그먼트
  → 32개 트랜잭션 (각 32B로 축소 가능)

◆ 섹터 모드 연속 접근 (4B, Maxwell+):
  Thread 0-7:   세그먼트 [0x1000, 0x1020) → 1개 트랜잭션 (32B)
  Thread 8-15:  세그먼트 [0x1020, 0x1040) → 1개 트랜잭션 (32B)
  Thread 16-23: 세그먼트 [0x1040, 0x1060) → 1개 트랜잭션 (32B)
  Thread 24-31: 세그먼트 [0x1060, 0x1080) → 1개 트랜잭션 (32B)
  → 총 4개 트랜잭션
```

### 10.5 Shared Memory 뱅크 충돌 모델링

`generate_mem_accesses()`에서 shared memory 접근 시 뱅크 충돌을 시뮬레이션한다:

```
서브워프(16스레드)별로:
  1. 각 스레드의 주소 → 뱅크 번호 계산 (addr / 4 % num_banks)
  2. 같은 뱅크에서 같은 워드에 접근하면 브로드캐스트 (1회로 처리)
  3. 같은 뱅크에서 다른 워드에 접근하면 충돌
  4. max_bank_accesses = 가장 많이 충돌한 뱅크의 고유 워드 수
  5. total_accesses += max_bank_accesses (initiation interval로 모델링)
```

---

## 11. Load/Store 유닛 (ldst_unit)

### 11.1 ldst_unit 구조

`ldst_unit`은 SM 내에서 메모리 명령어를 처리하는 파이프라인 유닛이다:

```
ldst_unit 내부 구조:
  ┌──────────────────────────────────────┐
  │ m_dispatch_reg: 현재 처리 중인 명령어  │
  │                                      │
  │ m_L1D:  L1 Data Cache (l1_cache*)    │
  │ m_L1C:  L1 Constant Cache            │
  │ m_L1T:  L1 Texture Cache             │
  │                                      │
  │ m_response_fifo: 하위 메모리 응답 FIFO │
  │ m_pending_writes: warp별 대기 쓰기 수  │
  │ m_next_global: L1 우회 시 대기 응답    │
  │                                      │
  │ l1_latency_queue: L1D 뱅크별 지연 큐  │
  │ m_pipeline_reg[]: 공유 메모리 파이프라인│
  └──────────────────────────────────────┘
```

### 11.2 ldst_unit::cycle() — 메인 사이클

매 사이클 호출되며 다음 순서로 처리한다:

```
1. writeback(): 완료된 메모리 요청을 파이프라인에 반환
2. 파이프라인 레지스터 전진 (bubble 제거)
3. response_fifo 처리: 하위 메모리 응답을 적절한 캐시에 fill
   - TEXTURE_ACC_R → m_L1T->fill()
   - CONST_ACC_R   → m_L1C->fill()
   - WRITE_ACK     → store_ack()
   - 읽기 응답:
     - L1 우회 → m_next_global에 저장
     - L1 사용 → m_L1D->fill()
4. 각 캐시의 cycle() 호출: m_L1T->cycle(), m_L1C->cycle(), m_L1D->cycle()
5. L1D 뱅크 지연 큐 처리: L1_latency_queue_cycle()
6. 메모리 공간별 접근 처리:
   - shared_cycle(): Shared memory
   - constant_cycle(): Constant cache
   - texture_cycle(): Texture cache
   - memory_cycle(): L1D / 글로벌 메모리
```

### 11.3 Pending Writes 추적

로드 명령어 이슈 시 `m_pending_writes[warp_id][reg_id] += n_accesses`로 미완료 메모리 접근 수를 기록한다. 각 메모리 응답이 돌아올 때마다 카운터를 감소시키고, 0이 되면 명령어 완료 및 스코어보드 해제한다.

### 11.4 Writeback 클라이언트

`m_num_writeback_clients = 5`로 5개 클라이언트가 라운드로빈으로 writeback 포트를 공유한다:
1. Shared memory
2. Global/Local (uncached)
3. L1D
4. L1T
5. L1C

---

## 12. L2 캐시 및 메모리 파티션

### 12.1 memory_partition_unit 구조

하나의 DRAM 채널에 연결된 메모리 파티션 유닛이다:

```
memory_partition_unit
  ├── memory_sub_partition[0]  ─┐
  ├── memory_sub_partition[1]  ─┤── 서브 파티션들
  ├── ...                      ─┘
  ├── dram_t (DRAM 모델)
  ├── arbitration_metadata (크레딧 기반 중재)
  └── m_dram_latency_queue (고정 레이턴시 큐)
```

### 12.2 memory_sub_partition 내부 데이터 흐름

```
                   ┌─────────────────────────────────────────────────┐
                   │         memory_sub_partition                     │
                   │                                                 │
  인터커넥트 ──→   │  [ROP delay] → [icnt_L2_queue] → L2 캐시 접근   │
  (push)           │                                   │    │        │
                   │                               HIT │  MISS│     │
                   │                                   ↓    ↓       │
                   │                     [L2_icnt_queue]  [L2_dram_queue]│
                   │                           ↑               │       │
                   │                           │               ↓       │
  인터커넥트 ←──   │             L2 fill ← [dram_L2_queue] ← DRAM    │
  (pop/top)        │                                                 │
                   └─────────────────────────────────────────────────┘
```

**FIFO 큐 크기 설정 형식:** `"icnt_L2:L2_dram:dram_L2:L2_icnt"`

### 12.3 mem_fetch 클래스

메모리 계층을 흐르는 "메모리 요청 패킷" 객체이다:

```
mem_fetch 주요 필드:
  m_request_uid    — 전역 고유 요청 ID (단조 증가)
  m_sid, m_tpc, m_wid — 요청 출처 (SM, TPC, warp)
  m_access         — 메모리 접근 정보 (주소, 크기, 타입, 마스크)
  m_data_size      — 데이터 페이로드 크기
  m_ctrl_size      — 제어 메타데이터 크기
  m_partition_addr  — DRAM 파티션 내부 선형 주소
  m_raw_addr       — 디코딩된 DRAM 주소 (chip, bank, row, col)
  m_type           — READ_REQUEST / WRITE_REQUEST / READ_REPLY / WRITE_ACK
  m_status         — 현재 위치 (MEM_FETCH_INITIALIZED → IN_L1D_MISS_QUEUE → ...)
  m_timestamp      — 생성 시각 (전체 레이턴시 측정 기준)
  original_mf      — L2 섹터 분할 시 원본 요청 포인터
  original_wr_mf   — fetch-on-write 시 원본 쓰기 요청 포인터
```

**Flit 수 계산:** `get_num_flits(bool simt_to_mem)`

```
SM→메모리 방향: 읽기 요청=ctrl만, 쓰기 요청=ctrl+data
메모리→SM 방향: 읽기 응답=ctrl+data, 쓰기 ACK=ctrl만
atomic: 양방향 모두 ctrl+data
flit 수 = ceil(패킷_크기 / icnt_flit_size)
```

### 12.4 L1 miss → L2 → DRAM 전체 흐름

```
1. SM에서 로드 명령어 실행 → L1D 미스
2. MSHR에 등록, miss_queue에 추가
3. baseline_cache::cycle()에서 m_memport->push(mf) → 인터커넥트에 주입
4. 인터커넥트 전송 (수 사이클)
5. memory_sub_partition::push() → ROP delay → icnt_L2_queue
6. memory_sub_partition::cache_cycle():
   a. icnt_L2_queue에서 요청 꺼냄
   b. L2 캐시 접근 (l2_cache::access())
      - HIT → L2_icnt_queue에 응답 추가
      - MISS → L2_dram_queue에 DRAM 요청 추가, MSHR 등록
7. memory_partition_unit::dram_cycle():
   a. L2_dram_queue에서 요청을 DRAM 레이턴시 큐로 이동
   b. 레이턴시 경과 후 dram_t::push()
8. DRAM 처리 (수십~수백 사이클)
9. DRAM 응답 → dram_L2_queue
10. cache_cycle()에서 dram_L2_queue 처리 → L2 fill → MSHR mark_ready
11. L2_icnt_queue에 응답 추가
12. 인터커넥트 역방향 전송
13. SM의 response_fifo에 도착 → L1D fill / writeback
```

### 12.5 Write-back 버퍼와 Atomic 처리

**Dirty eviction:** L2 캐시에서 dirty 라인이 퇴거되면 `L2_WRBK_ACC` 타입의 write-back 요청이 생성되어 DRAM에 기록된다. DRAM에서 처리 완료 후 `set_done()`으로 표시하고 mem_fetch를 삭제한다 (응답 불필요).

**Atomic 연산 경로:** L2 미스 → DRAM에서 데이터 가져옴 → fill 시 `has_atomic` 확인 → 블록을 MODIFIED로 표시 → MSHR에서 next_access()로 꺼낼 때 `do_atomic()` 호출하여 실제 원자적 연산 수행.

---

## 13. DRAM 시뮬레이션

### 13.1 DRAM 구조 다이어그램

```
DRAM 채널 (dram_t)
  ├── Bank Group 0 (bankgrp_t)
  │   ├── Bank 0 (bank_t)
  │   │   ├── Row Buffer (Sense Amplifier)
  │   │   │   └── curr_row: 현재 열린 Row
  │   │   ├── state: IDLE/ACTIVE
  │   │   └── 타이밍 카운터: RCDc, RASc, RPc, RCc, WTPc, RTPc
  │   ├── Bank 1
  │   ├── Bank 2
  │   └── Bank 3
  ├── Bank Group 1
  │   ├── Bank 4 ~ Bank 7
  ├── Bank Group 2
  │   ├── Bank 8 ~ Bank 11
  └── Bank Group 3
      ├── Bank 12 ~ Bank 15

  글로벌 타이밍: RRDc, CCDc, RTWc, WTRc

  내부 큐:
    mrqq (2 엔트리) → 스케줄러 → bk[].mrq → rwq (CL 파이프라인) → returnq
```

### 13.2 타이밍 파라미터 상세

| 파라미터 | 의미 | 영향 |
|----------|------|------|
| tRCD | Row-to-Column Delay: Activate 후 Read/Write까지 대기 | Row Buffer Miss의 레이턴시 증가 |
| tRAS | Row Active Strobe: Row가 활성 유지 최소 시간 | Precharge 전 최소 대기 |
| tRP | Row Precharge: Bank 닫기 시간 | Row Conflict의 추가 레이턴시 |
| tRC | Row Cycle: 같은 Bank 연속 Activate 간격 (≈ tRAS+tRP) | 같은 Bank 접근 빈도 제한 |
| tCCD | Column-to-Column Delay: 연속 Read/Write 간격 (칩 수준) | 데이터 버스 공유 제약 |
| tCCDL | 같은 Bank Group 내 연속 Column 명령 간격 (tCCD보다 김) | Bank Group 내 병렬성 제한 |
| tRRD | Row-to-Row Delay: 다른 Bank 연속 Activate 간격 | Bank-Level Parallelism 제한 |
| tRTW | Read-to-Write 전환 페널티 | 버스 턴어라운드 |
| tWTR | Write-to-Read 전환 페널티 | Write 데이터 안정화 대기 |
| CL | CAS Latency: Read 명령 후 데이터 출현까지 시간 | 읽기 레이턴시의 핵심 |
| WL | Write Latency: Write 명령 후 데이터 전달까지 시간 | 쓰기 레이턴시 |
| BL | Burst Length: 한 번에 전송하는 데이터 비트 수 | 대역폭 결정 |

### 13.3 Row Buffer 관리

```
Row Buffer 상태 전이:
  IDLE ──(Activate)──→ ACTIVE ──(Precharge)──→ IDLE
    │                    │
    │                    ├── Row Hit: 같은 Row에 접근 → Column 명령만 (tCCD 대기)
    │                    └── Row Conflict: 다른 Row에 접근 → Precharge + Activate + Column
    │
    └── Row Miss (Empty): Bank가 IDLE → Activate + Column (tRCD 대기)
```

### 13.4 dram_t::cycle() — DRAM 사이클 처리

```cpp
void dram_t::cycle() {
  // 1단계: rwq에서 전송 완료 데이터 → returnq
  cmd = rwq->pop();
  if (cmd) {
    cmd->dqbytes += dram_atom_size;
    if (cmd->dqbytes >= cmd->nbytes) {
      // 전송 완료: L1/L2 writeback이면 삭제, 아니면 returnq에 push
    }
  }

  // 2단계: 스케줄러 호출 (FIFO 또는 FR-FCFS)
  scheduler_fifo() / scheduler_frfcfs();

  // 3단계: 모든 Bank를 순회하며 명령 발행
  for (j = 0; j < nbk; j++) {
    if (bk[j]->mrq) {
      issue_col_command(j);  // Column 명령 시도 (Row Hit 시)
      issue_row_command(j);  // Row 명령 시도 (Activate/Precharge)
    }
  }

  // 4단계: BLP, Row Buffer Locality 통계 수집
  // 5단계: 모든 타이밍 카운터를 DEC2ZERO로 1씩 감소
  for (j = 0; j < nbk; j++) {
    DEC2ZERO(bk[j]->RCDc); DEC2ZERO(bk[j]->RASc);
    DEC2ZERO(bk[j]->RPc);  DEC2ZERO(bk[j]->RCc);
    // ...
  }
  DEC2ZERO(RRDc); DEC2ZERO(CCDc); DEC2ZERO(RTWc); DEC2ZERO(WTRc);
}
```

### 13.5 DRAM 스케줄링 정책

#### FIFO

가장 단순한 정책. `mrqq`에서 순서대로 꺼내 해당 Bank가 비어있으면 할당한다. Row Buffer Locality를 고려하지 않으므로 성능이 낮다.

#### FR-FCFS (First-Ready First-Come First-Served)

Row Buffer Hit 요청을 우선 처리하여 DRAM 대역폭 활용을 극대화한다.

```
schedule(bank, curr_row):
  1. Read/Write 모드 전환 (separate write queue 사용 시):
     - Write 큐 ≥ high_watermark → WRITE_MODE (Write drain)
     - Write 큐 < low_watermark  → READ_MODE

  2. m_last_row[bank] == NULL이면 새 Row 선택:
     a. m_bins[bank]에서 curr_row 검색
        - 있으면: Row Buffer Hit! → 해당 Row의 요청 리스트 선택
        - 없으면: Row Buffer Miss → 큐에서 가장 오래된 요청의 Row 선택 (FCFS)

  3. 선택된 Row의 가장 오래된 요청(back)을 반환

  4. 해당 Row의 요청이 모두 소진되면 m_last_row = NULL (다음에 새 Row 선택)
```

**FR-FCFS 자료구조:**

```
Bank별:
  m_queue[bank]:   요청 리스트 (도착 순서). push_front, oldest=back()
  m_bins[bank]:    Row→이터레이터 리스트 맵 (Row Hit 검색 O(log n))
  m_last_row[bank]: 현재 서비스 중인 Row의 이터레이터 리스트 포인터
```

**별도 Write 큐의 동작:**

Write와 Read를 분리하여 Read→Write, Write→Read 전환 횟수를 최소화한다. Write 큐가 high watermark에 도달하면 WRITE_MODE로 전환하여 Write를 집중 처리(drain)하고, low watermark 이하로 떨어지면 다시 READ_MODE로 복귀한다.

### 13.6 Bank 인덱싱 (주소→Bank 매핑)

| 정책 | 설명 |
|------|------|
| LINEAR_BK_INDEX | 주소에서 추출된 Bank 비트를 그대로 사용 |
| BITWISE_XORING_BK_INDEX | Row 하위 비트와 Bank 비트를 XOR하여 분산 |
| IPOLY_BK_INDEX | GF(2) 다항식 해시로 Bank 분산 (가장 균등) |

---

## 14. 인터커넥트 네트워크 (NoC)

### 14.1 GPU NoC의 역할

SM과 메모리 파티션 간 데이터 전송을 담당한다. 함수 포인터 기반 전략 패턴으로 두 가지 구현을 런타임에 교체할 수 있다:

```
icnt_wrapper.h의 함수 포인터들:
  icnt_create, icnt_init, icnt_has_buffer, icnt_push, icnt_pop,
  icnt_transfer, icnt_busy, icnt_drain, ...

g_network_mode = INTERSIM(1) → InterSim2 래퍼 함수 바인딩
g_network_mode = LOCAL_XBAR(2) → LocalInterconnect 래퍼 함수 바인딩
```

### 14.2 InterSim2 (상세 NoC)

BookSim에서 파생된 상세 NoC 시뮬레이터이다.

**Flit 기반 라우팅:**
- 패킷은 여러 flit(flow control unit)으로 분할된다
- Head flit: 라우팅 정보 포함, Virtual Channel 할당
- Body flit: 데이터 페이로드
- Tail flit: 패킷 끝 표시, 리소스 해제

**Virtual Channels (VC):**
- 데드락 방지를 위해 각 물리 채널을 여러 가상 채널로 분할
- 패킷이 블록되어도 같은 물리 링크의 다른 VC를 통해 다른 패킷 전달 가능

**Credit 기반 흐름 제어:**
- 다운스트림 라우터가 크레딧을 보내 버퍼 가용 공간을 알림
- 업스트림 라우터는 크레딧이 있을 때만 flit 전송

**IQ Router 5단계 파이프라인:**

| 단계 | 약자 | 동작 |
|------|------|------|
| Route Computation | RC | Head flit의 목적지에 따라 출력 포트 결정 |
| VC Allocation | VA | 출력 포트의 가상 채널 할당 |
| Switch Allocation | SA | 크로스바 스위치 사용권 중재 |
| Switch Traversal | ST | 크로스바 스위치를 통해 flit 이동 |
| Link Traversal | LT | 다음 라우터로 링크를 통해 flit 전송 |

**지원 토폴로지:** mesh, torus, butterfly, fat-tree, dragonfly 등

### 14.3 LocalInterconnect (크로스바)

단순한 입력 큐 스위치(IQ switch) 크로스바를 모델링한다. InterSim2보다 훨씬 빠르지만 홉 지연, 라우팅 지연을 모델링하지 않는다.

```
구조:
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │ in_buf[0]│──┐   │          │  ┌──→│out_buf[0]│
  │ in_buf[1]│──┤──→│ Arbiter  │──┤──→│out_buf[1]│
  │ ...      │──┤   │ (RR/iSLIP│  ├──→│...       │
  │ in_buf[N]│──┘   │          │  └──→│out_buf[M]│
  └──────────┘      └──────────┘      └──────────┘
```

**Request/Reply 서브넷 분리:**

프로토콜 데드락 방지를 위해 요청과 응답을 별도 크로스바로 분리한다:

```
net[0] = xbar_router(REQ_NET):  SM→메모리 (요청)
  active_in = n_shader, active_out = n_mem

net[1] = xbar_router(REPLY_NET): 메모리→SM (응답)
  active_in = n_mem, active_out = n_shader
```

#### 중재 알고리즘

**Naive Round-Robin:**
```
for i in range(total_nodes):
  node_id = (i + next_node_id) % total_nodes
  if in_buffers[node_id] has packet:
    packet = front of queue
    if out_buffer has space AND not already issued to this output:
      transfer packet
      issued[output] = true
    else: conflict++
next_node_id = (next_node_id + 1) % total_nodes
```

한계: Head-of-Line (HOL) blocking — 선두 패킷이 차단되면 뒤의 패킷도 모두 대기

**iSLIP:**
각 출력 포트마다 독립적인 라운드로빈 포인터(`next_node[i]`)를 유지한다. HOL blocking을 완화하고 높은 처리량을 달성한다. `grant_cycles` 파라미터로 그랜트 포인터 업데이트 주기를 조절할 수 있다.

### 14.4 트래픽 분류

인터커넥트를 통과하는 메모리 접근 타입:

| 타입 | 방향 | 설명 |
|------|------|------|
| GLOBAL_ACC_R | SM→Mem | 글로벌 메모리 읽기 |
| GLOBAL_ACC_W | SM→Mem | 글로벌 메모리 쓰기 |
| TEXTURE_ACC_R | SM→Mem | 텍스처 메모리 읽기 |
| CONST_ACC_R | SM→Mem | 상수 메모리 읽기 |
| L1_WRBK_ACC | SM→Mem | L1 dirty 라인 write-back |
| L2_WRBK_ACC | Mem(L2)→DRAM | L2 dirty 라인 write-back |
| L1_WR_ALLOC_R | SM→Mem | L1 write-allocate 읽기 |
| L2_WR_ALLOC_R | Mem(L2)→DRAM | L2 write-allocate 읽기 |

---

## 15. 주소 디코딩 및 해싱

### 15.1 물리 주소 분해

`linear_to_raw_address_translation::addrdec_tlx()`가 선형 주소를 DRAM 구성요소로 분해한다:

```
선형 주소 (64비트):
  [...row bits...][...bank bits...][...col bits...][...burst bits...]
                  ↑ chip bits가 여기에 삽입됨 (ADDR_CHIP_S 위치)

분해 결과 (addrdec_t):
  chip          — 메모리 채널(파티션) ID
  sub_partition — 채널 내 서브 파티션 ID (L2 슬라이스)
  bk            — Bank ID
  row           — Row 주소
  col           — Column 주소
  burst         — Burst 오프셋
```

**비트마스크 기반 주소 필드 추출:**

```
addrdec_mask[CHIP]   = 0x...   (chip 비트 위치를 나타내는 마스크)
addrdec_mask[BK]     = 0x...   (bank 비트 위치)
addrdec_mask[ROW]    = 0x...   (row 비트 위치)
addrdec_mask[COL]    = 0x...   (column 비트 위치)
addrdec_mask[BURST]  = 0x...   (burst 비트 위치)
```

각 마스크를 주소에 AND 연산 후 비트를 압축(squeeze)하여 해당 필드의 값을 추출한다.

**partition_address():** 선형 주소에서 채널 선택 비트를 제거한 파티션 내부 주소를 반환한다. L2 캐시의 세트 인덱스 계산에 사용하여 세트 캠핑(특정 세트에 접근 집중)을 방지한다.

### 15.2 해싱 알고리즘

#### IPOLY (Irreducible Polynomial)

GF(2) 체 위의 기약 다항식을 사용한 해싱이다. Rau (ISCA 1991) "Pseudo-randomly interleaved memory" 논문에서 제안되었다.

| 특성 | 설명 |
|------|------|
| 원리 | 상위 비트와 인덱스를 GF(2) 다항식으로 혼합 |
| 장점 | 모든 2^n stride에 대해 conflict-free 보장 |
| 지원 크기 | 16, 32, 64 |
| 하드웨어 비용 | XOR 게이트 체인 (낮음) |
| 사용처 | 캐시 세트 인덱싱, 메모리 파티션 인덱싱, Bank 인덱싱 |

#### Bitwise XOR

```
결과 = index XOR (higher_bits의 하위 log2(bank_set_num) 비트)
```

| 특성 | 설명 |
|------|------|
| 원리 | 상위 비트와 인덱스를 단순 XOR |
| 장점 | 구현이 매우 간단, 하드웨어 비용 최소 |
| 단점 | 특정 stride 패턴에서 편중 가능 (예: stride = 2 × bank_set_num이면 모든 접근이 같은 인덱스) |

#### PAE (Page Address Entropy)

페이지 주소의 엔트로피를 활용한 해싱이다. 페이지 비트와 Bank 비트를 혼합하여 주소 분산성을 높인다. 현재 32개 Bank만 지원한다.

### 15.3 메모리 파티션 인덱싱 함수

| 함수 | 설명 |
|------|------|
| CONSECUTIVE | 주소 비트를 그대로 채널 ID로 사용 (해싱 없음) |
| BITWISE_PERMUTATION | 비트별 XOR 순열로 분산 |
| IPOLY | GF(2) 다항식 기반 pseudo-random 인터리빙 |
| PAE | 페이지 주소 엔트로피 기반 |
| RANDOM | 소프트웨어 해시 테이블 기반 (실험용) |
| CUSTOM | 사용자 정의 |
