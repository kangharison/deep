# BaM (Big Accelerator Memory) 코드 구조 및 역할 분석

## 프로젝트 개요

BaM은 **GPU가 CPU 개입 없이 직접 NVMe SSD에 접근**할 수 있도록 하는 연구 시스템이다. ASPLOS'23에 발표된 논문 "GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture"의 구현체이며, PCIe P2P(Peer-to-Peer)를 통해 GPU에서 NVMe 디바이스로 직접 I/O 명령을 발행한다.

**핵심 아이디어**: 기존 Linux 스토리지 스택(syscall → VFS → 파일시스템 → Block Layer → NVMe 드라이버)을 우회하고, GPU 커널 스레드가 직접 NVMe Submission Queue에 명령을 넣고 Completion Queue를 폴링하는 구조이다.

## 하드웨어 요구사항

- x86 시스템 (PCIe P2P 지원 필수)
- NVMe SSD (다중 SSD 지원)
- NVIDIA Volta 이상 GPU (V100/A100/H100 — P2P를 위해 전체 메모리 노출 필요)
- BIOS: Above 4G Decoding 활성화, IOMMU 비활성화, ACS 비활성화
- Linux 커널 5.8.x, CUDA 12.3+, GCC 5.4.0+

## 디렉토리 구조 전체 맵

```
sources/bam/
├── CMakeLists.txt          ← 빌드 시스템 (CUDA/커널모듈/벤치마크 통합)
├── README.md               ← 프로젝트 소개
├── CONTRIBUTING.md         ← 기여 가이드
├── LICENSE
├── asplosaoe/              ← ASPLOS Artifact Evaluation 문서
│   └── README.md
├── include/                ← 공개 API 헤더 (21개 파일)
│   ├── nvm_types.h         ← 핵심 타입 정의
│   ├── nvm_queue.h         ← 큐 조작 API
│   ├── nvm_parallel_queue.h ← GPU 병렬 큐 프리미티브
│   ├── nvm_cmd.h           ← NVMe 명령 빌더
│   ├── nvm_dma.h           ← DMA 매핑
│   ├── nvm_admin.h         ← Admin 큐 명령
│   ├── nvm_ctrl.h          ← 컨트롤러 초기화
│   ├── page_cache.h        ← 소프트웨어 캐시 (2143줄, 핵심)
│   ├── ctrl.h              ← Controller C++ 래퍼
│   ├── queue.h             ← QueuePair C++ 래퍼
│   ├── buffer.h            ← 메모리 할당 헬퍼
│   └── ...
├── src/                    ← 코어 라이브러리 구현
│   ├── admin.cpp           ← Admin 명령 구현
│   ├── ctrl.cpp            ← 컨트롤러 핸들 관리
│   ├── dma.cpp             ← DMA 매핑 구현
│   ├── queue.cpp           ← 큐 리셋/폴링
│   ├── rpc.cpp             ← 멀티프로세스 Admin 큐 조율
│   ├── error.cpp           ← 에러 처리
│   ├── linux/              ← Linux 전용 디바이스/DMA
│   │   ├── device.cpp      ← ioctl/BAR 매핑
│   │   └── dma.cpp         ← Linux DMA
│   └── dis/                ← SmartIO/클러스터 지원 (선택)
│       ├── device.c
│       ├── dma.c
│       └── rpc.c
├── module/                 ← Linux 커널 모듈 (7개 파일)
│   ├── ctrl.c              ← 디바이스 probe, /dev/libnvm* 생성
│   ├── pci.c               ← PCI 열거, BAR 매핑
│   ├── map.c               ← 가상→물리 주소 변환
│   └── Makefile.in
├── benchmarks/             ← 벤치마크/애플리케이션 (16개 서브디렉토리)
│   ├── block/              ← fio 유사 raw I/O 벤치마크
│   ├── array/              ← 캐시+SSD 미스 테스트
│   ├── cache/              ← 캐시 동작 분석
│   ├── bfs/                ← 너비 우선 탐색 (그래프)
│   ├── cc/                 ← 연결 요소
│   ├── sssp/               ← 최단 경로
│   ├── pagerank/           ← 페이지랭크
│   └── ...
├── scripts/                ← 실행 스크립트 (15개)
└── deprecated/             ← 레거시 코드 (FIO 플러그인 등)
```

## 아키텍처 계층도

```
┌─────────────────────────────────────────────────┐
│            Application Layer (Benchmarks)        │
│         BFS, SSSP, PageRank, CC, Block I/O       │
├─────────────────────────────────────────────────┤
│           C++ Wrapper Layer (include/)            │
│   Controller │ QueuePair │ DmaPtr │ page_cache    │
├─────────────────────────────────────────────────┤
│          Core NVM Library (include/ + src/)       │
│  Queue API │ Command Builder │ Admin │ DMA Map    │
│  nvm_parallel_queue: device-scope atomics         │
├─────────────────────────────────────────────────┤
│         Linux Kernel Module (module/)             │
│  /dev/libnvm* │ BAR 매핑 │ VA→PA 변환 │ P2P      │
├─────────────────────────────────────────────────┤
│               Hardware                            │
│   GPU ←──PCIe P2P──→ NVMe SSD                    │
│   Admin Queue │ I/O Queue │ Doorbell Register     │
└─────────────────────────────────────────────────┘
```

## 핵심 컴포넌트 상세 분석

### 1. 커널 모듈 (module/)

기본 Linux NVMe 드라이버를 대체하는 커스텀 커널 모듈이다.

**역할:**
- NVMe 디바이스를 기본 드라이버에서 언바인드
- `/dev/libnvm0`, `/dev/libnvm1` 등 캐릭터 디바이스 생성
- PCIe BAR0 레지스터를 유저스페이스에 매핑
- 가상 주소 → 물리/IO 주소 변환 (GPU P2P DMA용)

**주요 파일:**
- `ctrl.c`: probe/remove 콜백, 디바이스 파일 관리
- `pci.c`: PCI 디바이스 열거, BAR 리소스 매핑
- `map.c`: `get_user_pages()`로 유저 페이지를 잠그고 물리 주소 추출

### 2. 핵심 타입 정의 (include/nvm_types.h)

```c
// NVMe 컨트롤러 핸들
typedef struct nvm_ctrl {
    uint32_t page_size;      // 컨트롤러 페이지 크기
    uint32_t max_queues;     // 최대 큐 수
    void*    mm_ptr;         // BAR0 매핑 포인터
} nvm_ctrl_t;

// NVMe 큐 (SQ/CQ 공용)
typedef struct nvm_queue {
    uint16_t head, tail;     // 큐 포인터
    uint32_t phase;          // Phase 태그 (wrap 감지)
    void*    db;             // 도어벨 레지스터 포인터
    // GPU용 추가 필드:
    uint32_t tickets;        // 티켓 기반 CID 할당 (atomic)
    uint64_t tail_copy;      // 꼬리 이동 추적
} nvm_queue_t;

// NVMe 명령 (64바이트, NVMe 스펙 준수)
typedef struct nvm_cmd {
    uint32_t dword[16];      // 16개 DWORD
} nvm_cmd_t;

// NVMe 완료 엔트리 (16바이트)
typedef struct nvm_cpl {
    uint32_t dword[4];       // 4개 DWORD
} nvm_cpl_t;
```

### 3. 큐 조작 API (include/nvm_queue.h)

NVMe 스펙의 Submission Queue / Completion Queue를 직접 조작하는 함수들이다.

```c
// Submission Queue에 명령 삽입
nvm_cmd_t* nvm_sq_enqueue(nvm_queue_t* sq);

// 도어벨 레지스터를 써서 컨트롤러에 통지
void nvm_sq_submit(nvm_queue_t* sq);

// Completion Queue에서 완료 엔트리 꺼내기
nvm_cpl_t* nvm_cq_dequeue(nvm_queue_t* cq);

// 완료 처리 후 도어벨 업데이트
void nvm_cq_update(nvm_queue_t* cq);
```

### 4. GPU 병렬 큐 프리미티브 (include/nvm_parallel_queue.h, 543줄)

BaM의 **가장 핵심적인 혁신** 중 하나이다. 수천 개의 GPU 스레드가 동시에 NVMe 큐를 조작할 수 있도록 lock-free 알고리즘을 구현한다.

**핵심 함수:**
```cuda
// Command ID를 원자적으로 할당 (GPU 스레드 간 충돌 방지)
__device__ uint16_t get_cid(nvm_queue_t* sq);

// 사용 완료된 CID 반환
__device__ void put_cid(nvm_queue_t* sq, uint16_t cid);

// SQ tail을 원자적으로 이동 (여러 스레드가 동시에 명령 삽입)
__device__ void move_tail(nvm_queue_t* sq);
```

**설계 원리:**
- `atomicAdd()`로 device-scope 원자 연산 사용
- 티켓 기반 CID 재사용으로 충돌 방지
- `__threadfence()`로 메모리 순서 보장
- 스레드 배리어 없이 큐 제출 가능 (확장성 핵심)

### 5. 소프트웨어 캐시 (include/page_cache.h, 2143줄)

GPU 메모리에 NVMe 데이터를 캐싱하는 소프트웨어 캐시 계층이다. BaM 성능의 핵심이다.

**캐시 상태 머신:**
```
INVALID → BUSY (fetch 중) → VALID (사용 가능)
                                ↓
                            DIRTY (수정됨) → BUSY (writeback 중) → VALID
```

**주요 기능:**
- 라인 기반 캐시 (512B~4KB 설정 가능)
- 캐시 미스 시 자동으로 NVMe에서 fetch
- 레퍼런스 카운팅으로 동시 접근 관리
- I/O 요청 합병(coalescing)으로 증폭 최소화
- 다중 컨트롤러 스트라이핑/복제 지원

**사용 예시 (GPU 커널 내에서):**
```cuda
__global__ void kernel(array_d_t<uint64_t>* arr) {
    uint64_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    uint64_t value = (*arr)[idx];  // operator[]가 캐시/SSD 로직 처리
    // 캐시 히트: GPU 메모리에서 바로 반환
    // 캐시 미스: NVMe에서 fetch → 캐시에 저장 → 반환
}
```

### 6. DMA 매핑 (src/dma.cpp + include/nvm_dma.h)

호스트/GPU 메모리를 NVMe 컨트롤러가 접근할 수 있는 물리 주소로 매핑한다.

```c
// 호스트 메모리 DMA 매핑
int nvm_dma_map_host(nvm_dma_t** handle, nvm_ctrl_t* ctrl, void* vaddr, size_t size);

// GPU 디바이스 메모리 DMA 매핑 (P2P)
int nvm_dma_map_device(nvm_dma_t** handle, nvm_ctrl_t* ctrl, int cudaDevice);
```

GPU P2P DMA의 경우, GPU 메모리의 물리 주소를 NVMe 컨트롤러에 PRP(Physical Region Page) 엔트리로 전달하여 CPU 바운스 버퍼 없이 직접 전송한다.

### 7. Admin 명령 (src/admin.cpp + include/nvm_admin.h)

NVMe 컨트롤러 관리를 위한 Admin 명령 구현이다.

```c
// 컨트롤러 정보 조회
int nvm_admin_identify_controller(nvm_aq_ref aq, void* buffer);

// 네임스페이스 정보 조회
int nvm_admin_identify_namespace(nvm_aq_ref aq, uint32_t nsid, void* buffer);

// I/O 큐 생성
int nvm_admin_create_cq(nvm_aq_ref aq, nvm_queue_t* cq, ...);
int nvm_admin_create_sq(nvm_aq_ref aq, nvm_queue_t* sq, ...);

// 큐 개수 요청
int nvm_admin_request_queues(nvm_aq_ref aq, uint16_t* n_sqs, uint16_t* n_cqs);
```

### 8. RPC 메커니즘 (src/rpc.cpp)

Admin 큐는 하나의 프로세스만 소유할 수 있으므로, 다른 프로세스가 큐 생성/삭제를 요청할 때 RPC를 사용한다.

- 공유 메모리 기반 IPC (호스트)
- SISCI 공유 메모리 (DIS 클러스터)
- 뮤텍스로 큐 상태 보호

## 데이터 흐름: GPU → NVMe 읽기 과정

```
1. GPU 커널 스레드가 데이터 필요
        ↓
2. page_cache에서 캐시 조회
        ↓ (미스)
3. get_cid()로 Command ID 할당 (atomic)
        ↓
4. nvm_sq_enqueue()로 SQ 슬롯 확보
        ↓
5. nvm_cmd_header() + nvm_cmd_rw_blks()로 READ 명령 구성
        ↓
6. nvm_cmd_data_ptr()로 GPU 메모리 PRP 주소 설정
        ↓
7. nvm_sq_submit()으로 도어벨 링
        ↓
8. NVMe 컨트롤러가 PCIe P2P로 GPU 메모리에 직접 데이터 전송
        ↓
9. GPU 스레드가 CQ 폴링하여 완료 확인
        ↓
10. 캐시에 데이터 등록, 스레드에 반환
```

## 다중 SSD 지원

```
GPU
 ├── 스레드 그룹 0~31   → SSD 0 (큐 0~N)
 ├── 스레드 그룹 32~63  → SSD 1 (큐 0~N)
 └── 스레드 그룹 64~95  → SSD 2 (큐 0~N)
```

- 라운드 로빈 방식으로 32 스레드 단위 큐 할당
- 스트라이핑: 데이터를 여러 SSD에 분산 저장
- 복제: 동일 데이터를 여러 SSD에 복사하여 읽기 대역폭 극대화

## 벤치마크 구성

### 마이크로벤치마크
| 벤치마크 | 측정 대상 |
|----------|-----------|
| `block/` | raw I/O 처리량 (fio 유사) |
| `cache/` | 캐시 히트/미스 동작 |
| `pattern/` | 순차/랜덤/혼합 접근 패턴 |
| `iodepth-block/` | 큐 깊이에 따른 성능 변화 |
| `readwrite/` | 대용량 순차 읽기/쓰기 |

### 애플리케이션 벤치마크
| 벤치마크 | 알고리즘 |
|----------|----------|
| `bfs/` | 너비 우선 탐색 (그래프 분석) |
| `cc/` | 연결 요소 찾기 |
| `sssp/` | 단일 출발점 최단 경로 |
| `pagerank/` | 페이지랭크 |

각 벤치마크는 `main.cu` (CUDA 커널), `settings.h` (인자 파서), `CMakeLists.txt`로 구성된다.

## 빌드 시스템

```bash
mkdir build && cd build
cmake .. -DNVIDIA=/path/to/nvidia/headers
make libnvm           # 코어 라이브러리 (libnvm.so)
make benchmarks       # 전체 벤치마크 빌드
make kernel_module    # 커널 모듈 빌드 (별도)
```

**CMake 옵션:**
- `no_cuda`: CUDA 지원 제외
- `no_smartio`: DIS/클러스터 지원 제외
- `no_module`: 커널 모듈 스킵
- `nvidia_archs`: GPU 컴퓨트 캐퍼빌리티 (기본: 70, 80, 90 = V100, A100, H100)

## Linux NVMe 드라이버와의 비교

| 항목 | Linux NVMe | BaM |
|------|-----------|-----|
| I/O 발행자 | CPU (커널) | GPU (유저스페이스) |
| 경로 | syscall → VFS → FS → Block → NVMe | GPU 커널 → NVMe 큐 직접 |
| 큐 관리 | blk-mq (CPU 코어당) | GPU 스레드 그룹당 |
| 캐시 | Page Cache (CPU 메모리) | page_cache (GPU 메모리) |
| 데이터 전송 | DMA (CPU 메모리 경유) | PCIe P2P (GPU↔SSD 직접) |
| 스케줄러 | I/O 스케줄러 | 없음 (직접 제출) |
| CPU 개입 | 필수 | 불필요 |

## 파일 수 요약

| 디렉토리 | 파일 수 | 유형 |
|-----------|---------|------|
| include/ | 21 | 헤더 |
| src/ | 14 | C/C++ |
| src/linux/ | 4 | Linux 전용 |
| src/dis/ | 7 | 클러스터/SmartIO |
| module/ | 7 | 커널 모듈 |
| benchmarks/ | 16개 디렉토리 | CUDA/애플리케이션 |
| scripts/ | 15 | 셸 스크립트 |
| **합계** | **~110** | **코드 + 스크립트** |

## 핵심 혁신 요약

1. **GPU-Direct I/O**: CPU를 스토리지 경로에서 완전히 제거 (PCIe P2P)
2. **Lock-free 병렬 큐**: Device-scope atomics로 수천 GPU 스레드가 동시에 NVMe 큐 조작
3. **소프트웨어 캐시**: GPU 메모리에 NVMe 데이터 캐싱, I/O 합병으로 증폭 최소화
4. **다중 SSD 확장**: 스트라이핑/복제로 대역폭 선형 확장
5. **유연한 백엔드**: Linux + SmartIO/DIS + 커스텀 확장 가능
