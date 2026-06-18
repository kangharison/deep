# Notifier error injection

> 출처(원문): https://docs.kernel.org/fault-injection/notifier-error-inject.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Notifier error injection

Notifier error injection provides the ability to inject artificial errors to
specified notifier chain callbacks. It is useful to test the error handling of
notifier call chain failures which is rarely executed. There are kernel
modules that can be used to test the following notifiers.

> * PM notifier
> * Memory hotplug notifier
> * powerpc pSeries reconfig notifier
> * Netdevice notifier

## PM notifier error injection module

This feature is controlled through debugfs interface

> /sys/kernel/debug/notifier-error-inject/pm/actions/<notifier event>/error

Possible PM notifier events to be failed are:

> * PM\_HIBERNATION\_PREPARE
> * PM\_SUSPEND\_PREPARE
> * PM\_RESTORE\_PREPARE

Example: Inject PM suspend error (-12 = -ENOMEM):

```
# cd /sys/kernel/debug/notifier-error-inject/pm/
# echo -12 > actions/PM_SUSPEND_PREPARE/error
# echo mem > /sys/power/state
bash: echo: write error: Cannot allocate memory
```

## Memory hotplug notifier error injection module

This feature is controlled through debugfs interface

> /sys/kernel/debug/notifier-error-inject/memory/actions/<notifier event>/error

Possible memory notifier events to be failed are:

> * MEM\_GOING\_ONLINE
> * MEM\_GOING\_OFFLINE

Example: Inject memory hotplug offline error (-12 == -ENOMEM):

```
# cd /sys/kernel/debug/notifier-error-inject/memory
# echo -12 > actions/MEM_GOING_OFFLINE/error
# echo offline > /sys/devices/system/memory/memoryXXX/state
bash: echo: write error: Cannot allocate memory
```

## powerpc pSeries reconfig notifier error injection module

This feature is controlled through debugfs interface

> /sys/kernel/debug/notifier-error-inject/pSeries-reconfig/actions/<notifier event>/error

Possible pSeries reconfig notifier events to be failed are:

> * PSERIES\_RECONFIG\_ADD
> * PSERIES\_RECONFIG\_REMOVE
> * PSERIES\_DRCONF\_MEM\_ADD
> * PSERIES\_DRCONF\_MEM\_REMOVE

## Netdevice notifier error injection module

This feature is controlled through debugfs interface

> /sys/kernel/debug/notifier-error-inject/netdev/actions/<notifier event>/error

Netdevice notifier events which can be failed are:

> * NETDEV\_REGISTER
> * NETDEV\_CHANGEMTU
> * NETDEV\_CHANGENAME
> * NETDEV\_PRE\_UP
> * NETDEV\_PRE\_TYPE\_CHANGE
> * NETDEV\_POST\_INIT
> * NETDEV\_PRECHANGEMTU
> * NETDEV\_PRECHANGEUPPER
> * NETDEV\_CHANGEUPPER

Example: Inject netdevice mtu change error (-22 == -EINVAL):

```
# cd /sys/kernel/debug/notifier-error-inject/netdev
# echo -22 > actions/NETDEV_CHANGEMTU/error
# ip link set eth0 mtu 1024
RTNETLINK answers: Invalid argument
```

## For more usage examples

There are tools/testing/selftests using the notifier error injection features
for CPU and memory notifiers.

> * tools/testing/selftests/cpu-hotplug/cpu-on-off-test.sh
> * tools/testing/selftests/memory-hotplug/mem-on-off-test.sh

These scripts first do simple online and offline tests and then do fault
injection tests if notifier error injection module is available.
