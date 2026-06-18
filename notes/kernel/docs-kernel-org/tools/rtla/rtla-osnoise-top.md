# rtla-osnoise-top

> 출처(원문): https://docs.kernel.org/tools/rtla/rtla-osnoise-top.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rtla-osnoise-top

## Display a summary of the operating system noise

Manual section:
:   1

### SYNOPSIS

**rtla osnoise top** [*OPTIONS*]

### DESCRIPTION

The **rtla osnoise** tool is an interface for the *osnoise* tracer. The
*osnoise* tracer dispatches a kernel thread per-cpu. These threads read the
time in a loop while with preemption, softirq and IRQs enabled, thus
allowing all the sources of operating system noise during its execution.
The *osnoise*’s tracer threads take note of the delta between each time
read, along with an interference counter of all sources of interference.
At the end of each period, the *osnoise* tracer displays a summary of
the results.

**rtla osnoise top** collects the periodic summary from the *osnoise* tracer,
including the counters of the occurrence of the interference source,
displaying the results in a user-friendly format.

The tool also allows many configurations of the *osnoise* tracer and the
collection of the tracer output.

### OPTIONS

**-a**, **--auto** *us*

> Set the automatic trace mode. This mode sets some commonly used options
> while debugging the system. It is equivalent to use **-s** *us* **-T 1 -t**.

**-p**, **--period** *us*

> Set the *osnoise* tracer period in microseconds.

**-r**, **--runtime** *us*

> Set the *osnoise* tracer runtime in microseconds.

**-s**, **--stop** *us*

> Stop the trace if a single sample is higher than the argument in microseconds.
> If **-T** is set, it will also save the trace to the output.

**-S**, **--stop-total** *us*

> Stop the trace if the total sample is higher than the argument in microseconds.
> If **-T** is set, it will also save the trace to the output.

**-T**, **--threshold** *us*

> Specify the minimum delta between two time reads to be considered noise.
> The default threshold is *5 us*.

**-t**, **--trace** [*file*]

> Save the stopped trace to [*file|osnoise\_trace.txt*].

**-q**, **--quiet**

> Print only a summary at the end of the session.

**-c**, **--cpus** *cpu-list*

> Set the osnoise top tracer to run the sample threads in the cpu-list.
>
> By default, the osnoise top tracer runs the sample threads on all CPUs.

**-H**, **--house-keeping** *cpu-list*

> Run rtla control threads only on the given cpu-list.
>
> If omitted, rtla will attempt to auto-migrate its main thread to any CPU that is not running any workload threads.

**-d**, **--duration** *time[s|m|h|d]*

> Set the duration of the session.

**-D**, **--debug**

> Print debug info.

**-e**, **--event** *sys:event*

> Enable an event in the trace (**-t**) session. The argument can be a specific event, e.g., **-e** *sched:sched\_switch*, or all events of a system group, e.g., **-e** *sched*. Multiple **-e** are allowed. It is only active when **-t** or **-a** are set.

**--filter** *<filter>*

> Filter the previous **-e** *sys:event* event with *<filter>*. For further information about event filtering see <https://www.kernel.org/doc/html/latest/trace/events.html#event-filtering>.

**--trigger** *<trigger>*
:   Enable a trace event trigger to the previous **-e** *sys:event*.
    If the *hist:* trigger is activated, the output histogram will be automatically saved to a file named *system\_event\_hist.txt*.
    For example, the command:

    rtla <command> <mode> -t -e osnoise:irq\_noise --trigger=”hist:key=desc,duration/1000:sort=desc,duration/1000:vals=hitcount”

    Will automatically save the content of the histogram associated to *osnoise:irq\_noise* event in *osnoise\_irq\_noise\_hist.txt*.

    For further information about event trigger see <https://www.kernel.org/doc/html/latest/trace/events.html#event-triggers>.

**-P**, **--priority** *o:prio|r:prio|f:prio|d:runtime:period*

> Set scheduling parameters to the osnoise top tracer threads, the format to set the priority are:
>
> * *o:prio* - use SCHED\_OTHER with *prio*;
> * *r:prio* - use SCHED\_RR with *prio*;
> * *f:prio* - use SCHED\_FIFO with *prio*;
> * *d:runtime[us|ms|s]:period[us|ms|s]* - use SCHED\_DEADLINE with *runtime* and *period* in nanoseconds.
>
> If not set, tracer threads keep their default priority. For rtla user threads, it is set to SCHED\_FIFO with priority 95. For kernel threads, see *osnoise* and *timerlat* tracer documentation for the running kernel version.

**-C**, **--cgroup** [*cgroup*]

> Set a *cgroup* to the tracer’s threads. If the **-C** option is passed without arguments, the tracer’s thread will inherit **rtla**’s *cgroup*. Otherwise, the threads will be placed on the *cgroup* passed to the option.
>
> If not set, the behavior differs between workload types. User workloads created by rtla will inherit rtla’s cgroup. Kernel workloads are assigned the root cgroup.

**--warm-up** *s*

> After starting the workload, let it run for *s* seconds before starting collecting the data, allowing the system to warm-up. Statistical data generated during warm-up is discarded.

**--trace-buffer-size** *kB*
:   Set the per-cpu trace buffer size in kB for the tracing output.

    If not set, the default tracefs buffer size is used.

**--on-threshold** *action*

> Defines an action to be executed when tracing is stopped on a latency threshold
> specified by **-a/--auto**, **-s/--stop**, or **-S/--stop-total**.
>
> Multiple --on-threshold actions may be specified, and they will be executed in
> the order they are provided. If any action fails, subsequent actions in the list
> will not be executed.
>
> Supported actions are:
>
> * *trace[,file=<filename>]*
>
>   Saves trace output, optionally taking a filename. Alternative to -t/--trace.
>   Note that unlike -t/--trace, specifying this multiple times will result in
>   the trace being saved multiple times.
> * *signal,num=<sig>,pid=<pid>*
>
>   Sends signal to process. “parent” might be specified in place of pid to target
>   the parent process of rtla.
> * *shell,command=<command>*
>
>   Execute shell command.
> * *continue*
>
>   Continue tracing after actions are executed instead of stopping.
>
> Example:
>
> $ rtla osnoise top -s 20 --on-threshold trace
> --on-threshold shell,command=”grep ipi\_send osnoise\_trace.txt”
> --on-threshold signal,num=2,pid=parent
>
> This will save a trace with the default filename “osnoise\_trace.txt”, print its
> lines that contain the text “ipi\_send” on standard output, and send signal 2
> (SIGINT) to the parent process.
>
> Performance Considerations:
>
> Due to implementational limitations, actions might be delayed
> up to one second after tracing is stopped.

**--on-end** *action*

> Defines an action to be executed at the end of tracing.
>
> Multiple --on-end actions can be specified, and they will be executed in the order
> they are provided. If any action fails, subsequent actions in the list will not be
> executed.
>
> See the documentation for **--on-threshold** for the list of supported actions, with
> the exception that *continue* has no effect.
>
> Example:
>
> $ rtla osnoise top -d 5s --on-end trace
>
> This runs rtla with the default options, and saves trace output at the end.

**-h**, **--help**

> Print help menu.

### EXAMPLE

In the example below, the **rtla osnoise top** tool is set to run with a
real-time priority *FIFO:1*, on CPUs *0-3*, for *900ms* at each period
(*1s* by default). The reason for reducing the runtime is to avoid starving
the rtla tool. The tool is also set to run for *one minute* and to display
a summary of the report at the end of the session:

```
[root@f34 ~]# rtla osnoise top -P F:1 -c 0-3 -r 900000 -d 1M -q
                                        Operating System Noise
duration:   0 00:01:00 | time is in us
CPU Period       Runtime        Noise  % CPU Aval   Max Noise   Max Single          HW          NMI          IRQ      Softirq       Thread
  0 #59         53100000       304896    99.42580        6978           56         549            0        53111         1590           13
  1 #59         53100000       338339    99.36282        8092           24         399            0        53130         1448           31
  2 #59         53100000       290842    99.45227        6582           39         855            0        53110         1406           12
  3 #59         53100000       204935    99.61405        6251           33         290            0        53156         1460           12
```

### SEE ALSO

**rtla-osnoise**(1), **rtla-osnoise-hist**(1)

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
