# Explicit volatile write back cache control

> 출처(원문): https://docs.kernel.org/block/writeback_cache_control.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Explicit volatile write back cache control

## Introduction

Many storage devices, especially in the consumer market, come with volatile
write back caches. That means the devices signal I/O completion to the
operating system before data actually has hit the non-volatile storage. This
behavior obviously speeds up various workloads, but it means the operating
system needs to force data out to the non-volatile storage when it performs
a data integrity operation like fsync, sync or an unmount.

The Linux block layer provides two simple mechanisms that let filesystems
control the caching behavior of the storage device. These mechanisms are
a forced cache flush, and the Force Unit Access (FUA) flag for requests.

## Explicit cache flushes

The REQ\_PREFLUSH flag can be OR ed into the r/w flags of a bio submitted from
the filesystem and will make sure the volatile cache of the storage device
has been flushed before the actual I/O operation is started. This explicitly
guarantees that previously completed write requests are on non-volatile
storage before the flagged bio starts. In addition the REQ\_PREFLUSH flag can be
set on an otherwise empty bio structure, which causes only an explicit cache
flush without any dependent I/O. It is recommend to use
the [`blkdev_issue_flush()`](../core-api/kernel-api.html#c.blkdev_issue_flush "blkdev_issue_flush") helper for a pure cache flush.

## Forced Unit Access

The REQ\_FUA flag can be OR ed into the r/w flags of a bio submitted from the
filesystem and will make sure that I/O completion for this request is only
signaled after the data has been committed to non-volatile storage.

## Implementation details for filesystems

Filesystems can simply set the REQ\_PREFLUSH and REQ\_FUA bits and do not have to
worry if the underlying devices need any explicit cache flushing and how
the Forced Unit Access is implemented. The REQ\_PREFLUSH and REQ\_FUA flags
may both be set on a single bio.

## Feature settings for block drivers

For devices that do not support volatile write caches there is no driver
support required, the block layer completes empty REQ\_PREFLUSH requests before
entering the driver and strips off the REQ\_PREFLUSH and REQ\_FUA bits from
requests that have a payload.

For devices with volatile write caches the driver needs to tell the block layer
that it supports flushing caches by setting the

> BLK\_FEAT\_WRITE\_CACHE

flag in the queue\_limits feature field. For devices that also support the FUA
bit the block layer needs to be told to pass on the REQ\_FUA bit by also setting
the

> BLK\_FEAT\_FUA

flag in the features field of the queue\_limits structure.

## Implementation details for bio based block drivers

For bio based drivers the REQ\_PREFLUSH and REQ\_FUA bit are simply passed on to
the driver if the driver sets the BLK\_FEAT\_WRITE\_CACHE flag and the driver
needs to handle them.

*NOTE*: The REQ\_FUA bit also gets passed on when the BLK\_FEAT\_FUA flags is
\_not\_ set. Any bio based driver that sets BLK\_FEAT\_WRITE\_CACHE also needs to
handle REQ\_FUA.

For remapping drivers the REQ\_FUA bits need to be propagated to underlying
devices, and a global flush needs to be implemented for bios with the
REQ\_PREFLUSH bit set.

## Implementation details for blk-mq drivers

When the BLK\_FEAT\_WRITE\_CACHE flag is set, REQ\_OP\_WRITE | REQ\_PREFLUSH requests
with a payload are automatically turned into a sequence of a REQ\_OP\_FLUSH
request followed by the actual write by the block layer.

When the BLK\_FEAT\_FUA flags is set, the REQ\_FUA bit is simply passed on for the
REQ\_OP\_WRITE request, else a REQ\_OP\_FLUSH request is sent by the block layer
after the completion of the write request for bio submissions with the REQ\_FUA
bit set.
