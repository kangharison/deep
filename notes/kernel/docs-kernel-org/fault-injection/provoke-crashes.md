# Provoking crashes with Linux Kernel Dump Test Module (LKDTM)

> 출처(원문): https://docs.kernel.org/fault-injection/provoke-crashes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Provoking crashes with Linux Kernel Dump Test Module (LKDTM)

The lkdtm module provides an interface to disrupt (and usually crash)
the kernel at predefined code locations to evaluate the reliability of
the kernel’s exception handling and to test crash dumps obtained using
different dumping solutions. The module uses KPROBEs to instrument the
trigger location, but can also trigger the kernel directly without KPROBE
support via debugfs.

You can select the location of the trigger (“crash point name”) and the
type of action (“crash point type”) either through module arguments when
inserting the module, or through the debugfs interface.

Usage:

```
insmod lkdtm.ko [recur_count={>0}] cpoint_name=<> cpoint_type=<>
                [cpoint_count={>0}]
```

recur\_count
:   Recursion level for the stack overflow test. By default this is
    dynamically calculated based on kernel configuration, with the
    goal of being just large enough to exhaust the kernel stack. The
    value can be seen at /sys/module/lkdtm/parameters/recur\_count.

cpoint\_name
:   Where in the kernel to trigger the action. It can be
    one of INT\_HARDWARE\_ENTRY, INT\_HW\_IRQ\_EN, INT\_TASKLET\_ENTRY,
    FS\_SUBMIT\_BH, MEM\_SWAPOUT, TIMERADD, SCSI\_QUEUE\_RQ, or DIRECT.

cpoint\_type
:   Indicates the action to be taken on hitting the crash point.
    These are numerous, and best queried directly from debugfs. Some
    of the common ones are PANIC, BUG, EXCEPTION, LOOP, and OVERFLOW.
    See the contents of /sys/kernel/debug/provoke-crash/DIRECT for
    a complete list.

cpoint\_count
:   Indicates the number of times the crash point is to be hit
    before triggering the action. The default is 10 (except for
    DIRECT, which always fires immediately).

You can also induce failures by mounting debugfs and writing the type to
<debugfs>/provoke-crash/<crashpoint>. E.g.:

```
mount -t debugfs debugfs /sys/kernel/debug
echo EXCEPTION > /sys/kernel/debug/provoke-crash/INT_HARDWARE_ENTRY
```

The special file DIRECT will induce the action directly without KPROBE
instrumentation. This mode is the only one available when the module is
built for a kernel without KPROBEs support:

```
# Instead of having a BUG kill your shell, have it kill "cat":
cat <(echo WRITE_RO) >/sys/kernel/debug/provoke-crash/DIRECT
```
