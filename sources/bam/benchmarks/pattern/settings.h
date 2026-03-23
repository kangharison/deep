/*
 * =============================================================================
 * BaM Pattern 벤치마크 설정 헤더 (benchmarks/pattern/settings.h)
 * =============================================================================
 *
 * [목적]
 * pattern 벤치마크의 커맨드라인 옵션을 파싱하고 관리하는 Settings 구조체를 정의한다.
 * I/O 접근 패턴(순차/랜덤/Zipfian/스트라이드) 벤치마크에서 GPU 디바이스, NVMe 컨트롤러,
 * page cache 크기, 큐 설정, Zipfian 분포 파라미터 등을 설정한다.
 *
 * [BaM 아키텍처에서의 위치]
 * benchmarks/pattern/main.cu에서 include하여 사용.
 * GPU 커널이 NVMe SSD에 다양한 접근 패턴으로 I/O를 발생시키는 벤치마크의 설정 관리.
 *
 * [주요 설정 옵션 요약]
 * | 옵션 | 필드명          | 기본값       | 설명                                          |
 * |------|-----------------|-------------|-----------------------------------------------|
 * | -a   | input_a         | (필수)       | 입력 파일 경로 (.bel)                          |
 * | -A   | afileoffset     | 0            | 입력 파일의 NVMe SSD 내 시작 오프셋 (바이트)    |
 * | -v   | type            | 1            | 커널 구현 타입 (0=BASELINE ~ 10=COALESCE_HASH_PC) |
 * | -m   | memalloc        | 2            | 메모리 모드 (0=GPUMEM, 1=UVM_RO, 2=UVM_DIR, 6=BAFS) |
 * | -s   | n_elems         | 1048576(1M)  | 벡터 크기 (원소 수, 각 8바이트)                 |
 * | -t   | numThreads      | 1024         | CUDA 스레드 수                                 |
 * | -b   | blkSize         | 64           | CUDA 스레드 블록 크기                          |
 * | -g   | cudaDevice      | 0            | CUDA GPU 디바이스 번호                         |
 * | -k   | n_ctrls         | 1            | NVMe 컨트롤러 수                               |
 * | -p   | pageSize        | 4096         | BaM page cache 페이지 크기 (바이트)            |
 * | -d   | queueDepth      | 1024         | NVMe 큐 깊이                                  |
 * | -q   | numQueues       | 128          | NVMe 큐 수                                    |
 * | -M   | maxPageCacheSize| 8589934592(8GB)| page cache 최대 크기                        |
 * | -P   | stride          | 1            | 워프 재매핑 stride 팩터                        |
 * | -S   | sectorsize      | 4096         | 캐시라인 섹터 크기                             |
 * | -E   | seed            | 0            | Zipfian/powerlaw 분포 시드                     |
 * | -C   | coarse          | 1            | 스레드 coarsening 팩터                         |
 *
 * [옵션 파싱 인프라 계층 구조]
 * OptionIface (추상 베이스) -> Option<T> (타입별 파서) -> Range (범위 검증 추가)
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
 * Settings 구조체: pattern 벤치마크 실행에 필요한 모든 설정을 관리한다.
 * vectoradd/settings.h와 유사하지만 input_b 필드가 없고, sectorsize/seed 필드가 추가되어 있다.
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
    const char*     input_a;            /* 입력 파일 경로 (-a 옵션) */
    size_t          afileoffset;        /* 입력 파일의 NVMe SSD 내 바이트 오프셋 */
    size_t          bfileoffset;        /* 두 번째 파일 오프셋 (pattern에서는 미사용) */
    size_t          type;               /* 커널 구현 타입 (0~10, BASELINE/COALESCE/HASH 등) */
    size_t          memalloc;           /* 메모리 할당 모드 (0=GPUMEM, 1=UVM_RO, 2=UVM_DIR, 6=BAFS) */
    size_t          numThreads;         /* CUDA 스레드 수 */
    uint32_t        domain;             /* GPU PCIe 도메인 (자동 탐지) */
    uint32_t        bus;                /* GPU PCIe 버스 (자동 탐지) */
    uint32_t        devfn;              /* GPU PCIe 디바이스/함수 (자동 탐지) */
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
    uint64_t sectorsize;               /* 캐시라인 섹터 크기 (바이트, 2의 거듭제곱) */
    uint64_t seed;                      /* Zipfian/powerlaw 랜덤 분포 시드 */
    //float    zipf;
    uint64_t coarse;                    /* 스레드 coarsening 팩터: 각 스레드가 처리할 원소 수 배율 */
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
typedef std::shared_ptr<OptionIface>  OptionPtr;    /* 옵션 파서의 shared_ptr 타입 */
typedef std::map<int, OptionPtr> OptionMap;          /* 단축키(int) -> 옵션 파서 매핑 */


/*
 * OptionIface - 옵션 파서 추상 베이스 클래스
 *
 * 모든 커맨드라인 옵션 파서의 공통 인터페이스를 정의한다.
 * type: 옵션 값 타입 문자열 ("number", "path" 등, 에러 메시지용)
 * name: 옵션 이름 (--name으로 사용)
 * description: 도움말 설명
 * defaultValue: 기본값 문자열 (도움말 표시용)
 * hasArgument: getopt의 no_argument / required_argument
 */
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

    /* 파생 클래스에서 타입별 파싱 로직을 구현 */
    virtual void parseArgument(const char* optstr, const char* optarg) = 0;

    virtual void throwError(const char*, const char* optarg) const
    {
        throw string("Option ") + name + string(" expects a ") + type + string(", but got `") + optarg + string("'");
    }
};


/*
 * Option<T> - 타입별 옵션 파서 템플릿
 *
 * Settings 구조체의 필드에 대한 참조(value)를 보유하고,
 * 문자열 인자를 T 타입으로 변환하여 직접 대입한다.
 * 특수화: uint32_t, uint64_t, bool, const char*
 */
template <typename T>
struct Option: public OptionIface
{
    T&              value;    /* Settings 필드에 대한 참조 */

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


/* uint32_t 특수화: strtoul로 문자열을 부호 없는 32비트 정수로 변환 */
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


/* uint64_t 특수화: strtoul로 문자열을 부호 없는 64비트 정수로 변환 */
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


/* bool 특수화: "true/yes/1/on/enable" -> true, "false/no/0/off/disable" -> false */
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


/* const char* 특수화: 문자열 포인터를 그대로 대입 (파일 경로 등) */
template <>
void Option<const char*>::parseArgument(const char* optstr, const char* optarg)
{
    if(optarg == nullptr){
            throwError(optstr, optarg);
    }
    value = optarg;
}


/*
 * Range - uint64_t 옵션에 [lower, upper] 범위 검증을 추가한 특수화
 *
 * lower == 0이면 하한 검사 생략, upper == 0이면 상한 검사 생략.
 * 범위를 벗어나면 throwError()로 에러 메시지와 함께 예외 발생.
 */
struct Range: public Option<uint64_t>
{
    uint64_t      lower;    /* 허용 최솟값 (0이면 검사 안 함) */
    uint64_t      upper;    /* 허용 최댓값 (0이면 검사 안 함) */

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
 * setBDF - CUDA GPU의 PCIe BDF(Bus/Device/Function) 정보를 자동 탐지하여 Settings에 저장
 *
 * @settings: GPU BDF 정보를 저장할 Settings 구조체
 *
 * cudaGetDeviceProperties()로 GPU의 PCIe 위치(도메인, 버스, 디바이스)를 조회한다.
 * BaM에서 GPU와 NVMe SSD 간 P2P DMA 경로를 설정할 때 GPU의 PCIe 위치가 필요하다.
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


/*
 * getDeviceBDF - GPU의 PCIe BDF를 "DDDD:BB:DD.0" 형식 문자열로 반환
 *
 * @return: "0000:3b:00.0" 형식의 PCIe BDF 문자열
 *
 * libnvm이 GPU와 NVMe 간 P2P DMA를 설정할 때 이 문자열을 사용한다.
 */
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



/* helpString - 등록된 모든 옵션의 도움말 테이블 문자열을 생성하여 반환 */
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
 * createLongOptions - OptionMap에서 getopt_long용 option 배열과 optstring을 자동 생성
 *
 * @options: 결과 option 구조체 벡터 (getopt_long의 longopts 매개변수)
 * @optionString: 결과 단축 옵션 문자열 (getopt_long의 optstring 매개변수)
 * @parsers: 등록된 옵션 파서 맵
 *
 * 항상 --help(-h)를 먼저 추가하고, parsers의 각 엔트리를 순회하며 long option 등록.
 * 단축키가 ASCII 범위('0'~'z')이면 optionString에도 추가한다.
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


/*
 * verifyCudaDevice - CUDA 디바이스 번호가 유효한지 검증
 *
 * @device: 검증할 CUDA 디바이스 번호
 * 시스템에 존재하는 GPU 수를 조회하여 범위를 확인한다. 유효하지 않으면 예외 발생.
 */
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


/*
 * verifyNumberOfThreads - 스레드 수가 2의 거듭제곱(1~32)인지 검증
 *
 * @numThreads: 검증할 스레드 수
 * 현재 코드에서는 호출부가 주석 처리되어 사용되지 않음.
 */
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
 * parseArguments - 커맨드라인 옵션을 파싱하여 Settings 필드에 저장
 *
 * @argc: 인자 개수
 * @argv: 인자 배열
 *
 * getopt_long을 사용하여 등록된 옵션을 파싱한다.
 * 파싱 완료 후 CUDA 디바이스 유효성 검증 및 PCIe BDF 자동 탐지를 수행한다.
 */
void Settings::parseArguments(int argc, char** argv)
{
    OptionMap parsers = {
        {'a', OptionPtr(new Option<const char*>(input_a, "path", "input_a", "File path A file is. Provide .bel path."))},
        {'A', OptionPtr(new Range(afileoffset, 0, (uint64_t)std::numeric_limits<uint64_t>::max, "aoffset", "Offset where the input file contents need to be stored in NVMe SSD", "0"))},
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
        {'S', OptionPtr(new Option<uint64_t>(sectorsize, "number", "sectorsize", "Sector size of the cacheline. Assumes power of 2", "4096"))},
        {'E', OptionPtr(new Option<uint64_t>(seed, "number", "seed", "seed for powerlaw distribution.", "0"))},
        //{'Z', OptionPtr(new Option<float>(zipf, "number", "zipf", "Zipfian scale factor. 1.25 gives 70-30, 1.45 gives 80-20, 1.8 gives 90-10. Default", "1.45"))},
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
 * Settings 기본값 생성자: 안전한 기본값으로 모든 필드를 초기화
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
    nvmNamespace = 1;           /* NVMe 네임스페이스 1번 */
    doubleBuffered = false;
    numReqs = 1;
    numPages = 1024;
    startBlock = 0;
    stats = false;
    input = nullptr;
    input_a = nullptr;
    output = nullptr;
    afileoffset = 0;
    bfileoffset = 0;
    type = 1;                   /* 기본 커널: COALESCE */
    memalloc = 2;               /* 기본 메모리: UVM_DIRECT */
    numThreads = 1024;
    blkSize = 64;
    domain = 0;
    bus = 0;
    devfn = 0;
    n_ctrls = 1;
    queueDepth = 1024;
    numQueues = 128;
    pageSize = 4096;            /* 4KB 페이지 */
    numElems = 2147483648;      /* 2G개 (legacy) */
    repeat = 32;
    src = 0;
    maxPageCacheSize = 8589934592;  /* 8GB page cache */
    stride = 1;
    sectorsize= 4096;          /* 4KB 섹터 */
    seed = 0;
    coarse = 1;
    //zipf = 1.45;
    n_elems= 1048576;          /* 1M개 원소 = 8MB */
}




#endif
