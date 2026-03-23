/*
 * rpc_identify.c - RPC를 통한 원격 NVMe Identify 명령 클라이언트 (Deprecated)
 *
 * [파일 역할]
 * DIS 클러스터에서 원격 NVMe 컨트롤러의 정보를 조회하는 클라이언트 프로그램.
 * rpc_server가 관리하는 Admin Queue에 RPC로 바인딩한 뒤,
 * NVMe Identify Controller 및 Identify Namespace Admin 명령을 원격으로 실행하여
 * 컨트롤러/네임스페이스 정보를 출력한다.
 *
 * [Deprecated 배경]
 * SISCI 기반 DIS 클러스터 원격 NVMe 접근 방식에서 커널 모듈 기반
 * GPU-initiated I/O로 전환되면서 deprecated되었다.
 *
 * [RPC를 통한 NVMe Admin 명령 구조]
 * 1. nvm_dis_ctrl_init()으로 원격 NVMe 컨트롤러 핸들 획득 (SmartIO FDID)
 * 2. nvm_dis_rpc_bind()로 rpc_server의 Admin Queue에 원격 바인딩
 * 3. nvm_admin_ctrl_info() / nvm_admin_ns_info() 호출 시 내부적으로
 *    RPC 메시지가 rpc_server로 전달되어 실제 Admin Queue에 명령이 제출됨
 * 4. 결과가 SISCI 세그먼트의 DMA 윈도우를 통해 로컬 메모리로 전달됨
 */
#include <nvm_types.h>
#include <nvm_ctrl.h>
#include <nvm_rpc.h>
#include <nvm_dma.h>
#include <nvm_admin.h>
#include <nvm_util.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <sisci_api.h>
#include "segment.h"
#include "util.h"


/*
 * cl_args - 커맨드라인 옵션 구조체
 *
 * @smartio_dev_id: SmartIO Fabric Device ID (16진수)
 * @nvm_namespace:  조회할 NVMe 네임스페이스 ID (0이면 네임스페이스 정보 생략)
 * @adapter:        컨트롤러에 연결할 DIS 어댑터 번호
 */
struct cl_args
{
    uint64_t    smartio_dev_id;     // Specify SmartIO device
    uint32_t    nvm_namespace;      // NVM namespace
    uint32_t    adapter;            // Controller adapter
};


static void give_usage(const char* program_name)
{
    fprintf(stderr,
            "Usage: %s --ctrl <fdid> [--adapter <adapter>] [--namespace <ns id>]\n",
            program_name);
}


static void parse_opts(int argc, char** argv, struct cl_args* args);


/*
 * show_ctrl_info - Identify Controller 명령을 RPC로 실행하여 컨트롤러 정보 출력
 *
 * @rpc:     RPC로 바인딩된 Admin Queue 참조
 * @ctrl:    NVMe 컨트롤러 핸들
 * @adapter: DMA 윈도우 생성용 DIS 어댑터 번호
 * @return:  성공 시 0, 실패 시 에러 코드
 *
 * [동작 흐름]
 * 1. 임시 SISCI 세그먼트 생성 (세그먼트 ID=12, 크기=4KB=0x1000)
 * 2. 세그먼트를 DMA 윈도우로 매핑 (Identify 결과 수신 버퍼)
 * 3. nvm_admin_ctrl_info()로 RPC를 통해 Identify Controller 실행
 *    - 내부적으로 rpc_server가 Admin Queue에 명령을 대신 제출
 *    - 결과가 DMA를 통해 로컬 세그먼트에 기록됨
 * 4. 성공 시 print_ctrl_info()로 컨트롤러 정보 출력
 * 5. DMA 윈도우 및 세그먼트 해제
 */
static int show_ctrl_info(nvm_aq_ref rpc, const nvm_ctrl_t* ctrl, uint32_t adapter)
{
    struct segment memory_page;
    nvm_dma_t* dma;

    int status = segment_create(&memory_page, 12, 0x1000);
    if (status != 0)
    {
        fprintf(stderr, "Failed to create memory segment\n");
        return 1;
    }

    status = dma_create(&dma, ctrl, &memory_page, adapter);
    if (status != 0)
    {
        segment_remove(&memory_page);
        fprintf(stderr, "Failed to create DMA window\n");
        return 2;
    }

    struct nvm_ctrl_info info;
    status = nvm_admin_ctrl_info(rpc, &info, dma->vaddr, dma->ioaddrs[0]);
    if (status == 0)
    {
        print_ctrl_info(stdout, &info);
    }
    else
    {
        fprintf(stderr, "RPC command request failed: %s\n", strerror(status));
    }

    dma_remove(dma, &memory_page, adapter);
    segment_remove(&memory_page);
    return status;
}




/*
 * show_ns_info - Identify Namespace 명령을 RPC로 실행하여 네임스페이스 정보 출력
 *
 * @rpc:      RPC로 바인딩된 Admin Queue 참조
 * @ns_id:    조회할 NVMe 네임스페이스 ID
 * @adapter:  DMA 윈도우 생성용 DIS 어댑터 번호
 * @return:   성공 시 0, 실패 시 에러 코드
 *
 * show_ctrl_info()와 동일한 패턴으로 세그먼트/DMA 윈도우를 생성하고,
 * nvm_admin_ns_info()로 Identify Namespace를 실행한다.
 * nvm_ctrl_from_aq_ref()로 rpc에서 컨트롤러 핸들을 추출하여 DMA 생성에 사용.
 */
static int show_ns_info(nvm_aq_ref rpc, uint32_t ns_id, uint32_t adapter)
{
    struct segment memory_page;
    nvm_dma_t* dma;

    int status = segment_create(&memory_page, 13, 0x1000);
    if (status != 0)
    {
        fprintf(stderr, "Failed to create memory segment\n");
        return 1;
    }

    status = dma_create(&dma, nvm_ctrl_from_aq_ref(rpc), &memory_page, adapter);
    if (status != 0)
    {
        segment_remove(&memory_page);
        fprintf(stderr, "Failed to create DMA window\n");
        return 2;
    }

    struct nvm_ns_info info;
    status = nvm_admin_ns_info(rpc, &info, ns_id, dma->vaddr, dma->ioaddrs[0]);
    if (status == 0)
    {
        print_ns_info(stdout, &info);
    }
    else
    {
        fprintf(stderr, "RPC command request failed: %s\n", strerror(status));
    }

    dma_remove(dma, &memory_page, adapter);
    segment_remove(&memory_page);
    return status;
}



/*
 * main - RPC Identify 클라이언트 메인 함수
 *
 * [실행 흐름]
 * 1. 커맨드라인 옵션 파싱
 * 2. SISCI 초기화
 * 3. SmartIO FDID로 NVMe 컨트롤러 핸들 획득
 * 4. nvm_dis_rpc_bind()로 rpc_server의 Admin Queue에 원격 바인딩
 * 5. show_ctrl_info()로 Identify Controller 실행 및 결과 출력
 * 6. 네임스페이스 ID가 지정되었으면 show_ns_info()로 Identify Namespace 실행
 * 7. 리소스 정리: RPC 해제 -> 컨트롤러 해제 -> SISCI 종료
 */
int main(int argc, char** argv)
{
    nvm_ctrl_t* ctrl;
    nvm_aq_ref rpc;

    struct cl_args args;
    parse_opts(argc, argv, &args);

    sci_error_t err;
    SCIInitialize(0, &err);
    if (err != SCI_ERR_OK)
    {
        fprintf(stderr, "Failed to initialize SISCI: %s\n", SCIGetErrorString(err));
        exit(1);
    }

    /* SmartIO FDID로 NVMe 컨트롤러 핸들 획득 */
    // Get controller reference
    int status = nvm_dis_ctrl_init(&ctrl, args.smartio_dev_id);
    if (status != 0)
    {
        fprintf(stderr, "Failed to get controller reference: %s\n", strerror(status));
        exit(2);
    }

    /* RPC 서버(rpc_server)의 Admin Queue에 원격 바인딩 */
    // Get RPC reference
    status = nvm_dis_rpc_bind(&rpc, ctrl, args.adapter);
    if (status != 0)
    {
        nvm_ctrl_free(ctrl);
        fprintf(stderr, "Failed to get RPC reference: %s\n", strerror(status));
        exit(2);
    }

    /* Identify Controller 실행 */
    status = show_ctrl_info(rpc, ctrl, args.adapter);
    if (status != 0)
    {
        goto leave;
    }

    /* 네임스페이스 ID가 지정되었으면 Identify Namespace도 실행 */
    if (args.nvm_namespace > 0)
    {
        status = show_ns_info(rpc, args.nvm_namespace, args.adapter);
    }

leave:
    // Free resources and quit
    nvm_rpc_unbind(rpc);
    nvm_ctrl_free(ctrl);
    SCITerminate();
    exit(status);
}


static void show_help(const char* program_name)
{
    give_usage(program_name);
    fprintf(stderr,
            "\nIdentify controller using a remote controller manager in a DIS cluster.\n\n"
            "    --ctrl             <fdid>      SmartIO device identifier.\n"
            "    --namespace        <ns id>     Show information about NVM namespace.\n"
            "    --adapter          <adapter>   Local adapter to reach device (default is 0).\n"
            "    --help                         Show this information.\n"
            "\n");
}


/*
 * parse_opts - 커맨드라인 옵션 파싱
 *
 * @argc: 인자 개수
 * @argv: 인자 배열
 * @args: 결과 저장 구조체
 *
 * 지원 옵션:
 *   --ctrl <fdid>:       SmartIO FDID (16진수, 필수)
 *   --namespace <ns_id>: NVMe 네임스페이스 ID (0이면 생략)
 *   --adapter <num>:     DIS 어댑터 번호 (기본 0)
 *
 * --ctrl 미지정 시 에러 종료.
 */
static void parse_opts(int argc, char** argv, struct cl_args* args)
{
    static struct option opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "ctrl", required_argument, NULL, 'd' },
        { "namespace", required_argument, NULL, 'n' },
        { "ns", required_argument, NULL, 'n' },
        { "adapter", required_argument, NULL, 'a' },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    int idx;

    memset(args, 0, sizeof(struct cl_args));

    // Parse arguments
    while ((opt = getopt_long(argc, argv, ":hd:a:n:", opts, &idx)) != -1)
    {
        switch (opt)
        {
            case '?': // unknown option
                fprintf(stderr, "Unknown option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit('?');

            case ':': // missing option argument
                fprintf(stderr, "Missing argument for option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit(':');

            case 'h': // show help
                show_help(argv[0]);
                exit(0);

            case 'd': // device identifier (16진수 SmartIO FDID)
                if (parse_u64(optarg, &args->smartio_dev_id, 16) != 0 || args->smartio_dev_id == 0)
                {
                    fprintf(stderr, "Invalid device id: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('c');
                }
                break;

            case 'n': // namespace identifier
                if (parse_u32(optarg, &args->nvm_namespace, 0) != 0 || args->nvm_namespace == 0)
                {
                    fprintf(stderr, "Invalid namespace id: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('n');
                }
                break;

            case 'a': // device adapter (10진수)
                if (parse_u32(optarg, &args->adapter, 10) != 0)
                {
                    fprintf(stderr, "Invalid adapter number: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('a');
                }
                break;
        }
    }

    /* 컨트롤러 FDID 미지정 시 에러 종료 */
    if (args->smartio_dev_id == 0)
    {
        fprintf(stderr, "No controller specified!\n");
        give_usage(argv[0]);
        exit('c');
    }
}
