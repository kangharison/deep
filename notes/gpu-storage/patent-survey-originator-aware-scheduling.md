# 특허 조사: I/O 발원지(GPU/CPU) 인식 기반 NVMe 디바이스 내부 차별적 스케줄링

## 제안 특허 개요

**발명의 명칭**: I/O 요청 발원지 식별 기반 NVMe 스토리지 디바이스의 차별적 내부 스케줄링 방법 및 장치

**핵심 아이디어**: NVMe 커맨드에 발원지 유형(CPU/GPU/DPU) hint를 포함시키고, SSD 컨트롤러가 이를 인식하여 발원지별로 차별화된 내부 스케줄링(latency 보장, bandwidth 할당, FTL 데이터 배치)을 수행한다.

**제안 메커니즘**:
1. Create I/O SQ 커맨드의 reserved CDW12에 Originator Type + Latency Class 인코딩
2. 또는 per-command CDW13 Dataset Management 필드에 Originator Type 추가
3. 또는 새로운 NVMe Directive Type (Originator Directive) 정의
4. SSD 컨트롤러 내부에서 발원지별 독립 큐 + Weighted Fair Queue 스케줄링 + FTL 계층 차별 배치

---

## 조사 특허 목록 (30건)

### A. NVMe 내부 스케줄링 / QoS (11건)

#### A-1. US20090150894A1 — SSD QoS by Parallelizing Command Execution
- **출원인**: 개인 발명자
- **출원일**: 2007-12-10 | **상태**: Abandoned
- **핵심 주장**: 커맨드 파서가 수신 커맨드를 유형·LBA·QoS 요구사항으로 분류. Strict Priority(SP), Deficit Round Robin(DRR), Round Robin(RR) 세 가지 스케줄링 알고리즘을 파이프라인으로 조합하여 NVM 채널별 병렬 실행
- **메커니즘**: SP + DRR + RR 다중 스케줄러 파이프라인
- **링크**: https://patents.google.com/patent/US20090150894

#### A-2. US10564857B2 — QoS over NVMe Virtualization Using Adaptive Command Fetching
- **출원인**: Western Digital Technologies
- **등록일**: 2020-02-18
- **핵심 주장**: NVMe 가상화 환경(SR-IOV)에서 각 Submission Queue에 대역폭을 할당. SQ가 할당 대역폭을 초과하면 컨트롤러가 해당 SQ의 커맨드 fetch를 지연시킴
- **메커니즘**: 대역폭 인식 적응형 커맨드 fetch 스로틀링
- **링크**: https://patents.google.com/patent/US10564857B2/en

#### A-3. US10255217B2 — Two Level QoS Scheduling for Latency and Queue Depth Control
- **출원인**: Veritas (Symantec)
- **핵심 주장**: 2단계 스케줄링 — (1) 각 스토리지 서브시스템의 지속 가능 큐 깊이 결정 (2) 전체 시스템 용량 산출 후 크레딧 기반 입장 제어로 IO 요청 수락/거부
- **메커니즘**: 2계층 크레딧 기반 입장 제어 + 큐 깊이 강제
- **링크**: https://patents.google.com/patent/US10255217B2/en

#### A-4. US10019181B2 — Managing I/O Queues by NVMe Controller
- **출원인**: Samsung Electronics
- **등록일**: 2018-07-10
- **핵심 주장**: NVMe 컨트롤러의 SQ/CQ 관리 절차 — 커맨드 fetch 순서 중재(arbitration) 로직
- **메커니즘**: NVMe 컨트롤러 측 다중 SQ 중재 관리
- **링크**: https://patents.google.com/patent/US10019181B2/en

#### A-5. US20160034415A1 — QoS of Host Commands in Multi-Port Multi-Function PCIe Devices
- **출원인**: Samsung Electronics
- **공개일**: 2016-02-04
- **핵심 주장**: 멀티포트 PCIe NVMe 디바이스에서 physical/virtual function별로 커맨드를 수신하고, QoS 계산을 적용하여 디바이스 자원을 비례 할당
- **메커니즘**: per-function QoS 중재 + 자원 비례 할당
- **링크**: https://patents.google.com/patent/US20160034415

#### A-6. US20190278523A1 — Managed Fetching and Execution of Commands from Submission Queues
- **출원인**: SanDisk (Western Digital)
- **공개일**: 2019-09-12
- **핵심 주장**: 내부 블로킹 조건(자원 충돌, GC 등)으로 실행 불가능한 커맨드가 있을 때, 해당 SQ를 건너뛰고 실행 가능한 다른 SQ의 커맨드를 fetch → head-of-line blocking 방지
- **메커니즘**: 블로킹 인식 동적 커맨드 fetch 관리
- **링크**: https://patents.justia.com/patent/20190278523

#### A-7. US10430329B2 — QoS-Aware SCM/NAND Flash Hybrid SSD
- **출원인**: Western Digital Technologies
- **등록일**: 2019-10-01
- **핵심 주장**: 하이브리드 SSD(SCM + NAND)에서 호스트 hint에 따라 데이터 배치 계층 결정. latency 민감 IO → SCM, throughput 지향 IO → NAND
- **메커니즘**: 호스트 hint 기반 SCM/NAND 계층 차별 배치
- **링크**: https://patents.justia.com/patent/10430329

#### A-8. US10996985B2 — Dynamic Queue Depth Management with NVM Controllers
- **출원인**: Hewlett Packard Enterprise
- **등록일**: 2021-05-04
- **핵심 주장**: HW 큐에 매핑된 복수 SW 큐의 IO 카운트를 모니터링하여 큐 깊이를 동적 조정 — 실제 IO가 할당 깊이 이하이면 축소, 초과이면 확대
- **메커니즘**: 피드백 루프 기반 동적 큐 깊이 재분배
- **링크**: https://patents.google.com/patent/US10996985B2/en

#### A-9. US10628233B2 — Rack-Level Scheduling for Reducing Long Tail Latency
- **등록일**: 2020-04-21
- **핵심 주장**: 랙 수준에서 다수 노드의 SSD 접근을 조율하여, GC/wear leveling 등 백그라운드 작업이 동시에 발생하는 것을 방지 → tail latency 감소
- **메커니즘**: 크로스 노드 랙 레벨 SSD IO 스케줄링 조율
- **링크**: https://patents.google.com/patent/US10628233B2/en

#### A-10. US10761775B2 — NVMe Inter Command Association in SSD Storage
- **출원인**: Samsung Electronics
- **등록일**: 2020-09-01
- **핵심 주장**: 브리지 디바이스가 대형 호스트 커맨드를 복수 서브 커맨드로 분할하고, 연관 태그(ATag) + 크기(ASize)를 삽입. SSD 컨트롤러는 동일 그룹의 커맨드를 우선 실행
- **메커니즘**: 커맨드 연관 태깅을 통한 그룹 우선 실행
- **링크**: https://patents.google.com/patent/US10761775

#### A-11. US8314807B2 — Memory Controller with QoS-Aware Scheduling
- **출원인**: Apple
- **등록일**: 2012-11-20
- **핵심 주장**: 복수 소스로부터 메모리 연산을 수신, 각 소스가 QoS 파라미터를 부여. 컨트롤러가 QoS 파라미터를 스케줄링 결정에 포함
- **메커니즘**: 소스 태깅 QoS 인식 메모리 스케줄링
- **링크**: https://patents.google.com/patent/US8314807B2/en

---

### B. GPU-Storage 직접 IO (10건)

#### B-1. US20170147516A1 — GPU-Storage Direct DMA Interface
- **출원인**: Western Digital (HGST)
- **핵심 주장**: GPU와 스토리지 간 직접 DMA 인터페이스 구현, CPU bounce buffer 우회
- **메커니즘**: GPU-스토리지 직접 DMA 경로
- **링크**: https://patents.google.com/patent/US20170147516A1/en

#### B-2. US8996781B2 — Integrated Storage/Processing Devices for Big Data Analytics
- **출원인**: Kioxia (OCZ → Toshiba)
- **핵심 주장**: NVMe의 64K 큐를 GPU 코어별로 할당하여 병렬 IO 수행. GPGPU 컴퓨팅과 NVMe 스토리지를 통합
- **메커니즘**: GPU 코어별 NVMe 큐 할당 병렬 IO
- **링크**: https://patents.google.com/patent/US8996781

#### B-3. US11182694B2 — GPU-SSD P2P DMA with KV-SSD Interface
- **출원인**: Samsung Electronics
- **핵심 주장**: GPU-SSD 간 P2P DMA + KV-SSD 인터페이스 (NVMe vendor-specific command 활용)
- **메커니즘**: GPU-SSD P2P DMA + Key-Value 커맨드 확장
- **링크**: https://patents.justia.com/patent/11182694

#### B-4. US10678733B2 — Local PCIe Switch for Direct GPU-NVM Connection
- **출원인**: ATI/AMD
- **핵심 주장**: 로컬 PCIe 스위치를 통해 GPU에 NVM을 직접 연결, 루트 컴플렉스 우회
- **메커니즘**: PCIe 스위치 기반 GPU-NVM 직접 연결
- **링크**: https://patents.justia.com/patent/10678733

#### B-5. US11182309B2 — Fabric Attached Memory for GPU
- **출원인**: NVIDIA
- **핵심 주장**: GPU가 NVLink/패브릭을 통해 NVMe 메모리 풀에 접근
- **메커니즘**: 패브릭 기반 GPU-NVMe 메모리 풀 접근
- **링크**: https://patents.google.com/patent/US11182309B2/en

#### B-6. US11132828B2 — Non-Volatile Storage for Graphics Hardware
- **출원인**: Intel (Tahoe Research)
- **등록일**: 2021-09-28
- **핵심 주장**: GPU 전용 NVM 부착, 텍스처 등 로컬 캐시 역할
- **메커니즘**: GPU 로컬 NVM 캐시 계층
- **링크**: https://patents.google.com/patent/US11132828B2/en

#### B-7. US11119957B2 / US10585827B1 — PCIe Fabric P2P Communication
- **출원인**: Liqid
- **핵심 주장**: PCIe 패브릭으로 GPU↔NVMe 간 호스트 우회 P2P 전송
- **메커니즘**: PCIe 패브릭 P2P 직접 통신
- **링크**: https://patents.google.com/patent/US11119957B2/en

#### B-8. US8949486B1 — Direct Memory Access to Storage Devices
- **출원인**: Mellanox
- **핵심 주장**: 프록시 인터페이스를 통해 NIC/GPU↔SSD 간 CPU-free DMA 트랜잭션
- **메커니즘**: 프록시 기반 CPU-free DMA 인터페이스
- **링크**: https://patents.google.com/patent/US8949486

#### B-9. US20250045094A1 — GPU-Initiated Data Access of Scaled Storage (SCADA)
- **출원인**: NVIDIA
- **출원일**: 2023-08-03 | **공개일**: 2025-02-06
- **핵심 주장**: GPU(PPU)가 직접 스토리지 접근을 시작. 데이터 tier에 따라 GPU 직접 접근 vs 서버 인터페이스 경유로 분기. GPU가 제어 경로(control path)까지 직접 담당
- **메커니즘**: GPU 자체 NVMe 드라이버 + 멀티 tier 스토리지 접근
- **링크**: https://patents.justia.com/patent/20250045094

#### B-10. US20230032278A1 — Unified Virtual Memory for GPU-Storage
- **출원인**: NVIDIA
- **핵심 주장**: UVM(Unified Virtual Memory)으로 GPU-스토리지 간 투명한 메모리 접근 추상화
- **메커니즘**: UVM 기반 GPU-스토리지 투명 접근
- **링크**: https://patents.google.com/patent/US20230032278A1/en

---

### C. NVMe Hint / Directive / Metadata (10건)

#### C-1. US10884920B2 — Metadata-Based Operations for Solid State Devices
- **출원인**: Western Digital Technologies
- **등록일**: 2021-01-05
- **핵심 주장**: NVMe IO 커맨드의 metadata에 힌트(hot/cold, sequential/random)를 인코딩. 컨트롤러가 metadata를 파싱하여 hint 추출 → 실제 디바이스 동작과 상관관계 분석 → 상관 강하면 hint 기반 최적화 적용, 약하면 중단
- **메커니즘**: metadata 임베디드 hint + 상관관계 기반 신뢰도 검증
- **링크**: https://patents.justia.com/patent/10884920

#### C-2. US20220404999A1 — Persistent Host Metadata Logs in NVMe SSD
- **출원인**: Dell Products
- **공개일**: 2022-12-22
- **핵심 주장**: NVMe set/get 관리 커맨드로 호스트 메타데이터(시스템명, HW정보, OS, 드라이버 버전)를 SSD NAND에 영구 기록. 전원 사이클에도 유지
- **메커니즘**: NVMe vendor-specific 영역 영구 메타데이터 로깅
- **링크**: https://patents.justia.com/patent/20220404999

#### C-3. US20140244898A1 — I/O Hint Framework for Server Flash Cache
- **출원인**: VMware
- **공개일**: 2014-08-28
- **핵심 주장**: VM에서 하이퍼바이저 플래시 캐시 모듈로 IO hint 전달. SCSI CDB의 vendor-specific 바이트에 hint 페이로드(순차성, 접근 빈도, 목표 latency) 인코딩
- **메커니즘**: VM-to-hypervisor SCSI CDB 기반 IO hint 전달
- **링크**: https://patents.google.com/patent/US20140244898A1/en

#### C-4. US11960741B2 — Implied Streams
- **출원인**: Western Digital Technologies
- **등록일**: 2024-04-16
- **핵심 주장**: 호스트가 stream ID를 명시하지 않아도 SSD 컨트롤러가 LBA 분석으로 자동 스트림 감지. 기존 스트림에 속하면 해당 스트림에, 아니면 새 스트림 개설 또는 overflow 스트림에 배치
- **메커니즘**: 디바이스 측 자동 스트림 감지 (호스트 명시 불필요)
- **링크**: https://patents.google.com/patent/US11960741B2

#### C-5. US20160246726A1 — Adaptive HMB Caching Using Unassisted Hinting
- **출원인**: SanDisk (Western Digital)
- **공개일**: 2016-08-25
- **핵심 주장**: SSD가 호스트 IO 패턴과 파일시스템 메타데이터를 관찰하여 자율적으로 hint를 도출("unassisted hinting"). 도출된 hint로 Host Memory Buffer의 FTL 테이블 캐싱 최적화
- **메커니즘**: 디바이스 자율 hint 도출 + HMB FTL 캐싱
- **링크**: https://patents.google.com/patent/US20160246726A1

#### C-6. US8521948B2 — Handling Dynamic and Static Data for NVM System
- **출원인**: Apple
- **등록일**: 2013-08-27
- **핵심 주장**: NVM 인터페이스가 쓰기 데이터를 dynamic(hot) vs static(cold)으로 분류하여 별도 스트림 블록에 배치 → GC write amplification 감소
- **메커니즘**: hot/cold 이진 분류 + 별도 물리 블록 배치
- **링크**: https://patents.google.com/patent/US8521948B2/en

#### C-7. US20170371585A1 — Programmable Multi-Stream Data Write
- **출원인**: NGD Systems
- **공개일**: 2017-12-28
- **핵심 주장**: 프로그래머블 다중 스트림 — VM별, 데이터베이스별, 작업별로 stream ID 할당. VM 종료 시 해당 stream의 erase unit만 삭제 → GC 회피
- **메커니즘**: 프로그래머블 workload-to-stream ID 매핑
- **링크**: https://patents.google.com/patent/US20170371585A1/en

#### C-8. US11048411B2 — Consolidating Data Streams for Multi-Stream SSDs
- **출원인**: Samsung Electronics
- **등록일**: 2021-06-29
- **핵심 주장**: 호스트 SW 스트림 수가 디바이스 HW 스트림 수를 초과할 때, 통계 수집 → 랭킹 → N개 SW 스트림을 M개 HW 스트림으로 통합 매핑(N > M)
- **메커니즘**: 통계 기반 다대소(many-to-few) 스트림 통합
- **링크**: https://patents.google.com/patent/US11048411B2/en

#### C-9. US12340113B2 — Host Controlled Garbage Collection in SSD
- **출원인**: SK hynix NAND Product Solutions
- **등록일**: ~2024-2025
- **핵심 주장**: 호스트가 SSD의 GC를 직접 제어 — GC 시작/중지 시점, 참여 NAND die 수 결정. latency 민감 read 중 GC 간섭 방지
- **메커니즘**: 호스트 주도 GC 스케줄링으로 deterministic read latency
- **링크**: https://www.storagenewsletter.com/2025/07/30/sk-hynix-nand-product-solutions-assigned-eighteen-patents/

#### C-10. US20210182192A1 — Storage Device with Enhanced Time to Ready Performance
- **출원인**: Seagate Technology
- **공개일**: 2021-06-17
- **핵심 주장**: FTL 메타데이터를 "client interest" 기준으로 우선순위 지정. 전원 복구 시 고관심 세그먼트 먼저 로드 후 "operationally ready" 알림. NVMe IO Determinism 모드 지원
- **메커니즘**: 관심도 기반 FTL 메타데이터 우선 복구 + IOD
- **링크**: https://patents.justia.com/patent/20210182192

---

### D. 이종 프로세서 IO / 차별화 서비스 (9건)

#### D-1. US9104482B2 — Differentiated Storage QoS
- **출원인**: Hewlett Packard
- **등록일**: 2015-08-11
- **핵심 주장**: 다수 호스트 애플리케이션을 소수 IO 클래스로 집계. QoS 컨트롤러가 per-app/per-class IO 통계를 수집하고, 합산 동시성이 임계치 초과 시 클래스별 비례 축소
- **메커니즘**: 클래스 레벨 적응형 동시성 제어
- **링크**: https://patents.google.com/patent/US9104482

#### D-2. US10884975B2 — Differentiated Storage Services in Ethernet SSD
- **출원인**: Samsung Electronics
- **등록일**: 2021-01-05
- **핵심 주장**: Ethernet SSD가 MPLS 네트워크 경유 IO 요청을 수신, Label Switched Path에 매칭하여 per-flow QoS 보장. RDMA, RoCE, NVMe-oF-TCP 지원
- **메커니즘**: MPLS LSP 매칭 기반 per-flow QoS 차별화
- **링크**: https://patents.google.com/patent/US10884975B2/en

#### D-3. US20190065073A1 — I/O Performance in Heterogeneous Storage Environments
- **출원인**: IBM
- **공개일**: 2019-02-28
- **핵심 주장**: 이종 스토리지(HDD, SSD, NVMe) 환경에서 블록 IO 스트림을 디바이스 유형 클래스로 분리, 해당 클래스의 물리 SAS 레인으로 라우팅
- **메커니즘**: 디바이스 유형별 IO 스트림 분류 + 물리 레인 라우팅
- **링크**: https://patents.google.com/patent/US20190065073A1/en

#### D-4. US8417911B2 — Associating I/O Device Requests with Memory via PCIe Requester ID
- **출원인**: IBM
- **등록일**: 2013-04-09
- **핵심 주장**: PCIe Requester ID(Bus/Device/Function)로 요청 소스를 식별. SR-IOV virtual function vs 표준 PCIe 디바이스 구분. RID를 사용하여 어느 LPAR이 해당 function을 소유하는지 결정
- **메커니즘**: PCIe Requester ID 기반 요청 소스 식별 + SR-IOV 인식
- **링크**: https://patents.google.com/patent/US8417911B2/en

#### D-5. US20130124805A1 — Servicing Latency-Sensitive Memory Requests (CPU vs GPU)
- **출원인**: AMD
- **공개일**: 2013-05-16
- **핵심 주장**: CPU/GPU 공유 메모리 컨트롤러에서 CPU 요청 = latency 민감(고우선), GPU 요청 = bandwidth 민감(저우선)으로 분류. boost 로직이 latency 민감 커맨드를 식별하여 우선순위 상향. age 기반 타이브레이킹
- **메커니즘**: CPU(latency) vs GPU(bandwidth) 우선순위 분류 + 동적 boost
- **링크**: https://patents.google.com/patent/US20130124805

#### D-6. US20200012437A1 — Memory System with Latency Distribution Optimization
- **출원인**: SK Hynix Memory Solutions
- **공개일**: 2020-01-09
- **핵심 주장**: SSD IO 전용 CPU 코어를 호스트 코어와 격리. 워크로드 스레드와 인터럽트 핸들러를 격리 실행하여 OS 스케줄링 간섭 제거 → latency 분산 감소
- **메커니즘**: CPU 코어 격리 + 인터럽트 어피니티로 latency 지터 최소화
- **링크**: https://patents.google.com/patent/US20200012437A1/en

#### D-7. US20220214987A1 — GPU Peer-to-Peer Arrangements
- **출원인**: NVIDIA
- **공개일**: 2022-07-07
- **핵심 주장**: 합성(synthetic) 디바이스를 생성하여 GPU 간 또는 GPU-NVMe 간 크로스 주소 도메인 P2P DMA 활성화
- **메커니즘**: 합성 디바이스 추상화 기반 P2P DMA
- **링크**: https://patents.google.com/patent/US20220214987A1/en

#### D-8. US11550504B2 — GPU-Storage Direct Access
- **출원인**: Samsung Electronics
- **등록일**: 2023-01-10
- **핵심 주장**: GPU가 호스트 CPU 메모리를 거치지 않고 직접 SSD에 read/write 수행. ML/빅데이터 워크로드 타겟
- **메커니즘**: GPU 직접 스토리지 접근 (CPU 메모리 스테이징 우회)
- **링크**: https://patents.google.com/patent/US11550504

#### D-9. US20210263677A1 — NVMe Queue Management Multi-Tier Storage Systems
- **출원인**: IBM
- **공개일**: 2021-08-26
- **핵심 주장**: 멀티 tier 스토리지에서 extent별 워크로드 특성에 따라 CPU 코어-to-NVMe IOQ 매핑을 동적 재조정
- **메커니즘**: 워크로드 기반 동적 코어-IOQ 재매핑
- **링크**: https://patents.google.com/patent/US20210263677A1/en

---

## 제안 특허와의 차별점 분석

### 종합 비교표

| # | 특허 | 출원인 | IO 소스 인식 | GPU/CPU 구분 | 디바이스 내부 스케줄링 | Latency 보장 | 본 발명과의 차이 |
|---|------|--------|:----------:|:----------:|:------------------:|:----------:|----------------|
| A-1 | US20090150894 | 개인 | X | X | O | △ | 커맨드 유형별 분류만 있고 발원지(GPU/CPU) 인식 없음 |
| A-2 | US10564857 | WDC | X | X | O | O | VM별 대역폭 제어이지 발원지 유형 인식 아님 |
| A-3 | US10255217 | Veritas | X | X | O | O | 호스트 측 스케줄링. 디바이스 내부가 아님 |
| A-4 | US10019181 | Samsung | X | X | O | X | 범용 NVMe 큐 중재. 발원지 무관 |
| A-5 | US20160034415 | Samsung | △ | X | O | O | PCIe function별 QoS. GPU/CPU 유형 인식은 없음 |
| A-6 | US20190278523 | SanDisk | X | X | O | △ | 블로킹 회피 fetch 최적화. 발원지 무관 |
| A-7 | US10430329 | WDC | O | X | △ | O | 호스트 hint로 SCM/NAND 배치. **GPU/CPU 구분 없음** |
| A-8 | US10996985 | HPE | X | X | O | △ | 큐 깊이 동적 조정. 발원지 무관 |
| A-9 | US10628233 | - | X | X | X | O | 랙 레벨 조율. 디바이스 내부 아님 |
| A-10 | US10761775 | Samsung | X | X | O | △ | 커맨드 그룹 연관 태깅. 발원지 무관 |
| A-11 | US8314807 | Apple | O | X | O | O | **소스 태깅 QoS 스케줄링. 가장 유사하나 메모리 컨트롤러 대상이며 NVMe/SSD가 아님. GPU/CPU 명시적 구분 없음** |
| B-1 | US20170147516 | WDC | X | △ | X | X | GPU-스토리지 DMA 경로만. 디바이스 내부 스케줄링 없음 |
| B-2 | US8996781 | Kioxia | X | O | X | X | GPU 코어별 큐 할당. 디바이스 측 스케줄링 차별화 없음 |
| B-3 | US11182694 | Samsung | X | O | X | X | GPU-SSD P2P DMA. 디바이스 내부 최적화 없음 |
| B-4 | US10678733 | AMD | X | O | X | X | PCIe 토폴로지 최적화. 디바이스 내부 무관 |
| B-5 | US11182309 | NVIDIA | X | O | X | X | 패브릭 메모리 접근. 디바이스 스케줄링 무관 |
| B-6 | US11132828 | Intel | X | O | X | X | GPU 전용 NVM 캐시. NVMe 프로토콜 차별화 없음 |
| B-7 | US11119957 | Liqid | X | △ | X | X | P2P 패브릭 통신. 디바이스 내부 무관 |
| B-8 | US8949486 | Mellanox | X | X | X | X | 프록시 DMA. 디바이스 스케줄링 무관 |
| B-9 | US20250045094 | NVIDIA | △ | O | X | X | **GPU 측 제어 경로. 디바이스 내부 스케줄링 최적화 없음** |
| B-10 | US20230032278 | NVIDIA | X | O | X | X | UVM 추상화. 디바이스 측 무관 |
| C-1 | US10884920 | WDC | O | X | △ | X | **범용 metadata hint. 발원지 유형 특화 아님. 스케줄링이 아닌 데이터 배치 중심** |
| C-2 | US20220404999 | Dell | O | X | X | X | 호스트 환경 메타데이터 로깅. 실시간 스케줄링 아님 |
| C-3 | US20140244898 | VMware | O | X | X | △ | VM-하이퍼바이저 hint. NVMe 디바이스 내부 아님 |
| C-4 | US11960741 | WDC | X | X | X | X | 자동 스트림 감지. 발원지 무관, 데이터 배치 목적 |
| C-5 | US20160246726 | SanDisk | X | X | X | X | 자율 hint 도출. 발원지 무관, FTL 캐싱 목적 |
| C-6 | US8521948 | Apple | X | X | X | X | hot/cold 분류. 발원지 무관 |
| C-7 | US20170371585 | NGD | O | X | X | X | workload별 stream. 발원지 유형이 아닌 애플리케이션별 |
| C-8 | US11048411 | Samsung | X | X | O | X | 스트림 통합 매핑. 발원지 무관 |
| C-9 | US12340113 | SK hynix | X | X | X | O | 호스트 주도 GC. 디바이스 자율 스케줄링 아님 |
| C-10 | US20210182192 | Seagate | X | X | X | △ | FTL 복구 최적화. IO 스케줄링 아님 |
| D-1 | US9104482 | HP | O | X | O | O | 애플리케이션별 IO 클래스. GPU/CPU 유형 구분 없음 |
| D-2 | US10884975 | Samsung | O | X | O | O | 네트워크(MPLS) 레벨 QoS. NVMe 프로토콜 레벨 아님 |
| D-3 | US20190065073 | IBM | O | X | X | X | 디바이스 유형별 라우팅(HDD/SSD). 프로세서 유형 아님 |
| D-4 | US8417911 | IBM | O | X | X | X | **PCIe RID로 소스 식별. 가장 관련 높으나 LPAR 매핑 목적이며 스케줄링 차별화 없음** |
| D-5 | US20130124805 | AMD | O | **O** | O | O | **가장 유사! CPU(latency) vs GPU(bandwidth) 분류 + 우선순위 스케줄링. 그러나 DRAM 메모리 컨트롤러 대상이며, NVMe SSD/Flash 특성(GC, SLC버퍼, FTL) 활용 없음** |
| D-6 | US20200012437 | SK Hynix | X | X | X | O | CPU 코어 격리. 디바이스 내부 아님 |
| D-7 | US20220214987 | NVIDIA | X | O | X | X | P2P DMA 추상화. 스케줄링 무관 |
| D-8 | US11550504 | Samsung | X | O | X | X | GPU 직접 접근. 디바이스 내부 최적화 없음 |
| D-9 | US20210263677 | IBM | X | X | O | X | 코어-IOQ 동적 재매핑. 발원지 유형 무관 |

---

## 핵심 차별점 요약

### 기존 특허들의 공백 (Gap)

```
기존 특허 영역:

  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
  │ 호스트 측        │     │ 데이터 경로      │     │ 디바이스 내부    │
  │ GPU-Storage      │     │ hint/metadata    │     │ 스케줄링/QoS     │
  │ 직접 IO 경로     │     │ 전달 메커니즘    │     │ 알고리즘         │
  │                  │     │                  │     │                  │
  │ B-1~B-10         │     │ C-1~C-10         │     │ A-1~A-11         │
  │ (NVIDIA, Samsung,│     │ (WDC, Dell,      │     │ (Samsung, WDC,   │
  │  AMD, Intel)     │     │  VMware, Apple)  │     │  Apple, HPE)     │
  └────────┬─────────┘     └────────┬─────────┘     └────────┬─────────┘
           │                        │                         │
           │   GPU가 IO를 보내는    │   범용 hint를 전달하는  │  IO를 공정하게
           │   물리적 경로 제공     │   프레임워크 제공       │  스케줄링하는 방법
           │                        │                         │
           └────────────────────────┴─────────────────────────┘
                                    │
                          ┌─────────▼──────────┐
                          │     *** GAP ***     │
                          │                     │
                          │ "GPU/CPU 발원지를   │
                          │  NVMe 프로토콜로    │
                          │  전달하고,          │
                          │  SSD 컨트롤러가     │
                          │  이를 인식하여      │
                          │  Flash 특성 활용한  │
                          │  차별적 스케줄링"   │
                          │                     │
                          │ = 본 발명의 영역    │
                          └─────────────────────┘
```

### 가장 유사한 선행 특허 3건과의 상세 차이

#### 1. US20130124805 (AMD) — CPU vs GPU 메모리 스케줄링
| 항목 | AMD 특허 | 본 발명 |
|------|---------|---------|
| 대상 디바이스 | DRAM 메모리 컨트롤러 | NVMe SSD (Flash) |
| 프로토콜 | 메모리 버스 (DDR) | NVMe over PCIe |
| hint 전달 방식 | HW 신호 (메모리 포트) | NVMe 커맨드 필드 (CDW12/CDW13) |
| 스케줄링 목표 | DRAM bank conflict 최소화 | Flash GC 간섭 회피 + SLC 버퍼 차별 할당 |
| 데이터 배치 | 해당 없음 (DRAM 균일) | SLC/TLC/QLC 계층별 차별 배치 |
| latency 스케일 | ~100ns | ~10-100μs (1000배 차이) |

#### 2. US10884920 (WDC) — Metadata 기반 SSD 최적화
| 항목 | WDC 특허 | 본 발명 |
|------|---------|---------|
| hint 내용 | hot/cold, sequential/random (범용) | 발원지 유형 (CPU/GPU/DPU) + latency class |
| hint 목적 | 데이터 배치 최적화 (GC 효율) | 스케줄링 차별화 + latency SLA 보장 |
| 스케줄링 변경 | 없음 | 발원지별 WFQ + preemptive 스케줄링 |
| Queue 레벨 인식 | 없음 | Create SQ 시 발원지 등록 → 큐 단위 정책 |

#### 3. US8314807 (Apple) — 소스 태깅 QoS 메모리 스케줄링
| 항목 | Apple 특허 | 본 발명 |
|------|-----------|---------|
| 대상 | DRAM/SRAM 메모리 컨트롤러 | NVMe SSD |
| 소스 구분 | 범용 소스 ID + QoS 파라미터 | GPU/CPU/DPU 유형 + latency class |
| Flash 특성 활용 | 없음 | SLC 버퍼 우선 할당, GC 간섭 회피, FTL 최적화 |
| 프로토콜 확장 | 없음 (HW 시그널) | NVMe Create SQ / Directive / CDW13 확장 |

### 본 발명의 6가지 신규성 요소

1. **NVMe 프로토콜 레벨의 발원지 유형 인코딩**: Create I/O SQ의 reserved CDW에 Originator Type을 정의하는 최초의 제안
2. **GPU/CPU/DPU 명시적 구분**: 기존 특허들은 "소스"를 VM, 애플리케이션, PCIe function으로 구분하지만 프로세서 유형(GPU/CPU/DPU)으로 구분하는 것은 없음
3. **SSD 컨트롤러 내부의 발원지 인식 스케줄링**: 호스트 측이 아닌 디바이스 측에서 발원지 유형을 인식하여 내부 IO 스케줄링을 차별화
4. **Flash 특성 활용 차별 배치**: GPU IO → SLC 버퍼 우선 (빠른 응답), CPU IO → TLC/QLC 직접 (용량 효율) — Flash 매체 특성과 발원지 유형의 결합
5. **발원지별 Latency SLA 보장**: 단순 우선순위가 아닌 Target Maximum Latency를 커맨드에 인코딩하여 디바이스가 SLA를 보장
6. **GC 간섭 차별 관리**: GPU IO 처리 중에는 GC를 억제/지연하고, CPU throughput IO 중에는 GC를 허용하는 적응형 GC 정책
