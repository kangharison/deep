# Example of udev rules

> 출처(원문): https://docs.kernel.org/admin-guide/aoe/examples.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Example of udev rules

> ```
> # These rules tell udev what device nodes to create for aoe support.
> # They may be installed along the following lines.  Check the section
> # 8 udev manpage to see whether your udev supports SUBSYSTEM, and
> # whether it uses one or two equal signs for SUBSYSTEM and KERNEL.
> #
> #   ecashin@makki ~$ su
> #   Password:
> #   bash# find /etc -type f -name udev.conf
> #   /etc/udev/udev.conf
> #   bash# grep udev_rules= /etc/udev/udev.conf
> #   udev_rules="/etc/udev/rules.d/"
> #   bash# ls /etc/udev/rules.d/
> #   10-wacom.rules  50-udev.rules
> #   bash# cp /path/to/linux/Documentation/admin-guide/aoe/udev.txt \
> #           /etc/udev/rules.d/60-aoe.rules
> #
>
> # aoe char devices
> SUBSYSTEM=="aoe", KERNEL=="discover",   NAME="etherd/%k", GROUP="disk", MODE="0220"
> SUBSYSTEM=="aoe", KERNEL=="err",        NAME="etherd/%k", GROUP="disk", MODE="0440"
> SUBSYSTEM=="aoe", KERNEL=="interfaces", NAME="etherd/%k", GROUP="disk", MODE="0220"
> SUBSYSTEM=="aoe", KERNEL=="revalidate", NAME="etherd/%k", GROUP="disk", MODE="0220"
> SUBSYSTEM=="aoe", KERNEL=="flush",      NAME="etherd/%k", GROUP="disk", MODE="0220"
>
> # aoe block devices
> KERNEL=="etherd*",       GROUP="disk"
> ```

# Example of udev install rules script

> ```
> # install the aoe-specific udev rules from udev.txt into 
> # the system's udev configuration
> # 
>
> me="`basename $0`"
>
> # find udev.conf, often /etc/udev/udev.conf
> # (or environment can specify where to find udev.conf)
> #
> if test -z "$conf"; then
> 	if test -r /etc/udev/udev.conf; then
> 		conf=/etc/udev/udev.conf
> 	else
> 		conf="`find /etc -type f -name udev.conf 2> /dev/null`"
> 		if test -z "$conf" || test ! -r "$conf"; then
> 			echo "$me Error: no udev.conf found" 1>&2
> 			exit 1
> 		fi
> 	fi
> fi
>
> # find the directory where udev rules are stored, often
> # /etc/udev/rules.d
> #
> rules_d="`sed -n '/^udev_rules=/{ s!udev_rules=!!; s!\"!!g; p; }' $conf`"
> if test -z "$rules_d" ; then
> 	rules_d=/etc/udev/rules.d
> fi
> if test ! -d "$rules_d"; then
> 	echo "$me Error: cannot find udev rules directory" 1>&2
> 	exit 1
> fi
> sh -xc "cp `dirname $0`/udev.txt $rules_d/60-aoe.rules"
> ```

# Example script to get status

> ```
> #! /bin/sh
> # collate and present sysfs information about AoE storage
> #
> # A more complete version of this script is aoe-stat, in the
> # aoetools.
>
> set -e
> format="%8s\t%8s\t%8s\n"
> me=`basename $0`
> sysd=${sysfs_dir:-/sys}
>
> # printf "$format" device mac netif state
>
> # Suse 9.1 Pro doesn't put /sys in /etc/mtab
> #test -z "`mount | grep sysfs`" && {
> test ! -d "$sysd/block" && {
> 	echo "$me Error: sysfs is not mounted" 1>&2
> 	exit 1
> }
>
> for d in `ls -d $sysd/block/etherd* 2>/dev/null | grep -v p` end; do
> 	# maybe ls comes up empty, so we use "end"
> 	test $d = end && continue
>
> 	dev=`echo "$d" | sed 's/.*!//'`
> 	printf "$format" \
> 		"$dev" \
> 		"`cat \"$d/netif\"`" \
> 		"`cat \"$d/state\"`"
> done | sort
> ```

# Example of AoE autoload script

> ```
> #!/bin/sh
> # set aoe to autoload by installing the
> # aliases in /etc/modprobe.d/
>
> f=/etc/modprobe.d/aoe.conf
>
> if test ! -r $f || test ! -w $f; then
> 	echo "cannot configure $f for module autoloading" 1>&2
> 	exit 1
> fi
>
> grep major-152 $f >/dev/null
> if [ $? = 1 ]; then
> 	echo alias block-major-152 aoe >> $f
> 	echo alias char-major-152 aoe >> $f
> fi
> ```
