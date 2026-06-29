#!/usr/bin/env drgn
# -*- coding: utf-8 -*-
#
# [한국어] 04_prp_payload.py — in-flight NVMe 요청의 PRP 디스크립터 + 실제 페이로드 확인
#
# === 이 스크립트의 역할 (이 묶음의 정점) ===
# 진행 중인 NVMe 요청 하나를 골라, 드라이버가 만든 nvme_command 의 PRP(Physical
# Region Page) 디스크립터를 해독하고, PRP 가 가리키는 물리 페이지의 "실제 DMA 데이터
# 바이트"를 직접 읽어 hexdump 한다. 즉 "디바이스가 읽거나 쓸 바로 그 메모리"를
# 커널 수정 없이 관찰한다.
#
# === PRP 개념 요약 ===
# NVMe 명령은 데이터 버퍼를 PRP 엔트리(=4KB 페이지 물리주소)들로 기술한다.
#   - PRP1 : 첫 데이터 페이지의 물리주소(페이지 내 오프셋 포함 가능)
#   - PRP2 : (a) 데이터가 ≤2페이지면 두 번째 페이지 주소,
#            (b) 더 크면 'PRP 리스트' 페이지의 물리주소(엔트리 배열)
#   - PRP 리스트가 꽉 차면(512엔트리=2MB) 마지막 엔트리가 다음 리스트로 체이닝.
#
# === 6.17 커널의 nvme_iod 레이아웃 (구버전과 다름!) ===
#   struct nvme_iod {
#       struct nvme_request req;
#       struct nvme_command cmd;        // ◄ 드라이버가 만든 명령(여기 dptr.prp1/prp2)
#       u8 flags;                       // IOD_* (IOD_SMALL_DESCRIPTOR 등)
#       u8 nr_descriptors;              // 사용한 디스크립터 페이지 수
#       size_t total_len;               // 총 전송 길이
#       struct dma_iova_state dma_state;
#       void *descriptors[5];           // ◄ descriptors[0] = PRP 리스트 페이지(커널 가상주소)
#       ...
#   };
#   ※ 구버전의 iod->first_dma / iod->sgt / iod->use_sgl / iod->list[] 는 없다.
#
# === 왜 이 머신에서 '페이로드 바이트'까지 읽히나 ===
#   IOMMU 가 비활성(/sys/class/iommu 비어있음)이라 DMA 주소 == 물리 주소다.
#   따라서 PRP 엔트리(=DMA 주소)를 그대로 prog.read(addr, n, physical=True) 하면
#   실제 데이터 페이지가 읽힌다. IOMMU 가 켜져 있으면 PRP 는 IOVA 라서 이 직접 읽기가
#   안 되고, IOMMU 페이지테이블 워킹 또는 bio 소스 페이지 경유가 필요하다(문서 §7 참조).
#
# 실행: sudo -E drgn 04_prp_payload.py
#   (반드시 부하를 깔아 in-flight 요청을 만들 것. PRP 리스트까지 보려면 bs를
#    4KB 초과 32KB 미만으로: 예) fio --bs=16k --rw=randread --direct=1 ...)

from drgn import Object, cast, container_of, FaultError, program_from_kernel
from drgn.helpers.linux.block import (
    for_each_disk, disk_name, request_queue_busy_iter,
    req_op, blk_rq_bytes, blk_mq_rq_to_pdu,
)

try:
    prog
except NameError:
    prog = program_from_kernel()

TARGET = b"nvme0n1"
PAGE_SIZE = 4096                 # [한국어] x86_64 기본 페이지. (huge page 아닌 일반 I/O 가정)
PAGE_MASK = PAGE_SIZE - 1
PRPS_PER_PAGE = PAGE_SIZE // 8   # [한국어] PRP 리스트 1페이지가 담는 8바이트 엔트리 수 = 512
DUMP_BYTES = 32                  # [한국어] 각 데이터 페이지에서 hexdump 할 바이트 수
MAX_REQS = 4                     # [한국어] 한 번에 분석할 요청 최대 개수


def find_disk(name):
    for disk in for_each_disk(prog):
        if disk_name(disk) == name:
            return disk
    return None


def hexdump(phys, n):
    """[한국어] 물리주소 phys 에서 n바이트를 읽어 hex+ascii 로. IOMMU 없음 → phys 직접 읽기.
    실패(FaultError)면 사유를 문자열로 돌려준다(예: P2P/디바이스 메모리)."""
    try:
        data = prog.read(phys, n, physical=True)   # physical=True 가 핵심
    except FaultError as e:
        return f"<읽기 실패: {e}>"
    hexs = " ".join(f"{b:02x}" for b in data)
    asci = "".join(chr(b) if 32 <= b < 127 else "." for b in data)
    return f"{hexs}  |{asci}|"


def analyze_prp(iod, total_len):
    """[한국어] 한 요청의 PRP 디스크립터를 해독하고 데이터 페이지들을 hexdump.
    @iod: struct nvme_iod *
    @total_len: 전송 총 바이트(iod.total_len 또는 blk_rq_bytes)
    """
    cmd = iod.cmd                                   # struct nvme_command (임베디드)
    flags = int(cmd.common.flags)
    # [한국어] PSDT(bits[7:6]) : 0=PRP 사용, 그 외=SGL 사용. SGL 이면 PRP 해독 스킵.
    psdt = (flags >> 6) & 0x3
    prp1 = int(cmd.common.dptr.prp1)
    prp2 = int(cmd.common.dptr.prp2)
    nr_desc = int(iod.nr_descriptors)
    iod_flags = int(iod.flags)

    print(f"    nvme_command: opcode={int(cmd.common.opcode):#04x} "
          f"cid={int(cmd.common.command_id)} nsid={int(cmd.common.nsid)} "
          f"flags={flags:#04x} (PSDT={psdt} {'SGL' if psdt else 'PRP'})")
    print(f"    iod.flags={iod_flags:#04x} nr_descriptors={nr_desc} total_len={total_len}")
    print(f"    PRP1={prp1:#018x}  PRP2={prp2:#018x}")

    if psdt != 0:
        print("    → 이 명령은 SGL 경로(PSDT≠0). PRP 해독 생략. (bs를 sgl_threshold 미만으로)")
        return

    # [한국어] 첫 페이지가 담는 바이트 = 페이지 끝까지 남은 공간(오프셋 고려).
    offset = prp1 & PAGE_MASK
    first_page_bytes = PAGE_SIZE - offset
    print(f"    [page0] PRP1 phys={prp1:#018x} (offset_in_page={offset})")
    print(f"            payload: {hexdump(prp1, DUMP_BYTES)}")

    if total_len <= first_page_bytes:
        # [한국어] case A: 한 페이지로 끝. PRP2 는 사용 안 함(0).
        print("    → 데이터가 1페이지 이내. PRP2 미사용.")
        return

    remaining = total_len - first_page_bytes
    n_more_pages = (remaining + PAGE_SIZE - 1) // PAGE_SIZE  # 첫 페이지 이후 필요한 페이지 수

    if n_more_pages == 1:
        # [한국어] case B: 정확히 2페이지. PRP2 자체가 두 번째 데이터 페이지 주소.
        print(f"    [page1] PRP2 phys={prp2:#018x} (두 번째 데이터 페이지)")
        print(f"            payload: {hexdump(prp2, DUMP_BYTES)}")
        return

    # [한국어] case C: 3페이지 이상 → PRP2 는 'PRP 리스트' 페이지를 가리킨다.
    #  드라이버는 그 리스트 페이지의 커널 가상주소를 iod.descriptors[0] 에 보관한다.
    #  (PRP2 = 그 페이지의 물리주소. 둘은 같은 메모리를 가상/물리로 본 것.)
    print(f"    → 3페이지 이상. PRP2={prp2:#018x} = PRP 리스트 페이지(물리).")
    if nr_desc < 1:
        print("    [WARN] nr_descriptors=0 인데 멀티페이지? 레이아웃/타이밍 재확인 필요.")
        return

    # [한국어] descriptors[0] (void*, 커널 가상) 를 __le64 배열로 보고 엔트리들을 읽는다.
    prp_list = cast("__le64 *", iod.descriptors[0])
    print(f"    PRP 리스트(@descriptors[0]={hex(iod.descriptors[0].value_())}), "
          f"data 페이지 {n_more_pages}개:")

    if n_more_pages > PRPS_PER_PAGE - 1:
        # [한국어] 한 리스트 페이지(512엔트리)를 넘기면 체이닝 발생(>~2MB I/O). 여기선 첫 페이지만 표시.
        print(f"    [안내] 리스트가 {PRPS_PER_PAGE-1}엔트리 초과 → 체이닝됨. 첫 리스트 페이지만 출력.")

    show = min(n_more_pages, PRPS_PER_PAGE - 1, 8)   # 너무 길면 8개까지만
    for i in range(show):
        entry = int(prp_list[i])                     # i번째 데이터 페이지의 물리주소
        print(f"      list[{i}] phys={entry:#018x}  {hexdump(entry, 16)}")
    if n_more_pages > show:
        print(f"      ... (총 {n_more_pages}개 중 {show}개만 표시)")


disk = find_disk(TARGET)
if disk is None:
    print(f"[ERR] {TARGET.decode()} 없음")
else:
    q = disk.queue
    print(f"대상: {TARGET.decode()}  (IOMMU 없음 가정 → PRP=물리주소 직접 읽기)")
    n = 0
    # [한국어] driver 태그 = HW 에 디스패치되어 nvme_command 가 '이미 만들어진' 요청.
    #  (스케줄러 단계 요청은 아직 nvme_queue_rq 를 안 거쳐 PRP 가 없을 수 있다.)
    for rq in request_queue_busy_iter(q, "driver"):
        n += 1
        op = req_op(rq)
        nbytes = int(blk_rq_bytes(rq))
        print(f"\n[req #{n}] rq={hex(rq.value_())} {str(op)} bytes={nbytes} "
              f"tag={int(rq.tag)} state={str(rq.state)}")
        # [한국어] blk_mq_rq_to_pdu: request 바로 뒤에 붙은 드라이버 private 영역 = nvme_iod.
        iod = cast("struct nvme_iod *", blk_mq_rq_to_pdu(rq))
        # [한국어] iod.total_len 우선, 없으면 요청 바이트로 대체.
        try:
            total_len = int(iod.total_len)
        except (AttributeError, FaultError):
            total_len = nbytes
        try:
            analyze_prp(iod, total_len)
        except FaultError as e:
            print(f"    [분석 중 FaultError] {e} (요청이 그새 완료됐을 수 있음 — race)")
        if n >= MAX_REQS:
            break

    if n == 0:
        print("\n[안내] driver 태그 in-flight 요청이 0개.")
        print("       부하를 깔고 다시 실행(README의 fio/dd 참조). PRP 리스트까지 보려면 bs=16k 권장.")
