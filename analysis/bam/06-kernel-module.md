# 06. 커널 모듈 분석

## 1. 개요

BaM의 커널 모듈(`libnvm helper`)은 NVMe PCI 디바이스에 대한 최소한의 드라이버로, Linux 커널의 표준 NVMe 드라이버(`drivers/nvme/host/`)를 대체하여 사용자 공간에서 NVMe 컨트롤러를 직접 제어할 수 있게 한다. 이 모듈은 NVMe 커맨드 처리를 커널이 아닌 GPU에서 수행하기 위해 설계되었다.

```
┌─────────────────────────────────────────────────────────┐
│                     사용자 공간                           │
│                                                         │
│  ┌───────────────────┐     ┌───────────────────────┐   │
│  │  libnvm 라이브러리  │     │  GPU 커널 (CUDA)       │   │
│  │                   │     │                       │   │
│  │  nvm_ctrl_init()  │     │  NVMe 커맨드 빌드       │   │
│  │  nvm_aq_create()  │     │  SQ 도어벨 쓰기         │   │
│  │  nvm_admin_*()    │     │  CQ 폴링/디큐           │   │
│  └───────┬───────────┘     └──────────┬────────────┘   │
│          │                            │                 │
│    open(/dev/libnvm0)           cudaHostRegister       │
│    ioctl(MAP_HOST/DEVICE)       (IoMemory) 경유        │
│    mmap(BAR0)                   BAR0 직접 접근          │
│          │                            │                 │
├──────────┼────────────────────────────┼─────────────────┤
│          ▼       커널 공간             ▼                 │
│  ┌───────────────────────────────────────────┐         │
│  │          libnvm helper 커널 모듈            │         │
│  │                                           │         │
│  │  PCI 드라이버 등록 (NVMe 클래스 매칭)        │         │
│  │  캐릭터 디바이스 생성 (/dev/libnvmN)         │         │
│  │  ioctl: DMA 매핑/해제                      │         │
│  │  mmap: BAR0 레지스터 노출                   │         │
│  │  ─── NVMe 커맨드 처리 없음! ───             │         │
│  └───────────────────────────────────────────┘         │
│          │                            │                 │
│          ▼                            ▼                 │
│  ┌──────────────┐            ┌───────────────┐         │
│  │ PCIe BAR0    │            │ NVMe SSD      │         │
│  │ (MMIO 레지스터)│           │ (DMA 엔진)     │         │
│  └──────────────┘            └───────────────┘         │
└─────────────────────────────────────────────────────────┘
```


## 2. 모듈 구성 파일

| 파일 | 역할 |
|------|------|
| `module/pci.c` | 메인 모듈 파일: PCI 드라이버, ioctl 핸들러, mmap 핸들러 |
| `module/map.c` | DMA 메모리 매핑: 호스트 페이지 pin, GPU P2P 매핑 |
| `module/map.h` | map 구조체와 매핑 함수 선언 |
| `module/ctrl.c` | 컨트롤러 관리: 생성, 검색, 캐릭터 디바이스 생성/제거 |
| `module/ctrl.h` | ctrl 구조체 선언 |
| `module/list.h` | 단순 연결 리스트 구현 |
| `src/linux/ioctl.h` | ioctl 인터페이스 정의 (커널/사용자 공간 공유) |


## 3. PCI 드라이버 등록

### 3.1 PCI 디바이스 매칭

```c
// module/pci.c:45
#define PCI_CLASS_NVME      0x010802     // 스토리지(01) NVMe(08) NVM(02)
#define PCI_CLASS_NVME_MASK 0xffffff

static const struct pci_device_id id_table[] = {
    { PCI_DEVICE_CLASS(PCI_CLASS_NVME, PCI_CLASS_NVME_MASK) },
    { 0 }   // sentinel
};
```
NVMe PCI 클래스 코드(0x010802)에 해당하는 모든 디바이스를 매칭한다. 벤더/디바이스 ID 대신 클래스 코드를 사용하므로 모든 NVMe SSD를 지원한다.

### 3.2 PCI 드라이버 구조체

```c
// module/pci.c:373
static struct pci_driver driver = {
    .name = "libnvm helper",
    .id_table = id_table,
    .probe = add_pci_dev,       // 디바이스 발견 시 호출
    .remove = remove_pci_dev,   // 디바이스 제거 시 호출
};
```


## 4. 모듈 초기화 흐름

### 4.1 module_init: libnvm_helper_entry()

```c
// module/pci.c:387
static int __init libnvm_helper_entry(void) {
    list_init(&ctrl_list);     // 컨트롤러 리스트 초기화
    list_init(&host_list);     // 호스트 DMA 매핑 리스트 초기화
    list_init(&device_list);   // GPU DMA 매핑 리스트 초기화

    alloc_chrdev_region(&dev_first, 0, max_num_ctrls, "libnvm helper");
    dev_class = class_create(THIS_MODULE, "libnvm helper");
    pci_register_driver(&driver);  // → 기존 NVMe에 대해 probe 즉시 호출
}
```

```
insmod libnvm.ko
    │
    ▼
libnvm_helper_entry()
    ├── list_init() × 3             ← 전역 리스트 초기화
    ├── alloc_chrdev_region()        ← major 번호 동적 할당
    ├── class_create()               ← sysfs 디바이스 클래스 생성
    └── pci_register_driver()        ← PCI 서브시스템에 등록
         │
         ▼ 시스템에 NVMe 디바이스가 있으면 즉시:
    add_pci_dev() (probe 콜백)
         ├── ctrl_get()              ← 컨트롤러 참조 생성 + 리스트 삽입
         ├── pci_request_region(0)   ← BAR0 메모리 영역 예약
         ├── pci_enable_device()     ← PCI 디바이스 활성화
         ├── ctrl_chrdev_create()    ← /dev/libnvmN 캐릭터 디바이스 생성
         └── pci_set_master()        ← DMA 버스 마스터링 활성화
```

### 4.2 probe: add_pci_dev()

```c
// module/pci.c:248
static int add_pci_dev(struct pci_dev* dev, const struct pci_device_id* id) {
    ctrl = ctrl_get(&ctrl_list, dev_class, dev, curr_ctrls);  // ctrl 구조체 생성
    pci_request_region(dev, 0, DRIVER_NAME);  // BAR0 예약
    pci_enable_device(dev);                   // PCI 디바이스 활성화
    ctrl_chrdev_create(ctrl, dev_first, &dev_fops);  // /dev/libnvmN 생성
    pci_set_master(dev);                      // 버스 마스터 활성화
    ++curr_ctrls;
}
```

**핵심 차이점:** Linux 표준 NVMe 드라이버(`nvme_probe`)는 여기서 NVMe 큐 생성, 블록 디바이스 등록, I/O 스케줄러 연결 등을 수행하지만, BaM 모듈은 PCI 리소스 확보와 캐릭터 디바이스 생성만 한다.


## 5. 캐릭터 디바이스 인터페이스

### 5.1 파일 연산 테이블

```c
// module/pci.c:235
static const struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = map_ioctl,    // DMA 매핑/해제 처리
    .mmap = mmap_registers,         // BAR0 레지스터 노출
};
```

BaM의 캐릭터 디바이스는 오직 두 가지 연산만 제공한다: DMA 매핑을 위한 `ioctl`과 NVMe 레지스터 접근을 위한 `mmap`.

### 5.2 /dev/libnvmN 디바이스 노드

각 NVMe 컨트롤러에 대해 `/dev/libnvm0`, `/dev/libnvm1`, ... 형태의 캐릭터 디바이스가 생성된다. `ctrl_chrdev_create()`(module/ctrl.c)에서 `cdev_add()`와 `device_create()`를 호출하여 udev가 자동으로 디바이스 노드를 생성한다.


## 6. mmap: BAR0 레지스터 노출

### 6.1 mmap_registers()

```c
// module/pci.c:86
static int mmap_registers(struct file* file, struct vm_area_struct* vma) {
    ctrl = ctrl_find_by_inode(&ctrl_list, file->f_inode);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);  // 캐시 비활성화
    return vm_iomap_memory(vma, pci_resource_start(ctrl->pdev, 0),
                           vma->vm_end - vma->vm_start);
}
```

**pgprot_noncached**: MMIO 레지스터에 대한 캐시를 비활성화하여, CPU가 레지스터를 읽을 때 캐시가 아닌 실제 하드웨어 값을 읽도록 보장한다. 이것이 없으면 도어벨 쓰기가 캐시에만 반영되고 디바이스에 전달되지 않을 수 있다.

**vm_iomap_memory**: PCI BAR0의 물리 주소를 사용자 프로세스의 가상 주소 공간에 매핑한다. 매핑 후 사용자는 포인터를 통해 NVMe 컨트롤러의 모든 레지스터(CAP, CC, CSTS, 도어벨 등)에 직접 접근할 수 있다.

### 6.2 사용자 공간에서의 BAR0 사용

```c
// src/linux/device.cpp:173
void* mm_ptr = mmap(NULL, mm_size, PROT_READ|PROT_WRITE,
                    MAP_SHARED|MAP_FILE|MAP_LOCKED, dev->fd, 0);
```
사용자 공간 라이브러리가 `/dev/libnvmN`을 열고 `mmap()`을 호출하면 BAR0이 가상 주소에 매핑된다. 이후 `_nvm_ctrl_init()`에서 CAP, CC, CSTS 등 NVMe 레지스터를 직접 읽어 컨트롤러 속성을 파악한다.

### 6.3 GPU에서의 BAR0 접근

```c
// include/ctrl.h:203
cudaHostRegister((void*) ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE, cudaHostRegisterIoMemory);
```
`cudaHostRegisterIoMemory` 플래그가 핵심이다. 이 플래그는 CUDA 런타임에 해당 메모리가 I/O 디바이스의 MMIO 레지스터임을 알려, GPU가 PCIe를 통해 직접 레지스터에 접근할 수 있게 한다.

```c
// include/queue.h:195
cudaHostGetDevicePointer(&devicePtr, (void*) this->cq.db, 0);
this->cq.db = (volatile uint32_t*) devicePtr;  // 도어벨을 GPU 포인터로 교체
```
`cudaHostGetDevicePointer()`로 호스트 가상 주소(BAR0 내 도어벨 오프셋)를 GPU 디바이스 포인터로 변환한다. GPU 커널은 이 포인터로 도어벨에 직접 쓸 수 있다.


## 7. ioctl: DMA 매핑 인터페이스

### 7.1 map_ioctl() 핸들러

```c
// module/pci.c:119
static long map_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
    ctrl = ctrl_find_by_inode(&ctrl_list, file->f_inode);

    switch (cmd) {
        case NVM_MAP_HOST_MEMORY:
            copy_from_user(&request, arg, sizeof(request));
            map = map_userspace(&host_list, ctrl, request.vaddr_start, request.n_pages);
            copy_to_user(request.ioaddrs, map->addrs, map->n_addrs * sizeof(uint64_t));
            break;

        case NVM_MAP_DEVICE_MEMORY:  // #ifdef _CUDA
            copy_from_user(&request, arg, sizeof(request));
            map = map_device_memory(&device_list, ctrl, request.vaddr_start,
                                    request.n_pages, &ctrl_list);
            copy_to_user(request.ioaddrs, map->addrs, map->n_addrs * sizeof(uint64_t));
            break;

        case NVM_UNMAP_MEMORY:
            copy_from_user(&addr, arg, sizeof(u64));
            map = map_find(&host_list, addr);       // 먼저 호스트 리스트 검색
            if (!map) map = map_find(&device_list, addr);  // 없으면 GPU 리스트
            if (map) unmap_and_release(map);
            break;
    }
}
```

### 7.2 매핑 검색: map_find()

```c
// module/map.c:109
struct map* map_find(const struct list* list, u64 vaddr) {
    while (element != NULL) {
        map = container_of(element, struct map, list);
        if (map->owner == current) {  // 현재 프로세스의 매핑만
            if (map->vaddr == (vaddr & PAGE_MASK) ||
                map->vaddr == (vaddr & GPU_PAGE_MASK))
                return map;
        }
        element = list_next(element);
    }
    return NULL;
}
```
보안을 위해 `current` 프로세스가 소유한 매핑만 검색한다. 호스트 4KB 페이지와 GPU 64KB 페이지 양쪽 정렬을 모두 확인한다.


## 8. 전역 리스트 구조

```
┌──────────────┐
│  ctrl_list   │ ─── ctrl[0] ─── ctrl[1] ─── ctrl[2] ─── ...
│ (컨트롤러)    │     /dev/libnvm0  /dev/libnvm1  /dev/libnvm2
└──────────────┘

┌──────────────┐
│  host_list   │ ─── map[0] ─── map[1] ─── ...
│ (호스트 DMA)  │     vaddr=0x7f...  vaddr=0x7f...
│              │     addrs=[버스주소]  addrs=[버스주소]
└──────────────┘

┌──────────────┐
│ device_list  │ ─── map[0] ─── map[1] ─── ...
│ (GPU DMA)    │     vaddr=0x...  vaddr=0x...
│              │     addrs=[P2P주소]  addrs=[P2P주소]
└──────────────┘
```


## 9. 모듈 언로드 흐름

```c
// module/pci.c:434
static void __exit libnvm_helper_exit(void) {
    clear_map_list(&device_list);   // GPU 매핑 강제 해제
    clear_map_list(&host_list);     // 호스트 매핑 강제 해제
    pci_unregister_driver(&driver); // → 각 디바이스에 remove_pci_dev 호출
    class_destroy(dev_class);       // sysfs 클래스 파괴
    unregister_chrdev_region(dev_first, max_num_ctrls);  // 번호 반환
}
```

```
rmmod libnvm
    │
    ▼
libnvm_helper_exit()
    ├── clear_map_list(&device_list)    ← 미해제 GPU 매핑 정리
    │   └── unmap_and_release() 반복
    │       └── release_gpu_memory() → nvidia_p2p_put_pages()
    ├── clear_map_list(&host_list)      ← 미해제 호스트 매핑 정리
    │   └── unmap_and_release() 반복
    │       └── release_user_pages() → dma_unmap_page() + put_page()
    ├── pci_unregister_driver()         ← PCI 드라이버 등록 해제
    │   └── remove_pci_dev() (각 디바이스에 대해)
    │       ├── ctrl_put()              ← 컨트롤러 참조 해제 + chrdev 제거
    │       ├── pci_release_region(0)   ← BAR0 예약 해제
    │       ├── pci_clear_master()      ← 버스 마스터 비활성화
    │       └── pci_disable_device()    ← PCI 디바이스 비활성화
    ├── class_destroy()                 ← sysfs 클래스 파괴
    └── unregister_chrdev_region()      ← 캐릭터 디바이스 번호 반환
```


## 10. Linux 표준 NVMe 드라이버와의 비교

### 10.1 기능 비교표

| 기능 | Linux NVMe 드라이버 | BaM 커널 모듈 |
|------|---------------------|---------------|
| **I/O 경로** | 커널 블록 레이어 경유 | GPU 커널에서 직접 |
| **커맨드 처리** | 커널 blk-mq 프레임워크 | GPU 스레드가 직접 |
| **인터럽트** | MSI-X 인터럽트 + irq 핸들러 | GPU 폴링 (인터럽트 없음) |
| **큐 관리** | 커널이 SQ/CQ 생성/관리 | 사용자 공간 Admin 커맨드 |
| **DMA 매핑** | bio → scatter-gather → dma_map_sg | 사전 매핑된 GPU 메모리 (P2P) |
| **블록 디바이스** | /dev/nvmeXnY 블록 디바이스 | 없음 (캐릭터 디바이스만) |
| **파일시스템** | ext4, xfs 등 지원 | 지원 없음 (raw 접근) |
| **I/O 스케줄러** | mq-deadline, kyber, none | 없음 |
| **NUMA 인식** | 큐를 CPU에 바인딩 | GPU SM에 큐 할당 |
| **에러 처리** | 커널 수준 리셋/복구 | 제한적 (타임아웃만) |
| **보안** | 커널 권한 보호 | root 필요 (/dev/libnvmN) |
| **도어벨** | 커널에서 MMIO 쓰기 | GPU에서 PCIe MMIO 직접 쓰기 |

### 10.2 아키텍처 비교

```
Linux 표준 NVMe 드라이버:
┌──────┐   ┌──────┐   ┌──────┐   ┌────────┐   ┌──────┐   ┌──────┐
│ App  │→ │ VFS  │→ │Block │→ │blk-mq  │→ │NVMe  │→ │ SSD  │
│      │   │      │   │Layer │   │        │   │Driver│   │      │
└──────┘   └──────┘   └──────┘   └────────┘   └──────┘   └──────┘
  유저        커널        커널        커널         커널       디바이스

BaM:
┌──────┐   ┌───────────────────────────────────────┐   ┌──────┐
│ GPU  │→ │  직접 SQ 쓰기 → 도어벨 → CQ 폴링        │→ │ SSD  │
│커널  │   │  (커널 바이패스, 인터럽트 없음)            │   │      │
└──────┘   └───────────────────────────────────────┘   └──────┘
  GPU                  PCIe P2P                        디바이스
```

### 10.3 BaM이 커널 드라이버를 바이패스하는 이유

1. **지연시간 제거**: 커널 블록 레이어, I/O 스케줄러, 인터럽트 핸들러 경유 없이 GPU에서 NVMe에 직접 접근하여 마이크로초 수준의 지연시간을 절감한다
2. **CPU 바이패스**: 데이터가 CPU 메모리를 경유하지 않고 NVMe→GPU 직접 전송되어 CPU 오버헤드와 메모리 대역폭 소비를 제거한다
3. **GPU 병렬성 활용**: 수천 개의 GPU 스레드가 동시에 I/O를 발행할 수 있어 NVMe SSD의 높은 큐 깊이를 자연스럽게 활용한다
4. **인터럽트 오버헤드 제거**: 완료를 인터럽트 대신 GPU 스레드가 직접 폴링하여 인터럽트 처리 비용을 제거한다


## 11. 모듈 파라미터

```c
// module/pci.c:73
static int max_num_ctrls = 64;
module_param(max_num_ctrls, int, 0);
MODULE_PARM_DESC(max_num_ctrls, "Number of controller devices");
```
`insmod libnvm.ko max_num_ctrls=128`처럼 모듈 로드 시 최대 컨트롤러 수를 지정할 수 있다. 이 값은 캐릭터 디바이스 번호 범위 할당과 GPU P2P 매핑 배열 크기를 결정한다.


## 12. 관련 파일 요약

| 파일 경로 | 역할 |
|-----------|------|
| `module/pci.c` | 모듈 메인: 초기화/종료, PCI probe/remove, ioctl, mmap |
| `module/map.c` | DMA 매핑: get_user_pages, nvidia_p2p_*, 매핑 리스트 관리 |
| `module/map.h` | struct map 정의, 매핑 함수 선언 |
| `module/ctrl.c` | 컨트롤러 관리: ctrl_get/put, chrdev 생성/제거 |
| `module/ctrl.h` | struct ctrl 정의 |
| `module/list.h` | 연결 리스트 유틸리티 |
| `src/linux/ioctl.h` | ioctl 명령 코드, nvm_ioctl_map 구조체 |
| `src/linux/device.cpp` | 사용자 공간: 컨트롤러 초기화, ioctl_map/unmap |
| `src/linux/dma.cpp` | 사용자 공간: DMA API (nvm_dma_create, nvm_dma_map_host/device) |
