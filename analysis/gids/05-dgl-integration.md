# GIDS ↔ DGL DataLoader 통합 구조 분석

## 1. 표준 DGL 파이프라인 vs GIDS 파이프라인

```
=== 표준 DGL (Baseline) ===

dgl.dataloading.DataLoader
│
├── Sampler: NeighborSampler([10, 15])
│   → 각 타겟 노드에 대해 1-hop 10개, 2-hop 15개 이웃 선택
│
├── 데이터 전달:
│   for input_nodes, seeds, blocks in dataloader:
│       batch_inputs = blocks[0].srcdata['feat']
│       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
│       ★ DGL이 Feature를 자동으로 가져옴:
│         CPU 메모리(또는 UVA)에서 → GPU로 복사
│         이 과정이 느림 (PCIe 병목)
│
│       batch_labels = blocks[-1].dstdata['label']
│       pred = model(blocks, batch_inputs)
│       loss = loss_fn(pred, batch_labels)


=== GIDS 파이프라인 ===

GIDS_DGLDataLoader (→ torch.utils.data.DataLoader 상속)
│
├── Sampler: NeighborSampler([10, 15])
│   → 동일한 sampling 로직
│
├── 데이터 전달:
│   for input_nodes, seeds, blocks, ret in dataloader:
│       batch_inputs = ret
│       ~~~~~~~~~~~~~~~~~~~
│       ★ GIDS가 Feature를 GPU에서 직접 가져옴:
│         GPU → BaM → PCIe P2P → NVMe SSD
│         CPU 개입 없음, 이미 GPU 메모리에 있음
│
│       batch_labels = blocks[-1].dstdata['label']
│       pred = model(blocks, batch_inputs)
│       loss = loss_fn(pred, batch_labels)
```

## 2. GIDS_DGLDataLoader 구조

```
GIDS.py :: GIDS_DGLDataLoader   Lines 63-190

class GIDS_DGLDataLoader(torch.utils.data.DataLoader):
    │
    │  torch.utils.data.DataLoader를 상속하여
    │  DGL의 sampling + GIDS의 feature fetching을 결합
    │
    def __init__(self, graph, indices, graph_sampler,
    │            batch_size, dim, GIDS, device, ...):
    │
    ├── [1] 그래프 전처리
    │   if graph.device == cpu:
    │     graph.pin_memory_()  ← CPU 그래프를 피닝 (UVA 접근용)
    │     use_uva = True
    │
    ├── [2] 인덱스 텐서화
    │   if isinstance(indices, dict):    ← Heterogeneous
    │     indices = {k: v.to(device) for k, v in indices.items()}
    │   else:                            ← Homogeneous
    │     indices = indices.to(device)
    │
    ├── [3] Collate 함수 래핑
    │   collator = CollateWrapper(graph_sampler.sample, graph, device)
    │   │
    │   │  CollateWrapper.__call__(items):
    │   │    return self.sample_func(self.g, items)
    │   │    → DGL의 sampling 함수를 호출
    │   │    → (input_nodes, seeds, blocks) 반환
    │   │
    │   │  ★ Feature는 여기서 가져오지 않음!
    │   │    Sampling 결과만 반환
    │
    ├── [4] DataLoader 초기화
    │   super().__init__(
    │     dataset=indices,          ← 노드 인덱스를 데이터셋으로
    │     batch_size=batch_size,
    │     collate_fn=collator,      ← Sampling 함수
    │     num_workers=0,            ← GPU sampling이므로 worker 불필요
    │     pin_memory=False
    │   )
    │
    └── self.GIDS_Loader = GIDS    ← GIDS 인스턴스 저장
        self.dim = dim
        self.device = device

    def __iter__(self):
        dataloader_it = super().__iter__()
        return _PrefetchingIter(self, dataloader_it, self.GIDS_Loader)
        │
        │  ★ 핵심: 표준 DataLoader의 이터레이터를
        │    _PrefetchingIter로 래핑
```

## 3. _PrefetchingIter 동작

```
GIDS.py :: _PrefetchingIter   Lines 46-59

class _PrefetchingIter:
    """
    DataLoader 이터레이터를 래핑하여
    매 next() 호출 시 GIDS로 feature를 가져옴
    """

    def __init__(self, dataloader, dataloader_it, GIDS_Loader):
        self.GIDS_Loader = GIDS_Loader
        self.dataloader_it = dataloader_it
        self.dataloader = dataloader
        self.dim = dataloader.dim
        self.device = dataloader.device

    def __iter__(self):
        return self

    def __next__(self):
        # GIDS의 fetch_feature가:
        # 1. dataloader_it에서 다음 sampling 결과 가져옴
        # 2. GIDS로 feature를 GPU에서 직접 읽음
        # 3. (input_nodes, seeds, blocks, features) 반환
        return self.GIDS_Loader.fetch_feature(
            self.dim,
            self.dataloader_it,
            self.device
        )


호출 체인:

  for batch in GIDS_DGLDataLoader:
      │
      └── _PrefetchingIter.__next__()
          │
          └── GIDS.fetch_feature(dim, it, device)
              │
              ├── (input_nodes, seeds, blocks) = next(it)
              │   └── CollateWrapper.__call__(batch_indices)
              │       └── sampler.sample(g, batch_indices)
              │           └── DGL NeighborSampler 실행
              │               → 이웃 노드 탐색
              │               → blocks (message flow graph) 생성
              │
              ├── BAM_FS.read_feature(...)
              │   └── CUDA 커널 → BaM → SSD
              │       → features GPU 텐서 완성
              │
              └── return (input_nodes, seeds, blocks, features)
                          ─────────────────────── ────────
                          DGL에서 온 것              GIDS에서 온 것
```

## 4. 학습 스크립트 통합 패턴

```python
# homogenous_train.py :: track_acc_GIDS()   (핵심 부분만)

# ──── 초기화 ────
GIDS_Loader = GIDS.GIDS(
    page_size=8, num_ssd=4, cache_size=8,
    cache_dim=1024, num_ele=args.num_ele,
    ssd_list=[0,1,2,3]
)

# CPU 버퍼 (선택)
if args.cpu_buffer:
    GIDS_Loader.cpu_backing_buffer(1024, num_pinned)
    pr = torch.load(args.pin_file)
    GIDS_Loader.set_cpu_buffer(pr, num_pinned)

# Storage Accumulator (선택)
if args.accumulator:
    GIDS_Loader.set_required_storage_access(
        bw=5.8, l_ssd=11, l_system=20,
        num_ssd=4, p=0.3)

# ──── DataLoader 생성 ────
sampler = dgl.dataloading.MultiLayerNeighborSampler(
    [int(x) for x in args.fan_out.split(',')])

train_dataloader = GIDS_DGLDataLoader(
    g, train_nid, sampler,
    batch_size=args.batch_size,
    dim=args.emb_size,        # 1024
    GIDS=GIDS_Loader,
    device=device,
    shuffle=True
)

# ──── 학습 루프 ────
for epoch in range(args.epochs):
    model.train()

    for step, (input_nodes, seeds, blocks, ret) in \
            enumerate(train_dataloader):
        #                                    ^^^
        #               ★ ret = GIDS가 가져온 feature 텐서
        #                 이미 GPU에 있음!

        blocks = [b.to(device) for b in blocks]
        batch_labels = blocks[-1].dstdata['label'].to(device)

        batch_inputs = ret    # ← GIDS feature (GPU tensor)

        batch_pred = model(blocks, batch_inputs)
        loss = loss_fn(batch_pred, batch_labels)
        loss.backward()
        optimizer.step()
        optimizer.zero_grad()
```

## 5. Heterogeneous DataLoader 차이점

```
heterogeneous_train.py에서의 차이:

# ──── GIDS 초기화 (이종 그래프) ────
key_offset = {'paper': 0, 'author': 269346000,
              'fos': 546567000, 'institute': 547280000}

GIDS_Loader = GIDS.GIDS(
    ...,
    heterograph=True,              # ★ 이종 그래프 모드
    heterograph_map=key_offset     # ★ 노드 타입별 SSD 오프셋
)

# ──── DataLoader ────
train_dataloader = GIDS_DGLDataLoader(
    g,
    {'paper': train_nid},          # ★ dict 형태 인덱스
    sampler,
    ...
)

# ──── 학습 루프 ────
for step, (input_nodes, seeds, blocks, ret) in \
        enumerate(train_dataloader):

    blocks = [b.to(device) for b in blocks]

    # ret는 이미 dict 형태:
    # {'paper': tensor(...), 'author': tensor(...), ...}
    batch_inputs = ret

    batch_labels = blocks[-1].dstdata['label']['paper']
    #                                          ^^^^^^^^
    #                              이종 그래프는 노드 타입 지정

    batch_pred = model(blocks, batch_inputs)
    loss = loss_fn(batch_pred, batch_labels)
```

## 6. Baseline vs GIDS 데이터 흐름 비교

```
=== Baseline (homogenous_train_baseline.py) ===

  CPU DRAM                  GPU VRAM
  ┌────────────┐           ┌─────────────┐
  │ Feature    │  PCIe     │             │
  │ Table      │ ────────► │ batch_inputs│
  │ (mmap)     │  copy     │             │
  └────────────┘           │ model()     │
       ▲                   │ loss()      │
  DGL가 이 복사를          │ backward()  │
  자동으로 수행            └─────────────┘
  (느림: CPU→GPU)


=== GIDS ===

  NVMe SSD                 GPU VRAM
  ┌────────────┐           ┌─────────────┐
  │ Feature    │  PCIe P2P │ Page Cache  │
  │ Table      │ ────────► │     ↓       │
  │ (striped)  │  Direct   │ batch_inputs│
  └────────────┘           │             │
       ▲                   │ model()     │
  GPU가 BaM을 통해         │ loss()      │
  직접 읽기                │ backward()  │
  (CPU 우회)               └─────────────┘


타이밍 차이:

Baseline:
  [DGL Sampling] → [CPU Feature Copy] → [GPU Transfer] → [Train]
  ~~~~~~~~          ~~~~~~~~~~~~~~~~~~   ~~~~~~~~~~~~~~~
  ~10ms             ~50-500ms (병목!)    ~5-20ms

GIDS:
  [DGL Sampling] → [GIDS Feature Read] → [Train]
  ~~~~~~~~          ~~~~~~~~~~~~~~~~~~~
  ~10ms             ~1-10ms (GPU 직접)
                    (CPU 병목 제거!)
```

## 7. 지원 Sampler 비교

```
┌──────────────────┬───────────────────┬──────────────────────┐
│ Sampler          │ Baseline          │ GIDS                 │
├──────────────────┼───────────────────┼──────────────────────┤
│ NeighborSampler  │ ✅ 지원           │ ✅ 지원 (기본)       │
│ (NHS)            │                   │                      │
├──────────────────┼───────────────────┼──────────────────────┤
│ ClusterGCN       │ ✅ 지원           │ ❌ 미지원            │
│                  │ (graph partition) │ (배치 기반이 아님)   │
├──────────────────┼───────────────────┼──────────────────────┤
│ LADIES           │ ✅ 지원           │ ❌ 미지원            │
│                  │ (importance-based)│ (COO 형식 필요)      │
└──────────────────┴───────────────────┴──────────────────────┘

GIDS는 NeighborSampler에 최적화:
- 배치마다 다른 노드 집합 → random access 패턴
- GPU-initiated random I/O에 적합
- BaM의 page cache가 locality 활용
```
