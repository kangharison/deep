/*
 * =============================================================================
 * BaM BFS 벤치마크 - 설정(Settings) 헤더
 * =============================================================================
 *
 * BFS 벤치마크의 모든 커맨드라인 옵션을 파싱하고 저장하는 구조체를 정의한다.
 * getopt_long 기반으로 옵션을 처리하며, 각 옵션은 Option/Range 템플릿으로 타입 안전하게 파싱된다.
 *
 * [주요 설정 항목]
 * - cudaDevice: 사용할 GPU 디바이스 번호
 * - input (-f): 그래프 파일 경로 (.bel 형식, 내부적으로 .col/.dst 파일을 읽음)
 * - type (-v): 커널 구현 방식 (COALESCE=1, COALESCE_CHUNK=2, COALESCE_PC=4, COALESCE_CHUNK_PC=5 등)
 * - memalloc (-m): 메모리 할당 방식 (GPUMEM=0, UVM_READONLY=1, UVM_DIRECT=2, BAFS_DIRECT=6)
 * - pageSize (-p): BaM page cache의 페이지 크기 (기본 4096바이트)
 * - maxPageCacheSize (-M): BaM page cache의 최대 크기 (기본 8GB)
 * - n_ctrls (-k): 사용할 NVMe 컨트롤러 수
 * - queueDepth (-d): NVMe 큐당 depth (기본 1024)
 * - numQueues (-q): 컨트롤러당 NVMe 큐 수 (기본 128)
 * - numThreads (-t): CUDA 커널 스레드 수 (기본 1024)
 * - blkSize (-b): CUDA 스레드 블록 크기 (기본 64)
 * - stride (-P): 해시 기반 커널의 stride 계수 (워프-정점 매핑 분산)
 * - coarse (-C): 스레드 coarsening 계수 (워프당 처리 정점 수)
 * - tsize (-T): 캐시라인 인식 타일 크기 (기본 4096)
 * - repeat (-r): 랜덤 소스 반복 횟수 (기본 32)
 * - src (-s): 시작 정점 (0이면 자동 분배)
 * - ofileoffset (-l): SSD 내 그래프 데이터 오프셋 위치
 * - ssdtype (-S): SSD 종류 (0=Samsung, 1=Intel)
 *
 * [PCIe BDF]
 * setBDF() 함수가 cudaDeviceProp에서 GPU의 PCIe Bus/Device/Function 정보를 읽어
 * NVMe 컨트롤러와 P2P DMA 설정에 사용한다.
 * =============================================================================
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
    uint32_t        cudaDevice;      // 사용할 GPU 디바이스 번호 (-g 옵션)
    uint64_t        cudaDeviceId;    // CUDA 디바이스 ID (내부용)
    const char*     blockDevicePath; // NVMe 블록 디바이스 경로 (미사용, 레거시)
    const char*     controllerPath;  // NVMe 컨트롤러 경로 (미사용, 레거시)
    uint64_t        controllerId;    // NVMe 컨트롤러 ID (미사용, 레거시)
    uint32_t        adapter;         // DIS 어댑터 번호 (미사용, 레거시)
    uint32_t        segmentId;       // DIS 세그먼트 ID (미사용, 레거시)
    uint32_t        nvmNamespace;    // NVMe 네임스페이스 번호 (기본 1)
    bool            doubleBuffered;  // 더블 버퍼링 여부 (미사용)
    size_t          numReqs;         // 스레드당 요청 수 (미사용)
    size_t          numPages;        // 캐시 페이지 수 (미사용, maxPageCacheSize로 대체)
    size_t          startBlock;      // 시작 블록 (미사용)
    bool            stats;           // 통계 출력 여부 (미사용)
    const char*     input;           // 그래프 파일 경로 (-f 옵션, .bel 경로)
    const char*     output;          // 출력 파일 경로 (미사용)
    size_t          ofileoffset;     // SSD 내 그래프 데이터 시작 오프셋 (-l 옵션)
    size_t          type;            // 커널 구현 타입 (-v 옵션, impl_type enum 참조)
    size_t          memalloc;        // 메모리 할당 방식 (-m 옵션, mem_type enum 참조)
    size_t          numThreads;      // CUDA 커널 총 스레드 수 (-t 옵션)
    uint32_t        domain;          // GPU PCIe 도메인 ID (자동 설정)
    uint32_t        bus;             // GPU PCIe 버스 ID (자동 설정)
    uint32_t        devfn;           // GPU PCIe 디바이스 ID (자동 설정)
    uint64_t stride;                 // 해시 커널의 stride 계수 (-P 옵션, 워프 분산용)
    uint64_t coarse;                 // 스레드 coarsening 계수 (-C 옵션, 워프당 정점 수)
    uint64_t tsize;                  // 캐시라인 인식 타일 크기 (-T 옵션, 0이면 pageSize 사용)
    uint32_t n_ctrls;                // NVMe 컨트롤러 수 (-k 옵션)
    size_t blkSize;                  // CUDA 스레드 블록 크기 (-b 옵션)
    size_t queueDepth;               // NVMe 큐 depth (-d 옵션)
    size_t numQueues;                // 컨트롤러당 NVMe 큐 수 (-q 옵션)
    size_t pageSize;                 // BaM page cache 페이지 크기 (-p 옵션, 바이트)
    uint64_t numElems;               // backing array 원소 수 (미사용)
    size_t repeat;                   // 반복 실행 횟수 (-r 옵션)
    size_t src;                      // BFS 시작 정점 (-s 옵션, 0이면 자동 분배)
    uint64_t maxPageCacheSize;       // BaM page cache 최대 크기 (-M 옵션, 바이트, 기본 8GB)
    uint64_t ssdtype;                // SSD 종류 (-S 옵션, 0=Samsung, 1=Intel)
    Settings();
    void parseArguments(int argc, char** argv);

    static std::string usageString(const std::string& name);

    std::string getDeviceBDF() const;
};


/* OptionIface / Option<T> / Range: 커맨드라인 옵션 파싱 프레임워크
 * - OptionIface: 옵션 파싱의 인터페이스 (이름, 타입, 설명, 기본값)
 * - Option<T>: 타입별 특수화로 문자열을 uint32_t/uint64_t/bool/const char*로 변환
 * - Range: uint64_t 값에 상한/하한 범위 검증을 추가한 파서
 */
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



/* setBDF: GPU의 PCIe Bus/Device/Function 정보를 cudaDeviceProp에서 읽어 Settings에 저장한다.
 * 이 정보는 BaM이 GPU-NVMe 간 P2P DMA 경로를 설정할 때 사용된다. */
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


/* getDeviceBDF: PCIe BDF를 "0000:00:00.0" 형식 문자열로 반환한다. */
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



/* parseArguments: 커맨드라인 인자를 파싱하여 Settings 구조체 멤버에 저장한다.
 * 각 옵션의 단축키와 설명은 아래 parsers 맵에 정의되어 있다. */
void Settings::parseArguments(int argc, char** argv)
{
    OptionMap parsers = {
        {'f', OptionPtr(new Option<const char*>(input, "path", "input", "File path where the vertex file is. Provide .bel path."))},
        {'l', OptionPtr(new Range(ofileoffset, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "loffset", "Offset where the input file contents need to be stored in NVMe SSD", "0"))},
        {'v', OptionPtr(new Range(type, 0, 30, "impl_type", "COALESCE = 1, COALESCE_CHUNK = 2, COALESCE_PC = 4, COALESCE_CHUNK_PC = 5", "1"))},
        {'m', OptionPtr(new Range(memalloc, 0, 6, "memalloc", "GPUMEM = 0, UVM_READONLY = 1, UVM_DIRECT = 2, BAFS_DIRECT = 6", "2"))},
        {'r', OptionPtr(new Range(repeat, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "repeat", "number of random source iteration to run", "32"))},
        {'s', OptionPtr(new Range(src, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "src", "start node of the graph", "0"))},

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
        {'P', OptionPtr(new Option<uint64_t>(stride, "number", "STRIDE", "Hashing stride factor for bfs coal. It is calculated as P = stride. Assumes power of 2", "1"))},
        {'C', OptionPtr(new Option<uint64_t>(coarse, "number", "COARSE", "Thread coarsening factor", "1"))},
        {'T', OptionPtr(new Option<uint64_t>(tsize, "number", "TileSize", "CLAware tile size", "4096"))},
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


/* Settings 기본 생성자: 모든 멤버를 기본값으로 초기화한다. */
Settings::Settings()
{
    cudaDevice = 8;
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
    stride = 1;
    coarse = 1;
    tsize = 0;
    maxPageCacheSize = 8589934592;
    ssdtype = 0;
}




#endif
