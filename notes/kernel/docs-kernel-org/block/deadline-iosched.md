# Deadline IO scheduler tunables

> 출처(원문): https://docs.kernel.org/block/deadline-iosched.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Deadline IO scheduler tunables

This little file attempts to document how the deadline io scheduler works.
In particular, it will clarify the meaning of the exposed tunables that may be
of interest to power users.

## Selecting IO schedulers

Refer to [Switching Scheduler](switching-sched.html) for information on
selecting an io scheduler on a per-device basis.

---

## read\_expire (in ms)

The goal of the deadline io scheduler is to attempt to guarantee a start
service time for a request. As we focus mainly on read latencies, this is
tunable. When a read request first enters the io scheduler, it is assigned
a deadline that is the current time + the read\_expire value in units of
milliseconds.

## write\_expire (in ms)

Similar to read\_expire mentioned above, but for writes.

## fifo\_batch (number of requests)

Requests are grouped into `batches` of a particular data direction (read or
write) which are serviced in increasing sector order. To limit extra seeking,
deadline expiries are only checked between batches. fifo\_batch controls the
maximum number of requests per batch.

This parameter tunes the balance between per-request latency and aggregate
throughput. When low latency is the primary concern, smaller is better (where
a value of 1 yields first-come first-served behaviour). Increasing fifo\_batch
generally improves throughput, at the cost of latency variation.

## writes\_starved (number of dispatches)

When we have to move requests from the io scheduler queue to the block
device dispatch queue, we always give a preference to reads. However, we
don’t want to starve writes indefinitely either. So writes\_starved controls
how many times we give preference to reads over writes. When that has been
done writes\_starved number of times, we dispatch some writes based on the
same criteria as reads.

## front\_merges (bool)

Sometimes it happens that a request enters the io scheduler that is contiguous
with a request that is already on the queue. Either it fits in the back of that
request, or it fits at the front. That is called either a back merge candidate
or a front merge candidate. Due to the way files are typically laid out,
back merges are much more common than front merges. For some work loads, you
may even know that it is a waste of time to spend any time attempting to
front merge requests. Setting front\_merges to 0 disables this functionality.
Front merges may still occur due to the cached last\_merge hint, but since
that comes at basically 0 cost we leave that on. We simply disable the
rbtree front sector lookup when the io scheduler merge function is called.

Nov 11 2002, Jens Axboe <[jens.axboe@oracle.com](mailto:jens.axboe%40oracle.com)>
