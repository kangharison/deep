# 7.35.ioctl VIDIOC_G_MODULATOR, VIDIOC_S_MODULATOR

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-modulator.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.35. ioctl VIDIOC\_G\_MODULATOR, VIDIOC\_S\_MODULATOR

## 7.35.1. Name

VIDIOC\_G\_MODULATOR - VIDIOC\_S\_MODULATOR - Get or set modulator attributes

## 7.35.2. Synopsis

VIDIOC\_G\_MODULATOR

`int ioctl(int fd, VIDIOC_G_MODULATOR, struct v4l2_modulator *argp)`

VIDIOC\_S\_MODULATOR

`int ioctl(int fd, VIDIOC_S_MODULATOR, const struct v4l2_modulator *argp)`

## 7.35.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_modulator`](#c.V4L.v4l2_modulator "v4l2_modulator").

## 7.35.4. Description

To query the attributes of a modulator applications initialize the
`index` field and zero out the `reserved` array of a struct
[`v4l2_modulator`](#c.V4L.v4l2_modulator "v4l2_modulator") and call the
[VIDIOC\_G\_MODULATOR](#vidioc-g-modulator) ioctl with a pointer to this structure. Drivers
fill the rest of the structure or return an `EINVAL` error code when the
index is out of bounds. To enumerate all modulators applications shall
begin at index zero, incrementing by one until the driver returns
EINVAL.

Modulators have two writable properties, an audio modulation set and the
radio frequency. To change the modulated audio subprograms, applications
initialize the `index` and `txsubchans` fields and the `reserved`
array and call the [VIDIOC\_S\_MODULATOR](#vidioc-g-modulator) ioctl. Drivers may choose a
different audio modulation if the request cannot be satisfied. However
this is a write-only ioctl, it does not return the actual audio
modulation selected.

[SDR](dev-sdr.html#sdr) specific modulator types are `V4L2_TUNER_SDR` and
`V4L2_TUNER_RF`. For SDR devices `txsubchans` field must be
initialized to zero. The term ‘modulator’ means SDR transmitter in this
context.

To change the radio frequency the
[VIDIOC\_S\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency) ioctl is available.

type v4l2\_modulator

struct v4l2\_modulator

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| \_\_u32 | `index` | Identifies the modulator, set by the application. | | |
| \_\_u8 | `name`[32] | Name of the modulator, a NUL-terminated ASCII string.  This information is intended for the user. | | |
| \_\_u32 | `capability` | Modulator capability flags. No flags are defined for this field, the tuner flags in struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") are used accordingly. The audio flags indicate the ability to encode audio subprograms. They will *not* change for example with the current video standard. | | |
| \_\_u32 | `rangelow` | The lowest tunable frequency in units of 62.5 KHz, or if the `capability` flag `V4L2_TUNER_CAP_LOW` is set, in units of 62.5 Hz, or if the `capability` flag `V4L2_TUNER_CAP_1HZ` is set, in units of 1 Hz. | | |
| \_\_u32 | `rangehigh` | The highest tunable frequency in units of 62.5 KHz, or if the `capability` flag `V4L2_TUNER_CAP_LOW` is set, in units of 62.5 Hz, or if the `capability` flag `V4L2_TUNER_CAP_1HZ` is set, in units of 1 Hz. | | |
| \_\_u32 | `txsubchans` | With this field applications can determine how audio sub-carriers shall be modulated. It contains a set of flags as defined in [Modulator Audio Transmission Flags](#modulator-txsubchans).  Note  The tuner `rxsubchans` flags are reused, but the semantics are different. Video output devices are assumed to have an analog or PCM audio input with 1-3 channels. The `txsubchans` flags select one or more channels for modulation, together with some audio subprogram indicator, for example, a stereo pilot tone. | | |
| \_\_u32 | `type` | Type of the modulator, see [`v4l2_tuner_type`](vidioc-g-tuner.html#c.V4L.v4l2_tuner_type "v4l2_tuner_type"). | | |
| \_\_u32 | `reserved`[3] | Reserved for future extensions.  Drivers and applications must set the array to zero. | | |

Modulator Audio Transmission Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_TUNER_SUB_MONO` | 0x0001 | Modulate channel 1 as mono audio, when the input has more channels, a down-mix of channel 1 and 2. This flag does not combine with `V4L2_TUNER_SUB_STEREO` or `V4L2_TUNER_SUB_LANG1`. |
| `V4L2_TUNER_SUB_STEREO` | 0x0002 | Modulate channel 1 and 2 as left and right channel of a stereo audio signal. When the input has only one channel or two channels and `V4L2_TUNER_SUB_SAP` is also set, channel 1 is encoded as left and right channel. This flag does not combine with `V4L2_TUNER_SUB_MONO` or `V4L2_TUNER_SUB_LANG1`. When the driver does not support stereo audio it shall fall back to mono. |
| `V4L2_TUNER_SUB_LANG1` | 0x0008 | Modulate channel 1 and 2 as primary and secondary language of a bilingual audio signal. When the input has only one channel it is used for both languages. It is not possible to encode the primary or secondary language only. This flag does not combine with `V4L2_TUNER_SUB_MONO`, `V4L2_TUNER_SUB_STEREO` or `V4L2_TUNER_SUB_SAP`. If the hardware does not support the respective audio matrix, or the current video standard does not permit bilingual audio the [VIDIOC\_S\_MODULATOR](#vidioc-g-modulator) ioctl shall return an `EINVAL` error code and the driver shall fall back to mono or stereo mode. |
| `V4L2_TUNER_SUB_LANG2` | 0x0004 | Same effect as `V4L2_TUNER_SUB_SAP`. |
| `V4L2_TUNER_SUB_SAP` | 0x0004 | When combined with `V4L2_TUNER_SUB_MONO` the first channel is encoded as mono audio, the last channel as Second Audio Program. When the input has only one channel it is used for both audio tracks. When the input has three channels the mono track is a down-mix of channel 1 and 2. When combined with `V4L2_TUNER_SUB_STEREO` channel 1 and 2 are encoded as left and right stereo audio, channel 3 as Second Audio Program. When the input has only two channels, the first is encoded as left and right channel and the second as SAP. When the input has only one channel it is used for all audio tracks. It is not possible to encode a Second Audio Program only. This flag must combine with `V4L2_TUNER_SUB_MONO` or `V4L2_TUNER_SUB_STEREO`. If the hardware does not support the respective audio matrix, or the current video standard does not permit SAP the [VIDIOC\_S\_MODULATOR](#vidioc-g-modulator) ioctl shall return an `EINVAL` error code and driver shall fall back to mono or stereo mode. |
| `V4L2_TUNER_SUB_RDS` | 0x0010 | Enable the RDS encoder for a radio FM transmitter. |

## 7.35.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`v4l2_modulator`](#c.V4L.v4l2_modulator "v4l2_modulator") `index` is
    out of bounds.
