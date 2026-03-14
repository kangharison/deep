# [I5] GeminiFS: A Companion File System for GPUs

- **학회/연도:** FAST 2025
- **저자:** Qiu et al.
- **분류:** GPU Initiated I/O

## 핵심 요약 (1~2문장)
GPU를 위한 "companion" 파일시스템으로, CPU 측 호스트 파일시스템과 협력하여 동작. GPU가 데이터 플레인(I/O)을 직접 처리하고, CPU가 제어 플레인(메타데이터 관리)을 담당하는 분리 아키텍처를 통해 기존 파일시스템과의 호환성을 유지하면서 GPU-initiated I/O의 성능 이점을 확보.

## 읽기 전 질문
- "Companion" 파일시스템이란 정확히 무엇인가? 기존 ext4/XFS와 어떻게 공존하는가?
- 제어 플레인(CPU)과 데이터 플레인(GPU) 분리 시, 메타데이터 일관성은 어떻게 유지하는가?
- GoFS와 비교하여 어떤 설계 트레이드오프를 선택했는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **BaM의 사용성 문제**: raw block 접근만 가능하여 애플리케이션이 데이터 배치를 직접 관리해야 함
- **GoFS의 호환성 문제**: 독자적 파일시스템 포맷을 사용하여 기존 호스트 파일시스템과 호환 불가
- **GPUfs의 성능 문제**: CPU를 통한 I/O로 GPU-initiated의 장점을 살리지 못함
- **핵심 딜레마**: 파일시스템 기능(호환성, 메타데이터 관리)과 GPU-initiated I/O 성능을 동시에 달성하기 어려움

### 2. 제안 방법 (Approach)
- **Companion 모델**: CPU 측 파일시스템(ext4 등)의 "동반자"로 동작. 메타데이터는 CPU가 관리, 데이터 I/O는 GPU가 직접 수행
- **제어/데이터 플레인 분리**:
  - **제어 플레인 (CPU)**: 파일 open/close, 메타데이터 lookup, extent 정보 조회, 권한 검사
  - **데이터 플레인 (GPU)**: extent 정보를 기반으로 GPU가 NVMe에 직접 I/O 발행
- **NVMe 드라이버 확장**: Linux 커널 NVMe 드라이버를 수정하여 GPU가 사용할 전용 I/O 큐를 할당. 기존 커널 NVMe 드라이버와 공존
- **공유 메타데이터 캐시**: CPU가 조회한 extent mapping을 GPU 접근 가능한 메모리에 캐시

### 3. 핵심 아키텍처/설계

```
┌─────────────────────────────────────────────────┐
│                 CPU Host                         │
│  ┌─────────────────────────────────────┐        │
│  │    Host File System (ext4/XFS)      │        │
│  │    - inode 관리                     │        │
│  │    - directory 관리                 │        │
│  │    - extent allocation              │        │
│  └──────────────┬──────────────────────┘        │
│                 │ extent map 공유                │
│  ┌──────────────▼──────────────────────┐        │
│  │  Shared Metadata Region             │        │
│  │  (CPU-GPU 공유 메모리)              │        │
│  │  file→extent→LBA 매핑 정보         │        │
│  └──────────────┬──────────────────────┘        │
└─────────────────┼───────────────────────────────┘
                  │ GPU가 매핑 정보 참조
┌─────────────────▼───────────────────────────────┐
│                 GPU                              │
│  ┌──────────────────────────────────────┐       │
│  │  GeminiFS GPU Runtime                │       │
│  │  - gfs_read(fd, buf, off, size)     │       │
│  │  - extent cache lookup              │       │
│  │  - NVMe cmd 작성 & doorbell         │       │
│  └──────────────┬───────────────────────┘       │
│                 │                                │
│  ┌──────────────▼───────────────────────┐       │
│  │  NVMe I/O Queues (GPU 전용)         │       │
│  │  (커널 NVMe 드라이버가 할당)         │       │
│  └──────────────┬───────────────────────┘       │
└─────────────────┼───────────────────────────────┘
                  │ PCIe P2P
             ┌────▼─────┐
             │ NVMe SSD │
             └──────────┘
```

**GeminiFS vs BaM vs GoFS 비교**:
```
          제어플레인    데이터플레인    FS 호환성
BaM       없음         GPU-initiated  없음(raw block)
GoFS      GPU          GPU-initiated  독자 포맷
GeminiFS  CPU(호스트FS) GPU-initiated  기존 FS 호환 ✓
```

- **핵심 차별점**: 기존 파일시스템의 메타데이터 관리를 그대로 활용하므로, CPU에서 만든 파일을 GPU에서 직접 읽을 수 있음
- **NVMe 큐 공유**: 커널 NVMe 드라이버가 일부 큐를 GPU 전용으로 할당. 나머지 큐는 CPU가 사용. 동일 SSD를 CPU와 GPU가 동시 접근 가능

### 4. 실험 결과 (Key Results)
- **GDS 대비 성능**: 대규모 random read에서 **2~5배** 성능 향상
- **BaM과 비교**: 파일시스템 오버헤드로 인해 raw block 접근보다 **5~15%** 느리지만, 사용성 대폭 개선
- **기존 파일시스템 호환**: ext4에서 생성한 파일을 GPU가 직접 읽기 가능 확인
- **그래프/데이터베이스 워크로드**: end-to-end 애플리케이션에서 GDS 대비 유의미한 성능 향상
- **메타데이터 캐시 효과**: 반복적으로 같은 파일에 접근하는 경우 CPU 메타데이터 조회 오버헤드 거의 제거

### 5. 한계점 및 향후 연구
- **CPU 의존성 잔존**: 제어 플레인이 CPU에 있으므로 파일 open/close 시 CPU 개입 필요. 초기 설정 단계에서 CPU가 병목이 될 수 있음
- **메타데이터 캐시 일관성**: CPU가 파일을 수정(truncate, append 등)하면 GPU의 캐시된 extent 정보가 stale해질 수 있음
- **커널 수정 필요**: NVMe 드라이버 확장을 위해 커널 패치 필요. 유지보수 부담
- **Write 지원 복잡성**: GPU에서 write 시 메타데이터(extent 할당) 업데이트를 CPU와 동기화해야 함
- **멀티 GPU 확장**: 여러 GPU가 동시에 같은 파일에 접근할 때의 확장성은 추가 검증 필요

## 다른 논문과의 관계
- 선행 연구: BaM [I1] (GPU-initiated I/O 인프라), GPUfs [P1] (GPU 파일시스템 개념)
- 후속 연구: 향후 GPU-native OS 연구의 기반
- 비교 대상: GoFS [I4] (독자 FS vs companion FS의 설계 철학 차이가 핵심 비교 포인트)
- 관련: NVIDIA GDS [G1] (GDS도 호스트 FS와 연동하지만 CPU-mediated)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure: 제어 플레인 / 데이터 플레인 분리 아키텍처 다이어그램
  - Figure: GeminiFS vs GDS vs BaM 성능 비교
  - Table: 기능 비교표 (FS 호환성, GPU-initiated, 메타데이터 관리 등)
- 핵심 수치/데이터:
  - GDS 대비 2~5배 성능 향상
  - BaM 대비 5~15% 오버헤드 (파일시스템 추상화 비용)
  - 기존 ext4 파일 직접 접근 가능

## 메모
- GeminiFS의 "companion" 접근법은 실용적: 기존 인프라(ext4, 커널 NVMe 드라이버)를 재활용하므로 채택 장벽이 낮음
- GoFS vs GeminiFS 비교는 "새로 만들기 vs 기존 확장" 트레이드오프의 좋은 사례 연구
