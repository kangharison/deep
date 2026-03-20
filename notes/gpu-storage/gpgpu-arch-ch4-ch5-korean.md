# Chapter 4: 메모리 시스템 (Memory System)

이 장에서는 GPU의 메모리 시스템을 탐구한다. GPU 컴퓨팅 커널은 load 및 store 명령어를 통해 메모리 시스템과 상호작용한다. CPU는 일반적으로 레지스터 파일(register file)과 메모리(memory)라는 두 개의 분리된 메모리 공간을 포함한다. 현대 GPU는 메모리를 논리적으로 로컬 메모리(local memory)와 글로벌 메모리(global memory) 공간으로 더 세분화한다. 로컬 메모리 공간은 스레드별로 사적(private)이며 일반적으로 레지스터 스필링(register spilling)에 사용되고, 글로벌 메모리는 다수의 스레드 간에 공유되는 데이터 구조에 사용된다. 현대 GPU는 일반적으로 협력 스레드 배열(cooperative thread array)에서 함께 실행되는 스레드들 간에 공유 접근이 가능한, 프로그래머가 관리하는 스크래치패드 메모리(scratchpad memory)를 구현한다.

## 4.1 1차 레벨 메모리 구조 (First-Level Memory Structures)

### 4.1.1 스크래치패드 메모리와 L1 데이터 캐시 (Scratchpad Memory and L1 Data Cache)

CUDA 프로그래밍 모델에서 "공유 메모리(shared memory)"는 낮은 지연시간(low latency)을 가질 것으로 기대되지만 주어진 CTA 내의 모든 스레드가 접근 가능한 비교적 작은 메모리 공간을 의미한다. 공유 메모리는 레인(lane)당 하나의 뱅크(bank)를 가진 SRAM으로 구현되며, 각 뱅크는 하나의 읽기 포트(read port)와 하나의 쓰기 포트(write port)를 갖는다. 뱅크 충돌(bank conflict)은 하나 이상의 스레드가 주어진 사이클에서 같은 뱅크에 접근하면서 서로 다른 위치에 접근하려 할 때 발생한다.

L1 데이터 캐시는 글로벌 메모리 주소 공간의 일부를 유지한다. 워프(warp) 내의 모든 스레드가 단일 L1 데이터 캐시 블록 내에 속하는 위치에 접근하고 해당 블록이 캐시에 존재하지 않는 경우, 하위 레벨 캐시로 단 하나의 요청만 전송하면 된다. 이러한 접근을 "합쳐진(coalesced)" 접근이라 한다. 워프 내의 스레드들이 서로 다른 캐시 블록에 접근하면 다수의 메모리 접근이 생성되어야 한다. 이러한 접근을 합쳐지지 않은(uncoalesced) 접근이라 한다.

Figure 4.1은 통합된 공유 메모리와 L1 데이터 캐시를 구현하는 GPU 캐시 구성을 보여준다. 이 설계는 뱅크 충돌과 L1 데이터 캐시 미스를 처리할 때 리플레이(replay) 메커니즘을 사용하여 명령어 파이프라인과의 비정지(non-stalling) 인터페이스를 지원한다.

공유 메모리 접근의 경우, 아비터(arbiter)가 요청된 주소들이 뱅크 충돌을 일으킬지 여부를 판단한다. 충돌이 발생하면 요청을 두 부분으로 분할한다 — 충돌하지 않는 부분은 처리되고, 충돌하는 부분은 "리플레이(replay)"를 위해 반환된다.

캐시 읽기 연산의 경우, 로드/스토어 유닛(load/store unit)이 메모리 주소를 계산하고 합병(coalescing) 규칙을 적용한다. 보류 요청 테이블(pending request table, PRT)은 전통적인 미스 상태 보관 레지스터(miss-status holding registers, MSHRs)와 유사한 기능을 제공한다. L1 데이터 캐시는 가상 인덱싱 및 가상 태깅(virtually indexed and virtually tagged) 방식이다.

### 4.1.2 L1 텍스처 캐시 (L1 Texture Cache)

텍스처 캐시 태그 배열(tag array)은 데이터 배열(data array)보다 앞서 동작한다. 태그 배열의 내용은 미스 요청이 메모리까지 왕복하는 시간과 대략 동일한 시간이 지난 후 데이터 배열이 포함할 내용을 반영한다. 처리량(throughput)은 향상되지만, 캐시 히트와 미스 모두 대략 동일한 지연시간을 경험한다.

### 4.1.3 통합 텍스처 및 데이터 캐시 (Unified Texture and Data Cache)

NVIDIA와 AMD의 최근 GPU 아키텍처에서는 데이터와 텍스처 값의 캐싱이 통합된 L1 캐시 구조를 사용하여 수행된다.

## 4.2 온칩 상호연결 네트워크 (On-Chip Interconnection Network)

고성능 GPU는 메모리 파티션 유닛(memory partition unit)을 통해 여러 DRAM 칩에 병렬로 연결된다. SIMT 코어는 온칩 상호연결 네트워크(on-chip interconnection network)를 통해 메모리 파티션 유닛에 연결된다. NVIDIA는 크로스바(crossbar)를 사용하고, AMD는 때때로 링 네트워크(ring network)를 사용해왔다.

## 4.3 메모리 파티션 유닛 (Memory Partition Unit)

각 메모리 파티션 유닛은 L2 캐시의 일부와 하나 이상의 메모리 접근 스케줄러(memory access scheduler) 및 래스터 연산(raster operation, ROP) 유닛을 포함한다.

### 4.3.1 L2 캐시 (L2 Cache)

각 메모리 파티션 내부의 L2 캐시 부분은 두 개의 슬라이스(slice)로 구성된다. 각 캐시 라인은 GDDR5 DRAM 원자 크기(atom size)에 맞는 네 개의 32바이트 섹터(sector)를 갖는다.

### 4.3.2 원자적 연산 (Atomic Operations)

ROP 유닛은 원자적(atomic) 연산 및 리덕션(reduction) 연산을 실행하기 위한 기능 유닛을 포함하며, 파이프라이닝을 위한 로컬 ROP 캐시를 갖는다.

### 4.3.3 메모리 접근 스케줄러 (Memory Access Scheduler)

DRAM은 각각 자체 행 버퍼(row buffer)를 가진 다수의 뱅크를 포함한다. 메모리 접근 스케줄러는 DRAM 메모리 접근 요청을 재정렬하여 행 전환 패널티(row switch penalty)를 줄인다. 각 스케줄러는 읽기 요청 정렬기(read request sorter)와 읽기 요청 저장소(read request store)를 사용하여 읽기 및 쓰기 요청을 분류하는 별도의 로직을 포함한다.

## 4.4 GPU 메모리 시스템 연구 방향 (Research Directions for GPU Memory Systems)

### 4.4.1 메모리 접근 스케줄링 및 상호연결 네트워크 설계

Yuan 등은 메모리 접근 스케줄러 설계를 탐구하며, 단일 SM에서 오는 요청은 행 버퍼 지역성(row-buffer locality)을 가지지만 다른 SM의 요청과 섞이면 이 지역성이 손실된다는 점을 관찰했다. Bakhoda 등은 온칩 상호연결 네트워크를 탐구하며 GPU 트래픽이 다대소대다(many-to-few-to-many) 패턴을 가진다는 것을 발견했다.

### 4.4.2 캐싱 효과 (Caching Effectiveness)

Bakhoda 등은 L1 및/또는 L2 캐시 추가의 영향을 연구했다. Jia 등은 NVIDIA Fermi 하드웨어에서 캐싱 효과를 특성화하고 세 가지 형태의 지역성(locality) — 워프 내(within-warp), 블록 내(within-block), 명령어 간(cross-instruction) — 에 대한 분류법(taxonomy)을 도입했다.

### 4.4.3 메모리 요청 우선순위 지정 및 캐시 바이패싱

Jia 등은 미스가 발생하고 연관도 정지(associativity stall)로 인해 캐시 블록을 할당할 수 없을 때 L1 데이터 캐시를 바이패스(bypass)하는 방법을 제안했다. 또한 "메모리 요청 우선순위 버퍼(memory request prioritization buffer, MRPB)"를 제안했다.

### 4.4.4 워프 간 이질성 활용 (Exploiting Inter-Warp Heterogeneity)

Ausavarungnirun 등은 워프 간 메모리 지연시간 발산(memory latency divergence)의 이질성을 활용하는 메모리 발산 보정(Memory Divergence Correction, MeDiC)을 제안했다. 워프는 전체 히트(all-hit), 대부분 히트(mostly-hit), 균형(balanced), 대부분 미스(mostly-miss), 전체 미스(all-miss)로 분류된다.

### 4.4.5 조율된 캐시 바이패싱 (Coordinated Cache Bypassing)

Xie 등은 프로파일링을 사용하여 각 정적 load 명령어를 좋은(good), 나쁜(poor), 보통(moderate) 지역성으로 표시함으로써 선택적으로 캐시 바이패싱을 활성화하는 방법을 탐구했다.

### 4.4.6 적응형 캐시 관리 (Adaptive Cache Management)

Chen 등은 보호 거리(protection distance)를 사용한 조율된 캐시 바이패싱과 워프 스로틀링(warp throttling)을 제안했다.

### 4.4.7 캐시 우선순위 지정 (Cache Prioritization)

Li 등은 어떤 워프가 L1 캐시에 라인을 할당할 수 있는지를 결정하기 위해 워프에 토큰(token)을 할당하는 방법을 제안했다. 추가적인 "비오염 워프(non-polluting warps)"는 실행할 수 있지만 데이터를 축출(evict)할 수 없다.

### 4.4.8 가상 메모리 페이지 배치 (Virtual Memory Page Placement)

Agarwal 등은 용량 최적화(capacity-optimized) 메모리와 대역폭 최적화(bandwidth-optimized) 메모리를 모두 갖춘 이기종 시스템에서의 페이지 배치를 고려했다.

### 4.4.9 데이터 배치 (Data Placement)

Chen 등은 명세 언어(specification language), 소스-투-소스 컴파일러(source-to-source compiler), 적응형 런타임 데이터 배치기(adaptive runtime data placer)를 갖춘 이식 가능한 데이터 배치 전략인 PORPLE을 제안했다.

### 4.4.10 다중 칩 모듈 GPU (Multi-Chip-Module GPUs)

Arunkumar 등은 무어의 법칙(Moore's Law) 한계를 넘어 성능 확장을 확장하기 위해 멀티칩 모듈(multichip module) 위에 작은 GPU 모듈들로 대형 GPU를 구축하는 방법을 제안했다.

# Chapter 5: GPU 컴퓨팅 아키텍처에 대한 횡단적 연구 (Crosscutting Research on GPU Computing Architectures)

이 장에서는 이전 장에 깔끔하게 맞지 않는 GPGPU 아키텍처의 여러 연구 방향을 상세히 다룬다.

## 5.1 스레드 스케줄링 (Thread Scheduling)

현대 GPU는 대규모 병렬성(massive parallelism)에 의존한다. 스레드가 조직되고 스케줄링되는 세 가지 주요 방식이 있다:

1. 스레드를 워프에 할당 (Assignment of Threads to Warps)
2. 스레드블록을 코어에 동적 할당 — 작업이 스레드블록(threadblock) 단위로 대량 할당됨 (Dynamic Assignment of Threadblocks to Cores)
3. 사이클별 스케줄링 결정 (Cycle-by-cycle Scheduling Decisions)
4. 다중 커널 스케줄링 (Scheduling Multiple Kernels)

### 5.1.1 스레드블록의 코어 할당에 관한 연구

Kayiran 등은 메모리 경합(memory contention)을 줄이기 위해 스레드블록 수를 스로틀링(throttling)하는 방법을 제안했다. Sethia와 Mahlke는 자원 경합을 동적으로 모니터링하고 스레드, 코어 주파수, 메모리 주파수를 조정하는 Equalizer를 제안했다.

### 5.1.2 사이클별 스케줄링 결정에 관한 연구

Lakshminarayana와 Kim은 다양한 워프 스케줄링 정책을 탐구했다. Gebhart 등은 워프를 활성(active) 풀과 비활성(inactive) 풀로 나누는 2단계 스케줄링(two-level scheduling)을 도입했다. Rogers 등은 빅팀 태그(victim tag) 기반의 손실 지역성 감지 메커니즘을 사용하여 메모리 시스템 피드백에 기반해 활성 워프를 스로틀링하는 캐시 인식 워프 스케줄링(Cache-Conscious Warp Scheduling, CCWS)을 제안했다. Rogers 등은 이후 CCWS를 확장한 발산 인식 워프 스케줄링(Divergence-Aware Warp Scheduling, DAWS)을 제안했다. Jog 등은 프리페칭 인식(prefetching-aware) 및 CTA 인식(CTA-aware) 워프 스케줄링을 탐구했다. Sethia 등은 메모리 인식 스케줄링과 캐시 접근 재실행을 결합한 Mascar를 소개했다.

### 5.1.3 다중 커널 스케줄링에 관한 연구

Park 등은 GPU에서 선점형 멀티태스킹(preemptive multitasking)을 지원하기 위한 Chimera를 제안했으며, 전체 컨텍스트 저장/복원(full context save/store), 완료 대기(waiting until finish), 또는 저장 없이 중지(멱등성인 경우)(stopping without saving, if idempotent) 중에서 동적으로 선택한다.

### 5.1.4 세밀한 동기화 인식 스케줄링 (Fine-Grain Synchronization Aware Scheduling)

ElTantawy와 Aamodt는 스핀 락(spin lock)을 동적으로 식별하고 스핀 루프를 실행하는 워프의 우선순위를 낮추는 하드웨어를 제안했다.

## 5.2 병렬성을 표현하는 대안적 방법 (Alternative Ways of Expressing Parallelism)

Kim과 Batten은 세밀한 하드웨어 워크리스트(fine-grain hardware worklist)를 제안했다. Lee 등은 중첩 병렬 패턴의 지역성 인식 매핑(Locality-Aware Mapping of Nested Parallel Patterns)을 제안했다. Wang과 Yalamanchili는 CUDA 동적 병렬성(Dynamic Parallelism)의 오버헤드를 특성화했다. Wang 등은 동적 스레드 블록 실행(Dynamic Thread Block Launch, DTBL)을 제안했다.

## 5.3 트랜잭셔널 메모리 지원 (Support for Transactional Memory)

### 5.3.1 Kilo TM

Kilo TM은 GPU 아키텍처를 위해 발표된 최초의 하드웨어 TM 제안이다. 값 기반 충돌 감지(value-based conflict detection)를 사용하며, 수백 개의 비충돌 트랜잭션에 대한 커밋 병렬성을 높이기 위해 최신성 블룸 필터(recency bloom filter)를 도입했다.

### 5.3.2 Warp TM과 시간적 충돌 감지 (Warp TM and Temporal Conflict Detection)

Xu 등은 GPU 소프트웨어 트랜잭셔널 메모리(STM) 시스템을 제안했다. 이 논문은 두 가지 핵심 최적화를 제안한다: (1) 같은 워프 내 스레드 간의 읽기-쓰기 충돌을 감지하는 워프 내 충돌 감지(intra-warp conflict detection), 그리고 (2) 트랜잭션의 시간적 순서를 활용하여 충돌 감지의 거짓 양성률(false positive rate)을 줄이는 시간적 충돌 감지(temporal conflict detection)이다.

## 5.4 이기종 시스템 (Heterogeneous Systems)

Power 등은 통합 CPU-GPU 시스템을 위한 이기종 시스템 일관성 프로토콜(heterogeneous system coherence protocol)을 제안했다. Lee와 Kim은 TLP 인식 캐시 관리 정책인 TAP를 제안했다. Qureshi와 Patt는 유틸리티 기반 캐시 파티셔닝(utility-based cache partitioning)을 제안했다.

# 저자 소개 (Authors' Biographies)

Tor M. Aamodt는 브리티시 컬럼비아 대학교(University of British Columbia)의 교수이다. 그는 널리 사용되는 GPGPU-Sim 시뮬레이터를 개발했다. 그는 NVIDIA에서 CUDA를 지원하는 최초의 NVIDIA GPU인 GeForce 8 시리즈 GPU의 메모리 시스템 아키텍처 작업에 참여했다.

Wilson Wai Lun Fung은 삼성전자(Samsung Electronics)의 아키텍트로, 차세대 GPU IP 개발에 기여하고 있다. 그는 GPGPU-Sim의 주요 기여자 중 한 명이었다.

Timothy G. Rogers는 퍼듀 대학교(Purdue University)의 조교수로, 대규모 멀티스레드 프로세서 설계에 집중하고 있다. 그는 NVIDIA Research와 AMD Research에서 인턴을 했다.
