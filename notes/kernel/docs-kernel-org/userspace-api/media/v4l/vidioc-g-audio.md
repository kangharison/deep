# 7.22.ioctl VIDIOC_G_AUDIO, VIDIOC_S_AUDIO

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-audio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.22. ioctl VIDIOC\_G\_AUDIO, VIDIOC\_S\_AUDIO

## 7.22.1. Name

VIDIOC\_G\_AUDIO - VIDIOC\_S\_AUDIO - Query or select the current audio input and its attributes

## 7.22.2. Synopsis

VIDIOC\_G\_AUDIO

`int ioctl(int fd, VIDIOC_G_AUDIO, struct v4l2_audio *argp)`

VIDIOC\_S\_AUDIO

`int ioctl(int fd, VIDIOC_S_AUDIO, const struct v4l2_audio *argp)`

## 7.22.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_audio`](#c.V4L.v4l2_audio "v4l2_audio").

## 7.22.4. Description

To query the current audio input applications zero out the `reserved`
array of a struct [`v4l2_audio`](#c.V4L.v4l2_audio "v4l2_audio") and call the
[VIDIOC\_G\_AUDIO](#vidioc-g-audio) ioctl with a pointer to this structure. Drivers fill
the rest of the structure or return an `EINVAL` error code when the device
has no audio inputs, or none which combine with the current video input.

Audio inputs have one writable property, the audio mode. To select the
current audio input *and* change the audio mode, applications initialize
the `index` and `mode` fields, and the `reserved` array of a
struct [`v4l2_audio`](#c.V4L.v4l2_audio "v4l2_audio") structure and call the [VIDIOC\_S\_AUDIO](#vidioc-g-audio)
ioctl. Drivers may switch to a different audio mode if the request
cannot be satisfied. However, this is a write-only ioctl, it does not
return the actual new audio mode.

type v4l2\_audio

struct v4l2\_audio

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | Identifies the audio input, set by the driver or application. |
| \_\_u8 | `name`[32] | Name of the audio input, a NUL-terminated ASCII string, for example: “Line In”. This information is intended for the user, preferably the connector label on the device itself. |
| \_\_u32 | `capability` | Audio capability flags, see [Audio Capability Flags](#audio-capability). |
| \_\_u32 | `mode` | Audio mode flags set by drivers and applications (on [VIDIOC\_S\_AUDIO](#vidioc-g-audio) ioctl), see [Audio Mode Flags](#audio-mode). |
| \_\_u32 | `reserved`[2] | Reserved for future extensions. Drivers and applications must set the array to zero. |

Audio Capability Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_AUDCAP_STEREO` | 0x00001 | This is a stereo input. The flag is intended to automatically disable stereo recording etc. when the signal is always monaural. The API provides no means to detect if stereo is *received*, unless the audio input belongs to a tuner. |
| `V4L2_AUDCAP_AVL` | 0x00002 | Automatic Volume Level mode is supported. |

Audio Mode Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_AUDMODE_AVL` | 0x00001 | AVL mode is on. |

## 7.22.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   No audio inputs combine with the current video input, or the number
    of the selected audio input is out of bounds or it does not combine.
