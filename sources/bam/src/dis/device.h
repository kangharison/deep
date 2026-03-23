/* device.h - SmartIO/DIS 디바이스 디스크립터 및 메모리 세그먼트 관리 헤더
 *
 * 이 파일은 Dolphin Interconnect Solutions(DIS)의 SmartIO 프레임워크를 통해
 * 원격 NVMe 디바이스에 접근하기 위한 디바이스 핸들 구조체와
 * SISCI 세그먼트 생성/연결 함수의 선언을 담고 있다.
 * SmartIO는 PCIe 패브릭을 통해 원격 장치를 빌려(borrow) 사용하는 구조이며,
 * SISCI API는 이 패브릭 위에서 공유 메모리 세그먼트를 생성·매핑하는 인터페이스를 제공한다.
 */
#ifndef __NVM_INTERNAL_DIS_DEVICE_H__
#define __NVM_INTERNAL_DIS_DEVICE_H__
#ifdef _SISCI

/* SISCI 클러스터 모드 활성화를 보장하는 매크로 */
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif

/* 필수 헤더 포함 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sisci_types.h>  /* SISCI API의 기본 타입 정의 (sci_desc_t, sci_remote_segment_t 등) */
#include "mutex.h"        /* BAM 내부 뮤텍스 래퍼 */



/*
 * Device descriptor (디바이스 디스크립터)
 *
 * SmartIO를 통해 "빌려온(borrowed)" 원격 NVMe 디바이스에 대한 참조를 보관한다.
 * SISCI의 SCIBorrowDevice()로 획득한 디바이스 핸들과
 * BAR0 메모리 세그먼트 매핑 정보를 함께 관리한다.
 */
struct device
{
    uint32_t                fdid;           /* FDID(Fabric Device Identifier): SmartIO 패브릭 상의 고유 디바이스 식별자 */
    sci_desc_t              sd;             /* SISCI 가상 디바이스 디스크립터: SCIOpen()으로 생성한 세션 핸들 */
    struct mutex            lock;           /* 디바이스에 대한 배타적 접근을 보장하는 뮤텍스 */
    uint32_t                counter;        /* 세그먼트 ID를 순차적으로 부여하기 위한 카운터 */
    sci_smartio_device_t    device;         /* SCIBorrowDevice()로 얻은 SmartIO 디바이스 핸들 */
    sci_remote_segment_t    segment;        /* NVMe 컨트롤러의 BAR0에 해당하는 원격 메모리 세그먼트 */
    size_t                  size;           /* BAR0 세그먼트의 크기(바이트) */
    volatile void*          ptr;            /* BAR0를 로컬 가상 주소 공간에 매핑한 포인터 */
    sci_map_t               md;             /* SISCI 매핑 디스크립터: SCIMapRemoteSegment()의 결과 */
};



/*
 * _nvm_device_memory_get - SmartIO 디바이스의 메모리 세그먼트에 연결한다.
 *
 * SCIConnectDeviceSegment()를 호출하여 원격 디바이스의 특정 메모리 영역(BAR 또는 private 등)에 연결한다.
 * 연결이 성공하면 segment에 원격 세그먼트 핸들이 저장된다.
 *
 * @segment: 연결된 원격 세그먼트 핸들을 받을 포인터
 * @dev:     대상 디바이스
 * @id:      세그먼트 식별자 (0이면 BAR0)
 * @memtype: 메모리 유형 (SCI_MEMTYPE_BAR, SCI_MEMTYPE_PRIVATE, SCI_MEMTYPE_SHARED 등)
 */
int _nvm_device_memory_get(sci_remote_segment_t* segment,
                           const struct device* dev,
                           uint32_t id,
                           unsigned int memtype);



/*
 * _nvm_device_memory_put - 디바이스 메모리 세그먼트 연결을 해제한다.
 *
 * SCIDisconnectSegment()를 반복 호출하여 BUSY 상태가 해소될 때까지 연결 해제를 시도한다.
 *
 * @segment: 해제할 원격 세그먼트 핸들의 포인터
 */
void _nvm_device_memory_put(sci_remote_segment_t* segment);



/*
 * _nvm_local_memory_get - 로컬 SISCI 세그먼트를 생성한다.
 *
 * 호스트 메모리 또는 GPU 메모리를 SISCI 로컬 세그먼트로 등록하여
 * DIS 패브릭을 통해 원격 디바이스가 DMA로 접근할 수 있도록 준비한다.
 *
 * - ptr이 NULL이면: SISCI가 내부적으로 메모리를 할당하는 일반 세그먼트 생성
 * - ptr이 NULL이 아니고 gpu_mem이 false이면: 사용자 메모리를 세그먼트에 등록 (SCIRegisterSegmentMemory)
 * - ptr이 NULL이 아니고 gpu_mem이 true이면: CUDA GPU 메모리를 세그먼트에 부착 (SCIAttachPhysicalMemory)
 *
 * @segment: 생성된 로컬 세그먼트 핸들을 받을 포인터
 * @adapter: 사용된 DIS 어댑터 번호를 받을 포인터
 * @dev:     대상 디바이스 (어댑터 정보 조회용)
 * @size:    세그먼트 크기
 * @ptr:     등록할 메모리 주소 (NULL이면 내부 할당)
 * @gpu_mem: true이면 CUDA GPU 메모리로 취급
 */
int _nvm_local_memory_get(sci_local_segment_t* segment,
                          uint32_t* adapter,
                          const struct device* dev,
                          size_t size,
                          void* ptr,
                          bool gpu_mem);



/*
 * _nvm_local_memory_put - 로컬 SISCI 세그먼트를 제거한다.
 *
 * SCIRemoveSegment()를 반복 호출하여 BUSY 상태가 해소될 때까지 제거를 시도한다.
 *
 * @segment: 제거할 로컬 세그먼트 핸들의 포인터
 */
void _nvm_local_memory_put(sci_local_segment_t* segment);



#endif /* _SISCI */
#endif /* __NVM_INTERNAL_DIS_DEVICE_H__ */
