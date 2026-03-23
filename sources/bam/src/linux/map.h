/*
 * map.h - 사용자 공간 DMA 매핑의 내부 데이터 구조 정의.
 * ioctl을 통해 커널 모듈에 DMA 매핑을 요청할 때 사용하는 매핑 컨테이너와 메모리 타입을 정의한다.
 */
#ifndef __NVM_INTERNAL_LINUX_MAP_H__
#define __NVM_INTERNAL_LINUX_MAP_H__
#ifdef __linux__

#include "linux/ioctl.h"
#include "dma.h"


/*
 * mapping_type - 매핑할 메모리의 종류를 구분하는 열거형.
 * DMA 매핑 해제 시 버퍼 소유권에 따라 처리 방법이 달라지므로 타입 구분이 필요하다.
 */
enum mapping_type
{
    MAP_TYPE_CUDA   =   0x1,   /* CUDA GPU 디바이스 메모리: NVIDIA P2P API로 매핑/해제한다 */
    MAP_TYPE_HOST   =   0x2,   /* 호스트 메모리 (RAM): 사용자가 제공한 버퍼, 해제 시 free하지 않는다 */
    MAP_TYPE_API    =   0x4    /* API 할당 메모리 (RAM): posix_memalign으로 할당, 해제 시 free한다 */
};



/*
 * ioctl_mapping - DMA 매핑 정보를 담는 컨테이너 구조체.
 * 메모리 타입, 원본 버퍼 주소, 그리고 va_range(가상 주소 범위 정보)를 하나로 묶는다.
 * container_of 패턴으로 va_range에서 이 구조체를 역추적할 수 있다.
 */
struct ioctl_mapping
{
    enum mapping_type   type;   /* 메모리 종류 (CUDA/HOST/API) */
    void*               buffer; /* 매핑 대상 원본 버퍼의 시작 주소 */
    struct va_range     range;  /* 가상 주소 범위 디스크립터 (페이지 크기, 페이지 수 등) */
};


#endif /* __linux__ */
#endif /* __NVM_INTERNAL_LINUX_MAP_H__ */
