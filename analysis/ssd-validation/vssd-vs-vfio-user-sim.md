# 커스텀 NVMe 드라이버 검증 백엔드 — vssd(QEMU NVMe) vs SSD 시뮬레이터(HW sim + 실 FW) over vfio-user

> 전제: 검증 대상 SSD는 **HW는 시뮬레이션, FW는 실제 코드**로 구성되어 host에서 돈다(사용자 제공 정보, 2026-10-20).
> 근거: QEMU `v11.1.2`(+qemu-dev Q4/Q5) 소스 코드. vfio-user 연결 자체는 아직 **미실측**이다.
> 관련: [[nvme-sriov-multipf-lifecycle]] · [[nvme-sriov-qemu-vs-kernel]] · [[06_qemu_nvme_device_emulation]]

---

## 1. 결론
- **둘 중 하나가 아니라 두 백엔드를 같은 테스트로 돌린다.**
  - **SSD 시뮬레이터 + vfio-user = 주 검증 대상(제품 동작).** 실제 FW의 명령 처리, 상태 머신, 벤더 명령, 펌웨어 업데이트, 시뮬레이션 HW의 지연·오류를 그대로 받는다.
  - **vssd = 기준(reference) 겸 빠른 회귀.** 스펙대로 동작하는 이상적 장치다. 같은 테스트에서 두 백엔드의 결과가 다르면 FW 버그이거나 드라이버의 잘못된 가정이다.
- 단, **지금 QEMU로는 vfio-user 경로에서 SR-IOV와 ARI(multi-PF)가 게스트에 보이지 않는다.** SR-IOV·multi-PF 드라이버 기능은 QEMU 수정(Q7) 전까지 vssd에서만 검증할 수 있다.

## 2. 비교
| 항목 | vssd (`-device nvme`) | SSD sim + vfio-user (`-device vfio-user-pci`) |
|---|---|---|
| 동작 근거 | QEMU의 스펙 구현 | **실 FW + HW 모델** |
| SR-IOV / multi-PF / NSSR | 가능 (Q4·Q5 확장 완료) | **불가**: QEMU가 SR-IOV·ARI ext cap을 버림(`hw/vfio/pci.c:2700~2701`) |
| 타이밍·오류 현실성 | 지연 편차·GC·NAND 오류 없음 | HW sim이 모델링하는 만큼 |
| 벤더 명령, FW 업데이트 | 없음 (FW 슬롯 RO 1개, `hw/nvme/ctrl.c:9414`) | FW 그대로 |
| 디버깅 | 단일 프로세스, QEMU trace | 다중 프로세스. FW·sim을 gdb로 직접 볼 수 있음. QEMU `vfio_user_*` trace |
| 재현성 | 높음 | sim의 시간 모델에 따라 다름 (§3.2) |
| 준비 비용 | 없음 | sim PCIe EP ↔ libvfio-user 브리지 (§3.1) |

## 3. vfio-user 브리지에서 해야 할 일

### 3.1 단일 PF (1단계)
| 항목 | sim 쪽 (libvfio-user 서버) | QEMU 쪽 |
|---|---|---|
| 연결 | `vfu_create_ctx` + 소켓 | `-device '{"driver":"vfio-user-pci","socket":{"path":"/tmp/ssd-pf0.sock","type":"unix"}}'` |
| config space | sim PCIe EP의 config를 그대로 제공(`vfu_pci_init`, cap). 단 QEMU가 SR-IOV/ARI는 숨김 | — |
| BAR0 (레지스터 + doorbell) | (a) MMIO 콜백: 접근마다 소켓 메시지 왕복 / (b) **doorbell 페이지를 sparse mmap으로 공유하고 sim이 polling**. HW sim이 레지스터를 폴링하는 구조라면 (b)가 자연스럽다 | (b)면 QEMU가 그 페이지를 게스트에 직접 매핑 |
| MSI-X | `vfu_setup_device_nr_irqs` + `vfu_irq_trigger` → eventfd | 게스트 MSI 주입 |
| DMA | sim DMA 엔진이 `vfu_addr_to_sgl`/`vfu_sgl_read/write` 또는 매핑된 포인터로 접근 | **게스트 RAM을 fd 공유 백엔드로**: `-object memory-backend-memfd,id=mem,size=4G,share=on -machine q35,memory-backend=mem`. 아니면 DMA마다 `VFIO_USER_DMA_READ/WRITE` 메시지가 오가 매우 느림(`docs/interop/vfio-user.rst:182~187`, `hw/vfio-user/container.c:90~131`) |
| 리셋 | `vfu_setup_device_reset_cb`(FLR, 장치 리셋, 연결 끊김) → sim PCIe 리셋. CC.EN 컨트롤러 리셋과 NSSR은 FW가 처리 | QEMU가 FLR/리셋 시 `VFIO_USER_DEVICE_RESET` 전송 |

### 3.2 시간 모델 — 가장 먼저 결정할 것
- 게스트 커널은 **벽시계**로 타임아웃을 잰다(nvme `admin_timeout` 60초, `io_timeout` 30초). 이 호스트는 VirtualBox 게스트라 **KVM이 없고 QEMU는 TCG로만** 돈다(`/dev/kvm` 없음, 실측).
- sim이 가상 시간(이벤트 구동, 사이클 정확)으로 실시간보다 느리게 돌면, 드라이버 타임아웃이 FW 지연과 무관하게 발동한다. 선택지는 다음과 같다.
  1. sim을 벽시계에 맞춰 free-running으로 돌린다(지연 수치가 실제와 다를 수 있음).
  2. 게스트 타임아웃을 sim 배속에 맞게 늘린다(`nvme_core.io_timeout=`, `admin_timeout=`).
  3. 타이밍 검증 항목은 sim 내부 시간으로 측정하고, 호스트에서는 기능만 검증한다.
- 로그 상관 분석을 위해 sim, QEMU trace, 게스트 로그에 공통 시간축(호스트 CLOCK_MONOTONIC)을 기록한다.

### 3.3 SR-IOV / multi-PF (3단계, QEMU 개발 Q7)
QEMU에는 이미 **사용자 생성 VF** 경로가 있다. 모든 PCI 장치에 `sriov-pf` 속성이 있고(`hw/pci/pci.c:104`), PF가 VF들로부터 SR-IOV cap을 **합성**한다(`pcie_sriov_pf_init_from_user_created_vfs`, `hw/pci/pcie_sriov.c:255`). 현재는 virtio-pci만 PF로 지원한다(`hw/virtio/virtio-pci.c:2164`).
```
-device vfio-user-pci,id=pf0,socket=/tmp/ssd-pf0.sock,addr=0x0.0
-device vfio-user-pci,socket=/tmp/ssd-vf1.sock,sriov-pf=pf0,addr=0x0.1
-device vfio-user-pci,socket=/tmp/ssd-vf2.sock,sriov-pf=pf0,addr=0x0.2
```
필요한 QEMU 변경(Q7):
1. vfio-user-pci 클래스에 `sriov_vf_user_creatable = true`.
2. PF realize에서 `pcie_sriov_pf_init_from_user_created_vfs` 호출. 서버의 SR-IOV cap은 지금처럼 버리고 QEMU가 합성한 cap을 에뮬레이션 영역(`emulated_config_bits`)으로 둔다.
3. **VF Enable / NumVFs 쓰기를 서버 PF에도 전달**한다. FW가 VF 활성화를 알아야 secondary 컨트롤러를 노출할 수 있다. 합성 cap과 서버 cap의 오프셋 매핑이 필요하다.
4. ARI: 합성 경로는 ARI가 없으면 nextfn=0으로 추가한다. multi-PF면 Q4의 `pcie_ari_init_nextfn`처럼 nextfn을 지정할 수 있어야 한다.
5. sim: VF마다 libvfio-user 소켓(엔드포인트)을 연다. VF BAR 크기는 모든 VF가 같아야 한다(QEMU 검사: `inconsistent SR-IOV BARs`). PF FLR 시 QEMU가 VF를 비활성화하므로 sim도 VF 리셋을 모델링해야 한다.
- vfio config space 쓰기 경로와 `pcie_sriov_config_write`가 어떻게 얽히는지는 프로토타입으로 확인이 필요하다(미검증).

### 3.4 단계별 계획
| 단계 | 내용 | 완료 기준 |
|---|---|---|
| 1 | 단일 PF 브리지: config, BAR(doorbell sparse mmap), MSI-X, DMA(memfd share) | 게스트 `nvme0` probe, Identify, R/W I/O |
| 2 | 리셋·오류 경로: FLR, CC.EN, NSSR(FW), sim 오류 주입(NAND 오류, 지연) | 드라이버 복구 경로 동작 |
| 3 | Q7: vfio-user SR-IOV(사용자 생성 VF), multi-PF ARI | VF enable → virt-mgmt → VF probe ([[nvme-sriov-multipf-lifecycle]] V1~V9 재실행) |
| 4 | 공통 테스트 스위트를 vssd와 sim 양쪽에서 실행하고 결과 diff | 차이 목록 = FW 이슈 또는 드라이버 가정 |

## 4. vssd를 계속 쓰는 이유
- SR-IOV·multi-PF·리셋 범위 실험이 이미 갖춰져 있다(Q4, Q5, 검증 매트릭스).
- 스펙 기준 동작이라 "드라이버가 스펙대로 하는가"를 FW 변수와 분리해 검증할 수 있다.
- QEMU에 오류 주입과 지연 속성을 추가하는 비용이 낮다. 결정적 재현이 필요한 회귀 테스트에 적합하다.
