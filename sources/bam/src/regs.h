/* regs.h - NVMe 컨트롤러 레지스터 접근 매크로 헤더
 *
 * NVMe 스펙에 정의된 컨트롤러 레지스터(BAR0 오프셋)에 접근하는 매크로를 정의한다.
 * CAP(Capabilities), CC(Controller Configuration), CSTS(Controller Status),
 * AQA(Admin Queue Attributes), ASQ/ACQ(Admin Queue Base Address) 레지스터와
 * 도어벨(Doorbell) 레지스터 오프셋 계산 매크로를 포함한다.
 */
#ifndef __NVM_INTERNAL_REGS_H__
#define __NVM_INTERNAL_REGS_H__

#include <nvm_util.h>     // _REG, _RB, _WB 비트필드 매크로
#include <stdint.h>       // 정수형 타입


/* NVMe 컨트롤러 레지스터 접근 매크로 (BAR0 오프셋 기준)
 * p는 BAR0 메모리 매핑의 시작 주소(volatile void*)이다.
 * _REG(p, offset, bits) 매크로로 해당 오프셋의 레지스터 포인터를 얻는다.
 */
#define CAP(p)          _REG(p, 0x0000, 64)     // 오프셋 0x00: Controller Capabilities (64비트, 읽기 전용)
#define VER(p)          _REG(p, 0x0008, 32)     // 오프셋 0x08: NVM Express Version (32비트, 읽기 전용)
#define CC(p)           _REG(p, 0x0014, 32)     // 오프셋 0x14: Controller Configuration (32비트, 읽기/쓰기)
#define CSTS(p)         _REG(p, 0x001c, 32)     // 오프셋 0x1C: Controller Status (32비트, 읽기 전용)
#define AQA(p)          _REG(p, 0x0024, 32)     // 오프셋 0x24: Admin Queue Attributes (32비트, 읽기/쓰기)
#define ASQ(p)          _REG(p, 0x0028, 64)     // 오프셋 0x28: Admin Submission Queue Base Address (64비트, 읽기/쓰기)
#define ACQ(p)          _REG(p, 0x0030, 64)     // 오프셋 0x30: Admin Completion Queue Base Address (64비트, 읽기/쓰기)


/* CAP 레지스터 비트필드 읽기 매크로
 * _RB(value, hi, lo)는 value의 비트 hi:lo를 추출한다.
 */
#define CAP$MPSMAX(p)   _RB(*CAP(p), 55, 52)    // 비트 55:52 - 지원하는 최대 메모리 페이지 크기 (2^(12+MPSMAX) 바이트)
#define CAP$MPSMIN(p)   _RB(*CAP(p), 51, 48)    // 비트 51:48 - 지원하는 최소 메모리 페이지 크기 (2^(12+MPSMIN) 바이트)
#define CAP$DSTRD(p)    _RB(*CAP(p), 35, 32)    // 비트 35:32 - 도어벨 스트라이드 (도어벨 레지스터 간격 = 4<<DSTRD 바이트)
#define CAP$TO(p)       _RB(*CAP(p), 31, 24)    // 비트 31:24 - 타임아웃 (500ms 단위, CC.EN 변경 후 CSTS.RDY 전환 최대 대기 시간)
#define CAP$CQR(p)      _RB(*CAP(p), 16, 16)    // 비트 16 - Contiguous Queues Required (1이면 큐 메모리가 물리적으로 연속이어야 함)
#define CAP$MQES(p)     _RB(*CAP(p), 15,  0)    // 비트 15:0 - Maximum Queue Entries Supported (0-based, 최대 큐 엔트리 수 = MQES+1)

/* CSTS 레지스터 비트필드 읽기 매크로 */
#define CSTS$RDY(p)     _RB(*CSTS(p), 0,  0)    // 비트 0 - Ready (1이면 컨트롤러가 명령 처리 준비 완료)


/* CC(Controller Configuration) 레지스터 비트필드 쓰기 매크로
 * _WB(value, hi, lo)는 value를 비트 hi:lo 위치에 배치한다.
 */
#define CC$IOCQES(v)    _WB(v, 23, 20)          // 비트 23:20 - I/O CQ 엔트리 크기 (2^IOCQES 바이트, 보통 4=16바이트)
#define CC$IOSQES(v)    _WB(v, 19, 16)          // 비트 19:16 - I/O SQ 엔트리 크기 (2^IOSQES 바이트, 보통 6=64바이트)
#define CC$MPS(v)       _WB(v, 10,  7)          // 비트 10:7 - Memory Page Size (호스트 페이지 크기 = 2^(12+MPS) 바이트)
#define CC$CSS(v)       _WB(0,  3,  1)          // 비트 3:1 - I/O Command Set Selected (0=NVM Command Set)
#define CC$EN(v)        _WB(v,  0,  0)          // 비트 0 - Enable (1이면 컨트롤러 활성화, 0이면 비활성화/리셋)

/* AQA(Admin Queue Attributes) 레지스터 비트필드 쓰기 매크로 */
#define AQA$ACQS(v)     _WB(v, 27, 16)          // 비트 27:16 - Admin Completion Queue Size (0-based, 엔트리 수 = ACQS+1)
#define AQA$ASQS(v)     _WB(v, 11,  0)          // 비트 11:0 - Admin Submission Queue Size (0-based, 엔트리 수 = ASQS+1)


/* SQ_DBL - Submission Queue y의 도어벨 레지스터 주소를 계산하는 매크로.
 * NVMe 스펙: SQ y Tail Doorbell 오프셋 = 0x1000 + (2y) * (4 << CAP.DSTRD)
 * 호스트가 SQ에 새 명령을 넣은 후 이 도어벨에 tail 값을 써서 컨트롤러에 알린다.
 */
#define SQ_DBL(p, y, dstrd)    \
        ((volatile uint32_t*) (((volatile unsigned char*) (p)) + 0x1000 + ((2*(y)) * (4 << (dstrd)))) )


/* CQ_DBL - Completion Queue y의 도어벨 레지스터 주소를 계산하는 매크로.
 * NVMe 스펙: CQ y Head Doorbell 오프셋 = 0x1000 + (2y + 1) * (4 << CAP.DSTRD)
 * 호스트가 CQ에서 완료를 처리한 후 이 도어벨에 head 값을 써서 컨트롤러에 알린다.
 */
#define CQ_DBL(p, y, dstrd)    \
        ((volatile uint32_t*) (((volatile unsigned char*) (p)) + 0x1000 + ((2*(y) + 1) * (4 << (dstrd)))) )

#endif /* __NVM_INTERNAL_REGS_H__ */
