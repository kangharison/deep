# NVMe Storage Stack 심층 분석 프로젝트

## 나의 목표
Linux Kernel의 NVMe 스택 전체를 깊이 이해하고
SPDK/DPDK와의 차이점을 코드 레벨에서 분석한다

## 나의 배경
- Linux 커널 드라이버 공부 시작 단계
- C 언어 기본 지식 있음
- 스토리지 시스템에 관심 많음

## 소스코드 위치
- 커널:     ~/sources/linux
- SPDK:     ~/sources/spdk
- DPDK:     ~/sources/dpdk
- nvme-cli: ~/sources/nvme-cli

## 공부 범위
1. PCIe Driver
2. NVMe Kernel Driver
3. Block Layer (blk-mq)
4. SPDK
5. DPDK
6. fio + ioengine

## 나의 공부 방식
- 코드 분석할 때는 함수 호출 체인을 추적해줘
- 커널 코드와 SPDK 코드를 항상 비교해서 설명해줘
- 어려운 개념은 그림(ASCII)으로 설명해줘
- 분석 후 notes/ 폴더에 정리할 내용 요약해줘

## 노트 위치
- notes/pcie.md
- notes/nvme.md
- notes/blk-mq.md
- notes/spdk.md
- notes/dpdk.md
- analysis/       ← 코드 분석 상세 내용
- scripts/        ← 자주 쓰는 명령어
