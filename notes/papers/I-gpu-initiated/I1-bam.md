# [I1] GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture ★필독

- **학회/연도:** ASPLOS 2023
- **저자:** Zaid Qureshi, Vikram Sharma Mailthody, Isaac Gelado et al. (UIUC, NVIDIA, IBM)
- **분류:** GPU Initiated I/O

## 핵심 요약 (1~2문장)
GPU Initiated I/O의 원조 논문. GPU 커널이 NVMe SQ에 직접 커맨드를 작성하고 doorbell을 발행하는 전체 아키텍처와 구현. GPU VRAM에 NVMe 큐와 I/O 버퍼를 배치하고 GPUDirect RDMA API를 활용.

## 읽기 전 질문
- 이 논문에서 답을 얻고 싶은 질문:

## 주요 내용 정리

### 1. 문제 정의 (Problem)

### 2. 제안 방법 (Approach)

### 3. 핵심 아키텍처/설계

### 4. 실험 결과 (Key Results)

### 5. 한계점 및 향후 연구

## 다른 논문과의 관계
- 선행 연구: GPUfs [P1], EMOGI [P4], FlashGPU [P3], SPDK [P7]
- 후속 연구: GIDS [I2], AGILE [I3], GoFS [I4], GeminiFS [I5], Phoenix [I6]
- 비교 대상: GDS [G1]

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
- 핵심 수치/데이터:

## 메모
