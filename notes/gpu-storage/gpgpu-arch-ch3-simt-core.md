# Chapter 3: SIMT 코어: 명령어 및 레지스터 데이터 흐름

> 출처: "General-Purpose Graphics Processor Architectures" by Aamodt, Fung, Rogers (2018)

이 장과 다음 장에서 우리는 현대 GPU의 아키텍처와 마이크로아키텍처를 살펴볼 것이다. GPU 아키텍처에 대한 논의를 두 부분으로 나눈다: (1) 이 장에서 연산을 구현하는 SIMT 코어를 살펴보고, (2) 다음 장에서 메모리 시스템을 살펴본다.

전통적인 그래픽 렌더링 역할에서 GPU는 상세한 텍스처 맵(texture map)과 같이 온칩(on-chip)에 완전히 캐시하기에는 너무 큰 데이터 세트에 접근한다. 고성능 프로그래밍 가능성을 실현하려면 대규모 오프칩(off-chip) 대역폭을 유지할 수 있는 아키텍처를 채택해야 한다. 따라서 오늘날의 GPU는 수만 개의 스레드를 동시에 실행한다. 스레드당 온칩 메모리 저장 공간은 작지만, 캐시는 여전히 상당수의 오프칩 메모리 접근을 줄이는 데 효과적일 수 있다.

Figure 3.1은 이 장에서 논의하는 GPU 파이프라인의 마이크로아키텍처를 보여준다. 파이프라인은 SIMT 프론트엔드(front-end)와 SIMD 백엔드(back-end)로 나눌 수 있다. 파이프라인은 단일 파이프라인에서 함께 작동하는 세 개의 스케줄링 "루프"로 구성된다: 명령어 페치 루프(instruction fetch loop), 명령어 이슈 루프(instruction issue loop), 그리고 레지스터 접근 스케줄링 루프(register access scheduling loop)이다.

## 3.1 단일 루프 근사(One-Loop Approximation)

단일 스케줄러를 가진 GPU를 고려하는 것부터 시작한다. 효율성을 높이기 위해 스레드는 NVIDIA에서 "워프(warp)", AMD에서 "웨이브프론트(wavefront)"라 불리는 그룹으로 조직된다. 따라서 스케줄링의 단위는 워프이다. 매 사이클마다 하드웨어는 스케줄링할 워프를 선택한다.

### 3.1.1 SIMT 실행 마스킹(Execution Masking)

현대 GPU의 핵심 특징은 SIMT 실행 모델로, 기능적 관점에서 프로그래머에게 개별 스레드가 완전히 독립적으로 실행된다는 추상화를 제공한다. 이 프로그래밍 모델은 프레디케이션(predication)만으로도 달성될 수 있다. 그러나 현재 GPU에서는 전통적인 프레디케이션과 프레디케이트 마스크 스택(stack of predicate masks)의 조합으로 구현되며, 이를 SIMT 스택(SIMT stack)이라 부른다.

SIMT 스택은 두 가지 핵심 문제를 효율적으로 처리하는 데 도움을 준다: 중첩 제어 흐름(nested control flow, 한 분기가 다른 분기에 제어 종속되는 경우)과 워프 내 모든 스레드가 특정 제어 흐름 경로를 피할 때 연산을 완전히 건너뛰는 것이다.

SIMT 스택을 설명하기 위해 예제를 사용한다. Figure 3.2는 do-while 루프 안에 두 개의 분기가 중첩된 CUDA C 코드를 보여주고, Figure 3.3은 대응하는 PTX 어셈블리를 보여준다. Figure 3.4는 워프당 네 개의 스레드를 가진 GPU를 가정할 때 이 코드가 SIMT 스택과 어떻게 상호작용하는지를 보여준다.

현재 GPU에서 사용되는 접근 방식은 주어진 워프 내에서 서로 다른 경로를 따르는 스레드의 실행을 직렬화(serialize)하는 것이다. SIMT 스택의 각 항목은 세 가지를 포함한다: 재수렴 프로그램 카운터(reconvergence program counter, RPC), 다음에 실행할 명령어의 주소(Next PC), 그리고 활성 마스크(active mask)이다.

재수렴 지점(reconvergence point)은 프로그램에서 분기한 스레드들이 다시 동기적으로(lock-step) 실행을 계속하도록 강제할 수 있는 위치이다. 일반적으로 가장 가까운 재수렴 지점이 선호된다. 주어진 프로그램 실행에서 컴파일 타임에 분기한 스레드들이 다시 동기적으로 실행할 수 있음을 보장할 수 있는 가장 이른 지점은 분기 발산을 유발한 분기문의 즉시 후위 지배자(immediate post-dominator)이다.

### 3.1.2 SIMT 교착 상태와 스택리스 SIMT 아키텍처

최근 NVIDIA는 Volta GPU 아키텍처의 세부 사항을 공개했다. 그들이 강조한 한 가지 변경 사항은 발산(divergence) 시 마스킹 동작과 이것이 동기화와 어떻게 상호작용하는지에 관한 것이다. 스택 기반의 SIMT 구현은 "SIMT 교착 상태(SIMT deadlock)"라 불리는 교착 상태 조건을 초래할 수 있다. 학술 연구에서는 SIMT 교착 상태를 방지할 수 있는 대안적 하드웨어를 기술했다. NVIDIA는 이 새로운 스레드 발산 관리 방식을 독립 스레드 스케줄링(Independent Thread Scheduling)이라 부른다.

핵심 아이디어는 스택을 워프별 수렴 배리어(per warp convergence barrier)로 대체하는 것이다. 수렴 배리어 메커니즘은 스케줄러가 발산된 스레드 그룹 간에 자유롭게 전환할 수 있는 대안적 구현을 제공한다. 이를 통해 일부 스레드가 락(lock)을 획득하고 다른 스레드는 획득하지 못한 상태에서도 워프 내 스레드 간 순방향 진행(forward progress)이 가능해진다.

### 3.1.3 워프 스케줄링(Warp Scheduling)

GPU의 각 코어는 많은 워프를 포함한다. 메모리 시스템이 "이상적"이어서 메모리 요청에 고정 지연 시간 내에 응답한다면, 세립도 멀티스레딩(fine-grained multithreading)을 사용하여 이 지연 시간을 숨길 수 있을 만큼 충분한 워프를 지원하도록 코어를 설계할 수 있을 것이다. 이 경우 워프는 "라운드 로빈(round robin)" 순서로 스케줄링될 수 있다.

그러나 중요한 트레이드오프가 있다: 매 사이클마다 다른 워프가 명령어를 발행할 수 있게 하려면 각 스레드가 자신만의 레지스터를 가져야 한다. 따라서 코어당 워프 수를 늘리면 실행 유닛에 할당되는 비율 대비 레지스터 파일 저장에 할당되는 칩 면적 비율이 증가한다.

## 3.2 이중 루프 근사(Two-Loop Approximation)

각 코어가 지원해야 하는 워프 수를 줄이는 데 도움을 주기 위해, 이전 명령어가 아직 완료되지 않은 상태에서도 워프로부터 후속 명령어를 발행할 수 있는 것이 유용하다. 의존성 정보를 제공하려면 먼저 메모리에서 명령어를 페치하여 어떤 데이터 및/또는 구조적 해저드(structural hazard)가 존재하는지 확인해야 한다. 이를 위해 GPU는 캐시 접근 후 명령어가 배치되는 명령어 버퍼(instruction buffer)를 구현한다. 명령어 버퍼에서 다음에 발행할 명령어를 결정하기 위해 별도의 스케줄러가 사용된다.

GPU는 순차 스코어보드(in-order scoreboard)를 구현한다. Coon 등이 제안한 설계는 워프당 소수의 항목을 포함하며, 각 항목은 발행되었지만 아직 실행이 완료되지 않은 명령어가 기록할 레지스터의 식별자이다.

## 3.3 삼중 루프 근사(Three-Loop Approximation)

앞서 기술한 바와 같이, 긴 메모리 지연 시간을 숨기려면 코어당 많은 워프를 지원해야 한다. 예를 들어 NVIDIA의 최근 GPU 아키텍처에서 레지스터 파일은 256 KB를 포함한다. SRAM 메모리의 면적은 포트 수에 비례한다. 면적을 줄이는 한 가지 방법은 단일 포트 메모리의 다중 뱅크를 사용하여 많은 수의 포트를 시뮬레이션하는 것이다. 이를 투명하게 달성하기 위해 오퍼랜드 컬렉터(operand collector)라 알려진 구조가 사용된다.

### 3.3.1 오퍼랜드 컬렉터(Operand Collector)

오퍼랜드 컬렉터 마이크로아키텍처는 스테이징 레지스터(staging register)를 컬렉터 유닛(collector unit)으로 대체한다. 각 명령어는 레지스터 읽기 단계에 진입할 때 컬렉터 유닛을 할당받는다. 여러 개의 컬렉터 유닛이 있어서 여러 명령어가 소스 오퍼랜드 읽기를 중첩할 수 있으며, 이는 뱅크 충돌(bank conflict) 상황에서 처리량을 향상시키는 데 도움이 된다.

오퍼랜드 컬렉터는 뱅크 충돌이 발생할 때 이를 허용하기 위해 스케줄링을 사용한다. Figure 3.16은 서로 다른 워프의 동일 레지스터를 다른 뱅크에 할당하여 뱅크 충돌을 줄이는 데 도움이 되는 수정된 레지스터 배치(swizzled)를 보여준다.

### 3.3.2 명령어 재실행: 구조적 해저드 처리(Instruction Replay: Handling Structural Hazards)

GPU 파이프라인에는 구조적 해저드의 잠재적 원인이 많다. 명령어가 구조적 해저드를 만나면 어떻게 되는가? GPU는 명령어 재실행(instruction replay)의 한 형태를 구현한다. 명령어 재실행은 파이프라인의 정체(clogging)와 스톨링(stalling)으로 인한 회로 면적 및/또는 타이밍 오버헤드를 방지하기 위해 GPU에서 사용된다.

## 3.4 분기 발산에 대한 연구 방향(Research Directions on Branch Divergence)

### 3.4.1 워프 압축(Warp Compaction)

동적 워프 형성(Dynamic Warp Formation, DWF)은 여러 발산된 정적 워프에 흩어진 스레드들을 같은 명령어를 실행하는 새로운 동적 워프로 재배열할 수 있다는 관찰을 활용한다. 스레드 블록 압축(Thread Block Compaction, TBC)은 이를 기반으로 압축이 스레드 블록 내에서만 발생하도록 제한한다. 대규모 워프 마이크로아키텍처(Large Warp Microarchitecture, LWM)는 워프 그룹의 재수렴을 관리하기 위해 SIMT 스택을 확장한다.

### 3.4.2 워프 내 발산 경로 관리(Intra-Warp Divergent Path Management)

다중 경로 병렬성(Multi-Path Parallelism): 워프가 분기에서 발산하면 스레드들은 워프 분할(warp-split)이라 불리는 여러 그룹으로 나뉜다. 동적 워프 세분화(Dynamic Warp Subdivision, DWS)는 발산된 워프를 동시 워프 분할로 세분화하기 위해 SIMT 스택을 워프 분할 테이블로 확장한다. 다중 경로 실행 모델(Multi-Path Execution Model, MPM)은 SIMT 스택을 워프 분할 테이블과 재수렴 테이블 두 개의 테이블로 대체한다.

더 나은 수렴(Better Convergence): 후위 지배자 스택 기반 재수렴 메커니즘은 통합 알고리즘을 사용하여 식별된 재수렴 지점을 사용한다. 유사 수렴 지점(Likely-Convergence Points)은 SIMT 스택을 확장한다. 스레드 프론티어(Thread Frontiers)는 SIMT 스택에서 완전히 벗어난다.

### 3.4.3 MIMD 기능 추가(Adding MIMD Capability)

벡터-스레드(Vector-Thread, VT) 아키텍처는 SIMD와 MIMD 아키텍처의 측면을 결합한다. 시간적 SIMT(Temporal SIMT)는 각 레인이 MIMD 방식으로 실행할 수 있게 한다. 가변 워프 크기(Variable Warp Size, VWS) 아키텍처는 각각 페치 및 디코드 유닛을 가진 다수의 슬라이스를 포함한다.

### 3.4.4 복잡도 효율적 발산 관리(Complexity-Effective Divergence Management)

AMD GCN은 소프트웨어에서 SIMT 스택을 에뮬레이션할 수 있는 스칼라 레지스터 파일(scalar register file)을 제공한다. 스레드 프론티어(Thread Frontiers)는 SIMT 스택을 스레드별 PC로 대체한다. 스택리스 SIMT(Stackless SIMT)는 시간적 SIMT를 syncwarp 명령어로 확장한다. 프레디케이션(Predication)은 간단한 if 분기를 저오버헤드로 처리하는 방법으로 현대 GPU에 남아 있다.

## 3.5 스칼라화 및 아핀 실행에 대한 연구 방향(Research Directions on Scalarization and Affine Execution)

### 3.5.1 균일 또는 아핀 변수의 탐지(Detection of Uniform or Affine Variables)

컴파일러 기반 탐지(Compiler-Driven Detection): AMD GCN은 컴파일러에 의존한다. Asanovic 등은 수렴 분석과 변이 분석을 결합한 방법을 소개한다.

하드웨어 탐지(Hardware Detection): Collange 등의 태그 기반 탐지(Tag-Based Detection)는 각 GPU 레지스터에 태그를 확장한다. FG-SIMT 아키텍처는 분기에 대한 더 나은 지원으로 이를 확장한다. Gilani 등의 쓰기 시점 비교(Comparison at Write-Back)는 각 쓰기 시점에서 모든 스레드의 레지스터 값을 비교한다.

### 3.5.2 GPU에서 균일 또는 아핀 변수의 활용(Exploiting Uniform or Affine Variables in GPU)

압축 레지스터 저장(Compressed Register Storage), 전용 스칼라 파이프라인(Dedicated Scalar Pipeline), 클록 게이트 SIMD 데이터패스(Clock-Gated SIMD Datapath), 아핀 워프로의 집합(Aggregate to Affine Warp), 메모리 접근 가속(Memory Access Acceleration).

## 3.6 레지스터 파일 아키텍처에 대한 연구 방향(Research Directions on Register File Architecture)

현대 GPU는 많은 수의 하드웨어 스레드(워프)를 사용하며, 모든 하드웨어 스레드의 레지스터를 온칩 레지스터 파일에 저장한다. 예를 들어 NVIDIA의 Fermi GPU는 20,000개 이상의 인플라이트(in-flight) 스레드를 유지할 수 있으며, 총 레지스터 용량은 2 MB이다.

### 3.6.1 계층적 레지스터 파일(Hierarchical Register File)

Gebhart 등은 주 레지스터 파일에 레지스터 파일 캐시(Register File Cache, RFC)를 추가하여 확장할 것을 제안하며, 이 계층 구조는 주 레지스터 파일에 대한 접근 빈도를 극적으로 줄인다.

### 3.6.2 졸린 상태 레지스터 파일(Drowsy State Register File)

Abdel-Majeed와 Annavaram은 누설 전력(leakage power)을 줄이는 3모드 레지스터 파일 설계(ON, OFF, Drowsy)를 제안한다.

### 3.6.3 레지스터 파일 가상화(Register File Virtualization)

Tarjan과 Skadron은 레지스터 리네이밍(register renaming)을 사용하여 물리 레지스터 파일 크기를 최대 50%까지 줄일 것을 제안한다. Jeon 등은 그 영향을 정량화하고 메타데이터 명령어를 제안한다.

### 3.6.4 분할 레지스터 파일(Partitioned Register File)

Abdel-Majeed 등은 파일럿 레지스터 파일(Pilot Register File)을 도입하여, 빠른(일반 SRAM) 레지스터 파일과 느린(근임계 전압 SRAM, near-threshold voltage SRAM) 레지스터 파일로 분할한다.

### 3.6.5 RegLess

Kloosterman 등은 레지스터 파일을 제거하고 오퍼랜드 스테이징 버퍼(operand staging buffer)로 대체하는 것을 목표로 한다. 그들의 평가에 따르면 512 항목의 OSU가 2048 KB 레지스터 파일 대비 약간 더 나은 성능을 달성하면서 공간의 25%만 차지하고 전체 GPU 에너지 소비를 11% 줄인다.
