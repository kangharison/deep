# 그림으로 배우는 BaM — 초기화부터 GPU I/O까지

BaM의 `Controller::Controller()` 생성자가 호출되면 일어나는 모든 과정을 단계별 그림으로 추적한다.

---

## 0. 전체 아키텍처 개요

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        유저 공간 (User Space)                           │
│                                                                         │
│  ┌─────────────────┐    ┌──────────────────┐    ┌────────────────────┐  │
│  │  benchmarks/     │    │  include/         │    │  src/ (libnvm)     │  │
│  │  block/main.cu   │    │  ctrl.h           │    │  ctrl.cpp          │  │
│  │                  │    │  queue.h          │    │  dma.cpp           │  │
│  │  GPU 커널 코드    │    │  struct Controller│    │  admin.cpp         │  │
│  │  + 벤치마크 main  │    │  struct QueuePair │    │  struct controller │  │
│  │                  │    │  page_cache.h     │    │  nvm_ctrl_t        │  │
│  └───────┬──────────┘    └────────┬─────────┘    └─────────┬──────────┘  │
│          │                        │                         │             │
│          │         Controller 생성자가 이 셋을 연결          │             │
│          └────────────────────────┼─────────────────────────┘             │
│                                   │                                      │
│                          open() / ioctl() / mmap()                       │
├───────────────────────────────────┼──────────────────────────────────────┤
│                        커널 공간 (Kernel Space)                          │
│                                   │                                      │
│                      ┌────────────┴────────────┐                         │
│                      │  module/ (커널 모듈)      │                        │
│                      │  pci.c  — mmap, ioctl    │                        │
│                      │  ctrl.c — /dev/libnvmN   │                        │
│                      │  map.c  — DMA, P2P 매핑   │                        │
│                      └────────────┬─────────────┘                        │
│                                   │                                      │
├───────────────────────────────────┼──────────────────────────────────────┤
│                        하드웨어 (Hardware)                               │
│                                   │                                      │
│     ┌─────────────────┐    ┌──────┴──────────┐    ┌─────────────────┐   │
│     │    GPU (VRAM)    │    │  NVMe 컨트롤러   │    │   PCIe 버스      │   │
│     │                 │◄──►│  BAR0 레지스터    │◄──►│                 │   │
│     │  SQ/CQ 메모리    │    │  Doorbell        │    │  P2P DMA        │   │
│     └─────────────────┘    └─────────────────┘    └─────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 1단계: open() — 디바이스 파일 열기

```cpp
// ctrl.h:182
int fd = open("/dev/libnvm0", O_RDWR);
```

```
유저 프로세스                              커널
─────────────                            ──────

open("/dev/libnvm0", O_RDWR)
  │
  │ 시스템콜
  ▼
VFS → inode 조회 → cdev 찾기
  │
  │ cdev는 모듈 probe 시 등록됨:
  │   cdev_init(&ctrl->cdev, &dev_fops)
  │   cdev_add(...)
  │   device_create(...)  →  /dev/libnvm0
  ▼
fd 반환 (예: fd = 3)

┌─ 프로세스 fd table ─────────────────┐
│ [0] stdin                           │
│ [1] stdout                          │
│ [2] stderr                          │
│ [3] ──→ struct file                 │
│          ├─ f_op = &dev_fops        │
│          │    .mmap = mmap_registers│
│          │    .ioctl = map_ioctl    │
│          └─ f_inode → cdev          │
│                        └─ ctrl      │
│                           ├─ pdev   │
│                           └─ BAR0   │
└─────────────────────────────────────┘
```

---

## 2단계: nvm_ctrl_init() — 컨트롤러 핸들 초기화

```cpp
// ctrl.h:189
nvm_ctrl_init(&ctrl, fd);
```

이 함수 하나에 3가지 핵심 동작이 들어있다.

### 2-1. dup(fd) — 파일 디스크립터 복제

```cpp
// src/linux/device.cpp:152
dev->fd = dup(filedes);
```

```
fd table (프로세스당 1개)
┌────────────────────────────────┐
│ [3] ──┐                       │
│       ├──→ struct file         │  refcount = 2
│ [4] ──┘    (dev/libnvm0)      │
└────────────────────────────────┘
  ↑              ↑
  호출자 소유     libnvm 소유 (dev->fd)

→ 호출자가 close(3) 해도 libnvm은 fd=4로 안전하게 작동
→ shared_ptr의 C 버전 (refcount 공유)
```

### 2-2. mmap() — BAR0 레지스터를 유저 공간에 매핑

```cpp
// src/linux/device.cpp:173
void* mm_ptr = mmap(NULL, mm_size, PROT_READ|PROT_WRITE,
                    MAP_SHARED|MAP_FILE|MAP_LOCKED, dev->fd, 0);
```

```
유저 공간                                    커널 모듈 (pci.c)
─────────                                   ────────────────

mmap(NULL, size, ..., fd, 0)
  │
  │ 시스템콜
  ▼
커널 VFS
  │
  ├─ get_unmapped_area()          ← 빈 가상 주소 찾기
  ├─ vm_area_alloc()              ← VMA 구조체 할당
  ├─ VMA 필드 채우기:
  │    vm_start, vm_end, vm_flags, vm_page_prot, vm_file
  │
  └─ file->f_op->mmap(file, vma) ← dev_fops.mmap 호출!
                    │
                    ▼
           mmap_registers(file, vma)       ← pci.c:86
             │
             ├─ 크기 검증:
             │    vma->vm_end - vm_start ≤ pci_resource_len(pdev, 0)?
             │
             ├─ 캐시 비활성화:
             │    vma->vm_page_prot = pgprot_noncached(vm_page_prot)
             │    → PTE에 PCD 비트 설정 → MMIO를 캐시하지 않음
             │
             └─ 물리 주소 매핑:
                  vm_iomap_memory(vma, pci_resource_start(pdev, 0), size)
                    └─ remap_pfn_range()
                       → 페이지 테이블에 BAR0 물리주소 기록
                       → VM_IO | VM_PFNMAP 설정


결과:

유저 가상 주소                         NVMe 컨트롤러 (PCIe BAR0)
┌──────────────────┐                  ┌──────────────────────┐
│ mm_ptr           │    페이지 테이블   │ 물리 주소             │
│  + 0x00: CAP     │──→  PTE  ──────→│ + 0x00: CAP          │
│  + 0x08: VS      │    (Uncached)    │ + 0x08: VS           │
│  + 0x14: CC      │                  │ + 0x14: CC           │
│  + 0x1C: CSTS    │                  │ + 0x1C: CSTS         │
│  + 0x1000: SQ0DB │                  │ + 0x1000: SQ0 Tail DB│
│  + 0x1004: CQ0DB │                  │ + 0x1004: CQ0 Head DB│
│  + 0x1008: SQ1DB │                  │ + 0x1008: SQ1 Tail DB│
│  + 0x100C: CQ1DB │                  │ + 0x100C: CQ1 Head DB│
│  ...             │                  │ ...                  │
└──────────────────┘                  └──────────────────────┘
```

### 2-3. struct controller 할당 + CAP 레지스터 읽기

```cpp
// src/ctrl.cpp → _nvm_ctrl_init()
struct controller* handle = malloc(sizeof(struct controller));
// BAR0에서 CAP 레지스터를 읽어 nvm_ctrl_t 필드를 채운다
```

```
malloc으로 할당된 struct controller (유저 힙)
┌──────────────────────────────────────────────┐
│ lock          = mutex (참조 카운트 보호)       │
│ count         = 1                            │
│ type          = DEVICE_TYPE_IOCTL            │
│ device*       ──→ { fd=4 }                   │
│ ops           = { release, ioctl_map, ioctl_unmap } │
│                                              │
│ handle (nvm_ctrl_t):      ◄─── 외부에 이것만 노출  │
│ ┌──────────────────────────────────────────┐ │
│ │ page_size  = 4096      (CAP.MPSMIN 에서) │ │
│ │ dstrd      = 0         (CAP.DSTRD 에서)  │ │
│ │ timeout    = 500ms     (CAP.TO 에서)     │ │
│ │ max_qs     = 1024      (CAP.MQES 에서)   │ │
│ │ mm_size    = 0x4000    (매핑 크기)        │ │
│ │ mm_ptr     ──→ BAR0 mmap 주소            │ │
│ └──────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
          ↑
    container_of로 역추적 가능:
    nvm_ctrl_t* → struct controller*
```

---

## 3단계: createDma() — Admin Queue용 DMA 메모리 할당

```cpp
// ctrl.h:196
aq_mem = createDma(ctrl, ctrl->page_size * 3);   // 3 페이지 = 12KB
```

### 3-1. posix_memalign — 유저 힙에 정렬 할당

```cpp
// src/linux/dma.cpp:119
posix_memalign(&buffer, ctrl->page_size, size);   // 4KB 정렬, 12KB
```

```
유저 힙 메모리
┌────────────────────────────────────────────────────┐
│           ...  다른 힙 데이터  ...                    │
├────────────────────────────────────────────────────┤ ← 4KB 경계
│ page 0  (4KB)  — Admin SQ용                        │ ← buffer
├────────────────────────────────────────────────────┤ ← 4KB 경계
│ page 1  (4KB)  — Admin CQ용                        │
├────────────────────────────────────────────────────┤ ← 4KB 경계
│ page 2  (4KB)  — Identify 데이터 버퍼               │
├────────────────────────────────────────────────────┤
│           ...  다른 힙 데이터  ...                    │
└────────────────────────────────────────────────────┘

주의: 가상 주소는 연속이지만, 물리 주소는 연속이 아닐 수 있다!
```

### 3-2. ioctl(NVM_MAP_HOST) — 커널에 DMA 매핑 요청

```cpp
// src/linux/device.cpp:56 (ioctl_map 콜백)
ioctl(dev->fd, NVM_MAP_HOST_MEMORY, &request);
```

```
유저 공간                              커널 모듈 (pci.c → map.c)
─────────                             ─────────────────────

ioctl(fd, NVM_MAP_HOST, {vaddr, size})
  │
  │ 시스템콜
  ▼
dev_fops.unlocked_ioctl
  │
  └─→ map_ioctl(file, cmd, arg)         ← pci.c:119
       │
       ├─ copy_from_user(&request, arg)  ← 유저 데이터 복사
       │
       └─ map_user_pages(map)            ← map.c
            │
            ├─ get_user_pages()
            │    → 가상 주소 → 물리 페이지 찾기
            │    → 물리 페이지 pin (스왑 방지)
            │
            ├─ dma_map_page()  (페이지마다)
            │    → 물리 주소 → DMA 버스 주소 변환
            │    → IOMMU 매핑 설정
            │
            └─ DMA 주소를 유저에게 반환


결과: nvm_dma_t 구조체 (유저 공간)

┌─────────────────────────────────────────────┐
│ nvm_dma_t                                   │
│                                             │
│ vaddr       ──→ posix_memalign 반환 주소     │
│ page_size   = 4096                          │
│ n_ioaddrs   = 3                             │
│ ioaddrs[0]  = 0x0020_1000  ← page 0 DMA 주소│
│ ioaddrs[1]  = 0x0050_3000  ← page 1 DMA 주소│  물리적으로
│ ioaddrs[2]  = 0x0010_7000  ← page 2 DMA 주소│  불연속!
└─────────────────────────────────────────────┘
      │
      │  NVMe 커맨드에서 이 DMA 주소를 PRP 엔트리로 사용
      │  → NVMe 컨트롤러가 이 주소로 DMA 읽기/쓰기
      ▼
```

### 3-3. ioctl_mapping — 매핑 메타데이터

```
ioctl_mapping (유저 힙, libnvm 내부)
┌─────────────────────────────────────┐
│ type    = MAP_TYPE_API              │  ← posix_memalign으로 할당했으니
│ buffer  ──→ 원본 버퍼 주소           │     해제 시 free() 필요
│ range:                              │
│   vaddr = buffer                    │
│   page_size = 4096                  │
│   n_pages = 3                       │
└─────────────────────────────────────┘
```

---

## 4단계: initializeController() — Admin Queue 생성 & 식별

```cpp
// ctrl.h:199
initializeController(*this, ns_id);
```

### 4-1. nvm_aq_create — 컨트롤러 리셋 + Admin Queue 활성화

```cpp
// ctrl.h:117
nvm_aq_create(&ctrl.aq_ref, ctrl.ctrl, ctrl.aq_mem.get());
```

```
                          NVMe 컨트롤러 레지스터 (BAR0)
                          ┌───────────────────────────────┐
① CC.EN = 0 (비활성화)    │ CC   [0x14] = 0x0000_0000     │
   → 컨트롤러 리셋        │ CSTS [0x1C]  → RDY=0 대기      │
                          │                               │
② ASQ 주소 기록           │ ASQ  [0x28] = ioaddrs[0]      │─→ page 0
   AQA 크기 기록           │ ACQ  [0x30] = ioaddrs[1]      │─→ page 1
                          │ AQA  [0x24] = (CQ크기|SQ크기)   │
                          │                               │
③ CC.EN = 1 (활성화)      │ CC   [0x14] = 0x0046_0001     │
   → 컨트롤러 동작 시작    │ CSTS [0x1C]  → RDY=1 폴링 대기  │
                          └───────────────────────────────┘

Admin Queue 메모리 (유저 힙, DMA 매핑됨):
┌─────────────────┬─────────────────┬─────────────────┐
│ page 0          │ page 1          │ page 2          │
│ Admin SQ        │ Admin CQ        │ Identify 버퍼    │
│                 │                 │                 │
│ ioaddrs[0]      │ ioaddrs[1]      │ ioaddrs[2]      │
│ = 0x0020_1000   │ = 0x0050_3000   │ = 0x0010_7000   │
│                 │                 │                 │
│ 호스트가 커맨드   │ 컨트롤러가       │ Identify 결과    │
│ 기록            │ 완료 엔트리 기록  │ 데이터 저장      │
└─────────────────┴─────────────────┴─────────────────┘
```

### 4-2. Admin 커맨드 실행 — Identify Controller/Namespace

```cpp
// ctrl.h:124-138
nvm_admin_ctrl_info(aq_ref, &info, ..., aq_mem->ioaddrs[2]);
nvm_admin_ns_info(aq_ref, &ns, ns_id, ..., aq_mem->ioaddrs[2]);
nvm_admin_get_num_queues(aq_ref, &n_cqs, &n_sqs);
```

```
Admin SQ (page 0)                    NVMe 컨트롤러              Admin CQ (page 1)
┌──────────────┐                     ┌───────────┐              ┌──────────────┐
│ [0] Identify │──도어벨 쓰기──→     │ SQ에서     │              │              │
│     Controller│                    │ 커맨드 읽기│              │              │
│ PRP1=ioaddrs[2]                    │     │      │              │              │
└──────────────┘                     │     ▼      │              │              │
                                     │ page 2에   │              │ [0] 완료     │
                                     │ 결과 DMA   │─ 완료 기록 ─→│  Status=OK   │
                                     │ 쓰기       │              │              │
                                     └───────────┘              └──────────────┘

Identify 버퍼 (page 2)에 기록된 결과:
┌──────────────────────────────────────────┐
│ struct nvm_ctrl_info (Identify Controller)│
│   Serial Number, Model, Firmware, ...    │
│   MDTS = Max Data Transfer Size          │
│   SQES/CQES = 큐 엔트리 크기 범위         │
├──────────────────────────────────────────┤
│ struct nvm_ns_info (Identify Namespace)   │
│   ns_id = 1                              │
│   lba_data_size = 512                    │
│   num_blocks = ...                       │
└──────────────────────────────────────────┘
```

---

## 5단계: cudaHostRegister — GPU가 BAR0 도어벨에 접근 가능하게

```cpp
// ctrl.h:203
cudaHostRegister((void*) ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE, cudaHostRegisterIoMemory);
```

```
이 단계 전:                              이 단계 후:

CPU만 BAR0 접근 가능                     GPU도 BAR0 접근 가능!

┌─────┐                                 ┌─────┐    ┌─────┐
│ CPU │──→ BAR0 (mmap)                  │ CPU │──→ BAR0 (mmap)
└─────┘                                 └─────┘
                                         ┌─────┐
                                         │ GPU │──→ BAR0 (cudaHostRegister)
                                         └─────┘
                                           ↑
                                      cudaHostRegisterIoMemory 플래그:
                                      "이건 일반 RAM이 아니라 MMIO 영역이야"
                                      → CUDA가 이 주소를 GPU 페이지 테이블에 등록
                                      → GPU 커널에서 도어벨에 직접 쓸 수 있음

                     cudaHostGetDevicePointer()로
                     GPU용 포인터를 얻을 수 있음 (6단계에서 사용)
```

---

## 6단계: QueuePair 생성 — I/O Queue를 GPU 메모리에

```cpp
// ctrl.h:231-236
for (i = 0; i < n_qps; i++) {
    h_qps[i] = new QueuePair(ctrl, cudaDevice, ns, info, aq_ref, i+1, queueDepth);
    cudaMemcpy(d_qps+i, h_qps[i], sizeof(QueuePair), cudaMemcpyHostToDevice);
}
```

### 6-1. SQ/CQ 메모리를 GPU VRAM에 할당 + DMA 매핑

```cpp
// queue.h:133-135
sq_mem = createDma(ctrl, sq_mem_size, cudaDevice);  // GPU 메모리!
cq_mem = createDma(ctrl, cq_mem_size, cudaDevice);
```

```
※ Admin Queue(3단계)와 다른 점: Admin은 호스트 RAM, I/O Queue는 GPU VRAM!

createDma(ctrl, size, cudaDevice)
  │
  ├─ cudaMalloc(&devptr, size)       ← GPU VRAM에 메모리 할당
  │
  └─ ioctl(fd, NVM_MAP_DEVICE_MEMORY, ...)   ← 커널에 GPU 메모리 매핑 요청
       │
       └─ 커널: nvidia_p2p_get_pages()        ← GPU VRAM을 64KB 단위로 pin
                nvidia_p2p_dma_map_pages()    ← NVMe 컨트롤러용 DMA 주소 획득


GPU VRAM 할당 결과:
┌────────────────────────────────────────────────────────────────┐
│                        GPU VRAM                                │
│                                                                │
│  ┌── SQ 메모리 (64KB 정렬) ──┐  ┌── CQ 메모리 (64KB 정렬) ──┐  │
│  │ 64KB page                 │  │ 64KB page                 │  │
│  │ DMA addr = 0xB000_0000    │  │ DMA addr = 0xB001_0000    │  │
│  │                           │  │                           │  │
│  │ SQ 엔트리[0] (64B)        │  │ CQ 엔트리[0] (16B)        │  │
│  │ SQ 엔트리[1]              │  │ CQ 엔트리[1]              │  │
│  │ ...                       │  │ ...                       │  │
│  │ SQ 엔트리[1023]           │  │ CQ 엔트리[1023]           │  │
│  └───────────────────────────┘  └───────────────────────────┘  │
│                                                                │
│  NVMe 컨트롤러가 P2P DMA로 직접 읽기/쓰기 가능                    │
└────────────────────────────────────────────────────────────────┘
```

### 6-2. Admin 커맨드로 I/O CQ, SQ 생성

```cpp
// queue.h:186, 204
nvm_admin_cq_create(aq_ref, &cq, qp_id, cq_mem, 0, cq_size, ...);
nvm_admin_sq_create(aq_ref, &sq, &cq, qp_id, sq_mem, 0, sq_size, ...);
```

```
Admin SQ를 통해 NVMe 컨트롤러에 큐 생성 요청:

Create I/O CQ 커맨드:
┌──────────────────────────────────────────┐
│ Opcode = 0x05 (Create I/O CQ)           │
│ DWORD10 = (cq_size-1) << 16 | qp_id    │
│ PRP1 = cq_mem->ioaddrs[0]  ← GPU VRAM  │
│        (0xB001_0000)                     │
└──────────────────────────────────────────┘
                   │
                   ▼
NVMe 컨트롤러:
  "ID=1인 CQ를 GPU VRAM 주소 0xB001_0000에 만들었다"

Create I/O SQ 커맨드:
┌──────────────────────────────────────────┐
│ Opcode = 0x01 (Create I/O SQ)           │
│ DWORD10 = (sq_size-1) << 16 | qp_id    │
│ DWORD11 = cq_id << 16  ← 이 SQ의 CQ    │
│ PRP1 = sq_mem->ioaddrs[0]  ← GPU VRAM  │
│        (0xB000_0000)                     │
└──────────────────────────────────────────┘
                   │
                   ▼
NVMe 컨트롤러:
  "ID=1인 SQ를 GPU VRAM 주소 0xB000_0000에 만들었다"
  "이 SQ의 완료는 CQ ID=1에 기록한다"
```

### 6-3. 도어벨을 GPU 포인터로 변환

```cpp
// queue.h:195, 211
cudaHostGetDevicePointer(&devicePtr, (void*) cq.db, 0);
cq.db = (volatile uint32_t*) devicePtr;
cudaHostGetDevicePointer(&devicePtr, (void*) sq.db, 0);
sq.db = (volatile uint32_t*) devicePtr;
```

```
변환 전:                              변환 후:

sq.db ──→ CPU 가상 주소              sq.db ──→ GPU 디바이스 포인터
           (mm_ptr + 0x1008)                    GPU 커널에서 직접 접근 가능!

NVMe BAR0 (PCIe MMIO):
┌──────────────────────────────────────────────────┐
│ ...                                              │
│ 0x1000: Admin SQ Tail Doorbell                   │
│ 0x1004: Admin CQ Head Doorbell                   │
│ 0x1008: I/O SQ1 Tail Doorbell  ← sq.db 가 가리킴 │
│ 0x100C: I/O CQ1 Head Doorbell  ← cq.db 가 가리킴 │
│ 0x1010: I/O SQ2 Tail Doorbell                    │
│ 0x1014: I/O CQ2 Head Doorbell                    │
│ ...                                              │
└──────────────────────────────────────────────────┘
       ↑                      ↑
   CPU mmap 주소          GPU 디바이스 포인터
   (5단계 cudaHostRegister로 GPU도 접근 가능해짐)
```

### 6-4. GPU 병렬 동기화 구조체 초기화

```cpp
// queue.h:63-93
init_gpu_specific_struct(cudaDevice);
```

```
GPU VRAM에 할당되는 Lock-free 동기화 배열들:

┌─── QueuePair (호스트에서 생성, GPU로 복사) ──────────────────────────────┐
│                                                                        │
│  sq (nvm_queue_t):                                                     │
│    db ──────→ GPU 도어벨 포인터 (BAR0)                                   │
│    tickets ──→ ┌─────────────────────────────────────┐                  │
│                │ [0][1][2]...[1023] (SQ 크기만큼)     │ GPU VRAM         │
│                │ 각 슬롯의 순서 번호 (Lamport bakery)  │                  │
│                └─────────────────────────────────────┘                  │
│    tail_mark → ┌─────────────────────────────────────┐                  │
│                │ [0][1][2]...[1023]                   │ GPU VRAM         │
│                │ 커맨드 기록 완료 표시 (도어벨 배치용)  │                  │
│                └─────────────────────────────────────┘                  │
│    cid ──────→ ┌─────────────────────────────────────┐                  │
│                │ [0][1][2]...[65535]                  │ GPU VRAM         │
│                │ Command ID 할당 상태 (LOCKED/FREE)   │                  │
│                └─────────────────────────────────────┘                  │
│                                                                        │
│  cq (nvm_queue_t):                                                     │
│    db ──────→ GPU 도어벨 포인터 (BAR0)                                   │
│    head_mark → ┌─────────────────────────────────────┐                  │
│                │ [0][1][2]...[1023]                   │ GPU VRAM         │
│                │ 완료 처리 완료 표시 (도어벨 배치용)    │                  │
│                └─────────────────────────────────────┘                  │
│    pos_locks → ┌─────────────────────────────────────┐                  │
│                │ [0][1][2]...[1023]                   │ GPU VRAM         │
│                │ CQ 슬롯별 잠금 (재사용 방지)          │                  │
│                └─────────────────────────────────────┘                  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 7단계: Controller를 GPU 메모리에 복사

```cpp
// ctrl.h:242-244
d_ctrl_buff = createBuffer(sizeof(Controller), cudaDevice);
d_ctrl_ptr = d_ctrl_buff.get();
cudaMemcpy(d_ctrl_ptr, this, sizeof(Controller), cudaMemcpyHostToDevice);
```

```
호스트 (CPU DRAM)                           GPU VRAM
─────────────────                          ──────────

struct Controller (this)                   Controller 복사본 (d_ctrl_ptr)
┌─────────────────────────┐   cudaMemcpy   ┌─────────────────────────┐
│ access_counter = 0      │──────────────→│ access_counter = 0      │
│ ctrl* ──→ nvm_ctrl_t    │               │ ctrl*                   │
│ aq_ref                  │               │ aq_ref                  │
│ info (Identify 결과)     │               │ info                    │
│ ns (Namespace 정보)      │               │ ns                      │
│ n_qps = 2               │               │ n_qps = 2               │
│ h_qps ──→ [호스트 배열]  │               │ h_qps                   │
│ d_qps ──→ [GPU QP 배열]  │               │ d_qps ──→ [GPU QP 배열]  │ ★
│ queue_counter = 0        │               │ queue_counter = 0        │
│ page_size = 4096         │               │ page_size = 4096         │
│ blk_size = 512           │               │ blk_size = 512           │
│ blk_size_log = 9         │               │ blk_size_log = 9         │
│ d_ctrl_ptr ──→ (자기자신) │               │ d_ctrl_ptr               │
└─────────────────────────┘               └─────────────────────────┘
                                                     │
                                              GPU 커널이 이 포인터로
                                              NVMe I/O를 직접 발행!
```

---

## 최종 상태: 모든 초기화 완료 후 메모리 배치도

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          CPU DRAM (호스트)                              │
│                                                                         │
│  struct Controller (this)                                               │
│  ┌──────────────────────────┐                                           │
│  │ ctrl* ──→ nvm_ctrl_t     │     struct controller                     │
│  │ aq_ref                   │     ┌─────────────────┐                   │
│  │ aq_mem ──→ nvm_dma_t     │     │ lock, count     │                   │
│  │ h_qps[0] ──→ QueuePair   │     │ device (fd=4)   │                   │
│  │ h_qps[1] ──→ QueuePair   │     │ ops (콜백)      │                   │
│  │ d_qps ──→ (GPU 포인터)   │     │ handle ─────────┤──→ nvm_ctrl_t     │
│  │ d_ctrl_ptr ──→(GPU 포인터)│     └─────────────────┘                   │
│  └──────────────────────────┘                                           │
│                                                                         │
│  Admin Queue DMA 메모리 (posix_memalign, pin됨)                          │
│  ┌──────────┬──────────┬──────────┐                                     │
│  │ ASQ      │ ACQ      │ Identify │                                     │
│  │ page 0   │ page 1   │ page 2   │                                     │
│  └──────────┴──────────┴──────────┘                                     │
│                                                                         │
├─────────────────────────── PCIe 버스 ───────────────────────────────────┤
│                                                                         │
│  NVMe 컨트롤러 BAR0 (MMIO, mmap + cudaHostRegister)                     │
│  ┌─────────────────────────────────────────┐                            │
│  │ CAP, VS, CC, CSTS, ...                  │  ← CPU mmap으로 읽기/쓰기   │
│  │ Doorbell SQ0, CQ0 (Admin)               │  ← CPU가 Admin 도어벨 접근  │
│  │ Doorbell SQ1, CQ1 (I/O)                 │  ← GPU가 I/O 도어벨 접근    │
│  │ Doorbell SQ2, CQ2 (I/O)                 │  ← GPU가 I/O 도어벨 접근    │
│  └─────────────────────────────────────────┘                            │
│                                                                         │
├─────────────────────────── PCIe 버스 ───────────────────────────────────┤
│                                                                         │
│                          GPU VRAM                                       │
│                                                                         │
│  Controller 복사본 (d_ctrl_ptr)                                          │
│  ┌──────────────────────────┐                                           │
│  │ d_qps ──→ QueuePair 배열 │                                           │
│  │ page_size, blk_size, ... │                                           │
│  └──────────────────────────┘                                           │
│                                                                         │
│  QueuePair 배열 (d_qps)                                                  │
│  ┌───────────────────────────────────────────────────────────────┐       │
│  │ QP[0]:                              QP[1]:                   │       │
│  │  sq.db ──→ BAR0 SQ1 Doorbell         sq.db ──→ BAR0 SQ2 DB  │       │
│  │  cq.db ──→ BAR0 CQ1 Doorbell         cq.db ──→ BAR0 CQ2 DB  │       │
│  │  tickets, tail_mark, cid             tickets, tail_mark, cid│       │
│  └───────────────────────────────────────────────────────────────┘       │
│                                                                         │
│  I/O SQ 메모리 (NVMe가 P2P DMA로 읽음)                                   │
│  ┌──────────────┬──────────────┐                                        │
│  │ SQ1 엔트리들  │ SQ2 엔트리들  │                                        │
│  └──────────────┴──────────────┘                                        │
│                                                                         │
│  I/O CQ 메모리 (NVMe가 P2P DMA로 씀)                                     │
│  ┌──────────────┬──────────────┐                                        │
│  │ CQ1 엔트리들  │ CQ2 엔트리들  │                                        │
│  └──────────────┴──────────────┘                                        │
│                                                                         │
│  동기화 배열들                                                            │
│  ┌──────────────────────────────────────────┐                           │
│  │ tickets[1024], tail_mark[1024]           │                           │
│  │ cid[65536], head_mark[1024]              │                           │
│  │ pos_locks[1024]                          │                           │
│  └──────────────────────────────────────────┘                           │
│                                                                         │
│  페이지 캐시 (별도 초기화, 데이터 버퍼)                                     │
│  ┌──────────────────────────────────────────┐                           │
│  │ cache pages (P2P DMA로 NVMe 데이터 수신)  │                           │
│  └──────────────────────────────────────────┘                           │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 8단계: GPU 커널 실행 시 I/O 흐름 (초기화 이후)

```cpp
// main.cu
__global__ void kernel(Controller** ctrls, page_cache_d_t* pc, ...)
```

```
GPU 스레드 1000개가 동시에 실행:

Thread 42                             NVMe 컨트롤러              GPU VRAM
─────────                             ──────────               ─────────

① 큐 선택 (라운드로빈)
   qp = &ctrls[0]->d_qps[tid % n_qps]

② CID 할당 (lock-free)
   cid = get_cid(sq)
   → cid[] 배열에서 FREE 슬롯을 atomicCAS로 획득

③ NVMe 커맨드 빌드
   sq.cmds[slot]:
   ┌──────────────────────┐
   │ Opcode = Read (0x02) │
   │ NSID = 1             │
   │ CID = 42             │
   │ PRP1 = 캐시 페이지 주소│──────────────────────────→ 데이터 목적지
   │ LBA = 시작 블록       │                             (GPU VRAM)
   │ NLB = 블록 수         │
   └──────────────────────┘

④ SQ Doorbell 쓰기 (MMIO)
   *sq.db = new_tail               ─→ BAR0 도어벨
   (PTX: st.mmio.relaxed.sys)         NVMe가 SQ 엔트리를 읽어감
                                           │
                                           │ DMA Read (P2P)
                                           ▼
                                      GPU VRAM SQ에서
                                      커맨드 읽기
                                           │
                                           │ NVMe 내부 처리
                                           │ (Flash 읽기)
                                           │
                                           │ DMA Write (P2P)
                                           ▼
                                      GPU VRAM 캐시 페이지에
⑤ CQ 폴링 (busy-wait)                데이터 직접 기록
   cq_poll(cq)                             │
   → CQ 엔트리의 Phase 비트 반전 대기        │
   → 완료 확인!                    ←────────┘
                                      CQ에 완료 엔트리 기록

⑥ CQ Doorbell 쓰기
   *cq.db = new_head               ─→ BAR0 도어벨
                                      NVMe: "CQ 슬롯 재사용 가능"

⑦ CID 반환 (lock-free)
   put_cid(sq, cid)
   → cid[42] = FREE
```

---

## 요약: 초기화 단계별 한 줄 정리

```
단계   코드 위치              하는 일                     결과물
─────────────────────────────────────────────────────────────────────
 1    open()                /dev/libnvm0 열기           fd
 2-1  dup(fd)               fd 복제 (소유권 분리)        dev->fd
 2-2  mmap()                BAR0를 유저에 매핑           mm_ptr
 2-3  _nvm_ctrl_init()      CAP 읽기, 구조체 초기화      nvm_ctrl_t*
 3    createDma()           Admin Q용 DMA 메모리 할당    aq_mem (3페이지)
 4-1  nvm_aq_create()       컨트롤러 리셋 + Admin Q 활성화 aq_ref
 4-2  Identify 커맨드들      컨트롤러/NS 정보 조회         info, ns
 5    cudaHostRegister()    BAR0를 GPU가 접근 가능하게    GPU 도어벨 접근
 6-1  createDma(GPU)        SQ/CQ를 GPU VRAM에 할당      sq_mem, cq_mem
 6-2  Admin CQ/SQ Create   NVMe에 I/O Queue 생성        cq, sq
 6-3  cudaHostGetDevicePtr  도어벨을 GPU 포인터로 변환    sq.db, cq.db
 6-4  init_gpu_struct       Lock-free 동기화 배열 할당    tickets, cid, marks
 7    cudaMemcpy            Controller를 GPU에 복사      d_ctrl_ptr
 8    kernel<<<>>>          GPU 스레드가 직접 NVMe I/O    P2P DMA!
```
