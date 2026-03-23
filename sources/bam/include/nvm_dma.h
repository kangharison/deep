/* nvm_dma.h - NVMe DMA 매핑 관리 API 헤더 */
/* NVMe 컨트롤러가 DMA로 접근할 메모리 영역을 매핑/해제하는 함수들을 선언한다 */
/* 호스트 메모리, GPU 디바이스 메모리, SmartIO(DIS) 세그먼트 등 다양한 메모리 소스를 지원한다 */
#ifndef __NVM_DMA_H__
#define __NVM_DMA_H__
// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <nvm_types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __DIS_CLUSTER__
#include <sisci_types.h>
#endif



/*
 * Create DMA mapping descriptor from physical/bus addresses.
 * 물리/버스 주소로부터 DMA 매핑 디스크립터를 생성한다.
 *
 * 호스트 페이지 크기와 컨트롤러 페이지 크기(MPS)가 다를 수 있으므로,
 * 이 함수가 필요한 오프셋을 자동 계산한다.
 * 가상 메모리는 연속이지만 물리 페이지는 비연속일 수 있다.
 * 물리/버스 주소는 컨트롤러 페이지 크기에 맞게 정렬되어야 한다.
 *
 * Create a DMA mapping descriptor, describing a region of memory that is
 * accessible for the NVM controller. The caller must supply physical/bus
 * addresses of physical memory pages, page size and total number of pages.
 * As the host's page size may differ from the controller's page size (MPS),
 * this function will calculate the necessary offsets into the actual memory
 * pages.
 *
 * While virtual memory is assumed to be continuous, the physical pages do not
 * need to be contiguous. Physical/bus addresses must be aligned to the
 * controller's page size.
 *
 * Note: vaddr can be NULL.
 */
int nvm_dma_map(nvm_dma_t** map,                // Mapping descriptor reference (매핑 디스크립터 참조 포인터)
                const nvm_ctrl_t* ctrl,         // NVM controller reference (NVMe 컨트롤러 핸들)
                void* vaddr,                    // Pointer to userspace memory (can be NULL if not required) (유저스페이스 메모리 포인터, NULL 가능)
                size_t page_size,               // Physical page size (물리 페이지 크기, 호스트 OS의 페이지 크기)
                size_t n_pages,                 // Number of pages to map (매핑할 페이지 수)
                const uint64_t* page_addrs);    // List of physical/bus addresses to the pages (각 페이지의 물리/버스 주소 배열)



/*
 * Create DMA mapping descriptor using offsets from a previously
 * created DMA descriptor.
 * 기존 DMA 디스크립터의 오프셋을 재사용하여 새 DMA 매핑 디스크립터를 생성한다.
 * 동일한 물리 메모리를 다른 컨텍스트에서 참조할 때 유용하다.
 */
int nvm_dma_remap(nvm_dma_t** new_map,          /* [out] 새 DMA 매핑 디스크립터 */
                  const nvm_dma_t* other_map);   /* 기존 DMA 매핑 디스크립터 (소스) */



/*
 * Remove DMA mapping descriptor.
 * DMA 매핑 디스크립터를 제거한다.
 *
 * 필요한 경우 DMA 매핑을 해제하고 디스크립터를 파괴한다.
 *
 * Unmap DMA mappings (if necessary) and remove the descriptor.
 * This function destroys the descriptor.
 */
void nvm_dma_unmap(nvm_dma_t* map);  /* 제거할 DMA 매핑 디스크립터 */



/*
 * Create DMA mapping descriptor from virtual address using the kernel module.
 * 커널 모듈을 사용하여 가상 주소로부터 DMA 매핑 디스크립터를 생성한다.
 *
 * nvm_dma_map과 유사하지만 물리/버스 주소를 직접 전달할 필요가 없다.
 * 커널 모듈이 가상 주소에서 물리 주소를 자동으로 변환한다.
 *
 * This function is similar to nvm_dma_map, except the user is not required
 * to pass physical/bus addresses.
 *
 * Note: vaddr can not be NULL, and must be aligned to system page size.
 */
int nvm_dma_map_host(nvm_dma_t** map,           /* [out] DMA 매핑 디스크립터 */
                     const nvm_ctrl_t* ctrl,     /* NVMe 컨트롤러 핸들 */
                     void* vaddr,                /* 호스트 가상 주소 (NULL 불가, 시스템 페이지 크기 정렬 필수) */
                     size_t size);               /* 매핑할 메모리 크기 (바이트) */



//#if ( defined( __CUDA__ ) || defined( __CUDACC__ ) )

/*
 * Create DMA mapping descriptor from CUDA device pointer using the kernel
 * module.
 * 커널 모듈을 사용하여 CUDA 디바이스 포인터로부터 DMA 매핑 디스크립터를 생성한다.
 *
 * nvm_dma_map_host와 유사하지만 GPU 메모리 포인터를 받는다.
 * BAM의 핵심 기능: GPU 메모리를 NVMe 컨트롤러가 직접 DMA로 접근할 수 있게 매핑한다.
 * 이를 통해 GPU-Direct Storage 방식의 데이터 전송이 가능하다.
 *
 * This function is similar to nvm_dma_map_host, except the memory
 * pointer must be a valid CUDA device pointer (see manual for
 * cudaGetPointerAttributes).
 *
 * The controller handle must have been created using the kernel module.
 *
 * Note: vaddr can not be NULL, and must be aligned to GPU page size.
 */
int nvm_dma_map_device(nvm_dma_t** map,          /* [out] DMA 매핑 디스크립터 */
                       const nvm_ctrl_t* ctrl,    /* NVMe 컨트롤러 핸들 */
                       void* devptr,              /* CUDA 디바이스 포인터 (GPU 메모리 주소, NULL 불가) */
                       size_t size);              /* 매핑할 GPU 메모리 크기 (바이트) */

//#endif /* __CUDA__ */



#if defined( __DIS_CLUSTER__ )

/*
 * Create DMA mapping descriptor from local SISCI segment.
 * 로컬 SISCI 세그먼트로부터 DMA 매핑 디스크립터를 생성한다.
 *
 * 로컬 세그먼트 핸들러에서 DMA 매핑 디스크립터를 생성하고,
 * 세그먼트를 역매핑하여 컨트롤러가 접근 가능하게 한다.
 * 세그먼트 메모리는 항상 연속이고 페이지 정렬되어 있으므로
 * 물리 주소 계산이 불필요하다.
 *
 * Create DMA mapping descriptor from a local segment handler, and
 * reverse-map the segment making it accessible from the controller.
 * As segment memory is always continuous and page-aligned, it is not
 * necessary to calculate physical memory addresses. However, the user
 * should ensure that the mapping size is aligned to a controller
 * page-size (MPS).
 *
 * The controller handle must have been created using SmartIO, and
 * the segment must already be prepared on the local adapter.
 */
int nvm_dis_dma_map_local(nvm_dma_t** map,              // Mapping descriptor reference (매핑 디스크립터 참조)
                          const nvm_ctrl_t* ctrl,       // NVM controller handle (NVMe 컨트롤러 핸들)
                          uint32_t dis_adapter,         // Local DIS adapter segment is prepared on (세그먼트가 준비된 로컬 DIS 어댑터)
                          sci_local_segment_t segment,  // Local segment descriptor (로컬 세그먼트 디스크립터)
                          bool map_vaddr);              // Should function also map segment into local space (로컬 가상 주소 매핑 여부)

#endif /* __DIS_CLUSTER__ */



#if defined( __DIS_CLUSTER__ )

/*
 * Create DMA mapping descriptor from remote SISCI segment.
 * 원격 SISCI 세그먼트로부터 DMA 매핑 디스크립터를 생성한다.
 *
 * nvm_dis_dma_map_local과 유사하지만 원격 노드의 세그먼트를 사용한다.
 * 원격 세그먼트는 이미 연결(connect)되어 있어야 한다.
 *
 * Create DMA mapping descriptor from a remote segment handler, and
 * reverse-map the segment making it accessible from the controller.
 * This function is similar to nvm_dis_dma_map_local.
 *
 * The remote segment must already be connected.
 *
 * Note: You should generally prefer write combining, except
 *       for mapped device registers that require fine-grained writes.
 */
int nvm_dis_dma_map_remote(nvm_dma_t** map,             // Mapping descriptor reference (매핑 디스크립터 참조)
                           const nvm_ctrl_t* ctrl,      // NVM controller handle (NVMe 컨트롤러 핸들)
                           sci_remote_segment_t segment,// Remote segment descriptor (원격 세그먼트 디스크립터)
                           bool map_vaddr,              // Should function also map segment into local space (로컬 가상 주소 매핑 여부)
                           bool map_wc);                // Should function map with write combining (Write Combining 매핑 여부)

#endif /* __DIS_CLUSTER__ */



#if ( !defined( __CUDA__ ) && !defined( __CUDACC__ ) ) && ( defined (__unix__) )
/*
 * Short-hand function for allocating a page aligned buffer and mapping it
 * for the controller.
 * 페이지 정렬된 버퍼를 할당하고 컨트롤러용으로 매핑하는 단축 함수.
 *
 * 내부적으로 posix_memalign으로 메모리를 할당한 뒤 nvm_dma_map_host를 호출한다.
 *
 * Note: this function will not work if you are using the CUDA API
 */
int nvm_dma_create(nvm_dma_t** map,              /* [out] DMA 매핑 디스크립터 */
                   const nvm_ctrl_t* ctrl,        /* NVMe 컨트롤러 핸들 */
                   size_t size);                  /* 할당할 메모리 크기 (바이트) */
#endif



#if defined( __DIS_CLUSTER__ )
/*
 * Create device memory segment and map it for the controller.
 * 디바이스 메모리 세그먼트를 생성하고 컨트롤러용으로 매핑한다.
 *
 * mem_hints가 0이면 로컬 세그먼트를 생성한다.
 *
 * Short-hand function for creating a device memory segment.
 * If mem_hints is 0, the API will create a local segment instead.
 */
int nvm_dis_dma_create(nvm_dma_t** map,           /* [out] DMA 매핑 디스크립터 */
                       const nvm_ctrl_t* ctrl,     /* NVMe 컨트롤러 핸들 */
                       size_t size,                /* 세그먼트 크기 (바이트) */
                       unsigned int mem_hints);    /* 메모리 힌트 (0이면 로컬 세그먼트) */

#endif /* __DIS_CLUSTER__ */



#if defined ( __DIS_CLUSTER__ )

/*
 * Note: This function requires the IOMMU to be enabled.
 * IOMMU가 활성화되어 있어야 동작하는 DIS 환경의 호스트 DMA 매핑 함수.
 */
int nvm_dis_dma_map_host(nvm_dma_t** map,         /* [out] DMA 매핑 디스크립터 */
                         const nvm_ctrl_t* ctrl,   /* NVMe 컨트롤러 핸들 */
                         void* vaddr,              /* 호스트 가상 주소 */
                         size_t size);             /* 매핑할 크기 */

#endif


#if ( ( defined( __CUDA__ ) || defined( __CUDACC__ ) ) && defined( __DIS_CLUSTER__ ) )

/* DIS 클러스터 환경에서 CUDA 디바이스 메모리를 NVMe 컨트롤러용으로 DMA 매핑한다 */
int nvm_dis_dma_map_device(nvm_dma_t** map,        /* [out] DMA 매핑 디스크립터 */
                           const nvm_ctrl_t* ctrl,  /* NVMe 컨트롤러 핸들 */
                           void* devptr,            /* CUDA 디바이스 포인터 */
                           size_t size);            /* 매핑할 크기 */

#endif /* __DIS_CLUSTER__ && __CUDA__ */




#endif /* __NVM_DMA_H__ */
