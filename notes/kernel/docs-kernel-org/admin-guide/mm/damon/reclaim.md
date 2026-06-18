# DAMON-based Reclamation

> 출처(원문): https://docs.kernel.org/admin-guide/mm/damon/reclaim.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DAMON-based Reclamation

DAMON-based Reclamation (DAMON\_RECLAIM) is a static kernel module that aimed to
be used for proactive and lightweight reclamation under light memory pressure.
It doesn’t aim to replace the LRU-list based page\_granularity reclamation, but
to be selectively used for different level of memory pressure and requirements.

## Where Proactive Reclamation is Required?

On general memory over-committed systems, proactively reclaiming cold pages
helps saving memory and reducing latency spikes that incurred by the direct
reclaim of the process or CPU consumption of kswapd, while incurring only
minimal performance degradation [[1]](#id4) [[2]](#id5) .

Free Pages Reporting [[3]](#id6) based memory over-commit virtualization systems are
good example of the cases. In such systems, the guest VMs reports their free
memory to host, and the host reallocates the reported memory to other guests.
As a result, the memory of the systems are fully utilized. However, the
guests could be not so memory-frugal, mainly because some kernel subsystems and
user-space applications are designed to use as much memory as available. Then,
guests could report only small amount of memory as free to host, results in
memory utilization drop of the systems. Running the proactive reclamation in
guests could mitigate this problem.

## How It Works?

DAMON\_RECLAIM finds memory regions that didn’t accessed for specific time
duration and page out. To avoid it consuming too much CPU for the paging out
operation, a speed limit can be configured. Under the speed limit, it pages
out memory regions that didn’t accessed longer time first. System
administrators can also configure under what situation this scheme should
automatically activated and deactivated with three memory pressure watermarks.

## Interface: Module Parameters

To use this feature, you should first ensure your system is running on a kernel
that is built with `CONFIG_DAMON_RECLAIM=y`.

To let sysadmins enable or disable it and tune for the given system,
DAMON\_RECLAIM utilizes module parameters. That is, you can put
`damon_reclaim.<parameter>=<value>` on the kernel boot command line or write
proper values to `/sys/module/damon_reclaim/parameters/<parameter>` files.

Below are the description of each parameter.

### enabled

Enable or disable DAMON\_RECLAIM.

You can enable DAMON\_RCLAIM by setting the value of this parameter as `Y`.
Setting it as `N` disables DAMON\_RECLAIM. Note that DAMON\_RECLAIM could do
no real monitoring and reclamation due to the watermarks-based activation
condition. Refer to below descriptions for the watermarks parameter for this.

### commit\_inputs

Make DAMON\_RECLAIM reads the input parameters again, except `enabled`.

Input parameters that updated while DAMON\_RECLAIM is running are not applied
by default. Once this parameter is set as `Y`, DAMON\_RECLAIM reads values
of parametrs except `enabled` again. Once the re-reading is done, this
parameter is set as `N`. If invalid parameters are found while the
re-reading, DAMON\_RECLAIM will be disabled.

Once `Y` is written to this parameter, the user must not write to any
parameters until reading `commit_inputs` again returns `N`. If users
violate this rule, the kernel may exhibit undefined behavior.

### min\_age

Time threshold for cold memory regions identification in microseconds.

If a memory region is not accessed for this or longer time, DAMON\_RECLAIM
identifies the region as cold, and reclaims it.

120 seconds by default.

### quota\_ms

Limit of time for the reclamation in milliseconds.

DAMON\_RECLAIM tries to use only up to this time within a time window
(quota\_reset\_interval\_ms) for trying reclamation of cold pages. This can be
used for limiting CPU consumption of DAMON\_RECLAIM. If the value is zero, the
limit is disabled.

10 ms by default.

### quota\_sz

Limit of size of memory for the reclamation in bytes.

DAMON\_RECLAIM charges amount of memory which it tried to reclaim within a time
window (quota\_reset\_interval\_ms) and makes no more than this limit is tried.
This can be used for limiting consumption of CPU and IO. If this value is
zero, the limit is disabled.

128 MiB by default.

### quota\_reset\_interval\_ms

The time/size quota charge reset interval in milliseconds.

The charget reset interval for the quota of time (quota\_ms) and size
(quota\_sz). That is, DAMON\_RECLAIM does not try reclamation for more than
quota\_ms milliseconds or quota\_sz bytes within quota\_reset\_interval\_ms
milliseconds.

1 second by default.

### quota\_mem\_pressure\_us

Desired level of memory pressure-stall time in microseconds.

While keeping the caps that set by other quotas, DAMON\_RECLAIM automatically
increases and decreases the effective level of the quota aiming this level of
memory pressure is incurred. System-wide `some` memory PSI in microseconds
per quota reset interval (`quota_reset_interval_ms`) is collected and
compared to this value to see if the aim is satisfied. Value zero means
disabling this auto-tuning feature.

Disabled by default.

### quota\_autotune\_feedback

User-specifiable feedback for auto-tuning of the effective quota.

While keeping the caps that set by other quotas, DAMON\_RECLAIM automatically
increases and decreases the effective level of the quota aiming receiving this
feedback of value `10,000` from the user. DAMON\_RECLAIM assumes the feedback
value and the quota are positively proportional. Value zero means disabling
this auto-tuning feature.

Disabled by default.

### wmarks\_interval

Minimal time to wait before checking the watermarks, when DAMON\_RECLAIM is
enabled but inactive due to its watermarks rule.

### wmarks\_high

Free memory rate (per thousand) for the high watermark.

If free memory of the system in bytes per thousand bytes is higher than this,
DAMON\_RECLAIM becomes inactive, so it does nothing but only periodically checks
the watermarks.

### wmarks\_mid

Free memory rate (per thousand) for the middle watermark.

If free memory of the system in bytes per thousand bytes is between this and
the low watermark, DAMON\_RECLAIM becomes active, so starts the monitoring and
the reclaiming.

### wmarks\_low

Free memory rate (per thousand) for the low watermark.

If free memory of the system in bytes per thousand bytes is lower than this,
DAMON\_RECLAIM becomes inactive, so it does nothing but periodically checks the
watermarks. In the case, the system falls back to the LRU-list based page
granularity reclamation logic.

### sample\_interval

Sampling interval for the monitoring in microseconds.

The sampling interval of DAMON for the cold memory monitoring. Please refer to
the DAMON documentation ([Detailed Usages](usage.html)) for more detail.

### aggr\_interval

Aggregation interval for the monitoring in microseconds.

The aggregation interval of DAMON for the cold memory monitoring. Please
refer to the DAMON documentation ([Detailed Usages](usage.html)) for more detail.

### min\_nr\_regions

Minimum number of monitoring regions.

The minimal number of monitoring regions of DAMON for the cold memory
monitoring. This can be used to set lower-bound of the monitoring quality.
But, setting this too high could result in increased monitoring overhead.
Please refer to the DAMON documentation ([Detailed Usages](usage.html)) for more detail.

Note that this must be 3 or higher. Please refer to the [Monitoring](../../../mm/damon/design.html#damon-design-monitoring) section of the design document for the rationale
behind this lower bound.

### max\_nr\_regions

Maximum number of monitoring regions.

The maximum number of monitoring regions of DAMON for the cold memory
monitoring. This can be used to set upper-bound of the monitoring overhead.
However, setting this too low could result in bad monitoring quality. Please
refer to the DAMON documentation ([Detailed Usages](usage.html)) for more detail.

### monitor\_region\_start

Start of target memory region in physical address.

The start physical address of memory region that DAMON\_RECLAIM will do work
against. That is, DAMON\_RECLAIM will find cold memory regions in this region
and reclaims. By default, biggest System RAM is used as the region.

### monitor\_region\_end

End of target memory region in physical address.

The end physical address of memory region that DAMON\_RECLAIM will do work
against. That is, DAMON\_RECLAIM will find cold memory regions in this region
and reclaims. By default, biggest System RAM is used as the region.

### addr\_unit

A scale factor for memory addresses and bytes.

This parameter is for setting and getting the [address unit](../../../mm/damon/design.html#damon-design-addr-unit) parameter of the DAMON instance for DAMON\_RECLAIM.

`monitor_region_start` and `monitor_region_end` should be provided in this
unit. For example, let’s suppose `addr_unit`, `monitor_region_start` and
`monitor_region_end` are set as `1024`, `0` and `10`, respectively.
Then DAMON\_RECLAIM will work for 10 KiB length of physical address range that
starts from address zero (`[0 * 1024, 10 * 1024)` in bytes).

`bytes_reclaim_tried_regions` and `bytes_reclaimed_regions` are also in
this unit. For example, let’s suppose values of `addr_unit`,
`bytes_reclaim_tried_regions` and `bytes_reclaimed_regions` are `1024`,
`42`, and `32`, respectively. Then it means DAMON\_RECLAIM tried to reclaim
42 KiB memory and successfully reclaimed 32 KiB memory in total.

If unsure, use only the default value (`1`) and forget about this.

### skip\_anon

Skip anonymous pages reclamation.

If this parameter is set as `Y`, DAMON\_RECLAIM does not reclaim anonymous
pages. By default, `N`.

### kdamond\_pid

PID of the DAMON thread.

If DAMON\_RECLAIM is enabled, this becomes the PID of the worker thread. Else,
-1.

### nr\_reclaim\_tried\_regions

Number of memory regions that tried to be reclaimed by DAMON\_RECLAIM.

### bytes\_reclaim\_tried\_regions

Total bytes of memory regions that tried to be reclaimed by DAMON\_RECLAIM.

### nr\_reclaimed\_regions

Number of memory regions that successfully be reclaimed by DAMON\_RECLAIM.

### bytes\_reclaimed\_regions

Total bytes of memory regions that successfully be reclaimed by DAMON\_RECLAIM.

### nr\_quota\_exceeds

Number of times that the time/space quota limits have exceeded.

## Example

Below runtime example commands make DAMON\_RECLAIM to find memory regions that
not accessed for 30 seconds or more and pages out. The reclamation is limited
to be done only up to 1 GiB per second to avoid DAMON\_RECLAIM consuming too
much CPU time for the paging out operation. It also asks DAMON\_RECLAIM to do
nothing if the system’s free memory rate is more than 50%, but start the real
works if it becomes lower than 40%. If DAMON\_RECLAIM doesn’t make progress and
therefore the free memory rate becomes lower than 20%, it asks DAMON\_RECLAIM to
do nothing again, so that we can fall back to the LRU-list based page
granularity reclamation.

```
# cd /sys/module/damon_reclaim/parameters
# echo 30000000 > min_age
# echo $((1 * 1024 * 1024 * 1024)) > quota_sz
# echo 1000 > quota_reset_interval_ms
# echo 500 > wmarks_high
# echo 400 > wmarks_mid
# echo 200 > wmarks_low
# echo Y > enabled
```

Note that this module (damon\_reclaim) cannot run simultaneously with other
DAMON-based special-purpose modules. Refer to [DAMON design special
purpose modules exclusivity](../../../mm/damon/design.html#damon-design-special-purpose-modules-exclusivity)
for more details.

[[1](#id1)]

<https://research.google/pubs/pub48551/>


[[2](#id2)]

<https://lwn.net/Articles/787611/>


[[3](#id3)]

<https://www.kernel.org/doc/html/latest/mm/free_page_reporting.html>
