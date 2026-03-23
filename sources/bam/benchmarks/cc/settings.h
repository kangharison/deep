/*
 * [BaM CC 벤치마크 - 설정 헤더]
 *
 * Connected Components(연결 요소, Label Propagation) 그래프 알고리즘 벤치마크의 설정 파일.
 * 커맨드라인 옵션을 파싱하여 CUDA 디바이스, NVMe 컨트롤러, 페이지 캐시,
 * 커널 구현 방식(baseline/coalesce/hash 등), 메모리 할당 방식(GPUMEM/UVM/BAFS) 등을 설정한다.
 *
 * 그래프는 CSR(Compressed Sparse Row) 포맷으로 저장됨:
 *   - .col 파일: vertexList (각 정점의 인접 리스트 시작 오프셋, 크기 vertex_count+1)
 *   - .dst 파일: edgeList (간선의 목적지 정점 ID, 크기 edge_count)
 *   vertexList[v] ~ vertexList[v+1] 범위가 정점 v의 이웃 정점들의 edgeList 인덱스이다.
 *
 * Settings 구조체 주요 필드 설명:
 *   cudaDevice       - 사용할 CUDA GPU 디바이스 번호
 *   blockDevicePath  - NVMe 블록 디바이스 경로 (예: /dev/nvme0n1)
 *   controllerPath   - NVMe 컨트롤러 경로 (예: /dev/libnvm0)
 *   nvmNamespace     - NVMe 네임스페이스 번호 (기본값 1)
 *   input            - 입력 그래프 파일 경로 (.bel 확장자)
 *   ofileoffset      - NVMe SSD에 저장할 때의 파일 오프셋 (바이트 단위)
 *   type             - 커널 구현 타입 (impl_type 열거형: 0=BASELINE ~ 27=OPTIMIZED_PC)
 *   memalloc         - 메모리 할당 방식 (0=GPUMEM, 1=UVM_READONLY, 2=UVM_DIRECT, 6=BAFS_DIRECT)
 *   numThreads       - CUDA 커널의 스레드 블록당 스레드 수
 *   n_ctrls          - 사용할 NVMe 컨트롤러 수 (멀티 SSD 지원)
 *   queueDepth       - NVMe 큐당 depth (동시 I/O 요청 수)
 *   numQueues        - 컨트롤러당 큐 수
 *   pageSize         - BaM 페이지 캐시의 페이지 크기 (바이트, 기본 4096)
 *   maxPageCacheSize - GPU 메모리에 할당되는 BaM 페이지 캐시 최대 크기 (바이트, 기본 8GB)
 *   stride           - 해싱 스트라이드 팩터 (SM 간 정점 분배를 위한 인터리빙 간격)
 *   coarse           - 스레드 코어스닝 팩터 (하나의 워프가 처리할 정점 수)
 *   largebin         - 이웃 크기 히스토그램의 빈(bin) 수
 *   binelems         - 빈당 원소 수 (이웃 크기를 빈으로 나눌 때의 단위)
 *   ssdtype          - SSD 종류 (0=Samsung, 1=Intel)
 *
 * OptionIface / Option<T> / Range:
 *   getopt_long 기반의 커맨드라인 파서. 각 옵션을 타입-세이프하게 파싱하며,
 *   Range는 값의 유효 범위를 검증한다.
 *
 * setBDF(): CUDA 디바이스의 PCIe Bus/Device/Function 정보를 자동 감지한다.
 *   BaM이 GPU와 NVMe SSD 간 P2P DMA를 설정할 때 PCIe 토폴로지 정보가 필요하다.
 */
#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>
#include <cstddef>
#include <cstdint>
#include <cuda.h>
#include "settings.h"
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <getopt.h>
#include <limits>


struct Settings
{
    uint32_t        cudaDevice;      // 사용할 CUDA GPU 디바이스 번호
    uint64_t        cudaDeviceId;    // CUDA 디바이스 ID (내부용)
    const char*     blockDevicePath; // NVMe 블록 디바이스 경로
    const char*     controllerPath;  // NVMe 컨트롤러 경로
    uint64_t        controllerId;    // 컨트롤러 식별자 (DIS 클러스터용)
    uint32_t        adapter;         // 네트워크 어댑터 번호 (DIS 클러스터용)
    uint32_t        segmentId;       // 세그먼트 ID (DIS 클러스터용)
    uint32_t        nvmNamespace;    // NVMe 네임스페이스 번호 (기본값 1)
    bool            doubleBuffered;  // 더블 버퍼링 사용 여부
    size_t          numReqs;         // 스레드당 I/O 요청 수
    size_t          numPages;        // 페이지 캐시 엔트리 수
    size_t          startBlock;      // 시작 블록 번호
    bool            stats;           // 통계 출력 여부
    const char*     input;           // 입력 그래프 파일 경로 (.bel 확장자)
    const char*     output;          // 출력 파일 경로
    size_t          ofileoffset;     // NVMe SSD 내 파일 오프셋 (바이트)
    size_t          type;            // 커널 구현 타입 (impl_type: 0=BASELINE ~ 27=OPTIMIZED_PC)
    size_t          memalloc;        // 메모리 할당 방식 (0=GPUMEM, 1=UVM_READONLY, 2=UVM_DIRECT, 6=BAFS_DIRECT)
    size_t          numThreads;      // CUDA 블록당 스레드 수
    uint32_t        domain;          // GPU PCIe 도메인 ID (자동 감지)
    uint32_t        bus;             // GPU PCIe 버스 ID (자동 감지)
    uint32_t        devfn;           // GPU PCIe 디바이스/함수 ID (자동 감지)
    uint32_t n_ctrls;                // NVMe 컨트롤러 수 (멀티 SSD)
    size_t blkSize;                  // CUDA 스레드 블록 크기
    size_t queueDepth;               // NVMe 큐당 depth (동시 I/O 수)
    size_t numQueues;                // 컨트롤러당 큐 수
    size_t pageSize;                 // BaM 페이지 캐시 페이지 크기 (바이트)
    uint64_t numElems;               // 배열의 64비트 원소 수
    size_t repeat;                   // 랜덤 소스 반복 실행 횟수
    size_t src;                      // 그래프 시작 노드
    uint64_t maxPageCacheSize;       // BaM 페이지 캐시 최대 크기 (바이트, 기본 8GB)
    uint64_t stride;                 // 해싱 스트라이드 (SM 간 정점 인터리빙 간격)
    uint64_t coarse;                 // 스레드 코어스닝 팩터 (워프당 처리 정점 수)
    uint64_t largebin;               // 이웃 크기 히스토그램 빈 수
    uint64_t binelems;               // 빈당 원소 수
    uint64_t ssdtype;                // SSD 종류 (0=Samsung, 1=Intel)
    Settings();
    void parseArguments(int argc, char** argv);

    static std::string usageString(const std::string& name);

    std::string getDeviceBDF() const;
};


/* OptionIface: 커맨드라인 옵션 인터페이스 (추상 클래스) */
struct OptionIface;
using std::string;
using std::vector;
using std::make_shared;
typedef std::shared_ptr<OptionIface>  OptionPtr;
typedef std::map<int, OptionPtr> OptionMap;


struct OptionIface
{
    const char*     type;
    const char*     name;
    const char*     description;
    const char*     defaultValue;
    int             hasArgument;

    virtual ~OptionIface() = default;

    OptionIface(const char* type, const char* name, const char* description)
        : type(type), name(name), description(description), hasArgument(no_argument) { }

    OptionIface(const char* type, const char* name, const char* description, const char* dvalue)
        : type(type), name(name), description(description), defaultValue(dvalue), hasArgument(no_argument) { }

    virtual void parseArgument(const char* optstr, const char* optarg) = 0;

    virtual void throwError(const char*, const char* optarg) const
    {
        throw string("Option ") + name + string(" expects a ") + type + string(", but got `") + optarg + string("'");
    }
};


/* Option<T>: 타입별 옵션 파서 (uint32_t, uint64_t, bool, const char* 특수화) */
template <typename T>
struct Option: public OptionIface
{
    T&              value;

    Option() = delete;
    Option(Option&& rhs) = delete;
    Option(const Option& rhs) = delete;

    Option(T& value, const char* type, const char* name, const char* description)
        : OptionIface(type, name, description)
        , value(value)
    {
        hasArgument = required_argument;
    }

    Option(T& value, const char* type, const char* name, const char* description, const char* dvalue)
        : OptionIface(type, name, description, dvalue)
        , value(value)
    {
        hasArgument = required_argument;
    }

    void parseArgument(const char* optstr, const char* optarg) override;
};


template <>
void Option<uint32_t>::parseArgument(const char* optstr, const char* optarg)
{
    char* endptr = nullptr;

    value = std::strtoul(optarg, &endptr, 0);

    if (endptr == nullptr || *endptr != '\0')
    {
        throwError(optstr, optarg);
    }
}


template <>
void Option<uint64_t>::parseArgument(const char* optstr, const char* optarg)
{
    char* endptr = nullptr;

    value = std::strtoul(optarg, &endptr, 0);

    if (endptr == nullptr || *endptr != '\0')
    {
        throwError(optstr, optarg);
    }
}


template <>
void Option<bool>::parseArgument(const char* optstr, const char* optarg)
{
    string str(optarg);
    std::transform(str.begin(), str.end(), str.begin(), std::ptr_fun<int, int>(std::tolower));

    if (str == "false" || str == "0" || str == "no" || str == "n" || str == "off" || str == "disable" || str == "disabled")
    {
        value = false;
    }
    else if (str == "true" || str == "1" || str == "yes" || str == "y" || str == "on" || str == "enable" || str == "enabled")
    {
        value = true;
    }
    else
    {
        throwError(optstr, optarg);
    }
}


template <>
void Option<const char*>::parseArgument(const char* optstr, const char* optarg)
{
    if(optarg == nullptr){
            throwError(optstr, optarg);
    }
    value = optarg;
}


/* Range: uint64_t 값의 유효 범위를 검증하는 옵션 파서 */
struct Range: public Option<uint64_t>
{
    uint64_t      lower;
    uint64_t      upper;

    Range(uint64_t& value, uint64_t lo, uint64_t hi, const char* name, const char* description, const char* dv)
        : Option<uint64_t>(value, "count", name, description, dv)
        , lower(lo)
        , upper(hi)
    { }

    void throwError(const char*, const char*) const override
    {
        if (upper != 0 && lower != 0)
        {
            throw string("Option ") + name + string(" expects a value between ") + std::to_string(lower) + " and " + std::to_string(upper);
        }
        else if (lower != 0)
        {
            throw string("Option ") + name + string(" must be at least ") + std::to_string(lower);
        }
        throw string("Option ") + name + string(" must lower than ") + std::to_string(upper);
    }

    void parseArgument(const char* optstr, const char* optarg) override
    {
        Option<uint64_t>::parseArgument(optstr, optarg);

        if (lower != 0 && value < lower)
        {
            throwError(optstr, optarg);
        }

        if (upper != 0 && value > upper)
        {
            throwError(optstr, optarg);
        }
    }
};



/* setBDF(): GPU의 PCIe BDF(Bus/Device/Function) 정보를 cudaDeviceProp에서 가져온다.
 * BaM의 libnvm 드라이버가 GPU-SSD 간 P2P DMA 경로를 설정할 때 이 정보를 사용한다. */
static void setBDF(Settings& settings)
{
    cudaDeviceProp props;

    cudaError_t err = cudaGetDeviceProperties(&props, settings.cudaDevice);
    if (err != cudaSuccess)
    {
        throw string("Failed to get device properties: ") + cudaGetErrorString(err);
    }

    settings.domain = props.pciDomainID;
    settings.bus = props.pciBusID;
    settings.devfn = props.pciDeviceID;
}


/* getDeviceBDF(): PCIe BDF를 "DDDD:BB:DD.0" 형식의 문자열로 반환한다. */
string Settings::getDeviceBDF() const
{
    using namespace std;
    ostringstream s;

    s << setfill('0') << setw(4) << hex << domain
        << ":" << setfill('0') << setw(2) << hex << bus
        << ":" << setfill('0') << setw(2) << hex << devfn
        << ".0";

    return s.str();
}



string Settings::usageString(const string& name)
{
    //return "Usage: " + name + " --ctrl=identifier [options]\n"
        //+  "   or: " + name + " --block-device=path [options]";
    return "\n";
}



static string helpString(const string& /*name*/, OptionMap& options)
{
    using namespace std;
    ostringstream s;

    s << "" << left
        << setw(16) << "OPTION"
        << setw(2) << " "
        << setw(16) << "TYPE"
        << setw(10) << "DEFAULT"
        << setw(36) << "DESCRIPTION"
        << endl;

    for (const auto& optPair: options)
    {
        const auto& opt = optPair.second;
        s << "  " << left
            << setw(16) << opt->name
            << setw(16) << opt->type
            << setw(10) << (opt->defaultValue != nullptr ? opt->defaultValue : "")
            << setw(36) << opt->description
            << endl;
    }

    return s.str();
}


static void createLongOptions(vector<option>& options, string& optionString, const OptionMap& parsers)
{
    options.push_back(option{ .name = "help", .has_arg = no_argument, .flag = nullptr, .val = 'h' });
    optionString = ":h";

    for (const auto& parserPair: parsers)
    {
        int shortOpt = parserPair.first;
        const OptionPtr& parser = parserPair.second;

        option opt;
        opt.name = parser->name;
        opt.has_arg = parser->hasArgument;
        opt.flag = nullptr;
        opt.val = shortOpt;

        options.push_back(opt);

        if ('0' <= shortOpt && shortOpt <= 'z')
        {
            optionString += (char) shortOpt;
            if (parser->hasArgument == required_argument)
            {
                optionString += ":";
            }
        }
    }

    options.push_back(option{ .name = nullptr, .has_arg = 0, .flag = nullptr, .val = 0 });
}


static void verifyCudaDevice(int device)
{
    int deviceCount = 0;

    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess)
    {
        throw string("Unexpected error: ") + cudaGetErrorString(err);
    }

    if (device < 0 || device >= deviceCount)
    {
        throw string("Invalid CUDA device: ") + std::to_string(device);
    }
}


static void verifyNumberOfThreads(size_t numThreads)
{
    size_t i = 0;

    while ((1ULL << i) <= 32)
    {
        if ((1ULL << i) == numThreads)
        {
            return;
        }

        ++i;
    }

    throw string("Invalid number of threads, must be a power of 2");
}



/* parseArguments(): 커맨드라인 옵션 파싱. 주요 옵션:
 *   -f : 입력 그래프 파일 경로 (.bel)
 *   -v : 커널 구현 타입 (BASELINE=0, COALESCE=1, ..., OPTIMIZED_PC=27)
 *   -m : 메모리 할당 (GPUMEM=0, UVM_READONLY=1, UVM_DIRECT=2, BAFS_DIRECT=6)
 *   -g : CUDA GPU 디바이스 번호
 *   -k : NVMe 컨트롤러 수
 *   -p : 페이지 캐시 페이지 크기 (바이트)
 *   -d : NVMe 큐 depth
 *   -q : 컨트롤러당 큐 수
 *   -M : 페이지 캐시 최대 크기 (바이트)
 *   -P : 해싱 스트라이드 팩터
 *   -C : 스레드 코어스닝 팩터
 *   -B : 히스토그램 빈 수
 *   -E : 빈당 원소 수
 *   -S : SSD 타입 (0=Samsung, 1=Intel)
 */
void Settings::parseArguments(int argc, char** argv)
{
    OptionMap parsers = {
        {'f', OptionPtr(new Option<const char*>(input, "path", "input", "File path where the vertex file is. Provide .bel path."))},
        {'l', OptionPtr(new Range(ofileoffset, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "loffset", "Offset where the input file contents need to be stored in NVMe SSD", "0"))},
        {'v', OptionPtr(new Range(type, 0, 50, "impl_type", "BASELINE=0, COALESCE = 1, COALESCE_CHUNK = 2, BASELINE_PC=3, COALESCE_PC = 4, COALESCE_CHUNK_PC = 5\n BASELINE_HASH = 6, COALESCE_HASH = 7, BASELINE_HASH_PC = 9, COALESCE_HASH_PC = 10", "1"))},
        {'m', OptionPtr(new Range(memalloc, 0, 6, "memalloc", "GPUMEM = 0, UVM_READONLY = 1, UVM_DIRECT = 2, BAFS_DIRECT = 6", "2"))},
        {'r', OptionPtr(new Range(repeat, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "repeat", "number of random source iteration to run", "32"))},
        {'s', OptionPtr(new Range(src, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "src", "start node of the graph", "0"))},

        {'t', OptionPtr(new Range(numThreads, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "threads", "number of CUDA threads", "1024"))},
        {'b', OptionPtr(new Range(blkSize, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "blk_size", "CUDA thread block size", "64"))},

        {'g', OptionPtr(new Option<uint32_t>(cudaDevice, "number", "gpu", "specify CUDA device", "0"))},
        {'k', OptionPtr(new Option<uint32_t>(n_ctrls, "number", "n_ctrls", "specify number of NVMe controllers", "1"))},
//        {'n', OptionPtr(new Range(numReqs, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "reqs", "number of reqs per thread", "1"))},
//        {'p', OptionPtr(new Range(numPages, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "pages", "number of pages in cache", "1024"))},
        {'p', OptionPtr(new Range(pageSize, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "page_size", "size of page in cache", "4096"))},
        {'d', OptionPtr(new Range(queueDepth, 2, 65536, "queue_depth", "queue depth per queue", "16"))},
        {'q', OptionPtr(new Range(numQueues, 1, 65536, "num_queues", "number of queues per controller", "1"))},
        {'M', OptionPtr(new Option<uint64_t>(maxPageCacheSize, "number", "maxPCSize", "Maximum Page Cache size in bytes", "8589934592"))},
        {'P', OptionPtr(new Option<uint64_t>(stride, "number", "STRIDE", "Hashing stride factor for cc. It is calculated as P = stride. Assumes power of 2", "1"))},
        {'C', OptionPtr(new Option<uint64_t>(coarse, "number", "COARSE", "Thread coarsening factor", "1"))},
        {'B', OptionPtr(new Option<uint64_t>(largebin, "number", "largebin", "Number of bins", "128"))},
        {'E', OptionPtr(new Option<uint64_t>(binelems, "number", "binelems", "Number of elems per bin", "1"))},
//        {'e', OptionPtr(new Range(numElems, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "num_elems", "number of 64-bit elements in backing array", "2147483648"))},
        {'S', OptionPtr(new Range(ssdtype, 0, 2, "ssd", "type of SSD to use 0->Samsung, 1->Intel", "0"))},
    };

    string optionString;
    vector<option> options;
    createLongOptions(options, optionString, parsers);

    int index;
    int option;
    OptionMap::iterator parser;

    while ((option = getopt_long(argc, argv, optionString.c_str(), &options[0], &index)) != -1)
    {
        switch (option)
        {
            case '?':
                throw string("Unknown option: `") + argv[optind - 1] + string("'");

            case ':':
                throw string("Missing argument for option `") + argv[optind - 1] + string("'");

            case 'h':
                throw helpString(argv[0], parsers);

            default:
                parser = parsers.find(option);
                if (parser == parsers.end())
                {
                    throw string("Unknown option: `") + argv[optind - 1] + string("'");
                }
                parser->second->parseArgument(argv[optind - 1], optarg);
                break;
        }
    }
/*
#ifdef __DIS_CLUSTER__
    if (blockDevicePath == nullptr && controllerId == 0)
    {
        throw string("No block device or NVM controller specified");
    }
    else if (blockDevicePath != nullptr && controllerId != 0)
    {
        throw string("Either block device or NVM controller must be specified, not both!");
    }
#else
    if (blockDevicePath == nullptr && controllerPath == nullptr)
    {
        throw string("No block device or NVM controller specified");
    }
    else if (blockDevicePath != nullptr && controllerPath != nullptr)
    {
        throw string("Either block device or NVM controller must be specified, not both!");
    }
#endif

    if (blockDevicePath != nullptr && doubleBuffered)
    {
        throw string("Double buffered reading from block device is not supported");
    }
*/
    verifyCudaDevice(cudaDevice);
    //verifyNumberOfThreads(numThreads);

    setBDF(*this);
}


/* Settings 기본값: GPU 0번, 1024 스레드, 페이지 4KB, 큐 depth 1024, 페이지 캐시 8GB */
Settings::Settings()
{
    cudaDevice = 0;
    cudaDeviceId = 0;
    blockDevicePath = nullptr;
    controllerPath = nullptr;
    controllerId = 0;
    adapter = 0;
    segmentId = 0;
    nvmNamespace = 1;
    doubleBuffered = false;
    numReqs = 1;
    numPages = 1024;
    startBlock = 0;
    stats = false;
    input = nullptr;
    output = nullptr;
    ofileoffset = 0;
    type = 1;
    memalloc = 2;
    numThreads = 1024;
    blkSize = 64;
    domain = 0;
    bus = 0;
    devfn = 0;
    n_ctrls = 1;
    queueDepth = 1024;
    numQueues = 128;
    pageSize = 4096;
    numElems = 2147483648;
    repeat = 32;
    src = 0;
    maxPageCacheSize = 8589934592;
    stride = 1;
    coarse = 1;
    largebin = 128;
    binelems = 64;
    ssdtype = 0;
}




#endif
