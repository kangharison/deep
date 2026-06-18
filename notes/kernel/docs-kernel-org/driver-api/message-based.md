# Message-based devices

> 출처(원문): https://docs.kernel.org/driver-api/message-based.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Message-based devices

## Fusion message devices

u8 mpt\_register(MPT\_CALLBACK cbfunc, MPT\_DRIVER\_CLASS dclass, char \*func\_name)
:   Register protocol-specific main callback handler.

**Parameters**

`MPT_CALLBACK cbfunc`
:   callback function pointer

`MPT_DRIVER_CLASS dclass`
:   Protocol driver’s class (`MPT_DRIVER_CLASS` `enum value`)

`char *func_name`
:   call function’s name

**Description**

> This routine is called by a protocol-specific driver (SCSI host,
> LAN, SCSI target) to register its reply callback routine. Each
> protocol-specific driver must do this before it will be able to
> use any IOC resources, such as obtaining request frames.

**NOTES**

The SCSI protocol driver currently calls this routine thrice
:   in order to register separate callbacks; one for “normal” SCSI IO;
    one for MptScsiTaskMgmt requests; one for Scan/DV requests.

    Returns u8 valued “handle” in the range (and S.O.D. order)
    {N,...,7,6,5,...,1} if successful.
    A return value of MPT\_MAX\_PROTOCOL\_DRIVERS (including zero!) should be
    considered an error by the caller.

void mpt\_deregister(u8 cb\_idx)
:   Deregister a protocol drivers resources.

**Parameters**

`u8 cb_idx`
:   previously registered callback handle

**Description**

> Each protocol-specific driver should call this routine when its
> module is unloaded.

int mpt\_event\_register(u8 cb\_idx, MPT\_EVHANDLER ev\_cbfunc)
:   Register protocol-specific event callback handler.

**Parameters**

`u8 cb_idx`
:   previously registered (via mpt\_register) callback handle

`MPT_EVHANDLER ev_cbfunc`
:   callback function

**Description**

> This routine can be called by one or more protocol-specific drivers
> if/when they choose to be notified of MPT events.
>
> Returns 0 for success.

void mpt\_event\_deregister(u8 cb\_idx)
:   Deregister protocol-specific event callback handler

**Parameters**

`u8 cb_idx`
:   previously registered callback handle

**Description**

> Each protocol-specific driver should call this routine
> when it does not (or can no longer) handle events,
> or when its module is unloaded.

int mpt\_reset\_register(u8 cb\_idx, MPT\_RESETHANDLER reset\_func)
:   Register protocol-specific IOC reset handler.

**Parameters**

`u8 cb_idx`
:   previously registered (via mpt\_register) callback handle

`MPT_RESETHANDLER reset_func`
:   reset function

**Description**

> This routine can be called by one or more protocol-specific drivers
> if/when they choose to be notified of IOC resets.
>
> Returns 0 for success.

void mpt\_reset\_deregister(u8 cb\_idx)
:   Deregister protocol-specific IOC reset handler.

**Parameters**

`u8 cb_idx`
:   previously registered callback handle

**Description**

> Each protocol-specific driver should call this routine
> when it does not (or can no longer) handle IOC reset handling,
> or when its module is unloaded.

int mpt\_device\_driver\_register(struct mpt\_pci\_driver \*dd\_cbfunc, u8 cb\_idx)
:   Register device driver hooks

**Parameters**

`struct mpt_pci_driver * dd_cbfunc`
:   driver callbacks struct

`u8 cb_idx`
:   MPT protocol driver index

void mpt\_device\_driver\_deregister(u8 cb\_idx)
:   DeRegister device driver hooks

**Parameters**

`u8 cb_idx`
:   MPT protocol driver index

MPT\_FRAME\_HDR \*mpt\_get\_msg\_frame(u8 cb\_idx, MPT\_ADAPTER \*ioc)
:   Obtain an MPT request frame from the pool

**Parameters**

`u8 cb_idx`
:   Handle of registered MPT protocol driver

`MPT_ADAPTER *ioc`
:   Pointer to MPT adapter structure

**Description**

> Obtain an MPT request frame from the pool (of 1024) that are
> allocated per MPT adapter.
>
> Returns pointer to a MPT request frame or `NULL` if none are available
> or IOC is not active.

void mpt\_put\_msg\_frame(u8 cb\_idx, MPT\_ADAPTER \*ioc, MPT\_FRAME\_HDR \*mf)
:   Send a protocol-specific MPT request frame to an IOC

**Parameters**

`u8 cb_idx`
:   Handle of registered MPT protocol driver

`MPT_ADAPTER *ioc`
:   Pointer to MPT adapter structure

`MPT_FRAME_HDR *mf`
:   Pointer to MPT request frame

**Description**

> This routine posts an MPT request frame to the request post FIFO of a
> specific MPT adapter.

void mpt\_put\_msg\_frame\_hi\_pri(u8 cb\_idx, MPT\_ADAPTER \*ioc, MPT\_FRAME\_HDR \*mf)
:   Send a hi-pri protocol-specific MPT request frame

**Parameters**

`u8 cb_idx`
:   Handle of registered MPT protocol driver

`MPT_ADAPTER *ioc`
:   Pointer to MPT adapter structure

`MPT_FRAME_HDR *mf`
:   Pointer to MPT request frame

**Description**

> Send a protocol-specific MPT request frame to an IOC using
> hi-priority request queue.
>
> This routine posts an MPT request frame to the request post FIFO of a
> specific MPT adapter.

void mpt\_free\_msg\_frame(MPT\_ADAPTER \*ioc, MPT\_FRAME\_HDR \*mf)
:   Place MPT request frame back on FreeQ.

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT adapter structure

`MPT_FRAME_HDR *mf`
:   Pointer to MPT request frame

**Description**

> This routine places a MPT request frame back on the MPT adapter’s
> FreeQ.

int mpt\_send\_handshake\_request(u8 cb\_idx, MPT\_ADAPTER \*ioc, int reqBytes, u32 \*req, int sleepFlag)
:   Send MPT request via doorbell handshake method.

**Parameters**

`u8 cb_idx`
:   Handle of registered MPT protocol driver

`MPT_ADAPTER *ioc`
:   Pointer to MPT adapter structure

`int reqBytes`
:   Size of the request in bytes

`u32 *req`
:   Pointer to MPT request frame

`int sleepFlag`
:   Use schedule if CAN\_SLEEP else use udelay.

**Description**

> This routine is used exclusively to send MptScsiTaskMgmt
> requests since they are required to be sent via doorbell handshake.

**NOTE**

It is the callers responsibility to byte-swap fields in the
:   request which are greater than 1 byte in size.

    Returns 0 for success, non-zero for failure.

int mpt\_verify\_adapter(int iocid, MPT\_ADAPTER \*\*iocpp)
:   Given IOC identifier, set pointer to its adapter structure.

**Parameters**

`int iocid`
:   IOC unique identifier (integer)

`MPT_ADAPTER **iocpp`
:   Pointer to pointer to IOC adapter

**Description**

> Given a unique IOC identifier, set pointer to the associated MPT
> adapter structure.
>
> Returns iocid and sets iocpp if iocid is found.
> Returns -1 if iocid is not found.

int mpt\_attach(struct pci\_dev \*pdev, const struct [pci\_device\_id](../PCI/pci.html#c.pci_device_id "pci_device_id") \*id)
:   Install a PCI intelligent MPT adapter.

**Parameters**

`struct pci_dev *pdev`
:   Pointer to pci\_dev structure

`const struct pci_device_id *id`
:   PCI device ID information

**Description**

> This routine performs all the steps necessary to bring the IOC of
> a MPT adapter to a OPERATIONAL state. This includes registering
> memory regions, registering the interrupt, and allocating request
> and reply memory pools.
>
> This routine also pre-fetches the LAN MAC address of a Fibre Channel
> MPT adapter.
>
> Returns 0 for success, non-zero for failure.
>
> TODO: Add support for polled controllers

void mpt\_detach(struct pci\_dev \*pdev)
:   Remove a PCI intelligent MPT adapter.

**Parameters**

`struct pci_dev *pdev`
:   Pointer to pci\_dev structure

int mpt\_suspend(struct pci\_dev \*pdev, pm\_message\_t state)
:   Fusion MPT base driver suspend routine.

**Parameters**

`struct pci_dev *pdev`
:   Pointer to pci\_dev structure

`pm_message_t state`
:   new state to enter

int mpt\_resume(struct pci\_dev \*pdev)
:   Fusion MPT base driver resume routine.

**Parameters**

`struct pci_dev *pdev`
:   Pointer to pci\_dev structure

u32 mpt\_GetIocState(MPT\_ADAPTER \*ioc, int cooked)
:   Get the current state of a MPT adapter.

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`int cooked`
:   Request raw or cooked IOC state

**Description**

> Returns all IOC Doorbell register bits if cooked==0, else just the
> Doorbell bits in MPI\_IOC\_STATE\_MASK.

int mpt\_alloc\_fw\_memory(MPT\_ADAPTER \*ioc, int size)
:   allocate firmware memory

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`int size`
:   total FW bytes

**Description**

> If memory has already been allocated, the same (cached) value
> is returned.
>
> Return 0 if successful, or non-zero for failure

void mpt\_free\_fw\_memory(MPT\_ADAPTER \*ioc)
:   free firmware memory

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

**Description**

> If alt\_img is NULL, delete from ioc structure.
> Else, delete a secondary image in same format.

int mptbase\_sas\_persist\_operation(MPT\_ADAPTER \*ioc, u8 persist\_opcode)
:   Perform operation on SAS Persistent Table

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`u8 persist_opcode`
:   see below

**Description**

> |  |  |
> | --- | --- |
> | MPI\_SAS\_OP\_CLEAR\_NOT\_PRESENT | Free all persist TargetID mappings for devices not currently present. |
> | MPI\_SAS\_OP\_CLEAR\_ALL\_PERSISTENT | Clear al persist TargetID mappings |

**NOTE**

Don’t use not this function during interrupt time.

> Returns 0 for success, non-zero error

int mpt\_raid\_phys\_disk\_pg0(MPT\_ADAPTER \*ioc, u8 phys\_disk\_num, RaidPhysDiskPage0\_t \*phys\_disk)
:   returns phys disk page zero

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to a Adapter Structure

`u8 phys_disk_num`
:   io unit unique phys disk num generated by the ioc

`RaidPhysDiskPage0_t *phys_disk`
:   requested payload data returned

**Return**

0 on success
-EFAULT if read of config page header fails or data pointer not NULL
-ENOMEM if pci\_alloc failed

int mpt\_raid\_phys\_disk\_get\_num\_paths(MPT\_ADAPTER \*ioc, u8 phys\_disk\_num)
:   returns number paths associated to this phys\_num

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to a Adapter Structure

`u8 phys_disk_num`
:   io unit unique phys disk num generated by the ioc

**Return**

returns number paths

int mpt\_raid\_phys\_disk\_pg1(MPT\_ADAPTER \*ioc, u8 phys\_disk\_num, RaidPhysDiskPage1\_t \*phys\_disk)
:   returns phys disk page 1

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to a Adapter Structure

`u8 phys_disk_num`
:   io unit unique phys disk num generated by the ioc

`RaidPhysDiskPage1_t *phys_disk`
:   requested payload data returned

**Return**

0 on success
-EFAULT if read of config page header fails or data pointer not NULL
-ENOMEM if pci\_alloc failed

int mpt\_findImVolumes(MPT\_ADAPTER \*ioc)
:   Identify IDs of hidden disks and RAID Volumes

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to a Adapter Strucutre

**Return**

0 on success
-EFAULT if read of config page header fails or data pointer not NULL
-ENOMEM if pci\_alloc failed

int mpt\_config(MPT\_ADAPTER \*ioc, CONFIGPARMS \*pCfg)
:   Generic function to issue config message

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to an adapter structure

`CONFIGPARMS *pCfg`
:   Pointer to a configuration structure. Struct contains
    action, page address, direction, physical address
    and pointer to a configuration page header
    Page header is updated.

**Description**

> Returns 0 for success
> -EAGAIN if no msg frames currently available
> -EFAULT for non-successful reply or no reply (timeout)

void mpt\_print\_ioc\_summary(MPT\_ADAPTER \*ioc, char \*buffer, int \*size, int len, int showlan)
:   Write ASCII summary of IOC to a buffer.

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`char *buffer`
:   Pointer to buffer where IOC summary info should be written

`int *size`
:   Pointer to number of bytes we wrote (set by this routine)

`int len`
:   Offset at which to start writing in buffer

`int showlan`
:   Display LAN stuff?

**Description**

> This routine writes (english readable) ASCII text, which represents
> a summary of IOC information, to a buffer.

int mpt\_set\_taskmgmt\_in\_progress\_flag(MPT\_ADAPTER \*ioc)
:   set flags associated with task management

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

**Description**

> Returns 0 for SUCCESS or -1 if FAILED.
>
> If -1 is return, then it was not possible to set the flags

void mpt\_clear\_taskmgmt\_in\_progress\_flag(MPT\_ADAPTER \*ioc)
:   clear flags associated with task management

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

void \_\_noreturn mpt\_halt\_firmware(MPT\_ADAPTER \*ioc)
:   Halts the firmware if it is operational and panic the kernel

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

int mpt\_Soft\_Hard\_ResetHandler(MPT\_ADAPTER \*ioc, int sleepFlag)
:   Try less expensive reset

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`int sleepFlag`
:   Indicates if sleep or schedule must be called.

**Description**

> Returns 0 for SUCCESS or -1 if FAILED.
> Try for softreset first, only if it fails go for expensive
> HardReset.

int mpt\_HardResetHandler(MPT\_ADAPTER \*ioc, int sleepFlag)
:   Generic reset handler

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`int sleepFlag`
:   Indicates if sleep or schedule must be called.

**Description**

> Issues SCSI Task Management call based on input arg values.
> If TaskMgmt fails, returns associated SCSI request.
>
> Remark: \_HardResetHandler can be invoked from an interrupt thread (timer)
> or a non-interrupt thread. In the former, must not call `schedule()`.

**Note**

A return of -1 is a FATAL error case, as it means a
:   FW reload/initialization failed.

    Returns 0 for SUCCESS or -1 if FAILED.

const char \*mptscsih\_info(struct Scsi\_Host \*SChost)
:   Return information about MPT adapter

**Parameters**

`struct Scsi_Host *SChost`
:   Pointer to Scsi\_Host structure

**Description**

> (linux scsi\_host\_template.info routine)
>
> Returns pointer to buffer where information was written.

enum scsi\_qc\_status mptscsih\_qcmd(struct scsi\_cmnd \*SCpnt)
:   Primary Fusion MPT SCSI initiator IO start routine.

**Parameters**

`struct scsi_cmnd *SCpnt`
:   Pointer to scsi\_cmnd structure

**Description**

> (linux scsi\_host\_template.queuecommand routine)
> This is the primary SCSI IO start routine. Create a MPI SCSIIORequest
> from a linux scsi\_cmnd request and send it to the IOC.
>
> Returns 0. (rtn value discarded by linux scsi mid-layer)

int mptscsih\_IssueTaskMgmt(MPT\_SCSI\_HOST \*hd, u8 type, u8 channel, u8 id, u64 lun, int ctx2abort, ulong timeout)
:   Generic send Task Management function.

**Parameters**

`MPT_SCSI_HOST *hd`
:   Pointer to MPT\_SCSI\_HOST structure

`u8 type`
:   Task Management type

`u8 channel`
:   channel number for task management

`u8 id`
:   Logical Target ID for reset (if appropriate)

`u64 lun`
:   Logical Unit for reset (if appropriate)

`int ctx2abort`
:   Context for the task to be aborted (if appropriate)

`ulong timeout`
:   timeout for task management control

**Description**

> Remark: \_HardResetHandler can be invoked from an interrupt thread (timer)
> or a non-interrupt thread. In the former, must not call `schedule()`.
>
> Not all fields are meaningfull for all task types.
>
> Returns 0 for SUCCESS, or FAILED.

int mptscsih\_abort(struct scsi\_cmnd \*SCpnt)
:   Abort linux scsi\_cmnd routine, new\_eh variant

**Parameters**

`struct scsi_cmnd * SCpnt`
:   Pointer to scsi\_cmnd structure, IO to be aborted

**Description**

> (linux scsi\_host\_template.eh\_abort\_handler routine)
>
> Returns SUCCESS or FAILED.

int mptscsih\_dev\_reset(struct scsi\_cmnd \*SCpnt)
:   Perform a SCSI LOGICAL\_UNIT\_RESET!

**Parameters**

`struct scsi_cmnd * SCpnt`
:   Pointer to scsi\_cmnd structure, IO which reset is due to

**Description**

> (linux scsi\_host\_template.eh\_dev\_reset\_handler routine)
>
> Returns SUCCESS or FAILED.

int mptscsih\_bus\_reset(struct scsi\_cmnd \*SCpnt)
:   Perform a SCSI BUS\_RESET! new\_eh variant

**Parameters**

`struct scsi_cmnd * SCpnt`
:   Pointer to scsi\_cmnd structure, IO which reset is due to

**Description**

> (linux scsi\_host\_template.eh\_bus\_reset\_handler routine)
>
> Returns SUCCESS or FAILED.

int mptscsih\_host\_reset(struct scsi\_cmnd \*SCpnt)
:   Perform a SCSI host adapter RESET (new\_eh variant)

**Parameters**

`struct scsi_cmnd *SCpnt`
:   Pointer to scsi\_cmnd structure, IO which reset is due to

**Description**

> (linux scsi\_host\_template.eh\_host\_reset\_handler routine)
>
> Returns SUCCESS or FAILED.

int mptscsih\_taskmgmt\_complete(MPT\_ADAPTER \*ioc, MPT\_FRAME\_HDR \*mf, MPT\_FRAME\_HDR \*mr)
:   Registered with Fusion MPT base driver

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`MPT_FRAME_HDR *mf`
:   Pointer to SCSI task mgmt request frame

`MPT_FRAME_HDR *mr`
:   Pointer to SCSI task mgmt reply frame

**Description**

> This routine is called from mptbase.c::`mpt_interrupt()` at the completion
> of any SCSI task management request.
> This routine is registered with the MPT (base) driver at driver
> load/init time via the [`mpt_register()`](#c.mpt_register "mpt_register") API call.
>
> Returns 1 indicating alloc’d request frame ptr should be freed.

struct scsi\_cmnd \*mptscsih\_get\_scsi\_lookup(MPT\_ADAPTER \*ioc, int i)
:   retrieves scmd entry

**Parameters**

`MPT_ADAPTER *ioc`
:   Pointer to MPT\_ADAPTER structure

`int i`
:   index into the array

**Description**

Returns the scsi\_cmd pointer
