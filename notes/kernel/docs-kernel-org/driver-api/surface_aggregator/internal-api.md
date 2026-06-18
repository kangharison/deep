# Internal API Documentation

> 출처(원문): https://docs.kernel.org/driver-api/surface_aggregator/internal-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Internal API Documentation](#id1)

## [Packet Transport Layer](#id2)

struct sshp\_buf
:   Parser buffer for SSH messages.

**Definition**:

```
struct sshp_buf {
    u8 *ptr;
    size_t len;
    size_t cap;
};
```

**Members**

`ptr`
:   Pointer to the beginning of the buffer.

`len`
:   Number of bytes used in the buffer.

`cap`
:   Maximum capacity of the buffer.

void sshp\_buf\_init(struct [sshp\_buf](#c.sshp_buf "sshp_buf") \*buf, u8 \*ptr, size\_t cap)
:   Initialize a SSH parser buffer.

**Parameters**

`struct sshp_buf *buf`
:   The buffer to initialize.

`u8 *ptr`
:   The memory backing the buffer.

`size_t cap`
:   The length of the memory backing the buffer, i.e. its capacity.

**Description**

Initializes the buffer with the given memory as backing and set its used
length to zero.

int sshp\_buf\_alloc(struct [sshp\_buf](#c.sshp_buf "sshp_buf") \*buf, size\_t cap, gfp\_t flags)
:   Allocate and initialize a SSH parser buffer.

**Parameters**

`struct sshp_buf *buf`
:   The buffer to initialize/allocate to.

`size_t cap`
:   The desired capacity of the buffer.

`gfp_t flags`
:   The flags used for allocating the memory.

**Description**

Allocates **cap** bytes and initializes the provided buffer `struct with` the
allocated memory.

**Return**

Returns zero on success and `-ENOMEM` if allocation failed.

void sshp\_buf\_free(struct [sshp\_buf](#c.sshp_buf "sshp_buf") \*buf)
:   Free a SSH parser buffer.

**Parameters**

`struct sshp_buf *buf`
:   The buffer to free.

**Description**

Frees a SSH parser buffer by freeing the memory backing it and then
resetting its pointer to `NULL` and length and capacity to zero. Intended to
free a buffer previously allocated with [`sshp_buf_alloc()`](#c.sshp_buf_alloc "sshp_buf_alloc").

void sshp\_buf\_drop(struct [sshp\_buf](#c.sshp_buf "sshp_buf") \*buf, size\_t n)
:   Drop data from the beginning of the buffer.

**Parameters**

`struct sshp_buf *buf`
:   The buffer to drop data from.

`size_t n`
:   The number of bytes to drop.

**Description**

Drops the first **n** bytes from the buffer. Re-aligns any remaining data to
the beginning of the buffer.

size\_t sshp\_buf\_read\_from\_fifo(struct [sshp\_buf](#c.sshp_buf "sshp_buf") \*buf, struct kfifo \*fifo)
:   Transfer data from a fifo to the buffer.

**Parameters**

`struct sshp_buf *buf`
:   The buffer to write the data into.

`struct kfifo *fifo`
:   The fifo to read the data from.

**Description**

Transfers the data contained in the fifo to the buffer, removing it from
the fifo. This function will try to transfer as much data as possible,
limited either by the remaining space in the buffer or by the number of
bytes available in the fifo.

**Return**

Returns the number of bytes transferred.

void sshp\_buf\_span\_from(struct [sshp\_buf](#c.sshp_buf "sshp_buf") \*buf, size\_t offset, struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*span)
:   Initialize a span from the given buffer and offset.

**Parameters**

`struct sshp_buf *buf`
:   The buffer to create the span from.

`size_t offset`
:   The offset in the buffer at which the span should start.

`struct ssam_span *span`
:   The span to initialize (output).

**Description**

Initializes the provided span to point to the memory at the given offset in
the buffer, with the length of the span being capped by the number of bytes
used in the buffer after the offset (i.e. bytes remaining after the
offset).

Warning: This function does not validate that **offset** is less than or equal
to the number of bytes used in the buffer or the buffer capacity. This must
be guaranteed by the caller.

bool sshp\_validate\_crc(const struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*src, const u8 \*crc)
:   Validate a CRC in raw message data.

**Parameters**

`const struct ssam_span *src`
:   The span of data over which the CRC should be computed.

`const u8 *crc`
:   The pointer to the expected u16 CRC value.

**Description**

Computes the CRC of the provided data span (**src**), compares it to the CRC
stored at the given address (**crc**), and returns the result of this
comparison, i.e. `true` if equal. This function is intended to run on raw
input/message data.

**Return**

Returns `true` if the computed CRC matches the stored CRC, `false`
otherwise.

bool sshp\_starts\_with\_syn(const struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*src)
:   Check if the given data starts with SSH SYN bytes.

**Parameters**

`const struct ssam_span *src`
:   The data span to check the start of.

bool sshp\_find\_syn(const struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*src, struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*rem)
:   Find SSH SYN bytes in the given data span.

**Parameters**

`const struct ssam_span *src`
:   The data span to search in.

`struct ssam_span *rem`
:   The span (output) indicating the remaining data, starting with SSH
    SYN bytes, if found.

**Description**

Search for SSH SYN bytes in the given source span. If found, set the **rem**
span to the remaining data, starting with the first SYN bytes and capped by
the source span length, and return `true`. This function does not copy any
data, but rather only sets pointers to the respective start addresses and
length values.

If no SSH SYN bytes could be found, set the **rem** span to the zero-length
span at the end of the source span and return `false`.

If partial SSH SYN bytes could be found at the end of the source span, set
the **rem** span to cover these partial SYN bytes, capped by the end of the
source span, and return `false`. This function should then be re-run once
more data is available.

**Return**

Returns `true` if a complete SSH SYN sequence could be found,
`false` otherwise.

int sshp\_parse\_frame(const struct [device](../infrastructure.html#c.device "device") \*dev, const struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*source, struct [ssh\_frame](client-api.html#c.ssh_frame "ssh_frame") \*\*frame, struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*payload, size\_t maxlen)
:   Parse SSH frame.

**Parameters**

`const struct device *dev`
:   The device used for logging.

`const struct ssam_span *source`
:   The source to parse from.

`struct ssh_frame **frame`
:   The parsed frame (output).

`struct ssam_span *payload`
:   The parsed payload (output).

`size_t maxlen`
:   The maximum supported message length.

**Description**

Parses and validates a SSH frame, including its payload, from the given
source. Sets the provided **frame** pointer to the start of the frame and
writes the limits of the frame payload to the provided **payload** span
pointer.

This function does not copy any data, but rather only validates the message
data and sets pointers (and length values) to indicate the respective parts.

If no complete SSH frame could be found, the frame pointer will be set to
the `NULL` pointer and the payload span will be set to the null span (start
pointer `NULL`, size zero).

**Return**

Returns zero on success or if the frame is incomplete, `-ENOMSG` if
the start of the message is invalid, `-EBADMSG` if any (frame-header or
payload) CRC is invalid, or `-EMSGSIZE` if the SSH message is bigger than
the maximum message length specified in the **maxlen** parameter.

int sshp\_parse\_command(const struct [device](../infrastructure.html#c.device "device") \*dev, const struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*source, struct [ssh\_command](client-api.html#c.ssh_command "ssh_command") \*\*command, struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*command\_data)
:   Parse SSH command frame payload.

**Parameters**

`const struct device *dev`
:   The device used for logging.

`const struct ssam_span *source`
:   The source to parse from.

`struct ssh_command **command`
:   The parsed command (output).

`struct ssam_span *command_data`
:   The parsed command data/payload (output).

**Description**

Parses and validates a SSH command frame payload. Sets the **command** pointer
to the command header and the **command\_data** span to the command data (i.e.
payload of the command). This will result in a zero-length span if the
command does not have any associated data/payload. This function does not
check the frame-payload-type field, which should be checked by the caller
before calling this function.

The **source** parameter should be the complete frame payload, e.g. returned
by the [`sshp_parse_frame()`](#c.sshp_parse_frame "sshp_parse_frame") command.

This function does not copy any data, but rather only validates the frame
payload data and sets pointers (and length values) to indicate the
respective parts.

**Return**

Returns zero on success or `-ENOMSG` if **source** does not represent a
valid command-type frame payload, i.e. is too short.

struct msgbuf
:   Buffer `struct to` construct SSH messages.

**Definition**:

```
struct msgbuf {
    u8 *begin;
    u8 *end;
    u8 *ptr;
};
```

**Members**

`begin`
:   Pointer to the beginning of the allocated buffer space.

`end`
:   Pointer to the end (one past last element) of the allocated buffer
    space.

`ptr`
:   Pointer to the first free element in the buffer.

void msgb\_init(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb, u8 \*ptr, size\_t cap)
:   Initialize the given message buffer struct.

**Parameters**

`struct msgbuf *msgb`
:   The buffer `struct to` initialize

`u8 *ptr`
:   Pointer to the underlying memory by which the buffer will be backed.

`size_t cap`
:   Size of the underlying memory.

**Description**

Initialize the given message buffer `struct using` the provided memory as
backing.

size\_t msgb\_bytes\_used(const struct [msgbuf](#c.msgbuf "msgbuf") \*msgb)
:   Return the current number of bytes used in the buffer.

**Parameters**

`const struct msgbuf *msgb`
:   The message buffer.

void msgb\_push\_u16(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb, u16 value)
:   Push a u16 value to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer.

`u16 value`
:   The value to push to the buffer.

void msgb\_push\_syn(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb)
:   Push SSH SYN bytes to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer.

void msgb\_push\_buf(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb, const u8 \*buf, size\_t len)
:   Push raw data to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer.

`const u8 *buf`
:   The data to push to the buffer.

`size_t len`
:   The length of the data to push to the buffer.

void msgb\_push\_crc(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb, const u8 \*buf, size\_t len)
:   Compute CRC and push it to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer.

`const u8 *buf`
:   The data for which the CRC should be computed.

`size_t len`
:   The length of the data for which the CRC should be computed.

void msgb\_push\_frame(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb, u8 ty, u16 len, u8 seq)
:   Push a SSH message frame header to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer

`u8 ty`
:   The type of the frame.

`u16 len`
:   The length of the payload of the frame.

`u8 seq`
:   The sequence ID of the frame/packet.

void msgb\_push\_ack(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb, u8 seq)
:   Push a SSH ACK frame to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer

`u8 seq`
:   The sequence ID of the frame/packet to be ACKed.

void msgb\_push\_nak(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb)
:   Push a SSH NAK frame to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer

void msgb\_push\_cmd(struct [msgbuf](#c.msgbuf "msgbuf") \*msgb, u8 seq, u16 rqid, const struct [ssam\_request](client-api.html#c.ssam_request "ssam_request") \*rqst)
:   Push a SSH command frame with payload to the buffer.

**Parameters**

`struct msgbuf *msgb`
:   The message buffer.

`u8 seq`
:   The sequence ID (SEQ) of the frame/packet.

`u16 rqid`
:   The request ID (RQID) of the request contained in the frame.

`const struct ssam_request *rqst`
:   The request to wrap in the frame.

enum ssh\_ptl\_state\_flags
:   State-flags for [`struct ssh_ptl`](#c.ssh_ptl "ssh_ptl").

**Constants**

`SSH_PTL_SF_SHUTDOWN_BIT`
:   Indicates that the packet transport layer has been shut down or is
    being shut down and should not accept any new packets/data.

struct ssh\_ptl\_ops
:   Callback operations for packet transport layer.

**Definition**:

```
struct ssh_ptl_ops {
    void (*data_received)(struct ssh_ptl *p, const struct ssam_span *data);
};
```

**Members**

`data_received`
:   Function called when a data-packet has been received. Both,
    the packet layer on which the packet has been received and
    the packet’s payload data are provided to this function.

struct ssh\_ptl
:   SSH packet transport layer.

**Definition**:

```
struct ssh_ptl {
    struct serdev_device *serdev;
    unsigned long state;
    struct {
        spinlock_t lock;
        struct list_head head;
    } queue;
    struct {
        spinlock_t lock;
        struct list_head head;
        atomic_t count;
    } pending;
    struct {
        atomic_t running;
        struct task_struct *thread;
        struct completion thread_cplt_tx;
        struct completion thread_cplt_pkt;
        struct wait_queue_head packet_wq;
    } tx;
    struct {
        struct task_struct *thread;
        struct wait_queue_head wq;
        struct kfifo fifo;
        struct sshp_buf buf;
        struct {
            u16 seqs[8];
            u16 offset;
        } blocked;
    } rx;
    struct {
        spinlock_t lock;
        ktime_t timeout;
        ktime_t expires;
        struct delayed_work reaper;
    } rtx_timeout;
    struct ssh_ptl_ops ops;
};
```

**Members**

`serdev`
:   Serial device providing the underlying data transport.

`state`
:   State(-flags) of the transport layer.

`queue`
:   Packet submission queue.

`queue.lock`
:   Lock for modifying the packet submission queue.

`queue.head`
:   List-head of the packet submission queue.

`pending`
:   Set/list of pending packets.

`pending.lock`
:   Lock for modifying the pending set.

`pending.head`
:   List-head of the pending set/list.

`pending.count`
:   Number of currently pending packets.

`tx`
:   Transmitter subsystem.

`tx.running`
:   Flag indicating (desired) transmitter thread state.

`tx.thread`
:   Transmitter thread.

`tx.thread_cplt_tx`
:   Completion for transmitter thread waiting on transfer.

`tx.thread_cplt_pkt`
:   Completion for transmitter thread waiting on packets.

`tx.packet_wq`
:   Waitqueue-head for packet transmit completion.

`rx`
:   Receiver subsystem.

`rx.thread`
:   Receiver thread.

`rx.wq`
:   Waitqueue-head for receiver thread.

`rx.fifo`
:   Buffer for receiving data/pushing data to receiver thread.

`rx.buf`
:   Buffer for evaluating data on receiver thread.

`rx.blocked`
:   List of recent/blocked sequence IDs to detect retransmission.

`rx.blocked.seqs`
:   Array of blocked sequence IDs.

`rx.blocked.offset`
:   Offset indicating where a new ID should be inserted.

`rtx_timeout`
:   Retransmission timeout subsystem.

`rtx_timeout.lock`
:   Lock for modifying the retransmission timeout reaper.

`rtx_timeout.timeout`
:   Timeout interval for retransmission.

`rtx_timeout.expires`
:   Time specifying when the reaper work is next scheduled.

`rtx_timeout.reaper`
:   Work performing timeout checks and subsequent actions.

`ops`
:   Packet layer operations.

struct [device](../infrastructure.html#c.device "device") \*ssh\_ptl\_get\_device(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Get device associated with packet transport layer.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Return**

Returns the device on which the given packet transport layer builds
upon.

void ssh\_ptl\_tx\_wakeup\_transfer(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Wake up packet transmitter thread for transfer.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Description**

Wakes up the packet transmitter thread, notifying it that the underlying
transport has more space for data to be transmitted. If the packet
transport layer has been shut down, calls to this function will be ignored.

bool ssh\_ptl\_should\_drop\_ack\_packet(void)
:   Error injection hook to drop ACK packets.

**Parameters**

`void`
:   no arguments

**Description**

Useful to test detection and handling of automated re-transmits by the EC.
Specifically of packets that the EC considers not-ACKed but the driver
already considers ACKed (due to dropped ACK). In this case, the EC
re-transmits the packet-to-be-ACKed and the driver should detect it as
duplicate/already handled. Note that the driver should still send an ACK
for the re-transmitted packet.

bool ssh\_ptl\_should\_drop\_nak\_packet(void)
:   Error injection hook to drop NAK packets.

**Parameters**

`void`
:   no arguments

**Description**

Useful to test/force automated (timeout-based) re-transmit by the EC.
Specifically, packets that have not reached the driver completely/with valid
checksums. Only useful in combination with receival of (injected) bad data.

bool ssh\_ptl\_should\_drop\_dsq\_packet(void)
:   Error injection hook to drop sequenced data packet.

**Parameters**

`void`
:   no arguments

**Description**

Useful to test re-transmit timeout of the driver. If the data packet has not
been ACKed after a certain time, the driver should re-transmit the packet up
to limited number of times defined in SSH\_PTL\_MAX\_PACKET\_TRIES.

int ssh\_ptl\_should\_fail\_write(void)
:   Error injection hook to make `serdev_device_write()` fail.

**Parameters**

`void`
:   no arguments

**Description**

Hook to simulate errors in serdev\_device\_write when transmitting packets.

bool ssh\_ptl\_should\_corrupt\_tx\_data(void)
:   Error injection hook to simulate invalid data being sent to the EC.

**Parameters**

`void`
:   no arguments

**Description**

Hook to simulate corrupt/invalid data being sent from host (driver) to EC.
Causes the packet data to be actively corrupted by overwriting it with
pre-defined values, such that it becomes invalid, causing the EC to respond
with a NAK packet. Useful to test handling of NAK packets received by the
driver.

bool ssh\_ptl\_should\_corrupt\_rx\_syn(void)
:   Error injection hook to simulate invalid data being sent by the EC.

**Parameters**

`void`
:   no arguments

**Description**

Hook to simulate invalid SYN bytes, i.e. an invalid start of messages and
test handling thereof in the driver.

bool ssh\_ptl\_should\_corrupt\_rx\_data(void)
:   Error injection hook to simulate invalid data being sent by the EC.

**Parameters**

`void`
:   no arguments

**Description**

Hook to simulate invalid data/checksum of the message frame and test handling
thereof in the driver.

void ssh\_packet\_init(struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*packet, unsigned long type, u8 priority, const struct [ssh\_packet\_ops](client-api.html#c.ssh_packet_ops "ssh_packet_ops") \*ops)
:   Initialize SSH packet.

**Parameters**

`struct ssh_packet *packet`
:   The packet to initialize.

`unsigned long type`
:   Type-flags of the packet.

`u8 priority`
:   Priority of the packet. See [`SSH_PACKET_PRIORITY()`](client-api.html#c.SSH_PACKET_PRIORITY "SSH_PACKET_PRIORITY") for details.

`const struct ssh_packet_ops *ops`
:   Packet operations.

**Description**

Initializes the given SSH packet. Sets the transmission buffer pointer to
`NULL` and the transmission buffer length to zero. For data-type packets,
this buffer has to be set separately via [`ssh_packet_set_data()`](client-api.html#c.ssh_packet_set_data "ssh_packet_set_data") before
submission, and must contain a valid SSH message, i.e. frame with optional
payload of any type.

int ssh\_ctrl\_packet\_cache\_init(void)
:   Initialize the control packet cache.

**Parameters**

`void`
:   no arguments

void ssh\_ctrl\_packet\_cache\_destroy(void)
:   Deinitialize the control packet cache.

**Parameters**

`void`
:   no arguments

int ssh\_ctrl\_packet\_alloc(struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*\*packet, struct [ssam\_span](client-api.html#c.ssam_span "ssam_span") \*buffer, gfp\_t flags)
:   Allocate packet from control packet cache.

**Parameters**

`struct ssh_packet **packet`
:   Where the pointer to the newly allocated packet should be stored.

`struct ssam_span *buffer`
:   The buffer corresponding to this packet.

`gfp_t flags`
:   Flags used for allocation.

**Description**

Allocates a packet and corresponding transport buffer from the control
packet cache. Sets the packet’s buffer reference to the allocated buffer.
The packet must be freed via [`ssh_ctrl_packet_free()`](#c.ssh_ctrl_packet_free "ssh_ctrl_packet_free"), which will also free
the corresponding buffer. The corresponding buffer must not be freed
separately. Intended to be used with `ssh_ptl_ctrl_packet_ops` as packet
operations.

**Return**

Returns zero on success, `-ENOMEM` if the allocation failed.

void ssh\_ctrl\_packet\_free(struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Free packet allocated from control packet cache.

**Parameters**

`struct ssh_packet *p`
:   The packet to free.

void ssh\_ptl\_tx\_wakeup\_packet(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Wake up packet transmitter thread for new packet.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Description**

Wakes up the packet transmitter thread, notifying it that a new packet has
arrived and is ready for transfer. If the packet transport layer has been
shut down, calls to this function will be ignored.

int ssh\_ptl\_tx\_start(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Start packet transmitter thread.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Return**

Returns zero on success, a negative error code on failure.

int ssh\_ptl\_tx\_stop(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Stop packet transmitter thread.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Return**

Returns zero on success, a negative error code on failure.

int ssh\_ptl\_submit(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl, struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Submit a packet to the transport layer.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer to submit the packet to.

`struct ssh_packet *p`
:   The packet to submit.

**Description**

Submits a new packet to the transport layer, queuing it to be sent. This
function should not be used for re-submission.

**Return**

Returns zero on success, `-EINVAL` if a packet field is invalid or
the packet has been canceled prior to submission, `-EALREADY` if the packet
has already been submitted, or `-ESHUTDOWN` if the packet transport layer
has been shut down.

void ssh\_ptl\_cancel(struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Cancel a packet.

**Parameters**

`struct ssh_packet *p`
:   The packet to cancel.

**Description**

Cancels a packet. There are no guarantees on when completion and release
callbacks will be called. This may occur during execution of this function
or may occur at any point later.

Note that it is not guaranteed that the packet will actually be canceled if
the packet is concurrently completed by another process. The only guarantee
of this function is that the packet will be completed (with success,
failure, or cancellation) and released from the transport layer in a
reasonable time-frame.

May be called before the packet has been submitted, in which case any later
packet submission fails.

int ssh\_ptl\_rx\_start(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Start packet transport layer receiver thread.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Return**

Returns zero on success, a negative error code on failure.

int ssh\_ptl\_rx\_stop(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Stop packet transport layer receiver thread.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Return**

Returns zero on success, a negative error code on failure.

ssize\_t ssh\_ptl\_rx\_rcvbuf(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl, const u8 \*buf, size\_t n)
:   Push data from lower-layer transport to the packet layer.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

`const u8 *buf`
:   Pointer to the data to push to the layer.

`size_t n`
:   Size of the data to push to the layer, in bytes.

**Description**

Pushes data from a lower-layer transport to the receiver fifo buffer of the
packet layer and notifies the receiver thread. Calls to this function are
ignored once the packet layer has been shut down.

**Return**

Returns the number of bytes transferred (positive or zero) on
success. Returns `-ESHUTDOWN` if the packet layer has been shut down.

void ssh\_ptl\_shutdown(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Shut down the packet transport layer.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer.

**Description**

Shuts down the packet transport layer, removing and canceling all queued
and pending packets. Packets canceled by this operation will be completed
with `-ESHUTDOWN` as status. Receiver and transmitter threads will be
stopped.

As a result of this function, the transport layer will be marked as shut
down. Submission of packets after the transport layer has been shut down
will fail with `-ESHUTDOWN`.

int ssh\_ptl\_init(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl, struct serdev\_device \*serdev, struct [ssh\_ptl\_ops](#c.ssh_ptl_ops "ssh_ptl_ops") \*ops)
:   Initialize packet transport layer.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer to initialize.

`struct serdev_device *serdev`
:   The underlying serial device, i.e. the lower-level transport.

`struct ssh_ptl_ops *ops`
:   Packet layer operations.

**Description**

Initializes the given packet transport layer. Transmitter and receiver
threads must be started separately via [`ssh_ptl_tx_start()`](#c.ssh_ptl_tx_start "ssh_ptl_tx_start") and
[`ssh_ptl_rx_start()`](#c.ssh_ptl_rx_start "ssh_ptl_rx_start"), after the packet-layer has been initialized and the
lower-level transport layer has been set up.

**Return**

Returns zero on success and a nonzero error code on failure.

void ssh\_ptl\_destroy(struct [ssh\_ptl](#c.ssh_ptl "ssh_ptl") \*ptl)
:   Deinitialize packet transport layer.

**Parameters**

`struct ssh_ptl *ptl`
:   The packet transport layer to deinitialize.

**Description**

Deinitializes the given packet transport layer and frees resources
associated with it. If receiver and/or transmitter threads have been
started, the layer must first be shut down via [`ssh_ptl_shutdown()`](#c.ssh_ptl_shutdown "ssh_ptl_shutdown") before
this function can be called.

## [Request Transport Layer](#id3)

enum ssh\_rtl\_state\_flags
:   State-flags for [`struct ssh_rtl`](#c.ssh_rtl "ssh_rtl").

**Constants**

`SSH_RTL_SF_SHUTDOWN_BIT`
:   Indicates that the request transport layer has been shut down or is
    being shut down and should not accept any new requests.

struct ssh\_rtl\_ops
:   Callback operations for request transport layer.

**Definition**:

```
struct ssh_rtl_ops {
    void (*handle_event)(struct ssh_rtl *rtl, const struct ssh_command *cmd, const struct ssam_span *data);
};
```

**Members**

`handle_event`
:   Function called when a SSH event has been received. The
    specified function takes the request layer, received command
    struct, and corresponding payload as arguments. If the event
    has no payload, the payload span is empty (not `NULL`).

struct ssh\_rtl
:   SSH request transport layer.

**Definition**:

```
struct ssh_rtl {
    struct ssh_ptl ptl;
    unsigned long state;
    struct {
        spinlock_t lock;
        struct list_head head;
    } queue;
    struct {
        spinlock_t lock;
        struct list_head head;
        atomic_t count;
    } pending;
    struct {
        struct work_struct work;
    } tx;
    struct {
        spinlock_t lock;
        ktime_t timeout;
        ktime_t expires;
        struct delayed_work reaper;
    } rtx_timeout;
    struct ssh_rtl_ops ops;
};
```

**Members**

`ptl`
:   Underlying packet transport layer.

`state`
:   State(-flags) of the transport layer.

`queue`
:   Request submission queue.

`queue.lock`
:   Lock for modifying the request submission queue.

`queue.head`
:   List-head of the request submission queue.

`pending`
:   Set/list of pending requests.

`pending.lock`
:   Lock for modifying the request set.

`pending.head`
:   List-head of the pending set/list.

`pending.count`
:   Number of currently pending requests.

`tx`
:   Transmitter subsystem.

`tx.work`
:   Transmitter work item.

`rtx_timeout`
:   Retransmission timeout subsystem.

`rtx_timeout.lock`
:   Lock for modifying the retransmission timeout reaper.

`rtx_timeout.timeout`
:   Timeout interval for retransmission.

`rtx_timeout.expires`
:   Time specifying when the reaper work is next scheduled.

`rtx_timeout.reaper`
:   Work performing timeout checks and subsequent actions.

`ops`
:   Request layer operations.

struct [device](../infrastructure.html#c.device "device") \*ssh\_rtl\_get\_device(struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*rtl)
:   Get device associated with request transport layer.

**Parameters**

`struct ssh_rtl *rtl`
:   The request transport layer.

**Return**

Returns the device on which the given request transport layer
builds upon.

struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*ssh\_request\_rtl(struct [ssh\_request](client-api.html#c.ssh_request "ssh_request") \*rqst)
:   Get request transport layer associated with request.

**Parameters**

`struct ssh_request *rqst`
:   The request to get the request transport layer reference for.

**Return**

Returns the [`struct ssh_rtl`](#c.ssh_rtl "ssh_rtl") associated with the given SSH request.

bool ssh\_rtl\_should\_drop\_response(void)
:   Error injection hook to drop request responses.

**Parameters**

`void`
:   no arguments

**Description**

Useful to cause request transmission timeouts in the driver by dropping the
response to a request.

int ssh\_rtl\_submit(struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*rtl, struct [ssh\_request](client-api.html#c.ssh_request "ssh_request") \*rqst)
:   Submit a request to the transport layer.

**Parameters**

`struct ssh_rtl *rtl`
:   The request transport layer.

`struct ssh_request *rqst`
:   The request to submit.

**Description**

Submits a request to the transport layer. A single request may not be
submitted multiple times without reinitializing it.

**Return**

Returns zero on success, `-EINVAL` if the request type is invalid or
the request has been canceled prior to submission, `-EALREADY` if the
request has already been submitted, or `-ESHUTDOWN` in case the request
transport layer has been shut down.

bool ssh\_rtl\_cancel(struct [ssh\_request](client-api.html#c.ssh_request "ssh_request") \*rqst, bool pending)
:   Cancel request.

**Parameters**

`struct ssh_request *rqst`
:   The request to cancel.

`bool pending`
:   Whether to also cancel pending requests.

**Description**

Cancels the given request. If **pending** is `false`, this will not cancel
pending requests, i.e. requests that have already been submitted to the
packet layer but not been completed yet. If **pending** is `true`, this will
cancel the given request regardless of the state it is in.

If the request has been canceled by calling this function, both completion
and release callbacks of the request will be executed in a reasonable
time-frame. This may happen during execution of this function, however,
there is no guarantee for this. For example, a request currently
transmitting will be canceled/completed only after transmission has
completed, and the respective callbacks will be executed on the transmitter
thread, which may happen during, but also some time after execution of the
cancel function.

**Return**

Returns `true` if the given request has been canceled or completed,
either by this function or prior to calling this function, `false`
otherwise. If **pending** is `true`, this function will always return `true`.

int ssh\_request\_init(struct [ssh\_request](client-api.html#c.ssh_request "ssh_request") \*rqst, enum [ssam\_request\_flags](client-api.html#c.ssam_request_flags "ssam_request_flags") flags, const struct [ssh\_request\_ops](client-api.html#c.ssh_request_ops "ssh_request_ops") \*ops)
:   Initialize SSH request.

**Parameters**

`struct ssh_request *rqst`
:   The request to initialize.

`enum ssam_request_flags flags`
:   Request flags, determining the type of the request.

`const struct ssh_request_ops *ops`
:   Request operations.

**Description**

Initializes the given SSH request and underlying packet. Sets the message
buffer pointer to `NULL` and the message buffer length to zero. This buffer
has to be set separately via [`ssh_request_set_data()`](client-api.html#c.ssh_request_set_data "ssh_request_set_data") before submission and
must contain a valid SSH request message.

**Return**

Returns zero on success or `-EINVAL` if the given flags are invalid.

int ssh\_rtl\_init(struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*rtl, struct serdev\_device \*serdev, const struct [ssh\_rtl\_ops](#c.ssh_rtl_ops "ssh_rtl_ops") \*ops)
:   Initialize request transport layer.

**Parameters**

`struct ssh_rtl *rtl`
:   The request transport layer to initialize.

`struct serdev_device *serdev`
:   The underlying serial device, i.e. the lower-level transport.

`const struct ssh_rtl_ops *ops`
:   Request transport layer operations.

**Description**

Initializes the given request transport layer and associated packet
transport layer. Transmitter and receiver threads must be started
separately via [`ssh_rtl_start()`](#c.ssh_rtl_start "ssh_rtl_start"), after the request-layer has been
initialized and the lower-level serial device layer has been set up.

**Return**

Returns zero on success and a nonzero error code on failure.

void ssh\_rtl\_destroy(struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*rtl)
:   Deinitialize request transport layer.

**Parameters**

`struct ssh_rtl *rtl`
:   The request transport layer to deinitialize.

**Description**

Deinitializes the given request transport layer and frees resources
associated with it. If receiver and/or transmitter threads have been
started, the layer must first be shut down via [`ssh_rtl_shutdown()`](#c.ssh_rtl_shutdown "ssh_rtl_shutdown") before
this function can be called.

int ssh\_rtl\_start(struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*rtl)
:   Start request transmitter and receiver.

**Parameters**

`struct ssh_rtl *rtl`
:   The request transport layer.

**Return**

Returns zero on success, a negative error code on failure.

int ssh\_rtl\_flush(struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*rtl, unsigned long timeout)
:   Flush the request transport layer.

**Parameters**

`struct ssh_rtl *rtl`
:   request transport layer

`unsigned long timeout`
:   timeout for the flush operation in jiffies

**Description**

Queue a special flush request and wait for its completion. This request
will be completed after all other currently queued and pending requests
have been completed. Instead of a normal data packet, this request submits
a special flush packet, meaning that upon completion, also the underlying
packet transport layer has been flushed.

Flushing the request layer guarantees that all previously submitted
requests have been fully completed before this call returns. Additionally,
flushing blocks execution of all later submitted requests until the flush
has been completed.

If the caller ensures that no new requests are submitted after a call to
this function, the request transport layer is guaranteed to have no
remaining requests when this call returns. The same guarantee does not hold
for the packet layer, on which control packets may still be queued after
this call.

**Return**

Returns zero on success, `-ETIMEDOUT` if the flush timed out and has
been canceled as a result of the timeout, or `-ESHUTDOWN` if the packet
and/or request transport layer has been shut down before this call. May
also return `-EINTR` if the underlying packet transmission has been
interrupted.

void ssh\_rtl\_shutdown(struct [ssh\_rtl](#c.ssh_rtl "ssh_rtl") \*rtl)
:   Shut down request transport layer.

**Parameters**

`struct ssh_rtl *rtl`
:   The request transport layer.

**Description**

Shuts down the request transport layer, removing and canceling all queued
and pending requests. Requests canceled by this operation will be completed
with `-ESHUTDOWN` as status. Receiver and transmitter threads will be
stopped, the lower-level packet layer will be shutdown.

As a result of this function, the transport layer will be marked as shut
down. Submission of requests after the transport layer has been shut down
will fail with `-ESHUTDOWN`.

## [Controller](#id4)

struct ssh\_seq\_counter
:   Safe counter for SSH sequence IDs.

**Definition**:

```
struct ssh_seq_counter {
    u8 value;
};
```

**Members**

`value`
:   The current counter value.

struct ssh\_rqid\_counter
:   Safe counter for SSH request IDs.

**Definition**:

```
struct ssh_rqid_counter {
    u16 value;
};
```

**Members**

`value`
:   The current counter value.

struct ssam\_nf\_head
:   Notifier head for SSAM events.

**Definition**:

```
struct ssam_nf_head {
    struct srcu_struct srcu;
    struct list_head head;
};
```

**Members**

`srcu`
:   The SRCU struct for synchronization.

`head`
:   List-head for notifier blocks registered under this head.

struct ssam\_nf
:   Notifier callback- and activation-registry for SSAM events.

**Definition**:

```
struct ssam_nf {
    struct mutex lock;
    struct rb_root refcount;
    struct ssam_nf_head head[SSH_NUM_EVENTS];
};
```

**Members**

`lock`
:   Lock guarding (de-)registration of notifier blocks. Note: This
    lock does not need to be held for notifier calls, only
    registration and deregistration.

`refcount`
:   The root of the RB-tree used for reference-counting enabled
    events/notifications.

`head`
:   The list of notifier heads for event/notification callbacks.

struct ssam\_event\_item
:   Struct for event queuing and completion.

**Definition**:

```
struct ssam_event_item {
    struct list_head node;
    u16 rqid;
    struct {
        void (*free)(struct ssam_event_item *event);
    } ops;
    struct ssam_event event;
};
```

**Members**

`node`
:   The node in the queue.

`rqid`
:   The request ID of the event.

`ops`
:   Instance specific functions.

`ops.free`
:   Callback for freeing this event item.

`event`
:   Actual event data.

struct ssam\_event\_queue
:   Queue for completing received events.

**Definition**:

```
struct ssam_event_queue {
    struct ssam_cplt *cplt;
    spinlock_t lock;
    struct list_head head;
    struct work_struct work;
};
```

**Members**

`cplt`
:   Reference to the completion system on which this queue is active.

`lock`
:   The lock for any operation on the queue.

`head`
:   The list-head of the queue.

`work`
:   The `struct work_struct` performing completion work for this queue.

struct ssam\_event\_target
:   Set of queues for a single SSH target ID.

**Definition**:

```
struct ssam_event_target {
    struct ssam_event_queue queue[SSH_NUM_EVENTS];
};
```

**Members**

`queue`
:   The array of queues, one queue per event ID.

struct ssam\_cplt
:   SSAM event/async request completion system.

**Definition**:

```
struct ssam_cplt {
    struct device *dev;
    struct workqueue_struct *wq;
    struct {
        struct ssam_event_target target[SSH_NUM_TARGETS];
        struct ssam_nf notif;
    } event;
};
```

**Members**

`dev`
:   The device with which this system is associated. Only used
    for logging.

`wq`
:   The `struct workqueue_struct` on which all completion work
    items are queued.

`event`
:   Event completion management.

`event.target`
:   Array of [`struct ssam_event_target`](#c.ssam_event_target "ssam_event_target"), one for each target.

`event.notif`
:   Notifier callbacks and event activation reference counting.

enum ssam\_controller\_state
:   State values for [`struct ssam_controller`](#c.ssam_controller "ssam_controller").

**Constants**

`SSAM_CONTROLLER_UNINITIALIZED`
:   The controller has not been initialized yet or has been deinitialized.

`SSAM_CONTROLLER_INITIALIZED`
:   The controller is initialized, but has not been started yet.

`SSAM_CONTROLLER_STARTED`
:   The controller has been started and is ready to use.

`SSAM_CONTROLLER_STOPPED`
:   The controller has been stopped.

`SSAM_CONTROLLER_SUSPENDED`
:   The controller has been suspended.

struct ssam\_controller\_caps
:   Controller device capabilities.

**Definition**:

```
struct ssam_controller_caps {
    u32 ssh_power_profile;
    u32 ssh_buffer_size;
    u32 screen_on_sleep_idle_timeout;
    u32 screen_off_sleep_idle_timeout;
    u32 d3_closes_handle:1;
};
```

**Members**

`ssh_power_profile`
:   SSH power profile.

`ssh_buffer_size`
:   SSH driver UART buffer size.

`screen_on_sleep_idle_timeout`
:   SAM UART screen-on sleep idle timeout.

`screen_off_sleep_idle_timeout`
:   SAM UART screen-off sleep idle timeout.

`d3_closes_handle`
:   SAM closes UART handle in D3.

**Description**

Controller and SSH device capabilities found in ACPI.

struct ssam\_controller
:   SSAM controller device.

**Definition**:

```
struct ssam_controller {
    struct kref kref;
    struct rw_semaphore lock;
    enum ssam_controller_state state;
    struct ssh_rtl rtl;
    struct ssam_cplt cplt;
    struct {
        struct ssh_seq_counter seq;
        struct ssh_rqid_counter rqid;
    } counter;
    struct {
        int num;
        bool wakeup_enabled;
    } irq;
    struct ssam_controller_caps caps;
};
```

**Members**

`kref`
:   Reference count of the controller.

`lock`
:   Main lock for the controller, used to guard state changes.

`state`
:   Controller state.

`rtl`
:   Request transport layer for SSH I/O.

`cplt`
:   Completion system for SSH/SSAM events and asynchronous requests.

`counter`
:   Safe SSH message ID counters.

`counter.seq`
:   Sequence ID counter.

`counter.rqid`
:   Request ID counter.

`irq`
:   Wakeup IRQ resources.

`irq.num`
:   The wakeup IRQ number.

`irq.wakeup_enabled`
:   Whether wakeup by IRQ is enabled during suspend.

`caps`
:   The controller device capabilities.

ssize\_t ssam\_controller\_receive\_buf(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl, const u8 \*buf, size\_t n)
:   Provide input-data to the controller.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

`const u8 *buf`
:   The input buffer.

`size_t n`
:   The number of bytes in the input buffer.

**Description**

Provide input data to be evaluated by the controller, which has been
received via the lower-level transport.

**Return**

Returns the number of bytes consumed, or, if the packet transport
layer of the controller has been shut down, `-ESHUTDOWN`.

void ssam\_controller\_write\_wakeup(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Notify the controller that the underlying device has space available for data to be written.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

void ssh\_seq\_reset(struct [ssh\_seq\_counter](#c.ssh_seq_counter "ssh_seq_counter") \*c)
:   Reset/initialize sequence ID counter.

**Parameters**

`struct ssh_seq_counter *c`
:   The counter to reset.

u8 ssh\_seq\_next(struct [ssh\_seq\_counter](#c.ssh_seq_counter "ssh_seq_counter") \*c)
:   Get next sequence ID.

**Parameters**

`struct ssh_seq_counter *c`
:   The counter providing the sequence IDs.

**Return**

Returns the next sequence ID of the counter.

void ssh\_rqid\_reset(struct [ssh\_rqid\_counter](#c.ssh_rqid_counter "ssh_rqid_counter") \*c)
:   Reset/initialize request ID counter.

**Parameters**

`struct ssh_rqid_counter *c`
:   The counter to reset.

u16 ssh\_rqid\_next(struct [ssh\_rqid\_counter](#c.ssh_rqid_counter "ssh_rqid_counter") \*c)
:   Get next request ID.

**Parameters**

`struct ssh_rqid_counter *c`
:   The counter providing the request IDs.

**Return**

Returns the next request ID of the counter, skipping any reserved
request IDs.

bool ssam\_event\_matches\_notifier(const struct [ssam\_event\_notifier](client-api.html#c.ssam_event_notifier "ssam_event_notifier") \*n, const struct [ssam\_event](client-api.html#c.ssam_event "ssam_event") \*event)
:   Test if an event matches a notifier.

**Parameters**

`const struct ssam_event_notifier *n`
:   The event notifier to test against.

`const struct ssam_event *event`
:   The event to test.

**Return**

Returns `true` if the given event matches the given notifier
according to the rules set in the notifier’s event mask, `false` otherwise.

int ssam\_nfblk\_call\_chain(struct [ssam\_nf\_head](#c.ssam_nf_head "ssam_nf_head") \*nh, struct [ssam\_event](client-api.html#c.ssam_event "ssam_event") \*event)
:   Call event notifier callbacks of the given chain.

**Parameters**

`struct ssam_nf_head *nh`
:   The notifier head for which the notifier callbacks should be called.

`struct ssam_event *event`
:   The event data provided to the callbacks.

**Description**

Call all registered notifier callbacks in order of their priority until
either no notifier is left or a notifier returns a value with the
`SSAM_NOTIF_STOP` bit set. Note that this bit is automatically set via
[`ssam_notifier_from_errno()`](client-api.html#c.ssam_notifier_from_errno "ssam_notifier_from_errno") on any non-zero error value.

**Return**

Returns the notifier status value, which contains the notifier
status bits (`SSAM_NOTIF_HANDLED` and `SSAM_NOTIF_STOP`) as well as a
potential error value returned from the last executed notifier callback.
Use [`ssam_notifier_to_errno()`](client-api.html#c.ssam_notifier_to_errno "ssam_notifier_to_errno") to convert this value to the original error
value.

int ssam\_nfblk\_insert(struct [ssam\_nf\_head](#c.ssam_nf_head "ssam_nf_head") \*nh, struct [ssam\_notifier\_block](client-api.html#c.ssam_notifier_block "ssam_notifier_block") \*nb)
:   Insert a new notifier block into the given notifier list.

**Parameters**

`struct ssam_nf_head *nh`
:   The notifier head into which the block should be inserted.

`struct ssam_notifier_block *nb`
:   The notifier block to add.

**Note**

This function must be synchronized by the caller with respect to other
insert, find, and/or remove calls by holding `struct ssam_nf.lock`.

**Return**

Returns zero on success, `-EEXIST` if the notifier block has already
been registered.

bool ssam\_nfblk\_find(struct [ssam\_nf\_head](#c.ssam_nf_head "ssam_nf_head") \*nh, struct [ssam\_notifier\_block](client-api.html#c.ssam_notifier_block "ssam_notifier_block") \*nb)
:   Check if a notifier block is registered on the given notifier head. list.

**Parameters**

`struct ssam_nf_head *nh`
:   The notifier head on which to search.

`struct ssam_notifier_block *nb`
:   The notifier block to search for.

**Note**

This function must be synchronized by the caller with respect to other
insert, find, and/or remove calls by holding `struct ssam_nf.lock`.

**Return**

Returns true if the given notifier block is registered on the given
notifier head, false otherwise.

void ssam\_nfblk\_remove(struct [ssam\_notifier\_block](client-api.html#c.ssam_notifier_block "ssam_notifier_block") \*nb)
:   Remove a notifier block from its notifier list.

**Parameters**

`struct ssam_notifier_block *nb`
:   The notifier block to be removed.

**Note**

This function must be synchronized by the caller with respect to
other insert, find, and/or remove calls by holding `struct ssam_nf.lock`.
Furthermore, the caller \_must\_ ensure SRCU synchronization by calling
[`synchronize_srcu()`](../../core-api/kernel-api.html#c.synchronize_srcu "synchronize_srcu") with `nh->srcu` after leaving the critical section, to
ensure that the removed notifier block is not in use any more.

int ssam\_nf\_head\_init(struct [ssam\_nf\_head](#c.ssam_nf_head "ssam_nf_head") \*nh)
:   Initialize the given notifier head.

**Parameters**

`struct ssam_nf_head *nh`
:   The notifier head to initialize.

void ssam\_nf\_head\_destroy(struct [ssam\_nf\_head](#c.ssam_nf_head "ssam_nf_head") \*nh)
:   Deinitialize the given notifier head.

**Parameters**

`struct ssam_nf_head *nh`
:   The notifier head to deinitialize.

struct ssam\_nf\_refcount\_key
:   Key used for event activation reference counting.

**Definition**:

```
struct ssam_nf_refcount_key {
    struct ssam_event_registry reg;
    struct ssam_event_id id;
};
```

**Members**

`reg`
:   The registry via which the event is enabled/disabled.

`id`
:   The ID uniquely describing the event.

struct ssam\_nf\_refcount\_entry
:   RB-tree entry for reference counting event activations.

**Definition**:

```
struct ssam_nf_refcount_entry {
    struct rb_node node;
    struct ssam_nf_refcount_key key;
    int refcount;
    u8 flags;
};
```

**Members**

`node`
:   The node of this entry in the rb-tree.

`key`
:   The key of the event.

`refcount`
:   The reference-count of the event.

`flags`
:   The flags used when enabling the event.

struct [ssam\_nf\_refcount\_entry](#c.ssam_nf_refcount_entry "ssam_nf_refcount_entry") \*ssam\_nf\_refcount\_inc(struct [ssam\_nf](#c.ssam_nf "ssam_nf") \*nf, struct [ssam\_event\_registry](client-api.html#c.ssam_event_registry "ssam_event_registry") reg, struct [ssam\_event\_id](client-api.html#c.ssam_event_id "ssam_event_id") id)
:   Increment reference-/activation-count of the given event.

**Parameters**

`struct ssam_nf *nf`
:   The notifier system reference.

`struct ssam_event_registry reg`
:   The registry used to enable/disable the event.

`struct ssam_event_id id`
:   The event ID.

**Description**

Increments the reference-/activation-count associated with the specified
event type/ID, allocating a new entry for this event ID if necessary. A
newly allocated entry will have a refcount of one.

**Note**

`nf->lock` must be held when calling this function.

**Return**

Returns the refcount entry on success. Returns an error pointer
with `-ENOSPC` if there have already been `INT_MAX` events of the specified
ID and type registered, or `-ENOMEM` if the entry could not be allocated.

struct [ssam\_nf\_refcount\_entry](#c.ssam_nf_refcount_entry "ssam_nf_refcount_entry") \*ssam\_nf\_refcount\_dec(struct [ssam\_nf](#c.ssam_nf "ssam_nf") \*nf, struct [ssam\_event\_registry](client-api.html#c.ssam_event_registry "ssam_event_registry") reg, struct [ssam\_event\_id](client-api.html#c.ssam_event_id "ssam_event_id") id)
:   Decrement reference-/activation-count of the given event.

**Parameters**

`struct ssam_nf *nf`
:   The notifier system reference.

`struct ssam_event_registry reg`
:   The registry used to enable/disable the event.

`struct ssam_event_id id`
:   The event ID.

**Description**

Decrements the reference-/activation-count of the specified event,
returning its entry. If the returned entry has a refcount of zero, the
caller is responsible for freeing it using [`kfree()`](../../core-api/mm-api.html#c.kfree "kfree").

**Note**

`nf->lock` must be held when calling this function.

**Return**

Returns the refcount entry on success or `NULL` if the entry has not
been found.

void ssam\_nf\_refcount\_dec\_free(struct [ssam\_nf](#c.ssam_nf "ssam_nf") \*nf, struct [ssam\_event\_registry](client-api.html#c.ssam_event_registry "ssam_event_registry") reg, struct [ssam\_event\_id](client-api.html#c.ssam_event_id "ssam_event_id") id)
:   Decrement reference-/activation-count of the given event and free its entry if the reference count reaches zero.

**Parameters**

`struct ssam_nf *nf`
:   The notifier system reference.

`struct ssam_event_registry reg`
:   The registry used to enable/disable the event.

`struct ssam_event_id id`
:   The event ID.

**Description**

Decrements the reference-/activation-count of the specified event, freeing
its entry if it reaches zero.

**Note**

`nf->lock` must be held when calling this function.

bool ssam\_nf\_refcount\_empty(struct [ssam\_nf](#c.ssam_nf "ssam_nf") \*nf)
:   Test if the notification system has any enabled/active events.

**Parameters**

`struct ssam_nf *nf`
:   The notification system.

void ssam\_nf\_call(struct [ssam\_nf](#c.ssam_nf "ssam_nf") \*nf, struct [device](../infrastructure.html#c.device "device") \*dev, u16 rqid, struct [ssam\_event](client-api.html#c.ssam_event "ssam_event") \*event)
:   Call notification callbacks for the provided event.

**Parameters**

`struct ssam_nf *nf`
:   The notifier system

`struct device *dev`
:   The associated device, only used for logging.

`u16 rqid`
:   The request ID of the event.

`struct ssam_event *event`
:   The event provided to the callbacks.

**Description**

Execute registered callbacks in order of their priority until either no
callback is left or a callback returns a value with the `SSAM_NOTIF_STOP`
bit set. Note that this bit is set automatically when converting non-zero
error values via [`ssam_notifier_from_errno()`](client-api.html#c.ssam_notifier_from_errno "ssam_notifier_from_errno") to notifier values.

Also note that any callback that could handle an event should return a value
with bit `SSAM_NOTIF_HANDLED` set, indicating that the event does not go
unhandled/ignored. In case no registered callback could handle an event,
this function will emit a warning.

In case a callback failed, this function will emit an error message.

int ssam\_nf\_init(struct [ssam\_nf](#c.ssam_nf "ssam_nf") \*nf)
:   Initialize the notifier system.

**Parameters**

`struct ssam_nf *nf`
:   The notifier system to initialize.

void ssam\_nf\_destroy(struct [ssam\_nf](#c.ssam_nf "ssam_nf") \*nf)
:   Deinitialize the notifier system.

**Parameters**

`struct ssam_nf *nf`
:   The notifier system to deinitialize.

int ssam\_event\_item\_cache\_init(void)
:   Initialize the event item cache.

**Parameters**

`void`
:   no arguments

void ssam\_event\_item\_cache\_destroy(void)
:   Deinitialize the event item cache.

**Parameters**

`void`
:   no arguments

void ssam\_event\_item\_free(struct [ssam\_event\_item](#c.ssam_event_item "ssam_event_item") \*item)
:   Free the provided event item.

**Parameters**

`struct ssam_event_item *item`
:   The event item to free.

struct [ssam\_event\_item](#c.ssam_event_item "ssam_event_item") \*ssam\_event\_item\_alloc(size\_t len, gfp\_t flags)
:   Allocate an event item with the given payload size.

**Parameters**

`size_t len`
:   The event payload length.

`gfp_t flags`
:   The flags used for allocation.

**Description**

Allocate an event item with the given payload size, preferring allocation
from the event item cache if the payload is small enough (i.e. smaller than
`SSAM_EVENT_ITEM_CACHE_PAYLOAD_LEN`). Sets the item operations and payload
length values. The item free callback (`ops.free`) should not be
overwritten after this call.

**Return**

Returns the newly allocated event item.

void ssam\_event\_queue\_push(struct [ssam\_event\_queue](#c.ssam_event_queue "ssam_event_queue") \*q, struct [ssam\_event\_item](#c.ssam_event_item "ssam_event_item") \*item)
:   Push an event item to the event queue.

**Parameters**

`struct ssam_event_queue *q`
:   The event queue.

`struct ssam_event_item *item`
:   The item to add.

struct [ssam\_event\_item](#c.ssam_event_item "ssam_event_item") \*ssam\_event\_queue\_pop(struct [ssam\_event\_queue](#c.ssam_event_queue "ssam_event_queue") \*q)
:   Pop the next event item from the event queue.

**Parameters**

`struct ssam_event_queue *q`
:   The event queue.

**Description**

Returns and removes the next event item from the queue. Returns `NULL` If
there is no event item left.

bool ssam\_event\_queue\_is\_empty(struct [ssam\_event\_queue](#c.ssam_event_queue "ssam_event_queue") \*q)
:   Check if the event queue is empty.

**Parameters**

`struct ssam_event_queue *q`
:   The event queue.

struct [ssam\_event\_queue](#c.ssam_event_queue "ssam_event_queue") \*ssam\_cplt\_get\_event\_queue(struct [ssam\_cplt](#c.ssam_cplt "ssam_cplt") \*cplt, u8 tid, u16 rqid)
:   Get the event queue for the given parameters.

**Parameters**

`struct ssam_cplt *cplt`
:   The completion system on which to look for the queue.

`u8 tid`
:   The target ID of the queue.

`u16 rqid`
:   The request ID representing the event ID for which to get the queue.

**Return**

Returns the event queue corresponding to the event type described
by the given parameters. If the request ID does not represent an event,
this function returns `NULL`. If the target ID is not supported, this
function will fall back to the default target ID (`tid = 1`).

bool ssam\_cplt\_submit(struct [ssam\_cplt](#c.ssam_cplt "ssam_cplt") \*cplt, struct work\_struct \*work)
:   Submit a work item to the completion system workqueue.

**Parameters**

`struct ssam_cplt *cplt`
:   The completion system.

`struct work_struct *work`
:   The work item to submit.

int ssam\_cplt\_submit\_event(struct [ssam\_cplt](#c.ssam_cplt "ssam_cplt") \*cplt, struct [ssam\_event\_item](#c.ssam_event_item "ssam_event_item") \*item)
:   Submit an event to the completion system.

**Parameters**

`struct ssam_cplt *cplt`
:   The completion system.

`struct ssam_event_item *item`
:   The event item to submit.

**Description**

Submits the event to the completion system by queuing it on the event item
queue and queuing the respective event queue work item on the completion
workqueue, which will eventually complete the event.

**Return**

Returns zero on success, `-EINVAL` if there is no event queue that
can handle the given event item.

void ssam\_cplt\_flush(struct [ssam\_cplt](#c.ssam_cplt "ssam_cplt") \*cplt)
:   Flush the completion system.

**Parameters**

`struct ssam_cplt *cplt`
:   The completion system.

**Description**

Flush the completion system by waiting until all currently submitted work
items have been completed.

**Note**

This function does not guarantee that all events will have been
handled once this call terminates. In case of a larger number of
to-be-completed events, the event queue work function may re-schedule its
work item, which this flush operation will ignore.

This operation is only intended to, during normal operation prior to
shutdown, try to complete most events and requests to get them out of the
system while the system is still fully operational. It does not aim to
provide any guarantee that all of them have been handled.

void ssam\_event\_queue\_init(struct [ssam\_cplt](#c.ssam_cplt "ssam_cplt") \*cplt, struct [ssam\_event\_queue](#c.ssam_event_queue "ssam_event_queue") \*evq)
:   Initialize an event queue.

**Parameters**

`struct ssam_cplt *cplt`
:   The completion system on which the queue resides.

`struct ssam_event_queue *evq`
:   The event queue to initialize.

int ssam\_cplt\_init(struct [ssam\_cplt](#c.ssam_cplt "ssam_cplt") \*cplt, struct [device](../infrastructure.html#c.device "device") \*dev)
:   Initialize completion system.

**Parameters**

`struct ssam_cplt *cplt`
:   The completion system to initialize.

`struct device *dev`
:   The device used for logging.

void ssam\_cplt\_destroy(struct [ssam\_cplt](#c.ssam_cplt "ssam_cplt") \*cplt)
:   Deinitialize the completion system.

**Parameters**

`struct ssam_cplt *cplt`
:   The completion system to deinitialize.

**Description**

Deinitialize the given completion system and ensure that all pending, i.e.
yet-to-be-completed, event items and requests have been handled.

void ssam\_controller\_lock(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*c)
:   Acquire the main controller lock.

**Parameters**

`struct ssam_controller *c`
:   The controller to lock.

**Description**

This lock must be held for any state transitions, including transition to
suspend/resumed states and during shutdown. See [`ssam_controller_statelock()`](client-api.html#c.ssam_controller_statelock "ssam_controller_statelock")
for more details on controller locking.

See `ssam_controller_unlock()` for the corresponding unlock function.

int ssam\_controller\_caps\_load\_from\_acpi(acpi\_handle handle, struct [ssam\_controller\_caps](#c.ssam_controller_caps "ssam_controller_caps") \*caps)
:   Load controller capabilities from ACPI \_DSM.

**Parameters**

`acpi_handle handle`
:   The handle of the ACPI controller/SSH device.

`struct ssam_controller_caps *caps`
:   Where to store the capabilities in.

**Description**

Initializes the given controller capabilities with default values, then
checks and, if the respective \_DSM functions are available, loads the
actual capabilities from the \_DSM.

**Return**

Returns zero on success, a negative error code on failure.

int ssam\_controller\_caps\_load\_from\_of(struct [device](../infrastructure.html#c.device "device") \*dev, struct [ssam\_controller\_caps](#c.ssam_controller_caps "ssam_controller_caps") \*caps)
:   Load controller capabilities from OF/DT.

**Parameters**

`struct device *dev`
:   A pointer to the controller device

`struct ssam_controller_caps *caps`
:   Where to store the capabilities in.

**Return**

Returns zero on success, a negative error code on failure.

int ssam\_controller\_caps\_load(struct [device](../infrastructure.html#c.device "device") \*dev, struct [ssam\_controller\_caps](#c.ssam_controller_caps "ssam_controller_caps") \*caps)
:   Load controller capabilities

**Parameters**

`struct device *dev`
:   A pointer to the controller device

`struct ssam_controller_caps *caps`
:   Where to store the capabilities in.

**Return**

Returns zero on success, a negative error code on failure.

int ssam\_controller\_init(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl, struct serdev\_device \*serdev)
:   Initialize SSAM controller.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to initialize.

`struct serdev_device *serdev`
:   The serial device representing the underlying data transport.

**Description**

Initializes the given controller. Does neither start receiver nor
transmitter threads. After this call, the controller has to be hooked up to
the serdev core separately via `struct serdev_device_ops`, relaying calls to
[`ssam_controller_receive_buf()`](#c.ssam_controller_receive_buf "ssam_controller_receive_buf") and [`ssam_controller_write_wakeup()`](#c.ssam_controller_write_wakeup "ssam_controller_write_wakeup"). Once the
controller has been hooked up, transmitter and receiver threads may be
started via [`ssam_controller_start()`](#c.ssam_controller_start "ssam_controller_start"). These setup steps need to be completed
before controller can be used for requests.

int ssam\_controller\_start(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Start the receiver and transmitter threads of the controller.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

**Note**

When this function is called, the controller should be properly
hooked up to the serdev core via `struct serdev_device_ops`. Please refer
to [`ssam_controller_init()`](#c.ssam_controller_init "ssam_controller_init") for more details on controller initialization.

This function must be called with the main controller lock held (i.e. by
calling [`ssam_controller_lock()`](#c.ssam_controller_lock "ssam_controller_lock")).

void ssam\_controller\_shutdown(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Shut down the controller.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

**Description**

Shuts down the controller by flushing all pending requests and stopping the
transmitter and receiver threads. All requests submitted after this call
will fail with `-ESHUTDOWN`. While it is discouraged to do so, this function
is safe to use in parallel with ongoing request submission.

In the course of this shutdown procedure, all currently registered
notifiers will be unregistered. It is, however, strongly recommended to not
rely on this behavior, and instead the party registering the notifier
should unregister it before the controller gets shut down, e.g. via the
SSAM bus which guarantees client devices to be removed before a shutdown.

Note that events may still be pending after this call, but, due to the
notifiers being unregistered, these events will be dropped when the
controller is subsequently destroyed via [`ssam_controller_destroy()`](#c.ssam_controller_destroy "ssam_controller_destroy").

This function must be called with the main controller lock held (i.e. by
calling [`ssam_controller_lock()`](#c.ssam_controller_lock "ssam_controller_lock")).

void ssam\_controller\_destroy(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Destroy the controller and free its resources.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

**Description**

Ensures that all resources associated with the controller get freed. This
function should only be called after the controller has been stopped via
[`ssam_controller_shutdown()`](#c.ssam_controller_shutdown "ssam_controller_shutdown"). In general, this function should not be called
directly. The only valid place to call this function directly is during
initialization, before the controller has been fully initialized and passed
to other processes. This function is called automatically when the
reference count of the controller reaches zero.

This function must be called with the main controller lock held (i.e. by
calling [`ssam_controller_lock()`](#c.ssam_controller_lock "ssam_controller_lock")).

int ssam\_controller\_suspend(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Suspend the controller.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to suspend.

**Description**

Marks the controller as suspended. Note that display-off and D0-exit
notifications have to be sent manually before transitioning the controller
into the suspended state via this function.

See [`ssam_controller_resume()`](#c.ssam_controller_resume "ssam_controller_resume") for the corresponding resume function.

**Return**

Returns `-EINVAL` if the controller is currently not in the
“started” state.

int ssam\_controller\_resume(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Resume the controller from suspend.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to resume.

**Description**

Resume the controller from the suspended state it was put into via
[`ssam_controller_suspend()`](#c.ssam_controller_suspend "ssam_controller_suspend"). This function does not issue display-on and
D0-entry notifications. If required, those have to be sent manually after
this call.

**Return**

Returns `-EINVAL` if the controller is currently not suspended.

struct ssh\_notification\_params
:   Command payload to enable/disable SSH notifications.

**Definition**:

```
struct ssh_notification_params {
    u8 target_category;
    u8 flags;
    __le16 request_id;
    u8 instance_id;
};
```

**Members**

`target_category`
:   The target category for which notifications should be
    enabled/disabled.

`flags`
:   Flags determining how notifications are being sent.

`request_id`
:   The request ID that is used to send these notifications.

`instance_id`
:   The specific instance in the given target category for
    which notifications should be enabled.

int ssam\_ssh\_event\_enable(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_event\_registry](client-api.html#c.ssam_event_registry "ssam_event_registry") reg, struct [ssam\_event\_id](client-api.html#c.ssam_event_id "ssam_event_id") id, u8 flags)
:   Enable SSH event.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which to enable the event.

`struct ssam_event_registry reg`
:   The event registry describing what request to use for enabling and
    disabling the event.

`struct ssam_event_id id`
:   The event identifier.

`u8 flags`
:   The event flags.

**Description**

Enables the specified event on the EC. This function does not manage
reference counting of enabled events and is basically only a wrapper for
the raw EC request. If the specified event is already enabled, the EC will
ignore this request.

**Return**

Returns the status of the executed SAM request (zero on success and
negative on direct failure) or `-EPROTO` if the request response indicates a
failure.

int ssam\_ssh\_event\_disable(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_event\_registry](client-api.html#c.ssam_event_registry "ssam_event_registry") reg, struct [ssam\_event\_id](client-api.html#c.ssam_event_id "ssam_event_id") id, u8 flags)
:   Disable SSH event.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which to disable the event.

`struct ssam_event_registry reg`
:   The event registry describing what request to use for enabling and
    disabling the event (must be same as used when enabling the event).

`struct ssam_event_id id`
:   The event identifier.

`u8 flags`
:   The event flags (likely ignored for disabling of events).

**Description**

Disables the specified event on the EC. This function does not manage
reference counting of enabled events and is basically only a wrapper for
the raw EC request. If the specified event is already disabled, the EC will
ignore this request.

**Return**

Returns the status of the executed SAM request (zero on success and
negative on direct failure) or `-EPROTO` if the request response indicates a
failure.

int ssam\_get\_firmware\_version(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl, u32 \*version)
:   Get the SAM/EC firmware version.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

`u32 *version`
:   Where to store the version number.

**Return**

Returns zero on success or the status of the executed SAM request
if that request failed.

int ssam\_ctrl\_notif\_display\_off(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Notify EC that the display has been turned off.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

**Description**

Notify the EC that the display has been turned off and the driver may enter
a lower-power state. This will prevent events from being sent directly.
Rather, the EC signals an event by pulling the wakeup GPIO high for as long
as there are pending events. The events then need to be manually released,
one by one, via the GPIO callback request. All pending events accumulated
during this state can also be released by issuing the display-on
notification, e.g. via [`ssam_ctrl_notif_display_on()`](#c.ssam_ctrl_notif_display_on "ssam_ctrl_notif_display_on"), which will also reset
the GPIO.

On some devices, specifically ones with an integrated keyboard, the keyboard
backlight will be turned off by this call.

This function will only send the display-off notification command if
display notifications are supported by the EC. Currently all known devices
support these notifications.

Use [`ssam_ctrl_notif_display_on()`](#c.ssam_ctrl_notif_display_on "ssam_ctrl_notif_display_on") to reverse the effects of this function.

**Return**

Returns zero on success or if no request has been executed, the
status of the executed SAM request if that request failed, or `-EPROTO` if
an unexpected response has been received.

int ssam\_ctrl\_notif\_display\_on(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Notify EC that the display has been turned on.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller.

**Description**

Notify the EC that the display has been turned back on and the driver has
exited its lower-power state. This notification is the counterpart to the
display-off notification sent via [`ssam_ctrl_notif_display_off()`](#c.ssam_ctrl_notif_display_off "ssam_ctrl_notif_display_off") and will
reverse its effects, including resetting events to their default behavior.

This function will only send the display-on notification command if display
notifications are supported by the EC. Currently all known devices support
these notifications.

See [`ssam_ctrl_notif_display_off()`](#c.ssam_ctrl_notif_display_off "ssam_ctrl_notif_display_off") for more details.

**Return**

Returns zero on success or if no request has been executed, the
status of the executed SAM request if that request failed, or `-EPROTO` if
an unexpected response has been received.

int ssam\_ctrl\_notif\_d0\_exit(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Notify EC that the driver/device exits the D0 power state.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller

**Description**

Notifies the EC that the driver prepares to exit the D0 power state in
favor of a lower-power state. Exact effects of this function related to the
EC are currently unknown.

This function will only send the D0-exit notification command if D0-state
notifications are supported by the EC. Only newer Surface generations
support these notifications.

Use [`ssam_ctrl_notif_d0_entry()`](#c.ssam_ctrl_notif_d0_entry "ssam_ctrl_notif_d0_entry") to reverse the effects of this function.

**Return**

Returns zero on success or if no request has been executed, the
status of the executed SAM request if that request failed, or `-EPROTO` if
an unexpected response has been received.

int ssam\_ctrl\_notif\_d0\_entry(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Notify EC that the driver/device enters the D0 power state.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller

**Description**

Notifies the EC that the driver has exited a lower-power state and entered
the D0 power state. Exact effects of this function related to the EC are
currently unknown.

This function will only send the D0-entry notification command if D0-state
notifications are supported by the EC. Only newer Surface generations
support these notifications.

See [`ssam_ctrl_notif_d0_exit()`](#c.ssam_ctrl_notif_d0_exit "ssam_ctrl_notif_d0_exit") for more details.

**Return**

Returns zero on success or if no request has been executed, the
status of the executed SAM request if that request failed, or `-EPROTO` if
an unexpected response has been received.

int ssam\_nf\_refcount\_enable(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_nf\_refcount\_entry](#c.ssam_nf_refcount_entry "ssam_nf_refcount_entry") \*entry, u8 flags)
:   Enable event for reference count entry if it has not already been enabled.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to enable the event on.

`struct ssam_nf_refcount_entry *entry`
:   The reference count entry for the event to be enabled.

`u8 flags`
:   The flags used for enabling the event on the EC.

**Description**

Enable the event associated with the given reference count entry if the
reference count equals one, i.e. the event has not previously been enabled.
If the event has already been enabled (i.e. reference count not equal to
one), check that the flags used for enabling match and warn about this if
they do not.

This does not modify the reference count itself, which is done with
[`ssam_nf_refcount_inc()`](#c.ssam_nf_refcount_inc "ssam_nf_refcount_inc") / [`ssam_nf_refcount_dec()`](#c.ssam_nf_refcount_dec "ssam_nf_refcount_dec").

**Note**

`nf->lock` must be held when calling this function.

**Return**

Returns zero on success. If the event is enabled by this call,
returns the status of the event-enable EC command.

int ssam\_nf\_refcount\_disable\_free(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl, struct [ssam\_nf\_refcount\_entry](#c.ssam_nf_refcount_entry "ssam_nf_refcount_entry") \*entry, u8 flags, bool ec)
:   Disable event for reference count entry if it is no longer in use and free the corresponding entry.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to disable the event on.

`struct ssam_nf_refcount_entry *entry`
:   The reference count entry for the event to be disabled.

`u8 flags`
:   The flags used for enabling the event on the EC.

`bool ec`
:   Flag specifying if the event should actually be disabled on the EC.

**Description**

If `ec` equals `true` and the reference count equals zero (i.e. the
event is no longer requested by any client), the specified event will be
disabled on the EC via the corresponding request.

If `ec` equals `false`, no request will be sent to the EC and the event
can be considered in a detached state (i.e. no longer used but still
enabled). Disabling an event via this method may be required for
hot-removable devices, where event disable requests may time out after the
device has been physically removed.

In both cases, if the reference count equals zero, the corresponding
reference count entry will be freed. The reference count entry must not be
used any more after a call to this function.

Also checks if the flags used for disabling the event match the flags used
for enabling the event and warns if they do not (regardless of reference
count).

This does not modify the reference count itself, which is done with
[`ssam_nf_refcount_inc()`](#c.ssam_nf_refcount_inc "ssam_nf_refcount_inc") / [`ssam_nf_refcount_dec()`](#c.ssam_nf_refcount_dec "ssam_nf_refcount_dec").

**Note**

`nf->lock` must be held when calling this function.

**Return**

Returns zero on success. If the event is disabled by this call,
returns the status of the event-enable EC command.

int ssam\_notifier\_disable\_registered(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Disable events for all registered notifiers.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which to disable the notifiers/events.

**Description**

Disables events for all currently registered notifiers. In case of an error
(EC command failing), all previously disabled events will be restored and
the error code returned.

This function is intended to disable all events prior to hibernation entry.
See [`ssam_notifier_restore_registered()`](#c.ssam_notifier_restore_registered "ssam_notifier_restore_registered") to restore/re-enable all events
disabled with this function.

Note that this function will not disable events for notifiers registered
after calling this function. It should thus be made sure that no new
notifiers are going to be added after this call and before the corresponding
call to [`ssam_notifier_restore_registered()`](#c.ssam_notifier_restore_registered "ssam_notifier_restore_registered").

**Return**

Returns zero on success. In case of failure returns the error code
returned by the failed EC command to disable an event.

void ssam\_notifier\_restore\_registered(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Restore/re-enable events for all registered notifiers.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which to restore the notifiers/events.

**Description**

Restores/re-enables all events for which notifiers have been registered on
the given controller. In case of a failure, the error is logged and the
function continues to try and enable the remaining events.

This function is intended to restore/re-enable all registered events after
hibernation. See [`ssam_notifier_disable_registered()`](#c.ssam_notifier_disable_registered "ssam_notifier_disable_registered") for the counter part
disabling the events and more details.

bool ssam\_notifier\_is\_empty(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Check if there are any registered notifiers.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to check on.

**Return**

Returns `true` if there are currently no notifiers registered on the
controller, `false` otherwise.

void ssam\_notifier\_unregister\_all(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Unregister all currently registered notifiers.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to unregister the notifiers on.

**Description**

Unregisters all currently registered notifiers. This function is used to
ensure that all notifiers will be unregistered and associated
entries/resources freed when the controller is being shut down.

int ssam\_irq\_setup(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Set up SAM EC wakeup-GPIO interrupt.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which the IRQ should be set up.

**Description**

Set up an IRQ for the wakeup-GPIO pin of the SAM EC. This IRQ can be used
to wake the device from a low power state.

Note that this IRQ can only be triggered while the EC is in the display-off
state. In this state, events are not sent to the host in the usual way.
Instead the wakeup-GPIO gets pulled to “high” as long as there are pending
events and these events need to be released one-by-one via the GPIO
callback request, either until there are no events left and the GPIO is
reset, or all at once by transitioning the EC out of the display-off state,
which will also clear the GPIO.

Not all events, however, should trigger a full system wakeup. Instead the
driver should, if necessary, inspect and forward each event to the
corresponding subsystem, which in turn should decide if the system needs to
be woken up. This logic has not been implemented yet, thus wakeup by this
IRQ should be disabled by default to avoid spurious wake-ups, caused, for
example, by the remaining battery percentage changing. Refer to comments in
this function and comments in the corresponding IRQ handler for more
details on how this should be implemented.

See also [`ssam_ctrl_notif_display_off()`](#c.ssam_ctrl_notif_display_off "ssam_ctrl_notif_display_off") and [`ssam_ctrl_notif_display_off()`](#c.ssam_ctrl_notif_display_off "ssam_ctrl_notif_display_off")
for functions to transition the EC into and out of the display-off state as
well as more details on it.

The IRQ is disabled by default and has to be enabled before it can wake up
the device from suspend via [`ssam_irq_arm_for_wakeup()`](#c.ssam_irq_arm_for_wakeup "ssam_irq_arm_for_wakeup"). On teardown, the IRQ
should be freed via [`ssam_irq_free()`](#c.ssam_irq_free "ssam_irq_free").

void ssam\_irq\_free(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Free SAM EC wakeup-GPIO interrupt.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which the IRQ should be freed.

**Description**

Free the wakeup-GPIO IRQ previously set-up via [`ssam_irq_setup()`](#c.ssam_irq_setup "ssam_irq_setup").

int ssam\_irq\_arm\_for\_wakeup(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Arm the EC IRQ for wakeup, if enabled.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which the IRQ should be armed.

**Description**

Sets up the IRQ so that it can be used to wake the device. Specifically,
this function enables the irq and then, if the device is allowed to wake up
the system, calls `enable_irq_wake()`. See [`ssam_irq_disarm_wakeup()`](#c.ssam_irq_disarm_wakeup "ssam_irq_disarm_wakeup") for the
corresponding function to disable the IRQ.

This function is intended to arm the IRQ before entering S2idle suspend.

**Note**

calls to [`ssam_irq_arm_for_wakeup()`](#c.ssam_irq_arm_for_wakeup "ssam_irq_arm_for_wakeup") and [`ssam_irq_disarm_wakeup()`](#c.ssam_irq_disarm_wakeup "ssam_irq_disarm_wakeup") must
be balanced.

void ssam\_irq\_disarm\_wakeup(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Disarm the wakeup IRQ.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller for which the IRQ should be disarmed.

**Description**

Disarm the IRQ previously set up for wake via [`ssam_irq_arm_for_wakeup()`](#c.ssam_irq_arm_for_wakeup "ssam_irq_arm_for_wakeup").

This function is intended to disarm the IRQ after exiting S2idle suspend.

**Note**

calls to [`ssam_irq_arm_for_wakeup()`](#c.ssam_irq_arm_for_wakeup "ssam_irq_arm_for_wakeup") and [`ssam_irq_disarm_wakeup()`](#c.ssam_irq_disarm_wakeup "ssam_irq_disarm_wakeup") must
be balanced.

## [Client Device Bus](#id5)

bool ssam\_device\_id\_compatible(const struct ssam\_device\_id \*id, struct [ssam\_device\_uid](client-api.html#c.ssam_device_uid "ssam_device_uid") uid)
:   Check if a device ID matches a UID.

**Parameters**

`const struct ssam_device_id *id`
:   The device ID as potential match.

`struct ssam_device_uid uid`
:   The device UID matching against.

**Description**

Check if the given ID is a match for the given UID, i.e. if a device with
the provided UID is compatible to the given ID following the match rules
described in its `ssam_device_id.match_flags` member.

**Return**

Returns `true` if the given UID is compatible to the match rule
described by the given ID, `false` otherwise.

bool ssam\_device\_id\_is\_null(const struct ssam\_device\_id \*id)
:   Check if a device ID is null.

**Parameters**

`const struct ssam_device_id *id`
:   The device ID to check.

**Description**

Check if a given device ID is null, i.e. all zeros. Used to check for the
end of `MODULE_DEVICE_TABLE(ssam, ...)` or similar lists.

**Return**

Returns `true` if the given ID represents a null ID, `false`
otherwise.

int ssam\_bus\_register(void)
:   Register and set-up the SSAM client device bus.

**Parameters**

`void`
:   no arguments

void ssam\_bus\_unregister(void)
:   Unregister the SSAM client device bus.

**Parameters**

`void`
:   no arguments

## [Core](#id6)

int ssam\_try\_set\_controller(struct [ssam\_controller](#c.ssam_controller "ssam_controller") \*ctrl)
:   Try to set the main controller reference.

**Parameters**

`struct ssam_controller *ctrl`
:   The controller to which the reference should point.

**Description**

Set the main controller reference to the given pointer if the reference
hasn’t been set already.

**Return**

Returns zero on success or `-EEXIST` if the reference has already
been set.

void ssam\_clear\_controller(void)
:   Remove/clear the main controller reference.

**Parameters**

`void`
:   no arguments

**Description**

Clears the main controller reference, i.e. sets it to `NULL`. This function
should be called before the controller is shut down.

## [Trace Helpers](#id7)

void ssam\_trace\_ptr\_uid(const void \*ptr, char \*uid\_str)
:   Convert the pointer to a non-pointer UID string.

**Parameters**

`const void *ptr`
:   The pointer to convert.

`char *uid_str`
:   A buffer of length SSAM\_PTR\_UID\_LEN where the UID will be stored.

**Description**

Converts the given pointer into a UID string that is safe to be shared
with userspace and logs, i.e. doesn’t give away the real memory location.

u16 ssam\_trace\_get\_packet\_seq(const struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Read the packet’s sequence ID.

**Parameters**

`const struct ssh_packet *p`
:   The packet.

**Return**

Returns the packet’s sequence ID (SEQ) field if present, or
`SSAM_SEQ_NOT_APPLICABLE` if not (e.g. flush packet).

u32 ssam\_trace\_get\_request\_id(const struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Read the packet’s request ID.

**Parameters**

`const struct ssh_packet *p`
:   The packet.

**Return**

Returns the packet’s request ID (RQID) field if the packet
represents a request with command data, or `SSAM_RQID_NOT_APPLICABLE` if not
(e.g. flush request, control packet).

u32 ssam\_trace\_get\_request\_tid(const struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Read the packet’s request target ID.

**Parameters**

`const struct ssh_packet *p`
:   The packet.

**Return**

Returns the packet’s request target ID (TID) field if the packet
represents a request with command data, or `SSAM_SSH_TID_NOT_APPLICABLE`
if not (e.g. flush request, control packet).

u32 ssam\_trace\_get\_request\_sid(const struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Read the packet’s request source ID.

**Parameters**

`const struct ssh_packet *p`
:   The packet.

**Return**

Returns the packet’s request source ID (SID) field if the packet
represents a request with command data, or `SSAM_SSH_TID_NOT_APPLICABLE`
if not (e.g. flush request, control packet).

u32 ssam\_trace\_get\_request\_tc(const struct [ssh\_packet](client-api.html#c.ssh_packet "ssh_packet") \*p)
:   Read the packet’s request target category.

**Parameters**

`const struct ssh_packet *p`
:   The packet.

**Return**

Returns the packet’s request target category (TC) field if the
packet represents a request with command data, or `SSAM_SSH_TC_NOT_APPLICABLE`
if not (e.g. flush request, control packet).
