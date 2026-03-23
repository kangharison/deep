/*
 * map.h - DMA 메모리 매핑 관리 헤더.
 * 사용자 공간 메모리(호스트 RAM 또는 GPU 메모리)를 커널에서 pin하고
 * DMA 버스 주소로 변환하는 매핑 구조체와 함수를 선언한다.
 */
#ifndef __LIBNVM_HELPER_MAP_H__
#define __LIBNVM_HELPER_MAP_H__

#include "list.h"
#include <linux/types.h>
#include <linux/mm_types.h>  /* struct task_struct: 프로세스를 나타내는 커널 구조체 */


/* 전방 선언 */
struct ctrl;
struct map;


/* 매핑 해제 시 호출될 콜백 함수 타입 */
typedef void (*release)(struct map*);


/*
 * map 구조체 - DMA 매핑된 메모리 범위를 나타낸다.
 * 호스트 메모리의 경우 get_user_pages로 pin한 페이지들과 dma_map_page로 얻은 버스 주소를 보유하고,
 * GPU 메모리의 경우 nvidia_p2p_get_pages로 얻은 GPU 페이지 정보와 P2P DMA 주소를 보유한다.
 * addrs[1]은 가변 길이 배열(flexible array)로, 실제로는 n_addrs개의 버스 주소를 저장한다.
 */
struct map
{
    struct list_node    list;           /* 매핑 연결 리스트의 노드 (host_list 또는 device_list에 삽입) */
    struct task_struct* owner;          /* 이 매핑을 생성한 프로세스 (current 매크로로 설정) */
    u64                 vaddr;          /* 매핑된 메모리의 시작 가상 주소 (페이지 정렬됨) */
    struct list*        ctrl_list;      /* GPU 매핑에서 사용: 모든 컨트롤러 리스트 참조 (P2P DMA 매핑에 필요) */
    struct pci_dev*     pdev;           /* 이 매핑과 연관된 PCI 디바이스 참조 */
    unsigned long       page_size;      /* 논리적 페이지 크기 (호스트: PAGE_SIZE 4KB, GPU: 64KB) */
    void*               data;           /* 타입별 커스텀 데이터 (호스트: struct page** 배열, GPU: struct gpu_region*) */
    release             release;        /* 매핑 해제 시 호출될 콜백 (DMA 언매핑 + 페이지 unpin 수행) */
    unsigned long       n_addrs;        /* 매핑된 페이지 수 (= DMA 버스 주소 개수) */
    uint64_t            addrs[1];       /* DMA 버스 주소 배열 (가변 길이, NVMe 디바이스가 이 주소로 DMA 수행) */
};



/*
 * map_userspace - 사용자 공간 호스트 메모리를 pin하고 DMA 버스 주소로 매핑한다.
 * get_user_pages()로 물리 페이지를 고정하고 dma_map_page()로 버스 주소를 얻는다.
 */
struct map* map_userspace(struct list* list, const struct ctrl* ctrl, u64 vaddr, unsigned long n_pages);



/*
 * unmap_and_release - DMA 매핑을 해제하고 리소스를 정리한다.
 * 리스트에서 제거 → release 콜백 호출(DMA 언매핑 + 페이지 unpin) → 메모리 해제
 */
void unmap_and_release(struct map* map);



#ifdef _CUDA
/*
 * map_device_memory - CUDA GPU 디바이스 메모리를 DMA 매핑한다.
 * nvidia_p2p_get_pages()로 GPU 페이지를 pin하고 nvidia_p2p_dma_map_pages()로 버스 주소를 얻는다.
 */
struct map* map_device_memory(struct list* list, const struct ctrl* ctrl, u64 vaddr, unsigned long n_pages, struct list* ctrl_list);
#endif



/*
 * map_find - 가상 주소와 현재 프로세스로 매핑을 검색한다.
 * 같은 프로세스(current)가 소유한 매핑 중 vaddr이 일치하는 것을 반환한다.
 */
struct map* map_find(const struct list* list, u64 vaddr);


#endif /* __LIBNVM_HELPER_MAP_H__ */
