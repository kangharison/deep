# 7.32.ioctl VIDIOC_G_FREQUENCY, VIDIOC_S_FREQUENCY

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-frequency.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.32. ioctl VIDIOC\_G\_FREQUENCY, VIDIOC\_S\_FREQUENCY

## 7.32.1. Name

VIDIOC\_G\_FREQUENCY - VIDIOC\_S\_FREQUENCY - Get or set tuner or modulator radio frequency

## 7.32.2. Synopsis

VIDIOC\_G\_FREQUENCY

`int ioctl(int fd, VIDIOC_G_FREQUENCY, struct v4l2_frequency *argp)`

VIDIOC\_S\_FREQUENCY

`int ioctl(int fd, VIDIOC_S_FREQUENCY, const struct v4l2_frequency *argp)`

## 7.32.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_frequency`](#c.V4L.v4l2_frequency "v4l2_frequency").

## 7.32.4. Description

To get the current tuner or modulator radio frequency applications set
the `tuner` field of a struct
[`v4l2_frequency`](#c.V4L.v4l2_frequency "v4l2_frequency") to the respective tuner or
modulator number (only input devices have tuners, only output devices
have modulators), zero out the `reserved` array and call the
[VIDIOC\_G\_FREQUENCY](#vidioc-g-frequency) ioctl with a pointer to this structure. The
driver stores the current frequency in the `frequency` field.

To change the current tuner or modulator radio frequency applications
initialize the `tuner`, `type` and `frequency` fields, and the
`reserved` array of a struct [`v4l2_frequency`](#c.V4L.v4l2_frequency "v4l2_frequency")
and call the [VIDIOC\_S\_FREQUENCY](#vidioc-g-frequency) ioctl with a pointer to this
structure. When the requested frequency is not possible the driver
assumes the closest possible value. However [VIDIOC\_S\_FREQUENCY](#vidioc-g-frequency) is a
write-only ioctl, it does not return the actual new frequency.

type v4l2\_frequency

struct v4l2\_frequency

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `tuner` | The tuner or modulator index number. This is the same value as in the struct [`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input") `tuner` field and the struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") `index` field, or the struct [`v4l2_output`](vidioc-enumoutput.html#c.V4L.v4l2_output "v4l2_output") `modulator` field and the struct [`v4l2_modulator`](vidioc-g-modulator.html#c.V4L.v4l2_modulator "v4l2_modulator") `index` field. |
| \_\_u32 | `type` | The tuner type. This is the same value as in the struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") `type` field. The type must be set to `V4L2_TUNER_RADIO` for `/dev/radioX` device nodes, and to `V4L2_TUNER_ANALOG_TV` for all others. Set this field to `V4L2_TUNER_RADIO` for modulators (currently only radio modulators are supported). See [`v4l2_tuner_type`](vidioc-g-tuner.html#c.V4L.v4l2_tuner_type "v4l2_tuner_type") |
| \_\_u32 | `frequency` | Tuning frequency in units of 62.5 kHz, or if the struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") or struct [`v4l2_modulator`](vidioc-g-modulator.html#c.V4L.v4l2_modulator "v4l2_modulator") `capability` flag `V4L2_TUNER_CAP_LOW` is set, in units of 62.5 Hz. A 1 Hz unit is used when the `capability` flag `V4L2_TUNER_CAP_1HZ` is set. |
| \_\_u32 | `reserved`[8] | Reserved for future extensions. Drivers and applications must set the array to zero. |

## 7.32.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The `tuner` index is out of bounds or the value in the `type`
    field is wrong.

EBUSY
:   A hardware seek is in progress.
