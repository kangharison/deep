/*
 * ctrl.c - NVM 컨트롤러 관리 구현.
 * 각 NVMe PCI 디바이스에 대응하는 컨트롤러 구조체를 생성/삭제하고,
 * 커널 캐릭터 디바이스(/dev/libnvmN)를 등록/해제하는 기능을 제공한다.
 */
#include "ctrl.h"
#include "list.h"
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>      /* cdev_init, cdev_add, cdev_del: 캐릭터 디바이스 등록/해제 함수 */
#include <linux/device.h>  /* device_create, device_destroy: /dev 노드 생성/삭제 함수 */
#include <linux/slab.h>    /* kmalloc, kfree: 커널 메모리 할당/해제 */
#include <asm/errno.h>



/*
 * ctrl_get - 새 컨트롤러 참조를 생성하여 리스트에 삽입한다.
 * PCI 프로브 시 호출되며, NVMe 디바이스 하나당 ctrl 구조체 하나를 만든다.
 * kmalloc으로 커널 힙에 할당하고, GFP_NOWAIT 플래그로 슬립 없이 할당을 시도한다.
 */
struct ctrl* ctrl_get(struct list* list, struct class* cls, struct pci_dev* pdev, int number)
{
    struct ctrl* ctrl = NULL;

    /* GFP_KERNEL | GFP_NOWAIT: 커널 컨텍스트에서 non-blocking 할당을 시도한다 */
    ctrl = kmalloc(sizeof(struct ctrl), GFP_KERNEL | GFP_NOWAIT);
    if (ctrl == NULL)
    {
        printk(KERN_CRIT "Failed to allocate controller reference\n");
        return ERR_PTR(-ENOMEM);  /* ERR_PTR: 에러 코드를 포인터로 인코딩하는 커널 관용구 */
    }

    list_node_init(&ctrl->list);  /* 연결 리스트 노드를 초기화한다 (prev/next를 NULL로) */

    ctrl->pdev = pdev;        /* 물리적 PCI 디바이스 참조를 저장한다 */
    ctrl->number = number;    /* 컨트롤러 번호를 설정한다 (minor 번호 계산에 사용) */
    ctrl->rdev = 0;           /* 캐릭터 디바이스 번호를 0으로 초기화한다 */
    ctrl->cls = cls;          /* sysfs 디바이스 클래스 참조를 저장한다 */
    ctrl->chrdev = NULL;      /* 아직 캐릭터 디바이스가 생성되지 않았음을 표시한다 */

    /* 디바이스 이름을 생성한다 (예: "libnvm0", "libnvm1") */
    snprintf(ctrl->name, sizeof(ctrl->name), "%s%d", KBUILD_MODNAME, ctrl->number);
    ctrl->name[sizeof(ctrl->name) - 1] = '\0';  /* 버퍼 오버플로 방지를 위해 null 종료를 보장한다 */

    list_insert(list, &ctrl->list);  /* 컨트롤러를 전역 리스트에 삽입한다 */

    return ctrl;
}



/*
 * ctrl_put - 컨트롤러 참조를 해제한다.
 * 리스트에서 제거하고 캐릭터 디바이스를 삭제한 뒤 메모리를 해제한다.
 */
void ctrl_put(struct ctrl* ctrl)
{
    if (ctrl != NULL)
    {
        list_remove(&ctrl->list);    /* 연결 리스트에서 이 컨트롤러를 제거한다 */
        ctrl_chrdev_remove(ctrl);    /* 캐릭터 디바이스(/dev 노드)를 제거한다 */
        kfree(ctrl);                 /* 컨트롤러 구조체 메모리를 커널에 반환한다 */
    }
}



/*
 * ctrl_find_by_pci_dev - PCI 디바이스 포인터로 컨트롤러를 검색한다.
 * PCI remove 콜백에서 제거할 컨트롤러를 찾을 때 사용한다.
 * 리스트를 순회하면서 pdev 포인터가 일치하는 항목을 반환한다.
 */
struct ctrl* ctrl_find_by_pci_dev(const struct list* list, const struct pci_dev* pdev)
{
    const struct list_node* element = list_next(&list->head);  /* 첫 번째 실제 노드를 가져온다 (head는 더미) */
    struct ctrl* ctrl;

    while (element != NULL)
    {
        /* container_of: list_node 멤버로부터 감싸고 있는 ctrl 구조체를 역추적한다 */
        ctrl = container_of(element, struct ctrl, list);

        if (ctrl->pdev == pdev)  /* PCI 디바이스 포인터가 일치하면 해당 컨트롤러를 반환한다 */
        {
            return ctrl;
        }

        element = list_next(element);  /* 다음 노드로 이동한다 */
    }

    return NULL;  /* 일치하는 컨트롤러를 찾지 못했다 */
}



/*
 * ctrl_find_by_inode - inode의 cdev 포인터로 컨트롤러를 검색한다.
 * ioctl/mmap 파일 연산 핸들러에서 요청이 어떤 컨트롤러에 대한 것인지 식별할 때 사용한다.
 * 사용자가 /dev/libnvmN을 열면 커널이 해당 파일의 inode에 cdev 참조를 설정하므로,
 * 이를 비교하여 올바른 컨트롤러를 찾을 수 있다.
 */
struct ctrl* ctrl_find_by_inode(const struct list* list, const struct inode* inode)
{
    const struct list_node* element = list_next(&list->head);
    struct ctrl* ctrl;

    while (element != NULL)
    {
        ctrl = container_of(element, struct ctrl, list);

        /* inode->i_cdev는 열린 캐릭터 디바이스의 cdev 포인터이다 */
        if (&ctrl->cdev == inode->i_cdev)
        {
            return ctrl;
        }

        element = list_next(element);
    }

    return NULL;
}



/*
 * ctrl_chrdev_create - 캐릭터 디바이스를 생성하여 /dev에 노드를 만든다.
 * 순서: cdev_init(cdev에 file_operations 연결) → cdev_add(커널에 등록) → device_create(/dev 노드 생성)
 * 이렇게 생성된 /dev/libnvmN 파일을 사용자 공간에서 열어 ioctl/mmap으로 통신한다.
 */
int ctrl_chrdev_create(struct ctrl* ctrl, dev_t first, const struct file_operations* fops)
{
    int err;
    struct device* chrdev = NULL;

    /* 이미 캐릭터 디바이스가 생성되어 있으면 중복 생성을 방지한다 */
    if (ctrl->chrdev != NULL)
    {
        printk(KERN_WARNING "Character device is already created\n");
        return 0;
    }

    /* MKDEV: major 번호와 (base minor + 컨트롤러 번호)를 조합하여 디바이스 번호를 만든다 */
    ctrl->rdev = MKDEV(MAJOR(first), MINOR(first) + ctrl->number);

    /* cdev_init: cdev 구조체를 초기화하고 file_operations(ioctl, mmap 핸들러)를 연결한다 */
    cdev_init(&ctrl->cdev, fops);
    /* cdev_add: 초기화된 cdev를 커널의 캐릭터 디바이스 테이블에 등록한다 (1개 minor 번호) */
    err = cdev_add(&ctrl->cdev, ctrl->rdev, 1);
    if (err != 0)
    {
        printk(KERN_ERR "Failed to add cdev\n");
        return err;
    }

    /* device_create: sysfs에 디바이스를 등록하고 udev가 /dev/libnvmN 노드를 자동 생성하게 한다 */
    chrdev = device_create(ctrl->cls, NULL, ctrl->rdev, NULL, ctrl->name);
    if (IS_ERR(chrdev))  /* IS_ERR: 포인터가 에러 코드인지 확인하는 커널 매크로 */
    {
        cdev_del(&ctrl->cdev);  /* 실패 시 이미 등록한 cdev를 제거한다 */
        printk(KERN_ERR "Failed to create character device\n");
        return PTR_ERR(chrdev);  /* PTR_ERR: 에러 포인터에서 에러 코드를 추출한다 */
    }

    ctrl->chrdev = chrdev;  /* 생성된 디바이스 핸들을 저장한다 */

    printk(KERN_INFO "Character device /dev/%s created (%d.%d)\n",
            ctrl->name, MAJOR(ctrl->rdev), MINOR(ctrl->rdev));

    return 0;
}



/*
 * ctrl_chrdev_remove - 캐릭터 디바이스를 제거한다.
 * device_destroy: /dev 노드와 sysfs 항목을 제거한다.
 * cdev_del: 커널 캐릭터 디바이스 테이블에서 cdev를 제거한다.
 */
void ctrl_chrdev_remove(struct ctrl* ctrl)
{
    if (ctrl->chrdev != NULL)
    {
        device_destroy(ctrl->cls, ctrl->rdev);  /* sysfs 디바이스를 파괴하고 /dev 노드를 삭제한다 */
        cdev_del(&ctrl->cdev);                   /* cdev를 커널에서 등록 해제한다 */
        ctrl->chrdev = NULL;                     /* 핸들을 NULL로 설정하여 제거 완료를 표시한다 */

        printk(KERN_DEBUG "Character device /dev/%s removed (%d.%d)\n",
                ctrl->name, MAJOR(ctrl->rdev), MINOR(ctrl->rdev));
    }
}
