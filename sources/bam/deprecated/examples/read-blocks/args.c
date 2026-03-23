/*
 * args.c - NVMe 블록 읽기 예제 커맨드라인 인자 파서 (deprecated)
 *
 * getopt_long으로 커맨드라인 인자를 파싱한다.
 * --ctrl: NVMe 컨트롤러 경로 또는 DIS FDID
 * --blocks: 읽을 블록 수, --offset: 시작 블록 오프셋
 * --output: 출력 파일, --input: 입력 파일 (Write시)
 * --chunk: 청크 크기, --queue-size: 큐 깊이
 * --ascii: hex 대신 ASCII 출력, --identify: Identify만 수행
 */
#include "args.h"       // options 구조체 정의 및 parse_options() 선언
#include <stdlib.h>     // exit(), posix_memalign(), strtoul() 등 표준 라이브러리
#include <stdio.h>      // fprintf(), stderr, FILE* 등 표준 I/O
#include <getopt.h>     // getopt_long(): GNU 확장 커맨드라인 파싱 함수
#include <fcntl.h>      // O_RDWR, O_NONBLOCK 등 파일 제어 플래그 (여기선 미사용이지만 관례적 포함)
#include <limits.h>     // UINT_MAX 등 정수 한계값 상수
#include <errno.h>      // errno 전역 변수, EINVAL 등 에러 코드
#include <string.h>     // strerror(): 에러 코드를 문자열로 변환



// getopt_long()에 전달할 옵션 정의 배열 (long option -> short option 매핑)
// .name: "--옵션이름", .has_arg: 인자 필요 여부, .val: getopt_long 반환값
static struct option opts[] = {
    { .name = "help", .has_arg = no_argument, .flag = NULL, .val = 'h' },           // --help: 도움말 출력 후 종료
    { .name = "ctrl", .has_arg = required_argument, .flag = NULL, .val = 'c' },     // --ctrl: NVMe 컨트롤러 경로(로컬) 또는 FDID(DIS)
    { .name = "namespace", .has_arg = required_argument, .flag = NULL, .val = 'n' },// --namespace: NVMe 네임스페이스 ID (기본 1)
    { .name = "ns", .has_arg = required_argument, .flag = NULL, .val = 'n' },       // --ns: --namespace의 축약형
    { .name = "blocks", .has_arg = required_argument, .flag = NULL, .val = 'b' },   // --blocks: 읽을 블록 수 (필수)
    { .name = "offset", .has_arg = required_argument, .flag = NULL, .val = 'o' },   // --offset: 시작 LBA 오프셋 (기본 0)
    { .name = "output", .has_arg = required_argument, .flag = NULL, .val = 0 },     // --output: 출력 파일 경로 (val=0이면 idx로 구분)
    { .name = "ascii", .has_arg = no_argument, .flag = NULL, .val = 2 },            // --ascii: hex 대신 ASCII 텍스트로 출력 (val=2)
    { .name = "identify", .has_arg = no_argument, .flag = NULL, .val = 3 },         // --identify: Identify Controller 정보 출력 (val=3)
    { .name = "chunk", .has_arg = required_argument, .flag = NULL, .val = 's' },    // --chunk: 한 번에 전송할 블록 수 (MDTS 이내)
    { .name = "depth", .has_arg = required_argument, .flag = NULL, .val = 'q' },    // --depth: SQ 깊이 (기본 64)
    { .name = "write", .has_arg = required_argument, .flag = NULL, .val = 'w' },    // --write: 입력 파일 → NVMe Write 후 Read
    { .name = NULL, .has_arg = no_argument, .flag = NULL, .val = 0 }                // 배열 종료 센티널 (NULL name)
};



// show_usage - 간략한 사용법을 stderr에 출력
static void show_usage(const char* name)
{
#ifdef __DIS_CLUSTER__
    // DIS 클러스터 모드: --ctrl에 FDID(16진수 디바이스 ID) 지정
    fprintf(stderr, "Usage: %s --ctrl <device id> --blocks <count> [--offset <count>] [--ns <id>] [--ascii | --output <path>]\n", name);
#else
    // 로컬 모드: --ctrl에 /dev/libnvmX 디바이스 경로 지정
    fprintf(stderr, "Usage: %s --ctrl <path> --blocks <count> [--offset <count>] [--ns <id>] [--ascii | --output <path>]\n", name);
#endif
}



// show_help - 상세 도움말을 stderr에 출력 (각 옵션의 설명 포함)
static void show_help(const char* name)
{
    show_usage(name);   // 먼저 사용법 한 줄 출력

    fprintf(stderr, ""
#ifdef __DIS_CLUSTER__
            "    --ctrl         <id>      Specify controller's device identifier.\n"    // DIS: SmartIO FDID 지정
#else
            "    --ctrl         <path>    Specify path to controller.\n"                 // 로컬: /dev/libnvmX 경로
#endif
            "    --chunk        <count>   Limit reads to a number of blocks at the time.\n"  // 한 번에 읽을 블록 수 제한
            "    --depth        <count>   Set submission queue depth.\n"                      // SQ 엔트리 수 설정
            "    --blocks       <count>   Read specified number of blocks from disk.\n"       // 총 읽을 블록 수
            "    --offset       <count>   Start reading at specified block (default 0).\n"    // 시작 LBA
            "    --namespace    <id>      Namespace identifier (default 1).\n"                // 네임스페이스 ID
            "    --ascii                  Show output of ASCII characters as text.\n"         // ASCII 텍스트 모드
            "    --output       <path>    Dump to file rather than stdout.\n"                 // 파일로 바이너리 덤프
            "    --write        <path>    Read file and write to disk before reading back.\n" // 파일 → Write → Read 모드
            "    --identify               Show IDENTIFY CONTROLLER structure.\n"              // Identify Controller 출력
           );
}



/*
 * parse_options - 커맨드라인 옵션을 파싱하여 options 구조체에 저장
 *
 * @argc: 인자 개수
 * @argv: 인자 배열
 * @args: 결과를 저장할 options 구조체
 *
 * [필수 옵션]
 * --ctrl: NVMe 컨트롤러 (로컬: 경로, DIS: FDID 16진수)
 * --blocks: 읽을 총 블록 수
 *
 * [선택 옵션]
 * --offset: 시작 LBA (기본 0)
 * --chunk: 한 번에 전송할 블록 수 (기본=num_blocks)
 * --depth: SQ 깊이 (기본 64)
 * --namespace: 네임스페이스 ID (기본 1)
 * --ascii: ASCII 텍스트 출력 모드
 * --output: 파일 출력
 * --write: 쓰기 후 읽기 모드
 * --identify: Identify Controller 정보 출력
 */
void parse_options(int argc, char** argv, struct options* args)
{
#ifdef __DIS_CLUSTER__
    // DIS 모드 옵션 문자열: 'a'(adapter)가 추가됨
    const char* argstr = ":hc:a:n:b:o:s:w:q:";
#else
    // 로컬 모드 옵션 문자열: 콜론(:) 접두사는 누락 인자를 ':'로 반환하겠다는 의미
    const char* argstr = ":hc:b:n:o:s:w:q:";
#endif

    int opt;        // getopt_long()이 반환하는 현재 옵션 문자
    int idx;        // long option 배열에서의 인덱스 (사용하지 않지만 getopt_long에 필요)
    char* endptr;   // strtoul()이 파싱을 멈춘 위치 (유효성 검증용)

    // 모든 필드를 기본값으로 초기화
#ifdef __DIS_CLUSTER__
    args->controller_id = 0;        // DIS: FDID 0은 "미지정" 표시
#else
    args->controller_path = NULL;   // 로컬: NULL은 "미지정" 표시
#endif
    args->queue_size = 0;           // 0이면 나중에 기본값 64로 설정
    args->chunk_size = 0;           // 0이면 나중에 num_blocks와 동일하게 설정
    args->namespace_id = 1;         // NVMe 네임스페이스 ID 기본값 1 (대부분의 SSD는 NS 1개)
    args->num_blocks = 0;           // 0이면 필수 옵션 누락 에러
    args->offset = 0;              // 시작 LBA 기본값 0 (디스크 처음부터)
    args->output = NULL;            // NULL이면 stdout에 hex/ascii 덤프
    args->input = NULL;             // NULL이면 Write 안 하고 Read만
    args->ascii = false;            // false이면 hex 덤프 모드
    args->identify = false;         // false이면 Identify Controller 정보 생략

    // getopt_long 루프: 모든 옵션을 순회하며 파싱
    while ((opt = getopt_long(argc, argv, argstr, opts, &idx)) != -1)
    {
        switch (opt)
        {
            case '?':  // 알 수 없는 옵션
                fprintf(stderr, "Unknown option: `%s'\n", argv[optind - 1]);
                exit('?');

            case ':':  // 인자가 필요한 옵션인데 인자가 누락됨
                fprintf(stderr, "Missing argument for option %s\n", argv[optind - 1]);
                exit('?');

            case 'h':  // --help: 도움말 출력 후 종료
                show_help(argv[0]);
                exit('?');

            case 0:    // --output: val이 0인 long option → 출력 파일 설정
                if (args->ascii)    // --ascii가 이미 설정되어 있으면 무시
                {
                    fprintf(stderr, "Output file is set, ignoring option --ascii\n");
                    args->ascii = false;    // 파일 출력이 우선, ASCII 모드 해제
                }

                args->output = fopen(optarg, "wb");     // 바이너리 쓰기 모드로 출력 파일 열기
                if (args->output == NULL)               // fopen 실패 시 에러 출력 후 종료
                {
                    fprintf(stderr, "Failed to open output file: %s\n", strerror(errno));
                    exit(1);
                }
                break;

            case 'w':  // --write: 입력 파일 → NVMe Write 후 Read
                args->input = fopen(optarg, "rb");      // 바이너리 읽기 모드로 입력 파일 열기
                if (args->input == NULL)                // fopen 실패 시 에러 출력 후 종료
                {
                    fprintf(stderr, "Failed to open input file: %s\n", strerror(errno));
                    exit(1);
                }
                break;


            case 3:    // --identify: Identify Controller 정보 출력 모드 활성화
                args->identify = true;
                break;

#ifdef __DIS_CLUSTER__
            case 'c':  // --ctrl (DIS 모드): FDID를 16진수로 파싱
                args->controller_id = strtoul(optarg, &endptr, 16);     // 16진수 파싱 (예: "0xc0c00")
                if (endptr == NULL || *endptr != '\0')                  // 문자열 전체가 유효한 숫자가 아니면 에러
                {
                    fprintf(stderr, "Invalid controller id: `%s'\n", optarg);
                    exit(1);
                }

                if (args->controller_id == 0)   // FDID 0은 유효하지 않음
                {
                    fprintf(stderr, "Controller id can not be 0!\n");
                    exit(1);
                }
                break;
#else
            case 'c':  // --ctrl (로컬 모드): /dev/libnvmX 경로를 그대로 저장
                args->controller_path = optarg;     // optarg는 argv[] 내부 포인터이므로 복사 불필요
                break;
#endif

            case 'n':  // --namespace / --ns: NVMe 네임스페이스 ID 파싱
                args->namespace_id = strtoul(optarg, &endptr, 0);   // base=0: 접두사로 자동 진법 감지
                if (endptr == NULL || *endptr != '\0' || args->namespace_id == 0xffffffff)  // 0xFFFFFFFF는 브로드캐스트 NSID로 예약됨
                {
                    fprintf(stderr, "Invalid namespace identifier: `%s'\n", optarg);
                    exit(2);
                }
                break;

            case 2:    // --ascii: hex 대신 ASCII 텍스트 출력 모드
                if (args->output == NULL)       // 출력 파일이 없을 때만 ASCII 모드 설정
                {
                    args->ascii = true;
                }
                else    // 출력 파일이 이미 설정되어 있으면 ASCII 무시 (바이너리 출력 우선)
                {
                    fprintf(stderr, "Output file is set, ignoring option %s\n", argv[optind - 1]);
                }
                break;

            case 'o':  // --offset: 시작 LBA 오프셋
                args->offset = strtoul(optarg, &endptr, 0);    // base=0: 자동 진법 감지
                if (endptr == NULL || *endptr != '\0')
                {
                    fprintf(stderr, "Invalid block count: `%s'\n", optarg);
                    exit(2);
                }
                break;

            case 'b':  // --blocks: 읽을 총 블록 수 (필수)
                args->num_blocks = strtoul(optarg, &endptr, 0);
                if (endptr == NULL || *endptr != '\0')
                {
                    fprintf(stderr, "Invalid block count: `%s'\n", optarg);
                    exit(2);
                }

                if (args->num_blocks == 0)  // 0 블록 읽기는 의미 없으므로 에러
                {
                    fprintf(stderr, "Number of blocks can not be 0!\n");
                    exit(2);
                }
                break;

            case 's':  // --chunk: 한 번에 전송할 블록 수 (MDTS 이내여야 함)
                args->chunk_size = strtoul(optarg, &endptr, 0);
                if (endptr == NULL || *endptr != '\0')
                {
                    fprintf(stderr, "Invalid block count: `%s'\n", optarg);
                    exit(2);
                }
                break;

            case 'q':  // --depth: SQ 깊이 (엔트리 수)
                args->queue_size = strtoul(optarg, &endptr, 0);
                if (endptr == NULL || *endptr != '\0')
                {
                    fprintf(stderr, "Invalid queue depth: `%s'\n", optarg);
                    exit(2);
                }
                break;
        }
    }

    // 필수 옵션 검증: 컨트롤러가 지정되었는지 확인
#ifdef __DIS_CLUSTER__
    if (args->controller_id == 0)   // DIS: FDID가 0이면 미지정
    {
        fprintf(stderr, "No controller specified!\n");
        show_usage(argv[0]);
        exit(1);
    }
#else
    if (args->controller_path == NULL)  // 로컬: 경로가 NULL이면 미지정
    {
        fprintf(stderr, "No controller specified!\n");
        show_usage(argv[0]);
        exit(1);
    }
#endif

    // 필수 옵션 검증: 블록 수가 지정되었는지 확인
    if (args->num_blocks == 0)
    {
        fprintf(stderr, "Block count is not specified!\n");
        show_usage(argv[0]);
        exit(2);
    }

    // 선택 옵션 기본값 설정: chunk_size를 지정하지 않으면 전체 블록 수와 동일
    if (args->chunk_size == 0)
    {
        args->chunk_size = args->num_blocks;
    }

    // 선택 옵션 기본값 설정: queue_size를 지정하지 않으면 64
    if (args->queue_size == 0)
    {
        args->queue_size = 64;
    }
}
