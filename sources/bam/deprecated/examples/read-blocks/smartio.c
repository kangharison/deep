/*
 * smartio.c - NVMe 블록 읽기 SmartIO(DIS) 백엔드 (deprecated)
 *
 * Dolphin SmartIO를 통해 원격 NVMe 디바이스의 블록을 읽는다.
 * SISCI API로 디바이스를 빌려오고, SISCI 세그먼트로 DMA 매핑을 수행한다.
 */
#include "args.h"           // struct options: 커맨드라인 옵션 구조체
#include "read.h"           // disk_info, queue_pair, get_disk_info(), create_queue_pair() 등
#include <nvm_types.h>      // nvm_ctrl_t, nvm_dma_t, nvm_aq_ref 등 libnvm 기본 타입
#include <nvm_ctrl.h>       // nvm_dis_ctrl_init(): FDID로 원격 NVMe 컨트롤러 핸들 획득
#include <nvm_dma.h>        // nvm_dis_dma_create(): SISCI 세그먼트 기반 DMA 메모리 생성
#include <nvm_aq.h>         // nvm_aq_create(): Admin Queue 생성 (컨트롤러 리셋 포함)
#include <nvm_admin.h>      // nvm_admin_cq_create/sq_create(): Admin Create I/O Queue 명령
#include <nvm_util.h>       // NVM_SQ_PAGES(), NVM_DMA_OFFSET() 등 유틸리티 매크로
#include <nvm_error.h>      // nvm_ok(), nvm_strerror(): 에러 처리 함수
#include <sisci_api.h>      // SCIInitialize(), SCITerminate(): Dolphin SISCI 라이브러리
#include <stdio.h>          // fprintf(), stderr: 에러 출력
#include <stdlib.h>         // exit(): 프로그램 종료
#include <stdbool.h>        // bool: 불리언 타입
#include <errno.h>          // errno: 시스템 에러 코드
#include <string.h>         // strerror(): 에러 코드를 문자열로 변환


/*
 * main - SmartIO(DIS) 경유 NVMe 블록 읽기/쓰기
 *
 * [실행 흐름]
 * 1. 커맨드라인 파싱
 * 2. SISCI 초기화 -> nvm_dis_ctrl_init(FDID)로 원격 컨트롤러 접근
 * 3. Admin Queue용 SISCI 세그먼트 DMA 할당 -> Admin Queue 생성
 * 4. Identify Controller/Namespace로 디스크 정보 획득
 * 5. 데이터 버퍼/CQ/SQ 메모리를 SISCI 세그먼트로 할당
 * 6. CQ/SQ 쌍 생성 -> 쓰기(옵션) -> 읽기+덤프 실행
 * 7. 리소스 역순 해제 -> SCITerminate
 */
int main(int argc, char** argv)
{
    int status;
    sci_error_t err;            // SISCI API 에러 코드

    struct disk_info info;      // Identify로 획득한 디스크 정보
    struct queue_pair queues;   // I/O SQ/CQ 쌍

    nvm_ctrl_t* ctrl = NULL;    // NVMe 컨트롤러 핸들
    nvm_dma_t* aq_mem = NULL;   // Admin Queue용 DMA 윈도우
    nvm_aq_ref aq_ref = NULL;   // Admin Queue 참조 핸들
    nvm_dma_t* buffer = NULL;   // I/O 데이터 DMA 버퍼
    nvm_dma_t* cq_mem = NULL;   // Completion Queue DMA 메모리
    nvm_dma_t* sq_mem = NULL;   // Submission Queue + PRP 리스트 DMA 메모리

    struct options args;        // 커맨드라인 파싱 결과

    // 커맨드라인 인자 파싱
    parse_options(argc, argv, &args);

    // SISCI 라이브러리 초기화: DIS 클러스터 통신 기반 설정
    SCIInitialize(0, &err);
    if (err != SCI_ERR_OK)
    {
        fprintf(stderr, "Something went wrong: %s\n", SCIGetErrorString(err));
        exit(1);
    }

    // SmartIO FDID로 원격 NVMe 컨트롤러 핸들 획득
    // DIS 패브릭을 통해 원격 노드의 NVMe 컨트롤러 BAR 레지스터에 접근 가능
    status = nvm_dis_ctrl_init(&ctrl, args.controller_id);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to get controller reference: %s\n", nvm_strerror(status));
        goto leave;
    }

    // Admin Queue용 DMA 메모리 생성: 3페이지 (ACQ + ASQ + Identify 버퍼)
    // nvm_dis_dma_create: SISCI 세그먼트를 내부적으로 생성하고 DMA 매핑
    status = nvm_dis_dma_create(&aq_mem, ctrl, ctrl->page_size * 3, 0);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create queue memory: %s\n", nvm_strerror(status));
        goto leave;
    }

    // Admin Queue 생성: 컨트롤러 리셋 → AQA/ASQ/ACQ 설정 → CC.EN=1
    status = nvm_aq_create(&aq_ref, ctrl, aq_mem);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to reset controller: %s\n", nvm_strerror(status));
        goto leave;
    }

    // Identify Controller/Namespace로 디스크 정보 획득
    // aq_mem의 3번째 페이지(인덱스 2)를 Identify 데이터 수신 버퍼로 사용
    status = get_disk_info(aq_ref, &info, args.namespace_id, NVM_DMA_OFFSET(aq_mem, 2), aq_mem->ioaddrs[2], args.identify);
    if (status != 0)
    {
        goto leave;
    }

    // 데이터 버퍼 크기 계산: min(chunk_size, num_blocks) × 블록 크기
    size_t buffer_size = (args.chunk_size <= args.num_blocks ? args.chunk_size : args.num_blocks) * info.block_size;

    // I/O 데이터 DMA 버퍼 생성: NVMe Read/Write 데이터가 이 버퍼를 통해 전송됨
    status = nvm_dis_dma_create(&buffer, ctrl, buffer_size, 0);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create data buffer: %s\n", nvm_strerror(status));
        goto leave;
    }

    // CQ용 DMA 메모리 생성: 1페이지
    // SCI_MEMACCESS_HOST_READ: 호스트(CPU)가 CQ를 폴링하여 읽기
    // SCI_MEMACCESS_DEVICE_WRITE: NVMe 컨트롤러가 CQ에 완료 엔트리를 DMA로 기록
    status = nvm_dis_dma_create(&cq_mem, ctrl, ctrl->page_size, SCI_MEMACCESS_HOST_READ | SCI_MEMACCESS_DEVICE_WRITE);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create completion queue memory: %s\n", nvm_strerror(status));
        goto leave;
    }

    // SQ + PRP 리스트용 DMA 메모리 생성
    // SQ 페이지 수 + PRP 리스트(queue_size개) + 여분 1페이지
    // SCI_MEMACCESS_HOST_WRITE: 호스트(CPU)가 SQ에 명령을 기록
    // SCI_MEMACCESS_DEVICE_READ: NVMe 컨트롤러가 SQ에서 명령을 DMA로 읽기
    size_t n_prp_lists = args.queue_size;

    status = nvm_dis_dma_create(&sq_mem, ctrl,
            NVM_SQ_PAGES(ctrl, args.queue_size) * ctrl->page_size + ctrl->page_size * (n_prp_lists + 1),
            SCI_MEMACCESS_HOST_WRITE | SCI_MEMACCESS_DEVICE_READ);
    if (!nvm_ok(status))
    {
        goto leave;
    }

    // Admin Create CQ/SQ 명령으로 I/O 큐 쌍 생성
    status = create_queue_pair(aq_ref, &queues, cq_mem, sq_mem, args.queue_size);
    if (!nvm_ok(status))
    {
        goto leave;
    }

    // --write 옵션 설정 시: 파일에서 읽어 NVMe Write 수행
    if (args.input != NULL)
    {
        status = write_blocks(&info, &queues, buffer, &args);
        if (status != 0)
        {
            goto leave;
        }
    }

    // NVMe Read 수행 후 데이터를 stdout/파일로 덤프
    status = read_and_dump(&info, &queues, buffer, &args);


leave:
    // 입력 파일 닫기
    if (args.input != NULL)
    {
        fclose(args.input);
    }

    // 출력 파일 닫기
    if (args.output != NULL)
    {
        fprintf(stderr, "Flushing output file...\n");
        fclose(args.output);
    }
    fprintf(stderr, "Done\n");

    // 리소스 역순 해제: I/O 큐 메모리 → AQ → 컨트롤러 순
    nvm_dma_unmap(cq_mem);      // CQ DMA 매핑 해제
    nvm_dma_unmap(sq_mem);      // SQ+PRP DMA 매핑 해제
    nvm_dma_unmap(buffer);      // 데이터 버퍼 DMA 매핑 해제
    nvm_aq_destroy(aq_ref);     // Admin Queue 파괴
    nvm_dma_unmap(aq_mem);      // Admin Queue DMA 매핑 해제
    nvm_ctrl_free(ctrl);        // 컨트롤러 핸들 해제
    SCITerminate();             // SISCI 라이브러리 종료

    if (status != 0)
    {
        fprintf(stderr, "%d\n", status);    // 에러 코드 출력
    }

    exit(status);
}
