/*
 * BaM Array Access Benchmark (array/main.cu)
 *
 * 이 벤치마크는 BaM의 array_t<uint64_t> 추상화를 사용하여 NVMe SSD의 대규모 배열에
 * GPU에서 직접 접근하는 성능을 테스트한다. array_t는 page_cache 위에 구축된 배열 추상화로,
 * GPU 스레드가 배열 인덱스로 접근하면 내부적으로 페이지 캐시를 통해 NVMe I/O가 수행된다.
 *
 * 테스트 모드:
 *   - 순차 접근: 스레드 tid에 해당하는 원소를 순서대로 접근
 *   - 랜덤 접근: 미리 생성된 랜덤 인덱스 배열을 사용하여 접근
 *
 * 핵심 데이터 흐름:
 *   GPU 스레드 → array_t::operator[] → page_cache acquire/release → NVMe Read → 데이터 반환
 */
#include <cuda.h>
#include <nvm_ctrl.h>
#include <nvm_types.h>
#include <nvm_queue.h>
#include <nvm_util.h>
#include <nvm_admin.h>
#include <nvm_error.h>
#include <nvm_cmd.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ctrl.h>
#include <buffer.h>
#include "settings.h"
#include <event.h>
#include <queue.h>
#include <nvm_parallel_queue.h>
#include <nvm_io.h>
#include <page_cache.h>
#include <util.h>
#include <iostream>
#include <fstream>
#ifdef __DIS_CLUSTER__
#include <sisci_api.h>
#endif

using error = std::runtime_error;
using std::string;



//uint32_t n_ctrls = 1;
const char* const sam_ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm4", "/dev/libnvm9", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8"};
const char* const intel_ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};


/*
 * sequential_access_kernel - array_t를 통한 순차 배열 접근 커널
 *
 * @dr: GPU 디바이스 측 array_d_t 포인터 (page_cache 기반 배열 추상화)
 * @n_reqs: 총 스레드 수
 * @req_count: 읽은 값의 누적 합 (결과 검증용, 실제로는 포인터에 더하므로 의미 없음)
 * @reqs_per_thread: 스레드당 반복 접근 횟수
 *
 * 각 스레드가 자신의 tid를 인덱스로 array_t의 원소를 접근한다.
 * (*dr)[tid]는 내부적으로 page_cache의 acquire → NVMe Read → release를 수행한다.
 */
__global__
void sequential_access_kernel(array_d_t<uint64_t>* dr, uint64_t n_reqs, unsigned long long* req_count, uint64_t reqs_per_thread) {

    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n_reqs) {
        for (size_t i = 0; i < reqs_per_thread; i++)
            req_count += (*dr)[(tid)];

    }

}

/*
 * random_access_kernel - array_t를 통한 랜덤 배열 접근 커널
 *
 * @dr: GPU 디바이스 측 array_d_t 포인터
 * @n_reqs: 총 스레드 수
 * @req_count: 읽은 값의 누적 합
 * @assignment: 미리 생성된 랜덤 인덱스 배열 (호스트에서 rand()로 생성 후 GPU에 복사)
 * @reqs_per_thread: 스레드당 반복 접근 횟수
 *
 * 랜덤 접근은 페이지 캐시 미스율이 높아 NVMe I/O가 빈번하게 발생하여, 순차 접근 대비 성능 차이를 측정할 수 있다.
 */
__global__
void random_access_kernel(array_d_t<uint64_t>* dr, uint64_t n_reqs, unsigned long long* req_count, uint64_t* assignment, uint64_t reqs_per_thread) {

    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n_reqs) {
        for (size_t i = 0; i < reqs_per_thread; i++)
            req_count += (*dr)[(assignment[tid])];

    }

}

/*
 * main() - BaM Array Access 벤치마크 진입점
 *
 * 흐름:
 * 1. Settings 파싱 (GPU, NVMe, 캐시, 스레드 설정)
 * 2. NVMe 컨트롤러 초기화 (Samsung/Intel SSD 선택)
 * 3. page_cache 생성 → range_t 생성 → array_t 생성 (배열 추상화 계층)
 * 4. 순차/랜덤 커널 실행 후 IOPS, 대역폭 측정
 */
int main(int argc, char** argv) {

    Settings settings;
    try
    {
        settings.parseArguments(argc, argv);
    }
    catch (const string& e)
    {
        fprintf(stderr, "%s\n", e.c_str());
        fprintf(stderr, "%s\n", Settings::usageString(argv[0]).c_str());
        return 1;
    }


    cudaDeviceProp properties;
    if (cudaGetDeviceProperties(&properties, settings.cudaDevice) != cudaSuccess)
    {
        fprintf(stderr, "Failed to get CUDA device properties\n");
        return 1;
    }

    try {
        //Controller ctrl(settings.controllerPath, settings.nvmNamespace, settings.cudaDevice);
        
        cuda_err_chk(cudaSetDevice(settings.cudaDevice));
        std::vector<Controller*> ctrls(settings.n_ctrls);
        for (size_t i = 0 ; i < settings.n_ctrls; i++)
            ctrls[i] = new Controller(settings.ssdtype == 0 ? sam_ctrls_paths[i] : intel_ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);

        //auto dma = createDma(ctrl.ctrl, NVM_PAGE_ALIGN(64*1024*10, 1UL << 16), settings.cudaDevice, settings.adapter, settings.segmentId);

        //std::cout << dma.get()->vaddr << std::endl;
        //QueuePair h_qp(ctrl, settings, 1);
        //std::cout << "in main: " << std::hex << h_qp.sq.cid << "raw: " << h_qp.sq.cid<< std::endl;
        //std::memset(&h_qp, 0, sizeof(QueuePair));
        //prepareQueuePair(h_qp, ctrl, settings, 1);
        //const uint32_t ps, const uint64_t np, const uint64_t c_ps, const Settings& settings, const Controller& ctrl)
        //
        /*
        Controller** d_ctrls;
        cuda_err_chk(cudaMalloc(&d_ctrls, n_ctrls*sizeof(Controller*)));
        for (size_t i = 0; i < n_ctrls; i++)
            cuda_err_chk(cudaMemcpy(d_ctrls+i, &(ctrls[i]->d_ctrl), sizeof(Controller*), cudaMemcpyHostToDevice));
        */
        uint64_t b_size = settings.blkSize;//64;
        uint64_t g_size = (settings.numThreads + b_size - 1)/b_size;//80*16;
        uint64_t n_threads = b_size * g_size;


        uint64_t page_size = settings.pageSize;
        uint64_t n_pages = settings.numPages;
        uint64_t total_cache_size = (page_size * n_pages);
        //uint64_t n_pages = total_cache_size/page_size;


        /* GPU 페이지 캐시 생성: n_pages개의 page_size 크기 페이지를 GPU VRAM에 할당 */
        page_cache_t h_pc(page_size, n_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
        std::cout << "finished creating cache\n";

        //QueuePair* d_qp;
        page_cache_t* d_pc = (page_cache_t*) (h_pc.d_pc_ptr);
        #define TYPE uint64_t
        uint64_t n_elems = settings.numElems;
        uint64_t t_size = n_elems * sizeof(TYPE);

        /* range_t: page_cache 위의 연속된 원소 범위를 나타내는 추상화. 원소 인덱스 → 페이지 번호 변환을 담당 */
        range_t<uint64_t> h_range((uint64_t)0, (uint64_t)n_elems, (uint64_t)0, (uint64_t)(t_size/page_size), (uint64_t)0, (uint64_t)page_size, &h_pc, settings.cudaDevice);
        range_t<uint64_t>* d_range = (range_t<uint64_t>*) h_range.d_range_ptr;

        std::vector<range_t<uint64_t>*> vr(1);
        vr[0] = & h_range;
        /* array_t: range_t 위에 구축된 배열 추상화. operator[]로 원소 접근 시 자동으로 NVMe I/O 수행 */
        array_t<uint64_t> a(n_elems, 0, vr, settings.cudaDevice);


        std::cout << "finished creating range\n";




        unsigned long long* d_req_count;
        cuda_err_chk(cudaMalloc(&d_req_count, sizeof(unsigned long long)));
        cuda_err_chk(cudaMemset(d_req_count, 0, sizeof(unsigned long long)));
        std::cout << "atlaunch kernel\n";
        char st[15];
        cuda_err_chk(cudaDeviceGetPCIBusId(st, 15, settings.cudaDevice));
        std::cout << st << std::endl;
        uint64_t* assignment;
        uint64_t* d_assignment;
        if (settings.random) {
            assignment = (uint64_t*) malloc(n_threads*sizeof(uint64_t));
            for (size_t i = 0; i< n_threads; i++)
                assignment[i] = rand() % (n_elems);


            cuda_err_chk(cudaMalloc(&d_assignment, n_threads*sizeof(uint64_t)));
            cuda_err_chk(cudaMemcpy(d_assignment, assignment,  n_threads*sizeof(uint64_t), cudaMemcpyHostToDevice));
        }
        Event before;
        //access_kernel<<<g_size, b_size>>>(h_pc.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs);
        if (settings.random)
            random_access_kernel<<<g_size, b_size>>>(a.d_array_ptr, n_threads, d_req_count, d_assignment, settings.numReqs);
        else
            sequential_access_kernel<<<g_size, b_size>>>(a.d_array_ptr, n_threads, d_req_count, settings.numReqs);
        Event after;
        //new_kernel<<<1,1>>>();
        //uint8_t* ret_array = (uint8_t*) malloc(n_pages*page_size);

        //cuda_err_chk(cudaMemcpy(ret_array, h_pc.base_addr,page_size*n_pages, cudaMemcpyDeviceToHost));
        cuda_err_chk(cudaDeviceSynchronize());


        double elapsed = after - before;
        uint64_t ios = g_size*b_size*settings.numReqs;
        uint64_t data = ios*sizeof(uint64_t);
        double iops = ((double)ios)/(elapsed/1000000);
        double bandwidth = (((double)data)/(elapsed/1000000))/(1024ULL*1024ULL*1024ULL);
        a.print_reset_stats();
        std::cout << std::dec << "Elapsed Time: " << elapsed << "\tNumber of Read Ops: "<< ios << "\tData Size (bytes): " << data << std::endl;
        std::cout << std::dec << "Read Ops/sec: " << iops << "\tEffective Bandwidth(GB/S): " << bandwidth << std::endl;

        //std::cout << std::dec << ctrls[0]->ns.lba_data_size << std::endl;

        //std::ofstream ofile("../data", std::ios::binary | std::ios::trunc);
        //ofile.write((char*)ret_array, data);
        //ofile.close();

        for (size_t i = 0 ; i < settings.n_ctrls; i++)
            delete ctrls[i];
        //hexdump(ret_array, n_pages*page_size);
/*
        cudaFree(d_qp);
        cudaFree(d_pc);
        cudaFree(d_req_count);
        free(ret_array);
*/

        //std::cout << "END\n";

        //std::cout << RAND_MAX << std::endl;

    }
    catch (const error& e) {
        fprintf(stderr, "Unexpected error: %s\n", e.what());
        return 1;
    }



}
