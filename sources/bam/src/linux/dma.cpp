/* 이 파일은 Linux 전용으로 컴파일되어야 한다 */
#ifndef __linux__
#error "Must compile for Linux"
#endif

/* CUDA가 활성화된 경우 __CUDA__ 매크로도 정의한다 */
#ifdef _CUDA
#ifndef __CUDA__
#define __CUDA__
#endif
#endif

#include <nvm_types.h>
#include <nvm_util.h>
#include <nvm_dma.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "lib_util.h"
#include "lib_ctrl.h"
#include "dma.h"
#include "linux/map.h"
#include "dprintf.h"



/*
 * remove_mapping_descriptor - 매핑 디스크립터를 제거하고 관련 메모리를 해제한다.
 * MAP_TYPE_API 타입인 경우 API가 직접 할당한 버퍼이므로 버퍼도 함께 해제한다.
 * MAP_TYPE_HOST인 경우 사용자가 제공한 버퍼이므로 버퍼는 해제하지 않는다.
 */
static void remove_mapping_descriptor(struct ioctl_mapping* md)
{
    if (md->type == MAP_TYPE_API)
    {
        free((void*) md->buffer);  /* API가 posix_memalign으로 할당한 버퍼를 해제한다 */
    }

    free(md);  /* 매핑 디스크립터 구조체 자체를 해제한다 */
}



/*
 * release_mapping_descriptor - va_range 콜백으로 사용되는 래퍼 함수.
 * DMA 해제 시 호출되며, va_range에서 상위 ioctl_mapping 구조체를 얻어 제거한다.
 */
static void release_mapping_descriptor(struct va_range* va)
{
    /* container_of 패턴: va_range 멤버로부터 감싸고 있는 ioctl_mapping 구조체를 역추적한다 */
    remove_mapping_descriptor(_nvm_container_of(va, struct ioctl_mapping, range));
}



/*
 * create_mapping_descriptor - DMA 매핑에 필요한 ioctl_mapping 디스크립터를 생성한다.
 * 메모리 타입, 버퍼 주소, 크기 정보를 캡슐화하여 커널 모듈에 전달할 준비를 한다.
 */
static int create_mapping_descriptor(struct ioctl_mapping** handle, size_t page_size, enum mapping_type type, void* buffer, size_t size)
{
    /* 버퍼 크기를 페이지 크기로 정렬하여 필요한 페이지 수를 계산한다 */
    size_t n_pages = NVM_PAGE_ALIGN(size, page_size) / page_size;
    if (n_pages == 0)
    {
        return EINVAL;  /* 페이지 수가 0이면 유효하지 않은 인자 */
    }

    /* ioctl_mapping 구조체를 힙에 할당한다 */
    struct ioctl_mapping* md = (struct ioctl_mapping*) malloc(sizeof(struct ioctl_mapping));
    if (md == NULL)
    {
        dprintf("Failed to allocate mapping descriptor: %s\n", strerror(errno));
        return errno;
    }

    md->type = type;                        /* 메모리 타입 (API/HOST/CUDA) */
    md->buffer = buffer;                    /* 매핑할 원본 버퍼 주소 */
    md->range.remote = false;               /* 로컬 매핑임을 표시 (원격 아님) */
    md->range.vaddr = (volatile void*) buffer;  /* 가상 주소를 va_range에도 설정 */
    md->range.page_size = page_size;        /* 페이지 크기 (호스트: 4KB, GPU: 64KB) */
    md->range.n_pages = n_pages;            /* 매핑할 총 페이지 수 */

    *handle = md;
    return 0;
}



/*
 * nvm_dma_create - DMA 버퍼를 새로 할당하고 DMA 매핑을 생성하는 공개 API 함수.
 * 내부적으로 posix_memalign을 사용하여 페이지 정렬된 버퍼를 할당하고,
 * 커널 모듈을 통해 해당 버퍼에 대한 DMA 매핑을 설정한다.
 */
int nvm_dma_create(nvm_dma_t** handle, const nvm_ctrl_t* ctrl, size_t size)
{
    void* buffer;
    struct ioctl_mapping* md;

    /* 크기를 컨트롤러 페이지 크기의 배수로 올림 정렬한다 */
    size = NVM_CTRL_ALIGN(ctrl, size);
    if (size == 0)
    {
        return EINVAL;  /* 정렬 후 크기가 0이면 에러 */
    }

    *handle = NULL;

    /* ioctl 기반 디바이스 타입인지 확인한다 (SmartIO 등 다른 방식이면 사용 불가) */
    if (_nvm_ctrl_type(ctrl) != DEVICE_TYPE_IOCTL)
    {
        return EBADF;
    }

    /* 컨트롤러 페이지 크기에 정렬된 메모리를 할당한다 (DMA에 필요한 정렬 요구사항 충족) */
    int err = posix_memalign(&buffer, ctrl->page_size, size);
    if (err != 0)
    {
        dprintf("Failed to allocate page-aligned memory buffer: %s\n", strerror(err));
        return err;
    }

    /* MAP_TYPE_API: API가 직접 할당한 버퍼이므로 해제 시 버퍼도 함께 해제한다 */
    err = create_mapping_descriptor(&md, ctrl->page_size, MAP_TYPE_API, buffer, size);
    if (err != 0)
    {
        free(buffer);  /* 매핑 디스크립터 생성 실패 시 버퍼를 해제한다 */
        return err;
    }

    /* 내부 DMA 핸들을 초기화하고 커널에 DMA 매핑을 요청한다 */
    err = _nvm_dma_init(handle, ctrl, &md->range, &release_mapping_descriptor);
    if (err != 0)
    {
        remove_mapping_descriptor(md);  /* DMA 초기화 실패 시 디스크립터를 정리한다 */
        return err;
    }

    return 0;
}



/*
 * nvm_dma_map_host - 사용자가 제공한 호스트 메모리 버퍼를 DMA 매핑하는 공개 API 함수.
 * nvm_dma_create와 달리 버퍼를 직접 할당하지 않고 외부에서 전달받은 vaddr를 매핑한다.
 * 해제 시 버퍼 자체는 해제하지 않는다 (사용자 소유).
 */
int nvm_dma_map_host(nvm_dma_t** handle, const nvm_ctrl_t* ctrl, void* vaddr, size_t size)
{
    struct ioctl_mapping* md;
    *handle = NULL;

    /* 크기를 컨트롤러 페이지 크기의 배수로 올림 정렬한다 */
    size = NVM_CTRL_ALIGN(ctrl, size);
    if (size == 0)
    {
        return EINVAL;
    }

    /* ioctl 기반 디바이스 타입인지 확인한다 */
    if (_nvm_ctrl_type(ctrl) != DEVICE_TYPE_IOCTL)
    {
        return EBADF;
    }

    /* MAP_TYPE_HOST: 사용자 제공 버퍼이므로 해제 시 버퍼를 free하지 않는다 */
    int err = create_mapping_descriptor(&md, ctrl->page_size, MAP_TYPE_HOST, vaddr, size);
    if (err != 0)
    {
        return err;
    }

    /* 내부 DMA 핸들을 초기화하고 커널에 DMA 매핑을 요청한다 */
    err = _nvm_dma_init(handle, ctrl, &md->range, &release_mapping_descriptor);
    if (err != 0)
    {
        remove_mapping_descriptor(md);
        return err;
    }

    return 0;
}



#ifdef _CUDA
/*
 * nvm_dma_map_device - CUDA GPU 디바이스 메모리를 DMA 매핑하는 공개 API 함수.
 * GPU 메모리는 64KB (1<<16) 페이지 크기를 사용한다 (NVIDIA P2P API 요구사항).
 * GPU-Direct RDMA를 통해 NVMe 디바이스가 GPU 메모리에 직접 DMA를 수행할 수 있게 한다.
 */
int nvm_dma_map_device(nvm_dma_t** handle, const nvm_ctrl_t* ctrl, void* devptr, size_t size)
{
    struct ioctl_mapping* md;
    *handle = NULL;

    /* ioctl 기반 디바이스 타입인지 확인한다 */
    if (_nvm_ctrl_type(ctrl) != DEVICE_TYPE_IOCTL)
    {
        return EBADF;
    }

    /* GPU 페이지 크기는 64KB (1ULL << 16), NVIDIA P2P API가 요구하는 최소 정렬 단위이다 */
    int err = create_mapping_descriptor(&md, 1ULL << 16, MAP_TYPE_CUDA, devptr, size);
    if (err != 0)
    {
        return err;
    }

    /* 내부 DMA 핸들을 초기화하고 커널에 GPU 메모리 DMA 매핑을 요청한다 */
    err = _nvm_dma_init(handle, ctrl, &md->range, &release_mapping_descriptor);
    if (err != 0)
    {
        remove_mapping_descriptor(md);
        return err;
    }

    return 0;
}
#endif
