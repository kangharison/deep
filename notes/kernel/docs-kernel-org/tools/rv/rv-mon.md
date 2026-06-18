# rv-mon

> 출처(원문): https://docs.kernel.org/tools/rv/rv-mon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rv-mon

## List available monitors

Manual section:
:   1

### SYNOPSIS

**rv mon** [*-h*] **monitor\_name** [*-h*] [*MONITOR OPTIONS*]

### DESCRIPTION

The **rv mon** command runs the monitor named *monitor\_name*. Each monitor
has its own set of options. The **rv list** command shows all available
monitors.

### OPTIONS

**-h**, **--help**

> Print help menu.

### AVAILABLE MONITORS

The **rv** tool provides the interface for a set of monitors. Use the
**rv list** command to list all available monitors.

Each monitor has its own set of options. See man **rv-mon**-*monitor\_name*
for details about each specific monitor. Also, running **rv mon**
**monitor\_name** **-h** display the help menu with the available
options.

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
