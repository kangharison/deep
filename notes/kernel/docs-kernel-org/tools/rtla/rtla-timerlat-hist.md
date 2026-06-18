# rtla-timerlat-hist

> 출처(원문): https://docs.kernel.org/tools/rtla/rtla-timerlat-hist.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# rtla-timerlat-hist

## Histograms of the operating system timer latency

Manual section:
:   1

### SYNOPSIS

**rtla timerlat hist** [*OPTIONS*] ...

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

The **rtla timerlat hist** displays a histogram of each tracer event
occurrence. This tool uses the periodic information, and the
**osnoise:** tracepoints are enabled when using the **-T** option.

### OPTIONS

**-a**, **--auto** *us*

> Set the automatic trace mode. This mode sets some commonly used options
> while debugging the system. It is equivalent to use **-T** *us* **-s** *us*
> **-t**. By default, *timerlat* tracer uses FIFO:95 for *timerlat* threads,
> thus equivalent to **-P** *f:95*.

**-p**, **--period** *us*

> Set the *timerlat* tracer period in microseconds.

**-i**, **--irq** *us*

> Stop trace if the *IRQ* latency is higher than the argument in us.

**-T**, **--thread** *us*

> Stop trace if the *Thread* latency is higher than the argument in us.

**-s**, **--stack** *us*

> Save the stack trace at the *IRQ* if a *Thread* latency is higher than the
> argument in us.

**-t**, **--trace** [*file*]

> Save the stopped trace to [*file|timerlat\_trace.txt*].

**--dma-latency** *us*
:   Set the /dev/cpu\_dma\_latency to *us*, aiming to bound exit from idle latencies.
    *cyclictest* sets this value to *0* by default, use **--dma-latency** *0* to have
    similar results.

**--deepest-idle-state** *n*
:   Disable idle states higher than *n* for cpus that are running timerlat threads to
    reduce exit from idle latencies. If *n* is -1, all idle states are disabled.
    On exit from timerlat, the idle state setting is restored to its original state
    before running timerlat.

    Requires rtla to be built with libcpupower.

**-k**, **--kernel-threads**

> Use timerlat kernel-space threads, in contrast of **-u**.

**-u**, **--user-threads**

> Set timerlat to run without a workload, and then dispatches user-space workloads
> to wait on the timerlat\_fd. Once the workload is awakened, it goes to sleep again
> adding so the measurement for the kernel-to-user and user-to-kernel to the tracer
> output. **--user-threads** will be used unless the user specify **-k**.

**-U**, **--user-load**

> Set timerlat to run without workload, waiting for the user to dispatch a per-cpu
> task that waits for a new period on the tracing/osnoise/per\_cpu/cpu$ID/timerlat\_fd.
> See linux/tools/rtla/example/timerlat\_load.py for an example of user-load code.

**--bpf-action** *bpf-program*

> Loads a BPF program from an ELF file and executes it when a latency threshold is exceeded.
>
> The BPF program must be a valid ELF file loadable with libbpf. The program must contain
> a function named `action_handler`, stored in an ELF section with the `tp_` prefix.
> The prefix is used by libbpf to set BPF program type to BPF\_PROG\_TYPE\_TRACEPOINT.
>
> The program receives a `struct trace_event_raw_timerlat_sample` parameter
> containing timerlat sample data.
>
> An example is provided in `tools/tracing/rtla/example/timerlat_bpf_action.c`.
> This example demonstrates how to create a BPF program that prints latency information using
> `bpf_trace_printk()` when a threshold is exceeded.
>
> **Note**: BPF actions require BPF support to be available. If BPF is not available
> or disabled, the tool falls back to tracefs mode and BPF actions are not supported.

**--stack-format** *format*

> Adjust the format of the stack trace printed during auto-analysis.
>
> The supported values for *format* are:
>
> * **truncate** Print the stack trace up to the first unknown address (default).
> * **skip** Skip unknown addresses.
> * **full** Print the entire stack trace, including unknown addresses.
>
> For unknown addresses, the raw pointer is printed.

**-b**, **--bucket-size** *N*

> Set the histogram bucket size (default *1*).

**-E**, **--entries** *N*

> Set the number of entries of the histogram (default 256).

**--no-header**

> Do not print header.

**--no-summary**

> Do not print summary.

**--no-index**

> Do not print index.

**--with-zeros**

> Print zero only entries.

**-c**, **--cpus** *cpu-list*

> Set the timerlat hist tracer to run the sample threads in the cpu-list.
>
> By default, the timerlat hist tracer runs the sample threads on all CPUs.

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

> Set scheduling parameters to the timerlat hist tracer threads, the format to set the priority are:
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
> specified by **-a/--auto**, **-i/--irq**, or **-T/--thread**.
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
> $ rtla timerlat hist -T 20 --on-threshold trace
> --on-threshold shell,command=”grep ipi\_send timerlat\_trace.txt”
> --on-threshold signal,num=2,pid=parent
>
> This will save a trace with the default filename “timerlat\_trace.txt”, print its
> lines that contain the text “ipi\_send” on standard output, and send signal 2
> (SIGINT) to the parent process.
>
> Performance Considerations:
>
> For time-sensitive actions, it is recommended to run **rtla timerlat** with BPF
> support and RT priority. Note that due to implementational limitations, actions
> might be delayed up to one second after tracing is stopped if BPF mode is not
> available or disabled.

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
> $ rtla timerlat hist -d 5s --on-end trace
>
> This runs rtla with the default options, and saves trace output at the end.

**-h**, **--help**

> Print help menu.

**--dump-tasks**

> prints the task running on all CPUs if stop conditions are met (depends on !--no-aa)

**--no-aa**

> disable auto-analysis, reducing rtla timerlat cpu usage

### EXAMPLE

In the example below, **rtla timerlat hist** is set to run for *10* minutes,
in the cpus *0-4*, *skipping zero* only lines. Moreover, **rtla timerlat
hist** will change the priority of the *timerlat* threads to run under
*SCHED\_DEADLINE* priority, with a *100us* runtime every *1ms* period. The
*1ms* period is also passed to the *timerlat* tracer. Auto-analysis is disabled
to reduce overhead

```
[root@alien ~]# timerlat hist -d 10m -c 0-4 -P d:100us:1ms -p 1000 --no-aa
# RTLA timerlat histogram
# Time unit is microseconds (us)
# Duration:   0 00:10:00
Index   IRQ-000   Thr-000   IRQ-001   Thr-001   IRQ-002   Thr-002   IRQ-003   Thr-003   IRQ-004   Thr-004
0        276489         0    206089         0    466018         0    481102         0    205546         0
1        318327     35487    388149     30024     94531     48382     83082     71078    388026     55730
2          3282    122584      4019    126527     28231    109012     23311     89309      4568     98739
3           940     11815       837      9863      6209     16227      6895     17196       910      9780
4           444     17287       424     11574      2097     38443      2169     36736       462     13476
5           206     43291       255     25581      1223    101908      1304    101137       236     28913
6           132    101501        96     64584       635    213774       757    215471        99     73453
7            74    169347        65    124758       350     57466       441     53639        69    148573
8            53     85183        31    156751       229      9052       306      9026        39    139907
9            22     10387        12     42762       161      2554       225      2689        19     26192
10           13      1898         8      5770       114      1247       128      1405        13      3772
11            9       560         9       924        71       686        76       765         8       713
12            4       256         2       360        50       411        64       474         3       278
13            2       167         2       172        43       256        53       350         4       180
14            1        88         1       116        15       198        42       223         0       115
15            2        63         3        94        11       139        20       150         0        58
16            2        37         0        56         5        78        10       102         0        39
17            0        18         0        28         4        57         8        80         0        15
18            0         8         0        17         2        50         6        56         0        12
19            0         9         0         5         0        19         0        48         0        18
20            0         4         0         8         0        11         2        27         0         4
21            0         2         0         3         1         9         1        18         0         6
22            0         1         0         3         1         7         0         3         0         5
23            0         2         0         4         0         2         0         7         0         2
24            0         2         0         2         1         3         0         3         0         5
25            0         0         0         1         0         1         0         1         0         3
26            0         1         0         0         0         2         0         2         0         0
27            0         0         0         3         0         1         0         0         0         1
28            0         0         0         3         0         0         0         1         0         0
29            0         0         0         2         0         2         0         1         0         3
30            0         1         0         0         0         0         0         0         0         0
31            0         1         0         0         0         0         0         2         0         2
32            0         0         0         1         0         2         0         0         0         0
33            0         0         0         2         0         0         0         0         0         1
34            0         0         0         0         0         0         0         0         0         2
35            0         1         0         1         0         0         0         0         0         1
36            0         1         0         0         0         1         0         1         0         0
37            0         0         0         1         0         0         0         0         0         0
40            0         0         0         0         0         1         0         1         0         0
41            0         0         0         0         0         0         0         0         0         1
42            0         0         0         0         0         0         0         0         0         1
44            0         0         0         0         0         1         0         0         0         0
46            0         0         0         0         0         0         0         1         0         0
47            0         0         0         0         0         0         0         0         0         1
50            0         0         0         0         0         0         0         0         0         1
54            0         0         0         1         0         0         0         0         0         0
58            0         0         0         1         0         0         0         0         0         0
over:         0         0         0         0         0         0         0         0         0         0
count:   600002    600002    600002    600002    600002    600002    600002    600002    600002    600002
min:          0         1         0         1         0         1         0         1         0         1
avg:          0         5         0         5         0         4         0         4         0         5
max:         16        36        15        58        24        44        21        46        13        50
```

### SEE ALSO

**rtla-timerlat**(1), **rtla-timerlat-top**(1)

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
