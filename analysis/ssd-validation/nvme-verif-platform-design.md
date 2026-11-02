# NVMe 커스텀 드라이버 검증 플랫폼(nvmeval) — 요구사항 및 설계

> 상태: 설계 초안 v0.2 (2026-11-02). 구현 전 단계. 실측 결과는 아직 없다.
> 환경: 호스트에 KVM이 없다(`/dev/kvm` 없음, vmx 플래그 없음). 그래서 QEMU는 **TCG로 실행**한다. QEMU `v11.1.2`(qemu-dev), Linux `v7.2` 기준.
> 관련: [[vssd-vs-vfio-user-sim]] · [[nvme-sriov-dynamic-resource-design]] · [[nvme-sriov-multipf-lifecycle]] · [[nvme-sriov-qemu-vs-kernel]] · [[nvme-queue-mgmt-design]] · [[nvme-more-queues-than-cpus-and-nto1-sqcq]] · [[20_msix_interrupt_e2e_qemu_trace]]

---

## 0. 전제와 범위

| 항목 | 결정 |
|---|---|
| 검증 대상 | 커스텀 nvme-pci 드라이버. 1차는 host 드라이버(`drivers/nvme/host`) 수정분이고, 이후 PCI/MSI, blk-mq 등 커널의 다른 부분으로 넓어질 수 있다 |
| 1차 대상 기능 | **queue mgmt (qmgmt)**: IO SQ/CQ 동적 생성·삭제, compaction, non-managed IRQ 재배치. 목적은 SR-IOV와 multi-function SSD 지원 |
| 개발 위치 | qmgmt 구현은 **회사 OA PC에만 있다**. 이 서버에서는 **플랫폼만** 개발한다. 따라서 드라이버 고유 심볼과 필드 이름은 adapter 설정으로 분리한다(§7) |
| 언어 | Python 3. 예외는 커널에 넣어야 하는 trigger stub 모듈(C)과 QEMU 패치(C)뿐이다 |
| 실행 | TCG. 타이밍과 성능은 검증하지 않는다. 판정은 **상태 일관성과 순서 관계**로만 한다 |
| 재현성 | 스냅샷과 icount는 범위 밖이다. 시드 고정과 산출물 보존만 한다 |
| VF | PF와 **같은 게스트**에 바인딩한다 (현재 기준) |
| 트리거 방식 | sysfs가 아니라 **trigger를 직접 주입**한다. 테스트가 드라이버 함수를 원하는 시점에 직접 호출한다(§5) |
| 호스트 | 호스트 커널은 수정하지 않는다. sudo 없이 동작해야 한다 |

---

## 1. 요구사항

### 1.1 기능 요구사항

| ID | 요구사항 |
|---|---|
| FR-1 | VM 생명주기: 표준 cmdline 생성(q35, smp 8, QMP unix 소켓, gdbstub, `-device vmcoreinfo`, 9p hostroot, virtio-serial, pvpanic), 기동, 종료, 산출물 수집 |
| FR-2 | **Trigger 주입**: 드라이버 내부 함수(예: compaction 진입점)를 게스트 커널의 sleep 가능한 process context에서 직접 호출한다. 동기와 비동기를 모두 지원한다 |
| FR-3 | 명령 주입: ioctl NVMe passthrough(Admin/IO), io_uring passthrough(`/dev/ng*`) |
| FR-4 | 관측: eBPF(kprobe, tracepoint)로 드라이버와 blk-mq 이벤트를 스트리밍한다. QEMU trace event로 장치 쪽 admin/IO 처리를 기록한다 |
| FR-5 | 상태 스냅샷: drgn으로 driver view와 IRQ view를, QMP로 device view를 읽는다(§3) |
| FR-6 | Checkpoint: 지정한 커널 심볼에서 게스트를 정지시키고, 세 view 스냅샷과 불변식 평가를 마친 뒤 재개한다 (gdbstub) |
| FR-7 | 장치 오류 주입: "트리거 조건 → 동작" 규칙을 QMP로 등록하고 해제한다. 예: N번째 Delete SQ에 오류 status 반환, Create CQ 성공 직후 CFS 설정 (§6) |
| FR-8 | 순서 제약 검사: qmgmt 절차의 순서 제약(§4)을 이벤트 스트림으로 검사한다 |
| FR-9 | 장애 감지: panic(`GUEST_PANICKED`), hang(watchdog), 타임아웃을 감지하면 시리얼 로그와 drgn 덤프를 남긴다 |
| FR-10 | 시나리오: pytest로 작성한다. 기능은 플러그인(`features/<name>/`)으로 추가한다 |

### 1.2 비기능 요구사항

| ID | 요구사항 |
|---|---|
| NFR-1 | **이식성**: 이 서버에서 개발한 플랫폼이 OA PC에서 설정 파일 교체만으로 돌아가야 한다. QEMU 패치는 patch series, stub 모듈은 out-of-tree 빌드로 관리한다 |
| NFR-2 | **드라이버 비의존 코어**: core/backends/views에는 qmgmt 고유 심볼이 없어야 한다 |
| NFR-3 | **장치 백엔드 추상화**: 지금은 vssd(QMP)를 쓴다. 나중에 SSD sim + vfio-user([[vssd-vs-vfio-user-sim]])로 같은 시나리오를 돌릴 수 있어야 한다 |
| NFR-4 | 산출물: 테스트마다 `artifacts/<test>/`에 serial.log, qmp-events.jsonl, ebpf.jsonl, qemu-trace.log, snapshots/*.json, verdict.json을 남긴다 |
| NFR-5 | 자체 검증: qmgmt가 없는 upstream 드라이버만으로도 코어, 오류 주입, 공통 불변식을 이 서버에서 검증할 수 있어야 한다 |

---

## 2. 대상 기능 모델 — qmgmt

```
 blk-mq hctx[0..nr_cpu)          ← CPU 수에 묶임
        │  hctx → qid 매핑 (가변)
        ▼
 ┌─ qmgmt 계층 ───────────────────────────────────────────────┐
 │  sq_bitmap / cq_bitmap   (nvmeq->flags와 별개)             │
 │  생성/삭제, compaction(qid 재배치), IRQ 재배치              │
 └────────────────────────────────────────────────────────────┘
        │
        ▼
 NVMe IO SQ/CQ (qid 1..NSQA/NCQA, 스펙상 최대 65535)
        │  CQ.iv → MSI-X vector
        ▼
 non-managed IRQ (affinity를 런타임에 irq_set_affinity로 변경)
        │
        ▼
 PF / VF (Virtualization Mgmt 0x1C로 VQ/VI 재분배)
```

전제가 되는 절차(사용자 설계, 2026-11-02 확인):
1. block quiesce → **drain(inflight 0)**. inflight가 남아 있으면 진행하지 않는다.
2. 대상 SQ 삭제 → 대상 CQ 삭제 → sq/cq bitmap 비트 해제.
3. **compaction은 대상 SQ/CQ pair가 모두 삭제된 뒤에만** 진행한다.
4. CQ 생성(iv 지정) → SQ 생성 → bitmap 비트 설정.
5. IRQ도 같은 drain 구간 안에서 해제·재할당·affinity 변경을 한다.
6. hctx 매핑 갱신 → unquiesce.

---

## 3. 세 관점 교차 검증 (3-view oracle)

같은 사실을 독립된 세 곳에서 읽고 대조한다. 정규화된 dataclass로 만들어 비교한다.

| View | 내용 | 수단 | 출처 |
|---|---|---|---|
| **Driver** | qid별 nvmeq 상태, `sq_bitmap`/`cq_bitmap`, hctx→qid 매핑, `cq_vector`, quiesce 상태, inflight 수 | drgn `--qemu` (QMP unix 소켓 + vmcoreinfo) | 게스트 메모리 |
| **Device** | `sq[]`/`cq[]` 존재 여부, size, cqid, iv, head/tail, IEN. MSI-X 테이블(addr/data/mask). VF별 VQ/VI 할당 | **신규 QMP `query-nvme-queues`** | QEMU `NvmeCtrl` |
| **IRQ** | `irq_desc` affinity와 effective_affinity, CPU별 카운트, managed 여부, MSI 메시지 destination APIC ID | drgn, agent(procfs) | 게스트 커널 |

공통 불변식은 drain 구간 밖에서는 항상 성립해야 한다.

| ID | 불변식 |
|---|---|
| INV-Q1 | driver의 `sq_bitmap` = device의 SQ 집합, driver의 `cq_bitmap` = device의 CQ 집합 |
| INV-Q2 | 모든 SQ의 cqid는 존재하는 CQ를 가리킨다. 모든 CQ의 iv는 활성(unmasked) MSI-X 엔트리를 가리킨다 |
| INV-Q3 | 모든 hctx가 live qid 하나 이상에 매핑되고, 삭제된 qid로 가는 매핑은 없다 |
| INV-Q4 | bitmap과 nvmeq->flags(ENABLED 등)가 서로 모순되지 않는다 |
| INV-I1 | CQ.iv의 MSI destination이 해당 IRQ의 effective_affinity에 속한다 ([[20_msix_interrupt_e2e_qemu_trace]] 방식) |
| INV-I2 | 재배치 이후 IRQ 카운트는 새 cpumask에 속한 CPU에서만 증가한다 |
| INV-I3 | 해제된 vector에 매달린 IRQ handler가 없다 (IRQ 누수 없음) |
| INV-IO | 제출 수 = 완료 수, fio verify 실패 0건, 오류 주입 시나리오가 아닌데 생긴 I/O error 0건 |
| INV-V1 | PF와 VF의 VQ/VI 합계 ≤ flexible 자원 상한. 0x1C 결과와 device view가 일치한다 |

---

## 4. 순서 제약 (sequence constraints)

qmgmt 절차를 시간 순서 속성으로 표현하고, 이벤트 스트림에서 검사한다. 이벤트는 다음 세 곳에서 받는다.
- 게스트 eBPF: qmgmt 함수 kprobe, `block_rq_*`, quiesce/unquiesce
- QEMU trace: `pci_nvme_create_sq`/`cq`, `pci_nvme_del_sq`/`cq`, `pci_nvme_irq_*`
- checkpoint 스냅샷

| ID | 제약 |
|---|---|
| S1 | `Delete SQ(q)`가 장치에 도착한 시점에는 q에 매핑된 hctx가 모두 quiesce 상태이고 `inflight(q) = 0`이다 |
| S2 | `Delete CQ(q)`는 `Delete SQ(q)`가 성공한 뒤에만 온다. CQ를 공유하는 SQ가 남아 있으면 안 된다 |
| S3 | compaction 진입 시 대상 q는 `sq_bit = 0 ∧ cq_bit = 0`이고, device에도 SQ/CQ가 없다 |
| S4 | vector v의 `free_irq`나 affinity 변경은 v를 쓰는 live CQ가 없을 때만, 또는 해당 CQ가 drain된 상태에서만 일어난다 |
| S5 | `Create CQ(q, iv)` 전에 iv가 할당되어 있다. `Create SQ(q)`는 `Create CQ(cqid)`가 성공한 뒤에만 온다 |
| S6 | bitmap 비트 변화는 quiesce부터 unquiesce까지의 구간 안에서만 일어난다 |
| S7 | unquiesce 직후 §3의 공통 불변식이 모두 성립한다 |
| E1 | 절차 중 admin 명령이 실패하면(오류 주입 포함), 복구가 끝난 뒤 INV-Q1~Q4와 INV-I3이 성립한다. 즉 부분 상태와 누수가 없다 |
| E2 | 절차 중 CFS나 컨트롤러 리셋이 발생하면, 리셋 완료 후 bitmap이 재구성된 장치 상태와 일치한다 |

검사기는 이벤트에 `(ctrl, qid, cid, vector)` 키를 붙여 조인한다. 시각 정렬에는 의존하지 않는다. 게스트 이벤트와 장치 이벤트는 같은 admin 명령의 cid로 묶는다.

---

## 5. Trigger 주입 — `nvmeval_stub.ko`

### 5.1 방식 비교

| 방식 | 장점 | 문제 | 판정 |
|---|---|---|---|
| sysfs/debugfs 노드를 드라이버에 추가 | 단순하다 | 기능마다 드라이버를 수정해야 하고, "동적인 feature 형태"가 아니다 | 기각 (사용자 결정) |
| gdbstub으로 함수 호출 주입(PC/레지스터 조작) | 모듈이 필요 없다 | 모든 vCPU가 멈춘 상태에서 호출하므로 sleep이 불가능하다. quiesce/drain은 sleep해야 한다 | 기각 |
| eBPF | 안전하다 | 임의의 커널 함수를 호출할 수 없다(kfunc만 가능) | 기각 |
| **범용 trigger stub 모듈** | sleep 가능한 context, 드라이버 무수정, 심볼 이름만으로 동작한다 | 테스트 전용 모듈을 빌드해야 한다 | **채택** |

> 참고: [[nvme-queue-mgmt-design]]에 따르면 custom 드라이버에는 이미 `NVME_IOCTL_QMGMT_CREATE/DELETE/INFO` ioctl이 있다. 그래서 trigger 채널은 두 가지를 함께 쓴다.
> **(a) 드라이버 UAPI ioctl**: 사용자가 실제로 쓰는 경로를 검증할 때. **(b) stub 직접 호출**: ioctl로 노출되지 않은 내부 단계(compaction, IRQ 재배치)를 따로 떼어 호출할 때.
> plugin의 `Trigger`는 `kind="ioctl" | "call"`로 둘 중 하나를 고른다. N:1 SQ-CQ 매핑이 있으므로 S2는 "CQ를 공유하는 모든 SQ가 삭제된 뒤"로 해석한다.

### 5.2 구조

```
host: Feature plugin ──(Trigger 정의: 심볼명 + 인자 스펙)──▶ agent RPC
guest agent ──ioctl(NVMEVAL_IOC_CALL)──▶ /dev/nvmeval  (nvmeval_stub.ko)
   nvmeval_stub: kallsyms 해석(kprobe 기법) → 함수 포인터 → 호출
               └ sync: ioctl을 부른 스레드(process context, sleep 가능)에서 직접 호출
               └ async: 전용 kthread에서 실행, 완료는 eventfd/poll로 알림
                        → "I/O 부하 + trigger 동시 실행" 경쟁 시나리오용
```

- **인자 모델**: `u64` 값, 또는 객체 selector(`ctrl:nvme0`, `nvmeq:nvme0:3`, `hctx:nvme0n1:2`). selector는 stub 안에서 해석한다. 예를 들어 `nvme0`은 `class_find_device`로 `nvme_ctrl`을 찾고, 거기서 `to_nvme_dev`로 내려간다. 호스트 drgn이 읽은 주소를 raw 포인터로 넘기는 방식도 허용하되, 커널 주소 범위인지 검사한다.
- **반환**: 함수의 반환값(int/long)과 실행 시간, 그리고 stub이 기록한 전후 시퀀스 번호를 돌려준다. 시퀀스 번호는 eBPF 이벤트와 조인할 때 쓴다.
- **드라이버 쪽 요구사항**: trigger 진입 함수는 `noinline`이어야 하고 kallsyms에 보여야 한다. static이어도 kallsyms에 있으면 된다(`CONFIG_KALLSYMS_ALL`). OA PC 구현에서 확인할 것.
- **안전장치**: 시그니처 화이트리스트(plugin이 선언한 심볼만 호출 가능), 재진입 직렬화 옵션, 테스트 전용 빌드(`MODULE_LICENSE("GPL")`, 배포 금지).

Plugin 선언 예:
```python
TRIGGERS = {
    "compact":   Trigger("nvme_qmgmt_compact",   args=[Obj("dev")]),            # 이름은 OA PC adapter에서 확정
    "del_pair":  Trigger("nvme_qmgmt_del_pair",  args=[Obj("dev"), U16("qid")]),
    "irq_remap": Trigger("nvme_qmgmt_irq_remap", args=[Obj("dev"), U16("qid"), CpuMask("mask")]),
}
```

---

## 6. 장치 오류 주입 — QEMU 규칙 엔진

QEMU `hw/nvme`에 규칙 테이블을 두고, admin/IO 명령을 처리하는 지점(`nvme_admin_cmd`, `nvme_io_cmd`, completion 게시, IRQ assert)에 hook을 건다.

**QMP 명령 (신규, QAPI):**
- `nvme-fault-add {id, trigger, action, count}`
- `nvme-fault-del {id}`
- `nvme-fault-list`
- `query-nvme-queues {id}`

| 트리거 필드 | 의미 |
|---|---|
| `queue` | `admin` / `io` / `sqid:N` |
| `opcode` | 예: 0x00 Delete SQ, 0x01 Create SQ, 0x04 Delete CQ, 0x05 Create CQ, 0x1C Virt Mgmt |
| `match` | CDW10/11 필드 매칭 (qid, qsize, iv, cqid) |
| `nth` / `count` | 몇 번째부터 몇 번 적용할지 |
| `phase` | `before`(처리 전 가로채기) / `after_ok`(정상 처리 후) / `after_err` |
| `after_rule` | 다른 규칙이 발동한 뒤에만 활성화 (연쇄) |

| 동작 | 효과 | 검증 대상 |
|---|---|---|
| `status{sct,sc,dnr}` | 처리 대신, 또는 처리 후 지정 status를 반환 | 롤백 경로 (E1) |
| `set_cfs` | CSTS.CFS = 1 | 리셋 복구 (E2) |
| `drop` | completion을 게시하지 않음 | admin 타임아웃 경로 |
| `delay{ms}` | completion을 지연 | 절차 중간에 다른 이벤트 끼워 넣기 |
| `aer{type,info}` | Async Event 발생 | AEN 처리 |
| `irq_suppress` / `irq_vector{v}` | 인터럽트 누락 / 다른 vector로 발생 | IRQ 재배치 검증 |
| `nssr` | NVM Subsystem Reset (기존 Q5 패치 재사용) | 전체 복구 |

예: "두 번째 Delete SQ는 Invalid Queue Deletion으로 실패하고, 그 직후 Create CQ가 성공하면 CFS를 설정한다."
```python
qmp.fault_add("r1", trigger=dict(queue="admin", opcode=0x00, nth=2, phase="before"),
              action=dict(type="status", sct=1, sc=0x0C, dnr=True), count=1)
qmp.fault_add("r2", trigger=dict(queue="admin", opcode=0x05, phase="after_ok", after_rule="r1"),
              action=dict(type="set_cfs"), count=1)
```

보조 수단:
- 게스트 `drivers/nvme/host/fault_inject.c`(debugfs): 드라이버가 받은 completion status를 변조한다.
- checkpoint(gdb breakpoint): 정확한 코드 지점에서 VM 전체를 멈추고, 그 상태에서 QMP 규칙을 활성화한 뒤 재개한다. "compaction 3단계 진입 직후 CFS" 같은 정밀 경쟁을 만들 때 쓴다.

---

## 7. 소프트웨어 구조

```
nvmeval/
├── core/
│   ├── machine.py      QEMU cmdline 빌더(프로파일: single / sriov / multi-pf), 기동·종료·watchdog
│   ├── artifacts.py    테스트별 산출물 디렉터리
│   ├── events.py       이벤트 버스, (ctrl,qid,cid,vector) 조인
│   ├── invariant.py    불변식 엔진(스냅샷 기반) + 순서 제약 검사기(이벤트 기반)
│   └── checkpoint.py   gdb bp → pause → snapshot → 평가 → cont
├── backends/
│   ├── qmp.py          qemu.qmp(asyncio). 이벤트 구독, fault_*, query_nvme_queues
│   ├── gdbrsp.py       순수 Python GDB RSP 클라이언트 (bp, mem r/w, cont)
│   ├── drgnview.py     drgn --qemu 연결, 타입/심볼 기반 읽기
│   ├── agent.py        게스트 agent JSON-RPC (virtio-serial)
│   └── device/         DeviceBackend 인터페이스: vssd(QMP) 구현 / vfio-user sim(향후)
├── views/              DriverView · DeviceView · IrqView (dataclass + 정규화)
├── guest/
│   ├── agent.py        게스트 쪽 (9p hostroot chroot의 python3): ioctl passthru, trigger, eBPF, fio
│   └── stub/           nvmeval_stub.ko 소스 (out-of-tree, 대상 커널에 맞춰 빌드)
├── adapters/
│   ├── upstream.yaml   upstream nvme-pci의 필드 경로 (이 서버의 자체 검증용)
│   └── qmgmt.yaml      OA PC qmgmt의 심볼/필드 경로 (bitmap 이름, trigger 심볼). OA PC에서 작성
├── features/
│   ├── common/         INV-Q1~Q3, INV-I*, INV-IO (모든 드라이버에 공통)
│   ├── qmgmt/          triggers, 순서 제약 S1~S7/E1~E2, 시나리오
│   ├── irq_remap/
│   └── sriov/          VF enable, 0x1C 재분배, VF 단위로 qmgmt 재사용
└── tests/              pytest (fixture: vm, qmp, gdb, drgn, agent, faults)
```

**adapter 예** (드라이버 고유 정보는 전부 여기에만 둔다):
```yaml
# adapters/qmgmt.yaml (OA PC에서 채움)
driver_view:
  dev_from_ctrl: "container_of(ctrl, struct nvme_dev, ctrl)"
  sq_bitmap: "dev->qmgmt.sq_bitmap"      # 실제 이름으로 교체
  cq_bitmap: "dev->qmgmt.cq_bitmap"
  queues:    "dev->queues[0:dev->nr_allocated_queues]"
  cq_vector: "nvmeq->cq_vector"
triggers:
  compact:   {symbol: nvme_qmgmt_compact, args: [obj:dev]}
checkpoints:
  compact_enter: nvme_qmgmt_compact
  after_pair_delete: nvme_qmgmt_del_pair+ret
```

시나리오 예:
```python
@pytest.mark.feature("qmgmt")
def test_compact_rollback_on_delete_sq_fail(vm, qmp, agent, views, seq):
    agent.fio_start(profile="randrw-verify", jobs=8)
    qmp.fault_add("r1", trigger=dict(queue="admin", opcode=0x00, nth=1),
                  action=dict(type="status", sct=1, sc=0x0C, dnr=True))
    r = agent.trigger("compact", dev="nvme0")
    assert r.ret != 0                         # 실패를 상위로 보고해야 한다
    seq.assert_ok(["S1", "S6", "E1"])         # 순서 제약 + 롤백
    views.assert_invariants(["INV-Q1", "INV-Q2", "INV-Q3", "INV-I3"])
    assert agent.fio_stop().verify_errors == 0
```

---

## 8. 수정이 필요한 곳

| 위치 | 작업 | 비고 |
|---|---|---|
| `qemu-debug/qemu-11.1.2` (qemu-dev) | Q6: `query-nvme-queues`, Q7': `nvme-fault-*` 규칙 엔진, trace event 보강 | patch series로 OA PC에 이식 |
| 게스트 커널 config | `BPF_SYSCALL`, `BPF_EVENTS`, `DEBUG_INFO_BTF`, `KPROBES`, `KALLSYMS_ALL`, `FAULT_INJECTION_DEBUG_FS`, `FW_CFG_SYSFS`, `PVPANIC`, `VIRTIO_CONSOLE`, `9P_FS` | OA PC 커널에도 같은 설정 필요 |
| `nvmeval/guest/stub` | `nvmeval_stub.ko` | 드라이버는 수정하지 않음 |
| 호스트 | `nvmeval` 패키지 | `qemu.qmp`, `drgn`, `pytest` |

---

## 9. 단계별 계획

| 단계 | 내용 | 완료 기준 (이 서버) |
|---|---|---|
| M1 | machine + QMP + agent 골격, 산출물 | upstream 드라이버 게스트가 smp 8로 기동하고, agent ping과 passthrough Identify가 성공 |
| M2 | QEMU `query-nvme-queues` + DeviceView + DriverView(upstream adapter) | upstream 드라이버에서 INV-Q1/Q2가 통과 |
| M3 | IrqView + INV-I1~I3 | MSI destination ↔ affinity 대조 통과 |
| M4 | QEMU fault 규칙 엔진 | passthrough로 보낸 Delete SQ가 지정 status로 실패. CFS 설정 후 드라이버 리셋을 관측 |
| M5 | nvmeval_stub.ko + trigger | upstream의 noinline 함수 하나(예: `nvme_reset_ctrl`)를 trigger로 호출하는 데 성공 |
| M6 | eBPF 이벤트 + 순서 제약 검사기 + checkpoint | upstream 리셋 절차에서 Delete/Create 순서 제약(S2/S5) 검사 통과 |
| M7 | features/qmgmt + sriov 프로파일 | **OA PC**에서 qmgmt adapter로 S1~S7/E1~E2 실행 |

---

## 10. 미결 사항

1. OA PC 환경: KVM 사용 가능 여부, 커널 버전(7.2 기준인지), QEMU 버전. QEMU 패치를 이식할 수 있는지가 여기에 달려 있다.
2. qmgmt trigger 진입 함수가 `noinline`이고 kallsyms에 보이는지.
3. bitmap 단위: qid당 1비트인지, VF별로 bitmap이 따로 있는지. adapter와 INV-V1 정의에 필요하다.
4. drain 판정 기준: `blk_mq_quiesce_tagset` + tag iterator인지, qmgmt 자체 카운터인지. S1 검사에 쓸 이벤트 소스가 정해진다.
5. 게스트 rootfs: 지금의 initramfs + 9p chroot 방식을 유지할지, 디스크 이미지 rootfs로 바꿀지. VF가 추가되면 NVMe 장치가 늘어나므로, rootfs를 NVMe에 두지 않는 것이 좋다.
