# GPGPU-Sim으로 배우는 GPU 아키텍처 — 학습 경로 가이드 (README)

> 이 문서는 `deep/analysis/gpgpu-sim/` 아래의 모든 분석 문서를 **초보 → 심화 순서**로 묶는
> 인덱스이자 학습 로드맵이다. "GPU 아키텍처를 GPGPU-Sim 코드로 이해한다"는 목표를 위해
> 어떤 문서를 어떤 순서로, 어떤 선수지식을 가지고 읽어야 하는지를 안내한다.
>
> - 대상 독자: GPU 아키텍처를 처음 깊게 파는 사람 (C/C++ 기본 + CUDA 개념 약간)
> - 분석 대상 코드: `gpgpu-sim-study/src/` (GPGPU-Sim 4.0 / Accel-Sim)
> - 참고 논문: ISCA 2020 *Accel-Sim: An Extensible Simulation Framework for Validated GPU Modeling*
> - 자매 자료: `deep/notes/gpu-storage/` 의 GPGPU-Sim 가이드 시리즈 (아래 §6)

---

## 0. 30초 요약: 무엇을 어떤 순서로 읽나

```
[기초 이론]        00  GPU 아키텍처 기초 (코드 없이 GPU 자체를 이해)
                       │
[시뮬레이터 입문]  01  GPGPU-Sim 전체 구조 + 메인 루프 + CUDA 가로채기
                       │
                ┌──────┴───────┐
[기능 모델]     05            02  [타이밍 모델: SM 파이프라인]
 PTX 실행/SIMT               warp 스케줄러/스코어보드/SIMT 스택
                └──────┬───────┘
                       │
[메모리 계층]      03  L1/L2/DRAM 캐시 + 주소 디코딩
                       │
[인터커넥트]       04  BookSim NoC (VC/flit/라우터)
                       │
[전체 통합]        06  Load/Store I/O 흐름 (코어→캐시→NoC→DRAM 한 바퀴)
                       │
[실습]             07  빌드/실행/gpgpusim.config 해석
                       │
[심화]             08  AccelWattch 전력 모델
```

**가장 빠른 길**: 시간이 없다면 `00 → 01 → 06` 만 읽어도 "GPU가 어떻게 생겼고, 시뮬레이터가
이를 어떻게 모델링하며, 메모리 요청 하나가 칩 전체를 어떻게 도는지"의 큰 그림이 잡힌다.

---

## 1. 이 폴더에 무엇이 있나

| 분류 | 파일 | 한 줄 설명 |
|------|------|-----------|
| **이론** | [[00_gpu_architecture_fundamentals]] | 시뮬레이터와 무관하게 GPU 하드웨어 자체를 화이트페이퍼 수준으로 설명 |
| **시뮬레이터 개요** | [[01_architecture_overview]] | GPGPU-Sim의 모듈 구조, 초기화, `cycle()` 메인 루프, CUDA API 인터셉션 |
| **타이밍: 코어** | [[02_core_pipeline_analysis]] | SM 파이프라인 6단계, 워프 스케줄러 6종, 스코어보드, SIMT 스택 |
| **타이밍: 메모리** | [[03_memory_hierarchy]] | L1/L2 캐시, MSHR, DRAM 컨트롤러(FR-FCFS), 주소 디코딩 |
| **타이밍: 네트워크** | [[04_interconnect_network]] | BookSim2 기반 NoC, 가상 채널, flit, 라우터 파이프라인 |
| **기능 모델** | [[05_cuda_execution_model]] | PTX 파싱/실행, 메모리 공간, 커널 실행, 명령어별 시맨틱 |
| **통합 흐름** | [[06_io_flow_analysis]] | 한 개의 Load/Store/Atomic 요청이 칩 전체를 도는 end-to-end 경로 |
| **실습** | [[07_benchmark_scenarios]] | 빌드·실행 절차, `gpgpusim.config` 파라미터 라인별 해석 |
| **전력** | [[08_power_model_accelwattch]] | McPAT 기반 AccelWattch 동적/정적 전력 모델 |
| **주석 원본** | `abstract_hardware_model_h_annotated.md` | 추상 HW 모델 헤더 한국어 주석본 (02·05의 부교재) |
| **주석 원본** | `shader_h_annotated.h` / `shader_cc_annotated.cc` | SM 파이프라인 핵심 코드 한국어 주석본 (02의 부교재) |

> 위키링크(`[[...]]`)는 옵시디언에서 클릭 이동된다. 일반 마크다운에서는 같은 폴더의 동명 `.md`
> 파일을 가리킨다.

---

## 2. 단계별 학습 경로 (상세)

각 단계마다 **무엇을 배우나 / 선수지식 / 핵심 개념 / 대응 소스코드 / 분량**을 정리했다.
분량은 대략적인 읽기 시간 기준이다(1단계 ≈ 1~2회 학습 세션).

---

### 0단계 — GPU 아키텍처 기초 이론 〔필수 출발점〕

- 문서: **[[00_gpu_architecture_fundamentals]]** (4,217줄 / 251 KB — 가장 큼, 나눠 읽기 권장)
- 선수지식: 없음. C 기본만 있으면 됨.
- 무엇을 배우나: GPGPU-Sim 코드를 보기 전에 **GPU라는 하드웨어 자체**를 이해한다.
  시뮬레이터 의존성이 전혀 없는 순수 아키텍처 교재 역할.
- 핵심 개념:
  - CPU vs GPU 설계 철학 (지연 최적화 vs 처리량 최적화)
  - HW 계층: GPC → SM → 처리 블록 → CUDA 코어 (V100 실제 예시)
  - CUDA 모델 ↔ HW 매핑: Grid/Block/Warp/Thread, occupancy, block 크기와 32배수
  - **Warp & SIMT**: 32스레드 실행 단위, SIMD와의 차이
  - **Warp Divergence**: active mask, SIMT 스택 재수렴, Volta independent thread scheduling
  - 메모리 계층: 레지스터 → 공유메모리/L1 → L2 → DRAM
- 대응 소스코드: (이론서이므로 직접 대응 없음 — 이후 단계에서 코드로 재확인)
- 자매 노트: `notes/gpu-storage/GPGPU_Architecture_Korean.md` (Aamodt 교과서 정리),
  `notes/gpu-storage/gpu-architecture-scaling-book.md` (JAX Scaling Book 요약)
- 읽고 나면 답할 수 있어야 할 질문:
  - "왜 GPU는 캐시가 작고 코어가 많은가?"
  - "warp divergence는 왜 성능을 떨어뜨리고 어떻게 재수렴하는가?"
  - "occupancy를 결정하는 자원 4가지는?"

> 팁: 00번은 매우 길다. §1~§6(설계 철학·SM·CUDA 모델·Warp·Divergence)까지만 먼저 읽고
> 메모리 계층(§7~)은 3단계 직전에 다시 봐도 좋다.

---

### 1단계 — 시뮬레이터 전체 구조 잡기

- 문서: **[[01_architecture_overview]]** (1,115줄 / 52 KB)
- 선수지식: 0단계 §1~§6
- 무엇을 배우나: GPGPU-Sim이 어떻게 구성되어 있고, 한 사이클이 어떻게 도는지 큰 지도를 그린다.
- 핵심 개념:
  - 디렉토리/모듈 지도: `cuda-sim/`(기능) vs `gpgpu-sim/`(타이밍)의 분리
  - **`gpgpu_sim::cycle()` 메인 루프** — 클록 도메인(Core:ICNT:L2:DRAM)별 진행
  - `issue_block2core()` — CTA(스레드 블록)를 SM에 배정
  - **CUDA API 인터셉션**: 실제 CUDA 바이너리의 `cuLaunchKernel`을 가로채는 원리
  - 핵심 자료구조 첫 등장: `kernel_info_t`, `warp_inst_t`, `mem_fetch`
- 대응 소스코드:
  - `gpgpu-sim-study/src/gpgpu-sim/gpu-sim.cc` (`cycle()`, `init()`, `launch()`)
  - `gpgpu-sim-study/src/gpgpusim_entrypoint.cc`
  - `gpgpu-sim-study/src/libcuda/cuda_runtime_api.cc`
- 자매 노트: `notes/gpu-storage/gpgpu-sim-architecture-guide.md` (Part 1 — 개요·범위·역사)
- 읽고 나면: "실제 CUDA 프로그램이 어떻게 시뮬레이터로 흘러 들어가고, 한 사이클에 어떤
  서브시스템들이 순서대로 도는가?"를 설명할 수 있어야 한다.

---

### 2단계(A) — 기능 모델: 명령어가 실제로 실행되는 곳

- 문서: **[[05_cuda_execution_model]]** (1,395줄 / 58 KB)
- 선수지식: 0단계 전체, 1단계
- 무엇을 배우나: 타이밍과 분리된 **기능 시뮬레이션**(`cuda-sim/`)이 PTX 명령어를 실제로
  실행해 결과값을 만드는 과정. "무엇이 계산되는가"를 담당.
- 핵심 개념:
  - PTX vs PTXPlus vs SASS, 가상 ISA의 의미
  - PTX 파싱 → assemble → pre-decode 파이프라인
  - 명령어 시맨틱: ADD/MUL/LD/ST/ATOM/BRA/EXIT/BAR 구현
  - 메모리 공간(global/shared/local/const/tex) 추상화와 주소 변환
  - **기능 레벨의 SIMT 스택**과 분기 처리 (`ptx_exec_inst`)
- 대응 소스코드:
  - `src/cuda-sim/cuda-sim.cc` (`ptx_exec_inst`), `src/cuda-sim/instructions.cc`
  - `src/cuda-sim/ptx_ir.{cc,h}`, `src/cuda-sim/memory.{cc,h}`
- 자매 노트: `notes/gpu-storage/gpgpu-sim-cuda-power-guide.md` (Part 3 — CUDA/PTX 실행)
- 왜 02보다 먼저 보는가: 타이밍 모델(02)이 "언제" 명령어가 진행되는지를 다룬다면,
  여기서는 "무엇이 실행되는지"를 다룬다. 값 계산을 먼저 이해하면 타이밍이 덜 추상적이다.

### 2단계(B) — 타이밍 모델: SM(셰이더 코어) 파이프라인 〔이 프로젝트의 정수〕

- 문서: **[[02_core_pipeline_analysis]]** (1,398줄 / 63 KB)
- 선수지식: 0단계(특히 §5 Warp, §6 Divergence), 1단계, 가능하면 2단계(A)
- 무엇을 배우나: 사이클 단위로 동작하는 SM 파이프라인 — GPGPU-Sim의 심장부.
- 핵심 개념:
  - 파이프라인 6단계: Fetch → Decode → Issue → Read Operands → Execute → Writeback
  - **워프 스케줄러 6종**: LRR / GTO / Two-Level / RRR / OldestFirst / SWL
  - **SIMT 스택** 타이밍 구현 + post-dominator 기반 재수렴 (`simt_stack::update`)
  - **Scoreboard**: RAW 해저드 감지 (`reserveRegisters`/`checkCollision`)
  - Operand Collector: 레지스터 뱅크 충돌 처리
- 대응 소스코드 (부교재 주석본 함께 보기):
  - `src/gpgpu-sim/shader.cc` ↔ `shader_cc_annotated.cc`
  - `src/gpgpu-sim/shader.h` ↔ `shader_h_annotated.h`
  - `src/abstract_hardware_model.h` ↔ `abstract_hardware_model_h_annotated.md`
  - `src/gpgpu-sim/scoreboard.{cc,h}`
- 읽고 나면: "한 warp 명령어가 fetch부터 writeback까지 어떤 자원 검사를 거치며,
  스케줄러가 매 사이클 어떤 warp를 고르는가?"를 단계별로 설명할 수 있어야 한다.

---

### 3단계 — 메모리 계층 (캐시 → DRAM)

- 문서: **[[03_memory_hierarchy]]** (925줄 / 43 KB)
- 선수지식: 2단계(B). (`mem_fetch` 개념은 1단계에서 첫 등장)
- 무엇을 배우나: SM이 만들어낸 메모리 요청이 캐시 계층과 DRAM에서 처리되는 타이밍.
- 핵심 개념:
  - 캐시 클래스 계층 (`cache_t` → baseline/read_only/data/L1/L2/tex)
  - 캐시 블록 상태 머신, 교체 정책, 세트 인덱스 함수
  - **MSHR** (Miss Status Holding Register) — 미스 병합
  - L2 캐시 + 메모리 파티션/서브파티션 구조와 중재
  - **DRAM 컨트롤러**: 타이밍 파라미터, `dram_t::cycle()`, 명령 시퀀스
  - 주소 디코딩(`addrdec`): 주소 → 채널/뱅크/행/열 매핑
  - `mem_fetch` 패킷 생명주기와 상태 전이
- 대응 소스코드:
  - `src/gpgpu-sim/gpu-cache.{cc,h}`, `src/gpgpu-sim/l2cache.{cc,h}`
  - `src/gpgpu-sim/dram.{cc,h}`, `src/gpgpu-sim/dram_sched.{cc,h}` (FR-FCFS)
  - `src/gpgpu-sim/addrdec.{cc,h}`, `src/gpgpu-sim/mem_fetch.{cc,h}`
- 자매 노트: `notes/gpu-storage/gpgpu-sim-memory-guide.md` (Part 2 — 메모리 시스템)

---

### 4단계 — 인터커넥트 네트워크 (NoC)

- 문서: **[[04_interconnect_network]]** (1,271줄 / 54 KB)
- 선수지식: 3단계 (요청이 어디서 어디로 가는지 알아야 함)
- 무엇을 배우나: SM 클러스터 ↔ 메모리 파티션을 잇는 칩 내부 네트워크.
- 핵심 개념:
  - 듀얼 서브넷 구조 (요청망/응답망)와 `icnt_wrapper` 함수 포인터 추상화
  - BookSim2 통합(`InterconnectInterface`): `Push`/`Pop`, 노드 매핑
  - 토폴로지 (Mesh/CMesh 등), IQ 라우터 파이프라인
  - **가상 채널(VC)**, 크레딧 기반 흐름 제어, 버퍼 정책
  - **Flit** 단위 전송, 패킷 분할/재조립(BoundaryBuffer)
- 대응 소스코드:
  - `src/gpgpu-sim/icnt_wrapper.{cc,h}`, `src/gpgpu-sim/local_interconnect.{cc,h}`
  - `src/intersim2/` (BookSim2 전체 — interconnect_interface, vc, flit, buffer, routefunc 등)

---

### 5단계 — 전체 통합: I/O 흐름 한 바퀴 〔지식 결합점〕

- 문서: **[[06_io_flow_analysis]]** (663줄 / 41 KB)
- 선수지식: 2~4단계
- 무엇을 배우나: 지금까지 따로 배운 코어·캐시·NoC·DRAM을 **하나의 메모리 요청 타임라인**으로
  연결한다. 이 문서를 읽으면 흩어진 지식이 한 그림으로 합쳐진다.
- 핵심 개념:
  - Load 경로: SM → L1 → (miss) → ICNT → L2 → (miss) → DRAM → 역방향 응답
  - Store / Atomic 경로의 차이
  - 클록 도메인별 사이클 진행 순서, 단일 Load(L1 miss/L2 miss)의 타임라인
  - `mem_fetch_status` 상태 전이 다이어그램
  - 설정 파라미터(캐시/DRAM/ICNT)가 성능에 미치는 영향
- 대응 소스코드: 위 모든 모듈을 가로지름 (특히 `gpu-sim.cc::cycle()` 순서)

---

### 6단계 — 실습: 빌드·실행·설정

- 문서: **[[07_benchmark_scenarios]]** (676줄 / 28 KB)
- 선수지식: 1·5단계 (구조와 흐름을 알아야 config가 보인다)
- 무엇을 배우나: 직접 시뮬레이터를 돌리는 방법과 `gpgpusim.config`를 읽는 법.
- 핵심 개념:
  - 빌드 절차, 동적 링크(`-lcudart`)로 CUDA 앱 가로채기
  - PTX 임베딩(`-save_embedded_ptx`) 워크플로
  - Accel-Sim을 통한 SASS 트레이스 기반 실행
  - **`gpgpusim.config` 라인별 해석** (SM7_QV100 기준): 클록 도메인, 파이프라인 폭,
    명령어 레이턴시, 캐시/DRAM 설정
- 대응 소스코드/설정: `gpgpu-sim-study/configs/`, `src/option_parser.{cc,h}`

---

### 7단계(심화) — 전력 모델 AccelWattch

- 문서: **[[08_power_model_accelwattch]]** (956줄 / 40 KB)
- 선수지식: 2~5단계 (성능 카운터가 어디서 나오는지 알아야 함)
- 무엇을 배우나: 성능 시뮬레이션 위에 얹는 McPAT 기반 전력 추정.
- 핵심 개념:
  - 세 가지 전력 시뮬레이션 모드, 전력 구성요소(`pwr_cmp_t`)
  - 동적/정적/누설 전력 계산, DVFS
  - CACTI 통합, 성능 카운터 → 전력 매핑, XML 설정
- 대응 소스코드:
  - `src/gpgpu-sim/power_interface.{cc,h}`, `src/gpgpu-sim/power_stat.{cc,h}`
  - `src/accelwattch/` (McPAT 내부는 작업 제외 대상)
- 자매 노트: `notes/gpu-storage/gpgpu-sim-cuda-power-guide.md` (Part 3 후반 — 전력)

---

## 3. 목표별 빠른 내비게이션

특정 주제만 빠르게 찾고 싶을 때:

| 알고 싶은 것 | 보는 곳 |
|--------------|---------|
| GPU가 왜 이렇게 생겼나 (CPU와 차이) | 00 §1 |
| Warp / SIMT 실행 단위 | 00 §5, 02 §1, 05 §6 |
| **Warp Divergence & 재수렴** | 00 §6(이론) → 02 §4(타이밍 구현) → 05 §6(기능 구현) |
| 워프 스케줄러 비교 (GTO/LRR/…) | 02 §3 |
| 데이터 해저드 / 스코어보드 | 02 §5 |
| 캐시 동작 / MSHR | 03 §2, §3 |
| DRAM 타이밍 / FR-FCFS | 03 §5 |
| 주소 → 채널/뱅크 매핑 | 03 §6 |
| NoC / 가상 채널 / flit | 04 §5~§7 |
| **메모리 요청 end-to-end 경로** | 06 §1~§2 |
| 시뮬레이터 한 사이클의 진행 순서 | 01 §4, 06 §3 |
| 빌드 & 실행 & config | 07 전체 |
| 전력 추정 | 08 전체 |
| CUDA API가 어떻게 가로채지나 | 01 §6 |

---

## 4. 문서 ↔ 소스코드 대응 지도

```
gpgpu-sim-study/src/
├── abstract_hardware_model.{cc,h} ── 02, 05  (warp_inst_t, SIMT stack, core_t)
├── gpgpusim_entrypoint.cc ────────── 01      (시뮬레이터 진입)
├── stream_manager.{cc,h} ─────────── 01      (커널 디스패치)
├── option_parser.{cc,h} ──────────── 07      (config 파싱)
├── libcuda/ ──────────────────────── 01 §6   (CUDA API 인터셉션)
├── cuda-sim/ ─────────────────────── 05      (기능 시뮬레이션: PTX 실행)
│   ├── cuda-sim.cc, instructions.cc          (명령어 시맨틱)
│   ├── ptx_ir.{cc,h}, ptx_loader.cc          (PTX 파싱/IR)
│   └── memory.{cc,h}                         (메모리 공간)
├── gpgpu-sim/ ────────────────────── 01~04, 06, 08  (타이밍 시뮬레이션)
│   ├── gpu-sim.{cc,h} ───────────── 01, 06   (cycle() 메인 루프)
│   ├── shader.{cc,h} ─────────────── 02      (SM 파이프라인) ★주석본 있음
│   ├── scoreboard.{cc,h} ─────────── 02      (해저드)
│   ├── gpu-cache.{cc,h} ──────────── 03      (L1/L2 캐시)
│   ├── l2cache.{cc,h} ────────────── 03      (메모리 파티션)
│   ├── dram.{cc,h}, dram_sched.* ── 03      (DRAM/FR-FCFS)
│   ├── addrdec.{cc,h} ────────────── 03      (주소 디코딩)
│   ├── mem_fetch.{cc,h} ──────────── 03, 06  (메모리 요청 패킷)
│   ├── icnt_wrapper.*, local_interconnect.* ─ 04  (NoC 래퍼)
│   └── power_interface.*, power_stat.* ────── 08  (전력)
├── intersim2/ ────────────────────── 04      (BookSim2 NoC)
└── accelwattch/ ──────────────────── 08      (McPAT 전력, 연동부만)
```

★ 주석본: `shader.{cc,h}`는 이 폴더에 한국어 라인 주석본
(`shader_cc_annotated.cc`, `shader_h_annotated.h`)이 있으니 02단계에서 원본과 대조하며 읽으면 좋다.

---

## 5. 핵심 용어 빠른 사전

처음 보는 약어가 나오면 여기를 먼저 확인:

| 용어 | 의미 |
|------|------|
| **SM** (Streaming Multiprocessor) | GPU 코어 유닛. 여러 warp를 동시 실행. 코드에선 "shader core" |
| **warp** | 32스레드의 SIMT 실행 단위 |
| **SIMT** | Single Instruction, Multiple Threads — 한 명령어를 32스레드가 함께 실행 |
| **SIMT stack** | warp 내 분기 처리를 위한 (active mask + PC) 스택 |
| **divergence / reconvergence** | warp 내 분기 갈림 / post-dominator에서 재합류 |
| **scoreboard** | 레지스터 의존성(RAW 해저드) 추적 |
| **operand collector** | 레지스터 파일 뱅크 충돌을 처리하는 교환기 |
| **mem_fetch** | 시뮬레이터 내부 메모리 요청 패킷 |
| **MSHR** | Miss Status Holding Register — 같은 캐시 라인 미스 병합 |
| **CTA** | Cooperative Thread Array = 스레드 블록 |
| **PTX / SASS** | 가상 ISA / 실제 ISA. PTXPlus는 SASS 근사 |
| **ICNT / NoC** | 인터커넥트 / Network-on-Chip (BookSim 기반) |
| **VC** (Virtual Channel) | 한 물리 링크를 논리적으로 나눈 채널 (교착 회피/QoS) |
| **flit** | NoC 전송의 최소 단위 (flow control digit) |
| **FR-FCFS** | First-Ready First-Come-First-Served — DRAM 스케줄링 정책 |
| **occupancy** | SM 자원 대비 실제 상주 warp 비율 |
| **execution-driven / trace-driven** | 실제 바이너리 실행 / 사전 캡처 SASS 트레이스 입력 |

더 자세한 용어/도메인 지식은 `gpgpu-sim-study/CLAUDE.md`의 "주요 용어 사전" 참고.

---

## 6. 자매 자료 (notes/)

`deep/notes/gpu-storage/` 에는 본 분석 문서와 짝을 이루는 가이드 시리즈가 있다. 분석 문서가
코드 라인 단위라면, notes는 서술형 가이드/교과서 정리에 가깝다.

| 노트 | 짝이 되는 단계 |
|------|----------------|
| `gpgpu-sim-architecture-guide.md` (Part 1) | 1단계(개요) |
| `gpgpu-sim-memory-guide.md` (Part 2) | 3단계(메모리) |
| `gpgpu-sim-cuda-power-guide.md` (Part 3) | 2A단계(CUDA/PTX) + 7단계(전력) |
| `GPGPU_Architecture_Korean.md` | 0단계 (Aamodt 교과서 정리) |
| `gpu-architecture-scaling-book.md` | 0단계 (현대 GPU 스케일링 관점) |

---

## 7. 추천 학습 일정 예시 (2주 플랜)

| 일차 | 학습 내용 |
|------|-----------|
| 1~2일 | 00 §1~§6 (설계 철학 ~ divergence) + `GPGPU_Architecture_Korean.md` |
| 3일 | 01 (시뮬레이터 구조 + 메인 루프) + architecture-guide(Part1) |
| 4일 | 00 §7~ (메모리 계층 이론) + 05 전반부 (PTX 실행) |
| 5~6일 | 02 (SM 파이프라인) — `shader_*_annotated`와 대조하며 정독 |
| 7일 | 02 §3~§5 복습 (스케줄러/스코어보드/SIMT 스택) |
| 8~9일 | 03 (메모리 계층) + memory-guide(Part2) |
| 10일 | 04 (NoC) |
| 11일 | 06 (I/O 흐름 통합) — 앞 내용 결합 |
| 12일 | 07 (빌드·실행, 직접 시뮬레이션 돌려보기) |
| 13일 | 08 (전력) |
| 14일 | 전체 복습 + §3 목표별 내비게이션으로 약한 부분 보강 |

---

## 부록: 이 README의 위치와 갱신 규칙

- 위치: `deep/analysis/gpgpu-sim/README.md`
- 새 분석 문서를 이 폴더에 추가하면 §1 목록, §2 학습 경로, §4 소스 대응 지도에 반영한다.
- 코드 위치 표기는 `gpgpu-sim-study/<경로>` 형식으로 트리 밖에서도 추적 가능하게 유지한다.
- 관련 문서는 옵시디언 위키링크(`[[파일명]]`)로 상호 연결한다.
