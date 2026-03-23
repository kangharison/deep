/* nvm_util.h - NVMe 유틸리티 매크로 및 헬퍼 함수 헤더 */
/* 비트 조작, 페이지 정렬, 주소 변환, 캐시 관리, NVMe 커맨드/컴플리션 필드 접근 매크로를 제공한다 */
/* GPU용 lane_id, warp_id, SM ID 조회 인라인 함수도 포함한다 */
#ifndef __NVM_UTIL_H__
#define __NVM_UTIL_H__
//#ifndef __CUDACC__
//#define __device__
//#define __host__
//#endif

#include <nvm_types.h>
#include <stddef.h>
#include <stdint.h>
#include <cuda.h>

#ifdef __DIS_CLUSTER__
#include <sisci_types.h>
#include <sisci_api.h>
#endif



/* Convenience macro for creating a bit mask */
/* 지정한 비트 수만큼의 비트 마스크를 생성한다 (예: num_bits=3 -> 0b111 = 7) */
#define _NVM_MASK(num_bits) \
    ((1ULL << (num_bits)) - 1)

/* hi비트부터 lo비트까지의 부분 비트 마스크를 생성한다 (예: hi=5, lo=2 -> 0b00111100) */
#define _NVM_MASK_PART(hi, lo) \
    (_NVM_MASK((hi) + 1) - _NVM_MASK(lo))


/* DIS 클러스터 환경에서 캐시 비일관성(__NO_COHERENCE__)이 있을 때 캐시 플러시 함수 */
#if defined( __NO_COHERENCE__ ) && defined( __DIS_CLUSTER__ )
#ifdef __CUDACC__
__host__ __device__
#endif
static inline
void _nvm_cache_flush(void* ptr, size_t size)
{
#ifndef __CUDA_ARCH__
    sci_error_t err;
    /* SISCI API로 지정 메모리 영역의 CPU 캐시를 플러시하여 디바이스가 최신 데이터를 볼 수 있게 한다 */
    SCICacheSync(NULL, ptr, size, SCI_FLAG_CACHE_FLUSH, &err);
#endif
}

/* 캐시 플러시 매크로 - 비일관성 환경에서만 실제 동작, 그 외에는 빈 매크로 */
#define nvm_cache_flush(ptr, size) _nvm_cache_flush(ptr, size)
#else
#define nvm_cache_flush(ptr, size)
#endif



/* DIS 클러스터 환경에서 캐시 비일관성 시 캐시 무효화(invalidate) 함수 */
#if defined( __NO_COHERENCE__ ) && defined( __DIS_CLUSTER__ )
#ifdef __CUDACC__
__host__ __device__
#endif
static inline
void _nvm_cache_invalidate(void* ptr, size_t size)
{
#ifndef __CUDA_ARCH__
    sci_error_t err;
    /* 캐시 플러시 + 무효화: 해당 영역의 캐시를 메모리에 쓰고, 캐시 라인을 무효화하여 다음 읽기 시 메모리에서 가져오게 한다 */
    SCICacheSync(NULL, ptr, size, SCI_FLAG_CACHE_FLUSH | SCI_FLAG_CACHE_INVALIDATE, &err);
#endif
}

/* 캐시 무효화 매크로 - 비일관성 환경에서만 실제 동작 */
#define nvm_cache_invalidate(ptr, size) _nvm_cache_invalidate(ptr, size)
#else
#define nvm_cache_invalidate(ptr, size)
#endif



/* DIS 클러스터 환경에서 Write Combining Buffer를 플러시하는 함수 */
#if defined( __DIS_CLUSTER__ )
#ifdef __CUDACC__
__host__ __device__
#endif
static inline
void _nvm_wcb_flush()
{
#ifndef __CUDA_ARCH__
    /* SISCI API로 Write Combining Buffer를 플러시하여 원격 메모리 쓰기가 완료되도록 보장한다 */
    SCIFlush(NULL, 0);
#endif
}

/* WCB 플러시 매크로 - DIS 클러스터 환경에서만 실제 동작 */
#define nvm_wcb_flush() _nvm_wcb_flush()
#else
#define nvm_wcb_flush()
#endif



/* Extract specific bits */
/* 값 v에서 hi비트부터 lo비트까지를 추출한다 (Read Bits) */
/* 예: _RB(0xFF, 7, 4) -> 0x0F (상위 4비트 추출) */
#define _RB(v, hi, lo)      \
    ( ( (v) & _NVM_MASK_PART((hi), (lo)) ) >> (lo) )


/* Set specifics bits */
/* 값 v를 hi비트부터 lo비트 위치에 배치한다 (Write Bits) */
/* 예: _WB(0x0F, 7, 4) -> 0xF0 (상위 4비트 위치에 배치) */
#define _WB(v, hi, lo)      \
    ( ( (v) << (lo) ) & _NVM_MASK_PART((hi), (lo)) )


/* Offset to a register */
/* 기준 포인터 p에서 offs 바이트 오프셋에 있는 bits 비트 크기의 레지스터 포인터를 반환한다 */
/* NVMe BAR0 레지스터 접근에 사용 (예: _REG(bar, 0x14, 32)는 CC 레지스터) */
#define _REG(p, offs, bits) \
    ((volatile uint##bits##_t *) (((volatile unsigned char*) ((volatile void*) (p))) + (offs)))



/*
 * Calculate block number from page number.
 * 페이지 번호에서 블록 번호를 계산한다.
 * 페이지 크기와 블록 크기가 다를 때 변환에 사용한다.
 */
#define NVM_PAGE_TO_BLOCK(page_size, block_size, pageno)    \
    (((page_size) * (pageno)) / (block_size))



/*
 * Calculate page number from block number.
 * 블록 번호에서 페이지 번호를 계산한다.
 */
#define NVM_BLOCK_TO_PAGE(page_size, block_size, blockno)   \
    (((block_size) * (blockno)) / (page_size))


/*
 * Create mask to clear away address offset.
 * 주소 오프셋을 제거하기 위한 페이지 마스크를 생성한다.
 * 예: page_size=4096(0x1000)이면 마스크는 0xFFFFF000
 */
#define NVM_PAGE_MASK(page_size)                    \
    ~((page_size) - 1)


/*
 * Round address down to nearest page alignment.
 * 주소를 가장 가까운 페이지 경계로 내림 정렬한다.
 */
#define NVM_ADDR_MASK(addr, page_size)              \
    (((uint64_t) (addr)) & NVM_PAGE_MASK((page_size)))



/*
 * Align size to page boundary.
 * 크기를 페이지 경계로 올림 정렬한다.
 * 예: size=5000, page_size=4096이면 결과는 8192
 */
#define NVM_PAGE_ALIGN(size, page_size)             \
    (((size) + (page_size) - 1) & NVM_PAGE_MASK((page_size)))


/*
 * Calculate page-aligned offset into address.
 * 기준 주소에서 pageno번째 페이지의 주소를 계산한다.
 */
#define NVM_ADDR_OFFSET(addr, page_size, pageno)    \
    (((uint64_t) (addr)) + ((page_size) * (pageno)))


/*
 * Calculate page-aligned offset into pointer.
 * 기준 포인터에서 pageno번째 페이지의 포인터를 계산한다.
 */
#define NVM_PTR_OFFSET(ptr, page_size, pageno)      \
    ((void*) (((unsigned char*) (ptr)) + ((page_size) * (pageno))))


/*
 * Align size to controller pages.
 * 크기를 컨트롤러 페이지 크기로 올림 정렬한다.
 */
#define NVM_CTRL_ALIGN(ctrl_ptr, size)              \
    NVM_PAGE_ALIGN((size), (ctrl_ptr)->page_size)


/*
 * Convert size to number of controller pages.
 * 크기를 컨트롤러 페이지 수로 변환한다 (올림).
 */
#define NVM_CTRL_PAGES(ctrl_ptr, size)              \
    (NVM_CTRL_ALIGN((ctrl_ptr), (size)) / (ctrl_ptr)->page_size)


/*
 * Align size to page size.
 * 크기를 DMA 페이지 크기로 올림 정렬한다.
 */
#define NVM_DMA_ALIGN(dma_ptr, size)                \
    NVM_PAGE_ALIGN((size), (dma_ptr)->page_size)


/*
 * Calculate controller page-aligned offset into DMA handle pointer.
 * DMA 핸들의 가상 주소에서 pageno번째 페이지의 포인터를 계산한다.
 */
#define NVM_DMA_OFFSET(dma_ptr, pageno)             \
    NVM_PTR_OFFSET((dma_ptr)->vaddr, (dma_ptr)->page_size, (pageno))


/*
 * Calculate number of pages needed for a
 * submission queue (SQ) with a given size.
 * 주어진 엔트리 수의 SQ에 필요한 페이지 수를 계산한다.
 * SQ 엔트리는 64바이트(nvm_cmd_t)이다.
 */
#define NVM_SQ_PAGES(ctrl_ptr, qs) \
    ((((uint16_t) ((qs) - 1))* sizeof(nvm_cmd_t)) / (ctrl_ptr)->page_size + 1)


/*
 * Calculate number of pages needed for a
 * completion queue (CQ) with a given size.
 * 주어진 엔트리 수의 CQ에 필요한 페이지 수를 계산한다.
 * CQ 엔트리는 16바이트(nvm_cpl_t)이다.
 */
#define NVM_CQ_PAGES(ctrl_ptr, qs) \
    ((((uint16_t) ((qs) - 1)) * sizeof(nvm_cpl_t)) / (ctrl_ptr)->page_size + 1)


/*
 * Number of submission queue entries aligned to a page size.
 * 한 페이지에 들어가는 SQ 엔트리 수를 계산한다.
 */
#define NVM_SQ_SIZE(ctrl_ptr, num_pages)   \
    ((ctrl_ptr)->page_size / sizeof(nvm_cmd_t))


/*
 * Number of completion queue entries aligned to a page size.
 * 한 페이지에 들어가는 CQ 엔트리 수를 계산한다.
 */
#define NVM_CQ_SIZE(ctrl_ptr, num_pages)   \
    ((ctrl_ptr)->page_size / sizeof(nvm_cpl_t))



/* Standard fields in a command */
/* NVMe 커맨드의 표준 필드 접근 매크로 */
#define NVM_CMD_CID(p)              _REG(p, 2, 16)   /* 커맨드 ID (DWORD0의 상위 16비트, 바이트 오프셋 2) */
#define NVM_CMD_NSID(p)             _REG(p, 1, 32)   /* 네임스페이스 ID (DWORD1, 바이트 오프셋 4) */


/* Standard fields in a completion */
/* NVMe 컴플리션의 표준 필드 접근 매크로 */
#define NVM_CPL_CID(p)              _REG(p, 12, 16)  /* 커맨드 ID (DWORD3의 상위 16비트) - 대응하는 커맨드를 식별 */
#define NVM_CPL_SQHD(p)             _REG(p,  8, 16)  /* SQ 헤드 포인터 (DWORD2의 하위 16비트) - 컨트롤러가 처리한 SQ 위치 */
#define NVM_CPL_SQID(p)             _REG(p, 10, 16)  /* SQ 식별자 (DWORD2의 상위 16비트) - 어떤 SQ의 커맨드인지 표시 */
#define NVM_CPL_SF(p)               _REG(p, 14, 16)  /* 상태 필드 (DWORD3의 상위 16비트) - SCT, SC, DNR, Phase 등 포함 */
#define NVM_CPL_STATUS(p)           NVM_CPL_SF(p)     /* 상태 필드의 별칭 */


/* Convenience macro for creating a default CID based on submission queue */
/* SQ 기반으로 기본 CID를 생성하는 편의 매크로 */
/* tail 인덱스와 위상 비트를 조합하여 고유한 CID를 만든다 */
#define NVM_DEFAULT_CID(sq)         ((uint16_t) ((sq)->tail + (!(sq)->phase) * (sq)->qs))


#ifdef __cplusplus
extern "C" {
#endif
/*
 * Get controller associated with admin queue-pair reference.
 * 어드민 큐 쌍 참조로부터 연결된 컨트롤러 핸들을 가져온다.
 */
const nvm_ctrl_t* nvm_ctrl_from_aq_ref(nvm_aq_ref ref);
#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif
/*
 * Get controller associated with DMA window
 * DMA 윈도우에 연결된 컨트롤러 핸들을 가져온다.
 */
const nvm_ctrl_t* nvm_ctrl_from_dma(const nvm_dma_t* dma);
#ifdef __cplusplus
}
#endif



#if defined( __DIS_CLUSTER__ )
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Get cluster node identifier from map.
 * DMA 매핑으로부터 DIS 클러스터 노드 식별자를 가져온다.
 */
uint32_t nvm_dis_node_from_dma(const nvm_dma_t* dma);


/*
 * Get cluster node identifier from controller.
 * 컨트롤러로부터 DIS 클러스터 노드 식별자를 가져온다.
 */
uint32_t nvm_dis_node_from_ctrl(const nvm_ctrl_t* ctrl);

#ifdef __cplusplus
}
#endif
#endif

/* GPU 인라인 함수: 현재 스레드의 warp 내 lane ID를 반환한다 (0~31) */
__forceinline__ __device__ uint32_t lane_id()
{
    uint32_t ret;
    asm volatile ("mov.u32 %0, %laneid;" : "=r"(ret));  /* PTX 인라인 어셈블리로 lane ID 읽기 */
    return ret;
}

/* GPU 인라인 함수: 현재 스레드의 warp ID를 반환한다 */
/* 주의: threadIdx.x / 32와 같지 않을 수 있다 (SM 내부 스케줄링에 의존) */
__forceinline__ __device__ unsigned warp_id()
{
    // this is not equal to threadIdx.x / 32
    unsigned ret;
    asm volatile ("mov.u32 %0, %warpid;" : "=r"(ret));  /* PTX 인라인 어셈블리로 warp ID 읽기 */
    return ret;
}

/* GPU 인라인 함수: 현재 스레드가 실행 중인 SM(Streaming Multiprocessor)의 ID를 반환한다 */
__forceinline__ __device__ uint32_t get_smid() {
     uint32_t ret;
     asm  ("mov.u32 %0, %smid;" : "=r"(ret) );  /* PTX 인라인 어셈블리로 SM ID 읽기 */
     return ret;
}

#endif /* __NVM_UTIL_H__ */
