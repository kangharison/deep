/*
 * smartio.c - NVMe Identify SmartIO(DIS) 백엔드 (deprecated)
 *
 * Dolphin SmartIO(SISCI)를 통해 원격 NVMe 컨트롤러에 접근하여 Identify 명령을 수행한다.
 * 원격 노드의 NVMe 디바이스를 DIS 패브릭을 통해 빌려오는(borrow) 방식이다.
 *
 * 흐름: SCIOpen → nvm_dis_ctrl_init(FDID) → SISCI 세그먼트 DMA 매핑 → Identify
 */
#include <nvm_dma.h>       // nvm_dis_dma_create, nvm_dma_unmap
#include <nvm_types.h>     // libnvm 기본 타입
#include <nvm_aq.h>        // nvm_aq_create, nvm_aq_destroy
#include <nvm_admin.h>     // Identify 명령
#include <nvm_ctrl.h>      // nvm_dis_ctrl_init, nvm_ctrl_free
#include <nvm_util.h>      // NVM_DMA_OFFSET 매크로
#include <sisci_types.h>   // SISCI 타입 정의 (sci_error_t)
#include <sisci_api.h>     // SCIInitialize, SCITerminate
#include <getopt.h>        // getopt_long
#include <stdint.h>        // uint64_t, uint32_t
#include <stddef.h>        // size_t
#include <stdlib.h>        // exit, strtoul
#include <stdio.h>         // fprintf, stderr
#include <stdbool.h>       // bool
#include <errno.h>         // EINVAL, errno
#include <string.h>        // strerror
#include <limits.h>        // UINT_MAX
#include "common.h"        // reset_ctrl, identify_ctrl, identify_ns


/*
 * cl_args - 커맨드라인 인자 구조체
 *
 * @dev_id:       SmartIO Fabric Device ID (16진수, 필수)
 * @namespace_id: NVMe 네임스페이스 ID (0이면 미지정)
 */
struct cl_args
{
    uint64_t    dev_id;       /* SmartIO FDID: DIS 패브릭에서 원격 NVMe를 식별하는 고유 ID */
    uint32_t    namespace_id; /* NVMe 네임스페이스 번호 (0이면 Namespace Identify 생략) */
};


static void parse_args(int argc, char** argv, struct cl_args* args);  // 전방 선언


/*
 * main - SmartIO(DIS) 경유 NVMe Identify 실행
 *
 * [실행 흐름]
 * 1. 커맨드라인 파싱 (--ctrl <FDID>, --ns <namespace>)
 * 2. SISCI API 초기화 (SCIInitialize)
 * 3. nvm_dis_ctrl_init(FDID)로 원격 NVMe 컨트롤러 핸들 획득
 * 4. nvm_dis_dma_create()로 3페이지(12KB) SISCI 세그먼트 DMA 메모리 할당
 * 5. reset_ctrl() -> identify_ctrl() -> identify_ns()
 * 6. 리소스 역순 해제 후 SCITerminate
 */
int main(int argc, char** argv)
{
    sci_error_t err;        // SISCI API 에러 코드
    struct cl_args args;    // 파싱된 커맨드라인 인자

    parse_args(argc, argv, &args);  // 커맨드라인 파싱

    /* SISCI API 초기화 (DIS 클러스터 통신 라이브러리) */
    SCIInitialize(0, &err);  // 플래그 0, 에러는 err에 반환

    nvm_ctrl_t* ctrl;  // NVMe 컨트롤러 핸들
    /* SmartIO FDID로 원격 NVMe 컨트롤러 핸들 획득: PCIe BAR0를 DIS 패브릭을 통해 매핑 */
    int status = nvm_dis_ctrl_init(&ctrl, args.dev_id);  // FDID로 원격 디바이스 접근
    if (status != 0)
    {
        fprintf(stderr, "Failed to initialize controller reference: %s\n", strerror(status));
        exit(status);
    }

    nvm_dma_t* window;  // DMA 윈도우 핸들
    /* SISCI 세그먼트로 3페이지(12KB) DMA 메모리 할당: 페이지0=ACQ, 페이지1=ASQ, 페이지2=Identify 버퍼 */
    status = nvm_dis_dma_create(&window, ctrl, 3 * 0x1000, 0);  // 3*4KB, 플래그 0
    if (status != 0)
    {
        nvm_ctrl_free(ctrl);  // 컨트롤러 핸들 해제
        fprintf(stderr, "Failed to create local segment: %s\n", strerror(status));
        exit(status);
    }
    memset(window->vaddr, 0, 3 * 0x1000);  // DMA 메모리 전체를 0으로 초기화 (큐 엔트리 클리어)

    nvm_aq_ref aq;  // Admin Queue 참조 핸들
    aq = reset_ctrl(ctrl, window);  // 컨트롤러 리셋 + Admin Queue 생성
    if (aq == NULL)
    {
        status = 1;
        goto leave;  // 실패 시 정리 코드로 이동
    }

    /* Identify Controller 실행: 페이지2를 데이터 버퍼로 사용 */
    status = identify_ctrl(aq, NVM_DMA_OFFSET(window, 2), window->ioaddrs[2]);  // 가상주소와 IO주소 전달
    if (status != 0)
    {
        goto leave;
    }

    /* 네임스페이스 ID가 지정된 경우 Identify Namespace도 실행 */
    if (args.namespace_id != 0)
    {
        status = identify_ns(aq, args.namespace_id, NVM_DMA_OFFSET(window, 2), window->ioaddrs[2]);
    }

leave:
    nvm_aq_destroy(aq);       // Admin Queue 파괴
    nvm_dma_unmap(window);    // DMA 윈도우 해제 (SISCI 세그먼트 해제 포함)
    nvm_ctrl_free(ctrl);      // 컨트롤러 핸들 해제
    SCITerminate();            // SISCI 라이브러리 종료

    fprintf(stderr, "Goodbye!\n");
    exit(status);
}


/* parse_u64 - 문자열을 uint64_t로 파싱. 성공 시 0, 실패 시 EINVAL */
static int parse_u64(const char* str, uint64_t* num, int base)
{
    char* endptr = NULL;                       // strtoul 파싱 종료 위치
    uint64_t ul = strtoul(str, &endptr, base); // 지정 진법으로 변환

    if (endptr == NULL || *endptr != '\0')     // 문자열 전체가 유효한 숫자인지 확인
    {
        return EINVAL;  // 파싱 실패
    }

    *num = ul;
    return 0;  // 파싱 성공
}


/* parse_u32 - 문자열을 uint32_t로 파싱. UINT_MAX 초과 시 EINVAL */
static int parse_u32(const char* str, uint32_t* num, int base)
{
    int status;
    uint64_t ul;

    status = parse_u64(str, &ul, base);  // 먼저 uint64_t로 파싱

    if (status != 0 || ul > UINT_MAX)    // 32비트 범위 초과 검증
    {
        return EINVAL;
    }

    *num = (uint32_t) ul;  // 안전하게 32비트로 캐스팅
    return status;
}


static void give_usage(const char* name)  // 사용법 출력
{
    fprintf(stderr, "Usage: %s --ctrl=<dev id> [--adapter=<adapter>] [--id=<segment id>]\n", name);
}


static void show_help(const char* name)  // 도움말 출력
{
    give_usage(name);
    fprintf(stderr, "\nCreate a manager and run an IDENTIFY CONTROLLER NVM admin command.\n\n"
            "    --ctrl     <fdid>          SmartIO device identifier (fabric device id).\n"
            "    --ns       <namespace id>  Show information about NVM namespace.\n"
            "    --help                     Show this information.\n\n");
}




/*
 * parse_args - 커맨드라인 옵션 파싱
 *
 * @argc: 인자 개수
 * @argv: 인자 배열
 * @args: 결과 구조체
 *
 * 필수: --ctrl <FDID> (SmartIO Fabric Device ID, 16진수)
 * 선택: --ns <namespace id>
 */
static void parse_args(int argc, char** argv, struct cl_args* args)
{
    static struct option opts[] = {
        { "help", no_argument, NULL, 'h' },          // --help
        { "ctrl", required_argument, NULL, 'c' },    // --ctrl: FDID (필수)
        { "ns", required_argument, NULL, 'n' },      // --ns: 네임스페이스 ID (선택)
        { "segment", required_argument, NULL, 's' },  // --segment: 미사용 (하위 호환성)
        { NULL, 0, NULL, 0 }
    };

    int opt;    // getopt 반환값
    int idx;    // 옵션 인덱스

    bool dev_set = false;     // --ctrl 지정 여부
    args->dev_id = 0;         // FDID 기본값
    args->namespace_id = 0;   // 네임스페이스 기본값: 미지정

    while ((opt = getopt_long(argc, argv, ":hc:n:", opts, &idx)) != -1)  // 옵션 루프
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

            case 'c': // --ctrl: SmartIO FDID를 16진수로 파싱
                dev_set = true;
                if (parse_u64(optarg, &args->dev_id, 16) != 0)  // 16진수로 파싱
                {
                    give_usage(argv[0]);
                    exit('c');
                }
                break;

            case 'n': // --ns: 네임스페이스 ID 파싱
                if (parse_u32(optarg, &args->namespace_id, 0) != 0)  // 자동 진법
                {
                    give_usage(argv[0]);
                    exit('n');
                }
                break;

            case 'h': // --help
                show_help(argv[0]);
                exit(0);
        }
    }

    if (!dev_set)  // --ctrl 미지정 시 에러
    {
        fprintf(stderr, "Device ID is not set!\n");
        give_usage(argv[0]);
        exit('c');
    }
}
