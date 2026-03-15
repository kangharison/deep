# Custom Kernel에서 kdump 설정 가이드

CentOS 9에서 커스텀 빌드 커널(NVMe 드라이버 수정 등)을 사용할 때, kdump가 동작하지 않아 `/var/crash/`에 vmcore가 생성되지 않는 문제를 해결하는 방법을 정리한다.

## 문제 상황

- 기본(stock) 커널: kdump 정상 동작, `/var/crash/`에 vmcore 생성됨
- 커스텀 빌드 커널: kernel crash 발생 시 dump 파일이 생성되지 않음
- NVMe 드라이버에서 invalid address 접근 등으로 crash 발생 시 분석 불가

## 1. 커널 빌드 설정 확인 (가장 흔한 원인)

커스텀 커널의 `.config`에 다음 옵션들이 반드시 활성화되어 있어야 한다:

```bash
# 현재 실행 중인 커널의 config 확인
cat /boot/config-$(uname -r) | grep -E "KEXEC|CRASH|DEBUG_INFO|RELOCATABLE"
```

### 필수 옵션

```
CONFIG_KEXEC=y
CONFIG_KEXEC_FILE=y
CONFIG_CRASH_DUMP=y
CONFIG_RELOCATABLE=y
CONFIG_DEBUG_INFO=y
CONFIG_PROC_VMCORE=y
```

특히 `CONFIG_RELOCATABLE=y`가 빠지면 kdump 커널이 예약된 메모리에서 부팅할 수 없어서 dump가 생성되지 않는다.

## 2. 디버그 심볼 포함 설정

crash 유틸리티로 분석하려면 디버그 심볼이 포함되어야 한다:

```
CONFIG_DEBUG_INFO=y
CONFIG_DEBUG_INFO_DWARF5=y      # 또는 DWARF4
CONFIG_DEBUG_INFO_REDUCED=n     # reduced면 crash 분석이 제한됨
CONFIG_DEBUG_INFO_COMPRESSED=n  # 압축되면 crash 유틸리티가 못 읽을 수 있음
```

## 3. 커스텀 커널 설치 후 kdump 재설정

커널을 설치한 뒤 kdump가 새 커널을 인식하도록 해야 한다:

```bash
# 1) 커널 빌드 & 설치
make -j$(nproc)
make modules_install
make install          # /boot에 vmlinuz, initramfs 설치됨

# 2) vmlinux (디버그 심볼 포함 바이너리) 복사 — crash 분석에 필요
cp vmlinux /boot/vmlinux-$(make kernelrelease)

# 3) kdump용 initramfs 재생성
kdumpctl rebuild

# 4) kdump 서비스 재시작
systemctl restart kdump

# 5) kdump 상태 확인 — "ready"가 나와야 정상
kdumpctl status
```

## 4. crashkernel 메모리 예약 확인

```bash
# GRUB에 crashkernel 파라미터가 있는지 확인
cat /proc/cmdline | grep crashkernel
```

출력에 `crashkernel=auto` 또는 `crashkernel=256M` 같은 값이 있어야 한다. 없으면:

```bash
# /etc/default/grub 수정
GRUB_CMDLINE_LINUX="... crashkernel=256M ..."

# GRUB 설정 재생성 (BIOS)
grub2-mkconfig -o /boot/grub2/grub.cfg
# UEFI인 경우
grub2-mkconfig -o /boot/efi/EFI/centos/grub.cfg

# 재부팅 후 확인
reboot
cat /proc/iomem | grep "Crash kernel"
# 출력 예: 2a000000-39ffffff : Crash kernel  ← 이게 보여야 정상
```

## 5. kdump 상세 진단

```bash
# 상세 로그로 실패 원인 확인
kdumpctl showstatus
journalctl -u kdump -b --no-pager

# kdump 서비스가 현재 커널에 대해 정상 로드되었는지
kdumpctl estimate    # 메모리 요구량 확인
```

## 6. 흔한 실패 시나리오와 해결

```
┌─────────────────────────────────────────────────────────────────┐
│  kdumpctl status 결과          │  원인 & 해결                   │
├─────────────────────────────────────────────────────────────────┤
│  "Kdump is not operational"   │  crashkernel 파라미터 누락      │
│                                │  → GRUB에 추가 후 재부팅       │
├─────────────────────────────────────────────────────────────────┤
│  "No memory reserved for      │  커널에 CONFIG_KEXEC 없음       │
│   crash kernel"               │  → .config 수정 후 재빌드      │
├─────────────────────────────────────────────────────────────────┤
│  "Rebuilding initramfs        │  kdump initramfs 생성 실패      │
│   failed"                     │  → dracut 모듈 확인            │
├─────────────────────────────────────────────────────────────────┤
│  status는 ready인데           │  CONFIG_RELOCATABLE=n이면       │
│  crash 시 dump 안 생김        │  kdump 커널 부팅 자체가 실패    │
│                                │  → 반드시 =y로 재빌드          │
└─────────────────────────────────────────────────────────────────┘
```

## 7. 빠른 체크리스트

순서대로 확인한다:

```bash
# ① 커널 config 필수 옵션
zcat /proc/config.gz 2>/dev/null || cat /boot/config-$(uname -r) \
  | grep -E "^CONFIG_(KEXEC|CRASH_DUMP|RELOCATABLE|DEBUG_INFO)="

# ② crashkernel 메모리 예약
cat /proc/cmdline | grep crashkernel

# ③ crash kernel 메모리 실제 할당
cat /proc/iomem | grep -i crash

# ④ kdump 서비스 상태
kdumpctl status

# ⑤ kdump initramfs 존재 여부
ls -la /boot/initramfs-*kdump.img
```

## 8. crash 유틸리티로 vmcore 분석

dump가 정상 생성되면 (`/var/crash/` 아래):

```bash
# vmlinux는 빌드 디렉토리의 것을 사용
crash /boot/vmlinux-$(uname -r) /var/crash/<timestamp>/vmcore
```

crash 프롬프트에서 유용한 명령어:

```
crash> bt          # backtrace — crash 발생 지점 확인
crash> log         # dmesg 출력
crash> mod         # loaded modules (커스텀 nvme 모듈 확인)
crash> dis <func>  # 함수 디스어셈블
crash> rd <addr>   # 메모리 주소 읽기
crash> struct <name> <addr>  # 구조체 내용 확인
```

### NVMe 드라이버 crash 분석 예시

```
crash> bt
#0  nvme_submit_cmd (...)
#1  nvme_queue_rq (...)
...

crash> mod | grep nvme
crash> dis nvme_submit_cmd
crash> struct nvme_command <addr>
```

## 핵심 요약

대부분의 경우 원인은 두 가지 중 하나이다:

1. **`CONFIG_RELOCATABLE=y`가 빠져있음** → `.config` 수정 후 커널 재빌드
2. **커스텀 커널 설치 후 `kdumpctl rebuild` + `systemctl restart kdump`를 안 함** → 실행 후 `kdumpctl status`로 확인
