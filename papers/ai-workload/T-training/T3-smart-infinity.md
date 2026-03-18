# [T3] Smart-Infinity: Fast Large Language Model Training using Near-Storage Processing on a Real System

- **학회/연도:** HPCA 2024
- **저자:** (KAIST 등)
- **분류:** 트레이닝 스토리지 최적화

## 핵심 요약 (1~2문장)
Samsung SmartSSD(NVMe + FPGA)를 활용한 Near-Storage Processing으로 LLM 트레이닝을 가속하는 실제 시스템. 최대 10대의 SmartSSD에서 옵티마이저 업데이트 등 연산을 SSD 측 FPGA에서 수행하여 데이터 이동을 줄이고 트레이닝 속도를 향상시킨다.

## 읽기 전 질문
- Near-Storage Processing으로 LLM 트레이닝의 어떤 단계를 가속할 수 있는가?
- SmartSSD의 FPGA가 GPU 대비 어떤 연산에서 효율적인가?
- 실제 시스템에서의 성능과 프로그래밍 복잡도는?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- LLM 파라미터가 GPU 메모리에 담기지 않아 CPU/SSD로 오프로드 필요 (ZeRO-Infinity [P6] 계보)
- 오프로드 시 GPU↔CPU↔SSD 간 데이터 이동이 병목
- 데이터를 이동시키는 대신, 데이터가 있는 곳(SSD)에서 연산하면 이동 비용 절감

### 2. 제안 방법 (Approach)

```
기존 오프로드 (ZeRO-Infinity):
  SSD ──[읽기]──→ CPU ──[전송]──→ GPU ──[연산]──→ CPU ──[쓰기]──→ SSD

Smart-Infinity (Near-Storage):
  SSD+FPGA ──[로컬 연산: optimizer update]──→ 결과만 GPU로 전송
  (데이터 이동량 대폭 감소)
```

**하드웨어:**
- Samsung SmartSSD: 4TB NVMe SSD + Xilinx Kintex UltraScale+ FPGA
  - FPGA: ~522K LUTs, 984 BRAMs, 1968 DSPs, 4GB DDR4
  - SSD↔FPGA: PCIe Gen3 x4 내부 버스로 직접 통신
- 최대 10대 SmartSSD를 단일 호스트에 연결

**Near-Storage 연산:**
- Adam optimizer의 상태 업데이트를 FPGA에서 수행
- Gradient를 SSD 측으로 전송 → FPGA가 파라미터 업데이트 → 업데이트된 파라미터만 GPU로 반환
- PCIe 대역폭 사용량을 크게 절감

### 3. 실험 결과
- ZeRO-Infinity 대비 트레이닝 속도 향상
- 실제 하드웨어(SmartSSD)에서 구현 및 검증 — 시뮬레이션이 아닌 real system
- 10대 SmartSSD에서 스케일링 확인

### 4. 한계점 및 향후 연구
- SmartSSD의 FPGA 연산 능력이 제한적 — 복잡한 연산은 여전히 GPU 필요
- SmartSSD 전용 하드웨어 의존 — 범용성 부족
- FPGA 프로그래밍 복잡도
- Gen3 x4 대역폭 제한

## 다른 논문과의 관계
- 선행: ZeRO-Infinity [P6] (오프로드 개념), SPDK [P7] (유저스페이스 NVMe 접근)
- 유사 방향: [G1] BeaconGNN, [S4] INF² (In/Near-Storage Processing for AI)
- gpu-ssd 연결: BaM [I1]이 "GPU→SSD 직접 접근"이라면, Smart-Infinity는 "SSD에서 직접 연산" — 상호 보완적

## 발표 자료 활용 포인트
- "데이터를 이동시키지 말고, 연산을 데이터로 보내라" — Near-Data Processing의 핵심 철학
- SmartSSD 아키텍처 다이어그램이 발표에 임팩트 있음
- Real system 구현이라는 점이 차별점

## 메모
- CSD(Computational Storage Device) 트렌드의 대표적 논문
- GPU-initiated I/O(BaM)와 Near-Storage Processing(Smart-Infinity)은 "스토리지와 연산의 거리를 줄인다"는 같은 목표의 다른 접근
- Samsung이 SmartSSD를 상용화하고 있어 산업적 관심도 높음
