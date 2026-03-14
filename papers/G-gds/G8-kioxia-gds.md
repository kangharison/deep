# [G8] KIOXIA GPUDirect Storage Performance Report

- **학회/연도:** KIOXIA 백서, 2023~2024
- **저자:** KIOXIA
- **분류:** GPUDirect Storage I/O

## 핵심 요약 (1~2문장)
KIOXIA CM6/CD8 시리즈 NVMe SSD에서 GDS 성능을 측정한 벤더 백서. 다양한 I/O 패턴에서 GDS 활성화 시 legacy 대비 성능 향상을 실측하고, KIOXIA SSD의 GDS 최적 구성 가이드라인을 제공.

## 읽기 전 질문
- KIOXIA SSD의 어떤 특성(BiCS FLASH, 컨트롤러)이 GDS 성능에 영향을 미치는가?
- Micron [G6]의 결과와 비교했을 때 KIOXIA SSD의 GDS 성능은 어떻게 다른가?
- GDS에 최적화된 SSD 설정(큐 깊이, I/O 크기 등)은 벤더마다 다른가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- AI/HPC 워크로드에서 GPU로의 데이터 공급이 병목 — SSD 벤더로서 GDS 지원 검증 필요
- 기존 CPU 바운스 버퍼 경로는 SSD의 최대 성능을 활용하지 못함
- 고객에게 KIOXIA SSD + GDS 환경에서의 성능 기대치와 최적 구성 가이드 제공 목적
- 경쟁 벤더(Micron, Samsung 등) 대비 자사 제품의 GDS 경쟁력 입증

### 2. 제안 방법 (Approach)
- **테스트 SSD**: KIOXIA CM6 (PCIe Gen4, 엔터프라이즈), CD8 시리즈
- **테스트 GPU**: NVIDIA A100/H100
- **벤치마크 도구**: NVIDIA gdsio
- 비교 구성:
  - GDS 활성화 (cuFile, P2P DMA) vs Legacy (POSIX + cudaMemcpy)
  - 다양한 I/O 크기 (4KB ~ 4MB)
  - Sequential / Random 패턴
  - 단일 SSD vs 다중 SSD 구성
  - 큐 깊이 변화 (1 ~ 128+)

### 3. 핵심 아키텍처/설계

```
  테스트 환경:

  ┌──────────────────────────────────────────┐
  │           KIOXIA GDS Test Setup          │
  │                                          │
  │  ┌──────────┐      ┌─────────────────┐   │
  │  │ NVIDIA   │      │ KIOXIA CM6/CD8  │   │
  │  │ A100/H100│      │ NVMe SSD        │   │
  │  │ 80GB HBM │      │ PCIe Gen4 x4    │   │
  │  └────┬─────┘      └───────┬─────────┘   │
  │       │   PCIe Fabric      │             │
  │       └─────────┬──────────┘             │
  │                 │                         │
  │        ┌────────▼────────┐               │
  │        │ CPU / PCIe Root │               │
  │        │   Complex       │               │
  │        └─────────────────┘               │
  └──────────────────────────────────────────┘

  KIOXIA SSD 제품 라인업:
  ┌──────────────────────────────────────┐
  │ CM6 시리즈 (엔터프라이즈)             │
  │ - PCIe Gen4 x4                       │
  │ - Seq Read: ~6.9 GB/s               │
  │ - Random 4K Read: ~1.4M IOPS        │
  │ - BiCS FLASH TLC                     │
  │ - U.2/U.3/E1.S 폼팩터               │
  ├──────────────────────────────────────┤
  │ CD8 시리즈 (데이터센터)               │
  │ - PCIe Gen4/Gen5 x4                  │
  │ - Seq Read: ~7.1 GB/s (Gen4)        │
  │ -          ~12 GB/s (Gen5)           │
  │ - E1.S/E3.S 폼팩터                   │
  └──────────────────────────────────────┘

  성능 비교:
  ┌─────────────────────────────────────────┐
  │  I/O Size  │  Legacy   │    GDS        │
  │            │ (GB/s)    │   (GB/s)      │
  ├────────────┼───────────┼───────────────┤
  │  128KB     │    ~3.5   │    ~4.5       │
  │  512KB     │    ~4.5   │    ~6.0       │
  │  1MB       │    ~5.0   │    ~6.5       │
  │  4MB       │    ~5.2   │    ~6.8       │
  └─────────────────────────────────────────┘
  (수치는 대략적 참고값, 실제 백서의 정확한 수치 확인 필요)
```

### 4. 실험 결과 (Key Results)
- **Sequential Read**: GDS가 legacy 대비 **~30-40% throughput 향상** (I/O 크기에 따라 상이)
- **대용량 I/O (1MB+)**: SSD 대역폭의 거의 최대치에 근접하는 성능
- **Random Read**: GDS에서도 유의미한 향상, 특히 큰 블록 크기에서
- **다중 SSD**: 4~8개 KIOXIA SSD 병렬 사용 시 aggregate throughput 스케일링 확인
- **CPU 사용률**: GDS 활성화 시 CPU 부하 감소 확인
- **Gen5 SSD (CD8P)**: Gen4 대비 더 큰 GDS 이점 — 높은 SSD 대역폭을 GDS가 더 잘 활용

### 5. 한계점 및 향후 연구
- 벤더 백서로 자사 제품 중심 — 타 벤더 SSD와의 직접 비교 없음
- 합성 벤치마크(gdsio) 위주 — 실제 AI 워크로드 미검증
- PCIe 토폴로지 최적화 상태에서만 테스트 — 비최적 토폴로지에서의 성능 저하 미분석
- GDS compatibility mode 성능 미포함
- 파일시스템 오버헤드, 캐시 효과 등 실환경 요소 미반영
- Gen5 SSD 테스트가 제한적 — Gen5의 잠재력을 충분히 보여주지 못할 수 있음

## 다른 논문과의 관계
- 선행 연구: NVIDIA GDS 기술 문서
- 후속 연구: KIOXIA Gen5 SSD + GDS 확장 테스트
- 비교 대상: Micron GDS [G6] — 동종 벤더 백서, 직접 비교 가치 높음

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: I/O 크기별 GDS vs Legacy 비교, 다중 SSD 스케일링 그래프
- 핵심 수치/데이터: CM6/CD8 GDS throughput, Gen4 vs Gen5 비교
- **활용**: [G6] Micron과 병렬 비교하여 "SSD 하드웨어가 GDS 성능에 미치는 영향" 분석, 벤더별 GDS 성능 격차 논의

## 메모
- [G6] Micron 백서와 함께 읽어서 벤더 간 GDS 성능 차이와 공통 패턴을 파악하는 것이 효과적.
- Gen5 SSD의 GDS 성능은 향후 더 중요해질 것 — 대역폭 증가에 따라 바운스 버퍼 오버헤드가 더 커지므로 GDS 이점도 증가 예상.
