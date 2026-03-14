# [G6] Micron 9400 NVMe SSD Performance with GPUDirect Storage

- **학회/연도:** Micron 백서, 2023
- **저자:** Micron
- **분류:** GPUDirect Storage I/O

## 핵심 요약 (1~2문장)
Micron 9400 NVMe SSD에서 GDS를 활성화했을 때의 성능을 legacy I/O 경로와 비교 측정한 벤더 백서. 최대 1.5배 throughput 향상과 1.5배 응답시간 개선을 실측하며, SSD 하드웨어 특성이 GDS 성능에 미치는 영향을 분석.

## 읽기 전 질문
- Micron 9400의 어떤 하드웨어 특성이 GDS 성능에 유리한가?
- GDS에서 SSD의 큐 깊이(QD)와 I/O 크기가 성능에 어떤 영향을 미치는가?
- 벤더별 SSD(Micron vs KIOXIA vs Samsung)의 GDS 성능 차이 원인은?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- AI/HPC 워크로드에서 GPU 데이터 로딩이 전체 파이프라인 병목
- 기존 I/O 경로(CPU 바운스 버퍼)는 NVMe SSD의 성능을 충분히 활용하지 못함
- SSD 벤더 관점에서 자사 제품이 GDS와 얼마나 잘 동작하는지 검증 필요
- 고객(HPC 센터, 클라우드)에게 GDS 환경에서의 성능 가이드라인 제공 목적

### 2. 제안 방법 (Approach)
- **Micron 9400 NVMe SSD** (PCIe Gen4, 최대 7GB/s seq read) 사용
- 테스트 구성: NVIDIA A100/H100 GPU + Micron 9400
- 비교: Legacy I/O (POSIX + cudaMemcpy) vs GDS (cuFile API)
- 벤치마크: NVIDIA gdsio 도구 사용
- 파라미터: I/O 크기(4KB~4MB), 큐 깊이(1~128), sequential/random, read/write
- 단일 SSD 및 다중 SSD 구성 테스트

### 3. 핵심 아키텍처/설계

```
  테스트 구성:

  ┌─────────────────────────────────────────┐
  │           Test Server                    │
  │                                         │
  │  ┌──────────┐         ┌──────────────┐  │
  │  │ NVIDIA   │         │ Micron 9400  │  │
  │  │ A100/H100│         │ NVMe SSD     │  │
  │  │ GPU      │         │ (PCIe Gen4)  │  │
  │  └────┬─────┘         └──────┬───────┘  │
  │       │     PCIe Fabric      │          │
  │       └──────────┬───────────┘          │
  │                  │                       │
  │         ┌────────▼────────┐             │
  │         │   PCIe Switch   │             │
  │         │  (P2P DMA 지원)  │             │
  │         └─────────────────┘             │
  └─────────────────────────────────────────┘

  성능 비교 경로:
  Legacy: SSD →[DMA]→ CPU RAM →[copy]→ User buf →[cudaMemcpy]→ GPU
  GDS:    SSD →[P2P DMA]──────────────────────────────────────→ GPU

  Micron 9400 특성:
  ┌─────────────────────────────┐
  │ Micron 9400 NVMe SSD        │
  │ - PCIe Gen4 x4              │
  │ - Seq Read: ~7 GB/s         │
  │ - Random 4K Read: ~1.5M IOPS│
  │ - 용량: 7.68TB/15.36TB      │
  │ - E1.S / U.2 폼팩터         │
  └─────────────────────────────┘
```

### 4. 실험 결과 (Key Results)
- **Sequential Read**: GDS가 legacy 대비 **최대 ~1.5배** throughput 향상
- **응답 시간**: GDS에서 **~1.5배 개선** (바운스 버퍼 제거로 latency 감소)
- **I/O 크기별**:
  - 1MB 이상: GDS 이점 최대 — SSD 대역폭 거의 풀로 활용
  - 128KB~1MB: 유의미한 향상
  - 4KB~64KB: GDS 오버헤드로 이점 제한적
- **큐 깊이**: QD 증가 시 GDS와 legacy 모두 성능 향상, GDS가 더 빨리 saturation 도달
- **다중 SSD**: 4~8개 SSD 구성에서 aggregate throughput이 PCIe 대역폭 한계까지 선형 증가
- **CPU 사용률**: GDS에서 현저히 낮은 CPU 사용률 확인

### 5. 한계점 및 향후 연구
- 벤더 백서로서 자사 제품에 유리한 조건에서 테스트된 가능성
- 경쟁 SSD와의 직접 비교 없음 (Micron 제품만 테스트)
- 실제 AI 워크로드(학습 데이터 로딩 등)가 아닌 합성 벤치마크 위주
- PCIe Gen5 SSD에서의 결과 미포함 (Gen4 기준)
- 파일시스템 오버헤드, 메타데이터 영향 등 미분석
- GDS compatibility mode에서의 성능 분석 부재

## 다른 논문과의 관계
- 선행 연구: NVIDIA GDS 기술 문서
- 후속 연구: 차세대 Micron SSD + GDS 테스트
- 비교 대상: KIOXIA GDS [G8] — 동종 벤더 백서로 비교 가치 높음

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: I/O 크기별 GDS vs Legacy throughput 비교 그래프, 다중 SSD 스케일링 차트
- 핵심 수치/데이터: 최대 1.5배 throughput 향상, 1.5배 응답시간 개선, Micron 9400 스펙
- **활용**: SSD 벤더 관점의 GDS 성능 데이터, [G8] KIOXIA와 병렬 비교하여 "SSD 선택이 GDS 성능에 미치는 영향" 논의

## 메모
- 벤더 백서이므로 객관성에 유의. [G8] KIOXIA 백서와 함께 읽으면 벤더별 차이 파악 가능.
