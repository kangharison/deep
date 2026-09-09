# QEMU 실측 추적: SSD가 "호스트가 정해준 MSI-X 주소"로 인터럽트를 쏘고 Local APIC이 CPU에 꽂기까지

> 실측 환경: QEMU 8.2.2 (TCG, `-smp 8`) + Linux 6.1.4 게스트 + 에뮬레이션 NVMe(`1b36:0010`)
> 추적 도구: QEMU trace backend(log), QMP `human-monitor-command`(`info lapic`), 게스트 gdbstub(`-s`) + `gdb vmlinux`,
> `drgn --qemu`(QMP 라이브 커널), busybox `devmem`(게스트에서 BAR/LAPIC 물리주소 직접 읽기/쓰기)
> 대상 코드: `qemu-8.2.2/hw/nvme/ctrl.c`, `hw/pci/msix.c`, `hw/pci/msi.c`, `hw/intc/apic.c`,
> `linux-6.1.4/drivers/nvme/host/pci.c`, `drivers/pci/msi/msi.c`, `arch/x86/kernel/apic/*`, `kernel/irq/chip.c`

> **처음 읽는다면 바로 아래 「먼저 읽기 — 초심자용 길잡이」부터 보세요.**
> 거기에 비유·용어집과, 이 문서에서 쓰는 **로그 출처 태그(`[driver]` / `[device]` / `[qemu]`)** 규칙이 있습니다.

이 문서는 "SSD가 인터럽트를 쏜다"는 한 줄을, **실제로 관측한 주소·데이터·비트 값**으로 끝까지 분해한다.
모든 숫자는 추론이 아니라 이 실험에서 찍은 값이며, 같은 값을 서로 다른 3~4개 관측점(장치 레지스터 /
커널 자료구조 / QEMU 트레이스 / LAPIC 상태)에서 교차 검증했다.

**한 줄 요약**: NVMe 큐 6번의 완료 인터럽트는 → MSI-X 테이블 엔트리 6번에 적힌
**주소 `0xFEE20004`에 데이터 `0x21`을 쓰는 4바이트 메모리 쓰기 한 번**이고,
그 주소의 `[19:12]`=`0x20`이 CPU5의 LDR과 매칭되어 CPU5의 LAPIC IRR 33번 비트가 서고,
CPU5가 IDT 33번(`irq_entries_start+8`)으로 점프해 `nvme_irq()`를 실행한다.

---

## 먼저 읽기 — 초심자용 길잡이 (5분)

### 이 문서가 답하는 질문

"SSD가 인터럽트를 건다"는 문장은 실제로는 무슨 일인가?

결론부터 말하면 — **SSD는 "인터럽트 신호"라는 특별한 걸 보내지 않는다.
호스트가 미리 알려준 주소에 4바이트를 쓸 뿐이다.** 그 주소가 하필
CPU 안의 인터럽트 접수 창구(Local APIC)가 차지하고 있는 주소라서,
그 평범한 메모리 쓰기가 인터럽트가 된다.

### 비유로 먼저

```
 ① 드라이버(리눅스 커널)
      "나한테 연락할 땐 이 번호로 걸어 — 5번 창구, 용건번호 33번"
      ← 이 메모를 SSD 안에 있는 수첩(MSI-X 테이블)에 적어 둔다
 ② SSD(장치)
      일이 끝나면 수첩을 펴서, 적힌 번호 그대로 전화를 건다
      (= 적힌 주소에 적힌 값을 쓴다. 해석은 안 한다. 그냥 받아 적은 대로)
 ③ Local APIC(CPU마다 하나씩 있는 교환대)
      전화를 받아 "5번 창구 손님, 용건 33번" 이라고 CPU5를 부른다
 ④ CPU5
      하던 일(idle)을 멈추고, 용건번호 33번 담당자(IDT[33])에게 넘긴다
 ⑤ 커널
      33번 → nvme0q6 → nvme_irq() 실행 → CQ를 훑어 완료 처리
```

핵심은 **수첩에 적는 사람(드라이버)과 수첩을 읽어 거는 사람(SSD)이 다르다**는 것이다.
그래서 이 문서는 계속 "커널이 적으려던 값"과 "장치에 실제로 적힌 값"을 나란히 놓고 비교한다.

### ★ 로그 출처 표기 규칙 (중요)

이 문서에는 **서로 다른 세 군데**에서 뜬 값이 섞여 있다. 헷갈리지 않도록
데이터 블록마다 아래 태그를 붙였다.

| 태그 | 누구의 값인가 | 어떻게 얻었나 | 실물 하드웨어에서도 볼 수 있나 |
|------|--------------|--------------|------------------------------|
| **`[driver]`** | **리눅스 커널이 자기 메모리에 들고 있는 장부** | drgn, 게스트 gdb, `/proc/interrupts` | ✅ drgn/crash로 가능 |
| **`[device]`** | **장치(SSD) 안에 실제로 적혀 있는 값** | 게스트에서 `devmem`/sysfs `config`로 PCI config space·BAR를 읽음 | ✅ 진짜 SSD도 똑같이 읽힌다 |
| **`[qemu]`** | **에뮬레이터만 볼 수 있는 "실제로 벌어진 일"** | QEMU `-trace` 로그, `info lapic` | ❌ 실물에선 전용 장비가 필요 |

읽는 요령:

* `[driver]`와 `[device]`가 **같은 값이면** → 커널이 의도한 대로 장치에 잘 적혔다는 뜻.
  §4.4가 이 둘을 9줄 통째로 대조한 표다.
* `[qemu]`는 그 값이 **실제로 쓰였을 때 무슨 일이 벌어졌는지**를 보여준다.
  실물 하드웨어라면 못 보는 부분이라, 이 실험을 QEMU에서 하는 이유가 여기 있다.
* **소스코드 블록에는 태그 대신 파일 경로**를 적었다. 경로만 봐도 어느 쪽인지 알 수 있다:
  * `hw/nvme/…`, `hw/pci/…`, `hw/intc/…` → **QEMU** (에뮬레이션된 장치·APIC의 동작)
  * `drivers/…`, `arch/…`, `kernel/…`, `include/linux/…` → **리눅스 커널** (드라이버 동작)

QEMU 트레이스 줄에는 앞에 `<스레드ID>@<시각>:`이 붙는데, 이걸로 **"장치가 한 일"과
"특정 CPU가 한 일"을 더 잘게 구분**할 수 있다(§0.2의 매핑표). 중요한 구간에서는
`[qemu·SSD]`, `[qemu·vCPU5]` 처럼 한 단계 더 쪼개서 표시했다.

### 최소 용어집

| 용어 | 한 줄 설명 | 이 문서의 실측값 |
|------|-----------|-----------------|
| MMIO | 장치 레지스터를 메모리 주소처럼 읽고 쓰는 방식 | 도어벨, MSI-X 테이블 접근이 전부 MMIO |
| BAR | 장치가 요구한 MMIO 주소 창. 위치는 펌웨어/커널이 정해준다 | BAR0 = `0xfebf0000`, 크기 16KB |
| SQ / CQ | Submission/Completion Queue. **호스트 메모리**에 있는 링버퍼 | SQ6=`0x4fb0000`, CQ6=`0x60bc000` |
| doorbell | "큐에 새 항목 넣었다"고 장치에 알리는 MMIO 쓰기 | BAR0+`0x1030`(SQ6), +`0x1034`(CQ6) |
| CQE | 완료 항목 하나(16바이트) | `{sq_id=6, status=0x1}` |
| MSI-X | 인터럽트를 **메모리 쓰기**로 보내는 PCIe 방식 | 이 문서의 주제 |
| MSI-X 테이블 | **장치 안에 있는 배열**. 엔트리 = (주소 8B, 데이터 4B, 제어 4B) | BAR0+`0x2000`, 65칸 중 9칸 사용 |
| PBA | Pending Bit Array. 마스크된 인터럽트가 밀려 있음을 표시하는 비트 배열 | BAR0+`0x3000` |
| 벡터(vector) | **CPU가 아는 인터럽트 번호(0~255)**. IDT의 인덱스 | 33 |
| IRQ 번호 | **리눅스가 붙인 논리 번호**. 벡터와 전혀 다른 값 | 30 (`nvme0q6`) |
| Local APIC | CPU마다 하나씩 있는 인터럽트 접수 창구 | 주소창 `0xFEE00000` |
| IRR / ISR | "접수됨" / "처리중"을 나타내는 256비트 배열 | IRR[33]=1 → ISR[33]=1 |
| LDR | Local APIC의 논리 주소. flat 모드에선 CPU n → `1<<n` | CPU5 = `0x20` |
| EOI | End Of Interrupt. "다 처리했다"고 APIC에 알리는 쓰기 | `0xFEE000B0`에 0 쓰기 |
| IDT | 벡터 → 진입 코드 주소 표 | IDT[33] = `irq_entries_start+8` |

> **가장 헷갈리는 두 가지**
> 1. **벡터 33과 IRQ 30은 다른 번호다.** 벡터는 CPU/하드웨어가 쓰는 번호,
>    IRQ는 리눅스가 내부적으로 붙인 번호다. 커널이 `vector_irq[33] → irq 30`으로 변환한다.
> 2. **MSI-X 테이블은 장치 안에 있다.** 커널 메모리가 아니다. 커널은 MMIO로 남의 집(장치)
>    수첩에 자기 연락처를 적어두는 것이다. 그래서 `devmem`으로 BAR를 읽으면 그 수첩이 보인다.

### 어디부터 읽을까

| 목적 | 읽는 순서 |
|------|-----------|
| 10분 만에 감 잡기 | 이 길잡이 → §1(한 장 요약) → §11(타임라인) → §16(직답) |
| 원리를 이해하기 | §2(장치가 수첩을 어디 두나) → §4(커널이 뭘 적나) → §5(장치가 어떻게 쏘나) → §6~§8(APIC·CPU가 어떻게 받나) |
| 직접 해보기 | §14(재현 스크립트) |
| "진짜 그런가?" 검증 | §9(마스킹 실험) → §10(CPU가 직접 쏴보기) |

---

## 0. 실험 환경과 재현 방법

### 0.1 게스트 기동

**실행 명령 — 호스트에서 게스트를 띄운다**

```bash
qemu-8.2.2/build/qemu-system-x86_64 \
  -accel tcg -smp 8 -m 2048 \
  -kernel linux-6.1.4/arch/x86/boot/bzImage -initrd irfs-msix.cpio.gz \
  -append 'console=ttyS0 nokaslr no_hash_pointers panic=-1' \
  -drive file=nvme.img,if=none,id=nvm0,format=raw \
  -device nvme,drive=nvm0,serial=deadbeef,id=nvme0 \
  -device vmcoreinfo \
  -trace events=trace-events.txt -D trace.log \
  -nographic -no-reboot -s -msg timestamp=on \
  -qmp unix:qmp.sock,server,nowait
```

| 옵션 | 이 실험에서 담당하는 역할 |
|------|--------------------------|
| `-smp 8` | I/O 큐 8개(+admin 1개) = MSI-X 벡터 9개가 생기도록. 벡터↔CPU 매핑을 눈으로 보려면 CPU가 여러 개여야 한다 |
| `-trace events=... -D ...` | `pci_nvme_irq_msix`, `apic_deliver_irq`, `apic_mem_writel` 등 QEMU 내부 이벤트 로그 |
| `-msg timestamp=on` | 트레이스 각 줄에 `<host-tid>@<epoch.usec>:` 접두사가 붙는다. **tid로 어느 vCPU 스레드인지 구분**할 수 있다 |
| `-s` | 게스트 gdbstub(:1234). 인터럽트 처리 도중에 게스트를 **정지**시켜 LAPIC ISR을 찍기 위해 필수 |
| `-qmp unix:...` | `info lapic`(HMP 브리지), `trace-event-set-state`(런타임 트레이스 on/off), `drgn --qemu` |
| `-device vmcoreinfo` | `drgn --qemu`가 VMCOREINFO를 얻어 vmlinux를 검증하는 데 필요 |

`-msg timestamp=on` 없이 띄우면 트레이스 줄에 시간/스레드가 안 붙는다. 인터럽트 경로는
**어느 스레드가 무엇을 했는지**가 핵심이므로 반드시 켠다.

### 0.2 host thread ↔ vCPU 매핑 (QMP `query-cpus-fast` 실측)

**`[qemu]`** QMP `query-cpus-fast` — vCPU와 호스트 스레드 대응

```
vCPU 0 thread-id 2703859      vCPU 4 thread-id 2703863
vCPU 1 thread-id 2703860      vCPU 5 thread-id 2703864
vCPU 2 thread-id 2703861      vCPU 6 thread-id 2703865
vCPU 3 thread-id 2703862      vCPU 7 thread-id 2703866
(그 외 tid 2703852 = QEMU main thread = 장치 에뮬레이션/블록 완료 BH가 도는 스레드)
```

이 표가 있어야 트레이스 한 줄만 보고 "이건 SSD가 한 일" / "이건 CPU5가 한 일"을 구분할 수 있다.
**tid 2703852로 찍힌 이벤트 = 에뮬레이션된 SSD(장치)가 한 일**이다.

### 0.3 관측점 4개

```
 ┌──────────────────────────────────────────────────────────────────────────────┐
 │ (A) 장치 레지스터 실물         게스트에서 busybox devmem으로 BAR0 물리주소를  │
 │     = MSI-X 테이블/PBA          직접 읽음 (0xfebf2000 / 0xfebf3000)           │
 │ (B) 커널 자료구조              drgn --qemu 로 msi_desc / irq_desc /           │
 │                                 apic_chip_data / per-CPU vector_irq 덤프      │
 │ (C) QEMU 내부 이벤트           -trace: pci_nvme_irq_msix, apic_deliver_irq,   │
 │                                 apic_mem_writel(EOI) …                        │
 │ (D) LAPIC 상태 스냅샷          게스트 gdb로 인터럽트 처리 도중 정지 →          │
 │                                 QMP `info lapic 5` 로 ISR/IRR/PPR 확인        │
 └──────────────────────────────────────────────────────────────────────────────┘
```

---

## 1. 한 장 요약 — 전체 경로와 이번 실측 값

```
  [게스트 프로세스]  dd (taskset -c 5)  → CPU5에 고정
        │ read()
        ▼
  [blk-mq]  CPU5 → hctx5 → NVMe qid 6
        │
        ▼  ① SQ에 커맨드 기록 + SQ doorbell 쓰기 (MMIO)
  ┌─────────────────────────────────────────────────────────────────────────┐
  │ 호스트 메모리                          BAR0 = 0xfebf0000 (16KB)          │
  │  SQ6 @ 0x04fb0000 (dma)                 +0x0000 컨트롤러 레지스터        │
  │  CQ6 @ 0x060bc000 (dma)                 +0x1000 doorbell (q6 SQ=+0x1030) │
  │                                         +0x2000 MSI-X 테이블(65엔트리)   │
  │                                         +0x3000 MSI-X PBA               │
  └─────────────────────────────────────────────────────────────────────────┘
        │
        ▼  ② 장치가 CQ6에 CQE를 DMA write (sq_id=6, status=phase|0)
        │
        ▼  ③ nvme_irq_assert() → msix_notify(dev, vector=6)
        │      · entry6 masked? 아니오  · entry6 내용 읽음
        │
        ▼  ④ msi_send_message() = **bus-master DMA write 4바이트**
        │      address = 0xFEE20004 , data = 0x00000021
        │
        ▼  ⑤ 그 주소는 RAM이 아니라 Local APIC MSI 창(0xFEE00000~)
        │      apic_mem_write() → apic_send_msi()
        │      dest=0x20  dest_mode=1(logical)  delivery=0(fixed)  vector=33  trigger=0(edge)
        │
        ▼  ⑥ apic_get_delivery_bitmask(): dest 0x20 & 각 LAPIC의 LDR
        │      → LDR=0x20 인 CPU5만 매칭
        │
        ▼  ⑦ apic_set_irq(CPU5, 33): IRR[33]=1 → cpu_interrupt(CPU_INTERRUPT_HARD)
        │
        ▼  ⑧ CPU5가 명령 경계에서 인터럽트 수락 → apic_get_interrupt()
        │      IRR[33]=0, ISR[33]=1, PPR 0x10 → 0x20
        │
        ▼  ⑨ IDT[33] = 0xffffffff81e001f8 <irq_entries_start+8> 로 점프
        │      → asm_common_interrupt → common_interrupt(regs, vector=33)
        │
        ▼  ⑩ this_cpu(vector_irq)[33] = irq_desc(irq 30, "nvme0q6")
        │      → handle_edge_irq() → chip->irq_ack = EOI(LAPIC 0xB0 쓰기) → ISR[33]=0
        │
        ▼  ⑪ nvme_irq() → nvme_poll_cq() → CQE 처리 → CQ6 doorbell 쓰기
        │
        ▼  ⑫ blk_mq_complete_request → bio_endio → dd 깨어남
```

### 이번 실측의 핵심 숫자 (큐 6번 기준)

| 항목 | 값 | 관측점 |
|------|-----|--------|
| PCI 주소 | `0000:00:04.0` (`1b36:0010`, class `010802`) | 게스트 sysfs |
| BAR0 물리주소 / 크기 | `0xfebf0000` / `0x4000` | sysfs `resource`, HMP `info pci` |
| MSI-X 테이블 위치 | BAR0 + `0x2000` = `0xfebf2000` | config space cap 0x40 |
| MSI-X PBA 위치 | BAR0 + `0x3000` = `0xfebf3000` | config space cap 0x40 |
| MSI-X 테이블 크기 | 65 엔트리 (`Table Size` 필드 = 64) | config space `0x8040` |
| 실제 사용 벡터 수 | 9 (`nvme0q0` ~ `nvme0q8`) | `/proc/interrupts`, `dev->num_vecs=9` |
| 큐 6번 엔트리(=entry6) | addr_lo=`0xFEE20004`, addr_hi=`0`, data=`0x00000021`, vctrl=`0` | devmem(A) = drgn msi_desc(B) |
| 목적지 CPU | CPU5 (LDR `0x20`) | `info lapic 5`(D) |
| x86 벡터 번호 | 33 (`0x21`) | data 필드, `apic_chip_data.vector`(B), backtrace(D) |
| Linux IRQ 번호 | 30 (`nvme0q6`) | `/proc/interrupts`, `desc->irq_data.irq`(D) |
| hwirq | 65542 = `(0x10000 | 6)` | `desc->irq_data.hwirq`(D) |
| IDT 게이트 | `0xffffffff81e001f8 <irq_entries_start+8>` | gdb `p/x idt_table[33]`(D) |

---

## 2. 장치 쪽 준비 — MSI-X capability와 BAR0 레이아웃

### 2.1 PCI config space 실측 덤프

게스트에서 `hexdump -C /sys/bus/pci/devices/0000:00:04.0/config`:

**`[device]`** 게스트에서 `hexdump -C /sys/bus/pci/devices/0000:00:04.0/config` — 장치의 PCI config space 실물

```
00000000  36 1b 10 00 07 05 10 00  02 02 08 01 00 00 00 00  |6...............|
00000010  04 00 bf fe 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000020  00 00 00 00 00 00 00 00  00 00 00 00 f4 1a 00 11  |................|
00000030  00 00 00 00 40 00 00 00  00 00 00 00 0b 01 00 00  |....@...........|
00000040  11 80 40 80 00 20 00 00  00 30 00 00 00 00 00 00  |..@.. ...0......|
00000050  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000060  01 00 03 00 08 00 00 00  00 00 00 00 00 00 00 00  |................|
00000080  10 60 02 00 00 80 00 10  00 00 00 00 11 04 00 00  |.`..............|
```

해독:

| 오프셋 | 값 | 의미 |
|--------|-----|------|
| `0x00` | `1b36:0010` | Red Hat / QEMU NVMe Controller |
| `0x04` Command | `0x0507` | bit0 IO=1, bit1 **Memory Space=1**, bit2 **Bus Master=1**(장치가 DMA/MSI 쓰기를 할 수 있다), bit8 SERR=1, **bit10 INTx Disable=1**(레거시 핀 인터럽트 차단 — MSI-X를 쓰고 있다는 증거) |
| `0x06` Status | `0x0010` | bit4 Capabilities List 존재 |
| `0x08` | `02 02 08 01` | rev 2, prog-if `0x02`(NVMe), subclass `0x08`, class `0x01` |
| `0x10` BAR0 | `0xfebf0004` | bits[3:0]=`0x4` → **64비트 메모리 BAR**, base=`0xfebf0000` (BAR1은 상위 32비트로 0) |
| `0x34` Cap ptr | `0x40` | capability 체인 시작 |
| `0x3c` | `0b 01` | Interrupt Line 11, **Interrupt Pin 1 (INTA#)** — MSI-X를 쓰더라도 핀 정보는 그대로 남는다 |

capability 체인은 `0x40`(MSI-X) → `0x80`(PCI Express) → `0x60`(Power Mgmt) → 끝.

### 2.2 MSI-X capability(오프셋 0x40) 해독

```
0x40:  11 80 40 80    00 20 00 00    00 30 00 00
       │  │  └──┴── Message Control = 0x8040
       │  └── Next Cap = 0x80
       └── Cap ID = 0x11 (MSI-X)
```

| 필드 | 값 | 의미 |
|------|-----|------|
| Message Control `0x8040` bit[10:0] | `0x040` = 64 | **Table Size 필드는 N-1** → 실제 65개 엔트리 |
| Message Control bit14 (Function Mask) | 0 | 전체 마스크 해제 |
| Message Control bit15 (MSI-X Enable) | **1** | MSI-X 동작 중 |
| Table Offset/BIR `0x00002000` | BIR=`0`(BAR0), offset=`0x2000` | 테이블 = `0xfebf0000 + 0x2000` = **`0xfebf2000`** |
| PBA Offset/BIR `0x00003000` | BIR=`0`, offset=`0x3000` | PBA = **`0xfebf3000`** |

> BIR(BAR Indicator Register)는 하위 3비트이고 offset은 하위 3비트를 0으로 본 값이다.
> 여기서는 둘 다 BAR0 안에 들어있다(NVMe에서 흔한 배치).

### 2.3 오프셋이 왜 0x2000 / 0x3000인가 — QEMU가 계산한다

`hw/nvme/ctrl.c:8006` `nvme_bar_size()`가 BAR0 레이아웃을 만든다.

**소스: QEMU** `hw/nvme/ctrl.c:8006` — 장치가 BAR0 레이아웃을 정하는 코드

```c
static uint64_t nvme_bar_size(unsigned total_queues, unsigned total_irqs,
                              unsigned *msix_table_offset,
                              unsigned *msix_pba_offset)
{
    bar_size  = sizeof(NvmeBar) + 2 * total_queues * NVME_DB_SIZE;   /* 레지스터 + 도어벨 */
    bar_size  = QEMU_ALIGN_UP(bar_size, 4 * KiB);
    *msix_table_offset = bar_size;                                    /* ← 테이블 시작 */
    msix_table_size    = PCI_MSIX_ENTRY_SIZE * total_irqs;            /* 16 * N */
    bar_size += msix_table_size;
    bar_size  = QEMU_ALIGN_UP(bar_size, 4 * KiB);
    *msix_pba_offset   = bar_size;                                    /* ← PBA 시작 */
    msix_pba_size      = QEMU_ALIGN_UP(total_irqs, 64) / 8;
    bar_size += msix_pba_size;
    return pow2ceil(bar_size);                                        /* BAR는 2의 거듭제곱 */
}
```

이번 장치의 기본 파라미터 `max_ioqpairs=64`, `msix_qsize=65`를 대입하면:

```
 sizeof(NvmeBar) = 0x1000 (4096)            ← include/block/nvme.h, 마지막 필드 css[484]까지 정확히 4KB
 도어벨 = 2 * (64+1) * 4 = 520 바이트
 4096 + 520 = 4616 → 4KB 정렬 → 8192 = 0x2000   ⇒ msix_table_offset = 0x2000   ✓ 실측 일치
 테이블 = 16 * 65 = 1040 → 8192+1040 = 9232 → 4KB 정렬 → 12288 = 0x3000
                                             ⇒ msix_pba_offset  = 0x3000     ✓ 실측 일치
 PBA   = align_up(65,64)/8 = 16 바이트 → 12288+16 = 12304 → pow2ceil → 16384 = 0x4000
                                             ⇒ BAR0 크기 = 0x4000            ✓ 실측 일치
```

세 값 모두 게스트에서 읽은 값과 정확히 맞는다. 즉 **테이블 주소는 장치(에뮬레이터)가 정하고,
그 안에 들어갈 내용은 호스트(커널)가 정한다.** 이 구분이 이 문서 전체의 축이다.

### 2.4 QEMU가 이 영역을 붙이는 코드

`hw/nvme/ctrl.c:8104` 부근:

**소스: QEMU** `hw/nvme/ctrl.c:8104` — BAR0와 MSI-X 테이블을 등록

```c
memory_region_init(&n->bar0, OBJECT(n), "nvme-bar0", bar_size);
memory_region_init_io(&n->iomem, OBJECT(n), &nvme_mmio_ops, n, "nvme", msix_table_offset);
memory_region_add_subregion(&n->bar0, 0, &n->iomem);     /* 0x0000~0x1FFF: 컨트롤러+도어벨 */
pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY |
                 PCI_BASE_ADDRESS_MEM_TYPE_64, &n->bar0);
ret = msix_init(pci_dev, n->params.msix_qsize,
                &n->bar0, 0, msix_table_offset,      /* 테이블: BAR0 + 0x2000 */
                &n->bar0, 0, msix_pba_offset, 0, errp);  /* PBA:  BAR0 + 0x3000 */
```

`msix_init()`은 `msix_table_mmio_ops`/`msix_pba_mmio_ops`라는 별도 MemoryRegion을 BAR0 위에 겹쳐
붙인다. 그래서 **게스트가 `0xfebf2060`에 `writel`을 하면 `msix_table_mmio_write()`가 호출**되고,
`0xfebf1030`에 쓰면 `nvme_mmio_write()`(도어벨)가 호출된다. 같은 BAR, 다른 핸들러다.

---

## 3. 호스트 쪽 준비 (1) — 벡터 번호와 목적지 CPU 결정

### 3.1 커널이 벡터를 몇 개 요청하는가

부팅 트레이스(타임스탬프 실측)를 보면 MSI-X 설정이 **두 번** 일어난다.

**`[qemu]`** `-trace` — 게스트가 MSI-X capability를 건드린 순간들

```
2703866@1788926783.173273:msix_write_config dev nvme enabled 1 masked 1   ← ① 1차 enable
2703866@1788926783.175898:msix_write_config dev nvme enabled 1 masked 0   ← ② function mask 해제
2703866@1788926783.185799:msix_write_config dev nvme enabled 0 masked 0   ← ③ 전부 해제(free)
2703866@1788926783.187257:msix_write_config dev nvme enabled 1 masked 1   ← ④ 2차 enable
2703866@1788926783.189203:msix_write_config dev nvme enabled 1 masked 0   ← ⑤ mask 해제
```

이 5줄이 `drivers/nvme/host/pci.c`의 2단계 벡터 할당 그대로다.

```
 nvme_probe
   └─ nvme_reset_work
        ├─ nvme_pci_enable()
        │    └─ pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES)  → ①②  (admin용 1개)
        │        · MSI-X Enable=1 & Function Mask=1 로 켠 뒤
        │        · 테이블 entry0을 쓰고 → Function Mask=0
        ├─ nvme_configure_admin_queue()   ← 이 구간의 admin 완료는 벡터 0으로 들어온다
        └─ nvme_setup_io_queues()
             ├─ pci_free_irq_vectors(pdev)                            → ③
             └─ nvme_setup_irqs() → pci_alloc_irq_vectors_affinity(   → ④⑤
                    pdev, 1, nr_io_queues+1, PCI_IRQ_ALL_TYPES|PCI_IRQ_AFFINITY, &affd)
```

MSI-X 스펙이 "테이블을 고칠 때는 Function Mask를 세워라"고 하므로, 커널은 항상
**Enable=1 & Mask=1 → 테이블 기록 → Mask=0** 순서를 지킨다. 위 5줄이 그 증거다.

결과: `dev->num_vecs = 9` (gdb 실측), `/proc/interrupts`에 `nvme0q0`~`nvme0q8` 9개.

큐↔벡터 대응은 `pci.c:1800` (이 트리 기준):

**소스: 리눅스 커널** `drivers/nvme/host/pci.c:1800`

```c
vector = dev->num_vecs == 1 ? 0 : qid;   /* 벡터 번호 = 큐 ID */
...
nvmeq->cq_vector = vector;
```

즉 **qid = MSI-X 테이블 엔트리 인덱스**다. gdb로 인터럽트 안에서 확인:

**`[driver]`** 게스트 gdb — 인터럽트 핸들러 안에서 본 커널 큐 구조체

```
(gdb) p ((struct nvme_queue *)data)->qid          → $12 = 6
(gdb) p ((struct nvme_queue *)data)->cq_vector    → $13 = 6
```

그리고 QEMU 쪽 큐 생성 트레이스도 같은 값을 말한다:

**`[qemu]`** `-trace` — 장치가 받은 Create CQ/SQ 커맨드

```
2703852@1788926783.197857:pci_nvme_create_cq ... addr=0x60bc000, cqid=6, vector=6, qsize=1023, qflags=3, ien=1
2703852@1788926783.198446:pci_nvme_create_sq ... addr=0x4fb0000, sqid=6, cqid=6, qsize=1023, qflags=1
```

커널이 gdb로 보여준 DMA 주소와 **완전히 동일**하다:

**`[driver]`** 게스트 gdb — 커널이 잡아둔 큐의 DMA 주소

```
(gdb) p/x ((struct nvme_queue *)data)->cq_dma_addr → $14 = 0x60bc000
(gdb) p/x ((struct nvme_queue *)data)->sq_dma_addr → $15 = 0x4fb0000
```

> 참고: `Create I/O Completion Queue` 커맨드의 DW11에 **Interrupt Vector(IV)** 필드가 있다.
> 트레이스의 `vector=6`이 바로 그 값이고, `ien=1`은 Interrupts Enabled 비트다.
> **MSI-X 테이블 인덱스는 이 IV 필드로 장치에 전달된다** — 테이블 자체에는 "몇 번 큐용"이라는
> 정보가 없다. 매핑은 큐 생성 커맨드가 정한다.

### 3.2 x86 벡터 번호는 왜 33/34인가 — per-CPU 벡터 공간

> **쉬운 말로**: "벡터 번호"는 전역 번호가 아니라 **CPU마다 따로 노는 번호**다.
> 같은 33번이라도 CPU0에서는 nvme 큐1, CPU2에서는 디스크 컨트롤러(ata_piix)일 수 있다.
> 그래서 NVMe 큐들이 33번과 34번으로 뒤죽박죽 섞여 보인다 — 버그가 아니다.

x86에서 **인터럽트 벡터는 CPU마다 독립적인 자원**이다. drgn으로 per-CPU `vector_irq[]`를 덤프하면:

**`[driver]`** drgn — CPU마다 따로 있는 "벡터 번호 → irq_desc" 표

```
=== per-CPU vector_irq[32..40] (drgn --qemu 실측) ===
  cpu0: v32:-  v33:irq25(nvme0q1)  v34:irq4(ttyS0)      v35..40:-
  cpu1: v32:-  v33:irq9(acpi)      v34:irq26(nvme0q2)   v35..40:-
  cpu2: v32:-  v33:irq14(ata_piix) v34:irq27(nvme0q3)   v35..40:-
  cpu3: v32:-  v33:irq15(ata_piix) v34:irq28(nvme0q4)   v35..40:-
  cpu4: v32:-  v33:irq29(nvme0q5)  v34:irq24(nvme0q0)   v35..40:-
  cpu5: v32:-  v33:irq30(nvme0q6)  v34:irq12(i8042)     v35..40:-
  cpu6: v32:-  v33:irq31(nvme0q7)  v34:irq1(i8042)      v35..40:-
  cpu7: v32:-  v33:irq32(nvme0q8)  v34:irq8(rtc0)       v35..40:-
```

읽는 법:

* **벡터 32(`0x20`)는 전 CPU에서 비어 있다.** `arch/x86/include/asm/irq_vectors.h:36,43`에서
  `FIRST_EXTERNAL_VECTOR = 0x20`이고 `IRQ_MOVE_CLEANUP_VECTOR`가 그 자리를 예약하기 때문에
  **일반 디바이스가 받을 수 있는 최저 벡터는 33**이다.
* CPU0은 33이 비어 있어서 `nvme0q1`이 33을 받았지만, CPU1~3은 33이 이미
  `acpi`/`ata_piix`에 쓰이고 있어서 `nvme0q2~q4`가 **34**를 받았다.
* 그래서 같은 NVMe 장치인데도 테이블 엔트리마다 data가 `0x21`(33)과 `0x22`(34)로 섞인다.
  **벡터 번호는 "장치 순서"가 아니라 "그 목적지 CPU에서 비어 있던 번호"**다.

이 사실은 `/proc/interrupts`만 봐서는 절대 안 보이고, per-CPU 벡터 테이블을 떠야 보인다.

### 3.3 목적지 CPU 표현 — flat logical APIC

drgn으로 확인한 APIC 드라이버:

**`[driver]`** drgn — 커널이 고른 APIC 드라이버

```
apic->name = flat | dest_mode_logical = True
```

`CONFIG_X86_X2APIC`가 꺼져 있고 CPU가 8개 이하이므로 커널은 `apic_flat`을 고른다.
이 모드에서 목적지 APIC ID는 **CPU 번호의 비트마스크**다.

**소스: 리눅스 커널** `arch/x86/kernel/apic/apic_common.c:14`

```c
/* arch/x86/kernel/apic/apic_common.c:14 */
u32 apic_flat_calc_apicid(unsigned int cpu)
{
	return 1U << cpu;
}
```

QEMU가 보는 각 LAPIC의 LDR/DFR (QMP `info lapic N` 실측):

| vCPU | LDR | DFR | 해석 |
|------|-----|-----|------|
| 0 | `0x01` | `0x0f` | flat model, 논리 ID 비트0 |
| 1 | `0x02` | `0x0f` | 비트1 |
| 2 | `0x04` | `0x0f` | 비트2 |
| 3 | `0x08` | `0x0f` | 비트3 |
| 4 | `0x10` | `0x0f` | 비트4 |
| 5 | **`0x20`** | `0x0f` | **비트5 ← 우리가 추적할 CPU** |
| 6 | `0x40` | `0x0f` | 비트6 |
| 7 | `0x80` | `0x0f` | 비트7 |

`DFR = 0x0f`가 flat model을 뜻한다(cluster model이면 `0x00`).
**이 LDR 값이 잠시 뒤 MSI 주소의 `[19:12]` 필드와 그대로 매칭된다.**

---

## 4. 호스트 쪽 준비 (2) — msi_msg 조립과 테이블 기록

### 4.1 커널이 메시지를 만드는 코드

`arch/x86/kernel/apic/apic.c:2556`:

**소스: 리눅스 커널** `arch/x86/kernel/apic/apic.c:2556` — 커널이 MSI 메시지를 조립하는 곳

```c
void __irq_msi_compose_msg(struct irq_cfg *cfg, struct msi_msg *msg, bool dmar)
{
	memset(msg, 0, sizeof(*msg));

	msg->arch_addr_lo.base_address     = X86_MSI_BASE_ADDRESS_LOW;  /* 0xfee00000 >> 20 = 0xfee */
	msg->arch_addr_lo.dest_mode_logical = apic->dest_mode_logical;  /* flat → 1 */
	msg->arch_addr_lo.destid_0_7        = cfg->dest_apicid & 0xFF;  /* 1<<cpu */

	msg->arch_data.delivery_mode        = APIC_DELIVERY_MODE_FIXED; /* 0 */
	msg->arch_data.vector               = cfg->vector;              /* 33 */

	msg->address_hi = X86_MSI_BASE_ADDRESS_HIGH;                    /* 0 */
	...
}
```

`cfg->vector`와 `cfg->dest_apicid`는 x86 벡터 도메인(`arch/x86/kernel/apic/vector.c`)이
`irq_matrix_alloc()`으로 정한 값이며, drgn으로 `apic_chip_data`를 직접 떠서 확인했다:

**`[driver]`** drgn — irq 30의 벡터/목적지 결정 결과

```
irq=30 name=nvme0q6
   [level 0] chip=PCI-MSI hwirq=65542
   [level 1] chip=APIC    hwirq=30
       apic_chip_data: vector=33 cpu=5 prev_vector=0 prev_cpu=0
                       is_managed=1 dest_apicid=0x20 cfg_vector=33
```

`is_managed=1`은 **managed IRQ affinity**(blk-mq가 큐↔CPU를 고정하기 위해
`PCI_IRQ_AFFINITY`로 요청) 상태다. 반면 admin 큐는:

**`[driver]`** drgn — admin 큐(irq 24)는 managed가 아니다

```
irq=24 name=nvme0q0
       apic_chip_data: vector=34 cpu=4 ... is_managed=0 dest_apicid=0x10
```

admin은 `is_managed=0`(일반 affinity)이고 커널이 CPU4를 골랐다.

### 4.2 MSI 주소/데이터 비트 필드 (x86)

`arch/x86/include/asm/msi.h:29`:

**소스: 리눅스 커널** `arch/x86/include/asm/msi.h:29` — MSI 주소/데이터 비트 정의

```c
typedef struct x86_msi_addr_lo {
	u32	reserved_0        :  2,   /* [1:0]   0 */
		dest_mode_logical :  1,   /* [2]     1=logical, 0=physical */
		redirect_hint     :  1,   /* [3]     RH */
		reserved_1        :  1,   /* [4] */
		virt_destid_8_14  :  7,   /* [11:5]  확장 dest id(하이퍼바이저용) */
		destid_0_7        :  8,   /* [19:12] 목적지 APIC ID(또는 논리 마스크) */
		base_address      : 12;   /* [31:20] 0xFEE 고정 */
} arch_msi_msg_addr_lo_t;

typedef struct x86_msi_data {
	u32	vector            :  8,   /* [7:0]   인터럽트 벡터 */
		delivery_mode     :  3,   /* [10:8]  0=Fixed, 1=Lowest Priority, 2=SMI, 4=NMI, 5=INIT */
		dest_mode_logical :  1,   /* [11] */
		reserved          :  2,
		active_low        :  1,   /* [14] */
		is_level          :  1;   /* [15]  0=edge */
} arch_msi_msg_data_t;
```

**엔트리 6번(`0xFEE20004` / `0x00000021`) 비트 단위 해독:**

```
 address_lo = 0xFEE2_0004
   ┌───────────────┬────────┬───────┬─┬─┬──┐
   │ 31..20 = 0xFEE│19..12  │11..5  │4│3│2 │1..0
   │ base_address  │ =0x20  │ =0    │0│0│1 │ 00
   └───────────────┴────────┴───────┴─┴─┴──┘
     └ MSI 고정 창   └ destid  └virt  │ │ └ dest_mode_logical=1 → 논리 목적지
                       =CPU5의       │ └ redirect_hint=0 → 지정된 목적지로 직접
                       LDR 0x20      └ reserved

 data = 0x0000_0021
   vector=0x21(33), delivery_mode=0(Fixed), dest_mode_logical=0, active_low=0, is_level=0(edge)
```

커널이 자기 비트필드로 해석한 값을 gdb로 그대로 출력하면(인터럽트 처리 도중 실측):

**`[driver]`** 게스트 gdb — 커널이 자기 비트필드로 해석해 보여준 MSI 메시지

```
(gdb) p ((struct msi_desc *)desc->irq_common_data.msi_desc)->msg
$7 = {{address_lo = 4276224004,                       /* = 0xFEE20004 */
       arch_addr_lo = {{{reserved_0 = 0, dest_mode_logical = 1, redirect_hint = 0,
                         reserved_1 = 0, virt_destid_8_14 = 0,
                         destid_0_7 = 32,             /* = 0x20 */
                         base_address = 4078},        /* = 0xFEE */
      ...}}},
      {address_hi = 0, ...},
      {data = 33, arch_data = {{{vector = 33, delivery_mode = 0,
                                 dest_mode_logical = 0, reserved = 0,
                                 active_low = 0, is_level = 0}, ...}}}}
```

### 4.3 테이블에 실제로 쓰는 코드

`drivers/pci/msi/msi.c:151` `__pci_write_msi_msg()`:

**소스: 리눅스 커널** `drivers/pci/msi/msi.c:151` — 테이블에 실제로 써 넣는 코드

```c
void __pci_write_msi_msg(struct msi_desc *entry, struct msi_msg *msg)
{
	...
	} else if (entry->pci.msi_attrib.is_msix) {
		void __iomem *base = pci_msix_desc_addr(entry);   /* BAR0 ioremap + 0x2000 + 16*idx */
		...
		writel(msg->address_lo, base + PCI_MSIX_ENTRY_LOWER_ADDR);  /* +0x0 */
		writel(msg->address_hi, base + PCI_MSIX_ENTRY_UPPER_ADDR);  /* +0x4 */
		writel(msg->data,       base + PCI_MSIX_ENTRY_DATA);        /* +0x8 */
	}
	entry->msg = *msg;   /* 커널 캐시본 — 우리가 drgn/gdb로 읽은 그 값 */
}
```

이 `writel` 세 번이 게스트 입장에선 MMIO 스토어이고, QEMU 입장에선
`hw/pci/msix.c:220 msix_table_mmio_write()` 호출이다:

**소스: QEMU** `hw/pci/msix.c:220` — 그 쓰기를 장치가 받는 쪽

```c
static void msix_table_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    PCIDevice *dev = opaque;
    int vector = addr / PCI_MSIX_ENTRY_SIZE;     /* addr 0x60 → vector 6 */
    bool was_masked = msix_is_masked(dev, vector);
    pci_set_long(dev->msix_table + addr, val);   /* 장치 내부 테이블 갱신 */
    msix_handle_mask_update(dev, vector, was_masked);
}
```

즉 **MSI-X 테이블은 장치가 소유한 메모리이고, 호스트는 MMIO로 그 안에 "나에게 연락할 주소"를
써 넣는다.** 이것이 질문의 "host에서 정해준 msix table의 주소"의 정확한 실체다.

### 4.4 실측 — 테이블 9개 엔트리 전체 (`[device]` 장치 실물 vs `[driver]` 커널 캐시)

> 이 절이 이 문서의 심장이다. **같은 값이 장치 안에도, 커널 안에도 있어야 한다.**
> 하나라도 어긋나면 인터럽트가 엉뚱한 CPU로 가거나 아예 안 온다.

게스트에서 BAR0 물리주소를 busybox `devmem`으로 직접 읽었다
(`CONFIG_IO_STRICT_DEVMEM=n`이라 MMIO 영역은 `/dev/mem`으로 읽을 수 있다):

**`[device]`** 게스트에서 `devmem`으로 BAR0+0x2000을 직접 읽는 명령

```sh
i=0; while [ $i -le 8 ]; do
  b=$((0xfebf2000 + i*16))
  printf "entry%d @0x%x: addr_lo=%s addr_hi=%s data=%s vctrl=%s\n" $i $b \
    "$(devmem $b 32)" "$(devmem $((b+4)) 32)" "$(devmem $((b+8)) 32)" "$(devmem $((b+12)) 32)"
  i=$((i+1))
done
```

| entry | `[device]` addr_lo | addr_hi | data | vctrl | destid[19:12] | → CPU | vector | Linux IRQ / 이름 |
|-------|--------------------|---------|------|-------|---------------|-------|--------|------------------|
| 0 | `0xFEE10004` | 0 | `0x22` | 0 | `0x10` | CPU4 | 34 | 24 / `nvme0q0` (admin) |
| 1 | `0xFEE01004` | 0 | `0x21` | 0 | `0x01` | CPU0 | 33 | 25 / `nvme0q1` |
| 2 | `0xFEE02004` | 0 | `0x22` | 0 | `0x02` | CPU1 | 34 | 26 / `nvme0q2` |
| 3 | `0xFEE04004` | 0 | `0x22` | 0 | `0x04` | CPU2 | 34 | 27 / `nvme0q3` |
| 4 | `0xFEE08004` | 0 | `0x22` | 0 | `0x08` | CPU3 | 34 | 28 / `nvme0q4` |
| 5 | `0xFEE10004` | 0 | `0x21` | 0 | `0x10` | CPU4 | 33 | 29 / `nvme0q5` |
| **6** | **`0xFEE20004`** | 0 | **`0x21`** | 0 | `0x20` | **CPU5** | **33** | **30 / `nvme0q6`** |
| 7 | `0xFEE40004` | 0 | `0x21` | 0 | `0x40` | CPU6 | 33 | 31 / `nvme0q7` |
| 8 | `0xFEE80004` | 0 | `0x21` | 0 | `0x80` | CPU7 | 33 | 32 / `nvme0q8` |

같은 순간 커널 쪽 `msi_desc.msg`(drgn `--qemu` 실측):

**`[driver]`** drgn — 커널이 들고 있는 msi_desc 9개

```
irq=24 nvme0q0  msi_msg: address_lo=0xfee10004 address_hi=0x00000000 data=0x00000022  msi_index=0
irq=25 nvme0q1  msi_msg: address_lo=0xfee01004 address_hi=0x00000000 data=0x00000021  msi_index=1
irq=26 nvme0q2  msi_msg: address_lo=0xfee02004 address_hi=0x00000000 data=0x00000022  msi_index=2
irq=27 nvme0q3  msi_msg: address_lo=0xfee04004 address_hi=0x00000000 data=0x00000022  msi_index=3
irq=28 nvme0q4  msi_msg: address_lo=0xfee08004 address_hi=0x00000000 data=0x00000022  msi_index=4
irq=29 nvme0q5  msi_msg: address_lo=0xfee10004 address_hi=0x00000000 data=0x00000021  msi_index=5
irq=30 nvme0q6  msi_msg: address_lo=0xfee20004 address_hi=0x00000000 data=0x00000021  msi_index=6
irq=31 nvme0q7  msi_msg: address_lo=0xfee40004 address_hi=0x00000000 data=0x00000021  msi_index=7
irq=32 nvme0q8  msi_msg: address_lo=0xfee80004 address_hi=0x00000000 data=0x00000021  msi_index=8
```

**9개 엔트리가 한 비트도 틀리지 않고 일치한다.** (A)장치 BAR 실물 = (B)커널 캐시.
`msi_index`가 곧 테이블 엔트리 인덱스이며, `nvme0q<N>`의 N과 같다.

`/proc/interrupts`가 이 매핑을 다시 확인해준다 — 부팅 직후 admin 인터럽트 24회가
**전부 CPU4에** 찍혀 있는데, 이는 entry0의 destid `0x10`(CPU4)과 정확히 일치한다:

**`[driver]`** 게스트 `/proc/interrupts`

```
           CPU0  CPU1  CPU2  CPU3  CPU4  CPU5  CPU6  CPU7
 24:          0     0     0     0    24     0     0     0   PCI-MSI 65536-edge  nvme0q0
 25..32:      0 ...                                          PCI-MSI 65537..65544-edge  nvme0q1..q8
```

트레이스로도 같은 사실이 보인다. admin 인터럽트(`vector 0`) 뒤의 CQ 도어벨은 **항상 tid 2703863
= vCPU4**가 친다:

**`[qemu]`** `-trace` — admin 인터럽트는 항상 vCPU4가 처리한다

```
2703852@...783.209238:pci_nvme_irq_msix raising MSI-X IRQ vector 0     ← 장치(main thread)
2703863@...783.209266:pci_nvme_mmio_doorbell_cq cqid 0 new_head 26     ← vCPU4가 처리
2703852@...783.209456:pci_nvme_irq_msix raising MSI-X IRQ vector 0
2703863@...783.209480:pci_nvme_mmio_doorbell_cq cqid 0 new_head 27
```

### 4.5 hwirq 65542의 정체

`desc->irq_data.hwirq = 65542`의 정체는 `drivers/pci/msi/irqdomain.c`:

**소스: 리눅스 커널** `drivers/pci/msi/irqdomain.c`

```c
static irq_hw_number_t pci_msi_domain_calc_hwirq(struct msi_desc *desc)
{
	struct pci_dev *dev = msi_desc_to_pci_dev(desc);

	return (irq_hw_number_t)desc->msi_index |
		pci_dev_id(dev) << 11 |
		(pci_domain_nr(dev->bus) & 0xFFFFFFFF) << 27;
}
```

우리 장치 `0000:00:04.0`을 대입하면:

```
 pci_dev_id = (bus 0 << 8) | devfn(4<<3 | 0) = 0x20 = 32
 32 << 11                                     = 65536 = 0x10000
 | msi_index(6)                               = 65542   ✓ 실측 일치
 (domain 0 이므로 상위 항은 0)
```

`/proc/interrupts`에 찍힌 `PCI-MSI 65536-edge nvme0q0` ~ `PCI-MSI 65544-edge nvme0q8`가
바로 이 값이다. 즉 **hwirq 하위 11비트가 MSI-X 테이블 엔트리 인덱스, 그 위가 BDF**다.
장치가 여러 개여도 hwirq만 보면 어느 장치의 몇 번 엔트리인지 알 수 있다.

---

## 5. 발사 — SSD가 인터럽트를 쏘는 순간

> **쉬운 말로**: 여기까지가 "준비", 여기서부터가 "실행"이다.
> SSD는 자기 수첩(MSI-X 테이블)의 6번 칸을 펴서 거기 적힌 주소 `0xFEE20004`에
> 적힌 값 `0x21`을 쓴다. 그게 전부다. SSD는 그 주소가 무엇인지 모르고 알 필요도 없다.

### 5.1 장치 내부 경로

```
  블록 백엔드 I/O 완료 (QEMU main thread, tid 2703852)
        │
        ▼
  nvme_rw_cb() → nvme_enqueue_req_completion()     hw/nvme/ctrl.c
        │  · CQE를 cq->tail 위치에 씀 (게스트 물리메모리로 DMA write)
        │  · phase 비트 토글
        ▼
  nvme_post_cqes()  ──▶ nvme_irq_assert(n, cq)     hw/nvme/ctrl.c:1529
        │
        ▼
  static void nvme_irq_assert(NvmeCtrl *n, NvmeCQueue *cq)   /* ctrl.c:662 */
  {
      if (cq->irq_enabled) {                 /* Create CQ 때의 IEN 비트 */
          if (msix_enabled(pci)) {
              trace_pci_nvme_irq_msix(cq->vector);   ← 트레이스 이벤트
              msix_notify(pci, cq->vector);          /* vector = 큐 ID = 테이블 인덱스 */
          } else {
              trace_pci_nvme_irq_pin();
              pci_irq_assert(pci);           /* 레거시 INTx */
          }
      } else {
          trace_pci_nvme_irq_masked();
      }
  }
```

### 5.2 `msix_notify()`가 하는 세 가지 검사 (`hw/pci/msix.c:525`)

**소스: QEMU** `hw/pci/msix.c:525` — 장치가 인터럽트를 쏘는 진입점

```c
void msix_notify(PCIDevice *dev, unsigned vector)
{
    MSIMessage msg;

    assert(vector < dev->msix_entries_nr);

    if (!dev->msix_entry_used[vector])      /* ① 이 벡터를 쓰기로 등록돼 있나 */
        return;

    if (msix_is_masked(dev, vector)) {      /* ② 마스크됐나 (Function Mask || Vector Ctrl bit0) */
        msix_set_pending(dev, vector);      /*    → PBA 비트만 세우고 리턴 (§9에서 실측) */
        return;
    }

    msg = msix_get_message(dev, vector);    /* ③ 테이블에서 address/data를 읽어온다 */
    msi_send_message(dev, msg);
}
```

`msix_get_message()`는 결국 이렇게 읽는다 (`msix.c:118`):

**소스: QEMU** `hw/pci/msix.c:118` — 테이블에서 주소/데이터를 꺼내는 부분

```c
uint8_t *table_entry = dev->msix_table + vector * PCI_MSIX_ENTRY_SIZE;
msg.address = pci_get_quad(table_entry + PCI_MSIX_ENTRY_LOWER_ADDR);  /* 64비트 */
msg.data    = pci_get_long(table_entry + PCI_MSIX_ENTRY_DATA);
```

**장치는 자기 테이블에 적힌 값을 그대로 읽어서 쓴다. 해석하지 않는다.**
`0xFEE20004`가 LAPIC인지 RAM인지 장치는 모른다.

### 5.3 인터럽트 = bus master DMA write 한 번

`hw/pci/msi.c:377` → `hw/pci/pci.c`의 기본 트리거:

**소스: QEMU** `hw/pci/msi.c:377`, `hw/pci/pci.c` — 인터럽트 = DMA 쓰기

```c
void msi_send_message(PCIDevice *dev, MSIMessage msg)
{
    dev->msi_trigger(dev, msg);          /* 기본값 = pci_msi_trigger */
}

static void pci_msi_trigger(PCIDevice *dev, MSIMessage msg)
{
    MemTxAttrs attrs = {};
    attrs.requester_id = pci_requester_id(dev);        /* BDF 00:04.0 */
    address_space_stl_le(&dev->bus_master_as,          /* ← 장치의 DMA 주소공간 */
                         msg.address, msg.data, attrs, NULL);
}
```

실제 PCIe에서는 이것이 **4바이트 Memory Write TLP** 하나다.

```
 PCIe Memory Write TLP (요약)
   Requester ID : 00:04.0        ← attrs.requester_id
   Address      : 0xFEE2_0004    ← MSI-X 테이블 entry6 addr_lo/hi
   Length       : 1 DW (4 bytes)
   Data         : 0x0000_0021    ← MSI-X 테이블 entry6 data
```

인터럽트 전용 신호선도, 특별한 트랜잭션 타입도 없다. **주소가 특별할 뿐이다.**
(그래서 §10에서 CPU가 같은 주소에 쓰면 똑같이 인터럽트가 난다.)

### 5.4 실측 — 완료 한 건의 타임스탬프 트레이스

게스트에서 `taskset -c 5 dd if=/dev/nvme0n1 of=/dev/null bs=4096 count=1 skip=4096`
(직전에 `drop_caches`로 페이지 캐시를 비워 실제 디바이스 읽기를 강제) 실행 중 캡처:

```
[qemu·vCPU5] 2703864@1788927128.493233:pci_nvme_mmio_doorbell_sq sqid 6 new_tail 2
[qemu·vCPU5] 2703864@1788927128.493679:apic_mem_writel 0x380 = 0x17d0f064
[qemu·SSD  ] 2703852@1788927128.493979:pci_nvme_enqueue_req_completion cid 4096 cqid 6 dw0 0x0 dw1 0x0 status 0x0
[qemu·SSD  ] 2703852@1788927128.494002:pci_nvme_irq_msix raising MSI-X IRQ vector 6
[qemu·APIC ] 2703852@1788927128.494008:apic_deliver_irq dest 32 dest_mode 1 delivery_mode 0 vector 33 trigger_mode 0
[qemu·vCPU5] 2703864@1788927128.494078:apic_mem_writel 0xb0 = 0x00000000
[qemu·vCPU5] 2703864@1788927128.494286:pci_nvme_mmio_doorbell_cq cqid 6 new_head 2

   ※ 앞의 [qemu·xxx]는 이 문서에서 붙인 표시다. 실제 로그는 <스레드ID>@<시각>: 부터 시작한다.
      2703864 = vCPU5 스레드, 2703852 = QEMU main 스레드(=에뮬레이션된 SSD와 APIC이 도는 곳).
```

줄별 해석:

| 시각(µs 기준) | tid | 이벤트 | 의미 |
|---------------|-----|--------|------|
| `.493233` | 2703864 = **vCPU5** | `doorbell_sq sqid 6 new_tail 2` | dd가 CPU5에서 제출 → SQ6 도어벨(BAR0+0x1030) |
| `.493679` | vCPU5 | `apic_mem_writel 0x380` | LAPIC Timer Initial Count — idle 진입 전 타이머 재설정 |
| `.493979` | 2703852 = **장치** | `enqueue_req_completion cid 4096 cqid 6 status 0x0` | CQE를 CQ6에 씀 (성공) |
| `.494002` | 장치 | `irq_msix raising MSI-X IRQ vector 6` | `nvme_irq_assert` → `msix_notify(dev,6)` |
| `.494008` | 장치 | `apic_deliver_irq dest 32 dest_mode 1 delivery_mode 0 vector 33 trigger_mode 0` | **MSI 쓰기가 LAPIC에 도달해 해석된 결과** |
| `.494078` | **vCPU5** | `apic_mem_writel 0xb0 = 0` | LAPIC **EOI**(오프셋 0xB0) — CPU5가 인터럽트를 받았다는 증거 |
| `.494286` | vCPU5 | `doorbell_cq cqid 6 new_head 2` | `nvme_irq→nvme_poll_cq`가 CQ 헤드 갱신 |

`dest 32` = `0x20` = CPU5의 LDR, `vector 33` = data 필드 `0x21`.
**테이블에 적어둔 값이 그대로 LAPIC 파라미터로 나타났다.**

구간별 소요(TCG라 절대값은 실제 하드웨어보다 훨씬 느리다. 비율만 참고):

```
 SQ 도어벨 → CQE 기록      746 µs   (에뮬레이션 블록 I/O 지연)
 CQE 기록  → MSI 발사       23 µs
 MSI 발사  → LAPIC 해석      6 µs
 LAPIC 해석 → CPU5 EOI      70 µs   (vCPU 스레드가 스케줄되어 인터럽트를 수락하기까지)
 EOI       → CQ 도어벨     208 µs   (핸들러 진입~CQE 파싱)
```

---

## 6. 배달 — Local APIC

> **쉬운 말로**: 방금 SSD가 쓴 4바이트를 받는 쪽 이야기다.
> `0xFEE20004`는 RAM이 아니라 **CPU 옆에 붙은 인터럽트 접수 창구(Local APIC)**의 주소다.
> 창구는 그 쓰기를 보고 두 가지를 읽어낸다 —
> **주소에서 "누구에게"(CPU5), 데이터에서 "몇 번 용건"(33번)**.
> 그리고 CPU5의 접수 장부(IRR)에 33번 도장을 찍고 CPU5를 깨운다.

### 6.1 왜 메모리 쓰기가 인터럽트가 되는가

x86에서 `0xFEE00000`~`0xFEEFFFFF`는 **Local APIC이 점유한 주소 영역**이다.
QEMU는 이 영역을 `apic-msi`라는 MemoryRegion으로 등록하고, 그 write 핸들러
`apic_mem_write()`가 오프셋으로 두 갈래를 나눈다(`hw/intc/apic.c:740`):

**소스: QEMU** `hw/intc/apic.c:740` — 0xFEE00000 영역에 쓰기가 들어왔을 때

```c
static void apic_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    int index = (addr >> 4) & 0xff;

    if (size < 4) return;

    if (addr > 0xfff || !index) {
        /* MSI and MMIO APIC are at the same memory location,
         * but actually not on the global bus: MSI is on PCI bus
         * APIC is connected directly to the CPU. ... */
        MSIMessage msi = { .address = addr, .data = val };
        apic_send_msi(&msi);              /* ← MSI 경로 */
        return;
    }

    dev = cpu_get_current_apic();         /* ← LAPIC 레지스터 경로 (현재 CPU 자신의 LAPIC) */
    s = APIC(dev);
    trace_apic_mem_writel(addr, val);
    switch (index) {
        ...
        case 0x0b:  apic_eoi(s); break;   /* 오프셋 0xB0 = EOI */
        ...
    }
}
```

핵심 분기 조건 `addr > 0xfff || !index`:

* `addr`는 **MemoryRegion 내부 오프셋**이다(= 물리주소 − `0xFEE00000`).
* `0xFEE20004` → 오프셋 `0x20004` → `> 0xfff` → **MSI 경로**.
* `0xFEE000B0`(EOI) → 오프셋 `0xb0` → index=0x0b → **레지스터 경로**.
* `0xFEE00004`(dest 0) → 오프셋 `0x4` → index=0 → `!index` 성립 → MSI 경로.
  (destid가 0인 MSI도 정상 처리되게 하는 예외 처리)

즉 **첫 4KB 안쪽은 LAPIC 레지스터, 그 위쪽(destid≠0)은 MSI 창**이다.
실제 하드웨어에서는 이 구분이 "CPU 코어가 직접 낸 접근(LAPIC 레지스터) vs 시스템 버스에서
들어온 접근(MSI)"으로 나뉘지만, QEMU는 주소 범위 트릭으로 근사한다(위 주석 그대로).

### 6.2 주소/데이터 → dest/vector 분해 (`hw/intc/apic.c:727`)

**소스: QEMU** `hw/intc/apic.c:727` — 주소/데이터를 dest·vector로 분해

```c
static void apic_send_msi(MSIMessage *msi)
{
    uint64_t addr = msi->address;                  /* = 0x20004 (리전 오프셋) */
    uint32_t data = msi->data;                     /* = 0x21 */
    uint8_t dest         = (addr & MSI_ADDR_DEST_ID_MASK) >> MSI_ADDR_DEST_ID_SHIFT; /* [19:12] → 0x20 */
    uint8_t vector       = (data & MSI_DATA_VECTOR_MASK) >> MSI_DATA_VECTOR_SHIFT;   /* [7:0]  → 33 */
    uint8_t dest_mode    = (addr >> MSI_ADDR_DEST_MODE_SHIFT) & 0x1;                 /* [2]    → 1 */
    uint8_t trigger_mode = (data >> MSI_DATA_TRIGGER_SHIFT) & 0x1;                   /* [15]   → 0 */
    uint8_t delivery     = (data >> MSI_DATA_DELIVERY_MODE_SHIFT) & 0x7;             /* [10:8] → 0 */
    /* XXX: Ignore redirection hint. */
    apic_deliver_irq(dest, dest_mode, delivery, vector, trigger_mode);
}
```

> 주의: 여기서 `addr`은 리전 오프셋(`0x20004`)이지 물리주소(`0xFEE20004`)가 아니다.
> `[19:12]` 마스크가 두 값 모두에서 같은 결과(`0x20`)를 주기 때문에 동작에 차이는 없다.
> 트레이스에서 `dest 32`로 보이는 값이 이 `0x20`이다.

### 6.3 논리 목적지 해석 — 누가 받을 것인가 (`apic.c:457`)

**소스: QEMU** `hw/intc/apic.c:457` — 어느 CPU가 받을지 고르는 곳

```c
static void apic_get_delivery_bitmask(uint32_t *deliver_bitmask, uint8_t dest, uint8_t dest_mode)
{
    if (dest_mode == 0) {                     /* physical: APIC ID 정확히 일치 */
        ...
    } else {                                  /* logical */
        for (i = 0; i < MAX_APICS; i++) {
            apic_iter = local_apics[i];
            if (!apic_iter) break;
            if (apic_iter->dest_mode == 0xf) {          /* DFR flat model */
                if (dest & apic_iter->log_dest)          /* ← LDR 비트마스크 AND */
                    apic_set_bit(deliver_bitmask, i);
            } else if (apic_iter->dest_mode == 0x0) {    /* cluster model */
                ...
            }
        }
    }
}
```

우리 케이스: `dest = 0x20`, 각 CPU의 `log_dest`(LDR) = `1<<cpu`.

```
  dest 0x20 & LDR
   CPU0 0x01 → 0    CPU4 0x10 → 0
   CPU1 0x02 → 0    CPU5 0x20 → 0x20  ✓ 선택
   CPU2 0x04 → 0    CPU6 0x40 → 0
   CPU3 0x08 → 0    CPU7 0x80 → 0
```

**정확히 CPU5 하나만 매칭.** 만약 커널이 affinity를 여러 CPU로 잡았다면 dest에 여러 비트가
서고, delivery_mode를 `Lowest Priority(1)`로 두면 QEMU는 `apic_bus_deliver()`에서
"비트마스크의 최하위 CPU"를 고른다(실제 하드웨어의 arbitration은 근사하지 않는다 —
소스 주석 `XXX: search for focus processor, arbitration`).

### 6.4 IRR 세팅과 vCPU 깨우기 (`apic.c:402`)

**소스: QEMU** `hw/intc/apic.c:402`, `:379` — IRR을 세우고 vCPU를 깨운다

```c
static void apic_set_irq(APICCommonState *s, int vector_num, int trigger_mode)
{
    kvm_report_irq_delivered(!apic_get_bit(s->irr, vector_num));

    apic_set_bit(s->irr, vector_num);            /* ← IRR[33] = 1 */
    if (trigger_mode) apic_set_bit(s->tmr, vector_num);   /* level이면 TMR도 */
    else              apic_reset_bit(s->tmr, vector_num); /* edge → TMR 클리어 */
    ...
    apic_update_irq(s);
}

static void apic_update_irq(APICCommonState *s)
{
    CPUState *cpu = CPU(s->cpu);
    if (!qemu_cpu_is_self(cpu)) {
        cpu_interrupt(cpu, CPU_INTERRUPT_POLL);   /* ← 다른 스레드(=장치)에서 온 경우 */
    } else if (apic_irq_pending(s) > 0) {
        cpu_interrupt(cpu, CPU_INTERRUPT_HARD);
    } ...
}
```

우리 흐름은 **QEMU main thread(장치)가 vCPU5의 LAPIC을 건드리는** 경우이므로
`qemu_cpu_is_self()`가 거짓 → `CPU_INTERRUPT_POLL`을 걸어 vCPU5 스레드를 TCG 실행
루프에서 빠져나오게 만든다. vCPU5 스레드가 그 다음 번역블록 경계에서 `apic_poll_irq()`
→ `apic_update_irq()`를 자기 컨텍스트로 다시 돌려 `CPU_INTERRUPT_HARD`로 승격시킨다.
이 hand-off가 §5.4 실측에서 `apic_deliver_irq`(.494008)와 EOI(.494078) 사이의 70 µs다.

우선순위 판정은 `apic_irq_pending()`:

**소스: QEMU** `hw/intc/apic.c:358` — 우선순위 판정

```c
irrv = get_highest_priority_int(s->irr);      /* IRR에서 가장 높은 벡터 */
ppr  = apic_get_ppr(s);                       /* Processor Priority Register */
if (ppr && (irrv & 0xf0) <= (ppr & 0xf0)) return -1;   /* 우선순위 클래스가 낮으면 보류 */
return irrv;
```

**벡터의 상위 4비트가 우선순위 클래스**다. 벡터 33(`0x21`) → 클래스 2.
실측한 CPU5의 `TPR = 0x10`(클래스 1)이므로 `(0x21 & 0xf0)=0x20 > (0x10 & 0xf0)=0x10` →
통과, 즉시 전달된다.

---

## 7. 수령 — CPU가 벡터를 집는다

> **쉬운 말로**: CPU5가 하던 일을 멈추고 접수 장부를 확인하는 단계다.
> 접수(IRR) → 처리중(ISR)으로 도장을 옮기고, 처리하는 동안에는 더 낮은 우선순위의
> 용건을 안 받도록 문턱(PPR)을 올린다. 그 다음 33번 용건 담당 코드로 점프한다.
> **이 순간을 gdb로 얼어붙게 만들어 찍은 게 §7.2다** — 교과서 그림이 실제 값으로 보인다.

### 7.1 `apic_get_interrupt()` — IRR에서 ISR로 (`apic.c:576`)

**소스: QEMU** `hw/intc/apic.c:576` — CPU가 벡터를 집어가는 순간

```c
int apic_get_interrupt(DeviceState *dev)
{
    ...
    intno = apic_irq_pending(s);
    if (intno == 0 || apic_check_pic(s)) { ...; return -1; }
    else if (intno < 0) { ...; return s->spurious_vec & 0xff; }   /* 우선순위에 막힘 → spurious */

    apic_reset_bit(s->irr, intno);      /* IRR[33] = 0 */
    apic_set_bit(s->isr, intno);        /* ISR[33] = 1  ← "서비스 중" */
    apic_sync_vapic(s, SYNC_TO_VAPIC);
    apic_update_irq(s);
    return intno;                       /* → x86 코어가 이 벡터로 IDT 진입 */
}
```

이 시점부터 EOI 전까지 **PPR이 벡터 우선순위 클래스로 올라가서 같은/낮은 클래스의 인터럽트가
막힌다.** (중첩 인터럽트 방지)

### 7.2 실측 — 인터럽트 처리 도중 정지시켜 찍은 LAPIC

게스트 gdb로 `nvme0q6`의 IRQ 흐름 핸들러 진입 직전에 정지시켰다:

**`[driver]`** 게스트 gdb — 인터럽트 처리 도중 VM을 정지시킨다

```
[driver] (gdb) break handle_edge_irq if desc->irq_data.irq == 30
[driver] (gdb) continue
         ... 게스트 콘솔에서: taskset -c 5 dd if=/dev/nvme0n1 of=/dev/null bs=4096 count=1 skip=9000

[driver] Thread 6 hit Breakpoint 1, handle_edge_irq (desc=0xffff8880060a1800) at kernel/irq/chip.c:777
         (Thread 6 = vCPU5. 여기서 게스트 전체가 정지한다)
```

`Thread 6` = vCPU5(gdbstub는 1-base). **이 정지 상태에서** QMP로 LAPIC을 떴다:

```
(qemu) info lapic 5
LVT0     0x00010700 active-hi edge  masked   ExtINT (vec 0)
LVT1     0x00010400 active-hi edge  masked   NMI
LVTERR   0x000000fe active-hi edge           Fixed  (vec 254)      ← ERROR_APIC_VECTOR
LVTT     0x000000ec active-hi edge one-shot  Fixed  (vec 236)      ← LOCAL_TIMER_VECTOR(0xec)
Timer    DCR=0x3 (divide by 16) initial_count = 386556963 current_count = 386517954
SPIV     0x000001ff APIC enabled, focus=off, spurious vec 255      ← SPURIOUS_APIC_VECTOR(0xff)
ICR      0x000008fb logical edge de-assert no-shorthand            ← 직전 IPI(0xfb=CALL_FUNCTION_SINGLE)
ICR2     0x01000000 mask 00000001 (APIC ID)
ESR      0x00000000
ISR      33                                                        ← ★ 벡터 33이 서비스 중
IRR      (none)                                                    ← ★ IRR에서는 이미 빠졌다

APR 0x05 TPR 0x10 DFR 0x0f LDR 0x20 PPR 0x20                        ← ★ PPR이 0x10 → 0x20 상승
```

**`ISR 33` / `IRR (none)` / `PPR 0x20`** — §7.1 코드가 한 일이 그대로 찍혔다.
`LDR 0x20`은 §3.3에서 본 CPU5의 논리 ID이고, MSI 주소의 destid와 일치한다.

같은 순간 백트레이스:

**`[driver]`** 게스트 gdb — 정지 지점의 커널 호출 스택

```
(gdb) bt
#0  handle_edge_irq (desc=0xffff8880060a1800) at kernel/irq/chip.c:777
#1  generic_handle_irq_desc (desc=0xffff8880060a1800) at ./include/linux/irqdesc.h:158
#2  handle_irq (regs=<optimized out>, desc=0xffff8880060a1800) at arch/x86/kernel/irq.c:231
#3  __common_interrupt (regs=<optimized out>, vector=33) at arch/x86/kernel/irq.c:250
#4  common_interrupt (regs=0xffffc900000abe38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```

`vector=33`이 커널 프레임에 그대로 살아 있다.

### 7.3 IDT 게이트 실측

**`[driver]`** 게스트 gdb — 벡터 33의 IDT 게이트

```
(gdb) p/x idt_table[33]
$8 = {offset_low = 0x1f8, segment = 0x10,
      bits = {ist = 0x0, zero = 0x0, type = 0xe, dpl = 0x0, p = 0x1},
      offset_middle = 0x81e0, offset_high = 0xffffffff, reserved = 0x0}

(gdb) p (void*)(((unsigned long)idt_table[33].offset_high<<32) |
                ((unsigned long)idt_table[33].offset_middle<<16) | idt_table[33].offset_low)
$9 = (void *) 0xffffffff81e001f8 <irq_entries_start+8>
```

| 필드 | 값 | 의미 |
|------|-----|------|
| type | `0xe` | 64비트 **Interrupt Gate**(진입 시 IF 자동 클리어) |
| dpl | `0` | 커널만 소프트웨어로 호출 가능 |
| p | `1` | present |
| segment | `0x10` | `__KERNEL_CS` |
| ist | `0` | 별도 인터럽트 스택 미사용 (일반 커널 스택 사용) |
| 진입점 | `irq_entries_start + 8` | 벡터 33 = `FIRST_EXTERNAL_VECTOR(32)` + 1 → 스텁 크기 8바이트 × 1 |

`irq_entries_start`의 각 스텁은 `push $vector; jmp asm_common_interrupt` 두 줄이다.
그래서 **IDT 하나로 200여 개 벡터를 다 받아도 커널은 자기가 몇 번 벡터로 들어왔는지 안다.**

인터럽트로 끊긴 지점(`pt_regs`)도 찍어 봤다:

**`[driver]`** 게스트 gdb — 인터럽트로 끊긴 지점의 레지스터

```
(gdb) p/x *(struct pt_regs *)0xffffc900000abe38
$10 = {..., orig_ax = 0xffffffffffffffff, ip = 0xffffffff81030119, cs = 0x10, flags = 0x246,
       sp = 0xffffc900000abee8, ss = 0x18}

(gdb) p (void*)0xffffffff81030119
$11 = (void *) 0xffffffff81030119 <amd_e400_idle+57>
```

* `ip = amd_e400_idle+57` — **CPU5는 idle 루프(HLT 직후)에서 깨어났다.** 정확히 기대한 그림이다.
* `cs = 0x10` → 커널 모드에서 인터럽트됨.
* `orig_ax = -1` — 진입 스텁이 push했던 벡터 값은 이미 `rsi`(2번째 인자)로 옮겨졌고,
  `entry_64.S`의 `idtentry_body`가 "이건 syscall이 아니다"라는 뜻으로 `-1`을 써넣었다.
* `flags = 0x246` — 저장된 RFLAGS(IF=1이었음; 게이트 진입으로 현재는 IF=0).

---

## 8. 처리 — 커널 인터럽트 핸들러

### 8.1 벡터 → irq_desc

**소스: 리눅스 커널** `arch/x86/kernel/irq.c:240`

```c
/* arch/x86/kernel/irq.c:240 */
DEFINE_IDTENTRY_IRQ(common_interrupt)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	struct irq_desc *desc;

	desc = __this_cpu_read(vector_irq[vector]);      /* ← §3.2에서 뜬 그 배열 */
	if (likely(!IS_ERR_OR_NULL(desc))) {
		handle_irq(desc, regs);                      /* → generic_handle_irq_desc */
	} ...
}
```

CPU5의 `vector_irq[33]` = `irq_desc(irq 30, "nvme0q6")` — §3.2 실측표 그대로다.
정지 상태에서 그 `irq_desc`를 직접 뜯었다:

**`[driver]`** 게스트 gdb — 이 인터럽트의 irq_desc 내용

```
(gdb) p desc->irq_data.irq          → $1 = 30
(gdb) p desc->irq_data.hwirq        → $2 = 65542
(gdb) p desc->irq_data.chip->name   → $3 = "PCI-MSI"
(gdb) p desc->action->name          → $4 = "nvme0q6"
(gdb) p desc->action->handler       → $5 = (irq_handler_t) 0xffffffff81826380 <nvme_irq>
(gdb) p desc->handle_irq            → $6 = (irq_flow_handler_t) 0xffffffff810dc950 <handle_edge_irq>
```

**흐름 핸들러 = `handle_edge_irq`** — MSI/MSI-X는 항상 edge다(§4.2에서 `is_level=0` 확인).
`/proc/interrupts`의 `PCI-MSI 65542-edge nvme0q6`가 이 세 정보를 그대로 표시한 것이다.

### 8.2 EOI가 핸들러보다 **먼저** 온다 (실측으로 증명)

**소스: 리눅스 커널** `kernel/irq/chip.c` — edge 인터럽트 흐름 핸들러

```c
/* kernel/irq/chip.c handle_edge_irq() */
void handle_edge_irq(struct irq_desc *desc)
{
	raw_spin_lock(&desc->lock);
	desc->istate &= ~(IRQS_REPLAY | IRQS_WAITING);
	...
	desc->irq_data.chip->irq_ack(&desc->irq_data);   /* ← ① EOI (여기!) */
	do {
		...
		handle_irq_event(desc);                      /* ← ② nvme_irq() */
	} while (...);
	raw_spin_unlock(&desc->lock);
}
```

`chip->irq_ack` 체인 (모두 이 트리에서 확인):

```
 desc->irq_data.chip = PCI-MSI       arch/x86/kernel/apic/msi.c:153
     .irq_ack = irq_chip_ack_parent  → 부모 도메인으로 위임
 부모 = lapic_controller ("APIC")    arch/x86/kernel/apic/vector.c:909
     .irq_ack = apic_ack_edge        arch/x86/kernel/apic/vector.c:895
         └─ irq_complete_move(cfg)   (affinity 이동 중이면 옛 벡터 정리)
         └─ apic_ack_irq(irqd) → ack_APIC_irq() → LAPIC 오프셋 0xB0에 0 쓰기
```

**실측 A — 트레이스 순서** (§5.4 재인용):

```
[qemu·APIC ] .494008  apic_deliver_irq ... vector 33      ← 배달
[qemu·vCPU5] .494078  apic_mem_writel 0xb0 = 0x00000000   ← EOI (핸들러보다 먼저!)
[qemu·vCPU5] .494286  pci_nvme_mmio_doorbell_cq cqid 6    ← nvme_irq 내부
```

**실측 B — 두 지점의 LAPIC 상태 비교**:

| 정지 지점 | ISR | IRR | PPR |
|-----------|-----|-----|-----|
| `handle_edge_irq` 진입 (ack 전) | **33** | (none) | **0x20** |
| `nvme_irq` 진입 (ack 후) | **(none)** | (none) | **0x10** |

**`[driver]`** 게스트 gdb + **`[qemu]`** `info lapic` — 한 단계 진행한 뒤 다시 확인

```
(gdb) break nvme_irq
(gdb) continue
Thread 6 hit Breakpoint 2, nvme_irq (irq=30, data=0xffff8880043eb600) at drivers/nvme/host/pci.c:1251

(qemu) info lapic 5
ISR      (none)
IRR      (none)
APR 0x05 TPR 0x10 DFR 0x0f LDR 0x20 PPR 0x10
```

`apic_eoi()`(`hw/intc/apic.c:424`)가 ISR 최상위 비트를 지우고 PPR을 되돌린 것이다:

**소스: QEMU** `hw/intc/apic.c:424` — EOI 처리

```c
static void apic_eoi(APICCommonState *s)
{
    int isrv = get_highest_priority_int(s->isr);
    if (isrv < 0) return;
    apic_reset_bit(s->isr, isrv);                               /* ISR[33] = 0 */
    if (!(s->spurious_vec & APIC_SV_DIRECTED_IO) && apic_get_bit(s->tmr, isrv))
        ioapic_eoi_broadcast(isrv);                             /* level(TMR)일 때만 IOAPIC에 전파 */
    ...
    apic_update_irq(s);
}
```

edge라 TMR 비트가 0이므로 IOAPIC 브로드캐스트는 일어나지 않는다.

> **왜 EOI를 먼저 하는가**: MSI는 edge이고 장치는 "쐈으면 끝"이다. EOI를 먼저 해두면
> 핸들러가 CQ를 훑는 동안 도착한 **다음 완료 인터럽트를 잃지 않는다**. level(INTx)에서는
> 반대로 원인을 지우기 전에 EOI를 하면 인터럽트 폭풍이 나므로 `handle_fasteoi_irq`가 쓰인다.

### 8.3 `nvme_irq()` 안에서 본 큐 상태 (실측)

**`[driver]`** 게스트 gdb — nvme_irq() 안에서 본 큐와 CQE

```
(gdb) p ((struct nvme_queue *)data)->qid              → 6
(gdb) p ((struct nvme_queue *)data)->cq_vector        → 6
(gdb) p ((struct nvme_queue *)data)->cq_head          → 0
(gdb) p ((struct nvme_queue *)data)->cq_phase         → 1
(gdb) p/x ((struct nvme_queue *)data)->cqes[0]
$18 = {result = {u32 = 0x0}, sq_head = 0x1, sq_id = 0x6, command_id = 0x0, status = 0x1}
(gdb) p/x ((struct nvme_queue *)data)->dev->bar        → 0xffffc900002da000   (BAR0 ioremap)
(gdb) p/x ((struct nvme_queue *)data)->dev->dbs        → 0xffffc900002db000   (= bar + 0x1000)
(gdb) p/x ((struct nvme_queue *)data)->q_db            → 0xffffc900002db030
```

* `cqes[0].status = 0x1` → **bit0이 phase 비트**이고 `cq_phase=1`과 일치 → "이 CQE는 새 것".
  `status >> 1 == 0` → Successful Completion.
* `sq_id = 6` → 이 완료가 SQ6에서 나왔다는 장치의 자기 신고.
* `q_db = dbs + 0x30` — 큐 6번의 SQ 도어벨 오프셋 `0x1000 + 2*6*4 = 0x1030`.
  CQ 도어벨은 그 +4(`0x1034`). QEMU 트레이스의 `doorbell_cq cqid 6`가 이 주소 쓰기다.
* `bar = 0xffffc900002da000`은 물리 `0xfebf0000`의 ioremap 결과. 즉
  **MSI-X 테이블(`0xfebf2000`)은 커널 가상주소 `0xffffc900002dc000`**으로도 접근 가능하다.

이후 `nvme_poll_cq → nvme_handle_cqe → blk_mq_complete_request → bio_endio`로 이어지는
블록 계층 경로는 [[14_interrupt_and_completion]]에서 다룬다.

---

## 9. 마스킹과 PBA — "지금은 받지 마" 실험

MSI-X 엔트리의 4번째 DWORD(Vector Control) bit0이 **Mask 비트**다. 마스크된 상태에서
장치가 인터럽트를 쏘면 QEMU는 PBA(Pending Bit Array)에 비트만 세우고 조용히 리턴한다
(§5.2 ②). 언마스크되는 순간 밀린 인터럽트를 발사한다.

게스트에서 커널 몰래 테이블을 직접 조작해 재현했다(`devmem`으로 `0xfebf206c` = entry6 + 12):

```sh
# ① 마스크 — 장치 수첩의 entry6 제어필드에 1을 쓴다
          echo 3 > /proc/sys/vm/drop_caches
          devmem 0xfebf206c 32 1
[device]     → vctrl=0x00000001  PBA=0x00000000

# ② 마스크된 채로 CPU5에서 읽기 실행
          taskset -c 5 dd if=/dev/nvme0n1 of=/dev/null bs=4096 count=1 &
[device]     → PBA=0x00000040        ← ★ 장치가 "밀린 인터럽트 있음"을 표시(비트6)
[driver]     → /proc/interrupts nvme0q6 카운트 변화 없음 (483 그대로)
                = 커널은 인터럽트를 한 번도 못 받았다

# ③ 언마스크
          devmem 0xfebf206c 32 0
[device]     → vctrl=0x00000000  PBA=0x00000000     = 밀린 것이 발사되어 비워짐
[driver]     → /proc/interrupts nvme0q6 = 485  (+2, 밀린 것 + 후속 I/O)
```

같은 구간의 QEMU 트레이스:

```
[마스크 구간]
  [qemu·vCPU5] pci_nvme_mmio_doorbell_sq sqid 6 new_tail 517
  [qemu·SSD  ] pci_nvme_enqueue_req_completion cid 45632 cqid 6 dw0 0x0 dw1 0x0 status 0x0
  [qemu·SSD  ] pci_nvme_irq_msix raising MSI-X IRQ vector 6   ← 장치는 분명히 "쏘려고 했다"
  (여기서 끝. apic_deliver_irq 가 없다 = LAPIC까지 가지 못했다)

[언마스크 직후]
  [qemu·vCPU5] pci_nvme_mmio_doorbell_cq cqid 6 new_head 517  ← 핸들러가 드디어 돌았다
```

`pci_nvme_irq_msix`는 찍혔는데 `apic_deliver_irq`가 없다 — **`msix_notify()`가
`msix_is_masked()`에서 걸려 `msix_set_pending()`만 하고 리턴한 것**이 로그로 증명된다.

언마스크가 pending을 발사하는 코드(`hw/pci/msix.c:145`):

**소스: QEMU** `hw/pci/msix.c:145` — 언마스크 시 밀린 인터럽트 발사

```c
static void msix_handle_mask_update(PCIDevice *dev, int vector, bool was_masked)
{
    bool is_masked = msix_is_masked(dev, vector);
    if (is_masked == was_masked) return;
    msix_fire_vector_notifier(dev, vector, is_masked);
    if (!is_masked && msix_is_pending(dev, vector)) {
        msix_clr_pending(dev, vector);
        msix_notify(dev, vector);          /* ← 밀렸던 인터럽트 발사 */
    }
}
```

PBA 비트 위치 계산(`msix.c:143`):

**소스: QEMU** `hw/pci/msix.c:143` — PBA 비트 위치 계산

```c
static uint8_t  msix_pending_mask(int vector)          { return 1 << (vector % 8); }
static uint8_t *msix_pending_byte(PCIDevice *dev, int v){ return dev->msix_pba + v / 8; }
```

벡터 6 → 바이트 0, 비트 6 → `0x40`. **실측 `PBA=0x00000040`과 정확히 일치.**

> 커널은 정상 경로에서 이 마스크 비트를 `pci_msix_mask_irq()`/`pci_msix_unmask_irq()`로
> 조작한다(예: affinity 변경 중 stray interrupt 방지). 위 실험은 그 하드웨어 동작을
> 사람이 손으로 재현한 것이다.

---

## 10. 결정적 검증 — CPU가 직접 `0xFEE20004`에 써보기

MSI가 "특별한 신호"가 아니라 **그냥 메모리 쓰기**라면, CPU가 같은 주소에 같은 값을 써도
똑같이 인터럽트가 나야 한다. 게스트에서 `/dev/mem`을 통해 직접 해봤다:

```sh
[driver] [before]  30:  0  0  0  0  0  2  0  0   PCI-MSI 65542-edge  nvme0q6
                                    ↑ CPU5 열

         devmem 0xfee20004 32 0x21     ← CPU가 직접, LAPIC의 MSI 창에 4바이트 쓰기

[driver] [after ]  30:  0  0  0  0  0  3  0  0   PCI-MSI 65542-edge  nvme0q6
                                    ↑ 2 → 3 으로 증가
```

**CPU5의 `nvme0q6` 카운트가 2 → 3으로 증가했다.** 커널은 이것이 SSD가 보낸 것인지
옆 CPU가 장난친 것인지 구분할 방법이 없다.

같은 순간의 QEMU 트레이스를, 진짜 장치가 쏜 것과 나란히 놓으면:

```
[qemu·SSD  ] 2703852@1788927128.494008:apic_deliver_irq dest 32 dest_mode 1 delivery_mode 0 vector 33 trigger_mode 0
              ↑ tid 2703852 = QEMU main thread = 에뮬레이션된 SSD가 DMA write로 쏜 것

[qemu·vCPU6] 2703865@1788927187.828197:apic_deliver_irq dest 32 dest_mode 1 delivery_mode 0 vector 33 trigger_mode 0
              ↑ tid 2703865 = vCPU6 = busybox devmem 프로세스가 그냥 store 명령으로 쓴 것
```

**두 줄이 완전히 동일하다.** 발신자만 다르고 LAPIC이 보는 것은 똑같다.

이 실험이 말해주는 것:

1. MSI-X 주소는 **DMA 주소 공간에서의 목적지**일 뿐이다. 장치는 "인터럽트를 건다"가 아니라
   "약속된 주소에 약속된 4바이트를 쓴다"만 한다.
2. 그래서 **IOMMU가 없으면 어떤 PCIe 장치든 아무 CPU에게 아무 벡터나 쏠 수 있다.**
   Interrupt Remapping(VT-d IR / AMD IOMMU)이 존재하는 이유가 이것이다
   (이 게스트는 `CONFIG_IRQ_REMAP=n`, IOMMU 없음 → 원본 형식 그대로 사용).
3. 반대로 **잘못된 주소를 테이블에 쓰면 인터럽트가 그냥 메모리 한 칸을 덮어쓴다**.
   `__irq_msi_compose_msg()`의 `WARN_ON_ONCE(cfg->dest_apicid > 0xFF)` 주석이
   "IOMMU만이 상위 주소 비트에 APIC ID를 넣는 트릭을 쓸 수 있다. 다른 것이 그러면
   그냥 메모리에 쓰는 것이 된다"라고 명시하는 이유다.

---

## 11. 전체 타임라인 한 장

```
 표기:  [HW] = 하드웨어(장치/LAPIC)가 하는 일    [SW] = 리눅스 커널이 하는 일
        근거 뒤의 [qemu]/[driver]/[device]는 그 사실을 어디서 확인했는지를 뜻한다.

 t(µs)              무슨 일이 일어나는가                          어떻게 확인했나
 ─────────────────────────────────────────────────────────────────────────────────────────
   0   [SW] vCPU5   dd → blk-mq hctx5 → SQ6(0x4fb0000)에 커맨드 기록
   0   [SW] vCPU5   BAR0+0x1030에 tail=2 쓰기 (SQ doorbell)
                    → 이 MMIO 쓰기가 장치를 깨운다                [qemu] pci_nvme_mmio_doorbell_sq

 746   [HW] SSD     블록 read 완료 → CQ6(0x60bc000)에 CQE DMA write
                    {sq_id=6, cid=4096, status=0x1(phase 포함)}   [qemu] pci_nvme_enqueue_req_completion
                                                                  [driver] gdb: cqes[0]

 769   [HW] SSD     nvme_irq_assert → msix_notify(dev, vector=6)  [qemu] pci_nvme_irq_msix vector 6
                    ├ entry6이 쓰이는 중인가? yes
                    ├ entry6이 마스크됐나? no (vctrl=0)
                    └ entry6 읽기 → {0xFEE20004, 0x21}            [device] devmem 0xfebf2060

 769   [HW] SSD     그 주소에 그 값을 쓴다 = PCIe MemWr TLP
                    address_space_stl_le(bus_master_as,
                                         0xFEE20004, 0x21)

 775   [HW] LAPIC   0xFEE00000 창이 받아서 해석                    [qemu] apic_deliver_irq
                    dest=0x20 dest_mode=1 delivery=0 vec=33         dest 32 ... vector 33
                    ├ dest 0x20 & 각 CPU의 LDR → CPU5만 매칭       [qemu] info lapic 5: LDR 0x20
                    └ CPU5의 IRR[33] = 1, vCPU5 깨우기

 845   [HW] CPU5    인터럽트 수락: IRR[33]=0 → ISR[33]=1           [qemu] info lapic 5:
                    PPR 0x10 → 0x20 (더 낮은 우선순위 차단)          ISR 33 / PPR 0x20
                                                                  ← 게스트 gdb로 정지시켜 촬영

 845   [HW] CPU5    IDT[33] = irq_entries_start+8 로 점프          [driver] gdb: p/x idt_table[33]
                    → push $33 → asm_common_interrupt

 845   [SW] 커널    common_interrupt(regs, vector=33)              [driver] gdb: bt (vector=33)
                    → this_cpu vector_irq[33] = irq_desc(irq 30)   [driver] drgn: per-CPU vector_irq
                    → handle_edge_irq()                            [driver] gdb: desc->handle_irq

 845   [SW] 커널    chip->irq_ack → ack_APIC_irq()                 [qemu] apic_mem_writel 0xb0
                    → LAPIC 0xB0 쓰기 → ISR[33]=0, PPR→0x10        [qemu] info lapic 5: ISR (none)
                    ★ 핸들러보다 EOI가 먼저다

1053   [SW] 커널    nvme_irq → nvme_poll_cq → CQE phase 검사        [driver] gdb: cqes[0].status=0x1
                    → CQ6 doorbell(BAR0+0x1034) head=2             [qemu] pci_nvme_mmio_doorbell_cq
                    → blk_mq_complete_request → bio_endio
                    → dd 깨어남
```

---

## 12. 대비군 — 같은 시간대의 IOAPIC 경로(ttyS0)

같은 트레이스 구간에 이런 줄이 대량으로 섞여 있었다:

**`[qemu]`** `-trace` — 같은 시간대에 섞여 있던 IOAPIC 인터럽트(ttyS0)

```
apic_deliver_irq dest 1 dest_mode 1 delivery_mode 0 vector 34 trigger_mode 0
```

`dest 1` = CPU0의 LDR, `vector 34` = CPU0의 `vector_irq[34]` = **irq4 (ttyS0)**.
이건 MSI가 아니라 **IOAPIC**이 보낸 것이다(직렬 콘솔 출력).

| | MSI-X (nvme0q6) | IOAPIC (ttyS0) |
|---|---|---|
| 발신 주체 | 장치가 직접 메모리 쓰기 | 장치가 INTx 핀 assert → IOAPIC이 redirection table 참조 후 전달 |
| 목적지/벡터 결정 위치 | **장치 안의 MSI-X 테이블** | **IOAPIC 안의 redirection table** |
| 벡터 개수 | 함수당 최대 2048 | 핀당 1개, 여러 장치가 공유 가능 |
| trigger | edge 고정(우리 케이스) | level/edge 설정 가능 |
| EOI | LAPIC EOI만 | LAPIC EOI + (level이면) IOAPIC EOI 브로드캐스트 |
| 커널 흐름 핸들러 | `handle_edge_irq` | edge면 `handle_edge_irq`(ttyS0=IRQ4가 이 경우), level이면 `handle_fasteoi_irq` |
| QEMU 코드 경로 | `msix_notify → msi_send_message → address_space_stl_le` | `ioapic_set_irq → ioapic_service → stl_le_phys` |

**두 경로가 QEMU 안에서 실제로 같은 방식으로 합류한다.** `hw/intc/ioapic.c:134`의 주석이
이를 명시한다:

**소스: QEMU** `hw/intc/ioapic.c:134`

```c
/* No matter whether IR is enabled, we translate
 * the IOAPIC message into a MSI one, and its
 * address space will decide whether we need a
 * translation. */
stl_le_phys(ioapic_as, info.addr, info.data);
```

즉 IOAPIC조차도 **redirection table 엔트리를 MSI 메시지(addr/data)로 번역해서
`0xFEE00000` 창에 4바이트를 쓴다.** 결국 `apic_mem_write → apic_send_msi →
apic_deliver_irq`라는 동일한 경로를 탄다. LAPIC 입장에선 "누가 보냈든 (dest, vector)가
전부"다 — §10에서 CPU가 직접 써도 똑같았던 이유와 같은 원리다.

---

## 13. QEMU 근사와 실제 하드웨어의 차이 (읽을 때 감안할 것)

| 항목 | 실제 하드웨어 | 이 QEMU 실험 |
|------|---------------|--------------|
| MSI 전달 매체 | PCIe MemWr TLP가 Root Complex를 거쳐 인터럽트 컨트롤러로 | `address_space_stl_le()` 함수 호출 (동기) |
| LAPIC MMIO vs MSI 구분 | CPU 내부 접근 / 시스템 버스 접근으로 물리적 구분 | 같은 MemoryRegion에서 **주소 오프셋으로** 구분 (`addr > 0xfff \|\| !index`) |
| Lowest Priority 중재 | APR/TPR 기반 arbitration | `apic_bus_deliver()`가 비트마스크 최하위 CPU 선택 (소스 주석 `XXX`) |
| Redirection Hint | 지원 | `apic_send_msi()`에서 **무시**(`/* XXX: Ignore redirection hint. */`) |
| 지연 시간 | MSI write → 인터럽트 수락 수백 ns | TCG에서 수십~수백 µs (스레드 hand-off 포함) |
| 인터럽트 도착 순서 | CQE DMA write와 MSI write의 순서가 PCIe ordering으로 보장 | 같은 스레드에서 순차 실행되므로 자동 보장 |
| x2APIC | 최신 시스템은 x2APIC + IR이 기본 | `CONFIG_X86_X2APIC=n` → **xAPIC flat logical**. 그래서 destid가 8비트 비트마스크로 보인다 |

특히 마지막 항목이 중요하다. **x2APIC + Interrupt Remapping 환경에서는 MSI 주소가
`0xFEE00000 | (handle << 5) | ...` 형태의 "IRTE 인덱스"로 바뀌고, 실제 벡터/목적지는
IOMMU의 Interrupt Remapping Table에 들어간다.** 이 실험에서 본 "주소에 CPU가 직접 적혀 있는"
형태는 IR이 없는 환경(및 대부분의 가상머신)의 모습이다.

---

## 14. 재현 스크립트 모음

### 14.1 MSI-X 테이블 전체 덤프 (게스트, busybox만 필요)

**`[device]`** 재현 스크립트 — 게스트에서 장치 수첩 통째로 읽기

```sh
D=/sys/bus/pci/devices/0000:00:04.0
# BAR0 물리주소
BAR=$(sed -n 1p $D/resource | cut -d' ' -f1)          # 0x00000000febf0000
# MSI-X capability에서 테이블 오프셋 읽기 (cap ptr가 0x40인 경우)
hexdump -C $D/config | sed -n '5p'                     # 0x40 행
# 테이블 덤프 (오프셋 0x2000 가정)
i=0; while [ $i -le 8 ]; do
  b=$((0xfebf2000 + i*16))
  printf "entry%d: addr_lo=%s addr_hi=%s data=%s vctrl=%s\n" $i \
    "$(devmem $b 32)" "$(devmem $((b+4)) 32)" "$(devmem $((b+8)) 32)" "$(devmem $((b+12)) 32)"
  i=$((i+1))
done
# PBA
devmem 0xfebf3000 32
```

> 전제: `CONFIG_DEVMEM=y`, `CONFIG_IO_STRICT_DEVMEM=n`. (`CONFIG_STRICT_DEVMEM=y`여도
> MMIO 영역은 허용된다 — RAM만 막힌다.)

### 14.2 커널 쪽 msi_desc / 벡터 매핑 (호스트에서 drgn)

**`[driver]`** 재현 스크립트 — 커널 장부 통째로 읽기

```python
# drgn --qemu qmp.sock -s linux-6.1.4/vmlinux  script.py
from drgn.helpers.linux.radixtree import radix_tree_lookup
from drgn.helpers.linux.percpu import per_cpu
from drgn import cast

tree = prog['irq_desc_tree'].address_of_()
for irq in range(24, 33):
    desc = cast('struct irq_desc *', radix_tree_lookup(tree, irq))
    md = desc.irq_common_data.msi_desc
    m = md.msg
    print(f"irq={irq} {desc.action.name.string_().decode()} "
          f"addr_lo=0x{m.address_lo.value_():08x} data=0x{m.data.value_():08x} idx={md.msi_index.value_()}")
    d = desc.irq_data.address_of_()
    while d:
        if d.chip and d.chip.name.string_() == b'APIC':
            acd = cast('struct apic_chip_data *', d.chip_data)
            print(f"   vector={acd.vector.value_()} cpu={acd.cpu.value_()} "
                  f"dest_apicid=0x{acd.hw_irq_cfg.dest_apicid.value_():x} managed={acd.is_managed.value_()}")
        d = d.parent_data

for c in range(8):                              # per-CPU 벡터 테이블
    vi = per_cpu(prog['vector_irq'], c)
    row = []
    for v in range(32, 41):
        if vi[v].value_():
            de = cast('struct irq_desc *', vi[v])
            row.append(f"v{v}:irq{de.irq_data.irq.value_()}({de.action.name.string_().decode()})")
    print(f"cpu{c}: " + "  ".join(row))
```

### 14.3 인터럽트 순간 정지시켜 LAPIC 찍기

**`[driver]`** + **`[qemu]`** 재현 스크립트 — 인터럽트 도중 정지시켜 LAPIC 찍기

```bash
# 터미널 A: 게스트 gdb
gdb -q linux-6.1.4/vmlinux -ex 'target remote :1234'
(gdb) break handle_edge_irq if desc->irq_data.irq == 30     # 30 = nvme0q6
(gdb) continue

# 터미널 B: 게스트 콘솔에서 I/O 유발
echo 3 > /proc/sys/vm/drop_caches
taskset -c 5 dd if=/dev/nvme0n1 of=/dev/null bs=4096 count=1 skip=9000

# 브레이크 걸린 뒤, 터미널 C: QMP로 LAPIC 스냅샷
python3 qmp.py qmp.sock '{"execute":"human-monitor-command",
                          "arguments":{"command-line":"info lapic 5"}}'
```

핵심: **QEMU 프로세스는 살아 있고 VM만 정지**하므로 QMP/HMP는 정상 응답한다.
`break nvme_irq`로 한 단계 더 진행시킨 뒤 다시 찍으면 EOI 전/후 ISR 차이를 볼 수 있다.

### 14.4 런타임에 트레이스 이벤트 켜고 끄기 (부팅 로그 폭발 방지)

**`[qemu]`** 재현 스크립트 — 트레이스 이벤트 런타임 on/off

```bash
# APIC 이벤트는 타이머 틱까지 잡아서 매우 시끄럽다. 필요한 순간에만 켠다.
python3 qmp.py qmp.sock \
  '{"execute":"trace-event-set-state","arguments":{"name":"apic_deliver_irq","enable":true}}' \
  '{"execute":"trace-event-set-state","arguments":{"name":"apic_mem_writel","enable":true}}'
# ... I/O 유발 ...
python3 qmp.py qmp.sock \
  '{"execute":"trace-event-set-state","arguments":{"name":"apic_deliver_irq","enable":false}}' \
  '{"execute":"trace-event-set-state","arguments":{"name":"apic_mem_writel","enable":false}}'
```

부팅부터 켜둘 이벤트는 `-trace events=<파일>`로 준다. 이번에 쓴 목록:

**`[qemu]`** 부팅부터 켜둔 트레이스 이벤트 목록

```
msix_write_config          pci_nvme_irq_msix           pci_nvme_irq_pin
pci_nvme_irq_masked        pci_nvme_mmio_doorbell_cq   pci_nvme_mmio_doorbell_sq
pci_nvme_enqueue_req_completion   pci_nvme_create_cq   pci_nvme_create_sq
pci_nvme_mmio_intm_set     pci_nvme_mmio_intm_clr      pci_nvme_mmio_cfg
pci_nvme_mmio_aqattr       pci_nvme_mmio_asqaddr       pci_nvme_mmio_acqaddr
pci_nvme_mmio_start_success
```

> `msix_table_mmio_write`에는 트레이스 이벤트가 없다(QEMU 8.2 기준).
> 테이블에 무엇이 쓰였는지는 §14.1처럼 **사후에 테이블을 읽거나**, 커널 쪽
> `msi_desc.msg`(§14.2)로 확인하는 편이 확실하다.

### 14.5 LAPIC 레지스터 오프셋 치트시트 (`apic_mem_writel` 해독용)

| 오프셋 | 레지스터 | 이번 트레이스에서 본 값 |
|--------|----------|------------------------|
| `0x0B0` | EOI | `0x00000000` (항상 0을 쓴다) |
| `0x0D0` | LDR | 부팅 시 `1<<cpu << 24` |
| `0x0E0` | DFR | `0xf0000000` (flat) |
| `0x0F0` | SPIV | `0x1ff` (APIC enable + spurious vec 255) |
| `0x300` | ICR low | `0x000008fb` = logical, fixed, vec `0xfb`(`CALL_FUNCTION_SINGLE_VECTOR`) |
| `0x310` | ICR high | `0x01000000` → 목적지 논리 ID `0x01`(CPU0) |
| `0x320` | LVT Timer | `0x000000ec`(vec 236 = `LOCAL_TIMER_VECTOR`), `0x000100ec`는 masked |
| `0x380` | Timer Initial Count | oneshot 타이머 재장전 (idle 진입 시 자주 보임) |
| `0x3E0` | Divide Configuration | `0x3` = divide by 16 |

x86 시스템 벡터 (`arch/x86/include/asm/irq_vectors.h`):
`0x20` `IRQ_MOVE_CLEANUP` / `0xec` `LOCAL_TIMER` / `0xfb` `CALL_FUNCTION_SINGLE` /
`0xfc` `CALL_FUNCTION` / `0xfd` `RESCHEDULE` / `0xfe` `ERROR_APIC` / `0xff` `SPURIOUS_APIC`.
**디바이스가 쓸 수 있는 범위는 `0x21`~`0xeb`.**

---

## 15. 부록 — 부팅 시퀀스 실측 (펌웨어 → 커널)

트레이스 맨 앞을 보면 NVMe 컨트롤러가 **두 번** 초기화된다.

**`[qemu]`** `-trace` — 컨트롤러가 두 번 초기화되는 구간

```
[qemu·vCPU0] 2703859@1788926780.257175:pci_nvme_mmio_cfg wrote MMIO, config controller config=0x0
2703859@1788926780.257767:pci_nvme_mmio_aqattr wrote MMIO, admin queue attributes=0xff003f
2703859@1788926780.257802:pci_nvme_mmio_asqaddr ... =0x7ffdd000
2703859@1788926780.257868:pci_nvme_mmio_cfg wrote MMIO, config controller config=0x460001
2703852@1788926780.258685:pci_nvme_irq_pin pulsing IRQ pin                 ← ★ INTx
2703852@1788926780.259152:pci_nvme_create_cq ... cqid=1, vector=0, qsize=255, qflags=1, ien=0
   ... (INTx 펄스 수십 회) ...
2703866@1788926783.173273:msix_write_config dev nvme enabled 1 masked 1    ← ★ 여기서부터 Linux
2703866@1788926783.176735:pci_nvme_mmio_aqattr ... =0x1f001f
2703852@1788926783.180158:pci_nvme_irq_msix raising MSI-X IRQ vector 0     ← ★ 첫 MSI-X
2703863@1788926783.180456:pci_nvme_mmio_doorbell_cq cqid 0 new_head 1      ← vCPU4가 처리
```

| 구간 | 주체 | 근거 |
|------|------|------|
| `780.257`~`780.27` | **펌웨어(SeaBIOS) NVMe 드라이버** | 게스트 커널 dmesg의 `nvme nvme0: pci function` 은 `[1.79]`인데, 그 시점의 벽시계는 `783.17`대다. `780.25`대는 커널 부팅 이전 |
| | | `create_cq ... ien=0` — **인터럽트를 끄고 폴링**한다(부트로더 전형) |
| | | 그럼에도 `pci_nvme_irq_pin`이 뜨는 이유: admin CQ는 QEMU가 항상 `irq_enabled=1`로 만들며, MSI-X가 아직 꺼져 있어 레거시 INTx 핀을 편다. 펌웨어는 그냥 무시 |
| `783.17`~ | **Linux nvme 드라이버** | `msix_write_config`로 MSI-X를 켜고, AQA를 `0x1f001f`(32엔트리)로 다시 잡고, `create_cq ... ien=1, qsize=1023`로 I/O 큐 8개 생성 |

`msix_write_config` 5줄이 §3.1의 2단계 벡터 할당(admin 1개 → free → 9개)과 정확히 대응한다.

---

## 16. 요약 — 질문에 대한 직답

> "SSD가 host에서 정해준 msix table의 주소로 interrupt를 발생시키는 전체 과정"

1. **주소를 정하는 쪽은 커널**이다. 커널은 (a) 목적지 CPU를 고르고(`flat` APIC → `1<<cpu`),
   (b) 그 CPU에서 비어 있는 벡터를 잡고(33 또는 34), (c) `__irq_msi_compose_msg()`로
   `0xFEE00000 | (destid<<12) | (logical<<2)`와 `vector | delivery_mode<<8`을 조립한다.
2. **주소를 적어두는 곳은 장치 안**이다. `__pci_write_msi_msg()`의 `writel` 3번이
   BAR0+0x2000+16×idx에 (addr_lo, addr_hi, data)를 기록한다. 이번 실측에서 큐6은
   **`0xFEE20004` / `0x00000021`**.
3. **인터럽트를 쏘는 행위**는 그 주소에 그 데이터를 쓰는 4바이트 DMA write 한 번이다
   (`msix_notify → msi_send_message → address_space_stl_le`). 마스크돼 있으면 대신
   PBA 비트만 선다.
4. **Local APIC**은 그 쓰기를 받아 주소에서 목적지(`0x20`)와 모드(logical)를,
   데이터에서 벡터(33)와 전달 모드(fixed/edge)를 뽑아, LDR이 매칭되는 CPU5의
   **IRR[33]을 세우고** vCPU를 깨운다.
5. **CPU5**는 인터럽트를 수락하면서 IRR[33]→ISR[33]으로 옮기고 PPR을 0x20으로 올린 뒤
   IDT[33](`irq_entries_start+8`)로 점프한다.
6. **커널**은 `common_interrupt(vector=33)` → `this_cpu(vector_irq)[33]` → `irq_desc(irq 30)`
   → `handle_edge_irq()`에서 **먼저 EOI**(LAPIC 0xB0)를 쳐 ISR을 비우고,
   그 다음 `nvme_irq()`를 호출해 CQ를 훑고 CQ 도어벨을 울린다.

이 6단계의 모든 숫자가 §1 표와 §11 타임라인에 실측값으로 정리돼 있다.

---

## 관련 문서

- [[14_interrupt_and_completion]] — 인터럽트 이후 블록 계층 완료 경로(softirq, IPI, `bio_endio`)
- [[04_pcie_doorbell_and_completion]] — 도어벨/CQE/phase 비트의 상세
- [[05_pcie_bar_mmio_connection]] — BAR ioremap과 MMIO 접근 경로
- [[09_pcie_config_space_and_ecam]] — config space와 capability 체인 읽는 법
- [[06_qemu_nvme_device_emulation]] — QEMU NVMe 디바이스 모델 전반
- [[08_qemu_pcie_nvme_connection]] — QEMU PCI 버스/BAR 등록 경로
- [[drgn-live-blkmq-nvme-pcie-prp]] — 같은 게스트에서 drgn으로 blk-mq/PRP를 뜨는 방법
- [[13_dma_and_iommu]] — MSI 주소가 IOMMU를 만나면(Interrupt Remapping) 어떻게 바뀌는지
