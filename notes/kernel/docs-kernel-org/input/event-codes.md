# 2.Input event codes

> 출처(원문): https://docs.kernel.org/input/event-codes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. Input event codes

The input protocol uses a map of types and codes to express input device values
to userspace. This document describes the types and codes and how and when they
may be used.

A single hardware event generates multiple input events. Each input event
contains the new value of a single data item. A special event type, EV\_SYN, is
used to separate input events into packets of input data changes occurring at
the same moment in time. In the following, the term “event” refers to a single
input event encompassing a type, code, and value.

The input protocol is a stateful protocol. Events are emitted only when values
of event codes have changed. However, the state is maintained within the Linux
input subsystem; drivers do not need to maintain the state and may attempt to
emit unchanged values without harm. Userspace may obtain the current state of
event code values using the EVIOCG\* ioctls defined in linux/input.h. The event
reports supported by a device are also provided by sysfs in
class/input/event\*/device/capabilities/, and the properties of a device are
provided in class/input/event\*/device/properties.

## 2.1. Event types

Event types are groupings of codes under a logical input construct. Each
type has a set of applicable codes to be used in generating events. See the
Codes section for details on valid codes for each type.

* EV\_SYN:

  + Used as markers to separate events. Events may be separated in time or in
    space, such as with the multitouch protocol.
* EV\_KEY:

  + Used to describe state changes of keyboards, buttons, or other key-like
    devices.
* EV\_REL:

  + Used to describe relative axis value changes, e.g. moving the mouse 5 units
    to the left.
* EV\_ABS:

  + Used to describe absolute axis value changes, e.g. describing the
    coordinates of a touch on a touchscreen.
* EV\_MSC:

  + Used to describe miscellaneous input data that do not fit into other types.
* EV\_SW:

  + Used to describe binary state input switches.
* EV\_LED:

  + Used to turn LEDs on devices on and off.
* EV\_SND:

  + Used to output sound to devices.
* EV\_REP:

  + Used for autorepeating devices.
* EV\_FF:

  + Used to send force feedback commands to an input device.
* EV\_PWR:

  + A special type for power button and switch input.
* EV\_FF\_STATUS:

  + Used to receive force feedback device status.

## 2.2. Event codes

Event codes define the precise type of event.

### 2.2.1. EV\_SYN

EV\_SYN event values are undefined. Their usage is defined only by when they are
sent in the evdev event stream.

* SYN\_REPORT:

  + Used to synchronize and separate events into packets of input data changes
    occurring at the same moment in time. For example, motion of a mouse may set
    the REL\_X and REL\_Y values for one motion, then emit a SYN\_REPORT. The next
    motion will emit more REL\_X and REL\_Y values and send another SYN\_REPORT.
* SYN\_CONFIG:

  + TBD
* SYN\_MT\_REPORT:

  + Used to synchronize and separate touch events. See the
    [Multi-touch (MT) Protocol](multi-touch-protocol.html) document for more information.
* SYN\_DROPPED:

  + Used to indicate buffer overrun in the evdev client’s event queue.
    Client should ignore all events up to and including next SYN\_REPORT
    event and query the device (using EVIOCG\* ioctls) to obtain its
    current state.

### 2.2.2. EV\_KEY

EV\_KEY events take the form KEY\_<name> or BTN\_<name>. For example, KEY\_A is used
to represent the ‘A’ key on a keyboard. When a key is depressed, an event with
the key’s code is emitted with value 1. When the key is released, an event is
emitted with value 0. Some hardware send events when a key is repeated. These
events have a value of 2. In general, KEY\_<name> is used for keyboard keys, and
BTN\_<name> is used for other types of momentary switch events.

A few EV\_KEY codes have special meanings:

* BTN\_TOOL\_<name>:

  + These codes are used in conjunction with input trackpads, tablets, and
    touchscreens. These devices may be used with fingers, pens, or other tools.
    When an event occurs and a tool is used, the corresponding BTN\_TOOL\_<name>
    code should be set to a value of 1. When the tool is no longer interacting
    with the input device, the BTN\_TOOL\_<name> code should be reset to 0. All
    trackpads, tablets, and touchscreens should use at least one BTN\_TOOL\_<name>
    code when events are generated. Likewise all trackpads, tablets, and
    touchscreens should export only one BTN\_TOOL\_<name> at a time. To not break
    existing userspace, it is recommended to not switch tool in one EV\_SYN frame
    but first emitting the old BTN\_TOOL\_<name> at 0, then emit one SYN\_REPORT
    and then set the new BTN\_TOOL\_<name> at 1.
* BTN\_TOUCH:

  > BTN\_TOUCH is used for touch contact. While an input tool is determined to be
  > within meaningful physical contact, the value of this property must be set
  > to 1. Meaningful physical contact may mean any contact, or it may mean
  > contact conditioned by an implementation defined property. For example, a
  > touchpad may set the value to 1 only when the touch pressure rises above a
  > certain value. BTN\_TOUCH may be combined with BTN\_TOOL\_<name> codes. For
  > example, a pen tablet may set BTN\_TOOL\_PEN to 1 and BTN\_TOUCH to 0 while the
  > pen is hovering over but not touching the tablet surface.

Note: For appropriate function of the legacy mousedev emulation driver,
BTN\_TOUCH must be the first evdev code emitted in a synchronization frame.

Note: Historically a touch device with BTN\_TOOL\_FINGER and BTN\_TOUCH was
interpreted as a touchpad by userspace, while a similar device without
BTN\_TOOL\_FINGER was interpreted as a touchscreen. For backwards compatibility
with current userspace it is recommended to follow this distinction. In the
future, this distinction will be deprecated and the device properties ioctl
EVIOCGPROP, defined in linux/input.h, will be used to convey the device type.

* BTN\_TOOL\_FINGER, BTN\_TOOL\_DOUBLETAP, BTN\_TOOL\_TRIPLETAP, BTN\_TOOL\_QUADTAP:

  + These codes denote one, two, three, and four finger interaction on a
    trackpad or touchscreen. For example, if the user uses two fingers and moves
    them on the touchpad in an effort to scroll content on screen,
    BTN\_TOOL\_DOUBLETAP should be set to value 1 for the duration of the motion.
    Note that all BTN\_TOOL\_<name> codes and the BTN\_TOUCH code are orthogonal in
    purpose. A trackpad event generated by finger touches should generate events
    for one code from each group. At most only one of these BTN\_TOOL\_<name>
    codes should have a value of 1 during any synchronization frame.

Note: Historically some drivers emitted multiple of the finger count codes with
a value of 1 in the same synchronization frame. This usage is deprecated.

Note: In multitouch drivers, the [`input_mt_report_finger_count()`](../driver-api/input.html#c.input_mt_report_finger_count "input_mt_report_finger_count") function should
be used to emit these codes. Please see [Multi-touch (MT) Protocol](multi-touch-protocol.html) for details.

### 2.2.3. EV\_REL

EV\_REL events describe relative changes in a property. For example, a mouse may
move to the left by a certain number of units, but its absolute position in
space is unknown. If the absolute position is known, EV\_ABS codes should be used
instead of EV\_REL codes.

A few EV\_REL codes have special meanings:

* REL\_WHEEL, REL\_HWHEEL:

  + These codes are used for vertical and horizontal scroll wheels,
    respectively. The value is the number of detents moved on the wheel, the
    physical size of which varies by device. For high-resolution wheels
    this may be an approximation based on the high-resolution scroll events,
    see REL\_WHEEL\_HI\_RES. These event codes are legacy codes and
    REL\_WHEEL\_HI\_RES and REL\_HWHEEL\_HI\_RES should be preferred where
    available.
* REL\_WHEEL\_HI\_RES, REL\_HWHEEL\_HI\_RES:

  + High-resolution scroll wheel data. The accumulated value 120 represents
    movement by one detent. For devices that do not provide high-resolution
    scrolling, the value is always a multiple of 120. For devices with
    high-resolution scrolling, the value may be a fraction of 120.

    If a vertical scroll wheel supports high-resolution scrolling, this code
    will be emitted in addition to REL\_WHEEL or REL\_HWHEEL. The REL\_WHEEL
    and REL\_HWHEEL may be an approximation based on the high-resolution
    scroll events. There is no guarantee that the high-resolution data
    is a multiple of 120 at the time of an emulated REL\_WHEEL or REL\_HWHEEL
    event.

### 2.2.4. EV\_ABS

EV\_ABS events describe absolute changes in a property. For example, a touchpad
may emit coordinates for a touch location.

A few EV\_ABS codes have special meanings:

* ABS\_DISTANCE:

  + Used to describe the distance of a tool from an interaction surface. This
    event should only be emitted while the tool is hovering, meaning in close
    proximity of the device and while the value of the BTN\_TOUCH code is 0. If
    the input device may be used freely in three dimensions, consider ABS\_Z
    instead.
  + BTN\_TOOL\_<name> should be set to 1 when the tool comes into detectable
    proximity and set to 0 when the tool leaves detectable proximity.
    BTN\_TOOL\_<name> signals the type of tool that is currently detected by the
    hardware and is otherwise independent of ABS\_DISTANCE and/or BTN\_TOUCH.
* ABS\_PROFILE:

  + Used to describe the state of a multi-value profile switch. An event is
    emitted only when the selected profile changes, indicating the newly
    selected profile value.
* ABS\_SND\_PROFILE:

  + Used to describe the state of a multi-value sound profile switch.
    An event is emitted only when the selected profile changes,
    indicating the newly selected profile value.
* ABS\_MT\_<name>:

  + Used to describe multitouch input events. Please see
    [Multi-touch (MT) Protocol](multi-touch-protocol.html) for details.
* ABS\_PRESSURE/ABS\_MT\_PRESSURE:

  > + For touch devices, many devices converted contact size into pressure.
  >   A finger flattens with pressure, causing a larger contact area and thus
  >   pressure and contact size are directly related. This is not the case
  >   for other devices, for example digitizers and touchpads with a true
  >   pressure sensor (“pressure pads”).
  >
  >   A device should set the resolution of the axis to indicate whether the
  >   pressure is in measurable units. If the resolution is zero, the
  >   pressure data is in arbitrary units. If the resolution is non-zero, the
  >   pressure data is in units/gram. For example, a value of 10 with a
  >   resolution of 1 represents 10 gram, a value of 10 with a resolution of
  >   1000 represents 10 microgram.

### 2.2.5. EV\_SW

EV\_SW events describe stateful binary switches. For example, the SW\_LID code is
used to denote when a laptop lid is closed.

Upon binding to a device or resuming from suspend, a driver must report
the current switch state. This ensures that the device, kernel, and userspace
state is in sync.

Upon resume, if the switch state is the same as before suspend, then the input
subsystem will filter out the duplicate switch state reports. The driver does
not need to keep the state of the switch at any time.

### 2.2.6. EV\_MSC

EV\_MSC events are used for input and output events that do not fall under other
categories.

A few EV\_MSC codes have special meaning:

* MSC\_TIMESTAMP:

  + Used to report the number of microseconds since the last reset. This event
    should be coded as an uint32 value, which is allowed to wrap around with
    no special consequence. It is assumed that the time difference between two
    consecutive events is reliable on a reasonable time scale (hours).
    A reset to zero can happen, in which case the time since the last event is
    unknown. If the device does not provide this information, the driver must
    not provide it to user space.

### 2.2.7. EV\_LED

EV\_LED events are used for input and output to set and query the state of
various LEDs on devices.

### 2.2.8. EV\_REP

EV\_REP events are used for specifying autorepeating events.

### 2.2.9. EV\_SND

EV\_SND events are used for sending sound commands to simple sound output
devices.

### 2.2.10. EV\_FF

EV\_FF events are used to initialize a force feedback capable device and to cause
such device to feedback.

### 2.2.11. EV\_PWR

EV\_PWR events are a special type of event used specifically for power
management. Its usage is not well defined. To be addressed later.

## 2.3. Device properties

Normally, userspace sets up an input device based on the data it emits,
i.e., the event types. In the case of two devices emitting the same event
types, additional information can be provided in the form of device
properties.

### 2.3.1. INPUT\_PROP\_DIRECT + INPUT\_PROP\_POINTER

The INPUT\_PROP\_DIRECT property indicates that device coordinates should be
directly mapped to screen coordinates (not taking into account trivial
transformations, such as scaling, flipping and rotating). Non-direct input
devices require non-trivial transformation, such as absolute to relative
transformation for touchpads. Typical direct input devices: touchscreens,
drawing tablets; non-direct devices: touchpads, mice.

The INPUT\_PROP\_POINTER property indicates that the device is not transposed
on the screen and thus requires use of an on-screen pointer to trace user’s
movements. Typical pointer devices: touchpads, tablets, mice; non-pointer
device: touchscreen.

If neither INPUT\_PROP\_DIRECT or INPUT\_PROP\_POINTER are set, the property is
considered undefined and the device type should be deduced in the
traditional way, using emitted event types.

### 2.3.2. INPUT\_PROP\_BUTTONPAD

For touchpads where the button is placed beneath the surface, such that
pressing down on the pad causes a button click, this property should be
set. Common in Clickpad notebooks and Macbooks from 2009 and onwards.

Originally, the buttonpad property was coded into the bcm5974 driver
version field under the name integrated button. For backwards
compatibility, both methods need to be checked in userspace.

### 2.3.3. INPUT\_PROP\_SEMI\_MT

Some touchpads, most common between 2008 and 2011, can detect the presence
of multiple contacts without resolving the individual positions; only the
number of contacts and a rectangular shape is known. For such
touchpads, the SEMI\_MT property should be set.

Depending on the device, the rectangle may enclose all touches, like a
bounding box, or just some of them, for instance the two most recent
touches. The diversity makes the rectangle of limited use, but some
gestures can normally be extracted from it.

If INPUT\_PROP\_SEMI\_MT is not set, the device is assumed to be a true MT
device.

### 2.3.4. INPUT\_PROP\_TOPBUTTONPAD

Some laptops, most notably the Lenovo 40 series provide a trackstick
device but do not have physical buttons associated with the trackstick
device. Instead, the top area of the touchpad is marked to show
visual/haptic areas for left, middle, right buttons intended to be used
with the trackstick.

If INPUT\_PROP\_TOPBUTTONPAD is set, userspace should emulate buttons
accordingly. This property does not affect kernel behavior.
The kernel does not provide button emulation for such devices but treats
them as any other INPUT\_PROP\_BUTTONPAD device.

### 2.3.5. INPUT\_PROP\_ACCELEROMETER

Directional axes on this device (absolute and/or relative x, y, z) represent
accelerometer data. Some devices also report gyroscope data, which devices
can report through the rotational axes (absolute and/or relative rx, ry, rz).

All other axes retain their meaning. A device must not mix
regular directional axes and accelerometer axes on the same event node.

### 2.3.6. INPUT\_PROP\_PRESSUREPAD

The INPUT\_PROP\_PRESSUREPAD property indicates that the device provides
simulated haptic feedback (e.g. a vibrator motor situated below the surface)
instead of physical haptic feedback (e.g. a hinge). This property is only set
if the device:

* can differentiate between at least 5 fingers
* uses correct resolution for the X/Y (units and value)
* follows the MT protocol type B

If the simulated haptic feedback is controllable by userspace the device must:

* support simple haptic auto and manual triggering, and
* report correct force per touch, and correct units for them (newtons or grams), and
* provide the EV\_FF FF\_HAPTIC force feedback effect.

Summing up, such devices follow the MS spec for input devices in
Win8 and Win8.1, and in addition may support the Simple haptic controller HID
table, and report correct units for the pressure.

Where applicable, this property is set in addition to INPUT\_PROP\_BUTTONPAD, it
does not replace that property.

## 2.4. Guidelines

The guidelines below ensure proper single-touch and multi-finger functionality.
For multi-touch functionality, see the [Multi-touch (MT) Protocol](multi-touch-protocol.html) document for
more information.

### 2.4.1. Mice

REL\_{X,Y} must be reported when the mouse moves. BTN\_LEFT must be used to report
the primary button press. BTN\_{MIDDLE,RIGHT,4,5,etc.} should be used to report
further buttons of the device. REL\_WHEEL and REL\_HWHEEL should be used to report
scroll wheel events where available.

### 2.4.2. Touchscreens

ABS\_{X,Y} must be reported with the location of the touch. BTN\_TOUCH must be
used to report when a touch is active on the screen.
BTN\_{MOUSE,LEFT,MIDDLE,RIGHT} must not be reported as the result of touch
contact. BTN\_TOOL\_<name> events should be reported where possible.

For new hardware, INPUT\_PROP\_DIRECT should be set.

### 2.4.3. Trackpads

Legacy trackpads that only provide relative position information must report
events like mice described above.

Trackpads that provide absolute touch position must report ABS\_{X,Y} for the
location of the touch. BTN\_TOUCH should be used to report when a touch is active
on the trackpad. Where multi-finger support is available, BTN\_TOOL\_<name> should
be used to report the number of touches active on the trackpad.

For new hardware, INPUT\_PROP\_POINTER should be set.

### 2.4.4. Tablets

BTN\_TOOL\_<name> events must be reported when a stylus or other tool is active on
the tablet. ABS\_{X,Y} must be reported with the location of the tool. BTN\_TOUCH
should be used to report when the tool is in contact with the tablet.
BTN\_{STYLUS,STYLUS2} should be used to report buttons on the tool itself. Any
button may be used for buttons on the tablet except BTN\_{MOUSE,LEFT}.
BTN\_{0,1,2,etc} are good generic codes for unlabeled buttons. Do not use
meaningful buttons, like BTN\_FORWARD, unless the button is labeled for that
purpose on the device.

For new hardware, both INPUT\_PROP\_DIRECT and INPUT\_PROP\_POINTER should be set.
