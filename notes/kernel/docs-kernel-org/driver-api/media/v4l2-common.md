# 3.27.V4L2 common functions and data structures

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-common.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.27. V4L2 common functions and data structures

int v4l2\_ctrl\_query\_fill(struct [v4l2\_queryctrl](v4l2-controls.html#c.v4l2_queryctrl "v4l2_queryctrl") \*qctrl, s32 min, s32 max, s32 step, s32 def)
:   Fill in a [`struct v4l2_queryctrl`](v4l2-controls.html#c.v4l2_queryctrl "v4l2_queryctrl")

**Parameters**

`struct v4l2_queryctrl *qctrl`
:   pointer to the [`struct v4l2_queryctrl`](v4l2-controls.html#c.v4l2_queryctrl "v4l2_queryctrl") to be filled

`s32 min`
:   minimum value for the control

`s32 max`
:   maximum value for the control

`s32 step`
:   control step

`s32 def`
:   default value for the control

**Description**

Fills the [`struct v4l2_queryctrl`](v4l2-controls.html#c.v4l2_queryctrl "v4l2_queryctrl") fields for the query control.

Note

This function assumes that the **qctrl->id** field is filled.

Returns -EINVAL if the control is not known by the V4L2 core, 0 on success.

enum v4l2\_i2c\_tuner\_type
:   specifies the range of tuner address that should be used when seeking for I2C devices.

**Constants**

`ADDRS_RADIO`
:   Radio tuner addresses.
    Represent the following I2C addresses:
    0x10 (if compiled with tea5761 support)
    and 0x60.

`ADDRS_DEMOD`
:   Demod tuner addresses.
    Represent the following I2C addresses:
    0x42, 0x43, 0x4a and 0x4b.

`ADDRS_TV`
:   TV tuner addresses.
    Represent the following I2C addresses:
    0x42, 0x43, 0x4a, 0x4b, 0x60, 0x61, 0x62,
    0x63 and 0x64.

`ADDRS_TV_WITH_DEMOD`
:   TV tuner addresses if demod is present, this
    excludes addresses used by the demodulator
    from the list of candidates.
    Represent the following I2C addresses:
    0x60, 0x61, 0x62, 0x63 and 0x64.

**NOTE**

All I2C addresses above use the 7-bit notation.

struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*v4l2\_i2c\_new\_subdev(struct [v4l2\_device](v4l2-device.html#c.v4l2_device "v4l2_device") \*v4l2\_dev, struct i2c\_adapter \*adapter, const char \*client\_type, u8 addr, const unsigned short \*probe\_addrs)
:   Load an i2c module and return an initialized [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev").

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device")

`struct i2c_adapter *adapter`
:   pointer to `struct i2c_adapter`

`const char *client_type`
:   name of the chip that’s on the adapter.

`u8 addr`
:   I2C address. If zero, it will use **probe\_addrs**

`const unsigned short *probe_addrs`
:   array with a list of address. The last entry at such
    array should be `I2C_CLIENT_END`.

**Description**

returns a [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") pointer.

struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*v4l2\_i2c\_new\_subdev\_board(struct [v4l2\_device](v4l2-device.html#c.v4l2_device "v4l2_device") \*v4l2\_dev, struct i2c\_adapter \*adapter, struct [i2c\_board\_info](../i2c.html#c.i2c_board_info "i2c_board_info") \*info, const unsigned short \*probe\_addrs)
:   Load an i2c module and return an initialized [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev").

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device")

`struct i2c_adapter *adapter`
:   pointer to `struct i2c_adapter`

`struct i2c_board_info *info`
:   pointer to [`struct i2c_board_info`](../i2c.html#c.i2c_board_info "i2c_board_info") used to replace the irq,
    platform\_data and addr arguments.

`const unsigned short *probe_addrs`
:   array with a list of address. The last entry at such
    array should be `I2C_CLIENT_END`.

**Description**

returns a [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") pointer.

void v4l2\_i2c\_subdev\_set\_name(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct [i2c\_client](../i2c.html#c.i2c_client "i2c_client") \*client, const char \*devname, const char \*postfix)
:   Set name for an I²C sub-device

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`struct i2c_client *client`
:   pointer to [`struct i2c_client`](../i2c.html#c.i2c_client "i2c_client")

`const char *devname`
:   the name of the device; if NULL, the I²C device drivers’s name
    will be used

`const char *postfix`
:   sub-device specific string to put right after the I²C device name;
    may be NULL

void v4l2\_i2c\_subdev\_init(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct [i2c\_client](../i2c.html#c.i2c_client "i2c_client") \*client, const struct [v4l2\_subdev\_ops](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") \*ops)
:   Initializes a [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") with data from an i2c\_client struct.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`struct i2c_client *client`
:   pointer to [`struct i2c_client`](../i2c.html#c.i2c_client "i2c_client")

`const struct v4l2_subdev_ops *ops`
:   pointer to [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops")

unsigned short v4l2\_i2c\_subdev\_addr(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   returns i2c client address of [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev").

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

**Description**

Returns the address of an I2C sub-device

const unsigned short \*v4l2\_i2c\_tuner\_addrs(enum [v4l2\_i2c\_tuner\_type](#c.v4l2_i2c_tuner_type "v4l2_i2c_tuner_type") type)
:   Return a list of I2C tuner addresses to probe.

**Parameters**

`enum v4l2_i2c_tuner_type type`
:   type of the tuner to seek, as defined by
    [`enum v4l2_i2c_tuner_type`](#c.v4l2_i2c_tuner_type "v4l2_i2c_tuner_type").

**NOTE**

Use only if the tuner addresses are unknown.

void v4l2\_i2c\_subdev\_unregister(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Unregister a v4l2\_subdev

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*v4l2\_spi\_new\_subdev(struct [v4l2\_device](v4l2-device.html#c.v4l2_device "v4l2_device") \*v4l2\_dev, struct [spi\_controller](../spi.html#c.spi_controller "spi_controller") \*ctlr, struct [spi\_board\_info](../spi.html#c.spi_board_info "spi_board_info") \*info)
:   Load an spi module and return an initialized [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev").

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device").

`struct spi_controller *ctlr`
:   pointer to [`struct spi_controller`](../spi.html#c.spi_controller "spi_controller").

`struct spi_board_info *info`
:   pointer to [`struct spi_board_info`](../spi.html#c.spi_board_info "spi_board_info").

**Description**

returns a [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") pointer.

void v4l2\_spi\_subdev\_init(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct [spi\_device](../spi.html#c.spi_device "spi_device") \*spi, const struct [v4l2\_subdev\_ops](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") \*ops)
:   Initialize a v4l2\_subdev with data from an spi\_device struct.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`struct spi_device *spi`
:   pointer to [`struct spi_device`](../spi.html#c.spi_device "spi_device").

`const struct v4l2_subdev_ops *ops`
:   pointer to [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops")

void v4l2\_spi\_subdev\_unregister(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Unregister a v4l2\_subdev

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

void v4l\_bound\_align\_image(unsigned int \*width, unsigned int wmin, unsigned int wmax, unsigned int walign, unsigned int \*height, unsigned int hmin, unsigned int hmax, unsigned int halign, unsigned int salign)
:   adjust video dimensions according to a given constraints.

**Parameters**

`unsigned int *width`
:   pointer to width that will be adjusted if needed.

`unsigned int wmin`
:   minimum width.

`unsigned int wmax`
:   maximum width.

`unsigned int walign`
:   least significant bit on width.

`unsigned int *height`
:   pointer to height that will be adjusted if needed.

`unsigned int hmin`
:   minimum height.

`unsigned int hmax`
:   maximum height.

`unsigned int halign`
:   least significant bit on height.

`unsigned int salign`
:   least significant bit for the image size (e. g.
    ![width * height](_images/math/20d03bfef00d6b5808ba0a5d41b1dd8ce7cb9e99.png)).

**Description**

Clip an image to have **width** between **wmin** and **wmax**, and **height** between
**hmin** and **hmax**, inclusive.

Additionally, the **width** will be a multiple of ![2^{walign}](_images/math/67a08268328fc3a482ec0910d19f7549058c7a9c.png),
the **height** will be a multiple of ![2^{halign}](_images/math/004528933b2819d5b7b497a28544867b4a71a3a9.png), and the overall
size ![width * height](_images/math/20d03bfef00d6b5808ba0a5d41b1dd8ce7cb9e99.png) will be a multiple of ![2^{salign}](_images/math/a16b7e62bdc98aa49db08480890a77ee9bff9d67.png).

Note

1. The clipping rectangle may be shrunk or enlarged to fit the alignment
   constraints.
2. **wmax** must not be smaller than **wmin**.
3. **hmax** must not be smaller than **hmin**.
4. The alignments must not be so high there are no possible image
   sizes within the allowed bounds.
5. **wmin** and **hmin** must be at least 1 (don’t use 0).
6. For **walign**, **halign** and **salign**, if you don’t care about a certain
   alignment, specify `0`, as ![2^0 = 1](_images/math/30e7079eba8ca63eee0077129966e10125c65838.png) and one byte alignment
   is equivalent to no alignment.
7. If you only want to adjust downward, specify a maximum that’s the
   same as the initial value.

v4l2\_find\_nearest\_size\_conditional

`v4l2_find_nearest_size_conditional (array, array_size, width_field, height_field, width, height, func, context)`

> Find the nearest size among a discrete set of resolutions contained in an array of a driver specific struct, with conditionally exlusion of certain modes

**Parameters**

`array`
:   a driver specific array of image sizes

`array_size`
:   the length of the driver specific array of image sizes

`width_field`
:   the name of the width field in the driver specific struct

`height_field`
:   the name of the height field in the driver specific struct

`width`
:   desired width

`height`
:   desired height

`func`
:   ignores mode if returns false

`context`
:   context for the function

**Description**

Finds the closest resolution to minimize the width and height differences
between what requested and the supported resolutions. The size of the width
and height fields in the driver specific must equal to that of u32, i.e. four
bytes. **func** is called for each mode considered, a mode is ignored if **func**
returns false for it.

Returns the best match or NULL if the length of the array is zero.

v4l2\_find\_nearest\_size

`v4l2_find_nearest_size (array, array_size, width_field, height_field, width, height)`

> Find the nearest size among a discrete set of resolutions contained in an array of a driver specific struct

**Parameters**

`array`
:   a driver specific array of image sizes

`array_size`
:   the length of the driver specific array of image sizes

`width_field`
:   the name of the width field in the driver specific struct

`height_field`
:   the name of the height field in the driver specific struct

`width`
:   desired width

`height`
:   desired height

**Description**

Finds the closest resolution to minimize the width and height differences
between what requested and the supported resolutions. The size of the width
and height fields in the driver specific must equal to that of u32, i.e. four
bytes.

Returns the best match or NULL if the length of the array is zero.

int v4l2\_g\_parm\_cap(struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev, struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct v4l2\_streamparm \*a)
:   helper routine for vidioc\_g\_parm to fill this in by calling the get\_frame\_interval op of the given subdev. It only works for V4L2\_BUF\_TYPE\_VIDEO\_CAPTURE(\_MPLANE), hence the \_cap in the function name.

**Parameters**

`struct video_device *vdev`
:   the [`struct video_device`](v4l2-dev.html#c.video_device "video_device") pointer. Used to determine the device caps.

`struct v4l2_subdev *sd`
:   the sub-device pointer.

`struct v4l2_streamparm *a`
:   the VIDIOC\_G\_PARM argument.

int v4l2\_s\_parm\_cap(struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev, struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct v4l2\_streamparm \*a)
:   helper routine for vidioc\_s\_parm to fill this in by calling the set\_frame\_interval op of the given subdev. It only works for V4L2\_BUF\_TYPE\_VIDEO\_CAPTURE(\_MPLANE), hence the \_cap in the function name.

**Parameters**

`struct video_device *vdev`
:   the [`struct video_device`](v4l2-dev.html#c.video_device "video_device") pointer. Used to determine the device caps.

`struct v4l2_subdev *sd`
:   the sub-device pointer.

`struct v4l2_streamparm *a`
:   the VIDIOC\_S\_PARM argument.

enum v4l2\_pixel\_encoding
:   specifies the pixel encoding value

**Constants**

`V4L2_PIXEL_ENC_UNKNOWN`
:   Pixel encoding is unknown/un-initialized

`V4L2_PIXEL_ENC_YUV`
:   Pixel encoding is YUV

`V4L2_PIXEL_ENC_RGB`
:   Pixel encoding is RGB

`V4L2_PIXEL_ENC_BAYER`
:   Pixel encoding is Bayer

struct v4l2\_format\_info
:   information about a V4L2 format

**Definition**:

```
struct v4l2_format_info {
    u32 format;
    u8 pixel_enc;
    u8 mem_planes;
    u8 comp_planes;
    u8 bpp[4];
    u8 bpp_div[4];
    u8 hdiv;
    u8 vdiv;
    u8 block_w[4];
    u8 block_h[4];
};
```

**Members**

`format`
:   4CC format identifier (V4L2\_PIX\_FMT\_\*)

`pixel_enc`
:   Pixel encoding (see [`enum v4l2_pixel_encoding`](#c.v4l2_pixel_encoding "v4l2_pixel_encoding") above)

`mem_planes`
:   Number of memory planes, which includes the alpha plane (1 to 4).

`comp_planes`
:   Number of component planes, which includes the alpha plane (1 to 4).

`bpp`
:   Array of per-plane bytes per pixel

`bpp_div`
:   Array of per-plane bytes per pixel divisors to support fractional pixel sizes.

`hdiv`
:   Horizontal chroma subsampling factor

`vdiv`
:   Vertical chroma subsampling factor

`block_w`
:   Per-plane macroblock pixel width (optional)

`block_h`
:   Per-plane macroblock pixel height (optional)

s64 v4l2\_get\_link\_freq(const struct [media\_pad](mc-core.html#c.media_pad "media_pad") \*pad, unsigned int mul, unsigned int div)
:   Get link rate from transmitter

**Parameters**

`const struct media_pad *pad`
:   The transmitter’s media pad

`unsigned int mul`
:   The multiplier between pixel rate and link frequency. Bits per pixel on
    D-PHY, samples per clock on parallel. 0 otherwise.

`unsigned int div`
:   The divisor between pixel rate and link frequency. Number of data lanes
    times two on D-PHY, 1 on parallel. 0 otherwise.

**Description**

This function obtains and returns the link frequency from the transmitter
sub-device’s pad. The link frequency is retrieved using the get\_mbus\_config
sub-device pad operation. If this fails, the function falls back to obtaining
the frequency either directly from the V4L2\_CID\_LINK\_FREQ control if
implemented by the transmitter, or by calculating it from the pixel rate
obtained from the V4L2\_CID\_PIXEL\_RATE control.

**Return**

* >0: Link frequency
* `-ENOENT`: Link frequency or pixel rate control not found
* `-EINVAL`: Invalid link frequency value

int v4l2\_get\_active\_data\_lanes(const struct [media\_pad](mc-core.html#c.media_pad "media_pad") \*pad, unsigned int max\_data\_lanes)
:   Get number of active data lanes from driver

**Parameters**

`const struct media_pad *pad`
:   The transmitter’s media pad.

`unsigned int max_data_lanes`
:   The maximum number of active data lanes supported by
    the MIPI CSI link in hardware.

**Description**

This function is intended for obtaining the number of data lanes that are
actively being used by the driver for a MIPI CSI-2 device on a given media pad.
This information is derived from a mbus\_config fetched from a device driver
using the get\_mbus\_config v4l2\_subdev pad op.

**Return**

* >0: Number of active data lanes
* `-EINVAL`: Number of active data lanes is invalid, as it exceeds the maximum
  :   supported data lanes.

int v4l2\_link\_freq\_to\_bitmap(struct [device](../infrastructure.html#c.device "device") \*dev, const u64 \*fw\_link\_freqs, unsigned int num\_of\_fw\_link\_freqs, const s64 \*driver\_link\_freqs, unsigned int num\_of\_driver\_link\_freqs, unsigned long \*bitmap)
:   Figure out platform-supported link frequencies

**Parameters**

`struct device *dev`
:   The [`struct device`](../infrastructure.html#c.device "device")

`const u64 *fw_link_freqs`
:   Array of link frequencies from firmware

`unsigned int num_of_fw_link_freqs`
:   Number of entries in **fw\_link\_freqs**

`const s64 *driver_link_freqs`
:   Array of link frequencies supported by the driver

`unsigned int num_of_driver_link_freqs`
:   Number of entries in **driver\_link\_freqs**

`unsigned long *bitmap`
:   Bitmap of driver-supported link frequencies found in **fw\_link\_freqs**

**Description**

This function checks which driver-supported link frequencies are enabled in
system firmware and sets the corresponding bits in **bitmap** (after first
zeroing it).

**Return**

* `0`: Success
* `-ENOENT`: No match found between driver-supported link frequencies and
  those available in firmware.
* `-ENODATA`: No link frequencies were specified in firmware.

struct clk \*devm\_v4l2\_sensor\_clk\_get(struct [device](../infrastructure.html#c.device "device") \*dev, const char \*id)
:   lookup and obtain a reference to a clock producer for a camera sensor

**Parameters**

`struct device *dev`
:   device for v4l2 sensor clock “consumer”

`const char *id`
:   clock consumer ID

**Description**

This function behaves the same way as [`devm_clk_get()`](../../core-api/kernel-api.html#c.devm_clk_get "devm_clk_get") except where there
is no clock producer like in ACPI-based platforms.

For ACPI-based platforms, the function will read the “clock-frequency”
ACPI \_DSD property and register a fixed-clock with the frequency indicated
in the property.

This function also handles the special ACPI-based system case where:

* The clock-frequency \_DSD property is present.
* A reference to the clock producer is present, where the clock is provided
  by a camera sensor PMIC driver (e.g. int3472/tps68470.c)

In this case try to set the clock-frequency value to the provided clock.

As the name indicates, this function may only be used on camera sensor
devices. This is because generally only camera sensors do need a clock to
query the frequency from, due to the requirement to configure the PLL for a
given CSI-2 interface frequency where the sensor’s external clock frequency
is a factor. Additionally, the clock frequency tends to be available on ACPI
firmware based systems for camera sensors specifically (if e.g. DisCo for
Imaging compliant).

Returns a pointer to a `struct clk` on success or an error pointer on failure.

struct clk \*devm\_v4l2\_sensor\_clk\_get\_legacy(struct [device](../infrastructure.html#c.device "device") \*dev, const char \*id, bool fixed\_rate, unsigned long clk\_rate)
:   lookup and obtain a reference to a clock producer for a camera sensor.

**Parameters**

`struct device *dev`
:   device for v4l2 sensor clock “consumer”

`const char *id`
:   clock consumer ID

`bool fixed_rate`
:   interpret the **clk\_rate** as a fixed rate or default rate

`unsigned long clk_rate`
:   the clock rate

**Description**

This function behaves the same way as [`devm_v4l2_sensor_clk_get()`](#c.devm_v4l2_sensor_clk_get "devm_v4l2_sensor_clk_get") except that
it extends the behaviour on ACPI platforms to all platforms.

The function also provides the ability to set the clock rate to a fixed
frequency by setting **fixed\_rate** to true and specifying the fixed frequency
in **clk\_rate**, or to use a default clock rate when the “clock-frequency”
property is absent by setting **fixed\_rate** to false and specifying the default
frequency in **clk\_rate**. Setting **fixed\_rate** to true and **clk\_rate** to 0 is an
error.

This function is meant to support legacy behaviour in existing drivers only.
It must not be used in any new driver.

Returns a pointer to a `struct clk` on success or an error pointer on failure.

struct v4l2\_ioctl\_ops
:   describe operations for each V4L2 ioctl

**Definition**:

```
struct v4l2_ioctl_ops {
    int (*vidioc_querycap)(struct file *file, void *priv, struct v4l2_capability *cap);
    int (*vidioc_enum_fmt_vid_cap)(struct file *file, void *priv, struct v4l2_fmtdesc *f);
    int (*vidioc_enum_fmt_vid_overlay)(struct file *file, void *priv, struct v4l2_fmtdesc *f);
    int (*vidioc_enum_fmt_vid_out)(struct file *file, void *priv, struct v4l2_fmtdesc *f);
    int (*vidioc_enum_fmt_sdr_cap)(struct file *file, void *priv, struct v4l2_fmtdesc *f);
    int (*vidioc_enum_fmt_sdr_out)(struct file *file, void *priv, struct v4l2_fmtdesc *f);
    int (*vidioc_enum_fmt_meta_cap)(struct file *file, void *priv, struct v4l2_fmtdesc *f);
    int (*vidioc_enum_fmt_meta_out)(struct file *file, void *priv, struct v4l2_fmtdesc *f);
    int (*vidioc_g_fmt_vid_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_vid_overlay)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_vid_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_vid_out_overlay)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_vbi_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_vbi_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_sliced_vbi_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_sliced_vbi_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_vid_cap_mplane)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_vid_out_mplane)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_sdr_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_sdr_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_meta_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_g_fmt_meta_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vid_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vid_overlay)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vid_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vid_out_overlay)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vbi_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vbi_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_sliced_vbi_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_sliced_vbi_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vid_cap_mplane)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_vid_out_mplane)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_sdr_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_sdr_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_meta_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_s_fmt_meta_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vid_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vid_overlay)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vid_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vid_out_overlay)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vbi_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vbi_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_sliced_vbi_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_sliced_vbi_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vid_cap_mplane)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_vid_out_mplane)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_sdr_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_sdr_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_meta_cap)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_try_fmt_meta_out)(struct file *file, void *priv, struct v4l2_format *f);
    int (*vidioc_reqbufs)(struct file *file, void *priv, struct v4l2_requestbuffers *b);
    int (*vidioc_querybuf)(struct file *file, void *priv, struct v4l2_buffer *b);
    int (*vidioc_qbuf)(struct file *file, void *priv, struct v4l2_buffer *b);
    int (*vidioc_expbuf)(struct file *file, void *priv, struct v4l2_exportbuffer *e);
    int (*vidioc_dqbuf)(struct file *file, void *priv, struct v4l2_buffer *b);
    int (*vidioc_create_bufs)(struct file *file, void *priv, struct v4l2_create_buffers *b);
    int (*vidioc_prepare_buf)(struct file *file, void *priv, struct v4l2_buffer *b);
    int (*vidioc_remove_bufs)(struct file *file, void *priv, struct v4l2_remove_buffers *d);
    int (*vidioc_overlay)(struct file *file, void *priv, unsigned int i);
    int (*vidioc_g_fbuf)(struct file *file, void *priv, struct v4l2_framebuffer *a);
    int (*vidioc_s_fbuf)(struct file *file, void *priv, const struct v4l2_framebuffer *a);
    int (*vidioc_streamon)(struct file *file, void *priv, enum v4l2_buf_type i);
    int (*vidioc_streamoff)(struct file *file, void *priv, enum v4l2_buf_type i);
    int (*vidioc_g_std)(struct file *file, void *priv, v4l2_std_id *norm);
    int (*vidioc_s_std)(struct file *file, void *priv, v4l2_std_id norm);
    int (*vidioc_querystd)(struct file *file, void *priv, v4l2_std_id *a);
    int (*vidioc_enum_input)(struct file *file, void *priv, struct v4l2_input *inp);
    int (*vidioc_g_input)(struct file *file, void *priv, unsigned int *i);
    int (*vidioc_s_input)(struct file *file, void *priv, unsigned int i);
    int (*vidioc_enum_output)(struct file *file, void *priv, struct v4l2_output *a);
    int (*vidioc_g_output)(struct file *file, void *priv, unsigned int *i);
    int (*vidioc_s_output)(struct file *file, void *priv, unsigned int i);
    int (*vidioc_query_ext_ctrl)(struct file *file, void *priv, struct v4l2_query_ext_ctrl *a);
    int (*vidioc_g_ext_ctrls)(struct file *file, void *priv, struct v4l2_ext_controls *a);
    int (*vidioc_s_ext_ctrls)(struct file *file, void *priv, struct v4l2_ext_controls *a);
    int (*vidioc_try_ext_ctrls)(struct file *file, void *priv, struct v4l2_ext_controls *a);
    int (*vidioc_querymenu)(struct file *file, void *priv, struct v4l2_querymenu *a);
    int (*vidioc_enumaudio)(struct file *file, void *priv, struct v4l2_audio *a);
    int (*vidioc_g_audio)(struct file *file, void *priv, struct v4l2_audio *a);
    int (*vidioc_s_audio)(struct file *file, void *priv, const struct v4l2_audio *a);
    int (*vidioc_enumaudout)(struct file *file, void *priv, struct v4l2_audioout *a);
    int (*vidioc_g_audout)(struct file *file, void *priv, struct v4l2_audioout *a);
    int (*vidioc_s_audout)(struct file *file, void *priv, const struct v4l2_audioout *a);
    int (*vidioc_g_modulator)(struct file *file, void *priv, struct v4l2_modulator *a);
    int (*vidioc_s_modulator)(struct file *file, void *priv, const struct v4l2_modulator *a);
    int (*vidioc_g_pixelaspect)(struct file *file, void *priv, int buf_type, struct v4l2_fract *aspect);
    int (*vidioc_g_selection)(struct file *file, void *priv, struct v4l2_selection *s);
    int (*vidioc_s_selection)(struct file *file, void *priv, struct v4l2_selection *s);
    int (*vidioc_g_jpegcomp)(struct file *file, void *priv, struct v4l2_jpegcompression *a);
    int (*vidioc_s_jpegcomp)(struct file *file, void *priv, const struct v4l2_jpegcompression *a);
    int (*vidioc_g_enc_index)(struct file *file, void *priv, struct v4l2_enc_idx *a);
    int (*vidioc_encoder_cmd)(struct file *file, void *priv, struct v4l2_encoder_cmd *a);
    int (*vidioc_try_encoder_cmd)(struct file *file, void *priv, struct v4l2_encoder_cmd *a);
    int (*vidioc_decoder_cmd)(struct file *file, void *priv, struct v4l2_decoder_cmd *a);
    int (*vidioc_try_decoder_cmd)(struct file *file, void *priv, struct v4l2_decoder_cmd *a);
    int (*vidioc_g_parm)(struct file *file, void *priv, struct v4l2_streamparm *a);
    int (*vidioc_s_parm)(struct file *file, void *priv, struct v4l2_streamparm *a);
    int (*vidioc_g_tuner)(struct file *file, void *priv, struct v4l2_tuner *a);
    int (*vidioc_s_tuner)(struct file *file, void *priv, const struct v4l2_tuner *a);
    int (*vidioc_g_frequency)(struct file *file, void *priv, struct v4l2_frequency *a);
    int (*vidioc_s_frequency)(struct file *file, void *priv, const struct v4l2_frequency *a);
    int (*vidioc_enum_freq_bands)(struct file *file, void *priv, struct v4l2_frequency_band *band);
    int (*vidioc_g_sliced_vbi_cap)(struct file *file, void *priv, struct v4l2_sliced_vbi_cap *a);
    int (*vidioc_log_status)(struct file *file, void *priv);
    int (*vidioc_s_hw_freq_seek)(struct file *file, void *priv, const struct v4l2_hw_freq_seek *a);
#ifdef CONFIG_VIDEO_ADV_DEBUG;
    int (*vidioc_g_register)(struct file *file, void *priv, struct v4l2_dbg_register *reg);
    int (*vidioc_s_register)(struct file *file, void *priv, const struct v4l2_dbg_register *reg);
    int (*vidioc_g_chip_info)(struct file *file, void *priv, struct v4l2_dbg_chip_info *chip);
#endif;
    int (*vidioc_enum_framesizes)(struct file *file, void *priv, struct v4l2_frmsizeenum *fsize);
    int (*vidioc_enum_frameintervals)(struct file *file, void *priv, struct v4l2_frmivalenum *fival);
    int (*vidioc_s_dv_timings)(struct file *file, void *priv, struct v4l2_dv_timings *timings);
    int (*vidioc_g_dv_timings)(struct file *file, void *priv, struct v4l2_dv_timings *timings);
    int (*vidioc_query_dv_timings)(struct file *file, void *priv, struct v4l2_dv_timings *timings);
    int (*vidioc_enum_dv_timings)(struct file *file, void *priv, struct v4l2_enum_dv_timings *timings);
    int (*vidioc_dv_timings_cap)(struct file *file, void *priv, struct v4l2_dv_timings_cap *cap);
    int (*vidioc_g_edid)(struct file *file, void *priv, struct v4l2_edid *edid);
    int (*vidioc_s_edid)(struct file *file, void *priv, struct v4l2_edid *edid);
    int (*vidioc_subscribe_event)(struct v4l2_fh *fh, const struct v4l2_event_subscription *sub);
    int (*vidioc_unsubscribe_event)(struct v4l2_fh *fh, const struct v4l2_event_subscription *sub);
    long (*vidioc_default)(struct file *file, void *priv, bool valid_prio, unsigned int cmd, void *arg);
};
```

**Members**

`vidioc_querycap`
:   pointer to the function that implements
    [VIDIOC\_QUERYCAP](../../userspace-api/media/v4l/vidioc-querycap.html#vidioc-querycap) ioctl

`vidioc_enum_fmt_vid_cap`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FMT](../../userspace-api/media/v4l/vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl logic
    for video capture in single and multi plane mode

`vidioc_enum_fmt_vid_overlay`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FMT](../../userspace-api/media/v4l/vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl logic
    for video overlay

`vidioc_enum_fmt_vid_out`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FMT](../../userspace-api/media/v4l/vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl logic
    for video output in single and multi plane mode

`vidioc_enum_fmt_sdr_cap`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FMT](../../userspace-api/media/v4l/vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl logic
    for Software Defined Radio capture

`vidioc_enum_fmt_sdr_out`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FMT](../../userspace-api/media/v4l/vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl logic
    for Software Defined Radio output

`vidioc_enum_fmt_meta_cap`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FMT](../../userspace-api/media/v4l/vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl logic
    for metadata capture

`vidioc_enum_fmt_meta_out`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FMT](../../userspace-api/media/v4l/vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl logic
    for metadata output

`vidioc_g_fmt_vid_cap`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video capture
    in single plane mode

`vidioc_g_fmt_vid_overlay`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video overlay

`vidioc_g_fmt_vid_out`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video out
    in single plane mode

`vidioc_g_fmt_vid_out_overlay`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video overlay output

`vidioc_g_fmt_vbi_cap`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for raw VBI capture

`vidioc_g_fmt_vbi_out`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for raw VBI output

`vidioc_g_fmt_sliced_vbi_cap`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for sliced VBI capture

`vidioc_g_fmt_sliced_vbi_out`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for sliced VBI output

`vidioc_g_fmt_vid_cap_mplane`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video capture
    in multiple plane mode

`vidioc_g_fmt_vid_out_mplane`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video out
    in multiplane plane mode

`vidioc_g_fmt_sdr_cap`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for Software Defined
    Radio capture

`vidioc_g_fmt_sdr_out`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for Software Defined
    Radio output

`vidioc_g_fmt_meta_cap`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for metadata capture

`vidioc_g_fmt_meta_out`
:   pointer to the function that implements
    [VIDIOC\_G\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for metadata output

`vidioc_s_fmt_vid_cap`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video capture
    in single plane mode

`vidioc_s_fmt_vid_overlay`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video overlay

`vidioc_s_fmt_vid_out`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video out
    in single plane mode

`vidioc_s_fmt_vid_out_overlay`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video overlay output

`vidioc_s_fmt_vbi_cap`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for raw VBI capture

`vidioc_s_fmt_vbi_out`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for raw VBI output

`vidioc_s_fmt_sliced_vbi_cap`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for sliced VBI capture

`vidioc_s_fmt_sliced_vbi_out`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for sliced VBI output

`vidioc_s_fmt_vid_cap_mplane`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video capture
    in multiple plane mode

`vidioc_s_fmt_vid_out_mplane`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video out
    in multiplane plane mode

`vidioc_s_fmt_sdr_cap`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for Software Defined
    Radio capture

`vidioc_s_fmt_sdr_out`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for Software Defined
    Radio output

`vidioc_s_fmt_meta_cap`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for metadata capture

`vidioc_s_fmt_meta_out`
:   pointer to the function that implements
    [VIDIOC\_S\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for metadata output

`vidioc_try_fmt_vid_cap`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video capture
    in single plane mode

`vidioc_try_fmt_vid_overlay`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video overlay

`vidioc_try_fmt_vid_out`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video out
    in single plane mode

`vidioc_try_fmt_vid_out_overlay`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video overlay
    output

`vidioc_try_fmt_vbi_cap`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for raw VBI capture

`vidioc_try_fmt_vbi_out`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for raw VBI output

`vidioc_try_fmt_sliced_vbi_cap`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for sliced VBI
    capture

`vidioc_try_fmt_sliced_vbi_out`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for sliced VBI output

`vidioc_try_fmt_vid_cap_mplane`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video capture
    in multiple plane mode

`vidioc_try_fmt_vid_out_mplane`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for video out
    in multiplane plane mode

`vidioc_try_fmt_sdr_cap`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for Software Defined
    Radio capture

`vidioc_try_fmt_sdr_out`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for Software Defined
    Radio output

`vidioc_try_fmt_meta_cap`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for metadata capture

`vidioc_try_fmt_meta_out`
:   pointer to the function that implements
    [VIDIOC\_TRY\_FMT](../../userspace-api/media/v4l/vidioc-g-fmt.html#vidioc-g-fmt) ioctl logic for metadata output

`vidioc_reqbufs`
:   pointer to the function that implements
    [VIDIOC\_REQBUFS](../../userspace-api/media/v4l/vidioc-reqbufs.html#vidioc-reqbufs) ioctl

`vidioc_querybuf`
:   pointer to the function that implements
    [VIDIOC\_QUERYBUF](../../userspace-api/media/v4l/vidioc-querybuf.html#vidioc-querybuf) ioctl

`vidioc_qbuf`
:   pointer to the function that implements
    [VIDIOC\_QBUF](../../userspace-api/media/v4l/vidioc-qbuf.html#vidioc-qbuf) ioctl

`vidioc_expbuf`
:   pointer to the function that implements
    [VIDIOC\_EXPBUF](../../userspace-api/media/v4l/vidioc-expbuf.html#vidioc-expbuf) ioctl

`vidioc_dqbuf`
:   pointer to the function that implements
    [VIDIOC\_DQBUF](../../userspace-api/media/v4l/vidioc-qbuf.html#vidioc-qbuf) ioctl

`vidioc_create_bufs`
:   pointer to the function that implements
    [VIDIOC\_CREATE\_BUFS](../../userspace-api/media/v4l/vidioc-create-bufs.html#vidioc-create-bufs) ioctl

`vidioc_prepare_buf`
:   pointer to the function that implements
    [VIDIOC\_PREPARE\_BUF](../../userspace-api/media/v4l/vidioc-prepare-buf.html#vidioc-prepare-buf) ioctl

`vidioc_remove_bufs`
:   pointer to the function that implements
    [VIDIOC\_REMOVE\_BUFS](../../userspace-api/media/v4l/vidioc-remove-bufs.html#vidioc-remove-bufs) ioctl

`vidioc_overlay`
:   pointer to the function that implements
    [VIDIOC\_OVERLAY](../../userspace-api/media/v4l/vidioc-overlay.html#vidioc-overlay) ioctl

`vidioc_g_fbuf`
:   pointer to the function that implements
    [VIDIOC\_G\_FBUF](../../userspace-api/media/v4l/vidioc-g-fbuf.html#vidioc-g-fbuf) ioctl

`vidioc_s_fbuf`
:   pointer to the function that implements
    [VIDIOC\_S\_FBUF](../../userspace-api/media/v4l/vidioc-g-fbuf.html#vidioc-g-fbuf) ioctl

`vidioc_streamon`
:   pointer to the function that implements
    [VIDIOC\_STREAMON](../../userspace-api/media/v4l/vidioc-streamon.html#vidioc-streamon) ioctl

`vidioc_streamoff`
:   pointer to the function that implements
    [VIDIOC\_STREAMOFF](../../userspace-api/media/v4l/vidioc-streamon.html#vidioc-streamon) ioctl

`vidioc_g_std`
:   pointer to the function that implements
    [VIDIOC\_G\_STD](../../userspace-api/media/v4l/vidioc-g-std.html#vidioc-g-std) ioctl

`vidioc_s_std`
:   pointer to the function that implements
    [VIDIOC\_S\_STD](../../userspace-api/media/v4l/vidioc-g-std.html#vidioc-g-std) ioctl

`vidioc_querystd`
:   pointer to the function that implements
    [VIDIOC\_QUERYSTD](../../userspace-api/media/v4l/vidioc-querystd.html#vidioc-querystd) ioctl

`vidioc_enum_input`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_INPUT](../../userspace-api/media/v4l/vidioc-g-input.html#vidioc-g-input) ioctl

`vidioc_g_input`
:   pointer to the function that implements
    [VIDIOC\_G\_INPUT](../../userspace-api/media/v4l/vidioc-g-input.html#vidioc-g-input) ioctl

`vidioc_s_input`
:   pointer to the function that implements
    [VIDIOC\_S\_INPUT](../../userspace-api/media/v4l/vidioc-g-input.html#vidioc-g-input) ioctl

`vidioc_enum_output`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_OUTPUT](../../userspace-api/media/v4l/vidioc-g-output.html#vidioc-g-output) ioctl

`vidioc_g_output`
:   pointer to the function that implements
    [VIDIOC\_G\_OUTPUT](../../userspace-api/media/v4l/vidioc-g-output.html#vidioc-g-output) ioctl

`vidioc_s_output`
:   pointer to the function that implements
    [VIDIOC\_S\_OUTPUT](../../userspace-api/media/v4l/vidioc-g-output.html#vidioc-g-output) ioctl

`vidioc_query_ext_ctrl`
:   pointer to the function that implements
    [VIDIOC\_QUERY\_EXT\_CTRL](../../userspace-api/media/v4l/vidioc-queryctrl.html#vidioc-queryctrl) ioctl

`vidioc_g_ext_ctrls`
:   pointer to the function that implements
    [VIDIOC\_G\_EXT\_CTRLS](../../userspace-api/media/v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) ioctl

`vidioc_s_ext_ctrls`
:   pointer to the function that implements
    [VIDIOC\_S\_EXT\_CTRLS](../../userspace-api/media/v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) ioctl

`vidioc_try_ext_ctrls`
:   pointer to the function that implements
    [VIDIOC\_TRY\_EXT\_CTRLS](../../userspace-api/media/v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) ioctl

`vidioc_querymenu`
:   pointer to the function that implements
    [VIDIOC\_QUERYMENU](../../userspace-api/media/v4l/vidioc-queryctrl.html#vidioc-queryctrl) ioctl

`vidioc_enumaudio`
:   pointer to the function that implements
    [VIDIOC\_ENUMAUDIO](../../userspace-api/media/v4l/vidioc-enumaudio.html#vidioc-enumaudio) ioctl

`vidioc_g_audio`
:   pointer to the function that implements
    [VIDIOC\_G\_AUDIO](../../userspace-api/media/v4l/vidioc-g-audio.html#vidioc-g-audio) ioctl

`vidioc_s_audio`
:   pointer to the function that implements
    [VIDIOC\_S\_AUDIO](../../userspace-api/media/v4l/vidioc-g-audio.html#vidioc-g-audio) ioctl

`vidioc_enumaudout`
:   pointer to the function that implements
    [VIDIOC\_ENUMAUDOUT](../../userspace-api/media/v4l/vidioc-enumaudioout.html#vidioc-enumaudout) ioctl

`vidioc_g_audout`
:   pointer to the function that implements
    [VIDIOC\_G\_AUDOUT](../../userspace-api/media/v4l/vidioc-g-audioout.html#vidioc-g-audout) ioctl

`vidioc_s_audout`
:   pointer to the function that implements
    [VIDIOC\_S\_AUDOUT](../../userspace-api/media/v4l/vidioc-g-audioout.html#vidioc-g-audout) ioctl

`vidioc_g_modulator`
:   pointer to the function that implements
    [VIDIOC\_G\_MODULATOR](../../userspace-api/media/v4l/vidioc-g-modulator.html#vidioc-g-modulator) ioctl

`vidioc_s_modulator`
:   pointer to the function that implements
    [VIDIOC\_S\_MODULATOR](../../userspace-api/media/v4l/vidioc-g-modulator.html#vidioc-g-modulator) ioctl

`vidioc_g_pixelaspect`
:   pointer to the function that implements
    the pixelaspect part of the [VIDIOC\_CROPCAP](../../userspace-api/media/v4l/vidioc-cropcap.html#vidioc-cropcap) ioctl

`vidioc_g_selection`
:   pointer to the function that implements
    [VIDIOC\_G\_SELECTION](../../userspace-api/media/v4l/vidioc-g-selection.html#vidioc-g-selection) ioctl

`vidioc_s_selection`
:   pointer to the function that implements
    [VIDIOC\_S\_SELECTION](../../userspace-api/media/v4l/vidioc-g-selection.html#vidioc-g-selection) ioctl

`vidioc_g_jpegcomp`
:   pointer to the function that implements
    [VIDIOC\_G\_JPEGCOMP](../../userspace-api/media/v4l/vidioc-g-jpegcomp.html#vidioc-g-jpegcomp) ioctl

`vidioc_s_jpegcomp`
:   pointer to the function that implements
    [VIDIOC\_S\_JPEGCOMP](../../userspace-api/media/v4l/vidioc-g-jpegcomp.html#vidioc-g-jpegcomp) ioctl

`vidioc_g_enc_index`
:   pointer to the function that implements
    [VIDIOC\_G\_ENC\_INDEX](../../userspace-api/media/v4l/vidioc-g-enc-index.html#vidioc-g-enc-index) ioctl

`vidioc_encoder_cmd`
:   pointer to the function that implements
    [VIDIOC\_ENCODER\_CMD](../../userspace-api/media/v4l/vidioc-encoder-cmd.html#vidioc-encoder-cmd) ioctl

`vidioc_try_encoder_cmd`
:   pointer to the function that implements
    [VIDIOC\_TRY\_ENCODER\_CMD](../../userspace-api/media/v4l/vidioc-encoder-cmd.html#vidioc-encoder-cmd) ioctl

`vidioc_decoder_cmd`
:   pointer to the function that implements
    [VIDIOC\_DECODER\_CMD](../../userspace-api/media/v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) ioctl

`vidioc_try_decoder_cmd`
:   pointer to the function that implements
    [VIDIOC\_TRY\_DECODER\_CMD](../../userspace-api/media/v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) ioctl

`vidioc_g_parm`
:   pointer to the function that implements
    [VIDIOC\_G\_PARM](../../userspace-api/media/v4l/vidioc-g-parm.html#vidioc-g-parm) ioctl

`vidioc_s_parm`
:   pointer to the function that implements
    [VIDIOC\_S\_PARM](../../userspace-api/media/v4l/vidioc-g-parm.html#vidioc-g-parm) ioctl

`vidioc_g_tuner`
:   pointer to the function that implements
    [VIDIOC\_G\_TUNER](../../userspace-api/media/v4l/vidioc-g-tuner.html#vidioc-g-tuner) ioctl

`vidioc_s_tuner`
:   pointer to the function that implements
    [VIDIOC\_S\_TUNER](../../userspace-api/media/v4l/vidioc-g-tuner.html#vidioc-g-tuner) ioctl

`vidioc_g_frequency`
:   pointer to the function that implements
    [VIDIOC\_G\_FREQUENCY](../../userspace-api/media/v4l/vidioc-g-frequency.html#vidioc-g-frequency) ioctl

`vidioc_s_frequency`
:   pointer to the function that implements
    [VIDIOC\_S\_FREQUENCY](../../userspace-api/media/v4l/vidioc-g-frequency.html#vidioc-g-frequency) ioctl

`vidioc_enum_freq_bands`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FREQ\_BANDS](../../userspace-api/media/v4l/vidioc-enum-freq-bands.html#vidioc-enum-freq-bands) ioctl

`vidioc_g_sliced_vbi_cap`
:   pointer to the function that implements
    [VIDIOC\_G\_SLICED\_VBI\_CAP](../../userspace-api/media/v4l/vidioc-g-sliced-vbi-cap.html#vidioc-g-sliced-vbi-cap) ioctl

`vidioc_log_status`
:   pointer to the function that implements
    [VIDIOC\_LOG\_STATUS](../../userspace-api/media/v4l/vidioc-log-status.html#vidioc-log-status) ioctl

`vidioc_s_hw_freq_seek`
:   pointer to the function that implements
    [VIDIOC\_S\_HW\_FREQ\_SEEK](../../userspace-api/media/v4l/vidioc-s-hw-freq-seek.html#vidioc-s-hw-freq-seek) ioctl

`vidioc_g_register`
:   pointer to the function that implements
    [VIDIOC\_DBG\_G\_REGISTER](../../userspace-api/media/v4l/vidioc-dbg-g-register.html#vidioc-dbg-g-register) ioctl

`vidioc_s_register`
:   pointer to the function that implements
    [VIDIOC\_DBG\_S\_REGISTER](../../userspace-api/media/v4l/vidioc-dbg-g-register.html#vidioc-dbg-g-register) ioctl

`vidioc_g_chip_info`
:   pointer to the function that implements
    [VIDIOC\_DBG\_G\_CHIP\_INFO](../../userspace-api/media/v4l/vidioc-dbg-g-chip-info.html#vidioc-dbg-g-chip-info) ioctl

`vidioc_enum_framesizes`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FRAMESIZES](../../userspace-api/media/v4l/vidioc-enum-framesizes.html#vidioc-enum-framesizes) ioctl

`vidioc_enum_frameintervals`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_FRAMEINTERVALS](../../userspace-api/media/v4l/vidioc-enum-frameintervals.html#vidioc-enum-frameintervals) ioctl

`vidioc_s_dv_timings`
:   pointer to the function that implements
    [VIDIOC\_S\_DV\_TIMINGS](../../userspace-api/media/v4l/vidioc-g-dv-timings.html#vidioc-g-dv-timings) ioctl

`vidioc_g_dv_timings`
:   pointer to the function that implements
    [VIDIOC\_G\_DV\_TIMINGS](../../userspace-api/media/v4l/vidioc-g-dv-timings.html#vidioc-g-dv-timings) ioctl

`vidioc_query_dv_timings`
:   pointer to the function that implements
    [VIDIOC\_QUERY\_DV\_TIMINGS](../../userspace-api/media/v4l/vidioc-query-dv-timings.html#vidioc-query-dv-timings) ioctl

`vidioc_enum_dv_timings`
:   pointer to the function that implements
    [VIDIOC\_ENUM\_DV\_TIMINGS](../../userspace-api/media/v4l/vidioc-enum-dv-timings.html#vidioc-enum-dv-timings) ioctl

`vidioc_dv_timings_cap`
:   pointer to the function that implements
    [VIDIOC\_DV\_TIMINGS\_CAP](../../userspace-api/media/v4l/vidioc-dv-timings-cap.html#vidioc-dv-timings-cap) ioctl

`vidioc_g_edid`
:   pointer to the function that implements
    [VIDIOC\_G\_EDID](../../userspace-api/media/v4l/vidioc-g-edid.html#vidioc-g-edid) ioctl

`vidioc_s_edid`
:   pointer to the function that implements
    [VIDIOC\_S\_EDID](../../userspace-api/media/v4l/vidioc-g-edid.html#vidioc-g-edid) ioctl

`vidioc_subscribe_event`
:   pointer to the function that implements
    [VIDIOC\_SUBSCRIBE\_EVENT](../../userspace-api/media/v4l/vidioc-subscribe-event.html#vidioc-subscribe-event) ioctl

`vidioc_unsubscribe_event`
:   pointer to the function that implements
    [VIDIOC\_UNSUBSCRIBE\_EVENT](../../userspace-api/media/v4l/vidioc-subscribe-event.html#vidioc-unsubscribe-event) ioctl

`vidioc_default`
:   pointed used to allow other ioctls

const char \*v4l2\_norm\_to\_name(v4l2\_std\_id id)
:   Ancillary routine to analog TV standard name from its ID.

**Parameters**

`v4l2_std_id id`
:   analog TV standard ID.

**Return**

returns a string with the name of the analog TV standard.
If the standard is not found or if **id** points to multiple standard,
it returns “Unknown”.

void v4l2\_video\_std\_frame\_period(int id, struct v4l2\_fract \*frameperiod)
:   Ancillary routine that fills a struct `v4l2_fract` pointer with the default framerate fraction.

**Parameters**

`int id`
:   analog TV standard ID.

`struct v4l2_fract *frameperiod`
:   struct `v4l2_fract` pointer to be filled

int v4l2\_video\_std\_construct(struct v4l2\_standard \*vs, int id, const char \*name)
:   Ancillary routine that fills in the fields of a `v4l2_standard` structure according to the **id** parameter.

**Parameters**

`struct v4l2_standard *vs`
:   struct `v4l2_standard` pointer to be filled

`int id`
:   analog TV standard ID.

`const char *name`
:   name of the standard to be used

**Description**

Note

This ancillary routine is obsolete. Shouldn’t be used on newer drivers.

int v4l\_video\_std\_enumstd(struct v4l2\_standard \*vs, v4l2\_std\_id id)
:   Ancillary routine that fills in the fields of a `v4l2_standard` structure according to the **id** and **vs->index** parameters.

**Parameters**

`struct v4l2_standard *vs`
:   struct `v4l2_standard` pointer to be filled.

`v4l2_std_id id`
:   analog TV standard ID.

void v4l\_printk\_ioctl(const char \*prefix, unsigned int cmd)
:   Ancillary routine that prints the ioctl in a human-readable format.

**Parameters**

`const char *prefix`
:   prefix to be added at the ioctl prints.

`unsigned int cmd`
:   ioctl name

**Description**

Note

If prefix != `NULL`, then it will issue a
``` printk(KERN_DEBUG "``s ```: “, prefix)`` first.

v4l2\_field\_names
:   `extern const char *v4l2_field_names[];`

    Helper array mapping `V4L2_FIELD_*` to strings.

    **Description**

    Specially when printing debug messages, it is interesting to output
    the field order at the V4L2 buffers. This array associates all possible
    values of field pix format from V4L2 API into a string.

v4l2\_type\_names
:   `extern const char *v4l2_type_names[];`

    Helper array mapping `V4L2_BUF_TYPE_*` to strings.

    **Description**

    When printing debug messages, it is interesting to output the V4L2 buffer
    type number with a name that represents its content.

long int v4l2\_compat\_ioctl32(struct [file](#c.v4l2_compat_ioctl32 "file") \*file, unsigned int cmd, unsigned long arg)
:   32 Bits compatibility layer for 64 bits processors

**Parameters**

`struct file *file`
:   Pointer to struct [`file`](../../filesystems/api-summary.html#c.file "file").

`unsigned int cmd`
:   Ioctl name.

`unsigned long arg`
:   Ioctl argument.

v4l2\_kioctl
:   **Typedef**: Typedef used to pass an ioctl handler.

**Syntax**

> `long v4l2_kioctl (struct file *file, unsigned int cmd, void *arg)`

**Parameters**

`struct file *file`
:   Pointer to struct [`file`](../../filesystems/api-summary.html#c.file "file").

`unsigned int cmd`
:   Ioctl name.

`void *arg`
:   Ioctl argument.

long int video\_usercopy(struct [file](#c.video_usercopy "file") \*file, unsigned int cmd, unsigned long int arg, [v4l2\_kioctl](#c.v4l2_kioctl "v4l2_kioctl") func)
:   copies data from/to userspace memory when an ioctl is issued.

**Parameters**

`struct file *file`
:   Pointer to struct [`file`](../../filesystems/api-summary.html#c.file "file").

`unsigned int cmd`
:   Ioctl name.

`unsigned long int arg`
:   Ioctl argument.

`v4l2_kioctl func`
:   function that will handle the ioctl

**Description**

Note

This routine should be used only inside the V4L2 core.

long int video\_ioctl2(struct [file](#c.video_ioctl2 "file") \*file, unsigned int cmd, unsigned long int arg)
:   Handles a V4L2 ioctl.

**Parameters**

`struct file *file`
:   Pointer to struct [`file`](../../filesystems/api-summary.html#c.file "file").

`unsigned int cmd`
:   Ioctl name.

`unsigned long int arg`
:   Ioctl argument.

**Description**

Method used to hancle an ioctl. Should be used to fill the
[`v4l2_ioctl_ops.unlocked_ioctl`](#c.v4l2_ioctl_ops "v4l2_ioctl_ops") on all V4L2 drivers.
