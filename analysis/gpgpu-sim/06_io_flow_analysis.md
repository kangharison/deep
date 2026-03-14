# GPGPU-Sim I/O 흐름 상세 분석

## 1. 전체 I/O 흐름도

GPGPU-Sim에서 데이터가 CUDA 애플리케이션으로부터 최종 DRAM까지 이동하는 전체 경로를 다이어그램으로 표현한다.

```
 ┌─────────────────────────────────────────────────────────────────────────────────┐
 │                          CUDA Application                                       │
 │  cudaMemcpy() → cudaLaunch() → kernel<<<grid,block>>>()                        │
 └───────────────────────────────────┬─────────────────────────────────────────────┘
                                     │ CUDA Runtime (libcudart_gpgpu-sim.so)
                                     ▼
 ┌─────────────────────────────────────────────────────────────────────────────────┐
 │                         gpgpu_sim::cycle()  [메인 시뮬레이션 루프]               │
 │                                                                                 │
 │  매 사이클마다 4개 클록 도메인을 순환:                                            │
 │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐                        │
 │  │   CORE   │  │   ICNT   │  │   DRAM   │  │    L2    │                        │
 │  │(Shader)  │  │(Intercon)│  │(Memory)  │  │(Cache)   │                        │
 │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘                        │
 └───────┼─────────────┼─────────────┼─────────────┼──────────────────────────────┘
         │             │             │             │
         ▼             ▼             ▼             ▼
 ┌───────────────────────────────────────────────────────────────────────────────┐
 │                        데이터 이동 상세 경로                                  │
 │                                                                               │
 │  ┌─────────┐    ┌──────────┐    ┌──────────────┐    ┌──────────────────────┐  │
 │  │ Shader  │    │ Intercon │    │  Memory      │    │       DRAM           │  │
 │  │ Core    │───▶│ Network  │───▶│  Partition    │───▶│  (HBM/GDDR5)       │  │
 │  │ (SM)    │◀───│ (ICNT)   │◀───│  (L2 Cache)  │◀───│                     │  │
 │  └─────────┘    └──────────┘    └──────────────┘    └──────────────────────┘  │
 │       │                                                                       │
 │  ┌────┴────────────────────────────────────────┐                              │
 │  │ SM 내부 구조:                                │                              │
 │  │  Warp Scheduler → Scoreboard → LDST Unit    │                              │
 │  │       │                            │         │                              │
 │  │       ▼                            ▼         │                              │
 │  │  Operand Collector        L1D/L1T/L1C Cache  │                              │
 │  │       │                            │         │                              │
 │  │       ▼                    Miss Queue        │                              │
 │  │  Execution Units          (→ ICNT로 전송)     │                              │
 │  └─────────────────────────────────────────────┘                              │
 └───────────────────────────────────────────────────────────────────────────────┘
```

### 상세 Load 경로 (Global Memory Read)

```
 Thread (LD 명령어 실행)
   │
   ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 1. Warp Scheduler가 메모리 명령어를 LDST Unit으로 dispatch            │
 │    → ldst_unit::cycle() 에서 memory_cycle() 호출                     │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 2. Coalescing: 32개 스레드의 주소를 합쳐서 mem_access_t 생성          │
 │    → inst.accessq_back()에서 coalesced access 획득                   │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 3a. L1D Cache 경로 (bypassL1D == false):                             │
 │     process_memory_access_queue_l1cache() 호출                       │
 │     → L1 latency queue에 삽입 (l1_latency_queue[bank][latency-1])   │
 │     → L1_latency_queue_cycle()에서 매 사이클 이동                    │
 │     → cache->access()로 L1 캐시 조회                                 │
 │     → HIT: m_L1D->next_access()로 즉시 반환                         │
 │     → MISS: miss queue → ICNT로 전송                                │
 │                                                                      │
 │ 3b. L1 Bypass 경로 (bypassL1D == true, .nc 접근 등):                 │
 │     → mem_fetch 패킷 생성 (m_mf_allocator->alloc)                   │
 │     → m_icnt->push(mf) 로 직접 ICNT에 삽입                          │
 │     상태: MEM_FETCH_INITIALIZED                                      │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 4. Interconnect Network (ICNT)                                       │
 │    → icnt_push(): SM에서 Memory Partition 방향으로 패킷 전송          │
 │    → icnt_transfer(): 매 ICNT 클록에서 네트워크 내부 이동              │
 │    상태: IN_ICNT_TO_MEM                                              │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 5. Memory Partition 진입                                             │
 │    → icnt_pop()으로 패킷 수신                                        │
 │    → memory_sub_partition::push(mf, cycle)                          │
 │    → 텍스처: 바로 icnt_L2_queue에 삽입                               │
 │      상태: IN_PARTITION_ICNT_TO_L2_QUEUE                             │
 │    → 비텍스처: ROP delay queue에 삽입 (rop_latency 만큼 지연)         │
 │      상태: IN_PARTITION_ROP_DELAY                                    │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 6. L2 Cache 조회 (cache_cycle 내부)                                  │
 │    → m_icnt_L2_queue에서 pop                                        │
 │    → m_L2cache->access()                                            │
 │    → HIT: set_reply() → m_L2_icnt_queue에 push                     │
 │      상태: IN_PARTITION_L2_TO_ICNT_QUEUE                             │
 │    → MISS: m_L2_dram_queue에 push                                   │
 │      상태: IN_PARTITION_L2_MISS_QUEUE → IN_PARTITION_L2_TO_DRAM_QUEUE│
 │    → RESERVATION_FAIL: 다음 사이클에 재시도                           │
 └──────────────┬───────────────────────────────────────────────────────┘
                │ (L2 Miss인 경우)
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 7. DRAM 접근                                                         │
 │    → memory_partition_unit::dram_cycle()에서 처리                    │
 │    → L2_dram_queue → dram_latency_queue (dram_latency 만큼 대기)    │
 │      상태: IN_PARTITION_DRAM_LATENCY_QUEUE                           │
 │    → dram_latency_queue → m_dram->push(mf) (DRAM 스케줄러)          │
 │      상태: IN_PARTITION_MC_INTERFACE_QUEUE → IN_PARTITION_DRAM       │
 │    → DRAM 타이밍 모델 (tRCD, tCAS, tRP 등) 적용                     │
 │    → 완료 후 return_queue에 삽입                                     │
 │      상태: IN_PARTITION_MC_RETURNQ                                   │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 8. DRAM → L2 반환                                                    │
 │    → dram_L2_queue에 push                                           │
 │      상태: IN_PARTITION_DRAM_TO_L2_QUEUE                             │
 │    → L2 fill: m_L2cache->fill(mf)                                  │
 │      상태: IN_PARTITION_L2_FILL_QUEUE                                │
 │    → L2에서 응답 생성 → m_L2_icnt_queue에 push                      │
 │      상태: IN_PARTITION_L2_TO_ICNT_QUEUE                             │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 9. ICNT 역방향 (Memory → Shader)                                     │
 │    → memory_sub_partition::pop()으로 L2_icnt_queue에서 꺼냄          │
 │    → set_reply() 호출 (READ_REQUEST → READ_REPLY)                   │
 │    → icnt_push(): Memory Partition에서 SM 방향으로 전송               │
 │      상태: IN_ICNT_TO_SHADER                                         │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 10. Shader Core 수신                                                 │
 │    → simt_core_cluster::icnt_cycle()에서 icnt_pop()                 │
 │    → m_response_fifo에 삽입                                          │
 │      상태: IN_CLUSTER_TO_SHADER_QUEUE                                │
 │    → accept_ldst_unit_response(mf)                                  │
 │    → ldst_unit::fill(mf) → m_response_fifo에 push                  │
 │      상태: IN_SHADER_LDST_RESPONSE_FIFO                              │
 └──────────────┬───────────────────────────────────────────────────────┘
                │
                ▼
 ┌──────────────────────────────────────────────────────────────────────┐
 │ 11. Writeback                                                        │
 │    → ldst_unit::cycle()에서 m_response_fifo 처리                    │
 │    → L1D fill 또는 bypass 경로로 m_next_global 설정                  │
 │      상태: IN_SHADER_FETCHED                                         │
 │    → ldst_unit::writeback()에서 m_next_wb 처리                      │
 │    → operand_collector->writeback()으로 레지스터 기록                 │
 │    → scoreboard->releaseRegister() (의존성 해제)                     │
 │    → warp_inst_complete() 호출                                      │
 └──────────────────────────────────────────────────────────────────────┘
```


## 2. 데이터 흐름 상세 분석

### 2.1 Load 명령어 경로

Load 명령어의 전체 실행 흐름을 소스 코드 기준으로 추적한다.

#### (1) 명령어 발행 ~ LDST Unit 진입

워프 스케줄러(gto 또는 lrr 정책)가 메모리 명령어를 선택하면, 해당 명령어는 ID_OC_MEM 파이프라인 레지스터를 거쳐 OC_EX_MEM으로 이동한다. 이때 Operand Collector가 소스 레지스터를 읽어 피연산자를 준비한다.

LDST Unit에 명령어가 도착하면 `ldst_unit::cycle()` 함수가 호출되며, 여기서 `memory_cycle()` 함수가 글로벌/로컬 메모리 접근을 처리한다.

```
함수 호출 체인:
shader_core_ctx::cycle()
  → execute()
    → ldst_unit::cycle()
      → writeback()                    // 먼저 완료된 요청 처리
      → response_fifo 처리              // ICNT에서 돌아온 응답 처리
      → memory_cycle(pipe_reg, ...)    // 새 메모리 요청 발행
```

#### (2) Coalescing과 mem_fetch 생성

`memory_cycle()` 함수(shader.cc:2260)에서는 명령어의 accessq(접근 큐)에서 coalesced 접근을 하나씩 꺼내 처리한다. 하드웨어에서와 마찬가지로 32개 스레드의 주소가 연속된 경우 하나의 128바이트 트랜잭션으로 합쳐진다.

`mem_fetch` 객체가 생성되며(mem_fetch.h:54), 이 객체는 메모리 요청의 전 생애를 추적하는 핵심 데이터 구조이다.

```cpp
// mem_fetch 주요 필드 (mem_fetch.h)
class mem_fetch {
    unsigned m_request_uid;        // 고유 요청 ID
    unsigned m_sid;                // 소스 Shader Core ID
    unsigned m_tpc;                // Thread Processing Cluster ID
    unsigned m_wid;                // Warp ID
    enum mem_fetch_status m_status; // 현재 위치/상태
    mem_access_t m_access;         // 접근 유형, 주소, 크기, 마스크
    new_addr_type m_partition_addr; // DRAM 파티션 내 주소
    addrdec_t m_raw_addr;          // 디코딩된 DRAM 주소 (chip-row-bank-column)
    enum mf_type m_type;           // READ_REQUEST/WRITE_REQUEST/READ_REPLY/WRITE_ACK
    unsigned m_timestamp;          // 생성 시각
    unsigned m_timestamp2;         // ICNT→Shader 반환 시각
    warp_inst_t m_inst;            // 원본 명령어 참조
};
```

#### (3) L1 Cache 경로 vs Bypass 경로

**L1 캐시 경유 경로** (`bypassL1D == false`):
- `process_memory_access_queue_l1cache()`가 호출된다 (shader.cc:2062).
- l1_latency 설정값(QV100: 20 사이클)에 따라 L1 latency queue에 삽입된다.
- `L1_latency_queue_cycle()`가 매 사이클 큐를 한 칸씩 이동시킨다.
- 큐 끝에 도달하면 `cache->access()`로 L1 캐시를 조회한다.
- HIT이면 즉시 `m_L1D->next_access()`로 writeback 경로에 진입한다.
- MISS이면 MSHR에 등록되고, miss queue를 통해 ICNT로 전송된다.

**L1 Bypass 경로** (`bypassL1D == true`):
- `CACHE_GLOBAL` 캐시 연산(.nc 접근)이거나 `gmem_skip_L1D` 옵션이 켜져 있으면 L1을 건너뛴다.
- `m_icnt->push(mf)`로 직접 인터커넥트에 삽입한다.

#### (4) Interconnect → Memory Partition

`gpgpu_sim::cycle()`의 L2 클록 도메인에서 `icnt_pop()`으로 메모리 파티션 쪽의 패킷을 꺼낸다 (gpu-sim.cc:2037). 꺼낸 패킷은 `memory_sub_partition::push()`를 통해 메모리 서브파티션에 진입한다.

비텍스처 접근은 ROP delay queue를 거친다. ROP latency는 QV100에서 160 사이클, GTX480에서 120 사이클이다.

#### (5) L2 Cache 조회

`memory_sub_partition::cache_cycle()`(l2cache.cc:465)에서 L2 캐시 접근이 이루어진다. 이 함수는 다음 순서로 동작한다:

1. **L2 fill response 처리**: L2 캐시에 fill이 완료된 요청을 L2_icnt_queue로 이동
2. **DRAM→L2 반환 처리**: dram_L2_queue에서 돌아온 데이터로 L2 fill 수행
3. **L2 캐시 자체 사이클**: MSHR 처리, eviction 등
4. **새 L2 접근 처리**: icnt_L2_queue에서 새 요청을 꺼내 L2 캐시 조회

```
L2 캐시 접근 결과에 따른 분기:
├── HIT (write 없음): set_reply() → L2_icnt_queue (응답 전송)
├── HIT (write 발생): L1_WRBK_ACC인 경우 삭제, 아니면 L2_icnt_queue
├── MISS (RESERVATION_FAIL 아님): L2_dram_queue로 이동 (DRAM 접근 필요)
│   └── Write + FETCH_ON_WRITE: write allocate read 생성
└── RESERVATION_FAIL: MSHR 또는 miss queue 포화 → 다음 사이클 재시도
```

#### (6) DRAM 접근과 반환

`memory_partition_unit::dram_cycle()`(l2cache.cc:306)에서 DRAM 접근이 처리된다.

```
DRAM 접근 순서:
1. return_queue에서 완료된 요청 꺼내기 → dram_L2_queue로 push
2. m_dram->cycle(): DRAM 스케줄러 실행 (FR-FCFS 등)
3. L2_dram_queue에서 새 요청 꺼내기 → dram_latency_queue로 push
   (dram_latency 만큼 대기, QV100: 100 사이클)
4. dram_latency_queue 만료된 요청 → m_dram->push(mf)
```

Arbitration metadata를 사용하여 여러 서브파티션 간 DRAM 접근을 공정하게 중재한다. 크레딧 기반으로, 각 서브파티션은 최소 1개의 private credit을 보유하고, 나머지는 shared credit pool에서 공유한다.

### 2.2 Store 명령어 경로

Store 명령어는 Load와 대부분 동일한 경로를 거치지만, 다음 차이점이 있다:

```
Store 경로의 차이점:
1. mem_fetch 타입이 WRITE_REQUEST로 생성됨
2. m_core->inc_store_req()로 pending store 카운트 증가
3. L1D 캐시: Write evict 정책 (write miss 시 allocate 안 함)
4. L2 캐시: Write-back 정책 (QV100 설정 기준)
   - FETCH_ON_WRITE: write miss 시 해당 라인을 먼저 읽어온 후 write
   - LAZY_FETCH_ON_READ: write miss 시 write 데이터만 기록
5. DRAM 응답: WRITE_ACK로 변환되어 반환됨
6. Shader 수신 시: store_ack()로 pending store 카운트 감소
```

Store의 writeback 처리(shader.cc:2859):
```cpp
if (mf->get_type() == WRITE_ACK) {
    m_core->store_ack(mf);          // store 완료 처리
    m_response_fifo.pop_front();
    delete mf;                       // mem_fetch 삭제
}
```

### 2.3 Atomic 연산 경로

Atomic 연산은 GLOBAL_ACC_R 접근 유형으로 처리되되, `isatomic()` 플래그가 설정된다.

```
Atomic 경로 특이점:
1. traffic_breakdown에서 "GLOBAL_ATOMIC"으로 분류됨
2. 전체 메모리 계층을 순회한 후 응답 수신 시:
   → memory_sub_partition::pop()에서 mf->do_atomic() 호출 (l2cache.cc:817)
   → 또는 SST 모드에서는 icnt_cycle_SST()에서 do_atomic() 호출
3. Writeback 시 decrement_atomic_count() 호출
4. Shared memory atomic: ldst_unit::writeback() case 0에서 do_atomic() 호출
```

## 3. 시뮬레이션 사이클별 데이터 이동

`gpgpu_sim::cycle()` 함수(gpu-sim.cc:1970)는 매 호출 시 클록 도메인 마스크에 따라 다음 순서로 실행된다.

### 사이클 실행 순서 (gpu-sim.cc 기준)

```
gpgpu_sim::cycle() 실행 순서:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[1단계] CORE 클록: Shader Core → ICNT 수신 (라인 1973-1977)
   for each cluster:
     cluster->icnt_cycle()
       → ICNT에서 응답 패킷 pop
       → response_fifo에 삽입
       → 해당 코어의 ldst_unit::fill()로 전달

[2단계] ICNT 클록: Memory Partition → ICNT 삽입 (라인 1979-2001)
   for each sub_partition:
     sub_partition->top()으로 L2_icnt_queue에서 응답 꺼냄
     → icnt_push()로 Shader 방향 ICNT에 삽입
     상태: IN_ICNT_TO_SHADER

[3단계] DRAM 클록: DRAM 사이클 실행 (라인 2004-2023)
   for each memory_partition:
     partition->dram_cycle() 또는 simple_dram_model_cycle()
       → DRAM return_queue → dram_L2_queue (응답 반환)
       → DRAM 스케줄러 실행
       → L2_dram_queue → dram_latency_queue (새 요청)
       → dram_latency_queue → DRAM push (만료된 요청)
     → 전력 통계 업데이트

[4단계] L2 클록: ICNT → Memory Partition + L2 캐시 사이클 (라인 2027-2047)
   for each sub_partition:
     icnt_pop()으로 Shader→Memory 방향 패킷 수신
     → sub_partition->push(mf)로 진입
       → ROP delay queue 또는 icnt_L2_queue
     sub_partition->cache_cycle()
       → L2 fill response 처리
       → DRAM→L2 반환 처리
       → L2 캐시 자체 사이클
       → 새 L2 접근 처리 (HIT/MISS/FAIL)

[5단계] ICNT 클록: 네트워크 전송 (라인 2054-2056)
   icnt_transfer()
     → 인터커넥트 내부 패킷 이동 (BookSim 시뮬레이터)

[6단계] CORE 클록: Shader Core 파이프라인 실행 (라인 2058-2077)
   for each cluster:
     cluster->core_cycle()
       → fetch / decode / issue / execute / writeback
       → ldst_unit::cycle()
         → writeback(): 완료된 메모리 응답 처리
         → response_fifo 소비: L1 fill 또는 bypass writeback
         → memory_cycle(): 새 메모리 요청 발행
       → L1 캐시 사이클
     → 전력/occupancy 통계 업데이트

[7단계] CORE 클록: 추가 처리 (라인 2097-2143)
   → AccelWattch 전력 모델 사이클 (McPAT 인터페이스)
   → issue_block2core(): 새 CTA를 코어에 할당
   → decrement_kernel_latency(): 커널 런치 레이턴시 감소
   → L1/L2 캐시 flush (모든 스레드 완료 시)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 클록 도메인 관계

```
QV100 (Volta) 클록 도메인:
  Core:  1132 MHz  ─── Shader Core 파이프라인, L1 캐시
  ICNT:  1132 MHz  ─── 인터커넥트 네트워크
  L2:    1132 MHz  ─── L2 캐시, Memory Partition 입구
  DRAM:   850 MHz  ─── DRAM 타이밍 모델

GTX480 (Fermi) 클록 도메인:
  Core:   700 MHz  ─── (1400/2, 파이프라인 16 EU이므로)
  ICNT:   700 MHz
  L2:     700 MHz
  DRAM:   924 MHz  ─── GDDR5

next_clock_domain()이 매 호출마다 어떤 도메인이 틱해야 하는지 비트마스크로 반환한다.
예: CORE|ICNT|L2|DRAM = 0x0F (모든 도메인 틱)
```

### 단일 Load 요청의 타임라인 예시 (L1 Miss, L2 Miss)

```
Cycle    위치                             상태
───────────────────────────────────────────────────────────────
  0      LDST Unit dispatch              MEM_FETCH_INITIALIZED
  1~20   L1 Latency Queue 통과           (L1 latency = 20)
  21     L1 Cache Miss → ICNT push       IN_ICNT_TO_MEM
  22~30  ICNT traversal                  IN_ICNT_TO_MEM
  31     Memory Partition 도착            IN_PARTITION_ROP_DELAY
  31~191 ROP Delay                       (rop_latency = 160)
  192    icnt_L2_queue 진입              IN_PARTITION_ICNT_TO_L2_QUEUE
  193    L2 Cache Miss                   IN_PARTITION_L2_MISS_QUEUE
  194    L2_dram_queue 진입              IN_PARTITION_L2_TO_DRAM_QUEUE
  195    dram_latency_queue 진입         IN_PARTITION_DRAM_LATENCY_QUEUE
  195~295 DRAM latency 대기              (dram_latency = 100)
  296    DRAM push                       IN_PARTITION_DRAM
  297~+  DRAM 타이밍 (tRCD+tCAS 등)      IN_PARTITION_DRAM
  350    DRAM return_queue               IN_PARTITION_MC_RETURNQ
  351    dram_L2_queue                   IN_PARTITION_DRAM_TO_L2_QUEUE
  352    L2 fill                         IN_PARTITION_L2_FILL_QUEUE
  353    L2_icnt_queue                   IN_PARTITION_L2_TO_ICNT_QUEUE
  354    ICNT push (역방향)               IN_ICNT_TO_SHADER
  355~365 ICNT traversal (역방향)          IN_ICNT_TO_SHADER
  366    Cluster response_fifo           IN_CLUSTER_TO_SHADER_QUEUE
  367    LDST response_fifo              IN_SHADER_LDST_RESPONSE_FIFO
  368    L1D fill 또는 bypass             IN_SHADER_FETCHED
  369    Writeback (레지스터 기록)         (완료)
```

## 4. 메모리 요청 상태 전이도 (mem_fetch_status)

`mem_fetch_status.tup` 파일에 정의된 모든 상태와 전이 관계를 나타낸다.

```
mem_fetch_status 전체 상태 목록 (mem_fetch_status.tup):

  MEM_FETCH_INITIALIZED          ← mem_fetch 생성 시
  IN_L1I_MISS_QUEUE              ← L1 Instruction 캐시 miss
  IN_L1D_MISS_QUEUE              ← L1 Data 캐시 miss
  IN_L1T_MISS_QUEUE              ← L1 Texture 캐시 miss
  IN_L1C_MISS_QUEUE              ← L1 Constant 캐시 miss
  IN_L1TLB_MISS_QUEUE            ← L1 TLB miss
  IN_VM_MANAGER_QUEUE            ← VM 관리자 큐
  IN_ICNT_TO_MEM                 ← SM → Memory 인터커넥트 전송 중
  IN_PARTITION_ROP_DELAY         ← ROP 지연 큐 (비텍스처 접근)
  IN_PARTITION_ICNT_TO_L2_QUEUE  ← Memory Partition 내 ICNT→L2 큐
  IN_PARTITION_L2_TO_DRAM_QUEUE  ← L2 miss → DRAM 큐
  IN_PARTITION_DRAM_LATENCY_QUEUE← DRAM 접근 레이턴시 큐
  IN_PARTITION_L2_MISS_QUEUE     ← L2 miss 상태
  IN_PARTITION_MC_INTERFACE_QUEUE← 메모리 컨트롤러 인터페이스 큐
  IN_PARTITION_MC_INPUT_QUEUE    ← 메모리 컨트롤러 입력 큐
  IN_PARTITION_MC_BANK_ARB_QUEUE ← 뱅크 중재 큐
  IN_PARTITION_DRAM              ← DRAM 내부 처리 중
  IN_PARTITION_MC_RETURNQ        ← 메모리 컨트롤러 반환 큐
  IN_PARTITION_DRAM_TO_L2_QUEUE  ← DRAM → L2 반환 큐
  IN_PARTITION_L2_FILL_QUEUE     ← L2 캐시 fill 큐
  IN_PARTITION_L2_TO_ICNT_QUEUE  ← L2 → ICNT 응답 큐
  IN_ICNT_TO_SHADER              ← Memory → SM 인터커넥트 전송 중
  IN_CLUSTER_TO_SHADER_QUEUE     ← Cluster 내 응답 큐
  IN_SHADER_LDST_RESPONSE_FIFO  ← LDST Unit 응답 FIFO
  IN_SHADER_FETCHED              ← 데이터 fetch 완료
  IN_SHADER_L1T_ROB              ← L1 Texture ROB
  MEM_FETCH_DELETED              ← mem_fetch 삭제됨
  NUM_MEM_REQ_STAT               ← 통계용 총 상태 수
```

### 상태 전이 다이어그램

```
                    ┌─────────────────────┐
                    │ MEM_FETCH_INITIALIZED│
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
    ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
    │IN_L1D_MISS  │  │IN_L1T_MISS  │  │IN_L1I_MISS  │
    │   _QUEUE    │  │   _QUEUE    │  │   _QUEUE    │
    └──────┬──────┘  └──────┬──────┘  └──────┬──────┘
           │                │                │
           └────────────────┼────────────────┘
                            ▼
                 ┌─────────────────────┐
                 │   IN_ICNT_TO_MEM    │
                 └──────────┬──────────┘
                            │
                  ┌─────────┴─────────┐
                  ▼                   ▼
        ┌──────────────┐    ┌──────────────────────┐
        │IN_PARTITION  │    │IN_PARTITION           │
        │_ROP_DELAY    │    │_ICNT_TO_L2_QUEUE      │
        └──────┬───────┘    │(텍스처 직행)           │
               │            └──────────┬───────────┘
               └───────────────────────┘
                            │
                            ▼
                ┌───────────────────────┐
                │    L2 Cache Access    │
                ├───────────┬───────────┤
                │           │           │
                ▼           ▼           ▼
         ┌──────────┐ ┌──────────┐ ┌──────────────┐
         │  HIT     │ │  MISS    │ │RESERVATION   │
         │          │ │          │ │   FAIL       │
         └────┬─────┘ └────┬─────┘ └──────┬───────┘
              │            │              │(재시도)
              ▼            ▼              └──→ (이전 상태)
  ┌────────────────┐ ┌──────────────────┐
  │IN_PARTITION    │ │IN_PARTITION      │
  │_L2_TO_ICNT_Q  │ │_L2_TO_DRAM_Q    │
  └───────┬────────┘ └───────┬──────────┘
          │                  ▼
          │        ┌──────────────────────┐
          │        │IN_PARTITION          │
          │        │_DRAM_LATENCY_QUEUE   │
          │        └───────┬──────────────┘
          │                ▼
          │        ┌──────────────────────┐
          │        │IN_PARTITION_DRAM     │
          │        │(MC_INTERFACE/INPUT/  │
          │        │ BANK_ARB)            │
          │        └───────┬──────────────┘
          │                ▼
          │        ┌──────────────────────┐
          │        │IN_PARTITION_MC_RETURNQ│
          │        └───────┬──────────────┘
          │                ▼
          │        ┌──────────────────────┐
          │        │IN_PARTITION          │
          │        │_DRAM_TO_L2_QUEUE     │
          │        └───────┬──────────────┘
          │                ▼
          │        ┌──────────────────────┐
          │        │IN_PARTITION          │
          │        │_L2_FILL_QUEUE        │
          │        └───────┬──────────────┘
          │                ▼
          │        ┌──────────────────────┐
          │        │IN_PARTITION          │
          │        │_L2_TO_ICNT_QUEUE     │
          │        └───────┬──────────────┘
          │                │
          └────────────────┘
                   │
                   ▼
          ┌──────────────────┐
          │ IN_ICNT_TO_SHADER│
          └───────┬──────────┘
                  ▼
          ┌───────────────────────┐
          │IN_CLUSTER_TO_SHADER_Q │
          └───────┬───────────────┘
                  ▼
          ┌───────────────────────────┐
          │IN_SHADER_LDST_RESPONSE_FIFO│
          └───────┬───────────────────┘
                  ▼
          ┌───────────────────┐
          │ IN_SHADER_FETCHED │
          └───────┬───────────┘
                  ▼
          ┌───────────────────┐
          │MEM_FETCH_DELETED  │
          └───────────────────┘
```

## 5. 설정 파라미터가 I/O 성능에 미치는 영향

### 5.1 캐시 관련 파라미터

| 파라미터 | QV100 값 | 영향 |
|---------|---------|------|
| `-gpgpu_cache:dl1 S:4:128:64,L:T:m:L:L,A:512:8,16:0,32` | 섹터형, 4세트, 128B 블록, 64-way | L1D 용량/적중률 결정. 섹터 캐시(S)는 128B 라인을 32B 섹터로 분할하여 대역폭 절약 |
| `-gpgpu_l1_latency 20` | 20 사이클 | L1 접근 레이턴시. 높을수록 메모리 의존 명령어 지연 증가 |
| `-gpgpu_cache:dl2 S:32:128:24,L:B:m:L:P,A:192:4,32:0,32` | 섹터형, 32세트, 128B, 24-way | L2 총 용량 = 32x128x24 x 64(서브파티션) = 6MB |
| `-gpgpu_gmem_skip_L1D 0` | 비활성 | 1로 설정하면 모든 global 접근이 L1을 bypass |
| `-gpgpu_adaptive_cache_config 1` | 활성 | L1D와 Shared Memory 크기를 동적으로 조절 |

### 5.2 메모리 파티션/DRAM 관련 파라미터

| 파라미터 | QV100 값 | 영향 |
|---------|---------|------|
| `-gpgpu_n_mem 32` | 32개 | 메모리 파티션 수. 더 많으면 병렬성 증가 |
| `-gpgpu_n_sub_partition_per_mchannel 2` | 2개 | 채널당 서브파티션. QV100 총 64개 서브파티션 |
| `-gpgpu_l2_rop_latency 160` | 160 사이클 | ROP delay. SM→L2 진입 전 추가 레이턴시 |
| `-dram_latency 100` | 100 사이클 | DRAM 기본 레이턴시 (simple_dram_model 사용 시) |
| `-gpgpu_dram_scheduler 1` | FR-FCFS | DRAM 스케줄링 정책. 0=FIFO, 1=FR-FCFS |
| `-gpgpu_frfcfs_dram_sched_queue_size 64` | 64 | DRAM 스케줄러 큐 크기. MLP 허용 수준 결정 |
| `-gpgpu_dram_return_queue_size 192` | 192 | DRAM 반환 큐 크기. 대역폭 버퍼링 |
| `-gpgpu_dram_partition_queues 64:64:64:64` | icnt_L2:L2_dram:dram_L2:L2_icnt | 각 큐의 크기. 백프레셔 발생 지점 결정 |

### 5.3 DRAM 타이밍 파라미터 (HBM, 850 MHz)

| 파라미터 | 값 | 의미 |
|---------|---|------|
| `nbk=16` | 16 | 뱅크 수 |
| `CCD=1` | 1 | Column-to-Column Delay |
| `RRD=3` | 3 | Row-to-Row Delay (다른 뱅크) |
| `RCD=12` | 12 | Row-to-Column Delay (ACT→READ) |
| `RAS=28` | 28 | Row Active Time |
| `RP=12` | 12 | Row Precharge Time |
| `RC=40` | 40 | Row Cycle Time (RAS + RP) |
| `CL=12` | 12 | CAS Latency |
| `WL=2` | 2 | Write Latency |
| `WR=10` | 10 | Write Recovery Time |
| `nbkgrp=4` | 4 | 뱅크 그룹 수 (HBM) |

### 5.4 인터커넥트 파라미터

| 파라미터 | QV100 값 | 영향 |
|---------|---------|------|
| `-network_mode 2` | Local Crossbar | 1=BookSim 외부 설정, 2=내장 로컬 Xbar |
| `-icnt_in_buffer_limit 512` | 512 | 입력 버퍼 크기. 가득 차면 stall 발생 |
| `-icnt_out_buffer_limit 512` | 512 | 출력 버퍼 크기 |
| `-icnt_subnets 2` | 2 | 요청/응답 분리 서브넷 |
| `-icnt_flit_size 40` | 40 바이트 | Flit 크기. 패킷이 몇 개 flit으로 분할되는지 결정 |

### 5.5 성능 영향 요약

```
I/O 성능에 가장 큰 영향을 미치는 파라미터 (중요도 순):

1. gpgpu_l2_rop_latency (160):  모든 비텍스처 접근에 추가되는 고정 레이턴시
2. dram_latency (100):          DRAM 기본 접근 레이턴시
3. DRAM 타이밍 (RCD+CL):        실제 DRAM row/column 접근 시간
4. L1/L2 캐시 크기/연관도:       캐시 적중률이 DRAM 접근 빈도 결정
5. gpgpu_n_mem (32):            메모리 병렬성 (파티션 수)
6. 인터커넥트 대역폭:            icnt_flit_size와 클록 주파수에 의해 결정
7. DRAM 스케줄러 큐 크기 (64):   MLP(메모리 레벨 병렬성) 활용 수준
```

## 6. 트래픽 분류 체계

`traffic_breakdown` 클래스(traffic_breakdown.cc)는 인터커넥트를 통과하는 모든 트래픽을 다음과 같이 분류한다:

```
트래픽 유형 (mem_access_type 기반):
  CONST_ACC_R      : 상수 메모리 읽기
  TEXTURE_ACC_R    : 텍스처 메모리 읽기
  GLOBAL_ACC_R     : 글로벌 메모리 읽기 (또는 GLOBAL_ATOMIC)
  GLOBAL_ACC_W     : 글로벌 메모리 쓰기
  LOCAL_ACC_R      : 로컬 메모리 읽기
  LOCAL_ACC_W      : 로컬 메모리 쓰기
  INST_ACC_R       : 명령어 페치
  L1_WRBK_ACC      : L1 캐시 writeback
  L2_WRBK_ACC      : L2 캐시 writeback
  L1_WR_ALLOC_R    : L1 write allocate read
  L2_WR_ALLOC_R    : L2 write allocate read
```

통계는 `traffic_stat_t` 맵에 (패킷유형, {패킷크기: 횟수}) 형태로 누적되며, 시뮬레이션 종료 시 `traffic_breakdown_[network_name][type] = total_bytes {size:count,...}` 형식으로 출력된다.

## 7. Visualizer (AerialVision) 연동

`visualizer.cc`의 `gpgpu_sim::visualizer_printstat()` 함수는 주기적으로 gzip 압축 로그를 생성한다. 이 로그에는 다음 정보가 포함된다:

```
Visualizer 출력 항목:
  - CTA count per shader: cflog_visualizer_gzprint()
  - Memory partition 통계: DRAM, L2 sub-partition 별
  - Shader 통계: m_shader_stats->visualizer_print()
  - L2 cache 통계: read_hit, read_miss, write_hit, write_miss
  - Memory latency 통계: average mf latency
  - Power 통계: AccelWattch 출력
  - 메모리 레이턴시 분포: LDmemlatdist, STmemlatdist
    → Load/Store 각각의 지연 시간 분포를 구간별로 기록
  - 전역 사이클/명령어 수: globalcyclecount, globalinsncount
```

`my_time_vector` 클래스(visualizer.cc:118)는 각 mem_fetch의 UID별로 각 상태 전이 시각을 기록하여, Load/Store 메모리 레이턴시가 어디서 소비되는지 분석할 수 있게 한다. 이를 통해 레이턴시 병목 구간을 시각적으로 파악할 수 있다.
