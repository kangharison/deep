/*
 * rpc_server.c - NVMe Admin Queue RPC 서버 (매니저) (Deprecated)
 *
 * [파일 역할]
 * DIS 클러스터에서 NVMe 컨트롤러의 Admin Queue를 소유하고 관리하는 RPC 서버.
 * 원격 클라이언트(rpc_identify, rpc_flush, rpc_dd 등)가 NVMe Admin 명령을
 * 실행해야 할 때, 이 서버가 대신 Admin Queue에 명령을 제출해 준다.
 *
 * [핵심 역할: Admin Queue 매니저]
 * NVMe 컨트롤러는 Admin Queue(ASQ/ACQ)를 하나만 가진다.
 * 멀티노드 환경에서 여러 클라이언트가 Admin 명령(Identify, Create Queue 등)을
 * 사용하려면 Admin Queue를 중앙에서 관리하는 매니저가 필요하다.
 * 이 서버가 그 역할을 수행하며, DIS SISCI의 RPC 메커니즘으로 클라이언트 요청을 받는다.
 *
 * [Deprecated 배경]
 * SISCI 기반 DIS 클러스터 원격 NVMe 접근 방식에서 커널 모듈(libnvm.ko) 기반
 * 로컬 GPU-NVMe 직접 접근으로 전환되면서 deprecated되었다.
 *
 * [실행 흐름]
 * 1. NVMe 컨트롤러 리셋 및 Admin Queue 생성 (매니저가 됨)
 * 2. 선택적으로 Identify Controller 실행
 * 3. 요청된 수만큼 I/O CQ/SQ 큐 예약 (Set Number of Queues Feature)
 * 4. 지정된 DIS 어댑터들에서 RPC 서비스 활성화
 * 5. SIGTERM/SIGINT를 받을 때까지 대기 (pthread_cond_wait)
 * 6. 시그널 수신 시 RPC 서비스 종료 및 리소스 정리
 */
#include <nvm_types.h>
#include <nvm_ctrl.h>
#include <nvm_aq.h>
#include <nvm_admin.h>
#include <nvm_dma.h>
#include <nvm_util.h>
#include <nvm_error.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sisci_api.h>
#include <dis/dis_types.h>
#include "segment.h"
#include "util.h"

/* DIS 어댑터 최대 개수 (DIS 라이브러리에서 정의한 상수) */
#define MAX_ADAPTS DIS_MAX_NSCIS


/*
 * cl_args - 커맨드라인 옵션 구조체
 *
 * @identify_ctrl:  true이면 서버 시작 시 Identify Controller 명령 실행
 * @smartio_dev_id: SmartIO Fabric Device ID (NVMe 컨트롤러 식별)
 * @ctrl_adapter:   컨트롤러에 연결할 DIS 어댑터 번호
 * @n_sqs:          예약할 I/O Submission Queue 개수
 * @n_cqs:          예약할 I/O Completion Queue 개수
 * @n_dis_adapters: RPC를 활성화할 DIS 어댑터 개수
 * @dis_adapters:   RPC를 활성화할 DIS 어댑터 번호 배열
 */
struct cl_args
{
    bool        identify_ctrl;      // Indicates if the manager should run an NVM identify controller command
    uint64_t    smartio_dev_id;     // Specify SmartIO device
    uint32_t    ctrl_adapter;       // Controller adapter
    uint16_t    n_sqs;              // Number of SQs to reserve
    uint16_t    n_cqs;              // Number of CQs to reserve
    size_t      n_dis_adapters;     // Number of adapters specified
    uint32_t    dis_adapters[MAX_ADAPTS]; // Indicate which local adapters to bind manager to
};


static bool verbose = false;


/* 시그널 수신 여부 플래그 (SIGTERM/SIGINT 핸들러에서 true로 설정) */
static bool signal_caught = false;


/* 시그널 대기용 조건 변수와 뮤텍스 (메인 스레드가 시그널을 받을 때까지 sleep) */
static pthread_cond_t signal_wq = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t signal_lk = PTHREAD_MUTEX_INITIALIZER;


static void give_usage(const char* program_name);
static void show_help(const char* program_name);
static void parse_opts(int argc, char** argv, struct cl_args* args);
static void identify_controller(const nvm_ctrl_t* ctrl, uint32_t adapter, nvm_aq_ref rpc);


/*
 * catch_signal - SIGTERM/SIGINT 시그널 핸들러
 *
 * 뮤텍스를 잡고 signal_caught 플래그를 설정한 뒤 조건 변수를 시그널링하여
 * 메인 스레드의 pthread_cond_wait()를 깨운다.
 */
static void catch_signal()
{
    if (verbose)
    {
        fprintf(stderr, "Signal caught, quiting...\n");
    }

    pthread_mutex_lock(&signal_lk);
    signal_caught = true;
    pthread_cond_signal(&signal_wq);
    pthread_mutex_unlock(&signal_lk);
}


/*
 * request_accepter - RPC 요청 수락 콜백
 *
 * @cmd:      클라이언트가 보낸 NVMe Admin 명령
 * @adapter:  요청이 들어온 DIS 어댑터 번호
 * @node_id:  요청을 보낸 원격 노드 ID
 * @return:   true이면 요청 수락, false이면 거부
 *
 * nvm_dis_rpc_enable() 호출 시 콜백으로 등록되며, 원격 클라이언트가
 * nvm_dis_rpc_bind()로 연결한 뒤 Admin 명령을 보내면 이 함수가 호출된다.
 * 현재 구현은 모든 요청을 무조건 수락한다.
 */
static bool request_accepter(nvm_cmd_t* cmd, uint32_t adapter, uint32_t node_id)
{
    if (verbose)
    {
        fprintf(stderr, "* Command request received on adapter %u from node %u (%u)\n",
                adapter, node_id, *NVM_CMD_CID(cmd));
    }

    return true;
}


/*
 * run_service - RPC 서비스 실행 (메인 로직)
 *
 * @ctrl:  NVMe 컨트롤러 핸들
 * @q_wnd: Admin Queue용 DMA 윈도우 (2페이지: ASQ+ACQ)
 * @args:  커맨드라인 옵션
 * @return: 종료 상태 코드 (0=정상, 2=AQ 생성 실패, 3=큐 예약 실패)
 *
 * [동작 흐름]
 * 1. nvm_aq_create()로 Admin Queue 생성 (컨트롤러 리셋 -> AQA/ASQ/ACQ 레지스터 설정 -> CC.EN=1)
 * 2. --identify 옵션 시 컨트롤러 정보 조회 및 출력
 * 3. --cqs/--sqs 옵션 시 Set Features(Number of Queues)로 I/O 큐 개수 예약
 *    - 컨트롤러가 요청보다 적은 수를 할당하면 에러 반환
 * 4. 지정된 각 DIS 어댑터에서 nvm_dis_rpc_enable()로 RPC 서비스 활성화
 *    - request_accepter 콜백을 등록하여 모든 원격 Admin 명령 요청 수락
 * 5. SIGTERM/SIGINT 대기 (pthread_cond_wait 루프)
 * 6. 시그널 수신 후 Admin Queue 파괴 및 반환
 */
static int run_service(const nvm_ctrl_t* ctrl, const nvm_dma_t* q_wnd, const struct cl_args* args)
{
    int status;
    nvm_aq_ref rpc;

    /* Admin Queue 생성: 컨트롤러 리셋 후 ASQ/ACQ를 q_wnd에 매핑 */
    status = nvm_aq_create(&rpc, ctrl, q_wnd);
    if (status != 0)
    {
        fprintf(stderr, "Failed to register as manager: %s\n", strerror(status));
        return 2;
    }

    /* 선택: Identify Controller 명령으로 컨트롤러 정보 조회 및 출력 */
    if (args->identify_ctrl)
    {
        identify_controller(ctrl, args->ctrl_adapter, rpc);
    }

    /* SQ/CQ 개수 예약: NVMe Set Features (Feature ID = Number of Queues) 명령 */
    if (args->n_sqs > 0 && args->n_cqs > 0)
    {
        uint16_t n_sqs = args->n_sqs;
        uint16_t n_cqs = args->n_cqs;
        status = nvm_admin_request_num_queues(rpc, &n_cqs, &n_sqs);
        if (!nvm_ok(status))
        {
            fprintf(stderr, "Failed to run admin command for reserving IO queues: %s\n", nvm_strerror(status));
            nvm_aq_destroy(rpc);
            return 3;
        }

        /* 컨트롤러가 요청보다 적은 큐를 할당한 경우 에러 */
        if (n_sqs < args->n_sqs || n_cqs < args->n_cqs)
        {
            fprintf(stderr, "Attempted to reserve %u CQs and %u SQs, got %u CQs and %u SQs\n",
                    args->n_cqs, args->n_sqs, n_cqs, n_sqs);
            nvm_aq_destroy(rpc);
            return 3;
        }

        if (verbose)
        {
            fprintf(stderr, "Reserved %u CQs and %u SQs\n", n_cqs, n_sqs);
        }
    }

    /* 지정된 각 DIS 어댑터에서 RPC 서비스 활성화 */
    for (size_t i_adapter = 0; i_adapter < args->n_dis_adapters; ++i_adapter)
    {
        // Enable RPC on adapter
        status = nvm_dis_rpc_enable(rpc, args->dis_adapters[i_adapter], request_accepter);
        if (status != 0)
        {
            fprintf(stderr, "Unexpected error: %s\n", strerror(status));
        }
    }

    /* 메인 스레드를 시그널 대기 상태로 전환 (서버 데몬 모드) */
    // Set current thread in background
    pthread_mutex_lock(&signal_lk);
    signal(SIGTERM, (sig_t) catch_signal);
    signal(SIGINT, (sig_t) catch_signal);

    while ( ! signal_caught )
    {
        pthread_cond_wait(&signal_wq, &signal_lk);
    }
    pthread_mutex_unlock(&signal_lk);

    /* Admin Queue 파괴 (컨트롤러의 AQ 연결 해제) */
    nvm_aq_destroy(rpc);
    return 0;
}


/*
 * main - RPC 서버 메인 함수
 *
 * [실행 흐름]
 * 1. 커맨드라인 옵션 파싱
 * 2. SISCI 초기화
 * 3. nvm_dis_ctrl_init()으로 NVMe 컨트롤러 핸들 획득
 * 4. SISCI 세그먼트 생성 (세그먼트 ID=2, 크기=2페이지=8KB) -> Admin Queue 메모리용
 * 5. DMA 윈도우 매핑 (컨트롤러가 이 메모리에 DMA 가능하게)
 * 6. run_service()로 RPC 서비스 실행 (시그널까지 블로킹)
 * 7. 리소스 정리: DMA 해제 -> 세그먼트 제거 -> 컨트롤러 해제 -> SISCI 종료
 */
int main(int argc, char** argv)
{
    nvm_ctrl_t* ctrl;
    struct segment segment;
    nvm_dma_t* q_wnd;

    struct cl_args args;
    parse_opts(argc, argv, &args);

    sci_error_t err;
    SCIInitialize(0, &err);
    if (err != SCI_ERR_OK)
    {
        fprintf(stderr, "Failed to initialize SISCI: %s\n", SCIGetErrorString(err));
        exit(1);
    }

    // Get controller reference
    if (verbose)
    {
        fprintf(stderr, "Resetting controller...\n");
    }

    /* SmartIO FDID로 NVMe 컨트롤러 핸들 획득 */
    int status = nvm_dis_ctrl_init(&ctrl, args.smartio_dev_id);
    if (status != 0)
    {
        fprintf(stderr, "Failed to get controller reference: %s\n", strerror(status));
        exit(2);
    }

    /* Admin Queue용 SISCI 세그먼트 생성: 2페이지 (ASQ 1페이지 + ACQ 1페이지) */
    // Create queue memory
    status = segment_create(&segment, 2, ctrl->page_size * 2);
    if (status != 0)
    {
        nvm_ctrl_free(ctrl);
        exit(1);
    }

    /* 세그먼트를 NVMe 컨트롤러가 DMA 접근 가능한 윈도우로 매핑 */
    // Map queue as DMA window
    status = dma_create(&q_wnd, ctrl, &segment, args.ctrl_adapter);
    if (status != 0)
    {
        segment_remove(&segment);
        nvm_ctrl_free(ctrl);
        exit(status);
    }

    /* RPC 서비스 실행 (시그널 수신까지 블로킹) */
    // Run manager RPC service
    if (verbose)
    {
        fprintf(stderr, "Starting service...\n");
    }
    status = run_service(ctrl, q_wnd, &args);

    /* 리소스 정리 */
    // Destroy queue memory
    dma_remove(q_wnd, &segment, args.ctrl_adapter);
    segment_remove(&segment);

    // Put controller reference
    nvm_ctrl_free(ctrl);

    SCITerminate();

    fprintf(stderr, "Goodbye!\n");
    exit(status);
}


/*
 * parse_opts - 커맨드라인 옵션 파싱
 *
 * @argc: 인자 개수
 * @argv: 인자 배열
 * @args: 파싱 결과를 저장할 구조체
 *
 * 지원 옵션:
 *   --ctrl <fdid>:     SmartIO 디바이스 ID (16진수, 필수)
 *   --adapter <num>:   컨트롤러 어댑터 번호 (기본 0)
 *   --enable <num>:    RPC를 활성화할 DIS 어댑터 (여러 번 지정 가능)
 *   --sqs <num>:       예약할 SQ 개수
 *   --cqs <num>:       예약할 CQ 개수
 *   --identify:        시작 시 Identify Controller 실행
 *   --verbose:         상세 로그 출력
 *
 * [유효성 검증]
 * - --ctrl 미지정 시 에러 종료
 * - --enable 미지정 시 ctrl_adapter를 RPC 어댑터로 사용
 * - CQ/SQ 중 하나만 지정하면 에러 (둘 다 지정하거나 둘 다 미지정)
 */
static void parse_opts(int argc, char** argv, struct cl_args* args)
{
    static struct option opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "identify", no_argument, NULL, 1 },
        { "sqs", required_argument, NULL, 's' },
        { "cqs", required_argument, NULL, 'c' },
        { "enable", required_argument, NULL, 'r' },
        { "ctrl", required_argument, NULL, 'd' },
        { "adapter", required_argument, NULL, 'a' },
        { "verbose", no_argument, NULL, 'v' },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    int idx;

    memset(args, 0, sizeof(struct cl_args));

    // Parse arguments
    while ((opt = getopt_long(argc, argv, ":hvd:c:a:s:", opts, &idx)) != -1)
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

            case 'v': // set verbose
                verbose = true;
                break;

            case 1: // identify controller
                args->identify_ctrl = true;
                break;

            case 'd': // device identifier (16진수 SmartIO FDID)
                if (parse_u64(optarg, &args->smartio_dev_id, 16) != 0)
                {
                    fprintf(stderr, "Invalid device id: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('c');
                }
                break;

            case 'a': // device adapter (10진수)
                if (parse_u32(optarg, &args->ctrl_adapter, 10) != 0)
                {
                    fprintf(stderr, "Invalid adapter number: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('a');
                }
                break;

            case 'r': // RPC adapter number (여러 번 지정 가능, 최대 MAX_ADAPTS개)
                if (args->n_dis_adapters == MAX_ADAPTS)
                {
                    fprintf(stderr, "Maximum adapters reached, ignoring\n");
                    break;
                }

                if (parse_u32(optarg, &args->dis_adapters[args->n_dis_adapters], 0) != 0)
                {
                    fprintf(stderr, "Invalid adapter number: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('r');
                }
                args->n_dis_adapters++;
                break;

            case 's': // Set number of SQs to reserve
                if (parse_u16(optarg, &args->n_sqs, 0) != 0)
                {
                    fprintf(stderr, "Invalid number of IO submission queues: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('q');
                }
                break;

            case 'c': // Set number of CQs to reserve
                if (parse_u16(optarg, &args->n_cqs, 0) != 0)
                {
                    fprintf(stderr, "Invalid number of IO completion queues: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('q');
                }
                break;
        }
    }

    /* 컨트롤러 FDID 미지정 시 에러 */
    if (args->smartio_dev_id == 0)
    {
        fprintf(stderr, "No controller specified!\n");
        give_usage(argv[0]);
        exit('c');
    }

    /* RPC 어댑터 미지정 시 컨트롤러 어댑터를 기본값으로 사용 */
    if (args->n_dis_adapters == 0)
    {
        args->dis_adapters[args->n_dis_adapters++] = args->ctrl_adapter;
    }

    /* CQ/SQ는 둘 다 지정하거나 둘 다 미지정이어야 함 */
    if ((args->n_cqs > 0 && args->n_sqs == 0) || (args->n_sqs > 0 && args->n_cqs == 0))
    {
        fprintf(stderr, "Must specify either both number of submission and completion queues, or none at all\n");
        give_usage(argv[0]);
        exit('q');
    }
}


/*
 * identify_controller - Identify Controller Admin 명령 실행 및 결과 출력
 *
 * @ctrl:    NVMe 컨트롤러 핸들
 * @adapter: DIS 어댑터 번호 (DMA 윈도우 생성용)
 * @rpc:     Admin Queue 참조 (명령 제출용)
 *
 * 임시 SISCI 세그먼트(세그먼트 ID=3, 4KB)를 할당하여 Identify 결과를 수신하고,
 * print_ctrl_info()로 컨트롤러 정보를 stdout에 출력한 뒤 세그먼트를 해제한다.
 */
static void identify_controller(const nvm_ctrl_t* ctrl, uint32_t adapter, nvm_aq_ref rpc)
{
    int status;
    struct segment segment;
    nvm_dma_t* identify_wnd;
    struct nvm_ctrl_info info;

    /* Identify 결과 수신용 임시 세그먼트 생성 (4KB) */
    status = segment_create(&segment, 3, 0x1000);
    if (status != 0)
    {
        return;
    }

    status = dma_create(&identify_wnd, ctrl, &segment, adapter);
    if (status != 0)
    {
        segment_remove(&segment);
        return;
    }

    /* Admin Identify Controller 명령 실행 */
    status = nvm_admin_ctrl_info(rpc, &info, identify_wnd->vaddr, identify_wnd->ioaddrs[0]);
    if (status == 0)
    {
        print_ctrl_info(stdout, &info);
    }

    dma_remove(identify_wnd, &segment, adapter);
    segment_remove(&segment);
}


static void give_usage(const char* program_name)
{
    fprintf(stderr,
            "Usage: %s --ctrl <fdid> [--adapter <adapter>] [--enable <adapter>]...\n",
            program_name);
}


static void show_help(const char* program_name)
{
    give_usage(program_name);
    fprintf(stderr,
            "\nRun controller manager RPC server in a DIS cluster.\n\n"
            "    --ctrl             <fdid>      SmartIO device identifier.\n"
            "    --adapter          <adapter>   Local adapter to reach device (default is 0).\n"
            "    --enable           <adapter>   Enable RPC on adapter (defaults to controller adapter).\n"
            "    --cqs              <number>    Reserve number of completion queues (default is not to reserve).\n"
            "    --sqs              <number>    Reserver number of submission queues (default is not to reserve).\n"
            "    --identify                     Print controller information.\n"
            "    --verbose                      Print more information.\n"
            "    --help                         Show this information.\n"
            "\n");
}
