# 1년 라인바이라인 I/O Flow 완전 정복 커리큘럼

## 커리큘럼 개요

**목표**: Application에서 `write()`/`read()`를 호출하는 순간부터, 커널 내부를 거쳐 QEMU NVMe 가상 디바이스에 데이터가 도달하고 다시 completion이 application까지 돌아오는 전체 경로를 코드 레벨(라인바이라인)로 완전히 이해한다. 최종적으로 커널 소스를 직접 수정하여 I/O 경로에 자신만의 기능을 추가할 수 있는 수준에 도달한다.

**기간**: 12개월 (2026-03 ~ 2027-02)
**방식**: 소스코드 라인바이라인 리딩 → ASCII 다이어그램 정리 → QEMU 환경에서 실험/검증 → 커널 수정 실습

**소스코드 버전 기준**:
- Linux Kernel: `sources/linux` (현재 트리)
- QEMU: `sources/qemu` (현재 트리)
- SPDK: `sources/spdk` (비교 분석용)

---

## 전체 I/O 경로 조감도

```
┌─────────────────────────────────────────────────────────────────────┐
│  APPLICATION (fio, custom app)                                      │
│  write(fd, buf, len)  /  io_submit()  /  io_uring_enter()           │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ syscall (SWI/SYSCALL instruction)
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
                               │ PCIe MMIO Write (doorbell)
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
│  QEMU: nvme_enqueue_req_completion() → msix_notify()                │
│  KERNEL: nvme_irq() → nvme_process_cq() → nvme_pci_complete_rq()   │
│  → blk_mq_end_request() → bio_endio() → dio_complete()             │
│  → complete()/wakeup → userspace return                             │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: 기반 다지기 (Month 1~2)

### Chapter 1: 개발 환경 구축과 QEMU 기반 커널 디버깅 (Week 1~3)

**목표**: QEMU + GDB로 커널 코드를 한 줄씩 따라갈 수 있는 환경을 완성한다. 이후 모든 챕터에서 "코드를 읽고 → 실제로 breakpoint 걸어서 확인"하는 사이클의 토대가 된다.

**역할**: 이 챕터 없이는 나머지 모든 챕터가 "읽기만 하는 학습"에 머문다. 디버깅 환경이 있어야 코드 흐름을 실시간으로 검증할 수 있다.

**학습 파일 및 라인바이라인 대상**:

```
Week 1: 커널 빌드 & QEMU 부팅
─────────────────────────────
1. 커널 빌드 설정
   - sources/linux/Makefile                    → 빌드 시스템 진입점
   - sources/linux/scripts/kconfig/Makefile    → menuconfig 동작 원리
   - CONFIG_DEBUG_INFO=y, CONFIG_GDB_SCRIPTS=y, CONFIG_BLK_DEV_NVME=y
   - CONFIG_NVME_CORE=y 설정 확인

2. QEMU NVMe 디바이스 옵션 이해
   - qemu-system-x86_64 -drive file=test.qcow2,if=none,id=nvm \
     -device nvme,serial=deadbeef,drive=nvm,id=nvme0
   - QEMU NVMe 디바이스 파라미터 전체 목록 파악

3. GDB 연결
   - qemu-system-x86_64 -s -S (gdb stub 활성화)
   - gdb vmlinux → target remote :1234
   - break nvme_queue_rq → continue → 실제 I/O 시 중단 확인

Week 2: rootfs 준비 & fio 설치
──────────────────────────────
1. debootstrap/buildroot로 최소 rootfs 생성
2. fio를 rootfs에 포함 (static build)
3. 부팅 → NVMe 디바이스 인식 확인 (lsblk, nvme list)
4. fio --name=test --ioengine=libaio --direct=1 --bs=4k \
   --rw=randwrite --numjobs=1 --size=128M --filename=/dev/nvme0n1
   → 정상 I/O 수행 확인

Week 3: 디버깅 워크플로 확립
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

실습 과제:
 □ QEMU에서 NVMe 디바이스로 fio 4K random write 수행
 □ GDB로 nvme_queue_rq()에 breakpoint 걸어 한 줄씩 step
 □ ftrace function_graph로 I/O 한 건의 전체 함수 호출 체인 캡처
 □ printk로 submission queue tail 값 변화 로그 출력
```

### Chapter 2: x86 시스템 콜 진입 (Week 4~5)

**목표**: 유저 공간의 `write()` 호출이 어떻게 커널 공간으로 진입하는지, CPU 레벨의 mode switching부터 syscall dispatch table까지 라인바이라인으로 이해한다.

**역할**: I/O flow의 가장 첫 단계. 유저↔커널 경계를 이해해야 이후 VFS, 블록 레이어 학습이 의미를 갖는다. 커널 수정 시 "어디서부터 커널 코드인가"를 정확히 아는 기초가 된다.

**학습 파일 및 라인바이라인 대상**:

```
Week 4: syscall 진입 메커니즘
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
     → 각 필드가 어떤 레지스터에 대응하는지
     → rdi=fd, rsi=buf, rdx=count (write의 경우)

Week 5: syscall 복귀 경로
─────────────────────────
1. 복귀 경로
   - entry_64.S: swapgs, sysretq
   - SYSRET vs IRET 차이 (signal pending 시 IRET)

2. 세 가지 I/O syscall 비교
   - sys_write()      → fs/read_write.c: ksys_write()
   - sys_io_submit()  → fs/aio.c: __x64_sys_io_submit()
   - sys_io_uring_enter() → io_uring/io_uring.c

3. GDB 실습
   - break entry_SYSCALL_64 → fio 실행 → step through
   - 실제 레지스터 값 확인 (info registers)

실습 과제:
 □ GDB로 entry_SYSCALL_64부터 do_syscall_64()까지 step
 □ 간단한 C 프로그램으로 write() 호출 → syscall 번호 확인
 □ strace로 fio의 syscall 패턴 캡처 (libaio vs sync)
 □ pt_regs 구조체 각 필드의 실제 값 GDB로 확인
```

### Chapter 3: VFS 계층 — 파일 디스크립터에서 파일 오퍼레이션까지 (Week 6~8)

**목표**: `sys_write(fd, buf, len)`이 호출된 후 fd → struct file → f_op → write_iter()로 이어지는 VFS 추상화를 완전히 이해한다. 블록 디바이스와 일반 파일시스템의 분기점을 정확히 파악한다.

**역할**: VFS는 "어떤 디바이스/파일시스템이든 동일한 인터페이스로 접근"하게 하는 추상화 계층이다. 이 구조를 이해해야 ext4 경로와 raw block device 경로가 왜 다른 함수로 분기하는지 알 수 있다. 커널 수정 시 "VFS 레벨에서 가로챌 것인가, 블록 레이어에서 가로챌 것인가"를 결정하는 판단 기준이 된다.

**학습 파일 및 라인바이라인 대상**:

```
Week 6: fd → struct file 변환
─────────────────────────────
1. fs/read_write.c: ksys_write()
   라인바이라인:
   → fdget_pos(fd): fd 테이블에서 struct fd 획득
     - include/linux/file.h: struct fd { struct file *file; unsigned int flags; }
     - fs/file.c: __fdget_pos() → __fget_light()
     → current->files->fdt->fd[fd]에서 struct file* 추출
   → file_ppos(f.file): f_pos 포인터 획득
   → vfs_write(f.file, buf, count, ppos) 호출

2. fs/read_write.c: vfs_write()
   라인바이라인:
   → rw_verify_area(): 접근 권한/보안 검사
   → __vfs_write() 또는 new_sync_write()
   → file->f_op->write_iter(kio, iter) 호출
     핵심: f_op가 가리키는 operations 구조체가 무엇인지에 따라 경로 결정

3. struct file_operations 분석
   - include/linux/fs.h: struct file_operations
     → .write_iter, .read_iter, .open, .release 등
   - 블록 디바이스: fs/block_dev.c의 def_blk_fops
   - ext4: fs/ext4/file.c의 ext4_file_operations
   - 비교표 작성: 어떤 함수 포인터가 다른지

Week 7: 블록 디바이스 경로 (raw device)
──────────────────────────────────────
1. block/fops.c: blkdev_write_iter()
   라인바이라인:
   → 파일 위치 검사, 크기 제한 검사
   → iocb->ki_flags & IOCB_DIRECT 분기:
     Direct I/O → blkdev_direct_IO()
     Buffered  → generic_perform_write() → __block_write_begin()

2. blkdev_direct_IO() → __blkdev_direct_IO()
   라인바이라인:
   → bio_alloc(bdev, ...) : bio 할당
   → __bio_add_page() : 사용자 버퍼 페이지를 bio에 추가
   → submit_bio(bio) : 블록 레이어로 전달
   → Direct I/O 완료 대기 (동기) 또는 kiocb callback (비동기)

3. iov_iter 구조체 분석
   - include/linux/uio.h: struct iov_iter
   → 사용자 버퍼를 커널이 어떻게 추상화하는지
   → ITER_UBUF, ITER_IOVEC, ITER_BVEC 등 타입별 차이

Week 8: ext4 파일시스템 경로
───────────────────────────
1. fs/ext4/file.c: ext4_file_write_iter()
   라인바이라인:
   → ext4_dio_write_iter() (Direct I/O)
   → ext4_buffered_write_iter() (Buffered I/O)

2. Direct I/O 경로
   - ext4_dio_write_iter()
   → iomap_dio_rw() (fs/iomap/direct-io.c)
   → __iomap_dio_rw()
     라인바이라인:
     → iomap_iter()로 extent 매핑 (논리→물리 블록 변환)
     → iomap_dio_bio_iter()에서 bio 생성
     → submit_bio()

3. Buffered I/O 경로 (개요)
   - generic_perform_write()
   → page cache에 쓰기 → dirty page 생성
   → writeback daemon이 나중에 submit_bio()
   (이 경로는 Phase 3에서 상세 분석)

4. ext4 block mapping
   - fs/ext4/extents.c: ext4_ext_map_blocks()
   → extent tree에서 논리 블록 → 물리 블록 변환
   → 이 물리 블록 번호가 bio->bi_iter.bi_sector가 됨

실습 과제:
 □ GDB로 ksys_write() → vfs_write() → blkdev_write_iter() step
 □ /dev/nvme0n1에 직접 write할 때와 ext4 마운트 후 파일 write 비교
 □ f_op 포인터가 실제로 어떤 함수를 가리키는지 GDB로 확인
 □ struct bio를 GDB에서 inspect: bi_iter.bi_sector, bi_vcnt 값 확인
 □ ftrace로 ext4_dio_write_iter → submit_bio 호출 체인 캡처
```

---

## Phase 2: Block Layer 심층 분석 (Month 3~5)

### Chapter 4: bio — 블록 I/O의 기본 단위 (Week 9~11)

**목표**: `struct bio`의 생성부터 소멸까지 전체 생명주기를 이해한다. bio가 어떻게 사용자 데이터를 physical page로 매핑하고, 디바이스 드라이버에게 전달되는 형태로 변환되는지 라인바이라인으로 추적한다.

**역할**: bio는 블록 레이어의 가장 기본적인 데이터 구조다. "사용자가 쓴 데이터가 디스크에 어떤 형태로 전달되는가"의 답이 bio에 담겨 있다. 이후 blk-mq, NVMe 드라이버 학습에서 bio→request 변환을 이해하려면 bio 자체를 먼저 완벽히 알아야 한다.

**학습 파일 및 라인바이라인 대상**:

```
Week 9: struct bio 구조체 해부
─────────────────────────────
1. include/linux/bio.h + include/linux/blk_types.h
   struct bio 핵심 필드:
   - bi_iter: struct bvec_iter (sector, size, idx, bvec_done)
   - bi_io_vec: struct bio_vec 배열 (page, offset, len)
   - bi_end_io: completion callback
   - bi_private: 상위 레이어 context
   - bi_opf: operation + flags (REQ_OP_READ/WRITE, REQ_SYNC 등)
   - bi_bdev: target block device

2. struct bio_vec 분석
   - include/linux/bvec.h
   → { struct page *bv_page; unsigned bv_len; unsigned bv_offset; }
   → 사용자 버퍼 페이지를 가리키는 scatter-gather element

3. struct bvec_iter 분석
   - bi_sector: 디바이스 내 시작 섹터 (512B 단위)
   - bi_size: 남은 바이트 수
   - bi_idx: 현재 bio_vec 인덱스
   - bio_advance(): iterator를 전진시키는 로직

Week 10: bio 할당과 페이지 매핑
──────────────────────────────
1. block/bio.c: bio_alloc_bioset()
   라인바이라인:
   → mempool_alloc()로 bio 메모리 확보
   → bio_init(): 필드 초기화
   → bi_max_vecs 설정 (최대 bio_vec 개수)

2. block/bio.c: bio_add_page()
   라인바이라인:
   → __bio_try_merge_page(): 이전 bio_vec과 merge 시도
   → 실패 시 새 bio_vec 추가
   → bv->bv_page = page, bv->bv_offset = off, bv->bv_len = len

3. Direct I/O에서의 bio 구성 과정
   - bio_iov_iter_get_pages(): 사용자 가상 주소 → struct page 변환
   → get_user_pages_fast() → 페이지 테이블 워크
   → pin된 페이지를 bio_vec에 추가

4. bio splitting
   - block/bio.c: bio_split()
   → 디바이스의 max_sectors 한계 초과 시 bio를 분할
   → bio_chain()으로 분할된 bio들을 연결

Week 11: bio 제출과 완료
────────────────────────
1. block/blk-core.c: submit_bio()
   라인바이라인:
   → submit_bio_noacct() → __submit_bio_noacct()
   → bio_set_dev() 확인
   → blk_mq_submit_bio()로 진입 (다음 챕터에서 상세)

2. bio 완료 경로
   - block/bio.c: bio_endio()
   라인바이라인:
   → bio_remaining_done(): bi_remaining 감소
   → bio->bi_end_io(bio) callback 호출
   → Direct I/O: blkdev_bio_end_io() → kiocb->ki_complete()
   → Buffered: end_bio_bh_io_sync() → page unlock

3. bio chaining과 완료 순서
   - bio_chain()으로 연결된 bio들의 완료 순서
   → child bio 완료 → parent bio의 bi_remaining 감소
   → 모든 child 완료 시 parent의 bi_end_io 호출

실습 과제:
 □ GDB로 bio_alloc → bio_add_page 과정에서 bio_vec 값 확인
 □ 4KB write 시 bio_vec이 1개, 128KB write 시 여러 개인지 확인
 □ bio->bi_iter.bi_sector 값과 실제 디스크 LBA 관계 확인
 □ bio_split이 발생하는 조건 만들기 (매우 큰 I/O 제출)
 □ bio_endio에 printk 추가하여 완료 시점 확인
```

### Chapter 5: blk-mq — Multi-Queue Block Layer (Week 12~16)

**목표**: 현대 리눅스 블록 레이어의 핵심인 blk-mq의 전체 아키텍처를 이해한다. Software queue → Hardware queue 매핑, request 할당과 병합, dispatch 과정을 모든 주요 함수에 대해 라인바이라인으로 추적한다.

**역할**: blk-mq는 application의 I/O 의도(bio)를 디바이스 드라이버가 처리할 수 있는 형태(request)로 변환하는 핵심 변환 계층이다. NVMe 드라이버의 멀티큐 구조는 blk-mq 위에서 동작하므로, blk-mq를 모르면 NVMe 드라이버 코드를 이해할 수 없다. 커널 수정으로 I/O 스케줄링이나 우선순위 정책을 바꾸려면 이 계층을 완벽히 이해해야 한다.

**학습 파일 및 라인바이라인 대상**:

```
Week 12: blk-mq 아키텍처 개요와 자료구조
────────────────────────────────────────
1. include/linux/blk-mq.h: 핵심 구조체
   - struct blk_mq_hw_ctx (hctx): hardware dispatch queue
     → .queue: 소속 request_queue
     → .dispatch: dispatch 대기 리스트
     → .nr_ctx: 매핑된 software ctx 수
     → .tags, .sched_tags: tag 관리
     → .flags: BLK_MQ_F_* 플래그

   - struct blk_mq_ctx (ctx): per-CPU software staging queue
     → .rq_lists[]: 타입별(READ/WRITE/etc) request 리스트
     → .cpu: 이 ctx가 속한 CPU 번호

   - struct blk_mq_ops: 드라이버 콜백
     → .queue_rq: request 처리 (NVMe: nvme_queue_rq)
     → .complete: completion 처리
     → .init_hctx, .exit_hctx: hctx 초기화/해제
     → .map_queues: CPU→queue 매핑

2. include/linux/blk-mq.h: struct blk_mq_tag_set
   - .ops: blk_mq_ops 포인터
   - .nr_hw_queues: hardware queue 수
   - .queue_depth: 큐당 최대 outstanding request 수
   - .cmd_size: request 뒤에 붙는 드라이버 private 크기
   - .tags[]: per-hctx tag 배열

3. CPU → Software Queue → Hardware Queue 매핑
   - blk_mq_map_queues() 분석
   → 기본: CPU i → hctx[i % nr_hw_queues]
   → NVMe: nvme_pci_map_queues()로 커스텀 매핑

Week 13: request 할당과 bio→request 변환
───────────────────────────────────────
1. block/blk-mq.c: blk_mq_submit_bio()
   전체 함수 라인바이라인:
   → __blk_mq_alloc_requests(): tag 할당 + request 메모리 획득
     - blk_mq_get_tag(): sbitmap에서 빈 tag 찾기
     - include/linux/sbitmap.h: struct sbitmap_queue
       → 비트맵 기반 lock-free tag 할당 메커니즘
   → blk_mq_bio_to_request(): bio 내용을 request에 복사
     - rq->__sector = bio->bi_iter.bi_sector
     - rq->__data_len = bio->bi_iter.bi_size
     - rq->bio = rq->biotail = bio

2. request 병합 (merge)
   - block/blk-merge.c: blk_attempt_plug_merge()
   → current->plug 리스트에서 인접 request 검색
   → 연속 섹터면 bio를 기존 request에 병합

   - block/blk-merge.c: blk_mq_sched_try_merge()
   → I/O 스케줄러의 merge 시도
   → elevator_merge_fn() 호출

3. plug/unplug 메커니즘
   - include/linux/blkdev.h: struct blk_plug
   → blk_start_plug(): 현재 태스크에 plug 설정
   → I/O들을 plug 리스트에 모음 (batching)
   → blk_finish_plug() → blk_flush_plug_list()
   → 모인 request들을 한꺼번에 dispatch

Week 14: dispatch — request를 드라이버에 전달
────────────────────────────────────────────
1. block/blk-mq.c: blk_mq_run_hw_queue()
   라인바이라인:
   → __blk_mq_delay_run_hw_queue()
   → __blk_mq_run_hw_queue()
   → blk_mq_sched_dispatch_requests()

2. block/blk-mq-sched.c: blk_mq_sched_dispatch_requests()
   라인바이라인:
   → 스케줄러가 있으면: e->type->ops.dispatch_request()
   → 스케줄러가 없으면: blk_mq_dispatch_rq_list()에 직접

3. block/blk-mq.c: blk_mq_dispatch_rq_list()
   핵심 함수 라인바이라인:
   → list_for_each_entry_safe(rq, ...) {
       ret = q->mq_ops->queue_rq(hctx, &bd);
       // bd.rq = rq, bd.last = (마지막 request인지)
       switch(ret) {
         BLK_STS_OK: 성공, 다음 request
         BLK_STS_RESOURCE: 리소스 부족, 재시도 예약
         BLK_STS_DEV_RESOURCE: 디바이스 busy
       }
     }
   → queue_rq()가 NVMe 드라이버의 nvme_queue_rq()

Week 15: tag & sbitmap 심층 분석
───────────────────────────────
1. include/linux/sbitmap.h: sbitmap 자료구조
   - struct sbitmap: multi-word bitmap
   - struct sbitmap_queue: wait queue 포함
   → lock-free allocation: per-CPU hint 기반

2. lib/sbitmap.c: sbitmap_get()
   라인바이라인:
   → __sbitmap_get_word(): word 단위 비트 검색
   → test_and_set_bit_lock(): atomic으로 tag 확보
   → 실패 시 다음 word로 이동

3. tag와 request의 관계
   - tag = request 배열의 index
   - NVMe command ID = tag (디바이스가 completion 시 반환)
   → blk_mq_tag_to_rq(): tag → request 역변환

4. tag 부족 시 동작
   - blk_mq_get_tag()에서 대기
   → io_schedule() → 다른 I/O 완료될 때까지 sleep
   → wakeup → 재시도

Week 16: I/O 스케줄러 (None / mq-deadline / kyber)
──────────────────────────────────────────────────
1. 스케줄러 없음 (none)
   - bio → request → 바로 dispatch
   - NVMe에서 가장 일반적 (디바이스 자체가 스케줄링)

2. mq-deadline 스케줄러
   - block/mq-deadline.c
   → dd_insert_request(): deadline 기반 RB tree 삽입
   → dd_dispatch_request(): deadline 만료 순 또는 FIFO
   → read starvation 방지 로직

3. kyber 스케줄러
   - block/kyber-iosched.c
   → token 기반 rate limiting
   → read/write/discard/other 4개 도메인 분리
   → latency target 기반 자동 조절

4. 스케줄러와 NVMe의 관계
   - NVMe는 보통 none이 기본
   - cat /sys/block/nvme0n1/queue/scheduler로 확인
   - 스케줄러 변경의 성능 영향 측정

실습 과제:
 □ GDB로 blk_mq_submit_bio() 전체를 step through
 □ request의 tag 값과 NVMe command ID가 같은지 확인
 □ blk_start_plug/blk_finish_plug에 printk → batching 동작 확인
 □ sbitmap에서 tag 할당 과정을 GDB로 관찰
 □ none/mq-deadline 스케줄러 전환 → fio 성능 비교
 □ /sys/kernel/debug/block/nvme0n1/ 디버그 정보 분석
```

### Chapter 6: Request 완료 경로 — 디바이스에서 Application까지 (Week 17~18)

**목표**: NVMe 디바이스(QEMU)가 I/O를 완료한 후 인터럽트 → completion queue 처리 → request 완료 → bio_endio → 사용자 프로세스 깨우기까지의 역방향 경로를 라인바이라인으로 추적한다.

**역할**: I/O flow의 후반부. 제출 경로만 알면 절반만 아는 것이다. Completion 경로를 이해해야 "I/O가 끝났는데 왜 application이 아직 안 깨어나는가" 같은 문제를 디버깅할 수 있다. 성능 최적화(interrupt coalescing, polling 등)의 기반이 된다.

**학습 파일 및 라인바이라인 대상**:

```
Week 17: blk-mq completion 경로
───────────────────────────────
1. block/blk-mq.c: blk_mq_end_request()
   라인바이라인:
   → __blk_mq_end_request()
   → blk_update_request(): 바이트 수 갱신, bio_endio() 호출
   → __blk_mq_free_request(): tag 해제, request 반환

2. block/blk-core.c: blk_update_request()
   라인바이라인:
   → req_bio_endio(rq, bio, nbytes): bio 완료 처리
   → bio->bi_status 설정
   → bio_endio(bio) 호출
   → rq->bio = bio->bi_next (다음 bio로 이동)

3. softirq vs direct completion
   - blk_mq_complete_request()
   → IPI 또는 softirq로 완료 처리를 적절한 CPU에 전달
   → blk_mq_complete_request_remote() 분석

Week 18: 비동기 I/O 완료 통지
─────────────────────────────
1. io_submit() + io_getevents() 경로 (AIO)
   - fs/aio.c: aio_complete_rw()
   → completion event를 ring buffer에 기록
   → io_getevents()에서 사용자가 수확

2. io_uring 완료 경로
   - io_uring/io_uring.c: io_complete_rw()
   → CQE(Completion Queue Entry)를 공유 ring에 기록
   → eventfd 또는 polling으로 통지

3. 동기 write의 완료
   - direct I/O: iomap_dio_complete()
   → kiocb->ki_complete() 또는 직접 return
   - buffered I/O: 함수 리턴 (page cache에 쓰기 완료)

실습 과제:
 □ GDB로 blk_mq_end_request() → bio_endio() step
 □ NVMe 인터럽트 → blk_mq_complete_request() 경로 ftrace
 □ io_uring CQE가 실제로 어떻게 유저 공간 ring에 기록되는지 확인
 □ completion이 어떤 CPU에서 실행되는지 확인 (mpstat + ftrace)
```

---

## Phase 3: NVMe Driver 심층 분석 (Month 5~7)

### Chapter 7: NVMe PCIe 드라이버 초기화 (Week 19~22)

**목표**: 시스템 부팅 시 NVMe PCIe 디바이스가 어떻게 발견되고 드라이버에 바인딩되어 사용 가능한 상태가 되는지, probe부터 queue 생성까지 전체 초기화 과정을 라인바이라인으로 분석한다.

**역할**: NVMe 드라이버가 I/O를 처리하려면 먼저 초기화가 완료되어야 한다. Admin Queue 생성, I/O Queue 생성, Namespace 등록 순서를 이해해야 "디바이스가 왜 안 보이는가", "큐가 몇 개 생기는가" 같은 질문에 답할 수 있다. 커널 수정으로 큐 설정을 바꾸거나 커스텀 Admin 명령을 추가하려면 이 초기화 과정을 완벽히 알아야 한다.

**학습 파일 및 라인바이라인 대상**:

```
Week 19: PCIe 디바이스 발견과 드라이버 매칭
──────────────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_pci_driver 구조체
   - .id_table = nvme_id_table
   → { PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_EXPRESS, 0xffffff) }
   → PCI class code 0x010802가 NVMe
   - .probe = nvme_probe

2. PCIe 열거 과정 (개요)
   - drivers/pci/probe.c: pci_scan_slot() → pci_scan_single_device()
   → pci_device_add() → device_add()
   → bus_probe_device() → __device_attach()
   → driver_match_device() → pci_bus_match()
   → PCI class code 비교 → match!
   → driver_probe_device() → pci_device_probe()
   → drv->probe(pci_dev) = nvme_probe() 호출

3. QEMU에서의 NVMe PCIe 디바이스 등록
   - QEMU 측: -device nvme가 어떻게 PCI config space를 설정하는지
   → sources/qemu/hw/nvme/ctrl.c: nvme_class_init()
   → PCI class code, vendor/device ID 설정

Week 20: nvme_probe() — 드라이버 초기화 시작
───────────────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_probe()
   전체 함수 라인바이라인:
   → nvme_dev_alloc(): struct nvme_dev 할당
   → pci_enable_device_mem(): PCI 디바이스 활성화
   → pci_set_master(): bus mastering 활성화 (DMA 가능)
   → pci_request_mem_regions(): BAR 영역 요청
   → nvme_map_bar(): BAR0를 ioremap (MMIO 접근 가능하게)
     → dev->bar = ioremap(pci_resource_start(pdev, 0), ...)
   → nvme_setup_prp_pools(): PRP(Physical Region Page) 리스트 풀 생성
   → nvme_init_ctrl(): NVMe core 초기화
   → nvme_reset_ctrl(): 리셋 시작 → 비동기 워크 큐잉

2. BAR 매핑 상세
   - NVMe spec의 Controller Registers (BAR0)
   → 0x00: CAP (Capabilities)
   → 0x08: VS (Version)
   → 0x0C: INTMS/INTMC
   → 0x14: CC (Controller Configuration)
   → 0x1C: CSTS (Controller Status)
   → 0x24: AQA (Admin Queue Attributes)
   → 0x28: ASQ (Admin SQ Base Address)
   → 0x30: ACQ (Admin CQ Base Address)
   → 0x1000+: Doorbell registers

Week 21: 리셋과 Admin Queue 생성
───────────────────────────────
1. drivers/nvme/host/pci.c: nvme_reset_work()
   전체 함수 라인바이라인:
   → nvme_pci_enable(): PCI enable + 인터럽트 설정
   → nvme_pci_configure_admin_queue(): Admin Queue 생성
     → nvme_alloc_queue(): nvme_queue 구조체 할당
     → dma_alloc_coherent(): SQ/CQ 메모리 할당 (DMA용)
     → writel(aqa, dev->bar + NVME_REG_AQA): 큐 크기 설정
     → lo_hi_writeq(sq_dma, dev->bar + NVME_REG_ASQ): SQ 주소 설정
     → lo_hi_writeq(cq_dma, dev->bar + NVME_REG_ACQ): CQ 주소 설정
     → nvme_enable_ctrl(): CC.EN=1 쓰기 → 컨트롤러 활성화
     → nvme_wait_ready(): CSTS.RDY=1 대기

2. Admin 명령으로 컨트롤러 정보 획득
   → nvme_identify_ctrl(): Identify Controller 명령
   → nvme_set_features(): Feature 설정 (Interrupt Coalescing 등)

Week 22: I/O Queue 생성과 Namespace 등록
───────────────────────────────────────
1. nvme_reset_work() 후반부
   → nvme_setup_io_queues():
     - nvme_create_io_queues():
       → nvme_alloc_queue(): I/O queue 할당
       → adapter_alloc_cq(): CQ 생성 Admin 명령
       → adapter_alloc_sq(): SQ 생성 Admin 명령
     - CPU 수와 queue 수 결정 로직
     → min(cpu 수, 디바이스 지원 최대 queue 수)

2. blk-mq tag_set 등록
   → nvme_dev_add()
   → blk_mq_alloc_tag_set(): blk-mq에 tag set 등록
     - .ops = nvme_mq_ops
     - .nr_hw_queues = dev->online_queues - 1
     - .queue_depth = dev->q_depth
     - .cmd_size = sizeof(struct nvme_iod)

3. Namespace 스캔과 디스크 등록
   → nvme_scan_work()
   → nvme_identify_ns(): Namespace Identify 명령
   → nvme_alloc_ns() → alloc_disk() → device_add_disk()
   → /dev/nvme0n1 등장!

실습 과제:
 □ GDB로 nvme_probe() 전체를 step (QEMU 부팅 시)
 □ BAR0 매핑 후 레지스터 값 GDB로 읽기 (x/8x dev->bar)
 □ Admin Queue의 DMA 주소가 BAR0 레지스터에 실제 쓰이는지 확인
 □ I/O Queue 수를 강제로 1개로 제한 → 성능 변화 측정
 □ dmesg에서 nvme 초기화 로그와 코드 라인 1:1 매핑
 □ /sys/class/nvme/nvme0/ 아래 속성 파일과 코드 연결
```

### Chapter 8: NVMe I/O 제출 경로 — nvme_queue_rq() (Week 23~26)

**목표**: blk-mq의 `queue_rq()` 콜백인 `nvme_queue_rq()`가 호출되어 NVMe 명령을 조립하고, Submission Queue에 기록하고, Doorbell을 울리는 전체 과정을 instruction 수준으로 이해한다.

**역할**: 이 챕터가 전체 커리큘럼의 핵심 중 핵심이다. "커널 코드가 실제로 하드웨어에 명령을 보내는 정확한 순간"이 여기에 있다. 커널 수정으로 I/O 명령에 커스텀 필드를 추가하거나, I/O 통계를 수집하거나, 특정 조건에서 I/O를 거부하는 기능을 만들려면 이 함수를 완벽히 이해해야 한다.

**학습 파일 및 라인바이라인 대상**:

```
Week 23: nvme_queue_rq() 전체 분석
─────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_queue_rq()
   전체 함수 라인바이라인:

   static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
                                      const struct blk_mq_queue_data *bd)
   {
     struct nvme_queue *nvmeq = hctx->driver_data;
     struct nvme_dev *dev = nvmeq->dev;
     struct request *req = bd->rq;
     struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
     // ↑ request 뒤에 붙어있는 드라이버 private 데이터
     struct nvme_command *cmnd = &iod->cmd;
     blk_status_t ret;

     // 1단계: NVMe 명령 조립
     ret = nvme_setup_cmd(req->q->disk->part0, req);
     // 2단계: 데이터 DMA 매핑
     ret = nvme_map_data(dev, req, cmnd);
     // 3단계: SQ에 명령 기록 & doorbell
     nvme_submit_cmd(nvmeq, cmnd, bd->last);
   }

2. 각 단계를 아래 Week에서 상세 분석

Week 24: NVMe 명령 조립 — nvme_setup_cmd()
──────────────────────────────────────────
1. drivers/nvme/host/core.c: nvme_setup_cmd()
   라인바이라인:
   → req_op(req) switch:
     REQ_OP_READ/WRITE: nvme_setup_rw()
     REQ_OP_FLUSH:      nvme_setup_flush()
     REQ_OP_DISCARD:    nvme_setup_discard()

2. drivers/nvme/host/core.c: nvme_setup_rw()
   라인바이라인:
   → cmnd->rw.opcode = (write ? nvme_cmd_write : nvme_cmd_read)
   → cmnd->rw.nsid = cpu_to_le32(ns->head->ns_id)
   → cmnd->rw.slba = cpu_to_le64(nvme_sect_to_lba(ns, blk_rq_pos(req)))
     // ↑ 섹터 번호 → LBA 변환
   → cmnd->rw.length = cpu_to_le16((blk_rq_bytes(req) >> ns->lba_shift) - 1)
     // ↑ 바이트 → LBA 개수 변환
   → cmnd->rw.control, cmnd->rw.dsmgmt: FUA, 스트림 등 설정

3. NVMe Command 구조체 분석
   - include/linux/nvme.h: struct nvme_rw_command
   → opcode(1B), flags(1B), command_id(2B)
   → nsid(4B), slba(8B), length(2B)
   → control(2B), dsmgmt(4B)
   → prp1(8B), prp2(8B) 또는 SGL

Week 25: DMA 매핑 — nvme_map_data()
───────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_map_data()
   라인바이라인:
   → sg_init_table(): scatter-gather 리스트 초기화
   → blk_rq_map_sg(): request의 bio_vec → SG 리스트 변환
   → dma_map_sg(): 가상 주소 → DMA(물리/IOMMU) 주소 변환
   → nvme_pci_setup_prps() 또는 nvme_pci_setup_sgls()

2. PRP (Physical Region Page) 리스트 구성
   - nvme_pci_setup_prps()
   라인바이라인:
   → 첫 번째 PRP: cmnd->rw.dptr.prp1 = sg_dma_address(sg)
   → 4KB 이하: PRP1만으로 충분
   → 4KB~8KB: PRP2에 두 번째 페이지 주소
   → 8KB 초과: PRP2가 PRP List 주소를 가리킴
     → PRP List: DMA coherent 메모리에 물리 주소 배열
     → 각 엔트리가 데이터 페이지의 물리 주소

3. IOMMU/DMA 주소 변환 (개요)
   - kernel/dma/mapping.c: dma_map_sg_attrs()
   → IOMMU 있으면: IOMMU 페이지 테이블로 변환
   → IOMMU 없으면: virt_to_phys() 직접 변환
   → QEMU에서는 보통 IOMMU 없이 직접 변환

Week 26: SQ 기록과 Doorbell — nvme_submit_cmd()
───────────────────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_submit_cmd()
   라인바이라인:
   → spin_lock(&nvmeq->sq_lock) (또는 lockless 경로)
   → memcpy(&nvmeq->sq_cmds[tail], cmd, sizeof(*cmd))
     // ↑ SQ의 tail 위치에 64B NVMe 명령 복사
   → nvme_write_sq_db()

2. nvme_write_sq_db()
   라인바이라인:
   → nvmeq->sq_tail = (tail + 1) % nvmeq->q_depth
   → if (nvme_dbbuf_update_and_check_event(tail, ...))
       writel(tail, nvmeq->q_db)
     // ↑ 이 writel()이 PCIe MMIO Write
     // ↑ QEMU 쪽에서 이 write를 감지하여 처리 시작

3. Doorbell Buffer Config (Shadow Doorbell)
   - nvme_dbbuf_update_and_check_event() 분석
   → DBBUF가 활성화되면 메모리에 먼저 쓰고
   → EventIdx와 비교하여 실제 MMIO 필요 여부 결정
   → MMIO 횟수 감소 → 성능 향상

4. 전체 명령 제출 과정 ASCII 다이어그램

   CPU                                NVMe Controller (QEMU)
    │                                        │
    │  memcpy(SQ[tail], cmd)                  │
    │  (호스트 메모리에 명령 기록)             │
    │                                        │
    │  writel(tail, doorbell_reg)             │
    │ ──── PCIe MMIO Write ─────────────────→│
    │                                        │
    │                      doorbell 감지       │
    │                      SQ에서 명령 읽기    │
    │                      (DMA Read)         │
    │←──── PCIe DMA Read ────────────────────│
    │                                        │
    │                      명령 처리           │
    │                      CQ에 완료 기록      │
    │                      (DMA Write)        │
    │←──── PCIe DMA Write ───────────────────│
    │                                        │
    │←──── MSI-X Interrupt ──────────────────│
    │                                        │

실습 과제:
 □ GDB로 nvme_queue_rq() 전체를 step — 모든 변수 값 확인
 □ nvme_setup_rw()에서 조립된 NVMe 명령의 opcode, slba, length 확인
 □ PRP 리스트의 실제 DMA 주소와 사용자 버퍼 물리 주소 비교
 □ writel(doorbell) 직전/직후의 SQ tail 값 GDB로 확인
 □ SQ 메모리의 실제 내용을 GDB로 덤프 (x/16x nvmeq->sq_cmds)
 □ Shadow Doorbell 활성화/비활성화 시 writel 호출 빈도 비교
```

### Chapter 9: NVMe Completion 경로 (Week 27~29)

**목표**: QEMU NVMe 디바이스가 MSI-X 인터럽트를 발생시킨 후, 커널의 인터럽트 핸들러가 Completion Queue를 읽고, request를 완료하여 application에게 결과를 전달하는 전체 과정을 라인바이라인으로 추적한다.

**역할**: 제출 경로의 정확한 대칭. Completion 경로에서의 지연이 전체 I/O latency에 직접 영향을 미친다. Interrupt coalescing, polling mode (io_poll) 등 성능 최적화 기법의 이해 기반이 된다.

**학습 파일 및 라인바이라인 대상**:

```
Week 27: 인터럽트 핸들러와 CQ 처리
─────────────────────────────────
1. drivers/nvme/host/pci.c: nvme_irq()
   라인바이라인:
   → nvme_process_cq()

2. nvme_process_cq()
   라인바이라인:
   → while ((head = nvme_cqe_pending(nvmeq))) {
       cqe = &nvmeq->cqes[head]
       // CQE에서 정보 추출:
       // cqe->command_id → 원본 명령의 tag
       // cqe->status → 완료 상태 (성공/에러)
       // cqe->result → 명령별 결과값
       nvme_handle_cqe(nvmeq, head)
       head = (head + 1) % nvmeq->q_depth
     }
   → nvme_ring_cq_doorbell(): CQ head를 doorbell에 기록
     → writel(head, nvmeq->q_db + nvmeq->dev->db_stride)

3. nvme_handle_cqe()
   라인바이라인:
   → tag = cqe->command_id
   → rq = blk_mq_tag_to_rq(tags, tag): tag → request 변환
   → nvme_pci_complete_rq(rq)

4. nvme_pci_complete_rq()
   → nvme_unmap_data(): DMA 매핑 해제
   → nvme_complete_rq() (core.c)
   → blk_mq_end_request() → (Chapter 6의 경로)

Week 28: MSI-X 인터럽트 상세
───────────────────────────
1. 인터럽트 등록
   - nvme_setup_irqs(): pci_alloc_irq_vectors_affinity()
   → MSI-X 벡터 할당 (큐 수만큼)
   - queue_request_irq(): request_irq(irq, nvme_irq, ...)
   → 각 CQ에 대한 인터럽트 핸들러 등록

2. x86 인터럽트 처리 경로 (개요)
   - MSI-X write → LAPIC → CPU interrupt
   - arch/x86/kernel/irq.c: do_IRQ()
   → irq_enter() → handle_irq() → nvme_irq()
   → irq_exit() → softirq 처리

3. Interrupt Coalescing
   - NVMe spec: Feature 08h (Interrupt Coalescing)
   → aggregation time + threshold 설정
   → nvme_set_features()로 설정

Week 29: Polling Mode (io_poll)
──────────────────────────────
1. drivers/nvme/host/pci.c: nvme_poll()
   라인바이라인:
   → 인터럽트 없이 CQ를 직접 polling
   → nvme_process_cq() 호출 (인터럽트 경로와 동일)

2. blk-mq polling 프레임워크
   - block/blk-mq.c: blk_mq_poll()
   → q->mq_ops->poll(hctx) = nvme_poll()

3. io_uring과 polling
   - io_uring에서 IORING_SETUP_IOPOLL 플래그
   → io_do_iopoll() → blk_mq_poll()
   → interrupt-free I/O 완료

4. Interrupt vs Polling 성능 비교
   - fio: --hipri=1 (polling mode)
   - latency 비교: interrupt ~10μs vs polling ~2μs (QEMU에서는 다를 수 있음)

실습 과제:
 □ GDB로 nvme_irq() → nvme_process_cq() 전체 step
 □ CQE의 command_id와 원본 request의 tag 일치 확인
 □ CQ doorbell write (writel) 확인
 □ MSI-X 벡터와 CPU affinity 매핑 확인 (/proc/interrupts)
 □ polling mode vs interrupt mode 성능 비교 (fio --hipri)
 □ interrupt coalescing 설정 변경 → latency 변화 측정
```

---

## Phase 4: QEMU NVMe Device Emulation (Month 7~9)

### Chapter 10: QEMU NVMe 컨트롤러 구조 (Week 30~33)

**목표**: QEMU가 NVMe 디바이스를 어떻게 에뮬레이션하는지 — PCI config space 구성, BAR 매핑, MMIO 핸들러 등록 과정을 라인바이라인으로 분석한다. 커널 드라이버가 하는 동작의 "반대편"을 이해한다.

**역할**: 지금까지 커널 쪽에서 "doorbell을 울리면 디바이스가 처리한다"고 했는데, 그 "디바이스" 내부가 바로 이 QEMU 코드다. 실제 하드웨어 대신 QEMU 코드를 읽으면 NVMe 스펙의 동작을 소프트웨어로 볼 수 있다. 향후 QEMU NVMe 디바이스를 수정하여 커스텀 NVMe 기능을 테스트할 수 있다.

**학습 파일 및 라인바이라인 대상**:

```
Week 30: QEMU 디바이스 모델 기초
───────────────────────────────
1. QEMU QOM (QEMU Object Model) 이해
   - hw/nvme/ctrl.c: nvme_class_init()
     → dc->realize = nvme_realize
     → set_bit(DEVICE_CATEGORY_STORAGE, dc->categories)
     → device_class_set_props(dc, nvme_props)

2. NVMe 디바이스 프로퍼티
   - hw/nvme/ctrl.c: nvme_props[]
     → DEFINE_PROP_STRING("serial", ...)
     → DEFINE_PROP_UINT32("cmb_size_mb", ...)
     → DEFINE_PROP_UINT16("mqes", ...) : 최대 queue entries
     → DEFINE_PROP_UINT8("mdts", ...) : 최대 data transfer size

3. PCI config space 설정
   - hw/nvme/ctrl.c: nvme_realize() 내부
     → pci_config_set_class(pci_conf, PCI_CLASS_STORAGE_EXPRESS)
     → pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_INTEL)
     → pci_config_set_prog_interface(pci_conf, 0x02) → NVMe

Week 31: BAR 등록과 MMIO 핸들러
──────────────────────────────
1. nvme_realize() 계속
   → memory_region_init_io(&n->iomem, ..., &nvme_mmio_ops, ...)
     // ↑ BAR0에 대한 MMIO 핸들러 등록
   → pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &n->iomem)
     // ↑ BAR0으로 등록

2. hw/nvme/ctrl.c: nvme_mmio_ops
   - .read = nvme_mmio_read
   - .write = nvme_mmio_write

3. nvme_mmio_read() 분석
   라인바이라인:
   → offset으로 어떤 레지스터인지 판별
   → switch(offset):
     NVME_REG_CAP: return n->bar.cap
     NVME_REG_VS:  return n->bar.vs
     NVME_REG_CSTS: return n->bar.csts
     ...

4. nvme_mmio_write() 분석
   라인바이라인:
   → switch(offset):
     NVME_REG_CC:  nvme_write_bar(n, offset, data) → 컨트롤러 설정
     NVME_REG_AQA: n->bar.aqa = data
     NVME_REG_ASQ: n->bar.asq = data
     NVME_REG_ACQ: n->bar.acq = data
     default:      → doorbell 영역이면 nvme_process_db()

Week 32: Admin Queue 생성 (QEMU 측)
──────────────────────────────────
1. CC.EN=1 처리
   - nvme_write_bar()에서 CC 레지스터 쓰기 감지
   → nvme_ctrl_enable() 또는 nvme_start_ctrl()
     라인바이라인:
     → n->bar.aqa에서 SQ/CQ 크기 추출
     → n->bar.asq에서 Admin SQ 주소 추출
     → n->bar.acq에서 Admin CQ 주소 추출
     → nvme_init_sq(): SQ 구조체 초기화
     → nvme_init_cq(): CQ 구조체 초기화
     → n->bar.csts |= NVME_CSTS_READY → CSTS.RDY=1 설정

2. I/O Queue 생성 (Admin 명령 경로)
   - nvme_create_sq() → Create I/O Submission Queue 명령 처리
   - nvme_create_cq() → Create I/O Completion Queue 명령 처리

Week 33: QEMU NVMe 자료구조
──────────────────────────
1. include/hw/nvme/nvme.h 또는 ctrl.c 내부 구조체
   - struct NvmeCtrl: 컨트롤러 전체 상태
     → .bar: 레지스터 값들
     → .sq[]: Submission Queue 배열
     → .cq[]: Completion Queue 배열
     → .namespaces: Namespace 리스트

2. struct NvmeSQueue: Submission Queue
   → .dma_addr: 호스트 메모리의 SQ 주소
   → .size: 큐 엔트리 수
   → .head, .tail: 큐 포인터
   → .cqid: 연결된 CQ 번호
   → .timer: polling 타이머

3. struct NvmeCQueue: Completion Queue
   → .dma_addr: 호스트 메모리의 CQ 주소
   → .head, .tail: 큐 포인터
   → .irq_enabled, .vector: 인터럽트 설정

실습 과제:
 □ QEMU 소스에 printf 추가하여 MMIO read/write 로그 출력
 □ 커널의 writel(doorbell)과 QEMU의 nvme_mmio_write() 대응 확인
 □ Admin Queue 생성 시 QEMU가 받는 주소와 커널이 보낸 DMA 주소 비교
 □ QEMU NVMe 프로퍼티(mqes, mdts 등) 변경 → 커널 동작 변화 관찰
 □ QEMU GDB 디버깅: nvme_mmio_write에 breakpoint
```

### Chapter 11: QEMU NVMe I/O 처리 — Doorbell에서 데이터 전송까지 (Week 34~37)

**목표**: 커널이 Doorbell을 울린 후, QEMU가 SQ에서 명령을 읽고, 데이터를 전송하고, CQ에 완료를 기록하고, MSI-X 인터럽트를 발생시키는 전체 과정을 라인바이라인으로 분석한다.

**역할**: I/O flow에서 "하드웨어 내부"에 해당하는 구간. 실제 NVMe SSD에서는 펌웨어로 동작하지만, QEMU에서는 C 코드로 구현되어 있어 한 줄씩 따라갈 수 있다. 커스텀 NVMe 명령, 에러 주입, 성능 시뮬레이션 등을 위한 QEMU 수정의 기반이 된다.

**학습 파일 및 라인바이라인 대상**:

```
Week 34: Doorbell 처리와 SQ 명령 읽기
────────────────────────────────────
1. hw/nvme/ctrl.c: nvme_process_db()
   라인바이라인:
   → offset에서 SQ/CQ 번호와 방향(SQ doorbell vs CQ doorbell) 추출
   → SQ doorbell인 경우:
     → new_tail = val (커널이 쓴 SQ tail 값)
     → sq->tail = new_tail
     → timer_mod(sq->timer, ...) → SQ 처리 타이머 스케줄

2. SQ 처리 타이머 → nvme_process_sq()
   라인바이라인:
   → while (sq->head != sq->tail) {
       // SQ에서 NVMe 명령 읽기 (호스트 메모리 → QEMU)
       nvme_addr_read(n, sq->dma_addr + sq->head * NVME_SQ_ENTRY_SIZE,
                      &cmd, sizeof(cmd))
       // ↑ DMA Read: 호스트 메모리에서 64B 명령을 읽어옴

       status = nvme_io_cmd(n, &cmd, req)
       // ↑ 명령 종류에 따라 처리

       sq->head = (sq->head + 1) % sq->size
     }

3. nvme_addr_read()
   → pci_dma_read(PCI_DEVICE(n), addr, buf, size)
   → QEMU의 AddressSpace에서 호스트 메모리 읽기

Week 35: NVMe 명령 처리 (Read/Write)
───────────────────────────────────
1. hw/nvme/ctrl.c: nvme_io_cmd()
   라인바이라인:
   → switch(cmd->opcode):
     NVME_CMD_READ:  nvme_rw(n, req)
     NVME_CMD_WRITE: nvme_rw(n, req)
     NVME_CMD_FLUSH: nvme_flush(n, req)
     NVME_CMD_DSM:   nvme_dsm(n, req)

2. nvme_rw() — 핵심 I/O 처리 함수
   라인바이라인:
   → slba = le64_to_cpu(rw->slba): 시작 LBA
   → nlb = le16_to_cpu(rw->length) + 1: LBA 개수
   → data_offset = nvme_l2b(ns, slba): LBA → 바이트 오프셋
   → data_size = nvme_l2b(ns, nlb): 데이터 크기

   → 데이터 전송 (PRP/SGL 기반):
     nvme_map_prp(): PRP 리스트에서 호스트 메모리 주소 추출
     → QEMUSGList 구성 (scatter-gather)

   → 실제 I/O 수행:
     dma_blk_read() 또는 dma_blk_write()
     → QEMU 블록 백엔드(qcow2/raw 파일)에 I/O 요청

3. nvme_map_prp() 상세
   라인바이라인:
   → prp1 = le64_to_cpu(cmd->dptr.prp1)
   → prp2 = le64_to_cpu(cmd->dptr.prp2)
   → 4KB 이하: prp1만 사용
   → 4KB~8KB: prp1 + prp2
   → 8KB 초과: prp2가 PRP List → 리스트에서 주소들 읽기
   → qemu_sglist_add(): SG 리스트에 추가

Week 36: QEMU 블록 백엔드와 실제 데이터 I/O
──────────────────────────────────────────
1. dma_blk_write() → blk_aio_pwritev()
   → QEMU의 block layer (block/block-backend.c)
   → coroutine 기반 비동기 I/O
   → 최종: host OS의 pwritev() syscall로 qcow2/raw 파일에 쓰기

2. QEMU block driver stack
   → BlockDriverState chain:
     throttle → qcow2/raw format → file-posix
   → file-posix.c: raw_co_pwritev()
   → pwritev(fd, ...) → 호스트 커널의 I/O 경로

3. I/O 완료 콜백
   → blk_aio_complete() → nvme_rw_cb()
   → nvme_enqueue_req_completion()

Week 37: CQ 기록과 MSI-X 인터럽트 발생
─────────────────────────────────────
1. hw/nvme/ctrl.c: nvme_enqueue_req_completion()
   라인바이라인:
   → STAILQ_INSERT_TAIL(&cq->req_list, req, entry)
   → timer_mod(cq->timer, ...) → CQ 처리 타이머 스케줄

2. nvme_post_cqes() — CQ에 완료 엔트리 기록
   라인바이라인:
   → while (!STAILQ_EMPTY(&cq->req_list) && cq has space) {
       cqe.result = req->cqe.result
       cqe.sq_head = sq->head
       cqe.sq_id = sq->sqid
       cqe.cid = req->cqe.cid     // ↑ command_id (= tag)
       cqe.status = req->status

       // Phase bit 토글
       cqe.status |= (cq->phase & 1) << 16

       // CQ에 기록 (QEMU → 호스트 메모리)
       nvme_addr_write(n, cq->dma_addr + cq->tail * NVME_CQ_ENTRY_SIZE,
                       &cqe, sizeof(cqe))
       // ↑ DMA Write: 호스트 메모리의 CQ에 16B CQE 기록

       cq->tail = (cq->tail + 1) % cq->size
       if (cq->tail == 0) cq->phase ^= 1  // phase bit 토글
     }

3. MSI-X 인터럽트 발생
   → nvme_irq_assert() → msix_notify()
   → QEMU의 MSI-X 에뮬레이션
   → 커널의 nvme_irq() 호출됨 (Chapter 9로 연결)

4. 전체 QEMU 내부 I/O 흐름 다이어그램

   커널 writel(doorbell)
         │
         ▼
   nvme_process_db()         ← MMIO trap
         │
         ▼
   nvme_process_sq()         ← SQ에서 명령 읽기
         │
         ▼
   nvme_io_cmd()
         │
         ▼
   nvme_rw()                 ← LBA/길이 파싱
         │
         ▼
   nvme_map_prp()            ← 호스트 메모리 주소 추출
         │
         ▼
   dma_blk_read/write()      ← QEMU 블록 백엔드 I/O
         │
         ▼
   nvme_rw_cb()              ← I/O 완료 콜백
         │
         ▼
   nvme_enqueue_req_completion()
         │
         ▼
   nvme_post_cqes()          ← CQ에 완료 기록
         │
         ▼
   msix_notify()             ← MSI-X 인터럽트
         │
         ▼
   커널 nvme_irq()           → Chapter 9로

실습 과제:
 □ QEMU nvme_process_sq()에 printf → 매 명령의 opcode, slba 출력
 □ nvme_rw()에서 PRP 주소와 커널의 DMA 매핑 주소 비교
 □ CQE의 command_id와 커널이 설정한 값 일치 확인
 □ msix_notify() 호출 시점 확인
 □ QEMU에 의도적 에러 주입 (status=NVME_SC_INTERNAL) → 커널 반응
 □ QEMU I/O latency에 인위적 지연 추가 → fio latency 변화
```

---

## Phase 5: 특수 I/O 경로와 고급 주제 (Month 9~10)

### Chapter 12: io_uring — 고성능 비동기 I/O (Week 38~40)

**목표**: io_uring의 커널 내부 구현을 라인바이라인으로 분석한다. SQ/CQ 공유 메모리 구조, submission 경로, completion 경로, 그리고 NVMe 드라이버와의 연결을 상세히 이해한다.

**역할**: io_uring은 현대 리눅스에서 가장 빠른 I/O 인터페이스다. 기존의 read/write, aio 대비 syscall overhead를 획기적으로 줄인다. fio의 io_uring 엔진이 실제로 어떻게 동작하는지 이해하면, 성능 측정과 최적화의 깊이가 달라진다.

**학습 파일 및 라인바이라인 대상**:

```
Week 38: io_uring 초기화와 공유 메모리
────────────────────────────────────
1. io_uring/io_uring.c: io_uring_setup()
   → io_uring_create(): io_ring_ctx 할당
   → io_allocate_scq_urings(): SQ/CQ 링 버퍼 할당
     → 사용자↔커널 공유 메모리 (mmap 가능)
   → io_sq_offsets() / io_cq_offsets(): 오프셋 정보 반환

2. 공유 메모리 구조
   - SQ Ring: head, tail, array[] (SQE 인덱스 배열)
   - CQ Ring: head, tail, cqes[] (CQE 배열)
   - SQE 배열: 별도 영역 (유저가 직접 쓰기)
   → mmap(fd, IORING_OFF_SQ_RING, ...) → SQ ring 매핑
   → mmap(fd, IORING_OFF_SQES, ...) → SQE 배열 매핑
   → mmap(fd, IORING_OFF_CQ_RING, ...) → CQ ring 매핑

Week 39: io_uring 제출 경로
──────────────────────────
1. io_uring/io_uring.c: io_uring_enter()
   → io_submit_sqes(): SQ에서 SQE 읽기
   → io_get_sqe(): SQ ring에서 다음 SQE 추출
   → io_submit_sqe(): SQE 타입별 처리
     → IORING_OP_READ/WRITE: io_read/io_write
     → IORING_OP_READV/WRITEV: io_readv/io_writev

2. io_read() / io_write() → kiocb 설정
   → io_prep_rw(): kiocb 초기화
   → vfs_iocb_iter_read/write()
   → file->f_op->read_iter/write_iter()
   → 이후 VFS → 블록 레이어 → NVMe (기존 경로와 합류)

3. batched submission
   → 여러 SQE를 한 번의 io_uring_enter()로 제출
   → plug/unplug batching과의 시너지

Week 40: io_uring 완료와 polling
───────────────────────────────
1. completion 경로
   - io_complete_rw() → __io_complete_rw()
   → io_req_complete(): CQE를 CQ ring에 기록
   → io_cqring_ev_posted(): eventfd 통지

2. IOPOLL 모드
   - IORING_SETUP_IOPOLL 플래그
   → io_uring_enter(IORING_ENTER_GETEVENTS)
   → io_do_iopoll() → blk_mq_poll()
   → NVMe의 nvme_poll() 호출 (인터럽트 없이 CQ 직접 확인)

3. SQPOLL 모드
   - IORING_SETUP_SQPOLL 플래그
   → 커널 스레드(io_sq_thread)가 SQ를 자동 polling
   → 유저는 SQE만 쓰면 됨 (syscall 필요 없음!)
   → io_sq_thread() 함수 라인바이라인

실습 과제:
 □ 간단한 io_uring 프로그램 작성: 4KB direct write → NVMe
 □ GDB로 io_uring_enter → io_submit_sqes → NVMe 경로 추적
 □ CQ ring의 실제 메모리 내용 확인 (GDB로 mmap 주소 읽기)
 □ IOPOLL vs 인터럽트 모드 latency 비교
 □ SQPOLL 모드에서 io_sq_thread의 CPU 사용률 확인
```

### Chapter 13: Buffered I/O와 Page Cache Writeback (Week 41~43)

**목표**: Direct I/O가 아닌 Buffered I/O 경로에서 데이터가 page cache에 저장되고, writeback daemon에 의해 비동기적으로 디스크에 기록되는 전체 과정을 분석한다.

**역할**: 실제 application의 대부분은 Buffered I/O를 사용한다. "write() 호출 후 즉시 리턴되는데, 데이터는 언제 디스크에 쓰이는가?"라는 근본적인 질문에 답한다. fsync()의 동작 원리와 데이터 안전성(durability) 이해의 기반이 된다.

**학습 파일 및 라인바이라인 대상**:

```
Week 41: Page Cache 쓰기
───────────────────────
1. mm/filemap.c: generic_perform_write()
   → pagecache_get_page(): page cache에서 페이지 찾기/할당
   → a_ops->write_begin(): 페이지 준비 (ext4: ext4_write_begin)
   → copy_page_from_iter(): 사용자 버퍼 → 페이지 복사
   → a_ops->write_end(): 완료 처리 → set_page_dirty()

2. Dirty 페이지 관리
   - include/linux/page-flags.h: PG_dirty 플래그
   - mm/page-writeback.c: __set_page_dirty_buffers()
   → radix tree에 dirty 태그 설정
   → balance_dirty_pages(): dirty 비율 제한

Week 42: Writeback 메커니즘
─────────────────────────
1. fs/fs-writeback.c: wb_workfn()
   → wb_do_writeback() → wb_writeback()
   → writeback_sb_inodes() → __writeback_single_inode()
   → do_writepages() → a_ops->writepages()

2. ext4 writepages
   - fs/ext4/inode.c: ext4_writepages()
   → mpage_prepare_extent_to_map()
   → mpage_map_and_submit_extent()
   → ext4_bio_write_folio() → submit_bio()
   → 여기서 블록 레이어 경로(Chapter 5)와 합류!

3. Writeback 트리거 조건
   - 주기적: dirty_writeback_centisecs (기본 5초)
   - dirty 비율: dirty_ratio / dirty_background_ratio
   - sync/fsync 호출 시

Week 43: fsync와 데이터 안전성
─────────────────────────────
1. fs/sync.c: vfs_fsync() → vfs_fsync_range()
   → file->f_op->fsync() = ext4_sync_file()

2. fs/ext4/fsync.c: ext4_sync_file()
   → filemap_write_and_wait_range(): dirty 페이지 쓰기 + 완료 대기
   → jbd2_complete_transaction(): 저널 커밋
   → blkdev_issue_flush(): NVMe flush 명령

3. NVMe flush 처리
   - nvme_setup_cmd()에서 REQ_OP_FLUSH → nvme_setup_flush()
   → NVMe Flush 명령 전송
   → QEMU: nvme_flush() → blk_aio_flush()

실습 과제:
 □ buffered write 후 /proc/meminfo의 Dirty 값 변화 관찰
 □ dirty_expire_centisecs 조정 → writeback 타이밍 변화 확인
 □ fsync() 호출 시 NVMe flush 명령까지의 경로 ftrace
 □ GDB로 writeback daemon(kworker)에서 submit_bio까지 추적
```

---

## Phase 6: 커널 소스 수정 실습 (Month 10~12)

### Chapter 14: 첫 번째 커널 수정 — I/O 통계 수집 모듈 (Week 44~47)

**목표**: 블록 레이어 또는 NVMe 드라이버에 코드를 추가하여, I/O 요청별로 latency, LBA 범위, 큐 깊이 등의 통계를 수집하고 debugfs를 통해 노출하는 기능을 구현한다.

**역할**: 지금까지 배운 I/O 경로 지식을 실제 코드 수정으로 증명하는 첫 단계. "코드를 읽을 줄 아는 것"과 "코드를 고칠 줄 아는 것"은 다르다. 비교적 안전한 통계 수집부터 시작하여 커널 수정의 전체 워크플로(수정 → 빌드 → QEMU 부팅 → 검증)를 체화한다.

```
Week 44: debugfs 인터페이스 설계
──────────────────────────────
1. debugfs 기초
   - fs/debugfs/inode.c: debugfs_create_file()
   - include/linux/debugfs.h: DEFINE_SHOW_ATTRIBUTE()
   - 기존 예시 분석: drivers/nvme/host/pci.c의 nvme_dev_attrs

2. 데이터 구조 설계
   - per-queue 통계: 제출 수, 완료 수, 에러 수
   - per-I/O 기록: timestamp, LBA, length, latency
   - ring buffer 방식으로 최근 N개 I/O 기록

Week 45: 제출 경로에 계측 코드 추가
─────────────────────────────────
1. nvme_queue_rq() 수정
   → 제출 시점 타임스탬프 기록: ktime_get_ns()
   → LBA, length, opcode 기록
   → per-queue 카운터 증가

2. nvme_iod에 계측 필드 추가
   → struct nvme_iod { ...; u64 submit_time; ... }
   → blk_mq_tag_set의 cmd_size 조정

Week 46: 완료 경로에 계측 코드 추가
─────────────────────────────────
1. nvme_pci_complete_rq() 수정
   → 완료 시점 타임스탬프 기록
   → latency = complete_time - submit_time
   → ring buffer에 기록

2. debugfs 파일로 노출
   → /sys/kernel/debug/nvme_io_stats/queue0/latency_histogram
   → /sys/kernel/debug/nvme_io_stats/queue0/recent_ios

Week 47: 테스트와 검증
────────────────────
1. QEMU에서 수정된 커널 부팅
2. fio 부하 → debugfs 통계 확인
3. 기존 NVMe 기능에 regression 없는지 확인
4. latency 분포 그래프 생성 (gnuplot)

실습 과제:
 □ 커널 수정 → 빌드 → QEMU 부팅 사이클 완성
 □ debugfs 파일에서 I/O 통계 읽기
 □ fio random read/write 시 latency 분포 확인
 □ 큐 깊이별 성능 변화 관찰
```

### Chapter 15: 두 번째 커널 수정 — I/O 스케줄링 정책 변경 (Week 48~50)

**목표**: 블록 레이어에 간단한 커스텀 I/O 스케줄러를 만들거나, 기존 스케줄러를 수정하여 특정 정책(예: 특정 LBA 범위 우선처리)을 구현한다.

**역할**: I/O 경로의 "정책 결정 지점"을 직접 건드리는 단계. 단순 계측(Chapter 14)을 넘어서, I/O의 순서와 우선순위를 변경하는 능력을 갖춘다. 학술 연구(논문)에서 흔히 나오는 "커스텀 스케줄러"를 직접 구현해볼 수 있다.

```
Week 48: 커스텀 I/O 스케줄러 프레임워크
─────────────────────────────────────
1. 기존 스케줄러 구조 분석 (복습)
   - include/linux/elevator.h: struct elevator_mq_ops
   → .insert_requests, .dispatch_request, .has_work
   → .init_sched, .exit_sched

2. 최소 스케줄러 skeleton 작성
   - block/my-iosched.c 생성
   → FIFO 스케줄러 (가장 단순한 형태)
   → Kconfig, Makefile 수정

Week 49: 정책 구현
────────────────
1. LBA 범위 기반 우선순위
   → 특정 LBA 범위(예: 메타데이터 영역)의 I/O를 우선 dispatch
   → 두 개의 큐: high-priority, normal

2. 또는: Read 우선 정책
   → Read request를 Write보다 먼저 dispatch
   → starvation 방지를 위한 aging 메커니즘

Week 50: 성능 측정과 비교
───────────────────────
1. 커스텀 스케줄러 vs none vs mq-deadline
   → fio mixed workload (70% read, 30% write)
   → latency, throughput, tail latency 비교

2. 결과 분석과 문서화
   → 왜 NVMe에서는 none이 대부분 최적인지 실험적 확인
   → 스케줄러가 의미있는 시나리오는 무엇인지

실습 과제:
 □ 커스텀 스케줄러 커널 모듈 빌드 & 로드
 □ echo "my-sched" > /sys/block/nvme0n1/queue/scheduler
 □ fio 벤치마크 비교 (none vs my-sched vs mq-deadline)
 □ blktrace로 I/O 순서 변화 확인
```

### Chapter 16: 세 번째 커널 수정 — QEMU NVMe 디바이스 확장 (Week 51~52)

**목표**: QEMU NVMe 디바이스에 커스텀 기능을 추가한다. 예를 들어 커스텀 Vendor Specific 명령, I/O latency 시뮬레이션, 또는 에러 주입 기능을 구현한다.

**역할**: 커널 드라이버 수정 + 디바이스 에뮬레이터 수정을 모두 경험하여 "양쪽"을 다 고칠 수 있는 능력을 갖춘다. NVMe 스펙에 정의된 Vendor Specific 명령을 통해 커널↔디바이스 간 커스텀 프로토콜을 설계하는 경험을 한다.

```
Week 51: QEMU NVMe 수정
──────────────────────
1. Vendor Specific Admin 명령 추가
   - hw/nvme/ctrl.c: nvme_admin_cmd() 수정
   → opcode 0xC0에 커스텀 핸들러 추가
   → 예: 내부 통계 리턴 (처리한 명령 수, 평균 latency 등)

2. I/O Latency 시뮬레이션
   - nvme_rw()에 configurable delay 추가
   → 실제 SSD의 read/write latency 차이 시뮬레이션
   → timer_mod()로 지연 후 completion

3. 에러 주입
   - 특정 LBA 범위에서 에러 반환
   → cqe.status = NVME_SC_UNRECOVERED_READ_ERROR

Week 52: 커널 드라이버와 연동
───────────────────────────
1. 커널 측 Vendor Specific 명령 전송
   - drivers/nvme/host/core.c에 커스텀 ioctl 추가
   → 또는 nvme-cli passthrough 사용

2. 에러 주입 시 커널 반응 분석
   → nvme_complete_rq()의 에러 처리 경로
   → retry 로직, I/O 에러 리포팅

3. 전체 통합 테스트
   → 커스텀 커널 + 커스텀 QEMU + fio 부하
   → 모든 수정 사항이 조화롭게 동작하는지 확인

실습 과제:
 □ QEMU에 Vendor Specific 명령 핸들러 추가
 □ 커널에서 해당 명령 전송 → 응답 확인
 □ latency 시뮬레이션: read=100μs, write=50μs → fio 결과 비교
 □ 에러 주입 → 커널 에러 핸들링 경로 추적
 □ 1년간의 학습 내용을 종합한 end-to-end 분석 문서 작성
```

---

## 월별 요약표

| 월 | Phase | Chapter | 핵심 내용 | 주요 소스 파일 |
|:---:|:---:|:---:|:---|:---|
| 1 | 기반 | Ch1 | QEMU+GDB 환경 구축, ftrace | Makefile, QEMU 옵션 |
| 2 | 기반 | Ch2~3 | syscall 진입, VFS, fd→file→f_op | entry_64.S, read_write.c, block_dev.c |
| 3 | Block | Ch4 | bio 구조체, 할당, 완료 | bio.c, blk_types.h |
| 4 | Block | Ch5 전반 | blk-mq 아키텍처, request 할당 | blk-mq.c, blk-mq.h |
| 5 | Block | Ch5 후반~Ch6 | dispatch, sbitmap, completion | blk-mq.c, blk-merge.c, sbitmap.c |
| 6 | NVMe | Ch7 | NVMe probe, 초기화, queue 생성 | pci.c, core.c |
| 7 | NVMe | Ch8 | nvme_queue_rq(), DMA, doorbell | pci.c, core.c, nvme.h |
| 8 | NVMe | Ch9 | CQ 처리, interrupt, polling | pci.c, blk-mq.c |
| 9 | QEMU | Ch10~11 전반 | QEMU NVMe 컨트롤러, MMIO | hw/nvme/ctrl.c |
| 10 | QEMU+고급 | Ch11 후반~Ch12 | QEMU I/O 처리, io_uring | hw/nvme/ctrl.c, io_uring.c |
| 11 | 고급+수정 | Ch13~Ch14 | Page cache writeback, 커널 수정1 | fs-writeback.c, 수정 코드 |
| 12 | 수정 | Ch15~Ch16 | 커스텀 스케줄러, QEMU 확장 | my-iosched.c, ctrl.c 수정 |

---

## 주당 학습 권장 시간

| 활동 | 시간/주 |
|:---|:---:|
| 소스코드 리딩 (라인바이라인) | 6~8시간 |
| GDB/ftrace 실습 | 3~4시간 |
| ASCII 다이어그램 & 노트 정리 | 2~3시간 |
| 커널 수정 & 실험 (Phase 6) | 4~6시간 |
| **합계** | **~15시간/주** |

---

## 각 Phase의 전제 조건과 연결 관계

```
Phase 1 (기반)
  │  환경 없으면 실습 불가
  ▼
Phase 2 (Block Layer)
  │  bio/request 모르면 NVMe 이해 불가
  ▼
Phase 3 (NVMe Driver)
  │  커널 드라이버 모르면 QEMU 대응 이해 불가
  ▼
Phase 4 (QEMU NVMe)
  │  디바이스 에뮬레이션 이해 → 양쪽 전체 파악
  ▼
Phase 5 (고급 주제)
  │  io_uring, writeback → 실전 I/O 패턴 이해
  ▼
Phase 6 (커널 수정)
     모든 지식을 통합하여 직접 코드 작성
```

---

## 학습 완료 후 달성 능력

1. **코드 레벨 이해**: Application의 write() 한 번이 커널 내부에서 어떤 함수를 몇 번 호출하는지 설명할 수 있다
2. **디버깅**: I/O 성능 문제가 VFS/Block/NVMe/디바이스 중 어디서 발생하는지 ftrace/GDB로 특정할 수 있다
3. **커널 수정**: 블록 레이어에 커스텀 스케줄러를 추가하거나, NVMe 드라이버에 계측 코드를 넣을 수 있다
4. **디바이스 에뮬레이션**: QEMU NVMe 디바이스에 새로운 기능을 추가할 수 있다
5. **성능 분석**: io_uring polling vs interrupt, Direct I/O vs Buffered I/O 등의 성능 차이를 코드 레벨에서 설명할 수 있다
6. **SPDK 비교**: 커널 NVMe 드라이버와 SPDK의 구조적 차이를 코드로 비교 설명할 수 있다
