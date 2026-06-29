#!/usr/bin/env drgn
# -*- coding: utf-8 -*-
#
# [한국어] 01_blkmq_inflight.py — blk-mq "진행 중(in-flight)" 요청 실시간 덤프
#
# === 이 스크립트의 역할 ===
# nvme0n1 의 request_queue 에 현재 떠 있는(완료되지 않은) struct request 들을
# 모두 찾아내고, 각 요청의 핵심 필드(op, 섹터, 길이, 상태, 태그, 어느 HW 큐에
# 디스패치됐는지)를 출력한다. "지금 이 순간 커널이 어떤 I/O 를 디바이스로
# 내려보내고 있는가"를 코드 수정/트레이스포인트 없이 메모리에서 직접 본다.
#
# === blk-mq 자료구조 한눈에 (이 스크립트가 따라가는 경로) ===
#
#   gendisk(nvme0n1)
#       └─ .queue ─────────────► struct request_queue
#                                    └─ .tag_set ─► struct blk_mq_tag_set
#                                    └─ HW 큐들(hctx) ─► struct blk_mq_tags
#                                            └─ sbitmap(bitmap_tags) ─ 비트가 켜진 태그 = 할당된 요청
#                                                    └─ static_rqs[tag] ─► struct request  ◄── 우리가 보는 것
#
# drgn 헬퍼 request_queue_busy_iter(q) 가 위 비트맵 워킹을 대신 해준다.
#   - tags="driver" : 드라이버 태그가 할당된 요청 = HW 디스패치되어 디바이스로 내려간 것
#   - tags="sched"  : 스케줄러 태그가 할당된 요청 = 제출됐지만 아직 elevator 에 있을 수 있음
#   - 기본값: 스케줄러가 none 이면 "driver", 아니면 "sched"
#     (nvme0n1 은 mq-deadline 이므로 기본은 "sched")
#
# 실행: sudo -E drgn 01_blkmq_inflight.py
#   (in-flight 를 보려면 다른 터미널에서 fio/dd 부하를 깔아둘 것 — README 참조)

from drgn import Object, cast, container_of, FaultError, program_from_kernel
from drgn.helpers.linux.block import (
    for_each_disk, disk_name, request_queue_busy_iter,
    req_op, blk_rq_pos, blk_rq_bytes, rq_data_dir,
)

try:
    prog
except NameError:
    prog = program_from_kernel()

TARGET = b"nvme0n1"   # [한국어] 관찰 대상 디스크명. 환경에 맞게 바꿔도 됨.


def find_disk(name):
    """[한국어] 이름으로 gendisk 찾기. 못 찾으면 None."""
    for disk in for_each_disk(prog):
        if disk_name(disk) == name:
            return disk
    return None


def dump_request(rq, kind):
    """[한국어] struct request * 하나를 사람이 읽을 수 있게 출력.
    @rq:   struct request *  (busy iter 가 yield 한 객체)
    @kind: 'driver' or 'sched' (어느 태그 공간에서 잡혔는지 라벨)
    """
    # [한국어] req_op(rq): cmd_flags 하위 비트에서 연산 종류(REQ_OP_*) 추출.
    op = req_op(rq)
    # [한국어] blk_rq_pos: 요청 시작 섹터(__sector). 512B 섹터 단위.
    sector = int(blk_rq_pos(rq))
    # [한국어] blk_rq_bytes: 남은 전송 바이트(__data_len).
    nbytes = int(blk_rq_bytes(rq))
    # [한국어] state: enum mq_rq_state — 0=IDLE, 1=IN_FLIGHT, 2=COMPLETE.
    #  IN_FLIGHT 면 디바이스가 아직 완료를 안 올린 진짜 진행 중 요청.
    state = rq.state
    # [한국어] tag: 드라이버 태그(=NVMe command id 로 그대로 쓰임). internal_tag: 스케줄러 태그.
    tag = int(rq.tag)
    itag = int(rq.internal_tag)
    # [한국어] mq_hctx: 이 요청이 매핑된 HW 컨텍스트(=NVMe I/O 큐 1개에 대응).
    hctx = rq.mq_hctx
    hctx_idx = int(hctx.queue_num) if hctx else -1
    rw = "WRITE" if rq_data_dir(rq) else "READ"

    print(f"  [{kind}] rq={hex(rq.value_())} {str(op):>22s} {rw:5s} "
          f"sector={sector:<12d} bytes={nbytes:<8d} "
          f"state={str(state):>20s} tag={tag} itag={itag} hctx={hctx_idx}")


disk = find_disk(TARGET)
if disk is None:
    print(f"[ERR] 디스크 {TARGET.decode()} 를 찾지 못함. for_each_disk 결과 확인 필요.")
else:
    q = disk.queue
    sched = q.elevator.type.elevator_name.string_().decode() if q.elevator else "none"
    print(f"대상: {TARGET.decode()}  request_queue={hex(q.value_())}  scheduler={sched}")
    print(f"nr_hw_queues={int(q.nr_hw_queues)}  queue_depth(tagset)={int(q.tag_set.queue_depth)}")

    # [한국어] 드라이버 태그 공간: 실제로 디바이스에 내려간(=NVMe SQ 에 올라간) 요청들.
    print("\n[driver 태그 = HW 디스패치되어 디바이스로 내려간 요청]")
    n = 0
    for rq in request_queue_busy_iter(q, "driver"):
        dump_request(rq, "driver")
        n += 1
    print(f"  → driver in-flight 개수: {n}")

    # [한국어] 스케줄러 태그 공간: 제출됐고 mq-deadline elevator 에 있는 요청 포함.
    print("\n[sched 태그 = 제출되어 스케줄러에 들어온 요청]")
    m = 0
    for rq in request_queue_busy_iter(q, "sched"):
        dump_request(rq, "sched")
        m += 1
    print(f"  → sched in-flight 개수: {m}")

    if n == 0 and m == 0:
        print("\n[안내] in-flight 요청이 0개. 에뮬 NVMe는 완료가 빨라 한순간엔 비어있기 쉽다.")
        print("       다른 터미널에서 부하를 깔고(README 참조) 다시 실행할 것:")
        print("       sudo dd if=/dev/nvme0n1 of=/dev/null bs=1M iflag=direct")
