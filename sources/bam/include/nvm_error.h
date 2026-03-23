/* nvm_error.h - NVMe 에러 처리 매크로 및 유틸리티 헤더 */
/* NVMe 컴플리션 상태 코드 추출, 에러 패킹/언패킹, 에러 문자열 변환 기능을 제공한다 */
#ifndef __NVM_ERROR_H__
#define __NVM_ERROR_H__

// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <stdint.h>
#include <nvm_types.h>
#include <nvm_util.h>




/* Get the status code type of an NVM completion. */
/* NVMe 컴플리션에서 상태 코드 타입(SCT)을 추출한다 (비트 11:9) */
/* SCT: 0=Generic, 1=Command Specific, 2=Media Error, 3=Path Related 등 */
#define NVM_ERR_SCT(cpl)            ((uint8_t) _RB(*NVM_CPL_STATUS(cpl), 11, 9))



/* Get the status code of an NVM completion */
/* NVMe 컴플리션에서 상태 코드(SC)를 추출한다 (비트 8:1) */
/* SC는 SCT에 따라 의미가 달라지며, 구체적인 에러 원인을 나타낸다 */
#define NVM_ERR_SC(cpl)             ((uint8_t) _RB(*NVM_CPL_STATUS(cpl), 8, 1))



/* Is do not retry flag set? */
/* DNR(Do Not Retry) 플래그가 설정되었는지 확인한다 (비트 15) */
/* DNR=1이면 동일 커맨드를 재시도해도 실패할 것임을 의미한다 */
#define NVM_ERR_DNR(cpl)            (!!_RB(*NVM_CPL_STATUS(cpl), 15, 15))



/* Is there more? (Get log page) */
/* MORE 플래그가 설정되었는지 확인한다 (비트 14) */
/* Get Log Page 응답에서 추가 데이터가 있음을 나타낸다 */
#define NVM_ERR_MORE(cpl)           (!!_RB(*NVM_CPL_STATUS(cpl), 14, 14))



/* Extract value from status field from NVM completion */
/* NVMe 컴플리션의 상태 필드에서 에러 값을 추출한다 */
/* SCT와 SC를 결합하여 음수 정수로 반환한다 (성공 시 0, 에러 시 음수) */
#define NVM_ERR_STATUS(cpl)         \
    ((int) ( (cpl) != NULL ? -((NVM_ERR_SCT(cpl) << 8) | NVM_ERR_SC(cpl)) : 0 ))


/* Convenience macro for checking if an NVM completion indicates success. */
/* NVMe 컴플리션이 성공을 나타내는지 확인하는 편의 매크로 (SCT=0, SC=0이면 성공) */
#define NVM_ERR_OK(cpl)             ( !NVM_ERR_SCT(cpl) && !NVM_ERR_SC(cpl) )



/* Pack errno and NVM completion status into a single status variable */
/* errno와 NVMe 컴플리션 상태를 하나의 상태 변수로 패킹한다 */
/* errno가 있으면(양수) errno를 반환하고, 없으면 NVMe 상태(음수)를 반환한다 */
#define NVM_ERR_PACK(cpl, err)      \
    ((int) ( (err) != 0 ? (err) : NVM_ERR_STATUS(cpl) ) )



/* Extract values from packed status */
/* 패킹된 상태에서 개별 값을 추출한다 */
/* 양수면 errno, 음수면 NVMe 에러 (SCT/SC로 분리) */
#define NVM_ERR_UNPACK_ERRNO(status)    ((status > 0) ? (status) : 0)       /* 패킹된 상태에서 errno 추출 (양수면 errno, 0이하면 0) */
#define NVM_ERR_UNPACK_SCT(status)      ((status < 0) ? (((-status) >> 8) & 0xff) : 0)  /* 패킹된 상태에서 SCT 추출 (음수면 상위 8비트) */
#define NVM_ERR_UNPACK_SC(status)       ((status < 0) ? ((-status) & 0xff) : 0)         /* 패킹된 상태에서 SC 추출 (음수면 하위 8비트) */


/* Check if everything is okay */
/* 상태가 정상(0)인지 확인하는 매크로 */
#define nvm_ok(status)              ( !(status) )



/*
 * Get an error string associated with the status code type and status code.
 * 상태 코드 타입과 상태 코드에 해당하는 에러 문자열을 반환한다.
 *
 * 패킹된 상태가 일반 errno이면 strerror()를 호출한다.
 * NVMe 에러이면 NVMe 스펙에 정의된 에러 메시지를 반환한다.
 *
 * This function calls strerror() if the packed status is a regular errno.
 */
const char* nvm_strerror(int status);  /* 패킹된 상태 값 (양수=errno, 음수=NVMe 에러) */




#endif /* __NVM_ERROR_H__ */
