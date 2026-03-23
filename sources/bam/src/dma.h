/* dma.h - DMA 내부 타입 및 함수 선언 헤더
 *
 * DMA 매핑의 내부 구현에 사용되는 가상 주소 범위 디스크립터(va_range)와
 * 관련 콜백 타입, 초기화/조회 함수를 선언한다.
 * 이 헤더는 내부 전용이며 외부 API에는 포함되지 않는다.
 */
#ifndef __NVM_INTERNAL_DMA_H__
#define __NVM_INTERNAL_DMA_H__

#include <nvm_types.h>    // nvm_dma_t, nvm_ctrl_t 타입 정의
#include <stddef.h>       // size_t 정의
#include <stdint.h>       // uint64_t 등 정수형 타입
#include <stdbool.h>      // bool 타입


/* 전방 선언 */
struct va_range;



/* 가상 주소 범위 해제 콜백 타입.
 * 디바이스 언매핑 완료 후 가상 주소 매핑을 해제할 때 호출된다.
 * 예: free() 또는 munmap() 등의 함수 포인터를 저장한다.
 */
typedef void (*va_range_free_t)(struct va_range* va);



/* 가상 주소 범위 디스크립터.
 * 유저스페이스에 매핑된 커스텀 주소 범위를 기술한다.
 * DMA 매핑 시 이 범위를 디바이스용 버스 주소로 변환한다.
 */
struct va_range
{
    bool            remote;     // 원격 메모리 여부 (true: GPU 등 원격 디바이스 메모리)
    volatile void*  vaddr;      // 매핑된 가상 주소 범위의 시작 주소
    size_t          page_size;  // 매핑의 정렬 단위 (페이지 크기)
    size_t          n_pages;    // 주소 범위의 페이지 수
};


/* va_range 초기화 매크로 - 구조체 리터럴로 va_range를 생성한다 */
#define VA_RANGE_INIT(remote, vaddr, page_size, n_pages)    \
    (struct va_range) {(remote), (vaddr), (page_size), (n_pages)}


/* _nvm_dma_init - 가상 주소 범위를 컨트롤러용으로 매핑하고 DMA 핸들을 생성한다.
 * 디바이스 드라이버의 map_range 콜백을 통해 IOMMU/DMA 매핑을 수행한다.
 */
int _nvm_dma_init(nvm_dma_t** handle,
                  const nvm_ctrl_t* ctrl,
                  struct va_range* va,
                  va_range_free_t release);



/* _nvm_dma_va - DMA 핸들에서 내부 가상 주소 범위 디스크립터를 추출한다.
 * 핸들의 원본 va_range 정보(페이지 크기, 페이지 수, 원격 여부 등)에 접근할 때 사용한다.
 */
const struct va_range* _nvm_dma_va(const nvm_dma_t* handle);


#endif /* __NVM_INTERNAL_DMA_H__ */
