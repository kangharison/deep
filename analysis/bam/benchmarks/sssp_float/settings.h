/*
 * =============================================================================
 * BaM SSSP_float (부동소수점 가중치) 벤치마크 - 설정(Settings) 헤더
 * =============================================================================
 *
 * SSSP_float 벤치마크의 커맨드라인 옵션 파싱 및 저장 구조체.
 * sssp/settings.h와 구조가 동일하다. WeightT가 float인 SSSP 변형에 사용된다.
 *
 * [sssp/settings.h와 차이점]
 * - 코드 구조 동일. main.cu에서 WeightT를 float으로 정의하여 부동소수점 가중치를 처리한다.
 * - float에 대한 atomicMin이 CUDA에 기본 제공되지 않으므로, main.cu에서 AtomicMin()을 CAS 루프로 구현한다.
 *
 * [주요 설정 항목] (sssp/settings.h 참조)
 * - wfileoffset (-w): SSD 내 가중치 파일 오프셋 (기본 32GB)
 * - ofileoffset (-l): SSD 내 간선 파일 오프셋
 * - 나머지는 sssp/settings.h와 동일
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
    uint32_t        cudaDevice;      // GPU 디바이스 번호 (-g)
    uint64_t        cudaDeviceId;    // CUDA 디바이스 ID (내부용)
    const char*     blockDevicePath; // NVMe 블록 디바이스 경로 (미사용)
    const char*     controllerPath;  // NVMe 컨트롤러 경로 (미사용)
    uint64_t        controllerId;    // NVMe 컨트롤러 ID (미사용)
    uint32_t        adapter;         // DIS 어댑터 (미사용)
    uint32_t        segmentId;       // DIS 세그먼트 (미사용)
    uint32_t        nvmNamespace;    // NVMe 네임스페이스 (기본 1)
    bool            doubleBuffered;  // 더블 버퍼링 (미사용)
    size_t          numReqs;         // 스레드당 요청 수 (미사용)
    size_t          numPages;        // 캐시 페이지 수 (미사용)
    size_t          startBlock;      // 시작 블록 (미사용)
    bool            stats;           // 통계 출력 (미사용)
    const char*     input;           // 그래프 파일 경로 (-f)
    const char*     output;          // 출력 파일 경로 (미사용)
    size_t          ofileoffset;     // SSD 내 간선 오프셋 (-l)
    size_t          wfileoffset;     // SSD 내 가중치 오프셋 (-w, 기본 32GB)
    size_t          type;            // 커널 구현 타입 (-v)
    size_t          memalloc;        // 메모리 할당 방식 (-m)
    size_t          numThreads;      // CUDA 스레드 수 (-t)
    uint32_t        domain;          // GPU PCIe 도메인 (자동)
    uint32_t        bus;             // GPU PCIe 버스 (자동)
    uint32_t        devfn;           // GPU PCIe 디바이스 (자동)
    uint32_t n_ctrls;                // NVMe 컨트롤러 수 (-k)
    size_t blkSize;                  // CUDA 블록 크기 (-b)
    size_t queueDepth;               // NVMe 큐 depth (-d)
    size_t numQueues;                // 컨트롤러당 큐 수 (-q)
    size_t pageSize;                 // page cache 페이지 크기 (-p)
    uint64_t numElems;               // backing array 원소 수 (미사용)
    size_t repeat;                   // 반복 횟수 (-r)
    size_t src;                      // 시작 정점 (-s)
    uint64_t maxPageCacheSize;       // page cache 최대 크기 (-M)
    Settings();
    void parseArguments(int argc, char** argv);

    static std::string usageString(const std::string& name);

    std::string getDeviceBDF() const;
};


/* OptionIface / Option<T> / Range: 커맨드라인 옵션 파싱 프레임워크 */
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



void Settings::parseArguments(int argc, char** argv)
{
    OptionMap parsers = {
        {'f', OptionPtr(new Option<const char*>(input, "path", "input", "File path where the vertex file is. Provide .bel path."))},
        {'l', OptionPtr(new Range(ofileoffset, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "loffset", "Offset where the input file contents need to be stored in NVMe SSD", "0"))},
        {'w', OptionPtr(new Range(wfileoffset, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "woffset", "Offset where the graph weight file contents are stored in NVMe SSD", "34359738368"))},
        {'v', OptionPtr(new Range(type, 0, 5, "impl_type", "COALESCE = 1, COALESCE_CHUNK = 2, COALESCE_PC = 4, COALESCE_CHUNK_PC = 5", "1"))},
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
//        {'e', OptionPtr(new Range(numElems, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "num_elems", "number of 64-bit elements in backing array", "2147483648"))},
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
    wfileoffset = 34359738368;
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
}




#endif
