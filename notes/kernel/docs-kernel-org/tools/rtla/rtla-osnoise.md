# rtla-osnoise

> 출처(원문): https://docs.kernel.org/tools/rtla/rtla-osnoise.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rtla-osnoise

## Measure the operating system noise

Manual section:
:   1

### SYNOPSIS

**rtla osnoise** [*MODE*] ...

### DESCRIPTION

The **rtla osnoise** tool is an interface for the *osnoise* tracer. The
*osnoise* tracer dispatches a kernel thread per-cpu. These threads read the
time in a loop while with preemption, softirq and IRQs enabled, thus
allowing all the sources of operating system noise during its execution.
The *osnoise*’s tracer threads take note of the delta between each time
read, along with an interference counter of all sources of interference.
At the end of each period, the *osnoise* tracer displays a summary of
the results.

The *osnoise* tracer outputs information in two ways. It periodically prints
a summary of the noise of the operating system, including the counters of
the occurrence of the source of interference. It also provides information
for each noise via the **osnoise:** tracepoints. The **rtla osnoise top**
mode displays information about the periodic summary from the *osnoise* tracer.
The **rtla osnoise hist** mode displays information about the noise using
the **osnoise:** tracepoints. For further details, please refer to the
respective man page.

### MODES

**top**

> Prints the summary from osnoise tracer.

**hist**

> Prints a histogram of osnoise samples.

If no MODE is given, the top mode is called, passing the arguments.

### OPTIONS

**-h**, **--help**

> Display the help text.

For other options, see the man page for the corresponding mode.

### SEE ALSO

**rtla-osnoise-top**(1), **rtla-osnoise-hist**(1)

[Osnoise tracer](https://docs.kernel.org/trace/osnoise-tracer.html)

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
