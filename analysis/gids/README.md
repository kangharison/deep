# GIDS (GPU-Initiated Direct Storage) 코드 분석

## 프로젝트 개요
- **논문:** GIDS: Accelerating Sampling and Aggregation Operations in GNN Frameworks (VLDB 2024)
- **저자:** Jeongmin Brian Park, Vikram Sharma Mailthody, Zaid Qureshi, Wen-mei Hwu
- **소스:** https://github.com/jeongminpark417/GIDS.git
- **핵심:** BaM의 GPU-initiated I/O를 GNN 학습 파이프라인(DGL)에 적용

## BaM과의 관계
```
BaM (기반 기술)                    GIDS (응용)
━━━━━━━━━━━━━━━━━━━              ━━━━━━━━━━━━━━━━━━━
GPU → NVMe 직접 I/O              BaM + GNN Feature Fetching
Lock-free Parallel Queue          + Page Cache 최적화
Page Cache                        + CPU Backing Buffer
DMA Mapping (커널 모듈)           + Window Buffering
                                  + Storage Access Accumulator
                                  + DGL/PyTorch 통합
```

## 소스 구조
```
gids/
├── gids_module/                ← C++/CUDA 핵심 모듈
│   ├── gids_kernel.cu          ← CUDA 커널 (feature read/write)
│   ├── gids_nvme.cu            ← BAM_Feature_Store 클래스 + PyBind11
│   ├── include/
│   │   ├── bam_nvme.h          ← 구조체 정의 (GIDS_Controllers, BAM_Feature_Store)
│   │   └── page_cache_backup.h ← 페이지 캐시, TLB 구현
│   ├── BAM_Feature_Store/      ← Python 패키지 (컴파일된 .so 래핑)
│   └── example/                ← 예제 패키지
├── GIDS_Setup/                 ← Python API 레이어
│   ├── GIDS/
│   │   └── GIDS.py             ← GIDS 클래스, GIDS_DGLDataLoader (569줄, 핵심)
│   └── setup.py
├── evaluation/                 ← 학습/평가 스크립트
│   ├── dataloader.py           ← IGB260M, OGB 데이터셋 래퍼
│   ├── models.py               ← GCN, SAGE, GAT, RGCN, RSAGE, RGAT
│   ├── mlperf_model.py         ← MLPerf RGNN 모델
│   ├── homogenous_train.py     ← GIDS 동종 그래프 학습
│   ├── homogenous_train_baseline.py ← 베이스라인 (mmap)
│   ├── heterogeneous_train.py  ← GIDS 이종 그래프 학습
│   ├── heterogeneous_train_baseline.py ← 베이스라인
│   ├── ladies_sampler.py       ← LADIES 샘플링 알고리즘
│   ├── page_rank_node_list_gen.py ← CPU 캐시용 노드 선별
│   ├── tensor_write.py         ← Feature를 SSD에 기록
│   ├── GIDS_unit_test.py       ← 단위 테스트
│   └── lock_mem.cpp            ← 메모리 락 유틸리티
└── bam/                        ← BaM 서브모듈 (외부 의존성)
```

## 분석 문서

| 문서 | 내용 |
|------|------|
| [gids-complete-architecture.md](gids-complete-architecture.md) | **완전 아키텍처 통합 문서** — 이 파일 하나로 전체 이해 (1,170줄) |

### 문서 구성 (14개 섹션)
1. 문제 정의 (기존 병목 vs GIDS 해결)
2. 전체 시스템 레이어 (Python → C++/CUDA → BaM → 하드웨어)
3. 핵심 자료구조 관계도
4. 초기화 플로우 (Controller → PageCache → Range → Array → CPU 버퍼)
5. 학습 1 Iteration 완전 호출 체인 + Sequence 다이어그램
6. 3가지 Feature Read 경로 (BaM 직접 / CPU 버퍼 hybrid / Window Buffering)
7. GPU 커널 내부 (Block/Warp 매핑, bam_ptr.read 동작)
8. BaM I/O 경로 (SQE 구조, doorbell, P2P DMA, STRIPE LBA)
9. Window Buffering & Storage Access Accumulator
10. 이종 그래프 지원 (key_offset 매핑, 다중 스트림 병렬 처리)
11. Feature 쓰기 플로우 (SSD 사전 기록)
12. pybind11 바인딩 계층
13. 핵심 파라미터 레퍼런스
14. 성능 설계 원칙 요약
