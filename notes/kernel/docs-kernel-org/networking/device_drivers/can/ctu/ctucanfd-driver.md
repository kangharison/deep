# CTU CAN FD Driver

> 출처(원문): https://docs.kernel.org/networking/device_drivers/can/ctu/ctucanfd-driver.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# CTU CAN FD Driver

Author: Martin Jerabek <[martin.jerabek01@gmail.com](mailto:martin.jerabek01%40gmail.com)>

## About CTU CAN FD IP Core

[CTU CAN FD](https://gitlab.fel.cvut.cz/canbus/ctucanfd_ip_core)
is an open source soft core written in VHDL.
It originated in 2015 as Ondrej Ille’s project
at the [Department of Measurement](https://meas.fel.cvut.cz/)
of [FEE](http://www.fel.cvut.cz/en/) at [CTU](https://www.cvut.cz/en).

The SocketCAN driver for Xilinx Zynq SoC based MicroZed board
[Vivado integration](https://gitlab.fel.cvut.cz/canbus/zynq/zynq-can-sja1000-top)
and Intel Cyclone V 5CSEMA4U23C6 based DE0-Nano-SoC Terasic board
[QSys integration](https://gitlab.fel.cvut.cz/canbus/intel-soc-ctucanfd)
has been developed as well as support for
[PCIe integration](https://gitlab.fel.cvut.cz/canbus/pcie-ctucanfd) of the core.

In the case of Zynq, the core is connected via the APB system bus, which does
not have enumeration support, and the device must be specified in Device Tree.
This kind of devices is called platform device in the kernel and is
handled by a platform device driver.

The basic functional model of the CTU CAN FD peripheral has been
accepted into QEMU mainline. See QEMU [CAN emulation support](https://www.qemu.org/docs/master/system/devices/can.html)
for CAN FD buses, host connection and CTU CAN FD core emulation. The development
version of emulation support can be cloned from ctu-canfd branch of QEMU local
development [repository](https://gitlab.fel.cvut.cz/canbus/qemu-canbus).

## About SocketCAN

SocketCAN is a standard common interface for CAN devices in the Linux
kernel. As the name suggests, the bus is accessed via sockets, similarly
to common network devices. The reasoning behind this is in depth
described in [Linux SocketCAN](https://www.kernel.org/doc/html/latest/networking/can.html).
In short, it offers a
natural way to implement and work with higher layer protocols over CAN,
in the same way as, e.g., UDP/IP over Ethernet.

### Device probe

Before going into detail about the structure of a CAN bus device driver,
let’s reiterate how the kernel gets to know about the device at all.
Some buses, like PCI or PCIe, support device enumeration. That is, when
the system boots, it discovers all the devices on the bus and reads
their configuration. The kernel identifies the device via its vendor ID
and device ID, and if there is a driver registered for this identifier
combination, its probe method is invoked to populate the driver’s
instance for the given hardware. A similar situation goes with USB, only
it allows for device hot-plug.

The situation is different for peripherals which are directly embedded
in the SoC and connected to an internal system bus (AXI, APB, Avalon,
and others). These buses do not support enumeration, and thus the kernel
has to learn about the devices from elsewhere. This is exactly what the
Device Tree was made for.

### Device tree

An entry in device tree states that a device exists in the system, how
it is reachable (on which bus it resides) and its configuration –
registers address, interrupts and so on. An example of such a device
tree is given in .

```
/ {
    /* ... */
    amba: amba {
        #address-cells = <1>;
        #size-cells = <1>;
        compatible = "simple-bus";

        CTU_CAN_FD_0: CTU_CAN_FD@43c30000 {
            compatible = "ctu,ctucanfd";
            interrupt-parent = <&intc>;
            interrupts = <0 30 4>;
            clocks = <&clkc 15>;
            reg = <0x43c30000 0x10000>;
        };
    };
};
```

### Driver structure

The driver can be divided into two parts – platform-dependent device
discovery and set up, and platform-independent CAN network device
implementation.

#### Platform device driver

In the case of Zynq, the core is connected via the AXI system bus, which
does not have enumeration support, and the device must be specified in
Device Tree. This kind of devices is called *platform device* in the
kernel and is handled by a *platform device driver* [[1]](#id11).

A platform device driver provides the following things:

* A *probe* function
* A *remove* function
* A table of *compatible* devices that the driver can handle

The *probe* function is called exactly once when the device appears (or
the driver is loaded, whichever happens later). If there are more
devices handled by the same driver, the *probe* function is called for
each one of them. Its role is to allocate and initialize resources
required for handling the device, as well as set up low-level functions
for the platform-independent layer, e.g., *read\_reg* and *write\_reg*.
After that, the driver registers the device to a higher layer, in our
case as a *network device*.

The *remove* function is called when the device disappears, or the
driver is about to be unloaded. It serves to free the resources
allocated in *probe* and to unregister the device from higher layers.

Finally, the table of *compatible* devices states which devices the
driver can handle. The Device Tree entry `compatible` is matched
against the tables of all *platform drivers*.

```
/* Match table for OF platform binding */
static const struct of_device_id ctucan_of_match[] = {
    { .compatible = "ctu,canfd-2", },
    { .compatible = "ctu,ctucanfd", },
    { /* end of list */ },
};
MODULE_DEVICE_TABLE(of, ctucan_of_match);

static int ctucan_probe(struct platform_device *pdev);
static int ctucan_remove(struct platform_device *pdev);

static struct platform_driver ctucanfd_driver = {
    .probe  = ctucan_probe,
    .remove = ctucan_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = ctucan_of_match,
    },
};
module_platform_driver(ctucanfd_driver);
```

#### Network device driver

Each network device must support at least these operations:

* Bring the device up: `ndo_open`
* Bring the device down: `ndo_close`
* Submit TX frames to the device: `ndo_start_xmit`
* Signal TX completion and errors to the network subsystem: ISR
* Submit RX frames to the network subsystem: ISR and NAPI

There are two possible event sources: the device and the network
subsystem. Device events are usually signaled via an interrupt, handled
in an Interrupt Service Routine (ISR). Handlers for the events
originating in the network subsystem are then specified in
`struct net_device_ops`.

When the device is brought up, e.g., by calling `ip link set can0 up`,
the driver’s function `ndo_open` is called. It should validate the
interface configuration and configure and enable the device. The
analogous opposite is `ndo_close`, called when the device is being
brought down, be it explicitly or implicitly.

When the system should transmit a frame, it does so by calling
`ndo_start_xmit`, which enqueues the frame into the device. If the
device HW queue (FIFO, mailboxes or whatever the implementation is)
becomes full, the `ndo_start_xmit` implementation informs the network
subsystem that it should stop the TX queue (via `netif_stop_queue`).
It is then re-enabled later in ISR when the device has some space
available again and is able to enqueue another frame.

All the device events are handled in ISR, namely:

1. **TX completion**. When the device successfully finishes transmitting
   a frame, the frame is echoed locally. On error, an informative error
   frame [[2]](#id12) is sent to the network subsystem instead. In both cases,
   the software TX queue is resumed so that more frames may be sent.
2. **Error condition**. If something goes wrong (e.g., the device goes
   bus-off or RX overrun happens), error counters are updated, and
   informative error frames are enqueued to SW RX queue.
3. **RX buffer not empty**. In this case, read the RX frames and enqueue
   them to SW RX queue. Usually NAPI is used as a middle layer (see ).

### NAPI

The frequency of incoming frames can be high and the overhead to invoke
the interrupt service routine for each frame can cause significant
system load. There are multiple mechanisms in the Linux kernel to deal
with this situation. They evolved over the years of Linux kernel
development and enhancements. For network devices, the current standard
is NAPI – *the New API*. It is similar to classical top-half/bottom-half
interrupt handling in that it only acknowledges the interrupt in the ISR
and signals that the rest of the processing should be done in softirq
context. On top of that, it offers the possibility to *poll* for new
frames for a while. This has a potential to avoid the costly round of
enabling interrupts, handling an incoming IRQ in ISR, re-enabling the
softirq and switching context back to softirq.

See [Documentation/networking/napi.rst](../../../napi.html#napi) for more information.

## Integrating the core to Xilinx Zynq

The core interfaces a simple subset of the Avalon
(search for Intel **Avalon Interface Specifications**)
bus as it was originally used on
Alterra FPGA chips, yet Xilinx natively interfaces with AXI
(search for ARM **AMBA AXI and ACE Protocol Specification AXI3,
AXI4, and AXI4-Lite, ACE and ACE-Lite**).
The most obvious solution would be to use
an Avalon/AXI bridge or implement some simple conversion entity.
However, the core’s interface is half-duplex with no handshake
signaling, whereas AXI is full duplex with two-way signaling. Moreover,
even AXI-Lite slave interface is quite resource-intensive, and the
flexibility and speed of AXI are not required for a CAN core.

Thus a much simpler bus was chosen – APB (Advanced Peripheral Bus)
(search for ARM **AMBA APB Protocol Specification**).
APB-AXI bridge is directly available in
Xilinx Vivado, and the interface adaptor entity is just a few simple
combinatorial assignments.

Finally, to be able to include the core in a block diagram as a custom
IP, the core, together with the APB interface, has been packaged as a
Vivado component.

## CTU CAN FD Driver design

The general structure of a CAN device driver has already been examined
in . The next paragraphs provide a more detailed description of the CTU
CAN FD core driver in particular.

### Low-level driver

The core is not intended to be used solely with SocketCAN, and thus it
is desirable to have an OS-independent low-level driver. This low-level
driver can then be used in implementations of OS driver or directly
either on bare metal or in a user-space application. Another advantage
is that if the hardware slightly changes, only the low-level driver
needs to be modified.

The code [[3]](#id13) is in part automatically generated and in part written
manually by the core author, with contributions of the thesis’ author.
The low-level driver supports operations such as: set bit timing, set
controller mode, enable/disable, read RX frame, write TX frame, and so
on.

### Configuring bit timing

On CAN, each bit is divided into four segments: SYNC, PROP, PHASE1, and
PHASE2. Their duration is expressed in multiples of a Time Quantum
(details in [CAN Specification, Version 2.0](http://esd.cs.ucr.edu/webres/can20.pdf), chapter 8).
When configuring
bitrate, the durations of all the segments (and time quantum) must be
computed from the bitrate and Sample Point. This is performed
independently for both the Nominal bitrate and Data bitrate for CAN FD.

SocketCAN is fairly flexible and offers either highly customized
configuration by setting all the segment durations manually, or a
convenient configuration by setting just the bitrate and sample point
(and even that is chosen automatically per Bosch recommendation if not
specified). However, each CAN controller may have different base clock
frequency and different width of segment duration registers. The
algorithm thus needs the minimum and maximum values for the durations
(and clock prescaler) and tries to optimize the numbers to fit both the
constraints and the requested parameters.

```
struct can_bittiming_const {
    char name[16];      /* Name of the CAN controller hardware */
    __u32 tseg1_min;    /* Time segment 1 = prop_seg + phase_seg1 */
    __u32 tseg1_max;
    __u32 tseg2_min;    /* Time segment 2 = phase_seg2 */
    __u32 tseg2_max;
    __u32 sjw_max;      /* Synchronisation jump width */
    __u32 brp_min;      /* Bit-rate prescaler */
    __u32 brp_max;
    __u32 brp_inc;
};
```

[lst:can\_bittiming\_const]

A curious reader will notice that the durations of the segments PROP\_SEG
and PHASE\_SEG1 are not determined separately but rather combined and
then, by default, the resulting TSEG1 is evenly divided between PROP\_SEG
and PHASE\_SEG1. In practice, this has virtually no consequences as the
sample point is between PHASE\_SEG1 and PHASE\_SEG2. In CTU CAN FD,
however, the duration registers `PROP` and `PH1` have different
widths (6 and 7 bits, respectively), so the auto-computed values might
overflow the shorter register and must thus be redistributed among the
two [[4]](#id14).

### Handling RX

Frame reception is handled in NAPI queue, which is enabled from ISR when
the RXNE (RX FIFO Not Empty) bit is set. Frames are read one by one
until either no frame is left in the RX FIFO or the maximum work quota
has been reached for the NAPI poll run (see ). Each frame is then passed
to the network interface RX queue.

An incoming frame may be either a CAN 2.0 frame or a CAN FD frame. The
way to distinguish between these two in the kernel is to allocate either
`struct can_frame` or `struct canfd_frame`, the two having different
sizes. In the controller, the information about the frame type is stored
in the first word of RX FIFO.

This brings us a chicken-egg problem: we want to allocate the `skb`
for the frame, and only if it succeeds, fetch the frame from FIFO;
otherwise keep it there for later. But to be able to allocate the
correct `skb`, we have to fetch the first work of FIFO. There are
several possible solutions:

1. Read the word, then allocate. If it fails, discard the rest of the
   frame. When the system is low on memory, the situation is bad anyway.
2. Always allocate `skb` big enough for an FD frame beforehand. Then
   tweak the `skb` internals to look like it has been allocated for
   the smaller CAN 2.0 frame.
3. Add option to peek into the FIFO instead of consuming the word.
4. If the allocation fails, store the read word into driver’s data. On
   the next try, use the stored word instead of reading it again.

Option 1 is simple enough, but not very satisfying if we could do
better. Option 2 is not acceptable, as it would require modifying the
private state of an integral kernel structure. The slightly higher
memory consumption is just a virtual cherry on top of the “cake”. Option
3 requires non-trivial HW changes and is not ideal from the HW point of
view.

Option 4 seems like a good compromise, with its disadvantage being that
a partial frame may stay in the FIFO for a prolonged time. Nonetheless,
there may be just one owner of the RX FIFO, and thus no one else should
see the partial frame (disregarding some exotic debugging scenarios).
Basides, the driver resets the core on its initialization, so the
partial frame cannot be “adopted” either. In the end, option 4 was
selected [[5]](#id15).

#### Timestamping RX frames

The CTU CAN FD core reports the exact timestamp when the frame has been
received. The timestamp is by default captured at the sample point of
the last bit of EOF but is configurable to be captured at the SOF bit.
The timestamp source is external to the core and may be up to 64 bits
wide. At the time of writing, passing the timestamp from kernel to
userspace is not yet implemented, but is planned in the future.

### Handling TX

The CTU CAN FD core has 4 independent TX buffers, each with its own
state and priority. When the core wants to transmit, a TX buffer in
Ready state with the highest priority is selected.

The priorities are 3bit numbers in register TX\_PRIORITY
(nibble-aligned). This should be flexible enough for most use cases.
SocketCAN, however, supports only one FIFO queue for outgoing
frames [[6]](#id16). The buffer priorities may be used to simulate the FIFO
behavior by assigning each buffer a distinct priority and *rotating* the
priorities after a frame transmission is completed.

In addition to priority rotation, the SW must maintain head and tail
pointers into the FIFO formed by the TX buffers to be able to determine
which buffer should be used for next frame (`txb_head`) and which
should be the first completed one (`txb_tail`). The actual buffer
indices are (obviously) modulo 4 (number of TX buffers), but the
pointers must be at least one bit wider to be able to distinguish
between FIFO full and FIFO empty – in this situation,
![txb\_head \equiv txb\_tail\ (\textrm{mod}\ 4)](_images/math/a34e41d7be6e6c23b5f5d1d7240e482edbfbc28d.png). An example of how
the FIFO is maintained, together with priority rotation, is depicted in

| TXB# | 0 | 1 | 2 | 3 |
| --- | --- | --- | --- | --- |
| Seq | A | B | C |  |
| Prio | 7 | 6 | 5 | 4 |
|  |  | T |  | H |

| TXB# | 0 | 1 | 2 | 3 |
| --- | --- | --- | --- | --- |
| Seq |  | B | C |  |
| Prio | 4 | 7 | 6 | 5 |
|  |  | T |  | H |

| TXB# | 0 | 1 | 2 | 3 | 0’ |
| --- | --- | --- | --- | --- | --- |
| Seq | E | B | C | D |  |
| Prio | 4 | 7 | 6 | 5 |  |
|  |  | T |  |  | H |

![../../../../_images/fsm_txt_buffer_user.svg](../../../../_images/fsm_txt_buffer_user.svg)

TX Buffer states with possible transitions

#### Timestamping TX frames

When submitting a frame to a TX buffer, one may specify the timestamp at
which the frame should be transmitted. The frame transmission may start
later, but not sooner. Note that the timestamp does not participate in
buffer prioritization – that is decided solely by the mechanism
described above.

Support for time-based packet transmission was recently merged to Linux
v4.19 [Time-based packet transmission](https://lwn.net/Articles/748879/),
but it remains yet to be researched
whether this functionality will be practical for CAN.

Also similarly to retrieving the timestamp of RX frames, the core
supports retrieving the timestamp of TX frames – that is the time when
the frame was successfully delivered. The particulars are very similar
to timestamping RX frames and are described in .

### Handling RX buffer overrun

When a received frame does no more fit into the hardware RX FIFO in its
entirety, RX FIFO overrun flag (STATUS[DOR]) is set and Data Overrun
Interrupt (DOI) is triggered. When servicing the interrupt, care must be
taken first to clear the DOR flag (via COMMAND[CDO]) and after that
clear the DOI interrupt flag. Otherwise, the interrupt would be
immediately [[7]](#id17) rearmed.

**Note**: During development, it was discussed whether the internal HW
pipelining cannot disrupt this clear sequence and whether an additional
dummy cycle is necessary between clearing the flag and the interrupt. On
the Avalon interface, it indeed proved to be the case, but APB being
safe because it uses 2-cycle transactions. Essentially, the DOR flag
would be cleared, but DOI register’s Preset input would still be high
the cycle when the DOI clear request would also be applied (by setting
the register’s Reset input high). As Set had higher priority than Reset,
the DOI flag would not be reset. This has been already fixed by swapping
the Set/Reset priority (see issue #187).

### Reporting Error Passive and Bus Off conditions

It may be desirable to report when the node reaches *Error Passive*,
*Error Warning*, and *Bus Off* conditions. The driver is notified about
error state change by an interrupt (EPI, EWLI), and then proceeds to
determine the core’s error state by reading its error counters.

There is, however, a slight race condition here – there is a delay
between the time when the state transition occurs (and the interrupt is
triggered) and when the error counters are read. When EPI is received,
the node may be either *Error Passive* or *Bus Off*. If the node goes
*Bus Off*, it obviously remains in the state until it is reset.
Otherwise, the node is *or was* *Error Passive*. However, it may happen
that the read state is *Error Warning* or even *Error Active*. It may be
unclear whether and what exactly to report in that case, but I
personally entertain the idea that the past error condition should still
be reported. Similarly, when EWLI is received but the state is later
detected to be *Error Passive*, *Error Passive* should be reported.

## CTU CAN FD Driver Sources Reference

int ctucan\_probe\_common(struct [device](../../../../driver-api/infrastructure.html#c.device "device") \*dev, void \_\_iomem \*addr, int irq, unsigned int ntxbufs, unsigned long can\_clk\_rate, int pm\_enable\_call, void (\*set\_drvdata\_fnc)(struct [device](../../../../driver-api/infrastructure.html#c.device "device") \*dev, struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev))
:   Device type independent registration call

**Parameters**

`struct device *dev`
:   Handle to the generic device structure

`void __iomem *addr`
:   Base address of CTU CAN FD core address

`int irq`
:   Interrupt number

`unsigned int ntxbufs`
:   Number of implemented Tx buffers

`unsigned long can_clk_rate`
:   Clock rate, if 0 then clock are taken from device node

`int pm_enable_call`
:   Whether pm\_runtime\_enable should be called

`void (*set_drvdata_fnc)(struct device *dev, struct net_device *ndev)`
:   Function to set network driver data for physical device

**Description**

This function does all the memory allocation and registration for the CAN
device.

**Return**

0 on success and failure value on error

const char \*ctucan\_state\_to\_str(enum can\_state state)
:   Converts CAN controller state code to corresponding text

**Parameters**

`enum can_state state`
:   CAN controller state code

**Return**

Pointer to string representation of the error state

int ctucan\_reset(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Issues software reset request to CTU CAN FD

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Return**

0 for success, -`ETIMEDOUT` if CAN controller does not leave reset

int ctucan\_set\_btr(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev, struct can\_bittiming \*bt, bool nominal)
:   Sets CAN bus bit timing in CTU CAN FD

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

`struct can_bittiming *bt`
:   Pointer to Bit timing structure

`bool nominal`
:   True - Nominal bit timing, False - Data bit timing

**Return**

0 - OK, -`EPERM` if controller is enabled

int ctucan\_set\_bittiming(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   CAN set nominal bit timing routine

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Return**

0 on success, -`EPERM` on error

int ctucan\_set\_data\_bittiming(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   CAN set data bit timing routine

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Return**

0 on success, -`EPERM` on error

int ctucan\_set\_secondary\_sample\_point(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Sets secondary sample point in CTU CAN FD

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Return**

0 on success, -`EPERM` if controller is enabled

void ctucan\_set\_mode(struct ctucan\_priv \*priv, const struct can\_ctrlmode \*mode)
:   Sets CTU CAN FDs mode

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to private data

`const struct can_ctrlmode *mode`
:   Pointer to controller modes to be set

int ctucan\_chip\_start(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   This routine starts the driver

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Description**

Routine expects that chip is in reset state. It setups initial
Tx buffers for FIFO priorities, sets bittiming, enables interrupts,
switches core to operational mode and changes controller
state to `CAN_STATE_STOPPED`.

**Return**

0 on success and failure value on error

int ctucan\_do\_set\_mode(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev, enum can\_mode mode)
:   Sets mode of the driver

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

`enum can_mode mode`
:   Tells the mode of the driver

**Description**

This check the drivers state and calls the corresponding modes to set.

**Return**

0 on success and failure value on error

enum ctucan\_txtb\_status ctucan\_get\_tx\_status(struct ctucan\_priv \*priv, u8 buf)
:   Gets status of TXT buffer

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to private data

`u8 buf`
:   Buffer index (0-based)

**Return**

Status of TXT buffer

bool ctucan\_is\_txt\_buf\_writable(struct ctucan\_priv \*priv, u8 buf)
:   Checks if frame can be inserted to TXT Buffer

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to private data

`u8 buf`
:   Buffer index (0-based)

**Return**

True - Frame can be inserted to TXT Buffer, False - If attempted, frame will not be
inserted to TXT Buffer

bool ctucan\_insert\_frame(struct ctucan\_priv \*priv, const struct canfd\_frame \*cf, u8 buf, bool isfdf)
:   Inserts frame to TXT buffer

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to private data

`const struct canfd_frame *cf`
:   Pointer to CAN frame to be inserted

`u8 buf`
:   TXT Buffer index to which frame is inserted (0-based)

`bool isfdf`
:   True - CAN FD Frame, False - CAN 2.0 Frame

**Return**

* True - Frame inserted successfully
* False - Frame was not inserted due to one of:
  :   1. TXT Buffer is not writable (it is in wrong state)
      2. Invalid TXT buffer index
      3. Invalid frame length

void ctucan\_give\_txtb\_cmd(struct ctucan\_priv \*priv, enum ctucan\_txtb\_command cmd, u8 buf)
:   Applies command on TXT buffer

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to private data

`enum ctucan_txtb_command cmd`
:   Command to give

`u8 buf`
:   Buffer index (0-based)

netdev\_tx\_t ctucan\_start\_xmit(struct [sk\_buff](../../../kapi.html#c.sk_buff "sk_buff") \*skb, struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Starts the transmission

**Parameters**

`struct sk_buff *skb`
:   sk\_buff pointer that contains data to be Txed

`struct net_device *ndev`
:   Pointer to net\_device structure

**Description**

Invoked from upper layers to initiate transmission. Uses the next available free TXT Buffer and
populates its fields to start the transmission.

**Return**

`NETDEV_TX_OK` on success, `NETDEV_TX_BUSY` when no free TXT buffer is available,
negative return values reserved for error cases

void ctucan\_read\_rx\_frame(struct ctucan\_priv \*priv, struct canfd\_frame \*cf, u32 ffw)
:   Reads frame from RX FIFO

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to CTU CAN FD’s private data

`struct canfd_frame *cf`
:   Pointer to CAN frame struct

`u32 ffw`
:   Previously read frame format word

**Note**

Frame format word must be read separately and provided in ‘ffw’.

int ctucan\_rx(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Called from CAN ISR to complete the received frame processing

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Description**

This function is invoked from the CAN isr(poll) to process the Rx frames. It does minimal
processing and invokes “netif\_receive\_skb” to complete further processing.

**Return**

1 when frame is passed to the network layer, 0 when the first frame word is read but
system is out of free SKBs temporally and left code to resolve SKB allocation later,
-`EAGAIN` in a case of empty Rx FIFO.

enum can\_state ctucan\_read\_fault\_state(struct ctucan\_priv \*priv)
:   Reads CTU CAN FDs fault confinement state.

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to private data

**Return**

Fault confinement state of controller

void ctucan\_get\_rec\_tec(struct ctucan\_priv \*priv, struct can\_berr\_counter \*bec)
:   Reads REC/TEC counter values from controller

**Parameters**

`struct ctucan_priv *priv`
:   Pointer to private data

`struct can_berr_counter *bec`
:   Pointer to Error counter structure

void ctucan\_err\_interrupt(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev, u32 isr)
:   Error frame ISR

**Parameters**

`struct net_device *ndev`
:   net\_device pointer

`u32 isr`
:   interrupt status register value

**Description**

This is the CAN error interrupt and it will check the type of error and forward the error
frame to upper layers.

int ctucan\_rx\_poll(struct napi\_struct \*napi, int quota)
:   Poll routine for rx packets (NAPI)

**Parameters**

`struct napi_struct *napi`
:   NAPI structure pointer

`int quota`
:   Max number of rx packets to be processed.

**Description**

This is the poll routine for rx part. It will process the packets maximux quota value.

**Return**

Number of packets received

void ctucan\_rotate\_txb\_prio(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Rotates priorities of TXT Buffers

**Parameters**

`struct net_device *ndev`
:   net\_device pointer

void ctucan\_tx\_interrupt(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Tx done Isr

**Parameters**

`struct net_device *ndev`
:   net\_device pointer

irqreturn\_t ctucan\_interrupt(int irq, void \*dev\_id)
:   CAN Isr

**Parameters**

`int irq`
:   irq number

`void *dev_id`
:   device id pointer

**Description**

This is the CTU CAN FD ISR. It checks for the type of interrupt
and invokes the corresponding ISR.

**Return**

IRQ\_NONE - If CAN device is in sleep mode, IRQ\_HANDLED otherwise

void ctucan\_chip\_stop(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Driver stop routine

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Description**

This is the drivers stop routine. It will disable the
interrupts and disable the controller.

int ctucan\_open(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Driver open routine

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Description**

This is the driver open routine.

**Return**

0 on success and failure value on error

int ctucan\_close(struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev)
:   Driver close routine

**Parameters**

`struct net_device *ndev`
:   Pointer to net\_device structure

**Return**

0 always

int ctucan\_get\_berr\_counter(const struct [net\_device](../../../kapi.html#c.net_device "net_device") \*ndev, struct can\_berr\_counter \*bec)
:   error counter routine

**Parameters**

`const struct net_device *ndev`
:   Pointer to net\_device structure

`struct can_berr_counter *bec`
:   Pointer to can\_berr\_counter structure

**Description**

This is the driver error counter routine.

**Return**

0 on success and failure value on error

int ctucan\_pci\_probe(struct pci\_dev \*pdev, const struct [pci\_device\_id](../../../../PCI/pci.html#c.pci_device_id "pci_device_id") \*ent)
:   PCI registration call

**Parameters**

`struct pci_dev *pdev`
:   Handle to the pci device structure

`const struct pci_device_id *ent`
:   Pointer to the entry from ctucan\_pci\_tbl

**Description**

This function does all the memory allocation and registration for the CAN
device.

**Return**

0 on success and failure value on error

void ctucan\_pci\_remove(struct pci\_dev \*pdev)
:   Unregister the device after releasing the resources

**Parameters**

`struct pci_dev *pdev`
:   Handle to the pci device structure

**Description**

This function frees all the resources allocated to the device.

**Return**

0 always

int ctucan\_platform\_probe(struct platform\_device \*pdev)
:   Platform registration call

**Parameters**

`struct platform_device *pdev`
:   Handle to the platform device structure

**Description**

This function does all the memory allocation and registration for the CAN
device.

**Return**

0 on success and failure value on error

void ctucan\_platform\_remove(struct platform\_device \*pdev)
:   Unregister the device after releasing the resources

**Parameters**

`struct platform_device *pdev`
:   Handle to the platform device structure

**Description**

This function frees all the resources allocated to the device.

**Return**

0 always

## CTU CAN FD IP Core and Driver Development Acknowledgment

* Odrej Ille <[ondrej.ille@gmail.com](mailto:ondrej.ille%40gmail.com)>

  + started the project as student at Department of Measurement, FEE, CTU
  + invested great amount of personal time and enthusiasm to the project over years
  + worked on more funded tasks
* [Department of Measurement](https://meas.fel.cvut.cz/),
  [Faculty of Electrical Engineering](http://www.fel.cvut.cz/en/),
  [Czech Technical University](https://www.cvut.cz/en)

  + is the main investor into the project over many years
  + uses project in their CAN/CAN FD diagnostics framework for [Skoda Auto](https://www.skoda-auto.cz/)
* [Digiteq Automotive](https://www.digiteqautomotive.com/en)

  + funding of the project CAN FD Open Cores Support Linux Kernel Based Systems
  + negotiated and paid CTU to allow public access to the project
  + provided additional funding of the work
* [Department of Control Engineering](https://control.fel.cvut.cz/en),
  [Faculty of Electrical Engineering](http://www.fel.cvut.cz/en/),
  [Czech Technical University](https://www.cvut.cz/en)

  + solving the project CAN FD Open Cores Support Linux Kernel Based Systems
  + providing GitLab management
  + virtual servers and computational power for continuous integration
  + providing hardware for HIL continuous integration tests
* [PiKRON Ltd.](http://pikron.com/)

  + minor funding to initiate preparation of the project open-sourcing
* Petr Porazil <[porazil@pikron.com](mailto:porazil%40pikron.com)>

  + design of PCIe transceiver addon board and assembly of boards
  + design and assembly of MZ\_APO baseboard for MicroZed/Zynq based system
* Martin Jerabek <[martin.jerabek01@gmail.com](mailto:martin.jerabek01%40gmail.com)>

  + Linux driver development
  + continuous integration platform architect and GHDL updates
  + thesis [Open-source and Open-hardware CAN FD Protocol Support](https://dspace.cvut.cz/bitstream/handle/10467/80366/F3-DP-2019-Jerabek-Martin-Jerabek-thesis-2019-canfd.pdf)
* Jiri Novak <[jnovak@fel.cvut.cz](mailto:jnovak%40fel.cvut.cz)>

  + project initiation, management and use at Department of Measurement, FEE, CTU
* Pavel Pisa <[pisa@cmp.felk.cvut.cz](mailto:pisa%40cmp.felk.cvut.cz)>

  + initiate open-sourcing, project coordination, management at Department of Control Engineering, FEE, CTU
* Jaroslav Beran<[jara.beran@gmail.com](mailto:jara.beran%40gmail.com)>

> * system integration for Intel SoC, core and driver testing and updates

* Carsten Emde ([OSADL](https://www.osadl.org/))

> * provided OSADL expertise to discuss IP core licensing
> * pointed to possible deadlock for LGPL and CAN bus possible patent case which lead to relicense IP core design to BSD like license

* Reiner Zitzmann and Holger Zeltwanger ([CAN in Automation](https://www.can-cia.org/))

> * provided suggestions and help to inform community about the project and invited us to events focused on CAN bus future development directions

* Jan Charvat

> * implemented CTU CAN FD functional model for QEMU which has been integrated into QEMU mainline ([docs/system/devices/can.rst](https://www.qemu.org/docs/master/system/devices/can.html))
> * Bachelor thesis Model of CAN FD Communication Controller for QEMU Emulator

## Notes

[[1](#id1)]

Other buses have their own specific driver interface to set up the
device.


[[2](#id2)]

Not to be mistaken with CAN Error Frame. This is a `can_frame` with
`CAN_ERR_FLAG` set and some error info in its `data` field.


[[3](#id3)]

Available in CTU CAN FD repository
<https://gitlab.fel.cvut.cz/canbus/ctucanfd_ip_core>


[[4](#id4)]

As is done in the low-level driver functions
`ctucan_hw_set_nom_bittiming` and
`ctucan_hw_set_data_bittiming`.


[[5](#id5)]

At the time of writing this thesis, option 1 is still being used and
the modification is queued in gitlab issue #222


[[6](#id6)]

Strictly speaking, multiple CAN TX queues are supported since v4.19
[can: enable multi-queue for SocketCAN devices](https://lore.kernel.org/patchwork/patch/913526/) but no mainline driver is using
them yet.


[[7](#id7)]

Or rather in the next clock cycle
