# BaM 완전 가이드: GPU-Initiated NVMe I/O 시스템 A-Z

> 소스 코드: `sources/bam/` (https://github.com/ZaidQureshi/bam)
> 논문: "BaM: A Case for Enabling Fine-grain High Throughput GPU-Orchestrated Access to Storage" (MICRO 2022)
> 이 문서는 BaM 소스코드 전체를 분석하여, 이 문서만 읽으면 소스 코드를 직접 볼 필요가 없을 정도로 상세하게 작성되었다.

---

## 1. BaM 개요

### 1.1 프로젝트 목적

BaM(Big accelerator Memory)은 GPU에서 NVMe SSD로 직접 I/O를 발행하는 시스템이다. 기존에는 CPU가 모든 I/O를 중재했지만, BaM에서는 GPU CUDA 커널 내의 스레드가 NVMe Submission Queue에 직접 커맨드를 기록하고 도어벨을 울려서, CPU 개입 없이 데이터를 GPU 메모리로 가져온다.

### 1.2 기존 방식(CPU-initiated) vs BaM(GPU-initiated) 비교

```
기존 CPU-Initiated I/O:
┌───────────┐    ①요청     ┌─────────┐   ②SQ기록    ┌──────────┐
│  GPU 커널 │ ──────────→ │   CPU   │ ──────────→ │ NVMe SSD │
│           │ ←────────── │         │ ←────────── │          │
└───────────┘    ④DMA      └─────────┘   ③CQ완료    └──────────┘

  지연: GPU→CPU 인터럽트 + CPU 스케줄링 + 커널 드라이버 오버헤드
  대역폭: CPU가 병목 (단일 코어가 수백만 IOPS 처리 불가)

BaM GPU-Initiated I/O:
┌───────────┐  ①SQ 직접기록  ┌──────────┐
│  GPU 커널 │ ─────────────→│ NVMe SSD │
│ (스레드)  │ ←─────────────│          │
└───────────┘  ②DMA→GPU캐시  └──────────┘
              ③CQ 직접폴링

  지연: GPU 스레드가 직접 NVMe 큐 조작 (CPU 개입 없음)
  대역폭: 수천 GPU 스레드가 병렬로 I/O 발행 → 수백만 IOPS 가능
```

### 1.3 핵심 혁신

1. **GPU 페이지 캐시**: GPU DRAM에 소프트웨어 관리 페이지 캐시를 구현하여, SSD 데이터를 GPU 메모리에 캐싱한다. 캐시 히트 시 SSD 접근 없이 즉시 데이터를 사용한다.
2. **NVMe 큐 직접 접근**: NVMe 컨트롤러의 BAR0 레지스터(도어벨 포함)를 `cudaHostRegisterIoMemory`로 GPU에 노출하여, GPU 스레드가 PCIe MMIO를 통해 도어벨을 직접 쓴다.
3. **Lock-free 병렬 큐**: 수천 GPU 스레드가 하나의 NVMe SQ/CQ를 동시에 사용할 수 있도록 티켓 기반 lock-free 알고리즘을 설계했다.
4. **Warp-level Coalescing**: 같은 페이지를 접근하는 warp 내 스레드들을 `__match_any_sync`로 그룹화하여, 단 한 번의 캐시 acquire로 최대 32개 스레드를 처리한다.

### 1.4 소스 코드 구조 개요

```
bam/
├── module/              ← 커널 모듈 (libnvm.ko)
│   ├── pci.c            ← PCI 드라이버 등록, ioctl, mmap
│   ├── ctrl.c/h         ← 컨트롤러 관리, 캐릭터 디바이스
│   ├── map.c/h          ← DMA 매핑 (호스트/GPU)
│   └── list.c/h         ← 스핀락 보호 이중 연결 리스트
├── include/             ← 공개 헤더 (사용자 공간 + GPU)
│   ├── nvm_types.h      ← 핵심 자료형 (nvm_ctrl_t, nvm_queue_t, nvm_cmd_t 등)
│   ├── nvm_ctrl.h       ← 컨트롤러 초기화/리셋 API
│   ├── nvm_admin.h      ← Admin 커맨드 API
│   ├── nvm_queue.h      ← CPU 측 큐 조작 (enqueue, poll, submit)
│   ├── nvm_cmd.h        ← NVMe 커맨드 빌더 (헤더, PRP, 블록 주소)
│   ├── nvm_dma.h        ← DMA 매핑 API (호스트/GPU/DIS)
│   ├── nvm_parallel_queue.h ← GPU 병렬 큐 (lock-free sq_enqueue, cq_poll 등)
│   ├── page_cache.h     ← GPU 페이지 캐시 핵심 (★ 가장 중요한 파일)
│   ├── ctrl.h           ← Controller 클래스 (NVMe 초기화 래퍼)
│   ├── queue.h          ← QueuePair 클래스 (SQ+CQ 쌍)
│   └── buffer.h         ← DMA/GPU 버퍼 할당 유틸리티
├── src/                 ← 사용자 공간 라이브러리 구현
│   ├── ctrl.cpp         ← nvm_ctrl_init 구현
│   ├── admin.cpp        ← Admin 커맨드 구현
│   ├── queue.cpp        ← 큐 초기화 구현
│   ├── dma.cpp          ← DMA 매핑 구현
│   ├── linux/           ← Linux 로컬 백엔드 (ioctl 통신)
│   └── dis/             ← DIS/SmartIO 원격 백엔드
└── benchmarks/          ← 벤치마크 CUDA 프로그램
    ├── bfs/main.cu      ← 너비 우선 탐색
    ├── sssp/main.cu     ← 최단 경로 (Bellman-Ford)
    ├── pagerank/main.cu ← PageRank
    ├── cc/main.cu       ← Connected Components
    ├── block/main.cu    ← 블록 I/O 성능 테스트
    └── array/main.cu    ← 배열 접근 IOPS 테스트
```

---

## 2. 시스템 아키텍처 전체 그림

```
┌─────────────────────────────────────────────────────────────────────┐
│                         GPU (CUDA Device)                           │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │                    GPU 페이지 캐시 (page_cache_d_t)          │   │
│  │  ┌─────────┬─────────┬─────────┬───────── ─ ─ ─┐           │   │
│  │  │ Page 0  │ Page 1  │ Page 2  │   ...   Page N │           │   │
│  │  │ (4~64KB)│ (4~64KB)│ (4~64KB)│                │           │   │
│  │  └─────────┴─────────┴─────────┴───────── ─ ─ ─┘           │   │
│  │  base_addr ──→ DMA 매핑됨 (NVMe가 직접 기록)                │   │
│  │                                                              │   │
│  │  cache_pages[]: 각 슬롯의 상태 (FREE/LOCKED/UNLOCKED)       │   │
│  │  page_ticket:   Clock 교체 포인터 (atomic 순환)              │   │
│  │  prp1[]/prp2[]: 각 슬롯의 NVMe PRP 물리 주소                │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────┐   ┌─────────────────────┐                 │
│  │  CUDA 커널 스레드들  │   │   NVMe 큐 (GPU 메모리) │              │
│  │                     │   │   ┌─────────────┐    │                │
│  │  seq_read(i) ───────┼──→│   │ SQ (64B엔트리)│   │                │
│  │  bam_ptr[i]  ───────┼──→│   │ 1024개 슬롯  │    │                │
│  │                     │   │   └──────┬──────┘    │                │
│  │  acquire_page() →   │   │          │ doorbell   │                │
│  │  find_slot() →      │   │          ▼           │                │
│  │  read_data() →      │   │   ┌─────────────┐    │                │
│  │  sq_enqueue() ──────┼──→│   │ CQ (16B엔트리)│   │                │
│  │  cq_poll()   ←──────┼───│   │ 4096개 슬롯  │    │                │
│  └─────────────────────┘   │   └─────────────┘    │                │
│                             └─────────────────────┘                 │
└────────────────────────────────────┬────────────────────────────────┘
                                     │ PCIe Bus
                                     │  ├─ SQ 도어벨: GPU → NVMe (MMIO st.mmio)
                                     │  ├─ CQ 도어벨: GPU → NVMe (MMIO st.mmio)
                                     │  ├─ DMA Read:  NVMe → GPU 캐시 (NVMe가 발행)
                                     │  └─ DMA Write: NVMe ← GPU 캐시 (write-back)
                                     │
                              ┌──────┴──────┐
                              │  NVMe SSD   │
                              │  (BAR0: 도어벨│
                              │   레지스터)   │
                              └─────────────┘

┌────────────────────────────────────────────────────────────────┐
│                    호스트 (CPU + 커널)                          │
│                                                                │
│  ┌───────────────────┐    ┌──────────────────────┐            │
│  │ libnvm.ko         │    │ 사용자 공간 라이브러리 │            │
│  │ (커널 모듈)        │    │ (src/)               │            │
│  │                   │    │                      │            │
│  │ ioctl:            │    │ nvm_ctrl_init()      │            │
│  │  호스트 DMA 매핑   │◄──│ nvm_admin_*()        │            │
│  │  GPU P2P 매핑      │    │ nvm_dma_map_device() │            │
│  │ mmap:             │    │                      │            │
│  │  BAR0 레지스터 노출 │    │ Controller 클래스     │            │
│  └───────────────────┘    │ QueuePair 클래스      │            │
│                            │ page_cache_t 클래스   │            │
│  /dev/libnvm0              └──────────────────────┘            │
│  /dev/libnvm1  ...                                             │
└────────────────────────────────────────────────────────────────┘
```

### 2.1 컨트롤러 초기화 → GPU I/O 발행까지의 전체 흐름

```
[호스트 초기화 단계]
1. open("/dev/libnvm0") → 커널 모듈 캐릭터 디바이스 열기
2. nvm_ctrl_init() → ioctl로 BAR0 크기 조회 + mmap으로 BAR0 매핑
3. nvm_aq_create() → 컨트롤러 리셋 + Admin Queue 설정
4. nvm_admin_ctrl_info() → Identify Controller
5. nvm_admin_ns_info() → Identify Namespace (LBA 크기 등)
6. cudaHostRegister(BAR0, cudaHostRegisterIoMemory) → GPU가 도어벨 접근 가능
7. nvm_admin_cq_create() + nvm_admin_sq_create() → I/O 큐 생성 (GPU 메모리에)
8. cudaHostGetDevicePointer(doorbell) → 도어벨의 GPU 디바이스 포인터 획득
9. cudaMalloc() + nvm_dma_map_device() → GPU 캐시 메모리 DMA 매핑
10. page_cache_t 생성 → PRP 테이블 구성, 캐시 메타데이터 GPU 복사

[GPU 런타임 단계]
11. CUDA 커널에서 da->seq_read(i) 호출
12. → find_range(i) → get_page(i) → coalesce_page()
13. → acquire_page() → 상태 머신 (INVALID→BUSY→VALID)
14. → 미스 시: find_slot() → read_data() → sq_enqueue() → cq_poll()
15. → 히트 시: 캐시 슬롯 주소 즉시 반환
```

---

## 3. NVMe 기본 개념 (BaM 이해를 위한 사전 지식)

### 3.1 NVMe 큐 구조

NVMe는 호스트와 컨트롤러 간 통신을 위해 큐 쌍(SQ + CQ)을 사용한다.

```
호스트 메모리 (또는 GPU 메모리):
┌──────────────────────────────────────────┐
│ Submission Queue (SQ)                     │
│ ┌────────┬────────┬────────┬─── ─ ─ ─┐  │
│ │ Entry 0│ Entry 1│ Entry 2│  ...     │  │  각 엔트리 = 64바이트
│ │ (cmd)  │ (cmd)  │ (cmd)  │          │  │  = nvm_cmd_t (16 DWORD)
│ └────────┴────────┴────────┴─── ─ ─ ─┘  │
│           ↑ head (컨트롤러가 읽는 위치)    │
│                    ↑ tail (호스트가 쓰는 위치)│
└──────────────────────────────────────────┘

┌──────────────────────────────────────────┐
│ Completion Queue (CQ)                     │
│ ┌────────┬────────┬────────┬─── ─ ─ ─┐  │
│ │ Entry 0│ Entry 1│ Entry 2│  ...     │  │  각 엔트리 = 16바이트
│ │ (cpl)  │ (cpl)  │ (cpl)  │          │  │  = nvm_cpl_t (4 DWORD)
│ └────────┴────────┴────────┴─── ─ ─ ─┘  │
│      ↑ head (호스트가 읽는 위치)           │
│               ↑ tail (컨트롤러가 쓰는 위치) │
└──────────────────────────────────────────┘

NVMe 컨트롤러 레지스터 (BAR0):
┌──────────────────────────────────────────────────────┐
│ Offset 0x0000: CAP (Controller Capabilities, 8바이트) │
│ Offset 0x0008: VS  (Version, 4바이트)                 │
│ Offset 0x0014: CC  (Controller Configuration)         │
│ Offset 0x001C: CSTS (Controller Status)               │
│ Offset 0x1000: SQ0 Tail Doorbell (Admin SQ)           │
│ Offset 0x1000 + (2y * (4 << DSTRD)):   SQy Tail DB   │
│ Offset 0x1000 + ((2y+1) * (4 << DSTRD)): CQy Head DB │
└──────────────────────────────────────────────────────┘
```

### 3.2 NVMe 커맨드 구조 (64바이트)

BaM에서 `nvm_cmd_t`는 16개의 DWORD(각 4바이트)로 구성된다:

```c
typedef struct __align__(64) {
    uint32_t dword[16];   // 총 64바이트
} __attribute__((aligned (64))) nvm_cmd_t;
```

| DWORD | 필드 | 설명 |
|-------|------|------|
| 0 | CID(31:16), PSDT(15:14), Opcode(7:0) | 커맨드 ID, 데이터 전송 모드, opcode |
| 1 | NSID | 네임스페이스 ID |
| 2-5 | 예약 | (사용 안 함) |
| 6-7 | PRP1 (64비트) | 첫 번째 데이터 페이지의 물리 주소 |
| 8-9 | PRP2 (64비트) | 두 번째 페이지 주소 또는 PRP 리스트 주소 |
| 10-11 | Starting LBA (64비트) | 시작 논리 블록 주소 |
| 12 | NLB(15:0) | 전송할 블록 수 - 1 (0-based) |
| 13-15 | 예약/커맨드별 필드 | DSM, 보호 정보 등 |

BaM 코드에서 커맨드를 구성하는 핵심 함수 3가지:

```c
// DWORD0(CID+opcode)과 DWORD1(NSID)을 설정
nvm_cmd_header(&cmd, cid, NVM_IO_READ, namespace_id);

// DWORD6-9에 PRP1, PRP2를 설정 (데이터가 전송될 물리 주소)
nvm_cmd_data_ptr(&cmd, prp1_addr, prp2_addr);

// DWORD10-12에 시작 LBA와 블록 수를 설정
nvm_cmd_rw_blks(&cmd, start_lba, n_blocks);
```

### 3.3 NVMe I/O 커맨드 Opcode

```c
enum nvm_io_command_set {
    NVM_IO_FLUSH        = 0x00,  // 캐시 플러시
    NVM_IO_WRITE        = 0x01,  // 호스트→SSD 데이터 쓰기
    NVM_IO_READ         = 0x02,  // SSD→호스트 데이터 읽기
    NVM_IO_WRITE_ZEROES = 0x08   // 제로 기록
};
```

### 3.4 PRP (Physical Region Page)

NVMe 데이터 전송 시 물리 주소를 지정하는 메커니즘이다:

- **1 페이지 전송**: PRP1만 사용
- **2 페이지 전송**: PRP1 + PRP2 직접 사용
- **3+ 페이지 전송**: PRP1 + PRP2(PRP 리스트의 물리 주소), PRP 리스트 내에 나머지 페이지 주소 나열

BaM에서는 `page_cache_t` 생성 시 각 캐시 슬롯의 PRP1/PRP2 주소를 미리 계산하여 GPU 메모리에 저장한다. GPU 커널이 `read_data()`를 호출할 때 슬롯 번호만으로 바로 PRP 주소를 참조한다.

---

## 4. 커널 모듈 (module/)

### 4.1 모듈 개요

`libnvm.ko` 커널 모듈은 3가지 핵심 역할을 수행한다:

1. **NVMe PCI 디바이스를 소유**: 커널의 기본 nvme 드라이버 대신 이 모듈이 NVMe 디바이스를 바인딩한다
2. **DMA 매핑 서비스**: 사용자 공간 호스트/GPU 메모리를 NVMe 컨트롤러가 접근 가능한 버스 주소로 변환한다
3. **BAR0 레지스터 노출**: mmap을 통해 NVMe 레지스터를 사용자 공간에 직접 노출한다

### 4.2 PCI 드라이버 등록 (pci.c)

```c
// NVMe 클래스 코드: 01(스토리지) 08(NVMe) 02(NVM 서브클래스)
#define PCI_CLASS_NVME      0x010802

// PCI 디바이스 ID 테이블: NVMe 클래스 디바이스를 모두 매칭
static const struct pci_device_id id_table[] = {
    { PCI_DEVICE_CLASS(PCI_CLASS_NVME, PCI_CLASS_NVME_MASK) },
    { 0 }  // sentinel
};

static struct pci_driver driver = {
    .name = "libnvm helper",
    .id_table = id_table,
    .probe = add_pci_dev,     // 디바이스 발견 시 호출
    .remove = remove_pci_dev, // 디바이스 제거 시 호출
};
```

**모듈 로드 순서** (`libnvm_helper_entry`):

```
1. list_init(&ctrl_list)    ← 컨트롤러 연결 리스트 초기화
   list_init(&host_list)    ← 호스트 DMA 매핑 리스트 초기화
   list_init(&device_list)  ← GPU DMA 매핑 리스트 초기화
2. alloc_chrdev_region()    ← 캐릭터 디바이스 major 번호 동적 할당
3. class_create()           ← sysfs 디바이스 클래스 생성 (udev가 /dev 노드 자동 생성)
4. pci_register_driver()    ← PCI 서브시스템에 드라이버 등록
   → 이미 존재하는 NVMe 디바이스마다 add_pci_dev() 즉시 호출
```

**PCI probe 콜백** (`add_pci_dev`):

```c
static int add_pci_dev(struct pci_dev* dev, const struct pci_device_id* id) {
    // 1. 컨트롤러 구조체 생성하여 전역 리스트에 삽입
    ctrl = ctrl_get(&ctrl_list, dev_class, dev, curr_ctrls);

    // 2. BAR0 메모리 영역을 이 드라이버가 독점 사용하도록 예약
    pci_request_region(dev, 0, DRIVER_NAME);

    // 3. PCI 디바이스를 활성화 (I/O 및 메모리 접근 가능하게)
    pci_enable_device(dev);

    // 4. 캐릭터 디바이스 파일 생성 (/dev/libnvm0, /dev/libnvm1, ...)
    ctrl_chrdev_create(ctrl, dev_first, &dev_fops);

    // 5. PCI 버스 마스터링 활성화 (디바이스가 DMA를 수행하려면 필수)
    pci_set_master(dev);
}
```

### 4.3 캐릭터 디바이스와 파일 연산

```c
static const struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = map_ioctl,   // DMA 매핑/해제 요청 처리
    .mmap = mmap_registers,        // BAR0 레지스터 노출
};
```

### 4.4 ioctl 인터페이스: DMA 매핑 요청 처리

사용자 공간과 커널 모듈 간 통신을 위한 ioctl 구조체와 명령 코드:

```c
// ioctl 요청 구조체 (src/linux/ioctl.h)
struct nvm_ioctl_map {
    uint64_t    vaddr_start;  // 매핑할 가상 주소 시작
    size_t      n_pages;      // 매핑할 페이지 수
    uint64_t*   ioaddrs;      // 커널이 DMA 버스 주소를 기록할 배열
};

// ioctl 명령 코드
enum nvm_ioctl_type {
    NVM_MAP_HOST_MEMORY   = _IOW(0x80, 1, struct nvm_ioctl_map),  // 호스트 메모리 매핑
    NVM_MAP_DEVICE_MEMORY = _IOW(0x80, 2, struct nvm_ioctl_map),  // GPU 메모리 매핑
    NVM_UNMAP_MEMORY      = _IOW(0x80, 3, uint64_t),              // 매핑 해제
};
```

**호스트 메모리 DMA 매핑 과정** (`NVM_MAP_HOST_MEMORY`):

```
1. copy_from_user(&request, arg)     ← 사용자 공간에서 요청 구조체 복사
2. map_userspace()
   ├── create_descriptor()           ← map 구조체 할당 (가변 길이 addrs[] 포함)
   ├── get_user_pages(vaddr, n_pages, FOLL_WRITE, pages)
   │   └── 사용자 가상 주소 → 물리 페이지 pin (스왑 아웃 방지)
   ├── dma_map_page(dev, pages[i], PAGE_SIZE, DMA_BIDIRECTIONAL)
   │   └── 각 물리 페이지 → PCI 디바이스 관점 버스 주소 변환
   │       (IOMMU 있으면 IOMMU 매핑, 없으면 물리 주소 직접 사용)
   └── list_insert(&host_list)       ← 매핑 리스트에 삽입
3. copy_to_user(request.ioaddrs, map->addrs)  ← DMA 주소 배열을 사용자 공간으로 반환
```

**GPU 메모리 DMA 매핑 과정** (`NVM_MAP_DEVICE_MEMORY`, `#ifdef _CUDA`):

```
1. copy_from_user(&request, arg)
2. map_device_memory()
   ├── create_descriptor()           ← GPU 페이지 크기 = 64KB (GPU_PAGE_SIZE)
   ├── nvidia_p2p_get_pages(vaddr, size, &pages, force_release_callback)
   │   └── NVIDIA P2P API: GPU 가상 주소를 pin하고 GPU 페이지 테이블 획득
   │       콜백 등록: GPU 컨텍스트 소멸 시 강제 해제
   ├── 모든 NVMe 컨트롤러에 대해:
   │   nvidia_p2p_dma_map_pages(ctrl->pdev, pages, &mapping)
   │   └── 각 PCI 디바이스가 GPU 메모리에 직접 DMA할 수 있는 주소 생성
   │       (PCIe P2P: NVMe 컨트롤러 → GPU 메모리 직접 전송)
   └── list_insert(&device_list)
3. copy_to_user(request.ioaddrs, map->addrs)  ← 첫 번째 컨트롤러의 DMA 주소 반환
```

핵심 차이점:
- 호스트 메모리: `get_user_pages()` + `dma_map_page()` → 페이지 크기 4KB
- GPU 메모리: `nvidia_p2p_get_pages()` + `nvidia_p2p_dma_map_pages()` → 페이지 크기 64KB
- GPU 매핑은 **모든** NVMe 컨트롤러에 대해 P2P DMA 매핑을 생성한다 (어떤 SSD에서든 GPU로 직접 DMA 가능)

### 4.5 mmap: BAR0 레지스터 노출

```c
static int mmap_registers(struct file* file, struct vm_area_struct* vma) {
    ctrl = ctrl_find_by_inode(&ctrl_list, file->f_inode);

    // 캐시를 비활성화하여 MMIO 레지스터에 대한 정확한 읽기/쓰기 보장
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    // PCI BAR0의 물리 주소를 사용자 공간 VMA에 매핑
    return vm_iomap_memory(vma, pci_resource_start(ctrl->pdev, 0), size);
}
```

이렇게 매핑된 BAR0 영역에는 NVMe 레지스터(CAP, VS, CC, CSTS)와 도어벨 레지스터가 포함된다. 사용자 공간에서 이 영역을 읽어 컨트롤러 속성을 확인하고, 도어벨에 직접 쓸 수 있다.

### 4.6 ctrl 구조체와 캐릭터 디바이스

```c
struct ctrl {
    struct list_node    list;       // 컨트롤러 연결 리스트 노드
    struct pci_dev*     pdev;       // PCI 디바이스 참조
    char                name[64];   // "libnvm0" 등
    int                 number;     // 컨트롤러 번호 (0~)
    dev_t               rdev;       // 캐릭터 디바이스 번호 (major + minor)
    struct class*       cls;        // sysfs 디바이스 클래스
    struct cdev         cdev;       // 커널 캐릭터 디바이스
    struct device*      chrdev;     // /dev 노드 디바이스 핸들
};
```

캐릭터 디바이스 생성 순서: `cdev_init(fops)` → `cdev_add(커널 등록)` → `device_create(/dev 노드 생성)`

### 4.7 map 구조체

```c
struct map {
    struct list_node    list;           // 매핑 연결 리스트 노드
    struct task_struct* owner;          // 매핑 소유 프로세스 (current)
    u64                 vaddr;          // 매핑 시작 가상 주소
    struct list*        ctrl_list;      // GPU 매핑에서 사용: 모든 컨트롤러 리스트
    struct pci_dev*     pdev;           // 연관 PCI 디바이스
    unsigned long       page_size;      // 호스트: 4KB, GPU: 64KB
    void*               data;           // 호스트: struct page** 배열, GPU: gpu_region*
    release             release;        // 해제 콜백 (release_user_pages 또는 release_gpu_memory)
    unsigned long       n_addrs;        // DMA 주소 개수
    uint64_t            addrs[1];       // 가변 길이 DMA 버스 주소 배열
};
```

### 4.8 리스트 관리 (list.c/h)

스핀락으로 보호되는 순환 이중 연결 리스트이다. head는 더미 노드(sentinel)로 실제 데이터를 담지 않으며, 삽입/삭제 시 경계 조건 처리를 단순화한다.

```c
struct list {
    struct list_node    head;   // 더미 노드 (빈 리스트: head.next = head.prev = &head)
    spinlock_t          lock;   // 동시 접근 보호
};

// 삽입: tail append (head.prev 뒤에 삽입)
void list_insert(struct list* list, struct list_node* element) {
    spin_lock(&list->lock);
    last = list->head.prev;
    last->next = element;
    element->prev = last;
    element->next = &list->head;
    list->head.prev = element;
    spin_unlock(&list->lock);
}
```

---

## 5. 사용자 공간 라이브러리 (src/ + include/)

### 5.1 nvm_ctrl_t: 컨트롤러 핸들

```c
typedef struct {
    size_t      page_size;   // 컨트롤러 페이지 크기 (MPS, CC 레지스터에서 결정)
    uint8_t     dstrd;       // 도어벨 스트라이드 (실제 간격 = 4 << dstrd 바이트)
    uint64_t    timeout;     // 컨트롤러 타임아웃 (밀리초, CAP.TO에서 읽음)
    uint32_t    max_qs;      // 최대 큐 엔트리 수 (CAP.MQES + 1)
    size_t      mm_size;     // BAR0 MMIO 영역 전체 크기
    volatile void* mm_ptr;   // BAR0 메모리 맵 포인터 (레지스터 직접 접근)
} nvm_ctrl_t;
```

**초기화 흐름** (`nvm_ctrl_init`):

```
1. ioctl(fd, ...) → 커널 모듈에서 BAR0 크기 조회
2. mmap(fd, BAR0_SIZE) → BAR0 레지스터를 사용자 공간에 매핑
3. CAP 레지스터 읽기 → page_size, dstrd, timeout, max_qs 추출
```

### 5.2 nvm_dma_t: DMA 매핑 디스크립터

```c
typedef struct {
    void*       vaddr;          // 영역 시작 가상 주소 (GPU 메모리이면 NULL 가능)
    int8_t      local;          // 로컬 메모리 여부 (1: 호스트, 0: 리모트/GPU)
    int8_t      contiguous;     // 물리 메모리 연속 여부
    size_t      page_size;      // 컨트롤러 페이지 크기 (MPS)
    size_t      n_ioaddrs;      // MPS 단위 페이지 개수
    uint64_t    ioaddrs[];      // 각 페이지의 물리/IO 주소 배열 (유연한 배열 멤버)
} nvm_dma_t;
```

호스트 OS 페이지(4KB)와 NVMe 컨트롤러 페이지(MPS)가 다를 수 있으므로, `nvm_dma_map()` 함수가 자동으로 오프셋을 계산한다.

### 5.3 nvm_queue_t: 큐 디스크립터

BaM의 `nvm_queue_t`는 기본 NVMe 큐 정보 외에 GPU 병렬 접근을 위한 다양한 동기화 필드를 포함한다:

```c
typedef struct {
    // === GPU 스레드 동기화 필드 (각 32바이트 패딩으로 false sharing 방지) ===
    atomic<uint32_t> head_lock;    // 큐 head 접근 락
    atomic<uint32_t> tail_lock;    // 큐 tail 접근 락
    atomic<uint32_t> head;         // 현재 head 인덱스
    atomic<uint32_t> tail;         // 현재 tail 인덱스
    atomic<uint32_t> in_ticket;    // 인큐 순서 번호 발급 카운터
    atomic<uint32_t> cid_ticket;   // CID 할당 순서 번호 카운터

    padded_struct* tickets;     // 슬롯별 세대 번호 (ticket lock)
    padded_struct* head_mark;   // head 이동 마킹 배열
    padded_struct* tail_mark;   // tail 이동 마킹 배열
    padded_struct* cid;         // 65536개 CID 사용 상태
    padded_struct* pos_locks;   // CQ 위치별 잠금

    uint32_t qs_minus_1;   // 큐 크기 - 1 (비트 AND 최적화)
    uint32_t qs_log2;      // log2(큐 크기) (시프트 최적화)

    // === 기본 NVMe 큐 정보 ===
    uint16_t    no;         // 큐 번호
    uint16_t    es;         // 엔트리 크기 (SQ=64, CQ=16)
    uint32_t    qs;         // 큐 크기 (엔트리 수)
    int8_t      phase;      // 현재 위상 태그 (0 또는 1)
    volatile uint32_t* db;  // 도어벨 레지스터 포인터 (GPU 디바이스 포인터)
    volatile void* vaddr;   // 큐 메모리 가상 주소
    uint64_t    ioaddr;     // 큐 메모리 물리/IO 주소
} nvm_queue_t;
```

### 5.4 NVMe Admin 커맨드

BaM이 지원하는 Admin 커맨드:

| 함수 | Admin Opcode | 설명 |
|------|-------------|------|
| `nvm_admin_ctrl_info()` | Identify(06h), CNS=1 | 컨트롤러 정보 조회 (모델, MDTS, MQES 등) |
| `nvm_admin_ns_info()` | Identify(06h), CNS=0 | 네임스페이스 정보 (크기, LBA 블록 크기) |
| `nvm_admin_set_num_queues()` | Set Features(09h), FID=7 | I/O 큐 수 요청 |
| `nvm_admin_get_num_queues()` | Get Features(0Ah), FID=7 | 할당된 큐 수 조회 |
| `nvm_admin_cq_create()` | Create I/O CQ(05h) | Completion Queue 생성 |
| `nvm_admin_sq_create()` | Create I/O SQ(01h) | Submission Queue 생성 |

### 5.5 컨트롤러 정보 구조체

```c
struct nvm_ctrl_info {
    uint32_t    nvme_version;    // NVMe 버전 (예: 0x00010300 = 1.3.0)
    size_t      page_size;       // MPS (보통 4KB)
    size_t      db_stride;       // 도어벨 스트라이드 (바이트)
    uint64_t    timeout;         // 타임아웃 (ms)
    int         contiguous;      // CQR: 큐 물리 연속 필요 여부
    uint16_t    max_entries;     // MQES: 최대 큐 엔트리 수
    char        serial_no[20];   // 시리얼 번호
    char        model_no[40];    // 모델 번호
    size_t      max_data_size;   // MDTS: 최대 데이터 전송 크기 (바이트)
    size_t      sq_entry_size;   // 보통 64바이트
    size_t      cq_entry_size;   // 보통 16바이트
};

struct nvm_ns_info {
    uint32_t    ns_id;           // 네임스페이스 ID (1~)
    size_t      size;            // 논리 블록 수 (NSZE)
    size_t      lba_data_size;   // LBA 크기 (보통 512 또는 4096바이트)
};
```

---

## 6. GPU 페이지 캐시 (page_cache.h) - BaM의 핵심

`page_cache.h`는 BaM에서 가장 중요한 파일이다. GPU DRAM에 소프트웨어 관리 페이지 캐시를 구현하여, NVMe SSD 데이터를 투명하게 캐싱한다.

### 6.1 전체 구조

```
┌─────────────────────────────────────────────────────────────┐
│                  page_cache_d_t (GPU 메모리)                 │
│                                                             │
│  base_addr ──→ ┌──────────────────────────────────────────┐ │
│                │  캐시 데이터 영역 (DMA 매핑됨)              │ │
│                │  ┌────────┬────────┬────────┬─── ─ ─ ─┐  │ │
│                │  │ Slot 0 │ Slot 1 │ Slot 2 │   ...   │  │ │
│                │  │ (ps B) │ (ps B) │ (ps B) │         │  │ │
│                │  └────────┴────────┴────────┴─── ─ ─ ─┘  │ │
│                └──────────────────────────────────────────┘ │
│                                                             │
│  cache_pages[] ──→ 캐시 슬롯 메타데이터 (n_pages개)          │
│    [0]: page_take_lock=FREE,  page_translation=0            │
│    [1]: page_take_lock=UNLOCKED, page_translation=0x0003    │
│    ...                                                      │
│                                                             │
│  prp1[] ──→ 각 슬롯의 NVMe PRP1 물리 주소                   │
│  prp2[] ──→ 각 슬롯의 NVMe PRP2 물리 주소 (필요 시)          │
│                                                             │
│  page_ticket ──→ Clock 교체 포인터 (atomic 증가)             │
│                                                             │
│  ranges[] ──→ range별 data_page_t 배열 포인터                │
│    ranges[0] → [data_page_t, data_page_t, ...]              │
│    ranges[1] → [data_page_t, data_page_t, ...]              │
│                                                             │
│  d_ctrls[] ──→ NVMe 컨트롤러 GPU 포인터 배열                 │
│    d_ctrls[0] → Controller (GPU 복사본)                      │
│    d_ctrls[1] → Controller (GPU 복사본)                      │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 핵심 자료 구조

#### data_page_t: SSD 페이지 상태 추적

```c
typedef struct __align__(32) {
    atomic<uint32_t> state;  // 상태 비트필드 + 참조 카운트
    uint32_t offset;         // 매핑된 캐시 슬롯 번호
} data_page_t;
```

`state` 필드의 비트 구조 (32비트):

```
비트 31: VALID  (0x80000000) - 데이터가 캐시에 유효하게 로드됨
비트 30: BUSY   (0x40000000) - SSD에서 데이터 로드 중
비트 29: DIRTY  (0x20000000) - 수정됨 (eviction 시 write-back 필요)
비트 28~0: 참조 카운트 (CNT_MASK = 0x1fffffff) - 사용 중인 스레드 수
```

상태 전이 다이어그램:

```
┌───────────────┐  fetch_or(BUSY)   ┌───────────────┐  read완료+XOR    ┌───────────────┐
│   INVALID     │ ────────────────→ │    BUSY       │ ──────────────→ │    VALID      │
│ state=0x0000  │                   │ state=0x4000  │                 │ state=0x8000  │
│ (NV_NB)       │                   │ (NV_B)        │                 │ (V_NB)        │
└───────┬───────┘                   └───────────────┘                 └───────┬───────┘
        │                                                                     │
        │  eviction                                              쓰기 시:     │
        │  (find_slot)                                   fetch_or(DIRTY)      │
        └──────────────── ← ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ──┘

상태 전환 XOR 마스크: DISABLE_BUSY_ENABLE_VALID = 0xC0000000
  BUSY(0x40000000) XOR 0xC0000000 = VALID(0x80000000)
```

#### cache_page_t: 캐시 슬롯 메타데이터

```c
typedef struct __align__(32) {
    atomic<uint32_t> page_take_lock;  // FREE(2) / LOCKED(1) / UNLOCKED(0)
    uint64_t page_translation;         // (page_offset << n_ranges_bits) | range_id
} cache_page_t;
```

- `FREE(2)`: 한 번도 사용되지 않은 빈 슬롯
- `LOCKED(1)`: eviction 또는 매핑 변경 중
- `UNLOCKED(0)`: 매핑되어 있지만 잠기지 않음 (eviction 후보)

`page_translation`에서 range_id와 page_offset 추출:

```c
range_id     = page_translation & n_ranges_mask;        // 하위 비트
page_offset  = page_translation >> n_ranges_bits;       // 상위 비트
```

### 6.3 페이지 캐시 접근 흐름 (GPU 커널에서)

가장 일반적인 접근 패턴은 `array_d_t<T>::seq_read(i)`:

```
seq_read(i):
  1. find_range(i)          ← i가 속하는 range를 선형 탐색
  2. get_page(i)            ← 원소 인덱스 → 페이지 번호 변환
     index = ((i - index_start) * sizeof(T) + page_start_offset) >> page_size_log
  3. get_subindex(i)        ← 페이지 내 바이트 오프셋 계산
     subindex = ((i - index_start) * sizeof(T) + page_start_offset) & page_size_minus_1
  4. get_global_address(page) ← 전역 주소 인코딩
     gaddr = (page << n_ranges_bits) | range_id
  5. coalesce_page()        ← warp-level coalescing (핵심 최적화)
     ├── __match_any_sync(mask, gaddr) → 같은 페이지 접근 스레드 그룹화
     ├── master = __ffs(eq_mask) - 1   → 그룹 마스터 선출
     ├── count = __popc(eq_mask)        → 그룹 크기 (참조 카운트 증가량)
     ├── [master만] acquire_page(page, count, write=false, ctrl, queue)
     │   └── 상태 머신 구동 (아래 상세 설명)
     └── __shfl_sync(eq_mask, base_master, master)  → 캐시 주소 브로드캐스트
  6. T ret = *(T*)(cache_page_addr + subindex)  ← 데이터 읽기
  7. __syncwarp(eq_mask)
  8. [master만] release_page(page, count)  ← 참조 카운트 감소
```

### 6.4 acquire_page: 페이지 획득 상태 머신

```c
uint64_t acquire_page(pg, count, write, ctrl_, queue) {
    // 참조 카운트를 먼저 증가 (acquire 순서)
    read_state = pages[pg].state.fetch_add(count, memory_order_acquire);

    do {
        st = (read_state >> 30) & 0x03;  // 상위 2비트 추출

        switch (st) {
        case NV_NB:  // INVALID: 캐시에 없음
            // BUSY 비트를 설정하여 로드 권한 획득 시도
            st_new = pages[pg].state.fetch_or(BUSY, acquire);
            if ((st_new & BUSY) == 0) {
                // 성공: 이 스레드가 데이터를 로드한다
                page_trans = cache.find_slot(pg, range_id, queue);  // 캐시 슬롯 할당
                ctrl = get_backing_ctrl(pg);                         // 담당 컨트롤러 결정
                b_page = get_backing_page(pg);                       // SSD 페이지 번호 계산
                read_data(&cache, qp, b_page * n_blocks, n_blocks, page_trans);  // SSD 읽기
                pages[pg].offset = page_trans;                       // 슬롯 번호 저장
                pages[pg].state.fetch_xor(0xC0000000, release);      // BUSY→VALID 전환
                return page_trans;
            }
            break;

        case V_NB:   // VALID: 캐시 히트!
            if (write) pages[pg].state.fetch_or(DIRTY, relaxed);
            return pages[pg].offset;   // 즉시 반환

        case NV_B:   // BUSY: 다른 스레드가 로드 중
        case V_B:    // VALID+BUSY: 경합 상태
            __nanosleep(ns);           // 지수 백오프로 대기
            ns = min(ns * 2, 256);
            read_state = pages[pg].state.load(acquire);  // 상태 재확인
            break;
        }
    } while (fail);
}
```

### 6.5 find_slot: 캐시 슬롯 할당 (Clock 교체 정책)

`find_slot`은 새 SSD 페이지를 위한 캐시 슬롯을 찾는다:

1. `page_ticket`을 atomic 증가시켜 다음 후보 슬롯을 순환 선택한다 (Clock/FIFO)
2. 해당 슬롯의 `page_take_lock`을 확인한다:
   - `FREE(2)`: 빈 슬롯이므로 즉시 사용 → LOCKED(1)로 설정
   - `UNLOCKED(0)`: 이전 매핑이 있지만 잠기지 않음 → eviction 시도
     - 이전 data_page_t의 참조 카운트가 0이어야 eviction 가능
     - DIRTY 비트가 설정되어 있으면 SSD에 write-back 수행
     - 이전 data_page_t의 state를 INVALID로 리셋
   - `LOCKED(1)`: 다른 스레드가 사용 중 → 다음 슬롯으로 넘어감
3. 성공하면 새 `page_translation`을 기록하고 `UNLOCKED(0)`으로 설정

### 6.6 페이지 교체 정책

BaM의 캐시 교체는 **Clock 알고리즘** 변형이다:

```c
// page_ticket: atomic<uint32_t>, 전역 순환 포인터
uint32_t slot = page_ticket->fetch_add(1) % n_pages;
// → slot 위치에서 시작하여 빈 슬롯 또는 eviction 가능 슬롯을 탐색
```

참조 카운트(`state`의 하위 29비트)가 0인 슬롯만 eviction 대상이 된다. 사용 중인 페이지(참조 카운트 > 0)는 건너뛴다.

### 6.7 range_d_t: 데이터 범위

```c
template <typename T>
struct range_d_t {
    uint64_t index_start;        // 배열 시작 인덱스
    uint64_t count;              // 배열 끝 인덱스 (배타적)
    uint64_t range_id;           // 캐시 내 고유 번호
    uint64_t page_start;         // SSD 시작 페이지 번호
    uint64_t page_count;         // 총 페이지 수
    uint64_t page_size;          // 캐시 페이지 크기
    size_t n_elems_per_page;     // 페이지당 T 원소 수
    data_dist_t dist;            // REPLICATE 또는 STRIPE
    pages_t pages;               // data_page_t 배열 (page_count개)
    page_cache_d_t cache;        // 소속 캐시 메타데이터 복사본
};
```

#### 데이터 분배 방식

```c
enum data_dist_t { REPLICATE = 0, STRIPE = 1 };
```

- **REPLICATE**: 모든 NVMe 컨트롤러에 동일 데이터를 복제. 읽기 시 라운드로빈으로 컨트롤러 선택. 쓰기 시 모든 컨트롤러에 기록.
- **STRIPE**: 페이지를 컨트롤러에 분산. `page_offset % n_ctrls`로 담당 컨트롤러 결정. 대역폭이 n_ctrls배 증가.

### 6.8 array_d_t: 타입 안전 배열 추상화

```c
template<typename T>
struct array_d_t {
    uint64_t n_elems;           // 전체 원소 수
    uint64_t n_ranges;          // range 수
    range_d_t<T>* d_ranges;     // range 배열 (GPU 메모리)

    T seq_read(size_t i);       // 인덱스 i의 값을 읽기
    void seq_write(size_t i, T val);  // 인덱스 i에 값 쓰기
    T operator[](size_t i);     // seq_read의 연산자 버전
    void operator()(size_t i, T val); // seq_write의 연산자 버전
    T AtomicAdd(size_t i, T val);     // atomic 덧셈
};
```

### 6.9 bam_ptr: 스마트 포인터

TLB 없이 직접 page_cache에 접근하는 스마트 포인터:

```c
template<typename T>
struct bam_ptr {
    data_page_t* page;      // 현재 data_page_t (참조 카운트 관리)
    array_d_t<T>* array;    // 소속 배열
    size_t start, end;      // 현재 페이지의 원소 범위
    T* addr;                // 현재 캐시 페이지 GPU 메모리 주소

    T operator[](size_t i) {
        if (i < start || i >= end)
            update_page(i);     // 범위 밖이면 새 페이지 acquire
        return addr[i - start]; // 캐시 데이터 직접 접근
    }

    T& operator[](size_t i) {   // 쓰기 버전: DIRTY 비트 자동 설정
        if (i < start || i >= end) {
            update_page(i);
            page->state.fetch_or(DIRTY, relaxed);
        }
        return addr[i - start];
    }
};
```

### 6.10 TLB (Translation Lookaside Buffer)

TLB를 사용하는 `bam_ptr_tlb`는 최근 접근한 페이지의 캐시 주소를 캐싱하여, 반복 접근 시 page_cache lookup을 건너뛴다:

```c
template<typename T, size_t n = 32>
struct tlb {
    tlb_entry entries[n];  // 직접 매핑 TLB (gid % n)

    T* acquire(i, gid, ...) {
        entry = entries[gid % n];
        // warp 내 같은 gid를 __match_any_sync로 그룹화
        // 마스터만 TLB를 조작, 결과를 __shfl_sync로 브로드캐스트
        // 히트: 참조 카운트만 증가
        // 미스: 이전 매핑 해제 → array->acquire_page_() → 새 매핑
    }
};
```

---

## 7. NVMe 큐 관리 (GPU에서의 직접 큐 접근)

### 7.1 개요

`nvm_parallel_queue.h`는 수천 GPU 스레드가 하나의 NVMe SQ/CQ를 동시에 사용할 때의 lock-free 알고리즘을 구현한다. 핵심 도전: 여러 스레드가 동시에 SQ에 커맨드를 기록하고 도어벨을 울려야 하는데, 도어벨은 **순차적으로 증가**해야 한다 (NVMe 스펙 요구사항).

### 7.2 sq_enqueue: SQ에 NVMe 커맨드 인큐 (핵심 함수)

```
sq_enqueue(sq, cmd) 알고리즘:

[단계 1] 전역 순서 번호 발급
  ticket = sq->in_ticket.fetch_add(1, relaxed)
  pos = ticket & qs_minus_1         // 큐 슬롯 위치
  id = (ticket >> qs_log2) * 2      // 세대 번호

[단계 2] 자기 차례까지 spin-wait (Bakery 알고리즘 변형)
  while (sq->tickets[pos].load() != id)
      __nanosleep(ns);              // 지수 백오프

[단계 3] 큐 메모리에 NVMe 커맨드 복사 (64바이트)
  // ulonglong4(32바이트) 단위로 2회 = 64바이트
  for (i = 0; i < 2; i++)
      queue_loc[i] = cmd_[i];

[단계 4] tail_mark 설정 (기록 완료 표시)
  sq->tail_mark[pos].store(LOCKED, release)

[단계 5] 리더 선출 및 도어벨 업데이트
  while (my tail_mark still LOCKED) {
      if (can grab tail_lock) {
          cur_tail = sq->tail.load()
          count = move_tail(sq, cur_tail)  // 연속 LOCKED 마크 수집
          if (count > 0) {
              new_db = (cur_tail + count) & qs_minus_1
              // PTX 인라인 어셈블리: GPU → PCIe MMIO 도어벨 쓰기
              asm("st.mmio.relaxed.sys.global.u32 [sq->db], new_db")
              sq->tail.store(cur_tail + count, release)
          }
          release tail_lock
      }
  }

[단계 6] tickets[pos] += 1  (디큐 세대 열기)
```

**핵심 인사이트**: 도어벨을 쓰는 비용이 크므로(PCIe MMIO), 리더 스레드가 연속 완료된 엔트리를 모아서 한 번에 도어벨을 업데이트한다. 이를 통해 도어벨 쓰기 횟수를 최소화한다.

### 7.3 cq_poll: CQ 폴링 (완료 검색)

```c
uint32_t cq_poll(cq, search_cid, &loc_, &cq_head) {
    while (true) {
        head = cq->head.load(relaxed);
        // CQ를 head부터 스캔
        for (i = 0; i < qs-1; i++) {
            cur_head = head + i;
            search_phase = (~(cur_head >> qs_log2)) & 0x01;  // 예상 위상
            loc = cur_head & qs_minus_1;
            cpl_entry = ((nvm_cpl_t*)cq->vaddr)[loc].dword[3];
            cid = cpl_entry & 0x0000ffff;                     // Command ID
            phase = (cpl_entry >> 16) & 1;                     // Phase bit

            if (cid == search_cid && phase == search_phase)
                return loc;  // 찾았다!
            if (phase != search_phase)
                break;       // 아직 새 완료 없음
        }
        __nanosleep(ns);  // 지수 백오프
    }
}
```

NVMe 위상(Phase) 비트 메커니즘: CQ가 한 바퀴 돌 때마다 위상이 반전된다. 이를 통해 새 완료와 오래된 완료를 구분한다.

### 7.4 cq_dequeue: CQ 완료 처리

3단계로 동작한다:

```
1단계 - 위치 잠금: pos_locks[pos] 획득 (같은 CQ 슬롯 동시 접근 방지)
2단계 - head_mark 설정 → 리더 선출 → 연속 처리된 CQ 엔트리 수집
        → CQ 도어벨 업데이트 (st.mmio) + SQ head 갱신
3단계 - CQ head가 자신의 논리적 위치를 넘어갈 때까지 대기
        → pos_locks[pos] 해제
```

### 7.5 CID (Command ID) 관리

65536개의 CID를 lock-free로 할당/해제한다:

```c
// 할당
uint16_t get_cid(sq) {
    do {
        id = sq->cid_ticket.fetch_add(1) & 65535;  // 순환
        old = sq->cid[id].val.fetch_or(LOCKED, acquire);
    } while (old == LOCKED);  // 이미 사용 중이면 재시도
    return id;
}

// 해제
void put_cid(sq, id) {
    sq->cid[id].val.store(UNLOCKED, release);
}
```

### 7.6 I/O 깊이(Queue Depth) 관리

BaM에서 큐 깊이는 QueuePair 생성 시 결정된다:

```c
// queue.h의 QueuePair 생성자에서:
sq_size = min(queueDepth, min(MAX_SQ_ENTRIES_64K, MQES+1));
cq_size = min(queueDepth, min(MAX_CQ_ENTRIES_64K, MQES+1));
```

- `MAX_SQ_ENTRIES_64K` = 1024 (64KB / 64B)
- `MAX_CQ_ENTRIES_64K` = 4096 (64KB / 16B)
- CQR(Contiguous Queues Required)이면 64KB로 제한한다

---

## 8. DIS/SmartIO 지원 (원격 PCIe)

### 8.1 Dolphin SmartIO 개요

Dolphin SmartIO는 PCIe 스위치 패브릭을 통해 서로 다른 노드의 PCIe 디바이스에 원격 접근하는 기술이다. BaM은 이를 지원하여 GPU(노드A) → PCIe 패브릭 → NVMe SSD(노드B)로 직접 I/O할 수 있다.

```
┌──────────────┐  PCIe   ┌──────────────────┐  PCIe   ┌──────────────┐
│   노드 A     │ 패브릭  │  PCIe Switch     │ 패브릭  │   노드 B     │
│ ┌──────────┐ │ ←────→ │  (Dolphin)       │ ←────→ │ ┌──────────┐ │
│ │  GPU     │ │        │  SmartIO         │        │ │ NVMe SSD │ │
│ └──────────┘ │        └──────────────────┘        │ └──────────┘ │
└──────────────┘                                     └──────────────┘
```

### 8.2 SISCI API 사용

BaM의 DIS 백엔드(`src/dis/`)는 SISCI(SCI Software Infrastructure) API를 사용한다:

- **세그먼트**: 노드 간 공유 메모리 영역 (로컬 세그먼트, 원격 세그먼트)
- **매핑**: 원격 세그먼트를 로컬 가상 주소에 매핑
- **데이터 인터럽트**: 노드 간 소량 데이터 전송 + 알림

### 8.3 RPC 메커니즘 (src/dis/rpc.c)

NVMe Admin 커맨드는 물리적으로 디바이스를 소유한 노드에서만 실행할 수 있다. 원격 노드가 Admin 커맨드를 실행하려면 RPC를 사용한다:

```
클라이언트(원격 노드)                        서버(디바이스 소유 노드)
  │                                           │
  │  SCITriggerDataInterrupt(명령)  ────────→ │
  │                                           │ nvm_admin_*() 로컬 실행
  │  ← ────────── SCITriggerDataInterrupt(결과)│
  │                                           │
```

### 8.4 원격 NVMe 접근 패턴

```c
// 원격 NVMe 컨트롤러 초기화 (src/dis/device.c)
nvm_dis_ctrl_init(&ctrl, smartio_fdid);
// → SISCI API로 원격 디바이스의 BAR0를 로컬에 매핑
// → 로컬에서 NVMe 레지스터에 직접 접근 가능

// 원격 DMA 매핑 (src/dis/dma.c)
nvm_dis_dma_map_local(&dma, ctrl, adapter, segment, true);
// → 로컬 세그먼트를 역매핑하여 원격 NVMe 컨트롤러가 접근 가능하게 함
```

---

## 9. Linux 로컬 백엔드 (src/linux/)

### 9.1 ioctl 기반 커널 모듈 통신

로컬 NVMe 접근 시 사용자 공간 라이브러리는 `src/linux/` 코드를 통해 커널 모듈과 통신한다:

```c
// src/linux/ioctl.h - 공유 인터페이스
struct nvm_ioctl_map {
    uint64_t    vaddr_start;   // 매핑할 가상 주소
    size_t      n_pages;       // 페이지 수
    uint64_t*   ioaddrs;       // DMA 주소 반환 배열
};

// src/linux/dma.cpp - GPU DMA 매핑
int nvm_dma_map_device(nvm_dma_t** map, const nvm_ctrl_t* ctrl, void* devptr, size_t size) {
    // 1. ioctl(NVM_MAP_DEVICE_MEMORY) → 커널 모듈이 NVIDIA P2P API로 GPU 메모리 DMA 매핑
    // 2. 반환된 DMA 버스 주소로 nvm_dma_t 생성
}
```

### 9.2 BAR0 mmap

```c
// src/linux/device.cpp
int nvm_ctrl_init(nvm_ctrl_t** ctrl, int fd) {
    // 1. mmap(fd, 0) → BAR0 레지스터를 사용자 공간에 매핑
    // 2. CAP 레지스터에서 page_size, dstrd, timeout, max_qs 읽기
    // 3. nvm_ctrl_t 구조체 할당 및 초기화
}
```

---

## 10. 벤치마크 분석 - 그래프 알고리즘

### 10.1 CSR 그래프 표현

```
vertexList[] (offsets):  [0, 3, 5, 8, ...]  ← 크기 = vertex_count + 1
edgeList[]   (edges):    [1, 2, 4, 0, 3, 1, 2, 5, ...]  ← 크기 = edge_count

정점 0의 이웃: edgeList[0..2] = {1, 2, 4}   (vertexList[0]=0 ~ vertexList[1]=3)
정점 1의 이웃: edgeList[3..4] = {0, 3}       (vertexList[1]=3 ~ vertexList[2]=5)
```

BaM에서 `vertexList`는 항상 GPU 메모리에, `edgeList`는 메모리 모드에 따라 GPU/UVM/SSD에 배치된다.

### 10.2 메모리 모드 비교

| 모드 | edgeList 위치 | 메커니즘 | 특징 |
|------|-------------|---------|------|
| `GPUMEM` | GPU DRAM | cudaMalloc | 최고 성능, GPU 메모리 한계 |
| `UVM_READONLY` | UVM (호스트+GPU) | cudaMallocManaged + ReadMostly 힌트 | 자동 마이그레이션 |
| `UVM_DIRECT` | UVM (호스트) | cudaMallocManaged + AccessedBy 힌트 | zero-copy 방식 |
| `BAFS_DIRECT` | NVMe SSD | BaM page_cache | GPU→SSD 직접 I/O |

### 10.3 BFS 커널 변형

**BASELINE**: 정점당 1스레드. 각 스레드가 자기 정점의 인접 리스트를 순차 순회한다.

**COALESCE**: 정점당 1워프(32스레드). 워프 내 스레드들이 인접 리스트를 stride 패턴으로 접근하여 메모리 coalescing을 달성한다.

**COALESCE_CHUNK**: 워프당 CHUNK_SIZE(8)개 정점을 묶어서 처리한다. 작은 정점들을 합쳐서 워프 활용률을 높인다.

**COALESCE_HASH**: 워프-정점 매핑에 stride 기반 해싱을 적용한다. 연속된 정점을 다른 워프에 분산시켜 SSD 페이지 캐시 히트율을 높인다.

**OPTIMIZED**: 캐시라인(타일) 단위로 워프를 할당한다. 같은 SSD 페이지에 속하는 간선들을 하나의 워프가 처리하여 I/O 효율을 극대화한다.

**`*_PC` 변형**: page_cache를 사용하는 버전. `da->seq_read(i)` 또는 `bam_ptr<uint64_t> ptr(da); ptr[i]`로 SSD 데이터에 접근한다.

### 10.4 BFS 호스트 코드 (BAFS_DIRECT 모드)

```cpp
// 1. NVMe 컨트롤러 초기화
Controller** ctrls = new Controller*[n_ctrls];
for (i = 0; i < n_ctrls; i++)
    ctrls[i] = new Controller(ctrls_paths[i], ns, cudaDevice, queueDepth, numQueues);

// 2. GPU 페이지 캐시 생성
page_cache_t h_pc(pageSize, n_pages, cudaDevice, *ctrls[0], max_range, ctrl_vec);

// 3. 데이터 범위 등록 (edgeList 영역)
range_t<uint64_t> edgeList_range(0, numEdges, startPage, pageCount, 0, pageSize, &h_pc, cudaDevice);

// 4. 배열 추상화 생성
array_t<uint64_t> edgeList_array(numEdges, startOffset, edgeList_range, cudaDevice);

// 5. BFS 커널 반복 실행
do {
    kernel<<<blocks, threads>>>(vertexList_d, edgeList_array.d_array_ptr, label_d, changed_d);
} while (changed);
```

### 10.5 SSSP (Single Source Shortest Path)

Bellman-Ford 기반이다. 각 반복에서 모든 간선을 검사하여 `atomicMin`으로 최단 거리를 갱신한다:

```cuda
// 의사 코드
if (dist[src] + weight < dist[dst])
    atomicMin(&dist[dst], dist[src] + weight);
```

### 10.6 PageRank

반복적 수렴 방식이다. 각 반복에서 모든 정점의 기여도를 인접 정점에 `atomicAdd`로 누적한다:

```cuda
// 의사 코드
contribution = pagerank[v] / out_degree[v];
for each neighbor u of v:
    atomicAdd(&new_pagerank[u], contribution);
```

BaM의 `array_d_t<T>::AtomicAdd(i, val)`은 페이지를 acquire한 뒤 캐시 페이지 내에서 `atomicAdd`를 수행하고, DIRTY 비트를 설정한 뒤 release한다.

---

## 11. 벤치마크 분석 - I/O 성능 테스트

### 11.1 block 벤치마크

페이지 캐시를 사용하지 않고, GPU 스레드가 NVMe 블록 I/O를 직접 수행하는 저수준 성능 테스트이다.

```
접근 패턴:
- Sequential: 스레드 i가 블록 i * blk_size부터 순차 읽기
- Random: 각 스레드가 랜덤 LBA에 접근

컨트롤러/큐 선택:
- 다중 NVMe SSD 사용 시 스레드를 라운드로빈으로 분배
- get_smid()로 SM 번호를 얻어 큐를 선택 (SM↔큐 친화성)
```

### 11.2 readwrite 벤치마크

page_cache를 통한 읽기/쓰기 성능을 측정한다. `array_d_t::seq_read()`와 `seq_write()`의 IOPS와 대역폭을 측정한다.

### 11.3 iodepth-block 벤치마크

I/O 큐 깊이에 따른 처리량 변화를 측정한다. 큐 깊이를 1부터 수백까지 변화시키면서 IOPS를 기록한다. 일반적으로 큐 깊이가 증가할수록 NVMe SSD의 내부 병렬성을 활용하여 IOPS가 증가하다가 포화점에 도달한다.

### 11.4 array 벤치마크

page_cache 위 `array_d_t` 추상화의 원소 접근 IOPS를 측정한다. 순차/랜덤 접근 패턴과 다양한 캐시 크기 조합에서의 성능을 비교한다.

---

## 12. 전체 I/O 흐름 (End-to-End)

### 12.1 캐시 미스 시 전체 경로

```
GPU 커널 스레드
  │
  ├─ seq_read(i)
  │   ├─ find_range(i) → range 결정
  │   ├─ get_page(i) → 페이지 번호 계산
  │   ├─ coalesce_page()
  │   │   ├─ __match_any_sync(gaddr) → warp 내 그룹화
  │   │   └─ 마스터만 acquire_page() 호출
  │   │
  │   ├─ acquire_page(page, count)
  │   │   ├─ state.fetch_add(count) → 참조 카운트 증가
  │   │   ├─ state가 INVALID(NV_NB) → BUSY 설정
  │   │   ├─ find_slot() → 캐시 슬롯 할당 (Clock 교체)
  │   │   │   ├─ page_ticket.fetch_add(1) → 다음 후보
  │   │   │   ├─ cache_pages[slot].page_take_lock 확인
  │   │   │   └─ DIRTY면 write_data()로 write-back
  │   │   │
  │   │   ├─ read_data(cache, qp, lba, n_blocks, slot)
  │   │   │   ├─ nvm_cmd_header(&cmd, cid, NVM_IO_READ, ns_id)
  │   │   │   ├─ nvm_cmd_data_ptr(&cmd, prp1[slot], prp2[slot])
  │   │   │   ├─ nvm_cmd_rw_blks(&cmd, start_lba, n_blocks)
  │   │   │   ├─ sq_enqueue(sq, &cmd)     ← lock-free SQ 인큐
  │   │   │   │   ├─ ticket 발급 → spin-wait → 커맨드 복사
  │   │   │   │   ├─ tail_mark = LOCKED
  │   │   │   │   ├─ 리더: move_tail + st.mmio(도어벨)
  │   │   │   │   └─ tickets[pos] += 1
  │   │   │   │
  │   │   │   │   [NVMe 컨트롤러가 SQ에서 커맨드를 fetch]
  │   │   │   │   [NVMe 컨트롤러가 SSD에서 데이터를 읽어 GPU 캐시에 DMA]
  │   │   │   │   [NVMe 컨트롤러가 CQ에 완료 엔트리를 기록]
  │   │   │   │
  │   │   │   ├─ cq_poll(cq, cid)          ← CQ 폴링 (Phase 비트 확인)
  │   │   │   ├─ put_cid(sq, cid)           ← CID 반환
  │   │   │   ├─ cq_dequeue(cq, pos, sq)    ← CQ head 전진 + 도어벨
  │   │   │   └─ sq_dequeue(sq, sq_pos)      ← SQ 슬롯 해제
  │   │   │
  │   │   ├─ pages[page].offset = slot       ← 슬롯 번호 기록
  │   │   └─ state XOR 0xC0000000            ← BUSY → VALID 전환
  │   │
  │   ├─ cache_page_addr = base_addr + slot * page_size
  │   ├─ T ret = *(T*)(cache_page_addr + subindex)  ← 데이터 읽기
  │   ├─ __syncwarp(eq_mask)
  │   └─ 마스터: release_page(page, count) → 참조 카운트 감소
  │
  └─ return ret
```

### 12.2 캐시 히트 시 경로 (빠른 경로)

```
GPU 커널 스레드
  │
  ├─ seq_read(i)
  │   ├─ find_range(i), get_page(i), coalesce_page()
  │   ├─ acquire_page(page, count)
  │   │   ├─ state.fetch_add(count) → 참조 카운트 증가
  │   │   └─ state가 VALID(V_NB) → 즉시 pages[page].offset 반환
  │   ├─ cache_page_addr + subindex에서 데이터 읽기
  │   └─ release_page(page, count)
  │
  └─ return ret

  ← SSD 접근 없음, GPU DRAM 접근만 수행
```

### 12.3 각 단계의 레이턴시 추정

| 단계 | 레이턴시 | 비고 |
|------|---------|------|
| Warp coalescing (__match_any_sync 등) | ~수 ns | GPU 내부 warp 연산 |
| 캐시 히트 (VALID 판정 + 데이터 읽기) | ~100 ns | GPU DRAM 접근 |
| 캐시 미스 - find_slot | ~수 us | atomic 경합에 따라 변동 |
| 캐시 미스 - sq_enqueue | ~수 us | 큐 경합 + 도어벨 MMIO |
| 도어벨 MMIO (st.mmio) | ~1-5 us | PCIe posted write |
| NVMe SSD 내부 처리 | ~10-100 us | SSD 종류에 따라 다름 |
| DMA 전송 (SSD→GPU) | ~수 us | PCIe 대역폭 의존 |
| cq_poll | ~수 us | CQ 폴링 대기 |
| 전체 캐시 미스 | ~20-200 us | SSD 성능에 크게 의존 |

### 12.4 최적화 기법

1. **Warp-level Coalescing**: `__match_any_sync`로 같은 페이지 접근 스레드를 그룹화하여 acquire/release 호출을 최소화
2. **도어벨 배칭**: 리더 스레드가 연속 완료된 엔트리를 모아 한 번에 도어벨 업데이트
3. **지수 백오프**: spin-wait 시 `__nanosleep(ns)`으로 점진적으로 대기 시간을 늘려 불필요한 PCIe 트래픽 감소
4. **SM 기반 큐 선택**: `get_smid() % n_qps`로 SM과 큐를 친화시켜 큐 경합 감소
5. **TLB 캐싱**: 최근 접근한 페이지의 캐시 주소를 TLB에 저장하여 반복 접근 시 page_cache lookup 생략
6. **PRP 사전 계산**: 캐시 생성 시 모든 슬롯의 PRP1/PRP2를 미리 계산하여 read_data 호출 시 즉시 사용

---

## 13. BaM vs GPUDirect Storage 비교

### 13.1 아키텍처 차이

```
BaM (GPU-Initiated I/O):
  GPU 스레드 → NVMe SQ 직접 기록 → NVMe SSD → DMA → GPU 캐시
  CPU 역할: 초기화만 (런타임에는 개입 없음)

GPUDirect Storage (CPU-Initiated, GPU 직접 DMA):
  GPU 커널 → cuFile API → CPU GDS 드라이버 → NVMe 드라이버 → SSD → DMA → GPU
  CPU 역할: 모든 I/O를 중재 (cuFile 호출마다 CPU 처리)
```

### 13.2 장단점 비교표

| 특성 | BaM | GPUDirect Storage (GDS) |
|------|-----|------------------------|
| I/O 발행 주체 | GPU 스레드 (직접) | CPU (드라이버 경유) |
| CPU 오버헤드 | 없음 (초기화 후) | 높음 (매 I/O마다) |
| 세밀도 | 페이지 단위 (4KB~) | 블록 단위 (보통 수 MB) |
| 지연 시간 | 낮음 (직접 경로) | 높음 (CPU 중재) |
| 처리량 | 수천 스레드 병렬 → 수백만 IOPS | CPU 코어 수에 제한 |
| 생태계 | 커스텀 커널 모듈 필요 | NVIDIA 공식 지원 (cuFile) |
| 파일시스템 지원 | 없음 (raw NVMe) | ext4, GFS2 등 지원 |
| NVMe 드라이버 | 커스텀 (libnvm.ko) | 표준 Linux NVMe 드라이버 |
| 안정성 | 연구 프로토타입 | 프로덕션 레벨 |
| GPU 캐시 | 소프트웨어 페이지 캐시 (자체 관리) | 없음 (bounce buffer 사용) |
| 다중 SSD 지원 | 네이티브 (STRIPE/REPLICATE) | 별도 구성 필요 |

### 13.3 사용 사례별 적합성

| 사용 사례 | BaM | GDS |
|----------|-----|-----|
| 그래프 분석 (불규칙 접근) | 최적 (세밀한 페이지 단위 I/O) | 부적합 (대용량 전송에 최적화) |
| 딥러닝 훈련 데이터 로딩 | 적합 가능 | 최적 (대용량 순차 전송) |
| 데이터베이스 쿼리 | 최적 (세밀한 랜덤 읽기) | 적합 가능 |
| 비디오 스트리밍 | 부적합 | 최적 (순차 대용량 전송) |
| 실시간 분석 (GPU에서 on-demand) | 최적 (CPU 개입 없는 저지연) | 부적합 (CPU 병목) |
