# ibm 3270 changelog

> 출처(원문): https://docs.kernel.org/arch/s390/text_files.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ibm 3270 changelog

```
ChangeLog for the UTS Global 3270-support patch

Sep 2002:       Get bootup colors right on 3270 console
        * In tubttybld.c, substantially revise ESC processing so that
          ESC sequences (especially coloring ones) and the strings
          they affect work as right as 3270 can get them.  Also, set
          screen height to omit the two rows used for input area, in
          tty3270_open() in tubtty.c.

Sep 2002:       Dynamically get 3270 input buffer
        * Oversize 3270 screen widths may exceed GEOM_MAXINPLEN columns,
          so get input-area buffer dynamically when sizing the device in
          tubmakemin() in tuball.c (if it's the console) or tty3270_open()
          in tubtty.c (if needed).  Change tubp->tty_input to be a
          pointer rather than an array, in tubio.h.

Sep 2002:       Fix tubfs kmalloc()s
        * Do read and write lengths correctly in fs3270_read()
          and fs3270_write(), while never asking kmalloc()
          for more than 0x800 bytes.  Affects tubfs.c and tubio.h.

Sep 2002:       Recognize 3270 control unit type 3174
        * Recognize control-unit type 0x3174 as well as 0x327?.
          The IBM 2047 device emulates a 3174 control unit.
          Modularize control-unit recognition in tuball.c by
          adding and invoking new tub3270_is_ours().

Apr 2002:       Fix 3270 console reboot loop
        * (Belated log entry) Fixed reboot loop if 3270 console,
          in tubtty.c:ttu3270_bh().

Feb 6, 2001:
        * This changelog is new
        * tub3270 now supports 3270 console:
                Specify y for CONFIG_3270 and y for CONFIG_3270_CONSOLE.
                Support for 3215 will not appear if 3270 console support
                is chosen.
                NOTE:  The default is 3270 console support, NOT 3215.
        * the components are remodularized: added source modules are
          tubttybld.c and tubttyscl.c, for screen-building code and
          scroll-timeout code.
        * tub3270 source for this (2.4.0) version is #ifdeffed to
          build with both 2.4.0 and 2.2.16.2.
        * color support and minimal other ESC-sequence support is added.
```

# ibm 3270 config3270.sh

```
#!/bin/sh
#
# config3270 -- Autoconfigure /dev/3270/* and /etc/inittab
#
#       Usage:
#               config3270
#
#       Output:
#               /tmp/mkdev3270
#
#       Operation:
#               1. Run this script
#               2. Run the script it produces: /tmp/mkdev3270
#               3. Issue "telinit q" or reboot, as appropriate.
#
P=/proc/tty/driver/tty3270
ROOT=
D=$ROOT/dev
SUBD=3270
TTY=$SUBD/tty
TUB=$SUBD/tub
SCR=$ROOT/tmp/mkdev3270
SCRTMP=$SCR.a
GETTYLINE=:2345:respawn:/sbin/mingetty
INITTAB=$ROOT/etc/inittab
NINITTAB=$ROOT/etc/NEWinittab
OINITTAB=$ROOT/etc/OLDinittab
ADDNOTE=\\"# Additional mingettys for the 3270/tty* driver, tub3270 ---\\"

if ! ls $P > /dev/null 2>&1; then
	modprobe tub3270 > /dev/null 2>&1
fi
ls $P > /dev/null 2>&1 || exit 1

# Initialize two files, one for /dev/3270 commands and one
# to replace the /etc/inittab file (old one saved in OLDinittab)
echo "#!/bin/sh" > $SCR || exit 1
echo " " >> $SCR
echo "# Script built by /sbin/config3270" >> $SCR
if [ ! -d /dev/dasd ]; then
	echo rm -rf "$D/$SUBD/*" >> $SCR
fi
echo "grep -v $TTY $INITTAB > $NINITTAB" > $SCRTMP || exit 1
echo "echo $ADDNOTE >> $NINITTAB" >> $SCRTMP
if [ ! -d /dev/dasd ]; then
	echo mkdir -p $D/$SUBD >> $SCR
fi

# Now query the tub3270 driver for 3270 device information
# and add appropriate mknod and mingetty lines to our files
echo what=config > $P
while read devno maj min;do
	if [ $min = 0 ]; then
		fsmaj=$maj
		if [ ! -d /dev/dasd ]; then
			echo mknod $D/$TUB c $fsmaj 0 >> $SCR
			echo chmod 666 $D/$TUB >> $SCR
		fi
	elif [ $maj = CONSOLE ]; then
		if [ ! -d /dev/dasd ]; then
			echo mknod $D/$TUB$devno c $fsmaj $min >> $SCR
		fi
	else
		if [ ! -d /dev/dasd ]; then
			echo mknod $D/$TTY$devno c $maj $min >>$SCR
			echo mknod $D/$TUB$devno c $fsmaj $min >> $SCR
		fi
		echo "echo t$min$GETTYLINE $TTY$devno >> $NINITTAB" >> $SCRTMP
	fi
done < $P

echo mv $INITTAB $OINITTAB >> $SCRTMP || exit 1
echo mv $NINITTAB $INITTAB >> $SCRTMP
cat $SCRTMP >> $SCR
rm $SCRTMP
exit 0
```
