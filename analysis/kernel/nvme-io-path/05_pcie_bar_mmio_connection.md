# PCIe BAR와 MMIO: NVMe 디바이스 레지스터가 커널 코드와 연결되는 원리

> 이 문서는 "NVMe 컨트롤러의 레지스터 영역이 어떻게 커널의 `readl()`/`writel()`과 연결되는가?"를 하드웨어 수준부터 코드 수준까지 연결하여 설명한다.

---

## 1. 큰 그림: 디바이스 레지스터에 접근하는 전체 흐름

```
  ┌───────────────────────────────────────────────────────────────────────┐
  │                        CPU (x86_64)                                   │
  │                                                                       │
  │   커널 코드: writel(value, dev->bar + NVME_REG_CC)                    │
  │              ~~~~~~~~~~~  ~~~~~~~~~~~~~~~~~~~~~~~~~~                  │
  │              값 (32bit)    가상 주소 (커널 공간)                       │
  │                    │                                                  │
  │                    ▼                                                  │
  │   ┌─────────────────────────────────────────────┐                    │
  │   │            MMU (Memory Management Unit)      │                    │
  │   │                                              │                    │
  │   │   가상 주소 → 물리 주소 변환                  │                    │
  │   │   이 물리 주소는 RAM이 아니라                 │                    │
  │   │   PCIe BAR에 매핑된 MMIO 영역!               │                    │
  │   └──────────────────┬──────────────────────────┘                    │
  │                      │                                                │
  │                      ▼                                                │
  │   ┌─────────────────────────────────────────────┐                    │
  │   │      CPU의 메모리 컨트롤러 / IOMMU           │                    │
  │   │                                              │                    │
  │   │   "이 물리 주소는 RAM이 아니라                │                    │
  │   │    PCIe 영역이다"                             │                    │
  │   │   → PCIe Root Complex로 라우팅               │                    │
  │   └──────────────────┬──────────────────────────┘                    │
  └──────────────────────┼────────────────────────────────────────────────┘
                         │
                         │  PCIe Memory Write TLP
                         │  (Transaction Layer Packet)
                         ▼
  ┌──────────────────────────────────────────────────────────────────────┐
  │                    PCIe Root Complex                                  │
  │                                                                      │
  │   물리 주소를 PCIe BDF (Bus:Device:Function)로 라우팅                 │
  │   PCIe 스위치를 거쳐 해당 디바이스에 TLP 전달                         │
  └──────────────────────┬───────────────────────────────────────────────┘
                         │
                         │  PCIe Link (Gen3: 8GT/s, Gen4: 16GT/s, Gen5: 32GT/s)
                         ▼
  ┌──────────────────────────────────────────────────────────────────────┐
  │                    NVMe SSD Controller                                │
  │                                                                      │
  │   ┌────────────────────────────────────────────────────────────┐     │
  │   │                    BAR0 레지스터 영역                       │     │
  │   │                                                            │     │
  │   │  0x0000: CAP   (Controller Capabilities)     ← 읽기 전용  │     │
  │   │  0x0008: VS    (Version)                     ← 읽기 전용  │     │
  │   │  0x000C: INTMS (Interrupt Mask Set)                        │     │
  │   │  0x0010: INTMC (Interrupt Mask Clear)                      │     │
  │   │  0x0014: CC    (Controller Configuration)    ← 읽기/쓰기  │     │
  │   │  0x001C: CSTS  (Controller Status)           ← 읽기 전용  │     │
  │   │  0x0020: NSSR  (NVM Subsystem Reset)                       │     │
  │   │  0x0024: AQA   (Admin Queue Attributes)      ← 읽기/쓰기  │     │
  │   │  0x0028: ASQ   (Admin SQ Base Address)       ← 읽기/쓰기  │     │
  │   │  0x0030: ACQ   (Admin CQ Base Address)       ← 읽기/쓰기  │     │
  │   │  ...                                                       │     │
  │   │  0x1000: SQ0 Tail Doorbell                   ← 쓰기 전용  │     │
  │   │  0x1004: CQ0 Head Doorbell (DSTRD=0 기준)    ← 쓰기 전용  │     │
  │   │  0x1008: SQ1 Tail Doorbell                   ← 쓰기 전용  │     │
  │   │  0x100C: CQ1 Head Doorbell                   ← 쓰기 전용  │     │
  │   │  ...                                                       │     │
  │   └────────────────────────────────────────────────────────────┘     │
  │                                                                      │
  │   TLP를 받으면:                                                      │
  │   - 오프셋으로 어떤 레지스터인지 판별                                 │
  │   - Doorbell이면 → 큐 처리 로직 트리거                               │
  │   - CC.EN이면 → 컨트롤러 활성화/비활성화                             │
  └──────────────────────────────────────────────────────────────────────┘
```

**핵심 포인트**: `writel(value, addr)`은 일반 메모리 쓰기가 아니다. CPU가 MMU를 통해 물리 주소를 확인하면, 그 주소가 RAM이 아니라 PCIe 디바이스의 BAR 영역이라는 것을 알게 되고, Memory Write TLP로 변환되어 PCIe 버스를 통해 NVMe 컨트롤러까지 전달된다.

---

## 2. PCIe BAR란 무엇인가?

### 2.1 BAR (Base Address Register)의 개념

PCIe 디바이스는 자신의 내부 레지스터나 메모리를 호스트 CPU에 노출하기 위해 **BAR**를 사용한다. BAR는 "내 레지스터들을 호스트 물리 주소 공간의 이 위치에 매핑해달라"고 시스템에 요청하는 메커니즘이다.

```
  호스트 물리 주소 공간 (예: 64비트 시스템)
  ┌──────────────────────┐  0x0000_0000_0000_0000
  │                      │
  │       RAM             │  0x0000_0000 ~ 0x0003_FFFF_FFFF (16GB 예시)
  │                      │
  ├──────────────────────┤
  │   예약 영역           │
  ├──────────────────────┤
  │                      │
  │  PCIe MMIO 영역      │  ← BIOS/UEFI가 여기에 BAR를 매핑
  │                      │
  │  ┌────────────────┐  │
  │  │ NVMe BAR0      │  │  예: 0x0000_00FB_0000_0000 ~ +16KB
  │  │ (컨트롤러 레지 │  │      (실제 주소는 시스템마다 다름)
  │  │  스터 + Doorbell)│  │
  │  └────────────────┘  │
  │  ┌────────────────┐  │
  │  │ GPU BAR0       │  │  예: 0x0000_00FC_0000_0000 ~ +256MB
  │  │ (VRAM/MMIO)    │  │
  │  └────────────────┘  │
  │  ┌────────────────┐  │
  │  │ NIC BAR0       │  │  예: 0x0000_00FD_0000_0000 ~ +64KB
  │  └────────────────┘  │
  │                      │
  └──────────────────────┘
```

### 2.2 BAR 할당 과정 (부팅 시)

```
  ┌─────────────┐     ┌──────────────┐     ┌──────────────┐
  │  BIOS/UEFI  │────▶│ PCIe 버스    │────▶│  NVMe SSD    │
  │             │     │  열거        │     │  (BAR0)      │
  └─────────────┘     └──────────────┘     └──────────────┘

  1. BIOS가 PCIe 버스를 스캔하여 모든 디바이스를 발견
  2. 각 디바이스의 BAR에 0xFFFFFFFF를 쓰고 다시 읽어서 필요한 크기를 파악
     - 예: BAR0를 읽었더니 0xFFFF_C000 → 하위 14비트가 0 → 16KB 필요
  3. BIOS가 물리 주소 공간에서 빈 영역을 찾아 BAR에 기록
     - 예: BAR0 ← 0xFB000000 (이 디바이스의 레지스터는 물리주소 0xFB000000부터 시작)
  4. 이제 CPU가 0xFB000000에 메모리 접근하면 → NVMe 컨트롤러의 레지스터에 접근
```

### 2.3 NVMe에서 BAR의 의미

NVMe 스펙은 BAR0에 **컨트롤러 레지스터**를 매핑하도록 규정한다:

| BAR | 용도 | 크기 |
|-----|------|------|
| BAR0 (+ BAR1) | 컨트롤러 레지스터 + Doorbell | 최소 16KB (0x1000 + 큐 수 × 8바이트) |
| BAR2 (+ BAR3) | MSIX Table (선택) | 디바이스 의존 |
| BAR4 (+ BAR5) | CMB (Controller Memory Buffer, 선택) | 디바이스 의존 |

BAR0의 크기는 큐 개수에 따라 달라진다:
- 컨트롤러 레지스터: 0x0000 ~ 0x0FFF (4KB)
- Doorbell 영역: 0x1000 ~ (0x1000 + 큐 수 × 2 × 4바이트)
  - 각 큐 페어마다 SQ Tail DB + CQ Head DB = 8바이트 (DSTRD=0 기준)

---

## 3. Linux 커널에서 BAR를 MMIO로 매핑하는 과정

### 3.1 단계별 흐름

```
  nvme_probe()                                    드라이버 진입점
      │
      ▼
  nvme_dev_map()                                  BAR0 물리 영역 확보
      │
      ├── pci_request_mem_regions(pdev, "nvme")   ① 이 BAR 영역을 "nvme" 드라이버 전용으로 예약
      │                                             다른 드라이버가 접근 못하게 막음
      │
      └── nvme_remap_bar(dev, size)               ② BAR0를 커널 가상 주소로 매핑
              │
              ├── pci_resource_start(pdev, 0)      BAR0의 물리 주소 얻기 (BIOS가 할당한 주소)
              │     → 예: 0x00000000FB000000
              │
              ├── pci_resource_len(pdev, 0)         BAR0의 전체 크기 얻기
              │     → 예: 0x4000 (16KB)
              │
              └── ioremap(phys_addr, size)          ③ 물리 주소 → 커널 가상 주소 매핑
                    → 예: 0xFFFF_A000_FB00_0000 (커널 가상 주소)

                    이 주소가 dev->bar에 저장됨!
```

### 3.2 실제 커널 코드 (pci.c)

```c
/* ===== nvme_dev_map() ===== */
static int nvme_dev_map(struct nvme_dev *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);

    /* ① BAR0 메모리 영역을 이 드라이버 전용으로 예약한다.
     *    /proc/iomem에서 "nvme"로 표시되게 된다.
     *    다른 드라이버가 같은 BAR에 접근하려고 하면 실패한다. */
    if (pci_request_mem_regions(pdev, "nvme"))
        return -ENODEV;

    /* ② 최소 크기(0x1000 + 4096 = 8KB)로 BAR0를 매핑한다.
     *    0x1000 = NVME_REG_DBS (Doorbell 시작 오프셋)
     *    + 4096 = Admin Queue 1쌍의 Doorbell 영역 */
    if (nvme_remap_bar(dev, NVME_REG_DBS + 4096))
        goto release;
    return 0;
}

/* ===== nvme_remap_bar() ===== */
static int nvme_remap_bar(struct nvme_dev *dev, unsigned long size)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);

    /* 이미 충분한 크기로 매핑되어 있으면 아무것도 안 함 */
    if (size <= dev->bar_mapped_size)
        return 0;

    /* 요청 크기가 BAR0의 실제 크기보다 크면 에러.
     * 예: 128개 큐를 만들고 싶은데 BAR가 16KB밖에 안 되면 실패 */
    if (size > pci_resource_len(pdev, 0))
        return -ENOMEM;

    /* 기존 매핑이 있으면 해제 */
    if (dev->bar)
        iounmap(dev->bar);     /* 가상 주소 매핑 해제 */

    /* ★ 핵심 ★
     * ioremap()은 물리 주소를 커널 가상 주소로 매핑한다.
     *
     * pci_resource_start(pdev, 0) = BAR0의 물리 시작 주소
     *                               (BIOS가 PCIe config space에 기록한 값)
     * size = 매핑할 크기
     *
     * 리턴값 = 커널 가상 주소 (이후 readl/writel로 접근 가능)
     *
     * 내부적으로는 페이지 테이블에 새로운 매핑을 추가하되,
     * 캐싱을 비활성화(Uncacheable)하는 것이 핵심 차이!
     * RAM은 Write-Back 캐싱이지만, MMIO는 UC(Uncacheable)로 설정해야
     * 매번 실제로 PCIe 트랜잭션이 발생한다. */
    dev->bar = ioremap(pci_resource_start(pdev, 0), size);
    if (!dev->bar) {
        dev->bar_mapped_size = 0;
        return -ENOMEM;
    }
    dev->bar_mapped_size = size;

    /* Doorbell 레지스터 시작 주소를 별도로 저장.
     * NVME_REG_DBS = 0x1000.
     * dev->dbs = dev->bar + 0x1000
     * 이후 각 큐의 Doorbell은 dev->dbs에서 오프셋으로 계산한다. */
    dev->dbs = dev->bar + NVME_REG_DBS;

    return 0;
}
```

### 3.3 ioremap()이 하는 일 (핵심!)

```
  ioremap() 호출 전:
  ┌─────────────────────────────────────────────────┐
  │  커널 페이지 테이블                              │
  │                                                  │
  │  가상 주소              물리 주소      속성       │
  │  0xFFFF_8000_0000_0000  0x0000_0000   WB (RAM)   │
  │  0xFFFF_8000_4000_0000  0x4000_0000   WB (RAM)   │
  │  ...                    ...           ...        │
  │  (BAR 영역은 아직 매핑 없음)                     │
  └─────────────────────────────────────────────────┘

  ioremap(0xFB000000, 0x4000) 호출 후:
  ┌─────────────────────────────────────────────────┐
  │  커널 페이지 테이블                              │
  │                                                  │
  │  가상 주소              물리 주소      속성       │
  │  0xFFFF_8000_0000_0000  0x0000_0000   WB (RAM)   │
  │  0xFFFF_8000_4000_0000  0x4000_0000   WB (RAM)   │
  │  ...                    ...           ...        │
  │  0xFFFF_A000_FB00_0000  0xFB00_0000   UC (MMIO)  │ ← 새로 추가!
  │  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~    │
  │  │ 이 가상 주소가 dev->bar에 저장됨              │
  │  │ UC = Uncacheable (캐시 안 함, 매번 PCIe 접근) │
  └─────────────────────────────────────────────────┘
```

**WB vs UC가 중요한 이유**:
- **WB (Write-Back)**: 일반 RAM. CPU가 캐시에 쓰고 나중에 RAM에 반영. 같은 주소를 여러 번 써도 마지막 값만 RAM에 씀
- **UC (Uncacheable)**: MMIO 영역. CPU가 매번 직접 PCIe 트랜잭션을 보냄. 같은 주소에 5번 쓰면 5번 다 PCIe TLP가 나감. **디바이스는 매 쓰기를 봐야 하므로 캐싱하면 안 됨!**

만약 Doorbell 레지스터를 캐싱하면:
```
  writel(1, doorbell);  → 캐시에만 씀 (디바이스 모름!)
  writel(2, doorbell);  → 캐시에만 씀 (디바이스 모름!)
  writel(3, doorbell);  → 캐시가 evict될 때 3만 디바이스에 전달
                          → 디바이스는 1, 2를 영영 모르게 됨! → I/O 유실!
```
그래서 `ioremap()`은 해당 영역을 반드시 UC로 매핑한다.

---

## 4. readl() / writel()은 실제로 무엇을 하는가?

### 4.1 writel()의 정체

```c
/* arch/x86/include/asm/io.h (x86 아키텍처) */

/* writel()은 결국 이렇게 된다: */
static inline void writel(u32 val, volatile void __iomem *addr)
{
    /* asm volatile: 컴파일러 최적화 방지 (순서 보장)
     * "movl %0, %1": x86 MOV 명령어로 val을 addr에 쓴다
     *
     * 이 MOV 명령어가 실행되면:
     * 1. CPU가 가상 주소 addr을 물리 주소로 변환 (MMU)
     * 2. 물리 주소가 RAM 영역이 아닌 MMIO 영역임을 확인
     * 3. CPU → Memory Controller → PCIe Root Complex로 전달
     * 4. Root Complex가 Memory Write TLP를 생성
     * 5. PCIe 링크를 통해 NVMe 컨트롤러에 TLP 도착
     * 6. NVMe 컨트롤러가 오프셋으로 레지스터를 식별하고 처리
     */
    asm volatile("movl %0,%1"
                 : : "r"(val), "m"(*(volatile u32 __force *)addr)
                 : "memory");
}
```

### 4.2 writel()로 Doorbell을 쓸 때 일어나는 일

```
  커널 코드:
  writel(nvmeq->sq_tail, nvmeq->q_db);
         ~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~
         예: 5 (SQ에 5개   dev->dbs + (2 * qid * stride)
              명령어 추가)  예: dev->bar + 0x1008 (SQ1 Doorbell)

  ┌─────────────────────────────────────────────────────────────────┐
  │ CPU Core                                                        │
  │                                                                 │
  │  MOV [0xFFFF_A000_FB00_1008], 5                                │
  │       ~~~~~~~~~~~~~~~~~~~~~~~~  ~                               │
  │       가상 주소 (커널 공간)     값                               │
  │            │                                                    │
  │            ▼                                                    │
  │  ┌──────────────┐                                              │
  │  │     MMU       │                                              │
  │  │  가상→물리    │  0xFFFF_A000_FB00_1008 → 0xFB00_1008        │
  │  │  페이지 속성: │  UC (Uncacheable) → 캐시 우회               │
  │  └──────┬───────┘                                              │
  │         │                                                       │
  │         ▼                                                       │
  │  ┌──────────────┐                                              │
  │  │ Store Buffer  │  UC이므로 Store Buffer를 통과하지만          │
  │  │              │  WC(Write-Combining)처럼 합치지는 않음        │
  │  └──────┬───────┘                                              │
  └─────────┼───────────────────────────────────────────────────────┘
            │
            ▼
  ┌──────────────────┐
  │ Memory Controller │  "물리 주소 0xFB001008은 RAM이 아님"
  │                   │  → PCIe Root Complex로 라우팅
  └────────┬─────────┘
           │
           ▼
  ┌──────────────────┐
  │ PCIe Root Complex │
  │                   │  물리 주소 0xFB001008이 어떤 디바이스의 BAR인지 확인
  │                   │  → NVMe SSD (Bus:Dev:Func = 01:00.0)의 BAR0
  │                   │
  │                   │  Memory Write TLP 생성:
  │                   │  ┌─────────────────────────────────┐
  │                   │  │ TLP Header:                      │
  │                   │  │   Type: MWr (Memory Write)       │
  │                   │  │   Address: 0x1008 (BAR 내 오프셋)│
  │                   │  │   Length: 1 DW (4바이트)          │
  │                   │  │   Requester ID: CPU의 BDF        │
  │                   │  │ Data: 0x00000005                  │
  │                   │  └─────────────────────────────────┘
  └────────┬─────────┘
           │  PCIe Link (x4 Gen4 = 약 8GB/s)
           ▼
  ┌──────────────────┐
  │   NVMe Controller │
  │                   │  TLP 수신 → 오프셋 0x1008 = SQ1 Tail Doorbell
  │                   │  → SQ1의 Tail을 5로 업데이트
  │                   │  → "SQ1에 새 명령어가 있구나!"
  │                   │  → DMA로 호스트 메모리의 SQ1에서 명령어 fetch
  └──────────────────┘
```

### 4.3 readl()로 컨트롤러 상태를 읽을 때

```c
/* 컨트롤러 상태 확인 */
u32 csts = readl(dev->bar + NVME_REG_CSTS);  /* BAR0 + 0x1C */
```

```
  CPU: MOV reg, [0xFFFF_A000_FB00_001C]  (읽기)
       │
       ▼
  MMU: 가상→물리 변환, UC 확인
       │
       ▼
  PCIe Root Complex:
       Memory Read TLP 생성:
       ┌──────────────────────────────┐
       │ Type: MRd (Memory Read)      │
       │ Address: 0x001C              │
       │ Length: 1 DW                 │
       │ Requester ID: CPU BDF        │
       │ Tag: 0x42 (응답 매칭용)      │
       └──────────────────────────────┘
       │
       ▼ (PCIe 링크를 통해 디바이스로)
  NVMe Controller:
       CSTS 레지스터 값을 읽어서 Completion TLP로 응답:
       ┌──────────────────────────────┐
       │ Type: CplD (Completion)      │
       │ Tag: 0x42 (요청과 매칭)      │
       │ Data: 0x00000001 (RDY=1)     │
       └──────────────────────────────┘
       │
       ▼ (PCIe 링크를 통해 CPU로)
  CPU: MOV 결과 = 0x00000001
       → CSTS.RDY = 1 → 컨트롤러 준비 완료!
```

**중요 차이**: `writel()`은 **Posted** (응답 안 기다림, 빠름), `readl()`은 **Non-Posted** (응답 기다림, 느림). 그래서 Doorbell writel()은 빠르고, readl()은 수백 ns 걸린다.

---

## 5. dev->bar의 여정: 물리 주소에서 코드까지

### 5.1 전체 연결 관계

```
  NVMe SSD 하드웨어                    Linux 커널 코드
  ═══════════════                    ═══════════════

  BAR0 물리 주소: 0xFB000000          dev->bar (ioremap 결과)
  (BIOS가 할당)                       = ioremap(0xFB000000, size)
                                      = 0xFFFF_A000_FB00_0000 (가상 주소)
  ┌──────────┐
  │ 0x0000   │ CAP ──────────────────→ readl(dev->bar + 0x0000)
  │ 0x0008   │ VS  ──────────────────→ readl(dev->bar + 0x0008)
  │ 0x0014   │ CC  ──────────────────→ writel(val, dev->bar + 0x0014)
  │ 0x001C   │ CSTS ─────────────────→ readl(dev->bar + 0x001C)
  │ 0x0024   │ AQA ──────────────────→ writel(aqa, dev->bar + 0x0024)
  │ 0x0028   │ ASQ ──────────────────→ lo_hi_writeq(addr, dev->bar + 0x0028)
  │ 0x0030   │ ACQ ──────────────────→ lo_hi_writeq(addr, dev->bar + 0x0030)
  │          │
  │ 0x1000   │ SQ0 Tail DB ──────────→ dev->dbs = dev->bar + 0x1000
  │ 0x1004   │ CQ0 Head DB            │
  │ 0x1008   │ SQ1 Tail DB            ├─→ nvmeq->q_db = &dev->dbs[qid * 2 * stride]
  │ 0x100C   │ CQ1 Head DB            │   writel(tail, nvmeq->q_db)
  │ 0x1010   │ SQ2 Tail DB            │   writel(head, nvmeq->q_db + stride)
  │ ...      │ ...                     │
  └──────────┘

  코드에서 매크로로 오프셋 정의 (include/linux/nvme.h):
  ───────────────────────────────────────────────────
  enum {
      NVME_REG_CAP  = 0x0000,   /* Controller Capabilities */
      NVME_REG_VS   = 0x0008,   /* Version */
      NVME_REG_CC   = 0x0014,   /* Controller Configuration */
      NVME_REG_CSTS = 0x001c,   /* Controller Status */
      NVME_REG_AQA  = 0x0024,   /* Admin Queue Attributes */
      NVME_REG_ASQ  = 0x0028,   /* Admin SQ Base Address */
      NVME_REG_ACQ  = 0x0030,   /* Admin CQ Base Address */
      NVME_REG_DBS  = 0x1000,   /* SQ 0 Tail Doorbell */
  };
```

### 5.2 transport 독립성: core.c vs pci.c

NVMe는 PCIe 외에 RDMA, TCP로도 연결할 수 있다. 그래서 core.c는 레지스터 접근을 직접 하지 않고, transport별 콜백을 통해 간접 접근한다.

```
  nvme_core (core.c):
      ctrl->ops->reg_read32(ctrl, NVME_REG_CSTS, &csts);
                  │
                  ▼
  ┌─────────────────────────────────────────────────────┐
  │  transport별 구현                                    │
  │                                                      │
  │  PCIe (pci.c):    readl(dev->bar + off)     ← MMIO  │
  │  RDMA (rdma.c):   NVMe-oF Fabric 커맨드    ← 네트워크│
  │  TCP  (tcp.c):    NVMe-oF Fabric 커맨드    ← 네트워크│
  └─────────────────────────────────────────────────────┘
```

실제 코드:
```c
/* pci.c - PCIe transport에서의 레지스터 접근 */
static int nvme_pci_reg_read32(struct nvme_ctrl *ctrl, u32 off, u32 *val)
{
    /* dev->bar는 ioremap()으로 매핑된 커널 가상 주소.
     * readl()은 MMIO 읽기 → PCIe Memory Read TLP 발생 */
    *val = readl(to_nvme_dev(ctrl)->bar + off);
    return 0;
}

static int nvme_pci_reg_write32(struct nvme_ctrl *ctrl, u32 off, u32 val)
{
    /* writel()은 MMIO 쓰기 → PCIe Memory Write TLP 발생 */
    writel(val, to_nvme_dev(ctrl)->bar + off);
    return 0;
}

/* 이 콜백들이 nvme_pci_ctrl_ops에 등록됨 */
static const struct nvme_ctrl_ops nvme_pci_ctrl_ops = {
    .reg_read32  = nvme_pci_reg_read32,   /* MMIO readl */
    .reg_write32 = nvme_pci_reg_write32,  /* MMIO writel */
    .reg_read64  = nvme_pci_reg_read64,   /* MMIO lo_hi_readq */
    ...
};
```

---

## 6. Doorbell 주소 계산 과정

### 6.1 수식

NVMe 스펙에 따른 Doorbell 주소 계산:
```
  SQ y Tail Doorbell = BAR0 + 0x1000 + (2y     × (4 << DSTRD))
  CQ y Head Doorbell = BAR0 + 0x1000 + ((2y+1) × (4 << DSTRD))

  DSTRD = CAP 레지스터의 Doorbell Stride (보통 0)
  DSTRD=0이면: 4 << 0 = 4바이트 간격

  예 (DSTRD=0):
  SQ0 Tail DB = 0x1000 + (0  × 4) = 0x1000
  CQ0 Head DB = 0x1000 + (1  × 4) = 0x1004
  SQ1 Tail DB = 0x1000 + (2  × 4) = 0x1008
  CQ1 Head DB = 0x1000 + (3  × 4) = 0x100C
  SQ2 Tail DB = 0x1000 + (4  × 4) = 0x1010
  CQ2 Head DB = 0x1000 + (5  × 4) = 0x1014
  ...
```

### 6.2 커널 코드에서의 계산

```c
/* nvme_pci_enable()에서 */
dev->db_stride = 1 << NVME_CAP_STRIDE(dev->ctrl.cap);
/* DSTRD=0이면: db_stride = 1 << 0 = 1
 * 실제 바이트 간격 = db_stride × 4 = 4바이트 */

dev->dbs = dev->bar + 4096;  /* 0x1000 = NVME_REG_DBS */

/* nvme_init_queue()에서 큐별 Doorbell 주소 설정 */
nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];
/* qid=0: q_db = dbs + 0     = BAR0 + 0x1000  (SQ0 Tail DB)
 * qid=1: q_db = dbs + 2*4   = BAR0 + 0x1008  (SQ1 Tail DB)
 * qid=2: q_db = dbs + 4*4   = BAR0 + 0x1010  (SQ2 Tail DB)
 *
 * CQ Head DB는 q_db + stride 위치:
 * qid=0: q_db + 1*4 = BAR0 + 0x1004  (CQ0 Head DB)
 * qid=1: q_db + 1*4 = BAR0 + 0x100C  (CQ1 Head DB)
 */
```

### 6.3 실제 Doorbell 쓰기 코드

```c
/* SQ Tail Doorbell 업데이트 */
static void nvme_write_sq_db(struct nvme_queue *nvmeq, bool write_sq)
{
    /* Shadow Doorbell이 활성화된 경우 */
    if (!write_sq) {
        /* Shadow DB: 호스트 메모리에만 쓰고 MMIO는 건너뛴다.
         * 컨트롤러가 DMA로 이 값을 읽어간다. */
        u16 next_tail = nvmeq->sq_tail + 1;
        if (next_tail == nvmeq->q_depth)
            next_tail = 0;

        /* Event Index와 비교하여 MMIO가 필요한지 확인 */
        if (!nvme_dbbuf_update_and_check_event(nvmeq->sq_tail,
                nvmeq->dbbuf_sq_db, nvmeq->dbbuf_sq_ei))
            return;  /* MMIO 불필요! 수백 ns 절약 */
    }

    /* ★ 이것이 실제 Doorbell Write ★
     * writel()은 UC MMIO 쓰기 → PCIe Memory Write TLP
     * → NVMe 컨트롤러가 SQ를 fetch하기 시작 */
    writel(nvmeq->sq_tail, nvmeq->q_db);
    nvmeq->last_sq_tail = nvmeq->sq_tail;
}
```

---

## 7. /proc/iomem으로 실제 매핑 확인하기

실제 시스템에서 NVMe의 BAR 매핑을 확인할 수 있다:

```bash
$ cat /proc/iomem | grep -i nvme
  fb000000-fb003fff : nvme          # ← BAR0: 0xFB000000 ~ 0xFB003FFF (16KB)
                                    #    컨트롤러 레지스터 + Doorbell 영역

$ lspci -v -s 01:00.0              # NVMe 디바이스의 PCIe 정보
01:00.0 Non-Volatile memory controller: Samsung Electronics Co Ltd
        ...
        Region 0: Memory at fb000000 (64-bit, non-prefetchable) [size=16K]
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        │ BAR0: 물리 주소 0xFB000000, 크기 16KB
        │ non-prefetchable = UC로 매핑해야 함 (캐싱 금지)
```

```bash
$ setpci -s 01:00.0 10.L           # BAR0 값 직접 읽기 (Config Space offset 0x10)
fb000004                            # 하위 비트 4 = 64비트 BAR
                                    # 상위 비트 = 물리 주소
```

---

## 8. 전체 정리: 커널 코드 한 줄이 하드웨어까지 닿는 경로

```
  ┌─────────────────────────────────────────────────────────────────────┐
  │  커널 코드 (pci.c)                                                  │
  │                                                                     │
  │  writel(nvmeq->sq_tail, nvmeq->q_db);                              │
  │         └─ 예: 5       └─ 예: dev->bar + 0x1008                    │
  │                              = ioremap(0xFB000000, ...) + 0x1008   │
  │                              = 0xFFFF_A000_FB00_1008 (가상 주소)   │
  └─────────────────────────┬───────────────────────────────────────────┘
                            │
  ┌─────────────────────────▼───────────────────────────────────────────┐
  │  CPU 하드웨어                                                       │
  │                                                                     │
  │  1. x86 MOV 명령어 실행                                            │
  │  2. MMU: 가상 0xFFFF_A000_FB00_1008 → 물리 0xFB00_1008            │
  │          페이지 속성: UC (Uncacheable) → 캐시 우회                  │
  │  3. Store Buffer → Memory Controller                               │
  │  4. "0xFB001008은 DRAM 범위 밖 → PCIe MMIO"                       │
  └─────────────────────────┬───────────────────────────────────────────┘
                            │
  ┌─────────────────────────▼───────────────────────────────────────────┐
  │  PCIe Root Complex                                                  │
  │                                                                     │
  │  5. 물리 주소 0xFB001008 → Bus:01 Dev:00 Func:0 BAR0 + 0x1008     │
  │  6. Memory Write TLP 생성 (Posted: 응답 안 기다림)                  │
  │     Header: Type=MWr, Addr=0x1008, Len=1DW                        │
  │     Data: 0x00000005                                                │
  └─────────────────────────┬───────────────────────────────────────────┘
                            │  PCIe Link
  ┌─────────────────────────▼───────────────────────────────────────────┐
  │  NVMe SSD Controller                                                │
  │                                                                     │
  │  7. TLP 수신, 오프셋 0x1008 디코딩                                 │
  │     = Doorbell 영역 (≥ 0x1000)                                     │
  │     = (0x1008 - 0x1000) / (4 * stride) = 2 → SQ1 Tail Doorbell    │
  │  8. SQ1 Tail = 5로 업데이트                                        │
  │  9. SQ1에서 명령어 fetch 시작 (DMA Read to host memory)            │
  │  10. 명령어 실행 (Read/Write to NAND flash)                        │
  │  11. 완료 시 CQ에 CQE 기록 (DMA Write to host memory)             │
  │  12. MSI-X 인터럽트 발생 (PCIe Memory Write TLP)                   │
  └─────────────────────────────────────────────────────────────────────┘
```

---

## 9. 자주 헷갈리는 개념 정리

### Q1: dev->bar는 물리 주소인가 가상 주소인가?
**가상 주소**다. `ioremap()`이 물리 주소를 커널 가상 주소로 변환한 결과이다. 커널 코드에서는 항상 가상 주소로 접근하고, CPU의 MMU가 물리 주소로 변환한다.

### Q2: writel()과 일반 포인터 쓰기의 차이는?
```c
/* 이렇게 하면 안 됨! */
*(u32 *)addr = value;  // 컴파일러가 최적화로 제거하거나 순서를 바꿀 수 있음

/* writel()을 써야 함 */
writel(value, addr);   // volatile + memory barrier → 순서 보장 + 최적화 방지
```
`writel()`은 내부적으로 `volatile` 접근과 메모리 배리어를 포함하여, 컴파일러가 접근을 제거하거나 순서를 변경하는 것을 방지한다.

### Q3: Doorbell은 왜 writel()이고 SQ/CQ는 왜 DMA인가?
```
  ┌─────────┐                         ┌──────────┐
  │  Host   │  writel() (MMIO)        │  NVMe    │
  │  CPU    │ ──────────────────────▶ │  SSD     │
  │         │  Doorbell만 MMIO로 알림 │          │
  │         │                         │          │
  │  Host   │  DMA (디바이스가 주도)   │  NVMe    │
  │  Memory │ ◀─────────────────────▶ │  SSD     │
  │  (RAM)  │  SQ/CQ/데이터는 DMA     │          │
  └─────────┘                         └──────────┘

  - Doorbell: 작은 값(4바이트)을 CPU가 디바이스에 알림 → MMIO가 적합
  - SQ/CQ/데이터: 큰 데이터를 주고받음 → DMA가 효율적
    - SQ: 디바이스가 호스트 메모리에서 읽어감 (DMA Read)
    - CQ: 디바이스가 호스트 메모리에 씀 (DMA Write)
    - 데이터: 디바이스가 호스트 메모리에서/로 전송 (DMA Read/Write)
```

### Q4: ioremap()과 mmap()의 차이는?
- `ioremap()`: **커널 공간**에서 MMIO 영역을 매핑. 드라이버가 레지스터 접근용으로 사용
- `mmap()`: **유저 공간**에서 파일이나 디바이스를 매핑. 유저 프로그램이 사용
- SPDK는 `mmap()`으로 BAR0를 유저 공간에 매핑하여 커널 우회 접근한다 (VFIO)

### Q5: Shadow Doorbell은 MMIO를 안 하는 건가?
아니다. "매번" MMIO를 하지 않는 것이다. 호스트 메모리에 Doorbell 값을 쓰면 컨트롤러가 DMA로 읽어간다. 컨트롤러가 "이 값 이상이 되면 MMIO로 알려줘"라고 Event Index를 설정하면, 그 값을 넘을 때만 실제 MMIO writel()을 수행한다. 결과적으로 MMIO 횟수가 크게 줄어든다.

---

## 10. 소스코드 위치 정리

| 파일 | 핵심 내용 |
|------|-----------|
| `drivers/nvme/host/pci.c:2813` | `nvme_remap_bar()` - BAR0를 ioremap으로 매핑 |
| `drivers/nvme/host/pci.c:4175` | `nvme_dev_map()` - BAR0 영역 예약 + 초기 매핑 |
| `drivers/nvme/host/pci.c:3750` | `nvme_pci_enable()` - CAP 읽기, db_stride/dbs 설정 |
| `drivers/nvme/host/pci.c:2851` | `nvme_pci_configure_admin_queue()` - AQA/ASQ/ACQ 레지스터 쓰기 |
| `drivers/nvme/host/pci.c:2607` | `nvme_init_queue()` - 큐별 q_db Doorbell 주소 계산 |
| `drivers/nvme/host/pci.c:4093` | `nvme_pci_reg_read32()` - MMIO readl wrapper |
| `drivers/nvme/host/pci.c:4099` | `nvme_pci_reg_write32()` - MMIO writel wrapper |
| `include/linux/nvme.h:217` | `NVME_REG_*` - 레지스터 오프셋 enum 정의 |
