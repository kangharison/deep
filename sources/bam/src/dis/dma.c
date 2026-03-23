/* dma.c - DIS/SISCI 기반 DMA 메모리 매핑 관리 구현
 *
 * 이 파일은 DIS 패브릭을 통해 NVMe 디바이스와 호스트/GPU/원격 노드 간의
 * DMA 전송을 위한 메모리 세그먼트 생성, 가상 주소 매핑, DMA 핸들 초기화를 구현한다.
 *
 * 세 가지 메모리 유형을 지원한다:
 * 1. 로컬 세그먼트: 호스트 메모리를 SISCI 세그먼트로 등록 (NVMe → 호스트 DMA)
 * 2. 원격 세그먼트: 디바이스 메모리를 연결하여 사용 (NVMe CMB 등)
 * 3. GPU 세그먼트: CUDA 디바이스 메모리를 SISCI 세그먼트로 등록 (GPUDirect)
 */

/* _SISCI 매크로가 정의되지 않으면 컴파일을 중단한다 */
#ifndef _SISCI
#error "Must compile with SISCI support"
#endif

/* DIS 클러스터 모드 활성화 */
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif

#include <nvm_types.h>      /* NVM 공통 타입 정의 */
#include <nvm_util.h>       /* NVM_CTRL_ALIGN, NVM_PAGE_ALIGN 등 유틸리티 매크로 */
#include <nvm_dma.h>        /* nvm_dma_t DMA 핸들, nvm_ctrl_from_dma 등 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "dma.h"            /* struct va_range, _nvm_dma_init 등 내부 DMA 함수 */
#include "ctrl.h"           /* struct controller, _nvm_ctrl_get/_nvm_ctrl_put 등 */
#include "dis/device.h"     /* struct device, 세그먼트 생성/연결 함수 */
#include "dis/map.h"        /* struct local_segment, struct remote_segment, struct va_map */
#include "dprintf.h"        /* 디버그 출력 매크로 */
#include <sisci_types.h>    /* SISCI 타입 정의 */
#include <sisci_error.h>    /* SISCI 에러 코드 */
#include <sisci_api.h>      /* SISCI API 함수 선언 */



/*
 * va_map_local - 로컬 세그먼트를 프로세스의 가상 주소 공간에 매핑한다.
 *
 * SCIMapLocalSegment()를 호출하여 로컬 SISCI 세그먼트를 이 프로세스에서
 * 읽고 쓸 수 있는 가상 주소로 매핑한다. NVMe 커맨드의 데이터 버퍼로 사용할
 * 호스트 메모리에 대한 CPU 접근을 가능하게 한다.
 *
 * @m:       매핑 상태를 저장할 va_map 구조체
 * @va:      가상 주소 범위 정보를 설정할 va_range 구조체
 * @segment: 매핑할 SISCI 로컬 세그먼트 핸들
 * @write:   true이면 읽기/쓰기, false이면 읽기 전용 매핑
 * @return:  성공 시 0, 실패 시 적절한 errno 값
 */
static int va_map_local(struct va_map* m, struct va_range* va, sci_local_segment_t segment, bool write)
{
    sci_error_t err;
    unsigned int flags = 0;
    void* ptr = NULL;
    size_t size = 0;

    /* 읽기 전용 매핑이면 SCI_FLAG_READONLY_MAP 플래그를 설정한다 */
    flags = !write ? SCI_FLAG_READONLY_MAP : 0;

    /* 세그먼트 크기를 조회한다 */
    size = SCIGetLocalSegmentSize(segment);
    /* SCIMapLocalSegment: 로컬 SISCI 세그먼트를 프로세스의 가상 주소 공간에 매핑한다.
     * 반환된 ptr을 통해 CPU가 이 메모리에 직접 읽기/쓰기할 수 있다.
     * m->md에 매핑 디스크립터가 저장되며, 나중에 SCIUnmapSegment()에 사용된다. */
    ptr = SCIMapLocalSegment(segment, &m->md, 0, size, NULL, flags, &err);

    switch (err)
    {
        case SCI_ERR_OK:
            m->mapped = true;            /* 매핑 성공 상태 기록 */
            va->remote = false;          /* 로컬 세그먼트임을 표시 */
            va->n_pages = 1;             /* SISCI 세그먼트는 연속 메모리이므로 1개 페이지로 취급 */
            va->page_size = size;        /* 페이지 크기 = 세그먼트 전체 크기 */
            va->vaddr = (volatile void*) ptr;  /* 매핑된 가상 주소 저장 */
            return 0;

        case SCI_ERR_FLAG_NOT_IMPLEMENTED:   /* 지원되지 않는 플래그 */
        case SCI_ERR_ILLEGAL_FLAG:           /* 잘못된 플래그 */
        case SCI_ERR_OUT_OF_RANGE:           /* 범위 초과 */
        case SCI_ERR_SIZE_ALIGNMENT:         /* 크기 정렬 오류 */
        case SCI_ERR_OFFSET_ALIGNMENT:       /* 오프셋 정렬 오류 */
            return EINVAL;

        default:
            dprintf("Mapping local segment into virtual address space failed: %s\n", _SCIGetErrorString(err));
            return EIO;
    }
}



/*
 * va_map_remote - 원격 세그먼트를 프로세스의 가상 주소 공간에 매핑한다.
 *
 * SCIMapRemoteSegment()를 호출하여 원격 노드 또는 디바이스의 메모리를
 * 이 프로세스에서 접근 가능한 가상 주소로 매핑한다.
 *
 * @m:       매핑 상태를 저장할 va_map 구조체
 * @va:      가상 주소 범위 정보를 설정할 va_range 구조체
 * @segment: 매핑할 SISCI 원격 세그먼트 핸들
 * @write:   true이면 읽기/쓰기, false이면 읽기 전용 매핑
 * @wc:      true이면 Write-Combining 매핑, false이면 I/O Space(uncacheable) 매핑
 * @return:  성공 시 0, 실패 시 적절한 errno 값
 */
static int va_map_remote(struct va_map* m, struct va_range* va, sci_remote_segment_t segment, bool write, bool wc)
{
    sci_error_t err;
    unsigned int flags = 0;
    volatile void* ptr = NULL;
    size_t size = 0;

    /* 매핑 플래그 설정: 읽기 전용 여부와 WC/IO 매핑 모드 */
    flags |= !write ? SCI_FLAG_READONLY_MAP : 0;
    /* SCI_FLAG_IO_MAP_IOSPACE: uncacheable I/O 매핑 (레지스터 접근용)
     * WC가 true이면 이 플래그를 설정하지 않아 Write-Combining 매핑이 된다.
     * WC 매핑은 데이터 전송 시 성능이 좋지만, 레지스터 접근에는 부적합하다. */
    flags |= !wc ? SCI_FLAG_IO_MAP_IOSPACE : 0;

    /* 원격 세그먼트 크기를 조회한다 */
    size = SCIGetRemoteSegmentSize(segment);
    /* SCIMapRemoteSegment: 원격 SISCI 세그먼트를 프로세스의 가상 주소 공간에 매핑한다.
     * PCIe 패브릭을 통해 원격 메모리에 대한 MMIO 접근이 가능해진다.
     * volatile 포인터를 사용하여 컴파일러 최적화로 인한 접근 순서 변경을 방지한다. */
    ptr = SCIMapRemoteSegment(segment, &m->md, 0, size, NULL, flags, &err);

    switch (err)
    {
        case SCI_ERR_OK:
            m->mapped = true;            /* 매핑 성공 상태 기록 */
            va->remote = true;           /* 원격 세그먼트임을 표시 */
            va->n_pages = 1;             /* SISCI 세그먼트는 연속 메모리이므로 1개 페이지로 취급 */
            va->page_size = size;        /* 페이지 크기 = 세그먼트 전체 크기 */
            va->vaddr = ptr;             /* 매핑된 가상 주소 저장 */
            return 0;

        case SCI_ERR_FLAG_NOT_IMPLEMENTED:
        case SCI_ERR_ILLEGAL_FLAG:
        case SCI_ERR_OUT_OF_RANGE:
        case SCI_ERR_SIZE_ALIGNMENT:
        case SCI_ERR_OFFSET_ALIGNMENT:
            return EINVAL;

        default:
            dprintf("Mapping local segment into virtual address space failed: %s\n", _SCIGetErrorString(err));
            return EIO;
    }
}



/*
 * va_unmap - SISCI 세그먼트의 가상 주소 매핑을 해제한다.
 *
 * SCIUnmapSegment()를 호출하여 이전에 SCIMapLocalSegment() 또는
 * SCIMapRemoteSegment()로 매핑한 가상 주소를 해제한다.
 *
 * @m: 매핑 상태를 관리하는 va_map 구조체
 */
static void va_unmap(struct va_map* m)
{
    if (m->mapped)
    {
        sci_error_t err;

        /* SCIUnmapSegment: 세그먼트의 가상 주소 매핑을 해제한다.
         * BUSY이면 아직 접근이 진행 중이므로 재시도한다. */
        do
        {
            SCIUnmapSegment(m->md, 0, &err);
        }
        while (err == SCI_ERR_BUSY);

        m->mapped = false;               /* 매핑 해제 상태로 전환 */

#ifndef NDEBUG
        if (err != SCI_ERR_OK)
        {
            dprintf("Failed to unmap segment: %s\n", _SCIGetErrorString(err));
        }
#endif
    }
}



/*
 * create_local_desc - 로컬 세그먼트 디스크립터를 생성하는 헬퍼 함수
 *
 * 이미 생성된 SISCI 로컬 세그먼트에 대한 메타데이터 구조체(local_segment)를
 * 할당하고 초기화한다. DMA 시스템에서 사용하는 va_range도 함께 설정된다.
 *
 * @ls:      생성된 디스크립터를 받을 이중 포인터
 * @ctrl:    NVM 컨트롤러 참조 (참조 카운트 관리)
 * @adapter: DIS 어댑터 번호
 * @segment: SISCI 로컬 세그먼트 핸들
 * @return:  성공 시 0, 실패 시 errno 값
 */
static int create_local_desc(struct local_segment** ls, struct controller* ctrl, uint32_t adapter, sci_local_segment_t segment)
{
    struct local_segment* s;

    /* 로컬 세그먼트 디스크립터를 힙에 할당한다 */
    s = (struct local_segment*) malloc(sizeof(struct local_segment));
    if (s == NULL)
    {
        dprintf("Failed to allocate local segment descriptor: %s\n", strerror(errno));
        return errno;
    }

    s->ctrl = ctrl;                      /* 컨트롤러 참조 저장 */
    s->adapter = adapter;                /* 어댑터 번호 저장 */
    s->segment = segment;                /* SISCI 세그먼트 핸들 저장 */
    s->remove = false;                   /* 기본적으로 세그먼트 제거 불필요 (외부에서 생성된 세그먼트) */
    s->map.mapped = false;               /* 아직 가상 주소에 매핑되지 않음 */
    s->map.md = NULL;                    /* 매핑 디스크립터 초기화 */
    /* VA_RANGE_INIT: va_range 구조체를 초기화한다.
     * remote=false(로컬), vaddr=NULL(아직 미매핑), page_size=세그먼트 크기, n_pages=1 */
    s->range = VA_RANGE_INIT(false, NULL, SCIGetLocalSegmentSize(s->segment), 1);

    *ls = s;
    return 0;
}



/*
 * remove_local_desc - 로컬 세그먼트 디스크립터를 제거하는 헬퍼 함수
 *
 * 가상 주소 매핑을 해제하고, 이 API가 세그먼트를 생성했다면(remove=true)
 * 세그먼트 자체도 제거한다. 마지막으로 컨트롤러 참조를 해제한다.
 *
 * @ls: 제거할 로컬 세그먼트 디스크립터
 */
static void remove_local_desc(struct local_segment* ls)
{
    if (ls == NULL)
    {
        return;
    }

    /* 가상 주소 매핑이 있으면 해제한다 */
    va_unmap(&ls->map);

    /* 이 API가 세그먼트를 생성한 경우에만 세그먼트 정리를 수행한다 */
    if (ls->remove)
    {
        sci_error_t err;

        /* SCISetSegmentUnavailable: 세그먼트를 비공개 상태로 전환한다.
         * 원격 노드/디바이스가 더 이상 접근할 수 없게 된다. */
        do
        {
            SCISetSegmentUnavailable(ls->segment, ls->adapter, 0, &err);
        }
        while (err == SCI_ERR_BUSY);

#ifndef NDEBUG
        if (err != SCI_ERR_OK)
        {
            dprintf("Failed to set local segment unavailable on adapter %u: %s\n",
                    ls->adapter, _SCIGetErrorString(err));
        }
#endif

        /* 로컬 세그먼트를 완전히 제거한다 (SCIRemoveSegment 내부 호출) */
        _nvm_local_memory_put(&ls->segment);
    }

    /* 컨트롤러 참조 카운트를 감소시킨다 */
    _nvm_ctrl_put(ls->ctrl);
    /* 디스크립터 메모리 해제 */
    free(ls);
}



/*
 * create_remote_desc - 원격 세그먼트 디스크립터를 생성하는 헬퍼 함수
 *
 * 이미 연결된 SISCI 원격 세그먼트에 대한 메타데이터 구조체(remote_segment)를
 * 할당하고 초기화한다.
 *
 * @rs:      생성된 디스크립터를 받을 이중 포인터
 * @ctrl:    NVM 컨트롤러 참조
 * @segment: SISCI 원격 세그먼트 핸들
 * @return:  성공 시 0, 실패 시 errno 값
 */
static int create_remote_desc(struct remote_segment** rs, struct controller* ctrl, sci_remote_segment_t segment)
{
    struct remote_segment* s;

    /* 원격 세그먼트 디스크립터를 힙에 할당한다 */
    s = (struct remote_segment*) malloc(sizeof(struct remote_segment));
    if (s == NULL)
    {
        dprintf("Failed to allocate remote segment descriptor: %s\n", strerror(errno));
        return errno;
    }

    s->ctrl = ctrl;                      /* 컨트롤러 참조 저장 */
    s->segment = segment;                /* SISCI 원격 세그먼트 핸들 저장 */
    s->disconnect = false;               /* 기본적으로 세그먼트 연결 해제 불필요 (외부에서 연결된 세그먼트) */
    s->map.mapped = false;               /* 아직 가상 주소에 매핑되지 않음 */
    s->map.md = NULL;
    /* VA_RANGE_INIT: remote=true(원격), vaddr=NULL, page_size=세그먼트 크기, n_pages=1 */
    s->range = VA_RANGE_INIT(true, NULL, SCIGetRemoteSegmentSize(s->segment), 1);

    *rs = s;

    return 0;
}



/*
 * remove_remote_desc - 원격 세그먼트 디스크립터를 제거하는 헬퍼 함수
 *
 * 가상 주소 매핑을 해제하고, 이 API가 세그먼트를 연결했다면(disconnect=true)
 * 세그먼트 연결도 해제한다. 마지막으로 컨트롤러 참조를 해제한다.
 *
 * @rs: 제거할 원격 세그먼트 디스크립터
 */
static void remove_remote_desc(struct remote_segment* rs)
{
    if (rs == NULL)
    {
        return;
    }

    /* 가상 주소 매핑이 있으면 해제한다 */
    va_unmap(&rs->map);

    /* 이 API가 세그먼트를 연결한 경우에만 연결 해제를 수행한다 */
    if (rs->disconnect)
    {
        _nvm_device_memory_put(&rs->segment);  /* SCIDisconnectSegment 내부 호출 */
    }

    /* 컨트롤러 참조 카운트를 감소시킨다 */
    _nvm_ctrl_put(rs->ctrl);
    /* 디스크립터 메모리 해제 */
    free(rs);
}



/*
 * release_range - va_range의 타입에 따라 적절한 해제 함수를 디스패치한다.
 *
 * DMA 핸들 해제 시 호출되는 콜백으로, va_range가 로컬 세그먼트인지
 * 원격 세그먼트인지에 따라 remove_local_desc() 또는 remove_remote_desc()를 호출한다.
 * _nvm_container_of 매크로로 va_range에서 감싸고 있는 구조체를 추출한다.
 *
 * @va: 해제할 가상 주소 범위
 */
static void release_range(struct va_range* va)
{
    if (va == NULL)
    {
        return;
    }

    if (va->remote)
    {
        /* va_range에서 remote_segment 컨테이너를 추출하여 제거 */
        remove_remote_desc(_nvm_container_of(va, struct remote_segment, range));
    }
    else
    {
        /* va_range에서 local_segment 컨테이너를 추출하여 제거 */
        remove_local_desc(_nvm_container_of(va, struct local_segment, range));
    }
}



/*
 * nvm_dis_dma_map_local - 기존 로컬 세그먼트에 대한 DMA 매핑을 생성한다.
 *
 * 이미 생성된 SISCI 로컬 세그먼트를 DMA 핸들로 감싸서 NVMe I/O에 사용할 수 있게 한다.
 * 사용자가 외부에서 세그먼트를 생성한 경우에 사용된다.
 *
 * @map:     생성된 DMA 핸들을 받을 이중 포인터
 * @ctrl:    NVMe 컨트롤러 핸들
 * @adapter: DIS 어댑터 번호
 * @lseg:    기존 SISCI 로컬 세그먼트 핸들
 * @map_va:  true이면 가상 주소 공간에도 매핑
 * @return:  성공 시 0, 실패 시 errno 값
 */
int nvm_dis_dma_map_local(nvm_dma_t** map, const nvm_ctrl_t* ctrl, uint32_t adapter, sci_local_segment_t lseg, bool map_va)
{
    int status;
    struct local_segment* ls;
    struct controller* ref;

    *map = NULL;

    /* 컨트롤러 참조를 획득한다 (참조 카운트 증가) */
    ref = _nvm_ctrl_get(ctrl);
    if (ref == NULL)
    {
        return ENOSYS;
    }

    /* 로컬 세그먼트 디스크립터를 생성한다 */
    status = create_local_desc(&ls, ref, adapter, lseg);
    if (status != 0)
    {
        _nvm_ctrl_put(ref);             /* 실패 시 참조 해제 */
        return status;
    }

    /* 가상 주소 매핑이 요청되면 세그먼트를 가상 주소 공간에 매핑한다 */
    if (map_va)
    {
        status = va_map_local(&ls->map, &ls->range, ls->segment, true);
        if (status != 0)
        {
            remove_local_desc(ls);       /* 실패 시 전체 정리 */
            return status;
        }
    }

    /* DMA 핸들을 초기화한다. 이후 NVMe I/O 커맨드에서 이 DMA 핸들의
     * 물리/I/O 주소가 PRP 엔트리로 사용된다. */
    status = _nvm_dma_init(map, ctrl, &ls->range, &release_range);
    if (status != 0)
    {
        remove_local_desc(ls);
        return status;
    }

    return 0;
}



/*
 * nvm_dis_dma_map_remote - 기존 원격 세그먼트에 대한 DMA 매핑을 생성한다.
 *
 * 이미 연결된 SISCI 원격 세그먼트를 DMA 핸들로 감싸서 NVMe I/O에 사용할 수 있게 한다.
 *
 * @map:     생성된 DMA 핸들을 받을 이중 포인터
 * @ctrl:    NVMe 컨트롤러 핸들
 * @rseg:    기존 SISCI 원격 세그먼트 핸들
 * @map_va:  true이면 가상 주소 공간에도 매핑
 * @map_wc:  true이면 Write-Combining 매핑 (map_va가 true일 때만 유효)
 * @return:  성공 시 0, 실패 시 errno 값
 */
int nvm_dis_dma_map_remote(nvm_dma_t** map, const nvm_ctrl_t* ctrl, sci_remote_segment_t rseg, bool map_va, bool map_wc)
{
    int status;
    struct remote_segment* rs;
    struct controller* ref;

    *map = NULL;

    /* WC 매핑은 가상 주소 매핑 없이는 무의미하다 */
    if (!map_va && map_wc)
    {
        return EINVAL;
    }

    /* 컨트롤러 참조를 획득한다 */
    ref = _nvm_ctrl_get(ctrl);
    if (ref == NULL)
    {
        return ENOSYS;
    }

    /* 원격 세그먼트 디스크립터를 생성한다 */
    status = create_remote_desc(&rs, ref, rseg);
    if (status != 0)
    {
        _nvm_ctrl_put(ref);
        return status;
    }

    /* 가상 주소 매핑이 요청되면 원격 세그먼트를 가상 주소 공간에 매핑한다 */
    if (map_va)
    {
        status = va_map_remote(&rs->map, &rs->range, rs->segment, true, map_wc);
        if (status != 0)
        {
            remove_remote_desc(rs);
            return status;
        }
    }

    /* DMA 핸들을 초기화한다 */
    status = _nvm_dma_init(map, ctrl, &rs->range, &release_range);
    if (status != 0)
    {
        remove_remote_desc(rs);
        return status;
    }

    return 0;
}



/*
 * create_local_segment - 로컬 SISCI 세그먼트를 새로 생성하고 매핑하는 내부 헬퍼
 *
 * _nvm_local_memory_get()으로 세그먼트를 생성하고, 디스크립터를 만들고,
 * 필요한 경우 가상 주소에 매핑한다. 세그먼트 정리 책임도 함께 설정한다(remove=true).
 *
 * @va:   생성된 va_range를 받을 이중 포인터
 * @ctrl: NVMe 컨트롤러 핸들
 * @size: 세그먼트 크기
 * @ptr:  등록할 메모리 포인터 (NULL이면 SISCI가 내부 할당)
 * @gpu:  true이면 CUDA GPU 메모리
 * @return: 성공 시 0, 실패 시 errno 값
 */
static int create_local_segment(struct va_range** va, const nvm_ctrl_t* ctrl, size_t size, void* ptr, bool gpu)
{
    int status;
    struct controller* ref;
    struct local_segment* ls;
    sci_local_segment_t lseg;
    uint32_t adapter;

    if (size == 0)
    {
        return ERANGE;
    }

    /* 컨트롤러 참조를 획득한다 (참조 카운트 증가) */
    ref = _nvm_ctrl_get(ctrl);
    if (ref == NULL)
    {
        return ENOSYS;
    }

    /* SISCI 로컬 세그먼트를 생성한다.
     * ptr이 NULL이면 내부 메모리 할당, 아니면 해당 메모리를 세그먼트에 등록한다. */
    status = _nvm_local_memory_get(&lseg, &adapter, ref->device, size, ptr, gpu);
    if (status != 0)
    {
        _nvm_ctrl_put(ref);
        return status;
    }

    /* 로컬 세그먼트 디스크립터를 생성한다 */
    status = create_local_desc(&ls, ref, adapter, lseg);
    if (status != 0)
    {
        _nvm_local_memory_put(&lseg);   /* 세그먼트 제거 */
        _nvm_ctrl_put(ref);             /* 참조 해제 */
        return status;
    }

    /* 이 함수가 세그먼트를 생성했으므로 정리 시 제거가 필요함을 표시 */
    ls->remove = true;

    /* ptr이 NULL인 경우(SISCI 내부 할당): 가상 주소 공간에 매핑하여 CPU 접근 가능하게 한다 */
    if (ptr == NULL)
    {
        status = va_map_local(&ls->map, &ls->range, ls->segment, true);
        if (status != 0)
        {
            remove_local_desc(ls);       /* 실패 시 세그먼트 제거와 참조 해제를 모두 수행 */
            return status;
        }
    }
    else
    {
        /* XXX 임시 해결: 외부 메모리를 직접 가상 주소로 설정한다.
         * 이미 CPU 접근 가능한 메모리이므로 SISCI 매핑이 불필요하다. */
        ls->range.vaddr = (volatile void*) ptr;
    }

    *va = &ls->range;                    /* 생성된 va_range 반환 */
    return 0;
}



/*
 * create_remote_segment - 디바이스 메모리 세그먼트를 새로 생성하고 연결하고 매핑하는 내부 헬퍼
 *
 * SmartIO 디바이스의 private 메모리에 새 세그먼트를 생성(SCICreateDeviceSegment)하고,
 * 연결(SCIConnectDeviceSegment)하고, 가상 주소에 매핑(SCIMapRemoteSegment)한다.
 * NVMe CMB(Controller Memory Buffer) 등 디바이스 측 메모리를 활용할 때 사용된다.
 *
 * @va:        생성된 va_range를 받을 이중 포인터
 * @ctrl:      NVMe 컨트롤러 핸들
 * @size:      세그먼트 크기
 * @hints:     메모리 힌트 (SISCI에 전달되는 메모리 배치 힌트)
 * @return:    성공 시 0, 실패 시 errno 값
 */
static int create_remote_segment(struct va_range** va, const nvm_ctrl_t* ctrl, size_t size, unsigned int hints)
{
    int status;
    uint32_t id;
    struct controller* ref;
    struct remote_segment* rs;
    sci_remote_segment_t rseg;
    sci_error_t err;

    /* 컨트롤러 참조를 획득한다 */
    ref = _nvm_ctrl_get(ctrl);
    if (ref == NULL)
    {
        return ENOSYS;
    }

    /* 뮤텍스로 보호하며 고유한 세그먼트 ID를 생성한다.
     * 여러 쓰레드가 동시에 세그먼트를 생성할 수 있으므로 카운터에 배타적 접근이 필요하다. */
    status = _nvm_mutex_lock(&ref->device->lock);
    if (status != 0)
    {
        _nvm_ctrl_put(ref);
        return status;
    }

    id = ++ref->device->counter;         /* 세그먼트 ID를 1 증가시켜 할당 */
    _nvm_mutex_unlock(&ref->device->lock);

    /* SCICreateDeviceSegment: SmartIO 디바이스의 private 메모리에 새 세그먼트를 생성한다.
     * SCI_MEMTYPE_PRIVATE는 디바이스 로컬 메모리(예: NVMe CMB)를 의미한다.
     * hints는 메모리 배치에 대한 힌트(예: 특정 NUMA 노드 선호 등)를 전달한다. */
    SCICreateDeviceSegment(ref->device->device, id, size, SCI_MEMTYPE_PRIVATE, hints, 0, &err);
    if (err != SCI_ERR_OK)
    {
        _nvm_ctrl_put(ref);
        dprintf("Failed to create device segment: %s\n", _SCIGetErrorString(err));
        return ENOSPC;
    }

    /* 생성된 디바이스 세그먼트에 연결한다 (SCIConnectDeviceSegment 내부 호출) */
    status = _nvm_device_memory_get(&rseg, ref->device, id, SCI_MEMTYPE_PRIVATE);
    if (status != 0)
    {
        _nvm_ctrl_put(ref);
        return status;
    }

    /* 원격 세그먼트 디스크립터를 생성한다 */
    status = create_remote_desc(&rs, ref, rseg);
    if (status != 0)
    {
        _nvm_device_memory_put(&rseg);   /* 세그먼트 연결 해제 */
        _nvm_ctrl_put(ref);
        return status;
    }

    /* 이 함수가 세그먼트를 연결했으므로 정리 시 연결 해제가 필요함을 표시 */
    rs->disconnect = true;

    /* 원격 세그먼트를 가상 주소 공간에 매핑한다.
     * write=true, wc=true: Write-Combining 모드로 매핑하여 데이터 전송 성능을 최적화한다. */
    status = va_map_remote(&rs->map, &rs->range, rs->segment, true, true);
    if (status != 0)
    {
        remove_remote_desc(rs);          /* 실패 시 전체 정리 (연결 해제 포함) */
        return status;
    }

    *va = &rs->range;                    /* 생성된 va_range 반환 */
    return 0;
}



/*
 * nvm_dis_dma_create - DMA용 세그먼트를 새로 생성하고 DMA 핸들을 초기화한다.
 *
 * mem_hints에 따라 로컬 세그먼트 또는 원격(디바이스) 세그먼트를 생성한다:
 * - mem_hints == 0: 호스트 메모리에 로컬 세그먼트 생성
 * - mem_hints != 0: 디바이스 메모리에 원격 세그먼트 생성 (CMB 활용 등)
 *
 * @map:       생성된 DMA 핸들을 받을 이중 포인터
 * @ctrl:      NVMe 컨트롤러 핸들
 * @size:      DMA 버퍼 크기
 * @mem_hints: 메모리 배치 힌트 (0이면 호스트 메모리, 그 외이면 디바이스 메모리)
 * @return:    성공 시 0, 실패 시 errno 값
 */
int nvm_dis_dma_create(nvm_dma_t** map, const nvm_ctrl_t* ctrl, size_t size, unsigned int mem_hints)
{
    int status;
    struct va_range* va = NULL;

    *map = NULL;

    /* NVM 컨트롤러의 페이지 크기에 맞게 크기를 정렬한다 */
    size = NVM_CTRL_ALIGN(ctrl, size);
    if (size == 0)
    {
        return EINVAL;
    }

    if (mem_hints == 0)
    {
        /* 호스트 메모리에 로컬 세그먼트를 생성한다 */
        status = create_local_segment(&va, ctrl, size, NULL, false);
    }
    else
    {
        /* 디바이스 메모리에 원격 세그먼트를 생성한다 */
        status = create_remote_segment(&va, ctrl, size, mem_hints);
    }

    if (status != 0)
    {
        return status;
    }

    /* DMA 핸들을 초기화한다. _nvm_dma_init은 세그먼트의 I/O 주소를 조회하여
     * nvm_dma_t 핸들에 저장하고, 해제 시 release_range 콜백이 호출되도록 등록한다. */
    status = _nvm_dma_init(map, ctrl, va, &release_range);
    if (status != 0)
    {
        release_range(va);               /* DMA 초기화 실패 시 세그먼트 정리 */
        return status;
    }

    return 0;
}



/*
 * nvm_dis_dma_map_host - 호스트 사용자 메모리를 DMA 매핑한다.
 *
 * 사용자가 이미 할당한 호스트 메모리(vaddr)를 SISCI 세그먼트로 등록하고
 * DMA 핸들을 생성한다. 메모리는 DMA 전송을 위해 내부적으로 pin(고정)된다.
 *
 * @map:   생성된 DMA 핸들을 받을 이중 포인터
 * @ctrl:  NVMe 컨트롤러 핸들
 * @vaddr: 등록할 호스트 메모리의 가상 주소
 * @size:  메모리 크기
 * @return: 성공 시 0, 실패 시 errno 값
 */
int nvm_dis_dma_map_host(nvm_dma_t** map, const nvm_ctrl_t* ctrl, void* vaddr, size_t size)
{
    int status;
    struct va_range* va = NULL;

    *map = NULL;

    if (vaddr == NULL)
    {
        return EINVAL;
    }

    /* NVM 컨트롤러의 페이지 크기에 맞게 크기를 정렬한다 */
    size = NVM_CTRL_ALIGN(ctrl, size);

    /* 사용자 메모리를 SISCI 로컬 세그먼트로 등록한다 (gpu=false) */
    status = create_local_segment(&va, ctrl, size, vaddr, false);
    if (status != 0)
    {
        return status;
    }

    /* DMA 핸들을 초기화한다 */
    status = _nvm_dma_init(map, ctrl, va, &release_range);
    if (status != 0)
    {
        release_range(va);
        return status;
    }

    return 0;
}



#ifdef _CUDA
/*
 * nvm_dis_dma_map_device - CUDA GPU 메모리를 NVMe 컨트롤러용 DMA 매핑한다.
 *
 * CUDA 디바이스 포인터(devptr)를 SISCI 세그먼트로 등록하여
 * NVMe 디바이스가 GPU 메모리에 직접 DMA 전송할 수 있게 한다 (GPUDirect RDMA).
 * 이를 통해 CPU 메모리를 경유하지 않고 NVMe → GPU 직접 전송이 가능해진다.
 *
 * @map:    생성된 DMA 핸들을 받을 이중 포인터
 * @ctrl:   NVMe 컨트롤러 핸들
 * @devptr: CUDA 디바이스 메모리 포인터
 * @size:   메모리 크기
 * @return: 성공 시 0, 실패 시 errno 값
 */
int nvm_dis_dma_map_device(nvm_dma_t** map, const nvm_ctrl_t* ctrl, void* devptr, size_t size)
{
    int status;
    struct va_range* va = NULL;

    *map = NULL;

    if (devptr == NULL)
    {
        return EINVAL;
    }

    /* GPU 메모리는 64KB 정렬이 필요하다 (1ULL << 16 = 65536) */
    size = NVM_PAGE_ALIGN(size, (1ULL << 16));

    /* GPU 메모리를 SISCI 로컬 세그먼트로 등록한다 (gpu=true).
     * 내부적으로 SCIAttachPhysicalMemory()가 호출되어 GPU 메모리가 SISCI에 등록된다. */
    status = create_local_segment(&va, ctrl, size, devptr, true);
    if (status != 0)
    {
        return status;
    }

    /* DMA 핸들을 초기화한다 */
    status = _nvm_dma_init(map, ctrl, va, &release_range);
    if (status != 0)
    {
        release_range(va);
        return status;
    }

    return 0;
}
#endif



/*
 * nvm_dis_node_from_dma - DMA 매핑의 메모리가 위치한 DIS 노드 ID를 조회한다.
 *
 * DMA 핸들에 연결된 세그먼트가 원격 세그먼트이면 해당 노드 ID를 직접 조회하고,
 * 로컬 세그먼트이면 로컬 노드의 ID를 조회하여 반환한다.
 *
 * @handle: DMA 핸들
 * @return: 노드 ID (실패 시 0)
 */
uint32_t nvm_dis_node_from_dma(const nvm_dma_t* handle)
{
    if (handle != NULL)
    {
        const struct va_range* va;
        const nvm_ctrl_t* ctrl;

        /* DMA 핸들에서 NVMe 컨트롤러 핸들을 추출한다 */
        ctrl = nvm_ctrl_from_dma(handle);
        if (ctrl == NULL || _nvm_ctrl_type(ctrl) != DEVICE_TYPE_SMARTIO)
        {
            return 0;                    /* SmartIO 타입이 아니면 노드 ID 조회 불가 */
        }

        /* DMA 핸들에서 va_range를 추출한다 */
        va = _nvm_dma_va(handle);
        if (va == NULL || va->n_pages != 1)
        {
            return 0;                    /* SISCI 세그먼트가 아니면 (n_pages != 1) 처리 불가 */
        }

        if (va->remote)
        {
            /* 원격 세그먼트인 경우: 세그먼트가 위치한 원격 노드의 ID를 조회 */
            const struct remote_segment* rseg = _nvm_container_of(va, struct remote_segment, range);

            /* SCIGetRemoteSegmentNodeId: 원격 세그먼트가 위치한 노드의 DIS 노드 ID를 반환한다 */
            return SCIGetRemoteSegmentNodeId(rseg->segment);
        }
        else
        {
            /* 로컬 세그먼트인 경우: 이 노드(로컬)의 ID를 조회 */
            sci_error_t err;
            uint32_t node_id = 0;
            const struct local_segment* lseg = _nvm_container_of(va, struct local_segment, range);

            /* SCIGetLocalNodeId: 지정된 어댑터의 로컬 노드 ID를 조회한다 */
            SCIGetLocalNodeId(lseg->adapter, &node_id, 0, &err);
            if (err == SCI_ERR_OK)
            {
                return node_id;
            }
        }
    }

    return 0;
}
