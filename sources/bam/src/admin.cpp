/* admin.cpp - NVMe 관리자(Admin) 명령 구현 파일
 *
 * NVMe 컨트롤러에 대한 Admin 명령(Identify, Create/Delete Queue, Get/Set Features 등)을
 * 생성하고 실행하는 함수들을 제공한다. Admin 명령은 I/O 큐 생성/삭제, 컨트롤러/네임스페이스 정보 조회 등
 * NVMe 디바이스 관리에 필수적인 동작을 수행한다.
 */
#include <nvm_types.h>   // NVMe 타입 정의 (nvm_cmd_t, nvm_cpl_t 등)
#include <nvm_admin.h>   // Admin 명령 API 선언
#include <nvm_cmd.h>     // NVMe 커맨드 헬퍼 함수 (nvm_cmd_header, nvm_cmd_data_ptr 등)
#include <nvm_rpc.h>     // RPC를 통한 Admin 명령 실행 인터페이스
#include <nvm_util.h>    // 유틸리티 매크로 (_RB 비트필드 읽기 등)
#include <nvm_error.h>   // 에러 처리 매크로 (NVM_ERR_PACK 등)
#include <nvm_ctrl.h>    // 컨트롤러 핸들 타입 및 관련 함수
#include <stddef.h>      // 표준 정의 (NULL, size_t 등)
#include <stdint.h>      // 정수형 타입 정의 (uint8_t, uint16_t 등)
#include <stdbool.h>     // bool 타입 정의
#include <string.h>      // 메모리/문자열 조작 함수 (memset, memcpy 등)
#include <errno.h>       // POSIX 에러 코드 정의 (EINVAL, ENOTTY 등)
#include "rpc.h"         // 내부 RPC 헬퍼 함수
#include "regs.h"        // NVMe 레지스터 접근 매크로 (CAP, VER 등)
#include "lib_util.h"    // 내부 유틸리티 함수 (_MIN, _nvm_container_of 등)
#include "dprintf.h"     // 디버그 출력 매크로



/* admin_cq_create - Admin 명령으로 I/O Completion Queue를 생성하는 커맨드를 구성한다.
 * NVMe spec의 Create I/O Completion Queue 명령 형식에 맞게 dword 필드를 채운다.
 */
static void admin_cq_create(nvm_cmd_t* cmd, const nvm_queue_t* cq, uint64_t ioaddr, bool need_prp = false)
{
    // 커맨드 헤더 설정: namespace ID=0, opcode=CREATE_CQ, 기타 필드=0
    nvm_cmd_header(cmd, 0, NVM_ADMIN_CREATE_CQ, 0);
    // 데이터 포인터(PRP1)에 CQ 메모리의 물리 주소 설정
    nvm_cmd_data_ptr(cmd, ioaddr, 0);

    // dword10: 상위 16비트 = 큐 크기(0-based), 하위 16비트 = 큐 식별자(Queue ID)
    cmd->dword[10] = (((uint32_t) cq->qs - 1) << 16) | cq->no;
    // dword11: 상위 16비트 = 인터럽트 벡터, 비트1 = IEN(인터럽트 활성화), 비트0 = PC(물리적 연속 메모리 여부)
    // need_prp가 false이면 PC=1(물리적 연속), true이면 PC=0(PRP 리스트 필요)
    cmd->dword[11] = (0x0000 << 16) | (0x00 << 1) | (!need_prp);
}



/* admin_cq_delete - Admin 명령으로 I/O Completion Queue를 삭제하는 커맨드를 구성한다. */
static void admin_cq_delete(nvm_cmd_t* cmd, const nvm_queue_t* cq)
{
    // 커맨드 헤더 설정: opcode=DELETE_CQ
    nvm_cmd_header(cmd, 0, NVM_ADMIN_DELETE_CQ, 0);
    // dword10: 하위 16비트에 삭제할 CQ의 큐 식별자 설정
    cmd->dword[10] = cq->no & 0xffff;
}



/* admin_sq_create - Admin 명령으로 I/O Submission Queue를 생성하는 커맨드를 구성한다.
 * 생성할 SQ가 연결될 CQ의 식별자도 함께 지정해야 한다.
 */
static void admin_sq_create(nvm_cmd_t* cmd, const nvm_queue_t* sq, const nvm_queue_t* cq, uint64_t ioaddr, bool need_prp = false)
{
    // 커맨드 헤더 설정: opcode=CREATE_SQ
    nvm_cmd_header(cmd, 0, NVM_ADMIN_CREATE_SQ, 0);
    // 데이터 포인터(PRP1)에 SQ 메모리의 물리 주소 설정
    nvm_cmd_data_ptr(cmd, ioaddr, 0);

    // dword10: 상위 16비트 = 큐 크기(0-based), 하위 16비트 = 큐 식별자
    cmd->dword[10] = (((uint32_t) sq->qs - 1) << 16) | sq->no;
    // dword11: 상위 16비트 = 연결될 CQ의 식별자, 비트1 = 큐 우선순위, 비트0 = PC(물리적 연속 여부)
    cmd->dword[11] = (((uint32_t) cq->no) << 16) | (0x00 << 1) | (!need_prp);
}


/* admin_sq_delete - Admin 명령으로 I/O Submission Queue를 삭제하는 커맨드를 구성한다. */
static void admin_sq_delete(nvm_cmd_t* cmd, const nvm_queue_t* sq)
{
    // 커맨드 헤더 설정: opcode=DELETE_SQ
    nvm_cmd_header(cmd, 0, NVM_ADMIN_DELETE_SQ, 0);
    // dword10: 하위 16비트에 삭제할 SQ의 큐 식별자 설정
    cmd->dword[10] = sq->no & 0xffff;
}



/* admin_current_num_queues - Get/Set Features 명령으로 현재 큐 개수를 조회하거나 설정하는 커맨드를 구성한다.
 * Feature ID 0x07은 Number of Queues 기능을 나타낸다.
 */
static void admin_current_num_queues(nvm_cmd_t* cmd, bool set, uint16_t n_cqs, uint16_t n_sqs)
{
    // set이 true이면 SET_FEATURES, false이면 GET_FEATURES opcode 사용
    nvm_cmd_header(cmd, 0, set ? NVM_ADMIN_SET_FEATURES : NVM_ADMIN_GET_FEATURES, 0);
    // 데이터 포인터는 사용하지 않음 (Number of Queues는 dword 내에서 전달)
    nvm_cmd_data_ptr(cmd, 0, 0);

    // dword10: Feature ID = 0x07 (Number of Queues)
    cmd->dword[10] = (0x00 << 8) | 0x07;
    // dword11: set 모드일 때 상위 16비트 = CQ 개수(0-based), 하위 16비트 = SQ 개수(0-based)
    cmd->dword[11] = set ? ((n_cqs - 1) << 16) | (n_sqs - 1) : 0;
}



/* admin_identify_ctrl - Identify Controller 명령 커맨드를 구성한다.
 * 컨트롤러의 하드웨어/펌웨어 정보를 조회하기 위해 사용한다.
 */
static void admin_identify_ctrl(nvm_cmd_t* cmd, uint64_t ioaddr)
{
    // 커맨드 헤더 설정: opcode=IDENTIFY
    nvm_cmd_header(cmd, 0, NVM_ADMIN_IDENTIFY, 0);
    // 응답 데이터를 받을 버퍼의 물리 주소 설정
    nvm_cmd_data_ptr(cmd, ioaddr, 0);

    // dword10: CNS=0x01은 Controller 식별 데이터 구조체를 반환하라는 의미
    cmd->dword[10] = (0 << 16) | 0x01;
    cmd->dword[11] = 0;
}



/* admin_identify_ns - Identify Namespace 명령 커맨드를 구성한다.
 * 특정 네임스페이스의 속성(크기, LBA 포맷 등)을 조회하기 위해 사용한다.
 */
static void admin_identify_ns(nvm_cmd_t* cmd, uint32_t ns_id, uint64_t ioaddr)
{
    // 커맨드 헤더 설정: namespace ID 지정, opcode=IDENTIFY
    nvm_cmd_header(cmd, 0, NVM_ADMIN_IDENTIFY, ns_id);
    // 응답 데이터를 받을 버퍼의 물리 주소 설정
    nvm_cmd_data_ptr(cmd, ioaddr, 0);

    // dword10: CNS=0x00은 Namespace 식별 데이터 구조체를 반환하라는 의미
    cmd->dword[10] = (0 << 16) | 0x00;
    cmd->dword[11] = 0;
}



/* admin_get_log_page - Get Log Page 명령 커맨드를 구성한다.
 * SMART/Health, Error Information 등 컨트롤러 로그 데이터를 조회한다.
 */
static void admin_get_log_page(nvm_cmd_t* cmd, uint32_t ns_id, uint64_t ioaddr, uint8_t log_id, uint64_t log_offset)
{
    // 커맨드 헤더 설정: namespace ID 지정, opcode=GET_LOG_PAGE
    nvm_cmd_header(cmd, 0, NVM_ADMIN_GET_LOG_PAGE, ns_id);
    // 로그 데이터를 받을 버퍼의 물리 주소 설정
    nvm_cmd_data_ptr(cmd, ioaddr, 0);

    // dword10: 상위 16비트 = 반환할 dword 수 (1024 dword = 4KB), 하위 8비트 = Log Page ID
    cmd->dword[10] = (1024 << 16) | log_id;
    cmd->dword[11] = 0;
    // dword12-13: 로그 페이지 내 오프셋 (64비트, 하위/상위 32비트로 분할)
    cmd->dword[12] = (uint32_t)log_offset;
    cmd->dword[13] = (uint32_t)(log_offset >> 32);
}



/* nvm_admin_ctrl_info - 컨트롤러 식별 정보를 조회하여 nvm_ctrl_info 구조체에 채운다.
 * BAR 레지스터에서 기본 속성을 읽고, Identify Controller 명령으로 상세 정보(시리얼, 모델, 펌웨어 등)를 얻는다.
 */
int nvm_admin_ctrl_info(nvm_aq_ref ref, struct nvm_ctrl_info* info, void* ptr, uint64_t ioaddr)
{
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼

    // 매개변수 유효성 검사: info, ptr, ioaddr 모두 유효해야 한다
    if (info == NULL || ptr == NULL || ioaddr == 0)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // 명령, 완료, info 구조체, 데이터 버퍼를 모두 0으로 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));
    memset(info, 0, sizeof(struct nvm_ctrl_info));
    // 4KB(0x1000) 데이터 버퍼 초기화 - Identify 응답 크기가 4KB이므로
    memset(ptr, 0, 0x1000);

    // DMA 전송 전에 CPU 캐시를 무효화하여 디바이스가 쓴 데이터를 정확히 읽을 수 있게 한다
    nvm_cache_invalidate(ptr, 0x1000);

    // Admin Queue 참조로부터 컨트롤러 핸들을 얻는다
    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);

    // BAR 레지스터에서 직접 읽을 수 있는 기본 속성들을 먼저 채운다
    info->nvme_version = (uint32_t) *VER(ctrl->mm_ptr);           // NVMe 버전 레지스터
    info->page_size = ctrl->page_size;                             // 컨트롤러 페이지 크기
    info->db_stride = 1UL << ctrl->dstrd;                          // 도어벨 스트라이드 (레지스터 간격)
    info->timeout = CAP$TO(ctrl->mm_ptr) * 500UL;                 // 타임아웃 값 (500ms 단위)
    info->contiguous = !!CAP$CQR(ctrl->mm_ptr);                   // 연속 메모리 큐 요구 여부
    info->max_entries = CAP$MQES(ctrl->mm_ptr) + 1;               // 최대 큐 엔트리 수 (0-based이므로 +1)

    // Identify Controller 명령 구성
    admin_identify_ctrl(&command, ioaddr);

    // RPC를 통해 Admin 명령 실행
    int err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Identify controller failed: %s\n", nvm_strerror(err));
        return err;
    }

    // 디바이스가 DMA로 쓴 데이터를 읽기 전에 캐시 무효화
    nvm_cache_invalidate(ptr, 0x1000);

    // Identify Controller 응답 데이터(4KB)에서 각 필드를 파싱한다
    const unsigned char* bytes = (const unsigned char*) ptr;
    memcpy(info->pci_vendor, bytes, 4);           // 오프셋 0: PCI Vendor ID + Subsystem Vendor ID (4바이트)
    memcpy(info->serial_no, bytes + 4, 20);       // 오프셋 4: 시리얼 번호 (20바이트)
    memcpy(info->model_no, bytes + 24, 40);       // 오프셋 24: 모델 번호 (40바이트)
    memcpy(info->firmware, bytes + 64, 8);        // 오프셋 64: 펌웨어 리비전 (8바이트)

    // 오프셋 77: MDTS (Maximum Data Transfer Size) - 2의 거듭제곱 단위
    // 최소 메모리 페이지 크기와 곱해서 실제 바이트 단위 최대 전송 크기를 구한다
    info->max_data_size = (1UL << bytes[77]) * (1UL << (12 + CAP$MPSMIN(ctrl->mm_ptr)));
    info->max_data_pages = info->max_data_size / info->page_size;  // 페이지 단위로 변환
    // 오프셋 512: SQES - SQ 엔트리 크기 (하위 4비트가 최소 크기의 log2 값)
    info->sq_entry_size = 1 << _RB(bytes[512], 3, 0);
    // 오프셋 513: CQES - CQ 엔트리 크기 (하위 4비트가 최소 크기의 log2 값)
    info->cq_entry_size = 1 << _RB(bytes[513], 3, 0);
    // 오프셋 514: MAXCMD - 미결 명령 최대 수
    info->max_out_cmds = *((uint16_t*) (bytes + 514));
    // 오프셋 516: NN - 최대 네임스페이스 수
    info->max_n_ns = *((uint32_t*) (bytes + 516));

    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_ns_info - 특정 네임스페이스의 식별 정보를 조회하여 nvm_ns_info 구조체에 채운다.
 * 네임스페이스의 크기, 용량, LBA 데이터 크기, 메타데이터 크기 등을 얻는다.
 */
int nvm_admin_ns_info(nvm_aq_ref ref, struct nvm_ns_info* info, uint32_t ns_id, void* ptr, uint64_t ioaddr)
{
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼

    // 매개변수 유효성 검사
    if (info == NULL || ptr == NULL || ioaddr == 0)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // 모든 버퍼를 0으로 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));
    memset(ptr, 0, 0x1000);
    memset(info, 0, sizeof(struct nvm_ns_info));

    // DMA 전에 캐시 무효화
    nvm_cache_invalidate(ptr, 0x1000);

    // 조회할 네임스페이스 ID 설정
    info->ns_id = ns_id;

    // Identify Namespace 명령 구성
    admin_identify_ns(&command, ns_id, ioaddr);

    // RPC를 통해 Admin 명령 실행
    int err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Identify namespace failed: %s\n", nvm_strerror(err));
        return err;
    }

    // 디바이스가 DMA로 쓴 데이터를 읽기 전에 캐시 무효화
    nvm_cache_invalidate(ptr, 0x1000);

    // Identify Namespace 응답 데이터에서 각 필드를 파싱한다
    const unsigned char* bytes = (const unsigned char*) ptr;
    info->size = *((uint64_t*) ptr);                           // 오프셋 0: NSZE - 네임스페이스 크기 (LBA 블록 수)
    info->capacity = *((uint64_t*) ((uint64_t)ptr + 8));       // 오프셋 8: NCAP - 네임스페이스 용량
    info->utilization = *((uint64_t*) ((uint64_t)ptr + 16));   // 오프셋 16: NUSE - 네임스페이스 사용량

    // 오프셋 26: FLBAS - 현재 사용 중인 LBA 포맷 인덱스 (하위 4비트)
    uint8_t format_idx = _RB(bytes[26], 3, 0);

    // 오프셋 128부터 LBA Format 테이블 시작, 각 엔트리 4바이트
    // 현재 포맷의 LBA 데이터 크기와 메타데이터 크기를 추출
    uint32_t lba_format = *((uint32_t*) (bytes + 128 + sizeof(uint32_t) * format_idx));
    // 비트 23:16 = LBADS (LBA Data Size의 log2 값)
    info->lba_data_size = 1 << _RB(lba_format, 23, 16);
    // 비트 15:0 = MS (Metadata Size in bytes)
    info->metadata_size = _RB(lba_format, 15, 0);

    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_get_log_page - Get Log Page Admin 명령을 실행하여 로그 데이터를 조회한다.
 * SMART/Health Information(log_id=0x02), Error Information(log_id=0x01) 등을 읽을 수 있다.
 */
int nvm_admin_get_log_page(nvm_aq_ref ref, uint32_t ns_id, void* ptr, uint64_t ioaddr, uint8_t log_id, uint64_t log_offset)
{
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼

    // 모든 버퍼를 0으로 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));
    memset(ptr, 0, 0x1000);

    // DMA 전에 캐시 무효화
    nvm_cache_invalidate(ptr, 0x1000);

    // Get Log Page 명령 구성
    admin_get_log_page(&command, ns_id, ioaddr, log_id, log_offset);

    // RPC를 통해 Admin 명령 실행
    int err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Get log page failed: %s\n", nvm_strerror(err));
        return err;
    }

    // 디바이스가 DMA로 쓴 데이터를 읽기 전에 캐시 무효화
    nvm_cache_invalidate(ptr, 0x1000);

    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_cq_create - I/O Completion Queue를 생성한다.
 * DMA 메모리를 할당받아 CQ를 초기화하고, Admin 명령으로 컨트롤러에 CQ 생성을 요청한다.
 */
int nvm_admin_cq_create(nvm_aq_ref ref, nvm_queue_t* cq, uint16_t id, const nvm_dma_t* dma, size_t offset, size_t qs, bool need_prp)
{
    int err;
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼
    nvm_queue_t queue;      // 임시 큐 디스크립터
    size_t n_pages = 0;     // 큐에 필요한 페이지 수

    // Queue number 0 is reserved for admin queues
    // 큐 번호 0은 Admin 큐 전용이므로 I/O 큐에 사용 불가
    if (id == 0)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // Get controller reference
    // Admin Queue 참조로부터 컨트롤러 핸들을 얻는다
    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);
    if (ctrl == NULL)
    {
        return NVM_ERR_PACK(NULL, ENOTTY);
    }

    // Check if a valid queue size was supplied
    // 큐 크기가 0이면 DMA 페이지 크기와 컨트롤러 최대값 중 작은 값으로 자동 설정
    if (qs == 0)
    {
        qs = _MIN(dma->page_size / sizeof(nvm_cpl_t), ctrl->max_qs);
    }

    // 큐 크기 범위 검증: 최소 2개, 최대 64K(0x10000)개, 컨트롤러 최대값 이하
    if (qs < 2 || qs > 0x10000 || qs > ctrl->max_qs)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // CQ에 필요한 컨트롤러 페이지 수 계산
    n_pages = NVM_CQ_PAGES(ctrl, qs);

    // We currently only support contiguous memory
    // 비연속 큐 메모리 지원은 현재 비활성화 상태
    //if (n_pages > 1 && !dma->contiguous) {
    //    dprintf("Non-contiguous queues are not supported\n");
    //    return NVM_ERR_PACK(NULL, ENOTSUP);
    //}

    // Do some sanity checking
    // DMA 가상 주소가 유효한지 확인
    if (dma->vaddr == NULL)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }
    // DMA 버퍼 내에서 offset부터 n_pages만큼의 공간이 충분한지 검증
    else if (n_pages == 0
            || n_pages > dma->n_ioaddrs
            || offset > dma->n_ioaddrs
            || offset + n_pages > dma->n_ioaddrs)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // 임시 큐 디스크립터를 초기화한다 (CQ 모드, 지정된 ID/크기/주소 사용)
    err = nvm_queue_clear(&queue, ctrl, true, id, qs,
            dma->local, NVM_DMA_OFFSET(dma, offset), dma->ioaddrs[offset]);
    if (err != 0)
    {
        return NVM_ERR_PACK(NULL, err);
    }

    // Admin 명령 버퍼 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));

    // Create I/O CQ 명령 구성
    admin_cq_create(&command, &queue, dma->ioaddrs[offset], need_prp);

    // RPC를 통해 Admin 명령 실행하여 컨트롤러에 CQ 생성 요청
    err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Creating completion queue failed: %s\n", nvm_strerror(err));
        return err;
    }
    // 생성 성공 시 임시 큐 디스크립터를 호출자의 cq로 복사
    memcpy((void*) cq, (void*) &queue, sizeof(nvm_queue_t));
    //*cq = queue;
    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_cq_delete - I/O Completion Queue를 삭제한다.
 * Admin 명령으로 컨트롤러에 CQ 삭제를 요청하고, 성공 시 도어벨 포인터를 무효화한다.
 */
int nvm_admin_cq_delete(nvm_aq_ref ref, nvm_queue_t* cq)
{
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼

    // 도어벨 포인터가 NULL이면 이미 삭제되었거나 유효하지 않은 큐
    if (cq->db == NULL)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // Admin 명령 버퍼 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));
    // Delete I/O CQ 명령 구성
    admin_cq_delete(&command, cq);

    // RPC를 통해 Admin 명령 실행
    int err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Deleting completion queue failed: %s\n", nvm_strerror(err));
        return err;
    }

    // 삭제 성공 시 도어벨 포인터를 NULL로 설정하여 큐가 무효함을 표시
    cq->db = NULL;

    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_sq_create - I/O Submission Queue를 생성한다.
 * DMA 메모리를 할당받아 SQ를 초기화하고, Admin 명령으로 컨트롤러에 SQ 생성을 요청한다.
 * SQ는 반드시 이미 존재하는 CQ에 연결되어야 한다.
 */
int nvm_admin_sq_create(nvm_aq_ref ref, nvm_queue_t* sq, const nvm_queue_t* cq, uint16_t id, const nvm_dma_t* dma, size_t offset, size_t qs, bool need_prp)
{
    int err;
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼
    nvm_queue_t queue;      // 임시 큐 디스크립터
    size_t n_pages = 0;     // 큐에 필요한 페이지 수

    // Queue number 0 is reserved for admin queues
    // 큐 번호 0은 Admin 큐 전용
    if (id == 0)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // Get controller reference
    // 컨트롤러 핸들을 얻는다
    const nvm_ctrl_t* ctrl = nvm_ctrl_from_aq_ref(ref);
    if (ctrl == NULL)
    {
        return NVM_ERR_PACK(NULL, ENOTTY);
    }

    // Check if a valid queue size was supplied
    // 큐 크기 자동 설정: SQ 엔트리(nvm_cmd_t) 크기 기준으로 계산
    if (qs == 0)
    {
        qs = _MIN(dma->page_size / sizeof(nvm_cmd_t), ctrl->max_qs);
    }

    // 큐 크기 범위 검증
    if (qs < 2 || qs > 0x10000 || qs > ctrl->max_qs)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // SQ에 필요한 컨트롤러 페이지 수 계산
    n_pages = NVM_SQ_PAGES(ctrl, qs);


    // We currently only support contiguous memory
    // 비연속 큐 메모리 지원은 현재 비활성화 상태
    //if (n_pages > 1 && !dma->contiguous) {
    //    dprintf("Non-contiguous queues are not supported\n");
    //    return NVM_ERR_PACK(NULL, ENOTSUP);
    //}


    // Do some sanity checking
    // DMA 가상 주소 유효성 검사
    if (dma->vaddr == NULL)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }
    // DMA 버퍼 공간 충분성 검증
    else if (n_pages == 0
            || n_pages > dma->n_ioaddrs
            || offset > dma->n_ioaddrs
            || offset + n_pages > dma->n_ioaddrs)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // 임시 큐 디스크립터 초기화 (SQ 모드 = cq 파라미터 false)
    err = nvm_queue_clear(&queue, ctrl, false, id, qs,
            dma->local, NVM_DMA_OFFSET(dma, offset), dma->ioaddrs[offset]);
    if (err != 0)
    {
        return NVM_ERR_PACK(NULL, err);
    }

    // Admin 명령 버퍼 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));
    // Create I/O SQ 명령 구성 (연결할 CQ 정보 포함)
    admin_sq_create(&command, &queue, cq, dma->ioaddrs[offset], need_prp);

    // RPC를 통해 Admin 명령 실행
    err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Creating submission queue failed: %s\n", nvm_strerror(err));
        return err;
    }
    // 생성 성공 시 임시 큐 디스크립터를 호출자의 sq로 복사
    memcpy((void*) sq, (void*) &queue, sizeof(nvm_queue_t));
    //*sq = queue;
    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_sq_delete - I/O Submission Queue를 삭제한다.
 * Admin 명령으로 컨트롤러에 SQ 삭제를 요청하고, 성공 시 도어벨 포인터를 무효화한다.
 */
int nvm_admin_sq_delete(nvm_aq_ref ref, nvm_queue_t* sq, const nvm_queue_t* cq)
{
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼

    // CQ가 유효한지 확인 (SQ 삭제 시 연결된 CQ가 존재해야 한다)
    if (cq == NULL || cq->db == NULL)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // SQ 도어벨이 유효한지 확인
    if (sq->db == NULL)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // Admin 명령 버퍼 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));
    // Delete I/O SQ 명령 구성
    admin_sq_delete(&command, sq);

    // RPC를 통해 Admin 명령 실행
    int err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Deleting submission queue failed: %s\n", nvm_strerror(err));
        return err;
    }

    // 삭제 성공 시 도어벨 포인터를 NULL로 설정
    sq->db = NULL;

    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_get_num_queues - 현재 컨트롤러에 할당된 I/O 큐 개수를 조회한다.
 * Get Features (Feature ID=0x07) 명령을 사용하여 CQ/SQ 각각의 개수를 반환한다.
 */
int nvm_admin_get_num_queues(nvm_aq_ref ref, uint16_t* n_cqs, uint16_t* n_sqs)
{
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼

    // 명령/완료 버퍼 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));

    // Get Features 명령 구성 (set=false: 조회 모드)
    admin_current_num_queues(&command, false, 0, 0);

    // RPC를 통해 Admin 명령 실행
    int err = nvm_raw_rpc(ref, &command, &completion);
    if (!nvm_ok(err))
    {
        dprintf("Failed to get current number of queues: %s\n", nvm_strerror(err));
        return err;
    }

    // 완료 응답의 dword[0]에서 큐 개수 추출 (0-based이므로 +1)
    // 상위 16비트 = SQ 개수, 하위 16비트 = CQ 개수
    *n_sqs = (completion.dword[0] >> 16) + 1;
    *n_cqs = (completion.dword[0] & 0xffff) + 1;

    return NVM_ERR_PACK(NULL, 0);
}



/* nvm_admin_set_num_queues - 요청할 I/O 큐 개수를 설정한다.
 * nvm_admin_request_num_queues의 래퍼 함수이다.
 */
int nvm_admin_set_num_queues(nvm_aq_ref ref, uint16_t n_cqs, uint16_t n_sqs)
{
    // request_num_queues를 호출하여 실제 설정을 수행한다
    return nvm_admin_request_num_queues(ref, &n_cqs, &n_sqs);
}



/* nvm_admin_request_num_queues - I/O 큐 개수를 요청하고 실제 할당된 개수를 반환한다.
 * Set Features 명령으로 원하는 큐 개수를 요청하면, 컨트롤러가 실제 할당 가능한 개수를 응답한다.
 * 요청한 값과 실제 할당된 값이 다를 수 있으므로 n_cqs, n_sqs는 입출력 파라미터이다.
 */
int nvm_admin_request_num_queues(nvm_aq_ref ref, uint16_t* n_cqs, uint16_t* n_sqs)
{
    nvm_cmd_t command;      // Admin 명령 버퍼
    nvm_cpl_t completion;   // 완료 응답 버퍼

    // 0개의 큐 요청은 유효하지 않다
    if (*n_cqs == 0 || *n_sqs == 0)
    {
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // 명령/완료 버퍼 초기화
    memset(&command, 0, sizeof(command));
    memset(&completion, 0, sizeof(completion));

    // Set Features 명령 구성 (set=true: 설정 모드)
    admin_current_num_queues(&command, true, *n_cqs, *n_sqs);

    // RPC를 통해 Admin 명령 실행
    int err = nvm_raw_rpc(ref, &command, &completion);
    if (err != 0)
    {
        dprintf("Failed to set current number of queues: %s\n", nvm_strerror(err));
        return err;
    }

    // 컨트롤러가 실제로 할당한 큐 개수를 응답에서 추출하여 반환
    *n_sqs = (completion.dword[0] >> 16) + 1;
    *n_cqs = (completion.dword[0] & 0xffff) + 1;

    return NVM_ERR_PACK(NULL, 0);
}
