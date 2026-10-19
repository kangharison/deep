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
| Q4 | QEMU | multi-PF SR-IOV: `ari-nextfn`, `sriov_vf_offset`/`sriov_vf_stride` 속성 — **구현·검증 완료**(qemu-dev `045e8fd98f`, §11.4.1~11.4.2) | `pcie.c:1238`, `ctrl.c:9180` | 하 |
| Q5 | QEMU | NVM Subsystem Reset 에뮬레이션: `nssrs=on`이면 CAP.NSSRS 광고, NSSR 쓰기로 subsystem 전 컨트롤러 리셋 + CSTS.NSSRO — **구현·검증 완료**(qemu-dev `43e5e6baec`, §11.4.4). PCIe link-down은 미모델 | `ctrl.c:8257, 8379` | 하 |
| K7 | 커널 nvme-pci/PCI | SR-IOV **구성 변경 감지**: PF probe/reset 때 live TotalVFs·VF BAR와 커널 캐시 비교 → 감소면 `pci_sriov_set_totalvfs`로 제한, 증가면 재열거 필요 경고 (§9.3.3) | `iov.c:827, 1261` | 하 |
| K8 | 커널 PCI | SR-IOV capability **제자리 재판독**: VFs가 꺼진 상태에서 PF를 떼지 않고 TotalVFs/VF BAR를 다시 읽고, 기존 IOV 예약 안이면 갱신 (§9.3.4) | `iov.c:796 sriov_init` | 중 |
| K9 | 커널 PCI | **부팅 시 IOV 과예약** `pci=sriov_reserve=<PF>:<max_vfs>[:<max_bar>]` (§9.3.5) | `pci.c:6743`, `iov.c:872` | 중 |
| K10 | 커널 nvme-pci/PCI | PF FLR 뒤 **VF 쪽 통지**: `pci_reset_function(PF)`는 PF 드라이버만 `reset_prepare/done`을 받아 VF의 config(command, MSI-X)가 조용히 0이 된다. PF `reset_done`에서 VF들을 reset/re-probe하거나 PCI 코어가 VF config도 저장·복원 (§11.4.4-5) | `nvme_reset_done` pci.c:3866, `pci_restore_iov_state` iov.c:1118 | 중 |
| K11 | 커널 nvme core | NSSR 뒤 **같은 subsystem의 다른 컨트롤러 리셋 예약**: 지금은 ioctl을 낸 컨트롤러도 바로 LIVE로 돌아가고, 나머지는 각자 타임아웃이 나야 복구된다 (§11.4.4-4) | `nvme_pci_subsystem_reset` pci.c:1702, `nvme_should_reset` pci.c:1824 | 중 |
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

### 9.3.4 목표: remove 없이 운영하기

원칙: **부팅 때 앞으로 필요할 최대치까지 예약하고, 이후에는 그 예약 안에서만 움직인다.** 커널에는 사용 중인 브리지 창을 런타임에 키우는 수단이 없다.
- 런타임 창 재배치는 Resizable BAR 경로(`pci_resize_resource` → `pci_do_resource_release_and_resize` → `pbus_reassign_bridge_resources`, `linux-7.2/drivers/pci/setup-bus.c:2308, 2377`)뿐이다. 이 경로도 **할당된 자식 리소스가 있는 창은 풀지 않는다**("was not released (still contains assigned resources)"). PF BAR0나 같은 스위치의 다른 SSD가 쓰는 창은 키울 수 없다.
- 핫플러그 여유(`pci=hpmmiosize` 등)는 `is_hotplug_bridge`인 브리지에만 더해진다(`setup-bus.c:1432`). 핫플러그 off 환경에서는 효과가 없다.

| 상황 | remove 없이 가능? | 방법 |
|---|---|---|
| TotalVFs·VF BAR가 하드웨어 고정 (일반적인 SSD, TotalVFs는 HwInit) | **가능** | 펌웨어 SR-IOV 예약 또는 `pci=realloc`으로 한 번 부팅(§9.3.1 실측). 이후 NumVFs 변경은 예약 안 |
| 장치 구성이 바뀌지만 **최대 구성으로 부팅할 수 있음** | **가능** | 최대 구성으로 부팅해 예약 → 런타임에는 줄이기만. 단 아래 K8 필요 |
| 부팅 예약보다 **커져야 함** | **현재 커널로는 불가** | 부팅 시 여유 예약(K9) 없이는 창을 키울 방법이 없음 |

주의 — "줄이기만"에도 함정이 있다:
- VF i의 BAR 주소는 커널이 `IOV 리소스 시작 + i × (캐시된 VF BAR 크기)`로 계산한다(`linux-7.2/drivers/pci/iov.c:377~379` `pci_iov_add_virtfn`).
- 하드웨어는 자기의 새 VF BAR 크기로 디코딩한다. 그래서 부팅 후 VF BAR 크기가 바뀌었는데 커널이 다시 읽지 않으면 **VF 1번부터 주소가 어긋난다.**
- TotalVFs만 줄어든 경우는 `pci_sriov_set_totalvfs`로 제한하면 된다. VF BAR 크기 변화는 반드시 커널이 다시 읽어야 한다.

remove를 없애기 위한 커널 개발 후보:
- **K8. SR-IOV capability 제자리 재판독**: VFs가 꺼진 상태(VFE=0, MSE=0)에서 PF를 떼지 않고 TotalVFs/InitialVFs/Offset/Stride/VF BAR 크기를 다시 읽는다. 새 요구량(VF BAR × TotalVFs)이 **이미 할당된 IOV 리소스 안에 들어가면** `iov->total_VFs`, `iov->barsz[]`, 리소스 크기만 갱신한다. 넘으면 실패 + 안내. PF 드라이버(nvme)는 바인딩된 채로 PF BAR0와 I/O를 유지한다. VF Resizable BAR의 `pci_iov_vf_bar_set_size`가 "예약은 그대로, 크기만 변경"하는 것과 같은 철학이다.
- **K9. 부팅 시 IOV 과예약**: 커널 인자(예: `pci=sriov_reserve=<BDF>:<max_vfs>:<max_vf_bar>`)나 드라이버 quirk로 `sriov_init` 직후 IOV 리소스를 **최대 VF 수 × 최대 VF BAR**로 키운다. 그러면 `pci=realloc`이 그 크기로 상위 창(RP/스위치)을 잡는다. 선례: PowerNV는 PE 정렬 때문에 IOV 리소스를 `VF BAR × total_pe_num`으로 확장한다(`linux-7.2/arch/powerpc/platforms/powernv/pci-sriov.c:203`).
- K8 + K9를 함께 쓰면 "부팅 때 최대치 예약 → 런타임에 장치 구성 변경 → 제자리 재판독 → VF 생성"이 **remove 없이** 닫힌다. 동적 자원(VQ/VI) 설계([[nvme-sriov-dynamic-resource-design]])와 합치면 NVMe SR-IOV 전체를 무중단 쪽으로 가져갈 수 있다.

### 9.3.5 최대치를 커널에 어떻게 전달하나 (K9 구현 위치)

**nvme 드라이버 안의 define이나 모듈 파라미터로는 늦다.** 실행 순서 때문이다.
```
pci_device_add()                                  linux-7.2/drivers/pci/probe.c:2726
  ├ pci_fixup_device(pci_fixup_header)            :2746   헤더 quirk (sriov_init 전)
  ├ pci_init_capabilities → pci_iov_init           :2663
  │    └ sriov_init: TotalVFs 판독 → IOV 리소스 = VF BAR × TotalVFs   iov.c:827, :872
  └ pcibios_device_add()                          :2760   아키텍처 훅 (PowerNV가 여기서 IOV 확장)
fs_initcall(pcibios_assign_resources)             arch/x86/pci/i386.c:376   브리지 창 확정
module_init(nvme_init) = device_initcall          drivers/nvme/host/pci.c:4330   nvme probe
```
nvme probe 시점에는 창이 이미 확정되어 사용 중이고, 사용 중인 창은 키울 수 없다(§9.3.4). 따라서 **`sriov_init` 직후, 브리지 sizing 전**에 개입해야 하고, 그 코드는 PCI 코어(또는 아키텍처 훅)에 있어야 한다.

| 전달 방법 | 위치 | 장점 | 단점 |
|---|---|---|---|
| **A. 커널 인자** `pci=sriov_reserve=<BDF 또는 VID:DID>:<max_vfs>[:<max_vf_bar>]` | 파싱: `pci_setup()` (`drivers/pci/pci.c:6743`, `hpmmiosize=`·`resource_alignment=`와 같은 곳). 적용: `sriov_init` | 재빌드 없이 장비마다 조정, 실험에 적합 | 부트로더 설정 관리 필요 |
| **B. 컴파일 타임 테이블 (define 성격)** VID:DID(+subsystem ID) → max_vfs/max_vf_bar | `drivers/pci/iov.c`(또는 `quirks.c`)의 정적 테이블을 `sriov_init`에서 조회. upstream이라면 IOV 전용 fixup 단계를 신설 | 제품군 고정이면 설정이 필요 없음 | 커널 재빌드, 제품마다 테이블 추가 |
| C. 기존 fixup 매크로 (`DECLARE_PCI_FIXUP_HEADER/FINAL`) | `quirks.c` | 새 인프라 불필요 | HEADER는 `sriov_init` **전**이라 덮어써짐. FINAL은 x86 부팅에서는 할당 전에 돌지만, 핫애드나 rescan에서는 할당 **후**라 순서가 보장되지 않음(`drivers/acpi/pci_root.c:739~756`) → 비권장 |
| D. 아키텍처 훅 `pcibios_device_add` | `arch/*` (PowerNV 선례 `arch/powerpc/kernel/pci-common.c:1085`) | 타이밍이 정확함 | x86 공용 경로가 아님 |
| E. 펌웨어 예약 | BIOS SR-IOV/MMIO 예약, ACPI, QEMU `mem-reserve` | 커널 수정 불필요 (§9.2 실측) | 펌웨어마다 다르고 세밀한 제어가 어려움 |
| F. 장치가 처음부터 최대치를 보고 | SSD 펌웨어: TotalVFs·VF BAR를 항상 최대로 노출하고 실제 수는 NumVFs·virt-mgmt로 제어 | 커널 수정 불필요, 가장 깔끔 | 벤더 펌웨어에 의존 |

A/B의 적용 스케치 (`sriov_init`, `iov.c:872` 부근):
```c
	iov->barsz[i] = resource_size(res);               /* 실제 VF BAR 크기 — VF 주소 계산용, 그대로 둔다 */
	res_vfs = max(total, pci_sriov_reserve_vfs(dev)); /* A: 커널 인자, B: 정적 테이블 조회 */
	res_bar = max(iov->barsz[i], pci_sriov_reserve_barsz(dev, i));
	resource_set_size(res, res_bar * res_vfs);        /* 예약만 최대치로 키운다 */
```
- `iov->barsz[]`와 `iov->total_VFs`는 **실제 하드웨어 값**을 유지한다. VF 주소는 `시작 + i × barsz`로 계산되므로(`iov.c:377~379`) 예약만 크게 잡아도 지금 VF들은 정상이다.
- `sriov_enable`의 검사 `vf_bar_sz * nr_virtfn > resource_size(res)`(`iov.c:685` 부근)는 큰 예약에서도 통과한다.
- 버스 번호: `compute_max_vf_buses`(`iov.c:110`)도 TotalVFs 기준이다. 최대 VF 수가 다음 버스로 넘어갈 수 있는 장치라면 버스 예약도 같은 최대치로 계산해야 한다. QEMU NVMe처럼 ARI + Offset 1/Stride 1이면 255개까지 같은 버스에 있으므로 해당하지 않는다.
- 이후 장치 구성이 커지면 K8(제자리 재판독)이 `total_VFs`/`barsz`를 이 예약 범위 안에서 갱신한다.

QEMU 검증 방법: 장치 `sriov_max_vfs=64`, 커널 인자로 128을 예약 → `resource` 7번째 줄이 2MB가 아니라 4MB인지, `pci=realloc` 시 RP/스위치 창이 그만큼 커지는지, VF 64개 enable이 정상인지 확인한다. QEMU는 TotalVFs를 런타임에 바꿀 수 없으므로 K8은 QEMU 쪽에 테스트용 확장이 필요하다.

### 9.4 드라이버 설계에 주는 함의
- 동적 자원 설계(`[[nvme-sriov-dynamic-resource-design]]`)의 VQ/VI 변경은 **VF BAR 크기를 바꾸지 않는다.** QEMU VF BAR는 VQFRSM/VIFRSM 최대치로 고정되어 있다(`hw/nvme/ctrl.c:9152`). 그래서 런타임 자원 변경 때문에 브리지 창이 부족해지는 일은 없다.
- 대신 `sriov_configure`(K1)는 `not enough MMIO resources`일 때 원인(IOV BAR 미할당)과 해결책(`pci=realloc`, 펌웨어 예약)을 로그로 안내하면 좋다.

---

## 10. User space 드라이버(SPDK + vfio-pci)라면 이 문제들이 없나?

결론: **PCI 주소 공간(BAR/브리지 창/TotalVFs 예약) 문제는 똑같다.** 누가 장치를 구동하든 BAR와 브리지 창은 커널 PCI 코어가 배정하기 때문이다. 차이가 나는 것은 NVMe 동작 계층(인터럽트, 큐 수, 복구 정책)뿐이다.

### 10.1 PCI 계층 — 동일
- VF 생성은 user space에서 직접 할 수 없다. vfio-pci는 SR-IOV 확장 capability를 **읽기 전용**으로 노출한다(`linux-7.2/drivers/vfio/pci/vfio_pci_config.c:87, 324` — `ecap_perms`가 readfn만 있음). 그래서 VF는 커널 sysfs `sriov_numvfs` → `vfio_pci_sriov_configure`(`drivers/vfio/pci/vfio_pci.c:209`, 모듈 인자 `enable_sriov=1` 필요) → `vfio_pci_core_sriov_configure`(`vfio_pci_core.c:2350`) → **`pci_enable_sriov`** 경로로만 생긴다. §9의 `not enough MMIO resources for SR-IOV`가 그대로 적용된다.
- uio_pci_generic은 `sriov_configure`가 없다. config space를 직접 써서 VFE를 켜면 커널은 VF `pci_dev`를 만들지 않고 VF BAR도 배정하지 않아 동작하지 않는다.
- 따라서 TotalVFs 기준 예약, `pci=realloc`, remove/rescan 필요성, K8/K9 모두 user space 드라이버에도 똑같이 해당한다.

### 10.2 NVMe 계층 — 다른 점
| 항목 | 커널 nvme-pci | SPDK (vfio-pci 위) | 근거 |
|---|---|---|---|
| VI(MSI-X) 증가 시 `-ERANGE`로 dead | 발생 (실측) | **기본 polling 모드면 무관**(`enable_interrupts=false`). 인터럽트 모드면 vfio도 같은 PCI/MSI 코어 도메인으로 할당하므로 같은 문제가 예상됨(미검증). vfio는 MSI-X 테이블 크기를 `vfio_pci_core_enable`(`vfio_pci_core.c:587`, 651줄)에서 open 때마다 다시 읽는다 | 커널 실측 §설계문서 1, SPDK 코드 |
| VQ 증가 반영 | 리셋 시 `blk_mq_update_nr_hw_queues`로 반영 (실측 1→3) | **반영 안 됨.** `opts.num_io_queues = min(응답, opts.num_io_queues)`로 자기 설정을 덮어써서(`spdk-study/lib/nvme/nvme_ctrlr.c:5901`) 한 번 줄면 리셋해도 다시 늘지 않는다. 늘리려면 detach 후 재-attach. 앱이 qpair를 직접 다시 만들어야 함 | 코드 (미검증) |
| VQ 감소 | 리셋 시 hctx 축소 | 줄어든 수만큼 qid 풀(`free_io_qids`, `:5920`)이 작아짐 → 초과 qpair는 재연결 실패 → 앱이 처리 | 코드 |
| Offline VF (CAP=0) 초기화 | CAP.TO=0 → 즉시 실패 → dead (실측) | ready timeout도 `CAP.TO × 500ms`(`nvme_ctrlr.c:2597`)라 즉시 실패하는 것은 같다. 다만 bdev_nvme에 **재연결 정책**(`reconnect_delay_sec`, `ctrlr_loss_timeout_sec`)이 있어 실패 후에도 I/O를 보류하고 주기적으로 재시도할 수 있다(`module/bdev/nvme/bdev_nvme.c:1977`). 기본값은 0(비활성, `:403~404`) | 코드 (PCIe VF 오프라인 복구는 미검증) |
| PF 측 virt-mgmt | 없음 (nvme-cli passthrough) | 헬퍼 API는 없지만 앱이 raw admin 명령으로 직접 발행 가능 | §4.3 |
| PF 리셋 → VF Offline | VF dead | 장치 동작은 같음. 재연결 정책을 켜면 Online 복귀 뒤 회복할 여지가 있음 | — |

### 10.3 정리
- user space 드라이버로 바꿔도 **"remove 없이 VF 늘리기"의 전제(부팅 시 최대치 예약)는 그대로**다. 오히려 VF 생성이 반드시 커널 `pci_enable_sriov`를 거치므로 커널 쪽 K8/K9가 user space 사용자에게도 필요하다.
- 동적 VQ/VI 측면에서는 SPDK가 polling 덕분에 VI 문제를 피하고 재연결 정책으로 자가 복구(P3)에 유리하다. 반면 VQ 증가는 재-attach 없이는 반영하지 못한다. 커널은 정반대 성향이다(VQ 증가는 자동 반영, 복구 정책은 약함).

---

## 11. Multi-PF 장치

Multi-PF = 한 장치(같은 버스·같은 상위 브리지 창)에 PF가 여러 개 있고, PF마다 자기 SR-IOV capability(TotalVFs, VF Offset/Stride, VF BAR)와 NVMe primary controller를 가진 구성이다. 듀얼 포트 SSD처럼 PF마다 다른 호스트에 연결되는 경우도 있다.

### 11.1 PCI 계층
| 항목 | 동작 | 근거 |
|---|---|---|
| PF 발견 | ARI가 켜진 포트 아래에서는 커널이 **PF0 ARI capability의 Next Function Number 사슬**을 따라 PF를 찾는다. VF는 이 사슬에 없다 | `linux-7.2/drivers/pci/probe.c:2798` `next_ari_fn` |
| ARI Capable Hierarchy | 첫 PF만 설정하고, 이후 PF는 기존 PF를 찾으면 ARI 비트 없이 쓴다(스펙상 가장 낮은 번호 PF에서만 유효) | `iov.c:815~823` |
| IOV 공간 | **PF마다 따로** `VF BAR × TotalVFs`를 잡지만 모두 **같은 상위 브리지 창** 안에 있어야 한다 → 필요량 = Σ(PF BAR) + Σ(PF별 VF BAR × TotalVFs) | `iov.c:872` |
| VF RID/버스 | PF별 VF = PF RID + Offset + n×Stride. PF끼리 겹치지 않게 Offset을 정하는 것은 장치의 책임이다. 커널은 같은 버스 PF들의 최대 VF 버스 수로 버스 범위를 예약한다 | `iov.c:1144` `pci_iov_bus_range`, `:110` |
| Function Dependency Link | PF끼리 의존 관계면 `dep_link` sysfs를 만들고, 대상이 PF인지만 확인한다 | `iov.c:705~719` |
| PF 하나만 bus reset | **불가.** 같은 버스에 다른 함수(다른 PF 또는 활성 VF)가 있으면 `pci_parent_bus_reset`이 `-ENOTTY` → PF 단위 리셋은 FLR에 의존한다. bus reset은 모든 PF와 VF를 함께 리셋한다 | `linux-7.2/drivers/pci/pci.c:4873` |

### 11.2 NVMe 계층
- PF마다 별도의 primary controller다. flexible 자원 풀(CNS 0x14)과 secondary 목록(CNS 0x15)이 PF별로 독립이고, virt-mgmt의 CNTLID도 자기 secondary만 대상으로 한다(QEMU `nvme_sctrl_for_cntlid`는 자기 `sec_ctrl_list`만 검색).
- 같은 NVM subsystem을 공유하면 cntlid 공간과 namespace를 공유한다. QEMU는 PF마다 자기 뒤에 VF 수만큼 cntlid를 연속 예약한다(`hw/nvme/subsys.c:60~96`). 그래서 shared namespace는 PF와 VF 전부에서 multipath 경로가 된다. QEMU 실측: subsystem 안 컨트롤러는 serial이 같아야 한다(다르면 `invalid controller serial`).
- 리셋 범위:
  | 리셋 | 영향 |
  |---|---|
  | PF 컨트롤러 리셋(CC.EN) / PF FLR | 그 PF의 secondary만 Offline (QEMU `ctrl.c:8073`은 자기 목록만 순회). 실측: PF0 `nvme reset` → PF0의 SCID 1,2가 Offline |
  | 링크 / secondary bus reset, AER fatal 복구 | 장치의 **모든 PF와 모든 VF** |
  | NVM Subsystem Reset (NSSR) | subsystem의 모든 컨트롤러(모든 PF와 VF) |

### 11.3 설계에 주는 함의
- 지금까지의 설계(P1 자원 관리자, K1 `sriov_configure`, K4 PF 리셋 복원)는 `nvme_dev` 즉 **PF 단위**라 그대로 적용된다.
- 추가로 필요한 것:
  1. **PF 간 공통 이벤트 조율**: bus reset·AER fatal·NSSR은 모든 PF의 VF를 한꺼번에 Offline시킨다. 각 PF의 K4 복원이 동시에 돌아도 되도록 PF별 잠금과 재시도가 필요하고, 같은 subsystem의 PF끼리 순서(예: 낮은 PF부터)를 정해 두면 디버깅이 쉽다.
  2. **K9 과예약은 PF 단위 인자**여야 한다(`pci=sriov_reserve=<PF BDF>:...`를 PF마다). 상위 창 크기는 모든 PF 예약의 합으로 정해진다.
  3. **K8 제자리 재판독도 PF 단위**: 다른 PF가 I/O 중이어도 해당 PF의 VF만 끄고 재판독할 수 있다. 공통 브리지 창은 건드리지 않는다.
  4. 벤더에 따라 PF들이 flexible 풀을 공유하는 장치가 있을 수 있다. 그런 경우 PF 간 자원 중재가 필요하다(스펙 범위 확인 필요, 미검증).
  5. 듀얼 포트(호스트별 PF)라면 각 호스트가 자기 PF만 보므로 예약과 복원도 호스트별로 독립이다. 다만 NSSR이나 subsystem 공유 namespace는 호스트 간 영향을 준다.

### 11.4 QEMU로 재현할 때의 한계 (실측)
- 구성: `xio3130-downstream` 아래 `nvme,addr=0x0.0`(PF0)와 `nvme,addr=0x8.0`(PF1), 같은 `nvme-subsys`, 각각 `sriov_max_vfs=4`.
- 결과: 게스트 커널에 **PF0(03:00.0)과 그 VF(03:00.1~)만 보이고 PF1(03:08.0)은 열거되지 않음.**
- 원인: QEMU `pcie_ari_init`이 ARI Next Function Number를 0으로 고정한다(`qemu-11.1.2/hw/pci/pcie.c:1227`, 호환용 플래그일 때만 1). 커널의 `next_ari_fn`은 사슬이 끝난 것으로 판단한다.
- QEMU 개발 후보 **Q4**: NVMe에 ARI next-function 지정(PF0 → PF1 사슬)과 VF Offset/Stride 설정 속성을 추가해 진짜 multi-PF SR-IOV를 에뮬레이션한다. 현재는 `NVME_VF_OFFSET 1`/`STRIDE 1` 고정(`hw/nvme/ctrl.c:231~233`)이라 PF0의 VF와 PF1 번호가 겹치지 않도록 PF1을 멀리 배치해야 한다. 대안으로 PF를 서로 다른 downstream 포트에 두면 "같은 subsystem의 별개 장치"가 되므로 NVMe 계층 실험(리셋 범위, cntlid, multipath)은 가능하지만 PCI 계층(같은 창, ARI 사슬)은 재현되지 않는다.

#### 11.4.1 Q4 구현 — `ari-nextfn`, `sriov_vf_offset`, `sriov_vf_stride` (qemu-dev `045e8fd98f`)
| 변경 | 내용 | 위치 |
|---|---|---|
| PCIe 공통 | `pcie_ari_init_nextfn(dev, offset, nextfn)` 추가. 기존 `pcie_ari_init`은 이 함수를 0(또는 호환 플래그 시 1)로 호출하므로 다른 장치 동작은 그대로 | `qemu-11.1.2/hw/pci/pcie.c:1225, 1238` |
| `ari-nextfn=<ARI fn>` | PF의 ARI Next Function Number. 기본 0 = 사슬 끝. `sriov_max_vfs` 필수, 자기 ARI 함수 번호(=devfn)보다 커야 함. VF는 PF params를 복사하지만 항상 0을 광고(VF는 Offset/Stride로 찾으므로 필드 미사용) | `hw/nvme/ctrl.c:8830~8858, 9180` |
| `sriov_vf_offset` / `sriov_vf_stride` | SR-IOV cap의 First VF Offset / VF Stride. 기본 1/1(기존 `NVME_VF_OFFSET`/`NVME_VF_STRIDE` 대체). offset 0, (VF>1일 때) stride 0은 거부 | `hw/nvme/ctrl.c:8842~8851, 9097` |
| 겹침/범위 검사 | 새로 만들지 않고 기존 두 검사를 그대로 씀: `pcie_sriov_pf_init_common`의 `VF addr overflows`(마지막 VF devfn ≥ 256), QEMU 버스의 slot 점유 검사(`PCI: slot 0 function 1 not available for nvme, in use by nvme`) | `hw/pci/pcie_sriov.c:108` |

ARI 함수 번호는 8비트 devfn 그대로다. `addr=0x8.0` → ARI fn 64, `addr=0x10.0` → 128, `addr=0x0.1` → 1. VF n의 RID = PF devfn + offset + n×stride.

거부 실측:
```
nvme,...,ari-nextfn=8 (sriov 없음)        → ari-nextfn requires sriov_max_vfs
sriov_vf_offset=0                         → sriov_vf_offset must be non-zero
sriov_vf_stride=0 (sriov_max_vfs=2)       → sriov_vf_stride must be non-zero when sriov_max_vfs > 1
addr=0x0.2,ari-nextfn=1                   → ari-nextfn must be greater than this function's ARI function number
offset=200,stride=60 (VF 2개)             → VF addr overflows
PF0 0x0.0(기본 1/1, VF 4개) + PF1 0x0.1    → PCI: slot 0 function 1 not available for nvme, in use by nvme
```
마지막 경우는 PF0의 VF0이 devfn 1을 먼저 점유했기 때문이다. QEMU는 VF 객체를 PF realize 때 미리 만들기 때문에 겹침이 장치 생성 시점에 드러난다.

#### 11.4.2 게스트 실측 (linux-7.2, Q4 적용 후)
공통 구성: q35, `pcie-root-port(hotplug=off)` → `x3130-upstream` → `xio3130-downstream(hotplug=off)`, `nvme-subsys` 하나, 두 PF 모두 `serial=mpf0`, PF마다 namespace 1개(nsid 1, 2), 커널 인자 `pci=realloc`. 스크립트: `deep/scripts/sriov/run-mpf.sh`(A/B/C/BAD), 게스트 init `deep/scripts/sriov/mpf-init.sh`(nvme-cli 2.8 irfs 복사본). 게스트 동작: 두 PF 모두 `sriov_drivers_autoprobe=0` → `sriov_numvfs` → 첫 secondary 2개에 virt-mgmt(VQ 2, VI 1, Online) → VF 2개를 `driver_override=nvme`로 바인딩 → PF0 `nvme reset`.

| 시나리오 | PF0 / PF1 속성 | 열거 결과 | VF RID |
|---|---|---|---|
| **A** (원래 문제 구성) | PF0 `addr=0x0.0,ari-nextfn=64` / PF1 `addr=0x8.0` (기본 offset/stride) | **03:00.0, 03:08.0 둘 다 열거** (Q4 전에는 03:08.0 없음) | PF0 → 03:00.1, .2 / PF1 → 03:08.1, .2 |
| **B** (조밀 배치) | PF0 `0x0.0,ari-nextfn=1,offset=2,stride=2` / PF1 `0x0.1,offset=2,stride=2` | 03:00.0, 03:00.1 | PF0 → 03:00.2, .4 / PF1 → 03:00.3, .5 (교차) |
| **C** (창 합산) | PF0 `0x0.0,ari-nextfn=128` / PF1 `0x10.0`, 각 `sriov_max_vfs=64` | 03:00.0, 03:10.0 | 각 PF **64개 enable 성공** |

PF별 sysfs와 config (A):
```
PF 03:00.0 (nvme1): totalvfs=4 offset=1 stride=1 vf_device=10
  ARI cap@0x100: hdr=1201000e (ID 0x0e, ver 1, next cap 0x120=SR-IOV)  cap=4000 → NFN=0x40(64)
  resource 8번째 줄(IOV BAR0): 0xfe808000-0xfe817fff (64KB = 16KB × 4)
PF 03:08.0 (nvme0): totalvfs=4 offset=1 stride=1
  ARI cap=0000 → NFN=0 (사슬 끝, 기본값 유지 확인)
  resource 8번째 줄: 0xfe818000-0xfe827fff (64KB)
```
B는 PF0 `cap=0100`(NFN=1), PF1 `cap=0000`, 두 PF 모두 `offset=2 stride=2`. C는 PF0 `cap=8000`(NFN=128).

cntlid / secondary (A, B 동일):
```
nvme1 03:00.0 cntlid=0   (PF0)  secondary SCID 1,2,3,4 (VFN 1~4)
nvme2 03:00.1 cntlid=1   nvme3 03:00.2 cntlid=2
nvme0 03:08.0 cntlid=5   (PF1)  secondary SCID 6,7,8,9
nvme4 03:08.1 cntlid=6   nvme5 03:08.2 cntlid=7
```
QEMU subsystem이 PF마다 "자기 cntlid + VF 수"를 연속 예약하는 것(§11.2)이 그대로 보인다. C에서는 PF1 cntlid=65, SCID 0x42~0x81. namespace는 공유되어 `/dev/nvme0n1`, `/dev/nvme0n2` 두 개만 생기고(multipath head), 6개 컨트롤러가 경로가 된다.

**DSP 창이 두 PF의 IOV 합을 담는지:**
| 시나리오 | IOV 합 | DSP(02:00.0) mem 창 | 배치 |
|---|---|---|---|
| A/B (VF 4개 × 16KB × 2 PF) | 128KB + PF BAR 2×16KB | `fe800000-fe9fffff`(2MB, 펌웨어 기본 그대로) | PF0 BAR, PF1 BAR, PF0 IOV, PF1 IOV 순으로 한 창 안 |
| **C** (VF 64개 × 16KB × 2 PF) | **2MB** + PF BAR 2×16KB | 펌웨어 2MB 창 부족 → `pci=realloc` 4번째 시도(`max bus depth: 3 pci_try_num: 4`)에서 **`50000000-502fffff`(3MB)** | `50000000` PF0 BAR / `50004000-50103fff` PF0 IOV(1MB) / `50104000` PF1 BAR / `50108000-50207fff` PF1 IOV(1MB) |
C의 RP·USP·DSP 창이 모두 3MB로 맞춰졌다. 창 크기는 **모든 PF의 TotalVFs 기준 IOV 합 + PF BAR**을 1MB 단위로 올린 값이고, 중간 시도에서 `VF BAR 0 [mem size 0x00100000]: can't assign; no space`가 두 PF 모두에 대해 찍힌 뒤 최종 성공한다. 즉 §11.3-2의 "상위 창 크기는 모든 PF 예약의 합"이 실측으로 확인됐다.

**PF0 `nvme reset` 영향 (A, B, C 동일):**
| | 리셋 전 | PF0 리셋 후 |
|---|---|---|
| PF0 secondary (SCID 1,2) | Online | **Offline** |
| PF1 secondary (SCID 6,7 / C는 0x42,0x43) | Online | **Online 유지** |
| 커널 컨트롤러 state (nvme2/3 = PF0의 VF) | live | **live로 남음** |

- PF 컨트롤러 리셋은 그 PF의 secondary만 Offline으로 만든다(QEMU `ctrl.c:8073`은 자기 `sec_ctrl_list`만 순회). 같은 장치·같은 subsystem의 다른 PF는 영향 없다.
- 커널은 PF0의 VF가 Offline이 된 것을 **알아채지 못한다.** VF 드라이버 상태가 `live`로 남아 있다가 다음 I/O나 admin 명령에서야 실패한다 → K4(PF 리셋 후 재-Online)와 K5(VF Offline 인지)가 multi-PF에서도 PF 단위로 필요하다는 근거.

#### 11.4.3 남은 한계
- bus reset·NSSR·FLR처럼 **리셋 범위가 다른 이벤트**는 §11.4.4에서 실측했다. NSSR은 원래 QEMU가 지원하지 않아(CAP.NSSRS=0) Q5로 구현했지만, 실제 장치처럼 PCIe link가 내려가는 동작은 에뮬레이션하지 않는다.
- QEMU는 TotalVFs·Offset·Stride를 장치 생성 시 고정한다. 스펙상 Offset/Stride는 NumVFs나 ARI Capable Hierarchy 비트에 따라 바뀔 수 있지만, 여기서는 상수다.
- flexible 자원 풀은 PF별 독립이다(§11.2). PF 간 풀 공유는 에뮬레이션하지 않는다.

#### 11.4.4 모든 PF에 걸치는 리셋 실측 — bus reset / NSSR / FLR
**구성**: 시나리오 A(PF0 `03:00.0,ari-nextfn=64`, PF1 `03:08.0`, DSP `02:00.0`, 같은 `nvme-subsys`). 두 PF 모두 VF 2개 enable → 첫 secondary 2개에 VQ 2·VI 1 할당 + Online → VF를 nvme에 바인딩한 상태에서 시작한다. 이때 secondary는 PF0 SCID 1,2 / PF1 SCID 6,7이 Online이고, 커널 컨트롤러 6개(nvme0 PF0, nvme1 PF1, nvme2·3 PF0의 VF, nvme4·5 PF1의 VF)가 모두 `live`이다.

**도구**: 게스트 init `deep/scripts/sriov/mpf-reset-init.sh`(커널 인자 `mpf_test=pfbus|sbr|sbrraw|nssr|flr`), 실행은 `run-mpf.sh A`에 `INITRD=irfs-rst.cpio.gz EXTRA="mpf_test=<시험> nvme_core.admin_timeout=10 nvme_core.io_timeout=10"`를 준다. NSSR은 `QPROP=",nssrs=on" QARGS="-trace pci_nvme_mmio_nssr"`를 더 준다. 각 시험은 따로 부팅하고 이벤트 뒤에 아래 순서로 본다.
1. 5초 뒤 snapshot: 커널 컨트롤러 state, PF의 커널 `sriov_numvfs`, 장치 config(PF IOV Control `0x128`, NumVFs `0x130`, command), VF config(command, MSI-X Message Control).
2. `nvme list-secondary`로 secondary 상태와 VQ/VI 할당을 본다.
3. probe: 컨트롤러마다 admin Identify 1개를 보낸다. "다음 명령"이 들어올 때 커널이 무엇을 하는지 보려는 것이다.
4. 15초 뒤 다시 snapshot을 찍는다.

QEMU는 qemu-dev `43e5e6baec`(Q5, NSSR 에뮬레이션)이다.

##### 결과 요약
| 이벤트 | 트리거 | PF0 secondary | PF1 secondary | 장치 config | 커널이 아는 시점 | 최종 커널 상태 |
|---|---|---|---|---|---|---|
| PF `reset_method=bus` | `echo bus > 03:00.0/reset_method` | — | — | — | 쓰기 거부 `-EINVAL` | 변화 없음(아래 1) |
| **SBR, 커널 조율** | `echo 1 > 02:00.0/reset_subordinate` | **Offline**, VQ/VI 0 | **Offline**, VQ/VI 0 | 커널이 저장·복원(IOV ctrl `0019`/`0009`, NumVFs 2) | 즉시(`reset_done`) | PF 2개 live, **VF 4개 dead** (0.2초 안) |
| **SBR, 커널 모름** | DSP Bridge Control `0x0002→0x0042→0x0002` 직접 쓰기 | (PF가 죽어 조회 불가) | (조회 불가) | **두 PF 모두 command·IOV ctrl·NumVFs = 0, VF 4개 소멸**(config `ffff`) | 다음 admin 명령 타임아웃(10초) | **6개 모두 dead**, 커널 `sriov_numvfs`는 2로 남음 |
| NSSR (기본 QEMU) | `nvme subsystem-reset /dev/nvme0` | — | — | — | `NVM Subsystem Reset not supported` (`-ENOTTY`) | 변화 없음 |
| **NSSR (`nssrs=on`)** | 위와 같음 | **Offline**, VQ/VI 0 | **Offline**, VQ/VI 0 | 그대로(VF cmd `0406`, MSI-X `8000`) | 컨트롤러마다 자기 다음 명령이 타임아웃 났을 때 | PF 2개 live, **VF 4개 dead** |
| **PF0 FLR** | `echo flr > reset_method; echo 1 > reset` | **Offline**, VQ/VI 0 | **Online 유지** | PF0 IOV는 복원, **PF0 VF의 cmd `0406→0000`, MSI-X `8000→0000`** / PF1 VF는 그대로 | PF0은 즉시, PF0의 VF는 다음 명령 타임아웃 | nvme0 live, **nvme2·3 dead**, PF1 쪽 3개 live |

모든 경우에 공통인 점이 있다. **secondary가 Offline으로 바뀌면 QEMU는 그 secondary의 VQ/VI 할당을 0으로 되돌린다.** `nvme_virt_set_state(.., false)`가 `nvme_update_virt_res(.., 0)`을 부르기 때문이다(`qemu-11.1.2/hw/nvme/ctrl.c:7332`). 그래서 커널이 VF를 다시 살리려면 Online 전환만으로는 부족하고 **자원 재할당부터 다시** 해야 한다(K4 범위). 실제로 리셋된 VF는 커널의 재초기화 시점에 `CSTS=0x2`(CFS) 또는 `0x12`(CFS|NSSRO)를 보이고, `Device not ready; aborting initialisation` → `Disabling device after reset failure: -19`를 거쳐 `dead`가 된다(`drivers/nvme/host/pci.c:3487`).

##### 1. PF `reset_method=bus` — 커널이 거부
```
before: PF0 reset_method: flr bus
echo bus > reset_method  → nvme 0000:03:00.0: Unsupported reset method 'bus'   (write error: Invalid argument)
```
`pci_parent_bus_reset`(`drivers/pci/pci.c:4865`)은 **버스에 자기 말고 다른 장치가 하나라도 있으면** `-ENOTTY`를 돌려준다. multi-PF에서는 PF1과 VF들이 같은 버스에 있으므로 PF 단위 bus reset은 성립하지 않는다. 그런데 `reset_method`는 처음에 `flr bus`를 보였다. 리셋 방법 목록은 장치를 추가할 때 한 번만 probe되는데, 그때는 PF0이 버스에 혼자였기 때문이다. 이 목록이 PF1·VF가 추가된 뒤에도 갱신되지 않고 남은 것이다. 이후 `echo 1 > reset`은 목록 첫 항목인 FLR로 수행되어 결과가 아래 5와 같았다.

##### 2. DSP `reset_subordinate` — 커널이 조율하는 SBR
```
DSP reset_subordinate rc=0
nvme0: 4/0/0 default/read/poll queues            ← PF0 reset_done → 재초기화 성공
nvme1: 4/0/0 default/read/poll queues            ← PF1 동일
nvme2..5: Device not ready; aborting initialisation, CSTS=0x2
nvme2..5: Disabling device after reset failure: -19
secondary PF0: 1 Offline q0 i0 | 2 Offline q0 i0 ...   PF1: 6 Offline q0 i0 | 7 Offline q0 i0 ...
```
- QEMU 쪽 경로는 다음과 같다. Bridge Control의 SBR 비트가 0→1로 바뀌면 `bus_cold_reset(sec_bus)`가 실행된다(`hw/pci/pci_bridge.c:276`). 그 결과 버스 아래 모든 함수가 리셋된다. PF마다 `nvme_pci_reset` → `nvme_ctrl_reset(FUNCTION)`이 돌아 자기 secondary를 모두 Offline으로 만들고, `pcie_sriov_pf_reset`이 VF Enable·NumVFs를 0으로 만들어 VF를 해제한다(`hw/pci/pcie_sriov.c:446`). **두 PF의 secondary가 모두 Offline**이 되는 것이 확인됐다.
- 커널 쪽 경로는 `pci_try_reset_bridge`(`drivers/pci/pci.c:5691`)이다. 버스 아래 모든 장치(PF와 VF 모두)에 대해 config 저장 → `reset_prepare` → SBR → config 복원 → `reset_done` 순으로 진행한다. PF의 IOV 상태는 `pci_restore_iov_state`(`drivers/pci/iov.c:1118`)가 되살리므로 VF가 다시 생기고 VF의 config도 복원된다. 이후 6개 컨트롤러 모두 `nvme_reset_done`(`pci.c:3866`)에서 재초기화된다. 결과는 PF는 성공, VF는 secondary가 Offline이라 CFS로 실패이다.
- 커널은 리셋을 직접 일으켰으므로 **즉시** 알고, 타임아웃을 기다리지 않는다. 그래도 VF를 살리지 못한다. 재초기화 순서를 보면 PF가 먼저 live가 되지만 PF 드라이버가 secondary를 다시 Online으로 돌리는 경로가 없기 때문이다. K4가 있으면 이 경우는 바로 해결된다. PF의 `reset_done`이 VF의 `reset_done`보다 먼저 끝난다는 보장이 필요하다.

##### 3. Bridge Control 직접 쓰기 — 커널이 모르는 SBR
```
DSP 0000:02:00.0 Bridge Control=0x0002, raw SBR (bit6): set -> 0x0042, clear -> 0x0002
after +5s: nvme0..5 state=live (6개 모두)
  PF 03:00.0: kernel numvfs=2 | dev IOV ctrl=0000 NumVFs=0000 cmd=0000
  PF 03:08.0: kernel numvfs=2 | dev IOV ctrl=0000 NumVFs=0000 cmd=0000
  VF 03:00.1/.2, 03:08.1/.2: config 읽기 ffff (함수 없음)
nvme0: I/O tag 18 opcode 0x6 (Admin Cmd) QID 0 timeout, reset controller
nvme0: enabling device (0000 -> 0002)
nvme0: Device not ready; aborting initialisation, CSTS=0x0
nvme2..5: Unable to change power state from D3cold to D0, device inaccessible → Disabling device after reset failure: -22
```
- 커널이 저장·복원을 하지 않으므로 SBR 뒤에는 장치가 리셋 직후 상태 그대로다. PF BAR, command, SR-IOV가 모두 0이고 VF는 존재하지 않는다. 반면 커널 자료구조는 VF 2개 × 2, 컨트롤러 6개 `live`로 남는다.
- 복구 경로를 보면, PF는 admin 타임아웃 → `nvme_reset_work` → `pci_enable_device`가 command의 memory 비트만 켠다. BAR는 0으로 남으므로 CSTS를 0으로 읽고 dead가 된다. VF는 config 접근 자체가 실패해 D3cold로 판정되고 dead가 된다.
- **6개 모두 dead**이며 PF 자체도 살아나지 않는다. 실제 하드웨어에서는 이런 경우를 link-down/AER(Advanced Error Reporting)/DPC(Downstream Port Containment)가 감지해 `pci_error_handlers` 경로로 처리한다. QEMU의 xio3130 DSP는 SBR로 그런 이벤트를 만들지 않으므로 이 실험은 "알림 없는 리셋"의 최악 사례에 해당한다.

##### 4. NSSR — QEMU 미지원 → Q5 구현 후 실측
**기본 QEMU**는 CAP.NSSRS=0이다. 커널은 `ctrl->subsystem = NVME_CAP_NSSRC(cap)`(`core.c:3749`)가 0이므로 `nvme_reset_subsystem`(`nvme.h:747`)에서 `-ENOTTY`를 돌려주고, nvme-cli는 `NVM Subsystem Reset not supported`를 출력한다. 장치에는 아무 일도 없다. 원래 QEMU는 NSSR 레지스터 쓰기를 받아도 trace만 남겼다.

**Q5 (qemu-dev `43e5e6baec`)**:
| 변경 | 내용 | 위치 |
|---|---|---|
| `nssrs=<bool>` | 기본 off. on이면 CAP.NSSRS=1 (VF는 PF params를 복사하므로 VF도 광고) | `hw/nvme/ctrl.c:9452, 9863` |
| NSSR 쓰기 | 값이 `0x4e564d65`("NVMe")이고 NSSRS이면 `nvme_subsys_reset` 호출. subsystem의 `ctrls[]`에 등록된 **모든 컨트롤러**(PF, VF)에 대해 `nvme_ctrl_reset(FUNCTION)` 후 CSTS.NSSRO=1 | `ctrl.c:8257, 8379` |
| CSTS.NSSRO | 컨트롤러 리셋(CC.EN 1→0, FLR)이 일어나도 유지되고 호스트가 1을 써야만 지워진다(W1C) | `ctrl.c:8040, 8362` |
| 미모델 | 실제 장치에서 NSSR이 일으키는 PCIe link-down. config(BAR, MSI-X, NumVFs)는 그대로 둔다 | — |

```
nvme subsystem-reset /dev/nvme0 → rc=0
pci_nvme_mmio_nssr NVM subsystem reset by cntlid 0, 10 controllers reset
after +5s: nvme0..5 state=live (6개 모두), VF cmd 0406 / MSI-X 8000 그대로
[22.7] nvme0: controller is down; will reset: CSTS=0x10, PCI_STATUS=0x10   → 4/0/0 queues (복구)
[33.0] nvme1: controller is down; will reset: CSTS=0x10                   → 복구
[43.2] nvme2: controller is down; will reset: CSTS=0x12 → Device not ready, CSTS=0x12 → dead
[53.3] nvme3 / [63.5] nvme4 / [73.6] nvme5: 같은 경로로 dead
secondary PF0: 1..4 Offline q0 i0   PF1: 6..9 Offline q0 i0
```
- **QEMU NSSR 범위**: `10 controllers reset` = PF 2개 + VF 객체 8개(PF마다 TotalVFs=4만큼 미리 realize되어 subsystem에 등록되어 있다). 두 PF의 secondary가 **모두 Offline**이 되었다. 즉 NSSR은 PCI 위치와 관계없이 **subsystem 단위**로 퍼진다. 코드가 `subsys->ctrls[]`만 순회하므로 다른 DSP 아래 PF가 같은 subsystem에 있어도 같이 리셋될 것이다(미실측).
- **커널 경로**: `nvme_pci_subsystem_reset`(`pci.c:1702`)은 RESETTING → NSSR 쓰기 → CONNECTING → LIVE로 상태만 한 바퀴 돌린 뒤 CSTS를 한 번 읽는다. 주석에 적힌 의도는 "pcie read error 유발"이다. 실제 장치라면 link-down으로 이 읽기가 실패하고 AER/DPC 복구가 시작된다. QEMU에는 link-down이 없으므로 읽기는 정상 값(`0x10`)을 돌려주고 ioctl을 낸 컨트롤러도 **바로 live로 돌아온다.**
- 그 뒤에는 **컨트롤러마다 자기 다음 명령이 타임아웃 나야** `nvme_should_reset`(`pci.c:1824`)이 CSTS.NSSRO를 보고 리셋한다. 여기서는 snapshot의 `list-secondary`와 probe가 컨트롤러에 명령을 하나씩 순서대로 보냈고 `admin_timeout=10`이라 10초 간격으로 처리됐다. 명령이 가지 않는 idle 컨트롤러는 **계속 `live`인 줄 알고 남는다.** I/O가 걸려 있는 경로는 기본 `io_timeout` 30초 동안 멈춘 뒤에야 복구에 들어간다. PF1은 PF0과 같은 subsystem인데도 **커널 쪽에서 subsystem 단위로 묶어 처리하는 로직이 없다**(K11).
- VF는 CSTS=`0x12`(CFS|NSSRO)로 끝나 dead가 된다. 이유는 SBR과 같다(secondary Offline + VQ/VI 0).

##### 5. PF0 FLR — PF0과 그 VF만
```
PF0 reset_method=flr → reset rc=0
nvme0: 4/0/0 default/read/poll queues     ← PF0만 reset_prepare/done
after +5s: nvme0..5 state=live
  PF 03:00.0: dev IOV ctrl=0019 NumVFs=0002            ← 커널이 복원
    VF 03:00.1/.2: cmd=0000 msix ctrl=0000              ← VF 자신의 config는 복원되지 않음
  PF 03:08.0: IOV ctrl=0009 NumVFs=0002
    VF 03:08.1/.2: cmd=0406 msix ctrl=8000              ← 영향 없음
secondary PF0: 1 Offline q0 i0 | 2 Offline q0 i0   PF1: 6 Online q2 i1 | 7 Online q2 i1
[24.3] nvme2: Admin Cmd timeout, reset controller → Device not ready, CSTS=0x2 → dead
[34.4] nvme3: 같은 경로로 dead
nvme4, nvme5 (PF1의 VF): cntlid 6, 7 정상 응답
```
- QEMU 쪽: FLR 비트 쓰기 → `pcie_cap_flr_write_config` → `pci_device_reset(PF0)`(`hw/pci/pcie.c:959`)가 실행된다. 리셋은 `nvme_pci_reset`(`ctrl.c:9919`)과 `pcie_sriov_pf_reset`(VF 해제)으로 이어진다. 범위는 **PF0과 PF0의 VF뿐**이고 PF1 secondary는 Online과 할당(q2 i1)을 그대로 유지한다. §11.4.2의 `nvme reset`(CC.EN 기반 Controller Reset)과 secondary 영향 범위는 같다. 차이는 FLR이 PCI 계층까지 리셋하므로 **VF의 PCI 함수도 해제·재생성된다**는 점이다.
- 커널 쪽: `pci_reset_function(PF0)`은 **PF0 하나만** 저장·복원한다. PF0의 IOV 상태 복원으로 VF가 다시 생기지만 **VF 드라이버는 `reset_prepare/done`을 받지 않는다.** 그 결과 VF의 command(memory/bus master)와 MSI-X Enable이 0인 채로 남는다. VF 컨트롤러는 `live`로 보이지만 인터럽트도 MMIO 디코드도 꺼져 있다. 다음 명령이 타임아웃 난 뒤에야 `nvme_reset_work`가 `pci_enable_device`로 config를 다시 세운다. 그런데 이번에는 secondary Offline 때문에 CFS가 나서 dead가 된다.
- 즉 PF FLR 뒤 VF가 다시 쓸 수 있게 되려면 두 가지가 모두 필요하다. (a) VF PCI 상태 재설정(K10), (b) secondary 재할당·Online(K4).

##### 부수 관찰: ARI Capable Hierarchy는 PF0에만
PF0의 IOV Control은 `0x0019`(VF Enable | VF MSE | ARI Capable Hierarchy), PF1은 `0x0009`로 ARI 비트가 없다. 커널 `sriov_init`(`drivers/pci/iov.c:815~825`)은 같은 버스에 SR-IOV PF가 이미 있으면 그 PF를 따라가고 ARI 비트를 새로 켜지 않는다. 그래서 첫 PF만 ARI Capable Hierarchy를 켠다. 스펙도 이 비트는 가장 낮은 번호의 PF에서만 의미가 있다고 정한다. QEMU는 PF마다 이 비트를 쓸 수 있게(`wmask`) 두지만 Offset/Stride가 고정이라 결과에는 차이가 없다.

##### 정리 — 리셋 범위 행렬
```
                         QEMU에서 Offline 되는 secondary     커널이 아는가     커널 최종 상태 (VF)
PF Controller Reset      PF 자신 것만                        모름 (VF는 live)   다음 명령에서 dead      (§11.4.2)
PF FLR                   PF 자신 것만 + VF PCI 함수 재생성     PF만 앎            다음 명령에서 dead      (5)
PF bus reset             —                                   커널이 거부         —                       (1)
DSP SBR (조율)            모든 PF                              전부 앎            즉시 dead               (2)
DSP SBR (raw)             모든 PF + PF config 소실              모름               PF 포함 전부 dead        (3)
NSSR                     subsystem 전체 (PCI 위치 무관)         ioctl 주체만 잠깐   PF는 복구, VF는 dead,
                                                                                  컨트롤러마다 타임아웃 하나씩 (4)
```
결론적으로 **어떤 리셋이든 VF가 살아남는 경우는 없다.** 지금 커널은 "secondary가 Offline이 되었다"는 사실을 VF의 CFS로만 간접 확인하고 그 시점에 포기한다. multi-PF에서는 리셋 범위가 PF 하나(FLR, Controller Reset), 버스(SBR), subsystem(NSSR)으로 다르므로, 커널 측 복구도 같은 단위로 설계해야 한다.
- PF 단위: K4(자원 재할당 + Online)와 K10(VF PCI 재설정).
- subsystem 단위: K11(같은 subsystem의 컨트롤러 일괄 리셋 예약)과, 그 뒤에 PF마다 K4.
