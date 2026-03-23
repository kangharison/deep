/*
 * rpc_dd.c - RPC를 통한 원격 NVMe 블록 Read/Write (dd 유사 도구) (Deprecated)
 *
 * [파일 역할]
 * Unix dd 명령과 유사하게, DIS 클러스터에서 원격 NVMe 디스크에 블록 단위로
 * 데이터를 읽거나 쓰는 도구. RPC 서버(rpc_server)를 통해 Admin 명령을 실행하고,
 * 직접 I/O SQ/CQ를 조작하여 NVMe Read/Write/Flush 명령을 제출한다.
 *
 * 읽기 모드: 디스크에서 지정한 LBA 범위를 읽어 stdout으로 출력
 * �기 모드: stdin에서 데이터를 읽어 디스크의 지정한 LBA에 기록
 *
 * [Deprecated 배경]
 * SISCI 기반 DIS 클러스터 원격 NVMe 접근에서 커널 모듈(libnvm.ko) 기반
 * GPU-initiated I/O로 전환되면서 deprecated되었다.
 *
 * [서버 모드 vs 클라이언트 모드]
 * --server 옵션을 지정하면 이 프로세스가 직접 Admin Queue를 생성하여
 * 컨트롤러 매니저 역할도 겸한다 (독립 실행 가능).
 * --server 없이 실행하면 별도의 rpc_server 프로세스에 RPC로 바인딩하여
 * Admin 명령을 위임한다.
 *
 * [NVMe I/O 명령 제출 흐름]
 * 1. SQ에 빈 슬롯 확보 (nvm_sq_enqueue, 큐 가득 시 usleep으로 대기)
 * 2. 명령 헤더 구성 (opcode: READ/WRITE/FLUSH)
 * 3. PRP 리스트를 통해 DMA 버퍼 주소 설정
 * 4. LBA 범위(시작 블록, 블록 수) 설정
 * 5. 캐시 flush 후 SQ doorbell 링
 * 6. CQ에서 완료 폴링 (nvm_cq_dequeue_block, 타임아웃 50ms)
 * 7. 에러 확인 후 큐 포인터 갱신
 */
#include <nvm_types.h>
#include <nvm_ctrl.h>
#include <nvm_admin.h>
#include <nvm_dma.h>
#include <nvm_rpc.h>
#include <nvm_aq.h>
#include <nvm_util.h>
#include <nvm_error.h>
#include <nvm_queue.h>
#include <nvm_cmd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sisci_api.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include "segment.h"
#include "util.h"

#define MIN(a, b)   ( (a) <= (b) ? (a) : (b) )


/*
 * arguments - 커맨드라인 옵션 구조체
 *
 * @segment_id:    SISCI 세그먼트 ID (기본 345)
 * @adapter:       DIS 어댑터 번호 (기본 0)
 * @queue_id:      사용할 I/O SQ/CQ 쌍 번호 (1부터 시작, 필수)
 * @controller_id: SmartIO Fabric Device ID (16진수, 필수)
 * @nvm_namespace: NVMe 네임스페이스 ID (기본 1)
 * @offset:        시작 LBA 오프셋 (블록 단위, 기본 0)
 * @blocks:        한 번에 전송할 블록 수 (버퍼 크기, 기본=max_data_size/block_size)
 * @count:         총 전송할 블록 수 (읽기 모드에서 필수)
 * @input:         쓰기 모드 시 stdin (NULL이면 읽기 모드)
 * @server:        true이면 서버 모드 (자체 Admin Queue 생성)
 * @num_queues:    서버 모드 시 예약할 SQ/CQ 쌍 개수
 */
struct arguments
{
    uint32_t    segment_id;
    uint32_t    adapter;
    uint16_t    queue_id;
    uint64_t    controller_id;
    uint32_t    nvm_namespace;
    uint64_t    offset;
    uint64_t    blocks;
    uint64_t    count;
    FILE*       input;
    bool        server;
    uint16_t    num_queues;
};


/*
 * disk_info - NVMe 디스크 정보 (Identify로 획득)
 *
 * @ns_id:      네임스페이스 ID
 * @page_size:  컨트롤러 페이지 크기 (보통 4KB)
 * @chunk_size: 최대 데이터 전송 크기 (MDTS, 컨트롤러가 한 명령으로 전송 가능한 최대 바이트)
 * @block_size: 논리 블록 크기 (LBA 크기, 보통 512B 또는 4KB)
 * @max_blocks: 네임스페이스 총 블록 수
 */
struct disk_info
{
    uint32_t    ns_id;
    size_t      page_size;
    size_t      chunk_size;
    size_t      block_size;
    size_t      max_blocks;
};


/*
 * queue_pair - I/O SQ/CQ 쌍 및 통계
 *
 * @cmds:   제출한 명령 수
 * @cpls:   완료된 명령 수
 * @errors: 에러 발생 수
 * @cq:     Completion Queue 구조체
 * @sq:     Submission Queue 구조체
 */
struct queue_pair
{
    size_t      cmds;
    size_t      cpls;
    size_t      errors;
    nvm_queue_t cq;
    nvm_queue_t sq;
};


/* 시그널(SIGTERM/SIGINT/SIGPIPE) 수신 플래그 */
static volatile bool signal_caught = false;


/*
 * catch_signal - 시그널 핸들러
 *
 * SIGTERM, SIGINT, SIGPIPE 수신 시 signal_caught를 true로 설정하여
 * 진행 중인 I/O 루프를 안전하게 중단시킨다.
 */
static void catch_signal()
{
    signal_caught = true;
    fprintf(stderr, "Stopping!\n");
}



static void give_usage(const char* progname)
{
    fprintf(stderr,
            "Usage: %s --ctrl <fdid> --qn <number> [--offset <count>] --count <count>\n"
            "   or: %s --ctrl <fdid> --server --nq <number> --qn <number> [--offset <count>] --count <count>\n"
            "   or: %s --ctrl <fdid> --qn <number> [--offset <count>] --input\n"
            "   or: %s --ctrl <fdid> --server --nq <number> --qn <number> [--offset <count>] --input\n"
            , progname, progname, progname, progname);
}


static void parse_args(int argc, char** argv, struct arguments* args);



/*
 * disk_w - stdin에서 읽은 데이터를 NVMe 디스크에 쓰기
 *
 * @disk:   디스크 정보 (블록 크기, 페이지 크기 등)
 * @qp:     I/O 큐 쌍 (SQ/CQ)
 * @fp:     입력 파일 포인터 (보통 stdin)
 * @blks:   한 번에 쓸 최대 블록 수 (버퍼 크기)
 * @offset: 시작 LBA
 * @wnd:    DMA 윈도우 (SQ/CQ 메모리 + PRP 리스트 + 데이터 버퍼 포함)
 *
 * [DMA 윈도우 레이아웃]
 * 페이지 0: CQ 메모리
 * 페이지 1: SQ 메모리
 * 페이지 2: PRP 리스트
 * 페이지 3~: 데이터 버퍼 (fread로 읽은 데이터가 여기에 저장됨)
 *
 * [동작 흐름]
 * 1. fread()로 stdin에서 데이터를 DMA 버퍼(페이지 3~)에 읽기
 * 2. 읽은 크기를 블록 단위로 올림 정렬 (NVM_PAGE_ALIGN)
 * 3. NVMe Write 명령 구성: 헤더 -> PRP 데이터 주소 -> LBA 범위
 * 4. nvm_cache_flush()로 CPU 캐시를 메모리로 flush (DMA 일관성 보장)
 * 5. SQ 제출 후 CQ 완료 대기 (타임아웃 50ms)
 * 6. offset을 증가시키며 EOF까지 반복
 * 7. 마지막에 NVMe Flush 명령 제출 (write cache -> NVM 미디어 flush)
 *
 * [에러 처리]
 * - SQ 가득 참: usleep(1)으로 대기 후 재시도
 * - CQ 완료 NULL: "Missing completion" 에러 카운트 증가
 * - NVMe 에러: stderr에 에러 문자열 출력, 에러 카운트 증가
 * - 시그널 수신 시 루프 즉시 탈출
 */
static void disk_w(const struct disk_info* disk, struct queue_pair* qp, FILE* fp, uint64_t blks, uint64_t offset, nvm_dma_t* wnd)
{
    /* DMA 윈도우 내 데이터 버퍼 시작 주소 (페이지 3부터) */
    void*           buf_ptr     = NVM_DMA_OFFSET(wnd, 3);
    const uint64_t* buf_ioaddrs = &wnd->ioaddrs[3];

    /* PRP 리스트: DMA 윈도우 페이지 2에 위치 */
    nvm_prp_list_t  prp_list    = NVM_PRP_LIST(wnd, 2);

    while (!signal_caught && !feof(fp) && !ferror(fp))
    {
        /* stdin에서 데이터 버퍼로 읽기 (최대 blks * block_size 바이트) */
        size_t is = fread(buf_ptr, 1, blks * disk->block_size, fp);
        /* 읽은 바이트를 블록 크기로 올림 정렬하여 실제 쓸 블록 수 계산 */
        size_t curr_blks = NVM_PAGE_ALIGN(is, disk->block_size) / disk->block_size;
        /* 페이지 크기로 올림 정렬 (NVMe 전송 단위) */
        size_t size = NVM_PAGE_ALIGN(curr_blks * disk->block_size, disk->page_size);

        /* SQ에 빈 슬롯이 나올 때까지 대기 */
        nvm_cmd_t* cmd;
        while (!signal_caught && (cmd = nvm_sq_enqueue(&qp->sq)) == NULL)
        {
            usleep(1);
        }

        if (signal_caught)
        {
            break;
        }

        /* NVMe Write 명령 구성 */
        nvm_cmd_header(cmd, NVM_DEFAULT_CID(&qp->sq), NVM_IO_WRITE, disk->ns_id);
        nvm_cmd_data(cmd, 1, &prp_list, size / disk->page_size, buf_ioaddrs);
        nvm_cmd_rw_blks(cmd, offset, curr_blks);

        /* CPU 캐시를 메모리로 flush (NVMe 컨트롤러가 DMA로 읽을 최신 데이터 보장) */
        nvm_cache_flush(buf_ptr, NVM_PAGE_ALIGN(curr_blks * disk->block_size, disk->page_size));

        /* SQ doorbell 링하여 컨트롤러에 새 명령 통보 */
        nvm_sq_submit(&qp->sq);
        qp->cmds++;

        /* CQ에서 완료 대기 (50ms 타임아웃) */
        nvm_cpl_t* cpl;
        while (!signal_caught && (cpl = nvm_cq_dequeue_block(&qp->cq, 50)) == NULL);

        if (signal_caught)
        {
            break;
        }

        if (cpl == NULL)
        {
            fprintf(stderr, "Missing completion!\n");
            qp->errors++;
            continue;
        }

        nvm_sq_update(&qp->sq);

        /* NVMe 완료 상태 확인 */
        if (!NVM_ERR_OK(cpl))
        {
            fprintf(stderr, "Agh! %s\n", nvm_strerror(NVM_ERR_STATUS(cpl)));
            qp->errors++;
        }

        nvm_cq_update(&qp->cq);
        qp->cpls++;

        /* 다음 쓰기 위치로 LBA 오프셋 전진 */
        offset += curr_blks;
    }

    /* 모든 쓰기 완료 후 NVMe Flush 명령 제출 (volatile write cache -> NVM 미디어) */
    nvm_cmd_t* cmd = nvm_sq_enqueue(&qp->sq);
    if (cmd == NULL)
    {
        qp->errors++;
        return;
    }

    nvm_cmd_header(cmd, NVM_DEFAULT_CID(&qp->sq), NVM_IO_FLUSH, disk->ns_id);
    nvm_cmd_data_ptr(cmd, 0, 0);  /* Flush는 데이터 전송이 없으므로 PRP=0 */
    nvm_sq_submit(&qp->sq);
}


/*
 * disk_r - NVMe 디스크에서 블록을 읽어 stdout으로 출력
 *
 * @disk:   디스크 정보
 * @qp:     I/O 큐 쌍
 * @blks:   한 번에 읽을 최대 블록 수
 * @count:  총 읽을 블록 수
 * @offset: 시작 LBA
 * @wnd:    DMA 윈도우
 *
 * [동작 흐름]
 * disk_w()와 유사하지만 방향이 반대:
 * 1. NVMe Read 명령 구성 및 SQ 제출
 * 2. CQ 완료 대기
 * 3. nvm_cache_invalidate()로 CPU 캐시 무효화 (DMA로 기록된 최신 데이터 보장)
 * 4. fwrite()로 DMA 버퍼의 데이터를 stdout에 출력
 * 5. count가 0이 될 때까지 반복
 */
static void disk_r(const struct disk_info* disk, struct queue_pair* qp, uint64_t blks, uint64_t count, uint64_t offset, nvm_dma_t* wnd)
{
    void*           buf_ptr     = NVM_DMA_OFFSET(wnd, 3);
    const uint64_t* buf_ioaddrs = &wnd->ioaddrs[3];

    nvm_prp_list_t  prp_list    = NVM_PRP_LIST(wnd, 2);

    while (!signal_caught && count != 0)
    {
        /* 남은 블록 수와 버퍼 크기 중 작은 값을 이번 전송량으로 결정 */
        size_t curr_blks = MIN(blks, count);
        size_t size = NVM_PAGE_ALIGN(curr_blks * disk->block_size, disk->page_size);

        /* SQ 슬롯 확보 (busy-wait with usleep) */
        nvm_cmd_t* cmd;
        while (!signal_caught && (cmd = nvm_sq_enqueue(&qp->sq)) == NULL)
        {
            usleep(1);
        }

        /* NVMe Read 명령 구성 */
        nvm_cmd_header(cmd, NVM_DEFAULT_CID(&qp->sq), NVM_IO_READ, disk->ns_id);
        nvm_cmd_data(cmd, 1, &prp_list, size / disk->page_size, buf_ioaddrs);
        nvm_cmd_rw_blks(cmd, offset, curr_blks);

        /* Read 전 캐시 flush: 버퍼 영역의 stale 데이터를 무효화 준비 */
        nvm_cache_flush(buf_ptr, size);

        nvm_sq_submit(&qp->sq);
        qp->cmds++;

        /* CQ 완료 대기 (50ms 타임아웃) */
        nvm_cpl_t* cpl;
        while (!signal_caught && (cpl = nvm_cq_dequeue_block(&qp->cq, 50)) == NULL);

        if (signal_caught)
        {
            break;
        }

        if (cpl == NULL)
        {
            fprintf(stderr, "Missing completion!\n");
            qp->errors++;
            continue;
        }

        nvm_sq_update(&qp->sq);

        if (!NVM_ERR_OK(cpl))
        {
            fprintf(stderr, "Agh! %s\n", nvm_strerror(NVM_ERR_STATUS(cpl)));
            qp->errors++;
        }

        nvm_cq_update(&qp->cq);
        qp->cpls++;

        /* CPU 캐시 무효화: NVMe 컨트롤러가 DMA로 기록한 데이터를 CPU가 최신으로 읽도록 보장 */
        nvm_cache_invalidate(buf_ptr, NVM_PAGE_ALIGN(curr_blks * disk->block_size, disk->page_size));

        /* 읽은 데이터를 stdout으로 출력 */
        fwrite(buf_ptr, 1, curr_blks * disk->block_size, stdout);

        offset += curr_blks;
        count -= curr_blks;
    }
}


/*
 * disk_rw - 디스크 Read 또는 Write 실행 및 결과 검증
 *
 * @disk:   디스크 정보
 * @qp:     I/O 큐 쌍
 * @args:   커맨드라인 옵션
 * @window: DMA 윈도우
 * @return: 성공 시 0, 명령/완료 수 불일치 시 EPIPE, 에러 발생 시 EIO
 *
 * args->input이 NULL이 아니면 쓰기 모드 (disk_w), NULL이면 읽기 모드 (disk_r).
 * 완료 후 cmds==cpls 확인 및 에러 수 체크.
 */
static int disk_rw(const struct disk_info* disk, struct queue_pair* qp, const struct arguments* args, nvm_dma_t* window)
{
    int status = 0;
    qp->cmds = 0;
    qp->cpls = 0;
    qp->errors = 0;

    if (args->input != NULL)
    {
        fprintf(stderr, "buffer size=%zu blks\n", args->blocks);
        disk_w(disk, qp, args->input, args->blocks, args->offset, window);
    }
    else
    {
        fprintf(stderr, "buffer size=%zu blks, count=%zu\n", args->blocks, args->count);
        disk_r(disk, qp, args->blocks, args->count, args->offset, window);
    }

    /* 제출한 명령 수와 완료된 명령 수가 다르면 에러 */
    if (qp->cmds != qp->cpls)
    {
        fprintf(stderr, "Agh! cmds=%zu cpls=%zu\n", qp->cmds, qp->cpls);
        status = EPIPE;
    }

    if (qp->errors > 0)
    {
        fprintf(stderr, "Owf!! errors: %zu\n", qp->errors);
        status = EIO;
    }

    return status;
}


/*
 * get_disk_info - Identify Controller/Namespace로 디스크 정보 획득
 *
 * @rpc:      Admin Queue 참조 (RPC 바인딩 또는 로컬 AQ)
 * @ns_id:    NVMe 네임스페이스 ID
 * @seg_id:   임시 SISCI 세그먼트 ID
 * @adapter:  DIS 어댑터 번호
 * @disk:     결과를 저장할 disk_info 구조체
 * @return:   성공 시 0, 실패 시 에러 코드
 *
 * 임시 4KB 세그먼트를 할당하여 Identify Controller -> Identify Namespace를
 * 순차 실행하고, 페이지 크기/MDTS/블록 크기/총 블록 수를 disk 구조체에 저장.
 */
static int get_disk_info(nvm_aq_ref rpc, uint32_t ns_id, uint32_t seg_id, uint32_t adapter, struct disk_info* disk)
{
    struct segment page;
    nvm_dma_t* dma;

    int status = segment_create(&page, seg_id, 0x1000);
    if (status != 0)
    {
        fprintf(stderr, "Failed to info segment\n");
        return status;
    }

    status = dma_create(&dma, nvm_ctrl_from_aq_ref(rpc), &page, adapter);
    if (status != 0)
    {
        segment_remove(&page);
        fprintf(stderr, "Failed to create DMA window\n");
        return status;
    }

    /* Identify Controller: 페이지 크기, 최대 데이터 전송 크기(MDTS) 등 획득 */
    struct nvm_ctrl_info ctrl_info;
    status = nvm_admin_ctrl_info(rpc, &ctrl_info, dma->vaddr, dma->ioaddrs[0]);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to identify controller: %s\n", nvm_strerror(status));
        goto leave;
    }

    /* Identify Namespace: 블록 크기, 총 블록 수 등 획득 */
    struct nvm_ns_info ns_info;
    status = nvm_admin_ns_info(rpc, &ns_info, ns_id, dma->vaddr, dma->ioaddrs[0]);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to identify namespace: %s\n", nvm_strerror(status));
        goto leave;
    }

    disk->ns_id = ns_info.ns_id;
    disk->page_size = ctrl_info.page_size;
    disk->chunk_size = ctrl_info.max_data_size;   /* MDTS: 한 명령으로 전송 가능한 최대 바이트 */
    disk->block_size = ns_info.lba_data_size;
    disk->max_blocks = ns_info.size;

leave:
    dma_remove(dma, &page, adapter);
    segment_remove(&page);
    return status;
}


/*
 * destroy_queues - I/O SQ/CQ 쌍 삭제
 *
 * @rpc: Admin Queue 참조 (Admin Delete Queue 명령 제출용)
 * @qp:  삭제할 큐 쌍
 *
 * NVMe 스펙상 SQ를 먼저 삭제한 후 CQ를 삭제해야 한다.
 */
static void destroy_queues(nvm_aq_ref rpc, struct queue_pair* qp)
{
    int err = nvm_admin_sq_delete(rpc, &qp->sq, &qp->cq);
    if (!nvm_ok(err))
    {
        fprintf(stderr, "Failed to delete SQ: %s\n", nvm_strerror(err));
    }

    err = nvm_admin_cq_delete(rpc, &qp->cq);
    if (!nvm_ok(err))
    {
        fprintf(stderr, "Failed to delete CQ: %s\n", nvm_strerror(err));
    }
}


/*
 * create_queues - I/O SQ/CQ 쌍 생성
 *
 * @rpc:      Admin Queue 참조
 * @queue_id: 큐 번호
 * @dma:      큐 메모리용 DMA 윈도우 (CQ: 오프셋 0, SQ: 오프셋 1)
 * @qp:       결과를 저장할 큐 쌍 구조체
 * @return:   성공 시 0, 실패 시 에러 코드
 *
 * DMA 윈도우의 페이지 0에 CQ, 페이지 1에 SQ를 배치한다.
 * NVM_CQ_SIZE/NVM_SQ_SIZE 매크로로 DMA 크기에 맞는 큐 깊이를 계산한다.
 */
static int create_queues(nvm_aq_ref rpc, uint16_t queue_id, nvm_dma_t* dma, struct queue_pair* qp)
{
    int status;

    /* CQ 생성: DMA 오프셋 0, 크기는 DMA 윈도우 1페이지에 들어가는 최대 엔트리 수 */
    status = nvm_admin_cq_create(rpc, &qp->cq, queue_id, dma, 0, NVM_CQ_SIZE(dma, 1));
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create IO completion queue (CQ): %s\n", nvm_strerror(status));
        return status;
    }

    /* SQ 생성: DMA 오프셋 1, CQ와 연결 */
    status = nvm_admin_sq_create(rpc, &qp->sq, &qp->cq, queue_id, dma, 1, NVM_SQ_SIZE(dma, 1));
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create IO submission queue (SQ): %s\n", nvm_strerror(status));
        return status;
    }

    return 0;
}


/*
 * prepare_buffer - I/O 데이터 버퍼 SISCI 세그먼트 및 DMA 윈도우 생성
 *
 * @rpc:     Admin Queue 참조 (컨트롤러 정보 접근용)
 * @args:    커맨드라인 옵션 (segment_id, adapter, blocks 등)
 * @disk:    디스크 정보 (페이지 크기, 블록 크기)
 * @segment: 결과 세그먼트 구조체
 * @dma:     결과 DMA 윈도우 포인터
 * @return:  성공 시 0, 실패 시 에러 코드
 *
 * 버퍼 크기 = 3페이지(CQ+SQ+PRP) + 데이터 영역(blocks * block_size, 정렬)
 * 할당 후 0으로 초기화하고 캐시 flush한다.
 */
static int prepare_buffer(nvm_aq_ref rpc,
                          const struct arguments* args,
                          const struct disk_info* disk,
                          struct segment* segment,
                          nvm_dma_t** dma)
{
    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(rpc);
    /* 총 크기: 3페이지(CQ, SQ, PRP 리스트) + 데이터 버퍼(컨트롤러 정렬) */
    size_t size = 3 * disk->page_size + NVM_CTRL_ALIGN(ctrl, disk->block_size * args->blocks);

    int status = segment_create(segment, args->segment_id, size);
    if (status != 0)
    {
        fprintf(stderr, "Failed to create buffer segment\n");
        return status;
    }

    status = dma_create(dma, ctrl, segment, args->adapter);
    if (status != 0)
    {
        segment_remove(segment);
        fprintf(stderr, "Failed to create DMA window\n");
        return status;
    }

    /* 버퍼 전체를 0으로 초기화하고 캐시 flush (DMA 일관성 보장) */
    memset((*dma)->vaddr, 0, size);
    nvm_cache_flush((*dma)->vaddr,  size);

    return status;
}



/*
 * main - RPC dd 도구 메인 함수
 *
 * [실행 흐름]
 * 1. 커맨드라인 옵션 파싱
 * 2. SISCI 초기화, 시그널 핸들러 등록 (SIGTERM/SIGINT/SIGPIPE)
 * 3. NVMe 컨트롤러 핸들 획득
 * 4. 서버 모드 vs 클라이언트 모드:
 *    서버 모드:
 *      a. Admin Queue용 세그먼트/DMA 생성
 *      b. nvm_aq_create()로 Admin Queue 생성 (컨트롤러 리셋)
 *      c. nvm_dis_rpc_enable()으로 RPC 서비스 활성화
 *      d. Set Features(Number of Queues)로 큐 개수 예약
 *    클라이언트 모드:
 *      a. nvm_dis_rpc_bind()로 외부 rpc_server에 바인딩
 * 5. get_disk_info()로 Identify Controller/Namespace 실행
 * 6. 버퍼 크기 및 읽기 범위 유효성 검증
 *    - blocks * block_size가 MDTS를 초과하면 에러
 *    - offset + count가 총 블록 수를 초과하면 에러
 * 7. prepare_buffer()로 I/O 버퍼 세그먼트/DMA 생성
 * 8. create_queues()로 I/O SQ/CQ 쌍 생성
 * 9. disk_rw()로 실제 Read 또는 Write 실행
 * 10. 리소스 역순 정리
 */
int main(int argc, char** argv)
{
    nvm_ctrl_t* ctrl;
    nvm_aq_ref rpc;
    struct segment segment;
    nvm_dma_t* dma_window;
    struct segment aq_mem;
    nvm_dma_t* aq_window;

    struct arguments args;
    parse_args(argc, argv, &args);

    sci_error_t err;
    SCIInitialize(0, &err);
    if (err != SCI_ERR_OK)
    {
        fprintf(stderr, "Failed to initialize SISCI: %s\n", SCIGetErrorString(err));
        exit(1);
    }

    /* 시그널 핸들러 등록 (안전한 I/O 중단용) */
    signal(SIGTERM, (sig_t) catch_signal);
    signal(SIGINT, (sig_t) catch_signal);
    signal(SIGPIPE, (sig_t) catch_signal);

    /* NVMe 컨트롤러 핸들 획득 (SmartIO FDID 기반) */
    // Get controller reference
    int status = nvm_dis_ctrl_init(&ctrl, args.controller_id);
    if (status != 0)
    {
        fprintf(stderr, "Failed to get controller reference: %s\n", strerror(status));
        exit(2);
    }

    if (args.server)
    {
        /* 서버 모드: 자체적으로 Admin Queue를 생성하여 컨트롤러 매니저 역할 수행 */
        // Become RPC server
        status = segment_create(&aq_mem, args.segment_id + 1, 2 * ctrl->page_size);
        if (status != 0)
        {
            nvm_ctrl_free(ctrl);
            SCITerminate();
            fprintf(stderr, "Failed to become RPC server: %s\n", strerror(status));
            exit(2);
        }

        status = dma_create(&aq_window, ctrl, &aq_mem, args.adapter);
        if (status != 0)
        {
            segment_remove(&aq_mem);
            nvm_ctrl_free(ctrl);
            SCITerminate();
            fprintf(stderr, "Failed to become RPC server: %s\n", strerror(status));
            exit(2);
        }

        /* Admin Queue 메모리를 0으로 초기화 (ASQ/ACQ) */
        memset(aq_window->vaddr, 0, 2 * aq_window->page_size);

        /* Admin Queue 생성: 컨트롤러 리셋 -> AQA/ASQ/ACQ 설정 -> CC.EN=1 */
        status = nvm_aq_create(&rpc, ctrl, aq_window);
        if (status != 0)
        {
            dma_remove(aq_window, &aq_mem, args.adapter);
            segment_remove(&aq_mem);
            nvm_ctrl_free(ctrl);
            SCITerminate();
            fprintf(stderr, "Failed to become RPC server: %s\n", strerror(status));
            exit(2);
        }

        /* 이 어댑터에서 RPC 서비스 활성화 (다른 프로세스가 Admin 명령을 위임 가능) */
        status = nvm_dis_rpc_enable(rpc, args.adapter, NULL);
        if (status != 0)
        {
            fprintf(stderr, "Ouch! %s\n", nvm_strerror(status));
            goto leave;
        }

        /* I/O 큐 개수 예약 (Set Features: Number of Queues) */
        uint16_t n_cqs = args.num_queues;
        uint16_t n_sqs = args.num_queues;
        status = nvm_admin_request_num_queues(rpc, &n_cqs, &n_sqs);
        if (!nvm_ok(status))
        {
            fprintf(stderr, "Ack!! %s\n", nvm_strerror(status));
            goto leave;
        }

        if (n_sqs < args.num_queues || n_cqs < args.num_queues)
        {
            fprintf(stderr, "Failed to reserve SQ/CQ queue-pairs\n");
            goto leave;
        }
    }
    else
    {
        /* 클라이언트 모드: 외부 rpc_server에 RPC 바인딩 */
        // Get RPC reference
        status = nvm_dis_rpc_bind(&rpc, ctrl, args.adapter);
        if (status != 0)
        {
            nvm_ctrl_free(ctrl);
            SCITerminate();
            fprintf(stderr, "Failed to get RPC reference: %s\n", strerror(status));
            exit(2);
        }
    }

    /* Identify Controller/Namespace로 디스크 정보 획득 */
    // Get information about the disk
    struct disk_info disk;
    status = get_disk_info(rpc, args.nvm_namespace, args.segment_id, args.adapter, &disk);
    if (status != 0)
    {
        goto leave;
    }

    /* 버퍼 크기 미지정 시 MDTS / block_size로 자동 설정 (최대 전송 가능 크기) */
    if (args.blocks == 0)
    {
        args.blocks = disk.chunk_size / disk.block_size;
    }

    /* 버퍼 크기가 MDTS를 초과하는지 검증 */
    // Check if buffer size is sensible
    if (disk.chunk_size < args.blocks * disk.block_size)
    {
        fprintf(stderr, "Buffer/chunk size exceeds controller's maximum data transfer size\n");
        status = 3;
        goto leave;
    }

    /* 읽기 범위가 디스크 크기를 초과하는지 검증 */
    // Check if read size is sensible
    if (args.offset + args.count > disk.max_blocks)
    {
        fprintf(stderr, "Trying to read larger than disk size\n");
        status = 3;
        goto leave;
    }

    /* I/O 버퍼 세그먼트 및 DMA 윈도우 생성 */
    // Create buffer and set up IO queues
    status = prepare_buffer(rpc, &args, &disk, &segment, &dma_window);
    if (status != 0)
    {
        goto leave;
    }

    /* I/O SQ/CQ 쌍 생성 */
    struct queue_pair queues;
    status = create_queues(rpc, args.queue_id, dma_window, &queues);
    if (status != 0)
    {
        goto remove;
    }

    /* 실제 디스크 Read 또는 Write 실행 */
    // Do the cool stuff
    status = disk_rw(&disk, &queues, &args, dma_window);

    destroy_queues(rpc, &queues);

remove:
    dma_remove(dma_window, &segment, args.adapter);
    segment_remove(&segment);

    /* 서버 모드일 때만 RPC 서비스 비활성화 */
    if (args.server)
    {
        nvm_dis_rpc_disable(rpc, args.adapter);
    }

leave:
    //nvm_rpc_unbind(rpc);
    nvm_aq_destroy(rpc);

    /* 서버 모드일 때 Admin Queue 리소스 정리 */
    if (args.server)
    {
        dma_remove(aq_window, &aq_mem, args.adapter);
        segment_remove(&aq_mem);
    }

    nvm_ctrl_free(ctrl);
    SCITerminate();
    fprintf(stderr, "Exit status: %s\n", strerror(status));
    exit(status);
}


static void show_help(const char* progname)
{
    give_usage(progname);
    fprintf(stderr,
            "\nRead or write blocks from disk using a remote controller manager in a DIS cluster.\n\n"
            "    --ctrl         <fdid>      SmartIO device identifier (fabric device id).\n"
            "    --adapter      <adapter>   Local adapter to reach device (default is 0).\n"
            "    --namespace    <ns id>     NVM namespace identifier (default is 1).\n"
            "    --ns           <ns id>     Short-hand for --namespace.\n"
            "    --segment-id   <seg id>    Local segment identifier (default is random).\n"
            "    --queue-id     <number>    SQ/CQ queue pair number.\n"
            "    --queue-num    <number>    Alternative to --queue-id.\n"
            "    --qn           <number>    Short-hand for --queue-id.\n"
            "    --offset       <count>     Number of blocks to offset (default is 0).\n"
            "    --buffer       <count>     Buffer size in number of blocks (default is max).\n"
            "    --count        <count>     Total number of blocks to read.\n"
            "    --blocks       <count>     Alternative to --count.\n"
            "    --input                    Read from stdin and write to disk.\n"
            "    --num-queues   <number>    Number of queue-pairs to reserve.\n"
            "    --nq           <number>    Short-hand for --num-queues.\n"
            "    --server                   Act as RPC server.\n"
            "    --help                     Show this information.\n"
            "\n");
}


/*
 * parse_args - 커맨드라인 옵션 파싱
 *
 * @argc: 인자 개수
 * @argv: 인자 배열
 * @args: 결과 저장 구조체
 *
 * [필수 옵션]
 * --ctrl <fdid>: NVMe 컨트롤러 SmartIO FDID (16진수)
 * --qn <number>: I/O 큐 번호 (1부터 시작)
 * --count <number> 또는 --input: 둘 중 하나 필수 (동시 사용 불가)
 *
 * [서버 모드 추가 필수 옵션]
 * --server: 서버 모드 활성화
 * --nq <number>: 예약할 큐 쌍 개수 (queue_id 이상이어야 함)
 *
 * [유효성 검증]
 * - --ctrl 미지정 시 에러
 * - --count와 --input 동시 지정 시 에러
 * - --qn 미지정 시 에러
 * - --server 지정 시 --nq 필수, nq >= queue_id 필수
 */
static void parse_args(int argc, char** argv, struct arguments* args)
{
    static struct option opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"ctrl", required_argument, NULL, 'd'},
        {"adapter", required_argument, NULL, 'a'},
        {"namespace", required_argument, NULL, 1},
        {"nvm-ns", required_argument, NULL, 1},
        {"ns", required_argument, NULL, 1},
        {"segment-id", required_argument, NULL, 0},
        {"queue-id", required_argument, NULL, 'q'},
        {"queue-num", required_argument, NULL, 'q'},
        {"qn", required_argument, NULL, 'q'},
        {"offset", required_argument, NULL, 'o'},
        {"buffer", required_argument, NULL, 'b'},
        {"blocks", required_argument, NULL, 'c'},
        {"count", required_argument, NULL, 'c'},
        {"num-queues", required_argument, NULL, 'n'},
        {"nq", required_argument, NULL, 'n'},
        {"server", no_argument, NULL, 's'},
        {"input", no_argument, NULL, 'i'},
        {NULL, 0, NULL, 0}
    };

    /* 기본값 초기화 */
    args->segment_id = 345;
    args->adapter = 0;
    args->queue_id = 0;
    args->controller_id = 0;
    args->nvm_namespace = 1;
    args->offset = 0;
    args->blocks = 0;
    args->count = 0;
    args->input = NULL;
    args->server = false;
    args->num_queues = 0;

    int opt;
    int idx;

    while ((opt = getopt_long(argc, argv, ":hd:a:n:sq:o:b:c:i", opts, &idx)) != -1)
    {
        switch (opt)
        {
            case '?': // unknown option
                fprintf(stderr, "Unknown option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit('?');

            case ':': // missing option argument
                fprintf(stderr, "Missing argument for option: `%s'\n", argv[optind - 1]);
                give_usage(argv[0]);
                exit(':');

            case 'h': // show help
                show_help(argv[0]);
                exit(0);

            case 'd': // device identifier (16진수 SmartIO FDID)
                if (parse_u64(optarg, &args->controller_id, 16) != 0)
                {
                    fprintf(stderr, "Invalid device id: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('c');
                }
                break;

            case 'a': // device adapter (10진수)
                if (parse_u32(optarg, &args->adapter, 10) != 0)
                {
                    fprintf(stderr, "Invalid adapter number: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('a');
                }
                break;

            case 1: // NVM namespace
                if (parse_u32(optarg, &args->nvm_namespace, 0) != 0 || args->nvm_namespace == 0)
                {
                    fprintf(stderr, "Invalid NVM namespace: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('n');
                }
                break;

            case 0: // Segment identifier
                if (parse_u32(optarg, &args->segment_id, 0) != 0)
                {
                    fprintf(stderr, "Invalid segment id: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('s');
                }
                break;

            case 'q': // Queue number (I/O 큐 쌍 번호, 1부터 시작)
                if (parse_u16(optarg, &args->queue_id, 0) != 0 || args->queue_id == 0)
                {
                    fprintf(stderr, "Invalid SQ/CQ queue-pair id: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('q');
                }
                break;

            case 'o': // Block offset (시작 LBA)
                if (parse_u64(optarg, &args->offset, 0) != 0)
                {
                    fprintf(stderr, "Invalid block count: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('b');
                }
                break;

            case 'b': // Buffer size (한 번에 전송할 블록 수)
                if (parse_u64(optarg, &args->blocks, 0) != 0 || args->blocks == 0)
                {
                    fprintf(stderr, "Invalid block count: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('b');
                }
                break;

            case 'c': // Loop count (총 전송할 블록 수)
                if (parse_u64(optarg, &args->count, 0) != 0 || args->count == 0)
                {
                    fprintf(stderr, "Invalid count: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('b');
                }
                break;

            case 'n': // Number of queues (서버 모드용)
                if (parse_u16(optarg, &args->num_queues, 0) != 0 || args->num_queues == 0)
                {
                    fprintf(stderr, "Invalid number of SQ/CQ queue-pairs: %s\n", optarg);
                    give_usage(argv[0]);
                    exit('q');
                }
                break;

            case 's': // 서버 모드 활성화
                args->server = true;
                break;

            case 'i': // 입력 모드 (stdin에서 읽어 디스크에 쓰기)
                args->input = stdin;
                break;
        }
    }

    /* 필수 옵션 검증 */
    if (args->controller_id == 0)
    {
        fprintf(stderr, "No controller specified!\n");
        give_usage(argv[0]);
        exit('c');
    }

    /* --count와 --input 중 하나는 반드시 지정해야 함 */
    if (args->input == NULL && args->count == 0)
    {
        fprintf(stderr, "Either count or input must be specified\n");
        give_usage(argv[0]);
        exit('c');
    }
    else if (args->input != NULL && args->count != 0)
    {
        fprintf(stderr, "Only one of count or input can be specified\n");
        give_usage(argv[0]);
        exit('i');
    }

    /* I/O 큐 번호 필수 */
    if (args->queue_id == 0)
    {
        fprintf(stderr, "Must specify SQ/CQ queue-pair id\n");
        give_usage(argv[0]);
        exit('q');
    }

    /* 서버 모드 시 --num-queues 필수 */
    if (args->server && args->num_queues == 0)
    {
        fprintf(stderr, "Option --server requires option --num-queues (or --nq)\n");
        give_usage(argv[0]);
        exit('n');
    }

    /* 서버 모드 시 queue_id가 num_queues 이하여야 함 */
    if (args->server && args->num_queues < args->queue_id)
    {
        fprintf(stderr, "SQ/CQ queue-pair id must be lower than total number of queue-pairs\n");
        give_usage(argv[0]);
        exit('n');
    }
}
