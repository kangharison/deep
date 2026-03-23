/* map.h - SISCI 세그먼트 매핑 디스크립터 정의
 *
 * 이 파일은 DIS/SISCI를 통해 생성된 로컬 및 원격 메모리 세그먼트를
 * 가상 주소 공간에 매핑하기 위한 구조체들을 정의한다.
 * local_segment는 호스트(또는 GPU) 메모리 기반 세그먼트를,
 * remote_segment는 원격 노드 또는 디바이스 메모리 기반 세그먼트를 나타낸다.
 * 두 구조체 모두 va_range를 내장하여 DMA I/O 매핑 시스템과 연동된다.
 */
#ifndef __NVM_INTERNAL_DIS_MAP_H__
#define __NVM_INTERNAL_DIS_MAP_H__
#ifdef _SISCI

/* SISCI 클러스터 모드 활성화를 보장하는 매크로 */
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif

/* 필수 헤더 포함 */
#include <stdbool.h>
#include "dma.h"            /* va_range, controller 등 DMA 관련 구조체 */
#include <sisci_types.h>    /* sci_map_t, sci_local_segment_t, sci_remote_segment_t 등 */


/*
 * va_map - 가상 주소 공간 매핑 상태를 추적하는 구조체
 *
 * SISCI 세그먼트가 SCIMapLocalSegment() 또는 SCIMapRemoteSegment()를 통해
 * 프로세스의 가상 주소 공간에 매핑되었는지 여부와 매핑 디스크립터를 보관한다.
 */
struct va_map
{
    bool                    mapped;     /* 세그먼트가 현재 가상 주소 공간에 매핑되어 있는지 여부 */
    sci_map_t               md;         /* SISCI 매핑 디스크립터: SCIMapXxxSegment()가 반환하는 핸들. SCIUnmapSegment()에 사용 */
};



/*
 * local_segment - 로컬 SISCI 세그먼트 디스크립터
 *
 * 이 노드의 호스트 메모리(또는 GPU 메모리)를 기반으로 생성된 SISCI 세그먼트를 나타낸다.
 * range.remote = false로 설정된다.
 * 원격 NVMe 디바이스가 이 세그먼트를 DMA 타겟으로 사용하여 데이터를 읽고 쓸 수 있다.
 */
struct local_segment
{
    /* XXX: ctrl 참조는 새로운 sci_desc_t로 대체될 수 있음 */
    struct controller*      ctrl;       /* NVM 컨트롤러 참조: 디바이스 I/O 매핑 시 필요 */
    uint32_t                adapter;    /* DIS 어댑터 번호: 이 세그먼트가 export된 패브릭 어댑터 */
    sci_local_segment_t     segment;    /* SISCI 로컬 세그먼트 핸들: SCICreateSegment()로 생성 */
    bool                    remove;     /* true이면 이 API가 세그먼트를 생성했으므로 정리 시 SCIRemoveSegment() 호출 필요 */
    struct va_map           map;        /* 가상 주소 매핑 상태 */
    struct va_range         range;      /* DMA 시스템이 사용하는 메모리 범위 디스크립터 (vaddr, page_size 등) */
};



/*
 * remote_segment - 원격 SISCI 세그먼트 디스크립터
 *
 * 원격 노드 또는 디바이스 메모리에 위치한 SISCI 세그먼트를 나타낸다.
 * range.remote = true로 설정된다.
 * 디바이스 측 메모리(예: NVMe CMB)를 호스트에서 접근할 때 사용한다.
 */
struct remote_segment
{
    /* XXX: ctrl 참조는 디바이스 세그먼트에만 필요 */
    struct controller*      ctrl;       /* NVM 컨트롤러 참조 */
    sci_remote_segment_t    segment;    /* SISCI 원격 세그먼트 핸들: SCIConnectDeviceSegment()로 연결 */
    bool                    disconnect; /* true이면 이 API가 세그먼트를 연결했으므로 정리 시 SCIDisconnectSegment() 호출 필요 */
    struct va_map           map;        /* 가상 주소 매핑 상태 */
    struct va_range         range;      /* DMA 시스템이 사용하는 메모리 범위 디스크립터 */
};


#endif /* _SISCI */
#endif /* __NVM_INTERNAL_DIS_MAP_H__ */
