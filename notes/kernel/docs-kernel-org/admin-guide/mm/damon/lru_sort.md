# DAMON-based LRU-lists Sorting

> 출처(원문): https://docs.kernel.org/admin-guide/mm/damon/lru_sort.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DAMON-based LRU-lists Sorting

DAMON-based LRU-lists Sorting (DAMON\_LRU\_SORT) is a static kernel module that
aimed to be used for proactive and lightweight data access pattern based
(de)prioritization of pages on their LRU-lists for making LRU-lists a more
trusworthy data access pattern source.

## Where Proactive LRU-lists Sorting is Required?

As page-granularity access checking overhead could be significant on huge
systems, LRU lists are normally not proactively sorted but partially and
reactively sorted for special events including specific user requests, system
calls and memory pressure. As a result, LRU lists are sometimes not so
perfectly prepared to be used as a trustworthy access pattern source for some
situations including reclamation target pages selection under sudden memory
pressure.

Because DAMON can identify access patterns of best-effort accuracy while
inducing only user-specified range of overhead, proactively running
DAMON\_LRU\_SORT could be helpful for making LRU lists more trustworthy access
pattern source with low and controlled overhead.

## How It Works?

DAMON\_LRU\_SORT finds hot pages (pages of memory regions that showing access
rates that higher than a user-specified threshold) and cold pages (pages of
memory regions that showing no access for a time that longer than a
user-specified threshold) using DAMON, and prioritizes hot pages while
deprioritizing cold pages on their LRU-lists. To avoid it consuming too much
CPU for the prioritizations, a CPU time usage limit can be configured. Under
the limit, it prioritizes and deprioritizes more hot and cold pages first,
respectively. System administrators can also configure under what situation
this scheme should automatically activated and deactivated with three memory
pressure watermarks.

Its default parameters for hotness/coldness thresholds and CPU quota limit are
conservatively chosen. That is, the module under its default parameters could
be widely used without harm for common situations while providing a level of
benefits for systems having clear hot/cold access patterns under memory
pressure while consuming only a limited small portion of CPU time.

## Interface: Module Parameters

To use this feature, you should first ensure your system is running on a kernel
that is built with `CONFIG_DAMON_LRU_SORT=y`.

To let sysadmins enable or disable it and tune for the given system,
DAMON\_LRU\_SORT utilizes module parameters. That is, you can put
`damon_lru_sort.<parameter>=<value>` on the kernel boot command line or write
proper values to `/sys/module/damon_lru_sort/parameters/<parameter>` files.

Below are the description of each parameter.

### enabled

Enable or disable DAMON\_LRU\_SORT.

You can enable DAMON\_LRU\_SORT by setting the value of this parameter as `Y`.
Setting it as `N` disables DAMON\_LRU\_SORT. Note that DAMON\_LRU\_SORT could do
no real monitoring and LRU-lists sorting due to the watermarks-based activation
condition. Refer to below descriptions for the watermarks parameter for this.

### commit\_inputs

Make DAMON\_LRU\_SORT reads the input parameters again, except `enabled`.

Input parameters that updated while DAMON\_LRU\_SORT is running are not applied
by default. Once this parameter is set as `Y`, DAMON\_LRU\_SORT reads values
of parametrs except `enabled` again. Once the re-reading is done, this
parameter is set as `N`. If invalid parameters are found while the
re-reading, DAMON\_LRU\_SORT will be disabled.

Once `Y` is written to this parameter, the user must not write to any
parameters until reading `commit_inputs` again returns `N`. If users
violate this rule, the kernel may exhibit undefined behavior.

### active\_mem\_bp

Desired active to [in]active memory ratio in bp (1/10,000).

While keeping the caps that set by other quotas, DAMON\_LRU\_SORT automatically
increases and decreases the effective level of the quota aiming the LRU
[de]prioritizations of the hot and cold memory resulting in this active to
[in]active memory ratio. Value zero means disabling this auto-tuning feature.

Disabled by default.

### autotune\_monitoring\_intervals

If this parameter is set as `Y`, DAMON\_LRU\_SORT automatically tunes DAMON’s
sampling and aggregation intervals. The auto-tuning aims to capture meaningful
amount of access events in each DAMON-snapshot, while keeping the sampling
interval 5 milliseconds in minimum, and 10 seconds in maximum. Setting this as
`N` disables the auto-tuning.

Disabled by default.

### filter\_young\_pages

Filter [non-]young pages accordingly for LRU [de]prioritizations.

If this is set, check page level access (youngness) once again before each
LRU [de]prioritization operation. LRU prioritization operation is skipped
if the page has not accessed since the last check (not young). LRU
deprioritization operation is skipped if the page has accessed since the
last check (young). The feature is enabled or disabled if this parameter is
set as `Y` or `N`, respectively.

Disabled by default.

### hot\_thres\_access\_freq

Access frequency threshold for hot memory regions identification in permil.

If a memory region is accessed in frequency of this or higher, DAMON\_LRU\_SORT
identifies the region as hot, and mark it as accessed on the LRU list, so that
it could not be reclaimed under memory pressure. 50% by default.

### cold\_min\_age

Time threshold for cold memory regions identification in microseconds.

If a memory region is not accessed for this or longer time, DAMON\_LRU\_SORT
identifies the region as cold, and mark it as unaccessed on the LRU list, so
that it could be reclaimed first under memory pressure. 120 seconds by
default.

### quota\_ms

Limit of time for trying the LRU lists sorting in milliseconds.

DAMON\_LRU\_SORT tries to use only up to this time within a time window
(quota\_reset\_interval\_ms) for trying LRU lists sorting. This can be used
for limiting CPU consumption of DAMON\_LRU\_SORT. If the value is zero, the
limit is disabled.

10 ms by default.

### quota\_reset\_interval\_ms

The time quota charge reset interval in milliseconds.

The charge reset interval for the quota of time (quota\_ms). That is,
DAMON\_LRU\_SORT does not try LRU-lists sorting for more than quota\_ms
milliseconds or quota\_sz bytes within quota\_reset\_interval\_ms milliseconds.

1 second by default.

### wmarks\_interval

The watermarks check time interval in microseconds.

Minimal time to wait before checking the watermarks, when DAMON\_LRU\_SORT is
enabled but inactive due to its watermarks rule. 5 seconds by default.

### wmarks\_high

Free memory rate (per thousand) for the high watermark.

If free memory of the system in bytes per thousand bytes is higher than this,
DAMON\_LRU\_SORT becomes inactive, so it does nothing but periodically checks the
watermarks. 200 (20%) by default.

### wmarks\_mid

Free memory rate (per thousand) for the middle watermark.

If free memory of the system in bytes per thousand bytes is between this and
the low watermark, DAMON\_LRU\_SORT becomes active, so starts the monitoring and
the LRU-lists sorting. 150 (15%) by default.

### wmarks\_low

Free memory rate (per thousand) for the low watermark.

If free memory of the system in bytes per thousand bytes is lower than this,
DAMON\_LRU\_SORT becomes inactive, so it does nothing but periodically checks the
watermarks. 50 (5%) by default.

### sample\_interval

Sampling interval for the monitoring in microseconds.

The sampling interval of DAMON for the cold memory monitoring. Please refer to
the DAMON documentation ([Detailed Usages](usage.html)) for more detail. 5ms by default.

### aggr\_interval

Aggregation interval for the monitoring in microseconds.

The aggregation interval of DAMON for the cold memory monitoring. Please
refer to the DAMON documentation ([Detailed Usages](usage.html)) for more detail. 100ms by
default.

### min\_nr\_regions

Minimum number of monitoring regions.

The minimal number of monitoring regions of DAMON for the cold memory
monitoring. This can be used to set lower-bound of the monitoring quality.
But, setting this too high could result in increased monitoring overhead.
Please refer to the DAMON documentation ([Detailed Usages](usage.html)) for more detail. 10 by
default.

Note that this must be 3 or higher. Please refer to the [Monitoring](../../../mm/damon/design.html#damon-design-monitoring) section of the design document for the rationale
behind this lower bound.

### max\_nr\_regions

Maximum number of monitoring regions.

The maximum number of monitoring regions of DAMON for the cold memory
monitoring. This can be used to set upper-bound of the monitoring overhead.
However, setting this too low could result in bad monitoring quality. Please
refer to the DAMON documentation ([Detailed Usages](usage.html)) for more detail. 1000 by
defaults.

### monitor\_region\_start

Start of target memory region in physical address.

The start physical address of memory region that DAMON\_LRU\_SORT will do work
against. By default, biggest System RAM is used as the region.

### monitor\_region\_end

End of target memory region in physical address.

The end physical address of memory region that DAMON\_LRU\_SORT will do work
against. By default, biggest System RAM is used as the region.

### addr\_unit

A scale factor for memory addresses and bytes.

This parameter is for setting and getting the [address unit](../../../mm/damon/design.html#damon-design-addr-unit) parameter of the DAMON instance for DAMON\_RECLAIM.

`monitor_region_start` and `monitor_region_end` should be provided in this
unit. For example, let’s suppose `addr_unit`, `monitor_region_start` and
`monitor_region_end` are set as `1024`, `0` and `10`, respectively.
Then DAMON\_LRU\_SORT will work for 10 KiB length of physical address range that
starts from address zero (`[0 * 1024, 10 * 1024)` in bytes).

Stat parameters having `bytes_` prefix are also in this unit. For example,
let’s suppose values of `addr_unit`, `bytes_lru_sort_tried_hot_regions` and
`bytes_lru_sorted_hot_regions` are `1024`, `42`, and `32`,
respectively. Then it means DAMON\_LRU\_SORT tried to LRU-sort 42 KiB of hot
memory and successfully LRU-sorted 32 KiB of the memory in total.

If unsure, use only the default value (`1`) and forget about this.

### kdamond\_pid

PID of the DAMON thread.

If DAMON\_LRU\_SORT is enabled, this becomes the PID of the worker thread. Else,
-1.

### nr\_lru\_sort\_tried\_hot\_regions

Number of hot memory regions that tried to be LRU-sorted.

### bytes\_lru\_sort\_tried\_hot\_regions

Total bytes of hot memory regions that tried to be LRU-sorted.

### nr\_lru\_sorted\_hot\_regions

Number of hot memory regions that successfully be LRU-sorted.

### bytes\_lru\_sorted\_hot\_regions

Total bytes of hot memory regions that successfully be LRU-sorted.

### nr\_hot\_quota\_exceeds

Number of times that the time quota limit for hot regions have exceeded.

### nr\_lru\_sort\_tried\_cold\_regions

Number of cold memory regions that tried to be LRU-sorted.

### bytes\_lru\_sort\_tried\_cold\_regions

Total bytes of cold memory regions that tried to be LRU-sorted.

### nr\_lru\_sorted\_cold\_regions

Number of cold memory regions that successfully be LRU-sorted.

### bytes\_lru\_sorted\_cold\_regions

Total bytes of cold memory regions that successfully be LRU-sorted.

### nr\_cold\_quota\_exceeds

Number of times that the time quota limit for cold regions have exceeded.

## Example

Below runtime example commands make DAMON\_LRU\_SORT to find memory regions
having >=50% access frequency and LRU-prioritize while LRU-deprioritizing
memory regions that not accessed for 120 seconds. The prioritization and
deprioritization is limited to be done using only up to 1% CPU time to avoid
DAMON\_LRU\_SORT consuming too much CPU time for the (de)prioritization. It also
asks DAMON\_LRU\_SORT to do nothing if the system’s free memory rate is more than
50%, but start the real works if it becomes lower than 40%. If DAMON\_RECLAIM
doesn’t make progress and therefore the free memory rate becomes lower than
20%, it asks DAMON\_LRU\_SORT to do nothing again, so that we can fall back to
the LRU-list based page granularity reclamation.

```
# cd /sys/module/damon_lru_sort/parameters
# echo 500 > hot_thres_access_freq
# echo 120000000 > cold_min_age
# echo 10 > quota_ms
# echo 1000 > quota_reset_interval_ms
# echo 500 > wmarks_high
# echo 400 > wmarks_mid
# echo 200 > wmarks_low
# echo Y > enabled
```

Note that this module (damon\_lru\_sort) cannot run simultaneously with other
DAMON-based special-purpose modules. Refer to [DAMON design special
purpose modules exclusivity](../../../mm/damon/design.html#damon-design-special-purpose-modules-exclusivity)
for more details.
