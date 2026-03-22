# abstract_hardware_model.h — 라인바이라인 주석 분석

> 파일: `src/abstract_hardware_model.h` (1,536줄)
> 역할: GPGPU-Sim 전체의 핵심 데이터 구조 정의. GPU 하드웨어를 추상화한 모델로, 커널, 워프, 명령어, 메모리 접근, SIMT 스택, 레지스터 셋 등 시뮬레이터의 근간이 되는 타입과 클래스를 선언한다.

---

## 1-31행: 저작권 및 라이선스 (BSD 3-Clause)

```cpp
// Copyright (c) 2009-2021, Tor M. Aamodt, Inderpreet Singh, Vijay Kandiah,
// Nikos Hardavellas, Mahmoud Khairy, Junrui Pan, Timothy G. Rogers
// The University of British Columbia, Northwestern University, Purdue University
// All rights reserved.
// ... BSD 3-Clause 라이선스 전문 ...
```

UBC, Northwestern, Purdue 대학교의 공동 저작물. BSD 3-Clause 라이선스로 배포된다.

---

## 32-33행: 인클루드 가드

```cpp
#ifndef ABSTRACT_HARDWARE_MODEL_INCLUDED  // 중복 인클루드 방지 매크로
#define ABSTRACT_HARDWARE_MODEL_INCLUDED
```

---

## 35-38행: 전방 선언 (Forward Declarations)

```cpp
class gpgpu_sim;      // GPU 시뮬레이터 최상위 클래스 (gpu-sim.h에서 정의)
class kernel_info_t;  // CUDA 커널 정보를 담는 클래스 (이 파일 아래에서 정의)
class gpgpu_context;  // 시뮬레이터 전역 컨텍스트 (여러 GPU 인스턴스를 관리)
```

순환 의존을 피하기 위한 전방 선언. 실제 정의는 각각 다른 헤더에 있다.

---

## 40-46행: 하드웨어 제한 상수

```cpp
#define MAX_CTA_PER_SHADER 32   // 하나의 SM(Shader)에 동시 할당 가능한 최대 CTA(스레드 블록) 수
                                 // 실제 CUDA는 8이지만, 시뮬레이터에서는 여유롭게 32로 설정
#define MAX_BARRIERS_PER_CTA 16 // CTA 하나에서 사용할 수 있는 최대 배리어 수 (명명된 배리어)

#define MAX_INPUT_VALUES 24     // 벡터 입출력 오퍼랜드 확장 후, 한 명령어의 최대 입력 값 수
#define MAX_OUTPUT_VALUES 8     // 한 명령어의 최대 출력 값 수
```

---

## 48-63행: 메모리 공간 열거형 (`_memory_space_t`)

```cpp
enum _memory_space_t {
  undefined_space = 0,        // 정의되지 않은 메모리 공간
  reg_space,                  // 레지스터 공간 — 스레드별 사적 레지스터
  local_space,                // 로컬 메모리 — 스레드별 사적, 레지스터 스필용 (오프칩)
  shared_space,               // 공유 메모리 — CTA 내 스레드 간 공유 (온칩 스크래치패드)
  sstarr_space,               // 특수 공유 메모리 배열 공간
  param_space_unclassified,   // 파라미터 공간 (분류 전)
  param_space_kernel,         // 커널 파라미터 공간 — 모든 스레드가 읽기만 가능 (상수처럼 동작)
  param_space_local,          // 로컬 파라미터 공간 — 스레드별 읽기/쓰기 가능
  const_space,                // 상수 메모리 — 전역 읽기 전용, 캐시됨
  tex_space,                  // 텍스처 메모리 — 공간적 지역성에 최적화된 읽기 전용 캐시
  surf_space,                 // 서피스 메모리 — 텍스처와 유사하나 쓰기도 가능
  global_space,               // 글로벌 메모리 — 모든 스레드가 읽기/쓰기 가능 (오프칩 DRAM)
  generic_space,              // 제네릭 공간 — 런타임에 실제 공간으로 매핑
  instruction_space           // 명령어 메모리 공간 — 명령어 페치용
};
```

PTX ISA의 메모리 공간 모델을 그대로 반영한다. CUDA 프로그래밍에서 `__shared__`, `__global__`, `__constant__` 등의 키워드가 이 열거형의 각 값에 대응한다.

---

## 65-87행: 전력 스케일링 계수 구조체 (`PowerscalingCoefficients`)

```cpp
struct PowerscalingCoefficients {
  double int_coeff;         // 정수 연산 전력 계수
  double int_mul_coeff;     // 정수 곱셈 전력 계수
  double int_mul24_coeff;   // 24비트 정수 곱셈 전력 계수
  double int_mul32_coeff;   // 32비트 정수 곱셈 전력 계수
  double int_div_coeff;     // 정수 나눗셈 전력 계수
  double fp_coeff;          // 단정밀도 부동소수점 전력 계수
  double dp_coeff;          // 배정밀도 부동소수점 전력 계수
  double fp_mul_coeff;      // FP 곱셈 전력 계수
  double fp_div_coeff;      // FP 나눗셈 전력 계수
  double dp_mul_coeff;      // DP 곱셈 전력 계수
  double dp_div_coeff;      // DP 나눗셈 전력 계수
  double sqrt_coeff;        // 제곱근 연산 전력 계수
  double log_coeff;         // 로그 연산 전력 계수
  double sin_coeff;         // 사인 연산 전력 계수
  double exp_coeff;         // 지수 연산 전력 계수
  double tensor_coeff;      // 텐서 코어 연산 전력 계수
  double tex_coeff;         // 텍스처 연산 전력 계수
};
```

AccelWattch(전력 모델)에서 사용하는 연산별 전력 소비 계수. 각 연산 유형마다 서로 다른 에너지를 소비하므로 개별 계수가 필요하다.

---

## 89-95행: 캐시 설정 열거형

```cpp
enum FuncCache {
  FuncCachePreferNone = 0,    // 캐시 선호 없음 (기본)
  FuncCachePreferShared = 1,  // 공유 메모리 우선 (L1 캐시 줄이고 공유 메모리 늘림)
  FuncCachePreferL1 = 2       // L1 캐시 우선 (공유 메모리 줄이고 L1 캐시 늘림)
};

enum AdaptiveCache { FIXED = 0, ADAPTIVE_CACHE = 1 };
// FIXED: 고정 캐시 크기, ADAPTIVE_CACHE: 런타임에 동적 조정
```

CUDA의 `cudaFuncSetCacheConfig()`에 대응. Fermi 이후 GPU에서 L1 캐시와 공유 메모리는 같은 온칩 SRAM을 분할하여 사용하므로, 비율을 조정할 수 있다.

---

## 103-109행: 주소 타입 정의 및 상수

```cpp
typedef unsigned long long new_addr_type;       // 메모리 주소 타입 (64비트)
typedef unsigned long long cudaTextureObject_t;  // CUDA 텍스처 객체 핸들
typedef unsigned long long address_type;         // 프로그램 카운터 등의 주소 타입
typedef unsigned long long addr_t;               // 범용 주소 타입

#define SPECIALIZED_UNIT_NUM 8     // 특수 기능 유닛의 최대 수 (사용자 정의 유닛)
#define SPEC_UNIT_START_ID 100     // 특수 유닛 연산 코드의 시작 ID
```

---

## 112-139행: 마이크로아키텍처 연산 코드 (`uarch_op_t`)

```cpp
enum uarch_op_t {
  NO_OP = -1,                // 연산 없음 (NOP)
  ALU_OP = 1,                // 일반 ALU 연산
  SFU_OP,                    // Special Function Unit 연산 (sin, cos, sqrt 등)
  TENSOR_CORE_OP,            // 텐서 코어 연산 (행렬 곱셈-누적)
  DP_OP,                     // 배정밀도(Double Precision) 부동소수점 연산
  SP_OP,                     // 단정밀도(Single Precision) 부동소수점 연산
  INTP_OP,                   // 정수(Integer) 연산
  ALU_SFU_OP,                // ALU + SFU 복합 연산
  LOAD_OP,                   // 메모리 로드 연산
  TENSOR_CORE_LOAD_OP,       // 텐서 코어용 로드 (wmma.load)
  TENSOR_CORE_STORE_OP,      // 텐서 코어용 스토어 (wmma.store)
  STORE_OP,                  // 메모리 스토어 연산
  BRANCH_OP,                 // 분기 명령어
  BARRIER_OP,                // 배리어 동기화 명령어 (__syncthreads)
  MEMORY_BARRIER_OP,         // 메모리 펜스 명령어 (__threadfence)
  CALL_OPS,                  // 함수 호출
  RET_OPS,                   // 함수 반환
  EXIT_OPS,                  // 스레드 종료
  SPECIALIZED_UNIT_1_OP = SPEC_UNIT_START_ID,  // 사용자 정의 특수 유닛 1-8
  SPECIALIZED_UNIT_2_OP,     // ...
  // ... 8개의 특수 유닛까지
};
```

타이밍 모델(성능 시뮬레이션)이 볼 수 있는 연산 유형. 각 연산은 서로 다른 파이프라인(SP, SFU, MEM 등)으로 라우팅되고 서로 다른 지연 시간을 갖는다.

---

## 142-186행: 배리어/리덕션/오퍼랜드/파이프라인 열거형

```cpp
enum uarch_bar_t { NOT_BAR = -1, SYNC = 1, ARRIVE, RED };
// NOT_BAR: 배리어 아님, SYNC: __syncthreads(), ARRIVE: 도착만 (대기 안 함), RED: 리덕션 배리어

enum uarch_red_t { NOT_RED = -1, POPC_RED = 1, AND_RED, OR_RED };
// 배리어 리덕션 연산 종류: POPC(population count), AND, OR

enum uarch_operand_type_t { UN_OP = -1, INT_OP, FP_OP };
// 오퍼랜드 타입: 정수 또는 부동소수점

enum special_operations_t {
  OTHER_OP, INT__OP, INT_MUL24_OP, INT_MUL32_OP, INT_MUL_OP, INT_DIV_OP,
  FP_MUL_OP, FP_DIV_OP, FP__OP, FP_SQRT_OP, FP_LG_OP, FP_SIN_OP, FP_EXP_OP,
  DP_MUL_OP, DP_DIV_OP, DP___OP, TENSOR__OP, TEX__OP
};
// 전력 모델에서 세분화된 연산 식별에 사용

enum operation_pipeline_t {
  UNKOWN_OP,          // 알 수 없는 파이프라인
  SP__OP,             // SP(단정밀도) 파이프라인
  DP__OP,             // DP(배정밀도) 파이프라인
  INTP__OP,           // 정수 파이프라인
  SFU__OP,            // SFU(특수 함수) 파이프라인
  TENSOR_CORE__OP,    // 텐서 코어 파이프라인
  MEM__OP,            // 메모리(로드/스토어) 파이프라인
  SPECIALIZED__OP,    // 사용자 정의 특수 파이프라인
};
// 명령어가 어떤 실행 파이프라인을 사용하는지 식별

enum _memory_op_t { no_memory_op = 0, memory_load, memory_store };
// ptxplus에서 사용하는 메모리 연산 타입
```

---

## 202-213행: `dim3comp` — 3D 좌표 비교기

```cpp
struct dim3comp {
  bool operator()(const dim3 &a, const dim3 &b) const {
    if (a.z < b.z) return true;       // z 먼저 비교
    else if (a.y < b.y) return true;  // 그 다음 y
    else if (a.x < b.x) return true;  // 마지막으로 x
    else return false;
  }
};
```

`std::map<dim3, ...>`에서 키로 dim3를 사용하기 위한 비교 연산자. CTA별 스트림 관리에 사용된다.

---

## 226-381행: `kernel_info_t` — CUDA 커널 정보 클래스

이 클래스는 하나의 CUDA 커널 론칭에 대한 모든 정보를 담는다.

```cpp
class kernel_info_t {
 public:
  // --- 생성자 ---
  kernel_info_t(dim3 gridDim, dim3 blockDim, class function_info *entry,
                unsigned long long streamID);
  // gridDim: 그리드 차원 (블록 수), blockDim: 블록 차원 (스레드 수)
  // entry: 커널 함수 엔트리 포인트, streamID: CUDA 스트림 ID

  // --- 코어 실행 관리 ---
  void inc_running() { m_num_cores_running++; }   // 이 커널을 실행 중인 코어 수 증가
  void dec_running() { m_num_cores_running--; }   // 코어 수 감소
  bool running() const { return m_num_cores_running > 0; }  // 실행 중인 코어가 있는지
  bool done() const { return no_more_ctas_to_run() && !running(); }
  // 커널 완료 조건: 더 이상 할당할 CTA가 없고, 실행 중인 코어도 없음

  // --- 커널 크기 정보 ---
  size_t num_blocks() const {
    return m_grid_dim.x * m_grid_dim.y * m_grid_dim.z;
    // 총 블록(CTA) 수 = gridDim.x × gridDim.y × gridDim.z
  }
  size_t threads_per_cta() const {
    return m_block_dim.x * m_block_dim.y * m_block_dim.z;
    // CTA당 스레드 수 = blockDim.x × blockDim.y × blockDim.z
  }

  // --- CTA 할당 관리 ---
  void increment_cta_id();          // 다음 CTA ID로 이동 (x→y→z 순서로 증가)
  dim3 get_next_cta_id() const;     // 다음에 할당할 CTA의 3D ID
  unsigned get_next_cta_id_single() const;  // 1D 선형 CTA ID로 변환
  bool no_more_ctas_to_run() const; // 모든 CTA가 이미 할당되었는지 확인
  // next_cta가 grid_dim 범위를 벗어나면 true

  // --- 스레드 ID 관리 ---
  void increment_thread_id();           // CTA 내 다음 스레드 ID로 이동
  unsigned get_next_thread_id() const;  // 1D 선형 스레드 ID
  bool more_threads_in_cta() const;     // 현재 CTA에 더 할당할 스레드가 있는지

  // --- 텍스처 바인딩 ---
  const struct cudaArray *get_texarray(const std::string &texname) const;
  const struct textureInfo *get_texinfo(const std::string &texname) const;
  // 커널 론칭 시점의 텍스처 매핑 스냅샷에서 이름으로 텍스처 정보를 조회

 private:
  class function_info *m_kernel_entry;  // PTX 커널 함수의 엔트리 포인트
  unsigned m_uid;                       // 커널 고유 ID (론칭 순서)
  unsigned long long m_streamID;        // CUDA 스트림 ID

  dim3 m_grid_dim;    // 그리드 차원 (<<<gridDim, blockDim>>>의 첫 번째 인자)
  dim3 m_block_dim;   // 블록 차원 (<<<gridDim, blockDim>>>의 두 번째 인자)
  dim3 m_next_cta;    // 다음에 할당할 CTA의 3D 인덱스 (라운드 로빈으로 SM에 분배)
  dim3 m_next_tid;    // 현재 CTA 내에서 다음에 할당할 스레드의 3D 인덱스

  unsigned m_num_cores_running;  // 이 커널을 실행 중인 SM(코어)의 수

  std::list<class ptx_thread_info *> m_active_threads;  // 활성 스레드 목록
  class memory_space *m_param_mem;  // 커널 파라미터가 저장된 메모리 공간

  // --- CDP(CUDA Dynamic Parallelism) 지원 ---
  kernel_info_t *m_parent_kernel;              // 부모 커널 (CDP로 론칭된 경우)
  dim3 m_parent_ctaid;                         // 부모의 CTA ID
  dim3 m_parent_tid;                           // 부모의 스레드 ID
  std::list<kernel_info_t *> m_child_kernels;  // 자식 커널 목록
  std::map<dim3, std::list<CUstream_st *>, dim3comp> m_cta_streams;
  // 각 CTA에서 생성된 스트림 목록 (CDP에서 디바이스 론칭한 스트림)

  // --- 커널 타이밍 ---
  unsigned long long launch_cycle;   // 커널이 론칭된 사이클
  unsigned long long start_cycle;    // 첫 CTA가 실행을 시작한 사이클
  unsigned long long end_cycle;      // 마지막 CTA가 완료된 사이클
  unsigned m_launch_latency;         // CPU→GPU 커널 론칭 지연 시간
  unsigned m_kernel_TB_latency;      // CPU-GPU 간 커널 전송 지연 (GPU 사이클 단위)
};
```

**핵심 동작 흐름:**
1. CPU가 `cudaLaunchKernel()` 호출 → `kernel_info_t` 생성
2. GPU 시뮬레이터가 `get_next_cta_id()`로 CTA를 SM에 할당
3. SM이 CTA 내 스레드를 워프로 그룹화하여 실행
4. 모든 CTA 완료 → `done()` = true

---

## 383-429행: `core_config` — SIMT 코어 설정

```cpp
class core_config {
 public:
  bool m_valid;                // 설정이 유효한지 여부
  unsigned warp_size;          // 워프 크기 (NVIDIA=32, AMD=64)
  class gpgpu_context *gpgpu_ctx;  // 시뮬레이터 컨텍스트 역참조

  // --- 메모리 합병(coalescing) 아키텍처 ---
  int gpgpu_coalesce_arch;     // 메모리 합병 방식 (세대별로 다름)

  // --- 공유 메모리 뱅크 충돌 검사 ---
  bool shmem_limited_broadcast;       // 제한된 브로드캐스트 지원 여부
  static const address_type WORD_SIZE = 4;  // 워드 크기 = 4바이트
  unsigned num_shmem_bank;            // 공유 메모리 뱅크 수 (기본 16)
  unsigned shmem_bank_func(address_type addr) const {
    return ((addr / WORD_SIZE) % num_shmem_bank);
    // 주소 → 뱅크 번호 매핑: (addr/4) mod num_banks
    // 이 함수로 뱅크 충돌을 감지한다
  }

  unsigned mem_warp_parts;    // 워프를 몇 부분으로 나눠 메모리 접근하는지
  unsigned gpgpu_shmem_size;  // 공유 메모리 크기 (바이트)
  unsigned gpgpu_shmem_sizeDefault;   // 기본 공유 메모리 크기
  unsigned gpgpu_shmem_sizePrefL1;    // L1 우선 시 공유 메모리 크기
  unsigned gpgpu_shmem_sizePrefShared;// 공유 메모리 우선 시 크기

  // --- 캐시 라인 크기 ---
  unsigned gpgpu_cache_texl1_linesize;    // 텍스처 L1 캐시 라인 크기
  unsigned gpgpu_cache_constl1_linesize;  // 상수 L1 캐시 라인 크기

  unsigned gpgpu_max_insn_issue_per_warp; // 워프당 사이클당 최대 명령어 발행 수
  bool gmem_skip_L1D;  // true면 글로벌 메모리 접근이 L1 캐시를 건너뜀
  bool adaptive_cache_config;  // 적응형 캐시 설정 사용 여부
};
```

---

## 431-481행: `simt_stack` — SIMT 재수렴 스택

```cpp
const unsigned MAX_WARP_SIZE = 32;                    // 최대 워프 크기 = 32 스레드
typedef std::bitset<MAX_WARP_SIZE> active_mask_t;     // 활성 마스크: 32비트, 각 비트=스레드 활성 여부
typedef std::bitset<MAX_WARP_SIZE> simt_mask_t;       // SIMT 마스크 (활성 마스크와 동일 크기)
typedef std::vector<address_type> addr_vector_t;      // PC 주소 벡터

class simt_stack {
 public:
  simt_stack(unsigned wid, unsigned warpSize, class gpgpu_sim *gpu);
  // wid: 워프 ID, warpSize: 워프 크기, gpu: GPU 시뮬레이터 참조

  void reset();  // 스택 초기화
  void launch(address_type start_pc, const simt_mask_t &active_mask);
  // 워프 론칭: 시작 PC와 활성 마스크를 스택에 푸시

  void update(simt_mask_t &thread_done, addr_vector_t &next_pc,
              address_type recvg_pc, op_type next_inst_op,
              unsigned next_inst_size, address_type next_inst_pc);
  // 분기 명령어 실행 후 스택 업데이트:
  //   thread_done: 종료된 스레드 마스크
  //   next_pc: 각 스레드의 다음 PC
  //   recvg_pc: 재수렴 지점 (즉시 후위 지배자의 PC)
  //   → 발산 시 스택에 새 엔트리 푸시, 재수렴 시 팝

  const simt_mask_t &get_active_mask() const;  // 현재 TOS의 활성 마스크 반환
  void get_pdom_stack_top_info(unsigned *pc, unsigned *rpc) const;
  // TOS의 PC와 재수렴 PC를 반환

 protected:
  unsigned m_warp_id;      // 이 스택이 속한 워프의 ID
  unsigned m_warp_size;    // 워프 크기

  struct simt_stack_entry {
    address_type m_pc;              // 이 엔트리의 다음 실행 PC
    unsigned int m_calldepth;       // 함수 호출 깊이 (call/ret 추적)
    simt_mask_t m_active_mask;      // 이 엔트리에서 활성인 스레드 비트마스크
    address_type m_recvg_pc;        // 재수렴 프로그램 카운터 (IPDOM PC)
    unsigned long long m_branch_div_cycle;  // 분기 발산이 발생한 사이클 (통계용)
    stack_entry_type m_type;        // NORMAL 또는 CALL (함수 호출 구분)
  };

  std::deque<simt_stack_entry> m_stack;  // 스택 본체 (deque로 구현)
  class gpgpu_sim *m_gpu;                // GPU 시뮬레이터 역참조
};
```

**SIMT 스택 동작 원리 (MICRO'07 PDOM 방식):**
1. 워프가 분기 명령어를 만남
2. 스레드들이 taken/not-taken으로 나뉨 (발산)
3. 즉시 후위 지배자(IPDOM)를 재수렴 지점으로 설정
4. 두 경로를 하나씩 직렬 실행 (TOS의 활성 마스크에 따라)
5. 두 경로 모두 재수렴 지점에 도달하면 엔트리 팝 → 모든 스레드 다시 합류

---

## 483-508행: GPU 메모리 공간 주소 상수

```cpp
const unsigned long long GLOBAL_HEAP_START = 0xC0000000;  // 전역 힙 시작 주소 (3GB)
const unsigned long long SHARED_MEM_SIZE_MAX = 96 * (1 << 10);  // 최대 공유 메모리 = 96KB (Volta)
const unsigned long long LOCAL_MEM_SIZE_MAX = 1 << 14;    // 최대 로컬 메모리 = 16KB/스레드 (Volta)
const unsigned MAX_STREAMING_MULTIPROCESSORS = 80;        // 최대 SM 수 = 80 (Volta Titan V)
const unsigned MAX_THREAD_PER_SM = 1 << 11;               // SM당 최대 스레드 = 2048
const unsigned MAX_WARP_PER_SM = 1 << 6;                  // SM당 최대 워프 = 64

// 전체 로컬 메모리 = 80 SM × 2048 스레드 × 16KB = 2.5GB
const unsigned long long TOTAL_LOCAL_MEM_PER_SM = MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX;
const unsigned long long TOTAL_SHARED_MEM = MAX_STREAMING_MULTIPROCESSORS * SHARED_MEM_SIZE_MAX;
const unsigned long long TOTAL_LOCAL_MEM = MAX_STREAMING_MULTIPROCESSORS * MAX_THREAD_PER_SM * LOCAL_MEM_SIZE_MAX;

// 주소 공간 배치 (높은 주소부터):
//   GLOBAL_HEAP_START (0xC0000000) ← 글로벌 힙 시작
//   SHARED_GENERIC_START           ← 공유 메모리 제네릭 주소 시작
//   LOCAL_GENERIC_START            ← 로컬 메모리 제네릭 주소 시작
//   STATIC_ALLOC_LIMIT             ← 정적 할당 상한
const unsigned long long SHARED_GENERIC_START = GLOBAL_HEAP_START - TOTAL_SHARED_MEM;
const unsigned long long LOCAL_GENERIC_START = SHARED_GENERIC_START - TOTAL_LOCAL_MEM;
const unsigned long long STATIC_ALLOC_LIMIT = GLOBAL_HEAP_START - (TOTAL_LOCAL_MEM + TOTAL_SHARED_MEM);
```

**주소 공간 레이아웃 (높은 주소 → 낮은 주소):**
```
0xFFFFFFFF ┌─────────────────┐
           │                 │
0xC0000000 ├─────────────────┤ ← GLOBAL_HEAP_START (글로벌 메모리 힙)
           │ 공유 메모리 영역  │
           ├─────────────────┤ ← SHARED_GENERIC_START
           │ 로컬 메모리 영역  │
           ├─────────────────┤ ← LOCAL_GENERIC_START
           │ 정적 할당 영역    │
           ├─────────────────┤ ← STATIC_ALLOC_LIMIT
           │ .ptx 전역 변수   │
0x00000000 └─────────────────┘
```

---

## 538-694행: `gpgpu_functional_sim_config` & `gpgpu_t` — 기능 시뮬레이션 설정 및 GPU 추상 클래스

```cpp
class gpgpu_functional_sim_config {
  // PTX 관련 옵션들:
  int m_ptx_convert_to_ptxplus;       // PTX → PTXPlus 변환 여부
  int m_ptx_use_cuobjdump;            // cuobjdump 사용 여부
  unsigned m_ptx_force_max_capability; // 강제할 최대 SM 호환성 버전
  unsigned m_texcache_linesize;        // 텍스처 캐시 라인 크기
  // 체크포인트/재개 관련 옵션들...
};

class gpgpu_t {
  // GPU의 기능적(functional) 측면을 추상화한 기본 클래스
  // gpgpu_sim이 이 클래스를 상속하여 타이밍 시뮬레이션을 추가

  unsigned long long gpu_sim_cycle;      // 현재 시뮬레이션 사이클
  unsigned long long gpu_tot_sim_cycle;  // 총 시뮬레이션 사이클

  // --- GPU 메모리 관리 함수 ---
  void *gpu_malloc(size_t size);         // GPU 메모리 할당 (cudaMalloc 구현)
  void gpu_memset(size_t dst, int c, size_t count);  // GPU 메모리 초기화
  void memcpy_to_gpu(size_t dst, const void *src, size_t count);    // Host→Device 복사
  void memcpy_from_gpu(void *dst, size_t src, size_t count);        // Device→Host 복사
  void memcpy_gpu_to_gpu(size_t dst, size_t src, size_t count);     // Device→Device 복사

  // --- 메모리 공간 접근 ---
  class memory_space *m_global_mem;   // 글로벌 메모리 (DRAM 시뮬레이션)
  class memory_space *m_tex_mem;      // 텍스처 메모리
  class memory_space *m_surf_mem;     // 서피스 메모리

  unsigned long long m_dev_malloc;    // 다음 GPU 메모리 할당 주소 (힙 포인터)

  // --- 텍스처 바인딩 관리 ---
  std::map<std::string, std::set<const struct textureReference *>> m_NameToTextureRef;
  std::map<const struct textureReference *, std::string> m_TextureRefToName;
  std::map<std::string, const struct cudaArray *> m_NameToCudaArray;
  std::map<std::string, const struct textureInfo *> m_NameToTextureInfo;
};
```

---

## 696-719행: PTX 시뮬레이션 정보 구조체

```cpp
struct gpgpu_ptx_sim_info {
  int lmem;            // 로컬 메모리 사용량 (바이트)
  int smem;            // 공유 메모리 사용량 (바이트)
  int cmem;            // 상수 메모리 사용량 (바이트)
  int gmem;            // 글로벌 메모리 사용량 (바이트)
  int regs;            // 레지스터 사용량 (레지스터 수)
  unsigned maxthreads; // 최대 스레드 수
  unsigned ptx_version;// PTX 버전
  unsigned sm_target;  // 대상 SM 아키텍처 (예: 70 = Volta)
};
// ptxinfo 파일에서 파싱한 커널 리소스 사용 정보
// nvcc의 --ptxas-options=-v 출력에 해당

struct gpgpu_ptx_sim_arg {
  const void *m_start;  // 커널 인자의 시작 주소
  size_t m_nbytes;      // 인자 크기 (바이트)
  size_t m_offset;      // 파라미터 공간에서의 오프셋
};
// CUDA 커널 론칭 시 전달되는 각 인자를 표현
```

---

## 723-762행: `memory_space_t` — 메모리 공간 타입 래퍼

```cpp
class memory_space_t {
  enum _memory_space_t m_type;  // 메모리 공간 종류 (global, shared, local 등)
  unsigned m_bank;              // 상수 메모리 뱅크 번호 (.const[n]에서 n)
                                // .const == .const[0] (PTX 2.1 매뉴얼 sec 5.1.3)

  bool is_const() const;  // 상수 메모리 또는 커널 파라미터 공간인지
  bool is_local() const;  // 로컬 메모리 또는 로컬 파라미터 공간인지
  bool is_global() const; // 글로벌 메모리인지
};
```

---

## 764-893행: 메모리 접근 관련 타입 및 `mem_access_t`

```cpp
const unsigned MAX_MEMORY_ACCESS_SIZE = 128;  // 최대 메모리 접근 크기 = 128바이트
typedef std::bitset<MAX_MEMORY_ACCESS_SIZE> mem_access_byte_mask_t;
// 128비트 바이트 마스크: 128바이트 캐시 라인 중 어떤 바이트가 접근되었는지

const unsigned SECTOR_CHUNCK_SIZE = 4;  // 섹터 수 = 4개
const unsigned SECTOR_SIZE = 32;        // 섹터 크기 = 32바이트
typedef std::bitset<SECTOR_CHUNCK_SIZE> mem_access_sector_mask_t;
// 4비트 섹터 마스크: 128바이트 캐시 라인의 4개 32바이트 섹터 중 어떤 것이 접근되었는지

// 메모리 접근 유형 열거형:
enum mem_access_type {
  GLOBAL_ACC_R,    // 글로벌 메모리 읽기
  LOCAL_ACC_R,     // 로컬 메모리 읽기
  CONST_ACC_R,     // 상수 메모리 읽기
  TEXTURE_ACC_R,   // 텍스처 메모리 읽기
  GLOBAL_ACC_W,    // 글로벌 메모리 쓰기
  LOCAL_ACC_W,     // 로컬 메모리 쓰기
  L1_WRBK_ACC,     // L1 캐시 쓰기 백(write-back)
  L2_WRBK_ACC,     // L2 캐시 쓰기 백
  INST_ACC_R,      // 명령어 페치 (읽기)
  L1_WR_ALLOC_R,   // L1 쓰기 할당을 위한 읽기
  L2_WR_ALLOC_R,   // L2 쓰기 할당을 위한 읽기
  NUM_MEM_ACCESS_TYPE
};

// 캐시 연산자 (PTX ISA의 캐시 힌트):
enum cache_operator_type {
  CACHE_ALL,        // .ca — 모든 레벨에 캐시
  CACHE_LAST_USE,   // .lu — 마지막 사용 (이후 축출 우선)
  CACHE_VOLATILE,   // .cv — 항상 메모리에서 읽기 (캐시 무시)
  CACHE_L1,         // .nc — L1에만 캐시 (비일관적)
  CACHE_STREAMING,  // .cs — 스트리밍 접근 (캐시 오염 최소화)
  CACHE_GLOBAL,     // .cg — 글로벌 수준에서만 캐시 (L1 바이패스)
  CACHE_WRITE_BACK, // .wb — 쓰기 백 정책
  CACHE_WRITE_THROUGH // .wt — 쓰기 스루 정책
};

class mem_access_t {
  // 하나의 합병된(coalesced) 메모리 접근을 표현
  unsigned m_uid;                         // 접근 고유 ID
  new_addr_type m_addr;                   // 요청 주소
  bool m_write;                           // 쓰기 여부 (true=store, false=load)
  unsigned m_req_size;                    // 요청 크기 (바이트)
  mem_access_type m_type;                 // 접근 유형 (GLOBAL_ACC_R 등)
  active_mask_t m_warp_mask;              // 이 접근에 참여하는 스레드 마스크
  mem_access_byte_mask_t m_byte_mask;     // 접근된 바이트 마스크
  mem_access_sector_mask_t m_sector_mask; // 접근된 섹터 마스크
};
```

**메모리 합병(coalescing) 과정:**
1. 워프의 32개 스레드가 각각 메모리 주소를 생성
2. 하드웨어가 이 주소들을 캐시 라인 단위로 그룹화
3. 같은 캐시 라인에 속하는 접근들을 하나의 `mem_access_t`로 합침
4. 서로 다른 캐시 라인에 대한 접근은 별도의 `mem_access_t` 생성

---

## 897-919행: `mem_fetch_interface` & `mem_fetch_allocator` — 메모리 요청 인터페이스

```cpp
class mem_fetch_interface {
  virtual bool full(unsigned size, bool write) const = 0;  // 큐가 가득 찼는지
  virtual void push(mem_fetch *mf) = 0;                    // 메모리 요청을 큐에 추가
};
// 메모리 시스템의 각 단계(L1→상호연결→L2→DRAM) 사이의 인터페이스

class mem_fetch_allocator {
  virtual mem_fetch *alloc(...) const = 0;  // mem_fetch 객체 생성 (팩토리 패턴)
};
// 메모리 요청 패킷(mem_fetch)의 할당자
```

---

## 937-1055행: `inst_t` — 디코딩된 명령어

```cpp
class inst_t {
  // 하나의 디코딩된 PTX/SASS 명령어를 표현
  // warp_inst_t의 부모 클래스

  address_type pc;                    // 프로그램 카운터 (명령어 주소)
  unsigned isize;                     // 명령어 크기 (바이트)
  op_type op;                         // 마이크로아키텍처 연산 코드 (ALU_OP, LOAD_OP 등)

  barrier_type bar_type;              // 배리어 유형 (SYNC, ARRIVE, RED)
  reduction_type red_type;            // 리덕션 유형 (POPC, AND, OR)
  unsigned bar_id;                    // 배리어 ID (명명된 배리어 식별)
  unsigned bar_count;                 // 배리어에 참여하는 스레드 수

  types_of_operands oprnd_type;       // 오퍼랜드 타입 (INT_OP 또는 FP_OP)
  special_ops sp_op;                  // 세부 연산 식별 (전력 모델용)
  operation_pipeline op_pipe;         // 파이프라인 식별 (SP, SFU, MEM 등)
  mem_operation mem_op;               // 텍스처 접근 여부 (TEX 또는 NOT_TEX)
  bool const_cache_operand;           // 상수 캐시에서 읽는 오퍼랜드가 있는지

  unsigned num_operands;              // 오퍼랜드 수
  unsigned num_regs;                  // 레지스터 오퍼랜드 수

  address_type reconvergence_pc;      // 재수렴 PC (-1: 분기 아님, -2: 함수 반환 주소 사용)

  unsigned out[8];                    // 출력 레지스터 번호 (최대 8개)
  unsigned in[24];                    // 입력 레지스터 번호 (최대 24개)
  int pred;                           // 프레디케이트 레지스터 번호
  int ar1, ar2;                       // 주소 레지스터 번호

  struct {
    int dst[MAX_REG_OPERANDS];        // 목적지 아키텍처 레지스터 (뱅크 충돌 평가용)
    int src[MAX_REG_OPERANDS];        // 소스 아키텍처 레지스터
  } arch_reg;

  unsigned latency;                   // 실행 지연 시간 (사이클)
  unsigned initiation_interval;       // 파이프라인 개시 간격 (사이클)
  unsigned data_size;                 // 연산 데이터 크기 (바이트)
  memory_space_t space;               // 메모리 접근 공간
  cache_operator_type cache_op;       // 캐시 연산 힌트 (.ca, .cg 등)

  // 편의 쿼리 함수들:
  bool is_load() const;   // 로드 명령어인지 (LOAD_OP, TENSOR_CORE_LOAD_OP, memory_load)
  bool is_store() const;  // 스토어 명령어인지
  bool is_sfu() const;    // SFU 연산인지 (sqrt, log, sin, exp, tensor)
  bool is_alu() const;    // 정수 ALU 연산인지
};
```

---

## 1057-1289행: `warp_inst_t` — 워프 수준 명령어

```cpp
class warp_inst_t : public inst_t {
  // inst_t를 상속하여 워프 전체의 실행 상태를 추가
  // 시뮬레이터에서 실제 파이프라인을 흐르는 단위

  // --- 핵심 상태 ---
  unsigned m_uid;                     // 발행된 명령어의 고유 ID (순서 추적)
  unsigned m_warp_id;                 // 이 명령어가 속한 워프 ID
  unsigned m_dynamic_warp_id;         // 동적 워프 ID (DWF 등에서 사용)
  bool m_empty;                       // 빈 슬롯인지 (파이프라인 버블)
  bool m_isatomic;                    // 원자적 연산인지

  active_mask_t m_warp_active_mask;   // 동적 활성 마스크 (프레디케이션 후)
  // 32비트: 각 비트가 해당 레인(스레드)의 활성 여부
  // 분기 발산으로 일부 스레드가 마스크 오프될 수 있음

  active_mask_t m_warp_issued_mask;   // 발행 시점의 활성 마스크 (프레디케이션 전)
  // 명령어 카운팅 통계에 사용

  unsigned long long issue_cycle;     // 발행된 사이클
  unsigned cycles;                    // 개시 간격 지연 카운터

  // --- 스레드별 정보 ---
  struct per_thread_info {
    dram_callback_t callback;         // DRAM 접근 완료 시 콜백 함수
    new_addr_type memreqaddr[MAX_ACCESSES_PER_INSN_PER_THREAD];
    // 스레드별 유효 주소 (최대 8개 — 32바이트 접근을 4바이트씩 8청크로)
  };
  std::vector<per_thread_info> m_per_scalar_thread;  // 32개 스레드의 개별 정보

  // --- 메모리 접근 큐 ---
  std::list<mem_access_t> m_accessq;  // 합병된 메모리 접근 요청 큐
  // generate_mem_accesses()가 32개 스레드의 주소를 합병하여 이 큐를 채움

  // --- 주요 메서드 ---
  void issue(const active_mask_t &mask, unsigned warp_id,
             unsigned long long cycle, int dynamic_warp_id,
             int sch_id, unsigned long long streamID);
  // 명령어 발행: 활성 마스크, 워프 ID, 사이클 등을 설정

  void generate_mem_accesses();
  // 32개 스레드의 메모리 주소를 캐시 라인 단위로 합병(coalescing)하여
  // m_accessq에 mem_access_t 객체들을 생성

  void memory_coalescing_arch(bool is_write, mem_access_type access_type);
  // 아키텍처별 메모리 합병 로직 실행

  struct transaction_info {
    std::bitset<4> chunks;            // 접근된 32바이트 청크 비트마스크
    mem_access_byte_mask_t bytes;     // 접근된 바이트 마스크
    active_mask_t active;             // 이 트랜잭션에 참여하는 스레드 마스크
  };
  // 메모리 합병 과정에서 각 캐시 라인 트랜잭션의 정보를 추적

  // CDP(CUDA Dynamic Parallelism) 지원:
  int m_is_cdp;        // 0: 일반, 1: CDP 관련 명령어

  // Ampere+ 비동기 복사 지원:
  bool m_is_ldgsts;    // ldgsts (비동기 글로벌→공유 복사) 명령어인지
  bool m_is_ldgdepbar; // ldgdepbar (비동기 복사 의존성 배리어) 명령어인지
  bool m_is_depbar;    // depbar (의존성 배리어) 명령어인지
};
```

---

## 1294-1376행: `core_t` — GPU 코어 추상 기본 클래스

```cpp
class core_t {
  // GPU 코어(SM)의 기능적 시뮬레이션을 위한 기본 클래스
  // shader_core_ctx (shader.h)가 이를 상속하여 타이밍 시뮬레이션 추가

  class gpgpu_sim *m_gpu;              // GPU 시뮬레이터 참조
  kernel_info_t *m_kernel;             // 현재 실행 중인 커널 정보

  simt_stack **m_simt_stack;           // 워프별 SIMT 재수렴 스택 배열
  // m_simt_stack[warp_id] → 해당 워프의 PDOM 스택

  class ptx_thread_info **m_thread;    // 스레드별 PTX 실행 상태 배열
  // m_thread[thread_id] → 해당 스레드의 레지스터, PC 등

  unsigned m_warp_size;                // 워프 크기 (보통 32)
  unsigned m_warp_count;               // 코어 내 워프 수 = threads_per_shader / warp_size

  unsigned reduction_storage[MAX_CTA_PER_SHADER][MAX_BARRIERS_PER_CTA];
  // 배리어 리덕션 연산 결과 저장소 — CTA별, 배리어별

  // --- 핵심 메서드 ---
  void execute_warp_inst_t(warp_inst_t &inst, unsigned warpId);
  // 명령어의 기능적 실행 (PTX 시뮬레이터 호출)

  void updateSIMTStack(unsigned warpId, warp_inst_t *inst);
  // 분기 명령어 실행 후 SIMT 스택 업데이트

  void initilizeSIMTStack(unsigned warp_count, unsigned warps_size);
  // 모든 워프의 SIMT 스택 초기화

  warp_inst_t getExecuteWarp(unsigned warpId);
  // 워프의 다음 실행 명령어를 SIMT 스택에서 가져옴
};
```

---

## 1378-1532행: `register_set` — 파이프라인 레지스터 세트

```cpp
class register_set {
  // 파이프라인 스테이지 간 명령어를 전달하는 레지스터 세트
  // 여러 warp_inst_t 슬롯을 포함하여, 한 사이클에 여러 명령어가 통과 가능
  // 실제 GPU의 파이프라인 레지스터(ID/RR, RR/EX 등)에 해당

  std::vector<warp_inst_t *> regs;  // 명령어 슬롯 배열
  const char *m_name;               // 레지스터 세트 이름 (디버깅용, 예: "ID_OC_SP")

  // --- 슬롯 관리 ---
  bool has_free();                    // 빈 슬롯이 있는지
  bool has_free(bool sub_core_model, unsigned reg_id);
  // 서브코어 모델에서는 각 스케줄러가 특정 슬롯만 사용 가능

  bool has_ready();                   // 발행 대기 중인 명령어가 있는지

  unsigned get_ready_reg_id();        // 가장 오래된 명령어의 슬롯 ID 반환
  // UID가 작을수록 먼저 발행된 명령어 → 가장 오래된 것 우선

  // --- 명령어 이동 ---
  void move_in(warp_inst_t *&src);    // 명령어를 빈 슬롯에 넣기
  void move_out_to(warp_inst_t *&dest); // 가장 오래된 명령어를 꺼내기

  // --- 서브코어 모델 지원 ---
  // sub_core_model=true이면, 각 워프 스케줄러(reg_id로 식별)가
  // 자기 전용 슬롯만 사용. Volta+에서 4개 서브코어 파티션에 해당.

  warp_inst_t **get_free();           // 빈 슬롯의 포인터 반환
  warp_inst_t **get_ready();          // 가장 오래된 명령어 슬롯의 포인터 반환
};
```

**register_set의 파이프라인 내 위치:**
```
[Fetch] → [I-Cache] → [Decode] → [I-Buffer]
    → register_set("ID_OC_SP") → [Operand Collector]
    → register_set("OC_EX_SP") → [Execute (SP/SFU/MEM)]
    → register_set("EX_WB")    → [Writeback]
```

---

## 1534-1536행: 인클루드 가드 닫기

```cpp
#endif  // #ifdef __cplusplus
#endif  // #ifndef ABSTRACT_HARDWARE_MODEL_INCLUDED
```

---

## 전체 클래스 관계도

```
gpgpu_t (기능적 GPU 추상 클래스)
  └── gpgpu_sim (타이밍 시뮬레이션 GPU — gpu-sim.h에서 정의)
        ├── kernel_info_t (커널 정보)
        │     └── function_info (PTX 함수 엔트리)
        ├── core_t (코어 추상 클래스)
        │     └── shader_core_ctx (SIMT 코어 — shader.h에서 정의)
        │           ├── simt_stack[] (워프별 PDOM 스택)
        │           ├── register_set[] (파이프라인 레지스터)
        │           └── warp_inst_t (파이프라인을 흐르는 명령어)
        │                 └── inst_t (디코딩된 명령어 기본)
        │                       └── mem_access_t (합병된 메모리 접근)
        └── memory_space (시뮬레이션된 메모리)
```
