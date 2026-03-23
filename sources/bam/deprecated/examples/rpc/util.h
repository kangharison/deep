/*
 * util.h - RPC 예제 공통 유틸리티 헤더 (Deprecated)
 *
 * [파일 역할]
 * RPC 예제 프로그램들(rpc_server, rpc_identify, rpc_dd, rpc_flush)이
 * 공통으로 사용하는 유틸리티 함수 선언.
 * 커맨드라인 인자 파싱(문자열->정수 변환)과 NVMe 정보 출력 기능을 제공한다.
 *
 * [Deprecated 배경]
 * SISCI 기반 RPC NVMe 접근 예제의 일부로, BaM이 커널 모듈 기반
 * GPU-initiated I/O로 전환되면서 함께 deprecated되었다.
 */
#ifndef __DISNVM_EXAMPLES_UTIL_H__  // 인클루드 가드: 중복 포함 방지
#define __DISNVM_EXAMPLES_UTIL_H__

#include <nvm_types.h>      // nvm_ctrl_info, nvm_ns_info: NVMe Identify 결과 구조체
#include <stdint.h>         // uint16_t, uint32_t, uint64_t: 고정 크기 정수 타입
#include <getopt.h>         // getopt 관련 타입 (옵션 파싱에서 간접 사용)
#include <stdio.h>          // FILE*: 출력 대상 파일 포인터 타입


/*
 * parse_u64 - 문자열을 uint64_t로 파싱
 *
 * @str:    변환할 문자열 (예: "0xc0c00", "1024")
 * @number: 결과를 저장할 포인터
 * @base:   진법 (16이면 16진수, 10이면 10진수, 0이면 접두사 자동 감지: 0x→16, 0→8, 그 외→10)
 * @return: 성공 시 0, 파싱 실패(유효하지 않은 문자 포함) 시 EINVAL
 */
int parse_u64(const char* str, uint64_t* number, int base);


/*
 * parse_u32 - 문자열을 uint32_t로 파싱
 *
 * @str:    변환할 문자열
 * @number: 결과를 저장할 포인터
 * @base:   진법
 * @return: 성공 시 0, 파싱 실패 또는 UINT_MAX(4,294,967,295) 초과 시 EINVAL
 *
 * 내부적으로 parse_u64()를 호출한 뒤 32비트 범위를 검증한다.
 */
int parse_u32(const char* str, uint32_t* number, int base);


/*
 * parse_u16 - 문자열을 uint16_t로 파싱
 *
 * @str:    변환할 문자열
 * @number: 결과를 저장할 포인터
 * @base:   진법
 * @return: 성공 시 0, 파싱 실패 또는 0xFFFF(65,535) 초과 시 EINVAL
 *
 * 내부적으로 parse_u64()를 호출한 뒤 16비트 범위를 검증한다.
 */
int parse_u16(const char* str, uint16_t* number, int base);


/*
 * print_ctrl_info - NVMe Identify Controller 결과를 사람이 읽기 쉬운 형태로 출력
 *
 * @fp:   출력 파일 포인터 (보통 stdout)
 * @info: nvm_admin_ctrl_info()로 획득한 컨트롤러 정보 구조체
 *
 * PCI Vendor ID, NVMe 버전, 페이지 크기, 최대 큐 엔트리 수,
 * 시리얼 번호, 모델명, 펌웨어 버전, MDTS, 동시 명령 수, 네임스페이스 수를 출력한다.
 */
void print_ctrl_info(FILE* fp, const struct nvm_ctrl_info* info);


/*
 * print_ns_info - NVMe Identify Namespace 결과를 사람이 읽기 쉬운 형태로 출력
 *
 * @fp:   출력 파일 포인터 (보통 stdout)
 * @info: nvm_admin_ns_info()로 획득한 네임스페이스 정보 구조체
 *
 * 네임스페이스 ID, 논리 블록(LBA) 크기, 네임스페이스 크기(총 블록 수), 용량을 출력한다.
 */
void print_ns_info(FILE* fp, const struct nvm_ns_info* info);

#endif // __DISNVM_EXAMPLES_UTIL_H__
