#!/bin/bash
# usage: S=<scratch dir> run-mpf.sh A|B|C|BAD   (nvme-sriov-qemu-vs-kernel.md §11.4.2)
# 리셋 실측(§11.4.4): INITRD=irfs-rst.cpio.gz EXTRA="mpf_test=flr nvme_core.admin_timeout=10" run-mpf.sh A
#   (irfs-rst.cpio.gz는 init 자리에 mpf-reset-init.sh를 넣어 같은 방법으로 만든다. NSSR은 QPROP=",nssrs=on", QEMU trace는 QARGS="-trace pci_nvme_mmio_nssr")
# 준비: cp -a qemu-debug/irfs $S/irfs; cp mpf-init.sh $S/irfs/init
#       (cd $S/irfs && find . | cpio -o -H newc | gzip > $S/irfs-mpf.cpio.gz)
#       truncate -s 64M $S/a0.img $S/a1.img
S=${S:-$(dirname "$0")}; Q=/home/harison/company/qemu-debug
case "$1" in
A)  PF0="addr=0x0.0,ari-nextfn=64"; PF1="addr=0x8.0" ;;
B)  PF0="addr=0x0.0,ari-nextfn=1,sriov_vf_offset=2,sriov_vf_stride=2"; PF1="addr=0x0.1,sriov_vf_offset=2,sriov_vf_stride=2" ;;
BAD) PF0="addr=0x0.0,ari-nextfn=1"; PF1="addr=0x0.1" ;;
C)  PF0="addr=0x0.0,ari-nextfn=128"; PF1="addr=0x10.0"; NVF=64
    COMMON="subsys=subsys0,serial=mpf0,bus=dsp1,sriov_max_vfs=64,max_ioqpairs=160,sriov_vq_flexible=128,msix_qsize=96,sriov_vi_flexible=64" ;;
esac
: ${NVF:=2}
: ${COMMON:="subsys=subsys0,serial=mpf0,bus=dsp1,sriov_max_vfs=4,sriov_vq_flexible=8,sriov_vi_flexible=4"}
exec timeout 600 $Q/qemu-11.1.2/build/qemu-system-x86_64 -machine q35 -accel tcg -smp 4 -m 1280 \
  -kernel $Q/linux-7.2/arch/x86/boot/bzImage -initrd $S/${INITRD:-irfs-mpf.cpio.gz} \
  -append "console=ttyS0 nokaslr no_hash_pointers panic=-1 pci=realloc mpf_nvf=$NVF $EXTRA" \
  -device pcie-root-port,id=rp1,chassis=1,slot=1,hotplug=off \
  -device x3130-upstream,id=up1,bus=rp1 \
  -device xio3130-downstream,id=dsp1,bus=up1,chassis=2,slot=0,hotplug=off \
  -device nvme-subsys,id=subsys0 \
  -drive file=$S/a0.img,if=none,id=d0,format=raw \
  -drive file=$S/a1.img,if=none,id=d1,format=raw \
  -device nvme,id=pf0,$COMMON$QPROP,$PF0 \
  -device nvme-ns,drive=d0,bus=pf0,nsid=1 \
  -device nvme,id=pf1,$COMMON$QPROP,$PF1 \
  -device nvme-ns,drive=d1,bus=pf1,nsid=2 \
  -nographic -no-reboot $QARGS
