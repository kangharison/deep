# NVMe Storage Stack & GPU-Storage I/O 심층 분석 프로젝트

## 나의 목표
Linux Kernel의 NVMe 스택 전체를 깊이 이해하고
SPDK/DPDK와의 차이점을 코드 레벨에서 분석한다.
GPU-NVMe 직접 접근(BaM, GPUDirect Storage) 기술을 연구한다.

## 나의 배경
- Linux 커널 드라이버 공부 시작 단계
- C 언어 기본 지식 있음
- 스토리지 시스템에 관심 많음

## 소스코드 위치
- 커널:        sources/linux
- SPDK:        sources/spdk
- DPDK:        sources/dpdk
- nvme-cli:    sources/nvme-cli
- BaM:         sources/bam
- GIDS:        sources/gids
- GPGPU-Sim:   sources/gpgpu-sim_distribution
- QEMU:        sources/qemu

## 공부 범위
1. PCIe Driver
2. NVMe Kernel Driver
3. Block Layer (blk-mq)
4. SPDK
5. DPDK
6. fio + ioengine
7. GPU Architecture & CUDA
8. GPU-NVMe I/O (BaM, GPUDirect Storage)

## 나의 공부 방식
- 코드 분석할 때는 함수 호출 체인을 추적해줘
- 커널 코드와 SPDK 코드를 항상 비교해서 설명해줘
- 어려운 개념은 그림(ASCII)으로 설명해줘
- 분석 후 적절한 폴더에 정리할 내용 요약해줘

## 논문/자료 정리 규칙
- 다시 원본을 읽지 않아도 될 정도로 디테일하게 정리 (수치, 조건, 맥락 모두 포함)
- 출처 URL을 문서 상단에 명시
- 스펙/비교는 표(table)로 정리
- 핵심 공식은 코드 블록으로 작성하고, 변수 정의를 반드시 포함
- 실험 결과는 구체적 수치(throughput, latency, 환경 등)까지 기록
- 논문의 핵심 기여(contribution), 한계(limitation), 관련 연구와의 차이점 명시
- 섹션을 번호로 구분하여 구조화
- 저장 위치: 논문은 papers/ 아래, 웹 자료/학습 노트는 notes/ 아래
- 작성 후 반드시 commit + push

## 프로젝트 구조

```
deep/
├── CLAUDE.md                    ← 프로젝트 설정 (이 파일)
├── notes/                       ← 학습 노트 (주제별)
│   ├── kernel/                  ← 커널 스택 분석 노트
│   │   ├── nvme-driver.md       ← NVMe 드라이버 큐 구조
│   │   └── spdk.md              ← SPDK 아키텍처
│   └── gpu-storage/             ← GPU-Storage 학습 자료
│       ├── nvidia-developer-resources.md  ← NVIDIA 학습 리소스 (120+)
│       ├── optimization-history.md        ← GPU-NVMe 최적화 역사
│       ├── experiment-design.md           ← 실험 설계
│       └── ppt-outline.md                 ← 발표 자료 구조
├── papers/                      ← 논문 리딩 노트
│   └── gpu-ssd/                 ← GPU-Storage I/O (41편)
│       ├── README.md            ← 논문 인덱스 & 읽기 순서
│       ├── experiment-metrics.md← 논문별 실험 메트릭 정리
│       ├── P-prereq/            ← 선행 연구 (7편)
│       ├── C-cpu-io/            ← CPU Initiated I/O (4편)
│       ├── G-gds/               ← GPUDirect Storage (8편)
│       ├── I-gpu-initiated/     ← GPU Initiated I/O (9편, 핵심)
│       ├── O-offload/           ← GPU-NVMe 오프로드 (8편)
│       └── N-industry/          ← 산업 동향 (5편)
├── analysis/                    ← 코드 분석 상세
│   ├── bam/                     ← BaM 코드 분석 (README + 12개 상세)
│   ├── gids/                    ← GIDS 코드 분석 (README + 9개 상세)
│   ├── gpgpu-sim/               ← GPGPU-Sim 시뮬레이터 분석 (9개)
│   └── kernel/                  ← 커널 I/O 경로 분석
├── scripts/                     ← 자주 쓰는 명령어
└── sources/                     ← 소스코드 (.gitignore)
```
