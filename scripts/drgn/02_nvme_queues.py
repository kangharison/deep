#!/usr/bin/env drgn
# -*- coding: utf-8 -*-
#
# [한국어] 02_nvme_queues.py — NVMe 컨트롤러의 SQ/CQ 링을 실시간 덤프
#
# === 이 스크립트의 역할 ===
# nvme0n1 뒤의 struct nvme_dev 를 찾고, 그 안의 nvme_queue[] (Admin + I/O 큐)를
# 순회하며 각 큐의 Submission Queue(SQ)/Completion Queue(CQ) 링 상태를 본다.
# blk-mq 의 HW 큐(hctx)가 실제로는 NVMe 의 SQ/CQ 쌍에 1:1로 매핑된다 — 그 "하드웨어
# 큐 실체"를 메모리에서 직접 확인하는 것이 목적이다.
#
# === 따라가는 경로 ===
#   gendisk(nvme0n1).private_data ─► struct nvme_ns *
#        └─ .ctrl ─► struct nvme_ctrl *
#              └─ container_of(ctrl, nvme_dev, ctrl) ─► struct nvme_dev *
#                    └─ .queues ─► struct nvme_queue * (배열: [0]=Admin, [1..]=I/O)
#                          ├─ .sq_cmds  : SQ 링 (dma_alloc_coherent, 호스트 RAM)  ─ nvme_command[]
#                          ├─ .cqes     : CQ 링 (dma_alloc_coherent, 호스트 RAM)  ─ nvme_completion[]
#                          ├─ .sq_tail / .cq_head : 드라이버가 관리하는 링 인덱스
#                          └─ .q_db     : 도어벨 레지스터(MMIO, BAR0) — 메모리로는 못 읽음(아래 주석)
#
# === 중요: 무엇을 읽을 수 있고 무엇을 못 읽는가 ===
#   읽기 O : sq_cmds/cqes (호스트 RAM에 dma_alloc_coherent 로 잡힌 링 버퍼) → drgn 가상읽기 가능
#   읽기 X : q_db(도어벨), dev->bar(컨트롤러 레지스터 CC/CSTS) → 이건 디바이스 MMIO 라서
#            /proc/kcore(=RAM 매핑)로는 안 읽힌다. 드라이버가 RAM에 캐싱한 미러값만 읽을 수 있다.
#
# 실행: sudo -E drgn 02_nvme_queues.py

from drgn import Object, cast, container_of, FaultError, program_from_kernel
from drgn.helpers.linux.block import for_each_disk, disk_name

try:
    prog
except NameError:
    prog = program_from_kernel()

TARGET = b"nvme0n1"
SQ_DUMP = 4   # [한국어] SQ 에서 덤프할 엔트리 수(상위 N개). 너무 많으면 잘라서 본다.


def find_disk(name):
    for disk in for_each_disk(prog):
        if disk_name(disk) == name:
            return disk
    return None


def get_nvme_dev(disk):
    """[한국어] gendisk → nvme_ns → nvme_ctrl → nvme_dev 로 거슬러 올라간다.
    nvme_dev 는 nvme_ctrl 을 멤버로 '포함'하므로 container_of 로 복원한다."""
    ns = cast("struct nvme_ns *", disk.private_data)   # gendisk.private_data 는 void*
    ctrl = ns.ctrl                                       # struct nvme_ctrl *
    nvme_dev = container_of(ctrl, "struct nvme_dev", "ctrl")
    return ns, ctrl, nvme_dev


# [한국어] NVMe Admin/IO opcode 일부 — 덤프를 사람이 읽기 쉽게 이름 붙이기 위한 표.
NVM_OPC = {0x00: "flush", 0x01: "write", 0x02: "read",
           0x09: "dsm", 0x0d: "write_zeroes"}
ADM_OPC = {0x00: "delete_sq", 0x01: "create_sq", 0x02: "get_log",
           0x04: "delete_cq", 0x05: "create_cq", 0x06: "identify",
           0x09: "set_features", 0x0a: "get_features", 0x0c: "async_event"}


disk = find_disk(TARGET)
if disk is None:
    print(f"[ERR] {TARGET.decode()} 없음")
else:
    ns, ctrl, dev = get_nvme_dev(disk)
    online = int(dev.online_queues)        # [한국어] 현재 살아있는 큐 수(Admin 포함)
    nr_alloc = int(dev.nr_allocated_queues) if hasattr(dev, "nr_allocated_queues") else online
    model = ctrl.model_number.string_().decode().strip() if hasattr(ctrl, "model_number") else "?"
    print(f"struct nvme_dev = {hex(dev.value_())}")
    print(f"model='{model}'  online_queues={online}  allocated_queues={nr_alloc}")
    # [한국어] dev->bar: 컨트롤러 레지스터가 ioremap 된 커널 가상주소(MMIO). 값 자체(주소)는
    #  읽히지만, 그 주소가 가리키는 레지스터 내용은 RAM 이 아니라 디바이스라 못 읽는다.
    print(f"dev->bar(ioremap, MMIO 주소만)= {hex(dev.bar.value_())}")
    print(f"dev->dbs(도어벨 베이스, MMIO)  = {hex(dev.dbs.value_())}")

    for qid in range(online):
        # [한국어] dev.queues 는 nvme_queue 배열의 시작 포인터. [qid] 로 인덱싱.
        nvmeq = dev.queues[qid]
        depth = int(nvmeq.q_depth)
        sq_tail = int(nvmeq.sq_tail)
        cq_head = int(nvmeq.cq_head)
        real_qid = int(nvmeq.qid)
        is_admin = (real_qid == 0)
        sq_dma = int(nvmeq.sq_dma_addr)    # [한국어] SQ 의 DMA 주소(=물리주소, IOMMU 없음)
        cq_dma = int(nvmeq.cq_dma_addr)
        print(f"\n── queue[{qid}] qid={real_qid} ({'ADMIN' if is_admin else 'IO'}) "
              f"depth={depth} sq_tail={sq_tail} cq_head={cq_head}")
        print(f"     sq_dma_addr={hex(sq_dma)}  cq_dma_addr={hex(cq_dma)}")

        # [한국어] SQ 링 덤프: sq_cmds(void*)를 nvme_command* 로 캐스팅 후 상위 엔트리 출력.
        #  sq_cmds 는 dma_alloc_coherent 로 잡힌 호스트 RAM → 가상주소로 그냥 읽힌다.
        try:
            sqc = cast("struct nvme_command *", nvmeq.sq_cmds)
            tbl = ADM_OPC if is_admin else NVM_OPC
            print(f"     SQ 상위 {SQ_DUMP} 엔트리:")
            for i in range(min(SQ_DUMP, depth)):
                c = sqc[i]
                opc = int(c.common.opcode)
                cid = int(c.common.command_id)
                nsid = int(c.common.nsid)
                name = tbl.get(opc, f"0x{opc:02x}")
                # [한국어] dptr.prp1: 이 명령의 첫 데이터 페이지 물리주소(PRP1). 04 스크립트에서 심층 분석.
                prp1 = int(c.common.dptr.prp1)
                print(f"       [{i}] cid={cid:<5d} opcode={name:<10s} nsid={nsid} prp1={hex(prp1)}")
        except FaultError as e:
            print(f"     [SQ 읽기 실패] {e}")

    print("\n[안내] q_db/bar 레지스터(CC, CSTS, 도어벨)는 디바이스 MMIO 라 메모리로는 못 읽는다.")
    print("       SQ/CQ 링 내용·인덱스는 호스트 RAM 이라 위처럼 직접 관찰 가능하다.")
