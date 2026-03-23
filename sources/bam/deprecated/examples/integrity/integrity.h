/*
 * integrity.h - NVMe 데이터 무결성 검증 예제 헤더 (deprecated)
 *
 * 데이터를 NVMe SSD에 쓰고 다시 읽어 원본과 비교하여 무결성을 검증하는 예제.
 * buffer(DMA 메모리), queue(SQ/CQ 쌍), disk(디바이스 정보) 구조체를 정의한다.
 *
 * Deprecated: BaM의 GPU 직접 I/O 방식으로 대체됨
 */
#ifndef __LIBNVM_SAMPLES_INTEGRITY_H__  // 인클루드 가드
#define __LIBNVM_SAMPLES_INTEGRITY_H__

#include <nvm_types.h>  // libnvm 기본 타입 (nvm_dma_t, nvm_queue_t, nvm_aq_ref 등)
#include <stdio.h>      // FILE* (디스크 읽기/쓰기에서 파일 포인터로 사용)
#include <stdint.h>     // uint32_t, uint16_t 등 고정 크기 정수


/*
 * buffer - DMA 메모리 디스크립터
 *
 * @buffer: posix_memalign으로 할당한 호스트 메모리 포인터 (DIS 모드에서는 NULL)
 * @dma:    libnvm DMA 윈도우 (가상 주소, IO 주소, 페이지 수 포함)
 *
 * NVMe Read/Write 명령에서 데이터 전송 버퍼로 사용된다.
 * DMA 윈도우의 ioaddrs[]가 NVMe PRP 리스트에 설정되어 컨트롤러가 직접 접근한다.
 */
struct buffer
{
    void*                   buffer;  /* 호스트 메모리 포인터 (free용, DIS 모드에서는 SISCI가 관리하므로 NULL) */
    nvm_dma_t*              dma;     /* libnvm DMA 윈도우: vaddr(CPU 접근용), ioaddrs[](NVMe DMA용), n_ioaddrs(페이지 수) */
};


/*
 * queue - NVMe I/O 큐 디스크립터
 *
 * @qmem:    큐 메모리 + PRP 리스트용 DMA 버퍼
 * @queue:   libnvm 큐 구조체 (SQ 또는 CQ, 도어벨 주소/헤드/테일 포함)
 * @counter: 이 큐에서 제출(SQ) 또는 완료(CQ)된 명령 수
 */
struct queue
{
    struct buffer           qmem;    /* 큐 엔트리가 저장되는 DMA 메모리 (SQ의 경우 PRP 리스트도 포함) */
    nvm_queue_t             queue;   /* libnvm 큐 상태: 도어벨 레지스터 주소, head/tail 포인터, 큐 크기 */
    size_t                  counter; /* 제출(SQ) 또는 완료(CQ) 명령 카운터 (producer-consumer 동기화에 사용) */
};


/*
 * disk - NVMe 디스크 정보 (Identify Controller/Namespace에서 획득)
 *
 * @page_size:      컨트롤러 페이지 크기 (CC.MPS, 보통 4KB)
 * @max_data_size:  최대 데이터 전송 크기 (MDTS, 한 명령으로 전송 가능한 최대 바이트)
 * @ns_id:          네임스페이스 ID
 * @block_size:     논리 블록 크기 (LBA 크기, 보통 512B 또는 4KB)
 */
struct disk
{
    size_t      page_size;      /* 컨트롤러 페이지 크기 (바이트): CC.MPS 레지스터에서 결정 */
    size_t      max_data_size;  /* MDTS: 한 NVMe 명령의 최대 전송 크기 (바이트 단위) */
    uint32_t    ns_id;          /* NVMe 네임스페이스 ID (1부터 시작) */
    size_t      block_size;     /* 논리 블록(LBA) 크기 (바이트): Identify Namespace에서 획득 */
};


/* create_buffer - DMA 버퍼 생성 (로컬: posix_memalign + nvm_dma_map_host, DIS: nvm_dis_dma_create) */
int create_buffer(struct buffer* b, nvm_aq_ref, size_t size);  // ref에서 컨트롤러 정보를 얻어 DMA 매핑 수행

/* remove_buffer - DMA 버퍼 해제 (DMA 언맵 + 메모리 free) */
void remove_buffer(struct buffer* b);  // buffer->dma 언맵 + buffer->buffer free


/* create_queue - NVMe I/O 큐 생성. cq==NULL이면 CQ 생성, 아니면 해당 CQ에 연결된 SQ 생성 */
int create_queue(struct queue* q, nvm_aq_ref ref, const struct queue* cq, uint16_t qno);  // Admin Create CQ/SQ 명령 사용

/* remove_queue - NVMe I/O 큐 해제 (큐 메모리 DMA 버퍼 해제) */
void remove_queue(struct queue* q);  // qmem 버퍼 해제


/* disk_write - 파일에서 읽어 NVMe에 쓰기. producer-consumer 패턴의 멀티 큐 병렬 전송 */
int disk_write(const struct disk* disk, struct buffer* buffer, struct queue* queues, uint16_t n_queues, FILE* fp, off_t size);

/* disk_read - NVMe에서 읽어 파일에 쓰기. 멀티 큐 병렬 전송 */
int disk_read(const struct disk* disk, struct buffer* buffer, struct queue* queues, uint16_t n_queues, FILE* fp, off_t size);


#endif  // __LIBNVM_SAMPLES_INTEGRITY_H__
