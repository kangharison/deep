# [R1] RAGX: In-Storage Acceleration of Retrieval-Augmented Generation as a Service

- **학회/연도:** ISCA 2025
- **저자:** (연구팀)
- **분류:** RAG 스토리지 가속

## 핵심 요약 (1~2문장)
RAG(Retrieval-Augmented Generation) 파이프라인의 검색/증강 단계를 SSD 내부에서 수행하는 In-Storage 가속기 아키텍처. 기존 CPU+NVMe 기반 검색 대비 최대 4.3배 end-to-end throughput 향상을 달성한다.

## 읽기 전 질문
- RAG 파이프라인에서 스토리지 병목은 어디에서 발생하는가?
- In-Storage에서 RAG의 어떤 연산을 가속할 수 있는가?
- 다양한 RAG 알고리즘에 대한 범용성은 어떻게 확보하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- RAG: LLM에 외부 지식을 검색하여 제공 → 할루시네이션 감소, 최신 정보 반영
- RAG 파이프라인: **Query → 벡터 검색(ANN) → 문서 검색 → 증강 → LLM 생성**
- 벡터 DB + 문서 저장소가 NVMe SSD에 위치 → 검색 단계의 I/O가 병목
- 수천 건/초의 ANN 검색 + Top-K 문서 읽기 = 대량의 작은 랜덤 I/O

### 2. 제안 방법 (Approach)

```
기존 RAG:
  NVMe SSD ──[벡터+문서 읽기]──→ CPU ──[ANN 검색]──→ GPU ──[LLM 생성]
                                  ↑ 병목: CPU ANN + PCIe

RAGX (In-Storage):
  NVMe SSD+가속기 ──[내부: ANN 검색 + 문서 추출]──→ 결과만 GPU로
  (검색 결과 문서만 전송, 전체 벡터 DB 전송 불필요)
```

**Metamorphic In-Storage 가속기:**
- 프로그래머블 아키텍처: 다양한 RAG 알고리즘, 데이터 구조, 연산 패턴 지원
- 벡터 유사도 검색(ANN), 문서 필터링, 증강(augmentation)을 SSD 내부에서 수행
- 동적 데이터 구조와 알고리즘 변경에 대응 가능

### 3. 실험 결과
- 기존 Xeon CPU + NVMe 대비 **최대 4.3× end-to-end throughput** 향상
- ISCA 2025(컴퓨터 아키텍처 최고 학회)에서 발표

### 4. 한계점 및 향후 연구
- 커스텀 하드웨어 필요 — 상용화까지 시간 필요
- 벡터 DB 업데이트(삽입/삭제) 시의 In-Storage 처리 복잡도
- 멀티모달 RAG(이미지+텍스트)에 대한 확장

## 다른 논문과의 관계
- 유사 방향: [G1] BeaconGNN, [S4] INF² (In-Storage AI 연산 계보)
- 워크로드 맥락: [R2] Athena (RAG 파이프라인 전체 벤치마크)
- gpu-ssd 연결: RAG의 벡터 검색이 BaM [I1]의 "불규칙 랜덤 읽기" 특성과 유사

## 발표 자료 활용 포인트
- ISCA 2025 — 아키텍처 최고 학회 발표로 권위 있음
- RAG가 LLM의 핵심 응용이므로 실용적 관심도 높음
- 4.3× throughput 향상 수치

## 메모
- RAG의 I/O 패턴: 벡터 유사도 검색(ANN) = 고차원 공간의 작은 랜덤 읽기 → GNN의 그래프 탐색과 I/O 특성이 유사
- In-Storage Computing 논문들(BeaconGNN, FlashGNN, Smart-Infinity, INF², RAGX)을 모으면 "AI를 위한 CSD" 서베이가 가능
