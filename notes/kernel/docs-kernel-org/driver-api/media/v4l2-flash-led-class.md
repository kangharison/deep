# 3.18.V4L2 flash functions and data structures

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-flash-led-class.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.18. V4L2 flash functions and data structures

struct v4l2\_flash\_ctrl\_data
:   flash control initialization data, filled basing on the features declared by the LED flash class driver in the v4l2\_flash\_config

> **Definition**:
>
> ```
> struct v4l2_flash_ctrl_data {
>       struct v4l2_ctrl_config config;
>       u32 cid;
> };
> ```

**Members**

`config`
:   initialization data for a control

`cid`
:   contains v4l2 flash control id if the config
    field was initialized, 0 otherwise

struct v4l2\_flash\_ops
:   V4L2 flash operations

> **Definition**:
>
> ```
> struct v4l2_flash_ops {
>       int (*external_strobe_set)(struct v4l2_flash *v4l2_flash, bool enable);
>       enum led_brightness (*intensity_to_led_brightness) (struct v4l2_flash *v4l2_flash, s32 intensity);
>       s32 (*led_brightness_to_intensity) (struct v4l2_flash *v4l2_flash, enum led_brightness);
> };
> ```

**Members**

`external_strobe_set`
:   Setup strobing the flash by hardware pin state
    assertion.

`intensity_to_led_brightness`
:   Convert intensity to brightness in a device
    specific manner

`led_brightness_to_intensity`
:   convert brightness to intensity in a device
    specific manner.

struct v4l2\_flash\_config
:   V4L2 Flash sub-device initialization data

> **Definition**:
>
> ```
> struct v4l2_flash_config {
>       char dev_name[32];
>       struct led_flash_setting intensity;
>       u32 flash_faults;
>       unsigned int has_external_strobe:1;
> };
> ```

**Members**

`dev_name`
:   the name of the media entity,
    unique in the system

`intensity`
:   non-flash strobe constraints for the LED

`flash_faults`
:   bitmask of flash faults that the LED flash class
    device can report; corresponding LED\_FAULT\* bit
    definitions are available in the header file
    <linux/led-class-flash.h>

`has_external_strobe`
:   external strobe capability

struct v4l2\_flash
:   Flash sub-device context

> **Definition**:
>
> ```
> struct v4l2_flash {
>       struct led_classdev_flash *fled_cdev;
>       struct led_classdev *iled_cdev;
>       const struct v4l2_flash_ops *ops;
>       struct v4l2_subdev sd;
>       struct v4l2_ctrl_handler hdl;
>       struct v4l2_ctrl **ctrls;
> };
> ```

**Members**

`fled_cdev`
:   LED flash class device controlled by this sub-device

`iled_cdev`
:   LED class device representing indicator LED associated
    with the LED flash class device

`ops`
:   V4L2 specific flash ops

`sd`
:   V4L2 sub-device

`hdl`
:   flash controls handler

`ctrls`
:   array of pointers to controls, whose values define
    the sub-device state

struct [v4l2\_flash](#c.v4l2_flash "v4l2_flash") \*v4l2\_subdev\_to\_v4l2\_flash(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Returns a [`struct v4l2_flash`](#c.v4l2_flash "v4l2_flash") from the [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") embedded on it.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

struct [v4l2\_flash](#c.v4l2_flash "v4l2_flash") \*v4l2\_ctrl\_to\_v4l2\_flash(struct [v4l2\_ctrl](v4l2-controls.html#c.v4l2_ctrl "v4l2_ctrl") \*c)
:   Returns a [`struct v4l2_flash`](#c.v4l2_flash "v4l2_flash") from the [`struct v4l2_ctrl`](v4l2-controls.html#c.v4l2_ctrl "v4l2_ctrl") embedded on it.

**Parameters**

`struct v4l2_ctrl *c`
:   pointer to [`struct v4l2_ctrl`](v4l2-controls.html#c.v4l2_ctrl "v4l2_ctrl")

struct [v4l2\_flash](#c.v4l2_flash "v4l2_flash") \*v4l2\_flash\_init(struct [device](../infrastructure.html#c.device "device") \*dev, struct fwnode\_handle \*fwn, struct led\_classdev\_flash \*fled\_cdev, const struct [v4l2\_flash\_ops](#c.v4l2_flash_ops "v4l2_flash_ops") \*ops, struct [v4l2\_flash\_config](#c.v4l2_flash_config "v4l2_flash_config") \*config)
:   initialize V4L2 flash led sub-device

**Parameters**

`struct device *dev`
:   flash device, e.g. an I2C device

`struct fwnode_handle *fwn`
:   fwnode\_handle of the LED, may be NULL if the same as device’s

`struct led_classdev_flash *fled_cdev`
:   LED flash class device to wrap

`const struct v4l2_flash_ops *ops`
:   V4L2 Flash device ops

`struct v4l2_flash_config *config`
:   initialization data for V4L2 Flash sub-device

**Description**

Create V4L2 Flash sub-device wrapping given LED subsystem device.
The ops pointer is stored by the V4L2 flash framework. No
references are held to config nor its contents once this function
has returned.

**Return**

A valid pointer, or, when an error occurs, the return
value is encoded using [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR"). Use [`IS_ERR()`](../../core-api/kernel-api.html#c.IS_ERR "IS_ERR") to check and
[`PTR_ERR()`](../../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") to obtain the numeric return value.

struct [v4l2\_flash](#c.v4l2_flash "v4l2_flash") \*v4l2\_flash\_indicator\_init(struct [device](../infrastructure.html#c.device "device") \*dev, struct fwnode\_handle \*fwn, struct led\_classdev \*iled\_cdev, struct [v4l2\_flash\_config](#c.v4l2_flash_config "v4l2_flash_config") \*config)
:   initialize V4L2 indicator sub-device

**Parameters**

`struct device *dev`
:   flash device, e.g. an I2C device

`struct fwnode_handle *fwn`
:   fwnode\_handle of the LED, may be NULL if the same as device’s

`struct led_classdev *iled_cdev`
:   LED flash class device representing the indicator LED

`struct v4l2_flash_config *config`
:   initialization data for V4L2 Flash sub-device

**Description**

Create V4L2 Flash sub-device wrapping given LED subsystem device.
The ops pointer is stored by the V4L2 flash framework. No
references are held to config nor its contents once this function
has returned.

**Return**

A valid pointer, or, when an error occurs, the return
value is encoded using [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR"). Use [`IS_ERR()`](../../core-api/kernel-api.html#c.IS_ERR "IS_ERR") to check and
[`PTR_ERR()`](../../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") to obtain the numeric return value.

void v4l2\_flash\_release(struct [v4l2\_flash](#c.v4l2_flash_release "v4l2_flash") \*v4l2\_flash)
:   release V4L2 Flash sub-device

**Parameters**

`struct v4l2_flash *v4l2_flash`
:   the V4L2 Flash sub-device to release

**Description**

Release V4L2 Flash sub-device.
