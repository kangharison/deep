# [I4] GoFS: Managing Scalable Direct Storage Accesses for GPUs

- **학회/연도:** SOSP 2025
- **저자:** Li et al.
- **분류:** GPU Initiated I/O

## 핵심 요약 (1~2문장)
GPU-initiated I/O 위에 확장 가능한 파일시스템 추상화를 제공하는 시스템. BaM이 raw block 접근만 지원하는 한계를 극복하여, GPU 커널에서 POSIX-like 파일 API(open/read/write)를 사용할 수 있게 하며, 멀티 GPU + 멀티 SSD 환경으로의 스케일링을 핵심 설계 목표로 함.

## 읽기 전 질문
- GPU 커널 내에서 파일시스템 메타데이터(inode, directory)를 어떻게 관리하는가? CPU와 공유하는가?
- 멀티 GPU가 동시에 같은 파일에 접근할 때 일관성(consistency)은 어떻게 보장하는가?
- 파일시스템 오버헤드가 BaM의 raw block 접근 대비 얼마나 추가되는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **BaM의 한계**: raw NVMe block 주소를 직접 지정해야 함. 애플리케이션이 LBA 관리를 직접 수행해야 하는 부담
- **GPUfs의 한계**: 파일시스템 인터페이스는 제공하지만 CPU에 의존(CPU가 VFS를 통해 I/O 수행). GPU-initiated가 아닌 CPU-mediated 방식
- **확장성 부재**: 기존 시스템들은 단일 GPU + 단일 SSD 구성에 최적화. 대규모 데이터 처리에 필요한 멀티 GPU, 멀티 SSD 확장이 어려움
- **파일 공유/동시 접근**: 여러 GPU 커널이 동일 파일을 동시에 접근하는 시나리오에 대한 고려 부족

### 2. 제안 방법 (Approach)
- **GPU-native 파일시스템**: GPU 커널 내에서 직접 실행되는 경량 파일시스템
- **계층적 설계**:
  1. **파일 인터페이스 레이어**: GPU 커널에 open/read/write/close API 제공
  2. **메타데이터 관리**: 경량 inode/extent 구조를 GPU 메모리에 캐시. CPU가 초기 설정 후 GPU가 독립 운영
  3. **I/O 디스패치**: 파일 오프셋 → LBA 변환 후 BaM 스타일 NVMe 직접 접근
- **멀티 디바이스 스케일링**: 파일 데이터를 여러 SSD에 striping하고, 여러 GPU가 각자의 NVMe 큐를 통해 병렬 접근

### 3. 핵심 아키텍처/설계

```
┌───────────────────────────────────────────────┐
│              GPU Kernel (Application)          │
│  gofs_open("data.bin")                        │
│  gofs_read(fd, buf, size, offset)             │
│  gofs_close(fd)                               │
└──────────────────┬────────────────────────────┘
                   │
┌──────────────────▼────────────────────────────┐
│              GoFS File Layer                   │
│  ┌────────────────┐  ┌────────────────────┐   │
│  │ File Descriptor │  │ Extent Map Cache  │   │
│  │ Table (GPU)    │  │ (inode→extent→LBA) │   │
│  └────────────────┘  └────────┬───────────┘   │
│                               │               │
│  offset → extent → LBA 변환  │               │
└──────────────────┬────────────┘───────────────┘
                   │
┌──────────────────▼────────────────────────────┐
│            I/O Dispatch Layer                  │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐     │
│  │SSD 0 │  │SSD 1 │  │SSD 2 │  │SSD 3 │     │
│  │Queue │  │Queue │  │Queue │  │Queue │     │
│  └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘     │
└─────┼─────────┼─────────┼─────────┼──────────┘
      │         │         │         │
   ┌──▼──┐  ┌──▼──┐  ┌──▼──┐  ┌──▼──┐
   │SSD 0│  │SSD 1│  │SSD 2│  │SSD 3│  (Striped)
   └─────┘  └─────┘  └─────┘  └─────┘

┌─────────────────────────────────────────┐
│         멀티 GPU 스케일링               │
│  GPU 0 ──→ SSD 0, SSD 1               │
│  GPU 1 ──→ SSD 2, SSD 3               │
│  각 GPU가 독립적 NVMe 큐 보유          │
│  메타데이터는 공유 or 복제              │
└─────────────────────────────────────────┘
```

- **메타데이터 관리 전략**: CPU에서 파일시스템 포맷/마운트를 수행하고 메타데이터를 GPU 메모리로 복사. 런타임에는 GPU가 캐시된 메타데이터로 독립 운영
- **Extent 기반 매핑**: 파일 오프셋 → extent(연속 LBA 범위)로 매핑. B-tree 또는 해시 기반 lookup으로 빠른 변환
- **스트라이핑**: 큰 파일은 여러 SSD에 걸쳐 stripe하여 대역폭 극대화

### 4. 실험 결과 (Key Results)
- **단일 GPU/SSD 성능**: BaM의 raw block 접근 대비 파일시스템 오버헤드 **5~10%** 수준 (메타데이터 lookup 비용)
- **멀티 SSD 스케일링**: 8개 SSD에서 **거의 선형 스케일링** 달성 (~50 GB/s 이상 aggregate 대역폭)
- **멀티 GPU 스케일링**: 4개 GPU에서 효율적인 병렬 접근 확인
- **워크로드 성능**:
  - 그래프 분석(BFS/PageRank): GDS 대비 **3~6배** 빠름
  - 데이터베이스 스캔: CPU-mediated 방식 대비 **2~4배** 가속
- **GPUfs 대비**: GPU-initiated 방식으로 CPU 병목 제거, **5~10배** 성능 향상

### 5. 한계점 및 향후 연구
- **파일시스템 기능 제한**: 전체 POSIX 시맨틱 미지원. 디렉토리 탐색, 심볼릭 링크 등 복잡한 기능 없음
- **메타데이터 업데이트**: 런타임 중 파일 생성/삭제/확장 시 CPU와의 동기화 필요 → 오버헤드
- **일관성 모델**: 멀티 GPU 환경에서의 강한 일관성 보장은 비용이 큼. 완화된 일관성 모델 사용
- **Crash consistency**: 파일시스템의 저널링/fsync에 해당하는 메커니즘이 제한적
- **호스트 파일시스템과의 호환성**: ext4/XFS 등 기존 파일시스템과 직접 호환되지 않음. 전용 포맷 필요

## 다른 논문과의 관계
- 선행 연구: BaM [I1] (raw block I/O 기반), GPUfs [P1] (GPU 파일시스템 개념의 선구자)
- 후속 연구: 향후 GPU-native 파일시스템 연구의 기반
- 비교 대상: GeminiFS [I5] (companion FS 접근 vs native FS 접근), GPUfs [P1] (CPU-mediated vs GPU-initiated)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure: GoFS 아키텍처 다이어그램 (파일 레이어 → I/O 디스패치 → 멀티 SSD)
  - Figure: 멀티 SSD/GPU 스케일링 그래프
  - Table: BaM, GDS, GPUfs와의 성능 비교
- 핵심 수치/데이터:
  - 파일시스템 오버헤드 5~10% (raw block 대비)
  - 8 SSD에서 선형 스케일링
  - GDS 대비 3~6배 성능 향상

## 메모
- BaM → GoFS 진화는 "raw socket → TCP/IP 스택" 진화와 유사. 추상화 레벨을 올려 사용성 개선
- 파일시스템은 GPU-initiated I/O의 대중화를 위한 필수 요소. 일반 프로그래머가 LBA를 직접 다루기는 어려움
