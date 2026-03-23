/* device.c - SmartIO/DIS 디바이스 관리 및 세그먼트 매핑 구현
 *
 * 이 파일은 Dolphin SmartIO를 통해 원격 NVMe 디바이스를 빌려(borrow)오고,
 * BAR0 레지스터 매핑, 로컬/원격 세그먼트의 I/O 매핑, DMA용 세그먼트 생성 등
 * 디바이스 수명주기 전체를 관리하는 핵심 구현체이다.
 *
 * SmartIO 흐름: SCIOpen → SCIBorrowDevice → SCIConnectDeviceSegment(BAR0) → SCIMapRemoteSegment
 * 해제 흐름:    SCIUnmapSegment → SCIDisconnectSegment → SCIReturnDevice → SCIClose
 */

/* _SISCI 매크로가 정의되지 않으면 컴파일을 중단한다 - SISCI 라이브러리 필수 */
#ifndef _SISCI
#error "Must compile with SISCI support"
#endif

/* DIS 클러스터 모드 활성화 */
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif

#include <nvm_types.h>      /* NVM 공통 타입 정의 */
#include <nvm_ctrl.h>       /* nvm_ctrl_t 컨트롤러 핸들 */
#include <nvm_util.h>       /* 유틸리티 매크로 (_nvm_container_of 등) */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include "mutex.h"          /* BAM 내부 뮤텍스 래퍼 */
#include "dis/device.h"     /* struct device 및 세그먼트 함수 선언 */
#include "dis/map.h"        /* struct local_segment, struct remote_segment */
#include "ctrl.h"           /* struct controller, _nvm_ctrl_init 등 */
#include "dma.h"            /* struct va_range, DMA 관련 */
#include "dprintf.h"        /* 디버그 출력 매크로 */
#include <sisci_types.h>    /* SISCI 타입 정의 */
#include <sisci_error.h>    /* SISCI 에러 코드 */
#include <sisci_api.h>      /* SISCI API 함수 선언 */



/*
 * query_device - SmartIO 디바이스의 정보를 질의한다.
 *
 * SCIQuery()를 SCI_Q_DEVICE 서브커맨드로 호출하여 디바이스의 어댑터 번호,
 * 노드 ID 등의 정보를 가져온다. 이 정보는 로컬 세그먼트를 어떤 어댑터에
 * export할지 결정하는 데 사용된다.
 *
 * @dev:  조회할 디바이스
 * @info: 조회 결과를 저장할 구조체 포인터
 * @return: 성공 시 0, 실패 시 EIO
 */
static int query_device(const struct device* dev, sci_smartio_device_info_t* info)
{
    sci_error_t err;
    sci_smartio_query_device_t query;    /* SISCI 디바이스 질의 파라미터 구조체 */

    query.fdid = dev->fdid;             /* 조회 대상 FDID 설정 */
    query.subcommand = SCI_Q_DEVICE_INFO;  /* 디바이스 기본 정보 조회 서브커맨드 */
    query.data = (void*) info;          /* 결과를 받을 버퍼 포인터 설정 */

    /* SCIQuery: SISCI 범용 질의 API. SCI_Q_DEVICE 타입으로 디바이스 정보를 조회한다 */
    SCIQuery(SCI_Q_DEVICE, (void*) &query, 0, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to query device: %s\n", _SCIGetErrorString(err));
        return EIO;
    }

    return 0;
}



/*
 * _nvm_device_memory_get - 원격 디바이스의 메모리 세그먼트에 연결하는 헬퍼 함수
 *
 * SCIConnectDeviceSegment()를 호출하여 SmartIO 디바이스의 특정 메모리 영역에 연결한다.
 * memtype에 따라 BAR 영역, private 메모리, shared 메모리 등에 접근할 수 있다.
 *
 * @segment: 연결된 원격 세그먼트 핸들을 받을 포인터
 * @dev:     대상 디바이스
 * @id:      세그먼트 식별자
 * @memtype: 메모리 유형 (SCI_MEMTYPE_BAR, SCI_MEMTYPE_PRIVATE, SCI_MEMTYPE_SHARED)
 * @return:  성공 시 0, 실패 시 적절한 errno 값
 */
int _nvm_device_memory_get(sci_remote_segment_t* segment, const struct device* dev, uint32_t id, unsigned int memtype)
{
    sci_error_t err;
    sci_remote_segment_t seg = NULL;    /* 연결될 원격 세그먼트 핸들 */

    *segment = NULL;                    /* 출력 파라미터 초기화 */

    /* SCIConnectDeviceSegment: SmartIO 디바이스의 메모리 세그먼트에 연결한다.
     * dev->device는 SCIBorrowDevice()로 얻은 디바이스 핸들이며,
     * id와 memtype으로 어떤 메모리 영역에 접근할지 결정한다. */
    SCIConnectDeviceSegment(dev->device, &seg, id, memtype, NULL, NULL, 0, &err);
    if (err == SCI_ERR_OK)
    {
        *segment = seg;                 /* 성공: 세그먼트 핸들 반환 */
        return 0;
    }

    /* 에러 코드를 POSIX errno로 변환 */
    switch (err)
    {
        case SCI_ERR_API_NOSPC:         /* API 내부 자원 부족 */
            return ENOMEM;

        case SCI_ERR_NOSPC:             /* 시스템 자원 부족 */
            return ENOSPC;

        case SCI_ERR_CONNECTION_REFUSED: /* 원격 노드가 연결을 거부 */
            return ECONNREFUSED;

        case SCI_ERR_NO_SUCH_SEGMENT:   /* 요청한 세그먼트가 존재하지 않음 */
        case SCI_ERR_NO_SUCH_NODEID:    /* 요청한 노드가 존재하지 않음 */
        case SCI_ERR_NO_REMOTE_LINK_ACCESS:  /* 원격 링크 접근 불가 */
        case SCI_ERR_NODE_NOT_RESPONDING:    /* 원격 노드 응답 없음 */
        case SCI_ERR_SEGMENT_NOT_CONNECTED:  /* 세그먼트 미연결 상태 */
        case SCI_ERR_NO_LINK_ACCESS:    /* 링크 접근 불가 */
            return EHOSTUNREACH;        /* 네트워크/패브릭 도달 불가 */

        default:
            dprintf("Failed to connect segment%s\n", SCIGetErrorString(err));
            return ENOMEM;
    }
}



/*
 * _nvm_device_memory_put - 디바이스 메모리 세그먼트 연결을 해제한다.
 *
 * SCIDisconnectSegment()를 BUSY가 아닐 때까지 반복 호출하여
 * 세그먼트 연결을 안전하게 해제한다.
 *
 * @segment: 해제할 원격 세그먼트 핸들의 포인터 (해제 후 NULL로 설정됨)
 */
void _nvm_device_memory_put(sci_remote_segment_t* segment)
{
    if (segment != NULL && *segment != NULL)
    {
        sci_error_t err = SCI_ERR_OK;

        /* SCIDisconnectSegment: 원격 세그먼트 연결을 해제한다.
         * SCI_ERR_BUSY가 반환되면 세그먼트가 아직 사용 중이므로 재시도한다. */
        do
        {
            SCIDisconnectSegment(*segment, 0, &err);
        }
        while (err == SCI_ERR_BUSY);    /* BUSY이면 재시도 (다른 쓰레드가 사용 중일 수 있음) */

#ifndef NDEBUG
        if (err != SCI_ERR_OK)
        {
            dprintf("Failed to disconnect segment: %s\n", _SCIGetErrorString(err));
        }
#endif

        *segment = NULL;                /* 해제 완료 후 핸들을 NULL로 설정하여 이중 해제 방지 */
    }
}



/*
 * _nvm_local_memory_get - 로컬 SISCI 세그먼트를 생성한다.
 *
 * 호스트 메모리 또는 GPU 메모리를 SISCI 세그먼트로 등록하여
 * DIS 패브릭을 통해 원격 디바이스가 DMA로 접근 가능하도록 한다.
 *
 * 처리 흐름:
 * 1. query_device()로 디바이스 도달 가능한 어댑터 번호 조회
 * 2. SCICreateSegment()로 로컬 세그먼트 생성
 * 3. (선택) GPU 메모리이면 SCIAttachPhysicalMemory(), 호스트 메모리이면 SCIRegisterSegmentMemory()
 * 4. SCIPrepareSegment()로 어댑터에 세그먼트 export 준비
 *
 * @segment: 생성된 세그먼트 핸들을 받을 포인터
 * @adapter: 선택된 어댑터 번호를 받을 포인터
 * @dev:     대상 디바이스 (어댑터 정보 조회용)
 * @size:    세그먼트 크기(바이트)
 * @ptr:     등록할 메모리 주소 (NULL이면 SISCI가 내부 할당)
 * @gpu:     true이면 CUDA GPU 메모리로 취급
 * @return:  성공 시 0, 실패 시 적절한 errno 값
 */
int _nvm_local_memory_get(sci_local_segment_t* segment, uint32_t* adapter, const struct device* dev, size_t size, void* ptr, bool gpu)
{
    int status;
    sci_error_t err;
    sci_smartio_device_info_t info;      /* 디바이스 질의 결과 */
    uint32_t flags = SCI_FLAG_AUTO_ID;   /* 세그먼트 ID 자동 할당 플래그 */
    sci_local_segment_t seg = NULL;
    uint32_t adapt;

    *segment = NULL;                     /* 출력 파라미터 초기화 */
    *adapter = 0;

    /* GPU 메모리 모드인데 포인터가 NULL이면 유효하지 않은 인자 */
    if (gpu && ptr == NULL)
    {
        return EINVAL;
    }

    /* 외부 메모리를 등록하는 경우 SCI_FLAG_EMPTY를 설정하여 빈 세그먼트를 생성한다.
     * 빈 세그먼트는 나중에 메모리를 연결(attach/register)할 수 있다. */
    if (ptr != NULL)
    {
        flags |= SCI_FLAG_EMPTY;
    }

    /* 디바이스 정보를 질의하여 도달 가능한 어댑터 번호를 얻는다 */
    status = query_device(dev, &info);
    if (status != 0)
    {
        return status;
    }

    /* 질의 결과에서 해당 디바이스에 도달 가능한 어댑터 번호를 추출한다 */
    adapt = info.adapter;

    /* SCICreateSegment: 로컬 SISCI 세그먼트를 생성한다.
     * dev->sd는 SISCI 세션 디스크립터, 512는 사용되지 않는 ID(AUTO_ID이므로),
     * size는 세그먼트 크기이다. */
    SCICreateSegment(dev->sd, &seg, 512, size, NULL, NULL, flags, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to create local segment: %s\n", _SCIGetErrorString(err));
        return ENOSPC;
    }

    /* GPU 메모리를 세그먼트에 부착하는 경우 */
    if (ptr != NULL && gpu)
    {
#ifdef _CUDA
        /* SCIAttachPhysicalMemory: CUDA GPU 메모리를 SISCI 세그먼트에 물리적으로 부착한다.
         * SCI_FLAG_CUDA_BUFFER 플래그가 GPU 메모리임을 명시한다.
         * 이를 통해 원격 디바이스가 GPU 메모리에 직접 DMA 가능해진다 (GPUDirect). */
        SCIAttachPhysicalMemory(0, ptr, 0, size, seg, SCI_FLAG_CUDA_BUFFER, &err);
        if (err != SCI_ERR_OK)
        {
            dprintf("Failed to attach GPU memory to local segment: %s\n", _SCIGetErrorString(err));
            SCIRemoveSegment(seg, 0, &err);  /* 실패 시 세그먼트 정리 */
            return EIO;
        }
#else
        /* CUDA 지원 없이 컴파일된 경우 GPU 메모리 사용 불가 */
        SCIRemoveSegment(seg, 0, &err);
        return ENOTSUP;
#endif
    }
    else if (ptr != NULL)
    {
        /* SCIRegisterSegmentMemory: 사용자 공간 호스트 메모리를 SISCI 세그먼트에 등록한다.
         * SCI_FLAG_LOCK_USER_MEM 플래그로 메모리를 고정(pin)하여
         * DMA 전송 중 페이지 스왑을 방지한다. */
        SCIRegisterSegmentMemory(ptr, size, seg, SCI_FLAG_LOCK_USER_MEM, &err);
        if (err != SCI_ERR_OK)
        {
            dprintf("Failed to register local segment memory: %s\n", _SCIGetErrorString(err));
            SCIRemoveSegment(seg, 0, &err);  /* 실패 시 세그먼트 정리 */
            return ENOTTY;
        }
    }

    /* SCIPrepareSegment: 세그먼트를 지정된 어댑터에서 export할 준비를 한다.
     * 이 호출 이후 SCISetSegmentAvailable()을 하면 원격 노드/디바이스가 접근 가능해진다. */
    SCIPrepareSegment(seg, adapt, 0, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to prepare local segment on adapter %u (ptr=%p, size=%zx): %s\n",
                adapt, ptr, size, _SCIGetErrorString(err));
        SCIRemoveSegment(seg, 0, &err);      /* 실패 시 세그먼트 정리 */
        return EIO;
    }

    *segment = seg;                          /* 생성된 세그먼트 핸들 반환 */
    *adapter = adapt;                        /* 사용된 어댑터 번호 반환 */
    return 0;
}



/*
 * _nvm_local_memory_put - 로컬 SISCI 세그먼트를 제거한다.
 *
 * SCIRemoveSegment()를 BUSY가 아닐 때까지 반복 호출하여 세그먼트를 안전하게 제거한다.
 *
 * @segment: 제거할 로컬 세그먼트 핸들의 포인터 (제거 후 NULL로 설정됨)
 */
void _nvm_local_memory_put(sci_local_segment_t* segment)
{
    if (segment != NULL && *segment != NULL)
    {
        sci_error_t err = SCI_ERR_OK;

        /* SCIRemoveSegment: 로컬 세그먼트를 제거한다.
         * BUSY이면 세그먼트가 아직 매핑되어 있거나 사용 중이므로 재시도한다. */
        do
        {
            SCIRemoveSegment(*segment, 0, &err);
        }
        while (err == SCI_ERR_BUSY);

#ifndef NDEBUG
        if (err != SCI_ERR_OK)
        {
            dprintf("Failed to remove local segment: %s\n", _SCIGetErrorString(err));
        }
#endif

        *segment = NULL;                     /* 이중 제거 방지 */
    }
}



/*
 * io_map_local - 로컬 세그먼트를 디바이스의 I/O 주소 공간에 매핑한다.
 *
 * 원격 NVMe 디바이스가 이 로컬 세그먼트(호스트 메모리)에 DMA로 접근할 수 있도록
 * I/O 주소를 할당한다. 이것이 NVMe 커맨드의 PRP/SGL에 들어가는 주소가 된다.
 *
 * 흐름: SCISetSegmentAvailable() → SCIMapLocalSegmentForDevice()
 *
 * @dev:    대상 디바이스
 * @ls:     매핑할 로컬 세그먼트
 * @ioaddr: 할당된 I/O 주소를 받을 포인터
 * @return: 성공 시 0, 실패 시 적절한 errno 값
 */
static int io_map_local(const struct device* dev, const struct local_segment* ls, uint64_t* ioaddr)
{
    sci_error_t err;
    sci_ioaddr_t addr;                   /* 디바이스가 인식하는 I/O 주소 */
    size_t size;

    *ioaddr = 0;                         /* 출력 초기화 */

    /* 로컬 세그먼트의 크기를 조회한다 */
    size = SCIGetLocalSegmentSize(ls->segment);
    if (size == 0)
    {
        return EINVAL;
    }

    /* SCISetSegmentAvailable: 세그먼트를 해당 어댑터에서 접근 가능 상태로 만든다.
     * 이 호출 이후에야 원격 노드/디바이스가 이 세그먼트를 볼 수 있다. */
    SCISetSegmentAvailable(ls->segment, ls->adapter, 0, &err);
    switch (err)
    {
        case SCI_ERR_OK:
            break;

        case SCI_ERR_ILLEGAL_OPERATION:
        case SCI_ERR_SEGMENT_NOT_PREPARED:   /* SCIPrepareSegment()가 선행되지 않음 */
            return EINVAL;

        default:
            dprintf("Failed to set segment available: %s\n", _SCIGetErrorString(err));
            return EIO;
    }

    /* SCIMapLocalSegmentForDevice: 로컬 세그먼트를 특정 디바이스의 I/O 주소 공간에 매핑한다.
     * 반환된 addr은 디바이스(NVMe 컨트롤러)가 DMA 시 사용할 물리 주소이다.
     * 이 주소는 NVMe 커맨드의 PRP 엔트리에 기록된다. */
    SCIMapLocalSegmentForDevice(ls->segment, ls->adapter, dev->device, &addr, 0, size, 0, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to map local segment for device: %s\n", _SCIGetErrorString(err));
        return EIO;
    }

    *ioaddr = (uint64_t) addr;           /* I/O 주소를 uint64_t로 변환하여 반환 */
    return 0;
}



/*
 * io_map_remote - 원격 세그먼트를 디바이스의 I/O 주소 공간에 매핑한다.
 *
 * 원격 노드의 메모리 또는 디바이스 메모리 세그먼트를 NVMe 디바이스가
 * DMA로 접근할 수 있도록 I/O 주소를 할당한다.
 * P2P(Peer-to-Peer) DMA에 해당하는 경우가 많다.
 *
 * @dev:    대상 디바이스
 * @rs:     매핑할 원격 세그먼트
 * @ioaddr: 할당된 I/O 주소를 받을 포인터
 * @return: 성공 시 0, 실패 시 적절한 errno 값
 */
static int io_map_remote(const struct device* dev, const struct remote_segment* rs, uint64_t* ioaddr)
{
    sci_error_t err;
    sci_ioaddr_t addr;
    size_t size;

    *ioaddr = 0;

    /* 원격 세그먼트의 크기를 조회한다 */
    size = SCIGetRemoteSegmentSize(rs->segment);
    if (size == 0)
    {
        return EINVAL;
    }

    /* SCIMapRemoteSegmentForDevice: 원격 세그먼트를 디바이스의 I/O 주소 공간에 매핑한다.
     * 디바이스가 원격 메모리에 직접 DMA할 수 있는 주소를 할당받는다.
     * 이를 통해 NVMe 디바이스 → 원격 노드 메모리 간 P2P 전송이 가능해진다. */
    SCIMapRemoteSegmentForDevice(rs->segment, dev->device, &addr, 0, size, 0, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to map remote segment for device: %s\n", _SCIGetErrorString(err));
        return EIO;
    }

    *ioaddr = (uint64_t) addr;
    return 0;
}



/*
 * borrow_device - SmartIO 디바이스를 빌려오고 BAR0를 매핑한다.
 *
 * NVMe 컨트롤러의 전체 초기화 과정:
 * 1. struct device 할당 및 초기화
 * 2. SCIOpen()으로 SISCI 세션 시작
 * 3. SCIBorrowDevice()로 원격 NVMe 디바이스를 이 노드에 빌려옴
 * 4. _nvm_device_memory_get()으로 BAR0 세그먼트에 연결
 * 5. SCIMapRemoteSegment()로 BAR0를 가상 주소 공간에 매핑
 *
 * "빌려오기(borrow)"는 SmartIO의 핵심 개념으로, 물리적으로 다른 노드에 있는
 * PCIe 디바이스를 이 노드에서 마치 로컬 디바이스처럼 사용할 수 있게 한다.
 *
 * @handle: 생성된 디바이스 핸들을 받을 이중 포인터
 * @fdid:   빌려올 디바이스의 FDID
 * @return: 성공 시 0, 실패 시 적절한 errno 값
 */
static int borrow_device(struct device** handle, uint32_t fdid)
{
    int status;
    sci_error_t err;
    struct device* dev;

    *handle = NULL;
    /* 디바이스 핸들 구조체를 힙에 할당한다 */
    dev = (struct device*) malloc(sizeof(struct device));
    if (dev == NULL)
    {
        dprintf("Failed to allocate device handle: %s\n", strerror(errno));
        return ENOMEM;
    }

    /* 뮤텍스 초기화: 세그먼트 ID 카운터 등 공유 자원 보호에 사용 */
    status = _nvm_mutex_init(&dev->lock);
    if (status != 0)
    {
        free(dev);
        return status;
    }

    /* 디바이스 핸들의 모든 필드를 초기값으로 설정 */
    dev->fdid = fdid;       /* 빌려올 디바이스의 패브릭 ID */
    dev->sd = NULL;
    dev->counter = 0;       /* 세그먼트 ID 카운터 0부터 시작 */
    dev->device = NULL;
    dev->segment = NULL;
    dev->size = 0;
    dev->ptr = NULL;
    dev->md = NULL;

    /* SCIOpen: SISCI 세션(가상 디바이스 디스크립터)을 연다.
     * 모든 SISCI 작업은 이 세션을 통해 수행된다. */
    SCIOpen(&dev->sd, 0, &err);
    if (err != SCI_ERR_OK)
    {
        _nvm_mutex_free(&dev->lock);
        free(dev);
        dprintf("Failed to create SISCI virtual device: %s\n", _SCIGetErrorString(err));
        return ENOSYS;
    }

    /* SCIBorrowDevice: SmartIO 패브릭에서 FDID로 지정된 디바이스를 빌려온다.
     * "빌려오기"는 디바이스의 참조 카운트를 증가시키고,
     * 이 노드에서 해당 디바이스의 BAR 영역 접근 등을 가능하게 한다. */
    SCIBorrowDevice(dev->sd, &dev->device, dev->fdid, 0, &err);
    if (err != SCI_ERR_OK)
    {
        status = ENODEV;
        dprintf("Failed to increase device reference: %s\n", _SCIGetErrorString(err));
        switch (err)
        {
            case SCI_ERR_NO_SUCH_FDID:      /* 해당 FDID의 디바이스가 패브릭에 없음 */
                status = ENODEV;
                break;


            case SCI_ERR_NOT_AVAILABLE:      /* 디바이스가 이미 다른 노드에 의해 독점 사용 중 */
                status = EBUSY;
                break;

            default:
                status = EIO;
                break;
        }

        /* 실패 시 자원 정리 */
        SCIClose(dev->sd, 0, &err);
        _nvm_mutex_free(&dev->lock);
        free(dev);
        return status;
    }

    /* BAR0 세그먼트에 연결한다. id=0은 BAR0, memtype=SCI_MEMTYPE_BAR는 PCI BAR 영역을 의미한다.
     * NVMe 컨트롤러의 BAR0에는 MLBAR(Memory-mapped Location BAR) 레지스터들이 위치한다. */
    status = _nvm_device_memory_get(&dev->segment, dev, 0, SCI_MEMTYPE_BAR);
    if (status != 0)
    {
        SCIReturnDevice(dev->device, 0, &err);  /* 디바이스 반환 */
        SCIClose(dev->sd, 0, &err);
        _nvm_mutex_free(&dev->lock);
        free(dev);
        return status;
    }

    /* 연결된 BAR0 세그먼트의 크기를 조회한다 */
    dev->size = SCIGetRemoteSegmentSize(dev->segment);

    /* SCIMapRemoteSegment: 원격 세그먼트(BAR0)를 이 프로세스의 가상 주소 공간에 매핑한다.
     * SCI_FLAG_IO_MAP_IOSPACE 플래그는 write-combining 없이 I/O 공간으로 매핑하라는 의미이다.
     * 레지스터 접근은 uncacheable이어야 하므로 WC가 아닌 IO 매핑을 사용한다. */
    dev->ptr = SCIMapRemoteSegment(dev->segment, &dev->md, 0, dev->size, NULL, SCI_FLAG_IO_MAP_IOSPACE, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to map device memory into local address space: %s\n", _SCIGetErrorString(err));
        _nvm_device_memory_put(&dev->segment);
        SCIReturnDevice(dev->device, 0, &err);
        SCIClose(dev->sd, 0, &err);
        _nvm_mutex_free(&dev->lock);
        free(dev);
        return ENOSPC;
    }

    *handle = dev;                       /* 성공: 완전히 초기화된 디바이스 핸들 반환 */
    return 0;
}



/*
 * return_device - 빌려온 디바이스를 반환하고 모든 자원을 해제한다.
 *
 * borrow_device()의 역순으로 모든 매핑과 연결을 해제한다:
 * SCIUnmapSegment → _nvm_device_memory_put → SCIReturnDevice → SCIClose → free
 *
 * @dev:     반환할 디바이스 핸들
 * @mm_ptr:  매핑된 포인터 (일관성 검증용)
 * @mm_size: 매핑 크기 (일관성 검증용)
 */
static void return_device(struct device* dev, volatile void* mm_ptr, size_t mm_size)
{
    sci_error_t err;

    /* 전달된 파라미터가 디바이스 핸들의 값과 일치하는지 검증 */
    if (mm_ptr != dev->ptr || mm_size != dev->size)
    {
        dprintf("Something is not right\n");
    }

    /* SCIUnmapSegment: BAR0의 가상 주소 매핑을 해제한다.
     * BUSY이면 매핑이 아직 사용 중이므로 재시도한다. */
    do
    {
        SCIUnmapSegment(dev->md, 0, &err);
    }
    while (err == SCI_ERR_BUSY);

    /* BAR0 원격 세그먼트 연결 해제 */
    _nvm_device_memory_put(&dev->segment);

    /* SCIReturnDevice: 빌려온 디바이스를 SmartIO 패브릭에 반환한다.
     * 참조 카운트가 감소하며, 0이 되면 디바이스가 해제된다. */
    SCIReturnDevice(dev->device, 0, &err);
    /* SISCI 세션 닫기 */
    SCIClose(dev->sd, 0, &err);
    /* 뮤텍스 해제 */
    _nvm_mutex_free(&dev->lock);
    /* 디바이스 핸들 메모리 해제 */
    free(dev);
}



/*
 * io_map - va_range 타입에 따라 적절한 I/O 매핑 함수를 디스패치한다.
 *
 * SISCI 세그먼트는 물리적으로 연속된 메모리이므로 하나의 큰 페이지로 취급되며,
 * va->n_pages는 반드시 1이어야 한다. va->remote 필드에 따라
 * io_map_remote() 또는 io_map_local()을 호출한다.
 *
 * @dev:    대상 디바이스
 * @va:     매핑할 가상 주소 범위
 * @ioaddr: 할당된 I/O 주소를 받을 포인터
 * @return: 성공 시 0, 실패 시 적절한 errno 값
 */
static int io_map(const struct device* dev, const struct va_range* va, uint64_t* ioaddr)
{
#ifndef NDEBUG
    /* SISCI 세그먼트는 연속 메모리이므로 페이지 수가 반드시 1이어야 한다 */
    if (va->n_pages != 1)
    {
        dprintf("Virtual address range must be a SISCI segment\n");
        return EINVAL;
    }
#endif

    if (va->remote)
    {
        /* 원격 세그먼트인 경우: va_range에서 remote_segment 컨테이너를 추출하여 매핑 */
        return io_map_remote(dev, _nvm_container_of(va, struct remote_segment, range), ioaddr);
    }
    else
    {
        /* 로컬 세그먼트인 경우: va_range에서 local_segment 컨테이너를 추출하여 매핑 */
        return io_map_local(dev, _nvm_container_of(va, struct local_segment, range), ioaddr);
    }
}



/*
 * io_unmap - 디바이스의 I/O 주소 공간에서 세그먼트 매핑을 해제한다.
 *
 * va->remote 필드에 따라 원격 또는 로컬 세그먼트의 I/O 매핑을 해제한다.
 * 로컬 세그먼트의 경우 추가로 SCISetSegmentUnavailable()을 호출하여
 * 세그먼트를 비공개 상태로 되돌린다.
 *
 * @dev: 대상 디바이스
 * @va:  매핑 해제할 가상 주소 범위
 */
static void io_unmap(const struct device* dev, const struct va_range* va)
{
    sci_error_t err = SCI_ERR_OK;

    if (va->remote)
    {
        /* 원격 세그먼트의 I/O 매핑 해제 */
        const struct remote_segment* rs;
        rs = _nvm_container_of(va, struct remote_segment, range);

        /* SCIUnmapRemoteSegmentForDevice: 디바이스의 I/O 주소 공간에서 원격 세그먼트 매핑을 해제한다 */
        do
        {
            SCIUnmapRemoteSegmentForDevice(rs->segment, dev->device, 0, &err);
        }
        while (err == SCI_ERR_BUSY);

#ifndef NDEBUG
        if (err != SCI_ERR_OK)
        {
            dprintf("Unmapping remote segment for device failed: %s\n", _SCIGetErrorString(err));
        }
#endif
    }
    else
    {
        /* 로컬 세그먼트의 I/O 매핑 해제 */
        const struct local_segment* ls;
        ls = _nvm_container_of(va, struct local_segment, range);

        /* SCIUnmapLocalSegmentForDevice: 디바이스의 I/O 주소 공간에서 로컬 세그먼트 매핑을 해제한다 */
        do
        {
            SCIUnmapLocalSegmentForDevice(ls->segment, ls->adapter, dev->device, 0, &err);
        }
        while (err == SCI_ERR_BUSY);

#ifndef NDEBUG
        if (err != SCI_ERR_OK)
        {
            dprintf("Unmapping local segment for device failed: %s\n", _SCIGetErrorString(err));
        }
#endif

        /* SCISetSegmentUnavailable: 세그먼트를 해당 어댑터에서 비공개로 전환한다.
         * 이후 원격 노드/디바이스가 더 이상 이 세그먼트에 접근할 수 없다. */
        do
        {
            SCISetSegmentUnavailable(ls->segment, ls->adapter, 0, &err);
        }
        while (err == SCI_ERR_BUSY);
    }
}



/*
 * smartio_device_ops - SmartIO 디바이스에 대한 연산 함수 테이블
 *
 * BAM의 디바이스 추상화 레이어에서 SmartIO 백엔드의 구체적인 동작을 정의한다.
 * release_device, map_range, unmap_range 세 가지 콜백을 등록한다.
 */
static const struct device_ops smartio_device_ops =
{
    .release_device = &return_device,    /* 디바이스 반환 함수 */
    .map_range = &io_map,                /* I/O 주소 매핑 함수 */
    .unmap_range = &io_unmap,            /* I/O 주소 매핑 해제 함수 */
};



/*
 * nvm_dis_ctrl_init - DIS/SmartIO를 통해 NVMe 컨트롤러를 초기화한다.
 *
 * 외부에 공개되는 최상위 초기화 함수로, FDID를 받아 해당 디바이스를 빌려오고
 * NVM 컨트롤러 핸들(nvm_ctrl_t)을 생성한다.
 *
 * @ctrl: 생성된 컨트롤러 핸들을 받을 이중 포인터
 * @fdid: 대상 NVMe 디바이스의 FDID
 * @return: 성공 시 0, 실패 시 errno 값
 */
int nvm_dis_ctrl_init(nvm_ctrl_t** ctrl, uint32_t fdid)
{
    int err;
    struct device* dev;

    *ctrl = NULL;

    /* SmartIO 디바이스를 빌려오고 BAR0를 매핑한다 */
    err = borrow_device(&dev, fdid);
    if (err != 0)
    {
        return err;
    }

    /* 내부 컨트롤러 구조체를 초기화한다.
     * DEVICE_TYPE_SMARTIO로 마크하고, BAR0 매핑 포인터와 크기를 전달한다.
     * _nvm_ctrl_init은 BAR0 레지스터를 읽어 NVMe 컨트롤러 파라미터를 추출한다. */
    err = _nvm_ctrl_init(ctrl, dev, &smartio_device_ops, DEVICE_TYPE_SMARTIO, dev->ptr, dev->size);
    if (err != 0)
    {
        return_device(dev, dev->ptr, dev->size);  /* 실패 시 디바이스 반환 */
        return err;
    }

    return 0;
}



/*
 * nvm_dis_ctrl_unmap_p2p_device - P2P 디바이스의 BAR 매핑을 해제한다.
 *
 * 다른 SmartIO 디바이스(예: GPU)가 이 NVMe 컨트롤러의 BAR0에 직접 접근하던
 * P2P 매핑을 해제한다.
 *
 * @ctrl: NVMe 컨트롤러 핸들
 * @dev:  P2P 매핑을 해제할 SmartIO 디바이스 핸들
 */
void nvm_dis_ctrl_unmap_p2p_device(const nvm_ctrl_t* ctrl, sci_smartio_device_t dev)
{
    if (ctrl != NULL)
    {
        /* 컨트롤러 핸들에서 내부 controller 구조체를 추출한다 */
        const struct controller* container = _nvm_container_of(ctrl, struct controller, handle);

        /* SmartIO 디바이스 타입인지 확인 */
        if (container->device != NULL && container->type == DEVICE_TYPE_SMARTIO)
        {
            sci_error_t err;

            /* SCIUnmapRemoteSegmentForDevice: P2P 디바이스의 BAR0 매핑을 해제한다 */
            do
            {
                SCIUnmapRemoteSegmentForDevice(container->device->segment, dev, 0, &err);
            }
            while (err == SCI_ERR_BUSY);
        }
    }
}



/*
 * nvm_dis_ctrl_map_p2p_device - P2P 디바이스에 NVMe 컨트롤러 BAR0를 매핑한다.
 *
 * 다른 SmartIO 디바이스(예: GPU)가 이 NVMe 컨트롤러의 BAR0 레지스터에
 * P2P DMA로 직접 접근할 수 있도록 I/O 주소를 매핑한다.
 * GPU가 NVMe Submission Queue에 직접 doorbell을 울리는 등의 시나리오에서 사용된다.
 *
 * @ctrl:   NVMe 컨트롤러 핸들
 * @dev:    P2P 접근할 SmartIO 디바이스 핸들
 * @ioaddr: 매핑된 I/O 주소를 받을 포인터 (NULL이면 주소를 반환하지 않음)
 * @return: 성공 시 0, 실패 시 errno 값
 */
int nvm_dis_ctrl_map_p2p_device(const nvm_ctrl_t* ctrl, sci_smartio_device_t dev, uint64_t* ioaddr)
{
    sci_error_t err;
    sci_ioaddr_t addr;
    const struct controller* container;

    if (ctrl == NULL)
    {
        return EINVAL;
    }

    /* 컨트롤러 핸들에서 내부 controller 구조체를 추출한다 */
    container = _nvm_container_of(ctrl, struct controller, handle);
    if (container->device == NULL || container->type != DEVICE_TYPE_SMARTIO)
    {
        return EINVAL;
    }

    /* SCIMapRemoteSegmentForDevice: NVMe BAR0 세그먼트를 P2P 디바이스의 I/O 주소 공간에 매핑한다.
     * 이를 통해 GPU 등의 디바이스가 NVMe 컨트롤러 레지스터에 직접 접근할 수 있다. */
    SCIMapRemoteSegmentForDevice(container->device->segment, dev, &addr, 0, container->device->size, 0, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to map controller BAR for device: %s\n", _SCIGetErrorString(err));
        return EIO;
    }

    if (ioaddr != NULL)
    {
        *ioaddr = (uint64_t) addr;       /* 매핑된 I/O 주소 반환 */
    }

    return 0;
}



/*
 * nvm_dis_node_from_ctrl - NVMe 컨트롤러가 위치한 DIS 노드 ID를 조회한다.
 *
 * SmartIO 디바이스 정보를 질의하여 해당 NVMe 디바이스가 물리적으로 연결된
 * DIS 노드의 식별자를 반환한다.
 *
 * @ctrl:   NVMe 컨트롤러 핸들
 * @return: 노드 ID (실패 시 0)
 */
uint32_t nvm_dis_node_from_ctrl(const nvm_ctrl_t* ctrl)
{
    int err;
    struct controller* c;
    sci_smartio_device_info_t info;

    if (ctrl == NULL)
    {
        return 0;
    }

    /* SmartIO 타입의 컨트롤러인지 확인 */
    if (_nvm_ctrl_type(ctrl) != DEVICE_TYPE_SMARTIO)
    {
        return 0;
    }

    /* 컨트롤러 참조를 획득한다 (참조 카운트 증가) */
    c = _nvm_ctrl_get(ctrl);
    if (c == NULL)
    {
        return 0;
    }

    /* 디바이스 정보를 질의하여 노드 ID를 얻는다 */
    err = query_device(c->device, &info);
    _nvm_ctrl_put(c);                    /* 참조 카운트 감소 */

    if (err == 0)
    {
        return info.nodeid;              /* 디바이스가 위치한 노드의 ID */
    }

    return 0;
}
