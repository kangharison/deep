# Other Firmware Interfaces

> 출처(원문): https://docs.kernel.org/driver-api/firmware/other_interfaces.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Other Firmware Interfaces

## DMI Interfaces

int dmi\_check\_system(const struct dmi\_system\_id \*list)
:   check system DMI data

**Parameters**

`const struct dmi_system_id *list`
:   array of dmi\_system\_id structures to match against
    All non-null elements of the list must match
    their slot’s (field index’s) data (i.e., each
    list string must be a substring of the specified
    DMI slot’s string data) to be considered a
    successful match.

**Description**

> Walk the blacklist table running matching functions until someone
> returns non zero or we hit the end. Callback function is called for
> each successful match. Returns the number of matches.
>
> dmi\_setup must be called before this function is called.

const struct dmi\_system\_id \*dmi\_first\_match(const struct dmi\_system\_id \*list)
:   find dmi\_system\_id structure matching system DMI data

**Parameters**

`const struct dmi_system_id *list`
:   array of dmi\_system\_id structures to match against
    All non-null elements of the list must match
    their slot’s (field index’s) data (i.e., each
    list string must be a substring of the specified
    DMI slot’s string data) to be considered a
    successful match.

**Description**

> Walk the blacklist table until the first match is found. Return the
> pointer to the matching entry or NULL if there’s no match.
>
> dmi\_setup must be called before this function is called.

const char \*dmi\_get\_system\_info(int field)
:   return DMI data value

**Parameters**

`int field`
:   data index (see `enum dmi_field`)

**Description**

> Returns one DMI data value, can be used to perform
> complex DMI data checks.

int dmi\_name\_in\_vendors(const char \*str)
:   Check if string is in the DMI system or board vendor name

**Parameters**

`const char *str`
:   Case sensitive Name

const struct dmi\_device \*dmi\_find\_device(int type, const char \*name, const struct dmi\_device \*from)
:   find onboard device by type/name

**Parameters**

`int type`
:   device type or `DMI_DEV_TYPE_ANY` to match all device types

`const char *name`
:   device name string or `NULL` to match all

`const struct dmi_device *from`
:   previous device found in search, or `NULL` for new search.

**Description**

> Iterates through the list of known onboard devices. If a device is
> found with a matching **type** and **name**, a pointer to its device
> structure is returned. Otherwise, `NULL` is returned.
> A new search is initiated by passing `NULL` as the **from** argument.
> If **from** is not `NULL`, searches continue from next device.

bool dmi\_get\_date(int field, int \*yearp, int \*monthp, int \*dayp)
:   parse a DMI date

**Parameters**

`int field`
:   data index (see `enum dmi_field`)

`int *yearp`
:   optional out parameter for the year

`int *monthp`
:   optional out parameter for the month

`int *dayp`
:   optional out parameter for the day

**Description**

> The date field is assumed to be in the form resembling
> [mm[/dd]]/yy[yy] and the result is stored in the out
> parameters any or all of which can be omitted.
>
> If the field doesn’t exist, all out parameters are set to zero
> and false is returned. Otherwise, true is returned with any
> invalid part of date set to zero.
>
> On return, year, month and day are guaranteed to be in the
> range of [0,9999], [0,12] and [0,31] respectively.

int dmi\_get\_bios\_year(void)
:   get a year out of DMI\_BIOS\_DATE field

**Parameters**

`void`
:   no arguments

**Description**

> Returns year on success, -ENXIO if DMI is not selected,
> or a different negative error code if DMI field is not present
> or not parseable.

int dmi\_walk(void (\*decode)(const struct dmi\_header\*, void\*), void \*private\_data)
:   Walk the DMI table and get called back for every record

**Parameters**

`void (*decode)(const struct dmi_header *, void *)`
:   Callback function

`void *private_data`
:   Private data to be passed to the callback function

**Description**

> Returns 0 on success, -ENXIO if DMI is not selected or not present,
> or a different negative error code if DMI walking fails.

bool dmi\_match(enum dmi\_field f, const char \*str)
:   compare a string to the dmi field (if exists)

**Parameters**

`enum dmi_field f`
:   DMI field identifier

`const char *str`
:   string to compare the DMI field to

**Description**

Returns true if the requested field equals to the str (including NULL).

u8 dmi\_memdev\_type(u16 handle)
:   get the memory type

**Parameters**

`u16 handle`
:   DMI structure handle

**Description**

Return the DMI memory type of the module in the slot associated with the
given DMI handle, or 0x0 if no such DMI handle exists.

u16 dmi\_memdev\_handle(int slot)
:   get the DMI handle of a memory slot

**Parameters**

`int slot`
:   slot number

**Description**

> Return the DMI handle associated with a given memory slot, or `0xFFFF`
> if there is no such slot.

## EDD Interfaces

ssize\_t edd\_show\_raw\_data(struct edd\_device \*edev, char \*buf)
:   copies raw data to buffer for userspace to parse

**Parameters**

`struct edd_device *edev`
:   target edd\_device

`char *buf`
:   output buffer

**Return**

number of bytes written, or -EINVAL on failure

void edd\_release(struct kobject \*kobj)
:   free edd structure

**Parameters**

`struct kobject * kobj`
:   kobject of edd structure

**Description**

> This is called when the refcount of the edd structure
> reaches 0. This should happen right after we unregister,
> but just in case, we use the release callback anyway.

int edd\_dev\_is\_type(struct edd\_device \*edev, const char \*type)
:   is this EDD device a ‘type’ device?

**Parameters**

`struct edd_device *edev`
:   target edd\_device

`const char *type`
:   a host bus or interface identifier string per the EDD spec

**Description**

Returns 1 (TRUE) if it is a ‘type’ device, 0 otherwise.

struct pci\_dev \*edd\_get\_pci\_dev(struct edd\_device \*edev)
:   finds pci\_dev that matches edev

**Parameters**

`struct edd_device *edev`
:   edd\_device

**Description**

Returns pci\_dev if found, or NULL

int edd\_init(void)
:   creates sysfs tree of EDD data

**Parameters**

`void`
:   no arguments

## Generic System Framebuffers Interface

void sysfb\_disable(struct [device](../infrastructure.html#c.device "device") \*dev)
:   disable the Generic System Framebuffers support

**Parameters**

`struct device *dev`
:   the device to check if non-NULL

**Description**

This disables the registration of system framebuffer devices that match the
generic drivers that make use of the system framebuffer set up by firmware.

It also unregisters a device if this was already registered by `sysfb_init()`.

**Context**

The function can sleep. A **disable\_lock** mutex is acquired to serialize
against `sysfb_init()`, that registers a system framebuffer device.

bool sysfb\_handles\_screen\_info(void)
:   reports if sysfb handles the global screen\_info

**Parameters**

`void`
:   no arguments

**Description**

Callers can use [`sysfb_handles_screen_info()`](#c.sysfb_handles_screen_info "sysfb_handles_screen_info") to determine whether the Generic
System Framebuffers (sysfb) can handle the global screen\_info data structure
or not. Drivers might need this information to know if they have to setup the
system framebuffer, or if they have to delegate this action to sysfb instead.

**Return**

True if sysfb handles the global screen\_info data structure.

## Intel Stratix10 SoC Service Layer

Some features of the Intel Stratix10 SoC require a level of privilege
higher than the kernel is granted. Such secure features include
FPGA programming. In terms of the ARMv8 architecture, the kernel runs
at Exception Level 1 (EL1), access to the features requires
Exception Level 3 (EL3).

The Intel Stratix10 SoC service layer provides an in kernel API for
drivers to request access to the secure features. The requests are queued
and processed one by one. ARM’s SMCCC is used to pass the execution
of the requests on to a secure monitor (EL3).

enum stratix10\_svc\_command\_code
:   supported service commands

**Constants**

`COMMAND_NOOP`
:   do ‘dummy’ request for integration/debug/trouble-shooting

`COMMAND_RECONFIG`
:   ask for FPGA configuration preparation, return status
    is SVC\_STATUS\_OK

`COMMAND_RECONFIG_DATA_SUBMIT`
:   submit buffer(s) of bit-stream data for the
    FPGA configuration, return status is SVC\_STATUS\_SUBMITTED or SVC\_STATUS\_ERROR

`COMMAND_RECONFIG_DATA_CLAIM`
:   check the status of the configuration, return
    status is SVC\_STATUS\_COMPLETED, or SVC\_STATUS\_BUSY, or SVC\_STATUS\_ERROR

`COMMAND_RECONFIG_STATUS`
:   check the status of the configuration, return
    status is SVC\_STATUS\_COMPLETED, or SVC\_STATUS\_BUSY, or SVC\_STATUS\_ERROR

`COMMAND_RSU_STATUS`
:   request remote system update boot log, return status
    is log data or SVC\_STATUS\_RSU\_ERROR

`COMMAND_RSU_UPDATE`
:   set the offset of the bitstream to boot after reboot,
    return status is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_RSU_NOTIFY`
:   report the status of hard processor system
    software to firmware, return status is SVC\_STATUS\_OK or
    SVC\_STATUS\_ERROR

`COMMAND_RSU_RETRY`
:   query firmware for the current image’s retry counter,
    return status is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_RSU_MAX_RETRY`
:   query firmware for the max retry value,
    return status is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_RSU_DCMF_VERSION`
:   query firmware for the DCMF version, return status
    is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_RSU_DCMF_STATUS`
:   query firmware for the DCMF status
    return status is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_FIRMWARE_VERSION`
:   query running firmware version, return status
    is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_RSU_GET_SPT_TABLE`
:   query firmware for SPT table
    return status is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_FCS_REQUEST_SERVICE`
:   request validation of image from firmware,
    return status is SVC\_STATUS\_OK, SVC\_STATUS\_INVALID\_PARAM

`COMMAND_FCS_SEND_CERTIFICATE`
:   send a certificate, return status is
    SVC\_STATUS\_OK, SVC\_STATUS\_INVALID\_PARAM, SVC\_STATUS\_ERROR

`COMMAND_FCS_GET_PROVISION_DATA`
:   read the provisioning data, return status is
    SVC\_STATUS\_OK, SVC\_STATUS\_INVALID\_PARAM, SVC\_STATUS\_ERROR

`COMMAND_FCS_DATA_ENCRYPTION`
:   encrypt the data, return status is
    SVC\_STATUS\_OK, SVC\_STATUS\_INVALID\_PARAM, SVC\_STATUS\_ERROR

`COMMAND_FCS_DATA_DECRYPTION`
:   decrypt the data, return status is
    SVC\_STATUS\_OK, SVC\_STATUS\_INVALID\_PARAM, SVC\_STATUS\_ERROR

`COMMAND_FCS_RANDOM_NUMBER_GEN`
:   generate a random number, return status
    is SVC\_STATUS\_OK, SVC\_STATUS\_ERROR

`COMMAND_POLL_SERVICE_STATUS`
:   poll if the service request is complete,
    return statis is SVC\_STATUS\_OK, SVC\_STATUS\_ERROR or SVC\_STATUS\_BUSY

`COMMAND_MBOX_SEND_CMD`
:   send generic mailbox command, return status is
    SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_SMC_SVC_VERSION`
:   Non-mailbox SMC SVC API Version,
    return status is SVC\_STATUS\_OK

`COMMAND_HWMON_READTEMP`
:   query the temperature from the hardware monitor,
    return status is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

`COMMAND_HWMON_READVOLT`
:   query the voltage from the hardware monitor,
    return status is SVC\_STATUS\_OK or SVC\_STATUS\_ERROR

struct stratix10\_svc\_client\_msg
:   message sent by client to service

**Definition**:

```
struct stratix10_svc_client_msg {
    void *payload;
    size_t payload_length;
    void *payload_output;
    size_t payload_length_output;
    enum stratix10_svc_command_code command;
    u64 arg[3];
};
```

**Members**

`payload`
:   starting address of data need be processed

`payload_length`
:   to be processed data size in bytes

`payload_output`
:   starting address of processed data

`payload_length_output`
:   processed data size in bytes

`command`
:   service command

`arg`
:   args to be passed via registers and not physically mapped buffers

struct stratix10\_svc\_command\_config\_type
:   config type

**Definition**:

```
struct stratix10_svc_command_config_type {
    u32 flags;
};
```

**Members**

`flags`
:   flag bit for the type of FPGA configuration

struct stratix10\_svc\_cb\_data
:   callback data structure from service layer

**Definition**:

```
struct stratix10_svc_cb_data {
    u32 status;
    void *kaddr1;
    void *kaddr2;
    void *kaddr3;
};
```

**Members**

`status`
:   the status of sent command

`kaddr1`
:   address of 1st completed data block

`kaddr2`
:   address of 2nd completed data block

`kaddr3`
:   address of 3rd completed data block

struct stratix10\_svc\_client
:   service client structure

**Definition**:

```
struct stratix10_svc_client {
    struct device *dev;
    void (*receive_cb)(struct stratix10_svc_client *client, struct stratix10_svc_cb_data *cb_data);
    void *priv;
};
```

**Members**

`dev`
:   the client device

`receive_cb`
:   callback to provide service client the received data

`priv`
:   client private data

struct stratix10\_svc\_chan \*stratix10\_svc\_request\_channel\_byname(struct [stratix10\_svc\_client](#c.stratix10_svc_client "stratix10_svc_client") \*client, const char \*name)
:   request a service channel

**Parameters**

`struct stratix10_svc_client *client`
:   pointer to service client

`const char *name`
:   service client name

**Description**

This function is used by service client to request a service channel.

**Return**

a pointer to channel assigned to the client on success,
or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

int stratix10\_svc\_add\_async\_client(struct stratix10\_svc\_chan \*chan, bool use\_unique\_clientid)
:   Add an asynchronous client to the Stratix10 service channel.

**Parameters**

`struct stratix10_svc_chan *chan`
:   Pointer to the Stratix10 service channel structure.

`bool use_unique_clientid`
:   Boolean flag indicating whether to use a
    unique client ID.

**Description**

This function adds an asynchronous client to the specified
Stratix10 service channel. If the use\_unique\_clientid flag is
set to true, a unique client ID is allocated for the asynchronous
channel. Otherwise, a common asynchronous channel is used.

**Return**

0 on success, or a negative error code on failure:
-EINVAL if the channel is NULL or the async controller is
not initialized.
-EOPNOTSUPP if async operations are not supported.
-EALREADY if the async channel is already allocated.
-ENOMEM if memory allocation fails.
Other negative values if ID allocation fails.

int stratix10\_svc\_remove\_async\_client(struct stratix10\_svc\_chan \*chan)
:   Remove an asynchronous client from the Stratix10 service channel.

**Parameters**

`struct stratix10_svc_chan *chan`
:   Pointer to the Stratix10 service channel structure.

**Description**

This function removes an asynchronous client associated with the
given service channel. It checks if the channel and the
asynchronous channel are valid, and then proceeds to decrement
the reference count for the common asynchronous channel if
applicable. If the reference count reaches zero, it destroys the
job ID pool and deallocates the asynchronous client ID. For
non-common asynchronous channels, it directly destroys the job ID
pool, deallocates the asynchronous client ID, and frees the
memory allocated for the asynchronous channel.

**Return**

0 on success, -EINVAL if the channel or asynchronous
channel is invalid.

int stratix10\_svc\_async\_send(struct stratix10\_svc\_chan \*chan, void \*msg, void \*\*handler, async\_callback\_t cb, void \*cb\_arg)
:   Send an asynchronous message to the Stratix10 service

**Parameters**

`struct stratix10_svc_chan *chan`
:   Pointer to the service channel structure

`void *msg`
:   Pointer to the message to be sent

`void **handler`
:   Pointer to the handler for the asynchronous message
    used by caller for later reference.

`async_callback_t cb`
:   Callback function to be called upon completion

`void *cb_arg`
:   Argument to be passed to the callback function

**Description**

This function sends an asynchronous message to the SDM mailbox in
EL3 secure firmware. It performs various checks and setups,
including allocating a job ID, setting up the transaction ID and
packaging it to El3 firmware. The function handles different
commands by setting up the appropriate arguments for the SMC call.
If the SMC call is successful, the handler is set up and the
function returns 0. If the SMC call fails, appropriate error
handling is performed along with cleanup of resources.

**Return**

0 on success, -EINVAL for invalid argument, -ENOMEM if
memory is not available, -EAGAIN if EL3 firmware is busy, -EBADF
if the message is rejected by EL3 firmware and -EIO on other
errors from EL3 firmware.

int stratix10\_svc\_async\_poll(struct stratix10\_svc\_chan \*chan, void \*tx\_handle, struct [stratix10\_svc\_cb\_data](#c.stratix10_svc_cb_data "stratix10_svc_cb_data") \*data)
:   Polls the status of an asynchronous transaction.

**Parameters**

`struct stratix10_svc_chan *chan`
:   Pointer to the service channel structure.

`void *tx_handle`
:   Handle to the transaction being polled.

`struct stratix10_svc_cb_data *data`
:   Pointer to the callback data structure.

**Description**

This function polls the status of an asynchronous transaction
identified by the given transaction handle. It ensures that the
necessary structures are initialized and valid before proceeding
with the poll operation. The function sets up the necessary
arguments for the SMC call, invokes the call, and prepares the
response data if the call is successful. If the call fails, the
function returns the error mapped to the SVC status error.

**Return**

0 on success, -EINVAL if any input parameter is invalid,
-EAGAIN if the transaction is still in progress,
-EPERM if the command is invalid, or other negative
error codes on failure.

int stratix10\_svc\_async\_done(struct stratix10\_svc\_chan \*chan, void \*tx\_handle)
:   Completes an asynchronous transaction.

**Parameters**

`struct stratix10_svc_chan *chan`
:   Pointer to the service channel structure.

`void *tx_handle`
:   Handle to the transaction being completed.

**Description**

This function completes an asynchronous transaction identified by
the given transaction handle. It ensures that the necessary
structures are initialized and valid before proceeding with the
completion operation. The function deallocates the transaction ID,
frees the memory allocated for the handler, and removes the handler
from the transaction list.

**Return**

0 on success, -EINVAL if any input parameter is invalid,
or other negative error codes on failure.

void stratix10\_svc\_free\_channel(struct stratix10\_svc\_chan \*chan)
:   free service channel

**Parameters**

`struct stratix10_svc_chan *chan`
:   service channel to be freed

**Description**

This function is used by service client to free a service channel.

int stratix10\_svc\_send(struct stratix10\_svc\_chan \*chan, void \*msg)
:   send a message data to the remote

**Parameters**

`struct stratix10_svc_chan *chan`
:   service channel assigned to the client

`void *msg`
:   message data to be sent, in the format of
    “[`struct stratix10_svc_client_msg`](#c.stratix10_svc_client_msg "stratix10_svc_client_msg")”

**Description**

This function is used by service client to add a message to the service
layer driver’s queue for being sent to the secure world.

**Return**

0 for success, -ENOMEM or -ENOBUFS on error.

void stratix10\_svc\_done(struct stratix10\_svc\_chan \*chan)
:   complete service request transactions

**Parameters**

`struct stratix10_svc_chan *chan`
:   service channel assigned to the client

**Description**

This function should be called when client has finished its request
or there is an error in the request process. It allows the service layer
to stop the running thread to have maximize savings in kernel resources.

void \*stratix10\_svc\_allocate\_memory(struct stratix10\_svc\_chan \*chan, size\_t size)
:   allocate memory

**Parameters**

`struct stratix10_svc_chan *chan`
:   service channel assigned to the client

`size_t size`
:   memory size requested by a specific service client

**Description**

Service layer allocates the requested number of bytes buffer from the
memory pool, service client uses this function to get allocated buffers.

**Return**

address of allocated memory on success, or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

void stratix10\_svc\_free\_memory(struct stratix10\_svc\_chan \*chan, void \*kaddr)
:   free allocated memory

**Parameters**

`struct stratix10_svc_chan *chan`
:   service channel assigned to the client

`void *kaddr`
:   memory to be freed

**Description**

This function is used by service client to free allocated buffers.
