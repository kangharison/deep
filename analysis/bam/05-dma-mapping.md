# 05. DMA 매핑 메커니즘 상세 분석

## 1. 개요

BaM 시스템에서 DMA(Direct Memory Access) 매핑은 NVMe 컨트롤러가 호스트 메모리 또는 GPU 메모리에 직접 데이터를 읽고 쓸 수 있도록 가상 주소를 물리/버스 주소로 변환하는 핵심 메커니즘이다. BaM은 호스트 RAM DMA, GPU 디바이스 메모리 DMA, DIS/SmartIO 세그먼트 DMA의 세 가지 백엔드를 지원한다.

```
┌─────────────────────────────────────────────────────────────────────┐
│                      사용자 공간 (User Space)                        │
│                                                                     │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────────────┐    │
│  │nvm_dma_create│   │nvm_dma_map   │   │nvm_dma_map_device    │    │
│  │   (호스트)    │   │   _host()    │   │  (GPU CUDA 메모리)    │    │
│  └──────┬───────┘   └──────┬───────┘   └──────────┬───────────┘    │
│         │                  │                      │                 │
│         └──────────┬───────┘                      │                 │
│                    ▼                              ▼                 │
│            ┌───────────────┐             ┌────────────────┐        │
│            │ ioctl_mapping │             │ ioctl_mapping  │        │
│            │ type=HOST/API │             │ type=CUDA      │        │
│            │ page=4KB      │             │ page=64KB      │        │
│            └───────┬───────┘             └────────┬───────┘        │
│                    │                              │                 │
│            ┌───────┴──────────────────────────────┘                 │
│            ▼                                                        │
│     ┌─────────────┐     ┌──────────────┐    ┌──────────────────┐   │
│     │_nvm_dma_init│────▶│  create_map   │───▶│create_container │   │
│     └──────┬──────┘     └──────────────┘    └──────────────────┘   │
│            │                                                        │
│            ▼                                                        │
│     ┌──────────────┐                                                │
│     │   dma_map()  │── map_range 콜백 ── ioctl_map()               │
│     └──────┬───────┘                                                │
│            │ ioctl(fd, NVM_MAP_HOST_MEMORY / NVM_MAP_DEVICE_MEMORY) │
├────────────┼────────────────────────────────────────────────────────┤
│            ▼          커널 공간 (Kernel Space)                       │
│     ┌──────────────┐                                                │
│     │  map_ioctl() │                                                │
│     └──────┬───────┘                                                │
│            ├────────────────────────────────────┐                   │
│            ▼                                    ▼                   │
│     ┌──────────────┐                    ┌───────────────────┐      │
│     │map_userspace │                    │map_device_memory  │      │
│     │  (호스트)     │                    │   (GPU CUDA)      │      │
│     └──────┬───────┘                    └───────┬───────────┘      │
│            ▼                                    ▼                   │
│     ┌──────────────┐                    ┌───────────────────┐      │
│     │map_user_pages│                    │  map_gpu_memory   │      │
│     │get_user_pages│                    │nvidia_p2p_get_page│      │
│     │dma_map_page  │                    │nvidia_p2p_dma_map │      │
│     └──────────────┘                    └───────────────────┘      │
│            │                                    │                   │
│            ▼                                    ▼                   │
│     ┌──────────────────────────────────────────────────────┐       │
│     │         DMA 버스 주소 배열 (ioaddrs[])                │       │
│     │  copy_to_user로 사용자 공간에 반환                     │       │
│     └──────────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────────────┘
```


## 2. 핵심 자료 구조

### 2.1 사용자 공간 자료 구조

#### nvm_dma_t (nvm_types.h:91)
사용자에게 노출되는 DMA 매핑 핸들이다. 가변 크기 배열(`ioaddrs[]`)을 포함하므로 사용자가 직접 할당할 수 없고, 반드시 API를 통해 생성해야 한다.

```c
typedef struct {
    void*      vaddr;       // 매핑된 가상 주소 (NULL 가능)
    int8_t     local;       // 로컬 메모리 여부
    int8_t     contiguous;  // 물리 메모리 연속 여부
    size_t     page_size;   // 컨트롤러 페이지 크기 (MPS)
    size_t     n_ioaddrs;   // MPS 크기 페이지 수
    uint64_t   ioaddrs[];   // 각 페이지의 물리/IO 주소 배열 (가변 길이)
} nvm_dma_t;
```

#### va_range (src/dma.h:33)
가상 주소 범위를 기술하는 내부 디스크립터이다. DMA 매핑의 입력으로 사용된다.

```c
struct va_range {
    bool            remote;     // 원격 메모리 여부 (GPU 등)
    volatile void*  vaddr;      // 가상 주소 시작
    size_t          page_size;  // 페이지 크기 (호스트: 4KB, GPU: 64KB)
    size_t          n_pages;    // 페이지 수
};
```

#### struct map / struct container (src/dma.cpp:35, 52)
내부적으로 참조 카운팅을 관리하는 매핑 디스크립터와 DMA 핸들 컨테이너이다. `container_of` 패턴으로 `nvm_dma_t` 핸들에서 역추적할 수 있다.

```c
struct map {
    struct mutex        lock;     // 참조 카운트 보호 뮤텍스
    uint32_t            count;    // 참조 카운트
    struct controller*  ctrl;     // 컨트롤러 참조
    struct va_range*    va;       // 가상 주소 범위
    va_range_free_t     release;  // 해제 콜백
    va_unmap_t          unmap;    // 디바이스 언매핑 콜백
};

struct __attribute__((aligned(32))) container {
    struct map*  map;     // 매핑 디스크립터
    nvm_dma_t    handle;  // 사용자 핸들 (ioaddrs[] 배열 뒤따름)
};
```

#### ioctl_mapping (src/linux/map.h:31)
Linux ioctl 경유 매핑을 위한 컨테이너이다. 메모리 타입을 구분하여 해제 시 다르게 처리한다.

```c
enum mapping_type {
    MAP_TYPE_CUDA = 0x1,  // NVIDIA P2P API로 해제
    MAP_TYPE_HOST = 0x2,  // 버퍼 해제 불필요 (사용자 소유)
    MAP_TYPE_API  = 0x4   // posix_memalign 버퍼도 함께 해제
};

struct ioctl_mapping {
    enum mapping_type  type;    // 메모리 종류
    void*              buffer;  // 원본 버퍼 주소
    struct va_range    range;   // 가상 주소 범위 디스크립터
};
```

### 2.2 커널 공간 자료 구조

#### struct map (module/map.h:29)
커널 모듈에서 DMA 매핑을 추적하는 구조체이다. 가변 길이 배열(`addrs[1]`)로 모든 페이지의 DMA 버스 주소를 저장한다.

```c
struct map {
    struct list_node    list;       // 매핑 연결 리스트 노드
    struct task_struct* owner;      // 소유 프로세스 (current)
    u64                 vaddr;      // 시작 가상 주소 (페이지 정렬)
    struct list*        ctrl_list;  // 모든 컨트롤러 리스트 (GPU P2P용)
    struct pci_dev*     pdev;       // PCI 디바이스 참조
    unsigned long       page_size;  // 페이지 크기 (4KB 또는 64KB)
    void*               data;       // 타입별 데이터 (struct page** 또는 gpu_region*)
    release             release;    // 해제 콜백
    unsigned long       n_addrs;    // DMA 주소 개수
    uint64_t            addrs[1];   // DMA 버스 주소 배열 (가변 길이)
};
```

#### struct gpu_region (module/map.c:28)
GPU P2P 매핑 전용 구조체로, NVIDIA P2P API가 반환한 페이지 테이블과 각 NVMe 컨트롤러별 DMA 매핑을 보유한다.

```c
struct gpu_region {
    nvidia_p2p_page_table_t*    pages;      // GPU 페이지 테이블
    nvidia_p2p_dma_mapping_t**  mappings;   // 컨트롤러별 P2P DMA 매핑 배열
};
```

### 2.3 ioctl 인터페이스 (src/linux/ioctl.h)

```c
#define NVM_IOCTL_TYPE  0x80

struct nvm_ioctl_map {
    uint64_t   vaddr_start;  // 매핑할 가상 주소
    size_t     n_pages;      // 페이지 수
    uint64_t*  ioaddrs;      // 커널이 DMA 주소를 기록할 배열
};

enum nvm_ioctl_type {
    NVM_MAP_HOST_MEMORY   = _IOW(0x80, 1, struct nvm_ioctl_map),  // 호스트 메모리 매핑
    NVM_MAP_DEVICE_MEMORY = _IOW(0x80, 2, struct nvm_ioctl_map),  // GPU 메모리 매핑
    NVM_UNMAP_MEMORY      = _IOW(0x80, 3, uint64_t)               // 매핑 해제
};
```


## 3. 호스트 메모리 DMA 매핑

### 3.1 전체 호출 체인

```
사용자 공간:
  nvm_dma_map_host() [src/linux/dma.cpp:152]
    → create_mapping_descriptor(type=MAP_TYPE_HOST, page_size=ctrl->page_size) [src/linux/dma.cpp:63]
    → _nvm_dma_init() [src/dma.cpp:343]
        → create_map() [src/dma.cpp:71]        ← 참조 카운트 매핑 디스크립터 생성
        → create_container() [src/dma.cpp:284]  ← DMA 핸들 컨테이너 할당
        → dma_map() [src/dma.cpp:179]           ← 디바이스 매핑 수행
            → ctrl->ops.map_range() → ioctl_map() [src/linux/device.cpp:56]
                → ioctl(fd, NVM_MAP_HOST_MEMORY, &request)

커널 공간:
  map_ioctl(cmd=NVM_MAP_HOST_MEMORY) [module/pci.c:119]
    → map_userspace() [module/map.c:250]
        → create_descriptor() [module/map.c:49]   ← 커널 map 구조체 할당
        → map_user_pages() [module/map.c:178]
            → get_user_pages(vaddr, n_addrs, FOLL_WRITE) ← 물리 페이지 pin
            → dma_map_page(dev, page, DMA_BIDIRECTIONAL) ← 버스 주소 변환 (반복)
        → list_insert(host_list)                   ← 매핑 리스트에 삽입
    → copy_to_user(request.ioaddrs, map->addrs)    ← 버스 주소를 사용자로 반환

사용자 공간 (계속):
  ioctl_map() → ioaddrs 배열에 버스 주소 수신
  dma_map() → populate_handle() [src/dma.cpp:143]  ← 컨트롤러 페이지 단위로 변환
```

### 3.2 VA→PA 변환 과정

호스트 메모리의 VA→PA 변환은 커널의 `get_user_pages()`와 `dma_map_page()` 두 단계로 이루어진다.

**단계 1: get_user_pages() - 가상 주소 → 물리 페이지**
```
사용자 가상 주소 → 페이지 테이블 워크 → struct page* 획득 → 페이지 pin (스왑 아웃 방지)
```
`get_user_pages()`는 사용자 공간 가상 주소에 대응하는 물리 페이지를 찾아 pin한다. pin된 페이지는 물리 메모리에 고정되어 DMA 전송 중에 스왑 아웃되지 않는다. `FOLL_WRITE` 플래그는 쓰기 가능한 페이지로 폴트인하여 NVMe 디바이스가 DMA로 데이터를 쓸 수 있게 한다.

**단계 2: dma_map_page() - 물리 페이지 → DMA 버스 주소**
```
struct page* → IOMMU가 있으면 IOMMU 테이블에 등록 → DMA 버스 주소 반환
             → IOMMU가 없으면 물리 주소를 그대로 반환
```
`dma_map_page()`는 물리 페이지를 PCI 디바이스가 접근 가능한 DMA 버스 주소로 변환한다. IOMMU가 활성화된 시스템에서는 IOMMU 페이지 테이블에 매핑 엔트리를 생성하고, IOMMU가 없는 시스템에서는 물리 주소를 직접 반환한다.

### 3.3 컨트롤러 페이지 변환 (populate_handle)

커널에서 받은 DMA 버스 주소는 호스트 페이지 크기(보통 4KB) 단위이다. NVMe 컨트롤러는 자체 MPS(Memory Page Size) 단위로 동작하므로, `populate_handle()`에서 변환한다.

```c
// src/dma.cpp:143 - populate_handle()
for (i_page = 0; i_page < handle->n_ioaddrs; ++i_page) {
    size_t current_page = (i_page * handle->page_size) / page_size;
    size_t offset_within_page = (i_page * handle->page_size) % page_size;
    handle->ioaddrs[i_page] = ioaddrs[current_page] + offset_within_page;
}
```

호스트 페이지 크기가 4KB이고 컨트롤러 MPS도 4KB이면 1:1 대응이다. 호스트 페이지가 더 크면(예: 2MB huge page) 하나의 호스트 페이지에서 여러 컨트롤러 페이지가 파생된다.


## 4. GPU 디바이스 메모리 DMA 매핑

### 4.1 전체 호출 체인

```
사용자 공간:
  nvm_dma_map_device(devptr, size) [src/linux/dma.cpp:196]
    → create_mapping_descriptor(type=MAP_TYPE_CUDA, page_size=64KB) [src/linux/dma.cpp:63]
    → _nvm_dma_init() [src/dma.cpp:343]
        → dma_map() → ioctl_map() [src/linux/device.cpp:56]
            → ioctl(fd, NVM_MAP_DEVICE_MEMORY, &request)

커널 공간:
  map_ioctl(cmd=NVM_MAP_DEVICE_MEMORY) [module/pci.c:162]
    → map_device_memory() [module/map.c:501]
        → create_descriptor(vaddr & GPU_PAGE_MASK, n_pages) [module/map.c:49]
        → map_gpu_memory() [module/map.c:395]
            → nvidia_p2p_get_pages()     ← GPU 메모리 pin + 페이지 테이블 획득
            → nvidia_p2p_dma_map_pages() ← 각 NVMe 컨트롤러에 P2P DMA 주소 생성 (반복)
        → list_insert(device_list)
    → copy_to_user(request.ioaddrs, map->addrs)
```

### 4.2 NVIDIA P2P API를 통한 GPU 메모리 DMA

GPU 메모리는 호스트의 MMU가 관리하지 않으므로 `get_user_pages()`를 사용할 수 없다. 대신 NVIDIA P2P(Peer-to-Peer) API를 사용하여 GPU 메모리를 pin하고 다른 PCIe 디바이스가 접근 가능한 DMA 주소를 생성한다.

**GPU 페이지 크기 상수:**
```c
// module/map.c:37
#define GPU_PAGE_SHIFT  16
#define GPU_PAGE_SIZE   (1UL << GPU_PAGE_SHIFT)   // 64KB
#define GPU_PAGE_MASK   ~(GPU_PAGE_SIZE - 1)
```
NVIDIA P2P API는 64KB 페이지 단위로 동작한다. 이는 GPU 내부 메모리 관리 단위에 해당한다.

**단계 1: nvidia_p2p_get_pages() - GPU 메모리 pin**
```c
// module/map.c:434
err = nvidia_p2p_get_pages(0, 0, map->vaddr, GPU_PAGE_SIZE * map->n_addrs,
                           &gd->pages, force_release_gpu_memory, map);
```
- GPU 가상 주소 범위를 NVIDIA 드라이버에 등록하여 pin한다
- GPU 메모리가 재배치되거나 해제되지 않도록 고정한다
- `nvidia_p2p_page_table_t` 구조체를 반환한다 (GPU 물리 페이지 주소 포함)
- 콜백 함수(`force_release_gpu_memory`)를 등록하여 GPU 컨텍스트 소멸 시 강제 해제한다

**단계 2: nvidia_p2p_dma_map_pages() - P2P DMA 주소 생성**
```c
// module/map.c:452
err = nvidia_p2p_dma_map_pages(ctrl->pdev, gd->pages, gd->mappings + j);
```
- 특정 PCI 디바이스(NVMe 컨트롤러)가 GPU 메모리에 직접 DMA할 수 있는 버스 주소를 생성한다
- IOMMU가 있으면 IOMMU 매핑을 설정하고, PCIe 스위치를 통한 P2P 경로를 구성한다
- **모든 NVMe 컨트롤러에 대해 매핑을 생성한다**: Multi-SSD 환경에서 어떤 NVMe에서든 GPU로 직접 DMA가 가능하도록 한다

```c
// module/map.c:446-472 - 모든 컨트롤러에 대한 P2P 매핑 루프
while (element != NULL) {
    ctrl = container_of(element, struct ctrl, list);
    err = nvidia_p2p_dma_map_pages(ctrl->pdev, gd->pages, gd->mappings + (j++));
    if (j == 1) {
        for (i = 0; i < map->n_addrs; ++i)
            map->addrs[i] = gd->mappings[0]->dma_addresses[i];  // 첫 번째 컨트롤러 주소 반환
    }
    element = list_next(element);
}
```

### 4.3 64KB 정렬 요구사항

GPU DMA 매핑은 64KB 정렬을 엄격히 요구한다. `buffer.h`의 `getDeviceMemory()` 함수가 이를 처리한다.

```c
// include/buffer.h:68
static void getDeviceMemory(int device, void*& bufferPtr, void*& devicePtr, size_t size, void*& origPtr) {
    size += 64*1024;                    // 64KB 추가 할당
    cudaMalloc(&bufferPtr, size);       // GPU 메모리 할당
    origPtr = bufferPtr;                // 원본 포인터 보존 (해제용)
    devicePtr = (void*) ((((uint64_t)attrs.devicePointer) + (64*1024)) & 0xffffffffff0000);  // 64KB 정렬
    bufferPtr = (void*) ((((uint64_t)bufferPtr) + (64*1024)) & 0xffffffffff0000);            // 64KB 정렬
}
```

### 4.4 GPU 메모리 해제: 정상 해제 vs 강제 해제

GPU 메모리 매핑 해제에는 두 가지 경로가 있다.

**정상 해제 (release_gpu_memory, module/map.c:344):**
사용자가 `NVM_UNMAP_MEMORY` ioctl로 명시적으로 해제를 요청할 때 호출된다.
```c
nvidia_p2p_dma_unmap_pages(ctrl->pdev, gd->pages, gd->mappings[j]);  // P2P DMA 매핑 해제
nvidia_p2p_put_pages(0, 0, map->vaddr, gd->pages);                   // GPU 페이지 반환
```

**강제 해제 (force_release_gpu_memory, module/map.c:292):**
NVIDIA 드라이버가 GPU 컨텍스트를 소멸시킬 때(프로세스 종료, GPU 리셋 등) 콜백으로 호출된다.
```c
nvidia_p2p_dma_unmap_pages(ctrl->pdev, gd->pages, gd->mappings[j]);  // P2P DMA 매핑 해제
nvidia_p2p_free_page_table(gd->pages);   // 페이지 테이블만 해제 (put_pages가 아님!)
```
강제 해제에서는 `nvidia_p2p_put_pages()` 대신 `nvidia_p2p_free_page_table()`을 사용한다. 드라이버가 이미 GPU 페이지를 회수했으므로 put_pages를 호출하면 안 된다.


## 5. PCIe P2P 주소 공간

### 5.1 PCIe P2P DMA 데이터 경로

BaM의 핵심은 NVMe SSD가 GPU 메모리에 직접 DMA하는 PCIe P2P(Peer-to-Peer) 전송이다.

```
                    PCIe Root Complex
                    ┌──────────┐
                    │  CPU/RC  │
                    └────┬─────┘
                         │
              ┌──────────┼──────────┐
              │          │          │
        ┌─────┴────┐  ┌──┴───┐  ┌──┴─────┐
        │ PCIe     │  │ PCIe │  │  ...   │
        │ Switch   │  │ Port │  │        │
        └────┬─────┘  └──────┘  └────────┘
             │
     ┌───────┼───────┐
     │               │
┌────┴────┐   ┌──────┴──────┐
│  GPU    │   │  NVMe SSD   │
│ VRAM    │   │ Controller  │
│(64KB pg)│   │             │
└─────────┘   └─────────────┘
     ▲               │
     │    PCIe P2P    │
     │    DMA 전송     │
     └────────────────┘
```

**P2P DMA 주소의 의미:**
- `nvidia_p2p_dma_map_pages()`가 반환하는 DMA 주소는 NVMe 컨트롤러가 PCIe 버스를 통해 GPU VRAM에 접근할 때 사용하는 버스 주소이다
- 이 주소는 PCIe 스위치가 GPU의 BAR 영역 내 물리 주소로 라우팅한다
- IOMMU가 있으면 IOMMU 테이블을 거쳐 실제 GPU 물리 주소로 변환된다

### 5.2 P2P DMA 전송 흐름

```
1. GPU 커널이 NVMe Read 커맨드를 SQ에 기록
   └── PRP1에 GPU 메모리의 DMA 버스 주소가 들어감

2. GPU 커널이 SQ 도어벨을 쓴다
   └── PCIe MMIO 쓰기 → NVMe 컨트롤러가 새 커맨드를 감지

3. NVMe 컨트롤러가 SQ에서 커맨드를 가져온다
   └── PCIe DMA 읽기 (NVMe → SQ in GPU memory)

4. NVMe 컨트롤러가 NAND에서 데이터를 읽는다
   └── 내부 플래시 읽기 + FTL

5. NVMe 컨트롤러가 데이터를 PRP1 주소로 DMA 쓴다
   └── PCIe P2P DMA 쓰기 (NVMe → GPU VRAM)
   └── PCIe 스위치가 GPU BAR 영역으로 라우팅

6. NVMe 컨트롤러가 CQ에 완료 엔트리를 쓴다
   └── PCIe DMA 쓰기 (NVMe → CQ in GPU memory)

7. GPU 커널이 CQ를 폴링하여 완료를 감지
   └── GPU 메모리 읽기 (완료 엔트리의 phase bit 확인)
```


## 6. PRP(Physical Region Page) 엔트리 구성

### 6.1 PRP 기본 개념

NVMe 커맨드는 데이터 전송 위치를 PRP(Physical Region Page) 방식으로 지정한다. 커맨드의 DWORD6-9에 PRP1과 PRP2 두 개의 64비트 주소 필드가 있다.

```
NVMe 커맨드 (64바이트):
┌────────────────────────────────┐
│ DWORD0: Opcode, CID            │
│ DWORD1: NSID                   │
│ DWORD2-5: Reserved             │
│ DWORD6-7: PRP1 (64비트)        │ ← 첫 번째 데이터 페이지 주소
│ DWORD8-9: PRP2 (64비트)        │ ← 두 번째 페이지 또는 PRP 리스트 주소
│ DWORD10-11: Starting LBA       │
│ DWORD12: Number of LBs         │
│ DWORD13-15: Command specific   │
└────────────────────────────────┘
```

### 6.2 전송 크기별 PRP 사용 방식

**1 페이지 이하 전송:**
```
PRP1 = 데이터 페이지의 DMA 주소
PRP2 = 0 (사용 안 함)
```

**2 페이지 전송:**
```
PRP1 = 첫 번째 페이지의 DMA 주소
PRP2 = 두 번째 페이지의 DMA 주소
```

**3 페이지 이상 전송 (PRP 리스트 사용):**
```
PRP1 = 첫 번째 페이지의 DMA 주소
PRP2 = PRP 리스트 페이지의 DMA 주소

PRP 리스트 페이지 (메모리 내 배열):
┌──────────────────────────────────┐
│ entry[0]: 두 번째 데이터 페이지 주소  │
│ entry[1]: 세 번째 데이터 페이지 주소  │
│ ...                              │
│ entry[N-2]: N번째 데이터 페이지 주소  │
│ entry[N-1]: 다음 PRP 리스트 주소     │ ← 체인 연결 (페이지 수 초과 시)
└──────────────────────────────────┘
```

### 6.3 BaM의 PRP 구현

#### nvm_cmd_data_ptr() (include/nvm_cmd.h:86)
PRP1과 PRP2를 커맨드의 DWORD6-9에 설정한다.
```c
void nvm_cmd_data_ptr(nvm_cmd_t* cmd, uint64_t prp1, uint64_t prp2) {
    cmd->dword[0] &= ~((0x03 << 14) | (0x03 << 8));  // PSDT=PRP 모드
    cmd->dword[6] = (uint32_t) prp1;
    cmd->dword[7] = (uint32_t) (prp1 >> 32UL);
    cmd->dword[8] = (uint32_t) prp2;
    cmd->dword[9] = (uint32_t) (prp2 >> 32UL);
}
```

#### nvm_prp_list() (include/nvm_cmd.h:148)
PRP 리스트 페이지를 DMA 주소로 채운다. 페이지 수가 리스트 용량을 초과하면 마지막 엔트리를 다음 리스트 포인터용으로 예약한다.
```c
size_t nvm_prp_list(const nvm_prp_list_t* list, size_t n_pages, const uint64_t* ioaddrs) {
    size_t n_prps = list->page_size / sizeof(uint64_t);  // 리스트 용량
    if (n_pages > n_prps) --n_prps;  // 마지막 엔트리 예약
    for (i_prp = 0; i_prp < n_prps; ++i_prp)
        entries[i_prp] = ioaddrs[i_prp];
    // 캐시 플러시
    return i_prp;
}
```

#### nvm_cmd_data() (include/nvm_cmd.h:252)
전송 크기에 따라 적절한 PRP 방식을 선택하는 통합 함수이다.
```c
size_t nvm_cmd_data(nvm_cmd_t* cmd, size_t n_lists, const nvm_prp_list_t* lists,
                    size_t n_pages, const uint64_t* ioaddrs) {
    dptr0 = ioaddrs[prp++];                          // PRP1 = 첫 페이지
    if (n_pages > 2 && n_lists != 0) {
        prp += nvm_prp_list_chain(n_lists, lists, n_pages - 1, &ioaddrs[prp]);
        dptr1 = lists[0].ioaddr;                     // PRP2 = PRP 리스트 주소
    } else if (n_pages >= 2) {
        dptr1 = ioaddrs[prp++];                      // PRP2 = 두 번째 페이지
    }
    nvm_cmd_data_ptr(cmd, dptr0, dptr1);
}
```

### 6.4 page_cache의 PRP 사전 계산

page_cache_t 생성자에서 모든 캐시 페이지에 대한 PRP1/PRP2 주소를 미리 계산하여 GPU 메모리에 저장한다. 이를 통해 GPU 커널에서 I/O 커맨드를 빌드할 때 별도의 PRP 리스트 구성 없이 `prp1[page_idx]`, `prp2[page_idx]`로 즉시 접근할 수 있다.

```
캐시 페이지 크기와 컨트롤러 페이지 크기 관계에 따라 3가지 모드:

1) cache_page_size <= ctrl_page_size (예: 4KB ≤ 4KB)
   → PRP1만 사용, PRP2 불필요
   → prp1[i] = pages_dma->ioaddrs[i/how_many] + (i%how_many)*ps

2) ctrl_page_size < cache_page_size ≤ 2 * ctrl_page_size (예: 8KB)
   → PRP1 + PRP2 직접 사용
   → prp1[i] = pages_dma->ioaddrs[i*2]
   → prp2[i] = pages_dma->ioaddrs[i*2+1]

3) cache_page_size > 2 * ctrl_page_size (예: 16KB 이상)
   → PRP1 + PRP 리스트 사용
   → prp1[i] = pages_dma->ioaddrs[i*how_many]
   → prp2[i] = prp_list_dma->ioaddrs[i] (PRP 리스트 페이지 주소)
   → PRP 리스트 내용: 나머지 컨트롤러 페이지 주소들
```


## 7. Linux ioctl Backend vs DIS/SmartIO Backend

### 7.1 아키텍처 비교

```
┌──────────────────────────┐    ┌─────────────────────────────────┐
│    Linux ioctl Backend   │    │    DIS/SmartIO Backend           │
│  (src/linux/*)           │    │  (src/dis/*)                    │
├──────────────────────────┤    ├─────────────────────────────────┤
│                          │    │                                 │
│  사용자 공간 라이브러리      │    │  사용자 공간 라이브러리              │
│  ┌──────────────────┐    │    │  ┌──────────────────────────┐  │
│  │ nvm_dma_map_host │    │    │  │nvm_dis_dma_map_local     │  │
│  │ nvm_dma_map_device│   │    │  │nvm_dis_dma_map_remote    │  │
│  │ nvm_dma_create   │    │    │  │nvm_dis_dma_create        │  │
│  └────────┬─────────┘    │    │  │nvm_dis_dma_map_host      │  │
│           │              │    │  │nvm_dis_dma_map_device    │  │
│           ▼              │    │  └────────────┬─────────────┘  │
│  ┌──────────────────┐    │    │               ▼                │
│  │   ioctl() 경유    │    │    │  ┌──────────────────────────┐  │
│  │  /dev/libnvmN    │    │    │  │   SISCI API 경유           │  │
│  └────────┬─────────┘    │    │  │ SCIMapLocalSegment        │  │
│           │              │    │  │ SCIMapRemoteSegment       │  │
│           ▼              │    │  │ SCICreateDeviceSegment    │  │
│  커널 모듈 (module/)      │    │  └──────────────────────────┘  │
│  ┌──────────────────┐    │    │                                 │
│  │ get_user_pages   │    │    │  SISCI가 내부적으로 메모리        │
│  │ dma_map_page     │    │    │  핀닝과 DMA 매핑을 처리           │
│  │ nvidia_p2p_*     │    │    │                                 │
│  └──────────────────┘    │    │                                 │
├──────────────────────────┤    ├─────────────────────────────────┤
│ 장점:                     │    │ 장점:                            │
│ - 표준 Linux API          │    │ - 멀티 노드 NVMe over Fabrics   │
│ - 추가 하드웨어 불필요      │    │ - 원격 메모리 직접 접근            │
│ - NVIDIA P2P GPU 지원     │    │ - 디바이스 메모리(CMB) 활용        │
│                          │    │ - 네트워크 클러스터 지원            │
│ 단점:                     │    │                                 │
│ - 단일 노드 제한           │    │ 단점:                            │
│ - ioctl 오버헤드           │    │ - DIS 전용 하드웨어 필요           │
│                          │    │ - SISCI 라이브러리 의존             │
└──────────────────────────┘    └─────────────────────────────────┘
```

### 7.2 디바이스 타입 구분

BaM은 컨트롤러 초기화 시 디바이스 타입을 기록하여 적절한 백엔드를 선택한다.

```c
// Linux ioctl 경유
_nvm_ctrl_init(ctrl, dev, &ops, DEVICE_TYPE_IOCTL, mm_ptr, mm_size);

// DIS/SmartIO 경유
_nvm_ctrl_init(ctrl, dev, &ops, DEVICE_TYPE_SMARTIO, mm_ptr, mm_size);
```

### 7.3 DIS 백엔드의 세그먼트 기반 매핑

DIS 백엔드는 SISCI 세그먼트 추상화를 사용한다. 메모리 영역을 세그먼트로 등록하면 DIS 패브릭에 연결된 모든 디바이스가 접근 가능해진다.

**로컬 세그먼트 (src/dis/dma.c:395):**
```
호스트 메모리 → SCICreateLocalSegment → SCISetSegmentAvailable → SCIMapLocalSegment
```
호스트 메모리를 SISCI 세그먼트로 등록하여 NVMe 디바이스와 원격 노드가 접근 가능하게 한다.

**원격 세그먼트 (src/dis/dma.c:455):**
```
원격 메모리 → SCIConnectSegment → SCIMapRemoteSegment
```
원격 노드 또는 NVMe CMB(Controller Memory Buffer)의 메모리를 로컬에서 접근 가능하게 한다.

**GPU 세그먼트 (src/dis/dma.c:790):**
```
CUDA 메모리 → SCIAttachPhysicalMemory → DIS 세그먼트에 GPU 물리 주소 등록
```
GPU 메모리를 DIS 세그먼트로 등록하여 원격 NVMe 디바이스가 직접 DMA할 수 있게 한다.


## 8. 참조 카운팅과 수명 관리

### 8.1 참조 카운트 구조

```
nvm_dma_t (사용자 핸들)
    │ container_of
    ▼
struct container ──▶ struct map (참조 카운트)
                         │
                         ├── count: 참조 카운트
                         ├── ctrl: 컨트롤러 참조 (_nvm_ctrl_get/_put)
                         ├── va: 가상 주소 범위
                         ├── release: VA 해제 콜백
                         └── unmap: 디바이스 언매핑 콜백
```

### 8.2 참조 카운트 동작

**생성 (create_map, src/dma.cpp:71):**
- 참조 카운트를 1로 초기화한다
- 컨트롤러 참조를 증가시킨다 (`_nvm_ctrl_get`)
- DMA 매핑이 유효한 동안 컨트롤러가 해제되지 않도록 보장한다

**복제 (nvm_dma_remap, src/dma.cpp:453):**
- 같은 물리 메모리에 대해 새로운 DMA 핸들을 생성한다
- 매핑 디스크립터의 참조 카운트를 증가시킨다 (`get_map`)
- 디바이스 매핑을 다시 수행하지 않고 기존 버스 주소를 공유한다

**해제 (nvm_dma_unmap → put_map, src/dma.cpp:221):**
```
nvm_dma_unmap(handle)
  → remove_container(container)
    → put_map(md)
      → --md->count
      → count == 0이면:
        → md->unmap()    ← 디바이스 언매핑 (ioctl_unmap)
        → md->release()  ← VA 해제 (release_mapping_descriptor)
        → remove_map()   ← 컨트롤러 참조 해제 (_nvm_ctrl_put)
```


## 9. 관련 파일 요약

| 파일 경로 | 역할 |
|-----------|------|
| `src/dma.cpp` | 핵심 DMA 매핑 로직 (참조 카운팅, populate_handle, 컨테이너 관리) |
| `src/dma.h` | 내부 va_range 타입 정의 |
| `src/linux/dma.cpp` | Linux 전용 DMA API (nvm_dma_create, nvm_dma_map_host, nvm_dma_map_device) |
| `src/linux/device.cpp` | Linux 전용 컨트롤러 초기화 (ioctl_map, ioctl_unmap, BAR0 mmap) |
| `src/linux/map.h` | ioctl_mapping 타입, mapping_type 열거형 |
| `src/linux/ioctl.h` | ioctl 명령 코드와 nvm_ioctl_map 구조체 |
| `module/pci.c` | 커널 모듈 메인: PCI probe, ioctl 핸들러, mmap 핸들러 |
| `module/map.c` | 커널 DMA 매핑: get_user_pages, nvidia_p2p_get_pages, DMA 주소 변환 |
| `module/map.h` | 커널 map 구조체 정의 |
| `include/nvm_dma.h` | DMA 공개 API 헤더 |
| `include/nvm_types.h` | nvm_dma_t, nvm_prp_list_t 타입 정의 |
| `include/nvm_cmd.h` | PRP 리스트 구성 함수 (nvm_prp_list, nvm_cmd_data) |
| `include/buffer.h` | C++ DMA 래퍼 (createDma, getDeviceMemory) |
| `src/dis/dma.c` | DIS/SmartIO 백엔드 DMA 매핑 구현 |
