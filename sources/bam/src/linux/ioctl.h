/*
 * ioctl.h - BAM 커널 모듈과 사용자 공간 라이브러리 간의 ioctl 인터페이스 정의.
 * 사용자 공간에서 커널 모듈에 DMA 매핑/해제를 요청할 때 사용하는 명령 코드와 데이터 구조를 정의한다.
 * 이 헤더는 사용자 공간(src/linux/)과 커널 공간(module/) 양쪽에서 공유된다.
 */
#ifndef __NVM_INTERNAL_LINUX_IOCTL_H__
#define __NVM_INTERNAL_LINUX_IOCTL_H__
#ifdef __linux__

#include <linux/types.h>
#include <asm/ioctl.h>   /* _IOW 매크로: ioctl 명령 번호를 생성하는 커널 매크로 */

/* ioctl 매직 넘버: 이 드라이버 고유의 ioctl 타입 식별자 (0x80) */
#define NVM_IOCTL_TYPE          0x80



/*
 * nvm_ioctl_map - ioctl을 통해 전달되는 메모리 매핑 요청 구조체.
 * 사용자 공간에서 커널로 매핑할 메모리 범위를 알려주고,
 * 커널이 DMA 버스 주소를 기록하여 돌려준다.
 */
struct nvm_ioctl_map
{
    uint64_t    vaddr_start;  /* 매핑할 가상 주소의 시작 위치 */
    size_t      n_pages;      /* 매핑할 페이지 수 */
    uint64_t*   ioaddrs;      /* 커널이 DMA 버스 주소를 기록할 사용자 공간 배열 포인터 */
};



/*
 * nvm_ioctl_type - 지원하는 ioctl 명령 코드를 정의하는 열거형.
 * _IOW 매크로는 (타입, 번호, 데이터크기)로 고유한 ioctl 명령 번호를 생성한다.
 * _IOW는 사용자→커널 방향의 쓰기 명령을 의미한다.
 */
enum nvm_ioctl_type
{
    /* 호스트(CPU) 메모리를 DMA용으로 매핑 요청 (명령번호 1) */
    NVM_MAP_HOST_MEMORY         = _IOW(NVM_IOCTL_TYPE, 1, struct nvm_ioctl_map),
#ifdef _CUDA
    /* CUDA GPU 디바이스 메모리를 DMA용으로 매핑 요청 (명령번호 2) */
    NVM_MAP_DEVICE_MEMORY       = _IOW(NVM_IOCTL_TYPE, 2, struct nvm_ioctl_map),
#endif
    /* 이전에 매핑된 메모리를 해제 요청 (명령번호 3), 가상 주소만 전달하면 된다 */
    NVM_UNMAP_MEMORY            = _IOW(NVM_IOCTL_TYPE, 3, uint64_t)
};


#endif /* __linux__ */
#endif /* __NVM_INTERNAL_LINUX_IOCTL_H__ */
