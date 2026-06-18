# 4.8.Radio Interface

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-radio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.8. Radio Interface

This interface is intended for AM and FM (analog) radio receivers and
transmitters.

Conventionally V4L2 radio devices are accessed through character device
special files named `/dev/radio` and `/dev/radio0` to
`/dev/radio63` with major number 81 and minor numbers 64 to 127.

## 4.8.1. Querying Capabilities

Devices supporting the radio interface set the `V4L2_CAP_RADIO` and
`V4L2_CAP_TUNER` or `V4L2_CAP_MODULATOR` flag in the
`capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. Other combinations of
capability flags are reserved for future extensions.

## 4.8.2. Supplemental Functions

Radio devices can support [controls](control.html#control), and must support
the [tuner or modulator](tuner.html#tuner) ioctls.

They do not support the video input or output, audio input or output,
video standard, cropping and scaling, compression and streaming
parameter, or overlay ioctls. All other ioctls and I/O methods are
reserved for future extensions.

## 4.8.3. Programming

Radio devices may have a couple audio controls (as discussed in
[User Controls](control.html#control)) such as a volume control, possibly custom controls.
Further all radio devices have one tuner or modulator (these are
discussed in [Tuners and Modulators](tuner.html#tuner)) with index number zero to select the radio
frequency and to determine if a monaural or FM stereo program is
received/emitted. Drivers switch automatically between AM and FM
depending on the selected frequency. The
[VIDIOC\_G\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner) or
[VIDIOC\_G\_MODULATOR](vidioc-g-modulator.html#vidioc-g-modulator) ioctl reports the
supported frequency range.
