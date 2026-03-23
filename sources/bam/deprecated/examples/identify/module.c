/*
 * module.c - NVMe Identify 커널 모듈 백엔드 (deprecated)
 *
 * libnvm 커널 모듈(/dev/libnvmX)을 통해 NVMe 컨트롤러에 접근하여 Identify 명령을 수행한다.
 * 커널 모듈이 PCIe BAR0 매핑과 DMA 버퍼 할당을 대행한다.
 *
 * 흐름: /dev/libnvmX open → nvm_ctrl_init() → nvm_dma_map_host() → reset_ctrl() → identify
 */
#include <nvm_types.h>     // libnvm 기본 타입
#include <nvm_ctrl.h>      // nvm_ctrl_init, nvm_ctrl_free
#include <nvm_dma.h>       // nvm_dma_map_host, nvm_dma_unmap
#include <nvm_aq.h>        // nvm_aq_create, nvm_aq_destroy
#include <nvm_admin.h>     // Identify 명령
#include <nvm_util.h>      // NVM_DMA_OFFSET 매크로
#include <stddef.h>        // size_t
#include <stdint.h>        // uint32_t
#include <stdio.h>         // fprintf
#include <stdlib.h>        // exit, posix_memalign
#include <stdbool.h>       // bool
#include <unistd.h>        // close, sysconf
#include <fcntl.h>         // open, O_RDWR, O_NONBLOCK
#include <sys/ioctl.h>     // ioctl (libnvm 커널 모듈 통신)
#include <getopt.h>        // getopt_long
#include <string.h>        // strerror, memset
#include <errno.h>         // errno
#include "common.h"        // reset_ctrl, identify_ctrl, identify_ns



static void parse_args(int argc, char** argv, char** device, uint32_t* ns_id);  // 전방 선언



/*
 * open_fd - libnvm 커널 모듈 디바이스 파일 열기
 *
 * @path:   디바이스 파일 경로 (예: /dev/libnvm0)
 * @return: 성공 시 파일 디스크립터, 실패 시 -1
 *
 * O_RDWR|O_NONBLOCK으로 열어야 libnvm ioctl을 사용할 수 있다.
 */
static int open_fd(const char* path)
{
    int fd;

    fd = open(path, O_RDWR|O_NONBLOCK);  // 읽기+쓰기+논블로킹 모드로 디바이스 파일 열기
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open descriptor: %s\n", strerror(errno));
        return -1;
    }

    return fd;  // 파일 디스크립터 반환
}


/*
 * main - 커널 모듈 경유 NVMe Identify 실행
 *
 * [실행 흐름]
 * 1. 커맨드라인 파싱 (--ctrl <디바이스 경로>, --ns <네임스페이스>)
 * 2. /dev/libnvmXXX 디바이스 파일 열기
 * 3. nvm_ctrl_init()으로 컨트롤러 핸들 획득 (커널 모듈이 BAR0 매핑 수행)
 * 4. 3페이지 DMA 메모리 할당 (페이지0=ACQ, 페이지1=ASQ, 페이지2=Identify 버퍼)
 * 5. nvm_dma_map_host()로 DMA 윈도우 생성 (커널이 IOMMU 매핑 수행)
 * 6. reset_ctrl() -> identify_ctrl() -> identify_ns()
 * 7. 리소스 역순 해제
 */
int main(int argc, char** argv)
{
    int status;                       // 함수 반환값
    nvm_ctrl_t* ctrl;                 // NVMe 컨트롤러 핸들
    nvm_dma_t* window = NULL;         // DMA 윈도우 핸들
    nvm_aq_ref admin = NULL;          // Admin Queue 참조
    uint32_t nvm_namespace;           // 조회할 네임스페이스 ID
    void* memory;                     // posix_memalign으로 할당한 메모리

    long page_size = sysconf(_SC_PAGESIZE);  // 시스템 페이지 크기 조회 (보통 4096)

    char* path = NULL;  // 디바이스 파일 경로 (/dev/libnvmXXX)
    parse_args(argc, argv, &path, &nvm_namespace);  // 커맨드라인 파싱

    int fd = open_fd(path);  // libnvm 디바이스 파일 열기
    if (fd < 0)
    {
        exit(1);
    }

    /* 커널 모듈 경유로 컨트롤러 핸들 획득: ioctl로 BAR0 레지스터를 읽어 CAP, 페이지 크기 등 파싱 */
    status = nvm_ctrl_init(&ctrl, fd);  // fd를 통해 커널 모듈과 통신
    if (status != 0)
    {
        close(fd);
        fprintf(stderr, "Failed to get controller reference: %s\n", strerror(status));
        exit(1);
    }

    close(fd);  // 컨트롤러 핸들 획득 후 fd는 더 이상 불필요

    /* 3페이지 페이지 정렬 메모리 할당: ACQ(1페이지) + ASQ(1페이지) + Identify 버퍼(1페이지) */
    status = posix_memalign(&memory, ctrl->page_size, 3 * page_size);  // 컨트롤러 페이지 크기로 정렬
    if (status != 0)
    {
        fprintf(stderr, "Failed to allocate page-aligned memory: %s\n", strerror(status));
        nvm_ctrl_free(ctrl);
        exit(2);
    }

    /* 커널 모듈이 IOMMU 매핑을 수행하여 DMA 윈도우 생성 (유저스페이스의 nvm_dma_map과 달리 물리주소 불필요) */
    status = nvm_dma_map_host(&window, ctrl, memory, 3 * page_size);  // 커널이 가상→IOMMU 주소 변환 수행
    if (status != 0)
    {
        free(memory);         // 할당한 메모리 해제
        nvm_ctrl_free(ctrl);  // 컨트롤러 핸들 해제
        exit(1);
    }

    /* 컨트롤러 리셋 및 Admin Queue 생성 */
    admin = reset_ctrl(ctrl, window);  // CC.EN=0 -> AQA/ASQ/ACQ 설정 -> CC.EN=1
    if (admin == NULL)
    {
        status = 1;
        goto leave;
    }

    /* Identify Controller 실행: DMA 윈도우 페이지 2를 데이터 버퍼로 사용 */
    status = identify_ctrl(admin, NVM_DMA_OFFSET(window, 2), window->ioaddrs[2]);  // 페이지2의 가상주소/IO주소
    if (status != 0)
    {
        goto leave;
    }

    /* 네임스페이스 ID가 지정된 경우 Identify Namespace도 실행 */
    if (nvm_namespace != 0)
    {
        status = identify_ns(admin, nvm_namespace, NVM_DMA_OFFSET(window, 2), window->ioaddrs[2]);
    }

leave:
    nvm_aq_destroy(admin);     // Admin Queue 파괴 (NULL-safe)
    nvm_dma_unmap(window);     // DMA 윈도우 해제 (IOMMU 매핑 해제 포함)
    free(memory);              // 할당한 메모리 해제
    nvm_ctrl_free(ctrl);       // 컨트롤러 핸들 해제

    fprintf(stderr, "Goodbye!\n");
    exit(status);
}


static void give_usage(const char* name)  // 사용법 출력
{
    fprintf(stderr, "Usage: %s --ctrl=<dev id>\n", name);
}


static void show_help(const char* name)  // 도움말 출력
{
    give_usage(name);
    fprintf(stderr, "\nCreate a manager and run an IDENTIFY CONTROLLER NVM admin command.\n\n"
            "    --ctrl     <path>          Path to controller device (/dev/libnvmXXX).\n"
            "    --ns       <namespace>     Show information about NVM namespace.\n"
            "    --help                     Show this information.\n\n");
}


/*
 * parse_args - 커맨드라인 옵션 파싱
 *
 * @argc:  인자 개수
 * @argv:  인자 배열
 * @dev:   컨트롤러 디바이스 경로 (결과, 예: /dev/libnvm0)
 * @ns_id: 네임스페이스 ID (결과, 0이면 미지정)
 *
 * 필수: --ctrl <path>
 * 선택: --ns <id>
 */
static void parse_args(int argc, char** argv, char** dev, uint32_t* ns_id)
{
    static struct option opts[] = {
        { "help", no_argument, NULL, 'h' },           // --help
        { "ctrl", required_argument, NULL, 'c' },     // --ctrl: 디바이스 경로 (필수)
        { "ns", required_argument, NULL, 'n' },       // --ns: 네임스페이스 ID (선택)
        { NULL, 0, NULL, 0 }
    };

    int opt;             // getopt 반환값
    int idx;             // 옵션 인덱스
    char* endptr = NULL; // strtoul 파싱 종료 위치

    *dev = NULL;   // 디바이스 경로 초기값: 미지정
    *ns_id = 0;    // 네임스페이스 ID 초기값: 미지정

    while ((opt = getopt_long(argc, argv, ":hc:n:", opts, &idx)) != -1)
    {
        switch (opt)
        {
            case '?': // 알 수 없는 옵션
                fprintf(stderr, "Unknown option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit('?');

            case ':': // 옵션 인자 누락
                fprintf(stderr, "Missing argument for option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit(':');

            case 'c': // --ctrl: 디바이스 경로를 직접 저장 (문자열 복사 아닌 포인터 대입)
                *dev = optarg;
                break;

            case 'n': // --ns: 네임스페이스 ID를 정수로 파싱
                *ns_id = strtoul(optarg, &endptr, 0);  // 자동 진법
                if (endptr == NULL || *endptr != '\0')
                {
                    fprintf(stderr, "Invalid NVM namespace");
                    give_usage(argv[0]);
                    exit('n');
                }
                break;

            case 'h': // --help
                show_help(argv[0]);
                exit(0);
        }
    }

    if (*dev == NULL)  // --ctrl 미지정 시 에러
    {
        fprintf(stderr, "Controller is not set!\n");
        give_usage(argv[0]);
        exit('c');
    }
}
