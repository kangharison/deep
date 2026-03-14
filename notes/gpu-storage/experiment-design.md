# GPU가 PCIe 기반 NVMe SSD에 미치는 영향 분석 — 실험 설계안

발표 주제에 맞춘 5가지 실험 설계와 기대 결과 정리.

---

## 실험 1: PCIe 대역폭 경합 (GPU 연산 vs NVMe I/O 동시 수행)

**목적**: GPU가 PCIe 버스를 점유할 때 NVMe SSD 성능이 얼마나 떨어지는가

**방법**:
- Baseline: GPU idle 상태에서 `fio`로 NVMe 순수 성능 측정 (4KB random read, 128KB sequential read/write)
- 부하 단계: GPU에서 `cudaMemcpy` (H2D/D2H)를 동시에 돌리면서 동일 fio 워크로드 실행
- 변수: GPU 전송 크기 (64MB, 256MB, 1GB), fio I/O 크기 (4KB, 64KB, 128KB, 1MB)

**기대 결과**:
- GPU `cudaMemcpy`가 PCIe 대역폭을 거의 독점 → NVMe throughput **30~60% 하락** 예상
- 특히 sequential read에서 하락폭이 클 것 (둘 다 대역폭 의존적)
- 4KB random read는 IOPS 위주라 상대적으로 영향 적을 것
- same PCIe switch 아래에 GPU와 NVMe가 있으면 영향 극대화, cross-switch면 완화

---

## 실험 2: PCIe 토폴로지 영향 (Same Switch vs Cross Switch)

**목적**: GPU와 NVMe SSD의 PCIe 토폴로지 배치가 성능에 미치는 영향

**방법**:
- `nvidia-smi topo -m`과 `lspci -tv`로 토폴로지 확인
- Case A: GPU와 NVMe가 같은 PCIe switch 아래
- Case B: GPU와 NVMe가 다른 PCIe switch (CPU root complex 경유)
- 각 케이스에서 GDS(`gdsio`) 또는 일반 `cudaMemcpy` + `fio` 동시 수행

**기대 결과**:
- Same switch: P2P DMA 시 latency 낮고 throughput 높음
- Cross switch: CPU root complex 경유로 **latency 1.5~2배 증가**, throughput **20~40% 감소** (논문 G5, I1, I7 참조)
- GDS 사용 시 same switch에서 최대 효과 (바운스 버퍼 제거 + 짧은 경로)

---

## 실험 3: GDS vs 전통 경로 (bounce buffer) 비교

**목적**: GPUDirect Storage가 NVMe→GPU 전송에서 CPU 부하와 throughput에 미치는 영향

**방법**:
- 전통 경로: `pread()` → `cudaMemcpy()` (CPU 바운스 버퍼 경유)
- GDS 경로: `cuFileRead()` (NVMe → GPU 직접 P2P DMA)
- I/O 크기별 비교: 4KB, 64KB, 128KB, 512KB, 1MB, 4MB
- 측정: throughput, CPU 사용률(`mpstat`), GPU-side latency

**기대 결과**:

```
I/O 크기        전통 경로       GDS 경로       개선율
─────────────────────────────────────────────────────
4KB             ~0.5 GB/s       ~0.3 GB/s      오히려 GDS가 느림 (오버헤드)
64KB            ~2 GB/s         ~2.5 GB/s      ~25% 개선
128KB           ~3 GB/s         ~5 GB/s        ~67% 개선  ← GDS 이점 본격화
1MB             ~4 GB/s         ~6+ GB/s       ~50%+ 개선
CPU 사용률      70~90%          10~30%         CPU 해방 효과 극대화
```

- **핵심 발견**: GDS 이점은 128KB 이상에서 본격화 (논문 G5, G6, G8과 일치)
- 소규모 I/O에서는 GDS 셋업 오버헤드가 전송 시간보다 커서 역효과

---

## 실험 4: Queue Depth에 따른 GPU-NVMe 동시 접근 성능

**목적**: NVMe의 deep queue를 GPU 워크로드가 얼마나 효과적으로 활용하는가

**방법**:
- `fio`의 `iodepth` 파라미터 변화: 1, 4, 16, 64, 128, 256
- io_engine 비교: `libaio`, `io_uring`(SQPOLL/IOPOLL), `spdk`
- GPU 부하 유/무 상태에서 각각 측정

**기대 결과**:

```
QD     libaio      io_uring(SQPOLL)    SPDK
──────────────────────────────────────────────
1      ~50K IOPS   ~80K IOPS           ~200K IOPS
16     ~300K IOPS  ~500K IOPS          ~800K IOPS
128    ~400K IOPS  ~700K IOPS          ~1M+ IOPS
256    포화         포화                 ~1M+ IOPS
```

- GPU 동시 부하 시: 커널 경로(libaio)는 CPU 경합으로 **20~40% IOPS 하락**, SPDK는 CPU 코어를 전용으로 쓰므로 영향 **5~10%**로 제한적

---

## 실험 5: AI 학습 워크로드에서 NVMe 오프로드 영향

**목적**: 실제 GPU 학습 중 NVMe SSD로 activation/checkpoint를 오프로드할 때 성능 영향

**방법**:
- PyTorch + ResNet-50 또는 GPT-2 small 학습
- Case A: GPU 메모리만 사용 (baseline)
- Case B: DeepSpeed ZeRO-Offload로 NVMe 오프로드 활성화
- Case C: activation checkpointing (recomputation)
- 측정: samples/sec, GPU 활용률, NVMe read/write throughput, 학습 정확도

**기대 결과**:
- Case B는 Case A 대비 throughput **10~30% 감소**하지만 배치 크기 **2~4배 확장** 가능
- Case C(recomputation)는 NVMe I/O 없이 GPU 재계산으로 **30~40% 느림**
- NVMe 오프로드(Case B)가 recomputation(Case C)보다 빠를 수 있음 (논문 O1 참조: 2.5x 속도 향상)
- 학습 정확도는 세 케이스 모두 **동일** (오프로드는 정확도에 영향 없음)

---

## 실험 요약 매트릭스

```
실험                        난이도    필요 장비              핵심 메시지
───────────────────────────────────────────────────────────────────────────
1. PCIe 대역폭 경합          ★★☆     GPU + NVMe + fio       GPU가 NVMe 성능을 잠식
2. PCIe 토폴로지             ★★★     다중 PCIe switch       물리 배치가 성능 좌우
3. GDS vs 전통 경로          ★★★     GDS 지원 드라이버      바운스 버퍼 제거 효과
4. Queue Depth 스케일링      ★★☆     fio + 다중 io_engine   NVMe 병렬성 활용도
5. AI 학습 오프로드          ★★★     PyTorch + DeepSpeed    실용적 GPU-NVMe 시너지
```

---

## 발표 스토리라인

1. **실험 1**로 문제 제기 — GPU가 NVMe를 방해한다
2. **실험 2**로 원인 분석 — PCIe 토폴로지가 핵심
3. **실험 3~4**로 해결책 탐색 — GDS, SPDK 등
4. **실험 5**로 실용적 가치 — AI 학습에서의 활용

"문제 → 원인 → 해결 → 응용" 구성.

---

## 참고 논문

- PCIe 토폴로지 영향: G5 (NIXL), I1 (BaM), I7 (GPU-Initiated Survey)
- GDS I/O 크기별 성능: G5, G6 (Micron GDS), G8 (KIOXIA GDS)
- Queue Depth 영향: C1, C2, G5, G6, G8
- CPU 효율: C1, C2, C4
- AI 오프로드: O1 (SSDTrain), O2 (TERAIO), P5 (FlashNeuron), P6 (ZeRO-Infinity)
