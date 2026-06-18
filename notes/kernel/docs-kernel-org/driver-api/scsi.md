# SCSI Interfaces Guide

> 출처(원문): https://docs.kernel.org/driver-api/scsi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# SCSI Interfaces Guide

Author:
:   James Bottomley

Author:
:   Rob Landley

## Introduction

### Protocol vs bus

Once upon a time, the Small Computer Systems Interface defined both a
parallel I/O bus and a data protocol to connect a wide variety of
peripherals (disk drives, tape drives, modems, printers, scanners,
optical drives, test equipment, and medical devices) to a host computer.

Although the old parallel (fast/wide/ultra) SCSI bus has largely fallen
out of use, the SCSI command set is more widely used than ever to
communicate with devices over a number of different buses.

The [SCSI protocol](https://www.t10.org/scsi-3.htm) is a big-endian
peer-to-peer packet based protocol. SCSI commands are 6, 10, 12, or 16
bytes long, often followed by an associated data payload.

SCSI commands can be transported over just about any kind of bus, and
are the default protocol for storage devices attached to USB, SATA, SAS,
Fibre Channel, FireWire, and ATAPI devices. SCSI packets are also
commonly exchanged over Infiniband,
TCP/IP ([iSCSI](https://en.wikipedia.org/wiki/ISCSI)), even [Parallel
ports](http://cyberelk.net/tim/parport/parscsi.html).

### Design of the Linux SCSI subsystem

The SCSI subsystem uses a three layer design, with upper, mid, and low
layers. Every operation involving the SCSI subsystem (such as reading a
sector from a disk) uses one driver at each of the 3 levels: one upper
layer driver, one lower layer driver, and the SCSI midlayer.

The SCSI upper layer provides the interface between userspace and the
kernel, in the form of block and char device nodes for I/O and ioctl().
The SCSI lower layer contains drivers for specific hardware devices.

In between is the SCSI mid-layer, analogous to a network routing layer
such as the IPv4 stack. The SCSI mid-layer routes a packet based data
protocol between the upper layer’s /dev nodes and the corresponding
devices in the lower layer. It manages command queues, provides error
handling and power management functions, and responds to ioctl()
requests.

## SCSI upper layer

The upper layer supports the user-kernel interface by providing device
nodes.

### sd (SCSI Disk)

sd (sd\_mod.o)

### sr (SCSI CD-ROM)

sr (sr\_mod.o)

### st (SCSI Tape)

st (st.o)

### sg (SCSI Generic)

sg (sg.o)

### ch (SCSI Media Changer)

ch (ch.c)

## SCSI mid layer

### SCSI midlayer implementation

#### include/scsi/scsi\_device.h

struct scsi\_vpd
:   SCSI Vital Product Data

**Definition**:

```
struct scsi_vpd {
    struct rcu_head rcu;
    int len;
    unsigned char   data[];
};
```

**Members**

`rcu`
:   For [`kfree_rcu()`](../core-api/kernel-api.html#c.kfree_rcu "kfree_rcu").

`len`
:   Length in bytes of **data**.

`data`
:   VPD data as defined in various T10 SCSI standard documents.

shost\_for\_each\_device

`shost_for_each_device (sdev, shost)`

> iterate over all devices of a host

**Parameters**

`sdev`
:   the `struct scsi_device` to use as a cursor

`shost`
:   the `struct scsi_host` to iterate over

**Description**

Iterator that returns each device attached to **shost**. This loop
takes a reference on each device and releases it at the end. If
you break out of the loop, you must call scsi\_device\_put(sdev).

\_\_shost\_for\_each\_device

`__shost_for_each_device (sdev, shost)`

> iterate over all devices of a host (UNLOCKED)

**Parameters**

`sdev`
:   the `struct scsi_device` to use as a cursor

`shost`
:   the `struct scsi_host` to iterate over

**Description**

Iterator that returns each device attached to **shost**. It does \_not\_
take a reference on the scsi\_device, so the whole loop must be
protected by shost->host\_lock.

**Note**

The only reason to use this is because you need to access the
device list in interrupt context. Otherwise you really want to use
shost\_for\_each\_device instead.

bool scsi\_device\_is\_pseudo\_dev(struct scsi\_device \*sdev)
:   Whether a device is a pseudo SCSI device.

**Parameters**

`struct scsi_device *sdev`
:   SCSI device to examine

**Description**

A pseudo SCSI device can be used to allocate SCSI commands but does not show
up in sysfs. Additionally, the logical unit information in **\*sdev** is made up.

This function tests the LUN number instead of comparing **sdev** with
**sdev->host->pseudo\_sdev** because this function may be called before
**sdev->host->pseudo\_sdev** has been initialized.

int scsi\_device\_supports\_vpd(struct scsi\_device \*sdev)
:   test if a device supports VPD pages

**Parameters**

`struct scsi_device *sdev`
:   the `struct scsi_device` to test

**Description**

If the ‘try\_vpd\_pages’ flag is set it takes precedence.
Otherwise we will assume VPD pages are supported if the
SCSI level is at least SPC-3 and ‘skip\_vpd\_pages’ is not set.

#### drivers/scsi/scsi.c

Main file for the SCSI midlayer.

int scsi\_change\_queue\_depth(struct scsi\_device \*sdev, int depth)
:   change a device’s queue depth

**Parameters**

`struct scsi_device *sdev`
:   SCSI Device in question

`int depth`
:   number of commands allowed to be queued to the driver

**Description**

Sets the device queue depth and returns the new value.

int scsi\_track\_queue\_full(struct scsi\_device \*sdev, int depth)
:   track QUEUE\_FULL events to adjust queue depth

**Parameters**

`struct scsi_device *sdev`
:   SCSI Device in question

`int depth`
:   Current number of outstanding SCSI commands on this device,
    not counting the one returned as QUEUE\_FULL.

**Description**

This function will track successive QUEUE\_FULL events on a
:   specific SCSI device to determine if and when there is a
    need to adjust the queue depth on the device.

Lock Status: None held on entry

**Return**

* 0 - No change needed
* >0 - Adjust queue depth to this new depth,
* -1 - Drop back to untagged operation using host->cmd\_per\_lun as the
  untagged command depth

**Notes**

Low level drivers may call this at any time and we will do
:   “The Right Thing.” We are interrupt context safe.

int scsi\_get\_vpd\_page(struct scsi\_device \*sdev, u8 page, unsigned char \*buf, int buf\_len)
:   Get Vital Product Data from a SCSI device

**Parameters**

`struct scsi_device *sdev`
:   The device to ask

`u8 page`
:   Which Vital Product Data to return

`unsigned char *buf`
:   where to store the VPD

`int buf_len`
:   number of bytes in the VPD buffer area

**Description**

SCSI devices may optionally supply Vital Product Data. Each ‘page’
of VPD is defined in the appropriate SCSI document (eg SPC, SBC).
If the device supports this VPD page, this routine fills **buf**
with the data from that page and return 0. If the VPD page is not
supported or its content cannot be retrieved, -EINVAL is returned.

int scsi\_report\_opcode(struct scsi\_device \*sdev, unsigned char \*buffer, unsigned int len, unsigned char opcode, unsigned short sa)
:   Find out if a given command is supported

**Parameters**

`struct scsi_device *sdev`
:   scsi device to query

`unsigned char *buffer`
:   scratch buffer (must be at least 20 bytes long)

`unsigned int len`
:   length of buffer

`unsigned char opcode`
:   opcode for the command to look up

`unsigned short sa`
:   service action for the command to look up

**Description**

Uses the REPORT SUPPORTED OPERATION CODES to check support for the
command identified with **opcode** and **sa**. If the command does not
have a service action, **sa** must be 0. Returns -EINVAL if RSOC fails,
0 if the command is not supported and 1 if the device claims to
support the command.

int scsi\_device\_get(struct scsi\_device \*sdev)
:   get an additional reference to a scsi\_device

**Parameters**

`struct scsi_device *sdev`
:   device to get a reference to

**Description**

Gets a reference to the scsi\_device and increments the use count
of the underlying LLDD module. You must hold host\_lock of the
parent Scsi\_Host or already have a reference when calling this.

This will fail if a device is deleted or cancelled, or when the LLD module
is in the process of being unloaded.

void scsi\_device\_put(struct scsi\_device \*sdev)
:   release a reference to a scsi\_device

**Parameters**

`struct scsi_device *sdev`
:   device to release a reference on.

**Description**

Release a reference to the scsi\_device and decrements the use
count of the underlying LLDD module. The device is freed once the last
user vanishes.

void starget\_for\_each\_device(struct scsi\_target \*starget, void \*data, void (\*fn)(struct scsi\_device\*, void\*))
:   helper to walk all devices of a target

**Parameters**

`struct scsi_target *starget`
:   target whose devices we want to iterate over.

`void *data`
:   Opaque passed to each function call.

`void (*fn)(struct scsi_device *, void *)`
:   Function to call on each device

**Description**

This traverses over each device of **starget**. The devices have
a reference that must be released by scsi\_host\_put when breaking
out of the loop.

void \_\_starget\_for\_each\_device(struct scsi\_target \*starget, void \*data, void (\*fn)(struct scsi\_device\*, void\*))
:   helper to walk all devices of a target (UNLOCKED)

**Parameters**

`struct scsi_target *starget`
:   target whose devices we want to iterate over.

`void *data`
:   parameter for callback **`fn()`**

`void (*fn)(struct scsi_device *, void *)`
:   callback function that is invoked for each device

**Description**

This traverses over each device of **starget**. It does \_not\_
take a reference on the scsi\_device, so the whole loop must be
protected by shost->host\_lock.

**Note**

The only reason why drivers would want to use this is because
they need to access the device list in irq context. Otherwise you
really want to use starget\_for\_each\_device instead.

struct scsi\_device \*\_\_scsi\_device\_lookup\_by\_target(struct scsi\_target \*starget, u64 lun)
:   find a device given the target (UNLOCKED)

**Parameters**

`struct scsi_target *starget`
:   SCSI target pointer

`u64 lun`
:   SCSI Logical Unit Number

**Description**

Looks up the scsi\_device with the specified **lun** for a given
**starget**. The returned scsi\_device does not have an additional
reference. You must hold the host’s host\_lock over this call and
any access to the returned scsi\_device. A scsi\_device in state
SDEV\_DEL is skipped.

**Note**

The only reason why drivers should use this is because
they need to access the device list in irq context. Otherwise you
really want to use scsi\_device\_lookup\_by\_target instead.

struct scsi\_device \*scsi\_device\_lookup\_by\_target(struct scsi\_target \*starget, u64 lun)
:   find a device given the target

**Parameters**

`struct scsi_target *starget`
:   SCSI target pointer

`u64 lun`
:   SCSI Logical Unit Number

**Description**

Looks up the scsi\_device with the specified **lun** for a given
**starget**. The returned scsi\_device has an additional reference that
needs to be released with scsi\_device\_put once you’re done with it.

struct scsi\_device \*\_\_scsi\_device\_lookup(struct Scsi\_Host \*shost, uint channel, uint id, u64 lun)
:   find a device given the host (UNLOCKED)

**Parameters**

`struct Scsi_Host *shost`
:   SCSI host pointer

`uint channel`
:   SCSI channel (zero if only one channel)

`uint id`
:   SCSI target number (physical unit number)

`u64 lun`
:   SCSI Logical Unit Number

**Description**

Looks up the scsi\_device with the specified **channel**, **id**, **lun**
for a given host. The returned scsi\_device does not have an additional
reference. You must hold the host’s host\_lock over this call and any access
to the returned scsi\_device.

**Note**

The only reason why drivers would want to use this is because
they need to access the device list in irq context. Otherwise you
really want to use scsi\_device\_lookup instead.

struct scsi\_device \*scsi\_device\_lookup(struct Scsi\_Host \*shost, uint channel, uint id, u64 lun)
:   find a device given the host

**Parameters**

`struct Scsi_Host *shost`
:   SCSI host pointer

`uint channel`
:   SCSI channel (zero if only one channel)

`uint id`
:   SCSI target number (physical unit number)

`u64 lun`
:   SCSI Logical Unit Number

**Description**

Looks up the scsi\_device with the specified **channel**, **id**, **lun**
for a given host. The returned scsi\_device has an additional reference that
needs to be released with scsi\_device\_put once you’re done with it.

#### drivers/scsi/scsicam.c

[SCSI Common Access
Method](http://www.t10.org/ftp/t10/drafts/cam/cam-r12b.pdf) support
functions, for use with HDIO\_GETGEO, etc.

unsigned char \*scsi\_bios\_ptable(struct gendisk \*dev)
:   Read PC partition table out of first sector of device.

**Parameters**

`struct gendisk *dev`
:   from this device

**Description**

Reads the first sector from the device and returns `0x42` bytes
:   starting at offset `0x1be`.

**Return**

partition table in kmalloc(GFP\_KERNEL) memory, or NULL on error.

bool scsi\_partsize(struct gendisk \*disk, sector\_t capacity, int geom[3])
:   Parse cylinders/heads/sectors from PC partition table

**Parameters**

`struct gendisk *disk`
:   gendisk of the disk to parse

`sector_t capacity`
:   size of the disk in sectors

`int geom[3]`
:   output in form of [hds, cylinders, sectors]

**Description**

Determine the BIOS mapping/geometry used to create the partition
table, storing the results in **geom**.

**Return**

`false` on failure, `true` on success.

int scsicam\_bios\_param(struct gendisk \*disk, sector\_t capacity, int \*ip)
:   Determine geometry of a disk in cylinders/heads/sectors.

**Parameters**

`struct gendisk *disk`
:   which device

`sector_t capacity`
:   size of the disk in sectors

`int *ip`
:   return value: ip[0]=heads, ip[1]=sectors, ip[2]=cylinders

**Description**

determine the BIOS mapping/geometry used for a drive in a
:   SCSI-CAM system, storing the results in ip as required
    by the HDIO\_GETGEO ioctl().

**Return**

-1 on failure, 0 on success.

#### drivers/scsi/scsi\_error.c

Common SCSI error/timeout handling routines.

void scsi\_schedule\_eh(struct Scsi\_Host \*shost)
:   schedule EH for SCSI host

**Parameters**

`struct Scsi_Host *shost`
:   SCSI host to invoke error handling on.

**Description**

Schedule SCSI EH without scmd.

int scsi\_block\_when\_processing\_errors(struct scsi\_device \*sdev)
:   Prevent cmds from being queued.

**Parameters**

`struct scsi_device *sdev`
:   Device on which we are performing recovery.

**Description**

> We block until the host is out of error recovery, and then check to
> see whether the host or the device is offline.

Return value:
:   0 when dev was taken offline by error recovery. 1 OK to proceed.

enum scsi\_disposition scsi\_check\_sense(struct scsi\_cmnd \*scmd)
:   Examine scsi cmd sense

**Parameters**

`struct scsi_cmnd *scmd`
:   Cmd to have sense checked.

**Description**

Return value:
:   SUCCESS or FAILED or NEEDS\_RETRY or ADD\_TO\_MLQUEUE

**Notes**

> When a deferred error is detected the current command has
> not been executed and needs retrying.

void scsi\_eh\_prep\_cmnd(struct scsi\_cmnd \*scmd, struct scsi\_eh\_save \*ses, unsigned char \*cmnd, int cmnd\_size, unsigned sense\_bytes)
:   Save a scsi command info as part of error recovery

**Parameters**

`struct scsi_cmnd *scmd`
:   SCSI command structure to hijack

`struct scsi_eh_save *ses`
:   structure to save restore information

`unsigned char *cmnd`
:   CDB to send. Can be NULL if no new cmnd is needed

`int cmnd_size`
:   size in bytes of **cmnd** (must be <= MAX\_COMMAND\_SIZE)

`unsigned sense_bytes`
:   size of sense data to copy. or 0 (if != 0 **cmnd** is ignored)

**Description**

This function is used to save a scsi command information before re-execution
as part of the error recovery process. If **sense\_bytes** is 0 the command
sent must be one that does not transfer any data. If **sense\_bytes** != 0
**cmnd** is ignored and this functions sets up a REQUEST\_SENSE command
and cmnd buffers to read **sense\_bytes** into **scmd->sense\_buffer**.

void scsi\_eh\_restore\_cmnd(struct scsi\_cmnd \*scmd, struct scsi\_eh\_save \*ses)
:   Restore a scsi command info as part of error recovery

**Parameters**

`struct scsi_cmnd* scmd`
:   SCSI command structure to restore

`struct scsi_eh_save *ses`
:   saved information from a coresponding call to scsi\_eh\_prep\_cmnd

**Description**

Undo any damage done by above [`scsi_eh_prep_cmnd()`](#c.scsi_eh_prep_cmnd "scsi_eh_prep_cmnd").

void scsi\_eh\_finish\_cmd(struct scsi\_cmnd \*scmd, struct list\_head \*done\_q)
:   Handle a cmd that eh is finished with.

**Parameters**

`struct scsi_cmnd *scmd`
:   Original SCSI cmd that eh has finished.

`struct list_head *done_q`
:   Queue for processed commands.

**Notes**

> We don’t want to use the normal command completion while we are are
> still handling errors - it may cause other commands to be queued,
> and that would disturb what we are doing. Thus we really want to
> keep a list of pending commands for final completion, and once we
> are ready to leave error handling we handle completion for real.

int scsi\_eh\_get\_sense(struct list\_head \*work\_q, struct list\_head \*done\_q)
:   Get device sense data.

**Parameters**

`struct list_head *work_q`
:   Queue of commands to process.

`struct list_head *done_q`
:   Queue of processed commands.

**Description**

> See if we need to request sense information. if so, then get it
> now, so we have a better idea of what to do.

**Notes**

> This has the unfortunate side effect that if a shost adapter does
> not automatically request sense information, we end up shutting
> it down before we request it.
>
> All drivers should request sense information internally these days,
> so for now all I have to say is tough noogies if you end up in here.
>
> XXX: Long term this code should go away, but that needs an audit of
> :   all LLDDs first.

void scsi\_eh\_ready\_devs(struct Scsi\_Host \*shost, struct list\_head \*work\_q, struct list\_head \*done\_q)
:   check device ready state and recover if not.

**Parameters**

`struct Scsi_Host *shost`
:   host to be recovered.

`struct list_head *work_q`
:   `list_head` for pending commands.

`struct list_head *done_q`
:   `list_head` for processed commands.

void scsi\_eh\_flush\_done\_q(struct list\_head \*done\_q)
:   finish processed commands or retry them.

**Parameters**

`struct list_head *done_q`
:   list\_head of processed commands.

void scsi\_report\_bus\_reset(struct Scsi\_Host \*shost, int channel)
:   report bus reset observed

**Parameters**

`struct Scsi_Host *shost`
:   Host in question

`int channel`
:   channel on which reset was observed.

**Description**

Utility function used by low-level drivers to report that
they have observed a bus reset on the bus being handled.

Lock status: Host lock must be held.

**Return**

Nothing

**Notes**

This only needs to be called if the reset is one which
:   originates from an unknown location. Resets originated
    by the mid-level itself don’t need to call this, but there
    should be no harm.

    The main purpose of this is to make sure that a CHECK\_CONDITION
    is properly treated.

void scsi\_report\_device\_reset(struct Scsi\_Host \*shost, int channel, int target)
:   report device reset observed

**Parameters**

`struct Scsi_Host *shost`
:   Host in question

`int channel`
:   channel on which reset was observed

`int target`
:   target on which reset was observed

**Description**

Utility function used by low-level drivers to report that
they have observed a device reset on the device being handled.

Lock status: Host lock must be held

**Return**

Nothing

**Notes**

This only needs to be called if the reset is one which
:   originates from an unknown location. Resets originated
    by the mid-level itself don’t need to call this, but there
    should be no harm.

    The main purpose of this is to make sure that a CHECK\_CONDITION
    is properly treated.

bool scsi\_get\_sense\_info\_fld(const u8 \*sense\_buffer, int sb\_len, u64 \*info\_out)
:   get information field from sense data (either fixed or descriptor format)

**Parameters**

`const u8 *sense_buffer`
:   byte array of sense data

`int sb_len`
:   number of valid bytes in sense\_buffer

`u64 *info_out`
:   pointer to 64 integer where 8 or 4 byte information
    field will be placed if found.

**Description**

Return value:
:   true if information field found, false if not found.

#### drivers/scsi/scsi\_devinfo.c

Manage scsi\_dev\_info\_list, which tracks blacklisted and whitelisted
devices.

int scsi\_dev\_info\_list\_add\_keyed(int compatible, char \*vendor, char \*model, char \*strflags, blist\_flags\_t flags, enum scsi\_devinfo\_key key)
:   add one dev\_info list entry.

**Parameters**

`int compatible`
:   if true, null terminate short strings. Otherwise space pad.

`char *vendor`
:   vendor string

`char *model`
:   model (product) string

`char *strflags`
:   integer string

`blist_flags_t flags`
:   if strflags NULL, use this flag value

`enum scsi_devinfo_key key`
:   specify list to use

**Description**

> Create and add one dev\_info entry for **vendor**, **model**,
> **strflags** or **flag** in list specified by **key**. If **compatible**,
> add to the tail of the list, do not space pad, and set
> devinfo->compatible. The scsi\_static\_device\_list entries are
> added with **compatible** 1 and **clfags** NULL.

**Return**

0 OK, -error on failure.

blist\_flags\_t scsi\_get\_device\_flags\_keyed(struct scsi\_device \*sdev, const unsigned char \*vendor, const unsigned char \*model, enum scsi\_devinfo\_key key)
:   get device specific flags from the dynamic device list

**Parameters**

`struct scsi_device *sdev`
:   `scsi_device` to get flags for

`const unsigned char *vendor`
:   vendor name

`const unsigned char *model`
:   model name

`enum scsi_devinfo_key key`
:   list to look up

**Description**

> Search the scsi\_dev\_info\_list specified by **key** for an entry
> matching **vendor** and **model**, if found, return the matching
> flags value, else return the host or global default settings.
> Called during scan time.

int scsi\_dev\_info\_add\_list(enum scsi\_devinfo\_key key, const char \*name)
:   add a new devinfo list

**Parameters**

`enum scsi_devinfo_key key`
:   key of the list to add

`const char *name`
:   Name of the list to add (for /proc/scsi/device\_info)

**Description**

Adds the requested list, returns zero on success, -EEXIST if the
key is already registered to a list, or other error on failure.

int scsi\_dev\_info\_remove\_list(enum scsi\_devinfo\_key key)
:   destroy an added devinfo list

**Parameters**

`enum scsi_devinfo_key key`
:   key of the list to destroy

**Description**

Iterates over the entire list first, freeing all the values, then
frees the list itself. Returns 0 on success or -EINVAL if the key
can’t be found.

#### drivers/scsi/scsi\_ioctl.c

Handle ioctl() calls for SCSI devices.

int scsi\_set\_medium\_removal(struct scsi\_device \*sdev, char state)
:   send command to allow or prevent medium removal

**Parameters**

`struct scsi_device *sdev`
:   target scsi device

`char state`
:   removal state to set (prevent or allow)

**Return**

* `0` if **sdev** is not removable or not lockable or successful.
* non-`0` is a SCSI result code if > 0 or kernel error code if < 0.
* Sets **sdev->locked** to the new state on success.

bool scsi\_cmd\_allowed(unsigned char \*cmd, bool open\_for\_write)
:   Check if the given command is allowed.

**Parameters**

`unsigned char *cmd`
:   SCSI command to check

`bool open_for_write`
:   is the file / block device opened for writing?

**Description**

Only a subset of commands are allowed for unprivileged users. Commands used
to format the media, update the firmware, etc. are not permitted.

**Return**

`true` if the cmd is allowed, otherwise **false**.

int scsi\_ioctl(struct scsi\_device \*sdev, bool open\_for\_write, int cmd, void \_\_user \*arg)
:   Dispatch ioctl to scsi device

**Parameters**

`struct scsi_device *sdev`
:   scsi device receiving ioctl

`bool open_for_write`
:   is the file / block device opened for writing?

`int cmd`
:   which ioctl is it

`void __user *arg`
:   data associated with ioctl

**Description**

The [`scsi_ioctl()`](#c.scsi_ioctl "scsi_ioctl") function differs from most ioctls in that it
does not take a major/minor number as the dev field. Rather, it takes
a pointer to a `struct scsi_device`.

**Return**

varies depending on the **cmd**

int scsi\_ioctl\_block\_when\_processing\_errors(struct scsi\_device \*sdev, int cmd, bool ndelay)
:   prevent commands from being queued

**Parameters**

`struct scsi_device *sdev`
:   target scsi device

`int cmd`
:   which ioctl is it

`bool ndelay`
:   no delay (non-blocking)

**Description**

We can process a reset even when a device isn’t fully operable.

**Return**

`0` on success, <0 error code.

#### drivers/scsi/scsi\_lib.c

SCSI queuing library.

void scsi\_failures\_reset\_retries(struct scsi\_failures \*failures)
:   reset all failures to zero

**Parameters**

`struct scsi_failures *failures`
:   `struct scsi_failures` with specific failure modes set

int scsi\_execute\_cmd(struct scsi\_device \*sdev, const unsigned char \*cmd, blk\_opf\_t opf, void \*buffer, unsigned int bufflen, int timeout, int ml\_retries, const struct scsi\_exec\_args \*args)
:   insert request and wait for the result

**Parameters**

`struct scsi_device *sdev`
:   scsi\_device

`const unsigned char *cmd`
:   scsi command

`blk_opf_t opf`
:   block layer request cmd\_flags

`void *buffer`
:   data buffer

`unsigned int bufflen`
:   len of buffer

`int timeout`
:   request timeout in HZ

`int ml_retries`
:   number of times SCSI midlayer will retry request

`const struct scsi_exec_args *args`
:   Optional args. See `struct definition` for field descriptions

**Description**

Returns the scsi\_cmnd result field if a command was executed, or a negative
Linux error code if we didn’t get that far.

blk\_status\_t scsi\_alloc\_sgtables(struct scsi\_cmnd \*cmd)
:   Allocate and initialize data and integrity scatterlists

**Parameters**

`struct scsi_cmnd *cmd`
:   SCSI command data structure to initialize.

**Description**

Initializes **cmd->sdb** and also **cmd->prot\_sdb** if data integrity is enabled
for **cmd**.

**Return**

* BLK\_STS\_OK - on success
* BLK\_STS\_RESOURCE - if the failure is retryable
* BLK\_STS\_IOERR - if the failure is fatal

struct request \*scsi\_alloc\_request(struct request\_queue \*q, blk\_opf\_t opf, blk\_mq\_req\_flags\_t flags)
:   allocate a block request and partially initialize its `scsi_cmnd`

**Parameters**

`struct request_queue *q`
:   the device’s request queue

`blk_opf_t opf`
:   the request operation code

`blk_mq_req_flags_t flags`
:   block layer allocation flags

**Return**

`struct request` pointer on success or `NULL` on failure

struct scsi\_cmnd \*scsi\_get\_internal\_cmd(struct scsi\_device \*sdev, enum dma\_data\_direction data\_direction, blk\_mq\_req\_flags\_t flags)
:   Allocate an internal SCSI command.

**Parameters**

`struct scsi_device *sdev`
:   SCSI device from which to allocate the command

`enum dma_data_direction data_direction`
:   Data direction for the allocated command

`blk_mq_req_flags_t flags`
:   request allocation flags, e.g. BLK\_MQ\_REQ\_RESERVED or
    BLK\_MQ\_REQ\_NOWAIT.

**Description**

Allocates a SCSI command for internal LLDD use.

void scsi\_put\_internal\_cmd(struct scsi\_cmnd \*scmd)
:   Free an internal SCSI command.

**Parameters**

`struct scsi_cmnd *scmd`
:   SCSI command to be freed

struct scsi\_device \*scsi\_device\_from\_queue(struct request\_queue \*q)
:   return sdev associated with a request\_queue

**Parameters**

`struct request_queue *q`
:   The request queue to return the sdev from

**Description**

Return the sdev associated with a request queue or NULL if the
request\_queue does not reference a SCSI device.

void scsi\_block\_requests(struct Scsi\_Host \*shost)
:   Utility function used by low-level drivers to prevent further commands from being queued to the device.

**Parameters**

`struct Scsi_Host *shost`
:   host in question

**Description**

There is no timer nor any other means by which the requests get unblocked
other than the low-level driver calling [`scsi_unblock_requests()`](#c.scsi_unblock_requests "scsi_unblock_requests").

void scsi\_unblock\_requests(struct Scsi\_Host \*shost)
:   Utility function used by low-level drivers to allow further commands to be queued to the device.

**Parameters**

`struct Scsi_Host *shost`
:   host in question

**Description**

There is no timer nor any other means by which the requests get unblocked
other than the low-level driver calling [`scsi_unblock_requests()`](#c.scsi_unblock_requests "scsi_unblock_requests"). This is done
as an API function so that changes to the internals of the scsi mid-layer
won’t require wholesale changes to drivers that use this feature.

int scsi\_mode\_select(struct scsi\_device \*sdev, int pf, int sp, unsigned char \*buffer, int len, int timeout, int retries, struct scsi\_mode\_data \*data, struct scsi\_sense\_hdr \*sshdr)
:   issue a mode select

**Parameters**

`struct scsi_device *sdev`
:   SCSI device to be queried

`int pf`
:   Page format bit (1 == standard, 0 == vendor specific)

`int sp`
:   Save page bit (0 == don’t save, 1 == save)

`unsigned char *buffer`
:   request buffer (may not be smaller than eight bytes)

`int len`
:   length of request buffer.

`int timeout`
:   command timeout

`int retries`
:   number of retries before failing

`struct scsi_mode_data *data`
:   returns a structure abstracting the mode header data

`struct scsi_sense_hdr *sshdr`
:   place to put sense data (or NULL if no sense to be collected).
    must be SCSI\_SENSE\_BUFFERSIZE big.

**Description**

> Returns zero if successful; negative error number or scsi
> status on error

int scsi\_mode\_sense(struct scsi\_device \*sdev, int dbd, int modepage, int subpage, unsigned char \*buffer, int len, int timeout, int retries, struct scsi\_mode\_data \*data, struct scsi\_sense\_hdr \*sshdr)
:   issue a mode sense, falling back from 10 to six bytes if necessary.

**Parameters**

`struct scsi_device *sdev`
:   SCSI device to be queried

`int dbd`
:   set to prevent mode sense from returning block descriptors

`int modepage`
:   mode page being requested

`int subpage`
:   sub-page of the mode page being requested

`unsigned char *buffer`
:   request buffer (may not be smaller than eight bytes)

`int len`
:   length of request buffer.

`int timeout`
:   command timeout

`int retries`
:   number of retries before failing

`struct scsi_mode_data *data`
:   returns a structure abstracting the mode header data

`struct scsi_sense_hdr *sshdr`
:   place to put sense data (or NULL if no sense to be collected).
    must be SCSI\_SENSE\_BUFFERSIZE big.

**Description**

> Returns zero if successful, or a negative error number on failure

int scsi\_test\_unit\_ready(struct scsi\_device \*sdev, int timeout, int retries, struct scsi\_sense\_hdr \*sshdr)
:   test if unit is ready

**Parameters**

`struct scsi_device *sdev`
:   scsi device to change the state of.

`int timeout`
:   command timeout

`int retries`
:   number of retries before failing

`struct scsi_sense_hdr *sshdr`
:   outpout pointer for decoded sense information.

**Description**

> Returns zero if successful or an error if TUR failed. For
> removable media, UNIT\_ATTENTION sets ->changed flag.

int scsi\_device\_set\_state(struct scsi\_device \*sdev, enum scsi\_device\_state state)
:   Take the given device through the device state model.

**Parameters**

`struct scsi_device *sdev`
:   scsi device to change the state of.

`enum scsi_device_state state`
:   state to change to.

**Description**

> Returns zero if successful or an error if the requested
> transition is illegal.

void sdev\_evt\_send(struct scsi\_device \*sdev, struct scsi\_event \*evt)
:   send asserted event to uevent thread

**Parameters**

`struct scsi_device *sdev`
:   scsi\_device event occurred on

`struct scsi_event *evt`
:   event to send

**Description**

> Assert scsi device event asynchronously.

struct scsi\_event \*sdev\_evt\_alloc(enum scsi\_device\_event evt\_type, gfp\_t gfpflags)
:   allocate a new scsi event

**Parameters**

`enum scsi_device_event evt_type`
:   type of event to allocate

`gfp_t gfpflags`
:   GFP flags for allocation

**Description**

> Allocates and returns a new scsi\_event.

void sdev\_evt\_send\_simple(struct scsi\_device \*sdev, enum scsi\_device\_event evt\_type, gfp\_t gfpflags)
:   send asserted event to uevent thread

**Parameters**

`struct scsi_device *sdev`
:   scsi\_device event occurred on

`enum scsi_device_event evt_type`
:   type of event to send

`gfp_t gfpflags`
:   GFP flags for allocation

**Description**

> Assert scsi device event asynchronously, given an event type.

int scsi\_device\_quiesce(struct scsi\_device \*sdev)
:   Block all commands except power management.

**Parameters**

`struct scsi_device *sdev`
:   scsi device to quiesce.

**Description**

> This works by trying to transition to the SDEV\_QUIESCE state
> (which must be a legal transition). When the device is in this
> state, only power management requests will be accepted, all others will
> be deferred.
>
> Must be called with user context, may sleep.
>
> Returns zero if successful or an error if not.

void scsi\_device\_resume(struct scsi\_device \*sdev)
:   Restart user issued commands to a quiesced device.

**Parameters**

`struct scsi_device *sdev`
:   scsi device to resume.

**Description**

> Moves the device from quiesced back to running and restarts the
> queues.
>
> Must be called with user context, may sleep.

int scsi\_internal\_device\_block\_nowait(struct scsi\_device \*sdev)
:   try to transition to the SDEV\_BLOCK state

**Parameters**

`struct scsi_device *sdev`
:   device to block

**Description**

Pause SCSI command processing on the specified device. Does not sleep.

Returns zero if successful or a negative error code upon failure.

**Notes**

This routine transitions the device to the SDEV\_BLOCK state (which must be
a legal transition). When the device is in this state, command processing
is paused until the device leaves the SDEV\_BLOCK state. See also
[`scsi_internal_device_unblock_nowait()`](#c.scsi_internal_device_unblock_nowait "scsi_internal_device_unblock_nowait").

int scsi\_internal\_device\_unblock\_nowait(struct scsi\_device \*sdev, enum scsi\_device\_state new\_state)
:   resume a device after a block request

**Parameters**

`struct scsi_device *sdev`
:   device to resume

`enum scsi_device_state new_state`
:   state to set the device to after unblocking

**Description**

Restart the device queue for a previously suspended SCSI device. Does not
sleep.

Returns zero if successful or a negative error code upon failure.

**Notes**

This routine transitions the device to the SDEV\_RUNNING state or to one of
the offline states (which must be a legal transition) allowing the midlayer
to goose the queue for this device.

void scsi\_block\_targets(struct Scsi\_Host \*shost, struct [device](infrastructure.html#c.device "device") \*dev)
:   transition all SCSI child devices to SDEV\_BLOCK state

**Parameters**

`struct Scsi_Host *shost`
:   the Scsi\_Host to which this device belongs

`struct device *dev`
:   a parent device of one or more scsi\_target devices

**Description**

Iterate over all children of **dev**, which should be scsi\_target devices,
and switch all subordinate scsi devices to SDEV\_BLOCK state. Wait for
ongoing `scsi_queue_rq()` calls to finish. May sleep.

**Note**

**dev** must not itself be a scsi\_target device.

int scsi\_host\_block(struct Scsi\_Host \*shost)
:   Try to transition all logical units to the SDEV\_BLOCK state

**Parameters**

`struct Scsi_Host *shost`
:   device to block

**Description**

Pause SCSI command processing for all logical units associated with the SCSI
host and wait until pending `scsi_queue_rq()` calls have finished.

Returns zero if successful or a negative error code upon failure.

void \*scsi\_kmap\_atomic\_sg(struct scatterlist \*sgl, int sg\_count, size\_t \*offset, size\_t \*len)
:   find and atomically map an sg-elemnt

**Parameters**

`struct scatterlist *sgl`
:   scatter-gather list

`int sg_count`
:   number of segments in sg

`size_t *offset`
:   offset in bytes into sg, on return offset into the mapped area

`size_t *len`
:   bytes to map, on return number of bytes mapped

**Description**

Returns virtual address of the start of the mapped page

void scsi\_kunmap\_atomic\_sg(void \*virt)
:   atomically unmap a virtual address, previously mapped with scsi\_kmap\_atomic\_sg

**Parameters**

`void *virt`
:   virtual address to be unmapped

int scsi\_vpd\_lun\_id(struct scsi\_device \*sdev, char \*id, size\_t id\_len)
:   return a unique device identification

**Parameters**

`struct scsi_device *sdev`
:   SCSI device

`char *id`
:   buffer for the identification

`size_t id_len`
:   length of the buffer

**Description**

Copies a unique device identification into **id** based
on the information in the VPD page 0x83 of the device.
The string will be formatted as a SCSI name string.

Returns the length of the identification or error on failure.
If the identifier is longer than the supplied buffer the actual
identifier length is returned and the buffer is not zero-padded.

int scsi\_vpd\_lun\_serial(struct scsi\_device \*sdev, char \*sn, size\_t sn\_size)
:   return a unique device serial number

**Parameters**

`struct scsi_device *sdev`
:   SCSI device

`char *sn`
:   buffer for the serial number

`size_t sn_size`
:   size of the buffer

**Description**

Copies the device serial number into **sn** based on the information in
the VPD page 0x80 of the device. The string will be null terminated
and have leading and trailing whitespace stripped.

Returns the length of the serial number or error on failure.

int scsi\_vpd\_tpg\_id(struct scsi\_device \*sdev, int \*rel\_id)
:   return a target port group identifier

**Parameters**

`struct scsi_device *sdev`
:   SCSI device

`int *rel_id`
:   pointer to return relative target port in if not `NULL`

**Description**

Returns the Target Port Group identifier from the information
from VPD page 0x83 of the device.
Optionally sets **rel\_id** to the relative target port on success.

**Return**

the identifier or error on failure.

void scsi\_build\_sense(struct scsi\_cmnd \*scmd, int desc, u8 key, u8 asc, u8 ascq)
:   build sense data for a command

**Parameters**

`struct scsi_cmnd *scmd`
:   scsi command for which the sense should be formatted

`int desc`
:   Sense format (non-zero == descriptor format,
    0 == fixed format)

`u8 key`
:   Sense key

`u8 asc`
:   Additional sense code

`u8 ascq`
:   Additional sense code qualifier

#### drivers/scsi/scsi\_lib\_dma.c

SCSI library functions depending on DMA (map and unmap scatter-gather
lists).

int scsi\_dma\_map(struct scsi\_cmnd \*cmd)
:   perform DMA mapping against command’s sg lists

**Parameters**

`struct scsi_cmnd *cmd`
:   scsi command

**Description**

Returns the number of sg lists actually used, zero if the sg lists
is NULL, or -ENOMEM if the mapping failed.

void scsi\_dma\_unmap(struct scsi\_cmnd \*cmd)
:   unmap command’s sg lists mapped by scsi\_dma\_map

**Parameters**

`struct scsi_cmnd *cmd`
:   scsi command

#### drivers/scsi/scsi\_proc.c

The functions in this file provide an interface between the PROC file
system and the SCSI device drivers It is mainly used for debugging,
statistics and to pass information directly to the lowlevel driver. I.E.
plumbing to manage /proc/scsi/\*

struct scsi\_proc\_entry
:   (host template, SCSI proc dir) association

**Definition**:

```
struct scsi_proc_entry {
    struct list_head        entry;
    const struct scsi_host_template *sht;
    struct proc_dir_entry   *proc_dir;
    unsigned int            present;
};
```

**Members**

`entry`
:   entry in scsi\_proc\_list.

`sht`
:   SCSI host template associated with the procfs directory.

`proc_dir`
:   procfs directory associated with the SCSI host template.

`present`
:   Number of SCSI hosts instantiated for **sht**.

struct proc\_dir\_entry \*scsi\_template\_proc\_dir(const struct scsi\_host\_template \*sht)
:   returns the procfs dir for a SCSI host template

**Parameters**

`const struct scsi_host_template *sht`
:   SCSI host template pointer.

int scsi\_proc\_hostdir\_add(const struct scsi\_host\_template \*sht)
:   Create directory in /proc for a scsi host

**Parameters**

`const struct scsi_host_template *sht`
:   owner of this directory

**Description**

Sets sht->proc\_dir to the new directory.

void scsi\_proc\_hostdir\_rm(const struct scsi\_host\_template \*sht)
:   remove directory in /proc for a scsi host

**Parameters**

`const struct scsi_host_template *sht`
:   owner of directory

void scsi\_proc\_host\_add(struct Scsi\_Host \*shost)
:   Add entry for this host to appropriate /proc dir

**Parameters**

`struct Scsi_Host *shost`
:   host to add

void scsi\_proc\_host\_rm(struct Scsi\_Host \*shost)
:   remove this host’s entry from /proc

**Parameters**

`struct Scsi_Host *shost`
:   which host

int proc\_print\_scsidevice(struct [device](infrastructure.html#c.device "device") \*dev, void \*data)
:   return data about this host

**Parameters**

`struct device *dev`
:   A scsi device

`void *data`
:   `struct seq_file` to output to.

**Description**

prints Host, Channel, Id, Lun, Vendor, Model, Rev, Type,
and revision.

int scsi\_add\_single\_device(uint host, uint channel, uint id, uint lun)
:   Respond to user request to probe for/add device

**Parameters**

`uint host`
:   user-supplied decimal integer

`uint channel`
:   user-supplied decimal integer

`uint id`
:   user-supplied decimal integer

`uint lun`
:   user-supplied decimal integer

**Description**

called by writing “scsi add-single-device” to /proc/scsi/scsi.

does [`scsi_host_lookup()`](#c.scsi_host_lookup "scsi_host_lookup") and either `user_scan()` if that transport
type supports it, or else `scsi_scan_host_selected()`

**Note**

this seems to be aimed exclusively at SCSI parallel busses.

int scsi\_remove\_single\_device(uint host, uint channel, uint id, uint lun)
:   Respond to user request to remove a device

**Parameters**

`uint host`
:   user-supplied decimal integer

`uint channel`
:   user-supplied decimal integer

`uint id`
:   user-supplied decimal integer

`uint lun`
:   user-supplied decimal integer

**Description**

called by writing “scsi remove-single-device” to
/proc/scsi/scsi. Does a [`scsi_device_lookup()`](#c.scsi_device_lookup "scsi_device_lookup") and [`scsi_remove_device()`](#c.scsi_remove_device "scsi_remove_device")

ssize\_t proc\_scsi\_write(struct [file](#c.proc_scsi_write "file") \*file, const char \_\_user \*buf, size\_t length, loff\_t \*ppos)
:   handle writes to /proc/scsi/scsi

**Parameters**

`struct file *file`
:   not used

`const char __user *buf`
:   buffer to write

`size_t length`
:   length of buf, at most PAGE\_SIZE

`loff_t *ppos`
:   not used

**Description**

this provides a legacy mechanism to add or remove devices by
Host, Channel, ID, and Lun. To use,
“echo ‘scsi add-single-device 0 1 2 3’ > /proc/scsi/scsi” or
“echo ‘scsi remove-single-device 0 1 2 3’ > /proc/scsi/scsi” with
“0 1 2 3” replaced by the Host, Channel, Id, and Lun.

**Note**

this seems to be aimed at parallel SCSI. Most modern busses (USB,
SATA, Firewire, Fibre Channel, etc) dynamically assign these values to
provide a unique identifier and nothing more.

int proc\_scsi\_open(struct [inode](#c.proc_scsi_open "inode") \*inode, struct [file](#c.proc_scsi_open "file") \*file)
:   glue function

**Parameters**

`struct inode *inode`
:   not used

`struct file *file`
:   passed to `single_open()`

**Description**

Associates proc\_scsi\_show with this file

int scsi\_init\_procfs(void)
:   create scsi and scsi/scsi in procfs

**Parameters**

`void`
:   no arguments

void scsi\_exit\_procfs(void)
:   Remove scsi/scsi and scsi from procfs

**Parameters**

`void`
:   no arguments

#### drivers/scsi/scsi\_netlink.c

Infrastructure to provide async events from transports to userspace via
netlink, using a single NETLINK\_SCSITRANSPORT protocol for all
transports. See [the original patch submission](https://lore.kernel.org/linux-scsi/1155070439.6275.5.camel@localhost.localdomain/)
for more details.

void scsi\_nl\_rcv\_msg(struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb)
:   Receive message handler.

**Parameters**

`struct sk_buff *skb`
:   socket receive buffer

**Description**

Extracts message from a receive buffer.
:   Validates message header and calls appropriate transport message handler

void scsi\_netlink\_init(void)
:   Called by SCSI subsystem to initialize the SCSI transport netlink interface

**Parameters**

`void`
:   no arguments

void scsi\_netlink\_exit(void)
:   Called by SCSI subsystem to disable the SCSI transport netlink interface

**Parameters**

`void`
:   no arguments

#### drivers/scsi/scsi\_scan.c

Scan a host to determine which (if any) devices are attached. The
general scanning/probing algorithm is as follows, exceptions are made to
it depending on device specific flags, compilation options, and global
variable (boot or module load time) settings. A specific LUN is scanned
via an INQUIRY command; if the LUN has a device attached, a scsi\_device
is allocated and setup for it. For every id of every channel on the
given host, start by scanning LUN 0. Skip hosts that don’t respond at
all to a scan of LUN 0. Otherwise, if LUN 0 has a device attached,
allocate and setup a scsi\_device for it. If target is SCSI-3 or up,
issue a REPORT LUN, and scan all of the LUNs returned by the REPORT LUN;
else, sequentially scan LUNs up until some maximum is reached, or a LUN
is seen that cannot have a device attached to it.

void scsi\_sanitize\_inquiry\_string(unsigned char \*s, int len)
:   remove non-graphical chars from an INQUIRY result string

**Parameters**

`unsigned char *s`
:   INQUIRY result string to sanitize

`int len`
:   length of the string

**Description**

> The SCSI spec says that INQUIRY vendor, product, and revision
> strings must consist entirely of graphic ASCII characters,
> padded on the right with spaces. Since not all devices obey
> this rule, we will replace non-graphic or non-ASCII characters
> with spaces. Exception: a NUL character is interpreted as a
> string terminator, so all the following characters are set to
> spaces.

int scsi\_add\_device(struct Scsi\_Host \*host, uint channel, uint target, u64 lun)
:   creates a new SCSI (LU) instance

**Parameters**

`struct Scsi_Host *host`
:   the `Scsi_Host` instance where the device is located

`uint channel`
:   target channel number (rarely other than `0`)

`uint target`
:   target id number

`u64 lun`
:   LUN of target device

**Description**

Probe for a specific LUN and add it if found.

**Notes**

This call is usually performed internally during a SCSI
bus scan when an HBA is added (i.e. [`scsi_scan_host()`](#c.scsi_scan_host "scsi_scan_host")). So it
should only be called if the HBA becomes aware of a new SCSI
device (LU) after [`scsi_scan_host()`](#c.scsi_scan_host "scsi_scan_host") has completed. If successful
this call can lead to `sdev_init()` and `sdev_configure()` callbacks
into the LLD.

**Return**

`0` on success or negative error code on failure

void scsi\_scan\_target(struct [device](infrastructure.html#c.device "device") \*parent, unsigned int channel, unsigned int id, u64 lun, enum scsi\_scan\_mode rescan)
:   scan a target id, possibly including all LUNs on the target.

**Parameters**

`struct device *parent`
:   host to scan

`unsigned int channel`
:   channel to scan

`unsigned int id`
:   target id to scan

`u64 lun`
:   Specific LUN to scan or SCAN\_WILD\_CARD

`enum scsi_scan_mode rescan`
:   passed to LUN scanning routines; SCSI\_SCAN\_INITIAL for
    no rescan, SCSI\_SCAN\_RESCAN to rescan existing LUNs,
    and SCSI\_SCAN\_MANUAL to force scanning even if
    ‘scan=manual’ is set.

**Description**

> Scan the target id on **parent**, **channel**, and **id**. Scan at least LUN 0,
> and possibly all LUNs on the target id.
>
> First try a REPORT LUN scan, if that does not scan the target, do a
> sequential scan of LUNs on the target id.

void scsi\_scan\_host(struct Scsi\_Host \*shost)
:   scan the given adapter

**Parameters**

`struct Scsi_Host *shost`
:   adapter to scan

**Notes**

Should be called after `scsi_add_host()`

#### drivers/scsi/scsi\_sysctl.c

Set up the sysctl entry: “/dev/scsi/logging\_level”
(DEV\_SCSI\_LOGGING\_LEVEL) which sets/returns scsi\_logging\_level.

#### drivers/scsi/scsi\_sysfs.c

SCSI sysfs interface routines.

void scsi\_remove\_device(struct scsi\_device \*sdev)
:   unregister a device from the scsi bus

**Parameters**

`struct scsi_device *sdev`
:   scsi\_device to unregister

void scsi\_remove\_target(struct [device](infrastructure.html#c.device "device") \*dev)
:   try to remove a target and all its devices

**Parameters**

`struct device *dev`
:   generic starget or parent of generic stargets to be removed

**Note**

This is slightly racy. It is possible that if the user
requests the addition of another device then the target won’t be
removed.

#### drivers/scsi/hosts.c

mid to lowlevel SCSI driver interface

void scsi\_remove\_host(struct Scsi\_Host \*shost)
:   remove a scsi host

**Parameters**

`struct Scsi_Host *shost`
:   a pointer to a scsi host to remove

int scsi\_add\_host\_with\_dma(struct Scsi\_Host \*shost, struct [device](infrastructure.html#c.device "device") \*dev, struct [device](infrastructure.html#c.device "device") \*dma\_dev)
:   add a scsi host with dma device

**Parameters**

`struct Scsi_Host *shost`
:   scsi host pointer to add

`struct device *dev`
:   a [`struct device`](infrastructure.html#c.device "device") of type scsi class

`struct device *dma_dev`
:   dma device for the host

**Note**

You rarely need to worry about this unless you’re in a
virtualised host environments, so use the simpler `scsi_add_host()`
function instead.

Return value:
:   0 on success / != 0 for error

struct Scsi\_Host \*scsi\_host\_alloc(const struct scsi\_host\_template \*sht, int privsize)
:   register a scsi host adapter instance.

**Parameters**

`const struct scsi_host_template *sht`
:   pointer to scsi host template

`int privsize`
:   extra bytes to allocate for driver

**Note**

> Allocate a new Scsi\_Host and perform basic initialization.
> The host is not published to the scsi midlayer until scsi\_add\_host
> is called.

Return value:
:   Pointer to a new Scsi\_Host

struct Scsi\_Host \*scsi\_host\_lookup(unsigned int hostnum)
:   get a reference to a Scsi\_Host by host no

**Parameters**

`unsigned int hostnum`
:   host number to locate

**Description**

Return value:
:   A pointer to located Scsi\_Host or NULL.

    The caller must do a [`scsi_host_put()`](#c.scsi_host_put "scsi_host_put") to drop the reference
    that [`scsi_host_get()`](#c.scsi_host_get "scsi_host_get") took. The [`put_device()`](infrastructure.html#c.put_device "put_device") below dropped
    the reference from [`class_find_device()`](infrastructure.html#c.class_find_device "class_find_device").

struct Scsi\_Host \*scsi\_host\_get(struct Scsi\_Host \*shost)
:   inc a Scsi\_Host ref count

**Parameters**

`struct Scsi_Host *shost`
:   Pointer to Scsi\_Host to inc.

int scsi\_host\_busy(struct Scsi\_Host \*shost)
:   Return the count of in-flight commands

**Parameters**

`struct Scsi_Host *shost`
:   Pointer to Scsi\_Host

void scsi\_host\_put(struct Scsi\_Host \*shost)
:   dec a Scsi\_Host ref count

**Parameters**

`struct Scsi_Host *shost`
:   Pointer to Scsi\_Host to dec.

int scsi\_queue\_work(struct Scsi\_Host \*shost, struct work\_struct \*work)
:   Queue work to the Scsi\_Host workqueue.

**Parameters**

`struct Scsi_Host *shost`
:   Pointer to Scsi\_Host.

`struct work_struct *work`
:   Work to queue for execution.

**Description**

Return value:
:   1 - work queued for execution
    0 - work is already queued
    -EINVAL - work queue doesn’t exist

void scsi\_flush\_work(struct Scsi\_Host \*shost)
:   Flush a Scsi\_Host’s workqueue.

**Parameters**

`struct Scsi_Host *shost`
:   Pointer to Scsi\_Host.

void scsi\_host\_complete\_all\_commands(struct Scsi\_Host \*shost, enum scsi\_host\_status status)
:   Terminate all running commands

**Parameters**

`struct Scsi_Host *shost`
:   Scsi Host on which commands should be terminated

`enum scsi_host_status status`
:   Status to be set for the terminated commands

**Description**

There is no protection against modification of the number
of outstanding commands. It is the responsibility of the
caller to ensure that concurrent I/O submission and/or
completion is stopped when calling this function.

void scsi\_host\_busy\_iter(struct Scsi\_Host \*shost, bool (\*fn)(struct scsi\_cmnd\*, void\*), void \*priv)
:   Iterate over all busy commands

**Parameters**

`struct Scsi_Host *shost`
:   Pointer to Scsi\_Host.

`bool (*fn)(struct scsi_cmnd *, void *)`
:   Function to call on each busy command

`void *priv`
:   Data pointer passed to **fn**

**Description**

If locking against concurrent command completions is required
ithas to be provided by the caller

#### drivers/scsi/scsi\_common.c

general support functions

const char \*scsi\_device\_type(unsigned type)
:   Return 17-char string indicating device type.

**Parameters**

`unsigned type`
:   type number to look up

u64 scsilun\_to\_int(struct scsi\_lun \*scsilun)
:   convert a scsi\_lun to an int

**Parameters**

`struct scsi_lun *scsilun`
:   `struct scsi_lun` to be converted.

**Description**

> Convert **scsilun** from a `struct scsi_lun` to a four-byte host byte-ordered
> integer, and return the result. The caller must check for
> truncation before using this function.

**Notes**

> For a description of the LUN format, post SCSI-3 see the SCSI
> Architecture Model, for SCSI-3 see the SCSI Controller Commands.
>
> Given a `struct scsi_lun` of: d2 04 0b 03 00 00 00 00, this function
> returns the integer: 0x0b03d204
>
> This encoding will return a standard integer LUN for LUNs smaller
> than 256, which typically use a single level LUN structure with
> addressing method 0.

void int\_to\_scsilun(u64 lun, struct scsi\_lun \*scsilun)
:   reverts an int into a scsi\_lun

**Parameters**

`u64 lun`
:   integer to be reverted

`struct scsi_lun *scsilun`
:   `struct scsi_lun` to be set.

**Description**

> Reverts the functionality of the scsilun\_to\_int, which packed
> an 8-byte lun value into an int. This routine unpacks the int
> back into the lun value.

**Notes**

> Given an integer : 0x0b03d204, this function returns a
> `struct scsi_lun` of: d2 04 0b 03 00 00 00 00

bool scsi\_normalize\_sense(const u8 \*sense\_buffer, int sb\_len, struct scsi\_sense\_hdr \*sshdr)
:   normalize main elements from either fixed or descriptor sense data format into a common format.

**Parameters**

`const u8 *sense_buffer`
:   byte array containing sense data returned by device

`int sb_len`
:   number of valid bytes in sense\_buffer

`struct scsi_sense_hdr *sshdr`
:   pointer to instance of structure that common
    elements are written to.

**Notes**

> The “main elements” from sense data are: response\_code, sense\_key,
> asc, ascq and additional\_length (only for descriptor format).
>
> Typically this function can be called after a device has
> responded to a SCSI command with the CHECK\_CONDITION status.

Return value:
:   true if valid sense data information found, else false;

const u8 \*scsi\_sense\_desc\_find(const u8 \*sense\_buffer, int sb\_len, int desc\_type)
:   search for a given descriptor type in descriptor sense data format.

**Parameters**

`const u8 * sense_buffer`
:   byte array of descriptor format sense data

`int sb_len`
:   number of valid bytes in sense\_buffer

`int desc_type`
:   value of descriptor type to find
    (e.g. 0 -> information)

**Notes**

> only valid when sense data is in descriptor format

Return value:
:   pointer to start of (first) descriptor if found else NULL

void scsi\_build\_sense\_buffer(int desc, u8 \*buf, u8 key, u8 asc, u8 ascq)
:   build sense data in a buffer

**Parameters**

`int desc`
:   Sense format (non-zero == descriptor format,
    0 == fixed format)

`u8 *buf`
:   Where to build sense data

`u8 key`
:   Sense key

`u8 asc`
:   Additional sense code

`u8 ascq`
:   Additional sense code qualifier

int scsi\_set\_sense\_information(u8 \*buf, int buf\_len, u64 info)
:   set the information field in a formatted sense data buffer

**Parameters**

`u8 *buf`
:   Where to build sense data

`int buf_len`
:   buffer length

`u64 info`
:   64-bit information value to be set

**Description**

Return value:
:   0 on success or -EINVAL for invalid sense buffer length

int scsi\_set\_sense\_field\_pointer(u8 \*buf, int buf\_len, u16 fp, u8 bp, bool cd)
:   set the field pointer sense key specific information in a formatted sense data buffer

**Parameters**

`u8 *buf`
:   Where to build sense data

`int buf_len`
:   buffer length

`u16 fp`
:   field pointer to be set

`u8 bp`
:   bit pointer to be set

`bool cd`
:   command/data bit

**Description**

Return value:
:   0 on success or -EINVAL for invalid sense buffer length

### Transport classes

Transport classes are service libraries for drivers in the SCSI lower
layer, which expose transport attributes in sysfs.

#### Fibre Channel transport

The file drivers/scsi/scsi\_transport\_fc.c defines transport attributes
for Fibre Channel.

u32 fc\_get\_event\_number(void)
:   Obtain the next sequential FC event number

**Parameters**

`void`
:   no arguments

**Notes**

> We could have inlined this, but it would have required fc\_event\_seq to
> be exposed. For now, live with the subroutine call.
> Atomic used to avoid lock/unlock...

void fc\_host\_post\_fc\_event(struct Scsi\_Host \*shost, u32 event\_number, enum fc\_host\_event\_code event\_code, u32 data\_len, char \*data\_buf, u64 vendor\_id)
:   routine to do the work of posting an event on an fc\_host.

**Parameters**

`struct Scsi_Host *shost`
:   host the event occurred on

`u32 event_number`
:   fc event number obtained from `get_fc_event_number()`

`enum fc_host_event_code event_code`
:   fc\_host event being posted

`u32 data_len`
:   amount, in bytes, of event data

`char *data_buf`
:   pointer to event data

`u64 vendor_id`
:   value for Vendor id

**Notes**

> This routine assumes no locks are held on entry.

void fc\_host\_post\_event(struct Scsi\_Host \*shost, u32 event\_number, enum fc\_host\_event\_code event\_code, u32 event\_data)
:   called to post an even on an fc\_host.

**Parameters**

`struct Scsi_Host *shost`
:   host the event occurred on

`u32 event_number`
:   fc event number obtained from `get_fc_event_number()`

`enum fc_host_event_code event_code`
:   fc\_host event being posted

`u32 event_data`
:   32bits of data for the event being posted

**Notes**

> This routine assumes no locks are held on entry.

void fc\_host\_post\_vendor\_event(struct Scsi\_Host \*shost, u32 event\_number, u32 data\_len, char \*data\_buf, u64 vendor\_id)
:   called to post a vendor unique event on an fc\_host

**Parameters**

`struct Scsi_Host *shost`
:   host the event occurred on

`u32 event_number`
:   fc event number obtained from `get_fc_event_number()`

`u32 data_len`
:   amount, in bytes, of vendor unique data

`char * data_buf`
:   pointer to vendor unique data

`u64 vendor_id`
:   Vendor id

**Notes**

> This routine assumes no locks are held on entry.

struct fc\_rport \*fc\_find\_rport\_by\_wwpn(struct Scsi\_Host \*shost, u64 wwpn)
:   find the fc\_rport pointer for a given wwpn

**Parameters**

`struct Scsi_Host *shost`
:   host the fc\_rport is associated with

`u64 wwpn`
:   wwpn of the fc\_rport device

**Notes**

> This routine assumes no locks are held on entry.

void fc\_host\_fpin\_rcv(struct Scsi\_Host \*shost, u32 fpin\_len, char \*fpin\_buf, u8 event\_acknowledge)
:   routine to process a received FPIN.

**Parameters**

`struct Scsi_Host *shost`
:   host the FPIN was received on

`u32 fpin_len`
:   length of FPIN payload, in bytes

`char *fpin_buf`
:   pointer to FPIN payload

`u8 event_acknowledge`
:   1, if LLDD handles this event.

**Notes**

> This routine assumes no locks are held on entry.

enum scsi\_timeout\_action fc\_eh\_timed\_out(struct scsi\_cmnd \*scmd)
:   FC Transport I/O timeout intercept handler

**Parameters**

`struct scsi_cmnd *scmd`
:   The SCSI command which timed out

**Description**

This routine protects against error handlers getting invoked while a
rport is in a blocked state, typically due to a temporarily loss of
connectivity. If the error handlers are allowed to proceed, requests
to abort i/o, reset the target, etc will likely fail as there is no way
to communicate with the device to perform the requested function. These
failures may result in the midlayer taking the device offline, requiring
manual intervention to restore operation.

This routine, called whenever an i/o times out, validates the state of
the underlying rport. If the rport is blocked, it returns
EH\_RESET\_TIMER, which will continue to reschedule the timeout.
Eventually, either the device will return, or devloss\_tmo will fire,
and when the timeout then fires, it will be handled normally.
If the rport is not blocked, normal error handling continues.

**Notes**

> This routine assumes no locks are held on entry.

void fc\_remove\_host(struct Scsi\_Host \*shost)
:   called to terminate any fc\_transport-related elements for a scsi host.

**Parameters**

`struct Scsi_Host *shost`
:   Which `Scsi_Host`

**Description**

This routine is expected to be called immediately preceding the
a driver’s call to [`scsi_remove_host()`](#c.scsi_remove_host "scsi_remove_host").

WARNING: A driver utilizing the fc\_transport, which fails to call
:   this routine prior to [`scsi_remove_host()`](#c.scsi_remove_host "scsi_remove_host"), will leave dangling
    objects in /sys/class/fc\_remote\_ports. Access to any of these
    objects can result in a system crash !!!

**Notes**

> This routine assumes no locks are held on entry.

struct fc\_rport \*fc\_remote\_port\_add(struct Scsi\_Host \*shost, int channel, struct fc\_rport\_identifiers \*ids)
:   notify fc transport of the existence of a remote FC port.

**Parameters**

`struct Scsi_Host *shost`
:   scsi host the remote port is connected to.

`int channel`
:   Channel on shost port connected to.

`struct fc_rport_identifiers *ids`
:   The world wide names, fc address, and FC4 port
    roles for the remote port.

**Description**

The LLDD calls this routine to notify the transport of the existence
of a remote port. The LLDD provides the unique identifiers (wwpn,wwn)
of the port, it’s FC address (port\_id), and the FC4 roles that are
active for the port.

For ports that are FCP targets (aka scsi targets), the FC transport
maintains consistent target id bindings on behalf of the LLDD.
A consistent target id binding is an assignment of a target id to
a remote port identifier, which persists while the scsi host is
attached. The remote port can disappear, then later reappear, and
it’s target id assignment remains the same. This allows for shifts
in FC addressing (if binding by wwpn or wwnn) with no apparent
changes to the scsi subsystem which is based on scsi host number and
target id values. Bindings are only valid during the attachment of
the scsi host. If the host detaches, then later re-attaches, target
id bindings may change.

This routine is responsible for returning a remote port structure.
The routine will search the list of remote ports it maintains
internally on behalf of consistent target id mappings. If found, the
remote port structure will be reused. Otherwise, a new remote port
structure will be allocated.

Whenever a remote port is allocated, a new fc\_remote\_port class
device is created.

Should not be called from interrupt context.

**Notes**

> This routine assumes no locks are held on entry.

void fc\_remote\_port\_delete(struct fc\_rport \*rport)
:   notifies the fc transport that a remote port is no longer in existence.

**Parameters**

`struct fc_rport *rport`
:   The remote port that no longer exists

**Description**

The LLDD calls this routine to notify the transport that a remote
port is no longer part of the topology. Note: Although a port
may no longer be part of the topology, it may persist in the remote
ports displayed by the fc\_host. We do this under 2 conditions:

1. If the port was a scsi target, we delay its deletion by “blocking” it.
   This allows the port to temporarily disappear, then reappear without
   disrupting the SCSI device tree attached to it. During the “blocked”
   period the port will still exist.
2. If the port was a scsi target and disappears for longer than we
   expect, we’ll delete the port and the tear down the SCSI device tree
   attached to it. However, we want to semi-persist the target id assigned
   to that port if it eventually does exist. The port structure will
   remain (although with minimal information) so that the target id
   bindings also remain.

If the remote port is not an FCP Target, it will be fully torn down
and deallocated, including the fc\_remote\_port class device.

If the remote port is an FCP Target, the port will be placed in a
temporary blocked state. From the LLDD’s perspective, the rport no
longer exists. From the SCSI midlayer’s perspective, the SCSI target
exists, but all sdevs on it are blocked from further I/O. The following
is then expected.

> If the remote port does not return (signaled by a LLDD call to
> [`fc_remote_port_add()`](#c.fc_remote_port_add "fc_remote_port_add")) within the dev\_loss\_tmo timeout, then the
> scsi target is removed - killing all outstanding i/o and removing the
> scsi devices attached to it. The port structure will be marked Not
> Present and be partially cleared, leaving only enough information to
> recognize the remote port relative to the scsi target id binding if
> it later appears. The port will remain as long as there is a valid
> binding (e.g. until the user changes the binding type or unloads the
> scsi host with the binding).
>
> If the remote port returns within the dev\_loss\_tmo value (and matches
> according to the target id binding type), the port structure will be
> reused. If it is no longer a SCSI target, the target will be torn
> down. If it continues to be a SCSI target, then the target will be
> unblocked (allowing i/o to be resumed), and a scan will be activated
> to ensure that all luns are detected.

Called from normal process context only - cannot be called from interrupt.

**Notes**

> This routine assumes no locks are held on entry.

void fc\_remote\_port\_rolechg(struct fc\_rport \*rport, u32 roles)
:   notifies the fc transport that the roles on a remote may have changed.

**Parameters**

`struct fc_rport *rport`
:   The remote port that changed.

`u32 roles`
:   New roles for this port.

**Description**

The LLDD calls this routine to notify the transport that the
roles on a remote port may have changed. The largest effect of this is
if a port now becomes a FCP Target, it must be allocated a
scsi target id. If the port is no longer a FCP target, any
scsi target id value assigned to it will persist in case the
role changes back to include FCP Target. No changes in the scsi
midlayer will be invoked if the role changes (in the expectation
that the role will be resumed. If it doesn’t normal error processing
will take place).

Should not be called from interrupt context.

**Notes**

> This routine assumes no locks are held on entry.

int fc\_block\_rport(struct fc\_rport \*rport)
:   Block SCSI eh thread for blocked fc\_rport.

**Parameters**

`struct fc_rport *rport`
:   Remote port that scsi\_eh is trying to recover.

**Description**

This routine can be called from a FC LLD scsi\_eh callback. It
blocks the scsi\_eh thread until the fc\_rport leaves the
FC\_PORTSTATE\_BLOCKED, or the fast\_io\_fail\_tmo fires. This is
necessary to avoid the scsi\_eh failing recovery actions for blocked
rports which would lead to offlined SCSI devices.

**Return**

0 if the fc\_rport left the state FC\_PORTSTATE\_BLOCKED.
FAST\_IO\_FAIL if the fast\_io\_fail\_tmo fired, this should be
passed back to scsi\_eh.

int fc\_block\_scsi\_eh(struct scsi\_cmnd \*cmnd)
:   Block SCSI eh thread for blocked fc\_rport

**Parameters**

`struct scsi_cmnd *cmnd`
:   SCSI command that scsi\_eh is trying to recover

**Description**

This routine can be called from a FC LLD scsi\_eh callback. It
blocks the scsi\_eh thread until the fc\_rport leaves the
FC\_PORTSTATE\_BLOCKED, or the fast\_io\_fail\_tmo fires. This is
necessary to avoid the scsi\_eh failing recovery actions for blocked
rports which would lead to offlined SCSI devices.

**Return**

0 if the fc\_rport left the state FC\_PORTSTATE\_BLOCKED.
FAST\_IO\_FAIL if the fast\_io\_fail\_tmo fired, this should be
passed back to scsi\_eh.

struct fc\_vport \*fc\_vport\_create(struct Scsi\_Host \*shost, int channel, struct fc\_vport\_identifiers \*ids)
:   Admin App or LLDD requests creation of a vport

**Parameters**

`struct Scsi_Host *shost`
:   scsi host the virtual port is connected to.

`int channel`
:   channel on shost port connected to.

`struct fc_vport_identifiers *ids`
:   The world wide names, FC4 port roles, etc for
    the virtual port.

**Notes**

> This routine assumes no locks are held on entry.

int fc\_vport\_terminate(struct fc\_vport \*vport)
:   Admin App or LLDD requests termination of a vport

**Parameters**

`struct fc_vport *vport`
:   fc\_vport to be terminated

**Description**

Calls the LLDD `vport_delete()` function, then deallocates and removes
the vport from the shost and object tree.

**Notes**

> This routine assumes no locks are held on entry.

#### iSCSI transport class

The file drivers/scsi/scsi\_transport\_iscsi.c defines transport
attributes for the iSCSI class, which sends SCSI packets over TCP/IP
connections.

struct iscsi\_endpoint \*iscsi\_lookup\_endpoint(u64 handle)
:   get ep from handle

**Parameters**

`u64 handle`
:   endpoint handle

**Description**

Caller must do a iscsi\_put\_endpoint.

struct iscsi\_bus\_flash\_session \*iscsi\_create\_flashnode\_sess(struct Scsi\_Host \*shost, int index, struct iscsi\_transport \*transport, int dd\_size)
:   Add flashnode session entry in sysfs

**Parameters**

`struct Scsi_Host *shost`
:   pointer to host data

`int index`
:   index of flashnode to add in sysfs

`struct iscsi_transport *transport`
:   pointer to transport data

`int dd_size`
:   total size to allocate

**Description**

Adds a sysfs entry for the flashnode session attributes

**Return**

pointer to allocated flashnode sess on success
`NULL` on failure

struct iscsi\_bus\_flash\_conn \*iscsi\_create\_flashnode\_conn(struct Scsi\_Host \*shost, struct iscsi\_bus\_flash\_session \*fnode\_sess, struct iscsi\_transport \*transport, int dd\_size)
:   Add flashnode conn entry in sysfs

**Parameters**

`struct Scsi_Host *shost`
:   pointer to host data

`struct iscsi_bus_flash_session *fnode_sess`
:   pointer to the parent flashnode session entry

`struct iscsi_transport *transport`
:   pointer to transport data

`int dd_size`
:   total size to allocate

**Description**

Adds a sysfs entry for the flashnode connection attributes

**Return**

pointer to allocated flashnode conn on success
`NULL` on failure

struct [device](infrastructure.html#c.device "device") \*iscsi\_find\_flashnode\_sess(struct Scsi\_Host \*shost, const void \*data, device\_match\_t fn)
:   finds flashnode session entry

**Parameters**

`struct Scsi_Host *shost`
:   pointer to host data

`const void *data`
:   pointer to data containing value to use for comparison

`device_match_t fn`
:   function pointer that does actual comparison

**Description**

Finds the flashnode session object comparing the data passed using logic
defined in passed function pointer

**Return**

pointer to found flashnode session device object on success
`NULL` on failure

struct [device](infrastructure.html#c.device "device") \*iscsi\_find\_flashnode\_conn(struct iscsi\_bus\_flash\_session \*fnode\_sess)
:   finds flashnode connection entry

**Parameters**

`struct iscsi_bus_flash_session *fnode_sess`
:   pointer to parent flashnode session entry

**Description**

Finds the flashnode connection object comparing the data passed using logic
defined in passed function pointer

**Return**

pointer to found flashnode connection device object on success
`NULL` on failure

void iscsi\_destroy\_flashnode\_sess(struct iscsi\_bus\_flash\_session \*fnode\_sess)
:   destroy flashnode session entry

**Parameters**

`struct iscsi_bus_flash_session *fnode_sess`
:   pointer to flashnode session entry to be destroyed

**Description**

Deletes the flashnode session entry and all children flashnode connection
entries from sysfs

void iscsi\_destroy\_all\_flashnode(struct Scsi\_Host \*shost)
:   destroy all flashnode session entries

**Parameters**

`struct Scsi_Host *shost`
:   pointer to host data

**Description**

Destroys all the flashnode session entries and all corresponding children
flashnode connection entries from sysfs

int iscsi\_block\_scsi\_eh(struct scsi\_cmnd \*cmd)
:   block scsi eh until session state has transistioned

**Parameters**

`struct scsi_cmnd *cmd`
:   scsi cmd passed to scsi eh handler

**Description**

If the session is down this function will wait for the recovery
timer to fire or for the session to be logged back in. If the
recovery timer fires then FAST\_IO\_FAIL is returned. The caller
should pass this error value to the scsi eh.

void iscsi\_unblock\_session(struct iscsi\_cls\_session \*session)
:   set a session as logged in and start IO.

**Parameters**

`struct iscsi_cls_session *session`
:   iscsi session

**Description**

Mark a session as ready to accept IO.

void iscsi\_force\_destroy\_session(struct iscsi\_cls\_session \*session)
:   destroy a session from the kernel

**Parameters**

`struct iscsi_cls_session *session`
:   session to destroy

**Description**

Force the destruction of a session from the kernel. This should only be
used when userspace is no longer running during system shutdown.

struct iscsi\_cls\_conn \*iscsi\_alloc\_conn(struct iscsi\_cls\_session \*session, int dd\_size, uint32\_t cid)
:   alloc iscsi class connection

**Parameters**

`struct iscsi_cls_session *session`
:   iscsi cls session

`int dd_size`
:   private driver data size

`uint32_t cid`
:   connection id

int iscsi\_add\_conn(struct iscsi\_cls\_conn \*conn)
:   add iscsi class connection

**Parameters**

`struct iscsi_cls_conn *conn`
:   iscsi cls connection

**Description**

This will expose iscsi\_cls\_conn to sysfs so make sure the related
resources for sysfs attributes are initialized before calling this.

void iscsi\_remove\_conn(struct iscsi\_cls\_conn \*conn)
:   remove iscsi class connection from sysfs

**Parameters**

`struct iscsi_cls_conn *conn`
:   iscsi cls connection

**Description**

Remove iscsi\_cls\_conn from sysfs, and wait for previous
read/write of iscsi\_cls\_conn’s attributes in sysfs to finish.

int iscsi\_session\_event(struct iscsi\_cls\_session \*session, enum iscsi\_uevent\_e event)
:   send session destr. completion event

**Parameters**

`struct iscsi_cls_session *session`
:   iscsi class session

`enum iscsi_uevent_e event`
:   type of event

#### Serial Attached SCSI (SAS) transport class

The file drivers/scsi/scsi\_transport\_sas.c defines transport
attributes for Serial Attached SCSI, a variant of SATA aimed at large
high-end systems.

The SAS transport class contains common code to deal with SAS HBAs, an
approximated representation of SAS topologies in the driver model, and
various sysfs attributes to expose these topologies and management
interfaces to userspace.

In addition to the basic SCSI core objects this transport class
introduces two additional intermediate objects: The SAS PHY as
represented by `struct sas_phy` defines an “outgoing” PHY on a SAS HBA or
Expander, and the SAS remote PHY represented by `struct sas_rphy` defines
an “incoming” PHY on a SAS Expander or end device. Note that this is
purely a software concept, the underlying hardware for a PHY and a
remote PHY is the exactly the same.

There is no concept of a SAS port in this code, users can see what PHYs
form a wide port based on the port\_identifier attribute, which is the
same for all PHYs in a port.

void sas\_remove\_children(struct [device](infrastructure.html#c.device "device") \*dev)
:   tear down a devices SAS data structures

**Parameters**

`struct device *dev`
:   device belonging to the sas object

**Description**

Removes all SAS PHYs and remote PHYs for a given object

void sas\_remove\_host(struct Scsi\_Host \*shost)
:   tear down a Scsi\_Host’s SAS data structures

**Parameters**

`struct Scsi_Host *shost`
:   Scsi Host that is torn down

**Description**

Removes all SAS PHYs and remote PHYs for a given Scsi\_Host and remove the
Scsi\_Host as well.

**Note**

Do not call [`scsi_remove_host()`](#c.scsi_remove_host "scsi_remove_host") on the Scsi\_Host any more, as it is
already removed.

u64 sas\_get\_address(struct scsi\_device \*sdev)
:   return the SAS address of the device

**Parameters**

`struct scsi_device *sdev`
:   scsi device

**Description**

Returns the SAS address of the scsi device

unsigned int sas\_tlr\_supported(struct scsi\_device \*sdev)
:   checking TLR bit in vpd 0x90

**Parameters**

`struct scsi_device *sdev`
:   scsi device struct

**Description**

Check Transport Layer Retries are supported or not.
If vpd page 0x90 is present, TRL is supported.

void sas\_disable\_tlr(struct scsi\_device \*sdev)
:   setting TLR flags

**Parameters**

`struct scsi_device *sdev`
:   scsi device struct

**Description**

Seting tlr\_enabled flag to 0.

void sas\_enable\_tlr(struct scsi\_device \*sdev)
:   setting TLR flags

**Parameters**

`struct scsi_device *sdev`
:   scsi device struct

**Description**

Seting tlr\_enabled flag 1.

bool sas\_ata\_ncq\_prio\_supported(struct scsi\_device \*sdev)
:   Check for ATA NCQ command priority support

**Parameters**

`struct scsi_device *sdev`
:   SCSI device

**Description**

Check if an ATA device supports NCQ priority using VPD page 89h (ATA
Information). Since this VPD page is implemented only for ATA devices,
this function always returns false for SCSI devices.

struct sas\_phy \*sas\_phy\_alloc(struct [device](infrastructure.html#c.device "device") \*parent, int number)
:   allocates and initialize a SAS PHY structure

**Parameters**

`struct device *parent`
:   Parent device

`int number`
:   Phy index

**Description**

Allocates an SAS PHY structure. It will be added in the device tree
below the device specified by **parent**, which has to be either a Scsi\_Host
or sas\_rphy.

**Return**

SAS PHY allocated or `NULL` if the allocation failed.

int sas\_phy\_add(struct sas\_phy \*phy)
:   add a SAS PHY to the device hierarchy

**Parameters**

`struct sas_phy *phy`
:   The PHY to be added

**Description**

Publishes a SAS PHY to the rest of the system.

void sas\_phy\_free(struct sas\_phy \*phy)
:   free a SAS PHY

**Parameters**

`struct sas_phy *phy`
:   SAS PHY to free

**Description**

Frees the specified SAS PHY.

**Note**

> This function must only be called on a PHY that has not
> successfully been added using [`sas_phy_add()`](#c.sas_phy_add "sas_phy_add").

void sas\_phy\_delete(struct sas\_phy \*phy)
:   remove SAS PHY

**Parameters**

`struct sas_phy *phy`
:   SAS PHY to remove

**Description**

Removes the specified SAS PHY. If the SAS PHY has an
associated remote PHY it is removed before.

int scsi\_is\_sas\_phy(const struct [device](infrastructure.html#c.device "device") \*dev)
:   check if a [`struct device`](infrastructure.html#c.device "device") represents a SAS PHY

**Parameters**

`const struct device *dev`
:   device to check

**Return**

`1` if the device represents a SAS PHY, `0` else

struct sas\_port \*sas\_port\_alloc(struct [device](infrastructure.html#c.device "device") \*parent, int port\_id)
:   allocate and initialize a SAS port structure

**Parameters**

`struct device *parent`
:   parent device

`int port_id`
:   port number

**Description**

Allocates a SAS port structure. It will be added to the device tree
below the device specified by **parent** which must be either a Scsi\_Host
or a sas\_expander\_device.

**Return**

`NULL` on error

struct sas\_port \*sas\_port\_alloc\_num(struct [device](infrastructure.html#c.device "device") \*parent)
:   allocate and initialize a SAS port structure

**Parameters**

`struct device *parent`
:   parent device

**Description**

Allocates a SAS port structure and a number to go with it. This
interface is really for adapters where the port number has no
meansing, so the sas class should manage them. It will be added to
the device tree below the device specified by **parent** which must be
either a Scsi\_Host or a sas\_expander\_device.

**Return**

`NULL` on error

int sas\_port\_add(struct sas\_port \*port)
:   add a SAS port to the device hierarchy

**Parameters**

`struct sas_port *port`
:   port to be added

**Description**

publishes a port to the rest of the system

void sas\_port\_free(struct sas\_port \*port)
:   free a SAS PORT

**Parameters**

`struct sas_port *port`
:   SAS PORT to free

**Description**

Frees the specified SAS PORT.

**Note**

> This function must only be called on a PORT that has not
> successfully been added using [`sas_port_add()`](#c.sas_port_add "sas_port_add").

void sas\_port\_delete(struct sas\_port \*port)
:   remove SAS PORT

**Parameters**

`struct sas_port *port`
:   SAS PORT to remove

**Description**

Removes the specified SAS PORT. If the SAS PORT has an
associated phys, unlink them from the port as well.

int scsi\_is\_sas\_port(const struct [device](infrastructure.html#c.device "device") \*dev)
:   check if a [`struct device`](infrastructure.html#c.device "device") represents a SAS port

**Parameters**

`const struct device *dev`
:   device to check

**Return**

`1` if the device represents a SAS Port, `0` else

struct sas\_phy \*sas\_port\_get\_phy(struct sas\_port \*port)
:   try to take a reference on a port member

**Parameters**

`struct sas_port *port`
:   port to check

void sas\_port\_add\_phy(struct sas\_port \*port, struct sas\_phy \*phy)
:   add another phy to a port to form a wide port

**Parameters**

`struct sas_port *port`
:   port to add the phy to

`struct sas_phy *phy`
:   phy to add

**Description**

When a port is initially created, it is empty (has no phys). All
ports must have at least one phy to operated, and all wide ports
must have at least two. The current code makes no difference
between ports and wide ports, but the only object that can be
connected to a remote device is a port, so ports must be formed on
all devices with phys if they’re connected to anything.

void sas\_port\_delete\_phy(struct sas\_port \*port, struct sas\_phy \*phy)
:   remove a phy from a port or wide port

**Parameters**

`struct sas_port *port`
:   port to remove the phy from

`struct sas_phy *phy`
:   phy to remove

**Description**

This operation is used for tearing down ports again. It must be
done to every port or wide port before calling sas\_port\_delete.

struct sas\_rphy \*sas\_end\_device\_alloc(struct sas\_port \*parent)
:   allocate an rphy for an end device

**Parameters**

`struct sas_port *parent`
:   which port

**Description**

Allocates an SAS remote PHY structure, connected to **parent**.

**Return**

SAS PHY allocated or `NULL` if the allocation failed.

struct sas\_rphy \*sas\_expander\_alloc(struct sas\_port \*parent, enum sas\_device\_type type)
:   allocate an rphy for an end device

**Parameters**

`struct sas_port *parent`
:   which port

`enum sas_device_type type`
:   SAS\_EDGE\_EXPANDER\_DEVICE or SAS\_FANOUT\_EXPANDER\_DEVICE

**Description**

Allocates an SAS remote PHY structure, connected to **parent**.

**Return**

SAS PHY allocated or `NULL` if the allocation failed.

int sas\_rphy\_add(struct sas\_rphy \*rphy)
:   add a SAS remote PHY to the device hierarchy

**Parameters**

`struct sas_rphy *rphy`
:   The remote PHY to be added

**Description**

Publishes a SAS remote PHY to the rest of the system.

void sas\_rphy\_free(struct sas\_rphy \*rphy)
:   free a SAS remote PHY

**Parameters**

`struct sas_rphy *rphy`
:   SAS remote PHY to free

**Description**

Frees the specified SAS remote PHY.

**Note**

> This function must only be called on a remote
> PHY that has not successfully been added using
> [`sas_rphy_add()`](#c.sas_rphy_add "sas_rphy_add") (or has been [`sas_rphy_remove()`](#c.sas_rphy_remove "sas_rphy_remove")’d)

void sas\_rphy\_delete(struct sas\_rphy \*rphy)
:   remove and free SAS remote PHY

**Parameters**

`struct sas_rphy *rphy`
:   SAS remote PHY to remove and free

**Description**

Removes the specified SAS remote PHY and frees it.

void sas\_rphy\_unlink(struct sas\_rphy \*rphy)
:   unlink SAS remote PHY

**Parameters**

`struct sas_rphy *rphy`
:   SAS remote phy to unlink from its parent port

**Description**

Removes port reference to an rphy

void sas\_rphy\_remove(struct sas\_rphy \*rphy)
:   remove SAS remote PHY

**Parameters**

`struct sas_rphy *rphy`
:   SAS remote phy to remove

**Description**

Removes the specified SAS remote PHY.

int scsi\_is\_sas\_rphy(const struct [device](infrastructure.html#c.device "device") \*dev)
:   check if a [`struct device`](infrastructure.html#c.device "device") represents a SAS remote PHY

**Parameters**

`const struct device *dev`
:   device to check

**Return**

`1` if the device represents a SAS remote PHY, `0` else

struct scsi\_transport\_template \*sas\_attach\_transport(struct sas\_function\_template \*ft)
:   instantiate SAS transport template

**Parameters**

`struct sas_function_template *ft`
:   SAS transport class function template

void sas\_release\_transport(struct scsi\_transport\_template \*t)
:   release SAS transport template instance

**Parameters**

`struct scsi_transport_template *t`
:   transport template instance

#### SATA transport class

The SATA transport is handled by libata, which has its own book of
documentation in this directory.

#### Parallel SCSI (SPI) transport class

The file drivers/scsi/scsi\_transport\_spi.c defines transport
attributes for traditional (fast/wide/ultra) SCSI buses.

void spi\_dv\_device(struct scsi\_device \*sdev)
:   Do Domain Validation on the device

**Parameters**

`struct scsi_device *sdev`
:   scsi device to validate

**Description**

> Performs the domain validation on the given device in the
> current execution thread. Since DV operations may sleep,
> the current thread must have user context. Also no SCSI
> related locks that would deadlock I/O issued by the DV may
> be held.

void spi\_schedule\_dv\_device(struct scsi\_device \*sdev)
:   schedule domain validation to occur on the device

**Parameters**

`struct scsi_device *sdev`
:   The device to validate

**Description**

> Identical to [`spi_dv_device()`](#c.spi_dv_device "spi_dv_device") above, except that the DV will be
> scheduled to occur in a workqueue later. All memory allocations
> are atomic, so may be called from any context including those holding
> SCSI locks.

void spi\_display\_xfer\_agreement(struct scsi\_target \*starget)
:   Print the current target transfer agreement

**Parameters**

`struct scsi_target *starget`
:   The target for which to display the agreement

**Description**

Each SPI port is required to maintain a transfer agreement for each
other port on the bus. This function prints a one-line summary of
the current agreement; more detailed information is available in sysfs.

int spi\_populate\_tag\_msg(unsigned char \*msg, struct scsi\_cmnd \*cmd)
:   place a tag message in a buffer

**Parameters**

`unsigned char *msg`
:   pointer to the area to place the tag

`struct scsi_cmnd *cmd`
:   pointer to the scsi command for the tag

**Notes**

> designed to create the correct type of tag message for the
> particular request. Returns the size of the tag message.
> May return 0 if TCQ is disabled for this device.

#### SCSI RDMA (SRP) transport class

The file drivers/scsi/scsi\_transport\_srp.c defines transport
attributes for SCSI over Remote Direct Memory Access.

int srp\_tmo\_valid(int reconnect\_delay, int fast\_io\_fail\_tmo, long dev\_loss\_tmo)
:   check timeout combination validity

**Parameters**

`int reconnect_delay`
:   Reconnect delay in seconds.

`int fast_io_fail_tmo`
:   Fast I/O fail timeout in seconds.

`long dev_loss_tmo`
:   Device loss timeout in seconds.

**Description**

The combination of the timeout parameters must be such that SCSI commands
are finished in a reasonable time. Hence do not allow the fast I/O fail
timeout to exceed SCSI\_DEVICE\_BLOCK\_MAX\_TIMEOUT nor allow dev\_loss\_tmo to
exceed that limit if failing I/O fast has been disabled. Furthermore, these
parameters must be such that multipath can detect failed paths timely.
Hence do not allow all three parameters to be disabled simultaneously.

void srp\_start\_tl\_fail\_timers(struct srp\_rport \*rport)
:   start the transport layer failure timers

**Parameters**

`struct srp_rport *rport`
:   SRP target port.

**Description**

Start the transport layer fast I/O failure and device loss timers. Do not
modify a timer that was already started.

int srp\_reconnect\_rport(struct srp\_rport \*rport)
:   reconnect to an SRP target port

**Parameters**

`struct srp_rport *rport`
:   SRP target port.

**Description**

Blocks SCSI command queueing before invoking `reconnect()` such that
`queuecommand()` won’t be invoked concurrently with `reconnect()` from outside
the SCSI EH. This is important since a `reconnect()` implementation may
reallocate resources needed by `queuecommand()`.

**Notes**

* This function neither waits until outstanding requests have finished nor
  tries to abort these. It is the responsibility of the `reconnect()`
  function to finish outstanding commands before reconnecting to the target
  port.
* It is the responsibility of the caller to ensure that the resources
  reallocated by the `reconnect()` function won’t be used while this function
  is in progress. One possible strategy is to invoke this function from
  the context of the SCSI EH thread only. Another possible strategy is to
  lock the rport mutex inside each SCSI LLD callback that can be invoked by
  the SCSI EH (the scsi\_host\_template.eh\_\*() functions and also the
  scsi\_host\_template.`queuecommand()` function).

enum scsi\_timeout\_action srp\_timed\_out(struct scsi\_cmnd \*scmd)
:   SRP transport intercept of the SCSI timeout EH

**Parameters**

`struct scsi_cmnd *scmd`
:   SCSI command.

**Description**

If a timeout occurs while an rport is in the blocked state, ask the SCSI
EH to continue waiting (SCSI\_EH\_RESET\_TIMER). Otherwise let the SCSI core
handle the timeout (SCSI\_EH\_NOT\_HANDLED).

**Note**

This function is called from soft-IRQ context and with the request
queue lock held.

void srp\_rport\_get(struct srp\_rport \*rport)
:   increment rport reference count

**Parameters**

`struct srp_rport *rport`
:   SRP target port.

void srp\_rport\_put(struct srp\_rport \*rport)
:   decrement rport reference count

**Parameters**

`struct srp_rport *rport`
:   SRP target port.

struct srp\_rport \*srp\_rport\_add(struct Scsi\_Host \*shost, struct srp\_rport\_identifiers \*ids)
:   add a SRP remote port to the device hierarchy

**Parameters**

`struct Scsi_Host *shost`
:   scsi host the remote port is connected to.

`struct srp_rport_identifiers *ids`
:   The port id for the remote port.

**Description**

Publishes a port to the rest of the system.

void srp\_rport\_del(struct srp\_rport \*rport)
:   remove a SRP remote port

**Parameters**

`struct srp_rport *rport`
:   SRP remote port to remove

**Description**

Removes the specified SRP remote port.

void srp\_remove\_host(struct Scsi\_Host \*shost)
:   tear down a Scsi\_Host’s SRP data structures

**Parameters**

`struct Scsi_Host *shost`
:   Scsi Host that is torn down

**Description**

Removes all SRP remote ports for a given Scsi\_Host.
Must be called just before scsi\_remove\_host for SRP HBAs.

void srp\_stop\_rport\_timers(struct srp\_rport \*rport)
:   stop the transport layer recovery timers

**Parameters**

`struct srp_rport *rport`
:   SRP remote port for which to stop the timers.

**Description**

Must be called after [`srp_remove_host()`](#c.srp_remove_host "srp_remove_host") and [`scsi_remove_host()`](#c.scsi_remove_host "scsi_remove_host"). The caller
must hold a reference on the rport (rport->dev) and on the SCSI host
(rport->dev.parent).

struct scsi\_transport\_template \*srp\_attach\_transport(struct srp\_function\_template \*ft)
:   instantiate SRP transport template

**Parameters**

`struct srp_function_template *ft`
:   SRP transport class function template

void srp\_release\_transport(struct scsi\_transport\_template \*t)
:   release SRP transport template instance

**Parameters**

`struct scsi_transport_template *t`
:   transport template instance

## SCSI lower layer

### Host Bus Adapter transport types

Many modern device controllers use the SCSI command set as a protocol to
communicate with their devices through many different types of physical
connections.

In SCSI language a bus capable of carrying SCSI commands is called a
“transport”, and a controller connecting to such a bus is called a “host
bus adapter” (HBA).

#### Debug transport

The file drivers/scsi/scsi\_debug.c simulates a host adapter with a
variable number of disks (or disk like devices) attached, sharing a
common amount of RAM. Does a lot of checking to make sure that we are
not getting blocks mixed up, and panics the kernel if anything out of
the ordinary is seen.

To be more realistic, the simulated devices have the transport
attributes of SAS disks.

For documentation see <http://sg.danny.cz/sg/scsi_debug.html>

#### todo

Parallel (fast/wide/ultra) SCSI, USB, SATA, SAS, Fibre Channel,
FireWire, ATAPI devices, Infiniband, Parallel ports,
netlink...
