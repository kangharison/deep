# The Linux-USB Host Side API

> 출처(원문): https://docs.kernel.org/driver-api/usb/usb.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux-USB Host Side API

## Introduction to USB on Linux

A Universal Serial Bus (USB) is used to connect a host, such as a PC or
workstation, to a number of peripheral devices. USB uses a tree
structure, with the host as the root (the system’s master), hubs as
interior nodes, and peripherals as leaves (and slaves). Modern PCs
support several such trees of USB devices, usually
a few USB 3.0 (5 GBit/s) or USB 3.1 (10 GBit/s) and some legacy
USB 2.0 (480 MBit/s) buses just in case.

That master/slave asymmetry was designed-in for a number of reasons, one
being ease of use. It is not physically possible to mistake upstream and
downstream or it does not matter with a type C plug (or they are built into the
peripheral). Also, the host software doesn’t need to deal with
distributed auto-configuration since the pre-designated master node
manages all that.

Kernel developers added USB support to Linux early in the 2.2 kernel
series and have been developing it further since then. Besides support
for each new generation of USB, various host controllers gained support,
new drivers for peripherals have been added and advanced features for latency
measurement and improved power management introduced.

Linux can run inside USB devices as well as on the hosts that control
the devices. But USB device drivers running inside those peripherals
don’t do the same things as the ones running inside hosts, so they’ve
been given a different name: *gadget drivers*. This document does not
cover gadget drivers.

## USB Host-Side API Model

Host-side drivers for USB devices talk to the “usbcore” APIs. There are
two. One is intended for *general-purpose* drivers (exposed through
driver frameworks), and the other is for drivers that are *part of the
core*. Such core drivers include the *hub* driver (which manages trees
of USB devices) and several different kinds of *host controller
drivers*, which control individual buses.

The device model seen by USB drivers is relatively complex.

* USB supports four kinds of data transfers (control, bulk, interrupt,
  and isochronous). Two of them (control and bulk) use bandwidth as
  it’s available, while the other two (interrupt and isochronous) are
  scheduled to provide guaranteed bandwidth.
* The device description model includes one or more “configurations”
  per device, only one of which is active at a time. Devices are supposed
  to be capable of operating at lower than their top
  speeds and may provide a BOS descriptor showing the lowest speed they
  remain fully operational at.
* From USB 3.0 on configurations have one or more “functions”, which
  provide a common functionality and are grouped together for purposes
  of power management.
* Configurations or functions have one or more “interfaces”, each of which may have
  “alternate settings”. Interfaces may be standardized by USB “Class”
  specifications, or may be specific to a vendor or device.

  USB device drivers actually bind to interfaces, not devices. Think of
  them as “interface drivers”, though you may not see many devices
  where the distinction is important. *Most USB devices are simple,
  with only one function, one configuration, one interface, and one alternate
  setting.*
* Interfaces have one or more “endpoints”, each of which supports one
  type and direction of data transfer such as “bulk out” or “interrupt
  in”. The entire configuration may have up to sixteen endpoints in
  each direction, allocated as needed among all the interfaces.
* Data transfer on USB is packetized; each endpoint has a maximum
  packet size. Drivers must often be aware of conventions such as
  flagging the end of bulk transfers using “short” (including zero
  length) packets.
* The Linux USB API supports synchronous calls for control and bulk
  messages. It also supports asynchronous calls for all kinds of data
  transfer, using request structures called “URBs” (USB Request
  Blocks).

Accordingly, the USB Core API exposed to device drivers covers quite a
lot of territory. You’ll probably need to consult the USB 3.0
specification, available online from www.usb.org at no cost, as well as
class or device specifications.

The only host-side drivers that actually touch hardware (reading/writing
registers, handling IRQs, and so on) are the HCDs. In theory, all HCDs
provide the same functionality through the same API. In practice, that’s
becoming more true, but there are still differences
that crop up especially with fault handling on the less common controllers.
Different controllers don’t
necessarily report the same aspects of failures, and recovery from
faults (including software-induced ones like unlinking an URB) isn’t yet
fully consistent. Device driver authors should make a point of doing
disconnect testing (while the device is active) with each different host
controller driver, to make sure drivers don’t have bugs of their own as
well as to make sure they aren’t relying on some HCD-specific behavior.

## USB-Standard Types

In `include/uapi/linux/usb/ch9.h` you will find the USB data types defined
in chapter 9 of the USB specification. These data types are used throughout
USB, and in APIs including this host side API, gadget APIs, usb character
devices and debugfs interfaces. That file is itself included by
`include/linux/usb/ch9.h`, which also contains declarations of a few
utility routines for manipulating these data types; the implementations
are in `drivers/usb/common/common.c`.

const char \*usb\_ep\_type\_string(int ep\_type)
:   Returns human readable-name of the endpoint type.

**Parameters**

`int ep_type`
:   The endpoint type to return human-readable name for. If it’s not
    any of the types: USB\_ENDPOINT\_XFER\_{CONTROL, ISOC, BULK, INT},
    usually got by `usb_endpoint_type()`, the string ‘unknown’ will be returned.

const char \*usb\_otg\_state\_string(enum usb\_otg\_state state)
:   returns human readable name of OTG state.

**Parameters**

`enum usb_otg_state state`
:   the OTG state to return the human readable name of. If it’s not
    any of the states defined in usb\_otg\_state enum, ‘UNDEFINED’ will be
    returned.

const char \*usb\_speed\_string(enum usb\_device\_speed speed)
:   Returns human readable-name of the speed.

**Parameters**

`enum usb_device_speed speed`
:   The speed to return human-readable name for. If it’s not
    any of the speeds defined in usb\_device\_speed enum, string for
    USB\_SPEED\_UNKNOWN will be returned.

enum usb\_device\_speed usb\_get\_maximum\_speed(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Get maximum requested speed for a given USB controller.

**Parameters**

`struct device *dev`
:   Pointer to the given USB controller device

**Description**

The function gets the maximum speed string from property “maximum-speed”,
and returns the corresponding `enum usb_device_speed`.

enum usb\_ssp\_rate usb\_get\_maximum\_ssp\_rate(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Get the signaling rate generation and lane count of a SuperSpeed Plus capable device.

**Parameters**

`struct device *dev`
:   Pointer to the given USB controller device

**Description**

If the string from “maximum-speed” property is super-speed-plus-genXxY where
‘X’ is the generation number and ‘Y’ is the number of lanes, then this
function returns the corresponding `enum usb_ssp_rate`.

const char \*usb\_state\_string(enum usb\_device\_state state)
:   Returns human readable name for the state.

**Parameters**

`enum usb_device_state state`
:   The state to return a human-readable name for. If it’s not
    any of the states devices in usb\_device\_state\_string enum,
    the string UNKNOWN will be returned.

enum usb\_dr\_mode usb\_get\_role\_switch\_default\_mode(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Get default mode for given device

**Parameters**

`struct device *dev`
:   Pointer to the given device

**Description**

The function gets string from property ‘role-switch-default-mode’,
and returns the corresponding `enum usb_dr_mode`.

unsigned int usb\_decode\_interval(const struct usb\_endpoint\_descriptor \*epd, enum usb\_device\_speed speed)
:   Decode bInterval into the time expressed in 1us unit

**Parameters**

`const struct usb_endpoint_descriptor *epd`
:   The descriptor of the endpoint

`enum usb_device_speed speed`
:   The speed that the endpoint works as

**Description**

Function returns the interval expressed in 1us unit for servicing
endpoint for data transfers.

enum usb\_dr\_mode of\_usb\_get\_dr\_mode\_by\_phy(struct device\_node \*np, int arg0)
:   Get dual role mode for the controller device which is associated with the given phy device\_node

**Parameters**

`struct device_node *np`
:   Pointer to the given phy device\_node

`int arg0`
:   phandle args[0] for phy’s with #phy-cells >= 1, or -1 for
    phys which do not have phy-cells

**Description**

In dts a usb controller associates with phy devices. The function gets
the string from property ‘dr\_mode’ of the controller associated with the
given phy device node, and returns the correspondig `enum usb_dr_mode`.

bool of\_usb\_host\_tpl\_support(struct device\_node \*np)
:   to get if Targeted Peripheral List is supported for given targeted hosts (non-PC hosts)

**Parameters**

`struct device_node *np`
:   Pointer to the given device\_node

**Description**

The function gets if the targeted hosts support TPL or not

int of\_usb\_update\_otg\_caps(struct device\_node \*np, struct usb\_otg\_caps \*otg\_caps)
:   to update usb otg capabilities according to the passed properties in DT.

**Parameters**

`struct device_node *np`
:   Pointer to the given device\_node

`struct usb_otg_caps *otg_caps`
:   Pointer to the target usb\_otg\_caps to be set

**Description**

The function updates the otg capabilities

struct [device](../infrastructure.html#c.device "device") \*usb\_of\_get\_companion\_dev(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Find the companion device

**Parameters**

`struct device *dev`
:   the device pointer to find a companion

**Description**

Find the companion device from platform bus.

Takes a reference to the returned `struct device` which needs to be dropped
after use.

**Return**

On success, a pointer to the companion device, `NULL` on failure.

In addition, some functions useful for creating debugging output are
defined in `drivers/usb/common/debug.c`.

## Host-Side Data Types and Macros

The host side API exposes several layers to drivers, some of which are
more necessary than others. These support lifecycle models for host side
drivers and devices, and support passing buffers through usbcore to some
HCD that performs the I/O for the device driver.

struct usb\_host\_endpoint
:   host-side endpoint descriptor and queue

**Definition**:

```
struct usb_host_endpoint {
    struct usb_endpoint_descriptor                  desc;
    struct usb_ss_ep_comp_descriptor                ss_ep_comp;
    struct usb_ssp_isoc_ep_comp_descriptor          ssp_isoc_ep_comp;
    struct usb_eusb2_isoc_ep_comp_descriptor        eusb2_isoc_ep_comp;
    struct list_head                urb_list;
    void *hcpriv;
    struct ep_device                *ep_dev;
    unsigned char *extra;
    int extralen;
    int enabled;
    int streams;
};
```

**Members**

`desc`
:   descriptor for this endpoint, wMaxPacketSize in native byteorder

`ss_ep_comp`
:   SuperSpeed companion descriptor for this endpoint

`ssp_isoc_ep_comp`
:   SuperSpeedPlus isoc companion descriptor for this endpoint

`eusb2_isoc_ep_comp`
:   eUSB2 isoc companion descriptor for this endpoint

`urb_list`
:   urbs queued to this endpoint; maintained by usbcore

`hcpriv`
:   for use by HCD; typically holds hardware dma queue head (QH)
    with one or more transfer descriptors (TDs) per urb; must be preserved
    by core while BW is allocated for the endpoint

`ep_dev`
:   ep\_device for sysfs info

`extra`
:   descriptors following this endpoint in the configuration

`extralen`
:   how many bytes of “extra” are valid

`enabled`
:   URBs may be submitted to this endpoint

`streams`
:   number of USB-3 streams allocated on the endpoint

**Description**

USB requests are always queued to a given endpoint, identified by a
descriptor within an active interface in a given USB configuration.

struct usb\_interface
:   what usb device drivers talk to

**Definition**:

```
struct usb_interface {
    struct usb_host_interface *altsetting;
    struct usb_host_interface *cur_altsetting;
    unsigned num_altsetting;
    struct usb_interface_assoc_descriptor *intf_assoc;
    int minor;
    enum usb_interface_condition condition;
    unsigned sysfs_files_created:1;
    unsigned ep_devs_created:1;
    unsigned unregistering:1;
    unsigned needs_remote_wakeup:1;
    unsigned needs_altsetting0:1;
    unsigned needs_binding:1;
    unsigned resetting_device:1;
    unsigned authorized:1;
    enum usb_wireless_status wireless_status;
    struct work_struct wireless_status_work;
    struct device dev;
    struct device *usb_dev;
    struct work_struct reset_ws;
};
```

**Members**

`altsetting`
:   array of interface structures, one for each alternate
    setting that may be selected. Each one includes a set of
    endpoint configurations. They will be in no particular order.

`cur_altsetting`
:   the current altsetting.

`num_altsetting`
:   number of altsettings defined.

`intf_assoc`
:   interface association descriptor

`minor`
:   the minor number assigned to this interface, if this
    interface is bound to a driver that uses the USB major number.
    If this interface does not use the USB major, this field should
    be unused. The driver should set this value in the `probe()`
    function of the driver, after it has been assigned a minor
    number from the USB core by calling [`usb_register_dev()`](#c.usb_core.usb_register_dev "usb_core.usb_register_dev").

`condition`
:   binding state of the interface: not bound, binding
    (in `probe()`), bound to a driver, or unbinding (in `disconnect()`)

`sysfs_files_created`
:   sysfs attributes exist

`ep_devs_created`
:   endpoint child pseudo-devices exist

`unregistering`
:   flag set when the interface is being unregistered

`needs_remote_wakeup`
:   flag set when the driver requires remote-wakeup
    capability during autosuspend.

`needs_altsetting0`
:   flag set when a set-interface request for altsetting 0
    has been deferred.

`needs_binding`
:   flag set when the driver should be re-probed or unbound
    following a reset or suspend operation it doesn’t support.

`resetting_device`
:   USB core reset the device, so use alt setting 0 as
    current; needs bandwidth alloc after reset.

`authorized`
:   This allows to (de)authorize individual interfaces instead
    a whole device in contrast to the device authorization.

`wireless_status`
:   if the USB device uses a receiver/emitter combo, whether
    the emitter is connected.

`wireless_status_work`
:   Used for scheduling wireless status changes
    from atomic context.

`dev`
:   driver model’s view of this device

`usb_dev`
:   if an interface is bound to the USB major, this will point
    to the sysfs representation for that device.

`reset_ws`
:   Used for scheduling resets from atomic context.

**Description**

USB device drivers attach to interfaces on a physical device. Each
interface encapsulates a single high level function, such as feeding
an audio stream to a speaker or reporting a change in a volume control.
Many USB devices only have one interface. The protocol used to talk to
an interface’s endpoints can be defined in a usb “class” specification,
or by a product’s vendor. The (default) control endpoint is part of
every interface, but is never listed among the interface’s descriptors.

The driver that is bound to the interface can use standard driver model
calls such as `dev_get_drvdata()` on the dev member of this structure.

Each interface may have alternate settings. The initial configuration
of a device sets altsetting 0, but the device driver can change
that setting using [`usb_set_interface()`](#c.usb_core.usb_set_interface "usb_core.usb_set_interface"). Alternate settings are often
used to control the use of periodic endpoints, such as by having
different endpoints use different amounts of reserved USB bandwidth.
All standards-conformant USB devices that use isochronous endpoints
will use them in non-default settings.

The USB specification says that alternate setting numbers must run from
0 to one less than the total number of alternate settings. But some
devices manage to mess this up, and the structures aren’t necessarily
stored in numerical order anyhow. Use [`usb_altnum_to_altsetting()`](#c.usb_core.usb_altnum_to_altsetting "usb_core.usb_altnum_to_altsetting") to
look up an alternate setting in the altsetting array based on its number.

void usb\_set\_intfdata(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf, void \*data)
:   associate driver-specific data with an interface

**Parameters**

`struct usb_interface *intf`
:   USB interface

`void *data`
:   driver data

**Description**

Drivers can use this function in their `probe()` callbacks to associate
driver-specific data with an interface.

Note that there is generally no need to clear the driver-data pointer even
if some drivers do so for historical or implementation-specific reasons.

struct usb\_interface\_cache
:   long-term representation of a device interface

**Definition**:

```
struct usb_interface_cache {
    unsigned num_altsetting;
    struct kref ref;
    struct usb_host_interface altsetting[];
};
```

**Members**

`num_altsetting`
:   number of altsettings defined.

`ref`
:   reference counter.

`altsetting`
:   variable-length array of interface structures, one for
    each alternate setting that may be selected. Each one includes a
    set of endpoint configurations. They will be in no particular order.

**Description**

These structures persist for the lifetime of a usb\_device, unlike
`struct usb_interface` (which persists only as long as its configuration
is installed). The altsetting arrays can be accessed through these
structures at any time, permitting comparison of configurations and
providing support for the /sys/kernel/debug/usb/devices pseudo-file.

struct usb\_host\_config
:   representation of a device’s configuration

**Definition**:

```
struct usb_host_config {
    struct usb_config_descriptor    desc;
    char *string;
    struct usb_interface_assoc_descriptor *intf_assoc[USB_MAXIADS];
    struct usb_interface *interface[USB_MAXINTERFACES];
    struct usb_interface_cache *intf_cache[USB_MAXINTERFACES];
    unsigned char *extra;
    int extralen;
};
```

**Members**

`desc`
:   the device’s configuration descriptor.

`string`
:   pointer to the cached version of the iConfiguration string, if
    present for this configuration.

`intf_assoc`
:   list of any interface association descriptors in this config

`interface`
:   array of pointers to usb\_interface structures, one for each
    interface in the configuration. The number of interfaces is stored
    in desc.bNumInterfaces. These pointers are valid only while the
    configuration is active.

`intf_cache`
:   array of pointers to usb\_interface\_cache structures, one
    for each interface in the configuration. These structures exist
    for the entire life of the device.

`extra`
:   pointer to buffer containing all extra descriptors associated
    with this configuration (those preceding the first interface
    descriptor).

`extralen`
:   length of the extra descriptors buffer.

**Description**

USB devices may have multiple configurations, but only one can be active
at any time. Each encapsulates a different operational environment;
for example, a dual-speed device would have separate configurations for
full-speed and high-speed operation. The number of configurations
available is stored in the device descriptor as bNumConfigurations.

A configuration can contain multiple interfaces. Each corresponds to
a different function of the USB device, and all are available whenever
the configuration is active. The USB standard says that interfaces
are supposed to be numbered from 0 to desc.bNumInterfaces-1, but a lot
of devices get this wrong. In addition, the interface array is not
guaranteed to be sorted in numerical order. Use [`usb_ifnum_to_if()`](#c.usb_core.usb_ifnum_to_if "usb_core.usb_ifnum_to_if") to
look up an interface entry based on its number.

Device drivers should not attempt to activate configurations. The choice
of which configuration to install is a policy decision based on such
considerations as available power, functionality provided, and the user’s
desires (expressed through userspace tools). However, drivers can call
[`usb_reset_configuration()`](#c.usb_core.usb_reset_configuration "usb_core.usb_reset_configuration") to reinitialize the current configuration and
all its interfaces.

struct usb\_device
:   kernel’s representation of a USB device

**Definition**:

```
struct usb_device {
    int devnum;
    char devpath[16];
    u32 route;
    enum usb_device_state   state;
    enum usb_device_speed   speed;
    unsigned int            rx_lanes;
    unsigned int            tx_lanes;
    enum usb_ssp_rate       ssp_rate;
    struct usb_tt   *tt;
    int ttport;
    unsigned int toggle[2];
    struct usb_device *parent;
    struct usb_bus *bus;
    struct usb_host_endpoint ep0;
    struct device dev;
    struct usb_device_descriptor descriptor;
    struct usb_host_bos *bos;
    struct usb_host_config *config;
    struct usb_host_config *actconfig;
    struct usb_host_endpoint *ep_in[16];
    struct usb_host_endpoint *ep_out[16];
    char **rawdescriptors;
    unsigned short bus_mA;
    u8 portnum;
    u8 level;
    u8 devaddr;
    unsigned can_submit:1;
    unsigned persist_enabled:1;
    unsigned reset_in_progress:1;
    unsigned have_langid:1;
    unsigned authorized:1;
    unsigned authenticated:1;
    unsigned lpm_capable:1;
    unsigned lpm_devinit_allow:1;
    unsigned usb2_hw_lpm_capable:1;
    unsigned usb2_hw_lpm_besl_capable:1;
    unsigned usb2_hw_lpm_enabled:1;
    unsigned usb2_hw_lpm_allowed:1;
    unsigned usb3_lpm_u1_enabled:1;
    unsigned usb3_lpm_u2_enabled:1;
    int string_langid;
    char *product;
    char *manufacturer;
    char *serial;
    struct list_head filelist;
    int maxchild;
    u32 quirks;
    atomic_t urbnum;
    unsigned long active_duration;
    unsigned long connect_time;
    unsigned do_remote_wakeup:1;
    unsigned reset_resume:1;
    unsigned port_is_suspended:1;
    unsigned offload_pm_locked:1;
    int offload_usage;
    spinlock_t offload_lock;
    enum usb_link_tunnel_mode tunnel_mode;
    struct device_link *usb4_link;
    int slot_id;
    struct usb2_lpm_parameters l1_params;
    struct usb3_lpm_parameters u1_params;
    struct usb3_lpm_parameters u2_params;
    unsigned lpm_disable_count;
    u16 hub_delay;
    unsigned use_generic_driver:1;
};
```

**Members**

`devnum`
:   device number; address on a USB bus

`devpath`
:   device ID string for use in messages (e.g., /port/...)

`route`
:   tree topology hex string for use with xHCI

`state`
:   device state: configured, not attached, etc.

`speed`
:   device speed: high/full/low (or error)

`rx_lanes`
:   number of rx lanes in use, USB 3.2 adds dual-lane support

`tx_lanes`
:   number of tx lanes in use, USB 3.2 adds dual-lane support

`ssp_rate`
:   SuperSpeed Plus phy signaling rate and lane count

`tt`
:   Transaction Translator info; used with low/full speed dev, highspeed hub

`ttport`
:   device port on that tt hub

`toggle`
:   one bit for each endpoint, with ([0] = IN, [1] = OUT) endpoints

`parent`
:   our hub, unless we’re the root

`bus`
:   bus we’re part of

`ep0`
:   endpoint 0 data (default control pipe)

`dev`
:   generic device interface

`descriptor`
:   USB device descriptor

`bos`
:   USB device BOS descriptor set

`config`
:   all of the device’s configs

`actconfig`
:   the active configuration

`ep_in`
:   array of IN endpoints

`ep_out`
:   array of OUT endpoints

`rawdescriptors`
:   raw descriptors for each config

`bus_mA`
:   Current available from the bus

`portnum`
:   parent port number (origin 1)

`level`
:   number of USB hub ancestors

`devaddr`
:   device address, XHCI: assigned by HW, others: same as devnum

`can_submit`
:   URBs may be submitted

`persist_enabled`
:   USB\_PERSIST enabled for this device

`reset_in_progress`
:   the device is being reset

`have_langid`
:   whether string\_langid is valid

`authorized`
:   policy has said we can use it;
    (user space) policy determines if we authorize this device to be
    used or not. By default, wired USB devices are authorized.
    WUSB devices are not, until we authorize them from user space.
    FIXME -- complete doc

`authenticated`
:   Crypto authentication passed

`lpm_capable`
:   device supports LPM

`lpm_devinit_allow`
:   Allow USB3 device initiated LPM, exit latency is in range

`usb2_hw_lpm_capable`
:   device can perform USB2 hardware LPM

`usb2_hw_lpm_besl_capable`
:   device can perform USB2 hardware BESL LPM

`usb2_hw_lpm_enabled`
:   USB2 hardware LPM is enabled

`usb2_hw_lpm_allowed`
:   Userspace allows USB 2.0 LPM to be enabled

`usb3_lpm_u1_enabled`
:   USB3 hardware U1 LPM enabled

`usb3_lpm_u2_enabled`
:   USB3 hardware U2 LPM enabled

`string_langid`
:   language ID for strings

`product`
:   iProduct string, if present (static)

`manufacturer`
:   iManufacturer string, if present (static)

`serial`
:   iSerialNumber string, if present (static)

`filelist`
:   usbfs files that are open to this device

`maxchild`
:   number of ports if hub

`quirks`
:   quirks of the whole device

`urbnum`
:   number of URBs submitted for the whole device

`active_duration`
:   total time device is not suspended

`connect_time`
:   time device was first connected

`do_remote_wakeup`
:   remote wakeup should be enabled

`reset_resume`
:   needs reset instead of resume

`port_is_suspended`
:   the upstream port is suspended (L2 or U3)

`offload_pm_locked`
:   prevents offload\_usage changes during PM transitions.

`offload_usage`
:   number of offload activities happening on this usb device.

`offload_lock`
:   protects offload\_usage and offload\_pm\_locked

`tunnel_mode`
:   Connection native or tunneled over USB4

`usb4_link`
:   device link to the USB4 host interface

`slot_id`
:   Slot ID assigned by xHCI

`l1_params`
:   best effor service latency for USB2 L1 LPM state, and L1 timeout.

`u1_params`
:   exit latencies for USB3 U1 LPM state, and hub-initiated timeout.

`u2_params`
:   exit latencies for USB3 U2 LPM state, and hub-initiated timeout.

`lpm_disable_count`
:   Ref count used by `usb_disable_lpm()` and `usb_enable_lpm()`
    to keep track of the number of functions that require USB 3.0 Link Power
    Management to be disabled for this usb\_device. This count should only
    be manipulated by those functions, with the bandwidth\_mutex is held.

`hub_delay`
:   cached value consisting of:
    parent->hub\_delay + wHubDelay + tTPTransmissionDelay (40ns)
    Will be used as wValue for SetIsochDelay requests.

`use_generic_driver`
:   ask driver core to reprobe using the generic driver.

**Notes**

Usbcore drivers should not set usbdev->state directly. Instead use
[`usb_set_device_state()`](#c.usb_core.usb_set_device_state "usb_core.usb_set_device_state").

usb\_hub\_for\_each\_child

`usb_hub_for_each_child (hdev, port1, child)`

> iterate over all child devices on the hub

**Parameters**

`hdev`
:   USB device belonging to the usb hub

`port1`
:   portnum associated with child device

`child`
:   child device pointer

int usb\_interface\_claimed(struct [usb\_interface](#c.usb_interface "usb_interface") \*iface)
:   returns true iff an interface is claimed

**Parameters**

`struct usb_interface *iface`
:   the interface being checked

**Return**

`true` (nonzero) iff the interface is claimed, else `false`
(zero).

**Note**

Callers must own the driver model’s usb bus readlock. So driver
`probe()` entries don’t need extra locking, but other call contexts
may need to explicitly claim that lock.

int usb\_make\_path(struct [usb\_device](#c.usb_device "usb_device") \*dev, char \*buf, size\_t size)
:   returns stable device path in the usb tree

**Parameters**

`struct usb_device *dev`
:   the device whose path is being constructed

`char *buf`
:   where to put the string

`size_t size`
:   how big is “buf”?

**Return**

Length of the string (> 0) or negative if size was too small.

**Note**

This identifier is intended to be “stable”, reflecting physical paths in
hardware such as physical bus addresses for host controllers or ports on
USB hubs. That makes it stay the same until systems are physically
reconfigured, by re-cabling a tree of USB devices or by moving USB host
controllers. Adding and removing devices, including virtual root hubs
in host controller driver modules, does not change these path identifiers;
neither does rebooting or re-enumerating. These are more useful identifiers
than changeable (“unstable”) ones like bus numbers or device addresses.

With a partial exception for devices connected to USB 2.0 root hubs, these
identifiers are also predictable. So long as the device tree isn’t changed,
plugging any USB device into a given hub port always gives it the same path.
Because of the use of “companion” controllers, devices connected to ports on
USB 2.0 root hubs (EHCI host controllers) will get one path ID if they are
high speed, and a different one if they are full or low speed.

USB\_DEVICE

`USB_DEVICE (vend, prod)`

> macro used to describe a specific usb device

**Parameters**

`vend`
:   the 16 bit USB Vendor ID

`prod`
:   the 16 bit USB Product ID

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific device.

USB\_DEVICE\_VER

`USB_DEVICE_VER (vend, prod, lo, hi)`

> describe a specific usb device with a version range

**Parameters**

`vend`
:   the 16 bit USB Vendor ID

`prod`
:   the 16 bit USB Product ID

`lo`
:   the bcdDevice\_lo value

`hi`
:   the bcdDevice\_hi value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific device, with a version range.

USB\_DEVICE\_INTERFACE\_CLASS

`USB_DEVICE_INTERFACE_CLASS (vend, prod, cl)`

> describe a usb device with a specific interface class

**Parameters**

`vend`
:   the 16 bit USB Vendor ID

`prod`
:   the 16 bit USB Product ID

`cl`
:   bInterfaceClass value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific interface class of devices.

USB\_DEVICE\_INTERFACE\_PROTOCOL

`USB_DEVICE_INTERFACE_PROTOCOL (vend, prod, pr)`

> describe a usb device with a specific interface protocol

**Parameters**

`vend`
:   the 16 bit USB Vendor ID

`prod`
:   the 16 bit USB Product ID

`pr`
:   bInterfaceProtocol value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific interface protocol of devices.

USB\_DEVICE\_INTERFACE\_NUMBER

`USB_DEVICE_INTERFACE_NUMBER (vend, prod, num)`

> describe a usb device with a specific interface number

**Parameters**

`vend`
:   the 16 bit USB Vendor ID

`prod`
:   the 16 bit USB Product ID

`num`
:   bInterfaceNumber value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific interface number of devices.

USB\_DEVICE\_INFO

`USB_DEVICE_INFO (cl, sc, pr)`

> macro used to describe a class of usb devices

**Parameters**

`cl`
:   bDeviceClass value

`sc`
:   bDeviceSubClass value

`pr`
:   bDeviceProtocol value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific class of devices.

USB\_INTERFACE\_INFO

`USB_INTERFACE_INFO (cl, sc, pr)`

> macro used to describe a class of usb interfaces

**Parameters**

`cl`
:   bInterfaceClass value

`sc`
:   bInterfaceSubClass value

`pr`
:   bInterfaceProtocol value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific class of interfaces.

USB\_DEVICE\_AND\_INTERFACE\_INFO

`USB_DEVICE_AND_INTERFACE_INFO (vend, prod, cl, sc, pr)`

> describe a specific usb device with a class of usb interfaces

**Parameters**

`vend`
:   the 16 bit USB Vendor ID

`prod`
:   the 16 bit USB Product ID

`cl`
:   bInterfaceClass value

`sc`
:   bInterfaceSubClass value

`pr`
:   bInterfaceProtocol value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific device with a specific class of interfaces.

This is especially useful when explicitly matching devices that have
vendor specific bDeviceClass values, but standards-compliant interfaces.

USB\_VENDOR\_AND\_INTERFACE\_INFO

`USB_VENDOR_AND_INTERFACE_INFO (vend, cl, sc, pr)`

> describe a specific usb vendor with a class of usb interfaces

**Parameters**

`vend`
:   the 16 bit USB Vendor ID

`cl`
:   bInterfaceClass value

`sc`
:   bInterfaceSubClass value

`pr`
:   bInterfaceProtocol value

**Description**

This macro is used to create a `struct usb_device_id` that matches a
specific vendor with a specific class of interfaces.

This is especially useful when explicitly matching devices that have
vendor specific bDeviceClass values, but standards-compliant interfaces.

struct usb\_driver
:   identifies USB interface driver to usbcore

**Definition**:

```
struct usb_driver {
    const char *name;
    int (*probe) (struct usb_interface *intf, const struct usb_device_id *id);
    void (*disconnect) (struct usb_interface *intf);
    int (*unlocked_ioctl) (struct usb_interface *intf, unsigned int code, void *buf);
    int (*suspend) (struct usb_interface *intf, pm_message_t message);
    int (*resume) (struct usb_interface *intf);
    int (*reset_resume)(struct usb_interface *intf);
    int (*pre_reset)(struct usb_interface *intf);
    int (*post_reset)(struct usb_interface *intf);
    void (*shutdown)(struct usb_interface *intf);
    const struct usb_device_id *id_table;
    const struct attribute_group **dev_groups;
    struct usb_dynids dynids;
    struct device_driver driver;
    unsigned int no_dynamic_id:1;
    unsigned int supports_autosuspend:1;
    unsigned int disable_hub_initiated_lpm:1;
    unsigned int soft_unbind:1;
};
```

**Members**

`name`
:   The driver name should be unique among USB drivers,
    and should normally be the same as the module name.

`probe`
:   Called to see if the driver is willing to manage a particular
    interface on a device. If it is, probe returns zero and uses
    `usb_set_intfdata()` to associate driver-specific data with the
    interface. It may also use [`usb_set_interface()`](#c.usb_core.usb_set_interface "usb_core.usb_set_interface") to specify the
    appropriate altsetting. If unwilling to manage the interface,
    return -ENODEV, if genuine IO errors occurred, an appropriate
    negative errno value.

`disconnect`
:   Called when the interface is no longer accessible, usually
    because its device has been (or is being) disconnected or the
    driver module is being unloaded.

`unlocked_ioctl`
:   Used for drivers that want to talk to userspace through
    the “usbfs” filesystem. This lets devices provide ways to
    expose information to user space regardless of where they
    do (or don’t) show up otherwise in the filesystem.

`suspend`
:   Called when the device is going to be suspended by the
    system either from system sleep or runtime suspend context. The
    return value will be ignored in system sleep context, so do NOT
    try to continue using the device if suspend fails in this case.
    Instead, let the resume or reset-resume routine recover from
    the failure.

`resume`
:   Called when the device is being resumed by the system.

`reset_resume`
:   Called when the suspended device has been reset instead
    of being resumed.

`pre_reset`
:   Called by [`usb_reset_device()`](#c.usb_core.usb_reset_device "usb_core.usb_reset_device") when the device is about to be
    reset. This routine must not return until the driver has no active
    URBs for the device, and no more URBs may be submitted until the
    post\_reset method is called.

`post_reset`
:   Called by [`usb_reset_device()`](#c.usb_core.usb_reset_device "usb_core.usb_reset_device") after the device
    has been reset

`shutdown`
:   Called at shut-down time to quiesce the device.

`id_table`
:   USB drivers use ID table to support hotplugging.
    Export this with MODULE\_DEVICE\_TABLE(usb,...). This must be set
    or your driver’s probe function will never get called.

`dev_groups`
:   Attributes attached to the device that will be created once it
    is bound to the driver.

`dynids`
:   used internally to hold the list of dynamically added device
    ids for this driver.

`driver`
:   The driver-model core driver structure.

`no_dynamic_id`
:   if set to 1, the USB core will not allow dynamic ids to be
    added to this driver by preventing the sysfs file from being created.

`supports_autosuspend`
:   if set to 0, the USB core will not allow autosuspend
    for interfaces bound to this driver.

`disable_hub_initiated_lpm`
:   if set to 1, the USB core will not allow hubs
    to initiate lower power link state transitions when an idle timeout
    occurs. Device-initiated USB 3.0 link PM will still be allowed.

`soft_unbind`
:   if set to 1, the USB core will not kill URBs and disable
    endpoints before calling the driver’s disconnect method.

**Description**

USB interface drivers must provide a name, `probe()` and `disconnect()`
methods, and an id\_table. Other driver fields are optional.

The id\_table is used in hotplugging. It holds a set of descriptors,
and specialized data may be associated with each entry. That table
is used by both user and kernel mode hotplugging support.

The `probe()` and `disconnect()` methods are called in a context where
they can sleep, but they should avoid abusing the privilege. Most
work to connect to a device should be done when the device is opened,
and undone at the last close. The disconnect code needs to address
concurrency issues with respect to `open()` and `close()` methods, as
well as forcing all pending I/O requests to complete (by unlinking
them as necessary, and blocking until the unlinks complete).

struct usb\_device\_driver
:   identifies USB device driver to usbcore

**Definition**:

```
struct usb_device_driver {
    const char *name;
    bool (*match) (struct usb_device *udev);
    int (*probe) (struct usb_device *udev);
    void (*disconnect) (struct usb_device *udev);
    int (*suspend) (struct usb_device *udev, pm_message_t message);
    int (*resume) (struct usb_device *udev, pm_message_t message);
    int (*choose_configuration) (struct usb_device *udev);
    const struct attribute_group **dev_groups;
    struct device_driver driver;
    const struct usb_device_id *id_table;
    unsigned int supports_autosuspend:1;
    unsigned int generic_subclass:1;
};
```

**Members**

`name`
:   The driver name should be unique among USB drivers,
    and should normally be the same as the module name.

`match`
:   If set, used for better device/driver matching.

`probe`
:   Called to see if the driver is willing to manage a particular
    device. If it is, probe returns zero and uses `dev_set_drvdata()`
    to associate driver-specific data with the device. If unwilling
    to manage the device, return a negative errno value.

`disconnect`
:   Called when the device is no longer accessible, usually
    because it has been (or is being) disconnected or the driver’s
    module is being unloaded.

`suspend`
:   Called when the device is going to be suspended by the system.

`resume`
:   Called when the device is being resumed by the system.

`choose_configuration`
:   If non-NULL, called instead of the default
    `usb_choose_configuration()`. If this returns an error then we’ll go
    on to call the normal `usb_choose_configuration()`.

`dev_groups`
:   Attributes attached to the device that will be created once it
    is bound to the driver.

`driver`
:   The driver-model core driver structure.

`id_table`
:   used with **`match()`** to select better matching driver at
    `probe()` time.

`supports_autosuspend`
:   if set to 0, the USB core will not allow autosuspend
    for devices bound to this driver.

`generic_subclass`
:   if set to 1, the generic USB driver’s probe, disconnect,
    resume and suspend functions will be called in addition to the driver’s
    own, so this part of the setup does not need to be replicated.

**Description**

USB device drivers must provide a name, other driver fields are optional.

struct usb\_class\_driver
:   identifies a USB driver that wants to use the USB major number

**Definition**:

```
struct usb_class_driver {
    char *name;
    char *(*devnode)(const struct device *dev, umode_t *mode);
    const struct file_operations *fops;
    int minor_base;
};
```

**Members**

`name`
:   the usb class device name for this driver. Will show up in sysfs.

`devnode`
:   Callback to provide a naming hint for a possible
    device node to create.

`fops`
:   pointer to the `struct file_operations` of this driver.

`minor_base`
:   the start of the minor range for this driver.

**Description**

This structure is used for the [`usb_register_dev()`](#c.usb_core.usb_register_dev "usb_core.usb_register_dev") and
[`usb_deregister_dev()`](#c.usb_core.usb_deregister_dev "usb_core.usb_deregister_dev") functions, to consolidate a number of the
parameters used for them.

module\_usb\_driver

`module_usb_driver (__usb_driver)`

> Helper macro for registering a USB driver

**Parameters**

`__usb_driver`
:   usb\_driver struct

**Description**

Helper macro for USB drivers which do not do anything special in module
init/exit. This eliminates a lot of boilerplate. Each module may only
use this macro once, and calling it replaces `module_init()` and `module_exit()`

struct urb
:   USB Request Block

**Definition**:

```
struct urb {
    struct list_head urb_list;
    struct list_head anchor_list;
    struct usb_anchor *anchor;
    struct usb_device *dev;
    struct usb_host_endpoint *ep;
    unsigned int pipe;
    unsigned int stream_id;
    int status;
    unsigned int transfer_flags;
    void *transfer_buffer;
    dma_addr_t transfer_dma;
    struct scatterlist *sg;
    struct sg_table *sgt;
    int num_mapped_sgs;
    int num_sgs;
    u32 transfer_buffer_length;
    u32 actual_length;
    unsigned char *setup_packet;
    dma_addr_t setup_dma;
    int start_frame;
    int number_of_packets;
    int interval;
    int error_count;
    void *context;
    usb_complete_t complete;
    struct usb_iso_packet_descriptor iso_frame_desc[];
};
```

**Members**

`urb_list`
:   For use by current owner of the URB.

`anchor_list`
:   membership in the list of an anchor

`anchor`
:   to anchor URBs to a common mooring

`dev`
:   Identifies the USB device to perform the request.

`ep`
:   Points to the endpoint’s data structure. Will eventually
    replace **pipe**.

`pipe`
:   Holds endpoint number, direction, type, and more.
    Create these values with the eight macros available;
    usb\_{snd,rcv}TYPEpipe(dev,endpoint), where the TYPE is “ctrl”
    (control), “bulk”, “int” (interrupt), or “iso” (isochronous).
    For example `usb_sndbulkpipe()` or `usb_rcvintpipe()`. Endpoint
    numbers range from zero to fifteen. Note that “in” endpoint two
    is a different endpoint (and pipe) from “out” endpoint two.
    The current configuration controls the existence, type, and
    maximum packet size of any given endpoint.

`stream_id`
:   the endpoint’s stream ID for bulk streams

`status`
:   This is read in non-iso completion functions to get the
    status of the particular request. ISO requests only use it
    to tell whether the URB was unlinked; detailed status for
    each frame is in the fields of the iso\_frame-desc.

`transfer_flags`
:   A variety of flags may be used to affect how URB
    submission, unlinking, or operation are handled. Different
    kinds of URB can use different flags.

`transfer_buffer`
:   This identifies the buffer to (or from) which the I/O
    request will be performed unless URB\_NO\_TRANSFER\_DMA\_MAP is set
    (however, do not leave garbage in transfer\_buffer even then).
    This buffer must be suitable for DMA; allocate it with
    `kmalloc()` or equivalent. For transfers to “in” endpoints, contents
    of this buffer will be modified. This buffer is used for the data
    stage of control transfers.

`transfer_dma`
:   When transfer\_flags includes URB\_NO\_TRANSFER\_DMA\_MAP,
    the device driver is saying that it provided this DMA address,
    which the host controller driver should use in preference to the
    transfer\_buffer.

`sg`
:   scatter gather buffer list, the buffer size of each element in
    the list (except the last) must be divisible by the endpoint’s
    max packet size if no\_sg\_constraint isn’t set in ‘`struct usb_bus`’

`sgt`
:   used to hold a scatter gather table returned by [`usb_alloc_noncoherent()`](#c.usb_core.usb_alloc_noncoherent "usb_core.usb_alloc_noncoherent"),
    which describes the allocated non-coherent and possibly non-contiguous
    memory and is guaranteed to have 1 single DMA mapped segment. The
    allocated memory needs to be freed by [`usb_free_noncoherent()`](#c.usb_core.usb_free_noncoherent "usb_core.usb_free_noncoherent").

`num_mapped_sgs`
:   (internal) number of mapped sg entries

`num_sgs`
:   number of entries in the sg list

`transfer_buffer_length`
:   How big is transfer\_buffer. The transfer may
    be broken up into chunks according to the current maximum packet
    size for the endpoint, which is a function of the configuration
    and is encoded in the pipe. When the length is zero, neither
    transfer\_buffer nor transfer\_dma is used.

`actual_length`
:   This is read in non-iso completion functions, and
    it tells how many bytes (out of transfer\_buffer\_length) were
    transferred. It will normally be the same as requested, unless
    either an error was reported or a short read was performed.
    The URB\_SHORT\_NOT\_OK transfer flag may be used to make such
    short reads be reported as errors.

`setup_packet`
:   Only used for control transfers, this points to eight bytes
    of setup data. Control transfers always start by sending this data
    to the device. Then transfer\_buffer is read or written, if needed.

`setup_dma`
:   DMA pointer for the setup packet. The caller must not use
    this field; setup\_packet must point to a valid buffer.

`start_frame`
:   Returns the initial frame for isochronous transfers.

`number_of_packets`
:   Lists the number of ISO transfer buffers.

`interval`
:   Specifies the polling interval for interrupt or isochronous
    transfers. The units are frames (milliseconds) for full and low
    speed devices, and microframes (1/8 millisecond) for highspeed
    and SuperSpeed devices.

`error_count`
:   Returns the number of ISO transfers that reported errors.

`context`
:   For use in completion functions. This normally points to
    request-specific driver context.

`complete`
:   Completion handler. This URB is passed as the parameter to the
    completion function. The completion function may then do what
    it likes with the URB, including resubmitting or freeing it.

`iso_frame_desc`
:   Used to provide arrays of ISO transfer buffers and to
    collect the transfer status for each buffer.

**Description**

This structure identifies USB transfer requests. URBs must be allocated by
calling `usb_alloc_urb()` and freed with a call to `usb_free_urb()`.
Initialization may be done using various usb\_fill\_\*`_urb()` functions. URBs
are submitted using `usb_submit_urb()`, and pending requests may be canceled
using `usb_unlink_urb()` or `usb_kill_urb()`.

Data Transfer Buffers:

Normally drivers provide I/O buffers allocated with `kmalloc()` or otherwise
taken from the general page pool. That is provided by transfer\_buffer
(control requests also use setup\_packet), and host controller drivers
perform a dma mapping (and unmapping) for each buffer transferred. Those
mapping operations can be expensive on some platforms (perhaps using a dma
bounce buffer or talking to an IOMMU),
although they’re cheap on commodity x86 and ppc hardware.

Alternatively, drivers may pass the URB\_NO\_TRANSFER\_DMA\_MAP transfer flag,
which tells the host controller driver that no such mapping is needed for
the transfer\_buffer since
the device driver is DMA-aware. For example, a device driver might
allocate a DMA buffer with [`usb_alloc_coherent()`](#c.usb_core.usb_alloc_coherent "usb_core.usb_alloc_coherent") or call `usb_buffer_map()`.
When this transfer flag is provided, host controller drivers will
attempt to use the dma address found in the transfer\_dma
field rather than determining a dma address themselves.

Note that transfer\_buffer must still be set if the controller
does not support DMA (as indicated by `hcd_uses_dma()`) and when talking
to root hub. If you have to transfer between highmem zone and the device
on such controller, create a bounce buffer or bail out with an error.
If transfer\_buffer cannot be set (is in highmem) and the controller is DMA
capable, assign NULL to it, so that usbmon knows not to use the value.
The setup\_packet must always be set, so it cannot be located in highmem.

Initialization:

All URBs submitted must initialize the dev, pipe, transfer\_flags (may be
zero), and complete fields. All URBs must also initialize
transfer\_buffer and transfer\_buffer\_length. They may provide the
URB\_SHORT\_NOT\_OK transfer flag, indicating that short reads are
to be treated as errors; that flag is invalid for write requests.

Bulk URBs may
use the URB\_ZERO\_PACKET transfer flag, indicating that bulk OUT transfers
should always terminate with a short packet, even if it means adding an
extra zero length packet.

Control URBs must provide a valid pointer in the setup\_packet field.
Unlike the transfer\_buffer, the setup\_packet may not be mapped for DMA
beforehand.

Interrupt URBs must provide an interval, saying how often (in milliseconds
or, for highspeed devices, 125 microsecond units)
to poll for transfers. After the URB has been submitted, the interval
field reflects how the transfer was actually scheduled.
The polling interval may be more frequent than requested.
For example, some controllers have a maximum interval of 32 milliseconds,
while others support intervals of up to 1024 milliseconds.
Isochronous URBs also have transfer intervals. (Note that for isochronous
endpoints, as well as high speed interrupt endpoints, the encoding of
the transfer interval in the endpoint descriptor is logarithmic.
Device drivers must convert that value to linear units themselves.)

If an isochronous endpoint queue isn’t already running, the host
controller will schedule a new URB to start as soon as bandwidth
utilization allows. If the queue is running then a new URB will be
scheduled to start in the first transfer slot following the end of the
preceding URB, if that slot has not already expired. If the slot has
expired (which can happen when IRQ delivery is delayed for a long time),
the scheduling behavior depends on the URB\_ISO\_ASAP flag. If the flag
is clear then the URB will be scheduled to start in the expired slot,
implying that some of its packets will not be transferred; if the flag
is set then the URB will be scheduled in the first unexpired slot,
breaking the queue’s synchronization. Upon URB completion, the
start\_frame field will be set to the (micro)frame number in which the
transfer was scheduled. Ranges for frame counter values are HC-specific
and can go from as low as 256 to as high as 65536 frames.

Isochronous URBs have a different data transfer model, in part because
the quality of service is only “best effort”. Callers provide specially
allocated URBs, with number\_of\_packets worth of iso\_frame\_desc structures
at the end. Each such packet is an individual ISO transfer. Isochronous
URBs are normally queued, submitted by drivers to arrange that
transfers are at least double buffered, and then explicitly resubmitted
in completion handlers, so
that data (such as audio or video) streams at as constant a rate as the
host controller scheduler can support.

Completion Callbacks:

The completion callback is made `in_interrupt()`, and one of the first
things that a completion handler should do is check the status field.
The status field is provided for all URBs. It is used to report
unlinked URBs, and status for all non-ISO transfers. It should not
be examined before the URB is returned to the completion handler.

The context field is normally used to link URBs back to the relevant
driver or request state.

When the completion callback is invoked for non-isochronous URBs, the
actual\_length field tells how many bytes were transferred. This field
is updated even when the URB terminated with an error or was unlinked.

ISO transfer status is reported in the status and actual\_length fields
of the iso\_frame\_desc array, and the number of errors is reported in
error\_count. Completion callbacks for ISO transfers will normally
(re)submit URBs to ensure a constant transfer rate.

Note that even fields marked “public” should not be touched by the driver
when the urb is owned by the hcd, that is, since the call to
`usb_submit_urb()` till the entry into the completion routine.

void usb\_fill\_control\_urb(struct [urb](#c.usb_fill_control_urb "urb") \*urb, struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned int pipe, unsigned char \*setup\_packet, void \*transfer\_buffer, int buffer\_length, usb\_complete\_t complete\_fn, void \*context)
:   initializes a control urb

**Parameters**

`struct urb *urb`
:   pointer to the urb to initialize.

`struct usb_device *dev`
:   pointer to the `struct usb_device` for this urb.

`unsigned int pipe`
:   the endpoint pipe

`unsigned char *setup_packet`
:   pointer to the setup\_packet buffer. The buffer must be
    suitable for DMA.

`void *transfer_buffer`
:   pointer to the transfer buffer. The buffer must be
    suitable for DMA.

`int buffer_length`
:   length of the transfer buffer

`usb_complete_t complete_fn`
:   pointer to the usb\_complete\_t function

`void *context`
:   what to set the urb context to.

**Description**

Initializes a control urb with the proper information needed to submit
it to a device.

The transfer buffer and the setup\_packet buffer will most likely be filled
or read via DMA. The simplest way to get a buffer that can be DMAed to is
allocating it via `kmalloc()` or equivalent, even for very small buffers.
If the buffers are embedded in a bigger structure, there is a risk that
the buffer itself, the previous fields and/or the next fields are corrupted
due to cache incoherencies; or slowed down if they are evicted from the
cache. For more information, check [`struct urb`](#c.urb "urb").

void usb\_fill\_bulk\_urb(struct [urb](#c.usb_fill_bulk_urb "urb") \*urb, struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned int pipe, void \*transfer\_buffer, int buffer\_length, usb\_complete\_t complete\_fn, void \*context)
:   macro to help initialize a bulk urb

**Parameters**

`struct urb *urb`
:   pointer to the urb to initialize.

`struct usb_device *dev`
:   pointer to the `struct usb_device` for this urb.

`unsigned int pipe`
:   the endpoint pipe

`void *transfer_buffer`
:   pointer to the transfer buffer. The buffer must be
    suitable for DMA.

`int buffer_length`
:   length of the transfer buffer

`usb_complete_t complete_fn`
:   pointer to the usb\_complete\_t function

`void *context`
:   what to set the urb context to.

**Description**

Initializes a bulk urb with the proper information needed to submit it
to a device.

Refer to `usb_fill_control_urb()` for a description of the requirements for
transfer\_buffer.

void usb\_fill\_int\_urb(struct [urb](#c.usb_fill_int_urb "urb") \*urb, struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned int pipe, void \*transfer\_buffer, int buffer\_length, usb\_complete\_t complete\_fn, void \*context, int interval)
:   macro to help initialize a interrupt urb

**Parameters**

`struct urb *urb`
:   pointer to the urb to initialize.

`struct usb_device *dev`
:   pointer to the `struct usb_device` for this urb.

`unsigned int pipe`
:   the endpoint pipe

`void *transfer_buffer`
:   pointer to the transfer buffer. The buffer must be
    suitable for DMA.

`int buffer_length`
:   length of the transfer buffer

`usb_complete_t complete_fn`
:   pointer to the usb\_complete\_t function

`void *context`
:   what to set the urb context to.

`int interval`
:   what to set the urb interval to, encoded like
    the endpoint descriptor’s bInterval value.

**Description**

Initializes a interrupt urb with the proper information needed to submit
it to a device.

Refer to `usb_fill_control_urb()` for a description of the requirements for
transfer\_buffer.

Note that High Speed and SuperSpeed(+) interrupt endpoints use a logarithmic
encoding of the endpoint interval, and express polling intervals in
microframes (eight per millisecond) rather than in frames (one per
millisecond).

int usb\_urb\_dir\_in(struct [urb](#c.usb_urb_dir_in "urb") \*urb)
:   check if an URB describes an IN transfer

**Parameters**

`struct urb *urb`
:   URB to be checked

**Return**

1 if **urb** describes an IN transfer (device-to-host),
otherwise 0.

int usb\_urb\_dir\_out(struct [urb](#c.usb_urb_dir_out "urb") \*urb)
:   check if an URB describes an OUT transfer

**Parameters**

`struct urb *urb`
:   URB to be checked

**Return**

1 if **urb** describes an OUT transfer (host-to-device),
otherwise 0.

struct usb\_sg\_request
:   support for scatter/gather I/O

**Definition**:

```
struct usb_sg_request {
    int status;
    size_t bytes;
};
```

**Members**

`status`
:   zero indicates success, else negative errno

`bytes`
:   counts bytes transferred.

**Description**

These requests are initialized using [`usb_sg_init()`](#c.usb_core.usb_sg_init "usb_core.usb_sg_init"), and then are used
as request handles passed to [`usb_sg_wait()`](#c.usb_core.usb_sg_wait "usb_core.usb_sg_wait") or [`usb_sg_cancel()`](#c.usb_core.usb_sg_cancel "usb_core.usb_sg_cancel"). Most
members of the request object aren’t for driver access.

The status and bytecount values are valid only after [`usb_sg_wait()`](#c.usb_core.usb_sg_wait "usb_core.usb_sg_wait")
returns. If the status is zero, then the bytecount matches the total
from the request.

After an error completion, drivers may need to clear a halt condition
on the endpoint.

## USB Core APIs

There are two basic I/O models in the USB API. The most elemental one is
asynchronous: drivers submit requests in the form of an URB, and the
URB’s completion callback handles the next step. All USB transfer types
support that model, although there are special cases for control URBs
(which always have setup and status stages, but may not have a data
stage) and isochronous URBs (which allow large packets and include
per-packet fault reports). Built on top of that is synchronous API
support, where a driver calls a routine that allocates one or more URBs,
submits them, and waits until they complete. There are synchronous
wrappers for single-buffer control and bulk transfers (which are awkward
to use in some driver disconnect scenarios), and for scatterlist based
streaming i/o (bulk or interrupt).

USB drivers need to provide buffers that can be used for DMA, although
they don’t necessarily need to provide the DMA mapping themselves. There
are APIs to use used when allocating DMA buffers, which can prevent use
of bounce buffers on some systems. In some cases, drivers may be able to
rely on 64bit DMA to eliminate another kind of bounce buffer.

void usb\_init\_urb(struct [urb](#c.usb_init_urb "urb") \*urb)
:   initializes a urb so that it can be used by a USB driver

**Parameters**

`struct urb *urb`
:   pointer to the urb to initialize

**Description**

Initializes a urb so that the USB subsystem can use it properly.

If a urb is created with a call to `usb_alloc_urb()` it is not
necessary to call this function. Only use this if you allocate the
space for a `struct urb` on your own. If you call this function, be
careful when freeing the memory for your urb that it is no longer in
use by the USB core.

Only use this function if you \_really\_ understand what you are doing.

struct [urb](#c.urb "urb") \*usb\_alloc\_urb(int iso\_packets, gfp\_t mem\_flags)
:   creates a new urb for a USB driver to use

**Parameters**

`int iso_packets`
:   number of iso packets for this urb

`gfp_t mem_flags`
:   the type of memory to allocate, see `kmalloc()` for a list of
    valid options for this.

**Description**

Creates an urb for the USB driver to use, initializes a few internal
structures, increments the usage counter, and returns a pointer to it.

If the driver want to use this urb for interrupt, control, or bulk
endpoints, pass ‘0’ as the number of iso packets.

The driver must call `usb_free_urb()` when it is finished with the urb.

**Return**

A pointer to the new urb, or `NULL` if no memory is available.

void usb\_free\_urb(struct [urb](#c.usb_free_urb "urb") \*urb)
:   frees the memory used by a urb when all users of it are finished

**Parameters**

`struct urb *urb`
:   pointer to the urb to free, may be NULL

**Description**

Must be called when a user of a urb is finished with it. When the last user
of the urb calls this function, the memory of the urb is freed.

**Note**

The transfer buffer associated with the urb is not freed unless the
URB\_FREE\_BUFFER transfer flag is set.

struct [urb](#c.usb_get_urb "urb") \*usb\_get\_urb(struct [urb](#c.usb_get_urb "urb") \*urb)
:   increments the reference count of the urb

**Parameters**

`struct urb *urb`
:   pointer to the urb to modify, may be NULL

**Description**

This must be called whenever a urb is transferred from a device driver to a
host controller driver. This allows proper reference counting to happen
for urbs.

**Return**

A pointer to the urb with the incremented reference counter.

void usb\_anchor\_urb(struct [urb](#c.usb_anchor_urb "urb") \*urb, struct usb\_anchor \*anchor)
:   anchors an URB while it is processed

**Parameters**

`struct urb *urb`
:   pointer to the urb to anchor

`struct usb_anchor *anchor`
:   pointer to the anchor

**Description**

This can be called to have access to URBs which are to be executed
without bothering to track them

void usb\_unanchor\_urb(struct [urb](#c.usb_unanchor_urb "urb") \*urb)
:   unanchors an URB

**Parameters**

`struct urb *urb`
:   pointer to the urb to anchor

**Description**

Call this to stop the system keeping track of this URB

int usb\_pipe\_type\_check(struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned int pipe)
:   sanity check of a specific pipe for a usb device

**Parameters**

`struct usb_device *dev`
:   `struct usb_device` to be checked

`unsigned int pipe`
:   pipe to check

**Description**

This performs a light-weight sanity check for the endpoint in the
given usb device. It returns 0 if the pipe is valid for the specific usb
device, otherwise a negative error code.

int usb\_urb\_ep\_type\_check(const struct [urb](#c.usb_urb_ep_type_check "urb") \*urb)
:   sanity check of endpoint in the given urb

**Parameters**

`const struct urb *urb`
:   urb to be checked

**Description**

This performs a light-weight sanity check for the endpoint in the
given urb. It returns 0 if the urb contains a valid endpoint, otherwise
a negative error code.

int usb\_submit\_urb(struct [urb](#c.usb_submit_urb "urb") \*urb, gfp\_t mem\_flags)
:   issue an asynchronous transfer request for an endpoint

**Parameters**

`struct urb *urb`
:   pointer to the urb describing the request

`gfp_t mem_flags`
:   the type of memory to allocate, see `kmalloc()` for a list
    of valid options for this.

**Description**

This submits a transfer request, and transfers control of the URB
describing that request to the USB subsystem. Request completion will
be indicated later, asynchronously, by calling the completion handler.
The three types of completion are success, error, and unlink
(a software-induced fault, also called “request cancellation”).

URBs may be submitted in interrupt context.

The caller must have correctly initialized the URB before submitting
it. Functions such as `usb_fill_bulk_urb()` and `usb_fill_control_urb()` are
available to ensure that most fields are correctly initialized, for
the particular kind of transfer, although they will not initialize
any transfer flags.

If the submission is successful, the `complete()` callback from the URB
will be called exactly once, when the USB core and Host Controller Driver
(HCD) are finished with the URB. When the completion function is called,
control of the URB is returned to the device driver which issued the
request. The completion handler may then immediately free or reuse that
URB.

With few exceptions, USB device drivers should never access URB fields
provided by usbcore or the HCD until its `complete()` is called.
The exceptions relate to periodic transfer scheduling. For both
interrupt and isochronous urbs, as part of successful URB submission
urb->interval is modified to reflect the actual transfer period used
(normally some power of two units). And for isochronous urbs,
urb->start\_frame is modified to reflect when the URB’s transfers were
scheduled to start.

Not all isochronous transfer scheduling policies will work, but most
host controller drivers should easily handle ISO queues going from now
until 10-200 msec into the future. Drivers should try to keep at
least one or two msec of data in the queue; many controllers require
that new transfers start at least 1 msec in the future when they are
added. If the driver is unable to keep up and the queue empties out,
the behavior for new submissions is governed by the URB\_ISO\_ASAP flag.
If the flag is set, or if the queue is idle, then the URB is always
assigned to the first available (and not yet expired) slot in the
endpoint’s schedule. If the flag is not set and the queue is active
then the URB is always assigned to the next slot in the schedule
following the end of the endpoint’s previous URB, even if that slot is
in the past. When a packet is assigned in this way to a slot that has
already expired, the packet is not transmitted and the corresponding
usb\_iso\_packet\_descriptor’s status field will return -EXDEV. If this
would happen to all the packets in the URB, submission fails with a
-EXDEV error code.

For control endpoints, the synchronous [`usb_control_msg()`](#c.usb_core.usb_control_msg "usb_core.usb_control_msg") call is
often used (in non-interrupt context) instead of this call.
That is often used through convenience wrappers, for the requests
that are standardized in the USB 2.0 specification. For bulk
endpoints, a synchronous [`usb_bulk_msg()`](#c.usb_core.usb_bulk_msg "usb_core.usb_bulk_msg") call is available.

Request Queuing:

URBs may be submitted to endpoints before previous ones complete, to
minimize the impact of interrupt latencies and system overhead on data
throughput. With that queuing policy, an endpoint’s queue would never
be empty. This is required for continuous isochronous data streams,
and may also be required for some kinds of interrupt transfers. Such
queuing also maximizes bandwidth utilization by letting USB controllers
start work on later requests before driver software has finished the
completion processing for earlier (successful) requests.

As of Linux 2.6, all USB endpoint transfer queues support depths greater
than one. This was previously a HCD-specific behavior, except for ISO
transfers. Non-isochronous endpoint queues are inactive during cleanup
after faults (transfer errors or cancellation).

Reserved Bandwidth Transfers:

Periodic transfers (interrupt or isochronous) are performed repeatedly,
using the interval specified in the urb. Submitting the first urb to
the endpoint reserves the bandwidth necessary to make those transfers.
If the USB subsystem can’t allocate sufficient bandwidth to perform
the periodic request, submitting such a periodic request should fail.

For devices under xHCI, the bandwidth is reserved at configuration time, or
when the alt setting is selected. If there is not enough bus bandwidth, the
configuration/alt setting request will fail. Therefore, submissions to
periodic endpoints on devices under xHCI should never fail due to bandwidth
constraints.

Device drivers must explicitly request that repetition, by ensuring that
some URB is always on the endpoint’s queue (except possibly for short
periods during completion callbacks). When there is no longer an urb
queued, the endpoint’s bandwidth reservation is canceled. This means
drivers can use their completion handlers to ensure they keep bandwidth
they need, by reinitializing and resubmitting the just-completed urb
until the driver longer needs that periodic bandwidth.

Memory Flags:

The general rules for how to decide which mem\_flags to use
are the same as for kmalloc. There are four
different possible values; GFP\_KERNEL, GFP\_NOFS, GFP\_NOIO and
GFP\_ATOMIC.

GFP\_NOFS is not ever used, as it has not been implemented yet.

GFP\_ATOMIC is used when
:   1. you are inside a completion handler, an interrupt, bottom half,
       tasklet or timer, or
    2. you are holding a spinlock or rwlock (does not apply to
       semaphores), or
    3. current->state != TASK\_RUNNING, this is the case only after
       you’ve changed it.

GFP\_NOIO is used in the block io path and error handling of storage
devices.

All other situations use GFP\_KERNEL.

Some more specific rules for mem\_flags can be inferred, such as
:   1. start\_xmit, timeout, and receive methods of network drivers must
       use GFP\_ATOMIC (they are called with a spinlock held);
    2. queuecommand methods of scsi drivers must use GFP\_ATOMIC (also
       called with a spinlock held);
    3. If you use a kernel thread with a network driver you must use
       GFP\_NOIO, unless (b) or (c) apply;
    4. after you have done a `down()` you can use GFP\_KERNEL, unless (b) or (c)
       apply or your are in a storage driver’s block io path;
    5. USB probe and disconnect can use GFP\_KERNEL unless (b) or (c) apply; and
    6. changing firmware on a running storage or net device uses
       GFP\_NOIO, unless b) or c) apply

**Return**

0 on successful submissions. A negative error number otherwise.

int usb\_unlink\_urb(struct [urb](#c.usb_unlink_urb "urb") \*urb)
:   abort/cancel a transfer request for an endpoint

**Parameters**

`struct urb *urb`
:   pointer to urb describing a previously submitted request,
    may be NULL

**Description**

This routine cancels an in-progress request. URBs complete only once
per submission, and may be canceled only once per submission.
Successful cancellation means termination of **urb** will be expedited
and the completion handler will be called with a status code
indicating that the request has been canceled (rather than any other
code).

Drivers should not call this routine or related routines, such as
`usb_kill_urb()`, after their disconnect method has returned. The
disconnect function should synchronize with a driver’s I/O routines
to insure that all URB-related activity has completed before it returns.

This request is asynchronous, however the HCD might call the ->`complete()`
callback during unlink. Therefore when drivers call `usb_unlink_urb()`, they
must not hold any locks that may be taken by the completion function.
Success is indicated by returning -EINPROGRESS, at which time the URB will
probably not yet have been given back to the device driver. When it is
eventually called, the completion function will see **urb->status** ==
-ECONNRESET.
Failure is indicated by `usb_unlink_urb()` returning any other value.
Unlinking will fail when **urb** is not currently “linked” (i.e., it was
never submitted, or it was unlinked before, or the hardware is already
finished with it), even if the completion handler has not yet run.

The URB must not be deallocated while this routine is running. In
particular, when a driver calls this routine, it must insure that the
completion handler cannot deallocate the URB.

Unlinking and Endpoint Queues:

[The behaviors and guarantees described below do not apply to virtual
root hubs but only to endpoint queues for physical USB devices.]

Host Controller Drivers (HCDs) place all the URBs for a particular
endpoint in a queue. Normally the queue advances as the controller
hardware processes each request. But when an URB terminates with an
error its queue generally stops (see below), at least until that URB’s
completion routine returns. It is guaranteed that a stopped queue
will not restart until all its unlinked URBs have been fully retired,
with their completion routines run, even if that’s not until some time
after the original completion handler returns. The same behavior and
guarantee apply when an URB terminates because it was unlinked.

Bulk and interrupt endpoint queues are guaranteed to stop whenever an
URB terminates with any sort of error, including -ECONNRESET, -ENOENT,
and -EREMOTEIO. Control endpoint queues behave the same way except
that they are not guaranteed to stop for -EREMOTEIO errors. Queues
for isochronous endpoints are treated differently, because they must
advance at fixed rates. Such queues do not stop when an URB
encounters an error or is unlinked. An unlinked isochronous URB may
leave a gap in the stream of packets; it is undefined whether such
gaps can be filled in.

Note that early termination of an URB because a short packet was
received will generate a -EREMOTEIO error if and only if the
URB\_SHORT\_NOT\_OK flag is set. By setting this flag, USB device
drivers can build deep queues for large or complex bulk transfers
and clean them up reliably after any sort of aborted transfer by
unlinking all pending URBs at the first fault.

When a control URB terminates with an error other than -EREMOTEIO, it
is quite likely that the status stage of the transfer will not take
place.

**Return**

-EINPROGRESS on success. See description for other values on
failure.

void usb\_kill\_urb(struct [urb](#c.usb_kill_urb "urb") \*urb)
:   cancel a transfer request and wait for it to finish

**Parameters**

`struct urb *urb`
:   pointer to URB describing a previously submitted request,
    may be NULL

**Description**

This routine cancels an in-progress request. It is guaranteed that
upon return all completion handlers will have finished and the URB
will be totally idle and available for reuse. These features make
this an ideal way to stop I/O in a `disconnect()` callback or `close()`
function. If the request has not already finished or been unlinked
the completion handler will see urb->status == -ENOENT.

While the routine is running, attempts to resubmit the URB will fail
with error -EPERM. Thus even if the URB’s completion handler always
tries to resubmit, it will not succeed and the URB will become idle.

The URB must not be deallocated while this routine is running. In
particular, when a driver calls this routine, it must insure that the
completion handler cannot deallocate the URB.

This routine may not be used in an interrupt context (such as a bottom
half or a completion handler), or when holding a spinlock, or in other
situations where the caller can’t `schedule()`.

This routine should not be called by a driver after its disconnect
method has returned.

void usb\_poison\_urb(struct [urb](#c.usb_poison_urb "urb") \*urb)
:   reliably kill a transfer and prevent further use of an URB

**Parameters**

`struct urb *urb`
:   pointer to URB describing a previously submitted request,
    may be NULL

**Description**

This routine cancels an in-progress request. It is guaranteed that
upon return all completion handlers will have finished and the URB
will be totally idle and cannot be reused. These features make
this an ideal way to stop I/O in a `disconnect()` callback.
If the request has not already finished or been unlinked
the completion handler will see urb->status == -ENOENT.

After and while the routine runs, attempts to resubmit the URB will fail
with error -EPERM. Thus even if the URB’s completion handler always
tries to resubmit, it will not succeed and the URB will become idle.

The URB must not be deallocated while this routine is running. In
particular, when a driver calls this routine, it must insure that the
completion handler cannot deallocate the URB.

This routine may not be used in an interrupt context (such as a bottom
half or a completion handler), or when holding a spinlock, or in other
situations where the caller can’t `schedule()`.

This routine should not be called by a driver after its disconnect
method has returned.

void usb\_block\_urb(struct [urb](#c.usb_block_urb "urb") \*urb)
:   reliably prevent further use of an URB

**Parameters**

`struct urb *urb`
:   pointer to URB to be blocked, may be NULL

**Description**

After the routine has run, attempts to resubmit the URB will fail
with error -EPERM. Thus even if the URB’s completion handler always
tries to resubmit, it will not succeed and the URB will become idle.

The URB must not be deallocated while this routine is running. In
particular, when a driver calls this routine, it must insure that the
completion handler cannot deallocate the URB.

void usb\_kill\_anchored\_urbs(struct usb\_anchor \*anchor)
:   kill all URBs associated with an anchor

**Parameters**

`struct usb_anchor *anchor`
:   anchor the requests are bound to

**Description**

This kills all outstanding URBs starting from the back of the queue,
with guarantee that no completer callbacks will take place from the
anchor after this function returns.

This routine should not be called by a driver after its disconnect
method has returned.

void usb\_poison\_anchored\_urbs(struct usb\_anchor \*anchor)
:   cease all traffic from an anchor

**Parameters**

`struct usb_anchor *anchor`
:   anchor the requests are bound to

**Description**

this allows all outstanding URBs to be poisoned starting
from the back of the queue. Newly added URBs will also be
poisoned

This routine should not be called by a driver after its disconnect
method has returned.

void usb\_unpoison\_anchored\_urbs(struct usb\_anchor \*anchor)
:   let an anchor be used successfully again

**Parameters**

`struct usb_anchor *anchor`
:   anchor the requests are bound to

**Description**

Reverses the effect of usb\_poison\_anchored\_urbs
the anchor can be used normally after it returns

void usb\_anchor\_suspend\_wakeups(struct usb\_anchor \*anchor)

**Parameters**

`struct usb_anchor *anchor`
:   the anchor you want to suspend wakeups on

**Description**

Call this to stop the last urb being unanchored from waking up any
usb\_wait\_anchor\_empty\_timeout waiters. This is used in the hcd urb give-
back path to delay waking up until after the completion handler has run.

void usb\_anchor\_resume\_wakeups(struct usb\_anchor \*anchor)

**Parameters**

`struct usb_anchor *anchor`
:   the anchor you want to resume wakeups on

**Description**

Allow usb\_wait\_anchor\_empty\_timeout waiters to be woken up again, and
wake up any current waiters if the anchor is empty.

int usb\_wait\_anchor\_empty\_timeout(struct usb\_anchor \*anchor, unsigned int timeout)
:   wait for an anchor to be unused

**Parameters**

`struct usb_anchor *anchor`
:   the anchor you want to become unused

`unsigned int timeout`
:   how long you are willing to wait in milliseconds

**Description**

Call this is you want to be sure all an anchor’s
URBs have finished

**Return**

Non-zero if the anchor became unused. Zero on timeout.

struct [urb](#c.urb "urb") \*usb\_get\_from\_anchor(struct usb\_anchor \*anchor)
:   get an anchor’s oldest urb

**Parameters**

`struct usb_anchor *anchor`
:   the anchor whose urb you want

**Description**

This will take the oldest urb from an anchor,
unanchor and return it

**Return**

The oldest urb from **anchor**, or `NULL` if **anchor** has no
urbs associated with it.

void usb\_scuttle\_anchored\_urbs(struct usb\_anchor \*anchor)
:   unanchor all an anchor’s urbs

**Parameters**

`struct usb_anchor *anchor`
:   the anchor whose urbs you want to unanchor

**Description**

use this to get rid of all an anchor’s urbs

int usb\_anchor\_empty(struct usb\_anchor \*anchor)
:   is an anchor empty

**Parameters**

`struct usb_anchor *anchor`
:   the anchor you want to query

**Return**

1 if the anchor has no urbs associated with it.

int usb\_control\_msg(struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned int pipe, \_\_u8 request, \_\_u8 requesttype, \_\_u16 value, \_\_u16 index, void \*data, \_\_u16 size, int timeout)
:   Builds a control urb, sends it off and waits for completion

**Parameters**

`struct usb_device *dev`
:   pointer to the usb device to send the message to

`unsigned int pipe`
:   endpoint “pipe” to send the message to

`__u8 request`
:   USB message request value

`__u8 requesttype`
:   USB message request type value

`__u16 value`
:   USB message value

`__u16 index`
:   USB message index value

`void *data`
:   pointer to the data to send

`__u16 size`
:   length in bytes of the data to send

`int timeout`
:   time in msecs to wait for the message to complete before timing out

**Context**

task context, might sleep.

**Description**

This function sends a simple control message to a specified endpoint and
waits for the message to complete, or timeout.

Don’t use this function from within an interrupt context. If you need
an asynchronous message, or need to send a message from within interrupt
context, use `usb_submit_urb()`. If a thread in your driver uses this call,
make sure your `disconnect()` method can wait for it to complete. Since you
don’t have a handle on the URB used, you can’t cancel the request.

**Return**

If successful, the number of bytes transferred. Otherwise, a negative
error number.

int usb\_control\_msg\_send(struct [usb\_device](#c.usb_device "usb_device") \*dev, \_\_u8 endpoint, \_\_u8 request, \_\_u8 requesttype, \_\_u16 value, \_\_u16 index, const void \*driver\_data, \_\_u16 size, int timeout, gfp\_t memflags)
:   Builds a control “send” message, sends it off and waits for completion

**Parameters**

`struct usb_device *dev`
:   pointer to the usb device to send the message to

`__u8 endpoint`
:   endpoint to send the message to

`__u8 request`
:   USB message request value

`__u8 requesttype`
:   USB message request type value

`__u16 value`
:   USB message value

`__u16 index`
:   USB message index value

`const void *driver_data`
:   pointer to the data to send

`__u16 size`
:   length in bytes of the data to send

`int timeout`
:   time in msecs to wait for the message to complete before timing out

`gfp_t memflags`
:   the flags for memory allocation for buffers

**Context**

!in\_interrupt ()

**Description**

This function sends a control message to a specified endpoint that is not
expected to fill in a response (i.e. a “send message”) and waits for the
message to complete, or timeout.

Do not use this function from within an interrupt context. If you need
an asynchronous message, or need to send a message from within interrupt
context, use `usb_submit_urb()`. If a thread in your driver uses this call,
make sure your `disconnect()` method can wait for it to complete. Since you
don’t have a handle on the URB used, you can’t cancel the request.

The data pointer can be made to a reference on the stack, or anywhere else,
as it will not be modified at all. This does not have the restriction that
[`usb_control_msg()`](#c.usb_core.usb_control_msg "usb_core.usb_control_msg") has where the data pointer must be to dynamically allocated
memory (i.e. memory that can be successfully DMAed to a device).

**Return**

If successful, 0 is returned, Otherwise, a negative error number.

int usb\_control\_msg\_recv(struct [usb\_device](#c.usb_device "usb_device") \*dev, \_\_u8 endpoint, \_\_u8 request, \_\_u8 requesttype, \_\_u16 value, \_\_u16 index, void \*driver\_data, \_\_u16 size, int timeout, gfp\_t memflags)
:   Builds a control “receive” message, sends it off and waits for completion

**Parameters**

`struct usb_device *dev`
:   pointer to the usb device to send the message to

`__u8 endpoint`
:   endpoint to send the message to

`__u8 request`
:   USB message request value

`__u8 requesttype`
:   USB message request type value

`__u16 value`
:   USB message value

`__u16 index`
:   USB message index value

`void *driver_data`
:   pointer to the data to be filled in by the message

`__u16 size`
:   length in bytes of the data to be received

`int timeout`
:   time in msecs to wait for the message to complete before timing out

`gfp_t memflags`
:   the flags for memory allocation for buffers

**Context**

!in\_interrupt ()

**Description**

This function sends a control message to a specified endpoint that is
expected to fill in a response (i.e. a “receive message”) and waits for the
message to complete, or timeout.

Do not use this function from within an interrupt context. If you need
an asynchronous message, or need to send a message from within interrupt
context, use `usb_submit_urb()`. If a thread in your driver uses this call,
make sure your `disconnect()` method can wait for it to complete. Since you
don’t have a handle on the URB used, you can’t cancel the request.

The data pointer can be made to a reference on the stack, or anywhere else
that can be successfully written to. This function does not have the
restriction that [`usb_control_msg()`](#c.usb_core.usb_control_msg "usb_core.usb_control_msg") has where the data pointer must be to
dynamically allocated memory (i.e. memory that can be successfully DMAed to a
device).

The “whole” message must be properly received from the device in order for
this function to be successful. If a device returns less than the expected
amount of data, then the function will fail. Do not use this for messages
where a variable amount of data might be returned.

**Return**

If successful, 0 is returned, Otherwise, a negative error number.

int usb\_interrupt\_msg(struct [usb\_device](#c.usb_device "usb_device") \*usb\_dev, unsigned int pipe, void \*data, int len, int \*actual\_length, int timeout)
:   Builds an interrupt urb, sends it off and waits for completion

**Parameters**

`struct usb_device *usb_dev`
:   pointer to the usb device to send the message to

`unsigned int pipe`
:   endpoint “pipe” to send the message to

`void *data`
:   pointer to the data to send

`int len`
:   length in bytes of the data to send

`int *actual_length`
:   pointer to a location to put the actual length transferred
    in bytes

`int timeout`
:   time in msecs to wait for the message to complete before timing out

**Context**

task context, might sleep.

**Description**

This function sends a simple interrupt message to a specified endpoint and
waits for the message to complete, or timeout.

Don’t use this function from within an interrupt context. If you need
an asynchronous message, or need to send a message from within interrupt
context, use `usb_submit_urb()` If a thread in your driver uses this call,
make sure your `disconnect()` method can wait for it to complete. Since you
don’t have a handle on the URB used, you can’t cancel the request.

**Return**

If successful, 0. Otherwise a negative error number. The number of actual
bytes transferred will be stored in the **actual\_length** parameter.

int usb\_bulk\_msg(struct [usb\_device](#c.usb_device "usb_device") \*usb\_dev, unsigned int pipe, void \*data, int len, int \*actual\_length, int timeout)
:   Builds a bulk urb, sends it off and waits for completion

**Parameters**

`struct usb_device *usb_dev`
:   pointer to the usb device to send the message to

`unsigned int pipe`
:   endpoint “pipe” to send the message to

`void *data`
:   pointer to the data to send

`int len`
:   length in bytes of the data to send

`int *actual_length`
:   pointer to a location to put the actual length transferred
    in bytes

`int timeout`
:   time in msecs to wait for the message to complete before timing out

**Context**

task context, might sleep.

**Description**

This function sends a simple bulk message to a specified endpoint
and waits for the message to complete, or timeout.

Don’t use this function from within an interrupt context. If you need
an asynchronous message, or need to send a message from within interrupt
context, use `usb_submit_urb()` If a thread in your driver uses this call,
make sure your `disconnect()` method can wait for it to complete. Since you
don’t have a handle on the URB used, you can’t cancel the request.

Because there is no [`usb_interrupt_msg()`](#c.usb_core.usb_interrupt_msg "usb_core.usb_interrupt_msg") and no USBDEVFS\_INTERRUPT ioctl,
users are forced to abuse this routine by using it to submit URBs for
interrupt endpoints. We will take the liberty of creating an interrupt URB
(with the default interval) if the target is an interrupt endpoint.

**Return**

If successful, 0. Otherwise a negative error number. The number of actual
bytes transferred will be stored in the **actual\_length** parameter.

int usb\_bulk\_msg\_killable(struct [usb\_device](#c.usb_device "usb_device") \*usb\_dev, unsigned int pipe, void \*data, int len, int \*actual\_length, int timeout)
:   Builds a bulk urb, sends it off and waits for completion in a killable state

**Parameters**

`struct usb_device *usb_dev`
:   pointer to the usb device to send the message to

`unsigned int pipe`
:   endpoint “pipe” to send the message to

`void *data`
:   pointer to the data to send

`int len`
:   length in bytes of the data to send

`int *actual_length`
:   pointer to a location to put the actual length transferred
    in bytes

`int timeout`
:   time in msecs to wait for the message to complete before
    timing out (if <= 0, the wait is as long as possible)

**Context**

task context, might sleep.

**Description**

This function is just like `usb_blk_msg()`, except that it waits in a
killable state and there is no limit on the timeout length.

**Return**

If successful, 0. Otherwise a negative error number. The number of actual
bytes transferred will be stored in the **actual\_length** parameter.

int usb\_sg\_init(struct [usb\_sg\_request](#c.usb_sg_request "usb_sg_request") \*io, struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned pipe, unsigned period, struct scatterlist \*sg, int nents, size\_t length, gfp\_t mem\_flags)
:   initializes scatterlist-based bulk/interrupt I/O request

**Parameters**

`struct usb_sg_request *io`
:   request block being initialized. until [`usb_sg_wait()`](#c.usb_core.usb_sg_wait "usb_core.usb_sg_wait") returns,
    treat this as a pointer to an opaque block of memory,

`struct usb_device *dev`
:   the usb device that will send or receive the data

`unsigned pipe`
:   endpoint “pipe” used to transfer the data

`unsigned period`
:   polling rate for interrupt endpoints, in frames or
    (for high speed endpoints) microframes; ignored for bulk

`struct scatterlist *sg`
:   scatterlist entries

`int nents`
:   how many entries in the scatterlist

`size_t length`
:   how many bytes to send from the scatterlist, or zero to
    send every byte identified in the list.

`gfp_t mem_flags`
:   SLAB\_\* flags affecting memory allocations in this call

**Description**

This initializes a scatter/gather request, allocating resources such as
I/O mappings and urb memory (except maybe memory used by USB controller
drivers).

The request must be issued using [`usb_sg_wait()`](#c.usb_core.usb_sg_wait "usb_core.usb_sg_wait"), which waits for the I/O to
complete (or to be canceled) and then cleans up all resources allocated by
[`usb_sg_init()`](#c.usb_core.usb_sg_init "usb_core.usb_sg_init").

The request may be canceled with [`usb_sg_cancel()`](#c.usb_core.usb_sg_cancel "usb_core.usb_sg_cancel"), either before or after
[`usb_sg_wait()`](#c.usb_core.usb_sg_wait "usb_core.usb_sg_wait") is called.

**Return**

Zero for success, else a negative errno value.

void usb\_sg\_wait(struct [usb\_sg\_request](#c.usb_sg_request "usb_sg_request") \*io)
:   synchronously execute scatter/gather request

**Parameters**

`struct usb_sg_request *io`
:   request block handle, as initialized with [`usb_sg_init()`](#c.usb_core.usb_sg_init "usb_core.usb_sg_init").
    some fields become accessible when this call returns.

**Context**

task context, might sleep.

**Description**

This function blocks until the specified I/O operation completes. It
leverages the grouping of the related I/O requests to get good transfer
rates, by queueing the requests. At higher speeds, such queuing can
significantly improve USB throughput.

There are three kinds of completion for this function.

1. success, where io->status is zero. The number of io->bytes
   transferred is as requested.
2. error, where io->status is a negative errno value. The number
   of io->bytes transferred before the error is usually less
   than requested, and can be nonzero.
3. cancellation, a type of error with status -ECONNRESET that
   is initiated by [`usb_sg_cancel()`](#c.usb_core.usb_sg_cancel "usb_core.usb_sg_cancel").

When this function returns, all memory allocated through [`usb_sg_init()`](#c.usb_core.usb_sg_init "usb_core.usb_sg_init") or
this call will have been freed. The request block parameter may still be
passed to [`usb_sg_cancel()`](#c.usb_core.usb_sg_cancel "usb_core.usb_sg_cancel"), or it may be freed. It could also be
reinitialized and then reused.

Data Transfer Rates:

Bulk transfers are valid for full or high speed endpoints.
The best full speed data rate is 19 packets of 64 bytes each
per frame, or 1216 bytes per millisecond.
The best high speed data rate is 13 packets of 512 bytes each
per microframe, or 52 KBytes per millisecond.

The reason to use interrupt transfers through this API would most likely
be to reserve high speed bandwidth, where up to 24 KBytes per millisecond
could be transferred. That capability is less useful for low or full
speed interrupt endpoints, which allow at most one packet per millisecond,
of at most 8 or 64 bytes (respectively).

It is not necessary to call this function to reserve bandwidth for devices
under an xHCI host controller, as the bandwidth is reserved when the
configuration or interface alt setting is selected.

void usb\_sg\_cancel(struct [usb\_sg\_request](#c.usb_sg_request "usb_sg_request") \*io)
:   stop scatter/gather i/o issued by [`usb_sg_wait()`](#c.usb_core.usb_sg_wait "usb_core.usb_sg_wait")

**Parameters**

`struct usb_sg_request *io`
:   request block, initialized with [`usb_sg_init()`](#c.usb_core.usb_sg_init "usb_core.usb_sg_init")

**Description**

This stops a request after it has been started by [`usb_sg_wait()`](#c.usb_core.usb_sg_wait "usb_core.usb_sg_wait").
It can also prevents one initialized by [`usb_sg_init()`](#c.usb_core.usb_sg_init "usb_core.usb_sg_init") from starting,
so that call just frees resources allocated to the request.

int usb\_get\_descriptor(struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned char type, unsigned char index, void \*buf, int size)
:   issues a generic GET\_DESCRIPTOR request

**Parameters**

`struct usb_device *dev`
:   the device whose descriptor is being retrieved

`unsigned char type`
:   the descriptor type (USB\_DT\_\*)

`unsigned char index`
:   the number of the descriptor

`void *buf`
:   where to put the descriptor

`int size`
:   how big is “buf”?

**Context**

task context, might sleep.

**Description**

Gets a USB descriptor. Convenience functions exist to simplify
getting some types of descriptors. Use
`usb_get_string()` or [`usb_string()`](#c.usb_core.usb_string "usb_core.usb_string") for USB\_DT\_STRING.
Device (USB\_DT\_DEVICE) and configuration descriptors (USB\_DT\_CONFIG)
are part of the device structure.
In addition to a number of USB-standard descriptors, some
devices also use class-specific or vendor-specific descriptors.

This call is synchronous, and may not be used in an interrupt context.

**Return**

The number of bytes received on success, or else the status code
returned by the underlying [`usb_control_msg()`](#c.usb_core.usb_control_msg "usb_core.usb_control_msg") call.

int usb\_string(struct [usb\_device](#c.usb_device "usb_device") \*dev, int index, char \*buf, size\_t size)
:   returns UTF-8 version of a string descriptor

**Parameters**

`struct usb_device *dev`
:   the device whose string descriptor is being retrieved

`int index`
:   the number of the descriptor

`char *buf`
:   where to put the string

`size_t size`
:   how big is “buf”?

**Context**

task context, might sleep.

**Description**

This converts the UTF-16LE encoded strings returned by devices, from
`usb_get_string_descriptor()`, to null-terminated UTF-8 encoded ones
that are more usable in most kernel contexts. Note that this function
chooses strings in the first language supported by the device.

This call is synchronous, and may not be used in an interrupt context.

**Return**

length of the string (>= 0) or usb\_control\_msg status (< 0).

char \*usb\_cache\_string(struct [usb\_device](#c.usb_device "usb_device") \*udev, int index)
:   read a string descriptor and cache it for later use

**Parameters**

`struct usb_device *udev`
:   the device whose string descriptor is being read

`int index`
:   the descriptor index

**Return**

A pointer to a kmalloc’ed buffer containing the descriptor string,
or `NULL` if the index is 0 or the string could not be read.

int usb\_get\_status(struct [usb\_device](#c.usb_device "usb_device") \*dev, int recip, int type, int target, void \*data)
:   issues a GET\_STATUS call

**Parameters**

`struct usb_device *dev`
:   the device whose status is being checked

`int recip`
:   USB\_RECIP\_\*; for device, interface, or endpoint

`int type`
:   USB\_STATUS\_TYPE\_\*; for standard or PTM status types

`int target`
:   zero (for device), else interface or endpoint number

`void *data`
:   pointer to two bytes of bitmap data

**Context**

task context, might sleep.

**Description**

Returns device, interface, or endpoint status. Normally only of
interest to see if the device is self powered, or has enabled the
remote wakeup facility; or whether a bulk or interrupt endpoint
is halted (“stalled”).

Bits in these status bitmaps are set using the SET\_FEATURE request,
and cleared using the CLEAR\_FEATURE request. The [`usb_clear_halt()`](#c.usb_core.usb_clear_halt "usb_core.usb_clear_halt")
function should be used to clear halt (“stall”) status.

This call is synchronous, and may not be used in an interrupt context.

Returns 0 and the status value in **\*data** (in host byte order) on success,
or else the status code from the underlying [`usb_control_msg()`](#c.usb_core.usb_control_msg "usb_core.usb_control_msg") call.

int usb\_clear\_halt(struct [usb\_device](#c.usb_device "usb_device") \*dev, int pipe)
:   tells device to clear endpoint halt/stall condition

**Parameters**

`struct usb_device *dev`
:   device whose endpoint is halted

`int pipe`
:   endpoint “pipe” being cleared

**Context**

task context, might sleep.

**Description**

This is used to clear halt conditions for bulk and interrupt endpoints,
as reported by URB completion status. Endpoints that are halted are
sometimes referred to as being “stalled”. Such endpoints are unable
to transmit or receive data until the halt status is cleared. Any URBs
queued for such an endpoint should normally be unlinked by the driver
before clearing the halt condition, as described in sections 5.7.5
and 5.8.5 of the USB 2.0 spec.

Note that control and isochronous endpoints don’t halt, although control
endpoints report “protocol stall” (for unsupported requests) using the
same status code used to report a true stall.

This call is synchronous, and may not be used in an interrupt context.
If a thread in your driver uses this call, make sure your `disconnect()`
method can wait for it to complete.

**Return**

Zero on success, or else the status code returned by the
underlying [`usb_control_msg()`](#c.usb_core.usb_control_msg "usb_core.usb_control_msg") call.

void usb\_reset\_endpoint(struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned int epaddr)
:   Reset an endpoint’s state.

**Parameters**

`struct usb_device *dev`
:   the device whose endpoint is to be reset

`unsigned int epaddr`
:   the endpoint’s address. Endpoint number for output,
    endpoint number + USB\_DIR\_IN for input

**Description**

Resets any host-side endpoint state such as the toggle bit,
sequence number or current window.

int usb\_set\_interface(struct [usb\_device](#c.usb_device "usb_device") \*dev, int interface, int alternate)
:   Makes a particular alternate setting be current

**Parameters**

`struct usb_device *dev`
:   the device whose interface is being updated

`int interface`
:   the interface being updated

`int alternate`
:   the setting being chosen.

**Context**

task context, might sleep.

**Description**

This is used to enable data transfers on interfaces that may not
be enabled by default. Not all devices support such configurability.
Only the driver bound to an interface may change its setting.

Within any given configuration, each interface may have several
alternative settings. These are often used to control levels of
bandwidth consumption. For example, the default setting for a high
speed interrupt endpoint may not send more than 64 bytes per microframe,
while interrupt transfers of up to 3KBytes per microframe are legal.
Also, isochronous endpoints may never be part of an
interface’s default setting. To access such bandwidth, alternate
interface settings must be made current.

Note that in the Linux USB subsystem, bandwidth associated with
an endpoint in a given alternate setting is not reserved until an URB
is submitted that needs that bandwidth. Some other operating systems
allocate bandwidth early, when a configuration is chosen.

xHCI reserves bandwidth and configures the alternate setting in
`usb_hcd_alloc_bandwidth()`. If it fails the original interface altsetting
may be disabled. Drivers cannot rely on any particular alternate
setting being in effect after a failure.

This call is synchronous, and may not be used in an interrupt context.
Also, drivers must not change altsettings while urbs are scheduled for
endpoints in that interface; all such urbs must first be completed
(perhaps forced by unlinking). If a thread in your driver uses this call,
make sure your `disconnect()` method can wait for it to complete.

**Return**

Zero on success, or else the status code returned by the
underlying [`usb_control_msg()`](#c.usb_core.usb_control_msg "usb_core.usb_control_msg") call.

int usb\_reset\_configuration(struct [usb\_device](#c.usb_device "usb_device") \*dev)
:   lightweight device reset

**Parameters**

`struct usb_device *dev`
:   the device whose configuration is being reset

**Description**

This issues a standard SET\_CONFIGURATION request to the device using
the current configuration. The effect is to reset most USB-related
state in the device, including interface altsettings (reset to zero),
endpoint halts (cleared), and endpoint state (only for bulk and interrupt
endpoints). Other usbcore state is unchanged, including bindings of
usb device drivers to interfaces.

Because this affects multiple interfaces, avoid using this with composite
(multi-interface) devices. Instead, the driver for each interface may
use [`usb_set_interface()`](#c.usb_core.usb_set_interface "usb_core.usb_set_interface") on the interfaces it claims. Be careful though;
some devices don’t support the SET\_INTERFACE request, and others won’t
reset all the interface state (notably endpoint state). Resetting the whole
configuration would affect other drivers’ interfaces.

The caller must own the device lock.

If this routine fails the device will probably be in an unusable state
with endpoints disabled, and interfaces only partially enabled.

**Return**

Zero on success, else a negative error code.

int usb\_set\_wireless\_status(struct [usb\_interface](#c.usb_interface "usb_interface") \*iface, enum usb\_wireless\_status status)
:   sets the wireless\_status `struct member`

**Parameters**

`struct usb_interface *iface`
:   the interface to modify

`enum usb_wireless_status status`
:   the new wireless status

**Description**

Set the wireless\_status `struct member` to the new value, and emit
sysfs changes as necessary.

**Return**

0 on success, -EALREADY if already set.

int usb\_driver\_set\_configuration(struct [usb\_device](#c.usb_device "usb_device") \*udev, int config)
:   Provide a way for drivers to change device configurations

**Parameters**

`struct usb_device *udev`
:   the device whose configuration is being updated

`int config`
:   the configuration being chosen.

**Context**

In process context, must be able to sleep

**Description**

Device interface drivers are not allowed to change device configurations.
This is because changing configurations will destroy the interface the
driver is bound to and create new ones; it would be like a floppy-disk
driver telling the computer to replace the floppy-disk drive with a
tape drive!

Still, in certain specialized circumstances the need may arise. This
routine gets around the normal restrictions by using a work thread to
submit the change-config request.

**Return**

0 if the request was successfully queued, error code otherwise.
The caller has no way to know whether the queued request will eventually
succeed.

int cdc\_parse\_cdc\_header(struct usb\_cdc\_parsed\_header \*hdr, struct [usb\_interface](#c.usb_interface "usb_interface") \*intf, u8 \*buffer, int buflen)
:   parse the extra headers present in CDC devices

**Parameters**

`struct usb_cdc_parsed_header *hdr`
:   the place to put the results of the parsing

`struct usb_interface *intf`
:   the interface for which parsing is requested

`u8 *buffer`
:   pointer to the extra headers to be parsed

`int buflen`
:   length of the extra headers

**Description**

This evaluates the extra headers present in CDC devices which
bind the interfaces for data and control and provide details
about the capabilities of the device.

**Return**

number of descriptors parsed or -EINVAL
if the header is contradictory beyond salvage

int usb\_register\_dev(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf, struct [usb\_class\_driver](#c.usb_class_driver "usb_class_driver") \*class\_driver)
:   register a USB device, and ask for a minor number

**Parameters**

`struct usb_interface *intf`
:   pointer to the usb\_interface that is being registered

`struct usb_class_driver *class_driver`
:   pointer to the usb\_class\_driver for this device

**Description**

This should be called by all USB drivers that use the USB major number.
If CONFIG\_USB\_DYNAMIC\_MINORS is enabled, the minor number will be
dynamically allocated out of the list of available ones. If it is not
enabled, the minor number will be based on the next available free minor,
starting at the class\_driver->minor\_base.

This function also creates a usb class device in the sysfs tree.

[`usb_deregister_dev()`](#c.usb_core.usb_deregister_dev "usb_core.usb_deregister_dev") must be called when the driver is done with
the minor numbers given out by this function.

**Return**

-EINVAL if something bad happens with trying to register a
device, and 0 on success.

void usb\_deregister\_dev(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf, struct [usb\_class\_driver](#c.usb_class_driver "usb_class_driver") \*class\_driver)
:   deregister a USB device’s dynamic minor.

**Parameters**

`struct usb_interface *intf`
:   pointer to the usb\_interface that is being deregistered

`struct usb_class_driver *class_driver`
:   pointer to the usb\_class\_driver for this device

**Description**

Used in conjunction with [`usb_register_dev()`](#c.usb_core.usb_register_dev "usb_core.usb_register_dev"). This function is called
when the USB driver is finished with the minor numbers gotten from a
call to [`usb_register_dev()`](#c.usb_core.usb_register_dev "usb_core.usb_register_dev") (usually when the device is disconnected
from the system.)

This function also removes the usb class device from the sysfs tree.

This should be called by all drivers that use the USB major number.

int usb\_driver\_claim\_interface(struct [usb\_driver](#c.usb_driver "usb_driver") \*driver, struct [usb\_interface](#c.usb_interface "usb_interface") \*iface, void \*data)
:   bind a driver to an interface

**Parameters**

`struct usb_driver *driver`
:   the driver to be bound

`struct usb_interface *iface`
:   the interface to which it will be bound; must be in the
    usb device’s active configuration

`void *data`
:   driver data associated with that interface

**Description**

This is used by usb device drivers that need to claim more than one
interface on a device when probing (audio and acm are current examples).
No device driver should directly modify internal usb\_interface or
usb\_device structure members.

Callers must own the device lock, so driver `probe()` entries don’t need
extra locking, but other call contexts may need to explicitly claim that
lock.

**Return**

0 on success.

void usb\_driver\_release\_interface(struct [usb\_driver](#c.usb_driver "usb_driver") \*driver, struct [usb\_interface](#c.usb_interface "usb_interface") \*iface)
:   unbind a driver from an interface

**Parameters**

`struct usb_driver *driver`
:   the driver to be unbound

`struct usb_interface *iface`
:   the interface from which it will be unbound

**Description**

This can be used by drivers to release an interface without waiting
for their `disconnect()` methods to be called. In typical cases this
also causes the driver `disconnect()` method to be called.

This call is synchronous, and may not be used in an interrupt context.
Callers must own the device lock, so driver `disconnect()` entries don’t
need extra locking, but other call contexts may need to explicitly claim
that lock.

const struct [usb\_device\_id](../basics.html#c.usb_device_id "usb_device_id") \*usb\_match\_id(struct [usb\_interface](#c.usb_interface "usb_interface") \*interface, const struct [usb\_device\_id](../basics.html#c.usb_device_id "usb_device_id") \*id)
:   find first usb\_device\_id matching device or interface

**Parameters**

`struct usb_interface *interface`
:   the interface of interest

`const struct usb_device_id *id`
:   array of usb\_device\_id structures, terminated by zero entry

**Description**

usb\_match\_id searches an array of usb\_device\_id’s and returns
the first one matching the device or interface, or null.
This is used when binding (or rebinding) a driver to an interface.
Most USB device drivers will use this indirectly, through the usb core,
but some layered driver frameworks use it directly.
These device tables are exported with MODULE\_DEVICE\_TABLE, through
modutils, to support the driver loading functionality of USB hotplugging.

What Matches:

The “match\_flags” element in a usb\_device\_id controls which
members are used. If the corresponding bit is set, the
value in the device\_id must match its corresponding member
in the device or interface descriptor, or else the device\_id
does not match.

“driver\_info” is normally used only by device drivers,
but you can create a wildcard “matches anything” usb\_device\_id
as a driver’s “modules.usbmap” entry if you provide an id with
only a nonzero “driver\_info” field. If you do this, the USB device
driver’s `probe()` routine should use additional intelligence to
decide whether to bind to the specified interface.

What Makes Good usb\_device\_id Tables:

The match algorithm is very simple, so that intelligence in
driver selection must come from smart driver id records.
Unless you have good reasons to use another selection policy,
provide match elements only in related groups, and order match
specifiers from specific to general. Use the macros provided
for that purpose if you can.

The most specific match specifiers use device descriptor
data. These are commonly used with product-specific matches;
the USB\_DEVICE macro lets you provide vendor and product IDs,
and you can also match against ranges of product revisions.
These are widely used for devices with application or vendor
specific bDeviceClass values.

Matches based on device class/subclass/protocol specifications
are slightly more general; use the USB\_DEVICE\_INFO macro, or
its siblings. These are used with single-function devices
where bDeviceClass doesn’t specify that each interface has
its own class.

Matches based on interface class/subclass/protocol are the
most general; they let drivers bind to any interface on a
multiple-function device. Use the USB\_INTERFACE\_INFO
macro, or its siblings, to match class-per-interface style
devices (as recorded in bInterfaceClass).

Note that an entry created by USB\_INTERFACE\_INFO won’t match
any interface if the device class is set to Vendor-Specific.
This is deliberate; according to the USB spec the meanings of
the interface class/subclass/protocol for these devices are also
vendor-specific, and hence matching against a standard product
class wouldn’t work anyway. If you really want to use an
interface-based match for such a device, create a match record
that also specifies the vendor ID. (Unforunately there isn’t a
standard macro for creating records like this.)

Within those groups, remember that not all combinations are
meaningful. For example, don’t give a product version range
without vendor and product IDs; or specify a protocol without
its associated class and subclass.

**Return**

The first matching usb\_device\_id, or `NULL`.

int usb\_register\_device\_driver(struct [usb\_device\_driver](#c.usb_device_driver "usb_device_driver") \*new\_udriver, struct module \*owner)
:   register a USB device (not interface) driver

**Parameters**

`struct usb_device_driver *new_udriver`
:   USB operations for the device driver

`struct module *owner`
:   module owner of this driver.

**Description**

Registers a USB device driver with the USB core. The list of
unattached devices will be rescanned whenever a new driver is
added, allowing the new driver to attach to any recognized devices.

**Return**

A negative error code on failure and 0 on success.

void usb\_deregister\_device\_driver(struct [usb\_device\_driver](#c.usb_device_driver "usb_device_driver") \*udriver)
:   unregister a USB device (not interface) driver

**Parameters**

`struct usb_device_driver *udriver`
:   USB operations of the device driver to unregister

**Context**

must be able to sleep

**Description**

Unlinks the specified driver from the internal USB driver list.

int usb\_register\_driver(struct [usb\_driver](#c.usb_driver "usb_driver") \*new\_driver, struct module \*owner, const char \*mod\_name)
:   register a USB interface driver

**Parameters**

`struct usb_driver *new_driver`
:   USB operations for the interface driver

`struct module *owner`
:   module owner of this driver.

`const char *mod_name`
:   module name string

**Description**

Registers a USB interface driver with the USB core. The list of
unattached interfaces will be rescanned whenever a new driver is
added, allowing the new driver to attach to any recognized interfaces.

**Return**

A negative error code on failure and 0 on success.

**NOTE**

if you want your driver to use the USB major number, you must call
[`usb_register_dev()`](#c.usb_core.usb_register_dev "usb_core.usb_register_dev") to enable that functionality. This function no longer
takes care of that.

void usb\_deregister(struct [usb\_driver](#c.usb_driver "usb_driver") \*driver)
:   unregister a USB interface driver

**Parameters**

`struct usb_driver *driver`
:   USB operations of the interface driver to unregister

**Context**

must be able to sleep

**Description**

Unlinks the specified driver from the internal USB driver list.

**NOTE**

If you called [`usb_register_dev()`](#c.usb_core.usb_register_dev "usb_core.usb_register_dev"), you still need to call
[`usb_deregister_dev()`](#c.usb_core.usb_deregister_dev "usb_core.usb_deregister_dev") to clean up your driver’s allocated minor numbers,
this \* call will no longer do it for you.

void usb\_enable\_autosuspend(struct [usb\_device](#c.usb_device "usb_device") \*udev)
:   allow a USB device to be autosuspended

**Parameters**

`struct usb_device *udev`
:   the USB device which may be autosuspended

**Description**

This routine allows **udev** to be autosuspended. An autosuspend won’t
take place until the autosuspend\_delay has elapsed and all the other
necessary conditions are satisfied.

The caller must hold **udev**’s device lock.

void usb\_disable\_autosuspend(struct [usb\_device](#c.usb_device "usb_device") \*udev)
:   prevent a USB device from being autosuspended

**Parameters**

`struct usb_device *udev`
:   the USB device which may not be autosuspended

**Description**

This routine prevents **udev** from being autosuspended and wakes it up
if it is already autosuspended.

The caller must hold **udev**’s device lock.

void usb\_autopm\_put\_interface(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   decrement a USB interface’s PM-usage counter

**Parameters**

`struct usb_interface *intf`
:   the usb\_interface whose counter should be decremented

**Description**

This routine should be called by an interface driver when it is
finished using **intf** and wants to allow it to autosuspend. A typical
example would be a character-device driver when its device file is
closed.

The routine decrements **intf**’s usage counter. When the counter reaches
0, a delayed autosuspend request for **intf**’s device is attempted. The
attempt may fail (see `autosuspend_check()`).

This routine can run only in process context.

void usb\_autopm\_put\_interface\_async(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   decrement a USB interface’s PM-usage counter

**Parameters**

`struct usb_interface *intf`
:   the usb\_interface whose counter should be decremented

**Description**

This routine does much the same thing as [`usb_autopm_put_interface()`](#c.usb_core.usb_autopm_put_interface "usb_core.usb_autopm_put_interface"):
It decrements **intf**’s usage counter and schedules a delayed
autosuspend request if the counter is <= 0. The difference is that it
does not perform any synchronization; callers should hold a private
lock and handle all synchronization issues themselves.

Typically a driver would call this routine during an URB’s completion
handler, if no more URBs were pending.

This routine can run in atomic context.

void usb\_autopm\_put\_interface\_no\_suspend(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   decrement a USB interface’s PM-usage counter

**Parameters**

`struct usb_interface *intf`
:   the usb\_interface whose counter should be decremented

**Description**

This routine decrements **intf**’s usage counter but does not carry out an
autosuspend.

This routine can run in atomic context.

int usb\_autopm\_get\_interface(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   increment a USB interface’s PM-usage counter

**Parameters**

`struct usb_interface *intf`
:   the usb\_interface whose counter should be incremented

**Description**

This routine should be called by an interface driver when it wants to
use **intf** and needs to guarantee that it is not suspended. In addition,
the routine prevents **intf** from being autosuspended subsequently. (Note
that this will not prevent suspend events originating in the PM core.)
This prevention will persist until [`usb_autopm_put_interface()`](#c.usb_core.usb_autopm_put_interface "usb_core.usb_autopm_put_interface") is called
or **intf** is unbound. A typical example would be a character-device
driver when its device file is opened.

**intf**’s usage counter is incremented to prevent subsequent autosuspends.
However if the autoresume fails then the counter is re-decremented.

This routine can run only in process context.

**Return**

0 on success.

int usb\_autopm\_get\_interface\_async(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   increment a USB interface’s PM-usage counter

**Parameters**

`struct usb_interface *intf`
:   the usb\_interface whose counter should be incremented

**Description**

This routine does much the same thing as
[`usb_autopm_get_interface()`](#c.usb_core.usb_autopm_get_interface "usb_core.usb_autopm_get_interface"): It increments **intf**’s usage counter and
queues an autoresume request if the device is suspended. The
differences are that it does not perform any synchronization (callers
should hold a private lock and handle all synchronization issues
themselves), and it does not autoresume the device directly (it only
queues a request). After a successful call, the device may not yet be
resumed.

This routine can run in atomic context.

**Return**

0 on success. A negative error code otherwise.

void usb\_autopm\_get\_interface\_no\_resume(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   increment a USB interface’s PM-usage counter

**Parameters**

`struct usb_interface *intf`
:   the usb\_interface whose counter should be incremented

**Description**

This routine increments **intf**’s usage counter but does not carry out an
autoresume.

This routine can run in atomic context.

int usb\_find\_common\_endpoints(struct usb\_host\_interface \*alt, struct usb\_endpoint\_descriptor \*\*bulk\_in, struct usb\_endpoint\_descriptor \*\*bulk\_out, struct usb\_endpoint\_descriptor \*\*int\_in, struct usb\_endpoint\_descriptor \*\*int\_out)
:   * look up common endpoint descriptors

**Parameters**

`struct usb_host_interface *alt`
:   alternate setting to search

`struct usb_endpoint_descriptor **bulk_in`
:   pointer to descriptor pointer, or NULL

`struct usb_endpoint_descriptor **bulk_out`
:   pointer to descriptor pointer, or NULL

`struct usb_endpoint_descriptor **int_in`
:   pointer to descriptor pointer, or NULL

`struct usb_endpoint_descriptor **int_out`
:   pointer to descriptor pointer, or NULL

**Description**

Search the alternate setting’s endpoint descriptors for the first bulk-in,
bulk-out, interrupt-in and interrupt-out endpoints and return them in the
provided pointers (unless they are NULL).

If a requested endpoint is not found, the corresponding pointer is set to
NULL.

**Return**

Zero if all requested descriptors were found, or -ENXIO otherwise.

int usb\_find\_common\_endpoints\_reverse(struct usb\_host\_interface \*alt, struct usb\_endpoint\_descriptor \*\*bulk\_in, struct usb\_endpoint\_descriptor \*\*bulk\_out, struct usb\_endpoint\_descriptor \*\*int\_in, struct usb\_endpoint\_descriptor \*\*int\_out)
:   * look up common endpoint descriptors

**Parameters**

`struct usb_host_interface *alt`
:   alternate setting to search

`struct usb_endpoint_descriptor **bulk_in`
:   pointer to descriptor pointer, or NULL

`struct usb_endpoint_descriptor **bulk_out`
:   pointer to descriptor pointer, or NULL

`struct usb_endpoint_descriptor **int_in`
:   pointer to descriptor pointer, or NULL

`struct usb_endpoint_descriptor **int_out`
:   pointer to descriptor pointer, or NULL

**Description**

Search the alternate setting’s endpoint descriptors for the last bulk-in,
bulk-out, interrupt-in and interrupt-out endpoints and return them in the
provided pointers (unless they are NULL).

If a requested endpoint is not found, the corresponding pointer is set to
NULL.

**Return**

Zero if all requested descriptors were found, or -ENXIO otherwise.

bool usb\_check\_bulk\_endpoints(const struct [usb\_interface](#c.usb_interface "usb_interface") \*intf, const u8 \*ep\_addrs)
:   Check whether an interface’s current altsetting contains a set of bulk endpoints with the given addresses.

**Parameters**

`const struct usb_interface *intf`
:   the interface whose current altsetting should be searched

`const u8 *ep_addrs`
:   0-terminated array of the endpoint addresses (number and
    direction) to look for

**Description**

Search for endpoints with the specified addresses and check their types.

**Return**

`true` if all the endpoints are found and are bulk, `false` otherwise.

bool usb\_check\_int\_endpoints(const struct [usb\_interface](#c.usb_interface "usb_interface") \*intf, const u8 \*ep\_addrs)
:   Check whether an interface’s current altsetting contains a set of interrupt endpoints with the given addresses.

**Parameters**

`const struct usb_interface *intf`
:   the interface whose current altsetting should be searched

`const u8 *ep_addrs`
:   0-terminated array of the endpoint addresses (number and
    direction) to look for

**Description**

Search for endpoints with the specified addresses and check their types.

**Return**

`true` if all the endpoints are found and are interrupt,
`false` otherwise.

struct usb\_host\_interface \*usb\_find\_alt\_setting(struct [usb\_host\_config](#c.usb_host_config "usb_host_config") \*config, unsigned int iface\_num, unsigned int alt\_num)
:   Given a configuration, find the alternate setting for the given interface.

**Parameters**

`struct usb_host_config *config`
:   the configuration to search (not necessarily the current config).

`unsigned int iface_num`
:   interface number to search in

`unsigned int alt_num`
:   alternate interface setting number to search for.

**Description**

Search the configuration’s interface cache for the given alt setting.

**Return**

The alternate setting, if found. `NULL` otherwise.

struct [usb\_interface](#c.usb_interface "usb_interface") \*usb\_ifnum\_to\_if(const struct [usb\_device](#c.usb_device "usb_device") \*dev, unsigned ifnum)
:   get the interface object with a given interface number

**Parameters**

`const struct usb_device *dev`
:   the device whose current configuration is considered

`unsigned ifnum`
:   the desired interface

**Description**

This walks the device descriptor for the currently active configuration
to find the interface object with the particular interface number.

Note that configuration descriptors are not required to assign interface
numbers sequentially, so that it would be incorrect to assume that
the first interface in that descriptor corresponds to interface zero.
This routine helps device drivers avoid such mistakes.
However, you should make sure that you do the right thing with any
alternate settings available for this interfaces.

Don’t call this function unless you are bound to one of the interfaces
on this device or you have locked the device!

**Return**

A pointer to the interface that has **ifnum** as interface number,
if found. `NULL` otherwise.

struct usb\_host\_interface \*usb\_altnum\_to\_altsetting(const struct [usb\_interface](#c.usb_interface "usb_interface") \*intf, unsigned int altnum)
:   get the altsetting structure with a given alternate setting number.

**Parameters**

`const struct usb_interface *intf`
:   the interface containing the altsetting in question

`unsigned int altnum`
:   the desired alternate setting number

**Description**

This searches the altsetting array of the specified interface for
an entry with the correct bAlternateSetting value.

Note that altsettings need not be stored sequentially by number, so
it would be incorrect to assume that the first altsetting entry in
the array corresponds to altsetting zero. This routine helps device
drivers avoid such mistakes.

Don’t call this function unless you are bound to the intf interface
or you have locked the device!

**Return**

A pointer to the entry of the altsetting array of **intf** that
has **altnum** as the alternate setting number. `NULL` if not found.

struct [usb\_interface](#c.usb_interface "usb_interface") \*usb\_find\_interface(struct [usb\_driver](#c.usb_driver "usb_driver") \*drv, int minor)
:   find usb\_interface pointer for driver and device

**Parameters**

`struct usb_driver *drv`
:   the driver whose current configuration is considered

`int minor`
:   the minor number of the desired device

**Description**

This walks the bus device list and returns a pointer to the interface
with the matching minor and driver. Note, this only works for devices
that share the USB major number.

**Return**

A pointer to the interface with the matching major and **minor**.

int usb\_for\_each\_dev(void \*data, int (\*fn)(struct [usb\_device](#c.usb_device "usb_device")\*, void\*))
:   iterate over all USB devices in the system

**Parameters**

`void *data`
:   data pointer that will be handed to the callback function

`int (*fn)(struct usb_device *, void *)`
:   callback function to be called for each USB device

**Description**

Iterate over all USB devices and call **fn** for each, passing it **data**. If it
returns anything other than 0, we break the iteration prematurely and return
that value.

struct [usb\_device](#c.usb_device "usb_device") \*usb\_alloc\_dev(struct [usb\_device](#c.usb_device "usb_device") \*parent, struct usb\_bus \*bus, unsigned port1)
:   usb device constructor (usbcore-internal)

**Parameters**

`struct usb_device *parent`
:   hub to which device is connected; null to allocate a root hub

`struct usb_bus *bus`
:   bus used to access the device

`unsigned port1`
:   one-based index of port; ignored for root hubs

**Context**

task context, might sleep.

**Description**

Only hub drivers (including virtual root hub drivers for host
controllers) should ever call this.

This call may not be used in a non-sleeping context.

**Return**

On success, a pointer to the allocated usb device. `NULL` on
failure.

struct [usb\_device](#c.usb_device "usb_device") \*usb\_get\_dev(struct [usb\_device](#c.usb_device "usb_device") \*dev)
:   increments the reference count of the usb device structure

**Parameters**

`struct usb_device *dev`
:   the device being referenced

**Description**

Each live reference to a device should be refcounted.

Drivers for USB interfaces should normally record such references in
their `probe()` methods, when they bind to an interface, and release
them by calling [`usb_put_dev()`](#c.usb_core.usb_put_dev "usb_core.usb_put_dev"), in their `disconnect()` methods.
However, if a driver does not access the usb\_device structure after
its `disconnect()` method returns then refcounting is not necessary,
because the USB core guarantees that a usb\_device will not be
deallocated until after all of its interface drivers have been unbound.

**Return**

A pointer to the device with the incremented reference counter.

void usb\_put\_dev(struct [usb\_device](#c.usb_device "usb_device") \*dev)
:   release a use of the usb device structure

**Parameters**

`struct usb_device *dev`
:   device that’s been disconnected

**Description**

Must be called when a user of a device is finished with it. When the last
user of the device calls this function, the memory of the device is freed.

struct [usb\_interface](#c.usb_interface "usb_interface") \*usb\_get\_intf(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   increments the reference count of the usb interface structure

**Parameters**

`struct usb_interface *intf`
:   the interface being referenced

**Description**

Each live reference to a interface must be refcounted.

Drivers for USB interfaces should normally record such references in
their `probe()` methods, when they bind to an interface, and release
them by calling [`usb_put_intf()`](#c.usb_core.usb_put_intf "usb_core.usb_put_intf"), in their `disconnect()` methods.
However, if a driver does not access the usb\_interface structure after
its `disconnect()` method returns then refcounting is not necessary,
because the USB core guarantees that a usb\_interface will not be
deallocated until after its driver has been unbound.

**Return**

A pointer to the interface with the incremented reference counter.

void usb\_put\_intf(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   release a use of the usb interface structure

**Parameters**

`struct usb_interface *intf`
:   interface that’s been decremented

**Description**

Must be called when a user of an interface is finished with it. When the
last user of the interface calls this function, the memory of the interface
is freed.

struct [device](../infrastructure.html#c.device "device") \*usb\_intf\_get\_dma\_device(struct [usb\_interface](#c.usb_interface "usb_interface") \*intf)
:   acquire a reference on the usb interface’s DMA endpoint

**Parameters**

`struct usb_interface *intf`
:   the usb interface

**Description**

While a USB device cannot perform DMA operations by itself, many USB
controllers can. A call to [`usb_intf_get_dma_device()`](#c.usb_core.usb_intf_get_dma_device "usb_core.usb_intf_get_dma_device") returns the DMA endpoint
for the given USB interface, if any. The returned device structure must be
released with `put_device()`.

See also `usb_get_dma_device()`.

**Return**

A reference to the usb interface’s DMA endpoint; or NULL if none
exists.

int usb\_lock\_device\_for\_reset(struct [usb\_device](#c.usb_device "usb_device") \*udev, const struct [usb\_interface](#c.usb_interface "usb_interface") \*iface)
:   cautiously acquire the lock for a usb device structure

**Parameters**

`struct usb_device *udev`
:   device that’s being locked

`const struct usb_interface *iface`
:   interface bound to the driver making the request (optional)

**Description**

Attempts to acquire the device lock, but fails if the device is
NOTATTACHED or SUSPENDED, or if iface is specified and the interface
is neither BINDING nor BOUND. Rather than sleeping to wait for the
lock, the routine polls repeatedly. This is to prevent deadlock with
disconnect; in some drivers (such as usb-storage) the `disconnect()`
or `suspend()` method will block waiting for a device reset to complete.

**Return**

A negative error code for failure, otherwise 0.

int usb\_get\_current\_frame\_number(struct [usb\_device](#c.usb_device "usb_device") \*dev)
:   return current bus frame number

**Parameters**

`struct usb_device *dev`
:   the device whose bus is being queried

**Return**

The current frame number for the USB host controller used
with the given USB device. This can be used when scheduling
isochronous requests.

**Note**

Different kinds of host controller have different “scheduling
horizons”. While one type might support scheduling only 32 frames
into the future, others could support scheduling up to 1024 frames
into the future.

void \*usb\_alloc\_coherent(struct [usb\_device](#c.usb_device "usb_device") \*dev, size\_t size, gfp\_t mem\_flags, dma\_addr\_t \*dma)
:   allocate dma-consistent buffer for URB\_NO\_xxx\_DMA\_MAP

**Parameters**

`struct usb_device *dev`
:   device the buffer will be used with

`size_t size`
:   requested buffer size

`gfp_t mem_flags`
:   affect whether allocation may block

`dma_addr_t *dma`
:   used to return DMA address of buffer

**Return**

Either null (indicating no buffer could be allocated), or the
cpu-space pointer to a buffer that may be used to perform DMA to the
specified device. Such cpu-space buffers are returned along with the DMA
address (through the pointer provided).

**Note**

These buffers are used with URB\_NO\_xxx\_DMA\_MAP set in urb->transfer\_flags
to avoid behaviors like using “DMA bounce buffers”, or thrashing IOMMU
hardware during URB completion/resubmit. The implementation varies between
platforms, depending on details of how DMA will work to this device.
Using these buffers also eliminates cacheline sharing problems on
architectures where CPU caches are not DMA-coherent. On systems without
bus-snooping caches, these buffers are uncached.

When the buffer is no longer used, free it with [`usb_free_coherent()`](#c.usb_core.usb_free_coherent "usb_core.usb_free_coherent").

void usb\_free\_coherent(struct [usb\_device](#c.usb_device "usb_device") \*dev, size\_t size, void \*addr, dma\_addr\_t dma)
:   free memory allocated with [`usb_alloc_coherent()`](#c.usb_core.usb_alloc_coherent "usb_core.usb_alloc_coherent")

**Parameters**

`struct usb_device *dev`
:   device the buffer was used with

`size_t size`
:   requested buffer size

`void *addr`
:   CPU address of buffer

`dma_addr_t dma`
:   DMA address of buffer

**Description**

This reclaims an I/O buffer, letting it be reused. The memory must have
been allocated using [`usb_alloc_coherent()`](#c.usb_core.usb_alloc_coherent "usb_core.usb_alloc_coherent"), and the parameters must match
those provided in that allocation request.

void \*usb\_alloc\_noncoherent(struct [usb\_device](#c.usb_device "usb_device") \*dev, size\_t size, gfp\_t mem\_flags, dma\_addr\_t \*dma, enum dma\_data\_direction dir, struct sg\_table \*\*table)
:   allocate dma-noncoherent buffer for URB\_NO\_xxx\_DMA\_MAP

**Parameters**

`struct usb_device *dev`
:   device the buffer will be used with

`size_t size`
:   requested buffer size

`gfp_t mem_flags`
:   affect whether allocation may block

`dma_addr_t *dma`
:   used to return DMA address of buffer

`enum dma_data_direction dir`
:   DMA transfer direction

`struct sg_table **table`
:   used to return sg\_table of allocated memory

**Description**

To explicit manage the memory ownership for the kernel vs the device by
USB core, the user needs save sg\_table to urb->sgt. Then USB core will
do DMA sync for CPU and device properly.

When the buffer is no longer used, free it with [`usb_free_noncoherent()`](#c.usb_core.usb_free_noncoherent "usb_core.usb_free_noncoherent").

**Return**

Either null (indicating no buffer could be allocated), or the
cpu-space pointer to a buffer that may be used to perform DMA to the
specified device. Such cpu-space buffers are returned along with the DMA
address (through the pointer provided).

void usb\_free\_noncoherent(struct [usb\_device](#c.usb_device "usb_device") \*dev, size\_t size, void \*addr, enum dma\_data\_direction dir, struct sg\_table \*table)
:   free memory allocated with [`usb_alloc_noncoherent()`](#c.usb_core.usb_alloc_noncoherent "usb_core.usb_alloc_noncoherent")

**Parameters**

`struct usb_device *dev`
:   device the buffer was used with

`size_t size`
:   requested buffer size

`void *addr`
:   CPU address of buffer

`enum dma_data_direction dir`
:   DMA transfer direction

`struct sg_table *table`
:   describe the allocated and DMA mapped memory,

**Description**

This reclaims an I/O buffer, letting it be reused. The memory must have
been allocated using [`usb_alloc_noncoherent()`](#c.usb_core.usb_alloc_noncoherent "usb_core.usb_alloc_noncoherent"), and the parameters must match
those provided in that allocation request.

u32 usb\_endpoint\_max\_periodic\_payload(struct [usb\_device](#c.usb_device "usb_device") \*udev, const struct [usb\_host\_endpoint](#c.usb_host_endpoint "usb_host_endpoint") \*ep)
:   Get maximum payload bytes per service interval

**Parameters**

`struct usb_device *udev`
:   The USB device

`const struct usb_host_endpoint *ep`
:   The endpoint

**Return**

the maximum number of bytes isochronous or interrupt endpoint **ep**
can transfer during a service interval, or 0 for other endpoints.

bool usb\_endpoint\_is\_hs\_isoc\_double(struct [usb\_device](#c.usb_device "usb_device") \*udev, const struct [usb\_host\_endpoint](#c.usb_host_endpoint "usb_host_endpoint") \*ep)
:   Tell whether an endpoint uses USB 2 Isochronous Double IN Bandwidth

**Parameters**

`struct usb_device *udev`
:   The USB device

`const struct usb_host_endpoint *ep`
:   The endpoint

**Return**

true if an endpoint **ep** conforms to USB 2 Isochronous Double IN
Bandwidth ECN, false otherwise.

int usb\_hub\_clear\_tt\_buffer(struct [urb](#c.usb_core.usb_hub_clear_tt_buffer "urb") \*urb)
:   clear control/bulk TT state in high speed hub

**Parameters**

`struct urb *urb`
:   an URB associated with the failed or incomplete split transaction

**Description**

High speed HCDs use this to tell the hub driver that some split control or
bulk transaction failed in a way that requires clearing internal state of
a transaction translator. This is normally detected (and reported) from
interrupt context.

It may not be possible for that hub to handle additional full (or low)
speed transactions until that state is fully cleared out.

**Return**

0 if successful. A negative error code otherwise.

void usb\_set\_device\_state(struct [usb\_device](#c.usb_device "usb_device") \*udev, enum usb\_device\_state new\_state)
:   change a device’s current state (usbcore, hcds)

**Parameters**

`struct usb_device *udev`
:   pointer to device whose state should be changed

`enum usb_device_state new_state`
:   new state value to be stored

**Description**

udev->state is \_not\_ fully protected by the device lock. Although
most transitions are made only while holding the lock, the state can
can change to USB\_STATE\_NOTATTACHED at almost any time. This
is so that devices can be marked as disconnected as soon as possible,
without having to wait for any semaphores to be released. As a result,
all changes to any device’s state must be protected by the
device\_state\_lock spinlock.

Once a device has been added to the device tree, all changes to its state
should be made using this routine. The state should \_not\_ be set directly.

If udev->state is already USB\_STATE\_NOTATTACHED then no change is made.
Otherwise udev->state is set to new\_state, and if new\_state is
USB\_STATE\_NOTATTACHED then all of udev’s descendants’ states are also set
to USB\_STATE\_NOTATTACHED.

void usb\_root\_hub\_lost\_power(struct [usb\_device](#c.usb_device "usb_device") \*rhdev)
:   called by HCD if the root hub lost Vbus power

**Parameters**

`struct usb_device *rhdev`
:   `struct usb_device` for the root hub

**Description**

The USB host controller driver calls this function when its root hub
is resumed and Vbus power has been interrupted or the controller
has been reset. The routine marks **rhdev** as having lost power.
When the hub driver is resumed it will take notice and carry out
power-session recovery for all the “USB-PERSIST”-enabled child devices;
the others will be disconnected.

int usb\_reset\_device(struct [usb\_device](#c.usb_device "usb_device") \*udev)
:   warn interface drivers and perform a USB port reset

**Parameters**

`struct usb_device *udev`
:   device to reset (not in NOTATTACHED state)

**Description**

Warns all drivers bound to registered interfaces (using their pre\_reset
method), performs the port reset, and then lets the drivers know that
the reset is over (using their post\_reset method).

**Return**

The same as for `usb_reset_and_verify_device()`.
However, if a reset is already in progress (for instance, if a
driver doesn’t have `pre_reset()` or `post_reset()` callbacks, and while
being unbound or re-bound during the ongoing reset its `disconnect()`
or `probe()` routine tries to perform a second, nested reset), the
routine returns -EINPROGRESS.

**Note**

The caller must own the device lock. For example, it’s safe to use
this from a driver `probe()` routine after downloading new firmware.
For calls that might not occur during `probe()`, drivers should lock
the device using [`usb_lock_device_for_reset()`](#c.usb_core.usb_lock_device_for_reset "usb_core.usb_lock_device_for_reset").

If an interface is currently being probed or disconnected, we assume
its driver knows how to handle resets. For all other interfaces,
if the driver doesn’t have pre\_reset and post\_reset methods then
we attempt to unbind it and rebind afterward.

void usb\_queue\_reset\_device(struct [usb\_interface](#c.usb_interface "usb_interface") \*iface)
:   Reset a USB device from an atomic context

**Parameters**

`struct usb_interface *iface`
:   USB interface belonging to the device to reset

**Description**

This function can be used to reset a USB device from an atomic
context, where [`usb_reset_device()`](#c.usb_core.usb_reset_device "usb_core.usb_reset_device") won’t work (as it blocks).

Doing a reset via this method is functionally equivalent to calling
[`usb_reset_device()`](#c.usb_core.usb_reset_device "usb_core.usb_reset_device"), except for the fact that it is delayed to a
workqueue. This means that any drivers bound to other interfaces
might be unbound, as well as users from usbfs in user space.

Corner cases:

* Scheduling two resets at the same time from two different drivers
  attached to two different interfaces of the same device is
  possible; depending on how the driver attached to each interface
  handles ->`pre_reset()`, the second reset might happen or not.
* If the reset is delayed so long that the interface is unbound from
  its driver, the reset will be skipped.
* This function can be called during .`probe()`. It can also be called
  during .`disconnect()`, but doing so is pointless because the reset
  will not occur. If you really want to reset the device during
  .`disconnect()`, call [`usb_reset_device()`](#c.usb_core.usb_reset_device "usb_core.usb_reset_device") directly -- but watch out
  for nested unbinding issues!

struct [usb\_device](#c.usb_device "usb_device") \*usb\_hub\_find\_child(struct [usb\_device](#c.usb_device "usb_device") \*hdev, int port1)
:   Get the pointer of child device attached to the port which is specified by **port1**.

**Parameters**

`struct usb_device *hdev`
:   USB device belonging to the usb hub

`int port1`
:   port num to indicate which port the child device
    is attached to.

**Description**

USB drivers call this function to get hub’s child device
pointer.

**Return**

`NULL` if input param is invalid and
child’s usb\_device pointer if non-NULL.

## Host Controller APIs

These APIs are only for use by host controller drivers, most of which
implement standard register interfaces such as XHCI, EHCI, OHCI, or UHCI. UHCI
was one of the first interfaces, designed by Intel and also used by VIA;
it doesn’t do much in hardware. OHCI was designed later, to have the
hardware do more work (bigger transfers, tracking protocol state, and so
on). EHCI was designed with USB 2.0; its design has features that
resemble OHCI (hardware does much more work) as well as UHCI (some parts
of ISO support, TD list processing). XHCI was designed with USB 3.0. It
continues to shift support for functionality into hardware.

There are host controllers other than the “big three”, although most PCI
based controllers (and a few non-PCI based ones) use one of those
interfaces. Not all host controllers use DMA; some use PIO, and there is
also a simulator and a virtual host controller to pipe USB over the network.

The same basic APIs are available to drivers for all those controllers.
For historical reasons they are in two layers: `struct
usb_bus` is a rather thin layer that became available
in the 2.2 kernels, while `struct usb_hcd`
is a more featureful layer
that lets HCDs share common code, to shrink driver size and
significantly reduce hcd-specific behaviors.

long usb\_calc\_bus\_time(int speed, int is\_input, int isoc, int bytecount)
:   approximate periodic transaction time in nanoseconds

**Parameters**

`int speed`
:   from dev->speed; USB\_SPEED\_{LOW,FULL,HIGH}

`int is_input`
:   true iff the transaction sends data to the host

`int isoc`
:   true for isochronous transactions, false for interrupt ones

`int bytecount`
:   how many bytes in the transaction.

**Return**

Approximate bus time in nanoseconds for a periodic transaction.

**Note**

See USB 2.0 spec section 5.11.3; only periodic transfers need to be
scheduled in software, this function is only used for such scheduling.

int usb\_hcd\_link\_urb\_to\_ep(struct usb\_hcd \*hcd, struct [urb](#c.usb_core.usb_hcd_link_urb_to_ep "urb") \*urb)
:   add an URB to its endpoint queue

**Parameters**

`struct usb_hcd *hcd`
:   host controller to which **urb** was submitted

`struct urb *urb`
:   URB being submitted

**Description**

Host controller drivers should call this routine in their `enqueue()`
method. The HCD’s private spinlock must be held and interrupts must
be disabled. The actions carried out here are required for URB
submission, as well as for endpoint shutdown and for usb\_kill\_urb.

**Return**

0 for no error, otherwise a negative error code (in which case
the `enqueue()` method must fail). If no error occurs but `enqueue()` fails
anyway, it must call [`usb_hcd_unlink_urb_from_ep()`](#c.usb_core.usb_hcd_unlink_urb_from_ep "usb_core.usb_hcd_unlink_urb_from_ep") before releasing
the private spinlock and returning.

int usb\_hcd\_check\_unlink\_urb(struct usb\_hcd \*hcd, struct [urb](#c.usb_core.usb_hcd_check_unlink_urb "urb") \*urb, int status)
:   check whether an URB may be unlinked

**Parameters**

`struct usb_hcd *hcd`
:   host controller to which **urb** was submitted

`struct urb *urb`
:   URB being checked for unlinkability

`int status`
:   error code to store in **urb** if the unlink succeeds

**Description**

Host controller drivers should call this routine in their `dequeue()`
method. The HCD’s private spinlock must be held and interrupts must
be disabled. The actions carried out here are required for making
sure than an unlink is valid.

> -EIDRM: **urb** was not submitted or has already completed.
> :   The completion function may not have been called yet.
>
> -EBUSY: **urb** has already been unlinked.

**Return**

0 for no error, otherwise a negative error code (in which case
the `dequeue()` method must fail). The possible error codes are:

void usb\_hcd\_unlink\_urb\_from\_ep(struct usb\_hcd \*hcd, struct [urb](#c.usb_core.usb_hcd_unlink_urb_from_ep "urb") \*urb)
:   remove an URB from its endpoint queue

**Parameters**

`struct usb_hcd *hcd`
:   host controller to which **urb** was submitted

`struct urb *urb`
:   URB being unlinked

**Description**

Host controller drivers should call this routine before calling
[`usb_hcd_giveback_urb()`](#c.usb_core.usb_hcd_giveback_urb "usb_core.usb_hcd_giveback_urb"). The HCD’s private spinlock must be held and
interrupts must be disabled. The actions carried out here are required
for URB completion.

void usb\_hcd\_giveback\_urb(struct usb\_hcd \*hcd, struct [urb](#c.usb_core.usb_hcd_giveback_urb "urb") \*urb, int status)
:   return URB from HCD to device driver

**Parameters**

`struct usb_hcd *hcd`
:   host controller returning the URB

`struct urb *urb`
:   urb being returned to the USB device driver.

`int status`
:   completion status code for the URB.

**Context**

atomic. The completion callback is invoked either in a work queue
(BH) context or in the caller’s context, depending on whether the HCD\_BH
flag is set in the **hcd** structure, except that URBs submitted to the
root hub always complete in BH context.

**Description**

This hands the URB from HCD to its USB device driver, using its
completion function. The HCD has freed all per-urb resources
(and is done using urb->hcpriv). It also released all HCD locks;
the device driver won’t cause problems if it frees, modifies,
or resubmits this URB.

If **urb** was unlinked, the value of **status** will be overridden by
**urb->unlinked**. Erroneous short transfers are detected in case
the HCD hasn’t checked for them.

int usb\_alloc\_streams(struct [usb\_interface](#c.usb_interface "usb_interface") \*interface, struct [usb\_host\_endpoint](#c.usb_host_endpoint "usb_host_endpoint") \*\*eps, unsigned int num\_eps, unsigned int num\_streams, gfp\_t mem\_flags)
:   allocate bulk endpoint stream IDs.

**Parameters**

`struct usb_interface *interface`
:   alternate setting that includes all endpoints.

`struct usb_host_endpoint **eps`
:   array of endpoints that need streams.

`unsigned int num_eps`
:   number of endpoints in the array.

`unsigned int num_streams`
:   number of streams to allocate.

`gfp_t mem_flags`
:   flags hcd should use to allocate memory.

**Description**

Sets up a group of bulk endpoints to have **num\_streams** stream IDs available.
Drivers may queue multiple transfers to different stream IDs, which may
complete in a different order than they were queued.

**Return**

On success, the number of allocated streams. On failure, a negative
error code.

int usb\_free\_streams(struct [usb\_interface](#c.usb_interface "usb_interface") \*interface, struct [usb\_host\_endpoint](#c.usb_host_endpoint "usb_host_endpoint") \*\*eps, unsigned int num\_eps, gfp\_t mem\_flags)
:   free bulk endpoint stream IDs.

**Parameters**

`struct usb_interface *interface`
:   alternate setting that includes all endpoints.

`struct usb_host_endpoint **eps`
:   array of endpoints to remove streams from.

`unsigned int num_eps`
:   number of endpoints in the array.

`gfp_t mem_flags`
:   flags hcd should use to allocate memory.

**Description**

Reverts a group of bulk endpoints back to not using stream IDs.
Can fail if we are given bad arguments, or HCD is broken.

**Return**

0 on success. On failure, a negative error code.

void usb\_hcd\_resume\_root\_hub(struct usb\_hcd \*hcd)
:   called by HCD to resume its root hub

**Parameters**

`struct usb_hcd *hcd`
:   host controller for this root hub

**Description**

The USB host controller calls this function when its root hub is
suspended (with the remote wakeup feature enabled) and a remote
wakeup request is received. The routine submits a workqueue request
to resume the root hub (that is, manage its downstream ports again).

int usb\_bus\_start\_enum(struct usb\_bus \*bus, unsigned port\_num)
:   start immediate enumeration (for OTG)

**Parameters**

`struct usb_bus *bus`
:   the bus (must use hcd framework)

`unsigned port_num`
:   1-based number of port; usually bus->otg\_port

**Context**

atomic

**Description**

Starts enumeration, with an immediate reset followed later by
hub\_wq identifying and possibly configuring the device.
This is needed by OTG controller drivers, where it helps meet
HNP protocol timing requirements for starting a port reset.

**Return**

0 if successful.

irqreturn\_t usb\_hcd\_irq(int irq, void \*\_\_hcd)
:   hook IRQs to HCD framework (bus glue)

**Parameters**

`int irq`
:   the IRQ being raised

`void *__hcd`
:   pointer to the HCD whose IRQ is being signaled

**Description**

If the controller isn’t HALTed, calls the driver’s irq handler.
Checks whether the controller is now dead.

**Return**

`IRQ_HANDLED` if the IRQ was handled. `IRQ_NONE` otherwise.

void usb\_hc\_died(struct usb\_hcd \*hcd)
:   report abnormal shutdown of a host controller (bus glue)

**Parameters**

`struct usb_hcd *hcd`
:   pointer to the HCD representing the controller

**Description**

This is called by bus glue to report a USB host controller that died
while operations may still have been pending. It’s called automatically
by the PCI glue, so only glue for non-PCI busses should need to call it.

Only call this function with the primary HCD.

struct usb\_hcd \*usb\_create\_shared\_hcd(const struct hc\_driver \*driver, struct [device](../infrastructure.html#c.device "device") \*dev, const char \*bus\_name, struct usb\_hcd \*primary\_hcd)
:   create and initialize an HCD structure

**Parameters**

`const struct hc_driver *driver`
:   HC driver that will use this hcd

`struct device *dev`
:   device for this HC, stored in hcd->self.controller

`const char *bus_name`
:   value to store in hcd->self.bus\_name

`struct usb_hcd *primary_hcd`
:   a pointer to the usb\_hcd structure that is sharing the
    PCI device. Only allocate certain resources for the primary HCD

**Context**

task context, might sleep.

**Description**

Allocate a `struct usb_hcd`, with extra space at the end for the
HC driver’s private data. Initialize the generic members of the
hcd structure.

**Return**

On success, a pointer to the created and initialized HCD structure.
On failure (e.g. if memory is unavailable), `NULL`.

struct usb\_hcd \*usb\_create\_hcd(const struct hc\_driver \*driver, struct [device](../infrastructure.html#c.device "device") \*dev, const char \*bus\_name)
:   create and initialize an HCD structure

**Parameters**

`const struct hc_driver *driver`
:   HC driver that will use this hcd

`struct device *dev`
:   device for this HC, stored in hcd->self.controller

`const char *bus_name`
:   value to store in hcd->self.bus\_name

**Context**

task context, might sleep.

**Description**

Allocate a `struct usb_hcd`, with extra space at the end for the
HC driver’s private data. Initialize the generic members of the
hcd structure.

**Return**

On success, a pointer to the created and initialized HCD
structure. On failure (e.g. if memory is unavailable), `NULL`.

int usb\_add\_hcd(struct usb\_hcd \*hcd, unsigned int irqnum, unsigned long irqflags)
:   finish generic HCD structure initialization and register

**Parameters**

`struct usb_hcd *hcd`
:   the usb\_hcd structure to initialize

`unsigned int irqnum`
:   Interrupt line to allocate

`unsigned long irqflags`
:   Interrupt type flags

**Description**

Finish the remaining parts of generic HCD initialization: allocate the
buffers of consistent memory, register the bus, request the IRQ line,
and call the driver’s `reset()` and `start()` routines.

void usb\_remove\_hcd(struct usb\_hcd \*hcd)
:   shutdown processing for generic HCDs

**Parameters**

`struct usb_hcd *hcd`
:   the usb\_hcd structure to remove

**Context**

task context, might sleep.

**Description**

Disconnects the root hub, then reverses the effects of [`usb_add_hcd()`](#c.usb_core.usb_add_hcd "usb_core.usb_add_hcd"),
invoking the HCD’s `stop()` method.

int usb\_hcd\_pci\_probe(struct pci\_dev \*dev, const struct hc\_driver \*driver)
:   initialize PCI-based HCDs

**Parameters**

`struct pci_dev *dev`
:   USB Host Controller being probed

`const struct hc_driver *driver`
:   USB HC driver handle

**Context**

task context, might sleep

**Description**

Allocates basic PCI resources for this USB host controller, and
then invokes the `start()` method for the HCD associated with it
through the hotplug entry’s driver\_data.

Store this function in the HCD’s `struct pci_driver` as `probe()`.

**Return**

0 if successful.

void usb\_hcd\_pci\_remove(struct pci\_dev \*dev)
:   shutdown processing for PCI-based HCDs

**Parameters**

`struct pci_dev *dev`
:   USB Host Controller being removed

**Context**

task context, might sleep

**Description**

Reverses the effect of [`usb_hcd_pci_probe()`](#c.usb_core.usb_hcd_pci_probe "usb_core.usb_hcd_pci_probe"), first invoking
the HCD’s `stop()` method. It is always called from a thread
context, normally “rmmod”, “apmd”, or something similar.

Store this function in the HCD’s `struct pci_driver` as `remove()`.

void usb\_hcd\_pci\_shutdown(struct pci\_dev \*dev)
:   shutdown host controller

**Parameters**

`struct pci_dev *dev`
:   USB Host Controller being shutdown

int hcd\_buffer\_create(struct usb\_hcd \*hcd)
:   initialize buffer pools

**Parameters**

`struct usb_hcd *hcd`
:   the bus whose buffer pools are to be initialized

**Context**

task context, might sleep

**Description**

Call this as part of initializing a host controller that uses the dma
memory allocators. It initializes some pools of dma-coherent memory that
will be shared by all drivers using that controller.

Call [`hcd_buffer_destroy()`](#c.usb_core.hcd_buffer_destroy "usb_core.hcd_buffer_destroy") to clean up after using those pools.

**Return**

0 if successful. A negative errno value otherwise.

void hcd\_buffer\_destroy(struct usb\_hcd \*hcd)
:   deallocate buffer pools

**Parameters**

`struct usb_hcd *hcd`
:   the bus whose buffer pools are to be destroyed

**Context**

task context, might sleep

**Description**

This frees the buffer pools created by [`hcd_buffer_create()`](#c.usb_core.hcd_buffer_create "usb_core.hcd_buffer_create").

## The USB character device nodes

This chapter presents the Linux character device nodes. You may prefer
to avoid writing new kernel code for your USB driver. User mode device
drivers are usually packaged as applications or libraries, and may use
character devices through some programming library that wraps it.
Such libraries include:

> * [libusb](http://libusb.sourceforge.net) for C/C++, and
> * [jUSB](http://jUSB.sourceforge.net) for Java.

Some old information about it can be seen at the “USB Device Filesystem”
section of the USB Guide. The latest copy of the USB Guide can be found
at <http://www.linux-usb.org/>

Note

* They were used to be implemented via *usbfs*, but this is not part of
  the sysfs debug interface.

> * This particular documentation is incomplete, especially with respect
>   to the asynchronous mode. As of kernel 2.5.66 the code and this
>   (new) documentation need to be cross-reviewed.

### What files are in “devtmpfs”?

Conventionally mounted at `/dev/bus/usb/`, usbfs features include:

* `/dev/bus/usb/BBB/DDD` ... magic files exposing the each device’s
  configuration descriptors, and supporting a series of ioctls for
  making device requests, including I/O to devices. (Purely for access
  by programs.)

Each bus is given a number (`BBB`) based on when it was enumerated; within
each bus, each device is given a similar number (`DDD`). Those `BBB/DDD`
paths are not “stable” identifiers; expect them to change even if you
always leave the devices plugged in to the same hub port. *Don’t even
think of saving these in application configuration files.* Stable
identifiers are available, for user mode applications that want to use
them. HID and networking devices expose these stable IDs, so that for
example you can be sure that you told the right UPS to power down its
second server. Pleast note that it doesn’t (yet) expose those IDs.

### /dev/bus/usb/BBB/DDD

Use these files in one of these basic ways:

* *They can be read,* producing first the device descriptor (18 bytes) and
  then the descriptors for the current configuration. See the USB 2.0 spec
  for details about those binary data formats. You’ll need to convert most
  multibyte values from little endian format to your native host byte
  order, although a few of the fields in the device descriptor (both of
  the BCD-encoded fields, and the vendor and product IDs) will be
  byteswapped for you. Note that configuration descriptors include
  descriptors for interfaces, altsettings, endpoints, and maybe additional
  class descriptors.
* *Perform USB operations* using *`ioctl()`* requests to make endpoint I/O
  requests (synchronously or asynchronously) or manage the device. These
  requests need the `CAP_SYS_RAWIO` capability, as well as filesystem
  access permissions. Only one ioctl request can be made on one of these
  device files at a time. This means that if you are synchronously reading
  an endpoint from one thread, you won’t be able to write to a different
  endpoint from another thread until the read completes. This works for
  *half duplex* protocols, but otherwise you’d use asynchronous i/o
  requests.

Each connected USB device has one file. The `BBB` indicates the bus
number. The `DDD` indicates the device address on that bus. Both
of these numbers are assigned sequentially, and can be reused, so
you can’t rely on them for stable access to devices. For example,
it’s relatively common for devices to re-enumerate while they are
still connected (perhaps someone jostled their power supply, hub,
or USB cable), so a device might be `002/027` when you first connect
it and `002/048` sometime later.

These files can be read as binary data. The binary data consists
of first the device descriptor, then the descriptors for each
configuration of the device. Multi-byte fields in the device descriptor
are converted to host endianness by the kernel. The configuration
descriptors are in bus endian format! The configuration descriptor
are wTotalLength bytes apart. If a device returns less configuration
descriptor data than indicated by wTotalLength there will be a hole in
the file for the missing bytes. This information is also shown
in text form by the `/sys/kernel/debug/usb/devices` file, described later.

These files may also be used to write user-level drivers for the USB
devices. You would open the `/dev/bus/usb/BBB/DDD` file read/write,
read its descriptors to make sure it’s the device you expect, and then
bind to an interface (or perhaps several) using an ioctl call. You
would issue more ioctls to the device to communicate to it using
control, bulk, or other kinds of USB transfers. The IOCTLs are
listed in the `<linux/usbdevice_fs.h>` file, and at this writing the
source code (`linux/drivers/usb/core/devio.c`) is the primary reference
for how to access devices through those files.

Note that since by default these `BBB/DDD` files are writable only by
root, only root can write such user mode drivers. You can selectively
grant read/write permissions to other users by using `chmod`. Also,
usbfs mount options such as `devmode=0666` may be helpful.

### Life Cycle of User Mode Drivers

Such a driver first needs to find a device file for a device it knows
how to handle. Maybe it was told about it because a `/sbin/hotplug`
event handling agent chose that driver to handle the new device. Or
maybe it’s an application that scans all the `/dev/bus/usb` device files,
and ignores most devices. In either case, it should `read()`
all the descriptors from the device file, and check them against what it
knows how to handle. It might just reject everything except a particular
vendor and product ID, or need a more complex policy.

Never assume there will only be one such device on the system at a time!
If your code can’t handle more than one device at a time, at least
detect when there’s more than one, and have your users choose which
device to use.

Once your user mode driver knows what device to use, it interacts with
it in either of two styles. The simple style is to make only control
requests; some devices don’t need more complex interactions than those.
(An example might be software using vendor-specific control requests for
some initialization or configuration tasks, with a kernel driver for the
rest.)

More likely, you need a more complex style driver: one using non-control
endpoints, reading or writing data and claiming exclusive use of an
interface. *Bulk* transfers are easiest to use, but only their sibling
*interrupt* transfers work with low speed devices. Both interrupt and
*isochronous* transfers offer service guarantees because their bandwidth
is reserved. Such “periodic” transfers are awkward to use through usbfs,
unless you’re using the asynchronous calls. However, interrupt transfers
can also be used in a synchronous “one shot” style.

Your user-mode driver should never need to worry about cleaning up
request state when the device is disconnected, although it should close
its open file descriptors as soon as it starts seeing the ENODEV errors.

### The ioctl() Requests

To use these ioctls, you need to include the following headers in your
userspace program:

```
#include <linux/usb.h>
#include <linux/usbdevice_fs.h>
#include <asm/byteorder.h>
```

The standard USB device model requests, from “Chapter 9” of the USB 2.0
specification, are automatically included from the `<linux/usb/ch9.h>`
header.

Unless noted otherwise, the ioctl requests described here will update
the modification time on the usbfs file to which they are applied
(unless they fail). A return of zero indicates success; otherwise, a
standard USB error code is returned (These are documented in
[USB Error codes](error-codes.html#usb-error-codes)).

Each of these files multiplexes access to several I/O streams, one per
endpoint. Each device has one control endpoint (endpoint zero) which
supports a limited RPC style RPC access. Devices are configured by
hub\_wq (in the kernel) setting a device-wide *configuration* that
affects things like power consumption and basic functionality. The
endpoints are part of USB *interfaces*, which may have *altsettings*
affecting things like which endpoints are available. Many devices only
have a single configuration and interface, so drivers for them will
ignore configurations and altsettings.

#### Management/Status Requests

A number of usbfs requests don’t deal very directly with device I/O.
They mostly relate to device management and status. These are all
synchronous requests.

USBDEVFS\_CLAIMINTERFACE
:   This is used to force usbfs to claim a specific interface, which has
    not previously been claimed by usbfs or any other kernel driver. The
    ioctl parameter is an integer holding the number of the interface
    (bInterfaceNumber from descriptor).

    Note that if your driver doesn’t claim an interface before trying to
    use one of its endpoints, and no other driver has bound to it, then
    the interface is automatically claimed by usbfs.

    This claim will be released by a RELEASEINTERFACE ioctl, or by
    closing the file descriptor. File modification time is not updated
    by this request.

USBDEVFS\_CONNECTINFO
:   Says whether the device is lowspeed. The ioctl parameter points to a
    structure like this:

    ```
    struct usbdevfs_connectinfo {
            unsigned int   devnum;
            unsigned char  slow;
    };
    ```

    File modification time is not updated by this request.

    *You can’t tell whether a “not slow” device is connected at high
    speed (480 MBit/sec) or just full speed (12 MBit/sec).* You should
    know the devnum value already, it’s the DDD value of the device file
    name.

USBDEVFS\_GET\_SPEED
:   Returns the speed of the device. The speed is returned as a
    numerical value in accordance with `enum usb_device_speed`

    File modification time is not updated by this request.

USBDEVFS\_GETDRIVER
:   Returns the name of the kernel driver bound to a given interface (a
    string). Parameter is a pointer to this structure, which is
    modified:

    ```
    struct usbdevfs_getdriver {
            unsigned int  interface;
            char          driver[USBDEVFS_MAXDRIVERNAME + 1];
    };
    ```

    File modification time is not updated by this request.

USBDEVFS\_IOCTL
:   Passes a request from userspace through to a kernel driver that has
    an ioctl entry in the *`struct usb_driver`* it registered:

    ```
    struct usbdevfs_ioctl {
            int     ifno;
            int     ioctl_code;
            void    *data;
    };

    /* user mode call looks like this.
     * 'request' becomes the driver->ioctl() 'code' parameter.
     * the size of 'param' is encoded in 'request', and that data
     * is copied to or from the driver->ioctl() 'buf' parameter.
     */
    static int
    usbdev_ioctl (int fd, int ifno, unsigned request, void *param)
    {
            struct usbdevfs_ioctl   wrapper;

            wrapper.ifno = ifno;
            wrapper.ioctl_code = request;
            wrapper.data = param;

            return ioctl (fd, USBDEVFS_IOCTL, &wrapper);
    }
    ```

    File modification time is not updated by this request.

    This request lets kernel drivers talk to user mode code through
    filesystem operations even when they don’t create a character or
    block special device. It’s also been used to do things like ask
    devices what device special file should be used. Two pre-defined
    ioctls are used to disconnect and reconnect kernel drivers, so that
    user mode code can completely manage binding and configuration of
    devices.

USBDEVFS\_RELEASEINTERFACE
:   This is used to release the claim usbfs made on interface, either
    implicitly or because of a USBDEVFS\_CLAIMINTERFACE call, before the
    file descriptor is closed. The ioctl parameter is an integer holding
    the number of the interface (bInterfaceNumber from descriptor); File
    modification time is not updated by this request.

    Warning

    *No security check is made to ensure that the task which made
    the claim is the one which is releasing it. This means that user
    mode driver may interfere other ones.*

USBDEVFS\_RESETEP
:   Resets the data toggle value for an endpoint (bulk or interrupt) to
    DATA0. The ioctl parameter is an integer endpoint number (1 to 15,
    as identified in the endpoint descriptor), with USB\_DIR\_IN added
    if the device’s endpoint sends data to the host.

    Warning

    *Avoid using this request. It should probably be removed.* Using
    it typically means the device and driver will lose toggle
    synchronization. If you really lost synchronization, you likely
    need to completely handshake with the device, using a request
    like CLEAR\_HALT or SET\_INTERFACE.

USBDEVFS\_DROP\_PRIVILEGES
:   This is used to relinquish the ability to do certain operations
    which are considered to be privileged on a usbfs file descriptor.
    This includes claiming arbitrary interfaces, resetting a device on
    which there are currently claimed interfaces from other users, and
    issuing USBDEVFS\_IOCTL calls. The ioctl parameter is a 32 bit mask
    of interfaces the user is allowed to claim on this file descriptor.
    You may issue this ioctl more than one time to narrow said mask.

#### Synchronous I/O Support

Synchronous requests involve the kernel blocking until the user mode
request completes, either by finishing successfully or by reporting an
error. In most cases this is the simplest way to use usbfs, although as
noted above it does prevent performing I/O to more than one endpoint at
a time.

USBDEVFS\_BULK
:   Issues a bulk read or write request to the device. The ioctl
    parameter is a pointer to this structure:

    ```
    struct usbdevfs_bulktransfer {
            unsigned int  ep;
            unsigned int  len;
            unsigned int  timeout; /* in milliseconds */
            void          *data;
    };
    ```

    The `ep` value identifies a bulk endpoint number (1 to 15, as
    identified in an endpoint descriptor), masked with USB\_DIR\_IN when
    referring to an endpoint which sends data to the host from the
    device. The length of the data buffer is identified by `len`; Recent
    kernels support requests up to about 128KBytes. *FIXME say how read
    length is returned, and how short reads are handled.*.

USBDEVFS\_CLEAR\_HALT
:   Clears endpoint halt (stall) and resets the endpoint toggle. This is
    only meaningful for bulk or interrupt endpoints. The ioctl parameter
    is an integer endpoint number (1 to 15, as identified in an endpoint
    descriptor), masked with USB\_DIR\_IN when referring to an endpoint
    which sends data to the host from the device.

    Use this on bulk or interrupt endpoints which have stalled,
    returning `-EPIPE` status to a data transfer request. Do not issue
    the control request directly, since that could invalidate the host’s
    record of the data toggle.

USBDEVFS\_CONTROL
:   Issues a control request to the device. The ioctl parameter points
    to a structure like this:

    ```
    struct usbdevfs_ctrltransfer {
            __u8   bRequestType;
            __u8   bRequest;
            __u16  wValue;
            __u16  wIndex;
            __u16  wLength;
            __u32  timeout;  /* in milliseconds */
            void   *data;
    };
    ```

    The first eight bytes of this structure are the contents of the
    SETUP packet to be sent to the device; see the USB 2.0 specification
    for details. The bRequestType value is composed by combining a
    `USB_TYPE_*` value, a `USB_DIR_*` value, and a `USB_RECIP_*`
    value (from `linux/usb.h`). If wLength is nonzero, it describes
    the length of the data buffer, which is either written to the device
    (USB\_DIR\_OUT) or read from the device (USB\_DIR\_IN).

    At this writing, you can’t transfer more than 4 KBytes of data to or
    from a device; usbfs has a limit, and some host controller drivers
    have a limit. (That’s not usually a problem.) *Also* there’s no way
    to say it’s not OK to get a short read back from the device.

USBDEVFS\_RESET
:   Does a USB level device reset. The ioctl parameter is ignored. After
    the reset, this rebinds all device interfaces. File modification
    time is not updated by this request.

Warning

*Avoid using this call* until some usbcore bugs get fixed, since
it does not fully synchronize device, interface, and driver (not
just usbfs) state.

USBDEVFS\_SETINTERFACE
:   Sets the alternate setting for an interface. The ioctl parameter is
    a pointer to a structure like this:

    ```
    struct usbdevfs_setinterface {
            unsigned int  interface;
            unsigned int  altsetting;
    };
    ```

    File modification time is not updated by this request.

    Those `struct members` are from some interface descriptor applying to
    the current configuration. The interface number is the
    bInterfaceNumber value, and the altsetting number is the
    bAlternateSetting value. (This resets each endpoint in the
    interface.)

USBDEVFS\_SETCONFIGURATION
:   Issues the `usb_set_configuration()` call for the
    device. The parameter is an integer holding the number of a
    configuration (bConfigurationValue from descriptor). File
    modification time is not updated by this request.

Warning

*Avoid using this call* until some usbcore bugs get fixed, since
it does not fully synchronize device, interface, and driver (not
just usbfs) state.

#### Asynchronous I/O Support

As mentioned above, there are situations where it may be important to
initiate concurrent operations from user mode code. This is particularly
important for periodic transfers (interrupt and isochronous), but it can
be used for other kinds of USB requests too. In such cases, the
asynchronous requests described here are essential. Rather than
submitting one request and having the kernel block until it completes,
the blocking is separate.

These requests are packaged into a structure that resembles the URB used
by kernel device drivers. (No POSIX Async I/O support here, sorry.) It
identifies the endpoint type (`USBDEVFS_URB_TYPE_*`), endpoint
(number, masked with USB\_DIR\_IN as appropriate), buffer and length,
and a user “context” value serving to uniquely identify each request.
(It’s usually a pointer to per-request data.) Flags can modify requests
(not as many as supported for kernel drivers).

Each request can specify a realtime signal number (between SIGRTMIN and
SIGRTMAX, inclusive) to request a signal be sent when the request
completes.

When usbfs returns these urbs, the status value is updated, and the
buffer may have been modified. Except for isochronous transfers, the
actual\_length is updated to say how many bytes were transferred; if the
USBDEVFS\_URB\_DISABLE\_SPD flag is set (“short packets are not OK”), if
fewer bytes were read than were requested then you get an error report:

```
struct usbdevfs_iso_packet_desc {
        unsigned int                     length;
        unsigned int                     actual_length;
        unsigned int                     status;
};

struct usbdevfs_urb {
        unsigned char                    type;
        unsigned char                    endpoint;
        int                              status;
        unsigned int                     flags;
        void                             *buffer;
        int                              buffer_length;
        int                              actual_length;
        int                              start_frame;
        int                              number_of_packets;
        int                              error_count;
        unsigned int                     signr;
        void                             *usercontext;
        struct usbdevfs_iso_packet_desc  iso_frame_desc[];
};
```

For these asynchronous requests, the file modification time reflects
when the request was initiated. This contrasts with their use with the
synchronous requests, where it reflects when requests complete.

USBDEVFS\_DISCARDURB
:   *TBS* File modification time is not updated by this request.

USBDEVFS\_DISCSIGNAL
:   *TBS* File modification time is not updated by this request.

USBDEVFS\_REAPURB
:   *TBS* File modification time is not updated by this request.

USBDEVFS\_REAPURBNDELAY
:   *TBS* File modification time is not updated by this request.

USBDEVFS\_SUBMITURB
:   *TBS*

## The USB devices

The USB devices are now exported via debugfs:

* `/sys/kernel/debug/usb/devices` ... a text file showing each of the USB
  devices on known to the kernel, and their configuration descriptors.
  You can also `poll()` this to learn about new devices.

### /sys/kernel/debug/usb/devices

This file is handy for status viewing tools in user mode, which can scan
the text format and ignore most of it. More detailed device status
(including class and vendor status) is available from device-specific
files. For information about the current format of this file, see below.

This file, in combination with the `poll()` system call, can also be used
to detect when devices are added or removed:

```
int fd;
struct pollfd pfd;

fd = open("/sys/kernel/debug/usb/devices", O_RDONLY);
pfd = { fd, POLLIN, 0 };
for (;;) {
    /* The first time through, this call will return immediately. */
    poll(&pfd, 1, -1);

    /* To see what's changed, compare the file's previous and current
       contents or scan the filesystem.  (Scanning is more precise.) */
}
```

Note that this behavior is intended to be used for informational and
debug purposes. It would be more appropriate to use programs such as
udev or HAL to initialize a device or start a user-mode helper program,
for instance.

In this file, each device’s output has multiple lines of ASCII output.

I made it ASCII instead of binary on purpose, so that someone
can obtain some useful data from it without the use of an
auxiliary program. However, with an auxiliary program, the numbers
in the first 4 columns of each `T:` line (topology info:
Lev, Prnt, Port, Cnt) can be used to build a USB topology diagram.

Each line is tagged with a one-character ID for that line:

```
T = Topology (etc.)
B = Bandwidth (applies only to USB host controllers, which are
virtualized as root hubs)
D = Device descriptor info.
P = Product ID info. (from Device descriptor, but they won't fit
together on one line)
S = String descriptors.
C = Configuration descriptor info. (* = active configuration)
I = Interface descriptor info.
E = Endpoint descriptor info.
```

#### /sys/kernel/debug/usb/devices output format

Legend::
:   d = decimal number (may have leading spaces or 0’s)
    x = hexadecimal number (may have leading spaces or 0’s)
    s = string

##### Topology info

```
T:  Bus=dd Lev=dd Prnt=dd Port=dd Cnt=dd Dev#=ddd Spd=dddd MxCh=dd
|   |      |      |       |       |      |        |        |__MaxChildren
|   |      |      |       |       |      |        |__Device Speed in Mbps
|   |      |      |       |       |      |__DeviceNumber
|   |      |      |       |       |__Count of devices at this level
|   |      |      |       |__Connector/Port on Parent for this device
|   |      |      |__Parent DeviceNumber
|   |      |__Level in topology for this bus
|   |__Bus number
|__Topology info tag
```

Speed may be:

> |  |  |
> | --- | --- |
> | 1.5 | Mbit/s for low speed USB |
> | 12 | Mbit/s for full speed USB |
> | 480 | Mbit/s for high speed USB (added for USB 2.0) |
> | 5000 | Mbit/s for SuperSpeed USB (added for USB 3.0) |

For reasons lost in the mists of time, the Port number is always
too low by 1. For example, a device plugged into port 4 will
show up with `Port=03`.

##### Bandwidth info

```
B:  Alloc=ddd/ddd us (xx%), #Int=ddd, #Iso=ddd
|   |                       |         |__Number of isochronous requests
|   |                       |__Number of interrupt requests
|   |__Total Bandwidth allocated to this bus
|__Bandwidth info tag
```

Bandwidth allocation is an approximation of how much of one frame
(millisecond) is in use. It reflects only periodic transfers, which
are the only transfers that reserve bandwidth. Control and bulk
transfers use all other bandwidth, including reserved bandwidth that
is not used for transfers (such as for short packets).

The percentage is how much of the “reserved” bandwidth is scheduled by
those transfers. For a low or full speed bus (loosely, “USB 1.1”),
90% of the bus bandwidth is reserved. For a high speed bus (loosely,
“USB 2.0”) 80% is reserved.

##### Device descriptor info & Product ID info

```
D:  Ver=x.xx Cls=xx(s) Sub=xx Prot=xx MxPS=dd #Cfgs=dd
P:  Vendor=xxxx ProdID=xxxx Rev=xx.xx
```

where:

```
D:  Ver=x.xx Cls=xx(sssss) Sub=xx Prot=xx MxPS=dd #Cfgs=dd
|   |        |             |      |       |       |__NumberConfigurations
|   |        |             |      |       |__MaxPacketSize of Default Endpoint
|   |        |             |      |__DeviceProtocol
|   |        |             |__DeviceSubClass
|   |        |__DeviceClass
|   |__Device USB version
|__Device info tag #1
```

where:

```
P:  Vendor=xxxx ProdID=xxxx Rev=xx.xx
|   |           |           |__Product revision number
|   |           |__Product ID code
|   |__Vendor ID code
|__Device info tag #2
```

##### String descriptor info

```
S:  Manufacturer=ssss
|   |__Manufacturer of this device as read from the device.
|      For USB host controller drivers (virtual root hubs) this may
|      be omitted, or (for newer drivers) will identify the kernel
|      version and the driver which provides this hub emulation.
|__String info tag

S:  Product=ssss
|   |__Product description of this device as read from the device.
|      For older USB host controller drivers (virtual root hubs) this
|      indicates the driver; for newer ones, it's a product (and vendor)
|      description that often comes from the kernel's PCI ID database.
|__String info tag

S:  SerialNumber=ssss
|   |__Serial Number of this device as read from the device.
|      For USB host controller drivers (virtual root hubs) this is
|      some unique ID, normally a bus ID (address or slot name) that
|      can't be shared with any other device.
|__String info tag
```

##### Configuration descriptor info

```
C:* #Ifs=dd Cfg#=dd Atr=xx MPwr=dddmA
| | |       |       |      |__MaxPower in mA
| | |       |       |__Attributes
| | |       |__ConfiguratioNumber
| | |__NumberOfInterfaces
| |__ "*" indicates the active configuration (others are " ")
|__Config info tag
```

USB devices may have multiple configurations, each of which act
rather differently. For example, a bus-powered configuration
might be much less capable than one that is self-powered. Only
one device configuration can be active at a time; most devices
have only one configuration.

Each configuration consists of one or more interfaces. Each
interface serves a distinct “function”, which is typically bound
to a different USB device driver. One common example is a USB
speaker with an audio interface for playback, and a HID interface
for use with software volume control.

##### Interface descriptor info (can be multiple per Config)

```
I:* If#=dd Alt=dd #EPs=dd Cls=xx(sssss) Sub=xx Prot=xx Driver=ssss
| | |      |      |       |             |      |       |__Driver name
| | |      |      |       |             |      |          or "(none)"
| | |      |      |       |             |      |__InterfaceProtocol
| | |      |      |       |             |__InterfaceSubClass
| | |      |      |       |__InterfaceClass
| | |      |      |__NumberOfEndpoints
| | |      |__AlternateSettingNumber
| | |__InterfaceNumber
| |__ "*" indicates the active altsetting (others are " ")
|__Interface info tag
```

A given interface may have one or more “alternate” settings.
For example, default settings may not use more than a small
amount of periodic bandwidth. To use significant fractions
of bus bandwidth, drivers must select a non-default altsetting.

Only one setting for an interface may be active at a time, and
only one driver may bind to an interface at a time. Most devices
have only one alternate setting per interface.

##### Endpoint descriptor info (can be multiple per Interface)

```
E:  Ad=xx(s) Atr=xx(ssss) MxPS=dddd Ivl=dddss
|   |        |            |         |__Interval (max) between transfers
|   |        |            |__EndpointMaxPacketSize
|   |        |__Attributes(EndpointType)
|   |__EndpointAddress(I=In,O=Out)
|__Endpoint info tag
```

The interval is nonzero for all periodic (interrupt or isochronous)
endpoints. For high speed endpoints the transfer interval may be
measured in microseconds rather than milliseconds.

For high speed periodic endpoints, the `EndpointMaxPacketSize` reflects
the per-microframe data transfer size. For “high bandwidth”
endpoints, that can reflect two or three packets (for up to
3KBytes every 125 usec) per endpoint.

With the Linux-USB stack, periodic bandwidth reservations use the
transfer intervals and sizes provided by URBs, which can be less
than those found in endpoint descriptor.

#### Usage examples

If a user or script is interested only in Topology info, for
example, use something like `grep ^T: /sys/kernel/debug/usb/devices`
for only the Topology lines. A command like
`grep -i ^[tdp]: /sys/kernel/debug/usb/devices` can be used to list
only the lines that begin with the characters in square brackets,
where the valid characters are TDPCIE. With a slightly more able
script, it can display any selected lines (for example, only T, D,
and P lines) and change their output format. (The `procusb`
Perl script is the beginning of this idea. It will list only
selected lines [selected from TBDPSCIE] or “All” lines from
`/sys/kernel/debug/usb/devices`.)

The Topology lines can be used to generate a graphic/pictorial
of the USB devices on a system’s root hub. (See more below
on how to do this.)

The Interface lines can be used to determine what driver is
being used for each device, and which altsetting it activated.

The Configuration lines could be used to list maximum power
(in milliamps) that a system’s USB devices are using.
For example, `grep ^C: /sys/kernel/debug/usb/devices`.

Here’s an example, from a system which has a UHCI root hub,
an external hub connected to the root hub, and a mouse and
a serial converter connected to the external hub.

```
T:  Bus=00 Lev=00 Prnt=00 Port=00 Cnt=00 Dev#=  1 Spd=12   MxCh= 2
B:  Alloc= 28/900 us ( 3%), #Int=  2, #Iso=  0
D:  Ver= 1.00 Cls=09(hub  ) Sub=00 Prot=00 MxPS= 8 #Cfgs=  1
P:  Vendor=0000 ProdID=0000 Rev= 0.00
S:  Product=USB UHCI Root Hub
S:  SerialNumber=dce0
C:* #Ifs= 1 Cfg#= 1 Atr=40 MxPwr=  0mA
I:  If#= 0 Alt= 0 #EPs= 1 Cls=09(hub  ) Sub=00 Prot=00 Driver=hub
E:  Ad=81(I) Atr=03(Int.) MxPS=   8 Ivl=255ms

T:  Bus=00 Lev=01 Prnt=01 Port=00 Cnt=01 Dev#=  2 Spd=12   MxCh= 4
D:  Ver= 1.00 Cls=09(hub  ) Sub=00 Prot=00 MxPS= 8 #Cfgs=  1
P:  Vendor=0451 ProdID=1446 Rev= 1.00
C:* #Ifs= 1 Cfg#= 1 Atr=e0 MxPwr=100mA
I:  If#= 0 Alt= 0 #EPs= 1 Cls=09(hub  ) Sub=00 Prot=00 Driver=hub
E:  Ad=81(I) Atr=03(Int.) MxPS=   1 Ivl=255ms

T:  Bus=00 Lev=02 Prnt=02 Port=00 Cnt=01 Dev#=  3 Spd=1.5  MxCh= 0
D:  Ver= 1.00 Cls=00(>ifc ) Sub=00 Prot=00 MxPS= 8 #Cfgs=  1
P:  Vendor=04b4 ProdID=0001 Rev= 0.00
C:* #Ifs= 1 Cfg#= 1 Atr=80 MxPwr=100mA
I:  If#= 0 Alt= 0 #EPs= 1 Cls=03(HID  ) Sub=01 Prot=02 Driver=mouse
E:  Ad=81(I) Atr=03(Int.) MxPS=   3 Ivl= 10ms

T:  Bus=00 Lev=02 Prnt=02 Port=02 Cnt=02 Dev#=  4 Spd=12   MxCh= 0
D:  Ver= 1.00 Cls=00(>ifc ) Sub=00 Prot=00 MxPS= 8 #Cfgs=  1
P:  Vendor=0565 ProdID=0001 Rev= 1.08
S:  Manufacturer=Peracom Networks, Inc.
S:  Product=Peracom USB to Serial Converter
C:* #Ifs= 1 Cfg#= 1 Atr=a0 MxPwr=100mA
I:  If#= 0 Alt= 0 #EPs= 3 Cls=00(>ifc ) Sub=00 Prot=00 Driver=serial
E:  Ad=81(I) Atr=02(Bulk) MxPS=  64 Ivl= 16ms
E:  Ad=01(O) Atr=02(Bulk) MxPS=  16 Ivl= 16ms
E:  Ad=82(I) Atr=03(Int.) MxPS=   8 Ivl=  8ms
```

Selecting only the `T:` and `I:` lines from this (for example, by using
`procusb ti`), we have

```
T:  Bus=00 Lev=00 Prnt=00 Port=00 Cnt=00 Dev#=  1 Spd=12   MxCh= 2
T:  Bus=00 Lev=01 Prnt=01 Port=00 Cnt=01 Dev#=  2 Spd=12   MxCh= 4
I:  If#= 0 Alt= 0 #EPs= 1 Cls=09(hub  ) Sub=00 Prot=00 Driver=hub
T:  Bus=00 Lev=02 Prnt=02 Port=00 Cnt=01 Dev#=  3 Spd=1.5  MxCh= 0
I:  If#= 0 Alt= 0 #EPs= 1 Cls=03(HID  ) Sub=01 Prot=02 Driver=mouse
T:  Bus=00 Lev=02 Prnt=02 Port=02 Cnt=02 Dev#=  4 Spd=12   MxCh= 0
I:  If#= 0 Alt= 0 #EPs= 3 Cls=00(>ifc ) Sub=00 Prot=00 Driver=serial
```

Physically this looks like (or could be converted to):

```
                    +------------------+
                    |  PC/root_hub (12)|   Dev# = 1
                    +------------------+   (nn) is Mbps.
  Level 0           |  CN.0   |  CN.1  |   [CN = connector/port #]
                    +------------------+
                        /
                       /
          +-----------------------+
Level 1   | Dev#2: 4-port hub (12)|
          +-----------------------+
          |CN.0 |CN.1 |CN.2 |CN.3 |
          +-----------------------+
              \           \____________________
               \_____                          \
                     \                          \
             +--------------------+      +--------------------+
Level 2      | Dev# 3: mouse (1.5)|      | Dev# 4: serial (12)|
             +--------------------+      +--------------------+
```

Or, in a more tree-like structure (ports [Connectors] without
connections could be omitted):

```
PC:  Dev# 1, root hub, 2 ports, 12 Mbps
|_ CN.0:  Dev# 2, hub, 4 ports, 12 Mbps
     |_ CN.0:  Dev #3, mouse, 1.5 Mbps
     |_ CN.1:
     |_ CN.2:  Dev #4, serial, 12 Mbps
     |_ CN.3:
|_ CN.1:
```
