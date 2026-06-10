# [한글 초안] Mind the Queue: GPU SIMT 병렬성을 NVMe 큐 아키텍처에 정렬하기

> **작업 방식**: 챕터별 한글 본문 완성 → 전체 확정 후 영문화.
> **분량 목표**: 실험(Evaluation)·결론(Conclusion) 제외 **총 7쪽** (full paper 형식).
>   - §1 서론 ~1.5쪽 / §2 배경 ~1.5쪽 / §3 분석 ~1.5쪽 / §4 설계 ~2.0쪽 / §7 관련 연구 ~0.5쪽
> **표기 원칙**: 약어는 최초 등장 시 전부 풀어 쓴다. 인용은 [저자+, 학회'연도] 형식으로
>   달고 문서 말미 참고문헌 목록에 모은다 (서지 확정 TODO 별도 표기).
> **상태**: §1 [초고 v2] / §2 [초고 v1 — 프로파일링 수치 TODO] / §3 [초고 v1] / §5 이후 [미작성]
> **챕터 구조 (2026-06-10 개편)**: §2 = 배경 및 동기 통합 (NVMe 모델 → CPU 선례 →
>   GPU 계층 → BaM 구조 제약 → 프로파일링), §3 = plink 아키텍처

---

# 1. 서론 (Introduction)

**¶1 — GPU 메모리 용량 부족이라는 구조적 문제**

대규모 언어 모델(Large Language Model, LLM)의 추론(inference)을 중심으로 한
최근의 인공지능 워크로드에서, GPU(Graphics Processing Unit)의 온보드
메모리는 시스템 전체에서 가장 희소한 자원이 되었다. 추론 시점에 GPU가
접근해야 하는 작업 집합(working set)은 모델 파라미터에 그치지 않는다.
컨텍스트 길이에 비례하여 증가하는 KV 캐시(Key-Value cache)는 긴 컨텍스트
서빙 환경에서 수십 기가바이트(GB)에 도달하고 [Kwon+, SOSP'23; Lee+,
OSDI'24], 전문가 혼합(Mixture-of-Experts, MoE) 모델은 토큰마다 일부만
활성화되는 전문가(expert) 가중치 전체를 보관해야 하며 [TODO: MoE
offloading 인용 확정], 추천 모델의 임베딩 테이블(embedding table)은
단일 모델이 테라바이트(TB) 규모에 이른다 [Naumov+, arXiv'19; Zhao+,
CIKM'19]. 벡터 검색의 색인(index) 또한 메모리 용량을 초과하는 대표적
사례다 [Jayaram Subramanya+, NeurIPS'19]. 그 결과, 최신 가속기의
고대역폭 메모리(High Bandwidth Memory, HBM)가 80GB에 달함에도 작업
집합이 이를 초과하는 상황은 예외가 아니라 일상이 되었다 [Sheng+,
ICML'23; Rajbhandari+, SC'21].

**¶2 — 계층화가 만들어내는 I/O의 형태**

프로덕션 시스템은 이 용량 격차를 저장 장치 계층화(tiering)로 해소한다.
당장 쓰이지 않는 차가운(cold) 상태 — 비활성 요청의 KV 캐시 블록,
비활성 전문가 가중치, 접근 빈도가 낮은 임베딩 행(row) — 를
NVMe(Non-Volatile Memory Express) SSD(Solid-State Drive)로 내보내고
(swap out), 필요해지는 순간 다시 불러들인다(swap in) [Sheng+, ICML'23;
Aminabadi+, SC'22; Lee+, OSDI'24; Gao+, ATC'24]. 이 과정에서 SSD에
도달하는 입출력(I/O)은 뚜렷한 형태를 갖는다. 첫째, 요청 크기가 **작다**.
4킬로바이트(KB) 페이지 하나에 KV 캐시 블록 몇 개 또는 임베딩 행 몇 줄이
담기는 세립(fine-grained) 접근이다. 둘째, 압도적으로 **읽기(read)
중심**이다 — 추론은 상태를 소비하는 작업이기 때문이다. 셋째, 접근
위치가 **불규칙(random)**하다. 어떤 토큰이 어떤 전문가와 어떤 캐시
블록을 필요로 할지는 입력에 의존하므로 미리 알 수 없다. 넷째,
**동시성이 극단적으로 높다**. 수천 개의 GPU 스레드가 각자 서로 다른
페이지를 같은 순간에 요구한다. 마지막으로, 이 읽기는 토큰 생성의
임계 경로(critical path) 위에 있다 — 페이지 도착이 지연된 만큼 토큰
생성이 멈추므로, 마이크로초(µs) 단위의 지연이 곧 서빙 처리량의 손실로
직결된다.

**¶3 — CPU 경유 경로의 한계와 GPU-initiated I/O의 부상**

전통적인 경로, 즉 GPU가 필요로 하는 데이터를 CPU(Central Processing
Unit)가 대신 읽어 GPU로 복사해 주는 구조는 이러한 트래픽 앞에서 두 가지
근본적 비용을 치른다. 데이터가 호스트 메모리를 한 번 경유(bounce)하며
PCIe(Peripheral Component Interconnect Express) 인터커넥트를 두 번
건너야 하고, "무엇을 읽을지"를 아는 주체(GPU 연산)와 읽기를 발행하는
주체(CPU)가 분리되어 있어 요청마다 GPU와 CPU 사이의 왕복 통신이 임계
경로에 박힌다. GPU-initiated I/O는 이 두 비용을 동시에 제거하는
접근이다. BaM [Qureshi+, ASPLOS'23]과 GIDS [Park+, VLDB'24] 같은
시스템은 GPU 스레드가 디바이스 코드 안에서 직접 NVMe 명령을 구성하여
제출하게 하고, 데이터는 PCIe 피어-투-피어(peer-to-peer, P2P)
DMA(Direct Memory Access)를 통해 SSD와 GPU 메모리 사이를 직접
이동시킨다 [Bergman+, ATC'17; Markussen+, TOCS'21]. 이들은 GPU가 CPU의
개입 없이 단일 SSD를 포화시킬 수 있음을 설득력 있게 입증했고, 그래프
신경망 학습과 대규모 데이터 분석에서 실질적인 응용 성능 향상을 보였다.

**¶4 — 그러나: 이식된 것은 CPU의 스레드 모델이다**

그러나 GPU가 SSD를 구동할 수 *있느냐*만큼, **어떻게** 구동하느냐가
중요하다. 우리가 주목하는 사실은, 기존 GPU-initiated I/O 엔진들이 GPU
스레드를 "동기(synchronous) I/O를 수행하는 CPU 스레드의 대체물"로
취급한다는 점이다. BaM의 큐 엔진에서 수천 개의 GPU 스레드는 소수의
NVMe 큐를 공유 풀(pool)로 사용한다. 각 스레드는 원자적(atomic) 티켓
연산으로 제출 큐(Submission Queue, SQ)의 슬롯을 경쟁적으로 획득하고,
자기 차례가 올 때까지 회전 대기(spin-wait)하며, 명령 제출 후에는 완료
큐(Completion Queue, CQ)를 자신의 명령 식별자(Command Identifier,
CID)가 나타날 때까지 반복 스캔하면서 블로킹(blocking)된 채 기다린다.
요컨대 하나의 I/O를 하나의 스레드가 처음부터 끝까지 동기적으로
책임지는 모델 — 운영체제 위의 CPU 스레드가 read() 시스템 콜을
부르는 것과 동형의 모델 — 이 GPU로 옮겨진 것이다.

**¶5 — 문제의 본질: NVMe가 구조로 주는 것을 동기화로 되사고 있다**

이 모델의 비용은 NVMe 아키텍처가 본래 제공하는 구조와 대조할 때
분명해진다. NVMe는 병렬성을 프로토콜 협상이 아니라 *구조*로 제공한다:
컨트롤러당 다수의 독립적인 SQ/CQ 쌍(pair), 큐당 깊은 비동기 큐 깊이
(queue depth), 꼬리(tail) 도어벨(doorbell) 한 번의 쓰기로 여러 명령을
일괄 제출하는 배칭(batching), 위상 비트(phase bit)를 이용한 순서대로의
(in-order) 완료 수확이 그것이다 [NVMe Spec]. 제출자(submitter)마다
전용 큐를 부여하면 이 모든 것이 동기화 없이 따라온다 — 리눅스
blk-mq의 코어별(per-core) 큐 매핑 [Bjørling+, SYSTOR'13]과 SPDK의
무공유(share-nothing) 설계 [Yang+, CloudCom'17]가 십여 년에 걸쳐
검증해 온 교훈이다. 반면 공유 큐 모델은 큐를 경합 지점으로 만들고,
NVMe가 공짜로 주는 것들을 티켓과 락(lock)과 식별자 검색이라는 동기화
비용을 지불하고 되산다. 그리고 이 낭비가 가장 첨예해지는 곳이, 바로
GPU-initiated I/O를 불러낸 그 워크로드다: 작은 읽기가 폭주하는
영역에서는 I/O 한 건당 오버헤드가 전체 비용을 지배하고, 추론이
진행되는 동안 GPU의 연산 유닛(Streaming Multiprocessor, SM) 사이클은
머신에서 가장 비싼 자원이다. **I/O 엔진이 티켓 락에 태우는 SM 사이클
하나하나가, 토큰 생성에서 빼앗아 온 사이클이다.**

**¶6 — 본 논문의 주장: 실행 계층을 큐 계층에 정렬하라**

본 논문은 GPU의 실행 계층을 NVMe의 큐 계층에 정렬할 것을 주장한다.
CPU 저장 스택의 교훈 — "큐를 독립 실행 단위에 매핑하면 큐에는 락이
필요 없다" — 에서 CPU의 독립 실행 단위가 코어(core)였다면, GPU에서
그 단위는 워프(warp)다: 32개 레인(lane)이 락스텝(lockstep)으로
실행되어 워프 내부의 협조는 사실상 무비용인 반면, 워프 사이의 공유
상태에 대한 원자적 연산은 비싼 직렬화 지점이기 때문이다. 따라서 우리는
**워프 하나가 SQ/CQ 쌍 하나를 배타적으로 소유**하는 설계를 제안한다.
이 단 하나의 불변식으로부터 공유 큐 모델의 동기화 장치 전부가
구조적으로 불필요해진다: SQ 꼬리 포인터와 CQ 머리(head) 포인터는
워프-사유(warp-private) 상태가 되고, 티켓도, 락도, 완료 큐에서의
식별자 검색도 존재할 이유가 사라진다. 우리는 이 원칙을 구현한
워프-네이티브(warp-native) GPU-initiated I/O 엔진 **plink**를 제시한다.
plink에서 각 워프는 전용 큐 쌍을 논블로킹(non-blocking), 깊이
N(depth-N)의 비동기 루프로 구동한다 — 완료를 위상 비트로 순서대로
수확하고, 빈 깊이만큼 새 명령을 채워 넣으며, 도어벨은 배치당 한 번만
울린다. 컨트롤러 초기화와 관리(admin) 명령 등 제어 평면(control
plane)은 호스트의 데몬(daemon)이 보유하여, GPU는 데이터 평면(data
plane)만을 담당한다.

**¶7 — 기여**

본 논문의 기여는 다음 세 가지다.

첫째, **진단**: BaM의 공유 큐 엔진에서 GPU 사이클이 어디로 사라지는지를
I/O 한 건 단위로 분해하여 정량적으로 분석한다. 워프 수가 늘어날수록
티켓 회전 대기, 꼬리 갱신 락 대기, 완료 큐 식별자 스캔 등 동기화
비용의 비중이 지배적이 됨을 보인다 (§3).

둘째, **설계**: 워프와 큐의 1:1 소유권 정렬에 기반한 plink의 설계와
구현을 제시한다 (§4). plink는 BaM과 동일한 libnvm 사용자 공간
드라이버 스택 위에서 큐 조작 계층만을 교체하므로, 두 시스템의 차이는
온전히 큐 설계의 차이로 귀속된다.

셋째, **초기 평가**: 단일 GPU와 단일 NVMe SSD 환경에서, plink가 BaM
대비 N배 적은 워프로 동일한 SSD를 포화시키고, I/O 한 건당 GPU 사이클을
M배 절감하며, 동일 처리량 조건에서 99.9 백분위(p99.9) 지연을 K배
개선함을 보인다 (§5). [수치는 측정 후 확정]

본 논문의 나머지는 다음과 같이 구성된다. §2는 NVMe 큐 모델과 고속
인터페이스가 제공하는 병렬성 구조, CPU 저장 스택이 수렴해 온 설계
원칙, 그리고 GPU 실행 계층을 정리한 뒤, BaM 큐 엔진의 구조적 제약을
해부하고 그 비용을 프로파일링으로 입증한다. §3은 plink의 아키텍처를
제시한다. §4는 평가 결과를, §5는 한계와 향후 과제를, §6은 관련 연구를
논한 뒤 §7로 맺는다.

---

# 2. 배경 및 동기 (Background and Motivation) — [초고 v1]

## 2.1 NVMe 큐 모델과 고속 인터페이스가 제공하는 병렬성

NVMe(Non-Volatile Memory Express)는 플래시 저장 장치의 내부 병렬성을
호스트에 노출하기 위해, 병렬성을 프로토콜 협상이 아니라 **자료구조의
배치(placement)로 제공**하도록 설계된 인터페이스다 [NVMe Spec]. 하나의
컨트롤러는 최대 65,535개의 입출력 큐 쌍 — 64바이트(B) 엔트리의 제출
큐(Submission Queue, SQ)와 16바이트 엔트리의 완료 큐(Completion Queue,
CQ) — 을 지원하며, 각 큐는 최대 65,536개의 엔트리 깊이(queue depth)를
가질 수 있다. 호스트가 SQ에 명령을 써 넣고 컨트롤러의 BAR0(Base
Address Register 0) 공간에 매핑된 꼬리 도어벨(tail doorbell)
레지스터에 새 꼬리 값을 한 번 쓰면, 컨트롤러가 명령들을
DMA(Direct Memory Access)로 가져가 실행하고, 완료는 위상
비트(phase bit)가 토글된 완료 엔트리(CQE)로 CQ에 순서대로 적재된다.

이 구조에서 핵심은 **큐를 제출자(submitter)마다 전속으로 부여했을 때
따라오는 것들**이다. 제출자가 유일하면 SQ의 꼬리 포인터는 단일
쓰기자(single writer)가, CQ의 머리(head) 포인터는 단일 읽기자(single
reader)가 된다 — 즉 큐 조작에 동기화가 **구조적으로** 불필요해진다.
나아가 (i) 여러 명령을 SQ에 쌓은 뒤 도어벨을 한 번만 울리는 일괄
제출(doorbell batching), (ii) 위상 비트 검사만으로 완료를 머리에서부터
순서대로 수확하는 O(1) 완료 판정, (iii) 큐 깊이만큼의 미결(outstanding)
명령으로 장치 지연을 은닉하는 비동기 운용이 전부 추가 비용 없이
가능해진다. 고속 인터커넥트 관점에서도 이 설계는 의도적이다:
PCIe(Peripheral Component Interconnect Express) 위에서
MMIO(Memory-Mapped I/O) 쓰기는 포스티드(posted) 트랜잭션이지만
직렬화 비용이 크므로, 인터페이스 효율은 도어벨 쓰기 횟수를
명령 수보다 훨씬 적게 유지하는 데에서 나온다. 요컨대 NVMe는 다음과
같은 **계약(contract)**을 제시한다: *제출자마다 전용 큐를 부여하라,
그러면 제출 경로에서 동기화가 사라진다.*

## 2.2 CPU 저장 스택이 수렴해 온 설계 원칙

CPU 기반 NVMe 저장 스택의 발전사는 이 계약을 이행하는 과정이었다.
리눅스 blk-mq는 코어별(per-core) 소프트웨어 큐를 하드웨어 큐에
매핑하고, 제출 경로를 무락(lock-free) 태그 비트맵으로 구성했으며,
인터럽트 친화도(IRQ affinity)를 통해 완료가 제출한 코어로 돌아오게
하여 지역성(locality)을 보존한다 [Bjørling+, SYSTOR'13]. SPDK(Storage
Performance Development Kit)는 한 걸음 더 나아가 무공유(share-nothing)
원칙을 채택했다: 코어마다 전용 큐 쌍을 부여하고 완료를
폴링(polling)으로 수확하여, 설계상 락이 0개인 사용자 공간 드라이버를
구현했다 [Yang+, CloudCom'17]. io_uring 역시 제출 링과 완료 링을
분리하고 커널 측 폴링(SQPOLL)을 제공하여 시스템 콜과 인터럽트를
제출 경로에서 제거한다 [Axboe'19]. 세 시스템의 공통 정책은 명확하다:
(1) 큐를 독립 실행 단위에 1:1로 매핑하고, (2) 제출을 배칭하며,
(3) 완료 수확을 제출 컨텍스트에 정렬한다. 십여 년에 걸친 교훈은
하나로 요약된다 — **큐를 독립 실행 단위에 매핑하면, 큐에는 락이
필요 없다. CPU에서 그 단위는 코어였다.**

## 2.3 GPU 실행 계층: 무엇이 싸고 무엇이 비싼가

GPU의 실행 계층에서 같은 질문 — 어느 단위가 큐를 소유해야 하는가 —
에 답하려면, 계층별 협조 비용의 비대칭을 보아야 한다. GPU는
SIMT(Single-Instruction, Multiple-Thread) 모델로 실행된다: 32개의
레인(lane)이 락스텝(lockstep)으로 움직이는 워프(warp)가 스케줄링의
기본 단위이고, 워프들이 모여 블록(block)과 그리드(grid)를 이룬다.
**워프 내부**의 협조 — 셔플(shuffle), 투표(ballot),
`__syncwarp()` — 는 레지스터 수준에서 일어나므로 사실상 무비용이다.
반면 **워프 사이**의 협조는 전역 메모리의 공유 상태에 대한 원자적
읽기-수정-쓰기(atomic Read-Modify-Write, RMW)를 요구하며, 이는 L2
캐시의 해당 캐시라인에서 직렬화된다. 수천 개의 워프가 같은 변수를
원자적으로 갱신하면 그 변수가 곧 병목이 된다. 한 가지 제약이 더
있다: GPU 디바이스 코드는 시스템 콜을 부를 수 없으므로, 장치와의
모든 상호작용은 메모리 폴링과 MMIO 쓰기로 구성해야 하며, 입출력
서비스를 상주시키려면 커널이 종료되지 않고 계속 도는 지속
커널(persistent kernel) 패턴을 쓴다. 이 비용 구조가 시사하는 바는
분명하다: GPU에서 동기화 없이 독립적으로 진행할 수 있는 실행 단위,
즉 CPU의 코어에 해당하는 단위는 **워프**다.

## 2.4 BaM 큐 엔진의 구조와 제약

BaM은 GPU 스레드가 NVMe 장치를 직접 구동하는 최초의 본격적인
시스템으로, 배열(array) 추상화와 소프트웨어 캐시를 통해 응용이
저장 장치를 메모리처럼 접근하게 했다 [Qureshi+, ASPLOS'23]. 본 절의
분석은 이 기여를 부정하지 않는다 — 우리가 해부하는 것은 그 추상화
**아래에 있는 큐 엔진**이며, 이 엔진은 GIDS [Park+, VLDB'24] 등 후속
시스템에도 그대로 상속되었으므로 분석의 일반성이 성립한다.

BaM의 큐 엔진(libnvm의 `nvm_parallel_queue`)에서 NVMe 큐는 모든 GPU
스레드가 공유하는 풀(pool)이다. 스레드 하나가 입출력 한 건을 제출하고
완료를 확인하는 경로는 다음 단계로 구성된다 (그림 1):

1. **티켓 획득**: 전역 원자 변수 `in_ticket`에 fetch-and-add를 수행해
   자기 순번을 받는다. 순번을 큐 깊이로 나눈 나머지가 SQ 슬롯 위치다.
2. **차례 대기**: 해당 슬롯의 티켓 카운터가 자기 순번과 일치할 때까지
   회전 대기(spin-wait)한다 — 같은 슬롯을 먼저 배정받은 이전 세대의
   명령이 끝나기를 기다리는 것이다.
3. **명령 기록**: 64바이트 제출 엔트리를 슬롯에 쓴다.
4. **꼬리 갱신 경쟁**: 슬롯에 "준비됨" 표시(tail mark)를 남긴 뒤, 전역
   꼬리 락(tail lock)을 두고 경쟁한다. 락을 획득한 스레드 하나가 연속된
   준비 슬롯들을 훑어(sweep) 꼬리를 전진시키고 도어벨을 MMIO로 한 번
   울린다. 락을 얻지 못한 스레드는 자기 표시가 지워질 때까지 회전
   대기한다.
5. **완료 대기**: 자신의 명령 식별자(Command Identifier, CID)가 완료
   큐에 나타날 때까지, CQ를 머리에서부터 — 최악의 경우 큐 깊이
   전체를 — 매 반복 선형 스캔하는 블로킹 폴(blocking poll)을 돈다.
   같은 CQ를 기다리는 모든 스레드가 이 스캔을 중복 수행한다.
6. **완료 정리**: CQ 꼬리 원자 갱신, 슬롯 잠금(pos lock) 획득, 머리
   갱신 락(head lock) 경쟁이 추가로 뒤따른다.

이 구조를 §2.1의 계약에 비추면 제약이 다섯 가지로 정리된다.
첫째, **큐가 경합 지점이 된다**: NVMe가 제출자별 전속으로 의도한 큐를
수천 스레드가 공유하므로, 큐 자체가 원자 연산과 락의 대상이 된다.
둘째, **비동기 깊이를 쓰지 못한다**: 스레드 하나가 입출력 한 건을
동기적으로 책임지므로, 미결 명령 수는 큐 깊이가 아니라 스레드 수로만
늘릴 수 있다 — 지연 은닉을 위해 워프를 과잉
투입(oversubscription)해야 하고, 그만큼 연산에 쓸 SM(Streaming
Multiprocessor)이 줄어든다. 셋째, **도어벨 배칭을 동기화로 되산다**:
단일 제출자라면 공짜인 일괄 제출을, 꼬리 락 경쟁과 슬롯 표시
스위핑이라는 비용을 지불하고 복원한다. 넷째, **순서대로의 완료 수확을
버린다**: 위상 비트만 보면 O(1)인 완료 판정 대신, 스레드마다 자기
식별자를 찾는 O(큐 깊이) 스캔을 — 그것도 중복으로 — 수행한다.
다섯째, **큐 수를 늘려도 구조가 바뀌지 않는다**: 큐 풀이 커져도
공유와 경쟁이라는 모델 자체는 그대로이므로, 경합은 완화될 뿐
사라지지 않는다.

## 2.5 비용 프로파일링: GPU 사이클은 어디로 가는가

위 구조 분석이 실제 비용으로 어떻게 나타나는지 확인하기 위해, 우리는
BaM의 큐 엔진을 계측하여 입출력 한 건당 GPU 사이클의 사용처를
분해했다. 측정 방법: 제출-완료 경로의 각 단계(티켓 획득, 차례 대기,
엔트리 기록, 꼬리 락 대기, 완료 스캔, 완료 정리) 전후에서 GPU 사이클
카운터(`clock64()`)를 읽어 단계별 소요를 기록하고, 4KB 임의
읽기(random read) 워크로드에서 워프 수를 1개부터 128개까지 늘려가며
반복한다. [환경: GPU 1기, NVMe SSD 1기 — §4 평가 환경과 동일. 수치
측정 TODO]

**그림 2** [측정 후 삽입]: 워프 수에 따른 입출력 1건당 사이클 분해
(누적 영역 그래프). 코드 구조로부터 예상되는 형태는 다음과 같다 —
워프가 적을 때는 엔트리 기록과 장치 대기가 지배적이지만, 워프 수가
늘수록 (i) 티켓 차례 대기(같은 슬롯의 이전 세대 점유), (ii) 꼬리 락
경쟁, (iii) 완료 큐 스캔량이 초선형으로 증가하여 동기화 비용이 전체를
지배하게 된다. **표 1**은 코드 분석으로 확정되는 입출력 1건당 공유
상태 연산 횟수를 정리한 것이다: 전역 원자 RMW 3회 이상(티켓 발급,
티켓 해제, CQ 꼬리), 락 경쟁 2종(꼬리 락, 머리 락), 회전 대기 구간
3곳, 그리고 완료당 최대 큐 깊이에 비례하는 CQE 읽기.

[**그림 3** — 측정 후 삽입: 동일 목표 IOPS(초당 입출력 수)를 달성하기
위해 필요한 워프 수. 과잉 투입 비용을 정량화.]

강조할 점은 이것이 BaM의 구현 결함이 아니라 **공유 큐 모델의 구조적
귀결**이라는 사실이다. 스레드가 큐를 공유하는 한, 티켓이든 락이든
어떤 형태로든 그 비용은 지불해야 한다. 비용을 없애는 길은 더 정교한
동기화가 아니라 **동기화가 필요 없는 소유권 구조**, 즉 큐를 GPU의
독립 실행 단위에 전속시키는 것이다. 다음 장에서 이 원칙을 구현한
plink의 아키텍처를 제시한다.

---

# 3. plink 아키텍처 (Architecture) — [초고 v1]

## 3.1 설계 원칙과 전체 구조

plink의 설계는 하나의 불변식에서 출발한다: **워프 하나가 SQ/CQ 쌍
하나를 배타적으로 소유하며, 큐 상태에 워프 간 공유 가변 상태를 두지
않는다.** SQ의 꼬리 포인터, CQ의 머리 포인터, 위상 비트, 미결
(outstanding) 명령 수, 다음 슬롯 번호 — 큐 운용에 필요한 모든 상태가
워프-사유(warp-private)다. §2.4에서 해부한 BaM의 동기화 장치들 —
티켓 발급과 차례 대기, 꼬리 락과 슬롯 표시 스위핑, 완료 큐 식별자
검색 — 은 더 빠르게 만들어야 할 대상이 아니라, 이 불변식 아래에서
**존재할 이유가 사라지는 대상**이다. 우리는 BaM의 큐를 더 영리하게
만들지 않았다; 큐가 영리해야 할 이유를 제거했다.

전체 구조는 두 평면으로 나뉜다 (그림 4). **데이터 평면**은 GPU에
상주하는 지속 커널(persistent kernel)이다: N개의 워프가 각각 전용
SQ/CQ 쌍을 구동하며, 큐 메모리와 데이터 버퍼는 GPU
VRAM(`cudaMalloc`)에 배치되어 SSD가 PCIe 피어-투-피어 DMA로 직접
읽고 쓴다. 컨트롤러의 도어벨 영역(BAR0)은 GPU 주소 공간에 매핑되어
디바이스 코드가 직접 MMIO 쓰기를 수행한다. **제어 평면**은 호스트의
사용자 공간 데몬이다: 장치 열기, 컨트롤러 리셋, 관리(admin) 큐 소유,
입출력 큐 생성과 자원 등록을 전담한다 (§3.5). GPU는 관리 큐를 절대
만지지 않고, 호스트는 정상 상태(steady state)의 입출력 경로에 절대
개입하지 않는다.

BaM과의 차이는 세 가지 메커니즘으로 구체화된다: (i) 제출 경로와 완료
경로의 분리를 통한 병렬성 확보 (§3.2), (ii) 명령 단위 대기가 아닌,
NVMe 드라이버 방식의 즉시 완료 수확 (§3.3), (iii) 워프 단위로
배칭되는 도어벨 MMIO 쓰기 (§3.4). 각각이 §2.4의 제약과 어떻게
대응되는지를 함께 서술한다.

**표 2** — BaM과 plink의 큐 운용 대비 (요약):

| 구분 | BaM (공유 큐) | plink (워프 소유 큐) |
|---|---|---|
| 큐 소유권 | 전체 스레드가 풀 공유 | 워프당 전용 SQ/CQ 쌍 |
| 제출/완료 결합 | 한 스레드가 제출→대기→완료 동기 수행 | 제출·완료가 분리된 비동기 단계 |
| 지연 은닉 주체 | 스레드 수 (과잉 투입) | 큐 깊이 (워프당 미결 N건) |
| 완료 판정 | 자기 CID를 O(깊이) 중복 스캔 | 위상 비트 O(1), 도착 순 즉시 처리 |
| 도어벨 | 꼬리 락 경쟁 + 스위핑으로 일괄화 | 워프 단위 배치, 락 없음 |
| 공유 상태 원자 연산 | 입출력 1건당 3회 이상 + 락 2종 | **0회** |

## 3.2 SQ/CQ 워크 분리: 제출과 완료의 비동기화

BaM에서 제출과 완료는 한 스레드 안에 직렬로 결합되어 있다 — 스레드는
명령을 제출한 뒤 그 명령이 끝날 때까지 다른 일을 하지 못한다. 이
결합이 §2.4의 두 번째 제약(비동기 깊이 미사용)의 직접적 원인이다:
미결 명령을 늘리는 유일한 방법이 스레드를 늘리는 것이기 때문이다.

plink는 제출 작업과 완료 작업을 **서로를 기다리지 않는 별개의
단계**로 분리한다. 각 워프의 메인 루프는 두 단계를 교대로 수행한다
(Listing 1). **수확(harvest) 단계**는 CQ 머리에서 위상 비트가 일치하는
완료 엔트리들을 도착한 만큼만 — 하나도 없으면 즉시 통과 — 논블로킹으로
처리한다. **재충전(refill) 단계**는 비워진 깊이만큼 새 명령을 SQ에
구성해 넣는다. 어느 단계도 특정 명령의 완료를 기다리며 멈추지
않으므로, 워프 하나가 항상 깊이 N의 미결 명령을 유지한 채 전진한다.
장치가 앞선 배치를 처리하는 동안 워프는 다음 배치를 구성한다 —
**지연 은닉의 주체가 스레드 수에서 큐 깊이로 옮겨가는 것**이며, 이는
NVMe가 의도한 바로 그 운용 방식이다 (§2.1).

```
Listing 1 — plink 워프 메인 루프 (의사코드, 모든 상태는 워프-사유)
state: tail, head, phase, outstanding, next_slot
loop until stop:
  # 수확: 도착해 있는 완료만 즉시 처리 (논블로킹)
  while cqe[head].phase == phase:
      complete(slot = cqe[head].cid)      # CID = 슬롯 번호 (§3.3)
      head++; outstanding--
  ring_cq_doorbell(head)                  # 배치당 MMIO 1회 (§3.4)
  # 재충전: 빈 깊이만큼 새 명령 구성
  while outstanding < DEPTH:
      build_sqe(slot = next_slot, cid = next_slot, lba = next())
      tail++; outstanding++; next_slot = (next_slot + 1) mod DEPTH
  ring_sq_doorbell(tail)                  # 배치당 MMIO 1회 (§3.4)
```

이 분리는 워프 내부에서 한 번 더 일어난다. 64바이트 제출 엔트리의
작성은 워프의 레인들이 분담하여 병합(coalesced) 쓰기로 수행하고, 완료
수확과 큐 포인터 관리는 대표 레인이 담당한다. §2.3에서 본 비용
비대칭을 그대로 활용하는 배치다 — **무비용인 워프 내부 병렬성은
최대로 쓰고, 비싼 워프 간 동기화는 전혀 쓰지 않는다.** 결과적으로
제출 처리량(레인 병렬 SQE 작성)과 완료 처리량(즉시 수확)이 서로를
가로막지 않고 동시에 확장된다.

## 3.3 드라이버식 완료 처리: 식별자 검색이 아닌 즉시 수확

BaM의 완료 처리는 "내 명령이 끝났는가"를 묻는다 — 각 스레드가 자기
명령 식별자(CID)를 완료 큐에서 찾을 때까지 큐 전체를 반복 스캔한다.
plink의 완료 처리는 질문 자체를 바꾼다: **"도착한 완료가 있는가"**.
이는 운영체제의 NVMe 드라이버가 완료를 다루는 방식과 동형이다 —
인터럽트 핸들러나 폴링 루프는 특정 요청을 기다리지 않고, CQ 머리에서
위상 비트가 유효한 엔트리를 도착 순서대로 모두 처리하며, 엔트리에
적힌 CID로 해당 요청의 상태를 역참조하여 완료시킨다. 제출자가 누구든,
끝난 것부터 즉시 처리된다.

plink에서 이 역참조는 배열 인덱싱 한 번이다. 명령 발급 시 CID를 큐
슬롯 번호와 같게 부여하므로(Listing 1), 완료 엔트리의 CID가 곧 해당
요청의 상태(데이터 버퍼 위치, 제출 시각)가 저장된 슬롯의 인덱스다.
완료 판정은 위상 비트 비교 한 번 — O(1) — 이고, 검색 루프도, 같은
큐를 여러 스레드가 중복으로 읽는 낭비도 없다(CQ는 소유 워프만 읽는
단일 읽기자다). §2.4의 네 번째 제약(O(깊이) 중복 스캔)이 이 지점에서
해소된다. 완료가 도착 순서대로 — 장치가 큐에 적재한 그 순서 그대로 —
소비되므로, 완료 큐 메모리에 대한 접근도 순차적이 되어 캐시 효율
면에서도 유리하다.

## 3.4 워프 단위 배치 도어벨

도어벨 MMIO 쓰기는 PCIe를 건너는 포스티드 쓰기로, 횟수 자체가
인터페이스 비용이다 (§2.1). plink에서 도어벨은 명령마다가 아니라
**단계(배치)마다 한 번** 울린다: 재충전 단계에서 깊이만큼의 제출
엔트리를 모두 써 넣은 뒤 SQ 꼬리 도어벨을 1회, 수확 단계에서 처리한
완료들을 반영하여 CQ 머리 도어벨을 1회 — 따라서 MMIO 횟수는 입출력
수가 아니라 배치 수에 비례한다. 도어벨 직전에는 시스템 수준 메모리
펜스로 제출 엔트리 쓰기가 장치에 가시화되는 순서를 보장한다.

강조할 점은 이 배칭에 **아무런 동기화도 들지 않는다**는 것이다. BaM은
같은 효과(일괄 제출)를 얻기 위해 꼬리 락 경쟁과 슬롯 표시 스위핑을
지불해야 했다 — 공유 큐에서는 "누가 도어벨을 울릴 것인가"부터가
경합이기 때문이다 (§2.4 세 번째 제약). plink에서는 꼬리의 쓰기자가
워프 하나뿐이므로, NVMe 계약이 약속한 그대로 배칭이 공짜로 따라온다.

## 3.5 제어 평면: 호스트 데몬과 표준 도구 호환

NVMe 컨트롤러의 생애 주기 관리는 고빈도 경로가 아니다 — 따라서
GPU에 둘 이유가 없다. plink의 호스트 데몬은 libnvm 사용자 공간
드라이버 [Markussen+, TOCS'21]로 장치를 열어 컨트롤러를 리셋하고,
관리 큐를 소유하며, Set Features 명령으로 큐 수를 확정한 뒤 GPU용
입출력 큐들을 생성한다 — 큐 메모리를 GPU VRAM에 할당하고
(`cudaMalloc`), GPU 메모리의 DMA 주소(IOVA)를 확보하여
(nvidia_p2p), Create I/O CQ/SQ 관리 명령으로 컨트롤러에 등록한다.
이후 지속 커널을 런치하면 데이터 평면이 가동된다. 워크로드
파라미터(워프 수, 큐 깊이, 블록 크기, 접근 패턴, 실행 시간)는
데몬이 노출하는 캐릭터 디바이스의 ioctl로 전달받는다.

이 분리의 부수 효과로, plink는 표준 도구 생태계와 공존한다. 데몬은
CUSE(Character device in USErspace)로 표준 NVMe 캐릭터 디바이스를
노출하므로, 외부의 `nvme-cli` 같은 도구가 — GPU가 입출력 큐를
포화시키는 동안에도 — 관리 명령(identify, SMART 로그 조회 등)을
동일 컨트롤러에 발행할 수 있다. GPU는 데이터 평면만, 호스트는 제어
평면만: 두 평면은 서로 다른 큐를 사용하므로 정상 상태에서 충돌하지
않는다.

## 3.6 구현

plink는 BaM과 동일한 libnvm 스택 위에 구현되었으며, 교체된 것은 큐
조작 계층뿐이다 — 컨트롤러 초기화, DMA 매핑, 피어-투-피어 경로는
두 시스템이 공유한다. 따라서 §4의 평가에서 관찰되는 차이는 온전히 큐
설계의 차이로 귀속된다. 데이터 평면은 CUDA로 작성된 지속 커널과
디바이스 측 큐 조작 함수들로, 제어 평면 데몬과 CUSE 프론트엔드는
C(libfuse3)로 작성되었다. [구현 분량·코드 라인 수 등 세부는 구현
확정 후 기입 — TODO. 현재 GPU 메인 입출력 경로는 설계 확정·구현
진행 중이며, 본문 서술은 목표 구현 기준이다.]

# 4. 평가 (Evaluation) — [본 7쪽 목표에서 제외, 별도 작성]

# 5. 논의 및 향후 과제 (Discussion & Future Work) — [미작성]

# 6. 관련 연구 (Related Work) — [미작성]

# 7. 결론 (Conclusion) — [본 7쪽 목표에서 제외, 별도 작성]

---

# 참고문헌 후보 목록 (서지 확정 TODO)

| 태그 | 서지 (확인 상태) |
|---|---|
| [Qureshi+, ASPLOS'23] | Z. Qureshi et al., "GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture," ASPLOS 2023. (확정) |
| [Park+, VLDB'24] | GIDS — GPU Initiated Direct Storage Accesses 기반 GNN 가속, PVLDB 17, 2024. (저자·제목 확인 필요) |
| [Kwon+, SOSP'23] | W. Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention" (vLLM), SOSP 2023. (확정) |
| [Lee+, OSDI'24] | InfiniGen — KV cache 오프로딩 관리, OSDI 2024. (저자 확인 필요) |
| [Gao+, ATC'24] | CachedAttention/AttentionStore — KV cache 계층 저장, USENIX ATC 2024. (저자·제목 확인 필요) |
| [Sheng+, ICML'23] | Y. Sheng et al., "FlexGen: High-Throughput Generative Inference of Large Language Models with a Single GPU," ICML 2023. (확정) |
| [Rajbhandari+, SC'21] | S. Rajbhandari et al., "ZeRO-Infinity: Breaking the GPU Memory Wall for Extreme Scale Deep Learning," SC 2021. (확정) |
| [Aminabadi+, SC'22] | R. Y. Aminabadi et al., "DeepSpeed-Inference," SC 2022. (확정) |
| [Naumov+, arXiv'19] | M. Naumov et al., "Deep Learning Recommendation Model for Personalization and Recommendation Systems," arXiv 2019. (확정) |
| [Zhao+, CIKM'19] | W. Zhao et al., "AIBox: CTR Prediction Model Training on a Single Node" (SSD 기반 임베딩), CIKM 2019. (제목 확인 필요) |
| [Jayaram Subramanya+, NeurIPS'19] | S. Jayaram Subramanya et al., "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node," NeurIPS 2019. (확정) |
| [Bergman+, ATC'17] | S. Bergman et al., "SPIN: Seamless Operating System Integration of Peer-to-Peer DMA Between SSDs and GPUs," USENIX ATC 2017. (확정) |
| [Markussen+, TOCS'21] | J. Markussen et al., SmartIO / ssd-gpu-dma(libnvm), ACM TOCS 2021. (제목 확인 필요) |
| [Bjørling+, SYSTOR'13] | M. Bjørling et al., "Linux Block IO: Introducing Multi-queue SSD Access on Multi-core Systems," SYSTOR 2013. (확정) |
| [Yang+, CloudCom'17] | Z. Yang et al., "SPDK: A Development Kit to Build High Performance Storage Applications," CloudCom 2017. (확정) |
| [NVMe Spec] | NVM Express Base Specification, Rev 2.x. (확정) |
| [TODO: MoE offloading] | MoE expert 오프로딩 — MoE-Infinity 또는 Pre-gated MoE 등에서 선정. (미정) |
| [TODO: io_uring] | J. Axboe, "Efficient IO with io_uring," 2019. — §2에서 사용 예정 |
| [TODO: GPUfs/GPUNet] | Silberstein+ ASPLOS'13 / Kim+ OSDI'14 — §7에서 사용 예정 |
| [TODO: GPUDirect Storage] | NVIDIA GPUDirect Storage 문서 — §7에서 사용 예정 |
