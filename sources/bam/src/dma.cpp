/* dma.cpp - DMA 매핑 관리 구현 파일
 *
 * NVMe 컨트롤러와 호스트 간의 DMA(Direct Memory Access) 전송을 위한 메모리 매핑을 관리한다.
 * 가상 주소 범위를 버스(물리) 주소로 변환하고, 참조 카운팅을 통해 매핑 수명을 관리한다.
 * DMA 핸들은 컨트롤러 페이지 크기 단위의 I/O 주소 배열을 유지한다.
 */
#ifdef _CUDA
#ifndef __CUDA__
#define __CUDA__
#endif
#endif

#include <nvm_types.h>    // NVMe 타입 정의
#include <nvm_dma.h>      // DMA 공개 API
#include <nvm_util.h>     // 유틸리티 매크로
#include <stddef.h>       // 표준 정의
#include <stdint.h>       // 정수형 타입
#include <stdbool.h>      // bool 타입
#include <stdlib.h>       // malloc, free, calloc
#include <string.h>       // memset 등
#include <errno.h>        // POSIX 에러 코드
#include "lib_util.h"     // 내부 유틸리티 (_nvm_container_of 등)
#include "lib_ctrl.h"     // 내부 컨트롤러 구조체
#include "dma.h"          // DMA 내부 타입 정의 (va_range 등)
#include "mutex.h"        // 뮤텍스 래퍼
#include "dprintf.h"      // 디버그 출력


// 가상 주소 범위 언매핑 콜백 타입
typedef void (*va_unmap_t)(const struct device*, const struct va_range*);

/* 참조 카운트 방식의 매핑 디스크립터.
 * 하나의 가상 주소 범위에 대해 여러 DMA 핸들이 공유할 수 있도록 참조 카운팅한다.
 */
struct map
{
    struct mutex        lock;   // 참조 카운트 보호를 위한 뮤텍스
    uint32_t            count;  // 참조 카운트
    struct controller*  ctrl;   // 컨트롤러 참조 (디바이스 콜백 접근용)
    struct va_range*    va;     // 가상 주소 범위 디스크립터

    va_range_free_t     release;// 주소 범위 해제 콜백 (va_range 메모리 반환용)
    va_unmap_t          unmap;// 디바이스 언매핑 콜백 (IOMMU 매핑 해제용)
};



/* 내부 DMA 핸들 컨테이너.
 * nvm_dma_t 핸들 뒤에 가변 크기의 ioaddrs 배열이 이어지므로 구조체 크기가 가변적이다.
 * 32바이트 정렬로 캐시라인 최적화를 한다.
 */
struct __attribute__((aligned (32))) container
{
    struct map*         map;    // 매핑 디스크립터 포인터
    nvm_dma_t           handle; // 사용자에게 노출되는 DMA 핸들 (ioaddrs 배열 포함)
};



/* 컨트롤러 페이지 단위로 페이지 수를 변환하는 매크로.
 * 호스트 페이지 크기와 컨트롤러 페이지 크기가 다를 수 있으므로 변환이 필요하다.
 */
#define n_ctrl_pages(ctrl, page_size, n_pages) \
    (((page_size) * (n_pages)) / (ctrl)->page_size)



/* create_map - 참조 카운트 방식의 매핑 디스크립터를 생성한다.
 * 컨트롤러 참조를 증가시키고, 가상 주소 범위와 해제 콜백을 저장한다.
 */
static int create_map(struct map** md, const nvm_ctrl_t* ctrl, struct va_range* va, va_range_free_t release)
{
    *md = NULL;

#ifndef NDEBUG
    // 디버그 빌드에서 release 콜백이 없으면 에러
    if (release == NULL)
    {
        return EINVAL;
    }
#endif

    // Take device reference
    // 컨트롤러 참조 카운트 증가 (DMA 매핑이 유효한 동안 컨트롤러가 해제되지 않도록)
    struct controller* ref = _nvm_ctrl_get(ctrl);
    if (ref == NULL)
    {
        return ENOSPC;
    }

    // map 구조체 할당
    struct map* m = (struct map*) malloc(sizeof(struct map));
    if (m == NULL)
    {
        _nvm_ctrl_put(ref);
        dprintf("Failed to allocate mapping descriptor: %s\n", strerror(errno));
        return errno;
    }

    // 뮤텍스 초기화 (참조 카운트 보호용)
    int err = _nvm_mutex_init(&m->lock);
    if (err != 0)
    {
        free(m);
        _nvm_ctrl_put(ref);
        return err;
    }

    // 초기 참조 카운트 = 1
    m->count = 1;
    // 컨트롤러 참조 저장
    m->ctrl = ref;
    // 가상 주소 범위 저장
    m->va = va;
    // 주소 범위 해제 콜백 저장
    m->release = release;
    // 언매핑 콜백은 아직 설정하지 않음 (dma_map에서 설정)
    m->unmap = NULL;

    *md = m;
    return 0;
}



/* remove_map - 매핑 디스크립터를 해제한다.
 * 컨트롤러 참조 카운트를 감소시키고 메모리를 반환한다.
 */
static void remove_map(struct map* md)
{
    // 컨트롤러 참조 감소
    _nvm_ctrl_put(md->ctrl);
    // 매핑 디스크립터 메모리 해제
    free(md);
}



/* populate_handle - DMA 핸들의 멤버를 초기화하고 버스 주소 리스트를 채운다.
 * 호스트 페이지 크기의 ioaddrs를 컨트롤러 페이지 크기 단위로 변환하여 핸들에 저장한다.
 * 연속성(contiguous) 여부도 검사한다.
 */
static void populate_handle(nvm_dma_t* handle, const struct va_range* va, const nvm_ctrl_t* ctrl, const uint64_t* ioaddrs)
{
    size_t i_page;
    size_t page_size = va->page_size;

    // Set handle members
    // 핸들 기본 멤버 설정
    handle->vaddr = (void*) va->vaddr;                                    // 가상 주소
    handle->page_size = ctrl->page_size;                                  // 컨트롤러 페이지 크기
    handle->n_ioaddrs = n_ctrl_pages(ctrl, page_size, va->n_pages);       // 컨트롤러 페이지 수

    // Calculate logical page addresses
    // 컨트롤러 페이지 단위로 각 I/O 주소를 계산한다
    handle->contiguous = true;  // 연속 메모리로 가정하고 시작
    for (i_page = 0; i_page < handle->n_ioaddrs; ++i_page)
    {
        // 컨트롤러 페이지가 호스트 페이지의 어디에 위치하는지 계산
        size_t current_page = (i_page * handle->page_size) / page_size;          // 호스트 페이지 인덱스
        size_t offset_within_page = (i_page * handle->page_size) % page_size;    // 호스트 페이지 내 오프셋

        // 호스트 페이지의 버스 주소 + 오프셋 = 컨트롤러 페이지의 버스 주소
        handle->ioaddrs[i_page] = ioaddrs[current_page] + offset_within_page;

        // 이전 페이지와 연속인지 검사 (비연속이면 contiguous=false)
        if (i_page > 0 && handle->ioaddrs[i_page - 1] + handle->page_size != handle->ioaddrs[i_page])
        {
            handle->contiguous = false;
        }
    }
}



/* dma_map - 가상 주소 범위를 디바이스용으로 매핑하고 DMA 핸들을 채운다.
 * 디바이스 드라이버의 map_range 콜백을 호출하여 IOMMU/DMA 매핑을 수행한다.
 */
static int dma_map(struct container* container)
{
    struct map* md = container->map;

    // Check if mapping is supported
    // 디바이스가 매핑 기능을 지원하는지 확인
    if (md->ctrl->ops.map_range == NULL)
    {
        return EINVAL;
    }

    // Allocate list of bus addresses
    // 호스트 페이지 수만큼 버스 주소 배열 할당
    uint64_t* ioaddrs = (uint64_t*) calloc(md->va->n_pages, sizeof(uint64_t));
    if (ioaddrs == NULL)
    {
        return ENOMEM;
    }

    // 디바이스 드라이버의 map_range 콜백을 호출하여 가상→버스 주소 변환
    int err = md->ctrl->ops.map_range(md->ctrl->device, md->va, ioaddrs);
    if (err != 0)
    {
        free(ioaddrs);
        return err;
    }
    // 매핑 성공 시 언매핑 콜백 저장 (나중에 해제할 때 사용)
    md->unmap = md->ctrl->ops.unmap_range;

    // 변환된 버스 주소로 DMA 핸들을 채운다
    populate_handle(&container->handle, md->va, &md->ctrl->handle, ioaddrs);
    // 임시 버스 주소 배열 해제 (핸들의 ioaddrs에 이미 복사됨)
    free(ioaddrs);

    return 0;
}



/* put_map - 매핑 디스크립터의 참조 카운트를 감소시킨다.
 * 참조 카운트가 0이 되면 디바이스 언매핑과 주소 범위 해제를 수행한다.
 */
static void put_map(struct map* md)
{
    if (md == NULL)
    {
        return;
    }

    _nvm_mutex_lock(&md->lock);
    if (--md->count == 0)
    {
        // 참조 카운트가 0이면 디바이스 언매핑 수행
        if (md->unmap != NULL)
        {
            md->unmap(md->ctrl->device, md->va);
        }
        // 가상 주소 범위 해제 콜백 호출
        md->release(md->va);
        // va를 NULL로 설정하여 삭제 대상임을 표시
        md->va = NULL;
    }
    _nvm_mutex_unlock(&md->lock);

    // 뮤텍스 해제 후 매핑 디스크립터 삭제 (va==NULL이면 마지막 참조였음)
    if (md->va == NULL)
    {
        remove_map(md);
    }
}



/* get_map - 매핑 디스크립터의 참조 카운트를 증가시킨다.
 * DMA 핸들을 복제(remap)할 때 호출한다.
 */
static int get_map(struct map* md)
{
    int err;

    if (md == NULL)
    {
        return EINVAL;
    }

    // 뮤텍스를 잡고 참조 카운트를 안전하게 증가
    err = _nvm_mutex_lock(&md->lock);
    if (err != 0)
    {
        dprintf("Failed to take map reference lock: %s\n", strerror(err));
        return err;
    }

    ++md->count;
    _nvm_mutex_unlock(&md->lock);

    return 0;
}



/* create_container - DMA 핸들 컨테이너를 할당한다.
 * 컨트롤러 페이지 수에 따라 가변 크기의 ioaddrs 배열을 포함하는 컨테이너를 생성한다.
 * 이 함수는 디바이스 참조가 이미 증가된 상태에서 호출되어야 한다.
 */
static int create_container(struct container** container, struct map* md)
{
    *container = NULL;
    const struct controller* ctrl = md->ctrl;

    size_t page_size = md->va->page_size;
    size_t n_pages = md->va->n_pages;

    // Do some sanity checking
    // 페이지 크기와 개수가 유효하고, 컨트롤러 페이지 크기로 나누어 떨어지는지 검증
    if (page_size == 0 || n_pages == 0 || (page_size * n_pages) % ctrl->handle.page_size != 0)
    {
        dprintf("Addresses do not align with controller pages\n");
        return EINVAL;
    }

    // Size of the handle container
    // 컨테이너 크기 = 고정 부분 + ioaddrs 배열 (컨트롤러 페이지 수만큼)
    size_t container_size = sizeof(struct container) + (n_ctrl_pages(&ctrl->handle, page_size, n_pages)) * sizeof(uint64_t);

    // Allocate the container and set mapping
    // 컨테이너 동적 할당
    *container = (struct container*) malloc(container_size);
    if (*container == NULL)
    {
        dprintf("Failed to allocate DMA descriptor: %s\n", strerror(errno));
        return ENOMEM;
    }

    // 매핑 디스크립터 연결
    (*container)->map = md;
    // 연속 메모리 여부 (나중에 populate_handle에서 업데이트됨)
    (*container)->handle.contiguous = true;
    // 로컬 메모리 여부 (remote의 반대)
    (*container)->handle.local = !md->va->remote;

    return 0;
}



/* remove_container - DMA 컨테이너를 해제한다.
 * 매핑 디스크립터의 참조 카운트를 감소시키고 컨테이너 메모리를 반환한다.
 * 주의: 디바이스 언매핑은 put_map 내부에서 처리된다.
 */
static void remove_container(struct container* container)
{
    // 매핑 디스크립터 참조 감소 (0이면 언매핑+해제)
    put_map(container->map);
    container->map = NULL;
    // 컨테이너 메모리 해제
    free(container);
}



/* _nvm_dma_init - DMA 핸들을 생성하고, 가상 주소 범위를 컨트롤러용으로 매핑한다.
 * 매핑 디스크립터 생성 → 컨테이너 생성 → 디바이스 매핑의 3단계로 진행한다.
 */
int _nvm_dma_init(nvm_dma_t** handle, const nvm_ctrl_t* ctrl, struct va_range* va, va_range_free_t release)
{
    *handle = NULL;
    struct map* map;
    struct container* container;

    // 해제 콜백 필수
    if (release == NULL)
    {
        return EINVAL;
    }

    // Create mapping descriptor
    // 매핑 디스크립터 생성 (참조 카운트=1, 컨트롤러 참조 증가)
    int err = create_map(&map, ctrl, va, release);
    if (err != 0)
    {
        return err;
    }

    // Create DMA handle container
    // DMA 핸들 컨테이너 생성 (ioaddrs 배열 포함)
    err = create_container(&container, map);
    if (err != 0)
    {
        remove_map(map);
        return err;
    }

    // Map controller for device and populate handle
    // 디바이스 드라이버를 통해 실제 DMA 매핑 수행 및 핸들 채우기
    err = dma_map(container);
    if (err != 0)
    {
        remove_map(map);
        free(container);
        return err;
    }

    *handle = &container->handle;
    return 0;
}



/* nvm_dma_map - 사용자가 제공한 물리/버스 주소로 DMA 매핑 디스크립터를 생성한다.
 * 디바이스 드라이버의 map_range 콜백을 거치지 않고, 직접 제공된 ioaddrs를 사용한다.
 * 사용자가 이미 물리 주소를 알고 있을 때 (예: GPU 메모리) 유용하다.
 */
int nvm_dma_map(nvm_dma_t** handle, const nvm_ctrl_t* ctrl, void* vaddr, size_t page_size, size_t n_pages, const uint64_t* ioaddrs)
{
    int status;
    struct map* map;
    struct container* container;
    struct va_range* va;

    *handle = NULL;

    // ioaddrs 배열이 반드시 필요하다
    if (ioaddrs == NULL)
    {
        return EINVAL;
    }

    // Create virtual address range descriptor
    // 가상 주소 범위 디스크립터를 동적 할당
    va = (struct va_range*) malloc(sizeof(struct va_range));
    if (va == NULL)
    {
        dprintf("Failed to allocate mapping descriptor: %s\n", strerror(errno));
        return ENOMEM;
    }

    // 가상 주소 범위 멤버 설정
    va->remote = false;                    // 로컬 메모리
    va->vaddr = (volatile void*) vaddr;    // 가상 주소
    va->page_size = page_size;             // 페이지 크기
    va->n_pages = n_pages;                 // 페이지 수

    // Create empty mapping descriptor
    // 매핑 디스크립터 생성 (해제 콜백으로 표준 free 사용)
    status = create_map(&map, ctrl, va, (va_range_free_t) &free);
    if (status != 0)
    {
        free(va);
        return status;
    }

    // Create DMA handle container
    // DMA 핸들 컨테이너 생성
    status = create_container(&container, map);
    if (status != 0)
    {
        remove_map(map);
        free(va);
        return status;
    }

    // 디바이스 매핑 없이 사용자 제공 ioaddrs로 직접 핸들을 채운다
    populate_handle(&container->handle, va, ctrl, ioaddrs);
    *handle = &container->handle;
    return 0;
}



/* nvm_dma_remap - 기존 DMA 핸들에서 새로운 DMA 핸들을 복제한다.
 * 같은 물리 메모리에 대해 독립적인 DMA 핸들을 생성하되, 매핑 디스크립터는 공유한다.
 * 디바이스 매핑을 다시 수행하지 않고 참조 카운트만 증가시킨다.
 */
int nvm_dma_remap(nvm_dma_t** handle, const nvm_dma_t* other)
{
    int status;
    struct container* container;
    // 원본 핸들에서 매핑 디스크립터를 추출
    struct map* map = _nvm_container_of(other, struct container, handle)->map;
    struct va_range va;

    *handle = NULL;

    // Increase mapping descriptor reference count
    // 매핑 디스크립터의 참조 카운트를 증가시킨다 (공유)
    status = get_map(map);
    if (status != 0)
    {
        return status;
    }

    // Create DMA handle container
    // 새 DMA 핸들 컨테이너 생성
    status = create_container(&container, map);
    if (status != 0)
    {
        put_map(map); // Will implicitly release device reference  // 실패 시 참조 감소 (디바이스 참조도 암묵적으로 해제)
        return status;
    }

    // Hack to get list of bus addresses, since we don't want to
    // actually call map again, simply increase reference count.
    // 실제 매핑을 다시 하지 않고, 원본의 ioaddrs를 사용하여 핸들을 채운다
    va.remote = !other->local;
    va.vaddr = map->va->vaddr;
    va.page_size = other->page_size;
    va.n_pages = other->n_ioaddrs;

    // 원본의 버스 주소 배열로 새 핸들을 채운다
    populate_handle(&container->handle, &va, &map->ctrl->handle, other->ioaddrs);
    *handle = &container->handle;

    return 0;
}



/* nvm_dma_unmap - DMA 매핑을 해제하고 핸들을 삭제한다.
 * 매핑 디스크립터의 참조 카운트가 0이 되면 실제 디바이스 언매핑도 수행된다.
 */
void nvm_dma_unmap(nvm_dma_t* handle)
{
    if (handle != NULL)
    {
        // container_of로 핸들에서 컨테이너를 역추적
        struct container* dma = _nvm_container_of(handle, struct container, handle);
        // 컨테이너 해제 (내부에서 put_map으로 매핑 디스크립터 참조 감소)
        remove_container(dma);
    }
}



/* nvm_ctrl_from_dma - DMA 핸들에서 컨트롤러 핸들을 추출한다.
 * DMA 매핑이 연결된 NVMe 컨트롤러의 핸들 포인터를 반환한다.
 */
const nvm_ctrl_t* nvm_ctrl_from_dma(const nvm_dma_t* handle)
{
    if (handle != NULL)
    {
        const struct container* dma;
        // container_of로 컨테이너를 역추적하고, 매핑→컨트롤러→핸들 경로로 접근
        dma = _nvm_container_of(handle, struct container, handle);
        return &dma->map->ctrl->handle;
    }

    return NULL;
}



/* _nvm_dma_va - DMA 핸들에서 내부 가상 주소 범위 디스크립터를 추출한다.
 * 내부 함수로, 가상 주소 범위의 세부 정보(페이지 크기, 페이지 수 등)에 접근할 때 사용한다.
 */
const struct va_range* _nvm_dma_va(const nvm_dma_t* handle)
{
    if (handle != NULL)
    {
        // container_of로 컨테이너를 역추적
        const struct container* dma = _nvm_container_of(handle, struct container, handle);

        // 매핑 디스크립터가 존재하면 가상 주소 범위 반환
        if (dma->map != NULL)
        {
            return dma->map->va;
        }
    }

    return NULL;
}
