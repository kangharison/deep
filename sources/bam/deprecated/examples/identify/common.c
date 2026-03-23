/*
 * =============================================================================
 * NVMe Identify 예제 공통 구현 (deprecated/examples/identify/common.c)
 * =============================================================================
 *
 * [목적]
 * NVMe Identify Controller/Namespace 명령을 libnvm API로 수행하고
 * 결과를 사람이 읽을 수 있는 형식으로 출력하는 공통 함수 구현.
 *
 * [Deprecated 배경]
 * libnvm 라이브러리의 초기 예제 코드. BaM이 커널 모듈(libnvm.ko) 기반
 * GPU-initiated I/O로 전환되면서 deprecated되었다.
 *
 * [BaM 아키텍처에서의 위치]
 * deprecated/examples/identify/ 디렉토리의 공통 구현.
 * module.c, smartio.c, userspace.c에서 링크하여 사용한다.
 *
 * [NVMe 스펙 참조]
 * - Identify Controller (CNS=01h): NVMe 1.4 스펙 Figure 247
 *   PCI Vendor ID, Serial Number, Model Number, Firmware Revision,
 *   MDTS(Max Data Transfer Size), Max Outstanding Commands 등
 * - Identify Namespace (CNS=00h): NVMe 1.4 스펙 Figure 245
 *   Namespace ID, LBA Data Size, Namespace Size, Namespace Capacity
 */
#include <nvm_types.h>   // libnvm 기본 타입 (nvm_ctrl_t, nvm_ctrl_info, nvm_ns_info 등)
#include <nvm_aq.h>      // Admin Queue 생성/파괴 (nvm_aq_create, nvm_aq_destroy)
#include <nvm_admin.h>   // NVMe Admin 명령 (nvm_admin_ctrl_info, nvm_admin_ns_info, nvm_admin_get_num_queues)
#include <nvm_error.h>   // NVMe 에러 처리 (nvm_ok, nvm_strerror)
#include <stdint.h>      // 고정 크기 정수 타입
#include <string.h>      // memcpy, memset, strerror
#include <stdio.h>       // fprintf, stdout, stderr
#include "common.h"      // 이 파일의 함수 선언 (reset_ctrl, identify_ctrl, identify_ns)



/*
 * print_ctrl_info - Identify Controller 결과를 포맷팅하여 출력
 *
 * @fp:    출력 파일 포인터 (보통 stdout)
 * @info:  nvm_admin_ctrl_info()로 채워진 컨트롤러 정보 구조체
 * @n_cqs: 현재 할당된 CQ 수 (Get Features: Number of Queues로 획득)
 * @n_sqs: 현재 할당된 SQ 수
 *
 * NVMe Identify Controller 응답의 주요 필드를 추출하여 출력:
 * - PCI Vendor/Subsystem Vendor ID (오프셋 0~3)
 * - NVMe 버전 (major.minor.patch)
 * - 컨트롤러 페이지 크기 (CC.MPS에서 결정)
 * - 최대 큐 엔트리 수 (CAP.MQES)
 * - 시리얼 번호 (20바이트), 모델 번호 (40바이트), 펌웨어 리비전 (8바이트)
 * - 최대 데이터 전송 크기 (MDTS, 바이트 단위)
 */
static void print_ctrl_info(FILE* fp, const struct nvm_ctrl_info* info, uint16_t n_cqs, uint16_t n_sqs)
{
    // PCI Vendor ID (2바이트) + Subsystem Vendor ID (2바이트)를 바이트 배열로 복사
    unsigned char vendor[4];
    memcpy(vendor, &info->pci_vendor, sizeof(vendor));  // pci_vendor 필드에서 4바이트 복사

    // NVMe 스펙: Serial Number은 20바이트 고정 길이, null 종료 미보장이므로 수동 처리
    char serial[21];
    memset(serial, 0, 21);                   // 21바이트 0으로 초기화 (null 종료 보장)
    memcpy(serial, info->serial_no, 20);     // 20바이트 시리얼 번호 복사

    // NVMe 스펙: Model Number은 40바이트 고정 길이
    char model[41];
    memset(model, 0, 41);                    // 41바이트 0으로 초기화
    memcpy(model, info->model_no, 40);       // 40바이트 모델 번호 복사

    // NVMe 스펙: Firmware Revision은 8바이트 고정 길이
    char revision[9];
    memset(revision, 0, 9);                  // 9바이트 0으로 초기화
    memcpy(revision, info->firmware, 8);     // 8바이트 펌웨어 리비전 복사

    // 컨트롤러 정보를 사람이 읽기 쉬운 형식으로 출력
    fprintf(fp, "------------- Controller information -------------\n");
    fprintf(fp, "PCI Vendor ID           : %x %x\n", vendor[0], vendor[1]);       // PCI Vendor ID (리틀 엔디안)
    fprintf(fp, "PCI Subsystem Vendor ID : %x %x\n", vendor[2], vendor[3]);       // PCI Subsystem Vendor ID
    fprintf(fp, "NVM Express version     : %u.%u.%u\n",
            info->nvme_version >> 16, (info->nvme_version >> 8) & 0xff, info->nvme_version & 0xff);  // NVMe 버전: 상위16=major, 중간8=minor, 하위8=patch
    fprintf(fp, "Controller page size    : %zu B (0x%zx)\n", info->page_size, info->page_size);      // CC.MPS에서 결정된 페이지 크기
    fprintf(fp, "Max queue entries       : %u\n", info->max_entries);              // CAP.MQES: 큐당 최대 엔트리 수
    fprintf(fp, "Serial Number           : %s\n", serial);                         // 디바이스 시리얼 번호
    fprintf(fp, "Model Number            : %s\n", model);                          // 디바이스 모델명
    fprintf(fp, "Firmware revision       : %s\n", revision);                       // 펌웨어 버전
    fprintf(fp, "Max data transfer size  : %zu B (%zu KiB)\n", info->max_data_size, info->max_data_size >> 10);  // MDTS를 바이트와 KiB로 출력
    fprintf(fp, "Max outstanding commands: %zu\n", info->max_out_cmds);            // 동시 실행 가능한 최대 명령 수
    fprintf(fp, "Max number of namespaces: %zu\n", info->max_n_ns);               // 지원 가능한 최대 네임스페이스 수
    fprintf(fp, "Current number of CQs   : %u\n", n_cqs);                         // 현재 할당된 I/O Completion Queue 수
    fprintf(fp, "Current number of SQs   : %u\n", n_sqs);                         // 현재 할당된 I/O Submission Queue 수
    fprintf(fp, "--------------------------------------------------\n");
}


/*
 * print_ns_info - Identify Namespace 결과를 포맷팅하여 출력
 *
 * @fp:   출력 파일 포인터
 * @info: nvm_admin_ns_info()로 채워진 네임스페이스 정보 구조체
 *
 * 출력 항목: 네임스페이스 ID, 논리 블록 크기(LBA Data Size),
 * 네임스페이스 크기(총 블록 수), 네임스페이스 용량(사용 가능 블록 수)
 */
static void print_ns_info(FILE* fp, const struct nvm_ns_info* info)
{
    fprintf(fp, "------------- Namespace  information -------------\n");
    fprintf(fp, "Namespace identifier    : %x\n", info->ns_id);                   // NSID: 네임스페이스 고유 식별자
    fprintf(fp, "Logical block size      : %zu bytes\n", info->lba_data_size);     // LBA 데이터 크기 (보통 512B 또는 4KB)
    fprintf(fp, "Namespace size          : %zu blocks\n", info->size);             // 네임스페이스 총 블록 수 (NSZE)
    fprintf(fp, "Namespace capacity      : %zu blocks\n", info->capacity);         // 네임스페이스 용량 블록 수 (NCAP)
    fprintf(fp, "--------------------------------------------------\n");
}



/*
 * reset_ctrl - NVMe 컨트롤러를 리셋하고 Admin Queue(ASQ/ACQ)를 설정
 *
 * @ctrl:       NVMe 컨트롤러 핸들
 * @dma_window: Admin Queue용 DMA 윈도우 (최소 2페이지: 페이지0=ACQ, 페이지1=ASQ)
 * @return:     성공 시 Admin Queue 참조, 실패 시 NULL
 *
 * [절차]
 * 1. DMA 윈도우 크기 검증 (최소 2페이지, n_ioaddrs >= 2)
 * 2. 큐 메모리 영역(2페이지)을 0으로 초기화
 * 3. nvm_aq_create()로 컨트롤러 리셋 + Admin Queue 생성
 *    내부적으로: CC.EN=0 -> AQA/ASQ/ACQ 설정 -> CC.EN=1 순서 수행
 */
nvm_aq_ref reset_ctrl(const nvm_ctrl_t* ctrl, const nvm_dma_t* dma_window)
{
    int status;           // 함수 반환값 저장용
    nvm_aq_ref admin;     // Admin Queue 참조 핸들

    // DMA 윈도우에 최소 2페이지(ACQ+ASQ)가 있는지 확인
    if (dma_window->n_ioaddrs < 2)
    {
        return NULL;      // 페이지 수 부족: Admin Queue 생성 불가
    }
    /* Admin Queue 메모리를 0으로 초기화 (ACQ 1페이지 + ASQ 1페이지) */
    memset((void*) dma_window->vaddr, 0, dma_window->page_size * 2);  // 큐 엔트리를 모두 0으로 클리어

    fprintf(stderr, "Resetting controller and setting up admin queues...\n");  // 진행 상태 출력
    status = nvm_aq_create(&admin, ctrl, dma_window);  // 컨트롤러 리셋 + ASQ/ACQ 레지스터 설정 + CC.EN=1
    if (status != 0)
    {
        fprintf(stderr, "Failed to reset controller: %s\n", strerror(status));  // 리셋 실패 시 에러 메시지
        return NULL;
    }

    fprintf(stderr, "Admin queues OK\n");  // Admin Queue 생성 성공
    return admin;                          // Admin Queue 참조 반환
}



/*
 * identify_ctrl - Identify Controller 명령 실행 및 정보 출력
 *
 * @admin:  Admin Queue 참조
 * @ptr:    결과를 받을 4KB 버퍼의 가상 주소
 * @ioaddr: 버퍼의 물리/IO 주소 (컨트롤러가 DMA로 결과를 기록할 주소)
 * @return: 성공 시 0, 실패 시 NVMe 에러 코드
 *
 * [동작]
 * 1. Get Features (Feature ID 0x07: Number of Queues)로 현재 CQ/SQ 수 조회
 * 2. Identify Controller 명령 실행 (Admin Opcode 0x06, CNS=1)
 * 3. 결과를 nvm_ctrl_info 구조체로 파싱하여 출력
 */
int identify_ctrl(nvm_aq_ref admin, void* ptr, uint64_t ioaddr)
{
    int status;                    // NVMe 명령 실행 결과
    uint16_t n_cqs = 0;           // 현재 할당된 I/O Completion Queue 수
    uint16_t n_sqs = 0;           // 현재 할당된 I/O Submission Queue 수
    struct nvm_ctrl_info info;    // Identify Controller 결과를 파싱한 구조체

    fprintf(stderr, "Getting number of queues...\n");  // 진행 상태 출력
    status = nvm_admin_get_num_queues(admin, &n_cqs, &n_sqs);  // Get Features (Feature ID 0x07)로 현재 큐 수 조회
    if (status != 0)
    {
        fprintf(stderr, "Failed to get number of queues\n");  // 큐 수 조회 실패
        return status;
    }

    fprintf(stderr, "Identifying controller...\n");  // 진행 상태 출력
    status = nvm_admin_ctrl_info(admin, &info, ptr, ioaddr);  // Admin Identify Controller 명령 (Opcode 0x06, CNS=1) 실행
    if (!nvm_ok(status))
    {
        fprintf(stderr, "Failed to identify controller: %s\n", nvm_strerror(status));  // NVMe 에러 문자열 출력
        return status;
    }

    print_ctrl_info(stdout, &info, n_cqs, n_sqs);  // 컨트롤러 정보를 stdout에 출력
    return 0;  // 성공
}



/*
 * identify_ns - Identify Namespace 명령 실행 및 정보 출력
 *
 * @admin:         Admin Queue 참조
 * @nvm_namespace: 조회할 네임스페이스 ID (1부터 시작)
 * @ptr:           결과를 받을 4KB 버퍼의 가상 주소
 * @ioaddr:        버퍼의 물리/IO 주소
 * @return:        성공 시 0, 실패 시 에러 코드
 *
 * Identify Namespace 명령 (Admin Opcode 0x06, CNS=0)을 실행하여
 * LBA 크기, 총 블록 수, 용량 등 네임스페이스 정보를 조회한다.
 */
int identify_ns(nvm_aq_ref admin, uint32_t nvm_namespace, void* ptr, uint64_t ioaddr)
{
    int status;                   // NVMe 명령 실행 결과
    struct nvm_ns_info info;      // Identify Namespace 결과를 파싱한 구조체

    status = nvm_admin_ns_info(admin, &info, nvm_namespace, ptr, ioaddr);  // Admin Identify Namespace 명령 (Opcode 0x06, CNS=0) 실행
    if (status != 0)
    {
        fprintf(stderr, "Failed to identify namespace: %s\n", strerror(status));  // 실패 시 에러 메시지
        return status;
    }

    print_ns_info(stdout, &info);  // 네임스페이스 정보를 stdout에 출력
    return 0;  // 성공
}
