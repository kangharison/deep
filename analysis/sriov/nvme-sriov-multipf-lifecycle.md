# NVMe SR-IOV · Multi-PF 자원 수명주기 총정리 — 생성 · 변경 · 삭제 · 리셋 · 펌웨어 업데이트

> 대상: Linux `v7.2`(upstream), QEMU `v11.1.2` + qemu-dev(Q4 `045e8fd98f` multi-PF, Q5 `43e5e6baec` NSSR)
> 근거: 소스 코드 + QEMU q35 게스트 실측(2026-10-17~19). 호스트 커널은 건드리지 않음. **(실측)** / **(코드)** / **(미검증)** 으로 근거 수준을 표시한다.
> 세부 문서: [[nvme-sriov-qemu-vs-kernel]](구현 수준, BAR/브리지 창 §9, user space §10, multi-PF §11), [[nvme-sriov-dynamic-resource-design]](I/O 유지 리사이즈 설계)

---

## 0. 한 장 요약

```
                ┌──────────────── 부팅 1회 (바꾸기 어려움) ────────────────┐
  PCI 계층       │ PF 열거(ARI 사슬) → TotalVFs·VF BAR 판독 → IOV 공간 예약     │  remove 없이 바꿀 수 없음
                │ → 상위 브리지 창 확정(pci=realloc/펌웨어) → 버스 번호 예약      │
                └───────────────────────────────────────────────────────────┘
                ┌──────────────── 운영 중 1회 (PF별) ─────────────────────┐
  PCI VF 함수    │ sriov_numvfs = 0 ↔ N 만 가능 (N→M 금지, EBUSY)             │  바꾸면 그 PF의 VF 전체 재생성
                └───────────────────────────────────────────────────────────┘
                ┌──────────────── 운영 중 수시 (VF별) ────────────────────┐
  NVMe 계층      │ secondary: Assign(VQ/VI) → Online → 드라이버 bind           │  ← "동적 VF 생성/삭제/변경"은
                │            unbind → Offline(자원 회수)                       │     여기서 한다
                └───────────────────────────────────────────────────────────┘
```

핵심 원칙 5가지:
1. **최대치는 부팅 때 예약한다.** IOV 공간 = PF별 `VF BAR × TotalVFs`의 합이고, enumeration 때 한 번만 계산된다. 사용 중인 창은 런타임에 키울 수 없다. (실측 §9, §11.4.2-C)
2. **PCI VF 함수는 한 번에 최대로 켜 둔다.** `sriov_numvfs`는 0↔N만 가능하므로 VF 수를 바꾸려면 그 PF의 VF 전체를 내려야 한다. (실측 §3.2)
3. **생성·삭제·변경은 NVMe secondary로 한다.** 자원 할당 + Online + bind로 "생성"하고, unbind + Offline으로 "삭제"한다. 다른 VF의 I/O에는 영향이 없다. (실측 §3)
4. **변경(리사이즈) = VF 리셋.** 스펙상 Assign은 Offline에서만 가능하다. 그래서 목표는 "무중단"이 아니라 "계획된 짧은 리셋 + I/O 보류/failover"다. (실측 설계문서 §1)
5. **리셋 범위(PF / 버스 / subsystem)마다 복구 단위가 다르다.** 지금 커널은 어떤 리셋에서도 VF를 살리지 못한다. (실측 §11.4.4)

---

## 1. 자원의 3계층과 소유자

| 계층 | 자원 | 결정 시점 | 소유/결정자 | 바꾸는 방법 |
|---|---|---|---|---|
| PCI 주소 공간 | PF BAR, **IOV 공간(VF BAR × TotalVFs)**, 브리지 창, 버스 번호 | 부팅(enumeration) | 커널 PCI 코어 (`sriov_init` `iov.c:796`, `pci_assign_unassigned_*`) | 펌웨어 예약 / `pci=realloc` / (개발) K9 과예약 / 최후 remove·rescan |
| PCI VF 함수 | VF `pci_dev`, VF BAR 주소, VF config | `sriov_numvfs` 쓰기 | 커널 PCI 코어 + PF 드라이버 `.sriov_configure` | 0↔N만 |
| NVMe secondary | VQ(SQ/CQ 쌍), VI(MSI-X 벡터), Online 상태, CNTLID | virt-mgmt 명령 | PF 컨트롤러(primary), 사람/도구 (커널은 관여 안 함) | Assign(0x8)/Online(0x9)/Offline(0x7), PF 자신은 Primary Alloc(0x1) |
| 블록/드라이버 | nvme 컨트롤러, hctx(큐) 수, IRQ, namespace 경로 | VF probe / 리셋 | VF 드라이버(nvme 또는 vfio+SPDK) | bind/unbind, 컨트롤러 리셋 |
| NVM subsystem | namespace, cntlid 공간, NSSR 범위 | 장치 구성 | 장치 | ns attach/detach |

Multi-PF에서 달라지는 점: PF마다 IOV 공간·TotalVFs·flexible 풀·secondary 목록이 **독립**이다. 다만 IOV 공간은 **같은 상위 창**을 공유하고(합산), namespace·cntlid·NSSR은 **subsystem 단위로 공유**한다. (실측 §11.4.2)

---

## 2. 전제 조건 (부팅 전·부팅 때)

| # | 조건 | 왜 | 확인 방법 | 근거 |
|---|---|---|---|---|
| B1 | PCIe 확장 config space(ECAM). QEMU는 `-machine q35` | SR-IOV/ARI cap이 0x100 이상에 있음 | `sriov_totalvfs` 존재 여부 | 실측 §5 |
| B2 | **TotalVFs·VF BAR가 최대치로 보고되는 상태로 부팅** | 커널은 enumeration 때 한 번만 읽음(`iov.c:827`), 리셋 후 재판독 없음(`:960`) | SR-IOV cap+0x0E, `resource` 8~13번째 줄 | 코드 §9.3.2 |
| B3 | 상위 브리지 창 ≥ Σ(PF BAR) + Σ(PF별 VF BAR × TotalVFs) | `sriov_enable` 시 할당되지 않았으면 VF 1개도 불가(`iov.c:689`) | dmesg `VF BAR ...: assigned` / `can't assign` | 실측 §9.2, §9.3.1, §11.4.2-C |
| B4 | 핫플러그 off면 여유 예약 없음 → **펌웨어 예약 또는 `pci=realloc`** | `hpmmiosize`는 hotplug 브리지에만 적용(`setup-bus.c:1432`) | — | 실측 §9.3.1 |
| B5 | Multi-PF: PF0의 **ARI Next Function 사슬**이 모든 PF를 가리킴 | 커널은 사슬로 PF를 찾음(`probe.c:2798`) | 모든 PF가 `lspci`에 보이는지 | 실측 §11.4 (Q4 전 PF1 미열거) |
| B6 | Multi-PF: PF별 VF RID 범위가 겹치지 않음(Offset/Stride), 버스 범위 충분 | 겹치면 장치 생성 실패, 버스 초과면 `can't enable %d VFs (bus out of range)` | `sriov_offset/stride` | 실측 §11.4.1, 코드 `iov.c:693` |
| B7 | 같은 subsystem 컨트롤러는 serial 동일 | QEMU 검사 | — | 실측 §11.4 |
| B8 | PF 드라이버가 바인딩되어 있어야 `sriov_numvfs` 가능 | `iov.c` `sriov_numvfs_store`: 드라이버 없으면 `-ENOENT` | — | 코드 |

**remove 없이 운영하려면 B2·B3를 부팅 때 만족시키는 것이 전부다.** 레지스터가 최대치를 보고하지 않는 장치는 K9(커널 인자 `pci=sriov_reserve`)로 최대치를 전달해야 한다. nvme 드라이버의 define은 늦다. nvme probe(device_initcall)가 창 확정(fs_initcall) 이후에 일어나기 때문이다. (코드 §9.3.5)

---

## 3. 생성 (Create)

### 3.1 권장 모델 — "PCI VF는 고정, NVMe secondary는 동적"
```
[PF별 1회]  echo 0 > sriov_drivers_autoprobe        Offline VF가 자동 probe되어 실패하는 것 방지
            echo <TotalVFs> > sriov_numvfs          VF 함수 전부 생성 (전부 secondary Offline 상태)
[VF 생성]   virt-mgmt -c SCID -r 0 -a 8 -n VQ        VQ 할당 (Offline 상태에서만 가능)
            virt-mgmt -c SCID -r 1 -a 8 -n VI        VI 할당
            virt-mgmt -c SCID -a 9                   Online → QEMU가 VF 함수 리셋, 새 자원 활성화
            echo nvme > <VF>/driver_override         (vfio-pci면 vfio-pci)
            echo <VF> > /sys/bus/pci/drivers_probe   → nvme probe → "x/0/0 queues"
```
**실측 (단일 PF, TotalVFs 4, `vq_flexible=16 vi_flexible=12`)**: VF A(SCID 1)에서 `dd iflag=direct` 루프를 돌리는 중에 다음을 수행했다.
- VF B 생성(VQ 3, VI 1) → 삭제 → 다른 자원으로 재생성(VQ 4, VI 3)
- 그동안 A는 `rd_ios` 4121→14905, 에러 0, 계속 `live`였다.
- B의 큐 수: VI 1 → `1/0/0`, VI 3 → `2/0/0` (**I/O 큐 수 = min(VQ−1, VI−1)**, VI가 1이면 1)

### 3.2 왜 PCI VF 수를 런타임에 바꾸지 않는가
- `sriov_numvfs`는 0↔N만 가능하다. N→M은 `-EBUSY`("%d VFs already enabled. Disable before enabling")다(`drivers/pci/iov.c:463~520`). **(실측: `echo 2` → `Device or resource busy`)**
- PCIe 스펙도 NumVFs는 VF Enable=0일 때만 바꾸도록 정한다.
- 따라서 VF를 하나 늘리려고 NumVFs를 바꾸면 그 PF의 모든 VF가 내려간다. VF 함수를 처음에 최대로 켜 두는 비용(VF `pci_dev`, cntlid)은 어차피 예약된 공간 안이다.

### 3.3 자원 계획 규칙
| 규칙 | 값 | 근거 |
|---|---|---|
| VF당 최소 | VQ ≥ 2(admin + I/O 1), VI ≥ 1 | QEMU `ctrl.c:7338` (Online 조건) |
| VF당 최대 | VQ ≤ VQFRSM, VI ≤ VIFRSM (기본 `flexible / max_vfs`) | 실측: VI 4 > VIFRSM 3 → `Invalid Number of Controller Resources(0x4121)` |
| 풀 | Σ secondary VQ ≤ VQFRT − VQRFAP (VI도 동일) | `ctrl.c:7286` |
| **VI = VQ** 권장 | I/O 큐 = 벡터 − 1이라 VI가 적으면 VQ가 남음 | 실측 (VQ 5·VI 4 → I/O 3) |
| 단계는 원자적이지 않음 | VQ Assign 성공 후 VI Assign 실패 시 **VQ만 할당된 반쪽 상태**가 남음 → 도구/드라이버가 롤백해야 함 | 실측: `SCID 2 Offline VQ 4 VI 0` |
| Multi-PF | 각 PF의 풀은 독립, 각 PF의 SCID에만 virt-mgmt 가능 | 실측 §11.4.2 |

### 3.4 생성 실패 지점
| 증상 | 원인 | 대처 |
|---|---|---|
| `not enough MMIO resources for SR-IOV` | IOV 공간 미할당 (B3) | 펌웨어 예약 / `pci=realloc` / K9 |
| `can't enable %d VFs (bus %02x out of range)` | VF 버스 범위 부족 (B6) | 버스 예약(펌웨어), 장치 Offset/Stride |
| VF probe `Device not ready; CSTS=0x2` | secondary Offline(자원 없음)인데 probe | autoprobe=0, Online 후 bind (K1이면 자동) |
| bind `No such device` | autoprobe=0이면 PCI 코어가 VF probe 거부(`pci-driver.c:461`) | `driver_override` 사용 |
| `0x4121 Invalid Number of Controller Resources` | VQFRSM/VIFRSM 초과 | 상한 확인 후 재시도, 반쪽 상태 롤백 |
| `0x4120 Invalid Secondary Controller State` | Online 상태에서 Assign, 또는 VQ<2·VI=0으로 Online | Offline 후 Assign |

### 3.5 목표 상태 (개발 후 — K1)
`echo N > sriov_numvfs` 한 번에 PF 드라이버가 Primary Caps 조회 → 분배 → Assign → Online → `pci_enable_sriov`까지 처리하고, autoprobe 그대로 VF가 정상 probe된다. 런타임 생성·삭제는 K1이 제공하는 VF별 sysfs(P1-4)로 Assign/Online/bind를 한 번에 처리한다.

---

## 4. 변경 (Update / Resize)

### 4.1 두 가지 경로
| 경로 | 절차 | I/O | 위험 |
|---|---|---|---|
| **R1. 재생성** (VF 사용자 잠깐 중단 가능) | unbind → Offline → Assign(새 값) → Online → bind | 해당 VF의 사용자가 장치를 닫아야 함 | 안전. unbind 때 MSI 도메인(devres)이 해제되므로 **VI 증가도 문제없음** (실측: VI 1→3 재생성 성공, `kernel/irq/msi.c:287, 318`) |
| **R2. 제자리 리사이즈** (사용자 유지) | (설계 P2) freeze/drain → `reset_prepare` → Offline → Assign → Online → `reset_done` → `blk_mq_update_nr_hw_queues` → unfreeze | 보류 후 재개, multipath면 failover | **VI 증가 시 `-ERANGE` → VF dead** (P0-1, 바인딩된 채라 도메인이 남음). VQ만 바꾸면 OK |

**R2 현재 동작 (조율 없이 PF에서 바꿨을 때, 실측):**
| 변경 | 결과 |
|---|---|
| VQ 2→5, VI 고정 | 에러 0, 큐 1→3, 대신 타임아웃 감지까지 **약 12초 정지**(timeout 5초 설정 기준, 기본값이면 수십 초) |
| VI 1→4 | 리셋 중 MSI-X `-ERANGE` → `-EIO` → **dead**, 이후 디스크 용량 0(dd가 0바이트로 "성공"하므로 반드시 `rd_ios`로 확인) |

### 4.2 R2를 쓰려면 필요한 것
1. **P0-1**: PCI/MSI 코어에서 도메인 hwsize가 바뀌면 재생성 (`drivers/pci/msi/irqdomain.c:272`).
2. **P1-4 + P2**: PF 드라이버의 VF별 리사이즈 인터페이스와 계획된 리셋 순서 (설계 문서 §3 P2).
3. **롤백**: Assign 실패 시 이전 값으로 재할당 → Online → `reset_done`.
4. **잠금**: PF별 `sriov->lock`, VF `device_lock`. 같은 PF의 리사이즈·PF 리셋·`sriov_configure`를 직렬화한다.

### 4.3 PF 자신의 flexible 자원 변경
- Primary Flexible Allocation(ACT 0x1)은 `next_pri_ctrl_cap`에 저장되고 **PF 함수 리셋 때** 반영된다(QEMU `ctrl.c:7238`, `:8005`).
- PF 리셋은 그 PF의 모든 secondary를 Offline + 자원 0으로 만든다(`ctrl.c:8073`, `:7332`). 그래서 **PF 몫을 바꾸는 것 = 그 PF의 VF 전체 재생성**이다. 운영 중에는 피하고, 부팅 직후 PF 몫을 확정해 둔다.
- 부팅 직후 QEMU PF의 flexible 점유는 0이다(첫 함수 리셋에서 `next_pri_ctrl_cap`=0으로 덮임). 그래서 flexible 전체를 곧바로 secondary에 줄 수 있다. (실측 `vqrfap 0`)

### 4.4 Multi-PF에서의 변경
- PF 간 자원 이동은 불가하다(풀이 PF별 독립, QEMU 기준). PF A의 VF에 더 주고 싶으면 PF A 풀 안에서만 재분배한다.
- 한 PF의 리사이즈는 다른 PF의 secondary와 I/O에 영향이 없다. (실측: PF0 리셋 시 PF1 secondary Online 유지)

---

## 5. 삭제 (Delete)

### 5.1 VF 하나 삭제 (다른 VF 유지)
```
1. VF 사용자 정리: 파일시스템 umount, 앱 종료 (multipath면 해당 경로만 빠짐)
2. echo <VF> > /sys/bus/pci/drivers/<nvme|vfio-pci>/unbind     ← 먼저 unbind (드라이버가 정상 shutdown)
3. virt-mgmt -c SCID -a 7                                      ← Offline: QEMU가 VQ/VI를 0으로 회수
4. (선택) echo > <VF>/driver_override
```
- **순서가 중요하다.** Offline을 먼저 하면 VF 드라이버가 살아 있는 상태에서 CFS를 보고 타임아웃 → 리셋 실패 → dead 경로를 탄다.
- VF PCI 함수는 남는다(NumVFs 그대로). 중간 VF만 PCI 계층에서 없앨 수는 없다(NumVFs는 끝에서부터 줄어드는 개념이고 그마저 0↔N만 가능).
- **(실측)** unbind → Offline 후 `list-secondary`가 `SCID 2 Offline VQ 0 VI 0`, 같은 PF의 다른 VF I/O는 무영향.

### 5.2 PF의 VF 전체 삭제
```
1. 모든 VF 사용자 정리
2. echo 0 > sriov_numvfs        → pci_disable_sriov: VF 드라이버 remove, VF pci_dev 제거
                                → QEMU: NumVFs 감소분 secondary 자동 Offline(ctrl.c:9840) + 자원 회수
3. (K1이면) 드라이버가 남은 secondary Offline/회수까지 정리
```

### 5.3 PF 삭제 (드라이버 unbind / remove)
- PF 드라이버를 unbind하려면 VF를 먼저 0으로 만드는 것이 안전하다. vfio-pci도 PF 해제 시 `vfio_pci_core_sriov_configure(vdev, 0)`을 호출한다(`vfio_pci_core.c:2320`).
- PF remove는 IOV 예약을 해제한다. 다시 rescan했을 때 그 창이 부모 창 안에 다시 들어가는지는 보장되지 않는다(§9.3.1: 부모 창이 빡빡하면 실패). → **remove는 하지 않는다**는 원칙과 충돌하므로 운영 절차에서 배제한다.

---

## 6. 리셋 · 장애 이벤트 — 범위와 할 일

**(실측 §11.4.4, multi-PF 시나리오 A)**
| 이벤트 | Offline 되는 secondary | 커널 인지 | 현재 커널 결과 (VF) | 해야 할 일 (개발 후) |
|---|---|---|---|---|
| PF 컨트롤러 리셋 (CC.EN, `nvme reset`) | 그 PF 것만 | 모름 (VF `live`로 남음) | 다음 명령에서 dead | **K4**: PF `reset_work` 끝에 VF별 재할당·Online → VF `reset_done` |
| PF FLR | 그 PF 것만 + VF PCI 함수 재생성 | PF만 앎 (VF는 `reset_prepare/done` 못 받음, cmd/MSI-X 0) | 다음 명령에서 dead | **K10**(VF PCI 재설정/재probe) + **K4** |
| PF 단독 bus reset | — | 커널이 거부(`pci.c:4873`, 다른 함수가 있으면 불가) | — | (FLR 사용) |
| DSP SBR (`reset_subordinate`) | **모든 PF** | 전부 앎 | PF 복구, **VF 즉시 dead** | PF별 K4. **PF `reset_done`이 VF `reset_done`보다 먼저 끝나야 함** |
| SBR (커널 모르게) / link-down | 모든 PF + PF config 소실 | 모름 | PF 포함 전부 dead | 실장비는 AER/DPC(`pci_error_handlers`)로 감지 → 그 경로에 K4 연결 |
| NSSR | **subsystem 전체** (PCI 위치 무관) | ioctl을 낸 컨트롤러만 잠깐 | PF는 각자 타임아웃 후 복구, VF dead, idle 컨트롤러는 계속 `live`로 착각 | **K11**: 같은 subsystem 컨트롤러 일괄 리셋 예약 → PF마다 K4 |
| 비계획 Offline (외부 도구, 다른 호스트가 virt-mgmt) | 해당 secondary | 모름 | 다음 명령에서 dead | **K5/P3**: Offline 인지 → 대기·재시도, 워치독 |

공통 사실: secondary가 Offline이 되면 QEMU는 VQ/VI를 0으로 되돌린다(`ctrl.c:7332`). **복구 = Online만으로는 부족하고 재할당부터 다시 해야 한다.** 따라서 PF 드라이버가 VF별 원하는 값(`want_nvq/want_nvi`)을 기억해야 한다.

복구 단위 정리:
```
PF 단위        : K4(재할당 + Online) + K10(VF PCI 재설정)           ← PF 리셋, PF FLR
버스/장치 단위  : 모든 PF에 대해 K4, PF 먼저 → VF 나중                 ← SBR, AER/DPC
subsystem 단위 : K11(일괄 리셋 예약) → PF마다 K4                      ← NSSR
VF 단위        : K5/P3(Offline 인지·대기·재시도)                       ← 비계획 Offline, VFIO 패스스루
```

---

## 7. 펌웨어 업데이트

> QEMU NVMe는 Firmware Download/Commit을 구현하지 않는다(슬롯 1개, 읽기 전용: `qemu-11.1.2/hw/nvme/ctrl.c:9414` `NVME_FRMW_SLOT1_RO`). 그래서 이 절은 **(코드)·(미검증)** 이다. QEMU 개발 후보 **Q6**으로 등록한다.

### 7.1 활성화 방식별 영향
| 활성화 | 장치 동작 | 커널 경로 | SR-IOV 영향 |
|---|---|---|---|
| 리셋 없이 즉시 활성화 (Commit Action 3) | 활성화 동안 CSTS.PP=1, AEN "Firmware Activation Starting" | `nvme_handle_aen_notice` → RESETTING → `nvme_fw_act_work`: I/O quiesce → PP 해제까지 대기(MTFA, 없으면 admin_timeout) → LIVE (`core.c:4795, 4850`). 시간 초과면 컨트롤러 리셋 | **컨트롤러마다** AEN을 받는다. VF 드라이버도 각자 처리한다. 리셋이 없으면 secondary 상태는 유지된다(장치 구현 의존) |
| 다음 리셋 때 활성화 (Action 1/2) | 다음 Controller Level Reset 또는 NSSR에서 새 펌웨어로 | 사용자가 `nvme reset` 또는 `subsystem-reset` | §6의 PF 리셋/NSSR과 같음 → **모든 VF secondary Offline → K4/K11 필요** |
| 전원 사이클 필요 | — | 재부팅 | 재부팅 시 TotalVFs/VF BAR를 다시 판독한다. 펌웨어가 SR-IOV 구성을 바꿨다면 여기서 반영된다 |

### 7.2 업데이트 절차 체크리스트
**전**
1. 장치의 활성화 요구사항 확인: Identify Controller `FRMW`(슬롯, 리셋 없이 활성화 지원), `MTFA`(최대 활성화 시간), 벤더 문서(VF 상태가 유지되는지, SR-IOV 구성이 바뀌는지).
2. VF별 현재 할당 기록: `nvme list-secondary` (SCID, VQ, VI, Online), VF↔소비자 매핑, multipath 경로.
3. 리셋이 필요한 활성화라면: VF 사용자를 다른 경로(PF·다른 VF·다른 장치)로 옮기거나 정지 계획을 세운다. multi-PF면 리셋 범위(PF vs subsystem)를 미리 판단한다.
4. 펌웨어 다운로드는 primary(PF) 컨트롤러에서 한다. secondary에서 허용되는지는 장치에 따라 다르다(미검증).

**중**
5. 리셋 없는 활성화: 각 컨트롤러가 PP 대기 중 I/O를 보류한다. MTFA 초과 시 리셋으로 넘어간다.
6. 리셋 활성화: §6 해당 행의 복구를 수행한다(지금은 수동: Assign → Online → VF 재bind).

**후**
7. **SR-IOV 구성 변경 여부 확인**: live TotalVFs(cap+0x0E)·VF BAR와 커널 캐시(`sriov_totalvfs`, `resource`)를 비교한다(K7). 감소면 상한을 제한하고, 증가면 재부팅이 필요하다(K8 없이는 remove 없는 반영 불가).
8. Primary Caps(VQFRT/VIFRT/FRSM)가 바뀌었는지 확인한다. 바뀌었다면 저장한 할당이 새 한도 안인지 재검증한다.
9. 기록한 할당대로 secondary를 복원하고 VF를 재bind한다. multipath 경로 수, `x/0/0 queues`, I/O 통계를 확인한다.

---

## 8. 운영 런북 (현재 커널 기준, 수동)

```sh
# 공통 변수
PF=/sys/bus/pci/devices/<PF BDF>; NV=/dev/nvmeX   # NV = 그 PF의 컨트롤러

# [초기화 — PF당 1회]
echo 0 > $PF/sriov_drivers_autoprobe
echo $(cat $PF/sriov_totalvfs) > $PF/sriov_numvfs
nvme primary-ctrl-caps $NV ; nvme list-secondary $NV

# [VF 생성]  SCID, VF BDF, VQ, VI
nvme virt-mgmt $NV -c $SCID -r 0 -a 8 -n $VQ || exit
nvme virt-mgmt $NV -c $SCID -r 1 -a 8 -n $VI || { nvme virt-mgmt $NV -c $SCID -r 0 -a 8 -n 0; exit; }  # 반쪽 롤백
nvme virt-mgmt $NV -c $SCID -a 9
echo nvme > /sys/bus/pci/devices/$VF/driver_override
echo $VF > /sys/bus/pci/drivers_probe

# [VF 변경 — R1 재생성]
echo $VF > /sys/bus/pci/drivers/nvme/unbind
nvme virt-mgmt $NV -c $SCID -a 7
#  → [VF 생성]을 새 VQ/VI로

# [VF 삭제]
echo $VF > /sys/bus/pci/drivers/nvme/unbind
nvme virt-mgmt $NV -c $SCID -a 7

# [PF 리셋 / NSSR / SBR 이후 복구 — 지금은 수동]
#  각 VF: unbind(이미 dead면 생략 가능) → [VF 생성]을 기록된 값으로
```
측정 시 주의: I/O 확인은 `/sys/block/<dev>/stat`의 `rd_ios`로 한다. dead 디스크에서 `dd`는 0바이트로 성공한다.

---

## 9. 개발 항목 총괄

| ID | 계층 | 내용 | 해결하는 문제 | 상태 |
|---|---|---|---|---|
| P0-1 | PCI/MSI | MSI-X 도메인 hwsize 변경 시 재생성 | R2에서 VI 증가 시 dead | 제안(카드) |
| P0-2 / K5 | nvme | Offline VF를 dead 대신 대기 | 비계획 Offline, VFIO | 설계 |
| K1 | nvme-pci | `sriov_configure` 자체 구현(분배·Online·enable) | 수동 virt-mgmt, Offline probe 실패 | 제안(카드) |
| K2 | nvme-pci | `sriov_set_msix_vec_count` (unbound VF의 VI) | VI 조정 표준 인터페이스 | 설계 |
| K3 / P1-4 | nvme-pci | VF별 VQ/VI sysfs (bound 포함) | 런타임 생성·변경 | 설계 |
| P2 | nvme-pci | 계획된 리셋(drain → reset_prepare → virt-mgmt → reset_done) + 롤백 | R2 I/O 유지 | 설계 |
| K4 | nvme-pci | PF 리셋 후 VF 재할당·Online·재초기화 (want 값 보관) | PF 리셋/SBR/NSSR 후 VF dead | 설계 |
| K6 | 헤더/sysfs | CNS 0x14 정의, Primary/Secondary 노출 | 관측성 | 설계 |
| K7 | nvme-pci/PCI | SR-IOV 구성 변경 감지 (live vs 캐시) | 펌웨어 업데이트 후 불일치 | 설계 |
| K8 | PCI | SR-IOV cap 제자리 재판독 | remove 없이 구성 변경 반영 | 설계 |
| K9 | PCI | `pci=sriov_reserve` 부팅 과예약 | 레지스터가 최대치를 안 줄 때 | 제안(카드) |
| K10 | nvme-pci/PCI | PF FLR 뒤 VF PCI 재설정/재probe | FLR 후 VF cmd·MSI-X 0 | 설계 |
| K11 | nvme core | NSSR 후 같은 subsystem 컨트롤러 일괄 리셋 | NSSR 후 idle 컨트롤러 착각, 순차 타임아웃 | 설계 |
| P3 | nvme | VF 워치독, 재시도 예산 | 감지 지연(수십 초), 재시도 고갈 | 설계 |
| Q1~Q3 | QEMU | VF QoS, VF의 VMS 비트, 상태 변경 통지 | 실험 기반 | 설계 |
| Q4 | QEMU | multi-PF(ARI 사슬, VF offset/stride) | multi-PF 에뮬레이션 | **완료** `045e8fd98f` |
| Q5 | QEMU | NSSR 에뮬레이션 | NSSR 실험 | **완료** `43e5e6baec` |
| Q6 | QEMU | Firmware Download/Commit + 활성화 방식(PP, 리셋) 에뮬레이션 | 펌웨어 업데이트 실험 | 신규 |

의존 관계와 권장 순서:
```
P0-1 ─┐
K6 ───┼─► K1 ─► K3/P1-4 ─► P2(+롤백) ─► K4 ─► K10, K11 ─► P3/K5
      │                                  ▲
K9 ───┴─► K8 ─► K7                        └─ Q6로 펌웨어 시나리오 검증
```
1단계(생성·삭제 자동화): P0-1, K6, K1. 2단계(I/O 유지 변경): K3, P2. 3단계(리셋 내성): K4, K10, K11, P3. 별도 트랙(remove 없는 확장): K9 → K8 → K7.

---

## 10. 검증 매트릭스 (QEMU 게스트)

| # | 시나리오 | 합격 기준 | 현재 |
|---|---|---|---|
| V1 | 단일 PF, VF 생성·삭제·재생성 중 다른 VF I/O | 다른 VF 에러 0, `rd_ios` 증가 | **통과(실측)** |
| V2 | NumVFs N→M | `-EBUSY` | **확인(실측)** |
| V3 | R1 재생성으로 VI 증가 | 정상 probe | **통과(실측, VI 1→3)** |
| V4 | R2 VQ만 증가 (조율 없음) | 에러 0 | 통과, 12초 정지 |
| V5 | R2 VI 증가 (조율 없음) | — | **실패: dead** → P0-1 필요 |
| V6 | R2 + P2 | 에러 0, 정지 < 1초 | 미구현 |
| V7 | PF 리셋 / FLR / SBR / NSSR 후 VF | 자동 복구 | **실패: VF dead** → K4/K10/K11 |
| V8 | multi-PF 한 PF 리셋 | 다른 PF secondary·I/O 무영향 | **통과(실측)** |
| V9 | multi-PF 창 합산 | 모든 PF TotalVFs enable | **통과(실측, 64+64, `pci=realloc`)** |
| V10 | K9 과예약 | 예약 = 지정 최대치 | 미구현 |
| V11 | 펌웨어 활성화(PP / 리셋) 중 VF | PP 동안 I/O 보류, 리셋 후 복구 | QEMU 미지원 → Q6 |

---

## 11. 코드 위치 색인 (이 문서에서 새로 쓴 것)
| 대상 | 위치 |
|---|---|
| `sriov_numvfs` 0↔N 규칙 | `qemu-debug/linux-7.2/drivers/pci/iov.c:463~520` |
| MSI 장치 데이터 devres (unbind 시 도메인 해제) | `qemu-debug/linux-7.2/kernel/irq/msi.c:287, 318` |
| 펌웨어 활성화 AEN / PP 대기 | `qemu-debug/linux-7.2/drivers/nvme/host/core.c:4795, 4850` |
| QEMU 펌웨어 슬롯(읽기 전용 1개) | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:9414` |
| QEMU Offline 시 자원 0 회수 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:7332` |
| QEMU NumVFs 감소 시 Offline | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:9840` |
| vfio PF 해제 시 VF 0 | `qemu-debug/linux-7.2/drivers/vfio/pci/vfio_pci_core.c:2320` |
| 그 외 | [[nvme-sriov-qemu-vs-kernel]] §8, §9, §11 / [[nvme-sriov-dynamic-resource-design]] §6 |
