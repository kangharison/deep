# PPT 자료: NVMe 드라이버 vs BaM — 초기화 Swimlane 다이어그램

발표에서 "NVMe PCIe 드라이버 초기화"와 "BaM 초기화"를 한 장씩 swimlane 다이어그램으로 표현하기 위한 자료.
두 장을 연속으로 보여주면 "주체가 CPU 커널 → GPU로 이동했다"가 시각적으로 강조된다.

---

## 1. Swimlane 다이어그램이란

수영장 레인(lane)처럼 가로/세로 줄을 그어, 각 레인에 **"누가/어디서"** 일이 벌어지는지 분류해서 보여주는 기법. UML 활동 다이어그램·BPMN에서 자주 쓴다.

- **가로축 = 시간(단계)**
- **세로축 = 주체(공간/레이어)**
- **박스 = "언제(컬럼) × 어디에(레인)" 생성되는 객체**
- **세로 화살표 = 레인 간 상호작용** (공간 경계를 넘는 통신)

---

## 2. NVMe PCIe 드라이버 초기화 Swimlane

**실제 Linux kernel upstream 소스 (`drivers/nvme/host/pci.c` 및 `core.c`) 기반.**
검증 기준: `pci.c:3712` `nvme_probe()`의 호출 순서를 한 줄씩 따라감.

레인 구성: User / Block Layer / NVMe Driver / Host DRAM / SSD BAR0 / SSD Controller FW / NAND

```
 시간 ─────────────────────────────────────────────────────────────────────────────────────────▶
         │①      │②alloc_  │③dev_   │④pci_enable          │⑤admin_ │⑥ctrl_  │⑦setup_ │⑧io_   │완성    │
         │insmod │ dev     │ map    │ +configure_admin_Q  │ tagset │ finish │ io_Qs  │ tagset│(idle)  │
         │       │(3717)   │(3725)  │(3735) → (3229)      │(3739)  │(3755)  │(3772)  │(3777) │        │
         │       │         │        │                     │        │Identify│        │+ scan │        │
 ════════╪═══════╪═════════╪════════╪═════════════════════╪════════╪════════╪════════╪═══════╪════════╡
         │       │         │        │                     │        │        │        │       │        │
 User    │       │         │        │                     │        │        │        │┌─────┐│  ▲     │
 space   │       │         │        │                     │        │        │        ││/dev/││  │     │
         │       │         │        │                     │        │        │        ││nvme ││ read() │
         │       │         │        │                     │        │        │        ││0n1  ││        │
         │       │         │        │                     │        │        │        │└──▲──┘│        │
 ────────┼───────┼─────────┼────────┼─────────────────────┼────────┼────────┼────────┼───┼───┼────────┤
         │       │         │        │                     │        │        │        │   │   │        │
 VFS /   │       │         │        │                     │┌──────┐│        │        │┌──┴──┐│        │
 Block   │       │         │        │                     ││admin ││        │        ││IO   ││bio →   │
 Layer   │       │         │        │                     ││blk-mq││        │        ││blk-mq│req    │
 (blk-mq)│       │         │        │                     ││tag_  ││        │        ││tag_ ││        │
         │       │         │        │                     ││set   ││        │        ││set  ││        │
         │       │         │        │                     ││hctx  ││        │        ││hctx[]│        │
         │       │         │        │                     ││(admin)│        │        ││tags[]│        │
         │       │         │        │                     │└──┬───┘│        │        │└─┬───┘│        │
         │       │         │        │                     │   │    │        │        │  │    │        │
         │       │         │        │                     │   │ .queue_rq = nvme_queue_rq    │        │
 ────────┼───────┼─────────┼────────┼─────────────────────┼───┼────┼────────┼────────┼──┼────┼────────┤
         │       │         │        │                     │   │    │        │        │  │    │        │
 NVMe    │┌─────┐│┌───────┐│┌──────┐│┌──────┬────────────┐│   │    │┌──────┐│┌──────┐│  │    │        │
 PCIe    ││pci_ │││nvme_  │││nvme_ │││pci_  │configure_  ││   │    ││Ident.│││alloc_│└──▶      queue_ │
 driver  ││drvr │││dev    │││dev_  │││enable│admin_Q     ││   │    ││Ctrl  │││queue │          rq()   │
 (kernel)││등록 │││kzalloc│││map   │││_device│@3229      ││   │    ││+SET_ │││(i)×N │                 │
         │└─────┘││+dev-> │││bar=  │││+CAP   │nvme_alloc_││   │    ││FEAT  │││+     │                 │
         │       ││queues[│││ioremap│+IRQ   │queue(0):  ││   │    ││NUM_QS│││CREATE│                 │
         │       ││] kcall│││(3160 ││vec 1  │sq_cmds/   ││   │    │└──┬───┘│││IO_CQ │                 │
         │       ││oc(빈) │││-ish) ││       │cqes 할당  ││   │    │   │    ││  /SQ │                 │
         │       ││(3647- │││      ││dev-> │(dev->queues││   │    │   │    │└──┬───┘│                 │
         │       ││ 3658) │││      ││dbs=  │[0] 채움)   ││   │    │   │    │   │    │                 │
         │       ││+INIT_W│││      ││bar+  │+ASQ/ACQ reg│   │    │   │    │   │    │                 │
         │       ││ORK    │││      ││4096  │write       ││   │    │   │    │   │    │                 │
         │       ││(reset │││      ││       │+CC.EN=1   ││   │    │   │    │   │    │                 │
         │       ││_work) │││      ││       │→CSTS.RDY  ││   │    │   │    │   │    │                 │
         │       │└───┬───┘│└──┬───┘│└──────┴────────────┘│   │    │   │    │   │    │                 │
         │       │    │ 슬롯[0] 채움        │              │   │    │   │    │ 슬롯[1..N] 채움         │
         │       │    └────────┼────────────┘              │   │    │   │    │   │    │                 │
 ════════╪═══════╪════════════╪═══════════════════════════╪═══╪════╪═══╪════╪═══╪════╪════════════════╡
         │       │            │          ▼                │   │    │   │    │   ▼    │                 │
 Host    │       │            │        ┌──────┐           │   │    │   │    │ ┌──────┐                │
 DRAM    │       │            │        │admin │           │   │    │   │    │ │IO    │                │
 (DMA-   │       │            │        │sq_cm │           │   │    │   │    │ │sq_cm │  PRP buf      │
  coh,   │       │            │        │ds[]  │           │   │    │   │    │ │ds[]  │  적재         │
  rings) │       │            │        │cqes[]│           │   │    │   │    │ │cqes[]│                │
         │       │            │        │(dev->│           │   │    │   │    │ │1..N+ │                │
         │       │            │        │ q[0])│           │   │    │   │    │ │PRP   │                │
         │       │            │        └──┬───┘           │   │    │   │    │ └──┬───┘                │
 ════════╪═══════╪════════════╪═══════════╪═══════════════╪═══╪════╪═══╪════╪════╪════════════════════╡◀PCIe
         │       │            │    ▼       ▼              │   │    │   │    │    ▼                    │
 SSD     │       │            │┌─────┐  ┌──────┐          │   │    │   │    │ ┌──────┐                │
 BAR0    │       │            ││BAR0 │  │CAP/  │          │   │    │   │    │ │IO SQ/│ doorbell       │
 (MMIO)  │       │            ││공간 │  │CC/   │          │   │    │   │    │ │CQ    │ write          │
         │       │            ││매핑 │  │CSTS/ │          │   │    │   │    │ │Door- │ from driver    │
         │       │            ││     │  │ASQ/  │          │   │    │   │    │ │bells │                │
         │       │            ││     │  │ACQ/  │          │   │    │   │    │ │[1..N]│                │
         │       │            ││     │  │AQ DB │          │   │    │   │    │ │      │                │
         │       │            │└─────┘  └──┬───┘          │   │    │   │    │ └──┬───┘                │
 ────────┼───────┼────────────┼────────────┼──────────────┼───┼────┼───┼────┼────┼────────────────────┤
         │       │            │            ▼              │   │    │   │    │    ▼                    │
 SSD     │       │            │        ┌──────┐           │   │    │┌──▼───┐│ ┌──────┐                │
 Ctrl    │       │            │        │Admin │           │   │    ││Ident.││ │I/O Q │ SQE fetch     │
 FW      │       │            │        │ Q    │           │   │    ││명령  ││ │arbit.│ →FTL L2P      │
 (on-SSD)│       │            │        │proc. │           │   │    ││처리  ││ │+ DMA │ →채널 스케줄  │
         │       │            │        │(CSTS │           │   │    ││(→CQE)│ │engine│                │
         │       │            │        │ .RDY)│           │   │    │└──┬───┘│ └──┬───┘                │
         │       │            │        └──┬───┘           │   │    │   │    │    ▼                    │
 ────────┼───────┼────────────┼────────────┼──────────────┼───┼────┼───┼────┼────┼────────────────────┤
         │       │            │            │              │   │    │   │    │  ┌──────┐               │
 NAND    │       │            │            │              │   │    │   │    │  │Chan  │               │
 Media   │       │            │            │              │   │    │   │    │  │0..K  │               │
         │       │            │            │              │   │    │   │    │  │read/ │               │
         │       │            │            │              │   │    │   │    │  │prog  │               │
         │       │            │            │              │   │    │   │    │  └──────┘               │
 ════════╧═══════╧════════════╧════════════╧══════════════╧═══╧════╧═══╧════╧══════════════════════╡

  ▣ 박스 = 그 단계에 그 레인에서 새로 만들어지는 객체
  (숫자) = pci.c 라인 번호 (upstream)
  ──▶ 레인 내부: 단계 진행 / 동일 객체의 후속 채움
  세로 화살표(↓): 공간 간 상호작용 (Driver→DRAM ring, DRAM ring 주소 → BAR0 register, …)
  ════ 굵은 가로 점선: PCIe 경계 (Host ↔ SSD)
```

### 단계별 함수/객체 요약 (실제 코드 기준)

| Phase | 위치 | 함수 | 하는 일 |
|---|---|---|---|
| ① insmod | — | `nvme_init` → `pci_register_driver` | `pci_driver` 등록 |
| ② alloc_dev | pci.c:3717→3638 | `nvme_pci_alloc_dev` | `kzalloc(struct nvme_dev)`, `INIT_WORK(reset_work)`, **`dev->queues = kcalloc_node(nr_allocated_queues=max_io_queues+1, sizeof(struct nvme_queue))` @3657** (빈 컨테이너), `nvme_init_ctrl` (ctrl framework 등록), DMA mask 설정 |
| ③ dev_map | pci.c:3725 | `nvme_dev_map` | **BAR0 ioremap** — `dev->bar`에 가상주소 설정 |
| ④ pci_enable + admin Q | pci.c:3735→3158 | `nvme_pci_enable` | `pci_enable_device_mem`, `pci_set_master`, `pci_alloc_irq_vectors(1,1)` (admin vec만), CAP 레지스터 읽기, `dev->dbs = bar+4096` (doorbell base), **`nvme_pci_configure_admin_queue` @3229**: `nvme_alloc_queue(0,depth)`로 `dev->queues[0]`의 `sq_cmds`/`cqes` `dma_alloc_coherent` 할당 + ASQ/ACQ base register에 DMA 주소 write + **`CC.EN=1`→CSTS.RDY 대기** |
| ⑤ admin tag_set | pci.c:3739 | `nvme_alloc_admin_tag_set` | **admin용 blk-mq tag_set** 할당 — admin hctx/tags/request_queue 설정 (Identify를 보낼 경로) |
| ⑥ ctrl_finish | pci.c:3755→core.c | `nvme_init_ctrl_finish` | **Admin queue로 Identify Controller 발행** + Identify NS + `SET_FEATURES(NUM_QUEUES)` + APST, timestamp 설정 |
| ⑦ setup_io_queues | pci.c:3772 | `nvme_setup_io_queues` | `pci_alloc_irq_vectors`로 N+1 벡터 확장, `nvme_create_io_queues` → 각 qid에 대해 `nvme_alloc_queue(i)` (pci.c:2103, DMA ring alloc) + `nvme_create_queue` (admin 명령 `CREATE_IO_CQ` → `CREATE_IO_SQ`로 SSD에 등록) — **`dev->queues[1..N]` 채움** |
| ⑧ io_tagset + scan | pci.c:3777, 3794 | `nvme_alloc_io_tag_set` + `nvme_start_ctrl` | **IO용 blk-mq tag_set** (IO Q가 존재한 뒤에야 `nr_hw_queues`를 알 수 있음), `nvme_queue_scan` → `nvme_scan_work` 큐잉 → `nvme_alloc_ns` (비동기지만 probe 끝에서 `flush_work`로 대기) → `/dev/nvme0n1` 등록 |

### 이 순서의 핵심 포인트 (당초 다이어그램과 달라진 부분)

1. **`BAR0 ioremap`은 `nvme_pci_alloc_dev` 밖** (`nvme_dev_map @3725`, 별도 단계). alloc_dev는 순수 커널 메모리 작업만.
2. **`dev->queues[]` kcalloc은 alloc_dev 안**(`@3657`). 이 시점엔 BAR 매핑 전이라서 doorbell 주소 세팅은 불가능 — 슬롯은 진짜로 **빈 컨테이너**.
3. **Admin queue ring DMA alloc은 `nvme_pci_configure_admin_queue` 안**(`@3229`), 즉 `nvme_pci_enable` 호출 내부. Controller reset/enable(`CC.EN=1`)과 **같은 함수** 안에서 묶여 있음 — 논리적으로 "Admin Q가 살아야 reset 완료"이기 때문.
4. **Admin blk-mq tag_set이 Identify보다 먼저**(`@3739 < @3755`). Identify는 admin queue + admin tag_set을 통해 발행되므로.
5. **IO blk-mq tag_set은 IO queue 생성 뒤**(`@3777 > @3772`). `nvme_alloc_io_tag_set`은 `nr_hw_queues = dev->online_queues - 1`이 결정된 뒤에야 호출 가능.
6. **Namespace scan은 비동기 워커** — `nvme_start_ctrl @3794`에서 `scan_work` 큐잉, `flush_work @3796`으로 probe 종료 직전 대기. 유저 입장에선 "probe 완료 = /dev/nvme0n1 존재"로 보임.

### `dev->queues[]`의 두-단계 채움 (upstream 기준)

```
② alloc_dev (kernel 일반 메모리, kcalloc_node)
─────────────────────────────────────────────
struct nvme_queue dev->queues[nr_allocated_queues]
┌──────────┬──────────┬──────────┬─────┐
│ slot[0]  │ slot[1]  │ slot[2]  │ ... │   모든 슬롯 zero-filled
│ (Admin)  │ (IO 1)   │ (IO 2)   │     │   sq_cmds=NULL, cqes=NULL,
│  비어있음│  비어있음│  비어있음│     │   q_db=NULL, qid=0
└──────────┴──────────┴──────────┴─────┘

④ pci_enable → configure_admin_queue → nvme_alloc_queue(dev, 0, depth)
────────────────────────────────────────────────────────────────
slot[0] 채움:
   ▸ sq_cmds  = dma_alloc_coherent(...)           — admin SQ ring
   ▸ cqes     = dma_alloc_coherent(...)           — admin CQ ring
   ▸ q_db     = dev->dbs + (0 * 2 * stride)       — admin SQ doorbell
   ▸ qid      = 0, q_depth = NVME_AQ_DEPTH

   + ASQ/ACQ base register에 DMA 주소 MMIO write
   + CC.EN = 1 → CSTS.RDY 폴링

⑦ setup_io_queues → nvme_alloc_queue(dev, i, depth) × N
────────────────────────────────────────────────────────
slot[1..N] 채움 (동일 패턴):
   ▸ sq_cmds  = dma_alloc_coherent(...)
   ▸ cqes     = dma_alloc_coherent(...)
   ▸ q_db     = dev->dbs + (i * 2 * stride)
   ▸ qid      = i

   + admin 명령 CREATE_IO_CQ(qid=i) → CREATE_IO_SQ(qid=i)로 SSD에 등록
```

→ ② 단계는 **커널 메모리(kcalloc, DMA 아님)** 에 컨테이너만 깔아두고, ④⑦ 단계에서 **각 슬롯의 ring buffer를 DMA-coherent 영역**(`dma_alloc_coherent`)에 별도 할당.

---

## 3. BaM 초기화 Swimlane

레인 구성: User / Kernel(libnvm 얇은 헬퍼) / Host DRAM(Admin 만) / **GPU VRAM(신설)** / SSD BAR0 / SSD Controller FW / NAND

```
 시간 ─────────────────────────────────────────────────────────────────────────────▶
        │①insmod  │②PCI     │③Ctrl    │④Admin Q │⑤I/O Q   │⑥page_   │⑦range_  │완성   │
        │libnvm.ko│probe    │객체생성 │ setup   │ setup    │ cache   │ array   │(GPU   │
        │         │+chrdev  │(user)   │(user)   │ (VRAM)   │ (VRAM)  │         │kernel)│
 ═══════╪═════════╪═════════╪═════════╪═════════╪══════════╪═════════╪═════════╪═══════╡
        │         │         │         │         │          │         │         │       │
 User   │         │         │┌───────┐│┌───────┐│┌────────┐│┌───────┐│┌───────┐│ app   │
 space  │         │         ││open() │││nvm_aq_│││Identify│││page_  │││range_t│││ calls │
 (host  │         │         ││mmap   │││create │││+       │││cache_ │││ +     │││ CUDA  │
  CPU)  │         │         ││(BAR0) │││nvm_   │││CREATE_│││t      │││array_t│││kernel │
        │         │         ││       │││admin_*│││IO_SQ/ │││(host  │││       │││       │
        │         │         ││       │││       │││  CQ   │││ proxy)│││       │││       │
        │         │         │└──┬────┘│└──┬────┘│└──┬─────┘│└──┬────┘│└──┬────┘│       │
 ───────┼─────────┼─────────┼───┼─────┼───┼─────┼───┼──────┼───┼─────┼───┼─────┼───────┤
        │         │         │   │     │   │     │   │      │   │     │   │     │       │
 Kernel │┌───────┐│┌───────┐│   │     │   │     │┌──▼────┐ │   │     │   │     │       │
 (libnvm││pci_   │││ctrl   ││   │     │   │     ││ioctl  │ │   │     │   │     │       │
  helper││driver │││+      ││   │     │   │     ││MAP    │ │   │     │   │     │       │
  얇음) ││등록   │││/dev/  ││   │     │   │     ││→nvidia│ │   │     │   │     │       │
        ││       │││libnvm0│││   │     │   │     ││_p2p_  │ │   │     │   │     │       │
        ││       │││chrdev │││   │     │   │     ││get_   │ │   │     │   │     │       │
        │└───────┘│└───────┘│   │     │   │     ││pages  │ │   │     │   │     │       │
        │         │         │   │     │   │     │└──┬────┘ │   │     │   │     │       │
        │         │         │   │     │   │     │   │IOVA  │   │     │   │     │       │
 ───────┼─────────┼─────────┼───┼─────┼───┼─────┼───┼──────┼───┼─────┼───┼─────┼───────┤
        │         │         │   │     │   │     │   │      │   │     │   │     │       │
 Host   │         │         │   │     │   ▼     │   │      │   │     │   │     │       │
 DRAM   │         │         │   │     │┌──────┐ │   │      │   │     │   │     │       │
 (admin │         │         │   │     ││ASQ/  │ │   │      │   │     │   │     │       │
  만)   │         │         │   │     ││ACQ   │ │   │      │   │     │   │     │       │
        │         │         │   │     ││ring  │ │   │      │   │     │   │     │       │
        │         │         │   │     ││(작음)│ │   │      │   │     │   │     │       │
        │         │         │   │     │└──┬───┘ │   │      │   │     │   │     │       │
 ═ ═ ═ ═╪═ ═ ═ ═ ═╪═ ═ ═ ═ ═╪═ ═│═ ═ ═╪═ ═│═ ═ ═╪═ ═│═ ═ ═ ╪═ ═│═ ═ ═╪═ ═│═ ═ ═╪═══════╡ ◀ PCIe
        │         │         │   │     │   │     │   ▼      │   ▼     │   ▼     │       │
 GPU    │         │         │   │     │   │     │┌────────┐│┌───────┐│┌───────┐│┌─────┐│
 VRAM   │         │         │   │     │   │     ││QueuePair│││page_  │││range/ │││__glob││
 (CUDA  │         │         │   │     │   │     ││[1..nQ] │││cache_t│││array_ │││al__  ││
 context)         │         │   │     │   │     ││▸SQ ring│││▸pages │││ (GPU  │││커널  ││
        │         │         │   │     │   │     ││▸CQ ring│││▸meta  │││ view) │││bam_  ││
        │         │         │   │     │   │     ││▸PRP    │││▸lock  │││       │││ptr.  ││
        │         │         │   │     │   │     ││ cache  │││       │││       │││read()││
        │         │         │   │     │   │     │└──┬─────┘│└──┬────┘│└──┬────┘│└──┬───┘│
 ═ ═ ═ ═╪═ ═ ═ ═ ═╪═ ═ ═ ═ ═╪═ ═│═ ═ ═╪═ ═│═ ═ ═╪═ ═│═ ═ ═ ╪═ ═│═ ═ ═╪═ ═│═ ═ ═╪═ ═│═══╡ ◀ PCIe
        │         │         │   ▼     │   ▼     │   ▼      │   │     │   │     │   │   │
 SSD    │         │┌───────┐│┌──────┐ │┌──────┐ │┌────────┐│   │     │   │     │   │   │
 BAR0   │         ││BAR0   │││CAP/  │ ││ASQ_  │ ││SQ/CQ   ││   │     │   │     │   │   │
 (MMIO) │         ││공간   │││CC/   │ ││TDBL, │ ││Doorbell│   │     │    │     │ doorbell │
        │         ││노출   │││CSTS  │ ││ACQ_  │ ││[1..nQ] ││   │     │   │     │ write │
        │         ││(GPU가 │││regs  │ ││HDBL  │ ││(GPU P2P│   │     │    │     │ from  │
        │         ││mmap)  │││      │ ││      │ ││ mmap)  ││   │     │   │     │ GPU   │
        │         │└───────┘│└──────┘ │└──┬───┘ │└──┬─────┘│   │     │   │     │   │   │
 ───────┼─────────┼─────────┼─────────┼───┼─────┼───┼──────┼───┼─────┼───┼─────┼───┼───┤
        │         │         │         │   ▼     │   ▼      │   │     │   │     │   ▼   │
 SSD    │         │         │         │┌──────┐ │┌────────┐│   │     │   │     │ SQE   │
 Ctrl   │         │         │         ││Admin │ ││I/O Q   │    │     │   │     │ fetch │
 FW     │         │         │         ││Q     │ ││arbiter │    │     │   │     │ (DMA  │
 (on-   │         │         │         ││proc. │ ││+ DMA   │    │     │   │     │  from │
  SSD)  │         │         │         │└──┬───┘ ││engine  │    │     │   │     │  VRAM)│
        │         │         │         │   │     │└──┬─────┘│   │     │   │     │       │
        │         │         │         │   │     │   ▼      │   │     │   │     │       │
        │         │         │         │   │     │┌────────┐│   │     │   │     │       │
        │         │         │         │   │     ││FTL L2P │    │     │   │     │       │
        │         │         │         │   │     │└──┬─────┘│   │     │   │     │       │
 ───────┼─────────┼─────────┼─────────┼───┼─────┼───┼──────┼───┼─────┼───┼─────┼───┼───┤
 NAND   │         │         │         │   │     │   ▼      │   │     │   │     │ page  │
 Media  │         │         │         │   │     │ Channels │   │     │   │     │ read  │
        │         │         │         │   │     │ (read/   │   │     │   │     │ →VRAM │
        │         │         │         │   │     │  program)│   │     │   │     │       │
 ═══════╧═════════╧═════════╧═════════╧═══╧═════╧═══╧══════╧═══╧═════╧═══╧═════╧═══╧═══╡

 ══ 두꺼운 점선: PCIe 경계 (위쪽=Host↔GPU, 아래쪽=GPU↔SSD)
 ──▶ 레인 내부: 단계 진행 / 소유
 세로 화살표: 공간 간 상호작용 (ioctl / MMIO / P2P DMA)
```

### 단계별 함수/객체 요약

| Phase | 주체 | 함수/객체 | 생성 위치 |
|---|---|---|---|
| ① insmod | Kernel | `libnvm_helper_entry` → `pci_register_driver` | Kernel |
| ② PCI probe | Kernel | `add_pci_dev` → `ctrl_chrdev_create` → `/dev/libnvm0` | Kernel (얇음) |
| ③ Controller 객체 | User | `Controller::Controller` → `open`/`mmap(BAR0)` → `nvm_ctrl_init` | User |
| ④ Admin Q | User | `nvm_aq_create` → Admin SQ/CQ ring | Host DRAM |
| ⑤ I/O Q | User + GPU | `QueuePair::QueuePair` → `cudaMalloc` + `ioctl(MAP)` → `nvidia_p2p_get_pages` → `CREATE_IO_SQ/CQ` | **GPU VRAM** (SQ/CQ) |
| ⑥ page_cache | GPU | `page_cache_t(page_size, n_pages, ...)` | GPU VRAM |
| ⑦ range_t/array_t | GPU | `range_t<T>(...)` → `array_t<T>(...)` | GPU VRAM |
| 완성 | GPU | `__global__` 커널에서 `bam_ptr.read()` | GPU kernel |

---

## 4. Kernel NVMe vs BaM — 레인 비교 (발표 한 장에 같이 두면 좋음)

| 측면 | Kernel NVMe Driver | BaM |
|---|---|---|
| **커널 레인의 부피** | ④~⑦ 전부 커널 — 드라이버가 주인공 | ①②만 커널, 이후 전부 **유저+GPU** — 커널은 BAR0 노출하는 "문지기"만 |
| **Block Layer 레인** | 존재 (blk-mq tag_set/hctx/request_queue) | **없음** — 파일시스템·blk-mq 완전 우회 |
| **Host DRAM 레인** | SQ/CQ 전부 + PRP pool | **Admin Q만** (작음) |
| **GPU VRAM 레인** | 레인 자체가 존재 안 함 | **신설 레인** — QueuePair·page_cache·array_t 모두 여기 |
| **Doorbell 접근 주체** | 커널이 `writel()` | **GPU 커널이 직접** ST 인스트럭션으로 write |
| **IRQ 레인** | MSI-X per CPU | **없음** — CQ polling으로 대체 |
| **PCIe 경계 교차** | 1회 (Host ↔ SSD) | 2회 (Host ↔ GPU, GPU ↔ SSD) |

### 시각적으로 드러나야 할 "BaM의 본질"

1. **커널 레인이 얇아지고 GPU VRAM 레인이 두꺼워짐** → 실행 주체가 CPU 커널 → GPU로 이동.
2. **PCIe 경계(굵은 점선)를 두 번 교차** → GPU가 호스트 DRAM을 거치지 않고 SSD와 직접 말하는 P2P 경로.
3. **⑤ 단계의 세로 화살표 집중** → `cudaMalloc(SQ/CQ)` (GPU VRAM) → `ioctl(MAP)` (Kernel) → `nvidia_p2p_get_pages` → IOVA → SSD에 `CREATE_IO_SQ`로 등록. BaM 초기화의 하이라이트.
4. **완성 컬럼의 경로**: 앱 → `__global__` 커널 → `bam_ptr.read()` → GPU VRAM SQ ring에 SQE write → BAR0 doorbell MMIO → SSD FW가 VRAM에서 직접 DMA fetch → NAND → VRAM 복귀. **User/Block/Driver 3개 레인을 통째로 건너뜀**.

---

## 5. PPT 제작 팁

### 레이아웃
- **두 슬라이드로 나란히**: 앞=Kernel NVMe swimlane, 뒤=BaM swimlane. 레인 이름과 단계 번호를 가능한 한 같은 세로 위치에 두면 애니메이션으로 "전환되는 레인"이 강조됨.
- **세 번째 슬라이드**: §4의 비교 표를 좌우 2-컬럼 + 차이 행 강조.

### 색 규칙
- Host(User/Block/Driver/DRAM) = **파랑 계열**
- GPU VRAM = **초록 계열**
- SSD(BAR0/FW/NAND) = **주황/빨강 계열**
- 비어있는 레인 = 연한 회색 배경 (BaM에서 Block Layer 레인을 비워두면 "BaM은 이 전체를 건너뛴다"가 시각적으로 강조됨)

### 애니메이션 포인트
- ⑤ 단계의 "VRAM SQ 생성 → ioctl → BAR0 등록" 세로 화살표를 순서대로 페이드인 → 발표 핵심을 시각적으로 강조.
- 완성 컬럼의 read() 경로를 마지막에 점선으로 덧그려 "idle 상태의 최종 시스템"과 "실제 I/O 경로"를 분리해 보여줌.

### 선 스타일
- 실선 = 포인터/소유 관계
- 점선 = DMA 데이터 경로
- 굵은 선 = MMIO doorbell write
- **굵은 가로 점선 = PCIe 경계** (BaM에서는 2개, NVMe에서는 1개)

---

## 6. 참고 (소스 파일 매핑)

### Kernel NVMe (upstream, `/home/harison/company/linux-study/`)
- `drivers/nvme/host/pci.c`
  - `nvme_probe` @3712 — 진입점, 아래 함수들을 순차 호출
  - `nvme_pci_alloc_dev` @3638 — `kzalloc(nvme_dev)` + `dev->queues[] kcalloc @3657`
  - `nvme_dev_map` — BAR0 ioremap
  - `nvme_pci_enable` @3158 — `pci_enable_device`, CAP, IRQ, `configure_admin_queue` @3229
  - `nvme_pci_configure_admin_queue` @2300 — admin queue ring DMA alloc + ASQ/ACQ reg + CC.EN=1
  - `nvme_alloc_queue` @2103 — 특정 qid에 대해 SQ/CQ ring `dma_alloc_coherent`
  - `nvme_setup_io_queues` — MSI-X 확장, `nvme_create_io_queues` 루프
- `drivers/nvme/host/core.c`
  - `nvme_init_ctrl_finish` — Identify Controller/NS, SET_FEATURES
  - `nvme_alloc_admin_tag_set` / `nvme_alloc_io_tag_set` — blk-mq tag_set 래퍼
  - `nvme_scan_work` / `nvme_alloc_ns` — 비동기 namespace 등록
- `block/blk-mq.c` — `blk_mq_alloc_tag_set`, `blk_mq_init_queue`

### BaM
- `bam/module/pci.c` — `libnvm_helper_entry`, `add_pci_dev`, `map_ioctl`, `mmap_registers`
- `bam/include/nvm_ctrl.h` / `bam/src/ctrl.c` — `nvm_ctrl_init`, `nvm_aq_create`
- `bam/include/ctrl.h` — `Controller::Controller`
- `bam/include/queue.h` — `QueuePair::QueuePair`
- `bam/include/page_cache.h` — `page_cache_t`, `range_t`, `array_t`, `bam_ptr`
