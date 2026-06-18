# rv-mon-sched

> 출처(원문): https://docs.kernel.org/tools/rv/rv-mon-sched.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rv-mon-sched

## Scheduler monitors collection

Manual section:
:   1

### SYNOPSIS

**rv mon sched** [*OPTIONS*]

**rv mon <NESTED\_MONITOR>** [*OPTIONS*]

**rv mon sched:<NESTED\_MONITOR>** [*OPTIONS*]

### DESCRIPTION

The scheduler monitor collection is a container for several monitors to model
the behaviour of the scheduler. Each monitor describes a specification that
the scheduler should follow.

As a monitor container, it will enable all nested monitors and set them
according to OPTIONS.
Nevertheless nested monitors can also be activated independently both by name
and by specifying sched: , e.g. to enable only monitor tss you can do any of:

> # rv mon sched:tss
>
> # rv mon tss

See kernel documentation for further information about this monitor:
<<https://docs.kernel.org/trace/rv/monitor_sched.html>>

### OPTIONS

**-h**, **--help**

> Print the monitor’s options and the available reactors list.

**-r**, **--reactor** *reactor*

> Enables the *reactor*. See **-h** for a list of available reactors.

**-s**, **--self**

> When tracing (**-t**), also print the events that happened during the **rv**
> command itself. If the **rv** command itself generates too many events,
> the tool might get busy processing its own events only.

**-t**, **--trace**

> Trace monitor’s events and error.

**-v**, **--verbose**

> Print debug messages.

### NESTED MONITOR

The available nested monitors are:
:   * scpd: schedule called with preemption disabled
    * snep: schedule does not enable preempt
    * sncid: schedule not called with interrupt disabled
    * snroc: set non runnable on its own context
    * sco: scheduling context operations
    * tss: task switch while scheduling

### SEE ALSO

**rv**(1), **rv-mon**(1)

Linux kernel *RV* documentation:
<<https://www.kernel.org/doc/html/latest/trace/rv/index.html>>

### AUTHOR

Written by Gabriele Monaco <[gmonaco@redhat.com](mailto:gmonaco%40redhat.com)>

### REPORTING BUGS

Report bugs to <[linux-kernel@vger.kernel.org](mailto:linux-kernel%40vger.kernel.org)>
and <[linux-trace-devel@vger.kernel.org](mailto:linux-trace-devel%40vger.kernel.org)>

### LICENSE

**rv** is Free Software licensed under the GNU GPLv2

### COPYING

Copyright (C) 2022 Red Hat, Inc. Free use of this software is granted under
the terms of the GNU Public License (GPL).
