# USB Type-C connector class

> 출처(원문): https://docs.kernel.org/driver-api/usb/typec.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# USB Type-C connector class

## Introduction

The typec class is meant for describing the USB Type-C ports in a system to the
user space in unified fashion. The class is designed to provide nothing else
except the user space interface implementation in hope that it can be utilized
on as many platforms as possible.

The platforms are expected to register every USB Type-C port they have with the
class. In a normal case the registration will be done by a USB Type-C or PD PHY
driver, but it may be a driver for firmware interface such as UCSI, driver for
USB PD controller or even driver for Thunderbolt3 controller. This document
considers the component registering the USB Type-C ports with the class as “port
driver”.

On top of showing the capabilities, the class also offer user space control over
the roles and alternate modes of ports, partners and cable plugs when the port
driver is capable of supporting those features.

The class provides an API for the port drivers described in this document. The
attributes are described in [ABI file testing/sysfs-class-typec](../../admin-guide/abi-testing-files.html#abi-file-testing-sysfs-class-typec).

## User space interface

Every port will be presented as its own device under /sys/class/typec/. The
first port will be named “port0”, the second “port1” and so on.

When connected, the partner will be presented also as its own device under
/sys/class/typec/. The parent of the partner device will always be the port it
is attached to. The partner attached to port “port0” will be named
“port0-partner”. Full path to the device would be
/sys/class/typec/port0/port0-partner/.

The cable and the two plugs on it may also be optionally presented as their own
devices under /sys/class/typec/. The cable attached to the port “port0” port
will be named port0-cable and the plug on the SOP Prime end (see USB Power
Delivery Specification ch. 2.4) will be named “port0-plug0” and on the SOP
Double Prime end “port0-plug1”. The parent of a cable will always be the port,
and the parent of the cable plugs will always be the cable.

If the port, partner or cable plug supports Alternate Modes, every supported
Alternate Mode SVID will have their own device describing them. Note that the
Alternate Mode devices will not be attached to the typec class. The parent of an
alternate mode will be the device that supports it, so for example an alternate
mode of port0-partner will be presented under /sys/class/typec/port0-partner/.
Every mode that is supported will have its own group under the Alternate Mode
device named “mode<index>”, for example /sys/class/typec/port0/<alternate
mode>/mode1/. The requests for entering/exiting a mode can be done with “active”
attribute file in that group.

## Driver API

### Registering the ports

The port drivers will describe every Type-C port they control with `struct
typec_capability` data structure, and register them with the following API:

struct typec\_port \*typec\_register\_port(struct [device](../infrastructure.html#c.device "device") \*parent, const struct typec\_capability \*cap)
:   Register a USB Type-C Port

**Parameters**

`struct device *parent`
:   Parent device

`const struct typec_capability *cap`
:   Description of the port

**Description**

Registers a device for USB Type-C Port described in **cap**.

Returns handle to the port on success or ERR\_PTR on failure.

void typec\_unregister\_port(struct typec\_port \*port)
:   Unregister a USB Type-C Port

**Parameters**

`struct typec_port *port`
:   The port to be unregistered

**Description**

Unregister device created with [`typec_register_port()`](#c.typec_register_port "typec_register_port").

When registering the ports, the prefer\_role member in `struct typec_capability`
deserves special notice. If the port that is being registered does not have
initial role preference, which means the port does not execute Try.SNK or
Try.SRC by default, the member must have value TYPEC\_NO\_PREFERRED\_ROLE.
Otherwise if the port executes Try.SNK by default, the member must have value
TYPEC\_DEVICE, and with Try.SRC the value must be TYPEC\_HOST.

### Registering Partners

After successful connection of a partner, the port driver needs to register the
partner with the class. Details about the partner need to be described in `struct
typec_partner_desc`. The class copies the details of the partner during
registration. The class offers the following API for registering/unregistering
partners.

struct typec\_partner \*typec\_register\_partner(struct typec\_port \*port, struct typec\_partner\_desc \*desc)
:   Register a USB Type-C Partner

**Parameters**

`struct typec_port *port`
:   The USB Type-C Port the partner is connected to

`struct typec_partner_desc *desc`
:   Description of the partner

**Description**

Registers a device for USB Type-C Partner described in **desc**.

Returns handle to the partner on success or ERR\_PTR on failure.

void typec\_unregister\_partner(struct typec\_partner \*partner)
:   Unregister a USB Type-C Partner

**Parameters**

`struct typec_partner *partner`
:   The partner to be unregistered

**Description**

Unregister device created with [`typec_register_partner()`](#c.typec_register_partner "typec_register_partner").

The class will provide a handle to `struct typec_partner` if the registration was
successful, or NULL.

If the partner is USB Power Delivery capable, and the port driver is able to
show the result of Discover Identity command, the partner descriptor structure
should include handle to `struct usb_pd_identity` instance. The class will then
create a sysfs directory for the identity under the partner device. The result
of Discover Identity command can then be reported with the following API:

int typec\_partner\_set\_identity(struct typec\_partner \*partner)
:   Report result from Discover Identity command

**Parameters**

`struct typec_partner *partner`
:   The partner updated identity values

**Description**

This routine is used to report that the result of Discover Identity USB power
delivery command has become available.

### Registering Cables

After successful connection of a cable that supports USB Power Delivery
Structured VDM “Discover Identity”, the port driver needs to register the cable
and one or two plugs, depending if there is CC Double Prime controller present
in the cable or not. So a cable capable of SOP Prime communication, but not SOP
Double Prime communication, should only have one plug registered. For more
information about SOP communication, please read chapter about it from the
latest USB Power Delivery specification.

The plugs are represented as their own devices. The cable is registered first,
followed by registration of the cable plugs. The cable will be the parent device
for the plugs. Details about the cable need to be described in `struct
typec_cable_desc` and about a plug in `struct typec_plug_desc`. The class copies
the details during registration. The class offers the following API for
registering/unregistering cables and their plugs:

struct typec\_plug \*typec\_register\_plug(struct typec\_cable \*cable, struct typec\_plug\_desc \*desc)
:   Register a USB Type-C Cable Plug

**Parameters**

`struct typec_cable *cable`
:   USB Type-C Cable with the plug

`struct typec_plug_desc *desc`
:   Description of the cable plug

**Description**

Registers a device for USB Type-C Cable Plug described in **desc**. A USB Type-C
Cable Plug represents a plug with electronics in it that can response to USB
Power Delivery SOP Prime or SOP Double Prime packages.

Returns handle to the cable plug on success or ERR\_PTR on failure.

void typec\_unregister\_plug(struct typec\_plug \*plug)
:   Unregister a USB Type-C Cable Plug

**Parameters**

`struct typec_plug *plug`
:   The cable plug to be unregistered

**Description**

Unregister device created with [`typec_register_plug()`](#c.typec_register_plug "typec_register_plug").

struct typec\_cable \*typec\_register\_cable(struct typec\_port \*port, struct typec\_cable\_desc \*desc)
:   Register a USB Type-C Cable

**Parameters**

`struct typec_port *port`
:   The USB Type-C Port the cable is connected to

`struct typec_cable_desc *desc`
:   Description of the cable

**Description**

Registers a device for USB Type-C Cable described in **desc**. The cable will be
parent for the optional cable plug devises.

Returns handle to the cable on success or ERR\_PTR on failure.

void typec\_unregister\_cable(struct typec\_cable \*cable)
:   Unregister a USB Type-C Cable

**Parameters**

`struct typec_cable *cable`
:   The cable to be unregistered

**Description**

Unregister device created with [`typec_register_cable()`](#c.typec_register_cable "typec_register_cable").

The class will provide a handle to `struct typec_cable` and `struct typec_plug` if
the registration is successful, or NULL if it isn’t.

If the cable is USB Power Delivery capable, and the port driver is able to show
the result of Discover Identity command, the cable descriptor structure should
include handle to `struct usb_pd_identity` instance. The class will then create a
sysfs directory for the identity under the cable device. The result of Discover
Identity command can then be reported with the following API:

int typec\_cable\_set\_identity(struct typec\_cable \*cable)
:   Report result from Discover Identity command

**Parameters**

`struct typec_cable *cable`
:   The cable updated identity values

**Description**

This routine is used to report that the result of Discover Identity USB power
delivery command has become available.

### Notifications

When the partner has executed a role change, or when the default roles change
during connection of a partner or cable, the port driver must use the following
APIs to report it to the class:

void typec\_set\_data\_role(struct typec\_port \*port, enum typec\_data\_role role)
:   Report data role change

**Parameters**

`struct typec_port *port`
:   The USB Type-C Port where the role was changed

`enum typec_data_role role`
:   The new data role

**Description**

This routine is used by the port drivers to report data role changes.

void typec\_set\_pwr\_role(struct typec\_port \*port, enum typec\_role role)
:   Report power role change

**Parameters**

`struct typec_port *port`
:   The USB Type-C Port where the role was changed

`enum typec_role role`
:   The new data role

**Description**

This routine is used by the port drivers to report power role changes.

void typec\_set\_vconn\_role(struct typec\_port \*port, enum typec\_role role)
:   Report VCONN source change

**Parameters**

`struct typec_port *port`
:   The USB Type-C Port which VCONN role changed

`enum typec_role role`
:   Source when **port** is sourcing VCONN, or Sink when it’s not

**Description**

This routine is used by the port drivers to report if the VCONN source is
changes.

void typec\_set\_pwr\_opmode(struct typec\_port \*port, enum typec\_pwr\_opmode opmode)
:   Report changed power operation mode

**Parameters**

`struct typec_port *port`
:   The USB Type-C Port where the mode was changed

`enum typec_pwr_opmode opmode`
:   New power operation mode

**Description**

This routine is used by the port drivers to report changed power operation
mode in **port**. The modes are USB (default), 1.5A, 3.0A as defined in USB
Type-C specification, and “USB Power Delivery” when the power levels are
negotiated with methods defined in USB Power Delivery specification.

### Alternate Modes

USB Type-C ports, partners and cable plugs may support Alternate Modes. Each
Alternate Mode will have identifier called SVID, which is either a Standard ID
given by USB-IF or vendor ID, and each supported SVID can have 1 - 6 modes. The
class provides `struct typec_mode_desc` for describing individual mode of a SVID,
and `struct typec_altmode_desc` which is a container for all the supported modes.

Ports that support Alternate Modes need to register each SVID they support with
the following API:

struct typec\_altmode \*typec\_port\_register\_altmode(struct typec\_port \*port, const struct typec\_altmode\_desc \*desc)
:   Register USB Type-C Port Alternate Mode

**Parameters**

`struct typec_port *port`
:   USB Type-C Port that supports the alternate mode

`const struct typec_altmode_desc *desc`
:   Description of the alternate mode

**Description**

This routine is used to register an alternate mode that **port** is capable of
supporting.

Returns handle to the alternate mode on success or ERR\_PTR on failure.

If a partner or cable plug provides a list of SVIDs as response to USB Power
Delivery Structured VDM Discover SVIDs message, each SVID needs to be
registered.

API for the partners:

struct typec\_altmode \*typec\_partner\_register\_altmode(struct typec\_partner \*partner, const struct typec\_altmode\_desc \*desc)
:   Register USB Type-C Partner Alternate Mode

**Parameters**

`struct typec_partner *partner`
:   USB Type-C Partner that supports the alternate mode

`const struct typec_altmode_desc *desc`
:   Description of the alternate mode

**Description**

This routine is used to register each alternate mode individually that
**partner** has listed in response to Discover SVIDs command. The modes for a
SVID listed in response to Discover Modes command need to be listed in an
array in **desc**.

Returns handle to the alternate mode on success or ERR\_PTR on failure.

API for the Cable Plugs:

struct typec\_altmode \*typec\_plug\_register\_altmode(struct typec\_plug \*plug, const struct typec\_altmode\_desc \*desc)
:   Register USB Type-C Cable Plug Alternate Mode

**Parameters**

`struct typec_plug *plug`
:   USB Type-C Cable Plug that supports the alternate mode

`const struct typec_altmode_desc *desc`
:   Description of the alternate mode

**Description**

This routine is used to register each alternate mode individually that **plug**
has listed in response to Discover SVIDs command. The modes for a SVID that
the plug lists in response to Discover Modes command need to be listed in an
array in **desc**.

Returns handle to the alternate mode on success or ERR\_PTR on failure.

So ports, partners and cable plugs will register the alternate modes with their
own functions, but the registration will always return a handle to `struct
typec_altmode` on success, or NULL. The unregistration will happen with the same
function:

void typec\_unregister\_altmode(struct typec\_altmode \*adev)
:   Unregister Alternate Mode

**Parameters**

`struct typec_altmode *adev`
:   The alternate mode to be unregistered

**Description**

Unregister device created with [`typec_partner_register_altmode()`](#c.typec_partner_register_altmode "typec_partner_register_altmode"),
[`typec_plug_register_altmode()`](#c.typec_plug_register_altmode "typec_plug_register_altmode") or [`typec_port_register_altmode()`](#c.typec_port_register_altmode "typec_port_register_altmode").

If a partner or cable plug enters or exits a mode, the port driver needs to
notify the class with the following API:

void typec\_altmode\_update\_active(struct typec\_altmode \*adev, bool active)
:   Report Enter/Exit mode

**Parameters**

`struct typec_altmode *adev`
:   Handle to the alternate mode

`bool active`
:   True when the mode has been entered

**Description**

If a partner or cable plug executes Enter/Exit Mode command successfully, the
drivers use this routine to report the updated state of the mode.

### Multiplexer/DeMultiplexer Switches

USB Type-C connectors may have one or more mux/demux switches behind them. Since
the plugs can be inserted right-side-up or upside-down, a switch is needed to
route the correct data pairs from the connector to the USB controllers. If
Alternate or Accessory Modes are supported, another switch is needed that can
route the pins on the connector to some other component besides USB. USB Type-C
Connector Class supplies an API for registering those switches.

struct typec\_switch\_dev \*typec\_switch\_register(struct [device](../infrastructure.html#c.device "device") \*parent, const struct typec\_switch\_desc \*desc)
:   Register USB Type-C orientation switch

**Parameters**

`struct device *parent`
:   Parent device

`const struct typec_switch_desc *desc`
:   Orientation switch description

**Description**

This function registers a switch that can be used for routing the correct
data pairs depending on the cable plug orientation from the USB Type-C
connector to the USB controllers. USB Type-C plugs can be inserted
right-side-up or upside-down.

void typec\_switch\_unregister(struct typec\_switch\_dev \*sw\_dev)
:   Unregister USB Type-C orientation switch

**Parameters**

`struct typec_switch_dev *sw_dev`
:   USB Type-C orientation switch

**Description**

Unregister switch that was registered with [`typec_switch_register()`](#c.typec_switch_register "typec_switch_register").

struct typec\_mux\_dev \*typec\_mux\_register(struct [device](../infrastructure.html#c.device "device") \*parent, const struct typec\_mux\_desc \*desc)
:   Register Multiplexer routing USB Type-C pins

**Parameters**

`struct device *parent`
:   Parent device

`const struct typec_mux_desc *desc`
:   Multiplexer description

**Description**

USB Type-C connectors can be used for alternate modes of operation besides
USB when Accessory/Alternate Modes are supported. With some of those modes,
the pins on the connector need to be reconfigured. This function registers
multiplexer switches routing the pins on the connector.

void typec\_mux\_unregister(struct typec\_mux\_dev \*mux\_dev)
:   Unregister Multiplexer Switch

**Parameters**

`struct typec_mux_dev *mux_dev`
:   USB Type-C Connector Multiplexer/DeMultiplexer

**Description**

Unregister mux that was registered with [`typec_mux_register()`](#c.typec_mux_register "typec_mux_register").

In most cases the same physical mux will handle both the orientation and mode.
However, as the port drivers will be responsible for the orientation, and the
alternate mode drivers for the mode, the two are always separated into their
own logical components: “mux” for the mode and “switch” for the orientation.

When a port is registered, USB Type-C Connector Class requests both the mux and
the switch for the port. The drivers can then use the following API for
controlling them:

int typec\_set\_orientation(struct typec\_port \*port, enum typec\_orientation orientation)
:   Set USB Type-C cable plug orientation

**Parameters**

`struct typec_port *port`
:   USB Type-C Port

`enum typec_orientation orientation`
:   USB Type-C cable plug orientation

**Description**

Set cable plug orientation for **port**.

int typec\_set\_mode(struct typec\_port \*port, int mode)
:   Set mode of operation for USB Type-C connector

**Parameters**

`struct typec_port *port`
:   USB Type-C connector

`int mode`
:   Accessory Mode, USB Operation or Safe State

**Description**

Configure **port** for Accessory Mode **mode**. This function will configure the
muxes needed for **mode**.

If the connector is dual-role capable, there may also be a switch for the data
role. USB Type-C Connector Class does not supply separate API for them. The
port drivers can use USB Role Class API with those.

Illustration of the muxes behind a connector that supports an alternate mode:

```
               ------------------------
               |       Connector      |
               ------------------------
                      |         |
               ------------------------
                \     Orientation    /
                 --------------------
                          |
                 --------------------
                /        Mode        \
               ------------------------
                   /              \
------------------------        --------------------
|       Alt Mode       |       /      USB Role      \
------------------------      ------------------------
                                   /            \
               ------------------------      ------------------------
               |       USB Host       |      |       USB Device     |
               ------------------------      ------------------------
```
