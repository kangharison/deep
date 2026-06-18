# 1.6.Tuners and Modulators

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/tuner.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.6. Tuners and Modulators

## 1.6.1. Tuners

Video input devices can have one or more tuners demodulating a RF
signal. Each tuner is associated with one or more video inputs,
depending on the number of RF connectors on the tuner. The `type`
field of the respective struct [`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input")
returned by the [ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) ioctl is
set to `V4L2_INPUT_TYPE_TUNER` and its `tuner` field contains the
index number of the tuner.

Radio input devices have exactly one tuner with index zero, no video
inputs.

To query and change tuner properties applications use the
[VIDIOC\_G\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner) and
[VIDIOC\_S\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner) ioctls, respectively. The
struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") returned by [VIDIOC\_G\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner)
also contains signal status information applicable when the tuner of the
current video or radio input is queried.

Note

[VIDIOC\_S\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner) does not switch the
current tuner, when there is more than one. The tuner is solely
determined by the current video input. Drivers must support both ioctls
and set the `V4L2_CAP_TUNER` flag in the struct [`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability")
returned by the [ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl when the
device has one or more tuners.

## 1.6.2. Modulators

Video output devices can have one or more modulators, that modulate a
video signal for radiation or connection to the antenna input of a TV
set or video recorder. Each modulator is associated with one or more
video outputs, depending on the number of RF connectors on the
modulator. The `type` field of the respective struct
[`v4l2_output`](vidioc-enumoutput.html#c.V4L.v4l2_output "v4l2_output") returned by the
[ioctl VIDIOC\_ENUMOUTPUT](vidioc-enumoutput.html#vidioc-enumoutput) ioctl is set to
`V4L2_OUTPUT_TYPE_MODULATOR` and its `modulator` field contains the
index number of the modulator.

Radio output devices have exactly one modulator with index zero, no
video outputs.

A video or radio device cannot support both a tuner and a modulator. Two
separate device nodes will have to be used for such hardware, one that
supports the tuner functionality and one that supports the modulator
functionality. The reason is a limitation with the
[VIDIOC\_S\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency) ioctl where you
cannot specify whether the frequency is for a tuner or a modulator.

To query and change modulator properties applications use the
[VIDIOC\_G\_MODULATOR](vidioc-g-modulator.html#vidioc-g-modulator) and
[VIDIOC\_S\_MODULATOR](vidioc-g-modulator.html#vidioc-g-modulator) ioctl. Note that
[VIDIOC\_S\_MODULATOR](vidioc-g-modulator.html#vidioc-g-modulator) does not switch the current modulator, when there
is more than one at all. The modulator is solely determined by the
current video output. Drivers must support both ioctls and set the
`V4L2_CAP_MODULATOR` flag in the struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl when the device has
one or more modulators.

## 1.6.3. Radio Frequency

To get and set the tuner or modulator radio frequency applications use
the [VIDIOC\_G\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency) and
[VIDIOC\_S\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency) ioctl which both take
a pointer to a struct [`v4l2_frequency`](vidioc-g-frequency.html#c.V4L.v4l2_frequency "v4l2_frequency"). These
ioctls are used for TV and radio devices alike. Drivers must support
both ioctls when the tuner or modulator ioctls are supported, or when
the device is a radio device.
