# 6.1.Differences between V4L and V4L2

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/diff-v4l.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1. Differences between V4L and V4L2

The Video For Linux API was first introduced in Linux 2.1 to unify and
replace various TV and radio device related interfaces, developed
independently by driver writers in prior years. Starting with Linux 2.5
the much improved V4L2 API replaces the V4L API. The support for the old
V4L calls were removed from Kernel, but the library [Libv4l Userspace Library](libv4l.html#libv4l)
supports the conversion of a V4L API system call into a V4L2 one.

## 6.1.1. Opening and Closing Devices

For compatibility reasons the character device file names recommended
for V4L2 video capture, overlay, radio and raw vbi capture devices did
not change from those used by V4L. They are listed in [Interfaces](devices.html#devices)
and below in [V4L Device Types, Names and Numbers](#v4l-dev).

The teletext devices (minor range 192-223) have been removed in V4L2 and
no longer exist. There is no hardware available anymore for handling
pure teletext. Instead raw or sliced VBI is used.

The V4L `videodev` module automatically assigns minor numbers to
drivers in load order, depending on the registered device type. We
recommend that V4L2 drivers by default register devices with the same
numbers, but the system administrator can assign arbitrary minor numbers
using driver module options. The major device number remains 81.

V4L Device Types, Names and Numbers

| Device Type | File Name | Minor Numbers |
| --- | --- | --- |
| Video capture and overlay | `/dev/video` and `/dev/bttv0` [[1]](#f1), `/dev/video0` to `/dev/video63` | 0-63 |
| Radio receiver | `/dev/radio` [[2]](#f2), `/dev/radio0` to `/dev/radio63` | 64-127 |
| Raw VBI capture | `/dev/vbi`, `/dev/vbi0` to `/dev/vbi31` | 224-255 |

V4L prohibits (or used to prohibit) multiple opens of a device file.
V4L2 drivers *may* support multiple opens, see [Opening and Closing Devices](open.html#open) for details
and consequences.

V4L drivers respond to V4L2 ioctls with an `EINVAL` error code.

## 6.1.2. Querying Capabilities

The V4L `VIDIOCGCAP` ioctl is equivalent to V4L2’s
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap).

The `name` field in struct `video_capability` became
`card` in struct [`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability"), `type`
was replaced by `capabilities`. Note V4L2 does not distinguish between
device types like this, better think of basic video input, video output
and radio devices supporting a set of related functions like video
capturing, video overlay and VBI capturing. See [Opening and Closing Devices](open.html#open) for an
introduction.

| `struct video_capability` `type` | struct [`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") `capabilities` flags | Purpose |
| --- | --- | --- |
| `VID_TYPE_CAPTURE` | `V4L2_CAP_VIDEO_CAPTURE` | The [video capture](dev-capture.html#capture) interface is supported. |
| `VID_TYPE_TUNER` | `V4L2_CAP_TUNER` | The device has a [tuner or modulator](tuner.html#tuner). |
| `VID_TYPE_TELETEXT` | `V4L2_CAP_VBI_CAPTURE` | The [raw VBI capture](dev-raw-vbi.html#raw-vbi) interface is supported. |
| `VID_TYPE_OVERLAY` | `V4L2_CAP_VIDEO_OVERLAY` | The [video overlay](dev-overlay.html#overlay) interface is supported. |
| `VID_TYPE_CHROMAKEY` | `V4L2_FBUF_CAP_CHROMAKEY` in field `capability` of struct [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "v4l2_framebuffer") | Whether chromakey overlay is supported. For more information on overlay see [Video Overlay Interface](dev-overlay.html#overlay). |
| `VID_TYPE_CLIPPING` | `V4L2_FBUF_CAP_LIST_CLIPPING` and `V4L2_FBUF_CAP_BITMAP_CLIPPING` in field `capability` of struct [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "v4l2_framebuffer") | Whether clipping the overlaid image is supported, see [Video Overlay Interface](dev-overlay.html#overlay). |
| `VID_TYPE_FRAMERAM` | `V4L2_FBUF_CAP_EXTERNOVERLAY` *not set* in field `capability` of struct [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "v4l2_framebuffer") | Whether overlay overwrites frame buffer memory, see [Video Overlay Interface](dev-overlay.html#overlay). |
| `VID_TYPE_SCALES` | `-` | This flag indicates if the hardware can scale images. The V4L2 API implies the scale factor by setting the cropping dimensions and image size with the [VIDIOC\_S\_CROP](vidioc-g-crop.html#vidioc-g-crop) and [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, respectively. The driver returns the closest sizes possible. For more information on cropping and scaling see [Image Cropping, Insertion and Scaling -- the CROP API](crop.html#crop). |
| `VID_TYPE_MONOCHROME` | `-` | Applications can enumerate the supported image formats with the [ioctl VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl to determine if the device supports grey scale capturing only. For more information on image formats see [Image Formats](pixfmt.html#pixfmt). |
| `VID_TYPE_SUBCAPTURE` | `-` | Applications can call the [VIDIOC\_G\_CROP](vidioc-g-crop.html#vidioc-g-crop) ioctl to determine if the device supports capturing a subsection of the full picture (“cropping” in V4L2). If not, the ioctl returns the `EINVAL` error code. For more information on cropping and scaling see [Image Cropping, Insertion and Scaling -- the CROP API](crop.html#crop). |
| `VID_TYPE_MPEG_DECODER` | `-` | Applications can enumerate the supported image formats with the [ioctl VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl to determine if the device supports MPEG streams. |
| `VID_TYPE_MPEG_ENCODER` | `-` | See above. |
| `VID_TYPE_MJPEG_DECODER` | `-` | See above. |
| `VID_TYPE_MJPEG_ENCODER` | `-` | See above. |

The `audios` field was replaced by `capabilities` flag
`V4L2_CAP_AUDIO`, indicating *if* the device has any audio inputs or
outputs. To determine their number applications can enumerate audio
inputs with the [VIDIOC\_G\_AUDIO](vidioc-g-audio.html#vidioc-g-audio) ioctl. The
audio ioctls are described in [Audio Inputs and Outputs](audio.html#audio).

The `maxwidth`, `maxheight`, `minwidth` and `minheight` fields
were removed. Calling the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) or
[VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl with the desired
dimensions returns the closest size possible, taking into account the
current video standard, cropping and scaling limitations.

## 6.1.3. Video Sources

V4L provides the `VIDIOCGCHAN` and `VIDIOCSCHAN` ioctl using struct
`video_channel` to enumerate the video inputs of a V4L
device. The equivalent V4L2 ioctls are
[ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput),
[VIDIOC\_G\_INPUT](vidioc-g-input.html#vidioc-g-input) and
[VIDIOC\_S\_INPUT](vidioc-g-input.html#vidioc-g-input) using struct
[`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input") as discussed in [Video Inputs and Outputs](video.html#video).

The `channel` field counting inputs was renamed to `index`, the
video input types were renamed as follows:

| struct `video_channel` `type` | struct [`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input") `type` |
| --- | --- |
| `VIDEO_TYPE_TV` | `V4L2_INPUT_TYPE_TUNER` |
| `VIDEO_TYPE_CAMERA` | `V4L2_INPUT_TYPE_CAMERA` |

Unlike the `tuners` field expressing the number of tuners of this
input, V4L2 assumes each video input is connected to at most one tuner.
However a tuner can have more than one input, i. e. RF connectors, and a
device can have multiple tuners. The index number of the tuner
associated with the input, if any, is stored in field `tuner` of
struct [`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input"). Enumeration of tuners is
discussed in [Tuners and Modulators](tuner.html#tuner).

The redundant `VIDEO_VC_TUNER` flag was dropped. Video inputs
associated with a tuner are of type `V4L2_INPUT_TYPE_TUNER`. The
`VIDEO_VC_AUDIO` flag was replaced by the `audioset` field. V4L2
considers devices with up to 32 audio inputs. Each set bit in the
`audioset` field represents one audio input this video input combines
with. For information about audio inputs and how to switch between them
see [Audio Inputs and Outputs](audio.html#audio).

The `norm` field describing the supported video standards was replaced
by `std`. The V4L specification mentions a flag `VIDEO_VC_NORM`
indicating whether the standard can be changed. This flag was a later
addition together with the `norm` field and has been removed in the
meantime. V4L2 has a similar, albeit more comprehensive approach to
video standards, see [Video Standards](standard.html#standard) for more information.

## 6.1.4. Tuning

The V4L `VIDIOCGTUNER` and `VIDIOCSTUNER` ioctl and struct
`video_tuner` can be used to enumerate the tuners of a
V4L TV or radio device. The equivalent V4L2 ioctls are
[VIDIOC\_G\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner) and
[VIDIOC\_S\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner) using struct
[`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner"). Tuners are covered in [Tuners and Modulators](tuner.html#tuner).

The `tuner` field counting tuners was renamed to `index`. The fields
`name`, `rangelow` and `rangehigh` remained unchanged.

The `VIDEO_TUNER_PAL`, `VIDEO_TUNER_NTSC` and `VIDEO_TUNER_SECAM`
flags indicating the supported video standards were dropped. This
information is now contained in the associated struct
[`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input"). No replacement exists for the
`VIDEO_TUNER_NORM` flag indicating whether the video standard can be
switched. The `mode` field to select a different video standard was
replaced by a whole new set of ioctls and structures described in
[Video Standards](standard.html#standard). Due to its ubiquity it should be mentioned the BTTV
driver supports several standards in addition to the regular
`VIDEO_MODE_PAL` (0), `VIDEO_MODE_NTSC`, `VIDEO_MODE_SECAM` and
`VIDEO_MODE_AUTO` (3). Namely N/PAL Argentina, M/PAL, N/PAL, and NTSC
Japan with numbers 3-6 (sic).

The `VIDEO_TUNER_STEREO_ON` flag indicating stereo reception became
`V4L2_TUNER_SUB_STEREO` in field `rxsubchans`. This field also
permits the detection of monaural and bilingual audio, see the
definition of struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") for details.
Presently no replacement exists for the `VIDEO_TUNER_RDS_ON` and
`VIDEO_TUNER_MBS_ON` flags.

The `VIDEO_TUNER_LOW` flag was renamed to `V4L2_TUNER_CAP_LOW` in
the struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") `capability` field.

The `VIDIOCGFREQ` and `VIDIOCSFREQ` ioctl to change the tuner
frequency where renamed to
[VIDIOC\_G\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency) and
[VIDIOC\_S\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency). They take a pointer
to a struct [`v4l2_frequency`](vidioc-g-frequency.html#c.V4L.v4l2_frequency "v4l2_frequency") instead of an
unsigned long integer.

## 6.1.5. Image Properties

V4L2 has no equivalent of the `VIDIOCGPICT` and `VIDIOCSPICT` ioctl
and struct `video_picture`. The following fields where
replaced by V4L2 controls accessible with the
[ioctls VIDIOC\_QUERYCTRL, VIDIOC\_QUERY\_EXT\_CTRL and VIDIOC\_QUERYMENU](vidioc-queryctrl.html#vidioc-queryctrl),
[VIDIOC\_G\_CTRL](vidioc-g-ctrl.html#vidioc-g-ctrl) and
[VIDIOC\_S\_CTRL](vidioc-g-ctrl.html#vidioc-g-ctrl) ioctls:

| struct `video_picture` | V4L2 Control ID |
| --- | --- |
| `brightness` | `V4L2_CID_BRIGHTNESS` |
| `hue` | `V4L2_CID_HUE` |
| `colour` | `V4L2_CID_SATURATION` |
| `contrast` | `V4L2_CID_CONTRAST` |
| `whiteness` | `V4L2_CID_WHITENESS` |

The V4L picture controls are assumed to range from 0 to 65535 with no
particular reset value. The V4L2 API permits arbitrary limits and
defaults which can be queried with the
[ioctls VIDIOC\_QUERYCTRL, VIDIOC\_QUERY\_EXT\_CTRL and VIDIOC\_QUERYMENU](vidioc-queryctrl.html#vidioc-queryctrl) ioctl. For general
information about controls see [User Controls](control.html#control).

The `depth` (average number of bits per pixel) of a video image is
implied by the selected image format. V4L2 does not explicitly provide
such information assuming applications recognizing the format are aware
of the image depth and others need not know. The `palette` field moved
into the struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"):

| struct `video_picture` `palette` | struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") `pixfmt` |
| --- | --- |
| `VIDEO_PALETTE_GREY` | [V4L2\_PIX\_FMT\_GREY](pixfmt-yuv-luma.html#v4l2-pix-fmt-grey) |
| `VIDEO_PALETTE_HI240` | [V4L2\_PIX\_FMT\_HI240](pixfmt-reserved.html#pixfmt-reserved) [[3]](#f3) |
| `VIDEO_PALETTE_RGB565` | [V4L2\_PIX\_FMT\_RGB565](pixfmt-rgb.html#pixfmt-rgb) |
| `VIDEO_PALETTE_RGB555` | [V4L2\_PIX\_FMT\_RGB555](pixfmt-rgb.html#pixfmt-rgb) |
| `VIDEO_PALETTE_RGB24` | [V4L2\_PIX\_FMT\_BGR24](pixfmt-rgb.html#pixfmt-rgb) |
| `VIDEO_PALETTE_RGB32` | [V4L2\_PIX\_FMT\_BGR32](pixfmt-rgb.html#pixfmt-rgb) [[4]](#f4) |
| `VIDEO_PALETTE_YUV422` | [V4L2\_PIX\_FMT\_YUYV](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuyv) |
| `VIDEO_PALETTE_YUYV` [[5]](#f5) | [V4L2\_PIX\_FMT\_YUYV](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuyv) |
| `VIDEO_PALETTE_UYVY` | [V4L2\_PIX\_FMT\_UYVY](pixfmt-packed-yuv.html#v4l2-pix-fmt-uyvy) |
| `VIDEO_PALETTE_YUV420` | None |
| `VIDEO_PALETTE_YUV411` | [V4L2\_PIX\_FMT\_Y41P](pixfmt-packed-yuv.html#v4l2-pix-fmt-y41p) [[6]](#f6) |
| `VIDEO_PALETTE_RAW` | None [[7]](#f7) |
| `VIDEO_PALETTE_YUV422P` | [V4L2\_PIX\_FMT\_YUV422P](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv422p) |
| `VIDEO_PALETTE_YUV411P` | [V4L2\_PIX\_FMT\_YUV411P](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv411p) [[8]](#f8) |
| `VIDEO_PALETTE_YUV420P` | [V4L2\_PIX\_FMT\_YVU420](pixfmt-yuv-planar.html#v4l2-pix-fmt-yvu420) |
| `VIDEO_PALETTE_YUV410P` | [V4L2\_PIX\_FMT\_YVU410](pixfmt-yuv-planar.html#v4l2-pix-fmt-yvu410) |

V4L2 image formats are defined in [Image Formats](pixfmt.html#pixfmt). The image format can
be selected with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl.

## 6.1.6. Audio

The `VIDIOCGAUDIO` and `VIDIOCSAUDIO` ioctl and struct
`video_audio` are used to enumerate the audio inputs
of a V4L device. The equivalent V4L2 ioctls are
[VIDIOC\_G\_AUDIO](vidioc-g-audio.html#vidioc-g-audio) and
[VIDIOC\_S\_AUDIO](vidioc-g-audio.html#vidioc-g-audio) using struct
[`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "v4l2_audio") as discussed in [Audio Inputs and Outputs](audio.html#audio).

The `audio` “channel number” field counting audio inputs was renamed
to `index`.

On `VIDIOCSAUDIO` the `mode` field selects *one* of the
`VIDEO_SOUND_MONO`, `VIDEO_SOUND_STEREO`, `VIDEO_SOUND_LANG1` or
`VIDEO_SOUND_LANG2` audio demodulation modes. When the current audio
standard is BTSC `VIDEO_SOUND_LANG2` refers to SAP and
`VIDEO_SOUND_LANG1` is meaningless. Also undocumented in the V4L
specification, there is no way to query the selected mode. On
`VIDIOCGAUDIO` the driver returns the *actually received* audio
programmes in this field. In the V4L2 API this information is stored in
the struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner") `rxsubchans` and
`audmode` fields, respectively. See [Tuners and Modulators](tuner.html#tuner) for more
information on tuners. Related to audio modes struct
[`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "v4l2_audio") also reports if this is a mono or
stereo input, regardless if the source is a tuner.

The following fields where replaced by V4L2 controls accessible with the
[ioctls VIDIOC\_QUERYCTRL, VIDIOC\_QUERY\_EXT\_CTRL and VIDIOC\_QUERYMENU](vidioc-queryctrl.html#vidioc-queryctrl),
[VIDIOC\_G\_CTRL](vidioc-g-ctrl.html#vidioc-g-ctrl) and
[VIDIOC\_S\_CTRL](vidioc-g-ctrl.html#vidioc-g-ctrl) ioctls:

| struct `video_audio` | V4L2 Control ID |
| --- | --- |
| `volume` | `V4L2_CID_AUDIO_VOLUME` |
| `bass` | `V4L2_CID_AUDIO_BASS` |
| `treble` | `V4L2_CID_AUDIO_TREBLE` |
| `balance` | `V4L2_CID_AUDIO_BALANCE` |

To determine which of these controls are supported by a driver V4L
provides the `flags` `VIDEO_AUDIO_VOLUME`, `VIDEO_AUDIO_BASS`,
`VIDEO_AUDIO_TREBLE` and `VIDEO_AUDIO_BALANCE`. In the V4L2 API the
[ioctls VIDIOC\_QUERYCTRL, VIDIOC\_QUERY\_EXT\_CTRL and VIDIOC\_QUERYMENU](vidioc-queryctrl.html#vidioc-queryctrl) ioctl reports if the
respective control is supported. Accordingly the `VIDEO_AUDIO_MUTABLE`
and `VIDEO_AUDIO_MUTE` flags where replaced by the boolean
`V4L2_CID_AUDIO_MUTE` control.

All V4L2 controls have a `step` attribute replacing the struct
`video_audio` `step` field. The V4L audio controls
are assumed to range from 0 to 65535 with no particular reset value. The
V4L2 API permits arbitrary limits and defaults which can be queried with
the [ioctls VIDIOC\_QUERYCTRL, VIDIOC\_QUERY\_EXT\_CTRL and VIDIOC\_QUERYMENU](vidioc-queryctrl.html#vidioc-queryctrl) ioctl. For general
information about controls see [User Controls](control.html#control).

## 6.1.7. Frame Buffer Overlay

The V4L2 ioctls equivalent to `VIDIOCGFBUF` and `VIDIOCSFBUF` are
[VIDIOC\_G\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) and
[VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf). The `base` field of struct
`video_buffer` remained unchanged, except V4L2 defines
a flag to indicate non-destructive overlays instead of a `NULL`
pointer. All other fields moved into the struct
[`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") `fmt` substructure of
struct [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "v4l2_framebuffer"). The `depth`
field was replaced by `pixelformat`. See [RGB Formats](pixfmt-rgb.html#pixfmt-rgb) for a
list of RGB formats and their respective color depths.

Instead of the special ioctls `VIDIOCGWIN` and `VIDIOCSWIN` V4L2
uses the general-purpose data format negotiation ioctls
[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) and
[VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt). They take a pointer to a struct
[`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") as argument. Here the `win` member
of the `fmt` `union is` used, a struct
[`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window").

The `x`, `y`, `width` and `height` fields of struct
`video_window` moved into struct
[`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") substructure `w` of struct
[`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window"). The `chromakey`, `clips`, and
`clipcount` fields remained unchanged. Struct
`video_clip` was renamed to struct
[`v4l2_clip`](dev-overlay.html#c.V4L.v4l2_clip "v4l2_clip"), also containing a struct
[`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect"), but the semantics are still the same.

The `VIDEO_WINDOW_INTERLACE` flag was dropped. Instead applications
must set the `field` field to `V4L2_FIELD_ANY` or
`V4L2_FIELD_INTERLACED`. The `VIDEO_WINDOW_CHROMAKEY` flag moved
into struct [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "v4l2_framebuffer"), under the new
name `V4L2_FBUF_FLAG_CHROMAKEY`.

In V4L, storing a bitmap pointer in `clips` and setting `clipcount`
to `VIDEO_CLIP_BITMAP` (-1) requests bitmap clipping, using a fixed
size bitmap of 1024 × 625 bits. Struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window")
has a separate `bitmap` pointer field for this purpose and the bitmap
size is determined by `w.width` and `w.height`.

The `VIDIOCCAPTURE` ioctl to enable or disable overlay was renamed to
[ioctl VIDIOC\_OVERLAY](vidioc-overlay.html#vidioc-overlay).

## 6.1.8. Cropping

To capture only a subsection of the full picture V4L defines the
`VIDIOCGCAPTURE` and `VIDIOCSCAPTURE` ioctls using struct
`video_capture`. The equivalent V4L2 ioctls are
[VIDIOC\_G\_CROP](vidioc-g-crop.html#vidioc-g-crop) and
[VIDIOC\_S\_CROP](vidioc-g-crop.html#vidioc-g-crop) using struct
[`v4l2_crop`](vidioc-g-crop.html#c.V4L.v4l2_crop "v4l2_crop"), and the related
[ioctl VIDIOC\_CROPCAP](vidioc-cropcap.html#vidioc-cropcap) ioctl. This is a rather
complex matter, see [Image Cropping, Insertion and Scaling -- the CROP API](crop.html#crop) for details.

The `x`, `y`, `width` and `height` fields moved into struct
[`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") substructure `c` of struct
[`v4l2_crop`](vidioc-g-crop.html#c.V4L.v4l2_crop "v4l2_crop"). The `decimation` field was dropped. In
the V4L2 API the scaling factor is implied by the size of the cropping
rectangle and the size of the captured or overlaid image.

The `VIDEO_CAPTURE_ODD` and `VIDEO_CAPTURE_EVEN` flags to capture
only the odd or even field, respectively, were replaced by
`V4L2_FIELD_TOP` and `V4L2_FIELD_BOTTOM` in the field named
`field` of struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") and
struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window"). These structures are used to
select a capture or overlay format with the
[VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl.

## 6.1.9. Reading Images, Memory Mapping

### 6.1.9.1. Capturing using the read method

There is no essential difference between reading images from a V4L or
V4L2 device using the [`read()`](func-read.html#c.V4L.read "read") function, however V4L2
drivers are not required to support this I/O method. Applications can
determine if the function is available with the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. All V4L2 devices
exchanging data with applications must support the
[`select()`](func-select.html#c.V4L.select "select") and [`poll()`](func-poll.html#c.V4L.poll "poll")
functions.

To select an image format and size, V4L provides the `VIDIOCSPICT` and
`VIDIOCSWIN` ioctls. V4L2 uses the general-purpose data format
negotiation ioctls [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) and
[VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt). They take a pointer to a struct
[`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") as argument, here the struct
[`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") named `pix` of its
`fmt` `union is` used.

For more information about the V4L2 read interface see [Read/Write](rw.html#rw).

### 6.1.9.2. Capturing using memory mapping

Applications can read from V4L devices by mapping buffers in device
memory, or more often just buffers allocated in DMA-able system memory,
into their address space. This avoids the data copying overhead of the
read method. V4L2 supports memory mapping as well, with a few
differences.

| V4L | V4L2 |
| --- | --- |
|  | The image format must be selected before buffers are allocated, with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl. When no format is selected the driver may use the last, possibly by another application requested format. |
| Applications cannot change the number of buffers. The it is built into the driver, unless it has a module option to change the number when the driver module is loaded. | The [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) ioctl allocates the desired number of buffers, this is a required step in the initialization sequence. |
| Drivers map all buffers as one contiguous range of memory. The `VIDIOCGMBUF` ioctl is available to query the number of buffers, the offset of each buffer from the start of the virtual file, and the overall amount of memory used, which can be used as arguments for the [`mmap()`](func-mmap.html#c.V4L.mmap "mmap") function. | Buffers are individually mapped. The offset and size of each buffer can be determined with the [ioctl VIDIOC\_QUERYBUF](vidioc-querybuf.html#vidioc-querybuf) ioctl. |
| The `VIDIOCMCAPTURE` ioctl prepares a buffer for capturing. It also determines the image format for this buffer. The ioctl returns immediately, eventually with an `EAGAIN` error code if no video signal had been detected. When the driver supports more than one buffer applications can call the ioctl multiple times and thus have multiple outstanding capture requests.  The `VIDIOCSYNC` ioctl suspends execution until a particular buffer has been filled. | Drivers maintain an incoming and outgoing queue. [ioctl VIDIOC\_QBUF, VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) enqueues any empty buffer into the incoming queue. Filled buffers are dequeued from the outgoing queue with the [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl. To wait until filled buffers become available this function, [`select()`](func-select.html#c.V4L.select "select") or [`poll()`](func-poll.html#c.V4L.poll "poll") can be used. The [ioctl VIDIOC\_STREAMON, VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon) ioctl must be called once after enqueuing one or more buffers to start capturing. Its counterpart [VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon) stops capturing and dequeues all buffers from both queues. Applications can query the signal status, if known, with the [ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) ioctl. |

For a more in-depth discussion of memory mapping and examples, see
[Streaming I/O (Memory Mapping)](mmap.html#mmap).

## 6.1.10. Reading Raw VBI Data

Originally the V4L API did not specify a raw VBI capture interface, only
the device file `/dev/vbi` was reserved for this purpose. The only
driver supporting this interface was the BTTV driver, de-facto defining
the V4L VBI interface. Reading from the device yields a raw VBI image
with the following parameters:

| struct [`v4l2_vbi_format`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "v4l2_vbi_format") | V4L, BTTV driver |
| --- | --- |
| sampling\_rate | 28636363 Hz NTSC (or any other 525-line standard); 35468950 Hz PAL and SECAM (625-line standards) |
| offset | ? |
| samples\_per\_line | 2048 |
| sample\_format | V4L2\_PIX\_FMT\_GREY. The last four bytes (a machine endianness integer) contain a frame counter. |
| start[] | 10, 273 NTSC; 22, 335 PAL and SECAM |
| count[] | 16, 16 [[9]](#f9) |
| flags | 0 |

Undocumented in the V4L specification, in Linux 2.3 the
`VIDIOCGVBIFMT` and `VIDIOCSVBIFMT` ioctls using struct
`vbi_format` were added to determine the VBI image
parameters. These ioctls are only partially compatible with the V4L2 VBI
interface specified in [Raw VBI Data Interface](dev-raw-vbi.html#raw-vbi).

An `offset` field does not exist, `sample_format` is supposed to be
`VIDEO_PALETTE_RAW`, equivalent to `V4L2_PIX_FMT_GREY`. The
remaining fields are probably equivalent to struct
[`v4l2_vbi_format`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "v4l2_vbi_format").

Apparently only the Zoran (ZR 36120) driver implements these ioctls. The
semantics differ from those specified for V4L2 in two ways. The
parameters are reset on [`open()`](func-open.html#c.V4L.open "open") and
`VIDIOCSVBIFMT` always returns an `EINVAL` error code if the parameters
are invalid.

## 6.1.11. Miscellaneous

V4L2 has no equivalent of the `VIDIOCGUNIT` ioctl. Applications can
find the VBI device associated with a video capture device (or vice
versa) by reopening the device and requesting VBI data. For details see
[Opening and Closing Devices](open.html#open).

No replacement exists for `VIDIOCKEY`, and the V4L functions for
microcode programming. A new interface for MPEG compression and playback
devices is documented in [Extended Controls API](extended-controls.html#extended-controls).

[[1](#id1)]

According to [Linux allocated devices (4.x+ version)](../../../admin-guide/devices.html) these should be symbolic links
to `/dev/video0`. Note the original bttv interface is not
compatible with V4L or V4L2.


[[2](#id2)]

According to `Documentation/admin-guide/devices.rst` a symbolic link to
`/dev/radio0`.


[[3](#id3)]

This is a custom format used by the BTTV driver, not one of the V4L2
standard formats.


[[4](#id4)]

Presumably all V4L RGB formats are little-endian, although some
drivers might interpret them according to machine endianness. V4L2
defines little-endian, big-endian and red/blue swapped variants. For
details see [RGB Formats](pixfmt-rgb.html#pixfmt-rgb).


[[5](#id5)]

`VIDEO_PALETTE_YUV422` and `VIDEO_PALETTE_YUYV` are the same
formats. Some V4L drivers respond to one, some to the other.


[[6](#id6)]

Not to be confused with `V4L2_PIX_FMT_YUV411P`, which is a planar
format.


[[7](#id7)]

V4L explains this as: “RAW capture (BT848)”


[[8](#id8)]

Not to be confused with `V4L2_PIX_FMT_Y41P`, which is a packed
format.


[[9](#id9)]

Old driver versions used different values, eventually the custom
`BTTV_VBISIZE` ioctl was added to query the correct values.
