/*
 * read.c - NVMe 블록 읽기/쓰기 핵심 로직 (deprecated)
 *
 * NVMe I/O 큐를 사용하여 블록 데이터를 읽고 쓰는 함수를 구현한다.
 * CQ 폴링 스레드와 SQ 제출 스레드가 분리된 비동기 I/O 구조.
 *
 * get_disk_info(): Identify Controller/Namespace로 MDTS, 블록 크기 등 조회
 * create_queue_pair(): Admin 명령으로 I/O CQ/SQ 생성
 * read_and_dump(): NVMe Read → stdout/파일 출력
 * write_blocks(): stdin/파일 → NVMe Write
 */
#include "read.h"           // disk_info, queue_pair 구조체 및 함수 선언
#include "args.h"           // struct options: 커맨드라인 옵션
#include <nvm_types.h>      // nvm_cmd_t: NVMe 명령 구조체, nvm_cpl_t: 완료 엔트리, nvm_dma_t: DMA 윈도우
#include <nvm_admin.h>      // nvm_admin_ctrl_info(), nvm_admin_ns_info(): Identify Admin 명령
#include <nvm_util.h>       // NVM_PAGE_TO_BLOCK(), NVM_SQ_PAGES() 등 유틸리티 매크로
#include <nvm_queue.h>      // nvm_sq_enqueue(), nvm_sq_submit(), nvm_cq_dequeue() 등 큐 조작 함수
#include <nvm_cmd.h>        // nvm_cmd_header(), nvm_cmd_data(), nvm_cmd_rw_blks(): NVMe 명령 구성
#include <nvm_error.h>      // nvm_ok(), nvm_strerror(), NVM_ERR_STATUS(): 에러 처리
#include <stdint.h>         // uint8_t, uint16_t, uint64_t: 고정 크기 정수
#include <stdbool.h>        // bool: 불리언 타입
#include <stdio.h>          // fprintf(), fwrite(), fflush(): 표준 I/O
#include <string.h>         // memset(), memcpy(): 메모리 조작
#include <errno.h>          // errno: 시스템 에러 코드
#include <pthread.h>        // pthread_create(), pthread_join(): CQ 폴링 스레드 관리
#include <unistd.h>         // usleep(): 마이크로초 단위 대기
#include <time.h>           // clock_gettime(), struct timespec: 성능 측정용 고해상도 타이머

// MIN 매크로: 두 값 중 작은 값을 반환 (청크 크기 계산 등에 사용)
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

/* timediff_us - 두 timespec 간의 차이를 마이크로초(μs)로 반환 */
uint64_t timediff_us(struct timespec* start, struct timespec* end) {
    // tv_sec(초)를 마이크로초로 변환하고, tv_nsec(나노초)를 마이크로초로 나눠서 합산
    return (uint64_t)(end->tv_sec - start->tv_sec) * 1000000 + (end->tv_nsec-start->tv_nsec) / 1000;
}

/* print_stats - 전송 시간과 처리량(MB/s)을 stderr에 출력 */
void print_stats(struct timespec* start, struct timespec* end, size_t bytes) {
        uint64_t diff = timediff_us(start, end);    // 경과 시간 (μs)
        // bytes/diff: 바이트/μs = MB/s (1MB = 10^6 바이트와 1μs = 10^-6초가 상쇄)
        fprintf(stderr, "Done in %lldus, %fMB/s\n", (unsigned long long) diff, (double)bytes/(double)diff);
}

/*
 * print_ctrl_info - Identify Controller 결과를 사람이 읽기 쉬운 형태로 포맷하여 출력
 *
 * NVMe Identify Controller 데이터 구조(4KB)에서 핵심 필드를 추출하여 출력한다.
 */
static void print_ctrl_info(FILE* fp, const struct nvm_ctrl_info* info)
{
    // PCI Vendor ID + Subsystem Vendor ID를 4바이트 배열로 추출
    unsigned char vendor[4];
    memcpy(vendor, &info->pci_vendor, sizeof(vendor));

    // NVMe 시리얼 번호: 스펙에서 20바이트 고정 길이, ASCII 문자열 (null 종료 없음)
    char serial[21];
    memset(serial, 0, 21);          // null 종료 보장
    memcpy(serial, info->serial_no, 20);

    // NVMe 모델 번호: 스펙에서 40바이트 고정 길이
    char model[41];
    memset(model, 0, 41);
    memcpy(model, info->model_no, 40);

    // NVMe 펌웨어 리비전: 스펙에서 8바이트 고정 길이
    char revision[9];
    memset(revision, 0, 9);
    memcpy(revision, info->firmware, 8);

    // 컨트롤러 정보를 포맷하여 출력
    fprintf(fp, "------------- Controller information -------------\n");
    fprintf(fp, "PCI Vendor ID           : %x %x\n", vendor[0], vendor[1]);             // PCI Vendor ID (예: 144d = Samsung)
    fprintf(fp, "PCI Subsystem Vendor ID : %x %x\n", vendor[2], vendor[3]);             // PCI Subsystem Vendor ID
    fprintf(fp, "NVM Express version     : %u.%u.%u\n",                                 // NVMe 스펙 버전 (major.minor.tertiary)
            info->nvme_version >> 16, (info->nvme_version >> 8) & 0xff, info->nvme_version & 0xff);
    fprintf(fp, "Controller page size    : %zu\n", info->page_size);                    // CC.MPS로 설정된 메모리 페이지 크기
    fprintf(fp, "Max queue entries       : %u\n", info->max_entries);                   // MQES+1: 한 큐의 최대 엔트리 수
    fprintf(fp, "Serial Number           : %s\n", serial);                               // SSD 시리얼 번호
    fprintf(fp, "Model Number            : %s\n", model);                                // SSD 모델명
    fprintf(fp, "Firmware revision       : %s\n", revision);                             // 현재 펌웨어 버전
    fprintf(fp, "Max data transfer size  : %zu\n", info->max_data_size);                // MDTS (바이트): 한 명령의 최대 전송 크기
    fprintf(fp, "Max outstanding commands: %zu\n", info->max_out_cmds);                 // 컨트롤러가 처리 가능한 동시 명령 수
    fprintf(fp, "Max number of namespaces: %zu\n", info->max_n_ns);                     // 지원하는 최대 네임스페이스 수
    fprintf(fp, "--------------------------------------------------\n");
}


/*
 * get_disk_info - Identify Controller/Namespace로 디스크 정보 획득
 *
 * @ref:    Admin Queue 참조 (Admin 명령을 이 AQ를 통해 제출)
 * @info:   결과를 저장할 disk_info 구조체
 * @ns_id:  조회할 네임스페이스 ID
 * @ptr:    Identify 데이터를 수신할 버퍼의 가상 주소 (CPU가 읽을 때 사용)
 * @ioaddr: 같은 버퍼의 IO 주소 (NVMe 컨트롤러가 DMA로 쓸 때 사용)
 * @show:   true이면 컨트롤러 정보도 stderr에 출력
 * @return: 성공 시 0, 실패 시 NVMe 에러 코드
 */
int get_disk_info(nvm_aq_ref ref, struct disk_info* info, uint32_t ns_id, void* ptr, uint64_t ioaddr, bool show)
{
    int status;
    struct nvm_ctrl_info ctrl_info;     // Identify Controller 결과를 파싱한 구조체
    struct nvm_ns_info ns_info;         // Identify Namespace 결과를 파싱한 구조체

    // NVMe Admin Identify Controller 명령 (opcode 0x06, CNS=01)
    // 컨트롤러가 DMA로 ptr(ioaddr)에 4KB Identify 데이터를 기록하고, libnvm이 ctrl_info로 파싱
    status = nvm_admin_ctrl_info(ref, &ctrl_info, ptr, ioaddr);
    if (!nvm_ok(status))    // NVMe 상태 코드가 성공이 아니면 에러 출력
    {
        fprintf(stderr, "Failed to identify controller: %s\n", nvm_strerror(status));
        return status;
    }

    // Identify Controller에서 얻은 페이지 크기와 MDTS를 disk_info에 저장
    info->page_size = ctrl_info.page_size;          // CC.MPS 기반 페이지 크기 (보통 4096)
    info->max_data_size = ctrl_info.max_data_size;  // MDTS 바이트 값

    // NVMe Admin Identify Namespace 명령 (opcode 0x06, CNS=00)
    // 지정한 ns_id의 네임스페이스 정보(LBA 크기, 총 블록 수 등)를 조회
    status = nvm_admin_ns_info(ref, &ns_info, ns_id, ptr, ioaddr);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed identify namespace: %s\n", nvm_strerror(status));
        return status;
    }

    info->ns_id = ns_info.ns_id;            // 실제 네임스페이스 ID
    info->block_size = ns_info.lba_data_size; // LBA 데이터 크기 (보통 512B 또는 4KB)

    if (show)   // --identify 옵션이 설정된 경우 컨트롤러 정보 출력
    {
        print_ctrl_info(stderr, &ctrl_info);
    }
    return 0;
}



/*
 * create_queue_pair - I/O CQ/SQ 쌍 생성
 *
 * @ref:    Admin Queue 참조 (Admin Create Queue 명령을 이 AQ를 통해 제출)
 * @qp:     결과를 저장할 queue_pair 구조체
 * @cq_mem: CQ 메모리용 DMA 윈도우 (물리 주소가 컨트롤러에 전달됨)
 * @sq_mem: SQ 메모리용 DMA 윈도우
 * @sqs:    SQ 깊이 (엔트리 수)
 * @return: 성공 시 0, 실패 시 NVMe 에러 코드
 *
 * 1. Set Features(Number of Queues)로 1:1 큐 쌍 예약
 * 2. CQ 생성 (Admin Opcode 0x05)
 * 3. SQ 생성 (Admin Opcode 0x01), CQ에 연결
 */
int create_queue_pair(nvm_aq_ref ref, struct queue_pair* qp, nvm_dma_t* cq_mem, nvm_dma_t* sq_mem, size_t sqs)
{
    int status;

    // Set Features(Feature ID=0x07, Number of Queues): I/O CQ 1개, SQ 1개를 컨트롤러에 예약 요청
    status = nvm_admin_set_num_queues(ref, 1, 1);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to set number of queues: %s\n", nvm_strerror(status));
        return status;
    }

    // CQ 메모리를 0으로 초기화 (phase bit 초기값 = 0, 컨트롤러는 1로 기록하여 새 완료를 표시)
    memset(cq_mem->vaddr, 0, cq_mem->page_size);
    // Admin Create I/O CQ (opcode 0x05): CQ ID=1, 오프셋 0, 크기는 NVM_CQ_SIZE 매크로로 계산
    status = nvm_admin_cq_create(ref, &qp->cq, 1, cq_mem, 0, NVM_CQ_SIZE(cq_mem, 1));
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create completion queue: %s\n", nvm_strerror(status));
        return status;
    }

    // SQ 메모리를 0으로 초기화 (주의: 크기가 cq_mem->page_size로 되어 있지만 sq_mem 영역 의도)
    memset(sq_mem->vaddr, 0, cq_mem->page_size);

    // Admin Create I/O SQ (opcode 0x01): SQ ID=1, CQ와 연결, 오프셋 0, 깊이=sqs
    status = nvm_admin_sq_create(ref, &qp->sq, &qp->cq, 1, sq_mem, 0, sqs);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to create submission queue: %s\n", nvm_strerror(status));
        return status;
    }

    // queue_pair 구조체의 나머지 필드 초기화
    qp->sq_mem = sq_mem;    // SQ DMA 윈도우 참조 저장
    qp->cq_mem = cq_mem;    // CQ DMA 윈도우 참조 저장
    qp->stop = false;       // 폴링 스레드 종료 플래그 초기화
    qp->num_cpls = 0;       // 완료 카운터 초기화
    return 0;
}



/*
 * consume_completions - CQ 완료 폴링 스레드 함수
 *
 * @qp: queue_pair 구조체 (CQ에서 완료 엔트리를 dequeue)
 * @return: NULL (pthread 규약)
 *
 * qp->stop이 true가 될 때까지 CQ를 폴링하여 완료된 명령을 처리한다.
 * 완료 시 SQ 헤드 포인터 갱신 -> CQ 도어벨 갱신 -> num_cpls 증가.
 */
static void* consume_completions(struct queue_pair* qp)
{
    nvm_cpl_t* cpl;         // CQ 완료 엔트리 포인터
    qp->stop = false;       // 종료 플래그 초기화
    qp->num_cpls = 0;       // 완료 카운터 초기화

    // 메인 스레드가 stop=true로 설정할 때까지 무한 폴링
    while (!qp->stop)
    {
        // CQ에서 완료 엔트리를 dequeue (100μs 타임아웃 블로킹)
        // phase bit가 기대값과 일치하는 엔트리가 없으면 NULL 반환
        if ((cpl = nvm_cq_dequeue_block(&qp->cq, 100)) == NULL)
        {
            continue;   // 타임아웃: 다시 폴링
        }
        // CQ 완료 엔트리에 기록된 SQ head 포인터로 SQ의 head를 갱신
        // 이를 통해 SQ에서 소비된 엔트리 슬롯이 재사용 가능해짐
        nvm_sq_update(&qp->sq);

        // NVMe 완료 상태 확인: 에러가 있으면 stderr에 출력
        if (!NVM_ERR_OK(cpl))
        {
            fprintf(stderr, "%s\n", nvm_strerror(NVM_ERR_STATUS(cpl)));
        }

        // CQ head 포인터를 다음으로 이동하고 CQ doorbell에 기록 (컨트롤러에 소비 완료 알림)
        nvm_cq_update(&qp->cq);
        qp->num_cpls++;     // 완료 카운터 증가 (메인 스레드가 이 값을 확인하여 동기화)
    }

    return NULL;
}



/*
 * dump_memory - DMA 버퍼의 데이터를 파일 또는 stdout에 출력
 *
 * @buffer: 데이터가 저장된 DMA 윈도우 (buffer->vaddr이 가상 주소)
 * @args:   옵션 (output 파일 포인터, ascii 모드)
 * @size:   출력할 바이트 수
 *
 * output이 지정되면 바이너리 fwrite, 아니면 hex 덤프(또는 ASCII 텍스트)로 stdout 출력.
 */
static void dump_memory(const nvm_dma_t* buffer, const struct options* args, size_t size)
{
    // 파일 출력 모드: 바이너리로 직접 기록
    if (args->output != NULL)
    {
        fwrite(buffer->vaddr, 1, size, args->output);   // DMA 버퍼의 가상 주소에서 size 바이트를 파일에 기록
        fflush(args->output);                            // 출력 버퍼를 즉시 플러시
        return;
    }

    // stdout 출력 모드: hex 또는 ASCII 덤프
    uint8_t* ptr = (uint8_t*) buffer->vaddr;    // DMA 버퍼를 바이트 배열로 캐스팅
    size_t byte = 0;
    while (byte < size)
    {
        fprintf(stdout, "%8lx: ", (long)byte);  // 현재 오프셋을 16진수로 출력 (hex 덤프 좌측 주소)
        // ASCII 모드: 한 줄에 128바이트(0x80), hex 모드: 한 줄에 32바이트(0x20)
        for (size_t n = byte + (args->ascii ? 0x80 : 0x20); byte < n; ++byte)
        {
            uint8_t value = ptr[byte];
            if (args->ascii)
            {
                // ASCII 인쇄 가능 범위(0x20~0x7E) 밖의 문자는 공백으로 치환
                if ( !(0x20 <= value && value <= 0x7e) )
                {
                    value = ' ';
                }
                fprintf(stdout, "%c", value);       // ASCII 문자로 출력
            }
            else
            {
                fprintf(stdout, " %02x", value);    // 2자리 16진수로 출력 (hex 덤프)
            }
        }
        fprintf(stdout, "\n");  // 한 줄 완료
    }
}


/*
 * rw_bytes - NVMe Read/Write 명령을 SQ에 제출하여 데이터 전송
 *
 * @disk:           디스크 정보 (MDTS, 페이지/블록 크기)
 * @qp:             SQ/CQ 쌍
 * @buffer:         데이터 DMA 버퍼 (I/O 데이터가 저장될 메모리)
 * @blk_offset:     현재 LBA 오프셋 (입출력 파라미터, 전송 후 갱신됨)
 * @size_remaining: 남은 전송 바이트 수 (입출력 파라미터, 전송 후 갱신됨)
 * @op:             NVMe opcode (NVM_IO_READ=0x02 또는 NVM_IO_WRITE=0x01)
 * @return:         이번 호출에서 SQ에 제출한 NVMe 명령 수
 *
 * MDTS 이하 단위로 분할하여 여러 NVMe 명령을 SQ에 enqueue한다.
 * 각 명령: 헤더 설정 -> PRP 리스트 설정 -> LBA 범위 설정 -> SQ 제출
 */
static size_t rw_bytes(const struct disk_info* disk, struct queue_pair* qp, const nvm_dma_t* buffer, uint64_t* blk_offset, size_t* size_remaining, uint8_t op)
{
    // Read blocks
    size_t page = 0;        // 현재 처리 중인 DMA 버퍼 내 페이지 인덱스
    size_t num_cmds = 0;    // 이번 호출에서 제출한 명령 수
    size_t num_pages = disk->max_data_size / disk->page_size;   // MDTS에 해당하는 페이지 수 (한 명령의 최대 전송 단위)
    // chunk_pages: 이번 청크에서 전송할 총 페이지 수 (버퍼 크기 또는 남은 데이터 중 작은 값)
    size_t chunk_pages = MIN(buffer->n_ioaddrs, NVM_PAGE_ALIGN(*size_remaining, disk->page_size) / disk->page_size);
    size_t offset = *blk_offset;    // 현재 시작 LBA
    nvm_prp_list_t list;            // PRP 리스트 디스크립터 (SQ 메모리 내 PRP 리스트 위치)

    // 청크 내 모든 페이지를 MDTS 단위로 분할하여 NVMe 명령 생성
    while (page < chunk_pages)
    {
        // 이번 명령에서 전송할 페이지 수: MDTS 이내이면서 버퍼 잔여 페이지 이내
        num_pages = MIN(buffer->n_ioaddrs - page, num_pages);

        nvm_cmd_t* cmd;
        // SQ에 빈 슬롯이 생길 때까지 대기 (SQ가 가득 차면 submit하고 재시도)
        while ((cmd = nvm_sq_enqueue(&qp->sq)) == NULL)
        {
            nvm_sq_submit(&qp->sq);     // doorbell을 링하여 컨트롤러에 처리 요청
            usleep(1);                   // 1μs 대기 후 재시도
        }

        // PRP 리스트 인덱스: SQ 엔트리 인덱스를 큐 크기로 mod하여 순환
        uint16_t prp_list = num_cmds % qp->sq.qs;
        // 이번 명령이 전송할 블록 수: 페이지 수를 LBA 블록 수로 변환
        size_t num_blocks = NVM_PAGE_TO_BLOCK(disk->page_size, disk->block_size, num_pages);
        // 이번 명령의 시작 LBA: 기본 오프셋 + 이미 처리한 페이지에 해당하는 블록 수
        size_t start_block = offset + NVM_PAGE_TO_BLOCK(disk->page_size, disk->block_size, page);

        // NVMe 명령 헤더 구성: CID(명령 ID), opcode(READ/WRITE), namespace ID
        nvm_cmd_header(cmd, NVM_DEFAULT_CID(&qp->sq), op, disk->ns_id);

        // PRP 리스트의 DMA 메모리 위치 계산: SQ 페이지 뒤의 PRP 리스트 영역에서 prp_list번째
        list = NVM_PRP_LIST(qp->sq_mem, NVM_SQ_PAGES(qp->sq_mem, qp->sq.qs) + prp_list);

        // NVMe 명령의 데이터 포인터(PRP1/PRP2) 설정: DMA 버퍼의 IO 주소를 PRP 리스트에 기록
        // 반환값: 실제 매핑된 페이지 수 (page 인덱스를 이만큼 전진)
        page += nvm_cmd_data(cmd, 1, &list, num_pages, &buffer->ioaddrs[page]);

        // NVMe 명령의 LBA 범위 설정: CDW10(시작 LBA), CDW12(블록 수)
        nvm_cmd_rw_blks(cmd, start_block, num_blocks);

        ++num_cmds;     // 제출 명령 수 증가
    }

    // 모든 명령을 SQ에 넣은 후 한 번에 doorbell 링 (배치 제출)
    nvm_sq_submit(&qp->sq);

    // 다음 호출을 위해 LBA 오프셋과 남은 바이트 수 갱신
    *blk_offset = offset + NVM_PAGE_TO_BLOCK(disk->page_size, disk->block_size, page);
    *size_remaining -= MIN(*size_remaining, chunk_pages * disk->page_size);
    return num_cmds;
}

/*
 * read_and_dump - NVMe Read 후 데이터를 stdout/파일에 덤프
 *
 * @disk:   디스크 정보 (MDTS, 블록/페이지 크기)
 * @qp:     SQ/CQ 쌍
 * @buffer: 데이터 DMA 버퍼
 * @args:   커맨드라인 옵션 (offset, num_blocks, output 등)
 * @return: 성공 시 0
 *
 * [동작]
 * 1. CQ 폴링 스레드 시작
 * 2. 청크 단위로 반복: rw_bytes(READ) -> 완료 대기 -> dump_memory
 * 3. 전체 블록 완료 후 폴링 스레드 종료
 * 각 청크의 전송 시간과 처리량(MB/s)을 출력한다.
 */
int read_and_dump(const struct disk_info* disk, struct queue_pair* qp, const nvm_dma_t* buffer, const struct options* args)
{
    int status;
    pthread_t completer;            // CQ 폴링 스레드 핸들
    struct timespec start, end;     // 각 청크의 시작/종료 시간

    // CQ 폴링 스레드 시작: consume_completions()가 별도 스레드에서 CQ를 계속 폴링
    status = pthread_create(&completer, NULL, (void *(*)(void*)) consume_completions, qp);
    if (status != 0)
    {
        fprintf(stderr, "Could not start completer thread\n");
        return status;
    }

    // PRP 리스트 영역을 0으로 초기화 (SQ 엔트리 뒤의 PRP 리스트 페이지들)
    size_t sq_pages = NVM_SQ_PAGES(qp->sq_mem, qp->sq.qs);    // SQ 엔트리가 차지하는 페이지 수
    memset(NVM_DMA_OFFSET(qp->sq_mem, sq_pages), 0, qp->sq_mem->page_size * (qp->sq_mem->n_ioaddrs - sq_pages));

    size_t num_cmds = 0;            // 누적 제출 명령 수
    uint64_t start_block = args->offset;    // 현재 읽기 시작 LBA
    size_t size_remaining = args->num_blocks * disk->block_size;    // 남은 읽기 바이트 수

    // 전체 데이터를 청크 단위로 반복 읽기
    while (size_remaining != 0)
    {
        // 진행 상황 출력: 이번 청크 크기와 지금까지 읽은 총 바이트
        fprintf(stderr, "Reading %zu bytes [%zu MB] (total=%zu)\n",
                buffer->n_ioaddrs * disk->page_size,
                (buffer->n_ioaddrs * disk->page_size) >> 20,
                args->num_blocks * disk->block_size - size_remaining);
        size_t remaining = size_remaining;  // 이번 청크 시작 시점의 남은 바이트 (전송량 계산용)

        clock_gettime(CLOCK_MONOTONIC, &start);     // 타이머 시작 (MONOTONIC: 시스템 시간 변경에 영향 없음)

        // NVMe Read 명령들을 SQ에 제출 (start_block과 size_remaining은 내부에서 갱신됨)
        num_cmds += rw_bytes(disk, qp, buffer, &start_block, &size_remaining, NVM_IO_READ);

        // 이번 청크의 모든 명령이 완료될 때까지 busy-wait
        while (qp->num_cpls < num_cmds)
        {
            usleep(1);  // 1μs 대기 후 다시 확인
        }
        clock_gettime(CLOCK_MONOTONIC, &end);       // 타이머 종료

        // 이번 청크의 전송 시간과 처리량 출력
        print_stats(&start, &end, remaining - size_remaining);

        // 읽은 데이터를 stdout/파일로 덤프
        dump_memory(buffer, args, remaining - size_remaining);
    }

    // 모든 읽기 완료 후 CQ 폴링 스레드 종료
    qp->stop = true;                    // 폴링 스레드에 종료 신호
    pthread_join(completer, NULL);      // 폴링 스레드가 종료할 때까지 대기

    return 0;
}



/*
 * write_blocks - 파일에서 읽어 NVMe Write 수행
 *
 * @disk:   디스크 정보 (MDTS, 블록/페이지 크기)
 * @qp:     SQ/CQ 쌍
 * @buffer: 데이터 DMA 버퍼
 * @args:   커맨드라인 옵션 (input 파일, offset, num_blocks)
 * @return: 성공 시 0
 *
 * [동작]
 * 1. CQ 폴링 스레드 시작
 * 2. 청크 단위로 반복: fread -> rw_bytes(WRITE) -> Flush 명령 -> 완료 대기
 * 3. NVMe Flush 명령으로 volatile write cache의 데이터를 NVM 미디어에 영속화
 */
int write_blocks(const struct disk_info* disk, struct queue_pair* qp, const nvm_dma_t* buffer, const struct options* args)
{
    int status;
    pthread_t completer;            // CQ 폴링 스레드 핸들
    struct timespec start, end;     // 각 청크의 시작/종료 시간

    // CQ 폴링 스레드 시작
    status = pthread_create(&completer, NULL, (void *(*)(void*)) consume_completions, qp);
    if (status != 0)
    {
        fprintf(stderr, "Could not start completer thread\n");
        return status;
    }

    // PRP 리스트 영역 초기화
    size_t sq_pages = NVM_SQ_PAGES(qp->sq_mem, qp->sq.qs);
    memset(NVM_DMA_OFFSET(qp->sq_mem, sq_pages), 0, qp->sq_mem->page_size * (qp->sq_mem->n_ioaddrs - sq_pages));

    size_t num_cmds = 0;            // 누적 제출 명령 수 (Write + Flush 포함)
    uint64_t start_block = args->offset;
    size_t size_remaining = args->num_blocks * disk->block_size;
    size_t file_size = 0;           // 파일에서 실제 읽은 바이트 수

    // 전체 데이터를 청크 단위로 반복 쓰기
    while (size_remaining != 0)
    {
        // 진행 상황 출력
        fprintf(stderr, "Writing %zu bytes [%zu MB] (total=%zu)\n",
                buffer->n_ioaddrs * disk->page_size,
                (buffer->n_ioaddrs * disk->page_size) >> 20,
                args->num_blocks * disk->block_size - size_remaining);

        // 입력 파일에서 데이터를 DMA 버퍼로 읽기
        if (!feof(args->input) && !ferror(args->input))
        {
            // fread: 파일에서 최대 (버퍼 페이지 수 × 페이지 크기) 바이트를 DMA 버퍼의 가상 주소로 읽기
            ssize_t ret = fread(buffer->vaddr, 1, buffer->n_ioaddrs * buffer->page_size, args->input);
            if (ret < 0)
            {
                ret = 0;    // 읽기 에러 시 0 바이트로 처리
            }
            file_size = (size_t) ret;
        }

        // 파일이 끝나거나 에러가 발생하면 경고 출력 (DMA 버퍼의 나머지 부분은 이전 데이터/0)
        if (feof(args->input) || ferror(args->input) || file_size < buffer->n_ioaddrs * buffer->page_size)
        {
            fprintf(stderr, "WARNING: End of file was reached (read %zu bytes, buffer is %zu)\n",
                    file_size, buffer->n_ioaddrs * buffer->page_size);
        }
        size_t remaining = size_remaining;

        clock_gettime(CLOCK_MONOTONIC, &start);

        // NVMe Write 명령들을 SQ에 제출
        num_cmds += rw_bytes(disk, qp, buffer, &start_block, &size_remaining, NVM_IO_WRITE);

        // NVMe Flush 명령 제출: volatile write cache → NVM 미디어로 영속화
        // Flush는 데이터를 전송하지 않고 opcode만 전달하는 명령
        nvm_cmd_t* cmd = nvm_sq_enqueue(&qp->sq);
        while (cmd == NULL)     // SQ가 가득 차면 submit 후 재시도
        {
            nvm_sq_submit(&qp->sq);
            usleep(1);
            cmd = nvm_sq_enqueue(&qp->sq);
        }

        // Flush 명령 구성: opcode=NVM_IO_FLUSH(0x00), 데이터 전송 없음
        nvm_cmd_header(cmd, NVM_DEFAULT_CID(&qp->sq), NVM_IO_FLUSH, disk->ns_id);
        nvm_cmd_data_ptr(cmd, 0, 0);   // PRP1=0, PRP2=0 (Flush는 데이터 버퍼 불필요)
        nvm_sq_submit(&qp->sq);        // doorbell 링
        ++num_cmds;                     // Flush도 명령 하나로 카운트

        // Write + Flush 명령 모두 완료될 때까지 busy-wait
        while (qp->num_cpls < num_cmds)
        {
            usleep(1);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        // 이번 청크의 전송 시간과 처리량 출력
        print_stats(&start, &end, remaining - size_remaining);
    }

    // 모든 쓰기 완료 후 CQ 폴링 스레드 종료
    qp->stop = true;
    pthread_join(completer, NULL);

    return 0;
}
