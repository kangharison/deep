/*
 * read.h - NVMe 블록 읽기/쓰기 예제 헤더 (deprecated)
 *
 * NVMe Read/Write 명령으로 디스크 블록을 읽고 쓰는 예제의 공통 헤더.
 * disk_info: 컨트롤러/네임스페이스 정보 (MDTS, 페이지 크기, 블록 크기)
 * queue_pair: I/O SQ/CQ 쌍과 DMA 메모리
 *
 * 세 가지 백엔드(module/smartio/userspace)에서 공통으로 사용.
 */
#ifndef __LIBNVM_SAMPLES_READ_BLOCKS_READ_H__  // 인클루드 가드: 중복 포함 방지
#define __LIBNVM_SAMPLES_READ_BLOCKS_READ_H__

#include <stdint.h>       // uint32_t: 네임스페이스 ID용 고정 크기 정수
#include <stdbool.h>      // bool: stop 플래그용 불리언 타입
#include <nvm_types.h>    // nvm_dma_t: DMA 윈도우, nvm_queue_t: SQ/CQ 구조체, nvm_aq_ref: Admin Queue 참조
#include "args.h"         // struct options: 커맨드라인 파싱 결과


/*
 * disk_info - NVMe 디스크 정보 (Identify Controller/Namespace에서 획득)
 *
 * NVMe I/O 명령 구성에 필요한 디스크 파라미터를 담는다.
 * get_disk_info()에서 Admin Identify 명령으로 채워진다.
 *
 * @ns_id:         네임스페이스 ID
 * @max_data_size: MDTS (한 NVMe 명령으로 전송 가능한 최대 바이트)
 * @page_size:     컨트롤러 페이지 크기 (CC.MPS, 보통 4KB)
 * @block_size:    논리 블록(LBA) 크기 (보통 512B 또는 4KB)
 */
struct disk_info
{
    uint32_t    ns_id;          /* NVMe 네임스페이스 ID: Identify Namespace 명령으로 획득한 NSID */
    size_t      max_data_size;  /* MDTS (바이트): 한 NVMe I/O 명령이 전송 가능한 최대 데이터 크기. 이를 초과하면 분할 필요 */
    size_t      page_size;      /* 컨트롤러 메모리 페이지 크기 (바이트): PRP 엔트리가 가리키는 물리 페이지 단위 */
    size_t      block_size;     /* 논리 블록(LBA) 크기 (바이트): NVMe Read/Write 명령의 최소 전송 단위 */
};



/*
 * queue_pair - I/O SQ/CQ 쌍 및 폴링 상태
 *
 * CQ 폴링 스레드와 SQ 제출 스레드가 이 구조체를 공유한다.
 * stop 플래그로 폴링 스레드를 안전하게 종료하고,
 * num_cpls로 완료된 명령 수를 추적한다.
 *
 * [멀티스레드 모델]
 * - 메인 스레드: SQ에 NVMe 명령을 enqueue하고 submit (doorbell 링)
 * - 폴링 스레드: CQ에서 완료 엔트리를 dequeue하여 num_cpls 증가
 * - 메인 스레드는 num_cpls가 제출 수와 같아질 때까지 busy-wait
 */
struct queue_pair
{
    nvm_dma_t*  sq_mem;    /* SQ DMA 윈도우: SQ 엔트리 배열 + PRP 리스트가 저장되는 DMA 매핑 메모리 */
    nvm_dma_t*  cq_mem;    /* CQ DMA 윈도우: CQ 엔트리 배열이 저장되는 DMA 매핑 메모리 */
    nvm_queue_t sq;        /* Submission Queue 상태: doorbell MMIO 주소, head/tail 포인터, 큐 크기(qs) */
    nvm_queue_t cq;        /* Completion Queue 상태: doorbell MMIO 주소, head/tail 포인터, phase bit */
    bool        stop;      /* 종료 플래그: true이면 CQ 폴링 스레드에 종료 신호를 보냄 */
    size_t      num_cpls;  /* 완료 카운터: CQ 폴링 스레드가 처리(dequeue)한 완료 엔트리 수 */
};



/* get_disk_info - Identify Controller/Namespace Admin 명령으로 디스크 정보 획득. show=true이면 컨트롤러 정보를 stderr에 출력 */
int get_disk_info(nvm_aq_ref ref, struct disk_info* info, uint32_t ns_id, void* ptr, uint64_t ioaddr, bool show);

/* create_queue_pair - I/O CQ/SQ 쌍 생성. Set Features(Number of Queues) → Admin Create I/O CQ → Admin Create I/O SQ 순서로 수행 */
int create_queue_pair(nvm_aq_ref ref, struct queue_pair* qp, nvm_dma_t* cq_mem, nvm_dma_t* sq_mem, size_t sqs);

/* read_and_dump - NVMe Read 수행 후 stdout/파일로 덤프. MDTS 이내의 청크 단위로 반복하며, 각 청크의 처리량(MB/s)을 출력 */
int read_and_dump(const struct disk_info* disk, struct queue_pair* qp, const nvm_dma_t* buffer, const struct options* args);

/* write_blocks - 파일에서 읽어 NVMe Write 수행. 각 청크 Write 후 NVMe Flush 명령으로 volatile write cache → NVM 미디어 영속화 */
int write_blocks(const struct disk_info* disk, struct queue_pair* qp, const nvm_dma_t* buffer, const struct options* args);


#endif  // __LIBNVM_SAMPLES_READ_BLOCKS_READ_H__
