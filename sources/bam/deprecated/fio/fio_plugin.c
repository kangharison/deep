/*
 * fio_plugin.c - libnvm 기반 fio I/O 엔진 플러그인 (Deprecated)
 *
 * [파일 역할]
 * fio 벤치마크 도구의 커스텀 I/O 엔진으로, Dolphin Interconnect Solutions(DIS)의
 * SISCI API와 libnvm 라이브러리를 사용하여 NVMe SSD에 직접 I/O를 수행한다.
 * 커널 블록 레이어를 우회하고, 유저스페이스에서 NVMe Submission Queue(SQ)와
 * Completion Queue(CQ)를 직접 조작하여 read/write 명령을 제출한다.
 *
 * [Deprecated 배경]
 * BaM 프로젝트 초기에는 DIS SmartIO 클러스터 환경에서 원격 NVMe 접근을 위해
 * SISCI 기반 libnvm을 사용했으나, 이후 GPU-initiated I/O와 커널 모듈(libnvm.ko)
 * 기반 아키텍처로 전환되면서 이 fio 플러그인은 더 이상 사용되지 않는다.
 *
 * [fio I/O 엔진 플러그인 구조]
 * fio는 ioengine_ops 구조체를 통해 커스텀 I/O 엔진을 등록한다.
 * 주요 콜백:
 *   - setup (thread_setup): 스레드별 초기 검증
 *   - init (thread_init): NVMe 컨트롤러 연결, Admin Queue 생성, I/O Queue 생성
 *   - queue (fio_queue): NVMe read/write 명령 제출 및 완료 대기 (동기 방식)
 *   - cleanup (thread_cleanup): 큐 삭제, DMA 해제, 컨트롤러 해제
 *   - iomem_alloc/iomem_free: DIS DMA 메모리로 fio 데이터 버퍼 할당/해제
 *   - open_file/close_file/invalidate: 최소 구현 (NVMe 직접 접근이므로 파일 불필요)
 *
 * [NVMe 커맨드 제출 흐름]
 * 1. nvm_cmd_header()로 NVMe 명령 헤더 구성 (opcode: READ 또는 WRITE)
 * 2. nvm_cmd_data()로 PRP(Physical Region Page) 리스트 설정 (DMA 주소 매핑)
 * 3. nvm_cmd_rw_blks()로 시작 LBA와 블록 수 설정
 * 4. nvm_sq_enqueue()로 SQ tail에 명령 삽입
 * 5. nvm_sq_submit()으로 SQ doorbell 링 (컨트롤러에 통보)
 * 6. nvm_cq_dequeue()로 CQ에서 완료 폴링 (busy-wait)
 * 7. nvm_sq_update()/nvm_cq_update()로 큐 포인터 갱신
 */
#define _GNU_SOURCE         // GNU 확장 기능 활성화 (fio 내부에서 필요)
#define __DIS_CLUSTER__     // DIS 클러스터 코드 경로 활성화 (SISCI API 사용)

#ifndef _REENTRANT
#define _REENTRANT          // 스레드 안전 함수 사용 (fio는 멀티스레드 구조)
#endif

#include "config-host.h"    // fio 빌드 구성 헤더 (호스트 환경 매크로 정의)
#include "fio.h"            // fio 메인 헤더: thread_data, io_u, fio_file, ioengine_ops 등
#include "optgroup.h"       // fio 옵션 그룹: FIO_OPT_C_ENGINE, FIO_OPT_G_INVALID 등

#include <stdint.h>         // uint8_t, uint16_t, uint32_t: 고정 크기 정수
#include <stdbool.h>        // bool: 불리언 타입
#include <stdlib.h>         // calloc(), free(): 메모리 할당/해제
#include <errno.h>          // EINVAL, EBADF, ENOMEM: 에러 코드

#include <nvm_types.h>   /* libnvm 기본 타입 정의 (nvm_ctrl_t, nvm_cmd_t, nvm_cpl_t, nvm_queue_t 등) */
#include <nvm_dma.h>     /* DMA 메모리 관리: nvm_dis_dma_create()로 SISCI 세그먼트 DMA 생성, nvm_dma_unmap()로 해제 */
#include <nvm_util.h>    /* 유틸리티 매크로: NVM_CQ_PAGES(), NVM_SQ_PAGES(), NVM_PRP_LIST() 등 */
#include <nvm_error.h>   /* 에러 처리: nvm_ok()로 성공 확인, nvm_strerror()로 에러 문자열, NVM_ERR_STATUS()로 상태 추출 */
#include <nvm_admin.h>   /* NVMe Admin 명령: Identify(ctrl_info/ns_info), Create/Delete Queue, Set/Get Num Queues */
#include <nvm_aq.h>      /* Admin Queue 라이프사이클: nvm_aq_create()(리셋+AQ생성), nvm_aq_destroy()(AQ파괴) */
#include <nvm_rpc.h>     /* RPC를 통한 원격 Admin Queue 바인딩: nvm_dis_rpc_bind(), nvm_rpc_unbind() */
#include <nvm_cmd.h>     /* NVMe I/O 명령 구성: nvm_cmd_header(), nvm_cmd_data(), nvm_cmd_rw_blks() */
#include <nvm_queue.h>   /* SQ/CQ 조작: nvm_sq_enqueue/submit(), nvm_cq_dequeue(), nvm_sq_update/cq_update() */
#include <nvm_ctrl.h>    /* NVMe 컨트롤러 관리: nvm_dis_ctrl_init()(FDID로 초기화), nvm_ctrl_free()(해제) */
#include <sisci_api.h>   /* Dolphin SISCI API: SCIInitialize()(라이브러리 초기화), SCITerminate()(종료) */


/* SISCI 라이브러리 초기화 완료 여부를 추적하는 전역 플래그 */
// fio_init 생성자에서 true로 설정, fio_exit 소멸자에서 false로 복원
static bool _initialized = false;


/*
 * libnvmfio_options - fio 엔진 커스텀 옵션 구조체
 *
 * fio의 --ioengine=libnvm 사용 시 지정 가능한 옵션들.
 * fio의 옵션 파싱 프레임워크가 offsetof()를 통해 각 필드에 직접 값을 기록한다.
 *
 * @pad:           fio 내부 정렬용 패딩 (fio 프레임워크 요구사항)
 * @fdid:          SmartIO Fabric Device ID (NVMe 컨트롤러를 DIS 클러스터에서 식별하는 16진수 ID)
 * @reset:         1이면 컨트롤러를 리셋하고 이 프로세스가 Admin Queue 매니저(소유자)가 됨
 * @remote_queue:  1이면 SQ 메모리를 컨트롤러 근처 원격 노드에 할당 (NUMA 레이턴시 최적화)
 * @queue_id:      사용할 I/O SQ/CQ 쌍의 번호 (1부터 시작, 0은 Admin Queue 전용)
 * @adapter:       DIS 네트워크 어댑터 번호 (RPC 바인딩 시 사용, 기본 0)
 * @namespace:     NVMe 네임스페이스 ID (기본값 1, 대부분의 SSD는 NS 1개)
 */
struct libnvmfio_options
{
    void* pad;                  // fio 옵션 프레임워크 내부 정렬용 패딩
    uint32_t fdid;              // SmartIO Fabric Device ID (0이면 미지정 → 에러)
    unsigned int reset;         // 컨트롤러 리셋 플래그 (set이면 AQ 매니저가 됨)
    unsigned int remote_queue;  // 원격 큐 배치 플래그 (SQ 메모리를 컨트롤러 노드에 할당)
    unsigned int queue_id;      // I/O 큐 번호 (1~N, 0은 Admin Queue)
    unsigned int adapter;       // DIS 어댑터 번호 (RPC 통신 경로)
    unsigned int namespace;     // NVMe 네임스페이스 ID
};



/*
 * libnvmfio_thread - fio 스레드별 NVMe 리소스 컨텍스트
 *
 * fio의 각 I/O 스레드가 독립적으로 보유하는 NVMe 자원 모음.
 * td->io_ops_data에 저장되어 콜백 함수 간에 공유된다.
 *
 * @ctrl:       NVMe 컨트롤러 핸들 (PCIe BAR0 레지스터 MMIO 매핑 포함)
 * @ctrl_info:  Identify Controller로 얻은 컨트롤러 정보 (모델명, 페이지 크기, MDTS 등)
 * @ns_info:    Identify Namespace로 얻은 네임스페이스 정보 (LBA 크기, 총 블록 수 등)
 * @aq_mem:     Admin Queue용 DMA 메모리 (리셋 모드에서만 할당, RPC 모드에서는 NULL)
 * @aq_ref:     Admin Queue 참조 핸들 (로컬 AQ 또는 원격 RPC 바인딩)
 * @cq_mem:     Completion Queue용 DMA 메모리 (NVMe 컨트롤러가 CQE를 DMA로 기록)
 * @cq:         Completion Queue 구조체 (head/tail 포인터, doorbell MMIO 주소, phase bit)
 * @sq_mem:     Submission Queue + PRP 리스트용 DMA 메모리
 * @sq:         Submission Queue 구조체 (head/tail 포인터, doorbell MMIO 주소)
 * @io_mem:     실제 I/O 데이터가 담기는 DMA 버퍼 (fio의 orig_buffer로 사용)
 * @prp_lists:  각 SQ 엔트리에 대응하는 PRP 리스트 배열 (scatter-gather DMA 주소 목록)
 */
struct libnvmfio_thread
{
    nvm_ctrl_t* ctrl;   /* NVMe 컨트롤러 핸들 */
    struct nvm_ctrl_info ctrl_info;     // Identify Controller 결과
    struct nvm_ns_info ns_info;         // Identify Namespace 결과
    nvm_dma_t* aq_mem;  /* Admin Queue DMA 메모리 (리셋 모드 전용) */
    nvm_aq_ref aq_ref;     /* Admin Queue 참조 (로컬 AQ 또는 RPC 바인딩) */
    nvm_dma_t* cq_mem;   /* Completion Queue DMA 메모리 */
    nvm_queue_t cq;     /* Completion Queue 구조체 */
    nvm_dma_t* sq_mem;   /* Submission Queue + PRP 리스트 DMA 메모리 */
    nvm_queue_t sq;     /* Submission Queue 구조체 */
    nvm_dma_t* io_mem;   /* I/O 데이터 DMA 버퍼 (fio의 orig_buffer로 매핑) */
    nvm_prp_list_t* prp_lists;  // 각 SQ 엔트리에 대응하는 PRP 리스트 배열
};



/*
 * identify - NVMe Identify Controller/Namespace Admin 명령 실행
 *
 * @data: 스레드 컨텍스트. ctrl, aq_ref가 초기화되어 있어야 한다.
 * @return: 성공 시 0, 실패 시 에러 코드
 *
 * [동작 흐름]
 * 1. 컨트롤러 페이지 크기만큼 임시 DMA 메모리 할당 (Identify 결과 수신용, 4KB)
 * 2. nvm_admin_ctrl_info()로 Identify Controller 실행 -> ctrl_info에 결과 저장
 * 3. nvm_admin_ns_info()로 Identify Namespace(nsid=1) 실행 -> ns_info에 결과 저장
 * 4. 임시 DMA 메모리 해제
 *
 * [에러 처리]
 * DMA 할당 실패 시 libnvm 에러 코드 반환.
 * Identify 명령 실패 시 임시 DMA를 해제하고 EIO 반환.
 */
static int identify(struct libnvmfio_thread* data)
{
    int err;
    nvm_dma_t* page;    // Identify 결과를 받을 임시 DMA 페이지 (4KB)

    /* Identify 결과 수신용 임시 DMA 페이지 할당 */
    // NVMe Identify 명령은 항상 4KB(1페이지)의 데이터를 반환
    err = nvm_dis_dma_create(&page, data->ctrl, data->ctrl->page_size, 0);
    if (!nvm_ok(err))
    {
        return err;     // SISCI 세그먼트 DMA 생성 실패
    }

    /* NVMe Identify Controller 명령 (Admin Opcode 0x06, CNS=01) */
    // 컨트롤러가 page->ioaddrs[0] 주소에 4KB Identify 데이터를 DMA로 기록
    // libnvm이 이를 파싱하여 ctrl_info 구조체에 저장
    err = nvm_admin_ctrl_info(data->aq_ref, &data->ctrl_info, page->vaddr, page->ioaddrs[0]);
    if (!nvm_ok(err))
    {
        nvm_dma_unmap(page);    // 실패 시 임시 DMA 해제
        return EIO;
    }

    /* NVMe Identify Namespace 명령 (Admin Opcode 0x06, CNS=00, NSID=1) */
    // 네임스페이스 1의 정보(LBA 크기, 총 블록 수, 용량 등)를 조회
    err = nvm_admin_ns_info(data->aq_ref, &data->ns_info, 1, page->vaddr, page->ioaddrs[0]);
    if (!nvm_ok(err))
    {
        nvm_dma_unmap(page);    // 실패 시 임시 DMA 해제
        return EIO;
    }

    nvm_dma_unmap(page);        // 임시 DMA 페이지 해제 (Identify 데이터는 이미 구조체에 파싱됨)
    return 0;
}


/*
 * create_queues - I/O Submission Queue와 Completion Queue 쌍 생성
 *
 * @data:      스레드 컨텍스트 (큐 메모리와 큐 구조체가 여기에 저장됨)
 * @queue_no:  큐 번호 (1부터 시작, Admin Queue는 0)
 * @iodepth:   fio에서 설정한 I/O 깊이 (실제로는 +1하여 사용, NVMe 큐의 full 판별을 위해)
 * @memhints:  DMA 메모리 할당 힌트 플래그 (원격 큐 시 SCI_MEMACCESS_DEVICE_READ 등 설정)
 * @return:    성공 시 0, 실패 시 에러 코드
 *
 * [동작 흐름]
 * 1. PRP 리스트 배열 할당 (iodepth+1개, 각 SQ 엔트리마다 하나)
 * 2. CQ용 DMA 메모리 할당 (NVM_CQ_PAGES 매크로로 필요 페이지 수 계산)
 * 3. SQ용 DMA 메모리 할당 (SQ 엔트리 + PRP 리스트 공간 포함)
 * 4. nvm_admin_cq_create()로 Admin 명령을 통해 CQ 생성
 * 5. nvm_admin_sq_create()로 Admin 명령을 통해 SQ 생성
 * 6. 각 SQ 엔트리에 대응하는 PRP 리스트 오프셋 계산
 *
 * [에러 처리]
 * 할당 실패 시 이미 할당된 리소스를 역순으로 해제하고 에러 반환 (unwind 레이블).
 */
static int create_queues(struct libnvmfio_thread* data, unsigned int queue_no, size_t iodepth, unsigned int memhints)
{
    int err;
    size_t i;
    const nvm_ctrl_t* ctrl = data->ctrl;    // NVMe 컨트롤러 핸들 (페이지 크기 등 참조)

    /* NVMe 큐는 (depth-1)개의 명령만 저장 가능하므로 +1 */
    // NVMe 스펙: SQ가 가득 찼는지(full) 비었는지(empty) 구분하기 위해 tail==head일 때 empty로 정의
    // 따라서 depth 크기의 큐에 실제로 넣을 수 있는 명령은 depth-1개이므로, +1하여 보정
    ++iodepth;

    /* 각 SQ 엔트리에 대응하는 PRP 리스트 포인터 배열 할당 */
    // PRP 리스트: 데이터가 여러 물리 페이지에 분산될 때 각 페이지의 주소를 나열하는 리스트
    data->prp_lists = calloc(iodepth, sizeof(nvm_prp_list_t));
    assert(data->prp_lists);       // 할당 실패 시 assert로 즉시 종료 (디버깅용)

    /* CQ용 DMA 메모리 할당: CQ 엔트리(16바이트) × iodepth개에 필요한 페이지 수만큼 */
    // NVM_CQ_PAGES: (iodepth × 16바이트 + 페이지크기-1) / 페이지크기로 올림 계산
    err = nvm_dis_dma_create(&data->cq_mem, ctrl,
            NVM_CQ_PAGES(ctrl, iodepth) * ctrl->page_size, 0);
    if (err != 0)
    {
        free(data->prp_lists);      // CQ 할당 실패 시 PRP 배열만 해제
        return err;
    }

    /* SQ용 DMA 메모리 할당: SQ 엔트리(64바이트) 페이지 + PRP 리스트(각 1페이지) × iodepth개 */
    // NVM_SQ_PAGES: SQ 엔트리가 차지하는 페이지 수
    // + iodepth: 각 명령마다 하나의 PRP 리스트 페이지 (scatter-gather DMA 주소 목록)
    err = nvm_dis_dma_create(&data->sq_mem, ctrl,
            (NVM_SQ_PAGES(ctrl, iodepth) + iodepth) * ctrl->page_size, memhints);
    if (err != 0)
    {
        nvm_dma_unmap(data->cq_mem);    // SQ 할당 실패 시 CQ와 PRP 역순 해제
        free(data->prp_lists);
        return err;
    }

    /* CQ 메모리를 0으로 초기화: phase bit 초기값 0 (컨트롤러는 1로 기록하여 새 완료 표시) */
    memset(data->cq_mem->vaddr, 0, data->cq_mem->page_size * data->cq_mem->n_ioaddrs);
    /* Admin Create I/O CQ 명령 (Admin Opcode 0x05): 컨트롤러에 CQ 등록 */
    // queue_no: CQ ID, 오프셋 0, 깊이=iodepth
    err = nvm_admin_cq_create(data->aq_ref, &data->cq, queue_no, data->cq_mem, 0, iodepth);
    if (err != 0)
    {
        goto unwind;    // CQ 생성 실패 시 정리
    }

    /* SQ 메모리를 0으로 초기화 */
    memset(data->sq_mem->vaddr, 0, data->sq_mem->page_size * data->sq_mem->n_ioaddrs);
    /* Admin Create I/O SQ 명령 (Admin Opcode 0x01): 컨트롤러에 SQ 등록, CQ와 연결 */
    // queue_no: SQ ID, &data->cq: 이 SQ의 완료가 전달될 CQ, 오프셋 0, 깊이=iodepth
    err = nvm_admin_sq_create(data->aq_ref, &data->sq, &data->cq, queue_no, data->sq_mem, 0, iodepth);
    if (err != 0)
    {
        goto unwind;    // SQ 생성 실패 시 정리
    }

    /* 각 SQ 엔트리에 대응하는 PRP 리스트의 DMA 메모리 내 오프셋을 미리 계산해 둠 */
    // NVM_PRP_LIST: SQ DMA 윈도우에서 특정 페이지 오프셋의 PRP 리스트 위치를 계산
    // SQ 엔트리 페이지 뒤에 PRP 리스트 페이지들이 연속으로 배치됨
    for (i = 0; i < iodepth; ++i)
    {
        data->prp_lists[i] = NVM_PRP_LIST(data->sq_mem, NVM_SQ_PAGES(ctrl, iodepth + i));
    }

    return 0;

unwind:
    // 에러 발생 시 이미 할당된 리소스를 역순으로 해제
    nvm_dma_unmap(data->sq_mem);
    nvm_dma_unmap(data->cq_mem);
    free(data->prp_lists);
    return err;
}



/*
 * destroy_queues - I/O SQ/CQ 쌍 삭제 및 관련 DMA 메모리 해제
 *
 * @data: 스레드 컨텍스트
 *
 * Admin Delete I/O SQ -> SQ DMA 해제 -> Admin Delete I/O CQ -> CQ DMA 해제 순서로 정리.
 * NVMe 스펙상 SQ를 먼저 삭제한 후 CQ를 삭제해야 한다 (SQ가 CQ를 참조하므로).
 */
static void destroy_queues(struct libnvmfio_thread* data)
{
    nvm_admin_sq_delete(data->aq_ref, &data->sq, &data->cq);   // Admin Delete I/O SQ (Opcode 0x00)
    nvm_dma_unmap(data->sq_mem);                                // SQ + PRP 리스트 DMA 해제
    nvm_admin_cq_delete(data->aq_ref, &data->cq);              // Admin Delete I/O CQ (Opcode 0x04)
    nvm_dma_unmap(data->cq_mem);                                // CQ DMA 해제
    free(data->prp_lists);                                      // PRP 리스트 포인터 배열 해제
}



/*
 * thread_setup - fio 스레드 셋업 콜백
 *
 * @td: fio 스레드 데이터
 * @return: 성공 시 0, SISCI 미초기화 시 -1
 *
 * fio가 각 스레드의 작업을 시작하기 전에 호출하는 콜백.
 * SISCI 초기화 여부만 확인하고, 각 파일명을 stderr에 출력한다.
 */
static int thread_setup(struct thread_data* td)
{
    unsigned int i;
    struct fio_file* f;     // fio 파일 구조체

    // fio의 use_thread 옵션 확인 (SPDK에서는 fork가 아닌 thread 사용을 검증함)
    if (!td->o.use_thread)
    {
        /* SPDK checks for this, should we too? */
    }

    // SISCI 라이브러리가 초기화되지 않았으면 에러 (fio_init 생성자 실패)
    if (!_initialized)
    {
        fprintf(stderr, "Failed to initialize SISCI\n");
        return -1;
    }

    // fio job에 지정된 모든 파일명을 stderr에 출력 (디버깅/확인용)
    for_each_file(td, f, i)     // fio 매크로: td->files[] 배열을 순회
    {
        fprintf(stderr, "%s\n", f->file_name);
    }

    return 0;
}


/*
 * fio_queue - NVMe I/O 명령 제출 콜백 (동기 방식)
 *
 * @td:   fio 스레드 데이터
 * @io_u: fio I/O 유닛 (방향, 오프셋, 크기, 버퍼 포인터 포함)
 * @return: FIO_Q_COMPLETED(완료) 또는 FIO_Q_BUSY(SQ 가득 참)
 *
 * [동작 흐름]
 * 1. io_u->buf의 DMA 버퍼 내 오프셋 계산 (io_mem->vaddr 기준)
 * 2. io_u->ddir에 따라 NVM_IO_READ 또는 NVM_IO_WRITE opcode 결정
 * 3. SQ tail 인덱스를 명령 ID(CID)로 사용
 * 4. NVMe 명령 구성: 헤더(opcode, nsid) -> 데이터(PRP, 버퍼 주소) -> LBA 범위
 * 5. nvm_sq_enqueue()로 SQ에 슬롯 확보, 명령 복사, nvm_sq_submit()으로 doorbell
 * 6. nvm_cq_dequeue()로 완료 busy-wait 폴링
 * 7. 에러 시 stderr에 출력, 큐 포인터 갱신 후 FIO_Q_COMPLETED 반환
 *
 * 주의: iodepth=1 전용 동기 엔진이므로 한 번에 하나의 명령만 처리한다.
 */
static enum fio_q_status fio_queue(struct thread_data* td, struct io_u* io_u)
{
    struct libnvmfio_thread* data = td->io_ops_data;    // 스레드별 NVMe 컨텍스트
    nvm_queue_t* sq = &data->sq;                        // Submission Queue
    nvm_queue_t* cq = &data->cq;                        // Completion Queue
    const size_t ps = data->ctrl->page_size;    /* 컨트롤러 메모리 페이지 크기 (보통 4096) */
    const size_t bs = data->ns_info.lba_data_size;  /* 논리 블록(LBA) 크기 (보통 512 또는 4096) */
    unsigned int idx;       // SQ 엔트리 인덱스 (= 명령 ID)
    uint8_t op;             // NVMe opcode (READ=0x02, WRITE=0x01)
    nvm_cmd_t cmd;          // NVMe 명령 구조체 (64바이트 SQE)
    nvm_cmd_t* ptr;         // SQ 슬롯 포인터
    nvm_cpl_t* cpl;         // CQ 완료 엔트리 포인터
    size_t offset;          // DMA 버퍼 내 바이트 오프셋

    /* fio가 할당한 I/O 버퍼(io_u->buf)의 DMA 윈도우 내 바이트 오프셋 계산 */
    // io_mem->vaddr: DMA 버퍼의 시작 가상 주소, io_u->buf: fio가 이 I/O에 할당한 버퍼
    offset = ((char*) io_u->buf) - ((char*) data->io_mem->vaddr);

    /* fio I/O 방향(ddir)에 따라 NVMe opcode 결정 */
    switch (io_u->ddir)
    {
        case DDIR_READ:             // 읽기 방향
            op = NVM_IO_READ;       // NVMe Read 명령 (Opcode 0x02)
            break;

        case DDIR_WRITE:            // 쓰기 방향
            op = NVM_IO_WRITE;      // NVMe Write 명령 (Opcode 0x01)
            break;

        default:                    // TRIM/기타는 미지원
            assert(false);
            break;
    }

    /* 현재 SQ tail 인덱스를 명령 ID(CID)로 사용 */
    // NVMe CID: 완료 시 어떤 명령이 완료되었는지 식별하는 데 사용
    idx = sq->tail;

    /* NVMe 명령 헤더 구성: CID, opcode(READ/WRITE), namespace ID */
    nvm_cmd_header(&cmd, idx, op, data->ns_info.ns_id);

    /* PRP 리스트를 통해 데이터 버퍼의 물리(DMA) 주소를 명령에 설정 */
    // io_u->xfer_buflen / ps: 이 I/O가 차지하는 페이지 수
    // &data->io_mem->ioaddrs[offset / ps]: DMA 버퍼에서 이 I/O의 시작 페이지 IO 주소
    nvm_cmd_data(&cmd, 1, &data->prp_lists[idx], io_u->xfer_buflen / ps, &data->io_mem->ioaddrs[offset / ps]);
    /* 시작 LBA와 전송할 블록 수 설정 (CDW10~CDW12) */
    // io_u->offset / bs: 바이트 오프셋 → LBA 번호 변환
    // io_u->xfer_buflen / bs: 전송 바이트 → 블록 수 변환
    nvm_cmd_rw_blks(&cmd, io_u->offset / bs, io_u->xfer_buflen / bs);

    /* SQ에 빈 슬롯 확보 (tail 위치에 새 명령을 넣을 공간) */
    ptr = nvm_sq_enqueue(sq);
    if (ptr == NULL)        // SQ가 가득 찬 경우 (tail+1 == head)
    {
        return FIO_Q_BUSY;  // fio에 "큐가 바쁨"을 알려 나중에 재시도
    }

    /* 로컬에서 구성한 명령을 SQ 슬롯(DMA 메모리)에 복사하고 doorbell 링 */
    *ptr = cmd;                 // 64바이트 명령 복사
    nvm_sq_submit(sq);          // SQ doorbell 레지스터에 새 tail 값 기록 → 컨트롤러에 통보

    /* CQ에서 완료 엔트리가 나타날 때까지 busy-wait 폴링 */
    // 동기 방식이므로 이 루프에서 완료될 때까지 블로킹
    while ((cpl = nvm_cq_dequeue(cq)) == NULL);
    // CQ 완료 엔트리에 기록된 SQ head 포인터로 SQ head 갱신
    nvm_sq_update(sq);

    /* 완료 상태 확인: NVMe 에러가 있으면 stderr에 출력 */
    if (!NVM_ERR_OK(cpl))
    {
        fprintf(stderr, "error %s\n", nvm_strerror(NVM_ERR_STATUS(cpl)));
    }

    /* CQ head 포인터를 다음으로 이동하고 CQ doorbell에 기록 (컨트롤러에 소비 완료 알림) */
    nvm_cq_update(cq);

    return FIO_Q_COMPLETED;     // fio에 "I/O 완료"를 알림
}


/*
 * iomem_alloc - fio I/O 데이터 버퍼를 DIS DMA 메모리로 할당
 *
 * @td:   fio 스레드 데이터
 * @size: 할당할 바이트 크기 (fio가 계산한 전체 I/O 버퍼 크기)
 * @return: 성공 시 0, 이미 할당된 경우 EINVAL, DMA 생성 실패 시 에러 코드
 *
 * fio는 보통 malloc/mmap으로 I/O 버퍼를 할당하지만, NVMe 직접 접근에는
 * DMA 가능한 물리 연속 메모리가 필요하므로 nvm_dis_dma_create()로 할당한다.
 * 할당된 가상 주소를 td->orig_buffer에 설정하면 fio가 이 주소를 사용한다.
 */
static int iomem_alloc(struct thread_data* td, size_t size)
{
    int err;
    struct libnvmfio_thread* data = td->io_ops_data;

    // 이미 할당된 경우 중복 할당 방지
    if (data->io_mem)
    {
        return EINVAL;
    }

    // SISCI 세그먼트 기반 DMA 메모리 생성 (NVMe 컨트롤러가 DMA로 접근 가능)
    err = nvm_dis_dma_create(&data->io_mem, data->ctrl, size, 0);
    if (!nvm_ok(err))
    {
        return err;
    }

    /* fio가 사용할 I/O 버퍼 주소를 DMA 윈도우의 가상 주소로 설정 */
    // fio는 이후 td->orig_buffer를 기반으로 각 io_u의 buf 포인터를 계산
    td->orig_buffer = data->io_mem->vaddr;
    return 0;
}


/*
 * iomem_free - DIS DMA I/O 데이터 버퍼 해제
 *
 * @td: fio 스레드 데이터
 *
 * iomem_alloc에서 할당한 DMA 메모리(SISCI 세그먼트)를 해제한다.
 */
static void iomem_free(struct thread_data* td)
{
    struct libnvmfio_thread* data = td->io_ops_data;
    nvm_dma_unmap(data->io_mem);    // DMA 윈도우 해제 (SISCI 세그먼트 포함)
    data->io_mem = NULL;            // 댕글링 포인터 방지
}


/*
 * fio_open - 파일 열기 콜백 (NOP)
 *
 * NVMe 직접 접근 방식이므로 파일시스템을 사용하지 않아 실제 파일 열기가 불필요.
 * 주석 처리된 코드는 fio에 디스크 크기를 알려주는 로직이었으나 미사용.
 */
static int fio_open(struct thread_data* td, struct fio_file* f)
{
//    struct libnvmfio_thread* data = td->io_ops_data;
//
//    f->filetype = FIO_TYPE_BLOCK;                                          // 블록 디바이스 타입으로 설정
//    f->real_file_size = data->ns_info.size * data->ns_info.lba_data_size;  // 네임스페이스 총 바이트 = 블록수 × 블록크기
//    fio_file_set_size_known(f);                                            // 크기 알려진 것으로 마킹
//
    return 0;       // 항상 성공 (NOP)
}


/*
 * fio_close - 파일 닫기 콜백 (NOP)
 *
 * NVMe 직접 접근이므로 파일 닫기 작업 불필요.
 */
static int fio_close(struct thread_data fio_unused *td, struct fio_file fio_unused *f)
{
    return 0;       // 항상 성공 (NOP)
}


/*
 * fio_invalidate - 파일 캐시 무효화 콜백 (NOP)
 *
 * 커널 페이지 캐시를 우회하는 NVMe 직접 접근이므로 캐시 무효화 불필요.
 * fio가 각 iteration 시작 시 이 콜백을 호출하지만 아무것도 하지 않는다.
 */
static int fio_invalidate(struct thread_data fio_unused *td, struct fio_file fio_unused *f)
{
    return 0;       // 항상 성공 (NOP)
}


/*
 * thread_init - fio 스레드 초기화 콜백 (핵심 설정)
 *
 * @td: fio 스레드 데이터 (td->eo에 libnvmfio_options, td->o.iodepth에 I/O 깊이)
 * @return: 성공 시 0, 실패 시 에러 코드
 *
 * [동작 흐름]
 * 1. 옵션 유효성 검증 (fdid, queue_id 필수)
 * 2. libnvmfio_thread 구조체 할당
 * 3. nvm_dis_ctrl_init()으로 NVMe 컨트롤러 핸들 획득 (SmartIO FDID 기반)
 * 4. reset 모드:
 *    - 로컬 Admin Queue DMA 메모리 할당 (2페이지: ASQ + ACQ)
 *    - nvm_aq_create()로 Admin Queue 생성 (컨트롤러 리셋 포함)
 *    - nvm_admin_set_num_queues()로 I/O 큐 개수 설정
 *    비-reset 모드:
 *    - nvm_dis_rpc_bind()로 원격 RPC 서버의 Admin Queue에 바인딩
 * 5. identify()로 컨트롤러/네임스페이스 정보 조회
 * 6. 요청한 queue_id가 허용 범위 내인지 확인
 * 7. create_queues()로 I/O SQ/CQ 쌍 생성
 * 8. td->io_ops_data에 컨텍스트 저장
 *
 * [에러 처리]
 * 각 단계에서 실패 시 이미 할당된 리소스를 역순으로 해제.
 * error 레이블에서 aq_ref, aq_mem, ctrl, data를 순차적으로 해제.
 */
static int thread_init(struct thread_data* td)
{
    int err;
    struct libnvmfio_options* opts = td->eo;    // fio 커스텀 옵션 (fdid, reset 등)
    struct libnvmfio_thread* data;              // 이 스레드의 NVMe 컨텍스트
    char diskname[41];                          // 디스크 모델명 출력용 버퍼
    unsigned int flags = 0;                     // DMA 메모리 할당 힌트 플래그
    uint16_t n_cqs, n_sqs;                      // 컨트롤러에 할당된 CQ/SQ 수

    /* SmartIO Fabric Device ID가 지정되지 않으면 에러 */
    if (opts->fdid == 0)
    {
        return EBADF;       // 잘못된 파일 디스크립터 (여기서는 "컨트롤러 미지정" 의미)
    }

    /* I/O 큐 번호 0은 Admin Queue 전용이므로 I/O에 사용 불가 */
    if (opts->queue_id == 0)
    {
        return EINVAL;      // 잘못된 인자
    }

    /* 이 엔진은 동기 방식(iodepth=1)만 지원: queue 콜백이 완료까지 블로킹 */
    if (td->o.iodepth != 1)
    {
        fprintf(stderr, "Warning: IO depth > 1 is not supported\n");
    }

    // 스레드 컨텍스트 구조체 할당 (calloc으로 모든 필드를 0/NULL로 초기화)
    data = calloc(1, sizeof(struct libnvmfio_thread));
    if (data == NULL)
    {
        return errno;       // 메모리 할당 실패
    }

    /* SmartIO FDID로 NVMe 컨트롤러 핸들 획득 */
    // DIS 패브릭을 통해 원격 NVMe 컨트롤러의 PCIe BAR0 레지스터에 MMIO 접근 가능
    err = nvm_dis_ctrl_init(&data->ctrl, opts->fdid);
    if (!nvm_ok(err))
    {
        free(data);
        return err;
    }

    if (opts->reset)
    {
        /* 리셋 모드: 이 프로세스가 Admin Queue 매니저(소유자)가 됨 */

        /* Admin Queue용 DMA 메모리 할당: 2페이지 (1페이지=ASQ, 1페이지=ACQ) */
        err = nvm_dis_dma_create(&data->aq_mem, data->ctrl, data->ctrl->page_size * 2, 0);
        if (!nvm_ok(err))
        {
            nvm_ctrl_free(data->ctrl);
            free(data);
            return err;
        }

        /* Admin Queue 생성: 컨트롤러 CC.EN=0(리셋) → AQA/ASQ/ACQ 레지스터 설정 → CC.EN=1 */
        err = nvm_aq_create(&data->aq_ref, data->ctrl, data->aq_mem);
        if (!nvm_ok(err))
        {
            nvm_dma_unmap(data->aq_mem);
            nvm_ctrl_free(data->ctrl);
            free(data);
            return err;
        }

        /* Set Features(Feature ID=0x07, Number of Queues): I/O 큐 개수를 컨트롤러에 예약 요청 */
        // opts->queue_id 수만큼의 CQ/SQ를 요청 (실제 할당은 컨트롤러가 결정)
        nvm_admin_set_num_queues(data->aq_ref, opts->queue_id, opts->queue_id);
    }
    else
    {
        /* 비-리셋 모드: 다른 프로세스(rpc_server)가 Admin Queue를 관리하고 있음 */
        /* RPC 바인딩으로 원격 서버의 Admin Queue를 통해 Admin 명령 전달 */
        err = nvm_dis_rpc_bind(&data->aq_ref, data->ctrl, opts->adapter);
        if (!nvm_ok(err))
        {
            nvm_ctrl_free(data->ctrl);
            free(data);
            return err;
        }
    }

    /* Identify Controller/Namespace로 디스크 정보(모델명, 페이지크기, LBA크기 등) 획득 */
    err = identify(data);
    if (err != 0)
    {
        goto error;
    }

    /* Get Features(Number of Queues)로 현재 컨트롤러에 할당된 I/O 큐 개수 조회 */
    err = nvm_admin_get_num_queues(data->aq_ref, &n_cqs, &n_sqs);
    if (err != 0)
    {
        goto error;
    }

    /* 요청한 큐 번호가 컨트롤러가 허용한 SQ 수를 초과하면 에러 */
    if (opts->queue_id > n_sqs)
    {
        err = EINVAL;
        goto error;
    }

    /* remote_queue 옵션: SQ 메모리를 NVMe 컨트롤러가 있는 원격 노드에 배치 */
    // DEVICE_READ: 컨트롤러가 SQ에서 명령을 읽음, HOST_WRITE: CPU가 SQ에 명령을 기록
    // 이렇게 하면 컨트롤러의 SQ fetch 레이턴시가 줄어듦 (NUMA 최적화)
    flags = opts->remote_queue ? (SCI_MEMACCESS_DEVICE_READ | SCI_MEMACCESS_HOST_WRITE) : 0;
    // I/O SQ/CQ 쌍 생성 (큐 번호, iodepth, DMA 힌트 전달)
    err = create_queues(data, opts->queue_id, td->o.iodepth, flags);
    if (err != 0)
    {
        goto error;
    }

    // SQ가 어떤 노드에 할당되었는지 출력 (NUMA 배치 확인)
    fprintf(stderr, "SQ on node: %u\n", nvm_dis_node_from_dma(data->sq_mem));

    // 디스크 모델명 출력
    strncpy(diskname, data->ctrl_info.model_no, 40);
    diskname[40] = '\0';        // null 종료 보장 (NVMe 모델명은 40바이트 고정, 패딩 포함)
    fprintf(stderr, "Using disk: 0x%x %s\n", opts->fdid, diskname);

    // 스레드 컨텍스트를 fio에 등록 (이후 콜백에서 td->io_ops_data로 접근)
    td->io_ops_data = data;
    return 0;

error:
    // 에러 발생 시 이미 할당된 리소스를 역순으로 해제
    nvm_aq_destroy(data->aq_ref);   // Admin Queue 파괴 (NULL이면 안전하게 무시)
    nvm_dma_unmap(data->aq_mem);    // Admin Queue DMA 해제 (NULL이면 안전하게 무시)
    nvm_ctrl_free(data->ctrl);      // 컨트롤러 핸들 해제
    free(data);                     // 스레드 컨텍스트 메모리 해제
    return err;
}


/*
 * thread_cleanup - fio 스레드 정리 콜백
 *
 * @td: fio 스레드 데이터
 *
 * thread_init에서 할당한 모든 리소스를 역순으로 해제한다.
 * 해제 순서: I/O 큐 삭제 → Admin Queue 파괴 → AQ DMA 해제 → 컨트롤러 해제 → 메모리 해제
 */
static void thread_cleanup(struct thread_data* td)
{
    if (td->io_ops_data)    // 초기화가 성공했을 때만 정리 수행
    {
        struct libnvmfio_thread* data = td->io_ops_data;
        td->io_ops_data = NULL;     // 댕글링 포인터 방지

        destroy_queues(data);           // I/O SQ/CQ 삭제 및 DMA 해제
        nvm_aq_destroy(data->aq_ref);   // Admin Queue 파괴
        nvm_dma_unmap(data->aq_mem);    // Admin Queue DMA 해제 (리셋 모드에서만 할당됨)
        nvm_ctrl_free(data->ctrl);      // NVMe 컨트롤러 핸들 해제

        free(data);                     // 스레드 컨텍스트 메모리 해제
    }
}


/*
 * fio 엔진 커스텀 옵션 정의 배열
 *
 * fio 커맨드라인 또는 job 파일에서 다음과 같이 사용:
 *   --ioengine=libnvm --fdid=0xc0c00 --queue_no=1 --reset --namespace=1
 *
 * 각 옵션은 libnvmfio_options 구조체의 해당 필드에 offsetof()로 매핑된다.
 * .name: 옵션 이름, .type: 값 타입, .off1: 구조체 내 오프셋, .help: 도움말 문자열
 */
static struct fio_option options[] =
{
    { .name = "fdid", .lname = "fdid", .alias = "ctrl", .def = "0", .type = FIO_OPT_ULL, .off1 = offsetof(struct libnvmfio_options, fdid), .help = "Fabric device identifier for NVMe controller", .category = FIO_OPT_C_ENGINE, .group = FIO_OPT_G_INVALID, },               // SmartIO FDID (필수)
    { .name = "reset", .lname = "reset_ctrl", .type = FIO_OPT_STR_SET, .off1 = offsetof(struct libnvmfio_options, reset), .help = "Reset controller and become manager", .category = FIO_OPT_C_ENGINE, .group = FIO_OPT_G_INVALID, },                                          // 컨트롤러 리셋 플래그
    { .name = "remote_queue", .lname = "remote_queue", .type = FIO_OPT_STR_SET, .off1 = offsetof(struct libnvmfio_options, remote_queue), .help = "Allocate submission queue close to controller", .category = FIO_OPT_C_ENGINE, .group = FIO_OPT_G_INVALID, },                  // SQ를 컨트롤러 노드에 배치
    { .name = "queue_no", .lname = "queue_no", .alias = "queue_id", .type = FIO_OPT_ULL, .off1 = offsetof(struct libnvmfio_options, queue_id), .help = "Submission queue identifier",.category = FIO_OPT_C_ENGINE, .group = FIO_OPT_G_INVALID, },                                // I/O 큐 번호 (1~N)
    { .name = "adapter", .lname = "adapter", .def = "0", .type = FIO_OPT_ULL, .off1 = offsetof(struct libnvmfio_options, adapter), .help = "DIS adapter number", .category = FIO_OPT_C_ENGINE, .group = FIO_OPT_G_INVALID, },                                                    // DIS 어댑터 번호
    { .name = "namespace", .lname = "nvm_namespace", .alias = "ns", .def = "1", .type = FIO_OPT_ULL, .off1 = offsetof(struct libnvmfio_options, namespace), .help = "NVM namespace", .category = FIO_OPT_C_ENGINE, .group = FIO_OPT_G_INVALID, },                                 // NVMe 네임스페이스 ID
    { .name = NULL, }   // 배열 종료 센티널
};


/*
 * fio I/O 엔진 등록 구조체
 *
 * fio가 이 shared object(.so)를 dlopen()할 때 이 구조체를 참조하여 엔진을 등록한다.
 *
 * 플래그 의미:
 *   FIO_SYNCIO:     동기 I/O (queue 콜백이 완료까지 블로킹, iodepth=1만 의미 있음)
 *   FIO_RAWIO:      raw 디바이스 I/O (파일시스템 불필요, /dev/xxx 직접 접근)
 *   FIO_NOEXTEND:   파일 확장 불가 (NVMe 디바이스 크기 고정)
 *   FIO_NODISKUTIL: fio의 디스크 유틸리티 통계(iostat 등) 비활성화
 *   FIO_MEMALIGN:   커스텀 iomem_alloc/iomem_free 사용 (DMA 메모리 할당)
 */
struct ioengine_ops engine =
{
    .name = "libnvm",               // fio에서 --ioengine=libnvm으로 지정
    .version = FIO_IOOPS_VERSION,   // fio I/O 엔진 API 버전 (호환성 검증)
    .flags = FIO_SYNCIO | FIO_RAWIO | FIO_NOEXTEND | FIO_NODISKUTIL | FIO_MEMALIGN,
    .setup = thread_setup,          // 스레드 셋업 콜백
    .init = thread_init,            // 스레드 초기화 콜백 (NVMe 연결)
    .queue = fio_queue,             // I/O 제출 콜백 (NVMe Read/Write)
    .cleanup = thread_cleanup,      // 스레드 정리 콜백 (리소스 해제)
    .open_file = fio_open,          // 파일 열기 (NOP)
    .close_file = fio_close,        // 파일 닫기 (NOP)
    .invalidate = fio_invalidate,   // 캐시 무효화 (NOP)
    .iomem_alloc = iomem_alloc,     // DMA I/O 버퍼 할당
    .iomem_free = iomem_free,       // DMA I/O 버퍼 해제
    //.get_file_size                // 미구현: 디스크 크기 조회 (fio_open에서 처리 가능했으나 주석)
    .option_struct_size = sizeof(struct libnvmfio_options),     // 커스텀 옵션 구조체 크기
    .options = options,             // 커스텀 옵션 정의 배열
};


/*
 * libnvm_fio_register - fio 엔진 라이브러리 로드 시 자동 호출되는 초기화 함수
 *
 * fio_init 매크로: __attribute__((constructor)) 역할, .so가 dlopen될 때 자동 실행.
 * SISCI 라이브러리 초기화 후 엔진을 fio에 등록한다.
 * SISCI 초기화 실패 시 엔진을 등록하지 않아 fio가 이 엔진을 사용할 수 없게 된다.
 */
static void fio_init libnvm_fio_register(void)
{
    sci_error_t err;

    SCIInitialize(0, &err);     // SISCI 라이브러리 초기화 (DIS 클러스터 통신 기반)
    if (err == SCI_ERR_OK)
    {
        _initialized = true;            // 초기화 성공 플래그 설정
        register_ioengine(&engine);     // fio에 이 I/O 엔진 등록
    }
}


/*
 * libnvm_fio_unregister - fio 엔진 라이브러리 언로드 시 자동 호출되는 정리 함수
 *
 * fio_exit 매크로: __attribute__((destructor)) 역할, .so가 dlclose될 때 자동 실행.
 * fio에서 엔진 등록 해제 후 SISCI 라이브러리를 종료한다.
 */
static void fio_exit libnvm_fio_unregister(void)
{
    if (_initialized)
    {
        unregister_ioengine(&engine);   // fio에서 이 I/O 엔진 등록 해제
        SCITerminate();                 // SISCI 라이브러리 종료
        _initialized = false;           // 초기화 플래그 리셋
    }
}
