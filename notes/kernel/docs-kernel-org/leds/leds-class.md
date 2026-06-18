# LED handling under Linux

> 출처(원문): https://docs.kernel.org/leds/leds-class.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# LED handling under Linux

In its simplest form, the LED class just allows control of LEDs from
userspace. LEDs appear in /sys/class/leds/. The maximum brightness of the
LED is defined in max\_brightness file. The brightness file will set the brightness
of the LED (taking a value 0-max\_brightness). Most LEDs don’t have hardware
brightness support so will just be turned on for non-zero brightness settings.

The class also introduces the optional concept of an LED trigger. A trigger
is a kernel based source of led events. Triggers can either be simple or
complex. A simple trigger isn’t configurable and is designed to slot into
existing subsystems with minimal additional code. Examples are the disk-activity,
nand-disk and sharpsl-charge triggers. With led triggers disabled, the code
optimises away.

Complex triggers while available to all LEDs have LED specific
parameters and work on a per LED basis. The timer trigger is an example.
The timer trigger will periodically change the LED brightness between
LED\_OFF and the current brightness setting. The “on” and “off” time can
be specified via /sys/class/leds/<device>/delay\_{on,off} in milliseconds.
You can change the brightness value of a LED independently of the timer
trigger. However, if you set the brightness value to LED\_OFF it will
also disable the timer trigger.

You can change triggers in a similar manner to the way an IO scheduler
is chosen (via /sys/class/leds/<device>/trigger). Trigger specific
parameters can appear in /sys/class/leds/<device> once a given trigger is
selected.

## Design Philosophy

The underlying design philosophy is simplicity. LEDs are simple devices
and the aim is to keep a small amount of code giving as much functionality
as possible. Please keep this in mind when suggesting enhancements.

## LED Device Naming

Is currently of the form:

> “devicename:color:function”

* devicename:
  :   it should refer to a unique identifier created by the kernel,
      like e.g. phyN for network devices or inputN for input devices, rather
      than to the hardware; the information related to the product and the bus
      to which given device is hooked is available in sysfs and can be
      retrieved using get\_led\_device\_info.sh script from tools/leds; generally
      this section is expected mostly for LEDs that are somehow associated with
      other devices.
* color:
  :   one of LED\_COLOR\_ID\_\* definitions from the header
      include/dt-bindings/leds/common.h.
* function:
  :   one of LED\_FUNCTION\_\* definitions from the header
      include/dt-bindings/leds/common.h.

If required color or function is missing, please submit a patch
to [linux-leds@vger.kernel.org](mailto:linux-leds%40vger.kernel.org).

It is possible that more than one LED with the same color and function will
be required for given platform, differing only with an ordinal number.
In this case it is preferable to just concatenate the predefined LED\_FUNCTION\_\*
name with required “-N” suffix in the driver. fwnode based drivers can use
function-enumerator property for that and then the concatenation will be handled
automatically by the LED core upon LED class device registration.

LED subsystem has also a protection against name clash, that may occur
when LED class device is created by a driver of hot-pluggable device and
it doesn’t provide unique devicename section. In this case numerical
suffix (e.g. “\_1”, “\_2”, “\_3” etc.) is added to the requested LED class
device name.

There might be still LED class drivers around using vendor or product name
for devicename, but this approach is now deprecated as it doesn’t convey
any added value. Product information can be found in other places in sysfs
(see tools/leds/get\_led\_device\_info.sh).

Examples of proper LED names:

> * “red:disk”
> * “white:flash”
> * “red:indicator”
> * “phy1:green:wlan”
> * “phy3::wlan”
> * “:kbd\_backlight”
> * “input5::kbd\_backlight”
> * “input3::numlock”
> * “input3::scrolllock”
> * “input3::capslock”
> * “mmc1::status”
> * “white:status”

get\_led\_device\_info.sh script can be used for verifying if the LED name
meets the requirements pointed out here. It performs validation of the LED class
devicename sections and gives hints on expected value for a section in case
the validation fails for it. So far the script supports validation
of associations between LEDs and following types of devices:

> * input devices
> * ieee80211 compliant USB devices

The script is open to extensions.

There have been calls for LED properties such as color to be exported as
individual led class attributes. As a solution which doesn’t incur as much
overhead, I suggest these become part of the device name. The naming scheme
above leaves scope for further attributes should they be needed. If sections
of the name don’t apply, just leave that section blank.

## Brightness setting API

LED subsystem core exposes following API for setting brightness:

> * led\_set\_brightness:
>   :   it is guaranteed not to sleep, passing LED\_OFF stops
>       blinking,
> * led\_set\_brightness\_sync:
>   :   for use cases when immediate effect is desired -
>       it can block the caller for the time required for accessing
>       device registers and can sleep, passing LED\_OFF stops hardware
>       blinking, returns -EBUSY if software blink fallback is enabled.

## LED registration API

A driver wanting to register a LED classdev for use by other drivers /
userspace needs to allocate and fill a led\_classdev `struct and` then call
[devm\_]led\_classdev\_register. If the non devm version is used the driver
must call led\_classdev\_unregister from its remove function before
free-ing the led\_classdev struct.

If the driver can detect hardware initiated brightness changes and thus
wants to have a brightness\_hw\_changed attribute then the LED\_BRIGHT\_HW\_CHANGED
flag must be set in flags before registering. Calling
led\_classdev\_notify\_brightness\_hw\_changed on a classdev not registered with
the LED\_BRIGHT\_HW\_CHANGED flag is a bug and will trigger a WARN\_ON.

## Hardware accelerated blink of LEDs

Some LEDs can be programmed to blink without any CPU interaction. To
support this feature, a LED driver can optionally implement the
`blink_set()` function (see <linux/leds.h>). To set an LED to blinking,
however, it is better to use the API function `led_blink_set()`, as it
will check and implement software fallback if necessary.

To turn off blinking, use the API function `led_brightness_set()`
with brightness value LED\_OFF, which should stop any software
timers that may have been required for blinking.

The `blink_set()` function should choose a user friendly blinking value
if it is called with \*delay\_on==0 && \*delay\_off==0 parameters. In this
case the driver should give back the chosen value through delay\_on and
delay\_off parameters to the leds subsystem.

Setting the brightness to zero with `brightness_set()` callback function
should completely turn off the LED and cancel the previously programmed
hardware blinking function, if any.

## Hardware driven LEDs

Some LEDs can be programmed to be driven by hardware. This is not
limited to blink but also to turn off or on autonomously.
To support this feature, a LED needs to implement various additional
ops and needs to declare specific support for the supported triggers.

With hw control we refer to the LED driven by hardware.

LED driver must define the following value to support hw control:

> * hw\_control\_trigger:
>   :   unique trigger name supported by the LED in hw control
>       mode.

LED driver must implement the following API to support hw control:
:   * hw\_control\_is\_supported:
      :   check if the flags passed by the supported trigger can
          be parsed and activate hw control on the LED.

          Return 0 if the passed flags mask is supported and
          can be set with `hw_control_set()`.

          If the passed flags mask is not supported -EOPNOTSUPP
          must be returned, the LED trigger will use software
          fallback in this case.

          Return a negative error in case of any other error like
          device not ready or timeouts.

    > * hw\_control\_set:
    >   :   activate hw control. LED driver will use the provided
    >       flags passed from the supported trigger, parse them to
    >       a set of mode and setup the LED to be driven by hardware
    >       following the requested modes.
    >
    >       Set LED\_OFF via the brightness\_set to deactivate hw control.
    >
    >       Return 0 on success, a negative error number on failing to
    >       apply flags.

    * hw\_control\_get:
      :   get active modes from a LED already in hw control, parse
          them and set in flags the current active flags for the
          supported trigger.

          Return 0 on success, a negative error number on failing
          parsing the initial mode.
          Error from this function is NOT FATAL as the device may
          be in a not supported initial state by the attached LED
          trigger.
    * hw\_control\_get\_device:
      :   return the device associated with the LED driver in
          hw control. A trigger might use this to match the
          returned device from this function with a configured
          device for the trigger as the source for blinking
          events and correctly enable hw control.
          (example a netdev trigger configured to blink for a
          particular dev match the returned dev from get\_device
          to set hw control)

          Returns a pointer to a [`struct device`](../driver-api/infrastructure.html#c.device "device") or NULL if nothing
          is currently attached.

LED driver can activate additional modes by default to workaround the
impossibility of supporting each different mode on the supported trigger.
Examples are hardcoding the blink speed to a set interval, enable special
feature like bypassing blink if some requirements are not met.

A trigger should first check if the hw control API are supported by the LED
driver and check if the trigger is supported to verify if hw control is possible,
use hw\_control\_is\_supported to check if the flags are supported and only at
the end use hw\_control\_set to activate hw control.

A trigger can use hw\_control\_get to check if a LED is already in hw control
and init their flags.

When the LED is in hw control, no software blink is possible and doing so
will effectively disable hw control.

## Known Issues

The LED Trigger core cannot be a module as the simple trigger functions
would cause nightmare dependency issues. I see this as a minor issue
compared to the benefits the simple trigger functionality brings. The
rest of the LED subsystem can be modular.
