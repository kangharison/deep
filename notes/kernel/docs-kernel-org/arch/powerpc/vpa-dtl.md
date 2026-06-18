# DTL (Dispatch Trace Log)

> 출처(원문): https://docs.kernel.org/arch/powerpc/vpa-dtl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [DTL (Dispatch Trace Log)](#id1)

Athira Rajeev, 19 April 2025

## [Basic overview](#id2)

The pseries Shared Processor Logical Partition(SPLPAR) machines can
retrieve a log of dispatch and preempt events from the hypervisor
using data from Disptach Trace Log(DTL) buffer. With this information,
user can retrieve when and why each dispatch & preempt has occurred.
The vpa-dtl PMU exposes the Virtual Processor Area(VPA) DTL counters
via perf.

## [Infrastructure used](#id3)

The VPA DTL PMU counters do not interrupt on overflow or generate any
PMI interrupts. Therefore, hrtimer is used to poll the DTL data. The timer
nterval can be provided by user via sample\_period field in nano seconds.
vpa dtl pmu has one hrtimer added per vpa-dtl pmu thread. DTL (Dispatch
Trace Log) contains information about dispatch/preempt, enqueue time etc.
We directly copy the DTL buffer data as part of auxiliary buffer and it
will be processed later. This will avoid time taken to create samples
in the kernel space. The PMU driver collecting Dispatch Trace Log (DTL)
entries makes use of AUX support in perf infrastructure. On the tools side,
this data is made available as PERF\_RECORD\_AUXTRACE records.

To correlate each DTL entry with other events across CPU’s, an auxtrace\_queue
is created for each CPU. Each auxtrace queue has a array/list of auxtrace buffers.
All auxtrace queues is maintained in auxtrace heap. The queues are sorted
based on timestamp. When the different PERF\_RECORD\_XX records are processed,
compare the timestamp of perf record with timestamp of top element in the
auxtrace heap so that DTL events can be co-related with other events
Process the auxtrace queue if the timestamp of element from heap is
lower than timestamp from entry in perf record. Sometimes it could happen that
one buffer is only partially processed. if the timestamp of occurrence of
another event is more than currently processed element in the queue, it will
move on to next perf record. So keep track of position of buffer to continue
processing next time. Update the timestamp of the auxtrace heap with the timestamp
of last processed entry from the auxtrace buffer.

This infrastructure ensures dispatch trace log entries can be correlated
and presented along with other events like sched.

## [vpa-dtl PMU example usage](#id4)

```
# ls /sys/devices/vpa_dtl/
events  format  perf_event_mux_interval_ms  power  subsystem  type  uevent
```

To capture the DTL data using perf record:
.. code-block:: sh

> # ./perf record -a -e sched:\*,vpa\_dtl/dtl\_all/ -c 1000000000 sleep 1

The result can be interpreted using perf record. Snippet of perf report -D

```
# ./perf report -D
```

There are different PERF\_RECORD\_XX records. In that records corresponding to
auxtrace buffers includes:

1. PERF\_RECORD\_AUX
   Conveys that new data is available in AUX area
2. PERF\_RECORD\_AUXTRACE\_INFO
   Describes offset and size of auxtrace data in the buffers
3. PERF\_RECORD\_AUXTRACE
   This is the record that defines the auxtrace data which here in case of
   vpa-dtl pmu is dispatch trace log data.

Snippet from perf report -D showing the PERF\_RECORD\_AUXTRACE dump

```

```

0 0 0x39b10 [0x30]: PERF\_RECORD\_AUXTRACE size: 0x690 offset: 0 ref: 0 idx: 0 tid: -1 cpu: 0
.
. ... VPA DTL PMU data: size 1680 bytes, entries is 35
. 00000000: boot\_tb: 21349649546353231, tb\_freq: 512000000
. 00000030: dispatch\_reason:decrementer interrupt, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:7064, ready\_to\_enqueue\_time:187, waiting\_to\_ready\_time:6611773
. 00000060: dispatch\_reason:priv doorbell, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:146, ready\_to\_enqueue\_time:0, waiting\_to\_ready\_time:15359437
. 00000090: dispatch\_reason:decrementer interrupt, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:4868, ready\_to\_enqueue\_time:232, waiting\_to\_ready\_time:5100709
. 000000c0: dispatch\_reason:priv doorbell, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:179, ready\_to\_enqueue\_time:0, waiting\_to\_ready\_time:30714243
. 000000f0: dispatch\_reason:priv doorbell, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:197, ready\_to\_enqueue\_time:0, waiting\_to\_ready\_time:15350648
. 00000120: dispatch\_reason:priv doorbell, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:213, ready\_to\_enqueue\_time:0, waiting\_to\_ready\_time:15353446
. 00000150: dispatch\_reason:priv doorbell, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:212, ready\_to\_enqueue\_time:0, waiting\_to\_ready\_time:15355126
. 00000180: dispatch\_reason:decrementer interrupt, preempt\_reason:H\_CEDE, enqueue\_to\_dispatch\_time:6368, ready\_to\_enqueue\_time:164, waiting\_to\_ready\_time:5104665

Above is representation of dtl entry of below format:

struct dtl\_entry {
:   u8 dispatch\_reason;
    u8 preempt\_reason;
    u16 processor\_id;
    u32 enqueue\_to\_dispatch\_time;
    u32 ready\_to\_enqueue\_time;
    u32 waiting\_to\_ready\_time;
    u64 timebase;
    u64 fault\_addr;
    u64 srr0;
    u64 srr1;

};

First two fields represent the dispatch reason and preempt reason. The post
processing of PERF\_RECORD\_AUXTRACE records will translate to meaningful data
for user to consume.

## [Visualize the dispatch trace log entries with perf report](#id5)

```
# ./perf record -a -e sched:*,vpa_dtl/dtl_all/ -c 1000000000 sleep 1
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.300 MB perf.data ]

# ./perf report
# Samples: 321  of event 'vpa-dtl'
# Event count (approx.): 321
#
# Children      Self  Command  Shared Object      Symbol
# ........  ........  .......  .................  ..............................
#
   100.00%   100.00%  swapper  [kernel.kallsyms]  [k] plpar_hcall_norets_notrace
```

## [Visualize the dispatch trace log entries with perf script](#id6)

```
# ./perf script
  migration/9      67 [009] 105373.359903:                     sched:sched_waking: comm=perf pid=13418 prio=120 target_cpu=009
  migration/9      67 [009] 105373.359904:               sched:sched_migrate_task: comm=perf pid=13418 prio=120 orig_cpu=9 dest_cpu=10
  migration/9      67 [009] 105373.359907:               sched:sched_stat_runtime: comm=migration/9 pid=67 runtime=4050 [ns]
  migration/9      67 [009] 105373.359908:                     sched:sched_switch: prev_comm=migration/9 prev_pid=67 prev_prio=0 prev_state=S ==> next_comm=swapper/9 next_pid=0 next_prio=120
         :256     256 [016] 105373.359913:                                vpa-dtl: timebase: 21403600706628832 dispatch_reason:decrementer interrupt, preempt_reason:H_CEDE, enqueue_to_dispatch_time:4854,                        ready_to_enqueue_time:139, waiting_to_ready_time:511842115 c0000000000fcd28 plpar_hcall_norets_notrace+0x18 ([kernel.kallsyms])
         :256     256 [017] 105373.360012:                                vpa-dtl: timebase: 21403600706679454 dispatch_reason:priv doorbell, preempt_reason:H_CEDE, enqueue_to_dispatch_time:236,                         ready_to_enqueue_time:0, waiting_to_ready_time:133864583 c0000000000fcd28 plpar_hcall_norets_notrace+0x18 ([kernel.kallsyms])
         perf   13418 [010] 105373.360048:               sched:sched_stat_runtime: comm=perf pid=13418 runtime=139748 [ns]
         perf   13418 [010] 105373.360052:                     sched:sched_waking: comm=migration/10 pid=72 prio=0 target_cpu=010
```
