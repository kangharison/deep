# pstore block oops/panic logger

> 출처(원문): https://docs.kernel.org/admin-guide/pstore-blk.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# pstore block oops/panic logger

## Introduction

pstore block (pstore/blk) is an oops/panic logger that writes its logs to a
block device and non-block device before the system crashes. You can get
these log files by mounting pstore filesystem like:

```
mount -t pstore pstore /sys/fs/pstore
```

## pstore block concepts

pstore/blk provides efficient configuration method for pstore/blk, which
divides all configurations into two parts, configurations for user and
configurations for driver.

Configurations for user determine how pstore/blk works, such as pmsg\_size,
kmsg\_size and so on. All of them support both Kconfig and module parameters,
but module parameters have priority over Kconfig.

Configurations for driver are all about block device and non-block device,
such as total\_size of block device and read/write operations.

## Configurations for user

All of these configurations support both Kconfig and module parameters, but
module parameters have priority over Kconfig.

Here is an example for module parameters:

```
pstore_blk.blkdev=/dev/mmcblk0p7 pstore_blk.kmsg_size=64 best_effort=y
```

The detail of each configurations may be of interest to you.

### blkdev

The block device to use. Most of the time, it is a partition of block device.
It’s required for pstore/blk. It is also used for MTD device.

When pstore/blk is built as a module, “blkdev” accepts the following variants:

1. /dev/<disk\_name> represents the device number of disk
2. /dev/<disk\_name><decimal> represents the device number of partition - device
   number of disk plus the partition number
3. /dev/<disk\_name>p<decimal> - same as the above; this form is used when disk
   name of partitioned disk ends with a digit.

When pstore/blk is built into the kernel, “blkdev” accepts the following variants:

1. <hex\_major><hex\_minor> device number in hexadecimal representation,
   with no leading 0x, for example b302.
2. PARTUUID=00112233-4455-6677-8899-AABBCCDDEEFF represents the unique id of
   a partition if the partition table provides it. The UUID may be either an
   EFI/GPT UUID, or refer to an MSDOS partition using the format SSSSSSSS-PP,
   where SSSSSSSS is a zero-filled hex representation of the 32-bit
   “NT disk signature”, and PP is a zero-filled hex representation of the
   1-based partition number.
3. PARTUUID=<UUID>/PARTNROFF=<int> to select a partition in relation to a
   partition with a known unique id.
4. <major>:<minor> major and minor number of the device separated by a colon.

It accepts the following variants for MTD device:

1. <device name> MTD device name. “pstore” is recommended.
2. <device number> MTD device number.

### kmsg\_size

The chunk size in KB for oops/panic front-end. It **MUST** be a multiple of 4.
It’s optional if you do not care about the oops/panic log.

There are multiple chunks for oops/panic front-end depending on the remaining
space except other pstore front-ends.

pstore/blk will log to oops/panic chunks one by one, and always overwrite the
oldest chunk if there is no more free chunk.

### pmsg\_size

The chunk size in KB for pmsg front-end. It **MUST** be a multiple of 4.
It’s optional if you do not care about the pmsg log.

Unlike oops/panic front-end, there is only one chunk for pmsg front-end.

Pmsg is a user space accessible pstore object. Writes to */dev/pmsg0* are
appended to the chunk. On reboot the contents are available in
*/sys/fs/pstore/pmsg-pstore-blk-0*.

### console\_size

The chunk size in KB for console front-end. It **MUST** be a multiple of 4.
It’s optional if you do not care about the console log.

Similar to pmsg front-end, there is only one chunk for console front-end.

All log of console will be appended to the chunk. On reboot the contents are
available in */sys/fs/pstore/console-pstore-blk-0*.

### ftrace\_size

The chunk size in KB for ftrace front-end. It **MUST** be a multiple of 4.
It’s optional if you do not care about the ftrace log.

Similar to oops front-end, there are multiple chunks for ftrace front-end
depending on the count of cpu processors. Each chunk size is equal to
ftrace\_size / processors\_count.

All log of ftrace will be appended to the chunk. On reboot the contents are
combined and available in */sys/fs/pstore/ftrace-pstore-blk-0*.

Persistent function tracing might be useful for debugging software or hardware
related hangs. Here is an example of usage:

```
# mount -t pstore pstore /sys/fs/pstore
# mount -t debugfs debugfs /sys/kernel/debug/
# echo 1 > /sys/kernel/debug/pstore/record_ftrace
# reboot -f
[...]
# mount -t pstore pstore /sys/fs/pstore
# tail /sys/fs/pstore/ftrace-pstore-blk-0
CPU:0 ts:5914676 c0063828  c0063b94  call_cpuidle <- cpu_startup_entry+0x1b8/0x1e0
CPU:0 ts:5914678 c039ecdc  c006385c  cpuidle_enter_state <- call_cpuidle+0x44/0x48
CPU:0 ts:5914680 c039e9a0  c039ecf0  cpuidle_enter_freeze <- cpuidle_enter_state+0x304/0x314
CPU:0 ts:5914681 c0063870  c039ea30  sched_idle_set_state <- cpuidle_enter_state+0x44/0x314
CPU:1 ts:5916720 c0160f59  c015ee04  kernfs_unmap_bin_file <- __kernfs_remove+0x140/0x204
CPU:1 ts:5916721 c05ca625  c015ee0c  __mutex_lock_slowpath <- __kernfs_remove+0x148/0x204
CPU:1 ts:5916723 c05c813d  c05ca630  yield_to <- __mutex_lock_slowpath+0x314/0x358
CPU:1 ts:5916724 c05ca2d1  c05ca638  __ww_mutex_lock <- __mutex_lock_slowpath+0x31c/0x358
```

### max\_reason

Limiting which kinds of kmsg dumps are stored can be controlled via
the `max_reason` value, as defined in include/linux/kmsg\_dump.h’s
`enum kmsg_dump_reason`. For example, to store both Oopses and Panics,
`max_reason` should be set to 2 (KMSG\_DUMP\_OOPS), to store only Panics
`max_reason` should be set to 1 (KMSG\_DUMP\_PANIC). Setting this to 0
(KMSG\_DUMP\_UNDEF), means the reason filtering will be controlled by the
`printk.always_kmsg_dump` boot param: if unset, it’ll be KMSG\_DUMP\_OOPS,
otherwise KMSG\_DUMP\_MAX.

## Configurations for driver

A device driver uses `register_pstore_device` with
`struct pstore_device_info` to register to pstore/blk.

int register\_pstore\_device(struct [pstore\_device\_info](#c.pstore_device_info "pstore_device_info") \*dev)
:   register non-block device to pstore/blk

**Parameters**

`struct pstore_device_info *dev`
:   non-block device information

**Return**

* 0 - OK
* Others - something error.

void unregister\_pstore\_device(struct [pstore\_device\_info](#c.pstore_device_info "pstore_device_info") \*dev)
:   unregister non-block device from pstore/blk

**Parameters**

`struct pstore_device_info *dev`
:   non-block device information

## Compression and header

Block device is large enough for uncompressed oops data. Actually we do not
recommend data compression because pstore/blk will insert some information into
the first line of oops/panic data. For example:

```
Panic: Total 16 times
```

It means that it’s OOPS|Panic for the 16th time since the first booting.
Sometimes the number of occurrences of oops|panic since the first booting is
important to judge whether the system is stable.

The following line is inserted by pstore filesystem. For example:

```
Oops#2 Part1
```

It means that it’s OOPS for the 2nd time on the last boot.

## Reading the data

The dump data can be read from the pstore filesystem. The format for these
files is `dmesg-pstore-blk-[N]` for oops/panic front-end,
`pmsg-pstore-blk-0` for pmsg front-end and so on. The timestamp of the
dump file records the trigger time. To delete a stored record from block
device, simply unlink the respective pstore file.

## Attentions in panic read/write APIs

If on panic, the kernel is not going to run for much longer, the tasks will not
be scheduled and most kernel resources will be out of service. It
looks like a single-threaded program running on a single-core computer.

The following points require special attention for panic read/write APIs:

1. Can **NOT** allocate any memory.
   If you need memory, just allocate while the block driver is initializing
   rather than waiting until the panic.
2. Must be polled, **NOT** interrupt driven.
   No task schedule any more. The block driver should delay to ensure the write
   succeeds, but NOT sleep.
3. Can **NOT** take any lock.
   There is no other task, nor any shared resource; you are safe to break all
   locks.
4. Just use CPU to transfer.
   Do not use DMA to transfer unless you are sure that DMA will not keep lock.
5. Control registers directly.
   Please control registers directly rather than use Linux kernel resources.
   Do I/O map while initializing rather than wait until a panic occurs.
6. Reset your block device and controller if necessary.
   If you are not sure of the state of your block device and controller when
   a panic occurs, you are safe to stop and reset them.

pstore/blk supports `psblk_blkdev_info()`, which is defined in
*linux/pstore\_blk.h*, to get information of using block device, such as the
device number, sector count and start sector of the whole disk.

## pstore block internals

For developer reference, here are all the important structures and APIs:

struct psz\_buffer
:   header of zone to flush to storage

**Definition**:

```
struct psz_buffer {
#define PSZ_SIG (0x43474244) ;
    uint32_t sig;
    atomic_t datalen;
    atomic_t start;
    uint8_t data[];
};
```

**Members**

`sig`
:   signature to indicate header (PSZ\_SIG xor PSZONE-type value)

`datalen`
:   length of data in **data**

`start`
:   offset into **data** where the beginning of the stored bytes begin

`data`
:   zone data.

struct psz\_kmsg\_header
:   kmsg dump-specific header to flush to storage

**Definition**:

```
struct psz_kmsg_header {
#define PSTORE_KMSG_HEADER_MAGIC 0x4dfc3ae5 ;
    uint32_t magic;
    struct timespec64 time;
    bool compressed;
    uint32_t counter;
    enum kmsg_dump_reason reason;
    uint8_t data[];
};
```

**Members**

`magic`
:   magic num for kmsg dump header

`time`
:   kmsg dump trigger time

`compressed`
:   whether compressed

`counter`
:   kmsg dump counter

`reason`
:   the kmsg dump reason (e.g. oops, panic, etc)

`data`
:   pointer to log data

**Description**

This is a sub-header for a kmsg dump, trailing after [`psz_buffer`](#c.psz_buffer "psz_buffer").

struct pstore\_zone
:   single stored buffer

**Definition**:

```
struct pstore_zone {
    loff_t off;
    const char *name;
    enum pstore_type_id type;
    struct psz_buffer *buffer;
    struct psz_buffer *oldbuf;
    size_t buffer_size;
    bool should_recover;
    atomic_t dirty;
};
```

**Members**

`off`
:   zone offset of storage

`name`
:   front-end name for this zone

`type`
:   front-end type for this zone

`buffer`
:   pointer to data buffer managed by this zone

`oldbuf`
:   pointer to old data buffer

`buffer_size`
:   bytes in **buffer->data**

`should_recover`
:   whether this zone should recover from storage

`dirty`
:   whether the data in **buffer** dirty

**Description**

zone structure in memory.

struct psz\_context
:   all about running state of pstore/zone

**Definition**:

```
struct psz_context {
    struct pstore_zone **kpszs;
    struct pstore_zone *ppsz;
    struct pstore_zone *cpsz;
    struct pstore_zone **fpszs;
    unsigned int kmsg_max_cnt;
    unsigned int kmsg_read_cnt;
    unsigned int kmsg_write_cnt;
    unsigned int pmsg_read_cnt;
    unsigned int console_read_cnt;
    unsigned int ftrace_max_cnt;
    unsigned int ftrace_read_cnt;
    unsigned int oops_counter;
    unsigned int panic_counter;
    atomic_t recovered;
    atomic_t on_panic;
    struct mutex pstore_zone_info_lock;
    struct pstore_zone_info *pstore_zone_info;
    struct pstore_info pstore;
};
```

**Members**

`kpszs`
:   kmsg dump storage zones

`ppsz`
:   pmsg storage zone

`cpsz`
:   console storage zone

`fpszs`
:   ftrace storage zones

`kmsg_max_cnt`
:   max count of **kpszs**

`kmsg_read_cnt`
:   counter of total read kmsg dumps

`kmsg_write_cnt`
:   counter of total kmsg dump writes

`pmsg_read_cnt`
:   counter of total read pmsg zone

`console_read_cnt`
:   counter of total read console zone

`ftrace_max_cnt`
:   max count of **fpszs**

`ftrace_read_cnt`
:   counter of max read ftrace zone

`oops_counter`
:   counter of oops dumps

`panic_counter`
:   counter of panic dumps

`recovered`
:   whether finished recovering data from storage

`on_panic`
:   whether panic is happening

`pstore_zone_info_lock`
:   lock to **pstore\_zone\_info**

`pstore_zone_info`
:   information from backend

`pstore`
:   structure for pstore

enum psz\_flush\_mode
:   flush mode for `psz_zone_write()`

**Constants**

`FLUSH_NONE`
:   do not flush to storage but update data on memory

`FLUSH_PART`
:   just flush part of data including meta data to storage

`FLUSH_META`
:   just flush meta data of zone to storage

`FLUSH_ALL`
:   flush all of zone

int psz\_recovery(struct [psz\_context](#c.psz_context "psz_context") \*cxt)
:   recover data from storage

**Parameters**

`struct psz_context *cxt`
:   the context of pstore/zone

**Description**

recovery means reading data back from storage after rebooting

**Return**

0 on success, others on failure.

struct pstore\_zone\_info
:   pstore/zone back-end driver structure

**Definition**:

```
struct pstore_zone_info {
    struct module *owner;
    const char *name;
    unsigned long total_size;
    unsigned long kmsg_size;
    int max_reason;
    unsigned long pmsg_size;
    unsigned long console_size;
    unsigned long ftrace_size;
    pstore_zone_read_op read;
    pstore_zone_write_op write;
    pstore_zone_erase_op erase;
    pstore_zone_write_op panic_write;
};
```

**Members**

`owner`
:   Module which is responsible for this back-end driver.

`name`
:   Name of the back-end driver.

`total_size`
:   The total size in bytes pstore/zone can use. It must be greater
    than 4096 and be multiple of 4096.

`kmsg_size`
:   The size of oops/panic zone. Zero means disabled, otherwise,
    it must be multiple of SECTOR\_SIZE(512 Bytes).

`max_reason`
:   Maximum kmsg dump reason to store.

`pmsg_size`
:   The size of pmsg zone which is the same as **kmsg\_size**.

`console_size`
:   The size of console zone which is the same as **kmsg\_size**.

`ftrace_size`
:   The size of ftrace zone which is the same as **kmsg\_size**.

`read`
:   The general read operation. Both of the function parameters
    **size** and **offset** are relative value to storage.
    On success, the number of bytes should be returned, others
    mean error.

`write`
:   The same as **read**, but the following error number:
    -EBUSY means try to write again later.
    -ENOMSG means to try next zone.

`erase`
:   The general erase operation for device with special removing
    job. Both of the function parameters **size** and **offset** are
    relative value to storage.
    Return 0 on success and others on failure.

`panic_write`
:   The write operation only used for panic case. It’s optional
    if you do not care panic log. The parameters are relative
    value to storage.
    On success, the number of bytes should be returned, others
    excluding -ENOMSG mean error. -ENOMSG means to try next zone.

struct pstore\_device\_info
:   back-end pstore/blk driver structure.

**Definition**:

```
struct pstore_device_info {
    unsigned int flags;
    struct pstore_zone_info zone;
};
```

**Members**

`flags`
:   Refer to macro starting with PSTORE\_FLAGS defined in
    linux/pstore.h. It means what front-ends this device support.
    Zero means all backends for compatible.

`zone`
:   The [`struct pstore_zone_info`](#c.pstore_zone_info "pstore_zone_info") details.

struct pstore\_blk\_config
:   the pstore\_blk backend configuration

**Definition**:

```
struct pstore_blk_config {
    char device[80];
    enum kmsg_dump_reason max_reason;
    unsigned long kmsg_size;
    unsigned long pmsg_size;
    unsigned long console_size;
    unsigned long ftrace_size;
};
```

**Members**

`device`
:   Name of the desired block device

`max_reason`
:   Maximum kmsg dump reason to store to block device

`kmsg_size`
:   Total size of for kmsg dumps

`pmsg_size`
:   Total size of the pmsg storage area

`console_size`
:   Total size of the console storage area

`ftrace_size`
:   Total size for ftrace logging data (for all CPUs)

int pstore\_blk\_get\_config(struct [pstore\_blk\_config](#c.pstore_blk_config "pstore_blk_config") \*info)
:   get a copy of the pstore\_blk backend configuration

**Parameters**

`struct pstore_blk_config *info`
:   The sturct pstore\_blk\_config to be filled in

**Description**

Failure returns negative error code, and success returns 0.
