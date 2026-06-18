# High Speed Synchronous Serial Interface (HSI)

> 출처(원문): https://docs.kernel.org/driver-api/hsi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# High Speed Synchronous Serial Interface (HSI)

## Introduction

High Speed Synchronous Interface (HSI) is a full duplex, low latency protocol,
that is optimized for die-level interconnect between an Application Processor
and a Baseband chipset. It has been specified by the MIPI alliance in 2003 and
implemented by multiple vendors since then.

The HSI interface supports full duplex communication over multiple channels
(typically 8) and is capable of reaching speeds up to 200 Mbit/s.

The serial protocol uses two signals, DATA and FLAG as combined data and clock
signals and an additional READY signal for flow control. An additional WAKE
signal can be used to wakeup the chips from standby modes. The signals are
commonly prefixed by AC for signals going from the application die to the
cellular die and CA for signals going the other way around.

```
+------------+                                 +---------------+
|  Cellular  |                                 |  Application  |
|    Die     |                                 |      Die      |
|            | - - - - - - CAWAKE - - - - - - >|               |
|           T|------------ CADATA ------------>|R              |
|           X|------------ CAFLAG ------------>|X              |
|            |<----------- ACREADY ------------|               |
|            |                                 |               |
|            |                                 |               |
|            |< - - - - -  ACWAKE - - - - - - -|               |
|           R|<----------- ACDATA -------------|T              |
|           X|<----------- ACFLAG -------------|X              |
|            |------------ CAREADY ----------->|               |
|            |                                 |               |
|            |                                 |               |
+------------+                                 +---------------+
```

## HSI Subsystem in Linux

In the Linux kernel the hsi subsystem is supposed to be used for HSI devices.
The hsi subsystem contains drivers for hsi controllers including support for
multi-port controllers and provides a generic API for using the HSI ports.

It also contains HSI client drivers, which make use of the generic API to
implement a protocol used on the HSI interface. These client drivers can
use an arbitrary number of channels.

## hsi-char Device

Each port automatically registers a generic client driver called hsi\_char,
which provides a character device for userspace representing the HSI port.
It can be used to communicate via HSI from userspace. Userspace may
configure the hsi\_char device using the following ioctl commands:

HSC\_RESET
:   flush the HSI port

HSC\_SET\_PM
:   enable or disable the client.

HSC\_SEND\_BREAK
:   send break

HSC\_SET\_RX
:   set RX configuration

HSC\_GET\_RX
:   get RX configuration

HSC\_SET\_TX
:   set TX configuration

HSC\_GET\_TX
:   get TX configuration

## The kernel HSI API

struct hsi\_channel
:   channel resource used by the hsi clients

**Definition**:

```
struct hsi_channel {
    unsigned int    id;
    const char      *name;
};
```

**Members**

`id`
:   Channel number

`name`
:   Channel name

struct hsi\_config
:   Configuration for RX/TX HSI modules

**Definition**:

```
struct hsi_config {
    unsigned int            mode;
    struct hsi_channel      *channels;
    unsigned int            num_channels;
    unsigned int            num_hw_channels;
    unsigned int            speed;
    union {
        unsigned int    flow;
        unsigned int    arb_mode;
    };
};
```

**Members**

`mode`
:   Bit transmission mode (STREAM or FRAME)

`channels`
:   Channel resources used by the client

`num_channels`
:   Number of channel resources

`num_hw_channels`
:   Number of channels the transceiver is configured for [1..16]

`speed`
:   Max bit transmission speed (Kbit/s)

`{unnamed_union}`
:   anonymous

`flow`
:   RX flow type (SYNCHRONIZED or PIPELINE)

`arb_mode`
:   Arbitration mode for TX frame (Round robin, priority)

struct hsi\_board\_info
:   HSI client board info

**Definition**:

```
struct hsi_board_info {
    const char              *name;
    unsigned int            hsi_id;
    unsigned int            port;
    struct hsi_config       tx_cfg;
    struct hsi_config       rx_cfg;
    void *platform_data;
    struct dev_archdata     *archdata;
};
```

**Members**

`name`
:   Name for the HSI device

`hsi_id`
:   HSI controller id where the client sits

`port`
:   Port number in the controller where the client sits

`tx_cfg`
:   HSI TX configuration

`rx_cfg`
:   HSI RX configuration

`platform_data`
:   Platform related data

`archdata`
:   Architecture-dependent device data

struct hsi\_client
:   HSI client attached to an HSI port

**Definition**:

```
struct hsi_client {
    struct device           device;
    struct hsi_config       tx_cfg;
    struct hsi_config       rx_cfg;
};
```

**Members**

`device`
:   Driver model representation of the device

`tx_cfg`
:   HSI TX configuration

`rx_cfg`
:   HSI RX configuration

struct hsi\_client\_driver
:   Driver associated to an HSI client

**Definition**:

```
struct hsi_client_driver {
    struct device_driver    driver;
};
```

**Members**

`driver`
:   Driver model representation of the driver

struct hsi\_msg
:   HSI message descriptor

**Definition**:

```
struct hsi_msg {
    struct list_head        link;
    struct hsi_client       *cl;
    struct sg_table         sgt;
    void *context;
    void (*complete)(struct hsi_msg *msg);
    void (*destructor)(struct hsi_msg *msg);
    int status;
    unsigned int            actual_len;
    unsigned int            channel;
    unsigned int            ttype:1;
    unsigned int            break_frame:1;
};
```

**Members**

`link`
:   Free to use by the current descriptor owner

`cl`
:   HSI device client that issues the transfer

`sgt`
:   Head of the scatterlist array

`context`
:   Client context data associated to the transfer

`complete`
:   Transfer completion callback

`destructor`
:   Destructor to free resources when flushing

`status`
:   Status of the transfer when completed

`actual_len`
:   Actual length of data transferred on completion

`channel`
:   Channel were to TX/RX the message

`ttype`
:   Transfer type (TX if set, RX otherwise)

`break_frame`
:   if true HSI will send/receive a break frame. Data buffers are
    ignored in the request.

struct hsi\_port
:   HSI port device

**Definition**:

```
struct hsi_port {
    struct device                   device;
    struct hsi_config               tx_cfg;
    struct hsi_config               rx_cfg;
    unsigned int                    num;
    unsigned int                    shared:1;
    int claimed;
    struct mutex                    lock;
    int (*async)(struct hsi_msg *msg);
    int (*setup)(struct hsi_client *cl);
    int (*flush)(struct hsi_client *cl);
    int (*start_tx)(struct hsi_client *cl);
    int (*stop_tx)(struct hsi_client *cl);
    int (*release)(struct hsi_client *cl);
    struct blocking_notifier_head   n_head;
};
```

**Members**

`device`
:   Driver model representation of the device

`tx_cfg`
:   Current TX path configuration

`rx_cfg`
:   Current RX path configuration

`num`
:   Port number

`shared`
:   Set when port can be shared by different clients

`claimed`
:   Reference count of clients which claimed the port

`lock`
:   Serialize port claim

`async`
:   Asynchronous transfer callback

`setup`
:   Callback to set the HSI client configuration

`flush`
:   Callback to clean the HW state and destroy all pending transfers

`start_tx`
:   Callback to inform that a client wants to TX data

`stop_tx`
:   Callback to inform that a client no longer wishes to TX data

`release`
:   Callback to inform that a client no longer uses the port

`n_head`
:   Notifier chain for signaling port events to the clients.

struct hsi\_controller
:   HSI controller device

**Definition**:

```
struct hsi_controller {
    struct device           device;
    struct module           *owner;
    unsigned int            id;
    unsigned int            num_ports;
    struct hsi_port         *port[];
};
```

**Members**

`device`
:   Driver model representation of the device

`owner`
:   Pointer to the module owning the controller

`id`
:   HSI controller ID

`num_ports`
:   Number of ports in the HSI controller

`port`
:   Array of HSI ports

unsigned int hsi\_id(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Get HSI controller ID associated to a client

**Parameters**

`struct hsi_client *cl`
:   Pointer to a HSI client

**Description**

Return the controller id where the client is attached to

unsigned int hsi\_port\_id(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Gets the port number a client is attached to

**Parameters**

`struct hsi_client *cl`
:   Pointer to HSI client

**Description**

Return the port number associated to the client

int hsi\_setup(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Configure the client’s port

**Parameters**

`struct hsi_client *cl`
:   Pointer to the HSI client

**Description**

When sharing ports, clients should either relay on a single
client setup or have the same setup for all of them.

Return -errno on failure, 0 on success

int hsi\_flush(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Flush all pending transactions on the client’s port

**Parameters**

`struct hsi_client *cl`
:   Pointer to the HSI client

**Description**

This function will destroy all pending hsi\_msg in the port and reset
the HW port so it is ready to receive and transmit from a clean state.

Return -errno on failure, 0 on success

int hsi\_async\_read(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl, struct [hsi\_msg](#c.hsi_msg "hsi_msg") \*msg)
:   Submit a read transfer

**Parameters**

`struct hsi_client *cl`
:   Pointer to the HSI client

`struct hsi_msg *msg`
:   HSI message descriptor of the transfer

**Description**

Return -errno on failure, 0 on success

int hsi\_async\_write(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl, struct [hsi\_msg](#c.hsi_msg "hsi_msg") \*msg)
:   Submit a write transfer

**Parameters**

`struct hsi_client *cl`
:   Pointer to the HSI client

`struct hsi_msg *msg`
:   HSI message descriptor of the transfer

**Description**

Return -errno on failure, 0 on success

int hsi\_start\_tx(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Signal the port that the client wants to start a TX

**Parameters**

`struct hsi_client *cl`
:   Pointer to the HSI client

**Description**

Return -errno on failure, 0 on success

int hsi\_stop\_tx(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Signal the port that the client no longer wants to transmit

**Parameters**

`struct hsi_client *cl`
:   Pointer to the HSI client

**Description**

Return -errno on failure, 0 on success

void hsi\_port\_unregister\_clients(struct [hsi\_port](#c.hsi_port "hsi_port") \*port)
:   Unregister an HSI port

**Parameters**

`struct hsi_port *port`
:   The HSI port to unregister

void hsi\_unregister\_controller(struct [hsi\_controller](#c.hsi_controller "hsi_controller") \*hsi)
:   Unregister an HSI controller

**Parameters**

`struct hsi_controller *hsi`
:   The HSI controller to register

int hsi\_register\_controller(struct [hsi\_controller](#c.hsi_controller "hsi_controller") \*hsi)
:   Register an HSI controller and its ports

**Parameters**

`struct hsi_controller *hsi`
:   The HSI controller to register

**Description**

Returns -errno on failure, 0 on success.

int hsi\_register\_client\_driver(struct [hsi\_client\_driver](#c.hsi_client_driver "hsi_client_driver") \*drv)
:   Register an HSI client to the HSI bus

**Parameters**

`struct hsi_client_driver *drv`
:   HSI client driver to register

**Description**

Returns -errno on failure, 0 on success.

void hsi\_put\_controller(struct [hsi\_controller](#c.hsi_controller "hsi_controller") \*hsi)
:   Free an HSI controller

**Parameters**

`struct hsi_controller *hsi`
:   Pointer to the HSI controller to freed

**Description**

HSI controller drivers should only use this function if they need
to free their allocated hsi\_controller structures before a successful
call to hsi\_register\_controller. Other use is not allowed.

struct [hsi\_controller](#c.hsi_controller "hsi_controller") \*hsi\_alloc\_controller(unsigned int n\_ports, gfp\_t flags)
:   Allocate an HSI controller and its ports

**Parameters**

`unsigned int n_ports`
:   Number of ports on the HSI controller

`gfp_t flags`
:   Kernel allocation flags

**Description**

Return NULL on failure or a pointer to an hsi\_controller on success.

void hsi\_free\_msg(struct [hsi\_msg](#c.hsi_msg "hsi_msg") \*msg)
:   Free an HSI message

**Parameters**

`struct hsi_msg *msg`
:   Pointer to the HSI message

**Description**

Client is responsible to free the buffers pointed by the scatterlists.

struct [hsi\_msg](#c.hsi_msg "hsi_msg") \*hsi\_alloc\_msg(unsigned int nents, gfp\_t flags)
:   Allocate an HSI message

**Parameters**

`unsigned int nents`
:   Number of memory entries

`gfp_t flags`
:   Kernel allocation flags

**Description**

nents can be 0. This mainly makes sense for read transfer.
In that case, HSI drivers will call the complete callback when
there is data to be read without consuming it.

Return NULL on failure or a pointer to an hsi\_msg on success.

int hsi\_async(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl, struct [hsi\_msg](#c.hsi_msg "hsi_msg") \*msg)
:   Submit an HSI transfer to the controller

**Parameters**

`struct hsi_client *cl`
:   HSI client sending the transfer

`struct hsi_msg *msg`
:   The HSI transfer passed to controller

**Description**

The HSI message must have the channel, ttype, complete and destructor
fields set beforehand. If nents > 0 then the client has to initialize
also the scatterlists to point to the buffers to write to or read from.

HSI controllers relay on pre-allocated buffers from their clients and they
do not allocate buffers on their own.

Once the HSI message transfer finishes, the HSI controller calls the
complete callback with the status and actual\_len fields of the HSI message
updated. The complete callback can be called before returning from
hsi\_async.

Returns -errno on failure or 0 on success

int hsi\_claim\_port(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl, unsigned int share)
:   Claim the HSI client’s port

**Parameters**

`struct hsi_client *cl`
:   HSI client that wants to claim its port

`unsigned int share`
:   Flag to indicate if the client wants to share the port or not.

**Description**

Returns -errno on failure, 0 on success.

void hsi\_release\_port(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Release the HSI client’s port

**Parameters**

`struct hsi_client *cl`
:   HSI client which previously claimed its port

int hsi\_register\_port\_event(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl, void (\*handler)(struct [hsi\_client](#c.hsi_client "hsi_client")\*, unsigned long))
:   Register a client to receive port events

**Parameters**

`struct hsi_client *cl`
:   HSI client that wants to receive port events

`void (*handler)(struct hsi_client *, unsigned long)`
:   Event handler callback

**Description**

Clients should register a callback to be able to receive
events from the ports. Registration should happen after
claiming the port.
The handler can be called in interrupt context.

Returns -errno on error, or 0 on success.

int hsi\_unregister\_port\_event(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl)
:   Stop receiving port events for a client

**Parameters**

`struct hsi_client *cl`
:   HSI client that wants to stop receiving port events

**Description**

Clients should call this function before releasing their associated
port.

Returns -errno on error, or 0 on success.

int hsi\_event(struct [hsi\_port](#c.hsi_port "hsi_port") \*port, unsigned long event)
:   Notifies clients about port events

**Parameters**

`struct hsi_port *port`
:   Port where the event occurred

`unsigned long event`
:   The event type

**Description**

Clients should not be concerned about wake line behavior. However, due
to a race condition in HSI HW protocol, clients need to be notified
about wake line changes, so they can implement a workaround for it.

Events:
HSI\_EVENT\_START\_RX - Incoming wake line high
HSI\_EVENT\_STOP\_RX - Incoming wake line down

Returns -errno on error, or 0 on success.

int hsi\_get\_channel\_id\_by\_name(struct [hsi\_client](#c.hsi_client "hsi_client") \*cl, char \*name)
:   acquire channel id by channel name

**Parameters**

`struct hsi_client *cl`
:   HSI client, which uses the channel

`char *name`
:   name the channel is known under

**Description**

Clients can call this function to get the hsi channel ids similar to
requesting IRQs or GPIOs by name. This function assumes the same
channel configuration is used for RX and TX.

Returns -errno on error or channel id on success.
