# 4.3.Digital TV Demux kABI

> 출처(원문): https://docs.kernel.org/driver-api/media/dtv-demux.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.3. Digital TV Demux kABI

## 4.3.1. Digital TV Demux

The Kernel Digital TV Demux kABI defines a driver-internal interface for
registering low-level, hardware specific driver to a hardware independent
demux layer. It is only of interest for Digital TV device driver writers.
The header file for this kABI is named `demux.h` and located in
`include/media`.

The demux kABI should be implemented for each demux in the system. It is
used to select the TS source of a demux and to manage the demux resources.
When the demux client allocates a resource via the demux kABI, it receives
a pointer to the kABI of that resource.

Each demux receives its TS input from a DVB front-end or from memory, as
set via this demux kABI. In a system with more than one front-end, the kABI
can be used to select one of the DVB front-ends as a TS source for a demux,
unless this is fixed in the HW platform.

The demux kABI only controls front-ends regarding to their connections with
demuxes; the kABI used to set the other front-end parameters, such as
tuning, are defined via the Digital TV Frontend kABI.

The functions that implement the abstract interface demux should be defined
static or module private and registered to the Demux core for external
access. It is not necessary to implement every function in the struct
[`dmx_demux`](#c.dmx_demux "dmx_demux"). For example, a demux interface might support Section filtering,
but not PES filtering. The kABI client is expected to check the value of any
function pointer before calling the function: the value of `NULL` means
that the function is not available.

Whenever the functions of the demux API modify shared data, the
possibilities of lost update and race condition problems should be
addressed, e.g. by protecting parts of code with mutexes.

Note that functions called from a bottom half context must not sleep.
Even a simple memory allocation without using `GFP_ATOMIC` can result in a
kernel thread being put to sleep if swapping is needed. For example, the
Linux Kernel calls the functions of a network device interface from a
bottom half context. Thus, if a demux kABI function is called from network
device code, the function must not sleep.

## 4.3.2. Demux Callback API

This kernel-space API comprises the callback functions that deliver filtered
data to the demux client. Unlike the other DVB kABIs, these functions are
provided by the client and called from the demux code.

The function pointers of this abstract interface are not packed into a
structure as in the other demux APIs, because the callback functions are
registered and used independent of each other. As an example, it is possible
for the API client to provide several callback functions for receiving TS
packets and no callbacks for PES packets or sections.

The functions that implement the callback API need not be re-entrant: when
a demux driver calls one of these functions, the driver is not allowed to
call the function again before the original call returns. If a callback is
triggered by a hardware interrupt, it is recommended to use the Linux
bottom half mechanism or start a tasklet instead of making the callback
function call directly from a hardware interrupt.

This mechanism is implemented by [`dmx_ts_cb()`](#c.dmx_ts_cb "dmx_ts_cb") and [`dmx_section_cb()`](#c.dmx_section_cb "dmx_section_cb")
callbacks.

## 4.3.3. Digital TV Demux device registration functions and data structures

enum dmxdev\_type
:   type of demux filter type.

**Constants**

`DMXDEV_TYPE_NONE`
:   no filter set.

`DMXDEV_TYPE_SEC`
:   section filter.

`DMXDEV_TYPE_PES`
:   Program Elementary Stream (PES) filter.

enum dmxdev\_state
:   state machine for the dmxdev.

**Constants**

`DMXDEV_STATE_FREE`
:   indicates that the filter is freed.

`DMXDEV_STATE_ALLOCATED`
:   indicates that the filter was allocated
    to be used.

`DMXDEV_STATE_SET`
:   indicates that the filter parameters are set.

`DMXDEV_STATE_GO`
:   indicates that the filter is running.

`DMXDEV_STATE_DONE`
:   indicates that a packet was already filtered
    and the filter is now disabled.
    Set only if `DMX_ONESHOT`. See
    `dmx_sct_filter_params`.

`DMXDEV_STATE_TIMEDOUT`
:   Indicates a timeout condition.

struct dmxdev\_feed
:   digital TV dmxdev feed

**Definition**:

```
struct dmxdev_feed {
    u16 pid;
    struct dmx_ts_feed *ts;
    struct list_head next;
};
```

**Members**

`pid`
:   Program ID to be filtered

`ts`
:   pointer to [`struct dmx_ts_feed`](#c.dmx_ts_feed "dmx_ts_feed")

`next`
:   `struct list_head` pointing to the next feed.

struct dmxdev\_filter
:   digital TV dmxdev filter

**Definition**:

```
struct dmxdev_filter {
    union {
        struct dmx_section_filter *sec;
    } filter;
    union {
        struct list_head ts;
        struct dmx_section_feed *sec;
    } feed;
    union {
        struct dmx_sct_filter_params sec;
        struct dmx_pes_filter_params pes;
    } params;
    enum dmxdev_type type;
    enum dmxdev_state state;
    struct dmxdev *dev;
    struct dvb_ringbuffer buffer;
    struct dvb_vb2_ctx vb2_ctx;
    struct mutex mutex;
    struct timer_list timer;
    int todo;
    u8 secheader[3];
};
```

**Members**

`filter`
:   a `union describing` a dmxdev filter.
    Currently used only for section filters.

`filter.sec`
:   a [`struct dmx_section_filter`](#c.dmx_section_filter "dmx_section_filter") pointer.
    For section filter only.

`feed`
:   a `union describing` a dmxdev feed.
    Depending on the filter type, it can be either
    **feed.ts** or **feed.sec**.

`feed.ts`
:   a `struct list_head` list.
    For TS and PES feeds.

`feed.sec`
:   a [`struct dmx_section_feed`](#c.dmx_section_feed "dmx_section_feed") pointer.
    For section feed only.

`params`
:   a `union describing` dmxdev filter parameters.
    Depending on the filter type, it can be either
    **params.sec** or **params.pes**.

`params.sec`
:   a `struct dmx_sct_filter_params` embedded struct.
    For section filter only.

`params.pes`
:   a `struct dmx_pes_filter_params` embedded struct.
    For PES filter only.

`type`
:   type of the dmxdev filter, as defined by [`enum dmxdev_type`](#c.dmxdev_type "dmxdev_type").

`state`
:   state of the dmxdev filter, as defined by [`enum dmxdev_state`](#c.dmxdev_state "dmxdev_state").

`dev`
:   pointer to [`struct dmxdev`](#c.dmxdev "dmxdev").

`buffer`
:   an embedded [`struct dvb_ringbuffer`](dtv-common.html#c.dvb_ringbuffer "dvb_ringbuffer") buffer.

`vb2_ctx`
:   control struct for VB2 handler

`mutex`
:   protects the access to [`struct dmxdev_filter`](#c.dmxdev_filter "dmxdev_filter").

`timer`
:   `struct timer_list` embedded timer, used to check for
    feed timeouts.
    Only for section filter.

`todo`
:   index for the **secheader**.
    Only for section filter.

`secheader`
:   buffer cache to parse the section header.
    Only for section filter.

struct dmxdev
:   Describes a digital TV demux device.

**Definition**:

```
struct dmxdev {
    struct dvb_device *dvbdev;
    struct dvb_device *dvr_dvbdev;
    struct dmxdev_filter *filter;
    struct dmx_demux *demux;
    int filternum;
    int capabilities;
    unsigned int may_do_mmap:1;
    unsigned int exit:1;
#define DMXDEV_CAP_DUPLEX 1;
    struct dmx_frontend *dvr_orig_fe;
    struct dvb_ringbuffer dvr_buffer;
#define DVR_BUFFER_SIZE (10*188*1024);
    struct dvb_vb2_ctx dvr_vb2_ctx;
    struct mutex mutex;
    spinlock_t lock;
};
```

**Members**

`dvbdev`
:   pointer to [`struct dvb_device`](dtv-common.html#c.dvb_device "dvb_device") associated with
    the demux device node.

`dvr_dvbdev`
:   pointer to [`struct dvb_device`](dtv-common.html#c.dvb_device "dvb_device") associated with
    the dvr device node.

`filter`
:   pointer to [`struct dmxdev_filter`](#c.dmxdev_filter "dmxdev_filter").

`demux`
:   pointer to [`struct dmx_demux`](#c.dmx_demux "dmx_demux").

`filternum`
:   number of filters.

`capabilities`
:   demux capabilities as defined by [`enum dmx_demux_caps`](#c.dmx_demux_caps "dmx_demux_caps").

`may_do_mmap`
:   flag used to indicate if the device may do mmap.

`exit`
:   flag to indicate that the demux is being released.

`dvr_orig_fe`
:   pointer to [`struct dmx_frontend`](#c.dmx_frontend "dmx_frontend").

`dvr_buffer`
:   embedded [`struct dvb_ringbuffer`](dtv-common.html#c.dvb_ringbuffer "dvb_ringbuffer") for DVB output.

`dvr_vb2_ctx`
:   control struct for VB2 handler

`mutex`
:   protects the usage of this structure.

`lock`
:   protects access to [`dmxdev->filter`](#c.dmxdev "dmxdev")->data.

int dvb\_dmxdev\_init(struct [dmxdev](#c.dvb_dmxdev_init "dmxdev") \*dmxdev, struct [dvb\_adapter](dtv-common.html#c.dvb_adapter "dvb_adapter") \*adap)
:   initializes a digital TV demux and registers both demux and DVR devices.

**Parameters**

`struct dmxdev *dmxdev`
:   pointer to [`struct dmxdev`](#c.dmxdev "dmxdev").

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](dtv-common.html#c.dvb_adapter "dvb_adapter").

void dvb\_dmxdev\_release(struct [dmxdev](#c.dvb_dmxdev_release "dmxdev") \*dmxdev)
:   releases a digital TV demux and unregisters it.

**Parameters**

`struct dmxdev *dmxdev`
:   pointer to [`struct dmxdev`](#c.dmxdev "dmxdev").

## 4.3.4. High-level Digital TV demux interface

enum dvb\_dmx\_filter\_type
:   type of demux feed.

**Constants**

`DMX_TYPE_TS`
:   feed is in TS mode.

`DMX_TYPE_SEC`
:   feed is in Section mode.

enum dvb\_dmx\_state
:   state machine for a demux filter.

**Constants**

`DMX_STATE_FREE`
:   indicates that the filter is freed.

`DMX_STATE_ALLOCATED`
:   indicates that the filter was allocated
    to be used.

`DMX_STATE_READY`
:   indicates that the filter is ready
    to be used.

`DMX_STATE_GO`
:   indicates that the filter is running.

struct dvb\_demux\_filter
:   Describes a DVB demux section filter.

**Definition**:

```
struct dvb_demux_filter {
    struct dmx_section_filter filter;
    u8 maskandmode[DMX_MAX_FILTER_SIZE];
    u8 maskandnotmode[DMX_MAX_FILTER_SIZE];
    bool doneq;
    struct dvb_demux_filter *next;
    struct dvb_demux_feed *feed;
    int index;
    enum dvb_dmx_state state;
    enum dvb_dmx_filter_type type;
};
```

**Members**

`filter`
:   Section filter as defined by [`struct dmx_section_filter`](#c.dmx_section_filter "dmx_section_filter").

`maskandmode`
:   logical `and` bit mask.

`maskandnotmode`
:   logical `and not` bit mask.

`doneq`
:   flag that indicates when a filter is ready.

`next`
:   pointer to the next section filter.

`feed`
:   [`struct dvb_demux_feed`](#c.dvb_demux_feed "dvb_demux_feed") pointer.

`index`
:   index of the used demux filter.

`state`
:   state of the filter as described by [`enum dvb_dmx_state`](#c.dvb_dmx_state "dvb_dmx_state").

`type`
:   type of the filter as described
    by [`enum dvb_dmx_filter_type`](#c.dvb_dmx_filter_type "dvb_dmx_filter_type").

struct dvb\_demux\_feed
:   describes a DVB field

**Definition**:

```
struct dvb_demux_feed {
    union {
        struct dmx_ts_feed ts;
        struct dmx_section_feed sec;
    } feed;
    union {
        dmx_ts_cb ts;
        dmx_section_cb sec;
    } cb;
    struct dvb_demux *demux;
    void *priv;
    enum dvb_dmx_filter_type type;
    enum dvb_dmx_state state;
    u16 pid;
    ktime_t timeout;
    struct dvb_demux_filter *filter;
    u32 buffer_flags;
    enum ts_filter_type ts_type;
    enum dmx_ts_pes pes_type;
    int cc;
    bool pusi_seen;
    u16 peslen;
    struct list_head list_head;
    unsigned int index;
};
```

**Members**

`feed`
:   a `union describing` a digital TV feed.
    Depending on the feed type, it can be either
    **feed.ts** or **feed.sec**.

`feed.ts`
:   a [`struct dmx_ts_feed`](#c.dmx_ts_feed "dmx_ts_feed") pointer.
    For TS feed only.

`feed.sec`
:   a [`struct dmx_section_feed`](#c.dmx_section_feed "dmx_section_feed") pointer.
    For section feed only.

`cb`
:   a `union describing` digital TV callbacks.
    Depending on the feed type, it can be either
    **cb.ts** or **cb.sec**.

`cb.ts`
:   a [`dmx_ts_cb()`](#c.dmx_ts_cb "dmx_ts_cb") calback function pointer.
    For TS feed only.

`cb.sec`
:   a [`dmx_section_cb()`](#c.dmx_section_cb "dmx_section_cb") callback function pointer.
    For section feed only.

`demux`
:   pointer to [`struct dvb_demux`](#c.dvb_demux "dvb_demux").

`priv`
:   private data that can optionally be used by a DVB driver.

`type`
:   type of the filter, as defined by [`enum dvb_dmx_filter_type`](#c.dvb_dmx_filter_type "dvb_dmx_filter_type").

`state`
:   state of the filter as defined by [`enum dvb_dmx_state`](#c.dvb_dmx_state "dvb_dmx_state").

`pid`
:   PID to be filtered.

`timeout`
:   feed timeout.

`filter`
:   pointer to [`struct dvb_demux_filter`](#c.dvb_demux_filter "dvb_demux_filter").

`buffer_flags`
:   Buffer flags used to report discontinuity users via DVB
    memory mapped API, as defined by `enum dmx_buffer_flags`.

`ts_type`
:   type of TS, as defined by [`enum ts_filter_type`](#c.ts_filter_type "ts_filter_type").

`pes_type`
:   type of PES, as defined by `enum dmx_ts_pes`.

`cc`
:   MPEG-TS packet continuity counter

`pusi_seen`
:   if true, indicates that a discontinuity was detected.
    it is used to prevent feeding of garbage from previous section.

`peslen`
:   length of the PES (Packet Elementary Stream).

`list_head`
:   head for the list of digital TV demux feeds.

`index`
:   a unique index for each feed. Can be used as hardware
    pid filter index.

struct dvb\_demux
:   represents a digital TV demux

**Definition**:

```
struct dvb_demux {
    struct dmx_demux dmx;
    void *priv;
    int filternum;
    int feednum;
    int (*start_feed)(struct dvb_demux_feed *feed);
    int (*stop_feed)(struct dvb_demux_feed *feed);
    int (*write_to_decoder)(struct dvb_demux_feed *feed, const u8 *buf, size_t len);
    u32 (*check_crc32)(struct dvb_demux_feed *feed, const u8 *buf, size_t len);
    void (*memcopy)(struct dvb_demux_feed *feed, u8 *dst, const u8 *src, size_t len);
    int users;
#define MAX_DVB_DEMUX_USERS 10;
    struct dvb_demux_filter *filter;
    struct dvb_demux_feed *feed;
    struct list_head frontend_list;
    struct dvb_demux_feed *pesfilter[DMX_PES_OTHER];
    u16 pids[DMX_PES_OTHER];
#define DMX_MAX_PID 0x2000;
    struct list_head feed_list;
    u8 tsbuf[204];
    int tsbufp;
    struct mutex mutex;
    spinlock_t lock;
    uint8_t *cnt_storage;
    ktime_t speed_last_time;
    uint32_t speed_pkts_cnt;
};
```

**Members**

`dmx`
:   embedded [`struct dmx_demux`](#c.dmx_demux "dmx_demux") with demux capabilities
    and callbacks.

`priv`
:   private data that can optionally be used by
    a DVB driver.

`filternum`
:   maximum amount of DVB filters.

`feednum`
:   maximum amount of DVB feeds.

`start_feed`
:   callback routine to be called in order to start
    a DVB feed.

`stop_feed`
:   callback routine to be called in order to stop
    a DVB feed.

`write_to_decoder`
:   callback routine to be called if the feed is TS and
    it is routed to an A/V decoder, when a new TS packet
    is received.
    Used only on av7110-av.c.

`check_crc32`
:   callback routine to check CRC. If not initialized,
    dvb\_demux will use an internal one.

`memcopy`
:   callback routine to memcopy received data.
    If not initialized, dvb\_demux will default to [`memcpy()`](../../core-api/kernel-api.html#c.memcpy "memcpy").

`users`
:   counter for the number of demux opened file descriptors.
    Currently, it is limited to 10 users.

`filter`
:   pointer to [`struct dvb_demux_filter`](#c.dvb_demux_filter "dvb_demux_filter").

`feed`
:   pointer to [`struct dvb_demux_feed`](#c.dvb_demux_feed "dvb_demux_feed").

`frontend_list`
:   `struct list_head` with frontends used by the demux.

`pesfilter`
:   array of [`struct dvb_demux_feed`](#c.dvb_demux_feed "dvb_demux_feed") with the PES types
    that will be filtered.

`pids`
:   list of filtered program IDs.

`feed_list`
:   `struct list_head` with feeds.

`tsbuf`
:   temporary buffer used internally to store TS packets.

`tsbufp`
:   temporary buffer index used internally.

`mutex`
:   pointer to `struct mutex` used to protect feed set
    logic.

`lock`
:   pointer to `spinlock_t`, used to protect buffer handling.

`cnt_storage`
:   buffer used for TS/TEI continuity check.

`speed_last_time`
:   `ktime_t` used for TS speed check.

`speed_pkts_cnt`
:   packets count used for TS speed check.

int dvb\_dmx\_init(struct [dvb\_demux](#c.dvb_demux "dvb_demux") \*demux)
:   initialize a digital TV demux struct.

**Parameters**

`struct dvb_demux *demux`
:   [`struct dvb_demux`](#c.dvb_demux "dvb_demux") to be initialized.

**Description**

Before being able to register a digital TV demux struct, drivers
should call this routine. On its typical usage, some fields should
be initialized at the driver before calling it.

A typical usecase is:

```
dvb->demux.dmx.capabilities =
        DMX_TS_FILTERING | DMX_SECTION_FILTERING |
        DMX_MEMORY_BASED_FILTERING;
dvb->demux.priv       = dvb;
dvb->demux.filternum  = 256;
dvb->demux.feednum    = 256;
dvb->demux.start_feed = driver_start_feed;
dvb->demux.stop_feed  = driver_stop_feed;
ret = dvb_dmx_init(&dvb->demux);
if (ret < 0)
        return ret;
```

void dvb\_dmx\_release(struct [dvb\_demux](#c.dvb_demux "dvb_demux") \*demux)
:   releases a digital TV demux internal buffers.

**Parameters**

`struct dvb_demux *demux`
:   [`struct dvb_demux`](#c.dvb_demux "dvb_demux") to be released.

**Description**

The DVB core internally allocates data at **demux**. This routine
releases those data. Please notice that the `struct itelf` is not
released, as it can be embedded on other structs.

void dvb\_dmx\_swfilter\_packets(struct [dvb\_demux](#c.dvb_demux "dvb_demux") \*demux, const u8 \*buf, size\_t count)
:   use dvb software filter for a buffer with multiple MPEG-TS packets with 188 bytes each.

**Parameters**

`struct dvb_demux *demux`
:   pointer to [`struct dvb_demux`](#c.dvb_demux "dvb_demux")

`const u8 *buf`
:   buffer with data to be filtered

`size_t count`
:   number of MPEG-TS packets with size of 188.

**Description**

The routine will discard a DVB packet that don’t start with 0x47.

Use this routine if the DVB demux fills MPEG-TS buffers that are
already aligned.

**NOTE**

The **buf** size should have size equal to `count * 188`.

void dvb\_dmx\_swfilter(struct [dvb\_demux](#c.dvb_demux "dvb_demux") \*demux, const u8 \*buf, size\_t count)
:   use dvb software filter for a buffer with multiple MPEG-TS packets with 188 bytes each.

**Parameters**

`struct dvb_demux *demux`
:   pointer to [`struct dvb_demux`](#c.dvb_demux "dvb_demux")

`const u8 *buf`
:   buffer with data to be filtered

`size_t count`
:   number of MPEG-TS packets with size of 188.

**Description**

If a DVB packet doesn’t start with 0x47, it will seek for the first
byte that starts with 0x47.

Use this routine if the DVB demux fill buffers that may not start with
a packet start mark (0x47).

**NOTE**

The **buf** size should have size equal to `count * 188`.

void dvb\_dmx\_swfilter\_204(struct [dvb\_demux](#c.dvb_demux "dvb_demux") \*demux, const u8 \*buf, size\_t count)
:   use dvb software filter for a buffer with multiple MPEG-TS packets with 204 bytes each.

**Parameters**

`struct dvb_demux *demux`
:   pointer to [`struct dvb_demux`](#c.dvb_demux "dvb_demux")

`const u8 *buf`
:   buffer with data to be filtered

`size_t count`
:   number of MPEG-TS packets with size of 204.

**Description**

If a DVB packet doesn’t start with 0x47, it will seek for the first
byte that starts with 0x47.

Use this routine if the DVB demux fill buffers that may not start with
a packet start mark (0x47).

**NOTE**

The **buf** size should have size equal to `count * 204`.

void dvb\_dmx\_swfilter\_raw(struct [dvb\_demux](#c.dvb_demux "dvb_demux") \*demux, const u8 \*buf, size\_t count)
:   make the raw data available to userspace without filtering

**Parameters**

`struct dvb_demux *demux`
:   pointer to [`struct dvb_demux`](#c.dvb_demux "dvb_demux")

`const u8 *buf`
:   buffer with data

`size_t count`
:   number of packets to be passed. The actual size of each packet
    depends on the [`dvb_demux->feed`](#c.dvb_demux "dvb_demux")->cb.ts logic.

**Description**

Use it if the driver needs to deliver the raw payload to userspace without
passing through the kernel demux. That is meant to support some
delivery systems that aren’t based on MPEG-TS.

This function relies on [`dvb_demux->feed`](#c.dvb_demux "dvb_demux")->cb.ts to actually handle the
buffer.

## 4.3.5. Driver-internal low-level hardware specific driver demux interface

enum ts\_filter\_type
:   filter type bitmap for dmx\_ts\_feed.`set()`

**Constants**

`TS_PACKET`
:   Send TS packets (188 bytes) to callback (default).

`TS_PAYLOAD_ONLY`
:   In case TS\_PACKET is set, only send the TS payload
    (<=184 bytes per packet) to callback

`TS_DECODER`
:   Send stream to built-in decoder (if present).

`TS_DEMUX`
:   In case TS\_PACKET is set, send the TS to the demux
    device, not to the dvr device

struct dmx\_ts\_feed
:   Structure that contains a TS feed filter

**Definition**:

```
struct dmx_ts_feed {
    int is_filtering;
    struct dmx_demux *parent;
    void *priv;
    int (*set)(struct dmx_ts_feed *feed, u16 pid, int type, enum dmx_ts_pes pes_type, ktime_t timeout);
    int (*start_filtering)(struct dmx_ts_feed *feed);
    int (*stop_filtering)(struct dmx_ts_feed *feed);
};
```

**Members**

`is_filtering`
:   Set to non-zero when filtering in progress

`parent`
:   pointer to [`struct dmx_demux`](#c.dmx_demux "dmx_demux")

`priv`
:   pointer to private data of the API client

`set`
:   sets the TS filter

`start_filtering`
:   starts TS filtering

`stop_filtering`
:   stops TS filtering

**Description**

A TS feed is typically mapped to a hardware PID filter on the demux chip.
Using this API, the client can set the filtering properties to start/stop
filtering TS packets on a particular TS feed.

struct dmx\_section\_filter
:   Structure that describes a section filter

**Definition**:

```
struct dmx_section_filter {
    u8 filter_value[DMX_MAX_FILTER_SIZE];
    u8 filter_mask[DMX_MAX_FILTER_SIZE];
    u8 filter_mode[DMX_MAX_FILTER_SIZE];
    struct dmx_section_feed *parent;
    void *priv;
};
```

**Members**

`filter_value`
:   Contains up to 16 bytes (128 bits) of the TS section header
    that will be matched by the section filter

`filter_mask`
:   Contains a 16 bytes (128 bits) filter mask with the bits
    specified by **filter\_value** that will be used on the filter
    match logic.

`filter_mode`
:   Contains a 16 bytes (128 bits) filter mode.

`parent`
:   Back-pointer to [`struct dmx_section_feed`](#c.dmx_section_feed "dmx_section_feed").

`priv`
:   Pointer to private data of the API client.

**Description**

The **filter\_mask** controls which bits of **filter\_value** are compared with
the section headers/payload. On a binary value of 1 in filter\_mask, the
corresponding bits are compared. The filter only accepts sections that are
equal to filter\_value in all the tested bit positions.

struct dmx\_section\_feed
:   Structure that contains a section feed filter

**Definition**:

```
struct dmx_section_feed {
    int is_filtering;
    struct dmx_demux *parent;
    void *priv;
    int check_crc;
    int (*set)(struct dmx_section_feed *feed, u16 pid, int check_crc);
    int (*allocate_filter)(struct dmx_section_feed *feed, struct dmx_section_filter **filter);
    int (*release_filter)(struct dmx_section_feed *feed, struct dmx_section_filter *filter);
    int (*start_filtering)(struct dmx_section_feed *feed);
    int (*stop_filtering)(struct dmx_section_feed *feed);
};
```

**Members**

`is_filtering`
:   Set to non-zero when filtering in progress

`parent`
:   pointer to [`struct dmx_demux`](#c.dmx_demux "dmx_demux")

`priv`
:   pointer to private data of the API client

`check_crc`
:   If non-zero, check the CRC values of filtered sections.

`set`
:   sets the section filter

`allocate_filter`
:   This function is used to allocate a section filter on
    the demux. It should only be called when no filtering
    is in progress on this section feed. If a filter cannot
    be allocated, the function fails with -ENOSPC.

`release_filter`
:   This function releases all the resources of a
    previously allocated section filter. The function
    should not be called while filtering is in progress
    on this section feed. After calling this function,
    the caller should not try to dereference the filter
    pointer.

`start_filtering`
:   starts section filtering

`stop_filtering`
:   stops section filtering

**Description**

A TS feed is typically mapped to a hardware PID filter on the demux chip.
Using this API, the client can set the filtering properties to start/stop
filtering TS packets on a particular TS feed.

dmx\_ts\_cb
:   **Typedef**: DVB demux TS filter callback function prototype

**Syntax**

> `int dmx_ts_cb (const u8 *buffer1, size_t buffer1_length, const u8 *buffer2, size_t buffer2_length, struct dmx_ts_feed *source, u32 *buffer_flags)`

**Parameters**

`const u8 *buffer1`
:   Pointer to the start of the filtered TS packets.

`size_t buffer1_length`
:   Length of the TS data in buffer1.

`const u8 *buffer2`
:   Pointer to the tail of the filtered TS packets, or NULL.

`size_t buffer2_length`
:   Length of the TS data in buffer2.

`struct dmx_ts_feed *source`
:   Indicates which TS feed is the source of the callback.

`u32 *buffer_flags`
:   Address where buffer flags are stored. Those are
    used to report discontinuity users via DVB
    memory mapped API, as defined by
    `enum dmx_buffer_flags`.

**Description**

This function callback prototype, provided by the client of the demux API,
is called from the demux code. The function is only called when filtering
on a TS feed has been enabled using the `start_filtering()` function at
the [`dmx_demux`](#c.dmx_demux "dmx_demux").
Any TS packets that match the filter settings are copied to a circular
buffer. The filtered TS packets are delivered to the client using this
callback function.
It is expected that the **buffer1** and **buffer2** callback parameters point to
addresses within the circular buffer, but other implementations are also
possible. Note that the called party should not try to free the memory
the **buffer1** and **buffer2** parameters point to.

When this function is called, the **buffer1** parameter typically points to
the start of the first undelivered TS packet within a circular buffer.
The **buffer2** buffer parameter is normally NULL, except when the received
TS packets have crossed the last address of the circular buffer and
“wrapped” to the beginning of the buffer. In the latter case the **buffer1**
parameter would contain an address within the circular buffer, while the
**buffer2** parameter would contain the first address of the circular buffer.
The number of bytes delivered with this function (i.e. **buffer1\_length** +
**buffer2\_length**) is usually equal to the value of callback\_length parameter
given in the `set()` function, with one exception: if a timeout occurs before
receiving callback\_length bytes of TS data, any undelivered packets are
immediately delivered to the client by calling this function. The timeout
duration is controlled by the `set()` function in the TS Feed API.

If a TS packet is received with errors that could not be fixed by the
TS-level forward error correction (FEC), the Transport\_error\_indicator
flag of the TS packet header should be set. The TS packet should not be
discarded, as the error can possibly be corrected by a higher layer
protocol. If the called party is slow in processing the callback, it
is possible that the circular buffer eventually fills up. If this happens,
the demux driver should discard any TS packets received while the buffer
is full and return -EOVERFLOW.

The type of data returned to the callback can be selected by the
[`dmx_ts_feed`](#c.dmx_ts_feed "dmx_ts_feed").\*\*set\*\* function. The type parameter decides if the raw
TS packet (TS\_PACKET) or just the payload (TS\_PACKET|TS\_PAYLOAD\_ONLY)
should be returned. If additionally the TS\_DECODER bit is set the stream
will also be sent to the hardware MPEG decoder.

* 0, on success;
* -EOVERFLOW, on buffer overflow.

dmx\_section\_cb
:   **Typedef**: DVB demux TS filter callback function prototype

**Syntax**

> `int dmx_section_cb (const u8 *buffer1, size_t buffer1_len, const u8 *buffer2, size_t buffer2_len, struct dmx_section_filter *source, u32 *buffer_flags)`

**Parameters**

`const u8 *buffer1`
:   Pointer to the start of the filtered section, e.g.
    within the circular buffer of the demux driver.

`size_t buffer1_len`
:   Length of the filtered section data in **buffer1**,
    including headers and CRC.

`const u8 *buffer2`
:   Pointer to the tail of the filtered section data,
    or NULL. Useful to handle the wrapping of a
    circular buffer.

`size_t buffer2_len`
:   Length of the filtered section data in **buffer2**,
    including headers and CRC.

`struct dmx_section_filter *source`
:   Indicates which section feed is the source of the
    callback.

`u32 *buffer_flags`
:   Address where buffer flags are stored. Those are
    used to report discontinuity users via DVB
    memory mapped API, as defined by
    `enum dmx_buffer_flags`.

**Description**

This function callback prototype, provided by the client of the demux API,
is called from the demux code. The function is only called when
filtering of sections has been enabled using the function
[`dmx_ts_feed`](#c.dmx_ts_feed "dmx_ts_feed").\*\*start\_filtering\*\*. When the demux driver has received a
complete section that matches at least one section filter, the client
is notified via this callback function. Normally this function is called
for each received section; however, it is also possible to deliver
multiple sections with one callback, for example when the system load
is high. If an error occurs while receiving a section, this
function should be called with the corresponding error type set in the
success field, whether or not there is data to deliver. The Section Feed
implementation should maintain a circular buffer for received sections.
However, this is not necessary if the Section Feed API is implemented as
a client of the TS Feed API, because the TS Feed implementation then
buffers the received data. The size of the circular buffer can be
configured using the [`dmx_ts_feed`](#c.dmx_ts_feed "dmx_ts_feed").\*\*set\*\* function in the Section Feed API.
If there is no room in the circular buffer when a new section is received,
the section must be discarded. If this happens, the value of the success
parameter should be DMX\_OVERRUN\_ERROR on the next callback.

enum dmx\_frontend\_source
:   Used to identify the type of frontend

**Constants**

`DMX_MEMORY_FE`
:   The source of the demux is memory. It means that
    the MPEG-TS to be filtered comes from userspace,
    via write() syscall.

`DMX_FRONTEND_0`
:   The source of the demux is a frontend connected
    to the demux.

struct dmx\_frontend
:   Structure that lists the frontends associated with a demux

**Definition**:

```
struct dmx_frontend {
    struct list_head connectivity_list;
    enum dmx_frontend_source source;
};
```

**Members**

`connectivity_list`
:   List of front-ends that can be connected to a
    particular demux;

`source`
:   Type of the frontend.

**Description**

FIXME: this structure should likely be replaced soon by some
:   media-controller based logic.

enum dmx\_demux\_caps
:   MPEG-2 TS Demux capabilities bitmap

**Constants**

`DMX_TS_FILTERING`
:   set if TS filtering is supported;

`DMX_SECTION_FILTERING`
:   set if section filtering is supported;

`DMX_MEMORY_BASED_FILTERING`
:   set if write() available.

**Description**

Those flags are OR’ed in the [`dmx_demux.capabilities`](#c.dmx_demux "dmx_demux") field

DMX\_FE\_ENTRY

`DMX_FE_ENTRY (list)`

> Casts elements in the list of registered front-ends from the generic type `struct list_head` to the type \* [`struct dmx_frontend`](#c.dmx_frontend "dmx_frontend")

**Parameters**

`list`
:   list of [`struct dmx_frontend`](#c.dmx_frontend "dmx_frontend")

struct dmx\_demux
:   Structure that contains the demux capabilities and callbacks.

**Definition**:

```
struct dmx_demux {
    enum dmx_demux_caps capabilities;
    struct dmx_frontend *frontend;
    void *priv;
    int (*open)(struct dmx_demux *demux);
    int (*close)(struct dmx_demux *demux);
    int (*write)(struct dmx_demux *demux, const char __user *buf, size_t count);
    int (*allocate_ts_feed)(struct dmx_demux *demux, struct dmx_ts_feed **feed, dmx_ts_cb callback);
    int (*release_ts_feed)(struct dmx_demux *demux, struct dmx_ts_feed *feed);
    int (*allocate_section_feed)(struct dmx_demux *demux, struct dmx_section_feed **feed, dmx_section_cb callback);
    int (*release_section_feed)(struct dmx_demux *demux, struct dmx_section_feed *feed);
    int (*add_frontend)(struct dmx_demux *demux, struct dmx_frontend *frontend);
    int (*remove_frontend)(struct dmx_demux *demux, struct dmx_frontend *frontend);
    struct list_head *(*get_frontends)(struct dmx_demux *demux);
    int (*connect_frontend)(struct dmx_demux *demux, struct dmx_frontend *frontend);
    int (*disconnect_frontend)(struct dmx_demux *demux);
    int (*get_pes_pids)(struct dmx_demux *demux, u16 *pids);
};
```

**Members**

`capabilities`
:   Bitfield of capability flags.

`frontend`
:   Front-end connected to the demux

`priv`
:   Pointer to private data of the API client

`open`
:   This function reserves the demux for use by the caller and, if
    necessary, initializes the demux. When the demux is no longer needed,
    the function **close** should be called. It should be possible for
    multiple clients to access the demux at the same time. Thus, the
    function implementation should increment the demux usage count when
    **open** is called and decrement it when **close** is called.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    It returns:
    0 on success;
    -EUSERS, if maximum usage count was reached;
    -EINVAL, on bad parameter.

`close`
:   This function reserves the demux for use by the caller and, if
    necessary, initializes the demux. When the demux is no longer needed,
    the function **close** should be called. It should be possible for
    multiple clients to access the demux at the same time. Thus, the
    function implementation should increment the demux usage count when
    **open** is called and decrement it when **close** is called.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    It returns:
    0 on success;
    -ENODEV, if demux was not in use (e. g. no users);
    -EINVAL, on bad parameter.

`write`
:   This function provides the demux driver with a memory buffer
    containing TS packets. Instead of receiving TS packets from the DVB
    front-end, the demux driver software will read packets from memory.
    Any clients of this demux with active TS, PES or Section filters will
    receive filtered data via the Demux callback API (see 0). The function
    returns when all the data in the buffer has been consumed by the demux.
    Demux hardware typically cannot read TS from memory. If this is the
    case, memory-based filtering has to be implemented entirely in software.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **buf** function parameter contains a pointer to the TS data in
    kernel-space memory.
    The **count** function parameter contains the length of the TS data.
    It returns:
    0 on success;
    -ERESTARTSYS, if mutex lock was interrupted;
    -EINTR, if a signal handling is pending;
    -ENODEV, if demux was removed;
    -EINVAL, on bad parameter.

`allocate_ts_feed`
:   Allocates a new TS feed, which is used to filter the TS
    packets carrying a certain PID. The TS feed normally corresponds to a
    hardware PID filter on the demux chip.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **feed** function parameter contains a pointer to the TS feed API and
    instance data.
    The **callback** function parameter contains a pointer to the callback
    function for passing received TS packet.
    It returns:
    0 on success;
    -ERESTARTSYS, if mutex lock was interrupted;
    -EBUSY, if no more TS feeds is available;
    -EINVAL, on bad parameter.

`release_ts_feed`
:   Releases the resources allocated with **allocate\_ts\_feed**.
    Any filtering in progress on the TS feed should be stopped before
    calling this function.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **feed** function parameter contains a pointer to the TS feed API and
    instance data.
    It returns:
    0 on success;
    -EINVAL on bad parameter.

`allocate_section_feed`
:   Allocates a new section feed, i.e. a demux resource
    for filtering and receiving sections. On platforms with hardware
    support for section filtering, a section feed is directly mapped to
    the demux HW. On other platforms, TS packets are first PID filtered in
    hardware and a hardware section filter then emulated in software. The
    caller obtains an API pointer of type dmx\_section\_feed\_t as an out
    parameter. Using this API the caller can set filtering parameters and
    start receiving sections.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **feed** function parameter contains a pointer to the TS feed API and
    instance data.
    The **callback** function parameter contains a pointer to the callback
    function for passing received TS packet.
    It returns:
    0 on success;
    -EBUSY, if no more TS feeds is available;
    -EINVAL, on bad parameter.

`release_section_feed`
:   Releases the resources allocated with
    **allocate\_section\_feed**, including allocated filters. Any filtering in
    progress on the section feed should be stopped before calling this
    function.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **feed** function parameter contains a pointer to the TS feed API and
    instance data.
    It returns:
    0 on success;
    -EINVAL, on bad parameter.

`add_frontend`
:   Registers a connectivity between a demux and a front-end,
    i.e., indicates that the demux can be connected via a call to
    **connect\_frontend** to use the given front-end as a TS source. The
    client of this function has to allocate dynamic or static memory for
    the frontend structure and initialize its fields before calling this
    function. This function is normally called during the driver
    initialization. The caller must not free the memory of the frontend
    `struct before` successfully calling **remove\_frontend**.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **frontend** function parameter contains a pointer to the front-end
    instance data.
    It returns:
    0 on success;
    -EINVAL, on bad parameter.

`remove_frontend`
:   Indicates that the given front-end, registered by a call
    to **add\_frontend**, can no longer be connected as a TS source by this
    demux. The function should be called when a front-end driver or a demux
    driver is removed from the system. If the front-end is in use, the
    function fails with the return value of -EBUSY. After successfully
    calling this function, the caller can free the memory of the frontend
    struct if it was dynamically allocated before the **add\_frontend**
    operation.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **frontend** function parameter contains a pointer to the front-end
    instance data.
    It returns:
    0 on success;
    -ENODEV, if the front-end was not found,
    -EINVAL, on bad parameter.

`get_frontends`
:   Provides the APIs of the front-ends that have been
    registered for this demux. Any of the front-ends obtained with this
    call can be used as a parameter for **connect\_frontend**. The include
    file demux.h contains the macro [`DMX_FE_ENTRY()`](#c.DMX_FE_ENTRY "DMX_FE_ENTRY") for converting an
    element of the generic type struct `list_head` \* to the type
    struct [`dmx_frontend`](#c.dmx_frontend "dmx_frontend") *. The caller must not free the memory of any of
    the elements obtained via this function call.
    The \*\*demux\** function parameter contains a pointer to the demux API and
    instance data.
    It returns a `struct list_head` pointer to the list of front-end
    interfaces, or NULL in the case of an empty list.

`connect_frontend`
:   Connects the TS output of the front-end to the input of
    the demux. A demux can only be connected to a front-end registered to
    the demux with the function **add\_frontend**. It may or may not be
    possible to connect multiple demuxes to the same front-end, depending
    on the capabilities of the HW platform. When not used, the front-end
    should be released by calling **disconnect\_frontend**.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **frontend** function parameter contains a pointer to the front-end
    instance data.
    It returns:
    0 on success;
    -EINVAL, on bad parameter.

`disconnect_frontend`
:   Disconnects the demux and a front-end previously
    connected by a **connect\_frontend** call.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    It returns:
    0 on success;
    -EINVAL on bad parameter.

`get_pes_pids`
:   Get the PIDs for DMX\_PES\_AUDIO0, DMX\_PES\_VIDEO0,
    DMX\_PES\_TELETEXT0, DMX\_PES\_SUBTITLE0 and DMX\_PES\_PCR0.
    The **demux** function parameter contains a pointer to the demux API and
    instance data.
    The **pids** function parameter contains an array with five u16 elements
    where the PIDs will be stored.
    It returns:
    0 on success;
    -EINVAL on bad parameter.
