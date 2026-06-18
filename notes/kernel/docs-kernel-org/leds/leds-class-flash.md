# Flash LED handling under Linux

> 출처(원문): https://docs.kernel.org/leds/leds-class-flash.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Flash LED handling under Linux

Some LED devices provide two modes - torch and flash. In the LED subsystem
those modes are supported by LED class (see [LED handling under Linux](leds-class.html))
and LED Flash class respectively. The torch mode related features are enabled
by default and the flash ones only if a driver declares it by setting
LED\_DEV\_CAP\_FLASH flag.

In order to enable the support for flash LEDs CONFIG\_LEDS\_CLASS\_FLASH symbol
must be defined in the kernel config. A LED Flash class driver must be
registered in the LED subsystem with led\_classdev\_flash\_register function.

Following sysfs attributes are exposed for controlling flash LED devices:
(see [ABI file testing/sysfs-class-led-flash](../admin-guide/abi-testing-files.html#abi-file-testing-sysfs-class-led-flash))

> * flash\_brightness
> * max\_flash\_brightness
> * flash\_timeout
> * max\_flash\_timeout
> * flash\_strobe
> * flash\_fault

## V4L2 flash wrapper for flash LEDs

A LED subsystem driver can be controlled also from the level of VideoForLinux2
subsystem. In order to enable this CONFIG\_V4L2\_FLASH\_LED\_CLASS symbol has to
be defined in the kernel config.

The driver must call the v4l2\_flash\_init function to get registered in the
V4L2 subsystem. The function takes six arguments:

* dev:
  :   flash device, e.g. an I2C device
* of\_node:
  :   of\_node of the LED, may be NULL if the same as device’s
* fled\_cdev:
  :   LED flash class device to wrap
* iled\_cdev:
  :   LED flash class device representing indicator LED associated with
      fled\_cdev, may be NULL
* ops:
  :   V4L2 specific ops

      + external\_strobe\_set
        :   defines the source of the flash LED strobe -
            V4L2\_CID\_FLASH\_STROBE control or external source, typically
            a sensor, which makes it possible to synchronise the flash
            strobe start with exposure start,
      + intensity\_to\_led\_brightness and led\_brightness\_to\_intensity
        :   perform
            `enum led_brightness` <-> V4L2 intensity conversion in a device
            specific manner - they can be used for devices with non-linear
            LED current scale.
* config:
  :   configuration for V4L2 Flash sub-device

      + dev\_name
        :   the name of the media entity, unique in the system,
      + flash\_faults
        :   bitmask of flash faults that the LED flash class
            device can report; corresponding LED\_FAULT\* bit definitions are
            available in <linux/led-class-flash.h>,
      + torch\_intensity
        :   constraints for the LED in TORCH mode
            in microamperes,
      + indicator\_intensity
        :   constraints for the indicator LED
            in microamperes,
      + has\_external\_strobe
        :   determines whether the flash strobe source
            can be switched to external,

On remove the v4l2\_flash\_release function has to be called, which takes one
argument - [`struct v4l2_flash`](../driver-api/media/v4l2-flash-led-class.html#c.v4l2_flash "v4l2_flash") pointer returned previously by v4l2\_flash\_init.
This function can be safely called with NULL or error pointer argument.

Please refer to drivers/leds/leds-max77693.c for an exemplary usage of the
v4l2 flash wrapper.

Once the V4L2 sub-device is registered by the driver which created the Media
controller device, the sub-device node acts just as a node of a native V4L2
flash API device would. The calls are simply routed to the LED flash API.

Opening the V4L2 flash sub-device makes the LED subsystem sysfs interface
unavailable. The interface is re-enabled after the V4L2 flash sub-device
is closed.
