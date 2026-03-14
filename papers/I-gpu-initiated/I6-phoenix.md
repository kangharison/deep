# [I6] Phoenix: A Refactored I/O Stack for GPU Direct Storage without Phony Buffers

- **학회/연도:** SC 2025
- **저자:** Yan et al.
- **분류:** GPU Initiated I/O

## 핵심 요약 (1~2문장)
NVIDIA GDS의 "phony buffer" 문제(CPU 측 bounce buffer를 할당하지만 실제로 사용하지 않는 낭비)를 근본적으로 해결하기 위해 Linux I/O 스택을 리팩토링. 페이지 캐시, bio 레이어 등 기존 커널 I/O 경로에서 불필요한 CPU 메모리 할당을 제거하고, 로컬 NVMe뿐 아니라 네트워크 스토리지(NVMe-oF 등)에서도 GPU 메모리로의 직접 전송을 지원.

## 읽기 전 질문
- "Phony buffer"란 정확히 무엇이고, 왜 발생하는가? 기존 Linux I/O 스택의 어떤 가정 때문인가?
- I/O 스택 리팩토링이 기존 커널 코드에 얼마나 침습적(invasive)인가?
- 네트워크 스토리지(NVMe-oF, NFS 등)까지 확장할 때 추가되는 복잡성은?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **GDS의 Phony Buffer 문제**:
  - NVIDIA GDS는 SSD에서 GPU 메모리로 직접 DMA 전송(GPUDirect Storage)을 수행
  - 그러나 Linux 커널 I/O 스택은 **모든 I/O에 CPU 측 페이지/버퍼가 존재한다고 가정**
  - 결과: GDS가 I/O를 발행할 때 실제로는 사용하지 않을 CPU 메모리 페이지를 "phony(가짜)"로 할당해야 함
  - 이 phony buffer는 DMA 완료 후 즉시 폐기되지만, 할당/해제 오버헤드와 메모리 낭비 발생
- **페이지 캐시 비호환**: Linux 페이지 캐시는 CPU 메모리 페이지를 전제로 설계. GPU 메모리 페이지를 캐시에 넣을 수 없음
- **네트워크 스토리지 미지원**: 기존 GDS는 로컬 NVMe에 최적화. 원격 스토리지(NVMe-oF, Lustre 등)에서의 GPU 직접 전송은 I/O 스택의 여러 레이어를 통과해야 하여 phony buffer 문제가 더 심각

### 2. 제안 방법 (Approach)
- **I/O 스택 리팩토링**: Linux 커널의 block layer, 파일시스템 layer를 수정하여 "buffer-less I/O 경로" 추가
- **핵심 수정 사항**:
  1. **bio 구조체 확장**: 기존 bio는 CPU 페이지를 참조하도록 설계. GPU 메모리 주소(DMA 가능 주소)를 직접 참조할 수 있도록 확장
  2. **페이지 캐시 우회**: GPU 대상 I/O는 페이지 캐시를 건너뛰고 Direct I/O 경로 사용 (O_DIRECT와 유사)
  3. **NVMe 드라이버 수정**: PRP/SGL에 GPU 물리 주소를 직접 설정하여 bounce buffer 없이 DMA
- **네트워크 스토리지 지원**: RDMA 기반 전송에서도 GPU 메모리를 target으로 직접 지정

### 3. 핵심 아키텍처/설계

```
기존 GDS I/O 경로 (Phony Buffer 문제):
┌─────────┐     ┌──────────┐     ┌──────────┐     ┌─────────┐
│ App     │────→│ VFS/FS   │────→│ Block    │────→│ NVMe    │
│ (cuFile)│     │ Layer    │     │ Layer    │     │ Driver  │
└─────────┘     └──────────┘     └──────────┘     └────┬────┘
                     │                │                  │
                     ▼                ▼                  │
               ┌──────────┐    ┌──────────┐             │
               │ Phony    │    │ Phony    │             │
               │ Page     │    │ bio page │  실제 안 씀  │
               │ Cache    │    │ alloc    │  but 할당    │
               └──────────┘    └──────────┘             │
                                                        ▼
                                                   GPU Memory
                                                   (실제 DMA 목적지)

Phoenix 리팩토링된 I/O 경로:
┌─────────┐     ┌──────────┐     ┌──────────┐     ┌─────────┐
│ App     │────→│ VFS/FS   │────→│ Block    │────→│ NVMe    │
│         │     │ (수정)   │     │ (수정)   │     │ (수정)  │
└─────────┘     └──────────┘     └──────────┘     └────┬────┘
                     │                │                  │
                     │ GPU addr       │ GPU addr         │
                     │ 직접 전달      │ bio에 직접       │
                     │ (phony 없음)   │ GPU addr 포함    │
                     ▼                ▼                  ▼
                ┌────────────────────────────────────────────┐
                │        GPU Memory (직접 DMA 목적지)        │
                │        Phony Buffer 할당 없음!            │
                └────────────────────────────────────────────┘
```

**네트워크 스토리지 확장**:
```
┌──────────┐         ┌──────────────┐         ┌──────────┐
│ GPU      │◀──RDMA──│ Network      │◀────────│ Remote   │
│ Memory   │  직접   │ (RoCE/IB)   │         │ Storage  │
│          │  전송   │ GPU addr를   │         │ (NVMe-oF)│
└──────────┘         │ RDMA target  │         └──────────┘
                     │ 으로 지정    │
                     └──────────────┘
```

### 4. 실험 결과 (Key Results)
- **Phony buffer 제거 효과**:
  - CPU 메모리 사용량: 대규모 I/O 시 GDS 대비 **수 GB** 의 phony buffer 메모리 절약
  - I/O 지연시간: phony buffer 할당/해제 오버헤드 제거로 **10~30%** 지연시간 감소
- **로컬 NVMe 성능**: GDS 대비 **1.1~1.3배** 처리량 향상 (오버헤드 제거 효과)
- **네트워크 스토리지**: NVMe-oF에서 GPU 직접 전송 성능을 최초로 정량적으로 측정. CPU bounce buffer 경유 대비 **1.5~2배** 대역폭 향상
- **확장성**: 다수의 동시 I/O 스트림에서 phony buffer로 인한 메모리 압박 없이 안정적 성능 유지

### 5. 한계점 및 향후 연구
- **커널 수정 범위가 넓음**: VFS, block layer, NVMe 드라이버, 네트워크 스택 등 다수의 커널 서브시스템 수정 필요. 업스트림 머지 어려움
- **페이지 캐시 활용 불가**: GPU 대상 I/O는 페이지 캐시를 완전히 우회하므로, 반복 접근 시 캐시 이점 없음 (BaM의 소프트웨어 캐시와 유사한 별도 메커니즘 필요)
- **파일시스템 호환성**: 모든 파일시스템에서 동작하지 않을 수 있음 (ext4, XFS 등 개별 수정 필요)
- **GPU-initiated 아님**: 여전히 CPU가 I/O를 orchestrate (GDS 개선). BaM/GoFS와는 다른 접근
- **보안/격리**: GPU 메모리 주소를 커널에 전달하는 과정에서의 보안 검증 필요

## 다른 논문과의 관계
- 선행 연구: GDS [G1] (phony buffer 문제의 원인), SPIN [G2] (커널 I/O 스택 최적화)
- 후속 연구: Linux 커널 upstream 기여 가능성
- 비교 대상: BaM [I1] (커널 우회 vs 커널 리팩토링의 대비), GDS [G1] (개선 대상)
- 의의: GPU-initiated I/O(BaM 계열)와는 다른 접근으로, 기존 I/O 스택을 GPU-friendly하게 개선하는 방향

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure: Phony buffer 문제를 설명하는 I/O 경로 다이어그램 (before/after)
  - Figure: GDS vs Phoenix 메모리 사용량 비교
  - Table: 로컬/네트워크 스토리지에서의 성능 비교
- 핵심 수치/데이터:
  - Phony buffer 제거로 10~30% 지연시간 감소
  - 네트워크 스토리지에서 1.5~2배 대역폭 향상
  - 수 GB 메모리 절약

## 메모
- Phoenix는 "커널을 고쳐서 GPU 스토리지 접근을 개선"하는 접근. BaM은 "커널을 우회"하는 접근. 상호 보완적
- Phony buffer 문제는 Linux I/O 스택이 GPU 시대에 맞지 않게 설계되었음을 잘 보여주는 사례
- 장기적으로는 Linux 커널이 GPU 메모리를 first-class citizen으로 다루는 방향으로 진화해야 함
