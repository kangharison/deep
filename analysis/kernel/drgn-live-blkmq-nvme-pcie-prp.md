# drgn 으로 라이브 커널 I/O 스택 관찰 — blk-mq / NVMe / PCIe / PRP payload 타당성 분석

> **출처/참고**
> - drgn 공식 문서: https://drgn.readthedocs.io/
> - drgn GitHub (Omar Sandoval, Meta): https://github.com/osandov/drgn
> - 커널 문서 미러: [[drgn]] (`deep/notes/kernel/docs-kernel-org/bpf/drgn.md`)
> - 실행 스크립트: `deep/scripts/drgn/` (이 문서와 1:1 대응, `01~04`)
> - drgn 코드베이스 분석: [[drgn-codebase-overview]] (= `analysis/drgn/01-overview.md`) · [[03-debuginfo-dwarf]] · [[06-linux-kernel]]
> - 관련 분석: [[nvme-tagset-blkmq-flow]] · [[nvme-queue-init-request-queue-tagset-hctx]] · [[io-lifecycle-no-scheduler]] · [[io_submit-to-nvme_queue_rq-flow]] · [[nvme-driver]] · [[kdump-custom-kernel]]
>
> **작성 환경 실측 일자**: 2026-06-29 / 커널 `6.17.0-35-generic` (Ubuntu 24.04 HWE, x86_64)

---

## 0. 한 줄 결론

> **네 가지(blk-mq · NVMe 드라이버 · PCIe · PRP payload) 모두 drgn 으로 실시간 관찰 가능하다.**
> `/proc/kcore` + IOMMU 미사용이 갖춰져 있어 **PRP가 가리키는 실제 DMA 데이터 바이트까지** 읽을 수 있다.
> 단 (a) `/proc/kcore` 접근에 **root** 필요, (b) 디바이스 **MMIO 레지스터(도어벨/CC/CSTS)** 는 읽을 수 없음(호스트 RAM이 아니라서), (c) **in-flight 요청은 부하를 깔아야 잡힘**, (d) 라이브 메모리는 **race** 가능.
>
> > ⚠️ **2026-06-29 정정 (drgn 소스 분석 후)**: 본 문서 초판은 "BTF로 충분 / debuginfod 자동 다운로드"라고 적었으나 **둘 다 이 우분투 커널엔 틀렸다.** drgn은 **DWARF만** 타입 소스로 쓰며 BTF(`/sys/kernel/btf/*`)는 읽지 않는다. 또 drgn은 `is_fedora_kernel()` 검사로 **비-Fedora 커널엔 debuginfod를 끈다**. 따라서 이 머신에서 라이브 디버깅을 하려면 **`linux-image-$(uname -r)-dbgsym`(ddebs 저장소)** 를 반드시 설치해 vmlinux DWARF를 확보해야 한다. 근거·상세는 [[03-debuginfo-dwarf]] §2·§4, [[06-linux-kernel]] §8. (§1·§3 본문도 정정함.)

### 타당성 매트릭스

| 대상 | 무엇을 보나 | 가능? | 핵심 경로 | 제약 |
|------|------------|:----:|----------|------|
| **blk-mq** | in-flight `struct request`, op/섹터/길이/상태/태그/hctx | ✅ 완전 | `request_queue_busy_iter(q)` | in-flight 타이밍 |
| **NVMe 드라이버** | `nvme_dev`→`nvme_queue` SQ/CQ 링 내용·인덱스 | ✅ 완전 | `nvme_ns→ctrl→container_of→nvme_dev` | 도어벨/BAR 레지스터는 ❌(MMIO) |
| **PCIe** | `pci_dev` 벤더/클래스/BAR/MSI-X/cap/전력상태 | ✅ (캐시값) | `nvme_dev.dev→container_of→pci_dev` | 라이브 config space는 ❌(MMIO/포트) |
| **PRP 디스크립터** | `nvme_command.dptr.prp1/prp2`, PRP 리스트 | ✅ 완전 | `blk_mq_rq_to_pdu(rq)→nvme_iod.cmd` | SGL 경로면 별도 |
| **PRP 페이로드(실데이터)** | PRP가 가리키는 물리 페이지의 바이트 | ✅ (이 머신) | `prog.read(prp, n, physical=True)` | **IOMMU 켜지면 ❌**(IOVA) |

---

## 1. drgn 이 라이브 커널을 읽는 원리 — "세 다리 의자"

drgn 은 실행 중인 커널을 멈추지 않고 메모리를 **읽기 전용**으로 들여다보는 프로그래머블 디버거다(=Python 으로 커널 구조체를 객체처럼 탐색). 라이브 세션은 세 가지 정보원이 동시에 맞물려야 동작한다.

```
        ┌──────────────────────────────────────────────────────────────┐
        │                       drgn Program 객체                        │
        └───────────────┬──────────────┬──────────────┬─────────────────┘
                        │              │              │
              (1) 메모리 소스    (2) 타입 소스    (3) 심볼 소스
                        │              │              │
                 /proc/kcore     DWARF(필수)      /proc/kallsyms
              "주소→바이트"     "구조체 레이아웃"   "이름→주소"
                              ※ BTF는 안 씀
              (RAM 스냅샷)      (필드 오프셋)      (전역변수/함수)
                        │              │              │
                        └──────────────┴──────────────┘
                            이 셋이 합쳐져야 prog["init_task"].comm 같은
                            "이름으로 구조체 필드 접근"이 성립한다.
```

| 다리 | 무엇 | 이 머신에서 |
|------|------|------------|
| (1) 메모리 | `/proc/kcore` (ELF로 본 물리/가상 RAM) | `CONFIG_PROC_KCORE=y` ✅, root 필요 |
| (2) 타입 | **DWARF**(vmlinux 디버그심볼) — drgn은 BTF 미사용 | ❗ dbgsym 미설치 → **설치 필요**([[03-debuginfo-dwarf]]) |
| (3) 심볼 | `/proc/kallsyms` | `CONFIG_KALLSYMS=y` ✅ (root 시 실주소) |

`gdb`/`crash` 대비 차별점: **명령어가 아니라 Python** 이라 `for rq in busy_iter(q): ...` 처럼 자료구조를 순회·필터·집계하는 임의 로직을 즉석에서 짤 수 있다. 커널을 멈추지 않으므로(=non-intrusive) 운영 중 시스템에도 붙는다.

---

## 2. 이 머신 환경 점검 결과 (실측)

| 항목 | 결과 | 의미 |
|------|------|------|
| drgn 설치 | ❌ 미설치 → pip wheel로 설치 가능 | `pip install drgn` (0.2.0, manylinux, **컴파일 불필요**) |
| 커널 | `6.17.0-35-generic` x86_64 | 최신, 아래 nvme_iod 레이아웃 주의 |
| **DWARF 디버그심볼** | ❌ **미설치** (`/usr/lib/debug/.../vmlinux` 없음) | ❗ **drgn의 필수 전제** — `linux-image-$(uname -r)-dbgsym` 설치 필요(ddebs) |
| `CONFIG_DEBUG_INFO_BTF` | `=y`, `BTF_MODULES=y`, `/sys/kernel/btf/*` 존재 | ⚠️ **drgn은 BTF를 안 씀** — 이 BTF는 drgn에 무의미(bpftool/pahole용) |
| `DEBUGINFOD_URLS` | `https://debuginfod.ubuntu.com` 설정됨 | ⚠️ drgn은 `is_fedora_kernel()` 검사로 **비-Fedora 커널엔 debuginfod 비활성** → 이 우분투 커널엔 무용 |
| `/proc/kcore` | 존재 (`root` 전용) | 라이브 메모리 소스 OK, **sudo 필요** |
| `kptr_restrict` | `1` | 비-root는 kallsyms 주소가 0 → root로 실행해야 함 |
| **IOMMU** | `/sys/class/iommu/` **비어있음**, nvme0에 `iommu_group` 없음 | **DMA 주소 = 물리 주소** → PRP 직접 읽기 가능 ★ |
| NVMe 디바이스 | `/dev/nvme0n1` = `ORCL-VBOX-NVME-VER12` (VirtualBox 에뮬) | 실제 드라이버 경로 그대로 동작(에뮬이어도 PRP 빌드됨) |
| PCIe 위치 | `0000:00:0e.0` | `class 0x010802`(NVMe) |
| blk-mq HW 큐 | **1개**(hctx0), `nr_requests=256` | 단일 I/O 큐(에뮬), `nvme0` `queue_count=2`(admin+io) |
| I/O 스케줄러 | `mq-deadline` (활성) | busy_iter 기본 태그가 `sched`가 됨(§5 주의) |
| `nvme.sgl_threshold` | `32768` (32 KB) | I/O가 32KB 미만이면 PRP, 이상이면 SGL 가능 |

> ★ **가장 운 좋은 조건**: IOMMU 미사용. VirtualBox는 기본적으로 vIOMMU를 노출하지 않아, NVMe 드라이버가 `dma_map_*` 으로 얻는 주소가 곧 게스트 물리주소다. 그래서 PRP 엔트리를 `physical=True` 로 그대로 읽으면 실제 데이터가 나온다. (베어메탈에서 `intel_iommu=on`/`amd_iommu=on`이면 PRP는 IOVA가 되어 이 직접 읽기가 깨진다 → §8 참조.)

---

## 3. 타입 소스 결정 — drgn은 DWARF만 쓴다 (BTF·debuginfod 함정)

> **초판 정정**: 이 절은 원래 "BTF로 충분"이라고 적었으나 **drgn 소스 분석 결과 틀렸다.** drgn은 타입 정보를 **DWARF**(`.debug_info`/`.debug_abbrev`/...)에서만 얻고, **BTF(`.BTF` 섹션)는 로드하지 않는다.** 코드 근거: drgn이 인덱싱하는 ELF 섹션 목록(`libdrgn/build-aux/gen_elf_sections.py`)에 `.BTF`가 없고, libdrgn 전체에 `BTF` 심볼이 0건이다. (BTF가 등장하는 곳은 `drgn/helpers/linux/bpf.py` — 타깃의 `struct btf`를 *데이터로* 순회하는 헬퍼일 뿐, drgn 자신의 타입 소스가 아니다.) 상세: [[03-debuginfo-dwarf]] §2.

drgn 라이브 디버깅의 유일한 실질 관문은 "**DWARF를 어디서 얻나**"다. 세 경로가 있는데, **이 우분투 머신에서 실제로 통하는 건 dbgsym 설치뿐**이다.

| 방법 | 준비물 | 이 우분투 6.17 머신 | 비고 |
|------|--------|--------------------|------|
| ~~BTF~~ | `/sys/kernel/btf/*` | ❌ **불가** | drgn은 BTF를 타입 소스로 쓰지 않음 |
| debuginfod 자동 다운로드 | 인터넷 + `DEBUGINFOD_URLS` | ❌ **불가** | drgn `is_fedora_kernel()`이 비-Fedora 커널엔 debuginfod 끔([[06-linux-kernel]] §8) |
| **dbgsym 패키지** | `linux-image-*-dbgsym` (ddebs) | ✅ **유일한 길** | 수백 MB, ddebs 저장소 추가 필요 |

### 이 머신에서 DWARF 확보 절차 (실행 전 필수)

```bash
# 1) ddebs(우분투 디버그심볼) 저장소 추가
sudo apt install -y ubuntu-dbgsym-keyring
echo "deb http://ddebs.ubuntu.com $(lsb_release -cs) main restricted universe multiverse
deb http://ddebs.ubuntu.com $(lsb_release -cs)-updates main restricted universe multiverse" \
  | sudo tee /etc/apt/sources.list.d/ddebs.list
sudo apt update

# 2) 실행 중인 커널의 vmlinux DWARF 설치 (수백 MB)
sudo apt install -y linux-image-$(uname -r)-dbgsym
#  → /usr/lib/debug/boot/vmlinux-$(uname -r) 생성. drgn이 자동으로 찾는다.
```

> 정밀 검증을 못 하는 환경(인터넷/디스크 제약)이라면, **이 머신과 동일한 커널을 KVM/QEMU로 띄우고 `--qemu`로 붙거나**, dbgsym이 이미 있는 Fedora 게스트에서 실습하는 것도 대안이다([[07-stack-arch]] §6 QMP).

---

## 4. 설치 & 실행

```bash
# 설치 (PEP 668 회피용 venv 권장)
python3 -m venv ~/.venv-drgn && ~/.venv-drgn/bin/pip install drgn   # → 0.2.0, wheel
DRGN=~/.venv-drgn/bin/drgn

# 라이브 세션은 root 필요(/proc/kcore). -E 로 DEBUGINFOD_URLS 보존.
sudo -E $DRGN deep/scripts/drgn/00_env_check.py
```

drgn CLI로 스크립트를 실행하면 `prog`(현재 커널) 전역과 `drgn.helpers.linux.*` 헬퍼가 자동 주입된다. REPL(`sudo -E drgn`)로 들어가 한 줄씩 탐색도 가능.

**in-flight를 잡으려면 부하 필수**(에뮬은 완료가 빨라 한순간엔 큐가 빔):
```bash
sudo fio --name=load --filename=/dev/nvme0n1 --rw=randread --bs=16k \
         --iodepth=64 --direct=1 --ioengine=libaio --runtime=60 --time_based
# bs<32KB(=sgl_threshold) + >4KB → PRP 경로 + PRP 리스트까지 생성됨
```

---

## 5. blk-mq 관찰 — `01_blkmq_inflight.py`

### 5.1 자료구조 경로

```
gendisk(nvme0n1)
   └─ .queue ───────────► struct request_queue
                              ├─ .tag_set ─► blk_mq_tag_set (.queue_depth, .nr_hw_queues)
                              ├─ .elevator ─► (mq-deadline)
                              └─ HW ctx들(hctx) ─► struct blk_mq_tags
                                     ├─ bitmap_tags (sbitmap)  : 비트1 = 드라이버태그 할당됨(=디스패치)
                                     ├─ breserved_tags         : 예약 태그
                                     └─ static_rqs[tag] ───────► struct request   ◄── 관찰 대상
```

drgn 헬퍼 `request_queue_busy_iter(q, tags)` 가 위 sbitmap 워킹을 대신한다. 반환은 `struct request *` 의 **이터레이터**.

| `tags` 값 | 의미 | 언제 잡히나 |
|-----------|------|------------|
| `"driver"` | 드라이버 태그 할당됨 = HW 디스패치되어 디바이스로 내려간 요청 | `nvme_queue_rq` 통과 후(=NVMe 명령 빌드됨) |
| `"sched"` | 스케줄러 태그 할당됨 = 제출됐으나 elevator에 머물 수도 | `blk_mq_submit_bio` 이후 |
| 기본값 | 스케줄러 `none`이면 driver, 아니면 sched | nvme0n1은 mq-deadline → **기본 `sched`** |

> **함의**: PRP/NVMe 명령을 보려면 반드시 `tags="driver"` 로 순회해야 한다. `sched` 단계 요청은 아직 `nvme_queue_rq` 를 안 거쳐 PRP가 비어 있을 수 있다. (그래서 `04_prp_payload.py` 는 `"driver"` 를 쓴다.)

### 5.2 각 `struct request` 에서 읽는 필드 (6.17)

| 필드 | 헬퍼/접근 | 의미 |
|------|----------|------|
| op | `req_op(rq)` | REQ_OP_READ/WRITE/FLUSH... (`cmd_flags` 하위 비트) |
| 섹터 | `blk_rq_pos(rq)` = `__sector` | 512B 단위 시작 LBA |
| 길이 | `blk_rq_bytes(rq)` = `__data_len` | 남은 전송 바이트 |
| 상태 | `rq.state` | `MQ_RQ_IDLE=0 / IN_FLIGHT=1 / COMPLETE=2` |
| 태그 | `rq.tag` / `rq.internal_tag` | 드라이버 태그(=NVMe command id) / 스케줄러 태그 |
| HW큐 | `rq.mq_hctx.queue_num` | 매핑된 hctx 번호(=NVMe I/O 큐) |

→ 상세 흐름은 [[nvme-tagset-blkmq-flow]] · [[io-lifecycle-no-scheduler]] 와 교차 참조.

---

## 6. NVMe 드라이버 관찰 — `02_nvme_queues.py`

### 6.1 nvme_dev 복원 경로 (실측 확인)

```
gendisk(nvme0n1).private_data (void*)
     └─► struct nvme_ns *
            └─ .ctrl ─► struct nvme_ctrl *
                  └─ container_of(ctrl, struct nvme_dev, ctrl) ─► struct nvme_dev *
                        ├─ .queues ─► struct nvme_queue *  (배열: [0]=Admin, [1..]=I/O)
                        ├─ .online_queues / .nr_allocated_queues
                        ├─ .bar  (ioremap된 BAR0 가상주소 — MMIO)
                        └─ .dbs  (도어벨 베이스 — MMIO)
```

`nvme_dev` 는 `nvme_ctrl` 을 **멤버로 포함**하므로 `container_of` 로 역복원한다(드라이버 자체가 `to_nvme_dev()` 매크로로 쓰는 방식과 동일).

### 6.2 SQ/CQ 링 — 무엇이 읽히고 무엇이 안 읽히나

```
   struct nvme_queue {
       void *sq_cmds;                 ◄ SQ 링 (dma_alloc_coherent, 호스트 RAM)  ─ 읽기 O
       struct nvme_completion *cqes;  ◄ CQ 링 (dma_alloc_coherent, 호스트 RAM)  ─ 읽기 O
       dma_addr_t sq_dma_addr;        ◄ SQ 물리주소 (IOMMU 없음→물리=DMA)
       u16 sq_tail, cq_head;          ◄ 드라이버가 RAM에 들고 있는 링 인덱스      ─ 읽기 O
       u32 __iomem *q_db;             ◄ 도어벨 레지스터 (BAR0 MMIO)              ─ 읽기 X ★
   };
```

★ **핵심 한계**: `q_db`(도어벨), `nvme_dev.bar`(컨트롤러 레지스터 CAP/CC/CSTS/AQA)는 **디바이스 MMIO** 다. `/proc/kcore` 는 RAM 매핑이라 MMIO 주소를 읽으면 의미 없는 값이거나 FaultError 다. 대신 **드라이버가 RAM에 캐싱한 미러**(`sq_tail`, `cq_head`, shadow doorbell `dbbuf_*`)는 읽힌다. 즉 "디바이스 레지스터 자체"는 못 봐도 "드라이버가 본 마지막 값"은 본다.

> SQ 엔트리(`sq_cmds[i]`)는 `dma_alloc_coherent` 로 잡힌 **호스트 RAM** 이라 가상주소로 그냥 읽힌다 → 큐에 올라간 NVMe 명령들을 그대로 덤프 가능. SPDK가 유저스페이스에서 같은 SQ/CQ 링을 직접 다루는 것과 대비해 보면 좋다([[spdk]] · [[sq-mapping-comparison]]).

---

## 7. PCIe 관찰 — `03_pcie.py`

### 7.1 경로와 읽히는 값

```
nvme_dev.dev (struct device*) ──container_of──► struct pci_dev *
```

| 읽히는 것 (커널이 RAM에 캐싱) | 필드 |
|------|------|
| 벤더/디바이스/서브시스템 ID | `pdev.vendor/.device/.subsystem_vendor/.subsystem_device` |
| 클래스 코드 | `pdev["class"]` (0x010802 = NVMe) |
| BAR 영역 | `pdev.resource[0..5].{start,end,flags}` (BAR0=컨트롤러 레지스터 MMIO 윈도우) |
| 인터럽트 | `pdev.irq`, `pdev.msi_enabled`, `pdev.msix_enabled` |
| Capability 오프셋 | `pdev.pcie_cap`, `pdev.msix_cap`, `pdev.msi_cap` |
| 전력상태 | `pdev.current_state` (PCI_D0=가동) |

### 7.2 한계

라이브 **PCIe config space**(링크 속도 LNKSTA, 실제 BAR 내용)는 config 사이클(MMCONFIG MMIO 또는 0xCF8/0xCFC 포트)로만 접근 → `/proc/kcore` 로는 못 읽는다. drgn으로 보는 건 **커널이 enumeration 때 읽어 캐싱한 메타데이터**다. ASPM/링크 상태 정밀 분석은 별도 트레이싱이 필요(이 레포의 `linux-study/annotate_aspm.py` 와 연결).

---

## 8. PRP payload 관찰 — `04_prp_payload.py` (정점)

### 8.1 PRP 개념 1분 요약

NVMe 데이터 버퍼는 PRP(Physical Region Page) 엔트리들 = 4KB 페이지 물리주소로 기술된다.

```
 nvme_command.dptr:
   PRP1 ── 첫 데이터 페이지 물리주소(페이지 내 오프셋 가능)
   PRP2 ── (A) 데이터 ≤2페이지: 두 번째 페이지 주소
           (B) 3페이지 이상   : 'PRP 리스트' 페이지(물리주소) — __le64 엔트리 배열
                                 512엔트리(2MB) 초과 시 마지막 엔트리가 다음 리스트로 체이닝
```

### 8.2 6.17 `nvme_iod` 레이아웃 — **구버전과 다름(중요)**

```c
struct nvme_iod {                 // drivers/nvme/host/pci.c:505 (커널 6.17)
    struct nvme_request req;
    struct nvme_command cmd;      // ◄ 드라이버가 빌드한 명령(dptr.prp1/prp2 여기)
    u8  flags;                    // enum nvme_iod_flags: IOD_SMALL_DESCRIPTOR(1<<1) 등
    u8  nr_descriptors;           // 사용한 디스크립터 페이지 수
    size_t total_len;             // 총 전송 길이
    struct dma_iova_state dma_state;
    void *descriptors[5];         // ◄ descriptors[0] = PRP 리스트 페이지(커널 가상주소)
    struct nvme_dma_vec *dma_vecs;
    ...
};
```

> ⚠️ **웹 튜토리얼 주의**: 대부분의 drgn-for-nvme 예제는 `iod->first_dma`, `iod->sgt`, `iod->use_sgl`, `iod->list[]` 를 쓴다 — 이건 6.15 전후 DMA API 재작업으로 **삭제된 구 레이아웃**이다. 6.17에서는 `iod->cmd.common.dptr.prp1/prp2`, `iod->descriptors[]`, `iod->flags`(PRP/SGL은 명령의 PSDT 비트로 판별), `iod->total_len` 을 써야 한다. 이 차이를 모르면 스크립트가 LookupError로 죽는다.

### 8.3 해독 + 페이로드 읽기 알고리즘 (스크립트 구현)

```
rq (driver 태그 in-flight)
  └─ blk_mq_rq_to_pdu(rq) ─► struct nvme_iod *
        cmd   = iod.cmd
        psdt  = (cmd.common.flags >> 6) & 3      # 0=PRP, 그외=SGL → SGL이면 스킵
        prp1  = cmd.common.dptr.prp1
        prp2  = cmd.common.dptr.prp2
        len   = iod.total_len

  offset = prp1 & 0xFFF
  page0  : prog.read(prp1, 32, physical=True)            # ★ 실제 DMA 데이터 바이트
  if len <= 4096-offset:           # case A: 1페이지 → 끝
  elif 한 페이지만 더 필요:         # case B: prp2가 곧 2번째 데이터 페이지
       prog.read(prp2, 32, physical=True)
  else:                            # case C: prp2 = PRP 리스트
       prp_list = cast('__le64 *', iod.descriptors[0])   # 커널 가상으로 같은 페이지 접근
       for i in range(n_more_pages):
           prog.read(prp_list[i], 16, physical=True)      # 각 데이터 페이지
```

`prog.read(addr, n, physical=True)` 가 **이 분석의 심장**이다. IOMMU가 없어 `prp_entry == 물리주소` 이므로, 디바이스가 DMA할 바로 그 페이지를 읽는다. 읽기 결과는 ATA/NVMe가 막 채운(read) 또는 채울(write) 실제 데이터(예: 파일시스템 메타데이터, 사용자 버퍼 내용).

### 8.4 IOMMU가 켜진 환경이라면 (이 머신은 해당 없음)

| 조건 | PRP 엔트리의 정체 | 페이로드 읽는 법 |
|------|------------------|-----------------|
| **IOMMU off** (이 머신) | = 물리주소 | `prog.read(prp, n, physical=True)` 직접 |
| IOMMU on (`intel_iommu=on` 등) | = **IOVA**(DMA 주소) | (a) IOMMU 페이지테이블 워킹으로 IOVA→PA, 또는 (b) `rq.bio→bio_vec→page` 로 **소스 페이지**를 가상읽기 |

(b) 경로는 IOMMU 유무와 무관하게 항상 동작하는 대안이다: `struct request.bio` 체인의 `bio_vec.bv_page` 들이 원본 데이터 페이지이므로, `page_to_virt(page)` 가상주소로 읽으면 같은 바이트를 얻는다. PRP는 "디바이스가 보는 주소", bio_vec는 "커널이 보는 같은 메모리" — 둘을 대조하면 DMA 매핑이 올바른지까지 검증 가능하다. (향후 확장 스크립트 후보.)

### 8.5 SGL 분기

`nvme.sgl_threshold=32768` 이라 32KB 이상 I/O는 SGL을 탈 수 있다(디바이스가 SGL 지원 광고 시). SGL이면 `cmd.common.flags` 의 PSDT 비트가 0이 아니고, 데이터는 `nvme_sgl_desc` 로 기술된다. 스크립트는 PSDT를 보고 SGL이면 PRP 해독을 건너뛴다. **PRP를 확실히 보려면 `bs<32KB`** 로 부하를 준다.

---

## 9. 한계·주의사항 (정직한 경계)

| # | 한계 | 상세 | 완화책 |
|---|------|------|--------|
| 1 | **root 필요** | `/proc/kcore`, 실 kallsyms 주소 | `sudo -E drgn` |
| 2 | **MMIO 못 읽음** | 도어벨/CC/CSTS/BAR 내용/PCIe config | 드라이버 RAM 미러로 갈음, 레지스터는 트레이싱 |
| 3 | **race(비일관 스냅샷)** | 라이브 메모리는 읽는 중 변함 | 짧게 읽기, FaultError 처리, 반복 샘플 |
| 4 | **in-flight 타이밍** | 에뮬 완료가 빨라 큐가 자주 빔 | fio/dd로 지속 부하(`iodepth↑`) |
| 5 | **IOMMU 시 PRP=IOVA** | 직접 물리읽기 불가 | bio_vec 경로 또는 IOMMU PT 워킹(§8.4) |
| 6 | **SGL 분기** | 큰 I/O는 PRP 아님 | `bs<sgl_threshold` |
| 7 | **버전 레이아웃** | nvme_iod 등 구조체가 커널마다 다름 | 본 스크립트는 **6.17 전용**, 다른 커널은 필드 재확인 |
| 8 | **에뮬 한계** | VirtualBox NVMe는 일부 기능(CMB/shadow doorbell) 미구현 | 드라이버 경로 학습엔 무방, 성능수치는 무의미 |

---

## 10. 검증 현황 (무엇을 실측했고, 사용자가 무엇을 실행해야 하나)

**제가(분석자) 이 머신에서 직접 확인한 것**
- ✅ drgn 0.2.0 venv 설치 성공(wheel, 컴파일 불필요), 헬퍼 심볼(`request_queue_busy_iter`/`blk_mq_rq_to_pdu`/`for_each_pci_dev`/`Program.read(physical=)`) 존재 실측
- ✅ `/proc/kcore`, `CONFIG_*` 플래그, IOMMU 미사용, NVMe/PCIe/blk-mq sysfs 토폴로지 실측
- ✅ (정정) drgn 소스 분석으로 **DWARF 전용·BTF 미사용·비-Fedora debuginfod 비활성** 확정 → 이 머신은 **dbgsym 설치가 전제**임을 명확화([[03-debuginfo-dwarf]] · [[06-linux-kernel]])
- ⏳ (미완) dbgsym 미설치 상태라 **실제 라이브 세션은 아직 미검증** — §3 절차로 dbgsym 설치 후 `00_env_check.py`부터 확인 필요
- ✅ 커널 6.17 소스에서 `nvme_iod`/`nvme_queue`/`nvme_dev`/`struct request` **정확한 필드명** 확인 → 스크립트에 반영
- ✅ 스크립트 5종 문법 컴파일 + 헬퍼 임포트 통과

**사용자가 실행해야 완결되는 것** (root sudo가 무암호가 아니라 분석자가 라이브 실행 불가)
- ⏳ `sudo -E drgn 00~04` 실제 실행 → in-flight 캡처 및 PRP 페이로드 hexdump 확인
- 실행 절차는 §4 / `deep/scripts/drgn/README.md` 참고. 결과가 기대와 다르면(특히 04에서 필드 LookupError) 알려주면 그 커널 빌드에 맞춰 필드를 정정한다.

---

## 11. 다음 단계 아이디어

1. **bio_vec ↔ PRP 대조 스크립트**: 같은 페이지를 "커널 가상(bio)" vs "디바이스 물리(PRP)" 두 경로로 읽어 DMA 매핑 정합성 검증(§8.4). IOMMU 환경 이식성도 확보.
2. **완료 경로 관찰**: CQ 링(`cqes[]`)의 phase 비트/status를 읽어 완료 대기 중인 명령 추적 → [[io-lifecycle-no-scheduler]] 의 완료 단계와 결합.
3. **SPDK 대비**: 동일 SQ/CQ 구조를 SPDK 유저스페이스가 다루는 모습과 1:1 비교([[sq-mapping-comparison]] · [[nvme-eg-fdp-kernel-vs-spdk]]).
4. **NVM-Set/SQ 격리 검증**: 본인이 작업 중인 per-set SQ 격리([[nvme-namespace-sq-isolation-driver]])가 런타임에 정말 큐를 분리하는지 drgn으로 `nvme_dev.queues[]` 매핑 실측.
