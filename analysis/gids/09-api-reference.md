# GIDS API 레퍼런스

## 1. Python API (GIDS_Setup/GIDS/GIDS.py)

### 1.1 GIDS 클래스

```python
class GIDS:
    """GPU-Initiated Direct Storage Dataloader"""

    def __init__(self,
        page_size=4096,          # 페이지 크기 (바이트 단위가 아닌 ×512B)
                                 # 실제 바이트 = page_size × 512
                                 # 기본값 4096 → 사용 시 보통 8 (=4KB)
        off=0,                   # SSD 내 feature 시작 오프셋 (LBA)
        cache_dim=1024,          # feature 벡터 차원 (BaM array 접근용)
        num_ele=300*1000*1000*1024,  # 논리 주소 공간 크기
        num_ssd=1,               # SSD 개수
        ssd_list=None,           # SSD 인덱스 리스트 (예: [0,1,2,3])
        cache_size=10,           # GPU Page Cache 크기 (GB)
        ctrl_idx=0,              # CUDA 디바이스 인덱스
        window_buffer=False,     # Window Buffering 활성화
        wb_size=8,               # Window Buffer 배치 수
        accumulator_flag=False,  # Storage Access Accumulator 활성화
        long_type=False,         # True: int64_t, False: float
        heterograph=False,       # 이종 그래프 모드
        heterograph_map=None     # 노드 타입→SSD 오프셋 맵
                                 # 예: {'paper':0, 'author':269M}
    )
```

### 1.2 GIDS 메서드

```python
# ── CPU Buffer ──
def cpu_backing_buffer(dim: int, length: int) -> None
    """CPU 피닝 메모리 할당
    dim: feature 차원 (1024)
    length: 노드 수
    총 크기: length × dim × sizeof(float)"""

def set_cpu_buffer(ten: torch.Tensor, N: int) -> None
    """PageRank 상위 노드를 CPU 버퍼에 로드
    ten: 노드 인덱스 텐서 (int64)
    N: 노드 수"""

# ── Window Buffering ──
def window_buffering(batch) -> None
    """배치의 노드에 대해 페이지 프리페치 힌트 설정"""

def fill_wb(it, num: int) -> None
    """초기 윈도우 버퍼를 num개 배치로 채움"""

# ── Storage Accumulator ──
def set_required_storage_access(
    bw: float,          # SSD 피크 대역폭 (GB/s)
    l_ssd: float,       # SSD 지연 (μs)
    l_system: float,    # 시스템 지연 (μs)
    num_ssd: int,       # SSD 수
    p: float            # 캐시 미스 확률
) -> None
    """대역폭 포화에 필요한 최소 I/O 수 계산"""

# ── Feature Fetching (핵심) ──
def fetch_feature(
    dim: int,                    # feature 차원
    it: Iterator,                # DataLoader 이터레이터
    device: torch.device         # 타겟 디바이스
) -> Tuple[input_nodes, seeds, blocks, features]
    """다음 배치를 sampling하고 feature를 GPU로 읽음
    반환: (input_nodes, seeds, blocks, feature_tensor)"""

# ── 데이터 I/O ──
def store_tensor(in_ten: torch.Tensor, offset: int) -> None
    """텐서를 SSD에 기록 (초기 데이터 저장용)"""

def store_mmap_tensor(in_ten: torch.Tensor, offset: int) -> None
    """mmap된 텐서를 SSD에 기록"""

def read_tensor(num: int, offset: int) -> None
    """SSD에서 텐서 읽기 (디버그용)"""

def flush_cache() -> None
    """GPU Page Cache를 SSD로 플러시"""

# ── 통계 ──
def print_stats() -> None
    """GIDS 성능 통계 출력"""
```

### 1.3 GIDS_DGLDataLoader 클래스

```python
class GIDS_DGLDataLoader(torch.utils.data.DataLoader):
    """DGL sampling + GIDS feature fetching 통합 DataLoader"""

    def __init__(self,
        graph: dgl.DGLGraph,          # DGL 그래프
        indices,                       # 학습 노드 인덱스
                                       # Homo: Tensor, Hetero: dict
        graph_sampler,                 # DGL BlockSampler
        batch_size: int,               # 배치 크기
        dim: int,                      # feature 차원
        GIDS: GIDS,                    # GIDS 인스턴스
        device: torch.device,          # 타겟 디바이스
        use_ddp: bool = False,         # DistributedDataParallel
        ddp_seed: int = 0,             # DDP 시드
        drop_last: bool = False,       # 마지막 불완전 배치 버림
        shuffle: bool = True,          # 셔플
        use_alternate_streams: bool = False  # 대체 스트림 사용
    )

    def __iter__(self) -> _PrefetchingIter
        """GIDS feature fetching이 통합된 이터레이터 반환"""

    def print_stats(self) -> None
        """GIDS 통계 출력"""

    def print_timer(self) -> None
        """타이머 리셋"""
```

---

## 2. C++/CUDA API (gids_module/)

### 2.1 GIDS_Controllers

```cpp
// bam_nvme.h:27-41
struct GIDS_Controllers {
    // 멤버
    const char *const ctrls_paths[6];  // /dev/libnvm0~5
    std::vector<Controller*> ctrls;
    uint32_t n_ctrls = 1;
    uint64_t queueDepth = 1024;
    uint64_t numQueues = 128;
    uint32_t cudaDevice = 0;
    uint32_t nvmNamespace = 1;

    // 메서드
    void init_GIDS_controllers(
        uint32_t num_ctrls,
        uint64_t q_depth,
        uint64_t num_q,
        const std::vector<int>& ssd_list
    );
};
```

### 2.2 BAM_Feature_Store\<TYPE\>

```cpp
// bam_nvme.h:52-122 + gids_nvme.cu
template<typename TYPE>
class BAM_Feature_Store {
    // 기본 설정
    uint32_t pageSize = 4096;
    uint64_t numElems = 300LL*1000*1000*1024;
    uint32_t blkSize = 128;         // CUDA block size

    // BaM 핵심 객체
    page_cache_t *h_pc;             // GPU Page Cache
    range_t<TYPE> *h_range;         // 논리→물리 매핑
    array_t<TYPE> *a;               // GPU 가상 배열

    // CPU 버퍼
    GIDS_CPU_buffer<TYPE> CPU_buffer;
    bool cpu_buffer_flag = false;

    // 통계
    float kernel_time = 0;
    uint64_t total_access = 0;
    unsigned int *d_cpu_access;     // GPU device counter

    // ── 초기화 ──
    void init_controllers(
        GIDS_Controllers ctrl,
        uint32_t ps,            // page size
        uint64_t read_off,      // read offset
        uint64_t cache_size,    // cache size (GB)
        uint64_t num_ele,       // total elements
        uint64_t num_ssd        // number of SSDs
    );

    void cpu_backing_buffer(uint64_t dim, uint64_t len);
    void set_cpu_buffer(uint64_t idx_buffer, int num);

    // ── Feature Read ──
    void read_feature(
        uint64_t i_ptr,          // 출력 텐서 GPU 포인터
        uint64_t i_index_ptr,    // 인덱스 GPU 포인터
        int64_t num_index,       // 노드 수
        int dim,                 // feature 차원
        int cache_dim,           // 캐시 차원
        uint64_t key_off         // 노드 타입 오프셋
    );

    void read_feature_hetero(
        int num_iter,                        // 노드 타입 수
        std::vector<uint64_t> i_ptr_list,    // 출력 포인터 리스트
        std::vector<uint64_t> i_index_list,  // 인덱스 포인터 리스트
        std::vector<int64_t> num_index,      // 각 타입의 노드 수
        int dim, int cache_dim,
        std::vector<uint64_t> key_off        // 각 타입의 오프셋
    );

    void read_feature_merged(
        int num_iter,                        // 누적 배치 수
        std::vector<uint64_t> i_ptr_list,
        std::vector<uint64_t> i_index_list,
        std::vector<int64_t> num_index,
        int dim, int cache_dim
    );

    void read_feature_merged_hetero(
        int num_iter,
        std::vector<uint64_t> i_ptr_list,
        std::vector<uint64_t> i_index_list,
        std::vector<int64_t> num_index,
        int dim, int cache_dim,
        std::vector<uint64_t> key_off
    );

    // ── 쓰기 ──
    void store_tensor(uint64_t tensor_ptr, uint64_t num, uint64_t offset);
    void flush_cache();

    // ── Window Buffering ──
    void set_window_buffering(uint64_t id_idx, int64_t num_pages, int hash_off);

    // ── 유틸리티 ──
    void set_offsets(uint64_t in_off, uint64_t index_off, uint64_t data_off);
    uint64_t get_offset_array();
    uint64_t get_array_ptr();
    void print_stats();
    void print_stats_no_ctrl();
    unsigned int get_cpu_access_count();
    void flush_cpu_access_count();
};
```

### 2.3 CUDA 커널

```cpp
// gids_kernel.cu

// 기본 Feature 읽기 (CPU 버퍼 없음)
template<typename T>
__global__ __launch_bounds__(64, 32)
void read_feature_kernel(
    array_d_t<T> *dr,         // BaM device array
    T *out_tensor_ptr,         // 출력 버퍼 (GPU)
    int64_t *index_ptr,        // 노드 인덱스 (GPU)
    int dim,                   // feature 차원
    int64_t num_idx,           // 총 노드 수
    int cache_dim,             // 캐시 차원
    uint64_t key_off           // 노드 타입 오프셋
);

// CPU 버퍼 포함 Feature 읽기
template<typename T>
__global__ __launch_bounds__(64, 32)
void read_feature_kernel_with_cpu_backing_memory(
    array_d_t<T> *dr,
    T *out_tensor_ptr,
    int64_t *index_ptr,
    int dim,
    int64_t num_idx,
    int cache_dim,
    uint64_t key_off,
    range_d_t<T> *range,       // CPU 버퍼 매핑 정보
    GIDS_CPU_buffer<T> CPU_buffer,  // CPU 버퍼 구조체
    bool cpu_seq,              // 순차 플래그
    unsigned int* d_cpu_access // CPU 접근 카운터
);

// CPU 버퍼 메타데이터 설정
template<typename T>
__global__
void set_cpu_buffer_kernel(
    range_d_t<T> *d_range,
    int64_t *idx_ptr,
    int64_t num
);

// CPU 버퍼 데이터 복사 (SSD → CPU)
template<typename T>
__global__
void set_cpu_buffer_data_kernel(
    array_d_t<T> *dr,
    T* cpu_buffer,
    int64_t *idx_ptr,
    int dim,
    int num
);

// Window Buffering 프리페치 힌트
template<typename T>
__global__
void set_window_buffering_kernel(
    array_d_t<T> *dr,
    int64_t id_idx,
    int64_t num_pages,
    int hash_off
);

// Feature 쓰기
template<typename T>
__global__
void write_feature_kernel2(
    array_d_t<T> *dr,
    T* in_tensor,
    int dim,
    int num
);
```

### 2.4 PyBind11 바인딩

```cpp
// gids_nvme.cu:518-584

PYBIND11_MODULE(BAM_Feature_Store, m) {
    // float 타입
    py::class_<BAM_Feature_Store<float>>(m, "BAM_Feature_Store_float")
        .def(py::init<>())
        .def("init_controllers", &BAM_Feature_Store<float>::init_controllers)
        .def("read_feature", &BAM_Feature_Store<float>::read_feature)
        .def("read_feature_hetero", &BAM_Feature_Store<float>::read_feature_hetero)
        .def("read_feature_merged", &BAM_Feature_Store<float>::read_feature_merged)
        .def("read_feature_merged_hetero", ...)
        .def("store_tensor", ...)
        .def("flush_cache", ...)
        .def("cpu_backing_buffer", ...)
        .def("set_cpu_buffer", ...)
        .def("set_window_buffering", ...)
        .def("print_stats", ...)
        .def("print_stats_no_ctrl", ...)
        .def("set_offsets", ...)
        .def("get_offset_array", ...)
        .def("get_array_ptr", ...)
        .def("get_cpu_access_count", ...)
        .def("flush_cpu_access_count", ...);

    // int64_t 타입
    py::class_<BAM_Feature_Store<int64_t>>(m, "BAM_Feature_Store_long")
        // (동일한 메서드들)

    // Controllers
    py::class_<GIDS_Controllers>(m, "GIDS_Controllers")
        .def(py::init<>())
        .def("init_GIDS_controllers", ...);
}
```

---

## 3. 주요 파라미터 기본값 및 권장값

```
┌─────────────────────┬──────────────┬─────────────────────────────────┐
│ 파라미터             │ 기본값        │ 설명 / 권장                     │
├─────────────────────┼──────────────┼─────────────────────────────────┤
│ page_size           │ 8            │ ×512B = 4KB (NVMe 최소 단위)    │
│ cache_size          │ 8 (GB)       │ GPU VRAM 여유에 따라 조절        │
│ cache_dim           │ 1024         │ = emb_size (feature 차원)        │
│ queueDepth          │ 1024         │ SSD QD, 높을수록 대역폭↑        │
│ numQueues           │ 128          │ 병렬 큐 수                       │
│ blkSize (CUDA)      │ 128          │ 4 warps per block               │
│ batch_size          │ 1024         │ GNN 미니배치 크기                │
│ fan_out             │ "10,15"      │ 레이어별 이웃 수                 │
│ cpu_buffer_percent  │ 0.2          │ 전체 노드의 20%                  │
│ wb_size             │ 6-8          │ 프리페치 배치 수                 │
│ bw (accumulator)    │ 5.8 GB/s     │ SSD 피크 대역폭                  │
│ l_ssd               │ 11.0 μs      │ SSD 지연                         │
│ l_system            │ 20.0 μs      │ 시스템 지연                      │
└─────────────────────┴──────────────┴─────────────────────────────────┘
```
