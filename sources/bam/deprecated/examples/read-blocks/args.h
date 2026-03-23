/*
 * args.h - NVMe 블록 읽기 예제 커맨드라인 인자 헤더 (deprecated)
 *
 * 커맨드라인 옵션: 컨트롤러 경로, 큐 크기, 청크 크기, 네임스페이스 ID,
 * 읽을 블록 수, 시작 오프셋, 출력/입력 파일, ASCII 모드, Identify 모드
 */
#ifndef __LIBNVM_SAMPLES_READ_BLOCKS_OPTIONS_H__  // 인클루드 가드: 중복 포함 방지
#define __LIBNVM_SAMPLES_READ_BLOCKS_OPTIONS_H__

#include <stdint.h>     // uint32_t, uint64_t: 고정 크기 정수 타입
#include <stdbool.h>    // bool, true, false: C99 불리언 타입
#include <stdio.h>      // FILE*: 파일 포인터 타입 (output, input 필드에 사용)
#include <nvm_types.h>  // libnvm 기본 타입: nvm_ctrl_t, nvm_dma_t 등 (간접 의존)


/*
 * options - 커맨드라인 옵션 구조체
 *
 * NVMe 블록 읽기/쓰기에 필요한 모든 설정을 관리한다.
 * parse_options()에서 커맨드라인 인자를 파싱하여 이 구조체의 각 필드에 값을 저장한다.
 */
struct options
{
#ifdef __DIS_CLUSTER__
    uint64_t    controller_id;   /* SmartIO FDID: DIS 클러스터에서 원격 NVMe를 식별하는 Fabric Device ID (16진수, 예: 0xc0c00) */
#else
    const char* controller_path; /* /dev/libnvmX: 로컬 모드에서 libnvm 커널 모듈이 생성한 캐릭터 디바이스 경로 */
#endif
    size_t      queue_size;      /* SQ 깊이: Submission Queue에 동시에 대기 가능한 NVMe 명령 수 (기본값: 64) */
    size_t      chunk_size;      /* 청크 크기: 한 번의 I/O 반복에서 전송할 블록 수 (MDTS 이내, 0이면 num_blocks와 동일) */
    uint32_t    namespace_id;    /* NVMe 네임스페이스 ID: NVMe 논리 디스크 식별자 (기본값: 1, 대부분의 SSD는 NS 1개) */
    size_t      num_blocks;      /* 읽을 총 블록 수: 전체 I/O 요청의 블록 수 (필수 지정, 0이면 에러) */
    size_t      offset;          /* 시작 LBA 오프셋: 이 논리 블록 주소부터 읽기/쓰기 시작 (기본값: 0) */
    FILE*       output;          /* 출력 파일 포인터: NULL이면 stdout에 hex/ascii 덤프, 지정 시 바이너리 fwrite */
    FILE*       input;           /* 입력 파일 포인터: NULL이 아니면 파일을 읽어 NVMe Write 후 Read 수행 */
    bool        ascii;           /* ASCII 출력 모드: true이면 hex 대신 인쇄 가능 문자를 텍스트로 출력 */
    bool        identify;        /* Identify 출력 모드: true이면 NVMe Identify Controller 정보를 stderr에 출력 */
};


/* parse_options - 커맨드라인 옵션을 파싱하여 options 구조체에 저장. 필수 옵션(--ctrl, --blocks) 미지정 시 exit() 호출 */
void parse_options(int argc, char** argv, struct options* options);


#endif  // __LIBNVM_SAMPLES_READ_BLOCKS_OPTIONS_H__
