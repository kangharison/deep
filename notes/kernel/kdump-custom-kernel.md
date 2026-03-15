# Custom Kernel에서 kdump 설정 가이드

CentOS 9에서 커스텀 빌드 커널(NVMe 드라이버 수정 등)을 사용할 때, kdump가 동작하지 않아 `/var/crash/`에 vmcore가 생성되지 않는 문제를 해결하는 방법을 정리한다.

## 문제 상황

- 기본(stock) 커널: kdump 정상 동작, `/var/crash/`에 vmcore 생성됨
- 커스텀 빌드 커널: kernel crash 발생 시 dump 파일이 생성되지 않음
- NVMe 드라이버에서 invalid address 접근 등으로 crash 발생 시 분석 불가

## kdump 동작 원리

kdump가 어떻게 동작하는지 이해해야 왜 커스텀 커널에서 실패하는지 파악할 수 있다.

### 전체 흐름

```
 정상 운영 중                     Crash 발생 시
┌─────────────────┐             ┌──────────────────────────────────────────┐
│  Production     │  panic!     │  1. kexec가 예약된 메모리로 점프         │
│  Kernel         │ ─────────►  │  2. 예약 메모리에 미리 로드된            │
│  (1st kernel)   │             │     kdump 전용 커널(2nd kernel)이 부팅   │
│                 │             │  3. 2nd kernel이 /proc/vmcore로          │
│  ┌────────────┐ │             │     1st kernel 메모리를 노출             │
│  │ crashkernel│ │             │  4. makedumpfile이 vmcore를              │
│  │ 예약 영역  │ │             │     /var/crash/에 저장                   │
│  │ (256M)     │ │             │  5. 시스템 재부팅                        │
│  └────────────┘ │             └──────────────────────────────────────────┘
└─────────────────┘
```

### 핵심 구성 요소

| 구성 요소 | 역할 | 파일 위치 |
|-----------|------|-----------|
| **kexec** | crash 발생 시 2nd kernel로 점프하는 시스템콜 | 커널 내장 (`CONFIG_KEXEC`) |
| **crashkernel 메모리** | 2nd kernel이 사용할 예약 메모리 영역 | 부트 파라미터로 지정 |
| **kdump initramfs** | 2nd kernel 부팅 시 사용할 초기 램디스크. vmcore 저장 로직 포함 | `/boot/initramfs-<ver>kdump.img` |
| **makedumpfile** | vmcore를 필터링/압축하여 디스크에 저장하는 도구 | kdump initramfs 내부 |
| **vmlinux** | 디버그 심볼이 포함된 ELF 바이너리. crash 분석에 사용 | 빌드 디렉토리 루트 |
| **kdump.conf** | dump 저장 위치, 필터링 옵션 등 설정 | `/etc/kdump.conf` |

### stock 커널에서는 왜 자동으로 되는가?

CentOS/RHEL의 stock 커널 RPM 패키지는 설치 시 다음을 자동으로 수행한다:

1. `CONFIG_KEXEC`, `CONFIG_CRASH_DUMP`, `CONFIG_RELOCATABLE` 등이 이미 활성화된 상태로 빌드됨
2. `kernel-core` RPM의 post-install 스크립트가 GRUB에 `crashkernel=auto` 파라미터를 자동 추가
3. `kdump` 패키지의 post-install 스크립트가 kdump initramfs를 자동 생성
4. `kdump.service`가 enable 상태로 설치됨

커스텀 빌드 커널은 이 자동화 과정을 거치지 않기 때문에 수동으로 모든 단계를 설정해야 한다.

## 1. 커널 빌드 설정 (CONFIG)

### 1.1 필수 옵션과 각각의 역할

```bash
# 현재 실행 중인 커널의 config 확인
cat /boot/config-$(uname -r) | grep -E "KEXEC|CRASH|DEBUG_INFO|RELOCATABLE"
```

#### kexec 관련

```
CONFIG_KEXEC=y
```

kexec 시스템콜을 커널에 포함시킨다. kexec는 현재 실행 중인 커널에서 BIOS/UEFI를 거치지 않고 직접 다른 커널로 점프하는 메커니즘이다. crash 발생 시 kdump 커널(2nd kernel)로 전환하는 데 사용된다. 이 옵션이 없으면 panic 발생 시 아무 일도 일어나지 않고 시스템이 그냥 멈추거나 재부팅된다.

```
CONFIG_KEXEC_FILE=y
```

`kexec_file_load()` 시스템콜을 활성화한다. 기존 `kexec_load()`는 유저스페이스에서 커널 이미지의 세그먼트를 직접 매핑해야 했지만, `kexec_file_load()`는 파일 디스크립터만 넘기면 커널이 알아서 파싱한다. CentOS 9의 kdumpctl은 기본적으로 `kexec_file_load()`를 사용하므로 이 옵션이 필요하다.

#### crash dump 관련

```
CONFIG_CRASH_DUMP=y
```

2nd kernel(kdump 커널)이 1st kernel의 메모리를 `/proc/vmcore`로 접근할 수 있게 한다. 이 옵션이 없으면 kdump 커널이 부팅되어도 1st kernel의 메모리 내용을 읽을 수 없어서 빈 vmcore가 생성되거나 아예 생성되지 않는다.

```
CONFIG_PROC_VMCORE=y
```

`/proc/vmcore` 파일시스템 인터페이스를 제공한다. kdump 커널이 부팅된 뒤 makedumpfile이 이 인터페이스를 통해 1st kernel의 메모리 덤프를 읽는다.

#### 재배치 관련

```
CONFIG_RELOCATABLE=y
```

**이것이 커스텀 커널에서 kdump가 실패하는 가장 흔한 원인이다.**

일반적으로 커널은 고정된 물리 주소(보통 0x1000000, 16MB)에 로드된다. kdump 커널은 `crashkernel=` 파라미터로 예약된 메모리 영역에 로드되어야 하는데, 이 주소는 고정 주소와 다르다. `CONFIG_RELOCATABLE=y`가 설정되면 커널이 어떤 물리 주소에 로드되든 자신의 위치를 기준으로 상대 주소를 계산하여 정상 동작한다.

이 옵션이 `n`이면:
- 1st kernel은 정상 부팅됨 (기본 주소에 로드되므로)
- kdump 커널은 예약 영역의 다른 주소에 로드되므로 부팅 실패
- 결과적으로 crash 발생 시 아무 dump도 생성되지 않음

```
CONFIG_RANDOMIZE_BASE=y  # (선택, KASLR)
```

KASLR(Kernel Address Space Layout Randomization)을 활성화한다. `CONFIG_RELOCATABLE=y`가 전제 조건이다. 보안을 위해 활성화하는 것이 좋지만 kdump 자체의 필수 조건은 아니다.

### 1.2 디버그 심볼 설정

crash 유틸리티로 vmcore를 분석하려면 디버그 심볼이 포함된 `vmlinux`가 필요하다.

```
CONFIG_DEBUG_INFO=y
```

컴파일 시 `-g` 플래그를 추가하여 DWARF 디버그 정보를 ELF 바이너리(vmlinux)에 포함시킨다. 이 정보에는 함수명, 변수명, 구조체 레이아웃, 소스 파일/라인 정보가 들어있다. 이 옵션이 없으면 crash에서 `bt`(backtrace)를 해도 함수 이름 대신 메모리 주소만 보인다.

```
CONFIG_DEBUG_INFO_DWARF5=y
```

DWARF 포맷 버전 5를 사용한다. DWARF4 대비 더 작은 디버그 정보와 더 빠른 파싱이 가능하다. CentOS 9의 crash 유틸리티는 DWARF5를 지원한다. DWARF4도 사용 가능하지만 5를 권장한다.

```
CONFIG_DEBUG_INFO_REDUCED=n
```

`y`로 설정하면 빌드 속도를 위해 디버그 정보를 축소한다. 구조체 멤버 정보 등이 빠져서 crash에서 `struct` 명령어가 제대로 동작하지 않게 된다. 반드시 `n`으로 설정해야 한다.

```
CONFIG_DEBUG_INFO_COMPRESSED=n
```

`y`로 설정하면 `.debug_*` 섹션을 zlib으로 압축한다. 디스크 공간은 절약되지만 crash 유틸리티 버전에 따라 압축된 디버그 정보를 읽지 못할 수 있다. 안전하게 `n`으로 설정한다.

### 1.3 stock 커널 config를 기반으로 시작하기

처음부터 `.config`를 만드는 대신 stock 커널의 config를 기반으로 시작하면 kdump 관련 옵션이 이미 설정되어 있다:

```bash
# stock 커널 config를 복사해서 시작
cp /boot/config-$(uname -r) /path/to/linux-source/.config

# 커스텀 옵션만 수정
make menuconfig
# 또는 직접 편집 후
make olddefconfig    # 새 옵션은 기본값으로 채움
```

이렇게 하면 kdump 관련 설정을 놓칠 확률이 크게 줄어든다.

## 2. crashkernel 메모리 예약

### 2.1 왜 메모리를 미리 예약해야 하는가

crash가 발생하면 1st kernel의 메모리는 오염된 상태이다. kdump 커널이 이 메모리를 사용하면 연쇄적으로 crash가 발생할 수 있다. 따라서 부팅 시점에 미리 "이 영역은 1st kernel이 절대 사용하지 마라"고 예약해둔 깨끗한 메모리 영역에 kdump 커널이 로드되어야 한다.

### 2.2 설정 방법

```bash
# 현재 상태 확인
cat /proc/cmdline | grep crashkernel
```

출력이 없으면 GRUB에 파라미터를 추가해야 한다:

```bash
# /etc/default/grub 편집
# GRUB_CMDLINE_LINUX 라인에 crashkernel 추가
GRUB_CMDLINE_LINUX="... crashkernel=256M ..."
```

#### crashkernel 값 선택 기준

| 시스템 메모리 | 권장 값 | 비고 |
|--------------|--------|------|
| 4GB 이하 | `crashkernel=128M` | 최소 사양 |
| 4GB ~ 64GB | `crashkernel=256M` | 일반적인 서버 |
| 64GB 이상 | `crashkernel=512M` | 대형 서버, 많은 드라이버 로드 시 |
| 자동 | `crashkernel=auto` | CentOS/RHEL이 메모리에 따라 자동 결정 |

`auto` 옵션은 CentOS/RHEL에서 `/usr/lib/modules/<ver>/kdump-crashkernel.conf` 파일을 참조하여 자동으로 크기를 결정한다. 커스텀 커널에서는 이 파일이 없을 수 있으므로 명시적으로 `256M` 등을 지정하는 것이 안전하다.

```bash
# GRUB 설정 재생성
# BIOS 시스템
grub2-mkconfig -o /boot/grub2/grub.cfg
# UEFI 시스템
grub2-mkconfig -o /boot/efi/EFI/centos/grub.cfg

# 재부팅
reboot
```

### 2.3 예약 확인

재부팅 후 반드시 확인한다:

```bash
# 방법 1: /proc/iomem에서 Crash kernel 영역 확인
cat /proc/iomem | grep -i crash
# 출력 예: 2a000000-39ffffff : Crash kernel
# → 0x2a000000 ~ 0x39ffffff = 256MB 예약됨

# 방법 2: dmesg에서 확인
dmesg | grep -i crash
# 출력 예: Reserving 256MB of memory at 672MB for crashkernel

# 방법 3: cmdline에서 확인
cat /proc/cmdline | grep crashkernel
```

`/proc/iomem`에 "Crash kernel" 항목이 보이지 않으면 예약에 실패한 것이다. 이 경우:
- `crashkernel=` 파라미터가 GRUB에 제대로 반영되었는지 확인
- 메모리가 부족하여 예약에 실패했는지 `dmesg`에서 확인
- `crashkernel=256M,high crashkernel=128M,low` 형태로 high/low 분리 시도

## 3. kdump initramfs 재생성

### 3.1 initramfs란 무엇인가

initramfs(Initial RAM Filesystem)는 커널이 부팅 초기에 사용하는 임시 루트 파일시스템이다. 실제 루트 파일시스템을 마운트하기 전에 필요한 드라이버, 도구, 스크립트를 포함한다.

### 3.2 일반 initramfs vs kdump initramfs

시스템에는 두 종류의 initramfs가 존재한다:

```
/boot/
├── initramfs-6.x.y-custom.img         ← 일반 부팅용 initramfs
└── initramfs-6.x.y-custom-kdump.img   ← kdump 전용 initramfs
```

| 구분 | 일반 initramfs | kdump initramfs |
|------|---------------|-----------------|
| 용도 | 정상 부팅 시 루트 파일시스템 마운트 | crash 발생 후 vmcore 저장 |
| 포함 드라이버 | 스토리지, 네트워크, 파일시스템 등 전체 | vmcore 저장에 필요한 최소 드라이버만 |
| 포함 도구 | systemd, udev, mount 등 | makedumpfile, dracut-kdump.sh 등 |
| 크기 | 수십 MB | 일반 initramfs보다 작음 |
| 생성 방법 | `dracut` 또는 `make install` 시 자동 | `kdumpctl rebuild` 또는 `dracut --force` |

### 3.3 왜 kdump initramfs를 재생성해야 하는가

kdump initramfs가 커스텀 커널에 맞게 재생성되어야 하는 이유는 다음과 같다:

**1) 커널 모듈 버전 불일치**

kdump initramfs 안에는 2nd kernel이 부팅할 때 로드할 커널 모듈(.ko 파일)이 포함되어 있다. 이 모듈들은 커널 버전에 정확히 맞아야 한다. stock 커널용 kdump initramfs에는 stock 커널 버전의 모듈이 들어있으므로, 커스텀 커널에서는 `modprobe`가 실패하여 2nd kernel이 정상 부팅되지 않는다.

```bash
# 예: stock 커널 모듈 vs 커스텀 커널 모듈
/lib/modules/5.14.0-362.el9.x86_64/    ← stock
/lib/modules/6.8.0-custom/             ← 커스텀 빌드

# kdump initramfs 안에 들어가는 모듈은 현재 커널 버전의 것이어야 함
```

**2) 스토리지 드라이버 포함**

vmcore를 디스크에 저장하려면 kdump initramfs 안에 해당 스토리지 컨트롤러의 드라이버가 포함되어야 한다. NVMe SSD를 사용하는 경우 `nvme.ko`, `nvme-core.ko` 등이 필요하다. 커스텀 커널에서 NVMe 드라이버를 수정했다면, 수정된 버전의 모듈이 kdump initramfs에 들어가야 한다.

**3) makedumpfile의 커널 의존성**

makedumpfile은 vmcore를 생성할 때 커널의 `vmcoreinfo`를 참조한다. 이 정보에는 커널 페이지 테이블 구조, 물리/가상 메모리 매핑 정보 등이 포함되어 있어서 커널 버전과 정확히 일치해야 한다.

### 3.4 재생성 방법

```bash
# 방법 1: kdumpctl 사용 (권장)
kdumpctl rebuild

# 방법 2: dracut 직접 사용
dracut -f /boot/initramfs-$(uname -r)kdump.img $(uname -r) \
  --omit-drivers "bnxt_en mlx5_core" \
  -d "nvme nvme-core ahci sd_mod" \
  --add kdump

# 생성 확인
ls -la /boot/initramfs-*kdump.img
```

`kdumpctl rebuild`는 내부적으로 dracut을 호출하되 `/etc/kdump.conf`의 설정을 반영하여 적절한 옵션으로 kdump initramfs를 생성한다.

### 3.5 kdump initramfs 내용 검증

```bash
# kdump initramfs 안에 포함된 파일 목록 확인
lsinitrd /boot/initramfs-$(uname -r)kdump.img | head -50

# NVMe 드라이버가 포함되어 있는지 확인
lsinitrd /boot/initramfs-$(uname -r)kdump.img | grep nvme
# 출력 예:
# -rw-r--r--  1 root root  123456 ... usr/lib/modules/.../nvme.ko
# -rw-r--r--  1 root root   98765 ... usr/lib/modules/.../nvme-core.ko

# makedumpfile이 포함되어 있는지 확인
lsinitrd /boot/initramfs-$(uname -r)kdump.img | grep makedumpfile
```

## 4. kdump 서비스 설정

### 4.1 kexec 로드 과정

`systemctl start kdump`가 하는 일을 단계별로 보면:

```
systemctl start kdump
    │
    ▼
kdumpctl start
    │
    ├─ 1. /proc/iomem에서 crashkernel 예약 영역 확인
    │
    ├─ 2. kdump initramfs 존재 여부 확인
    │     없으면 자동 rebuild 시도
    │
    ├─ 3. kexec -p 명령으로 kdump 커널을 예약 메모리에 로드
    │     kexec -p /boot/vmlinuz-<ver> \
    │       --initrd=/boot/initramfs-<ver>kdump.img \
    │       --append="root=... irqpoll maxcpus=1 ..."
    │
    └─ 4. 로드 성공 시 "Kdump is operational" 출력
```

`kexec -p`의 `-p`는 panic을 의미한다. crash(panic) 발생 시에만 로드된 커널로 전환한다.

### 4.2 /etc/kdump.conf 설정

```bash
cat /etc/kdump.conf
```

주요 설정 항목:

```bash
# vmcore 저장 위치 (기본: 로컬 파일시스템)
path /var/crash

# dump 레벨 (어떤 페이지를 저장할지)
# 31 = 불필요한 페이지(zero, cache 등) 제외 → 파일 크기 크게 감소
core_collector makedumpfile -l --message-level 7 -d 31

# 덤프 실패 시 동작
failure_action reboot

# 덤프 성공 후 동작
final_action reboot

# NFS/SSH로 원격 저장도 가능
# nfs my.server:/export/crash
# ssh user@my.server
```

### 4.3 서비스 활성화 및 확인

```bash
# kdump 서비스 enable (부팅 시 자동 시작)
systemctl enable kdump

# 서비스 시작
systemctl restart kdump

# 상태 확인
kdumpctl status
# 출력: "Kdump is operational" ← 이것이 나와야 정상
```

## 5. 커스텀 커널 설치 전체 절차 (kdump 포함)

NVMe 드라이버를 수정한 커스텀 커널을 설치하고 kdump까지 정상 동작하게 하는 전체 과정이다:

```bash
# ── 1단계: 커널 빌드 ──
cd /path/to/linux-source

# stock config 기반으로 시작 (kdump 옵션이 이미 포함됨)
cp /boot/config-$(uname -r) .config
make olddefconfig

# 필수 옵션 확인
grep -E "^CONFIG_(KEXEC|CRASH_DUMP|RELOCATABLE|DEBUG_INFO)=" .config
# CONFIG_KEXEC=y
# CONFIG_KEXEC_FILE=y
# CONFIG_CRASH_DUMP=y
# CONFIG_RELOCATABLE=y
# CONFIG_DEBUG_INFO=y

# 빌드
make -j$(nproc)

# ── 2단계: 설치 ──
sudo make modules_install     # /lib/modules/<ver>에 모듈 설치
sudo make install              # /boot에 vmlinuz, System.map, config 설치

# vmlinux 복사 (crash 분석에 필요한 디버그 심볼 포함 바이너리)
sudo cp vmlinux /boot/vmlinux-$(make kernelrelease)

# ── 3단계: GRUB에 crashkernel 파라미터 확인 ──
grep crashkernel /boot/grub2/grub.cfg
# 없으면 /etc/default/grub에 추가 후 grub2-mkconfig

# ── 4단계: kdump initramfs 생성 ──
# 새로 설치한 커널로 부팅한 후 실행해야 함
# (make install이 자동으로 새 커널을 기본 부팅으로 설정함)
sudo reboot

# 재부팅 후 커스텀 커널로 부팅되었는지 확인
uname -r    # 6.8.0-custom 등

# ── 5단계: kdump 설정 ──
# crashkernel 예약 확인
cat /proc/iomem | grep -i crash
# 출력이 없으면 GRUB 파라미터 문제 → 2.2절 참조

# kdump initramfs 재생성
sudo kdumpctl rebuild

# kdump 서비스 시작
sudo systemctl enable kdump
sudo systemctl restart kdump

# 상태 확인
kdumpctl status
# "Kdump is operational" 확인
```

## 6. kdump 동작 테스트

설정이 완료되면 의도적으로 crash를 발생시켜서 dump가 정상 생성되는지 테스트한다.

### 6.1 sysrq를 이용한 수동 crash

```bash
# Magic SysRq 활성화 확인
cat /proc/sys/kernel/sysrq
# 0이면: echo 1 > /proc/sys/kernel/sysrq

# 강제 crash 트리거 (주의: 시스템이 즉시 crash됨)
echo c > /proc/sysrq-trigger
```

이 명령은 커널에서 의도적으로 NULL 포인터 역참조를 발생시켜 panic을 유발한다.

### 6.2 crash 후 확인

시스템이 재부팅된 후:

```bash
# dump 파일 확인
ls -la /var/crash/
# 출력 예:
# drwxr-xr-x. 2 root root 4096 Mar 15 10:30 127.0.0.1-2026-03-15-10:30:00/

ls -la /var/crash/127.0.0.1-2026-03-15-10:30:00/
# -rw-------. 1 root root 268435456 Mar 15 10:30 vmcore
# -rw-r--r--. 1 root root     12345 Mar 15 10:30 vmcore-dmesg.txt
```

`vmcore` 파일과 `vmcore-dmesg.txt` 파일이 생성되었으면 kdump가 정상 동작한 것이다.

### 6.3 테스트 실패 시

dump 파일이 생성되지 않았다면:

```bash
# kdump 서비스 상태 다시 확인
kdumpctl status

# kdump 관련 로그 확인 (crash 이전 부팅의 로그)
journalctl -u kdump -b -1 --no-pager

# dmesg에서 kexec/crash 관련 메시지 확인
journalctl -b -1 | grep -iE "kexec|crash|kdump|panic"
```

## 7. crash 유틸리티로 vmcore 분석

### 7.1 crash 실행

```bash
# crash 유틸리티 설치 (없는 경우)
sudo dnf install crash

# vmcore 분석 시작
# vmlinux: 빌드 디렉토리의 디버그 심볼 포함 바이너리
# vmcore: kdump가 생성한 메모리 덤프
crash /boot/vmlinux-$(uname -r) /var/crash/<timestamp>/vmcore
```

### 7.2 기본 분석 명령어

```
crash> sys              # 시스템 정보 (커널 버전, uptime 등)
crash> bt               # 현재 태스크의 backtrace (crash 지점)
crash> bt -a            # 모든 CPU의 backtrace
crash> log              # 커널 링 버퍼 (dmesg) 출력
crash> mod              # 로드된 모듈 목록
crash> ps               # 프로세스 목록
crash> vm               # 가상 메모리 정보
crash> files            # 열린 파일 목록
crash> mount            # 마운트된 파일시스템
crash> irq              # 인터럽트 정보
```

### 7.3 NVMe 드라이버 crash 분석 예시

NVMe 드라이버에서 invalid address 접근으로 crash가 발생한 경우:

```
# 1) backtrace로 crash 지점 확인
crash> bt
PID: 1234  TASK: ffff9a3e0c5d0000  CPU: 3   COMMAND: "nvme_wq"
 #0 [ffffa5c40f3e3a00] machine_kexec at ffffffff81069370
 #1 [ffffa5c40f3e3a58] __crash_kexec at ffffffff81135bac
 #2 [ffffa5c40f3e3b20] crash_kexec at ffffffff81135c9a
 #3 [ffffa5c40f3e3b38] oops_end at ffffffff8102493d
 #4 [ffffa5c40f3e3b58] page_fault_oops at ffffffff81074c15
 #5 [ffffa5c40f3e3bc0] do_page_fault at ffffffff81074ff0
 #6 [ffffa5c40f3e3bf0] async_page_fault at ffffffff81e01286
 #7 [ffffa5c40f3e3c50] nvme_submit_cmd at ffffffffc0a12345  ← crash 지점
 #8 [ffffa5c40f3e3c88] nvme_queue_rq at ffffffffc0a15678
 ...

# 2) crash 지점의 코드 디스어셈블
crash> dis nvme_submit_cmd
# 어떤 명령어에서 fault가 발생했는지 확인

# 3) NVMe 관련 모듈 확인
crash> mod | grep nvme
ffffffffc0a10000  nvme_core     102400  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0a30000  nvme           81920  (not loaded)  [CONFIG_KALLSYMS]

# 4) 특정 구조체 내용 확인
crash> struct nvme_dev <주소>
crash> struct nvme_queue <주소>

# 5) 레지스터 값 확인
crash> bt -f    # full backtrace with frame info

# 6) 특정 메모리 주소 읽기
crash> rd <주소> 16    # 16개 워드 읽기
crash> rd -x <주소> 8  # 16진수로 8개 워드 읽기
```

### 7.4 커스텀 NVMe 모듈의 심볼 로드

커스텀 NVMe 드라이버를 모듈(`.ko`)로 빌드한 경우, crash에서 해당 모듈의 디버그 심볼을 로드해야 함수명과 소스 라인 정보를 볼 수 있다:

```
# 모듈의 디버그 심볼 로드
crash> mod -s nvme /path/to/drivers/nvme/host/nvme.ko
crash> mod -s nvme_core /path/to/drivers/nvme/core/nvme-core.ko

# 로드 후 다시 backtrace
crash> bt
# 이제 nvme 함수들의 소스 파일:라인 정보가 출력됨
```

## 8. 상세 진단 체크리스트

순서대로 확인한다:

```bash
# ① 커널 config 필수 옵션 확인
zcat /proc/config.gz 2>/dev/null || cat /boot/config-$(uname -r) \
  | grep -E "^CONFIG_(KEXEC|KEXEC_FILE|CRASH_DUMP|RELOCATABLE|DEBUG_INFO|PROC_VMCORE)="

# ② crashkernel 부트 파라미터 확인
cat /proc/cmdline | grep crashkernel

# ③ crash kernel 메모리 실제 할당 확인
cat /proc/iomem | grep -i crash

# ④ kdump initramfs 존재 여부
ls -la /boot/initramfs-$(uname -r)kdump.img

# ⑤ kdump initramfs 안에 필요한 모듈 포함 여부
lsinitrd /boot/initramfs-$(uname -r)kdump.img | grep -E "nvme|makedump"

# ⑥ kdump 서비스 상태
kdumpctl status

# ⑦ kexec로 kdump 커널이 로드되었는지
cat /sys/kernel/kexec_crash_loaded
# 1 = 로드됨 (정상), 0 = 로드 안 됨 (문제)

# ⑧ kdump 서비스 로그
journalctl -u kdump -b --no-pager

# ⑨ vmlinux 파일 존재 여부 (crash 분석에 필요)
ls -la /boot/vmlinux-$(uname -r)
```

## 9. 흔한 실패 시나리오와 해결

| kdumpctl status 결과 | 원인 | 해결 |
|---------------------|------|------|
| "Kdump is not operational" | crashkernel 파라미터 누락 | GRUB에 `crashkernel=256M` 추가 후 재부팅 |
| "No memory reserved for crash kernel" | CONFIG_KEXEC 없음 또는 crashkernel 파라미터 없음 | `.config` 수정 후 재빌드, GRUB 파라미터 확인 |
| "Rebuilding initramfs failed" | dracut 모듈 누락 또는 커널 모듈 경로 문제 | `make modules_install` 재실행 후 `kdumpctl rebuild` |
| status는 ready인데 crash 시 dump 안 생김 | `CONFIG_RELOCATABLE=n`이면 kdump 커널 부팅 자체가 실패 | 반드시 `=y`로 재빌드 |
| vmcore는 생성되지만 crash에서 분석 불가 | `CONFIG_DEBUG_INFO=n` 또는 vmlinux 파일 없음 | 디버그 심볼 포함하여 재빌드, vmlinux를 `/boot`에 복사 |
| "kexec_file_load failed" | 커널 서명 검증 실패 (Secure Boot) | Secure Boot 비활성화 또는 커널 서명 |

## 10. 핵심 요약

커스텀 커널에서 kdump가 동작하지 않는 원인은 대부분 다음 세 가지 중 하나이다:

1. **`CONFIG_RELOCATABLE=y`가 빠져있음** → kdump 커널이 예약 메모리에서 부팅 불가. `.config` 수정 후 재빌드 필요
2. **커스텀 커널 설치 후 kdump initramfs를 재생성하지 않음** → 모듈 버전 불일치로 2nd kernel 부팅 실패. `kdumpctl rebuild` + `systemctl restart kdump` 실행 필요
3. **crashkernel 부트 파라미터가 GRUB에 없음** → 메모리 예약이 안 되어 kdump 커널을 로드할 수 없음. GRUB 설정 수정 후 재부팅 필요
