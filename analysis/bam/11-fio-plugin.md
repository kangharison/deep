# BaM fio 플러그인 분석 (deprecated/fio/fio_plugin.c)

**소스코드:** `sources/bam/deprecated/fio/fio_plugin.c`
**상태:** deprecated (더 이상 유지보수되지 않음)
**목적:** fio 벤치마크 도구에서 BaM의 libnvm 라이브러리를 ioengine으로 사용하여, **CPU 유저스페이스에서 NVMe 큐를 직접 조작**하는 I/O를 fio로 측정

## 1. 이 플러그인의 위치: GPU가 아닌 CPU 유저스페이스

이 플러그인은 **GPU-initiated I/O가 아니야.** BaM의 libnvm 라이브러리를 CPU에서 사용하는 SPDK 스타일의 유저스페이스 I/O 엔진이다.

```
BaM 코드베이스의 두 가지 사용 모드:

  ① GPU-initiated I/O (benchmarks/*.cu)
     GPU 커널이 NVMe 큐를 직접 조작
     SQ/CQ: GPU VRAM
     → BaM 논문의 핵심

  ② CPU 유저스페이스 I/O (deprecated/fio/fio_plugin.c) ← 이 파일
     CPU 스레드가 NVMe 큐를 직접 조작 (SPDK와 동일한 방식)
     SQ/CQ: Host DRAM
     → fio 벤치마크용 보조 도구, deprecated됨
```

## 2. fio ioengine 구조

fio는 플러그인 방식으로 I/O 엔진을 교체할 수 있다. 이 플러그인은 `struct ioengine_ops`에 콜백 함수들을 등록한다.

**파일:** `fio_plugin.c:442-458`

```c
struct ioengine_ops engine = {
    .name        = "libnvm",
    .version     = FIO_IOOPS_VERSION,
    .flags       = FIO_SYNCIO | FIO_RAWIO | FIO_NOEXTEND
                 | FIO_NODISKUTIL | FIO_MEMALIGN,
    .setup       = thread_setup,     // fio 스레드 사전 검증
    .init        = thread_init,      // NVMe 초기화 + 큐 생성
    .queue       = fio_queue,        // ★ 실제 I/O 발행 (핵심)
    .cleanup     = thread_cleanup,   // 큐 삭제 + 자원 해제
    .open_file   = fio_open,         // no-op (raw device)
    .close_file  = fio_close,        // no-op
    .invalidate  = fio_invalidate,   // no-op
    .iomem_alloc = iomem_alloc,      // DMA 버퍼 할당
    .iomem_free  = iomem_free,       // DMA 버퍼 해제
    .options     = options,          // fio 커맨드라인 옵션
};
```

**플래그 의미:**

| 플래그 | 의미 |
|--------|------|
| `FIO_SYNCIO` | 동기 I/O — queue()가 완료까지 대기 후 반환 |
| `FIO_RAWIO` | raw 블록 디바이스 접근 (파일시스템 없음) |
| `FIO_NOEXTEND` | 파일 크기 확장 없음 |
| `FIO_NODISKUTIL` | 디스크 utilization 통계 수집 안 함 |
| `FIO_MEMALIGN` | 메모리 정렬 요구 |

## 3. 초기화 흐름: thread_init()

**파일:** `fio_plugin.c:301-410`

fio의 각 워커 스레드가 시작할 때 호출된다.

```
thread_init(td)
  │
  ├── nvm_dis_ctrl_init(&ctrl, fdid)
  │   → DIS(Dolphin Interconnect) 클러스터 경유로 NVMe 컨트롤러 접근
  │   → 로컬이 아닌 원격 NVMe도 지원 (SmartIO/SISCI)
  │
  ├── opts->reset인 경우:
  │   ├── Admin Queue 메모리 할당 (nvm_dis_dma_create, 2 페이지)
  │   ├── nvm_aq_create() → 컨트롤러 리셋 + Admin Queue 생성
  │   └── nvm_admin_set_num_queues() → 큐 수 설정
  │
  ├── opts->reset이 아닌 경우:
  │   └── nvm_dis_rpc_bind() → 이미 초기화된 컨트롤러에 RPC로 연결
  │       (다른 프로세스가 Admin을 소유한 경우)
  │
  ├── identify(data)
  │   → Identify Controller + Identify Namespace
  │   → ctrl_info, ns_info 획득 (LBA 크기 등)
  │
  ├── nvm_admin_get_num_queues() → 사용 가능한 큐 수 조회
  │
  └── create_queues(data, queue_id, iodepth, flags)
      ├── CQ 메모리 할당 (Host DRAM, DIS DMA)
      ├── SQ 메모리 할당 (Host DRAM, DIS DMA)
      ├── nvm_admin_cq_create() → CQ 생성
      ├── nvm_admin_sq_create() → SQ 생성 (CQ와 연결)
      └── PRP 리스트 초기화
```

**핵심 차이점 — GPU 버전(ctrl.h)과 비교:**

| 항목 | fio 플러그인 | GPU 버전 (ctrl.h) |
|------|:-:|:-:|
| SQ/CQ 메모리 | Host DRAM (`nvm_dis_dma_create`) | GPU VRAM (`cudaMalloc + P2P`) |
| DMA 방식 | DIS/SISCI 클러스터 DMA | NVIDIA P2P API |
| 도어벨 접근 | CPU writel | GPU st.mmio |
| 큐 조작 API | `nvm_sq_enqueue` (단일 스레드) | `sq_enqueue` (lock-free 병렬) |
| 원격 NVMe | 지원 (SmartIO) | 미지원 (로컬 P2P만) |

## 4. I/O 발행: fio_queue() — 핵심 함수

**파일:** `fio_plugin.c:187-245`

fio가 I/O 요청 하나를 처리할 때 호출된다. **동기식(sync) — 완료까지 폴링 후 반환.**

```c
static enum fio_q_status fio_queue(struct thread_data* td, struct io_u* io_u)
{
    // ① fio의 io_u에서 I/O 방향 결정
    switch (io_u->ddir) {
        case DDIR_READ:  op = NVM_IO_READ;  break;
        case DDIR_WRITE: op = NVM_IO_WRITE; break;
    }

    // ② NVMe 커맨드 구성
    idx = sq->tail;                    // 현재 SQ tail = Command ID로 사용
    nvm_cmd_header(&cmd, idx, op, ns_id);
    nvm_cmd_data(&cmd, ...);           // PRP 주소 설정 (Host DRAM DMA 버퍼)
    nvm_cmd_rw_blks(&cmd,
        io_u->offset / bs,            // 시작 LBA = fio offset / LBA 크기
        io_u->xfer_buflen / bs);      // 블록 수 = 전송 크기 / LBA 크기

    // ③ SQ에 커맨드 삽입
    ptr = nvm_sq_enqueue(sq);          // SQ tail 슬롯 포인터 획득
    if (ptr == NULL) return FIO_Q_BUSY; // SQ full
    *ptr = cmd;                        // 64B 커맨드 복사

    // ④ 도어벨 링
    nvm_sq_submit(sq);                 // CPU writel(tail, sq->db)

    // ⑤ CQ 폴링 (완료까지 busy-wait) ★ 동기식
    while ((cpl = nvm_cq_dequeue(cq)) == NULL);

    // ⑥ SQ head 갱신 + 에러 체크
    nvm_sq_update(sq);
    if (!NVM_ERR_OK(cpl)) {
        fprintf(stderr, "error %s\n", nvm_strerror(NVM_ERR_STATUS(cpl)));
    }

    // ⑦ CQ 도어벨 갱신
    nvm_cq_update(cq);

    return FIO_Q_COMPLETED;  // fio에 "완료" 반환
}
```

**이 I/O 경로를 그림으로:**

```
fio 워커 스레드 (CPU)
  │
  ├── ② nvm_cmd_header + nvm_cmd_rw_blks → NVMe 커맨드 64B 구성
  ├── ③ nvm_sq_enqueue → SQ tail 슬롯에 커맨드 복사
  ├── ④ nvm_sq_submit  → CPU writel 도어벨 → NVMe 컨트롤러에 통지
  │         │
  │         ▼
  │    NVMe 컨트롤러:
  │      SQ에서 cmd fetch (Host DRAM DMA)
  │      NAND에서 데이터 읽기
  │      PRP 주소(Host DRAM)로 데이터 DMA
  │      CQ에 완료 기록 (Host DRAM DMA)
  │         │
  │         ▼
  ├── ⑤ while(nvm_cq_dequeue(cq) == NULL) → CQ 폴링 (CPU busy-wait)
  ├── ⑥ nvm_sq_update → SQ head 갱신
  └── ⑦ nvm_cq_update → CQ 도어벨 → NVMe에 CQ head 통지

  → 전부 Host DRAM, 전부 CPU, 전부 유저스페이스
  → 커널 블록 레이어 우회 (SPDK와 동일)
  → GPU 관여 없음
```

## 5. GPU 버전 vs fio 플러그인 I/O 경로 비교

```
fio 플러그인 (이 파일):             GPU-initiated (benchmarks/*.cu):

  CPU thread                        GPU warp (32 threads)
    │                                 │
  nvm_sq_enqueue() ← 단일 스레드     sq_enqueue() ← 수천 warp 동시
  *ptr = cmd       ← memcpy          ulonglong4×2 ← GPU 레지스터
  nvm_sq_submit()  ← writel          st.mmio      ← PTX 인라인
    │                                 │
  while(cq==NULL)  ← CPU spin        cq_poll()    ← GPU spin + nanosleep
  nvm_sq_update()  ← 단순 갱신       cq_dequeue() ← lock-free + 배칭
  nvm_cq_update()  ← writel          st.mmio      ← 배칭된 도어벨
    │                                 │
  Host DRAM                          GPU VRAM
  1 thread / 1 SQ                    N warp / 1 SQ (ticket 동기화)
```

## 6. DIS 클러스터 의존성

이 플러그인이 deprecated된 핵심 이유 중 하나. **Dolphin Interconnect Solutions(DIS)의 SmartIO/SISCI API에 의존:**

```c
#define __DIS_CLUSTER__
#include <sisci_api.h>

// 초기화
SCIInitialize(0, &err);          // SISCI 라이브러리 초기화

// 컨트롤러 접근
nvm_dis_ctrl_init(&ctrl, fdid);  // DIS fabric device ID로 NVMe 접근

// DMA 메모리
nvm_dis_dma_create(&mem, ...);   // DIS 공유 메모리로 DMA 버퍼 생성

// RPC
nvm_dis_rpc_bind(&aq_ref, ...);  // DIS를 통한 원격 Admin Queue 접근
```

DIS SmartIO는 **PCIe fabric을 통해 원격 노드의 NVMe에 접근**할 수 있는 상용 하드웨어/소프트웨어. 일반 환경에는 없으므로 이 플러그인을 그대로 빌드할 수 없다.

## 7. fio 커맨드라인 옵션

**파일:** `fio_plugin.c:430-438`

```
fio --ioengine=external:./fio_plugin.o \
    --fdid=0x10000          ← DIS fabric device ID (NVMe 컨트롤러)
    --reset                 ← 컨트롤러 리셋 + Admin Queue 소유
    --queue_no=1            ← 사용할 I/O Queue ID
    --remote_queue          ← SQ를 컨트롤러 근처 메모리에 할당
    --namespace=1           ← NVMe 네임스페이스 ID
    --adapter=0             ← DIS 어댑터 번호
```

## 8. 제한사항

```c
// fio_plugin.c:320-323
if (td->o.iodepth != 1)
{
    fprintf(stderr, "Warning: IO depth > 1 is not supported\n");
}
```

- **iodepth=1만 지원** — 동기식(FIO_SYNCIO) 플래그로 인해 한 번에 1개 I/O만
- DIS/SISCI 하드웨어 필수 — 일반 로컬 NVMe에서 사용 불가
- GPU 관여 없음 — CPU 유저스페이스 전용
- deprecated — 현재 BaM 프로젝트에서 유지보수 안 함

## 9. 요약: 이 파일의 의미

| 질문 | 답 |
|------|---|
| GPU-initiated I/O인가? | **아니오** — CPU 유저스페이스 I/O |
| 커널 블록 레이어를 거치나? | **아니오** — 유저스페이스에서 NVMe 큐 직접 조작 |
| SPDK와 비슷한가? | **매우 비슷** — CPU 폴링 기반 유저스페이스 NVMe 드라이버 |
| 왜 deprecated인가? | DIS 의존성 + GPU-initiated가 BaM의 핵심이므로 |
| 쓸 수 있나? | DIS SmartIO 하드웨어 없으면 빌드/실행 불가 |

BaM 프로젝트가 원래 DIS SmartIO 기반 원격 NVMe 접근 연구에서 출발했고, 이 fio 플러그인은 그 초기 단계의 산물이야. 이후 GPU-initiated I/O가 핵심이 되면서 deprecated로 이동됨.
