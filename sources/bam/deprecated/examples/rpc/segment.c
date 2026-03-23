/*
 * segment.c - SISCI 세그먼트 및 DMA 윈도우 관리 구현 (Deprecated)
 *
 * [파일 역할]
 * Dolphin SISCI API를 사용하여 로컬 메모리 세그먼트를 생성/삭제하고,
 * NVMe 컨트롤러가 DMA로 접근할 수 있는 윈도우를 매핑/해제하는 유틸리티.
 * RPC 예제의 모든 프로그램이 NVMe SQ/CQ 메모리, Identify 버퍼,
 * I/O 데이터 버퍼 등을 할당할 때 이 함수들을 사용한다.
 *
 * [Deprecated 배경]
 * SISCI 기반 DIS 클러스터 원격 NVMe 접근에서 커널 모듈 기반
 * 로컬 GPU-NVMe 직접 접근으로 전환되면서 deprecated되었다.
 *
 * [SISCI 세그먼트 -> NVMe DMA 매핑 흐름]
 * 1. SCIOpen()으로 SISCI 가상 디바이스 디스크립터 획득
 * 2. SCICreateSegment()로 로컬 물리 메모리 세그먼트 생성
 * 3. SCIPrepareSegment()로 특정 DIS 어댑터에서 세그먼트 사용 준비
 * 4. SCISetSegmentAvailable()로 세그먼트를 해당 어댑터에서 공개
 * 5. nvm_dis_dma_map_local()로 NVMe 컨트롤러의 DMA 주소 공간에 매핑
 *    -> 이제 NVMe 컨트롤러가 이 세그먼트에 DMA read/write 가능
 */
#include <stddef.h>         // size_t: 크기 타입
#include <stdint.h>         // uint32_t: 세그먼트 ID, 어댑터 번호 등
#include <errno.h>          // EIO, EEXIST, ENOSPC: 에러 코드
#include <nvm_types.h>      // nvm_ctrl_t, nvm_dma_t: libnvm 타입
#include <nvm_dma.h>        // nvm_dis_dma_map_local(): SISCI 세그먼트를 NVMe DMA로 매핑, nvm_dma_unmap(): 해제
#include <nvm_util.h>       // libnvm 유틸리티 매크로
#include <sisci_types.h>    // sci_desc_t, sci_local_segment_t, sci_error_t: SISCI 타입 정의
#include <sisci_api.h>      // SCIOpen/Close, SCICreateSegment/RemoveSegment, SCIPrepareSegment 등 SISCI API
#include <sisci_error.h>    // SCI_ERR_OK, SCI_ERR_BUSY, SCI_ERR_SEGMENTID_USED 등 SISCI 에러 코드
#include "segment.h"        // struct segment, 함수 선언


/*
 * segment_create - SISCI 로컬 메모리 세그먼트 생성
 *
 * @segment:    결과를 저장할 segment 구조체 포인터
 * @segment_id: 세그먼트 ID 힌트 (SCI_FLAG_AUTO_ID로 실제 ID는 SISCI가 자동 결정)
 * @size:       세그먼트 크기 (바이트)
 * @return:     성공 시 0, 실패 시 에러 코드
 *
 * [동작 흐름]
 * 1. SCIOpen()으로 SISCI 가상 디바이스 디스크립터 획득
 * 2. SCICreateSegment()로 물리 메모리 세그먼트 생성
 *    - SCI_FLAG_AUTO_ID: segment_id를 힌트로만 사용, 충돌 시 자동 재할당
 * 3. SCIGetLocalSegmentId()로 실제 할당된 세그먼트 ID 조회
 *
 * [에러 처리]
 * - SCIOpen 실패: EIO 반환
 * - SCICreateSegment 실패: SCI_ERR_SEGMENTID_USED면 EEXIST, 그 외 ENOSPC
 *   디스크립터는 SCIClose()로 해제
 */
int segment_create(struct segment* segment, uint32_t segment_id, size_t size)
{
    sci_error_t err;        // 현재 SISCI 호출의 에러 코드
    sci_error_t status;     // 정리(cleanup) 과정의 에러 코드 (SCIClose용)

    /* SISCI 가상 디바이스 디스크립터 열기: 이후 모든 SISCI 호출의 컨텍스트 */
    SCIOpen(&segment->sd, 0, &err);     // 두 번째 인자 0: 기본 플래그
    if (err != SCI_ERR_OK)
    {
        return EIO;     // I/O 에러: SISCI 디바이스 접근 실패
    }

    /* 로컬 물리 메모리 세그먼트 생성 */
    // SCI_FLAG_AUTO_ID: segment_id는 힌트로만 사용, 충돌 시 SISCI가 자동으로 다른 ID 할당
    // NULL, NULL: 콜백 함수와 콜백 인자 (세그먼트 이벤트 알림 불필요)
    SCICreateSegment(segment->sd, &segment->segment, segment_id, size, NULL, NULL, SCI_FLAG_AUTO_ID, &err);
    if (err != SCI_ERR_OK)
    {
        SCIClose(segment->sd, 0, &status);  // 실패 시 디스크립터 정리

        if (err == SCI_ERR_SEGMENTID_USED)  // 세그먼트 ID가 이미 다른 프로세스에 의해 사용 중
        {
            return EEXIST;
        }

        return ENOSPC;      // 메모리 부족 또는 기타 세그먼트 생성 실패
    }

    /* 실제 할당된 세그먼트 ID와 크기를 구조체에 저장 */
    segment->id = SCIGetLocalSegmentId(segment->segment);   // 자동 할당된 실제 ID 조회
    segment->size = size;
    return 0;
}


/*
 * segment_remove - SISCI 로컬 세그먼트 제거
 *
 * @segment: 제거할 세그먼트 구조체
 *
 * SCIRemoveSegment()가 SCI_ERR_BUSY를 반환하면 다른 프로세스/어댑터가
 * 아직 세그먼트를 사용 중이므로, 해제될 때까지 재시도한다 (busy-wait).
 * 세그먼트 제거 후 SCIClose()로 디스크립터도 해제한다.
 */
void segment_remove(struct segment* segment)
{
    sci_error_t err;

    // 세그먼트 제거 시도: 다른 프로세스가 접근 중이면 SCI_ERR_BUSY 반환
    do
    {
        SCIRemoveSegment(segment->segment, 0, &err);   // 두 번째 인자 0: 기본 플래그
    }
    while (err == SCI_ERR_BUSY);    // busy-wait: 다른 사용자가 해제할 때까지 반복

    SCIClose(segment->sd, 0, &err); // SISCI 가상 디바이스 디스크립터 닫기
}


/*
 * dma_create - SISCI 세그먼트를 NVMe 컨트롤러의 DMA 윈도우로 매핑
 *
 * @window:   결과 DMA 핸들 포인터
 * @ctrl:     NVMe 컨트롤러 핸들
 * @segment:  매핑할 SISCI 세그먼트
 * @adapter:  DIS 어댑터 번호
 * @return:   성공 시 0, 실패 시 에러 코드
 *
 * [동작 흐름]
 * 1. SCIPrepareSegment()로 지정 어댑터에서 세그먼트 사용 준비
 * 2. SCISetSegmentAvailable()로 세그먼트를 어댑터에서 공개 (원격 DMA 허용)
 * 3. nvm_dis_dma_map_local()로 NVMe 컨트롤러의 DMA 주소 공간에 매핑
 *    - 마지막 인자 true는 가상 주소 매핑도 함께 수행하라는 의미
 *    - 이후 window->vaddr로 CPU 접근, window->ioaddrs[]로 NVMe DMA 접근 가능
 *
 * [에러 처리]
 * - SCIPrepareSegment 실패: ENOSPC 반환
 * - SCISetSegmentAvailable 실패: EIO 반환
 * - nvm_dis_dma_map_local 실패: 세그먼트를 다시 비공개로 전환한 뒤 에러 반환
 */
int dma_create(nvm_dma_t** window, const nvm_ctrl_t* ctrl, struct segment* segment, uint32_t adapter)
{
    sci_error_t err;

    /* 지정 어댑터에서 세그먼트 접근 준비: DIS 라우팅 테이블에 세그먼트 등록 */
    SCIPrepareSegment(segment->segment, adapter, 0, &err);  // 세 번째 인자 0: 기본 플래그
    if (err != SCI_ERR_OK)
    {
        return ENOSPC;  // 어댑터 리소스 부족 또는 준비 실패
    }

    /* 세그먼트를 어댑터에서 공개: 원격 노드의 NVMe 컨트롤러가 DIS 패브릭을 통해 이 메모리에 DMA 접근 가능 */
    SCISetSegmentAvailable(segment->segment, adapter, 0, &err);
    if (err != SCI_ERR_OK)
    {
        return EIO;     // 세그먼트 공개 실패 (어댑터 문제 또는 권한 부족)
    }

    /* libnvm을 통해 NVMe 컨트롤러의 DMA 주소 공간에 세그먼트 매핑 */
    // true: 가상 주소도 함께 매핑 (window->vaddr로 CPU 접근 가능)
    int status = nvm_dis_dma_map_local(window, ctrl, adapter, segment->segment, true);
    if (status != 0)
    {
        /* 매핑 실패 시 세그먼트를 다시 비공개로 전환하여 정리 */
        do
        {
            SCISetSegmentUnavailable(segment->segment, adapter, 0, &err);
        }
        while (err == SCI_ERR_BUSY);    // 다른 접근이 완료될 때까지 재시도

        return status;
    }

    return 0;
}


/*
 * dma_remove - DMA 윈도우 해제 및 세그먼트 비공개 전환
 *
 * @window:   해제할 DMA 핸들
 * @segment:  대상 SISCI 세그먼트
 * @adapter:  비공개로 전환할 DIS 어댑터 번호
 *
 * 1. nvm_dma_unmap()으로 NVMe DMA 매핑 해제
 * 2. SCISetSegmentUnavailable()로 세그먼트를 어댑터에서 비공개로 전환
 *    - SCI_ERR_BUSY 시 재시도 (다른 프로세스가 아직 접근 중일 수 있음)
 */
void dma_remove(nvm_dma_t* window, struct segment* segment, uint32_t adapter)
{
    sci_error_t err;

    /* NVMe DMA 매핑 해제: 컨트롤러가 더 이상 이 메모리에 DMA 접근 불가 */
    nvm_dma_unmap(window);

    /* 세그먼트를 어댑터에서 비공개로 전환: 원격 DMA 접근 차단 */
    do
    {
        SCISetSegmentUnavailable(segment->segment, adapter, 0, &err);
    }
    while (err == SCI_ERR_BUSY);    // 진행 중인 DMA 전송이 완료될 때까지 재시도
}
