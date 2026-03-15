# 13. DMA 서브시스템과 IOMMU: NVMe I/O의 메모리 접근 메커니즘

## 개요

NVMe 디바이스가 호스트 메모리의 데이터를 읽고 쓰려면 DMA(Direct Memory Access)를 사용한다. 이 문서는 Linux 커널의 DMA 서브시스템이 어떻게 동작하는지, IOMMU가 어떤 역할을 하는지, 그리고 NVMe 드라이버가 이 API들을 어떻게 사용하는지를 소스 코드 레벨에서 분석한다.

관련 소스 파일:
- `include/linux/dma-mapping.h` - DMA API 헤더
- `kernel/dma/mapping.c` - DMA 매핑 코어 로직
- `kernel/dma/direct.c` - IOMMU 없이 직접 DMA 매핑
- `kernel/dma/swiotlb.c` - Software I/O TLB (bounce buffer)
- `drivers/iommu/iommu.c` - IOMMU 프레임워크
- `drivers/nvme/host/pci.c` - NVMe PCIe 드라이버
- `include/linux/scatterlist.h` - Scatter-Gather List 구조체

---

## 1. DMA란 무엇인가 (기초부터)

### 1.1 DMA의 정의

DMA(Direct Memory Access)란 CPU의 개입 없이 디바이스가 시스템 메모리를 직접 읽고 쓰는 메커니즘이다. CPU가 모든 데이터 전송을 중개하면 막대한 오버헤드가 발생하므로, 고속 I/O 디바이스(NVMe, 네트워크 카드, GPU 등)는 반드시 DMA를 사용한다.

### 1.2 Programmed I/O vs DMA 비교

```
=== Programmed I/O (PIO) ===

  CPU                    Memory                  Device
   │                       │                       │
   │  (1) read from device │                       │
   │───────────────────────┼──────────────────────>│
   │<──────────────────────┼───────────────────────│ data
   │                       │                       │
   │  (2) write to memory  │                       │
   │──────────────────────>│                       │
   │                       │                       │
   │  ... CPU가 바이트마다 반복 (매우 느림) ...     │
   │                       │                       │

  문제: CPU가 매 바이트/워드마다 중개해야 함
        → CPU 사이클 낭비, 대역폭 제한


=== DMA ===

  CPU                    Memory                  Device
   │                       │                       │
   │  (1) "주소 X에서       │                       │
   │       N바이트 읽어라"  │                       │
   │──────────────────────────────────────────────>│ 커맨드
   │                       │                       │
   │  CPU는 다른 일 수행    │  (2) Device가 직접     │
   │  ~~~~~~~~~~~~~~~~~~~~  │      메모리 접근       │
   │                       │<─────────────────────│ DMA Read
   │                       │─────────────────────>│ DMA Write
   │                       │                       │
   │  (3) 완료 인터럽트     │                       │
   │<──────────────────────────────────────────────│ IRQ
   │                       │                       │

  장점: CPU 해방, 높은 대역폭 (PCIe 링크 속도)
```

### 1.3 DMA의 두 가지 방향

DMA 전송에는 방향(direction)이 있다:

| 방향 | 의미 | NVMe 예시 |
|------|------|-----------|
| `DMA_TO_DEVICE` | Memory → Device | 호스트가 쓰기 데이터를 NVMe로 전송 |
| `DMA_FROM_DEVICE` | Device → Memory | NVMe가 읽기 데이터를 호스트 메모리에 기록 |
| `DMA_BIDIRECTIONAL` | 양방향 | SQ/CQ처럼 양쪽이 모두 접근하는 경우 |

방향 지정은 캐시 동기화에서 중요하다. `DMA_TO_DEVICE`이면 CPU 캐시를 flush해야 하고, `DMA_FROM_DEVICE`이면 CPU 캐시를 invalidate해야 한다.

### 1.4 NVMe에서 DMA가 사용되는 4가지 경우

```
┌─────────────────────────────────────────────────────────────┐
│                    호스트 메모리                              │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────┐  │
│  │ SQ 버퍼   │  │ CQ 버퍼   │  │ PRP List │  │ 데이터 버퍼 │  │
│  │          │  │          │  │          │  │            │  │
│  │ Coherent │  │ Coherent │  │ DMA Pool │  │ Streaming  │  │
│  │   DMA    │  │   DMA    │  │   DMA    │  │    DMA     │  │
│  └─────┬────┘  └────┬─────┘  └─────┬────┘  └─────┬──────┘  │
│        │            │              │              │          │
└────────┼────────────┼──────────────┼──────────────┼──────────┘
         │            │              │              │
    ①SQ Fetch    ②CQ Post     ③PRP/SGL Fetch  ④Data R/W
         │            │              │              │
         ▼            │              ▼              ▼
┌────────────────────────────────────────────────────────────┐
│                   NVMe 컨트롤러                             │
│                                                            │
│  ① SQ Fetch: 컨트롤러가 SQ에서 커맨드를 DMA Read           │
│  ② CQ Post:  컨트롤러가 CQ에 완료 엔트리를 DMA Write       │
│  ③ PRP/SGL:  컨트롤러가 PRP List를 DMA Read하여            │
│              데이터 버퍼 주소를 파악                         │
│  ④ Data:     컨트롤러가 데이터 버퍼에 DMA Read/Write        │
└────────────────────────────────────────────────────────────┘
```

| # | DMA 대상 | 방향 | DMA 유형 | 설명 |
|---|---------|------|---------|------|
| ① | SQ 버퍼 | Device ← Memory | Coherent | 컨트롤러가 SQ에서 커맨드를 가져감 |
| ② | CQ 버퍼 | Device → Memory | Coherent | 컨트롤러가 CQ에 완료 엔트리를 씀 |
| ③ | PRP/SGL | Device ← Memory | DMA Pool | 컨트롤러가 PRP List를 읽어 데이터 주소 파악 |
| ④ | 데이터 | 양방향 | Streaming | 실제 사용자 데이터 전송 |

---

## 2. 주소 변환 문제: 왜 DMA가 복잡한가

### 2.1 CPU와 디바이스의 주소 공간 차이

CPU는 가상 주소(Virtual Address)를 사용한다. 프로세스가 `malloc()`으로 할당받은 버퍼의 주소는 가상 주소이며, MMU(Memory Management Unit)가 이를 물리 주소(Physical Address)로 변환한다.

반면 디바이스는 버스 주소(Bus Address)를 사용하여 DMA를 수행한다. x86 시스템에서 IOMMU가 없으면 버스 주소 = 물리 주소이지만, IOMMU가 있으면 버스 주소 = IOVA(I/O Virtual Address)이다.

### 2.2 물리 메모리의 비연속성 문제

사용자가 4KB 이상의 버퍼를 할당하면, 이 버퍼는 가상 주소 공간에서는 연속이지만 물리 메모리에서는 연속이 아닐 수 있다. 각 페이지가 물리 메모리의 다른 위치에 흩어져 있을 수 있다.

```
가상 주소 공간 (연속)              물리 메모리 (비연속)
┌──────────────────┐
│  Page 0 (VA)     │ ────────>  물리 주소 0x1000_0000
├──────────────────┤
│  Page 1 (VA)     │ ────────>  물리 주소 0x5000_0000  ← 멀리 떨어져 있음!
├──────────────────┤
│  Page 2 (VA)     │ ────────>  물리 주소 0x2000_8000
├──────────────────┤
│  Page 3 (VA)     │ ────────>  물리 주소 0x8000_4000
└──────────────────┘
```

이 문제를 해결하기 위해 Scatter-Gather(SG) 메커니즘이 필요하다. 디바이스에게 "이 주소에서 이만큼, 저 주소에서 저만큼" 식으로 여러 물리 영역을 나열해주는 것이다. NVMe에서는 이것이 PRP List 또는 SGL이다.

### 2.3 IOMMU 유무에 따른 주소 변환

```
=== IOMMU가 없는 경우 ===

  CPU 가상 주소                           물리 주소
  ┌─────────┐      MMU                ┌─────────┐
  │  0x7fff  │ ──────────────────────> │ 0x1000  │ ← 디바이스도 이 주소 사용
  │  ...     │   페이지 테이블 변환     │  ...    │
  └─────────┘                         └─────────┘
                                           │
                                      DMA 주소 = 물리 주소
                                           │
                                           ▼
                                    ┌─────────────┐
                                    │ NVMe Device  │
                                    └─────────────┘


=== IOMMU가 있는 경우 ===

  CPU 가상 주소          물리 주소          IOVA (I/O VA)
  ┌─────────┐         ┌─────────┐        ┌─────────┐
  │  0x7fff  │──MMU──>│ 0x1000  │        │ 0xA000  │
  │  ...     │        │  ...    │        │  ...    │
  └─────────┘         └─────────┘        └─────────┘
                           │                  │
                           │    IOMMU         │
                           │  페이지 테이블    │
                           │<────────────────│
                           │  IOVA → PA 변환  │
                                              │
                                         DMA 주소 = IOVA
                                              │
                                              ▼
                                       ┌─────────────┐
                                       │ NVMe Device  │
                                       └─────────────┘
```

핵심 차이:
- **IOMMU 없음**: 디바이스가 물리 주소를 직접 사용한다. 흩어진 페이지들을 하나하나 SG 리스트로 나열해야 한다.
- **IOMMU 있음**: IOMMU가 흩어진 물리 페이지들을 연속된 IOVA 공간으로 매핑할 수 있다. 디바이스 입장에서는 연속된 하나의 DMA 주소를 본다 (SG merging).

---

## 3. Linux DMA API

Linux 커널은 디바이스 드라이버가 DMA를 수행할 때 사용하는 통합 API를 제공한다. 이 API는 IOMMU 유무에 관계없이 동일한 인터페이스를 제공하며, 내부적으로 적절한 백엔드(direct, iommu, swiotlb)를 선택한다.

### 3.1 Coherent DMA (dma_alloc_coherent)

#### 용도

CPU와 디바이스가 동시에 접근하는 공유 메모리 영역에 사용한다. 캐시 일관성(cache coherency)이 하드웨어적으로 보장되므로, 별도의 sync 호출이 필요 없다. NVMe의 SQ/CQ가 대표적인 예다.

#### API

```c
/* include/linux/dma-mapping.h */
static inline void *dma_alloc_coherent(struct device *dev, size_t size,
        dma_addr_t *dma_handle, gfp_t gfp)
{
    return dma_alloc_attrs(dev, size, dma_handle, gfp,
            (gfp & __GFP_NOWARN) ? DMA_ATTR_NO_WARN : 0);
}
```

반환값:
- CPU가 사용할 가상 주소 (함수 리턴값)
- 디바이스가 사용할 DMA 주소 (`*dma_handle`에 저장)

#### 내부 동작 (kernel/dma/mapping.c)

`dma_alloc_attrs()` 함수가 실제 할당을 수행한다:

```c
void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle,
        gfp_t flag, unsigned long attrs)
{
    const struct dma_map_ops *ops = get_dma_ops(dev);
    void *cpu_addr;

    if (dma_alloc_direct(dev, ops)) {
        cpu_addr = dma_direct_alloc(dev, size, dma_handle, flag, attrs);
    } else if (use_dma_iommu(dev)) {
        cpu_addr = iommu_dma_alloc(dev, size, dma_handle, flag, attrs);
    } else if (ops->alloc) {
        cpu_addr = ops->alloc(dev, size, dma_handle, flag, attrs);
    }
    ...
}
```

세 가지 경로가 있다:
1. **dma_direct**: IOMMU 없이 직접 물리 메모리를 할당한다. `dma_direct_alloc()`이 페이지를 할당하고 `phys_to_dma()`로 DMA 주소를 계산한다.
2. **iommu_dma**: IOMMU를 통해 메모리를 할당하고 IOVA를 매핑한다.
3. **ops->alloc**: 플랫폼별 커스텀 할당자.

#### NVMe에서의 사용: SQ/CQ 할당

`drivers/nvme/host/pci.c`의 `nvme_alloc_queue()`에서 SQ와 CQ 버퍼를 할당한다:

```c
/* CQ 버퍼 할당 (line 2593) */
nvmeq->cqes = dma_alloc_coherent(dev->dev, CQ_SIZE(nvmeq),
                 &nvmeq->cq_dma_addr, GFP_KERNEL);

/* SQ 버퍼 할당 (line 2563) */
nvmeq->sq_cmds = dma_alloc_coherent(dev->dev, SQ_SIZE(nvmeq),
                &nvmeq->sq_dma_addr, GFP_KERNEL);
```

왜 Coherent DMA를 사용하는가:
- SQ: 호스트(CPU)가 커맨드를 쓰고, 컨트롤러(디바이스)가 DMA로 읽는다. 양쪽이 동시에 접근한다.
- CQ: 컨트롤러가 완료 엔트리를 DMA로 쓰고, 호스트가 읽는다. 역시 양쪽이 동시에 접근한다.
- 캐시 일관성 없이는 CPU 캐시에 오래된 데이터가 남아 완료를 놓칠 수 있다.

`sq_dma_addr`과 `cq_dma_addr`은 나중에 Create I/O Queue 커맨드로 컨트롤러에 전달된다. 컨트롤러는 이 DMA 주소를 사용하여 SQ에서 커맨드를 fetch하고, CQ에 완료 엔트리를 post한다.

#### Shadow Doorbell Buffer도 Coherent DMA

```c
/* line 483-490 */
dev->dbbuf_dbs = dma_alloc_coherent(dev->dev, mem_size,
                    &dev->dbbuf_dbs_dma_addr, GFP_KERNEL);
dev->dbbuf_eis = dma_alloc_coherent(dev->dev, mem_size,
                    &dev->dbbuf_eis_dma_addr, GFP_KERNEL);
```

Shadow Doorbell과 Event Index 버퍼도 호스트와 컨트롤러가 동시에 접근하므로 Coherent DMA를 사용한다.

### 3.2 Streaming DMA (dma_map_sg / dma_map_single / dma_map_page)

#### 용도

일시적으로 디바이스에 매핑하는 메모리이다. 데이터 전송이 끝나면 매핑을 해제한다. NVMe의 사용자 데이터 전송에 사용된다. Coherent DMA보다 오버헤드가 적고, 기존에 할당된 메모리를 매핑할 수 있다.

#### API

```c
/* include/linux/dma-mapping.h */

/* 단일 연속 버퍼 매핑 */
static inline dma_addr_t dma_map_single_attrs(struct device *dev, void *ptr,
        size_t size, enum dma_data_direction dir, unsigned long attrs)
{
    /* vmalloc 영역은 거부 */
    if (dev_WARN_ONCE(dev, is_vmalloc_addr(ptr),
              "rejecting DMA map of vmalloc memory\n"))
        return DMA_MAPPING_ERROR;
    return dma_map_page_attrs(dev, virt_to_page(ptr), offset_in_page(ptr),
            size, dir, attrs);
}

/* Scatter-Gather 리스트 매핑 */
unsigned int dma_map_sg_attrs(struct device *dev, struct scatterlist *sg,
        int nents, enum dma_data_direction dir, unsigned long attrs);

/* sg_table 기반 매핑 (현대적 API) */
int dma_map_sgtable(struct device *dev, struct sg_table *sgt,
        enum dma_data_direction dir, unsigned long attrs);
```

#### 방향 지정의 의미

```c
/* include/linux/dma-direction.h */
enum dma_data_direction {
    DMA_BIDIRECTIONAL = 0,  /* 양방향 */
    DMA_TO_DEVICE     = 1,  /* Memory → Device (쓰기) */
    DMA_FROM_DEVICE   = 2,  /* Device → Memory (읽기) */
    DMA_NONE          = 3,  /* DMA 없음 */
};
```

방향에 따른 캐시 동작:
- `DMA_TO_DEVICE`: 매핑 시 CPU 캐시를 flush(clean) → 디바이스가 최신 데이터를 읽을 수 있도록
- `DMA_FROM_DEVICE`: 언매핑 시 CPU 캐시를 invalidate → CPU가 디바이스가 쓴 새 데이터를 읽을 수 있도록
- `DMA_BIDIRECTIONAL`: 양쪽 모두 수행

x86에서는 캐시 일관성이 하드웨어로 보장되므로(DMA coherent architecture) 이 동작이 no-op이다. ARM 등 non-coherent 아키텍처에서는 실제로 캐시 flush/invalidate가 필요하다.

#### 내부 동작: __dma_map_sg_attrs()

```c
/* kernel/dma/mapping.c */
static int __dma_map_sg_attrs(struct device *dev, struct scatterlist *sg,
     int nents, enum dma_data_direction dir, unsigned long attrs)
{
    const struct dma_map_ops *ops = get_dma_ops(dev);

    if (dma_map_direct(dev, ops) ||
        arch_dma_map_sg_direct(dev, sg, nents))
        ents = dma_direct_map_sg(dev, sg, nents, dir, attrs);
    else if (use_dma_iommu(dev))
        ents = iommu_dma_map_sg(dev, sg, nents, dir, attrs);
    else
        ents = ops->map_sg(dev, sg, nents, dir, attrs);
    ...
}
```

역시 세 가지 경로로 분기한다:
1. **dma_direct_map_sg**: 각 scatterlist 엔트리에 대해 `phys_to_dma()`로 물리 주소를 DMA 주소로 변환한다. 엔트리 수는 변하지 않는다.
2. **iommu_dma_map_sg**: IOMMU를 통해 매핑한다. 흩어진 물리 페이지들을 연속된 IOVA로 합칠 수 있으므로(SG merging), 반환되는 엔트리 수가 입력보다 적을 수 있다.
3. **ops->map_sg**: 플랫폼별 구현.

#### Sync 동작

Streaming DMA는 소유권(ownership) 개념이 있다. 매핑 후에는 디바이스가 버퍼를 소유하고, CPU가 접근하려면 sync가 필요하다:

```c
/* CPU가 버퍼를 읽으려면 (DMA 완료 후) */
dma_sync_single_for_cpu(dev, dma_addr, size, dir);

/* 디바이스에 다시 넘기려면 */
dma_sync_single_for_device(dev, dma_addr, size, dir);
```

`kernel/dma/mapping.c`에서:

```c
void __dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr,
        size_t size, enum dma_data_direction dir)
{
    if (dma_map_direct(dev, ops))
        dma_direct_sync_single_for_cpu(dev, addr, size, dir);
    else if (use_dma_iommu(dev))
        iommu_dma_sync_single_for_cpu(dev, addr, size, dir);
    ...
}
```

x86 DMA coherent 아키텍처에서는 `dev->dma_skip_sync = true`로 설정되어 이 함수가 스킵된다. SWIOTLB bounce buffer를 사용하는 경우에만 실제 데이터 복사(sync)가 수행된다.

### 3.3 DMA Pool (dma_pool_alloc)

#### 용도

`dma_alloc_coherent()`는 최소 1페이지(4KB) 단위로 할당한다. PRP List처럼 수백 바이트만 필요한 경우에 4KB씩 할당하면 메모리가 낭비된다. DMA Pool은 큰 DMA coherent 메모리 블록에서 작은 단위를 효율적으로 잘라서 제공한다.

#### NVMe에서의 사용: PRP/SGL 디스크립터 풀

```c
/* drivers/nvme/host/pci.c, line 226-229 */
struct nvme_descriptor_pools {
    struct dma_pool *large;   /* 4KB - 큰 PRP 리스트용 */
    struct dma_pool *small;   /* 256B - 작은 I/O 최적화용 */
};
```

초기화 코드 (`nvme_setup_descriptor_pools`, line 629-655):

```c
/* large 풀: 4KB 크기, 4KB 정렬 */
pools->large = dma_pool_create_node("nvme descriptor page", dev->dev,
        NVME_CTRL_PAGE_SIZE, NVME_CTRL_PAGE_SIZE, 0, numa_node);

/* small 풀: 256B 크기, 256B 정렬 */
pools->small = dma_pool_create_node("nvme descriptor small", dev->dev,
        NVME_SMALL_POOL_SIZE, small_align, 0, numa_node);
```

사용 코드 (PRP 리스트 할당, line 1220):

```c
prp_list = dma_pool_alloc(nvme_dma_pool(nvmeq, iod), GFP_ATOMIC, &prp2_dma);
```

`dma_pool_alloc()`은 이미 `dma_alloc_coherent()`로 할당된 큰 블록에서 작은 청크를 잘라 반환한다. 반환값은 CPU 가상 주소이고, `prp2_dma`에 DMA 주소가 저장된다.

풀 선택 로직:

```c
static inline struct dma_pool *nvme_dma_pool(struct nvme_queue *nvmeq,
        struct nvme_iod *iod)
{
    if (iod->flags & IOD_SMALL_DESCRIPTOR)
        return nvmeq->descriptor_pools.small;    /* 256B 풀 */
    return nvmeq->descriptor_pools.large;         /* 4KB 풀 */
}
```

필요한 PRP 엔트리가 적으면(`NVME_SMALL_POOL_SIZE / sizeof(__le64)` = 32개 이하) small 풀을 사용하여 메모리를 절약한다.

---

## 4. Scatter-Gather List (SGL)

### 4.1 struct scatterlist

```c
/* include/linux/scatterlist.h */
struct scatterlist {
    unsigned long   page_link;    /* 페이지 포인터 + 플래그 (하위 2비트) */
    unsigned int    offset;       /* 페이지 내 오프셋 */
    unsigned int    length;       /* 데이터 길이 */
    dma_addr_t      dma_address;  /* DMA 매핑 후 디바이스 주소 */
#ifdef CONFIG_NEED_SG_DMA_LENGTH
    unsigned int    dma_length;   /* DMA 매핑 후 길이 (merging 시 다를 수 있음) */
#endif
};
```

`page_link`의 하위 2비트는 특별한 의미를 가진다:
- bit 0 (`SG_CHAIN`): 이 엔트리가 다음 scatterlist 배열을 가리키는 체인 포인터
- bit 1 (`SG_END`): 이 엔트리가 리스트의 마지막

### 4.2 struct sg_table

```c
struct sg_table {
    struct scatterlist *sgl;    /* scatterlist 배열 포인터 */
    unsigned int nents;         /* DMA 매핑된 엔트리 수 */
    unsigned int orig_nents;    /* 원래 엔트리 수 (매핑 전) */
};
```

`nents`와 `orig_nents`의 차이가 중요하다:
- `orig_nents`: 물리 페이지 수 (DMA 매핑 전)
- `nents`: DMA 매핑 후 엔트리 수 (IOMMU merging으로 줄어들 수 있음)

### 4.3 bio_vec에서 scatterlist로의 변환

block layer의 I/O 요청은 `bio_vec` 배열로 데이터를 기술한다:

```c
struct bio_vec {
    struct page     *bv_page;    /* 페이지 */
    unsigned int    bv_len;      /* 길이 */
    unsigned int    bv_offset;   /* 페이지 내 오프셋 */
};
```

`bio_vec`과 `scatterlist`는 구조가 비슷하다. block layer의 `blk_rq_map_sg()` 함수가 요청의 `bio_vec`들을 `scatterlist` 배열로 변환한다. 이 과정에서 인접한 물리 페이지들이 하나의 SG 엔트리로 합쳐질 수 있다.

### 4.4 dma_map_sg(): 물리 페이지들을 DMA 주소로 매핑

```
=== dma_map_sg 동작 (IOMMU 없는 경우) ===

  scatterlist 입력              dma_map_sg 후
  ┌────────────────┐           ┌────────────────┐
  │ page=0x1000    │           │ dma_addr=0x1000│  ← PA = DMA addr
  │ len=4096       │    ──>    │ dma_len=4096   │
  ├────────────────┤           ├────────────────┤
  │ page=0x5000    │           │ dma_addr=0x5000│
  │ len=4096       │    ──>    │ dma_len=4096   │
  ├────────────────┤           ├────────────────┤
  │ page=0x2000    │           │ dma_addr=0x2000│
  │ len=4096       │    ──>    │ dma_len=4096   │
  └────────────────┘           └────────────────┘
  nents = 3                    nents = 3  (변화 없음)


=== dma_map_sg 동작 (IOMMU가 있는 경우 - SG merging) ===

  scatterlist 입력              dma_map_sg 후
  ┌────────────────┐           ┌────────────────┐
  │ page=0x1000    │           │ dma_addr=0xA000│  ← IOVA
  │ len=4096       │    ──>    │ dma_len=12288  │  ← 3 페이지 합침!
  ├────────────────┤           └────────────────┘
  │ page=0x5000    │
  │ len=4096       │    ──>    (merged)
  ├────────────────┤
  │ page=0x2000    │
  │ len=4096       │    ──>    (merged)
  └────────────────┘
  nents = 3                    nents = 1  (3→1로 축소!)
```

IOMMU SG merging의 장점:
- PRP/SGL 엔트리 수 감소 → PRP List 할당 불필요 또는 크기 감소
- 디바이스가 연속된 주소로 DMA → PCIe TLP 효율 향상
- 큰 전송 단위 → 더 높은 대역폭

### 4.5 현대 NVMe 드라이버의 DMA 이터레이터

최신 커널의 NVMe 드라이버는 `blk_dma_iter` 기반의 새로운 DMA 매핑 방식을 사용한다. 전통적인 `blk_rq_map_sg()` + `dma_map_sg()` 대신 `blk_rq_dma_map_iter_start()`와 `blk_rq_dma_map_iter_next()`로 SG 리스트 구축과 DMA 매핑을 동시에 수행한다:

```c
struct blk_dma_iter {
    dma_addr_t addr;     /* 현재 세그먼트의 DMA 주소 */
    unsigned int len;    /* 현재 세그먼트의 길이 */
    blk_status_t status; /* 에러 상태 */
    ...
};
```

이 방식은 별도의 SG 테이블 할당 없이 요청의 bio_vec을 직접 순회하면서 DMA 매핑을 수행하므로 메모리 할당 오버헤드가 줄어든다.

---

## 5. NVMe에서 DMA가 사용되는 전체 흐름

### 5.1 SQ/CQ: Coherent DMA

```
┌──────────────────────────────────────────────────────────────┐
│  nvme_alloc_queue()                                          │
│                                                              │
│  (1) dma_alloc_coherent(CQ_SIZE)                             │
│       → 물리적으로 연속된 메모리 할당                          │
│       → CPU 주소: nvmeq->cqes (커널이 CQ를 읽을 때 사용)     │
│       → DMA 주소: nvmeq->cq_dma_addr                        │
│                                                              │
│  (2) dma_alloc_coherent(SQ_SIZE) 또는 CMB 사용               │
│       → CPU 주소: nvmeq->sq_cmds (커널이 SQ에 쓸 때 사용)    │
│       → DMA 주소: nvmeq->sq_dma_addr                        │
│                                                              │
│  (3) Create I/O CQ/SQ 커맨드로 DMA 주소를 컨트롤러에 전달    │
│       → 컨트롤러는 sq_dma_addr에서 SQE를 DMA Read            │
│       → 컨트롤러는 cq_dma_addr에 CQE를 DMA Write            │
└──────────────────────────────────────────────────────────────┘
```

Coherent DMA의 핵심 특성:
- 물리적으로 연속 → DMA 주소 하나로 전체 버퍼를 기술할 수 있음
- 캐시 일관성 보장 → CPU가 SQ에 쓴 즉시 디바이스가 볼 수 있고, 디바이스가 CQ에 쓴 즉시 CPU가 볼 수 있음
- 디바이스 수명 동안 유지 → 매핑/해제 반복 없음

### 5.2 데이터 전송: Streaming DMA + PRP/SGL

데이터 전송 경로는 더 복잡하다. 매 I/O 요청마다 데이터 버퍼를 DMA 매핑하고, 그 주소를 PRP 또는 SGL에 기록해야 한다.

```
┌──────────────────────────────────────────────────────────────────┐
│  I/O 요청의 데이터 DMA 매핑 흐름                                  │
│                                                                  │
│  (1) 사용자 버퍼                                                 │
│       │  write(fd, buf, 128KB)                                   │
│       ▼                                                          │
│  (2) bio_vec 배열                                                │
│       │  [page0,4KB] [page1,4KB] ... [page31,4KB]                │
│       ▼                                                          │
│  (3) DMA 이터레이터로 순회하며 매핑                               │
│       │  blk_rq_dma_map_iter_start() → 첫 세그먼트 매핑          │
│       │  blk_rq_dma_map_iter_next()  → 다음 세그먼트 매핑        │
│       │                                                          │
│       │  IOMMU 없음: 각 페이지 → 개별 DMA 주소                   │
│       │  IOMMU 있음: 여러 페이지 → 하나의 연속 IOVA (가능한 경우) │
│       ▼                                                          │
│  (4) PRP List 또는 SGL에 DMA 주소 기록                           │
│       │  PRP: [dma0][dma1][dma2]...[dma31]                       │
│       │  SGL: [addr0,len0][addr1,len1]...                        │
│       ▼                                                          │
│  (5) SQE.DPTR에 PRP1/PRP2 또는 SGL 설정                         │
│       │  PRP1 = 첫 번째 DMA 주소                                 │
│       │  PRP2 = PRP List의 DMA 주소 (DMA Pool에서 할당)          │
│       ▼                                                          │
│  (6) Doorbell → 컨트롤러가 SQE fetch → PRP List fetch            │
│       → 데이터 DMA Read/Write                                    │
└──────────────────────────────────────────────────────────────────┘
```

### 5.3 nvme_map_data() 상세 분석

`nvme_map_data()`는 NVMe I/O의 데이터 DMA 매핑을 담당하는 핵심 함수이다 (line 1425-1465).

```c
static blk_status_t nvme_map_data(struct request *req)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
    struct nvme_queue *nvmeq = req->mq_hctx->driver_data;
    struct nvme_dev *dev = nvmeq->dev;
    enum nvme_use_sgl use_sgl = nvme_pci_use_sgls(dev, req);
    struct blk_dma_iter iter;
    blk_status_t ret;
```

**단계 1: 단일 세그먼트 빠른 경로** (line 1438-1442)

```c
    if (blk_rq_nr_phys_segments(req) == 1) {
        ret = nvme_pci_setup_data_simple(req, use_sgl);
        if (ret != BLK_STS_AGAIN)
            return ret;
    }
```

대부분의 4KB 랜덤 I/O는 단일 세그먼트이다. 이 경우 `nvme_pci_setup_data_simple()`이 DMA 이터레이터 없이 직접 `dma_map_bvec()`를 호출하여 성능을 최적화한다:

```c
/* nvme_pci_setup_data_simple, line 1372-1409 */
dma_addr = dma_map_bvec(nvmeq->dev->dev, &bv, rq_dma_dir(req), 0);
iod->flags |= IOD_SINGLE_SEGMENT;

/* PRP 가능하면 PRP1/PRP2에 직접 설정 */
iod->cmd.common.dptr.prp1 = cpu_to_le64(dma_addr);
if (bv.bv_len > first_prp_len)
    iod->cmd.common.dptr.prp2 = cpu_to_le64(dma_addr + first_prp_len);
```

단일 세그먼트 + PRP 가능한 경우: DMA Pool 할당 없이 PRP1/PRP2만으로 완료된다. 이것이 가장 빠른 경로이다.

**단계 2: 다중 세그먼트 DMA 이터레이터 시작** (line 1444-1445)

```c
    if (!blk_rq_dma_map_iter_start(req, dev->dev, &iod->dma_state, &iter))
        return iter.status;
```

`blk_rq_dma_map_iter_start()`가 첫 번째 세그먼트를 DMA 매핑하고 `iter`에 결과를 저장한다.

**단계 3: PRP vs SGL 선택** (line 1460-1464)

```c
    if (use_sgl == SGL_FORCED ||
        (use_sgl == SGL_SUPPORTED &&
         (sgl_threshold && nvme_pci_avg_seg_size(req) >= sgl_threshold)))
        return nvme_pci_setup_data_sgl(req, &iter);
    return nvme_pci_setup_data_prp(req, &iter);
```

- 컨트롤러가 SGL만 지원하면 (`SGL_FORCED`): SGL 경로
- 컨트롤러가 SGL도 지원하고 평균 세그먼트 크기가 threshold 이상이면: SGL 경로
- 그 외: PRP 경로 (기본)

**PRP 리스트 구축 흐름 (nvme_pci_setup_data_prp):**

```
  iter에서 DMA 주소 순회
       │
       ▼
  PRP1 = 첫 번째 DMA 주소 (비정렬 가능)
       │
       ▼
  length <= 1 page?  ──yes──>  PRP2 = 0, 완료
       │ no
       ▼
  length <= 2 pages? ──yes──>  PRP2 = 두 번째 DMA 주소, 완료
       │ no
       ▼
  PRP 리스트 필요:
  dma_pool_alloc() → PRP 리스트 페이지 할당
  PRP2 = PRP 리스트의 DMA 주소
       │
       ▼
  for each remaining segment:
      prp_list[i] = DMA 주소
      if 리스트가 가득 차면:
          새 PRP 리스트 페이지 할당 (dma_pool_alloc)
          이전 리스트 마지막 엔트리 = 새 리스트의 DMA 주소 (체인)
       │
       ▼
  SQE.DPTR.PRP1 = prp1_dma
  SQE.DPTR.PRP2 = prp2_dma (리스트 주소)
```

PRP 리스트 체인의 구조:

```
SQE.PRP2 ──> PRP List 0 (4KB, DMA Pool에서 할당)
             ┌─────────────────┐
             │ DMA addr[0]     │  데이터 페이지 0
             │ DMA addr[1]     │  데이터 페이지 1
             │ ...             │
             │ DMA addr[510]   │  데이터 페이지 510
             │ → PRP List 1 ──────> PRP List 1 (4KB)
             └─────────────────┘    ┌─────────────────┐
                                    │ DMA addr[511]   │
                                    │ DMA addr[512]   │
                                    │ ...             │
                                    └─────────────────┘
```

4KB PRP 리스트 하나에 512개의 8바이트 엔트리가 들어간다. 마지막 엔트리(인덱스 511)는 다음 PRP 리스트를 가리키는 링크이므로, 실제 데이터 주소는 511개이다. 512KB I/O(128 pages)면 PRP 리스트 1개로 충분하고, 2MB I/O(512 pages)면 2개가 필요하다.

### 5.4 nvme_unmap_data(): DMA 매핑 해제

I/O 완료 후 `nvme_unmap_data()`가 호출된다 (line 1072-1106):

```c
static void nvme_unmap_data(struct request *req)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);

    /* 단일 세그먼트: 간단히 dma_unmap_page */
    if (iod->flags & IOD_SINGLE_SEGMENT) {
        dma_unmap_page(dma_dev, le64_to_cpu(iod->cmd.common.dptr.prp1),
                iod->total_len, rq_dma_dir(req));
        return;
    }

    /* 다중 세그먼트: SGL 또는 PRP 방식에 따라 각 매핑 해제 */
    if (!blk_rq_dma_unmap(req, dma_dev, &iod->dma_state, ...)) {
        if (nvme_pci_cmd_use_sgl(&iod->cmd))
            nvme_free_sgls(req, ...);   /* SGL 엔트리별 unmap */
        else
            nvme_free_prps(req, ...);   /* PRP 엔트리별 unmap */
    }

    /* DMA Pool에서 할당한 디스크립터(PRP List/SGL List) 해제 */
    if (iod->nr_descriptors)
        nvme_free_descriptors(req);
}
```

---

## 6. IOMMU (VT-d / AMD-Vi)

### 6.1 IOMMU란

IOMMU(Input/Output Memory Management Unit)는 디바이스를 위한 MMU이다. CPU가 가상 주소를 물리 주소로 변환하듯, IOMMU는 디바이스의 DMA 주소(IOVA)를 물리 주소로 변환한다.

```
=== IOMMU 없는 시스템 ===

                    메모리 컨트롤러
                    ┌─────────┐
   CPU ──── MMU ────┤         ├──── 물리 메모리
                    │         │
   NVMe ────────────┤         │  ← 디바이스가 물리 주소로 직접 접근
                    └─────────┘     (어떤 메모리든 접근 가능 = 위험!)


=== IOMMU 있는 시스템 ===

                    메모리 컨트롤러
                    ┌─────────┐
   CPU ──── MMU ────┤         ├──── 물리 메모리
                    │         │
   NVMe ── IOMMU ───┤         │  ← IOMMU가 접근 제어
           │        └─────────┘
           │
    ┌──────┴──────┐
    │ IOVA → PA   │  IOMMU 페이지 테이블
    │ 변환 + 보호  │  (디바이스별 별도)
    └─────────────┘
```

Intel 시스템에서는 **VT-d (Virtualization Technology for Directed I/O)**, AMD 시스템에서는 **AMD-Vi**라고 부른다.

### 6.2 IOMMU의 목적

#### (1) DMA 보호 (Isolation)

IOMMU 없이는 디바이스가 시스템 메모리 어디든 DMA로 접근할 수 있다. 악의적이거나 버그가 있는 디바이스가 커널 메모리를 덮어쓸 수 있다. IOMMU는 디바이스별로 접근 가능한 메모리 영역을 제한한다.

#### (2) Scatter-Gather Merging

흩어진 물리 페이지들을 IOMMU 페이지 테이블에서 연속된 IOVA로 매핑하면, 디바이스 입장에서는 하나의 연속 버퍼처럼 보인다. PRP/SGL 엔트리 수가 줄어들어 성능이 향상된다.

#### (3) 가상화 (Device Passthrough)

VM에 디바이스를 직접 할당(passthrough)할 때, IOMMU가 VM의 GPA(Guest Physical Address)를 HPA(Host Physical Address)로 변환한다. 이를 통해 VM이 디바이스를 직접 사용하면서도 호스트 메모리를 보호할 수 있다.

### 6.3 IOMMU 동작 원리

#### IOMMU 페이지 테이블 구조

IOMMU는 CPU의 페이지 테이블과 유사한 다단계 페이지 테이블을 사용한다. Intel VT-d의 경우:

```
Root Table (Bus별)
  └─ Context Table (Device/Function별)
       └─ Page Table (4단계, CPU 페이지 테이블과 유사)
            └─ IOVA → PA 매핑

Root Entry [Bus 0]  ──> Context Entry [Dev 0, Fn 0] ──> Domain 페이지 테이블
                        Context Entry [Dev 1, Fn 0] ──> (다른) Domain 페이지 테이블
Root Entry [Bus 1]  ──> Context Entry [Dev 0, Fn 0] ──> ...
```

**Domain**: IOMMU의 주소 공간 단위. 하나의 디바이스 또는 디바이스 그룹이 하나의 domain을 공유한다. 같은 domain의 디바이스들은 같은 IOVA → PA 매핑을 본다.

#### iommu_map() 함수

```c
/* drivers/iommu/iommu.c, line 2582 */
int iommu_map(struct iommu_domain *domain, unsigned long iova,
          phys_addr_t paddr, size_t size, int prot, gfp_t gfp)
{
    int ret;

    ret = iommu_map_nosync(domain, iova, paddr, size, prot, gfp);
    if (ret)
        return ret;

    ret = iommu_sync_map(domain, iova, size);
    if (ret)
        iommu_unmap(domain, iova, size);

    return ret;
}
```

`iommu_map_nosync()`가 IOMMU 페이지 테이블에 매핑을 추가하고, `iommu_sync_map()`이 IOTLB를 무효화하여 하드웨어에 변경을 반영한다.

#### DMA API에서 IOMMU까지의 호출 체인

```
드라이버: dma_map_page()
  → kernel/dma/mapping.c: dma_map_page_attrs()
    → dma_map_phys()
      → use_dma_iommu(dev)?
        → YES: iommu_dma_map_phys()
          → IOVA 할당 (iova_alloc)
          → iommu_map(domain, iova, phys, size, prot)
            → IOMMU 페이지 테이블에 IOVA→PA 매핑 추가
          → DMA 주소 = IOVA 반환
        → NO: dma_direct_map_phys()
          → DMA 주소 = phys_to_dma(dev, phys) 반환
```

### 6.4 NVMe와 IOMMU

#### DMA 마스크 설정

NVMe 드라이버 초기화 시 DMA 마스크를 설정한다 (line 4315-4320):

```c
if (dev->ctrl.quirks & NVME_QUIRK_DMA_ADDRESS_BITS_48)
    dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(48));
else
    dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));

dma_set_min_align_mask(&pdev->dev, NVME_CTRL_PAGE_SIZE - 1);
dma_set_max_seg_size(&pdev->dev, 0xffffffff);
```

대부분의 NVMe 컨트롤러는 64비트 DMA를 지원한다. 48비트 제한이 있는 컨트롤러도 일부 존재한다. `dma_set_mask_and_coherent()`는 streaming DMA 마스크와 coherent DMA 마스크를 동시에 설정한다:

```c
/* include/linux/dma-mapping.h */
static inline int dma_set_mask_and_coherent(struct device *dev, u64 mask)
{
    int rc = dma_set_mask(dev, mask);
    if (rc == 0)
        dma_set_coherent_mask(dev, mask);
    return rc;
}
```

#### IOMMU 유무에 따른 PRP 주소의 의미

```
=== IOMMU가 없을 때 ===

  PRP1 = 0x1234_5000  (물리 주소)
  PRP List:
    [0] = 0x5678_0000  (물리 주소)
    [1] = 0x9ABC_0000  (물리 주소)

  컨트롤러가 PCIe 버스에서 이 주소로 DMA 요청
  → 메모리 컨트롤러가 물리 주소로 직접 접근


=== IOMMU가 있을 때 ===

  PRP1 = 0x0000_A000  (IOVA, 물리 주소가 아님!)
  PRP List:
    [0] = 0x0000_B000  (IOVA)
    [1] = 0x0000_C000  (IOVA)

  컨트롤러가 PCIe 버스에서 IOVA로 DMA 요청
  → IOMMU가 IOVA를 가로채서 물리 주소로 변환
  → 변환된 물리 주소로 메모리 접근
```

드라이버 코드는 이 차이를 신경 쓸 필요가 없다. `dma_map_*()` API가 IOMMU 유무에 따라 적절한 주소(물리 주소 또는 IOVA)를 반환하기 때문이다.

#### Passthrough 모드

IOMMU passthrough 모드에서는 IOVA = PA (1:1 매핑)이다. IOMMU 하드웨어는 활성화되어 있지만, 모든 주소를 그대로 통과시킨다. 보호 기능은 유지하면서 주소 변환 오버헤드를 줄인다.

커널 부트 파라미터로 설정:
- `iommu=pt`: passthrough 모드
- `iommu=on`: 완전한 IOMMU 매핑 (기본값, 배포판에 따라 다름)

Passthrough 모드에서 `dma_map_*()` 호출의 동작:

```c
/* kernel/dma/mapping.c */
static bool dma_go_direct(struct device *dev, dma_addr_t mask,
        const struct dma_map_ops *ops)
{
    if (use_dma_iommu(dev))
        return false;         /* IOMMU 사용 → direct 아님 */
    if (likely(!ops))
        return true;          /* ops 없음 → direct */
#ifdef CONFIG_DMA_OPS_BYPASS
    if (dev->dma_ops_bypass)  /* bypass 플래그 → direct */
        return min_not_zero(mask, dev->bus_dma_limit) >=
                dma_direct_get_required_mask(dev);
#endif
    return false;
}
```

Passthrough 모드에서는 `dma_ops_bypass`가 설정되어 `dma_direct_*` 함수가 호출된다. IOMMU 페이지 테이블 조작 없이 `phys_to_dma()`만으로 주소를 변환하므로 성능이 최적이다.

### 6.5 VFIO와 IOMMU (SPDK 연결고리)

#### VFIO란

VFIO(Virtual Function I/O)는 IOMMU를 이용하여 유저스페이스에서 디바이스에 안전하게 접근할 수 있는 프레임워크이다. 커널 드라이버를 우회하여 유저스페이스 프로그램이 디바이스 레지스터와 DMA를 직접 제어할 수 있다.

```
=== 일반 NVMe (커널 드라이버) ===

  Application
       │  read()/write()
       ▼
  Kernel (VFS → Block Layer → NVMe Driver)
       │  DMA 매핑, 인터럽트 처리 등
       ▼
  NVMe Device


=== SPDK (VFIO + 유저스페이스) ===

  SPDK Application (유저스페이스)
       │  MMIO 직접 접근 (BAR 매핑)
       │  DMA 주소 직접 설정 (VFIO DMA map)
       │  폴링으로 완료 확인 (인터럽트 없음)
       ▼
  NVMe Device (커널 드라이버 언바인드)

  IOMMU가 보호:
  - SPDK가 할당한 메모리만 DMA 접근 가능
  - 다른 프로세스/커널 메모리 접근 불가
```

#### SPDK가 VFIO를 사용하는 방법

1. NVMe 디바이스를 커널 nvme 드라이버에서 언바인드
2. vfio-pci 드라이버에 바인드
3. VFIO 컨테이너/그룹 열기
4. `VFIO_IOMMU_MAP_DMA` ioctl로 유저스페이스 메모리를 IOMMU에 매핑
5. 매핑된 IOVA를 PRP/SGL에 직접 기록
6. BAR0 MMIO로 Doorbell 직접 쓰기
7. CQ를 폴링하여 완료 확인

VFIO의 DMA 매핑:

```c
/* SPDK의 유저스페이스 코드 (개념적) */
struct vfio_iommu_type1_dma_map dma_map = {
    .vaddr = (uintptr_t)user_buffer,   /* 유저스페이스 가상 주소 */
    .iova  = desired_iova,              /* 원하는 IOVA */
    .size  = buffer_size,
    .flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE,
};
ioctl(container_fd, VFIO_IOMMU_MAP_DMA, &dma_map);

/* 이후 desired_iova를 PRP/SGL에 직접 기록 */
```

커널 NVMe 드라이버와의 차이:
- 커널: `dma_map_*()` API가 IOVA 할당과 IOMMU 매핑을 자동으로 수행
- SPDK: VFIO ioctl로 유저스페이스에서 직접 IOMMU 매핑을 관리

IOMMU가 없으면 SPDK는 `vfio-pci` 대신 `uio_pci_generic` 드라이버를 사용할 수 있지만, DMA 보호가 없어 보안 위험이 있다. 프로덕션 환경에서는 반드시 IOMMU + VFIO 조합을 사용해야 한다.

---

## 7. Bounce Buffer (SWIOTLB)

### 7.1 문제: 디바이스가 접근할 수 없는 메모리

일부 디바이스는 전체 물리 메모리 영역에 접근할 수 없다. 대표적인 경우:
- 32비트 DMA만 지원하는 디바이스: 4GB 이상의 메모리에 접근 불가
- AMD SME/SEV 암호화 메모리: 디바이스가 암호화된 메모리에 직접 접근하면 쓰레기 데이터를 읽음
- 특정 하드웨어의 DMA 주소 제한

현대 NVMe 컨트롤러는 64비트 DMA를 지원하므로 주소 제한 문제는 드물지만, 암호화 메모리(SEV 등)나 커널 부트 파라미터(`swiotlb=force`)로 강제할 수 있다.

### 7.2 SWIOTLB: Software I/O TLB

SWIOTLB(Software I/O Translation Lookaside Buffer)는 디바이스가 접근할 수 있는 낮은 메모리 영역에 bounce buffer를 할당하여 문제를 해결한다.

```
=== Bounce Buffer 동작 (DMA_TO_DEVICE, 쓰기) ===

  (1) 원본 버퍼 (높은 메모리, 예: 8GB 영역)
      ┌──────────────────┐
      │ 쓰기 데이터       │  ← 디바이스가 접근 불가
      └────────┬─────────┘
               │
  (2) memcpy   │  swiotlb_bounce()
               ▼
  Bounce Buffer (낮은 메모리, 예: 1GB 이내)
      ┌──────────────────┐
      │ 쓰기 데이터 복사  │  ← 디바이스가 접근 가능
      └────────┬─────────┘
               │
  (3) DMA      │
               ▼
      ┌──────────────────┐
      │ NVMe 컨트롤러    │  bounce buffer에서 데이터 읽기
      └──────────────────┘


=== Bounce Buffer 동작 (DMA_FROM_DEVICE, 읽기) ===

  (1) NVMe 컨트롤러가 bounce buffer에 데이터 쓰기
      ┌──────────────────┐
      │ NVMe 컨트롤러    │
      └────────┬─────────┘
               │ DMA
               ▼
  Bounce Buffer (낮은 메모리)
      ┌──────────────────┐
      │ 읽기 데이터       │
      └────────┬─────────┘
               │
  (2) memcpy   │  swiotlb_bounce() (sync 시)
               ▼
  원본 버퍼 (높은 메모리)
      ┌──────────────────┐
      │ 읽기 데이터 복사  │  ← CPU가 여기서 데이터를 읽음
      └──────────────────┘
```

### 7.3 SWIOTLB 핵심 구조

```c
/* kernel/dma/swiotlb.c */

/* IO TLB 슬롯 디스크립터 */
struct io_tlb_slot {
    phys_addr_t orig_addr;     /* 원본 버퍼의 물리 주소 */
    size_t alloc_size;         /* 할당 크기 */
    unsigned short list;       /* free list 카운터 */
    unsigned short pad_slots;  /* 정렬 패딩 슬롯 수 */
};
```

SWIOTLB 풀은 부팅 시 연속된 물리 메모리(기본 64MB)를 할당한다. 이 메모리는 2KB(`IO_TLB_SIZE`) 단위의 슬롯으로 나뉜다.

### 7.4 swiotlb_tbl_map_single() 분석

```c
/* kernel/dma/swiotlb.c, line 1367 */
phys_addr_t swiotlb_tbl_map_single(struct device *dev, phys_addr_t orig_addr,
        size_t mapping_size, unsigned int alloc_align_mask,
        enum dma_data_direction dir, unsigned long attrs)
{
    struct io_tlb_mem *mem = dev->dma_io_tlb_mem;

    /* SWIOTLB 풀에서 빈 슬롯 찾기 */
    index = swiotlb_find_slots(dev, orig_addr, size, alloc_align_mask, &pool);
    if (index == -1) {
        dev_warn_ratelimited(dev,
            "swiotlb buffer is full (sz: %zd bytes)...\n", size);
        return (phys_addr_t)DMA_MAPPING_ERROR;
    }

    /* 원본 → bounce buffer 복사 (DMA_TO_DEVICE 방향) */
    swiotlb_bounce(dev, tlb_addr, mapping_size, DMA_TO_DEVICE, pool);

    return tlb_addr;
}
```

### 7.5 swiotlb_bounce(): 데이터 복사

```c
/* kernel/dma/swiotlb.c, line 857 */
static void swiotlb_bounce(struct device *dev, phys_addr_t tlb_addr,
        size_t size, enum dma_data_direction dir, struct io_tlb_pool *mem)
{
    int index = (tlb_addr - mem->start) >> IO_TLB_SHIFT;
    phys_addr_t orig_addr = mem->slots[index].orig_addr;
    unsigned char *vaddr = mem->vaddr + tlb_addr - mem->start;

    /* 방향에 따라 복사 */
    if (dir == DMA_TO_DEVICE)
        /* 원본 → bounce buffer */
        memcpy(vaddr, phys_to_virt(orig_addr), size);
    else
        /* bounce buffer → 원본 */
        memcpy(phys_to_virt(orig_addr), vaddr, size);
}
```

### 7.6 swiotlb_map(): DMA 매핑 + bounce

```c
/* kernel/dma/swiotlb.c, line 1574 */
dma_addr_t swiotlb_map(struct device *dev, phys_addr_t paddr, size_t size,
        enum dma_data_direction dir, unsigned long attrs)
{
    /* bounce buffer 할당 + 원본 데이터 복사 */
    swiotlb_addr = swiotlb_tbl_map_single(dev, paddr, size, 0, dir, attrs);

    /* bounce buffer의 물리 주소를 DMA 주소로 변환 */
    dma_addr = phys_to_dma_unencrypted(dev, swiotlb_addr);

    /* DMA 주소가 디바이스 마스크 내인지 확인 */
    if (unlikely(!dma_capable(dev, dma_addr, size, true))) {
        __swiotlb_tbl_unmap_single(dev, swiotlb_addr, ...);
        return DMA_MAPPING_ERROR;
    }

    return dma_addr;
}
```

### 7.7 Sync 동작

SWIOTLB를 사용하면 I/O 완료 후 bounce buffer의 데이터를 원본 버퍼로 복사하는 sync가 필요하다:

```c
/* kernel/dma/direct.c */
void dma_direct_sync_sg_for_cpu(struct device *dev,
        struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
    for_each_sg(sgl, sg, nents, i) {
        phys_addr_t paddr = dma_to_phys(dev, sg_dma_address(sg));

        /* non-coherent 플랫폼: 캐시 동기화 */
        if (!dev_is_dma_coherent(dev))
            arch_sync_dma_for_cpu(paddr, sg->length, dir);

        /* SWIOTLB: bounce buffer → 원본 복사 */
        swiotlb_sync_single_for_cpu(dev, paddr, sg->length, dir);
    }
}
```

### 7.8 성능 영향

SWIOTLB bounce buffering은 상당한 성능 저하를 초래한다:

| 항목 | 직접 DMA | SWIOTLB |
|------|---------|---------|
| 메모리 복사 | 없음 | 매 I/O마다 memcpy |
| 추가 메모리 | 없음 | bounce buffer 풀 (기본 64MB) |
| 지연 시간 | 최소 | 복사 오버헤드 추가 |
| 대역폭 | PCIe 링크 속도 | 메모리 대역폭에 의해 제한 |

SWIOTLB가 발생하는 경우:
1. 디바이스 DMA 마스크보다 높은 주소의 메모리를 매핑할 때
2. `swiotlb=force` 커널 파라미터 사용 시 (디버깅용)
3. AMD SEV/SME 암호화 메모리 사용 시
4. CMA 할당된 메모리가 디바이스 주소 범위 밖일 때

NVMe 컨트롤러는 대부분 64비트 DMA를 지원하므로 일반적으로 SWIOTLB가 발생하지 않는다. 하지만 SEV 환경이나 특수한 하드웨어에서는 주의가 필요하다. `dmesg | grep swiotlb`로 SWIOTLB 사용 여부를 확인할 수 있다.

---

## 8. 전체 DMA 아키텍처 요약

### 8.1 API 계층 구조

```
                    드라이버 (NVMe, 네트워크 등)
                           │
         ┌─────────────────┼─────────────────────┐
         │                 │                      │
         ▼                 ▼                      ▼
  dma_alloc_coherent  dma_map_sg/page       dma_pool_alloc
  (SQ/CQ 할당)       (데이터 매핑)          (PRP List 할당)
         │                 │                      │
         └─────────────────┼──────────────────────┘
                           │
                    kernel/dma/mapping.c
                    dma_alloc_attrs()
                    dma_map_page_attrs()
                           │
              ┌────────────┼────────────┐
              │            │            │
              ▼            ▼            ▼
        dma_direct    iommu_dma      ops->xxx
        (직접 매핑)   (IOMMU 사용)   (플랫폼별)
              │            │
              │     ┌──────┴──────┐
              │     │  IOVA 할당   │
              │     │  iommu_map() │
              │     └─────────────┘
              │
        ┌─────┴──────┐
        │ SWIOTLB?   │
        │ (필요 시)   │
        │ bounce buf  │
        └────────────┘
```

### 8.2 NVMe I/O에서 DMA 사용 정리

```
┌─────────────────────────────────────────────────────────────────┐
│                      NVMe I/O 전체 DMA 흐름                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  [초기화 시 - 한 번만]                                          │
│                                                                 │
│  1. dma_set_mask_and_coherent(64bit)  ← DMA 주소 범위 설정      │
│  2. dma_alloc_coherent(SQ)           ← SQ 버퍼 할당             │
│  3. dma_alloc_coherent(CQ)           ← CQ 버퍼 할당             │
│  4. dma_pool_create(large, 4KB)      ← PRP List 풀 생성         │
│  5. dma_pool_create(small, 256B)     ← 작은 디스크립터 풀 생성   │
│  6. dma_alloc_coherent(Shadow DB)    ← Shadow Doorbell 할당     │
│                                                                 │
│  [매 I/O마다 - 반복]                                            │
│                                                                 │
│  7. nvme_map_data() 호출                                        │
│     ├─ 단일 세그먼트:                                            │
│     │  └─ dma_map_bvec() → DMA 주소 1개                        │
│     │     └─ PRP1 (+ PRP2) 설정                                │
│     │                                                           │
│     └─ 다중 세그먼트:                                            │
│        ├─ blk_rq_dma_map_iter_start() → 첫 세그먼트 매핑       │
│        ├─ blk_rq_dma_map_iter_next()  → 후속 세그먼트 매핑     │
│        ├─ dma_pool_alloc()            → PRP List 할당          │
│        └─ PRP List에 DMA 주소 기록                              │
│                                                                 │
│  8. SQE를 SQ에 기록 (coherent DMA 메모리이므로 sync 불필요)     │
│  9. Doorbell 쓰기 (MMIO)                                       │
│                                                                 │
│  [컨트롤러 동작 - DMA 전송]                                     │
│                                                                 │
│  10. 컨트롤러가 SQ에서 SQE를 DMA Read   (sq_dma_addr 사용)     │
│  11. 컨트롤러가 PRP List를 DMA Read     (PRP2의 DMA 주소)      │
│  12. 컨트롤러가 데이터를 DMA Read/Write (PRP의 DMA 주소들)     │
│  13. 컨트롤러가 CQ에 CQE를 DMA Write   (cq_dma_addr 사용)     │
│  14. MSI-X 인터럽트 발생                                        │
│                                                                 │
│  [I/O 완료 후]                                                  │
│                                                                 │
│  15. nvme_unmap_data() 호출                                     │
│      ├─ dma_unmap_page() 또는 각 세그먼트 unmap                │
│      └─ dma_pool_free() → PRP List 반환                        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 8.3 커널 NVMe vs SPDK: DMA 처리 비교

| 항목 | 커널 NVMe 드라이버 | SPDK |
|------|-------------------|------|
| DMA API | `dma_map_*()` 커널 API | VFIO `VFIO_IOMMU_MAP_DMA` ioctl |
| 주소 변환 | 커널이 자동 관리 | 유저스페이스에서 직접 관리 |
| IOMMU | 커널 IOMMU 드라이버가 처리 | VFIO가 IOMMU 매핑 관리 |
| Bounce Buffer | 필요 시 SWIOTLB 자동 사용 | hugepage 사용으로 회피 |
| PRP/SGL 구축 | 커널에서 매 I/O마다 | 유저스페이스에서 직접 |
| DMA Pool | `dma_pool_alloc()` | 자체 메모리 풀 |
| 성능 특성 | 시스템콜 + 컨텍스트 스위치 오버헤드 | 유저스페이스 폴링, 제로카피 |

SPDK가 hugepage(2MB/1GB)를 사용하는 이유:
1. 물리적으로 연속된 큰 메모리 확보 → SG 엔트리 감소
2. IOMMU TLB 미스 감소 (큰 페이지 → 적은 매핑 수)
3. 유저스페이스에서 물리 주소를 알 수 있음 (`/proc/self/pagemap`)
4. SWIOTLB bounce buffer 불필요 (hugepage는 항상 물리 주소 범위 내)

---

## 9. 디버깅과 모니터링

### 9.1 DMA 관련 커널 파라미터

| 파라미터 | 설명 |
|---------|------|
| `iommu=on` | IOMMU 활성화 (전체 매핑) |
| `iommu=pt` | IOMMU passthrough 모드 (1:1 매핑, 최고 성능) |
| `iommu=off` | IOMMU 비활성화 |
| `swiotlb=force` | 모든 DMA에 SWIOTLB 강제 사용 (디버깅용) |
| `swiotlb=65536` | SWIOTLB 슬롯 수 지정 (기본 32768, ×2KB = 64MB) |
| `intel_iommu=on` | Intel VT-d 명시적 활성화 |

### 9.2 확인 명령

```bash
# IOMMU 상태 확인
dmesg | grep -i iommu

# SWIOTLB 사용 확인
dmesg | grep -i swiotlb

# 디바이스별 IOMMU 그룹 확인
ls -la /sys/kernel/iommu_groups/

# NVMe 디바이스의 DMA 마스크 확인
cat /sys/bus/pci/devices/0000:XX:XX.X/dma_mask_bits

# SWIOTLB 사용량 모니터링
cat /sys/kernel/debug/swiotlb/io_tlb_nslabs
cat /sys/kernel/debug/swiotlb/io_tlb_used
```

### 9.3 CONFIG_DMA_API_DEBUG

커널 설정 `CONFIG_DMA_API_DEBUG=y`를 활성화하면 DMA API 오남용을 감지한다:
- 매핑 후 해제하지 않는 경우 (DMA leak)
- 잘못된 방향 지정
- 범위를 벗어난 접근
- vmalloc 영역 매핑 시도

---

## 10. 핵심 정리

### DMA API 선택 가이드 (NVMe 관점)

```
┌──────────────────────────────────────────────────────────────┐
│                     어떤 DMA API를 쓸까?                      │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  Q: CPU와 디바이스가 동시에 접근하는가?                       │
│  │                                                           │
│  ├─ YES → dma_alloc_coherent()                               │
│  │        예: SQ/CQ 버퍼, Shadow Doorbell                    │
│  │                                                           │
│  └─ NO → Q: 작은 크기(수백 바이트)를 자주 할당/해제하는가?   │
│           │                                                  │
│           ├─ YES → dma_pool_alloc()                          │
│           │        예: PRP List, SGL List                    │
│           │                                                  │
│           └─ NO → dma_map_page() / dma_map_sg()              │
│                   예: 사용자 데이터 버퍼                     │
│                   (기존 메모리를 일시적으로 DMA 매핑)        │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 핵심 개념 요약

1. **DMA**는 CPU 없이 디바이스가 메모리를 직접 접근하는 메커니즘이다. NVMe의 모든 데이터 전송은 DMA로 이루어진다.

2. **Coherent DMA**는 CPU와 디바이스가 동시에 접근하는 공유 메모리이다. 캐시 일관성이 보장되며, SQ/CQ에 사용된다. `dma_alloc_coherent()`로 할당한다.

3. **Streaming DMA**는 일시적 DMA 매핑이다. 데이터 전송에 사용되며, 방향과 소유권 관리가 필요하다. `dma_map_page()`/`dma_map_sg()`로 매핑한다.

4. **DMA Pool**은 작은 DMA 메모리를 효율적으로 할당한다. NVMe PRP List(4KB)와 작은 디스크립터(256B)에 사용된다.

5. **Scatter-Gather List**는 비연속 물리 페이지들을 기술하는 리스트이다. `dma_map_sg()`가 각 엔트리에 DMA 주소를 채운다.

6. **IOMMU**는 디바이스용 MMU로, DMA 보호와 SG merging을 제공한다. `dma_map_*()` API가 IOMMU 유무를 자동으로 처리한다.

7. **SWIOTLB**는 디바이스가 접근 불가능한 메모리에 대해 bounce buffer를 제공하지만, memcpy 오버헤드로 성능이 저하된다.

8. **VFIO + IOMMU** 조합으로 SPDK 같은 유저스페이스 드라이버가 안전하게 DMA를 수행할 수 있다.
