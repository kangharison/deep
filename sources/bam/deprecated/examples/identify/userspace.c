/*
 * =============================================================================
 * NVMe Identify 유저스페이스 백엔드 (deprecated/examples/identify/userspace.c)
 * =============================================================================
 *
 * [목적]
 * 커널 모듈 없이 순수 유저스페이스에서 NVMe 컨트롤러에 직접 접근하여 Identify 명령을 수행한다.
 *
 * [Deprecated 배경]
 * libnvm 라이브러리의 초기 예제 코드. BaM이 커널 모듈(libnvm.ko) 기반 GPU-initiated I/O로 전환되면서 deprecated.
 *
 * [BaM 아키텍처에서의 위치]
 * deprecated/examples/identify/ 디렉토리. 세 가지 접근 방식 중 하나:
 * - userspace.c: PCIe sysfs로 직접 BAR0 mmap (이 파일)
 * - module.c: 커널 모듈 경유 (/dev/libnvmXXX)
 * - smartio.c: SISCI/SmartIO DIS 클러스터 경유 (원격 접근)
 *
 * [유저스페이스 직접 접근 방식]
 * 1. sysfs를 통해 PCI 디바이스를 활성화하고 Bus Master DMA를 설정
 * 2. PCIe BAR0 리소스 파일을 mmap하여 NVMe 레지스터에 직접 접근
 * 3. /proc/self/pagemap을 통해 사용자 메모리의 물리 주소(I/O 주소)를 조회
 * 4. 조회된 물리 주소로 DMA 윈도우를 생성하고 Admin Queue를 설정
 * 5. Identify Controller/Namespace 명령을 수행
 *
 * [한계]
 * - IOMMU가 활성화된 환경에서는 물리 주소와 IOMMU 주소가 다를 수 있어 DMA 실패 가능
 * - /proc/self/pagemap 접근에 root 권한 필요
 * - 커널 NVMe 드라이버와 충돌 가능 (unbind 필요)
 */
#include <nvm_util.h>     // NVM_CTRL_MEM_MINSIZE, NVM_DMA_OFFSET 등 유틸리티 매크로
#include <stdio.h>        // fprintf, fopen, fread, fseek 등
#include <stdlib.h>       // exit, posix_memalign
#include <stddef.h>       // size_t
#include <stdint.h>       // uint64_t, uint32_t 등
#include <errno.h>        // errno, EINVAL
#include <string.h>       // strerror, strrchr, strchr, memset
#include <getopt.h>       // getopt_long, struct option
#include <stdbool.h>      // bool, true, false
#include <nvm_types.h>    // libnvm 기본 타입 (nvm_ctrl_t, nvm_dma_t)
#include <nvm_ctrl.h>     // nvm_raw_ctrl_init, nvm_ctrl_free
#include <nvm_dma.h>      // nvm_dma_map, nvm_dma_unmap
#include <nvm_aq.h>       // nvm_aq_create, nvm_aq_destroy
#include <nvm_admin.h>    // nvm_admin_ctrl_info, nvm_admin_ns_info
#include <unistd.h>       // sysconf, close
#include <fcntl.h>        // open, O_RDWR
#include <sys/mman.h>     // mmap, munmap, mlock, munlock, MAP_SHARED
#include <sys/stat.h>     // stat, S_ISDIR
#include "common.h"       // reset_ctrl, identify_ctrl, identify_ns


/*
 * bdf - PCIe Bus/Device/Function 식별자 구조체
 *
 * @domain:   PCIe 도메인 번호 (보통 0x0000)
 * @bus:      PCIe 버스 번호 (0x00~0xFF)
 * @device:   PCIe 디바이스 번호 (0x00~0x1F)
 * @function: PCIe 펑션 번호 (0x0~0x7)
 *
 * sysfs 경로 형식: /sys/bus/pci/devices/DDDD:BB:DD.F/
 */
struct bdf
{
    int     domain;      // PCIe 도메인 (보통 0)
    int     bus;         // PCIe 버스 번호
    int     device;      // PCIe 디바이스(슬롯) 번호
    int     function;    // PCIe 펑션 번호 (멀티펑션 디바이스용)
};



/*
 * lookup_ioaddrs - /proc/self/pagemap에서 가상 주소의 물리 주소를 조회
 *
 * @ptr:       물리 주소를 조회할 가상 메모리 시작 주소
 * @page_size: 시스템 페이지 크기 (보통 4KB)
 * @n_pages:   조회할 페이지 수
 * @ioaddrs:   결과 물리 주소 배열 (출력)
 * @return:    성공 시 0, 실패 시 errno
 *
 * [/proc/self/pagemap 형식 (Linux)]
 * 각 가상 페이지에 대해 8바이트 엔트리:
 * - 비트 63: 페이지 present 플래그
 * - 비트 0~54: 페이지 프레임 번호 (PFN)
 * 물리 주소 = PFN * page_size
 *
 * [주의] mlock()으로 페이지를 물리 메모리에 고정해야 정확한 PFN을 얻을 수 있다.
 */
static int lookup_ioaddrs(void* ptr, size_t page_size, size_t n_pages, uint64_t* ioaddrs)
{
    uint64_t vaddr = (uint64_t) ptr;  // 가상 주소를 정수로 변환
    /* pagemap에서 이 가상 주소에 해당하는 오프셋 계산: (가상 페이지 번호) * 8바이트 */
    size_t offset = (vaddr / page_size) * sizeof(void*);  // 각 pagemap 엔트리는 8바이트(64비트)

    FILE* fp = fopen("/proc/self/pagemap", "r");  // 현재 프로세스의 pagemap 열기 (root 권한 필요)
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open page map: %s\n", strerror(errno));
        return errno;
    }

    if (fseek(fp, offset, SEEK_SET) != 0)  // pagemap에서 해당 가상 주소 엔트리 위치로 이동
    {
        fclose(fp);
        fprintf(stderr, "Failed to seek: %s\n", strerror(errno));
        return errno;
    }

    if (fread(ioaddrs, sizeof(uint64_t), n_pages, fp) != n_pages)  // n_pages개의 pagemap 엔트리 읽기
    {
        fclose(fp);
        fprintf(stderr, "Failed to read: %s\n", strerror(errno));
        return errno;
    }

    fclose(fp);  // pagemap 파일 닫기

    for (size_t i_page = 0; i_page < n_pages; ++i_page)  // 각 페이지의 pagemap 엔트리를 물리 주소로 변환
    {
        /* 비트 63이 0이면 페이지가 물리 메모리에 없음 (swap 등) */
        if (!(ioaddrs[i_page] & (1ULL << 63)))  // present 비트 확인
        {
            fprintf(stderr, "Page not present in memory!\n");
            return EINVAL;  // 페이지가 물리 메모리에 없으면 DMA 불가
        }

        /* 비트 0~53에서 PFN 추출 후 page_size를 곱하여 물리 주소로 변환 */
        ioaddrs[i_page] = (ioaddrs[i_page] & ((1ULL << 54) - 1)) * page_size;  // PFN * page_size = 물리 주소
    }

    return 0;  // 성공
}



/*
 * identify - Identify Controller/Namespace 명령 실행 (유저스페이스 DMA 매핑)
 *
 * @ctrl:       NVMe 컨트롤러 핸들 (BAR0 mmap으로 생성)
 * @nvm_ns_id:  네임스페이스 ID (0이면 Namespace Identify 생략)
 * @return:     성공 시 0, 실패 시 에러 코드
 *
 * [DMA 메모리 할당 절차]
 * 1. posix_memalign()으로 3페이지 페이지 정렬 메모리 할당
 * 2. mlock()으로 물리 메모리에 고정 (스왑 방지)
 * 3. /proc/self/pagemap에서 물리 주소 조회
 * 4. nvm_dma_map()으로 DMA 윈도우 생성 (물리 주소 직접 지정)
 *    - 커널 모듈의 nvm_dma_map_host()와 달리 물리 주소를 직접 제공
 * 5. Admin Queue 생성 -> Identify 실행
 * 6. 리소스 역순 해제 (munlock 포함)
 */
static int identify(const nvm_ctrl_t* ctrl, uint32_t nvm_ns_id)
{
    int status;              // 함수 반환값
    void* memory;            // posix_memalign으로 할당한 3페이지 메모리
    nvm_dma_t* window = NULL;   // DMA 윈도우 핸들
    nvm_aq_ref admin = NULL;    // Admin Queue 참조
    uint64_t ioaddrs[3];        // 3페이지의 물리 주소 배열

    long page_size = sysconf(_SC_PAGESIZE);  // 시스템 페이지 크기 조회 (보통 4096)
    if (page_size == -1)
    {
        fprintf(stderr, "Failed to look up page size: %s\n", strerror(errno));
        return 1;
    }

    /* 3페이지 페이지 정렬 메모리 할당 (페이지0=ACQ, 페이지1=ASQ, 페이지2=Identify 버퍼) */
    status = posix_memalign(&memory, ctrl->page_size, 3 * page_size);  // 컨트롤러 페이지 크기로 정렬
    if (status != 0)
    {
        fprintf(stderr, "Failed to allocate page-aligned memory: %s\n", strerror(status));
        return 1;
    }

    /* mlock: 물리 메모리에 고정하여 DMA 중 스왑 아웃 방지 */
    status = mlock(memory, 3 * page_size);  // 3페이지를 물리 메모리에 pin
    if (status != 0)
    {
        free(memory);  // 할당한 메모리 해제
        fprintf(stderr, "Failed to page-lock memory: %s\n", strerror(status));
        return 1;
    }

    /* /proc/self/pagemap에서 3페이지의 물리 주소 조회 */
    status = lookup_ioaddrs(memory, page_size, 3, ioaddrs);  // 가상→물리 주소 변환
    if (status != 0)
    {
        munlock(memory, 3 * page_size);  // 물리 메모리 pin 해제
        free(memory);                    // 메모리 해제
        goto out;
    }

    /* 물리 주소를 직접 지정하여 DMA 윈도우 생성 (IOMMU 우회) */
    status = nvm_dma_map(&window, ctrl, memory, page_size, 3, ioaddrs);  // 가상주소+물리주소로 DMA 윈도우 매핑
    if (status != 0)
    {
        fprintf(stderr, "Failed to create DMA window: %s\n", strerror(status));
        status = 2;
        goto out;
    }

    /* 컨트롤러 리셋 및 Admin Queue 생성 (DMA 윈도우의 페이지 0=ACQ, 페이지 1=ASQ) */
    admin = reset_ctrl(ctrl, window);  // CC.EN=0 -> AQA/ASQ/ACQ 설정 -> CC.EN=1
    if (admin == NULL)
    {
        goto out;
    }

    /* Identify Controller 실행 (페이지 2를 Identify 데이터 버퍼로 사용) */
    status = identify_ctrl(admin, NVM_DMA_OFFSET(window, 2), window->ioaddrs[2]);  // 페이지2의 가상주소와 IO주소 전달
    if (status != 0)
    {
        goto out;
    }

    /* 네임스페이스 ID가 지정된 경우 Identify Namespace도 실행 */
    if (nvm_ns_id != 0)
    {
        status = identify_ns(admin, nvm_ns_id, NVM_DMA_OFFSET(window, 2), window->ioaddrs[2]);  // 같은 버퍼 재사용
    }

out:
    nvm_aq_destroy(admin);               // Admin Queue 파괴 (NULL-safe)
    nvm_dma_unmap(window);               // DMA 윈도우 해제 (NULL-safe)
    munlock(memory, 3 * page_size);      // 물리 메모리 pin 해제
    free(memory);                        // 할당한 메모리 해제
    return status;
}


/*
 * pci_enable_device - sysfs를 통해 PCI 디바이스 활성화
 *
 * @dev:    PCI BDF 식별자
 * @return: 성공 시 0, 실패 시 errno
 *
 * /sys/bus/pci/devices/DDDD:BB:DD.F/enable에 '1'을 기록하여 디바이스를 활성화.
 * 커널 드라이버가 unbind된 디바이스를 수동으로 활성화할 때 필요하다.
 */
static int pci_enable_device(const struct bdf* dev)
{
    char path[64];  // sysfs enable 파일 경로 버퍼
    sprintf(path, "/sys/bus/pci/devices/%04x:%02x:%02x.%x/enable",
            dev->domain, dev->bus, dev->device, dev->function);  // DDDD:BB:DD.F 형식으로 경로 구성

    FILE *fp = fopen(path, "w");  // enable 파일을 쓰기 모드로 열기
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open file descriptor: %s\n", strerror(errno));
        return errno;
    }

    fputc('1', fp);  // '1'을 기록하여 디바이스 활성화 (PCI Configuration Space의 Command 레지스터 설정)
    fclose(fp);       // 파일 닫기
    return 0;
}


/*
 * pci_set_bus_master - PCIe Command 레지스터에서 Bus Master 비트 설정
 *
 * @dev:    PCI BDF 식별자
 * @return: 성공 시 0, 실패 시 errno
 *
 * PCIe Configuration Space 오프셋 0x04 (Command 레지스터)의
 * 비트 2 (Bus Master Enable)를 1로 설정하여 디바이스가 DMA를 수행할 수 있게 한다.
 * NVMe 컨트롤러가 호스트 메모리의 SQ/CQ에 접근하려면 Bus Master가 반드시 활성화되어야 한다.
 */
static int pci_set_bus_master(const struct bdf* dev)
{
    char path[64];  // sysfs config 파일 경로 버퍼
    sprintf(path, "/sys/bus/pci/devices/%04x:%02x:%02x.%x/config",
            dev->domain, dev->bus, dev->device, dev->function);  // PCIe Config Space 파일 경로

    FILE* fp = fopen(path, "r+");  // Config Space 파일을 읽기+쓰기 모드로 열기
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open config space file: %s\n", strerror(errno));
        return errno;
    }

    /* PCIe Command 레지스터 (오프셋 0x04, 16비트) 읽기 */
    uint16_t command;
    fseek(fp, 0x04, SEEK_SET);                     // Command 레지스터 오프셋으로 이동
    fread(&command, sizeof(command), 1, fp);        // 현재 Command 레지스터 값 읽기

    /* 비트 2 (Bus Master Enable) 설정: 디바이스가 PCIe 버스에서 DMA를 개시할 수 있게 허용 */
    command |= (1 << 0x02);  // 비트 2를 OR로 설정 (기존 비트 유지)

    /* 수정된 Command 레지스터 기록 */
    fseek(fp, 0x04, SEEK_SET);                     // 다시 Command 레지스터 오프셋으로 이동
    fwrite(&command, sizeof(command), 1, fp);       // Bus Master 비트가 설정된 값 기록

    fclose(fp);  // Config Space 파일 닫기
    return 0;
}


/*
 * pci_open_bar - PCI 디바이스의 BAR(Base Address Register) 리소스 파일 열기
 *
 * @dev:    PCI BDF 식별자
 * @bar:    BAR 번호 (NVMe는 BAR0 사용)
 * @return: 성공 시 파일 디스크립터, 실패 시 -1
 *
 * /sys/bus/pci/devices/DDDD:BB:DD.F/resource0을 O_RDWR로 열어
 * mmap()으로 디바이스 레지스터에 직접 접근할 수 있게 한다.
 * NVMe BAR0는 NVMe 레지스터(CAP, CC, CSTS, AQA, ASQ, ACQ 등)를 포함한다.
 */
static int pci_open_bar(const struct bdf* dev, int bar)
{
    char path[64];  // sysfs resource 파일 경로 버퍼
    sprintf(path, "/sys/bus/pci/devices/%04x:%02x:%02x.%x/resource%d",
            dev->domain, dev->bus, dev->device, dev->function, bar);  // BAR 리소스 파일 경로

    int fd = open(path, O_RDWR);  // BAR 리소스 파일을 읽기+쓰기로 열기 (mmap 대상)
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open resource file: %s\n", strerror(errno));
    }

    return fd;  // 파일 디스크립터 반환 (실패 시 -1)
}


static void parse_args(int argc, char** argv, struct bdf* device, uint32_t* nvm_ns_id);  // 전방 선언: 커맨드라인 파서


/*
 * main - 유저스페이스 직접 접근으로 NVMe Identify 실행
 *
 * [실행 흐름]
 * 1. 커맨드라인 파싱 (--ctrl <PCI BDF>, --ns <namespace>)
 * 2. PCI 디바이스 활성화 (sysfs enable)
 * 3. Bus Master DMA 활성화 (PCIe Command 레지스터 비트 2)
 * 4. BAR0 리소스 파일을 mmap하여 NVMe 레지스터에 직접 접근
 * 5. nvm_raw_ctrl_init()으로 컨트롤러 핸들 획득 (mmap된 주소 직접 전달)
 * 6. identify() 호출로 Admin Queue 생성 + Identify 실행
 * 7. 리소스 해제 (munmap, close)
 */
int main(int argc, char** argv)
{
    int status;          // 함수 반환값
    nvm_ctrl_t* ctrl;    // NVMe 컨트롤러 핸들

    uint32_t nvm_ns_id;  // 조회할 네임스페이스 ID (0이면 미지정)
    struct bdf device;   // PCIe BDF 식별자
    parse_args(argc, argv, &device, &nvm_ns_id);  // 커맨드라인 옵션 파싱

    /* PCI 디바이스 활성화: sysfs enable 파일에 '1' 기록 */
    status = pci_enable_device(&device);
    if (status != 0)
    {
        fprintf(stderr, "Failed to enable device %04x:%02x:%02x.%x\n",
                device.domain, device.bus, device.device, device.function);
        exit(1);
    }

    /* Bus Master DMA 활성화: PCIe Config Space Command 레지스터 비트 2 설정 */
    status = pci_set_bus_master(&device);
    if (status != 0)
    {
        fprintf(stderr, "Failed to access device config space %04x:%02x:%02x.%x\n",
                device.domain, device.bus, device.device, device.function);
        exit(2);
    }

    /* BAR0 리소스 파일 열기 (NVMe 레지스터 접근용) */
    int fd = pci_open_bar(&device, 0);  // BAR0 = NVMe 컨트롤러 레지스터
    if (fd < 0)
    {
        fprintf(stderr, "Failed to access device BAR memory\n");
        exit(3);
    }

    /* BAR0를 프로세스 주소 공간에 매핑: NVMe 레지스터(CAP, CC, CSTS 등)에 직접 접근 가능 */
    volatile void* ctrl_registers = mmap(NULL, NVM_CTRL_MEM_MINSIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);  // BAR0 전체를 mmap
    if (ctrl_registers == NULL || ctrl_registers == MAP_FAILED)
    {
        fprintf(stderr, "Failed to memory map BAR reasource file: %s\n", strerror(errno));
        close(fd);
        exit(3);
    }

    /* mmap된 레지스터 주소로 컨트롤러 핸들 생성 (커널 모듈 불필요, 직접 BAR 접근) */
    status = nvm_raw_ctrl_init(&ctrl, ctrl_registers, NVM_CTRL_MEM_MINSIZE);  // BAR0에서 CAP 레지스터 파싱하여 페이지 크기 등 결정
    if (status != 0)
    {
        munmap((void*) ctrl_registers, NVM_CTRL_MEM_MINSIZE);  // BAR0 mmap 해제
        close(fd);                                               // BAR0 파일 닫기
        fprintf(stderr, "Failed to get controller reference: %s\n", strerror(status));
        exit(4);
    }

    /* Identify 명령 실행: DMA 메모리 할당 -> Admin Queue 생성 -> Identify Controller/Namespace */
    status = identify(ctrl, nvm_ns_id);

    nvm_ctrl_free(ctrl);                                         // 컨트롤러 핸들 해제
    munmap((void*) ctrl_registers, NVM_CTRL_MEM_MINSIZE);        // BAR0 mmap 해제
    close(fd);                                                    // BAR0 파일 닫기

    fprintf(stderr, "Goodbye!\n");
    exit(status);
}


static void give_usage(const char* name)  // 사용법 출력 함수
{
    fprintf(stderr, "Usage: %s --ctrl=<pci bdf>\n", name);
}


static void show_help(const char* name)  // 도움말 출력 함수
{
    give_usage(name);
    fprintf(stderr, "\nCreate a manager and run an IDENTIFY CONTROLLER NVM admin command.\n\n"
            "    --ctrl     <pci bdf>       PCI bus-device-function to controller.\n"
            "    --ns       <namespace>     Show information about NVM namespace.\n"
            "    --help                     Show this information.\n\n");
}


/*
 * parse_bdf - "DDDD:BB:DD.F" 형식의 PCIe BDF 문자열을 파싱
 *
 * @str: 입력 문자열 (예: "0000:3b:00.0" 또는 "3b:00.0" 또는 "00.0")
 * @dev: 결과를 저장할 bdf 구조체
 * @return: 성공 시 0, 파싱 실패 시 1
 *
 * 콜론(:)과 점(.)을 구분자로 사용하여 domain, bus, device, function을 분리한다.
 * 생략된 부분은 기본값 0으로 처리한다.
 */
static int parse_bdf(char* str, struct bdf* dev)
{
    char* colon = strrchr(str, ':');                              // 마지막 콜론 위치 (bus:device 구분자)
    char* dot = strchr(colon != NULL ? colon : str, '.');         // 점 위치 (device.function 구분자)
    char* endptr;                                                  // strtoul 파싱 종료 위치

    char* function = "0";   // 기본값: function 0
    char* slot = str;       // device 문자열 (기본값: str 전체)
    char* bus = "0";        // 기본값: bus 0
    char* domain = "0";     // 기본값: domain 0

    if (colon != NULL)  // 콜론이 있으면 bus:device 형식
    {
        bus = str;          // 콜론 앞 = bus (또는 domain:bus)
        *colon++ = '\0';    // 콜론을 null로 교체하고 다음 문자로 이동
        slot = colon;       // 콜론 뒤 = device

        colon = strchr(str, ':');  // 추가 콜론 확인 (domain:bus:device 형식)
        if (colon != NULL)  // domain:bus 형식
        {
            domain = str;       // 첫 번째 세그먼트 = domain
            *colon++ = '\0';    // 콜론을 null로 교체
            bus = colon;        // 두 번째 세그먼트 = bus
        }
    }

    if (dot != NULL)  // 점이 있으면 device.function 형식
    {
        *dot++ = '\0';      // 점을 null로 교체
        function = dot;     // 점 뒤 = function
    }

    dev->domain = strtoul(domain, &endptr, 16);  // domain을 16진수로 파싱
    if (endptr == NULL || *endptr != '\0' || dev->domain > 0xffff)  // 유효성 검증
    {
        fprintf(stderr, "Invalid PCI domain number: '%s'\n", domain);
        return 1;
    }

    dev->bus = strtoul(bus, &endptr, 16);  // bus를 16진수로 파싱
    if (endptr == NULL || *endptr != '\0' || dev->bus > 0xff)  // 범위 검증 (0~255)
    {
        fprintf(stderr, "Invalid PCI bus number: '%s'\n", bus);
        return 1;
    }

    dev->device = strtoul(slot, &endptr, 16);  // device를 16진수로 파싱
    if (endptr == NULL || *endptr != '\0' || dev->device > 0xff)  // 범위 검증
    {
        fprintf(stderr, "Invalid PCI device number: '%s'\n", slot);
        return 1;
    }

    dev->function = strtoul(function, &endptr, 0);  // function을 자동 진법으로 파싱
    if (endptr == NULL || *endptr != '\0')  // 유효성 검증
    {
        fprintf(stderr, "Invalid PCI device function: '%s'\n", function);
        return 1;
    }

    return 0;  // 파싱 성공
}



/*
 * parse_args - 커맨드라인 옵션 파싱
 *
 * @argc:      인자 개수
 * @argv:      인자 배열
 * @dev:       결과 PCI BDF 구조체
 * @nvm_ns_id: 결과 네임스페이스 ID (0이면 미지정)
 *
 * 필수: --ctrl <PCI BDF> (예: --ctrl 0000:3b:00.0)
 * 선택: --ns <id> (예: --ns 1)
 *
 * 파싱 후 PCI 디바이스가 실제로 존재하는지 /sys/bus/pci/devices/에서 확인한다.
 */
static void parse_args(int argc, char** argv, struct bdf* dev, uint32_t* nvm_ns_id)
{
    static struct option opts[] = {
        { "help", no_argument, NULL, 'h' },           // --help: 도움말
        { "ctrl", required_argument, NULL, 'c' },     // --ctrl: PCI BDF (필수)
        { "ns", required_argument, NULL, 'n' },       // --ns: 네임스페이스 ID (선택)
        { NULL, 0, NULL, 0 }                           // 옵션 배열 종료 표시
    };

    int opt;                 // getopt 반환값
    int idx;                 // 옵션 인덱스
    char* endptr = NULL;     // strtoul 파싱 종료 위치
    bool dev_set = false;    // --ctrl 옵션이 지정되었는지 추적

    *nvm_ns_id = 0;                        // 네임스페이스 ID 기본값: 미지정(0)
    memset(dev, 0, sizeof(struct bdf));    // BDF 구조체 0으로 초기화

    while ((opt = getopt_long(argc, argv, ":hc:n:", opts, &idx)) != -1)  // 옵션 루프
    {
        switch (opt)
        {
            case '?':  // 알 수 없는 옵션
                fprintf(stderr, "Unknown option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit('?');

            case ':':  // 옵션 인자 누락
                fprintf(stderr, "Missing argument for option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit(':');

            case 'c':  // --ctrl: PCI BDF 문자열 파싱
                if (parse_bdf(optarg, dev) != 0)  // BDF 문자열을 구조체로 변환
                {
                    give_usage(argv[0]);
                    exit('c');
                }
                dev_set = true;  // --ctrl 옵션 지정됨
                break;

            case 'n':  // --ns: 네임스페이스 ID 파싱
                *nvm_ns_id = strtoul(optarg, &endptr, 0);  // 자동 진법으로 파싱
                if (endptr == NULL || *endptr != '\0')
                {
                    fprintf(stderr, "Invalid NVM namespace!\n");
                    exit('n');
                }
                break;

            case 'h':  // --help
                show_help(argv[0]);
                exit(0);
        }
    }

    if (!dev_set)  // --ctrl 미지정 시 에러
    {
        fprintf(stderr, "Controller is not specified!\n");
        exit('c');
    }

    /* PCI 디바이스가 실제로 존재하는지 sysfs에서 확인 */
    char path[64];
    struct stat s;
    sprintf(path, "/sys/bus/pci/devices/%04x:%02x:%02x.%x",
            dev->domain, dev->bus, dev->device, dev->function);  // sysfs 디바이스 경로

    if (stat(path, &s) != 0 || ! S_ISDIR(s.st_mode))  // 경로가 존재하고 디렉토리인지 확인
    {
        fprintf(stderr, "%04x:%02x:%02x.%x is not a valid PCI device\n",
                dev->domain, dev->bus, dev->device, dev->function);
        exit('c');
    }
}
