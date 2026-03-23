/* queue.h - NVMe I/O Queue Pair (SQ + CQ) 생성 및 관리
 * NVMe I/O Submission Queue와 Completion Queue를 쌍으로 생성하고,
 * GPU에서 직접 도어벨 레지스터를 읽고 쓸 수 있도록 CUDA 디바이스 포인터를 설정한다.
 * GPU 스레드가 NVMe 큐를 병렬로 사용하기 위한 lock-free 데이터 구조(tickets, marks 등)도 초기화한다.
 */
#ifndef __BENCHMARK_QUEUEPAIR_H__
#define __BENCHMARK_QUEUEPAIR_H__
// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <algorithm>
#include <cstdint>
#include "buffer.h"
#include "ctrl.h"
#include "cuda.h"
#include "nvm_types.h"
#include "nvm_util.h"
#include "nvm_error.h"
#include "nvm_admin.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <cmath>
#include "util.h"

using error = std::runtime_error;
using std::string;


/* QueuePair 구조체: NVMe I/O Submission Queue와 Completion Queue를 하나의 쌍으로 관리한다.
 * BaM에서는 여러 GPU 스레드가 동시에 하나의 QueuePair를 사용하므로,
 * lock-free 동기화 구조(tickets, tail_mark, head_mark 등)를 함께 관리한다.
 */
struct QueuePair
{
    uint32_t            pageSize;           // NVMe 컨트롤러 페이지 크기 (보통 4KB)
    uint32_t            block_size;         // LBA 데이터 크기 (보통 512B 또는 4KB)
    uint32_t            block_size_log;     // LBA 크기의 log2 값 (비트 시프트 연산용)
    uint32_t            block_size_minus_1; // LBA 크기 - 1 (마스크 연산용)
    uint32_t            nvmNamespace;       // NVMe 네임스페이스 ID
    nvm_queue_t         sq;                 // Submission Queue 디스크립터
    nvm_queue_t         cq;                 // Completion Queue 디스크립터
    uint16_t            qp_id;              // Queue Pair 식별 번호
    DmaPtr              sq_mem;             // SQ 메모리 DMA 매핑 (GPU 메모리)
    DmaPtr              cq_mem;             // CQ 메모리 DMA 매핑 (GPU 메모리)
    DmaPtr              prp_mem;            // PRP 리스트 DMA 매핑
    BufferPtr           sq_tickets;         // SQ 슬롯별 티켓 배열 (lock-free 순서 보장에 사용)
    BufferPtr           sq_tail_mark;       // SQ tail 이동 마킹 배열 (도어벨 배치 업데이트용)
    BufferPtr           sq_cid;             // Command ID 할당 배열 (65536개 CID 관리)
    BufferPtr           cq_head_mark;       // CQ head 이동 마킹 배열 (도어벨 배치 업데이트용)
    BufferPtr           cq_pos_locks;       // CQ 위치별 잠금 배열 (동일 슬롯 재사용 방지)


/* 64KB SQ/CQ 메모리 내 최대 엔트리 수.
 * SQ 엔트리 = 64바이트, CQ 엔트리 = 16바이트이므로:
 * SQ: 64KB / 64B = 1024, CQ: 64KB / 16B = 4096
 */
#define MAX_SQ_ENTRIES_64K  (64*1024/64)
#define MAX_CQ_ENTRIES_64K  (64*1024/16)

    /* init_gpu_specific_struct: GPU 스레드 병렬 접근을 위한 동기화 구조체들을 GPU 메모리에 할당한다.
     * tickets: 각 SQ 슬롯의 차례를 나타내는 티켓 (Lamport's bakery 알고리즘 변형)
     * tail_mark: SQ에 커맨드가 기록 완료되었음을 표시하는 마커
     * cid: 65536개 Command ID의 사용 상태 (LOCKED/UNLOCKED)
     * head_mark: CQ 완료 처리가 끝났음을 표시하는 마커
     * pos_locks: CQ 슬롯 재사용 경쟁을 방지하는 잠금
     */
    inline void init_gpu_specific_struct( const uint32_t cudaDevice) {
        // SQ 슬롯별 티켓 배열 할당 (lock-free 순서 보장)
        this->sq_tickets = createBuffer(this->sq.qs * sizeof(padded_struct), cudaDevice);
        // SQ tail 마킹 배열 할당 (커맨드 기록 완료 표시)
        this->sq_tail_mark = createBuffer(this->sq.qs * sizeof(padded_struct), cudaDevice);
        // 65536개 Command ID 사용 상태 배열 할당
        this->sq_cid = createBuffer(65536 * sizeof(padded_struct), cudaDevice);
        // nvm_queue_t에 GPU 포인터를 설정한다
        this->sq.tickets = (padded_struct*) this->sq_tickets.get();
        this->sq.tail_mark = (padded_struct*) this->sq_tail_mark.get();
        this->sq.cid = (padded_struct*) this->sq_cid.get();
        // 큐 크기 관련 최적화 값을 계산한다
        this->sq.qs_minus_1 = this->sq.qs - 1;       // 모듈러 연산 대신 비트 AND에 사용
        this->sq.qs_log2 = (uint32_t) std::log2(this->sq.qs); // 나눗셈 대신 시프트에 사용

        // CQ head 마킹 배열 할당 (완료 처리 완료 표시)
        this->cq_head_mark = createBuffer(this->cq.qs * sizeof(padded_struct), cudaDevice);
        this->cq.head_mark = (padded_struct*) this->cq_head_mark.get();
        // CQ 큐 크기 관련 최적화 값 계산
        this->cq.qs_minus_1 = this->cq.qs - 1;
        this->cq.qs_log2 = (uint32_t) std::log2(this->cq.qs);
        // CQ 위치별 잠금 배열 할당
        this->cq_pos_locks = createBuffer(this->cq.qs * sizeof(padded_struct), cudaDevice);
        this->cq.pos_locks = (padded_struct*) this->cq_pos_locks.get();
    }



    /* QueuePair 생성자:
     * 1) NVMe CAP 레지스터에서 CQR(Contiguous Queues Required) 비트를 확인한다
     * 2) 큐 크기를 결정한다 (하드웨어 최대, 64K 제한, 사용자 지정 중 최소값)
     * 3) SQ/CQ DMA 메모리를 GPU에 할당한다
     * 4) Admin 커맨드로 CQ, SQ를 생성한다
     * 5) 도어벨 레지스터를 GPU 디바이스 포인터로 변환한다
     * 6) GPU 병렬 접근용 동기화 구조체를 초기화한다
     */
    inline QueuePair( const nvm_ctrl_t* ctrl, const uint32_t cudaDevice, const struct nvm_ns_info ns, const struct nvm_ctrl_info info, nvm_aq_ref& aq_ref, const uint16_t qp_id, const uint64_t queueDepth)
    {
        // NVMe CAP 레지스터를 읽어 CQR(Contiguous Queues Required) 비트를 확인한다
        uint64_t cap = ((volatile uint64_t*) ctrl->mm_ptr)[0];
        bool cqr = (cap & 0x0000000000010000) == 0x0000000000010000; // CAP.CQR 비트

        // SQ 크기 결정: CQR이면 64KB 제한 적용, MQES+1과 queueDepth 중 최소값
        uint64_t sq_size = (cqr) ?
            ((MAX_SQ_ENTRIES_64K <= ((((volatile uint16_t*) ctrl->mm_ptr)[0] + 1) )) ? MAX_SQ_ENTRIES_64K :  ((((volatile uint16_t*) ctrl->mm_ptr)[0] + 1) ) ) :
            ((((volatile uint16_t*) ctrl->mm_ptr)[0] + 1) );
        // CQ 크기 결정: 동일한 로직
        uint64_t cq_size = (cqr) ?
            ((MAX_CQ_ENTRIES_64K <= ((((volatile uint16_t*) ctrl->mm_ptr)[0] + 1) )) ? MAX_CQ_ENTRIES_64K :  ((((volatile uint16_t*) ctrl->mm_ptr)[0] + 1) ) ) :
            ((((volatile uint16_t*) ctrl->mm_ptr)[0] + 1) );
        // 사용자 지정 큐 깊이와 비교하여 최소값 사용
        sq_size = std::min(queueDepth, sq_size);
        cq_size = std::min(queueDepth, cq_size);

        // PRP 리스트가 필요한지 여부 (현재는 비활성화)
        bool sq_need_prp = false;
        bool cq_need_prp = false;

        // SQ/CQ 메모리 크기 계산 (엔트리 수 * 엔트리 크기)
        size_t sq_mem_size =  sq_size * sizeof(nvm_cmd_t) + sq_need_prp*(64*1024);
        size_t cq_mem_size =  cq_size * sizeof(nvm_cpl_t) + cq_need_prp*(64*1024);

        // SQ 메모리를 GPU에 DMA 매핑하여 할당한다 (NVMe 컨트롤러가 직접 접근 가능)
        this->sq_mem = createDma(ctrl, NVM_PAGE_ALIGN(sq_mem_size, 1UL << 16), cudaDevice);
        // CQ 메모리를 GPU에 DMA 매핑하여 할당한다
        this->cq_mem = createDma(ctrl, NVM_PAGE_ALIGN(cq_mem_size, 1UL << 16), cudaDevice);

        // QueuePair 멤버 설정
        this->pageSize = info.page_size;
        this->block_size = ns.lba_data_size;
        this->block_size_minus_1 = ns.lba_data_size-1;
        this->block_size_log = std::log2(ns.lba_data_size);
        this->nvmNamespace = ns.ns_id;
        this->qp_id = qp_id;

        // CQ에 PRP 리스트가 필요한 경우 PRP 엔트리를 설정한다 (현재 비활성)
        if (cq_need_prp) {
            size_t iters = (size_t)ceil(((float)cq_size*sizeof(nvm_cpl_t))/((float)ctrl->page_size));
            uint64_t* cpu_vaddrs = (uint64_t*) malloc(64*1024);
            memset((void*)cpu_vaddrs, 0, 64*1024);
            for (size_t i = 0; i < iters; i++) {
                size_t page_64  = i/(64*1024);
                size_t page_4 = i%(64*1024/ctrl->page_size);
                cpu_vaddrs[i] = this->cq_mem.get()->ioaddrs[1 + page_64] + (page_4 * ctrl->page_size);
            }

            if (this->cq_mem.get()->vaddr) {
                cuda_err_chk(cudaMemcpy(this->cq_mem.get()->vaddr, cpu_vaddrs, 64*1024, cudaMemcpyHostToDevice));
            }

            this->cq_mem.get()->vaddr = (void*)((uint64_t)this->cq_mem.get()->vaddr + 64*1024);

            free(cpu_vaddrs);
        }

        // SQ에 PRP 리스트가 필요한 경우 PRP 엔트리를 설정한다 (현재 비활성)
        if (sq_need_prp) {
            size_t iters = (size_t)ceil(((float)sq_size*sizeof(nvm_cpl_t))/((float)ctrl->page_size));
            uint64_t* cpu_vaddrs = (uint64_t*) malloc(64*1024);
            memset((void*)cpu_vaddrs, 0, 64*1024);
            for (size_t i = 0; i < iters; i++) {
                size_t page_64  = i/(64*1024);
                size_t page_4 = i%(64*1024/ctrl->page_size);
                cpu_vaddrs[i] = this->sq_mem.get()->ioaddrs[1 + page_64] + (page_4 * ctrl->page_size);
            }

            if (this->sq_mem.get()->vaddr) {
                cuda_err_chk(cudaMemcpy(this->sq_mem.get()->vaddr, cpu_vaddrs, 64*1024, cudaMemcpyHostToDevice));
            }

            this->sq_mem.get()->vaddr = (void*)((uint64_t)this->sq_mem.get()->vaddr + 64*1024);

            free(cpu_vaddrs);
        }

        // Admin 커맨드로 Completion Queue를 생성한다
        int status = nvm_admin_cq_create(aq_ref, &this->cq, qp_id, this->cq_mem.get(), 0, cq_size, cq_need_prp);
        if (!nvm_ok(status))
        {
            throw error(string("Failed to create completion queue: ") + nvm_strerror(status));
        }

        // CQ 도어벨 레지스터의 GPU 디바이스 포인터를 얻는다
        // cudaHostGetDevicePointer: cudaHostRegisterIoMemory로 등록된 MMIO 주소를 GPU 포인터로 변환
        void* devicePtr = nullptr;
        cudaError_t err = cudaHostGetDevicePointer(&devicePtr, (void*) this->cq.db, 0);
        if (err != cudaSuccess)
        {
            throw error(string("Failed to get device pointer") + cudaGetErrorString(err));
        }
        // CQ 도어벨을 GPU 디바이스 포인터로 교체한다 (GPU 커널에서 직접 도어벨 쓰기 가능)
        this->cq.db = (volatile uint32_t*) devicePtr;

        // Admin 커맨드로 Submission Queue를 생성한다 (CQ와 연결)
        status = nvm_admin_sq_create(aq_ref, &this->sq, &this->cq, qp_id, this->sq_mem.get(), 0, sq_size, sq_need_prp);
        if (!nvm_ok(status))
        {
            throw error(string("Failed to create submission queue: ") + nvm_strerror(status));
        }

        // SQ 도어벨 레지스터의 GPU 디바이스 포인터를 얻는다
        err = cudaHostGetDevicePointer(&devicePtr, (void*) this->sq.db, 0);
        if (err != cudaSuccess)
        {
            throw error(string("Failed to get device pointer") + cudaGetErrorString(err));
        }
        // SQ 도어벨을 GPU 디바이스 포인터로 교체한다
        this->sq.db = (volatile uint32_t*) devicePtr;

        // GPU 병렬 접근용 동기화 구조체(tickets, marks, cid 등)를 초기화한다
        init_gpu_specific_struct(cudaDevice);
        return;



    }

};
#endif
