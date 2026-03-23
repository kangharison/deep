/*
 * rpc_flush.c - RPC를 통한 원격 NVMe Flush 명령 예제 (Deprecated)
 *
 * [파일 역할]
 * DIS(Dolphin Interconnect Solutions) 클러스터 환경에서 원격 NVMe 컨트롤러에
 * RPC로 연결한 뒤, NVMe I/O Flush 명령을 SQ/CQ를 통해 직접 제출하는 예제.
 * Flush 명령은 컨트롤러의 volatile write cache에 있는 데이터를 NVM 미디어로
 * 강제로 기록하라는 명령이다 (데이터 영속성 보장).
 *
 * [Deprecated 배경]
 * SISCI 기반 원격 NVMe 접근 방식에서 커널 모듈(libnvm.ko) 기반 GPU-initiated I/O
 * 아키텍처로 전환되면서 이 RPC 예제는 더 이상 사용되지 않는다.
 *
 * [RPC를 통한 NVMe Admin 명령 구조]
 * 1. nvm_dis_ctrl_init()으로 원격 NVMe 컨트롤러 핸들 획득
 * 2. nvm_dis_rpc_bind()로 RPC 서버(rpc_server)의 Admin Queue에 바인딩
 * 3. RPC를 통해 Admin 명령(Create CQ/SQ)을 원격 전달하여 I/O 큐 생성
 * 4. I/O 큐에 직접 명령을 제출 (Flush 명령은 데이터 전송 없이 opcode만 전달)
 *
 * [SISCI 세그먼트 공유 방식]
 * segment_create()로 SISCI 로컬 메모리 세그먼트를 생성하고,
 * dma_create()로 이를 NVMe 컨트롤러가 접근 가능한 DMA 윈도우로 매핑한다.
 * 이 세그먼트는 DIS 패브릭을 통해 원격 노드의 컨트롤러가 DMA로 접근할 수 있다.
 */
#include <nvm_types.h>      // nvm_ctrl_t, nvm_cmd_t, nvm_cpl_t, nvm_queue_t 등 기본 타입
#include <nvm_ctrl.h>       // nvm_dis_ctrl_init(): FDID로 원격 컨트롤러 핸들 획득, nvm_ctrl_free(): 해제
#include <nvm_admin.h>      // nvm_admin_cq_create/sq_create/sq_delete/cq_delete(): Admin 큐 관리 명령
#include <nvm_dma.h>        // nvm_dma_unmap(): DMA 매핑 해제
#include <nvm_rpc.h>        // nvm_dis_rpc_bind(): RPC 서버의 Admin Queue에 원격 바인딩, nvm_rpc_unbind(): 해제
#include <nvm_aq.h>         // nvm_aq_ref: Admin Queue 참조 타입
#include <nvm_util.h>       // NVM_DEFAULT_CID(): SQ tail 기반 자동 명령 ID 생성 매크로
#include <nvm_error.h>      // nvm_strerror(), NVM_ERR_STATUS(), NVM_ERR_OK(): 에러 처리
#include <nvm_queue.h>      // nvm_sq_enqueue/submit(), nvm_cq_dequeue(), nvm_sq_update/cq_update(): 큐 조작
#include <nvm_cmd.h>        // nvm_cmd_header(): NVMe 명령 헤더 구성
#include <stdio.h>          // fprintf(), stderr: 에러 출력
#include <stdint.h>         // uint32_t: NVMe 네임스페이스 ID 등
#include <stdlib.h>         // exit(): 프로그램 종료
#include <stdbool.h>        // bool: 불리언 타입
#include <getopt.h>         // getopt 관련 (미사용이지만 관례적 포함)
#include <string.h>         // memset(): NVMe 명령 구조체 초기화
#include <errno.h>          // ENOMEM: 메모리 부족 에러 코드
#include <sisci_api.h>      // SCIInitialize(), SCITerminate(): Dolphin SISCI 라이브러리 초기화/종료
#include "segment.h"        // segment_create(), segment_remove(), dma_create(), dma_remove(): SISCI 세그먼트 유틸
#include "util.h"           // 공통 유틸리티 (미사용이지만 rpc 예제 공통 헤더)
#include <unistd.h>         // usleep(): 마이크로초 대기 (주석 처리된 폴링 간격용)


/*
 * flush - NVMe I/O Flush 명령을 SQ에 제출하고 CQ에서 완료를 대기
 *
 * @cq: Completion Queue 포인터
 * @sq: Submission Queue 포인터
 * @ns: NVMe 네임스페이스 ID
 * @return: NVMe 완료 상태 코드 (0이면 성공)
 *
 * [동작 흐름]
 * 1. NVMe 명령 구조체를 0으로 초기화
 * 2. nvm_cmd_header()로 Flush opcode(NVM_IO_FLUSH)와 네임스페이스 ID 설정
 * 3. nvm_sq_enqueue()로 SQ에 빈 슬롯 확보 (NULL이면 큐 가득 참 -> ENOMEM)
 * 4. 명령을 SQ 슬롯에 복사 후 nvm_sq_submit()으로 doorbell 링
 * 5. nvm_cq_dequeue()로 완료 엔트리 busy-wait 폴링
 * 6. nvm_sq_update()/nvm_cq_update()로 큐 포인터 갱신
 * 7. 완료 상태를 stderr에 출력하고 반환
 */
static int flush(nvm_queue_t* cq, nvm_queue_t* sq, uint32_t ns)
{
    nvm_cmd_t cmd;              // NVMe 명령 구조체 (64바이트, SQE 형식)
    memset(&cmd, 0, sizeof(cmd));   // 전체를 0으로 초기화 (예약 필드 클리어)
    nvm_cpl_t* cpl;             // CQ 완료 엔트리 포인터

    /* Flush 명령 헤더 구성: CID는 NVM_DEFAULT_CID 매크로로 SQ tail 기반 자동 생성 */
    // NVM_IO_FLUSH(0x00): volatile write cache → NVM 미디어 영속화
    // ns: 대상 네임스페이스 (0xFFFFFFFF이면 모든 네임스페이스)
    nvm_cmd_header(&cmd, NVM_DEFAULT_CID(sq), NVM_IO_FLUSH, ns);

    /* SQ에 빈 슬롯 확보: tail 포인터가 가리키는 위치에 새 명령을 넣을 공간 */
    nvm_cmd_t* ptr = nvm_sq_enqueue(sq);
    if (ptr == NULL)    // SQ가 가득 찬 경우 (tail+1 == head)
    {
        return ENOMEM;  // 메모리(큐 슬롯) 부족
    }

    /* 로컬에서 구성한 명령을 SQ 슬롯(DMA 메모리)에 복사 */
    *ptr = cmd;

    /* SQ doorbell 레지스터에 새 tail 값을 기록하여 컨트롤러에 새 명령 도착을 알림 */
    nvm_sq_submit(sq);

    /* CQ에서 완료 엔트리가 나타날 때까지 busy-wait 폴링 */
    // nvm_cq_dequeue: CQ head 위치의 phase bit가 기대값과 일치하면 완료 엔트리 반환
    while ((cpl = nvm_cq_dequeue(cq)) == NULL)
    {
        //usleep(1);    // 폴링 간격을 두면 CPU 사용량 감소 (현재 tight loop)
    }

    /* CQ 완료 엔트리에 기록된 SQ head 포인터로 SQ head 갱신 (소비된 슬롯 재활용) */
    nvm_sq_update(sq);
    /* CQ head 포인터를 다음으로 이동하고 CQ doorbell에 기록 */
    nvm_cq_update(cq);

    // NVMe 완료 상태 코드 추출 및 문자열로 출력
    int status = NVM_ERR_STATUS(cpl);   // CQE의 Status Field에서 에러 코드 추출
    fprintf(stderr, "%s\n", nvm_strerror(status));  // 상태를 사람이 읽을 수 있는 문자열로 출력

    return status;
}


/*
 * main - RPC Flush 예제 메인 함수
 *
 * [실행 흐름]
 * 1. SISCI 라이브러리 초기화
 * 2. nvm_dis_ctrl_init()으로 NVMe 컨트롤러 핸들 획득 (FDID 0xc0c00 하드코딩)
 * 3. nvm_dis_rpc_bind()로 RPC 서버의 Admin Queue에 바인딩
 * 4. segment_create()로 SISCI 로컬 세그먼트 생성 (세그먼트 ID=123, 크기=0x2000)
 * 5. dma_create()로 세그먼트를 NVMe DMA 윈도우로 매핑
 * 6. RPC를 통해 Admin Create CQ/SQ 명령으로 I/O 큐 쌍 생성 (큐 ID=1, 깊이=2)
 * 7. flush()를 10회 반복 실행하여 Flush 명령 테스트
 * 8. 역순으로 리소스 정리: SQ 삭제 -> CQ 삭제 -> DMA 해제 -> 세그먼트 제거 -> RPC 해제 -> 컨트롤러 해제
 * 9. SISCI 종료
 *
 * 주의: 컨트롤러 FDID, 세그먼트 ID 등이 하드코딩되어 있어 테스트/데모 용도.
 */
int main(int argc, char** argv)
{
    nvm_ctrl_t* ctrl;           // NVMe 컨트롤러 핸들
    nvm_aq_ref rpc;             // RPC를 통한 원격 Admin Queue 참조
    sci_error_t scierr;         // SISCI API 에러 코드
    struct segment segment;     // SISCI 로컬 세그먼트 (SQ/CQ 메모리용)
    nvm_dma_t* dma;             // 세그먼트의 NVMe DMA 윈도우 핸들
    nvm_queue_t cq;             // Completion Queue 구조체
    nvm_queue_t sq;             // Submission Queue 구조체

    /* SISCI 라이브러리 초기화 (DIS 클러스터 노드 간 통신 기반 설정) */
    SCIInitialize(0, &scierr);
    if (scierr != SCI_ERR_OK)
    {
        exit(1);    // SISCI 초기화 실패 시 즉시 종료
    }

    /* SmartIO FDID 0xc0c00으로 원격 NVMe 컨트롤러 핸들 획득 */
    // 0xc0c00: 테스트 환경에서 사용하는 하드코딩된 FDID
    int err = nvm_dis_ctrl_init(&ctrl, 0xc0c00);

    /* RPC 서버(rpc_server 프로세스)의 Admin Queue에 원격 바인딩 */
    // 어댑터 0번: 첫 번째 DIS 네트워크 어댑터를 통해 RPC 통신
    // 이후 rpc를 통해 Admin 명령(Create Queue 등)을 원격 서버에 전달 가능
    err = nvm_dis_rpc_bind(&rpc, ctrl, 0);

    /* SISCI 로컬 세그먼트 생성: ID 힌트=123, 크기=0x2000(8KB) */
    // 8KB = 2페이지: CQ 1페이지 + SQ 1페이지로 사용
    err = segment_create(&segment, 123, 0x2000);

    /* 세그먼트를 NVMe 컨트롤러가 DMA 접근 가능한 윈도우로 매핑 */
    // 어댑터 0번: 세그먼트를 이 어댑터에서 공개하여 원격 DMA 허용
    err = dma_create(&dma, ctrl, &segment, 0);

    /* Admin Create I/O CQ 명령: RPC를 통해 원격 Admin Queue에 전달 */
    // CQ ID=1, DMA 오프셋 0(첫 번째 페이지), 깊이=2 엔트리
    err = nvm_admin_cq_create(rpc, &cq, 1, dma, 0, 2);

    /* Admin Create I/O SQ 명령: SQ ID=1, 위에서 생성한 CQ에 연결 */
    // DMA 오프셋 1(두 번째 페이지), 깊이=2 엔트리
    err = nvm_admin_sq_create(rpc, &sq, &cq, 1, dma, 1, 2);

    fprintf(stderr, "OK\n");   // 큐 생성 성공 메시지

    /* Flush 명령을 10회 반복 제출하여 동작 확인 (네임스페이스 ID=1) */
    for (int i = 0; i < 10; ++i)
    {
        flush(&cq, &sq, 1);    // 각 Flush의 NVMe 상태가 stderr에 출력됨
    }

    /* 리소스 역순 정리 */
    nvm_admin_sq_delete(rpc, &sq, &cq);   /* Admin Delete I/O SQ: SQ를 먼저 삭제 (CQ 참조 해제) */
    nvm_admin_cq_delete(rpc, &cq);         /* Admin Delete I/O CQ: SQ가 없으므로 안전하게 삭제 */
    dma_remove(dma, &segment, 0);          /* NVMe DMA 윈도우 해제 + 세그먼트 비공개 전환 */
    segment_remove(&segment);              /* SISCI 세그먼트 물리 메모리 반환 + 디스크립터 닫기 */
    nvm_rpc_unbind(rpc);                   /* RPC 바인딩 해제 (원격 Admin Queue 연결 종료) */
    nvm_ctrl_free(ctrl);                   /* NVMe 컨트롤러 핸들 해제 (BAR 언맵 포함) */


    SCITerminate();     // SISCI 라이브러리 종료
    return 0;
}
