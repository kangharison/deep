# target and iSCSI Interfaces Guide

> 출처(원문): https://docs.kernel.org/driver-api/target.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# target and iSCSI Interfaces Guide

## Introduction and Overview

TBD

## Target core device interfaces

This section is blank because no kerneldoc comments have been added to
drivers/target/target\_core\_device.c.

## Target core transport interfaces

void transport\_init\_session(struct se\_session \*se\_sess)
:   initialize a session object

**Parameters**

`struct se_session *se_sess`
:   Session object pointer.

**Description**

The caller must have zero-initialized **se\_sess** before calling this function.

struct se\_session \*transport\_alloc\_session(enum target\_prot\_op sup\_prot\_ops)
:   allocate a session object and initialize it

**Parameters**

`enum target_prot_op sup_prot_ops`
:   bitmask that defines which T10-PI modes are supported.

int transport\_alloc\_session\_tags(struct se\_session \*se\_sess, unsigned int tag\_num, unsigned int tag\_size)
:   allocate target driver private data

**Parameters**

`struct se_session *se_sess`
:   Session pointer.

`unsigned int tag_num`
:   Maximum number of in-flight commands between initiator and target.

`unsigned int tag_size`
:   Size in bytes of the private data a target driver associates with
    each command.

int target\_init\_cmd(struct [se\_cmd](#c.target_init_cmd "se_cmd") \*se\_cmd, struct se\_session \*se\_sess, unsigned char \*sense, u64 unpacked\_lun, u32 data\_length, int task\_attr, int data\_dir, int flags)
:   initialize se\_cmd

**Parameters**

`struct se_cmd *se_cmd`
:   command descriptor to init

`struct se_session *se_sess`
:   associated se\_sess for endpoint

`unsigned char *sense`
:   pointer to SCSI sense buffer

`u64 unpacked_lun`
:   unpacked LUN to reference for `struct se_lun`

`u32 data_length`
:   fabric expected data transfer length

`int task_attr`
:   SAM task attribute

`int data_dir`
:   DMA data direction

`int flags`
:   flags for command submission from target\_sc\_flags\_tables

**Description**

Task tags are supported if the caller has set **se\_cmd->tag**.

If the fabric driver calls target\_stop\_session, then it must check the
return code and handle failures. This will never fail for other drivers,
and the return code can be ignored.

**Return**

* less than zero to signal active I/O shutdown failure.
* zero on success.

int target\_submit\_prep(struct [se\_cmd](#c.target_submit_prep "se_cmd") \*se\_cmd, unsigned char \*cdb, struct scatterlist \*sgl, u32 sgl\_count, struct scatterlist \*sgl\_bidi, u32 sgl\_bidi\_count, struct scatterlist \*sgl\_prot, u32 sgl\_prot\_count, gfp\_t gfp)
:   prepare cmd for submission

**Parameters**

`struct se_cmd *se_cmd`
:   command descriptor to prep

`unsigned char *cdb`
:   pointer to SCSI CDB

`struct scatterlist *sgl`
:   `struct scatterlist` memory for unidirectional mapping

`u32 sgl_count`
:   scatterlist count for unidirectional mapping

`struct scatterlist *sgl_bidi`
:   `struct scatterlist` memory for bidirectional READ mapping

`u32 sgl_bidi_count`
:   scatterlist count for bidirectional READ mapping

`struct scatterlist *sgl_prot`
:   `struct scatterlist` memory protection information

`u32 sgl_prot_count`
:   scatterlist count for protection information

`gfp_t gfp`
:   gfp allocation type

**Return**

* less than zero to signal failure.
* zero on success.

**Description**

If failure is returned, lio will the callers queue\_status to complete
the cmd.

void target\_submit\_cmd(struct [se\_cmd](#c.target_submit_cmd "se_cmd") \*se\_cmd, struct se\_session \*se\_sess, unsigned char \*cdb, unsigned char \*sense, u64 unpacked\_lun, u32 data\_length, int task\_attr, int data\_dir, int flags)
:   lookup unpacked lun and submit uninitialized se\_cmd

**Parameters**

`struct se_cmd *se_cmd`
:   command descriptor to submit

`struct se_session *se_sess`
:   associated se\_sess for endpoint

`unsigned char *cdb`
:   pointer to SCSI CDB

`unsigned char *sense`
:   pointer to SCSI sense buffer

`u64 unpacked_lun`
:   unpacked LUN to reference for `struct se_lun`

`u32 data_length`
:   fabric expected data transfer length

`int task_attr`
:   SAM task attribute

`int data_dir`
:   DMA data direction

`int flags`
:   flags for command submission from target\_sc\_flags\_tables

**Description**

Task tags are supported if the caller has set **se\_cmd->tag**.

This may only be called from process context, and also currently
assumes internal allocation of fabric payload buffer by target-core.

It also assumes interal target core SGL memory allocation.

This function must only be used by drivers that do their own
sync during shutdown and does not use target\_stop\_session. If there
is a failure this function will call into the fabric driver’s
queue\_status with a CHECK\_CONDITION.

int target\_submit(struct [se\_cmd](#c.target_submit "se_cmd") \*se\_cmd)
:   perform final initialization and submit cmd to LIO core

**Parameters**

`struct se_cmd *se_cmd`
:   command descriptor to submit

**Description**

target\_submit\_prep or something similar must have been called on the cmd,
and this must be called from process context.

int target\_submit\_tmr(struct [se\_cmd](#c.target_submit_tmr "se_cmd") \*se\_cmd, struct se\_session \*se\_sess, unsigned char \*sense, u64 unpacked\_lun, void \*fabric\_tmr\_ptr, unsigned char tm\_type, gfp\_t gfp, u64 tag, int flags)
:   lookup unpacked lun and submit uninitialized se\_cmd for TMR CDBs

**Parameters**

`struct se_cmd *se_cmd`
:   command descriptor to submit

`struct se_session *se_sess`
:   associated se\_sess for endpoint

`unsigned char *sense`
:   pointer to SCSI sense buffer

`u64 unpacked_lun`
:   unpacked LUN to reference for `struct se_lun`

`void *fabric_tmr_ptr`
:   fabric context for TMR req

`unsigned char tm_type`
:   Type of TM request

`gfp_t gfp`
:   gfp type for caller

`u64 tag`
:   referenced task tag for TMR\_ABORT\_TASK

`int flags`
:   submit cmd flags

**Description**

Callable from all contexts.

int target\_get\_sess\_cmd(struct [se\_cmd](#c.target_get_sess_cmd "se_cmd") \*se\_cmd, bool ack\_kref)
:   Verify the session is accepting cmds and take ref

**Parameters**

`struct se_cmd *se_cmd`
:   command descriptor to add

`bool ack_kref`
:   Signal that fabric will perform an ack [`target_put_sess_cmd()`](#c.target_put_sess_cmd "target_put_sess_cmd")

int target\_put\_sess\_cmd(struct [se\_cmd](#c.target_put_sess_cmd "se_cmd") \*se\_cmd)
:   decrease the command reference count

**Parameters**

`struct se_cmd *se_cmd`
:   command to drop a reference from

**Description**

Returns 1 if and only if this [`target_put_sess_cmd()`](#c.target_put_sess_cmd "target_put_sess_cmd") call caused the
refcount to drop to zero. Returns zero otherwise.

void target\_stop\_cmd\_counter(struct target\_cmd\_counter \*cmd\_cnt)
:   Stop new IO from being added to the counter.

**Parameters**

`struct target_cmd_counter *cmd_cnt`
:   counter to stop

void target\_stop\_session(struct se\_session \*se\_sess)
:   Stop new IO from being queued on the session.

**Parameters**

`struct se_session *se_sess`
:   session to stop

void target\_wait\_for\_cmds(struct target\_cmd\_counter \*cmd\_cnt)
:   Wait for outstanding cmds.

**Parameters**

`struct target_cmd_counter *cmd_cnt`
:   counter to wait for active I/O for.

void target\_wait\_for\_sess\_cmds(struct se\_session \*se\_sess)
:   Wait for outstanding commands

**Parameters**

`struct se_session *se_sess`
:   session to wait for active I/O

bool transport\_wait\_for\_tasks(struct se\_cmd \*cmd)
:   set CMD\_T\_STOP and wait for t\_transport\_stop\_comp

**Parameters**

`struct se_cmd *cmd`
:   command to wait on

int target\_send\_busy(struct se\_cmd \*cmd)
:   Send SCSI BUSY status back to the initiator

**Parameters**

`struct se_cmd *cmd`
:   SCSI command for which to send a BUSY reply.

**Note**

Only call this function if target\_submit\_cmd\*() failed.

## Target-supported userspace I/O

### Userspace I/O

Define a shared-memory interface for LIO to pass SCSI commands and
data to userspace for processing. This is to allow backends that
are too complex for in-kernel support to be possible.

It uses the UIO framework to do a lot of the device-creation and
introspection work for us.

See the .h file for how the ring is laid out. Note that while the
command ring is defined, the particulars of the data area are
not. Offset values in the command entry point to other locations
internal to the mmap-ed area. There is separate space outside the
command ring for data buffers. This leaves maximum flexibility for
moving buffer allocations, or even page flipping or other
allocation techniques, without altering the command ring layout.

SECURITY:
The user process must be assumed to be malicious. There’s no way to
prevent it breaking the command ring protocol if it wants, but in
order to prevent other issues we must only ever read *data* from
the shared memory area, not offsets or sizes. This applies to
command ring entries as well as the mailbox. Extra code needed for
this may have a ‘UAM’ comment.

### Ring Design

The mmaped area is divided into three parts:
1) The mailbox (`struct tcmu_mailbox`, below);
2) The command ring;
3) Everything beyond the command ring (data).

The mailbox tells userspace the offset of the command ring from the
start of the shared memory region, and how big the command ring is.

The kernel passes SCSI commands to userspace by putting a `struct
tcmu_cmd_entry` in the ring, updating mailbox->cmd\_head, and poking
userspace via UIO’s interrupt mechanism.

tcmu\_cmd\_entry contains a header. If the header type is PAD,
userspace should skip hdr->length bytes (mod cmdr\_size) to find the
next cmd\_entry.

Otherwise, the entry will contain offsets into the mmaped area that
contain the cdb and data buffers -- the latter accessible via the
iov array. iov addresses are also offsets into the shared area.

When userspace is completed handling the command, set
entry->rsp.scsi\_status, fill in rsp.sense\_buffer if appropriate,
and also set mailbox->cmd\_tail equal to the old cmd\_tail plus
hdr->length, mod cmdr\_size. If cmd\_tail doesn’t equal cmd\_head, it
should process the next packet the same way, and so on.

## iSCSI helper functions

void iscsi\_prep\_data\_out\_pdu(struct iscsi\_task \*task, struct iscsi\_r2t\_info \*r2t, struct iscsi\_data \*hdr)
:   initialize Data-Out

**Parameters**

`struct iscsi_task *task`
:   scsi command task

`struct iscsi_r2t_info *r2t`
:   R2T info

`struct iscsi_data *hdr`
:   iscsi data in pdu

**Notes**

> Initialize Data-Out within this R2T sequence and finds
> proper data\_offset within this SCSI command.
>
> This function is called with connection lock taken.

void \_\_iscsi\_put\_task(struct iscsi\_task \*task)
:   drop the refcount on a task

**Parameters**

`struct iscsi_task *task`
:   iscsi\_task to drop the refcount on

**Description**

The back\_lock must be held when calling in case it frees the task.

void iscsi\_complete\_scsi\_task(struct iscsi\_task \*task, uint32\_t exp\_cmdsn, uint32\_t max\_cmdsn)
:   finish scsi task normally

**Parameters**

`struct iscsi_task *task`
:   iscsi task for scsi cmd

`uint32_t exp_cmdsn`
:   expected cmd sn in cpu format

`uint32_t max_cmdsn`
:   max cmd sn in cpu format

**Description**

This is used when drivers do not need or cannot perform
lower level pdu processing.

Called with session back\_lock

struct iscsi\_task \*iscsi\_itt\_to\_task(struct iscsi\_conn \*conn, itt\_t itt)
:   look up task by itt

**Parameters**

`struct iscsi_conn *conn`
:   iscsi connection

`itt_t itt`
:   itt

**Description**

This should be used for mgmt tasks like login and nops, or if
the LDD’s itt space does not include the session age.

The session back\_lock must be held.

int \_\_iscsi\_complete\_pdu(struct iscsi\_conn \*conn, struct iscsi\_hdr \*hdr, char \*data, int datalen)
:   complete pdu

**Parameters**

`struct iscsi_conn *conn`
:   iscsi conn

`struct iscsi_hdr *hdr`
:   iscsi header

`char *data`
:   data buffer

`int datalen`
:   len of data buffer

**Description**

Completes pdu processing by freeing any resources allocated at
queuecommand or send generic. session back\_lock must be held and verify
itt must have been called.

struct iscsi\_task \*iscsi\_itt\_to\_ctask(struct iscsi\_conn \*conn, itt\_t itt)
:   look up ctask by itt

**Parameters**

`struct iscsi_conn *conn`
:   iscsi connection

`itt_t itt`
:   itt

**Description**

This should be used for cmd tasks.

The session back\_lock must be held.

void iscsi\_requeue\_task(struct iscsi\_task \*task)
:   requeue task to run from session workqueue

**Parameters**

`struct iscsi_task *task`
:   task to requeue

**Description**

Callers must have taken a ref to the task that is going to be requeued.

void iscsi\_suspend\_queue(struct iscsi\_conn \*conn)
:   suspend iscsi\_queuecommand

**Parameters**

`struct iscsi_conn *conn`
:   iscsi conn to stop queueing IO on

**Description**

This grabs the session frwd\_lock to make sure no one is in
xmit\_task/queuecommand, and then sets suspend to prevent
new commands from being queued. This only needs to be called
by offload drivers that need to sync a path like ep disconnect
with the iscsi\_queuecommand/xmit\_task. To start IO again libiscsi
will call iscsi\_start\_tx and iscsi\_unblock\_session when in FFP.

void iscsi\_suspend\_tx(struct iscsi\_conn \*conn)
:   suspend iscsi\_data\_xmit

**Parameters**

`struct iscsi_conn *conn`
:   iscsi conn to stop processing IO on.

**Description**

This function sets the suspend bit to prevent iscsi\_data\_xmit
from sending new IO, and if work is queued on the xmit thread
it will wait for it to be completed.

void iscsi\_suspend\_rx(struct iscsi\_conn \*conn)
:   Prevent recvwork from running again.

**Parameters**

`struct iscsi_conn *conn`
:   iscsi conn to stop.

void iscsi\_conn\_unbind(struct iscsi\_cls\_conn \*cls\_conn, bool is\_active)
:   prevent queueing to conn.

**Parameters**

`struct iscsi_cls_conn *cls_conn`
:   iscsi conn ep is bound to.

`bool is_active`
:   is the conn in use for boot or is this for EH/termination

**Description**

This must be called by drivers implementing the ep\_disconnect callout.
It disables queueing to the connection from libiscsi in preparation for
an ep\_disconnect call.

int iscsi\_eh\_session\_reset(struct scsi\_cmnd \*sc)
:   drop session and attempt relogin

**Parameters**

`struct scsi_cmnd *sc`
:   scsi command

**Description**

This function will wait for a relogin, session termination from
userspace, or a recovery/replacement timeout.

int iscsi\_eh\_recover\_target(struct scsi\_cmnd \*sc)
:   reset target and possibly the session

**Parameters**

`struct scsi_cmnd *sc`
:   scsi command

**Description**

This will attempt to send a warm target reset. If that fails,
we will escalate to ERL0 session recovery.

int iscsi\_host\_add(struct Scsi\_Host \*shost, struct [device](infrastructure.html#c.device "device") \*pdev)
:   add host to system

**Parameters**

`struct Scsi_Host *shost`
:   scsi host

`struct device *pdev`
:   parent device

**Description**

This should be called by partial offload and software iscsi drivers
to add a host to the system.

struct Scsi\_Host \*iscsi\_host\_alloc(const struct scsi\_host\_template \*sht, int dd\_data\_size, bool xmit\_can\_sleep)
:   allocate a host and driver data

**Parameters**

`const struct scsi_host_template *sht`
:   scsi host template

`int dd_data_size`
:   driver host data size

`bool xmit_can_sleep`
:   bool indicating if LLD will queue IO from a work queue

**Description**

This should be called by partial offload and software iscsi drivers.
To access the driver specific memory use the `iscsi_host_priv()` macro.

void iscsi\_host\_remove(struct Scsi\_Host \*shost, bool is\_shutdown)
:   remove host and sessions

**Parameters**

`struct Scsi_Host *shost`
:   scsi host

`bool is_shutdown`
:   true if called from a driver shutdown callout

**Description**

If there are any sessions left, this will initiate the removal and wait
for the completion.

struct iscsi\_cls\_session \*iscsi\_session\_setup(struct iscsi\_transport \*iscsit, struct Scsi\_Host \*shost, uint16\_t cmds\_max, int dd\_size, int cmd\_task\_size, uint32\_t initial\_cmdsn, unsigned int id)
:   create iscsi cls session and host and session

**Parameters**

`struct iscsi_transport *iscsit`
:   iscsi transport template

`struct Scsi_Host *shost`
:   scsi host

`uint16_t cmds_max`
:   session can queue

`int dd_size`
:   private driver data size, added to session allocation size

`int cmd_task_size`
:   LLD task private data size

`uint32_t initial_cmdsn`
:   initial CmdSN

`unsigned int id`
:   target ID to add to this session

**Description**

This can be used by software iscsi\_transports that allocate
a session per scsi host.

Callers should set cmds\_max to the largest total numer (mgmt + scsi) of
tasks they support. The iscsi layer reserves ISCSI\_MGMT\_CMDS\_MAX tasks
for nop handling and login/logout requests.

void iscsi\_session\_free(struct iscsi\_cls\_session \*cls\_session)
:   Free iscsi session and it’s resources

**Parameters**

`struct iscsi_cls_session *cls_session`
:   iscsi session

void iscsi\_session\_teardown(struct iscsi\_cls\_session \*cls\_session)
:   destroy session and cls\_session

**Parameters**

`struct iscsi_cls_session *cls_session`
:   iscsi session

struct iscsi\_cls\_conn \*iscsi\_conn\_setup(struct iscsi\_cls\_session \*cls\_session, int dd\_size, uint32\_t conn\_idx)
:   create iscsi\_cls\_conn and iscsi\_conn

**Parameters**

`struct iscsi_cls_session *cls_session`
:   iscsi\_cls\_session

`int dd_size`
:   private driver data size

`uint32_t conn_idx`
:   cid

void iscsi\_conn\_teardown(struct iscsi\_cls\_conn \*cls\_conn)
:   teardown iscsi connection

**Parameters**

`struct iscsi_cls_conn *cls_conn`
:   iscsi class connection

**Description**

TODO: we may need to make this into a two step process
like scsi-mls remove + put host

## iSCSI boot information

struct iscsi\_boot\_kobj \*iscsi\_boot\_create\_target(struct iscsi\_boot\_kset \*boot\_kset, int index, void \*data, ssize\_t (\*show)(void \*data, int type, char \*buf), umode\_t (\*is\_visible)(void \*data, int type), void (\*release)(void \*data))
:   create boot target sysfs dir

**Parameters**

`struct iscsi_boot_kset *boot_kset`
:   boot kset

`int index`
:   the target id

`void *data`
:   driver specific data for target

`ssize_t (*show) (void *data, int type, char *buf)`
:   attr show function

`umode_t (*is_visible) (void *data, int type)`
:   attr visibility function

`void (*release) (void *data)`
:   release function

**Note**

The boot sysfs lib will free the data passed in for the caller
when all refs to the target kobject have been released.

struct iscsi\_boot\_kobj \*iscsi\_boot\_create\_initiator(struct iscsi\_boot\_kset \*boot\_kset, int index, void \*data, ssize\_t (\*show)(void \*data, int type, char \*buf), umode\_t (\*is\_visible)(void \*data, int type), void (\*release)(void \*data))
:   create boot initiator sysfs dir

**Parameters**

`struct iscsi_boot_kset *boot_kset`
:   boot kset

`int index`
:   the initiator id

`void *data`
:   driver specific data

`ssize_t (*show) (void *data, int type, char *buf)`
:   attr show function

`umode_t (*is_visible) (void *data, int type)`
:   attr visibility function

`void (*release) (void *data)`
:   release function

**Note**

The boot sysfs lib will free the data passed in for the caller
when all refs to the initiator kobject have been released.

struct iscsi\_boot\_kobj \*iscsi\_boot\_create\_ethernet(struct iscsi\_boot\_kset \*boot\_kset, int index, void \*data, ssize\_t (\*show)(void \*data, int type, char \*buf), umode\_t (\*is\_visible)(void \*data, int type), void (\*release)(void \*data))
:   create boot ethernet sysfs dir

**Parameters**

`struct iscsi_boot_kset *boot_kset`
:   boot kset

`int index`
:   the ethernet device id

`void *data`
:   driver specific data

`ssize_t (*show) (void *data, int type, char *buf)`
:   attr show function

`umode_t (*is_visible) (void *data, int type)`
:   attr visibility function

`void (*release) (void *data)`
:   release function

**Note**

The boot sysfs lib will free the data passed in for the caller
when all refs to the ethernet kobject have been released.

struct iscsi\_boot\_kobj \*iscsi\_boot\_create\_acpitbl(struct iscsi\_boot\_kset \*boot\_kset, int index, void \*data, ssize\_t (\*show)(void \*data, int type, char \*buf), umode\_t (\*is\_visible)(void \*data, int type), void (\*release)(void \*data))
:   create boot acpi table sysfs dir

**Parameters**

`struct iscsi_boot_kset *boot_kset`
:   boot kset

`int index`
:   not used

`void *data`
:   driver specific data

`ssize_t (*show)(void *data, int type, char *buf)`
:   attr show function

`umode_t (*is_visible)(void *data, int type)`
:   attr visibility function

`void (*release)(void *data)`
:   release function

**Note**

The boot sysfs lib will free the data passed in for the caller
when all refs to the acpitbl kobject have been released.

struct iscsi\_boot\_kset \*iscsi\_boot\_create\_kset(const char \*set\_name)
:   creates root sysfs tree

**Parameters**

`const char *set_name`
:   name of root dir

struct iscsi\_boot\_kset \*iscsi\_boot\_create\_host\_kset(unsigned int hostno)
:   creates root sysfs tree for a scsi host

**Parameters**

`unsigned int hostno`
:   host number of scsi host

void iscsi\_boot\_destroy\_kset(struct iscsi\_boot\_kset \*boot\_kset)
:   destroy kset and kobjects under it

**Parameters**

`struct iscsi_boot_kset *boot_kset`
:   boot kset

**Description**

This will remove the kset and kobjects and attrs under it.

## iSCSI TCP interfaces

int iscsi\_sw\_tcp\_recv(read\_descriptor\_t \*rd\_desc, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, unsigned int offset, size\_t len)
:   TCP receive in sendfile fashion

**Parameters**

`read_descriptor_t *rd_desc`
:   read descriptor

`struct sk_buff *skb`
:   socket buffer

`unsigned int offset`
:   offset in skb

`size_t len`
:   skb->len - offset

int iscsi\_sw\_sk\_state\_check(struct [sock](../networking/kapi.html#c.sock "sock") \*sk)
:   check socket state

**Parameters**

`struct sock *sk`
:   socket

**Description**

If the socket is in CLOSE or CLOSE\_WAIT we should
not close the connection if there is still some
data pending.

Must be called with sk\_callback\_lock.

void iscsi\_sw\_tcp\_write\_space(struct [sock](../networking/kapi.html#c.sock "sock") \*sk)
:   Called when more output buffer space is available

**Parameters**

`struct sock *sk`
:   socket space is available for

int iscsi\_sw\_tcp\_xmit\_segment(struct iscsi\_tcp\_conn \*tcp\_conn, struct iscsi\_segment \*segment)
:   transmit segment

**Parameters**

`struct iscsi_tcp_conn *tcp_conn`
:   the iSCSI TCP connection

`struct iscsi_segment *segment`
:   the buffer to transmnit

**Description**

This function transmits as much of the buffer as
the network layer will accept, and returns the number of
bytes transmitted.

If CRC hashing is enabled, the function will compute the
hash as it goes. When the entire segment has been transmitted,
it will retrieve the hash value and send it as well.

int iscsi\_sw\_tcp\_xmit(struct iscsi\_conn \*conn)
:   TCP transmit

**Parameters**

`struct iscsi_conn *conn`
:   iscsi connection

int iscsi\_sw\_tcp\_xmit\_qlen(struct iscsi\_conn \*conn)
:   return the number of bytes queued for xmit

**Parameters**

`struct iscsi_conn *conn`
:   iscsi connection

int iscsi\_tcp\_segment\_done(struct iscsi\_tcp\_conn \*tcp\_conn, struct iscsi\_segment \*segment, int recv, unsigned copied)
:   check whether the segment is complete

**Parameters**

`struct iscsi_tcp_conn *tcp_conn`
:   iscsi tcp connection

`struct iscsi_segment *segment`
:   iscsi segment to check

`int recv`
:   set to one of this is called from the recv path

`unsigned copied`
:   number of bytes copied

**Description**

Check if we’re done receiving this segment. If the receive
buffer is full but we expect more data, move on to the
next entry in the scatterlist.

If the amount of data we received isn’t a multiple of 4,
we will transparently receive the pad bytes, too.

This function must be re-entrant.

void iscsi\_tcp\_hdr\_recv\_prep(struct iscsi\_tcp\_conn \*tcp\_conn)
:   prep segment for hdr reception

**Parameters**

`struct iscsi_tcp_conn *tcp_conn`
:   iscsi connection to prep for

**Description**

This function always passes NULL for the crcp argument, because when this
function is called we do not yet know the final size of the header and want
to delay the digest processing until we know that.

void iscsi\_tcp\_cleanup\_task(struct iscsi\_task \*task)
:   free tcp\_task resources

**Parameters**

`struct iscsi_task *task`
:   iscsi task

**Description**

must be called with session back\_lock

int iscsi\_tcp\_recv\_segment\_is\_hdr(struct iscsi\_tcp\_conn \*tcp\_conn)
:   tests if we are reading in a header

**Parameters**

`struct iscsi_tcp_conn *tcp_conn`
:   iscsi tcp conn

**Description**

returns non zero if we are currently processing or setup to process
a header.

int iscsi\_tcp\_recv\_skb(struct iscsi\_conn \*conn, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, unsigned int offset, bool offloaded, int \*status)
:   Process skb

**Parameters**

`struct iscsi_conn *conn`
:   iscsi connection

`struct sk_buff *skb`
:   network buffer with header and/or data segment

`unsigned int offset`
:   offset in skb

`bool offloaded`
:   bool indicating if transfer was offloaded

`int *status`
:   iscsi TCP status result

**Description**

Will return status of transfer in **status**. And will return
number of bytes copied.

int iscsi\_tcp\_task\_init(struct iscsi\_task \*task)
:   Initialize iSCSI SCSI\_READ or SCSI\_WRITE commands

**Parameters**

`struct iscsi_task *task`
:   scsi command task

int iscsi\_tcp\_task\_xmit(struct iscsi\_task \*task)
:   xmit normal PDU task

**Parameters**

`struct iscsi_task *task`
:   iscsi command task

**Description**

We’re expected to return 0 when everything was transmitted successfully,
-EAGAIN if there’s still data in the queue, or != 0 for any other kind
of error.
