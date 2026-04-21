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

## 2. NVMe PCIe 드라이버 초기화 Swimlane (발표용)

**실제 Linux kernel upstream 소스 (`drivers/nvme/host/pci.c` 및 `core.c`) 기반.**

Phase 경계는 실제 `nvme_probe()` 호출 순서에 맞춘 **8단계**. 박스 안에는 "어떤 리소스가 생기는가"만 적고, 함수명·라인번호는 §3 flow 다이어그램에서.

레인 구성: User / Block Layer / NVMe Driver / Host DRAM / SSD BAR0 / SSD Controller FW / NAND

### Phase 한 줄 요약
- **① Module Load** — `nvme.ko` insmod → PCI 드라이버 등록, `nvme_wq` 워크큐 생성
- **② PCI Probe** — `nvme_dev` 구조체 할당, `reset_work` 초기화, `dev->queues[]` 빈 슬롯 확보 (N+1개), ctrl framework 등록, DMA mask 64bit 설정
- **③ BAR0 Map** — NVMe 컨트롤러의 MMIO 영역을 `ioremap`으로 커널 가상주소에 매핑 (`dev->bar`)
- **④ Ctrl Enable** — 아래가 **한 함수** 안에서 순차로 일어남:
  1. `pci_enable_device_mem` (BAR 공간 접근 활성화)
  2. **`pci_set_master` — Bus Master Enable** (SSD가 host DRAM에 DMA 가능)
  3. `pci_alloc_irq_vectors(1,1)` — admin MSI-X 벡터 1개
  4. `CAP` 레지스터 읽기 (max queue size, stride 등)
  5. `dev->dbs = bar + 4096` — doorbell 레지스터 base 포인터 세팅
  6. Admin Q ring DMA 할당 → `dev->queues[0]` 채움 (slot 0)
  7. DMA 주소를 `ASQ/ACQ` base 레지스터에 MMIO write
  8. `CC.EN = 1` → `CSTS.RDY` 폴링 (SSD 기동)
- **⑤ Admin blk-mq** — Admin용 `blk_mq_alloc_tag_set` + admin `request_queue` (Identify를 request로 감싸 발행 가능)
- **⑥ Identify** — Admin Q로 `Identify Controller`(NN, OACS, OCS, MDTS) + `Identify NS list` + `SET_FEATURES(NUM_QUEUES)` 협상
- **⑦ I/O Q Setup** — `pci_alloc_irq_vectors`를 N+1개로 확장, 각 `qid`에 대해 I/O SQ/CQ DMA ring (`dev->queues[1..N]` 채움) + admin 명령 `CREATE_IO_CQ` → `CREATE_IO_SQ` 쌍 발행
- **⑧ blk-mq + NS Scan** — IO용 `blk_mq_alloc_tag_set` (`nr_hw_queues = N`, `hctx[]` per CPU, `tags[]`), `.queue_rq = nvme_queue_rq` 바인딩, `scan_work` 비동기 워커 → `nvme_alloc_ns` → `gendisk` + per-NS `request_queue` → `/dev/nvmeXn1` 등록

```
 시간 ──────────────────────────────────────────────────────────────────────────────────────────────────▶
         │①Module  │②PCI      │③BAR0   │④Ctrl Enable         │⑤Admin   │⑥Identify │⑦I/O Q       │⑧blk-mq  │
         │ Load    │ Probe    │ Map    │                     │ blk-mq  │          │ Setup       │+NS scan │
 ════════╪═════════╪══════════╪════════╪═════════════════════╪═════════╪══════════╪═════════════╪═════════╡
 User    │         │          │        │                     │         │          │             │┌───────┐│
 space   │         │          │        │                     │         │          │             ││/dev/  ││
         │         │          │        │                     │         │          │             ││nvmeXn1│─▶ read()
         │         │          │        │                     │         │          │             │└───────┘│
 ────────┼─────────┼──────────┼────────┼─────────────────────┼─────────┼──────────┼─────────────┼─────────┤
 VFS /   │         │          │        │                     │┌───────┐│          │             │┌───────┐│
 Block   │         │          │        │                     ││admin  ││          │             ││IO     ││
 Layer   │         │          │        │                     ││tag_set││          │             ││tag_set││
 (blk-mq)│         │          │        │                     ││+hctx  ││          │             ││+hctx[]││
         │         │          │        │                     ││+req_q ││          │             ││(per   ││
         │         │          │        │                     ││(admin)││          │             ││CPU)   ││
         │         │          │        │                     │└───────┘│          │             ││+tags[]││
         │         │          │        │                     │         │          │             │└───────┘│
         │         │          │        │                     │         │          │             │+gendisk │
         │         │          │        │                     │         │          │             │+req_q   │
         │         │          │        │                     │         │          │             │(per NS) │
 ────────┼─────────┼──────────┼────────┼─────────────────────┼─────────┼──────────┼─────────────┼─────────┤
 NVMe    │┌───────┐│┌────────┐│┌──────┐│┌───────────────────┐│         │┌────────┐│┌───────────┐│         │
 PCIe    ││pci_   │││nvme_dev│││dev-> │││▸pci_enable_device ││         ││Identify│││IRQ 확장:  │││         │
 driver  ││driver │││kzalloc │││bar = │││ _mem (BAR 접근)   ││         ││Ctrl 응답│││pci_alloc_ │││.queue_rq│
 (kernel)││등록   │││        │││ioremap││▸pci_set_master    ││         ││저장:   │││irq_vectors│││= nvme_  │
         ││+nvme_ │││+INIT_  │││       ││ (Bus Master EN    ││         ││-NN     │││(N+1)      │││queue_   │
         ││wq    │││WORK    │││       ││  → DMA 허용)      ││         ││-OACS   │││           │││rq       │
         ││생성   │││(reset_ │││       ││▸pci_alloc_irq_    ││         ││-OCS    │││for qid    │││바인딩   │
         │└───────┘││work)   │││       ││ vectors(1,1)      ││         ││-MDTS   │││ in 1..N:  │││(admin/  │
         │        ││+dev->  │││       ││ (admin MSI-X)     ││         ││        │││ ▸nvme_    │││ IO 모두)│
         │        ││queues[]│││       ││▸CAP 레지스터 read ││         ││Identify│││  alloc_   │││         │
         │        ││=       │││       ││▸dev->dbs =        ││         ││NS list │││  queue(i) │││         │
         │        ││kcalloc │││       ││ bar + 4096        ││         ││        │││ ▸CREATE_  │││         │
         │        ││(N+1,빈)│││       ││ (doorbell base)   ││         ││SET_FEAT│││  IO_CQ    │││         │
         │        ││+nvme_  │││       ││▸configure_admin_Q:│         │ ││NUM_    │││ ▸CREATE_  │││         │
         │        ││init_   │││       ││ ▸nvme_alloc_queue │         │ ││QUEUES  │││  IO_SQ    │││         │
         │        ││ctrl    │││       ││  (0,depth):       ││         ││(개수   │││  (admin   │││         │
         │        ││(frame- │││       ││  admin SQ/CQ ring │         │ ││협상)   │││  명령)    │││         │
         │        ││work    │││       ││  dma_alloc_coh    │         │ │└────────┘│└───────────┘│         │
         │        ││등록)   │││       ││  → slot[0] 채움   │         │ │          │             │         │
         │        ││+DMA    │││       ││ ▸ASQ/ACQ base     │         │ │          │             │         │
         │        ││mask    │││       ││  reg MMIO write   │         │ │          │             │         │
         │        ││64bit   │││       ││ ▸CC.EN = 1        │         │ │          │             │         │
         │        │└────────┘│└──────┘││ ▸CSTS.RDY 폴링    │         │ │          │             │         │
         │        │          │        │└───────────────────┘│         │ │          │             │         │
 ════════╪═════════╪══════════╪════════╪═════════════════════╪═════════╪══════════╪═════════════╪═════════╡
 Host    │         │          │        │ ┌─────────┐         │         │          │ ┌─────────┐ │ PRP     │
 DRAM    │         │          │        │ │admin    │         │         │          │ │IO SQ/CQ │ │ buf     │
 (DMA-   │         │          │        │ │SQ/CQ    │         │         │          │ │rings×N  │ │ 적재    │
  coh)   │         │          │        │ │ring     │         │         │          │ │+PRP pool│ │         │
         │         │          │        │ │(slot 0) │         │         │          │ │(slot    │ │         │
         │         │          │        │ └────┬────┘         │         │          │ │ 1..N)   │ │         │
         │         │          │        │      │DMA 주소      │         │          │ └────┬────┘ │         │
 ════════╪═════════╪══════════╪════════╪══════╪══════════════╪═════════╪══════════╪══════╪══════╪═════════╡ ◀ PCIe
         │         │          │        │      ▼              │         │          │      ▼      │         │
 SSD     │         │          │┌──────┐│ ┌─────────┐         │         │          │ ┌─────────┐ │doorbell │
 BAR0    │         │          ││BAR0  ││ │CAP/CC/  │         │         │          │ │IO SQ/CQ │ │write    │
 (MMIO)  │         │          ││공간  ││ │CSTS 접근│         │         │          │ │Doorbells│ │from     │
         │         │          ││노출  ││ │ASQ/ACQ  │         │         │          │ │[1..N]   │ │driver   │
         │         │          ││      ││ │base reg │         │         │          │ │         │ │         │
         │         │          ││      ││ │+admin DB│         │         │          │ │         │ │         │
         │         │          │└──────┘│ └────┬────┘         │         │          │ └────┬────┘ │         │
 ────────┼─────────┼──────────┼────────┼──────┼──────────────┼─────────┼──────────┼──────┼──────┼─────────┤
         │         │          │        │      ▼              │         │          │      ▼      │         │
 SSD     │         │          │        │ ┌─────────┐         │         │┌────────┐│ ┌─────────┐ │SQE      │
 Ctrl FW │         │          │        │ │admin Q  │         │         ││Identify│││ │IO Q     │ │fetch    │
 (on-SSD)│         │          │        │ │processor│         │         ││명령    │││ │arbiter  │ │→ FTL    │
         │         │          │        │ │(CSTS.   │         │         ││처리    │││ │+ DMA    │ │→ LBA    │
         │         │          │        │ │ RDY)    │         │         ││(→CQE)  │││ │engine   │ │         │
         │         │          │        │ └─────────┘         │         │└────────┘│ │+ FTL L2P│ │         │
         │         │          │        │                     │         │          │ └────┬────┘ │         │
 ────────┼─────────┼──────────┼────────┼─────────────────────┼─────────┼──────────┼──────┼──────┼─────────┤
 NAND    │         │          │        │                     │         │          │      ▼      │(ready,  │
 Media   │         │          │        │                     │         │          │ Chan 0..K   │ read/   │
         │         │          │        │                     │         │          │ (read/prog) │ program)│
 ════════╧═════════╧══════════╧════════╧═════════════════════╧═════════╧══════════╧═════════════╧═════════╡

  ▣ 박스 = 그 Phase에서 그 레인에 "추가되는" 리소스 (이전 Phase 박스는 유지됨)
  ▸ 박스 안의 세부 이벤트 (시간 순서로 나열)
  세로 화살표(↓) = 공간 경계를 넘는 상호작용 (DRAM ring 주소 → BAR0 register 등)
  ════ 굵은 가로 점선 = PCIe 경계 (Host ↔ SSD)
```

### Phase 간 "결정적 사건" (발표 시 narration)

| 전환 | 무엇이 일어나는가 | 왜 중요한가 |
|---|---|---|
| ③ → ④ | BAR0이 매핑된 뒤에야 CAP/CC/CSTS 레지스터 접근 가능 | 매핑 없으면 CC.EN 쓸 주소조차 없음 |
| ④ 내부 | Admin Q ring을 DRAM에 만들고, 그 **DMA 주소를 BAR0의 ASQ/ACQ 레지스터에 MMIO write**, 그 다음 CC.EN=1 | 순서 뒤집히면 SSD가 Admin Q를 못 찾아 hang |
| ⑤ → ⑥ | Admin blk-mq 경로가 먼저 열려야 Identify 명령을 request로 감쌀 수 있음 | blk-mq 없이는 admin 명령 발행 불가 |
| ⑥ → ⑦ | Identify 결과의 `CAP.MQES`, SET_FEAT(NUM_QUEUES) 협상값으로 **IO Q 개수 결정** | 이전 Phase의 정보 없이 IO Q 생성 못 함 |
| ⑦ → ⑧ | IO Q가 살아야 `nr_hw_queues` 확정 → IO blk-mq tag_set 할당 가능 | 그래서 IO tag_set이 ⑦ **뒤**임 (⑤의 admin tag_set과 대칭) |

→ 8 phase는 **독립된 단계가 아니라 직렬 의존성**. 그래서 `nvme_probe()`가 한 함수 안에서 순차적으로 모두 호출.

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

## 4. BaM 초기화 Swimlane (발표용)

**실제 BaM (NVIDIA PDL @UIUC, ASPLOS'23) 소스 (`bam-study/module/` + `src/` + `include/`) 기반.**

Phase 경계는 insmod부터 GPU 커널 실행까지 **8단계** — NVMe의 `nvme_probe()` 8단계와 **의도적으로 같은 개수**로 대응시켜 대비가 드러나도록 구성. 핵심 차이는 **주체의 이동**이다: NVMe는 ①~⑧이 전부 커널, BaM은 ①②만 커널이고 ③~⑧이 **유저스페이스 + GPU**.

레인 구성: User / libnvm Kernel Module / Host DRAM (Admin 전용) / **GPU VRAM (신설)** / SSD BAR0 / SSD Controller FW / NAND

### Phase 한 줄 요약
- **① Module Load** — `insmod libnvm.ko` → `libnvm_helper_entry` (`module/pci.c:726`):
  1. `list_init(&ctrl_list)` + `list_init(&host_list)` + `list_init(&device_list)` — **세 개**의 전역 리스트 (컨트롤러 / 호스트 DMA 매핑 / GPU P2P 매핑) 초기화
  2. `alloc_chrdev_region(&dev_first, 0, max_num_ctrls, "libnvm helper")` — MAJOR 동적 + MINOR 64개 예약
  3. `dev_class = class_create(THIS_MODULE, "libnvm helper")` — `/sys/class/libnvm*/` 생성, udev 자동 mknod 트리거 준비
  4. `pci_register_driver(&driver)` — PCI 서브시스템에 등록. `driver.id_table = PCI_DEVICE_CLASS(PCI_CLASS_NVME,...)` 이므로 **모든 NVMe 클래스 장치**에 대해 `add_pci_dev()` 즉시 호출
- **② PCI Probe** — `add_pci_dev` (`module/pci.c:469`) 안에서 **순차로**:
  1. `ctrl_get(&ctrl_list, dev_class, dev, curr_ctrls)` — `struct ctrl` kzalloc, `ctrl_list`에 삽입, MINOR 번호 할당
  2. `pci_request_region(dev, 0, DRIVER_NAME)` — **BAR0만** reserve (BaM은 BAR0 외 BAR 사용 안 함)
  3. `pci_enable_device(dev)` — Command 레지스터 MEM/IO 비트 셋
  4. `ctrl_chrdev_create(ctrl, dev_first, &dev_fops)` (`module/ctrl.c:250`): `cdev_init(&ctrl->cdev, &dev_fops)` + `cdev_add(&ctrl->cdev, rdev, 1)` + `device_create(dev_class, ..., "libnvm%u", MINOR)` → **`/dev/libnvm0` 생성**
  5. `pci_set_master(dev)` — **Bus Master Enable** (이후 SSD가 host/GPU로 DMA 가능)
  → **커널 모듈의 일은 여기까지**. NVMe 커널 드라이버(`nvme_pci_enable`, `configure_admin_queue`, `CC.EN=1`, `Identify`, IO Q 생성)에 해당하는 작업은 **하나도 안 한다**. 단순 "chrdev 문지기 + DMA 매핑 서비스".
- **③ User Open + BAR0 Mmap + nvm_ctrl_init** — 유저스페이스 `Controller::Controller` (`include/ctrl.h:401`) 진입:
  1. `fd = open("/dev/libnvm0", O_RDWR)`
  2. `nvm_ctrl_init(&ctrl, fd)` 내부에서 `mmap(..., BAR0_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)` 호출 → 커널 `mmap_registers()` (`module/pci.c:185`) 진입: `vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot)` + `vm_iomap_memory(vma, pci_resource_start(pdev,0), ...)` → **BAR0 MMIO가 유저 VA로 매핑**
  3. 유저 `nvm_ctrl_t` 채움: `mm_ptr = BAR0 VA`, `CAP` 레지스터 readq → `page_size`, `dstrd`(doorbell stride), `max_qs` 추출, `dbs = mm_ptr + 4096` (doorbell base — NVMe 스펙 3.1.10)
- **④ Admin Q Setup (host DRAM)** — `Controller::Controller` 계속 (`ctrl.h:422`):
  1. `aq_mem = createDma(ctrl, page_size * 3)` — user `posix_memalign` 3페이지 → `ioctl(fd, NVM_MAP_HOST_MEMORY, &req)` → 커널 `map_userspace()` (`module/map.c:474`) → `map_user_pages()` (`map.c:348`): `get_user_pages(vaddr, n_pages, FOLL_WRITE, pages, NULL)` 으로 페이지 핀, `dma_map_page(dev, pg, PAGE_SIZE, DMA_BIDIRECTIONAL)` 반복 → `map->addrs[]` (IOVA 배열) `copy_to_user`
  2. `nvm_aq_create(&aq_ref, ctrl, aq_mem.get())` (`include/nvm_aq.h:92`, `src/rpc.cpp`):
     - CC.EN=0 writel, CSTS.RDY=0 폴링 (controller reset)
     - AQA 레지스터 writel (admin Q attrs: SQS/CQS size)
     - ASQ base 레지스터 writeq ← `aq_mem->ioaddrs[0]`
     - ACQ base 레지스터 writeq ← `aq_mem->ioaddrs[1]`
     - CC.EN=1, CSTS.RDY=1 폴링 (timeout = CAP.TO)
- **⑤ Identify + Negotiate + CUDA Bind** — `initializeController()` (`ctrl.h:306-339`):
  1. `nvm_admin_ctrl_info(aq_ref, &info, buf)` — **Identify Controller** (opcode 0x06, CNS=0x01, data buffer = aq_mem 3번째 페이지)
  2. `nvm_admin_ns_info(aq_ref, &ns, ns_id, buf)` — **Identify Namespace** (opcode 0x06, CNS=0x00)
  3. `nvm_admin_get_num_queues(aq_ref, &n_cqs, &n_sqs)` — **Get Features NUM_QUEUES** (opcode 0x0A, FID=0x07): 현재 허용치 질의
  4. `reserveQueues(MAX_QUEUES, MAX_QUEUES)` (`ctrl.h:530`) → `nvm_admin_request_num_queues(...)` — **Set Features NUM_QUEUES** (opcode 0x09): 허용치 협상 (NVMe와 동일)
  5. **`cudaHostRegisterIoMemory(ctrl.mm_ptr, NVM_CTRL_MEM_MINSIZE, ...)` (`ctrl.h:425`)** — BaM의 결정적 한 줄. BAR0 유저 VA를 **CUDA unified 주소공간에 등록**해서 GPU 커널이 **같은 VA로 doorbell을 MMIO write** 할 수 있게 한다 (P2P doorbell의 관문)
- **⑥ I/O Q Setup on GPU VRAM** — `for qp_id in 1..n_qps: new QueuePair(ctrl, aq_ref, qp_id, qs, cudaDevice)` (`include/queue.h:250`):
  1. `cudaSetDevice(cudaDevice)`
  2. `sq_mem = createDma(ctrl, qs*64, cudaDevice)` → 내부: `cudaMalloc(&sq_va, qs*64)` → `ioctl(fd, NVM_MAP_DEVICE_MEMORY, &req)` → 커널 `map_device_memory()` (`module/map.c:887`) → `map_gpu_memory()`: `nvidia_p2p_get_pages(0,0,va,size,&page_tbl, p2p_free_cb, gd)` (`map.c:766`) → `nvidia_p2p_dma_map_pages(pdev, page_tbl, &dma_map)` → `dma_map->hw_address[]` (**P2P IOVA** — GPU VRAM ↔ SSD 직통 주소) `copy_to_user`
  3. `cq_mem = createDma(ctrl, qs*16, cudaDevice)` — 같은 경로로 CQ ring도 GPU VRAM
  4. `nvm_admin_cq_create(aq_ref, &cq, qp_id, cq_mem.get(), ivec)` — **Admin 명령 opcode 0x05 (Create I/O CQ)**, CQ base = **P2P IOVA**, interrupt vector=0 (BaM은 IRQ 대신 polling)
  5. `cudaHostGetDevicePointer(&cq.db, mm_ptr + (2*qp_id+1)*4*(1<<dstrd), 0)` — GPU VA에서 CQ doorbell 접근
  6. `nvm_admin_sq_create(aq_ref, &sq, &cq, qp_id, sq_mem.get())` — **Admin 명령 opcode 0x01 (Create I/O SQ)**, SQ base = P2P IOVA, linked `CQID = cq.id`
  7. `cudaHostGetDevicePointer(&sq.db, mm_ptr + (2*qp_id)*4*(1<<dstrd), 0)` — SQ doorbell GPU VA
  8. `init_gpu_specific_struct(cudaDevice)` (`queue.h:187`): **GPU VRAM 전용** lock-free 동기화 구조 6종 할당 — `sq_tickets[qs]` (ticket 예약), `sq_tail_mark[qs]` (slot 채움 완료 플래그), `sq_cid[65536]` (CID↔slot 역매핑), `cq_head_mark[qs]`, `cq_pos_locks[qs]`
- **⑦ page_cache + range_t + array_t (GPU-side runtime 객체)** — `page_cache_t pc(page_size, n_pages, ctrl_list, ...)` (`include/page_cache.h`):
  1. `createBuffer(n_pages*page_size, cudaDevice)` + `NVM_MAP_DEVICE_MEMORY` → **데이터 풀** (GPU VRAM, SSD가 직접 DMA write할 타겟)
  2. `createBuffer(n_pages*sizeof(data_page_t))` → per-page 상태 배열 (FREE/VALID/BUSY atomic)
  3. `createBuffer(n_slots*sizeof(cache_page_t))` → per-slot lock + LBA→slot translation
  4. `createBuffer(n_prps*sizeof(u64))` + P2P 등록 → **PRP list** (>4KB 전송용)
  - `range_t<T> rng(start_lba, n_blocks, ctrl_id, dist)` — 논리적 데이터 영역 (REPLICATE/STRIPE)
  - `array_t<T> arr(rng, ..., cudaDevice)` — GPU-side view, `bam_ptr` 반환
  - **`cudaMemcpy(d_ctrl_ptr, this, sizeof(Controller), H2D)` (`ctrl.h:453`)** — Controller 객체 자체를 GPU VRAM에 복제 → GPU 커널이 `d_ctrl->sq[i].db` 참조를 dereference 가능
- **⑧ Kernel Launch (완성)** — `app_kernel<<<grid, block>>>(d_ctrls, d_pc, d_arr, ...)` → 각 GPU thread:
  1. `bam_ptr<T>::read(lba)` → page_cache 룩업
  2. miss → `sq.tickets` atomicAdd로 SQ slot 예약
  3. `sq_cmds[tail] = SQE` (VRAM→VRAM store)
  4. `*sq.db = sq.tail` — **GPU 커널이 직접 BAR0 doorbell MMIO write** (PCIe P2P, CPU 개입 없음)
  5. CQ polling (`cqes[cq.head].status` atomic read)
  6. SSD가 page_cache 데이터 슬롯으로 P2P DMA write → GPU thread read

```
 시간 ───────────────────────────────────────────────────────────────────────────────────────────────────────▶
         │①Module  │②PCI      │③User+BAR0│④Admin Q   │⑤Identify+ │⑥I/O Q on    │⑦page_cache│⑧Kernel  │
         │ Load    │ Probe    │ Mmap+init│ (host DRAM)│ CUDA Bind │ GPU VRAM    │ +array_t  │ Launch  │
 ════════╪═════════╪══════════╪══════════╪═══════════╪═══════════╪═════════════╪═══════════╪═════════╡
 User    │         │          │┌────────┐│┌─────────┐│┌─────────┐│┌───────────┐│┌─────────┐│┌───────┐│
 space   │         │          ││open()  │││createDma│││Identify │││QueuePair  │││page_    │││kernel ││
 (host   │         │          ││mmap    │││(aq_mem  │││Ctrl+NS  │││×N:        │││cache_t  │││launch ││
  CPU)   │         │          ││BAR0    │││3 host   │││GetFeat  │││▸cudaMalloc│││(GPU)    │││args:  ││
         │         │          ││        │││ pages)  │││NUM_Q    │││▸ioctl MAP │││range_t  │││d_ctrls││
         │         │          ││nvm_ctrl│││nvm_aq_  │││SetFeat  │││▸admin     │││array_t  │││d_pc   ││
         │         │          ││_init:  │││create:  │││NUM_Q    │││ CREATE_IO │││         │││d_arr  ││
         │         │          ││CAP read│││ASQ/ACQ  │││         │││ _CQ/_SQ   │││cudaMem  │││       ││
         │         │          ││dstrd,  │││reg write│││cudaHost │││▸GPU DB    │││cpy(ctrl │││       ││
         │         │          ││max_qs  │││CC.EN=1  │││Register │││ GetDevPtr │││ , H2D)  │││       ││
         │         │          ││dbs base│││CSTS poll│││IoMemory │││▸gpu sync  │││         │││       ││
         │         │          │└──┬─────┘│└────┬────┘│└────┬────┘│└─────┬─────┘│└────┬────┘│└────┬──┘│
 ────────┼─────────┼──────────┼──mmap────┼──ioctl─────┼────MMIO───┼────ioctl────┼───ioctl────┼──launch┤
 libnvm  │┌───────┐│┌────────┐│┌────────┐│┌─────────┐│           │┌───────────┐│┌─────────┐│         │
 Kernel  ││lib_   │││add_pci_│││mmap_   │││map_user_│││           │││map_device_│││map_dev_ │││         │
 Module  ││helper_│││dev:    │││registers│││space:  │││           │││memory:    │││memory   │││         │
 (얇음   ││entry: │││ctrl_get│││vm_iomap│││get_user │││           │││nvidia_p2p_│││(2nd     │││         │
  ②까지) ││list_  │││request_│││_memory  │││_pages   │││           │││get_pages  │││call for │││         │
         ││init×3 │││region  │││(BAR0,   │││dma_map_ │││           │││dma_map_   │││pc pages)│││         │
         ││alloc_ │││enable_ │││pgprot_  │││page:    │││           │││pages:     │││         │││         │
         ││chrdev │││device  │││nocached)│││host IOVA│││           │││P2P IOVA   │││         │││         │
         ││class_ │││chrdev_ │││         │││ returned│││           │││ returned  │││         │││         │
         ││create │││create  │││         │││         │││           │││           │││         │││         │
         ││pci_reg│││set_    │││         │││         │││           │││           │││         │││         │
         ││ister_ │││master  │││         │││         │││           │││           │││         │││         │
         ││driver │└────────┘│└─────────┘│└────┬────┘│           │└──────┬────┘│└────┬────┘│         │
         │└───────┘│/dev/     │          │     │IOVA │           │       │P2P  │     │P2P  │         │
         │         │libnvm0   │          │     ▼     │           │       ▼     │     ▼     │         │
 ════════╪═════════╪══════════╪══════════╪═══════════╪═══════════╪═════════════╪═══════════╪═════════╡
 Host    │         │          │          │┌─────────┐│           │             │           │         │
 DRAM    │         │          │          ││ASQ/ACQ  ││           │  (사용 안   │           │         │
 (Admin  │         │          │          ││+data buf││           │   함 — I/O  │           │         │
  only)  │         │          │          ││3 pages  ││           │   전부 VRAM)│           │         │
         │         │          │          │└────┬────┘│           │             │           │         │
 ═ ═ ═ ═ ╪═ ═ ═ ═ ═╪═ ═ ═ ═ ═ ╪═ ═ ═ ═ ═ ╪═ ═ │═ ═ ═╪═ ═ ═ ═ ═ ═╪═ ═ ═ ═ ═ ═ ╪═ ═ ═ ═ ═ ═╪═ ═ ═ ═ ╡ ◀ PCIe (H↔GPU)
 GPU     │         │          │          │    │      │           │┌───────────┐│┌─────────┐│┌───────┐│
 VRAM    │         │          │          │    │      │           │││SQ/CQ rings││││data    │││app    ││
 (CUDA   │         │          │          │    │      │           │││×N (P2P    │││pool    │││kernel ││
  ctx)   │         │          │          │    │      │           │││ IOVA)     │││+PRP    │││ threads││
         │         │          │          │    │      │           │││sq_tickets │││list    │││bam_ptr││
         │         │          │          │    │      │           │││sq_cid     │││cache   │││.read()││
         │         │          │          │    │      │           │││cq_locks   │││state   │││       ││
         │         │          │          │    │      │           │││+ d_ctrl   │││array_t │││       ││
         │         │          │          │    │      │           │└─────┬─────┘│└────┬────┘│└───┬───┘│
 ═ ═ ═ ═ ╪═ ═ ═ ═ ═╪═ ═ ═ ═ ═ ╪═ ═ ═ ═ ═ ╪═ ═ │═ ═ ═╪═ ═ ═ ═ ═ ═╪═ ═ ═ │═ ═ ═ ╪═ ═ ═ │═ ═ ═╪═ ═ │═ ═╡ ◀ PCIe (GPU↔SSD)
 SSD     │         │┌────────┐│┌────────┐│    ▼      │           │      ▼      │     ▼     │    ▼    │
 BAR0    │         ││BAR0    │││CAP/CC/ ││┌─────────┐│┌─────────┐│┌───────────┐│           │doorbell │
 (MMIO)  │         ││reserved│││CSTS    │││ASQ/ACQ  │││Admin    │││IO SQ/CQ   ││           │write    │
         │         ││(request│││regs    │││base regs│││Submit   │││Doorbells  ││           │from     │
         │         ││_region)│││(유저 VA│││ written │││Queue    │││[1..N]     ││           │GPU      │
         │         ││        │││ 접근)  │││AQA reg  │││CAP.TO   │││(GPU VA)   ││           │(P2P)    │
         │         │└────────┘│└────────┘│└────┬────┘│└────┬────┘│└─────┬─────┘│           │         │
 ────────┼─────────┼──────────┼──────────┼─────┼─────┼─────┼─────┼──────┼──────┼───────────┼─────────┤
         │         │          │          │     ▼     │     ▼     │      ▼      │           │  SQE    │
 SSD     │         │          │          │┌─────────┐│┌─────────┐│┌───────────┐│           │ fetch   │
 Ctrl FW │         │          │          ││ctrl    ││││Identify │││CREATE_IO_ ││           │(DMA     │
 (on-SSD)│         │          │          ││reset & ││││Ctrl/NS  │││SQ/CQ 처리 ││           │from     │
         │         │          │          ││enable  ││││SetFeat  │││ → I/O Q   ││           │VRAM     │
         │         │          │          ││(CC.EN=1│││NUM_Q    │││  registered ││           │P2P)     │
         │         │          │          │└────────┘│└─────────┘│└─────┬─────┘│           │→ FTL    │
         │         │          │          │           │           │      ▼      │           │  L2P    │
         │         │          │          │           │           │┌───────────┐│           │         │
         │         │          │          │           │           │││FTL L2P    ││           │         │
         │         │          │          │           │           │└─────┬─────┘│           │         │
 ────────┼─────────┼──────────┼──────────┼───────────┼───────────┼──────┼──────┼───────────┼─────────┤
 NAND    │         │          │          │           │           │      ▼      │           │ page    │
 Media   │         │          │          │           │           │  Channels   │           │ read    │
         │         │          │          │           │           │  0..K       │           │ → VRAM  │
         │         │          │          │           │           │  (ready)    │           │ (P2P)   │
 ════════╧═════════╧══════════╧══════════╧═══════════╧═══════════╧═════════════╧═══════════╧═════════╡

  ▣ 박스 = 그 Phase에서 그 레인에 "추가되는" 리소스 (이전 Phase 박스는 유지)
  ══ 굵은 가로 점선 = PCIe 경계 (두 개! Host↔GPU, GPU↔SSD)
  세로 화살표(↓) = 공간 경계를 넘는 상호작용 (mmap / ioctl / MMIO / P2P DMA)
```

### Phase 간 "결정적 사건" (발표 시 narration)

| 전환 | 무엇이 일어나는가 | 왜 중요한가 |
|---|---|---|
| ① → ② | `pci_register_driver` 직후 PCI 서브시스템이 **즉시** `add_pci_dev` 호출 (기존 NVMe 장치 전부) | 한 함수 리턴 안에서 `/dev/libnvm0`까지 생성 완료 |
| ② 내부 | **`pci_set_master` 후 BAR0 ioremap/CC.EN은 안 함** | NVMe 커널 드라이버와의 **핵심 차이**. 커널은 DMA만 허용하고 컨트롤러 초기화는 유저에게 위임 |
| ② → ③ | 유저가 `open` → `mmap` 호출해야 비로소 BAR0가 프로세스 VA에 매핑 | 커널 모듈이 자발적으로 ioremap하지 않으므로 유저 트리거가 필수 |
| ③ → ④ | BAR0 mmap 완료 후에야 CC/CSTS 레지스터 접근 가능 → Admin Q 세팅 가능 | NVMe와 동일한 의존성이지만 **호출 주체가 커널이 아닌 유저** |
| ④ 내부 | 유저 페이지를 **`get_user_pages` + `dma_map_page`**로 핀 → 반환된 IOVA를 **유저가 직접** ASQ/ACQ 레지스터에 MMIO write | NVMe 커널 드라이버의 `dma_alloc_coherent` 대신 **user-pinned** 경로. 페이지 수명이 유저 프로세스에 묶임 |
| ④ → ⑤ | Admin Q 살아야 Identify/Set Features 발행 가능 | NVMe와 동일 |
| ⑤ 내부 | **`cudaHostRegisterIoMemory(BAR0)`** — BAR0 유저 VA를 CUDA 컨텍스트에 등록 | 이게 있어야 ⑥에서 `cudaHostGetDevicePointer`로 GPU가 doorbell을 쓸 수 있음 — BaM의 **doorbell P2P 관문** |
| ⑤ → ⑥ | Set Features로 협상된 queue 개수로 I/O Q 개수 결정 | NVMe와 동일 |
| ⑥ 내부 | `cudaMalloc` → `ioctl(NVM_MAP_DEVICE_MEMORY)` → `nvidia_p2p_get_pages` + `nvidia_p2p_dma_map_pages` → **P2P IOVA** → 이 주소를 Admin `CREATE_IO_SQ`의 SQ base로 등록 | **BaM 초기화의 하이라이트**. 이 체인이 완성되어야 SSD가 GPU VRAM으로 직접 DMA 가능 |
| ⑥ → ⑦ | I/O Q가 살아야 page_cache가 submit할 대상 존재 | page_cache는 "큐 준비된 상태"를 전제 |
| ⑦ 내부 | `cudaMemcpy(d_ctrl, &ctrl, H2D)` — Controller 객체를 GPU VRAM에 **복제** | GPU 커널이 `d_ctrl->sq[i].db`를 dereference해 doorbell을 쓰려면 구조 자체가 GPU에 있어야 함 |
| ⑦ → ⑧ | 모든 GPU-side 객체가 준비되면 application `__global__` 커널 런치 가능 | NVMe의 `/dev/nvmeXnY` 노출에 해당하는 "사용 가능 상태" 완성 |

→ 8 phase는 **독립된 단계가 아니라 직렬 의존성**. 하지만 NVMe와 달리 ①②만 커널에서, ③~⑧은 **유저스페이스 생성자 호출 한 번(`Controller` + `QueuePair` + `page_cache_t`)에 전부 묶여** 있음. `nvme_probe()`가 커널에서 하던 일을 **유저 C++ 생성자 체인이 대체**.

---

## 4.1 BaM 초기화 상세 호출 Flow (참고용)

위 swimlane의 각 phase가 실제 소스에서 어떤 함수 호출 트리를 거치는지. NVMe §3에 대응.

```
insmod libnvm.ko
 │
 └─[Phase ①: Module Load]────────────────────────── module/pci.c
     libnvm_helper_entry()                            @726
     ├─ list_init(&ctrl_list)                         @730  ◀ 컨트롤러 리스트
     ├─ list_init(&host_list)                         @732  ◀ 호스트 DMA 매핑
     ├─ list_init(&device_list)                       @734  ◀ GPU P2P 매핑
     ├─ alloc_chrdev_region(&dev_first, 0, 64, DRIVER_NAME)  @739
     ├─ dev_class = class_create(THIS_MODULE, ...)    @756
     └─ pci_register_driver(&driver)                  @775
         └─ driver = {
              .id_table = [PCI_DEVICE_CLASS(NVMe, mask)],
              .probe    = add_pci_dev,
              .remove   = remove_pci_dev,
           }
           → PCI 서브시스템이 기존 NVMe 장치마다 즉시:
              add_pci_dev(pdev, id) 호출

 ┌─[Phase ②: PCI Probe callback]──────────────────── module/pci.c
 │  add_pci_dev(dev, id)                              @469
 │  ├─ ctrl = ctrl_get(&ctrl_list, dev_class, dev, curr_ctrls)  @493
 │  │   (module/ctrl.c) → kzalloc(struct ctrl) + list_insert
 │  ├─ pci_request_region(dev, 0, DRIVER_NAME)        @508  ◀ BAR0만
 │  ├─ pci_enable_device(dev)                         @526
 │  ├─ ctrl_chrdev_create(ctrl, dev_first, &dev_fops) @544
 │  │   (module/ctrl.c:250)
 │  │   ├─ ctrl->rdev = MKDEV(MAJOR(first), MINOR(first)+ctrl->number)
 │  │   ├─ cdev_init(&ctrl->cdev, fops)
 │  │   ├─ cdev_add(&ctrl->cdev, ctrl->rdev, 1)
 │  │   └─ device_create(dev_class, NULL, rdev, NULL, "libnvm%u")
 │  │        → /dev/libnvm0 udev로 생성
 │  └─ pci_set_master(dev)                            @564  ◀ Bus Master EN

---- 여기서부터 유저스페이스 ----

 User application (예: benchmarks/block/main.cu)
 │
 ├─[Phase ③: Controller 생성자]─────────────────── include/ctrl.h + src/ctrl.cpp
 │  Controller ctrl(device_id, ns_id, cudaDevice, n_qps, queue_depth)  @401
 │  ├─ fd = open("/dev/libnvm0", O_RDWR)              @406
 │  ├─ nvm_ctrl_init(&this->ctrl, fd)                 (include/nvm_ctrl.h)
 │  │   ├─ mmap(NULL, BAR0_SIZE, PROT_RW, MAP_SHARED, fd, 0)
 │  │   │   └─ 커널: mmap_registers()                 module/pci.c:185
 │  │   │       ├─ ctrl = ctrl_find_by_inode(...)
 │  │   │       ├─ vma->vm_page_prot = pgprot_noncached(...)
 │  │   │       └─ vm_iomap_memory(vma, pci_resource_start(pdev,0), ...)
 │  │   ├─ ctrl->mm_ptr = (BAR0 유저 VA)
 │  │   ├─ CAP readq → page_size, dstrd, max_qs 추출
 │  │   └─ ctrl->dbs = ctrl->mm_ptr + 4096 (doorbell base)
 │
 ├─[Phase ④: Admin Queue setup]─────────────────── ctrl.h:422
 │  ├─ aq_mem = createDma(ctrl, page_size * 3)
 │  │   ├─ posix_memalign(&va, page_size, 3*page_size)
 │  │   └─ ioctl(fd, NVM_MAP_HOST_MEMORY, &req={va, n_pages=3})
 │  │       └─ 커널: map_ioctl() → map_userspace()     module/map.c:474
 │  │           └─ map_user_pages()                    module/map.c:348
 │  │               ├─ kvcalloc(n_addrs, struct page*)
 │  │               ├─ get_user_pages(vaddr, n, FOLL_WRITE, pages, NULL)  @383
 │  │               └─ for i in 0..n:
 │  │                    dma_map_page(dev, pages[i], 0, PAGE_SIZE, BIDIR)
 │  │                    map->addrs[i] = dma_addr  ◀ IOVA
 │  │               → copy_to_user(req.ioaddrs, map->addrs)
 │  │
 │  └─ initializeController(*this, ns_id)             ctrl.h:306
 │      └─ nvm_aq_create(&aq_ref, ctrl, aq_mem.get()) include/nvm_aq.h:92
 │          (src/rpc.cpp 또는 src/admin.cpp)
 │          ├─ writel(CC, 0x0)                 ← Reset (CC.EN=0)
 │          ├─ while (readl(CSTS) & RDY) ;     ← RDY=0 폴링
 │          ├─ writel(AQA, (CQS<<16)|SQS)      ← Admin Q size
 │          ├─ writeq(ASQ, aq_mem->ioaddrs[0]) ← ASQ base = host IOVA
 │          ├─ writeq(ACQ, aq_mem->ioaddrs[1]) ← ACQ base
 │          ├─ writel(CC, EN|...)              ← CC.EN=1
 │          └─ while (!(readl(CSTS) & RDY)) ;  ← RDY=1 폴링 (timeout=CAP.TO)
 │
 ├─[Phase ⑤: Identify + Negotiate + CUDA bind]──── ctrl.h:317-434
 │  initializeController 계속:
 │  ├─ nvm_admin_ctrl_info(aq_ref, &info, buf)        ctrl.h:318
 │  │   → Admin opcode 0x06, CNS=0x01 (Identify Controller)
 │  ├─ nvm_admin_ns_info(aq_ref, &ns, ns_id, buf)     ctrl.h:326
 │  │   → Admin opcode 0x06, CNS=0x00 (Identify Namespace)
 │  ├─ nvm_admin_get_num_queues(aq_ref, &n_cqs, &n_sqs)  ctrl.h:334
 │  │   → Admin opcode 0x0A, FID=0x07 (Get Features NUM_QUEUES)
 │  │
 │  ├─ cudaHostRegisterIoMemory(                      ctrl.h:425
 │  │      ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE,
 │  │      cudaHostRegisterIoMemory)
 │  │   ▶ BAR0 유저 VA를 CUDA unified space에 등록
 │  │   ▶ GPU가 이후 같은 VA로 doorbell MMIO 접근 가능
 │  │
 │  └─ reserveQueues(MAX_QUEUES, MAX_QUEUES)          ctrl.h:434,530
 │      └─ nvm_admin_request_num_queues(&n_sq, &n_cq)
 │          → Admin opcode 0x09, FID=0x07 (Set Features NUM_QUEUES)
 │
 ├─[Phase ⑥: I/O Queue on GPU VRAM]──────────────── ctrl.h:442
 │  for qp_id in 1..n_qps:
 │    h_qps[qp_id] = new QueuePair(ctrl, aq_ref, qp_id,
 │                                 queue_depth, cudaDevice)    queue.h:250
 │    │
 │    ├─ cudaSetDevice(cudaDevice)
 │    │
 │    ├─ sq_mem = createDma(ctrl, qs*64, cudaDevice)  ◀ GPU VRAM SQ
 │    │   ├─ cudaMalloc(&sq_va, qs*64)
 │    │   └─ ioctl(fd, NVM_MAP_DEVICE_MEMORY, &req={sq_va, n_pages})
 │    │       └─ 커널: map_device_memory()            module/map.c:887
 │    │           └─ map_gpu_memory()                 module/map.c:~740
 │    │               ├─ nvidia_p2p_get_pages(0,0,va,sz,&pg_tbl,
 │    │               │                         p2p_free_cb, gd)  @766
 │    │               ├─ nvidia_p2p_dma_map_pages(pdev, pg_tbl,
 │    │               │                            &dma_map)
 │    │               └─ map->addrs[i] = dma_map->hw_address[i]
 │    │                  ◀ P2P IOVA (GPU VRAM ↔ SSD 직통)
 │    │               → copy_to_user(req.ioaddrs, map->addrs)
 │    │
 │    ├─ cq_mem = createDma(ctrl, qs*16, cudaDevice)  ◀ GPU VRAM CQ
 │    │   (동일 경로)
 │    │
 │    ├─ nvm_admin_cq_create(aq_ref, &cq, qp_id,
 │    │                      cq_mem.get(), /*ivec=*/0)
 │    │   → Admin opcode 0x05 (Create I/O CQ)
 │    │     CQ base = P2P IOVA, interrupt=0 (BaM=polling)
 │    │
 │    ├─ cudaHostGetDevicePointer(&cq.db,
 │    │       ctrl->mm_ptr + (2*qp_id+1)*4*(1<<dstrd), 0)
 │    │   ◀ GPU가 접근할 CQ doorbell VA
 │    │
 │    ├─ nvm_admin_sq_create(aq_ref, &sq, &cq, qp_id,
 │    │                      sq_mem.get())
 │    │   → Admin opcode 0x01 (Create I/O SQ)
 │    │     SQ base = P2P IOVA, linked CQID = cq.id
 │    │
 │    ├─ cudaHostGetDevicePointer(&sq.db,
 │    │       ctrl->mm_ptr + (2*qp_id)*4*(1<<dstrd), 0)
 │    │
 │    └─ init_gpu_specific_struct(cudaDevice)         queue.h:187
 │        ├─ sq_tickets  = createBuffer(qs*padded, cudaDev) ◀ ticket alloc
 │        ├─ sq_tail_mark= createBuffer(qs*padded, cudaDev)
 │        ├─ sq_cid      = createBuffer(65536*padded, cudaDev)
 │        ├─ cq_head_mark= createBuffer(cq.qs*padded, cudaDev)
 │        └─ cq_pos_locks= createBuffer(cq.qs*padded, cudaDev)
 │
 ├─[Phase ⑦: page_cache + range_t + array_t]─────── include/page_cache.h
 │  page_cache_t pc(page_size, n_pages, cudaDevice,
 │                  ctrl_list, ...)
 │  ├─ pages      = createBuffer(n_pages*page_size, cudaDev)
 │  │              + ioctl(NVM_MAP_DEVICE_MEMORY) 로 P2P 등록
 │  │              ◀ 데이터 풀 (SSD가 이리로 DMA write)
 │  ├─ data_pages = createBuffer(n_pages*sizeof(data_page_t))
 │  ├─ cache_pages= createBuffer(n_slots*sizeof(cache_page_t))
 │  └─ prp_list   = createBuffer(n_prps*sizeof(u64))
 │                  + P2P 등록                         ◀ >4KB 전송용
 │
 │  range_t<T> rng(start_lba, n_blocks, ctrl_id, dist) ◀ REPLICATE/STRIPE
 │  array_t<T> arr(rng, ..., cudaDevice)               ◀ GPU view (bam_ptr)
 │
 │  cudaMalloc(&d_ctrl_ptr, sizeof(Controller))        ctrl.h:451
 │  cudaMemcpy(d_ctrl_ptr, this,                       ctrl.h:453
 │             sizeof(Controller), H2D)
 │  ◀ Controller 객체 자체를 GPU VRAM에 복제
 │
 └─[Phase ⑧: Kernel Launch]────────────────────── benchmarks/*.cu
    app_kernel<<<grid, block>>>(d_ctrls, d_pc, d_arr, ...)
    │
    └─ __device__ bam_ptr<T>::read(lba)
        ├─ page_cache 룩업 (cache_page_t state atomic)
        ├─ miss → sq.tickets atomicAdd → slot 예약
        ├─ sq_cmds[tail] = SQE (VRAM store)
        ├─ *sq.db = sq.tail   ◀ GPU → BAR0 doorbell MMIO (P2P)
        ├─ CQ polling (cqes[cq.head].status)
        └─ 데이터는 SSD → page_cache.pages (P2P DMA)
```

### 핵심: DMA 매핑의 "세 공간 삼중주"

```
NVMe 커널 드라이버 (§3 참고)
──────────────────────────
dma_alloc_coherent 두 번 (Admin Q + IO Q 모두 host DRAM)
│
▼
"같은 커널 배열(dev->queues[])을 두 단계로 점진 완성"

BaM (§4)
──────────────────────────
Phase ④: get_user_pages + dma_map_page          ← Host DRAM IOVA (Admin)
              │
              ▼
          aq_mem->ioaddrs[]
Phase ⑥: nvidia_p2p_get_pages + nvidia_p2p_dma_map_pages  ← P2P IOVA (IO Q)
              │
              ▼
          sq_mem->ioaddrs[], cq_mem->ioaddrs[]
Phase ⑦: nvidia_p2p_dma_map_pages 재사용          ← P2P IOVA (데이터 풀)
              │
              ▼
          pages, prp_list
│
▼
"세 공간(user-pinned host DRAM / GPU VRAM SQ·CQ / GPU VRAM 데이터 풀)이
 각각 다른 커널 경로로 IOVA를 얻음. NVMe처럼 '같은 배열 두 번 채움'이 아니라
 '세 개의 별개 DMA 매핑이 각기 다른 ioctl로 생성'"
```

→ BaM 초기화의 구조적 특징은 `dma_alloc_coherent`의 **부재**. 모든 DMA 버퍼는 **이미 존재하는 유저/GPU 메모리를 핀 + 매핑**하는 방식이라, "커널이 버퍼를 만들어준다"가 아니라 "유저가 만든 버퍼에 대해 커널이 IOVA를 반환"하는 뒤집힌 모델.

### 단계별 함수/객체 요약 (컴팩트 표)

| Phase | 주체 | 주요 함수 | 파일:라인 | 생성 객체 / 위치 |
|---|---|---|---|---|
| ① Module Load | Kernel | `libnvm_helper_entry` | `module/pci.c:726` | `ctrl_list`, chrdev region, `pci_driver` |
| ② PCI Probe | Kernel | `add_pci_dev` → `ctrl_chrdev_create` | `module/pci.c:469`, `module/ctrl.c:250` | `struct ctrl`, `/dev/libnvm0` |
| ③ User+Mmap | User + Kernel | `Controller::Controller` → `mmap_registers` | `include/ctrl.h:401`, `module/pci.c:185` | `nvm_ctrl_t` (BAR0 유저 VA), CAP 파싱 |
| ④ Admin Q | User + Kernel | `createDma(host)` → `map_userspace` → `nvm_aq_create` | `include/ctrl.h:422`, `module/map.c:474` | **Host DRAM**: ASQ/ACQ/data ring 3페이지 |
| ⑤ Identify | User + GPU | `nvm_admin_*` + `cudaHostRegisterIoMemory` | `include/ctrl.h:306-434` | `info`, `ns`, CUDA BAR0 binding |
| ⑥ I/O Q | User + Kernel + GPU | `QueuePair::QueuePair` → `map_device_memory` → `nvm_admin_cq/sq_create` | `include/queue.h:250`, `module/map.c:887` | **GPU VRAM**: SQ/CQ rings ×N + sync 구조 |
| ⑦ page_cache | User + GPU | `page_cache_t` + `range_t` + `array_t` + `cudaMemcpy(H2D Controller)` | `include/page_cache.h`, `ctrl.h:451-453` | **GPU VRAM**: 데이터 풀, PRP list, d_ctrl |
| ⑧ Kernel Launch | GPU | `app_kernel<<<>>>` → `bam_ptr::read` | `benchmarks/*.cu` | GPU thread가 직접 SQ write + doorbell MMIO |

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
