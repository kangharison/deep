# 2년 초심자→시니어 스토리지 개발자 완전 정복 커리큘럼

## 커리큘럼 개요

**목표**: C언어 기본 지식만 가진 초심자가 24개월 후 Linux Storage Stack 시니어 개발자 수준에 도달한다. Application의 `write()`부터 NVMe SSD까지의 전체 I/O 경로를 코드 레벨로 이해하고, 커널 패치를 업스트림에 제출하며, SPDK와 GPU-Storage까지 설계 수준에서 다룰 수 있는 능력을 갖춘다.

**기간**: 24개월 (2026-03 ~ 2028-02)
**방식**: 이론 → 소스코드 라인바이라인 리딩 → QEMU 실험/검증 → 코드 수정 → 업스트림 기여

**소스코드 버전 기준**:
- Linux Kernel: `sources/linux` (현재 트리)
- QEMU: `sources/qemu` (현재 트리)
- SPDK: `sources/spdk` (비교 분석용)
- DPDK: `sources/dpdk` (비교 분석용)
- BaM: `sources/bam` (GPU-Storage)

**레벨 정의**:

```
Level 0 (입문)  : C 기본, OS 개념 학습 ← 여기서 시작
Level 1 (초급)  : 커널 빌드/부팅, GDB/ftrace 사용 가능
Level 2 (중급)  : I/O 경로 전체를 함수 단위로 설명 가능
Level 3 (중상급): 커널 코드 수정 → 빌드 → 테스트 가능
Level 4 (고급)  : SPDK/GPU-Storage 아키텍처 설계 가능
Level 5 (시니어): 업스트림 패치 제출, 아키텍처 리뷰, 성능 최적화 주도 ← 최종 목표
```

---

## 전체 로드맵 조감도

```
Month   1   2   3 │ 4   5   6 │ 7   8   9 │10  11  12 │13  14  15 │16  17  18 │19  20  21 │22  23  24
Phase ──────────── │────────── │────────── │────────── │────────── │────────── │────────── │──────────
  0: C/OS 기초     │           │           │           │           │           │           │
                   │ 1: Syscall│           │           │           │           │           │
                   │    VFS    │           │           │           │           │           │
                   │    Bio    │ 2: blk-mq │           │           │           │           │
                   │           │    NVMe   │           │           │           │           │
                   │           │    Compl  │           │           │           │           │
                   │           │           │ 3: QEMU   │           │           │           │
                   │           │           │    NVMe   │           │           │           │
                   │           │           │  io_uring │           │           │           │
                   │           │           │  Writeback│           │           │           │
                   │           │           │           │ 4: 커널   │           │           │
                   │           │           │           │    수정    │           │           │
                   │           │           │           │           │ 5: SPDK   │           │
                   │           │           │           │           │    DPDK   │           │
                   │           │           │           │           │           │ 6: 성능   │
                   │           │           │           │           │           │    엔지   │
                   │           │           │           │           │           │ 7: GPU    │
                   │           │           │           │           │           │    Storage│
                   │           │           │           │           │           │           │ 8: 시니어
                   │           │           │           │           │           │           │    실전
Level  ──0──  ──1──  ──1──  ──2──  ──2──  ──2──  ──2──  ──2──  ──3──  ──3──  ──3──  ──3──  ──3──  ──4──  ──4──  ──4──  ──4──  ──4──  ──4──  ──5──  ──5──  ──5──  ──5──  ──5──
```

---

## 전체 I/O 경로 조감도

```
┌─────────────────────────────────────────────────────────────────────┐
│  FIO / APPLICATION                                                   │
│  fio: ioengine 선택 (sync/libaio/io_uring/pvsync2)                  │
│  engines/sync.c → write()                                           │
│  engines/libaio.c → io_submit()                                     │
│  engines/io_uring.c → io_uring_enter()                              │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ syscall (SYSCALL instruction)
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  SYSCALL ENTRY  (arch/x86/entry/entry_64.S → do_syscall_64)        │
│  → sys_write() / sys_io_submit() / sys_io_uring_enter()            │
└──────────────────────────────┬──────────────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  VFS LAYER  (fs/read_write.c)                                       │
│  vfs_write() → new_sync_write() → file->f_op->write_iter()         │
└──────────────────────────────┬──────────────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  FILESYSTEM  (fs/ext4/file.c 또는 block_dev)                        │
│  ext4_file_write_iter() → ext4_dio_write_iter()                     │
│  또는 blkdev_write_iter() (raw block device)                        │
└──────────────────────────────┬──────────────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  PAGE CACHE / DIRECT I/O                                            │
│  Buffered: generic_perform_write() → mark_page_dirty()              │
│  Direct:   __iomap_dio_rw() → bio 생성                              │
└──────────────────────────────┬──────────────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  BIO LAYER  (block/bio.c)                                           │
│  bio_alloc() → bio_add_page() → submit_bio()                       │
└──────────────────────────────┬──────────────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  BLOCK LAYER (blk-mq)  (block/blk-mq.c, block/blk-core.c)         │
│  submit_bio() → __submit_bio() → blk_mq_submit_bio()               │
│  → blk_mq_get_request() → blk_mq_bio_to_request()                 │
│  → blk_mq_run_hw_queue() → __blk_mq_delay_run_hw_queue()          │
│  → blk_mq_dispatch_rq_list() → q->mq_ops->queue_rq()              │
└──────────────────────────────┬──────────────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  NVMe DRIVER  (drivers/nvme/host/pci.c, core.c)                    │
│  nvme_queue_rq() → nvme_setup_cmd() → nvme_map_data()              │
│  → nvme_submit_cmd() → writel(sq->tail, q->q_db)  ← doorbell       │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ writel() = MMIO store
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  PCIe LAYER  (CPU ↔ Device 물리 전송)                               │
│  CPU store → write combining buffer → Root Complex                  │
│  → PCIe TLP (Memory Write, 3DW/4DW header + 4B payload)            │
│  → Switch/Bridge → Endpoint (NVMe Controller)                       │
│  DMA: TLP Memory Read/Write (PRP 기반 데이터 전송)                   │
│  MSI-X: TLP Memory Write to LAPIC address                           │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ QEMU: memory_region MMIO trap
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  QEMU PCIe EMULATION  (hw/pci/, hw/pci-host/)                      │
│  MMIO dispatch: flatview → MemoryRegion → nvme_mmio_ops            │
│  DMA dispatch: pci_dma_read/write → address_space_rw               │
│  MSI-X: msix_notify() → MSI address/data → LAPIC                   │
└──────────────────────────────┬──────────────────────────────────────┘
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  QEMU NVMe DEVICE  (hw/nvme/ctrl.c)                                │
│  nvme_mmio_write() → nvme_process_sq() → nvme_rw()                 │
│  → dma_memory_read/write() → blk_aio_preadv/pwritev()              │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ Completion (반대 방향)
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  COMPLETION PATH (역방향)                                            │
│  QEMU: nvme_enqueue_req_completion() → nvme_post_cqes()             │
│        → msix_notify() → PCIe TLP(MSI-X) → LAPIC → CPU interrupt   │
│  KERNEL: nvme_irq() → nvme_process_cq() → nvme_pci_complete_rq()   │
│  → blk_mq_end_request() → bio_endio() → dio_complete()             │
│  → complete()/wakeup → fio td->io_u_completed++                    │
└─────────────────────────────────────────────────────────────────────┘
```

**SPDK 대비 경로** (Phase 5에서 상세):

```
┌─────────────────────────────────────────────────────────────────────┐
│  SPDK APPLICATION (perf, bdevperf)                                   │
│  spdk_nvme_ns_cmd_write() → nvme_qpair_submit_request()            │
│  → nvme_pcie_qpair_submit_request()                                 │
│  → writel(sq->tail, doorbell)  ← 커널 bypass, 직접 doorbell         │
│  Polling: spdk_nvme_qpair_process_completions() ← interrupt 없음    │
└─────────────────────────────────────────────────────────────────────┘
위 경로는 커널 I/O 경로의 VFS/Block/NVMe Driver를 모두 건너뛴다.
syscall 0회, context switch 0회, interrupt 0회.
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 0: 기초 체력 — C, OS, 컴퓨터 구조 (Month 1~3)
## ═══════════════════════════════════════════════════════════════

> **이 Phase가 없으면 나머지 전체가 "외우기"가 된다.**
> 시니어가 되려면 "왜 이렇게 만들었는가"를 판단하는 기초 체력이 필요하다.

### Chapter 0: C 언어 실전 & 개발 도구 (Week 1~4)

**목표**: 커널 코드를 읽는 데 필요한 C 패턴(포인터, 구조체, 매크로, 함수 포인터)과 빌드 시스템(Makefile, Kconfig)을 완벽히 익힌다.

**도달 수준**: 커널 소스의 아무 파일이나 열어서 "이 코드가 뭘 하는지" 5분 내에 파악 가능.

```
Week 1: C 포인터 & 메모리
─────────────────────────
1. 포인터 산술, 이중 포인터, void 포인터
   - 커널에서의 활용: container_of() 매크로
     include/linux/kernel.h:
     #define container_of(ptr, type, member) ...
     → 구조체 멤버 포인터에서 구조체 시작 주소를 역산
     → 커널 전체에서 수천 번 사용되는 핵심 패턴

2. 구조체 패딩과 메모리 레이아웃
   - sizeof, offsetof, __packed, __aligned
   - NVMe 명령 구조체가 왜 64바이트로 정렬되는지

3. 동적 메모리: kmalloc/kfree, slab allocator 개념
   - 유저공간 malloc과의 차이
   - GFP_KERNEL, GFP_ATOMIC 플래그

실습:
 □ container_of()를 직접 구현하고 테스트
 □ struct bio (include/linux/blk_types.h)의 메모리 레이아웃 그려보기
 □ Valgrind로 메모리 버그 탐지 연습

Week 2: 함수 포인터 & 콜백 패턴
──────────────────────────────
1. 함수 포인터 기초
   - int (*func)(int, int) 선언과 호출
   - typedef로 가독성 높이기

2. 커널의 Operations 패턴
   - struct file_operations: .read_iter, .write_iter, .open, .release
   - struct blk_mq_ops: .queue_rq, .complete, .init_hctx
   → "인터페이스"를 함수 포인터 테이블로 구현하는 C의 OOP
   → 이 패턴을 이해하면 커널 코드의 50%는 읽을 수 있다

3. 콜백 함수
   - bio->bi_end_io: I/O 완료 시 호출되는 콜백
   - timer_setup() + callback: 타이머 콜백

실습:
 □ 간단한 "가상 디바이스" 구현: ops 테이블 + read/write 콜백
 □ 커널 소스에서 file_operations 구조체 5개 찾아서 비교표 작성
 □ 콜백 체인 구현: A 완료 → B 호출 → C 호출

Week 3: 매크로 & 커널 자료구조
─────────────────────────────
1. 전처리기 매크로
   - #define, ##(토큰 결합), #(문자열화)
   - 가변 인자 매크로: pr_info(), pr_err()
   - 커널 특유 매크로: likely()/unlikely(), BUG_ON(), WARN_ON()

2. 커널 연결 리스트 (Linked List)
   - include/linux/list.h: struct list_head
   - list_add(), list_del(), list_for_each_entry()
   → 커널 전체에서 가장 많이 쓰이는 자료구조
   → bio 체인, request 리스트, NVMe queue 관리에 모두 사용

3. 커널 RB-Tree, radix tree
   - include/linux/rbtree.h: I/O 스케줄러의 deadline 관리
   - XArray (radix tree 후속): page cache 인덱싱

실습:
 □ list_head를 사용한 간단한 큐 구현 (유저 공간)
 □ BIO 체인이 list_head로 어떻게 연결되는지 코드에서 추적
 □ mq-deadline 스케줄러의 RB-tree 사용 부분 찾기

Week 4: 빌드 시스템 & 디버깅 도구
─────────────────────────────────
1. Makefile 기초
   - sources/linux/Makefile: 커널 빌드 진입점
   - obj-y, obj-m: 빌트인 vs 모듈
   - ccflags-y: 컴파일 플래그

2. Kconfig 시스템
   - sources/linux/drivers/nvme/host/Kconfig
   - CONFIG_BLK_DEV_NVME=y vs =m
   - make menuconfig 사용법

3. GDB 기초
   - break, step, next, print, x (메모리 덤프)
   - watchpoint: 특정 변수 변경 시 중단
   - 구조체 pretty-print: python gdb scripts

4. Git 기초 (업스트림 기여 준비)
   - git log --oneline --graph: 커밋 히스토리
   - git blame: 코드 라인별 작성자/커밋
   - git format-patch / git send-email

실습:
 □ 커널을 직접 빌드 (make -j$(nproc))
 □ drivers/nvme/host/Makefile 분석: 어떤 .o가 만들어지는지
 □ GDB로 간단한 C 프로그램 디버깅 연습
 □ git blame으로 nvme_queue_rq() 최근 변경 이력 추적
```

### Chapter 1: 컴퓨터 구조 & OS 핵심 개념 (Week 5~8)

**목표**: CPU, 메모리, PCIe, DMA, 인터럽트 등 하드웨어 기초와 프로세스/메모리 관리 등 OS 핵심을 이해한다. 이 지식 없이는 커널 코드를 읽어도 "왜" 이렇게 하는지 이해할 수 없다.

**도달 수준**: "DMA가 왜 필요한가?", "인터럽트 vs polling의 trade-off는?", "가상 메모리가 I/O에 미치는 영향은?" 같은 질문에 답할 수 있다.

```
Week 5: CPU 아키텍처 & 실행 모드
─────────────────────────────────
1. x86_64 레지스터 & 실행 모드
   - Ring 0 (커널) vs Ring 3 (유저)
   - SYSCALL/SYSRET 명령: 모드 전환 메커니즘
   - RSP 전환: 유저 스택 → 커널 스택

2. 캐시 계층 (L1/L2/L3)
   - cache line (64B): I/O 자료구조 정렬의 이유
   - cache coherence: MESI 프로토콜 (DMA와의 관계)
   - write combining: MMIO doorbell write 최적화

3. TLB (Translation Lookaside Buffer)
   - 가상→물리 주소 변환 캐시
   - TLB flush: context switch 시 발생
   - hugepage: TLB miss 감소 → SPDK에서 활용

실습:
 □ perf stat으로 cache miss rate 측정
 □ /proc/cpuinfo 분석: cache 크기, CPU 토폴로지
 □ CPUID instruction으로 CPU feature 확인

Week 6: 메모리 관리 기초
───────────────────────
1. 가상 메모리 & 페이지 테이블
   - 4-level page table (PGD → PUD → PMD → PTE)
   - struct page: 물리 페이지 표현
   - 커널 가상 주소 vs 물리 주소 vs DMA 주소

2. DMA (Direct Memory Access)
   - CPU 개입 없이 디바이스↔메모리 직접 전송
   - DMA 주소 = IOMMU가 변환한 디바이스 관점 주소
   - dma_alloc_coherent(): NVMe SQ/CQ 메모리 할당
   - dma_map_sg(): scatter-gather DMA 매핑

3. IOMMU (Intel VT-d / AMD-Vi)
   - 디바이스의 DMA 주소를 물리 주소로 변환
   - 보안: DMA 공격 방지
   - QEMU에서의 IOMMU 에뮬레이션 (vIOMMU)

실습:
 □ /proc/iomem으로 물리 메모리 맵 확인
 □ dmesg에서 IOMMU/DMA 관련 메시지 분석
 □ 가상→물리 주소 변환 과정을 그림으로 그리기

Week 7: PCIe 버스 아키텍처
─────────────────────────
1. PCIe 토폴로지
   - Root Complex → Switch → Endpoint (디바이스)
   - lspci -tv: 트리 구조 확인
   - NVMe SSD = PCIe Endpoint 디바이스

2. PCIe Configuration Space
   - Vendor ID, Device ID, Class Code (0x010802 = NVMe)
   - BAR (Base Address Register): MMIO 영역 주소
   - MSI-X Capability: 인터럽트 벡터 정보
   - lspci -vvv로 상세 정보 확인

3. PCIe TLP (Transaction Layer Packet)
   - Memory Read/Write: MMIO 접근과 DMA
   - Configuration Read/Write: config space 접근
   - MSI-X: Memory Write TLP로 인터럽트 전달

4. MMIO (Memory-Mapped I/O)
   - ioremap(): BAR 물리 주소 → 커널 가상 주소 매핑
   - readl()/writel(): MMIO 레지스터 접근
   - NVMe doorbell = BAR0 + 0x1000 오프셋의 MMIO 레지스터

실습:
 □ lspci -vvv로 NVMe 디바이스 BAR, MSI-X 정보 확인
 □ setpci로 PCI config space 직접 읽기
 □ NVMe spec의 레지스터 맵과 lspci 출력 매핑

Week 8: 인터럽트 & 동기화
─────────────────────────
1. 인터럽트 메커니즘
   - MSI-X: PCIe 디바이스의 현대적 인터럽트 방식
   - LAPIC → IDT → 인터럽트 핸들러
   - Top-half (hardirq) vs Bottom-half (softirq, tasklet, workqueue)
   - /proc/interrupts 분석

2. Polling vs Interrupt
   - Interrupt: latency는 낮지만 overhead 있음
   - Polling: overhead 없지만 CPU 소비
   - Hybrid: interrupt coalescing, busy polling
   → NVMe/SPDK 성능 차이의 핵심 요인

3. 커널 동기화 기법
   - spinlock: 짧은 critical section (인터럽트 컨텍스트 가능)
   - mutex: 긴 critical section (sleep 가능)
   - RCU (Read-Copy-Update): lock-free 읽기
   - atomic 연산: test_and_set_bit() → sbitmap tag 할당
   - per-CPU 변수: lock 없이 CPU별 데이터 관리

4. 동시성과 I/O
   - blk-mq per-CPU software queue: lock contention 감소
   - NVMe per-CPU I/O queue: 큐 간 lock 불필요
   → "왜 멀티큐인가"의 근본적 답

실습:
 □ /proc/interrupts에서 NVMe MSI-X 벡터 확인
 □ spin_lock의 실제 어셈블리 코드 분석
 □ lockdep 활성화 → 데드락 탐지 실습
```

### Chapter 2: 개발 환경 구축과 QEMU 기반 커널 디버깅 (Week 9~11)

**목표**: QEMU + GDB로 커널 코드를 한 줄씩 따라갈 수 있는 환경을 완성한다.

**도달 수준**: "이 함수에서 변수 값이 뭔지 궁금하다" → 5분 내에 GDB breakpoint + print로 확인 가능.

```
Week 9: 커널 빌드 & QEMU 부팅
─────────────────────────────
1. 커널 빌드 설정
   - sources/linux/Makefile → 빌드 시스템 진입점
   - sources/linux/scripts/kconfig/Makefile → menuconfig 동작 원리
   - 필수 CONFIG:
     CONFIG_DEBUG_INFO=y          ← GDB 심볼
     CONFIG_GDB_SCRIPTS=y         ← GDB python scripts
     CONFIG_BLK_DEV_NVME=y        ← NVMe 드라이버
     CONFIG_NVME_CORE=y           ← NVMe core
     CONFIG_DEBUG_FS=y            ← debugfs (계측용)
     CONFIG_BLK_DEV_IO_TRACE=y    ← blktrace
     CONFIG_FTRACE=y              ← 함수 트레이싱
     CONFIG_KPROBES=y             ← 동적 계측
     CONFIG_IO_URING=y            ← io_uring

2. QEMU NVMe 디바이스 옵션 이해
   - qemu-system-x86_64 -drive file=test.qcow2,if=none,id=nvm \
     -device nvme,serial=deadbeef,drive=nvm,id=nvme0
   - QEMU NVMe 디바이스 파라미터 전체 목록 파악
   - 멀티큐 설정: -device nvme,...,max_ioqpairs=4

3. GDB 연결
   - qemu-system-x86_64 -s -S (gdb stub 활성화)
   - gdb vmlinux → target remote :1234
   - break nvme_queue_rq → continue → 실제 I/O 시 중단 확인

Week 10: rootfs 준비 & fio 설치
──────────────────────────────
1. debootstrap/buildroot로 최소 rootfs 생성
2. fio를 rootfs에 포함 (static build)
3. 부팅 → NVMe 디바이스 인식 확인 (lsblk, nvme list)
4. fio --name=test --ioengine=libaio --direct=1 --bs=4k \
   --rw=randwrite --numjobs=1 --size=128M --filename=/dev/nvme0n1
   → 정상 I/O 수행 확인

Week 11: 디버깅 워크플로 확립
────────────────────────────
1. ftrace 설정
   - /sys/kernel/debug/tracing/set_ftrace_filter에 nvme_*, blk_mq_* 등록
   - trace-cmd record -p function_graph -g nvme_queue_rq
   → 함수 호출 트리 시각화

2. printk 기반 디버깅
   - drivers/nvme/host/pci.c의 nvme_queue_rq()에 printk 추가
   - 커널 재빌드 → QEMU 재부팅 → dmesg 확인

3. kprobe/kretprobe 동적 트레이싱
   - echo 'p:my_probe nvme_queue_rq' > /sys/kernel/debug/tracing/kprobe_events
   → argument 값 실시간 확인

4. BPF/bpftrace (고급)
   - bpftrace -e 'kprobe:nvme_queue_rq { @start[tid] = nsecs; }'
   → 리부팅 없이 동적 계측, Phase 6(성능 엔지니어링)의 핵심 도구

실습 과제:
 □ QEMU에서 NVMe 디바이스로 fio 4K random write 수행
 □ GDB로 nvme_queue_rq()에 breakpoint 걸어 한 줄씩 step
 □ ftrace function_graph로 I/O 한 건의 전체 함수 호출 체인 캡처
 □ printk로 submission queue tail 값 변화 로그 출력
 □ kprobe로 nvme_queue_rq의 인자 값 트레이싱
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 1: 커널 I/O 경로 전반부 — Syscall → Block Layer (Month 4~6)
## ═══════════════════════════════════════════════════════════════

### Chapter 3: x86 시스템 콜 진입 & VFS (Week 12~16)

**목표**: 유저 공간의 `write()` 호출이 커널 공간으로 진입하여 VFS를 거쳐 블록 레이어에 도달하기까지의 전체 경로를 라인바이라인으로 이해한다.

**역할**: I/O flow의 가장 첫 단계. "커널 코드는 어디서부터인가"를 정확히 아는 기초.

```
Week 12: syscall 진입 메커니즘
────────────────────────────
1. glibc → kernel 진입
   - glibc의 write() wrapper → SYSCALL instruction
   - arch/x86/entry/entry_64.S: entry_SYSCALL_64
     라인바이라인:
     L79~L95:  swapgs, RSP 전환 (user→kernel stack)
     L96~L110: pt_regs 구조체에 레지스터 저장
     L111:     do_syscall_64() 호출

2. syscall dispatch
   - arch/x86/entry/common.c: do_syscall_64()
     라인바이라인:
     → nr = regs->orig_ax (syscall 번호 추출)
     → sys_call_table[nr](regs) 호출
   - arch/x86/entry/syscalls/syscall_64.tbl
     → 1번: sys_write, 209번: sys_io_submit 등 매핑 확인

3. pt_regs 구조체 분석
   - arch/x86/include/asm/ptrace.h: struct pt_regs
     → rdi=fd, rsi=buf, rdx=count (write의 경우)

Week 13: syscall 복귀 & 세 가지 I/O syscall 비교
────────────────────────────────────────────────
1. 복귀 경로: entry_64.S swapgs, sysretq
   - SYSRET vs IRET 차이 (signal pending 시 IRET)

2. 세 가지 I/O syscall 비교
   - sys_write()        → fs/read_write.c: ksys_write()
   - sys_io_submit()    → fs/aio.c: __x64_sys_io_submit()
   - sys_io_uring_enter → io_uring/io_uring.c

3. SPDK와의 비교 (핵심 차이점 첫 등장)
   → SPDK는 syscall 자체가 없다
   → 유저공간에서 MMIO 직접 접근 (vfio + mmap)
   → syscall overhead가 0인 이유

Week 14: VFS — fd에서 file_operations까지
─────────────────────────────────────────
1. fs/read_write.c: ksys_write()
   라인바이라인:
   → fdget_pos(fd) → current->files->fdt->fd[fd]에서 struct file* 추출
   → vfs_write(f.file, buf, count, ppos) 호출

2. fs/read_write.c: vfs_write()
   → rw_verify_area(): 접근 권한/보안 검사
   → new_sync_write() → file->f_op->write_iter(kio, iter)
   → 핵심: f_op가 무엇이냐에 따라 이후 경로 결정

3. struct file_operations 비교
   - 블록 디바이스: block/fops.c의 def_blk_fops
   - ext4: fs/ext4/file.c의 ext4_file_operations
   - 비교표: 어떤 함수 포인터가 다른지

Week 15: 블록 디바이스 경로 (raw device)
──────────────────────────────────────
1. block/fops.c: blkdev_write_iter()
   → IOCB_DIRECT 분기:
     Direct I/O → blkdev_direct_IO() → __blkdev_direct_IO()
     Buffered   → generic_perform_write()

2. __blkdev_direct_IO()
   → bio_alloc(bdev, ...) → __bio_add_page() → submit_bio(bio)

3. iov_iter 구조체: 사용자 버퍼 추상화
   - ITER_UBUF, ITER_IOVEC, ITER_BVEC 등

Week 16: ext4 파일시스템 경로
───────────────────────────
1. ext4_file_write_iter() → ext4_dio_write_iter() (Direct I/O)
2. iomap_dio_rw() → __iomap_dio_rw()
   → iomap_iter()로 extent 매핑 (논리→물리 블록 변환)
   → iomap_dio_bio_iter()에서 bio 생성 → submit_bio()
3. ext4 block mapping: ext4_ext_map_blocks() → extent tree

실습 과제:
 □ GDB로 ksys_write() → vfs_write() → blkdev_write_iter() step
 □ /dev/nvme0n1에 직접 write할 때와 ext4 마운트 후 파일 write 비교
 □ f_op 포인터가 실제로 어떤 함수를 가리키는지 GDB로 확인
 □ strace로 fio의 syscall 패턴 캡처 (libaio vs sync vs io_uring)
 □ ftrace로 ext4_dio_write_iter → submit_bio 호출 체인 캡처
```

### Chapter 4: bio — 블록 I/O의 기본 단위 (Week 17~19)

**목표**: `struct bio`의 생성부터 소멸까지 전체 생명주기를 이해한다.

**역할**: bio는 블록 레이어의 가장 기본적인 데이터 구조. 이후 모든 계층에서 bio를 다룬다.

```
Week 17: struct bio 구조체 해부
─────────────────────────────
1. include/linux/bio.h + include/linux/blk_types.h
   struct bio 핵심 필드:
   - bi_iter: struct bvec_iter (sector, size, idx, bvec_done)
   - bi_io_vec: struct bio_vec 배열 (page, offset, len)
   - bi_end_io: completion callback
   - bi_opf: operation + flags (REQ_OP_READ/WRITE, REQ_SYNC 등)
   - bi_bdev: target block device

2. struct bio_vec: { struct page *bv_page; unsigned bv_len; unsigned bv_offset; }
   → scatter-gather element: 사용자 버퍼 페이지를 가리킴

3. struct bvec_iter: bi_sector(512B 단위), bi_size, bi_idx
   → bio_advance(): iterator 전진 로직

Week 18: bio 할당과 페이지 매핑
──────────────────────────────
1. bio_alloc_bioset() → mempool_alloc() → bio_init()
2. bio_add_page() → __bio_try_merge_page() → 새 bio_vec 추가
3. bio_iov_iter_get_pages() → get_user_pages_fast() → 페이지 pin
4. bio_split() + bio_chain(): max_sectors 초과 시 분할

Week 19: bio 제출과 완료
────────────────────────
1. submit_bio() → submit_bio_noacct() → blk_mq_submit_bio()
2. bio_endio() → bio->bi_end_io(bio) callback 호출
3. bio chaining: child 완료 → parent bi_remaining 감소 → 모두 완료 시 parent callback

실습 과제:
 □ GDB로 bio_alloc → bio_add_page에서 bio_vec 값 확인
 □ 4KB vs 128KB write 시 bio_vec 개수 차이 확인
 □ bio_split 발생 조건 만들기 (매우 큰 I/O 제출)
 □ bio_endio에 printk 추가하여 완료 시점 확인
```

### Chapter 5: blk-mq — Multi-Queue Block Layer (Week 20~25)

**목표**: blk-mq의 전체 아키텍처 — Software Queue → Hardware Queue 매핑, request 할당/병합, dispatch, 스케줄링을 모든 주요 함수에 대해 라인바이라인으로 추적한다.

**역할**: bio(I/O 의도)를 request(디바이스 명령)로 변환하는 핵심 계층. NVMe 멀티큐 구조의 기반.

```
Week 20: blk-mq 아키텍처와 자료구조
────────────────────────────────────
1. include/linux/blk-mq.h 핵심 구조체:
   - struct blk_mq_hw_ctx (hctx): hardware dispatch queue
   - struct blk_mq_ctx (ctx): per-CPU software staging queue
   - struct blk_mq_ops: 드라이버 콜백 (.queue_rq, .complete, .map_queues)
   - struct blk_mq_tag_set: 드라이버가 등록하는 tag/queue 설정

2. CPU → Software Queue → Hardware Queue 매핑
   - blk_mq_map_queues(): CPU i → hctx[i % nr_hw_queues]
   - NVMe: nvme_pci_map_queues()로 커스텀 매핑

3. SPDK 비교: SPDK에는 블록 레이어가 없다
   → Application이 직접 NVMe 명령을 조립
   → bio/request 변환 overhead = 0
   → 대신 application이 scatter-gather를 직접 관리해야 함

Week 21: request 할당과 bio→request 변환
───────────────────────────────────────
1. blk_mq_submit_bio() 전체 라인바이라인:
   → __blk_mq_alloc_requests(): sbitmap에서 tag 할당 + request 획득
   → blk_mq_bio_to_request(): bio → request 복사
     rq->__sector = bio->bi_iter.bi_sector
     rq->bio = rq->biotail = bio

2. request 병합 (merge)
   - blk_attempt_plug_merge(): plug 리스트에서 인접 request 검색
   - blk_mq_sched_try_merge(): 스케줄러 merge

3. plug/unplug: batching 메커니즘
   - blk_start_plug() → I/O들을 plug 리스트에 모음
   - blk_finish_plug() → blk_flush_plug_list() → 한꺼번에 dispatch

Week 22: dispatch — request를 드라이버에 전달
────────────────────────────────────────────
1. blk_mq_run_hw_queue() → __blk_mq_run_hw_queue()
   → blk_mq_sched_dispatch_requests()

2. blk_mq_dispatch_rq_list() 핵심:
   → list_for_each_entry_safe(rq, ...) {
       ret = q->mq_ops->queue_rq(hctx, &bd);
       // → NVMe의 nvme_queue_rq()
     }

Week 23: tag & sbitmap 심층
───────────────────────────
1. sbitmap: multi-word bitmap, lock-free allocation
   - sbitmap_get() → __sbitmap_get_word() → test_and_set_bit_lock()
2. tag = request 배열 index = NVMe command ID
   - blk_mq_tag_to_rq(): tag → request 역변환
3. tag 부족 시: io_schedule() → 다른 I/O 완료 대기

Week 24: I/O 스케줄러 (none / mq-deadline / kyber)
──────────────────────────────────────────────────
1. none: bio → request → 바로 dispatch (NVMe 기본)
2. mq-deadline: deadline RB-tree, read starvation 방지
3. kyber: token 기반 rate limiting, latency target 자동 조절
4. 스케줄러와 NVMe: NVMe는 보통 none이 최적인 이유

Week 25: Request 완료 경로
─────────────────────────
1. blk_mq_end_request() → blk_update_request() → bio_endio()
2. __blk_mq_free_request(): tag 해제, request 반환
3. softirq vs direct completion: blk_mq_complete_request_remote()
4. 비동기 완료: aio_complete_rw(), io_uring CQE 기록

실습 과제:
 □ GDB로 blk_mq_submit_bio() 전체를 step through
 □ request의 tag 값과 NVMe command ID가 같은지 확인
 □ plug/unplug에 printk → batching 동작 확인
 □ sbitmap tag 할당 과정을 GDB로 관찰
 □ none/mq-deadline 스케줄러 전환 → fio 성능 비교
 □ completion이 어떤 CPU에서 실행되는지 확인
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 2: NVMe Driver 심층 분석 (Month 7~9)
## ═══════════════════════════════════════════════════════════════

### Chapter 6: NVMe PCIe 드라이버 초기화 (Week 26~29)

**목표**: NVMe PCIe 디바이스의 probe부터 queue 생성, namespace 등록까지 전체 초기화 과정을 라인바이라인으로 분석한다.

**역할**: "디바이스가 왜 안 보이는가", "큐가 몇 개 생기는가"에 답하려면 초기화를 알아야 한다.

```
Week 26: PCIe 디바이스 발견과 드라이버 매칭
──────────────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_pci_driver
   - .id_table = nvme_id_table → PCI_CLASS_STORAGE_EXPRESS (0x010802)
   - .probe = nvme_probe

2. PCIe 열거: pci_scan_slot() → pci_device_add() → bus_probe_device()
   → driver_match_device() → PCI class code 비교 → nvme_probe() 호출

3. QEMU 측: nvme_class_init() → PCI class code, vendor/device ID 설정

Week 27: nvme_probe() — 드라이버 초기화
───────────────────────────────────────
1. nvme_probe() 전체 라인바이라인:
   → nvme_dev_alloc() → pci_enable_device_mem() → pci_set_master()
   → pci_request_mem_regions() → nvme_map_bar(): BAR0 ioremap
   → nvme_setup_prp_pools() → nvme_init_ctrl() → nvme_reset_ctrl()

2. BAR0 레지스터 맵 (NVMe spec):
   0x00: CAP, 0x14: CC, 0x1C: CSTS, 0x24: AQA, 0x28: ASQ, 0x30: ACQ
   0x1000+: Doorbell registers

Week 28: 리셋과 Admin Queue 생성
───────────────────────────────
1. nvme_reset_work() 라인바이라인:
   → nvme_pci_enable() → nvme_pci_configure_admin_queue()
   → nvme_alloc_queue() → dma_alloc_coherent(): SQ/CQ DMA 메모리
   → writel(aqa, dev->bar + NVME_REG_AQA)
   → lo_hi_writeq(sq_dma, dev->bar + NVME_REG_ASQ)
   → nvme_enable_ctrl(): CC.EN=1 → nvme_wait_ready(): CSTS.RDY=1

2. Admin 명령: nvme_identify_ctrl(), nvme_set_features()

Week 29: I/O Queue 생성과 Namespace 등록
───────────────────────────────────────
1. nvme_setup_io_queues() → nvme_create_io_queues()
   → min(cpu 수, 디바이스 최대 queue 수) → 큐 수 결정
2. nvme_dev_add() → blk_mq_alloc_tag_set()
   → .ops = nvme_mq_ops, .nr_hw_queues, .queue_depth
3. nvme_scan_work() → nvme_identify_ns() → nvme_alloc_ns() → device_add_disk()
   → /dev/nvme0n1 등장!

실습 과제:
 □ GDB로 nvme_probe() 전체를 step (QEMU 부팅 시)
 □ BAR0 레지스터 값 GDB로 읽기 (x/8x dev->bar)
 □ I/O Queue 수를 1개로 제한 → 성능 변화 측정
 □ dmesg에서 nvme 초기화 로그와 코드 라인 1:1 매핑
```

### Chapter 7: NVMe I/O 제출 경로 — nvme_queue_rq() (Week 30~33)

**목표**: `nvme_queue_rq()`가 NVMe 명령을 조립하고, SQ에 기록하고, Doorbell을 울리는 전체 과정을 instruction 수준으로 이해한다.

**역할**: **전체 커리큘럼의 핵심 중 핵심.** "커널이 하드웨어에 명령을 보내는 정확한 순간."

```
Week 30: nvme_queue_rq() 전체 분석
─────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_queue_rq()
   struct nvme_queue *nvmeq = hctx->driver_data;
   struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
   // 1단계: NVMe 명령 조립
   ret = nvme_setup_cmd(req->q->disk->part0, req);
   // 2단계: 데이터 DMA 매핑
   ret = nvme_map_data(dev, req, cmnd);
   // 3단계: SQ에 명령 기록 & doorbell
   nvme_submit_cmd(nvmeq, cmnd, bd->last);

2. SPDK 비교: spdk_nvme_ns_cmd_write()
   → nvme_qpair_submit_request()
   → nvme_pcie_qpair_submit_request()
   → 커널의 nvme_queue_rq()와 구조 유사하지만
     VFS/블록레이어 경유 없이 직접 도달
     DMA 매핑도 hugepage + VFIO로 사전 매핑 → 제출 시 overhead 0

Week 31: NVMe 명령 조립 — nvme_setup_cmd()
──────────────────────────────────────────
1. nvme_setup_rw() 라인바이라인:
   → cmnd->rw.opcode = nvme_cmd_write / nvme_cmd_read
   → cmnd->rw.nsid = cpu_to_le32(ns->head->ns_id)
   → cmnd->rw.slba = cpu_to_le64(nvme_sect_to_lba(ns, blk_rq_pos(req)))
   → cmnd->rw.length = cpu_to_le16((blk_rq_bytes(req) >> ns->lba_shift) - 1)

2. NVMe Command 구조체 (include/linux/nvme.h):
   64B: opcode(1B), flags(1B), command_id(2B), nsid(4B),
        slba(8B), length(2B), prp1(8B), prp2(8B)...

Week 32: DMA 매핑 — nvme_map_data()
───────────────────────────────────
1. nvme_map_data() 라인바이라인:
   → sg_init_table() → blk_rq_map_sg() → dma_map_sg()
   → nvme_pci_setup_prps() 또는 nvme_pci_setup_sgls()

2. PRP (Physical Region Page) 리스트:
   → 4KB 이하: PRP1만
   → 4KB~8KB: PRP1 + PRP2
   → 8KB 초과: PRP2 = PRP List 주소 → 물리 주소 배열

Week 33: SQ 기록과 Doorbell — nvme_submit_cmd()
───────────────────────────────────────────────
1. memcpy(&nvmeq->sq_cmds[tail], cmd, sizeof(*cmd))
2. nvme_write_sq_db() → writel(tail, nvmeq->q_db) ← PCIe MMIO Write
3. Shadow Doorbell (DBBUF): MMIO 횟수 감소 최적화

   CPU                                NVMe Controller (QEMU)
    │  memcpy(SQ[tail], cmd)                  │
    │  writel(tail, doorbell_reg)             │
    │ ──── PCIe MMIO Write ─────────────────→│
    │                      DMA Read(SQ)       │
    │←──── PCIe DMA Read ────────────────────│
    │                      명령 처리/DMA Write │
    │←──── PCIe DMA Write ───────────────────│
    │←──── MSI-X Interrupt ──────────────────│

실습 과제:
 □ GDB로 nvme_queue_rq() 전체 step — 모든 변수 값 확인
 □ PRP 리스트의 DMA 주소와 사용자 버퍼 물리 주소 비교
 □ SQ 메모리 실제 내용 GDB로 덤프 (x/16x nvmeq->sq_cmds)
 □ Shadow Doorbell 활성화/비활성화 시 writel 빈도 비교
```

### Chapter 8: NVMe Completion 경로 (Week 34~36)

**목표**: MSI-X 인터럽트 → CQ 처리 → request 완료 → application 통지까지의 역방향 경로를 추적한다.

```
Week 34: 인터럽트 핸들러와 CQ 처리
─────────────────────────────────
1. nvme_irq() → nvme_process_cq()
   → while (nvme_cqe_pending(nvmeq)):
     cqe = &nvmeq->cqes[head]
     → cqe->command_id = tag, cqe->status = 완료 상태
     → nvme_handle_cqe() → blk_mq_tag_to_rq(tag) → nvme_pci_complete_rq()
   → nvme_ring_cq_doorbell(): writel(head, cq_db)

2. nvme_pci_complete_rq()
   → nvme_unmap_data() → nvme_complete_rq() → blk_mq_end_request()

Week 35: MSI-X 인터럽트 상세
───────────────────────────
1. nvme_setup_irqs() → pci_alloc_irq_vectors_affinity()
2. x86: MSI-X write → LAPIC → do_IRQ() → nvme_irq()
3. Interrupt Coalescing: Feature 08h, aggregation time + threshold

Week 36: Polling Mode (io_poll)
──────────────────────────────
1. nvme_poll(): 인터럽트 없이 CQ 직접 polling
2. blk_mq_poll() → q->mq_ops->poll() = nvme_poll()
3. io_uring IORING_SETUP_IOPOLL → io_do_iopoll() → blk_mq_poll()
4. Interrupt ~10μs vs Polling ~2μs (실제 하드웨어 기준)

5. SPDK 비교: SPDK는 항상 polling
   → spdk_nvme_qpair_process_completions()
   → interrupt overhead 완전 제거
   → 대신 CPU 100% 사용 (dedicated core 필요)

실습 과제:
 □ GDB로 nvme_irq() → nvme_process_cq() 전체 step
 □ CQE command_id와 원본 request tag 일치 확인
 □ polling mode vs interrupt mode fio 성능 비교 (--hipri)
 □ interrupt coalescing 설정 변경 → latency 변화 측정
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 3: QEMU NVMe & 고급 I/O 경로 (Month 10~12)
## ═══════════════════════════════════════════════════════════════

### Chapter 9: QEMU NVMe 컨트롤러 구조 (Week 37~40)

**목표**: QEMU가 NVMe 디바이스를 어떻게 에뮬레이션하는지 — 커널 드라이버의 "반대편"을 이해한다.

```
Week 37: QEMU 디바이스 모델 기초
───────────────────────────────
1. QOM (QEMU Object Model): nvme_class_init() → dc->realize = nvme_realize
2. NVMe 프로퍼티: serial, cmb_size_mb, mqes, mdts
3. PCI config space 설정: class code, vendor/device ID

Week 38: BAR 등록과 MMIO 핸들러
──────────────────────────────
1. nvme_realize(): memory_region_init_io(&n->iomem, ..., &nvme_mmio_ops)
2. nvme_mmio_read(): offset 기반 레지스터 반환 (CAP, VS, CSTS...)
3. nvme_mmio_write(): CC, AQA, ASQ, ACQ 설정 + doorbell → nvme_process_db()

Week 39: Admin Queue 생성 & I/O Queue 생성 (QEMU 측)
────────────────────────────────────────────────────
1. CC.EN=1 처리: nvme_start_ctrl() → SQ/CQ 초기화 → CSTS.RDY=1
2. Create I/O SQ/CQ Admin 명령 처리

Week 40: QEMU NVMe 자료구조
──────────────────────────
1. struct NvmeCtrl: .bar, .sq[], .cq[], .namespaces
2. struct NvmeSQueue: .dma_addr, .size, .head, .tail
3. struct NvmeCQueue: .dma_addr, .head, .tail, .vector

실습 과제:
 □ QEMU 소스에 printf → MMIO read/write 로그 출력
 □ 커널 writel(doorbell)과 QEMU nvme_mmio_write() 대응 확인
 □ QEMU NVMe 프로퍼티(mqes, mdts) 변경 → 커널 동작 변화 관찰
```

### Chapter 10: QEMU NVMe I/O 처리 (Week 41~44)

**목표**: Doorbell → SQ 명령 읽기 → 데이터 전송 → CQ 완료 → MSI-X의 전체 과정.

```
Week 41: Doorbell 처리와 SQ 명령 읽기
────────────────────────────────────
1. nvme_process_db(): SQ tail 값 업데이트 → 타이머 스케줄
2. nvme_process_sq():
   → while (sq->head != sq->tail):
     nvme_addr_read(SQ[head]) → 64B NVMe 명령 읽기
     nvme_io_cmd() → 명령별 처리

Week 42: NVMe 명령 처리 (Read/Write)
───────────────────────────────────
1. nvme_io_cmd(): switch(opcode) → nvme_rw(), nvme_flush(), nvme_dsm()
2. nvme_rw(): slba, nlb 파싱 → nvme_map_prp() → dma_blk_read/write()
3. nvme_map_prp(): PRP1/PRP2/PRP List → QEMUSGList 구성

Week 43: QEMU 블록 백엔드
────────────────────────
1. dma_blk_write() → blk_aio_pwritev() → QEMU block layer
2. BlockDriverState chain: throttle → qcow2/raw → file-posix
3. file-posix.c: raw_co_pwritev() → 호스트 OS의 pwritev()

Week 44: CQ 기록과 MSI-X 인터럽트
─────────────────────────────────
1. nvme_enqueue_req_completion() → nvme_post_cqes()
   → CQE 16B 기록: result, sq_head, cid, status + phase bit
   → nvme_addr_write(): 호스트 메모리 CQ에 DMA Write
2. msix_notify() → 커널 nvme_irq() 호출 (Chapter 8로 연결)

   커널 writel(doorbell)
     → nvme_process_db() → nvme_process_sq() → nvme_io_cmd()
     → nvme_rw() → nvme_map_prp() → dma_blk_read/write()
     → nvme_rw_cb() → nvme_enqueue_req_completion()
     → nvme_post_cqes() → msix_notify() → 커널 nvme_irq()

실습 과제:
 □ QEMU nvme_process_sq()에 printf → 매 명령의 opcode, slba 출력
 □ QEMU에 에러 주입 (status=NVME_SC_INTERNAL) → 커널 반응 확인
 □ QEMU에 latency 지연 추가 → fio latency 변화 측정
```

### Chapter 11: io_uring — 고성능 비동기 I/O (Week 45~47)

**목표**: io_uring의 SQ/CQ 공유 메모리, submission/completion, polling을 분석한다.

```
Week 45: io_uring 초기화와 공유 메모리
────────────────────────────────────
1. io_uring_setup() → io_ring_ctx 할당, SQ/CQ 링 버퍼 할당
2. 공유 메모리: mmap으로 유저↔커널 공유
   SQ Ring: head, tail, array[]
   CQ Ring: head, tail, cqes[]
   SQE 배열: 유저가 직접 쓰기

Week 46: 제출 경로
──────────────────
1. io_uring_enter() → io_submit_sqes() → io_get_sqe()
   → IORING_OP_READ/WRITE: io_read/io_write
   → vfs_iocb_iter_read/write() → 기존 VFS 경로와 합류

Week 47: 완료와 polling
──────────────────────
1. io_complete_rw() → CQE를 CQ ring에 기록 → eventfd 통지
2. IOPOLL: io_do_iopoll() → blk_mq_poll() → nvme_poll()
3. SQPOLL: io_sq_thread() → syscall 없이 SQ 자동 polling

실습 과제:
 □ 간단한 io_uring C 프로그램 작성: 4KB direct write → NVMe
 □ IOPOLL vs 인터럽트 모드 latency 비교
 □ SQPOLL 모드에서 io_sq_thread CPU 사용률 확인
```

### Chapter 12: Buffered I/O와 Page Cache Writeback (Week 48~50)

**목표**: page cache 쓰기, writeback daemon, fsync의 전체 경로.

```
Week 48: Page Cache 쓰기
───────────────────────
1. generic_perform_write() → pagecache_get_page()
   → a_ops->write_begin() → copy_page_from_iter() → a_ops->write_end()
   → set_page_dirty()
2. Dirty 페이지 관리: PG_dirty, balance_dirty_pages()

Week 49: Writeback 메커니즘
─────────────────────────
1. wb_workfn() → wb_writeback() → writeback_sb_inodes()
   → do_writepages() → ext4_writepages() → submit_bio()
2. 트리거: dirty_writeback_centisecs(5초), dirty_ratio, sync/fsync

Week 50: fsync와 데이터 안전성
─────────────────────────────
1. vfs_fsync() → ext4_sync_file()
   → filemap_write_and_wait_range() → jbd2_complete_transaction()
   → blkdev_issue_flush() → NVMe Flush 명령
2. NVMe flush: nvme_setup_flush() → QEMU nvme_flush() → blk_aio_flush()

실습 과제:
 □ buffered write 후 /proc/meminfo Dirty 값 변화 관찰
 □ fsync() 시 NVMe flush 명령까지의 경로 ftrace
 □ dirty_expire_centisecs 조정 → writeback 타이밍 변화 확인
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 4: 커널 소스 수정 실습 (Month 13~15)
## ═══════════════════════════════════════════════════════════════

> **여기서부터 "읽는 사람"에서 "고치는 사람"으로 전환된다.**

### Chapter 13: 첫 번째 수정 — I/O 통계 수집 모듈 (Week 51~54)

**목표**: NVMe 드라이버에 I/O별 latency/LBA/큐 깊이 통계를 수집하여 debugfs로 노출.

```
Week 51: debugfs 인터페이스 설계
──────────────────────────────
1. debugfs_create_file(), DEFINE_SHOW_ATTRIBUTE()
2. 데이터 구조: per-queue ring buffer (최근 N개 I/O 기록)

Week 52: 제출 경로 계측
─────────────────────
1. nvme_queue_rq() 수정: ktime_get_ns()로 타임스탬프
2. nvme_iod에 submit_time 필드 추가

Week 53: 완료 경로 계측
─────────────────────
1. nvme_pci_complete_rq() 수정: latency = complete - submit
2. debugfs 노출: /sys/kernel/debug/nvme_io_stats/

Week 54: 테스트와 검증
────────────────────
1. QEMU 부팅 → fio 부하 → debugfs 통계 확인
2. regression 테스트, latency 분포 그래프 (gnuplot)

실습 과제:
 □ 수정 → 빌드 → QEMU 부팅 사이클 완성
 □ fio random read/write 시 latency 분포 확인
 □ 큐 깊이별 성능 변화 관찰
```

### Chapter 14: 두 번째 수정 — 커스텀 I/O 스케줄러 (Week 55~58)

**목표**: blk-mq에 커스텀 스케줄러를 추가하여 특정 정책(LBA 우선순위, Read 우선)을 구현.

```
Week 55: 스케줄러 프레임워크
───────────────────────────
1. struct elevator_mq_ops: .insert_requests, .dispatch_request, .has_work
2. 최소 스케줄러 skeleton 작성: block/my-iosched.c

Week 56~57: 정책 구현
────────────────────
1. LBA 범위 기반 우선순위 또는 Read 우선 + aging
2. Kconfig, Makefile 수정

Week 58: 성능 측정
────────────────
1. my-sched vs none vs mq-deadline: fio mixed workload 비교
2. blktrace로 I/O 순서 변화 확인

실습 과제:
 □ echo "my-sched" > /sys/block/nvme0n1/queue/scheduler
 □ fio 벤치마크 비교, blktrace I/O 순서 분석
```

### Chapter 15: 세 번째 수정 — QEMU NVMe 확장 (Week 59~62)

**목표**: QEMU NVMe에 Vendor Specific 명령, latency 시뮬레이션, 에러 주입 기능 추가.

```
Week 59: Vendor Specific Admin 명령
───────────────────────────────────
1. hw/nvme/ctrl.c: opcode 0xC0에 커스텀 핸들러
2. 내부 통계 리턴 (처리 명령 수, 평균 latency)

Week 60: I/O Latency 시뮬레이션 & 에러 주입
──────────────────────────────────────────
1. nvme_rw()에 configurable delay (timer_mod)
2. 특정 LBA에서 에러 반환: NVME_SC_UNRECOVERED_READ_ERROR

Week 61~62: 커널↔QEMU 연동
──────────────────────────
1. 커널에서 Vendor Specific 명령 전송 (ioctl/nvme-cli passthrough)
2. 에러 주입 시 커널 retry/에러 리포팅 경로 분석
3. 커스텀 커널 + 커스텀 QEMU + fio 통합 테스트

실습 과제:
 □ QEMU Vendor Specific 명령 → 커널에서 응답 확인
 □ latency 시뮬레이션 read=100μs, write=50μs → fio 비교
 □ 에러 주입 → 커널 에러 핸들링 경로 추적
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 5: SPDK/DPDK — 유저공간 I/O (Month 16~18)
## ═══════════════════════════════════════════════════════════════

> **커널 I/O와 SPDK를 코드 레벨에서 비교할 수 있어야 시니어다.**

### Chapter 16: SPDK 아키텍처 & NVMe 드라이버 (Week 63~70)

**목표**: SPDK의 NVMe 드라이버가 커널을 bypass하여 직접 디바이스에 접근하는 전체 구조를 이해하고, 커널 경로와 1:1 비교한다.

```
Week 63~64: SPDK 빌드 & 환경 구축
─────────────────────────────────
1. sources/spdk 빌드, 기본 예제 실행
2. DPDK 기반 hugepage 설정, PCI unbind, vfio-pci
3. SPDK hello_world 예제 실행 → NVMe 접근 확인

Week 65~66: SPDK NVMe 드라이버 구조
───────────────────────────────────
1. 디바이스 초기화
   - spdk_nvme_probe() → spdk_nvme_ctrlr_process_init()
   → BAR 매핑: spdk_pci_device_map_bar() → mmap으로 유저공간 매핑
   → Admin Queue 생성: 커널과 동일한 NVMe spec 절차
   → 핵심 차이: 커널 ioremap vs SPDK mmap (유저공간 직접 접근)

2. I/O Queue 생성
   → spdk_nvme_ctrlr_alloc_io_qpair()
   → hugepage에서 SQ/CQ 메모리 할당
   → IOMMU/VFIO로 DMA 주소 = 가상 주소 (1:1 매핑)

Week 67~68: SPDK I/O 제출 경로 (커널 비교)
──────────────────────────────────────────
커널 경로:
  write() → syscall → VFS → Block → NVMe driver → writel(doorbell)
  함수 호출 ~20개, context switch 있음, interrupt 대기

SPDK 경로:
  spdk_nvme_ns_cmd_write()
  → nvme_qpair_submit_request()
  → nvme_pcie_qpair_submit_request()
  → memcpy(sq[tail], cmd)
  → writel(tail, doorbell)       ← 유저공간에서 직접 MMIO write
  함수 호출 ~5개, context switch 0, interrupt 0

1. spdk_nvme_ns_cmd_write() 라인바이라인:
   → NVMe 명령 조립 (커널 nvme_setup_rw()와 동일 구조)
   → scatter-gather list 구성 (커널 blk_rq_map_sg()와 유사)
   → PRP 리스트 생성 (커널 nvme_pci_setup_prps()와 동일)

2. 핵심 차이점 요약:
   ┌────────────────────┬──────────────────┬──────────────────┐
   │ 항목               │ 커널 NVMe        │ SPDK             │
   ├────────────────────┼──────────────────┼──────────────────┤
   │ syscall            │ 매 I/O마다       │ 없음 (0)         │
   │ context switch     │ user↔kernel      │ 없음 (0)         │
   │ 메모리 복사        │ bio→request 변환 │ 직접 SQ 기록     │
   │ interrupt          │ MSI-X per I/O    │ 없음 (polling)   │
   │ DMA 매핑           │ 매 I/O마다       │ 사전 매핑 (0)    │
   │ CPU 사용           │ 필요 시 사용     │ dedicated core   │
   │ 멀티테넌시         │ 커널이 보호      │ app이 직접 관리  │
   │ 파일시스템         │ ext4/xfs 가능    │ 자체 blobstore   │
   └────────────────────┴──────────────────┴──────────────────┘

Week 69~70: SPDK Completion & 이벤트 루프
────────────────────────────────────────
1. spdk_nvme_qpair_process_completions()
   → CQ를 직접 polling: cqe의 phase bit 확인
   → 커널 nvme_process_cq()와 동일 로직, interrupt 없이 수행

2. SPDK 리액터 모델
   → spdk_reactors_start() → 각 코어에서 무한 루프
   → spdk_reactor_run() → 이벤트 처리 + I/O completion polling
   → 커널의 softirq/workqueue 대신 cooperative multitasking

3. SPDK bdev 추상화 계층
   → bdev = 커널의 블록 디바이스 + I/O 스케줄러 역할
   → bdev_nvme, bdev_malloc, bdev_aio 등 모듈

실습 과제:
 □ SPDK perf 도구로 NVMe 성능 측정 → fio(커널) 결과와 비교
 □ SPDK NVMe 초기화 과정을 GDB로 추적
 □ spdk_nvme_ns_cmd_write()와 nvme_queue_rq() 코드 나란히 비교
 □ SPDK hello_world에 latency 측정 코드 추가
 □ SPDK에서 I/O 에러 발생 시 처리 경로 (커널과 비교)
```

### Chapter 17: DPDK 기초 & SPDK와의 관계 (Week 71~74)

**목표**: DPDK의 메모리/PCIe 관리가 SPDK의 기반이 되는 구조를 이해한다.

```
Week 71~72: DPDK EAL (Environment Abstraction Layer)
───────────────────────────────────────────────────
1. rte_eal_init(): hugepage 설정, PCI 스캔, 메모리 풀 초기화
2. hugepage 메모리 관리: rte_mempool, rte_malloc
   → 왜 hugepage인가: TLB miss 감소, DMA 주소 = 가상 주소
3. PCI UIO/VFIO: rte_pci_probe()
   → 커널 드라이버 unbind → VFIO로 유저공간 접근
   → BAR mmap: 유저공간에서 MMIO 직접 가능

Week 73~74: DPDK → SPDK 연결
────────────────────────────
1. SPDK의 DPDK 의존성: 메모리, PCI, 스레드 관리
2. spdk_env_dpdk_post_init(): DPDK 위에 SPDK 환경 구성
3. 커널 vs DPDK vs SPDK 메모리 경로:
   커널: kmalloc → slab → buddy → 물리 페이지
   DPDK: rte_malloc → hugepage pool → 물리 연속 메모리
   → DMA 매핑이 trivial해지는 이유

실습 과제:
 □ DPDK testpmd 실행 (네트워크 예제)
 □ hugepage 할당 확인: /proc/meminfo, /sys/kernel/mm/hugepages/
 □ VFIO PCI unbind/bind 과정 수동 실행
 □ DPDK 메모리 풀과 커널 slab allocator 비교 분석
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 6: 성능 엔지니어링 (Month 19~20)
## ═══════════════════════════════════════════════════════════════

> **시니어는 "느리다"를 "어디서 얼마나 느린지" 수치로 말한다.**

### Chapter 18: 체계적 벤치마킹 방법론 (Week 75~78)

**목표**: fio를 사용한 체계적 성능 측정, perf/BPF 기반 프로파일링, latency 분석 방법론을 습득한다.

```
Week 75: fio 마스터 클래스
─────────────────────────
1. fio 파라미터 완전 이해
   - ioengine: sync, libaio, io_uring, psync, pvsync2
   - direct=1: O_DIRECT (page cache bypass)
   - iodepth: outstanding I/O 수 (큐 깊이)
   - numjobs: 병렬 워커 수
   - bs: 블록 사이즈 (4K, 64K, 1M)
   - rw: read, write, randread, randwrite, randrw

2. 결과 해석
   - IOPS, bandwidth, latency (avg/p50/p99/p99.9)
   - slat (submission latency): 커널 경로 overhead
   - clat (completion latency): 디바이스 처리 시간
   - lat (total latency): slat + clat

3. 실험 설계
   - 변수 하나만 변경: bs=4K→64K→1M (나머지 고정)
   - warm-up: 초기 결과 버리기
   - 반복 측정: 최소 3회, 표준편차 확인

Week 76: perf & flamegraph
─────────────────────────
1. perf record -g -a -- fio ... → perf report
   → I/O 경로에서 CPU 시간이 어디에 소비되는지
2. FlameGraph: perf script | stackcollapse-perf.pl | flamegraph.pl
   → 시각적으로 병목 지점 식별
3. perf stat: IPC, cache miss, branch miss 측정

Week 77: BPF/bpftrace 실전
─────────────────────────
1. biolatency: 블록 레이어 latency 히스토그램
2. biosnoop: 실시간 I/O 모니터링
3. 커스텀 bpftrace 스크립트:
   - NVMe doorbell 빈도 측정
   - per-queue latency 분포
   - I/O size 분포

Week 78: 성능 분석 케이스 스터디
──────────────────────────────
1. 사례 1: iodepth 1→32→128 변화 시 IOPS/latency 트레이드오프
2. 사례 2: Direct I/O vs Buffered I/O 성능 차이 원인 분석
3. 사례 3: 커널 NVMe vs SPDK 성능 차이 정량 분석
   → syscall overhead, interrupt overhead, DMA 매핑 overhead 분리
4. 사례 4: io_uring polling vs interrupt 모드 latency 분석

실습 과제:
 □ fio sweep: bs=4K, iodepth={1,4,16,64,128} → IOPS/latency 그래프
 □ perf flamegraph로 I/O 경로 CPU 사용 분석
 □ bpftrace로 NVMe submission-to-completion latency 히스토그램
 □ 커널 vs SPDK 동일 워크로드 비교 보고서 작성
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 7: GPU-Storage I/O (Month 21~22)
## ═══════════════════════════════════════════════════════════════

> **GPU-NVMe 직접 I/O는 차세대 스토리지의 핵심 주제다.**

### Chapter 19: GPUDirect Storage & NVIDIA GDS (Week 79~82)

**목표**: GPU 메모리와 NVMe SSD 간 직접 데이터 전송 메커니즘을 이해한다.

```
Week 79: GPU 메모리 & PCIe 기초
──────────────────────────────
1. GPU VRAM: PCIe BAR로 접근 가능한 디바이스 메모리
2. CUDA 메모리 관리: cudaMalloc, cudaMemcpy
3. CPU 경유 경로의 문제:
   SSD → CPU 메모리 → GPU 메모리 (double copy)
   → PCIe 대역폭 낭비, CPU 개입, latency 증가

Week 80: GPUDirect Storage 아키텍처
──────────────────────────────────
1. GDS 핵심 아이디어:
   SSD → [PCIe] → GPU 메모리 (direct DMA, CPU bypass)
   → cuFile API: cuFileRead(), cuFileWrite()

2. 구현 메커니즘:
   → nvidia-fs 커널 모듈이 NVMe DMA 주소를 GPU BAR 주소로 설정
   → NVMe 컨트롤러가 DMA할 때 데이터가 GPU 메모리에 직접 도달
   → CPU 메모리를 경유하지 않음 (bounce buffer 없음)

3. NVMe 드라이버와의 상호작용:
   → PRP/SGL에 GPU 물리 주소가 들어감
   → IOMMU peer-to-peer (P2P) DMA 필요

Week 81: GDS 성능 특성
─────────────────────
1. CPU 경유 vs GDS 직접: 대역폭 2~3배 향상 (large sequential I/O)
2. latency: CPU bounce buffer 제거 → latency 감소
3. CPU 부하: GPU→SSD 전송 시 CPU 거의 유휴
4. 한계: small I/O에서는 이점 적음, NVMe 컨트롤러 지원 필요

Week 82: cuFile API & 실습
─────────────────────────
1. cuFile API: cuFileDriverOpen(), cuFileBufRegister(), cuFileRead/Write()
2. GDS와 기존 I/O 경로 비교:
   기존: cudaMalloc → read(fd, cpu_buf) → cudaMemcpy(gpu_buf, cpu_buf)
   GDS:  cudaMalloc → cuFileBufRegister(gpu_buf) → cuFileRead(fd, gpu_buf)

실습 과제:
 □ GPU 메모리 ↔ NVMe 데이터 경로를 ASCII 다이어그램으로 그리기
 □ papers/gpu-ssd/ 디렉토리의 GDS 관련 논문 3편 읽기
 □ P2P DMA 경로에서 IOMMU 역할 분석
```

### Chapter 20: BaM — GPU-Initiated I/O (Week 83~86)

**목표**: sources/bam의 BaM(Big Accelerator Memory) 코드를 분석하여 GPU가 직접 NVMe I/O를 발행하는 메커니즘을 이해한다.

```
Week 83~84: BaM 아키텍처
───────────────────────
1. BaM vs GDS 핵심 차이:
   GDS: CPU가 I/O를 발행, 데이터만 GPU로 직접 전달
   BaM: GPU가 직접 I/O를 발행 (GPU-initiated I/O)

2. BaM 구현:
   → GPU 커널에서 NVMe doorbell에 직접 MMIO write
   → GPU 스레드가 NVMe SQ에 명령 기록
   → CMB (Controller Memory Buffer) 활용: SQ를 NVMe CMB에 배치

3. 소스 분석: sources/bam/
   → GPU 커널 코드: NVMe 명령 조립 + doorbell write
   → 호스트 드라이버: NVMe 초기화 + GPU에 BAR 주소 전달

Week 85~86: GPU-Storage 미래 방향
────────────────────────────────
1. Computational Storage: SSD 내부에서 연산
2. CXL (Compute Express Link): CPU-GPU-SSD 메모리 통합
3. NVMe CMB/PMR: 디바이스 메모리를 호스트가 직접 접근
4. SmartNIC + GPU + NVMe: 삼자 직접 통신

실습 과제:
 □ sources/bam/ 코드 구조 분석 (analysis/bam/ 참조)
 □ BaM의 GPU 커널에서 NVMe 명령 조립 코드 라인바이라인 분석
 □ GDS vs BaM 비교표 작성 (I/O 발행자, 데이터 경로, overhead)
 □ papers/gpu-ssd/I-gpu-initiated/ 논문 3편 읽기
```

---

## ═══════════════════════════════════════════════════════════════
## Phase 8: 시니어 레벨 실전 (Month 23~24)
## ═══════════════════════════════════════════════════════════════

> **시니어 = 코드를 읽고 고치는 것 + 설계하고 리뷰하고 가르치는 것.**

### Chapter 21: NVMe 고급 주제 — NVMe-oF, ZNS, CMB (Week 87~90)

**목표**: 현대 NVMe 생태계의 고급 기능을 이해한다.

```
Week 87: NVMe over Fabrics (NVMe-oF)
────────────────────────────────────
1. NVMe-oF 아키텍처: RDMA/TCP로 원격 NVMe 접근
   - drivers/nvme/host/rdma.c, tcp.c
   - drivers/nvme/target/ (서버 측)
2. 커널 NVMe 드라이버와의 관계:
   - nvme_queue_rq()는 동일, transport만 다름
   - PCIe: writel(doorbell) → MMIO
   - RDMA: 네트워크 전송
   - TCP: socket 전송
3. SPDK NVMe-oF target: 고성능 서버 구현

Week 88: Zoned Namespaces (ZNS)
──────────────────────────────
1. ZNS 개념: sequential write only zone
   → SSD 내부 FTL 부담 감소 → write amplification 감소
2. 커널 지원: drivers/nvme/host/zns.c
3. zonefs, btrfs zoned mode
4. fio --zonemode=zbd

Week 89: CMB (Controller Memory Buffer) & PMR
─────────────────────────────────────────────
1. CMB: NVMe 컨트롤러의 메모리를 호스트가 BAR로 접근
   → SQ를 CMB에 배치 → DMA Read 불필요 → latency 감소
2. PMR: Persistent Memory Region
3. BaM에서 CMB 활용: GPU가 CMB에 직접 SQ 기록

Week 90: NVMe 2.0+ 최신 스펙
────────────────────────────
1. I/O Determinism: NVM Set, Endurance Group
2. Multipath: ana (Asymmetric Namespace Access)
3. Key-Value Commands, Copy Command
4. NVMe spec 문서 읽는 법
```

### Chapter 22: 업스트림 기여 & 아키텍처 설계 (Week 91~96)

**목표**: 실제 Linux 커널 또는 SPDK에 패치를 제출하고, 시스템 아키텍처를 설계하는 능력을 갖춘다.

```
Week 91~92: 커널 패치 제출 프로세스
──────────────────────────────────
1. Linux 커널 코딩 스타일: Documentation/process/coding-style.rst
2. git format-patch → 패치 생성
3. scripts/checkpatch.pl → 스타일 검사
4. get_maintainer.pl → 메인테이너 파악
5. git send-email → 메일링 리스트 제출
6. 코드 리뷰 응대: 피드백 반영 → v2 패치

실제 기여 후보:
 □ NVMe 드라이버 경고/에러 메시지 개선
 □ 중복 코드 리팩토링
 □ 문서/코멘트 개선
 □ 작은 버그 수정 (bugzilla.kernel.org)

Week 93~94: 코드 리뷰 & 아키텍처 설계
─────────────────────────────────────
1. 다른 사람의 패치 리뷰 연습
   - LKML (linux-nvme 메일링 리스트) 구독
   - 최근 NVMe 패치 읽고 리뷰 작성

2. 아키텍처 설계 연습
   - "NVMe SSD에 QoS를 추가하라" → 어디를 수정할지 설계
   - "GPU-NVMe 직접 I/O를 커널에 추가하라" → 설계 문서 작성
   - trade-off 분석: 성능 vs 복잡도 vs 호환성

Week 95~96: 종합 프로젝트 & 포트폴리오
─────────────────────────────────────
1. End-to-End 분석 문서 작성
   → write(fd, buf, 4096) 한 번의 전체 경로를
     함수명, 라인번호, 변수 값까지 포함하여 기술

2. 포트폴리오 정리
   → I/O 통계 모듈 (Chapter 13)
   → 커스텀 스케줄러 (Chapter 14)
   → QEMU NVMe 확장 (Chapter 15)
   → 커널 패치 제출 이력
   → 성능 분석 보고서

3. 시니어 역량 자가 점검:
   □ "이 코드가 왜 이렇게 만들어졌는가" 설명 가능
   □ "이 부분을 바꾸면 어디에 영향이 가는가" 판단 가능
   □ "이 설계의 trade-off는 무엇인가" 분석 가능
   □ "더 나은 방법은 없는가" 대안 제시 가능
   □ "후배에게 이 코드를 가르칠 수 있는가" 교육 가능
```

---

## 월별 요약표

| 월 | Phase | Chapter | 핵심 내용 | Level |
|:---:|:---:|:---:|:---|:---:|
| 1 | 0 | Ch0 | C 포인터/매크로/함수포인터, 커널 자료구조 | 0 |
| 2 | 0 | Ch1 | CPU 아키텍처, 메모리, PCIe, 인터럽트, 동기화 | 0 |
| 3 | 0 | Ch2 | QEMU+GDB 환경 구축, ftrace, kprobe | 0→1 |
| 4 | 1 | Ch3 전반 | syscall 진입, VFS, fd→file→f_op | 1 |
| 5 | 1 | Ch3 후반~Ch4 | ext4/blkdev 경로, bio 구조체 | 1 |
| 6 | 1 | Ch5 | blk-mq 전체: request, tag, dispatch, 스케줄러 | 1→2 |
| 7 | 2 | Ch6 | NVMe probe, 초기화, queue 생성 | 2 |
| 8 | 2 | Ch7 | nvme_queue_rq(), DMA, doorbell | 2 |
| 9 | 2 | Ch8 | NVMe CQ 처리, interrupt, polling | 2 |
| 10 | 3 | Ch9~10 전반 | QEMU NVMe 컨트롤러, MMIO, I/O 처리 | 2 |
| 11 | 3 | Ch10 후반~Ch11 | QEMU 완료 경로, io_uring | 2 |
| 12 | 3 | Ch12 | Page cache writeback, fsync | 2→3 |
| 13 | 4 | Ch13 | 커널 수정: I/O 통계 모듈 | 3 |
| 14 | 4 | Ch14 | 커널 수정: 커스텀 스케줄러 | 3 |
| 15 | 4 | Ch15 | QEMU NVMe 확장 (Vendor Specific, 에러 주입) | 3 |
| 16 | 5 | Ch16 전반 | SPDK 빌드, NVMe 드라이버 구조 | 3 |
| 17 | 5 | Ch16 후반 | SPDK I/O 제출/완료, 커널 비교 | 3→4 |
| 18 | 5 | Ch17 | DPDK EAL, hugepage, VFIO | 4 |
| 19 | 6 | Ch18 전반 | fio 마스터, perf flamegraph | 4 |
| 20 | 6 | Ch18 후반 | BPF/bpftrace, 성능 케이스 스터디 | 4 |
| 21 | 7 | Ch19 | GPUDirect Storage, cuFile API | 4 |
| 22 | 7 | Ch20 | BaM GPU-initiated I/O, 미래 방향 | 4→5 |
| 23 | 8 | Ch21 | NVMe-oF, ZNS, CMB, 최신 스펙 | 5 |
| 24 | 8 | Ch22 | 업스트림 기여, 아키텍처 설계, 포트폴리오 | 5 |

---

## 주당 학습 권장 시간

| 활동 | Phase 0~3 | Phase 4~5 | Phase 6~8 |
|:---|:---:|:---:|:---:|
| 이론/개념 학습 | 4~5시간 | 2~3시간 | 2시간 |
| 소스코드 리딩 (라인바이라인) | 6~8시간 | 4~6시간 | 3~4시간 |
| GDB/ftrace/perf 실습 | 3~4시간 | 3~4시간 | 4~5시간 |
| 코드 수정 & 실험 | - | 4~6시간 | 4~6시간 |
| 노트 정리 & 문서화 | 2~3시간 | 2~3시간 | 3~4시간 |
| **합계** | **~18시간/주** | **~18시간/주** | **~18시간/주** |

---

## 각 Phase의 전제 조건과 연결 관계

```
Phase 0 (C/OS 기초)
  │  기초 없으면 커널 코드 읽기 불가
  ▼
Phase 1 (Syscall → Block Layer)
  │  bio/request 모르면 NVMe 이해 불가
  ▼
Phase 2 (NVMe Driver)
  │  커널 드라이버 모르면 QEMU 대응 이해 불가
  ▼
Phase 3 (QEMU NVMe + 고급 I/O)
  │  디바이스 에뮬레이션 + io_uring/writeback
  ▼
Phase 4 (커널 수정)
  │  "읽기"에서 "쓰기"로 전환, 코드 수정 경험
  ▼
Phase 5 (SPDK/DPDK)             ← 커널 경로를 알아야 비교 가능
  │  유저공간 I/O, 커널과 1:1 비교
  ▼
Phase 6 (성능 엔지니어링)       ← 전체 경로를 알아야 병목 분석 가능
  │  체계적 벤치마킹, 프로파일링
  ▼
Phase 7 (GPU-Storage)           ← NVMe + PCIe + DMA 이해 필수
  │  GDS, BaM, GPU-initiated I/O
  ▼
Phase 8 (시니어 실전)           ← 모든 지식 통합
     업스트림 기여, 아키텍처 설계, 포트폴리오
```

---

## 학습 완료 후 달성 능력 (시니어 체크리스트)

### Level 5 — 시니어 스토리지 개발자

1. **End-to-End 코드 이해**: write() 한 번이 커널 내부에서 어떤 함수를 몇 번 호출하는지, 변수 값까지 포함하여 설명할 수 있다
2. **성능 디버깅**: I/O 성능 문제가 VFS/Block/NVMe/디바이스 중 어디서 발생하는지 perf/BPF/ftrace로 5분 내에 특정할 수 있다
3. **커널 수정**: 블록 레이어에 커스텀 스케줄러를 추가하거나, NVMe 드라이버에 새 기능을 구현할 수 있다
4. **디바이스 에뮬레이션**: QEMU NVMe 디바이스에 새로운 NVMe 명령을 추가할 수 있다
5. **SPDK 비교**: 커널 NVMe와 SPDK의 성능 차이를 코드 레벨에서 정량적으로 설명할 수 있다
6. **GPU-Storage**: GDS/BaM 아키텍처를 설명하고, GPU-NVMe 직접 I/O 시스템을 설계할 수 있다
7. **업스트림 기여**: Linux 커널에 패치를 제출하고 코드 리뷰를 수행할 수 있다
8. **아키텍처 설계**: "NVMe SSD에 QoS를 추가하라" 같은 요구사항에 대해 설계 문서를 작성하고 trade-off를 분석할 수 있다
9. **성능 분석**: fio/perf/BPF로 체계적 벤치마킹을 수행하고, 결과를 해석하여 최적화 방향을 제시할 수 있다
10. **교육**: 후배 개발자에게 I/O 경로를 가르치고, 코드 리뷰에서 "왜 이렇게 해야 하는가"를 설명할 수 있다
