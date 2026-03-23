/*
 * util.c - NVMe 무결성 검증 예제 유틸리티 (deprecated)
 *
 * buffer/queue 생성/삭제 등 공통 유틸리티 함수를 구현한다.
 * create_buffer(): posix_memalign으로 DMA 버퍼 할당 + nvm_dma_map_host로 DMA 매핑
 * create_queue(): Admin Queue를 통해 I/O CQ/SQ 쌍을 생성
 * remove_buffer/queue(): 리소스 정리
 */
#include <nvm_ctrl.h>     // nvm_ctrl_from_aq_ref: AQ 참조에서 컨트롤러 핸들 추출
#include <nvm_dma.h>      // nvm_dma_map_host, nvm_dis_dma_create, nvm_dma_unmap
#include <nvm_admin.h>    // nvm_admin_cq_create, nvm_admin_sq_create
#include <nvm_error.h>    // nvm_ok, nvm_strerror
#include <stdio.h>        // fprintf, stderr
#include <stdint.h>       // uint16_t
#include <stddef.h>       // size_t
#include <stdlib.h>       // posix_memalign, free
#include <stdbool.h>      // bool
#include <string.h>       // memset, strerror
#include "integrity.h"    // buffer, queue, disk 구조체 정의


/*
 * create_buffer - DMA 버퍼 생성
 *
 * @b:    결과를 저장할 buffer 구조체
 * @ref:  Admin Queue 참조 (컨트롤러 정보 접근용)
 * @size: 할당할 바이트 수
 * @return: 성공 시 0, 실패 시 에러 코드
 *
 * DIS 클러스터 모드: nvm_dis_dma_create()로 SISCI 세그먼트 할당
 * 로컬 모드: posix_memalign()으로 페이지 정렬 메모리 할당 + nvm_dma_map_host()로 DMA 매핑
 * 할당 후 0으로 초기화한다.
 */
int create_buffer(struct buffer* b, nvm_aq_ref ref, size_t size)
{
    int status;  // 함수 반환값

    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);  // AQ 참조에서 컨트롤러 핸들 추출 (페이지 크기 등 정보 접근)

#ifdef __DIS_CLUSTER__
    // DIS 모드: SISCI 세그먼트로 DMA 메모리 할당 (호스트 메모리 포인터는 SISCI가 관리하므로 NULL)
    b->buffer = NULL;
    status = nvm_dis_dma_create(&b->dma, ctrl, size, 0);  // SISCI 세그먼트 생성 + NVMe DMA 매핑
#else
    // 로컬 모드: posix_memalign으로 페이지 정렬 메모리 할당
    status = posix_memalign(&b->buffer, ctrl->page_size, size);  // 컨트롤러 페이지 크기로 정렬
    if (status != 0)
    {
        fprintf(stderr, "Failed to allocate memory: %s\n", strerror(status));
        return status;
    }

    // 커널 모듈을 통해 IOMMU DMA 매핑 수행
    status = nvm_dma_map_host(&b->dma, ctrl, b->buffer, size);  // 가상주소 → IOMMU 주소 변환
#endif
    if (!nvm_ok(status))  // DMA 매핑 실패 확인
    {
        free(b->buffer);  // 할당한 메모리 해제
        fprintf(stderr, "Failed to create local segment: %s\n", nvm_strerror(status));
        return status;
    }

    // DMA 버퍼 전체를 0으로 초기화 (전체 페이지 * 페이지 크기)
    memset(b->dma->vaddr, 0, b->dma->page_size * b->dma->n_ioaddrs);

    return 0;  // 성공
}


/* remove_buffer - DMA 버퍼 해제 (DMA 언맵 + 호스트 메모리 free) */
void remove_buffer(struct buffer* b)
{
    nvm_dma_unmap(b->dma);  // DMA 윈도우 해제 (IOMMU 매핑 해제 또는 SISCI 세그먼트 해제)
    free(b->buffer);        // 호스트 메모리 해제 (DIS 모드에서는 NULL이므로 nop)
}


/*
 * create_queue - NVMe I/O 큐 생성 (CQ 또는 SQ)
 *
 * @q:   결과를 저장할 queue 구조체
 * @ref: Admin Queue 참조 (Admin Create CQ/SQ 명령 제출용)
 * @cq:  NULL이면 CQ 생성, 아니면 이 CQ에 연결된 SQ 생성
 * @qno: 큐 번호 (1부터 시작)
 * @return: 성공 시 0, 실패 시 에러 코드
 *
 * 큐 메모리(DMA 버퍼) 할당 후 Admin 명령으로 CQ 또는 SQ를 컨트롤러에 생성한다.
 * SQ의 경우 PRP 리스트용 추가 메모리도 함께 할당한다.
 */
int create_queue(struct queue* q, nvm_aq_ref ref, const struct queue* cq, uint16_t qno)
{
    int status;  // 함수 반환값

    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);  // 컨트롤러 핸들 획득 (페이지 크기, max_qs 등)

    // SQ 생성 시 PRP 리스트 페이지 수 계산 (CQ 생성 시에는 0)
    size_t prp_lists = 0;
    if (cq != NULL)  // cq가 NULL이 아니면 SQ를 생성하는 것
    {
        // SQ 1페이지에 들어가는 엔트리 수 계산 (64바이트 엔트리 기준: 4096/64 = 64)
        size_t n_entries = ctrl->page_size / sizeof(nvm_cmd_t);
        // 컨트롤러 최대 큐 크기와 비교하여 작은 값 선택
        prp_lists = n_entries <= ctrl->max_qs ? n_entries : ctrl->max_qs;
    }

    // 큐 메모리 DMA 버퍼 할당: PRP 리스트 페이지 + 큐 엔트리 1페이지
    status = create_buffer(&q->qmem, ref, prp_lists * ctrl->page_size + ctrl->page_size);
    if (!nvm_ok(status))
    {
        return status;
    }

    if (cq == NULL)  // CQ 생성: Admin Create I/O Completion Queue 명령 (Opcode 0x05)
    {
        // NVM_CQ_SIZE: 1페이지에 들어가는 CQ 엔트리 수 계산
        status = nvm_admin_cq_create(ref, &q->queue, qno, q->qmem.dma, 0, NVM_CQ_SIZE(ctrl, 1));
    }
    else  // SQ 생성: Admin Create I/O Submission Queue 명령 (Opcode 0x01)
    {
        // NVM_SQ_SIZE: 1페이지에 들어가는 SQ 엔트리 수 계산, CQ와 연결
        status = nvm_admin_sq_create(ref, &q->queue, &cq->queue, qno, q->qmem.dma, 0, NVM_SQ_SIZE(ctrl, 1));
    }

    if (!nvm_ok(status))  // 큐 생성 실패
    {
        remove_buffer(&q->qmem);  // 할당한 큐 메모리 해제
        fprintf(stderr, "Failed to create queue: %s\n", nvm_strerror(status));
        return status;
    }

    q->counter = 0;  // 명령 카운터 초기화
    return 0;  // 성공
}


/* remove_queue - NVMe I/O 큐 해제 (큐 메모리 버퍼 해제) */
void remove_queue(struct queue* q)
{
    remove_buffer(&q->qmem);  // 큐 메모리 DMA 버퍼 해제
}
