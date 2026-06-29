# drgn 라이브 커널 디버깅 스크립트 — blk-mq / NVMe / PCIe / PRP

이 디렉토리는 **실행 중인 리눅스 커널**의 I/O 스택 자료구조를
[drgn](https://drgn.readthedocs.io/) 으로 실시간 관찰하기 위한 스크립트 모음이다.

대응 분석 문서: [[drgn-live-blkmq-nvme-pcie-prp]]
(`deep/analysis/kernel/drgn-live-blkmq-nvme-pcie-prp.md`)

## 검증 환경 (이 스크립트들이 맞춰진 대상)

| 항목 | 값 |
|------|-----|
| 커널 | `6.17.0-35-generic` (Ubuntu 24.04 HWE, x86_64) |
| 타입 소스 | BTF (`/sys/kernel/btf/vmlinux` + 모듈 BTF) / debuginfod 풀 DWARF |
| NVMe | `/dev/nvme0n1` (VirtualBox 에뮬, `ORCL-VBOX-NVME-VER12`) |
| PCIe 위치 | `0000:00:0e.0` |
| blk-mq HW 큐 | 1개 (hctx0), I/O 스케줄러 `mq-deadline` |
| IOMMU | **비활성** (`/sys/class/iommu/` 비어있음) → DMA 주소 = 물리 주소 |

> ⚠️ `struct nvme_iod` 는 6.15 전후 DMA API 재작업으로 레이아웃이 크게 바뀌었다.
> 이 스크립트들은 **6.17 레이아웃**(`iod->cmd`, `iod->descriptors[]`,
> `iod->flags`, `iod->total_len`)을 사용한다. 구버전 커널의
> `iod->first_dma` / `iod->sgt` / `iod->use_sgl` / `iod->list[]` 는 없다.

## 설치

```bash
# 권장: 격리된 venv (Ubuntu 24.04 PEP 668 정책 회피)
python3 -m venv ~/.venv-drgn
~/.venv-drgn/bin/pip install drgn        # manylinux wheel → 컴파일 불필요

# 또는 시스템 전역
pipx install drgn
# 또는
pip3 install --break-system-packages drgn
```

drgn 0.2.0 이상 권장 (BTF + 모듈 BTF 지원).

## 실행

라이브 커널 메모리(`/proc/kcore`)는 **root 권한**이 필요하다.
debuginfod URL을 자식 프로세스로 넘기려면 `-E` 로 환경변수를 보존한다.

```bash
DRGN=~/.venv-drgn/bin/drgn

sudo -E $DRGN deep/scripts/drgn/00_env_check.py       # 환경/타입소스 확인
sudo -E $DRGN deep/scripts/drgn/01_blkmq_inflight.py  # blk-mq in-flight 요청
sudo -E $DRGN deep/scripts/drgn/02_nvme_queues.py     # NVMe SQ/CQ 링
sudo -E $DRGN deep/scripts/drgn/03_pcie.py            # PCIe pci_dev / BAR
sudo -E $DRGN deep/scripts/drgn/04_prp_payload.py     # PRP 디스크립터 + 페이로드
```

### in-flight 요청을 "잡으려면" 부하를 깔아라

`01` / `04` 는 **그 순간 진행 중인 I/O** 만 보인다. 에뮬 NVMe는 완료가
매우 빠르므로, 다른 터미널에서 지속적인 I/O를 생성한 상태로 실행한다:

```bash
# 읽기 부하 (페이지 캐시 우회로 매 요청이 디바이스까지 내려가게 O_DIRECT)
sudo fio --name=load --filename=/dev/nvme0n1 --rw=randread \
         --bs=64k --iodepth=64 --direct=1 --ioengine=libaio \
         --runtime=60 --time_based --numjobs=4

# fio가 없으면 간단히:
sudo dd if=/dev/nvme0n1 of=/dev/null bs=1M iflag=direct &
```

> bs를 `sgl_threshold`(32768=32KB) **미만**으로 주면 드라이버가 PRP 경로를
> 타고, 이상이면 SGL 경로를 탈 수 있다. PRP 페이로드 관찰이 목적이면
> `--bs=16k` 같이 32KB 미만 + 다중 페이지(>4KB)를 쓰면 PRP 리스트까지 생긴다.

## 대화형 셸

```bash
sudo -E ~/.venv-drgn/bin/drgn          # REPL 진입 → prog, 헬퍼 자동 로드
>>> from drgn.helpers.linux.block import for_each_disk, disk_name
>>> [disk_name(d) for d in for_each_disk(prog)]
```
