# Client Driver API Documentation

> 출처(원문): https://docs.kernel.org/driver-api/surface_aggregator/client-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Client Driver API Documentation](#id1)

## [Serial Hub Communication](#id2)

enum ssh\_frame\_type
:   Frame types for SSH frames.

**Constants**

`SSH_FRAME_TYPE_DATA_SEQ`
:   Indicates a data frame, followed by a payload with the length specified
    in the `struct ssh_frame.len` field. This frame is sequenced, meaning
    that an ACK is required.

`SSH_FRAME_TYPE_DATA_NSQ`
:   Same as `SSH_FRAME_TYPE_DATA_SEQ`, but unsequenced, meaning that the
    message does not have to be ACKed.

`SSH_FRAME_TYPE_ACK`
:   Indicates an ACK message.

`SSH_FRAME_TYPE_NAK`
:   Indicates an error response for previously sent frame. In general, this
    means that the frame and/or payload is malformed, e.g. a CRC is wrong.
    For command-type payloads, this can also mean that the command is
    invalid.

struct ssh\_frame
:   SSH communication frame.

**Definition**:

```
struct ssh_frame {
    u8 type;
    __le16 len;
    u8 seq;
};
```

**Members**

`type`
:   The type of the frame. See [`enum ssh_frame_type`](#c.ssh_frame_type "ssh_frame_type").

`len`
:   The length of the frame payload directly following the CRC for this
    frame. Does not include the final CRC for that payload.

`seq`
:   The sequence number for this message/exchange.

enum ssh\_payload\_type
:   Type indicator for the SSH payload.

**Constants**

`SSH_PLD_TYPE_CMD`
:   The payload is a command structure with optional command
    payload.

struct ssh\_command
:   Payload of a command-type frame.

**Definition**:

```
struct ssh_command {
    u8 type;
    u8 tc;
    u8 tid;
    u8 sid;
    u8 iid;
    __le16 rqid;
    u8 cid;
};
```

**Members**

`type`
:   The type of the payload. See [`enum ssh_payload_type`](#c.ssh_payload_type "ssh_payload_type"). Should be
    SSH\_PLD\_TYPE\_CMD for this struct.

`tc`
:   Command target category.

`tid`
:   Target ID. Indicates the target of the message.

`sid`
:   Source ID. Indicates the source of the message.

`iid`
:   Instance ID.

`rqid`
:   Request ID. Used to match requests with responses and differentiate
    between responses and events.

`cid`
:   Command ID.

SSH\_MESSAGE\_LENGTH

`SSH_MESSAGE_LENGTH (payload_size)`

> Compute length of SSH message.

**Parameters**

`payload_size`
:   Length of the payload inside the SSH frame.

**Return**

Returns the length of a SSH message with payload of specified size.

SSH\_COMMAND\_MESSAGE\_LENGTH

`SSH_COMMAND_MESSAGE_LENGTH (payload_size)`

> Compute length of SSH command message.

**Parameters**

`payload_size`
:   Length of the command payload.

**Return**

Returns the length of a SSH command message with command payload of
specified size.

SSH\_MSGOFFSET\_FRAME

`SSH_MSGOFFSET_FRAME (field)`

> Compute offset in SSH message to specified field in frame.

**Parameters**

`field`
:   The field for which the offset should be computed.

**Return**

Returns the offset of the specified [`struct ssh_frame`](#c.ssh_frame "ssh_frame") field in the
raw SSH message data as. Takes SYN bytes (u16) preceding the frame into
account.

SSH\_MSGOFFSET\_COMMAND

`SSH_MSGOFFSET_COMMAND (field)`

> Compute offset in SSH message to specified field in command.

**Parameters**

`field`
:   The field for which the offset should be computed.

**Return**

Returns the offset of the specified [`struct ssh_command`](#c.ssh_command "ssh_command") field in
the raw SSH message data. Takes SYN bytes (u16) preceding the frame and the
frame CRC (u16) between frame and command into account.

u16 ssh\_crc(const u8 \*buf, size\_t len)
:   Compute CRC for SSH messages.

**Parameters**

`const u8 *buf`
:   The pointer pointing to the data for which the CRC should be computed.

`size_t len`
:   The length of the data for which the CRC should be computed.

**Return**

Returns the CRC computed on the provided data, as used for SSH
messages.

u16 ssh\_rqid\_next\_valid(u16 rqid)
:   Return the next valid request ID.

**Parameters**

`u16 rqid`
:   The current request ID.

**Return**

Returns the next valid request ID, following the current request ID
provided to this function. This function skips any request IDs reserved for
events.

u16 ssh\_rqid\_to\_event(u16 rqid)
:   Convert request ID to its corresponding event ID.

**Parameters**

`u16 rqid`
:   The request ID to convert.

bool ssh\_rqid\_is\_event(u16 rqid)
:   Check if given request ID is a valid event ID.

**Parameters**

`u16 rqid`
:   The request ID to check.

u16 ssh\_tc\_to\_rqid(u8 tc)
:   Convert target category to its corresponding request ID.

**Parameters**

`u8 tc`
:   The target category to convert.

u8 ssh\_tid\_to\_index(u8 tid)
:   Convert target ID to its corresponding target index.

**Parameters**

`u8 tid`
:   The target ID to convert.

bool ssh\_tid\_is\_valid(u8 tid)
:   Check if target ID is valid/supported.

**Parameters**

`u8 tid`
:   The target ID to check.

struct ssam\_span
:   Reference to a buffer region.

**Definition**:

```
struct ssam_span {
    u8 *ptr;
    size_t len;
};
```

**Members**

`ptr`
:   Pointer to the buffer region.

`len`
:   Length of the buffer region.

**Description**

A reference to a (non-owned) buffer segment, consisting of pointer and
length. Use of this `struct indicates` non-owned data, i.e. data of which the
life-time is managed (i.e. it is allocated/freed) via another pointer.

enum ssam\_ssh\_tid
:   Target/source IDs for Serial Hub messages.

**Constants**

`SSAM_SSH_TID_HOST`
:   We as the kernel Serial Hub driver.

`SSAM_SSH_TID_SAM`
:   The Surface Aggregator EC.

`SSAM_SSH_TID_KIP`
:   Keyboard and perihperal controller.

`SSAM_SSH_TID_DEBUG`
:   Debug connector.

`SSAM_SSH_TID_SURFLINK`
:   SurfLink connector.

enum ssh\_packet\_base\_priority
:   Base priorities for [`struct ssh_packet`](#c.ssh_packet "ssh_packet").

**Constants**

`SSH_PACKET_PRIORITY_FLUSH`
:   Base priority for flush packets.

`SSH_PACKET_PRIORITY_DATA`
:   Base priority for normal data packets.

`SSH_PACKET_PRIORITY_NAK`
:   Base priority for NAK packets.

`SSH_PACKET_PRIORITY_ACK`
:   Base priority for ACK packets.

SSH\_PACKET\_PRIORITY

`SSH_PACKET_PRIORITY (base, try)`

> Compute packet priority from base priority and number of tries.

**Parameters**

`base`
:   The base priority as suffix of [`enum ssh_packet_base_priority`](#c.ssh_packet_base_priority "ssh_packet_base_priority"), e.g.
    `FLUSH`, `DATA`, `ACK`, or `NAK`.

`try`
:   The number of tries (must be less than 16).

**Description**

Compute the combined packet priority. The combined priority is dominated by
the base priority, whereas the number of (re-)tries decides the precedence
of packets with the same base priority, giving higher priority to packets
that already have more tries.

**Return**

Returns the computed priority as value fitting inside a `u8`. A
higher number means a higher priority.

u8 ssh\_packet\_priority\_get\_try(u8 priority)
:   Get number of tries from packet priority.

**Parameters**

`u8 priority`
:   The packet priority.

**Return**

Returns the number of tries encoded in the specified packet
priority.

u8 ssh\_packet\_priority\_get\_base(u8 priority)
:   Get base priority from packet priority.

**Parameters**

`u8 priority`
:   The packet priority.

**Return**

Returns the base priority encoded in the given packet priority.

struct ssh\_packet\_ops
:   Callback operations for a SSH packet.

**Definition**:

```
struct ssh_packet_ops {
    void (*release)(struct ssh_packet *p);
    void (*complete)(struct ssh_packet *p, int status);
};
```

**Members**

`release`
:   Function called when the packet reference count reaches zero.
    This callback must be relied upon to ensure that the packet has
    left the transport system(s).

`complete`
:   Function called when the packet is completed, either with
    success or failure. In case of failure, the reason for the
    failure is indicated by the value of the provided status code
    argument. This value will be zero in case of success. Note that
    a call to this callback does not guarantee that the packet is
    not in use by the transport system any more.

struct ssh\_packet
:   SSH transport packet.

**Definition**:

```
struct ssh_packet {
    struct ssh_ptl *ptl;
    struct kref refcnt;
    u8 priority;
    struct {
        size_t len;
        u8 *ptr;
    } data;
    unsigned long state;
    ktime_t timestamp;
    struct list_head queue_node;
    struct list_head pending_node;
    const struct ssh_packet_ops *ops;
};
```

**Members**

`ptl`
:   Pointer to the packet transport layer. May be `NULL` if the packet
    (or enclosing request) has not been submitted yet.

`refcnt`
:   Reference count of the packet.

`priority`
:   Priority of the packet. Must be computed via
    [`SSH_PACKET_PRIORITY()`](#c.SSH_PACKET_PRIORITY "SSH_PACKET_PRIORITY"). Must only be accessed while holding the
    queue lock after first submission.

`data`
:   Raw message data.

`data.len`
:   Length of the raw message data.

`data.ptr`
:   Pointer to the raw message data buffer.

`state`
:   State and type flags describing current packet state (dynamic)
    and type (static). See `enum ssh_packet_flags` for possible
    options.

`timestamp`
:   Timestamp specifying when the latest transmission of a
    currently pending packet has been started. May be `KTIME_MAX`
    before or in-between transmission attempts. Used for the packet
    timeout implementation. Must only be accessed while holding the
    pending lock after first submission.

`queue_node`
:   The list node for the packet queue.

`pending_node`
:   The list node for the set of pending packets.

`ops`
:   Packet operations.

void ssh\_packet\_set\_data(struct [ssh\_packet](#c.ssh_packet "ssh_packet") \*p, u8 \*ptr, size\_t len)
:   Set raw message data of packet.

**Parameters**

`struct ssh_packet *p`
:   The packet for which the message data should be set.

`u8 *ptr`
:   Pointer to the memory holding the message data.

`size_t len`
:   Length of the message data.

**Description**

Sets the raw message data buffer of the packet to the provided memory. The
memory is not copied. Instead, the caller is responsible for management
(i.e. allocation and deallocation) of the memory. The caller must ensure
that the provided memory is valid and contains a valid SSH message,
starting from the time of submission of the packet until the `release`
callback has been called. During this time, the memory may not be altered
in any way.

struct ssh\_request\_ops
:   Callback operations for a SSH request.

**Definition**:

```
struct ssh_request_ops {
    void (*release)(struct ssh_request *rqst);
    void (*complete)(struct ssh_request *rqst, const struct ssh_command *cmd, const struct ssam_span *data, int status);
};
```

**Members**

`release`
:   Function called when the request’s reference count reaches zero.
    This callback must be relied upon to ensure that the request has
    left the transport systems (both, packet an request systems).

`complete`
:   Function called when the request is completed, either with
    success or failure. The command data for the request response
    is provided via the [`struct ssh_command`](#c.ssh_command "ssh_command") parameter (`cmd`),
    the command payload of the request response via the `struct
    ssh_span` parameter (`data`).

**Description**

> If the request does not have any response or has not been
> completed with success, both `cmd` and `data` parameters will
> be NULL. If the request response does not have any command
> payload, the `data` span will be an empty (zero-length) span.
>
> In case of failure, the reason for the failure is indicated by
> the value of the provided status code argument (`status`). This
> value will be zero in case of success and a regular errno
> otherwise.
>
> Note that a call to this callback does not guarantee that the
> request is not in use by the transport systems any more.

struct ssh\_request
:   SSH transport request.

**Definition**:

```
struct ssh_request {
    struct ssh_packet packet;
    struct list_head node;
    unsigned long state;
    ktime_t timestamp;
    const struct ssh_request_ops *ops;
};
```

**Members**

`packet`
:   The underlying SSH transport packet.

`node`
:   List node for the request queue and pending set.

`state`
:   State and type flags describing current request state (dynamic)
    and type (static). See `enum ssh_request_flags` for possible
    options.

`timestamp`
:   Timestamp specifying when we start waiting on the response of
    the request. This is set once the underlying packet has been
    completed and may be `KTIME_MAX` before that, or when the request
    does not expect a response. Used for the request timeout
    implementation.

`ops`
:   Request Operations.

struct [ssh\_request](#c.ssh_request "ssh_request") \*to\_ssh\_request(struct [ssh\_packet](#c.ssh_packet "ssh_packet") \*p)
:   Cast a SSH packet to its enclosing SSH request.

**Parameters**

`struct ssh_packet *p`
:   The packet to cast.

**Description**

Casts the given [`struct ssh_packet`](#c.ssh_packet "ssh_packet") to its enclosing [`struct ssh_request`](#c.ssh_request "ssh_request").
The caller is responsible for making sure that the packet is actually
wrapped in a [`struct ssh_request`](#c.ssh_request "ssh_request").

**Return**

Returns the [`struct ssh_request`](#c.ssh_request "ssh_request") wrapping the provided packet.

struct [ssh\_request](#c.ssh_request "ssh_request") \*ssh\_request\_get(struct [ssh\_request](#c.ssh_request "ssh_request") \*r)
:   Increment reference count of request.

**Parameters**

`struct ssh_request *r`
:   The request to increment the reference count of.

**Description**

Increments the reference count of the given request by incrementing the
reference count of the underlying [`struct ssh_packet`](#c.ssh_packet "ssh_packet"), enclosed in it.

See also [`ssh_request_put()`](#c.ssh_request_put "ssh_request_put"), [`ssh_packet_get()`](#c.ssh_packet_get "ssh_packet_get").

**Return**

Returns the request provided as input.

void ssh\_request\_put(struct [ssh\_request](#c.ssh_request "ssh_request") \*r)
:   Decrement reference count of request.

**Parameters**

`struct ssh_request *r`
:   The request to decrement the reference count of.

**Description**

Decrements the reference count of the given request by decrementing the
reference count of the underlying [`struct ssh_packet`](#c.ssh_packet "ssh_packet"), enclosed in it. If
the reference count reaches zero, the `release` callback specified in the
request’s [`struct ssh_request_ops`](#c.ssh_request_ops "ssh_request_ops"), i.e. `r->ops->release`, will be
called.

See also [`ssh_request_get()`](#c.ssh_request_get "ssh_request_get"), [`ssh_packet_put()`](#c.ssh_packet_put "ssh_packet_put").

void ssh\_request\_set\_data(struct [ssh\_request](#c.ssh_request "ssh_request") \*r, u8 \*ptr, size\_t len)
:   Set raw message data of request.

**Parameters**

`struct ssh_request *r`
:   The request for which the message data should be set.

`u8 *ptr`
:   Pointer to the memory holding the message data.

`size_t len`
:   Length of the message data.

**Description**

Sets the raw message data buffer of the underlying packet to the specified
buffer. Does not copy the actual message data, just sets the buffer pointer
and length. Refer to [`ssh_packet_set_data()`](#c.ssh_packet_set_data "ssh_packet_set_data") for more details.

struct [ssh\_packet](#c.ssh_packet "ssh_packet") \*ssh\_packet\_get(struct [ssh\_packet](#c.ssh_packet "ssh_packet") \*packet)
:   Increment reference count of packet.

**Parameters**

`struct ssh_packet *packet`
:   The packet to increment the reference count of.

**Description**

Increments the reference count of the given packet. See [`ssh_packet_put()`](#c.ssh_packet_put "ssh_packet_put")
for the counter-part of this function.

**Return**

Returns the packet provided as input.

void ssh\_packet\_put(struct [ssh\_packet](#c.ssh_packet "ssh_packet") \*packet)
:   Decrement reference count of packet.

**Parameters**

`struct ssh_packet *packet`
:   The packet to decrement the reference count of.

**Description**

If the reference count reaches zero, the `release` callback specified in
the packet’s [`struct ssh_packet_ops`](#c.ssh_packet_ops "ssh_packet_ops"), i.e. `packet->ops->release`, will be
called.

See [`ssh_packet_get()`](#c.ssh_packet_get "ssh_packet_get") for the counter-part of this function.

## [Controller and Core Interface](#id3)

enum ssam\_event\_flags
:   Flags for enabling/disabling SAM events

**Constants**

`SSAM_EVENT_SEQUENCED`
:   The event will be sent via a sequenced data frame.

struct ssam\_event
:   SAM event sent from the EC to the host.

**Definition**:

```
struct ssam_event {
    u8 target_category;
    u8 target_id;
    u8 command_id;
    u8 instance_id;
    u16 length;
    u8 data[];
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

enum ssam\_request\_flags
:   Flags for SAM requests.

**Constants**

`SSAM_REQUEST_HAS_RESPONSE`
:   Specifies that the request expects a response. If not set, the request
    will be directly completed after its underlying packet has been
    transmitted. If set, the request transport system waits for a response
    of the request.

`SSAM_REQUEST_UNSEQUENCED`
:   Specifies that the request should be transmitted via an unsequenced
    packet. If set, the request must not have a response, meaning that this
    flag and the `SSAM_REQUEST_HAS_RESPONSE` flag are mutually exclusive.

struct ssam\_request
:   SAM request description.

**Definition**:

```
struct ssam_request {
    u8 target_category;
    u8 target_id;
    u8 command_id;
    u8 instance_id;
    u16 flags;
    u16 length;
    const u8 *payload;
};
```

**Members**

`target_category`
:   Category of the request’s target. See `enum ssam_ssh_tc`.

`target_id`
:   ID of the request’s target.

`command_id`
:   Command ID of the request.

`instance_id`
:   Instance ID of the request’s target.

`flags`
:   Flags for the request. See [`enum ssam_request_flags`](#c.ssam_request_flags "ssam_request_flags").

`length`
:   Length of the request payload in bytes.

`payload`
:   Request payload data.

**Description**

This `struct fully` describes a SAM request with payload. It is intended to
help set up the actual transport struct, e.g. [`struct ssam_request_sync`](#c.ssam_request_sync "ssam_request_sync"),
and specifically its raw message data via [`ssam_request_write_data()`](#c.ssam_request_write_data "ssam_request_write_data").

struct ssam\_response
:   Response buffer for SAM request.

**Definition**:

```
struct ssam_response {
    size_t capacity;
    size_t length;
    u8 *pointer;
};
```

**Members**

`capacity`
:   Capacity of the buffer, in bytes.

`length`
:   Length of the actual data stored in the memory pointed to by
    **pointer**, in bytes. Set by the transport system.

`pointer`
:   Pointer to the buffer’s memory, storing the response payload data.

struct ssam\_request\_sync
:   Synchronous SAM request struct.

**Definition**:

```
struct ssam_request_sync {
    struct ssh_request base;
    struct completion comp;
    struct ssam_response *resp;
    int status;
};
```

**Members**

`base`
:   Underlying SSH request.

`comp`
:   Completion used to signal full completion of the request. After the
    request has been submitted, this `struct may` only be modified or
    deallocated after the completion has been signaled.
    request has been submitted,

`resp`
:   Buffer to store the response.

`status`
:   Status of the request, set after the base request has been
    completed or has failed.

void ssam\_request\_sync\_set\_data(struct [ssam\_request\_sync](#c.ssam_request_sync "ssam_request_sync") \*rqst, u8 \*ptr, size\_t len)
:   Set message data of a synchronous request.

**Parameters**

`struct ssam_request_sync *rqst`
:   The request.

`u8 *ptr`
:   Pointer to the request message data.

`size_t len`
:   Length of the request message data.

**Description**

Set the request message data of a synchronous request. The provided buffer
needs to live until the request has been completed.

void ssam\_request\_sync\_set\_resp(struct [ssam\_request\_sync](#c.ssam_request_sync "ssam_request_sync") \*rqst, struct [ssam\_response](#c.ssam_response "ssam_response") \*resp)
:   Set response buffer of a synchronous request.

**Parameters**

`struct ssam_request_sync *rqst`
:   The request.

`struct ssam_response *resp`
:   The response buffer.

**Description**

Sets the response buffer of a synchronous request. This buffer will store
the response of the request after it has been completed. May be `NULL` if no
response is expected.

int ssam\_request\_sync\_wait(struct [ssam\_request\_sync](#c.ssam_request_sync "ssam_request_sync") \*rqst)
:   Wait for completion of a synchronous request.

**Parameters**

`struct ssam_request_sync *rqst`
:   The request to wait for.

**Description**

Wait for completion and release of a synchronous request. After this
function terminates, the request is guaranteed to have left the transport
system. After successful submission of a request, this function must be
called before accessing the response of the request, freeing the request,
or freeing any of the buffers associated with the request.

This function must not be called if the request has not been submitted yet
and may lead to a deadlock/infinite wait if a subsequent request submission
fails in that case, due to the completion never triggering.

**Return**

Returns the status of the given request, which is set on completion
of the packet. This value is zero on success and negative on failure.

ssam\_request\_do\_sync\_onstack

`ssam_request_do_sync_onstack (ctrl, rqst, rsp, payload_len)`

> Execute a synchronous request on the stack.

**Parameters**

`ctrl`
:   The controller via which the request is submitted.

`rqst`
:   The request specification.

`rsp`
:   The response buffer.

`payload_len`
:   The (maximum) request payload length.

**Description**

Allocates a synchronous request with specified payload length on the stack,
fully initializes it via the provided request specification, submits it,
and finally waits for its completion before returning its status. This
helper macro essentially allocates the request message buffer on the stack
and then calls [`ssam_request_do_sync_with_buffer()`](#c.ssam_request_do_sync_with_buffer "ssam_request_do_sync_with_buffer").

**Note**

The **payload\_len** parameter specifies the maximum payload length, used
for buffer allocation. The actual payload length may be smaller.

**Return**

Returns the status of the request or any failure during setup, i.e.
zero on success and a negative value on failure.

\_\_ssam\_retry

`__ssam_retry (request, n, args...)`

> Retry request in case of I/O errors or timeouts.

**Parameters**

`request`
:   The request function to execute. Must return an integer.

`n`
:   Number of tries.

`args...`
:   Arguments for the request function.

**Description**

Executes the given request function, i.e. calls **request**. In case the
request returns `-EREMOTEIO` (indicates I/O error) or `-ETIMEDOUT` (request
or underlying packet timed out), **request** will be re-executed again, up to
**n** times in total.

**Return**

Returns the return value of the last execution of **request**.

ssam\_retry

`ssam_retry (request, args...)`

> Retry request in case of I/O errors or timeouts up to three times in total.

**Parameters**

`request`
:   The request function to execute. Must return an integer.

`args...`
:   Arguments for the request function.

**Description**

Executes the given request function, i.e. calls **request**. In case the
request returns `-EREMOTEIO` (indicates I/O error) or -`ETIMEDOUT` (request
or underlying packet timed out), **request** will be re-executed again, up to
three times in total.

See [`__ssam_retry()`](#c.__ssam_retry "__ssam_retry") for a more generic macro for this purpose.

**Return**

Returns the return value of the last execution of **request**.

struct ssam\_request\_spec
:   Blue-print specification of SAM request.

**Definition**:

```
struct ssam_request_spec {
    u8 target_category;
    u8 target_id;
    u8 command_id;
    u8 instance_id;
    u8 flags;
};
```

**Members**

`target_category`
:   Category of the request’s target. See `enum ssam_ssh_tc`.

`target_id`
:   ID of the request’s target.

`command_id`
:   Command ID of the request.

`instance_id`
:   Instance ID of the request’s target.

`flags`
:   Flags for the request. See [`enum ssam_request_flags`](#c.ssam_request_flags "ssam_request_flags").

**Description**

Blue-print specification for a SAM request. This `struct describes` the
unique static parameters of a request (i.e. type) without specifying any of
its instance-specific data (e.g. payload). It is intended to be used as base
for defining simple request functions via the
`SSAM_DEFINE_SYNC_REQUEST_x()` family of macros.

struct ssam\_request\_spec\_md
:   Blue-print specification for multi-device SAM request.

**Definition**:

```
struct ssam_request_spec_md {
    u8 target_category;
    u8 command_id;
    u8 flags;
};
```

**Members**

`target_category`
:   Category of the request’s target. See `enum ssam_ssh_tc`.

`command_id`
:   Command ID of the request.

`flags`
:   Flags for the request. See [`enum ssam_request_flags`](#c.ssam_request_flags "ssam_request_flags").

**Description**

Blue-print specification for a multi-device SAM request, i.e. a request
that is applicable to multiple device instances, described by their
individual target and instance IDs. This `struct describes` the unique static
parameters of a request (i.e. type) without specifying any of its
instance-specific data (e.g. payload) and without specifying any of its
device specific IDs (i.e. target and instance ID). It is intended to be
used as base for defining simple multi-device request functions via the
`SSAM_DEFINE_SYNC_REQUEST_MD_x()` and `SSAM_DEFINE_SYNC_REQUEST_CL_x()`
families of macros.

SSAM\_DEFINE\_SYNC\_REQUEST\_N

`SSAM_DEFINE_SYNC_REQUEST_N (name, spec...)`

> Define synchronous SAM request function with neither argument nor return value.

**Parameters**

`name`
:   Name of the generated function.

`spec...`
:   Specification ([`struct ssam_request_spec`](#c.ssam_request_spec "ssam_request_spec")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request having neither argument nor return value. The
generated function takes care of setting up the request `struct and` buffer
allocation, as well as execution of the request itself, returning once the
request has been fully completed. The required transport buffer will be
allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl)`, returning the status of the request, which is
zero on success and negative on failure. The `ctrl` parameter is the
controller via which the request is being sent.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_W

`SSAM_DEFINE_SYNC_REQUEST_W (name, atype, spec...)`

> Define synchronous SAM request function with argument.

**Parameters**

`name`
:   Name of the generated function.

`atype`
:   Type of the request’s argument.

`spec...`
:   Specification ([`struct ssam_request_spec`](#c.ssam_request_spec "ssam_request_spec")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request taking an argument of type **atype** and having no
return value. The generated function takes care of setting up the request
struct, buffer allocation, as well as execution of the request itself,
returning once the request has been fully completed. The required transport
buffer will be allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl, const atype *arg)`, returning the status of the
request, which is zero on success and negative on failure. The `ctrl`
parameter is the controller via which the request is sent. The request
argument is specified via the `arg` pointer.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_R

`SSAM_DEFINE_SYNC_REQUEST_R (name, rtype, spec...)`

> Define synchronous SAM request function with return value.

**Parameters**

`name`
:   Name of the generated function.

`rtype`
:   Type of the request’s return value.

`spec...`
:   Specification ([`struct ssam_request_spec`](#c.ssam_request_spec "ssam_request_spec")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request taking no argument but having a return value of
type **rtype**. The generated function takes care of setting up the request
and response structs, buffer allocation, as well as execution of the
request itself, returning once the request has been fully completed. The
required transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl, rtype *ret)`, returning the status of the request,
which is zero on success and negative on failure. The `ctrl` parameter is
the controller via which the request is sent. The request’s return value is
written to the memory pointed to by the `ret` parameter.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_WR

`SSAM_DEFINE_SYNC_REQUEST_WR (name, atype, rtype, spec...)`

> Define synchronous SAM request function with both argument and return value.

**Parameters**

`name`
:   Name of the generated function.

`atype`
:   Type of the request’s argument.

`rtype`
:   Type of the request’s return value.

`spec...`
:   Specification ([`struct ssam_request_spec`](#c.ssam_request_spec "ssam_request_spec")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by **spec**,
with the request taking an argument of type **atype** and having a return value
of type **rtype**. The generated function takes care of setting up the request
and response structs, buffer allocation, as well as execution of the request
itself, returning once the request has been fully completed. The required
transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl, const atype *arg, rtype *ret)`, returning the status
of the request, which is zero on success and negative on failure. The
`ctrl` parameter is the controller via which the request is sent. The
request argument is specified via the `arg` pointer. The request’s return
value is written to the memory pointed to by the `ret` parameter.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_MD\_N

`SSAM_DEFINE_SYNC_REQUEST_MD_N (name, spec...)`

> Define synchronous multi-device SAM request function with neither argument nor return value.

**Parameters**

`name`
:   Name of the generated function.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request having neither argument nor return value. Device
specifying parameters are not hard-coded, but instead must be provided to
the function. The generated function takes care of setting up the request
struct, buffer allocation, as well as execution of the request itself,
returning once the request has been fully completed. The required transport
buffer will be allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl, u8 tid, u8 iid)`, returning the status of the
request, which is zero on success and negative on failure. The `ctrl`
parameter is the controller via which the request is sent, `tid` the
target ID for the request, and `iid` the instance ID.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_MD\_W

`SSAM_DEFINE_SYNC_REQUEST_MD_W (name, atype, spec...)`

> Define synchronous multi-device SAM request function with argument.

**Parameters**

`name`
:   Name of the generated function.

`atype`
:   Type of the request’s argument.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request taking an argument of type **atype** and having no
return value. Device specifying parameters are not hard-coded, but instead
must be provided to the function. The generated function takes care of
setting up the request struct, buffer allocation, as well as execution of
the request itself, returning once the request has been fully completed.
The required transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl, u8 tid, u8 iid, const atype *arg)`, returning the
status of the request, which is zero on success and negative on failure.
The `ctrl` parameter is the controller via which the request is sent,
`tid` the target ID for the request, and `iid` the instance ID. The
request argument is specified via the `arg` pointer.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_MD\_R

`SSAM_DEFINE_SYNC_REQUEST_MD_R (name, rtype, spec...)`

> Define synchronous multi-device SAM request function with return value.

**Parameters**

`name`
:   Name of the generated function.

`rtype`
:   Type of the request’s return value.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request taking no argument but having a return value of
type **rtype**. Device specifying parameters are not hard-coded, but instead
must be provided to the function. The generated function takes care of
setting up the request and response structs, buffer allocation, as well as
execution of the request itself, returning once the request has been fully
completed. The required transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl, u8 tid, u8 iid, rtype *ret)`, returning the status
of the request, which is zero on success and negative on failure. The
`ctrl` parameter is the controller via which the request is sent, `tid`
the target ID for the request, and `iid` the instance ID. The request’s
return value is written to the memory pointed to by the `ret` parameter.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_MD\_WR

`SSAM_DEFINE_SYNC_REQUEST_MD_WR (name, atype, rtype, spec...)`

> Define synchronous multi-device SAM request function with both argument and return value.

**Parameters**

`name`
:   Name of the generated function.

`atype`
:   Type of the request’s argument.

`rtype`
:   Type of the request’s return value.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by **spec**,
with the request taking an argument of type **atype** and having a return value
of type **rtype**. Device specifying parameters are not hard-coded, but instead
must be provided to the function. The generated function takes care of
setting up the request and response structs, buffer allocation, as well as
execution of the request itself, returning once the request has been fully
completed. The required transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct
ssam_controller *ctrl, u8 tid, u8 iid, const atype *arg, rtype *ret)`,
returning the status of the request, which is zero on success and negative
on failure. The `ctrl` parameter is the controller via which the request
is sent, `tid` the target ID for the request, and `iid` the instance ID.
The request argument is specified via the `arg` pointer. The request’s
return value is written to the memory pointed to by the `ret` parameter.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

enum ssam\_notif\_flags
:   Flags used in return values from SSAM notifier callback functions.

**Constants**

`SSAM_NOTIF_HANDLED`
:   Indicates that the notification has been handled. This flag should be
    set by the handler if the handler can act/has acted upon the event
    provided to it. This flag should not be set if the handler is not a
    primary handler intended for the provided event.

`SSAM_NOTIF_STOP`
:   Indicates that the notifier traversal should stop. If this flag is
    returned from a notifier callback, notifier chain traversal will
    immediately stop and any remaining notifiers will not be called. This
    flag is automatically set when [`ssam_notifier_from_errno()`](#c.ssam_notifier_from_errno "ssam_notifier_from_errno") is called
    with a negative error value.

**Description**

> If this flag has not been set by any handler after the notifier chain
> has been traversed, a warning will be emitted, stating that the event
> has not been handled.

struct ssam\_notifier\_block
:   Base notifier block for SSAM event notifications.

**Definition**:

```
struct ssam_notifier_block {
    struct list_head node;
    ssam_notifier_fn_t fn;
    int priority;
};
```

**Members**

`node`
:   The node for the list of notifiers.

`fn`
:   The callback function of this notifier. This function takes the
    respective notifier block and event as input and should return
    a notifier value, which can either be obtained from the flags
    provided in [`enum ssam_notif_flags`](#c.ssam_notif_flags "ssam_notif_flags"), converted from a standard
    error value via [`ssam_notifier_from_errno()`](#c.ssam_notifier_from_errno "ssam_notifier_from_errno"), or a combination of
    both (e.g. `ssam_notifier_from_errno(e) | SSAM_NOTIF_HANDLED`).

`priority`
:   Priority value determining the order in which notifier callbacks
    will be called. A higher value means higher priority, i.e. the
    associated callback will be executed earlier than other (lower
    priority) callbacks.

u32 ssam\_notifier\_from\_errno(int err)
:   Convert standard error value to notifier return code.

**Parameters**

`int err`
:   The error code to convert, must be negative (in case of failure) or
    zero (in case of success).

**Return**

Returns the notifier return value obtained by converting the
specified **err** value. In case **err** is negative, the `SSAM_NOTIF_STOP` flag
will be set, causing notifier call chain traversal to abort.

int ssam\_notifier\_to\_errno(u32 ret)
:   Convert notifier return code to standard error value.

**Parameters**

`u32 ret`
:   The notifier return value to convert.

**Return**

Returns the negative error value encoded in **ret** or zero if **ret**
indicates success.

struct ssam\_event\_registry
:   Registry specification used for enabling events.

**Definition**:

```
struct ssam_event_registry {
    u8 target_category;
    u8 target_id;
    u8 cid_enable;
    u8 cid_disable;
};
```

**Members**

`target_category`
:   Target category for the event registry requests.

`target_id`
:   Target ID for the event registry requests.

`cid_enable`
:   Command ID for the event-enable request.

`cid_disable`
:   Command ID for the event-disable request.

**Description**

This `struct describes` a SAM event registry via the minimal collection of
SAM IDs specifying the requests to use for enabling and disabling an event.
The individual event to be enabled/disabled itself is specified via [`struct
ssam_event_id`](#c.ssam_event_id "ssam_event_id").

struct ssam\_event\_id
:   Unique event ID used for enabling events.

**Definition**:

```
struct ssam_event_id {
    u8 target_category;
    u8 instance;
};
```

**Members**

`target_category`
:   Target category of the event source.

`instance`
:   Instance ID of the event source.

**Description**

This `struct specifies` the event to be enabled/disabled via an externally
provided registry. It does not specify the registry to be used itself, this
is done via [`struct ssam_event_registry`](#c.ssam_event_registry "ssam_event_registry").

enum ssam\_event\_mask
:   Flags specifying how events are matched to notifiers.

**Constants**

`SSAM_EVENT_MASK_TARGET`
:   In addition to filtering by target category, only execute the notifier
    callback for events with a target ID matching to the one of the
    registry used for enabling/disabling the event.

`SSAM_EVENT_MASK_INSTANCE`
:   In addition to filtering by target category, only execute the notifier
    callback for events with an instance ID matching to the instance ID
    used when enabling the event.

`SSAM_EVENT_MASK_NONE`
:   Run the callback for any event with matching target category. Do not
    do any additional filtering.

`SSAM_EVENT_MASK_STRICT`
:   Do all the filtering above.

SSAM\_EVENT\_REGISTRY

`SSAM_EVENT_REGISTRY (tc, tid, cid_en, cid_dis)`

> Define a new event registry.

**Parameters**

`tc`
:   Target category for the event registry requests.

`tid`
:   Target ID for the event registry requests.

`cid_en`
:   Command ID for the event-enable request.

`cid_dis`
:   Command ID for the event-disable request.

**Return**

Returns the [`struct ssam_event_registry`](#c.ssam_event_registry "ssam_event_registry") specified by the given
parameters.

enum ssam\_event\_notifier\_flags
:   Flags for event notifiers.

**Constants**

`SSAM_EVENT_NOTIFIER_OBSERVER`
:   The corresponding notifier acts as observer. Registering a notifier
    with this flag set will not attempt to enable any event. Equally,
    unregistering will not attempt to disable any event. Note that a
    notifier with this flag may not even correspond to a certain event at
    all, only to a specific event target category. Event matching will not
    be influenced by this flag.

struct ssam\_event\_notifier
:   Notifier block for SSAM events.

**Definition**:

```
struct ssam_event_notifier {
    struct ssam_notifier_block base;
    struct {
        struct ssam_event_registry reg;
        struct ssam_event_id id;
        enum ssam_event_mask mask;
        u8 flags;
    } event;
    unsigned long flags;
};
```

**Members**

`base`
:   The base notifier block with callback function and priority.

`event`
:   The event for which this block will receive notifications.

`event.reg`
:   Registry via which the event will be enabled/disabled.

`event.id`
:   ID specifying the event.

`event.mask`
:   Flags determining how events are matched to the notifier.

`event.flags`
:   Flags used for enabling the event.

`flags`
:   Notifier flags (see [`enum ssam_event_notifier_flags`](#c.ssam_event_notifier_flags "ssam_event_notifier_flags")).

int ssam\_notifier\_unregister(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_event\_notifier](#c.ssam_event_notifier "ssam_event_notifier") \*n)
:   Unregister an event notifier.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller the notifier has been registered on.

`struct ssam_event_notifier *n`
:   The event notifier to unregister.

**Description**

Unregister an event notifier. Decrement the usage counter of the associated
SAM event if the notifier is not marked as an observer. If the usage counter
reaches zero, the event will be disabled.

**Return**

Returns zero on success, `-ENOENT` if the given notifier block has
not been registered on the controller. If the given notifier block was the
last one associated with its specific event, returns the status of the
event-disable EC-command.

struct [device](../infrastructure.html#c.device "device") \*ssam\_controller\_device(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*c)
:   Get the [`struct device`](../infrastructure.html#c.device "device") associated with this controller.

**Parameters**

`struct ssam_controller *c`
:   The controller for which to get the device.

**Return**

Returns the [`struct device`](../infrastructure.html#c.device "device") associated with this controller,
providing its lower-level transport.

struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ssam\_controller\_get(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*c)
:   Increment reference count of controller.

**Parameters**

`struct ssam_controller *c`
:   The controller.

**Return**

Returns the controller provided as input.

void ssam\_controller\_put(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*c)
:   Decrement reference count of controller.

**Parameters**

`struct ssam_controller *c`
:   The controller.

void ssam\_controller\_statelock(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*c)
:   Lock the controller against state transitions.

**Parameters**

`struct ssam_controller *c`
:   The controller to lock.

**Description**

Lock the controller against state transitions. Holding this lock guarantees
that the controller will not transition between states, i.e. if the
controller is in state “started”, when this lock has been acquired, it will
remain in this state at least until the lock has been released.

Multiple clients may concurrently hold this lock. In other words: The
`statelock` functions represent the read-lock part of a r/w-semaphore.
Actions causing state transitions of the controller must be executed while
holding the write-part of this r/w-semaphore (see [`ssam_controller_lock()`](internal-api.html#c.ssam_controller_lock "ssam_controller_lock")
and `ssam_controller_unlock()` for that).

See [`ssam_controller_stateunlock()`](#c.ssam_controller_stateunlock "ssam_controller_stateunlock") for the corresponding unlock function.

void ssam\_controller\_stateunlock(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*c)
:   Unlock controller state transitions.

**Parameters**

`struct ssam_controller *c`
:   The controller to unlock.

**Description**

See [`ssam_controller_statelock()`](#c.ssam_controller_statelock "ssam_controller_statelock") for the corresponding lock function.

ssize\_t ssam\_request\_write\_data(struct [ssam\_span](#c.ssam_span "ssam_span") \*buf, struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, const struct [ssam\_request](#c.ssam_request "ssam_request") \*spec)
:   Construct and write SAM request message to buffer.

**Parameters**

`struct ssam_span *buf`
:   The buffer to write the data to.

`struct ssam_controller *ctrl`
:   The controller via which the request will be sent.

`const struct ssam_request *spec`
:   The request data and specification.

**Description**

Constructs a SAM/SSH request message and writes it to the provided buffer.
The request and transport counters, specifically RQID and SEQ, will be set
in this call. These counters are obtained from the controller. It is thus
only valid to send the resulting message via the controller specified here.

For calculation of the required buffer size, refer to the
[`SSH_COMMAND_MESSAGE_LENGTH()`](#c.SSH_COMMAND_MESSAGE_LENGTH "SSH_COMMAND_MESSAGE_LENGTH") macro.

**Return**

Returns the number of bytes used in the buffer on success. Returns
`-EINVAL` if the payload length provided in the request specification is too
large (larger than `SSH_COMMAND_MAX_PAYLOAD_SIZE`) or if the provided buffer
is too small.

int ssam\_request\_sync\_alloc(size\_t payload\_len, gfp\_t flags, struct [ssam\_request\_sync](#c.ssam_request_sync "ssam_request_sync") \*\*rqst, struct [ssam\_span](#c.ssam_span "ssam_span") \*buffer)
:   Allocate a synchronous request.

**Parameters**

`size_t payload_len`
:   The length of the request payload.

`gfp_t flags`
:   Flags used for allocation.

`struct ssam_request_sync **rqst`
:   Where to store the pointer to the allocated request.

`struct ssam_span *buffer`
:   Where to store the buffer descriptor for the message buffer of
    the request.

**Description**

Allocates a synchronous request with corresponding message buffer. The
request still needs to be initialized [`ssam_request_sync_init()`](#c.ssam_request_sync_init "ssam_request_sync_init") before
it can be submitted, and the message buffer data must still be set to the
returned buffer via [`ssam_request_sync_set_data()`](#c.ssam_request_sync_set_data "ssam_request_sync_set_data") after it has been filled,
if need be with adjusted message length.

After use, the request and its corresponding message buffer should be freed
via [`ssam_request_sync_free()`](#c.ssam_request_sync_free "ssam_request_sync_free"). The buffer must not be freed separately.

**Return**

Returns zero on success, `-ENOMEM` if the request could not be
allocated.

void ssam\_request\_sync\_free(struct [ssam\_request\_sync](#c.ssam_request_sync "ssam_request_sync") \*rqst)
:   Free a synchronous request.

**Parameters**

`struct ssam_request_sync *rqst`
:   The request to be freed.

**Description**

Free a synchronous request and its corresponding buffer allocated with
[`ssam_request_sync_alloc()`](#c.ssam_request_sync_alloc "ssam_request_sync_alloc"). Do not use for requests allocated on the stack
or via any other function.

Warning: The caller must ensure that the request is not in use any more.
I.e. the caller must ensure that it has the only reference to the request
and the request is not currently pending. This means that the caller has
either never submitted the request, request submission has failed, or the
caller has waited until the submitted request has been completed via
[`ssam_request_sync_wait()`](#c.ssam_request_sync_wait "ssam_request_sync_wait").

int ssam\_request\_sync\_init(struct [ssam\_request\_sync](#c.ssam_request_sync "ssam_request_sync") \*rqst, enum [ssam\_request\_flags](#c.ssam_request_flags "ssam_request_flags") flags)
:   Initialize a synchronous request struct.

**Parameters**

`struct ssam_request_sync *rqst`
:   The request to initialize.

`enum ssam_request_flags flags`
:   The request flags.

**Description**

Initializes the given request struct. Does not initialize the request
message data. This has to be done explicitly after this call via
[`ssam_request_sync_set_data()`](#c.ssam_request_sync_set_data "ssam_request_sync_set_data") and the actual message data has to be written
via [`ssam_request_write_data()`](#c.ssam_request_write_data "ssam_request_write_data").

**Return**

Returns zero on success or `-EINVAL` if the given flags are invalid.

int ssam\_request\_sync\_submit(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_request\_sync](#c.ssam_request_sync "ssam_request_sync") \*rqst)
:   Submit a synchronous request.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller with which to submit the request.

`struct ssam_request_sync *rqst`
:   The request to submit.

**Description**

Submit a synchronous request. The request has to be initialized and
properly set up, including response buffer (may be `NULL` if no response is
expected) and command message data. This function does not wait for the
request to be completed.

If this function succeeds, [`ssam_request_sync_wait()`](#c.ssam_request_sync_wait "ssam_request_sync_wait") must be used to ensure
that the request has been completed before the response data can be
accessed and/or the request can be freed. On failure, the request may
immediately be freed.

This function may only be used if the controller is active, i.e. has been
initialized and not suspended.

int ssam\_request\_do\_sync(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, const struct [ssam\_request](#c.ssam_request "ssam_request") \*spec, struct [ssam\_response](#c.ssam_response "ssam_response") \*rsp)
:   Execute a synchronous request.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller via which the request will be submitted.

`const struct ssam_request *spec`
:   The request specification and payload.

`struct ssam_response *rsp`
:   The response buffer.

**Description**

Allocates a synchronous request with its message data buffer on the heap
via [`ssam_request_sync_alloc()`](#c.ssam_request_sync_alloc "ssam_request_sync_alloc"), fully initializes it via the provided
request specification, submits it, and finally waits for its completion
before freeing it and returning its status.

**Return**

Returns the status of the request or any failure during setup.

int ssam\_request\_do\_sync\_with\_buffer(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, const struct [ssam\_request](#c.ssam_request "ssam_request") \*spec, struct [ssam\_response](#c.ssam_response "ssam_response") \*rsp, struct [ssam\_span](#c.ssam_span "ssam_span") \*buf)
:   Execute a synchronous request with the provided buffer as back-end for the message buffer.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller via which the request will be submitted.

`const struct ssam_request *spec`
:   The request specification and payload.

`struct ssam_response *rsp`
:   The response buffer.

`struct ssam_span *buf`
:   The buffer for the request message data.

**Description**

Allocates a synchronous request `struct on` the stack, fully initializes it
using the provided buffer as message data buffer, submits it, and then
waits for its completion before returning its status. The
[`SSH_COMMAND_MESSAGE_LENGTH()`](#c.SSH_COMMAND_MESSAGE_LENGTH "SSH_COMMAND_MESSAGE_LENGTH") macro can be used to compute the required
message buffer size.

This function does essentially the same as [`ssam_request_do_sync()`](#c.ssam_request_do_sync "ssam_request_do_sync"), but
instead of dynamically allocating the request and message data buffer, it
uses the provided message data buffer and stores the (small) request `struct
on` the heap.

**Return**

Returns the status of the request or any failure during setup.

int ssam\_notifier\_register(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_event\_notifier](#c.ssam_event_notifier "ssam_event_notifier") \*n)
:   Register an event notifier.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to register the notifier on.

`struct ssam_event_notifier *n`
:   The event notifier to register.

**Description**

Register an event notifier. Increment the usage counter of the associated
SAM event if the notifier is not marked as an observer. If the event is not
marked as an observer and is currently not enabled, it will be enabled
during this call. If the notifier is marked as an observer, no attempt will
be made at enabling any event and no reference count will be modified.

Notifiers marked as observers do not need to be associated with one specific
event, i.e. as long as no event matching is performed, only the event target
category needs to be set.

**Return**

Returns zero on success, `-ENOSPC` if there have already been
`INT_MAX` notifiers for the event ID/type associated with the notifier block
registered, `-ENOMEM` if the corresponding event entry could not be
allocated. If this is the first time that a notifier block is registered
for the specific associated event, returns the status of the event-enable
EC-command.

int \_\_ssam\_notifier\_unregister(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_event\_notifier](#c.ssam_event_notifier "ssam_event_notifier") \*n, bool disable)
:   Unregister an event notifier.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller the notifier has been registered on.

`struct ssam_event_notifier *n`
:   The event notifier to unregister.

`bool disable`
:   Whether to disable the corresponding event on the EC.

**Description**

Unregister an event notifier. Decrement the usage counter of the associated
SAM event if the notifier is not marked as an observer. If the usage counter
reaches zero and `disable` equals `true`, the event will be disabled.

Useful for hot-removable devices, where communication may fail once the
device has been physically removed. In that case, specifying `disable` as
`false` avoids communication with the EC.

**Return**

Returns zero on success, `-ENOENT` if the given notifier block has
not been registered on the controller. If the given notifier block was the
last one associated with its specific event, returns the status of the
event-disable EC-command.

int ssam\_controller\_event\_enable(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_event\_registry](#c.ssam_event_registry "ssam_event_registry") reg, struct [ssam\_event\_id](#c.ssam_event_id "ssam_event_id") id, u8 flags)
:   Enable the specified event.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to enable the event for.

`struct ssam_event_registry reg`
:   The event registry to use for enabling the event.

`struct ssam_event_id id`
:   The event ID specifying the event to be enabled.

`u8 flags`
:   The SAM event flags used for enabling the event.

**Description**

Increment the event reference count of the specified event. If the event has
not been enabled previously, it will be enabled by this call.

**Note**

In general, [`ssam_notifier_register()`](#c.ssam_notifier_register "ssam_notifier_register") with a non-observer notifier
should be preferred for enabling/disabling events, as this will guarantee
proper ordering and event forwarding in case of errors during event
enabling/disabling.

**Return**

Returns zero on success, `-ENOSPC` if the reference count for the
specified event has reached its maximum, `-ENOMEM` if the corresponding event
entry could not be allocated. If this is the first time that this event has
been enabled (i.e. the reference count was incremented from zero to one by
this call), returns the status of the event-enable EC-command.

int ssam\_controller\_event\_disable(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_event\_registry](#c.ssam_event_registry "ssam_event_registry") reg, struct [ssam\_event\_id](#c.ssam_event_id "ssam_event_id") id, u8 flags)
:   Disable the specified event.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to disable the event for.

`struct ssam_event_registry reg`
:   The event registry to use for disabling the event.

`struct ssam_event_id id`
:   The event ID specifying the event to be disabled.

`u8 flags`
:   The flags used when enabling the event.

**Description**

Decrement the reference count of the specified event. If the reference count
reaches zero, the event will be disabled.

**Note**

In general, [`ssam_notifier_register()`](#c.ssam_notifier_register "ssam_notifier_register")/[`ssam_notifier_unregister()`](#c.ssam_notifier_unregister "ssam_notifier_unregister") with a
non-observer notifier should be preferred for enabling/disabling events, as
this will guarantee proper ordering and event forwarding in case of errors
during event enabling/disabling.

**Return**

Returns zero on success, `-ENOENT` if the given event has not been
enabled on the controller. If the reference count of the event reaches zero
during this call, returns the status of the event-disable EC-command.

struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ssam\_get\_controller(void)
:   Get reference to SSAM controller.

**Parameters**

`void`
:   no arguments

**Description**

Returns a reference to the SSAM controller of the system or `NULL` if there
is none, it hasn’t been set up yet, or it has already been unregistered.
This function automatically increments the reference count of the
controller, thus the calling party must ensure that [`ssam_controller_put()`](#c.ssam_controller_put "ssam_controller_put")
is called when it doesn’t need the controller any more.

int ssam\_client\_link(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*c, struct [device](../infrastructure.html#c.device "device") \*client)
:   Link an arbitrary client device to the controller.

**Parameters**

`struct ssam_controller *c`
:   The controller to link to.

`struct device *client`
:   The client device.

**Description**

Link an arbitrary client device to the controller by creating a device link
between it as consumer and the controller device as provider. This function
can be used for non-SSAM devices (or SSAM devices not registered as child
under the controller) to guarantee that the controller is valid for as long
as the driver of the client device is bound, and that proper suspend and
resume ordering is guaranteed.

The device link does not have to be destructed manually. It is removed
automatically once the driver of the client device unbinds.

**Return**

Returns zero on success, `-ENODEV` if the controller is not ready or
going to be removed soon, or `-ENOMEM` if the device link could not be
created for other reasons.

struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ssam\_client\_bind(struct [device](../infrastructure.html#c.device "device") \*client)
:   Bind an arbitrary client device to the controller.

**Parameters**

`struct device *client`
:   The client device.

**Description**

Link an arbitrary client device to the controller by creating a device link
between it as consumer and the main controller device as provider. This
function can be used for non-SSAM devices to guarantee that the controller
returned by this function is valid for as long as the driver of the client
device is bound, and that proper suspend and resume ordering is guaranteed.

This function does essentially the same as [`ssam_client_link()`](#c.ssam_client_link "ssam_client_link"), except that
it first fetches the main controller reference, then creates the link, and
finally returns this reference. Note that this function does not increment
the reference counter of the controller, as, due to the link, the
controller lifetime is assured as long as the driver of the client device
is bound.

It is not valid to use the controller reference obtained by this method
outside of the driver bound to the client device at the time of calling
this function, without first incrementing the reference count of the
controller via [`ssam_controller_get()`](#c.ssam_controller_get "ssam_controller_get"). Even after doing this, care must be
taken that requests are only submitted and notifiers are only
(un-)registered when the controller is active and not suspended. In other
words: The device link only lives as long as the client driver is bound and
any guarantees enforced by this link (e.g. active controller state) can
only be relied upon as long as this link exists and may need to be enforced
in other ways afterwards.

The created device link does not have to be destructed manually. It is
removed automatically once the driver of the client device unbinds.

**Return**

Returns the controller on success, an error pointer with `-ENODEV`
if the controller is not present, not ready or going to be removed soon, or
`-ENOMEM` if the device link could not be created for other reasons.

## [Client Bus and Client Device API](#id4)

enum ssam\_device\_domain
:   SAM device domain.

**Constants**

`SSAM_DOMAIN_VIRTUAL`
:   Virtual device.

`SSAM_DOMAIN_SERIALHUB`
:   Physical device connected via Surface Serial Hub.

enum ssam\_virtual\_tc
:   Target categories for the virtual SAM domain.

**Constants**

`SSAM_VIRTUAL_TC_HUB`
:   Device hub category.

struct ssam\_device\_uid
:   Unique identifier for SSAM device.

**Definition**:

```
struct ssam_device_uid {
    u8 domain;
    u8 category;
    u8 target;
    u8 instance;
    u8 function;
};
```

**Members**

`domain`
:   Domain of the device.

`category`
:   Target category of the device.

`target`
:   Target ID of the device.

`instance`
:   Instance ID of the device.

`function`
:   Sub-function of the device. This field can be used to split a
    single SAM device into multiple virtual subdevices to separate
    different functionality of that device and allow one driver per
    such functionality.

SSAM\_DEVICE

`SSAM_DEVICE (d, cat, tid, iid, fun)`

> Initialize a `struct ssam_device_id` with the given parameters.

**Parameters**

`d`
:   Domain of the device.

`cat`
:   Target category of the device.

`tid`
:   Target ID of the device.

`iid`
:   Instance ID of the device.

`fun`
:   Sub-function of the device.

**Description**

Initializes a `struct ssam_device_id` with the given parameters. See [`struct
ssam_device_uid`](#c.ssam_device_uid "ssam_device_uid") for details regarding the parameters. The special values
`SSAM_SSH_TID_ANY`, `SSAM_SSH_IID_ANY`, and `SSAM_SSH_FUN_ANY` can be used to specify that
matching should ignore target ID, instance ID, and/or sub-function,
respectively. This macro initializes the `match_flags` field based on the
given parameters.

**Note**

The parameters **d** and **cat** must be valid `u8` values, the parameters
**tid**, **iid**, and **fun** must be either valid `u8` values or `SSAM_SSH_TID_ANY`,
`SSAM_SSH_IID_ANY`, or `SSAM_SSH_FUN_ANY`, respectively. Other non-`u8` values are not
allowed.

SSAM\_VDEV

`SSAM_VDEV (cat, tid, iid, fun)`

> Initialize a `struct ssam_device_id` as virtual device with the given parameters.

**Parameters**

`cat`
:   Target category of the device.

`tid`
:   Target ID of the device.

`iid`
:   Instance ID of the device.

`fun`
:   Sub-function of the device.

**Description**

Initializes a `struct ssam_device_id` with the given parameters in the
virtual domain. See [`struct ssam_device_uid`](#c.ssam_device_uid "ssam_device_uid") for details regarding the
parameters. The special values `SSAM_SSH_TID_ANY`, `SSAM_SSH_IID_ANY`, and
`SSAM_SSH_FUN_ANY` can be used to specify that matching should ignore target ID,
instance ID, and/or sub-function, respectively. This macro initializes the
`match_flags` field based on the given parameters.

**Note**

The parameter **cat** must be a valid `u8` value, the parameters **tid**,
**iid**, and **fun** must be either valid `u8` values or `SSAM_SSH_TID_ANY`,
`SSAM_SSH_IID_ANY`, or `SSAM_SSH_FUN_ANY`, respectively. Other non-`u8` values are not
allowed.

SSAM\_SDEV

`SSAM_SDEV (cat, tid, iid, fun)`

> Initialize a `struct ssam_device_id` as physical SSH device with the given parameters.

**Parameters**

`cat`
:   Target category of the device.

`tid`
:   Target ID of the device.

`iid`
:   Instance ID of the device.

`fun`
:   Sub-function of the device.

**Description**

Initializes a `struct ssam_device_id` with the given parameters in the SSH
domain. See [`struct ssam_device_uid`](#c.ssam_device_uid "ssam_device_uid") for details regarding the parameters.
The special values `SSAM_SSH_TID_ANY`, `SSAM_SSH_IID_ANY`, and
`SSAM_SSH_FUN_ANY` can be used to specify that matching should ignore target
ID, instance ID, and/or sub-function, respectively. This macro initializes
the `match_flags` field based on the given parameters.

**Note**

The parameter **cat** must be a valid `u8` value, the parameters **tid**,
**iid**, and **fun** must be either valid `u8` values or `SSAM_SSH_TID_ANY`,
`SSAM_SSH_IID_ANY`, or `SSAM_SSH_FUN_ANY`, respectively. Other non-`u8` values
are not allowed.

struct ssam\_device
:   SSAM client device.

**Definition**:

```
struct ssam_device {
    struct device dev;
    struct ssam_controller *ctrl;
    struct ssam_device_uid uid;
    unsigned long flags;
};
```

**Members**

`dev`
:   Driver model representation of the device.

`ctrl`
:   SSAM controller managing this device.

`uid`
:   UID identifying the device.

`flags`
:   Device state flags, see `enum ssam_device_flags`.

struct ssam\_device\_driver
:   SSAM client device driver.

**Definition**:

```
struct ssam_device_driver {
    struct device_driver driver;
    const struct ssam_device_id *match_table;
    int (*probe)(struct ssam_device *sdev);
    void (*remove)(struct ssam_device *sdev);
};
```

**Members**

`driver`
:   Base driver model structure.

`match_table`
:   Match table specifying which devices the driver should bind to.

`probe`
:   Called when the driver is being bound to a device.

`remove`
:   Called when the driver is being unbound from the device.

bool is\_ssam\_device(struct [device](../infrastructure.html#c.device "device") \*d)
:   Check if the given device is a SSAM client device.

**Parameters**

`struct device *d`
:   The device to test the type of.

**Return**

Returns `true` if the specified device is of type [`struct
ssam_device`](#c.ssam_device "ssam_device"), i.e. the device type points to `ssam_device_type`, and `false`
otherwise.

to\_ssam\_device

`to_ssam_device (d)`

> Casts the given device to a SSAM client device.

**Parameters**

`d`
:   The device to cast.

**Description**

Casts the given [`struct device`](../infrastructure.html#c.device "device") to a [`struct ssam_device`](#c.ssam_device "ssam_device"). The caller has to
ensure that the given device is actually enclosed in a [`struct ssam_device`](#c.ssam_device "ssam_device"),
e.g. by calling [`is_ssam_device()`](#c.is_ssam_device "is_ssam_device").

**Return**

Returns a pointer to the [`struct ssam_device`](#c.ssam_device "ssam_device") wrapping the given
device **d**.

to\_ssam\_device\_driver

`to_ssam_device_driver (d)`

> Casts the given device driver to a SSAM client device driver.

**Parameters**

`d`
:   The driver to cast.

**Description**

Casts the given [`struct device_driver`](../infrastructure.html#c.device_driver "device_driver") to a [`struct ssam_device_driver`](#c.ssam_device_driver "ssam_device_driver"). The
caller has to ensure that the given driver is actually enclosed in a
[`struct ssam_device_driver`](#c.ssam_device_driver "ssam_device_driver").

**Return**

Returns the pointer to the [`struct ssam_device_driver`](#c.ssam_device_driver "ssam_device_driver") wrapping the
given device driver **d**.

void ssam\_device\_mark\_hot\_removed(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Mark the given device as hot-removed.

**Parameters**

`struct ssam_device *sdev`
:   The device to mark as hot-removed.

**Description**

Mark the device as having been hot-removed. This signals drivers using the
device that communication with the device should be avoided and may lead to
timeouts.

bool ssam\_device\_is\_hot\_removed(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Check if the given device has been hot-removed.

**Parameters**

`struct ssam_device *sdev`
:   The device to check.

**Description**

Checks if the given device has been marked as hot-removed. See
[`ssam_device_mark_hot_removed()`](#c.ssam_device_mark_hot_removed "ssam_device_mark_hot_removed") for more details.

**Return**

Returns `true` if the device has been marked as hot-removed.

struct [ssam\_device](#c.ssam_device "ssam_device") \*ssam\_device\_get(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Increment reference count of SSAM client device.

**Parameters**

`struct ssam_device *sdev`
:   The device to increment the reference count of.

**Description**

Increments the reference count of the given SSAM client device by
incrementing the reference count of the enclosed [`struct device`](../infrastructure.html#c.device "device") via
[`get_device()`](../infrastructure.html#c.get_device "get_device").

See [`ssam_device_put()`](#c.ssam_device_put "ssam_device_put") for the counter-part of this function.

**Return**

Returns the device provided as input.

void ssam\_device\_put(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Decrement reference count of SSAM client device.

**Parameters**

`struct ssam_device *sdev`
:   The device to decrement the reference count of.

**Description**

Decrements the reference count of the given SSAM client device by
decrementing the reference count of the enclosed [`struct device`](../infrastructure.html#c.device "device") via
[`put_device()`](../infrastructure.html#c.put_device "put_device").

See [`ssam_device_get()`](#c.ssam_device_get "ssam_device_get") for the counter-part of this function.

void \*ssam\_device\_get\_drvdata(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Get driver-data of SSAM client device.

**Parameters**

`struct ssam_device *sdev`
:   The device to get the driver-data from.

**Return**

Returns the driver-data of the given device, previously set via
[`ssam_device_set_drvdata()`](#c.ssam_device_set_drvdata "ssam_device_set_drvdata").

void ssam\_device\_set\_drvdata(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev, void \*data)
:   Set driver-data of SSAM client device.

**Parameters**

`struct ssam_device *sdev`
:   The device to set the driver-data of.

`void *data`
:   The data to set the device’s driver-data pointer to.

ssam\_device\_driver\_register

`ssam_device_driver_register (drv)`

> Register a SSAM client device driver.

**Parameters**

`drv`
:   The driver to register.

module\_ssam\_device\_driver

`module_ssam_device_driver (drv)`

> Helper macro for SSAM device driver registration.

**Parameters**

`drv`
:   The driver managed by this module.

**Description**

Helper macro to register a SSAM device driver via [`module_init()`](../basics.html#c.module_init "module_init") and
[`module_exit()`](../basics.html#c.module_exit "module_exit"). This macro may only be used once per module and replaces the
aforementioned definitions.

int ssam\_register\_clients(struct [device](../infrastructure.html#c.device "device") \*dev, struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl)
:   Register all client devices defined under the given parent device.

**Parameters**

`struct device *dev`
:   The parent device under which clients should be registered.

`struct ssam_controller *ctrl`
:   The controller with which client should be registered.

**Description**

Register all clients that have via firmware nodes been defined as children
of the given (parent) device. The respective child firmware nodes will be
associated with the correspondingly created child devices.

The given controller will be used to instantiate the new devices. See
[`ssam_device_add()`](#c.ssam_device_add "ssam_device_add") for details.

**Return**

Returns zero on success, nonzero on failure.

int ssam\_device\_register\_clients(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Register all client devices defined under the given SSAM parent device.

**Parameters**

`struct ssam_device *sdev`
:   The parent device under which clients should be registered.

**Description**

Register all clients that have via firmware nodes been defined as children
of the given (parent) device. The respective child firmware nodes will be
associated with the correspondingly created child devices.

The controller used by the parent device will be used to instantiate the new
devices. See [`ssam_device_add()`](#c.ssam_device_add "ssam_device_add") for details.

**Return**

Returns zero on success, nonzero on failure.

SSAM\_DEFINE\_SYNC\_REQUEST\_CL\_N

`SSAM_DEFINE_SYNC_REQUEST_CL_N (name, spec...)`

> Define synchronous client-device SAM request function with neither argument nor return value.

**Parameters**

`name`
:   Name of the generated function.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request having neither argument nor return value. Device
specifying parameters are not hard-coded, but instead are provided via the
client device, specifically its UID, supplied when calling this function.
The generated function takes care of setting up the request struct, buffer
allocation, as well as execution of the request itself, returning once the
request has been fully completed. The required transport buffer will be
allocated on the stack.

The generated function is defined as `static int name(struct ssam_device
*sdev)`, returning the status of the request, which is zero on success and
negative on failure. The `sdev` parameter specifies both the target
device of the request and by association the controller via which the
request is sent.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_CL\_W

`SSAM_DEFINE_SYNC_REQUEST_CL_W (name, atype, spec...)`

> Define synchronous client-device SAM request function with argument.

**Parameters**

`name`
:   Name of the generated function.

`atype`
:   Type of the request’s argument.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request taking an argument of type **atype** and having no
return value. Device specifying parameters are not hard-coded, but instead
are provided via the client device, specifically its UID, supplied when
calling this function. The generated function takes care of setting up the
request struct, buffer allocation, as well as execution of the request
itself, returning once the request has been fully completed. The required
transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct ssam_device
*sdev, const atype *arg)`, returning the status of the request, which is
zero on success and negative on failure. The `sdev` parameter specifies
both the target device of the request and by association the controller via
which the request is sent. The request’s argument is specified via the
`arg` pointer.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_CL\_R

`SSAM_DEFINE_SYNC_REQUEST_CL_R (name, rtype, spec...)`

> Define synchronous client-device SAM request function with return value.

**Parameters**

`name`
:   Name of the generated function.

`rtype`
:   Type of the request’s return value.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by
**spec**, with the request taking no argument but having a return value of
type **rtype**. Device specifying parameters are not hard-coded, but instead
are provided via the client device, specifically its UID, supplied when
calling this function. The generated function takes care of setting up the
request struct, buffer allocation, as well as execution of the request
itself, returning once the request has been fully completed. The required
transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct ssam_device
*sdev, rtype *ret)`, returning the status of the request, which is zero on
success and negative on failure. The `sdev` parameter specifies both the
target device of the request and by association the controller via which
the request is sent. The request’s return value is written to the memory
pointed to by the `ret` parameter.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

SSAM\_DEFINE\_SYNC\_REQUEST\_CL\_WR

`SSAM_DEFINE_SYNC_REQUEST_CL_WR (name, atype, rtype, spec...)`

> Define synchronous client-device SAM request function with argument and return value.

**Parameters**

`name`
:   Name of the generated function.

`atype`
:   Type of the request’s argument.

`rtype`
:   Type of the request’s return value.

`spec...`
:   Specification ([`struct ssam_request_spec_md`](#c.ssam_request_spec_md "ssam_request_spec_md")) defining the request.

**Description**

Defines a function executing the synchronous SAM request specified by **spec**,
with the request taking an argument of type **atype** and having a return value
of type **rtype**. Device specifying parameters are not hard-coded, but instead
are provided via the client device, specifically its UID, supplied when
calling this function. The generated function takes care of setting up the
request struct, buffer allocation, as well as execution of the request
itself, returning once the request has been fully completed. The required
transport buffer will be allocated on the stack.

The generated function is defined as `static int name(struct ssam_device
*sdev, const atype *arg, rtype *ret)`, returning the status of the request,
which is zero on success and negative on failure. The `sdev` parameter
specifies both the target device of the request and by association the
controller via which the request is sent. The request’s argument is
specified via the `arg` pointer. The request’s return value is written to
the memory pointed to by the `ret` parameter.

Refer to [`ssam_request_do_sync_onstack()`](#c.ssam_request_do_sync_onstack "ssam_request_do_sync_onstack") for more details on the behavior of
the generated function.

int ssam\_device\_notifier\_register(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev, struct [ssam\_event\_notifier](#c.ssam_event_notifier "ssam_event_notifier") \*n)
:   Register an event notifier for the specified client device.

**Parameters**

`struct ssam_device *sdev`
:   The device the notifier should be registered on.

`struct ssam_event_notifier *n`
:   The event notifier to register.

**Description**

Register an event notifier. Increment the usage counter of the associated
SAM event if the notifier is not marked as an observer. If the event is not
marked as an observer and is currently not enabled, it will be enabled
during this call. If the notifier is marked as an observer, no attempt will
be made at enabling any event and no reference count will be modified.

Notifiers marked as observers do not need to be associated with one specific
event, i.e. as long as no event matching is performed, only the event target
category needs to be set.

**Return**

Returns zero on success, `-ENOSPC` if there have already been
`INT_MAX` notifiers for the event ID/type associated with the notifier block
registered, `-ENOMEM` if the corresponding event entry could not be
allocated, `-ENODEV` if the device is marked as hot-removed. If this is the
first time that a notifier block is registered for the specific associated
event, returns the status of the event-enable EC-command.

int ssam\_device\_notifier\_unregister(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev, struct [ssam\_event\_notifier](#c.ssam_event_notifier "ssam_event_notifier") \*n)
:   Unregister an event notifier for the specified client device.

**Parameters**

`struct ssam_device *sdev`
:   The device the notifier has been registered on.

`struct ssam_event_notifier *n`
:   The event notifier to unregister.

**Description**

Unregister an event notifier. Decrement the usage counter of the associated
SAM event if the notifier is not marked as an observer. If the usage counter
reaches zero, the event will be disabled.

In case the device has been marked as hot-removed, the event will not be
disabled on the EC, as in those cases any attempt at doing so may time out.

**Return**

Returns zero on success, `-ENOENT` if the given notifier block has
not been registered on the controller. If the given notifier block was the
last one associated with its specific event, returns the status of the
event-disable EC-command.

struct [ssam\_device](#c.ssam_device "ssam_device") \*ssam\_device\_alloc(struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_device\_uid](#c.ssam_device_uid "ssam_device_uid") uid)
:   Allocate and initialize a SSAM client device.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller under which the device should be added.

`struct ssam_device_uid uid`
:   The UID of the device to be added.

**Description**

Allocates and initializes a new client device. The parent of the device
will be set to the controller device and the name will be set based on the
UID. Note that the device still has to be added via [`ssam_device_add()`](#c.ssam_device_add "ssam_device_add").
Refer to that function for more details.

**Return**

Returns the newly allocated and initialized SSAM client device, or
`NULL` if it could not be allocated.

int ssam\_device\_add(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Add a SSAM client device.

**Parameters**

`struct ssam_device *sdev`
:   The SSAM client device to be added.

**Description**

Added client devices must be guaranteed to always have a valid and active
controller. Thus, this function will fail with `-ENODEV` if the controller
of the device has not been initialized yet, has been suspended, or has been
shut down.

The caller of this function should ensure that the corresponding call to
[`ssam_device_remove()`](#c.ssam_device_remove "ssam_device_remove") is issued before the controller is shut down. If the
added device is a direct child of the controller device (default), it will
be automatically removed when the controller is shut down.

By default, the controller device will become the parent of the newly
created client device. The parent may be changed before ssam\_device\_add is
called, but care must be taken that a) the correct suspend/resume ordering
is guaranteed and b) the client device does not outlive the controller,
i.e. that the device is removed before the controller is being shut down.
In case these guarantees have to be manually enforced, please refer to the
[`ssam_client_link()`](#c.ssam_client_link "ssam_client_link") and [`ssam_client_bind()`](#c.ssam_client_bind "ssam_client_bind") functions, which are intended to
set up device-links for this purpose.

**Return**

Returns zero on success, a negative error code on failure.

void ssam\_device\_remove(struct [ssam\_device](#c.ssam_device "ssam_device") \*sdev)
:   Remove a SSAM client device.

**Parameters**

`struct ssam_device *sdev`
:   The device to remove.

**Description**

Removes and unregisters the provided SSAM client device.

const struct ssam\_device\_id \*ssam\_device\_id\_match(const struct ssam\_device\_id \*table, const struct [ssam\_device\_uid](#c.ssam_device_uid "ssam_device_uid") uid)
:   Find the matching ID table entry for the given UID.

**Parameters**

`const struct ssam_device_id *table`
:   The table to search in.

`const struct ssam_device_uid uid`
:   The UID to matched against the individual table entries.

**Description**

Find the first match for the provided device UID in the provided ID table
and return it. Returns `NULL` if no match could be found.

const struct ssam\_device\_id \*ssam\_device\_get\_match(const struct [ssam\_device](#c.ssam_device "ssam_device") \*dev)
:   Find and return the ID matching the device in the ID table of the bound driver.

**Parameters**

`const struct ssam_device *dev`
:   The device for which to get the matching ID table entry.

**Description**

Find the fist match for the UID of the device in the ID table of the
currently bound driver and return it. Returns `NULL` if the device does not
have a driver bound to it, the driver does not have match\_table (i.e. it is
`NULL`), or there is no match in the driver’s match\_table.

This function essentially calls [`ssam_device_id_match()`](#c.ssam_device_id_match "ssam_device_id_match") with the ID table of
the bound device driver and the UID of the device.

**Return**

Returns the first match for the UID of the device in the device
driver’s match table, or `NULL` if no such match could be found.

const void \*ssam\_device\_get\_match\_data(const struct [ssam\_device](#c.ssam_device "ssam_device") \*dev)
:   Find the ID matching the device in the ID table of the bound driver and return its `driver_data` member.

**Parameters**

`const struct ssam_device *dev`
:   The device for which to get the match data.

**Description**

Find the fist match for the UID of the device in the ID table of the
corresponding driver and return its driver\_data. Returns `NULL` if the
device does not have a driver bound to it, the driver does not have
match\_table (i.e. it is `NULL`), there is no match in the driver’s
match\_table, or the match does not have any driver\_data.

This function essentially calls [`ssam_device_get_match()`](#c.ssam_device_get_match "ssam_device_get_match") and, if any match
could be found, returns its `struct ssam_device_id.driver_data` member.

**Return**

Returns the driver data associated with the first match for the UID
of the device in the device driver’s match table, or `NULL` if no such match
could be found.

int \_\_ssam\_device\_driver\_register(struct [ssam\_device\_driver](#c.ssam_device_driver "ssam_device_driver") \*sdrv, struct module \*owner)
:   Register a SSAM client device driver.

**Parameters**

`struct ssam_device_driver *sdrv`
:   The driver to register.

`struct module *owner`
:   The module owning the provided driver.

**Description**

Please refer to the [`ssam_device_driver_register()`](#c.ssam_device_driver_register "ssam_device_driver_register") macro for the normal way
to register a driver from inside its owning module.

void ssam\_device\_driver\_unregister(struct [ssam\_device\_driver](#c.ssam_device_driver "ssam_device_driver") \*sdrv)
:   Unregister a SSAM device driver.

**Parameters**

`struct ssam_device_driver *sdrv`
:   The driver to unregister.

int \_\_ssam\_register\_clients(struct [device](../infrastructure.html#c.device "device") \*parent, struct [ssam\_controller](internal-api.html#c.ssam_controller "ssam_controller") \*ctrl, struct fwnode\_handle \*node)
:   Register client devices defined under the given firmware node as children of the given device.

**Parameters**

`struct device *parent`
:   The parent device under which clients should be registered.

`struct ssam_controller *ctrl`
:   The controller with which client should be registered.

`struct fwnode_handle *node`
:   The firmware node holding definitions of the devices to be added.

**Description**

Register all clients that have been defined as children of the given root
firmware node as children of the given parent device. The respective child
firmware nodes will be associated with the correspondingly created child
devices.

The given controller will be used to instantiate the new devices. See
[`ssam_device_add()`](#c.ssam_device_add "ssam_device_add") for details.

Note that, generally, the use of either [`ssam_device_register_clients()`](#c.ssam_device_register_clients "ssam_device_register_clients") or
[`ssam_register_clients()`](#c.ssam_register_clients "ssam_register_clients") should be preferred as they directly use the
firmware node and/or controller associated with the given device. This
function is only intended for use when different device specifications (e.g.
ACPI and firmware nodes) need to be combined (as is done in the platform hub
of the device registry).

**Return**

Returns zero on success, nonzero on failure.

void ssam\_remove\_clients(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Remove SSAM client devices registered as direct children under the given parent device.

**Parameters**

`struct device *dev`
:   The (parent) device to remove all direct clients for.

**Description**

Remove all SSAM client devices registered as direct children under the given
device. Note that this only accounts for direct children of the device.
Refer to [`ssam_device_add()`](#c.ssam_device_add "ssam_device_add")/[`ssam_device_remove()`](#c.ssam_device_remove "ssam_device_remove") for more details.
