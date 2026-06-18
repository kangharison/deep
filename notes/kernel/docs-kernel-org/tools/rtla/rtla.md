# rtla

> 출처(원문): https://docs.kernel.org/tools/rtla/rtla.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rtla

## Real-time Linux Analysis tool

Manual section:
:   1

### SYNOPSIS

**rtla** *COMMAND* [*OPTIONS*]

### DESCRIPTION

The **rtla** is a meta-tool that includes a set of commands that aims to
analyze the real-time properties of Linux. But instead of testing Linux
as a black box, **rtla** leverages kernel tracing capabilities to provide
precise information about the properties and root causes of unexpected
results.

### COMMANDS

**hwnoise**

> Detect and quantify hardware-related noise.

**osnoise**

> Gives information about the operating system noise (osnoise).

**timerlat**

> Measures the IRQ and thread timer latency.

### OPTIONS

**-h**, **--help**

> Display the help text.

For other options, see the man page for the corresponding command.

### SEE ALSO

**rtla-hwnoise**(1), **rtla-osnoise**(1), **rtla-timerlat**(1)

### AUTHOR

Daniel Bristot de Oliveira <[bristot@kernel.org](mailto:bristot%40kernel.org)>

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
