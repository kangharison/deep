/*
 * ctrl.h - NVM 컨트롤러 관리 헤더.
 * 각 NVMe PCI 디바이스에 대응하는 컨트롤러 구조체와 캐릭터 디바이스 관리 함수를 선언한다.
 * 커널의 cdev/device 서브시스템을 사용하여 /dev/libnvm0 같은 캐릭터 디바이스를 생성한다.
 */
#ifndef __LIBNVM_HELPER_CTRL_H__
#define __LIBNVM_HELPER_CTRL_H__

#include "list.h"
#include <linux/pci.h>     /* struct pci_dev: PCI 디바이스를 나타내는 커널 구조체 */
#include <linux/cdev.h>    /* struct cdev: 캐릭터 디바이스를 커널에 등록하기 위한 구조체 */
#include <linux/fs.h>      /* struct file_operations: 파일 연산 콜백 테이블 */
#include <linux/device.h>  /* struct device, struct class: 디바이스 모델과 sysfs 클래스 */


/*
 * ctrl 구조체 - 하나의 NVMe 컨트롤러를 나타낸다.
 * PCI 디바이스 참조, 캐릭터 디바이스 정보, 연결 리스트 노드를 하나로 묶는다.
 * 커널 모듈이 여러 NVMe 디바이스를 동시에 관리할 수 있도록 연결 리스트로 관리된다.
 */
struct ctrl
{
    struct list_node    list;       /* 컨트롤러 연결 리스트의 노드 (ctrl_list에 삽입된다) */
    struct pci_dev*     pdev;       /* 이 컨트롤러에 대응하는 물리적 PCI 디바이스 참조 */
    char                name[64];   /* 캐릭터 디바이스 이름 (예: "libnvm0") */
    int                 number;     /* 컨트롤러 번호 (0부터 시작, minor 번호 계산에 사용) */
    dev_t               rdev;       /* 캐릭터 디바이스 번호 (major + minor 조합) */
    struct class*       cls;        /* sysfs 디바이스 클래스 참조 (udev가 /dev 노드 생성에 사용) */
    struct cdev         cdev;       /* 커널 캐릭터 디바이스 구조체 (file_operations를 커널에 등록) */
    struct device*      chrdev;     /* device_create()가 반환한 디바이스 핸들 */
};



/*
 * ctrl_get - 새 컨트롤러 참조를 생성하고 리스트에 삽입한다.
 */
struct ctrl* ctrl_get(struct list* list, struct class* cls, struct pci_dev* pdev, int number);



/*
 * ctrl_put - 컨트롤러 참조를 해제하고 리스트에서 제거한다.
 */
void ctrl_put(struct ctrl* ctrl);



/*
 * ctrl_find_by_pci_dev - PCI 디바이스로 컨트롤러를 검색한다.
 */
struct ctrl* ctrl_find_by_pci_dev(const struct list* list, const struct pci_dev* pdev);



/*
 * ctrl_find_by_inode - inode의 cdev 포인터로 컨트롤러를 검색한다.
 * ioctl/mmap 핸들러에서 어떤 컨트롤러에 대한 요청인지 식별할 때 사용한다.
 */
struct ctrl* ctrl_find_by_inode(const struct list* list, const struct inode* inode);



/*
 * ctrl_chrdev_create - 캐릭터 디바이스를 생성하고 /dev에 노드를 만든다.
 * cdev_init() → cdev_add() → device_create() 순서로 커널에 등록한다.
 */
int ctrl_chrdev_create(struct ctrl* ctrl,
                       dev_t first,
                       const struct file_operations* fops);



/*
 * ctrl_chrdev_remove - 캐릭터 디바이스를 제거하고 /dev 노드를 삭제한다.
 */
void ctrl_chrdev_remove(struct ctrl* ctrl);



#endif /* __LIBNVM_HELPER_CTRL_H__ */
