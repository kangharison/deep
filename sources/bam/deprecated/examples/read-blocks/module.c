/*
 * module.c - NVMe 블록 읽기 커널 모듈 백엔드 (deprecated)
 *
 * libnvm 커널 모듈(/dev/libnvmX)을 통해 NVMe 컨트롤러에 접근하여 블록을 읽는다.
 * nvm_ctrl_init() → nvm_dma_map_host() → Admin Queue 생성 → I/O Queue 생성 → Read/Write
 */
#include "args.h"           // struct options: 커맨드라인 옵션 구조체
#include "read.h"           // disk_info, queue_pair, get_disk_info(), create_queue_pair() 등
#include <nvm_types.h>      // nvm_ctrl_t: 컨트롤러 핸들, nvm_dma_t: DMA 윈도우, nvm_aq_ref: AQ 참조
#include <nvm_ctrl.h>       // nvm_ctrl_init(): fd로 컨트롤러 핸들 획득, nvm_ctrl_free(): 해제
#include <nvm_dma.h>        // nvm_dma_map_host(): 호스트 메모리를 IOMMU 매핑, nvm_dma_unmap(): 해제
#include <nvm_aq.h>         // nvm_aq_create(): Admin Queue 생성(컨트롤러 리셋 포함), nvm_aq_destroy(): 파괴
#include <nvm_error.h>      // nvm_ok(): 상태 코드 성공 여부 확인, nvm_strerror(): 에러 문자열
#include <nvm_util.h>       // NVM_SQ_PAGES(), NVM_DMA_OFFSET(), NVM_CTRL_ALIGN() 등 매크로
#include <stdio.h>          // fprintf(), stderr: 에러 출력
#include <stdbool.h>        // bool: 불리언 타입
#include <stdlib.h>         // posix_memalign(), free(): 정렬된 메모리 할당/해제
#include <fcntl.h>          // O_RDWR, O_NONBLOCK: 파일 열기 플래그
#include <unistd.h>         // close(): 파일 디스크립터 닫기
#include <errno.h>          // errno: 시스템 에러 코드
#include <string.h>         // strerror(): 에러 코드를 문자열로 변환



/*
 * prepare_and_read - DMA 버퍼/큐를 할당하고 NVMe Read(또는 Write후 Read) 실행
 *
 * @ref:  Admin Queue 참조
 * @disk: 디스크 정보 (MDTS, 블록 크기 등)
 * @args: 커맨드라인 옵션
 * @return: 성공 시 0, 실패 시 에러 코드
 *
 * [메모리 레이아웃]
 * buffer_ptr: 데이터 버퍼 (chunk_size * block_size 바이트)
 * queue_ptr: CQ(1페이지) + SQ(N페이지) + PRP 리스트(qs개 페이지)
 *
 * nvm_dma_map_host()로 커널 모듈이 IOMMU 매핑을 수행한다.
 */
static int prepare_and_read(nvm_aq_ref ref, const struct disk_info* disk, const struct options* args)
{
    int status = 0;

    const size_t qs = args->queue_size;     // SQ 깊이 (엔트리 수)
    void* buffer_ptr = NULL;    // 데이터 DMA 버퍼의 원시 포인터 (posix_memalign으로 할당)
    nvm_dma_t* buffer = NULL;   // 데이터 DMA 윈도우 핸들
    void* queue_ptr = NULL;     // CQ+SQ+PRP 메모리의 원시 포인터
    nvm_dma_t* sq_mem = NULL;   // SQ+PRP DMA 윈도우 핸들
    nvm_dma_t* cq_mem = NULL;   // CQ DMA 윈도우 핸들
    size_t n_prp_lists = qs;    // PRP 리스트 수 = SQ 깊이 (각 SQ 엔트리마다 하나의 PRP 리스트)
    struct queue_pair queues;   // I/O SQ/CQ 쌍 구조체

    // nvm_ctrl_from_aq_ref: AQ 참조에서 컨트롤러 핸들을 추출 (정렬 매크로에 필요)
    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);

    // 데이터 버퍼 크기 결정: chunk_size와 num_blocks 중 작은 값 × 블록 크기
    const size_t buffer_blocks = args->chunk_size <= args->num_blocks ? args->chunk_size : args->num_blocks;
    // posix_memalign: 페이지 크기로 정렬된 메모리 할당 (DMA에는 물리 페이지 경계 정렬 필요)
    // NVM_CTRL_ALIGN: 컨트롤러 페이지 크기의 배수로 올림 정렬
    status = posix_memalign(&buffer_ptr, disk->page_size, NVM_CTRL_ALIGN(ctrl, buffer_blocks * disk->block_size));
    if (status != 0)
    {
        fprintf(stderr, "Failed to allocate memory buffer: %s\n", strerror(status));
        goto leave;
    }

    // 큐 메모리 할당: SQ 페이지 + PRP 리스트 페이지 + CQ 1페이지 + 여분 1페이지
    status = posix_memalign(&queue_ptr, disk->page_size,
            NVM_SQ_PAGES(disk, qs) * disk->page_size + disk->page_size * (n_prp_lists + 2));
    if (status != 0)
    {
        fprintf(stderr, "Failed to allocate queue memory: %s\n", strerror(status));
        goto leave;
    }

    // SQ+PRP 메모리를 IOMMU DMA 매핑: queue_ptr의 두 번째 페이지부터 (첫 페이지는 CQ용)
    // NVM_PTR_OFFSET: 포인터를 (page_size × 1)만큼 전진
    status = nvm_dma_map_host(&sq_mem, ctrl, NVM_PTR_OFFSET(queue_ptr, disk->page_size, 1),
            NVM_SQ_PAGES(disk, qs) * disk->page_size + disk->page_size * (n_prp_lists + 1));
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to map memory for controller: %s\n", nvm_strerror(status));
        goto leave;
    }

    // CQ 메모리를 IOMMU DMA 매핑: queue_ptr의 첫 번째 페이지 (1페이지만)
    status = nvm_dma_map_host(&cq_mem, ctrl, queue_ptr, disk->page_size);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to map memory for controller: %s\n", nvm_strerror(status));
        goto leave;
    }

    // 데이터 버퍼를 IOMMU DMA 매핑: NVMe Read/Write 데이터가 이 버퍼를 통해 전송됨
    status = nvm_dma_map_host(&buffer, ctrl, buffer_ptr, buffer_blocks * disk->block_size);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to map memory for controller: %s\n", nvm_strerror(status));
        goto leave;
    }

    // Admin Create CQ/SQ 명령으로 I/O 큐 쌍 생성
    status = create_queue_pair(ref, &queues, cq_mem, sq_mem, qs);
    if (status != 0)
    {
        goto leave;
    }

    // --write 옵션이 설정된 경우: 파일에서 읽어 NVMe Write 수행 (이후 Read로 검증)
    if (args->input != NULL)
    {
        status = write_blocks(disk, &queues, buffer, args);
        if (status != 0)
        {
            goto leave;
        }
    }

    // NVMe Read 수행 후 데이터를 stdout/파일로 덤프
    status = read_and_dump(disk, &queues, buffer, args);

leave:
    // 리소스 역순 해제 (NULL이면 nvm_dma_unmap/free가 안전하게 무시)
    nvm_dma_unmap(buffer);      // 데이터 버퍼 DMA 매핑 해제
    nvm_dma_unmap(sq_mem);      // SQ+PRP DMA 매핑 해제
    nvm_dma_unmap(cq_mem);      // CQ DMA 매핑 해제
    free(buffer_ptr);           // 데이터 버퍼 메모리 해제
    free(queue_ptr);            // 큐 메모리 해제
    return status;
}



/*
 * main - 커널 모듈 경유 NVMe 블록 읽기/쓰기
 *
 * [실행 흐름]
 * 1. 커맨드라인 파싱
 * 2. /dev/libnvmXXX 열기 -> nvm_ctrl_init()
 * 3. Admin Queue용 3페이지 DMA 메모리 할당 + Admin Queue 생성
 * 4. Identify Controller/Namespace로 디스크 정보 획득
 * 5. prepare_and_read()로 I/O 버퍼/큐 할당 + Read/Write 실행
 * 6. 리소스 역순 해제
 */
int main(int argc, char** argv)
{
    int status;
    int fd;                     // /dev/libnvmX 파일 디스크립터

    struct disk_info disk;      // Identify로 획득한 디스크 정보

    nvm_ctrl_t* ctrl = NULL;    // NVMe 컨트롤러 핸들 (PCIe BAR0 매핑 포함)
    void* aq_ptr = NULL;        // Admin Queue DMA 메모리의 원시 포인터
    nvm_dma_t* aq_mem = NULL;   // Admin Queue DMA 윈도우 핸들
    nvm_aq_ref aq_ref = NULL;   // Admin Queue 참조 (Admin 명령 제출에 사용)

    struct options args;        // 커맨드라인 파싱 결과

    // 커맨드라인 인자 파싱 (필수 옵션 누락 시 내부에서 exit)
    parse_options(argc, argv, &args);

    // libnvm 커널 모듈이 생성한 캐릭터 디바이스 열기 (/dev/libnvmX)
    // O_RDWR: 읽기/쓰기, O_NONBLOCK: 비차단 모드 (ioctl 호출 시 필요)
    fd = open(args.controller_path, O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open file descriptor: %s\n", strerror(errno));
        exit(1);
    }

    // nvm_ctrl_init: fd를 통해 커널 모듈에 ioctl → PCIe BAR0 매핑 → 컨트롤러 핸들 생성
    status = nvm_ctrl_init(&ctrl, fd);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to initialize controller reference: %s\n", nvm_strerror(status));
        goto leave;
    }

    // Admin Queue용 DMA 메모리 할당: 3페이지 (ASQ 1페이지 + ACQ 1페이지 + Identify 데이터 1페이지)
    status = posix_memalign(&aq_ptr, ctrl->page_size, ctrl->page_size * 3);
    if (status != 0)
    {
        fprintf(stderr, "Failed to allocate queue memory: %s\n", strerror(status));
        goto leave;
    }

    // 3페이지 메모리를 IOMMU DMA 매핑 (커널 모듈이 물리 주소를 NVMe 컨트롤러에 전달)
    status = nvm_dma_map_host(&aq_mem, ctrl, aq_ptr, ctrl->page_size * 3);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to map memory for controller: %s\n", nvm_strerror(status));
        goto leave;
    }

    // Admin Queue 생성: 컨트롤러 리셋(CC.EN=0) → AQA/ASQ/ACQ 레지스터 설정 → CC.EN=1
    // aq_mem의 페이지 0=ACQ, 페이지 1=ASQ로 사용
    status = nvm_aq_create(&aq_ref, ctrl, aq_mem);
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to reset controller: %s\n", nvm_strerror(status));
        goto leave;
    }

    // Identify Controller/Namespace로 디스크 정보(MDTS, 블록 크기 등) 획득
    // NVM_DMA_OFFSET(aq_mem, 2): 3번째 페이지(인덱스 2)를 Identify 데이터 버퍼로 사용
    status = get_disk_info(aq_ref, &disk, args.namespace_id, NVM_DMA_OFFSET(aq_mem, 2), aq_mem->ioaddrs[2], args.identify);
    if (status != 0)
    {
        goto leave;
    }

    // I/O 버퍼와 큐를 할당하고 Read/Write 실행
    status = prepare_and_read(aq_ref, &disk, &args);

leave:
    // 입력 파일 닫기
    if (args.input != NULL)
    {
        fclose(args.input);
    }

    // 출력 파일 닫기 (fclose 전에 flush 메시지 출력)
    if (args.output != NULL)
    {
        fprintf(stderr, "Flushing output file...\n");
        fclose(args.output);
    }

    fprintf(stderr, "Done\n");

    // 리소스 역순 해제
    nvm_aq_destroy(aq_ref);     // Admin Queue 파괴 (컨트롤러에 Admin Delete Queue는 불필요)
    nvm_dma_unmap(aq_mem);      // Admin Queue DMA 매핑 해제
    free(aq_ptr);               // Admin Queue 메모리 해제
    nvm_ctrl_free(ctrl);        // 컨트롤러 핸들 해제 (PCIe BAR 언맵 포함)
    close(fd);                  // /dev/libnvmX 파일 디스크립터 닫기
    exit(status);
}
