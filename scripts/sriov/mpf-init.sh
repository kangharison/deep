#!/bin/sh
/bin/busybox mount -t proc none /proc
/bin/busybox mount -t sysfs none /sys
/bin/busybox mount -t devtmpfs none /dev
export PATH=/bin:/sbin
B=/bin/busybox
P=/sys/bus/pci/devices
NVF=$($B sed -n "s/.*mpf_nvf=\([0-9]*\).*/\1/p" /proc/cmdline); [ -n "$NVF" ] || NVF=2
cfgw() { $B od -A n -t x2 -j $(($2)) -N 2 $P/$1/config | $B tr -d ' '; }
cfgl() { $B od -A n -t x4 -j $(($2)) -N 4 $P/$1/config | $B tr -d ' '; }
ctrl_of() { for c in /sys/class/nvme/nvme*; do [ "$($B basename $($B readlink -f $c/device))" = "$1" ] && $B basename $c; done; }
sec() { nvme list-secondary /dev/$1 2>&1 | $B grep -E "SCID|SCS|VFN|Entry" | $B tr -s ' \n' ' '; echo; }
echo "##### MPF: devices #####"
for d in $P/*; do echo "$($B basename $d) class=$($B cat $d/class) id=$($B cat $d/vendor):$($B cat $d/device)"; done
PFS=$(for d in $P/*; do [ -e $d/sriov_totalvfs ] && $B basename $d; done)
echo "PFS: $PFS"
for pf in $PFS; do
  echo "--- PF $pf ($(ctrl_of $pf)) ---"
  echo " totalvfs=$($B cat $P/$pf/sriov_totalvfs) offset=$($B cat $P/$pf/sriov_offset) stride=$($B cat $P/$pf/sriov_stride) vf_device=$($B cat $P/$pf/sriov_vf_device)"
  echo " ARI cap@0x100: hdr=$(cfgl $pf 0x100) cap=$(cfgw $pf 0x104) (NFN=bits15:8)"
  echo " resource line8 (IOV BAR0): $($B sed -n 8p $P/$pf/resource)"
done
DSP=$($B basename $($B dirname $($B readlink -f $P/$($B echo $PFS | $B awk '{print $1}'))))
echo "--- DSP $DSP windows (resource lines 14-16) ---"; $B sed -n 14,16p $P/$DSP/resource
echo "--- /proc/iomem (PCI part) ---"; $B grep -A40 "PCI Bus 0000:00" /proc/iomem | $B grep -vE "System RAM|Reserved|Kernel" | $B head -50
for pf in $PFS; do
  c=$(ctrl_of $pf)
  echo "##### $pf ($c): enable $NVF VFs (bind first 2) + virt-mgmt #####"
  nvme id-ctrl /dev/$c | $B grep -E "^cntlid|^sn "
  echo 0 > $P/$pf/sriov_drivers_autoprobe
  echo $NVF > $P/$pf/sriov_numvfs; echo " numvfs=$($B cat $P/$pf/sriov_numvfs)"
  for s in $(nvme list-secondary /dev/$c 2>/dev/null | $B grep -i "SCID" | $B awk '{print $NF}' | $B head -2); do
    nvme virt-mgmt /dev/$c -c $s -r 0 -a 8 -n 2 >/dev/null; nvme virt-mgmt /dev/$c -c $s -r 1 -a 8 -n 1 >/dev/null
    nvme virt-mgmt /dev/$c -c $s -a 9 | $B head -1
  done
  for v in $P/$pf/virtfn0 $P/$pf/virtfn1; do vf=$($B basename $($B readlink -f $v)); echo " $($B basename $v) -> $vf"; echo nvme > $P/$vf/driver_override; echo $vf > /sys/bus/pci/drivers_probe; done
done
$B sleep 3
echo "##### controllers #####"
for c in /sys/class/nvme/nvme*; do echo "$($B basename $c) $($B basename $($B readlink -f $c/device)) cntlid=$($B cat $c/cntlid) state=$($B cat $c/state)"; done
ls /dev/nvme*n* 2>/dev/null | $B tr '\n' ' '; echo
echo "--- resource line8 after enable ---"; for pf in $PFS; do echo "$pf: $($B sed -n 8p $P/$pf/resource)"; done
for pf in $PFS; do echo "secondary $pf: $(sec $(ctrl_of $pf))"; done
PF0=$($B echo $PFS | $B awk '{print $1}'); C0=$(ctrl_of $PF0)
echo "##### nvme reset /dev/$C0 (PF0 $PF0) #####"
nvme reset /dev/$C0; $B sleep 3
for pf in $PFS; do echo "secondary $pf: $(sec $(ctrl_of $pf))"; done
for c in /sys/class/nvme/nvme*; do echo "$($B basename $c) $($B basename $($B readlink -f $c/device)) state=$($B cat $c/state)"; done
dmesg | $B grep -iE "nvme|iov|sr-iov" | $B tail -25
echo "##### MPF end #####"
$B poweroff -f
