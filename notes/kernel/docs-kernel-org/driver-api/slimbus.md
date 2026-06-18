# Linux kernel SLIMbus support

> 출처(원문): https://docs.kernel.org/driver-api/slimbus.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux kernel SLIMbus support

## Overview

### What is SLIMbus?

SLIMbus (Serial Low Power Interchip Media Bus) is a specification developed by
MIPI (Mobile Industry Processor Interface) alliance. The bus uses master/slave
configuration, and is a 2-wire multi-drop implementation (clock, and data).

Currently, SLIMbus is used to interface between application processors of SoCs
(System-on-Chip) and peripheral components (typically codec). SLIMbus uses
Time-Division-Multiplexing to accommodate multiple data channels, and
a control channel.

The control channel is used for various control functions such as bus
management, configuration and status updates. These messages can be unicast (e.g.
reading/writing device specific values), or multicast (e.g. data channel
reconfiguration sequence is a broadcast message announced to all devices)

A data channel is used for data-transfer between 2 SLIMbus devices. Data
channel uses dedicated ports on the device.

### Hardware description:

SLIMbus specification has different types of device classifications based on
their capabilities.
A manager device is responsible for enumeration, configuration, and dynamic
channel allocation. Every bus has 1 active manager.

A generic device is a device providing application functionality (e.g. codec).

Framer device is responsible for clocking the bus, and transmitting frame-sync
and framing information on the bus.

Each SLIMbus component has an interface device for monitoring physical layer.

Typically each SoC contains SLIMbus component having 1 manager, 1 framer device,
1 generic device (for data channel support), and 1 interface device.
External peripheral SLIMbus component usually has 1 generic device (for
functionality/data channel support), and an associated interface device.
The generic device’s registers are mapped as ‘value elements’ so that they can
be written/read using SLIMbus control channel exchanging control/status type of
information.
In case there are multiple framer devices on the same bus, manager device is
responsible to select the active-framer for clocking the bus.

Per specification, SLIMbus uses “clock gears” to do power management based on
current frequency and bandwidth requirements. There are 10 clock gears and each
gear changes the SLIMbus frequency to be twice its previous gear.

Each device has a 6-byte enumeration-address and the manager assigns every
device with a 1-byte logical address after the devices report presence on the
bus.

### Software description:

There are 2 types of SLIMbus drivers:

slim\_controller represents a ‘controller’ for SLIMbus. This driver should
implement duties needed by the SoC (manager device, associated
interface device for monitoring the layers and reporting errors, default
framer device).

slim\_device represents the ‘generic device/component’ for SLIMbus, and a
slim\_driver should implement driver for that slim\_device.

### Device notifications to the driver:

Since SLIMbus devices have mechanisms for reporting their presence, the
framework allows drivers to bind when corresponding devices report their
presence on the bus.
However, it is possible that the driver needs to be probed
first so that it can enable corresponding SLIMbus device (e.g. power it up and/or
take it out of reset). To support that behavior, the framework allows drivers
to probe first as well (e.g. using standard DeviceTree compatibility field).
This creates the necessity for the driver to know when the device is functional
(i.e. reported present). device\_up callback is used for that reason when the
device reports present and is assigned a logical address by the controller.

Similarly, SLIMbus devices ‘report absent’ when they go down. A ‘device\_down’
callback notifies the driver when the device reports absent and its logical
address assignment is invalidated by the controller.

Another notification “boot\_device” is used to notify the slim\_driver when
controller resets the bus. This notification allows the driver to take necessary
steps to boot the device so that it’s functional after the bus has been reset.

### Driver and Controller APIs:

struct slim\_eaddr
:   Enumeration address for a SLIMbus device

**Definition**:

```
struct slim_eaddr {
    u8 instance;
    u8 dev_index;
    u16 prod_code;
    u16 manf_id;
};
```

**Members**

`instance`
:   Instance value

`dev_index`
:   Device index

`prod_code`
:   Product code

`manf_id`
:   Manufacturer Id for the device

enum slim\_device\_status
:   slim device status

**Constants**

`SLIM_DEVICE_STATUS_DOWN`
:   Slim device is absent or not reported yet.

`SLIM_DEVICE_STATUS_UP`
:   Slim device is announced on the bus.

`SLIM_DEVICE_STATUS_RESERVED`
:   Reserved for future use.

struct slim\_device
:   Slim device handle.

**Definition**:

```
struct slim_device {
    struct device           dev;
    struct slim_eaddr       e_addr;
    struct slim_controller  *ctrl;
    enum slim_device_status status;
    u8 laddr;
    bool is_laddr_valid;
    struct list_head        stream_list;
    spinlock_t stream_list_lock;
};
```

**Members**

`dev`
:   Driver model representation of the device.

`e_addr`
:   Enumeration address of this device.

`ctrl`
:   slim controller instance.

`status`
:   slim device status

`laddr`
:   1-byte Logical address of this device.

`is_laddr_valid`
:   indicates if the laddr is valid or not

`stream_list`
:   List of streams on this device

`stream_list_lock`
:   lock to protect the stream list

**Description**

This is the client/device handle returned when a SLIMbus
device is registered with a controller.
Pointer to this structure is used by client-driver as a handle.

struct slim\_driver
:   SLIMbus ‘generic device’ (slave) device driver (similar to ‘spi\_device’ on SPI)

**Definition**:

```
struct slim_driver {
    int (*probe)(struct slim_device *sl);
    void (*remove)(struct slim_device *sl);
    void (*shutdown)(struct slim_device *sl);
    int (*device_status)(struct slim_device *sl, enum slim_device_status s);
    struct device_driver            driver;
    const struct slim_device_id     *id_table;
};
```

**Members**

`probe`
:   Binds this driver to a SLIMbus device.

`remove`
:   Unbinds this driver from the SLIMbus device.

`shutdown`
:   Standard shutdown callback used during powerdown/halt.

`device_status`
:   This callback is called when
    - The device reports present and gets a laddr assigned
    - The device reports absent, or the bus goes down.

`driver`
:   SLIMbus device drivers should initialize name and owner field of
    this structure

`id_table`
:   List of SLIMbus devices supported by this driver

struct slim\_val\_inf
:   Slimbus value or information element

**Definition**:

```
struct slim_val_inf {
    u16 start_offset;
    u8 num_bytes;
    u8 *rbuf;
    const u8                *wbuf;
    struct completion      *comp;
};
```

**Members**

`start_offset`
:   Specifies starting offset in information/value element map

`num_bytes`
:   upto 16. This ensures that the message will fit the slicesize
    per SLIMbus spec

`rbuf`
:   buffer to read the values

`wbuf`
:   buffer to write

`comp`
:   completion for asynchronous operations, valid only if TID is
    required for transaction, like REQUEST operations.
    Rest of the transactions are synchronous anyway.

struct slim\_stream\_config
:   SLIMbus stream configuration Configuring a stream is done at hw\_params or prepare call from audio drivers where they have all the required information regarding rate, number of channels and so on. There is a 1:1 mapping of channel and ports.

**Definition**:

```
struct slim_stream_config {
    unsigned int rate;
    unsigned int bps;
    unsigned int ch_count;
    unsigned int *chs;
    unsigned long port_mask;
    int direction;
};
```

**Members**

`rate`
:   data rate

`bps`
:   bits per data sample

`ch_count`
:   number of channels

`chs`
:   pointer to list of channel numbers

`port_mask`
:   port mask of ports to use for this stream

`direction`
:   direction of the stream, SNDRV\_PCM\_STREAM\_PLAYBACK
    or SNDRV\_PCM\_STREAM\_CAPTURE.

module\_slim\_driver

`module_slim_driver (__slim_driver)`

> Helper macro for registering a SLIMbus driver

**Parameters**

`__slim_driver`
:   slimbus\_driver struct

**Description**

Helper macro for SLIMbus drivers which do not do anything special in module
init/exit. This eliminates a lot of boilerplate. Each module may only
use this macro once, and calling it replaces [`module_init()`](basics.html#c.module_init "module_init") and [`module_exit()`](basics.html#c.module_exit "module_exit")

struct slim\_framer
:   Represents SLIMbus framer. Every controller may have multiple framers. There is 1 active framer device responsible for clocking the bus. Manager is responsible for framer hand-over.

**Definition**:

```
struct slim_framer {
    struct device           dev;
    struct slim_eaddr       e_addr;
    int rootfreq;
    int superfreq;
};
```

**Members**

`dev`
:   Driver model representation of the device.

`e_addr`
:   Enumeration address of the framer.

`rootfreq`
:   Root Frequency at which the framer can run. This is maximum
    frequency (‘clock gear 10’) at which the bus can operate.

`superfreq`
:   Superframes per root frequency. Every frame is 6144 bits.

struct slim\_msg\_txn
:   Message to be sent by the controller. This structure has packet header, payload and buffer to be filled (if any)

**Definition**:

```
struct slim_msg_txn {
    u8 rl;
    u8 mt;
    u8 mc;
    u8 dt;
    u16 ec;
    u8 tid;
    u8 la;
    struct slim_val_inf     *msg;
    struct completion      *comp;
};
```

**Members**

`rl`
:   Header field. remaining length.

`mt`
:   Header field. Message type.

`mc`
:   Header field. LSB is message code for type mt.

`dt`
:   Header field. Destination type.

`ec`
:   Element code. Used for elemental access APIs.

`tid`
:   Transaction ID. Used for messages expecting response.
    (relevant for message-codes involving read operation)

`la`
:   Logical address of the device this message is going to.
    (Not used when destination type is broadcast.)

`msg`
:   Elemental access message to be read/written

`comp`
:   completion if read/write is synchronous, used internally
    for tid based transactions.

enum slim\_clk\_state
:   SLIMbus controller’s clock state used internally for maintaining current clock state.

**Constants**

`SLIM_CLK_ACTIVE`
:   SLIMbus clock is active

`SLIM_CLK_ENTERING_PAUSE`
:   SLIMbus clock pause sequence is being sent on the
    bus. If this succeeds, state changes to SLIM\_CLK\_PAUSED. If the
    transition fails, state changes back to SLIM\_CLK\_ACTIVE

`SLIM_CLK_PAUSED`
:   SLIMbus controller clock has paused.

struct slim\_sched
:   Framework uses this structure internally for scheduling.

**Definition**:

```
struct slim_sched {
    enum slim_clk_state     clk_state;
    struct completion       pause_comp;
    struct mutex            m_reconf;
};
```

**Members**

`clk_state`
:   Controller’s clock state from [`enum slim_clk_state`](#c.slim_clk_state "slim_clk_state")

`pause_comp`
:   Signals completion of clock pause sequence. This is useful when
    client tries to call SLIMbus transaction when controller is entering
    clock pause.

`m_reconf`
:   This mutex is held until current reconfiguration (data channel
    scheduling, message bandwidth reservation) is done. Message APIs can
    use the bus concurrently when this mutex is held since elemental access
    messages can be sent on the bus when reconfiguration is in progress.

enum slim\_port\_direction
:   SLIMbus port direction

**Constants**

`SLIM_PORT_SINK`
:   SLIMbus port is a sink

`SLIM_PORT_SOURCE`
:   SLIMbus port is a source

enum slim\_port\_state
:   SLIMbus Port/Endpoint state machine according to SLIMbus Spec 2.0

**Constants**

`SLIM_PORT_DISCONNECTED`
:   SLIMbus port is disconnected
    entered from Unconfigure/configured state after
    DISCONNECT\_PORT or REMOVE\_CHANNEL core command

`SLIM_PORT_UNCONFIGURED`
:   SLIMbus port is in unconfigured state.
    entered from disconnect state after CONNECT\_SOURCE/SINK core command

`SLIM_PORT_CONFIGURED`
:   SLIMbus port is in configured state.
    entered from unconfigured state after DEFINE\_CHANNEL, DEFINE\_CONTENT
    and ACTIVATE\_CHANNEL core commands. Ready for data transmission.

enum slim\_channel\_state
:   SLIMbus channel state machine used by core.

**Constants**

`SLIM_CH_STATE_DISCONNECTED`
:   SLIMbus channel is disconnected

`SLIM_CH_STATE_ALLOCATED`
:   SLIMbus channel is allocated

`SLIM_CH_STATE_ASSOCIATED`
:   SLIMbus channel is associated with port

`SLIM_CH_STATE_DEFINED`
:   SLIMbus channel parameters are defined

`SLIM_CH_STATE_CONTENT_DEFINED`
:   SLIMbus channel content is defined

`SLIM_CH_STATE_ACTIVE`
:   SLIMbus channel is active and ready for data

`SLIM_CH_STATE_REMOVED`
:   SLIMbus channel is inactive and removed

enum slim\_ch\_data\_fmt
:   SLIMbus channel data Type identifiers according to Table 60 of SLIMbus Spec 1.01.01

**Constants**

`SLIM_CH_DATA_FMT_NOT_DEFINED`
:   Undefined

`SLIM_CH_DATA_FMT_LPCM_AUDIO`
:   LPCM audio

`SLIM_CH_DATA_FMT_IEC61937_COMP_AUDIO`
:   IEC61937 Compressed audio

`SLIM_CH_DATA_FMT_PACKED_PDM_AUDIO`
:   Packed PDM audio

enum slim\_ch\_aux\_bit\_fmt
:   SLIMbus channel Aux Field format IDs according to Table 63 of SLIMbus Spec 2.0

**Constants**

`SLIM_CH_AUX_FMT_NOT_APPLICABLE`
:   Undefined

`SLIM_CH_AUX_FMT_ZCUV_TUNNEL_IEC60958`
:   ZCUV for tunneling IEC60958

`SLIM_CH_AUX_FMT_USER_DEFINED`
:   User defined

struct slim\_channel
:   SLIMbus channel, used for state machine

**Definition**:

```
struct slim_channel {
    int id;
    int prrate;
    int seg_dist;
    enum slim_ch_data_fmt data_fmt;
    enum slim_ch_aux_bit_fmt aux_fmt;
    enum slim_channel_state state;
};
```

**Members**

`id`
:   ID of channel

`prrate`
:   Presense rate of channel from Table 66 of SLIMbus 2.0 Specs

`seg_dist`
:   segment distribution code from Table 20 of SLIMbus 2.0 Specs

`data_fmt`
:   Data format of channel.

`aux_fmt`
:   Aux format for this channel.

`state`
:   channel state machine

struct slim\_port
:   SLIMbus port

**Definition**:

```
struct slim_port {
    int id;
    enum slim_port_direction direction;
    enum slim_port_state state;
    struct slim_channel ch;
};
```

**Members**

`id`
:   Port id

`direction`
:   Port direction, Source or Sink.

`state`
:   state machine of port.

`ch`
:   channel associated with this port.

enum slim\_transport\_protocol
:   SLIMbus Transport protocol list from Table 47 of SLIMbus 2.0 specs.

**Constants**

`SLIM_PROTO_ISO`
:   Isochronous Protocol, no flow control as data rate match
    channel rate flow control embedded in the data.

`SLIM_PROTO_PUSH`
:   Pushed Protocol, includes flow control, Used to carry
    data whose rate is equal to, or lower than the channel rate.

`SLIM_PROTO_PULL`
:   Pulled Protocol, similar usage as pushed protocol
    but pull is a unicast.

`SLIM_PROTO_LOCKED`
:   Locked Protocol

`SLIM_PROTO_ASYNC_SMPLX`
:   Asynchronous Protocol-Simplex

`SLIM_PROTO_ASYNC_HALF_DUP`
:   Asynchronous Protocol-Half-duplex

`SLIM_PROTO_EXT_SMPLX`
:   Extended Asynchronous Protocol-Simplex

`SLIM_PROTO_EXT_HALF_DUP`
:   Extended Asynchronous Protocol-Half-duplex

struct slim\_stream\_runtime
:   SLIMbus stream runtime instance

**Definition**:

```
struct slim_stream_runtime {
    const char *name;
    struct slim_device *dev;
    int direction;
    enum slim_transport_protocol prot;
    unsigned int rate;
    unsigned int bps;
    unsigned int ratem;
    int num_ports;
    struct slim_port *ports;
    struct list_head node;
};
```

**Members**

`name`
:   Name of the stream

`dev`
:   SLIM Device instance associated with this stream

`direction`
:   direction of stream

`prot`
:   Transport protocol used in this stream

`rate`
:   Data rate of samples \*

`bps`
:   bits per sample

`ratem`
:   rate multipler which is super frame rate/data rate

`num_ports`
:   number of ports

`ports`
:   pointer to instance of ports

`node`
:   list head for stream associated with slim device.

struct slim\_controller
:   Controls every instance of SLIMbus (similar to ‘master’ on SPI)

**Definition**:

```
struct slim_controller {
    struct device           *dev;
    unsigned int            id;
    char name[SLIMBUS_NAME_SIZE];
    int min_cg;
    int max_cg;
    int clkgear;
    struct ida              laddr_ida;
    struct slim_framer      *a_framer;
    struct mutex            lock;
    struct list_head        devices;
    struct idr              tid_idr;
    spinlock_t txn_lock;
    struct slim_sched       sched;
    int (*xfer_msg)(struct slim_controller *ctrl, struct slim_msg_txn *tx);
    int (*set_laddr)(struct slim_controller *ctrl, struct slim_eaddr *ea, u8 laddr);
    int (*get_laddr)(struct slim_controller *ctrl, struct slim_eaddr *ea, u8 *laddr);
    int (*enable_stream)(struct slim_stream_runtime *rt);
    int (*disable_stream)(struct slim_stream_runtime *rt);
    int (*wakeup)(struct slim_controller *ctrl);
};
```

**Members**

`dev`
:   Device interface to this driver

`id`
:   Board-specific number identifier for this controller/bus

`name`
:   Name for this controller

`min_cg`
:   Minimum clock gear supported by this controller (default value: 1)

`max_cg`
:   Maximum clock gear supported by this controller (default value: 10)

`clkgear`
:   Current clock gear in which this bus is running

`laddr_ida`
:   logical address id allocator

`a_framer`
:   Active framer which is clocking the bus managed by this controller

`lock`
:   Mutex protecting controller data structures

`devices`
:   Slim device list

`tid_idr`
:   tid id allocator

`txn_lock`
:   Lock to protect table of transactions

`sched`
:   scheduler structure used by the controller

`xfer_msg`
:   Transfer a message on this controller (this can be a broadcast
    control/status message like data channel setup, or a unicast message
    like value element read/write.

`set_laddr`
:   Setup logical address at laddr for the slave with elemental
    address e\_addr. Drivers implementing controller will be expected to
    send unicast message to this device with its logical address.

`get_laddr`
:   It is possible that controller needs to set fixed logical
    address table and get\_laddr can be used in that case so that controller
    can do this assignment. Use case is when the master is on the remote
    processor side, who is resposible for allocating laddr.

`enable_stream`
:   This function pointer implements controller-specific procedure
    to enable a stream.

`disable_stream`
:   This function pointer implements controller-specific procedure
    to disable stream.

`wakeup`
:   This function pointer implements controller-specific procedure
    to wake it up from clock-pause. Framework will call this to bring
    the controller out of clock pause.

**Description**

> ‘Manager device’ is responsible for device management, bandwidth
> allocation, channel setup, and port associations per channel.
> Device management means Logical address assignment/removal based on
> enumeration (report-present, report-absent) of a device.
> Bandwidth allocation is done dynamically by the manager based on active
> channels on the bus, message-bandwidth requests made by SLIMbus devices.
> Based on current bandwidth usage, manager chooses a frequency to run
> the bus at (in steps of ‘clock-gear’, 1 through 10, each clock gear
> representing twice the frequency than the previous gear).
> Manager is also responsible for entering (and exiting) low-power-mode
> (known as ‘clock pause’).
> Manager can do handover of framer if there are multiple framers on the
> bus and a certain usecase warrants using certain framer to avoid keeping
> previous framer being powered-on.
>
> Controller here performs duties of the manager device, and ‘interface
> device’. Interface device is responsible for monitoring the bus and
> reporting information such as loss-of-synchronization, data
> slot-collision.

int slim\_unregister\_controller(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl)
:   Controller tear-down.

**Parameters**

`struct slim_controller *ctrl`
:   Controller to tear-down.

void slim\_report\_absent(struct [slim\_device](#c.slim_device "slim_device") \*sbdev)
:   Controller calls this function when a device reports absent, OR when the device cannot be communicated with

**Parameters**

`struct slim_device *sbdev`
:   Device that cannot be reached, or sent report absent

struct [slim\_device](#c.slim_device "slim_device") \*slim\_get\_device(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, struct [slim\_eaddr](#c.slim_eaddr "slim_eaddr") \*e\_addr)
:   get handle to a device.

**Parameters**

`struct slim_controller *ctrl`
:   Controller on which this device will be added/queried

`struct slim_eaddr *e_addr`
:   Enumeration address of the device to be queried

**Description**

Takes a reference to the embedded [`struct device`](infrastructure.html#c.device "device") which needs to be dropped
after use.

**Return**

pointer to a device if it has already reported. Creates a new
device and returns pointer to it if the device has not yet enumerated.

struct [slim\_device](#c.slim_device "slim_device") \*of\_slim\_get\_device(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, struct device\_node \*np)
:   get handle to a device using dt node.

**Parameters**

`struct slim_controller *ctrl`
:   Controller on which this device will be queried

`struct device_node *np`
:   node pointer to device

**Description**

Takes a reference to the embedded [`struct device`](infrastructure.html#c.device "device") which needs to be dropped
after use.

**Return**

pointer to a device if it has been registered, otherwise NULL.

int slim\_device\_report\_present(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, struct [slim\_eaddr](#c.slim_eaddr "slim_eaddr") \*e\_addr, u8 \*laddr)
:   Report enumerated device.

**Parameters**

`struct slim_controller *ctrl`
:   Controller with which device is enumerated.

`struct slim_eaddr *e_addr`
:   Enumeration address of the device.

`u8 *laddr`
:   Return logical address (if valid flag is false)

**Description**

Called by controller in response to REPORT\_PRESENT. Framework will assign
a logical address to this enumeration address.
Function returns -EXFULL to indicate that all logical addresses are already
taken.

int slim\_get\_logical\_addr(struct [slim\_device](#c.slim_device "slim_device") \*sbdev)
:   get/allocate logical address of a SLIMbus device.

**Parameters**

`struct slim_device *sbdev`
:   client handle requesting the address.

**Return**

zero if a logical address is valid or a new logical address
has been assigned. error code in case of error.

### Clock-pause:

SLIMbus mandates that a reconfiguration sequence (known as clock-pause) be
broadcast to all active devices on the bus before the bus can enter low-power
mode. Controller uses this sequence when it decides to enter low-power mode so
that corresponding clocks and/or power-rails can be turned off to save power.
Clock-pause is exited by waking up framer device (if controller driver initiates
exiting low power mode), or by toggling the data line (if a slave device wants
to initiate it).

#### Clock-pause APIs:

int slim\_ctrl\_clk\_pause(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, bool wakeup, u8 restart)
:   Called by slimbus controller to enter/exit ‘clock pause’

**Parameters**

`struct slim_controller *ctrl`
:   controller requesting bus to be paused or woken up

`bool wakeup`
:   Wakeup this controller from clock pause.

`u8 restart`
:   Restart time value per spec used for clock pause. This value
    isn’t used when controller is to be woken up.

**Description**

Slimbus specification needs this sequence to turn-off clocks for the bus.
The sequence involves sending 3 broadcast messages (reconfiguration
sequence) to inform all devices on the bus.
To exit clock-pause, controller typically wakes up active framer device.
This API executes clock pause reconfiguration sequence if wakeup is false.
If wakeup is true, controller’s wakeup is called.
For entering clock-pause, -EBUSY is returned if a message txn in pending.

### Messaging:

The framework supports regmap and read/write apis to exchange control-information
with a SLIMbus device. APIs can be synchronous or asynchronous.
The header file <linux/slimbus.h> has more documentation about messaging APIs.

#### Messaging APIs:

void slim\_msg\_response(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, u8 \*reply, u8 tid, u8 len)
:   Deliver Message response received from a device to the framework.

**Parameters**

`struct slim_controller *ctrl`
:   Controller handle

`u8 *reply`
:   Reply received from the device

`u8 tid`
:   Transaction ID received with which framework can associate reply.

`u8 len`
:   Length of the reply

**Description**

Called by controller to inform framework about the response received.
This helps in making the API asynchronous, and controller-driver doesn’t need
to manage 1 more table other than the one managed by framework mapping TID
with buffers

int slim\_alloc\_txn\_tid(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, struct [slim\_msg\_txn](#c.slim_msg_txn "slim_msg_txn") \*txn)
:   Allocate a tid to txn

**Parameters**

`struct slim_controller *ctrl`
:   Controller handle

`struct slim_msg_txn *txn`
:   transaction to be allocated with tid.

**Return**

zero on success with valid txn->tid and error code on failures.

void slim\_free\_txn\_tid(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, struct [slim\_msg\_txn](#c.slim_msg_txn "slim_msg_txn") \*txn)
:   Free tid of txn

**Parameters**

`struct slim_controller *ctrl`
:   Controller handle

`struct slim_msg_txn *txn`
:   transaction whose tid should be freed

int slim\_do\_transfer(struct [slim\_controller](#c.slim_controller "slim_controller") \*ctrl, struct [slim\_msg\_txn](#c.slim_msg_txn "slim_msg_txn") \*txn)
:   Process a SLIMbus-messaging transaction

**Parameters**

`struct slim_controller *ctrl`
:   Controller handle

`struct slim_msg_txn *txn`
:   Transaction to be sent over SLIMbus

**Description**

Called by controller to transmit messaging transactions not dealing with
Interface/Value elements. (e.g. transmitting a message to assign logical
address to a slave device

**Return**

-ETIMEDOUT: If transmission of this message timed out
(e.g. due to bus lines not being clocked or driven by controller)

int slim\_xfer\_msg(struct [slim\_device](#c.slim_device "slim_device") \*sbdev, struct [slim\_val\_inf](#c.slim_val_inf "slim_val_inf") \*msg, u8 mc)
:   Transfer a value info message on slim device

**Parameters**

`struct slim_device *sbdev`
:   slim device to which this msg has to be transferred

`struct slim_val_inf *msg`
:   value info message pointer

`u8 mc`
:   message code of the message

**Description**

Called by drivers which want to transfer a vlaue or info elements.

**Return**

-ETIMEDOUT: If transmission of this message timed out

int slim\_read(struct [slim\_device](#c.slim_device "slim_device") \*sdev, u32 addr, size\_t count, u8 \*val)
:   Read SLIMbus value element

**Parameters**

`struct slim_device *sdev`
:   client handle.

`u32 addr`
:   address of value element to read.

`size_t count`
:   number of bytes to read. Maximum bytes allowed are 16.

`u8 *val`
:   will return what the value element value was

**Return**

-EINVAL for Invalid parameters, -ETIMEDOUT If transmission of
this message timed out (e.g. due to bus lines not being clocked
or driven by controller)

int slim\_readb(struct [slim\_device](#c.slim_device "slim_device") \*sdev, u32 addr)
:   Read byte from SLIMbus value element

**Parameters**

`struct slim_device *sdev`
:   client handle.

`u32 addr`
:   address in the value element to read.

**Return**

byte value of value element.

int slim\_write(struct [slim\_device](#c.slim_device "slim_device") \*sdev, u32 addr, size\_t count, u8 \*val)
:   Write SLIMbus value element

**Parameters**

`struct slim_device *sdev`
:   client handle.

`u32 addr`
:   address in the value element to write.

`size_t count`
:   number of bytes to write. Maximum bytes allowed are 16.

`u8 *val`
:   value to write to value element

**Return**

-EINVAL for Invalid parameters, -ETIMEDOUT If transmission of
this message timed out (e.g. due to bus lines not being clocked
or driven by controller)

int slim\_writeb(struct [slim\_device](#c.slim_device "slim_device") \*sdev, u32 addr, u8 value)
:   Write byte to SLIMbus value element

**Parameters**

`struct slim_device *sdev`
:   client handle.

`u32 addr`
:   address of value element to write.

`u8 value`
:   value to write to value element

**Return**

-EINVAL for Invalid parameters, -ETIMEDOUT If transmission of
this message timed out (e.g. due to bus lines not being clocked
or driven by controller)

#### Streaming APIs:

struct [slim\_stream\_runtime](#c.slim_stream_runtime "slim_stream_runtime") \*slim\_stream\_allocate(struct [slim\_device](#c.slim_device "slim_device") \*dev, const char \*name)
:   Allocate a new SLIMbus Stream

**Parameters**

`struct slim_device *dev`
:   Slim device to be associated with

`const char *name`
:   name of the stream

**Description**

This is very first call for SLIMbus streaming, this API will allocate
a new SLIMbus stream and return a valid stream runtime pointer for client
to use it in subsequent stream apis. state of stream is set to ALLOCATED

**Return**

valid pointer on success and error code on failure.
From ASoC DPCM framework, this state is linked to `startup()` operation.

int slim\_stream\_prepare(struct [slim\_stream\_runtime](#c.slim_stream_runtime "slim_stream_runtime") \*rt, struct [slim\_stream\_config](#c.slim_stream_config "slim_stream_config") \*cfg)
:   Prepare a SLIMbus Stream

**Parameters**

`struct slim_stream_runtime *rt`
:   instance of slim stream runtime to configure

`struct slim_stream_config *cfg`
:   new configuration for the stream

**Description**

This API will configure SLIMbus stream with config parameters from cfg.
return zero on success and error code on failure. From ASoC DPCM framework,
this state is linked to `hw_params()` operation.

int slim\_stream\_enable(struct [slim\_stream\_runtime](#c.slim_stream_runtime "slim_stream_runtime") \*stream)
:   Enable a prepared SLIMbus Stream

**Parameters**

`struct slim_stream_runtime *stream`
:   instance of slim stream runtime to enable

**Description**

This API will enable all the ports and channels associated with
SLIMbus stream

**Return**

zero on success and error code on failure. From ASoC DPCM framework,
this state is linked to `trigger()` start operation.

int slim\_stream\_disable(struct [slim\_stream\_runtime](#c.slim_stream_runtime "slim_stream_runtime") \*stream)
:   Disable a SLIMbus Stream

**Parameters**

`struct slim_stream_runtime *stream`
:   instance of slim stream runtime to disable

**Description**

This API will disable all the ports and channels associated with
SLIMbus stream

**Return**

zero on success and error code on failure. From ASoC DPCM framework,
this state is linked to `trigger()` pause operation.

int slim\_stream\_unprepare(struct [slim\_stream\_runtime](#c.slim_stream_runtime "slim_stream_runtime") \*stream)
:   Un-prepare a SLIMbus Stream

**Parameters**

`struct slim_stream_runtime *stream`
:   instance of slim stream runtime to unprepare

**Description**

This API will un allocate all the ports and channels associated with
SLIMbus stream

**Return**

zero on success and error code on failure. From ASoC DPCM framework,
this state is linked to `trigger()` stop operation.

int slim\_stream\_free(struct [slim\_stream\_runtime](#c.slim_stream_runtime "slim_stream_runtime") \*stream)
:   Free a SLIMbus Stream

**Parameters**

`struct slim_stream_runtime *stream`
:   instance of slim stream runtime to free

**Description**

This API will un allocate all the memory associated with
slim stream runtime, user is not allowed to make an dereference
to stream after this call.

**Return**

zero on success and error code on failure. From ASoC DPCM framework,
this state is linked to `shutdown()` operation.
