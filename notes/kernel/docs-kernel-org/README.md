# docs.kernel.org 미러 (Linux Kernel Documentation)

> 출처: https://docs.kernel.org/ · 미러링 날짜: 2026-06-18
> 생성 방식: Sphinx 인벤토리(`objects.inv`)로 전체 페이지 열거 → curl 다운로드 → 본문(`div.body[role=main]`) 추출 → `markdownify` 로 HTML→Markdown 변환.
> 재현 스크립트: [`../../../scripts/km_crawl.py`](../../../scripts/km_crawl.py), 페이지 목록: [`../../../scripts/km_pages.txt`](../../../scripts/km_pages.txt)

- **총 3967 페이지** (영문 원문 자동 변환본; 정본은 각 파일 상단의 출처 URL)
- **디렉토리 구조 = docs.kernel.org URL 경로 = 챕터 구조** 그대로 보존
- 각 .md 상단에 원문 URL 헤더 부착. 재실행 시 기존 파일은 skip(재개 가능).
- 일부 페이지(`mm/swap` 등)는 원본이 제목만 있는 스텁이라 본문이 비어 있음(원문 충실).

## 챕터(상위 디렉토리)별 페이지 수

| 챕터 | 페이지 수 |
|------|-----------|
| `translations/` | 534 |
| `userspace-api/` | 458 |
| `admin-guide/` | 397 |
| `driver-api/` | 359 |
| `hwmon/` | 274 |
| `networking/` | 261 |
| `arch/` | 238 |
| `filesystems/` | 145 |
| `gpu/` | 102 |
| `core-api/` | 74 |
| `virt/` | 63 |
| `trace/` | 61 |
| `sound/` | 58 |
| `process/` | 56 |
| `dev-tools/` | 55 |
| `bpf/` | 53 |
| `mm/` | 47 |
| `i2c/` | 45 |
| `scsi/` | 42 |
| `input/` | 35 |
| `fb/` | 34 |
| `tools/` | 33 |
| `netlink/` | 32 |
| `power/` | 29 |
| `security/` | 28 |
| `crypto/` | 28 |
| `usb/` | 26 |
| `iio/` | 26 |
| `PCI/` | 25 |
| `firmware-guide/` | 24 |
| `leds/` | 23 |
| `scheduler/` | 19 |
| `RCU/` | 19 |
| `misc-devices/` | 19 |
| `w1/` | 18 |
| `locking/` | 18 |
| `block/` | 16 |
| `kbuild/` | 14 |
| `hid/` | 13 |
| `devicetree/` | 13 |
| `wmi/` | 12 |
| `watchdog/` | 10 |
| `staging/` | 9 |
| `livepatch/` | 9 |
| `accel/` | 9 |
| `maintainer/` | 8 |
| `infiniband/` | 8 |
| `timers/` | 7 |
| `spi/` | 7 |
| `doc-guide/` | 7 |
| `tee/` | 6 |
| `rust/` | 6 |
| `accounting/` | 6 |
| `pcmcia/` | 5 |
| `netlabel/` | 5 |
| `fault-injection/` | 5 |
| `target/` | 4 |
| `kernel-hacking/` | 4 |
| `edac/` | 4 |
| `cpu-freq/` | 4 |
| `nvme/` | 3 |
| `mhi/` | 3 |
| `peci/` | 2 |
| `fpga/` | 2 |
| `cdrom/` | 2 |
| `subsystem-apis.md/` | 1 |
| `search.md/` | 1 |
| `py-modindex.md/` | 1 |
| `nvdimm/` | 1 |
| `index.md/` | 1 |
| `genindex.md/` | 1 |

## 주의
- 자동 변환본이라 표/도표/각주 등 일부 서식은 원문과 다를 수 있다. 정확한 내용은 출처 URL 참조.
- `translations/` 는 비영어 번역본(중국어/일본어/한국어 등) 미러.

## 관련 노트
- [[nvme-driver]], [[nvme-namespace-sq-isolation-driver]] 등 본 저장소의 커널 분석 노트
