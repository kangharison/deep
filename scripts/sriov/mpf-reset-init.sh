#!/bin/sh
# 게스트 init: multi-PF SR-IOV에서 "모든 PF에 걸치는 리셋"의 영향 실측
# (nvme-sriov-qemu-vs-kernel.md §11.4.4). run-mpf.sh A 구성 + 커널 인자 mpf_test=<시험>.
#   pfbus   : PF0 reset_method=bus → reset (커널이 거부하는지)
#   sbr     : DSP reset_subordinate (커널이 저장/복원하는 secondary bus reset)
#   sbrraw  : DSP Bridge Control SBR 비트를 config 직접 쓰기 (커널 모름)
#   nssr    : nvme subsystem-reset /dev/<PF0>
#   flr     : PF0 reset_method=flr → reset
/bin/busybox mount -t proc none /proc
/bin/busybox mount -t sysfs none /sys
/bin/busybox mount -t devtmpfs none /dev
export PATH=/bin:/sbin
B=/bin/busybox
P=/sys/bus/pci/devices
NVF=$($B sed -n "s/.*mpf_nvf=\([0-9]*\).*/\1/p" /proc/cmdline); [ -n "$NVF" ] || NVF=2
T=$($B sed -n "s/.*mpf_test=\([a-z]*\).*/\1/p" /proc/cmdline)
cfgw() { $B od -A n -t x2 -j $(($2)) -N 2 $P/$1/config | $B tr -d ' '; }
cfgb() { $B od -A n -t x1 -j $(($2)) -N 1 $P/$1/config | $B tr -d ' '; }
ctrl_of() { for c in /sys/class/nvme/nvme*; do [ "$($B basename $($B readlink -f $c/device))" = "$1" ] && $B basename $c; done; }
# 표준 capability 사슬에서 MSI-X(0x11) 위치 (함수가 사라져 0xff가 읽히면 중단)
msix_of() { p=0x$(cfgb $1 0x34); n=0; while [ $((p)) -ne 0 ] && [ $((p)) -ne 255 ] && [ $n -lt 16 ]; do n=$((n+1)); [ "$(cfgb $1 $p)" = "11" ] && { echo $p; return; }; p=0x$(cfgb $1 $((p+1))); done; }
sec() { $B timeout 20 nvme list-secondary /dev/$1 2>&1 | $B awk '/SCID/{s=$NF}/SCS/{printf "%s:%s ",s,$NF}/VFN/{printf "vfn%s ",$NF}/NVQ/{printf "q%s ",$NF}/NVI/{printf "i%s | ",$NF}/rror|failed|nvalid|denied/{print}'; echo; }
snap() {
  echo "=== snap: $1 ==="
  for c in /sys/class/nvme/nvme*; do echo " $($B basename $c) $($B basename $($B readlink -f $c/device)) cntlid=$($B cat $c/cntlid 2>/dev/null) state=$($B cat $c/state)"; done
  for pf in $PFS; do
    echo " PF $pf: kernel numvfs=$($B cat $P/$pf/sriov_numvfs) | dev IOV ctrl(0x128)=$(cfgw $pf 0x128) NumVFs(0x130)=$(cfgw $pf 0x130) cmd=$(cfgw $pf 4)"
    for v in $P/$pf/virtfn*; do [ -e $v ] || continue; vf=$($B basename $($B readlink -f $v)); m=$(msix_of $vf)
      echo "   VF $vf: devid=$(cfgw $vf 2) cmd=$(cfgw $vf 4) msix@$m ctrl=$([ -n "$m" ] && cfgw $vf $((m+2))) drv=$($B basename $($B readlink -f $P/$vf/driver 2>/dev/null) 2>/dev/null)"; done
  done
}
secs() { for pf in $PFS; do c=$(ctrl_of $pf); echo " secondary $pf ($c): $( [ -n "$c" ] && sec $c)"; done; }
probe() {
  echo "=== probe: admin Identify on every controller (admin_timeout from cmdline) ==="
  for c in /sys/class/nvme/nvme*; do n=$($B basename $c)
    r=$($B timeout 40 nvme id-ctrl /dev/$n 2>&1 | $B grep -E "^cntlid|rror|nterrupt|failed" | $B head -1)
    echo " $n: rc=$? ${r:-<no output>} -> state=$($B cat $c/state 2>/dev/null)"; done
}
echo "##### MPF-RESET test=$T #####"
PFS=$(for d in $P/*; do [ -e $d/sriov_totalvfs ] && $B basename $d; done)
PF0=$($B echo $PFS | $B awk '{print $1}'); C0=$(ctrl_of $PF0)
DSP=$($B basename $($B dirname $($B readlink -f $P/$PF0)))
echo "PFS: $PFS  PF0=$PF0($C0)  DSP=$DSP  PF0 reset_method: $($B cat $P/$PF0/reset_method)"
for pf in $PFS; do
  c=$(ctrl_of $pf)
  echo 0 > $P/$pf/sriov_drivers_autoprobe
  echo $NVF > $P/$pf/sriov_numvfs
  for s in $(nvme list-secondary /dev/$c 2>/dev/null | $B grep -i "SCID" | $B awk '{print $NF}' | $B head -2); do
    nvme virt-mgmt /dev/$c -c $s -r 0 -a 8 -n 2 >/dev/null; nvme virt-mgmt /dev/$c -c $s -r 1 -a 8 -n 1 >/dev/null
    nvme virt-mgmt /dev/$c -c $s -a 9 >/dev/null
  done
  for v in $P/$pf/virtfn0 $P/$pf/virtfn1; do vf=$($B basename $($B readlink -f $v)); echo nvme > $P/$vf/driver_override; echo $vf > /sys/bus/pci/drivers_probe; done
done
$B sleep 3
snap before; secs
$B dmesg -c > /dev/null
case "$T" in
pfbus)
  echo "##### PF0 reset_method=bus #####"
  echo bus > $P/$PF0/reset_method; echo " write bus -> rc=$? now=$($B cat $P/$PF0/reset_method 2>&1)"
  echo 1 > $P/$PF0/reset; echo " reset rc=$?" ;;
sbr)
  echo "##### DSP $DSP reset_subordinate #####"
  echo 1 > $P/$DSP/reset_subordinate; echo " rc=$?" ;;
sbrraw)
  bc=$(cfgw $DSP 0x3e); echo "##### DSP $DSP Bridge Control=0x$bc, raw SBR (bit6) #####"
  v=$((0x$bc | 0x40)); $B printf "\\$($B printf %o $((v & 0xff)))\\$($B printf %o $((v >> 8)))" | $B dd of=$P/$DSP/config bs=1 seek=62 count=2 conv=notrunc 2>/dev/null
  echo " set -> 0x$(cfgw $DSP 0x3e)"; $B sleep 1
  v=$((0x$bc)); $B printf "\\$($B printf %o $((v & 0xff)))\\$($B printf %o $((v >> 8)))" | $B dd of=$P/$DSP/config bs=1 seek=62 count=2 conv=notrunc 2>/dev/null
  echo " clear -> 0x$(cfgw $DSP 0x3e)" ;;
nssr)
  echo "##### nvme subsystem-reset /dev/$C0 #####"
  nvme subsystem-reset /dev/$C0; echo " rc=$?" ;;
flr)
  echo "##### PF0 reset_method=flr #####"
  echo flr > $P/$PF0/reset_method; echo " now=$($B cat $P/$PF0/reset_method)"
  echo 1 > $P/$PF0/reset; echo " reset rc=$?" ;;
esac
$B sleep 5
snap "after +5s"; secs
echo "--- dmesg (after event) ---"; $B dmesg -c | $B grep -vE "^\s*$" | $B tail -40
probe
$B sleep 15
snap "after probe +15s"; secs
echo "--- dmesg (probe/recovery) ---"; $B dmesg -c | $B tail -60
echo "##### MPF-RESET end #####"
$B poweroff -f
