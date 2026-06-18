# 5.Remote Controller devices

> 출처(원문): https://docs.kernel.org/driver-api/media/rc-core.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5. Remote Controller devices

## 5.1. Remote Controller core

The remote controller core implements infrastructure to receive and send
remote controller keyboard keystrokes and mouse events.

Every time a key is pressed on a remote controller, a scan code is produced.
Also, on most hardware, keeping a key pressed for more than a few dozens of
milliseconds produce a repeat key event. That’s somewhat similar to what
a normal keyboard or mouse is handled internally on Linux[[1]](#f1). So, the
remote controller core is implemented on the top of the linux input/evdev
interface.

[[1](#id1)]

The main difference is that, on keyboard events, the keyboard controller
produces one event for a key press and another one for key release. On
infrared-based remote controllers, there’s no key release event. Instead,
an extra code is produced to indicate key repeats.

However, most of the remote controllers use infrared (IR) to transmit signals.
As there are several protocols used to modulate infrared signals, one
important part of the core is dedicated to adjust the driver and the core
system to support the infrared protocol used by the emitter.

The infrared transmission is done by blinking a infrared emitter using a
carrier. The carrier can be switched on or off by the IR transmitter
hardware. When the carrier is switched on, it is called *PULSE*.
When the carrier is switched off, it is called *SPACE*.

In other words, a typical IR transmission can be viewed as a sequence of
*PULSE* and *SPACE* events, each with a given duration.

The carrier parameters (frequency, duty cycle) and the intervals for
*PULSE* and *SPACE* events depend on the protocol.
For example, the NEC protocol uses a carrier of 38kHz, and transmissions
start with a 9ms *PULSE* and a 4.5ms SPACE. It then transmits 16 bits of
scan code, being 8 bits for address (usually it is a fixed number for a
given remote controller), followed by 8 bits of code. A bit “1” is modulated
with 560µs *PULSE* followed by 1690µs *SPACE* and a bit “0” is modulated
with 560µs *PULSE* followed by 560µs *SPACE*.

At receiver, a simple low-pass filter can be used to convert the received
signal in a sequence of *PULSE/SPACE* events, filtering out the carrier
frequency. Due to that, the receiver doesn’t care about the carrier’s
actual frequency parameters: all it has to do is to measure the amount
of time it receives *PULSE/SPACE* events.
So, a simple IR receiver hardware will just provide a sequence of timings
for those events to the Kernel. The drivers for hardware with such kind of
receivers are identified by `RC_DRIVER_IR_RAW`, as defined by
[`rc_driver_type`](#c.rc_driver_type "rc_driver_type")[[2]](#f2). Other hardware come with a
microcontroller that decode the *PULSE/SPACE* sequence and return scan
codes to the Kernel. Such kind of receivers are identified
by `RC_DRIVER_SCANCODE`.

[[2](#id2)]

The RC core also supports devices that have just IR emitters,
without any receivers. Right now, all such devices work only in
raw TX mode. Such kind of hardware is identified as
`RC_DRIVER_IR_RAW_TX`.

When the RC core receives events produced by `RC_DRIVER_IR_RAW` IR
receivers, it needs to decode the IR protocol, in order to obtain the
corresponding scan code. The protocols supported by the RC core are
defined at enum [`rc_proto`](../../userspace-api/media/rc/lirc-dev-intro.html#c.rc_proto "rc_proto").

When the RC code receives a scan code (either directly, by a driver
of the type `RC_DRIVER_SCANCODE`, or via its IR decoders), it needs
to convert into a Linux input event code. This is done via a mapping
table.

The Kernel has support for mapping tables available on most media
devices. It also supports loading a table in runtime, via some
sysfs nodes. See the [RC userspace API](../../userspace-api/media/rc/rc-intro.html#remote-controllers-intro)
for more details.

### 5.1.1. Remote controller data structures and functions

enum rc\_driver\_type
:   type of the RC driver.

**Constants**

`RC_DRIVER_SCANCODE`
:   Driver or hardware generates a scancode.

`RC_DRIVER_IR_RAW`
:   Driver or hardware generates pulse/space sequences.
    It needs a Infra-Red pulse/space decoder

`RC_DRIVER_IR_RAW_TX`
:   Device transmitter only,
    driver requires pulse/space data sequence.

struct rc\_scancode\_filter
:   Filter scan codes.

**Definition**:

```
struct rc_scancode_filter {
    u32 data;
    u32 mask;
};
```

**Members**

`data`
:   Scancode data to match.

`mask`
:   Mask of bits of scancode to compare.

enum rc\_filter\_type
:   Filter type constants.

**Constants**

`RC_FILTER_NORMAL`
:   Filter for normal operation.

`RC_FILTER_WAKEUP`
:   Filter for waking from suspend.

`RC_FILTER_MAX`
:   Number of filter types.

struct lirc\_fh
:   represents an open lirc file

**Definition**:

```
struct lirc_fh {
    struct list_head list;
    struct rc_dev *rc;
    unsigned int *rawir;
    struct lirc_scancode *scancodes;
    wait_queue_head_t wait_poll;
    u32 carrier_low;
    u8 send_mode;
    u8 rec_mode;
};
```

**Members**

`list`
:   list of open file handles

`rc`
:   rcdev for this lirc chardev

`rawir`
:   queue for incoming raw IR

`scancodes`
:   queue for incoming decoded scancodes

`wait_poll`
:   poll struct for lirc device

`carrier_low`
:   when setting the carrier range, first the low end must be
    set with an ioctl and then the high end with another ioctl

`send_mode`
:   lirc mode for sending, either LIRC\_MODE\_SCANCODE or
    LIRC\_MODE\_PULSE

`rec_mode`
:   lirc mode for receiving, either LIRC\_MODE\_SCANCODE or
    LIRC\_MODE\_MODE2

struct rc\_dev
:   represents a remote control device

**Definition**:

```
struct rc_dev {
    struct device                   dev;
    bool registered;
    bool idle;
    bool encode_wakeup;
    unsigned int                    minor;
    const struct attribute_group    *sysfs_groups[5];
    const char                      *device_name;
    const char                      *input_phys;
    struct input_id                 input_id;
    const char                      *driver_name;
    const char                      *map_name;
    struct rc_map                   rc_map;
    struct mutex                    lock;
    struct ir_raw_event_ctrl        *raw;
    struct input_dev                *input_dev;
    enum rc_driver_type             driver_type;
    u32 users;
    u64 allowed_protocols;
    u64 enabled_protocols;
    u64 allowed_wakeup_protocols;
    enum rc_proto                   wakeup_protocol;
    struct rc_scancode_filter       scancode_filter;
    struct rc_scancode_filter       scancode_wakeup_filter;
    u32 scancode_mask;
    void *priv;
    spinlock_t keylock;
    bool keypressed;
    u8 last_toggle;
    u32 last_keycode;
    enum rc_proto                   last_protocol;
    u64 last_scancode;
    unsigned long                   keyup_jiffies;
    struct timer_list               timer_keyup;
    struct timer_list               timer_repeat;
    u32 timeout;
    u32 min_timeout;
    u32 max_timeout;
    u32 rx_resolution;
#ifdef CONFIG_LIRC;
    struct device                   lirc_dev;
    struct cdev                     lirc_cdev;
    ktime_t gap_start;
    spinlock_t lirc_fh_lock;
    struct list_head                lirc_fh;
#endif;
    int (*change_protocol)(struct rc_dev *dev, u64 *rc_proto);
    int (*open)(struct rc_dev *dev);
    void (*close)(struct rc_dev *dev);
    int (*s_tx_mask)(struct rc_dev *dev, u32 mask);
    int (*s_tx_carrier)(struct rc_dev *dev, u32 carrier);
    int (*s_tx_duty_cycle)(struct rc_dev *dev, u32 duty_cycle);
    int (*s_rx_carrier_range)(struct rc_dev *dev, u32 min, u32 max);
    int (*tx_ir)(struct rc_dev *dev, unsigned *txbuf, unsigned n);
    void (*s_idle)(struct rc_dev *dev, bool enable);
    int (*s_wideband_receiver)(struct rc_dev *dev, int enable);
    int (*s_carrier_report) (struct rc_dev *dev, int enable);
    int (*s_filter)(struct rc_dev *dev, struct rc_scancode_filter *filter);
    int (*s_wakeup_filter)(struct rc_dev *dev, struct rc_scancode_filter *filter);
    int (*s_timeout)(struct rc_dev *dev, unsigned int timeout);
};
```

**Members**

`dev`
:   driver model’s view of this device

`registered`
:   set to true by [`rc_register_device()`](#c.rc_register_device "rc_register_device"), false by
    rc\_unregister\_device

`idle`
:   used to keep track of RX state

`encode_wakeup`
:   wakeup filtering uses IR encode API, therefore the allowed
    wakeup protocols is the set of all raw encoders

`minor`
:   unique minor remote control device number

`sysfs_groups`
:   sysfs attribute groups

`device_name`
:   name of the rc child device

`input_phys`
:   physical path to the input child device

`input_id`
:   id of the input child device (`struct input_id`)

`driver_name`
:   name of the hardware driver which registered this device

`map_name`
:   name of the default keymap

`rc_map`
:   current scan/key table

`lock`
:   used to ensure we’ve filled in all protocol details before
    anyone can call show\_protocols or store\_protocols

`raw`
:   additional data for raw pulse/space devices

`input_dev`
:   the input child device used to communicate events to userspace

`driver_type`
:   specifies if protocol decoding is done in hardware or software

`users`
:   number of current users of the device

`allowed_protocols`
:   bitmask with the supported RC\_PROTO\_BIT\_\* protocols

`enabled_protocols`
:   bitmask with the enabled RC\_PROTO\_BIT\_\* protocols

`allowed_wakeup_protocols`
:   bitmask with the supported RC\_PROTO\_BIT\_\* wakeup
    protocols

`wakeup_protocol`
:   the enabled RC\_PROTO\_\* wakeup protocol or
    RC\_PROTO\_UNKNOWN if disabled.

`scancode_filter`
:   scancode filter

`scancode_wakeup_filter`
:   scancode wakeup filters

`scancode_mask`
:   some hardware decoders are not capable of providing the full
    scancode to the application. As this is a hardware limit, we can’t do
    anything with it. Yet, as the same keycode table can be used with other
    devices, a mask is provided to allow its usage. Drivers should generally
    leave this field in blank

`priv`
:   driver-specific data

`keylock`
:   protects the remaining members of the struct

`keypressed`
:   whether a key is currently pressed

`last_toggle`
:   toggle value of last command

`last_keycode`
:   keycode of last keypress

`last_protocol`
:   protocol of last keypress

`last_scancode`
:   scancode of last keypress

`keyup_jiffies`
:   time (in jiffies) when the current keypress should be released

`timer_keyup`
:   timer for releasing a keypress

`timer_repeat`
:   timer for autorepeat events. This is needed for CEC, which
    has non-standard repeats.

`timeout`
:   optional time after which device stops sending data

`min_timeout`
:   minimum timeout supported by device

`max_timeout`
:   maximum timeout supported by device

`rx_resolution`
:   resolution (in us) of input sampler

`lirc_dev`
:   lirc device

`lirc_cdev`
:   lirc char cdev

`gap_start`
:   start time for gap after timeout if non-zero

`lirc_fh_lock`
:   protects lirc\_fh list

`lirc_fh`
:   list of open files

`change_protocol`
:   allow changing the protocol used on hardware decoders

`open`
:   callback to allow drivers to enable polling/irq when IR input device
    is opened.

`close`
:   callback to allow drivers to disable polling/irq when IR input device
    is opened.

`s_tx_mask`
:   set transmitter mask (for devices with multiple tx outputs)

`s_tx_carrier`
:   set transmit carrier frequency

`s_tx_duty_cycle`
:   set transmit duty cycle (0% - 100%)

`s_rx_carrier_range`
:   inform driver about carrier it is expected to handle

`tx_ir`
:   transmit IR

`s_idle`
:   enable/disable hardware idle mode, upon which,
    device doesn’t interrupt host until it sees IR pulses

`s_wideband_receiver`
:   enable wide band receiver used for learning

`s_carrier_report`
:   enable carrier reports

`s_filter`
:   set the scancode filter

`s_wakeup_filter`
:   set the wakeup scancode filter. If the mask is zero
    then wakeup should be disabled. wakeup\_protocol will be set to
    a valid protocol if mask is nonzero.

`s_timeout`
:   set hardware timeout in us

struct [rc\_dev](#c.rc_dev "rc_dev") \*rc\_allocate\_device(enum [rc\_driver\_type](#c.rc_driver_type "rc_driver_type"))
:   Allocates a RC device

**Parameters**

`enum rc_driver_type`
:   specifies the type of the RC output to be allocated
    returns a pointer to [`struct rc_dev`](#c.rc_dev "rc_dev").

struct [rc\_dev](#c.rc_dev "rc_dev") \*devm\_rc\_allocate\_device(struct [device](../infrastructure.html#c.device "device") \*dev, enum [rc\_driver\_type](#c.rc_driver_type "rc_driver_type"))
:   Managed RC device allocation

**Parameters**

`struct device *dev`
:   pointer to [`struct device`](../infrastructure.html#c.device "device")

`enum rc_driver_type`
:   specifies the type of the RC output to be allocated
    returns a pointer to [`struct rc_dev`](#c.rc_dev "rc_dev").

void rc\_free\_device(struct [rc\_dev](#c.rc_dev "rc_dev") \*dev)
:   Frees a RC device

**Parameters**

`struct rc_dev *dev`
:   pointer to [`struct rc_dev`](#c.rc_dev "rc_dev").

int rc\_register\_device(struct [rc\_dev](#c.rc_dev "rc_dev") \*dev)
:   Registers a RC device

**Parameters**

`struct rc_dev *dev`
:   pointer to [`struct rc_dev`](#c.rc_dev "rc_dev").

int devm\_rc\_register\_device(struct [device](../infrastructure.html#c.device "device") \*parent, struct [rc\_dev](#c.rc_dev "rc_dev") \*dev)
:   Manageded registering of a RC device

**Parameters**

`struct device *parent`
:   pointer to [`struct device`](../infrastructure.html#c.device "device").

`struct rc_dev *dev`
:   pointer to [`struct rc_dev`](#c.rc_dev "rc_dev").

void rc\_unregister\_device(struct [rc\_dev](#c.rc_dev "rc_dev") \*dev)
:   Unregisters a RC device

**Parameters**

`struct rc_dev *dev`
:   pointer to [`struct rc_dev`](#c.rc_dev "rc_dev").

struct rc\_map\_table
:   represents a scancode/keycode pair

**Definition**:

```
struct rc_map_table {
    u64 scancode;
    u32 keycode;
};
```

**Members**

`scancode`
:   scan code (u64)

`keycode`
:   Linux input keycode

struct rc\_map
:   represents a keycode map table

**Definition**:

```
struct rc_map {
    struct rc_map_table     *scan;
    unsigned int            size;
    unsigned int            len;
    unsigned int            alloc;
    enum rc_proto           rc_proto;
    const char              *name;
    spinlock_t lock;
};
```

**Members**

`scan`
:   pointer to struct [`rc_map_table`](#c.rc_map_table "rc_map_table")

`size`
:   Max number of entries

`len`
:   Number of entries that are in use

`alloc`
:   size of \*scan, in bytes

`rc_proto`
:   type of the remote controller protocol, as defined at
    enum [`rc_proto`](../../userspace-api/media/rc/lirc-dev-intro.html#c.rc_proto "rc_proto")

`name`
:   name of the key map table

`lock`
:   lock to protect access to this structure

struct rc\_map\_list
:   list of the registered [`rc_map`](#c.rc_map "rc_map") maps

**Definition**:

```
struct rc_map_list {
    struct list_head         list;
    struct rc_map map;
};
```

**Members**

`list`
:   pointer to struct `list_head`

`map`
:   pointer to struct [`rc_map`](#c.rc_map "rc_map")

int rc\_map\_register(struct [rc\_map\_list](#c.rc_map_list "rc_map_list") \*map)
:   Registers a Remote Controller scancode map

**Parameters**

`struct rc_map_list *map`
:   pointer to [`struct rc_map_list`](#c.rc_map_list "rc_map_list")

void rc\_map\_unregister(struct [rc\_map\_list](#c.rc_map_list "rc_map_list") \*map)
:   Unregisters a Remote Controller scancode map

**Parameters**

`struct rc_map_list *map`
:   pointer to [`struct rc_map_list`](#c.rc_map_list "rc_map_list")

struct [rc\_map](#c.rc_map "rc_map") \*rc\_map\_get(const char \*name)
:   gets an RC map from its name

**Parameters**

`const char *name`
:   name of the RC scancode map
