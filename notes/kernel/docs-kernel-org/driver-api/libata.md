# libATA Developer’s Guide

> 출처(원문): https://docs.kernel.org/driver-api/libata.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# libATA Developer’s Guide

Author:
:   Jeff Garzik

## Introduction

libATA is a library used inside the Linux kernel to support ATA host
controllers and devices. libATA provides an ATA driver API, class
transports for ATA and ATAPI devices, and SCSI<->ATA translation for ATA
devices according to the T10 SAT specification.

This Guide documents the libATA driver API, library functions, library
internals, and a couple sample ATA low-level drivers.

## libata Driver API

`struct ata_port_operations`
is defined for every low-level libata
hardware driver, and it controls how the low-level driver interfaces
with the ATA and SCSI layers.

FIS-based drivers will hook into the system with `->qc_prep()` and
`->qc_issue()` high-level hooks. Hardware which behaves in a manner
similar to PCI IDE hardware may utilize several generic helpers,
defining at a bare minimum the bus I/O addresses of the ATA shadow
register blocks.

### `struct ata_port_operations`

#### Post-IDENTIFY device configuration

```
void (*dev_config) (struct ata_port *, struct ata_device *);
```

Called after IDENTIFY [PACKET] DEVICE is issued to each device found.
Typically used to apply device-specific fixups prior to issue of SET
FEATURES - XFER MODE, and prior to operation.

This entry may be specified as NULL in ata\_port\_operations.

#### Set PIO/DMA mode

```
void (*set_piomode) (struct ata_port *, struct ata_device *);
void (*set_dmamode) (struct ata_port *, struct ata_device *);
void (*post_set_mode) (struct ata_port *);
unsigned int (*mode_filter) (struct ata_port *, struct ata_device *, unsigned int);
```

Hooks called prior to the issue of SET FEATURES - XFER MODE command. The
optional `->mode_filter()` hook is called when libata has built a mask of
the possible modes. This is passed to the `->mode_filter()` function
which should return a mask of valid modes after filtering those
unsuitable due to hardware limits. It is not valid to use this interface
to add modes.

`dev->pio_mode` and `dev->dma_mode` are guaranteed to be valid when
`->set_piomode()` and when `->set_dmamode()` is called. The timings for
any other drive sharing the cable will also be valid at this point. That
is the library records the decisions for the modes of each drive on a
channel before it attempts to set any of them.

`->post_set_mode()` is called unconditionally, after the SET FEATURES -
XFER MODE command completes successfully.

`->set_piomode()` is always called (if present), but `->set_dma_mode()`
is only called if DMA is possible.

#### Taskfile read/write

```
void (*sff_tf_load) (struct ata_port *ap, struct ata_taskfile *tf);
void (*sff_tf_read) (struct ata_port *ap, struct ata_taskfile *tf);
```

`->tf_load()` is called to load the given taskfile into hardware
registers / DMA buffers. `->tf_read()` is called to read the hardware
registers / DMA buffers, to obtain the current set of taskfile register
values. Most drivers for taskfile-based hardware (PIO or MMIO) use
`ata_sff_tf_load()` and `ata_sff_tf_read()` for these hooks.

#### PIO data read/write

```
void (*sff_data_xfer) (struct ata_device *, unsigned char *, unsigned int, int);
```

All bmdma-style drivers must implement this hook. This is the low-level
operation that actually copies the data bytes during a PIO data
transfer. Typically the driver will choose one of
`ata_sff_data_xfer()`, or `ata_sff_data_xfer32()`.

#### ATA command execute

```
void (*sff_exec_command)(struct ata_port *ap, struct ata_taskfile *tf);
```

causes an ATA command, previously loaded with `->tf_load()`, to be
initiated in hardware. Most drivers for taskfile-based hardware use
`ata_sff_exec_command()` for this hook.

#### Per-cmd ATAPI DMA capabilities filter

```
int (*check_atapi_dma) (struct ata_queued_cmd *qc);
```

Allow low-level driver to filter ATA PACKET commands, returning a status
indicating whether or not it is OK to use DMA for the supplied PACKET
command.

This hook may be specified as NULL, in which case libata will assume
that atapi dma can be supported.

#### Read specific ATA shadow registers

```
u8   (*sff_check_status)(struct ata_port *ap);
u8   (*sff_check_altstatus)(struct ata_port *ap);
```

Reads the Status/AltStatus ATA shadow register from hardware. On some
hardware, reading the Status register has the side effect of clearing
the interrupt condition. Most drivers for taskfile-based hardware use
`ata_sff_check_status()` for this hook.

#### Write specific ATA shadow register

```
void (*sff_set_devctl)(struct ata_port *ap, u8 ctl);
```

Write the device control ATA shadow register to the hardware. Most
drivers don’t need to define this.

#### Select ATA device on bus

```
void (*sff_dev_select)(struct ata_port *ap, unsigned int device);
```

Issues the low-level hardware command(s) that causes one of N hardware
devices to be considered ‘selected’ (active and available for use) on
the ATA bus. This generally has no meaning on FIS-based devices.

Most drivers for taskfile-based hardware use `ata_sff_dev_select()` for
this hook.

#### Private tuning method

```
void (*set_mode) (struct ata_port *ap);
```

By default libata performs drive and controller tuning in accordance
with the ATA timing rules and also applies blacklists and cable limits.
Some controllers need special handling and have custom tuning rules,
typically raid controllers that use ATA commands but do not actually do
drive timing.

> **Warning**
>
> This hook should not be used to replace the standard controller
> tuning logic when a controller has quirks. Replacing the default
> tuning logic in that case would bypass handling for drive and bridge
> quirks that may be important to data reliability. If a controller
> needs to filter the mode selection it should use the mode\_filter
> hook instead.

#### Control PCI IDE BMDMA engine

```
void (*bmdma_setup) (struct ata_queued_cmd *qc);
void (*bmdma_start) (struct ata_queued_cmd *qc);
void (*bmdma_stop) (struct ata_port *ap);
u8   (*bmdma_status) (struct ata_port *ap);
```

When setting up an IDE BMDMA transaction, these hooks arm
(`->bmdma_setup`), fire (`->bmdma_start`), and halt (`->bmdma_stop`) the
hardware’s DMA engine. `->bmdma_status` is used to read the standard PCI
IDE DMA Status register.

These hooks are typically either no-ops, or simply not implemented, in
FIS-based drivers.

Most legacy IDE drivers use `ata_bmdma_setup()` for the
`bmdma_setup()` hook. `ata_bmdma_setup()` will write the pointer
to the PRD table to the IDE PRD Table Address register, enable DMA in the DMA
Command register, and call `exec_command()` to begin the transfer.

Most legacy IDE drivers use `ata_bmdma_start()` for the
`bmdma_start()` hook. `ata_bmdma_start()` will write the
ATA\_DMA\_START flag to the DMA Command register.

Many legacy IDE drivers use `ata_bmdma_stop()` for the
`bmdma_stop()` hook. `ata_bmdma_stop()` clears the ATA\_DMA\_START
flag in the DMA command register.

Many legacy IDE drivers use `ata_bmdma_status()` as the
`bmdma_status()` hook.

#### High-level taskfile hooks

```
enum ata_completion_errors (*qc_prep) (struct ata_queued_cmd *qc);
int (*qc_issue) (struct ata_queued_cmd *qc);
```

Higher-level hooks, these two hooks can potentially supersede several of
the above taskfile/DMA engine hooks. `->qc_prep` is called after the
buffers have been DMA-mapped, and is typically used to populate the
hardware’s DMA scatter-gather table. Some drivers use the standard
`ata_bmdma_qc_prep()` and `ata_bmdma_dumb_qc_prep()` helper
functions, but more advanced drivers roll their own.

`->qc_issue` is used to make a command active, once the hardware and S/G
tables have been prepared. IDE BMDMA drivers use the helper function
`ata_sff_qc_issue()` for taskfile protocol-based dispatch. More
advanced drivers implement their own `->qc_issue`.

`ata_sff_qc_issue()` calls `->sff_tf_load()`, `->bmdma_setup()`, and
`->bmdma_start()` as necessary to initiate a transfer.

#### Exception and probe handling (EH)

```
void (*freeze) (struct ata_port *ap);
void (*thaw) (struct ata_port *ap);
```

[`ata_port_freeze()`](#c.ata_port_freeze "ata_port_freeze") is called when HSM violations or some other
condition disrupts normal operation of the port. A frozen port is not
allowed to perform any operation until the port is thawed, which usually
follows a successful reset.

The optional `->freeze()` callback can be used for freezing the port
hardware-wise (e.g. mask interrupt and stop DMA engine). If a port
cannot be frozen hardware-wise, the interrupt handler must ack and clear
interrupts unconditionally while the port is frozen.

The optional `->thaw()` callback is called to perform the opposite of
`->freeze()`: prepare the port for normal operation once again. Unmask
interrupts, start DMA engine, etc.

```
void (*error_handler) (struct ata_port *ap);
```

`->error_handler()` is a driver’s hook into probe, hotplug, and recovery
and other exceptional conditions. The primary responsibility of an
implementation is to call [`ata_std_error_handler()`](#c.ata_std_error_handler "ata_std_error_handler").

[`ata_std_error_handler()`](#c.ata_std_error_handler "ata_std_error_handler") will perform a standard error handling sequence
to resurrect failed devices, detach lost devices and add new devices (if any).
This function will call the various reset operations for a port, as needed.
These operations are as follows.

* The ‘prereset’ operation (which may be NULL) is called during an EH reset,
  before any other action is taken.
* The ‘postreset’ hook (which may be NULL) is called after the EH reset is
  performed. Based on existing conditions, severity of the problem, and hardware
  capabilities,
* Either the ‘softreset’ operation or the ‘hardreset’ operation will be called
  to perform the low-level EH reset. If both operations are defined,
  ‘hardreset’ is preferred and used. If both are not defined, no low-level reset
  is performed and EH assumes that an ATA class device is connected through the
  link.

```
void (*post_internal_cmd) (struct ata_queued_cmd *qc);
```

Perform any hardware-specific actions necessary to finish processing
after executing a probe-time or EH-time command via
[`ata_exec_internal()`](#c.ata_exec_internal "ata_exec_internal").

#### Hardware interrupt handling

```
irqreturn_t (*irq_handler)(int, void *, struct pt_regs *);
void (*irq_clear) (struct ata_port *);
```

`->irq_handler` is the interrupt handling routine registered with the
system, by libata. `->irq_clear` is called during probe just before the
interrupt handler is registered, to be sure hardware is quiet.

The second argument, dev\_instance, should be cast to a pointer to
`struct ata_host_set`.

Most legacy IDE drivers use `ata_sff_interrupt()` for the irq\_handler
hook, which scans all ports in the host\_set, determines which queued
command was active (if any), and calls ata\_sff\_host\_intr(ap,qc).

Most legacy IDE drivers use `ata_sff_irq_clear()` for the
`irq_clear()` hook, which simply clears the interrupt and error flags
in the DMA status register.

#### SATA phy read/write

```
int (*scr_read) (struct ata_port *ap, unsigned int sc_reg,
         u32 *val);
int (*scr_write) (struct ata_port *ap, unsigned int sc_reg,
                   u32 val);
```

Read and write standard SATA phy registers.
sc\_reg is one of SCR\_STATUS, SCR\_CONTROL, SCR\_ERROR, or SCR\_ACTIVE.

#### Init and shutdown

```
int (*port_start) (struct ata_port *ap);
void (*port_stop) (struct ata_port *ap);
void (*host_stop) (struct ata_host_set *host_set);
```

`->port_start()` is called just after the data structures for each port
are initialized. Typically this is used to alloc per-port DMA buffers /
tables / rings, enable DMA engines, and similar tasks. Some drivers also
use this entry point as a chance to allocate driver-private memory for
`ap->private_data`.

Many drivers use `ata_port_start()` as this hook or call it from their
own `port_start()` hooks. `ata_port_start()` allocates space for
a legacy IDE PRD table and returns.

`->port_stop()` is called after `->host_stop()`. Its sole function is to
release DMA/memory resources, now that they are no longer actively being
used. Many drivers also free driver-private data from port at this time.

`->host_stop()` is called after all `->port_stop()` calls have completed.
The hook must finalize hardware shutdown, release DMA and other
resources, etc. This hook may be specified as NULL, in which case it is
not called.

## Error handling

This chapter describes how errors are handled under libata. Readers are
advised to read SCSI EH ([SCSI EH](../scsi/scsi_eh.html)) and ATA
exceptions doc first.

### Origins of commands

In libata, a command is represented with
`struct ata_queued_cmd` or qc.
qc’s are preallocated during port initialization and repetitively used
for command executions. Currently only one qc is allocated per port but
yet-to-be-merged NCQ branch allocates one for each tag and maps each qc
to NCQ tag 1-to-1.

libata commands can originate from two sources - libata itself and SCSI
midlayer. libata internal commands are used for initialization and error
handling. All normal blk requests and commands for SCSI emulation are
passed as SCSI commands through queuecommand callback of SCSI host
template.

### How commands are issued

Internal commands
:   Once allocated qc’s taskfile is initialized for the command to be
    executed. qc currently has two mechanisms to notify completion. One
    is via `qc->complete_fn()` callback and the other is completion
    `qc->waiting`. `qc->complete_fn()` callback is the asynchronous path
    used by normal SCSI translated commands and `qc->waiting` is the
    synchronous (issuer sleeps in process context) path used by internal
    commands.

    Once initialization is complete, host\_set lock is acquired and the
    qc is issued.

SCSI commands
:   All libata drivers use [`ata_scsi_queuecmd()`](#c.ata_scsi_queuecmd "ata_scsi_queuecmd") as
    `hostt->queuecommand` callback. scmds can either be simulated or
    translated. No qc is involved in processing a simulated scmd. The
    result is computed right away and the scmd is completed.

    `qc->complete_fn()` callback is used for completion notification. ATA
    commands use `ata_scsi_qc_complete()` while ATAPI commands use
    `atapi_qc_complete()`. Both functions end up calling `qc->scsidone`
    to notify upper layer when the qc is finished. After translation is
    completed, the qc is issued with [`ata_qc_issue()`](#c.ata_qc_issue "ata_qc_issue").

    Note that SCSI midlayer invokes hostt->queuecommand while holding
    host\_set lock, so all above occur while holding host\_set lock.

### How commands are processed

Depending on which protocol and which controller are used, commands are
processed differently. For the purpose of discussion, a controller which
uses taskfile interface and all standard callbacks is assumed.

Currently 6 ATA command protocols are used. They can be sorted into the
following four categories according to how they are processed.

ATA NO DATA or DMA
:   ATA\_PROT\_NODATA and ATA\_PROT\_DMA fall into this category. These
    types of commands don’t require any software intervention once
    issued. Device will raise interrupt on completion.

ATA PIO
:   ATA\_PROT\_PIO is in this category. libata currently implements PIO
    with polling. ATA\_NIEN bit is set to turn off interrupt and
    pio\_task on ata\_wq performs polling and IO.

ATAPI NODATA or DMA
:   ATA\_PROT\_ATAPI\_NODATA and ATA\_PROT\_ATAPI\_DMA are in this
    category. packet\_task is used to poll BSY bit after issuing PACKET
    command. Once BSY is turned off by the device, packet\_task
    transfers CDB and hands off processing to interrupt handler.

ATAPI PIO
:   ATA\_PROT\_ATAPI is in this category. ATA\_NIEN bit is set and, as
    in ATAPI NODATA or DMA, packet\_task submits cdb. However, after
    submitting cdb, further processing (data transfer) is handed off to
    pio\_task.

### How commands are completed

Once issued, all qc’s are either completed with [`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete") or
time out. For commands which are handled by interrupts,
`ata_host_intr()` invokes [`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete"), and, for PIO tasks,
pio\_task invokes [`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete"). In error cases, packet\_task may
also complete commands.

[`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete") does the following.

1. DMA memory is unmapped.
2. ATA\_QCFLAG\_ACTIVE is cleared from qc->flags.
3. qc->complete\_fn callback is invoked. If the return value of the
   callback is not zero. Completion is short circuited and
   [`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete") returns.
4. `__ata_qc_complete()` is called, which does

   1. `qc->flags` is cleared to zero.
   2. `ap->active_tag` and `qc->tag` are poisoned.
   3. `qc->waiting` is cleared & completed (in that order).
   4. qc is deallocated by clearing appropriate bit in `ap->qactive`.

So, it basically notifies upper layer and deallocates qc. One exception
is short-circuit path in #3 which is used by `atapi_qc_complete()`.

For all non-ATAPI commands, whether it fails or not, almost the same
code path is taken and very little error handling takes place. A qc is
completed with success status if it succeeded, with failed status
otherwise.

However, failed ATAPI commands require more handling as REQUEST SENSE is
needed to acquire sense data. If an ATAPI command fails,
[`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete") is invoked with error status, which in turn invokes
`atapi_qc_complete()` via `qc->complete_fn()` callback.

This makes `atapi_qc_complete()` set `scmd->result` to
SAM\_STAT\_CHECK\_CONDITION, complete the scmd and return 1. As the
sense data is empty but `scmd->result` is CHECK CONDITION, SCSI midlayer
will invoke EH for the scmd, and returning 1 makes [`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete")
to return without deallocating the qc. This leads us to
[`ata_scsi_error()`](#c.ata_scsi_error "ata_scsi_error") with partially completed qc.

### [`ata_scsi_error()`](#c.ata_scsi_error "ata_scsi_error")

[`ata_scsi_error()`](#c.ata_scsi_error "ata_scsi_error") is the current `transportt->eh_strategy_handler()`
for libata. As discussed above, this will be entered in two cases -
timeout and ATAPI error completion. This function will check if a qc is active
and has not failed yet. Such a qc will be marked with AC\_ERR\_TIMEOUT such that
EH will know to handle it later. Then it calls low level libata driver’s
`error_handler()` callback.

When the `error_handler()` callback is invoked it stops BMDMA and
completes the qc. Note that as we’re currently in EH, we cannot call
scsi\_done. As described in SCSI EH doc, a recovered scmd should be
either retried with `scsi_queue_insert()` or finished with
`scsi_finish_command()`. Here, we override `qc->scsidone` with
`scsi_finish_command()` and calls [`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete").

If EH is invoked due to a failed ATAPI qc, the qc here is completed but
not deallocated. The purpose of this half-completion is to use the qc as
place holder to make EH code reach this place. This is a bit hackish,
but it works.

Once control reaches here, the qc is deallocated by invoking
`__ata_qc_complete()` explicitly. Then, internal qc for REQUEST SENSE
is issued. Once sense data is acquired, scmd is finished by directly
invoking `scsi_finish_command()` on the scmd. Note that as we already
have completed and deallocated the qc which was associated with the
scmd, we don’t need to/cannot call [`ata_qc_complete()`](#c.ata_qc_complete "ata_qc_complete") again.

### Problems with the current EH

* Error representation is too crude. Currently any and all error
  conditions are represented with ATA STATUS and ERROR registers.
  Errors which aren’t ATA device errors are treated as ATA device
  errors by setting ATA\_ERR bit. Better error descriptor which can
  properly represent ATA and other errors/exceptions is needed.
* When handling timeouts, no action is taken to make device forget
  about the timed out command and ready for new commands.
* EH handling via [`ata_scsi_error()`](#c.ata_scsi_error "ata_scsi_error") is not properly protected from
  usual command processing. On EH entrance, the device is not in
  quiescent state. Timed out commands may succeed or fail any time.
  pio\_task and atapi\_task may still be running.
* Too weak error recovery. Devices / controllers causing HSM mismatch
  errors and other errors quite often require reset to return to known
  state. Also, advanced error handling is necessary to support features
  like NCQ and hotplug.
* ATA errors are directly handled in the interrupt handler and PIO
  errors in pio\_task. This is problematic for advanced error handling
  for the following reasons.

  First, advanced error handling often requires context and internal qc
  execution.

  Second, even a simple failure (say, CRC error) needs information
  gathering and could trigger complex error handling (say, resetting &
  reconfiguring). Having multiple code paths to gather information,
  enter EH and trigger actions makes life painful.

  Third, scattered EH code makes implementing low level drivers
  difficult. Low level drivers override libata callbacks. If EH is
  scattered over several places, each affected callbacks should perform
  its part of error handling. This can be error prone and painful.

## libata Library

struct ata\_link \*ata\_link\_next(struct ata\_link \*link, struct ata\_port \*ap, enum ata\_link\_iter\_mode mode)
:   link iteration helper

**Parameters**

`struct ata_link *link`
:   the previous link, NULL to start

`struct ata_port *ap`
:   ATA port containing links to iterate

`enum ata_link_iter_mode mode`
:   iteration mode, one of ATA\_LITER\_\*

**Description**

> LOCKING:
> Host lock or EH context.

**Return**

Pointer to the next link.

struct ata\_device \*ata\_dev\_next(struct ata\_device \*dev, struct ata\_link \*link, enum ata\_dev\_iter\_mode mode)
:   device iteration helper

**Parameters**

`struct ata_device *dev`
:   the previous device, NULL to start

`struct ata_link *link`
:   ATA link containing devices to iterate

`enum ata_dev_iter_mode mode`
:   iteration mode, one of ATA\_DITER\_\*

**Description**

> LOCKING:
> Host lock or EH context.

**Return**

Pointer to the next device.

int atapi\_cmd\_type(u8 opcode)
:   Determine ATAPI command type from SCSI opcode

**Parameters**

`u8 opcode`
:   SCSI opcode

**Description**

> Determine ATAPI command type from **opcode**.
>
> LOCKING:
> None.

**Return**

ATAPI\_{READ|WRITE|READ\_CD|PASS\_THRU|MISC}

unsigned int ata\_pack\_xfermask(unsigned int pio\_mask, unsigned int mwdma\_mask, unsigned int udma\_mask)
:   Pack pio, mwdma and udma masks into xfer\_mask

**Parameters**

`unsigned int pio_mask`
:   pio\_mask

`unsigned int mwdma_mask`
:   mwdma\_mask

`unsigned int udma_mask`
:   udma\_mask

**Description**

> Pack **pio\_mask**, **mwdma\_mask** and **udma\_mask** into a single
> unsigned int xfer\_mask.
>
> LOCKING:
> None.

**Return**

Packed xfer\_mask.

u8 ata\_xfer\_mask2mode(unsigned int xfer\_mask)
:   Find matching XFER\_\* for the given xfer\_mask

**Parameters**

`unsigned int xfer_mask`
:   xfer\_mask of interest

**Description**

> Return matching XFER\_\* value for **xfer\_mask**. Only the highest
> bit of **xfer\_mask** is considered.
>
> LOCKING:
> None.

**Return**

Matching XFER\_\* value, 0xff if no match found.

unsigned int ata\_xfer\_mode2mask(u8 xfer\_mode)
:   Find matching xfer\_mask for XFER\_\*

**Parameters**

`u8 xfer_mode`
:   XFER\_\* of interest

**Description**

> Return matching xfer\_mask for **xfer\_mode**.
>
> LOCKING:
> None.

**Return**

Matching xfer\_mask, 0 if no match found.

int ata\_xfer\_mode2shift(u8 xfer\_mode)
:   Find matching xfer\_shift for XFER\_\*

**Parameters**

`u8 xfer_mode`
:   XFER\_\* of interest

**Description**

> Return matching xfer\_shift for **xfer\_mode**.
>
> LOCKING:
> None.

**Return**

Matching xfer\_shift, -1 if no match found.

const char \*ata\_mode\_string(unsigned int xfer\_mask)
:   convert xfer\_mask to string

**Parameters**

`unsigned int xfer_mask`
:   mask of bits supported; only highest bit counts.

**Description**

> Determine string which represents the highest speed
> (highest bit in **modemask**).
>
> LOCKING:
> None.

**Return**

Constant C string representing highest speed listed in
**mode\_mask**, or the constant C string “<n/a>”.

unsigned int ata\_dev\_classify(const struct ata\_taskfile \*tf)
:   determine device type based on ATA-spec signature

**Parameters**

`const struct ata_taskfile *tf`
:   ATA taskfile register set for device to be identified

**Description**

> Determine from taskfile register contents whether a device is
> ATA or ATAPI, as per “Signature and persistence” section
> of ATA/PI spec (volume 1, sect 5.14).
>
> LOCKING:
> None.

**Return**

Device type, `ATA_DEV_ATA`, `ATA_DEV_ATAPI`, `ATA_DEV_PMP`,
`ATA_DEV_ZAC`, or `ATA_DEV_UNKNOWN` the event of failure.

void ata\_id\_string(const u16 \*id, unsigned char \*s, unsigned int ofs, unsigned int len)
:   Convert IDENTIFY DEVICE page into string

**Parameters**

`const u16 *id`
:   IDENTIFY DEVICE results we will examine

`unsigned char *s`
:   string into which data is output

`unsigned int ofs`
:   offset into identify device page

`unsigned int len`
:   length of string to return. must be an even number.

**Description**

> The strings in the IDENTIFY DEVICE page are broken up into
> 16-bit chunks. Run through the string, and output each
> 8-bit chunk linearly, regardless of platform.
>
> LOCKING:
> caller.

void ata\_id\_c\_string(const u16 \*id, unsigned char \*s, unsigned int ofs, unsigned int len)
:   Convert IDENTIFY DEVICE page into C string

**Parameters**

`const u16 *id`
:   IDENTIFY DEVICE results we will examine

`unsigned char *s`
:   string into which data is output

`unsigned int ofs`
:   offset into identify device page

`unsigned int len`
:   length of string to return. must be an odd number.

**Description**

> This function is identical to ata\_id\_string except that it
> trims trailing spaces and terminates the resulting string with
> null. **len** must be actual maximum length (even number) + 1.
>
> LOCKING:
> caller.

unsigned int ata\_id\_xfermask(const u16 \*id)
:   Compute xfermask from the given IDENTIFY data

**Parameters**

`const u16 *id`
:   IDENTIFY data to compute xfer mask from

**Description**

> Compute the xfermask for this device. This is not as trivial
> as it seems if we must consider early devices correctly.
>
> FIXME: pre IDE drive timing (do we care ?).
>
> LOCKING:
> None.

**Return**

Computed xfermask

unsigned int ata\_pio\_need\_iordy(const struct ata\_device \*adev)
:   check if iordy needed

**Parameters**

`const struct ata_device *adev`
:   ATA device

**Description**

> Check if the current speed of the device requires IORDY. Used
> by various controllers for chip configuration.

unsigned int ata\_do\_dev\_read\_id(struct ata\_device \*dev, struct ata\_taskfile \*tf, \_\_le16 \*id)
:   default ID read method

**Parameters**

`struct ata_device *dev`
:   device

`struct ata_taskfile *tf`
:   proposed taskfile

`__le16 *id`
:   data buffer

**Description**

> Issue the identify taskfile and hand back the buffer containing
> identify data. For some RAID controllers and for pre ATA devices
> this function is wrapped or replaced by the driver

int ata\_cable\_40wire(struct ata\_port \*ap)
:   return 40 wire cable type

**Parameters**

`struct ata_port *ap`
:   port

**Description**

> Helper method for drivers which want to hardwire 40 wire cable
> detection.

int ata\_cable\_80wire(struct ata\_port \*ap)
:   return 80 wire cable type

**Parameters**

`struct ata_port *ap`
:   port

**Description**

> Helper method for drivers which want to hardwire 80 wire cable
> detection.

int ata\_cable\_unknown(struct ata\_port \*ap)
:   return unknown PATA cable.

**Parameters**

`struct ata_port *ap`
:   port

**Description**

> Helper method for drivers which have no PATA cable detection.

int ata\_cable\_ignore(struct ata\_port \*ap)
:   return ignored PATA cable.

**Parameters**

`struct ata_port *ap`
:   port

**Description**

> Helper method for drivers which don’t use cable type to limit
> transfer mode.

int ata\_cable\_sata(struct ata\_port \*ap)
:   return SATA cable type

**Parameters**

`struct ata_port *ap`
:   port

**Description**

> Helper method for drivers which have SATA cables

struct ata\_device \*ata\_dev\_pair(struct ata\_device \*adev)
:   return other device on cable

**Parameters**

`struct ata_device *adev`
:   device

**Description**

> Obtain the other device on the same cable, or if none is
> present NULL is returned

int ata\_set\_mode(struct ata\_link \*link, struct ata\_device \*\*r\_failed\_dev)
:   Program timings and issue SET FEATURES - XFER

**Parameters**

`struct ata_link *link`
:   link on which timings will be programmed

`struct ata_device **r_failed_dev`
:   out parameter for failed device

**Description**

> Standard implementation of the function used to tune and set
> ATA device disk transfer mode (PIO3, UDMA6, etc.). If
> `ata_dev_set_mode()` fails, pointer to the failing device is
> returned in **r\_failed\_dev**.
>
> LOCKING:
> PCI/etc. bus probe sem.

**Return**

0 on success, negative errno otherwise

int ata\_wait\_after\_reset(struct ata\_link \*link, unsigned long deadline, int (\*check\_ready)(struct ata\_link \*link))
:   wait for link to become ready after reset

**Parameters**

`struct ata_link *link`
:   link to be waited on

`unsigned long deadline`
:   deadline jiffies for the operation

`int (*check_ready)(struct ata_link *link)`
:   callback to check link readiness

**Description**

> Wait for **link** to become ready after reset.
>
> LOCKING:
> EH context.

**Return**

0 if **link** is ready before **deadline**; otherwise, -errno.

int ata\_std\_prereset(struct ata\_link \*link, unsigned long deadline)
:   prepare for reset

**Parameters**

`struct ata_link *link`
:   ATA link to be reset

`unsigned long deadline`
:   deadline jiffies for the operation

**Description**

> **link** is about to be reset. Initialize it. Failure from
> prereset makes libata abort whole reset sequence and give up
> that port, so prereset should be best-effort. It does its
> best to prepare for reset sequence but if things go wrong, it
> should just whine, not fail.
>
> LOCKING:
> Kernel thread context (may sleep)

**Return**

Always 0.

void ata\_std\_postreset(struct ata\_link \*link, unsigned int \*classes)
:   standard postreset callback

**Parameters**

`struct ata_link *link`
:   the target ata\_link

`unsigned int *classes`
:   classes of attached devices

**Description**

> This function is invoked after a successful reset. Note that
> the device might have been reset more than once using
> different reset methods before postreset is invoked.
>
> LOCKING:
> Kernel thread context (may sleep)

unsigned int ata\_dev\_set\_feature(struct ata\_device \*dev, u8 subcmd, u8 action)
:   Issue SET FEATURES

**Parameters**

`struct ata_device *dev`
:   Device to which command will be sent

`u8 subcmd`
:   The SET FEATURES subcommand to be sent

`u8 action`
:   The sector count represents a subcommand specific action

**Description**

> Issue SET FEATURES command to device **dev** on port **ap** with sector count
>
> LOCKING:
> PCI/etc. bus probe sem.

**Return**

0 on success, AC\_ERR\_\* mask otherwise.

int ata\_std\_qc\_defer(struct ata\_queued\_cmd \*qc)
:   Check whether a qc needs to be deferred

**Parameters**

`struct ata_queued_cmd *qc`
:   ATA command in question

**Description**

> Non-NCQ commands cannot run with any other command, NCQ or
> not. As upper layer only knows the queue depth, we are
> responsible for maintaining exclusion. This function checks
> whether a new command **qc** can be issued.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

ATA\_DEFER\_\* if deferring is needed, 0 otherwise.

void ata\_qc\_complete(struct ata\_queued\_cmd \*qc)
:   Complete an active ATA command

**Parameters**

`struct ata_queued_cmd *qc`
:   Command to complete

**Description**

> Indicate to the mid and upper layers that an ATA command has
> completed, with either an ok or not-ok status.
>
> Refrain from calling this function multiple times when
> successfully completing multiple NCQ commands.
> `ata_qc_complete_multiple()` should be used instead, which will
> properly update IRQ expect state.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

u64 ata\_qc\_get\_active(struct ata\_port \*ap)
:   get bitmask of active qcs

**Parameters**

`struct ata_port *ap`
:   port in question

**Description**

> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Bitmask of active qcs

bool ata\_link\_online(struct ata\_link \*link)
:   test whether the given link is online

**Parameters**

`struct ata_link *link`
:   ATA link to test

**Description**

> Test whether **link** is online. This is identical to
> [`ata_phys_link_online()`](#c.ata_phys_link_online "ata_phys_link_online") when there’s no slave link. When
> there’s a slave link, this function should only be called on
> the master link and will return true if any of M/S links is
> online.
>
> LOCKING:
> None.

**Return**

True if the port online status is available and online.

bool ata\_link\_offline(struct ata\_link \*link)
:   test whether the given link is offline

**Parameters**

`struct ata_link *link`
:   ATA link to test

**Description**

> Test whether **link** is offline. This is identical to
> [`ata_phys_link_offline()`](#c.ata_phys_link_offline "ata_phys_link_offline") when there’s no slave link. When
> there’s a slave link, this function should only be called on
> the master link and will return true if both M/S links are
> offline.
>
> LOCKING:
> None.

**Return**

True if the port offline status is available and offline.

void ata\_host\_suspend(struct ata\_host \*host, pm\_message\_t mesg)
:   suspend host

**Parameters**

`struct ata_host *host`
:   host to suspend

`pm_message_t mesg`
:   PM message

**Description**

> Suspend **host**. Actual operation is performed by port suspend.

void ata\_host\_resume(struct ata\_host \*host)
:   resume host

**Parameters**

`struct ata_host *host`
:   host to resume

**Description**

> Resume **host**. Actual operation is performed by port resume.

struct ata\_port \*ata\_port\_alloc(struct ata\_host \*host)
:   allocate and initialize basic ATA port resources

**Parameters**

`struct ata_host *host`
:   ATA host this allocated port belongs to

**Description**

> Allocate and initialize basic ATA port resources.
>
> LOCKING:
> Inherited from calling layer (may sleep).

**Return**

Allocate ATA port on success, NULL on failure.

struct ata\_host \*ata\_host\_alloc(struct [device](infrastructure.html#c.device "device") \*dev, int n\_ports)
:   allocate and init basic ATA host resources

**Parameters**

`struct device *dev`
:   generic device this host is associated with

`int n_ports`
:   the number of ATA ports associated with this host

**Description**

> Allocate and initialize basic ATA host resources. LLD calls
> this function to allocate a host, initializes it fully and
> attaches it using [`ata_host_register()`](#c.ata_host_register "ata_host_register").
>
> LOCKING:
> Inherited from calling layer (may sleep).

**Return**

Allocate ATA host on success, NULL on failure.

struct ata\_host \*ata\_host\_alloc\_pinfo(struct [device](infrastructure.html#c.device "device") \*dev, const struct ata\_port\_info \*const \*ppi, int n\_ports)
:   alloc host and init with port\_info array

**Parameters**

`struct device *dev`
:   generic device this host is associated with

`const struct ata_port_info * const * ppi`
:   array of ATA port\_info to initialize host with

`int n_ports`
:   number of ATA ports attached to this host

**Description**

> Allocate ATA host and initialize with info from **ppi**. If NULL
> terminated, **ppi** may contain fewer entries than **n\_ports**. The
> last entry will be used for the remaining ports.
>
> LOCKING:
> Inherited from calling layer (may sleep).

**Return**

Allocate ATA host on success, NULL on failure.

int ata\_host\_start(struct ata\_host \*host)
:   start and freeze ports of an ATA host

**Parameters**

`struct ata_host *host`
:   ATA host to start ports for

**Description**

> Start and then freeze ports of **host**. Started status is
> recorded in host->flags, so this function can be called
> multiple times. Ports are guaranteed to get started only
> once. If host->ops is not initialized yet, it is set to the
> first non-dummy port ops.
>
> LOCKING:
> Inherited from calling layer (may sleep).

**Return**

0 if all ports are started successfully, -errno otherwise.

void ata\_host\_init(struct ata\_host \*host, struct [device](infrastructure.html#c.device "device") \*dev, struct ata\_port\_operations \*ops)
:   Initialize a host struct for sas (ipr, libsas)

**Parameters**

`struct ata_host *host`
:   host to initialize

`struct device *dev`
:   device host is attached to

`struct ata_port_operations *ops`
:   port\_ops

int ata\_host\_register(struct ata\_host \*host, const struct scsi\_host\_template \*sht)
:   register initialized ATA host

**Parameters**

`struct ata_host *host`
:   ATA host to register

`const struct scsi_host_template *sht`
:   template for SCSI host

**Description**

> Register initialized ATA host. **host** is allocated using
> [`ata_host_alloc()`](#c.ata_host_alloc "ata_host_alloc") and fully initialized by LLD. This function
> starts ports, registers **host** with ATA and SCSI layers and
> probe registered devices.
>
> LOCKING:
> Inherited from calling layer (may sleep).

**Return**

0 on success, -errno otherwise.

int ata\_host\_activate(struct ata\_host \*host, int irq, irq\_handler\_t irq\_handler, unsigned long irq\_flags, const struct scsi\_host\_template \*sht)
:   start host, request IRQ and register it

**Parameters**

`struct ata_host *host`
:   target ATA host

`int irq`
:   IRQ to request

`irq_handler_t irq_handler`
:   irq\_handler used when requesting IRQ

`unsigned long irq_flags`
:   irq\_flags used when requesting IRQ

`const struct scsi_host_template *sht`
:   scsi\_host\_template to use when registering the host

**Description**

> After allocating an ATA host and initializing it, most libata
> LLDs perform three steps to activate the host - start host,
> request IRQ and register it. This helper takes necessary
> arguments and performs the three steps in one go.
>
> An invalid IRQ skips the IRQ registration and expects the host to
> have set polling mode on the port. In this case, **irq\_handler**
> should be NULL.
>
> LOCKING:
> Inherited from calling layer (may sleep).

**Return**

0 on success, -errno otherwise.

void ata\_host\_detach(struct ata\_host \*host)
:   Detach all ports of an ATA host

**Parameters**

`struct ata_host *host`
:   Host to detach

**Description**

> Detach all ports of **host**.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_pci\_remove\_one(struct pci\_dev \*pdev)
:   PCI layer callback for device removal

**Parameters**

`struct pci_dev *pdev`
:   PCI device that was removed

**Description**

> PCI layer indicates to libata via this hook that hot-unplug or
> module unload event has occurred. Detach all ports. Resource
> release is handled via devres.
>
> LOCKING:
> Inherited from PCI layer (may sleep).

void ata\_platform\_remove\_one(struct platform\_device \*pdev)
:   Platform layer callback for device removal

**Parameters**

`struct platform_device *pdev`
:   Platform device that was removed

**Description**

> Platform layer indicates to libata via this hook that hot-unplug or
> module unload event has occurred. Detach all ports. Resource
> release is handled via devres.
>
> LOCKING:
> Inherited from platform layer (may sleep).

void ata\_msleep(struct ata\_port \*ap, unsigned int msecs)
:   ATA EH owner aware msleep

**Parameters**

`struct ata_port *ap`
:   ATA port to attribute the sleep to

`unsigned int msecs`
:   duration to sleep in milliseconds

**Description**

> Sleeps **msecs**. If the current task is owner of **ap**’s EH, the
> ownership is released before going to sleep and reacquired
> after the sleep is complete. IOW, other ports sharing the
> **ap->host** will be allowed to own the EH while this task is
> sleeping.
>
> LOCKING:
> Might sleep.

u32 ata\_wait\_register(struct ata\_port \*ap, void \_\_iomem \*reg, u32 mask, u32 val, unsigned int interval, unsigned int timeout)
:   wait until register value changes

**Parameters**

`struct ata_port *ap`
:   ATA port to wait register for, can be NULL

`void __iomem *reg`
:   IO-mapped register

`u32 mask`
:   Mask to apply to read register value

`u32 val`
:   Wait condition

`unsigned int interval`
:   polling interval in milliseconds

`unsigned int timeout`
:   timeout in milliseconds

**Description**

> Waiting for some bits of register to change is a common
> operation for ATA controllers. This function reads 32bit LE
> IO-mapped register **reg** and tests for the following condition.
>
> (**\*reg** & mask) != val
>
> If the condition is met, it returns; otherwise, the process is
> repeated after **interval\_msec** until timeout.
>
> LOCKING:
> Kernel thread context (may sleep)

**Return**

The final register value.

## libata Core Internals

struct ata\_link \*ata\_dev\_phys\_link(struct ata\_device \*dev)
:   find physical link for a device

**Parameters**

`struct ata_device *dev`
:   ATA device to look up physical link for

**Description**

> Look up physical link which **dev** is attached to. Note that
> this is different from **dev->link** only when **dev** is on slave
> link. For all other cases, it’s the same as **dev->link**.
>
> LOCKING:
> Don’t care.

**Return**

Pointer to the found physical link.

void ata\_force\_cbl(struct ata\_port \*ap)
:   force cable type according to libata.force

**Parameters**

`struct ata_port *ap`
:   ATA port of interest

**Description**

> Force cable type according to libata.force and whine about it.
> The last entry which has matching port number is used, so it
> can be specified as part of device force parameters. For
> example, both “a:40c,1.00:udma4” and “1.00:40c,udma4” have the
> same effect.
>
> LOCKING:
> EH context.

void ata\_force\_pflags(struct ata\_port \*ap)
:   force port flags according to libata.force

**Parameters**

`struct ata_port *ap`
:   ATA port of interest

**Description**

> Force port flags according to libata.force and whine about it.
>
> LOCKING:
> EH context.

void ata\_force\_link\_limits(struct ata\_link \*link)
:   force link limits according to libata.force

**Parameters**

`struct ata_link *link`
:   ATA link of interest

**Description**

> Force link flags and SATA spd limit according to libata.force
> and whine about it. When only the port part is specified
> (e.g. 1:), the limit applies to all links connected to both
> the host link and all fan-out ports connected via PMP. If the
> device part is specified as 0 (e.g. 1.00:), it specifies the
> first fan-out link not the host link. Device number 15 always
> points to the host link whether PMP is attached or not. If the
> controller has slave link, device number 16 points to it.
>
> LOCKING:
> EH context.

void ata\_force\_xfermask(struct ata\_device \*dev)
:   force xfermask according to libata.force

**Parameters**

`struct ata_device *dev`
:   ATA device of interest

**Description**

> Force xfer\_mask according to libata.force and whine about it.
> For consistency with link selection, device number 15 selects
> the first device connected to the host link.
>
> LOCKING:
> EH context.

void ata\_force\_quirks(struct ata\_device \*dev)
:   force quirks according to libata.force

**Parameters**

`struct ata_device *dev`
:   ATA device of interest

**Description**

> Force quirks according to libata.force and whine about it.
> For consistency with link selection, device number 15 selects
> the first device connected to the host link.
>
> LOCKING:
> EH context.

bool ata\_set\_rwcmd\_protocol(struct ata\_device \*dev, struct ata\_taskfile \*tf)
:   set taskfile r/w command and protocol

**Parameters**

`struct ata_device *dev`
:   target device for the taskfile

`struct ata_taskfile *tf`
:   taskfile to examine and configure

**Description**

> Examine the device configuration and tf->flags to determine
> the proper read/write command and protocol to use for **tf**.
>
> LOCKING:
> caller.

u64 ata\_tf\_read\_block(const struct ata\_taskfile \*tf, struct ata\_device \*dev)
:   Read block address from ATA taskfile

**Parameters**

`const struct ata_taskfile *tf`
:   ATA taskfile of interest

`struct ata_device *dev`
:   ATA device **tf** belongs to

**Description**

> LOCKING:
> None.
>
> Read block address from **tf**. This function can handle all
> three address formats - LBA, LBA48 and CHS. tf->protocol and
> flags select the address format to use.

**Return**

Block address read from **tf**.

int ata\_build\_rw\_tf(struct ata\_queued\_cmd \*qc, u64 block, u32 n\_block, unsigned int tf\_flags, int cdl, int class)
:   Build ATA taskfile for given read/write request

**Parameters**

`struct ata_queued_cmd *qc`
:   Metadata associated with the taskfile to build

`u64 block`
:   Block address

`u32 n_block`
:   Number of blocks

`unsigned int tf_flags`
:   RW/FUA etc...

`int cdl`
:   Command duration limit index

`int class`
:   IO priority class

**Description**

> LOCKING:
> None.
>
> Build ATA taskfile for the command **qc** for read/write request described
> by **block**, **n\_block**, **tf\_flags** and **class**.
>
> 0 on success, -ERANGE if the request is too large for **dev**,
> -EINVAL if the request is invalid.

void ata\_unpack\_xfermask(unsigned int xfer\_mask, unsigned int \*pio\_mask, unsigned int \*mwdma\_mask, unsigned int \*udma\_mask)
:   Unpack xfer\_mask into pio, mwdma and udma masks

**Parameters**

`unsigned int xfer_mask`
:   xfer\_mask to unpack

`unsigned int *pio_mask`
:   resulting pio\_mask

`unsigned int *mwdma_mask`
:   resulting mwdma\_mask

`unsigned int *udma_mask`
:   resulting udma\_mask

**Description**

> Unpack **xfer\_mask** into **pio\_mask**, **mwdma\_mask** and **udma\_mask**.
> Any NULL destination masks will be ignored.

int ata\_read\_native\_max\_address(struct ata\_device \*dev, u64 \*max\_sectors)
:   Read native max address

**Parameters**

`struct ata_device *dev`
:   target device

`u64 *max_sectors`
:   out parameter for the result native max address

**Description**

> Perform an LBA48 or LBA28 native size query upon the device in
> question.

**Return**

0 on success, -EACCES if command is aborted by the drive.
-EIO on other errors.

int ata\_set\_max\_sectors(struct ata\_device \*dev, u64 new\_sectors)
:   Set max sectors

**Parameters**

`struct ata_device *dev`
:   target device

`u64 new_sectors`
:   new max sectors value to set for the device

**Description**

> Set max sectors of **dev** to **new\_sectors**.

**Return**

0 on success, -EACCES if command is aborted or denied (due to
previous non-volatile SET\_MAX) by the drive. -EIO on other
errors.

int ata\_hpa\_resize(struct ata\_device \*dev)
:   Resize a device with an HPA set

**Parameters**

`struct ata_device *dev`
:   Device to resize

**Description**

> Read the size of an LBA28 or LBA48 disk with HPA features and resize
> it if required to the full size of the media. The caller must check
> the drive has the HPA feature set enabled.

**Return**

0 on success, -errno on failure.

void ata\_dump\_id(struct ata\_device \*dev, const u16 \*id)
:   IDENTIFY DEVICE info debugging output

**Parameters**

`struct ata_device *dev`
:   device from which the information is fetched

`const u16 *id`
:   IDENTIFY DEVICE page to dump

**Description**

> Dump selected 16-bit words from the given IDENTIFY DEVICE
> page.
>
> LOCKING:
> caller.

unsigned int ata\_exec\_internal(struct ata\_device \*dev, struct ata\_taskfile \*tf, const u8 \*cdb, enum dma\_data\_direction dma\_dir, void \*buf, unsigned int buflen, unsigned int timeout)
:   execute libata internal command

**Parameters**

`struct ata_device *dev`
:   Device to which the command is sent

`struct ata_taskfile *tf`
:   Taskfile registers for the command and the result

`const u8 *cdb`
:   CDB for packet command

`enum dma_data_direction dma_dir`
:   Data transfer direction of the command

`void *buf`
:   Data buffer of the command

`unsigned int buflen`
:   Length of data buffer

`unsigned int timeout`
:   Timeout in msecs (0 for default)

**Description**

> Executes libata internal command with timeout. **tf** contains
> the command on entry and the result on return. Timeout and error
> conditions are reported via the return value. No recovery action
> is taken after a command times out. It is the caller’s duty to
> clean up after timeout.
>
> LOCKING:
> None. Should be called with kernel context, might sleep.

**Return**

Zero on success, AC\_ERR\_\* mask on failure

u32 ata\_pio\_mask\_no\_iordy(const struct ata\_device \*adev)
:   Return the non IORDY mask

**Parameters**

`const struct ata_device *adev`
:   ATA device

**Description**

> Compute the highest mode possible if we are not using iordy. Return
> -1 if no iordy mode is available.

int ata\_dev\_read\_id(struct ata\_device \*dev, unsigned int \*p\_class, unsigned int flags, u16 \*id)
:   Read ID data from the specified device

**Parameters**

`struct ata_device *dev`
:   target device

`unsigned int *p_class`
:   pointer to class of the target device (may be changed)

`unsigned int flags`
:   ATA\_READID\_\* flags

`u16 *id`
:   buffer to read IDENTIFY data into

**Description**

> Read ID data from the specified device. ATA\_CMD\_ID\_ATA is
> performed on ATA devices and ATA\_CMD\_ID\_ATAPI on ATAPI
> devices. This function also issues ATA\_CMD\_INIT\_DEV\_PARAMS
> for pre-ATA4 drives.
>
> FIXME: ATA\_CMD\_ID\_ATA is optional for early drives and right
> now we abort if we hit that case.
>
> LOCKING:
> Kernel thread context (may sleep)

**Return**

0 on success, -errno otherwise.

void ata\_dev\_power\_set\_standby(struct ata\_device \*dev)
:   Set a device power mode to standby

**Parameters**

`struct ata_device *dev`
:   target device

**Description**

> Issue a STANDBY IMMEDIATE command to set a device power mode to standby.
> For an HDD device, this spins down the disks.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_dev\_power\_set\_active(struct ata\_device \*dev)
:   Set a device power mode to active

**Parameters**

`struct ata_device *dev`
:   target device

**Description**

> Issue a VERIFY command to enter to ensure that the device is in the
> active power mode. For a spun-down HDD (standby or idle power mode),
> the VERIFY command will complete after the disk spins up.
>
> LOCKING:
> Kernel thread context (may sleep).

unsigned int ata\_read\_log\_page(struct ata\_device \*dev, u8 log, u8 page, void \*buf, unsigned int sectors)
:   read a specific log page

**Parameters**

`struct ata_device *dev`
:   target device

`u8 log`
:   log to read

`u8 page`
:   page to read

`void *buf`
:   buffer to store read page

`unsigned int sectors`
:   number of sectors to read

**Description**

> Read log page using READ\_LOG\_EXT command.
>
> LOCKING:
> Kernel thread context (may sleep).

**Return**

0 on success, AC\_ERR\_\* mask otherwise.

int ata\_dev\_configure(struct ata\_device \*dev)
:   Configure the specified ATA/ATAPI device

**Parameters**

`struct ata_device *dev`
:   Target device to configure

**Description**

> Configure **dev** according to **dev->id**. Generic and low-level
> driver specific fixups are also applied.
>
> LOCKING:
> Kernel thread context (may sleep)

**Return**

0 on success, -errno otherwise

void sata\_print\_link\_status(struct ata\_link \*link)
:   Print SATA link status

**Parameters**

`struct ata_link *link`
:   SATA link to printk link status about

**Description**

> This function prints link speed and status of a SATA link.
>
> LOCKING:
> None.

u8 ata\_timing\_cycle2mode(unsigned int xfer\_shift, int cycle)
:   find xfer mode for the specified cycle duration

**Parameters**

`unsigned int xfer_shift`
:   ATA\_SHIFT\_\* value for transfer type to examine.

`int cycle`
:   cycle duration in ns

**Description**

> Return matching xfer mode for **cycle**. The returned mode is of
> the transfer type specified by **xfer\_shift**. If **cycle** is too
> slow for **xfer\_shift**, 0xff is returned. If **cycle** is faster
> than the fastest known mode, the fasted mode is returned.
>
> LOCKING:
> None.

**Return**

Matching xfer\_mode, 0xff if no match found.

int ata\_down\_xfermask\_limit(struct ata\_device \*dev, unsigned int sel)
:   adjust dev xfer masks downward

**Parameters**

`struct ata_device *dev`
:   Device to adjust xfer masks

`unsigned int sel`
:   ATA\_DNXFER\_\* selector

**Description**

> Adjust xfer masks of **dev** downward. Note that this function
> does not apply the change. Invoking [`ata_set_mode()`](#c.ata_set_mode "ata_set_mode") afterwards
> will apply the limit.
>
> LOCKING:
> Inherited from caller.

**Return**

0 on success, negative errno on failure

int ata\_wait\_ready(struct ata\_link \*link, unsigned long deadline, int (\*check\_ready)(struct ata\_link \*link))
:   wait for link to become ready

**Parameters**

`struct ata_link *link`
:   link to be waited on

`unsigned long deadline`
:   deadline jiffies for the operation

`int (*check_ready)(struct ata_link *link)`
:   callback to check link readiness

**Description**

> Wait for **link** to become ready. **check\_ready** should return
> positive number if **link** is ready, 0 if it isn’t, -ENODEV if
> link doesn’t seem to be occupied, other errno for other error
> conditions.
>
> Transient -ENODEV conditions are allowed for
> ATA\_TMOUT\_FF\_WAIT.
>
> LOCKING:
> EH context.

**Return**

0 if **link** is ready before **deadline**; otherwise, -errno.

int ata\_dev\_same\_device(struct ata\_device \*dev, unsigned int new\_class, const u16 \*new\_id)
:   Determine whether new ID matches configured device

**Parameters**

`struct ata_device *dev`
:   device to compare against

`unsigned int new_class`
:   class of the new device

`const u16 *new_id`
:   IDENTIFY page of the new device

**Description**

> Compare **new\_class** and **new\_id** against **dev** and determine
> whether **dev** is the device indicated by **new\_class** and
> **new\_id**.
>
> LOCKING:
> None.

**Return**

1 if **dev** matches **new\_class** and **new\_id**, 0 otherwise.

int ata\_dev\_reread\_id(struct ata\_device \*dev, unsigned int readid\_flags)
:   Re-read IDENTIFY data

**Parameters**

`struct ata_device *dev`
:   target ATA device

`unsigned int readid_flags`
:   read ID flags

**Description**

> Re-read IDENTIFY page and make sure **dev** is still attached to
> the port.
>
> LOCKING:
> Kernel thread context (may sleep)

**Return**

0 on success, negative errno otherwise

int ata\_dev\_revalidate(struct ata\_device \*dev, unsigned int new\_class, unsigned int readid\_flags)
:   Revalidate ATA device

**Parameters**

`struct ata_device *dev`
:   device to revalidate

`unsigned int new_class`
:   new class code

`unsigned int readid_flags`
:   read ID flags

**Description**

> Re-read IDENTIFY page, make sure **dev** is still attached to the
> port and reconfigure it according to the new IDENTIFY page.
>
> LOCKING:
> Kernel thread context (may sleep)

**Return**

0 on success, negative errno otherwise

int ata\_is\_40wire(struct ata\_device \*dev)
:   check drive side detection

**Parameters**

`struct ata_device *dev`
:   device

**Description**

> Perform drive side detection decoding, allowing for device vendors
> who can’t follow the documentation.

int cable\_is\_40wire(struct ata\_port \*ap)
:   40/80/SATA decider

**Parameters**

`struct ata_port *ap`
:   port to consider

**Description**

> This function encapsulates the policy for speed management
> in one place. At the moment we don’t cache the result but
> there is a good case for setting ap->cbl to the result when
> we are called with unknown cables (and figuring out if it
> impacts hotplug at all).
>
> Return 1 if the cable appears to be 40 wire.

void ata\_dev\_xfermask(struct ata\_device \*dev)
:   Compute supported xfermask of the given device

**Parameters**

`struct ata_device *dev`
:   Device to compute xfermask for

**Description**

> Compute supported xfermask of **dev** and store it in
> dev->\*\_mask. This function is responsible for applying all
> known limits including host controller limits, device quirks, etc...
>
> LOCKING:
> None.

unsigned int ata\_dev\_set\_xfermode(struct ata\_device \*dev)
:   Issue SET FEATURES - XFER MODE command

**Parameters**

`struct ata_device *dev`
:   Device to which command will be sent

**Description**

> Issue SET FEATURES - XFER MODE command to device **dev**
> on port **ap**.
>
> LOCKING:
> PCI/etc. bus probe sem.

**Return**

0 on success, AC\_ERR\_\* mask otherwise.

unsigned int ata\_dev\_init\_params(struct ata\_device \*dev, u16 heads, u16 sectors)
:   Issue INIT DEV PARAMS command

**Parameters**

`struct ata_device *dev`
:   Device to which command will be sent

`u16 heads`
:   Number of heads (taskfile parameter)

`u16 sectors`
:   Number of sectors (taskfile parameter)

**Description**

> LOCKING:
> Kernel thread context (may sleep)

**Return**

0 on success, AC\_ERR\_\* mask otherwise.

int atapi\_check\_dma(struct ata\_queued\_cmd \*qc)
:   Check whether ATAPI DMA can be supported

**Parameters**

`struct ata_queued_cmd *qc`
:   Metadata associated with taskfile to check

**Description**

> Allow low-level driver to filter ATA PACKET commands, returning
> a status indicating whether or not it is OK to use DMA for the
> supplied PACKET command.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

0 when ATAPI DMA can be used
nonzero otherwise

void ata\_sg\_init(struct ata\_queued\_cmd \*qc, struct scatterlist \*sg, unsigned int n\_elem)
:   Associate command with scatter-gather table.

**Parameters**

`struct ata_queued_cmd *qc`
:   Command to be associated

`struct scatterlist *sg`
:   Scatter-gather table.

`unsigned int n_elem`
:   Number of elements in s/g table.

**Description**

> Initialize the data-related elements of queued\_cmd **qc**
> to point to a scatter-gather table **sg**, containing **n\_elem**
> elements.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_sg\_clean(struct ata\_queued\_cmd \*qc)
:   Unmap DMA memory associated with command

**Parameters**

`struct ata_queued_cmd *qc`
:   Command containing DMA memory to be released

**Description**

> Unmap all mapped DMA memory associated with this command.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

int ata\_sg\_setup(struct ata\_queued\_cmd \*qc)
:   DMA-map the scatter-gather table associated with a command.

**Parameters**

`struct ata_queued_cmd *qc`
:   Command with scatter-gather table to be mapped.

**Description**

> DMA-map the scatter-gather table associated with queued\_cmd **qc**.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Zero on success, negative on error.

void swap\_buf\_le16(u16 \*buf, unsigned int buf\_words)
:   swap halves of 16-bit words in place

**Parameters**

`u16 *buf`
:   Buffer to swap

`unsigned int buf_words`
:   Number of 16-bit words in buffer.

**Description**

> Swap halves of 16-bit words if needed to convert from
> little-endian byte order to native cpu byte order, or
> vice-versa.
>
> LOCKING:
> Inherited from caller.

void ata\_qc\_free(struct ata\_queued\_cmd \*qc)
:   free unused ata\_queued\_cmd

**Parameters**

`struct ata_queued_cmd *qc`
:   Command to complete

**Description**

> Designed to free unused ata\_queued\_cmd object
> in case something prevents using it.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_qc\_issue(struct ata\_queued\_cmd \*qc)
:   issue taskfile to device

**Parameters**

`struct ata_queued_cmd *qc`
:   command to issue to device

**Description**

> Prepare an ATA command to submission to device.
> This includes mapping the data into a DMA-able
> area, filling in the S/G table, and finally
> writing the taskfile to hardware, starting the command.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

bool ata\_phys\_link\_online(struct ata\_link \*link)
:   test whether the given link is online

**Parameters**

`struct ata_link *link`
:   ATA link to test

**Description**

> Test whether **link** is online. Note that this function returns
> 0 if online status of **link** cannot be obtained, so
> ata\_link\_online(link) != !ata\_link\_offline(link).
>
> LOCKING:
> None.

**Return**

True if the port online status is available and online.

bool ata\_phys\_link\_offline(struct ata\_link \*link)
:   test whether the given link is offline

**Parameters**

`struct ata_link *link`
:   ATA link to test

**Description**

> Test whether **link** is offline. Note that this function
> returns 0 if offline status of **link** cannot be obtained, so
> ata\_link\_online(link) != !ata\_link\_offline(link).
>
> LOCKING:
> None.

**Return**

True if the port offline status is available and offline.

void ata\_dev\_init(struct ata\_device \*dev)
:   Initialize an ata\_device structure

**Parameters**

`struct ata_device *dev`
:   Device structure to initialize

**Description**

> Initialize **dev** in preparation for probing.
>
> LOCKING:
> Inherited from caller.

void ata\_link\_init(struct ata\_port \*ap, struct ata\_link \*link, int pmp)
:   Initialize an ata\_link structure

**Parameters**

`struct ata_port *ap`
:   ATA port link is attached to

`struct ata_link *link`
:   Link structure to initialize

`int pmp`
:   Port multiplier port number

**Description**

> Initialize **link**.
>
> LOCKING:
> Kernel thread context (may sleep)

int sata\_link\_init\_spd(struct ata\_link \*link)
:   Initialize link->sata\_spd\_limit

**Parameters**

`struct ata_link *link`
:   Link to configure sata\_spd\_limit for

**Description**

> Initialize `link->[hw_]sata_spd_limit` to the currently
> configured value.
>
> LOCKING:
> Kernel thread context (may sleep).

**Return**

0 on success, -errno on failure.

void ata\_finalize\_port\_ops(struct ata\_port\_operations \*ops)
:   finalize ata\_port\_operations

**Parameters**

`struct ata_port_operations *ops`
:   ata\_port\_operations to finalize

**Description**

> An ata\_port\_operations can inherit from another ops and that
> ops can again inherit from another. This can go on as many
> times as necessary as long as there is no loop in the
> inheritance chain.
>
> Ops tables are finalized when the host is started. NULL or
> unspecified entries are inherited from the closet ancestor
> which has the method and the entry is populated with it.
> After finalization, the ops table directly points to all the
> methods and ->inherits is no longer necessary and cleared.
>
> Using ATA\_OP\_NULL, inheriting ops can force a method to NULL.
>
> LOCKING:
> None.

void ata\_dev\_free\_resources(struct ata\_device \*dev)
:   Free a device resources

**Parameters**

`struct ata_device *dev`
:   Target ATA device

**Description**

> Free resources allocated to support a device features.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_port\_detach(struct ata\_port \*ap)
:   Detach ATA port in preparation of device removal

**Parameters**

`struct ata_port *ap`
:   ATA port to be detached

**Description**

> Detach all ATA devices and the associated SCSI devices of **ap**;
> then, remove the associated SCSI host. **ap** is guaranteed to
> be quiescent on return from this function.
>
> LOCKING:
> Kernel thread context (may sleep).

void \_\_ata\_ehi\_push\_desc(struct ata\_eh\_info \*ehi, const char \*fmt, ...)
:   push error description without adding separator

**Parameters**

`struct ata_eh_info *ehi`
:   target EHI

`const char *fmt`
:   printf format string

`...`
:   variable arguments

**Description**

> Format string according to **fmt** and append it to **ehi->desc**.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_ehi\_push\_desc(struct ata\_eh\_info \*ehi, const char \*fmt, ...)
:   push error description with separator

**Parameters**

`struct ata_eh_info *ehi`
:   target EHI

`const char *fmt`
:   printf format string

`...`
:   variable arguments

**Description**

> Format string according to **fmt** and append it to **ehi->desc**.
> If **ehi->desc** is not empty, “, “ is added in-between.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_ehi\_clear\_desc(struct ata\_eh\_info \*ehi)
:   clean error description

**Parameters**

`struct ata_eh_info *ehi`
:   target EHI

**Description**

> Clear **ehi->desc**.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_port\_desc(struct ata\_port \*ap, const char \*fmt, ...)
:   append port description

**Parameters**

`struct ata_port *ap`
:   target ATA port

`const char *fmt`
:   printf format string

`...`
:   variable arguments

**Description**

> Format string according to **fmt** and append it to port
> description. If port description is not empty, “ “ is added
> in-between. This function is to be used while initializing
> ata\_host. The description is printed on host registration.
>
> LOCKING:
> None.

void ata\_port\_pbar\_desc(struct ata\_port \*ap, int bar, ssize\_t offset, const char \*name)
:   append PCI BAR description

**Parameters**

`struct ata_port *ap`
:   target ATA port

`int bar`
:   target PCI BAR

`ssize_t offset`
:   offset into PCI BAR

`const char *name`
:   name of the area

**Description**

> If **offset** is negative, this function formats a string which
> contains the name, address, size and type of the BAR and
> appends it to the port description. If **offset** is zero or
> positive, only name and offsetted address is appended.
>
> LOCKING:
> None.

unsigned int ata\_internal\_cmd\_timeout(struct ata\_device \*dev, u8 cmd)
:   determine timeout for an internal command

**Parameters**

`struct ata_device *dev`
:   target device

`u8 cmd`
:   internal command to be issued

**Description**

> Determine timeout for internal command **cmd** for **dev**.
>
> LOCKING:
> EH context.

**Return**

Determined timeout.

void ata\_internal\_cmd\_timed\_out(struct ata\_device \*dev, u8 cmd)
:   notification for internal command timeout

**Parameters**

`struct ata_device *dev`
:   target device

`u8 cmd`
:   internal command which timed out

**Description**

> Notify EH that internal command **cmd** for **dev** timed out. This
> function should be called only for commands whose timeouts are
> determined using [`ata_internal_cmd_timeout()`](#c.ata_internal_cmd_timeout "ata_internal_cmd_timeout").
>
> LOCKING:
> EH context.

void ata\_eh\_acquire(struct ata\_port \*ap)
:   acquire EH ownership

**Parameters**

`struct ata_port *ap`
:   ATA port to acquire EH ownership for

**Description**

> Acquire EH ownership for **ap**. This is the basic exclusion
> mechanism for ports sharing a host. Only one port hanging off
> the same host can claim the ownership of EH.
>
> LOCKING:
> EH context.

void ata\_eh\_release(struct ata\_port \*ap)
:   release EH ownership

**Parameters**

`struct ata_port *ap`
:   ATA port to release EH ownership for

**Description**

> Release EH ownership for **ap** if the caller. The caller must
> have acquired EH ownership using [`ata_eh_acquire()`](#c.ata_eh_acquire "ata_eh_acquire") previously.
>
> LOCKING:
> EH context.

void ata\_scsi\_error(struct Scsi\_Host \*host)
:   SCSI layer error handler callback

**Parameters**

`struct Scsi_Host *host`
:   SCSI host on which error occurred

**Description**

> Handles SCSI-layer-thrown error events.
>
> LOCKING:
> Inherited from SCSI layer (none, can sleep)

**Return**

Zero.

int ata\_scsi\_cmd\_error\_handler(struct Scsi\_Host \*host, struct ata\_port \*ap, struct list\_head \*eh\_work\_q)
:   error callback for a list of commands

**Parameters**

`struct Scsi_Host *host`
:   scsi host containing the port

`struct ata_port *ap`
:   ATA port within the host

`struct list_head *eh_work_q`
:   list of commands to process

**Description**

process the given list of commands and return those finished to the
ap->eh\_done\_q. This function is the first part of the libata error
handler which processes a given list of failed commands.

Return the number of commands that timed out.

void ata\_scsi\_port\_error\_handler(struct Scsi\_Host \*host, struct ata\_port \*ap)
:   recover the port after the commands

**Parameters**

`struct Scsi_Host *host`
:   SCSI host containing the port

`struct ata_port *ap`
:   the ATA port

**Description**

Handle the recovery of the port **ap** after all the commands
have been recovered.

void ata\_port\_wait\_eh(struct ata\_port \*ap)
:   Wait for the currently pending EH to complete

**Parameters**

`struct ata_port *ap`
:   Port to wait EH for

**Description**

> Wait until the currently pending EH is complete.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_eh\_set\_pending(struct ata\_port \*ap, bool fastdrain)
:   set ATA\_PFLAG\_EH\_PENDING and activate fast drain

**Parameters**

`struct ata_port *ap`
:   target ATA port

`bool fastdrain`
:   activate fast drain

**Description**

> Set ATA\_PFLAG\_EH\_PENDING and activate fast drain if **fastdrain**
> is non-zero and EH wasn’t pending before. Fast drain ensures
> that EH kicks in in timely manner.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_qc\_schedule\_eh(struct ata\_queued\_cmd \*qc)
:   schedule qc for error handling

**Parameters**

`struct ata_queued_cmd *qc`
:   command to schedule error handling for

**Description**

> Schedule error handling for **qc**. EH will kick in as soon as
> other commands are drained.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_std\_sched\_eh(struct ata\_port \*ap)
:   non-libsas ata\_ports issue eh with this common routine

**Parameters**

`struct ata_port *ap`
:   ATA port to schedule EH for

**Description**

> LOCKING: inherited from ata\_port\_schedule\_eh
> spin\_lock\_irqsave(host lock)

void ata\_std\_end\_eh(struct ata\_port \*ap)
:   non-libsas ata\_ports complete eh with this common routine

**Parameters**

`struct ata_port *ap`
:   ATA port to end EH for

**Description**

In the libata object model there is a 1:1 mapping of ata\_port to
shost, so host fields can be directly manipulated under ap->lock, in
the libsas case we need to hold a lock at the ha->level to coordinate
these events.

> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_port\_schedule\_eh(struct ata\_port \*ap)
:   schedule error handling without a qc

**Parameters**

`struct ata_port *ap`
:   ATA port to schedule EH for

**Description**

> Schedule error handling for **ap**. EH will kick in as soon as
> all commands are drained.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

int ata\_link\_abort(struct ata\_link \*link)
:   abort all qc’s on the link

**Parameters**

`struct ata_link *link`
:   ATA link to abort qc’s for

**Description**

> Abort all active qc’s active on **link** and schedule EH.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Number of aborted qc’s.

int ata\_port\_abort(struct ata\_port \*ap)
:   abort all qc’s on the port

**Parameters**

`struct ata_port *ap`
:   ATA port to abort qc’s for

**Description**

> Abort all active qc’s of **ap** and schedule EH.
>
> LOCKING:
> spin\_lock\_irqsave(host\_set lock)

**Return**

Number of aborted qc’s.

void \_\_ata\_port\_freeze(struct ata\_port \*ap)
:   freeze port

**Parameters**

`struct ata_port *ap`
:   ATA port to freeze

**Description**

> This function is called when HSM violation or some other
> condition disrupts normal operation of the port. Frozen port
> is not allowed to perform any operation until the port is
> thawed, which usually follows a successful reset.
>
> ap->ops->`freeze()` callback can be used for freezing the port
> hardware-wise (e.g. mask interrupt and stop DMA engine). If a
> port cannot be frozen hardware-wise, the interrupt handler
> must ack and clear interrupts unconditionally while the port
> is frozen.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

int ata\_port\_freeze(struct ata\_port \*ap)
:   abort & freeze port

**Parameters**

`struct ata_port *ap`
:   ATA port to freeze

**Description**

> Abort and freeze **ap**. The freeze operation must be called
> first, because some hardware requires special operations
> before the taskfile registers are accessible.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Number of aborted commands.

void ata\_eh\_freeze\_port(struct ata\_port \*ap)
:   EH helper to freeze port

**Parameters**

`struct ata_port *ap`
:   ATA port to freeze

**Description**

> Freeze **ap**.
>
> LOCKING:
> None.

void ata\_eh\_thaw\_port(struct ata\_port \*ap)
:   EH helper to thaw port

**Parameters**

`struct ata_port *ap`
:   ATA port to thaw

**Description**

> Thaw frozen port **ap**.
>
> LOCKING:
> None.

void ata\_eh\_qc\_complete(struct ata\_queued\_cmd \*qc)
:   Complete an active ATA command from EH

**Parameters**

`struct ata_queued_cmd *qc`
:   Command to complete

**Description**

> Indicate to the mid and upper layers that an ATA command has
> completed. To be used from EH.

void ata\_eh\_qc\_retry(struct ata\_queued\_cmd \*qc)
:   Tell midlayer to retry an ATA command after EH

**Parameters**

`struct ata_queued_cmd *qc`
:   Command to retry

**Description**

> Indicate to the mid and upper layers that an ATA command
> should be retried. To be used from EH.
>
> SCSI midlayer limits the number of retries to scmd->allowed.
> scmd->allowed is incremented for commands which get retried
> due to unrelated failures (qc->err\_mask is zero).

void ata\_dev\_disable(struct ata\_device \*dev)
:   disable ATA device

**Parameters**

`struct ata_device *dev`
:   ATA device to disable

**Description**

> Disable **dev**.
>
> Locking:
> EH context.

void ata\_eh\_detach\_dev(struct ata\_device \*dev)
:   detach ATA device

**Parameters**

`struct ata_device *dev`
:   ATA device to detach

**Description**

> Detach **dev**.
>
> LOCKING:
> None.

void ata\_eh\_about\_to\_do(struct ata\_link \*link, struct ata\_device \*dev, unsigned int action)
:   about to perform eh\_action

**Parameters**

`struct ata_link *link`
:   target ATA link

`struct ata_device *dev`
:   target ATA dev for per-dev action (can be NULL)

`unsigned int action`
:   action about to be performed

**Description**

> Called just before performing EH actions to clear related bits
> in **link->eh\_info** such that eh actions are not unnecessarily
> repeated.
>
> LOCKING:
> None.

void ata\_eh\_done(struct ata\_link \*link, struct ata\_device \*dev, unsigned int action)
:   EH action complete

**Parameters**

`struct ata_link *link`
:   ATA link for which EH actions are complete

`struct ata_device *dev`
:   target ATA dev for per-dev action (can be NULL)

`unsigned int action`
:   action just completed

**Description**

> Called right after performing EH actions to clear related bits
> in **link->eh\_context**.
>
> LOCKING:
> None.

const char \*ata\_err\_string(unsigned int err\_mask)
:   convert err\_mask to descriptive string

**Parameters**

`unsigned int err_mask`
:   error mask to convert to string

**Description**

> Convert **err\_mask** to descriptive string. Errors are
> prioritized according to severity and only the most severe
> error is reported.
>
> LOCKING:
> None.

**Return**

Descriptive string for **err\_mask**

unsigned int atapi\_eh\_tur(struct ata\_device \*dev, u8 \*r\_sense\_key)
:   perform ATAPI TEST\_UNIT\_READY

**Parameters**

`struct ata_device *dev`
:   target ATAPI device

`u8 *r_sense_key`
:   out parameter for sense\_key

**Description**

> Perform ATAPI TEST\_UNIT\_READY.
>
> LOCKING:
> EH context (may sleep).

**Return**

0 on success, AC\_ERR\_\* mask on failure.

enum scsi\_disposition ata\_eh\_decide\_disposition(struct ata\_queued\_cmd \*qc)
:   Disposition a qc based on sense data

**Parameters**

`struct ata_queued_cmd *qc`
:   qc to examine

**Description**

> For a regular SCSI command, the SCSI completion callback (`scsi_done()`)
> will call `scsi_complete()`, which will call `scsi_decide_disposition()`,
> which will call [`scsi_check_sense()`](scsi.html#c.scsi_check_sense "scsi_check_sense"). `scsi_complete()` finally calls
> `scsi_finish_command()`. This is fine for SCSI, since any eventual sense
> data is usually returned in the completion itself (without invoking SCSI
> EH). However, for a QC, we always need to fetch the sense data
> explicitly using SCSI EH.
>
> A command that is completed via SCSI EH will instead be completed using
> [`scsi_eh_flush_done_q()`](scsi.html#c.scsi_eh_flush_done_q "scsi_eh_flush_done_q"), which will call `scsi_finish_command()` directly
> (without ever calling [`scsi_check_sense()`](scsi.html#c.scsi_check_sense "scsi_check_sense")).
>
> For a command that went through SCSI EH, it is the responsibility of the
> SCSI EH strategy handler to call `scsi_decide_disposition()`, see e.g. how
> [`scsi_eh_get_sense()`](scsi.html#c.scsi_eh_get_sense "scsi_eh_get_sense") calls `scsi_decide_disposition()` for SCSI LLDDs that
> do not get the sense data as part of the completion.
>
> Thus, for QC commands that went via SCSI EH, we need to call
> [`scsi_check_sense()`](scsi.html#c.scsi_check_sense "scsi_check_sense") ourselves, similar to how [`scsi_eh_get_sense()`](scsi.html#c.scsi_eh_get_sense "scsi_eh_get_sense") calls
> `scsi_decide_disposition()`, which calls [`scsi_check_sense()`](scsi.html#c.scsi_check_sense "scsi_check_sense"), in order to
> set the correct SCSI ML byte (if any).
>
> LOCKING:
> EH context.

**Return**

SUCCESS or FAILED or NEEDS\_RETRY or ADD\_TO\_MLQUEUE

bool ata\_eh\_request\_sense(struct ata\_queued\_cmd \*qc)
:   perform REQUEST\_SENSE\_DATA\_EXT

**Parameters**

`struct ata_queued_cmd *qc`
:   qc to perform REQUEST\_SENSE\_SENSE\_DATA\_EXT to

**Description**

> Perform REQUEST\_SENSE\_DATA\_EXT after the device reported CHECK
> SENSE. This function is an EH helper.
>
> LOCKING:
> Kernel thread context (may sleep).

**Return**

true if sense data could be fetched, false otherwise.

unsigned int atapi\_eh\_request\_sense(struct ata\_device \*dev, u8 \*sense\_buf, u8 dfl\_sense\_key)
:   perform ATAPI REQUEST\_SENSE

**Parameters**

`struct ata_device *dev`
:   device to perform REQUEST\_SENSE to

`u8 *sense_buf`
:   result sense data buffer (SCSI\_SENSE\_BUFFERSIZE bytes long)

`u8 dfl_sense_key`
:   default sense key to use

**Description**

> Perform ATAPI REQUEST\_SENSE after the device reported CHECK
> SENSE. This function is EH helper.
>
> LOCKING:
> Kernel thread context (may sleep).

**Return**

0 on success, AC\_ERR\_\* mask on failure

void ata\_eh\_analyze\_serror(struct ata\_link \*link)
:   analyze SError for a failed port

**Parameters**

`struct ata_link *link`
:   ATA link to analyze SError for

**Description**

> Analyze SError if available and further determine cause of
> failure.
>
> LOCKING:
> None.

unsigned int ata\_eh\_analyze\_tf(struct ata\_queued\_cmd \*qc)
:   analyze taskfile of a failed qc

**Parameters**

`struct ata_queued_cmd *qc`
:   qc to analyze

**Description**

> Analyze taskfile of **qc** and further determine cause of
> failure. This function also requests ATAPI sense data if
> available.
>
> LOCKING:
> Kernel thread context (may sleep).

**Return**

Determined recovery action

unsigned int ata\_eh\_speed\_down\_verdict(struct ata\_device \*dev)
:   Determine speed down verdict

**Parameters**

`struct ata_device *dev`
:   Device of interest

**Description**

> This function examines error ring of **dev** and determines
> whether NCQ needs to be turned off, transfer speed should be
> stepped down, or falling back to PIO is necessary.
>
> ECAT\_ATA\_BUS : ATA\_BUS error for any command
>
> ECAT\_TOUT\_HSMTIMEOUT for any command or HSM violation for
> :   IO commands
>
> ECAT\_UNK\_DEV : Unknown DEV error for IO commands
>
> ECAT\_DUBIOUS\_\*Identical to above three but occurred while
> :   data transfer hasn’t been verified.
>
> Verdicts are
>
> NCQ\_OFF : Turn off NCQ.
>
> SPEED\_DOWNSpeed down transfer speed but don’t fall back
> :   to PIO.
>
> FALLBACK\_TO\_PIO : Fall back to PIO.
>
> Even if multiple verdicts are returned, only one action is
> taken per error. An action triggered by non-DUBIOUS errors
> clears ering, while one triggered by DUBIOUS\_\* errors doesn’t.
> This is to expedite speed down decisions right after device is
> initially configured.
>
> The following are speed down rules. #1 and #2 deal with
> DUBIOUS errors.
>
> 1. If more than one DUBIOUS\_ATA\_BUS or DUBIOUS\_TOUT\_HSM errors
>    occurred during last 5 mins, SPEED\_DOWN and FALLBACK\_TO\_PIO.
> 2. If more than one DUBIOUS\_TOUT\_HSM or DUBIOUS\_UNK\_DEV errors
>    occurred during last 5 mins, NCQ\_OFF.
> 3. If more than 8 ATA\_BUS, TOUT\_HSM or UNK\_DEV errors
>    occurred during last 5 mins, FALLBACK\_TO\_PIO
> 4. If more than 3 TOUT\_HSM or UNK\_DEV errors occurred
>    during last 10 mins, NCQ\_OFF.
> 5. If more than 3 ATA\_BUS or TOUT\_HSM errors, or more than 6
>    UNK\_DEV errors occurred during last 10 mins, SPEED\_DOWN.
>
> LOCKING:
> Inherited from caller.

**Return**

OR of ATA\_EH\_SPDN\_\* flags.

unsigned int ata\_eh\_speed\_down(struct ata\_device \*dev, unsigned int eflags, unsigned int err\_mask)
:   record error and speed down if necessary

**Parameters**

`struct ata_device *dev`
:   Failed device

`unsigned int eflags`
:   mask of ATA\_EFLAG\_\* flags

`unsigned int err_mask`
:   err\_mask of the error

**Description**

> Record error and examine error history to determine whether
> adjusting transmission speed is necessary. It also sets
> transmission limits appropriately if such adjustment is
> necessary.
>
> LOCKING:
> Kernel thread context (may sleep).

**Return**

Determined recovery action.

int ata\_eh\_worth\_retry(struct ata\_queued\_cmd \*qc)
:   analyze error and decide whether to retry

**Parameters**

`struct ata_queued_cmd *qc`
:   qc to possibly retry

**Description**

> Look at the cause of the error and decide if a retry
> might be useful or not. We don’t want to retry media errors
> because the drive itself has probably already taken 10-30 seconds
> doing its own internal retries before reporting the failure.

bool ata\_eh\_quiet(struct ata\_queued\_cmd \*qc)
:   check if we need to be quiet about a command error

**Parameters**

`struct ata_queued_cmd *qc`
:   qc to check

**Description**

> Look at the qc flags anbd its scsi command request flags to determine
> if we need to be quiet about the command failure.

int ata\_eh\_link\_set\_lpm(struct ata\_link \*link, enum ata\_lpm\_policy policy, struct ata\_device \*\*r\_failed\_dev)
:   configure SATA interface power management

**Parameters**

`struct ata_link *link`
:   link to configure

`enum ata_lpm_policy policy`
:   the link power management policy

`struct ata_device **r_failed_dev`
:   out parameter for failed device

**Description**

> Enable SATA Interface power management. This will enable
> Device Interface Power Management (DIPM) for min\_power and
> medium\_power\_with\_dipm policies, and then call driver specific
> callbacks for enabling Host Initiated Power management.
>
> LOCKING:
> EH context.

**Return**

0 on success, -errno on failure.

void ata\_eh\_link\_autopsy(struct ata\_link \*link)
:   analyze error and determine recovery action

**Parameters**

`struct ata_link *link`
:   host link to perform autopsy on

**Description**

> Analyze why **link** failed and determine which recovery actions
> are needed. This function also sets more detailed AC\_ERR\_\*
> values and fills sense data for ATAPI CHECK SENSE.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_eh\_autopsy(struct ata\_port \*ap)
:   analyze error and determine recovery action

**Parameters**

`struct ata_port *ap`
:   host port to perform autopsy on

**Description**

> Analyze all links of **ap** and determine why they failed and
> which recovery actions are needed.
>
> LOCKING:
> Kernel thread context (may sleep).

const char \*ata\_get\_cmd\_name(u8 command)
:   get name for ATA command

**Parameters**

`u8 command`
:   ATA command code to get name for

**Description**

> Return a textual name of the given command or “unknown”
>
> LOCKING:
> None

void ata\_eh\_link\_report(struct ata\_link \*link)
:   report error handling to user

**Parameters**

`struct ata_link *link`
:   ATA link EH is going on

**Description**

> Report EH to user.
>
> LOCKING:
> None.

void ata\_eh\_report(struct ata\_port \*ap)
:   report error handling to user

**Parameters**

`struct ata_port *ap`
:   ATA port to report EH about

**Description**

> Report EH to user.
>
> LOCKING:
> None.

int ata\_eh\_set\_mode(struct ata\_link \*link, struct ata\_device \*\*r\_failed\_dev)
:   Program timings and issue SET FEATURES - XFER

**Parameters**

`struct ata_link *link`
:   link on which timings will be programmed

`struct ata_device **r_failed_dev`
:   out parameter for failed device

**Description**

> Set ATA device disk transfer mode (PIO3, UDMA6, etc.). If
> [`ata_eh_set_mode()`](#c.ata_eh_set_mode "ata_eh_set_mode") fails, pointer to the failing device is
> returned in **r\_failed\_dev**.
>
> LOCKING:
> PCI/etc. bus probe sem.

**Return**

0 on success, negative errno otherwise

int atapi\_eh\_clear\_ua(struct ata\_device \*dev)
:   Clear ATAPI UNIT ATTENTION after reset

**Parameters**

`struct ata_device *dev`
:   ATAPI device to clear UA for

**Description**

> Resets and other operations can make an ATAPI device raise
> UNIT ATTENTION which causes the next operation to fail. This
> function clears UA.
>
> LOCKING:
> EH context (may sleep).

**Return**

0 on success, -errno on failure.

int ata\_eh\_maybe\_retry\_flush(struct ata\_device \*dev)
:   Retry FLUSH if necessary

**Parameters**

`struct ata_device *dev`
:   ATA device which may need FLUSH retry

**Description**

> If **dev** failed FLUSH, it needs to be reported upper layer
> immediately as it means that **dev** failed to remap and already
> lost at least a sector and further FLUSH retrials won’t make
> any difference to the lost sector. However, if FLUSH failed
> for other reasons, for example transmission error, FLUSH needs
> to be retried.
>
> This function determines whether FLUSH failure retry is
> necessary and performs it if so.

**Return**

0 if EH can continue, -errno if EH needs to be repeated.

int ata\_eh\_recover(struct ata\_port \*ap, struct ata\_reset\_operations \*reset\_ops, struct ata\_link \*\*r\_failed\_link)
:   recover host port after error

**Parameters**

`struct ata_port *ap`
:   host port to recover

`struct ata_reset_operations *reset_ops`
:   The set of reset operations to use

`struct ata_link **r_failed_link`
:   out parameter for failed link

**Description**

> This is the alpha and omega, eum and yang, heart and soul of
> libata exception handling. On entry, actions required to
> recover each link and hotplug requests are recorded in the
> link’s eh\_context. This function executes all the operations
> with appropriate retrials and fallbacks to resurrect failed
> devices, detach goners and greet newcomers.
>
> LOCKING:
> Kernel thread context (may sleep).

**Return**

0 on success, -errno on failure.

void ata\_eh\_finish(struct ata\_port \*ap)
:   finish up EH

**Parameters**

`struct ata_port *ap`
:   host port to finish EH for

**Description**

> Recovery is complete. Clean up EH states and retry or finish
> failed qcs.
>
> LOCKING:
> None.

void ata\_std\_error\_handler(struct ata\_port \*ap)
:   standard error handler

**Parameters**

`struct ata_port *ap`
:   host port to handle error for

**Description**

> Perform standard error handling sequence.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_eh\_handle\_port\_suspend(struct ata\_port \*ap)
:   perform port suspend operation

**Parameters**

`struct ata_port *ap`
:   port to suspend

**Description**

> Suspend **ap**.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_eh\_handle\_port\_resume(struct ata\_port \*ap)
:   perform port resume operation

**Parameters**

`struct ata_port *ap`
:   port to resume

**Description**

> Resume **ap**.
>
> LOCKING:
> Kernel thread context (may sleep).

## libata SCSI translation/emulation

int ata\_std\_bios\_param(struct scsi\_device \*sdev, struct gendisk \*unused, sector\_t capacity, int geom[])
:   generic bios head/sector/cylinder calculator used by sd.

**Parameters**

`struct scsi_device *sdev`
:   SCSI device for which BIOS geometry is to be determined

`struct gendisk *unused`
:   gendisk associated with **sdev**

`sector_t capacity`
:   capacity of SCSI device

`int geom[]`
:   location to which geometry will be output

**Description**

> Generic bios head/sector/cylinder calculator
> used by sd. Most BIOSes nowadays expect a XXX/255/16 (CHS)
> mapping. Some situations may arise where the disk is not
> bootable if this is not used.
>
> LOCKING:
> Defined by the SCSI layer. We don’t really care.

**Return**

Zero.

void ata\_scsi\_unlock\_native\_capacity(struct scsi\_device \*sdev)
:   unlock native capacity

**Parameters**

`struct scsi_device *sdev`
:   SCSI device to adjust device capacity for

**Description**

> This function is called if a partition on **sdev** extends beyond
> the end of the device. It requests EH to unlock HPA.
>
> LOCKING:
> Defined by the SCSI layer. Might sleep.

bool ata\_scsi\_dma\_need\_drain(struct request \*rq)
:   Check whether data transfer may overflow

**Parameters**

`struct request *rq`
:   request to be checked

**Description**

> ATAPI commands which transfer variable length data to host
> might overflow due to application error or hardware bug. This
> function checks whether overflow should be drained and ignored
> for **request**.
>
> LOCKING:
> None.

**Return**

1 if ; otherwise, 0.

int ata\_scsi\_sdev\_init(struct scsi\_device \*sdev)
:   Early setup of SCSI device

**Parameters**

`struct scsi_device *sdev`
:   SCSI device to examine

**Description**

> This is called from `scsi_alloc_sdev()` when the scsi device
> associated with an ATA device is scanned on a port.
>
> LOCKING:
> Defined by SCSI layer. We don’t really care.

int ata\_scsi\_sdev\_configure(struct scsi\_device \*sdev, struct queue\_limits \*lim)
:   Set SCSI device attributes

**Parameters**

`struct scsi_device *sdev`
:   SCSI device to examine

`struct queue_limits *lim`
:   queue limits

**Description**

> This is called before we actually start reading
> and writing to the device, to configure certain
> SCSI mid-layer behaviors.
>
> LOCKING:
> Defined by SCSI layer. We don’t really care.

void ata\_scsi\_sdev\_destroy(struct scsi\_device \*sdev)
:   SCSI device is about to be destroyed

**Parameters**

`struct scsi_device *sdev`
:   SCSI device to be destroyed

**Description**

> **sdev** is about to be destroyed for hot/warm unplugging. If
> this unplugging was initiated by libata as indicated by NULL
> dev->sdev, this function doesn’t have to do anything.
> Otherwise, SCSI layer initiated warm-unplug is in progress.
> Clear dev->sdev, schedule the device for ATA detach and invoke
> EH.
>
> LOCKING:
> Defined by SCSI layer. We don’t really care.

enum scsi\_qc\_status ata\_scsi\_queuecmd(struct Scsi\_Host \*shost, struct scsi\_cmnd \*cmd)
:   Issue SCSI cdb to libata-managed device

**Parameters**

`struct Scsi_Host *shost`
:   SCSI host of command to be sent

`struct scsi_cmnd *cmd`
:   SCSI command to be sent

**Description**

> In some cases, this function translates SCSI commands into
> ATA taskfiles, and queues the taskfiles to be sent to
> hardware. In other cases, this function simulates a
> SCSI device by evaluating and responding to certain
> SCSI commands. This creates the overall effect of
> ATA and ATAPI devices appearing as SCSI devices.
>
> LOCKING:
> ATA host lock

**Return**

Return value from `__ata_scsi_queuecmd()` if **cmd** can be queued,
0 otherwise.

void ata\_scsi\_set\_passthru\_sense\_fields(struct ata\_queued\_cmd \*qc)
:   Set ATA fields in sense buffer

**Parameters**

`struct ata_queued_cmd *qc`
:   ATA PASS-THROUGH command.

**Description**

> Populates “ATA Status Return sense data descriptor” / “Fixed format
> sense data” with ATA taskfile fields.
>
> LOCKING:
> None.

int ata\_get\_identity(struct ata\_port \*ap, struct scsi\_device \*sdev, void \_\_user \*arg)
:   Handler for HDIO\_GET\_IDENTITY ioctl

**Parameters**

`struct ata_port *ap`
:   target port

`struct scsi_device *sdev`
:   SCSI device to get identify data for

`void __user *arg`
:   User buffer area for identify data

**Description**

> LOCKING:
> Defined by the SCSI layer. We don’t really care.

**Return**

Zero on success, negative errno on error.

int ata\_cmd\_ioctl(struct scsi\_device \*scsidev, void \_\_user \*arg)
:   Handler for HDIO\_DRIVE\_CMD ioctl

**Parameters**

`struct scsi_device *scsidev`
:   Device to which we are issuing command

`void __user *arg`
:   User provided data for issuing command

**Description**

> LOCKING:
> Defined by the SCSI layer. We don’t really care.

**Return**

Zero on success, negative errno on error.

int ata\_task\_ioctl(struct scsi\_device \*scsidev, void \_\_user \*arg)
:   Handler for HDIO\_DRIVE\_TASK ioctl

**Parameters**

`struct scsi_device *scsidev`
:   Device to which we are issuing command

`void __user *arg`
:   User provided data for issuing command

**Description**

> LOCKING:
> Defined by the SCSI layer. We don’t really care.

**Return**

Zero on success, negative errno on error.

struct ata\_queued\_cmd \*ata\_scsi\_qc\_new(struct ata\_device \*dev, struct scsi\_cmnd \*cmd)
:   acquire new ata\_queued\_cmd reference

**Parameters**

`struct ata_device *dev`
:   ATA device to which the new command is attached

`struct scsi_cmnd *cmd`
:   SCSI command that originated this ATA command

**Description**

> Obtain a reference to an unused ata\_queued\_cmd structure,
> which is the basic libata structure representing a single
> ATA command sent to the hardware.
>
> If a command was available, fill in the SCSI-specific
> portions of the structure with information on the
> current command.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Command allocated, or `NULL` if none available.

void ata\_to\_sense\_error(u8 drv\_stat, u8 drv\_err, u8 \*sk, u8 \*asc, u8 \*ascq)
:   convert ATA error to SCSI error

**Parameters**

`u8 drv_stat`
:   value contained in ATA status register

`u8 drv_err`
:   value contained in ATA error register

`u8 *sk`
:   the sense key we’ll fill out

`u8 *asc`
:   the additional sense code we’ll fill out

`u8 *ascq`
:   the additional sense code qualifier we’ll fill out

**Description**

> Converts an ATA error into a SCSI error. Fill out pointers to
> SK, ASC, and ASCQ bytes for later use in fixed or descriptor
> format sense blocks.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_gen\_ata\_sense(struct ata\_queued\_cmd \*qc)
:   generate a SCSI fixed sense block

**Parameters**

`struct ata_queued_cmd *qc`
:   Command that we are erroring out

**Description**

> Generate sense block for a failed ATA command **qc**.
>
> LOCKING:
> None.

unsigned int ata\_scsi\_start\_stop\_xlat(struct ata\_queued\_cmd \*qc)
:   Translate SCSI START STOP UNIT command

**Parameters**

`struct ata_queued_cmd *qc`
:   Storage for translated ATA taskfile

**Description**

> Sets up an ATA taskfile to issue STANDBY (to stop) or READ VERIFY
> (to start). Perhaps these commands should be preceded by
> CHECK POWER MODE to see what power mode the device is already in.
> [See SAT revision 5 at www.t10.org]
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Zero on success, non-zero on error.

unsigned int ata\_scsi\_flush\_xlat(struct ata\_queued\_cmd \*qc)
:   Translate SCSI SYNCHRONIZE CACHE command

**Parameters**

`struct ata_queued_cmd *qc`
:   Storage for translated ATA taskfile

**Description**

> Sets up an ATA taskfile to issue FLUSH CACHE or
> FLUSH CACHE EXT.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Zero on success, non-zero on error.

void scsi\_6\_lba\_len(const u8 \*cdb, u64 \*plba, u32 \*plen)
:   Get LBA and transfer length

**Parameters**

`const u8 *cdb`
:   SCSI command to translate

`u64 *plba`
:   the LBA

`u32 *plen`
:   the transfer length

**Description**

> Calculate LBA and transfer length for 6-byte commands.

void scsi\_10\_lba\_len(const u8 \*cdb, u64 \*plba, u32 \*plen)
:   Get LBA and transfer length

**Parameters**

`const u8 *cdb`
:   SCSI command to translate

`u64 *plba`
:   the LBA

`u32 *plen`
:   the transfer length

**Description**

> Calculate LBA and transfer length for 10-byte commands.

void scsi\_16\_lba\_len(const u8 \*cdb, u64 \*plba, u32 \*plen)
:   Get LBA and transfer length

**Parameters**

`const u8 *cdb`
:   SCSI command to translate

`u64 *plba`
:   the LBA

`u32 *plen`
:   the transfer length

**Description**

> Calculate LBA and transfer length for 16-byte commands.

int scsi\_dld(const u8 \*cdb)
:   Get duration limit descriptor index

**Parameters**

`const u8 *cdb`
:   SCSI command to translate

**Description**

> Returns the dld bits indicating the index of a command duration limit
> descriptor.

unsigned int ata\_scsi\_verify\_xlat(struct ata\_queued\_cmd \*qc)
:   Translate SCSI VERIFY command into an ATA one

**Parameters**

`struct ata_queued_cmd *qc`
:   Storage for translated ATA taskfile

**Description**

> Converts SCSI VERIFY command to an ATA READ VERIFY command.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Zero on success, non-zero on error.

unsigned int ata\_scsi\_rw\_xlat(struct ata\_queued\_cmd \*qc)
:   Translate SCSI r/w command into an ATA one

**Parameters**

`struct ata_queued_cmd *qc`
:   Storage for translated ATA taskfile

**Description**

> Converts any of six SCSI read/write commands into the
> ATA counterpart, including starting sector (LBA),
> sector count, and taking into account the device’s LBA48
> support.
>
> Commands `READ_6`, `READ_10`, `READ_16`, `WRITE_6`, `WRITE_10`, and
> `WRITE_16` are currently supported.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Zero on success, non-zero on error.

int ata\_scsi\_translate(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, ata\_xlat\_func\_t xlat\_func)
:   Translate then issue SCSI command to ATA device

**Parameters**

`struct ata_device *dev`
:   ATA device to which the command is addressed

`struct scsi_cmnd *cmd`
:   SCSI command to execute

`ata_xlat_func_t xlat_func`
:   Actor which translates **cmd** to an ATA taskfile

**Description**

> Our ->`queuecommand()` function has decided that the SCSI
> command issued can be directly translated into an ATA
> command, rather than handled internally.
>
> This function sets up an ata\_queued\_cmd structure for the
> SCSI command, and sends that ata\_queued\_cmd to the hardware.
>
> The xlat\_func argument (actor) returns 0 if ready to execute
> ATA command, else 1 to finish translation. If 1 is returned
> then cmd->result (and possibly cmd->sense\_buffer) are assumed
> to be set reflecting an error condition or clean (early)
> termination.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

0 on success, SCSI\_ML\_QUEUE\_DEVICE\_BUSY or SCSI\_MLQUEUE\_HOST\_BUSY if the
command needs to be deferred.

void ata\_scsi\_rbuf\_fill(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, unsigned int (\*actor)(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf))
:   wrapper for SCSI command simulators

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`unsigned int (*actor)(struct ata_device *dev, struct scsi_cmnd *cmd, u8 *rbuf)`
:   Callback hook for desired SCSI command simulator

**Description**

> Takes care of the hard work of simulating a SCSI command...
> Mapping the response buffer, calling the command’s handler,
> and handling the handler’s return value. This return value
> indicates whether the handler wishes the SCSI command to be
> completed successfully (0), or not (in which case cmd->result
> and sense buffer are assumed to be set).
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_std(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate standard INQUIRY command

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Returns standard device identification data associated
> with non-VPD INQUIRY command output.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_00(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page 0, list of pages

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Returns list of inquiry VPD pages available.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_80(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page 80, device serial number

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Returns ATA device serial number.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_83(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page 83, device identity

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Yields two logical unit device identification designators:
> :   * vendor specific ASCII containing the ATA serial number
>     * SAT defined “t10 vendor id based” containing ASCII vendor
>       name (“ATA “), model and serial numbers.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_89(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page 89, ATA info

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Yields SAT-specified ATA VPD page.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_b0(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page B0, Block Limits

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Return data for the VPD page B0h (Block Limits).
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_b1(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page B1, Block Device Characteristics

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Return data for the VPD page B1h (Block Device Characteristics).
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_b2(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page B2, Logical Block Provisioning

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Return data for the VPD page B2h (Logical Block Provisioning).
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_b6(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page B6, Zoned Block Device Characteristics

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Return data for the VPD page B2h (Zoned Block Device Characteristics).
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inq\_b9(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY VPD page B9, Concurrent Positioning Ranges

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Return data for the VPD page B9h (Concurrent Positioning Ranges).
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_inquiry(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate INQUIRY command

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Returns data associated with an INQUIRY command output.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void modecpy(u8 \*dest, const u8 \*src, int n, bool changeable)
:   Prepare response for MODE SENSE

**Parameters**

`u8 *dest`
:   output buffer

`const u8 *src`
:   data being copied

`int n`
:   length of mode page

`bool changeable`
:   whether changeable parameters are requested

**Description**

> Generate a generic MODE SENSE page for either current or changeable
> parameters.
>
> LOCKING:
> None.

unsigned int ata\_msense\_caching(u16 \*id, u8 \*buf, bool changeable)
:   Simulate MODE SENSE caching info page

**Parameters**

`u16 *id`
:   device IDENTIFY data

`u8 *buf`
:   output buffer

`bool changeable`
:   whether changeable parameters are requested

**Description**

> Generate a caching info page, which conditionally indicates
> write caching to the SCSI layer, depending on device
> capabilities.
>
> LOCKING:
> None.

unsigned int ata\_msense\_control(struct ata\_device \*dev, u8 \*buf, u8 spg, bool changeable)
:   Simulate MODE SENSE control mode page

**Parameters**

`struct ata_device *dev`
:   ATA device of interest

`u8 *buf`
:   output buffer

`u8 spg`
:   sub-page code

`bool changeable`
:   whether changeable parameters are requested

**Description**

> Generate a generic MODE SENSE control mode page.
>
> LOCKING:
> None.

unsigned int ata\_msense\_rw\_recovery(u8 \*buf, bool changeable)
:   Simulate MODE SENSE r/w error recovery page

**Parameters**

`u8 *buf`
:   output buffer

`bool changeable`
:   whether changeable parameters are requested

**Description**

> Generate a generic MODE SENSE r/w error recovery page.
>
> LOCKING:
> None.

unsigned int ata\_scsiop\_mode\_sense(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate MODE SENSE 6, 10 commands

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Simulate MODE SENSE commands. Assume this is invoked for direct
> access devices (e.g. disks) only. There should be no block
> descriptor for other device types.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsiop\_read\_cap(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate READ CAPACITY[ 16] commands

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Simulate READ CAPACITY commands.
>
> LOCKING:
> None.

unsigned int ata\_scsiop\_report\_luns(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate REPORT LUNS command

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Simulate REPORT LUNS command.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int atapi\_xlat(struct ata\_queued\_cmd \*qc)
:   Initialize PACKET taskfile

**Parameters**

`struct ata_queued_cmd *qc`
:   command structure to be initialized

**Description**

> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Zero on success, non-zero on failure.

struct ata\_device \*ata\_scsi\_find\_dev(struct ata\_port \*ap, const struct scsi\_device \*scsidev)
:   lookup ata\_device from scsi\_cmnd

**Parameters**

`struct ata_port *ap`
:   ATA port to which the device is attached

`const struct scsi_device *scsidev`
:   SCSI device from which we derive the ATA device

**Description**

> Given various information provided in `struct scsi_cmnd`,
> map that onto an ATA bus, and using that mapping
> determine which ata\_device is associated with the
> SCSI command to be sent.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

Associated ATA device, or `NULL` if not found.

unsigned int ata\_scsi\_pass\_thru(struct ata\_queued\_cmd \*qc)
:   convert ATA pass-thru CDB to taskfile

**Parameters**

`struct ata_queued_cmd *qc`
:   command structure to be initialized

**Description**

> Handles either 12, 16, or 32-byte versions of the CDB.

**Return**

Zero on success, non-zero on failure.

size\_t ata\_format\_dsm\_trim\_descr(struct scsi\_cmnd \*cmd, u32 trmax, u64 sector, u32 count)
:   SATL Write Same to DSM Trim

**Parameters**

`struct scsi_cmnd *cmd`
:   SCSI command being translated

`u32 trmax`
:   Maximum number of entries that will fit in sector\_size bytes.

`u64 sector`
:   Starting sector

`u32 count`
:   Total Range of request in logical sectors

**Description**

Rewrite the WRITE SAME descriptor to be a DSM TRIM little-endian formatted
descriptor.

Upto 64 entries of the format:
:   > 63:48 Range Length
    > 47:0 LBA

    Range Length of 0 is ignored.
    LBA’s should be sorted order and not overlap.

**NOTE**

this is the same format as ADD LBA(S) TO NV CACHE PINNED SET

**Return**

Number of bytes copied into sglist.

unsigned int ata\_scsi\_write\_same\_xlat(struct ata\_queued\_cmd \*qc)
:   SATL Write Same to ATA SCT Write Same

**Parameters**

`struct ata_queued_cmd *qc`
:   Command to be translated

**Description**

Translate a SCSI WRITE SAME command to be either a DSM TRIM command or
an SCT Write Same command.
Based on WRITE SAME has the UNMAP flag:

> * When set translate to DSM TRIM
> * When clear translate to SCT Write Same

unsigned int ata\_scsiop\_maint\_in(struct ata\_device \*dev, struct scsi\_cmnd \*cmd, u8 \*rbuf)
:   Simulate a subset of MAINTENANCE\_IN

**Parameters**

`struct ata_device *dev`
:   Target device.

`struct scsi_cmnd *cmd`
:   SCSI command of interest.

`u8 *rbuf`
:   Response buffer, to which simulated SCSI cmd output is sent.

**Description**

> Yields a subset to satisfy [`scsi_report_opcode()`](scsi.html#c.scsi_report_opcode "scsi_report_opcode")
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_scsi\_report\_zones\_complete(struct ata\_queued\_cmd \*qc)
:   convert ATA output

**Parameters**

`struct ata_queued_cmd *qc`
:   command structure returning the data

**Description**

> Convert T-13 little-endian field representation into
> T-10 big-endian field representation.
> What a mess.

int ata\_mselect\_caching(struct ata\_queued\_cmd \*qc, const u8 \*buf, int len, u16 \*fp)
:   Simulate MODE SELECT for caching info page

**Parameters**

`struct ata_queued_cmd *qc`
:   Storage for translated ATA taskfile

`const u8 *buf`
:   input buffer

`int len`
:   number of valid bytes in the input buffer

`u16 *fp`
:   out parameter for the failed field on error

**Description**

> Prepare a taskfile to modify caching information for the device.
>
> LOCKING:
> None.

int ata\_mselect\_control(struct ata\_queued\_cmd \*qc, u8 spg, const u8 \*buf, int len, u16 \*fp)
:   Simulate MODE SELECT for control page

**Parameters**

`struct ata_queued_cmd *qc`
:   Storage for translated ATA taskfile

`u8 spg`
:   target sub-page of the control page

`const u8 *buf`
:   input buffer

`int len`
:   number of valid bytes in the input buffer

`u16 *fp`
:   out parameter for the failed field on error

**Description**

> Prepare a taskfile to modify caching information for the device.
>
> LOCKING:
> None.

unsigned int ata\_scsi\_mode\_select\_xlat(struct ata\_queued\_cmd \*qc)
:   Simulate MODE SELECT 6, 10 commands

**Parameters**

`struct ata_queued_cmd *qc`
:   Storage for translated ATA taskfile

**Description**

> Converts a MODE SELECT command to an ATA SET FEATURES taskfile.
> Assume this is invoked for direct access devices (e.g. disks) only.
> There should be no block descriptor for other device types.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

unsigned int ata\_scsi\_var\_len\_cdb\_xlat(struct ata\_queued\_cmd \*qc)
:   SATL variable length CDB to Handler

**Parameters**

`struct ata_queued_cmd *qc`
:   Command to be translated

**Description**

> Translate a SCSI variable length CDB to specified commands.
> It checks a service action value in CDB to call corresponding handler.

**Return**

Zero on success, non-zero on failure

ata\_xlat\_func\_t ata\_get\_xlat\_func(struct ata\_device \*dev, u8 cmd)
:   check if SCSI to ATA translation is possible

**Parameters**

`struct ata_device *dev`
:   ATA device

`u8 cmd`
:   SCSI command opcode to consider

**Description**

> Look up the SCSI command given, and determine whether the
> SCSI command is to be translated or simulated.

**Return**

Pointer to translation function if possible, `NULL` if not.

void ata\_scsi\_simulate(struct ata\_device \*dev, struct scsi\_cmnd \*cmd)
:   simulate SCSI command on ATA device

**Parameters**

`struct ata_device *dev`
:   the target device

`struct scsi_cmnd *cmd`
:   SCSI command being sent to device.

**Description**

> Interprets and directly executes a select list of SCSI commands
> that can be handled internally.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

bool ata\_scsi\_offline\_dev(struct ata\_device \*dev)
:   offline attached SCSI device

**Parameters**

`struct ata_device *dev`
:   ATA device to offline attached SCSI device for

**Description**

> This function is called from [`ata_eh_detach_dev()`](#c.ata_eh_detach_dev "ata_eh_detach_dev") and is responsible for
> taking the SCSI device attached to **dev** offline. This function is
> called with host lock which protects dev->sdev against clearing.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

**Return**

true if attached SCSI device exists, false otherwise.

void ata\_scsi\_remove\_dev(struct ata\_device \*dev)
:   remove attached SCSI device

**Parameters**

`struct ata_device *dev`
:   ATA device to remove attached SCSI device for

**Description**

> This function is called from `ata_eh_scsi_hotplug()` and
> responsible for removing the SCSI device attached to **dev**.
>
> LOCKING:
> Kernel thread context (may sleep).

void ata\_scsi\_media\_change\_notify(struct ata\_device \*dev)
:   send media change event

**Parameters**

`struct ata_device *dev`
:   Pointer to the disk device with media change event

**Description**

> Tell the block layer to send a media change notification
> event.
>
> LOCKING:
> spin\_lock\_irqsave(host lock)

void ata\_scsi\_hotplug(struct work\_struct \*work)
:   SCSI part of hotplug

**Parameters**

`struct work_struct *work`
:   Pointer to ATA port to perform SCSI hotplug on

**Description**

> Perform SCSI part of hotplug. It’s executed from a separate
> workqueue after EH completes. This is necessary because SCSI
> hot plugging requires working EH and hot unplugging is
> synchronized with hot plugging with a mutex.
>
> LOCKING:
> Kernel thread context (may sleep).

int ata\_scsi\_user\_scan(struct Scsi\_Host \*shost, unsigned int channel, unsigned int id, u64 lun)
:   indication for user-initiated bus scan

**Parameters**

`struct Scsi_Host *shost`
:   SCSI host to scan

`unsigned int channel`
:   Channel to scan

`unsigned int id`
:   ID to scan

`u64 lun`
:   LUN to scan

**Description**

> This function is called when user explicitly requests bus
> scan. Set probe pending flag and invoke EH.
>
> LOCKING:
> SCSI layer (we don’t care)

**Return**

Zero.

void ata\_scsi\_dev\_rescan(struct work\_struct \*work)
:   initiate `scsi_rescan_device()`

**Parameters**

`struct work_struct *work`
:   Pointer to ATA port to perform `scsi_rescan_device()`

**Description**

> After ATA pass thru (SAT) commands are executed successfully,
> libata need to propagate the changes to SCSI layer.
>
> LOCKING:
> Kernel thread context (may sleep).

## ATA errors and exceptions

This chapter tries to identify what error/exception conditions exist for
ATA/ATAPI devices and describe how they should be handled in
implementation-neutral way.

The term ‘error’ is used to describe conditions where either an explicit
error condition is reported from device or a command has timed out.

The term ‘exception’ is either used to describe exceptional conditions
which are not errors (say, power or hotplug events), or to describe both
errors and non-error exceptional conditions. Where explicit distinction
between error and exception is necessary, the term ‘non-error exception’
is used.

### Exception categories

Exceptions are described primarily with respect to legacy taskfile + bus
master IDE interface. If a controller provides other better mechanism
for error reporting, mapping those into categories described below
shouldn’t be difficult.

In the following sections, two recovery actions - reset and
reconfiguring transport - are mentioned. These are described further in
[EH recovery actions](#exrec).

#### HSM violation

This error is indicated when STATUS value doesn’t match HSM requirement
during issuing or execution any ATA/ATAPI command.

* ATA\_STATUS doesn’t contain !BSY && DRDY && !DRQ while trying to
  issue a command.
* !BSY && !DRQ during PIO data transfer.
* DRQ on command completion.
* !BSY && ERR after CDB transfer starts but before the last byte of CDB
  is transferred. ATA/ATAPI standard states that “The device shall not
  terminate the PACKET command with an error before the last byte of
  the command packet has been written” in the error outputs description
  of PACKET command and the state diagram doesn’t include such
  transitions.

In these cases, HSM is violated and not much information regarding the
error can be acquired from STATUS or ERROR register. IOW, this error can
be anything - driver bug, faulty device, controller and/or cable.

As HSM is violated, reset is necessary to restore known state.
Reconfiguring transport for lower speed might be helpful too as
transmission errors sometimes cause this kind of errors.

#### ATA/ATAPI device error (non-NCQ / non-CHECK CONDITION)

These are errors detected and reported by ATA/ATAPI devices indicating
device problems. For this type of errors, STATUS and ERROR register
values are valid and describe error condition. Note that some of ATA bus
errors are detected by ATA/ATAPI devices and reported using the same
mechanism as device errors. Those cases are described later in this
section.

For ATA commands, this type of errors are indicated by !BSY && ERR
during command execution and on completion.

For ATAPI commands,

* !BSY && ERR && ABRT right after issuing PACKET indicates that PACKET
  command is not supported and falls in this category.
* !BSY && ERR(==CHK) && !ABRT after the last byte of CDB is transferred
  indicates CHECK CONDITION and doesn’t fall in this category.
* !BSY && ERR(==CHK) && ABRT after the last byte of CDB is transferred
  \*probably\* indicates CHECK CONDITION and doesn’t fall in this
  category.

Of errors detected as above, the following are not ATA/ATAPI device
errors but ATA bus errors and should be handled according to
[ATA bus error](#excatATAbusErr).

CRC error during data transfer
:   This is indicated by ICRC bit in the ERROR register and means that
    corruption occurred during data transfer. Up to ATA/ATAPI-7, the
    standard specifies that this bit is only applicable to UDMA
    transfers but ATA/ATAPI-8 draft revision 1f says that the bit may be
    applicable to multiword DMA and PIO.

ABRT error during data transfer or on completion
:   Up to ATA/ATAPI-7, the standard specifies that ABRT could be set on
    ICRC errors and on cases where a device is not able to complete a
    command. Combined with the fact that MWDMA and PIO transfer errors
    aren’t allowed to use ICRC bit up to ATA/ATAPI-7, it seems to imply
    that ABRT bit alone could indicate transfer errors.

    However, ATA/ATAPI-8 draft revision 1f removes the part that ICRC
    errors can turn on ABRT. So, this is kind of gray area. Some
    heuristics are needed here.

ATA/ATAPI device errors can be further categorized as follows.

Media errors
:   This is indicated by UNC bit in the ERROR register. ATA devices
    reports UNC error only after certain number of retries cannot
    recover the data, so there’s nothing much else to do other than
    notifying upper layer.

    READ and WRITE commands report CHS or LBA of the first failed sector
    but ATA/ATAPI standard specifies that the amount of transferred data
    on error completion is indeterminate, so we cannot assume that
    sectors preceding the failed sector have been transferred and thus
    cannot complete those sectors successfully as SCSI does.

Media changed / media change requested error
:   <<TODO: fill here>>

Address error
:   This is indicated by IDNF bit in the ERROR register. Report to upper
    layer.

Other errors
:   This can be invalid command or parameter indicated by ABRT ERROR bit
    or some other error condition. Note that ABRT bit can indicate a lot
    of things including ICRC and Address errors. Heuristics needed.

Depending on commands, not all STATUS/ERROR bits are applicable. These
non-applicable bits are marked with “na” in the output descriptions but
up to ATA/ATAPI-7 no definition of “na” can be found. However,
ATA/ATAPI-8 draft revision 1f describes “N/A” as follows.

> 3.2.3.3a N/A
> :   A keyword the indicates a field has no defined value in this
>     standard and should not be checked by the host or device. N/A
>     fields should be cleared to zero.

So, it seems reasonable to assume that “na” bits are cleared to zero by
devices and thus need no explicit masking.

#### ATAPI device CHECK CONDITION

ATAPI device CHECK CONDITION error is indicated by set CHK bit (ERR bit)
in the STATUS register after the last byte of CDB is transferred for a
PACKET command. For this kind of errors, sense data should be acquired
to gather information regarding the errors. REQUEST SENSE packet command
should be used to acquire sense data.

Once sense data is acquired, this type of errors can be handled
similarly to other SCSI errors. Note that sense data may indicate ATA
bus error (e.g. Sense Key 04h HARDWARE ERROR && ASC/ASCQ 47h/00h SCSI
PARITY ERROR). In such cases, the error should be considered as an ATA
bus error and handled according to [ATA bus error](#excatATAbusErr).

#### ATA device error (NCQ)

NCQ command error is indicated by cleared BSY and set ERR bit during NCQ
command phase (one or more NCQ commands outstanding). Although STATUS
and ERROR registers will contain valid values describing the error, READ
LOG EXT is required to clear the error condition, determine which
command has failed and acquire more information.

READ LOG EXT Log Page 10h reports which tag has failed and taskfile
register values describing the error. With this information the failed
command can be handled as a normal ATA command error as in
[ATA/ATAPI device error (non-NCQ / non-CHECK CONDITION)](#excatDevErr)
and all other in-flight commands must be retried. Note that this retry
should not be counted - it’s likely that commands retried this way would
have completed normally if it were not for the failed command.

Note that ATA bus errors can be reported as ATA device NCQ errors. This
should be handled as described in [ATA bus error](#excatATAbusErr).

If READ LOG EXT Log Page 10h fails or reports NQ, we’re thoroughly
screwed. This condition should be treated according to
[HSM violation](#excatHSMviolation).

#### ATA bus error

ATA bus error means that data corruption occurred during transmission
over ATA bus (SATA or PATA). This type of errors can be indicated by

* ICRC or ABRT error as described in
  [ATA/ATAPI device error (non-NCQ / non-CHECK CONDITION)](#excatDevErr).
* Controller-specific error completion with error information
  indicating transmission error.
* On some controllers, command timeout. In this case, there may be a
  mechanism to determine that the timeout is due to transmission error.
* Unknown/random errors, timeouts and all sorts of weirdities.

As described above, transmission errors can cause wide variety of
symptoms ranging from device ICRC error to random device lockup, and,
for many cases, there is no way to tell if an error condition is due to
transmission error or not; therefore, it’s necessary to employ some kind
of heuristic when dealing with errors and timeouts. For example,
encountering repetitive ABRT errors for known supported command is
likely to indicate ATA bus error.

Once it’s determined that ATA bus errors have possibly occurred,
lowering ATA bus transmission speed is one of actions which may
alleviate the problem. See [Reconfigure transport](#exrecReconf) for
more information.

#### PCI bus error

Data corruption or other failures during transmission over PCI (or other
system bus). For standard BMDMA, this is indicated by Error bit in the
BMDMA Status register. This type of errors must be logged as it
indicates something is very wrong with the system. Resetting host
controller is recommended.

#### Late completion

This occurs when timeout occurs and the timeout handler finds out that
the timed out command has completed successfully or with error. This is
usually caused by lost interrupts. This type of errors must be logged.
Resetting host controller is recommended.

#### Unknown error (timeout)

This is when timeout occurs and the command is still processing or the
host and device are in unknown state. When this occurs, HSM could be in
any valid or invalid state. To bring the device to known state and make
it forget about the timed out command, resetting is necessary. The timed
out command may be retried.

Timeouts can also be caused by transmission errors. Refer to
[ATA bus error](#excatATAbusErr) for more details.

#### Hotplug and power management exceptions

<<TODO: fill here>>

### EH recovery actions

This section discusses several important recovery actions.

#### Clearing error condition

Many controllers require its error registers to be cleared by error
handler. Different controllers may have different requirements.

For SATA, it’s strongly recommended to clear at least SError register
during error handling.

#### Reset

During EH, resetting is necessary in the following cases.

* HSM is in unknown or invalid state
* HBA is in unknown or invalid state
* EH needs to make HBA/device forget about in-flight commands
* HBA/device behaves weirdly

Resetting during EH might be a good idea regardless of error condition
to improve EH robustness. Whether to reset both or either one of HBA and
device depends on situation but the following scheme is recommended.

* When it’s known that HBA is in ready state but ATA/ATAPI device is in
  unknown state, reset only device.
* If HBA is in unknown state, reset both HBA and device.

HBA resetting is implementation specific. For a controller complying to
taskfile/BMDMA PCI IDE, stopping active DMA transaction may be
sufficient iff BMDMA state is the only HBA context. But even mostly
taskfile/BMDMA PCI IDE complying controllers may have implementation
specific requirements and mechanism to reset themselves. This must be
addressed by specific drivers.

OTOH, ATA/ATAPI standard describes in detail ways to reset ATA/ATAPI
devices.

PATA hardware reset
:   This is hardware initiated device reset signalled with asserted PATA
    RESET- signal. There is no standard way to initiate hardware reset
    from software although some hardware provides registers that allow
    driver to directly tweak the RESET- signal.

Software reset
:   This is achieved by turning CONTROL SRST bit on for at least 5us.
    Both PATA and SATA support it but, in case of SATA, this may require
    controller-specific support as the second Register FIS to clear SRST
    should be transmitted while BSY bit is still set. Note that on PATA,
    this resets both master and slave devices on a channel.

EXECUTE DEVICE DIAGNOSTIC command
:   Although ATA/ATAPI standard doesn’t describe exactly, EDD implies
    some level of resetting, possibly similar level with software reset.
    Host-side EDD protocol can be handled with normal command processing
    and most SATA controllers should be able to handle EDD’s just like
    other commands. As in software reset, EDD affects both devices on a
    PATA bus.

    Although EDD does reset devices, this doesn’t suit error handling as
    EDD cannot be issued while BSY is set and it’s unclear how it will
    act when device is in unknown/weird state.

ATAPI DEVICE RESET command
:   This is very similar to software reset except that reset can be
    restricted to the selected device without affecting the other device
    sharing the cable.

SATA phy reset
:   This is the preferred way of resetting a SATA device. In effect,
    it’s identical to PATA hardware reset. Note that this can be done
    with the standard SCR Control register. As such, it’s usually easier
    to implement than software reset.

One more thing to consider when resetting devices is that resetting
clears certain configuration parameters and they need to be set to their
previous or newly adjusted values after reset.

Parameters affected are.

* CHS set up with INITIALIZE DEVICE PARAMETERS (seldom used)
* Parameters set with SET FEATURES including transfer mode setting
* Block count set with SET MULTIPLE MODE
* Other parameters (SET MAX, MEDIA LOCK...)

ATA/ATAPI standard specifies that some parameters must be maintained
across hardware or software reset, but doesn’t strictly specify all of
them. Always reconfiguring needed parameters after reset is required for
robustness. Note that this also applies when resuming from deep sleep
(power-off).

Also, ATA/ATAPI standard requires that IDENTIFY DEVICE / IDENTIFY PACKET
DEVICE is issued after any configuration parameter is updated or a
hardware reset and the result used for further operation. OS driver is
required to implement revalidation mechanism to support this.

#### Reconfigure transport

For both PATA and SATA, a lot of corners are cut for cheap connectors,
cables or controllers and it’s quite common to see high transmission
error rate. This can be mitigated by lowering transmission speed.

The following is a possible scheme Jeff Garzik suggested.

> If more than $N (3?) transmission errors happen in 15 minutes,
>
> * if SATA, decrease SATA PHY speed. if speed cannot be decreased,
> * decrease UDMA xfer speed. if at UDMA0, switch to PIO4,
> * decrease PIO xfer speed. if at PIO3, complain, but continue

## ata\_piix Internals

int ich\_pata\_cable\_detect(struct ata\_port \*ap)
:   Probe host controller cable detect info

**Parameters**

`struct ata_port *ap`
:   Port for which cable detect info is desired

**Description**

> Read 80c cable indicator from ATA PCI device’s PCI config
> register. This register is normally set by firmware (BIOS).
>
> LOCKING:
> None (inherited from caller).

int piix\_pata\_prereset(struct ata\_link \*link, unsigned long deadline)
:   prereset for PATA host controller

**Parameters**

`struct ata_link *link`
:   Target link

`unsigned long deadline`
:   deadline jiffies for the operation

**Description**

> LOCKING:
> None (inherited from caller).

void piix\_set\_piomode(struct ata\_port \*ap, struct ata\_device \*adev)
:   Initialize host controller PATA PIO timings

**Parameters**

`struct ata_port *ap`
:   Port whose timings we are configuring

`struct ata_device *adev`
:   Drive in question

**Description**

> Set PIO mode for device, in host controller PCI config space.
>
> LOCKING:
> None (inherited from caller).

void do\_pata\_set\_dmamode(struct ata\_port \*ap, struct ata\_device \*adev, int isich)
:   Initialize host controller PATA PIO timings

**Parameters**

`struct ata_port *ap`
:   Port whose timings we are configuring

`struct ata_device *adev`
:   Drive in question

`int isich`
:   set if the chip is an ICH device

**Description**

> Set UDMA mode for device, in host controller PCI config space.
>
> LOCKING:
> None (inherited from caller).

void piix\_set\_dmamode(struct ata\_port \*ap, struct ata\_device \*adev)
:   Initialize host controller PATA DMA timings

**Parameters**

`struct ata_port *ap`
:   Port whose timings we are configuring

`struct ata_device *adev`
:   um

**Description**

> Set MW/UDMA mode for device, in host controller PCI config space.
>
> LOCKING:
> None (inherited from caller).

void ich\_set\_dmamode(struct ata\_port \*ap, struct ata\_device \*adev)
:   Initialize host controller PATA DMA timings

**Parameters**

`struct ata_port *ap`
:   Port whose timings we are configuring

`struct ata_device *adev`
:   um

**Description**

> Set MW/UDMA mode for device, in host controller PCI config space.
>
> LOCKING:
> None (inherited from caller).

int piix\_check\_450nx\_errata(struct pci\_dev \*ata\_dev)
:   Check for problem 450NX setup

**Parameters**

`struct pci_dev *ata_dev`
:   the PCI device to check

**Description**

> Check for the present of 450NX errata #19 and errata #25. If
> they are found return an error code so we can turn off DMA

int piix\_init\_one(struct pci\_dev \*pdev, const struct [pci\_device\_id](../PCI/pci.html#c.pci_device_id "pci_device_id") \*ent)
:   Register PIIX ATA PCI device with kernel services

**Parameters**

`struct pci_dev *pdev`
:   PCI device to register

`const struct pci_device_id *ent`
:   Entry in piix\_pci\_tbl matching with **pdev**

**Description**

> Called from kernel PCI layer. We probe for combined mode (sigh),
> and then hand over control to libata, for it to do the rest.
>
> LOCKING:
> Inherited from PCI layer (may sleep).

**Return**

Zero on success, or -ERRNO value.

## sata\_sil Internals

int sil\_set\_mode(struct ata\_link \*link, struct ata\_device \*\*r\_failed)
:   wrap set\_mode functions

**Parameters**

`struct ata_link *link`
:   link to set up

`struct ata_device **r_failed`
:   returned device when we fail

**Description**

> Wrap the libata method for device setup as after the setup we need
> to inspect the results and do some configuration work

void sil\_dev\_config(struct ata\_device \*dev)
:   Apply device/host-specific errata fixups

**Parameters**

`struct ata_device *dev`
:   Device to be examined

**Description**

> After the IDENTIFY [PACKET] DEVICE step is complete, and a
> device is known to be present, this function is called.
> We apply two errata fixups which are specific to Silicon Image,
> a Seagate and a Maxtor fixup.
>
> For certain Seagate devices, we must limit the maximum sectors
> to under 8K.
>
> For certain Maxtor devices, we must not program the drive
> beyond udma5.
>
> Both fixups are unfairly pessimistic. As soon as I get more
> information on these errata, I will create a more exhaustive
> list, and apply the fixups to only the specific
> devices/hosts/firmwares that need it.
>
> 20040111 - Seagate drives affected by the Mod15Write bug are quirked
> The Maxtor quirk is in sil\_quirks, but I’m keeping the original
> pessimistic fix for the following reasons...
> - There seems to be less info on it, only one device gleaned off the
> Windows driver, maybe only one is affected. More info would be greatly
> appreciated.
> - But then again UDMA5 is hardly anything to complain about

## Thanks

The bulk of the ATA knowledge comes thanks to long conversations with
Andre Hedrick (www.linux-ide.org), and long hours pondering the ATA and
SCSI specifications.

Thanks to Alan Cox for pointing out similarities between SATA and SCSI,
and in general for motivation to hack on libata.

libata’s device detection method, ata\_pio\_devchk, and in general all
the early probing was based on extensive study of Hale Landis’s
probe/reset code in his ATADRVR driver (www.ata-atapi.com).
