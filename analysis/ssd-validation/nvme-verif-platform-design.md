# NVMe 커스텀 드라이버 검증 플랫폼(nvmeval) — 요구사항 및 아키텍처 설계

> 상태: **설계 v0.5 (2026-11-02), 1차 외부 리뷰 반영**. 구현 전 단계라 아직 실측한 것은 없다. 단, 코드 근거는 표시해 두었다. 리뷰 반영 내역은 §15에 있다.
> 기준 소스: QEMU `v11.1.2`(qemu-dev 브랜치), Linux `v7.2`. 근거 위치는 `qemu-debug/<트리>/<경로>:<라인>` 형식으로 적는다.
> 관련: [[vssd-vs-vfio-user-sim]] · [[nvme-sriov-dynamic-resource-design]] · [[nvme-sriov-multipf-lifecycle]] · [[nvme-sriov-qemu-vs-kernel]] · [[nvme-queue-mgmt-design]] · [[nvme-more-queues-than-cpus-and-nto1-sqcq]] · [[20_msix_interrupt_e2e_qemu_trace]]

### 리뷰어 안내
- 각 결정에는 상태를 붙였다. **[확정]**은 사용자가 결정한 것, **[제안]**은 설계자가 제안하고 사용자가 큰 틀에 동의한 것, **[미결]**은 아직 정하지 않은 것이다.
- 특히 검토가 필요한 부분은 §13 "알려진 위험과 검토 포인트"에 모아 두었다.
- 검증 대상 드라이버(qmgmt)의 실제 코드는 이 문서 작성 환경에 **없다**. 문서에 나오는 드라이버 심볼, 필드, ioctl 이름은 모두 **가상의 예시**다.

---

## 0. 전제와 범위

| 항목 | 내용 | 상태 |
|---|---|---|
| 목적 | 개발 중인 커스텀 nvme-pci 드라이버 기능을 호스트에서 검증한다. 수단은 QEMU 게스트 조작과 QEMU 가상 NVMe 장치(vssd) 조작이다 | 확정 |
| 검증 대상 범위 | 1차는 host 드라이버(`drivers/nvme/host`) 수정분이다. 이후 커널의 다른 부분(PCI/MSI, blk-mq 등)으로 넓어질 수 있다 | 확정 |
| 1차 대상 기능 | **queue mgmt (qmgmt)**: IO SQ/CQ 동적 생성·삭제, compaction, non-managed IRQ 재배치. 목적은 SR-IOV와 multi-function SSD 지원이다 (§2) | 확정 |
| 이후 대상 | 개발 중인 다른 기능들. 플러그인으로 추가한다 | 확정 (목록은 미결) |
| 코드 위치 | qmgmt 구현은 **회사 OA PC에만 있다**. 이 서버에서는 **플랫폼만** 개발한다 | 확정 |
| **API 차이** | OA PC 드라이버의 API와 custom field는 이 서버의 코드와 **다르다**. [[nvme-queue-mgmt-design]]에 적힌 ioctl/구조체와도 다를 수 있다. 플랫폼 코어는 어떤 드라이버 심볼도 가정하지 않는다 (§6) | 확정 |
| 언어 | Python. 예외는 커널 trigger stub 모듈(C)과 QEMU 패치(C)뿐이다 | 확정 |
| 실행 가속 | 개발 서버에는 KVM이 없어 **TCG**로 실행한다(`/dev/kvm` 없음, vmx 플래그 없음). 타이밍과 성능은 검증하지 않는다. 판정은 **상태 일관성과 순서 관계**로만 한다 | 확정 |
| 재현성 | 스냅샷과 icount/record-replay는 범위 밖이다. 시드 고정과 산출물 보존만 한다 | 확정 |
| VF 배치 | PF와 **같은 게스트**에 바인딩한다 (현재 기준) | 확정 |
| 트리거 방식 | sysfs가 아니라 **trigger를 직접 주입**한다. 테스트가 드라이버 함수나 UAPI를 원하는 시점에 직접 호출한다 (§7) | 확정 |
| 호스트 안전 | 호스트 커널은 수정하지 않는다. sudo 없이 동작해야 한다 | 확정 |
| **실사용 환경 (OA PC)** | 게스트: CentOS Stream 9 userspace + 직접 빌드한 Linux **v7.2**. QEMU **11.1**. 모든 바이너리에 debug symbol이 있고 kallsyms를 확인할 수 있다 | 확정 (KVM 여부는 미결) |
| 개발 환경 (이 서버) | Ubuntu 24.04, Python 3.12, QEMU 11.1.2(qemu-dev, 자체 패치 2개: multi-PF SR-IOV, NSSR), Linux v7.2 | 확정 |

---

## 1. 요구사항

### 1.1 기능 요구사항

| ID | 요구사항 |
|---|---|
| FR-1 | **VM 생명주기**: profile로 QEMU cmdline을 만들고 기동·종료·산출물 수집을 한다. 기본 구성은 q35, smp 8, QMP unix 소켓, gdbstub, `-device vmcoreinfo`, virtio-serial, pvpanic, 9p 산출물 공유 |
| FR-2 | **Trigger 주입**: 두 가지 경로를 지원한다. (a) 드라이버 UAPI ioctl. (b) 드라이버 내부 함수를 게스트 커널의 sleep 가능한 process context에서 직접 호출. 동기와 비동기를 모두 지원한다 |
| FR-3 | **명령 주입**: NVMe passthrough ioctl(Admin/IO), io_uring passthrough(`/dev/ng*`) |
| FR-4 | **관측**: 게스트 eBPF(kprobe, tracepoint)로 이벤트를 스트리밍한다. QEMU trace event로 장치 쪽 명령 처리를 기록한다 |
| FR-5 | **상태 스냅샷**: driver view와 IRQ view는 drgn, device view는 QMP로 읽고, 모두 **정규 스키마**(§4)로 변환한다 |
| FR-6 | **Checkpoint**: 지정한 커널 심볼에서 VM을 정지시키고, 세 view 스냅샷과 판정을 마친 뒤 재개한다 (gdbstub) |
| FR-7 | **장치 오류 주입**: "트리거 조건 → 동작" 규칙을 QMP로 등록·해제한다 (§8) |
| FR-8 | **판정**: 세 축으로 판정한다. 스냅샷 기반 불변식(invariant), 이벤트 기반 순서 제약(sequence), 기한 기반 진행 보장(liveness) (§5) |
| FR-9 | **장애 감지**: panic(QMP `GUEST_PANICKED`), hang(watchdog), 타임아웃을 감지하면 시리얼 로그와 drgn 덤프를 남긴다 |
| FR-10 | **시나리오**: pytest로 작성한다. 기능은 플러그인(`features/<name>/`)으로 추가한다 |
| FR-11 | **adapter 검증과 생성**: 세션 시작 시 adapter의 필드 경로, 타입, 심볼, 구조체 크기를 DWARF와 대조해 맞지 않으면 즉시 실패한다. DWARF에서 adapter 초안을 만드는 도구를 제공한다 (§6) |

### 1.2 비기능 요구사항

| ID | 요구사항 |
|---|---|
| NFR-1 | **이식성**: 이 서버에서 개발한 플랫폼이 OA PC에서 profile과 adapter 교체만으로 돌아가야 한다 |
| NFR-2 | **드라이버 비의존 코어**: `nvmeval/` 패키지에는 드라이버 고유 심볼, 필드, ioctl 번호가 하나도 없어야 한다 |
| NFR-3 | **장치 백엔드 추상화**: 지금은 vssd(QMP)를 쓴다. 나중에 SSD sim + vfio-user([[vssd-vs-vfio-user-sim]])로 같은 시나리오를 돌릴 수 있어야 한다 |
| NFR-4 | **Python 버전**: 게스트 agent는 Python **3.9** 호환이고 stdlib만 쓴다(CentOS Stream 9 기본). 호스트 패키지도 3.9 이상을 목표로 한다 |
| NFR-5 | **산출물**: 테스트마다 `artifacts/<run>/<test>/`에 serial.log, qmp-events.jsonl, events.jsonl(정규 이벤트), qemu-trace.log, snapshots/*.json, verdict.json을 남긴다 |
| NFR-6 | **자체 검증**: qmgmt 없이 upstream 드라이버만으로도 코어, 오류 주입, 공통 불변식을 이 서버에서 검증할 수 있어야 한다. 판정 엔진은 VM 없이 합성 데이터로 단위 테스트할 수 있어야 한다 |
| NFR-7 | **판정 신뢰성**: 관측 누락(eBPF 이벤트 손실 등)이 있으면 결과를 PASS가 아니라 **INCONCLUSIVE**로 보고한다 (§5.4) |
| NFR-8 | **회사 코드 분리**: 회사 드라이버 정보를 담는 `features/`와 `adapters/`는 외부 경로에서 불러올 수 있어야 한다 (§10, 미결 Q2) |

---

## 2. 대상 기능 모델 — qmgmt

```
 blk-mq hctx[0..nr_cpu)          ← CPU 수에 묶임
        │  hctx → qid 매핑 (가변)
        ▼
 ┌─ qmgmt 계층 ───────────────────────────────────────────────┐
 │  sq/cq 관리 bitmap  (nvmeq->flags와 별개)                   │
 │  생성/삭제, compaction(qid 재배치), IRQ 재배치              │
 └────────────────────────────────────────────────────────────┘
        │
        ▼
 NVMe IO SQ/CQ (qid 1..NSQA/NCQA, 스펙상 최대 65535). N:1 SQ→CQ 매핑 가능
        │  CQ.iv → MSI-X vector
        ▼
 non-managed IRQ (affinity를 런타임에 irq_set_affinity로 변경)
        │
        ▼
 PF / VF (Virtualization Mgmt 0x1C로 VQ/VI 재분배)
```

전제가 되는 절차 [확정, 사용자 설계]:
1. block quiesce → **drain(inflight 0)**. inflight가 남아 있으면 진행하지 않는다.
2. 대상 SQ 삭제 → 대상 CQ 삭제 → sq/cq bitmap 비트 해제. CQ를 공유하는 SQ가 있으면 그 SQ들을 모두 삭제한 뒤에 CQ를 삭제한다.
3. **compaction은 대상 SQ/CQ pair가 모두 삭제된 뒤에만** 진행한다.
4. CQ 생성(iv 지정) → SQ 생성 → bitmap 비트 설정.
5. IRQ도 같은 drain 구간 안에서 해제·재할당·affinity 변경을 한다.
6. hctx 매핑 갱신 → unquiesce.

이 절차의 목적은 **자원(queue, vector, bitmap)을 갱신하는 동안 다른 경로가 그 자원을 건드리지 못하게 제약하는 것**이다. 플랫폼은 이 제약이 실제로 지켜지는지를 검증한다.

---

## 3. 아키텍처 개요

### 3.1 구성도

```
┌─────────────────────── Host: nvmeval (단일 프로세스, asyncio) ───────────────────────┐
│  Scenario (pytest)          "trigger → 관측 → 판정"                                  │
│     │                                                                                │
│  ┌──▼───────────────┐   ┌──────────────┐   ┌───────────────────────────────┐          │
│  │ Feature plugins  │   │ Fault plans  │   │ Oracle                        │          │
│  │ qmgmt/sriov/...  │   │ (테스트 범위)│   │  · Invariant (스냅샷 기반)    │          │
│  └──┬───────────────┘   └──────┬───────┘   │  · Sequence  (이벤트 기반)    │          │
│     │                          │           │  · Phase window               │          │
│     │                          │           └──────▲───────────────▲────────┘          │
│  ┌──▼──────────────────────────▼──────┐   ┌──────┴──────┐  ┌──────┴───────┐           │
│  │ Control plane                       │   │ State model │  │ Event bus    │           │
│  │ trigger · passthru · fault · stop   │   │ (정규 스키마)│  │ (정규 이벤트)│           │
│  └──┬──────────┬─────────┬────────────┘   └──▲──────────┘  └──▲───────────┘           │
│     │          │         │            views + adapter      event normalizers          │
│  ┌──▼───┐  ┌───▼──┐  ┌───▼───┐  ┌──────────┴─────┐  ┌──────────┴──────────┐           │
│  │ QMP  │  │ gdb  │  │ agent │  │ drgn            │  │ eBPF / QEMU trace   │           │
│  └──┬───┘  └──┬───┘  └───┬───┘  └──┬─────────────┘  └──┬──────────────────┘           │
└─────┼─────────┼──────────┼─────────┼────────────────────┼─────────────────────────────┘
      ▼         ▼          ▼         ▼                    ▼
   QEMU (vssd + fault engine)    Guest kernel (driver + nvmeval_stub.ko) / guest agent
```

계층은 셋으로 나뉘고, 판정 로직은 맨 위 계층에만 있다.
- **통신(backends)**: 외부 시스템과 주고받기만 한다. 판정하지 않는다.
- **변환(views, event normalizers)**: 원본 데이터를 정규 스키마와 정규 이벤트로 바꾼다.
- **판정(oracle)**: 정규 형식만 입력으로 받는다.

### 3.2 아키텍처 결정 목록

| ID | 결정 | 상태 | 근거 / 대안 |
|---|---|---|---|
| D1 | **정규 상태 스키마를 플랫폼의 중심 계약으로 둔다.** 흐름은 `드라이버 → adapter → 정규 스키마 → 판정`이다. 구조는 core 스키마 + feature 확장 네임스페이스(`ext`) | 제안 | 드라이버 API가 계속 바뀐다. 판정이 드라이버 필드를 직접 읽으면 드라이버가 바뀔 때마다 판정 코드도 고쳐야 한다. 대안 "feature마다 독자 스키마"는 공통 불변식(INV-Q*)을 재사용할 수 없어 기각 (§4) |
| D2 | **판정은 세 축으로 한다.** Invariant(상태), Sequence(순서), Liveness(진행). 앞의 두 축은 phase window로 잇는다. 정상적인 과도·축퇴 상태는 선언된 예외(allowed state)로 처리한다 | 제안 (v0.5에서 liveness와 예외 추가) | drain 구간 안에서는 상태가 일시적으로 불일치하는 것이 정상이다. 그래서 구간 안은 순서로, 구간 밖은 상태로 판정한다. 그런데 safety 규칙만 있으면 "아무 일도 일어나지 않는" hang이 PASS가 된다. 그래서 liveness 축을 추가한다 (§5) |
| D3 | **전역 시계에 의존하지 않는다.** 출처 내부 순서, cid 인과 조인, stub 마커, checkpoint 정지로 순서를 정한다 | 제안 | 게스트 ktime과 QEMU 시각을 맞출 수 없다. TCG에서는 시간 비율도 일정하지 않다 (§5.3) |
| D4 | **게스트 agent는 얇게 둔다.** 실행과 스트림 전달만 하고, 판정 로직은 모두 호스트에 둔다 | 제안 | 게스트는 Python 3.9이고 OA PC에 배포해야 한다. 바뀌는 부분(판정, adapter)을 호스트에 모은다 |
| D5 | **호스트는 단일 asyncio 루프로 실행한다.** drgn(동기 API)은 전용 스레드에서 돌린다 | 제안 | 경쟁 시나리오("fio 부하 + 비동기 trigger + N번째 Delete SQ fault")를 한 곳에서 조율해야 한다. 대안인 멀티프로세스는 조율 비용이 커서 기각 |
| D6 | **feature는 플러그인으로 조합한다.** 구성: `{triggers, 스키마 ext, adapter 키, invariants, sequence rules, scenarios}` | 제안 | sriov는 VF마다 qmgmt를 재사용한다. 그래서 스키마의 최상위 단위를 function(PF/VF)으로 둔다 |
| D7 | **fault의 수명은 테스트에 묶는다.** 테스트가 끝나면 자동 해제하고 발동 이력을 산출물에 남긴다. 장치(QEMU), 커널(fault_inject), gdb 주입을 같은 `FaultPlan` 인터페이스로 다룬다 | 제안 | fault 규칙이 남아 있으면 다음 테스트가 오염된다 |
| D8 | **환경 차이는 profile로 흡수한다.** `profile = {qemu 경로, kernel, rootfs, adapter, accel, features 경로}` | 제안 | 개발 서버와 OA PC에서 코드는 같고 설정만 다르게 한다 |
| D9 | **trigger 채널은 UAPI ioctl과 stub 직접 호출을 함께 쓴다** | 제안 | UAPI 경로 검증과 내부 단계 분리 검증이 모두 필요하다 (§7) |
| D10 | **장치 오류 주입은 QEMU 규칙 엔진 + 신규 QMP 명령으로 한다** | 확정 (방향) | 기본 QMP로는 NVMe 프로토콜 수준의 오류를 넣을 수 없다 (§8) |

---

## 4. 정규 상태 스키마 (D1)

### 4.1 계층 구조

```
Snapshot                                   ← 한 시점, 한 view의 전체 상태
 ├─ meta: view(driver|device|irq), source, snap_seq, vm_state(running|stopped),
 │        phase(현재 phase window 이름 또는 None), checkpoint(이름 또는 None)
 └─ functions{fn_id → Function}            ← PF/VF 단위. fn_id = PCI BDF 문자열
      ├─ ctrl:    ControllerState           (enabled, csts{rdy,cfs,shst}, cc, is_vf, parent_fn, nr_vfs)
      ├─ sqs:     {qid → SQ}                (cqid, size, state, prio, head, tail)
      ├─ cqs:     {qid → CQ}                (iv, size, ien, state, head, tail)
      ├─ vectors: {v   → Vector}            (irq, affinity, effective, dest_apic, masked, managed,
      │                                      move_pending, pending_mask, cqids[])   ← cqids: 역참조 (N:1)
      ├─ hctx:    {hctx_idx → Hctx}         (qids[], cpus[], quiesced, inflight, busy_tags)
      ├─ blkmq:   BlkmqState                (nr_hw_queues, map_mode(normal|fallback|single), q_usage)
      └─ ext:     {"qmgmt": QmgmtExt, "sriov": SriovExt, ...}   ← feature 확장 네임스페이스
```

- **최상위 단위는 function이다.** VF와 multi-PF를 처음부터 넣어 두면 sriov feature가 qmgmt 판정을 VF마다 그대로 재사용할 수 있다.
- **core에 넣는 기준**: NVMe 스펙이나 커널 공통 개념(SQ, CQ, vector, hctx)이고, **두 개 이상의 view에서 관측할 수 있는** 필드만 core에 둔다. 교차 검증의 대상이 core다.
- **ext에 넣는 기준**: 한 view에서만 보이는 드라이버 고유 상태다(예: qmgmt 관리 bitmap). ext는 feature가 dataclass로 선언하고 `schema.ext.register("qmgmt", QmgmtExt)`로 등록한다. 다른 feature의 ext는 읽기만 할 수 있다.
- **qid 0(admin queue)도 sqs/cqs에 포함한다.** 판정 규칙에서 IO queue만 볼 때는 필터로 거른다.
- **관계의 다중도를 스키마에서 허용한다** (v0.5):
  - SQ → CQ는 N:1이다 (`sqs[q].cqid`).
  - CQ → vector도 N:1이다. VI < VQ, 즉 VF에 vector가 queue pair보다 적게 배정된 경우다. 그래서 `vectors[v].cqids[]`로 역참조를 둔다. driver view와 device view 각각에서 `cqs[*].iv`로 계산한다.
  - vector 개수 정책(예: `min(VI, 쌍 수 + 1)`, +1은 admin vector)은 **드라이버 정책**이므로 core 불변식이 아니다. feature 불변식(qmgmt/sriov)으로 둔다.
- **IRQ 이동 대기 상태를 스키마에 넣는다** (v0.5): `move_pending`, `pending_mask`. 의미는 §5.6에서 설명한다.

### 4.2 view별 채우는 필드

| 필드 | Driver (drgn + adapter) | Device (QMP) | IRQ (drgn / procfs) |
|---|---|---|---|
| `ctrl.csts`, `ctrl.cc` | ✅ 드라이버가 마지막에 읽은 값 | ✅ 레지스터 값 | – |
| `sqs[qid].cqid/size` | ✅ | ✅ | – |
| `sqs[qid].head/tail` | ✅ 드라이버가 아는 값 | ✅ 장치가 아는 값 | – |
| `cqs[qid].iv` | ✅ | ✅ | – |
| `vectors[v].irq/affinity/effective/managed` | △ (드라이버가 기록한 irq 번호) | – | ✅ |
| `vectors[v].dest_apic/masked` | – | ✅ MSI-X 테이블 | ✅ MSI 메시지 |
| `vectors[v].move_pending/pending_mask` | – | – | ✅ `IRQD_SETAFFINITY_PENDING`, `desc->pending_mask` |
| `vectors[v].cqids` | ✅ (driver의 cq_vector로 계산) | ✅ (device의 CQ.iv로 계산) | – |
| `hctx[*]` (busy_tags 포함) | ✅ | – | – |
| `blkmq.*` | ✅ | – | – |
| `sqs[qid]` 미처리 명령 수 | – | ✅ SQ head≠tail, 장치 내부 outstanding 요청 | – |
| `ext.qmgmt.*` (bitmap 등) | ✅ | – | – |
| `ext.sriov.*` (VQ/VI 할당) | ✅ | ✅ | – |

### 4.3 값의 의미 규칙

| 값 | 의미 | 판정에서의 처리 |
|---|---|---|
| 필드 없음 / 키 없음 | 해당 객체가 **존재하지 않는다**(예: 삭제된 SQ) | 존재 여부 비교 대상 |
| `Unknown` (센티넬) | 이 view가 **관측할 수 없다** | 비교에서 제외한다. 규칙이 그 필드를 필수로 요구하면 INCONCLUSIVE |
| `None` | 관측했고, 값이 "없음"이다 (예: 할당되지 않은 irq) | 일반 값으로 비교 |

이 구분을 두지 않으면 "device view가 hctx를 모른다"와 "hctx가 없다"를 구별할 수 없어 판정이 틀어진다.

### 4.4 스냅샷 일관성

| 모드 | 방법 | 일관성 | 용도 |
|---|---|---|---|
| **stopped** | QMP `stop`이나 gdb breakpoint로 VM을 정지시킨 뒤 세 view를 읽는다 | 세 view가 같은 시점이다. 단 아래 주의 사항이 있다 | 불변식 판정의 기본 |
| running | VM을 실행한 채로 읽는다 | 필드끼리 시점이 어긋난 값(torn read)이 나올 수 있다 | 모니터링, 디버깅용. **판정에는 쓰지 않는다** |

**stopped 모드 주의 사항** (코드 근거):
- QEMU의 `do_vm_stop()`은 vCPU를 멈춘 뒤 `bdrv_drain_all()`을 호출한다(`qemu-debug/qemu-11.1.2/system/cpus.c:296~320`). gdbstub breakpoint로 정지할 때도 같은 경로를 탄다. 근거: `vm_stop(RUN_STATE_DEBUG)`(`qemu-debug/qemu-11.1.2/gdbstub/system.c:119`).
- 따라서 정지 시점에 **장치는 진행 중이던 블록 I/O를 완료해 CQ 엔트리를 쓰고 interrupt를 올렸지만, 게스트 드라이버는 아직 처리하지 않은 상태**가 있을 수 있다. 즉 device view가 driver view보다 앞서 있을 수 있다.
- 처리 방침: CQ head/tail, 미처리 CQE 수 같은 "진행 정도" 필드는 정확히 같을 것을 요구하지 않는다. 대신 **"장치가 같거나 앞서 있다"는 순서 관계로 비교**한다. queue 존재 여부, cqid, iv, size 같은 구성 필드는 정확히 같아야 한다.
- doorbell이 기록됐지만 장치가 아직 처리하지 않은 admin 명령이 있을 수 있다. 그래서 device view에 admin SQ head/tail을 포함시켜, 판정할 때 "처리 대기 중인 admin 명령 수"를 함께 본다.

---

## 5. 판정 (D2, D3)

### 5.1 Invariant 축: 스냅샷 기반

규칙 정의 형태:
```python
@invariant("INV-Q1", scope="function", phases=OUTSIDE_DRAIN,
           needs=["driver.ext.qmgmt.sq_bitmap", "device.sqs"])
def sq_bitmap_matches_device(drv: Function, dev: Function) -> Verdict:
    want = set(drv.ext["qmgmt"].sq_bitmap)               # 드라이버가 할당했다고 믿는 qid
    have = {q for q in dev.sqs if q != 0}                # 장치에 실제로 있는 IO SQ
    return expect_equal(want, have, detail="sq bitmap vs device SQ set")
```

- `scope`: function, controller, global 중 하나. function scope 규칙은 PF/VF마다 자동으로 반복 적용된다.
- `phases`: 이 규칙이 적용되는 phase window. drain 구간 안에서는 공통 불변식이 비활성이다.
- `needs`: 필요한 필드 목록. 하나라도 `Unknown`이면 INCONCLUSIVE를 반환한다.
- 결과는 PASS / FAIL / INCONCLUSIVE 셋 중 하나이고, 비교한 값과 출처 스냅샷 id를 담는다.

평가 시점: (1) 테스트 시작 직후의 기준선, (2) 각 phase window가 끝난 직후, (3) checkpoint, (4) 테스트 종료. 모두 stopped 스냅샷으로 평가한다.

공통 불변식 (drain 구간 밖에서 항상 성립):

| ID | 불변식 | 비교 view |
|---|---|---|
| INV-Q1 | driver의 SQ 할당 집합(qmgmt면 bitmap) = device의 SQ 집합. CQ도 마찬가지 | driver ↔ device |
| INV-Q2 | 모든 SQ의 cqid는 존재하는 CQ를 가리킨다. 모든 CQ의 iv는 활성(unmasked) MSI-X 엔트리를 가리킨다 | device (+ driver) |
| INV-Q3 | 모든 hctx가 live qid 하나 이상에 매핑되고, 삭제된 qid로 가는 매핑은 없다 | driver ↔ device |
| INV-Q4 | 드라이버 내부 표현끼리 모순이 없다 (qmgmt bitmap ↔ nvmeq->flags 등. 구체 규칙은 feature가 정의) | driver |
| INV-Q5 | 구성 필드(cqid, size, iv)가 driver와 device에서 같다 | driver ↔ device |
| INV-I1 | vector v의 MSI destination이 v의 effective_affinity에 속한다 ([[20_msix_interrupt_e2e_qemu_trace]] 방식). **`move_pending`인 vector는 확정 상태가 아니므로 판정을 미룬다** (§5.6 AS-3) | device ↔ irq |
| INV-I2 | 재배치 이후 IRQ 카운트는 새 cpumask에 속한 CPU에서만 증가한다 (두 스냅샷의 차분). 재배치 직후 첫 인터럽트 1건은 이전 CPU에서 올 수 있으므로 제외한다 (AS-3) | irq |
| INV-I3 | 해제된 vector에 매달린 IRQ handler가 없다 (IRQ 누수 없음) | driver ↔ irq |
| INV-I4 | (v0.5) vector를 공유하는 CQ들(`vectors[v].cqids`)이 서비스하는 hctx CPU의 합집합이 v의 affinity에 포함된다. 공유 vector에서는 affinity를 CQ 하나가 아니라 **합집합과 비교**해야 한다. 포함(⊆)인지 일치(=)인지는 feature 정책으로 정한다 | driver ↔ irq |
| INV-I5 | (v0.5) driver의 `cqs[*].iv`로 계산한 `cqids`와 device의 CQ.iv로 계산한 `cqids`가 같다 (N:1 매핑 일치) | driver ↔ device |
| INV-IO | 제출 수 = 완료 수, fio verify 실패 0건, 오류 주입 시나리오가 아닌데 생긴 I/O error 0건 | agent |
| INV-V1 | PF와 VF의 VQ/VI 합계 ≤ flexible 자원 상한. 0x1C 결과와 device view가 일치한다. VF의 IO queue pair 수 ≤ VQ − 1 (VQ에 admin 포함, §8.5 SR-1) | driver ↔ device |

### 5.2 Sequence 축: 이벤트 기반

정규 이벤트의 형식:
```
Event{ src(guest_bpf|qemu_trace|stub|agent|qmp), src_seq, kind, fn_id, qid?, cid?, vector?, args{}, status? }
```
- kind 예시: `AdminSubmit`, `AdminComplete`, `DevCreateSQ`, `DevDeleteSQ`, `DevCreateCQ`, `DevDeleteCQ`, `Quiesce`, `Unquiesce`, `IrqAlloc`, `IrqFree`, `IrqAffinity`, `BitmapSet`, `BitmapClear`, `TriggerEnter`, `TriggerExit`, `FaultFired`, `CtrlReset`
- 원본(bpftrace 출력, QEMU trace 줄)을 정규 이벤트로 바꾸는 normalizer는 출처별로 하나씩 둔다. 드라이버 고유 이벤트(`BitmapSet` 등)의 kprobe 위치는 adapter가 정한다.

qmgmt 순서 제약:

| ID | 제약 |
|---|---|
| S1 | `DevDeleteSQ(q)` 시점에 q에 매핑된 hctx가 모두 quiesce 상태이고 `inflight(q) = 0`이다. **inflight는 feature 자체 카운터가 아니라 blk-mq 상태와 장치 상태로 판정한다** (§5.7) |
| S2 | `DevDeleteCQ(c)`는 c를 cqid로 가진 모든 SQ의 `DevDeleteSQ`가 성공한 뒤에만 온다 (N:1 매핑 고려) |
| S3 | compaction 진입(`TriggerEnter` 또는 adapter가 정한 kprobe) 시 대상 q는 `sq_bit = 0 ∧ cq_bit = 0`이고, device에도 SQ/CQ가 없다 |
| S4 | vector v의 `IrqFree`나 `IrqAffinity`는 v를 iv로 쓰는 live CQ가 없거나, 그 CQ가 drain된 상태에서만 일어난다 |
| S5 | `DevCreateCQ(c, iv)` 전에 iv에 대한 `IrqAlloc`이 있다. `DevCreateSQ(q, cqid=c)`는 `DevCreateCQ(c)`가 성공한 뒤에만 온다 |
| S6 | `BitmapSet` / `BitmapClear`는 `Quiesce`~`Unquiesce` 구간 안에서만 일어난다 |
| S7 | `Unquiesce` 직후 §5.1의 공통 불변식이 모두 성립한다 (invariant 축으로 넘김) |
| E1 | 절차 중 admin 명령이 실패하면, 복구가 끝난 뒤 INV-Q1~Q5와 INV-I3이 성립한다 (부분 상태와 누수 없음) |
| E2 | 절차 중 CFS나 컨트롤러 리셋이 발생하면, 리셋 완료 후 driver 상태가 재구성된 장치 상태와 일치한다 |

### 5.3 순서 결정 방법 (D3)

| 범위 | 방법 |
|---|---|
| 같은 출처 안 | `src_seq` (출처가 단조 증가로 부여) |
| 게스트 ↔ 장치 | admin 명령의 `(fn_id, cid)`로 인과 관계를 잇는다. `AdminSubmit` → `DevXxx` → `AdminComplete` |
| trigger 경계 | stub이 `TriggerEnter`/`TriggerExit` 마커를 남긴다. 같은 시퀀스 카운터를 eBPF 쪽에서도 읽어 조인한다 |
| 정확한 순간 | checkpoint로 VM을 정지시키고 스냅샷을 찍는다 |

순서 규칙 엔진은 이벤트를 위 관계로 이은 **부분 순서(partial order)** 위에서 규칙을 평가한다. 두 이벤트의 순서를 결정할 수 없는데 규칙이 그 순서를 요구하면 INCONCLUSIVE다.

### 5.4 phase window와 판정 결과

- phase window는 이름이 붙은 구간이다. 예: `drain` = `Quiesce` ~ `Unquiesce`, `reset` = `CtrlReset` ~ `CSTS.RDY=1`.
- window 경계는 이벤트로 정한다. window 안에서는 그 window에 지정된 순서 규칙만 켜고, 공통 불변식은 끈다.
- 판정 집계: 규칙 하나라도 FAIL이면 FAIL. FAIL이 없고 INCONCLUSIVE가 있으면 INCONCLUSIVE. 모두 PASS일 때만 PASS.
- **관측 누락은 INCONCLUSIVE로 승격한다**: eBPF ringbuf 손실 카운터가 0이 아니거나, QEMU trace가 잘렸거나, agent 스트림이 끊기면 해당 구간의 순서 규칙 결과를 INCONCLUSIVE로 바꾼다 (NFR-7).
- 판정 결과에 **DEGRADED** 표시를 추가한다 (v0.5). 선언된 예외 상태(§5.6) 덕분에 통과한 경우 PASS(DEGRADED)로 표시한다. 예: blk-mq fallback 매핑. 오류 주입 시나리오에서는 정상 결과일 수 있지만, 오류 주입이 없는 시나리오라면 기능 결함 신호다. 시나리오가 `allow_degraded=False`로 엄격하게 만들 수 있다.

### 5.5 Liveness 축: 기한 기반 진행 보장 (v0.5 추가)

safety 규칙(invariant, sequence)만으로는 "drain이 끝나지 않는다", "리셋이 영원히 RESETTING에 머문다" 같은 hang을 잡지 못한다. 아무 일도 일어나지 않으면 위반 이벤트도 생기지 않아 PASS가 되기 때문이다. 그래서 "결국 일어나야 하는 일"을 기한과 함께 검사한다.

| ID | 진행 보장 | 종결 조건 |
|---|---|---|
| L1 | 제출된 모든 요청은 결국 완료되거나 실패한다 | 요청마다 `AdminComplete`/`block_rq_complete` 또는 오류 반환. 테스트 종료 시 driver view의 busy tag가 0 |
| L2 | fio(부하 생성기)는 결국 종료된다 | agent가 fio 종료 코드와 요약을 수신 |
| L3 | 리셋이나 fault 이후 컨트롤러는 결국 **종결 상태(LIVE 또는 DEAD)**에 도달한다 | `nvme_ctrl.state`가 LIVE 또는 DEAD. RESETTING/CONNECTING에 머물면 위반. 어느 쪽이 기대값인지는 시나리오가 정한다 |
| L4 | 모든 `Quiesce`는 결국 `Unquiesce`로 닫힌다 (drain window 종결) | 짝이 되는 `Unquiesce` 이벤트, 그리고 driver view의 quiesced=false |
| L5 | trigger 호출은 결국 반환된다 | `TriggerExit` 마커 |
| L6 | `move_pending`인 IRQ 이동은 I/O가 발생하면 결국 반영된다 | 다음 인터럽트 이후 `move_pending=false`, effective가 새 mask에 속함 (§5.6 AS-3) |

**기한을 정하는 방식** (TCG 환경 고려):
- 벽시계 시간만으로는 정할 수 없다. TCG에서는 게스트가 호스트 부하에 따라 느려지기 때문이다.
- 기한은 **게스트 진행량**으로 정한다. drgn으로 읽은 `jiffies` 증가량, 또는 게스트 sched clock을 쓴다. 예: "게스트 시간으로 30초 안에 L3 도달".
- 별도로 벽시계 상한(예: 10분)을 두어 플랫폼 자체가 멈추지 않게 한다.
- 위반 판정 시: 게스트 시간이 흘렀는데 종결되지 않았으면 **FAIL(hang)**이다. 게스트 시간이 흐르지 않았으면(VM 자체가 멈춤) **INCONCLUSIVE + 플랫폼 오류**다.
- hang FAIL이 나면 자동으로 다음을 덤프한다: drgn으로 hung task 스택, blk-mq 상태(hctx 상태, busy tag, dispatch list), 컨트롤러 상태.

### 5.6 선언된 예외 상태 (allowed states, v0.5 추가)

정상이지만 일반 불변식에 어긋나 보이는 과도·축퇴 상태가 있다. 이런 상태를 불변식마다 if 문으로 흩어 두지 않고, **이름을 붙인 예외 상태로 선언**한다. 불변식은 "어떤 예외 상태에서 면제되는지"를 명시한다.

| ID | 상태 | 판정 조건 | 면제 / 변형되는 규칙 | 근거 |
|---|---|---|---|---|
| AS-1 | **빈 풀**: IO queue pair가 0개 (admin queue만 존재) | driver와 device 모두 IO SQ/CQ가 0개 | INV-Q3(모든 hctx가 live qid에 매핑)를 면제한다. 대신 "IO 제출이 막히거나 오류를 반환한다"를 검사한다. 시나리오가 이 상태를 허용할 때만 적용 | qmgmt가 모든 pair를 삭제한 중간 또는 최종 상태 |
| AS-2 | **blk-mq fallback 매핑** | `blkmq.nr_hw_queues` ≠ 장치 IO queue 수, 또는 모든 CPU가 hctx 0에 매핑됨 | INV-Q3을 "모든 CPU가 live hctx에 매핑되고, 그 hctx가 live qid를 가진다"로 완화한다. 결과는 DEGRADED | `blk_mq_update_nr_hw_queues` 실패 시 이전 개수로 fallback (`qemu-debug/linux-7.2/block/blk-mq.c:5162~5176`). 재매핑 전 `blk_mq_clear_mq_map`이 모든 CPU를 0으로 초기화 (`block/blk-mq.c:4777~4780`). `blk_mq_map_hw_queues`의 실패 fallback (`block/blk-mq-cpumap.c:129~130`) |
| AS-3 | **IRQ 이동 대기**: affinity를 바꿨지만 아직 반영되지 않음 | `vectors[v].move_pending = true` | INV-I1/I2는 v에 대해 판정을 미룬다(INCONCLUSIVE가 아니라 "대기"). 판정 전에 해당 queue로 I/O를 보내 인터럽트를 유발한 뒤 다시 스냅샷을 찍는다. L6로 결국 반영되는지 검사한다 | x86에서 interrupt remapping이 없으면 PCI MSI chip에 `IRQCHIP_MOVE_DEFERRED`가 설정된다 (`qemu-debug/linux-7.2/arch/x86/kernel/apic/msi.c:216~217`). 그러면 `irq_set_affinity`는 즉시 바꾸지 않고 pending으로 기록한다 (`kernel/irq/manage.c:304~312`). 실제 이동은 다음 인터럽트 처리 때 일어난다. 개발 서버 커널은 `CONFIG_IRQ_REMAP` 없음, `GENERIC_PENDING_IRQ=y` |
| AS-4 | drain window 내부 | phase = drain | 공통 불변식 전체 면제. 순서 규칙만 적용 | §5.4 |
| AS-5 | 리셋 진행 중 | ctrl.state ∈ {RESETTING, CONNECTING} | 공통 불변식 면제. L3로 종결을 검사 | §5.5 |

AS-3 관련 profile 옵션: q35에 `-device intel-iommu,intremap=on`을 붙인 **IR 모드 profile**을 별도로 두어, 즉시 이동(IR)과 지연 이동(non-IR) 양쪽에서 같은 시나리오를 돌린다. OA PC 실장비 환경이 어느 쪽인지에 따라 어느 결과를 기준으로 삼을지 정한다.

### 5.7 drain 판정 기준 (v0.5, 미결 Q4 해결)

S1의 "inflight = 0"은 **검증 대상 기능의 자체 카운터로 판정하지 않는다.** 검증 대상이 자기 상태를 스스로 보고하는 셈이 되어, 카운터 버그를 잡을 수 없기 때문이다. 독립된 두 출처로 판정하고, feature 카운터는 대조용으로만 쓴다.

| 출처 | 판정 방법 | 주의 |
|---|---|---|
| **blk-mq tag 상태** (주 판정) | q에 매핑된 각 hctx에 대해, driver tag 비트맵(sbitmap)에서 할당된 tag의 request 상태가 `MQ_RQ_IN_FLIGHT`인 것이 0개. 커널이 CPU offline drain에 쓰는 `blk_mq_hctx_has_requests()`(`qemu-debug/linux-7.2/block/blk-mq.c:3677`)와 같은 방식의 순회를 drgn으로 한다 | tag는 할당됐지만 아직 발행되지 않은 request(dispatch list, requeue)는 SQ에 들어가지 않았으므로 inflight가 아니다. 상태(IN_FLIGHT)로 구분해야 한다 |
| **`q_usage_counter`** (보조) | `percpu_ref`가 0인지 확인 | **freeze를 쓸 때만 의미가 있다.** quiesce는 dispatch만 멈추므로, quiesce 중에도 sw queue나 스케줄러에 대기 중인 request가 참조를 잡고 있어 0이 아닐 수 있다. 이것은 정상이다. percpu 모드에서는 CPU별 합산이 필요하고, freeze 후 atomic 모드일 때 정확하다(`block/blk-mq.c:171~221`) |
| **장치 상태** (교차 확인) | device view에서 해당 SQ의 head = tail이고, 장치 내부에 outstanding 요청이 없으며, 해당 CQ에 미처리 CQE가 없다 | 드라이버가 inflight 0이라고 믿어도, 장치에 명령이 남아 있으면 S1 위반이다 |
| feature 카운터 (대조) | adapter가 지정한 qmgmt 자체 카운터 | 위 출처들과 다르면 그 자체로 FAIL (카운터 버그) |

---

## 6. Adapter — 드라이버별 매핑 (D1, FR-11)

### 6.1 원칙
- OA PC 드라이버의 API와 custom field는 이 서버의 코드와 다르고, 앞으로도 계속 바뀐다. 그래서 **드라이버 고유 정보는 전부 adapter(yaml 데이터)와 feature(Python)에만 둔다.**
- adapter가 담는 것:
  - 필드 경로 (drgn C 표현식)
  - kprobe 위치 (정규 이벤트 생성용)
  - trigger 심볼과 인자
  - **UAPI ioctl 번호와 구조체 레이아웃**: ctypes 정의를 코드에 박지 않고 adapter 선언으로 생성한다
  - checkpoint 위치

### 6.2 예시 (가상의 이름)
```yaml
# adapters/qmgmt-oa.yaml — OA PC에서 adapter-scan으로 초안 생성 후 보정
adapter: qmgmt-oa
kernel: "7.2.0-*"
driver_view:
  dev_from_ctrl: "container_of(ctrl, struct nvme_dev, ctrl)"
  queues:        "dev->queues[0:dev->nr_allocated_queues]"
  sq:  {cqid: "nvmeq->cqid", size: "nvmeq->q_depth", tail: "nvmeq->sq_tail"}
  cq:  {iv: "nvmeq->cq_vector", head: "nvmeq->cq_head"}
  ext.qmgmt:
    sq_bitmap: {expr: "dev->qmgmt.sq_bitmap", type: bitmap, nbits: "dev->qmgmt.max_qid"}
    cq_bitmap: {expr: "dev->qmgmt.cq_bitmap", type: bitmap, nbits: "dev->qmgmt.max_qid"}
events:                       # 드라이버 고유 정규 이벤트의 kprobe 위치
  BitmapSet:   {kprobe: "nvme_qmgmt_bitmap_set",   args: {qid: arg1, kind: arg2}}
  BitmapClear: {kprobe: "nvme_qmgmt_bitmap_clear", args: {qid: arg1, kind: arg2}}
triggers:
  compact:  {kind: call,  symbol: nvme_qmgmt_compact, args: [obj:dev]}
  create:   {kind: ioctl, dev: "/dev/nvme{n}", nr: "_IOWR('N', 0x80, struct nvme_qmgmt_create)",
             struct: nvme_qmgmt_create}      # 레이아웃은 DWARF에서 읽고 sizeof를 검증
checkpoints:
  compact_enter: nvme_qmgmt_compact
  after_pair_delete: "nvme_qmgmt_del_pair%return"
```

### 6.3 검증과 생성
- **세션 시작 시 검증** (`adapter/loader.py`): 하나라도 실패하면 테스트를 시작하지 않는다.
  - 모든 필드 경로를 drgn으로 평가해 존재 여부와 타입을 확인한다.
  - trigger, kprobe, checkpoint 심볼이 kallsyms에 있는지, inline으로 사라지지 않았는지 확인한다.
  - ioctl 구조체의 `sizeof`와 필드 오프셋을 DWARF와 대조한다.
- **초안 생성** (`nvmeval adapter-scan`): drgn으로 `struct nvme_dev`, `struct nvme_queue`, 이름 패턴(`*qmgmt*` 등)에 맞는 심볼과 구조체를 덤프해 adapter 초안을 만든다. 손으로 옮겨 적다가 생기는 오류를 줄이기 위한 것이다.

---

## 7. Trigger 주입 (D9)

### 7.1 방식 비교

| 방식 | 장점 | 문제 | 판정 |
|---|---|---|---|
| sysfs/debugfs 노드를 드라이버에 추가 | 단순하다 | 기능마다 드라이버를 수정해야 한다. 동적인 feature 형태가 아니다 | 기각 (사용자 결정) |
| gdbstub으로 함수 호출 주입(PC/레지스터 조작) | 모듈이 필요 없다 | 모든 vCPU가 멈춘 상태에서 호출하므로 sleep할 수 없다. quiesce/drain은 sleep해야 한다 | 기각 |
| eBPF | 안전하다 | 임의의 커널 함수를 호출할 수 없다 (kfunc만 가능) | 기각 |
| **드라이버 UAPI ioctl** | 실제 사용 경로를 그대로 검증한다 | 노출된 단위로만 호출할 수 있다 | **채택 (kind=ioctl)** |
| **범용 trigger stub 모듈** | sleep 가능한 context, 드라이버 무수정, 심볼만으로 내부 단계를 호출한다 | 테스트 전용 모듈을 빌드해야 한다 | **채택 (kind=call)** |

### 7.2 stub 구조

```
host: Feature plugin ──(Trigger: kind, 심볼/ioctl, 인자 스펙)──▶ agent RPC
guest agent ─┬─ kind=ioctl: 드라이버 장치 노드에 직접 ioctl (구조체는 adapter 선언으로 생성)
             └─ kind=call : ioctl(NVMEVAL_IOC_CALL) ──▶ /dev/nvmeval (nvmeval_stub.ko)
                  nvmeval_stub: 심볼 해석(kprobe 등록으로 주소 획득) → 함수 포인터 → 호출
                    ├ sync : ioctl을 부른 스레드(process context, sleep 가능)에서 호출
                    └ async: 전용 kthread에서 실행. 완료는 poll/eventfd로 알림
                             → "I/O 부하 + trigger 동시 실행" 경쟁 시나리오용
```

- **인자 모델**: `u64` 값, 또는 객체 selector(`ctrl:nvme0`, `nvmeq:nvme0:3`, `hctx:nvme0n1:2`). selector는 stub 안에서 해석한다. `nvme_ctrl`→`nvme_dev` 변환처럼 드라이버 구조에 의존하는 부분은 adapter가 정의한 오프셋을 stub에 넘겨서 처리한다. 호스트 drgn이 읽은 raw 포인터도 허용하되, 커널 주소 범위인지 검사한다.
- **반환**: 함수 반환값, 실행 시간, stub 시퀀스 번호(`TriggerEnter`/`TriggerExit`).
- **드라이버 쪽 요구사항**: `kind=call` 대상 함수는 kallsyms에 보여야 하고 inline되지 않아야 한다. kallsyms는 OA PC에서 확인할 수 있다고 사용자가 확인했다. inline 여부는 adapter 검증 단계에서 잡는다.
- **안전장치**: adapter가 선언한 심볼만 호출할 수 있다(화이트리스트). 재진입 직렬화 옵션을 둔다. 테스트 전용 빌드이며 배포하지 않는다.
- **호출 계약**: 호출되는 함수가 기대하는 락이나 상태 전제(예: 호출자가 ctrl mutex를 잡고 있어야 함)는 stub이 알 수 없다. 그래서 `kind=call` 대상은 **"외부에서 호출해도 되는 진입점"으로 설계된 함수에 한정**한다 (§13 R3).

---

## 8. 장치 오류 주입 — QEMU 규칙 엔진 (D10)

QEMU `hw/nvme`에 규칙 테이블을 두고, 명령 처리 지점(admin/IO 명령 디스패치, completion 게시, IRQ assert)에 hook을 건다.

**신규 QMP 명령 (QAPI):**
- `nvme-fault-add {id, ctrl, trigger, action, count}`
- `nvme-fault-del {id}`
- `nvme-fault-list` (발동 횟수 포함)
- `query-nvme-queues {ctrl}`: device view 원본

| 트리거 필드 | 의미 |
|---|---|
| `queue` | `admin` / `io` / `sqid:N` |
| `opcode` | 예: 0x00 Delete I/O SQ, 0x01 Create I/O SQ, 0x04 Delete I/O CQ, 0x05 Create I/O CQ, 0x1C Virtualization Mgmt |
| `match` | CDW10/11 필드 매칭 (qid, qsize, iv, cqid) |
| `nth` / `count` | 몇 번째부터 몇 번 적용할지 |
| `phase` | `before`(처리 전에 가로챔) / `after_ok`(정상 처리 후) / `after_err` |
| `after_rule` | 다른 규칙이 발동한 뒤에만 활성화 (연쇄) |

| 동작 | 효과 | 검증 대상 |
|---|---|---|
| `status{sct,sc,dnr}` | 처리 대신(before) 또는 처리 후(after) 지정 status를 반환 | 롤백 경로 (E1) |
| `set_cfs` | CSTS.CFS = 1 | 리셋 복구 (E2) |
| `drop` | completion을 게시하지 않음 | admin 타임아웃 경로 |
| `delay{ms}` | completion을 지연 | 절차 중간에 다른 이벤트 끼워 넣기 |
| `aer{type,info}` | Async Event 발생 | AEN 처리 |
| `irq_suppress` / `irq_vector{v}` | 인터럽트 누락 / 다른 vector로 발생 | IRQ 재배치 검증 |
| `nssr` | NVM Subsystem Reset (기존 NSSR 패치 재사용) | 전체 복구 |
| `pause` | (v0.5) 명령 처리 직전에 VM 정지를 요청한다(BH로 `vm_stop` 지연 요청 → QMP `STOP` 이벤트). 플랫폼이 stopped 스냅샷을 찍은 뒤 `cont`로 재개 | S1처럼 "장치가 명령을 받는 순간"의 상태 판정 (R12) |

`phase=after_*`의 `status` 동작은 **장치 상태는 바뀌었는데 드라이버는 실패로 아는** 상황을 만든다. 예를 들어 SQ는 실제로 삭제됐는데 드라이버에는 오류가 돌아간다. 이것은 롤백 로직이 장치 상태를 다시 확인하는지 검증하는 핵심 케이스다.

예: "두 번째 Delete SQ는 Invalid Queue Deletion으로 실패하고, 그 뒤 Create CQ가 성공하면 CFS를 설정한다."
```python
faults.add("r1", trigger=dict(queue="admin", opcode=0x00, nth=2, phase="before"),
           action=dict(type="status", sct=1, sc=0x0C, dnr=True), count=1)
faults.add("r2", trigger=dict(queue="admin", opcode=0x05, phase="after_ok", after_rule="r1"),
           action=dict(type="set_cfs"), count=1)
```

### 8.1 커널 쪽 오류 주입 — 같은 `FaultPlan`에 포함 (v0.5 확장)

장치 오류만으로는 **드라이버 내부 실패 경로**(메모리 할당 실패, IRQ 할당 실패, 커널 API 오류 반환)를 만들 수 없다. 그런데 qmgmt 롤백(E1)에서 가장 버그가 많을 곳이 바로 여기다. 그래서 커널 오류 주입을 장치 오류와 같은 `FaultPlan`의 1급 백엔드로 둔다.

| 백엔드 | 수단 | 대표 용도 | 필요 config |
|---|---|---|---|
| `kernel.nvme_status` | `drivers/nvme/host/fault_inject.c` (debugfs `fault_inject/`) | 드라이버가 받은 completion status 변조 | `FAULT_INJECTION_DEBUG_FS` + `FAULT_INJECTION` |
| `kernel.function` | fail_function (debugfs `fail_function/inject`) | `ALLOW_ERROR_INJECTION`으로 표시된 함수의 반환값 강제 | `FUNCTION_ERROR_INJECTION`, `FAIL_FUNCTION` |
| `kernel.bpf_override` | kprobe + `bpf_override_return` | 위와 같은 대상에 조건부 주입 (특정 qid, N번째 호출 등) | `BPF_KPROBE_OVERRIDE` |
| `kernel.alloc` | failslab / fail_page_alloc (task 필터 지원) | queue 생성 경로의 메모리 할당 실패 | `FAILSLAB`, `FAIL_PAGE_ALLOC` |
| `kernel.io_timeout` | fail_io_timeout (블록 장치별 `io-timeout-fail`) | 요청 타임아웃 경로 | `FAIL_IO_TIMEOUT` |
| `kernel.make_request` | fail_make_request | 블록 계층 제출 실패 | `FAIL_MAKE_REQUEST` |
| `gdb.checkpoint` | gdb breakpoint로 정지 → 다른 규칙 활성화 → 재개 | "compaction 특정 단계 진입 직후 CFS" 같은 정밀 경쟁 | (gdbstub) |

- 드라이버 고유 함수(예: qmgmt 내부 단계)에 `kernel.function`/`bpf_override`를 쓰려면 그 함수에 `ALLOW_ERROR_INJECTION`이 있어야 한다. 이건 드라이버 쪽 결정이다. 허용 목록은 adapter에 적고, 세션 시작 시 `/sys/kernel/debug/fail_function/injectable`과 대조해 검증한다.
- 범위 제한: failslab/fail_page_alloc은 시스템 전체에 영향을 주므로 반드시 `task-filter`를 쓴다. 대상은 trigger를 실행하는 스레드(stub의 sync 호출 스레드나 async kthread)로 한정한다. 아니면 게스트 전체가 불안정해져 판정을 신뢰할 수 없다.
- 모든 백엔드는 D7을 따른다: 테스트 범위 수명, 종료 시 자동 해제, 발동 이력(`FaultFired` 이벤트) 기록.

---

## 8.5 SR-IOV 규칙 (v0.5 추가)

sriov feature가 따라야 할 스펙과 구현의 사실이다. 시나리오를 생성하거나 판정할 때 이 규칙을 전제로 한다.

| ID | 규칙 | 근거 | 플랫폼에 미치는 영향 |
|---|---|---|---|
| SR-1 | **VQ 자원에는 admin queue가 포함된다.** VF에 VQ를 N개 배정하면 IO queue pair는 N−1개다. VI도 admin vector를 포함한다 | QEMU: secondary 컨트롤러의 `conf_ioqpairs = nvq - 1` (`qemu-debug/qemu-11.1.2/hw/nvme/ctrl.c:8028`). VF마다 최소 2개 VQ가 필요하다: `sriov_vq_flexible >= 2 * sriov_max_vfs` (`ctrl.c:8914`) | INV-V1과 vector 개수 정책(`min(VI, 쌍 수 + 1)`)은 이 규칙으로 계산한다. 시나리오 생성기는 VQ < 2를 유효하지 않은 배정(오류 기대)으로 분류한다 |
| SR-2 | **자원 배정(Secondary Assign)은 VF 컨트롤러가 Offline일 때만 허용된다** | QEMU: secondary가 online(`sctrl->scs`)이면 `NVME_INVALID_SEC_CTRL_STATE`를 반환한다 (`ctrl.c:7298~` `nvme_assign_virt_res_to_sec`). [[nvme-sriov-dynamic-resource-design]] §2.1 | VF 자원 변경 시나리오는 항상 `offline → assign → online` 순서다. 이 순서 자체를 순서 규칙(SR-S1)으로 검사한다. online 상태에서 assign하는 것은 오류 기대 시나리오로 둔다 |
| SR-3 | **VF의 Vendor ID/Device ID는 config 공간에서 0xFFFF로 읽힌다** (SR-IOV 스펙). 그래서 VF 준비 완료 여부를 vendor ID로 확인할 수 없다 | QEMU: VF config에 vendor/device id 0xffff 설정 (`qemu-debug/qemu-11.1.2/hw/pci/pcie_sriov.c:186~187, 340`). 커널: `pci_dev_wait()`는 VF면 `PCI_VENDOR_ID` 대신 `PCI_COMMAND`를 읽는다 (`qemu-debug/linux-7.2/drivers/pci/pci.c:1237~1241`) | 플랫폼이 VF 준비를 확인할 때(agent나 QMP로 config 읽기) **Command 레지스터**를 쓴다. 0xFFFF 읽기를 "장치 없음"으로 해석하는 공통 헬퍼를 VF에 적용하지 않는다. VF 식별은 PF의 SR-IOV capability(VF offset/stride)와 sysfs `physfn` 링크로 한다 |
| SR-4 | PF 리셋 시 모든 VF가 Offline이 된다 | [[nvme-sriov-dynamic-resource-design]] §0 | E2 판정 시 VF 상태 기대값은 Offline이다 |

SR-IOV 순서 규칙:
- **SR-S1:** `VirtMgmt(assign, cntlid=c)` 시점에 c는 Offline이다.
- **SR-S2:** VF c에서 `VirtMgmt(online)` 이후 VF 드라이버가 재초기화를 마치면 INV-Q*와 INV-V1이 성립한다(L3으로 종결을 검사).

---

## 9. 시나리오 예

```python
@pytest.mark.feature("qmgmt")
def test_compact_rollback_on_delete_sq_fail(vm, faults, trigger, agent, oracle):
    agent.fio_start(profile="randrw-verify", jobs=8)
    faults.add("r1", trigger=dict(queue="admin", opcode=0x00, nth=1),
               action=dict(type="status", sct=1, sc=0x0C, dnr=True))
    r = trigger("compact", fn="nvme0")
    assert r.ret != 0                                  # 실패를 상위로 보고해야 한다
    oracle.sequence.expect(["S1", "S6", "E1"])         # 순서 제약 + 롤백
    oracle.invariant.expect(["INV-Q1", "INV-Q2", "INV-Q3", "INV-I3"])
    assert agent.fio_stop().verify_errors == 0
    # 종료 시 fixture가 fault 해제, 최종 스냅샷, verdict.json 기록
```

---

## 10. 디렉토리 구조 [제안]

위치: `company/nvmeval/`, 독립 git 저장소. 설계 문서는 `deep/`에 둔다.

```
nvmeval/
├── pyproject.toml              # Python ≥3.9. 의존성: qemu.qmp, drgn, pytest, pyyaml
├── README.md                   # 사용법만. 설계는 deep/ 문서에 링크
│
├── nvmeval/                    # ── 호스트 패키지 (드라이버 비의존 코어) ──
│   ├── schema/                 # D1: 정규 상태 스키마
│   │   ├── core.py             #   Snapshot, Function, SQ, CQ, Vector, Hctx, Unknown
│   │   ├── ext.py              #   feature 확장 등록
│   │   └── events.py           #   정규 이벤트 타입
│   ├── backends/               # 통신만 담당 (판정 없음)
│   │   ├── qmp.py              #   qemu.qmp(asyncio), 이벤트 구독
│   │   ├── gdbrsp.py           #   순수 Python GDB RSP (bp, mem r/w, cont)
│   │   ├── drgnconn.py         #   drgn --qemu 연결 (전용 스레드)
│   │   ├── agent.py            #   게스트 agent RPC 클라이언트 (virtio-serial)
│   │   └── tracefile.py        #   QEMU trace 파일 스트리밍
│   ├── views/                  # 원본 → 정규 스키마
│   │   ├── driver.py           #   adapter 해석 + drgn
│   │   ├── device.py           #   DeviceBackend: vssd(QMP) / vfio-user sim(향후)
│   │   └── irq.py
│   ├── events/                 # 원본 → 정규 이벤트 (출처별 normalizer)
│   ├── adapter/
│   │   ├── loader.py           #   로드 + 세션 시작 시 검증
│   │   └── scan.py             #   adapter-scan
│   ├── oracle/
│   │   ├── invariant.py
│   │   ├── sequence.py         #   부분 순서 위의 규칙 엔진
│   │   └── phase.py
│   ├── control/
│   │   ├── trigger.py          #   kind=ioctl | call
│   │   ├── passthru.py
│   │   ├── fault.py            #   FaultPlan (QEMU / fault_inject / gdb)
│   │   └── checkpoint.py
│   ├── machine/
│   │   ├── cmdline.py          #   profile → QEMU 인자
│   │   ├── vm.py               #   기동, 종료, watchdog, panic 감지
│   │   └── artifacts.py
│   ├── plugin.py               # Feature 인터페이스 + 로더 (외부 경로 지원)
│   ├── pytest_plugin.py        # fixture: vm, faults, trigger, agent, views, oracle
│   └── cli.py                  # nvmeval doctor | adapter-scan | run | report
│
├── features/                   # ── 기능 플러그인 ──
│   ├── common/                 #   INV-Q*, INV-I*, INV-IO (모든 드라이버 공통)
│   └── upstream_selftest/      #   upstream 드라이버로 플랫폼 자체 검증 (리셋 절차 S2/S5 등)
│   # qmgmt/, irq_remap/, sriov/ 는 회사 정보 포함 → 외부 경로 가능 (미결 Q2)
│
├── adapters/
│   └── upstream-v7.2.yaml      #   개발 서버 자체 검증용
│
├── profiles/
│   ├── dev-server.yaml         #   TCG, qemu-debug 경로, 이 서버 커널
│   └── oa-pc.example.yaml      #   OA PC용 예시
│
├── guest/                      # ── 게스트 배포물 (이 디렉토리만 복사) ──
│   ├── agent/                  #   Python 3.9, stdlib만 사용
│   │   ├── nvmeval_agent.py
│   │   └── nvmeval-agent.service
│   ├── stub/                   #   nvmeval_stub.ko (C, out-of-tree, KDIR 지정 빌드)
│   │   ├── nvmeval_stub.c
│   │   ├── nvmeval_stub_uapi.h
│   │   └── Makefile
│   ├── bpf/                    #   bpftrace 스크립트 (정규 이벤트 원본 형식으로 출력)
│   └── kconfig/nvmeval.config  #   필요한 config 조각 (CentOS config에 merge)
│
├── qemu/
│   ├── patches/                #   qemu-dev 브랜치에서 git format-patch로 export + series
│   └── README.md               #   적용 대상(11.1.0 / 11.1.2), 적용 방법
│
├── images/
│   ├── build-rootfs.sh         #   CentOS Stream 9 qcow2 + cloud-init seed
│   └── cloud-init/
│
├── tests/                      # ── 플랫폼 단위 테스트 (VM 없이) ──
│   ├── test_schema.py
│   ├── test_invariant_engine.py
│   ├── test_sequence_engine.py #   합성 이벤트로 부분 순서 / INCONCLUSIVE 검증
│   └── test_adapter_loader.py
│
└── artifacts/                  # 실행 결과 (.gitignore)
```

| 원칙 | 구조에 반영된 곳 |
|---|---|
| 드라이버 API가 계속 바뀐다 | 드라이버 지식은 `adapters/*.yaml`(데이터)과 `features/*`(로직)에만 있다. `nvmeval/`에는 없다 |
| 통신, 변환, 판정 분리 | `backends` → `views`/`events` → `oracle`. 판정은 VM 없이 단위 테스트한다(`tests/`) |
| 게스트 배포물 격리 | `guest/`만 게스트에 복사한다. Python 3.9 제약은 이 디렉토리에만 적용된다 |
| QEMU 트리 미포함 | 개발은 `qemu-debug/qemu-11.1.2`의 qemu-dev 브랜치에서 하고, 결과만 patch series로 둔다 |
| 환경 차이 | `profiles/`만 바꾼다 |

---

## 11. 실행 환경 제약

### 11.1 실사용 환경 (CentOS Stream 9 + v7.2)

| 항목 | 영향 | 결정 |
|---|---|---|
| CentOS 9 기본 Python 3.9 | 게스트 agent가 게스트 python3로 실행된다 | agent는 3.9 호환, stdlib만(fcntl, ctypes, json, select, socket). 호스트도 3.9 이상(`match` 문, PEP 695 금지) |
| 게스트가 실제 rootfs를 가진다 | 9p hostroot + chroot 트릭이 필요 없다 | agent, bpftrace, fio, nvme-cli를 게스트에 설치한다. 9p는 쓰기 가능한 산출물 공유 하나만 둔다 |
| CentOS config 기반 v7.2 빌드 | BPF/BTF/KPROBES/KALLSYMS_ALL은 켜져 있을 가능성이 높다. fault injection 계열, `FW_CFG_SYSFS`, `PVPANIC`은 확인이 필요하다 | `nvmeval doctor`가 게스트 config를 자동 점검한다 (§11.3 목록) |
| debug symbol 있음 | drgn이 DWARF로 타입 기반 읽기를 할 수 있다 | adapter 필드 경로를 C 표현식으로 둔다 |
| QEMU 11.1 (개발은 11.1.2) | 같은 minor 버전이다 | 패치를 v11.1.0에도 rebase해 확인한다 |

### 11.2 개발 서버

| 항목 | 현재 상태 | 조치 |
|---|---|---|
| 게스트 | initramfs + busybox. 실환경과 다르다 | CentOS Stream 9 qcow2를 virtio-blk rootfs로 두고, v7.2를 `-kernel`로 부팅 |
| 게스트 커널 config (`qemu-debug/linux-7.2/.config`) | `XFS_FS` 없음(CentOS rootfs가 xfs), `PVPANIC` 없음, `DEBUG_INFO_BTF` 없음, **fault injection 계열 전부 없음**(`FAULT_INJECTION`부터 `FAIL_*`, `BPF_KPROBE_OVERRIDE`까지). `INTEL_IOMMU=y`, `IRQ_REMAP` 없음(AS-3의 non-IR 모드). VIRTIO_BLK/CONSOLE, 9P, FW_CFG_SYSFS, BPF_SYSCALL, KPROBES, KALLSYMS_ALL은 있음 | config 보강 후 재빌드 (2026-11-02 확인) |
| QEMU trace backend | `log` backend로 빌드됨 (`build/config-host.h`의 `CONFIG_TRACE_LOG`) | `-trace` 출력 파일을 스트리밍 파싱 |
| Python 도구 | drgn은 `company/drgn-study` 소스 빌드만 있고 모듈 설치는 안 됨. `qemu.qmp` 없음 | 프로젝트 venv에 설치 |

---

### 11.3 `nvmeval doctor` 점검 항목 (v0.5)

| 분류 | config | 없으면 |
|---|---|---|
| 필수: 관측 | `KALLSYMS_ALL`, `KPROBES`, `BPF_SYSCALL`, `BPF_EVENTS`, `DEBUG_INFO_BTF`, `FW_CFG_SYSFS`(drgn `--qemu`의 vmcoreinfo) | 실행 거부 |
| 필수: 통신 | `VIRTIO_CONSOLE`, `9P_FS`, `NET_9P_VIRTIO` | 실행 거부 |
| 필수: 장애 감지 | `PVPANIC`, `PVPANIC_PCI` | 실행 거부 |
| **fault injection** | `FAULT_INJECTION`, `FAULT_INJECTION_DEBUG_FS`, `FUNCTION_ERROR_INJECTION`, `FAIL_FUNCTION`, `BPF_KPROBE_OVERRIDE`, `FAILSLAB`, `FAIL_PAGE_ALLOC`, `FAIL_IO_TIMEOUT`, `FAIL_MAKE_REQUEST` | 해당 `kernel.*` 백엔드를 쓰는 테스트만 SKIP (사유 기록) |
| 모드 식별 | `IRQ_REMAP`, `INTEL_IOMMU`, `GENERIC_PENDING_IRQ` | 실패가 아니라 AS-3 모드(IR / non-IR)를 결정하는 입력 |

config 조각은 `guest/kconfig/nvmeval.config`로 제공하고, CentOS config에 `scripts/kconfig/merge_config.sh`로 합친다.

## 12. 수정이 필요한 곳과 단계별 계획

| 위치 | 작업 |
|---|---|
| `qemu-debug/qemu-11.1.2` (qemu-dev) | `query-nvme-queues`, `nvme-fault-*` 규칙 엔진, trace event 보강 → patch series로 export |
| 게스트 커널 config | §11의 누락 항목 보강 |
| `nvmeval/guest/stub` | `nvmeval_stub.ko` (드라이버는 수정하지 않음) |
| `nvmeval/` | 호스트 패키지 |

| 단계 | 내용 | 완료 기준 (이 서버) |
|---|---|---|
| M0 | schema, oracle 엔진, adapter loader + 단위 테스트 | VM 없이 합성 스냅샷/이벤트로 PASS/FAIL/INCONCLUSIVE 판정 테스트 통과 |
| M1 | machine + QMP + agent + CentOS 9 게스트 이미지 | smp 8 기동, agent ping, passthrough Identify 성공 |
| M2 | QEMU `query-nvme-queues` + DeviceView + DriverView(upstream adapter) | upstream 드라이버에서 INV-Q1/Q2/Q5 통과 (stopped 스냅샷) |
| M3 | IrqView + INV-I1~I5 + AS-3 | MSI destination ↔ affinity 대조 통과. non-IR 모드에서 affinity 변경 후 `move_pending` 관측 → I/O 유발 → 반영 확인(L6). IR profile에서는 즉시 반영 확인 |
| M4 | QEMU fault 규칙 엔진 | passthrough Delete SQ가 지정 status로 실패. CFS 설정 후 드라이버 리셋 관측 |
| M5 | `nvmeval_stub.ko` + trigger | upstream의 외부 호출 가능한 함수 하나를 kind=call로 호출 성공 |
| M6 | eBPF normalizer + sequence 엔진 + liveness 엔진 + checkpoint | upstream 리셋 절차에서 S2/S5/L3 통과. 이벤트 손실 시 INCONCLUSIVE, `drop` fault로 만든 hang이 L1 FAIL로 잡힘 |
| M6.5 | 커널 fault 백엔드 (`kernel.*`) | fail_function/failslab(task-filter)로 upstream 리셋 경로의 할당 실패를 주입하고 L3 종결(DEAD 또는 LIVE) 확인 |
| M7 | features/qmgmt + sriov profile | **OA PC**에서 qmgmt adapter로 S1~S7/E1~E2 실행 |

---

## 13. 알려진 위험과 검토 포인트 (리뷰어 중점 검토 요청)

| ID | 위험 | 현재 대응 | 검토 요청 |
|---|---|---|---|
| R1 | running 상태에서 drgn으로 읽으면 torn read가 생긴다 | 판정은 stopped 스냅샷만 쓴다 (§4.4) | TCG에서 정지·재개를 자주 하면 비용이 얼마나 될지, 판정 빈도를 정하는 기준이 적절한지 |
| R2 | stop 시 `bdrv_drain_all()` 때문에 device view가 driver view보다 앞설 수 있다 | 진행 정도 필드는 "같거나 앞섬" 관계로 비교한다 (§4.4) | 구성 필드도 영향을 받는 경우가 있는지. 예: 정지 직전 처리된 Delete SQ의 CQE를 드라이버가 아직 처리하지 않은 경우 → INV-Q1이 거짓 FAIL을 낼 수 있다. 처리 대기 admin 명령을 고려한 비교 규칙이 필요한지 |
| R3 | stub이 임의 커널 함수를 호출하면 락이나 상태 전제를 어길 수 있다 | 화이트리스트, 외부 호출 가능한 진입점으로 한정 (§7.2) | 진입점 계약을 adapter에 명시하는 방식(필요한 락, 컨텍스트)이 필요한지 |
| R4 | `kallsyms_lookup_name`이 export되지 않는다 (5.7 이후) | kprobe 등록으로 주소를 얻는 기법을 쓴다 | v7.2에서 이 기법이 유효한지, 대안이 있는지 |
| R5 | eBPF 이벤트 손실로 순서 규칙이 거짓 PASS를 낼 수 있다 | 손실 카운터 → INCONCLUSIVE (§5.4) | bpftrace에서 손실 카운터를 확실히 얻을 수 있는지. libbpf 직접 사용이 필요한지 |
| R6 | 정규 스키마가 미래 기능을 수용하지 못할 수 있다 | core 최소화 + ext 네임스페이스 | function / queue / vector / hctx 외의 core 단위가 필요한지 (예: namespace, multipath path) |
| R7 | phase window 경계 이벤트가 누락되면 공통 불변식이 잘못 켜지거나 꺼진다 | 경계 이벤트 누락 시 해당 구간 INCONCLUSIVE | 경계를 이벤트 대신 driver view 상태(quiesced 플래그)로 확인하는 이중화가 필요한지 |
| R8 | `phase=after_ok` + status 변조는 장치와 드라이버의 인식을 의도적으로 어긋나게 한다 | 롤백 검증용으로만 쓰고, 해당 테스트에서는 E1 기준 자체를 "장치 상태 기준 재동기화"로 정의 | 이 시나리오에서 불변식 정의가 일관적인지 |
| R9 | TCG에서 게스트 시간 흐름이 불규칙해 드라이버 타임아웃이 거짓으로 발생할 수 있다 | 게스트 `nvme_core.io_timeout`/`admin_timeout`을 넉넉하게 설정 | 타임아웃 경로 자체를 검증할 때(`drop` 동작)와 충돌하지 않는지 |
| R10 | vssd(QEMU)와 실제 SSD/FW의 동작 차이 | NFR-3 백엔드 추상화, 향후 vfio-user sim | – |
| R11 | (v0.5) liveness 기한을 게스트 jiffies로 정하는데, 게스트가 hang 상태여도 타이머 인터럽트는 돌아 jiffies가 증가한다 | 의도한 동작이다. 게스트 시간이 흘렀는데 종결되지 않았으면 hang FAIL로 판정한다. 게스트 시간 자체가 멈춘 경우만 플랫폼 오류로 본다 | 기한 기본값(게스트 시간 30초 등)이 적절한지. TCG에서 smp 8 기준 |
| R12 | (v0.5) blk-mq tag 순회를 drgn으로 할 때, running 상태에서는 tag 비트맵과 request 상태를 읽는 사이에 값이 바뀐다 | S1 판정은 `DevDeleteSQ` 시점의 checkpoint(stopped)에서 한다. 그러려면 장치 쪽 Delete SQ 처리 직전에 VM을 정지시키는 hook이 필요하다 → fault 규칙 엔진에 `action=pause`(QMP stop 요청 이벤트) 추가 | QEMU 디바이스 코드 안에서 vm_stop을 요청하는 것이 안전한지(BH로 지연 요청) |
| R13 | (v0.5) 예외 상태(AS-*)가 많아지면 불변식이 사실상 무력화될 수 있다 | 예외는 이름으로 선언하고, PASS(DEGRADED)로 노출한다. 시나리오별로 허용할 예외를 명시한다 | 예외 판정 조건 자체가 잘못될 위험(예: fallback이 아닌데 fallback으로 오인)을 어떻게 막을지 |

---

## 14. 미결 사항

| ID | 내용 | 영향 |
|---|---|---|
| Q1 | OA PC에서 KVM 사용 여부 | TCG에서도 동작하게 설계하므로 필수는 아니다. R9 완화에 영향 |
| Q2 | 회사 정보가 들어가는 `features/qmgmt`, `adapters/qmgmt-oa.yaml`을 같은 저장소에 둘지, 별도 저장소로 분리해 profile의 경로로 불러올지 | NFR-8, 저장소 구성 |
| Q3 | qmgmt bitmap 단위: qid당 1비트인지, VF별로 따로 있는지 | ext.qmgmt 스키마, INV-V1 |
| ~~Q4~~ | ~~drain 판정 기준~~ → 해결 (v0.5): blk-mq tag 상태 + 장치 상태가 주 판정, feature 카운터는 대조용 (§5.7) | – |
| Q5 | trigger나 checkpoint 대상 중 inline으로 사라진 함수가 있는지 | `noinline` 필요 여부 (adapter 검증으로 확인 가능) |
| Q6 | qmgmt 외에 개발 중인 기능 목록과 성격 | 스키마 core 범위(R6), feature 설계 |
| Q7 | QEMU 패치 관리: qemu-dev 브랜치 개발 + patch export(제안) vs submodule | §10 `qemu/` 구성 |
| Q8 | 저장소 이름 `nvmeval` 확정 여부 | – |
| Q9 | (v0.5) OA PC 실장비 환경에 interrupt remapping(IOMMU IR)이 있는지 | AS-3에서 어느 모드를 기준으로 삼을지 |
| Q10 | (v0.5) 드라이버의 vector 개수 정책이 `min(VI, 쌍 수 + 1)`인지, admin vector를 IO와 공유하는지 | qmgmt/sriov feature 불변식 |
| Q11 | (v0.5) qmgmt 내부 단계 중 오류 주입 대상 함수에 `ALLOW_ERROR_INJECTION`을 붙일 수 있는지 | `kernel.function`/`bpf_override` 사용 가능 범위 |

---

## 15. 리뷰 반영 이력

### 1차 외부 리뷰 (2026-11-02) → v0.5

| # | 피드백 | 검토 결과 | 반영 위치 |
|---|---|---|---|
| 1 | 판정 축에 liveness 추가 (요청 완료/실패, fio 종료, 컨트롤러 LIVE/DEAD 도달) | **수용.** safety 규칙만으로는 hang이 PASS로 나온다는 지적이 정확하다. TCG를 고려해 기한은 게스트 진행량(jiffies)으로 정하고, 벽시계 상한은 따로 둔다. drain 종결(L4), trigger 반환(L5), IRQ 이동 반영(L6)도 추가했다 | §5.5, D2 |
| 2 | 정상 과도 상태 예외 (빈 풀, IRQ 이동 지연, blk-mq fallback) | **수용, 코드로 확인함.** (a) IRQ 이동 지연: non-IR x86 MSI는 `IRQCHIP_MOVE_DEFERRED`라서 다음 인터럽트에서 반영된다(`arch/x86/kernel/apic/msi.c:216~217`, `kernel/irq/manage.c:304~312`). 개발 서버 커널도 `IRQ_REMAP`이 없어 이 모드다. (b) fallback: `blk_mq_update_nr_hw_queues` 실패 fallback(`block/blk-mq.c:5162~5176`), 재매핑 전 전체 CPU→0 초기화(`block/blk-mq.c:4777~4780`). 예외를 불변식에 흩지 않고 이름 붙인 상태(AS-1~5)로 선언하고, DEGRADED로 드러나게 했다. IR/non-IR profile을 둘 다 둔다 | §5.6, §5.4 |
| 3 | CQ→vector N:1 (VQ > VI) 스키마/불변식, 벡터 수 `min(VI, 쌍 수+1)`, INV-I1은 공유 vector의 affinity 합집합과 비교 | **수용하되 일부 재구성.** 스키마에 `vectors[v].cqids` 역참조를 추가하고 N:1 일치 불변식(INV-I5)을 만들었다. 다만 INV-I1(MSI destination ∈ effective affinity)은 vector 단위라서 공유 여부와 관계없이 그대로 맞다. 합집합 비교는 별도 불변식 INV-I4(공유 CQ들의 hctx CPU 합집합 ⊆ vector affinity)로 분리했다. 벡터 수 공식은 드라이버 정책이므로 core가 아니라 feature 불변식으로 두고 확인을 요청했다(Q10) | §4.1, §4.2, §5.1 |
| 4 | drain 판정은 기능 카운터가 아니라 blk-mq 상태(`q_usage_counter`, tag busy 비트맵)로 | **수용, 보정 포함.** 검증 대상의 자체 카운터로 판정하면 안 된다는 원칙에 동의한다. 다만 `q_usage_counter`는 **freeze를 쓸 때만** drain 지표가 된다. quiesce만 하면 sw queue에 대기 중인 request가 참조를 잡고 있어 0이 아닌 것이 정상이다. 그래서 주 판정은 tag 비트맵 + request 상태 `MQ_RQ_IN_FLIGHT`(커널의 `blk_mq_hctx_has_requests`와 같은 방식)로 하고, `q_usage_counter`는 보조로 둔다. 장치 쪽 SQ head=tail 교차 확인과, feature 카운터와의 대조(불일치 시 FAIL)를 추가했다. stopped 상태에서 판정해야 하므로 R12(`action=pause`)를 새로 만들었다 | §5.7, S1, R12, Q4 해결 |
| 5 | 커널 쪽 장애 주입도 FaultPlan에 포함, config 점검에 fault injection 추가 | **수용.** v0.4에서는 nvme fault_inject만 보조 수단으로 있었다. 이를 fail_function, bpf_override_return, failslab/fail_page_alloc(task-filter 필수), fail_io_timeout, fail_make_request까지 1급 백엔드로 넓혔다. 개발 서버 커널에는 이 계열 config가 **하나도 없음**을 확인했다. `nvmeval doctor` 점검 목록에 추가했다 | §8.1, §11.2, §11.3, M6.5 |
| 6 | SR-IOV: VQ는 admin queue 포함, 자원 배정은 VF offline에서만, VF vendor ID는 0xFFFF라 준비 확인은 Command 레지스터로 | **수용, 세 가지 모두 코드로 확인함.** VQ admin 포함: `conf_ioqpairs = nvq - 1`(`hw/nvme/ctrl.c:8028`), VF당 최소 2 VQ(`ctrl.c:8914`). offline 전용: online이면 `INVALID_SEC_CTRL_STATE`(`ctrl.c:7298~`). VF ID 0xFFFF: QEMU `hw/pci/pcie_sriov.c:186~187`, 커널 `pci_dev_wait`가 VF면 `PCI_COMMAND`를 읽음(`drivers/pci/pci.c:1237~1241`). (리뷰 원문의 "0xfffff"는 16비트 0xFFFF의 오타로 보인다) | §8.5, INV-V1 |

