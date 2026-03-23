/*
 * =============================================================================
 * BaM VectorAdd 벤치마크 설정 헤더 (benchmarks/vectoradd/settings.h)
 * =============================================================================
 *
 * [목적]
 * vectoradd 벤치마크의 커맨드라인 옵션을 파싱하고 관리하는 Settings 구조체를 정의한다.
 * reduction/settings.h와 거의 동일하지만 입력 파일이 2개(A, B)이고 bfileoffset이 추가되어 있다.
 *
 * [주요 설정 옵션 요약]
 * | 옵션 | 필드명          | 기본값       | 설명                                          |
 * |------|-----------------|-------------|-----------------------------------------------|
 * | -a   | input_a         | (필수)       | 입력 벡터 A 파일 경로 (.bel의 .dst)            |
 * | -b   | input_b         | (필수)       | 입력 벡터 B 파일 경로 (.bel의 .dst)            |
 * | -A   | afileoffset     | 0            | 벡터 A의 NVMe SSD 내 시작 오프셋 (바이트)      |
 * | -B   | bfileoffset     | 0            | 벡터 B의 NVMe SSD 내 시작 오프셋 (바이트)      |
 * | -v   | type            | 1            | 커널 구현 타입 (0=BASELINE, 1=OPTIMIZED 등)    |
 * | -m   | memalloc        | 2            | 메모리 모드 (0=GPUMEM, 1=UVM_RO, 2=UVM_DIR, 6=BAFS) |
 * | -s   | n_elems         | 1048576(1M)  | 벡터 크기 (원소 수, 각 8바이트)                 |
 * | -t   | numThreads      | 1024         | CUDA 블록당 스레드 수                          |
 * | -g   | cudaDevice      | 0            | CUDA GPU 디바이스 번호                         |
 * | -k   | n_ctrls         | 1            | NVMe 컨트롤러 수                               |
 * | -p   | pageSize        | 4096         | BaM page cache 페이지 크기 (바이트)            |
 * | -d   | queueDepth      | 1024         | NVMe 큐 깊이                                  |
 * | -q   | numQueues       | 128          | NVMe 큐 수                                    |
 * | -M   | maxPageCacheSize| 8589934592(8GB)| page cache 최대 크기                        |
 * | -P   | stride          | 1            | 워프 재매핑 stride 팩터                        |
 * | -C   | coarse          | 1            | 스레드 coarsening 팩터                         |
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


/*
 * Settings 구조체: vectoradd 벤치마크 실행에 필요한 모든 설정을 관리한다.
 * reduction과의 차이점: input_b 필드와 bfileoffset 필드가 추가됨 (두 번째 입력 벡터용).
 */
struct Settings
{
    uint32_t        cudaDevice;         /* 사용할 CUDA GPU 디바이스 번호 (기본값: 0) */
    uint64_t        cudaDeviceId;       /* CUDA 디바이스 내부 ID */
    const char*     blockDevicePath;    /* NVMe 블록 디바이스 경로 (legacy, 미사용) */
    const char*     controllerPath;     /* NVMe 컨트롤러 경로 (legacy, 미사용) */
    uint64_t        controllerId;       /* 컨트롤러 ID (미사용) */
    uint32_t        adapter;            /* SmartIO 어댑터 (미사용) */
    uint32_t        segmentId;          /* SmartIO 세그먼트 (미사용) */
    uint32_t        nvmNamespace;       /* NVMe 네임스페이스 번호 (기본값: 1) */
    bool            doubleBuffered;     /* 더블 버퍼링 (미사용) */
    size_t          numReqs;            /* 요청 수 (미사용) */
    size_t          numPages;           /* 페이지 수 (미사용) */
    size_t          startBlock;         /* 시작 블록 (미사용) */
    bool            stats;              /* 통계 출력 (미사용) */
    const char*     input;              /* 범용 입력 (미사용) */
    const char*     output;             /* 범용 출력 (미사용) */
    const char*     input_a;            /* 입력 벡터 A 파일 경로 (-a 옵션) */
    const char*     input_b;            /* 입력 벡터 B 파일 경로 (-b 옵션). reduction에는 없는 필드 */
    size_t          afileoffset;        /* 벡터 A의 NVMe SSD 내 바이트 오프셋 */
    size_t          bfileoffset;        /* 벡터 B의 NVMe SSD 내 바이트 오프셋 */
    size_t          type;               /* 커널 구현 타입 (0~3) */
    size_t          memalloc;           /* 메모리 할당 모드 (0,1,2,6) */
    size_t          numThreads;         /* 블록당 CUDA 스레드 수 */
    uint32_t        domain;             /* GPU PCIe 도메인 (자동) */
    uint32_t        bus;                /* GPU PCIe 버스 (자동) */
    uint32_t        devfn;              /* GPU PCIe 디바이스/함수 (자동) */
    uint32_t n_ctrls;                   /* NVMe 컨트롤러 수 */
    size_t blkSize;                     /* CUDA 스레드 블록 크기 */
    size_t queueDepth;                  /* NVMe I/O 큐 깊이 */
    size_t numQueues;                   /* 컨트롤러당 I/O 큐 수 */
    size_t pageSize;                    /* BaM page cache 페이지 크기 (바이트) */
    uint64_t numElems;                  /* 원소 수 (legacy) */
    size_t repeat;                      /* 반복 횟수 (미사용) */
    size_t src;                         /* 소스 선택 (미사용) */
    uint64_t maxPageCacheSize;          /* page cache 최대 크기 (바이트) */
    uint64_t stride;                    /* 워프 재매핑 stride. OPTIMIZED_PC에서 워프를 SSD 채널에 분산시키는 데 사용 */
    uint64_t coarse;                    /* 스레드 coarsening 팩터 */
    uint64_t n_elems;                   /* 벡터 원소 수 (기본값: 1M, 각 8바이트) */
    Settings();
    void parseArguments(int argc, char** argv);

    static std::string usageString(const std::string& name);

    std::string getDeviceBDF() const;
};


/* 옵션 파싱 인프라: OptionIface -> Option<T> -> Range 계층 구조 */
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


/* Range: uint64_t 옵션에 [lower, upper] 범위 검증을 추가한 특수화 */
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



/* setBDF: CUDA GPU의 PCIe BDF(Bus/Device/Function) 정보를 자동 탐지하여 Settings에 저장 */
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



/*
 * parseArguments: 커맨드라인 옵션을 파싱한다.
 * vectoradd 고유 옵션: -b(input_b), -B(bfileoffset) -- 두 번째 입력 벡터 관련
 */
void Settings::parseArguments(int argc, char** argv)
{
    OptionMap parsers = {
        {'a', OptionPtr(new Option<const char*>(input_a, "path", "input_a", "File path A file is. Provide .bel path."))},
        {'b', OptionPtr(new Option<const char*>(input_b, "path", "input_b", "File path B file is. Provide .bel path."))},
        {'A', OptionPtr(new Range(afileoffset, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "aoffset", "Offset where the input file contents need to be stored in NVMe SSD", "0"))},
        {'B', OptionPtr(new Range(bfileoffset, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "boffset", "Offset where the input file contents need to be stored in NVMe SSD", "0"))},
        {'v', OptionPtr(new Range(type, 0, 50, "impl_type", "BASELINE=0, COALESCE = 1, COALESCE_CHUNK = 2, BASELINE_PC=3, COALESCE_PC = 4, COALESCE_CHUNK_PC = 5\n BASELINE_HASH = 6, COALESCE_HASH = 7, BASELINE_HASH_PC = 9, COALESCE_HASH_PC = 10", "1"))},
        {'m', OptionPtr(new Range(memalloc, 0, 6, "memalloc", "GPUMEM = 0, UVM_READONLY = 1, UVM_DIRECT = 2, BAFS_DIRECT = 6", "2"))},
        {'s', OptionPtr(new Option<uint64_t>(n_elems, "number", "n_elems", "specify vector size in elements for both A and B. Each element is of 8B. Default uses 1M elements", "1048576"))},
        {'t', OptionPtr(new Range(numThreads, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "threads", "number of CUDA threads", "1024"))},
        {'b', OptionPtr(new Range(blkSize, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "blk_size", "CUDA thread block size", "64"))},
        {'g', OptionPtr(new Option<uint32_t>(cudaDevice, "number", "gpu", "specify CUDA device", "0"))},
        {'k', OptionPtr(new Option<uint32_t>(n_ctrls, "number", "n_ctrls", "specify number of NVMe controllers", "1"))},
        {'p', OptionPtr(new Range(pageSize, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "page_size", "size of page in cache", "4096"))},
        {'d', OptionPtr(new Range(queueDepth, 2, 65536, "queue_depth", "queue depth per queue", "16"))},
        {'q', OptionPtr(new Range(numQueues, 1, 65536, "num_queues", "number of queues per controller", "1"))},
        {'M', OptionPtr(new Option<uint64_t>(maxPageCacheSize, "number", "maxPCSize", "Maximum Page Cache size in bytes", "8589934592"))},
        {'P', OptionPtr(new Option<uint64_t>(stride, "number", "STRIDE", "Hashing stride factor for cc. It is calculated as P = stride. Assumes power of 2", "1"))},
        {'C', OptionPtr(new Option<uint64_t>(coarse, "number", "COARSE", "Thread coarsening factor", "1"))},
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


/*
 * Settings 기본값 생성자: 안전한 기본값으로 초기화
 */
Settings::Settings()
{
    cudaDevice = 0;
    cudaDeviceId = 0;
    blockDevicePath = nullptr;
    controllerPath = nullptr;
    controllerId = 0;
    adapter = 0;
    segmentId = 0;
    nvmNamespace = 1;          /* NVMe 네임스페이스 1번 */
    doubleBuffered = false;
    numReqs = 1;
    numPages = 1024;
    startBlock = 0;
    stats = false;
    input = nullptr;
    input_a = nullptr;
    input_b = nullptr;         /* vectoradd 고유: 두 번째 입력 벡터 경로 */
    output = nullptr;
    afileoffset = 0;
    bfileoffset = 0;           /* vectoradd 고유: 벡터 B의 SSD 오프셋 */
    type = 1;                  /* 기본 커널: OPTIMIZED */
    memalloc = 2;              /* 기본 메모리: UVM_DIRECT */
    numThreads = 1024;
    blkSize = 64;
    domain = 0;
    bus = 0;
    devfn = 0;
    n_ctrls = 1;
    queueDepth = 1024;
    numQueues = 128;
    pageSize = 4096;           /* 4KB 페이지 */
    numElems = 2147483648;     /* 2G개 (legacy) */
    repeat = 32;
    src = 0;
    maxPageCacheSize = 8589934592;  /* 8GB page cache */
    stride = 1;
    coarse = 1;
    n_elems= 1048576;          /* 1M개 원소 = 8MB */
}




#endif
