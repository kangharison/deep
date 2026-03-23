/* 이 파일은 Linux 전용으로 컴파일되어야 한다 */
#ifndef __linux__
#error "Must compile for Linux"
#endif

#include <nvm_types.h>
#include <nvm_ctrl.h>
#include <nvm_ctrl.h>
#include <nvm_util.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>  /* ioctl() 시스템콜을 사용하기 위한 헤더 */
#include <unistd.h>     /* close(), dup() 등 POSIX 함수 */
#include <fcntl.h>      /* fcntl() 파일 디스크립터 제어 */
#include <sys/mman.h>   /* mmap()/munmap() 메모리 매핑 */
#include <stdio.h>
#include "linux/map.h"
#include "linux/ioctl.h"
#include "lib_ctrl.h"
#include "dprintf.h"



/*
 * 디바이스 디스크립터 구조체.
 * 커널 모듈과 통신하기 위한 ioctl 파일 디스크립터를 보유한다.
 */
struct device
{
    int fd; /* ioctl 파일 디스크립터: 커널 모듈의 캐릭터 디바이스를 가리킨다 */
};



/*
 * release_device - 컨트롤러 메모리 매핑을 해제하고 파일 디스크립터를 닫는 정리 함수.
 * NVM 컨트롤러가 해제될 때 호출되며, mmap된 레지스터 영역을 해제하고 디바이스 파일을 닫는다.
 */
static void release_device(struct device* dev, volatile void* mm_ptr, size_t mm_size)
{
    munmap((void*) mm_ptr, mm_size);  /* mmap으로 매핑했던 NVMe 컨트롤러 레지스터 영역을 해제한다 */
    close(dev->fd);                   /* 커널 모듈 캐릭터 디바이스의 파일 디스크립터를 닫는다 */
    free(dev);                        /* 디바이스 구조체 자체의 메모리를 해제한다 */
}



/*
 * ioctl_map - 커널 모듈에 ioctl을 호출하여 사용자 공간 메모리를 DMA용으로 매핑한다.
 * 매핑 타입(호스트/CUDA)에 따라 적절한 ioctl 명령을 선택하고,
 * 커널이 반환하는 I/O 주소(버스 주소)를 ioaddrs 배열에 저장한다.
 */
static int ioctl_map(const struct device* dev, const struct va_range* va, uint64_t* ioaddrs)
{
    /* va_range를 감싸고 있는 ioctl_mapping 구조체를 container_of 매크로로 얻는다 */
    const struct ioctl_mapping* m = _nvm_container_of(va, struct ioctl_mapping, range);
    enum nvm_ioctl_type type;

    /* 매핑 타입에 따라 커널에 보낼 ioctl 명령 코드를 결정한다 */
    switch (m->type)
    {
        case MAP_TYPE_API:   /* API가 자체 할당한 메모리 (호스트 RAM) */
        case MAP_TYPE_HOST:  /* 사용자가 제공한 호스트 메모리 */
            type = NVM_MAP_HOST_MEMORY;  /* 호스트 메모리 매핑 ioctl 명령 */
            break;

#ifdef _CUDA
        case MAP_TYPE_CUDA:  /* CUDA GPU 디바이스 메모리 */
            type = NVM_MAP_DEVICE_MEMORY;  /* GPU 메모리 매핑 ioctl 명령 */
            break;
#endif
        default:
            dprintf("Unknown memory type in map for device");
            return EINVAL;  /* 알 수 없는 메모리 타입이면 유효하지 않은 인자 에러 반환 */
    }

    /* 커널에 전달할 ioctl 요청 구조체를 구성한다 */
    struct nvm_ioctl_map request = {
        .vaddr_start = (uintptr_t) m->buffer,  /* 매핑할 가상 주소 시작점 */
        .n_pages = va->n_pages,                 /* 매핑할 페이지 수 */
        .ioaddrs = ioaddrs                      /* 커널이 DMA 버스 주소를 기록할 배열 */
    };

    /* ioctl 시스템콜로 커널 모듈에 DMA 매핑을 요청한다 */
    int err = ioctl(dev->fd, type, &request);
    if (err < 0)
    {
        dprintf("Page mapping kernel request failed (ptr=%p, n_pages=%zu): %s\n",
                m->buffer, va->n_pages, strerror(errno));
        return errno;
    }

    return 0;
}



/*
 * ioctl_unmap - 커널 모듈에 ioctl을 호출하여 이전에 DMA 매핑된 메모리를 해제한다.
 * 버퍼의 가상 주소를 커널에 전달하면 커널이 해당 DMA 매핑을 제거한다.
 */
static void ioctl_unmap(const struct device* dev, const struct va_range* va)
{
    /* va_range를 감싸고 있는 ioctl_mapping 구조체를 얻는다 */
    const struct ioctl_mapping* m = _nvm_container_of(va, struct ioctl_mapping, range);
    uint64_t addr = (uintptr_t) m->buffer;  /* 해제할 버퍼의 가상 주소 */


    /* NVM_UNMAP_MEMORY ioctl로 커널에 DMA 매핑 해제를 요청한다 */
    int err = ioctl(dev->fd, NVM_UNMAP_MEMORY, &addr);
    if (err < 0)
    {
        dprintf("Page unmapping kernel request failed: %s\n", strerror(errno));
    }
}



/*
 * nvm_ctrl_init - NVM 컨트롤러를 초기화하는 공개 API 함수.
 * 커널 모듈의 캐릭터 디바이스 파일 디스크립터를 받아서:
 * 1) 디바이스 핸들을 생성하고
 * 2) NVMe 컨트롤러 레지스터를 mmap으로 사용자 공간에 매핑하고
 * 3) 내부 컨트롤러 구조체를 초기화한다
 */
int nvm_ctrl_init(nvm_ctrl_t** ctrl, int filedes)
{
    int err;
    struct device* dev;

    /* 디바이스 연산 콜백 함수 테이블을 설정한다 */
    const struct device_ops ops = {
        .release_device = &release_device,  /* 컨트롤러 해제 시 호출될 함수 */
        .map_range = &ioctl_map,            /* DMA 매핑 시 호출될 함수 */
        .unmap_range = &ioctl_unmap,        /* DMA 해제 시 호출될 함수 */
    };

    *ctrl = NULL;

    /* 디바이스 핸들 구조체를 힙에 할당한다 */
    dev = (struct device*) malloc(sizeof(struct device));
    if (dev == NULL)
    {
        dprintf("Failed to allocate device handle: %s\n", strerror(errno));
        return ENOMEM;
    }

    /* 전달받은 파일 디스크립터를 복제하여 독립적인 FD를 확보한다 (원본 FD가 닫혀도 안전하도록) */
    dev->fd = dup(filedes);
    if (dev->fd < 0)
    {
        free(dev);
        dprintf("Could not duplicate file descriptor: %s\n", strerror(errno));
        return errno;
    }

    /* 복제한 파일 디스크립터를 읽기/쓰기 모드로 설정한다 */
    err = fcntl(dev->fd, F_SETFD, O_RDWR);
    if (err == -1)
    {
        close(dev->fd);
        free(dev);
        dprintf("Failed to set file descriptor control: %s\n", strerror(errno));
        return errno;
    }

    /* NVMe 컨트롤러 레지스터(BAR0)를 사용자 공간에 mmap한다 */
    /* MAP_SHARED: 커널과 공유, MAP_LOCKED: 스왑 아웃 방지 */
    const size_t mm_size = NVM_CTRL_MEM_MINSIZE;  /* 컨트롤러 레지스터에 필요한 최소 매핑 크기 */
    void* mm_ptr = mmap(NULL, mm_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FILE|MAP_LOCKED, dev->fd, 0);
    if (mm_ptr == NULL)
    {
        close(dev->fd);
        free(dev);
        dprintf("Failed to map device memory: %s\n", strerror(errno));
        return errno;
    }

    /* 내부 컨트롤러 구조체를 초기화한다. DEVICE_TYPE_IOCTL은 커널 모듈 경유 방식임을 표시한다 */
    err = _nvm_ctrl_init(ctrl, dev, &ops, DEVICE_TYPE_IOCTL, mm_ptr, mm_size);
    if (err != 0)
    {
        release_device(dev, mm_ptr, mm_size);  /* 실패 시 할당된 모든 리소스를 정리한다 */
        return err;
    }

    return 0;
}
