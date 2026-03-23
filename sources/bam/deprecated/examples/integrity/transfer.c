/*
 * transfer.c - NVMe Read/Write 전송 로직 구현 (deprecated)
 *
 * NVMe I/O 큐(SQ/CQ)를 사용하여 블록 데이터를 읽고 쓰는 핵심 전송 함수를 구현한다.
 * NVMe 커맨드 빌드(nvm_cmd_header/data_ptr/rw_blks) → SQ enqueue → 도어벨 → CQ 폴링 흐름.
 * MDTS(Max Data Transfer Size)를 준수하여 큰 전송을 분할한다.
 *
 * disk_write(): 파일에서 데이터를 읽어 NVMe에 순차 Write
 * disk_read(): NVMe에서 순차 Read하여 파일에 기록
 */
#define _GNU_SOURCE
#include <nvm_types.h>    // libnvm 기본 타입
#include <nvm_util.h>     // NVM_PAGE_ALIGN, NVM_BLOCK_TO_PAGE, NVM_PAGE_TO_BLOCK 등 매크로
#include <nvm_cmd.h>      // nvm_cmd_header, nvm_cmd_data, nvm_cmd_rw_blks, nvm_cmd_data_ptr
#include <nvm_dma.h>      // nvm_dma_t (DMA 윈도우)
#include <nvm_admin.h>    // Admin 명령 (미사용이지만 헤더 의존성)
#include <nvm_error.h>    // NVM_ERR_OK, NVM_ERR_STATUS, nvm_strerror
#include <nvm_queue.h>    // nvm_sq_enqueue, nvm_sq_submit, nvm_cq_dequeue_block, nvm_sq_update, nvm_cq_update
#include <stdio.h>        // fprintf, stderr
#include <stdint.h>       // uint32_t
#include <stddef.h>       // size_t
#include <stdlib.h>       // calloc, free
#include <stdbool.h>      // bool
#include <string.h>       // memset
#include <errno.h>        // errno
#include <pthread.h>      // pthread_create, pthread_join
#include "integrity.h"    // buffer, queue, disk 구조체


/*
 * consumer - CQ 완료 폴링 스레드 컨텍스트
 *
 * 별도 스레드에서 CQ를 폴링하여 완료된 명령을 처리한다.
 * 다수의 producer(SQ 제출) 스레드와 1개의 consumer(CQ 폴링) 스레드가 협력한다.
 */
struct consumer
{
    pthread_t           thread;     /* CQ 폴링 스레드 핸들 */
    struct queue*       queues;     /* 큐 배열 (인덱스 0이 CQ, 1~n이 SQ) */
    uint16_t            n_queues;   /* SQ 수 */
    bool                cancel;     /* true이면 폴링 스레드 종료 요청 */
};


/*
 * producer - SQ 명령 제출 스레드 컨텍스트
 *
 * 각 SQ에 대해 하나의 producer 스레드가 NVMe Read/Write 명령을 제출한다.
 * 데이터 영역은 start_block부터 n_blocks개 블록을 담당한다.
 */
struct producer
{
    pthread_t           thread;       /* SQ 제출 스레드 핸들 */
    bool                write;        /* true=NVMe Write, false=NVMe Read */
    struct buffer*      buffer;       /* 데이터 DMA 버퍼 (공유, 영역별로 분할 사용) */
    uint16_t            queue_no;     /* 이 스레드가 사용할 SQ 인덱스 (1부터) */
    struct queue*       queues;       /* 큐 배열 (CQ + SQ 전체) */
    const struct disk*  disk;         /* 디스크 정보 (MDTS, 블록 크기 등) */
    size_t              start_block;  /* 이 스레드의 시작 LBA */
    size_t              n_blocks;     /* 이 스레드가 처리할 블록 수 */
};


/*
 * consume_completions - CQ 완료 폴링 스레드 함수
 *
 * @c:      consumer 컨텍스트
 * @return: consumer 포인터 (pthread_join에서 회수)
 *
 * CQ에서 완료 엔트리를 dequeue하고, 해당 SQ의 헤드 포인터를 갱신한다.
 * 에러 발생 시 stderr에 출력. cancel 플래그가 설정되면 루프 종료.
 */
static struct consumer* consume_completions(struct consumer* c)
{
    nvm_cpl_t* cpl;                                // CQ 완료 엔트리 포인터
    nvm_queue_t* cq = &c->queues[0].queue;         // CQ는 큐 배열 인덱스 0
    nvm_queue_t* sq = NULL;                         // 완료 엔트리가 참조하는 SQ

    while (!c->cancel)  // 취소 요청이 올 때까지 폴링
    {
        cpl = nvm_cq_dequeue_block(cq, 10);  // CQ에서 완료 엔트리 dequeue (10ms 타임아웃)

        if (cpl == NULL)  // 타임아웃: 완료 엔트리 없음
        {
            sched_yield();  // 다른 스레드에 CPU 양보
            continue;
        }

        sq = &c->queues[*NVM_CPL_SQID(cpl)].queue;  // 완료 엔트리의 SQID 필드로 해당 SQ 식별
        nvm_sq_update(sq);  // 완료된 명령의 SQ head 포인터 갱신 (SQ 슬롯 회수)

        if (!NVM_ERR_OK(cpl))  // NVMe 에러 확인
        {
            fprintf(stderr, "%s\n", nvm_strerror(NVM_ERR_STATUS(cpl)));  // 에러 문자열 출력
        }

        nvm_cq_update(cq);         // CQ head 포인터 갱신 + CQ 도어벨 링
        c->queues[0].counter++;    // 완료 카운터 증가
    }

    return c;
}


/*
 * produce_commands - SQ 명령 제출 스레드 함수
 *
 * @p:      producer 컨텍스트
 * @return: producer 포인터 (pthread_join에서 회수)
 *
 * [동작]
 * 1. 담당 블록 범위를 MDTS 단위(transfer_pages)로 분할
 * 2. 각 분할에 대해: SQ enqueue -> 명령 헤더(Read/Write) -> PRP 리스트 설정 -> LBA 범위 설정
 * 3. Write 모드이면 마지막에 Flush 명령 추가
 * 4. SQ doorbell 링
 */
static struct producer* produce_commands(struct producer* p)
{
    nvm_cmd_t* cmd;                        // SQ에 삽입할 명령 포인터
    size_t block_size = p->disk->block_size;   // 논리 블록 크기 (보통 512B)
    size_t page_size = p->buffer->dma->page_size;  // 컨트롤러 페이지 크기 (보통 4KB)

    // 이 스레드가 처리할 총 페이지 수
    size_t n_pages = NVM_PAGE_ALIGN(p->n_blocks * block_size, page_size) / page_size;

    // 한 NVMe 명령으로 전송 가능한 최대 페이지 수 (MDTS / page_size)
    size_t transfer_pages = p->disk->max_data_size / page_size;

    // DMA 버퍼 내 이 스레드의 시작 페이지 오프셋
    size_t page_base = NVM_BLOCK_TO_PAGE(page_size, block_size, p->start_block);
    size_t page_offset = 0;  // 현재 전송 중인 페이지 오프셋

    uint32_t ns_id = p->disk->ns_id;       // NVMe 네임스페이스 ID

    nvm_dma_t* dma = p->buffer->dma;       // 데이터 DMA 윈도우
    struct queue* queue = &p->queues[p->queue_no];  // 이 스레드가 사용할 SQ의 queue 구조체
    nvm_dma_t* prp = queue->qmem.dma;      // PRP 리스트가 저장된 DMA 메모리

    nvm_queue_t* sq = &queue->queue;        // SQ 핸들

    while (page_offset < n_pages)  // 모든 페이지를 전송할 때까지 반복
    {
        // 남은 페이지가 MDTS보다 적으면 남은 페이지만 전송
        if (n_pages - page_offset < transfer_pages)
        {
            transfer_pages = n_pages - page_offset;
        }

        // SQ 슬롯 확보 (가득 차면 doorbell 링 후 CPU 양보하며 재시도)
        while ((cmd = nvm_sq_enqueue(sq)) == NULL)
        {
            nvm_sq_submit(sq);  // 대기 중인 명령을 먼저 제출
            sched_yield();      // CPU 양보
        }

        // NVMe 명령 헤더: CID(자동생성), opcode(Read/Write), namespace ID 설정
        nvm_cmd_header(cmd, NVM_DEFAULT_CID(sq), p->write ? NVM_IO_WRITE : NVM_IO_READ, ns_id);

        // LBA 범위 계산 및 설정
        size_t n_blocks = NVM_PAGE_TO_BLOCK(page_size, block_size, transfer_pages);
        size_t start_block = p->start_block + NVM_PAGE_TO_BLOCK(page_size, block_size, page_offset);
        nvm_cmd_rw_blks(cmd, start_block, n_blocks);  // CDW10-11: 시작 LBA, CDW12: 블록 수

        // PRP 리스트 설정: CID를 인덱스로 사용하여 PRP 리스트 위치 결정
        uint16_t prp_no = (*NVM_CMD_CID(cmd) % sq->qs) + 1;
        nvm_prp_list_t prp_desc = NVM_PRP_LIST(prp, prp_no);  // PRP 리스트 디스크립터

        // 데이터 버퍼의 DMA 주소를 PRP 리스트에 설정
        nvm_cmd_data(cmd, 1, &prp_desc, transfer_pages, &dma->ioaddrs[page_base + page_offset]);

        page_offset += transfer_pages;  // 다음 전송 위치로 이동
        queue->counter++;               // 제출 카운터 증가
    }

    if (p->write)  // Write 모드이면 Flush 명령 추가 (volatile write cache → NVM 미디어)
    {
        while ((cmd = nvm_sq_enqueue(sq)) == NULL);  // SQ 슬롯 대기
        nvm_cmd_header(cmd, NVM_DEFAULT_CID(sq), NVM_IO_FLUSH, ns_id);  // Flush opcode
        nvm_cmd_data_ptr(cmd, 0, 0);    // Flush는 데이터 전송 없음 (PRP=0)
        nvm_cmd_rw_blks(cmd, 0, 0);     // LBA 범위도 0
        queue->counter++;               // Flush도 카운터에 포함
    }

    nvm_sq_submit(sq);  // 최종 SQ doorbell 링 (남은 명령 일괄 제출)

    return p;
}



/*
 * transfer - 멀티 큐 병렬 NVMe 데이터 전송
 *
 * @d:        디스크 정보
 * @buffer:   데이터 DMA 버퍼
 * @queues:   큐 배열 (인덱스 0=CQ, 1~n=SQ)
 * @n_queues: SQ 수
 * @size:     전송할 총 바이트 수
 * @write:    true=Write, false=Read
 * @return:   성공 시 0, 실패 시 -1
 *
 * [아키텍처: producer-consumer 패턴]
 * 1. consumer 스레드 1개 시작 (CQ 폴링)
 * 2. producer 스레드 n_queues개 시작 (각 SQ에 명령 제출)
 * 3. 각 producer는 전체 블록을 균등 분할하여 담당
 * 4. 모든 producer 완료 대기 -> consumer 종료 대기
 */
static int transfer(const struct disk* d, struct buffer* buffer, struct queue* queues, uint16_t n_queues, off_t size, bool write)
{
    // 전체 블록 수 및 페이지 수 계산 (블록 크기와 페이지 크기로 각각 올림 정렬)
    size_t n_blocks = NVM_PAGE_ALIGN(size, d->block_size) / d->block_size;
    size_t n_pages = NVM_PAGE_ALIGN(size, d->page_size) / d->page_size;

    size_t pages_per_queue = n_pages / n_queues;  // 각 SQ가 담당할 페이지 수

    // producer 스레드 컨텍스트 배열 할당
    struct producer* producers = calloc(n_queues, sizeof(struct producer));
    if (producers == NULL)
    {
        fprintf(stderr, "Failed to allocate thread descriptors\n");
        return -1;
    }

    // consumer (CQ 폴링) 스레드 설정 및 시작
    struct consumer consumer;
    consumer.queues = queues;
    consumer.n_queues = n_queues;
    consumer.cancel = false;

    pthread_create(&consumer.thread, NULL, (void *(*)(void*)) consume_completions, &consumer);  // CQ 폴링 스레드 시작

    // producer (SQ 제출) 스레드들 시작
    for (uint16_t i = 0; i < n_queues; ++i)
    {
        producers[i].write = write;                    // Read/Write 방향
        producers[i].buffer = buffer;                  // 공유 데이터 버퍼
        producers[i].queue_no = i + 1;                 // SQ 인덱스 (1부터)
        producers[i].queues = queues;                  // 큐 배열 참조
        producers[i].disk = d;                         // 디스크 정보
        // 각 producer의 시작 블록 = 페이지 기반으로 계산 (블록-페이지 변환)
        producers[i].start_block = NVM_PAGE_TO_BLOCK(d->page_size, d->block_size, pages_per_queue * i);
        producers[i].n_blocks = NVM_PAGE_TO_BLOCK(d->page_size, d->block_size, pages_per_queue);

        if (i == n_queues - 1)  // 마지막 producer는 나머지 블록 전부 담당
        {
            producers[i].n_blocks = n_blocks - producers[i].start_block;
        }

        pthread_create(&producers[i].thread, NULL, (void *(*)(void*)) produce_commands, &producers[i]);
        fprintf(stderr, "\tQueue #%u: block %zu to block %zu (page %zu + %zu)\n",
                i, producers[i].start_block, producers[i].start_block + producers[i].n_blocks,
                NVM_BLOCK_TO_PAGE(d->page_size, d->block_size, producers[i].start_block),
                NVM_PAGE_ALIGN(producers[i].n_blocks * d->block_size, d->page_size) / d->page_size);

    }

    // 모든 producer 스레드 완료 대기 및 총 명령 수 집계
    size_t commands = 0;
    for (uint16_t i = 0; i < n_queues; ++i)
    {
        struct producer* p;
        pthread_join(producers[i].thread, (void**) &p);  // producer 스레드 완료 대기
        commands += queues[p->queue_no].counter;          // 해당 SQ의 제출 카운터 합산
    }

    // 모든 명령이 CQ에서 완료될 때까지 busy-wait
    while (queues[0].counter < commands);

    // consumer 스레드 종료
    consumer.cancel = true;                                // 폴링 종료 요청
    pthread_join(consumer.thread, NULL);                   // consumer 스레드 완료 대기
    fprintf(stderr, "Total blocks: %zu\n", n_blocks);


    free(producers);  // producer 배열 해제
    return 0;
}



/*
 * disk_write - 파일에서 데이터를 읽어 NVMe SSD에 쓰기
 *
 * @d:        디스크 정보
 * @buffer:   데이터 DMA 버퍼
 * @queues:   큐 배열
 * @n_queues: SQ 수
 * @fp:       입력 파일 포인터
 * @size:     쓸 바이트 수
 * @return:   성공 시 0
 *
 * fread()로 파일 데이터를 DMA 버퍼에 로드한 후 transfer(write=true)를 호출한다.
 */
int disk_write(const struct disk* d, struct buffer* buffer, struct queue* queues, uint16_t n_queues, FILE* fp, off_t size)
{
    fread(buffer->dma->vaddr, 1, size, fp);  // 파일 데이터를 DMA 버퍼에 로드
    return transfer(d, buffer, queues, n_queues, size, true);  // NVMe Write 실행
}


/*
 * disk_read - NVMe SSD에서 데이터를 읽어 파일에 쓰기
 *
 * @d:        디스크 정보
 * @buffer:   데이터 DMA 버퍼
 * @queues:   큐 배열
 * @n_queues: SQ 수
 * @fp:       출력 파일 포인터
 * @size:     읽을 바이트 수
 * @return:   성공 시 0
 *
 * transfer(write=false)로 NVMe Read를 수행한 후 fwrite()로 결과를 파일에 기록한다.
 */
int disk_read(const struct disk* d, struct buffer* buffer, struct queue* queues, uint16_t n_queues, FILE* fp, off_t size)
{
    int status = transfer(d, buffer, queues, n_queues, size, false);  // NVMe Read 실행
    if (status == 0)
    {
        fwrite(buffer->dma->vaddr, 1, size, fp);  // DMA 버퍼의 데이터를 파일에 기록
        fflush(fp);                                 // 버퍼 flush하여 파일 기록 보장
    }
    return status;
}
