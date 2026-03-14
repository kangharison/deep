# GPGPU-Sim 인터커넥트 네트워크 심층 분석

## 1. GPU 내부 인터커넥트 아키텍처 개요

GPU 내부에서 SM(Streaming Multiprocessor)과 Memory Partition(L2 캐시 + DRAM 컨트롤러) 사이의 데이터 이동은 인터커넥트 네트워크를 통해 이루어진다.
GPGPU-Sim은 두 가지 인터커넥트 모델을 지원한다: BookSim2 기반의 상세 NoC 시뮬레이션(INTERSIM)과 단순화된 크로스바(LOCAL_XBAR).

```
                        GPU 인터커넥트 아키텍처 (Dual Subnet)
 ┌─────────────────────────────────────────────────────────────────────┐
 │                                                                     │
 │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐        ┌──────┐ ┌──────┐     │
 │  │ SM 0 │ │ SM 1 │ │ SM 2 │ │ SM 3 │  ...   │SM N-2│ │SM N-1│     │
 │  └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘        └──┬───┘ └──┬───┘     │
 │     │        │        │        │                │        │          │
 │  ═══╪════════╪════════╪════════╪════════════════╪════════╪═══════   │
 │  ║  │  Subnet 0: Request Network (SM → Memory)  │       │      ║   │
 │  ║  │  READ_REQUEST, WRITE_REQUEST               │       │      ║   │
 │  ═══╪════════╪════════╪════════╪════════════════╪════════╪═══════   │
 │     │        │        │        │                │        │          │
 │  ═══╪════════╪════════╪════════╪════════════════╪════════╪═══════   │
 │  ║  │  Subnet 1: Reply Network (Memory → SM)    │       │      ║   │
 │  ║  │  READ_REPLY, WRITE_REPLY                   │       │      ║   │
 │  ═══╪════════╪════════╪════════╪════════════════╪════════╪═══════   │
 │     │        │        │        │                │        │          │
 │  ┌──┴───┐ ┌──┴───┐ ┌──┴───┐ ┌──┴───┐        ┌──┴───┐ ┌──┴───┐    │
 │  │Mem P0│ │Mem P1│ │Mem P2│ │Mem P3│  ...   │Mem Pm│ │Mem Pm│    │
 │  │L2+MC │ │L2+MC │ │L2+MC │ │L2+MC │        │L2+MC │ │L2+MC │    │
 │  └──────┘ └──────┘ └──────┘ └──────┘        └──────┘ └──────┘     │
 └─────────────────────────────────────────────────────────────────────┘

  SM = Streaming Multiprocessor
  Mem P = Memory Partition (L2 Cache Slice + DRAM Controller)
  MC = Memory Controller
```

### 듀얼 서브넷 구조

GPGPU-Sim의 인터커넥트는 기본적으로 2개의 독립된 서브네트워크로 구성된다.

| 서브넷 | 방향 | 트래픽 유형 | 용도 |
|--------|------|-------------|------|
| Subnet 0 (REQ_NET) | SM → Memory | READ_REQUEST, WRITE_REQUEST | 메모리 요청 전송 |
| Subnet 1 (REPLY_NET) | Memory → SM | READ_REPLY, WRITE_REPLY | 메모리 응답 반환 |

이 분리 설계는 request-reply 데드락을 방지하고, 각 방향의 트래픽을 독립적으로 관리할 수 있게 한다.

---

## 2. 인터커넥트 모드 선택과 icnt_wrapper

### 2.1 함수 포인터 기반 추상화 (icnt_wrapper.h)

`icnt_wrapper.h`는 인터커넥트의 **함수 포인터 인터페이스**를 정의한다.
GPGPU-Sim 코어 시뮬레이터는 이 함수 포인터들을 통해 인터커넥트와 상호작용하며, 실제 구현체가 BookSim2인지 Local Crossbar인지 알 필요가 없다.

```
소스 위치: src/gpgpu-sim/icnt_wrapper.h, icnt_wrapper.cc

  ┌──────────────────────────────┐
  │   GPGPU-Sim Core Simulator  │
  │   (shader_core, mem_ctrl)    │
  └──────────┬───────────────────┘
             │ icnt_push(), icnt_pop(), icnt_has_buffer(), ...
             │ (함수 포인터 호출)
  ┌──────────┴───────────────────┐
  │        icnt_wrapper          │
  │  g_network_mode 에 따라 분기  │
  ├─────────────┬────────────────┤
  │ INTERSIM=1  │  LOCAL_XBAR=2  │
  │ (BookSim2)  │  (Local Xbar)  │
  └──────┬──────┴───────┬────────┘
         │              │
  ┌──────┴──────┐ ┌─────┴──────┐
  │ intersim2/  │ │local_inter │
  │ BookSim2    │ │connect     │
  └─────────────┘ └────────────┘
```

### 2.2 icnt_wrapper_init() 핵심 흐름

```cpp
// icnt_wrapper.cc:163
void icnt_wrapper_init() {
  switch (g_network_mode) {
    case INTERSIM:   // 모드 1: BookSim2 기반 상세 시뮬레이션
      g_icnt_interface = InterconnectInterface::New(g_network_config_filename);
      // 모든 함수 포인터를 intersim2_* 래퍼에 바인딩
      icnt_create = intersim2_create;    // → g_icnt_interface->CreateInterconnect()
      icnt_push   = intersim2_push;      // → g_icnt_interface->Push()
      icnt_pop    = intersim2_pop;       // → g_icnt_interface->Pop()
      icnt_transfer = intersim2_transfer; // → g_icnt_interface->Advance()
      // ... 나머지 함수 포인터도 동일 패턴
      break;

    case LOCAL_XBAR: // 모드 2: 단순 크로스바
      g_localicnt_interface = LocalInterconnect::New(g_inct_config);
      icnt_create = LocalInterconnect_create;
      icnt_push   = LocalInterconnect_push;
      // ... 동일 패턴
      break;
  }
}
```

### 2.3 설정 옵션

`icnt_reg_options()`에서 등록하는 주요 파라미터:

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `-network_mode` | 1 | 1=INTERSIM(BookSim2), 2=LOCAL_XBAR |
| `-inter_config_file` | "mesh" | BookSim2 설정 파일 경로 |
| `-icnt_in_buffer_limit` | 64 | Local Xbar 입력 버퍼 크기 |
| `-icnt_out_buffer_limit` | 64 | Local Xbar 출력 버퍼 크기 |
| `-icnt_subnets` | 2 | 서브네트워크 수 |
| `-icnt_arbiter_algo` | 1 (iSLIP) | 중재 알고리즘 (0=RR, 1=iSLIP) |
| `-icnt_grant_cycles` | 1 | iSLIP grant 유지 사이클 |

---

## 3. BookSim2 통합: InterconnectInterface

### 3.1 InterconnectInterface의 역할

`InterconnectInterface`는 GPGPU-Sim과 BookSim2 NoC 시뮬레이터 사이의 **브릿지** 역할을 한다.
GPGPU-Sim의 `mem_fetch` 패킷을 BookSim2의 `Flit` 단위로 변환하고, 네트워크를 통과한 flit을 다시 패킷으로 조립한다.

```
소스 위치: src/intersim2/interconnect_interface.cpp, interconnect_interface.hpp

  GPGPU-Sim 측                    InterconnectInterface                BookSim2 측
  ┌─────────┐                    ┌───────────────────┐               ┌────────────┐
  │mem_fetch │──Push()──────────→│ _input_queue      │──WriteFlit()─→│  Network   │
  │ (패킷)   │                   │ (flit 분할/큐잉)   │               │  (라우터들) │
  │          │                   │                   │               │            │
  │          │←─Pop()────────────│ _boundary_buffer  │←─ReadFlit()──│            │
  │          │                   │ (flit 재조립)      │               │            │
  └─────────┘                    │                   │               │            │
                                 │ _ejection_buffer  │←──────────────│            │
                                 │ _ejected_flit_q   │               │            │
                                 └───────────────────┘               └────────────┘
```

### 3.2 CreateInterconnect() 초기화

```cpp
// interconnect_interface.cpp:83
void InterconnectInterface::CreateInterconnect(unsigned n_shader, unsigned n_mem) {
  _n_shader = n_shader;
  _n_mem = n_mem;

  // 1. 라우팅 맵 초기화 (VC 범위를 트래픽 유형별로 분할)
  InitializeRoutingMap(*_icnt_config);

  // 2. 서브넷 수만큼 Network 객체 생성 (topology에 따라 Mesh, Torus, CMesh 등)
  _subnets = _icnt_config->GetInt("subnets");  // 보통 2
  _net.resize(_subnets);
  for (int i = 0; i < _subnets; ++i) {
    _net[i] = Network::New(*_icnt_config, name.str()); // 토폴로지에 맞는 네트워크 생성
  }

  // 3. GPUTrafficManager 생성 (BookSim2의 TrafficManager를 GPU용으로 확장)
  _traffic_manager = static_cast<GPUTrafficManager*>(
    TrafficManager::New(*_icnt_config, _net));

  // 4. flit 크기 설정
  _flit_size = _icnt_config->GetInt("flit_size");

  // 5. 인터페이스 버퍼 생성 (ejection, boundary)
  _CreateBuffer();

  // 6. 노드 매핑 생성 (SM/Mem deviceID → 네트워크 icntID)
  _CreateNodeMap(_n_shader, _n_mem, ...);
}
```

### 3.3 노드 매핑 (Node Map)

BookSim2 네트워크에서 SM과 메모리 파티션의 물리적 위치를 결정한다.
`use_map=1`이면 사전 정의된 배치 맵을 사용하여 SM과 메모리를 메시 토폴로지에 교대로 배치한다.

예시: 8 SM + 8 Memory 일 때 4x4 메시 배치:

```
  +----+----+----+----+
  | C0 | M0 | C1 | M1 |    C = Compute (SM)
  +----+----+----+----+    M = Memory Partition
  | M2 | C2 | M3 | C3 |
  +----+----+----+----+    deviceID → icntID 매핑:
  | C4 | M4 | C5 | M5 |    SM0→0, SM1→2, SM2→5, SM3→7
  +----+----+----+----+    Mem0→1, Mem1→3, Mem2→4, Mem3→6
  | M6 | C6 | M7 | C7 |
  +----+----+----+----+
```

### 3.4 Push(): 패킷 주입

```cpp
// interconnect_interface.cpp:147
void InterconnectInterface::Push(unsigned input_deviceID, unsigned output_deviceID,
                                  void *data, unsigned int size) {
  // 1. 버퍼 공간 확인
  assert(HasBuffer(input_deviceID, size));

  // 2. deviceID를 네트워크 내부 icntID로 변환
  int output_icntID = _node_map[output_deviceID];
  int input_icntID  = _node_map[input_deviceID];

  // 3. 패킷 크기를 flit 수로 변환
  //    예: 128B 패킷, flit_size=32B → 4 flits
  unsigned int n_flits = size / _flit_size + ((size % _flit_size) ? 1 : 0);

  // 4. 서브넷 결정: SM이 보내면 subnet 0 (요청), 메모리가 보내면 subnet 1 (응답)
  int subnet;
  if (input_deviceID < _n_shader) subnet = 0;  // SM → 요청 네트워크
  else                            subnet = 1;  // Memory → 응답 네트워크

  // 5. mem_fetch 타입에서 flit 타입으로 변환
  Flit::FlitType packet_type;
  mem_fetch* mf = static_cast<mem_fetch*>(data);
  switch (mf->get_type()) {
    case READ_REQUEST:  packet_type = Flit::READ_REQUEST;  break;
    case WRITE_REQUEST: packet_type = Flit::WRITE_REQUEST; break;
    case READ_REPLY:    packet_type = Flit::READ_REPLY;    break;
    case WRITE_ACK:     packet_type = Flit::WRITE_REPLY;   break;
  }

  // 6. GPUTrafficManager에 패킷 생성 요청
  _traffic_manager->_GeneratePacket(input_icntID, -1, 0/*class*/, _traffic_manager->_time,
                                     subnet, n_flits, packet_type, data, output_icntID);
}
```

### 3.5 Pop(): 패킷 추출

```cpp
// interconnect_interface.cpp:200
void* InterconnectInterface::Pop(unsigned deviceID) {
  int icntID = _node_map[deviceID];

  // SM이 pop하면 → reply 네트워크(subnet 1)에서, 메모리가 pop하면 → request 네트워크(subnet 0)에서
  int subnet = 0;
  if (deviceID < _n_shader) subnet = 1;  // SM은 응답을 받음

  // 라운드-로빈으로 VC를 순회하며 완성된 패킷 추출
  int turn = _round_robin_turn[subnet][icntID];
  void* data = NULL;
  for (int vc = 0; (vc < _vcs) && (data == NULL); vc++) {
    if (_boundary_buffer[subnet][icntID][turn].HasPacket()) {
      data = _boundary_buffer[subnet][icntID][turn].PopPacket();
    }
    turn = (turn + 1) % _vcs;
  }
  return data;  // 완성된 패킷의 원본 data 포인터 (mem_fetch*)
}
```

### 3.6 인터페이스 버퍼 계층

```
  Network(BookSim2) 출력
        │
        ▼
  ┌─────────────────┐    ReadFlit()으로 네트워크에서 flit 수신
  │ Ejection Buffer │    크기: [subnets][nodes][vcs]
  │ (VC별 큐)        │    용량: ejection_buffer_capacity (= vc_buf_size)
  └────────┬────────┘
           │ Transfer2BoundaryBuffer()
           ▼
  ┌─────────────────┐    flit 데이터를 모아 패킷 재조립
  │ Boundary Buffer │    tail flit이 도착하면 패킷 완성
  │ (VC별 패킷 큐)   │    용량: boundary_buffer_capacity
  └────────┬────────┘
           │ Pop() → PopPacket()
           ▼
    GPGPU-Sim으로 전달
```

---

## 4. 네트워크 토폴로지

### 4.1 지원 토폴로지 목록

`Network::New()`에서 설정 파일의 `topology` 파라미터에 따라 생성된다.

```cpp
// networks/network.cpp:80
Network * Network::New(const Configuration & config, const string & name) {
  const string topo = config.GetStr("topology");
  if      (topo == "torus")        → KNCube (wrap-around 있음)
  else if (topo == "mesh")         → KNCube (wrap-around 없음)
  else if (topo == "cmesh")        → CMesh  (Concentrated Mesh)
  else if (topo == "fly")          → KNFly  (Butterfly)
  else if (topo == "qtree")        → QTree  (Quad Tree)
  else if (topo == "tree4")        → Tree4  (4-ary Tree)
  else if (topo == "fattree")      → FatTree
  else if (topo == "flatfly")      → FlatFlyOnChip
  else if (topo == "anynet")       → AnyNet (사용자 정의)
  else if (topo == "dragonflynew") → DragonFlyNew
}
```

### 4.2 Network 기본 클래스 구조

```cpp
// networks/network.cpp
class Network {
  int _size;      // 라우터 수
  int _nodes;     // 노드(단말) 수 (SM + Memory)
  int _channels;  // 라우터 간 채널 수

  vector<Router*>       _routers;      // 모든 라우터
  vector<FlitChannel*>  _inject;       // 주입 채널 (노드 → 라우터)
  vector<CreditChannel*> _inject_cred; // 주입 크레딧 채널
  vector<FlitChannel*>  _eject;        // 추출 채널 (라우터 → 노드)
  vector<CreditChannel*> _eject_cred;  // 추출 크레딧 채널
  vector<FlitChannel*>  _chan;         // 라우터 간 flit 채널
  vector<CreditChannel*> _chan_cred;   // 라우터 간 크레딧 채널
};
```

### 4.3 Concentrated Mesh (CMesh) 토폴로지

GPGPU-Sim에서 가장 많이 사용되는 토폴로지이다.
일반 2D mesh에서 각 라우터에 여러 노드를 **집중(concentrate)** 연결한 구조이다.

```
  CMesh 구성 파라미터:
  - k: 각 차원의 라우터 수 (예: k=4 → 4x4 = 16 라우터)
  - n: 차원 수 (2 = 2D mesh)
  - c: concentration factor (4 = 라우터당 4개 노드)
  - 총 노드 수 = c * k^n (예: 4 * 4^2 = 64 노드)
  - 총 라우터 수 = k^n (예: 16 라우터)
  - 채널 수 = 2 * n * k^n (예: 64 채널)
```

```
   CMesh 4x4 (c=4, k=4, n=2) 토폴로지

   라우터 하나의 내부 연결:
   ┌─────────────────────────┐
   │      Router (i,j)       │
   │                         │
   │  Port 0~3: 노드 연결     │  ← c=4개의 로컬 노드 (SM 또는 Mem)
   │  Port 4: +x 방향 채널    │  ← 인접 라우터 연결
   │  Port 5: -x 방향 채널    │
   │  Port 6: +y 방향 채널    │
   │  Port 7: -y 방향 채널    │
   └─────────────────────────┘

   전체 토폴로지:
   ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
   │ R(0,0)   │────│ R(0,1)   │────│ R(0,2)   │────│ R(0,3)   │
   │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│
   └────┬─────┘    └────┬─────┘    └────┬─────┘    └────┬─────┘
        │               │               │               │
   ┌────┴─────┐    ┌────┴─────┐    ┌────┴─────┐    ┌────┴─────┐
   │ R(1,0)   │────│ R(1,1)   │────│ R(1,2)   │────│ R(1,3)   │
   │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│
   └────┬─────┘    └────┬─────┘    └────┬─────┘    └────┬─────┘
        │               │               │               │
   ┌────┴─────┐    ┌────┴─────┐    ┌────┴─────┐    ┌────┴─────┐
   │ R(2,0)   │────│ R(2,1)   │────│ R(2,2)   │────│ R(2,3)   │
   │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│
   └────┬─────┘    └────┬─────┘    └────┬─────┘    └────┬─────┘
        │               │               │               │
   ┌────┴─────┐    ┌────┴─────┐    ┌────┴─────┐    ┌────┴─────┐
   │ R(3,0)   │────│ R(3,1)   │────│ R(3,2)   │────│ R(3,3)   │
   │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│    │ [4 nodes]│
   └──────────┘    └──────────┘    └──────────┘    └──────────┘

   Express Link: 가장자리 라우터끼리 k/2 거리만큼 떨어진 라우터와 직접 연결
```

### 4.4 CMesh Express Link

CMesh는 가장자리 라우터에서 **express link**를 제공한다.
이는 메시 가장자리에서 먼 목적지까지의 홉 수를 줄여준다.

```cpp
// cmesh.cpp:196
// 예: x=0 (왼쪽 가장자리) → -x 방향으로 k/2 거리의 같은 가장자리 라우터에 연결
if (x == 0) {
  if (y < _k / 2)
    nx_in = _k * (y + _k/2) + x + offset;  // 아래쪽 절반 → 위쪽 절반으로 연결
  else
    nx_in = _k * (y - _k/2) + x + offset;  // 위쪽 절반 → 아래쪽 절반으로 연결
}
```

### 4.5 CMesh NoC 레이턴시

`use_noc_latency=1`이면 실제 칩 내 와이어 지연을 모델링한다.

| 채널 유형 | 레이턴시 |
|----------|---------|
| 인접 라우터 간 (+x/-x) | _cX 사이클 (concentration X 방향 크기) |
| 인접 라우터 간 (+y/-y) | _cY 사이클 |
| Express link (+x 가장자리) | _cY * k/2 사이클 |
| 주입/추출 채널 | 1 사이클 |

---

## 5. 라우터 구조: IQ Router (Input-Queued Router)

### 5.1 IQ Router 파이프라인

GPGPU-Sim의 BookSim2는 주로 Input-Queued Router를 사용한다.
각 라우터는 매 사이클마다 다단계 파이프라인을 실행한다.

```
소스 위치: src/intersim2/routers/iq_router.cpp

  IQ Router 파이프라인 (4~5 스테이지)

  ┌─────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
  │  Input   │──→│ Routing  │──→│    VC    │──→│  Switch  │──→│ Switch   │
  │ Queuing  │   │ (RC)     │   │ Allocate │   │ Allocate │   │ Traversal│
  │          │   │          │   │ (VA)     │   │ (SA)     │   │ (ST)     │
  └─────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘
       │                                                           │
       │  Credit                                            Output │
       │  Return  ←───────────────────────────────────────  Buffer │
       │                                                           │
```

### 5.2 각 파이프라인 스테이지 상세

#### Stage 1: Input Queuing (_InputQueuing)

```cpp
// iq_router.cpp:339
void IQRouter::_InputQueuing() {
  // 수신된 flit을 해당 VC의 입력 버퍼에 저장
  for (각 수신된 flit f) {
    Buffer* cur_buf = _buf[input];
    cur_buf->AddFlit(vc, f);

    if (VC 상태가 idle && f가 head flit) {
      if (_routing_delay > 0) {
        // 라우팅 계산이 필요 → routing 스테이지로
        cur_buf->SetState(vc, VC::routing);
        _route_vcs에 추가;
      } else {
        // Lookahead routing 사용 → 바로 VC allocation으로
        cur_buf->SetRouteSet(vc, &f->la_route_set);
        cur_buf->SetState(vc, VC::vc_alloc);
      }
    }
  }
  // 수신된 크레딧 처리 → 다음 라우터의 버퍼 상태 업데이트
  dest_buf->ProcessCredit(c);
}
```

#### Stage 2: Route Computation (_RouteEvaluate / _RouteUpdate)

head flit이 도착하면 라우팅 함수를 호출하여 출력 포트와 VC 범위를 결정한다.

```cpp
// iq_router.cpp:499
void IQRouter::_RouteUpdate() {
  cur_buf->Route(vc, _rf, this, f, input);  // 라우팅 함수 호출
  cur_buf->SetState(vc, VC::vc_alloc);      // VC 할당 단계로 전이
}
```

#### Stage 3: VC Allocation (_VCAllocEvaluate / _VCAllocUpdate)

출력 포트의 VC를 할당받는 단계이다. 여러 입력 VC가 같은 출력 VC를 요청하면 중재기(Allocator)가 해결한다.

```cpp
// iq_router.cpp:553
void IQRouter::_VCAllocEvaluate() {
  for (각 VC 할당 대기 중인 flit) {
    OutputSet const * route_set = cur_buf->GetRouteSet(vc);
    for (각 가능한 출력 포트/VC 조합) {
      if (dest_buf->IsAvailableFor(out_vc) && !dest_buf->IsFullFor(out_vc)) {
        // 중재기에 요청 등록
        _vc_allocator->AddRequest(input_and_vc, out_port*_vcs + out_vc, ...);
      }
    }
  }
  _vc_allocator->Allocate();  // 중재 실행
  // 결과: 각 입력 VC에 출력 VC 할당 또는 STALL
}
```

#### Stage 4: Switch Allocation (_SWAllocEvaluate / _SWAllocUpdate)

크로스바 스위치 포트를 할당받는 단계이다.

#### Stage 5: Switch Traversal (_SwitchEvaluate / _SwitchUpdate)

할당된 크로스바 포트를 통해 flit이 출력 채널로 전달된다.

### 5.3 VC 상태 머신

```
                 head flit 도착
                      │
                      ▼
  ┌─────┐  routing  ┌──────────┐  route done  ┌──────────┐  VC granted
  │idle │──────────→│ routing  │─────────────→│ vc_alloc │────────────→
  └─────┘  delay>0  └──────────┘              └──────────┘
    ↑                                                          │
    │                                                          ▼
    │       tail flit                                    ┌──────────┐
    │       전송 완료                                     │  active  │
    └───────────────────────────────────────────────────│          │
                                                        │ body/tail│
                                                        │ flit 전송 │
                                                        └──────────┘
```

### 5.4 라우터 구성 요소

```cpp
// iq_router.cpp:50
IQRouter::IQRouter(...) {
  _vcs = config.GetInt("num_vcs");         // Virtual Channel 수
  _routing_delay = config.GetInt("routing_delay");     // RC 지연 (0이면 lookahead)
  _vc_alloc_delay = config.GetInt("vc_alloc_delay");   // VA 지연
  _sw_alloc_delay = config.GetInt("sw_alloc_delay");   // SA 지연
  _speculative = (config.GetInt("speculative") > 0);   // VA+SA 병렬 실행 여부

  // 입력 버퍼: 포트별로 Buffer 객체
  _buf.resize(_inputs);  // _inputs = degree_in (예: CMesh에서 4+c)

  // 다음 라우터의 버퍼 상태 추적 (크레딧 기반 흐름 제어)
  _next_buf.resize(_outputs);  // BufferState 객체

  // VC 중재기: 입력 VC → 출력 VC 매핑
  _vc_allocator = Allocator::NewAllocator(..., _vcs*_inputs, _vcs*_outputs);

  // 스위치 중재기: 입력 포트 → 출력 포트 매핑
  _sw_allocator = Allocator::NewAllocator(..., _inputs*_input_speedup, _outputs*_output_speedup);
}
```

---

## 6. Virtual Channel (VC) 관리

### 6.1 VC의 목적

Virtual Channel은 물리 채널 하나를 여러 논리 채널로 분할하여 다음을 달성한다:
- **데드락 방지**: 서로 다른 트래픽 유형(READ_REQ, WRITE_REQ, READ_REPLY, WRITE_REPLY)을 별도 VC에 할당
- **Head-of-Line blocking 완화**: 한 VC에서 막힌 flit이 다른 VC의 flit을 차단하지 않음
- **대역폭 활용도 향상**: 여러 패킷이 같은 물리 링크를 공유

### 6.2 트래픽 유형별 VC 분할

```cpp
// routefunc.cpp:1917
void InitializeRoutingMap(const Configuration & config) {
  gNumVCs = config.GetInt("num_vcs");

  // 기본 분할: 전체 VC를 절반으로 나눔
  // VC 0 ~ (numVCs/2-1): Request (READ_REQ, WRITE_REQ)
  // VC (numVCs/2) ~ (numVCs-1): Reply (READ_REPLY, WRITE_REPLY)
  gReadReqBeginVC   = 0;
  gReadReqEndVC     = gNumVCs / 2 - 1;
  gWriteReqBeginVC  = 0;
  gWriteReqEndVC    = gNumVCs / 2 - 1;
  gReadReplyBeginVC = gNumVCs / 2;
  gReadReplyEndVC   = gNumVCs - 1;
  gWriteReplyBeginVC= gNumVCs / 2;
  gWriteReplyEndVC  = gNumVCs - 1;
}
```

### 6.3 BufferState: 크레딧 기반 흐름 제어

`BufferState`는 다음 홉 라우터의 입력 버퍼 상태를 추적한다.
flit을 보낼 때 크레딧을 소비하고, 상대방이 flit을 처리하면 크레딧을 반환한다.

```
소스 위치: src/intersim2/buffer_state.cpp

  라우터 A                          라우터 B
  ┌────────────┐    flit 전송     ┌────────────┐
  │ BufferState│──────────────→  │ Input Buf  │
  │ (B의 버퍼   │                  │ (실제 버퍼)  │
  │  상태 추적) │  ←─ credit ───  │            │
  │            │    반환           │            │
  │ _occupancy │                  │            │
  │ _vc_occupancy[vc]│            │            │
  │ _in_use_by[vc]  │            │            │
  └────────────┘                  └────────────┘
```

```cpp
// buffer_state.cpp:572
void BufferState::ProcessCredit(Credit const * const c) {
  for (각 VC in credit) {
    --_occupancy;           // 전체 사용량 감소
    --_vc_occupancy[vc];    // VC별 사용량 감소
    // tail 크레딧이면 VC 해제
    if (_wait_for_tail_credit && !_vc_occupancy[vc] && _tail_sent[vc]) {
      _in_use_by[vc] = -1;  // VC를 idle로 전환
    }
    _buffer_policy->FreeSlotFor(vc);
  }
}

void BufferState::SendingFlit(Flit const * const f) {
  ++_occupancy;             // 전체 사용량 증가
  ++_vc_occupancy[f->vc];  // VC별 사용량 증가
  if (f->tail) {
    _tail_sent[vc] = true;  // 패킷의 마지막 flit 전송 완료
    _in_use_by[vc] = -1;    // wait_for_tail_credit가 아니면 바로 해제
  }
}

void BufferState::TakeBuffer(int vc, int tag) {
  // VC를 특정 패킷에 할당 (VA 성공 시 호출)
  _in_use_by[vc] = tag;
  _tail_sent[vc] = false;
}
```

### 6.4 버퍼 정책 (Buffer Policy)

BufferState는 다양한 버퍼 할당 정책을 지원한다.

| 정책 | 설명 |
|------|------|
| `private` | 각 VC에 고정 크기 할당 (vc_buf_size) |
| `shared` | private 영역 + 공유 풀 |
| `limited` | shared + VC당 최대 점유 제한 |
| `dynamic` | active VC 수에 따라 동적 제한 (buf_size / active_vcs) |
| `feedback` | RTT 측정 기반 동적 제한 |

---

## 7. Flit 기반 패킷 전송

### 7.1 Flit 구조

```
소스 위치: src/intersim2/flit.cpp

  Flit 구조체 주요 필드:
  ┌─────────────────────────────────┐
  │ id:     고유 flit ID              │
  │ pid:    소속 패킷 ID              │
  │ head:   head flit 여부 (true/false)│
  │ tail:   tail flit 여부            │
  │ type:   READ_REQ/WRITE_REQ/...   │
  │ src:    소스 노드 ID              │
  │ dest:   목적지 노드 ID (head만)    │
  │ vc:     할당된 Virtual Channel    │
  │ cl:     트래픽 class              │
  │ data:   원본 mem_fetch 포인터     │
  │ pri:    우선순위                   │
  │ ctime:  생성 시간                  │
  │ itime:  주입 시간                  │
  │ atime:  도착 시간                  │
  │ hops:   거친 홉 수                 │
  │ la_route_set: lookahead 라우팅 정보│
  └─────────────────────────────────┘
```

### 7.2 Flit 메모리 풀

Flit 객체는 빈번하게 생성/소멸되므로, **오브젝트 풀**로 관리한다.

```cpp
// flit.cpp:85
Flit * Flit::New() {
  Flit * f;
  if (_free.empty()) {
    f = new Flit;         // 풀이 비었으면 새로 할당
    _all.push(f);         // 전체 목록에 추가
  } else {
    f = _free.top();      // 재사용 가능한 flit 가져옴
    f->Reset();           // 필드 초기화
    _free.pop();
  }
  return f;
}

void Flit::Free() {
  _free.push(this);  // 풀에 반환 (delete 하지 않음)
}
```

### 7.3 패킷 분할 (Packetization)

하나의 `mem_fetch`는 여러 flit으로 분할된다.

```cpp
// gputrafficmanager.cpp:192
void GPUTrafficManager::_GeneratePacket(int source, ..., int packet_size, ..., void* data, int dest) {
  unsigned long long pid = _cur_pid++;  // 패킷 ID 할당

  for (int i = 0; i < size; ++i) {
    Flit * f = Flit::New();
    f->id  = _cur_id++;      // 각 flit에 고유 ID
    f->pid = pid;             // 같은 패킷의 모든 flit은 같은 pid
    f->src = source;
    f->data = data;           // 모든 flit이 원본 데이터 포인터를 가짐

    if (i == 0) {             // 첫 번째 flit = Head
      f->head = true;
      f->dest = packet_destination;
    }
    if (i == size - 1) {      // 마지막 flit = Tail
      f->tail = true;
    }

    f->vc = -1;               // 아직 VC 미할당 (주입 시 결정)
    _input_queue[subnet][source][cl].push_back(f);
  }
}
```

```
  패킷 분할 예시 (128B 패킷, 32B flit_size → 4 flits)

  ┌────────────────────────────────────────────────────────┐
  │                   mem_fetch (128 bytes)                  │
  │              (READ_REQUEST 또는 READ_REPLY)               │
  └────────────────────────────────────────────────────────┘
                              │
                    flit 분할 (n_flits = 128/32 = 4)
                              │
         ┌────────────────────┼────────────────────┐
         │                    │                    │
  ┌──────┴──────┐  ┌─────────┴─────┐  ┌──────────┴────┐  ┌──────────┐
  │  Flit 0     │  │  Flit 1       │  │  Flit 2       │  │ Flit 3   │
  │  head=true  │  │  head=false   │  │  head=false   │  │head=false│
  │  tail=false │  │  tail=false   │  │  tail=false   │  │tail=true │
  │  dest=N     │  │  dest=-1      │  │  dest=-1      │  │dest=-1   │
  │  data=ptr   │  │  data=ptr     │  │  data=ptr     │  │data=ptr  │
  └─────────────┘  └───────────────┘  └───────────────┘  └──────────┘
```

### 7.4 패킷 재조립 (BoundaryBuffer)

네트워크에서 추출된 flit은 BoundaryBuffer에서 패킷으로 재조립된다.

```cpp
// interconnect_interface.cpp:323
void InterconnectInterface::Transfer2BoundaryBuffer(int subnet, int output) {
  for (vc = 0; vc < _vcs; vc++) {
    if (!_ejection_buffer[subnet][output][vc].empty() &&
        _boundary_buffer[subnet][output][vc].Size() < _boundary_buffer_capacity) {
      flit = _ejection_buffer[subnet][output][vc].front();
      _ejection_buffer[subnet][output][vc].pop();
      // flit의 데이터와 tail 여부를 boundary buffer에 저장
      _boundary_buffer[subnet][output][vc].PushFlitData(flit->data, flit->tail);
      // 크레딧 반환을 위해 ejected_flit_queue에 추가
      _ejected_flit_queue[subnet][output].push(flit);
    }
  }
}

// PopPacket(): tail flit이 도착할 때까지 데이터를 모아 하나의 패킷으로 반환
void* _BoundaryBufferItem::PopPacket() {
  void* data = NULL;
  while (data == NULL) {
    if (_tail_flag.front()) {  // tail flit 발견
      data = _buffer.front();  // 원본 data 포인터 반환
      _packet_n--;
    }
    _buffer.pop();
    _tail_flag.pop();
  }
  return data;  // → mem_fetch* 로 캐스팅되어 GPGPU-Sim에 전달
}
```

---

## 8. FlitChannel: 채널 지연 모델링

### 8.1 FlitChannel 구조

`FlitChannel`은 라우터 간(또는 노드-라우터 간) 물리적 와이어를 모델링한다.
내부적으로 FIFO 큐를 사용하여 채널 지연(latency)을 시뮬레이션한다.

```
소스 위치: src/intersim2/flitchannel.cpp

  FlitChannel은 Channel<Flit> 템플릿을 상속
  ┌──────────────────────────────────────┐
  │ FlitChannel (delay = N cycles)       │
  │                                      │
  │  Send(flit) → ┌─┬─┬─┬─┐ → Receive() │
  │                │ │ │ │ │  (N-cycle    │
  │                └─┴─┴─┴─┘   FIFO)     │
  │                                      │
  │  _routerSource: 소스 라우터            │
  │  _routerSink: 싱크 라우터              │
  │  _active[class]: 클래스별 활성 카운트   │
  │  _idle: 유휴 사이클 카운트              │
  └──────────────────────────────────────┘
```

```cpp
// flitchannel.cpp:64
void FlitChannel::Send(Flit * f) {
  if (f) {
    ++_active[f->cl];  // 활성 전송 통계
  } else {
    ++_idle;           // 유휴 사이클 통계
  }
  Channel<Flit>::Send(f);  // 부모 클래스의 FIFO에 삽입
}
```

---

## 9. 트래픽 관리: GPUTrafficManager

### 9.1 GPUTrafficManager의 역할

BookSim2의 `TrafficManager`를 상속하여 GPU 특화 동작을 구현한다.
표준 BookSim2는 합성 트래픽을 자체 생성하지만, GPU 버전은 GPGPU-Sim이 생성한 `mem_fetch` 패킷을 처리한다.

### 9.2 _Step(): 매 사이클 실행되는 핵심 함수

```cpp
// gputrafficmanager.cpp:335
void GPUTrafficManager::_Step() {
  // === Phase 1: 네트워크 출력 처리 (Ejection) ===
  for (subnet = 0; subnet < _subnets; ++subnet) {
    for (n = 0; n < _nodes; ++n) {
      // 1a. 네트워크에서 flit 읽기
      Flit * f = _net[subnet]->ReadFlit(n);
      if (f) {
        g_icnt_interface->WriteOutBuffer(subnet, n, f);  // ejection buffer에 저장
      }

      // 1b. ejection → boundary 버퍼로 이동
      g_icnt_interface->Transfer2BoundaryBuffer(subnet, n);

      // 1c. ejected flit에 대한 크레딧 처리
      Flit* ejected = g_icnt_interface->GetEjectedFlit(subnet, n);
      if (ejected) {
        flits[subnet][n] = ejected;  // retire 대기열에 추가
      }

      // 1d. 크레딧 수신 처리
      Credit * c = _net[subnet]->ReadCredit(n);
      if (c) {
        _buf_states[n][subnet]->ProcessCredit(c);  // 다음 라우터 버퍼 상태 업데이트
        c->Free();
      }
    }
    _net[subnet]->ReadInputs();  // 채널 입력 읽기
  }

  // === Phase 2: 패킷 주입 (Injection) ===
  for (subnet = 0; subnet < _subnets; ++subnet) {
    for (n = 0; n < _nodes; ++n) {
      Flit * f = NULL;
      BufferState * dest_buf = _buf_states[n][subnet];

      // 2a. _input_queue에서 주입할 flit 선택
      //     head flit이면 사용 가능한 VC를 찾아 할당
      if (cf->head && cf->vc == -1) {
        // 라우팅 함수로 출력 포트 결정
        _rf(NULL, cf, -1, &route_set, true);
        // 사용 가능한 VC 범위에서 빈 VC 탐색
        for (vc = vc_start; vc <= vc_end; vc++) {
          if (dest_buf->IsAvailableFor(vc) && !dest_buf->IsFullFor(vc)) {
            cf->vc = vc;  // VC 할당
            break;
          }
        }
      }

      // 2b. flit 주입
      if (f) {
        if (f->head) {
          dest_buf->TakeBuffer(f->vc);  // VC 예약
          // Lookahead routing 정보 생성
          _rf(router, f, in_channel, &f->la_route_set, false);
        }
        f->itime = _time;  // 주입 시간 기록
        // 같은 패킷의 다음 flit에 VC 전파
        if (!_input_queue[subnet][n][c].empty() && !f->tail) {
          nf->vc = f->vc;
        }
        _net[subnet]->WriteFlit(f, n);  // 네트워크에 flit 주입
        dest_buf->SendingFlit(f);       // 크레딧 소비
      }
    }
  }

  // === Phase 3: 크레딧 전송 및 네트워크 진행 ===
  for (subnet = 0; subnet < _subnets; ++subnet) {
    for (n = 0; n < _nodes; ++n) {
      if (flits[subnet]에 n이 있으면) {
        Flit * f = flits[subnet][n];
        f->atime = _time;  // 도착 시간 기록

        // 크레딧 반환
        Credit * c = Credit::New();
        c->vc.insert(f->vc);
        _net[subnet]->WriteCredit(c, n);

        _RetireFlit(f, n);  // flit 은퇴 처리
      }
    }
    // 네트워크 시뮬레이션 한 스텝 진행
    _net[subnet]->Evaluate();
    _net[subnet]->WriteOutputs();
  }

  ++_time;
}
```

### 9.3 _Step() 전체 흐름도

```
  ┌──────────────────────────────────────────────────────┐
  │                    _Step() 한 사이클                    │
  ├──────────────────────────────────────────────────────┤
  │                                                      │
  │  Phase 1: Ejection (네트워크 → 인터페이스)              │
  │  ┌─────────────────────────────────────────────────┐ │
  │  │ for 각 (subnet, node):                          │ │
  │  │   ReadFlit() → WriteOutBuffer()                 │ │
  │  │   Transfer2BoundaryBuffer()                     │ │
  │  │   GetEjectedFlit() → retire 큐에 추가            │ │
  │  │   ReadCredit() → ProcessCredit()                │ │
  │  │ ReadInputs() (채널 FIFO 진행)                    │ │
  │  └─────────────────────────────────────────────────┘ │
  │                                                      │
  │  Phase 2: Injection (인터페이스 → 네트워크)             │
  │  ┌─────────────────────────────────────────────────┐ │
  │  │ for 각 (subnet, node):                          │ │
  │  │   _input_queue에서 flit 선택                     │ │
  │  │   head flit: VC 할당, lookahead routing          │ │
  │  │   WriteFlit() → 네트워크에 주입                   │ │
  │  │   SendingFlit() → 크레딧 소비                    │ │
  │  └─────────────────────────────────────────────────┘ │
  │                                                      │
  │  Phase 3: Network Advance                            │
  │  ┌─────────────────────────────────────────────────┐ │
  │  │ for 각 (subnet, node):                          │ │
  │  │   ejected flit에 대해 크레딧 생성 및 WriteCredit  │ │
  │  │   _RetireFlit() → 통계 수집                      │ │
  │  │ Evaluate() → 라우터 내부 로직 실행                 │ │
  │  │ WriteOutputs() → 채널 출력 기록                   │ │
  │  └─────────────────────────────────────────────────┘ │
  │                                                      │
  │  ++_time                                             │
  └──────────────────────────────────────────────────────┘
```

### 9.4 _RetireFlit(): Flit 은퇴 처리

```cpp
// gputrafficmanager.cpp:64
void GPUTrafficManager::_RetireFlit(Flit *f, int dest) {
  _deadlock_timer = 0;  // 데드락 타이머 리셋

  // in-flight 추적 맵에서 제거
  _total_in_flight_flits[f->cl].erase(f->id);

  // 레이턴시 통계 수집
  _flat_stats[f->cl]->AddSample(f->atime - f->itime);  // 네트워크 flat 레이턴시

  if (f->tail) {  // 패킷의 마지막 flit
    // 패킷 레이턴시 = 도착시간 - 생성시간
    _plat_stats[f->cl]->AddSample(f->atime - head->ctime);
    // 네트워크 레이턴시 = 도착시간 - 주입시간
    _nlat_stats[f->cl]->AddSample(f->atime - head->itime);
    // fragmentation = 실제 전송 시간 - 이상적 전송 시간
    _frag_stats[f->cl]->AddSample((f->atime - head->atime) - (f->id - head->id));
  }

  f->Free();  // flit 객체 풀에 반환
}
```

---

## 10. Local Crossbar (LOCAL_XBAR) 모드

### 10.1 개요

BookSim2의 상세한 NoC 시뮬레이션 대신, 단순한 크로스바 스위치로 인터커넥트를 모델링한다.
flit 분할 없이 패킷 단위로 전송하며, 사이클 정확도는 낮지만 시뮬레이션 속도가 빠르다.

```
소스 위치: src/gpgpu-sim/local_interconnect.h, local_interconnect.cc
```

### 10.2 xbar_router 구조

```
  xbar_router 내부 구조

  입력 (SM 또는 Memory)                           출력 (Memory 또는 SM)
  ┌─────────────┐                               ┌─────────────┐
  │ in_buffer[0] │──┐                        ┌──→│out_buffer[0] │
  ├─────────────┤  │    ┌──────────────┐    │  ├─────────────┤
  │ in_buffer[1] │──┼───→│              │────┼──→│out_buffer[1] │
  ├─────────────┤  │    │   Crossbar   │    │  ├─────────────┤
  │ in_buffer[2] │──┼───→│   Arbiter   │────┼──→│out_buffer[2] │
  ├─────────────┤  │    │ (RR or iSLIP)│    │  ├─────────────┤
  │     ...      │──┼───→│              │────┼──→│    ...       │
  ├─────────────┤  │    └──────────────┘    │  ├─────────────┤
  │in_buffer[N-1]│──┘                        └──→│out_buffer[N-1]│
  └─────────────┘                               └─────────────┘
```

### 10.3 iSLIP 중재 알고리즘

iSLIP은 McKeown(1999)이 제안한 입력 큐잉 스위치용 스케줄링 알고리즘이다.
각 출력 포트마다 독립적인 라운드-로빈 포인터(`next_node[i]`)를 유지한다.

```cpp
// local_interconnect.cc:179
void xbar_router::iSLIP_Advance() {
  // Phase 1: 충돌 통계 수집
  for (각 입력 i) {
    if (in_buffer[i]에 패킷이 있으면) {
      해당 output이 이미 요청된 적 있으면 → conflict_sub++
    }
  }

  // Phase 2: 출력 포트별 Grant
  for (출력 i = 0; i < total_nodes; i++) {
    if (Has_Buffer_Out(i, 1)) {          // 출력 버퍼에 공간이 있으면
      for (j를 next_node[i]부터 순회) {  // 라운드-로빈으로 입력 선택
        unsigned node_id = (j + next_node[i]) % total_nodes;
        if (in_buffer[node_id]의 패킷이 출력 i를 목적지로 함) {
          // Grant: 입력 node_id → 출력 i로 전송
          out_buffers[i].push(패킷);
          in_buffers[node_id].pop();
          if (grant_cycles_count == 1)
            next_node[i] = (++node_id % total_nodes);  // 포인터 전진
          break;
        }
      }
    }
  }
}
```

### 10.4 Naive Round-Robin 중재

단일 글로벌 포인터(`next_node_id`)로 입력을 순회한다.
iSLIP보다 간단하지만 throughput이 낮을 수 있다.

```cpp
// local_interconnect.cc:123
void xbar_router::RR_Advance() {
  vector<bool> issued(total_nodes, false);  // 이미 grant된 출력 추적
  for (i = 0; i < total_nodes; i++) {
    unsigned node_id = (i + next_node_id) % total_nodes;  // 글로벌 RR
    if (in_buffer[node_id]에 패킷이 있으면) {
      Packet pkt = in_buffer[node_id].front();
      if (출력 버퍼에 공간 있고 && 해당 출력이 아직 grant되지 않았으면) {
        out_buffers[pkt.output].push(pkt);
        in_buffers[node_id].pop();
        issued[pkt.output] = true;
      } else {
        conflict_sub++;  // 충돌
      }
    }
  }
  next_node_id = (next_node_id + 1) % total_nodes;  // 포인터 전진
}
```

### 10.5 LocalInterconnect 서브넷 라우팅

```cpp
// local_interconnect.cc:325
void LocalInterconnect::Push(unsigned input_deviceID, unsigned output_deviceID, ...) {
  // SM(input < n_shader)이 보내면 → subnet 0 (REQ_NET)
  // Memory(input >= n_shader)가 보내면 → subnet 1 (REPLY_NET)
  unsigned subnet = (input_deviceID < n_shader) ? 0 : 1;
  net[subnet]->Push(input_deviceID, output_deviceID, data, size);
}

void* LocalInterconnect::Pop(unsigned output_deviceID) {
  // SM이 받으면 → subnet 1 (REPLY_NET)
  // Memory가 받으면 → subnet 0 (REQ_NET)
  int subnet = (output_deviceID < n_shader) ? 1 : 0;
  return net[subnet]->Pop(output_deviceID);
}
```

---

## 11. 라우팅 함수

### 11.1 라우팅 함수 등록

각 토폴로지는 자신만의 라우팅 함수를 `gRoutingFunctionMap`에 등록한다.
등록 형식은 `"라우팅알고리즘_토폴로지"` 이다.

```cpp
// cmesh.cpp:64
void CMesh::RegisterRoutingFunctions() {
  gRoutingFunctionMap["dor_cmesh"] = &dor_cmesh;                       // DOR with express
  gRoutingFunctionMap["dor_no_express_cmesh"] = &dor_no_express_cmesh; // DOR without express
  gRoutingFunctionMap["xy_yx_cmesh"] = &xy_yx_cmesh;                   // Random XY-YX
  gRoutingFunctionMap["xy_yx_no_express_cmesh"] = &xy_yx_no_express_cmesh;
}
```

### 11.2 Dimension-Order Routing (DOR)

CMesh에서의 XY 라우팅: 먼저 X 차원을 정렬한 후 Y 차원을 정렬한다.

```cpp
// cmesh.cpp:343
int cmesh_xy(int cur, int dest) {
  // 포트 번호: 0=+x, 1=-x, 2=+y, 3=-y
  // 먼저 X 방향 이동
  if (cur_x < dest_x) return gC + POSITIVE_X;
  if (cur_x > dest_x) return gC + NEGATIVE_X;
  // X가 같으면 Y 방향 이동
  if (cur_y < dest_y) return gC + POSITIVE_Y;
  if (cur_y > dest_y) return gC + NEGATIVE_Y;
  return 0;  // 현재 라우터가 목적지 → 로컬 포트로 ejection
}
```

### 11.3 XY-YX Adaptive Routing

첫 홉에서 XY 또는 YX 순서를 랜덤으로 선택하여 적응적으로 라우팅한다.
데드락 방지를 위해 XY/YX 각각에 별도 VC 집합을 할당한다.

```cpp
// cmesh.cpp:455
void xy_yx_cmesh(const Router *r, const Flit *f, int in_channel,
                  OutputSet *outputs, bool inject) {
  // VC 범위를 flit 타입에 따라 결정
  int vcBegin, vcEnd;
  // ... (READ_REQ, WRITE_REQ 등에 따라 설정)

  int available_vcs = (vcEnd - vcBegin + 1) / 2;

  // 첫 홉 (in_channel < gC): XY 또는 YX를 랜덤 선택
  bool x_then_y = (in_channel < gC) ? (RandomInt(1) > 0) : (f->vc < vcBegin + available_vcs);

  if (x_then_y) {
    out_port = cmesh_xy(cur_router, dest_router);
    vcEnd -= available_vcs;    // XY는 하위 VC 사용
  } else {
    out_port = cmesh_yx(cur_router, dest_router);
    vcBegin += available_vcs;  // YX는 상위 VC 사용
  }

  outputs->AddRange(out_port, vcBegin, vcEnd);
}
```

---

## 12. 네트워크 시뮬레이션 사이클

### 12.1 Network::Evaluate / WriteOutputs

```cpp
// network.cpp:191
void Network::Evaluate() {
  // 모든 timed_module을 순회하며 Evaluate 호출
  // 여기에는 모든 라우터, FlitChannel, CreditChannel이 포함됨
  for (각 timed_module : _timed_modules)
    (*iter)->Evaluate();
}

void Network::WriteOutputs() {
  // 모든 timed_module의 출력을 기록
  for (각 timed_module : _timed_modules)
    (*iter)->WriteOutputs();
}
```

### 12.2 전체 시뮬레이션 루프

```
  GPGPU-Sim gpu_sim_cycle()
       │
       ├── icnt_push()    // SM/Memory가 패킷을 인터커넥트에 주입
       │
       ├── icnt_transfer() // 인터커넥트 한 사이클 진행
       │   └── InterconnectInterface::Advance()
       │       └── GPUTrafficManager::_Step()
       │           ├── Phase 1: ReadFlit/ReadCredit (Ejection)
       │           ├── Phase 2: WriteFlit (Injection)
       │           └── Phase 3: Evaluate/WriteOutputs (Network Step)
       │
       └── icnt_pop()     // SM/Memory가 인터커넥트에서 패킷 수신
```

---

## 13. 핵심 데이터 흐름 요약

```
  SM에서 메모리 요청 (READ_REQUEST) 전송 전체 흐름:

  1. SM → icnt_push(sm_id, mem_id, mem_fetch*, size)
  2. InterconnectInterface::Push()
     - deviceID → icntID 변환
     - size → n_flits 변환 (예: 4 flits)
     - subnet 0 (REQ_NET) 선택
     - GPUTrafficManager::_GeneratePacket()
       → head/body/tail flit 생성 → _input_queue에 저장

  3. icnt_transfer() → _Step()
     Phase 2 (Injection):
     - _input_queue에서 flit 꺼냄
     - head flit: VC 할당, lookahead routing
     - Network::WriteFlit() → inject 채널로 전송
     Phase 3:
     - 라우터들이 flit을 처리 (RC→VA→SA→ST)
     - 채널을 통해 다음 라우터로 이동
     - 홉마다 반복

  4. 목적지 라우터에서 eject 채널로 출력
     Phase 1 (Ejection):
     - ReadFlit() → WriteOutBuffer()
     - Transfer2BoundaryBuffer()
     - tail flit 도착 시 패킷 완성

  5. Memory → icnt_pop(mem_id)
     - boundary_buffer에서 완성된 패킷 추출
     - 원본 mem_fetch* 반환
```

---

## 14. BookSim2 vs Local Crossbar 비교

| 특성 | BookSim2 (INTERSIM) | Local Crossbar (LOCAL_XBAR) |
|------|--------------------|-----------------------------|
| 정확도 | 사이클 정확, 상세 NoC 모델 | 단순화된 모델 |
| 시뮬레이션 속도 | 느림 | 빠름 |
| Flit 분할 | 패킷을 여러 flit으로 분할 | flit 분할 없음 (패킷 단위) |
| 토폴로지 | Mesh, Torus, CMesh, FatTree 등 | 단순 크로스바 |
| 라우팅 | DOR, XY-YX, 적응적 라우팅 | 직접 연결 (라우팅 불필요) |
| Virtual Channel | 다수 VC 지원, 크레딧 흐름 제어 | VC 없음 |
| 중재 | VC allocator + Switch allocator | RR 또는 iSLIP |
| 채널 지연 | 물리적 와이어 지연 모델링 | 1사이클 (출력 버퍼 대기만) |
| 통계 | 상세 레이턴시/처리량/홉 통계 | 충돌/버퍼 사용률 통계 |
| 적합한 용도 | 네트워크 연구, 정확한 성능 분석 | 빠른 시뮬레이션, 네트워크가 비핵심일 때 |

---

## 15. 주요 소스 파일 요약

| 파일 | 위치 | 역할 |
|------|------|------|
| `icnt_wrapper.h/cc` | `src/gpgpu-sim/` | 인터커넥트 함수 포인터 인터페이스, 모드 선택 |
| `local_interconnect.h/cc` | `src/gpgpu-sim/` | Local Crossbar 구현 (xbar_router, iSLIP/RR) |
| `interconnect_interface.hpp/cpp` | `src/intersim2/` | BookSim2 ↔ GPGPU-Sim 브릿지 |
| `gputrafficmanager.cpp` | `src/intersim2/` | GPU 트래픽 관리 (_Step, _GeneratePacket, _RetireFlit) |
| `network.cpp` | `src/intersim2/networks/` | Network 기본 클래스, 토폴로지 팩토리 |
| `cmesh.cpp` | `src/intersim2/networks/` | Concentrated Mesh 토폴로지 및 라우팅 함수 |
| `iq_router.cpp` | `src/intersim2/routers/` | Input-Queued Router (RC→VA→SA→ST 파이프라인) |
| `flit.cpp` | `src/intersim2/` | Flit 구조체, 오브젝트 풀 관리 |
| `flitchannel.cpp` | `src/intersim2/` | FlitChannel (지연 모델링 FIFO) |
| `buffer_state.cpp` | `src/intersim2/` | 크레딧 기반 흐름 제어, 버퍼 정책 |
| `routefunc.cpp` | `src/intersim2/` | 라우팅 함수 등록, VC 범위 초기화 |
