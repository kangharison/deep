# BaM 원본 vs GIDS 비교 분석

## 1. 코드 레벨 차이

```
BaM (benchmarks/block/main.cu)
│
│ 범용 블록 I/O 벤치마크
│ → 임의 LBA에 대해 read/write
│ → GPU 스레드가 직접 NVMe 커맨드 발행
│ → 결과: IOPS, Bandwidth 측정
│
│ 핵심 API:
│   Controller(path, ns, cuda, depth, queues)
│   page_cache_t(pageSize, nPages, cuda, ctrl, 64, ctrls)
│   read_data(pc, qp, lba, n_blocks, entry)
│   write_data(pc, qp, lba, n_blocks, entry)


GIDS (gids_module/gids_nvme.cu)
│
│ GNN Feature Fetching 특화
│ → 노드 ID → feature 벡터 읽기
│ → BaM의 array_t/range_t/page_cache_t 위에 구축
│ → PyTorch 텐서로 결과 반환
│
│ 핵심 API:
│   GIDS_Controllers.init_GIDS_controllers(n, depth, queues, ssd_list)
│   BAM_Feature_Store.init_controllers(ctrl, ps, off, cache, ele, ssd)
│   BAM_Feature_Store.read_feature(out, idx, n, dim, cache_dim, key_off)
│   BAM_Feature_Store.read_feature_hetero(...)
│   BAM_Feature_Store.read_feature_merged(...)
```

## 2. 아키텍처 레이어 비교

```
┌─────────────────────────────┬──────────────────────────────────────┐
│ BaM                         │ GIDS                                 │
├─────────────────────────────┼──────────────────────────────────────┤
│                             │ Python Training Script               │
│                             │ (homogenous_train.py)                │
│                             │         │                            │
│                             │ GIDS Python API                      │
│                             │ (GIDS.py)                            │
│                             │ - GIDS class                         │
│                             │ - GIDS_DGLDataLoader                 │
│                             │ - fetch_feature()                    │
│                             │ - window_buffering()                 │
│                             │ - accumulator logic                  │
│                             │         │                            │
│                             │ C++/CUDA Module                      │
│                             │ (gids_nvme.cu, gids_kernel.cu)       │
│                             │ - BAM_Feature_Store                  │
│                             │ - read_feature_kernel                │
│                             │ - CPU buffer integration             │
│                             │ - PyBind11 bindings                  │
│                             │         │                            │
├─────────────────────────────┼──────────────────────────────────────┤
│ Benchmark (main.cu)         │         │                            │
│ - sequential/random kernel  │         │                            │
│ - direct read_data/write_data         │                            │
│         │                   │         │                            │
├─────────────────────────────┼──────────────────────────────────────┤
│                  BaM Core Library (공유)                            │
│         Controller, QueuePair, page_cache_t                        │
│         range_t, array_t, bam_ptr                                  │
│         nvm_parallel_queue.h (lock-free queue)                     │
│         sq_enqueue, cq_poll, cq_dequeue                            │
├─────────────────────────────┴──────────────────────────────────────┤
│                  BaM Kernel Module (공유)                           │
│         /dev/libnvm0~5, BAR0 mmap, GPU DMA mapping                 │
├────────────────────────────────────────────────────────────────────┤
│                  Hardware                                           │
│         GPU ◄──── PCIe P2P ────► NVMe SSD                          │
└────────────────────────────────────────────────────────────────────┘
```

## 3. I/O 패턴 비교

```
=== BaM Block Benchmark ===

I/O 패턴: 순수 블록 I/O
  sequential: LBA 0, 1, 2, 3, ... (연속)
  random:     LBA 42, 9999, 7, 1337, ... (무작위)

접근 단위: page_size (설정 가능)
  보통 512B, 4KB, 8KB 등

워크로드: read-only, write-only, mixed
  read:write 비율 조절 가능 (--ratio)

커널 구조:
  각 GPU 스레드 → 독립적 I/O
  read_data(pc, qp, start_lba, n_blocks, cache_entry)
  → 직접 NVMe 커맨드 발행


=== GIDS Feature Read ===

I/O 패턴: GNN Feature Gathering
  노드 ID 기반 random access
  같은 노드가 여러 배치에서 반복 접근 (locality 있음)
  배치 내에서는 불규칙 (sampling 결과에 의존)

접근 단위: feature 벡터
  dim × sizeof(float) = 1024 × 4 = 4096B (보통 1 페이지)

워크로드: read-only (학습 시)
  feature를 SSD에서 읽기만 함
  쓰기는 초기 데이터 저장 시에만

커널 구조:
  각 warp (32 스레드) → 노드 1개의 feature
  bam_ptr.read(row_index * cache_dim + j)
  → array_t → range_t → page_cache → NVMe
  → 간접 계층이 BaM보다 많음
```

## 4. 기능 차이 상세

```
┌──────────────────────┬──────────────┬─────────────────────────────┐
│ 기능                  │ BaM          │ GIDS                        │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 언어/바인딩           │ C++/CUDA     │ C++/CUDA + PyBind11 + Python│
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 프레임워크 통합       │ 없음         │ DGL + PyTorch               │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 데이터 분산           │ STRIPE,      │ STRIPE (고정)               │
│                      │ REPLICATE    │                             │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ I/O 타입              │ Read + Write │ Read 중심                   │
│                      │ + Mixed      │ (Write는 초기 저장만)       │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ Page Cache            │ ✅ 기본 제공  │ ✅ + CPU Backing Buffer     │
│                      │              │   + Window Buffering         │
│                      │              │   + Storage Accumulator      │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ SSD 개수              │ CLI 설정     │ ssd_list로 선택적 지정      │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 이종 그래프           │ 없음         │ key_offset 기반 지원        │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 배치 처리             │ 개별 스레드   │ 배치 누적 + 다중 스트림     │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 핫 데이터 최적화      │ Page Cache만 │ + PageRank CPU 피닝         │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 프리페칭             │ 없음         │ Window Buffer               │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 통계/프로파일링       │ 기본 IOPS/BW │ + 커널 시간, CPU 히트율,    │
│                      │              │   per-batch 통계             │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 워크 단위             │ 스레드 1개 = │ warp (32스레드) =            │
│                      │ I/O 1개      │ 노드 1개 (1024 dim)         │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 큐 선택              │ SM ID 기반   │ SM ID 기반 (BaM 동일)       │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 컨트롤러 선택        │ atomic 카운터│ atomic 카운터 (BaM 동일)    │
│                      │ + warp 브로드│                             │
├──────────────────────┼──────────────┼─────────────────────────────┤
│ 주요 사용자           │ 시스템 연구자│ GNN/ML 연구자               │
└──────────────────────┴──────────────┴─────────────────────────────┘
```

## 5. GIDS가 BaM에 추가한 코드량

```
GIDS 고유 코드:

gids_module/gids_kernel.cu    190줄   CUDA 커널
gids_module/gids_nvme.cu      589줄   C++ 클래스 + PyBind11
gids_module/include/bam_nvme.h 125줄   헤더
GIDS_Setup/GIDS/GIDS.py       569줄   Python API
evaluation/*.py               ~2500줄  학습/평가 스크립트
────────────────────────────────────
합계:                          ~3973줄  GIDS 고유 코드

BaM 재사용 코드:
  Controller, QueuePair, page_cache_t, range_t, array_t
  nvm_parallel_queue.h, nvm_queue.h
  커널 모듈 (module/)
  ~15,000줄+

비율: GIDS 고유 ~20%, BaM 재사용 ~80%
→ BaM의 인프라 위에 GNN 특화 로직을 얹은 구조
```

## 6. 성능 특성 비교

```
BaM Block Benchmark:
  ┌─────────────────────────────────────┐
  │ 4 SSD, random 4KB read              │
  │ IOPS: ~5-6M IOPS                    │
  │ BW:   ~20-24 GB/s                   │
  │ 순수 I/O 성능 측정                  │
  │ 오버헤드: 최소 (직접 NVMe 접근)     │
  └─────────────────────────────────────┘

GIDS GNN Training:
  ┌─────────────────────────────────────┐
  │ 4 SSD, feature read (4KB per node)  │
  │ Feature BW: ~20+ GB/s              │
  │ 캐시 히트 시: ~60-80% 히트율        │
  │ 유효 BW: BW / (1 - hit_rate)        │
  │                                     │
  │ 추가 오버헤드:                      │
  │ - array_t/range_t 주소 변환         │
  │ - TLB/Page Cache 룩업               │
  │ - CPU Buffer 체크                   │
  │ - Python↔C++ 바인딩                 │
  │ - DGL Sampling 시간                 │
  │                                     │
  │ End-to-End 효과:                    │
  │ - DGL CPU baseline 대비 582배 가속  │
  │ - UVA baseline 대비 2-8배 가속      │
  └─────────────────────────────────────┘
```
