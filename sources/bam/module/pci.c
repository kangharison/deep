/*
 * pci.c - BAM(BaM) 커널 모듈의 메인 파일.
 * NVMe PCI 디바이스를 위한 커널 드라이버로서 다음 기능을 제공한다:
 * 1) NVMe 디바이스를 PCI 드라이버로 등록하여 probe/remove를 처리한다
 * 2) 각 디바이스에 대해 캐릭터 디바이스(/dev/libnvmN)를 생성한다
 * 3) ioctl을 통해 사용자 공간 메모리(호스트/GPU)를 DMA 매핑한다
 * 4) mmap을 통해 NVMe 컨트롤러 레지스터(BAR0)를 사용자 공간에 노출한다
 */
#include "ioctl.h"
#include "list.h"
#include "ctrl.h"
#include "map.h"
#include <linux/module.h>       /* MODULE_* 매크로: 모듈 메타정보 정의 */
#include <linux/moduleparam.h>  /* module_param: 모듈 파라미터(insmod 시 전달) */
#include <linux/kernel.h>       /* printk: 커널 로그 출력 */
#include <linux/init.h>         /* __init, __exit: 초기화/종료 함수 섹션 지정 */
#include <linux/slab.h>         /* kmalloc, kfree */
#include <linux/pci.h>          /* pci_driver, pci_register_driver: PCI 드라이버 프레임워크 */
#include <linux/fs.h>           /* alloc_chrdev_region: 캐릭터 디바이스 번호 할당 */
#include <linux/err.h>          /* IS_ERR, PTR_ERR, ERR_PTR: 에러 처리 매크로 */
#include <linux/device.h>       /* class_create, class_destroy: sysfs 디바이스 클래스 */
#include <linux/uaccess.h>      /* copy_from_user, copy_to_user: 사용자↔커널 데이터 복사 */
#include <asm/io.h>             /* vm_iomap_memory: I/O 메모리를 사용자 공간에 매핑 */
#include <asm/errno.h>
#include <asm/page.h>           /* PAGE_SIZE, PAGE_MASK: 시스템 페이지 크기 상수 */

#define DRIVER_NAME         "libnvm helper"
/* NVMe 디바이스의 PCI 클래스 코드: 01(스토리지) 08(NVMe) 02(NVM 서브클래스) */
#define PCI_CLASS_NVME      0x010802
#define PCI_CLASS_NVME_MASK 0xffffff


/* 모듈 메타정보 정의 (modinfo 명령으로 확인 가능) */
MODULE_AUTHOR("Jonas Markussen <jonassm@ifi.uio.no>");
MODULE_DESCRIPTION("Set up DMA mappings for userspace buffers");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.3");


/*
 * PCI 디바이스 ID 테이블: 이 드라이버가 관심 있는 디바이스를 정의한다.
 * PCI_DEVICE_CLASS 매크로로 NVMe 클래스(0x010802)에 해당하는 모든 디바이스를 매칭한다.
 * 커널의 PCI 서브시스템이 이 테이블을 참조하여 probe 함수를 호출한다.
 */
static const struct pci_device_id id_table[] =
{
    { PCI_DEVICE_CLASS(PCI_CLASS_NVME, PCI_CLASS_NVME_MASK) },
    { 0 }  /* 테이블 종료 마커 (sentinel) */
};


/* 캐릭터 디바이스 번호 범위의 첫 번째 번호 (alloc_chrdev_region으로 동적 할당) */
static dev_t dev_first;


/* sysfs 디바이스 클래스: udev가 이 클래스를 보고 /dev 노드를 생성한다 */
static struct class* dev_class;


/* 등록된 모든 NVMe 컨트롤러의 연결 리스트 */
static struct list ctrl_list;


/* DMA 매핑된 호스트 메모리의 연결 리스트 */
static struct list host_list;


/* DMA 매핑된 GPU 디바이스 메모리의 연결 리스트 */
static struct list device_list;


/* 지원할 최대 컨트롤러 수 (module_param으로 insmod 시 변경 가능) */
static int max_num_ctrls = 64;
module_param(max_num_ctrls, int, 0);  /* module_param: insmod 시 max_num_ctrls=128 같이 전달 가능 */
MODULE_PARM_DESC(max_num_ctrls, "Number of controller devices");

/* 현재까지 등록된 컨트롤러 수 */
static int curr_ctrls = 0;


/*
 * mmap_registers - mmap 파일 연산 핸들러.
 * 사용자 공간에서 mmap()을 호출하면 NVMe 컨트롤러의 BAR0 레지스터 영역을
 * 사용자 공간 가상 주소에 매핑한다. 이를 통해 사용자 공간에서 NVMe 레지스터에 직접 접근할 수 있다.
 */
static int mmap_registers(struct file* file, struct vm_area_struct* vma)
{
    struct ctrl* ctrl = NULL;

    /* 열린 파일의 inode로부터 대응하는 컨트롤러를 찾는다 */
    ctrl = ctrl_find_by_inode(&ctrl_list, file->f_inode);
    if (ctrl == NULL)
    {
        printk(KERN_CRIT "Unknown controller reference\n");
        return -EBADF;
    }

    /* 요청된 매핑 크기가 BAR0 영역 크기를 초과하는지 검증한다 */
    if (vma->vm_end - vma->vm_start > pci_resource_len(ctrl->pdev, 0))
    {
        printk(KERN_WARNING "Invalid range size\n");
        return -EINVAL;
    }

    /* pgprot_noncached: 캐시를 비활성화하여 MMIO 레지스터에 대한 정확한 읽기/쓰기를 보장한다 */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /* vm_iomap_memory: PCI BAR0의 물리 주소를 사용자 공간 VMA에 매핑한다 */
    return vm_iomap_memory(vma, pci_resource_start(ctrl->pdev, 0), vma->vm_end - vma->vm_start);
}



/*
 * map_ioctl - ioctl 파일 연산 핸들러 (unlocked_ioctl).
 * 사용자 공간에서 ioctl()을 호출하면 이 함수가 호출된다.
 * 명령 코드에 따라 호스트 메모리 매핑, GPU 메모리 매핑, 메모리 해제를 처리한다.
 * copy_from_user/copy_to_user로 사용자↔커널 간 데이터를 안전하게 복사한다.
 */
static long map_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    long retval = 0;
    struct ctrl* ctrl = NULL;
    struct nvm_ioctl_map request;   /* 사용자 공간에서 전달받을 매핑 요청 구조체 */
    struct map* map = NULL;
    u64 addr;

    /* 열린 파일의 inode로부터 대응하는 컨트롤러를 찾는다 */
    ctrl = ctrl_find_by_inode(&ctrl_list, file->f_inode);
    if (ctrl == NULL)
    {
        printk(KERN_CRIT "Unknown controller reference\n");
        return -EBADF;
    }

    switch (cmd)
    {
        case NVM_MAP_HOST_MEMORY:
            /* copy_from_user: 사용자 공간 메모리를 커널 공간으로 안전하게 복사한다 (페이지 폴트 처리 포함) */
            if (copy_from_user(&request, (void __user*) arg, sizeof(request)))
            {
                return -EFAULT;  /* 사용자 공간 주소가 유효하지 않으면 EFAULT 반환 */
            }

            /* 사용자 공간 페이지를 pin(고정)하고 DMA 버스 주소로 매핑한다 */
            map = map_userspace(&host_list, ctrl, request.vaddr_start, request.n_pages);

            if (!IS_ERR_OR_NULL(map))
            {
                /* copy_to_user: DMA 버스 주소 배열을 사용자 공간으로 복사한다 */
                if (copy_to_user((void __user*) request.ioaddrs, map->addrs, map->n_addrs * sizeof(uint64_t)))
                {
                    return -EFAULT;
                }
                retval = 0;
            }
            else
            {
                retval = PTR_ERR(map);  /* 매핑 실패 시 에러 코드를 추출한다 */
            }
            break;

#ifdef _CUDA
        case NVM_MAP_DEVICE_MEMORY:
            /* GPU 메모리 매핑 요청: NVIDIA P2P API를 사용하여 GPU 메모리를 DMA 매핑한다 */
            if (copy_from_user(&request, (void __user*) arg, sizeof(request)))
            {
                return -EFAULT;
            }

            /* NVIDIA P2P API로 GPU 메모리를 pin하고 DMA 주소를 얻는다 */
            map = map_device_memory(&device_list, ctrl, request.vaddr_start, request.n_pages, &ctrl_list);

            if (!IS_ERR_OR_NULL(map))
            {
                /* DMA 버스 주소를 사용자 공간에 반환한다 */
                if (copy_to_user((void __user*) request.ioaddrs, map->addrs, map->n_addrs * sizeof(uint64_t)))
                {
                    return -EFAULT;
                }
                retval = 0;
            }
            else
            {
                retval = PTR_ERR(map);
            }
            break;
#endif

        case NVM_UNMAP_MEMORY:
            /* 메모리 해제 요청: 가상 주소를 받아서 해당 매핑을 찾아 해제한다 */
            if (copy_from_user(&addr, (void __user*) arg, sizeof(u64)))
            {
                return -EFAULT;
            }

            /* 먼저 호스트 메모리 매핑 리스트에서 검색한다 */
            map = map_find(&host_list, addr);
            if (map != NULL)
            {
                unmap_and_release(map);  /* DMA 매핑을 해제하고 페이지를 unpin한다 */
                break;
            }

#ifdef _CUDA
            /* 호스트에서 못 찾으면 GPU 메모리 매핑 리스트에서 검색한다 */
            map = map_find(&device_list, addr);
            if (map != NULL)
            {
                unmap_and_release(map);  /* GPU P2P 매핑을 해제한다 */
                break;
            }
#endif
            retval = -EINVAL;
            printk(KERN_WARNING "Mapping for address %llx not found\n", addr);
            break;

        default:
            /* 알 수 없는 ioctl 명령: 프로세스 ID와 명령 코드를 로그에 기록한다 */
            printk(KERN_NOTICE "Unknown ioctl command from process %d: %u\n",
                    current->pid, cmd);
            retval = -EINVAL;
            break;
    }

    return retval;
}



/*
 * dev_fops - 캐릭터 디바이스의 파일 연산 테이블.
 * unlocked_ioctl: DMA 매핑/해제 요청을 처리하는 ioctl 핸들러
 * mmap: NVMe BAR0 레지스터를 사용자 공간에 매핑하는 핸들러
 */
static const struct file_operations dev_fops =
{
    .owner = THIS_MODULE,          /* 이 모듈이 사용 중일 때 언로드를 방지한다 */
    .unlocked_ioctl = map_ioctl,   /* ioctl 시스템콜 핸들러 (BKL 없이 호출됨) */
    .mmap = mmap_registers,        /* mmap 시스템콜 핸들러 */
};


/*
 * add_pci_dev - PCI 디바이스 probe 콜백.
 * 커널이 NVMe 클래스에 해당하는 PCI 디바이스를 발견하면 이 함수를 호출한다.
 * 컨트롤러 참조 생성, PCI 리소스 확보, PCI 디바이스 활성화, 캐릭터 디바이스 생성, DMA 버스 마스터 설정을 수행한다.
 */
static int add_pci_dev(struct pci_dev* dev, const struct pci_device_id* id)
{
    int err;
    struct ctrl* ctrl = NULL;

    /* 최대 컨트롤러 수에 도달하면 더 이상 추가하지 않는다 */
    if (curr_ctrls >= max_num_ctrls)
    {
        printk(KERN_NOTICE "Maximum number of controller devices added\n");
        return 0;
    }

    /* PCI BDF(Bus:Device.Function) 주소를 로그에 출력한다 */
    printk(KERN_INFO "Adding controller device: %02x:%02x.%1x",
            dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));

    /* 컨트롤러 참조를 생성하고 전역 리스트에 삽입한다 */
    ctrl = ctrl_get(&ctrl_list, dev_class, dev, curr_ctrls);
    if (IS_ERR(ctrl))
    {
        return PTR_ERR(ctrl);
    }

    /* pci_request_region: BAR0 메모리 영역을 이 드라이버가 독점 사용하도록 예약한다 */
    err = pci_request_region(dev, 0, DRIVER_NAME);
    if (err != 0)
    {
        ctrl_put(ctrl);
        printk(KERN_ERR "Failed to get controller register memory\n");
        return err;
    }

    /* pci_enable_device: PCI 디바이스를 활성화하고 I/O 및 메모리 접근을 가능하게 한다 */
    err = pci_enable_device(dev);
    if (err < 0)
    {
        pci_release_region(dev, 0);
        ctrl_put(ctrl);
        printk(KERN_ERR "Failed to enable controller\n");
        return err;
    }

    /* 캐릭터 디바이스 파일(/dev/libnvmN)을 생성한다 */
    err = ctrl_chrdev_create(ctrl, dev_first, &dev_fops);
    if (err != 0)
    {
        pci_disable_device(dev);
        pci_release_region(dev, 0);
        ctrl_put(ctrl);
        return err;
    }

    /* pci_set_master: PCI 버스 마스터링을 활성화한다 (디바이스가 DMA를 수행하려면 필수) */
    pci_set_master(dev);

    ++curr_ctrls;
    return 0;
}


/*
 * remove_pci_dev - PCI 디바이스 remove 콜백.
 * 디바이스가 제거되거나 모듈이 언로드될 때 호출된다.
 * probe에서 설정한 모든 리소스를 역순으로 해제한다.
 */
static void remove_pci_dev(struct pci_dev* dev)
{
    struct ctrl* ctrl = NULL;
    printk(KERN_DEBUG DRIVER_NAME " Starting remove_pci_dev\n");
    if (dev == NULL)
    {
        printk(KERN_WARNING "Remove controller device was invoked with NULL\n");
        return;
    }

    --curr_ctrls;

    /* PCI 디바이스로 컨트롤러를 찾아서 제거한다 (캐릭터 디바이스 삭제 포함) */
    ctrl = ctrl_find_by_pci_dev(&ctrl_list, dev);
    ctrl_put(ctrl);

    /* pci_release_region: BAR0 메모리 영역 예약을 해제한다 */
    pci_release_region(dev, 0);

    /* pci_clear_master: 버스 마스터링을 비활성화한다 */
    pci_clear_master(dev);
    /* pci_disable_device: PCI 디바이스를 비활성화한다 */
    pci_disable_device(dev);

    printk(KERN_DEBUG "Controller device removed: %02x:%02x.%1x\n",
            dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));
}


/*
 * clear_map_list - 매핑 리스트의 모든 항목을 해제한다.
 * 모듈 언로드 시 아직 해제되지 않은 DMA 매핑을 강제로 정리할 때 사용한다.
 */
static unsigned long clear_map_list(struct list* list)
{
    unsigned long i = 0;
    struct list_node* ptr = list_next(&list->head);  /* 첫 번째 실제 노드 */
    struct map* map;

    while (ptr != NULL)
    {
        /* container_of로 map 구조체를 얻어 DMA 매핑을 해제한다 */
        map = container_of(ptr, struct map, list);
        unmap_and_release(map);  /* DMA 언매핑 + 페이지 unpin + 메모리 해제 */
        ++i;

        /* unmap_and_release가 노드를 리스트에서 제거하므로 다시 head.next부터 시작한다 */
        ptr = list_next(&list->head);
    }

    return i;  /* 해제된 매핑 수를 반환한다 */
}



/*
 * PCI 드라이버 구조체: 커널 PCI 서브시스템에 등록할 드라이버 정보.
 * probe: 디바이스 발견 시 호출, remove: 디바이스 제거 시 호출
 * id_table: 이 드라이버가 처리할 디바이스 목록
 */
static struct pci_driver driver =
{
    .name = DRIVER_NAME,
    .id_table = id_table,   /* NVMe 클래스 디바이스를 매칭한다 */
    .probe = add_pci_dev,   /* 디바이스 발견 시 호출되는 콜백 */
    .remove = remove_pci_dev,  /* 디바이스 제거 시 호출되는 콜백 */
};


/*
 * libnvm_helper_entry - 모듈 초기화 함수 (__init 섹션).
 * insmod/modprobe로 모듈을 로드할 때 커널이 호출한다.
 * 순서: 리스트 초기화 → 캐릭터 디바이스 번호 할당 → 디바이스 클래스 생성 → PCI 드라이버 등록
 */
static int __init libnvm_helper_entry(void)
{
    int err;

    /* 세 개의 연결 리스트를 초기화한다: 컨트롤러, 호스트 매핑, GPU 매핑 */
    list_init(&ctrl_list);
    list_init(&host_list);
    list_init(&device_list);

    /* alloc_chrdev_region: 사용 가능한 major 번호를 동적으로 할당받는다 */
    err = alloc_chrdev_region(&dev_first, 0, max_num_ctrls, DRIVER_NAME);
    if (err < 0)
    {
        printk(KERN_CRIT "Failed to allocate character device region\n");
        return err;
    }

    /* class_create: sysfs에 디바이스 클래스를 생성한다 (udev가 /dev 노드를 자동 생성하는 데 필요) */
    dev_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(dev_class))
    {
        unregister_chrdev_region(dev_first, max_num_ctrls);  /* 실패 시 할당한 번호 범위를 반환한다 */
        printk(KERN_CRIT "Failed to create character device class\n");
        return PTR_ERR(dev_class);
    }

    /* pci_register_driver: PCI 서브시스템에 드라이버를 등록한다. 이미 존재하는 NVMe 디바이스에 대해 probe가 즉시 호출된다 */
    err = pci_register_driver(&driver);
    if (err != 0)
    {
        class_destroy(dev_class);  /* 실패 시 역순으로 정리한다 */
        unregister_chrdev_region(dev_first, max_num_ctrls);
        printk(KERN_ERR "Failed to register as PCI driver\n");
        return err;
    }

    printk(KERN_DEBUG DRIVER_NAME " loaded\n");
    return 0;
}
module_init(libnvm_helper_entry);  /* module_init 매크로: 모듈 로드 시 호출할 함수를 지정한다 */


/*
 * libnvm_helper_exit - 모듈 종료 함수 (__exit 섹션).
 * rmmod로 모듈을 언로드할 때 커널이 호출한다.
 * 모든 미해제 매핑을 정리하고, PCI 드라이버를 등록 해제하고, 디바이스 클래스와 번호 범위를 반환한다.
 */
static void __exit libnvm_helper_exit(void)
{
    unsigned long remaining = 0;

    /* GPU 메모리 매핑 리스트를 정리한다 (미해제 매핑이 있으면 경고 출력) */
    remaining = clear_map_list(&device_list);
    if (remaining != 0)
    {
        printk(KERN_NOTICE "%lu GPU memory mappings were still in use on unload\n", remaining);
    }

    /* 호스트 메모리 매핑 리스트를 정리한다 */
    remaining = clear_map_list(&host_list);
    if (remaining != 0)
    {
        printk(KERN_NOTICE "%lu host memory mappings were still in use on unload\n", remaining);
    }

    printk(KERN_DEBUG DRIVER_NAME " Before pci_unregister_driver\n");
    /* pci_unregister_driver: PCI 드라이버를 등록 해제한다. 각 디바이스에 대해 remove 콜백이 호출된다 */
    pci_unregister_driver(&driver);
    printk(KERN_DEBUG DRIVER_NAME " After pci_unregister_driver\n");

    class_destroy(dev_class);  /* sysfs 디바이스 클래스를 파괴한다 */
    unregister_chrdev_region(dev_first, max_num_ctrls);  /* 캐릭터 디바이스 번호 범위를 커널에 반환한다 */

    printk(KERN_DEBUG DRIVER_NAME " unloaded\n");
}
module_exit(libnvm_helper_exit);  /* module_exit 매크로: 모듈 언로드 시 호출할 함수를 지정한다 */
