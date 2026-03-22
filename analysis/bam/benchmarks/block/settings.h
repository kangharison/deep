/*
 * settings.h - BaM Block Benchmark 설정 파일
 * ===========================================
 * 벤치마크의 모든 커맨드라인 옵션을 파싱하고 관리하는 Settings 구조체와
 * 옵션 파싱 프레임워크(OptionIface, Option<T>, Range)를 정의한다.
 *
 * 주요 옵션:
 *   -g: GPU 디바이스 번호       -k: NVMe 컨트롤러 수
 *   -t: CUDA 스레드 수          -b: CUDA 블록 크기
 *   -p: 캐시 페이지 수          -P: 페이지 크기(바이트)
 *   -n: 스레드당 요청 횟수      -d: NVMe 큐 깊이
 *   -q: 컨트롤러당 큐 수        -e: backing 배열 블록 수
 *   -r: 랜덤/순차 선택          -o: 접근 유형(read/write/mixed)
 *   -s: mixed 모드 읽기 비율(%) -S: SSD 유형(Samsung/Intel)
 *   -f: 입력 파일 경로
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

/* 접근 유형 상수. access_type 파라미터로 전달되어 커널에서 분기에 사용한다. */
#define READ 0    /* 읽기 전용 벤치마크 */
#define WRITE 1   /* 쓰기 전용 벤치마크 */
#define MIXED 2   /* 읽기/쓰기 혼합 벤치마크 (ratio로 비율 조절) */

/*
 * Settings 구조체: 벤치마크 실행에 필요한 모든 파라미터를 보관한다.
 * 생성자에서 기본값을 설정하고, parseArguments()에서 커맨드라인 인자로 덮어쓴다.
 */
struct Settings
{
    uint32_t        cudaDevice;       /* CUDA GPU 디바이스 번호 (기본 0) */
    uint64_t        cudaDeviceId;     /* DIS 클러스터 모드에서의 CUDA 디바이스 FDID */
    const char*     blockDevicePath;  /* 블록 디바이스 경로 (미사용, legacy) */
    const char*     controllerPath;   /* NVMe 컨트롤러 경로 (미사용, ctrls_paths[] 사용) */
    uint64_t        controllerId;     /* DIS 클러스터 모드에서의 NVMe 컨트롤러 FDID */
    uint32_t        adapter;          /* DIS 어댑터 번호 */
    uint32_t        segmentId;        /* DIS 세그먼트 ID 오프셋 */
    uint32_t        nvmNamespace;     /* NVMe 네임스페이스 ID (기본 1) */
    bool            doubleBuffered;   /* 더블 버퍼링 사용 여부 (미사용) */
    size_t          numReqs;          /* 스레드당 I/O 요청 반복 횟수 (기본 1) */
    size_t          numPages;         /* 페이지 캐시의 페이지 수 (기본 1024) */
    size_t          startBlock;       /* 시작 블록 오프셋 (미사용) */
    bool            stats;            /* 통계 출력 여부 (미사용) */
    const char*     output;           /* 출력 파일 경로 (미사용) */
    size_t          numThreads;       /* 총 CUDA 스레드 수 = 총 I/O 요청 수 (기본 64) */
    uint32_t        domain;           /* GPU PCIe 도메인 (자동 설정) */
    uint32_t        bus;              /* GPU PCIe 버스 (자동 설정) */
    uint32_t        devfn;            /* GPU PCIe 디바이스 (자동 설정) */
    uint32_t n_ctrls;                 /* 사용할 NVMe 컨트롤러 수 (기본 1) */
    size_t blkSize;                   /* CUDA 스레드 블록 크기 (기본 64, warp 2개) */
    size_t queueDepth;                /* NVMe 큐 깊이 (기본 16) */
    size_t numQueues;                 /* 컨트롤러당 I/O 큐 수 (기본 1) */
    size_t pageSize;                  /* 캐시 페이지 크기 = I/O 요청 크기 (기본 4096B) */
    uint64_t numBlks;                 /* SSD backing 배열의 총 블록 수 (기본 2M = 8GB@4KB) */
    bool random;                      /* true: 랜덤 접근, false: 순차 접근 (기본 true) */
    uint64_t accessType;              /* READ(0), WRITE(1), MIXED(2) (기본 READ) */
    uint64_t ratio;                   /* MIXED 모드 시 읽기 비율(%) (기본 100 = 전부 읽기) */
    uint64_t ssdtype;                 /* SSD 유형: 0=Samsung, 1=Intel (기본 0) */
    const char*     input;            /* 입력 파일 경로: mmap하여 SSD에 쓸 데이터를 제공 */
    Settings();
    void parseArguments(int argc, char** argv);

    static std::string usageString(const std::string& name);

    /* GPU의 PCIe BDF(Bus:Device.Function)를 "DDDD:BB:DD.0" 형식 문자열로 반환한다. */
    std::string getDeviceBDF() const;
};


/*
 * OptionIface: 커맨드라인 옵션 파서의 추상 인터페이스.
 * 각 옵션의 이름, 타입 설명, 도움말 텍스트, 기본값을 보관하고
 * parseArgument()를 통해 문자열을 실제 값으로 변환한다.
 */
struct OptionIface;
using std::string;
using std::vector;
using std::make_shared;
typedef std::shared_ptr<OptionIface>  OptionPtr;
typedef std::map<int, OptionPtr> OptionMap;  /* key: 옵션 문자(short opt), value: 파서 */


struct OptionIface
{
    const char*     type;           /* 값의 타입 이름 (예: "number", "bool", "path") */
    const char*     name;           /* 옵션의 long name (예: "gpu", "threads") */
    const char*     description;    /* 사용법 설명 */
    const char*     defaultValue;   /* 기본값 문자열 (표시용) */
    int             hasArgument;    /* getopt의 no_argument / required_argument */

    virtual ~OptionIface() = default;

    OptionIface(const char* type, const char* name, const char* description)
        : type(type), name(name), description(description), hasArgument(no_argument) { }

    OptionIface(const char* type, const char* name, const char* description, const char* dvalue)
        : type(type), name(name), description(description), defaultValue(dvalue), hasArgument(no_argument) { }

    /* 문자열 인자를 파싱하여 내부 값을 설정하는 순수 가상 함수 */
    virtual void parseArgument(const char* optstr, const char* optarg) = 0;

    virtual void throwError(const char*, const char* optarg) const
    {
        throw string("Option ") + name + string(" expects a ") + type + string(", but got `") + optarg + string("'");
    }
};


/*
 * Option<T>: 타입별 옵션 파서 (템플릿 특수화로 uint32_t, uint64_t, bool, const char* 지원).
 * value 레퍼런스를 통해 Settings의 멤버를 직접 수정한다.
 */
template <typename T>
struct Option: public OptionIface
{
    T&              value;          /* Settings 구조체 멤버에 대한 참조 */

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


/* uint32_t 옵션: 문자열을 strtoul로 변환 */
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


/* uint64_t 옵션: 문자열을 strtoul로 변환 */
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


/* bool 옵션: "true"/"yes"/"1" 등과 "false"/"no"/"0" 등을 인식 */
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


/* const char* 옵션: 인자 문자열을 그대로 포인터에 저장 */
template <>
void Option<const char*>::parseArgument(const char*, const char* optarg)
{
    value = optarg;
}


/*
 * Range: 범위 제한이 있는 uint64_t 옵션.
 * lower~upper 범위를 벗어나면 에러를 던진다.
 * 큐 깊이, 스레드 수 등 유효 범위가 있는 파라미터에 사용된다.
 */
struct Range: public Option<uint64_t>
{
    uint64_t      lower;   /* 최솟값 (0이면 제한 없음) */
    uint64_t      upper;   /* 최댓값 (0이면 제한 없음) */

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



/*
 * setBDF: CUDA 디바이스의 PCIe BDF(Bus:Device.Function) 정보를
 * Settings에 자동으로 채운다. GPU의 PCIe 위치를 알아야
 * libnvm이 peer-to-peer DMA 경로를 설정할 수 있다.
 */
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


/* GPU PCIe BDF를 "DDDD:BB:DD.0" 형식 문자열로 반환 */
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



/*
 * helpString: 등록된 모든 옵션의 이름, 타입, 기본값, 설명을 표 형식으로 출력한다.
 * --help(-h) 옵션 사용 시 호출된다.
 */
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


/*
 * createLongOptions: OptionMap에서 getopt_long용 option 배열과
 * 옵션 문자열을 생성한다. 각 파서의 short opt 문자와
 * long name을 struct option으로 변환한다.
 */
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


/* CUDA 디바이스 번호가 유효한지 확인한다 */
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


/* 스레드 수가 2의 거듭제곱(1~32)인지 확인한다 (현재 주석처리되어 미사용) */
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
 * parseArguments: 커맨드라인 인자를 getopt_long으로 파싱하여
 * Settings 멤버에 값을 설정한다.
 *
 * 옵션 매핑 요약:
 *   -f: input        입력 파일 경로 (SSD에 쓸 데이터)
 *   -g: gpu          CUDA 디바이스 번호
 *   -k: n_ctrls      NVMe 컨트롤러 수
 *   -n: reqs         스레드당 I/O 요청 반복 횟수
 *   -p: pages        캐시 페이지 수
 *   -P: page_size    페이지 크기(바이트)
 *   -t: threads      총 CUDA 스레드 수
 *   -b: blk_size     CUDA 블록 크기 (스레드/블록)
 *   -d: queue_depth  NVMe 큐 깊이
 *   -q: num_queues   컨트롤러당 큐 수
 *   -e: num_blks     SSD backing 블록 수
 *   -r: random       true=랜덤, false=순차
 *   -o: access_type  0=read, 1=write, 2=mixed
 *   -s: ratio        mixed 읽기 비율(%)
 *   -S: ssd          0=Samsung, 1=Intel
 */
void Settings::parseArguments(int argc, char** argv)
{
    OptionMap parsers = {
#ifdef __DIS_CLUSTER__
        {'c', OptionPtr(new Option<uint64_t>(controllerId, "fdid", "ctrl", "NVM controller device identifier"))},
        {'f', OptionPtr(new Option<uint64_t>(cudaDeviceId, "fdid", "fdid", "CUDA device FDID"))},
        {'a', OptionPtr(new Option<uint32_t>(adapter, "number", "adapter", "DIS adapter number", "0"))},
        {'S', OptionPtr(new Option<uint32_t>(segmentId, "offset", "segment", "DIS segment identifier offset", "0"))},
#else
        //{'c', OptionPtr(new Option<const char*>(controllerPath, "path", "ctrl", "NVM controller device path"))},
#endif
        {'f', OptionPtr(new Option<const char*>(input, "path", "input", "Input dataset path used to write to NVMe SSD"))},
        {'g', OptionPtr(new Option<uint32_t>(cudaDevice, "number", "gpu", "specify CUDA device", "0"))},
        {'k', OptionPtr(new Option<uint32_t>(n_ctrls, "number", "n_ctrls", "specify number of NVMe controllers", "1"))},
        //{'i', OptionPtr(new Option<uint32_t>(nvmNamespace, "identifier", "namespace", "NVM namespace identifier", "1"))},
        //{'B', OptionPtr(new Option<bool>(doubleBuffered, "bool", "double-buffer", "double buffer disk reads", "false"))},
        //{'r', OptionPtr(new Option<bool>(stats, "bool", "stats", "print statistics", "false"))},
        {'n', OptionPtr(new Range(numReqs, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "reqs", "number of reqs per thread", "1"))},
        {'p', OptionPtr(new Range(numPages, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "pages", "number of pages in cache", "1024"))},
        {'P', OptionPtr(new Range(pageSize, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "page_size", "size of page in cache", "4096"))},
        {'t', OptionPtr(new Range(numThreads, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "threads", "number of CUDA threads", "1024"))},
        {'b', OptionPtr(new Range(blkSize, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "blk_size", "CUDA thread block size", "64"))},
        {'d', OptionPtr(new Range(queueDepth, 2, 65536, "queue_depth", "queue depth per queue", "16"))},
        {'q', OptionPtr(new Range(numQueues, 1, 65536, "num_queues", "number of queues per controller", "1"))},
        {'e', OptionPtr(new Range(numBlks, 1, (uint64_t)std::numeric_limits<uint64_t>::max, "num_blks", "number of pages in backing array", "2097152"))},
        {'r', OptionPtr(new Option<bool>(random, "bool", "random", "if true the random access benchmark runs, if false the sequential access benchmark runs", "true"))},
        //{'o', OptionPtr(new Option<const char*>(output, "path", "output", "output read data to file"))},
        //{'s', OptionPtr(new Option<uint64_t>(startBlock, "offset", "offset", "number of blocks to offset", "0"))},
        //{'j', OptionPtr(new Option<const char*>(blockDevicePath, "path", "block-device", "path to block device"))},
        {'o', OptionPtr(new Range(accessType, 0, 3, "access_type", "type of access to make: 0->read, 1->write, 2->mixed", "0"))},
        {'s', OptionPtr(new Range(ratio, 0, 100, "ratio", "ratio split for % of mixed accesses that are read", "100"))},
        {'S', OptionPtr(new Range(ssdtype, 0, 2, "ssd", "type of SSD to use 0->Samsung, 1->Intel", "0"))},
    };

    string optionString;
    vector<option> options;
    createLongOptions(options, optionString, parsers);

    int index;
    int option;
    OptionMap::iterator parser;

    /* getopt_long 루프: 모든 커맨드라인 옵션을 순회하며 파싱 */
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
    /* CUDA 디바이스 유효성 검사 후 PCIe BDF 정보를 자동 설정 */
    verifyCudaDevice(cudaDevice);
    //verifyNumberOfThreads(numThreads);

    setBDF(*this);
}


/*
 * Settings 기본값 설정.
 * 대부분의 벤치마크에서 기본적으로 동작하는 안전한 값들이다.
 * numBlks=2097152(2M)은 4KB 페이지 기준 약 8GB의 SSD 영역을 커버한다.
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
    nvmNamespace = 1;
    doubleBuffered = false;
    numReqs = 1;
    numPages = 1024;
    startBlock = 0;
    stats = false;
    output = nullptr;
    numThreads = 64;
    blkSize = 64;
    domain = 0;
    bus = 0;
    devfn = 0;
    n_ctrls = 1;
    queueDepth = 16;
    numQueues = 1;
    pageSize = 4096;
    numBlks = 2097152;
    random = true;
    accessType = READ;
    ratio = 100;
    input = nullptr;
    ssdtype =0;
}




#endif
