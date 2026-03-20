# BaM 드라이버 아키텍처: GPU가 NVMe에 직접 접근할 수 있는 이유

**소스코드 위치:** `sources/bam/`
**핵심 파일:**
- `module/pci.c` — 커널 모듈 메인: PCI probe, BAR0 mmap, ioctl(DMA 매핑)
- `module/map.c` — 호스트/GPU 메모리 DMA 매핑 (NVIDIA P2P API 포함)
- `module/ctrl.c` — /dev/libnvmN 캐릭터 디바이스 관리
- `src/linux/device.cpp` — 유저스페이스에서 커널 모듈과 통신
- `include/ctrl.h` — Controller 생성자: cudaHostRegisterIoMemory + QP 생성
- `include/queue.h` — QueuePair: SQ/CQ를 GPU VRAM에 할당

## 1. 핵심 질문: 왜 GPU가 NVMe에 직접 접근할 수 있는가?

일반적으로 NVMe는 Linux 커널 드라이버(nvme.ko)가 독점 관리한다. GPU 커널은 NVMe 레지스터에 접근할 수 없고, GPU VRAM을 NVMe 컨트롤러가 DMA로 접근할 수도 없다. BaM은 **3가지 장벽**을 커스텀 커널 모듈로 해체한다:

```
장벽 ①: NVMe 레지스터(도어벨)가 커널 공간에만 매핑되어 있음
  → 해결: BAR0를 유저스페이스에 mmap + cudaHostRegisterIoMemory로 GPU에 노출

장벽 ②: GPU VRAM의 물리 주소를 NVMe 컨트롤러가 모름
  → 해결: NVIDIA P2P API로 GPU 메모리를 pin + NVMe 컨트롤러용 DMA 주소 획득

장벽 ③: SQ/CQ가 Host DRAM에 있어서 GPU가 직접 접근 불가
  → 해결: SQ/CQ를 GPU VRAM에 할당 + DMA 주소를 NVMe 컨트롤러에 전달
```

## 2. 드라이버 계층 구조

```
┌─────────────────────────────────────────────────────────────────────┐
│                    유저스페이스 (C++ / CUDA)                         │
│                                                                     │
│  Controller 생성자 (ctrl.h)                                        │
│    │                                                                │
│    ├── open("/dev/libnvm0")  ← 커널 모듈이 생성한 캐릭터 디바이스  │
│    ├── nvm_ctrl_init()       ← mmap으로 BAR0 레지스터 매핑 획득    │
│    ├── cudaHostRegister(BAR0, cudaHostRegisterIoMemory) ★          │
│    │   → GPU 커널이 BAR0(도어벨) MMIO에 직접 접근 가능             │
│    ├── createDma(GPU VRAM)   ← ioctl로 GPU 메모리 DMA 매핑 요청   │
│    │   → NVIDIA P2P API로 GPU VRAM의 PCIe 버스 주소 획득           │
│    └── Admin Cmd로 SQ/CQ 생성 (GPU VRAM 주소를 NVMe에 전달)       │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│                    커널 모듈 (module/)                               │
│                                                                     │
│  pci.c: PCI 드라이버                                               │
│    ├── probe: NVMe 디바이스 발견 → /dev/libnvmN 생성               │
│    ├── mmap: BAR0 레지스터를 유저스페이스에 매핑                    │
│    └── ioctl:                                                       │
│        ├── NVM_MAP_HOST_MEMORY   → get_user_pages + dma_map_page   │
│        └── NVM_MAP_DEVICE_MEMORY → nvidia_p2p_get_pages ★         │
│                                   + nvidia_p2p_dma_map_pages ★     │
│                                                                     │
│  map.c: DMA 매핑 관리                                              │
│    ├── map_user_pages()    → 호스트 메모리 pin + DMA 주소 변환     │
│    └── map_gpu_memory() ★ → GPU VRAM pin + P2P DMA 주소 변환     │
│                                                                     │
│  ctrl.c: 캐릭터 디바이스 관리                                      │
│    └── /dev/libnvm0, /dev/libnvm1 ... 생성/삭제                    │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│                    하드웨어                                          │
│                                                                     │
│  GPU ◄──── PCIe P2P ────► NVMe SSD                                 │
│  │                          │                                       │
│  │ VRAM에 SQ/CQ 존재       │ BAR0에 도어벨 레지스터                │
│  │ VRAM에 PRP 버퍼 존재    │ DMA 엔진이 GPU VRAM 직접 접근        │
│  │                          │                                       │
│  └──── GPU가 도어벨에 ─────→ NVMe가 SQ에서 cmd fetch (P2P)        │
│        st.mmio write         NVMe가 데이터를 PRP(GPU VRAM)에 DMA   │
└─────────────────────────────────────────────────────────────────────┘
```

## 3. 장벽 ① 해제: BAR0 도어벨을 GPU에 노출

### 3.1 커널 모듈: BAR0를 유저스페이스에 mmap

**파일:** `module/pci.c:86-109`

```c
// /dev/libnvmN에 mmap()을 호출하면 이 핸들러가 실행
static int mmap_registers(struct file* file, struct vm_area_struct* vma)
{
    // NVMe 컨트롤러의 BAR0 물리 주소를 가져옴
    // pci_resource_start(pdev, 0) = BAR0의 물리 시작 주소

    // 캐시를 비활성화: MMIO 레지스터는 캐시하면 안 됨
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    // BAR0의 물리 주소를 유저스페이스 가상 주소에 매핑
    return vm_iomap_memory(vma, pci_resource_start(ctrl->pdev, 0),
                           vma->vm_end - vma->vm_start);
}
```

→ 이 시점에서 유저스페이스 프로세스가 BAR0(도어벨 포함)를 CPU로 접근 가능

### 3.2 유저스페이스: BAR0를 GPU에 다시 노출

**파일:** `include/ctrl.h:201-207`

```cpp
// BAR0의 유저스페이스 매핑(ctrl->mm_ptr)을 GPU 디바이스 메모리로 등록
cudaHostRegister((void*)ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE,
                 cudaHostRegisterIoMemory);
//                ↑ 이 플래그가 핵심!
// cudaHostRegisterIoMemory:
//   "이 메모리는 I/O 매핑된 MMIO 영역이다.
//    GPU 커널에서 이 주소에 쓰면 PCIe MMIO write로 나간다."
```

→ 이 시점에서 **GPU 커널이 BAR0의 도어벨 레지스터에 직접 MMIO write 가능**

### 3.3 도어벨 포인터를 GPU 디바이스 포인터로 변환

**파일:** `include/queue.h:192-201, 210-217`

```cpp
// nvm_queue_clear()가 계산한 도어벨 주소:
//   sq.db = BAR0 + 0x1000 + (2*QID) * (4 << dstrd)    ← CPU 가상주소
//   cq.db = BAR0 + 0x1000 + (2*QID+1) * (4 << dstrd)  ← CPU 가상주소

// cudaHostGetDevicePointer로 GPU에서 접근 가능한 포인터로 변환
void* devicePtr = nullptr;
cudaHostGetDevicePointer(&devicePtr, (void*)this->sq.db, 0);
this->sq.db = (volatile uint32_t*)devicePtr;  // ← 이제 GPU 포인터

// GPU 커널에서:
asm volatile("st.mmio.relaxed.sys.global.u32 [%0], %1;"
             :: "l"(sq->db), "r"(new_tail) : "memory");
// → GPU가 PCIe를 통해 NVMe 도어벨에 직접 쓰기!
```

**전체 흐름:**
```
BAR0 물리주소 (NVMe MMIO)
  ↓ vm_iomap_memory (커널 모듈 mmap)
BAR0 유저스페이스 가상주소 (ctrl->mm_ptr)
  ↓ cudaHostRegister(cudaHostRegisterIoMemory)
BAR0 GPU 디바이스 포인터 (sq.db, cq.db)
  ↓ GPU 커널에서 st.mmio PTX
NVMe 컨트롤러의 도어벨 레지스터에 도달 ★
```

## 4. 장벽 ② 해제: GPU VRAM을 NVMe가 DMA로 접근

### 4.1 커널 모듈: NVIDIA P2P API로 GPU 메모리 DMA 매핑

**파일:** `module/map.c:395-491`

```c
int map_gpu_memory(struct map* map, struct list* ctrl_list)
{
    // ① GPU 가상 주소를 pin하고 물리 페이지 테이블을 얻음
    //    (GPU 메모리가 스왑되거나 이동되지 않도록 고정)
    err = nvidia_p2p_get_pages(
        0, 0,
        map->vaddr,                    // GPU 가상 주소
        GPU_PAGE_SIZE * map->n_addrs,  // 크기 (64KB 페이지 단위)
        &gd->pages,                    // 반환: GPU 페이지 테이블
        force_release_gpu_memory,      // 콜백: GPU 컨텍스트 소멸 시 강제 해제
        map
    );

    // ② 각 NVMe 컨트롤러에 대해 P2P DMA 주소를 생성
    //    (NVMe 컨트롤러가 GPU VRAM에 PCIe P2P로 접근할 버스 주소)
    while (ctrl = 다음 컨트롤러) {
        err = nvidia_p2p_dma_map_pages(
            ctrl->pdev,         // NVMe PCI 디바이스
            gd->pages,          // GPU 페이지 테이블
            &gd->mappings[j]    // 반환: DMA 주소 매핑
        );
        // → gd->mappings[j]->dma_addresses[i] = NVMe가 GPU 페이지 i에 DMA할 버스 주소
    }

    // DMA 주소를 유저스페이스에 반환 (ioctl 응답)
    for (i = 0; i < map->n_addrs; i++)
        map->addrs[i] = gd->mappings[0]->dma_addresses[i];
}
```

### 4.2 NVIDIA P2P API의 역할

```
nvidia_p2p_get_pages():
  GPU VRAM 가상주소 → GPU 물리 페이지를 pin (이동/해제 방지)
  → GPU 페이지 테이블 반환

nvidia_p2p_dma_map_pages():
  GPU 페이지 테이블 + NVMe PCI 디바이스 → PCIe 버스 주소 생성
  → NVMe 컨트롤러가 이 주소로 DMA 하면 GPU VRAM에 직접 도달
  → CPU 메모리를 경유하지 않음 (PCIe P2P)

결과:
  NVMe의 DMA 엔진이 GPU VRAM의 PCIe 버스 주소를 알게 됨
  → NVMe Read 커맨드의 PRP(Physical Region Page)에 이 주소를 넣으면
  → NVMe가 데이터를 GPU VRAM에 직접 쓸 수 있음 ★
```

### 4.3 ioctl로 유저스페이스에 DMA 주소 전달

**파일:** `module/pci.c:162-186`

```c
case NVM_MAP_DEVICE_MEMORY:
    // 유저스페이스에서 GPU 가상주소와 페이지 수를 받음
    copy_from_user(&request, arg, sizeof(request));

    // NVIDIA P2P API로 GPU 메모리를 DMA 매핑
    map = map_device_memory(&device_list, ctrl,
                            request.vaddr_start, request.n_pages,
                            &ctrl_list);

    // DMA 버스 주소 배열을 유저스페이스에 반환
    copy_to_user(request.ioaddrs, map->addrs,
                 map->n_addrs * sizeof(uint64_t));
    // → 유저스페이스가 이 주소를 NVMe 커맨드의 PRP에 넣을 수 있음
```

## 5. 장벽 ③ 해제: SQ/CQ를 GPU VRAM에 배치

### 5.1 QueuePair 생성자에서 GPU VRAM에 DMA 할당

**파일:** `include/queue.h:132-135`

```cpp
// SQ 메모리를 GPU VRAM에 할당 + NVMe 컨트롤러용 DMA 주소 획득
this->sq_mem = createDma(ctrl,
    NVM_PAGE_ALIGN(sq_mem_size, 64KB),
    cudaDevice);  // ← GPU 디바이스 ID 지정!
// 내부적으로:
//   1. cudaMalloc()으로 GPU VRAM에 메모리 할당
//   2. ioctl(NVM_MAP_DEVICE_MEMORY)로 DMA 주소 획득
//   3. sq_mem->vaddr = GPU 가상주소, sq_mem->ioaddrs = PCIe 버스 주소

// CQ 메모리도 동일하게 GPU VRAM에 할당
this->cq_mem = createDma(ctrl, NVM_PAGE_ALIGN(cq_mem_size, 64KB), cudaDevice);
```

### 5.2 Admin 커맨드로 NVMe에 GPU VRAM SQ/CQ 등록

```cpp
// CQ 생성: GPU VRAM의 DMA 주소를 NVMe에 전달
nvm_admin_cq_create(aq_ref, &this->cq, qp_id,
                    this->cq_mem.get(),  // DMA 핸들 (ioaddrs = GPU VRAM 버스 주소)
                    0, cq_size, false);
// → NVMe 컨트롤러는 이제 CQ 완료 엔트리를 GPU VRAM에 직접 쓸 수 있음

// SQ 생성: 동일하게 GPU VRAM DMA 주소 전달
nvm_admin_sq_create(aq_ref, &this->sq, &this->cq, qp_id,
                    this->sq_mem.get(),  // DMA 핸들 (ioaddrs = GPU VRAM 버스 주소)
                    0, sq_size, false);
// → NVMe 컨트롤러는 SQ 커맨드를 GPU VRAM에서 직접 fetch할 수 있음
```

## 6. 전체 초기화 시퀀스 (시간순)

```
① insmod libnvm.ko (커널 모듈 로드)
   │
   ├── pci_register_driver → NVMe 디바이스 탐색
   ├── add_pci_dev (probe):
   │     ├── pci_request_region(BAR0)   → BAR0 메모리 예약
   │     ├── pci_enable_device          → PCI 디바이스 활성화
   │     ├── ctrl_chrdev_create         → /dev/libnvm0 생성
   │     └── pci_set_master             → DMA 버스 마스터 활성화 ★
   │
② 유저스페이스 프로세스 시작 (BaM 벤치마크)
   │
   ├── open("/dev/libnvm0")
   │
   ├── nvm_ctrl_init(fd):
   │     └── mmap(fd, BAR0 크기)        → BAR0를 유저 가상주소에 매핑
   │         (커널: mmap_registers → vm_iomap_memory)
   │
   ├── cudaHostRegister(BAR0, IoMemory) → BAR0를 GPU가 접근 가능하게 등록 ★
   │
   ├── Admin Queue 생성 + 컨트롤러 리셋/Identify
   │
   ├── for each QueuePair:
   │     ├── createDma(GPU) → cudaMalloc + ioctl(NVM_MAP_DEVICE_MEMORY)
   │     │   (커널: nvidia_p2p_get_pages + nvidia_p2p_dma_map_pages) ★
   │     │   → SQ/CQ 메모리가 GPU VRAM에 존재, NVMe는 DMA 주소로 접근
   │     │
   │     ├── nvm_admin_cq_create(GPU VRAM DMA 주소)
   │     │   → NVMe 컨트롤러가 CQ를 GPU VRAM에 쓸 수 있음
   │     │
   │     ├── cudaHostGetDevicePointer(CQ doorbell)
   │     │   → GPU 커널이 CQ 도어벨에 직접 쓸 수 있음
   │     │
   │     ├── nvm_admin_sq_create(GPU VRAM DMA 주소)
   │     │   → NVMe 컨트롤러가 SQ를 GPU VRAM에서 fetch할 수 있음
   │     │
   │     └── cudaHostGetDevicePointer(SQ doorbell)
   │         → GPU 커널이 SQ 도어벨에 직접 쓸 수 있음
   │
   └── cudaMemcpy(Controller → GPU) → GPU 커널이 QP에 접근 가능
   │
③ GPU 커널 실행 (런타임)
   │
   ├── GPU thread: SQ에 커맨드 작성 (GPU VRAM 로컬 접근, PCIe 불필요)
   ├── GPU thread: 도어벨 st.mmio → PCIe P2P → NVMe 컨트롤러
   ├── NVMe: SQ에서 cmd fetch → PCIe P2P → GPU VRAM 읽기
   ├── NVMe: NAND에서 데이터 읽기
   ├── NVMe: PRP 주소로 DMA → PCIe P2P → GPU VRAM 쓰기
   ├── NVMe: CQ에 완료 기록 → PCIe P2P → GPU VRAM 쓰기
   └── GPU thread: CQ 폴링 (GPU VRAM 로컬 접근) → 완료 확인
```

## 7. Linux 기본 NVMe 드라이버와의 결정적 차이

| 항목 | Linux nvme.ko | BaM libnvm.ko |
|------|:---:|:---:|
| **목적** | 범용 블록 디바이스 제공 | GPU P2P 접근 전용 |
| **SQ/CQ 위치** | Host DRAM (dma_alloc_coherent) | **GPU VRAM** (NVIDIA P2P) |
| **도어벨 접근** | 커널 드라이버 (writel) | **GPU 커널** (st.mmio) |
| **BAR0 노출** | 커널 공간 전용 | 유저스페이스 mmap + **GPU 등록** |
| **DMA 매핑** | dma_map_page (Host 페이지) | **nvidia_p2p_dma_map_pages** (GPU 페이지) |
| **I/O 스택** | VFS→FS→blk-mq→NVMe | **GPU 커널 → NVMe 직접** |
| **CPU 개입** | 전체 I/O 경로에서 필수 | **초기화만** (런타임 불필요) |
| **데이터 경로** | SSD → Host DRAM → (GPU) | **SSD → GPU VRAM** (1-hop) |

## 8. 필수 하드웨어/소프트웨어 요구사항

BaM 드라이버가 동작하려면:

| 요구사항 | 이유 |
|---------|------|
| **NVIDIA GPU (Volta 이상)** | `nvidia_p2p_get_pages` API 지원 필요 |
| **cudaHostRegisterIoMemory** | MMIO 영역을 GPU에 노출하는 CUDA 기능 |
| **PCIe P2P 지원** | GPU↔SSD 간 직접 DMA (CPU 경유 없이) |
| **IOMMU 비활성화** | P2P DMA가 IOMMU를 통과하면 실패하거나 성능 저하 |
| **ACS 비활성화** | PCIe Access Control Services가 P2P를 차단할 수 있음 |
| **Above 4G Decoding** | GPU BAR가 4GB 이상에 매핑될 수 있으므로 |
| **같은 PCIe 스위치/루트** | GPU와 SSD가 같은 PCIe 계층에 있어야 P2P 최적 |

## 9. 핵심 NVIDIA API 3개

| API | 역할 | 파일 |
|-----|------|------|
| `nvidia_p2p_get_pages()` | GPU VRAM을 pin하고 물리 페이지 테이블 획득 | map.c:434 |
| `nvidia_p2p_dma_map_pages()` | NVMe PCI 디바이스용 DMA 버스 주소 생성 | map.c:452 |
| `nvidia_p2p_put_pages()` | GPU 페이지 pin 해제 (정상 종료) | map.c:374 |
| `nvidia_p2p_free_page_table()` | 페이지 테이블 해제 (강제 회수) | map.c:323 |

이 API들은 NVIDIA 드라이버(`nvidia.ko`)가 제공하며, `#include <nv-p2p.h>`로 사용한다. NVIDIA 드라이버가 GPU 메모리의 물리 페이지 정보를 외부(BaM 커널 모듈)에 노출하는 유일한 공식 인터페이스이다.

## 10. .cu 파일에서의 호출 흐름: Host 코드 vs Device 코드

`.cu` 파일 안에는 **host 코드(CPU)**와 **device 코드(GPU)**가 함께 존재한다. open/mmap/ioctl로 커널 모듈을 호출하는 것은 **host 코드**이며, GPU 커널은 이미 준비된 포인터를 직접 사용한다.

### 10.1 .cu 파일 내부 구조

```cuda
// ============ HOST 코드 (CPU에서 실행) ============
int main() {
    // ① open → 커널 모듈의 file_operations에 도달
    Controller ctrl("/dev/libnvm0", ns, gpu, qd, nq);
    //  내부:
    //    open("/dev/libnvm0")         → 커널 모듈 pci.c
    //    mmap(fd, BAR0)               → 커널 모듈 mmap_registers()
    //    ioctl(NVM_MAP_DEVICE_MEMORY) → 커널 모듈 map_ioctl()
    //    cudaHostRegister(IoMemory)   → CUDA 런타임
    //    Admin cmd (Create CQ/SQ)     → NVMe 컨트롤러

    // ② GPU 커널 실행
    random_access_kernel<<<blocks, threads>>>(ctrls, pc, ...);
}

// ============ DEVICE 코드 (GPU에서 실행) ============
__global__ void random_access_kernel(...) {
    // ③ 여기서는 open/mmap/ioctl 절대 안 함!
    //    이미 초기화된 포인터(sq.db, sq.vaddr 등)를 직접 사용
    sq_enqueue(&qp->sq, &cmd);  // GPU VRAM의 SQ에 직접 쓰기
    asm("st.mmio ...");          // 도어벨에 직접 MMIO write
    cq_poll(&qp->cq, cid);      // GPU VRAM의 CQ에서 직접 읽기
}
```

### 10.2 Host→커널 모듈 호출 체인 상세

```
main() [HOST, CPU]
  │
  ├── Controller("/dev/libnvm0", ...) [HOST 코드, ctrl.h]
  │     │
  │     ├── open("/dev/libnvm0")
  │     │     └──→ 커널 모듈: pci.c의 file_operations
  │     │
  │     ├── nvm_ctrl_init(fd)
  │     │     └── mmap(fd, ...)
  │     │           └──→ 커널 모듈: mmap_registers()
  │     │                 └── vm_iomap_memory(BAR0)
  │     │                       → BAR0가 유저 가상주소에 매핑됨
  │     │
  │     ├── cudaHostRegister(BAR0, IoMemory)  [CUDA API, CPU]
  │     │     → GPU가 BAR0(도어벨)에 접근 가능해짐
  │     │
  │     ├── createDma(ctrl, size, cudaDevice)
  │     │     ├── cudaMalloc() → GPU VRAM 할당
  │     │     └── ioctl(fd, NVM_MAP_DEVICE_MEMORY, ...)
  │     │           └──→ 커널 모듈: map_ioctl()
  │     │                 └── map_device_memory()
  │     │                       ├── nvidia_p2p_get_pages()
  │     │                       └── nvidia_p2p_dma_map_pages()
  │     │                             → GPU VRAM의 DMA 주소 반환
  │     │
  │     ├── nvm_admin_cq_create(GPU DMA 주소)  [Admin cmd]
  │     ├── nvm_admin_sq_create(GPU DMA 주소)  [Admin cmd]
  │     ├── cudaHostGetDevicePointer(도어벨)   [CUDA API]
  │     └── cudaMemcpy(Controller → GPU)       [HOST→GPU 복사]
  │
  │   ════ 여기까지 전부 CPU에서 실행 (초기화 완료) ════
  │
  └── random_access_kernel<<<...>>>(ctrls, pc, ...)
        │
        └──→ GPU 커널 시작 (이제부터 GPU에서 실행) ──→
```

### 10.3 GPU 커널 실행 시: 커널 모듈 관여 없음

```
__global__ random_access_kernel() [DEVICE, GPU]
  │
  │  ★ 이 시점에서 모든 포인터가 이미 준비되어 있음:
  │    - d_qps[queue].sq.vaddr → GPU VRAM (SQ 메모리)
  │    - d_qps[queue].sq.db   → GPU 디바이스 포인터 (BAR0 도어벨)
  │    - pc->prp1[entry]      → GPU VRAM (데이터 버퍼 DMA 주소)
  │
  ├── sq_enqueue()  → GPU VRAM에 직접 쓰기 (커널 모듈 관여 없음)
  ├── st.mmio       → PCIe로 도어벨 직접 쓰기 (커널 모듈 관여 없음)
  ├── cq_poll()     → GPU VRAM에서 직접 읽기 (커널 모듈 관여 없음)
  └── (데이터 도착)  → NVMe가 GPU VRAM에 P2P DMA (커널 모듈 관여 없음)
```

### 10.4 단계별 커널 모듈 관여 여부

| 단계 | 실행 위치 | 커널 모듈 관여 | 목적 |
|------|:--------:|:------------:|------|
| `open("/dev/libnvm0")` | CPU (host) | **O** — file open | 디바이스 파일 열기 |
| `mmap(BAR0)` | CPU (host) | **O** — mmap_registers | BAR0를 유저에 노출 |
| `cudaHostRegister(IoMemory)` | CPU (host) | X — CUDA 런타임 | BAR0를 GPU에 노출 |
| `ioctl(NVM_MAP_DEVICE_MEMORY)` | CPU (host) | **O** — map_ioctl | GPU VRAM DMA 매핑 |
| `Admin Create CQ/SQ` | CPU (host) | X — NVMe 직접 | SQ/CQ를 NVMe에 등록 |
| `cudaMemcpy(→GPU)` | CPU (host) | X — CUDA 런타임 | 포인터들을 GPU에 전달 |
| **GPU 커널: sq_enqueue** | **GPU** | **X** ★ | SQ에 커맨드 쓰기 |
| **GPU 커널: doorbell** | **GPU** | **X** ★ | NVMe에 통지 |
| **GPU 커널: cq_poll** | **GPU** | **X** ★ | 완료 확인 |
| **NVMe DMA → GPU VRAM** | **HW** | **X** ★ | 데이터 전송 |

**결론:** 커널 모듈(`libnvm.ko`)은 **초기화 시에만 3번 관여**(open, mmap, ioctl)하고, GPU 커널이 실제 I/O를 수행하는 런타임에는 **커널 모듈이 전혀 개입하지 않는다**. 이것이 BaM이 "CPU 개입 없는 GPU-initiated I/O"를 달성하는 핵심 구조이다.
