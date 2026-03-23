/* nvm_cmd.h - NVMe 커맨드 빌드 및 PRP 리스트 구성 헤더 */
/* NVMe 커맨드의 헤더, 데이터 포인터, 블록 주소 설정과 PRP 리스트 체인 생성 함수를 제공한다 */
#ifndef __NVM_COMMAND_H__
#define __NVM_COMMAND_H__

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif

#include <nvm_util.h>
#include <nvm_types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>



/* All namespaces identifier */
/* 모든 네임스페이스를 지칭하는 특수 식별자 (0xFFFFFFFF) */
#define NVM_CMD_NS_ALL                  0xffffffff

/* NVMe 커맨드 opcode를 생성하는 매크로 */
/* gen: generic 비트(7), fun: function 비트(6:2), data: data transfer 비트(1:0) */
/* NVMe 스펙에서 opcode는 이 세 필드의 조합으로 정의된다 */
#define NVM_CMD_OPCODE(gen, fun, data)  (_WB((gen), 7, 7) | _WB((fun), 6, 2) | _WB((data), 1, 0))


/* List of NVM IO command opcodes */
/* NVMe I/O 커맨드 opcode 목록 */
enum nvm_io_command_set
{
    NVM_IO_FLUSH            = NVM_CMD_OPCODE(0, 0, 0),  // 00h - 캐시된 데이터를 비휘발성 매체에 플러시
    NVM_IO_WRITE            = NVM_CMD_OPCODE(0, 0, 1),  // 01h - 호스트에서 NVMe 디바이스로 데이터 쓰기
    NVM_IO_READ             = NVM_CMD_OPCODE(0, 0, 2),  // 02h - NVMe 디바이스에서 호스트로 데이터 읽기
    NVM_IO_WRITE_ZEROES     = NVM_CMD_OPCODE(0, 2, 0)   // 08h - 지정 범위를 0으로 기록 (실제 데이터 전송 없음)
};



/* List of NVM admin command opcodes */
/* NVMe 어드민 커맨드 opcode 목록 */
enum nvm_admin_command_set
{
    NVM_ADMIN_DELETE_SQ     = NVM_CMD_OPCODE(0, 0, 0),  // 00h - I/O Submission Queue 삭제
    NVM_ADMIN_CREATE_SQ     = NVM_CMD_OPCODE(0, 0, 1),  // 01h - I/O Submission Queue 생성
    NVM_ADMIN_GET_LOG_PAGE  = NVM_CMD_OPCODE(0, 0, 2),  // 02h - 로그 페이지 조회 (에러, SMART 등)
    NVM_ADMIN_DELETE_CQ     = NVM_CMD_OPCODE(0, 1, 0),  // 04h - I/O Completion Queue 삭제
    NVM_ADMIN_CREATE_CQ     = NVM_CMD_OPCODE(0, 1, 1),  // 05h - I/O Completion Queue 생성
    NVM_ADMIN_IDENTIFY      = NVM_CMD_OPCODE(0, 1, 2),  // 06h - 컨트롤러/네임스페이스 식별 정보 조회
    NVM_ADMIN_ABORT         = NVM_CMD_OPCODE(0, 2, 0),  // 08h - 진행 중인 커맨드 중단
    NVM_ADMIN_SET_FEATURES  = NVM_CMD_OPCODE(0, 2, 1),  // 09h - 컨트롤러 기능 설정 (큐 수 등)
    NVM_ADMIN_GET_FEATURES  = NVM_CMD_OPCODE(0, 2, 2)   // 0Ah - 컨트롤러 기능 조회
};



/*
 * Set command's DWORD0 and DWORD1
 * NVMe 커맨드의 DWORD0(CID, opcode)과 DWORD1(NSID)을 설정한다.
 *
 * DWORD0 구성: [31:16] CID(커맨드 ID), [15:14] PSDT(PRP/SGL), [13:8] 예약, [7:0] opcode
 * DWORD1: 대상 네임스페이스 ID
 */
__device__ __host__ static inline
void nvm_cmd_header(nvm_cmd_t* cmd, uint16_t cid, uint8_t opcode, uint32_t ns_id)
{
    /* DWORD0: CID를 상위 16비트에, opcode를 하위 7비트에 배치. PSDT와 FUSE는 0으로 설정 */
    cmd->dword[0] = ((uint32_t) cid << 16) | (0x00 << 14) | (0x00 << 8) | (opcode & 0x7f);
    /* DWORD1: 네임스페이스 ID 설정 */
    cmd->dword[1] = ns_id;
}



/*
 * Set command's DPTR field (DWORD6-9)
 * NVMe 커맨드의 데이터 포인터 필드(DWORD6-9)를 PRP 방식으로 설정한다.
 *
 * PRP1(prp1): 첫 번째 데이터 페이지의 물리 주소
 * PRP2(prp2): 두 번째 데이터 페이지 주소 또는 PRP 리스트의 물리 주소
 */
__device__ __host__ static inline
void nvm_cmd_data_ptr(nvm_cmd_t* cmd, uint64_t prp1, uint64_t prp2)
{
    /* DWORD0에서 PSDT(비트 15:14)와 비트 9:8을 클리어하여 PRP 전송 모드로 설정 */
    cmd->dword[0] &= ~( (0x03 << 14) | (0x03 << 8) );

    /* PRP1 주소를 DWORD6(하위 32비트)과 DWORD7(상위 32비트)에 분할 저장 */
    cmd->dword[6] = (uint32_t) prp1;
    cmd->dword[7] = (uint32_t) (prp1 >> 32UL);
    /* PRP2 주소를 DWORD8(하위 32비트)과 DWORD9(상위 32비트)에 분할 저장 */
    cmd->dword[8] = (uint32_t) prp2;
    cmd->dword[9] = (uint32_t) (prp2 >> 32UL);
}



/*
 * Set command's block fields (DWORD10-12)
 * NVMe Read/Write 커맨드의 블록 관련 필드(DWORD10-12)를 설정한다.
 *
 * start_lba: 시작 논리 블록 주소 (64비트)
 * n_blks: 전송할 블록 수 (1-based, 내부에서 0-based로 변환)
 */
__device__ __host__ static inline
void nvm_cmd_rw_blks(nvm_cmd_t* cmd, uint64_t start_lba, uint16_t n_blks)
{
    /* DWORD10: 시작 LBA의 하위 32비트 */
    cmd->dword[10] = start_lba;
    /* DWORD11: 시작 LBA의 상위 32비트 */
    cmd->dword[11] = start_lba >> 32;
    /* DWORD12: 하위 16비트에 NLB(Number of Logical Blocks) 설정, n_blks-1이 NVMe 스펙의 0-based 값 */
    cmd->dword[12] = (cmd->dword[12] & 0xffff0000) | ((n_blks - 1) & 0xffff);
}



/*
 * Set command's dataset management (DSM) field (DWORD13)
 * 데이터셋 관리 필드 설정 (현재 미구현)
 */
//__device__ __host__ static inline
//void nvm_cmd_dataset(nvm_cmd_t* cmd, bool sequential, bool low_latency)
//{
//    cmd->dword[13] = 0; // not supported yet
//}



/*
 * Build PRP list consisting of PRP entries.
 * PRP 엔트리로 구성된 PRP 리스트를 빌드한다.
 *
 * 메모리 페이지 하나를 PRP 엔트리(물리 주소)로 채운다.
 * 전송할 페이지 수가 리스트 용량을 초과하면 마지막 엔트리를 다음 PRP 리스트 연결용으로 예약한다.
 * (NVMe 스펙 Chapter 4.4, Figure 14 참조)
 *
 * Populate a memory page with PRP entries required for a transfer.
 * If the number of pages exceed the available number of entries, the last
 * entry will not be used (so that it can be used to point to the next list).
 * (See Chapter 4.4, Figure 14 in the NVMe specification).
 *
 * Returns the number of PRP entries used.
 */
__host__ __device__ static inline
size_t nvm_prp_list(const nvm_prp_list_t* list, size_t n_pages, const uint64_t* ioaddrs)
{
    /* 이 PRP 리스트 페이지에 들어갈 수 있는 최대 PRP 엔트리 수 (페이지크기 / 8바이트) */
    size_t n_prps = list->page_size / sizeof(uint64_t);
    size_t i_prp;
    /* PRP 리스트 페이지의 가상 주소를 uint64_t 배열로 캐스팅하여 엔트리 기록에 사용 */
    volatile uint64_t* entries = (volatile uint64_t*) list->vaddr;

#if !defined( __CUDA_ARCH__ )
    /* 호스트 코드에서만 0페이지 검사 수행 (GPU에서는 분기 최소화를 위해 생략) */
    if (n_pages == 0)
    {
        return 0;
    }
#endif

    // Do we need to reserve the last entry for the next list?
    /* 전송할 페이지가 리스트 용량을 초과하면 마지막 엔트리를 다음 리스트 포인터용으로 예약 */
    if (n_pages > n_prps)
    {
        --n_prps;  /* 마지막 엔트리를 체인 연결용으로 남겨둠 */
    }
    else
    {
        n_prps = n_pages;  /* 모든 페이지가 이 리스트에 들어감 */
    }

    // Populate list
    /* PRP 엔트리를 물리 주소로 채움 */
    for (i_prp = 0; i_prp < n_prps; ++i_prp)
    {
        entries[i_prp] = ioaddrs[i_prp];  /* 각 엔트리에 해당 페이지의 물리/IO 주소를 기록 */
    }

    // Flush list cache
    /* PRP 리스트 캐시를 플러시하여 컨트롤러가 최신 데이터를 읽을 수 있도록 보장 */
    if (list->local)
    {
        nvm_cache_flush((void*) list->vaddr, sizeof(uint64_t) * i_prps);  /* 로컬 메모리: 캐시 플러시 */
    }
    else
    {
        nvm_wcb_flush();  /* 리모트 메모리: Write Combining Buffer 플러시 */
    }

    return i_prp;  /* 사용된 PRP 엔트리 수 반환 */
}



/*
 * Build chain of PRP lists.
 * PRP 리스트 체인을 빌드한다.
 *
 * 여러 PRP 리스트 페이지를 연결하여 대용량 데이터 전송을 지원한다.
 * 각 리스트의 마지막 엔트리가 다음 리스트의 물리 주소를 가리키는 체인 구조를 형성한다.
 *
 * Returns the total number of PRP entries.
 */
__host__ __device__ static inline
size_t nvm_prp_list_chain(size_t n_lists, const nvm_prp_list_t* lists, size_t n_pages, const uint64_t* ioaddrs)
{
    size_t i_list;
    size_t list_prps;
    size_t n_prps;

    /* 리스트가 없거나 NULL이면 0 반환 */
    if (n_lists == 0 || lists == NULL)
    {
        return 0;
    }

    /* 첫 번째 PRP 리스트를 채움 */
    list_prps = nvm_prp_list(&lists[0], n_pages, ioaddrs);

    /* 나머지 PRP 리스트들을 순차적으로 채우며 체인으로 연결 */
    for (i_list = 1, n_prps = list_prps; i_list < n_lists && n_prps < n_pages; ++i_list, n_prps += list_prps)
    {
        /* 이전 리스트의 마지막 엔트리(예약된 위치)에 다음 리스트의 물리 주소를 기록하여 체인 연결 */
        volatile uint64_t* next_list_ptr = ((volatile uint64_t*) lists[i_list - 1].vaddr) + list_prps;
        *next_list_ptr = lists[i_list].ioaddr;

        /* 다음 리스트를 나머지 페이지 주소로 채움 */
        list_prps = nvm_prp_list(&lists[i_list], n_pages - n_prps, &ioaddrs[n_prps]);
    }

    return n_prps;  /* 전체 PRP 엔트리 수 반환 */
}



/*
 * Helper function to build a PRP list and set a command's data pointer fields.
 * PRP 리스트를 빌드하고 커맨드의 데이터 포인터 필드를 설정하는 헬퍼 함수.
 *
 * 전송할 페이지 수에 따라 PRP1/PRP2를 직접 설정하거나 PRP 리스트 체인을 구성한다.
 * - 1페이지: PRP1만 사용
 * - 2페이지: PRP1 + PRP2 직접 사용
 * - 3페이지 이상: PRP1 + PRP 리스트 체인
 *
 * Number of pages should always be max_data_size (MDTS) for IO commands.
 */
__host__ __device__ static inline
size_t nvm_cmd_data(nvm_cmd_t* cmd, size_t n_lists, const nvm_prp_list_t* lists, size_t n_pages, const uint64_t* ioaddrs)
{
    size_t prp = 0;      /* 처리된 PRP 엔트리(페이지) 수 카운터 */
    uint64_t dptr0 = 0;  /* PRP1 값 (첫 번째 데이터 페이지 주소) */
    uint64_t dptr1 = 0;  /* PRP2 값 (두 번째 페이지 주소 또는 PRP 리스트 주소) */

#if !defined( __CUDA_ARCH__ )
    /* 호스트 코드에서만 0페이지 검사 */
    if (n_pages == 0)
    {
        return 0;
    }
#endif
    /* PRP 리스트가 NULL이면 리스트 수를 0으로 설정 */
    if (lists == NULL)
    {
        n_lists = 0;
    }

    /* PRP1: 첫 번째 페이지의 물리 주소를 설정 */
    dptr0 = ioaddrs[prp++];

    if (n_pages > 2 && n_lists != 0)
    {
        /* 3페이지 이상: PRP 리스트 체인을 구성하고, PRP2에 첫 번째 리스트의 주소를 설정 */
        prp += nvm_prp_list_chain(n_lists, lists, n_pages - 1, &ioaddrs[prp]);
        dptr1 = lists[0].ioaddr;  /* PRP2 = 첫 번째 PRP 리스트 페이지의 물리 주소 */
    }
    else if (n_pages >= 2)
    {
        /* 2페이지: PRP2에 두 번째 페이지의 물리 주소를 직접 설정 */
        dptr1 = ioaddrs[prp++];
    }

    /* 커맨드의 DWORD6-9에 PRP1, PRP2 값을 기록 */
    nvm_cmd_data_ptr(cmd, dptr0, dptr1);
    return prp;  /* 처리된 총 페이지 수 반환 */
}



/* Make PRP list descriptor from values */
/* 값들로부터 PRP 리스트 디스크립터를 생성하는 매크로 */
#define NVM_PRP_LIST_INIT(vaddr, local, page_size, ioaddr) \
    ((nvm_prp_list_t) {(vaddr), !!(local), (page_size), (ioaddr)})


/* Make PRP list descriptor from DMA descriptor */
/* DMA 디스크립터에서 특정 오프셋의 PRP 리스트 디스크립터를 생성하는 매크로 */
/* offset번째 페이지의 가상주소, 로컬 여부, 페이지 크기, IO 주소를 추출한다 */
#define NVM_PRP_LIST(dma_ptr, offset)               \
    NVM_PRP_LIST_INIT(NVM_DMA_OFFSET(dma_ptr, offset), (dma_ptr)->local, (dma_ptr)->page_size, (dma_ptr)->ioaddrs[(offset)])




//#ifndef __CUDACC__
//#undef __device__
//#undef __host__
//#endif

#endif /* __NVM_COMMAND_H__ */
