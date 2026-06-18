# 2.Using camera sensor drivers

> 출처(원문): https://docs.kernel.org/userspace-api/media/drivers/camera-sensor.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. Using camera sensor drivers

This section describes common practices for how the V4L2 sub-device interface is
used to control the camera sensor drivers.

You may also find [Writing camera sensor drivers](../../../driver-api/media/camera-sensor.html#media-writing-camera-sensor-drivers) useful.

## 2.1. Sensor internal pipeline configuration

Camera sensors have an internal processing pipeline including cropping and
binning functionality. The sensor drivers belong to two distinct classes, freely
configurable and register list-based drivers, depending on how the driver
configures this functionality.

### 2.1.1. Freely configurable camera sensor drivers

Freely configurable camera sensor drivers expose the device’s internal
processing pipeline as one or more sub-devices with different cropping and
scaling configurations. The output size of the device is the result of a series
of cropping and scaling operations from the device’s pixel array’s size.

An example of such a driver is the CCS driver.

### 2.1.2. Register list-based drivers

Register list-based drivers generally, instead of able to configure the device
they control based on user requests, are limited to a number of preset
configurations that combine a number of different parameters that on hardware
level are independent. How a driver picks such configuration is based on the
format set on a source pad at the end of the device’s internal pipeline.

Most sensor drivers are implemented this way.

## 2.2. Frame interval configuration

There are two different methods for obtaining possibilities for different frame
intervals as well as configuring the frame interval. Which one to implement
depends on the type of the device.

### 2.2.1. Raw camera sensors

Instead of a high level parameter such as frame interval, the frame interval is
a result of the configuration of a number of camera sensor implementation
specific parameters. Luckily, these parameters tend to be the same for more or
less all modern raw camera sensors.

The frame interval is calculated using the following equation:

```
frame interval = (analogue crop width + horizontal blanking) *
                 (analogue crop height + vertical blanking) / pixel rate
```

The formula is bus independent and is applicable for raw timing parameters on
large variety of devices beyond camera sensors. Devices that have no analogue
crop, use the full source image size, i.e. pixel array size.

Horizontal and vertical blanking are specified by `V4L2_CID_HBLANK` and
`V4L2_CID_VBLANK`, respectively. The unit of the `V4L2_CID_HBLANK` control
is pixels and the unit of the `V4L2_CID_VBLANK` is lines. The pixel rate in
the sensor’s **pixel array** is specified by `V4L2_CID_PIXEL_RATE` in the same
sub-device. The unit of that control is pixels per second.

Register list-based drivers need to implement read-only sub-device nodes for the
purpose. Devices that are not register list based need these to configure the
device’s internal processing pipeline.

The first entity in the linear pipeline is the pixel array. The pixel array may
be followed by other entities that are there to allow configuring binning,
skipping, scaling or digital crop, see [VIDIOC\_SUBDEV\_G\_SELECTION](../v4l/vidioc-subdev-g-selection.html#vidioc-subdev-g-selection).

### 2.2.2. USB cameras etc. devices

USB video class hardware, as well as many cameras offering a similar higher
level interface natively, generally use the concept of frame interval (or frame
rate) on device level in firmware or hardware. This means lower level controls
implemented by raw cameras may not be used on uAPI (or even kAPI) to control the
frame interval on these devices.

## 2.3. Rotation, orientation and flipping

Some systems have the camera sensor mounted upside down compared to its natural
mounting rotation. In such cases, drivers shall expose the information to
userspace with the [V4L2\_CID\_CAMERA\_SENSOR\_ROTATION](../v4l/ext-ctrls-camera.html#v4l2-camera-sensor-rotation) control.

Sensor drivers shall also report the sensor’s mounting orientation with the
[V4L2\_CID\_CAMERA\_SENSOR\_ORIENTATION](../v4l/ext-ctrls-camera.html#v4l2-camera-sensor-orientation).

Sensor drivers that have any vertical or horizontal flips embedded in the
register programming sequences shall initialize the [V4L2\_CID\_HFLIP](../v4l/control.html#v4l2-cid-hflip) and [V4L2\_CID\_VFLIP](../v4l/control.html#v4l2-cid-vflip) controls with the
values programmed by the register sequences. The default values of these
controls shall be 0 (disabled). Especially these controls shall not be inverted,
independently of the sensor’s mounting rotation.
