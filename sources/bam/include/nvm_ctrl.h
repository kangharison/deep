/* nvm_ctrl.h - NVMe 컨트롤러 초기화, 리셋, 해제 API 헤더 */
/* 컨트롤러 핸들 생성 및 관리를 위한 함수들을 선언한다 */
#ifndef __NVM_CTRL_H__
#define __NVM_CTRL_H__
// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <nvm_types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* SmartIO(DIS 클러스터) 환경에서 SISCI 타입을 포함 */
#ifdef __DIS_CLUSTER__
#include <sisci_types.h>
#endif



/*
 * Minimum size of mapped controller memory.
 * 컨트롤러 메모리 맵 최소 크기 (0x2000 = 8KB).
 * BAR0의 NVMe 레지스터 영역을 읽으려면 최소 이 크기만큼 매핑해야 한다.
 * 이 크기에는 기본 레지스터(CAP, VS, CC 등)와 최소한의 도어벨 레지스터가 포함된다.
 */
#define NVM_CTRL_MEM_MINSIZE                        0x2000



#if defined (__unix__)
/*
 * Initialize NVM controller handle.
 * NVMe 컨트롤러 핸들을 초기화한다.
 *
 * 컨트롤러 레지스터를 읽어 핸들을 초기화한다.
 * 커널 모듈 사용 시 또는 sysfs에서 수동으로 읽을 때 이 함수를 사용한다.
 * fd는 커널 모듈의 디바이스 파일 디스크립터이다.
 *
 * Read from controller registers and initialize controller handle.
 * This function should be used when using the kernel module or to manually
 * read from sysfs.
 *
 * Note: fd must be opened with O_RDWR and O_NONBLOCK
 */
int nvm_ctrl_init(nvm_ctrl_t** ctrl,  /* [out] 초기화된 컨트롤러 핸들 포인터를 받을 이중 포인터 */
                  int fd);             /* 커널 모듈 디바이스 파일의 파일 디스크립터 (O_RDWR | O_NONBLOCK으로 열어야 함) */
#endif



/*
 * Initialize NVM controller handle.
 * 메모리 맵 포인터를 사용하여 NVMe 컨트롤러 핸들을 초기화한다.
 *
 * PCI 디바이스 BAR에 대한 메모리 맵 포인터로 컨트롤러 핸들을 초기화한다.
 * SmartIO나 disnvme 커널 모듈을 사용하지 않을 때 이 함수를 사용한다.
 * 예를 들어 VFIO를 통해 BAR0을 직접 mmap한 경우에 해당한다.
 *
 * Read from controller registers and initialize the controller handle using
 * a memory-mapped pointer to the PCI device BAR.
 *
 * This function should be used when neither SmartIO nor the disnvme kernel
 * module are used.
 *
 * Note: ctrl_mem must be at least NVM_CTRL_MEM_MINSIZE large and mapped
 *       as IO memory. See arguments for mmap() for more info.
 */
int nvm_raw_ctrl_init(nvm_ctrl_t** ctrl,       /* [out] 초기화된 컨트롤러 핸들 포인터 */
                      volatile void* mm_ptr,    /* BAR0에 대한 메모리 맵 포인터 (최소 NVM_CTRL_MEM_MINSIZE 크기) */
                      size_t mm_size);           /* 메모리 맵 영역의 크기 */



/*
 * Release controller handle.
 * 컨트롤러 핸들을 해제한다.
 * nvm_ctrl_init 또는 nvm_raw_ctrl_init으로 할당된 메모리를 반환한다.
 */
void nvm_ctrl_free(nvm_ctrl_t* ctrl);  /* 해제할 컨트롤러 핸들 포인터 */



/*
 * Reset NVM controller.
 * NVMe 컨트롤러를 리셋한다.
 *
 * 큐 메모리는 0으로 초기화되어야 하며, 정확히 한 페이지 크기여야 한다.
 * IO 주소는 컨트롤러 페이지 크기에 맞게 정렬되어야 한다.
 * CC.EN을 0으로 설정하여 컨트롤러를 비활성화한 후, ACQ/ASQ 주소를 설정하고 다시 활성화한다.
 *
 * The queue memory must be memset to zero and be exactly one page size large.
 * IO addresses must align to the controller page size.
 *
 * Note: The controller must be unbound from any driver before attempting to
 *       reset the controller.
 *
 * Note: This function is implicitly called by the controller manager, so it
 *       should not be necessary to call it directly.
 */
int nvm_raw_ctrl_reset(const nvm_ctrl_t* ctrl,  /* 리셋할 컨트롤러 핸들 */
                       uint64_t acq_ioaddr,      /* Admin Completion Queue의 물리/IO 주소 */
                       uint64_t asq_ioaddr);     /* Admin Submission Queue의 물리/IO 주소 */



#ifdef __DIS_CLUSTER__
/*
 * Initialize NVM controller handle.
 * SmartIO를 사용하여 NVMe 컨트롤러 핸들을 초기화한다.
 *
 * SmartIO의 FDID(Fabric Device ID)를 통해 원격 NVMe 디바이스에 접근한다.
 *
 * Read from device registers and initialize controller handle.
 * This function should be used when SmartIO is being used.
 */
int nvm_dis_ctrl_init(nvm_ctrl_t** ctrl,       /* [out] 초기화된 컨트롤러 핸들 포인터 */
                      uint32_t smartio_fdid);   /* SmartIO Fabric Device ID */
#endif



#ifdef __DIS_CLUSTER__
/* SmartIO 디바이스에 대한 P2P(Peer-to-Peer) 매핑을 생성한다 */
/* 원격 디바이스의 메모리를 로컬 컨트롤러가 직접 접근할 수 있도록 매핑한다 */
int nvm_dis_ctrl_map_p2p_device(const nvm_ctrl_t* ctrl,     /* 컨트롤러 핸들 */
                                sci_smartio_device_t dev,    /* SmartIO 디바이스 핸들 */
                                uint64_t* ioaddr);           /* [out] 매핑된 IO 주소 */
#endif



#ifdef __DIS_CLUSTER__
/* SmartIO 디바이스에 대한 P2P 매핑을 해제한다 */
void nvm_dis_ctrl_unmap_p2p_device(const nvm_ctrl_t* ctrl,  /* 컨트롤러 핸들 */
                                   sci_smartio_device_t dev); /* 매핑 해제할 SmartIO 디바이스 */
#endif


#endif /* __NVM_CTRL_H__ */
