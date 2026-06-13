# GIDS 벤치마크 워크로드 특징 분석

> 대상: GIDS 저장소(`gids-study/`)의 `evaluation/` 스크립트에서 사용되는 모든 데이터셋·모델·샘플러.
> 참조 문서: [[gids-complete-architecture]] (시스템 아키텍처 전체)
> 코드 위치: `gids-study/evaluation/` (학습 스크립트), `gids-study/evaluation/dataloader.py` (데이터셋 로더)

---

## 1. 개요

GIDS가 해결하려는 문제는 **GNN 미니배치 학습에서의 Feature I/O 병목**이다. 전통적인 방식은
`SSD → CPU mmap → Host DRAM → PCIe → GPU` 경로로 특성 텐서를 옮기는데, 테라바이트 규모의
그래프에서는 이 경로가 GPU 연산 대비 압도적으로 느려 GPU 가동률이 낮아진다.

GIDS는 GPU가 직접 NVMe SSD에서 특성 텐서를 읽어오는 **GPU-Initiated Direct Storage** 경로로
이 병목을 우회한다. 이 분석 문서는 GIDS가 검증에 사용하는 워크로드들의 구체적 특징을 파악해,
어떤 조건에서 GIDS의 최적화가 유효한지 이해하는 데 목적이 있다.

### 1.1 워크로드 분류

```
GIDS 벤치마크 워크로드
│
├── 데이터셋
│   ├── IGB260M          (동종 그래프, 6개 크기 변형)
│   ├── IGBH             (이종 그래프, large/full = 6 ntype)
│   └── OGB-papers100M   (동종 그래프, 외부 표준 벤치마크)
│
├── 모델
│   ├── 동종: SAGE / GCN / GAT
│   └── 이종: RGCN / RSAGE / RGAT / RGNN(MLPerf)
│
└── 샘플러
    ├── NeighborSampler  (기본값, 학습 스크립트 전체)
    ├── ClusterGCN       (homogenous_train_ClusterGCN.py)
    ├── LADIES           (ladies_sampler.py, 중요도 기반)
    └── NHS / PoissonLADIES (ClusterGCN 스크립트의 변형)
```

---

## 2. 데이터셋 분석

### 2.1 IGB260M — 동종 그래프

IGB260M(Illinois Graph Benchmark 260M)은 GIDS 논문의 저자들이 설계한 **학술 인용 그래프**로,
paper 노드 간 cites 엣지 하나로만 구성된 동종 그래프(homogeneous graph)다. 실제 학술 논문
데이터를 기반으로 하며, GIDS 논문의 벤치마크를 위해 6가지 크기 변형을 제공한다.

#### 노드 수 및 특성 규격

| Size          | Paper 노드 수   | Feature 크기               | 라벨 파일 (19-class / 2983-class) |
|---------------|-----------------|----------------------------|------------------------------------|
| experimental  | 100,000         | (100K, 1024) × float32     | node_label_19.npy / node_label_2K.npy |
| small         | 1,000,000       | (1M, 1024) × float32       | 동일                               |
| medium        | 10,000,000      | (10M, 1024) × float32      | 동일                               |
| large         | 100,000,000     | (100M, 1024) × float32     | 동일                               |
| full          | **269,346,174** | (269M, 1024) × float32     | 19-class: 227,130,858개 라벨<br>2983-class: 157,675,969개 라벨 |
| OGB-papers100M | 111,059,956    | (111M, 1024) × float32     | OGB 표준 split (train.csv.gz 등)  |

- **Feature dim**: 1024 (float32) — 1 feature vector = 4096 bytes = **NVMe 1 페이지** (page_size=4096과 정확히 일치)
- **Feature 저장소 크기**:
  - full: 269,346,174 × 1024 × 4 bytes ≈ **1,035 GB (약 1 TB)**
  - large: 100,000,000 × 1024 × 4 bytes ≈ **384 GB**
  - medium: 10,000,000 × 1024 × 4 bytes ≈ **38 GB**
- **Graph 구조**: `paper__cites__paper` edge_index (.npy, shape (E,2) or CSC 변환본)
  - full 사이즈는 사전 변환된 CSC(Compressed Sparse Column) 포맷 (`edge_index_csc_row_idx.npy`, `edge_index_csc_col_idx.npy`)으로 로드

#### 라벨 분류 태스크

- **19-class**: IGB 자체 19개 분야 분류 (Kaggle IGB 챌린지 기준)
- **2983-class**: 세분화된 학술 분야 2983개 분류
- Train/Val/Test 분할: **60% / 20% / 20%** 랜덤 순열 (`torch.randperm`)
  - full 사이즈는 라벨이 있는 노드(227M 또는 157M) 범위에서만 test_mask 설정

#### 데이터 로딩 방식

```
in_memory=0 (기본): np.load(path, mmap_mode='r')
  → 파일을 가상 메모리에 매핑. 실제 페이지는 접근 시 OS가 fault로 로드.
  → GIDS 경로에서는 이 memmap이 SSD에 기록된 .npy를 직접 GPU가 읽음.

in_memory=1: np.load(path) / np.memmap(...).copy()
  → Host DRAM에 전체 복사. full 사이즈는 1TB DRAM 필요 → 비권장.

uva_graph=1: edge_index를 cudaHostAllocMapped로 pinned memory에 적재.
  → DGL NeighborSampler가 GPU에서 직접 graph 구조를 접근 가능.
```

---

### 2.2 IGBH — 이종 그래프 (IGB Heterogeneous)

IGBH는 IGB260M을 확장해 여러 노드 타입을 포함한 **이종 그래프(heterogeneous graph)**다.
논문·저자·연구분야·기관 등의 엔티티가 여러 관계로 연결되어 있어, GNN의 이종 그래프 처리 능력을
측정하는 데 적합하다.

#### 노드 타입 및 규모 (small/medium — 4 ntype)

```
paper        ──cites──▶       paper
paper        ──written_by──▶  author
author       ──affiliated_to──▶ institute
paper        ──topic──▶       fos (Field of Study)
(역방향 4개 추가 = 총 8 edge type)
```

#### 노드 타입 및 규모 (large/full — 6 ntype, Massive 버전)

```
위 4 ntype + journal + conference 추가
paper   ──published──▶ journal
paper   ──venue──▶     conference
(역방향 포함 총 12 edge type)
```

#### 각 크기별 노드 수

| Size  | paper        | author       | fos      | institute | 비고                    |
|-------|-------------|-------------|----------|-----------|-------------------------|
| small  | 소규모      | 소규모      | 소규모   | 소규모    | 정확한 수치 비공개      |
| medium | 소규모      | 소규모      | 소규모   | 소규모    | small보다 큼            |
| large  | 100,000,000 | 116,959,896 | 소규모   | 소규모    | + journal, conference   |
| **full** | **269,346,174** | **277,220,883** | ≈713,000 | ≈713,000 | + journal, conference |

#### GIDS heterograph_map (key_offset 테이블)

GIDS는 이종 그래프의 각 ntype feature를 SSD 내에 **연속적으로 배치**하고,
ntype별 시작 오프셋(byte 단위 아님, node index 단위)을 아래 테이블로 관리한다:

```python
# gids-study/evaluation/heterogeneous_train.py 내 key_offset 딕셔너리
heterograph_map = {
    'full': {
        'paper':     0,           # node index offset: 0
        'author':    269346174,   # paper 다음
        'fos':       546567057,   # author(277M개) 다음
        'institute': 547280017,   # fos(약 713K개) 다음
    }
}
```

- 총 노드 수 (4 ntype): ≈ 547,280,017 + institute 규모 ≈ **약 5억 5천만 노드**
- journal, conference 포함 시 Massive 버전에서 더 많음

#### IGBH Feature 저장소 크기

```
총 element 수 (run_BaM_IGBH.sh, run_GIDS_IGBH.sh):
  num_ele = 550 × 1,000,000 × 1024 = 563,200,000,000 elements

Feature 스토리지:
  563,200,000,000 × 4 bytes = 약 2,253 GB ≈ 2.25 TB

노드 수 역산:
  563,200,000,000 / 1024 features = 550,000,000 nodes (약 5.5억 노드)

페이지 레이아웃:
  page_size = 4096 bytes = 1 feature vector
  총 페이지 수: 2.25 TB / 4 KB = 약 5.9억 페이지
```

#### 분류 태스크

- 분류 대상: `paper` 노드 (author, fos, institute, journal, conference는 라벨 없음)
- num_classes: 2983 (full 기준, IGBH 전용 세분화 분류)
- Train/Val/Test: 60/20/20 (paper 노드 수 기준 랜덤 순열)

---

### 2.3 OGB-papers100M — 외부 표준 벤치마크

OGB(Open Graph Benchmark) `ogbn-papers100M`은 약 **1억 1천만 paper 노드**의 동종 인용 그래프다.
IGB260M과 구조는 유사하나 OGB의 공식 표준 split 파일(train/valid/test.csv.gz)을 사용한다는 점이 다르다.

| 항목        | 값                  |
|-------------|---------------------|
| 노드 수     | 111,059,956         |
| Feature dim | 1024 (float32)      |
| Feature 크기 | ≈ 428 GB            |
| Split       | OGB 공식 (랜덤 아닌 시간 기반) |
| Edge 파일   | edge_index.npy (shape (2,E)) |

---

## 3. GNN 모델 아키텍처 분석

### 3.1 동종 그래프 모델 (`evaluation/models.py`)

모든 동종 모델은 `in_feats=emb_size(1024)`, `hid_feats=256(기본)`, `num_classes=19(또는 2983)` 구성.

#### GraphSAGE (SAGE)

```
입력 (1024) → SAGEConv(mean) × 3층 → 출력 (num_classes)

각 층: h_v = ReLU( W_self × h_v + W_neigh × mean(h_u for u ∈ N(v)) )
집계 방식: mean aggregator (SAGEConv의 aggregator_type='mean')
드롭아웃: 입력 직전 (p=0.5, 기본값)

파라미터 수 (대략):
  Layer 0: 1024×256 + 1024×256 = 524,288 (self + neighbor)
  Layer 1: 256×256 + 256×256 = 131,072
  Layer 2: 256×num_classes
  총합: ~670K parameters
```

- **특징**: 이웃 평균 집계 → 2-hop 이웃 정보 3층에서 3-hop. 각 Layer마다 별도 W.
- **학습 이점**: simple하고 효과적. mini-batch 학습에서 가장 안정적.

#### GCN (Graph Convolutional Network)

```
입력 (1024) → GraphConv(normal) × 2층 → 출력 (num_classes)

각 층: h_v = σ( Σ_{u ∈ N(v)∪{v}} (1/√(deg_v × deg_u)) × W × h_u )
정규화: symmetric normalized adjacency (allow_zero_in_degree=True)
드롭아웃: 레이어 사이

파라미터 수 (대략):
  Layer 0: 1024×256
  Layer 1: 256×num_classes
  총합: ~266K parameters
```

- **특징**: 그래프 신호 처리 관점. degree 정규화로 과도한 집계 방지.
- **한계**: 2층 → fan_out [10, 15] = 최대 150개 이웃 feature 접근.

#### GAT (Graph Attention Network)

```
입력 (1024) → GATConv(4 heads) × 3층 → 출력 (num_classes)

각 층: h_v = concat( α_{vu} × W × h_u for u ∈ N(v) ) for each head
어텐션: e_{vu} = LeakyReLU( a^T [Wh_v || Wh_u] )
드롭아웃: 어텐션 가중치 및 피처 (p=0.5)

파라미터 수 (대략):
  Layer 0: 1024×(256/4) × 4 heads + attention params
  훨씬 많은 파라미터; ~2M parameters
```

- **특징**: 중요한 이웃에 높은 가중치 부여. 4 head → 다양한 관계 포착.
- **I/O 부담**: SAGE, GCN보다 feature 접근 패턴이 동일하지만 어텐션 계산 시 GPU 연산량 증가.

---

### 3.2 이종 그래프 모델 (`evaluation/models.py`, `mlperf_model.py`)

모든 이종 모델은 `DGL.HeteroGraphConv`를 사용해 **각 edge type마다 독립적인 컨볼루션**을 적용한다.
기본 설정: `num_layers=6`, `hidden=512`, `num_classes=2983` (IGBH full 기준).

#### 공통 구조

```python
HeteroGraphConv({
    'cites':         GraphConv/SAGEConv/GATConv,  # paper → paper
    'written_by':    GraphConv/SAGEConv/GATConv,  # paper → author
    'affiliated_to': GraphConv/SAGEConv/GATConv,  # author → institute
    'topic':         GraphConv/SAGEConv/GATConv,  # paper → fos
    ... (역방향 4개 포함 = 총 8 edge type)
}, aggregate='sum')
```

이종 그래프 컨볼루션 후 각 ntype별 출력을 sum/mean 집계 → 다음 layer 입력.
최종적으로 `paper` ntype의 출력만 분류에 사용 (분류 대상이 paper).

#### RGCN (Relational GCN)

```
각 edge type마다 GraphConv 적용
파라미터: edge type 수 × (in_feats × out_feats) = 8 × (1024×512) = 4M per layer
6 layers 기준 총 ~24M parameters (hetero 집계 weight 제외)
```

#### RSAGE (Relational GraphSAGE)

```
각 edge type마다 SAGEConv(mean) 적용
run_BaM_IGBH.sh / run_GIDS_IGBH.sh의 기본 모델 (--model_type rsage)
fan_out 3층 → num_layers=3
```

- **GIDS 논문 기본 평가 모델**: 실험 스크립트에서 가장 자주 사용됨.

#### RGAT (Relational GAT)

```
각 edge type마다 GATConv 적용 (4 heads)
가장 파라미터가 많고 어텐션 계산 overhead 가장 큼
```

#### RGNN (MLPerf, `mlperf_model.py`)

```
HeteroGraphConv + linear projection (ntype별 입력 차원 통일)
residual connection 포함
각 layer: Linear(emb → hidden) → HeteroConv → ReLU → Linear(hidden → hidden)
MLPerf GNN benchmark 표준 구현체
```

---

## 4. 샘플러 전략 분석

### 4.1 NeighborSampler (기본값)

DGL의 표준 이웃 샘플러. 각 layer마다 고정된 수의 이웃을 **균등 무작위**로 선택한다.

```
동종 모델 (GCN 2층): fan_out = [10, 15]
  → seed 1024개 × 10이웃 × 15이웃 = 최대 153,600 노드 (중복 포함)
  → 중복 제거 후 실제 feature fetch 대상: 통상 수만 노드

이종 모델 (RSAGE 3층): fan_out = [10, 5, 5]
  → seed 1024개 × 10 × 5 × 5 = 최대 256,000 노드 (중복 포함)
  → ntype별로 분리되어 각 BAM_Feature_Store_{float,long}.read_feature() 호출
```

**I/O 특성**:
- **완전 랜덤 접근(random access)**: 이웃 샘플은 node ID 공간 전체에 분산됨
- **공간적 지역성 없음**: 인접 batch 간 node ID 중복이 낮음 (power-law 분포에서 허브 노드 제외)
- **GIDS CPU 버퍼의 효과 극대화**: PageRank 상위 노드(허브)는 반복 등장 → pinned 메모리 히트

### 4.2 ClusterGCN (`homogenous_train_ClusterGCN.py`)

그래프를 미리 **클러스터(파티션)**로 분할하고, 각 mini-batch를 한 클러스터 내 노드들로 구성한다.

```python
# 동작 원리
# 1. Metis 등으로 graph를 num_parts 개 클러스터로 분할 (전처리)
# 2. 학습 시: batch_size개의 클러스터를 무작위 선택
# 3. 선택된 클러스터 내 노드들 = seed nodes (추가 이웃 샘플링 없음)
# 4. 클러스터 내부의 실제 edge만 사용 → 클러스터 간 edge는 drop

DGL API: dgl.dataloading.ClusterGCNSampler
batch_size_for_clusters: 클러스터 몇 개씩 묶을지 (기본 1)
```

**I/O 특성**:
- **공간적 지역성 높음**: 같은 클러스터 내 노드는 인접 → 인접 배치 간 node ID 중복 가능성 높음
- **feature access 크기**: 클러스터 크기 × feature_dim (고정, 예측 가능)
- **BaM 페이지 캐시 친화적**: 연속적인 클러스터가 비슷한 LBA 범위를 공유할 수 있음
- **단점**: 클러스터 경계 edge drop → 수렴 속도 저하 가능

```
homogenous_train_ClusterGCN.py의 SSD 설정:
  ssd_list = [5]  # /dev/libnvm5 하드코딩 (단일 SSD)
  num_queues = 32, queue_depth = 32
  page_size = 4096, cache_size = 4000 MB
```

### 4.3 LADIES (Layer-wise Adaptive Importance Sampling)

`ladies_sampler.py`의 `LadiesSampler` 클래스. 각 layer마다 **중요도 비례 확률**로 이웃을 선택한다.

```
알고리즘:
1. seed_nodes에서 모든 1-hop 이웃 후보 집합 생성 (inductive subgraph)
2. 각 후보의 중요도 계산: prob_v ∝ Σ_{u ∈ seed ∩ N(v)} 1/√(deg_u × deg_v)
3. multinomial(prob, num_neighbors_per_layer) 로 고정 수 샘플
4. 선택된 이웃으로 bipartite block(MFG) 구성, edge에 importance weight 부여

파라미터: nodes_per_layer = [S0, S1, ...] (layer별 최대 노드 수)
```

**I/O 특성**:
- **NeighborSampler보다 선택적**: 중요한 이웃만 선택 → 같은 정확도에서 fetch 노드 수 감소 가능
- **중요도 계산 overhead**: CPU에서 확률 계산 필요 → 배치 준비 시간 증가
- **LADIES vs NeighborSampler**: GIDS 논문은 LADIES가 I/O 효율 측면에서 유리할 수 있음을 시사

### 4.4 NHS / PoissonLADIES

`homogenous_train_ClusterGCN.py`에 존재하는 변형:

- **PoissonLADIES**: LADIES의 Poisson 샘플링 버전. 고정 수 대신 확률적 개수 샘플.
- **NHS(Node-level Hybrid Sampler)**: ClusterGCN + LADIES 혼합. 클러스터 내부는 full graph, 경계는 importance sampling.

---

## 5. 메모리 풋프린트 분석

### 5.1 Feature 스토리지 용량 (SSD 기록 데이터)

```
데이터셋                   Feature 용량     비고
─────────────────────────────────────────────────────────────────
IGB260M experimental       ~0.38 GB        개발/디버그용
IGB260M small              ~3.8 GB         단일 NVMe도 충분
IGB260M medium             ~38 GB          일반 NVMe
IGB260M large              ~384 GB         대형 NVMe × 2-4
IGB260M full               ~1,035 GB       NVMe × 3-4 STRIPE 필요
OGB-papers100M             ~428 GB         NVMe × 2
IGBH full (all ntypes)     ~2,250 GB       NVMe × 6+ STRIPE 필요
─────────────────────────────────────────────────────────────────
계산: nodes × 1024 feat × 4 bytes (float32)
```

### 5.2 그래프 구조 메모리 (Host DRAM)

Edge index는 (E,2) int64 배열:

```
IGB260M full edges (추정):
  - paper citation graph: ~3억 edges × 2 × 8 bytes ≈ ~5 GB
  - uva_graph=1 → pinned host memory에 전체 보유

IGBH full edges (추정):
  - 8 edge types 합계: ~10-20억 edges → 수십 GB
  - large/full은 메모리 부담 → uva_graph 필수
  - 그래프 구조 자체가 수십 GB DRAM을 점유
```

### 5.3 BaM GPU 페이지 캐시 (GPU HBM)

```
벤치마크 설정: cache_size = 4 × 1024 MB = 4096 MB = 4 GB
page_size = 4096 bytes (= 1 feature vector)
페이지 개수: 4096 MB / 4 KB = 1,048,576 페이지 (약 100만 노드 feature 캐시)

전체 노드 대비 캐시 비율:
  IGB full (269M): 1M / 269M = 0.37%
  IGBH full (~550M): 1M / 550M = 0.18%

→ 매우 낮은 캐시 히트율 (power-law 분포 덕분에 실제 히트율은 이보다 높음)
```

### 5.4 Constant CPU Buffer (Pinned Host Memory)

```
cpu_buffer_percent = 0.2 → 전체 노드의 20%를 pinned host memory에 pin

IGB full: 269M × 0.2 = 53.8M 노드 × 4096 bytes = ~208 GB
  → cudaHostAllocMapped → GPU가 pinned 영역 직접 읽기 가능

IGBH full: ~550M × 0.2 = 110M 노드 × 4096 bytes = ~427 GB
  → 이만한 pinned 메모리를 실제 서버에서 할당하려면 수백 GB DRAM 필요

실용 설정: 상위 PageRank 20% 노드를 page_rank_node_list_gen.py로 선별
  → power-law 분포에서 상위 20% 허브 노드가 training access의 대부분을 차지
```

### 5.5 GPU HBM 예산 (단일 A100 80GB 기준)

```
BaM 페이지 캐시:     ~4 GB
모델 파라미터:       ~0.1-2 GB (모델 크기에 따라)
그래프 구조(UVA):    host DRAM 상주 (UVA mapping)
Feature batch:       batch_size × fan_out × emb_size × 4 bytes
  = 1024 × (10+5+5) × 1024 × 4 ≈ 80 MB per batch
GPU OS/드라이버:     ~2 GB

사용 가능 HBM: 80 - 4 - 2 - 2 = ~72 GB → 여유 있음
```

---

## 6. I/O 접근 패턴 분석

### 6.1 랜덤 접근 특성

GIDS 워크로드의 핵심 특징은 **무작위 랜덤 접근(random access)**이다.

```
NeighborSampler 기준 배치당 I/O 패턴:

1. sampler가 seed 1024개 선택 (train_mask에서 랜덤)
2. 각 layer마다 fan_out 이웃 샘플 → node ID 집합 확장
   Layer 0 (closest to output): 1024 × 10 = 10,240 후보
   Layer 1:                    10,240 × 5 = 51,200 후보
   Layer 2 (input features):   51,200 × 5 = 256,000 후보
   중복 제거 후 실제 fetch: ~수만 노드 (graph 밀도에 따라)

3. 각 노드 ID → SSD 상의 offset = node_id × 4096 bytes (page 단위)
4. 각 node의 feature는 1 page = 1 NVMe I/O 요청

→ 배치 1회당 수만 건의 랜덤 NVMe 읽기 발생
→ 전통적 mmap 경로: CPU page fault storm → DRAM → GPU 전송 = 수십 ms latency
→ GIDS: GPU가 직접 NVMe 발행 = 수 ms 이하 (BaM cache hit 시 더 빠름)
```

### 6.2 Power-law 노드 접근 분포

GNN 학습에서 이웃 샘플링의 접근 분포는 **power-law(멱법칙)** 특성을 보인다:

```
학술 인용 그래프의 특성:
- 인용 많이 받는 논문(허브) → N(v) 크기 큼 → fan_out 샘플에서 자주 선택됨
- 전체 노드의 20% 허브가 학습 접근의 ~80%를 차지 (파레토 원칙)

GIDS 최적화 설계 근거:
- Constant CPU Buffer (20% 노드): 이 20%를 pinned memory에 고정 → SSD 접근 회피
- Window Buffering: 다음 배치에서도 비슷한 허브 노드가 나올 것을 예측 → 프리패치
- BaM 페이지 캐시 히트율: power-law 덕분에 0.37%의 캐시가 실제로 높은 히트율 달성
```

### 6.3 배치당 I/O 볼륨

```
NeighborSampler, fan_out=[10,5,5], batch_size=1024, emb_size=1024:

최악의 경우 (중복 없음):
  fetch 노드 수: 1024 × (1 + 10 + 10×5 + 10×5×5) = 1024 × 311 = 318,464 노드
  I/O 볼륨: 318,464 × 4096 bytes = ~1.24 GB per batch

실제 (graph 중복 고려):
  IGB full 그래프: 중복 제거 후 통상 수만 노드
  I/O 볼륨: 50,000 × 4096 bytes = ~200 MB per batch

배치 처리 시간 (목표):
  GPU 연산: ~수 ms (model forward + backward)
  SSD I/O (GIDS): batch당 수만 × 4KB → NVMe bandwidth ~3 GB/s 기준 ~70 ms
  → I/O가 지배적: 최적화의 여지가 큼
```

### 6.4 STRIPE 모드에서의 I/O 대역폭 확장

```
num_ssd = N, STRIPE 모드:
  page_i → ctrls[i % N] 로 라운드로빈 분산

단일 NVMe 순차 읽기: ~3 GB/s
단일 NVMe 랜덤 4KB IOPS: ~100K IOPS = 400 MB/s
N=4 STRIPE 랜덤: ~1.6 GB/s 유효 대역폭

IGBH full (2.25 TB): 최소 4-6개 NVMe 필요 (용량 + 대역폭)
IGB full (1 TB): 최소 2-3개 NVMe
```

---

## 7. 벤치마크 구성: Baseline vs BaM vs GIDS

세 가지 학습 경로를 동일 하이퍼파라미터로 비교해 GIDS 최적화의 기여도를 측정한다.

### 7.1 공통 하이퍼파라미터

```
데이터셋:    IGBH full (run scripts 기준)
모델:        RSAGE (RGAT 등도 지원)
num_layers:  3
fan_out:     [10, 5, 5]
batch_size:  1024 seed nodes
epochs:      1 (throughput 측정 목적)
log_every:   1000 iterations
emb_size:    1024
num_classes: 2983
uva_graph:   1 (edge_index를 UVA pinned memory에 적재)
```

### 7.2 Baseline (DGL 순정, `run_base_IGBH.sh`)

```bash
python heterogeneous_train_baseline.py \
  --dataset_size full --path /mnt/raid0/ \
  --data IGB --model_type rsage --num_layers 3 --fan_out '10,5,5' \
  --emb_size 1024
```

- **Feature 로딩**: `numpy.memmap` → OS page fault → DRAM copy → GPU 전송
- **DataLoader**: DGL 표준 `DataLoader` + `NeighborSampler`
- **BaM/GIDS**: 미사용
- **병목**: CPU page fault + PCIe 전송 latency

### 7.3 BaM Baseline (`run_BaM_IGBH.sh`)

```bash
CUDA_VISIBLE_DEVICES=0 python heterogeneous_train.py \
  --dataset_size full --path /mnt/raid0/ \
  --GIDS --uva_graph 1 --batch_size 1024 \
  --data IGB --model_type rsage --num_layers 3 --fan_out '10,5,5' \
  --cache_size $((4*1024)) --num_ssd 1 \
  --num_ele $((550*1000*1000*1024)) --page_size 4096 --emb_size 1024
```

- **Feature 로딩**: GPU → BaM `bam_ptr.read()` → NVMe SQE 제출 → DMA → GPU
- **최적화**: BaM 페이지 캐시(4GB)만 사용. CPU buffer/Window Buffering/Accumulator 없음
- **커널**: `read_feature_kernel` (단순 BaM 직접 경로)
- **병목**: random NVMe read latency (페이지 캐시 miss 시)

### 7.4 GIDS Full (`run_GIDS_IGBH.sh`)

```bash
CUDA_VISIBLE_DEVICES=0 python heterogeneous_train.py \
  --dataset_size full --path /mnt/raid0/ \
  --GIDS --uva_graph 1 --batch_size 1024 \
  --data IGB --model_type rsage --num_layers 3 --fan_out '10,5,5' \
  --cache_size $((4*1024)) --num_ssd 1 \
  --num_ele $((550*1000*1000*1024)) --page_size 4096 --emb_size 1024 \
  --window_buffer --wb_size 8 \
  --cpu_buffer --cpu_buffer_percent 0.2 \
  --accumulator
```

- **Feature 로딩**: 3단 계층 (CPU Buffer hit → BaM cache hit → NVMe miss)
- **Window Buffering**: 다음 8 배치의 page를 BaM 캐시에 미리 적재
- **Constant CPU Buffer**: 상위 20% PageRank 노드 feature를 pinned host memory에 고정
- **Accumulator**: 여러 배치를 묶어 단일 read_feature_merged() 호출로 I/O 효율화

---

## 8. GIDS 최적화별 효과 분석

### 8.1 Constant CPU Buffer — power-law 활용

```
대상: PageRank 상위 cpu_buffer_percent × N_total 개 노드
저장: cudaHostAllocMapped pinned host memory (GPU peer access 가능)

효과:
  - 가장 자주 접근되는 허브 노드 → NVMe I/O 없이 pinned memory에서 직접 읽기
  - PCIe 대역폭 사용 (≈ 12 GB/s, NVMe 랜덤 IOPS ≈ 400 MB/s보다 30× 빠름)

한계:
  - 20% 노드 pin = 수백 GB DRAM 필요 (IGB full: ~208 GB)
  - 비허브 노드에는 효과 없음
```

### 8.2 Window Buffering — 프리패치로 latency hiding

```
동작:
  - 배치 t 학습 중, 배치 t+1 ~ t+8의 seed node ID 미리 알고 있음
  - set_window_buffering_kernel 으로 이 node들의 page를 BaM 캐시에 선제 로드
  - 배치 t+1 실행 시 → BaM 캐시 히트율 급상승

효과:
  - latency hiding: GPU가 t번째 batch 연산하는 동안 t+1 batch I/O 진행
  - 캐시 효율 향상: cold-start miss 감소

한계:
  - wb_size=8 → 8배치 ahead lookahead = 8 × 수백 MB I/O를 미리 예약
  - random access 패턴에서는 예측 정확도가 낮아 효과 감소
```

### 8.3 Storage Access Accumulator — 배치 단위 I/O 합산

```
동작:
  - 여러 배치를 합산해 set_required_storage_access() 통지
  - read_feature_merged() 한 번에 여러 batch의 node feature를 fetch
  - SSD SQ(Submission Queue) depth를 최대한 활용

효과:
  - 큐 깊이 활용률 향상 (queue_depth=32 채우기)
  - I/O 요청 overhead(커널 실행, doorbell) 감소
  - 배치 간 node ID 중복 → 중복 읽기 제거 가능
```

### 8.4 BaM STRIPE — 다중 SSD 대역폭 확장

```
동작:
  - num_ssd = N 설정 시, page_i → /dev/libnvm(i % N)로 라운드로빈
  - 각 SSD가 독립적으로 서로 다른 page를 병렬 서비스

효과:
  - 랜덤 read bandwidth = 단일 SSD × N (이상적)
  - 단일 NVMe ~400 MB/s × 4 SSD = ~1.6 GB/s
  - 논문 실험에서 SSD 수 증가에 따른 선형적 throughput 향상 보고

벤치마크 스크립트: num_ssd=1 (보수적 설정)
실제 배포: IGBH full은 num_ssd=4-6 권장
```

---

## 9. 워크로드별 최적화 효과 요약

| 워크로드 특성                         | 관련 GIDS 최적화            | 효과 수준 |
|-------------------------------------|---------------------------|----------|
| Power-law 접근 분포 (허브 노드 집중)  | Constant CPU Buffer        | 높음 ★★★ |
| 연속 배치 간 노드 중복               | Window Buffering, Accumulator | 보통 ★★  |
| 대규모 SSD 스토리지 (>1TB)           | BaM GPU-direct + STRIPE    | 높음 ★★★ |
| 소규모 데이터셋 (<100GB)             | BaM (minimal benefit)      | 낮음 ★   |
| ClusterGCN (공간적 지역성 있음)      | BaM 캐시 히트율 향상        | 보통 ★★  |
| 이종 그래프 (다중 ntype)             | ntype별 병렬 스트림 처리     | 보통 ★★  |
| 많은 layer/fan_out (깊은 샘플링)     | Accumulator (배치 합산)     | 높음 ★★★ |

---

## 10. 데이터셋·모델·샘플러 조합 매트릭스

| 데이터셋          | 모델        | 샘플러       | 스크립트                            | 비고           |
|-----------------|-------------|-------------|-------------------------------------|----------------|
| IGB260M (any)   | SAGE/GCN/GAT| NeighborSampler | homogenous_train.py             | 기본 homo 실험 |
| IGB260M (any)   | SAGE        | NeighborSampler | homogenous_train_baseline.py    | mmap baseline  |
| IGB260M         | ClusterSAGE | ClusterGCN  | homogenous_train_ClusterGCN.py     | 클러스터 샘플러|
| IGB260M         | SAGE        | LADIES      | homogenous_train_ClusterGCN.py     | 중요도 샘플러  |
| IGBH (any)      | RGCN/RSAGE/RGAT/RGNN | NeighborSampler | heterogeneous_train.py | GIDS hetero    |
| IGBH (any)      | RSAGE       | NeighborSampler | heterogeneous_train_baseline.py | mmap baseline  |
| OGB-papers100M  | SAGE/GCN/GAT| NeighborSampler | homogenous_train.py (--data OGB) | OGB 표준       |
| OGB-MAG         | RGCN 등     | NeighborSampler | heterogeneous_train.py           | OGB hetero     |

---

## 11. 주요 수치 요약

```
IGB260M full:
  Nodes:          269,346,174
  Feature dim:    1024 (float32)
  Feature size:   ~1,035 GB
  Labels:         19 (227M labeled) or 2983 (157M labeled)
  Train/Val/Test: 60/20/20
  Page footprint: 269M pages × 4 KB = 1 TB

IGBH full:
  paper:          269,346,174 nodes
  author:         277,220,883 nodes
  fos:            ~713,000 nodes
  institute:      ~713,000 nodes
  Edge types:     8 (small/medium) or 12 (large/full)
  Total features: ~2.25 TB (550M nodes × 1024 × 4 bytes)
  Key task:       paper 2983-class classification

Benchmark config:
  batch_size:         1024 seed nodes/batch
  fan_out:            [10,5,5] (3 layers)
  BaM cache:          4 GB (1M pages)
  CPU buffer:         20% of nodes (pinned host)
  Window buffer:      8 batches lookahead
  NVMe page size:     4096 bytes = 1 feature vector
  SSD (benchmark):    1 × NVMe (STRIPE-ready, num_ssd 설정으로 확장)

Access pattern:
  Type:               랜덤 접근 (random access) 지배적
  Distribution:       power-law (허브 노드 집중)
  Batch I/O (rough):  수만 노드 × 4 KB = ~200 MB/batch
  I/O-to-compute:     I/O 지배적 → GIDS 최적화 유효
```

---

*참조: [[gids-complete-architecture]] — GIDS 시스템 전체 아키텍처 및 코드 흐름*
*코드: `gids-study/evaluation/dataloader.py` (데이터셋), `gids-study/evaluation/models.py` (모델), `gids-study/evaluation/ladies_sampler.py` (LADIES 샘플러)*
