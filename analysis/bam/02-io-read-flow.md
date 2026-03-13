# BaM GPU에서 NVMe 읽기 요청의 전체 흐름

## 개요

BaM에서 GPU 커널이 NVMe 데이터를 읽을 때, 전체 경로는 다음과 같다: GPU 스레드가 `array_d_t::operator[]`를 호출하면, 소프트웨어 페이지 캐시를 조회하고, 캐시 미스 시 NVMe SQ에 직접 읽기 명령을 삽입한 뒤 CQ 완료를 폴링하여 데이터를 GPU 메모리로 가져온다. 모든 과정이 GPU 커널 내에서 수행되며, CPU 개입이 없다.

## 전체 읽기 흐름 시퀀스

```
GPU 스레드                  page_cache          NVMe SQ/CQ        NVMe SSD
    │                          │                    │                 │
    │ array[i]                 │                    │                 │
    ├──► find_range(i)         │                    │                 │
    ├──► get_page(i)           │                    │                 │
    ├──► get_global_address()  │                    │                 │
    │                          │                    │                 │
    │ coalesce_page()          │                    │                 │
    │  ├─ __activemask()       │                    │                 │
    │  ├─ __match_any_sync()   │                    │                 │
    │  └─ master만 실행:       │                    │                 │
    │                          │                    │                 │
    ├──────► acquire_page()    │                    │                 │
    │        │                 │                    │                 │
    │        ├─ state.fetch_add(count)              │                 │
    │        │                 │                    │                 │
    │   [캐시 히트: V_NB]      │                    │                 │
    │        ├─ return page_trans (offset)          │                 │
    │        │                 │                    │                 │
    │   [캐시 미스: NV_NB]     │                    │                 │
    │        ├─ state.fetch_or(BUSY)                │                 │
    │        ├─────────► find_slot()                │                 │
    │        │           (캐시 슬롯 할당/교체)      │                 │
    │        ├─────────────────► read_data()        │                 │
    │        │                 │  ├ get_cid()       │                 │
    │        │                 │  ├ nvm_cmd_header  │                 │
    │        │                 │  ├ nvm_cmd_data_ptr│                 │
    │        │                 │  ├ nvm_cmd_rw_blks │                 │
    │        │                 │  ├ sq_enqueue() ───┼──► SQ에 삽입    │
    │        │                 │  │  (도어벨 링)     │  ──────────►   │
    │        │                 │  │                  │                 │
    │        │                 │  ├ cq_poll() ◄─────┼──── 완료 대기   │
    │        │                 │  │  (CID 매칭)      │  ◄────────────  │
    │        │                 │  ├ cq_dequeue() ───┼──► CQ 도어벨    │
    │        │                 │  └ put_cid()       │                 │
    │        │                 │                    │                 │
    │        ├─ pages[index].offset = page_trans    │                 │
    │        ├─ DISABLE_BUSY_ENABLE_VALID           │                 │
    │        └─ return page_trans                   │                 │
    │                          │                    │                 │
    ├──► get_cache_page_addr(base_master)           │                 │
    ├──► addr = base_addr + page_trans * page_size  │                 │
    │                          │                    │                 │
    │ ret = addr[subindex]     │                    │                 │
    │                          │                    │                 │
    ├──► release_page(page, count)                  │                 │
    │    pages[index].state.fetch_sub(count)        │                 │
    │                          │                    │                 │
    ◄── return ret             │                    │                 │
```

## 단계별 상세 분석

### Step 1: 데이터 접근 시작 - array_d_t::operator[]

**파일**: `include/page_cache.h` (array_d_t::seq_read, line ~1541)

GPU 커널에서 `(*dr)[index]` 또는 `array[i]`를 호출하면, 내부적으로 `seq_read()` 또는 `operator[]`가 실행된다.

```cpp
__device__
T seq_read(const size_t i) const {
    uint32_t lane = lane_id();           // warp 내 레인 ID
    int64_t r = find_range(i);           // 해당 인덱스가 속한 range 검색
    auto r_ = d_ranges + r;

    uint32_t mask = __activemask();      // 활성 스레드 마스크
    uint64_t page = r_->get_page(i);     // 인덱스 → 페이지 번호 변환
    uint64_t subindex = r_->get_subindex(i);  // 페이지 내 오프셋
    uint64_t gaddr = r_->get_global_address(page);  // 글로벌 캐시 주소

    // Warp 내 동일 페이지 접근을 합치기
    coalesce_page(lane, mask, r, page, gaddr, false,
                  eq_mask, master, count, base_master);

    // 캐시에서 데이터 읽기
    ret = ((T*)(r_->get_cache_page_addr(base_master) + subindex))[0];

    // 페이지 해제
    if (master == lane)
        r_->release_page(page, count);

    return ret;
}
```

### Step 2: 범위 검색 - find_range()

**파일**: `include/page_cache.h:1318`

```cpp
__device__
int64_t find_range(const size_t i) const {
    int64_t range = -1;
    for (int64_t k = 0; k < n_ranges; k++) {
        if ((d_ranges[k].index_start <= i) && (d_ranges[k].count > i)) {
            range = k;
            break;
        }
    }
    return range;
}
```

### Step 3: 페이지/서브인덱스 계산

**파일**: `include/page_cache.h:1060, 1067`

```cpp
__device__
uint64_t get_page(const size_t i) const {
    // 인덱스를 캐시 페이지 번호로 변환
    return ((i - index_start) * sizeof(T) + page_start_offset) >> cache.page_size_log;
}

__device__
uint64_t get_subindex(const size_t i) const {
    // 페이지 내 바이트 오프셋 계산
    return ((i - index_start) * sizeof(T) + page_start_offset) & cache.page_size_minus_1;
}
```

### Step 4: I/O 합병 - coalesce_page()

**파일**: `include/page_cache.h:1332`

같은 warp 내에서 동일 페이지에 접근하는 스레드들을 합쳐서, master 스레드 하나만 캐시 조회/I/O를 수행한다.

```cpp
__device__
void coalesce_page(uint32_t lane, uint32_t mask, int64_t r,
                   uint64_t page, uint64_t gaddr, bool write,
                   uint32_t& eq_mask, int& master,
                   uint32_t& count, uint64_t& base_master) const
{
    // 같은 글로벌 주소를 접근하는 스레드 찾기
    eq_mask = __match_any_sync(mask, gaddr);
    eq_mask &= __match_any_sync(mask, (uint64_t)this);

    master = __ffs(eq_mask) - 1;      // 마스터 스레드 선출
    count = __popc(eq_mask);           // 동일 페이지 접근 스레드 수

    uint32_t dirty = __any_sync(eq_mask, write);

    if (master == lane) {
        base = r_->acquire_page(page, count, dirty, ctrl, queue);
        base_master = base;
    }
    // 마스터의 결과를 모든 스레드에 브로드캐스트
    base_master = __shfl_sync(eq_mask, base_master, master);
}
```

**합병의 효과**:
```
Warp (32 스레드) 에서 4개 스레드가 같은 페이지 접근 시:
  Thread 0: page[100] ──┐
  Thread 5: page[100] ──┼──► 1번의 acquire_page() 호출 (count=4)
  Thread 12: page[100] ─┤    → 1번의 NVMe I/O (미스 시)
  Thread 28: page[100] ─┘    → ref count +4
```

### Step 5: 페이지 획득 - acquire_page()

**파일**: `include/page_cache.h:1122`

캐시의 핵심 함수로, 상태 머신을 기반으로 캐시 히트/미스를 처리한다.

```cpp
__device__
uint64_t acquire_page(size_t pg, uint32_t count, bool write,
                      uint32_t ctrl_, uint32_t queue)
{
    access_cnt.fetch_add(count, simt::memory_order_relaxed);

    // 레퍼런스 카운트를 먼저 증가 (원자적)
    read_state = pages[index].state.fetch_add(count, simt::memory_order_acquire);

    do {
        st = (read_state >> (CNT_SHIFT+1)) & 0x03;  // 상위 비트에서 상태 추출

        switch (st) {
        case NV_NB:  // Not Valid, Not Busy → 캐시 미스
            st_new = pages[index].state.fetch_or(BUSY, ...);
            if ((st_new & BUSY) == 0) {
                // 캐시 슬롯 할당
                page_trans = cache.find_slot(index, range_id, queue);

                // NVMe 읽기 수행
                ctrl = get_backing_ctrl(index);
                b_page = get_backing_page(index);
                read_data(&cache, (c->d_qps)+queue,
                          b_page * n_blocks_per_page,
                          n_blocks_per_page, page_trans);

                pages[index].offset = page_trans;
                // BUSY 해제, VALID 설정
                pages[index].state.fetch_xor(DISABLE_BUSY_ENABLE_VALID, ...);
                return page_trans;
            }
            break;

        case V_NB:  // Valid, Not Busy → 캐시 히트
            page_trans = pages[index].offset;
            hit_cnt.fetch_add(count, simt::memory_order_relaxed);
            return page_trans;

        case NV_B:  // Not Valid, Busy → 다른 스레드가 로딩 중
        case V_B:   // Valid, Busy → 교체 진행 중
        default:    // spin-wait (나노 슬립)
            break;
        }

        __nanosleep(ns);  // 지수 백오프
        read_state = pages[index].state.load(simt::memory_order_acquire);
    } while (fail);
}
```

### Step 6: 캐시 슬롯 할당 - find_slot()

**파일**: `include/page_cache.h:1786`

CLOCK 알고리즘 유사 방식으로 캐시 슬롯을 찾는다.

```cpp
__device__
uint32_t page_cache_d_t::find_slot(uint64_t address, uint64_t range_id, uint32_t queue_)
{
    uint64_t global_address = (address << n_ranges_bits) | range_id;

    do {
        // 라운드 로빈으로 다음 캐시 페이지 선택
        page = page_ticket->fetch_add(1, ...) % n_pages;

        v = cache_pages[page].page_take_lock.load(...);

        if (v == FREE) {
            // 빈 슬롯 → 바로 할당
            lock = cache_pages[page].page_take_lock.compare_exchange_weak(v, LOCKED, ...);
            if (lock) {
                cache_pages[page].page_translation = global_address;
                cache_pages[page].page_take_lock.store(UNLOCKED, ...);
                fail = false;
            }
        }
        else if (v == UNLOCKED) {
            // 사용 중인 슬롯 → 교체 시도
            lock = cache_pages[page].page_take_lock.compare_exchange_weak(v, LOCKED, ...);
            if (lock) {
                // 이전 매핑의 상태 확인
                previous_global_address = cache_pages[page].page_translation;
                expected_state = ranges[prev_range][prev_address].state.load(...);
                cnt = expected_state & CNT_MASK;

                if (cnt == 0 && !(expected_state & BUSY)) {
                    // 레퍼런스 카운트 0 → 교체 가능
                    // dirty 페이지는 먼저 write-back
                    if (expected_state & DIRTY) {
                        write_data(this, qp, index*n_blocks, n_blocks, page);
                    }

                    // 새 주소로 매핑 변경
                    cache_pages[page].page_translation = global_address;
                    // 이전 페이지 상태 INVALID로
                    ranges[prev_range][prev_address].state.fetch_and(CNT_MASK, ...);
                    fail = false;
                }
                cache_pages[page].page_take_lock.store(UNLOCKED, ...);
            }
        }
    } while (fail);

    return page;
}
```

### Step 7: NVMe 읽기 명령 실행 - read_data()

**파일**: `include/page_cache.h:2009`

```cpp
__device__
void read_data(page_cache_d_t* pc, QueuePair* qp,
               uint64_t starting_lba, uint64_t n_blocks,
               unsigned long long pc_entry)
{
    nvm_cmd_t cmd;

    // 1. CID 할당 (lock-free)
    uint16_t cid = get_cid(&qp->sq);

    // 2. NVMe 명령 구성
    nvm_cmd_header(&cmd, cid, NVM_IO_READ, qp->nvmNamespace);
    uint64_t prp1 = pc->prp1[pc_entry];     // 캐시 페이지의 물리 주소
    uint64_t prp2 = pc->prps ? pc->prp2[pc_entry] : 0;
    nvm_cmd_data_ptr(&cmd, prp1, prp2);      // PRP 설정
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks);  // LBA 범위 설정

    // 3. SQ에 삽입 + 도어벨 링
    uint16_t sq_pos = sq_enqueue(&qp->sq, &cmd);

    // 4. CQ 폴링 (완료 대기)
    uint32_t head, head_;
    uint32_t cq_pos = cq_poll(&qp->cq, cid, &head, &head_);

    // 5. CQ dequeue + 도어벨 업데이트
    cq_dequeue(&qp->cq, cq_pos, &qp->sq, head, head_);

    // 6. 추가 읽기 (더블 버퍼링 최적화)
    enqueue_second(pc, qp, starting_lba, &cmd, cid, pc_pos, pc_prev_head);

    // 7. CID 반환
    put_cid(&qp->sq, cid);
}
```

### Step 8: NVMe 명령 구성 상세

**파일**: `include/nvm_cmd.h`

#### 8.1 명령 헤더 (DWORD 0-1)
```cpp
void nvm_cmd_header(nvm_cmd_t* cmd, uint16_t cid, uint8_t opcode, uint32_t ns_id) {
    cmd->dword[0] = (cid << 16) | opcode;  // CID + opcode
    cmd->dword[1] = ns_id;                  // 네임스페이스 ID
}
```

#### 8.2 데이터 포인터 (DWORD 6-9)
```cpp
void nvm_cmd_data_ptr(nvm_cmd_t* cmd, uint64_t prp1, uint64_t prp2) {
    cmd->dword[6] = (uint32_t) prp1;        // PRP1 하위 32비트
    cmd->dword[7] = (uint32_t) (prp1 >> 32);// PRP1 상위 32비트
    cmd->dword[8] = (uint32_t) prp2;        // PRP2 하위 32비트
    cmd->dword[9] = (uint32_t) (prp2 >> 32);// PRP2 상위 32비트
}
```

PRP1은 캐시 페이지의 **GPU 메모리 물리 주소**를 가리킨다. NVMe 컨트롤러가 DMA로 데이터를 직접 GPU 메모리에 쓴다.

#### 8.3 블록 범위 (DWORD 10-12)
```cpp
void nvm_cmd_rw_blks(nvm_cmd_t* cmd, uint64_t start_lba, uint16_t n_blks) {
    cmd->dword[10] = start_lba;              // Starting LBA (하위)
    cmd->dword[11] = start_lba >> 32;        // Starting LBA (상위)
    cmd->dword[12] = (n_blks - 1) & 0xffff; // Number of Logical Blocks (0-based)
}
```

### Step 9: 병렬 SQ Enqueue - sq_enqueue()

**파일**: `include/nvm_parallel_queue.h:168`

수천 개의 GPU 스레드가 동시에 NVMe 명령을 삽입하기 위한 lock-free 알고리즘이다.

```
sq_enqueue() 실행 흐름:
┌─────────────────────────────────────────────┐
│ 1. 티켓 획득: in_ticket.fetch_add(1)        │
│ 2. 위치 계산: pos = ticket & qs_minus_1     │
│ 3. ID 계산:  id = get_id(ticket, qs_log2)  │
│ 4. 스핀 대기: tickets[pos] == id 될 때까지  │
│ 5. 명령 복사: queue[pos] = cmd (ulonglong4) │
│ 6. tail_mark[pos] = LOCKED (준비 완료 표시) │
│ 7. 도어벨 링 시도:                          │
│    ├─ tail_lock 획득                        │
│    ├─ move_tail() → 연속 LOCKED 슬롯 수집  │
│    ├─ *(sq->db) = new_tail (MMIO 쓰기)     │
│    └─ tail_lock 해제                        │
│ 8. tickets[pos] += 1 (다음 라운드 허용)     │
└─────────────────────────────────────────────┘
```

### Step 10: CQ 폴링 - cq_poll()

**파일**: `include/nvm_parallel_queue.h:379`

```cpp
__device__
uint32_t cq_poll(nvm_queue_t* cq, uint16_t search_cid,
                 uint32_t* loc_, uint32_t* cq_head)
{
    while (true) {
        uint32_t head = cq->head.load(simt::memory_order_relaxed);

        for (size_t i = 0; i < cq->qs_minus_1; i++) {
            uint32_t cur_head = head + i;
            bool search_phase = (~(cur_head >> cq->qs_log2)) & 0x01;
            uint32_t loc = cur_head & cq->qs_minus_1;

            uint32_t cpl_entry = ((nvm_cpl_t*)cq->vaddr)[loc].dword[3];
            uint32_t cid = cpl_entry & 0x0000ffff;
            bool phase = (cpl_entry & 0x00010000) >> 16;

            // CID와 phase tag가 일치하면 완료
            if ((cid == search_cid) && (phase == search_phase)) {
                *cq_head = head;
                *loc_ = cur_head;
                return loc;
            }
            if (phase != search_phase)
                break;
        }
        __nanosleep(ns);  // 지수 백오프
    }
}
```

### Step 11: 데이터 반환 경로

캐시 히트든 미스든, 최종적으로 `get_cache_page_addr()`로 GPU 메모리 내 데이터 주소를 얻는다.

```cpp
__device__
uint64_t get_cache_page_addr(uint32_t page_trans) const {
    return (uint64_t)(cache.base_addr + (page_trans * cache.page_size));
}
```

## PCIe P2P 데이터 전송 상세

```
┌──────────────┐     PCIe Bus     ┌──────────────┐
│   GPU        │◄────────────────►│  NVMe SSD    │
│              │                  │              │
│  GPU 메모리  │                  │  NVMe 컨트롤 │
│  ┌────────┐  │   DMA Write     │  ┌────────┐  │
│  │ SQ     │──┼─────────────────┼──│ SQ 처리│  │
│  │ (도어벨)│  │   MMIO Write    │  │        │  │
│  │        │──┼─────────────────┼──│ 도어벨 │  │
│  └────────┘  │                  │  └────────┘  │
│              │                  │              │
│  ┌────────┐  │   DMA Write     │  ┌────────┐  │
│  │ CQ     │◄─┼─────────────────┼──│ CQ 작성│  │
│  └────────┘  │                  │  └────────┘  │
│              │                  │              │
│  ┌────────┐  │   DMA Write     │  ┌────────┐  │
│  │캐시    │◄─┼─────────────────┼──│데이터  │  │
│  │데이터  │  │  (P2P DMA)      │  │전송    │  │
│  └────────┘  │                  │  └────────┘  │
└──────────────┘                  └──────────────┘
```

**데이터 전송 경로**:
1. GPU 스레드가 SQ 슬롯에 명령을 기록 → SQ는 GPU 메모리에 위치
2. GPU 스레드가 도어벨 레지스터에 MMIO 쓰기 → PCIe를 통해 NVMe 컨트롤러의 BAR0에 도달
3. NVMe 컨트롤러가 DMA로 GPU 메모리의 SQ에서 명령을 읽음
4. NVMe 컨트롤러가 플래시에서 데이터를 읽어 DMA로 GPU 메모리(캐시 영역)에 직접 기록 (PCIe P2P)
5. NVMe 컨트롤러가 DMA로 GPU 메모리의 CQ에 완료 엔트리를 기록
6. GPU 스레드가 CQ를 폴링하여 완료를 감지

**핵심**: 모든 데이터 전송이 PCIe 버스를 통한 P2P DMA로 이루어지며, 호스트 메모리를 경유하지 않는다. `nvidia_p2p_get_pages()`와 `nvidia_p2p_dma_map_pages()`가 GPU 메모리의 물리 주소를 NVMe 컨트롤러가 접근 가능한 DMA 주소로 변환한다.

## 읽기 흐름 함수 호출 체인 요약

```
GPU 커널
└── array_d_t<T>::seq_read()                  (page_cache.h:1541)
    ├── find_range(i)                         (page_cache.h:1318)
    ├── range_d_t<T>::get_page(i)             (page_cache.h:1063)
    ├── range_d_t<T>::get_subindex(i)         (page_cache.h:1070)
    ├── range_d_t<T>::get_global_address()    (page_cache.h:1077)
    ├── array_d_t<T>::coalesce_page()         (page_cache.h:1332)
    │   ├── __activemask()
    │   ├── __match_any_sync(mask, gaddr)
    │   ├── __ffs(eq_mask) - 1                (master 선출)
    │   ├── __popc(eq_mask)                   (count 계산)
    │   └── range_d_t<T>::acquire_page()      (page_cache.h:1122)
    │       ├── [캐시 히트] return page_trans
    │       └── [캐시 미스]
    │           ├── page_cache_d_t::find_slot()  (page_cache.h:1786)
    │           │   └── [dirty 교체 시] write_data()  (page_cache.h:2057)
    │           └── read_data()                  (page_cache.h:2009)
    │               ├── get_cid()                (nvm_parallel_queue.h:36)
    │               ├── nvm_cmd_header()         (nvm_cmd.h:55)
    │               ├── nvm_cmd_data_ptr()       (nvm_cmd.h:67)
    │               ├── nvm_cmd_rw_blks()        (nvm_cmd.h:83)
    │               ├── sq_enqueue()             (nvm_parallel_queue.h:168)
    │               │   ├── in_ticket.fetch_add(1)
    │               │   ├── [티켓 대기]
    │               │   ├── [명령 복사]
    │               │   ├── tail_mark[pos] = LOCKED
    │               │   ├── move_tail()          (nvm_parallel_queue.h:60)
    │               │   └── *(sq->db) = new_tail (도어벨 링)
    │               ├── cq_poll()                (nvm_parallel_queue.h:379)
    │               │   └── [CID + phase 매칭 대기]
    │               ├── cq_dequeue()             (nvm_parallel_queue.h:425)
    │               │   ├── head_mark[pos] = LOCKED
    │               │   ├── move_head_cq()       (nvm_parallel_queue.h:83)
    │               │   └── *(cq->db) = new_head (CQ 도어벨)
    │               ├── enqueue_second()         (page_cache.h:1961)
    │               └── put_cid()                (nvm_parallel_queue.h:55)
    ├── __shfl_sync(eq_mask, base_master, master)
    ├── range_d_t<T>::get_cache_page_addr()   (page_cache.h:1107)
    ├── 데이터 읽기: ret = addr[subindex]
    ├── range_d_t<T>::release_page()          (page_cache.h:1083)
    └── return ret
```

## 성능 최적화 포인트

1. **Warp-level I/O Coalescing**: `__match_any_sync()`으로 같은 페이지를 접근하는 스레드들을 합쳐 I/O 횟수를 줄임
2. **Lock-free CID 할당**: `cid_ticket.fetch_add()` + `fetch_or(LOCKED)` 패턴으로 경합 최소화
3. **지수 백오프**: `__nanosleep(ns)`에서 `ns`를 8→256까지 2배씩 증가시켜 바쁜 대기 비용 절감
4. **MMIO asm 최적화**: `st.mmio.relaxed.sys.global.u32` 인라인 어셈블리로 도어벨 쓰기 최적화
5. **더블 버퍼링**: `enqueue_second()`로 읽기 완료 후 동일 LBA에 대한 추가 읽기를 즉시 큐잉하여 프리페치 효과
