# NVMe SR-IOV 동적 자원 변경 중 I/O 유지 — 드라이버 설계

> 대상: Linux `v7.2` nvme-pci (+ PCI/MSI 코어), QEMU `v11.1.2` NVMe SR-IOV 에뮬레이션 (둘 다 upstream 무수정)
> 근거: 소스 코드 + QEMU q35 게스트 실측 (2026-10-17). 호스트 커널은 건드리지 않음.
> 선행 문서: [[nvme-sriov-qemu-vs-kernel]] (구현 수준 분석, 용어)
> 관련: [[11_nvme_driver_init_probe]] · [[14_interrupt_and_completion]] · [[nvme-tagset-blkmq-flow]]

---

## 0. 결론 요약

1. **스펙상 "무중단 자원 변경"은 불가능하다.** Secondary Assign은 secondary가 Offline일 때만 허용되고, Online/Offline 전환은 VF 컨트롤러 리셋을 일으킨다. 따라서 목표는 **"짧고 계획된 리셋 동안 I/O를 붙잡아 두었다가(또는 다른 경로로 돌렸다가) 재개해, 에러 0건과 짧은 정지 시간을 만드는 것"** 이다.
2. **커널 리셋 경로는 이미 절반을 해 준다.** 큐 개수 변경(`blk_mq_update_nr_hw_queues`), IRQ 재할당, 취소된 요청의 재큐잉까지 처리한다. 실측에서 VQ만 바꾸면 에러 0건으로 큐 1개가 3개로 늘었다.
3. **막히는 곳은 세 군데다.**
   - **(버그) VI(MSI-X 벡터) 증가 시 VF가 dead가 된다.** PCI/MSI 코어의 장치별 MSI-X irq 도메인이 처음 크기로 고정되어 `-ERANGE`가 난다.
   - **(조율 없음) 변경을 I/O 타임아웃으로만 알아챈다.** 그래서 정지 시간이 `io_timeout + admin_timeout` 수준이 된다(기본값이면 수십 초).
   - **(PF 리셋) PF 리셋 한 번에 모든 VF가 Offline이 된다.** 커널은 이를 복구하지 않는다.
4. 할 일은 4계층이다: **P0 버그 수정 → P1 PF 측 자원 관리자 → P2 계획된 리셋(drain → offline/assign/online → 재초기화) → P3 비계획 상황 자가 복구**.

---

## 1. 기준선 실측 — 지금 드라이버로 I/O 중 자원 변경

공통 환경: `-machine q35 -smp 4`, `nvme-subsys` + `nvme,sriov_max_vfs=1,sriov_vq_flexible=8,sriov_vi_flexible=4` + `nvme-ns`,
게스트 커널 인자 `nvme_core.multipath=N nvme_core.io_timeout=5 nvme_core.admin_timeout=5`(정지 시간을 줄여 보려고 타임아웃을 5초로 낮춤).
VF(nvme1)에서 `dd if=/dev/nvme1n1 bs=4k count=16 iflag=direct`를 계속 반복하는 중에, PF에서 `virt-mgmt` 명령으로 offline → assign → online을 실행했다(드라이버 조율 없음).

| 시나리오 | 변경 | 결과 | I/O 정지 | 에러 | 최종 상태 |
|---|---|---|---|---|---|
| A | VQ 2→5, **VI 1→4** | 리셋 중 `nvme_setup_io_queues` = -EIO | 8.5s → 영구 | `I/O error` 1건 이후 디스크 용량 0 (dd가 `0+0 records`로 "성공") | **dead**, `rd_ios` 5603에서 정지 |
| B | VQ 2→5, VI 4→4 (VI 불변) | 리셋 성공, `1/0/0` → `3/0/0` queues | 약 11.6s (8.5 → 20.1) | **0건** | live, I/O 계속 진행 |

시나리오 A의 kretprobe 실측:
```
pci_msix_vec_count            ret=0x4          ← 새 테이블 크기(4)는 정확히 읽음
__pci_enable_msix_range       ret=0x1          ← nvme_pci_enable의 1벡터 할당은 성공
__pci_enable_msix_range       ret=0xffffffde   ← -34 = -ERANGE (nvme_setup_irqs, 최대 5벡터 요청)
nvme_setup_io_queues          ret=0xfffffffb   ← -5 = -EIO → "Disabling device after reset failure: -5"
```
QEMU 트레이스: 리셋 후 `CC.EN=1`과 Set Features(NumQ=4)는 성공했지만 **Create CQ는 한 번도 오지 않았다**. 즉 커널이 IRQ 단계에서 포기했다.

시나리오 B의 정지 구간 동안 일어난 일:
```
t=8.5   PF: offline → assign → online   (QEMU가 VF 컨트롤러를 리셋 → 큐 소멸)
t=13.9  I/O 타임아웃 → abort 발행
t=20.0  abort도 타임아웃 → 컨트롤러 리셋
t=20.04 "3/0/0 default/read/poll queues"  (VI 4 = admin 1 + I/O 3)
        취소된 요청 → HOST_ABORTED(0x371) → nvme_decide_disposition = RETRY → 재전송
```

---

## 2. 제약 조건 (왜 리셋이 불가피한가)

### 2.1 스펙과 QEMU 동작
| 제약 | 근거 |
|---|---|
| Secondary Assign은 Offline 상태에서만 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:7296` `if (sctrl->scs) return INVALID_SEC_CTRL_STATE` |
| Online/Offline 전환 = VF 함수 리셋 | `ctrl.c:7320` `nvme_virt_set_state` → `nvme_ctrl_reset(sn, NVME_RESET_FUNCTION)` |
| VF 리셋 시 큐와 진행 중 명령이 소멸 | `ctrl.c:8028` `nvme_ctrl_reset`: 진행 중 명령 cancel, SQ/CQ free |
| VF 리셋 시 MSI-X Table Size를 새 VI로 갱신 | `ctrl.c:8093` `nvme_update_msixcap_ts(pci_dev, conf_msix_qsize)` |
| Set Features NumQ 응답 = 새 VQ-1 | `ctrl.c:6825` |
| PF의 **어떤** 리셋이든 모든 secondary가 Offline + 자원 회수 | `ctrl.c:8073` |
| PF 자신의 flexible 할당(ACT 0x1)은 PF 함수 리셋 후에 반영 | `ctrl.c:7238` (`next_pri_ctrl_cap`), `:8005` |
| VF BAR0 크기는 VQFRSM/VIFRSM 최대치로 고정 | `ctrl.c:9152` → VQ를 늘려도 doorbell 공간은 충분 |

→ **VQ/VI 변경 = VF 리셋.** PF 쪽 flexible 재분배까지 하려면 PF 리셋이 필요하고, 그 순간 모든 VF가 리셋된다.

### 2.2 두 가지 배치 시나리오
```
(H) 같은 호스트                         (V) VFIO로 VM에 패스스루
┌─────────── host kernel ───────────┐   ┌─ host kernel ─┐  ┌─ guest kernel ─┐
│ nvme(PF) ── 직접 호출 ──► nvme(VF) │   │ nvme(PF)      │  │ nvme(VF)       │
│  자원 관리자        reset_prepare/ │   │ 자원 관리자   │  │ 호스트와 통신  │
│                     reset_done     │   │      ✗ 통신 채널 없음 ✗          │
└────────────────────────────────────┘   └───────────────┘  └────────────────┘
  → PF가 VF를 '계획적으로' 멈췄다가 재개   → VF가 스스로 감지·대기·복구해야 함
```
NVMe에는 NIC의 PF↔VF 메일박스 같은 통지 채널이 없다. 그래서 (V)에서는 VF 드라이버의 자가 복구(P3)가 유일한 수단이다(장기적으로는 vfio-pci variant driver가 중재할 수 있다).

### 2.3 커널이 이미 해 주는 것
| 기능 | 위치 |
|---|---|
| 리셋 시 IRQ 해제 후 재할당 (새 테이블 크기를 다시 읽음) | `qemu-debug/linux-7.2/drivers/nvme/host/pci.c:3325`(disable), `:2893` `nvme_setup_irqs`, `:2944` `nvme_setup_io_queues` |
| I/O 큐 수 = min(CPU 기반 요청, 컨트롤러 응답, 벡터-1) | `pci.c:2978`, `:3040` `max_qid = vecs-1 (+poll)` |
| hctx 수 변경 | `pci.c:3168` `nvme_pci_update_nr_queues` → `blk_mq_update_nr_hw_queues` |
| 리셋 중 진행 중 요청 → HOST_ABORTED → RETRY | `core.c:538` `nvme_cancel_request`, `core.c:403` `nvme_decide_disposition` |
| multipath면 path error(0x371) → 다른 경로로 FAILOVER | `core.c:416`, `nvme.h:796`, `multipath.c:147` |
| RESETTING/CONNECTING 경로가 있으면 multipath I/O 대기 | `multipath.c:475` `nvme_available_path` |
| FLR용 정지·재개 훅 | `pci.c:3853` `nvme_reset_prepare`, `:3866` `nvme_reset_done` |

빠진 것은 **변경 시점에 맞춘 조율(drain)**, **VI 증가 지원**, **PF 리셋 복구**, **Offline 상태 대기**다.

---

## 3. 해야 할 일 — 4계층 설계

### P0. 전제 버그 수정 (PCI/MSI 코어)

**P0-1. 장치별 MSI-X 도메인의 hwsize 고정 문제**
- 현상: `pci_setup_msix_device_domain()`(`drivers/pci/msi/irqdomain.c:272`)은 도메인이 이미 있으면 `true`를 바로 반환한다. 이 도메인은 **첫 enable 때의 hwsize로 만들어져 장치가 제거될 때까지 유지**된다(주석 "preserved until the device is removed"). 그래서 테이블이 커진 뒤 index ≥ 옛 hwsize를 할당하면 `kernel/irq/msi.c:125`에서 `-ERANGE`가 난다.
- 수정안: MSI-X가 꺼져 있는 상태에서 `pci_setup_msix_device_domain`이 기존 도메인의 hwsize(`msi_domain_get_hwsize`)와 새 hwsize가 다르면 `msi_remove_device_irq_domain()` 후 다시 만든다. 범용 PCI 코어 수정이라 linux-pci 쪽 리뷰 대상이다.
- 축소(4→1)는 index가 범위 안이라 코드상 문제가 없어 보인다(미검증).
- 참고: upstream의 `sriov_vf_msix_count` API(`drivers/pci/iov.c:248`)는 VF에 드라이버가 붙어 있으면 `-EBUSY`를 반환한다. 즉 upstream 설계는 "바인딩된 VF의 벡터 수 변경"을 의도적으로 막는다. live 변경은 NVMe 전용 경로로 가야 하고, 이 도메인 수정은 그 경로의 전제 조건이다.
- 대안(코어 수정 없이): VI를 VF 생성 시 최대치로 고정하고 VQ만 동적으로 바꾼다(시나리오 B처럼 안전). 단, 벡터 수가 I/O 큐 상한을 정하므로(`I/O 큐 ≤ VI-1`) 실제 큐 증가 폭이 제한된다.

**P0-2. Offline VF를 즉시 dead로 판정하는 문제**
- Offline VF는 CSTS 외 모든 레지스터를 0으로 읽는다(QEMU `ctrl.c:8502`). 그래서 `CAP.TO=0`이 되어 `nvme_wait_ready`(`core.c:2691`)가 즉시 타임아웃되고, `nvme_pci_ctrl_is_dead`(`pci.c:3283`)는 CFS를 보고 dead로 판정한다.
- 수정안: VF(`pdev->is_virtfn`)이면서 `CSTS.CFS && CAP==0` 패턴이면 "secondary offline"으로 분류한다. 리셋 경로에서 dead 대신 **백오프 폴링으로 Online 복귀를 기다린다**(module param 예: `sriov_offline_tmo`, 기본 수십 초). 기다리는 동안 I/O 큐는 quiesce 상태로 유지해 요청을 실패시키지 않는다.

### P1. PF 측 SR-IOV 자원 관리자 (nvme-pci)

```c
/* nvme_dev에 추가할 상태 (개념) */
struct nvme_sriov {
	struct mutex        lock;        /* 자원 변경·PF 리셋·sriov_configure 직렬화 */
	bool                vms;         /* OACS bit7 */
	struct nvme_pri_ctrl_cap cap;    /* CNS 0x14 캐시: VQFRT/VQFRSM/VIFRT/VIFRSM... */
	struct nvme_vf_res {
		u16 scid, vfn;
		u16 nvq, nvi;            /* 현재 할당 (CNS 0x15) */
		u16 want_nvq, want_nvi;  /* 사용자가 원한 값 → PF 리셋 후 복원에 사용 */
		bool online;
	} *vf;                           /* TotalVFs 개 */
};
```
| 항목 | 내용 |
|---|---|
| P1-1 탐색 | probe/reset 끝에서 OACS.VMS 확인, CNS 0x14/0x15 조회. `include/linux/nvme.h:561` enum에 `NVME_ID_CNS_PRIMARY_CTRL_CAPS = 0x14` 추가, 0x14/0x15 구조체 정의 |
| P1-2 `sriov_configure` (K1) | `pci_sriov_configure_simple`(`pci.c:4305`)를 대체한다. N개 VF에 자원을 분배 → Assign → Online → `pci_enable_sriov`. 그러면 autoprobe 그대로 VF가 정상 probe된다. N=0이면 disable 후 Offline하고 자원을 회수한다 |
| P1-3 분배 정책 | **VI = VQ**로 맞춘다(I/O 큐 = VI-1이므로 VQ를 더 줘도 벡터가 없으면 남는다 — 시나리오 B에서 VQ 5, VI 4 → I/O 큐 3). 상한은 VQFRSM/VIFRSM, 최소는 VQ 2·VI 1(QEMU `ctrl.c:7338`) |
| P1-4 런타임 변경 인터페이스 | VF PCI 디바이스에 NVMe 전용 sysfs(예: `nvme_vf_resources`에 `"vq vi"` 쓰기). `sriov_vf_msix_count`는 bound VF를 거부하므로 재사용할 수 없다. 남은 풀(`VQFRT − VQRFA − VQRFAP`)로 승인 여부를 검사한다 |
| P1-5 관측 | 같은 파일 읽기 또는 debugfs로 Primary Caps, Secondary List, want/cur 값 노출 |

### P2. 계획된 리셋 — 같은 호스트(H)에서 에러 0건 + 짧은 정지

```
resize(VF, vq, vi)                               [PF: sriov->lock 보유]
 1. 승인 검사 (풀, FRSM, 최소값)
 2. VF가 nvme에 바인딩돼 있으면:
    a. nvme_start_freeze(vf)                     새 요청은 blk_queue_enter에서 대기 (실패 아님)
    b. nvme_wait_freeze_timeout(vf, T_drain)     진행 중 요청을 완료까지 기다림 → 취소·재시도 최소화
    c. nvme_reset_prepare(vf)  (pci.c:3853)      = nvme_dev_disable(vf,false): 큐 삭제 + CC.EN=0 + IRQ 해제
                                                 (drain에 실패한 잔여 요청만 HOST_ABORTED → RETRY)
 3. PF admin: virt-mgmt ACT 0x7 Offline → 0x8 Assign(VQ) → 0x8 Assign(VI) → 0x9 Online
 4. VF가 바인딩돼 있으면:
    a. nvme_reset_done(vf)     (pci.c:3866)      → reset_work: 새 CAP/NumQ/MSI-X로 재초기화
                                                 → nvme_pci_update_nr_queues → blk_mq_update_nr_hw_queues
    b. nvme_unfreeze(vf)                         2a의 freeze와 짝 (freeze 깊이 카운트 주의)
 5. 실패하면: 이전 값으로 Assign을 다시 시도 → Online → reset_done (롤백)
```
- 정지 시간 ≈ drain 시간 + admin 명령 4개 + VF 재초기화. 타임아웃(수 초~수십 초)에 의존하지 않는다.
- multipath + shared namespace면 2c~4a 동안 요청이 PF나 다른 VF 경로로 failover되어 정지가 거의 없다(`core.c:416` path error → FAILOVER).
- 잠금 순서: `sriov->lock` → VF `device_lock`(PCI 코어 FLR 경로와 같은 패턴). VF 드라이버의 `remove`와 경합하지 않도록 VF device_lock을 잡은 상태에서 `pci_get_drvdata`를 확인한다.
- **PF 리셋 연동 (K4)**: PF `nvme_reset_work`(`pci.c:3385`) 앞뒤에 끼워 넣는다.
  ```
  PF reset 시작 전: 로컬에 바인딩된 VF 전부 freeze + reset_prepare
  PF 재초기화 후:  want_nvq/want_nvi로 Assign → Online → VF reset_done + unfreeze
  ```
  QEMU에서는 PF 리셋이 모든 secondary를 Offline시키고(`ctrl.c:8073`) 자원도 0으로 만들기 때문에, 복원 정보는 PF 드라이버가 들고 있어야 한다.

### P3. 비계획 상황 자가 복구 — VFIO 패스스루(V) 또는 외부 도구가 바꾼 경우

| 항목 | 내용 |
|---|---|
| P3-1 Offline 대기 | P0-2를 적용한다. VF는 reset 경로에서 Online 복귀까지 기다리며 I/O를 붙잡아 둔다 |
| P3-2 빠른 감지 | VF 전용 저비용 워치독(예: 200ms마다 CSTS 읽기). CFS가 보이면 I/O 타임아웃을 기다리지 않고 바로 reset을 예약한다. 시나리오 B의 약 12초(기본 타임아웃이면 30~90초) 정지를 감지 주기 수준으로 줄인다 |
| P3-3 재시도 예산 | 비계획 리셋은 요청마다 `nvme_max_retries`(기본 5, `core.c:65`)를 1씩 소모한다. 연속 변경이 이어져도 예산이 고갈되지 않도록, offline 대기 중 취소된 요청은 `retries`를 올리지 않는 방안을 검토한다 |
| P3-4 진행 중 명령 유실 | VF 리셋 시 QEMU는 진행 중 명령의 완료를 보내지 않는다 → 감지 후 cancel → RETRY가 유일한 경로. 데이터 무결성은 블록 계층 재전송으로 보장된다(쓰기 순서 보장이 필요한 상위 계층은 FUA/flush 의미가 유지되는지 확인) |

### P4. 성능·정책 (선택)
- 큐 종류 분배: VI가 VQ보다 적을 때 남는 VQ를 **poll 큐**로 쓰면 인터럽트 없이 큐 수를 늘릴 수 있다. 다만 tagset의 map 수는 처음 할당할 때 고정된다(`pci.c` `nvme_setup_io_queues`: "maps are allocated only once"). 그래서 VF는 처음부터 `nr_maps=3`으로 만들어야 나중에 poll 큐를 켤 수 있다.
- `blk_mq_update_nr_hw_queues`는 모든 큐를 freeze하므로, 변경 빈도에 레이트 리밋을 둔다.

---

## 4. 구현 순서와 검증 계획

| 단계 | 작업 | 위치 | 성공 기준 (QEMU 게스트) |
|---|---|---|---|
| 1 | P0-1 MSI-X 도메인 재생성 | `drivers/pci/msi/irqdomain.c:272` | 시나리오 A에서 리셋 성공, 큐 1→3(또는 4), 에러 0 |
| 2 | P1-1/P1-2 탐색 + `sriov_configure` | `pci.c:4305`, `include/linux/nvme.h` | autoprobe=1로 `echo 2 > sriov_numvfs` → VF 2개 probe |
| 3 | P1-4 + P2 계획된 리셋 | `pci.c` 신규 | I/O 루프 중 VQ/VI 변경 → **에러 0건, 정지 < 1초**(TCG 기준), `rd_ios`가 계속 증가 |
| 4 | P2 PF 리셋 연동 | `nvme_reset_work` | `nvme reset /dev/nvme0` 후 VF가 live + 같은 자원 |
| 5 | P0-2 + P3-2 | `core.c`/`pci.c` | 외부 `nvme virt-mgmt`로 바꿔도 VF가 dead가 되지 않고 감지 주기 안에 재개 |
| 6 | multipath 변형 | — | `multipath=Y` + shared ns에서 VF 변경 중 PF 경로로 무정지 |

측정 도구: `dd iflag=direct` 루프 + `/sys/block/<dev>/stat`의 `rd_ios`(0바이트 "성공"에 속지 않도록 반드시 통계로 확인), kretprobe(`pci_msix_vec_count`, `__pci_enable_msix_range`, `nvme_setup_io_queues`), QEMU `-trace 'pci_nvme_err_*' -trace pci_nvme_virt_mngmt -trace 'pci_nvme_create_*' -trace pci_nvme_setfeat_numq`.

---

## 5. 재현 스크립트 (게스트 init 핵심부)

```sh
PF=/sys/bus/pci/devices/0000:00:03.0; VF=0000:00:03.1
echo 0 > $PF/sriov_drivers_autoprobe; echo 1 > $PF/sriov_numvfs
nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 8 -n 2     # VQ 2
nvme virt-mgmt /dev/nvme0 -c 1 -r 1 -a 8 -n 1     # VI 1   (시나리오 B는 4)
nvme virt-mgmt /dev/nvme0 -c 1 -a 9               # Online
echo nvme > /sys/bus/pci/devices/$VF/driver_override; echo $VF > /sys/bus/pci/drivers_probe
# I/O 루프 (백그라운드)
( i=0; while :; do dd if=/dev/nvme1n1 of=/dev/null bs=4096 count=16 skip=$((i%4000)) iflag=direct || echo ERR; i=$((i+1)); done ) &
sleep 4
# 조율 없는 변경
nvme virt-mgmt /dev/nvme0 -c 1 -a 7
nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 8 -n 5
nvme virt-mgmt /dev/nvme0 -c 1 -r 1 -a 8 -n 4
nvme virt-mgmt /dev/nvme0 -c 1 -a 9
```
QEMU: `-machine q35 ... -device nvme-subsys,id=subsys0 -device nvme,id=nvme0,serial=deadbeef,subsys=subsys0,sriov_max_vfs=1,sriov_vq_flexible=8,sriov_vi_flexible=4 -device nvme-ns,drive=nvm0,bus=nvme0,nsid=1`,
게스트 인자 `nvme_core.multipath=N nvme_core.io_timeout=5 nvme_core.admin_timeout=5`.

---

## 6. 코드 위치 색인

| 대상 | 위치 |
|---|---|
| MSI-X 도메인 재사용 (P0-1 원인) | `qemu-debug/linux-7.2/drivers/pci/msi/irqdomain.c:272` |
| -ERANGE 발생 지점 | `qemu-debug/linux-7.2/kernel/irq/msi.c:125` |
| MSI-X enable 범위 계산 | `qemu-debug/linux-7.2/drivers/pci/msi/msi.c:801` |
| bound VF 벡터 변경 거부 | `qemu-debug/linux-7.2/drivers/pci/iov.c:248` |
| nvme IRQ/큐 설정 | `qemu-debug/linux-7.2/drivers/nvme/host/pci.c:2893, 2944, 3168` |
| nvme dead 판정 / disable / reset_work | `qemu-debug/linux-7.2/drivers/nvme/host/pci.c:3283, 3297, 3385` |
| FLR 훅 | `qemu-debug/linux-7.2/drivers/nvme/host/pci.c:3853, 3866, 4089` |
| sriov_configure | `qemu-debug/linux-7.2/drivers/nvme/host/pci.c:4305` |
| 재시도 판정 / cancel | `qemu-debug/linux-7.2/drivers/nvme/host/core.c:403, 538, 65` |
| wait_ready | `qemu-debug/linux-7.2/drivers/nvme/host/core.c:2691` |
| multipath failover / 가용 경로 | `qemu-debug/linux-7.2/drivers/nvme/host/multipath.c:147, 475` |
| QEMU Assign/Online/리셋/MSI-X 크기 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:7286, 7320, 8028, 8073, 8093` |
| QEMU Offline VF MMIO | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:8502, 8674` |
| QEMU NumQ 응답 | `qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:6825` |
