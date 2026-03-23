/*
 * map.c - DMA 메모리 매핑 관리 구현.
 * 사용자 공간의 호스트/GPU 메모리를 커널에서 pin(고정)하고 DMA 버스 주소로 변환한다.
 * 호스트 메모리: get_user_pages() + dma_map_page() 사용
 * GPU 메모리: NVIDIA P2P API (nvidia_p2p_get_pages + nvidia_p2p_dma_map_pages) 사용
 */
#include "map.h"
#include "list.h"
#include "ctrl.h"
#include <linux/version.h>
#include <linux/sched.h>        /* current 매크로: 현재 실행 중인 프로세스의 task_struct 포인터 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>         /* kmalloc, kfree: 커널 메모리 할당 */
#include <linux/mm_types.h>
#include <linux/mm.h>           /* get_user_pages: 사용자 공간 페이지를 pin하는 함수 */
#include <linux/dma-mapping.h>  /* dma_map_page, dma_unmap_page: DMA 버스 주소 매핑/해제 */
#include <linux/err.h>          /* IS_ERR, ERR_PTR, PTR_ERR: 에러 처리 매크로 */

#ifdef _CUDA
#include <nv-p2p.h>  /* NVIDIA P2P API: GPU 메모리를 다른 PCIe 디바이스가 직접 접근할 수 있게 한다 */

/*
 * gpu_region - GPU P2P 매핑 정보를 담는 구조체.
 * nvidia_p2p_get_pages로 얻은 GPU 페이지 테이블과
 * nvidia_p2p_dma_map_pages로 얻은 각 컨트롤러별 DMA 매핑을 보유한다.
 */
struct gpu_region
{
    nvidia_p2p_page_table_t* pages;       /* NVIDIA P2P API가 반환한 GPU 페이지 테이블 */
    nvidia_p2p_dma_mapping_t** mappings;  /* 각 NVMe 컨트롤러별 P2P DMA 매핑 배열 */
};
#endif


/* GPU 페이지 크기 관련 상수: NVIDIA P2P API는 64KB 페이지 단위로 동작한다 */
#define GPU_PAGE_SHIFT  16
#define GPU_PAGE_SIZE   (1UL << GPU_PAGE_SHIFT)   /* 64KB = 65536 바이트 */
#define GPU_PAGE_MASK   ~(GPU_PAGE_SIZE - 1)      /* 64KB 페이지 정렬 마스크 */

uint32_t max_num_ctrls = 64;  /* 최대 컨트롤러 수 (GPU 매핑 시 모든 컨트롤러에 대해 DMA 매핑 생성) */


/*
 * create_descriptor - 매핑 디스크립터(struct map)를 생성한다.
 * 가변 길이 배열 패턴으로 map 구조체 + (n_pages-1)개의 추가 주소 공간을 한 번에 할당한다.
 * kvmalloc을 사용하여 작은 할당은 kmalloc, 큰 할당은 vmalloc으로 자동 전환한다.
 */
static struct map* create_descriptor(const struct ctrl* ctrl, u64 vaddr, unsigned long n_pages)
{
    unsigned long i;
    struct map* map = NULL;

    /* kvmalloc: 커널 가상 메모리 할당 (kmalloc 실패 시 vmalloc으로 fallback) */
    /* 가변 길이 배열: struct map에 addrs[1]이 있으므로 (n_pages-1)개의 추가 공간이 필요하다 */
    map = kvmalloc(sizeof(struct map) + (n_pages - 1) * sizeof(uint64_t), GFP_KERNEL);
    if (map == NULL)
    {
        printk(KERN_CRIT "Failed to allocate mapping descriptor\n");
        return ERR_PTR(-ENOMEM);
    }

    list_node_init(&map->list);  /* 연결 리스트 노드를 초기화한다 */

    map->owner = current;      /* current: 현재 프로세스의 task_struct (매핑 소유자) */
    map->vaddr = vaddr;        /* 매핑 대상 가상 주소 (페이지 정렬됨) */
    map->pdev = ctrl->pdev;    /* PCI 디바이스 참조 (DMA 매핑에 필요) */
    map->page_size = 0;        /* 페이지 크기는 이후 설정된다 (호스트: 4KB, GPU: 64KB) */
    map->data = NULL;          /* 커스텀 데이터는 이후 설정된다 */
    map->release = NULL;       /* 해제 콜백은 이후 설정된다 */
    map->n_addrs = n_pages;    /* DMA 주소 개수 = 페이지 수 */


    /* 모든 DMA 주소를 0으로 초기화한다 */
    for (i = 0; i < map->n_addrs; ++i)
    {
        map->addrs[i] = 0;
    }

    return map;
}



/*
 * unmap_and_release - 매핑을 리스트에서 제거하고 DMA 매핑을 해제한 뒤 메모리를 반환한다.
 * release 콜백이 설정되어 있으면 호출하여 타입별 정리(호스트: dma_unmap + put_page, GPU: P2P 해제)를 수행한다.
 */
void unmap_and_release(struct map* map)
{
    list_remove(&map->list);  /* 연결 리스트에서 이 매핑을 제거한다 */

    /* release 콜백이 있고 커스텀 데이터가 있으면 타입별 해제를 수행한다 */
    if (map->release != NULL && map->data != NULL)
    {
        map->release(map);  /* 호스트: release_user_pages, GPU: release_gpu_memory */
    }

    kvfree(map);  /* kvmalloc으로 할당한 메모리를 해제한다 (kmalloc/vmalloc 모두 처리) */
}



/*
 * map_find - 가상 주소와 현재 프로세스로 매핑을 검색한다.
 * 보안상 현재 프로세스(current)가 소유한 매핑만 반환한다.
 * 호스트 페이지(PAGE_MASK)와 GPU 페이지(GPU_PAGE_MASK) 양쪽 정렬을 모두 확인한다.
 */
struct map* map_find(const struct list* list, u64 vaddr)
{
    const struct list_node* element = list_next(&list->head);
    struct map* map = NULL;

    while (element != NULL)
    {
        map = container_of(element, struct map, list);

        /* 현재 프로세스가 소유한 매핑만 검색한다 (다른 프로세스의 매핑 접근 방지) */
        if (map->owner == current)
        {
            /* 호스트 페이지(4KB) 또는 GPU 페이지(64KB) 정렬 후 주소가 일치하는지 확인한다 */
            if (map->vaddr == (vaddr & PAGE_MASK) || map->vaddr == (vaddr & GPU_PAGE_MASK))
            {
                return map;
            }
        }

        element = list_next(element);
    }

    return NULL;  /* 일치하는 매핑을 찾지 못했다 */
}



/*
 * release_user_pages - 호스트 메모리 DMA 매핑을 해제하고 pin된 페이지를 반환한다.
 * 1) dma_unmap_page: 각 페이지의 DMA 버스 주소 매핑을 해제한다
 * 2) put_page: get_user_pages로 증가시킨 페이지 참조 카운트를 감소시켜 pin을 해제한다
 */
static void release_user_pages(struct map* map)
{
    unsigned long i;
    struct page** pages;
    struct device* dev;

    dev = &map->pdev->dev;  /* PCI 디바이스의 generic device 구조체 (DMA API에 필요) */

    /* 모든 페이지의 DMA 버스 주소 매핑을 해제한다 */
    for (i = 0; i < map->n_addrs; ++i)
    {
        /* dma_unmap_page: IOMMU/SWIOTLB의 DMA 매핑을 해제한다 */
        dma_unmap_page(dev, map->addrs[i], PAGE_SIZE, DMA_BIDIRECTIONAL);
    }

    pages = (struct page**) map->data;  /* 커스텀 데이터에 저장된 struct page* 배열 */

    /* 모든 페이지의 참조 카운트를 감소시켜 pin을 해제한다 */
    for (i = 0; i < map->n_addrs; ++i)
    {
        put_page(pages[i]);  /* 페이지 참조 카운트를 1 감소시킨다 (0이 되면 페이지가 해제 가능해진다) */
    }

    kvfree(map->data);  /* struct page* 배열 메모리를 해제한다 */
    map->data = NULL;

    //printk(KERN_DEBUG "Released %lu host pages\n", map->n_addrs);
}



/*
 * map_user_pages - 사용자 공간 호스트 메모리 페이지를 pin하고 DMA 버스 주소를 얻는다.
 * 1) get_user_pages: 사용자 가상 주소에 해당하는 물리 페이지를 pin(스왑 아웃 방지)한다
 * 2) dma_map_page: 각 물리 페이지를 PCI 디바이스가 접근 가능한 DMA 버스 주소로 매핑한다
 *    (IOMMU가 있으면 IOMMU 매핑, 없으면 물리 주소를 직접 반환)
 */
static long map_user_pages(struct map* map)
{
    unsigned long i;
    long retval;
    struct page** pages;
    struct device* dev;

    /* 페이지 포인터 배열을 할당한다 (각 페이지의 struct page*를 저장) */
    pages = (struct page**) kvcalloc(map->n_addrs, sizeof(struct page*), GFP_KERNEL);
    if (pages == NULL)
    {
        printk(KERN_CRIT "Failed to allocate page array\n");
        return -ENOMEM;
    }

    /* get_user_pages: 사용자 공간 가상 주소를 물리 페이지로 변환하고 pin한다 */
    /* 커널 버전에 따라 API가 다르므로 조건부 컴파일한다 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 5, 7)
#warning "Building for older kernel, not properly tested"
    retval = get_user_pages(current, current->mm, map->vaddr, map->n_addrs, 1, 0, pages, NULL);
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4, 8, 17)
#warning "Building for older kernel, not properly tested"
    retval = get_user_pages(map->vaddr, map->n_addrs, 1, 0, pages, NULL);
#else
    /* FOLL_WRITE: 쓰기 가능한 페이지로 폴트인한다 (DMA가 쓰기도 하므로) */
    retval = get_user_pages(map->vaddr, map->n_addrs, FOLL_WRITE, pages, NULL);
#endif
    if (retval <= 0)
    {
        kfree(pages);
        printk(KERN_ERR "get_user_pages() failed: %ld\n", retval);
        return retval;
    }

    /* 요청한 페이지 수와 실제 pin된 페이지 수가 다를 수 있다 */
    if (map->n_addrs != retval)
    {
        printk(KERN_WARNING "Requested %lu GPU pages, but only got %ld\n", map->n_addrs, retval);
    }
    map->n_addrs = retval;            /* 실제 pin된 페이지 수로 갱신한다 */
    map->page_size = PAGE_SIZE;       /* 호스트 페이지 크기 (보통 4KB) */
    map->data = (void*) pages;        /* struct page* 배열을 커스텀 데이터로 저장한다 */
    map->release = release_user_pages;  /* 해제 콜백을 설정한다 */

    dev = &map->pdev->dev;

    /* 각 물리 페이지를 DMA 버스 주소로 매핑한다 */
    for (i = 0; i < map->n_addrs; ++i)
    {
        /* dma_map_page: 물리 페이지를 PCI 디바이스가 접근 가능한 버스 주소로 변환한다 */
        /* DMA_BIDIRECTIONAL: 디바이스가 읽기/쓰기 모두 가능하도록 매핑한다 */
        map->addrs[i] = dma_map_page(dev, pages[i], 0, PAGE_SIZE, DMA_BIDIRECTIONAL);

        /* DMA 매핑 에러를 확인한다 (IOMMU 테이블 부족 등의 이유로 실패할 수 있다) */
        retval = dma_mapping_error(dev, map->addrs[i]);
        if (retval != 0)
        {
            printk(KERN_ERR "Failed to map page for some reason\n");
            return retval;
        }
       // printk("map_user_page: device: %02x:%02x.%1x\tvaddr: %llx\ti: %lu\tdma_addr: %llx\n", map->pdev->bus->number, PCI_SLOT(map->pdev->devfn), PCI_FUNC(map->pdev->devfn), (uint64_t) map->vaddr, i, map->addrs[i]);
    }

    return 0;
}



/*
 * map_userspace - 사용자 공간 호스트 메모리를 DMA 매핑하는 공개 함수.
 * 매핑 디스크립터를 생성하고, 사용자 페이지를 pin/매핑한 뒤, 매핑 리스트에 삽입한다.
 */
struct map* map_userspace(struct list* list, const struct ctrl* ctrl, u64 vaddr, unsigned long n_pages)
{
    long err;
    struct map* md;

    if (n_pages < 1)
    {
        return ERR_PTR(-EINVAL);  /* 페이지 수가 0이면 유효하지 않은 인자 */
    }

    /* 가상 주소를 페이지 크기(4KB)로 내림 정렬하여 디스크립터를 생성한다 */
    md = create_descriptor(ctrl, vaddr & PAGE_MASK, n_pages);
    if (IS_ERR(md))
    {
        return md;
    }

    md->page_size = PAGE_SIZE;  /* 호스트 페이지 크기를 설정한다 */

    /* 사용자 페이지를 pin하고 DMA 버스 주소를 얻는다 */
    err = map_user_pages(md);
    if (err != 0)
    {
        unmap_and_release(md);  /* 실패 시 정리한다 */
        return ERR_PTR(err);
    }

    list_insert(list, &md->list);  /* 성공하면 호스트 매핑 리스트에 삽입한다 */

    //printk(KERN_DEBUG "Mapped %lu host pages starting at address %llx\n",
    //        md->n_addrs, md->vaddr);
    return md;
}



#ifdef _CUDA
/*
 * force_release_gpu_memory - NVIDIA 드라이버가 GPU 메모리를 강제 회수할 때 호출되는 콜백.
 * nvidia_p2p_get_pages 호출 시 등록한 콜백으로, GPU 컨텍스트가 소멸될 때 드라이버가 호출한다.
 * nvidia_p2p_free_page_table을 사용한다 (nvidia_p2p_put_pages가 아님: 드라이버가 이미 페이지를 회수했으므로).
 */
static void force_release_gpu_memory(struct map* map)
{
    struct gpu_region* gd = (struct gpu_region*) map->data;
    struct list* list = map->ctrl_list;

    if (gd != NULL)
    {
        /* 모든 컨트롤러에 대한 P2P DMA 매핑을 해제한다 */
        if (gd->mappings != NULL)
        {
            const struct list_node* element = list_next(&list->head);
            struct ctrl* ctrl;

            uint32_t j = 0;
            while (element != NULL)
            {
                ctrl = container_of(element, struct ctrl, list);
                if (gd->mappings[j] != NULL)
                    /* nvidia_p2p_dma_unmap_pages: P2P DMA 매핑을 해제한다 */
                    nvidia_p2p_dma_unmap_pages(ctrl->pdev, gd->pages, gd->mappings[j++]);

                element = list_next(element);
            }
            kfree(gd->mappings);  /* 매핑 배열을 해제한다 */

        }

        if (gd->pages != NULL)
        {
            /* nvidia_p2p_free_page_table: 드라이버가 강제 회수한 경우 페이지 테이블만 해제한다 */
            /* (nvidia_p2p_put_pages 대신 사용: 페이지가 이미 드라이버에 의해 회수되었으므로) */
            nvidia_p2p_free_page_table(gd->pages);
        }

        kfree(gd);
        map->data = NULL;

        printk(KERN_DEBUG "Nvidia driver forcefully reclaimed %lu GPU pages\n", map->n_addrs);
    }

    unmap_and_release(map);  /* 매핑 디스크립터를 리스트에서 제거하고 해제한다 */
}
#endif



#ifdef _CUDA
/*
 * release_gpu_memory - GPU 메모리 매핑의 정상 해제 콜백.
 * 사용자가 명시적으로 매핑 해제를 요청할 때 호출된다.
 * nvidia_p2p_put_pages를 사용하여 GPU 페이지를 NVIDIA 드라이버에 반환한다.
 */
void release_gpu_memory(struct map* map)
{
    struct gpu_region* gd = (struct gpu_region*) map->data;
    struct list* list = map->ctrl_list;

    if (gd != NULL)
    {
        /* 모든 컨트롤러에 대한 P2P DMA 매핑을 해제한다 */
        if (gd->mappings != NULL)
        {
            const struct list_node* element = list_next(&list->head);
            struct ctrl* ctrl;

            uint32_t j = 0;
            while (element != NULL)
            {
                ctrl = container_of(element, struct ctrl, list);
                if (gd->mappings[j] != NULL)
                    nvidia_p2p_dma_unmap_pages(ctrl->pdev, gd->pages, gd->mappings[j++]);

                element = list_next(element);
            }
            kfree(gd->mappings);

        }

        if (gd->pages != NULL)
        {
            /* nvidia_p2p_put_pages: GPU 페이지를 NVIDIA 드라이버에 정상 반환한다 */
            /* (force_release와 달리 페이지가 아직 유효하므로 put_pages를 사용) */
            nvidia_p2p_put_pages(0, 0, map->vaddr, gd->pages);
        }

        kfree(gd);
        map->data = NULL;

        //printk(KERN_DEBUG "Released %lu GPU pages\n", map->n_addrs);
    }
}
#endif



#ifdef _CUDA
/*
 * map_gpu_memory - GPU 메모리를 pin하고 모든 NVMe 컨트롤러에 대해 P2P DMA 매핑을 생성한다.
 * NVIDIA P2P API 호출 순서:
 * 1) nvidia_p2p_get_pages: GPU 가상 주소를 pin하고 GPU 페이지 테이블을 얻는다
 * 2) nvidia_p2p_dma_map_pages: 각 PCI 디바이스에 대해 P2P DMA 주소를 생성한다
 * 모든 컨트롤러에 매핑을 생성하는 이유: GPU-Direct Storage에서 어떤 NVMe에서든 GPU로 직접 DMA 가능하도록
 */
int map_gpu_memory(struct map* map, struct list* list)
{
    unsigned long i;
    uint32_t j;
    int err;
    struct gpu_region* gd;
    const struct list_node* element;
    struct ctrl* ctrl;

    /* GPU 리전 디스크립터를 할당한다 */
    gd = kmalloc(sizeof(struct gpu_region), GFP_KERNEL);
    if (gd == NULL)
    {
        printk(KERN_CRIT "Failed to allocate mapping descriptor\n");
        return -ENOMEM;
    }

    /* 최대 컨트롤러 수만큼의 DMA 매핑 포인터 배열을 할당한다 */
    gd->mappings = (nvidia_p2p_dma_mapping_t**)  kmalloc(sizeof(nvidia_p2p_dma_mapping_t*) * max_num_ctrls, GFP_KERNEL);

    if (gd->mappings == NULL)
    {
        printk(KERN_CRIT "Failed to allocate mapping descriptor\n");
        kfree(gd);
        return -ENOMEM;
    }
    /* 모든 매핑 포인터를 NULL로 초기화한다 */
    for (j = 0; j < max_num_ctrls; j++)
        gd->mappings[j] = NULL;

    gd->pages = NULL;
    //gd->mappings = NULL;

    map->page_size = GPU_PAGE_SIZE;       /* GPU 페이지 크기: 64KB */
    map->data = gd;                       /* GPU 리전을 커스텀 데이터로 저장한다 */
    map->release = release_gpu_memory;    /* 정상 해제 콜백을 설정한다 */

    /* nvidia_p2p_get_pages: GPU 가상 주소 범위를 pin하고 페이지 테이블을 얻는다 */
    /* force_release_gpu_memory를 콜백으로 등록: GPU 컨텍스트 소멸 시 강제 해제용 */
    err = nvidia_p2p_get_pages(0, 0, map->vaddr, GPU_PAGE_SIZE * map->n_addrs, &gd->pages,
            (void (*)(void*)) force_release_gpu_memory, map);
    if (err != 0)
    {
        printk(KERN_ERR "nvidia_p2p_get_pages() failed: %d\n", err);
        return err;
    }

    element = list_next(&list->head);


    j = 0;
    /* 모든 NVMe 컨트롤러에 대해 P2P DMA 매핑을 생성한다 */
    while (element != NULL)
    {
        ctrl = container_of(element, struct ctrl, list);

        /* nvidia_p2p_dma_map_pages: 해당 PCI 디바이스가 GPU 메모리에 직접 DMA할 수 있는 주소를 생성한다 */
        err = nvidia_p2p_dma_map_pages(ctrl->pdev, gd->pages, gd->mappings + (j++));
        if (err != 0)
        {
            //printk(KERN_ERR "nvidia_p2p_dma_map_pages() failed for nvme%u: %d\n", j-1, err);
            return err;
        }
        //for (i = 0; i < map->n_addrs; ++i)
        //{

        //   printk("device: %u\ti: %lu\tpaddr: %llx\n", (j-1), i, (uint64_t)  gd->mappings[j-1]->dma_addresses[i]);
        //}
        /* 첫 번째 컨트롤러의 DMA 주소를 map->addrs에 저장한다 (기본 반환 주소) */
        if (j == 1) {
            for (i = 0; i < map->n_addrs; ++i)
            {
                map->addrs[i] = gd->mappings[0]->dma_addresses[i];
                //printk("++paddr: %llx\n", (uint64_t) map->addrs[i]);
            }
        }
        element = list_next(element);
    }




    /* 요청한 페이지 수와 실제 얻은 페이지 수가 다를 수 있다 */
    if (map->n_addrs != gd->pages->entries)
    {
        printk(KERN_WARNING "Requested %lu GPU pages, but only got %u\n", map->n_addrs, gd->pages->entries);
    }

    map->n_addrs = gd->pages->entries;  /* 실제 얻은 GPU 페이지 수로 갱신한다 */

    //printk("vaddr: %llx\n", (uint64_t) map->vaddr);
//    for (j = 0; j < map->n_addrs; j++)
//        printk("\tpaddr: %llx\n", (uint64_t) map->addrs[j]);

    return 0;
}
#endif



#ifdef _CUDA
/*
 * map_device_memory - CUDA GPU 디바이스 메모리를 DMA 매핑하는 공개 함수.
 * 매핑 디스크립터를 생성하고, GPU 메모리를 P2P 매핑한 뒤, 매핑 리스트에 삽입한다.
 * GPU 메모리는 64KB 페이지 정렬을 사용한다 (GPU_PAGE_MASK).
 */
struct map* map_device_memory(struct list* list, const struct ctrl* ctrl, u64 vaddr, unsigned long n_pages, struct list* ctrl_list)
{
    int err;
    struct map* md = NULL;

    if (n_pages < 1)
    {
        return ERR_PTR(-EINVAL);
    }

    /* GPU 페이지 크기(64KB)로 주소를 정렬하여 디스크립터를 생성한다 */
    md = create_descriptor(ctrl, vaddr & GPU_PAGE_MASK, n_pages);
    if (IS_ERR(md))
    {
        return md;
    }

    md->page_size = GPU_PAGE_SIZE;  /* GPU 페이지 크기를 설정한다 */
    md->ctrl_list = ctrl_list;      /* 모든 컨트롤러 리스트 참조를 저장한다 (P2P 매핑에 필요) */

    /* GPU 메모리를 pin하고 모든 컨트롤러에 대해 P2P DMA 매핑을 생성한다 */
    err = map_gpu_memory(md, ctrl_list);
    if (err != 0)
    {
        unmap_and_release(md);
        return ERR_PTR(err);
    }

    list_insert(list, &md->list);  /* 성공하면 GPU 매핑 리스트에 삽입한다 */

    //printk(KERN_DEBUG "Mapped %lu GPU pages starting at address %llx\n",
    //        md->n_addrs, md->vaddr);
    return md;
}
#endif
