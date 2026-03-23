/*
 * segment.h - SISCI 세그먼트 및 DMA 윈도우 관리 헤더 (Deprecated)
 *
 * [파일 역할]
 * Dolphin Interconnect Solutions(DIS)의 SISCI API를 사용하여 로컬 메모리 세그먼트를
 * 생성하고, 이를 NVMe 컨트롤러가 DMA로 접근 가능한 윈도우로 매핑하는 유틸리티.
 * RPC 예제에서 SQ/CQ 메모리, Identify 버퍼, I/O 데이터 버퍼 등에 사용된다.
 *
 * [Deprecated 배경]
 * SISCI 기반 DIS 클러스터 원격 NVMe 접근에서 커널 모듈 기반 로컬 GPU-NVMe
 * 직접 접근으로 전환되면서 deprecated되었다.
 *
 * [SISCI 세그먼트 공유 방식]
 * SISCI(Software Infrastructure for Shared-memory Computer Interconnects)는
 * DIS 클러스터에서 노드 간 공유 메모리를 제공하는 API이다.
 * 1. SCICreateSegment()로 로컬 노드에 메모리 세그먼트 생성
 * 2. SCIPrepareSegment()로 특정 어댑터에서 세그먼트 접근 준비
 * 3. SCISetSegmentAvailable()로 원격 노드가 세그먼트에 접근 가능하게 설정
 * 4. nvm_dis_dma_map_local()로 NVMe 컨트롤러가 이 세그먼트를 DMA 대상으로 인식하게 매핑
 * 이렇게 하면 원격 노드의 NVMe 컨트롤러가 DIS 패브릭을 통해 이 메모리에 직접 DMA 가능.
 */
#ifndef __DIS_NVM_EXAMPLES_SEGMENT_H__  // 인클루드 가드: 중복 포함 방지
#define __DIS_NVM_EXAMPLES_SEGMENT_H__
#ifdef __DIS_CLUSTER__                  // DIS 클러스터 빌드에서만 이 헤더의 내용을 포함

#include <nvm_types.h>      // nvm_ctrl_t: NVMe 컨트롤러 핸들, nvm_dma_t: DMA 윈도우 핸들
#include <stddef.h>         // size_t: 세그먼트 크기 타입
#include <stdint.h>         // uint32_t: 세그먼트 ID, 어댑터 번호
#include <sisci_types.h>    // sci_desc_t: SISCI 가상 디바이스 디스크립터, sci_local_segment_t: 로컬 세그먼트 핸들


/*
 * segment - SISCI 로컬 세그먼트 디스크립터 래퍼 구조체
 *
 * SISCI의 세그먼트 관련 핸들과 메타데이터를 하나로 묶어 관리한다.
 *
 * @id:      세그먼트 ID (SCICreateSegment 시 SCI_FLAG_AUTO_ID로 자동 할당됨)
 * @sd:      SISCI 가상 디바이스 디스크립터 (SCIOpen으로 획득, 모든 SISCI 호출의 컨텍스트)
 * @segment: SISCI 로컬 세그먼트 핸들 (물리 메모리 영역을 가리킴)
 * @size:    세그먼트 크기 (바이트)
 */
struct segment
{
    uint32_t            id;         // Segment ID: SISCI가 자동 할당한 고유 식별자
    sci_desc_t          sd;         // SISCI virtual device: 세그먼트 생성/관리의 컨텍스트
    sci_local_segment_t segment;    // Local segment descriptor: 실제 물리 메모리 영역 핸들
    size_t              size;       // Size of segment: 바이트 단위 세그먼트 크기
};



/*
 * segment_create - SISCI 로컬 세그먼트 생성
 *
 * @segment:    결과를 저장할 segment 구조체 포인터
 * @segment_id: 세그먼트 ID 힌트 (SCI_FLAG_AUTO_ID 사용으로 실제 ID는 자동 할당)
 * @size:       세그먼트 크기 (바이트)
 * @return:     성공 시 0, SCIOpen 실패 시 EIO, 세그먼트 ID 충돌 시 EEXIST, 기타 ENOSPC
 */
int segment_create(struct segment* segment, uint32_t segment_id, size_t size);


/*
 * segment_remove - SISCI 로컬 세그먼트 제거
 *
 * @segment: 제거할 세그먼트 구조체 포인터
 *
 * SCI_ERR_BUSY가 반환되면 다른 프로세스가 세그먼트를 사용 중이므로 재시도한다.
 * 세그먼트 제거 후 SCIClose()로 SISCI 디스크립터도 해제한다.
 */
void segment_remove(struct segment* segment);



/*
 * dma_create - SISCI 세그먼트를 NVMe DMA 윈도우로 매핑
 *
 * @dma_window:  결과 DMA 핸들을 저장할 포인터
 * @ctrl:        NVMe 컨트롤러 핸들 (DMA 주소 공간의 소유자)
 * @segment:     매핑할 SISCI 세그먼트 (물리 메모리)
 * @dis_adapter: DIS 어댑터 번호 (세그먼트를 공개할 어댑터)
 * @return:      성공 시 0, 실패 시 에러 코드
 *
 * SCIPrepareSegment() -> SCISetSegmentAvailable() -> nvm_dis_dma_map_local() 순서로 처리.
 * 매핑 후 dma_window->vaddr로 CPU 접근, dma_window->ioaddrs[]로 NVMe DMA 접근 가능.
 */
int dma_create(nvm_dma_t** dma_window, const nvm_ctrl_t* ctrl, struct segment* segment, uint32_t dis_adapter);



/*
 * dma_remove - DMA 윈도우 해제 및 세그먼트 비공개 설정
 *
 * @dma_window:  해제할 DMA 핸들
 * @segment:     대상 세그먼트
 * @dis_adapter: 비공개로 전환할 DIS 어댑터 번호
 *
 * nvm_dma_unmap()로 NVMe DMA 매핑 해제 후 SCISetSegmentUnavailable()로 세그먼트 비공개 전환.
 * SCI_ERR_BUSY 시 재시도하여 진행 중인 DMA가 완료될 때까지 기다린다.
 */
void dma_remove(nvm_dma_t* dma_window, struct segment* segment, uint32_t dis_adapter);


#endif // __DIS_CLUSTER__
#endif // __DIS_NVM_EXAMPLES_SEGMENT_H__
