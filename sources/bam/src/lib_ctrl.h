/* lib_ctrl.h - 내부 컨트롤러 핸들 구조체 및 참조 카운팅 헤더
 *
 * NVMe 컨트롤러 핸들의 내부 구현을 정의한다.
 * 외부 API의 nvm_ctrl_t를 감싸는 controller 구조체, 디바이스 콜백(device_ops),
 * 참조 카운팅 함수 등을 선언한다. 이 헤더는 내부 전용이다.
 */
#ifndef __NVM_INTERNAL_CTRL_H__
#define __NVM_INTERNAL_CTRL_H__

#include <nvm_types.h>    // nvm_ctrl_t 타입 정의
#include "mutex.h"        // struct mutex 정의
#include "lib_util.h"     // _nvm_container_of 매크로


/* 디바이스 핸들 전방 선언.
 * 구체적인 구현은 플랫폼별(ioctl, SISCI 등)로 다르다.
 */
struct device;



/* 가상 메모리 주소 범위 전방 선언.
 * DMA 매핑에 사용되는 va_range 구조체이다.
 */
struct va_range;



/* 디바이스 참조 콜백 함수 테이블.
 * 컨트롤러 핸들의 수명 관리와 DMA 매핑에 사용되는 콜백들이다.
 */
struct device_ops
{
    /* release_device - 디바이스 참조를 해제한다 (참조 카운트가 0일 때 호출).
     * BAR0(MLBAR) 메모리 매핑 해제도 이 콜백에서 수행해야 한다.
     */
    void (*release_device)(struct device* dev, volatile void* mm_ptr, size_t mm_size);


    /* map_range - 가상 주소 범위를 디바이스용으로 매핑한다.
     * IOMMU 매핑 등을 수행하고, 결과 버스 주소를 ioaddrs 배열에 채운다.
     */
    int (*map_range)(const struct device* dev, const struct va_range* va, uint64_t* ioaddrs);


    /* unmap_range - 디바이스용 주소 매핑을 해제한다.
     * map_range의 역연산을 수행한다.
     */
    void (*unmap_range)(const struct device* dev, const struct va_range* va);
};



/* 컨트롤러 디바이스 타입 열거형.
 * 컨트롤러 핸들이 어떤 방식으로 초기화되었는지를 나타낸다.
 */
enum device_type
{
    DEVICE_TYPE_UNKNOWN =   0x00,       /* 사용자가 수동으로 메모리 매핑한 경우 */
    DEVICE_TYPE_IOCTL   =   0x01,       /* UNIX 파일 디스크립터(ioctl)를 통해 매핑한 경우 */
    DEVICE_TYPE_SMARTIO =   0x02,       /* SISCI SmartIO API를 통해 매핑한 경우 */
};



/* 내부 컨트롤러 핸들 구조체.
 * 외부 API의 nvm_ctrl_t를 handle 멤버로 포함하며, 참조 카운팅과 디바이스 콜백을 관리한다.
 * _nvm_container_of 매크로로 nvm_ctrl_t*에서 이 구조체를 역추적할 수 있다.
 */
struct controller
{
    struct mutex                lock;           /* 참조 카운트 보호용 뮤텍스 */
    uint32_t                    count;          /* 참조 카운트 (0이 되면 해제) */
    enum device_type            type;           /* 디바이스 타입 (초기화 방식) */
    struct device*              device;         /* 플랫폼별 디바이스 핸들 */
    struct device_ops           ops;            /* 디바이스 콜백 함수 테이블 */
    nvm_ctrl_t                  handle;         /* 사용자에게 노출되는 컨트롤러 핸들 */
};


/* _nvm_ctrl_init - BAR 레지스터를 읽어 컨트롤러 핸들을 초기화한다.
 * CAP 레지스터에서 페이지 크기, 타임아웃, 도어벨 스트라이드, 최대 큐 크기를 읽는다.
 */
int _nvm_ctrl_init(nvm_ctrl_t** handle,             /* 출력: 사용자 핸들 */
                   struct device* dev,              /* 플랫폼별 디바이스 핸들 */
                   const struct device_ops* ops,    /* 디바이스 콜백 함수 테이블 */
                   enum device_type type,           /* 디바이스 타입 */
                   volatile void* mm_ptr,           /* BAR0 메모리 매핑 포인터 */
                   size_t mm_size);                 /* BAR0 매핑 크기 */



/* _nvm_ctrl_get - 컨트롤러 참조 카운트를 증가시킨다.
 * 핸들을 사용하는 동안 해제되지 않도록 보장한다.
 */
struct controller* _nvm_ctrl_get(const nvm_ctrl_t* handle);



/* _nvm_ctrl_put - 컨트롤러 참조 카운트를 감소시킨다.
 * 참조 카운트가 0이 되면 디바이스 리소스를 해제하고 메모리를 반환한다.
 */
void _nvm_ctrl_put(struct controller* ctrl);



/* _nvm_ctrl_type - 컨트롤러의 디바이스 타입을 조회하는 편의 매크로.
 * nvm_ctrl_t 포인터에서 container_of로 controller를 역추적하여 type을 반환한다.
 */
#define _nvm_ctrl_type(ctrl) _nvm_container_of(ctrl, struct controller, handle)->type

#endif /* __NVM_INTERNAL_CTRL_H__ */
