/*
 * integrity.c - NVMe 데이터 무결성 검증 예제 메인 (deprecated)
 *
 * 랜덤 데이터를 NVMe SSD에 Write한 뒤 Read하여 원본과 바이트 단위로 비교한다.
 * 커널 모듈 백엔드를 사용하여 /dev/libnvmX를 통해 NVMe 컨트롤러에 접근한다.
 *
 * 흐름: 컨트롤러 초기화 → Admin Queue 생성 → I/O Queue 생성 → Write → Read → 비교
 */
#include <nvm_types.h>    // libnvm 기본 타입
#include <nvm_ctrl.h>     // nvm_ctrl_init, nvm_dis_ctrl_init, nvm_ctrl_free
#include <nvm_dma.h>      // nvm_dma_create, nvm_dis_dma_create, nvm_dma_unmap
#include <nvm_aq.h>       // nvm_aq_create, nvm_aq_destroy
#include <nvm_admin.h>    // nvm_admin_ctrl_info, nvm_admin_ns_info, nvm_admin_request_num_queues
#include <nvm_error.h>    // nvm_ok, nvm_strerror
#include <stdio.h>        // fprintf, fopen, fclose, fseek, ftell, rewind
#include <stdint.h>       // uint64_t, uint32_t, uint16_t
#include <stddef.h>       // size_t
#include <stdlib.h>       // exit, strtoul, calloc, free, posix_memalign
#include <stdbool.h>      // bool
#include <getopt.h>       // getopt_long
#include <string.h>       // strerror, memset, memcpy
#include <errno.h>        // errno, ENOMEM

#ifdef __DIS_CLUSTER__
#include <sisci_api.h>        // SCIInitialize, SCITerminate (DIS 클러스터 모드)
#include <dis/dis_types.h>    // DIS_MAX_NSCIS
#define MAX_ADAPTERS DIS_MAX_NSCIS  // DIS 어댑터 최대 개수
#else
#include <fcntl.h>            // open, O_RDWR (로컬 모드)
#include <unistd.h>           // close
#endif

#include "integrity.h"    // buffer, queue, disk 구조체 및 함수 선언



/*
 * arguments - 커맨드라인 인자 구조체
 *
 * @device_id:    SmartIO FDID (DIS 클러스터 모드에서 사용)
 * @device_path:  /dev/libnvmX 경로 (로컬 모드에서 사용)
 * @ns_id:        NVMe 네임스페이스 ID (기본값: 1)
 * @n_queues:     사용할 I/O SQ 수 (기본값: 1)
 * @read_bytes:   읽기 모드 시 읽을 바이트 수 (0이면 쓰기 모드)
 * @filename:     읽기/쓰기 대상 파일 경로
 */
struct arguments
{
    uint64_t        device_id;     /* SmartIO FDID (DIS 모드에서 원격 NVMe 식별) */
    const char*     device_path;   /* /dev/libnvmX 경로 (로컬 모드에서 NVMe 접근) */
    uint32_t        ns_id;         /* NVMe 네임스페이스 ID (1부터 시작) */
    uint16_t        n_queues;      /* 병렬 I/O에 사용할 SQ 수 */
    uint64_t        read_bytes;    /* 읽기 바이트 수 (0이면 쓰기 모드로 동작) */
    const char*     filename;      /* 대상 파일 경로 (stdin/stdout 대신 사용) */
};



/* parse_number - 문자열을 uint64_t로 파싱하고 [lo, hi) 범위를 검증. 성공 시 true */
static bool parse_number(uint64_t* number, const char* str, int base, uint64_t lo, uint64_t hi)
{
    char* endptr = NULL;                         // strtoul 파싱 종료 위치
    uint64_t ul = strtoul(str, &endptr, base);   // 지정 진법으로 변환

    if (endptr == NULL || *endptr != '\0')       // 문자열 전체가 유효한 숫자인지 확인
    {
        return false;
    }

    if (lo < hi && (ul < lo || ul >= hi))        // 범위 검증 (lo <= ul < hi)
    {
        return false;
    }

    *number = ul;
    return true;  // 파싱 성공
}



/*
 * parse_arguments - 커맨드라인 옵션 파싱
 *
 * 필수: --ctrl <path/fdid>, filename (위치 인자)
 * 선택: --read <bytes>, --namespace <id>, --queues <count>
 *
 * --read가 지정되면 읽기 모드 (디스크 -> 파일), 아니면 쓰기 모드 (파일 -> 디스크)
 */
static void parse_arguments(int argc, char** argv, struct arguments* args)
{
    struct option opts[] = {
        { "help", no_argument, NULL, 'h' },              // --help: 도움말
        { "read", required_argument, NULL, 'r' },        // --read: 읽기 바이트 수
        { "ctrl", required_argument, NULL, 'c' },        // --ctrl: 컨트롤러 경로/FDID
        { "namespace", required_argument, NULL, 'n' },   // --namespace: NS ID
        { "queues", required_argument, NULL, 'q' },      // --queues: SQ 수
        { NULL, 0, NULL, 0 }
    };

    int opt;       // getopt 반환값
    int idx;       // 옵션 인덱스
    uint64_t num;  // parse_number 결과 임시 저장

    // 기본값 초기화
    args->device_id = 0;
    args->device_path = NULL;
    args->ns_id = 1;          // 기본 네임스페이스 ID
    args->n_queues = 1;       // 기본 SQ 수
    args->read_bytes = 0;     // 0이면 쓰기 모드
    args->filename = NULL;

    while ((opt = getopt_long(argc, argv, ":hr:c:n:q:", opts, &idx)) != -1)  // 옵션 루프
    {
        switch (opt)
        {
            case '?': // 알 수 없는 옵션
            default:
                fprintf(stderr, "Unknown option: `%s'\n", argv[optind - 1]);
                exit(4);

            case ':': // 옵션 인자 누락
                fprintf(stderr, "Missing argument for option `%s'\n", argv[optind - 1]);
                exit(4);

            case 'h': // --help
                fprintf(stderr, "Usage: %s --ctrl=device-path [--read=bytes] [-n namespace] [-q queues] filename\n", argv[0]);
                exit(4);

            case 'r': // --read: 읽기 바이트 수 (0보다 커야 함)
                if ( !parse_number(&args->read_bytes, optarg, 0, 0, 0) || args->read_bytes == 0 )
                {
                    fprintf(stderr, "Invalid number of bytes: `%s'\n", optarg);
                    exit(1);
                }
                break;

            case 'c': // --ctrl: 컨트롤러 식별자 (DIS=FDID 정수, 로컬=경로 문자열)
#ifdef __DIS_CLUSTER__
                if (! parse_number(&args->device_id, optarg, 0, 0, 0) )  // DIS 모드: 정수 파싱
                {
                    fprintf(stderr, "Invalid controller identifier: `%s'\n", optarg);
                    exit(1);
                }
#else
                args->device_path = optarg;  // 로컬 모드: 경로 문자열 저장
#endif
                break;

            case 'n': // --namespace: 네임스페이스 ID
                if (! parse_number(&num, optarg, 0, 0, 0) )
                {
                    fprintf(stderr, "Invalid controller identifier: `%s'\n", optarg);
                    exit(3);
                }
                args->ns_id = (uint32_t) num;  // uint64 -> uint32 변환
                break;

            case 'q': // --queues: I/O SQ 수 (범위: 1~65534)
                if (! parse_number(&num, optarg, 0, 1, 0xffff) )
                {
                    fprintf(stderr, "Invalid number of queues: `%s'\n", optarg);
                    exit(3);
                }
                args->n_queues = (uint16_t) num;
                break;
        }
    }

    // 위치 인자 처리 (옵션이 아닌 나머지 인자)
    argc -= optind;  // 남은 인자 개수
    argv += optind;  // 남은 인자 시작 위치

#ifdef __DIS_CLUSTER__
    if (args->device_id == 0)  // DIS 모드: FDID 미지정 검증
    {
        fprintf(stderr, "No controller specified!\n");
        exit(1);
    }
#else
    if (args->device_path == NULL)  // 로컬 모드: 경로 미지정 검증
    {
        fprintf(stderr, "No controller specified!\n");
        exit(1);
    }
#endif

    if (argc < 1)  // 파일명 미지정
    {
        fprintf(stderr, "File not specified!\n");
        exit(2);
    }
    else if (argc > 1)  // 파일명 2개 이상
    {
        fprintf(stderr, "More than one filename specified!\n");
        exit(2);
    }

    args->filename = argv[0];  // 위치 인자를 파일명으로 저장
}


/* print_ctrl_info - Identify Controller 결과를 사람이 읽을 수 있는 형식으로 출력 */
static void print_ctrl_info(FILE* fp, const struct nvm_ctrl_info* info)
{
    // PCI Vendor/Subsystem Vendor ID를 바이트 배열로 추출
    unsigned char vendor[4];
    memcpy(vendor, &info->pci_vendor, sizeof(vendor));

    // NVMe 스펙 고정 길이 문자열을 null 종료 처리
    char serial[21];
    memset(serial, 0, 21);
    memcpy(serial, info->serial_no, 20);  // 시리얼 번호 20바이트

    char model[41];
    memset(model, 0, 41);
    memcpy(model, info->model_no, 40);    // 모델 번호 40바이트

    char revision[9];
    memset(revision, 0, 9);
    memcpy(revision, info->firmware, 8);  // 펌웨어 리비전 8바이트

    fprintf(fp, "------------- Controller information -------------\n");
    fprintf(fp, "PCI Vendor ID           : %x %x\n", vendor[0], vendor[1]);
    fprintf(fp, "PCI Subsystem Vendor ID : %x %x\n", vendor[2], vendor[3]);
    fprintf(fp, "NVM Express version     : %u.%u.%u\n",
            info->nvme_version >> 16, (info->nvme_version >> 8) & 0xff, info->nvme_version & 0xff);
    fprintf(fp, "Controller page size    : %zu\n", info->page_size);
    fprintf(fp, "Max queue entries       : %u\n", info->max_entries);
    fprintf(fp, "Serial Number           : %s\n", serial);
    fprintf(fp, "Model Number            : %s\n", model);
    fprintf(fp, "Firmware revision       : %s\n", revision);
    fprintf(fp, "Max data transfer size  : %zu\n", info->max_data_size);
    fprintf(fp, "Max outstanding commands: %zu\n", info->max_out_cmds);
    fprintf(fp, "Max number of namespaces: %zu\n", info->max_n_ns);
    fprintf(fp, "--------------------------------------------------\n");
}


/*
 * identify_controller - Identify Controller 명령을 실행하여 디스크 정보 획득
 *
 * @ref:  Admin Queue 참조
 * @disk: 결과를 저장할 disk 구조체 (page_size, max_data_size)
 * @return: 성공 시 0, 실패 시 에러 코드
 */
static int identify_controller(nvm_aq_ref ref, struct disk* disk)
{
    int status;
    nvm_dma_t* window;             // Identify 결과 수신용 임시 DMA 버퍼
    struct nvm_ctrl_info info;     // Identify Controller 결과 구조체

    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);  // AQ에서 컨트롤러 핸들 추출

#ifdef __DIS_CLUSTER__
    // DIS 모드: SISCI 세그먼트로 1페이지 DMA 할당 (호스트 읽기 + 디바이스 쓰기 힌트)
    status = nvm_dis_dma_create(&window, ctrl, ctrl->page_size,
            SCI_MEMACCESS_HOST_READ | SCI_MEMACCESS_DEVICE_WRITE);
#else
    // 로컬 모드: 커널 모듈을 통한 1페이지 DMA 할당
    status = nvm_dma_create(&window, ctrl, ctrl->page_size);
#endif
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create buffer: %s\n", nvm_strerror(status));
        return status;
    }

    // Admin Identify Controller 명령 실행 (Opcode 0x06, CNS=1)
    status = nvm_admin_ctrl_info(ref, &info, window->vaddr, window->ioaddrs[0]);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to identify controller: %s\n", nvm_strerror(status));
    }
    nvm_dma_unmap(window);  // 임시 DMA 버퍼 해제

    // 디스크 정보 구조체에 결과 저장
    disk->page_size = info.page_size;            // 컨트롤러 페이지 크기
    disk->max_data_size = info.max_data_size;    // MDTS (최대 데이터 전송 크기)

    print_ctrl_info(stderr, &info);  // 컨트롤러 정보를 stderr에 출력

    return status;
}


/*
 * identify_namespace - Identify Namespace 명령을 실행하여 블록 크기 등 획득
 *
 * @ref:   Admin Queue 참조
 * @disk:  결과를 저장할 disk 구조체 (ns_id, block_size)
 * @ns_id: 조회할 네임스페이스 ID
 * @return: 성공 시 0, 실패 시 에러 코드
 */
static int identify_namespace(nvm_aq_ref ref, struct disk* disk, uint32_t ns_id)
{
    int status;
    nvm_dma_t* window;            // Identify 결과 수신용 임시 DMA 버퍼
    struct nvm_ns_info info;      // Identify Namespace 결과 구조체

    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);  // 컨트롤러 핸들 추출

#ifdef __DIS_CLUSTER__
    status = nvm_dis_dma_create(&window, ctrl, ctrl->page_size,
            SCI_MEMACCESS_HOST_READ | SCI_MEMACCESS_DEVICE_WRITE);
#else
    status = nvm_dma_create(&window, ctrl, ctrl->page_size);
#endif
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create buffer: %s\n", nvm_strerror(status));
        return status;
    }

    // Admin Identify Namespace 명령 실행 (Opcode 0x06, CNS=0)
    status = nvm_admin_ns_info(ref, &info, ns_id, window->vaddr, window->ioaddrs[0]);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to identify namespace: %s\n", nvm_strerror(status));
    }

    // 네임스페이스 정보 저장
    disk->ns_id = info.ns_id;                  // NSID
    disk->block_size = info.lba_data_size;     // LBA 크기 (보통 512B 또는 4KB)

    nvm_dma_unmap(window);  // 임시 DMA 버퍼 해제
    return status;
}


/* remove_queues - 모든 I/O 큐(CQ + SQ들) 해제 및 메모리 free */
static void remove_queues(struct queue* queues, uint16_t n_queues)
{
    uint16_t i;

    if (queues != NULL)
    {
        for (i = 0; i < n_queues + 1; ++i)  // CQ(인덱스 0) + SQ(인덱스 1~n) 모두 해제
        {
            remove_queue(&queues[i]);
        }

        free(queues);  // 큐 배열 메모리 해제
    }
}



/*
 * request_queues - I/O CQ 1개 + SQ n개를 생성
 *
 * @ref:    Admin Queue 참조
 * @args:   커맨드라인 옵션 (n_queues)
 * @queues: 결과 큐 배열 포인터 (인덱스 0=CQ, 1~n=SQ)
 * @return: 성공 시 0, 실패 시 에러 코드
 *
 * 1. Set Features(Number of Queues)로 큐 수 예약
 * 2. CQ 1개 생성 (인덱스 0)
 * 3. SQ n_queues개 생성 (인덱스 1~n, 모두 같은 CQ에 연결)
 */
static int request_queues(nvm_aq_ref ref, struct arguments* args, struct queue** queues)
{
    struct queue* q;
    *queues = NULL;
    uint16_t i;

    uint16_t n_cqs = 1;              // CQ 1개 요청
    uint16_t n_sqs = args->n_queues; // SQ n개 요청

    // Set Features (Feature ID 0x07: Number of Queues)로 큐 수 예약
    int status = nvm_admin_request_num_queues(ref, &n_cqs, &n_sqs);

    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to request queues: %s\n", nvm_strerror(status));
        return status;
    }

    if (n_sqs < args->n_queues)  // 컨트롤러가 요청보다 적은 SQ를 할당한 경우
    {
        fprintf(stderr, "Requested too many queues, controller only supports %u queues\n", n_sqs);
        return ENOMEM;
    }

    // 큐 배열 할당: CQ 1개 + SQ n_queues개 = (n_queues + 1)개
    q = calloc(args->n_queues + 1, sizeof(struct queue));
    if (q == NULL)
    {
        fprintf(stderr, "Failed to allocate queues: %s\n", strerror(errno));
        return ENOMEM;
    }

    // CQ 생성 (인덱스 0, 큐 번호 1)
    status = create_queue(&q[0], ref, NULL, 1);  // cq=NULL이면 CQ 생성
    if (status != 0)
    {
        free(q);
        return status;
    }

    // SQ 생성 (인덱스 1~n, 모두 같은 CQ에 연결)
    for (i = 0; i < args->n_queues; ++i)
    {
        status = create_queue(&q[i + 1], ref, &q[0], i+1);  // q[0]의 CQ에 연결된 SQ 생성
        if (status != 0)
        {
            remove_queues(q, i);  // 이미 생성된 큐들 정리
            return status;
        }
    }

    *queues = q;  // 결과 큐 배열 반환
    return status;
}



int main(int argc, char** argv)
{
#ifdef __DIS_CLUSTER__
    sci_error_t err;  // SISCI 에러 코드
#endif
    struct arguments args;     // 커맨드라인 옵션
    nvm_ctrl_t* ctrl;          // NVMe 컨트롤러 핸들
    int status;                // 함수 반환값
    nvm_dma_t* aq_dma;        // Admin Queue DMA 메모리
    nvm_aq_ref aq_ref;         // Admin Queue 참조
    struct disk disk;          // 디스크 정보 (MDTS, 블록 크기 등)
    struct queue* queues = NULL;  // I/O 큐 배열
    struct buffer buffer;      // 데이터 DMA 버퍼

    // 커맨드라인 인자 파싱
    parse_arguments(argc, argv, &args);

    // 파일 열기: 읽기 모드(read_bytes > 0)이면 "w", 쓰기 모드이면 "r"
    FILE* fp = fopen(args.filename, args.read_bytes ? "w" : "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open file `%s': %s\n",
                args.filename, strerror(errno));
        exit(2);
    }

    // 전송할 바이트 수 결정
    off_t file_size = 0;
    if (args.read_bytes > 0)  // 읽기 모드: 지정된 바이트 수만큼 디스크에서 읽기
    {
        file_size = args.read_bytes;
        fprintf(stderr, "Reading from disk and dumping to file `%s' (%lu bytes)\n", args.filename, (unsigned long)file_size);
    }
    else  // 쓰기 모드: 파일 크기만큼 디스크에 쓰기
    {
        fseek(fp, 0, SEEK_END);    // 파일 끝으로 이동
        file_size = ftell(fp);     // 현재 위치 = 파일 크기
        rewind(fp);                // 파일 시작으로 되돌리기

        if (file_size == 0)
        {
            fprintf(stderr, "File `%s' is empty!\n", args.filename);
            fclose(fp);
            exit(2);
        }

        fprintf(stderr, "Reading from file `%s' and writing to disk (%lu bytes)\n", args.filename, (unsigned long)file_size);
    }


#ifdef __DIS_CLUSTER__
    // DIS 모드: SISCI API 초기화
    SCIInitialize(0, &err);
#endif

    // NVMe 컨트롤러 핸들 획득
#ifdef __DIS_CLUSTER__
    status = nvm_dis_ctrl_init(&ctrl, args.device_id);  // DIS 모드: SmartIO FDID로 원격 접근
    if (status != 0)
    {
        fclose(fp);
        fprintf(stderr, "Failed to get controller reference: %s\n", strerror(status));
        exit(1);
    }
#else
    int fd = open(args.device_path, O_RDWR | O_NONBLOCK);  // 로컬 모드: /dev/libnvmX 열기
    if (fd < 0)
    {
        fclose(fp);
        fprintf(stderr, "Failed to open device file: %s\n", strerror(errno));
        exit(1);
    }

    status = nvm_ctrl_init(&ctrl, fd);  // 커널 모듈 경유 컨트롤러 핸들 획득
    if (status != 0)
    {
        close(fd);
        fclose(fp);
        fprintf(stderr, "Failed to get controller reference: %s\n", strerror(status));
    }

    close(fd);  // fd는 더 이상 불필요

#endif

    // Admin Queue DMA 메모리 할당 (2페이지: ASQ + ACQ)
#ifdef __DIS_CLUSTER__
    status = nvm_dis_dma_create(&aq_dma, ctrl, ctrl->page_size * 2, 0);  // SISCI 세그먼트
#else
    status = nvm_dma_create(&aq_dma, ctrl, ctrl->page_size * 2);         // 커널 모듈 경유 DMA
#endif
    if (status != 0)
    {
        nvm_ctrl_free(ctrl);
        fclose(fp);
        fprintf(stderr, "Failed to create admin queue pair: %s\n", strerror(status));
        exit(1);
    }

    // 컨트롤러 리셋 + Admin Queue 생성
    fprintf(stderr, "Resetting controller and configuring admin queue pair...\n");
    status = nvm_aq_create(&aq_ref, ctrl, aq_dma);  // CC.EN=0 → AQA/ASQ/ACQ 설정 → CC.EN=1
    if (status != 0)
    {
        nvm_dma_unmap(aq_dma);
        nvm_ctrl_free(ctrl);
        fclose(fp);
        fprintf(stderr, "Failed to create admin queue pair: %s\n", strerror(status));
        exit(1);
    }

    // 데이터 DMA 버퍼 할당 (파일 크기를 컨트롤러 정렬로 올림)
    status = create_buffer(&buffer, aq_ref, NVM_CTRL_ALIGN(ctrl, file_size));
    if (status != 0)
    {
        nvm_aq_destroy(aq_ref);
        nvm_dma_unmap(aq_dma);
        nvm_ctrl_free(ctrl);
        fclose(fp);
        exit(1);
    }


    // Identify Controller로 페이지 크기, MDTS 획득
    status = identify_controller(aq_ref, &disk);
    if (status != 0)
    {
        goto out;
    }

    // Identify Namespace로 블록 크기, NSID 획득
    status = identify_namespace(aq_ref, &disk, args.ns_id);
    if (status != 0)
    {
        goto out;
    }

    // SQ 수가 전체 블록 수보다 크면 블록 수로 제한
    if (args.n_queues > NVM_PAGE_ALIGN(file_size, disk.block_size) / disk.block_size)
    {
        args.n_queues = NVM_PAGE_ALIGN(file_size, disk.block_size) / disk.block_size;
    }

    // I/O 큐 생성: CQ 1개 + SQ n_queues개
    status = request_queues(aq_ref, &args, &queues);
    if (status != 0)
    {
        goto out;
    }

    fprintf(stderr, "Using %u submission queues:\n", args.n_queues);

    // 읽기 모드 또는 쓰기 모드에 따라 전송 실행
    if (args.read_bytes > 0)
    {
        status = disk_read(&disk, &buffer, queues, args.n_queues, fp, file_size);  // NVMe Read → 파일
    }
    else
    {
        status = disk_write(&disk, &buffer, queues, args.n_queues, fp, file_size);  // 파일 → NVMe Write
    }

out:
    // 리소스 역순 해제
    remove_queues(queues, args.n_queues);  // I/O 큐 해제
    nvm_aq_destroy(aq_ref);                // Admin Queue 파괴
    nvm_dma_unmap(aq_dma);                 // Admin Queue DMA 해제
    remove_buffer(&buffer);                // 데이터 버퍼 해제
    nvm_ctrl_free(ctrl);                   // 컨트롤러 핸들 해제
    fclose(fp);                            // 파일 닫기
#ifdef __DIS_CLUSTER__
    SCITerminate();                        // SISCI 라이브러리 종료
#endif
    exit(status);
}
