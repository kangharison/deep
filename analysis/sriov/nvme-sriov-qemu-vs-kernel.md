# NVMe SR-IOV 구현 수준 분석 — QEMU 11.1.2 vs Linux 7.2

> 분석 대상
> - QEMU `v11.1.2` (`qemu-debug/qemu-11.1.2`, 브랜치 `qemu-dev`, upstream 무수정)
> - Linux `v7.2` (`qemu-debug/linux-7.2`, 브랜치 `qemu-dev`, upstream 무수정, `CONFIG_PCI_IOV=y`)
> - 비교 참고: SPDK (`spdk-study`, fd62cb62a)
>
> 근거: 위 소스 코드 + QEMU q35 게스트에서 실측(2026-10-17, TCG, nvme-cli 2.8).
> 스펙: NVMe Base Specification — "Virtualization Enhancements"(Primary/Secondary Controller, Virtualization Management 명령), PCIe Base Spec — SR-IOV Extended Capability.
>
> 관련 문서: [[06_qemu_nvme_device_emulation]] · [[09_pcie_config_space_and_ecam]] · [[11_nvme_driver_init_probe]] · [[20_msix_interrupt_e2e_qemu_trace]]

---

## 0. 한 줄 결론

| 계층 | 구현 수준 | 요약 |
|---|---|---|
| **QEMU NVMe 디바이스** | **스펙의 핵심 흐름 완비** | PF/VF, Primary/Secondary Controller, VQ/VI flexible 자원, Virtualization Management(할당·Online·Offline), Identify CNS 0x14/0x15 모두 동작. 2022년 도입 후 버그 수정 위주. CMB/PMR 병용 불가, QoS/중재 없음. |
| **QEMU PCIe SR-IOV 코어** | 완비 | SR-IOV ext cap, ARI, VF BAR, VF 사전 생성 후 VFE로 enable. |
| **Linux PCI 코어 (`iov.c`)** | 완비 | `sriov_numvfs` 등 sysfs, VF enumerate, autoprobe 제어, **동적 VF MSI-X 할당 API**(mlx5/ice만 사용). |
| **Linux NVMe 드라이버** | **거의 없음** | `.sriov_configure = pci_sriov_configure_simple` 한 줄뿐. Virtualization Management·Primary/Secondary 개념을 전혀 모름 → 자원 할당/Online은 **유저가 nvme-cli passthrough로 수동**. PF 리셋 시 VF가 조용히 죽음. |
| **SPDK** | 정의만 | 스펙 구조체/opcode 정의 + nvmf에서 passthrough 허용. 헬퍼 API 없음. |

즉 **"디바이스(QEMU)는 할 줄 아는데 호스트 드라이버(커널)가 그 능력을 거의 쓰지 않는다."** — 커널 쪽에 기능 개발 여지가 크다.

---

## 1. 배경 개념

### 1.1 PCIe SR-IOV
- **PF(Physical Function)**: 원래 PCI 함수. SR-IOV Extended Capability(ID 0x10)를 가짐.
- **VF(Virtual Function)**: PF가 `NumVFs` 레지스터 + `VF Enable(VFE)` 비트로 만들어내는 경량 함수. 자체 config space(대부분 PF에서 파생)·BAR·MSI-X를 갖고, VM에 통째로 패스스루(VFIO) 가능.
- **ARI(Alternative Routing-ID Interpretation)**: device 번호 없이 function 번호 8비트를 모두 써서 한 디바이스에 256개 함수를 허용. VF 다수를 위해 필요.
- SR-IOV/ARI는 **PCIe 확장 config space(0x100 이상)** 에 있다 → ECAM이 있는 머신(q35)이어야 커널이 읽는다. 기본 `pc`(i440fx)에서는 SR-IOV cap이 안 보인다(실측, §5).

### 1.2 NVMe Virtualization Enhancements
| 용어 | 의미 |
|---|---|
| Primary Controller | PF에 있는 컨트롤러. 자원 관리 권한을 가짐 |
| Secondary Controller | VF에 있는 컨트롤러. SCID(Secondary Controller ID)로 식별 |
| VQ 자원 | Virtual Queue — SQ/CQ 쌍 (admin 큐 포함) |
| VI 자원 | Virtual Interrupt — MSI-X 벡터 |
| Private 자원 | 컨트롤러에 고정된 자원 (`VQPRT`, `VIPRT`) |
| Flexible 자원 | Primary가 secondary들에게 나눠 줄 수 있는 풀 (`VQFRT`, `VIFRT`) |
| Online / Offline | Secondary가 자원을 받아 동작 가능한 상태인지 (`SCS` 비트) |

관련 명령/구조:
```
Identify CNS 0x14 : Primary Controller Capabilities  (VQFRT, VQRFA, VQRFAP, VQPRT, VQFRSM, VQGRAN, VI* 동일)
Identify CNS 0x15 : Secondary Controller List        (SCID, PCID, SCS, VFN, NVQ, NVI)
Admin opcode 0x1C : Virtualization Management
   CDW10[3:0]  ACT : 0x1 Primary Flexible Allocation
                     0x7 Secondary Offline
                     0x8 Secondary Assign
                     0x9 Secondary Online
   CDW10[10:8] RT  : 0 = VQ, 1 = VI
   CDW10[31:16]    : CNTLID (대상 컨트롤러)
   CDW11[15:0] NR  : 자원 개수
Identify Controller OACS bit 7 : Virtualization Management 지원(VMS)
```
(정의: `qemu-debug/qemu-11.1.2/include/block/nvme.h:1724` NvmePriCtrlCap, `:1750` NvmeSecCtrlEntry, `:1768` 액션 enum)

---

## 2. 전체 그림

```
 ┌───────────────────────── Guest (Linux 7.2) ─────────────────────────┐
 │ user: nvme-cli  ── ioctl(NVME_IOCTL_ADMIN_CMD, opcode 0x1C/0x06) ─┐  │
 │        echo N > .../sriov_numvfs        echo nvme > driver_override│  │
 │             │                                     │                │  │
 │   drivers/pci/iov.c                     drivers/pci/pci-driver.c   │  │
 │   sriov_numvfs_store → sriov_configure  pci_device_can_probe()     │  │
 │             │                                     │                │  │
 │   drivers/nvme/host/pci.c  .sriov_configure = pci_sriov_configure_simple │
 │   (PF: nvme0)  ◄──── admin passthrough ────────────────────────────┘  │
 │   (VF: nvme1)  일반 nvme_probe 그대로 — VF/secondary 개념 없음         │
 └──────────────┬───────────────────────────────────┬──────────────────┘
       config write (VFE/NumVFs)            MMIO doorbell / admin SQ
 ┌──────────────▼───────────── QEMU 11.1.2 ─────────▼──────────────────┐
 │ hw/pci/pcie_sriov.c   SR-IOV cap @0x120, ARI @0x100                 │
 │   pf_init: VF PCIDevice를 total_vfs개 '미리' realize (disabled)       │
 │   consume_config: VFE=1 → register_vfs → pci_set_enabled(vf,true)   │
 │ hw/nvme/ctrl.c                                                      │
 │   PF NvmeCtrl: pri_ctrl_cap, sec_ctrl_list[], next_pri_ctrl_cap     │
 │   nvme_virt_mngmt → assign/online/offline                           │
 │   VF NvmeCtrl: params는 PF 복사, 자원은 sec_ctrl_list[vfn]에서 결정    │
 │ hw/nvme/subsys.c   cntlid 예약(PF 뒤에 VF 수만큼 연속)                 │
 └─────────────────────────────────────────────────────────────────────┘
```

---

## 3. QEMU 구현 상세

### 3.1 히스토리 (`git log -- hw/nvme`)
| 날짜 | 커밋 | 내용 |
|---|---|---|
| 2022-05-09 | 44c2c09488 | hw/nvme: Add support for SR-IOV |
| 2022-05-09 | 5e6f963f01 / 99f48ae7ae | Primary Controller Capabilities / Secondary Controller List |
| 2022-05-09 | 746d42b133 / 11871f53ef | 구조 초기화 / Virtualization Management 명령 |
| 2024-01 ~ 2025-09 | 4f0a4a3d58, 91bb64a8d2, 8ab8a6dbe4, e85987786d, 6651f8f2e5, 2e54e5fda7 … | VF serial 분리, BAR 크기 버그, INTx pin=0, subsys refcount, VF MMIO 버그 등 **수정만** |

→ 기능 추가는 2022년 한 번, 이후는 안정화. 디바이스 모델은 성숙한 편.

### 3.2 사용자 파라미터와 검증 (`nvme_check_params`, `ctrl.c:8720`, SR-IOV 부분 `:8818`)
| 파라미터 | 의미 | 제약 |
|---|---|---|
| `sriov_max_vfs` | TotalVFs | `nvme-subsys` 필수, CMB/PMR 병용 불가 |
| `sriov_vq_flexible` | flexible VQ 풀 크기 | ≥ `max_vfs*2` (VF당 admin+IO 최소 2), `max_ioqpairs - vq_flexible ≥ 2` |
| `sriov_vi_flexible` | flexible VI 풀 크기 | ≥ `max_vfs`, `msix_qsize - vi_flexible ≥ 1` |
| `sriov_max_vq_per_vf` / `sriov_max_vi_per_vf` | VF당 상한(VQFRSM/VIFRSM) | 0이면 `flexible / max_vfs` |

상수: `NVME_VF_RES_GRANULARITY 1`, `NVME_VF_OFFSET 0x1`, `NVME_VF_STRIDE 1` (`ctrl.c:231`).
VF 수 상한은 명시 없음 — 실질적으로 devfn 256 오버플로 검사(`pcie_sriov.c:88`), subsys cntlid 256(`hw/nvme/nvme.h:27`), Identify 0x15 한 페이지 127엔트리(`ctrl.c:5903`)에 묶인다.

### 3.3 PCI 계층 초기화 호출 체인
```
nvme_realize (ctrl.c:9627)
 ├─ (VF면) PF의 params를 memcpy — VF는 독자 파라미터가 없다            :9634
 ├─ nvme_init_state (:8885)
 │    PF: nr_sec_ctrls = max_vfs, sec_ctrl_list[i] = {pcid=PF cntlid, vfn=i+1}
 │        pri_ctrl_cap: vqprt = 1+max_ioqpairs-vq_flexible, vqfrt = vq_flexible,
 │                      viprt = msix_qsize-vi_flexible,     vifrt = vi_flexible
 │    VF: conf_ioqpairs = nvq-1, conf_msix_qsize = nvi (오프라인이면 0 / 1)
 ├─ nvme_init_pci (:9113)
 │    pcie_ari_init(0x100)                                               :9137
 │    BAR0 크기: PF=num_queues 기준, VF=PF의 vqfrsm/vifrsm 기준(최대치로 고정) :9152
 │    nvme_init_sriov(0x120) (:9049)
 │       └─ pcie_sriov_pf_init (pcie_sriov.c:152)
 │            ├─ SR-IOV cap 작성: VF DID, TotalVFs, Offset=1, Stride=1  :88
 │            └─ for i < total_vfs: pci_new + qdev_realize  ← VF를 '미리' 생성
 │       └─ pcie_sriov_pf_init_vf_bar(0, 64bit mem, bar_size)
 └─ nvme_init_ctrl (:9254)
      OACS |= VMS, cse.acs[VIRT_MNGMT] = CSUPP   (params.sriov_max_vfs≠0이면) :9310
```
- VF 객체는 PF realize 시점에 전부 만들어지고 **disabled** 상태. 게스트가 `NumVFs`+`VFE`를 쓰면 `pcie_sriov_config_write → consume_config → register_vfs → pci_set_enabled(vf,true)` (`pcie_sriov.c:417, 67, 35`).
- `NumVFs`를 줄이면 `nvme_sriov_post_write_config`(`ctrl.c:9840`)가 사라지는 VF의 secondary를 **Offline** 처리.

### 3.4 Virtualization Management 호출 체인
```
admin opcode 0x1C
 └─ nvme_virt_mngmt (ctrl.c:7361)   ACT=CDW10[3:0], RT=CDW10[10:8], CNTLID=CDW10[31:16], NR=CDW11
     ├─ ACT 0x8 SEC_ASSIGN → nvme_assign_virt_res_to_sec (:7286)
     │     조건: secondary가 Offline(scs==0), NR ≤ VQFRSM/VIFRSM,
     │           (NR - 현재값) ≤ free = total - prim - sec
     │     → nvme_update_virt_res (:7268): sctrl->nvq/nvi 갱신, pri_cap.vqrfa/virfa 합계 갱신
     ├─ ACT 0x1 PRM_ALLOC  → nvme_assign_virt_res_to_prim (:7238)
     │     next_pri_ctrl_cap.vqrfap/virfap 에만 기록 → '다음 함수 리셋' 때 반영
     ├─ ACT 0x9 SEC_ONLINE → nvme_virt_set_state(online) (:7320)
     │     조건: nvi ≥ 1 && nvq ≥ 2 && VF 객체 존재
     │     scs=1 후 nvme_ctrl_reset(VF, NVME_RESET_FUNCTION)
     │        └─ nvme_activate_virt_res (:8005): VF conf_ioqpairs=nvq-1, msix=nvi
     └─ ACT 0x7 SEC_OFFLINE → nvme_virt_set_state(offline)
           nvi=nvq=0으로 회수, scs=0, VF 함수 리셋
```

### 3.5 Offline VF의 하드웨어적 모습
| 위치 | 동작 |
|---|---|
| `nvme_mmio_read` (`ctrl.c:8502`) | Offline VF는 **CSTS 외 모든 레지스터를 0으로 읽음** (CAP=0 포함) |
| `nvme_mmio_write` (`:8674`) | CSTS 외 쓰기 무시 (doorbell 포함) |
| `nvme_start_ctrl` (`:8142`) | CC.EN=1이어도 시작 실패 |
| 리셋 경로 (`:8098`, `:9381`) | CSTS = **CFS(0x2)** |

→ 커널 입장에서는 CAP.TO=0이라 `nvme_wait_ready`가 즉시 타임아웃되고, CSTS.CFS로 "fatal"로 보인다.

### 3.6 리셋과 secondary 상태 (`nvme_ctrl_reset`, `ctrl.c:8028`)
```
nvme_ctrl_reset(PF, 어떤 종류든)            ← CC.EN 토글(Controller Reset)도 포함
  └─ for 모든 secondary: nvme_virt_set_state(offline)   :8073
       → 모든 VF가 Offline + 할당 자원 0으로 회수
  └─ rst != CONTROLLER 이면 nvme_activate_virt_res(PF)
       → PF의 flexible 점유량(vqrfap/virfap) = next_pri_ctrl_cap 값
```
- 주의 1: PF의 **어떤 리셋**이든 VF 전부 Offline. 실측으로 확인(§5 단계 5).
- 주의 2: 초기화 시 `vqrfap = vqfrt`(PF가 flexible 전부 점유)로 시작하지만 첫 함수 리셋에서 `next_pri_ctrl_cap`(=0)으로 덮여 **부팅 후 PF는 flexible을 하나도 안 가진 상태** → 곧바로 secondary에 줄 수 있다(실측 `vqrfap: 0`).

### 3.7 Subsystem / Namespace 연동
- cntlid: PF 등록 시 그 뒤로 `max_vfs`개 cntlid를 연속 예약(`hw/nvme/subsys.c:18, 60~96`). 실측 PF=0, VF SCID=1.
- `nvme-ns`의 `shared=true`(기본값, `ns.c:1062`)면 **모든 컨트롤러(VF 포함) realize 시 자동 attach**(`ctrl.c:10467~10475`). 실측: VF에 attach-ns 시 "Namespace Already Attached".
- 그래서 게스트에서는 `nvme0n1`(multipath head) 아래 `nvme0c0n1`(PF 경로)·`nvme0c1n1`(VF 경로) 두 경로가 생김.

### 3.8 QEMU의 한계 / 특이점
| 항목 | 상태 |
|---|---|
| CMB / PMR + SR-IOV | 불가 (파라미터 검증에서 거부) |
| 자원 종류 | VQ, VI 두 가지뿐 |
| VF별 QoS·대역폭·중재(WRR) | 없음 — 모든 SQ를 동일하게 처리 |
| VF 독자 파라미터 | 없음 (PF 복사) |
| **VF도 OACS.VMS 광고** | VF가 PF params를 복사하므로 `oacs=0x1ab`(bit7 VMS 포함). 그러나 VF에서 virt-mgmt 발행 시 `Invalid Controller Identifier(0x411f)` (실측). 스펙상 VMS는 primary만의 기능이라 어색한 부분 |
| `drive=` 레거시 속성 | subsys와 함께 쓰면 거부 → `nvme-ns` 디바이스로 붙여야 함 |

---

## 4. Linux 7.2 구현 상세

### 4.1 PCI 코어 (`drivers/pci/iov.c`, 1385줄) — 범용 SR-IOV는 완비
sysfs (PF 디바이스 디렉토리):
| 속성 | 줄 | 역할 |
|---|---|---|
| `sriov_totalvfs` (RO) | :582 | SR-IOV cap의 TotalVFs |
| `sriov_numvfs` (RW) | :583, store :463 | VF 생성/삭제 → 드라이버 `.sriov_configure` 호출 |
| `sriov_offset`, `sriov_stride`, `sriov_vf_device` | :584~586 | cap 값 노출 |
| `sriov_drivers_autoprobe` (RW) | :587 | VF 생성 시 드라이버 자동 probe 여부 |
| `sriov_vf_total_msix` (RO) | :230 | PF 드라이버 `.sriov_get_vf_total_msix` 결과 |
| `sriov_vf_msix_count` (WO, VF 쪽) | :248 | PF 드라이버 `.sriov_set_msix_vec_count`로 **VF MSI-X 수 동적 변경** |

호출 체인:
```
echo N > /sys/bus/pci/devices/<PF>/sriov_numvfs
 └─ sriov_numvfs_store (iov.c:463)
     └─ pdev->driver->sriov_configure(pdev, N)
          nvme: pci_sriov_configure_simple (iov.c:1304)
           └─ sriov_enable (:649)
               ├─ VF BAR 자원 할당, NumVFs/VFE config write  ──► QEMU consume_config
               └─ sriov_add_vfs (:628) → pci_iov_add_virtfn (:346)
                    └─ pci_bus_add_device → (autoprobe면) 드라이버 매칭
                         └─ pci_device_probe → pci_device_can_probe (pci-driver.c:461)
                              VF && !autoprobe && !driver_override → -ENODEV
```
- **동적 MSI-X API**(`sriov_vf_msix_count`)는 NVMe의 **VI flexible 자원과 정확히 같은 개념**인데, 사용하는 드라이버는 mlx5·ice뿐(`drivers/net/.../mlx5/core/main.c:2286`, `ice_main.c:5857`). NVMe는 미구현.

### 4.2 NVMe 드라이버 — SR-IOV 지원은 사실상 한 줄
```c
/* drivers/nvme/host/pci.c:4305 */
.sriov_configure = pci_sriov_configure_simple,
```
| 항목 | 상태 | 근거 |
|---|---|---|
| VF enable/disable | O (PCI 코어에 위임) | 2016 `13880f5b57ad` 도입, 2018 `74d986abc20b`에서 simple로 교체. 이후 변경 없음 |
| Virtualization Management(0x1C) | **X** — opcode 이름만 (trace용) | `include/linux/nvme.h:1325`, `constants.c:46` |
| Identify CNS 0x14 (Primary Caps) | **정의조차 없음** | `include/linux/nvme.h:561~576`에 0x14 누락 |
| Identify CNS 0x15 (Secondary List) | enum만, 사용처 없음 | `:573` |
| OACS.VMS 확인 | X | |
| VF 자원 자동 할당/Online | X → VF가 Offline으로 생성되어 probe 실패 | §5 |
| `sriov_get_vf_total_msix` / `sriov_set_msix_vec_count` | X | |
| VF Offline 인지 | X — CSTS.CFS를 일반 fatal로만 취급 | `nvme_should_reset` (pci.c:1824) |
| PF 리셋 ↔ VF 조율 | X — PF 리셋이 VF를 Offline시켜도 VF 드라이버는 `live` 유지, 다음 명령 타임아웃 후 `dead` | §5 |

VF 쪽 probe는 PF와 완전히 같은 `nvme_probe`. VF가 Offline이면:
```
nvme_probe (pci.c:3747) → nvme_pci_enable (:3193) → nvme_pci_configure_admin_queue (:2332)
  → nvme_enable_ctrl (core.c:2744): CAP=0 이므로 CAP.TO=0 → nvme_wait_ready(core.c:2691) 즉시 타임아웃
  → "Device not ready; aborting initialisation, CSTS=0x2"  → probe -ENODEV
```

### 4.3 SPDK 비교
| 항목 | Linux NVMe | SPDK |
|---|---|---|
| opcode 0x1C 정의 | O | O (`nvme_spec.h:2827`) |
| CNS 0x14 / 0x15 정의 | 0x15만 | 둘 다 (`:3308`, `:3313`) + 4096B 구조체 (`:4327`, `:4368`) |
| OACS VMS 비트 | X | O (`:3524`) |
| 헬퍼 API | X | X (`spdk_nvme_ctrlr_cmd_admin_raw`로 직접) |
| NVMe-oF 타깃에서 | — | 컨트롤러 범위 명령으로 분류, 커스텀 핸들러 등록 가능 (`lib/nvmf/ctrlr.c:4962, 6630`) |

→ 둘 다 "스펙 정의는 있지만 관리 로직은 유저 몫". SPDK가 정의는 더 완전하다.

---

## 5. QEMU 게스트 실측

환경: `-machine q35 -smp 4 -m 1024 -accel tcg`,
`-device nvme-subsys,id=subsys0 -device nvme,id=nvme0,serial=deadbeef,subsys=subsys0,sriov_max_vfs=1,sriov_vq_flexible=2,sriov_vi_flexible=1 -device nvme-ns,drive=nvm0,bus=nvme0,nsid=1`
(기본 `max_ioqpairs=64`, `msix_qsize=65`)

| 단계 | 명령 | 결과 |
|---|---|---|
| 0 | i440fx(기본 `pc`)로 부팅 | PF에 `sriov_totalvfs` 파일 자체가 없음, `sriov_numvfs` 쓰기 Permission denied — 확장 config space 없음 |
| 0' | q35, 자원 할당 없이 `echo 1 > sriov_numvfs` (autoprobe=1) | VF `0000:00:03.1` 생성 → `nvme nvme1: Device not ready; aborting initialisation, CSTS=0x2` → probe 실패 |
| 1 | `nvme primary-ctrl-caps /dev/nvme0` | `crt 0x3, vqfrt 2, vqrfap 0, vqprt 63, vqfrsm 2, vifrt 1, viprt 64, vifrsm 1` |
| 1 | `nvme list-secondary /dev/nvme0` | SCID 0x1, PCID 0, **SCS Offline**, VFN 1, NVQ 0, NVI 0 |
| 2 | `echo 0 > sriov_drivers_autoprobe; echo 1 > sriov_numvfs` | VF 생성, 드라이버 미바인딩 |
| 3 | `nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 8 -n 2` / `-r 1 -a 8 -n 1` / `-a 9` | NRM 2, 1, 0 → **SCS Online, NVQ 2, NVI 1** |
| 4 | `echo 0000:00:03.1 > /sys/bus/pci/drivers/nvme/bind` | **`write error: No such device`** — autoprobe=0이면 `driver_override` 없이는 probe 거부 |
| 4' | `echo nvme > .../0000:00:03.1/driver_override; echo 0000:00:03.1 > /sys/bus/pci/drivers_probe` | `nvme nvme1: 1/0/0 default/read/poll queues` (NVQ 2 = admin 1 + IO 1) |
| 5 | VF `id-ctrl` | `cntlid 0x1`, `oacs 0x1ab`(VMS 포함). VF에서 virt-mgmt → `Invalid Controller Identifier` |
| 6 | `/sys/block` | `nvme0n1`, `nvme0c0n1`, `nvme0c1n1` — shared ns가 VF에 자동 attach, `dd` 읽기 OK |
| 7 | `nvme reset /dev/nvme0` (PF 컨트롤러 리셋) | secondary **Offline, NVQ 0, NVI 0** / VF 레지스터 `cc 0, csts 2` / 그러나 커널 VF 상태 **`live`** |
| 8 | VF로 admin 명령 (timeout 3s) | `controller is down; will reset: CSTS=0x2` → `Device not ready…` → `Disabling device after reset failure: -19` → 상태 **`dead`** |

해석:
- 디바이스 측(QEMU) 상태 머신은 스펙대로 정확히 움직인다.
- 호스트 측(커널)은 (a) VF 생성 전에 자원 할당이 필요하다는 것을 모르고, (b) PF 리셋이 VF를 무너뜨린다는 것을 모르며, (c) 무너진 VF를 다시 살릴 경로가 없다(unbind/bind 수동 필요).

---

## 6. 갭(Gap) 정리와 개발 후보

| # | 위치 | 아이디어 | 연결 지점 | 난이도 |
|---|---|---|---|---|
| K1 | 커널 nvme-pci | `sriov_configure`를 자체 구현: `numvfs` 쓰기 시 Primary Caps 읽기 → VF마다 VQ/VI 균등 할당 → Online → `sriov_enable` 순으로 처리해 **autoprobe 그대로 VF가 정상 probe** | `pci.c:4305`, `iov.c:1304` 대체 | 중 |
| K2 | 커널 nvme-pci | `sriov_get_vf_total_msix`/`sriov_set_msix_vec_count` 구현 → 기존 PCI sysfs(`sriov_vf_msix_count`)로 **VI flexible 자원 조정** (mlx5 방식 차용). VF가 unbound일 때만 허용 | `iov.c:230/248`, mlx5 `sriov.c:233` 참고 | 중 |
| K3 | 커널 nvme-pci | VQ 자원용 sysfs(예: `sriov_vf_nvq`) — 범용 PCI API가 없으므로 NVMe 전용 속성 | 신규 | 중 |
| K4 | 커널 nvme-pci | **PF 리셋 후 secondary 재-Online**: VF별 할당값을 PF가 기억했다가 `nvme_reset_work` 끝에 재할당·Online | `nvme_reset_work` (pci.c:3385), QEMU `ctrl.c:8073`과 짝 | 중상 |
| K5 | 커널 nvme core/pci | VF Offline 인지: VF에서 CSTS.CFS + CAP==0 패턴이면 `dead` 대신 대기/재시도(deferred probe 또는 주기 재검사) | `nvme_should_reset` pci.c:1824, `nvme_wait_ready` core.c:2691 | 중 |
| K6 | 커널 헤더/sysfs | CNS 0x14 정의 추가, Primary Caps·Secondary List를 sysfs/debugfs로 노출 | `include/linux/nvme.h:561` | 하 |
| Q1 | QEMU | VF별 QoS: secondary마다 IOPS/BW 한도(throttle-group 연동) 또는 SQ 중재(WRR/우선순위) — 커널 측 격리 실험의 기반 | `nvme_process_sq` 계열 | 중상 |
| Q2 | QEMU | VF의 OACS.VMS 광고 정리 (secondary는 VMS 비트 끄기) | `ctrl.c:9310` | 하 |
| Q3 | QEMU | secondary 상태 변화 시 Async Event 등 호스트 통지 실험 (스펙 범위 확인 필요) | AER 경로 | 중 |
| X1 | 양쪽 | K1+K4+Q1을 묶어 "VF 단위 자원·성능 격리"를 커널 정책으로 관리 | — | 상 |

추천 출발점: **K1 → K4 → K2** 순. K1만으로도 "echo N > sriov_numvfs 한 번에 VF가 바로 쓸 수 있게 되는" 체감 변화가 있고, QEMU 수정 없이 검증 가능하다.

---

## 7. 재현 절차 (게스트 내부, nvme-cli 2.8)

```sh
PF=/sys/bus/pci/devices/0000:00:03.0        # q35에서 PF 위치 (실측)
nvme primary-ctrl-caps /dev/nvme0
nvme list-secondary   /dev/nvme0
echo 0 > $PF/sriov_drivers_autoprobe          # 자원 없이 probe되는 것을 막음
echo 1 > $PF/sriov_numvfs                      # VF 0000:00:03.1 생성
nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 8 -n 2  # VQ 2개 (admin + IO 1)
nvme virt-mgmt /dev/nvme0 -c 1 -r 1 -a 8 -n 1  # VI 1개
nvme virt-mgmt /dev/nvme0 -c 1 -a 9            # Online
echo nvme > /sys/bus/pci/devices/0000:00:03.1/driver_override
echo 0000:00:03.1 > /sys/bus/pci/drivers_probe # → nvme1
```

함정 정리:
1. `-machine q35` 필수 (i440fx는 확장 config space 없음).
2. `nvme-subsys` 사용 시 `-device nvme,drive=…` 금지 → `-device nvme-ns,drive=…,bus=nvme0`.
3. autoprobe=1 상태로 numvfs를 먼저 쓰면 VF probe가 즉시 실패(CAP=0).
4. autoprobe=0이면 `drivers/nvme/bind`가 `-ENODEV` → `driver_override` 사용.
5. PF 리셋(`nvme reset`, CC.EN 토글 포함) 한 번에 모든 VF가 Offline + 자원 회수 → 커널 VF는 다음 명령에서 `dead`.
6. (미검증) QEMU는 VF 객체를 미리 만들어 두므로 numvfs 전에 virt-mgmt Online도 가능해 보인다(`nvme_virt_set_state`는 VF 객체 존재만 확인). 이 경우 autoprobe=1로 바로 쓸 수 있을지 확인 필요.

---

## 8. 코드 위치 색인

| 대상 | 위치 |
|---|---|
| QEMU SR-IOV 파라미터 문서 | `qemu-debug/qemu-11.1.2/docs/system/devices/nvme.rst:318` |
| QEMU Identify 0x14 / 0x15 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:5884`, `:5892` |
| QEMU Virtualization Management | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:7228~7392` |
| QEMU 자원 활성화 / 리셋 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:8005`, `:8028` |
| QEMU Offline VF MMIO 처리 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:8502`, `:8674`, `:8142` |
| QEMU 파라미터 검증 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:8818` |
| QEMU init_state / init_pci / init_sriov | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:8885`, `:9113`, `:9049` |
| QEMU OACS.VMS | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:9310` |
| QEMU NumVFs 감소 처리 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:9840` |
| QEMU PCIe SR-IOV 코어 | `qemu-debug/qemu-11.1.2/hw/pci/pcie_sriov.c:35, 67, 88, 152, 417, 446` |
| QEMU cntlid 예약 | `qemu-debug/qemu-11.1.2/hw/nvme/subsys.c:18~108` |
| Linux PCI IOV sysfs / enable | `qemu-debug/linux-7.2/drivers/pci/iov.c:230, 248, 463, 582~587, 628, 649, 1304` |
| Linux VF probe 게이트 | `qemu-debug/linux-7.2/drivers/pci/pci-driver.c:461` |
| Linux nvme sriov_configure | `qemu-debug/linux-7.2/drivers/nvme/host/pci.c:4305` |
| Linux nvme CFS 처리 | `qemu-debug/linux-7.2/drivers/nvme/host/pci.c:1824` |
| Linux nvme wait_ready | `qemu-debug/linux-7.2/drivers/nvme/host/core.c:2691` |
| Linux CNS / opcode 정의 | `qemu-debug/linux-7.2/include/linux/nvme.h:561~576`, `:1325` |
| SPDK 스펙 정의 | `spdk-study/include/spdk/nvme_spec.h:2827, 3308, 3313, 3524, 4327, 4368` |

---

## 9. VF BAR 공간과 상위 브리지 창 — "VF를 늘리면 메모리가 모자라나?"

### 9.1 결론
- **VF BAR 공간은 NumVFs가 아니라 TotalVFs 기준으로 enumeration 때 한 번에 잡는다.** `sriov_init`에서 IOV 리소스 크기 = VF BAR 크기 × TotalVFs (`qemu-debug/linux-7.2/drivers/pci/iov.c:872`).
  → 그 공간을 확보하는 데 성공했다면 **NumVFs를 TotalVFs까지 늘리는 동안 브리지 창이 더 필요해지는 일은 없다.**
- **`sriov_enable` 시점에는 공간을 새로 할당하지 않는다.** IOV 리소스가 할당돼 있지 않으면(`res->parent == NULL`) `not enough MMIO resources for SR-IOV`(-ENOMEM)로 끝난다 (`iov.c:685~690`). 이 경우에는 **VF 1개도 켤 수 없다.**
- 문제는 **부팅 시 펌웨어가 상위 브리지 창을 VF BAR 없이 PF BAR 기준으로만 잡은 경우**다. 커널은 기본적으로 펌웨어 배치를 유지하고, `CONFIG_PCI_REALLOC_ENABLE_AUTO`도 꺼져 있다(이 트리 `.config`). 그래서 IOV BAR가 할당되지 않은 채 남는다.

### 9.2 실측 (QEMU q35, PF를 `pcie-root-port` 아래에 배치)
설정: `nvme,max_ioqpairs=130,msix_qsize=65,sriov_max_vfs=64,sriov_vq_flexible=128,sriov_vi_flexible=64,sriov_max_vq_per_vf=64,sriov_max_vi_per_vf=1024`
→ VF BAR 32KB × 64 = **2MB** (`VF BAR 0 [mem 0x00000000-0x001fffff 64bit]: contains BAR 0 for 64 VFs`)

| 방법 | 루트 포트 mem 창 | VF BAR | `echo 4 > sriov_numvfs` |
|---|---|---|---|
| 기본 부팅 (SeaBIOS 배치 유지) | `fe800000-fe9fffff` (2MB, PF BAR만 고려) | `can't assign; no space` | **실패** `not enough MMIO resources for SR-IOV` |
| 기본 부팅 + `echo 1 > /sys/bus/pci/rescan`만 | 변화 없음 | 미할당 | **실패** |
| 기본 부팅 + 루트 포트 `remove` → `rescan` | `40000000-403fffff` (4MB로 재배치) | `assigned` | **성공**, 64개까지 (bus 01에 함수 65개) |
| 커널 인자 `pci=realloc` | 부팅 중 `40000000-403fffff`로 재배치 | `assigned` | **성공** (remove/rescan 불필요) |
| 펌웨어 예약 힌트 `pcie-root-port,...,mem-reserve=8M` | `fe200000-fe9fffff` (8MB) | 부팅 시 `assigned` | **성공** |

### 9.3 해석과 권장 순서
1. **펌웨어가 미리 예약하게 한다.** 실장비에서는 BIOS의 "SR-IOV Support" 옵션이고, QEMU에서는 루트 포트의 `mem-reserve`/`pref64-reserve`다. 이것이 가장 안전하다.
2. **안 되면 `pci=realloc`.** 부팅 중 브리지 창을 다시 잡는다. 부팅 시점이라 사용 중인 장치가 없다. 커널을 `CONFIG_PCI_REALLOC_ENABLE_AUTO=y`로 빌드하면 IOV 미할당을 감지했을 때 자동으로 켜진다(`drivers/pci/setup-bus.c:1827` `pci_realloc_detect`).
3. **remove/rescan은 최후 수단이다.**
   - rescan만으로는 이미 할당된 브리지 창을 키우지 않는다(실측).
   - **브리지째 remove해야 하므로 PF도 함께 제거된다.** PF의 namespace와 I/O가 모두 사라지므로 루트 FS가 그 SSD에 있으면 불가능하다.
   - 상위 창(호스트 브리지 또는 상위 스위치)에 빈 공간이 있어야 한다. 없으면 그 상위 레벨까지 remove해야 한다.
4. **창의 종류도 중요하다.** NVMe BAR(PF·VF 모두)는 64bit **non-prefetchable**이라 브리지의 32bit non-pref 창(4GB 아래)을 쓴다. VF 수 × VF BAR가 크면 4GB 아래의 귀한 MMIO 공간을 먹는다. 실측에서도 재배치된 창은 `0x40000000`대(32bit 영역)였다.
5. **버스 번호도 같은 성격의 제약이다.** VF Offset/Stride에 따라 VF가 다음 버스 번호로 넘어가면 브리지의 subordinate 범위 안에 있어야 한다. 아니면 `can't enable %d VFs (bus %02x out of range ...)` (`iov.c:693` 부근, 미검증 — QEMU NVMe는 ARI + Offset 1/Stride 1이라 255개까지 같은 버스에 들어간다).

### 9.3.1 PCIe 스위치 아래 SSD, 핫플러그 off (실측)
토폴로지: `pcie-root-port(hotplug=off)` → `x3130-upstream` → `xio3130-downstream(hotplug=off)` → nvme PF (`03:00.0`), VF BAR 합계 2MB.
핫플러그가 꺼져 있으면 펌웨어나 커널이 핫플러그용 여유 공간(`pci=hpmmiosize` 등)을 예약하지 않는다. 그래서 창이 필요한 만큼만 잡힌다.

| 방법 | RP / USP / DSP mem 창 | VF |
|---|---|---|
| 기본 부팅 | 셋 다 `fe800000-fe9fffff`(2MB). 커널이 "Some PCI device resources are unassigned, try booting with pci=realloc" 출력 | 4개도 실패 (`not enough MMIO resources`) |
| **스위치 USP remove → rescan** | RP 창(2MB)은 그대로 → USP/DSP에 3MB 필요 → `can't assign; no space` → 오히려 1MB(`fe800000-fe8fffff`)로 축소 | **여전히 실패** |
| **`pci=realloc`** | `max bus depth: 3 pci_try_num: 4` → 4번째 시도에서 셋 다 `40000000-402fffff`(3MB) | **4개, 64개 모두 성공** |

해석:
- remove/rescan은 **제거한 브리지의 부모 창에 여유가 있을 때만** 통한다. 스위치를 remove해도 그 위 루트 포트 창이 빡빡하면 실패한다. 이때는 여유가 있는 레벨(여기서는 루트 포트)까지 올라가서 remove해야 한다(§9.2에서 루트 포트 remove는 성공). 현장에서 "스위치 remove/rescan으로 됐다"는 것은 루트 포트 창이 원래 넉넉했다는 뜻이다.
- `pci=realloc`은 부팅 중 할당 실패 시 최대 `버스 깊이 + 1`번 재시도한다. 2번째까지는 말단 브리지 창만, 3번째부터는 하위 트리 전체의 창을 풀고 상위부터 다시 잡는다(`drivers/pci/setup-bus.c:2182` `pci_assign_unassigned_root_bus_resources`). 그래서 다단 스위치도 한 번에 TotalVFs 기준으로 맞춘다.
- `pci=realloc`이 **통하지 않는** 경우:
  1. 최상위 호스트 브리지 창(ACPI `_CRS`) 자체가 모자랄 때. 특히 NVMe BAR는 non-prefetchable이라 4GB 아래 창만 쓸 수 있다.
  2. 부팅 **후**에 장치의 SR-IOV 구성(TotalVFs, VF BAR 크기, SR-IOV cap 노출 여부)이 바뀔 때. 예를 들어 벤더 도구로 모드를 바꾸고 리셋한 경우에는 부팅 시 계산이 낡은 값이 되므로 재열거(remove/rescan 또는 재부팅)가 필요하다.
  3. 펌웨어가 부팅 구성 보존(`preserve_config`, ACPI `_DSM`)을 요구하는 플랫폼. 이때 자동 realloc은 꺼진다(`setup-bus.c:1837`). 명시적 `pci=realloc`의 효과는 플랫폼에 따라 확인이 필요하다(미검증).
  4. 부팅 중 다른 장치의 BAR 주소도 바뀔 수 있다. 펌웨어가 쓰던 장치(EFI 프레임버퍼 등)에 영향을 줄 수 있어서 배포판 기본값이 꺼져 있는 것이다.

### 9.3.2 최대치는 어디서 읽나 — TotalVFs와 VF BAR 크기

예약 크기는 **VF BAR 크기 × TotalVFs**다. 두 값이 enumeration 이후 커지지 않는다면 VF를 몇 개로 바꾸든 문제가 없다.

| 무엇 | 하드웨어 원본 (PCIe SR-IOV Ext Cap, ID 0x0010) | 커널 캐시 / 사용자 공간 |
|---|---|---|
| TotalVFs | cap+0x0E (RO, HwInit) | `sriov_totalvfs` (드라이버가 `pci_sriov_set_totalvfs`로 낮춘 값일 수 있음) |
| InitialVFs | cap+0x0C | — |
| NumVFs | cap+0x10 | `sriov_numvfs` |
| VF Offset / Stride | cap+0x14 / +0x16 | `sriov_offset` / `sriov_stride` |
| VF Device ID | cap+0x1A | `sriov_vf_device` |
| Supported / System Page Size | cap+0x1C / +0x20 | — (VF BAR는 System Page Size 단위로 정렬됨) |
| VF BAR0~5 | cap+0x24~0x38 (크기는 sizing으로 측정) | `resource` 파일 8~13번째 줄(IOV 리소스 7~12) = **VF BAR × TotalVFs**, `lspci -vvv`의 SR-IOV 블록 |
| VF Resizable BAR | Ext Cap ID 0x24 | `pci_iov_vf_bar_get_sizes()` |
| NVMe 쪽 상한 | Identify CNS 0x14 (VQFRSM/VIFRSM), CNS 0x15 (secondary 수) | `nvme primary-ctrl-caps`, `nvme list-secondary` |

게스트 실측 (q35 + 스위치 + `pci=realloc`, PF `03:00.0`):
```
sriov_totalvfs=64  sriov_numvfs=0  sriov_offset=1  sriov_stride=1  sriov_vf_device=10
resource 7: 0x40000000-0x401fffff (2MB = 32KB × 64)
ext cap @0x100 id=0x000e(ARI) → @0x120 id=0x0010(SR-IOV)
InitialVFs=64 TotalVFs=64 NumVFs=0 VFOffset=1 VFStride=1 VFDID=0x0010
SupPgSize=0x553 SysPgSize=0x1(4K) VF BAR0=0x40000004 (64bit, non-prefetchable)
```
(busybox만 있을 때: `od -A n -t u2 -j $((0x120+0x0e)) -N 2 /sys/bus/pci/devices/<PF>/config`. lspci가 있으면 `lspci -vvv -s <PF>`.)

커널이 이 값을 언제 읽는지가 핵심이다.
- `sriov_init()`가 TotalVFs를 **enumeration 때 한 번** 읽는다(`linux-7.2/drivers/pci/iov.c:827`, 저장 `:882`). 호출 경로는 `pci_device_add → pci_iov_init`(`probe.c:2663`)뿐이다.
- 장치 리셋 후에는 `sriov_restore_state`(`iov.c:960`)가 Control·VF BAR 주소·System Page Size·NumVFs를 **써 넣기만** 하고 TotalVFs와 VF BAR 크기는 다시 읽지 않는다.
- 따라서 부팅 후 장치가 TotalVFs나 VF BAR 크기를 바꾸면 **커널은 옛 값으로 동작한다.** `nr_virtfn > iov->total_VFs`면 `-EINVAL`(`iov.c:672`), 예약 공간을 넘으면 `not enough MMIO resources`.

### 9.3.3 동적으로 바뀌는 경우의 대처

| 경우 | 공간 문제 | 대처 |
|---|---|---|
| NumVFs만 변경 (TotalVFs 이내) | 없음 | 그대로 사용 |
| 드라이버가 상한을 낮춤 (`pci_sriov_set_totalvfs`, `iov.c:1261`) | 없음 (예약은 HW TotalVFs 기준 유지) | 그대로 사용 |
| VF Resizable BAR로 VF BAR 크기 변경 (`pci_iov_vf_bar_set_size`, `iov.c:1345`) | 예약은 그대로 → 커진 BAR × VF 수가 예약을 넘으면 enable 실패 | `pci_iov_vf_bar_get_sizes(dev, resno, num_vfs)`로 **고정 예약 안에서 (VF 수 × 크기)를 고른다** (커널이 의도한 방식) |
| 장치 TotalVFs·VF BAR가 **감소** (벤더 설정 + 리셋) | 없음. 다만 커널 캐시가 실제보다 큼 | `pci_sriov_set_totalvfs`로 새 값에 맞춰 제한하거나 재열거 |
| 장치 TotalVFs·VF BAR가 **증가** | 있음 | 아래 절차 |

증가 시 절차 (뒤로 갈수록 영향 범위가 크다):
1. **예방이 최선이다.** 장치를 **최대 SR-IOV 구성으로 둔 상태에서 부팅**해 그 기준으로 예약(펌웨어 SR-IOV 옵션 / `mem-reserve` / `pci=realloc`)하고, 실제 사용 수는 런타임에 NumVFs·`pci_sriov_set_totalvfs`·NVMe virt-mgmt로 줄인다. "최대로 잡고 소프트웨어로 줄인다."
2. **PF만 재열거**: VF 사용 중지 → `sriov_numvfs=0` → PF 드라이버 사용 중지 → `echo 1 > <PF>/remove` → `echo 1 > <DSP>/rescan`(또는 `/sys/bus/pci/rescan`). `sriov_init`이 다시 돌면서 새 TotalVFs/VF BAR를 읽는다. **바로 위 브리지(DSP) 창 안에 들어갈 때만** 성공한다.
3. **브리지 재열거**: 안 들어가면 부모 창에 여유가 있는 가장 낮은 브리지(DSP → 스위치 USP → RP)까지 remove → rescan. **그 아래의 모든 장치(같은 스위치의 다른 SSD 포함)가 함께 제거된다**(§9.3.1 실측: 부모 창이 빡빡하면 실패).
4. **재부팅 + `pci=realloc`**: 호스트 브리지까지 빡빡하면 새 구성 상태로 재부팅해 처음부터 다시 잡는다.

드라이버(nvme-pci)에 넣을 만한 보호 장치 (설계 후보 K7):
- PF probe/reset 끝에서 **live TotalVFs(cap+0x0E)와 캐시(`iov->total_VFs`)를 비교**하고, VF BAR는 VFs가 꺼져 있을 때만 sizing해 비교한다.
- 감소면 `pci_sriov_set_totalvfs(min)`으로 자동 제한하고, 증가면 "SR-IOV 구성 변경 감지: 재열거(remove/rescan) 또는 재부팅 필요" 경고를 남긴다.
- NVMe 쪽 secondary 수(CNS 0x15)와 TotalVFs가 다르면 작은 쪽으로 제한한다.

### 9.4 드라이버 설계에 주는 함의
- 동적 자원 설계(`[[nvme-sriov-dynamic-resource-design]]`)의 VQ/VI 변경은 **VF BAR 크기를 바꾸지 않는다.** QEMU VF BAR는 VQFRSM/VIFRSM 최대치로 고정되어 있다(`hw/nvme/ctrl.c:9152`). 그래서 런타임 자원 변경 때문에 브리지 창이 부족해지는 일은 없다.
- 대신 `sriov_configure`(K1)는 `not enough MMIO resources`일 때 원인(IOV BAR 미할당)과 해결책(`pci=realloc`, 펌웨어 예약)을 로그로 안내하면 좋다.
