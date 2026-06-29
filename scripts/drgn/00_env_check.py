#!/usr/bin/env drgn
# -*- coding: utf-8 -*-
#
# [한국어] 00_env_check.py — drgn 라이브 디버깅 환경/타입소스 사전 점검
#
# === 이 스크립트의 역할 ===
# drgn 으로 본격적인 자료구조 탐색에 들어가기 전에, "지금 이 drgn 세션이
# 커널 타입 정보를 제대로 잡았는가"를 확인하는 스모크 테스트다.
# 타입을 못 잡으면(BTF/DWARF 부재) 이후 모든 스크립트가 LookupError 로 죽는다.
#
# 확인 항목:
#   1) 커널 릴리스/버전 (uts_namespace 또는 init_uts_ns)
#   2) 임의 커널 전역(jiffies, init_task)을 실제로 읽을 수 있는가 → 메모리 접근 OK
#   3) 모듈 타입(struct nvme_dev) 을 해석할 수 있는가 → 모듈 BTF/DWARF OK
#   4) 블록 디바이스 목록 (for_each_disk)
#
# 실행: sudo -E drgn 00_env_check.py
#
# drgn CLI 로 실행하면 prog 전역과 helpers.linux.* 가 자동 주입된다.
# python 으로 직접 돌릴 경우를 대비해 명시적 import + prog 폴백을 둔다.

from drgn import Object, cast, container_of, FaultError, program_from_kernel
from drgn.helpers.linux.block import for_each_disk, disk_name

# [한국어] drgn CLI 가 주입한 prog 가 없으면(=python 직접 실행) 직접 라이브 커널에 attach.
#  program_from_kernel(): /proc/kcore 를 메모리 소스로, BTF/DWARF 를 타입 소스로 여는 헬퍼.
try:
    prog
except NameError:
    prog = program_from_kernel()


def line(title):
    print(f"\n=== {title} ===")


line("1) 커널 릴리스 / 타입소스 동작 확인")
# [한국어] init_uts_ns: 부팅 시 고정되는 전역 utsname. 이걸 읽을 수 있으면
#  (a) 전역 심볼 해석(kallsyms), (b) 타입 해석(BTF/DWARF), (c) 메모리 읽기(kcore)
#  3박자가 모두 동작한다는 뜻 — drgn 라이브 세션의 가장 빠른 헬스체크.
try:
    uts = prog["init_uts_ns"].name
    print("  release :", uts.release.string_().decode())
    print("  version :", uts.version.string_().decode())
    print("  machine :", uts.machine.string_().decode())
    print("  → 전역심볼 + 타입 + 메모리 접근 모두 OK")
except (KeyError, FaultError) as e:
    print("  [FAIL] init_uts_ns 접근 실패:", e)
    print("  → root 가 아니거나(/proc/kcore 권한), 타입 소스(BTF/DWARF) 부재")

line("2) 코어 타입 해석 (struct task_struct via init_task)")
# [한국어] init_task 는 0번 스레드(swapper)의 task_struct. comm 필드 접근으로
#  vmlinux 코어 타입(BTF)이 살아있는지 본다.
try:
    print("  init_task.comm :", prog["init_task"].comm.string_().decode())
    print("  init_task.pid  :", int(prog["init_task"].pid))
except (KeyError, FaultError) as e:
    print("  [FAIL] init_task:", e)

line("3) 모듈 타입 해석 (struct nvme_dev) — 모듈 BTF/DWARF 필요")
# [한국어] nvme_dev 는 nvme(pci) 모듈에 정의된 타입. 코어 vmlinux BTF 에는 없고
#  모듈 BTF(CONFIG_DEBUG_INFO_BTF_MODULES) 또는 debuginfod 모듈 DWARF 가 있어야 잡힌다.
#  여기서 성공하면 02/04 스크립트(NVMe/PRP)가 동작할 수 있다는 신호.
try:
    t = prog.type("struct nvme_dev")
    print("  struct nvme_dev 크기 :", t.size, "bytes — 모듈 타입 해석 OK ✓")
    t2 = prog.type("struct nvme_iod")
    print("  struct nvme_iod 크기 :", t2.size, "bytes")
except LookupError as e:
    print("  [WARN] nvme 모듈 타입 미해석:", e)
    print("  → 'modprobe nvme' 확인, 또는 debuginfod 활성화 필요")

line("4) 블록 디바이스 목록")
# [한국어] for_each_disk: 커널의 모든 gendisk 를 순회하는 헬퍼.
#  blk-mq/NVMe/PRP 스크립트가 다룰 대상 디스크(nvme0n1)가 보이는지 확인.
try:
    for disk in for_each_disk(prog):
        name = disk_name(disk).decode(errors="replace")
        # [한국어] disk.queue: 이 디스크의 request_queue (blk-mq 진입점).
        q = disk.queue
        nr_hw = int(q.nr_hw_queues) if hasattr(q, "nr_hw_queues") else -1
        print(f"  {name:12s}  request_queue={hex(q.value_())}  nr_hw_queues={nr_hw}")
except FaultError as e:
    print("  [FAIL] for_each_disk:", e)

print("\n[완료] 환경 점검 끝. 위 1~3 이 모두 OK 면 01~04 스크립트 실행 가능.")
