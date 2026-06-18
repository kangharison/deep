# User-Space EC Interface (cdev)

> 출처(원문): https://docs.kernel.org/driver-api/surface_aggregator/clients/cdev.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [User-Space EC Interface (cdev)](#id2)

The `surface_aggregator_cdev` module provides a misc-device for the SSAM
controller to allow for a (more or less) direct connection from user-space to
the SAM EC. It is intended to be used for development and debugging, and
therefore should not be used or relied upon in any other way. Note that this
module is not loaded automatically, but instead must be loaded manually.

The provided interface is accessible through the `/dev/surface/aggregator`
device-file. All functionality of this interface is provided via IOCTLs.
These IOCTLs and their respective input/output parameter structs are defined in
`include/uapi/linux/surface_aggregator/cdev.h`.

A small python library and scripts for accessing this interface can be found
at <https://github.com/linux-surface/surface-aggregator-module/tree/master/scripts/ssam>.

## [Receiving Events](#id3)

Events can be received by reading from the device-file. The are represented by
the [`struct ssam_cdev_event`](#c.ssam_cdev_event "ssam_cdev_event") datatype.

Before events are available to be read, however, the desired notifiers must be
registered via the `SSAM_CDEV_NOTIF_REGISTER` IOCTL. Notifiers are, in
essence, callbacks, called when the EC sends an event. They are, in this
interface, associated with a specific target category and device-file-instance.
They forward any event of this category to the buffer of the corresponding
instance, from which it can then be read.

Notifiers themselves do not enable events on the EC. Thus, it may additionally
be necessary to enable events via the `SSAM_CDEV_EVENT_ENABLE` IOCTL. While
notifiers work per-client (i.e. per-device-file-instance), events are enabled
globally, for the EC and all of its clients (regardless of userspace or
non-userspace). The `SSAM_CDEV_EVENT_ENABLE` and `SSAM_CDEV_EVENT_DISABLE`
IOCTLs take care of reference counting the events, such that an event is
enabled as long as there is a client that has requested it.

Note that enabled events are not automatically disabled once the client
instance is closed. Therefore any client process (or group of processes) should
balance their event enable calls with the corresponding event disable calls. It
is, however, perfectly valid to enable and disable events on different client
instances. For example, it is valid to set up notifiers and read events on
client instance `A`, enable those events on instance `B` (note that these
will also be received by A since events are enabled/disabled globally), and
after no more events are desired, disable the previously enabled events via
instance `C`.

## [Controller IOCTLs](#id4)

The following IOCTLs are provided:

Controller IOCTLs

| Type | Number | Direction | Name | Description |
| --- | --- | --- | --- | --- |
| `0xA5` | `1` | `WR` | `REQUEST` | Perform synchronous SAM request. |
| `0xA5` | `2` | `W` | `NOTIF_REGISTER` | Register event notifier. |
| `0xA5` | `3` | `W` | `NOTIF_UNREGISTER` | Unregister event notifier. |
| `0xA5` | `4` | `W` | `EVENT_ENABLE` | Enable event source. |
| `0xA5` | `5` | `W` | `EVENT_DISABLE` | Disable event source. |

### [`SSAM_CDEV_REQUEST`](#id5)

Defined as `_IOWR(0xA5, 1, struct ssam_cdev_request)`.

Executes a synchronous SAM request. The request specification is passed in
as argument of type [`struct ssam_cdev_request`](#c.ssam_cdev_request "ssam_cdev_request"), which is then written to/modified
by the IOCTL to return status and result of the request.

Request payload data must be allocated separately and is passed in via the
`payload.data` and `payload.length` members. If a response is required,
the response buffer must be allocated by the caller and passed in via the
`response.data` member. The `response.length` member must be set to the
capacity of this buffer, or if no response is required, zero. Upon
completion of the request, the call will write the response to the response
buffer (if its capacity allows it) and overwrite the length field with the
actual size of the response, in bytes.

Additionally, if the request has a response, this must be indicated via the
request flags, as is done with in-kernel requests. Request flags can be set
via the `flags` member and the values correspond to the values found in
[`enum ssam_cdev_request_flags`](#c.ssam_cdev_request_flags "ssam_cdev_request_flags").

Finally, the status of the request itself is returned in the `status`
member (a negative errno value indicating failure). Note that failure
indication of the IOCTL is separated from failure indication of the request:
The IOCTL returns a negative status code if anything failed during setup of
the request (`-EFAULT`) or if the provided argument or any of its fields
are invalid (`-EINVAL`). In this case, the status value of the request
argument may be set, providing more detail on what went wrong (e.g.
`-ENOMEM` for out-of-memory), but this value may also be zero. The IOCTL
will return with a zero status code in case the request has been set up,
submitted, and completed (i.e. handed back to user-space) successfully from
inside the IOCTL, but the request `status` member may still be negative in
case the actual execution of the request failed after it has been submitted.

A full definition of the argument `struct is` provided below.

### [`SSAM_CDEV_NOTIF_REGISTER`](#id6)

Defined as `_IOW(0xA5, 2, struct ssam_cdev_notifier_desc)`.

Register a notifier for the event target category specified in the given
notifier description with the specified priority. Notifiers registration is
required to receive events, but does not enable events themselves. After a
notifier for a specific target category has been registered, all events of that
category will be forwarded to the userspace client and can then be read from
the device file instance. Note that events may have to be enabled, e.g. via the
`SSAM_CDEV_EVENT_ENABLE` IOCTL, before the EC will send them.

Only one notifier can be registered per target category and client instance. If
a notifier has already been registered, this IOCTL will fail with `-EEXIST`.

Notifiers will automatically be removed when the device file instance is
closed.

### [`SSAM_CDEV_NOTIF_UNREGISTER`](#id7)

Defined as `_IOW(0xA5, 3, struct ssam_cdev_notifier_desc)`.

Unregisters the notifier associated with the specified target category. The
priority field will be ignored by this IOCTL. If no notifier has been
registered for this client instance and the given category, this IOCTL will
fail with `-ENOENT`.

### [`SSAM_CDEV_EVENT_ENABLE`](#id8)

Defined as `_IOW(0xA5, 4, struct ssam_cdev_event_desc)`.

Enable the event associated with the given event descriptor.

Note that this call will not register a notifier itself, it will only enable
events on the controller. If you want to receive events by reading from the
device file, you will need to register the corresponding notifier(s) on that
instance.

Events are not automatically disabled when the device file is closed. This must
be done manually, via a call to the `SSAM_CDEV_EVENT_DISABLE` IOCTL.

### [`SSAM_CDEV_EVENT_DISABLE`](#id9)

Defined as `_IOW(0xA5, 5, struct ssam_cdev_event_desc)`.

Disable the event associated with the given event descriptor.

Note that this will not unregister any notifiers. Events may still be received
and forwarded to user-space after this call. The only safe way of stopping
events from being received is unregistering all previously registered
notifiers.

## [Structures and Enums](#id10)

enum ssam\_cdev\_request\_flags
:   Request flags for SSAM cdev request IOCTL.

**Constants**

`SSAM_CDEV_REQUEST_HAS_RESPONSE`
:   Specifies that the request expects a response. If not set, the request
    will be directly completed after its underlying packet has been
    transmitted. If set, the request transport system waits for a response
    of the request.

`SSAM_CDEV_REQUEST_UNSEQUENCED`
:   Specifies that the request should be transmitted via an unsequenced
    packet. If set, the request must not have a response, meaning that this
    flag and the `SSAM_CDEV_REQUEST_HAS_RESPONSE` flag are mutually
    exclusive.

struct ssam\_cdev\_request
:   Controller request IOCTL argument.

**Definition**:

```
struct ssam_cdev_request {
    __u8 target_category;
    __u8 target_id;
    __u8 command_id;
    __u8 instance_id;
    __u16 flags;
    __s16 status;
    struct {
        __u64 data;
        __u16 length;
        __u8 __pad[6];
    } payload;
    struct {
        __u64 data;
        __u16 length;
        __u8 __pad[6];
    } response;
};
```

**Members**

`target_category`
:   Target category of the SAM request.

`target_id`
:   Target ID of the SAM request.

`command_id`
:   Command ID of the SAM request.

`instance_id`
:   Instance ID of the SAM request.

`flags`
:   Request flags (see [`enum ssam_cdev_request_flags`](#c.ssam_cdev_request_flags "ssam_cdev_request_flags")).

`status`
:   Request status (output).

`payload`
:   Request payload (input data).

`payload.data`
:   Pointer to request payload data.

`payload.length`
:   Length of request payload data (in bytes).

`response`
:   Request response (output data).

`response.data`
:   Pointer to response buffer.

`response.length`
:   On input: Capacity of response buffer (in bytes).
    On output: Length of request response (number of bytes
    in the buffer that are actually used).

struct ssam\_cdev\_notifier\_desc
:   Notifier descriptor.

**Definition**:

```
struct ssam_cdev_notifier_desc {
    __s32 priority;
    __u8 target_category;
};
```

**Members**

`priority`
:   Priority value determining the order in which notifier
    callbacks will be called. A higher value means higher
    priority, i.e. the associated callback will be executed
    earlier than other (lower priority) callbacks.

`target_category`
:   The event target category for which this notifier should
    receive events.

**Description**

Specifies the notifier that should be registered or unregistered,
specifically with which priority and for which target category of events.

struct ssam\_cdev\_event\_desc
:   Event descriptor.

**Definition**:

```
struct ssam_cdev_event_desc {
    struct {
        __u8 target_category;
        __u8 target_id;
        __u8 cid_enable;
        __u8 cid_disable;
    } reg;
    struct {
        __u8 target_category;
        __u8 instance;
    } id;
    __u8 flags;
};
```

**Members**

`reg`
:   Registry via which the event will be enabled/disabled.

`reg.target_category`
:   Target category for the event registry requests.

`reg.target_id`
:   Target ID for the event registry requests.

`reg.cid_enable`
:   Command ID for the event-enable request.

`reg.cid_disable`
:   Command ID for the event-disable request.

`id`
:   ID specifying the event.

`id.target_category`
:   Target category of the event source.

`id.instance`
:   Instance ID of the event source.

`flags`
:   Flags used for enabling the event.

**Description**

Specifies which event should be enabled/disabled and how to do that.

struct ssam\_cdev\_event
:   SSAM event sent by the EC.

**Definition**:

```
struct ssam_cdev_event {
    __u8 target_category;
    __u8 target_id;
    __u8 command_id;
    __u8 instance_id;
    __u16 length;
    __u8 data[];
};
```

**Members**

`target_category`
:   Target category of the event source. See `enum ssam_ssh_tc`.

`target_id`
:   Target ID of the event source.

`command_id`
:   Command ID of the event.

`instance_id`
:   Instance ID of the event source.

`length`
:   Length of the event payload in bytes.

`data`
:   Event payload data.
