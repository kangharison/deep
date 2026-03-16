# GPU-Storage Direct I/O 관련 US 특허 조사

조사일: 2026-03-16

GPU-initiated storage I/O, GPU direct storage access, GPU-NVMe 상호작용과 관련된 미국 특허를 조사한 결과를 정리한다.

---

## Patent 1: US20170147516A1 — Direct Interface Between GPU and Data Storage Unit

| 항목 | 내용 |
|------|------|
| **특허번호** | US 2017/0147516 A1 (출원공개) |
| **제목** | Direct Interface Between Graphics Processing Unit and Data Storage Unit |
| **출원인(Assignee)** | HGST Netherlands B.V. → Western Digital Technologies, Inc. (2020년 양도) |
| **출원일** | 2016-01-12 (우선권: US Provisional 62/257,510, 2015-11-19) |
| **공개일** | 2017-05-25 |
| **핵심 청구항** | GPU와 스토리지 유닛 간 호스트 프로세서/시스템 메모리를 거치지 않는 직접 통신 경로를 확립. 스토리지의 DMA 컨트롤러가 GPU 메모리 내 데이터 버퍼 위치를 저장하여 향후 GPU와의 정보 교환에 사용. |
| **핵심 메커니즘** | 데이터 버스를 통해 GPU와 스토리지 유닛이 직접 연결되며, 호스트 CPU의 중재 없이 GPU 메모리 버퍼 ↔ 스토리지 간 DMA 전송 수행. CPU bounce buffer를 완전히 우회. |

---

## Patent 2: US 8,996,781 B2 — Integrated Storage/Processing with NVMe GPU Queues

| 항목 | 내용 |
|------|------|
| **특허번호** | US 8,996,781 B2 |
| **제목** | Integrated Storage/Processing Devices, Systems and Methods for Performing Big Data Analytics |
| **출원인(Assignee)** | OCZ Technology Group → Toshiba Corporation → Toshiba Memory Corporation → Kioxia Corporation (2021년 합병) |
| **출원일** | 2013 |
| **등록일** | 2015-03-31 |
| **핵심 청구항** | NVM Express 표준을 구현하는 메모리 컨트롤러를 포함하는 통합 스토리지/프로세싱 디바이스. NVMe가 지원하는 최대 64K개 큐를 활용하여 각 GPU 코어에 하나 이상의 전용 큐를 할당, 진정한 병렬 처리 달성. |
| **핵심 메커니즘** | PCIe 채널을 통한 SSD 접근에 NVMe 스펙을 사용하되, GPU 코어별 전용 NVMe 큐를 할당하여 GPU 코어가 독립적으로 스토리지에 병렬 I/O 수행. |

---

## Patent 3: US 11,182,694 B2 — GPU-SSD P2P DMA with Key-Value Interface

| 항목 | 내용 |
|------|------|
| **특허번호** | US 11,182,694 B2 |
| **제목** | Data Path for GPU Machine Learning Training with Key Value SSD |
| **출원인(Assignee)** | Samsung Electronics Co., Ltd. (수원시) |
| **출원일** | 2018-03-30 |
| **등록일** | 2021-11-23 |
| **발명자** | Joo Hwan Lee (San Jose, CA), Yang Seok Ki (Palo Alto, CA) |
| **핵심 청구항** | GPU 메모리의 I/O 영역에 key-value 요청 큐를 배치하고, 온보드 key-value SSD가 이 큐에서 요청을 읽어 결과값을 GPU 메모리에 직접 기록. GPU가 P2P DMA에 대한 완전한 제어권을 가짐. |
| **핵심 메커니즘** | 글로벌 PCIe 버스를 거치지 않고 GPU와 온보드 KV-SSD 간 peer-to-peer DMA 수행. KV 커맨드는 NVMe vendor-specific command로 구현되어 SSD 펌웨어와 GPU 드라이버 소프트웨어에서 정의. ML 학습 워크로드의 데이터 로딩 오버헤드 감소. |

---

## Patent 4: US 10,678,733 B2 — Connecting NVM Locally to GPU via Local PCIe Switch

| 항목 | 내용 |
|------|------|
| **특허번호** | US 10,678,733 B2 |
| **제목** | Apparatus for Connecting Non-Volatile Memory Locally to a GPU Through a Local Switch |
| **출원인(Assignee)** | ATI Technologies ULC (Markham, Ontario) — AMD 자회사 |
| **출원일** | 2019-04-10 (원출원: 2016-12-23, US 15/389,747의 연속출원) |
| **등록일** | 2020-06-09 |
| **발명자** | Nima Osqueizadeh (Markham) |
| **핵심 청구항** | 로컬 PCIe 스위치를 통해 비휘발성 메모리(NVM)를 GPU에 직접 연결하는 장치. NVM 컨트롤러가 로컬 PCIe 스위치를 통해 직접 데이터 전송을 개시할 수 있음. 호스트 프로세서의 루트 컴플렉스를 우회. |
| **핵심 메커니즘** | GPU 보드 상에 로컬 PCIe 스위치를 배치하여 GPU와 NVM 간 P2P 연결 확립. 메인 PCIe 스위치(루트 컴플렉스)를 거치지 않으므로 레이턴시 감소 및 대역폭 향상. GPU가 NVM을 로컬 스토리지처럼 직접 접근. |

---

## Patent 5: US 11,182,309 B2 — Fabric Attached Memory for GPUs via NVMe

| 항목 | 내용 |
|------|------|
| **특허번호** | US 11,182,309 B2 |
| **제목** | Techniques for an Efficient Fabric Attached Memory |
| **출원인(Assignee)** | NVIDIA Corporation |
| **등록일** | 2021-11-23 |
| **발명자** | Glenn Dearth, Samuel H. Duncan, John Feehrer, Denis Foley, Ram Gummadi, Mark Hummel, Brian Kelleher, Vyas Venkataraman |
| **핵심 청구항** | 인터커넥트 패브릭 스위치의 라우팅 테이블을 통해 메모리 접근 요청 내 주소 압축을 선택적으로 수행하여, 소스 GPU와 패브릭 연결 메모리 간의 주소 공간 차이를 보상. |
| **핵심 메커니즘** | Fabric Attached Memory(FAM) 풀을 하나 이상의 GPU가 네트워크 패브릭(NVLink 등)을 통해 접근. 각 FAM 디바이스는 컨트롤러(축소된 GPU 또는 커스텀 ASIC)와 DDR/GDDR/HBM/NVMe 등 다양한 메모리를 번들로 포함. 불완전한 프로세서를 메모리 컨트롤러로 재활용하여 다른 프로세서가 FAM으로 접근 가능. |

---

## Patent 6: US 11,132,828 B2 — Non-Volatile Storage for Graphics Hardware

| 항목 | 내용 |
|------|------|
| **특허번호** | US 11,132,828 B2 |
| **제목** | Non-Volatile Storage for Graphics Hardware |
| **출원인(Assignee)** | Intel Corporation → Tahoe Research, Ltd. (2022년 양도) |
| **출원일** | 2020-02-18 (원출원: 2010-05-29, 다수 연속출원) |
| **등록일** | 2021-09-28 |
| **발명자** | Adam W. Herr, Adam T. Lake, Ryan T. Tabrah |
| **핵심 청구항** | GPU 전용 비휘발성 메모리를 포함하는 장치에서, 콘텐츠 요청 감지 시 GPU 전용 NVM에 해당 콘텐츠가 존재하는지 판단. 존재하면 호스트 PCIe 인터페이스를 거치지 않고 GPU가 직접 NVM에서 읽음. |
| **핵심 메커니즘** | GPU에 전용 SSD(NVM)를 부착하고, 텍스처 데이터 등을 호스트 CPU로부터 volatile 메모리를 거쳐 수신한 후 GPU 카드 내 SSD에 영구 저장. 이후 재사용 시 호스트 없이 GPU-local NVM에서 직접 로드. 캐시 계층 역할. |

---

## Patent 7: US 11,119,957 B2 / US 10,585,827 B1 — PCIe P2P Communications for GPU-Storage

| 항목 | 내용 |
|------|------|
| **특허번호** | US 11,119,957 B2 (연속출원: US 10,585,827 B1) |
| **제목** | PCIe Device Peer-to-Peer Communications / PCIe Fabric Enabled Peer-to-Peer Communications |
| **출원인(Assignee)** | Liqid Inc. |
| **출원일** | 2020-03-06 (원출원: 2019-02-05) |
| **등록일** | 2021-09-14 |
| **발명자** | Sumit Puri, James Scott Cannata, Christopher R. Long, Bryan Schramm |
| **핵심 청구항** | 제1 PCIe 디바이스에서 제2 PCIe 디바이스의 주소 범위에 해당하는 전송을 감지하여, 호스트 프로세서를 거치지 않고 PCIe 패브릭을 통해 제2 디바이스로 직접 리다이렉트. |
| **핵심 메커니즘** | PCIe 스위치 패브릭 위에서 관리 프로세서가 P2P 배치를 설정. GPU→NVMe SSD 또는 NVMe SSD→GPU 간 데이터 전송이 호스트 CPU를 우회하여 PCIe 패브릭 내에서 직접 수행. Composable/disaggregated 인프라에 적합. |

---

## Patent 8: US 8,949,486 B1 — Direct Memory Access to Storage Devices

| 항목 | 내용 |
|------|------|
| **특허번호** | US 8,949,486 B1 |
| **제목** | Direct Memory Access to Storage Devices |
| **출원인(Assignee)** | Mellanox Technologies Ltd. |
| **출원일** | 2013-07-17 |
| **등록일** | 2015-02-03 |
| **핵심 청구항** | I/O 디바이스(NIC/GPU)가 개시한 제1 DMA 트랜잭션과 스토리지 드라이브가 개시한 제2 DMA 트랜잭션 사이에 인터페이스 디바이스를 배치. 제1 DMA에 응답하여 제2 DMA를 자동 호출하여 CPU/호스트 메모리 개입 없이 NIC↔SSD 직접 전송 수행. |
| **핵심 메커니즘** | 인터페이스 디바이스 내 버퍼 메모리와 제어 로직이 프록시 역할 수행. GPU/NIC에서 온 DMA 요청을 감지하면 스토리지 측 DMA를 자동 트리거하여, 데이터가 호스트 메모리를 거치지 않고 I/O 디바이스 ↔ 스토리지 간 직접 이동. RDMA + NVMe 조합의 원형. |

---

## Patent 9: US 2025/0045094 A1 — GPU-Initiated Data Access of Scaled Storage (SCADA)

| 항목 | 내용 |
|------|------|
| **특허번호** | US 2025/0045094 A1 (출원공개) |
| **제목** | GPU-Initiated Data Access of Scaled Storage |
| **출원인(Assignee)** | NVIDIA Corporation (추정, SCADA 기술의 원천) |
| **우선권일** | 2023-08-03 (US Provisional 63/530,654) |
| **공개일** | 2025-02-06 |
| **핵심 청구항** | 하나 이상의 병렬 처리 유닛(GPU)이 데이터 접근을 직접 개시(initiate). 데이터 위치가 GPU가 직접 접근 가능한 제1 티어에 있으면 GPU가 직접 읽기/쓰기 수행. 제2 티어(원격 스토리지 등)에 있으면 서버 인터페이스를 통해 접근. API 기반 데이터 접근 요청. |
| **핵심 메커니즘** | SCADA(Scaled Accelerated Data Access) 아키텍처. GPU가 스토리지 I/O의 제어 경로(control path)까지 직접 담당. 기존 GPUDirect Storage가 데이터 경로만 CPU에서 분리한 것과 달리, SCADA는 제어 경로도 GPU로 이관. 다중 티어 스토리지 계층(로컬 NVMe, 원격 NVMe-oF 등)에 대한 GPU-initiated 접근을 클라이언트-서버 구조로 구현. |

---

## Patent 10: US 2023/0032278 A1 — Unified Virtual Memory Management for GPU-Storage

| 항목 | 내용 |
|------|------|
| **특허번호** | US 2023/0032278 A1 (출원공개) |
| **제목** | Unified Virtual Memory Management in Heterogeneous Computing Systems |
| **출원인(Assignee)** | NVIDIA Corporation |
| **공개일** | 2023-02-02 |
| **핵심 청구항** | 이기종 컴퓨팅 플랫폼(CPU+GPU)에서 통합 가상 메모리(UVM) 드라이버를 통해 GPU 메모리 압력 발생 시 자동으로 CPU 메모리로 eviction 수행. GPU 메모리 오버서브스크립션 지원. 프로그래머가 각 디바이스별 메모리 관리를 수동으로 작성할 필요 제거. |
| **핵심 메커니즘** | UVM 시스템이 CPU와 GPU 간 메모리 페이지 마이그레이션을 자동 관리. GPU가 스토리지/호스트 메모리의 데이터에 투명하게 접근할 수 있는 추상화 계층 제공. GPU-initiated storage access의 소프트웨어 기반(memory-mapped) 접근 방식. |

---

## 특허 분류 요약

| 접근 방식 | 관련 특허 |
|-----------|-----------|
| **GPU-Storage 직접 DMA 경로** | US20170147516A1 (Western Digital), US8949486B1 (Mellanox) |
| **GPU 온보드/로컬 NVM 부착** | US10678733B2 (ATI/AMD), US11132828B2 (Intel) |
| **NVMe 큐 GPU 코어 할당** | US8996781B2 (Kioxia) |
| **P2P PCIe 패브릭** | US11119957B2 / US10585827B1 (Liqid) |
| **GPU-SSD P2P + KV인터페이스** | US11182694B2 (Samsung) |
| **패브릭 연결 메모리** | US11182309B2 (NVIDIA) |
| **GPU-initiated 제어 경로 (SCADA)** | US20250045094A1 (NVIDIA) |
| **통합 가상 메모리** | US20230032278A1 (NVIDIA) |

---

## 연구 관련성

이들 특허는 GPU-Storage I/O 연구의 핵심 기술적 기반을 보여준다:

1. **데이터 경로 최적화**: CPU bounce buffer 제거 → GPU 메모리와 NVMe SSD 간 직접 DMA (Western Digital, Mellanox, Liqid 특허)
2. **로컬 스토리지 부착**: GPU 보드에 NVM을 직접 탑재하여 PCIe 루트 컴플렉스 우회 (AMD, Intel 특허)
3. **NVMe 큐 병렬화**: GPU 코어별 전용 NVMe submission/completion 큐 할당 (Kioxia 특허) — BaM 논문의 핵심 아이디어와 직결
4. **KV-SSD 통합**: ML 워크로드를 위한 GPU-SSD P2P DMA + key-value 인터페이스 (Samsung 특허)
5. **제어 경로까지 GPU 이관**: 데이터 경로뿐 아니라 I/O 제어 자체를 GPU가 수행하는 SCADA 아키텍처 (NVIDIA 최신 특허)
