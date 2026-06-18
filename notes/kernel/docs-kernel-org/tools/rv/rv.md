# rv

> 출처(원문): https://docs.kernel.org/tools/rv/rv.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rv

## Runtime Verification

Manual section:
:   1

### SYNOPSIS

**rv** *COMMAND* [*OPTIONS*]

### DESCRIPTION

Runtime Verification (**RV**) is a lightweight (yet rigorous) method
for formal verification with a practical approach for complex systems.
Instead of relying on a fine-grained model of a system (e.g., a
re-implementation a instruction level), RV works by analyzing the trace
of the system’s actual execution, comparing it against a formal
specification of the system behavior.

The **rv** tool provides the interface for a collection of runtime
verification (rv) monitors.

### COMMANDS

**list**

> List all available monitors.

**mon**

> Run monitor.

### OPTIONS

**-h**, **--help**

> Display the help text.

For other options, see the man page for the corresponding command.

### SEE ALSO

**rv-list**(1), **rv-mon**(1)

Linux kernel *RV* documentation:
<<https://www.kernel.org/doc/html/latest/trace/rv/index.html>>

### AUTHOR

Daniel Bristot de Oliveira <[bristot@kernel.org](mailto:bristot%40kernel.org)>

### REPORTING BUGS

Report bugs to <[linux-kernel@vger.kernel.org](mailto:linux-kernel%40vger.kernel.org)>
and <[linux-trace-devel@vger.kernel.org](mailto:linux-trace-devel%40vger.kernel.org)>

### LICENSE

**rv** is Free Software licensed under the GNU GPLv2

### COPYING

Copyright (C) 2022 Red Hat, Inc. Free use of this software is granted under
the terms of the GNU Public License (GPL).
