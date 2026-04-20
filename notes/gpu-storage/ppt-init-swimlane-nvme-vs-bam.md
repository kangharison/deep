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

## 2. NVMe PCIe 드라이버 초기화 Swimlane (발표용 — 간소화)

**실제 Linux kernel upstream 소스 (`drivers/nvme/host/pci.c` 및 `core.c`) 기반.**

팀장님께 한눈에 보여드리기 위한 **4-phase 간소 버전**. 세부 함수·라인 번호는 §3의 flow 다이어그램에서.

레인 구성: User / Block Layer / NVMe Driver / Host DRAM / SSD BAR0 / SSD Controller FW / NAND

### 4개 Phase의 의미 (한 줄 요약)
- **A. Probe**: 드라이버 객체 할당 + BAR0 매핑까지. 아직 SSD와 대화 못 함.
- **B. Ctrl Live**: Admin Queue 가동 + Identify 완료. SSD가 자기 정보 알려줌.
- **C. I/O Ready**: I/O Queue N개 생성 + blk-mq 연결. I/O 경로 개통.
- **D. NS Ready**: Namespace scan → `/dev/nvmeXn1` 등록. 앱이 read/write 가능.

```
 시간 ─────────────────────────────────────────────────────────────────▶
         │  A. Probe      │  B. Ctrl Live    │  C. I/O Ready   │  D. NS Ready   │
         │  (struct +     │  (admin Q up +   │  (IO Qs +       │  (/dev/        │
         │   BAR0 매핑)   │   Identify done) │   blk-mq 연결)  │   nvmeXn1)     │
 ════════╪════════════════╪══════════════════╪═════════════════╪════════════════╡
         │                │                  │                 │                │
 User    │                │                  │                 │ ┌────────┐     │
 space   │                │                  │                 │ │/dev/   │     │
         │                │                  │                 │ │nvmeXn1 │ ─▶ read()
         │                │                  │                 │ └────────┘     │
 ────────┼────────────────┼──────────────────┼─────────────────┼────────────────┤
         │                │                  │                 │                │
 VFS /   │                │ ┌──────────┐     │ ┌──────────┐    │ ┌──────────┐   │
 Block   │                │ │admin     │     │ │IO blk-mq │    │ │gendisk   │   │
 Layer   │                │ │blk-mq    │     │ │tag_set   │    │ │request_q │   │
 (blk-mq)│                │ │tag_set   │     │ │hctx[]    │    │ │          │   │
         │                │ │(+req_q)  │     │ │tags[]    │    │ │          │   │
         │                │ └──────────┘     │ └──────────┘    │ └──────────┘   │
 ────────┼────────────────┼──────────────────┼─────────────────┼────────────────┤
         │                │                  │                 │                │
 NVMe    │ ┌──────────┐   │ ┌──────────┐     │ ┌──────────┐    │                │
 PCIe    │ │nvme_dev  │   │ │CC.EN=1   │     │ │IO Queues │    │ (.queue_rq =   │
 driver  │ │+ queues[]│   │ │admin Q   │     │ │created   │    │  nvme_queue_rq)│
 (kernel)│ │(빈 슬롯) │   │ │ready     │     │ │(×N)      │    │                │
         │ │+ ctrl    │   │ │Identify  │     │ │          │    │                │
         │ │ framework│   │ │done      │     │ │          │    │                │
         │ └──────────┘   │ └──────────┘     │ └──────────┘    │                │
 ────────┼────────────────┼──────────────────┼─────────────────┼────────────────┤
         │                │                  │                 │                │
 Host    │                │ ┌──────────┐     │ ┌──────────┐    │                │
 DRAM    │                │ │admin     │     │ │IO SQ/CQ  │    │                │
 (DMA-   │                │ │SQ/CQ ring│     │ │ring × N  │    │                │
  coh)   │                │ │          │     │ │+ PRP pool│    │                │
         │                │ └──────────┘     │ └──────────┘    │                │
 ════════╪════════════════╪══════════════════╪═════════════════╪════════════════╡ ◀ PCIe
         │                │                  │                 │                │
 SSD     │ ┌──────────┐   │ ┌──────────┐     │ ┌──────────┐    │                │
 BAR0    │ │BAR0 매핑 │   │ │CAP/CC/   │     │ │IO SQ/CQ  │    │                │
 (MMIO)  │ │됨 (bar + │   │ │CSTS/ASQ/ │     │ │Doorbells │    │                │
         │ │dbs base) │   │ │ACQ regs +│     │ │[1..N]    │    │                │
         │ │          │   │ │admin DB  │     │ │          │    │                │
         │ └──────────┘   │ └──────────┘     │ └──────────┘    │                │
 ────────┼────────────────┼──────────────────┼─────────────────┼────────────────┤
         │                │                  │                 │                │
 SSD     │                │ ┌──────────┐     │ ┌──────────┐    │                │
 Ctrl FW │                │ │admin Q   │     │ │IO Q      │    │                │
 (on-SSD)│                │ │processor │     │ │arbiter + │    │                │
         │                │ │(CSTS.RDY)│     │ │DMA engine│    │                │
         │                │ │+ Identify│     │ │+ FTL L2P │    │                │
         │                │ │ 응답     │     │ │          │    │                │
         │                │ └──────────┘     │ └──────────┘    │                │
 ────────┼────────────────┼──────────────────┼─────────────────┼────────────────┤
         │                │                  │                 │                │
 NAND    │                │                  │                 │ (idle, ready   │
 Media   │                │                  │                 │  for read/     │
         │                │                  │                 │  program)      │
 ════════╧════════════════╧══════════════════╧═════════════════╧════════════════╡

  ▣ 박스 = 그 Phase에서 그 레인에 "추가되는" 리소스 (이전 Phase 박스는 유지됨)
  ════ 굵은 가로 점선 = PCIe 경계 (Host ↔ SSD)
```

### Phase 전환 시 일어나는 "결정적 사건" (발표 포인트)

- **A → B**: `CC.EN=1` MMIO write → SSD FW가 부팅해서 CSTS.RDY를 세움. 이때 **Admin Queue가 살아야 Identify를 발행 가능**.
- **B → C**: Identify 결과의 `NN`(namespace 개수), `CAP.MQES`(queue depth)를 근거로 **IO Queue를 몇 개 만들지 결정**. 이전 Phase의 정보 없이는 C를 시작 못 함.
- **C → D**: IO Queue가 살아야 → IO blk-mq tag_set에 `nr_hw_queues` 부여 가능 → 그제야 gendisk를 만들어 /dev에 노출.

→ 네 phase는 **독립된 단계가 아니라 직렬 의존성**. 그래서 `nvme_probe()`가 동기적으로 한 함수에서 다 호출.

---

## 3. NVMe 초기화 상세 호출 Flow (참고용)

위 swimlane의 각 phase가 실제 소스에서 어떤 함수 호출 트리를 거치는지. 백업 슬라이드나 구두 설명용.

```
nvme_probe(pdev, id)                                       pci.c:3712
 │
 ├─[Phase A: Probe]────────────────────────────────────────────────
 │   │
 │   ├─ nvme_pci_alloc_dev                                 @3717 → @3638
 │   │   ├─ kzalloc(struct nvme_dev)                       @3647
 │   │   ├─ INIT_WORK(&dev->ctrl.reset_work, nvme_reset_work) @3651
 │   │   ├─ dev->nr_allocated_queues = max_io_queues + 1   @3656
 │   │   ├─ dev->queues = kcalloc_node(N+1, nvme_queue)    @3657  ◀ 빈 컨테이너
 │   │   ├─ nvme_init_ctrl (ctrl framework 등록)           @3681
 │   │   └─ DMA mask 설정                                  @3686-3691
 │   ├─ nvme_add_ctrl                                      @3721
 │   ├─ nvme_dev_map                                       @3725  ◀ BAR0 ioremap
 │   └─ nvme_pci_alloc_iod_mempool                         @3729
 │
 ├─[Phase B: Ctrl Live]────────────────────────────────────────────
 │   │
 │   ├─ nvme_pci_enable                                    @3735 → @3158
 │   │   ├─ pci_enable_device_mem(pdev)                    @3164
 │   │   ├─ pci_set_master(pdev)                           @3167
 │   │   ├─ pci_alloc_irq_vectors(1, 1)    ← admin vec만   @3182
 │   │   ├─ dev->ctrl.cap = readq(CAP)                     @3186
 │   │   ├─ dev->dbs = dev->bar + 4096     ← doorbell base @3191
 │   │   └─ nvme_pci_configure_admin_queue                 @3229 → @2300
 │   │       ├─ nvme_alloc_queue(dev, 0, NVME_AQ_DEPTH)    → @2103
 │   │       │   ├─ dma_alloc_coherent → sq_cmds[]
 │   │       │   ├─ dma_alloc_coherent → cqes[]
 │   │       │   └─ dev->queues[0] 채움 (qid=0, q_db=dbs)
 │   │       ├─ ASQ/ACQ base 레지스터에 DMA 주소 MMIO write
 │   │       └─ CC.EN = 1 → CSTS.RDY 폴링
 │   ├─ nvme_alloc_admin_tag_set                           @3739  ◀ admin blk-mq
 │   │   └─ blk_mq_alloc_tag_set + request_queue(admin)
 │   └─ nvme_init_ctrl_finish                              @3755 (core.c)
 │       ├─ Identify Controller (admin 명령, admin Q로 발행)
 │       ├─ Identify Namespace list
 │       └─ SET_FEATURES(NUM_QUEUES) — IO Q 개수 협상
 │
 ├─[Phase C: I/O Ready]────────────────────────────────────────────
 │   │
 │   ├─ nvme_setup_io_queues                               @3772
 │   │   ├─ pci_alloc_irq_vectors(N+1)  ← IO vec 추가
 │   │   └─ nvme_create_io_queues                          → @3019
 │   │       └─ for qid in 1..N:
 │   │           ├─ nvme_alloc_queue(dev, qid, q_depth)    @2103
 │   │           │   └─ dma_alloc_coherent → sq_cmds/cqes
 │   │           │       dev->queues[qid] 채움
 │   │           └─ nvme_create_queue(qid)
 │   │               ├─ CREATE_IO_CQ admin 명령
 │   │               └─ CREATE_IO_SQ admin 명령
 │   └─ nvme_alloc_io_tag_set                              @3777  ◀ IO blk-mq
 │       └─ blk_mq_alloc_tag_set(nr_hw_queues = online_queues-1)
 │
 └─[Phase D: NS Ready]─────────────────────────────────────────────
     │
     ├─ nvme_change_ctrl_state(NVME_CTRL_LIVE)             @3785
     ├─ nvme_start_ctrl                                    @3794
     │   └─ nvme_queue_scan → schedule nvme_scan_work (async)
     │       └─ nvme_alloc_ns
     │           ├─ blk_mq_init_queue (per-ns request_queue)
     │           ├─ __alloc_disk_node → gendisk
     │           └─ device_add_disk → /dev/nvmeXn1
     └─ flush_work(&dev->ctrl.scan_work)                   @3796  ◀ async 완료 대기
```

### 핵심: `dev->queues[]` 의 두-단계 채움

```
Phase A (kcalloc, kernel 일반 메모리)
────────────────────────────────────
struct nvme_queue dev->queues[N+1]     ← 슬롯만 할당, 내용은 zero
┌──────┬──────┬──────┬──────┐
│slot[0]│slot[1]│slot[2]│ ... │
│Admin │IO 1  │IO 2  │      │  sq_cmds=NULL, cqes=NULL, q_db=NULL
└──────┴──────┴──────┴──────┘

Phase B: slot[0] 채움 (admin Q ring, dma_alloc_coherent)
Phase C: slot[1..N] 채움 (IO Q rings, dma_alloc_coherent)
```

→ 같은 배열이 **두 번의 DMA 할당 이벤트**로 점진 완성. 이게 NVMe 초기화의 구조적 특징.

---

## 4. BaM 초기화 Swimlane

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

## 5. Kernel NVMe vs BaM — 레인 비교 (발표 한 장에 같이 두면 좋음)

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

## 6. PPT 제작 팁

### 레이아웃 (팀장님 발표용 추천 구성)
- **슬라이드 1 — 메인 swimlane (§2)**: NVMe 4-phase 간소화 버전. 팀장님은 이 한 장만 봐도 "무엇이 어디에 생기는가" 이해 가능. 세부 함수명은 **언급하지 않음**.
- **슬라이드 2 — BaM 4-phase swimlane (§4)**: 같은 구조로 NVMe와 대비.
- **슬라이드 3 — 비교 표 (§5)**: 좌우 2-컬럼, 차이 행 강조.
- **백업 슬라이드 (질문 받을 때만 펼침)**:
  - §3의 NVMe 호출 flow (함수명 + 라인 번호)
  - BaM 쪽 상세 flow
  → 팀장님이 "구체적으로 어느 함수에서?" 물어볼 때만 넘어가서 설명. 평소엔 숨김.

### 발표 스크립트 흐름
1. "NVMe 초기화는 4 단계를 거칩니다" (swimlane 메인만 보여주며)
2. "각 phase에서 이 레인(공간)에 이런 객체가 추가됩니다" (박스 하나씩 짚기)
3. "Phase 간에는 **직렬 의존성**이 있습니다" (A→B→C→D 사이 화살표 강조)
4. "BaM은 같은 구조를 이렇게 변형합니다" (다음 슬라이드 전환)

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

## 7. 참고 (소스 파일 매핑)

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
