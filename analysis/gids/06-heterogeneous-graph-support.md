# GIDS 이종 그래프 (Heterogeneous Graph) 지원 분석

## 1. 이종 그래프란

```
동종 그래프 (Homogeneous):
  모든 노드가 같은 타입, 모든 엣지가 같은 타입
  예: 논문 → cites → 논문

이종 그래프 (Heterogeneous):
  여러 노드 타입, 여러 엣지 타입
  예: IGB260M-Hetero

  paper ──cites──► paper
  author ──writes──► paper
  author ──affiliated──► institute
  paper ──topic──► fos (field of study)
  paper ──published──► journal
  paper ──presented──► conference
```

## 2. SSD Feature 레이아웃

```
이종 그래프의 Feature Table SSD 배치:

노드 타입별로 연속 영역에 저장, key_offset으로 구분

SSD 주소 공간:
┌───────────────────────────────────────────────────────────────┐
│ Offset 0                                                      │
│ ┌──────────────────────────────────────────────────────────┐  │
│ │ paper features                                           │  │
│ │ 269,346,000 × 1024 × 4B = ~1.03 TB                     │  │
│ └──────────────────────────────────────────────────────────┘  │
│ Offset 269,346,000 (= key_offset['author'])                  │
│ ┌──────────────────────────────────────────────────────────┐  │
│ │ author features                                          │  │
│ │ 277,221,000 × 1024 × 4B = ~1.06 TB                     │  │
│ └──────────────────────────────────────────────────────────┘  │
│ Offset 546,567,000 (= key_offset['fos'])                     │
│ ┌──────────────────────────────────────────────────────────┐  │
│ │ fos features                                             │  │
│ │ 713,000 × 1024 × 4B = ~2.9 GB                          │  │
│ └──────────────────────────────────────────────────────────┘  │
│ Offset 547,280,000 (= key_offset['institute'])               │
│ ┌──────────────────────────────────────────────────────────┐  │
│ │ institute features                                       │  │
│ │ ...                                                      │  │
│ └──────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘

STRIPE 모드에서 페이지 분산:
  전체 논리 주소 공간이 page 단위로 SSD에 분산
  Page 0 → SSD0, Page 1 → SSD1, Page 2 → SSD2, Page 3 → SSD3, ...
  → 모든 노드 타입의 feature가 균등하게 SSD에 분산됨
```

## 3. key_offset 테이블 (데이터셋 크기별)

```
heterogeneous_train.py에 하드코딩된 오프셋:

┌──────────┬────────────┬────────────┬────────────┬────────────┬──────────────┐
│ Size     │ paper      │ author     │ fos        │ institute  │ 비고         │
├──────────┼────────────┼────────────┼────────────┼────────────┼──────────────┤
│ tiny     │ 0          │ 100,000    │ 457,999    │ 541,901    │ 테스트용     │
│ small    │ 0          │ 1,000,000  │ 1,192,115  │ 1,382,429  │              │
│ medium   │ 0          │ 10,000,000 │ 25,544,626 │ 25,959,349 │              │
│ large    │ 0          │ 100,000,000│ 216,959,530│ 217,608,610│              │
│ full     │ 0          │ 269,346,174│ 546,567,174│ 547,280,174│ +journal     │
│          │            │            │            │            │ +conference  │
└──────────┴────────────┴────────────┴────────────┴────────────┴──────────────┘

Full 크기 추가 타입:
  journal:    546,593,174
  conference: 546,643,174
```

## 4. Heterogeneous Feature Read 경로

```
GIDS.py :: fetch_feature() 이종 그래프 경로

input_nodes 형태 (이종):
  {
    'paper':     tensor([0, 42, 999, ...], device='cuda:0'),
    'author':    tensor([5, 100, 8888, ...], device='cuda:0'),
    'fos':       tensor([3, 77, ...], device='cuda:0'),
    'institute': tensor([1, 2, ...], device='cuda:0')
  }

[Non-Accumulator 모드]
│
├── num_keys = len(input_nodes)   # 노드 타입 수 (예: 4)
│
├── 각 노드 타입별 출력 텐서 할당
│   i_ptr_list = []
│   i_index_list = []
│   num_index_list = []
│   key_off_list = []
│
│   for ntype in input_nodes:
│     nodes = input_nodes[ntype].to(gids_device)
│     n = len(nodes)
│     out = torch.zeros(n, dim, device=gids_device)
│
│     i_ptr_list.append(out.data_ptr())
│     i_index_list.append(nodes.data_ptr())
│     num_index_list.append(n)
│     key_off_list.append(heterograph_map[ntype])
│
├── C++ 호출
│   BAM_FS.read_feature_hetero(
│       num_keys,          # 4
│       i_ptr_list,        # [paper_out_ptr, author_out_ptr, ...]
│       i_index_list,      # [paper_idx_ptr, author_idx_ptr, ...]
│       num_index_list,    # [1000, 500, 200, 50]
│       dim,               # 1024
│       cache_dim,         # 1024
│       key_off_list       # [0, 269M, 546M, 547M]
│   )
│
│   → gids_nvme.cu :: read_feature_hetero()
│
│     for i in 0..num_keys:
│       cudaStreamCreate(&stream[i])
│       read_feature_kernel<<<grid, 128, 0, stream[i]>>>(
│           ...,
│           key_off = key_off_list[i]
│       )
│
│     ┌─ Stream 0 ─┐ ┌─ Stream 1 ─┐ ┌─ Stream 2 ─┐ ┌─ Stream 3 ─┐
│     │ paper      │ │ author     │ │ fos        │ │ institute  │
│     │ 1000 nodes │ │ 500 nodes  │ │ 200 nodes  │ │ 50 nodes   │
│     │ key_off=0  │ │ key_off=   │ │ key_off=   │ │ key_off=   │
│     │            │ │ 269M       │ │ 546M       │ │ 547M       │
│     └────────────┘ └────────────┘ └────────────┘ └────────────┘
│           ▲              ▲              ▲              ▲
│           └──────────────┴──────────────┴──────────────┘
│                    GPU가 4개 스트림을 동시 실행
│                    각 스트림이 다른 SSD 영역 접근
│                    → SSD 대역폭 극대화
│
└── 결과 조립
    ret = {}
    for ntype in input_nodes:
      ret[ntype] = out_tensors[ntype]

    return (input_nodes, seeds, blocks, ret)
    → 학습 루프에서: batch_inputs = ret  (dict of tensors)
```

## 5. 커널 내부: key_off의 역할

```
gids_kernel.cu :: read_feature_kernel

row_index = index_ptr[warp_id] + key_off
                                  ~~~~~~~
                                  노드 타입별 SSD 오프셋

예: author 노드 ID 100
    key_off = 269,346,000

    row_index = 100 + 269,346,000 = 269,346,100

    BaM array에서 이 주소로 접근:
    addr = 269,346,100 × 1024 + dim_offset
    → SSD의 author 영역에서 읽기

    page_id = addr × 4 / 4096
    STRIPE: ssd_idx = page_id % 4
            lba = page_id / 4 × sectors
```

## 6. Heterogeneous 모델과의 연결

```
models.py에서 이종 그래프 모델:

class RSAGE(nn.Module):
    def __init__(self, in_dim, h_dim, out_dim, num_layers, etypes):
        self.layers = nn.ModuleList()
        self.layers.append(
            dgl.nn.HeteroGraphConv({
                etype: dgl.nn.SAGEConv(in_dim, h_dim, 'gcn')
                for etype in etypes      # 엣지 타입별 별도 Conv
            }, aggregate='mean'))
        ...
        self.linear = nn.Linear(h_dim, out_dim)

    def forward(self, blocks, h):
        # h = {'paper': tensor, 'author': tensor, ...}
        # ★ GIDS가 반환한 dict 텐서가 여기로 들어옴

        for l, (layer, block) in enumerate(zip(self.layers, blocks)):
            h_dst = {k: v[:block.num_dst_nodes(k)]
                     for k, v in h.items()}
            h = layer(block, (h, h_dst))
            # HeteroGraphConv가 각 엣지 타입별로
            # message passing 수행 후 aggregate

            if l != len(self.layers) - 1:
                h = {k: F.relu(v) for k, v in h.items()}

        return self.linear(h['paper'])
        # 최종 예측은 paper 노드에 대해서만
```

## 7. CPU Buffer + Heterogeneous 조합

```
이종 그래프에서 CPU Buffer 설정:

page_rank_node_list_gen.py :: compute_pagerank_hetero()
│
├── 각 노드 타입별 PageRank 계산
│   pv_paper = pagerank(paper subgraph)
│   pv_author = pagerank(author subgraph)
│   ...
│
├── key_offset 적용하여 글로벌 인덱스 변환
│   paper_indices += 0
│   author_indices += 269,346,000
│   ...
│
└── 전체 상위 60% 노드 저장
    → torch.save(merged_indices, "pin_file.pt")

런타임:
  GIDS 커널이 row_index = node_id + key_off로 접근 시
  CPU buffer의 check_cpu_buffer(row_index) 에서
  글로벌 인덱스 기준으로 CPU 버퍼 히트 확인
```
