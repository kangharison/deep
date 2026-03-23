/* nvm_admin.h - NVMe 어드민 커맨드 API 헤더 */
/* 컨트롤러/네임스페이스 식별, I/O 큐 생성/삭제, Feature 설정 등 어드민 커맨드를 수행하는 함수들을 선언한다 */
#ifndef __NVM_ADMIN_H__
#define __NVM_ADMIN_H__
// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <nvm_types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>



/*
 * Get controller information.
 * 컨트롤러 정보를 조회한다.
 *
 * IDENTIFY CONTROLLER 어드민 커맨드를 실행하여 NVMe 컨트롤러의
 * 모델명, 시리얼, 펌웨어, 최대 전송 크기 등의 정보를 가져온다.
 * buffer는 컨트롤러가 DMA로 결과를 기록할 임시 버퍼이다.
 */
int nvm_admin_ctrl_info(nvm_aq_ref ref,               // AQ pair reference (어드민 큐 쌍 참조 핸들)
                        struct nvm_ctrl_info* info,   // Controller information structure (결과를 저장할 컨트롤러 정보 구조체)
                        void* buffer,                 // Temporary buffer (must be at least 4 KB) (임시 DMA 버퍼, 최소 4KB)
                        uint64_t ioaddr);             // Bus address of buffer as seen by the controller (버퍼의 컨트롤러 관점 버스 주소)



/*
 * Get namespace information.
 * 네임스페이스 정보를 조회한다.
 *
 * IDENTIFY NAMESPACE 어드민 커맨드를 실행하여 지정 네임스페이스의
 * 크기, 용량, 블록 크기 등의 정보를 가져온다.
 */
int nvm_admin_ns_info(nvm_aq_ref ref,                 // AQ pair reference (어드민 큐 쌍 참조 핸들)
                      struct nvm_ns_info* info,       // NVM namespace information (결과를 저장할 네임스페이스 정보 구조체)
                      uint32_t ns_id,                 // Namespace identifier (조회할 네임스페이스 ID, 1부터 시작)
                      void* buffer,                   // Temporary buffer (must be at least 4 KB) (임시 DMA 버퍼, 최소 4KB)
                      uint64_t ioaddr);               // Bus address of buffer as seen by controller (버퍼의 컨트롤러 관점 버스 주소)



/*
 * Make controller allocate and reserve queues.
 * 컨트롤러에 I/O 큐 수를 할당하고 예약한다.
 *
 * SET FEATURES (Number of Queues) 어드민 커맨드를 실행하여
 * 원하는 CQ/SQ 수를 컨트롤러에 요청한다.
 */
int nvm_admin_set_num_queues(nvm_aq_ref ref,   /* 어드민 큐 쌍 참조 핸들 */
                             uint16_t n_cqs,    /* 요청할 Completion Queue 수 */
                             uint16_t n_sqs);   /* 요청할 Submission Queue 수 */


/*
 * Retrieve the number of allocated queues.
 * 할당된 I/O 큐 수를 조회한다.
 *
 * GET FEATURES (Number of Queues) 어드민 커맨드를 실행한다.
 */
int nvm_admin_get_num_queues(nvm_aq_ref ref,    /* 어드민 큐 쌍 참조 핸들 */
                             uint16_t* n_cqs,   /* [out] 할당된 CQ 수 */
                             uint16_t* n_sqs);  /* [out] 할당된 SQ 수 */


/*
 * Make controller allocate number of queues before issuing them.
 * 큐를 발급하기 전에 컨트롤러가 큐 수를 할당하도록 요청한다.
 *
 * Set과 Get을 결합한 편의 함수로, 요청한 큐 수와 실제 할당된 큐 수를 동시에 처리한다.
 */
int nvm_admin_request_num_queues(nvm_aq_ref ref,  /* 어드민 큐 쌍 참조 핸들 */
                                uint16_t* n_cqs,   /* [in/out] 요청/할당된 CQ 수 */
                                uint16_t* n_sqs);  /* [in/out] 요청/할당된 SQ 수 */


/*
 * Create IO completion queue (CQ)
 * I/O Completion Queue를 생성한다.
 *
 * CREATE IO COMPLETION QUEUE 어드민 커맨드를 실행한다.
 * 호출자가 큐 메모리를 미리 0으로 초기화해야 한다.
 * 큐 엔트리 수가 한 페이지를 초과하면 DMA 메모리가 연속이어야 한다.
 * qs가 0이면 한 페이지 분량의 큐 메모리를 사용한다.
 *
 * Caller must set queue memory to zero manually.
 *
 * If number of queue entries (qs) exceeds a page,
 * DMA memory must be contiguous.
 *
 * If qs is 0, the API will use one page for queue memory.
 */
int nvm_admin_cq_create(nvm_aq_ref ref,                 // AQ pair reference (어드민 큐 쌍 참조 핸들)
                        nvm_queue_t* cq,                // CQ descriptor (생성된 CQ 정보를 저장할 디스크립터)
                        uint16_t id,                    // Queue identifier (큐 식별자, 1 이상의 고유 번호)
                        const nvm_dma_t* dma,           // Queue memory handle (큐 메모리의 DMA 핸들)
                        size_t page_offset,             // Number of pages to offset into the handle (DMA 핸들 내 페이지 오프셋)
                        size_t qs,                      // Queue size/depth (큐 크기/깊이, 0이면 한 페이지 기본값)
                        bool need_prp = false);                 // non-contiguous queue (비연속 큐 여부, true면 PRP 리스트 사용)

/*
 * Delete IO completion queue (CQ)
 * I/O Completion Queue를 삭제한다.
 *
 * DELETE IO COMPLETION QUEUE 어드민 커맨드를 실행한다.
 * 삭제 후 큐는 더 이상 사용할 수 없으며 재생성해야 한다.
 * 연결된 모든 Submission Queue를 먼저 삭제해야 한다.
 *
 * After calling this, the queue is no longer used and must be recreated.
 * All associated submission queues must be deleted first.
 */
int nvm_admin_cq_delete(nvm_aq_ref ref,  /* 어드민 큐 쌍 참조 핸들 */
                        nvm_queue_t* cq); /* 삭제할 CQ 디스크립터 */



/*
 * Create IO submission queue (SQ)
 * I/O Submission Queue를 생성한다.
 *
 * CREATE IO SUBMISSION QUEUE 어드민 커맨드를 실행한다.
 * 호출자가 큐 메모리를 미리 0으로 초기화해야 한다.
 * 각 SQ는 반드시 하나의 CQ와 쌍을 이루어야 한다.
 *
 * Caller must set queue memory to zero manually.
 *
 * If number of queue entries (qs) exceeds a page,
 * DMA memory must be contiguous.
 *
 * If qs is 0, the API will use one page for queue memory.
 */
int nvm_admin_sq_create(nvm_aq_ref ref,                 // AQ pair reference (어드민 큐 쌍 참조 핸들)
                        nvm_queue_t* sq,                // SQ descriptor (생성된 SQ 정보를 저장할 디스크립터)
                        const nvm_queue_t* cq,          // Descriptor to paired CQ (쌍을 이루는 CQ의 디스크립터)
                        uint16_t id,                    // Queue identifier (큐 식별자, 1 이상의 고유 번호)
                        const nvm_dma_t* dma,           // Queue memory handle (큐 메모리의 DMA 핸들)
                        size_t page_offset,             // Number of pages to offset into the handle (DMA 핸들 내 페이지 오프셋)
                        size_t qs,                      // Number of pages to use (큐 크기, 0이면 한 페이지 기본값)
                        bool need_prp = false);                 // non-contiguous queue (비연속 큐 여부)



/*
 * Delete IO submission queue (SQ)
 * I/O Submission Queue를 삭제한다.
 *
 * DELETE IO SUBMISSION QUEUE 어드민 커맨드를 실행한다.
 * 삭제 후 큐는 더 이상 사용할 수 없으며 재생성해야 한다.
 *
 * After calling this, the queue is no longer used and must be recreated.
 */
int nvm_admin_sq_delete(nvm_aq_ref ref,          /* 어드민 큐 쌍 참조 핸들 */
                        nvm_queue_t* sq,          /* 삭제할 SQ 디스크립터 */
                        const nvm_queue_t* cq);   /* 쌍을 이루는 CQ 디스크립터 */


/*
 * Get log page.
 * 로그 페이지를 조회한다.
 *
 * GET LOG PAGE 어드민 커맨드를 실행하여 에러 로그, SMART/Health 정보 등을 가져온다.
 */
int nvm_admin_get_log_page(nvm_aq_ref ref,         /* 어드민 큐 쌍 참조 핸들 */
                           uint32_t ns_id,          /* 네임스페이스 ID (0xFFFFFFFF면 전체) */
                           void* ptr,               /* 결과를 저장할 호스트 버퍼 */
                           uint64_t ioaddr,         /* 버퍼의 컨트롤러 관점 버스 주소 */
                           uint8_t log_id,          /* 로그 페이지 식별자 (1=에러, 2=SMART 등) */
                           uint64_t log_offset);    /* 로그 페이지 내 읽기 시작 오프셋 */


#endif /* #ifdef __NVM_ADMIN_H__ */
