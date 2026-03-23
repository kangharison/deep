/* nvm_types.h - BAM(BaM) NVMe 라이브러리의 핵심 자료형 정의 헤더 */
/* 이 파일은 NVMe 컨트롤러, DMA 매핑, 큐, 커맨드/컴플리션 엔트리 등 모든 기본 타입을 정의한다 */
#ifndef __NVM_TYPES_H__
#define __NVM_TYPES_H__
// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <stddef.h>
#include <stdint.h>
#include <simt/atomic>  /* CUDA용 atomic 연산 헤더 - GPU 스레드 간 동기화에 사용 */

/* __align__ 매크로가 정의되지 않은 경우(비-CUDA 환경) 빈 매크로로 정의하여 컴파일 에러 방지 */
#ifndef __align__
#define __align__(x)
#endif



/*
 * NVM controller handle.
 * NVMe 컨트롤러 핸들 구조체.
 * PCIe BAR0 레지스터에서 읽어온 컨트롤러의 기본 속성을 저장한다.
 * API에 의해 할당되며, 반드시 API를 통해 해제해야 한다.
 *
 * Note: This structure will be allocated by the API and needs to be
 *       released by the API.
 */
typedef struct
{
    size_t                  page_size;      // Memory page size used by the controller (MPS)
                                            // 컨트롤러가 사용하는 메모리 페이지 크기 (CC.MPS 레지스터에서 결정)
    uint8_t                 dstrd;          // Doorbell stride (in encoded form)
                                            // 도어벨 레지스터 간 간격 (인코딩된 형태, 실제 간격 = 4 << dstrd 바이트)
    uint64_t                timeout;        // Controller timeout in milliseconds (TO)
                                            // 컨트롤러 타임아웃 값 (밀리초 단위, CAP.TO 레지스터에서 읽음)
    uint32_t                max_qs;         // Maximum queue entries supported (MQES)
                                            // 지원 가능한 최대 큐 엔트리 수 (CAP.MQES + 1)
    size_t                  mm_size;        // Size of memory-mapped region
                                            // 메모리 맵 영역의 크기 (BAR0 MMIO 영역 전체 크기)
    volatile void*          mm_ptr;         // Memory-mapped pointer to BAR0 of the physical device
                                            // 물리 디바이스 BAR0에 대한 메모리 맵 포인터 (레지스터 접근용)
} nvm_ctrl_t;




/*
 * NVM admin queue-pair reference handle.
 * NVMe 어드민 큐 쌍(Admin SQ + Admin CQ)에 대한 참조 핸들.
 *
 * 컨트롤러 리셋과 어드민 큐 설정은 단일 프로세스만 수행할 수 있으므로,
 * 이 구조체는 해당 프로세스에 대한 원격 핸들(RPC 디스크립터)로 사용된다.
 *
 * As only a single process can be responsible of resetting the controller and
 * setting administration queues, this structure represents a remote handle to
 * that process. It is used as a descriptor for executing RPC calls to the
 * remote process owning the admin queues.
 *
 * Note: This structure will be allocated by the API and needs to be released
 *       by the API.
 */
struct nvm_admin_reference;
typedef struct nvm_admin_reference* nvm_aq_ref;  /* 어드민 큐 참조의 불투명(opaque) 포인터 타입 */



/*
 * DMA mapping descriptor.
 * DMA 매핑 디스크립터 구조체.
 *
 * NVMe 컨트롤러가 DMA로 접근 가능한 메모리 영역을 기술한다.
 * 가상 메모리는 연속이지만 물리 페이지는 비연속일 수 있다.
 * 가변 크기 배열(ioaddrs)로 각 물리 페이지의 버스 주소를 저장한다.
 *
 * This structure describes a region of memory that is accessible for the
 * NVM controller using DMA. The API assumes a continuous virtual memory
 * address, but the physical pages do not need to be contiguous.
 *
 * The structure contains a variably sized array of bus addresses that maps
 * to the physical memory pages. The user should therefore not create a local
 * instance of this descriptor, but rather rely on the API to allocate and
 * instantiate members.
 *
 * Note: Only page-aligned addresses are supported in NVM Express
 *
 * Note: This structure will be allocated by the API and needs to be released
 *       by the API.
 */
typedef struct
{
    void*                   vaddr;          // Virtual address to start of region (NB! can be NULL)
                                            // 영역 시작의 가상 주소 (GPU 메모리인 경우 NULL일 수 있음)
    int8_t                  local;          // Is this local memory
                                            // 로컬 메모리 여부 (1이면 호스트 로컬, 0이면 리모트/GPU)
    int8_t                  contiguous;     // Is memory contiguous
                                            // 물리 메모리가 연속인지 여부 (연속이면 단일 PRP로 충분)
    size_t                  page_size;      // Controller's page size (MPS)
                                            // 컨트롤러의 페이지 크기 (nvm_ctrl_t의 page_size와 동일)
    size_t                  n_ioaddrs;      // Number of MPS-sized pages
                                            // MPS 크기 페이지의 개수 (ioaddrs 배열의 길이)
    uint64_t                ioaddrs[];      // Physical/IO addresses of the memory pages
                                            // 각 메모리 페이지의 물리/IO 주소 배열 (유연한 배열 멤버)
}  nvm_dma_t;



/* GPU 디바이스 스코프 atomic uint32_t 타입 - 캐시라인 패딩된 동기화 변수에 사용 */
typedef simt::atomic<uint32_t, simt::thread_scope_device> padded_struct_pc;

/* GPU 캐시라인 크기 정의 (128바이트) - false sharing 방지를 위한 패딩 기준 */
#define CACHELINE_SIZE (128)

/* 캐시라인 하나에 들어가는 padded_struct_pc 변수 개수 */
#define STATES_PER_CACHELINE (CACHELINE_SIZE/sizeof(padded_struct_pc))
/* typedef struct __align__(32) */
/* { */
/*     simt::atomic<uint32_t, simt::thread_scope_device>  val; */
/* //    uint8_t pad[32-4]; */
/* } __attribute__((aligned (32))) padded_struct_pc; */


/* 32바이트 정렬된 atomic 래퍼 구조체 - GPU 스레드 간 동기화 변수를 false sharing 없이 사용 */
typedef struct __align__(32)
{
    simt::atomic<uint32_t, simt::thread_scope_device>  val;  /* 디바이스 스코프 atomic 값 */
    //uint8_t pad[32-8];
} __attribute__((aligned (32))) padded_struct;

/* typedef struct __align__(32) */
/* { */
/*     simt::atomic<uint32_t, simt::thread_scope_system>  val; */
/*     //uint8_t pad[32-8]; */
/* } __attribute__((aligned (32))) padded_struct_pc; */


/* typedef struct __align__(32) */
/* { */
/*     simt::atomic<uint64_t, simt::thread_scope_system>  val; */
/*     uint8_t pad[32-8]; */
/* } __attribute__((aligned (32))) padded_struct; */

/*
 * NVM queue descriptor.
 * NVMe I/O 큐 디스크립터 구조체.
 *
 * NVMe I/O 큐(SQ 또는 CQ)의 메모리 주소, 큐 엔트리, 도어벨 레지스터 포인터 등을 보유한다.
 * 최대 큐 크기는 단일 페이지로 제한된다.
 * GPU에서 직접 NVMe 커맨드를 제출하기 위해 다양한 동기화 필드(lock, ticket 등)를 포함한다.
 *
 * This structure represents an NVM IO queue and holds information
 * about memory addresses, queue entries as well as a memory mapped pointer to
 * the device doorbell register. Maximum queue size is limited to a single
 * page.
 *
 * Note: This descriptor represents both completion and submission queues.
 */
typedef struct
{
    /* head_lock: 큐 헤드 접근 시 GPU 스레드 간 상호 배제를 위한 락 */
    simt::atomic<uint32_t, simt::thread_scope_device> head_lock;
    uint8_t pad0[28];   /* 캐시라인 정렬 패딩 - false sharing 방지 */
    /* tail_lock: 큐 테일 접근 시 GPU 스레드 간 상호 배제를 위한 락 */
    simt::atomic<uint32_t, simt::thread_scope_device> tail_lock;
    uint8_t pad1[28];   /* 캐시라인 정렬 패딩 */
    /* head: 큐의 현재 헤드 인덱스 (CQ에서는 다음 읽을 위치, SQ에서는 컨트롤러가 처리한 위치) */
    simt::atomic<uint32_t, simt::thread_scope_device> head;
    uint8_t pad2[28];   /* 캐시라인 정렬 패딩 */
    /* tail: 큐의 현재 테일 인덱스 (SQ에서는 다음 쓸 위치, CQ에서는 컨트롤러가 완료를 쓸 위치) */
    simt::atomic<uint32_t, simt::thread_scope_device> tail;
    uint8_t pad3[28];   /* 캐시라인 정렬 패딩 */
    /* tail_copy: 시스템 스코프 tail 복사본 - 호스트와 GPU 간 동기화용 */
    simt::atomic<uint32_t, simt::thread_scope_system> tail_copy;
    uint8_t pad4[28];   /* 캐시라인 정렬 패딩 */
    /* head_copy: 시스템 스코프 head 복사본 - 호스트와 GPU 간 동기화용 */
    simt::atomic<uint32_t, simt::thread_scope_system> head_copy;
    uint8_t pad5[28];   /* 캐시라인 정렬 패딩 */

    /* padded_struct<simt::atomic<uint32_t, simt::thread_scope_system>> head; */
    /* padded_struct<simt::atomic<uint32_t, simt::thread_scope_system>> tail; */
    /* in_ticket: GPU 스레드들이 큐 슬롯을 할당받기 위한 티켓 카운터 (ticket lock 메커니즘) */
    simt::atomic<uint32_t, simt::thread_scope_device> in_ticket;
    uint8_t pad6[28];   /* 캐시라인 정렬 패딩 */
    /* cid_ticket: 커맨드 ID(CID)를 할당받기 위한 티켓 카운터 */
    simt::atomic<uint32_t, simt::thread_scope_device> cid_ticket;
    //uint8_t pad7[28];
    padded_struct* tickets;    /* 티켓 락 배열 - 각 큐 슬롯에 대한 순서 보장용 */

    padded_struct* head_mark;  /* 헤드 마크 배열 - 각 슬롯의 헤드 처리 완료 표시 */
    padded_struct* tail_mark;  /* 테일 마크 배열 - 각 슬롯의 테일 기록 완료 표시 */
    padded_struct* cid;        /* CID 배열 - 각 슬롯에 할당된 커맨드 ID 추적 */
    padded_struct* pos_locks;  /* 위치 락 배열 - 각 큐 위치에 대한 세밀한 동기화 */

    uint16_t* clean_cid;       /* 정리된 CID 목록 - 재사용 가능한 커맨드 ID 풀 */
    uint32_t qs_minus_1;       /* 큐 크기 - 1 (모듈러 연산 최적화: index & qs_minus_1) */
    uint32_t qs_log2;          /* 큐 크기의 log2 값 (비트 시프트 연산 최적화용) */
    uint16_t                no;             // Queue number (must be unique per SQ/CQ pair)
                                            // 큐 번호 (SQ/CQ 쌍별로 고유해야 함)
    uint16_t                es;             // Queue entry size
                                            // 큐 엔트리 크기 (SQ=64바이트, CQ=16바이트)
    uint32_t                qs;             // Queue size (number of entries)
                                            // 큐 크기 (엔트리 개수)
    //uint16_t                head;           // Queue's head pointer
    //uint16_t                tail;           // Queue's tail pointer
    int8_t                  phase;          // Current phase tag
                                            // 현재 위상 태그 (CQ에서 새 컴플리션 감지에 사용, 0 또는 1)
    int8_t                  local;          // Is the queue allocated in local memory
                                            // 큐가 로컬 메모리에 할당되었는지 여부
    uint32_t                last;           // Used internally to check db writes
                                            // 도어벨 쓰기 검사용 내부 값
    volatile uint32_t*      db;             // Pointer to doorbell register (NB! write only)
                                            // 도어벨 레지스터 포인터 (쓰기 전용, BAR0의 오프셋)
    volatile void*          vaddr;          // Virtual address to start of queue memory
                                            // 큐 메모리 시작의 가상 주소
    uint64_t                ioaddr;         // Physical/IO address to start of queue memory
                                            // 큐 메모리 시작의 물리/IO 주소 (컨트롤러가 DMA로 접근)
} nvm_queue_t;



/*
 * Convenience type for representing a single-page PRP list.
 * 단일 페이지 PRP(Physical Region Page) 리스트를 나타내는 편의 타입.
 * PRP 리스트는 NVMe 데이터 전송 시 물리 메모리 페이지 주소 목록을 컨트롤러에 전달하는 데 사용된다.
 */
typedef struct __align__(32)
{
    volatile void*          vaddr;          // Virtual address to memory page
                                            // 메모리 페이지의 가상 주소 (PRP 엔트리를 기록할 위치)
    int16_t                 local;          // Indicates if the page is local memory
                                            // 로컬 메모리 여부 (캐시 플러시 방식 결정에 사용)
    size_t                  page_size;      // Page size
                                            // 페이지 크기 (PRP 엔트리 수 계산에 사용)
    uint64_t                ioaddr;         // Physical/IO address of memory page
                                            // 메모리 페이지의 물리/IO 주소 (PRP 체인 연결에 사용)
} __attribute__((aligned (32))) nvm_prp_list_t;



/*
 * NVM completion queue entry type (16 bytes)
 * NVMe 컴플리션 큐 엔트리 타입 (16바이트).
 * 컨트롤러가 커맨드 처리 결과를 CQ에 기록하는 형식이다.
 * dword[0]: 커맨드 고유 값, dword[1]: 예약, dword[2]: SQ 헤드/SQ ID, dword[3]: 상태/CID/위상비트
 */
typedef struct __align__(16)
{
    volatile uint32_t                dword[4];       // The name DWORD is chosen to reflect the specification
                                                     // NVMe 스펙의 DWORD 명명 규칙을 따름 (각 4바이트)
} __attribute__((aligned (16))) nvm_cpl_t;



/*
 * NVM command queue entry type (64 bytes)
 * NVMe 커맨드 큐 엔트리 타입 (64바이트).
 * 호스트/GPU가 SQ에 기록하는 NVMe 커맨드 형식이다.
 * dword[0]: opcode/CID, dword[1]: NSID, dword[6-9]: 데이터 포인터(PRP), dword[10-15]: 커맨드별 필드
 */
typedef struct __align__(64)
{
    uint32_t                dword[16];     /* NVMe 스펙에 따른 16개 DWORD (각 4바이트, 총 64바이트) */
} __attribute__((aligned (64))) nvm_cmd_t;



/*
 * Controller information structure.
 * 컨트롤러 정보 구조체.
 *
 * NVMe 컨트롤러의 온보드 레지스터 읽기 및 IDENTIFY CONTROLLER 어드민 커맨드 실행으로 얻은 정보를 보유한다.
 *
 * Holds information about an NVM controller retrieved from reading on-board
 * registers and running an IDENTIFY CONTROLLER admin command.
 */
struct nvm_ctrl_info
{
    uint32_t                nvme_version;   // NVM Express version number
                                            // NVMe 버전 번호 (예: 0x00010300 = 1.3.0)
    size_t                  page_size;      // Memory page size used by the controller (MPS)
                                            // 컨트롤러가 사용하는 메모리 페이지 크기
    size_t                  db_stride;      // Doorbell stride (DSTRD)
                                            // 도어벨 스트라이드 (바이트 단위 간격)
    uint64_t                timeout;        // Controller timeout in milliseconds (TO)
                                            // 컨트롤러 타임아웃 (밀리초)
    int                     contiguous;     // Contiguous queues required (CQR)
                                            // 연속 큐 요구 여부 (1이면 큐 메모리가 물리적으로 연속이어야 함)
    uint16_t                max_entries;    // Maximum queue entries supported (MQES)
                                            // 지원 가능한 최대 큐 엔트리 수
    uint8_t                 pci_vendor[4];  // PCI vendor and subsystem vendor identifier
                                            // PCI 벤더 및 서브시스템 벤더 식별자
    char                    serial_no[20];  // Serial number (NB! not null terminated)
                                            // 시리얼 번호 (널 종단 없음, 20바이트 고정)
    char                    model_no[40];   // Model number (NB! not null terminated)
                                            // 모델 번호 (널 종단 없음, 40바이트 고정)
    char                    firmware[8];    // Firmware revision
                                            // 펌웨어 리비전 (8바이트 고정)
    size_t                  max_data_size;  // Maximum data transfer size (MDTS)
                                            // 최대 데이터 전송 크기 (바이트 단위)
    size_t                  max_data_pages; // Maximum data transfer size (in controller pages)
                                            // 최대 데이터 전송 크기 (컨트롤러 페이지 수 단위)
    size_t                  cq_entry_size;  // CQ entry size (CQES)
                                            // CQ 엔트리 크기 (보통 16바이트)
    size_t                  sq_entry_size;  // SQ entry size (SQES)
                                            // SQ 엔트리 크기 (보통 64바이트)
    size_t                  max_out_cmds;   // Maximum outstanding commands (MAXCMD)
                                            // 최대 미완료 커맨드 수
    size_t                  max_n_ns;       // Maximum number of namespaces (NN)
                                            // 최대 네임스페이스 수
};



/*
 * Namespace information structure.
 * 네임스페이스 정보 구조체.
 *
 * NVMe 네임스페이스의 용량, 블록 크기 등 정보를 보유한다.
 * IDENTIFY NAMESPACE 어드민 커맨드로 획득한다.
 *
 * Holds informaiton about an NVM namespace.
 */
struct nvm_ns_info
{
    uint32_t                ns_id;          // Namespace identifier
                                            // 네임스페이스 식별자 (1부터 시작)
    size_t                  size;           // Size in logical blocks (NSZE)
                                            // 논리 블록 단위 크기 (네임스페이스 전체 크기)
    size_t                  capacity;       // Capacity in logical blocks (NCAP)
                                            // 논리 블록 단위 용량 (할당 가능한 최대 블록 수)
    size_t                  utilization;    // Utilization in logical blocks (NUSE)
                                            // 논리 블록 단위 사용량 (현재 사용 중인 블록 수)
    size_t                  lba_data_size;  // Logical block size (LBADS)
                                            // 논리 블록 크기 (바이트 단위, 보통 512 또는 4096)
    size_t                  metadata_size;  // Metadata size (MS)
                                            // 메타데이터 크기 (LBA당 추가 메타데이터 바이트)
};



//#ifndef __CUDACC__
//#undef __align__
//#endif

#endif /* __NVM_TYPES_H__ */
