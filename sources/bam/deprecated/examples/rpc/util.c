/*
 * util.c - RPC 예제 공통 유틸리티 구현 (Deprecated)
 *
 * [파일 역할]
 * 커맨드라인 인자 파싱(문자열->정수 변환)과 NVMe Identify 결과를
 * 사람이 읽기 쉬운 형태로 출력하는 공통 유틸리티 함수 구현.
 * 모든 RPC 예제 프로그램이 이 함수들을 사용한다.
 *
 * [Deprecated 배경]
 * SISCI 기반 RPC NVMe 접근 예제의 일부로, BaM이 커널 모듈 기반
 * GPU-initiated I/O로 전환되면서 함께 deprecated되었다.
 */
#include "util.h"           // 이 파일의 함수 선언 (parse_u64, parse_u32 등)
#include <stdint.h>         // uint16_t, uint32_t, uint64_t: 고정 크기 정수 타입
#include <stdlib.h>         // strtoul(): 문자열 → unsigned long 변환
#include <stdio.h>          // fprintf(), FILE*: 출력 함수
#include <time.h>           // time 관련 (미사용이지만 관례적 포함)
#include <sys/time.h>       // gettimeofday 관련 (미사용이지만 관례적 포함)
#include <string.h>         // memcpy(), memset(): NVMe 문자열 필드 복사
#include <errno.h>          // EINVAL: 잘못된 인자 에러 코드
#include <nvm_types.h>      // nvm_ctrl_info, nvm_ns_info: NVMe Identify 결과 구조체
#include <limits.h>         // UINT_MAX: uint32_t 최대값 (4,294,967,295)


/*
 * parse_u64 - 문자열을 uint64_t로 파싱
 *
 * @str:    변환할 문자열 (예: "0xc0c00", "1024", "0777")
 * @num:    결과를 저장할 포인터
 * @base:   진법 (0이면 접두사로 자동 감지: 0x=16진, 0=8진, 그 외 10진)
 * @return: 성공 시 0, 파싱 실패(잘못된 문자 포함) 시 EINVAL
 *
 * strtoul()로 변환 후 endptr이 문자열 끝('\0')을 가리키는지 확인하여
 * 문자열 전체가 유효한 숫자인지 검증한다.
 */
int parse_u64(const char* str, uint64_t* num, int base)
{
    char* endptr = NULL;        // strtoul이 파싱을 멈춘 위치를 가리킬 포인터
    uint64_t ul = strtoul(str, &endptr, base);  // 문자열 → unsigned long 변환

    /* endptr이 NULL이거나 문자열 끝('\0')이 아니면 유효하지 않은 문자가 포함된 것 */
    if (endptr == NULL || *endptr != '\0')
    {
        return EINVAL;  // 잘못된 인자
    }

    *num = ul;      // 변환된 값을 출력 파라미터에 저장
    return 0;
}


/*
 * parse_u32 - 문자열을 uint32_t로 파싱
 *
 * @str:    변환할 문자열
 * @num:    결과를 저장할 포인터
 * @base:   진법
 * @return: 성공 시 0, 파싱 실패 또는 UINT_MAX(2^32-1) 초과 시 EINVAL
 *
 * parse_u64()로 먼저 uint64_t로 변환한 뒤, UINT_MAX를 초과하면 EINVAL 반환.
 * 이렇게 계층적으로 구현하여 코드 중복을 줄인다.
 */
int parse_u32(const char* str, uint32_t* num, int base)
{
    int status;
    uint64_t ul;        // 중간 변환 결과 (64비트로 먼저 변환)

    status = parse_u64(str, &ul, base);     // 문자열 → uint64_t 변환

    if (status != 0 || ul > UINT_MAX)       // 파싱 실패 또는 32비트 범위 초과
    {
        return EINVAL;
    }

    *num = (uint32_t) ul;   // 안전하게 32비트로 캐스팅
    return status;
}


/*
 * parse_u16 - 문자열을 uint16_t로 파싱
 *
 * @str:    변환할 문자열
 * @num:    결과를 저장할 포인터
 * @base:   진법
 * @return: 성공 시 0, 파싱 실패 또는 0xFFFF(65535) 초과 시 EINVAL
 *
 * parse_u64()로 먼저 uint64_t로 변환한 뒤, 0xFFFF를 초과하면 EINVAL 반환.
 */
int parse_u16(const char* str, uint16_t* num, int base)
{
    int status;
    uint64_t ul;        // 중간 변환 결과

    status = parse_u64(str, &ul, base);

    if (status != 0 || ul > 0xffff)     // 16비트 범위(0~65535) 초과 검사
    {
        return EINVAL;
    }

    *num = (uint16_t) ul;   // 안전하게 16비트로 캐스팅
    return status;
}



/*
 * print_ctrl_info - NVMe Identify Controller 결과를 포맷하여 출력
 *
 * @fp:   출력 파일 포인터 (보통 stdout)
 * @info: nvm_admin_ctrl_info()로 획득한 컨트롤러 정보 구조체
 *
 * 출력 항목:
 * - PCI Vendor ID / Subsystem Vendor ID (4바이트를 2+2로 분리)
 * - NVMe 스펙 버전 (major.minor.patch)
 * - 컨트롤러 페이지 크기 (보통 4096)
 * - 최대 큐 엔트리 수 (max_entries)
 * - 시리얼 번호 (20바이트, null 종료 처리)
 * - 모델 번호 (40바이트, null 종료 처리)
 * - 펌웨어 리비전 (8바이트, null 종료 처리)
 * - 최대 데이터 전송 크기 (MDTS, 바이트 및 KiB)
 * - 최대 동시 명령 수 (max_out_cmds)
 * - 최대 네임스페이스 수 (max_n_ns)
 */
void print_ctrl_info(FILE* fp, const struct nvm_ctrl_info* info)
{
    // PCI Vendor ID + Subsystem Vendor ID를 바이트 배열로 추출
    unsigned char vendor[4];
    memcpy(vendor, &info->pci_vendor, sizeof(vendor));

    /* 시리얼 번호: NVMe 스펙에서 20바이트 고정 길이 (null 종료 없음) */
    char serial[21];
    memset(serial, 0, 21);              // 21번째 바이트를 0으로 설정하여 null 종료 보장
    memcpy(serial, info->serial_no, 20);    // NVMe Identify 데이터에서 20바이트 복사

    /* 모델 번호: 40바이트 고정 */
    char model[41];
    memset(model, 0, 41);
    memcpy(model, info->model_no, 40);

    /* 펌웨어 리비전: 8바이트 고정 */
    char revision[9];
    memset(revision, 0, 9);
    memcpy(revision, info->firmware, 8);

    fprintf(fp, "------------- Controller information -------------\n");
    fprintf(fp, "PCI Vendor ID           : %x %x\n", vendor[0], vendor[1]);             // 예: 144d = Samsung
    fprintf(fp, "PCI Subsystem Vendor ID : %x %x\n", vendor[2], vendor[3]);
    fprintf(fp, "NVM Express version     : %u.%u.%u\n",                                 // NVMe 스펙 버전 파싱
            info->nvme_version >> 16,               // major (상위 16비트)
            (info->nvme_version >> 8) & 0xff,       // minor (중간 8비트)
            info->nvme_version & 0xff);             // tertiary (하위 8비트)
    fprintf(fp, "Controller page size    : %zu\n", info->page_size);                    // CC.MPS 기반 (보통 4096)
    fprintf(fp, "Max queue entries       : %u\n", info->max_entries);                   // CAP.MQES + 1
    fprintf(fp, "Serial Number           : %s\n", serial);
    fprintf(fp, "Model Number            : %s\n", model);
    fprintf(fp, "Firmware revision       : %s\n", revision);
    fprintf(fp, "Max data transfer size  : %zu bytes (%zu KiB)\n", info->max_data_size, info->max_data_size >> 10);  // MDTS
    fprintf(fp, "Max outstanding commands: %zu\n", info->max_out_cmds);                 // 동시 처리 가능 명령 수
    fprintf(fp, "Max number of namespaces: %zu\n", info->max_n_ns);                     // 지원 네임스페이스 수
    fprintf(fp, "--------------------------------------------------\n");
}


/*
 * print_ns_info - NVMe Identify Namespace 결과를 포맷하여 출력
 *
 * @fp:   출력 파일 포인터 (보통 stdout)
 * @info: nvm_admin_ns_info()로 획득한 네임스페이스 정보 구조체
 *
 * 출력 항목:
 * - 네임스페이스 ID (NSID): 16진수
 * - 논리 블록 크기 (LBA Data Size): 보통 512B 또는 4KB
 * - 네임스페이스 크기 (총 블록 수 및 MiB 환산)
 * - 네임스페이스 용량 (할당 가능 블록 수 및 MiB 환산)
 */
void print_ns_info(FILE* fp, const struct nvm_ns_info* info)
{
    fprintf(fp, "------------- Namespace  information -------------\n");
    fprintf(fp, "Namespace identifier    : %x\n", info->ns_id);                         // NSID (16진수)
    fprintf(fp, "Logical block size      : %zu bytes\n", info->lba_data_size);          // LBA 크기 (바이트)
    fprintf(fp, "Namespace size          : %zu blocks (%zu MiB)\n", info->size, info->size >> 20);       // 총 블록 수 및 MiB
    fprintf(fp, "Namespace capacity      : %zu blocks (%zu MiB)\n", info->capacity, info->capacity >> 20); // 용량 (블록/MiB)
    fprintf(fp, "--------------------------------------------------\n");
}
