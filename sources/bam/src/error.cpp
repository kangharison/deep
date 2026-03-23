/* error.cpp - NVMe 에러 코드 해석 및 문자열 변환 구현 파일
 *
 * NVMe 스펙에 정의된 Status Code Type(SCT)과 Status Code(SC)를 사람이 읽을 수 있는
 * 문자열로 변환한다. Generic Command Status, Command Specific Status,
 * Media and Data Integrity 에러 등 NVMe 완료 큐의 상태 필드를 해석한다.
 */
#include <nvm_error.h>    // 에러 관련 공개 API (NVM_ERR_PACK, NVM_ERR_UNPACK 등)
#include <nvm_util.h>     // 유틸리티 매크로
#include <stddef.h>       // 표준 정의
#include <stdint.h>       // 정수형 타입
#include <errno.h>        // POSIX 에러 코드
#include <string.h>       // strerror
#include <stdio.h>        // printf


/* NVMe Generic Command Status 문자열 테이블 (SCT=0x00, SC=0x00~0x1F)
 * NVMe 스펙 Figure 126: Generic Command Status Values에 대응한다.
 */
static const char* generic_status[] =
{
    "Success",                                              // 0x00: 성공
    "Invalid command opcode",                               // 0x01: 잘못된 명령 opcode
    "Invalid field in command",                             // 0x02: 명령 내 잘못된 필드
    "Command ID conflict",                                  // 0x03: 명령 ID 충돌
    "Data transfer error",                                  // 0x04: 데이터 전송 에러
    "Commands aborted due to power loss notification",      // 0x05: 전원 손실 알림으로 명령 중단
    "Internal error",                                       // 0x06: 내부 에러
    "Command abort requested",                              // 0x07: 명령 중단 요청됨
    "Command aborted due to SQ deletion",                   // 0x08: SQ 삭제로 명령 중단
    "Command aborted due to failed fused command",          // 0x09: 실패한 fused 명령으로 중단
    "Command aborted due to missing fused command",         // 0x0A: 누락된 fused 명령으로 중단
    "Invalid namespace or format",                          // 0x0B: 잘못된 네임스페이스 또는 포맷
    "Command sequence error",                               // 0x0C: 명령 시퀀스 에러
    "Invalid SGL segment descriptor",                       // 0x0D: 잘못된 SGL 세그먼트 디스크립터
    "Invalid number of SQL descriptors",                    // 0x0E: 잘못된 SGL 디스크립터 수
    "Data SGL length invalid",                              // 0x0F: 데이터 SGL 길이 무효
    "Metadata SGL length invalid",                          // 0x10: 메타데이터 SGL 길이 무효
    "SGL descriptor type invalid",                          // 0x11: SGL 디스크립터 타입 무효
    "Invalid use of controller memory buffer",              // 0x12: CMB 잘못된 사용
    "PRP offset invalid",                                   // 0x13: PRP 오프셋 무효
    "Atomic write unit exceeded",                           // 0x14: 원자적 쓰기 단위 초과
    "Operation denied",                                     // 0x15: 동작 거부됨
    "SGL offset invalid",                                   // 0x16: SGL 오프셋 무효
    "Unknown/reserved",                                     // 0x17: 예약됨
    "Host identifier inconsistent format",                  // 0x18: 호스트 식별자 형식 불일치
    "Keep alive timer expired",                             // 0x19: Keep Alive 타이머 만료
    "Keep alive timer invalid",                             // 0x1A: Keep Alive 타이머 무효
    "Command aborted due to preempt and abort",             // 0x1B: 선점 및 중단으로 명령 중단
    "Sanitize failed",                                      // 0x1C: 삭제(Sanitize) 실패
    "Sanitize in progress",                                 // 0x1D: 삭제(Sanitize) 진행 중
    "SGL data block granularity invalid",                   // 0x1E: SGL 데이터 블록 단위 무효
    "Command not supported for queue in CMB"                // 0x1F: CMB 내 큐에서 지원하지 않는 명령
};



/* NVM 명령 전용 Generic Status 문자열 테이블 (SCT=0x00, SC=0x80~0x84)
 * I/O 명령에서 발생하는 추가 에러 코드들이다.
 */
static const char* generic_status_nvm_commands[] =
{
    "LBA out of range",          // 0x80: LBA가 범위를 벗어남
    "Capacity exceeded",         // 0x81: 용량 초과
    "Namespace not ready",       // 0x82: 네임스페이스 준비 안 됨
    "Reservation conflict",      // 0x83: 예약 충돌
    "Format in progress"         // 0x84: 포맷 진행 중
};



/* Command Specific Status 문자열 테이블 (SCT=0x01, SC=0x00~0x22)
 * Admin/I/O 명령별 고유 에러 코드들이다.
 */
static const char* command_specific_status[] =
{
    "Completion queue invalid",                             // 0x00: 완료 큐 무효
    "Invalid queue identifier",                             // 0x01: 잘못된 큐 식별자
    "Invalid queue size",                                   // 0x02: 잘못된 큐 크기
    "Abort command limit exceeded",                         // 0x03: 명령 중단 한도 초과
    "Unknown/reserved",                                     // 0x04: 예약됨
    "Asynchronous event request limit exceeded",            // 0x05: 비동기 이벤트 요청 한도 초과
    "Invalid firmware slot",                                // 0x06: 잘못된 펌웨어 슬롯
    "Invalid firmware image",                               // 0x07: 잘못된 펌웨어 이미지
    "Invalid interrupt vector",                             // 0x08: 잘못된 인터럽트 벡터
    "Invalid log page",                                     // 0x09: 잘못된 로그 페이지
    "Invalid format",                                       // 0x0A: 잘못된 포맷
    "Firmware activation requires conventional reset",      // 0x0B: 펌웨어 활성화에 일반 리셋 필요
    "Invalid queue deletion",                               // 0x0C: 잘못된 큐 삭제
    "Feature identifier not saveable",                      // 0x0D: Feature ID 저장 불가
    "Feature not changeable",                               // 0x0E: Feature 변경 불가
    "Feature not namespace specific",                       // 0x0F: Feature가 네임스페이스 특정이 아님
    "Firmware activation requires NVM subsystem reset",     // 0x10: 펌웨어 활성화에 NVM 서브시스템 리셋 필요
    "Firmware activation requires reset",                   // 0x11: 펌웨어 활성화에 리셋 필요
    "Firmware activation requires maximum time violation",  // 0x12: 펌웨어 활성화 최대 시간 위반
    "Firmware activation prohibited",                       // 0x13: 펌웨어 활성화 금지됨
    "Overlapping range",                                    // 0x14: 범위 겹침
    "Namespace insufficient capacity",                      // 0x15: 네임스페이스 용량 부족
    "Namespace identifier unavailable",                     // 0x16: 네임스페이스 ID 사용 불가
    "Unknown/reserved",                                     // 0x17: 예약됨
    "Namespace already attached",                           // 0x18: 네임스페이스 이미 연결됨
    "Namespace is private",                                 // 0x19: 네임스페이스가 private
    "Namespace not attached",                               // 0x1A: 네임스페이스 연결 안 됨
    "Thin provisioning not supported",                      // 0x1B: 씬 프로비저닝 미지원
    "Controller list invalid",                              // 0x1C: 컨트롤러 리스트 무효
    "Device self-test in progress",                         // 0x1D: 디바이스 자가 테스트 진행 중
    "Boot partition write prohibited",                      // 0x1E: 부트 파티션 쓰기 금지
    "Invalid controller identifier",                        // 0x1F: 잘못된 컨트롤러 식별자
    "Invalid secondary controller state",                   // 0x20: 잘못된 보조 컨트롤러 상태
    "Invalid number of controller resources",               // 0x21: 잘못된 컨트롤러 리소스 수
    "Invalid resource identifier"                           // 0x22: 잘못된 리소스 식별자
};



/* NVM 명령 전용 Command Specific Status 문자열 테이블 (SCT=0x01, SC=0x80~0x82) */
static const char* command_specific_status_nvm_commands[] =
{
    "Conflicting attributes",                    // 0x80: 속성 충돌
    "Invalid protection information",            // 0x81: 잘못된 보호 정보
    "Attempted write to read only range"         // 0x82: 읽기 전용 영역에 쓰기 시도
};



/* Media and Data Integrity 에러 문자열 테이블 (SCT=0x02, SC=0x80~0x87)
 * 저장 매체 관련 에러 코드들이다.
 */
static const char* media_and_data_integrity_nvm_commands[] =
{
    "Write fault",                                // 0x80: 쓰기 결함
    "Unrecovered read error",                     // 0x81: 복구 불가 읽기 에러
    "End-to-end guard check error",               // 0x82: 종단간 가드 검사 에러
    "End-to-end application tag check error",     // 0x83: 종단간 애플리케이션 태그 검사 에러
    "End-to-end reference tag check error",       // 0x84: 종단간 참조 태그 검사 에러
    "Compare failure",                            // 0x85: 비교 실패
    "Access denied",                              // 0x86: 접근 거부
    "Deallocated or unwritten logical block"      // 0x87: 할당 해제되었거나 기록되지 않은 논리 블록
};



/* lookup_string - Status Code Type과 Status Code를 문자열로 변환하는 내부 함수.
 * NVMe CQ 엔트리의 SF(Status Field)에서 추출한 SCT/SC 값으로 에러 메시지를 찾는다.
 */
static const char* lookup_string(uint8_t status_code_type, uint8_t status_code)
{
    switch (status_code_type)
    {
        case 0x00: // Generic command status (일반 명령 상태)
            if (status_code < 0x20)
            {
                // 일반 에러 코드 범위 (0x00 ~ 0x1F)
                return generic_status[status_code];
            }
            else if (0x80 <= status_code && status_code <= 0x84)
            {
                // NVM 명령 전용 일반 에러 코드 범위 (0x80 ~ 0x84)
                return generic_status_nvm_commands[status_code - 0x80];
            }
            return "Unknown generic error";

        case 0x01: // Command specific status (명령별 고유 상태)
            if (status_code < 0x23)
            {
                // 명령별 에러 코드 범위 (0x00 ~ 0x22)
                return command_specific_status[status_code];
            }
            else if (0x80 <= status_code && status_code <= 0x82)
            {
                // NVM 명령 전용 명령별 에러 코드 범위 (0x80 ~ 0x82)
                return command_specific_status_nvm_commands[status_code - 0x80];
            }
            return "Unknown command specific error";

        case 0x02: // Media and data integrity errors (매체 및 데이터 무결성 에러)
            if (0x80 <= status_code && status_code <= 0x87)
            {
                return media_and_data_integrity_nvm_commands[status_code - 0x80];
            }
            return "Unknown media or data integrity error";

        default:
            // 알 수 없는 SCT 값
            return "Unknown status code type";
    }
}



/* nvm_strerror - NVM 에러 상태 코드를 사람이 읽을 수 있는 문자열로 변환한다.
 * NVM_ERR_PACK으로 패킹된 상태 코드에서 POSIX errno, NVMe SCT, SC를 추출하고,
 * NVMe 에러가 있으면 NVMe 에러 문자열을, 없으면 POSIX strerror를 반환한다.
 */
const char* nvm_strerror(int status)
{
    int err;        // POSIX errno 부분
    uint8_t sct;    // NVMe Status Code Type
    uint8_t sc;     // NVMe Status Code

    // 패킹된 에러 코드에서 각 부분을 추출
    err = NVM_ERR_UNPACK_ERRNO(status);
    sct = NVM_ERR_UNPACK_SCT(status);
    sc = NVM_ERR_UNPACK_SC(status);

    // NVMe 에러가 있으면 (SCT 또는 SC가 0이 아니면)
    if (sct != 0 || sc != 0)
    {
        // 디버그용으로 SCT/SC 값 출력
        printf("sct: %x\tsc: %x\n", sct, sc);
        // NVMe 에러 문자열 테이블에서 조회하여 반환
        return lookup_string(sct, sc);

    }

    // NVMe 에러가 없으면 POSIX errno에 해당하는 문자열 반환
    return strerror(err);
}
