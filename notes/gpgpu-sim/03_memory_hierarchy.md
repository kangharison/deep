# GPGPU-Sim 메모리 계층 구조 심층 분석

## 소스 코드 위치
- `src/gpgpu-sim/gpu-cache.h / .cc` : 캐시 구현 (tag_array, mshr_table, baseline_cache, data_cache, l1_cache, l2_cache)
- `src/gpgpu-sim/l2cache.h / .cc` : L2 캐시 및 메모리 파티션 (memory_partition_unit, memory_sub_partition)
- `src/gpgpu-sim/dram.h / .cc` : DRAM 컨트롤러 (dram_t, bank_t, dram_req_t)
- `src/gpgpu-sim/dram_sched.h / .cc` : DRAM 스케줄러 (frfcfs_scheduler)
- `src/gpgpu-sim/mem_fetch.h / .cc` : 메모리 요청 패킷 (mem_fetch)
- `src/gpgpu-sim/addrdec.h / .cc` : 주소 디코딩 (linear_to_raw_address_translation)
- `src/gpgpu-sim/mem_latency_stat.h / .cc` : 메모리 통계 (memory_stats_t)
- `src/gpgpu-sim/delayqueue.h` : 지연 큐 (fifo_pipeline)

---

## 1. GPU 메모리 계층 구조 전체 아키텍처

```
 ┌─────────────────────────────────────────────────────────────┐
 │                     SM (Shader Core) #0                     │
 │  ┌───────────┐  ┌───────────┐  ┌───────────┐               │
 │  │ L1 Data   │  │ L1 Tex    │  │ L1 Const  │               │
 │  │ Cache     │  │ Cache     │  │ Cache     │               │
 │  │(l1_cache) │  │(tex_cache)│  │(read_only │               │
 │  │           │  │           │  │  _cache)  │               │
 │  └─────┬─────┘  └─────┬─────┘  └─────┬─────┘               │
 │        │              │              │                      │
 │        └──────────────┼──────────────┘                      │
 │                       │                                     │
 └───────────────────────┼─────────────────────────────────────┘
                         │
              ┌──────────┴──────────┐
              │ Interconnection     │
              │ Network (ICNT)      │
              └──────────┬──────────┘
                         │
     ┌───────────────────┼───────────────────┐
     │                   │                   │
 ┌───┴────────────┐ ┌───┴────────────┐ ┌───┴────────────┐
 │ Memory         │ │ Memory         │ │ Memory         │
 │ Partition #0   │ │ Partition #1   │ │ Partition #N   │
 │                │ │                │ │                │
 │ ┌────────────┐ │ │ ┌────────────┐ │ │ ┌────────────┐ │
 │ │Sub-Part #0 │ │ │ │Sub-Part #2 │ │ │ │Sub-Part #M │ │
 │ │┌──────────┐│ │ │ │┌──────────┐│ │ │ │┌──────────┐│ │
 │ ││  L2 $    ││ │ │ ││  L2 $    ││ │ │ ││  L2 $    ││ │
 │ ││(l2_cache)││ │ │ ││(l2_cache)││ │ │ ││(l2_cache)││ │
 │ │└──────────┘│ │ │ │└──────────┘│ │ │ │└──────────┘│ │
 │ └────────────┘ │ │ └────────────┘ │ │ └────────────┘ │
 │ ┌────────────┐ │ │ ┌────────────┐ │ │ ┌────────────┐ │
 │ │Sub-Part #1 │ │ │ │Sub-Part #3 │ │ │ │Sub-Part #M+1│ │
 │ │┌──────────┐│ │ │ │┌──────────┐│ │ │ │┌──────────┐│ │
 │ ││  L2 $    ││ │ │ ││  L2 $    ││ │ │ ││  L2 $    ││ │
 │ │└──────────┘│ │ │ │└──────────┘│ │ │ │└──────────┘│ │
 │ └────────────┘ │ │ └────────────┘ │ │ └────────────┘ │
 │                │ │                │ │                │
 │ ┌────────────┐ │ │ ┌────────────┐ │ │ ┌────────────┐ │
 │ │  DRAM      │ │ │ │  DRAM      │ │ │ │  DRAM      │ │
 │ │ Controller │ │ │ │ Controller │ │ │ │ Controller │ │
 │ │ (dram_t)   │ │ │ │ (dram_t)   │ │ │ │ (dram_t)   │ │
 │ └─────┬──────┘ │ │ └─────┬──────┘ │ │ └─────┬──────┘ │
 └───────┼────────┘ └───────┼────────┘ └───────┼────────┘
         │                  │                  │
    ┌────┴────┐        ┌────┴────┐        ┌────┴────┐
    │  GDDR5  │        │  GDDR5  │        │  GDDR5  │
    │  / HBM  │        │  / HBM  │        │  / HBM  │
    └─────────┘        └─────────┘        └─────────┘
```

### 핵심 구조 관계

- **memory_partition_unit**: 하나의 DRAM 채널에 대응하며, 여러 개의 `memory_sub_partition`과 하나의 `dram_t`를 포함한다.
- **memory_sub_partition**: 각각 독립된 L2 캐시 뱅크(`l2_cache`)를 가지며, ICNT와 DRAM 사이의 FIFO 큐들을 관리한다.
- **L1 캐시들**: 각 SM(Shader Core) 내부에 위치하며, 종류별로 다른 클래스를 사용한다 (`l1_cache`, `tex_cache`, `read_only_cache`).

---

## 2. 캐시 구현 상세

### 2.1 캐시 클래스 상속 계층

```
 cache_t (추상 인터페이스)
   │
   ├── baseline_cache (공통 기능: tag_array, MSHR, bandwidth)
   │     │
   │     ├── read_only_cache (L1 명령어/상수 캐시)
   │     │
   │     └── data_cache (읽기/쓰기 가능한 캐시)
   │           │
   │           ├── l1_cache (L1 데이터 캐시, write-evict/write-back)
   │           │
   │           └── l2_cache (L2 캐시, global write-back + write-allocate)
   │
   └── tex_cache (텍스처 캐시, FIFO 기반 별도 구현)
```

### 2.2 캐시 설정 파싱 (cache_config::init)

캐시 설정 문자열 형식: `<type>:<nset>:<line_sz>:<assoc>,<rp>:<wp>:<ap>:<wap>:<sif>,<mshr_type>:<mshr_entries>:<mshr_max_merge>,<miss_queue_size>:<result_fifo>,<data_port_width>`

| 필드 | 설명 | 값 |
|------|------|-----|
| type | 캐시 타입 | `N`=NORMAL, `S`=SECTOR |
| nset | 세트 수 | 숫자 |
| line_sz | 캐시라인 크기(byte) | 숫자 |
| assoc | 연관도 | 숫자 |
| rp | 교체 정책 | `L`=LRU, `F`=FIFO |
| wp | 쓰기 정책 | `R`=읽기전용, `B`=write-back, `T`=write-through, `E`=write-evict, `L`=local_wb_global_wt |
| ap | 할당 정책 | `m`=on-miss, `f`=on-fill, `s`=streaming |
| wap | 쓰기 할당 정책 | `N`=no-write-allocate, `W`=write-allocate, `F`=fetch-on-write, `L`=lazy-fetch-on-read |
| sif | 세트 인덱스 함수 | `L`=linear, `H`=Fermi hash, `P`=IPoly, `X`=bitwise XOR, `C`=custom |
| mshr_type | MSHR 타입 | `A`=ASSOC, `S`=SECTOR_ASSOC, `F`=TEX_FIFO, `T`=SECTOR_TEX_FIFO |

### 2.3 캐시 블록 상태 머신

```
    allocate()
 ┌─────────────────┐
 │                 ▼
 │   ┌──────────────────┐
 │   │     INVALID      │ ◄──── 초기 상태 / invalidate
 │   └──────────┬───────┘
 │              │ allocate() - tag_array::access()에서 ON_MISS일 때
 │              ▼
 │   ┌──────────────────┐
 │   │    RESERVED      │ ◄──── 하위 메모리에 요청 전송 완료, 응답 대기 중
 │   └──────────┬───────┘
 │              │ fill() - 하위 메모리에서 데이터 도착
 │              ▼
 │   ┌──────────────────┐         write hit (write-back)
 │   │      VALID       │ ──────────────────────────────┐
 │   └──────────────────┘                               │
 │                                                      ▼
 │                                           ┌──────────────────┐
 │                                           │    MODIFIED      │
 │                                           └──────────┬───────┘
 │                                                      │
 │   eviction (dirty) → write-back 요청 생성             │
 └──────────────────────────────────────────────────────┘
```

**cache_block_state 열거형** (gpu-cache.h:47):
- `INVALID` (0): 유효하지 않은 블록
- `RESERVED` (1): 할당되었지만 아직 fill되지 않음 (하위 메모리 응답 대기)
- `VALID` (2): 유효한 데이터가 있음 (clean)
- `MODIFIED` (3): 수정된 데이터가 있음 (dirty)

### 2.4 두 가지 캐시 블록 타입

#### line_cache_block (일반 캐시)
- 하나의 캐시라인 전체에 대해 단일 상태를 유지한다.
- `m_status`: 전체 라인의 상태 (INVALID/RESERVED/VALID/MODIFIED)
- `m_alloc_time`, `m_last_access_time`, `m_fill_time`: LRU/FIFO 교체를 위한 타임스탬프

#### sector_cache_block (섹터 캐시, Volta 이후)
- 하나의 캐시라인을 `SECTOR_CHUNCK_SIZE`(=4)개의 섹터로 분할하여, 각 섹터가 독립된 상태를 가진다.
- `m_status[SECTOR_CHUNCK_SIZE]`: 각 섹터별 상태
- 섹터 크기 = `SECTOR_SIZE` (32바이트), 캐시라인 = 128바이트 = 4 섹터
- `SECTOR_MISS`: 캐시라인은 존재하지만 요청된 섹터가 유효하지 않을 때 발생

### 2.5 교체 정책 (replacement_policy_t)

`tag_array::probe()` (gpu-cache.cc:246-333) 에서 교체 후보를 선택한다.

```
for (way = 0; way < m_assoc; way++) {
    // 1단계: 태그 매치 검사 → HIT, HIT_RESERVED, SECTOR_MISS 반환
    // 2단계: reserved가 아닌 라인 중 교체 후보 선택
    if (m_replacement_policy == LRU) {
        // 가장 오래 전에 접근된 라인 선택 (last_access_time 최소)
    } else if (m_replacement_policy == FIFO) {
        // 가장 먼저 할당된 라인 선택 (alloc_time 최소)
    }
}
```

추가로, **dirty line 비율 제한** 기능이 있다: `m_wr_percent` 설정값으로 dirty 라인의 비율이 일정 이상이 되어야만 dirty 라인을 교체 후보로 허용한다. 이는 write-back 캐시에서 과도한 writeback을 방지한다.

### 2.6 세트 인덱스 함수

`cache_config::hash_function()` (gpu-cache.cc:81-154):

| 함수 | 설명 |
|------|------|
| `LINEAR_SET_FUNCTION` | `(addr >> line_sz_log2) & (nset - 1)` - 가장 기본적인 인덱싱 |
| `FERMI_HASH_SET_FUNCTION` | Fermi GPU의 해싱 함수 재현. 비트 7-11과 비트 13,14,15,17,19를 XOR |
| `BITWISE_XORING_FUNCTION` | 상위 비트와 인덱스 비트를 bitwise XOR |
| `HASH_IPOLY_FUNCTION` | IPoly 해싱 (pseudo-random interleaving) |

L2 캐시의 세트 인덱스(`l2_cache_config::set_index`)는 메모리 파티션 비트를 제거한 `partition_address`를 사용하여 세트 캠핑(set camping) 문제를 줄인다.

### 2.7 MSHR (Miss Status Holding Register)

MSHR은 `mshr_table` 클래스로 구현되며 (gpu-cache.h:1024-1080), 하위 메모리에 보낸 미처리 요청(outstanding request)을 추적한다.

#### 핵심 자료 구조

```cpp
struct mshr_entry {
    std::list<mem_fetch *> m_list;  // 같은 블록에 대한 병합된 요청들
    bool m_has_atomic;               // atomic 연산 포함 여부
};

// block_addr → mshr_entry 매핑 (fully associative)
tr1_hash_map<new_addr_type, mshr_entry> m_data;

// fill 응답이 도착한 엔트리의 주소 목록
std::list<new_addr_type> m_current_response;
```

#### MSHR 동작 흐름

```
 ┌─────────────────────────────────────────────────────┐
 │                  캐시 미스 발생                       │
 └──────────────────┬──────────────────────────────────┘
                    │
         ┌──────────┴──────────┐
         │  mshr.probe(addr)   │
         │  동일 블록 요청 존재?  │
         └──────────┬──────────┘
                    │
           ┌────────┴────────┐
           │ YES (MSHR Hit)  │ NO (MSHR 새 엔트리)
           │                 │
    ┌──────┴──────┐   ┌──────┴──────┐
    │mshr.full()?│   │mshr.full()?│
    │병합 가능?    │   │엔트리 가용? │
    └──────┬──────┘   └──────┬──────┘
           │                 │
    YES: merge    YES: 새 엔트리 생성 + 하위 메모리에 요청 전송
    (추가 요청 안 보냄)  NO: RESERVATION_FAIL
    NO: MSHR_MERGE_     NO: MSHR_ENRTY_FAIL
        ENRTY_FAIL
```

- `m_num_entries`: 최대 MSHR 엔트리 수 (구성 파일의 `m_mshr_entries`)
- `m_max_merged`: 하나의 엔트리에 병합 가능한 최대 요청 수 (구성 파일의 `m_mshr_max_merge`)
- `probe(block_addr)`: 해당 블록에 대한 pending 요청이 있는지 확인
- `full(block_addr)`: 새 요청을 수용할 수 있는지 확인 (기존 엔트리의 merge 한도 또는 전체 엔트리 수)
- `add(block_addr, mf)`: 요청을 추가하거나 기존 엔트리에 병합
- `mark_ready(block_addr)`: fill 응답 도착 시 해당 엔트리를 ready로 표시
- `next_access()`: ready 상태의 요청을 하나씩 반환하고, 모든 병합 요청이 반환되면 엔트리 해제

### 2.8 캐시 대역폭 관리

`bandwidth_management` 클래스 (gpu-cache.h:1450-1477)는 캐시의 데이터 포트와 fill 포트를 모델링한다.

- **data port**: 캐시 접근(읽기/쓰기)에 사용. HIT 시 `data_size / port_width` 사이클 점유, writeback 발생 시 evicted 블록의 modified_size / port_width 사이클 추가 점유.
- **fill port**: 하위 메모리에서 데이터가 돌아올 때 사용. `atom_sz / port_width` 사이클 점유.
- 매 사이클 `replenish_port_bandwidth()`로 점유 카운터를 1씩 감소시킨다.

---

## 3. data_cache::access() 함수 라인바이라인 분석

파일: `gpu-cache.cc:1977-1995`

```cpp
enum cache_request_status data_cache::access(
    new_addr_type addr, mem_fetch *mf,
    unsigned time, std::list<cache_event> &events)
{
    // [1] 요청 데이터 크기가 캐시의 atom 크기를 초과하지 않는지 확인
    assert(mf->get_data_size() <= m_config.get_atom_sz());

    // [2] 쓰기 여부 판단
    bool wr = mf->get_is_write();

    // [3] 주소를 캐시라인 경계로 정렬 (하위 비트 제거)
    new_addr_type block_addr = m_config.block_addr(addr);

    // [4] tag_array에서 해당 주소를 probe (읽기 전용 모드, probe_mode=true)
    //     → HIT, HIT_RESERVED, MISS, SECTOR_MISS, RESERVATION_FAIL 중 하나 반환
    unsigned cache_index = (unsigned)-1;
    enum cache_request_status probe_status =
        m_tag_array->probe(block_addr, cache_index, mf, mf->is_write(), true);

    // [5] probe 결과에 따라 적절한 함수 포인터 호출
    //     - 쓰기 HIT → m_wr_hit (wr_hit_wb / wr_hit_wt / wr_hit_we 중 하나)
    //     - 쓰기 MISS → m_wr_miss (wr_miss_wa_naive / wr_miss_no_wa 등)
    //     - 읽기 HIT → m_rd_hit (rd_hit_base)
    //     - 읽기 MISS → m_rd_miss (rd_miss_base)
    enum cache_request_status access_status =
        process_tag_probe(wr, probe_status, addr, cache_index, mf, time, events);

    // [6] 통계 업데이트
    m_stats.inc_stats(mf->get_access_type(),
        m_stats.select_stats_status(probe_status, access_status),
        mf->get_streamID());
    m_stats.inc_stats_pw(mf->get_access_type(),
        m_stats.select_stats_status(probe_status, access_status),
        mf->get_streamID());

    return access_status;
}
```

### 캐시 미스 처리 흐름 (read miss)

`rd_miss_base()` → `send_read_request()` (gpu-cache.cc:1354-1399):

```
1. mshr_addr = mshr_addr(mf->get_addr())  // MSHR 주소 계산 (atom_sz 경계)
2. mshr_hit = m_mshrs.probe(mshr_addr)    // 이미 pending 요청이 있는지 확인
3. mshr_avail = !m_mshrs.full(mshr_addr)  // MSHR에 공간이 있는지 확인

경우 1: mshr_hit && mshr_avail (MSHR Hit - 기존 요청에 병합)
  → tag_array에서 캐시라인 접근 (상태 갱신)
  → m_mshrs.add()로 요청 병합
  → 하위 메모리에 추가 요청을 보내지 않음 (기존 요청이 데이터를 가져올 것)
  → MSHR_HIT 통계 증가

경우 2: !mshr_hit && mshr_avail && miss_queue에 공간 있음 (새로운 미스)
  → tag_array에서 캐시라인 접근 (ON_MISS: 라인 할당 + RESERVED 상태)
  → m_mshrs.add()로 새 엔트리 생성
  → m_extra_mf_fields에 원래 주소/크기 정보 저장
  → mf의 주소와 크기를 mshr_addr/atom_sz로 변경
  → m_miss_queue에 요청 추가 → 다음 cycle()에서 하위 메모리로 전송
  → READ_REQUEST_SENT 이벤트 생성

경우 3: mshr_hit && !mshr_avail → MSHR_MERGE_ENRTY_FAIL (병합 한도 초과)
경우 4: !mshr_hit && !mshr_avail → MSHR_ENRTY_FAIL (엔트리 수 초과)
```

### 캐시 fill 처리 (baseline_cache::fill)

하위 메모리에서 데이터가 돌아오면 (gpu-cache.cc:1231-1276):

```
1. SECTOR_ASSOC인 경우, original_mf의 pending_read를 감소시키고 아직 남아있으면 return
2. m_extra_mf_fields에서 원래의 block_addr, addr, cache_index, data_size를 복원
3. ON_MISS: tag_array->fill(cache_index, time, mf) → RESERVED → VALID (또는 MODIFIED)
   ON_FILL: tag_array->fill(block_addr, time, mf) → 라인 할당 후 즉시 fill
4. m_mshrs.mark_ready(block_addr) → 해당 엔트리를 ready 목록에 추가
5. atomic이면 블록을 MODIFIED로 표시
6. m_extra_mf_fields에서 엔트리 삭제
7. fill port 대역폭 소비
```

---

## 4. L2 캐시 및 메모리 파티션

### 4.1 memory_sub_partition 내부 FIFO 큐 구조

```
  ICNT (Interconnect)
    │
    ▼
 ┌──────────────────┐
 │  ROP Delay Queue  │  ← 텍스처가 아닌 요청에 rop_latency 지연 적용
 │  (m_rop)          │
 └────────┬─────────┘
          ▼
 ┌──────────────────┐
 │ m_icnt_L2_queue  │  ← ICNT에서 L2로 들어오는 요청 큐
 └────────┬─────────┘
          │
          ▼
 ┌──────────────────┐     ┌──────────────────┐
 │    L2 Cache      │────▶│ m_L2_icnt_queue  │──▶ ICNT (응답)
 │   (l2_cache)     │     └──────────────────┘
 └────────┬─────────┘
          │ (미스 시)
          ▼
 ┌──────────────────┐     ┌──────────────────┐
 │ m_L2_dram_queue  │────▶│ m_dram_L2_queue  │
 └──────────────────┘     └──────────────────┘
          │                        ▲
          ▼                        │
 ┌──────────────────────────────────────────┐
 │          memory_partition_unit           │
 │   ┌──────────────────────────────────┐   │
 │   │     DRAM Latency Queue          │   │
 │   │  (m_dram_latency_queue)         │   │
 │   └──────────────┬───────────────────┘   │
 │                  ▼                       │
 │   ┌──────────────────────────────────┐   │
 │   │       DRAM Controller           │   │
 │   │          (dram_t)               │   │
 │   └─────────────────────────────────┘   │
 └──────────────────────────────────────────┘
```

### 4.2 memory_sub_partition::cache_cycle() 상세 흐름

파일: `l2cache.cc:465-597`

이 함수는 매 사이클 호출되며, **아래에서 위로(downstream → upstream)** 순서로 처리한다:

```
Phase 1: L2 fill 응답 처리 (L2 → ICNT)
  - L2 캐시에 ready 접근이 있고 m_L2_icnt_queue가 가득 차지 않았으면
  - L2에서 next_access()로 완료된 요청을 꺼내 응답(set_reply)으로 변환
  - m_L2_icnt_queue에 push
  - L2_WR_ALLOC_R 타입이면 ICNT로 보내지 않고, FETCH_ON_WRITE일 때는 원본 write mf를 응답으로 보냄

Phase 2: DRAM → L2 (데이터 fill)
  - m_dram_L2_queue에서 요청을 꺼내옴
  - L2가 이 요청의 fill을 기다리고 있으면 → L2 fill port가 free일 때 L2에 fill
  - L2가 기다리지 않으면 → m_L2_icnt_queue에 직접 push (L2 bypass)

Phase 3: L2 캐시 자체 사이클 (m_L2cache->cycle())
  - miss queue의 요청을 m_L2_dram_queue로 전송

Phase 4: 새로운 L2 접근 (ICNT → L2)
  - m_icnt_L2_queue에서 요청을 꺼내 L2에 access() 호출
  - HIT: 응답을 m_L2_icnt_queue에 push
  - MISS/SECTOR_MISS/HIT_RESERVED: L2가 자체적으로 miss queue에 요청 추가
  - RESERVATION_FAIL: 다음 사이클에 재시도

Phase 5: ROP 지연 큐
  - m_rop 큐에서 ready_cycle이 된 요청을 m_icnt_L2_queue로 이동
```

### 4.3 memory_sub_partition::push() - 요청 수신

파일: `l2cache.cc:786-812`

```
1. 통계 기록 (memlatstat_icnt2mem_pop)
2. 섹터 캐시인 경우, 요청을 섹터 단위 요청들로 분해 (breakdown_request_to_sector_requests)
3. 각 요청에 대해:
   - m_request_tracker에 추가 (busy 상태 추적)
   - 텍스처 요청이면 → m_icnt_L2_queue에 직접 push (ROP 지연 없음)
   - 그 외 요청이면 → m_rop 큐에 push (rop_latency 만큼 지연)
```

### 4.4 memory_partition_unit::dram_cycle() 상세 분석

파일: `l2cache.cc:306-376`

```cpp
void memory_partition_unit::dram_cycle() {
    // ===== Phase 1: DRAM 완료 → sub-partition 반환 =====
    // returnq에서 완료된 요청을 꺼내 해당 sub_partition의 dram_L2_queue로 push
    mem_fetch *mf_return = m_dram->return_queue_top();
    if (mf_return) {
        unsigned dest_global_spid = mf_return->get_sub_partition_id();
        int dest_spid = global_sub_partition_id_to_local_id(dest_global_spid);
        if (!m_sub_partition[dest_spid]->dram_L2_queue_full()) {
            if (mf_return->get_access_type() == L1_WRBK_ACC) {
                // L1 writeback은 완료 처리 후 삭제
                m_sub_partition[dest_spid]->set_done(mf_return);
                delete mf_return;
            } else {
                // 정상 응답: dram_L2_queue에 push
                m_sub_partition[dest_spid]->dram_L2_queue_push(mf_return);
                m_arbitration_metadata.return_credit(dest_spid);
            }
            m_dram->return_queue_pop();
        }
    }

    // ===== Phase 2: DRAM 내부 사이클 =====
    m_dram->cycle();       // bank 명령 발행 + 타이밍 카운터 갱신
    m_dram->dram_log(SAMPLELOG);

    // ===== Phase 3: L2 → DRAM (서브파티션 중재) =====
    // 라운드 로빈으로 서브파티션을 순회하며 DRAM에 요청 전달
    int last_issued_partition = m_arbitration_metadata.last_borrower();
    for (unsigned p = 0; p < m_config->m_n_sub_partition_per_memory_channel; p++) {
        int spid = (p + last_issued_partition + 1) %
                   m_config->m_n_sub_partition_per_memory_channel;
        if (!m_sub_partition[spid]->L2_dram_queue_empty() &&
            can_issue_to_dram(spid)) {
            mem_fetch *mf = m_sub_partition[spid]->L2_dram_queue_top();
            if (m_dram->full(mf->is_write())) break;
            m_sub_partition[spid]->L2_dram_queue_pop();
            // DRAM latency queue에 넣고, dram_latency 사이클 후에 DRAM에 도착
            dram_delay_t d;
            d.req = mf;
            d.ready_cycle = gpu_sim_cycle + gpu_tot_sim_cycle + dram_latency;
            m_dram_latency_queue.push_back(d);
            m_arbitration_metadata.borrow_credit(spid);
            break;  // 사이클당 하나만 발행
        }
    }

    // ===== Phase 4: DRAM Latency Queue → DRAM push =====
    // ready_cycle이 된 요청을 실제 DRAM 컨트롤러에 push
    if (!m_dram_latency_queue.empty() &&
        (current_cycle >= m_dram_latency_queue.front().ready_cycle) &&
        !m_dram->full(m_dram_latency_queue.front().req->is_write())) {
        mem_fetch *mf = m_dram_latency_queue.front().req;
        m_dram_latency_queue.pop_front();
        m_dram->push(mf);
    }
}
```

### 4.5 서브파티션 중재 (Arbitration)

`arbitration_metadata` 클래스는 여러 서브파티션이 하나의 DRAM 채널을 공유할 때 크레딧 기반 흐름 제어를 수행한다:

- **private credit**: 각 서브파티션에 최소 1개의 크레딧을 보장하여 forward progress를 보장
- **shared credit**: 나머지 크레딧을 모든 서브파티션이 공유
- 공유 크레딧 한도 = `gpgpu_frfcfs_dram_sched_queue_size` + `gpgpu_dram_return_queue_size` - (서브파티션 수 - 1)
- `can_issue_to_dram(spid)`: 크레딧이 남아있고 해당 서브파티션의 dram_L2_queue가 full이 아닐 때만 발행 허용

---

## 5. DRAM 컨트롤러

### 5.1 DRAM 구조

```
 dram_t
 ├── bank_t **bk[nbk]           ← 뱅크 배열
 │   ├── state: BANK_IDLE / BANK_ACTIVE
 │   ├── curr_row: 현재 열린 행
 │   ├── mrq: 현재 서비스 중인 요청
 │   └── 타이밍 카운터: RCDc, RASc, RPc, RCc, WTPc, RTPc, RCDWRc
 │
 ├── bankgrp_t **bkgrp[nbkgrp]  ← 뱅크 그룹 (GDDR5/HBM)
 │   ├── CCDLc: 같은 뱅크 그룹 내 column-to-column 지연
 │   └── RTPLc: read-to-precharge 지연 (뱅크 그룹 단위)
 │
 ├── fifo_pipeline<dram_req_t> *rwq   ← 읽기/쓰기 파이프라인 (CL/WL 지연 모델링)
 ├── fifo_pipeline<dram_req_t> *mrqq  ← 메모리 요청 큐 (incoming)
 ├── fifo_pipeline<mem_fetch> *returnq ← 완료된 요청 반환 큐
 │
 ├── frfcfs_scheduler *m_frfcfs_scheduler ← FR-FCFS 스케줄러
 │
 └── 글로벌 타이밍 카운터: RRDc, CCDc, RTWc, WTRc
```

### 5.2 DRAM 타이밍 파라미터

| 파라미터 | 설명 | 범위 |
|----------|------|------|
| `tRCD` | Row-to-Column Delay. ACT 후 READ/WRITE 가능까지 | 뱅크 단위 |
| `tRCDWR` | Row-to-Column Write Delay. ACT 후 WRITE 가능까지 | 뱅크 단위 |
| `tCL` (=CL) | CAS Latency. READ 명령 후 데이터 출력까지 | rwq min_length로 모델링 |
| `tWL` (=WL) | Write Latency. WRITE 명령 후 데이터 입력까지 | rwq min_length로 모델링 |
| `tRAS` | Row Active Strobe. ACT 후 PRE 가능까지 최소 시간 | 뱅크 단위 |
| `tRP` | Row Precharge. PRE 후 다음 ACT 가능까지 | 뱅크 단위 |
| `tRC` | Row Cycle. ACT 후 같은 뱅크에 다음 ACT 가능까지 (= tRAS + tRP) | 뱅크 단위 |
| `tRRD` | Row-to-Row Delay. 다른 뱅크에 연속 ACT 간 최소 간격 | 전역 |
| `tCCD` | Column-to-Column Delay. 연속 READ/WRITE 간 최소 간격 | 전역 |
| `tCCDL` | Column-to-Column Delay Long. 같은 뱅크 그룹 내 | 뱅크 그룹 단위 |
| `tRTW` | Read-To-Write 전환 지연 | 전역 |
| `tWTR` | Write-To-Read 전환 지연 | 전역 |
| `tWTP` | Write-To-Precharge. WRITE 후 PRE 가능까지 | 뱅크 단위 |
| `tRTP` | Read-To-Precharge. READ 후 PRE 가능까지 | 뱅크 단위 |
| `tRTPL` | Read-To-Precharge Long (뱅크 그룹 단위) | 뱅크 그룹 단위 |
| `BL` | Burst Length. 데이터 버스트 길이 | - |

### 5.3 dram_t::cycle() 상세 분석

파일: `dram.cc:290-553`

```
Phase 1: rwq에서 완료된 전송 처리 (데이터 버스 출력)
  - rwq에서 cmd를 pop
  - cmd->dqbytes에 dram_atom_size를 더함
  - dqbytes >= nbytes이면 전송 완료:
    → write-back이면: set_done() 후 delete
    → 일반 요청이면: set_reply() 후 returnq에 push

Phase 2: 스케줄러 호출
  - DRAM_FIFO: scheduler_fifo() - mrqq에서 순서대로 뱅크에 배치
  - DRAM_FRFCFS: scheduler_frfcfs() - FR-FCFS 스케줄러로 뱅크에 배치

Phase 3: 명령 발행 (모든 뱅크 순회)
  - dual_bus_interface인 경우: column 명령과 row 명령을 독립적으로 발행
  - single_bus_interface인 경우: column 또는 row 명령 중 하나만 발행

  Column 명령 (issue_col_command):
    READ 조건:
      !CCDc && !RCDc && !CCDLc(같은 뱅크 그룹) &&
      curr_row == mrq->row (행 히트) && mrq->rw == READ &&
      WTRc == 0 (W→R 전환 완료) && BANK_ACTIVE && !rwq->full()
      → rwq에 push, CCD/CCDL/RTW/RTP 카운터 설정

    WRITE 조건:
      !CCDc && !RCDWRc && !CCDLc &&
      curr_row == mrq->row && mrq->rw == WRITE &&
      RTWc == 0 (R→W 전환 완료) && BANK_ACTIVE && !rwq->full()
      → rwq에 push, CCD/CCDL/WTR/WTP 카운터 설정

  Row 명령 (issue_row_command):
    ACTIVATE 조건:
      !RRDc && state == BANK_IDLE && !RPc && !RCc
      → curr_row = mrq->row, state = BANK_ACTIVE
      → RRD/RCD/RCDWR/RAS/RC 카운터 설정

    PRECHARGE 조건:
      curr_row != mrq->row && state == BANK_ACTIVE &&
      !RASc && !WTPc && !RTPc && !RTPLc(뱅크 그룹)
      → state = BANK_IDLE, RP 카운터 설정

Phase 4: 모든 타이밍 카운터 감소 (DEC2ZERO 매크로)
```

### 5.4 DRAM 명령 시퀀스 예시

```
 Row Miss (행 미스) 시:
   현재 상태: BANK_ACTIVE, curr_row=X, 요청 row=Y (X != Y)

   [PRE] BANK_ACTIVE → BANK_IDLE     tRP 사이클 대기
   [ACT] BANK_IDLE → BANK_ACTIVE     tRCD 사이클 대기 (READ) / tRCDWR (WRITE)
   [RD/WR] Column 명령 발행           tCL/tWL 사이클 후 데이터 전송

 Row Hit (행 히트) 시:
   현재 상태: BANK_ACTIVE, curr_row=Y, 요청 row=Y

   [RD/WR] Column 명령 즉시 발행      PRE/ACT 불필요!
```

### 5.5 스케줄링 정책

#### FIFO 스케줄러 (scheduler_fifo)
파일: `dram.cc:272-282`

가장 단순한 스케줄러. mrqq에서 맨 앞의 요청을 꺼내 해당 뱅크가 비어있으면 배치한다.

#### FR-FCFS 스케줄러 (First-Ready First-Come-First-Served)
파일: `dram_sched.cc:109-199`

```
1. 모드 전환 (별도 쓰기 큐 활성화 시):
   - READ_MODE에서 write_pending >= write_high_watermark이면 → WRITE_MODE
   - WRITE_MODE에서 write_pending < write_low_watermark이면 → READ_MODE

2. 스케줄링 우선순위:
   (1) Row Hit 우선: curr_row과 같은 행의 요청을 먼저 서비스
   (2) Row Miss 시: FCFS (가장 오래된 요청)

3. 자료 구조:
   - m_queue[bank]: 각 뱅크별 요청 리스트
   - m_bins[bank][row]: 행 번호별 요청 이터레이터 그룹
   - m_last_row[bank]: 현재 서비스 중인 행의 이터레이터 리스트

4. schedule(bank, curr_row):
   - m_last_row[bank]이 NULL이면:
     - curr_row에 대한 bin을 찾음 → 있으면 row hit, 없으면 큐의 가장 뒤 요청(FCFS)으로 fallback
   - m_last_row[bank]에서 가장 오래된 요청(back)을 선택하여 반환
   - 해당 행의 요청이 모두 소진되면 m_last_row[bank] = NULL
```

---

## 6. 메모리 주소 디코딩

### 6.1 addrdec_t 구조체

```cpp
struct addrdec_t {
    unsigned chip;          // DRAM 채널 번호
    unsigned bk;            // 뱅크 번호
    unsigned row;           // 행 번호
    unsigned col;           // 열 번호
    unsigned burst;         // 버스트 위치
    unsigned sub_partition; // 서브파티션 ID
};
```

### 6.2 주소 디코딩 방식

`linear_to_raw_address_translation::addrdec_tlx()` (addrdec.cc:95-201)

물리 주소에서 비트마스크를 사용하여 각 필드를 추출한다:

```
 물리 주소 (64비트)
 ┌─────────────────────────────────────────────────────────────┐
 │ ... | ROW bits | CHIP bits | BK bits | COL bits | BURST    │
 └─────────────────────────────────────────────────────────────┘
       ▲            ▲          ▲          ▲          ▲
       │            │          │          │          │
  addrdec_mask[ROW] │   mask[BK]   mask[COL]   mask[BURST]
               mask[CHIP]
```

**주소 디코딩 두 가지 모드:**

1. **Power-of-2 채널 수** (`gap == 0`):
   - 비트마스크로 각 필드를 직접 추출: `chip = packbits(mask[CHIP], addr)`
   - 채널 비트는 `ADDR_CHIP_S` 위치에 삽입되고, 그 위의 비트들은 시프트됨

2. **Non-power-of-2 채널 수** (`gap != 0`):
   - 채널 ID = `(addr >> ADDR_CHIP_S) % n_channel`
   - 나머지 주소 = `((addr >> ADDR_CHIP_S) / n_channel) << ADDR_CHIP_S | (addr & lower_bits)`
   - 나머지 주소에서 bk, row, col, burst를 비트마스크로 추출

### 6.3 파티션 인덱싱 함수

| 함수 | 설명 |
|------|------|
| `CONSECUTIVE` | 기본, 비트마스크 그대로 사용 |
| `BITWISE_PERMUTATION` | 상위 비트와 chip ID를 bitwise XOR |
| `IPOLY` | IPoly 해싱으로 sub_partition을 계산. chip과 bk 하위 비트를 결합한 후 해싱 |
| `RANDOM` | 해시 테이블 기반 랜덤 매핑 (비현실적, 실험용) |

### 6.4 partition_address()

L2 캐시의 세트 인덱싱에서 파티션 비트를 제거하여 세트 캠핑을 방지:

```
partition_address = packbits(~(chip_mask | sub_partition_mask), addr)
```

이 함수는 chip ID와 sub_partition ID에 해당하는 비트를 주소에서 "짜내서" 제거한 주소를 반환한다. L2 캐시는 이 압축된 주소를 사용하여 세트 인덱스를 계산하므로, 서로 다른 파티션에 매핑되는 주소들이 같은 세트에 캠핑되는 문제를 방지한다.

### 6.5 구성 가능한 주소 매핑 문자열

옵션 `-gpgpu_mem_addr_mapping`으로 비트별 매핑을 직접 지정할 수 있다:

```
"dramid@<start_bit>;<mapping_string>"

매핑 문자열 문자:
  D/d = DRAM chip ID 비트
  B/b = Bank 비트
  R/r = Row 비트
  C/c = Column 비트
  S/s = Burst 비트 (Column에도 포함)
  0   = 무시 비트
  |, ' ', . = 구분자 (무시)
```

---

## 7. mem_fetch 패킷 생명주기

### 7.1 mem_fetch 주요 필드

```cpp
class mem_fetch {
    unsigned m_request_uid;       // 고유 요청 ID (전역 카운터)
    unsigned m_sid;               // 소스 Shader Core ID
    unsigned m_tpc;               // 소스 TPC (Thread Processing Cluster)
    unsigned m_wid;               // 소스 Warp ID

    mem_access_t m_access;        // 접근 타입, 주소, 크기, warp mask, byte mask, sector mask
    unsigned m_data_size;         // 데이터 크기 (변경 가능 - MSHR에서 atom_sz로 변경)
    unsigned m_ctrl_size;         // 제어 메타데이터 크기
    new_addr_type m_partition_addr; // 파티션 내 주소 (chip 비트 제거)
    addrdec_t m_raw_addr;         // 디코딩된 DRAM 주소 (chip, bk, row, col, sub_partition)

    enum mf_type m_type;          // READ_REQUEST, WRITE_REQUEST, READ_REPLY, WRITE_ACK
    enum mem_fetch_status m_status; // 현재 위치/상태

    unsigned m_timestamp;         // 생성 시각
    unsigned m_timestamp2;        // ICNT→shader 전송 시각 (읽기 전용)

    mem_fetch *original_mf;       // 섹터 분할 시 원본 요청 포인터
    mem_fetch *original_wr_mf;    // fetch-on-write 시 원본 쓰기 요청 포인터
};
```

### 7.2 mem_fetch_status 전이 다이어그램

```
 MEM_FETCH_INITIALIZED
     │
     ├──▶ IN_L1D_MISS_QUEUE (L1 데이터 캐시 미스)
     ├──▶ IN_L1T_MISS_QUEUE (L1 텍스처 캐시 미스)
     ├──▶ IN_L1C_MISS_QUEUE (L1 상수 캐시 미스)
     ├──▶ IN_L1I_MISS_QUEUE (L1 명령어 캐시 미스)
     │
     ▼
 IN_ICNT_TO_MEM                          ← 인터커넥트를 통해 메모리로 이동
     │
     ▼
 IN_PARTITION_ROP_DELAY                  ← ROP 지연 (텍스처가 아닌 요청)
     │
     ▼
 IN_PARTITION_ICNT_TO_L2_QUEUE           ← L2 접근 대기
     │
     ├──▶ [L2 HIT] IN_PARTITION_L2_TO_ICNT_QUEUE ──▶ IN_ICNT_TO_SHADER
     │
     ▼ [L2 MISS]
 IN_PARTITION_L2_MISS_QUEUE              ← L2 미스 큐 (baseline_cache의 m_miss_queue)
     │
     ▼
 IN_PARTITION_L2_TO_DRAM_QUEUE           ← L2에서 DRAM으로
     │
     ▼
 IN_PARTITION_DRAM_LATENCY_QUEUE         ← L2↔DRAM 간 고정 지연
     │
     ▼
 IN_PARTITION_MC_INTERFACE_QUEUE         ← DRAM 컨트롤러 입력 (mrqq)
     │
     ▼
 IN_PARTITION_MC_INPUT_QUEUE             ← FR-FCFS 스케줄러 큐
     │
     ▼
 IN_PARTITION_MC_BANK_ARB_QUEUE          ← 뱅크에 배치됨
     │
     ▼
 IN_PARTITION_DRAM                       ← DRAM 서비스 중
     │
     ▼
 IN_PARTITION_MC_RETURNQ                 ← DRAM 완료, returnq
     │
     ▼
 IN_PARTITION_DRAM_TO_L2_QUEUE           ← DRAM에서 L2로 응답
     │
     ▼
 IN_PARTITION_L2_FILL_QUEUE              ← L2 fill 처리
     │
     ▼
 IN_PARTITION_L2_TO_ICNT_QUEUE           ← L2에서 ICNT로 응답
     │
     ▼
 IN_ICNT_TO_SHADER                       ← 인터커넥트를 통해 SM으로
     │
     ▼
 IN_CLUSTER_TO_SHADER_QUEUE              ← 클러스터 내 셰이더 큐
     │
     ▼
 IN_SHADER_FETCHED / MEM_FETCH_DELETED   ← 셰이더에서 소비 / 삭제
```

### 7.3 mem_fetch 생성 (생성자)

파일: `mem_fetch.cc:37-79`

```cpp
mem_fetch::mem_fetch(const mem_access_t &access, ...) {
    m_request_uid = sm_next_mf_request_uid++;  // 전역 고유 ID
    m_access = access;
    m_data_size = access.get_size();
    m_ctrl_size = ctrl_size;

    // 주소 디코딩: 물리 주소 → (chip, bk, row, col, sub_partition)
    config->m_address_mapping.addrdec_tlx(access.get_addr(), &m_raw_addr);
    // 파티션 내 주소 계산 (chip 비트 제거)
    m_partition_addr = config->m_address_mapping.partition_address(access.get_addr());

    // 쓰기면 WRITE_REQUEST, 읽기면 READ_REQUEST
    m_type = m_access.is_write() ? WRITE_REQUEST : READ_REQUEST;
    m_timestamp = cycle;  // 생성 시각 기록
    m_status = MEM_FETCH_INITIALIZED;
}
```

### 7.4 set_reply()

요청이 DRAM에서 처리 완료되면 응답으로 전환:
- `READ_REQUEST` → `READ_REPLY`
- `WRITE_REQUEST` → `WRITE_ACK`
- writeback 요청(`L1_WRBK_ACC`, `L2_WRBK_ACC`)은 set_reply()를 호출하지 않고 직접 삭제됨

---

## 8. delayqueue (fifo_pipeline)

파일: `delayqueue.h`

`fifo_pipeline<T>` 는 최소 지연(min_len)을 가진 FIFO 큐로, DRAM 파이프라인과 메모리 서브시스템의 각 단계 간 전송 지연을 모델링한다.

```cpp
fifo_pipeline(const char* nm, unsigned int minlen, unsigned int maxlen);
```

- `minlen`: 최소 파이프라인 길이. push 후 최소 minlen 사이클 후에 데이터가 top()에 나타남. NULL 노드를 삽입하여 지연을 시뮬레이션한다.
- `maxlen`: 최대 큐 길이. full()은 `length >= maxlen`일 때 true.
- push 시 큐의 끝에 삽입, pop 시 큐의 앞에서 제거. pop 후 길이가 minlen 미만이면 자동으로 NULL을 push하여 최소 길이를 유지한다.

주요 사용 예:
- `rwq` (DRAM read/write queue): minlen=CL, maxlen=CL+1 → CAS Latency를 모델링
- `returnq` (DRAM 반환 큐): minlen=0, maxlen=설정값 → 지연 없는 버퍼
- `m_icnt_L2_queue`, `m_L2_dram_queue` 등: minlen=0 → 지연 없는 FIFO

---

## 9. 메모리 통계 (memory_stats_t)

### 9.1 주요 추적 지표

| 지표 | 설명 |
|------|------|
| `mf_total_lat` | 전체 메모리 요청 레이턴시 합 |
| `num_mfs` | 완료된 메모리 요청 수 |
| `tot_icnt2mem_latency` | ICNT → 메모리 도착까지 레이턴시 합 |
| `tot_icnt2sh_latency` | ICNT → 셰이더 도착까지 레이턴시 합 |
| `tot_mrq_latency` | DRAM 스케줄러 큐 대기 레이턴시 합 |
| `bankreads[sid][dram][bank]` | 셰이더별, DRAM 칩별, 뱅크별 읽기 횟수 |
| `bankwrites[sid][dram][bank]` | 셰이더별, DRAM 칩별, 뱅크별 쓰기 횟수 |
| `concurrent_row_access[dram][bank]` | 같은 행에 대한 동시 접근 수 |
| `num_activates[dram][bank]` | 뱅크별 ACT 명령 횟수 |
| `row_access[dram][bank]` | 뱅크별 행 접근 횟수 |
| `L2_read_miss/hit, L2_write_miss/hit` | L2 캐시 통계 (AerialVision용) |

### 9.2 레이턴시 측정 방식

`memlatstat_done(mf)`: 전체 레이턴시 = `현재 사이클 - mf->get_timestamp()`

여기서 `m_timestamp`는 mem_fetch 생성 시각이므로, L1 미스부터 셰이더에 응답이 도착할 때까지의 전체 레이턴시를 측정한다.

---

## 10. 주요 설계 패턴 요약

### 10.1 함수 포인터를 통한 정책 설정

`data_cache`는 쓰기 정책(write-back/write-through/write-evict)과 쓰기 할당 정책(write-allocate/no-write-allocate/fetch-on-write/lazy-fetch-on-read)을 **멤버 함수 포인터**로 구현한다:

```cpp
// 쓰기 히트 함수 포인터
enum cache_request_status (data_cache::*m_wr_hit)(...);
// 쓰기 미스 함수 포인터
enum cache_request_status (data_cache::*m_wr_miss)(...);
// 읽기 히트/미스 함수 포인터
enum cache_request_status (data_cache::*m_rd_hit)(...);
enum cache_request_status (data_cache::*m_rd_miss)(...);
```

`init()` 에서 설정 파일의 정책에 따라 적절한 함수를 바인딩한다. 이를 통해 `access()` 함수에서 긴 조건 분기 없이 `(this->*m_wr_hit)(...)` 형태로 호출한다.

### 10.2 크레딧 기반 흐름 제어

메모리 파티션에서 서브파티션들이 DRAM 채널을 공유할 때, `arbitration_metadata`의 크레딧 시스템으로 deadlock을 방지하면서도 공정한 자원 분배를 보장한다. 각 서브파티션은 최소 1개의 private 크레딧으로 forward progress를 보장받는다.

### 10.3 섹터 캐시 모델

Volta 아키텍처 이후의 섹터 캐시를 모델링한다. 128바이트 캐시라인을 4개의 32바이트 섹터로 분할하여:
- 섹터 단위로 할당/유효화 가능 → 대역폭 절약
- `SECTOR_MISS`: 라인은 있지만 해당 섹터가 invalid → 해당 섹터만 fetch
- L2에서 수신한 128바이트 요청을 `breakdown_request_to_sector_requests()`로 32바이트 섹터 요청들로 분해
