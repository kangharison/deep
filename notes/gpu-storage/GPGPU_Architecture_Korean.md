# 범용 그래픽 프로세서 아키텍처 (General-Purpose Graphics Processor Architectures)

> 원서: "General-Purpose Graphics Processor Architectures"
> 저자: Tor M. Aamodt, Wilson Wai Lun Fung, Timothy G. Rogers (2018)
> Morgan & Claypool Publishers, Synthesis Lectures on Computer Architecture #44
> DOI: 10.2200/S00848ED1V01Y201804CAC044

---

## 초록 (Abstract)

원래 비디오 게임을 지원하기 위해 개발된 그래픽 처리 장치(GPU)는 이제 머신 러닝에서 암호화폐 채굴에 이르기까지 범용(비그래픽) 애플리케이션에 점점 더 많이 사용되고 있다. GPU는 하드웨어 자원의 더 큰 비율을 연산에 할당함으로써 중앙 처리 장치(CPU) 대비 향상된 성능과 효율을 달성할 수 있다. 또한 범용 프로그래밍 가능성은 도메인 특화 가속기와 비교하여 현대 GPU를 소프트웨어 개발자에게 매력적으로 만든다. 이 책은 범용 컴퓨팅을 지원하는 GPU의 아키텍처를 연구하는 데 관심 있는 사람들을 위한 입문서를 제공한다.

이 책의 1장은 GPU의 기본 하드웨어 구조를 설명하고 역사에 대한 간략한 개요를 제공한다. 2장은 이 책의 나머지 부분과 관련된 GPU 프로그래밍 모델에 대한 요약을 제공한다. 3장은 GPU 컴퓨트 코어의 아키텍처를 탐구한다. 4장은 GPU 메모리 시스템의 아키텍처를 탐구한다. 기존 시스템의 아키텍처를 설명한 후, 3장과 4장은 관련 연구의 개요를 제공한다. 5장은 컴퓨트 코어와 메모리 시스템 모두에 영향을 미치는 횡단적 연구를 요약한다.

**키워드:** GPGPU, 컴퓨터 아키텍처

---

# 서문 (Preface)

이 책은 그래픽 처리 장치(GPU)의 아키텍처를 이해하고, GPU 설계 개선 방안을 탐구하는 연구 분야에 대한 입문서를 제공하고자 하는 독자를 위해 쓰였다. 독자가 파이프라이닝(pipelining)과 캐시(cache) 같은 컴퓨터 아키텍처 개념에 익숙하며, GPU 아키텍처와 관련된 연구 및/또는 개발에 관심이 있다고 가정한다. 이러한 작업은 서로 다른 설계 간의 트레이드오프(trade-off)에 초점을 맞추는 경향이 있으므로, 이 책은 독자가 경험 많은 설계자들이 이미 알고 있는 것을 시행착오를 통해 배워야 하는 상황을 피할 수 있도록 그러한 트레이드오프에 대한 통찰을 제공하는 관점에서 서술되었다.

이를 달성하기 위해, 이 책은 특허, 제품 문서, 연구 논문 등 다양하고 이질적인 출처에 흩어져 있는 관련 정보를 하나의 자료로 모았다. 이것이 연구를 막 시작하는 학생이나 실무자가 생산성을 발휘하기까지 걸리는 시간을 줄이는 데 도움이 되기를 바란다.

이 책은 현재 GPU 설계의 여러 측면을 다루면서도, 출판된 연구를 "종합(synthesize)"하려는 시도도 한다. 이는 부분적으로 필요에 의한 것인데, 벤더들이 특정 GPU 제품의 마이크로아키텍처에 대해 공개한 정보가 매우 적기 때문이다. "기준(baseline)" GPGPU 아키텍처를 기술함에 있어, 이 책은 출판된 제품 설명(학술 논문, 백서, 매뉴얼)과 경우에 따라서는 특허의 기술 내용에 의존한다. 특허에서 발견되는 세부 사항은 실제 제품의 마이크로아키텍처와 상당히 다를 수 있다. 일부 경우에는 마이크로벤치마크(microbenchmark) 연구가 연구자들에게 일부 세부 사항을 명확히 해주었지만, 다른 경우에는 우리의 기준이 공개적으로 이용 가능한 정보에 기반한 "최선의 추측"을 나타낸다. 그럼에도 불구하고, 우리의 초점이 이미 연구되었거나 향후 연구에서 탐구할 만한 아키텍처 트레이드오프를 이해하는 데 있으므로, 이것이 도움이 될 것이라 믿는다.

이 책의 여러 부분은 GPU 아키텍처 개선이라는 주제에 관한 많은 최근 연구 논문을 요약하는 데 집중한다. 이 주제는 최근 몇 년간 인기가 크게 높아졌기 때문에, 이 책에서 모든 것을 다루기에는 너무 방대하다. 따라서 무엇을 다루고 무엇을 제외할지 어려운 선택을 해야 했다.

Tor M. Aamodt, Wilson Wai Lun Fung, Timothy G. Rogers
2018년 4월

---

# 감사의 글 (Acknowledgments)

이 책을 집필하는 동안 지원해 준 가족들에게 감사드린다. 또한 출판사의 Michael Morgan과 편집자 Margaret Martonosi에게 이 책이 완성되는 동안 보여준 극진한 인내심에 감사한다. 또한 이 책의 초기 원고에 상세한 피드백을 제공해 준 Carole-Jean Wu, Andreas Moshovos, Yash Ukidave, Aamir Raihan, Amruth Sandhupatla에게 감사한다. 마지막으로 Synthesis Lectures 집필 전략에 대한 생각과 이 책에 대한 구체적인 제안을 공유해 준 Mark Hill에게 감사한다.

Tor M. Aamodt, Wilson Wai Lun Fung, Timothy G. Rogers
2018년 4월

---

# 1장: 서론 (Introduction)

이 책은 그래픽 처리 장치(GPU)의 하드웨어 설계를 탐구한다. GPU는 처음에 비디오 게임에 초점을 맞춘 실시간 렌더링을 가능하게 하기 위해 도입되었다. 오늘날 GPU는 스마트폰, 노트북, 데이터센터, 그리고 슈퍼컴퓨터에 이르기까지 어디에서나 발견된다. 실제로 Apple A8 애플리케이션 프로세서의 분석에 따르면, 중앙 처리 장치(CPU) 코어보다 통합 GPU에 더 많은 다이 면적을 할당하고 있다. 점점 더 사실적인 그래픽 렌더링에 대한 수요가 GPU 혁신의 최초 동력이었다. 그래픽 가속이 여전히 GPU의 주요 목적이지만, GPU는 점점 더 비그래픽(non-graphics) 컴퓨팅을 지원하고 있다. 오늘날 주목받는 대표적인 예는 머신 러닝(machine learning) 시스템을 개발하고 배포하는 데 GPU의 사용이 증가하고 있다는 것이다. 따라서 이 책의 강조점은 비그래픽 애플리케이션의 성능과 에너지 효율을 개선하는 데 관련된 기능에 있다.

이 서론 장은 GPU에 대한 간략한 개요를 제공한다. 1.1절에서는 GPU가 다른 선택지와 어떻게 비교되는지 이해하기 위해 더 넓은 범주의 연산 가속기(computation accelerator)에 대한 동기를 살펴보는 것으로 시작한다. 그런 다음 1.2절에서는 현대 GPU 하드웨어에 대한 빠른 개요를 제공한다. 마지막으로 1.4절에서는 이 책의 나머지 부분에 대한 로드맵을 제공한다.

## 1.1 연산 가속기의 지형 (The Landscape of Computation Accelerators)

수십 년간 세대를 거듭하는 컴퓨팅 시스템은 달러당 성능이 기하급수적으로 증가하는 모습을 보여왔다. 근본적인 원인은 트랜지스터 크기 축소, 하드웨어 아키텍처 개선, 컴파일러 기술 개선, 그리고 알고리즘의 조합이었다. 일부 추정에 따르면 이러한 성능 향상의 절반은 더 빠르게 동작하는 소자를 만드는 트랜지스터 크기 축소 덕분이었다. 그러나 2005년경부터 트랜지스터의 스케일링은 현재 데나드 스케일링(Dennard Scaling)으로 알려진 고전적 규칙을 따르지 못하게 되었다. 핵심적인 결과 중 하나는 소자가 작아져도 클록 주파수가 훨씬 느리게 개선된다는 것이다. 성능을 향상시키려면 더 효율적인 하드웨어 아키텍처를 찾아야 한다.

하드웨어 특수화(hardware specialization)를 활용하면 에너지 효율을 최대 500배까지 개선할 수 있다. Hameed 등이 보여주었듯이, 이러한 효율 향상을 달성하는 데에는 몇 가지 핵심 측면이 있다. GPU에서 볼 수 있는 것과 같은 벡터 하드웨어(vector hardware)로 전환하면 명령어 처리의 오버헤드를 제거하여 약 10배의 효율 향상을 얻는다. 하드웨어 특수화의 나머지 효율 향상의 상당 부분은 데이터 이동을 최소화한 결과인데, 이는 레지스터 파일(register file)과 같은 큰 메모리 배열에 대한 접근을 피하면서 여러 산술 연산을 수행하는 복합 연산을 도입함으로써 달성할 수 있다.

오늘날 컴퓨터 아키텍트가 직면한 핵심 과제는 특수화된 하드웨어를 사용하여 얻을 수 있는 효율 향상과 광범위한 프로그램을 지원하는 데 필요한 유연성 사이의 균형을 더 잘 맞추는 방법을 찾는 것이다. 아키텍처만으로는 다수의 애플리케이션에 사용할 수 있는 알고리즘만 효율적으로 실행될 것이다. 떠오르는 예는 Google의 텐서 처리 장치(Tensor Processing Unit)와 같은 심층 신경망(deep neural network) 지원에 특수화된 하드웨어이다. 머신 러닝이 컴퓨팅 하드웨어 자원의 매우 큰 비중을 차지할 것으로 보이며, 이들은 특수화된 하드웨어로 이전될 수 있지만, 전통적인 프로그래밍 언어로 작성된 소프트웨어로 표현되는 연산을 효율적으로 지원할 필요성은 여전히 남아 있을 것이라고 우리는 주장한다.

머신 러닝을 위한 GPU 사용 외에 GPU 컴퓨팅에 대한 강한 관심의 한 가지 이유는 현대 GPU가 튜링 완전(Turing Complete) 프로그래밍 모델을 지원한다는 것이다. 튜링 완전이란 충분한 시간과 메모리가 주어지면 어떤 계산이든 실행할 수 있다는 것을 의미한다. 특수 목적 가속기에 비해 현대 GPU는 유연하다. GPU 하드웨어를 충분히 활용할 수 있는 소프트웨어의 경우, GPU는 CPU보다 한 자릿수(order of magnitude) 더 효율적일 수 있다. 이러한 유연성과 효율의 조합은 매우 바람직하다. 그 결과로 최고 성능과 에너지 효율 측면에서 많은 최상위 슈퍼컴퓨터가 현재 GPU를 사용하고 있다. 제품 세대를 거듭하면서 GPU 제조업체들은 유연성을 높이는 동시에 에너지 효율을 향상시키기 위해 GPU 아키텍처와 프로그래밍 모델을 개선해 왔다.

## 1.2 GPU 하드웨어 기초 (GPU Hardware Basics)

GPU를 처음 접하는 사람들은 종종 GPU가 언젠가 CPU를 완전히 대체할 수 있을지 묻는다. 이는 가능성이 낮아 보인다. 현재 시스템에서 GPU는 독립형 컴퓨팅 장치가 아니다. 오히려 GPU는 단일 칩에서 CPU와 결합되거나, CPU가 포함된 시스템에 GPU만 탑재된 애드인 카드(add-in card)를 삽입하여 사용된다. CPU는 GPU에서의 연산 시작과 GPU와의 데이터 전송을 담당한다. CPU와 GPU 사이의 이러한 역할 분담의 한 가지 이유는 연산의 시작과 끝이 일반적으로 입출력(I/O) 장치에 대한 접근을 필요로 하기 때문이다. GPU에서 직접 I/O 서비스를 제공하는 애플리케이션 프로그래밍 인터페이스(API)를 개발하려는 노력이 진행 중이지만, 지금까지 이들 모두 가까이에 CPU가 존재한다고 가정한다.

왜 CPU를 제거하지 않는가? I/O 장치에 접근하고 그 밖의 운영 체제 서비스를 제공하는 데 사용되는 소프트웨어에는 대규모 병렬성(massive parallelism)과 같은, GPU에서 실행하기에 적합하게 만드는 특성이 부족한 것으로 보인다. 따라서 CPU와 GPU의 상호작용을 고려하는 것에서 시작한다.

CPU와 GPU를 포함하는 전형적인 시스템의 추상 다이어그램은 Figure 1.1에 나와 있다. 왼쪽에는 NVIDIA의 Volta GPU와 같은 아키텍처를 위해 CPU와 GPU를 연결하는 버스(예: PCIe)를 포함하는 전형적인 디스크리트(discrete) GPU 구성이 있고, 오른쪽에는 AMD의 Bristol Ridge APU나 모바일 GPU와 같은 전형적인 통합(integrated) CPU-GPU의 논리 다이어그램이 있다. 디스크리트 GPU를 포함하는 시스템은 CPU용(흔히 시스템 메모리라 부름)과 GPU용(흔히 디바이스 메모리라 부름)으로 별도의 DRAM 메모리 공간을 갖는다는 점에 주목하라. 이러한 메모리에 사용되는 DRAM 기술은 종종 다르다(CPU용 DDR vs. GPU용 GDDR). CPU DRAM은 일반적으로 저지연(low latency) 접근에 최적화되어 있는 반면, GPU DRAM은 높은 처리량(high throughput)에 최적화되어 있다. 반면 통합 GPU를 가진 시스템은 단일 DRAM 메모리 공간을 가지므로 반드시 동일한 메모리 기술을 사용한다.

GPU 컴퓨팅 애플리케이션은 CPU에서 실행을 시작한다. 일반적으로 애플리케이션의 CPU 부분은 일부 데이터 구조를 할당하고 초기화한다. NVIDIA와 AMD의 구형 디스크리트 GPU에서는 GPU 컴퓨팅 애플리케이션의 CPU 부분이 일반적으로 CPU와 GPU 메모리 모두에 데이터 구조를 위한 공간을 할당한다. 이러한 GPU의 경우, 애플리케이션의 CPU 부분이 CPU 메모리에서 GPU 메모리로의 데이터 이동을 조율해야 한다. 더 최근의 디스크리트 GPU(예: NVIDIA의 Pascal 아키텍처)는 CPU 메모리에서 GPU 메모리로 데이터를 자동으로 전송하는 소프트웨어 및 하드웨어 지원을 갖추고 있다. 이는 CPU와 GPU 모두에서 가상 메모리(virtual memory) 지원을 활용하여 달성할 수 있다. NVIDIA는 이를 "통합 메모리(unified memory)"라고 부른다.

어느 시점에서 CPU는 GPU에서의 연산을 시작해야 한다. 현재 시스템에서 이는 CPU에서 실행되는 드라이버의 도움으로 수행된다. GPU에서 연산을 시작하기 전에, GPU 컴퓨팅 애플리케이션은 GPU에서 실행할 코드를 지정한다. 이 코드는 일반적으로 커널(kernel)이라고 불린다. 동시에 GPU 컴퓨팅 애플리케이션의 CPU 부분은 얼마나 많은 스레드가 실행되어야 하는지와 이 스레드들이 입력 데이터를 어디에서 찾아야 하는지도 지정한다. 실행할 커널, 스레드 수, 데이터 위치는 CPU에서 실행되는 드라이버를 통해 GPU 하드웨어에 전달된다.

현대 GPU는 Figure 1.2에 나와 있듯이 많은 코어로 구성된다. NVIDIA는 이러한 코어를 스트리밍 멀티프로세서(streaming multiprocessor)라고 부르고, AMD는 컴퓨트 유닛(compute unit)이라고 부른다. 각 GPU 코어는 GPU에서 실행되도록 론칭된 커널에 해당하는 단일 명령어 다중 스레드(single-instruction multiple-thread, SIMT) 프로그램을 실행한다. GPU의 각 코어는 일반적으로 수천 개 규모의 스레드를 실행할 수 있다. 단일 코어에서 실행되는 스레드들은 스크래치패드 메모리(scratchpad memory)를 통해 통신하고, 빠른 배리어(barrier) 연산을 사용하여 동기화할 수 있다. 각 코어는 또한 일반적으로 1차 명령어 및 데이터 캐시를 포함한다.

GPU는 고도로 병렬적인 워크로드에서 슈퍼스칼라 비순차(superscalar out-of-order) CPU에 비해 단위 면적당 향상된 성능을 얻을 수 있는데, 이는 다이 면적의 더 큰 비율을 산술 논리 장치(ALU)에 할당하고 그에 상응하여 더 적은 면적을 제어 논리에 할당함으로써 달성된다. CPU와 GPU 아키텍처 간의 트레이드오프에 대한 직관을 개발하기 위해, Guz 등은 스레드 수에 따라 성능이 어떻게 변하는지 보여주는 통찰력 있는 분석 모델을 개발했다. 적은 수의 스레드가 큰 캐시를 공유할 때(멀티코어 CPU의 경우처럼), 성능은 스레드 수에 따라 증가한다. 그러나 스레드 수가 캐시가 전체 워킹 셋(working set)을 담을 수 없는 지점까지 증가하면 성능은 감소한다. 스레드 수가 더 증가하면 멀티스레딩이 긴 오프칩 지연(off-chip latency)을 숨길 수 있는 능력에 따라 성능이 다시 증가한다. GPU 아키텍처는 이 그림의 오른쪽 부분에 해당한다. GPU는 멀티스레딩을 사용하여 빈번한 캐시 미스를 허용하도록 설계되었다.

Table 1.1: 45 nm 공정 기술에서 다양한 연산의 에너지 소비:

| 연산 | 에너지 [pJ] | 상대 비용 |
|-----------|-------------|---------------|
| 32비트 정수 ADD | 0.1 | 1 |
| 32비트 부동소수점 ADD | 0.9 | 9 |
| 32비트 정수 MULT | 3.1 | 31 |
| 32비트 부동소수점 MULT | 3.7 | 37 |
| 32비트 32KB SRAM | 5 | 50 |
| 32비트 DRAM | 640 | 6400 |

## 1.3 GPU의 간략한 역사 (A Brief History of GPUs)

이 절에서는 그래픽 처리 장치의 역사를 간략히 설명한다. 컴퓨터 그래픽은 1960년대에 Ivan Sutherland의 Sketchpad와 같은 프로젝트와 함께 등장했다. 초기부터 컴퓨터 그래픽은 영화 애니메이션을 위한 오프라인 렌더링과, 이와 병행하여 비디오 게임에서 사용되는 실시간 렌더링 개발에 필수적이었다. 초기 비디오 카드는 텍스트만 지원하던 1981년 IBM 모노크롬 디스플레이 어댑터(MDA)에서 시작되었다. 이후 비디오 카드는 2D 가속을 도입한 뒤 3D 가속을 도입했다.

NVIDIA GeForce 256과 같은 초기 3D 그래픽 프로세서는 비교적 고정 기능(fixed-function)이었다. NVIDIA는 2001년에 출시된 GeForce 3에서 버텍스 셰이더(vertex shader)와 픽셀 셰이더(pixel shader) 형태로 GPU에 프로그래밍 가능성(programmability)을 도입했다. 연구자들은 행렬 데이터를 텍스처(texture)에 매핑하고 셰이더를 적용하여 이러한 초기 GPU에서 선형 대수를 구현하는 방법을 빠르게 배웠고, 프로그래머가 그래픽을 알 필요 없이 범용 컴퓨팅을 GPU에 매핑하는 학술 연구가 곧 뒤따랐다.

이러한 노력은 GPU 제조업체들이 그래픽 외에 범용 컴퓨팅을 직접 지원하도록 영감을 주었다. 이를 수행한 최초의 상용 제품은 NVIDIA GeForce 8 시리즈였다. GeForce 8 시리즈는 셰이더에서 임의의 메모리 주소에 쓸 수 있는 기능과 오프칩 대역폭을 제한하기 위한 스크래치패드 메모리를 포함한 여러 혁신을 도입했다. 다음 혁신은 NVIDIA의 Fermi 아키텍처에서 읽기-쓰기 데이터의 캐싱을 가능하게 한 것이었다. 이후의 개선 사항에는 CPU와 GPU를 같은 다이에 통합한 AMD의 Fusion 아키텍처와 GPU 자체에서 스레드 론칭을 가능하게 하는 동적 병렬성(dynamic parallelism)이 포함된다. 가장 최근에는 NVIDIA의 Volta가 머신 러닝 가속을 특별히 목표로 하는 텐서 코어(Tensor Core)와 같은 기능을 도입했다.

## 1.4 책의 구성 (Book Outline)

이 책의 나머지 부분은 다음과 같이 구성된다.

하드웨어를 설계할 때는 그것이 지원할 소프트웨어를 고려하는 것이 중요하다. 따라서 2장에서는 프로그래밍 모델, 코드 개발 과정, 컴파일 흐름에 대한 간략한 요약을 제공한다.

3장에서는 수천 개의 스레드 실행을 지원하는 개별 GPU 코어의 아키텍처를 탐구한다. 높은 처리량과 유연한 프로그래밍 모델을 지원하는 데 관련된 트레이드오프에 대해 점진적으로 더 상세한 이해를 쌓아간다.

4장에서는 GPU 코어 내에 있는 1차 캐시와 메모리 파티션의 내부 구성을 포함하는 메모리 시스템을 탐구한다.

마지막으로 5장에서는 3장이나 4장에 깔끔하게 들어맞지 않는 GPU 컴퓨팅 아키텍처에 대한 추가 연구의 개요를 제공한다.

---

# 2장: 프로그래밍 모델 (Programming Model)

이 장의 목표는 GPU에 대한 사전 경험이 없는 사람들도 이후 장의 논의를 따라갈 수 있도록, GPU가 비그래픽 컴퓨팅을 위해 어떻게 프로그래밍되는지에 대한 충분한 맥락을 제공하는 것이다. 현대 GPU는 GPU 애플리케이션의 데이터 수준 병렬성(data-level parallelism)을 활용하기 위해 넓은 SIMD 하드웨어를 사용한다. 이 SIMD 하드웨어를 프로그래머에게 직접 노출하는 대신, CUDA와 OpenCL과 같은 GPU 컴퓨팅 API는 프로그래머가 대규모 스칼라 스레드 배열을 GPU에 론칭할 수 있게 하는 MIMD와 유사한 프로그래밍 모델을 제공한다. 이러한 각 스칼라 스레드는 고유한 실행 경로를 따를 수 있으며 임의의 메모리 위치에 접근할 수 있다. 런타임에 GPU 하드웨어는 워프(warp, AMD 용어로는 웨이브프론트(wavefront))라고 불리는 스칼라 스레드 그룹을 SIMD 하드웨어에서 록스텝(lockstep)으로 실행하여 그들의 규칙성과 공간적 지역성(spatial locality)을 활용한다. 이 실행 모델을 단일 명령어 다중 스레드(single-instruction, multiple-thread, SIMT)라고 부른다.

## 2.1 실행 모델 (Execution Model)

GPU 컴퓨팅 애플리케이션은 CPU에서 실행을 시작한다. 디스크리트 GPU의 경우, 애플리케이션의 CPU 부분은 일반적으로 GPU에서의 연산에 사용할 메모리를 할당하고, 입력 데이터를 GPU 메모리로 전송을 시작한 후, 마지막으로 GPU에서 연산 커널을 론칭한다. 연산 커널은 (일반적으로) 수천 개의 스레드로 구성된다. 각 스레드는 동일한 프로그램을 실행하지만, 연산의 결과에 따라 해당 프로그램의 다른 제어 흐름을 따를 수 있다.

Figure 2.1은 SAXPY의 CPU 구현을 위한 C 코드를 제공한다. Figure 2.2는 CPU와 GPU에 걸쳐 실행을 분할하는 대응하는 CUDA 버전의 SAXPY를 제공한다.

Figure 2.2의 예제는 CUDA와 관련 프로그래밍 모델이 제공하는 추상화를 보여준다. 컴퓨트 커널을 구성하는 스레드들은 워프로 구성된 스레드 블록의 그리드(grid)로 이루어진 계층 구조로 조직된다. CUDA 프로그래밍 모델에서 개별 스레드는 피연산자가 스칼라 값인 명령어를 실행한다. 효율을 높이기 위해 일반적인 GPU 하드웨어는 스레드 그룹을 록스텝으로 함께 실행한다. 이 그룹을 NVIDIA는 워프(warp), AMD는 웨이브프론트(wavefront)라고 부른다. NVIDIA 워프는 32개의 스레드로 구성되고, AMD 웨이브프론트는 64개의 스레드로 구성된다. 워프는 협력 스레드 배열(cooperative thread array, CTA) 또는 NVIDIA 용어로 스레드 블록(thread block)이라 불리는 더 큰 단위로 그룹화된다.

CTA 내의 스레드들은 컴퓨트 코어별 스크래치패드 메모리를 통해 서로 효율적으로 통신할 수 있다. 이 스크래치패드를 NVIDIA는 공유 메모리(shared memory)라고 부른다. AMD의 Graphics Core Next(GCN) 아키텍처는 AMD가 로컬 데이터 스토어(local data store, LDS)라고 부르는 유사한 스크래치패드 메모리를 포함한다. 이러한 스크래치패드 메모리는 SM당 16-64 KB 범위로 크기가 작으며, 프로그래머에게 다른 메모리 공간으로 노출된다.

CTA 내의 스레드들은 하드웨어 지원 배리어 명령어를 사용하여 효율적으로 동기화할 수 있다. 서로 다른 CTA에 속한 스레드들도 통신할 수 있지만, 모든 스레드가 접근 가능한 전역 주소 공간(global address space)을 통해 통신한다.

NVIDIA는 Kepler 세대의 GPU에서 CUDA 동적 병렬성(CUDA Dynamic Parallelism, CDP)을 도입했다. CDP는 데이터 집약적인 비정규(irregular) 애플리케이션이 GPU에서 실행 중인 스레드 간에 부하 불균형(load imbalance)을 초래할 수 있다는 관찰에 의해 동기부여되었다.

## 2.2 GPU 명령어 집합 아키텍처 (GPU Instruction Set Architectures)

이 절에서는 CUDA와 OpenCL과 같은 고수준 언어에서 GPU 하드웨어가 실행하는 어셈블리 수준으로의 컴퓨트 커널 번역에 대해 간략히 논의한다. GPU 아키텍처에서 CPU 아키텍처와 다소 다른 흥미로운 측면은 GPU 생태계가 명령어 집합 진화를 지원하기 위해 발전해 온 방식이다. 예를 들어, x86 마이크로프로세서는 1976년에 출시된 Intel 8086과 하위 호환성(backwards compatible)을 유지한다.

### 2.2.1 NVIDIA GPU 명령어 집합 아키텍처

NVIDIA가 2007년 초에 CUDA를 도입했을 때, 유사한 경로를 따르기로 결정하고 GPU 컴퓨팅을 위한 자체 고수준 가상 명령어 집합 아키텍처인 병렬 스레드 실행 ISA(Parallel Thread Execution ISA), 즉 PTX를 도입했다. NVIDIA는 각 CUDA 릴리스와 함께 이 가상 명령어 집합 아키텍처를 완전히 문서화한다. PTX는 여러 면에서 ARM, MIPS, SPARC, ALPHA와 같은 표준 축소 명령어 집합 컴퓨터(RISC) 명령어 집합 아키텍처와 유사하다.

PTX 코드를 GPU에서 실행하기 전에 PTX를 하드웨어가 지원하는 실제 명령어 집합 아키텍처로 컴파일해야 한다. NVIDIA는 이 수준을 SASS라고 부르는데, 이는 "Streaming ASSembler"의 약자이다. PTX에서 SASS로의 변환 과정은 GPU 드라이버 또는 NVIDIA의 CUDA 툴킷과 함께 제공되는 ptxas라는 독립 실행 프로그램에 의해 수행될 수 있다. NVIDIA는 SASS를 완전히 문서화하지 않는다. 이로 인해 학술 연구자들이 모든 컴파일러 최적화 효과를 포착하는 아키텍처 시뮬레이터를 개발하기가 더 어려워지지만, NVIDIA는 고객들의 하드웨어 수준 하위 호환성 제공 요구에서 자유로워져 한 세대에서 다음 세대로 명령어 집합 아키텍처를 완전히 재설계할 수 있게 된다.

### 2.2.2 AMD Graphics Core Next 명령어 집합 아키텍처

NVIDIA와 대조적으로, AMD는 Southern Islands 아키텍처를 도입할 때 완전한 하드웨어 수준 ISA 사양을 공개했다. Southern Islands는 AMD의 Graphics Core Next(GCN) 아키텍처의 첫 번째 세대였다.

AMD의 GCN 아키텍처와 NVIDIA GPU 사이의 핵심적인 차이점은 별도의 스칼라(scalar) 명령어와 벡터(vector) 명령어이다. AMD GCN 아키텍처에서 각 컴퓨트 유닛은 하나의 스칼라 유닛과 네 개의 벡터 유닛을 포함한다. 벡터 명령어는 벡터 유닛에서 실행되며 웨이브프론트 내의 각 개별 스레드에 대해 서로 다른 32비트 값을 계산한다. 반면 스칼라 명령어는 스칼라 유닛에서 실행되며 웨이브프론트 내의 모든 스레드가 공유하는 단일 32비트 값을 계산한다.

AMD의 GCN 하드웨어 명령어 집합 매뉴얼은 AMD GPU 하드웨어에 대한 많은 흥미로운 통찰을 제공한다. 예를 들어, 긴 지연 시간 연산에 대한 데이터 의존성 해결을 가능하게 하기 위해 AMD의 GCN 아키텍처는 S_WAITCNT 명령어를 포함한다. 각 웨이브프론트에는 세 개의 카운터가 있다: 벡터 메모리 카운트(vector memory count), 로컬/전역 데이터 스토어 카운트(local/global data store count), 레지스터 내보내기 카운트(register export count).

---

# 3장: SIMT 코어: 명령어 및 레지스터 데이터 흐름 (The SIMT Core: Instruction and Register Data Flow)

이 장과 다음 장에서 우리는 현대 GPU의 아키텍처와 마이크로아키텍처를 살펴볼 것이다. GPU 아키텍처에 대한 논의를 두 부분으로 나눈다: (1) 이 장에서 연산을 구현하는 SIMT 코어를 살펴보고, (2) 다음 장에서 메모리 시스템을 살펴본다.

전통적인 그래픽 렌더링 역할에서 GPU는 상세한 텍스처 맵(texture map)과 같이 온칩(on-chip)에 완전히 캐시하기에는 너무 큰 데이터 세트에 접근한다. 고성능 프로그래밍 가능성을 실현하려면 대규모 오프칩(off-chip) 대역폭을 유지할 수 있는 아키텍처를 채택해야 한다. 따라서 오늘날의 GPU는 수만 개의 스레드를 동시에 실행한다. 스레드당 온칩 메모리 저장 공간은 작지만, 캐시는 여전히 상당수의 오프칩 메모리 접근을 줄이는 데 효과적일 수 있다.

Figure 3.1은 이 장에서 논의하는 GPU 파이프라인의 마이크로아키텍처를 보여준다. 파이프라인은 SIMT 프론트엔드(front-end)와 SIMD 백엔드(back-end)로 나눌 수 있다. 파이프라인은 단일 파이프라인에서 함께 작동하는 세 개의 스케줄링 "루프"로 구성된다: 명령어 페치 루프(instruction fetch loop), 명령어 이슈 루프(instruction issue loop), 그리고 레지스터 접근 스케줄링 루프(register access scheduling loop)이다.

## 3.1 단일 루프 근사 (One-Loop Approximation)

단일 스케줄러를 가진 GPU를 고려하는 것부터 시작한다. 효율성을 높이기 위해 스레드는 NVIDIA에서 "워프(warp)", AMD에서 "웨이브프론트(wavefront)"라 불리는 그룹으로 조직된다. 따라서 스케줄링의 단위는 워프이다. 매 사이클마다 하드웨어는 스케줄링할 워프를 선택한다.

### 3.1.1 SIMT 실행 마스킹 (Execution Masking)

현대 GPU의 핵심 특징은 SIMT 실행 모델로, 기능적 관점에서 프로그래머에게 개별 스레드가 완전히 독립적으로 실행된다는 추상화를 제공한다. 이 프로그래밍 모델은 프레디케이션(predication)만으로도 달성될 수 있다. 그러나 현재 GPU에서는 전통적인 프레디케이션과 프레디케이트 마스크 스택(stack of predicate masks)의 조합으로 구현되며, 이를 SIMT 스택(SIMT stack)이라 부른다.

SIMT 스택은 두 가지 핵심 문제를 효율적으로 처리하는 데 도움을 준다: 중첩 제어 흐름(nested control flow, 한 분기가 다른 분기에 제어 종속되는 경우)과 워프 내 모든 스레드가 특정 제어 흐름 경로를 피할 때 연산을 완전히 건너뛰는 것이다.

현재 GPU에서 사용되는 접근 방식은 주어진 워프 내에서 서로 다른 경로를 따르는 스레드의 실행을 직렬화(serialize)하는 것이다. SIMT 스택의 각 항목은 세 가지를 포함한다: 재수렴 프로그램 카운터(reconvergence program counter, RPC), 다음에 실행할 명령어의 주소(Next PC), 그리고 활성 마스크(active mask)이다.

재수렴 지점(reconvergence point)은 프로그램에서 분기한 스레드들이 다시 동기적으로(lock-step) 실행을 계속하도록 강제할 수 있는 위치이다. 일반적으로 가장 가까운 재수렴 지점이 선호된다. 주어진 프로그램 실행에서 컴파일 타임에 분기한 스레드들이 다시 동기적으로 실행할 수 있음을 보장할 수 있는 가장 이른 지점은 분기 발산을 유발한 분기문의 즉시 후위 지배자(immediate post-dominator)이다.

### 3.1.2 SIMT 교착 상태와 스택리스 SIMT 아키텍처

최근 NVIDIA는 Volta GPU 아키텍처의 세부 사항을 공개했다. 그들이 강조한 한 가지 변경 사항은 발산(divergence) 시 마스킹 동작과 이것이 동기화와 어떻게 상호작용하는지에 관한 것이다. 스택 기반의 SIMT 구현은 "SIMT 교착 상태(SIMT deadlock)"라 불리는 교착 상태 조건을 초래할 수 있다. 학술 연구에서는 SIMT 교착 상태를 방지할 수 있는 대안적 하드웨어를 기술했다. NVIDIA는 이 새로운 스레드 발산 관리 방식을 독립 스레드 스케줄링(Independent Thread Scheduling)이라 부른다.

핵심 아이디어는 스택을 워프별 수렴 배리어(per warp convergence barrier)로 대체하는 것이다. 수렴 배리어 메커니즘은 스케줄러가 발산된 스레드 그룹 간에 자유롭게 전환할 수 있는 대안적 구현을 제공한다. 이를 통해 일부 스레드가 락(lock)을 획득하고 다른 스레드는 획득하지 못한 상태에서도 워프 내 스레드 간 순방향 진행(forward progress)이 가능해진다.

### 3.1.3 워프 스케줄링 (Warp Scheduling)

GPU의 각 코어는 많은 워프를 포함한다. 메모리 시스템이 "이상적"이어서 메모리 요청에 고정 지연 시간 내에 응답한다면, 세립도 멀티스레딩(fine-grained multithreading)을 사용하여 이 지연 시간을 숨길 수 있을 만큼 충분한 워프를 지원하도록 코어를 설계할 수 있을 것이다. 이 경우 워프는 "라운드 로빈(round robin)" 순서로 스케줄링될 수 있다.

그러나 중요한 트레이드오프가 있다: 매 사이클마다 다른 워프가 명령어를 발행할 수 있게 하려면 각 스레드가 자신만의 레지스터를 가져야 한다. 따라서 코어당 워프 수를 늘리면 실행 유닛에 할당되는 비율 대비 레지스터 파일 저장에 할당되는 칩 면적 비율이 증가한다.

## 3.2 이중 루프 근사 (Two-Loop Approximation)

각 코어가 지원해야 하는 워프 수를 줄이는 데 도움을 주기 위해, 이전 명령어가 아직 완료되지 않은 상태에서도 워프로부터 후속 명령어를 발행할 수 있는 것이 유용하다. 의존성 정보를 제공하려면 먼저 메모리에서 명령어를 페치하여 어떤 데이터 및/또는 구조적 해저드(structural hazard)가 존재하는지 확인해야 한다. 이를 위해 GPU는 캐시 접근 후 명령어가 배치되는 명령어 버퍼(instruction buffer)를 구현한다. 명령어 버퍼에서 다음에 발행할 명령어를 결정하기 위해 별도의 스케줄러가 사용된다.

GPU는 순차 스코어보드(in-order scoreboard)를 구현한다. Coon 등이 제안한 설계는 워프당 소수의 항목을 포함하며, 각 항목은 발행되었지만 아직 실행이 완료되지 않은 명령어가 기록할 레지스터의 식별자이다.

## 3.3 삼중 루프 근사 (Three-Loop Approximation)

앞서 기술한 바와 같이, 긴 메모리 지연 시간을 숨기려면 코어당 많은 워프를 지원해야 한다. 예를 들어 NVIDIA의 최근 GPU 아키텍처에서 레지스터 파일은 256 KB를 포함한다. SRAM 메모리의 면적은 포트 수에 비례한다. 면적을 줄이는 한 가지 방법은 단일 포트 메모리의 다중 뱅크를 사용하여 많은 수의 포트를 시뮬레이션하는 것이다. 이를 투명하게 달성하기 위해 오퍼랜드 컬렉터(operand collector)라 알려진 구조가 사용된다.

### 3.3.1 오퍼랜드 컬렉터 (Operand Collector)

오퍼랜드 컬렉터 마이크로아키텍처는 스테이징 레지스터(staging register)를 컬렉터 유닛(collector unit)으로 대체한다. 각 명령어는 레지스터 읽기 단계에 진입할 때 컬렉터 유닛을 할당받는다. 여러 개의 컬렉터 유닛이 있어서 여러 명령어가 소스 오퍼랜드 읽기를 중첩할 수 있으며, 이는 뱅크 충돌(bank conflict) 상황에서 처리량을 향상시키는 데 도움이 된다.

오퍼랜드 컬렉터는 뱅크 충돌이 발생할 때 이를 허용하기 위해 스케줄링을 사용한다. Figure 3.16은 서로 다른 워프의 동일 레지스터를 다른 뱅크에 할당하여 뱅크 충돌을 줄이는 데 도움이 되는 수정된 레지스터 배치(swizzled)를 보여준다.

### 3.3.2 명령어 재실행: 구조적 해저드 처리 (Instruction Replay)

GPU 파이프라인에는 구조적 해저드의 잠재적 원인이 많다. 명령어가 구조적 해저드를 만나면 어떻게 되는가? GPU는 명령어 재실행(instruction replay)의 한 형태를 구현한다. 명령어 재실행은 파이프라인의 정체(clogging)와 스톨링(stalling)으로 인한 회로 면적 및/또는 타이밍 오버헤드를 방지하기 위해 GPU에서 사용된다.

## 3.4 분기 발산에 대한 연구 방향 (Research Directions on Branch Divergence)

### 3.4.1 워프 압축 (Warp Compaction)

동적 워프 형성(Dynamic Warp Formation, DWF)은 여러 발산된 정적 워프에 흩어진 스레드들을 같은 명령어를 실행하는 새로운 동적 워프로 재배열할 수 있다는 관찰을 활용한다. 스레드 블록 압축(Thread Block Compaction, TBC)은 이를 기반으로 압축이 스레드 블록 내에서만 발생하도록 제한한다. 대규모 워프 마이크로아키텍처(Large Warp Microarchitecture, LWM)는 워프 그룹의 재수렴을 관리하기 위해 SIMT 스택을 확장한다.

### 3.4.2 워프 내 발산 경로 관리 (Intra-Warp Divergent Path Management)

**다중 경로 병렬성(Multi-Path Parallelism):** 워프가 분기에서 발산하면 스레드들은 워프 분할(warp-split)이라 불리는 여러 그룹으로 나뉜다. 동적 워프 세분화(Dynamic Warp Subdivision, DWS)는 발산된 워프를 동시 워프 분할로 세분화하기 위해 SIMT 스택을 워프 분할 테이블로 확장한다. 다중 경로 실행 모델(Multi-Path Execution Model, MPM)은 SIMT 스택을 워프 분할 테이블과 재수렴 테이블 두 개의 테이블로 대체한다.

**더 나은 수렴(Better Convergence):** 후위 지배자 스택 기반 재수렴 메커니즘은 통합 알고리즘을 사용하여 식별된 재수렴 지점을 사용한다. 유사 수렴 지점(Likely-Convergence Points)은 SIMT 스택을 확장한다. 스레드 프론티어(Thread Frontiers)는 SIMT 스택에서 완전히 벗어난다.

### 3.4.3 MIMD 기능 추가 (Adding MIMD Capability)

벡터-스레드(Vector-Thread, VT) 아키텍처는 SIMD와 MIMD 아키텍처의 측면을 결합한다. 시간적 SIMT(Temporal SIMT)는 각 레인이 MIMD 방식으로 실행할 수 있게 한다. 가변 워프 크기(Variable Warp Size, VWS) 아키텍처는 각각 페치 및 디코드 유닛을 가진 다수의 슬라이스를 포함한다.

### 3.4.4 복잡도 효율적 발산 관리 (Complexity-Effective Divergence Management)

AMD GCN은 소프트웨어에서 SIMT 스택을 에뮬레이션할 수 있는 스칼라 레지스터 파일(scalar register file)을 제공한다. 스레드 프론티어(Thread Frontiers)는 SIMT 스택을 스레드별 PC로 대체한다. 스택리스 SIMT(Stackless SIMT)는 시간적 SIMT를 syncwarp 명령어로 확장한다. 프레디케이션(Predication)은 간단한 if 분기를 저오버헤드로 처리하는 방법으로 현대 GPU에 남아 있다.

## 3.5 스칼라화 및 아핀 실행에 대한 연구 방향 (Research Directions on Scalarization and Affine Execution)

### 3.5.1 균일 또는 아핀 변수의 탐지 (Detection of Uniform or Affine Variables)

**컴파일러 기반 탐지(Compiler-Driven Detection):** AMD GCN은 컴파일러에 의존한다. Asanovic 등은 수렴 분석과 변이 분석을 결합한 방법을 소개한다.

**하드웨어 탐지(Hardware Detection):** Collange 등의 태그 기반 탐지(Tag-Based Detection)는 각 GPU 레지스터에 태그를 확장한다. FG-SIMT 아키텍처는 분기에 대한 더 나은 지원으로 이를 확장한다. Gilani 등의 쓰기 시점 비교(Comparison at Write-Back)는 각 쓰기 시점에서 모든 스레드의 레지스터 값을 비교한다.

### 3.5.2 GPU에서 균일 또는 아핀 변수의 활용 (Exploiting Uniform or Affine Variables)

압축 레지스터 저장(Compressed Register Storage), 전용 스칼라 파이프라인(Dedicated Scalar Pipeline), 클록 게이트 SIMD 데이터패스(Clock-Gated SIMD Datapath), 아핀 워프로의 집합(Aggregate to Affine Warp), 메모리 접근 가속(Memory Access Acceleration).

## 3.6 레지스터 파일 아키텍처에 대한 연구 방향 (Research Directions on Register File Architecture)

현대 GPU는 많은 수의 하드웨어 스레드(워프)를 사용하며, 모든 하드웨어 스레드의 레지스터를 온칩 레지스터 파일에 저장한다. 예를 들어 NVIDIA의 Fermi GPU는 20,000개 이상의 인플라이트(in-flight) 스레드를 유지할 수 있으며, 총 레지스터 용량은 2 MB이다.

### 3.6.1 계층적 레지스터 파일 (Hierarchical Register File)
Gebhart 등은 주 레지스터 파일에 레지스터 파일 캐시(Register File Cache, RFC)를 추가하여 확장할 것을 제안하며, 이 계층 구조는 주 레지스터 파일에 대한 접근 빈도를 극적으로 줄인다.

### 3.6.2 졸린 상태 레지스터 파일 (Drowsy State Register File)
Abdel-Majeed와 Annavaram은 누설 전력(leakage power)을 줄이는 3모드 레지스터 파일 설계(ON, OFF, Drowsy)를 제안한다.

### 3.6.3 레지스터 파일 가상화 (Register File Virtualization)
Tarjan과 Skadron은 레지스터 리네이밍(register renaming)을 사용하여 물리 레지스터 파일 크기를 최대 50%까지 줄일 것을 제안한다. Jeon 등은 그 영향을 정량화하고 메타데이터 명령어를 제안한다.

### 3.6.4 분할 레지스터 파일 (Partitioned Register File)
Abdel-Majeed 등은 파일럿 레지스터 파일(Pilot Register File)을 도입하여, 빠른(일반 SRAM) 레지스터 파일과 느린(근임계 전압 SRAM, near-threshold voltage SRAM) 레지스터 파일로 분할한다.

### 3.6.5 RegLess
Kloosterman 등은 레지스터 파일을 제거하고 오퍼랜드 스테이징 버퍼(operand staging buffer)로 대체하는 것을 목표로 한다. 그들의 평가에 따르면 512 항목의 OSU가 2048 KB 레지스터 파일 대비 약간 더 나은 성능을 달성하면서 공간의 25%만 차지하고 전체 GPU 에너지 소비를 11% 줄인다.

---

# 4장: 메모리 시스템 (Memory System)

이 장에서는 GPU의 메모리 시스템을 탐구한다. GPU 컴퓨팅 커널은 load 및 store 명령어를 통해 메모리 시스템과 상호작용한다. CPU는 일반적으로 레지스터 파일(register file)과 메모리(memory)라는 두 개의 분리된 메모리 공간을 포함한다. 현대 GPU는 메모리를 논리적으로 로컬 메모리(local memory)와 글로벌 메모리(global memory) 공간으로 더 세분화한다. 로컬 메모리 공간은 스레드별로 사적(private)이며 일반적으로 레지스터 스필링(register spilling)에 사용되고, 글로벌 메모리는 다수의 스레드 간에 공유되는 데이터 구조에 사용된다. 현대 GPU는 일반적으로 협력 스레드 배열(cooperative thread array)에서 함께 실행되는 스레드들 간에 공유 접근이 가능한, 프로그래머가 관리하는 스크래치패드 메모리(scratchpad memory)를 구현한다.

## 4.1 1차 레벨 메모리 구조 (First-Level Memory Structures)

### 4.1.1 스크래치패드 메모리와 L1 데이터 캐시

CUDA 프로그래밍 모델에서 "공유 메모리(shared memory)"는 낮은 지연시간(low latency)을 가질 것으로 기대되지만 주어진 CTA 내의 모든 스레드가 접근 가능한 비교적 작은 메모리 공간을 의미한다. 공유 메모리는 레인(lane)당 하나의 뱅크(bank)를 가진 SRAM으로 구현되며, 각 뱅크는 하나의 읽기 포트(read port)와 하나의 쓰기 포트(write port)를 갖는다. 뱅크 충돌(bank conflict)은 하나 이상의 스레드가 주어진 사이클에서 같은 뱅크에 접근하면서 서로 다른 위치에 접근하려 할 때 발생한다.

L1 데이터 캐시는 글로벌 메모리 주소 공간의 일부를 유지한다. 워프 내의 모든 스레드가 단일 L1 데이터 캐시 블록 내에 속하는 위치에 접근하고 해당 블록이 캐시에 존재하지 않는 경우, 하위 레벨 캐시로 단 하나의 요청만 전송하면 된다. 이러한 접근을 "합쳐진(coalesced)" 접근이라 한다. 워프 내의 스레드들이 서로 다른 캐시 블록에 접근하면 다수의 메모리 접근이 생성되어야 한다. 이러한 접근을 합쳐지지 않은(uncoalesced) 접근이라 한다.

Figure 4.1은 통합된 공유 메모리와 L1 데이터 캐시를 구현하는 GPU 캐시 구성을 보여준다. 이 설계는 뱅크 충돌과 L1 데이터 캐시 미스를 처리할 때 리플레이(replay) 메커니즘을 사용하여 명령어 파이프라인과의 비정지(non-stalling) 인터페이스를 지원한다.

### 4.1.2 L1 텍스처 캐시

텍스처 캐시 태그 배열(tag array)은 데이터 배열(data array)보다 앞서 동작한다. 태그 배열의 내용은 미스 요청이 메모리까지 왕복하는 시간과 대략 동일한 시간이 지난 후 데이터 배열이 포함할 내용을 반영한다. 처리량(throughput)은 향상되지만, 캐시 히트와 미스 모두 대략 동일한 지연시간을 경험한다.

### 4.1.3 통합 텍스처 및 데이터 캐시

NVIDIA와 AMD의 최근 GPU 아키텍처에서는 데이터와 텍스처 값의 캐싱이 통합된 L1 캐시 구조를 사용하여 수행된다.

## 4.2 온칩 상호연결 네트워크 (On-Chip Interconnection Network)

고성능 GPU는 메모리 파티션 유닛(memory partition unit)을 통해 여러 DRAM 칩에 병렬로 연결된다. SIMT 코어는 온칩 상호연결 네트워크(on-chip interconnection network)를 통해 메모리 파티션 유닛에 연결된다. NVIDIA는 크로스바(crossbar)를 사용하고, AMD는 때때로 링 네트워크(ring network)를 사용해왔다.

## 4.3 메모리 파티션 유닛 (Memory Partition Unit)

각 메모리 파티션 유닛은 L2 캐시의 일부와 하나 이상의 메모리 접근 스케줄러(memory access scheduler) 및 래스터 연산(raster operation, ROP) 유닛을 포함한다.

### 4.3.1 L2 캐시
각 메모리 파티션 내부의 L2 캐시 부분은 두 개의 슬라이스(slice)로 구성된다. 각 캐시 라인은 GDDR5 DRAM 원자 크기(atom size)에 맞는 네 개의 32바이트 섹터(sector)를 갖는다.

### 4.3.2 원자적 연산 (Atomic Operations)
ROP 유닛은 원자적(atomic) 연산 및 리덕션(reduction) 연산을 실행하기 위한 기능 유닛을 포함하며, 파이프라이닝을 위한 로컬 ROP 캐시를 갖는다.

### 4.3.3 메모리 접근 스케줄러
DRAM은 각각 자체 행 버퍼(row buffer)를 가진 다수의 뱅크를 포함한다. 메모리 접근 스케줄러는 DRAM 메모리 접근 요청을 재정렬하여 행 전환 패널티(row switch penalty)를 줄인다. 각 스케줄러는 읽기 요청 정렬기(read request sorter)와 읽기 요청 저장소(read request store)를 사용하여 읽기 및 쓰기 요청을 분류하는 별도의 로직을 포함한다.

## 4.4 GPU 메모리 시스템 연구 방향 (Research Directions for GPU Memory Systems)

### 4.4.1 메모리 접근 스케줄링 및 상호연결 네트워크 설계
Yuan 등은 메모리 접근 스케줄러 설계를 탐구하며, 단일 SM에서 오는 요청은 행 버퍼 지역성(row-buffer locality)을 가지지만 다른 SM의 요청과 섞이면 이 지역성이 손실된다는 점을 관찰했다. Bakhoda 등은 온칩 상호연결 네트워크를 탐구하며 GPU 트래픽이 다대소대다(many-to-few-to-many) 패턴을 가진다는 것을 발견했다.

### 4.4.2 캐싱 효과 (Caching Effectiveness)
Bakhoda 등은 L1 및/또는 L2 캐시 추가의 영향을 연구했다. Jia 등은 NVIDIA Fermi 하드웨어에서 캐싱 효과를 특성화하고 세 가지 형태의 지역성(locality) — 워프 내(within-warp), 블록 내(within-block), 명령어 간(cross-instruction) — 에 대한 분류법(taxonomy)을 도입했다.

### 4.4.3 메모리 요청 우선순위 지정 및 캐시 바이패싱
Jia 등은 미스가 발생하고 연관도 정지(associativity stall)로 인해 캐시 블록을 할당할 수 없을 때 L1 데이터 캐시를 바이패스(bypass)하는 방법을 제안했다. 또한 "메모리 요청 우선순위 버퍼(memory request prioritization buffer, MRPB)"를 제안했다.

### 4.4.4 워프 간 이질성 활용 (Exploiting Inter-Warp Heterogeneity)
Ausavarungnirun 등은 워프 간 메모리 지연시간 발산(memory latency divergence)의 이질성을 활용하는 메모리 발산 보정(Memory Divergence Correction, MeDiC)을 제안했다. 워프는 전체 히트(all-hit), 대부분 히트(mostly-hit), 균형(balanced), 대부분 미스(mostly-miss), 전체 미스(all-miss)로 분류된다.

### 4.4.5 조율된 캐시 바이패싱 (Coordinated Cache Bypassing)
Xie 등은 프로파일링을 사용하여 각 정적 load 명령어를 좋은(good), 나쁜(poor), 보통(moderate) 지역성으로 표시함으로써 선택적으로 캐시 바이패싱을 활성화하는 방법을 탐구했다.

### 4.4.6 적응형 캐시 관리 (Adaptive Cache Management)
Chen 등은 보호 거리(protection distance)를 사용한 조율된 캐시 바이패싱과 워프 스로틀링(warp throttling)을 제안했다.

### 4.4.7 캐시 우선순위 지정 (Cache Prioritization)
Li 등은 어떤 워프가 L1 캐시에 라인을 할당할 수 있는지를 결정하기 위해 워프에 토큰(token)을 할당하는 방법을 제안했다. 추가적인 "비오염 워프(non-polluting warps)"는 실행할 수 있지만 데이터를 축출(evict)할 수 없다.

### 4.4.8 가상 메모리 페이지 배치 (Virtual Memory Page Placement)
Agarwal 등은 용량 최적화(capacity-optimized) 메모리와 대역폭 최적화(bandwidth-optimized) 메모리를 모두 갖춘 이기종 시스템에서의 페이지 배치를 고려했다.

### 4.4.9 데이터 배치 (Data Placement)
Chen 등은 명세 언어(specification language), 소스-투-소스 컴파일러(source-to-source compiler), 적응형 런타임 데이터 배치기(adaptive runtime data placer)를 갖춘 이식 가능한 데이터 배치 전략인 PORPLE을 제안했다.

### 4.4.10 다중 칩 모듈 GPU (Multi-Chip-Module GPUs)
Arunkumar 등은 무어의 법칙(Moore's Law) 한계를 넘어 성능 확장을 확장하기 위해 멀티칩 모듈(multichip module) 위에 작은 GPU 모듈들로 대형 GPU를 구축하는 방법을 제안했다.

---

# 5장: GPU 컴퓨팅 아키텍처에 대한 횡단적 연구 (Crosscutting Research on GPU Computing Architectures)

이 장에서는 이전 장에 깔끔하게 맞지 않는 GPGPU 아키텍처의 여러 연구 방향을 상세히 다룬다.

## 5.1 스레드 스케줄링 (Thread Scheduling)

현대 GPU는 대규모 병렬성(massive parallelism)에 의존한다. 스레드가 조직되고 스케줄링되는 세 가지 주요 방식이 있다:

1. **스레드를 워프에 할당 (Assignment of Threads to Warps)**
2. **스레드블록을 코어에 동적 할당 (Dynamic Assignment of Threadblocks to Cores)** — 작업이 스레드블록(threadblock) 단위로 대량 할당됨
3. **사이클별 스케줄링 결정 (Cycle-by-cycle Scheduling Decisions)**
4. **다중 커널 스케줄링 (Scheduling Multiple Kernels)**

### 5.1.1 스레드블록의 코어 할당에 관한 연구
Kayiran 등은 메모리 경합(memory contention)을 줄이기 위해 스레드블록 수를 스로틀링(throttling)하는 방법을 제안했다. Sethia와 Mahlke는 자원 경합을 동적으로 모니터링하고 스레드, 코어 주파수, 메모리 주파수를 조정하는 Equalizer를 제안했다.

### 5.1.2 사이클별 스케줄링 결정에 관한 연구
Lakshminarayana와 Kim은 다양한 워프 스케줄링 정책을 탐구했다. Gebhart 등은 워프를 활성(active) 풀과 비활성(inactive) 풀로 나누는 2단계 스케줄링(two-level scheduling)을 도입했다. Rogers 등은 빅팀 태그(victim tag) 기반의 손실 지역성 감지 메커니즘을 사용하여 메모리 시스템 피드백에 기반해 활성 워프를 스로틀링하는 캐시 인식 워프 스케줄링(Cache-Conscious Warp Scheduling, CCWS)을 제안했다. Rogers 등은 이후 CCWS를 확장한 발산 인식 워프 스케줄링(Divergence-Aware Warp Scheduling, DAWS)을 제안했다. Jog 등은 프리페칭 인식(prefetching-aware) 및 CTA 인식(CTA-aware) 워프 스케줄링을 탐구했다. Sethia 등은 메모리 인식 스케줄링과 캐시 접근 재실행을 결합한 Mascar를 소개했다.

### 5.1.3 다중 커널 스케줄링에 관한 연구
Park 등은 GPU에서 선점형 멀티태스킹(preemptive multitasking)을 지원하기 위한 Chimera를 제안했으며, 전체 컨텍스트 저장/복원(full context save/store), 완료 대기(waiting until finish), 또는 저장 없이 중지(멱등성인 경우)(stopping without saving, if idempotent) 중에서 동적으로 선택한다.

### 5.1.4 세밀한 동기화 인식 스케줄링 (Fine-Grain Synchronization Aware Scheduling)
ElTantawy와 Aamodt는 스핀 락(spin lock)을 동적으로 식별하고 스핀 루프를 실행하는 워프의 우선순위를 낮추는 하드웨어를 제안했다.

## 5.2 병렬성을 표현하는 대안적 방법 (Alternative Ways of Expressing Parallelism)

Kim과 Batten은 세밀한 하드웨어 워크리스트(fine-grain hardware worklist)를 제안했다. Lee 등은 중첩 병렬 패턴의 지역성 인식 매핑(Locality-Aware Mapping of Nested Parallel Patterns)을 제안했다. Wang과 Yalamanchili는 CUDA 동적 병렬성(Dynamic Parallelism)의 오버헤드를 특성화했다. Wang 등은 동적 스레드 블록 실행(Dynamic Thread Block Launch, DTBL)을 제안했다.

## 5.3 트랜잭셔널 메모리 지원 (Support for Transactional Memory)

### 5.3.1 Kilo TM
Kilo TM은 GPU 아키텍처를 위해 발표된 최초의 하드웨어 TM 제안이다. 값 기반 충돌 감지(value-based conflict detection)를 사용하며, 수백 개의 비충돌 트랜잭션에 대한 커밋 병렬성을 높이기 위해 최신성 블룸 필터(recency bloom filter)를 도입했다.

### 5.3.2 Warp TM과 시간적 충돌 감지
Xu 등은 GPU 소프트웨어 트랜잭셔널 메모리(STM) 시스템을 제안했다. 이 논문은 두 가지 핵심 최적화를 제안한다: (1) 같은 워프 내 스레드 간의 읽기-쓰기 충돌을 감지하는 워프 내 충돌 감지(intra-warp conflict detection), 그리고 (2) 트랜잭션의 시간적 순서를 활용하여 충돌 감지의 거짓 양성률(false positive rate)을 줄이는 시간적 충돌 감지(temporal conflict detection)이다.

## 5.4 이기종 시스템 (Heterogeneous Systems)

Power 등은 통합 CPU-GPU 시스템을 위한 이기종 시스템 일관성 프로토콜(heterogeneous system coherence protocol)을 제안했다. Lee와 Kim은 TLP 인식 캐시 관리 정책인 TAP를 제안했다. Qureshi와 Patt는 유틸리티 기반 캐시 파티셔닝(utility-based cache partitioning)을 제안했다.

---

# 저자 소개 (Authors' Biographies)

**Tor M. Aamodt**는 브리티시 컬럼비아 대학교(University of British Columbia)의 교수이다. 그는 널리 사용되는 GPGPU-Sim 시뮬레이터를 개발했다. 그는 NVIDIA에서 CUDA를 지원하는 최초의 NVIDIA GPU인 GeForce 8 시리즈 GPU의 메모리 시스템 아키텍처 작업에 참여했다.

**Wilson Wai Lun Fung**은 삼성전자(Samsung Electronics)의 아키텍트로, 차세대 GPU IP 개발에 기여하고 있다. 그는 GPGPU-Sim의 주요 기여자 중 한 명이었다.

**Timothy G. Rogers**는 퍼듀 대학교(Purdue University)의 조교수로, 대규모 멀티스레드 프로세서 설계에 집중하고 있다. 그는 NVIDIA Research와 AMD Research에서 인턴을 했다.
