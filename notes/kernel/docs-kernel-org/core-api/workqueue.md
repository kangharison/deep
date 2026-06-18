# Workqueue

> 출처(원문): https://docs.kernel.org/core-api/workqueue.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Workqueue

Date:
:   September, 2010

Author:
:   Tejun Heo <[tj@kernel.org](mailto:tj%40kernel.org)>

Author:
:   Florian Mickler <[florian@mickler.org](mailto:florian%40mickler.org)>

## Introduction

There are many cases where an asynchronous process execution context
is needed and the workqueue (wq) API is the most commonly used
mechanism for such cases.

When such an asynchronous execution context is needed, a work item
describing which function to execute is put on a queue. An
independent thread serves as the asynchronous execution context. The
queue is called workqueue and the thread is called worker.

While there are work items on the workqueue the worker executes the
functions associated with the work items one after the other. When
there is no work item left on the workqueue the worker becomes idle.
When a new work item gets queued, the worker begins executing again.

## Why Concurrency Managed Workqueue?

In the original wq implementation, a multi threaded (MT) wq had one
worker thread per CPU and a single threaded (ST) wq had one worker
thread system-wide. A single MT wq needed to keep around the same
number of workers as the number of CPUs. The kernel grew a lot of MT
wq users over the years and with the number of CPU cores continuously
rising, some systems saturated the default 32k PID space just booting
up.

Although MT wq wasted a lot of resource, the level of concurrency
provided was unsatisfactory. The limitation was common to both ST and
MT wq albeit less severe on MT. Each wq maintained its own separate
worker pool. An MT wq could provide only one execution context per CPU
while an ST wq one for the whole system. Work items had to compete for
those very limited execution contexts leading to various problems
including proneness to deadlocks around the single execution context.

The tension between the provided level of concurrency and resource
usage also forced its users to make unnecessary tradeoffs like libata
choosing to use ST wq for polling PIOs and accepting an unnecessary
limitation that no two polling PIOs can progress at the same time. As
MT wq don’t provide much better concurrency, users which require
higher level of concurrency, like async or fscache, had to implement
their own thread pool.

Concurrency Managed Workqueue (cmwq) is a reimplementation of wq with
focus on the following goals.

* Maintain compatibility with the original workqueue API.
* Use per-CPU unified worker pools shared by all wq to provide
  flexible level of concurrency on demand without wasting a lot of
  resource.
* Automatically regulate worker pool and level of concurrency so that
  the API users don’t need to worry about such details.

## The Design

In order to ease the asynchronous execution of functions a new
abstraction, the work item, is introduced.

A work item is a simple struct that holds a pointer to the function
that is to be executed asynchronously. Whenever a driver or subsystem
wants a function to be executed asynchronously it has to set up a work
item pointing to that function and queue that work item on a
workqueue.

A work item can be executed in either a thread or the BH (softirq) context.

For threaded workqueues, special purpose threads, called [k]workers, execute
the functions off of the queue, one after the other. If no work is queued,
the worker threads become idle. These worker threads are managed in
worker-pools.

The cmwq design differentiates between the user-facing workqueues that
subsystems and drivers queue work items on and the backend mechanism
which manages worker-pools and processes the queued work items.

There are two worker-pools, one for normal work items and the other
for high priority ones, for each possible CPU and some extra
worker-pools to serve work items queued on unbound workqueues - the
number of these backing pools is dynamic.

BH workqueues use the same framework. However, as there can only be one
concurrent execution context, there’s no need to worry about concurrency.
Each per-CPU BH worker pool contains only one pseudo worker which represents
the BH execution context. A BH workqueue can be considered a convenience
interface to softirq.

Subsystems and drivers can create and queue work items through special
workqueue API functions as they see fit. They can influence some
aspects of the way the work items are executed by setting flags on the
workqueue they are putting the work item on. These flags include
things like CPU locality, concurrency limits, priority and more. To
get a detailed overview refer to the API description of
`alloc_workqueue()` below.

When a work item is queued to a workqueue, the target worker-pool is
determined according to the queue parameters and workqueue attributes
and appended on the shared worklist of the worker-pool. For example,
unless specifically overridden, a work item of a bound workqueue will
be queued on the worklist of either normal or highpri worker-pool that
is associated to the CPU the issuer is running on.

For any thread pool implementation, managing the concurrency level
(how many execution contexts are active) is an important issue. cmwq
tries to keep the concurrency at a minimal but sufficient level.
Minimal to save resources and sufficient in that the system is used at
its full capacity.

Each worker-pool bound to an actual CPU implements concurrency
management by hooking into the scheduler. The worker-pool is notified
whenever an active worker wakes up or sleeps and keeps track of the
number of the currently runnable workers. Generally, work items are
not expected to hog a CPU and consume many cycles. That means
maintaining just enough concurrency to prevent work processing from
stalling should be optimal. As long as there are one or more runnable
workers on the CPU, the worker-pool doesn’t start execution of a new
work, but, when the last running worker goes to sleep, it immediately
schedules a new worker so that the CPU doesn’t sit idle while there
are pending work items. This allows using a minimal number of workers
without losing execution bandwidth.

Keeping idle workers around doesn’t cost other than the memory space
for kthreads, so cmwq holds onto idle ones for a while before killing
them.

For unbound workqueues, the number of backing pools is dynamic.
Unbound workqueue can be assigned custom attributes using
`apply_workqueue_attrs()` and workqueue will automatically create
backing worker pools matching the attributes. The responsibility of
regulating concurrency level is on the users. There is also a flag to
mark a bound wq to ignore the concurrency management. Please refer to
the API section for details.

Forward progress guarantee relies on that workers can be created when
more execution contexts are necessary, which in turn is guaranteed
through the use of rescue workers. All work items which might be used
on code paths that handle memory reclaim are required to be queued on
wq’s that have a rescue-worker reserved for execution under memory
pressure. Else it is possible that the worker-pool deadlocks waiting
for execution contexts to free up.

## Application Programming Interface (API)

`alloc_workqueue()` allocates a wq. The original
`create_*workqueue()` functions are deprecated and scheduled for
removal. `alloc_workqueue()` takes three arguments - `@name`,
`@flags` and `@max_active`. `@name` is the name of the wq and
also used as the name of the rescuer thread if there is one.

A wq no longer manages execution resources but serves as a domain for
forward progress guarantee, flush and work item attributes. `@flags`
and `@max_active` control how work items are assigned execution
resources, scheduled and executed.

### `flags`

`WQ_BH`
:   BH workqueues can be considered a convenience interface to softirq. BH
    workqueues are always per-CPU and all BH work items are executed in the
    queueing CPU’s softirq context in the queueing order.

    All BH workqueues must have 0 `max_active` and `WQ_HIGHPRI` is the
    only allowed additional flag.

    BH work items cannot sleep. All other features such as delayed queueing,
    flushing and canceling are supported.

`WQ_PERCPU`
:   Work items queued to a per-cpu wq are bound to a specific CPU.
    This flag is the right choice when cpu locality is important.

    This flag is the complement of `WQ_UNBOUND`.

`WQ_UNBOUND`
:   Work items queued to an unbound wq are served by the special
    worker-pools which host workers which are not bound to any
    specific CPU. This makes the wq behave as a simple execution
    context provider without concurrency management. The unbound
    worker-pools try to start execution of work items as soon as
    possible. Unbound wq sacrifices locality but is useful for
    the following cases.

    * Wide fluctuation in the concurrency level requirement is
      expected and using bound wq may end up creating large number
      of mostly unused workers across different CPUs as the issuer
      hops through different CPUs.
    * Long running CPU intensive workloads which can be better
      managed by the system scheduler.

`WQ_FREEZABLE`
:   A freezable wq participates in the freeze phase of the system
    suspend operations. Work items on the wq are drained and no
    new work item starts execution until thawed.

`WQ_MEM_RECLAIM`
:   All wq which might be used in the memory reclaim paths **MUST**
    have this flag set. The wq is guaranteed to have at least one
    execution context regardless of memory pressure.

`WQ_HIGHPRI`
:   Work items of a highpri wq are queued to the highpri
    worker-pool of the target cpu. Highpri worker-pools are
    served by worker threads with elevated nice level.

    Note that normal and highpri worker-pools don’t interact with
    each other. Each maintains its separate pool of workers and
    implements concurrency management among its workers.

`WQ_CPU_INTENSIVE`
:   Work items of a CPU intensive wq do not contribute to the
    concurrency level. In other words, runnable CPU intensive
    work items will not prevent other work items in the same
    worker-pool from starting execution. This is useful for bound
    work items which are expected to hog CPU cycles so that their
    execution is regulated by the system scheduler.

    Although CPU intensive work items don’t contribute to the
    concurrency level, start of their executions is still
    regulated by the concurrency management and runnable
    non-CPU-intensive work items can delay execution of CPU
    intensive work items.

    This flag is meaningless for unbound wq.

### `max_active`

`@max_active` determines the maximum number of execution contexts per
CPU which can be assigned to the work items of a wq. For example, with
`@max_active` of 16, at most 16 work items of the wq can be executing
at the same time per CPU. This is always a per-CPU attribute, even for
unbound workqueues.

The maximum limit for `@max_active` is 2048 and the default value used
when 0 is specified is 1024. These values are chosen sufficiently high
such that they are not the limiting factor while providing protection in
runaway cases.

The number of active work items of a wq is usually regulated by the
users of the wq, more specifically, by how many work items the users
may queue at the same time. Unless there is a specific need for
throttling the number of active work items, specifying ‘0’ is
recommended.

Some users depend on strict execution ordering where only one work item
is in flight at any given time and the work items are processed in
queueing order. While the combination of `@max_active` of 1 and
`WQ_UNBOUND` used to achieve this behavior, this is no longer the
case. Use [`alloc_ordered_workqueue()`](#c.alloc_ordered_workqueue "alloc_ordered_workqueue") instead.

## Example Execution Scenarios

The following example execution scenarios try to illustrate how cmwq
behave under different configurations.

> Work items w0, w1, w2 are queued to a bound wq q0 on the same CPU.
> w0 burns CPU for 5ms then sleeps for 10ms then burns CPU for 5ms
> again before finishing. w1 and w2 burn CPU for 5ms then sleep for
> 10ms.

Ignoring all other tasks, works and processing overhead, and assuming
simple FIFO scheduling, the following is one highly simplified version
of possible sequences of events with the original wq.

```
TIME IN MSECS  EVENT
0              w0 starts and burns CPU
5              w0 sleeps
15             w0 wakes up and burns CPU
20             w0 finishes
20             w1 starts and burns CPU
25             w1 sleeps
35             w1 wakes up and finishes
35             w2 starts and burns CPU
40             w2 sleeps
50             w2 wakes up and finishes
```

And with cmwq with `@max_active` >= 3,

```
TIME IN MSECS  EVENT
0              w0 starts and burns CPU
5              w0 sleeps
5              w1 starts and burns CPU
10             w1 sleeps
10             w2 starts and burns CPU
15             w2 sleeps
15             w0 wakes up and burns CPU
20             w0 finishes
20             w1 wakes up and finishes
25             w2 wakes up and finishes
```

If `@max_active` == 2,

```
TIME IN MSECS  EVENT
0              w0 starts and burns CPU
5              w0 sleeps
5              w1 starts and burns CPU
10             w1 sleeps
15             w0 wakes up and burns CPU
20             w0 finishes
20             w1 wakes up and finishes
20             w2 starts and burns CPU
25             w2 sleeps
35             w2 wakes up and finishes
```

Now, let’s assume w1 and w2 are queued to a different wq q1 which has
`WQ_CPU_INTENSIVE` set,

```
TIME IN MSECS  EVENT
0              w0 starts and burns CPU
5              w0 sleeps
5              w1 and w2 start and burn CPU
10             w1 sleeps
15             w2 sleeps
15             w0 wakes up and burns CPU
20             w0 finishes
20             w1 wakes up and finishes
25             w2 wakes up and finishes
```

## Guidelines

* Do not forget to use `WQ_MEM_RECLAIM` if a wq may process work
  items which are used during memory reclaim. Each wq with
  `WQ_MEM_RECLAIM` set has an execution context reserved for it. If
  there is dependency among multiple work items used during memory
  reclaim, they should be queued to separate wq each with
  `WQ_MEM_RECLAIM`.
* Unless strict ordering is required, there is no need to use ST wq.
* Unless there is a specific need, using 0 for @max\_active is
  recommended. In most use cases, concurrency level usually stays
  well under the default limit.
* A wq serves as a domain for forward progress guarantee
  (`WQ_MEM_RECLAIM`, flush and work item attributes. Work items
  which are not involved in memory reclaim and don’t need to be
  flushed as a part of a group of work items, and don’t require any
  special attribute, can use one of the system wq. There is no
  difference in execution characteristics between using a dedicated wq
  and a system wq.

  Note: If something may generate more than @max\_active outstanding
  work items (do stress test your producers), it may saturate a system
  wq and potentially lead to deadlock. It should utilize its own
  dedicated workqueue rather than the system wq.
* Unless work items are expected to consume a huge amount of CPU
  cycles, using a bound wq is usually beneficial due to the increased
  level of locality in wq operations and work item execution.

## Affinity Scopes

An unbound workqueue groups CPUs according to its affinity scope to improve
cache locality. For example, if a workqueue is using the default affinity
scope of “cache\_shard”, it will group CPUs into sub-LLC shards. A work item
queued on the workqueue will be assigned to a worker on one of the CPUs
within the same shard as the issuing CPU.
Once started, the worker may or may not be allowed to move outside the scope
depending on the `affinity_strict` setting of the scope.

Workqueue currently supports the following affinity scopes.

`default`
:   Use the scope in module parameter `workqueue.default_affinity_scope`
    which is always set to one of the scopes below.

`cpu`
:   CPUs are not grouped. A work item issued on one CPU is processed by a
    worker on the same CPU. This makes unbound workqueues behave as per-cpu
    workqueues without concurrency management.

`smt`
:   CPUs are grouped according to SMT boundaries. This usually means that the
    logical threads of each physical CPU core are grouped together.

`cache`
:   CPUs are grouped according to cache boundaries. Which specific cache
    boundary is used is determined by the arch code. L3 is used in a lot of
    cases.

`cache_shard`
:   CPUs are grouped into sub-LLC shards of at most `wq_cache_shard_size`
    cores (default 8, tunable via the `workqueue.cache_shard_size` boot
    parameter). Shards are always split on core (SMT group) boundaries.
    This is the default affinity scope.

`numa`
:   CPUs are grouped according to NUMA boundaries.

`system`
:   All CPUs are put in the same group. Workqueue makes no effort to process a
    work item on a CPU close to the issuing CPU.

The default affinity scope can be changed with the module parameter
`workqueue.default_affinity_scope` and a specific workqueue’s affinity
scope can be changed using `apply_workqueue_attrs()`.

If `WQ_SYSFS` is set, the workqueue will have the following affinity scope
related interface files under its `/sys/devices/virtual/workqueue/WQ_NAME/`
directory.

`affinity_scope`
:   Read to see the current affinity scope. Write to change.

    When default is the current scope, reading this file will also show the
    current effective scope in parentheses, for example, `default (cache)`.

`affinity_strict`
:   0 by default indicating that affinity scopes are not strict. When a work
    item starts execution, workqueue makes a best-effort attempt to ensure
    that the worker is inside its affinity scope, which is called
    repatriation. Once started, the scheduler is free to move the worker
    anywhere in the system as it sees fit. This enables benefiting from scope
    locality while still being able to utilize other CPUs if necessary and
    available.

    If set to 1, all workers of the scope are guaranteed always to be in the
    scope. This may be useful when crossing affinity scopes has other
    implications, for example, in terms of power consumption or workload
    isolation. Strict NUMA scope can also be used to match the workqueue
    behavior of older kernels.

## Affinity Scopes and Performance

It’d be ideal if an unbound workqueue’s behavior is optimal for vast
majority of use cases without further tuning. Unfortunately, in the current
kernel, there exists a pronounced trade-off between locality and utilization
necessitating explicit configurations when workqueues are heavily used.

Higher locality leads to higher efficiency where more work is performed for
the same number of consumed CPU cycles. However, higher locality may also
cause lower overall system utilization if the work items are not spread
enough across the affinity scopes by the issuers. The following performance
testing with dm-crypt clearly illustrates this trade-off.

The tests are run on a CPU with 12-cores/24-threads split across four L3
caches (AMD Ryzen 9 3900x). CPU clock boost is turned off for consistency.
`/dev/dm-0` is a dm-crypt device created on NVME SSD (Samsung 990 PRO) and
opened with `cryptsetup` with default settings.

### Scenario 1: Enough issuers and work spread across the machine

The command used:

```
$ fio --filename=/dev/dm-0 --direct=1 --rw=randrw --bs=32k --ioengine=libaio \
  --iodepth=64 --runtime=60 --numjobs=24 --time_based --group_reporting \
  --name=iops-test-job --verify=sha512
```

There are 24 issuers, each issuing 64 IOs concurrently. `--verify=sha512`
makes `fio` generate and read back the content each time which makes
execution locality matter between the issuer and `kcryptd`. The following
are the read bandwidths and CPU utilizations depending on different affinity
scope settings on `kcryptd` measured over five runs. Bandwidths are in
MiBps, and CPU util in percents.

| Affinity | Bandwidth (MiBps) | CPU util (%) |
| --- | --- | --- |
| system | 1159.40 ±1.34 | 99.31 ±0.02 |
| cache | 1166.40 ±0.89 | 99.34 ±0.01 |
| cache (strict) | 1166.00 ±0.71 | 99.35 ±0.01 |

With enough issuers spread across the system, there is no downside to
“cache”, strict or otherwise. All three configurations saturate the whole
machine but the cache-affine ones outperform by 0.6% thanks to improved
locality.

### Scenario 2: Fewer issuers, enough work for saturation

The command used:

```
$ fio --filename=/dev/dm-0 --direct=1 --rw=randrw --bs=32k \
  --ioengine=libaio --iodepth=64 --runtime=60 --numjobs=8 \
  --time_based --group_reporting --name=iops-test-job --verify=sha512
```

The only difference from the previous scenario is `--numjobs=8`. There are
a third of the issuers but is still enough total work to saturate the
system.

| Affinity | Bandwidth (MiBps) | CPU util (%) |
| --- | --- | --- |
| system | 1155.40 ±0.89 | 97.41 ±0.05 |
| cache | 1154.40 ±1.14 | 96.15 ±0.09 |
| cache (strict) | 1112.00 ±4.64 | 93.26 ±0.35 |

This is more than enough work to saturate the system. Both “system” and
“cache” are nearly saturating the machine but not fully. “cache” is using
less CPU but the better efficiency puts it at the same bandwidth as
“system”.

Eight issuers moving around over four L3 cache scope still allow “cache
(strict)” to mostly saturate the machine but the loss of work conservation
is now starting to hurt with 3.7% bandwidth loss.

### Scenario 3: Even fewer issuers, not enough work to saturate

The command used:

```
$ fio --filename=/dev/dm-0 --direct=1 --rw=randrw --bs=32k \
  --ioengine=libaio --iodepth=64 --runtime=60 --numjobs=4 \
  --time_based --group_reporting --name=iops-test-job --verify=sha512
```

Again, the only difference is `--numjobs=4`. With the number of issuers
reduced to four, there now isn’t enough work to saturate the whole system
and the bandwidth becomes dependent on completion latencies.

| Affinity | Bandwidth (MiBps) | CPU util (%) |
| --- | --- | --- |
| system | 993.60 ±1.82 | 75.49 ±0.06 |
| cache | 973.40 ±1.52 | 74.90 ±0.07 |
| cache (strict) | 828.20 ±4.49 | 66.84 ±0.29 |

Now, the tradeoff between locality and utilization is clearer. “cache” shows
2% bandwidth loss compared to “system” and “cache (struct)” whopping 20%.

### Conclusion and Recommendations

In the above experiments, the efficiency advantage of the “cache” affinity
scope over “system” is, while consistent and noticeable, small. However, the
impact is dependent on the distances between the scopes and may be more
pronounced in processors with more complex topologies.

While the loss of work-conservation in certain scenarios hurts, it is a lot
better than “cache (strict)” and maximizing workqueue utilization is
unlikely to be the common case anyway. As such, “cache” is the default
affinity scope for unbound pools.

* As there is no one option which is great for most cases, workqueue usages
  that may consume a significant amount of CPU are recommended to configure
  the workqueues using `apply_workqueue_attrs()` and/or enable
  `WQ_SYSFS`.
* An unbound workqueue with strict “cpu” affinity scope behaves the same as
  `WQ_CPU_INTENSIVE` per-cpu workqueue. There is no real advanage to the
  latter and an unbound workqueue provides a lot more flexibility.
* Affinity scopes are introduced in Linux v6.5. To emulate the previous
  behavior, use strict “numa” affinity scope.
* The loss of work-conservation in non-strict affinity scopes is likely
  originating from the scheduler. There is no theoretical reason why the
  kernel wouldn’t be able to do the right thing and maintain
  work-conservation in most cases. As such, it is possible that future
  scheduler improvements may make most of these tunables unnecessary.

## Examining Configuration

Use tools/workqueue/wq\_dump.py to examine unbound CPU affinity
configuration, worker pools and how workqueues map to the pools:

```
$ tools/workqueue/wq_dump.py
Affinity Scopes
===============
wq_unbound_cpumask=0000000f

CPU
  nr_pods  4
  pod_cpus [0]=00000001 [1]=00000002 [2]=00000004 [3]=00000008
  pod_node [0]=0 [1]=0 [2]=1 [3]=1
  cpu_pod  [0]=0 [1]=1 [2]=2 [3]=3

SMT
  nr_pods  4
  pod_cpus [0]=00000001 [1]=00000002 [2]=00000004 [3]=00000008
  pod_node [0]=0 [1]=0 [2]=1 [3]=1
  cpu_pod  [0]=0 [1]=1 [2]=2 [3]=3

CACHE (default)
  nr_pods  2
  pod_cpus [0]=00000003 [1]=0000000c
  pod_node [0]=0 [1]=1
  cpu_pod  [0]=0 [1]=0 [2]=1 [3]=1

NUMA
  nr_pods  2
  pod_cpus [0]=00000003 [1]=0000000c
  pod_node [0]=0 [1]=1
  cpu_pod  [0]=0 [1]=0 [2]=1 [3]=1

SYSTEM
  nr_pods  1
  pod_cpus [0]=0000000f
  pod_node [0]=-1
  cpu_pod  [0]=0 [1]=0 [2]=0 [3]=0

Worker Pools
============
pool[00] ref= 1 nice=  0 idle/workers=  4/  4 cpu=  0
pool[01] ref= 1 nice=-20 idle/workers=  2/  2 cpu=  0
pool[02] ref= 1 nice=  0 idle/workers=  4/  4 cpu=  1
pool[03] ref= 1 nice=-20 idle/workers=  2/  2 cpu=  1
pool[04] ref= 1 nice=  0 idle/workers=  4/  4 cpu=  2
pool[05] ref= 1 nice=-20 idle/workers=  2/  2 cpu=  2
pool[06] ref= 1 nice=  0 idle/workers=  3/  3 cpu=  3
pool[07] ref= 1 nice=-20 idle/workers=  2/  2 cpu=  3
pool[08] ref=42 nice=  0 idle/workers=  6/  6 cpus=0000000f
pool[09] ref=28 nice=  0 idle/workers=  3/  3 cpus=00000003
pool[10] ref=28 nice=  0 idle/workers= 17/ 17 cpus=0000000c
pool[11] ref= 1 nice=-20 idle/workers=  1/  1 cpus=0000000f
pool[12] ref= 2 nice=-20 idle/workers=  1/  1 cpus=00000003
pool[13] ref= 2 nice=-20 idle/workers=  1/  1 cpus=0000000c

Workqueue CPU -> pool
=====================
[    workqueue \ CPU              0  1  2  3 dfl]
events                   percpu   0  2  4  6
events_highpri           percpu   1  3  5  7
events_long              percpu   0  2  4  6
events_unbound           unbound  9  9 10 10  8
events_freezable         percpu   0  2  4  6
events_power_efficient   percpu   0  2  4  6
events_freezable_pwr_ef  percpu   0  2  4  6
rcu_gp                   percpu   0  2  4  6
rcu_par_gp               percpu   0  2  4  6
slub_flushwq             percpu   0  2  4  6
netns                    ordered  8  8  8  8  8
...
```

See the command’s help message for more info.

## Monitoring

Use tools/workqueue/wq\_monitor.py to monitor workqueue operations:

```
$ tools/workqueue/wq_monitor.py events
                            total  infl  CPUtime  CPUhog CMW/RPR  mayday rescued
events                      18545     0      6.1       0       5       -       -
events_highpri                  8     0      0.0       0       0       -       -
events_long                     3     0      0.0       0       0       -       -
events_unbound              38306     0      0.1       -       7       -       -
events_freezable                0     0      0.0       0       0       -       -
events_power_efficient      29598     0      0.2       0       0       -       -
events_freezable_pwr_ef        10     0      0.0       0       0       -       -
sock_diag_events                0     0      0.0       0       0       -       -

                            total  infl  CPUtime  CPUhog CMW/RPR  mayday rescued
events                      18548     0      6.1       0       5       -       -
events_highpri                  8     0      0.0       0       0       -       -
events_long                     3     0      0.0       0       0       -       -
events_unbound              38322     0      0.1       -       7       -       -
events_freezable                0     0      0.0       0       0       -       -
events_power_efficient      29603     0      0.2       0       0       -       -
events_freezable_pwr_ef        10     0      0.0       0       0       -       -
sock_diag_events                0     0      0.0       0       0       -       -

...
```

See the command’s help message for more info.

## Debugging

Because the work functions are executed by generic worker threads
there are a few tricks needed to shed some light on misbehaving
workqueue users.

Worker threads show up in the process list as:

```
root      5671  0.0  0.0      0     0 ?        S    12:07   0:00 [kworker/0:1]
root      5672  0.0  0.0      0     0 ?        S    12:07   0:00 [kworker/1:2]
root      5673  0.0  0.0      0     0 ?        S    12:12   0:00 [kworker/0:0]
root      5674  0.0  0.0      0     0 ?        S    12:13   0:00 [kworker/1:0]
```

If kworkers are going crazy (using too much cpu), there are two types
of possible problems:

> 1. Something being scheduled in rapid succession
> 2. A single work item that consumes lots of cpu cycles

The first one can be tracked using tracing:

```
$ echo workqueue:workqueue_queue_work > /sys/kernel/tracing/set_event
$ cat /sys/kernel/tracing/trace_pipe > out.txt
(wait a few secs)
^C
```

If something is busy looping on work queueing, it would be dominating
the output and the offender can be determined with the work item
function.

For the second type of problems it should be possible to just check
the stack trace of the offending worker thread.

```
$ cat /proc/THE_OFFENDING_KWORKER/stack
```

The work item’s function should be trivially visible in the stack
trace.

## Non-reentrance Conditions

Workqueue guarantees that a work item cannot be re-entrant if the following
conditions hold after a work item gets queued:

> 1. The work function hasn’t been changed.
> 2. No one queues the work item to another workqueue.
> 3. The work item hasn’t been reinitiated.

In other words, if the above conditions hold, the work item is guaranteed to be
executed by at most one worker system-wide at any given time.

Note that requeuing the work item (to the same queue) in the self function
doesn’t break these conditions, so it’s safe to do. Otherwise, caution is
required when breaking the conditions inside a work function.

## Kernel Inline Documentations Reference

struct workqueue\_attrs
:   A struct for workqueue attributes.

**Definition**:

```
struct workqueue_attrs {
    int nice;
    cpumask_var_t cpumask;
    cpumask_var_t __pod_cpumask;
    bool affn_strict;
    enum wq_affn_scope affn_scope;
    bool ordered;
};
```

**Members**

`nice`
:   nice level

`cpumask`
:   allowed CPUs

    Work items in this workqueue are affine to these CPUs and not allowed
    to execute on other CPUs. A pool serving a workqueue must have the
    same **cpumask**.

`__pod_cpumask`
:   internal attribute used to create per-pod pools

    Internal use only.

    Per-pod unbound worker pools are used to improve locality. Always a
    subset of ->cpumask. A workqueue can be associated with multiple
    worker pools with disjoint **\_\_pod\_cpumask**’s. Whether the enforcement
    of a pool’s **\_\_pod\_cpumask** is strict depends on **affn\_strict**.

`affn_strict`
:   affinity scope is strict

    If clear, workqueue will make a best-effort attempt at starting the
    worker inside **\_\_pod\_cpumask** but the scheduler is free to migrate it
    outside.

    If set, workers are only allowed to run inside **\_\_pod\_cpumask**.

`affn_scope`
:   unbound CPU affinity scope

    CPU pods are used to improve execution locality of unbound work
    items. There are multiple pod types, one for each wq\_affn\_scope, and
    every CPU in the system belongs to one pod in every pod type. CPUs
    that belong to the same pod share the worker pool. For example,
    selecting `WQ_AFFN_NUMA` makes the workqueue use a separate worker
    pool for each NUMA node.

`ordered`
:   work items must be executed one by one in queueing order

**Description**

This can be used to change attributes of an unbound workqueue.

work\_pending

`work_pending (work)`

> Find out whether a work item is currently pending

**Parameters**

`work`
:   The work item in question

delayed\_work\_pending

`delayed_work_pending (w)`

> Find out whether a delayable work item is currently pending

**Parameters**

`w`
:   The work item in question

struct workqueue\_struct \*alloc\_workqueue(const char \*fmt, unsigned int flags, int max\_active, ...)
:   allocate a workqueue

**Parameters**

`const char *fmt`
:   printf format for the name of the workqueue

`unsigned int flags`
:   WQ\_\* flags

`int max_active`
:   max in-flight work items, 0 for default

`...`
:   args for **fmt**

**Description**

For a per-cpu workqueue, **max\_active** limits the number of in-flight work
items for each CPU. e.g. **max\_active** of 1 indicates that each CPU can be
executing at most one work item for the workqueue.

For unbound workqueues, **max\_active** limits the number of in-flight work items
for the whole system. e.g. **max\_active** of 16 indicates that there can be
at most 16 work items executing for the workqueue in the whole system.

As sharing the same active counter for an unbound workqueue across multiple
NUMA nodes can be expensive, **max\_active** is distributed to each NUMA node
according to the proportion of the number of online CPUs and enforced
independently.

Depending on online CPU distribution, a node may end up with per-node
max\_active which is significantly lower than **max\_active**, which can lead to
deadlocks if the per-node concurrency limit is lower than the maximum number
of interdependent work items for the workqueue.

To guarantee forward progress regardless of online CPU distribution, the
concurrency limit on every node is guaranteed to be equal to or greater than
min\_active which is set to min(**max\_active**, `WQ_DFL_MIN_ACTIVE`). This means
that the sum of per-node max\_active’s may be larger than **max\_active**.

For detailed information on `WQ_`\* flags, please refer to
[Workqueue](#).

**Return**

Pointer to the allocated workqueue on success, `NULL` on failure.

struct workqueue\_struct \*devm\_alloc\_workqueue(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*fmt, unsigned int flags, int max\_active, ...)
:   Resource-managed allocate a workqueue

**Parameters**

`struct device *dev`
:   Device to allocate workqueue for

`const char *fmt`
:   printf format for the name of the workqueue

`unsigned int flags`
:   WQ\_\* flags

`int max_active`
:   max in-flight work items, 0 for default

`...`
:   args for **fmt**

**Description**

Resource managed workqueue, see [`alloc_workqueue()`](#c.alloc_workqueue "alloc_workqueue") for details.

The workqueue will be automatically destroyed on driver detach. Typically
this should be used in drivers already relying on devm interafaces.

**Return**

Pointer to the allocated workqueue on success, `NULL` on failure.

struct workqueue\_struct \*alloc\_workqueue\_lockdep\_map(const char \*fmt, unsigned int flags, int max\_active, struct [lockdep\_map](#c.alloc_workqueue_lockdep_map "lockdep_map") \*lockdep\_map, ...)
:   allocate a workqueue with user-defined lockdep\_map

**Parameters**

`const char *fmt`
:   printf format for the name of the workqueue

`unsigned int flags`
:   WQ\_\* flags

`int max_active`
:   max in-flight work items, 0 for default

`struct lockdep_map *lockdep_map`
:   user-defined lockdep\_map

`...`
:   args for **fmt**

**Description**

Same as alloc\_workqueue but with the a user-define lockdep\_map. Useful for
workqueues created with the same purpose and to avoid leaking a lockdep\_map
on each workqueue creation.

**Return**

Pointer to the allocated workqueue on success, `NULL` on failure.

alloc\_ordered\_workqueue\_lockdep\_map

`alloc_ordered_workqueue_lockdep_map (fmt, flags, lockdep_map, args...)`

> allocate an ordered workqueue with user-defined lockdep\_map

**Parameters**

`fmt`
:   printf format for the name of the workqueue

`flags`
:   WQ\_\* flags (only WQ\_FREEZABLE and WQ\_MEM\_RECLAIM are meaningful)

`lockdep_map`
:   user-defined lockdep\_map

`args...`
:   args for **fmt**

**Description**

Same as alloc\_ordered\_workqueue but with the a user-define lockdep\_map.
Useful for workqueues created with the same purpose and to avoid leaking a
lockdep\_map on each workqueue creation.

**Return**

Pointer to the allocated workqueue on success, `NULL` on failure.

alloc\_ordered\_workqueue

`alloc_ordered_workqueue (fmt, flags, args...)`

> allocate an ordered workqueue

**Parameters**

`fmt`
:   printf format for the name of the workqueue

`flags`
:   WQ\_\* flags (only WQ\_FREEZABLE and WQ\_MEM\_RECLAIM are meaningful)

`args...`
:   args for **fmt**

**Description**

Allocate an ordered workqueue. An ordered workqueue executes at
most one work item at any given time in the queued order. They are
implemented as unbound workqueues with **max\_active** of one.

**Return**

Pointer to the allocated workqueue on success, `NULL` on failure.

bool queue\_work(struct workqueue\_struct \*wq, struct work\_struct \*work)
:   queue work on a workqueue

**Parameters**

`struct workqueue_struct *wq`
:   workqueue to use

`struct work_struct *work`
:   work to queue

**Description**

Returns `false` if **work** was already on a queue, `true` otherwise.

We queue the work to the CPU on which it was submitted, but if the CPU dies
it can be processed by another CPU.

Memory-ordering properties: If it returns `true`, guarantees that all stores
preceding the call to [`queue_work()`](#c.queue_work "queue_work") in the program order will be visible from
the CPU which will execute **work** by the time such work executes, e.g.,

{ x is initially 0 }

> CPU0 CPU1
>
> WRITE\_ONCE(x, 1); [ **work** is being executed ]
> r0 = queue\_work(wq, work); r1 = READ\_ONCE(x);

Forbids: r0 == true && r1 == 0

bool queue\_delayed\_work(struct workqueue\_struct \*wq, struct delayed\_work \*dwork, unsigned long delay)
:   queue work on a workqueue after delay

**Parameters**

`struct workqueue_struct *wq`
:   workqueue to use

`struct delayed_work *dwork`
:   delayable work to queue

`unsigned long delay`
:   number of jiffies to wait before queueing

**Description**

Equivalent to [`queue_delayed_work_on()`](#c.queue_delayed_work_on "queue_delayed_work_on") but tries to use the local CPU.

bool mod\_delayed\_work(struct workqueue\_struct \*wq, struct delayed\_work \*dwork, unsigned long delay)
:   modify delay of or queue a delayed work

**Parameters**

`struct workqueue_struct *wq`
:   workqueue to use

`struct delayed_work *dwork`
:   work to queue

`unsigned long delay`
:   number of jiffies to wait before queueing

**Description**

[`mod_delayed_work_on()`](#c.mod_delayed_work_on "mod_delayed_work_on") on local CPU.

bool schedule\_work\_on(int cpu, struct work\_struct \*work)
:   put work task on a specific cpu

**Parameters**

`int cpu`
:   cpu to put the work task on

`struct work_struct *work`
:   job to be done

**Description**

This puts a job on a specific cpu

bool schedule\_work(struct work\_struct \*work)
:   put work task in per-CPU workqueue

**Parameters**

`struct work_struct *work`
:   job to be done

**Description**

Returns `false` if **work** was already on the system per-CPU workqueue and
`true` otherwise.

This puts a job in the system per-CPU workqueue if it was not already
queued and leaves it in the same position on the system per-CPU
workqueue otherwise.

Shares the same memory-ordering properties of [`queue_work()`](#c.queue_work "queue_work"), cf. the
DocBook header of [`queue_work()`](#c.queue_work "queue_work").

bool enable\_and\_queue\_work(struct workqueue\_struct \*wq, struct work\_struct \*work)
:   Enable and queue a work item on a specific workqueue

**Parameters**

`struct workqueue_struct *wq`
:   The target workqueue

`struct work_struct *work`
:   The work item to be enabled and queued

**Description**

This function combines the operations of [`enable_work()`](#c.enable_work "enable_work") and [`queue_work()`](#c.queue_work "queue_work"),
providing a convenient way to enable and queue a work item in a single call.
It invokes [`enable_work()`](#c.enable_work "enable_work") on **work** and then queues it if the disable depth
reached 0. Returns `true` if the disable depth reached 0 and **work** is queued,
and `false` otherwise.

Note that **work** is always queued when disable depth reaches zero. If the
desired behavior is queueing only if certain events took place while **work** is
disabled, the user should implement the necessary state tracking and perform
explicit conditional queueing after [`enable_work()`](#c.enable_work "enable_work").

bool schedule\_delayed\_work\_on(int cpu, struct delayed\_work \*dwork, unsigned long delay)
:   queue work in per-CPU workqueue on CPU after delay

**Parameters**

`int cpu`
:   cpu to use

`struct delayed_work *dwork`
:   job to be done

`unsigned long delay`
:   number of jiffies to wait

**Description**

After waiting for a given time this puts a job in the system per-CPU
workqueue on the specified CPU.

bool schedule\_delayed\_work(struct delayed\_work \*dwork, unsigned long delay)
:   put work task in per-CPU workqueue after delay

**Parameters**

`struct delayed_work *dwork`
:   job to be done

`unsigned long delay`
:   number of jiffies to wait or 0 for immediate execution

**Description**

After waiting for a given time this puts a job in the system per-CPU
workqueue.

for\_each\_pool

`for_each_pool (pool, pi)`

> iterate through all worker\_pools in the system

**Parameters**

`pool`
:   iteration cursor

`pi`
:   integer used for iteration

**Description**

This must be called either with wq\_pool\_mutex held or RCU read
locked. If the pool needs to be used beyond the locking in effect, the
caller is responsible for guaranteeing that the pool stays online.

The if/else clause exists only for the lockdep assertion and can be
ignored.

for\_each\_pool\_worker

`for_each_pool_worker (worker, pool)`

> iterate through all workers of a worker\_pool

**Parameters**

`worker`
:   iteration cursor

`pool`
:   worker\_pool to iterate workers of

**Description**

This must be called with wq\_pool\_attach\_mutex.

The if/else clause exists only for the lockdep assertion and can be
ignored.

for\_each\_pwq

`for_each_pwq (pwq, wq)`

> iterate through all pool\_workqueues of the specified workqueue

**Parameters**

`pwq`
:   iteration cursor

`wq`
:   the target workqueue

**Description**

This must be called either with wq->mutex held or RCU read locked.
If the pwq needs to be used beyond the locking in effect, the caller is
responsible for guaranteeing that the pwq stays online.

The if/else clause exists only for the lockdep assertion and can be
ignored.

int worker\_pool\_assign\_id(struct worker\_pool \*pool)
:   allocate ID and assign it to **pool**

**Parameters**

`struct worker_pool *pool`
:   the pool pointer of interest

**Description**

Returns 0 if ID in [0, WORK\_OFFQ\_POOL\_NONE) is allocated and assigned
successfully, -errno on failure.

struct cpumask \*unbound\_effective\_cpumask(struct workqueue\_struct \*wq)
:   effective cpumask of an unbound workqueue

**Parameters**

`struct workqueue_struct *wq`
:   workqueue of interest

**Description**

**wq->unbound\_attrs->cpumask** contains the cpumask requested by the user which
is masked with wq\_unbound\_cpumask to determine the effective cpumask. The
default pwq is always mapped to the pool with the current effective cpumask.

struct worker\_pool \*get\_work\_pool(struct work\_struct \*work)
:   return the worker\_pool a given work was associated with

**Parameters**

`struct work_struct *work`
:   the work item of interest

**Description**

Pools are created and destroyed under wq\_pool\_mutex, and allows read
access under RCU read lock. As such, this function should be
called under wq\_pool\_mutex or inside of a [`rcu_read_lock()`](kernel-api.html#c.rcu_read_lock "rcu_read_lock") region.

All fields of the returned pool are accessible as long as the above
mentioned locking is in effect. If the returned pool needs to be used
beyond the critical section, the caller is responsible for ensuring the
returned pool is and stays online.

**Return**

The worker\_pool **work** was last associated with. `NULL` if none.

void worker\_set\_flags(struct [worker](#c.worker_set_flags "worker") \*worker, unsigned int flags)
:   set worker flags and adjust nr\_running accordingly

**Parameters**

`struct worker *worker`
:   self

`unsigned int flags`
:   flags to set

**Description**

Set **flags** in **worker->flags** and adjust nr\_running accordingly.

void worker\_clr\_flags(struct [worker](#c.worker_clr_flags "worker") \*worker, unsigned int flags)
:   clear worker flags and adjust nr\_running accordingly

**Parameters**

`struct worker *worker`
:   self

`unsigned int flags`
:   flags to clear

**Description**

Clear **flags** in **worker->flags** and adjust nr\_running accordingly.

void worker\_enter\_idle(struct [worker](#c.worker_enter_idle "worker") \*worker)
:   enter idle state

**Parameters**

`struct worker *worker`
:   worker which is entering idle state

**Description**

**worker** is entering idle state. Update stats and idle timer if
necessary.

LOCKING:
raw\_spin\_lock\_irq(pool->lock).

void worker\_leave\_idle(struct [worker](#c.worker_leave_idle "worker") \*worker)
:   leave idle state

**Parameters**

`struct worker *worker`
:   worker which is leaving idle state

**Description**

**worker** is leaving idle state. Update stats.

LOCKING:
raw\_spin\_lock\_irq(pool->lock).

struct worker \*find\_worker\_executing\_work(struct worker\_pool \*pool, struct work\_struct \*work)
:   find worker which is executing a work

**Parameters**

`struct worker_pool *pool`
:   pool of interest

`struct work_struct *work`
:   work to find worker for

**Description**

Find a worker which is executing **work** on **pool** by searching
**pool->busy\_hash** which is keyed by the address of **work**. For a worker
to match, its current execution should match the address of **work** and
its work function. This is to avoid unwanted dependency between
unrelated work executions through a work item being recycled while still
being executed.

This is a bit tricky. A work item may be freed once its execution
starts and nothing prevents the freed area from being recycled for
another work item. If the same work item address ends up being reused
before the original execution finishes, workqueue will identify the
recycled work item as currently executing and make it wait until the
current execution finishes, introducing an unwanted dependency.

This function checks the work item address and work function to avoid
false positives. Note that this isn’t complete as one may construct a
work function which can introduce dependency onto itself through a
recycled work item. Well, if somebody wants to shoot oneself in the
foot that badly, there’s only so much we can do, and if such deadlock
actually occurs, it should be easy to locate the culprit work function.

**Context**

raw\_spin\_lock\_irq(pool->lock).

**Return**

Pointer to worker which is executing **work** if found, `NULL`
otherwise.

void move\_linked\_works(struct work\_struct \*work, struct list\_head \*head, struct work\_struct \*\*nextp)
:   move linked works to a list

**Parameters**

`struct work_struct *work`
:   start of series of works to be scheduled

`struct list_head *head`
:   target list to append **work** to

`struct work_struct **nextp`
:   out parameter for nested worklist walking

**Description**

Schedule linked works starting from **work** to **head**. Work series to be
scheduled starts at **work** and includes any consecutive work with
WORK\_STRUCT\_LINKED set in its predecessor. See [`assign_work()`](#c.assign_work "assign_work") for details on
**nextp**.

**Context**

raw\_spin\_lock\_irq(pool->lock).

bool assign\_work(struct work\_struct \*work, struct [worker](#c.assign_work "worker") \*worker, struct work\_struct \*\*nextp)
:   assign a work item and its linked work items to a worker

**Parameters**

`struct work_struct *work`
:   work to assign

`struct worker *worker`
:   worker to assign to

`struct work_struct **nextp`
:   out parameter for nested worklist walking

**Description**

Assign **work** and its linked work items to **worker**. If **work** is already being
executed by another worker in the same pool, it’ll be punted there.

If **nextp** is not NULL, it’s updated to point to the next work of the last
scheduled work. This allows [`assign_work()`](#c.assign_work "assign_work") to be nested inside
[`list_for_each_entry_safe()`](list.html#c.list_for_each_entry_safe "list_for_each_entry_safe").

Returns `true` if **work** was successfully assigned to **worker**. `false` if **work**
was punted to another worker already executing it.

bool kick\_pool(struct worker\_pool \*pool)
:   wake up an idle worker if necessary

**Parameters**

`struct worker_pool *pool`
:   pool to kick

**Description**

**pool** may have pending work items. Wake up worker if necessary. Returns
whether a worker was woken up.

void wq\_worker\_running(struct task\_struct \*task)
:   a worker is running again

**Parameters**

`struct task_struct *task`
:   task waking up

**Description**

This function is called when a worker returns from `schedule()`

void wq\_worker\_sleeping(struct task\_struct \*task)
:   a worker is going to sleep

**Parameters**

`struct task_struct *task`
:   task going to sleep

**Description**

This function is called from `schedule()` when a busy worker is
going to sleep.

void wq\_worker\_tick(struct task\_struct \*task)
:   a scheduler tick occurred while a kworker is running

**Parameters**

`struct task_struct *task`
:   task currently running

**Description**

Called from `sched_tick()`. We’re in the IRQ context and the current
worker’s fields which follow the ‘K’ locking rule can be accessed safely.

work\_func\_t wq\_worker\_last\_func(struct task\_struct \*task)
:   retrieve worker’s last work function

**Parameters**

`struct task_struct *task`
:   Task to retrieve last work function of.

**Description**

Determine the last function a worker executed. This is called from
the scheduler to get a worker’s last known identity.

This function is called during `schedule()` when a kworker is going
to sleep. It’s used by psi to identify aggregation workers during
dequeuing, to allow periodic aggregation to shut-off when that
worker is the last task in the system or cgroup to go to sleep.

As this function doesn’t involve any workqueue-related locking, it
only returns stable values when called from inside the scheduler’s
queuing and dequeuing paths, when **task**, which must be a kworker,
is guaranteed to not be processing any works.

**Context**

raw\_spin\_lock\_irq(rq->lock)

**Return**

The last work function `current` executed as a worker, NULL if it
hasn’t executed any work yet.

struct [wq\_node\_nr\_active](#c.wq_node_nr_active "wq_node_nr_active") \*wq\_node\_nr\_active(struct workqueue\_struct \*wq, int node)
:   Determine wq\_node\_nr\_active to use

**Parameters**

`struct workqueue_struct *wq`
:   workqueue of interest

`int node`
:   NUMA node, can be `NUMA_NO_NODE`

**Description**

Determine wq\_node\_nr\_active to use for **wq** on **node**. Returns:

* `NULL` for per-cpu workqueues as they don’t need to use shared nr\_active.
* node\_nr\_active[nr\_node\_ids] if **node** is `NUMA_NO_NODE`.
* Otherwise, node\_nr\_active[**node**].

void wq\_update\_node\_max\_active(struct workqueue\_struct \*wq, int off\_cpu)
:   Update per-node max\_actives to use

**Parameters**

`struct workqueue_struct *wq`
:   workqueue to update

`int off_cpu`
:   CPU that’s going down, -1 if a CPU is not going down

**Description**

Update **wq->node\_nr\_active\*\*[]->max. \*\*wq** must be unbound. max\_active is
distributed among nodes according to the proportions of numbers of online
cpus. The result is always between **wq->min\_active** and max\_active.

void get\_pwq(struct pool\_workqueue \*pwq)
:   get an extra reference on the specified pool\_workqueue

**Parameters**

`struct pool_workqueue *pwq`
:   pool\_workqueue to get

**Description**

Obtain an extra reference on **pwq**. The caller should guarantee that
**pwq** has positive refcnt and be holding the matching pool->lock.

void put\_pwq(struct pool\_workqueue \*pwq)
:   put a pool\_workqueue reference

**Parameters**

`struct pool_workqueue *pwq`
:   pool\_workqueue to put

**Description**

Drop a reference of **pwq**. If its refcnt reaches zero, schedule its
destruction. The caller should be holding the matching pool->lock.

void put\_pwq\_unlocked(struct pool\_workqueue \*pwq)
:   [`put_pwq()`](#c.put_pwq "put_pwq") with surrounding pool lock/unlock

**Parameters**

`struct pool_workqueue *pwq`
:   pool\_workqueue to put (can be `NULL`)

**Description**

[`put_pwq()`](#c.put_pwq "put_pwq") with locking. This function also allows `NULL` **pwq**.

bool pwq\_tryinc\_nr\_active(struct pool\_workqueue \*pwq, bool fill)
:   Try to increment nr\_active for a pwq

**Parameters**

`struct pool_workqueue *pwq`
:   pool\_workqueue of interest

`bool fill`
:   max\_active may have increased, try to increase concurrency level

**Description**

Try to increment nr\_active for **pwq**. Returns `true` if an nr\_active count is
successfully obtained. `false` otherwise.

bool pwq\_activate\_first\_inactive(struct pool\_workqueue \*pwq, bool fill)
:   Activate the first inactive work item on a pwq

**Parameters**

`struct pool_workqueue *pwq`
:   pool\_workqueue of interest

`bool fill`
:   max\_active may have increased, try to increase concurrency level

**Description**

Activate the first inactive work item of **pwq** if available and allowed by
max\_active limit.

Returns `true` if an inactive work item has been activated. `false` if no
inactive work item is found or max\_active limit is reached.

void unplug\_oldest\_pwq(struct workqueue\_struct \*wq)
:   unplug the oldest pool\_workqueue

**Parameters**

`struct workqueue_struct *wq`
:   workqueue\_struct where its oldest pwq is to be unplugged

**Description**

This function should only be called for ordered workqueues where only the
oldest pwq is unplugged, the others are plugged to suspend execution to
ensure proper work item ordering:

```
dfl_pwq --------------+     [P] - plugged
                      |
                      v
pwqs -> A -> B [P] -> C [P] (newest)
        |    |        |
        1    3        5
        |    |        |
        2    4        6
```

When the oldest pwq is drained and removed, this function should be called
to unplug the next oldest one to start its work item execution. Note that
pwq’s are linked into wq->pwqs with the oldest first, so the first one in
the list is the oldest.

void node\_activate\_pending\_pwq(struct [wq\_node\_nr\_active](#c.wq_node_nr_active "wq_node_nr_active") \*nna, struct worker\_pool \*caller\_pool)
:   Activate a pending pwq on a wq\_node\_nr\_active

**Parameters**

`struct wq_node_nr_active *nna`
:   wq\_node\_nr\_active to activate a pending pwq for

`struct worker_pool *caller_pool`
:   worker\_pool the caller is locking

**Description**

Activate a pwq in **nna->pending\_pwqs**. Called with **caller\_pool** locked.
**caller\_pool** may be unlocked and relocked to lock other worker\_pools.

void pwq\_dec\_nr\_active(struct pool\_workqueue \*pwq)
:   Retire an active count

**Parameters**

`struct pool_workqueue *pwq`
:   pool\_workqueue of interest

**Description**

Decrement **pwq**’s nr\_active and try to activate the first inactive work item.
For unbound workqueues, this function may temporarily drop **pwq->pool->lock**.

void pwq\_dec\_nr\_in\_flight(struct pool\_workqueue \*pwq, unsigned long work\_data)
:   decrement pwq’s nr\_in\_flight

**Parameters**

`struct pool_workqueue *pwq`
:   pwq of interest

`unsigned long work_data`
:   work\_data of work which left the queue

**Description**

A work either has completed or is removed from pending queue,
decrement nr\_in\_flight of its pwq and handle workqueue flushing.

**NOTE**

For unbound workqueues, this function may temporarily drop **pwq->pool->lock**
and thus should be called after all other state updates for the in-flight
work item is complete.

**Context**

raw\_spin\_lock\_irq(pool->lock).

int try\_to\_grab\_pending(struct work\_struct \*work, u32 cflags, unsigned long \*irq\_flags)
:   steal work item from worklist and disable irq

**Parameters**

`struct work_struct *work`
:   work item to steal

`u32 cflags`
:   `WORK_CANCEL_` flags

`unsigned long *irq_flags`
:   place to store irq state

**Description**

Try to grab PENDING bit of **work**. This function can handle **work** in any
stable state - idle, on timer or on worklist.

> |  |  |
> | --- | --- |
> | 1 | if **work** was pending and we successfully stole PENDING |
> | 0 | if **work** was idle and we claimed PENDING |
> | -EAGAIN | if PENDING couldn’t be grabbed at the moment, safe to busy-retry |

**Note**

On >= 0 return, the caller owns **work**’s PENDING bit. To avoid getting
interrupted while holding PENDING and **work** off queue, irq must be
disabled on entry. This, combined with delayed\_work->timer being
irqsafe, ensures that we return -EAGAIN for finite short period of time.

On successful return, >= 0, irq is disabled and the caller is
responsible for releasing it using local\_irq\_restore(**\*irq\_flags**).

This function is safe to call from any context including IRQ handler.

bool work\_grab\_pending(struct work\_struct \*work, u32 cflags, unsigned long \*irq\_flags)
:   steal work item from worklist and disable irq

**Parameters**

`struct work_struct *work`
:   work item to steal

`u32 cflags`
:   `WORK_CANCEL_` flags

`unsigned long *irq_flags`
:   place to store IRQ state

**Description**

Grab PENDING bit of **work**. **work** can be in any stable state - idle, on timer
or on worklist.

Can be called from any context. IRQ is disabled on return with IRQ state
stored in **\*irq\_flags**. The caller is responsible for re-enabling it using
`local_irq_restore()`.

Returns `true` if **work** was pending. `false` if idle.

void insert\_work(struct pool\_workqueue \*pwq, struct work\_struct \*work, struct list\_head \*head, unsigned int extra\_flags)
:   insert a work into a pool

**Parameters**

`struct pool_workqueue *pwq`
:   pwq **work** belongs to

`struct work_struct *work`
:   work to insert

`struct list_head *head`
:   insertion point

`unsigned int extra_flags`
:   extra WORK\_STRUCT\_\* flags to set

**Description**

Insert **work** which belongs to **pwq** after **head**. **extra\_flags** is or’d to
work\_struct flags.

**Context**

raw\_spin\_lock\_irq(pool->lock).

bool queue\_work\_on(int cpu, struct workqueue\_struct \*wq, struct work\_struct \*work)
:   queue work on specific cpu

**Parameters**

`int cpu`
:   CPU number to execute work on

`struct workqueue_struct *wq`
:   workqueue to use

`struct work_struct *work`
:   work to queue

**Description**

We queue the work to a specific CPU, the caller must ensure it
can’t go away. Callers that fail to ensure that the specified
CPU cannot go away will execute on a randomly chosen CPU.
But note well that callers specifying a CPU that never has been
online will get a splat.

**Return**

`false` if **work** was already on a queue, `true` otherwise.

int select\_numa\_node\_cpu(int node)
:   Select a CPU based on NUMA node

**Parameters**

`int node`
:   NUMA node ID that we want to select a CPU from

**Description**

This function will attempt to find a “random” cpu available on a given
node. If there are no CPUs available on the given node it will return
WORK\_CPU\_UNBOUND indicating that we should just schedule to any
available CPU if we need to schedule this work.

bool queue\_work\_node(int node, struct workqueue\_struct \*wq, struct work\_struct \*work)
:   queue work on a “random” cpu for a given NUMA node

**Parameters**

`int node`
:   NUMA node that we are targeting the work for

`struct workqueue_struct *wq`
:   workqueue to use

`struct work_struct *work`
:   work to queue

**Description**

We queue the work to a “random” CPU within a given NUMA node. The basic
idea here is to provide a way to somehow associate work with a given
NUMA node.

This function will only make a best effort attempt at getting this onto
the right NUMA node. If no node is requested or the requested node is
offline then we just fall back to standard queue\_work behavior.

Currently the “random” CPU ends up being the first available CPU in the
intersection of cpu\_online\_mask and the cpumask of the node, unless we
are running on the node. In that case we just use the current CPU.

**Return**

`false` if **work** was already on a queue, `true` otherwise.

bool queue\_delayed\_work\_on(int cpu, struct workqueue\_struct \*wq, struct delayed\_work \*dwork, unsigned long delay)
:   queue work on specific CPU after delay

**Parameters**

`int cpu`
:   CPU number to execute work on

`struct workqueue_struct *wq`
:   workqueue to use

`struct delayed_work *dwork`
:   work to queue

`unsigned long delay`
:   number of jiffies to wait before queueing

**Description**

We queue the delayed\_work to a specific CPU, for non-zero delays the
caller must ensure it is online and can’t go away. Callers that fail
to ensure this, may get **dwork->timer** queued to an offlined CPU and
this will prevent queueing of **dwork->work** unless the offlined CPU
becomes online again.

**Return**

`false` if **work** was already on a queue, `true` otherwise. If
**delay** is zero and **dwork** is idle, it will be scheduled for immediate
execution.

bool mod\_delayed\_work\_on(int cpu, struct workqueue\_struct \*wq, struct delayed\_work \*dwork, unsigned long delay)
:   modify delay of or queue a delayed work on specific CPU

**Parameters**

`int cpu`
:   CPU number to execute work on

`struct workqueue_struct *wq`
:   workqueue to use

`struct delayed_work *dwork`
:   work to queue

`unsigned long delay`
:   number of jiffies to wait before queueing

**Description**

If **dwork** is idle, equivalent to [`queue_delayed_work_on()`](#c.queue_delayed_work_on "queue_delayed_work_on"); otherwise,
modify **dwork**’s timer so that it expires after **delay**. If **delay** is
zero, **work** is guaranteed to be scheduled immediately regardless of its
current state.

This function is safe to call from any context including IRQ handler.
See [`try_to_grab_pending()`](#c.try_to_grab_pending "try_to_grab_pending") for details.

**Return**

`false` if **dwork** was idle and queued, `true` if **dwork** was
pending and its timer was modified.

bool queue\_rcu\_work(struct workqueue\_struct \*wq, struct rcu\_work \*rwork)
:   queue work after a RCU grace period

**Parameters**

`struct workqueue_struct *wq`
:   workqueue to use

`struct rcu_work *rwork`
:   work to queue

**Return**

`false` if **rwork** was already pending, `true` otherwise. Note
that a full RCU grace period is guaranteed only after a `true` return.
While **rwork** is guaranteed to be executed after a `false` return, the
execution may happen before a full RCU grace period has passed.

void worker\_attach\_to\_pool(struct [worker](#c.worker_attach_to_pool "worker") \*worker, struct worker\_pool \*pool)
:   attach a worker to a pool

**Parameters**

`struct worker *worker`
:   worker to be attached

`struct worker_pool *pool`
:   the target pool

**Description**

Attach **worker** to **pool**. Once attached, the `WORKER_UNBOUND` flag and
cpu-binding of **worker** are kept coordinated with the pool across
cpu-[un]hotplugs.

void worker\_detach\_from\_pool(struct [worker](#c.worker_detach_from_pool "worker") \*worker)
:   detach a worker from its pool

**Parameters**

`struct worker *worker`
:   worker which is attached to its pool

**Description**

Undo the attaching which had been done in [`worker_attach_to_pool()`](#c.worker_attach_to_pool "worker_attach_to_pool"). The
caller worker shouldn’t access to the pool after detached except it has
other reference to the pool.

struct worker \*create\_worker(struct worker\_pool \*pool)
:   create a new workqueue worker

**Parameters**

`struct worker_pool *pool`
:   pool the new worker will belong to

**Description**

Create and start a new worker which is attached to **pool**.

**Context**

Might sleep. Does GFP\_KERNEL allocations.

**Return**

Pointer to the newly created worker.

void set\_worker\_dying(struct [worker](#c.set_worker_dying "worker") \*worker, struct list\_head \*list)
:   Tag a worker for destruction

**Parameters**

`struct worker *worker`
:   worker to be destroyed

`struct list_head *list`
:   transfer worker away from its pool->idle\_list and into list

**Description**

Tag **worker** for destruction and adjust **pool** stats accordingly. The worker
should be idle.

**Context**

raw\_spin\_lock\_irq(pool->lock).

void idle\_worker\_timeout(struct timer\_list \*t)
:   check if some idle workers can now be deleted.

**Parameters**

`struct timer_list *t`
:   The pool’s idle\_timer that just expired

**Description**

The timer is armed in [`worker_enter_idle()`](#c.worker_enter_idle "worker_enter_idle"). Note that it isn’t disarmed in
[`worker_leave_idle()`](#c.worker_leave_idle "worker_leave_idle"), as a worker flicking between idle and active while its
pool is at the `too_many_workers()` tipping point would cause too much timer
housekeeping overhead. Since IDLE\_WORKER\_TIMEOUT is long enough, we just let
it expire and re-evaluate things from there.

void idle\_cull\_fn(struct work\_struct \*work)
:   cull workers that have been idle for too long.

**Parameters**

`struct work_struct *work`
:   the pool’s work for handling these idle workers

**Description**

This goes through a pool’s idle workers and gets rid of those that have been
idle for at least IDLE\_WORKER\_TIMEOUT seconds.

We don’t want to disturb isolated CPUs because of a pcpu kworker being
culled, so this also resets worker affinity. This requires a sleepable
context, hence the split between timer callback and work item.

void maybe\_create\_worker(struct worker\_pool \*pool)
:   create a new worker if necessary

**Parameters**

`struct worker_pool *pool`
:   pool to create a new worker for

**Description**

Create a new worker for **pool** if necessary. **pool** is guaranteed to
have at least one idle worker on return from this function. If
creating a new worker takes longer than MAYDAY\_INTERVAL, mayday is
sent to all rescuers with works scheduled on **pool** to resolve
possible allocation deadlock.

On return, `need_to_create_worker()` is guaranteed to be `false` and
`may_start_working()` `true`.

LOCKING:
raw\_spin\_lock\_irq(pool->lock) which may be released and regrabbed
multiple times. Does GFP\_KERNEL allocations. Called only from
manager.

bool manage\_workers(struct [worker](#c.manage_workers "worker") \*worker)
:   manage worker pool

**Parameters**

`struct worker *worker`
:   self

**Description**

Assume the manager role and manage the worker pool **worker** belongs
to. At any given time, there can be only zero or one manager per
pool. The exclusion is handled automatically by this function.

The caller can safely start processing works on false return. On
true return, it’s guaranteed that `need_to_create_worker()` is false
and `may_start_working()` is true.

**Context**

raw\_spin\_lock\_irq(pool->lock) which may be released and regrabbed
multiple times. Does GFP\_KERNEL allocations.

**Return**

`false` if the pool doesn’t need management and the caller can safely
start processing works, `true` if management function was performed and
the conditions that the caller verified before calling the function may
no longer be true.

void process\_one\_work(struct [worker](#c.process_one_work "worker") \*worker, struct work\_struct \*work)
:   process single work

**Parameters**

`struct worker *worker`
:   self

`struct work_struct *work`
:   work to process

**Description**

Process **work**. This function contains all the logics necessary to
process a single work including synchronization against and
interaction with other workers on the same cpu, queueing and
flushing. As long as context requirement is met, any worker can
call this function to process a work.

**Context**

raw\_spin\_lock\_irq(pool->lock) which is released and regrabbed.

void process\_scheduled\_works(struct [worker](#c.process_scheduled_works "worker") \*worker)
:   process scheduled works

**Parameters**

`struct worker *worker`
:   self

**Description**

Process all scheduled works. Please note that the scheduled list
may change while processing a work, so this function repeatedly
fetches a work from the top and executes it.

**Context**

raw\_spin\_lock\_irq(pool->lock) which may be released and regrabbed
multiple times.

int worker\_thread(void \*\_\_worker)
:   the worker thread function

**Parameters**

`void *__worker`
:   self

**Description**

The worker thread function. All workers belong to a worker\_pool -
either a per-cpu one or dynamic unbound one. These workers process all
work items regardless of their specific target workqueue. The only
exception is work items which belong to workqueues with a rescuer which
will be explained in [`rescuer_thread()`](#c.rescuer_thread "rescuer_thread").

**Return**

0

int rescuer\_thread(void \*\_\_rescuer)
:   the rescuer thread function

**Parameters**

`void *__rescuer`
:   self

**Description**

Workqueue rescuer thread function. There’s one rescuer for each
workqueue which has WQ\_MEM\_RECLAIM set.

Regular work processing on a pool may block trying to create a new
worker which uses GFP\_KERNEL allocation which has slight chance of
developing into deadlock if some works currently on the same queue
need to be processed to satisfy the GFP\_KERNEL allocation. This is
the problem rescuer solves.

When such condition is possible, the pool summons rescuers of all
workqueues which have works queued on the pool and let them process
those works so that forward progress can be guaranteed.

This should happen rarely.

**Return**

0

void check\_flush\_dependency(struct workqueue\_struct \*target\_wq, struct work\_struct \*target\_work, bool from\_cancel)
:   check for flush dependency sanity

**Parameters**

`struct workqueue_struct *target_wq`
:   workqueue being flushed

`struct work_struct *target_work`
:   work item being flushed (NULL for workqueue flushes)

`bool from_cancel`
:   are we called from the work cancel path

**Description**

`current` is trying to flush the whole **target\_wq** or **target\_work** on it.
If this is not the cancel path (which implies work being flushed is either
already running, or will not be at all), check if **target\_wq** doesn’t have
`WQ_MEM_RECLAIM` and verify that `current` is not reclaiming memory or running
on a workqueue which doesn’t have `WQ_MEM_RECLAIM` as that can break forward-
progress guarantee leading to a deadlock.

void insert\_wq\_barrier(struct pool\_workqueue \*pwq, struct wq\_barrier \*barr, struct work\_struct \*target, struct [worker](#c.insert_wq_barrier "worker") \*worker)
:   insert a barrier work

**Parameters**

`struct pool_workqueue *pwq`
:   pwq to insert barrier into

`struct wq_barrier *barr`
:   wq\_barrier to insert

`struct work_struct *target`
:   target work to attach **barr** to

`struct worker *worker`
:   worker currently executing **target**, NULL if **target** is not executing

**Description**

**barr** is linked to **target** such that **barr** is completed only after
**target** finishes execution. Please note that the ordering
guarantee is observed only with respect to **target** and on the local
cpu.

Currently, a queued barrier can’t be canceled. This is because
[`try_to_grab_pending()`](#c.try_to_grab_pending "try_to_grab_pending") can’t determine whether the work to be
grabbed is at the head of the queue and thus can’t clear LINKED
flag of the previous work while there must be a valid next work
after a work with LINKED flag set.

Note that when **worker** is non-NULL, **target** may be modified
underneath us, so we can’t reliably determine pwq from **target**.

**Context**

raw\_spin\_lock\_irq(pool->lock).

bool flush\_workqueue\_prep\_pwqs(struct workqueue\_struct \*wq, int flush\_color, int work\_color)
:   prepare pwqs for workqueue flushing

**Parameters**

`struct workqueue_struct *wq`
:   workqueue being flushed

`int flush_color`
:   new flush color, < 0 for no-op

`int work_color`
:   new work color, < 0 for no-op

**Description**

Prepare pwqs for workqueue flushing.

If **flush\_color** is non-negative, flush\_color on all pwqs should be
-1. If no pwq has in-flight commands at the specified color, all
pwq->flush\_color’s stay at -1 and `false` is returned. If any pwq
has in flight commands, its pwq->flush\_color is set to
**flush\_color**, **wq->nr\_pwqs\_to\_flush** is updated accordingly, pwq
wakeup logic is armed and `true` is returned.

The caller should have initialized **wq->first\_flusher** prior to
calling this function with non-negative **flush\_color**. If
**flush\_color** is negative, no flush color update is done and `false`
is returned.

If **work\_color** is non-negative, all pwqs should have the same
work\_color which is previous to **work\_color** and all will be
advanced to **work\_color**.

**Context**

mutex\_lock(wq->mutex).

**Return**

`true` if **flush\_color** >= 0 and there’s something to flush. `false`
otherwise.

void \_\_flush\_workqueue(struct workqueue\_struct \*wq)
:   ensure that any scheduled work has run to completion.

**Parameters**

`struct workqueue_struct *wq`
:   workqueue to flush

**Description**

This function sleeps until all work items which were queued on entry
have finished execution, but it is not livelocked by new incoming ones.

void drain\_workqueue(struct workqueue\_struct \*wq)
:   drain a workqueue

**Parameters**

`struct workqueue_struct *wq`
:   workqueue to drain

**Description**

Wait until the workqueue becomes empty. While draining is in progress,
only chain queueing is allowed. IOW, only currently pending or running
work items on **wq** can queue further work items on it. **wq** is flushed
repeatedly until it becomes empty. The number of flushing is determined
by the depth of chaining and should be relatively short. Whine if it
takes too long.

bool flush\_work(struct work\_struct \*work)
:   wait for a work to finish executing the last queueing instance

**Parameters**

`struct work_struct *work`
:   the work to flush

**Description**

Wait until **work** has finished execution. **work** is guaranteed to be idle
on return if it hasn’t been requeued since flush started.

**Return**

`true` if [`flush_work()`](#c.flush_work "flush_work") waited for the work to finish execution,
`false` if it was already idle.

bool flush\_delayed\_work(struct delayed\_work \*dwork)
:   wait for a dwork to finish executing the last queueing

**Parameters**

`struct delayed_work *dwork`
:   the delayed work to flush

**Description**

Delayed timer is cancelled and the pending work is queued for
immediate execution. Like [`flush_work()`](#c.flush_work "flush_work"), this function only
considers the last queueing instance of **dwork**.

**Return**

`true` if [`flush_work()`](#c.flush_work "flush_work") waited for the work to finish execution,
`false` if it was already idle.

bool flush\_rcu\_work(struct rcu\_work \*rwork)
:   wait for a rwork to finish executing the last queueing

**Parameters**

`struct rcu_work *rwork`
:   the rcu work to flush

**Return**

`true` if [`flush_rcu_work()`](#c.flush_rcu_work "flush_rcu_work") waited for the work to finish execution,
`false` if it was already idle.

bool cancel\_work\_sync(struct work\_struct \*work)
:   cancel a work and wait for it to finish

**Parameters**

`struct work_struct *work`
:   the work to cancel

**Description**

Cancel **work** and wait for its execution to finish. This function can be used
even if the work re-queues itself or migrates to another workqueue. On return
from this function, **work** is guaranteed to be not pending or executing on any
CPU as long as there aren’t racing enqueues.

cancel\_work\_sync(`delayed_work->work`) must not be used for delayed\_work’s.
Use [`cancel_delayed_work_sync()`](#c.cancel_delayed_work_sync "cancel_delayed_work_sync") instead.

Must be called from a sleepable context if **work** was last queued on a non-BH
workqueue. Can also be called from non-hardirq atomic contexts including BH
if **work** was last queued on a BH workqueue.

Returns `true` if **work** was pending, `false` otherwise.

bool cancel\_delayed\_work(struct delayed\_work \*dwork)
:   cancel a delayed work

**Parameters**

`struct delayed_work *dwork`
:   delayed\_work to cancel

**Description**

Kill off a pending delayed\_work.

**Return**

`true` if **dwork** was pending and canceled; `false` if it wasn’t
pending.

**Note**

The work callback function may still be running on return, unless
it returns `true` and the work doesn’t re-arm itself. Explicitly flush or
use [`cancel_delayed_work_sync()`](#c.cancel_delayed_work_sync "cancel_delayed_work_sync") to wait on it.

This function is safe to call from any context including IRQ handler.

bool cancel\_delayed\_work\_sync(struct delayed\_work \*dwork)
:   cancel a delayed work and wait for it to finish

**Parameters**

`struct delayed_work *dwork`
:   the delayed work cancel

**Description**

This is [`cancel_work_sync()`](#c.cancel_work_sync "cancel_work_sync") for delayed works.

**Return**

`true` if **dwork** was pending, `false` otherwise.

bool disable\_work(struct work\_struct \*work)
:   Disable and cancel a work item

**Parameters**

`struct work_struct *work`
:   work item to disable

**Description**

Disable **work** by incrementing its disable count and cancel it if currently
pending. As long as the disable count is non-zero, any attempt to queue **work**
will fail and return `false`. The maximum supported disable depth is 2 to the
power of `WORK_OFFQ_DISABLE_BITS`, currently 65536.

Can be called from any context. Returns `true` if **work** was pending, `false`
otherwise.

bool disable\_work\_sync(struct work\_struct \*work)
:   Disable, cancel and drain a work item

**Parameters**

`struct work_struct *work`
:   work item to disable

**Description**

Similar to [`disable_work()`](#c.disable_work "disable_work") but also wait for **work** to finish if currently
executing.

Must be called from a sleepable context if **work** was last queued on a non-BH
workqueue. Can also be called from non-hardirq atomic contexts including BH
if **work** was last queued on a BH workqueue.

Returns `true` if **work** was pending, `false` otherwise.

bool enable\_work(struct work\_struct \*work)
:   Enable a work item

**Parameters**

`struct work_struct *work`
:   work item to enable

**Description**

Undo disable\_work[\_sync]() by decrementing **work**’s disable count. **work** can
only be queued if its disable count is 0.

Can be called from any context. Returns `true` if the disable count reached 0.
Otherwise, `false`.

bool disable\_delayed\_work(struct delayed\_work \*dwork)
:   Disable and cancel a delayed work item

**Parameters**

`struct delayed_work *dwork`
:   delayed work item to disable

**Description**

[`disable_work()`](#c.disable_work "disable_work") for delayed work items.

bool disable\_delayed\_work\_sync(struct delayed\_work \*dwork)
:   Disable, cancel and drain a delayed work item

**Parameters**

`struct delayed_work *dwork`
:   delayed work item to disable

**Description**

[`disable_work_sync()`](#c.disable_work_sync "disable_work_sync") for delayed work items.

bool enable\_delayed\_work(struct delayed\_work \*dwork)
:   Enable a delayed work item

**Parameters**

`struct delayed_work *dwork`
:   delayed work item to enable

**Description**

[`enable_work()`](#c.enable_work "enable_work") for delayed work items.

int schedule\_on\_each\_cpu(work\_func\_t func)
:   execute a function synchronously on each online CPU

**Parameters**

`work_func_t func`
:   the function to call

**Description**

[`schedule_on_each_cpu()`](#c.schedule_on_each_cpu "schedule_on_each_cpu") executes **func** on each online CPU using the
system workqueue and blocks until all CPUs have completed.
[`schedule_on_each_cpu()`](#c.schedule_on_each_cpu "schedule_on_each_cpu") is very slow.

**Return**

0 on success, -errno on failure.

int execute\_in\_process\_context(work\_func\_t fn, struct execute\_work \*ew)
:   reliably execute the routine with user context

**Parameters**

`work_func_t fn`
:   the function to execute

`struct execute_work *ew`
:   guaranteed storage for the execute work structure (must
    be available when the work executes)

**Description**

Executes the function immediately if process context is available,
otherwise schedules the function for delayed execution.

**Return**

0 - function was executed
1 - function was scheduled for execution

void free\_workqueue\_attrs(struct [workqueue\_attrs](#c.workqueue_attrs "workqueue_attrs") \*attrs)
:   free a workqueue\_attrs

**Parameters**

`struct workqueue_attrs *attrs`
:   workqueue\_attrs to free

**Description**

Undo [`alloc_workqueue_attrs()`](#c.alloc_workqueue_attrs "alloc_workqueue_attrs").

struct [workqueue\_attrs](#c.workqueue_attrs "workqueue_attrs") \*alloc\_workqueue\_attrs(void)
:   allocate a workqueue\_attrs

**Parameters**

`void`
:   no arguments

**Description**

Allocate a new workqueue\_attrs, initialize with default settings and
return it.

**Return**

The allocated new workqueue\_attr on success. `NULL` on failure.

int init\_worker\_pool(struct worker\_pool \*pool)
:   initialize a newly zalloc’d worker\_pool

**Parameters**

`struct worker_pool *pool`
:   worker\_pool to initialize

**Description**

Initialize a newly zalloc’d **pool**. It also allocates **pool->attrs**.

**Return**

0 on success, -errno on failure. Even on failure, all fields
inside **pool** proper are initialized and [`put_unbound_pool()`](#c.put_unbound_pool "put_unbound_pool") can be called
on **pool** safely to release it.

void put\_unbound\_pool(struct worker\_pool \*pool)
:   put a worker\_pool

**Parameters**

`struct worker_pool *pool`
:   worker\_pool to put

**Description**

Put **pool**. If its refcnt reaches zero, it gets destroyed in RCU
safe manner. [`get_unbound_pool()`](#c.get_unbound_pool "get_unbound_pool") calls this function on its failure path
and this function should be able to release pools which went through,
successfully or not, [`init_worker_pool()`](#c.init_worker_pool "init_worker_pool").

Should be called with wq\_pool\_mutex held.

struct worker\_pool \*get\_unbound\_pool(const struct [workqueue\_attrs](#c.workqueue_attrs "workqueue_attrs") \*attrs)
:   get a worker\_pool with the specified attributes

**Parameters**

`const struct workqueue_attrs *attrs`
:   the attributes of the worker\_pool to get

**Description**

Obtain a worker\_pool which has the same attributes as **attrs**, bump the
reference count and return it. If there already is a matching
worker\_pool, it will be used; otherwise, this function attempts to
create a new one.

Should be called with wq\_pool\_mutex held.

**Return**

On success, a worker\_pool with the same attributes as **attrs**.
On failure, `NULL`.

void wq\_calc\_pod\_cpumask(struct [workqueue\_attrs](#c.workqueue_attrs "workqueue_attrs") \*attrs, int cpu)
:   calculate a wq\_attrs’ cpumask for a pod

**Parameters**

`struct workqueue_attrs *attrs`
:   the wq\_attrs of the default pwq of the target workqueue

`int cpu`
:   the target CPU

**Description**

Calculate the cpumask a workqueue with **attrs** should use on **pod**.
The result is stored in **attrs->\_\_pod\_cpumask**.

If pod affinity is not enabled, **attrs->cpumask** is always used. If enabled
and **pod** has online CPUs requested by **attrs**, the returned cpumask is the
intersection of the possible CPUs of **pod** and **attrs->cpumask**.

The caller is responsible for ensuring that the cpumask of **pod** stays stable.

int apply\_workqueue\_attrs(struct workqueue\_struct \*wq, const struct [workqueue\_attrs](#c.workqueue_attrs "workqueue_attrs") \*attrs)
:   apply new workqueue\_attrs to an unbound workqueue

**Parameters**

`struct workqueue_struct *wq`
:   the target workqueue

`const struct workqueue_attrs *attrs`
:   the workqueue\_attrs to apply, allocated with [`alloc_workqueue_attrs()`](#c.alloc_workqueue_attrs "alloc_workqueue_attrs")

**Description**

Apply **attrs** to an unbound workqueue **wq**. Unless disabled, this function maps
a separate pwq to each CPU pod with possibles CPUs in **attrs->cpumask** so that
work items are affine to the pod it was issued on. Older pwqs are released as
in-flight work items finish. Note that a work item which repeatedly requeues
itself back-to-back will stay on its current pwq.

Performs GFP\_KERNEL allocations.

**Return**

0 on success and -errno on failure.

void unbound\_wq\_update\_pwq(struct workqueue\_struct \*wq, int cpu)
:   update a pwq slot for CPU hot[un]plug

**Parameters**

`struct workqueue_struct *wq`
:   the target workqueue

`int cpu`
:   the CPU to update the pwq slot for

**Description**

This function is to be called from `CPU_DOWN_PREPARE`, `CPU_ONLINE` and
`CPU_DOWN_FAILED`. **cpu** is in the same pod of the CPU being hot[un]plugged.

If pod affinity can’t be adjusted due to memory allocation failure, it falls
back to **wq->dfl\_pwq** which may not be optimal but is always correct.

Note that when the last allowed CPU of a pod goes offline for a workqueue
with a cpumask spanning multiple pods, the workers which were already
executing the work items for the workqueue will lose their CPU affinity and
may execute on any CPU. This is similar to how per-cpu workqueues behave on
CPU\_DOWN. If a workqueue user wants strict affinity, it’s the user’s
responsibility to flush the work item from CPU\_DOWN\_PREPARE.

void wq\_adjust\_max\_active(struct workqueue\_struct \*wq)
:   update a wq’s max\_active to the current setting

**Parameters**

`struct workqueue_struct *wq`
:   target workqueue

**Description**

If **wq** isn’t freezing, set **wq->max\_active** to the saved\_max\_active and
activate inactive work items accordingly. If **wq** is freezing, clear
**wq->max\_active** to zero.

void destroy\_workqueue(struct workqueue\_struct \*wq)
:   safely terminate a workqueue

**Parameters**

`struct workqueue_struct *wq`
:   target workqueue

**Description**

Safely destroy a workqueue. All work currently pending will be done first.

This function does NOT guarantee that non-pending work that has been
submitted with [`queue_delayed_work()`](#c.queue_delayed_work "queue_delayed_work") and similar functions will be done
before destroying the workqueue. The fundamental problem is that, currently,
the workqueue has no way of accessing non-pending delayed\_work. delayed\_work
is only linked on the timer-side. All delayed\_work must, therefore, be
canceled before calling this function.

TODO: It would be better if the problem described above wouldn’t exist and
[`destroy_workqueue()`](#c.destroy_workqueue "destroy_workqueue") would cleanly cancel all pending and non-pending
delayed\_work.

void workqueue\_set\_max\_active(struct workqueue\_struct \*wq, int max\_active)
:   adjust max\_active of a workqueue

**Parameters**

`struct workqueue_struct *wq`
:   target workqueue

`int max_active`
:   new max\_active value.

**Description**

Set max\_active of **wq** to **max\_active**. See the [`alloc_workqueue()`](#c.alloc_workqueue "alloc_workqueue") function
comment.

**Context**

Don’t call from IRQ context.

void workqueue\_set\_min\_active(struct workqueue\_struct \*wq, int min\_active)
:   adjust min\_active of an unbound workqueue

**Parameters**

`struct workqueue_struct *wq`
:   target unbound workqueue

`int min_active`
:   new min\_active value

**Description**

Set min\_active of an unbound workqueue. Unlike other types of workqueues, an
unbound workqueue is not guaranteed to be able to process max\_active
interdependent work items. Instead, an unbound workqueue is guaranteed to be
able to process min\_active number of interdependent work items which is
`WQ_DFL_MIN_ACTIVE` by default.

Use this function to adjust the min\_active value between 0 and the current
max\_active.

struct work\_struct \*current\_work(void)
:   retrieve `current` task’s work struct

**Parameters**

`void`
:   no arguments

**Description**

Determine if `current` task is a workqueue worker and what it’s working on.
Useful to find out the context that the `current` task is running in.

**Return**

work struct if `current` task is a workqueue worker, `NULL` otherwise.

bool current\_is\_workqueue\_rescuer(void)
:   is `current` workqueue rescuer?

**Parameters**

`void`
:   no arguments

**Description**

Determine whether `current` is a workqueue rescuer. Can be used from
work functions to determine whether it’s being run off the rescuer task.

**Return**

`true` if `current` is a workqueue rescuer. `false` otherwise.

bool workqueue\_congested(int cpu, struct workqueue\_struct \*wq)
:   test whether a workqueue is congested

**Parameters**

`int cpu`
:   CPU in question

`struct workqueue_struct *wq`
:   target workqueue

**Description**

Test whether **wq**’s cpu workqueue for **cpu** is congested. There is
no synchronization around this function and the test result is
unreliable and only useful as advisory hints or for debugging.

If **cpu** is WORK\_CPU\_UNBOUND, the test is performed on the local CPU.

With the exception of ordered workqueues, all workqueues have per-cpu
pool\_workqueues, each with its own congested state. A workqueue being
congested on one CPU doesn’t mean that the workqueue is contested on any
other CPUs.

**Return**

`true` if congested, `false` otherwise.

unsigned int work\_busy(struct work\_struct \*work)
:   test whether a work is currently pending or running

**Parameters**

`struct work_struct *work`
:   the work to be tested

**Description**

Test whether **work** is currently pending or running. There is no
synchronization around this function and the test result is
unreliable and only useful as advisory hints or for debugging.

**Return**

OR’d bitmask of WORK\_BUSY\_\* bits.

void set\_worker\_desc(const char \*fmt, ...)
:   set description for the current work item

**Parameters**

`const char *fmt`
:   printf-style format string

`...`
:   arguments for the format string

**Description**

This function can be called by a running work function to describe what
the work item is about. If the worker task gets dumped, this
information will be printed out together to help debugging. The
description can be at most WORKER\_DESC\_LEN including the trailing ‘0’.

void print\_worker\_info(const char \*log\_lvl, struct task\_struct \*task)
:   print out worker information and description

**Parameters**

`const char *log_lvl`
:   the log level to use when printing

`struct task_struct *task`
:   target task

**Description**

If **task** is a worker and currently executing a work item, print out the
name of the workqueue being serviced and worker description set with
[`set_worker_desc()`](#c.set_worker_desc "set_worker_desc") by the currently executing work item.

This function can be safely called on any task as long as the
task\_struct itself is accessible. While safe, this function isn’t
synchronized and may print out mixups or garbages of limited length.

void show\_one\_workqueue(struct workqueue\_struct \*wq)
:   dump state of specified workqueue

**Parameters**

`struct workqueue_struct *wq`
:   workqueue whose state will be printed

void show\_one\_worker\_pool(struct worker\_pool \*pool)
:   dump state of specified worker pool

**Parameters**

`struct worker_pool *pool`
:   worker pool whose state will be printed

void show\_all\_workqueues(void)
:   dump workqueue state

**Parameters**

`void`
:   no arguments

**Description**

Called from a sysrq handler and prints out all busy workqueues and pools.

void show\_freezable\_workqueues(void)
:   dump freezable workqueue state

**Parameters**

`void`
:   no arguments

**Description**

Called from `try_to_freeze_tasks()` and prints out all freezable workqueues
still busy.

void rebind\_workers(struct worker\_pool \*pool)
:   rebind all workers of a pool to the associated CPU

**Parameters**

`struct worker_pool *pool`
:   pool of interest

**Description**

**pool->cpu** is coming online. Rebind all workers to the CPU.

void restore\_unbound\_workers\_cpumask(struct worker\_pool \*pool, int cpu)
:   restore cpumask of unbound workers

**Parameters**

`struct worker_pool *pool`
:   unbound pool of interest

`int cpu`
:   the CPU which is coming up

**Description**

An unbound pool may end up with a cpumask which doesn’t have any online
CPUs. When a worker of such pool get scheduled, the scheduler resets
its cpus\_allowed. If **cpu** is in **pool**’s cpumask which didn’t have any
online CPU before, cpus\_allowed of all its workers should be restored.

long work\_on\_cpu\_key(int cpu, long (\*fn)(void\*), void \*arg, struct lock\_class\_key \*key)
:   run a function in thread context on a particular cpu

**Parameters**

`int cpu`
:   the cpu to run on

`long (*fn)(void *)`
:   the function to run

`void *arg`
:   the function arg

`struct lock_class_key *key`
:   The lock class key for lock debugging purposes

**Description**

It is up to the caller to ensure that the cpu doesn’t go offline.
The caller must not hold any locks which would prevent **fn** from completing.

**Return**

The value **fn** returns.

void freeze\_workqueues\_begin(void)
:   begin freezing workqueues

**Parameters**

`void`
:   no arguments

**Description**

Start freezing workqueues. After this function returns, all freezable
workqueues will queue new works to their inactive\_works list instead of
pool->worklist.

**Context**

Grabs and releases wq\_pool\_mutex, wq->mutex and pool->lock’s.

bool freeze\_workqueues\_busy(void)
:   are freezable workqueues still busy?

**Parameters**

`void`
:   no arguments

**Description**

Check whether freezing is complete. This function must be called
between [`freeze_workqueues_begin()`](#c.freeze_workqueues_begin "freeze_workqueues_begin") and [`thaw_workqueues()`](#c.thaw_workqueues "thaw_workqueues").

**Context**

Grabs and releases wq\_pool\_mutex.

**Return**

`true` if some freezable workqueues are still busy. `false` if freezing
is complete.

void thaw\_workqueues(void)
:   thaw workqueues

**Parameters**

`void`
:   no arguments

**Description**

Thaw workqueues. Normal queueing is restored and all collected
frozen works are transferred to their respective pool worklists.

**Context**

Grabs and releases wq\_pool\_mutex, wq->mutex and pool->lock’s.

int workqueue\_unbound\_housekeeping\_update(const struct cpumask \*hk)
:   Propagate housekeeping cpumask update

**Parameters**

`const struct cpumask *hk`
:   the new housekeeping cpumask

**Description**

Update the unbound workqueue cpumask on top of the new housekeeping cpumask such
that the effective unbound affinity is the intersection of the new housekeeping
with the requested affinity set via nohz\_full=/isolcpus= or sysfs.

**Return**

0 on success and -errno on failure.

int workqueue\_set\_unbound\_cpumask(cpumask\_var\_t cpumask)
:   Set the low-level unbound cpumask

**Parameters**

`cpumask_var_t cpumask`
:   the cpumask to set

**Description**

> The low-level workqueues cpumask is a global cpumask that limits
> the affinity of all unbound workqueues. This function check the **cpumask**
> and apply it to all unbound workqueues and updates all pwqs of them.

**Return**

0 - Success
-EINVAL - Invalid **cpumask**
-ENOMEM - Failed to allocate memory for attrs or pwqs.

int workqueue\_sysfs\_register(struct workqueue\_struct \*wq)
:   make a workqueue visible in sysfs

**Parameters**

`struct workqueue_struct *wq`
:   the workqueue to register

**Description**

Expose **wq** in sysfs under /sys/bus/workqueue/devices.
alloc\_workqueue\*() automatically calls this function if WQ\_SYSFS is set
which is the preferred method.

Workqueue user should use this function directly iff it wants to apply
workqueue\_attrs before making the workqueue visible in sysfs; otherwise,
[`apply_workqueue_attrs()`](#c.apply_workqueue_attrs "apply_workqueue_attrs") may race against userland updating the
attributes.

**Return**

0 on success, -errno on failure.

void workqueue\_sysfs\_unregister(struct workqueue\_struct \*wq)
:   undo [`workqueue_sysfs_register()`](#c.workqueue_sysfs_register "workqueue_sysfs_register")

**Parameters**

`struct workqueue_struct *wq`
:   the workqueue to unregister

**Description**

If **wq** is registered to sysfs by [`workqueue_sysfs_register()`](#c.workqueue_sysfs_register "workqueue_sysfs_register"), unregister.

void workqueue\_init\_early(void)
:   early init for workqueue subsystem

**Parameters**

`void`
:   no arguments

**Description**

This is the first step of three-staged workqueue subsystem initialization and
invoked as soon as the bare basics - memory allocation, cpumasks and idr are
up. It sets up all the data structures and system workqueues and allows early
boot code to create workqueues and queue/cancel work items. Actual work item
execution starts only after kthreads can be created and scheduled right
before early initcalls.

void workqueue\_init(void)
:   bring workqueue subsystem fully online

**Parameters**

`void`
:   no arguments

**Description**

This is the second step of three-staged workqueue subsystem initialization
and invoked as soon as kthreads can be created and scheduled. Workqueues have
been created and work items queued on them, but there are no kworkers
executing the work items yet. Populate the worker pools with the initial
workers and enable future kworker creations.

int llc\_count\_cores(const struct cpumask \*pod\_cpus, struct wq\_pod\_type \*smt\_pods)
:   count distinct cores (SMT groups) within an LLC pod

**Parameters**

`const struct cpumask *pod_cpus`
:   the cpumask of CPUs in the LLC pod

`struct wq_pod_type *smt_pods`
:   the SMT pod type, used to identify sibling groups

**Description**

A core is represented by the lowest-numbered CPU in its SMT group. Returns
the number of distinct cores found in **pod\_cpus**.

void llc\_populate\_cpu\_shard\_id(const struct cpumask \*pod\_cpus, struct wq\_pod\_type \*smt\_pods, int nr\_cores)
:   populate cpu\_shard\_id[] for each CPU in an LLC pod

**Parameters**

`const struct cpumask *pod_cpus`
:   the cpumask of CPUs in the LLC pod

`struct wq_pod_type *smt_pods`
:   the SMT pod type, used to identify sibling groups

`int nr_cores`
:   number of distinct cores in **pod\_cpus** (from [`llc_count_cores()`](#c.llc_count_cores "llc_count_cores"))

**Description**

Walks **pod\_cpus** in order. At each SMT group leader, advances to the next
shard once the current shard is full. Results are written to cpu\_shard\_id[].

void precompute\_cache\_shard\_ids(void)
:   assign each CPU its shard index within its LLC

**Parameters**

`void`
:   no arguments

**Description**

Iterates over all LLC pods. For each pod, counts distinct cores then assigns
shard indices to all CPUs in the pod. Must be called after WQ\_AFFN\_CACHE and
WQ\_AFFN\_SMT have been initialized.

void workqueue\_init\_topology(void)
:   initialize CPU pods for unbound workqueues

**Parameters**

`void`
:   no arguments

**Description**

This is the third step of three-staged workqueue subsystem initialization and
invoked after SMP and topology information are fully initialized. It
initializes the unbound CPU pods accordingly.
