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

레인 구성: User / Block Layer / NVMe Driver / Host DRAM / SSD BAR0 / SSD Controller FW / NAND

```
 시간 ─────────────────────────────────────────────────────────────────────────▶
        │①insmod │②PCI    │③Ctrl   │④Admin Q│⑤I/O Q  │⑥blk-mq │⑦NS scan│완성   │
        │nvme.ko │probe   │enable  │ setup  │ setup  │ setup  │        │(idle) │
 ═══════╪════════╪════════╪════════╪════════╪════════╪════════╪════════╪═══════╡
        │        │        │        │        │        │        │        │       │
 User   │        │        │        │        │        │        │┌──────┐│  ▲    │
 space  │        │        │        │        │        │        ││/dev/ ││  │    │
        │        │        │        │        │        │        ││nvme0n1│ read()│
        │        │        │        │        │        │        │└──▲───┘│       │
 ───────┼────────┼────────┼────────┼────────┼────────┼────────┼───┼────┼───────┤
        │        │        │        │        │        │        │   │    │       │
 VFS /  │        │        │        │        │        │┌──────┐│┌──┴───┐│       │
 Block  │        │        │        │        │        ││blk_mq││gendisk││bio →  │
 Layer  │        │        │        │        │        ││tag_  ││+ req_ ││req    │
 (blk-  │        │        │        │        │        ││set   ││queue  ││       │
  mq)   │        │        │        │        │        ││hctx[]││       ││       │
        │        │        │        │        │        ││tags[]││       ││       │
        │        │        │        │        │        │└──┬───┘│└──────┘│       │
 ───────┼────────┼────────┼────────┼────────┼────────┼───┼────┼────────┼───────┤
        │        │        │        │        │        │   │    │        │       │
 NVMe   │┌──────┐│┌──────┐│┌──────┐│┌──────┐│┌──────┐│   │    │        │       │
 PCIe   ││pci_  │││nvme_ │││CC.EN │││Identify││alloc_│└─▶ .queue_rq =   │       │
 driver ││driver│││dev   │││=0→1  │││ctrl   │││queue ││   nvme_queue_rq│queue_ │
 (kernel│││등록  │││BAR0  │││설정  │││+NUM_  │││×N    ││                │rq()  │
        │└──────┘││ioremap││CAP읽 │││QUEUES││CREATE││                │       │
        │        │└──┬───┘│└──┬───┘│└──┬───┘││IO_SQ/││                │       │
        │        │   │    │   │    │   │    ││   CQ ││                │       │
        │        │   │    │   │    │   │    │└──┬───┘│                │       │
 ───────┼────────┼───┼────┼───┼────┼───┼────┼───┼────┼────────┼───────┼───────┤
        │        │   │    │   │    │   │    │   │    │        │       │       │
 Host   │        │   │    │   │    │   ▼    │   ▼    │        │       │       │
 DRAM   │        │   │    │   │    │┌──────┐│┌──────┐│        │       │ PRP   │
 (DMA)  │        │   │    │   │    ││ASQ/  │││SQ/CQ │││        │       │ buf   │
        │        │   │    │   │    ││ACQ   │││ring  │││        │       │ 적재  │
        │        │   │    │   │    ││ring  │││×N    │││        │       │       │
        │        │   │    │   │    ││(nvme_│││+ PRP │││        │       │       │
        │        │   │    │   │    ││queue0│││ pool ││        │        │       │
        │        │   │    │   │    │└──┬───┘│└──┬───┘│        │       │       │
 ═ ═ ═ ═╪═ ═ ═ ═╪═ ═│═ ═ ╪═ ═│═ ═ ╪═ ═│═ ═ ╪═ ═│═ ═ ╪═ ═ ═ ═╪═══════╪PCIe═══╡
        │        │   ▼    │   ▼    │   ▼    │   ▼    │        │       │       │
 SSD    │        │┌──────┐│┌──────┐│┌──────┐│┌──────┐│        │       │ door- │
 BAR0   │        ││BAR0  │││CAP/  │││ASQ_  │││SQ/CQ │││        │       │ bell  │
 (MMIO) │        ││주소  │││CC/   │││TDBL, │││Door- │││        │       │ write │
        │        ││공간  │││CSTS  │││ACQ_  │││bells │││        │       │       │
        │        ││노출  │││regs  │││HDBL  │││[1..N]│││        │       │       │
        │        │└──────┘│└──────┘│└──┬───┘│└──┬───┘│        │       │       │
 ───────┼────────┼────────┼────────┼───┼────┼───┼────┼────────┼───────┼───────┤
        │        │        │        │   │    │   │    │        │       │       │
 SSD    │        │        │        │   ▼    │   ▼    │        │       │ SQE   │
 Ctrl   │        │        │        │┌──────┐│┌──────┐│        │       │ fetch │
 FW     │        │        │        ││Admin │││I/O Q │││        │       │ → FTL │
 (on-   │        │        │        ││Q     │││arbiter│        │       │ →LBA  │
  SSD)  │        │        │        ││proc. │││round- │        │       │       │
        │        │        │        │└──┬───┘│└rob   │        │        │       │
        │        │        │        │   │    │└──┬───┘│        │       │       │
        │        │        │        │   ▼    │   ▼    │        │       │       │
        │        │        │        │┌──────┐│┌──────┐│        │       │       │
        │        │        │        ││FTL   │││DMA   │││        │       │       │
        │        │        │        ││L2P   │││engine│││        │       │       │
        │        │        │        ││map   │││(PRP→ │││        │       │       │
        │        │        │        │└──┬───┘││ NAND)│        │        │       │
        │        │        │        │   │    │└──┬───┘│        │       │       │
 ───────┼────────┼────────┼────────┼───┼────┼───┼────┼────────┼───────┼───────┤
        │        │        │        │   │    │   │    │        │       │       │
 NAND   │        │        │        │   │    │┌──────┐│        │       │ page  │
 Media  │        │        │        │   │    ││Chan0 │││        │       │ read/ │
        │        │        │        │   │    ││Chan1 │││        │       │ program│
        │        │        │        │   │    ││...   │││        │       │       │
        │        │        │        │   │    ││ChanK │││        │       │       │
        │        │        │        │   │    │└──────┘│        │       │       │
 ═══════╧════════╧════════╧════════╧═══╧════╧═══╧════╧════════╧═══════╧═══════╡

       ══ 점선: PCIe 경계 (호스트 ↔ SSD)
       ──▶ 레인 내부: 단계 진행 / 소유
       세로 화살표: 공간 간 상호작용 (DRAM↔BAR0, BAR0↔FW)
```

### 단계별 함수/객체 요약

| Phase | 함수 | 만들어지는 객체 |
|---|---|---|
| ① insmod | `nvme_init` → `pci_register_driver` | `pci_driver` 등록 |
| ② PCI probe | `nvme_probe` / `nvme_pci_alloc_dev` | `nvme_dev`, BAR0 ioremap |
| ③ Ctrl reset | `nvme_pci_enable` / `nvme_enable_ctrl` | CC/CSTS 설정 |
| ④ Admin Q | `nvme_pci_configure_admin_queue` | `nvme_queue[0]`, ASQ/ACQ ring |
| ⑤ I/O Q | `nvme_setup_io_queues` → `nvme_alloc_queue` × N + `CREATE_IO_SQ/CQ` | `nvme_queue[1..N]` |
| ⑥ blk-mq | `nvme_dev_add` → `blk_mq_alloc_tag_set` | `tag_set`, `hctx[]`, `tags[]` |
| ⑦ NS scan | `nvme_scan_work` → `nvme_alloc_ns` | `request_queue`, `/dev/nvme0n1` |

### 이 그림의 포인트
- **Block Layer와 NVMe Driver를 분리**: `.queue_rq = nvme_queue_rq` 바인딩(⑥ 말미)으로 연결. 같은 blk-mq 프레임워크를 SCSI/virtio도 쓰는 이유가 설명됨.
- **SSD 내부를 3 레인으로 분해**: BAR0(얼굴) / Controller FW(펌웨어) / NAND(미디어).
- **④·⑤ 단계의 세로 화살표 집중**: 드라이버가 DRAM에 SQ/CQ 버퍼 만듦 → 그 주소를 BAR0에 MMIO write → SSD FW가 DMA fetch. **호스트와 SSD의 "악수"** 구간.

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

### Kernel NVMe
- `linux/drivers/nvme/host/pci.c` — `nvme_probe`, `nvme_pci_alloc_dev`, `nvme_pci_configure_admin_queue`, `nvme_setup_io_queues`
- `linux/drivers/nvme/host/core.c` — `nvme_init_identify`, `nvme_dev_add`, `nvme_scan_work`
- `linux/block/blk-mq.c` — `blk_mq_alloc_tag_set`, `blk_mq_init_queue`

### BaM
- `bam/module/pci.c` — `libnvm_helper_entry`, `add_pci_dev`, `map_ioctl`, `mmap_registers`
- `bam/include/nvm_ctrl.h` / `bam/src/ctrl.c` — `nvm_ctrl_init`, `nvm_aq_create`
- `bam/include/ctrl.h` — `Controller::Controller`
- `bam/include/queue.h` — `QueuePair::QueuePair`
- `bam/include/page_cache.h` — `page_cache_t`, `range_t`, `array_t`, `bam_ptr`
