/*
 * =============================================================================
 * NVMe Identify 예제 공통 헤더 (deprecated/examples/identify/common.h)
 * =============================================================================
 *
 * [목적]
 * NVMe Identify Controller/Namespace 명령의 공통 함수 선언.
 * module.c, smartio.c, userspace.c에서 공유하는 인터페이스를 정의한다.
 *
 * [Deprecated 배경]
 * libnvm 라이브러리의 초기 예제 코드. BaM이 커널 모듈(libnvm.ko) 기반
 * GPU-initiated I/O로 전환되면서 deprecated되었다.
 *
 * [BaM 아키텍처에서의 위치]
 * deprecated/examples/identify/ 디렉토리의 공통 헤더.
 * NVMe Admin 명령(Identify Controller/Namespace)을 libnvm API로 수행하는 예제.
 *
 * [NVMe 스펙 참조]
 * - Identify Controller (CNS=01h): NVMe 1.4 스펙 Figure 247
 * - Identify Namespace (CNS=00h): NVMe 1.4 스펙 Figure 245
 */
#ifndef __LIBNVM_SAMPLES_IDENTIFY_H__  // 인클루드 가드: 중복 포함 방지
#define __LIBNVM_SAMPLES_IDENTIFY_H__

#include <nvm_types.h>  // libnvm 기본 타입 정의 (nvm_ctrl_t, nvm_dma_t, nvm_aq_ref 등)
#include <stdint.h>     // 고정 크기 정수 타입 (uint32_t, uint64_t 등)


/*
 * reset_ctrl - NVMe 컨트롤러를 리셋하고 Admin Queue를 생성
 *
 * @ctrl:       NVMe 컨트롤러 핸들
 * @dma_window: Admin Queue용 DMA 윈도우 (최소 2페이지: 페이지0=ACQ, 페이지1=ASQ)
 * @return:     성공 시 Admin Queue 참조, 실패 시 NULL
 *
 * NVMe 스펙의 컨트롤러 리셋 절차:
 * 1. CC.EN=0으로 비활성화 -> CSTS.RDY=0 대기
 * 2. AQA, ASQ, ACQ 레지스터 설정
 * 3. CC.EN=1로 활성화 -> CSTS.RDY=1 대기
 */
nvm_aq_ref reset_ctrl(const nvm_ctrl_t* ctrl, const nvm_dma_t* dma_window);  // 컨트롤러 리셋 + Admin Queue 생성 함수


/*
 * identify_ctrl - NVMe Identify Controller 명령 실행 및 정보 출력
 *
 * @admin:  Admin Queue 참조
 * @ptr:    Identify 데이터를 받을 버퍼의 가상 주소 (4KB 이상)
 * @ioaddr: 버퍼의 I/O 주소 (컨트롤러가 DMA로 결과를 기록할 주소)
 * @return: 성공 시 0, 실패 시 NVMe 에러 코드
 *
 * NVMe Admin Command Opcode 0x06 (Identify)를 CNS=1로 실행하여
 * 컨트롤러 정보(벤더, 시리얼, 모델, 펌웨어, MDTS, 큐 수 등)를 조회한다.
 */
int identify_ctrl(nvm_aq_ref admin, void* ptr, uint64_t ioaddr);  // Identify Controller 명령 실행


/*
 * identify_ns - NVMe Identify Namespace 명령 실행 및 정보 출력
 *
 * @admin:         Admin Queue 참조
 * @nvm_namespace: 조회할 NVMe 네임스페이스 ID (1부터 시작)
 * @ptr:           Identify 데이터를 받을 버퍼의 가상 주소
 * @ioaddr:        버퍼의 I/O 주소
 * @return:        성공 시 0, 실패 시 NVMe 에러 코드
 *
 * NVMe Admin Command Opcode 0x06 (Identify)를 CNS=0으로 실행하여
 * 네임스페이스 정보(LBA 크기, 총 블록 수, 용량 등)를 조회한다.
 */
int identify_ns(nvm_aq_ref admin, uint32_t nvm_namespace, void* ptr, uint64_t ioaddr);  // Identify Namespace 명령 실행


#endif  // __LIBNVM_SAMPLES_IDENTIFY_H__ 인클루드 가드 종료
