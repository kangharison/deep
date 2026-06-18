# 8.Pixel data transmitter and receiver drivers

> 출처(원문): https://docs.kernel.org/driver-api/media/tx-rx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 8. Pixel data transmitter and receiver drivers

V4L2 supports various devices that transmit and receive pixel data. Examples of
these devices include a camera sensor, a TV tuner and a parallel, a BT.656 or a
CSI-2 receiver in an SoC.

## 8.1. Bus types

The following buses are the most common. This section discusses these two only.

### 8.1.1. MIPI CSI-2

CSI-2 is a data bus intended for transferring images from cameras to
the host SoC. It is defined by the [MIPI alliance](https://www.mipi.org/).

### 8.1.2. Parallel and BT.656

The parallel and [BT.656](https://en.wikipedia.org/wiki/ITU-R_BT.656) buses transport one bit of data on each clock cycle
per data line. The parallel bus uses synchronisation and other additional
signals whereas BT.656 embeds synchronisation.

## 8.2. Transmitter drivers

Transmitter drivers generally need to provide the receiver drivers with the
configuration of the transmitter. What is required depends on the type of the
bus. These are common for both buses.

### 8.2.1. Media bus pixel code

See [Media Bus Pixel Codes](../../userspace-api/media/v4l/subdev-formats.html#v4l2-mbus-pixelcode).

### 8.2.2. Link frequency

The [V4L2\_CID\_LINK\_FREQ](../../userspace-api/media/v4l/ext-ctrls-image-process.html#v4l2-cid-link-freq) control is used to tell the
receiver the frequency of the bus (i.e. it is not the same as the symbol rate).

Drivers that do not have user-configurable link frequency should report it
through the `.get_mbus_config()` subdev pad operation, in the `link_freq`
field of [`struct v4l2_mbus_config`](v4l2-mediabus.html#c.v4l2_mbus_config "v4l2_mbus_config"), instead of through controls.

Receiver drivers should use [`v4l2_get_link_freq()`](v4l2-common.html#c.v4l2_get_link_freq "v4l2_get_link_freq") helper to obtain the
link frequency from the transmitter sub-device.

### 8.2.3. `.enable_streams()` and `.disable_streams()` callbacks

The [`struct v4l2_subdev_pad_ops`](v4l2-subdev.html#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops")->`enable_streams()` and [`struct
v4l2_subdev_pad_ops`](v4l2-subdev.html#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops")->`disable_streams()` callbacks are used by the receiver driver
to control the transmitter driver’s streaming state. These callbacks may not be
called directly, but by using `v4l2_subdev_enable_streams()` and
`v4l2_subdev_disable_streams()`.

### 8.2.4. Stopping the transmitter

A transmitter stops sending the stream of images as a result of
calling the `.disable_streams()` callback. Some transmitters may stop the
stream at a frame boundary whereas others stop immediately,
effectively leaving the current frame unfinished. The receiver driver
should not make assumptions either way, but function properly in both
cases.

## 8.3. CSI-2 transmitter drivers

### 8.3.1. Pixel rate

The pixel rate on the bus is calculated as follows:

```
pixel_rate = link_freq * 2 * nr_of_lanes * 16 / k / bits_per_sample
```

where

variables in pixel rate calculation

| variable or constant | description |
| --- | --- |
| link\_freq | The value of the `V4L2_CID_LINK_FREQ` integer64 menu item. |
| nr\_of\_lanes | Number of data lanes used on the CSI-2 link. |
| 2 | Data is transferred on both rising and falling edge of the signal. |
| bits\_per\_sample | Number of bits per sample. |
| k | 16 for D-PHY and 7 for C-PHY. |

Information on whether D-PHY or C-PHY is used, and the value of `nr_of_lanes`, can be obtained from the OF endpoint configuration.

Note

The pixel rate calculated this way is **not** the same thing as the
pixel rate on the camera sensor’s pixel array which is indicated by the
[V4L2\_CID\_PIXEL\_RATE](../../userspace-api/media/v4l/ext-ctrls-image-process.html#v4l2-cid-pixel-rate) control.

### 8.3.2. LP-11 and LP-111 states

As part of transitioning to high speed mode, a CSI-2 transmitter typically
briefly sets the bus to LP-11 or LP-111 state, depending on the PHY. This period
may be as short as 100 µs, during which the receiver observes this state and
proceeds its own part of high speed mode transition.

Most receivers are capable of autonomously handling this once the software has
configured them to do so, but there are receivers which require software
involvement in observing LP-11 or LP-111 state. 100 µs is a brief period to hit
in software, especially when there is no interrupt telling something is
happening.

One way to address this is to configure the transmitter side explicitly to LP-11
or LP-111 state, which requires support from the transmitter hardware. This is
not universally available. Many devices return to this state once streaming is
stopped while the state after power-on is LP-00 or LP-000.

The `.pre_streamon()` callback may be used to prepare a transmitter for
transitioning to streaming state, but not yet start streaming. Similarly, the
`.post_streamoff()` callback is used to undo what was done by the
`.pre_streamon()` callback. The caller of `.pre_streamon()` is thus required
to call `.post_streamoff()` for each successful call of `.pre_streamon()`.

In the context of CSI-2, the `.pre_streamon()` callback is used to transition
the transmitter to the LP-11 or LP-111 state. This also requires powering on the
device, so this should be only done when it is needed.

Receiver drivers that do not need explicit LP-11 or LP-111 state setup are
waived from calling the two callbacks.
