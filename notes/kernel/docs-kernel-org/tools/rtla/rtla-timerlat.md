# rtla-timerlat

> 출처(원문): https://docs.kernel.org/tools/rtla/rtla-timerlat.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rtla-timerlat

## Measures the operating system timer latency

Manual section:
:   1

### SYNOPSIS

**rtla timerlat** [*MODE*] ...

### DESCRIPTION

The **rtla timerlat** tool is an interface for the *timerlat* tracer. The
*timerlat* tracer dispatches a kernel thread per-cpu. These threads
set a periodic timer to wake themselves up and go back to sleep. After
the wakeup, they collect and generate useful information for the
debugging of operating system timer latency.

The *timerlat* tracer outputs information in two ways. It periodically
prints the timer latency at the timer *IRQ* handler and the *Thread*
handler. It also enables the trace of the most relevant information via
**osnoise:** tracepoints.

The **rtla timerlat** tool sets the options of the *timerlat* tracer
and collects and displays a summary of the results. By default,
the collection is done synchronously in kernel space using a dedicated
BPF program attached to the *timerlat* tracer. If either BPF or
the **osnoise:timerlat\_sample** tracepoint it attaches to is
unavailable, the **rtla timerlat** tool falls back to using tracefs to
process the data asynchronously in user space.

The **rtla timerlat top** mode displays a summary of the periodic output
from the *timerlat* tracer. The **rtla timerlat hist** mode displays
a histogram of each tracer event occurrence. For further details, please
refer to the respective man page.

### MODES

**top**

> Prints the summary from *timerlat* tracer.

**hist**

> Prints a histogram of timerlat samples.

If no *MODE* is given, the top mode is called, passing the arguments.

### OPTIONS

**-h**, **--help**

> Display the help text.

For other options, see the man page for the corresponding mode.

### SEE ALSO

**rtla-timerlat-top**(1), **rtla-timerlat-hist**(1)

[Timerlat tracer](https://docs.kernel.org/trace/timerlat-tracer.html)

### AUTHOR

Written by Daniel Bristot de Oliveira <[bristot@kernel.org](mailto:bristot%40kernel.org)>

### SIGINT BEHAVIOR

On the first SIGINT, RTLA exits after collecting all outstanding samples up to
the point of receiving the signal.

When receiving more than one SIGINT, RTLA discards any outstanding samples, and
exits while displaying only samples that have already been processed.

If SIGINT is received during RTLA cleanup, RTLA exits immediately via
the default signal handler.

Note: For the purpose of SIGINT behavior, the expiry of duration specified via
the -d/--duration option is treated as equivalent to receiving a SIGINT. For
example, a SIGINT received after duration expired but samples have not been
processed yet will drop any outstanding samples.

Also note that when using the timerlat tool in BPF mode, samples are processed
in-kernel; RTLA only copies them out to display them to the user. A second
SIGINT does not affect in-kernel sample aggregation.

### EXIT STATUS

```
0  Passed: the test did not hit the stop tracing condition
1  Error: invalid argument
2  Failed: the test hit the stop tracing condition
```

### REPORTING BUGS

Report bugs to <[linux-kernel@vger.kernel.org](mailto:linux-kernel%40vger.kernel.org)>
and <[linux-trace-devel@vger.kernel.org](mailto:linux-trace-devel%40vger.kernel.org)>

### LICENSE

**rtla** is Free Software licensed under the GNU GPLv2

### COPYING

Copyright (C) 2021 Red Hat, Inc. Free use of this software is granted under
the terms of the GNU Public License (GPL).
