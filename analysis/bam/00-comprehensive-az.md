# BaM (Big Accelerator Memory) A-Z 종합 코드 분석

## 목차

1. [프로젝트 개요 및 아키텍처](#1-프로젝트-개요-및-아키텍처)
2. [핵심 타입 시스템 (nvm_types.h)](#2-핵심-타입-시스템)
3. [커널 모듈 (module/)](#3-커널-모듈)
4. [컨트롤러 관리 (ctrl.cpp)](#4-컨트롤러-관리)
5. [NVMe 레지스터 접근 (regs.h)](#5-nvme-레지스터-접근)
6. [Admin 명령 시스템 (admin.cpp)](#6-admin-명령-시스템)
7. [큐 관리 (queue.cpp)](#7-큐-관리)
8. [DMA 메모리 매핑 (dma.cpp)](#8-dma-메모리-매핑)
9. [RPC 메커니즘 (rpc.cpp)](#9-rpc-메커니즘)
10. [GPU Lock-free 병렬 큐 (nvm_parallel_queue.h)](#10-gpu-lock-free-병렬-큐)
11. [GPU 페이지 캐시 (page_cache.h)](#11-gpu-페이지-캐시)
12. [C++ 래퍼 계층 (ctrl.h, queue.h, buffer.h)](#12-c-래퍼-계층)
13. [Smart Pointer 시스템 (bafs_ptr.h)](#13-smart-pointer-시스템)
14. [플랫폼 추상화 (linux/, dis/)](#14-플랫폼-추상화)
15. [벤치마크 카탈로그](#15-벤치마크-카탈로그)
16. [전체 데이터 흐름 추적](#16-전체-데이터-흐름-추적)
17. [빌드 시스템](#17-빌드-시스템)

---

## 1. 프로젝트 개요 및 아키텍처

BaM은 GPU가 CPU 개입 없이 PCIe P2P를 통해 NVMe SSD에 직접 I/O 명령을 발행하는 시스템이다. ASPLOS'23 논문의 구현체로, Linux 스토리지 스택 전체를 우회한다.

### 전체 아키텍처 계층도

```
┌──────────────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER (benchmarks/)                    │
│         BFS, CC, SSSP, PageRank, Block I/O, Pattern, etc.            │
│                                                                      │
│  ┌─ bafs_ptr<T> ─┐  ┌─ bam_ptr<T> ─┐  ┌─ bam_ptr_tlb<T> ─┐       │
│  │ Smart Pointer  │  │  Direct Ptr   │  │  TLB-cached Ptr  │       │
│  └────────┬───────┘  └───────┬───────┘  └────────┬─────────┘       │
│           └──────────────────┼───────────────────┘                   │
│                              ↓                                       │
├──────────────────────────────────────────────────────────────────────┤
│           array_d_t<T> / range_d_t<T> (논리적 배열 추상화)           │
├──────────────────────────────────────────────────────────────────────┤
│              page_cache_d_t (GPU 소프트웨어 페이지 캐시)              │
│         ┌─────────────┬──────────────┬────────────────┐              │
│         │ data_page_t │ cache_page_t │ Clock 교체     │              │
│         │ (페이지상태) │ (슬롯메타)   │ (find_slot)    │              │
│         └─────────────┴──────────────┴────────────────┘              │
├──────────────────────────────────────────────────────────────────────┤
│           nvm_parallel_queue.h (Lock-free GPU 큐 프리미티브)          │
│         ┌──────────┬──────────┬───────────┬──────────────┐           │
│         │ sq_enqueue│ cq_poll  │ cq_dequeue│ get/put_cid  │           │
│         │ (8단계)   │ (탐색)   │ (헤드이동) │ (ID관리)     │           │
│         └──────────┴──────────┴───────────┴──────────────┘           │
├──────────────────────────────────────────────────────────────────────┤
│          C++ Wrapper Layer (include/ctrl.h, queue.h, buffer.h)       │
│         Controller │ QueuePair │ DmaPtr │ BufferPtr │ Event          │
├──────────────────────────────────────────────────────────────────────┤
│          Core NVM Library (src/ + include/)                          │
│   ctrl.cpp │ admin.cpp │ queue.cpp │ dma.cpp │ rpc.cpp │ error.cpp  │
│   nvm_queue.h │ nvm_cmd.h │ nvm_dma.h │ nvm_admin.h │ nvm_ctrl.h   │
├──────────────────────────────────────────────────────────────────────┤
│         Platform Abstraction                                         │
│   ┌─ Linux (src/linux/) ─┐    ┌─ DIS/SmartIO (src/dis/) ─┐         │
│   │ device.cpp (ioctl)   │    │ device.c (SISCI)          │         │
│   │ dma.cpp (host/GPU)   │    │ dma.c (segment)           │         │
│   │ ioctl.h (커널↔유저)  │    │ rpc.c (클러스터 RPC)      │         │
│   └──────────────────────┘    │ interrupt.c (DI 인터럽트) │         │
│                                └──────────────────────────┘         │
├──────────────────────────────────────────────────────────────────────┤
│         Linux Kernel Module (module/)                                │
│   pci.c (PCI 드라이버) │ ctrl.c (chardev) │ map.c (DMA매핑)        │
│   list.c (연결리스트)                                                │
│   → /dev/libnvm0, /dev/libnvm1, ...                                 │
├──────────────────────────────────────────────────────────────────────┤
│                          HARDWARE                                    │
│   GPU ←────── PCIe P2P DMA ──────→ NVMe SSD                        │
│   (도어벨 MMIO 쓰기, CQ 폴링)      (SQ/CQ, BAR0 레지스터)          │
└──────────────────────────────────────────────────────────────────────┘
```

### 소스 파일 전체 맵 (역할별)

| 파일 | 역할 | 유형 |
|------|------|------|
| **include/nvm_types.h** | 핵심 타입 (nvm_ctrl_t, nvm_queue_t, nvm_dma_t, nvm_cmd_t, nvm_cpl_t) | 타입 정의 |
| **include/nvm_ctrl.h** | 컨트롤러 초기화/해제 API | API |
| **include/nvm_cmd.h** | NVMe 명령 빌더 (헤더, PRP, LBA) | API |
| **include/nvm_queue.h** | CPU 큐 조작 (enqueue, dequeue, submit) | API |
| **include/nvm_parallel_queue.h** | GPU Lock-free 큐 (8단계 알고리즘) | API (핵심) |
| **include/nvm_dma.h** | DMA 메모리 매핑 API | API |
| **include/nvm_admin.h** | Admin 명령 (큐 생성, Identify) | API |
| **include/nvm_aq.h** | Admin 큐 생성/파괴 | API |
| **include/nvm_rpc.h** | RPC 바인딩 API | API |
| **include/nvm_error.h** | NVMe 에러 코드 파싱 매크로 | 유틸 |
| **include/nvm_util.h** | 비트 조작, 레지스터 접근, GPU 헬퍼 | 유틸 |
| **include/page_cache.h** | GPU 페이지 캐시 (2143줄, 최핵심) | 코어 |
| **include/ctrl.h** | Controller C++ 클래스 | 래퍼 |
| **include/queue.h** | QueuePair C++ 클래스 | 래퍼 |
| **include/buffer.h** | DMA/GPU 메모리 할당 헬퍼 | 래퍼 |
| **include/bafs_ptr.h** | 투명 SSD 배열 Smart Pointer | 래퍼 |
| **include/event.h** | CUDA 이벤트 타이밍 래퍼 | 유틸 |
| **include/host_util.h** | CUDA 빌트인 CPU 폴백 | 유틸 |
| **include/util.h** | cuda_err_chk, warp_memcpy | 유틸 |
| **src/ctrl.cpp** | 컨트롤러 핸들 관리 (레지스터, 리셋) | 구현 |
| **src/admin.cpp** | Admin 명령 구성 및 실행 | 구현 |
| **src/queue.cpp** | 큐 초기화/리셋/블로킹 폴링 | 구현 |
| **src/dma.cpp** | DMA 매핑 레퍼런스 카운팅 | 구현 |
| **src/rpc.cpp** | Admin 큐 RPC (로컬/원격) | 구현 |
| **src/error.cpp** | NVMe 에러 코드 → 문자열 변환 | 구현 |
| **src/mutex.cpp** | pthread 뮤텍스 래퍼 | 구현 |
| **src/regs.h** | BAR0 레지스터 오프셋 매크로 | 내부 |
| **src/linux/device.cpp** | Linux ioctl 경로 컨트롤러 초기화 | 플랫폼 |
| **src/linux/dma.cpp** | Linux host/GPU DMA 매핑 | 플랫폼 |
| **src/linux/ioctl.h** | ioctl 명령 코드 정의 | 플랫폼 |
| **src/dis/** | SmartIO/DIS 클러스터 지원 (6파일) | 플랫폼 |
| **module/pci.c** | PCI 드라이버 등록, ioctl/mmap 핸들러 | 커널 |
| **module/ctrl.c** | 캐릭터 디바이스 관리 | 커널 |
| **module/map.c** | 유저 메모리 → DMA 주소 변환 | 커널 |
| **module/list.c** | 스핀락 보호 이중 연결 리스트 | 커널 |

---

## 2. 핵심 타입 시스템

### nvm_types.h — 전체 필드 상세

#### nvm_ctrl_t (NVMe 컨트롤러 핸들)

```c
typedef struct nvm_ctrl {
    size_t      page_size;   // 컨트롤러 메모리 페이지 크기 (CC.MPS에서 계산: 2^(12+MPS))
    uint8_t     dstrd;       // 도어벨 스트라이드 (인코딩 값; 실제 = 4 << dstrd 바이트)
    uint64_t    timeout;     // 컨트롤러 타임아웃 (밀리초, CAP.TO × 500ms)
    uint32_t    max_qs;      // 최대 큐 엔트리 수 (CAP.MQES + 1)
    size_t      mm_size;     // BAR0 메모리맵 영역 크기
    volatile void* mm_ptr;   // BAR0 메모리맵 포인터 (MMIO 레지스터 접근용)
} nvm_ctrl_t;
```

#### nvm_queue_t (NVMe 큐 — SQ/CQ 공용, GPU 확장 포함)

이것이 BaM의 핵심 데이터 구조로, CPU와 GPU 양쪽에서 사용된다.

```c
typedef struct nvm_queue {
    // === GPU Lock-free 동기화 필드 (simt::atomic) ===
    simt::atomic<uint32_t, thread_scope_device> head_lock;  // CQ 헤드 리더 선출
    simt::atomic<uint32_t, thread_scope_device> tail_lock;  // SQ 꼬리 리더 선출
    simt::atomic<uint32_t, thread_scope_device> head;       // 현재 헤드 (device scope)
    simt::atomic<uint32_t, thread_scope_device> tail;       // 현재 꼬리 (device scope)
    simt::atomic<uint32_t, thread_scope_system> tail_copy;  // 꼬리 사본 (system scope)
    simt::atomic<uint32_t, thread_scope_system> head_copy;  // 헤드 사본 (system scope)
    simt::atomic<uint32_t, thread_scope_device> in_ticket;  // 전역 순서 티켓 (SQ 진입)
    simt::atomic<uint32_t, thread_scope_device> cid_ticket; // CID 할당 티켓

    // === GPU 동기화 배열 포인터 ===
    padded_struct* tickets;     // 슬롯별 티켓 (Lamport 빵집 알고리즘 변형)
    padded_struct* head_mark;   // 슬롯별 헤드 마크 (LOCKED/UNLOCKED)
    padded_struct* tail_mark;   // 슬롯별 꼬리 마크 (LOCKED/UNLOCKED)
    padded_struct* cid;         // CID 배열 (각 CID의 사용/비사용 상태)
    padded_struct* pos_locks;   // CQ 위치 잠금 (슬롯 재사용 안전)
    uint16_t* clean_cid;        // 재사용 가능 CID 풀

    // === 최적화 필드 ===
    uint32_t qs_minus_1;  // qs - 1 (모듈러 대신 AND 연산용)
    uint32_t qs_log2;     // log2(qs) (나눗셈 대신 시프트 연산용)

    // === NVMe 스펙 필드 ===
    uint16_t no;              // 큐 번호 (SQ/CQ 쌍 고유 ID)
    uint16_t es;              // 엔트리 크기 (SQ=64B, CQ=16B)
    uint32_t qs;              // 큐 크기 (엔트리 수)
    int8_t   phase;           // Phase 태그 (0 또는 1, CQ 완료 감지용)
    volatile uint32_t* db;    // 도어벨 레지스터 포인터 (쓰기 전용 MMIO)
    volatile void*     vaddr; // 큐 메모리 가상 주소
    uint64_t           ioaddr;// 큐 메모리 물리/IO 주소
} nvm_queue_t;
```

**padded_struct**: 32바이트 정렬된 `simt::atomic<uint32_t, thread_scope_device>` 래퍼로, GPU L2 캐시라인(128B) 내에 4개가 들어가 false sharing을 최소화한다.

#### nvm_dma_t (DMA 매핑 디스크립터)

```c
typedef struct nvm_dma {
    void*    vaddr;       // 가상 주소 (GPU 메모리일 경우 NULL 가능)
    int8_t   local;       // 로컬 메모리 여부 (1=호스트, 0=원격/GPU)
    int8_t   contiguous;  // 물리적 연속 여부
    size_t   page_size;   // 컨트롤러 페이지 크기 (MPS)
    size_t   n_ioaddrs;   // MPS 크기 페이지 수
    uint64_t ioaddrs[];   // 물리/IO 주소 가변 배열 (NVMe PRP에 사용)
} nvm_dma_t;
```

#### nvm_cmd_t / nvm_cpl_t (NVMe 명령/완료 엔트리)

```c
// NVMe 명령 (64바이트, NVMe 스펙 1:1 매핑)
typedef struct { uint32_t dword[16]; } nvm_cmd_t;
//   dword[0]:  CID(31:16) | Opcode(7:0)
//   dword[1]:  NSID
//   dword[6-9]: PRP1, PRP2 (데이터 포인터)
//   dword[10-12]: LBA (시작 블록), NLB (블록 수)

// NVMe 완료 (16바이트)
typedef struct { volatile uint32_t dword[4]; } nvm_cpl_t;
//   dword[2]: SQHD(15:0) | SQID(31:16)
//   dword[3]: CID(31:16) | Status(15:1) | Phase(0)
```

#### nvm_ctrl_info / nvm_ns_info (컨트롤러/네임스페이스 정보)

```c
struct nvm_ctrl_info {
    uint32_t nvme_version;      // e.g., 0x00010300 = NVMe 1.3.0
    size_t   page_size;         // 메모리 페이지 크기 (MPS)
    size_t   db_stride;         // 도어벨 스트라이드
    uint64_t timeout;           // 타임아웃 (ms)
    int      contiguous;        // CQR (연속 큐 필요 여부)
    uint16_t max_entries;       // MQES (최대 큐 엔트리)
    uint8_t  pci_vendor[4];     // PCI 벤더/서브시스템 ID
    char     serial_no[20];     // 시리얼 번호 (20B, 널 종료 없음)
    char     model_no[40];      // 모델명 (40B)
    char     firmware[8];       // 펌웨어 리비전
    size_t   max_data_size;     // MDTS (최대 데이터 전송 크기)
    size_t   max_data_pages;    // MDTS를 페이지 수로
    size_t   cq_entry_size;     // CQ 엔트리 크기
    size_t   sq_entry_size;     // SQ 엔트리 크기
};

struct nvm_ns_info {
    uint32_t ns_id;             // 네임스페이스 ID
    size_t   size;              // NSZE (논리 블록 수)
    size_t   capacity;          // NCAP (용량)
    size_t   utilization;       // NUSE (사용량)
    size_t   lba_data_size;     // LBADS (보통 512 또는 4096B)
    size_t   metadata_size;     // 메타데이터 크기
};
```

---

## 3. 커널 모듈

### 역할

표준 Linux NVMe 드라이버를 대체하여, NVMe 디바이스를 유저스페이스와 GPU에 직접 노출한다.

### 핵심 구조체

#### struct ctrl (module/ctrl.h)

```c
struct ctrl {
    struct list_node list;      // ctrl_list에 연결되는 노드
    struct pci_dev*  pdev;      // 물리 PCI 디바이스 참조
    char             name[64];  // "libnvm0", "libnvm1" 등
    int              number;    // 컨트롤러 ID (0, 1, 2, ...)
    dev_t            rdev;      // 캐릭터 디바이스 번호 (major + minor)
    struct class*    cls;       // sysfs 디바이스 클래스
    struct cdev      cdev;      // 커널 cdev 구조체
    struct device*   chrdev;    // device_create() 결과
};
```

#### struct map (module/map.h)

```c
struct map {
    struct list_node list;          // host_list 또는 device_list의 노드
    struct task_struct* owner;      // 매핑을 생성한 프로세스 (보안 검증용)
    u64              vaddr;         // 매핑된 메모리 시작 가상 주소
    struct list*     ctrl_list;     // GPU 매핑용: 전체 컨트롤러 리스트 참조
    struct pci_dev*  pdev;          // 매핑 관련 PCI 디바이스
    unsigned long    page_size;     // 4KB (호스트) 또는 64KB (GPU)
    void*            data;          // 타입별 커스텀 데이터
    release          release;       // 정리 콜백 (호스트/GPU 다름)
    unsigned long    n_addrs;       // 페이지/DMA 주소 수
    uint64_t         addrs[1];      // DMA 버스 주소 가변 배열
};
```

### PCI 드라이버 등록 (module/pci.c)

```
모듈 초기화 흐름:
  libnvm_helper_entry()
    → list_init(ctrl_list, host_list, device_list)  // 3개 리스트 초기화
    → alloc_chrdev_region()                          // 동적 major 번호 할당
    → class_create()                                 // sysfs 클래스 생성
    → pci_register_driver(&driver)                   // PCI 드라이버 등록
```

**PCI 디바이스 매칭**: `PCI_DEVICE_CLASS(0x010802, 0xffffff)` — NVMe 클래스 01.08.02

### Probe 콜백 흐름 (add_pci_dev)

```
add_pci_dev(pdev)
  1. max_num_ctrls 체크 (최대 64개)
  2. ctrl_get() — ctrl 구조체 할당, ctrl_list에 삽입
  3. pci_request_region(pdev, 0) — BAR0 영역 예약
  4. pci_enable_device(pdev) — PCI 디바이스 활성화
  5. ctrl_chrdev_create() — /dev/libnvmN 캐릭터 디바이스 생성
     → cdev_init()
     → cdev_add()
     → device_create()
  6. pci_set_master(pdev) — 버스 마스터링 활성화 (DMA 필수)
  7. curr_ctrls++
```

### IOCTL 인터페이스

| 명령 | 코드 | 방향 | 용도 |
|------|------|------|------|
| `NVM_MAP_HOST_MEMORY` | `_IOW(0x80, 1, ...)` | Write | 호스트 RAM 핀닝 → DMA 주소 반환 |
| `NVM_MAP_DEVICE_MEMORY` | `_IOW(0x80, 2, ...)` | Write | GPU VRAM P2P 핀닝 → DMA 주소 반환 |
| `NVM_UNMAP_MEMORY` | `_IOW(0x80, 3, ...)` | Write | 매핑 해제 (vaddr만 필요) |

```c
struct nvm_ioctl_map {
    uint64_t  vaddr_start;  // 매핑할 가상 주소
    size_t    n_pages;      // 페이지 수
    uint64_t* ioaddrs;      // 출력: DMA 주소 배열 (유저 포인터)
};
```

### 호스트 메모리 매핑 흐름

```
ioctl(NVM_MAP_HOST_MEMORY)
  → map_userspace(ctrl, vaddr, n_pages)
    → create_descriptor(n_addrs)           // map 구조체 + 가변배열 할당
    → map_user_pages(pdev, map, vaddr)
      → get_user_pages(vaddr, n_pages)     // 물리 페이지 핀닝
      → dma_map_page(pdev, page, DMA_BIDIRECTIONAL)  // IOMMU 통과 → DMA 주소
      → map->addrs[i] = dma_addr           // DMA 주소 저장
    → list_insert(&host_list, &map->list)
  → copy_to_user(ioaddrs, map->addrs)      // DMA 주소를 유저에게 반환
```

### GPU 메모리 P2P 매핑 흐름

```
ioctl(NVM_MAP_DEVICE_MEMORY)
  → map_device_memory(ctrl, gpu_vaddr, n_pages)
    → create_descriptor(n_addrs)
    → map_gpu_memory(ctrl_list, map, gpu_vaddr, size)
      → nvidia_p2p_get_pages(gpu_vaddr, size, &page_table, callback)
         // NVIDIA 드라이버가 GPU 물리 페이지를 핀닝하고 페이지 테이블 반환
      → for each controller in ctrl_list:
          nvidia_p2p_dma_map_pages(pdev, page_table, &dma_mapping)
          // 모든 NVMe 컨트롤러에 대해 P2P DMA 매핑 생성
      → map->addrs[i] = dma_mapping->dma_addresses[i]  // 64KB 페이지 단위
    → list_insert(&device_list, &map->list)
  → copy_to_user(ioaddrs, map->addrs)
```

**핵심 설계**: GPU 메모리는 **모든** NVMe 컨트롤러에 동시에 매핑된다. 어떤 NVMe든 GPU 메모리에 DMA할 수 있어 컨트롤러 친화성 제약이 없다.

### mmap 핸들러 (BAR0 레지스터 노출)

```c
mmap_registers(file, vma)
  → ctrl = ctrl_find_by_inode(inode)
  → validate: vma_size ≤ pci_resource_len(pdev, 0)
  → pgprot_noncached(vma->vm_page_prot)   // MMIO 안전을 위해 캐시 비활성화
  → vm_iomap_memory(vma, pci_resource_start(pdev, 0), size)
  // 결과: 유저스페이스에서 NVMe BAR0 레지스터 직접 접근 가능
```

### 보안 모델

- `map->owner == current` 검증으로 프로세스 격리
- 페이지 핀닝으로 DMA 안정성 보장 (스왑 아웃 방지)
- IOMMU 통합으로 주소 변환 + 접근 제어

---

## 4. 컨트롤러 관리

### src/ctrl.cpp — 핵심 함수

#### _nvm_ctrl_init() — 컨트롤러 초기화

```
_nvm_ctrl_init(ctrl, mm_ptr, mm_size)
  1. CAP 레지스터 읽기 (BAR0 + 0x00, 64비트)
     → max_qs = CAP.MQES + 1
     → dstrd = CAP.DSTRD
     → timeout = CAP.TO × 500ms
     → mpsmin = 12 + CAP.MPSMIN
     → mpsmax = 12 + CAP.MPSMAX

  2. 호스트 페이지 크기 호환성 검증
     → mpsmin ≤ host_page_size ≤ mpsmax

  3. 핸들 초기화
     → ctrl->page_size = max(호스트 페이지, 2^mpsmin)
     → ctrl->dstrd = dstrd
     → ctrl->timeout = timeout (ms)
     → ctrl->max_qs = max_qs
     → ctrl->mm_ptr = mm_ptr
     → ctrl->mm_size = mm_size
```

#### nvm_raw_ctrl_reset() — 컨트롤러 리셋 시퀀스

```
nvm_raw_ctrl_reset(ctrl, acq_ioaddr, asq_ioaddr)
  1. CC.EN = 0 (컨트롤러 비활성화)
  2. CSTS.RDY 폴링: 1→0 대기 (타임아웃 적용)
     → _nvm_delay_remain()으로 1ms 단위 폴링
  3. AQA 레지스터 설정
     → ACQS = 큐사이즈-1 (bits 27:16)
     → ASQS = 큐사이즈-1 (bits 11:0)
  4. ACQ 주소 설정 (BAR0 + 0x30, 64비트)
  5. ASQ 주소 설정 (BAR0 + 0x28, 64비트)
  6. CC 레지스터 설정
     → EN = 1 (활성화)
     → CSS = 0 (NVM Command Set)
     → MPS = 페이지크기 계산
     → IOSQES = 6 (64바이트)
     → IOCQES = 4 (16바이트)
  7. CSTS.RDY 폴링: 0→1 대기 (타임아웃 적용)
```

#### 레퍼런스 카운팅

```c
_nvm_ctrl_get(ctrl)  → ref_count++ (뮤텍스 보호)
_nvm_ctrl_put(ctrl)  → ref_count--, count==0이면 디바이스 리소스 해제
```

---

## 5. NVMe 레지스터 접근

### src/regs.h — BAR0 오프셋 매크로

```
BAR0 레지스터 맵:
  Offset  크기  이름        접근   설명
  0x00    64b   CAP         RO     Capabilities
  0x08    32b   VER         RO     Version
  0x14    32b   CC          RW     Controller Configuration
  0x1C    32b   CSTS        RO     Controller Status
  0x24    32b   AQA         RW     Admin Queue Attributes
  0x28    64b   ASQ         RW     Admin SQ Base Address
  0x30    64b   ACQ         RW     Admin CQ Base Address
  0x1000+ 32b   SQ y DB     WO     SQ y Tail Doorbell
  0x1000+ 32b   CQ y DB     WO     CQ y Head Doorbell
```

**도어벨 주소 계산**:
```
SQ y 도어벨 = 0x1000 + (2×y) × (4 << DSTRD)
CQ y 도어벨 = 0x1000 + (2×y + 1) × (4 << DSTRD)
```

**CAP 레지스터 필드**:

| 필드 | 비트 | 의미 |
|------|------|------|
| MQES | 15:0 | 최대 큐 엔트리 (0-기반) |
| CQR | 16 | 연속 큐 필요 |
| TO | 31:24 | 타임아웃 (×500ms) |
| DSTRD | 35:32 | 도어벨 스트라이드 |
| MPSMIN | 51:48 | 최소 페이지 크기 (2^(12+val)) |
| MPSMAX | 55:52 | 최대 페이지 크기 |

---

## 6. Admin 명령 시스템

### src/admin.cpp — 주요 Admin 명령

#### Identify Controller

```
nvm_admin_ctrl_info(aq_ref, info, buffer, ioaddr)
  1. 명령 구성:
     → opcode = NVM_ADMIN_IDENTIFY (0x06)
     → CDW10 = 0x01 (Controller Identify)
     → PRP1 = ioaddr (4KB 응답 버퍼)

  2. nvm_raw_rpc(aq_ref, &cmd, &cpl) 실행

  3. 응답 파싱 (4KB 버퍼의 특정 오프셋):
     Offset 0:    PCI Vendor/Subsystem ID
     Offset 4:    시리얼 번호 (20B)
     Offset 24:   모델명 (40B)
     Offset 64:   펌웨어 리비전 (8B)
     Offset 77:   MDTS (2^MDTS × min_page_size)
     Offset 512:  SQES (SQ 엔트리 크기 log2)
     Offset 513:  CQES (CQ 엔트리 크기 log2)
     Offset 514:  최대 미결 명령 수
     Offset 516:  최대 네임스페이스 수
```

#### Create I/O CQ / SQ

```
admin_cq_create(aq_ref, cq, id, dma, qs)
  → dword[10] = (qs-1)<<16 | id           // 큐 크기 | 큐 ID
  → dword[11] = IRQ벡터<<16 | IEN | PC    // 인터럽트 | 물리 연속
  → PRP1 = CQ 메모리 물리 주소

admin_sq_create(aq_ref, sq, cq, id, dma, qs)
  → dword[10] = (qs-1)<<16 | id           // 큐 크기 | 큐 ID
  → dword[11] = CQ_ID<<16 | QP | PC       // 연결된 CQ | 우선순위 | 물리 연속
  → PRP1 = SQ 메모리 물리 주소
```

#### Set Features (큐 수 요청)

```
nvm_admin_request_num_queues(aq_ref, &n_cqs, &n_sqs)
  1. SET FEATURES:
     → FID = 0x07 (Number of Queues)
     → CDW11 = (n_cqs-1)<<16 | (n_sqs-1)
  2. GET FEATURES:
     → 실제 할당된 큐 수 확인
     → n_cqs = CDW0(31:16) + 1
     → n_sqs = CDW0(15:0) + 1
```

---

## 7. 큐 관리

### src/queue.cpp

#### nvm_queue_clear() — 큐 초기화

```
nvm_queue_clear(q, ctrl, is_cq, queue_no, qs, local, vaddr, ioaddr)
  1. 큐 메모리 제로화 (memset)
  2. 기본 필드 설정:
     → q->no = queue_no
     → q->es = is_cq ? 16 : 64      // CQ=16B, SQ=64B
     → q->qs = qs
     → q->phase = is_cq ? 1 : 0     // CQ는 1부터 시작
     → q->head = q->tail = 0
  3. 도어벨 주소 계산:
     → offset = 0x1000 + (2*no + is_cq) * (4 << dstrd)
     → q->db = (volatile uint32_t*)(mm_ptr + offset)
  4. 메모리 주소 설정:
     → q->vaddr = vaddr
     → q->ioaddr = ioaddr
```

#### nvm_cq_dequeue_block() — 블로킹 CQ 폴링

```
nvm_cq_dequeue_block(cq, timeout_ms)
  → timeout을 나노초로 변환
  → 루프:
    → cpl = nvm_cq_dequeue(cq)    // phase 비트 확인
    → if (cpl != NULL) return cpl
    → _nvm_delay_remain(1ms)      // 1ms 대기
    → timeout 초과 시 NULL 반환
```

---

## 8. DMA 메모리 매핑

### src/dma.cpp — 레퍼런스 카운팅 기반 DMA 관리

#### 내부 구조체

```c
struct map {           // 매핑 디스크립터 (공유, 레퍼런스 카운팅)
    struct mutex lock;
    unsigned long ref_count;
    struct device_ops* ops;  // 플랫폼별 콜백 (ioctl/SISCI)
    void* va_range;          // 가상 주소 범위 정보
};

struct container {     // DMA 핸들 + ioaddrs 배열 (가변 크기)
    nvm_dma_t handle;
    struct map* mapping;
    uint64_t ioaddrs[];  // 실제 IO 주소 배열
};
```

#### populate_handle() — 호스트 페이지 → 컨트롤러 페이지 주소 변환

호스트 페이지 크기(보통 4KB)와 컨트롤러 페이지 크기(MPS)가 다를 수 있으므로, 변환이 필요하다:

```
ctrl_page_addr[i] = host_page_addr[current_page] + offset_within_page

예: 호스트 4KB, 컨트롤러 4KB → 1:1 대응
    호스트 4KB, 컨트롤러 64KB → 16개 호스트 페이지를 1개 컨트롤러 페이지 내로 변환
```

#### _nvm_dma_init() — 핵심 초기화

```
_nvm_dma_init(handle, ctrl, vaddr, local, ops, data)
  1. create_map(): ref_count=1, ctrl ref++
  2. create_container(): ioaddrs 배열 할당
  3. ops->map_range(): 플랫폼별 DMA 매핑 (ioctl 또는 SISCI)
  4. populate_handle(): 호스트 → 컨트롤러 주소 변환
```

#### nvm_dma_unmap() — 해제 흐름

```
nvm_dma_unmap(handle)
  → remove_container(container)
    → put_map(mapping)
      → ref_count--
      → if ref_count == 0:
        → ops->unmap_range()  // 실제 DMA 해제
        → _nvm_ctrl_put()     // 컨트롤러 레퍼런스 해제
```

---

## 9. RPC 메커니즘

### src/rpc.cpp — Admin 큐 RPC

Admin 큐는 프로세스당 하나만 소유 가능하므로, 다른 프로세스의 Admin 명령을 중계하는 RPC 계층이 필요하다.

#### 로컬 모드

```
nvm_aq_create(ref, ctrl, dma_window)
  1. Admin 참조 구조체 할당
  2. DMA 버퍼 할당 (2페이지: ACQ + ASQ)
  3. ACQ 초기화 (페이지 0)
  4. ASQ 초기화 (페이지 1)
  5. stub = execute_command (로컬 실행)
  6. nvm_raw_ctrl_reset() → 컨트롤러 활성화

execute_command(admin_ref, cmd)
  1. ASQ에 명령 삽입 (nvm_sq_enqueue)
  2. CID 할당 (NVM_DEFAULT_CID)
  3. 메모리 펜스 (seq_cst)
  4. SQ 도어벨 링 (nvm_sq_submit)
  5. CQ 블로킹 폴링 (nvm_cq_dequeue_block)
  6. CQ 도어벨 링 (nvm_cq_update)
```

#### 원격 모드 (DIS 클러스터)

```
서버 (nvm_dis_rpc_enable):
  1. 공유 메모리 세그먼트 생성 (SCI_MEMTYPE_SHARED)
  2. 로컬 인터럽트 생성 (콜백 = handle_remote_command)
  3. 핸들 정보 (magic, node_id, intr_no)를 공유 메모리에 기록

클라이언트 (nvm_dis_rpc_bind):
  1. 공유 메모리 읽기 → 서버 정보 획득
  2. 원격 인터럽트 연결
  3. stub = remote_command

RPC 메시지 흐름:
  Client: rpc_cmd(node_id, intr_no, cmd[64B]) → 서버 인터럽트 트리거
  Server: handle_remote_command() → 명령 실행 → 응답 전송
  Client: 로컬 인터럽트 대기 → rpc_cpl(cmd[64B], cpl[16B]) 수신
```

#### nvm_raw_rpc() — 통합 RPC 인터페이스

```
nvm_raw_rpc(ref, cmd, cpl)
  1. _nvm_mutex_lock(ref->lock)
  2. ref->stub(cmd)  // execute_command 또는 remote_command
  3. _nvm_mutex_unlock()
  4. NVM_ERR_PACK(errno, nvme_status) 반환
```

---

## 10. GPU Lock-free 병렬 큐

### include/nvm_parallel_queue.h — BaM의 핵심 혁신

수천 개의 GPU 스레드가 동시에 NVMe 큐를 조작할 수 있도록 하는 lock-free 알고리즘이다.

### CID (Command ID) 관리

```cuda
__device__ uint16_t get_cid(nvm_queue_t* sq) {
    // CID 배열을 순회하며 FREE 슬롯을 찾아 원자적으로 점유
    uint32_t ticket = sq->cid_ticket.fetch_add(1);
    uint32_t pos = ticket & sq->qs_minus_1;

    // cid[pos]가 UNLOCKED일 때까지 스핀
    while (sq->cid[pos].val.fetch_or(LOCKED) == LOCKED)
        __nanosleep(backoff);

    return (uint16_t)pos;
}

__device__ void put_cid(nvm_queue_t* sq, uint16_t id) {
    sq->cid[id].val.store(UNLOCKED, memory_order_release);
}
```

### sq_enqueue() — 8단계 Lock-free 큐 삽입

이것이 BaM의 가장 핵심적인 알고리즘이다:

```
__device__ sq_enqueue(sq, cmd, ...):

┌─ Stage 1: 전역 순서 티켓 ─────────────────────────────────┐
│  ticket = sq->in_ticket.fetch_add(1)                      │
│  pos = ticket & sq->qs_minus_1     // 슬롯 위치 계산      │
│  gen_id = (ticket >> qs_log2) * 2  // 세대 번호 계산      │
└───────────────────────────────────────────────────────────┘
                    ↓
┌─ Stage 2: 슬롯 대기 (Lamport 빵집 알고리즘 변형) ────────┐
│  while (tickets[pos].val.load(relaxed) != gen_id)         │
│      __nanosleep(backoff *= 2);  // 지수 백오프           │
│  tickets[pos].val.load(acquire);  // 획득 장벽            │
└───────────────────────────────────────────────────────────┘
                    ↓
┌─ Stage 3: 명령 복사 (64바이트 = 2 × 32바이트) ──────────┐
│  ulonglong4* dst = (ulonglong4*)(sq->vaddr + pos*64);     │
│  ulonglong4* src = (ulonglong4*)cmd;                      │
│  dst[0] = src[0];  // 첫 32바이트                         │
│  dst[1] = src[1];  // 나머지 32바이트                     │
└───────────────────────────────────────────────────────────┘
                    ↓
┌─ Stage 4: 꼬리 마크 설정 ───────────────────────────────┐
│  tail_mark[pos].val.store(LOCKED, release);              │
│  // "이 슬롯에 명령 준비 완료" 표시                       │
└───────────────────────────────────────────────────────────┘
                    ↓
┌─ Stage 5: 리더 선출 (도어벨 배칭) ──────────────────────┐
│  was_leader = tail_lock.fetch_or(LOCKED);                │
│  if (was_leader == UNLOCKED):                            │
│      // 이 스레드가 리더 — 도어벨 담당                    │
└───────────────────────────────────────────────────────────┘
                    ↓
┌─ Stage 6: 리더의 꼬리 이동 ─────────────────────────────┐
│  if (is_leader):                                         │
│      cur_tail = sq->tail.load(relaxed)                   │
│      // 연속된 LOCKED 마크를 수집                         │
│      while (tail_mark[cur_tail].load(acquire) == LOCKED):│
│          tail_mark[cur_tail].store(UNLOCKED)  // 마크 해제│
│          cur_tail = (cur_tail + 1) & qs_minus_1          │
│      sq->tail.store(cur_tail, release)                   │
│      tail_lock.store(UNLOCKED, release)                  │
└───────────────────────────────────────────────────────────┘
                    ↓
┌─ Stage 7: 도어벨 링 (PTX st.mmio) ─────────────────────┐
│  if (is_leader):                                         │
│      // PTX 인라인 어셈블리로 MMIO 쓰기                   │
│      asm volatile("st.mmio.relaxed.sys.global.u32 ..."   │
│                   : : "l"(sq->db), "r"(cur_tail));       │
│      // GPU에서 PCIe MMIO 쓰기 → NVMe 도어벨 레지스터    │
└───────────────────────────────────────────────────────────┘
                    ↓
┌─ Stage 8: 자기 마크 해제 대기 ──────────────────────────┐
│  while (tail_mark[pos].load(acquire) != UNLOCKED)        │
│      __nanosleep(backoff);                               │
│  tickets[pos].val.fetch_add(1, release);                 │
│  // 다음 세대를 위해 티켓 증가                            │
└───────────────────────────────────────────────────────────┘
```

**도어벨 배칭의 핵심**: 여러 스레드가 거의 동시에 명령을 삽입하면, 리더 하나만 도어벨을 한 번 울려서 여러 명령을 한꺼번에 제출한다. PCIe MMIO 쓰기 비용이 높으므로 이 배칭이 성능에 결정적이다.

### cq_poll() — CQ 폴링 (CID 매칭)

```cuda
__device__ cq_poll(cq, search_cid):
  cur_head = cq->head.load(relaxed)

  loop:
    cpl = (nvm_cpl_t*)(cq->vaddr + (cur_head & qs_minus_1) * 16)

    // Phase 비트 확인 (큐 wrap 감지)
    expected_phase = ~(cur_head >> qs_log2) & 0x01

    if (cpl->dword[3] & 0x01) == expected_phase:  // 새 완료 존재
        if NVM_CPL_CID(cpl) == search_cid:
            return cur_head     // 매칭 성공!
        cur_head++              // 다음 엔트리 확인
    else:
        __nanosleep(backoff)    // 아직 완료 없음, 지수 백오프
```

### cq_dequeue() — CQ 완료 처리

```
__device__ cq_dequeue(cq, pos, sq):
  1. cq->tail.fetch_add(1)           // 논리적 꼬리 증가
  2. pos_locks[pos] 획득             // 슬롯 재사용 방지
  3. head_mark[pos] = LOCKED         // "완료 처리 중" 표시
  4. head_lock 리더 선출
  5. 리더: 연속 LOCKED 마크 수집
     → sq->head 업데이트 (CQ 엔트리에서 SQHD 추출)
     → cq->head 이동
     → head_lock 해제
  6. 자기 마크 UNLOCKED 대기
  7. 논리적 위치까지 헤드 도달 대기
  8. pos_locks[pos] 해제
```

---

## 11. GPU 페이지 캐시

### include/page_cache.h — 2143줄, BaM의 핵심 엔진

### 페이지 상태 머신 (32비트 원자적)

```
비트 레이아웃: [VALID(1)][BUSY(1)][DIRTY(1)][RefCount(29)]
               bit31     bit30    bit29     bits28-0

상태값:
  INVALID = 0x00000000  — 캐시에 없음, 로드 필요
  VALID   = 0x80000000  — GPU 캐시에 존재, 사용 가능
  BUSY    = 0x40000000  — SSD 읽기 진행 중
  DIRTY   = 0x20000000  — 수정됨, 퇴출 시 write-back 필요
  CNT_MASK= 0x1FFFFFFF  — 참조 카운트 (최대 ~536M 스레드)

상태 전이:
  INVALID ──fetch_or(BUSY)──→ BUSY ──fetch_xor(0xC0000000)──→ VALID
   (0x00)     경쟁 승리         (0x40)    read_data 완료         (0x80)
                                                                   ↓
   ↑                                                           DIRTY
   └────────── 퇴출 (ref_count=0, find_slot) ──────────────┘  (0xA0)
```

### 핵심 데이터 구조

#### data_page_t (SSD 페이지 메타데이터)

```c
struct __align__(32) data_page_t {
    simt::atomic<uint32_t> state;  // [VALID|BUSY|DIRTY|RefCount(29)]
    uint32_t offset;               // 캐시 슬롯 인덱스 (cache_pages[] 내 위치)
};
```

#### cache_page_t (캐시 슬롯 메타데이터)

```c
struct __align__(32) cache_page_t {
    simt::atomic<uint32_t> page_take_lock;  // FREE(2) | LOCKED(1) | UNLOCKED(0)
    uint64_t page_translation;               // 인코딩: (page_offset << n_ranges_bits) | range_id
};
```

#### page_cache_d_t (GPU 디바이스 측 캐시 제어 블록)

```c
struct page_cache_d_t {
    // === 메모리 ===
    uint8_t*       base_addr;      // 캐시 데이터 영역 시작 (GPU 메모리, DMA 매핑됨)
    cache_page_t*  cache_pages;    // 슬롯 메타데이터 배열
    padded_struct_pc* page_ticket; // Clock 교체 포인터 (원자적)
    uint64_t*      prp1;           // NVMe PRP1 물리 주소 배열
    uint64_t*      prp2;           // NVMe PRP2 물리 주소 배열

    // === 캐시 크기 ===
    uint64_t page_size;         // 캐시 페이지 크기 (보통 4KB~64KB)
    uint64_t n_pages;           // 전체 캐시 슬롯 수
    uint64_t ctrl_page_size;    // NVMe 컨트롤러 페이지 크기

    // === 범위 (Range) 관리 ===
    pages_t*     ranges;              // range별 data_page_t 배열
    data_dist_t* ranges_dists;        // range별 분배 정책 (STRIPE/REPLICATE)
    uint64_t*    ranges_page_starts;  // range별 SSD 페이지 오프셋
    uint64_t     n_ranges;            // range 수
    uint64_t     n_ranges_bits;       // range 비트 수 (인코딩용)
    uint64_t     n_ranges_mask;       // range 마스크

    // === 컨트롤러 ===
    Controller** d_ctrls;       // NVMe 컨트롤러 배열
    uint64_t     n_ctrls;       // 컨트롤러 수

    // === 최적화 ===
    simt::atomic<uint64_t>* ctrl_counter;  // 라운드 로빈 카운터
    simt::atomic<uint64_t>* q_head;        // 더블 리드 최적화
    simt::atomic<uint64_t>* q_tail;
    simt::atomic<uint64_t>* q_lock;
    simt::atomic<uint64_t>* extra_reads;

    // === 메서드 ===
    __device__ cache_page_t* get_cache_page(uint32_t page);
    __device__ uint32_t find_slot(uint64_t address, uint64_t range_id, uint32_t queue_);
};
```

### 다중 컨트롤러 분배 전략

```
STRIPE 모드:
  데이터를 페이지 오프셋 기준으로 컨트롤러에 분산
  page_offset=10, n_ctrls=4:
    컨트롤러 = 10 % 4 = 2 (컨트롤러 2에 저장)
    SSD 내 페이지 = page_start + 10/4 = page_start + 2

REPLICATE 모드:
  모든 컨트롤러에 동일 데이터 복제
  읽기 시 라운드 로빈: ctrl = ctrl_counter++ % n_ctrls
  읽기 대역폭 극대화
```

### read_data() / write_data() — GPU→NVMe I/O 발행

```cuda
__device__ read_data(page_cache_d_t* pc, QueuePair* qp,
                     uint64_t starting_lba, uint64_t n_blocks,
                     unsigned long long cache_slot) {
    // 1. NVMe Read 명령 구성
    nvm_cmd_t cmd;
    nvm_cmd_header(&cmd, cid, NVM_IO_READ, qp->nvmNamespace);
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks - 1);
    nvm_cmd_data_ptr(&cmd, pc->prp1[cache_slot], pc->prp2[cache_slot]);
    //                      ↑ GPU 캐시 슬롯의 물리 주소 (NVMe가 여기에 DMA)

    // 2. SQ에 삽입 (lock-free 8단계 알고리즘)
    uint16_t pos = sq_enqueue(&qp->sq, &cmd, ...);

    // 3. CQ 폴링 (CID 매칭)
    uint32_t cq_pos = cq_poll(&qp->cq, cid, ...);

    // 4. CQ 완료 처리
    cq_dequeue(&qp->cq, cq_pos, &qp->sq, ...);

    // 5. CID 반환
    put_cid(&qp->sq, cid);
}
```

### acquire_page() — 페이지 캐시 핵심 로직

```
__device__ acquire_page(page_index, ref_count, is_write, ctrl, queue):

  ┌───────────────────────────────────────────────────────────────┐
  │ 1. 참조 카운트 증가 (원자적)                                   │
  │    read_state = pages[page_index].state.fetch_add(ref_count)  │
  │                                                                │
  │ 2. 상태 판별                                                   │
  │    ┌─────────────────────────────────────────────────────────┐│
  │    │ CASE: VALID (bit31=1, bit30=0)                         ││
  │    │   → 캐시 히트! 즉시 pages[page_index].offset 반환      ││
  │    │   → I/O 없음, 순수 GPU 메모리 접근                      ││
  │    ├─────────────────────────────────────────────────────────┤│
  │    │ CASE: INVALID (bit31=0, bit30=0)                       ││
  │    │   → 내가 로드 담당:                                     ││
  │    │   a) fetch_or(BUSY) → BUSY 비트 설정                   ││
  │    │   b) 경쟁 확인 (이미 BUSY면 다른 스레드가 먼저 설정)     ││
  │    │   c) 경쟁 승리 시:                                      ││
  │    │      slot = find_slot()  // Clock 교체로 빈 슬롯 확보   ││
  │    │      ctrl_id = get_backing_ctrl_(page_offset)           ││
  │    │      lba = get_backing_page_(page_offset)               ││
  │    │      read_data(cache, qp, lba, n_blocks, slot)          ││
  │    │      pages[page_index].offset = slot                    ││
  │    │      if (is_write): pages[page_index].state |= DIRTY   ││
  │    │      pages[page_index].state ^= 0xC0000000             ││
  │    │        // BUSY(0x40) XOR → 0xC0 = VALID(0x80)          ││
  │    │      return slot                                        ││
  │    ├─────────────────────────────────────────────────────────┤│
  │    │ CASE: BUSY (bit30=1)                                   ││
  │    │   → 다른 스레드가 로드 중, VALID 될 때까지 스핀         ││
  │    │   → __nanosleep(backoff *= 2) 지수 백오프               ││
  │    └─────────────────────────────────────────────────────────┘│
  └───────────────────────────────────────────────────────────────┘
```

### find_slot() — Clock 페이지 교체

```
__device__ find_slot(address, range_id, queue_):
  page_ticket = atomic_increment(page_ticket)  // 다음 후보
  slot = page_ticket % n_pages                 // 순환

  loop:
    if (cache_pages[slot].page_take_lock == FREE):
        return slot  // 빈 슬롯 발견

    elif (해당 슬롯의 참조 카운트 == 0):
        // 퇴출 가능
        old_translation = cache_pages[slot].page_translation

        if (old_page의 DIRTY 비트):
            write_data(old_lba, old_data)  // NVMe write-back

        cache_pages[slot].page_translation = (address << n_ranges_bits) | range_id
        cache_pages[slot].page_take_lock = LOCKED
        return slot

    else:
        slot = (slot + 1) % n_pages  // 다음 슬롯 시도
```

### Warp-level 합병 (Coalescing)

같은 페이지를 요청하는 32개 워프 스레드를 하나의 acquire로 합치는 최적화:

```cuda
__device__ coalesce_page(gaddr):
  1. uint32_t eq_mask = __match_any_sync(mask, gaddr)
     // gaddr이 같은 모든 스레드의 비트마스크 반환

  2. eq_mask &= __match_any_sync(mask, (uint64_t)this)
     // 같은 array 인스턴스인지도 확인

  3. int master = __ffs(eq_mask) - 1
     // 마스터 스레드 선출 (최하위 레인)

  4. uint32_t count = __popc(eq_mask)
     // 합병된 스레드 수 = 참조 카운트 증가량

  5. if (master == lane):
         base = acquire_page(page, count, ...)
         // 마스터만 acquire 실행, count만큼 한번에 참조 증가

  6. base_master = __shfl_sync(eq_mask, base_master, master)
     // 결과를 모든 합병 스레드에 브로드캐스트
```

### 소프트웨어 TLB (반복 접근 최적화)

```c
template<typename T, int n=32>
struct tlb {
    tlb_entry entries[n];  // 직접 매핑 (n 엔트리)
    array_d_t<T>* array;

    __device__ acquire(gid):
        idx = gid % n
        if (entries[idx].global_id == gid):
            entries[idx].state.fetch_add(count)  // TLB 히트
            return entries[idx].page
        else:
            // TLB 미스 → 전체 경로 (acquire_page)
            release old, acquire new
};
```

### __flush() 커널 — 더티 페이지 라이트백

```cuda
__global__ __flush(page_cache_d_t* pc) {
    for each cache_page in parallel:
        if (page is DIRTY && ref_count == 0):
            write_data(page_lba, cache_data)  // NVMe 쓰기
            clear DIRTY bit
}
// 호스트에서 flush_cache()로 호출
```

---

## 12. C++ 래퍼 계층

### include/ctrl.h — Controller 클래스

```cpp
struct Controller {
    // === 상태 ===
    simt::atomic<uint64_t> access_counter;  // SSD 접근 횟수
    nvm_ctrl_t*      ctrl;        // NVMe 컨트롤러 핸들
    nvm_aq_ref       aq_ref;      // Admin 큐 참조
    DmaPtr           aq_mem;      // Admin 큐 메모리 (3페이지)
    nvm_ctrl_info    info;        // 컨트롤러 식별 정보
    nvm_ns_info      ns;          // 네임스페이스 정보
    uint16_t         n_sqs, n_cqs, n_qps;  // 큐 수
    uint32_t         deviceId;    // CUDA 디바이스 ID
    QueuePair**      h_qps;       // 호스트 큐 쌍 포인터 배열
    QueuePair*       d_qps;       // GPU 큐 쌍 배열
    simt::atomic<uint64_t> queue_counter;  // 라운드 로빈 셀렉터

    // === GPU 사본 ===
    void*     d_ctrl_ptr;    // GPU 디바이스 포인터
    BufferPtr d_ctrl_buff;   // GPU 버퍼

    // === 초기화 흐름 (Linux) ===
    Controller(path, ns_id, cuda_dev, qd, n_queues):
      1. open(path)                              // /dev/libnvmN 열기
      2. nvm_ctrl_init(&ctrl, fd)                // 컨트롤러 초기화
      3. createDma(ctrl, 3*ctrl->page_size)      // Admin 큐 메모리
      4. nvm_aq_create(&aq_ref, ctrl, dma)       // Admin 큐 생성
      5. nvm_admin_ctrl_info(aq_ref, &info)      // Identify Controller
      6. nvm_admin_ns_info(aq_ref, &ns, ns_id)   // Identify Namespace
      7. cudaHostRegister(ctrl->mm_ptr, IoMemory) // GPU에서 BAR0 접근 가능하게
      8. reserveQueues(n_queues)                  // SET FEATURES
      9. for each queue pair:
           h_qps[i] = new QueuePair(...)          // I/O 큐 쌍 생성
      10. GPU 메모리에 Controller 복사
};
```

### include/queue.h — QueuePair 클래스

```cpp
struct QueuePair {
    uint32_t pageSize, block_size, block_size_log;
    uint32_t nvmNamespace;
    uint16_t qp_id;
    nvm_queue_t sq, cq;            // NVMe SQ/CQ 디스크립터
    DmaPtr sq_mem, cq_mem, prp_mem;  // 큐 메모리 DMA 매핑

    // GPU 동기화 구조체
    BufferPtr sq_tickets, sq_tail_mark, sq_cid;
    BufferPtr cq_head_mark, cq_pos_locks;

    // 크기 제한
    MAX_SQ_ENTRIES_64K = 1024   // 64KB / 64B
    MAX_CQ_ENTRIES_64K = 4096   // 64KB / 16B

    // === 초기화 흐름 ===
    QueuePair(ctrl, aq_ref, id, cuda_dev):
      1. CAP 레지스터에서 CQR 확인
      2. SQ/CQ 크기 결정 (64K 또는 MQES+1 중 작은 값)
      3. createDma(GPU 메모리)로 큐 메모리 할당
      4. nvm_admin_cq_create()                  // CQ 생성
      5. cudaHostGetDevicePointer(&devPtr, cq.db)
         → cq.db = devPtr                       // GPU에서 CQ 도어벨 접근
      6. nvm_admin_sq_create()                  // SQ 생성
      7. cudaHostGetDevicePointer(&devPtr, sq.db)
         → sq.db = devPtr                       // GPU에서 SQ 도어벨 접근
      8. init_gpu_specific_struct(cuda_dev)     // tickets, marks, cid 배열 할당
};
```

**핵심**: `cudaHostGetDevicePointer()`로 NVMe 도어벨 MMIO 레지스터를 GPU 디바이스 포인터로 변환한다. 이렇게 하면 GPU 커널이 `sq.db`에 직접 쓸 수 있고, 이것이 PCIe MMIO 쓰기로 변환되어 NVMe 컨트롤러에 전달된다.

### include/buffer.h — 메모리 할당 헬퍼

```cpp
// 호스트 DMA 버퍼 생성 (4KB 정렬)
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size);

// GPU DMA 버퍼 생성 (64KB 정렬, P2P용)
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size, int cudaDevice);

// 핀드 호스트 메모리 (cudaHostAlloc)
BufferPtr createBuffer(size_t size);

// GPU 메모리 (128B L2 캐시라인 정렬)
BufferPtr createBuffer(size_t size, int cudaDevice);

// 64KB 정렬 GPU 메모리 (NVMe DMA 요구사항 충족)
void getDeviceMemory(int device, void*& bufferPtr, void*& devicePtr,
                     size_t size, void*& origPtr);
```

---

## 13. Smart Pointer 시스템

### bafs_ptr<T> — 투명 SSD 배열 접근

```cpp
template<typename T>
struct bafs_ptr {
    array_t<T>*   h_pData;    // 호스트 배열 핸들 (통계용)
    array_d_t<T>* pData;      // 디바이스 배열 핸들
    uint64_t      start_idx;  // 현재 논리 오프셋

    // 읽기: page_cache → acquire_page → GPU 메모리 반환
    __device__ T operator[](uint64_t i) const {
        return (*pData)[start_idx + i];
        // 내부: find_range → acquire_page → get_cache_addr → read
    }

    // 쓰기: DIRTY 플래그 설정
    __device__ void operator()(uint64_t i, T val) {
        (*pData)(start_idx + i, val);
        // 내부: acquire_page(write=true) → set DIRTY → write to cache
    }

    // 포인터 산술
    bafs_ptr operator+(int64_t i) { return {h_pData, pData, start_idx + i}; }
    bafs_ptr& operator++()       { start_idx++; return *this; }
};
```

### bam_ptr<T> — 직접 페이지 캐시 포인터

```cpp
template<typename T>
struct bam_ptr {
    data_page_t*   page;   // 현재 캐싱된 페이지
    array_d_t<T>*  array;  // 소유 배열

    // 읽기 + 자동 페이지 전환
    __device__ T operator[](uint64_t i) {
        update_page(i);  // 페이지 경계 변경 시 acquire/release
        return *(T*)(cache_addr + offset);
    }
};
```

### bam_ptr_tlb<T> — TLB 캐시 포인터

```cpp
template<typename T, int n=32>
struct bam_ptr_tlb {
    tlb<T, n> tlb_cache;  // 32 엔트리 TLB
    // 반복 접근 시 TLB 히트로 acquire_page 오버헤드 제거
};
```

---

## 14. 플랫폼 추상화

### Linux 경로 (src/linux/)

```
device.cpp:
  nvm_ctrl_init(fd)
    → dup(fd)                              // FD 독립성
    → mmap(BAR0, MAP_SHARED | MAP_LOCKED)  // BAR0 매핑
    → _nvm_ctrl_init(ctrl, mm_ptr)         // 컨트롤러 초기화

dma.cpp:
  nvm_dma_map_host(ctrl, vaddr, size)
    → ioctl(NVM_MAP_HOST_MEMORY)  // 커널이 VA→PA 변환

  nvm_dma_map_device(ctrl, devptr, size)
    → 64KB 정렬 확인
    → ioctl(NVM_MAP_DEVICE_MEMORY)  // 커널이 nvidia_p2p_get_pages 호출
```

### DIS/SmartIO 경로 (src/dis/)

```
device.c:
  borrow_device(fdid)
    → SCIOpen() → SCIBorrowDevice() → SCIConnectDeviceSegment()
    → SCIMapRemoteSegment(SCI_FLAG_IO_MAP_IOSPACE)

dma.c:
  nvm_dis_dma_create()
    → SCICreateSegment() + SCIAttachPhysicalMemory(SCI_FLAG_CUDA_BUFFER)

  nvm_dis_dma_map_device()
    → SCIMapLocalSegmentForDevice()  // GPU 메모리 → 원격 DMA 매핑

interrupt.c:
  SCICreateDataInterrupt()  → 로컬 인터럽트 (콜백 기반)
  SCIConnectDataInterrupt() → 원격 인터럽트 연결
  SCITriggerDataInterrupt() → 데이터 전송 + 인터럽트

rpc.c:
  서버: 공유 메모리에 핸들 정보 기록, 인터럽트 콜백으로 명령 처리
  클라이언트: 공유 메모리에서 핸들 정보 읽기, 원격 인터럽트로 명령 전송
```

---

## 15. 벤치마크 카탈로그

### 마이크로벤치마크

| 벤치마크 | 워크로드 | 접근 패턴 | 핵심 측정 |
|----------|----------|-----------|-----------|
| **array/** | 단일 요소 접근 | 순차/랜덤 | 요소 IOPS |
| **block/** | NVMe 블록 I/O | 순차/랜덤/혼합 | 블록 IOPS, 대역폭 |
| **cache/** | 캐시 효율 분석 | 워프/블록 단위 | 캐시 히트율 |
| **readwrite/** | 혼합 R/W | 비율 설정 가능 | 혼합 IOPS |
| **pattern/** | 접근 패턴 분석 | 순차/랜덤/스트라이드/Zipf | 패턴별 IOPS |
| **iodepth-block/** | 큐 깊이 변화 | 랜덤 | 처리량 vs 깊이 |
| **reduction/** | 트리 리덕션 | 순차 읽기 | 리덕션 ops/s |
| **scan/** | 프리픽스 스캔 | 순차 | 스캔 원소/s |
| **vectoradd/** | C[i]=A[i]+B[i] | 순차 | 연산 ops/s |

### 그래프 애플리케이션 벤치마크

| 벤치마크 | 알고리즘 | I/O 특성 | 구현 변형 |
|----------|----------|----------|-----------|
| **bfs/** | 너비 우선 탐색 | 불규칙 (그래프 토폴로지 의존) | BASELINE, COALESCE, COALESCE_CHUNK (0~10) |
| **cc/** | 연결 요소 | 불규칙 + 해시 파티셔닝 | HASH, PTR 기반 (해시 빈: 128개, 원소/빈: 64개) |
| **sssp/** | 최단 경로 (정수) | 불규칙 (이완 반복) | Bellman-Ford 스타일, 반복 32회 기본 |
| **sssp_float/** | 최단 경로 (실수) | 불규칙 | 부동소수점 AtomicMin |
| **pagerank/** | 페이지랭크 | 불규칙 (수렴까지 반복) | 반복 32회 기본 |

### 공통 파라미터

```
-g: GPU 디바이스 ID (기본: 0)
-k: NVMe 컨트롤러 수 (기본: 1)
-p: 캐시 페이지 수 (기본: 1024)
-P: 페이지 크기 바이트 (기본: 4096)
-t: CUDA 스레드 수 (기본: 1024)
-b: CUDA 블록 크기 (기본: 64)
-d: 큐 깊이 (기본: 16)
-q: 컨트롤러당 큐 수 (기본: 1)
-m: 메모리 타입 (0=GPU, 1=UVM RO, 2=UVM Direct, 6=BAFS)
-S: SSD 타입 (0=Samsung, 1=Intel)
```

### 그래프 데이터 형식

`bam_data_conversion.py`: MTX(Matrix Market) → BaM CSR 형식 변환
- `.bel.col`: 정점 포인터 (int64 CSR indptr)
- `.bel.dst`: 간선 목적지 (int64 인접)
- `.bel.val`: 간선 가중치 (float32)

---

## 16. 전체 데이터 흐름 추적

### 초기화 → I/O → 완료의 완전한 흐름

```
┌──────────────────────────────────────────────────────────────────────┐
│ 1. 시스템 준비                                                       │
│                                                                      │
│    scripts/unbind.sh                                                 │
│      → NVMe 디바이스를 Linux nvme 드라이버에서 분리                   │
│      → echo BDF > /sys/bus/pci/devices/BDF/driver/unbind             │
│                                                                      │
│    insmod libnvm.ko                                                  │
│      → libnvm_helper_entry()                                         │
│        → pci_register_driver() → add_pci_dev()                      │
│          → /dev/libnvm0, /dev/libnvm1, ... 생성                     │
└──────────────────────────────────────────────────────────────────────┘
                              ↓
┌──────────────────────────────────────────────────────────────────────┐
│ 2. 유저스페이스 초기화 (Controller 생성자)                            │
│                                                                      │
│    open("/dev/libnvm0")                                              │
│      ↓                                                               │
│    mmap(BAR0) → GPU에서 접근 가능하도록 cudaHostRegisterIoMemory     │
│      ↓                                                               │
│    Admin 큐 생성 (DMA 메모리 할당 → nvm_raw_ctrl_reset)              │
│      ↓                                                               │
│    Identify Controller/Namespace                                     │
│      ↓                                                               │
│    I/O 큐 쌍 생성 (GPU 메모리에 SQ/CQ 할당)                          │
│      → cudaHostGetDevicePointer()로 도어벨을 GPU 포인터로 변환       │
│      → GPU lock-free 구조체 초기화 (tickets, marks, cid)             │
│      ↓                                                               │
│    페이지 캐시 생성 (GPU 메모리에 캐시 영역 할당)                     │
│      → DMA 매핑으로 NVMe가 GPU 캐시에 직접 DMA 가능                  │
│      → PRP 테이블 초기화 (prp1[], prp2[])                            │
└──────────────────────────────────────────────────────────────────────┘
                              ↓
┌──────────────────────────────────────────────────────────────────────┐
│ 3. GPU 커널 실행 (예: BFS)                                           │
│                                                                      │
│    __global__ bfs_kernel(array_d_t<uint64_t>* vertices, ...) {       │
│                                                                      │
│        // 3a. 정점 데이터 접근 요청                                   │
│        uint64_t neighbor = (*edges)[edge_index];                     │
│                                                                      │
│        // 3b. Warp 합병 (32 스레드가 같은 페이지면 1번만 acquire)     │
│        eq_mask = __match_any_sync(mask, page_addr)                   │
│        master = __ffs(eq_mask) - 1                                   │
│        count = __popc(eq_mask)                                       │
│                                                                      │
│        // 3c. 마스터만 페이지 획득                                    │
│        if (lane == master):                                          │
│            slot = acquire_page(page, count, ...)                     │
│                                                                      │
│            // 캐시 히트 (VALID):                                     │
│            //   → 즉시 slot 반환, I/O 없음                          │
│                                                                      │
│            // 캐시 미스 (INVALID → BUSY):                            │
│            //   → find_slot()으로 빈 슬롯 확보 (Clock 교체)          │
│            //   → read_data() 호출:                                  │
│            //     ┌──────────────────────────────────────┐           │
│            //     │ NVMe Read 명령 구성                   │           │
│            //     │  opcode=0x02, nsid, lba, n_blocks     │           │
│            //     │  PRP1=GPU캐시슬롯물리주소             │           │
│            //     │                                       │           │
│            //     │ sq_enqueue (8단계 lock-free):          │           │
│            //     │  1. 티켓 획득                          │           │
│            //     │  2. 슬롯 대기                          │           │
│            //     │  3. 명령 복사 (64B)                    │           │
│            //     │  4. 꼬리 마크 설정                     │           │
│            //     │  5. 리더 선출                          │           │
│            //     │  6. 연속 마크 수집                     │           │
│            //     │  7. 도어벨 링 (GPU→PCIe MMIO)         │           │
│            //     │  8. 마크 해제 대기                     │           │
│            //     │                                       │           │
│            //     │ NVMe 컨트롤러:                        │           │
│            //     │  SQ에서 명령 읽음                      │           │
│            //     │  → SSD에서 데이터 읽음                 │           │
│            //     │  → PCIe P2P DMA로 GPU 캐시에 직접 전송 │           │
│            //     │  → CQ에 완료 엔트리 작성               │           │
│            //     │                                       │           │
│            //     │ cq_poll (CID 매칭):                   │           │
│            //     │  CQ 헤드부터 search_cid 탐색           │           │
│            //     │  Phase 비트로 새 완료 판별              │           │
│            //     │                                       │           │
│            //     │ cq_dequeue:                           │           │
│            //     │  헤드 마크 → 리더 선출 → 헤드 이동     │           │
│            //     │  CQ 도어벨 업데이트                    │           │
│            //     └──────────────────────────────────────┘           │
│            //                                                        │
│            //   → BUSY→VALID 전이 (XOR 0xC0000000)                  │
│                                                                      │
│        // 3d. 결과 브로드캐스트                                       │
│        slot = __shfl_sync(eq_mask, slot, master)                     │
│                                                                      │
│        // 3e. GPU 메모리에서 데이터 읽기                              │
│        data = *(T*)(base_addr + slot * page_size + offset)           │
│        // 순수 GPU 메모리 접근 — 지연 시간 ~100ns                    │
│                                                                      │
│        // 3f. 참조 해제                                              │
│        if (lane == master):                                          │
│            release_page(page, count)                                 │
│            // state.fetch_sub(count, memory_order_release)           │
│    }                                                                 │
└──────────────────────────────────────────────────────────────────────┘
```

### GPU 메모리 레이아웃

```
GPU 디바이스 메모리 (전부 DMA 매핑됨):
┌──────────────────────────────────────────────────┐
│ SQ 메모리 (1024 엔트리 × 64B = 64KB)             │ ← 64KB 정렬
│   NVMe 명령 버퍼, GPU 스레드가 직접 쓰기          │
├──────────────────────────────────────────────────┤
│ CQ 메모리 (4096 엔트리 × 16B = 64KB)             │ ← 64KB 정렬
│   NVMe 완료 버퍼, GPU 스레드가 폴링               │
├──────────────────────────────────────────────────┤
│ 캐시 데이터 페이지 (n_pages × page_size)          │ ← 64KB 정렬
│   NVMe 컨트롤러가 여기에 직접 DMA                 │
│   GPU 스레드가 여기서 직접 읽기/쓰기              │
├──────────────────────────────────────────────────┤
│ 캐시 메타데이터 (cache_page_t[n_pages])           │ ← 128B 정렬
├──────────────────────────────────────────────────┤
│ PRP 테이블 (prp1[], prp2[])                      │
│   캐시 페이지의 물리 주소 (NVMe 명령에 사용)      │
├──────────────────────────────────────────────────┤
│ 상태 배열 (data_page_t[], tickets[], marks[])     │ ← 32B 정렬
│   False sharing 최소화를 위한 패딩               │
└──────────────────────────────────────────────────┘

MMIO 레지스터 (PCIe, cudaHostRegisterIoMemory로 매핑):
  SQ 도어벨 @ 0x1000 + (qid × 8)   ← GPU 커널이 직접 쓰기
  CQ 도어벨 @ 0x1004 + (qid × 8)   ← GPU 커널이 직접 쓰기
```

---

## 17. 빌드 시스템

### CMakeLists.txt 주요 옵션

```cmake
set(KERNEL "/lib/modules/.../build")     # 커널 소스 경로
set(NVIDIA "" CACHE PATH "NVIDIA driver") # P2P 헤더 경로
set(no_module false)                      # 커널 모듈 빌드 제외
set(no_cuda false)                        # CUDA 지원 제외
set(no_smartio false)                     # DIS 클러스터 제외
set(nvidia_archs "70;80;90")             # GPU 아키텍처 (V100, A100, H100)
```

### 빌드 명령

```bash
mkdir build && cd build
cmake .. -DNVIDIA=/path/to/nvidia/headers
make libnvm           # 코어 라이브러리
make benchmarks       # 전체 벤치마크
make kernel_module    # 커널 모듈 (별도 빌드)
```

### 실행 흐름

```bash
# 1. NVMe 디바이스를 기존 드라이버에서 분리
./scripts/unbind.sh

# 2. BaM 커널 모듈 로드
insmod build/module/libnvm.ko

# 3. 벤치마크 실행
./build/benchmarks/bfs/nvm-bfs-bench \
  -f /data/graph.bel \
  -g 0 -k 2 -p 4096 -P 4096 \
  -d 1024 -q 128 -m 6 -v 4
```

---

## Linux NVMe 드라이버와의 완전 비교

| 항목 | Linux NVMe | BaM |
|------|-----------|-----|
| **I/O 발행자** | CPU (커널 컨텍스트) | GPU (CUDA 커널 스레드) |
| **경로** | syscall → VFS → FS → Block Layer → NVMe 드라이버 | GPU 커널 → page_cache → NVMe 큐 직접 |
| **큐 관리** | blk-mq (CPU 코어당 큐) | GPU 스레드 그룹당 큐 (lock-free) |
| **동기화** | 스핀락, RCU | device-scope atomics, 티켓 락, 리더 선출 |
| **캐시** | Linux Page Cache (CPU 메모리) | page_cache_d_t (GPU 메모리) |
| **데이터 전송** | DMA (CPU 메모리 경유, 바운스 버퍼) | PCIe P2P (GPU↔SSD 직접, CPU 우회) |
| **도어벨** | CPU MMIO 쓰기 | GPU PTX st.mmio (GPU→PCIe MMIO) |
| **스케줄러** | mq-deadline, kyber 등 | 없음 (직접 제출) |
| **CPU 개입** | 모든 I/O에 필수 | 초기화 후 불필요 |
| **확장성** | CPU 코어 수에 비례 | GPU SM/워프 수에 비례 (수천 동시 요청) |
| **대역폭** | CPU-메모리 대역폭에 제한 | GPU-SSD PCIe 대역폭까지 활용 |
| **페이지 교체** | LRU (2-list) | Clock (순환 포인터) |

---

## 핵심 혁신 요약

1. **GPU-Direct I/O**: CPU를 스토리지 데이터 경로에서 완전히 제거. GPU 커널 스레드가 NVMe SQ에 직접 명령 삽입, CQ를 직접 폴링.

2. **Lock-free 병렬 큐 (8단계 알고리즘)**: Lamport 빵집 변형 + 리더 선출 + 꼬리 마크 배칭으로 수천 GPU 스레드의 동시 큐 접근을 지원.

3. **GPU 소프트웨어 페이지 캐시**: 32비트 원자적 상태 머신 (INVALID→BUSY→VALID), 29비트 참조 카운팅, Clock 교체, Warp-level 합병.

4. **도어벨 배칭**: 여러 스레드가 동시 삽입할 때 리더 하나만 도어벨을 울려 PCIe MMIO 비용 최소화.

5. **다중 SSD 확장**: STRIPE/REPLICATE 분배로 대역폭 선형 확장.

6. **투명한 접근 API**: `bafs_ptr<T>`로 SSD 데이터를 일반 배열처럼 접근. 캐시/I/O 로직이 `operator[]` 뒤에 숨겨짐.

7. **커널 모듈의 이중 역할**: NVMe BAR0 레지스터를 유저스페이스에 mmap 제공 + 호스트/GPU 메모리의 DMA 주소 변환 제공.
