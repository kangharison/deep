# rv-mon-wwnr

> 출처(원문): https://docs.kernel.org/tools/rv/rv-mon-wwnr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rv-mon-wwnr

## Wakeup While Not Running monitor

Manual section:
:   1

### SYNOPSIS

**rv mon wip** [*OPTIONS*]

### DESCRIPTION

The wakeup while not running (**wwnr**) is a per-task sample monitor.

See kernel documentation for further information about this monitor:
<<https://docs.kernel.org/trace/rv/monitor_wwnr.html>>

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

### SEE ALSO

**rv**(1), **rv-mon**(1)

Linux kernel *RV* documentation:
<<https://www.kernel.org/doc/html/latest/trace/rv/index.html>>

### AUTHOR

Written by Daniel Bristot de Oliveira <[bristot@kernel.org](mailto:bristot%40kernel.org)>

### REPORTING BUGS

Report bugs to <[linux-kernel@vger.kernel.org](mailto:linux-kernel%40vger.kernel.org)>
and <[linux-trace-devel@vger.kernel.org](mailto:linux-trace-devel%40vger.kernel.org)>

### LICENSE

**rv** is Free Software licensed under the GNU GPLv2

### COPYING

Copyright (C) 2022 Red Hat, Inc. Free use of this software is granted under
the terms of the GNU Public License (GPL).
