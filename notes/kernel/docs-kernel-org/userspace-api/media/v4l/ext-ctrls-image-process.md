# 1.14.Image Process Control Reference

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/ext-ctrls-image-process.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.14. Image Process Control Reference

The Image Process control class is intended for low-level control of
image processing functions. Unlike `V4L2_CID_IMAGE_SOURCE_CLASS`, the
controls in this class affect processing the image, and do not control
capturing of it.

## 1.14.1. Image Process Control IDs

`V4L2_CID_IMAGE_PROC_CLASS (class)`
:   The IMAGE\_PROC class descriptor.

`V4L2_CID_LINK_FREQ (integer menu)`
:   The frequency of the data bus (e.g. parallel or CSI-2).

`V4L2_CID_PIXEL_RATE (64-bit integer)`
:   Pixel sampling rate in the device’s pixel array. This control is
    read-only and its unit is pixels / second.

    Some devices use horizontal and vertical blanking to configure the frame
    rate. The frame rate can be calculated from the pixel rate, analogue crop
    rectangle as well as horizontal and vertical blanking. The pixel rate
    control may be present in a different sub-device than the blanking controls
    and the analogue crop rectangle configuration.

    The configuration of the frame rate is performed by selecting the desired
    horizontal and vertical blanking. The unit of this control is Hz.

`V4L2_CID_TEST_PATTERN (menu)`
:   Some capture/display/sensor devices have the capability to generate
    test pattern images. These hardware specific test patterns can be
    used to test if a device is working properly.

`V4L2_CID_DEINTERLACING_MODE (menu)`
:   The video deinterlacing mode (such as Bob, Weave, ...). The menu items are
    driver specific and are documented in [Video4Linux (V4L) driver-specific documentation](../../../admin-guide/media/v4l-drivers.html#uapi-v4l-drivers).

`V4L2_CID_DIGITAL_GAIN (integer)`
:   Digital gain is the value by which all colour components
    are multiplied by. Typically the digital gain applied is the
    control value divided by e.g. 0x100, meaning that to get no
    digital gain the control value needs to be 0x100. The no-gain
    configuration is also typically the default.
