# 7.48.ioctl VIDIOC_QUERYCAP

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-querycap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.48. ioctl VIDIOC\_QUERYCAP

## 7.48.1. Name

VIDIOC\_QUERYCAP - Query device capabilities

## 7.48.2. Synopsis

VIDIOC\_QUERYCAP

`int ioctl(int fd, VIDIOC_QUERYCAP, struct v4l2_capability *argp)`

## 7.48.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_capability`](#c.V4L.v4l2_capability "v4l2_capability").

## 7.48.4. Description

All V4L2 devices support the `VIDIOC_QUERYCAP` ioctl. It is used to
identify kernel devices compatible with this specification and to obtain
information about driver and hardware capabilities. The ioctl takes a
pointer to a struct [`v4l2_capability`](#c.V4L.v4l2_capability "v4l2_capability") which is
filled by the driver. When the driver is not compatible with this
specification the ioctl returns an `EINVAL` error code.

type v4l2\_capability

struct v4l2\_capability

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `driver`[16] | Name of the driver, a unique NUL-terminated ASCII string. For example: “bttv”. Driver specific applications can use this information to verify the driver identity. It is also useful to work around known bugs, or to identify drivers in error reports.  Storing strings in fixed sized arrays is bad practice but unavoidable here. Drivers and applications should take precautions to never read or write beyond the end of the array and to make sure the strings are properly NUL-terminated. |
| \_\_u8 | `card`[32] | Name of the device, a NUL-terminated UTF-8 string. For example: “Yoyodyne TV/FM”. One driver may support different brands or models of video hardware. This information is intended for users, for example in a menu of available devices. Since multiple TV cards of the same brand may be installed which are supported by the same driver, this name should be combined with the character device file name (e. g. `/dev/video2`) or the `bus_info` string to avoid ambiguities. |
| \_\_u8 | `bus_info`[32] | Location of the device in the system, a NUL-terminated ASCII string. For example: “PCI:0000:05:06.0”. This information is intended for users, to distinguish multiple identical devices. If no such information is available the field must simply count the devices controlled by the driver (“platform:vivid-000”). The bus\_info must start with “PCI:” for PCI boards, “PCIe:” for PCI Express boards, “usb-” for USB devices, “I2C:” for i2c devices, “ISA:” for ISA devices, “parport” for parallel port devices and “platform:” for platform devices. |
| \_\_u32 | `version` | Version number of the driver.  Starting with kernel 3.1, the version reported is provided by the V4L2 subsystem following the kernel numbering scheme. However, it may not always return the same version as the kernel if, for example, a stable or distribution-modified kernel uses the V4L2 stack from a newer kernel.  The version number is formatted using the `KERNEL_VERSION()` macro. For example if the media stack corresponds to the V4L2 version shipped with Kernel 4.14, it would be equivalent to: |
| `#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))`  `__u32 version = KERNEL_VERSION(4, 14, 0);`  `printf ("Version: %u.%u.%u\\n",`  `(version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);` | | |
| \_\_u32 | `capabilities` | Available capabilities of the physical device as a whole, see [Device Capabilities Flags](#device-capabilities). The same physical device can export multiple devices in /dev (e.g. /dev/videoX, /dev/vbiY and /dev/radioZ). The `capabilities` field should contain a `union of` all capabilities available around the several V4L2 devices exported to userspace. For all those devices the `capabilities` field returns the same set of capabilities. This allows applications to open just one of the devices (typically the video device) and discover whether video, vbi and/or radio are also supported. |
| \_\_u32 | `device_caps` | Device capabilities of the opened device, see [Device Capabilities Flags](#device-capabilities). Should contain the available capabilities of that specific device node. So, for example, `device_caps` of a radio device will only contain radio related capabilities and no video or vbi capabilities. This field is only set if the `capabilities` field contains the `V4L2_CAP_DEVICE_CAPS` capability. Only the `capabilities` field can have the `V4L2_CAP_DEVICE_CAPS` capability, `device_caps` will never set `V4L2_CAP_DEVICE_CAPS`. |
| \_\_u32 | `reserved`[3] | Reserved for future extensions. Drivers must set this array to zero. |

Device Capabilities Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_CAP_VIDEO_CAPTURE` | 0x00000001 | The device supports the single-planar API through the [Video Capture](dev-capture.html#capture) interface. |
| `V4L2_CAP_VIDEO_CAPTURE_MPLANE` | 0x00001000 | The device supports the [multi-planar API](planar-apis.html#planar-apis) through the [Video Capture](dev-capture.html#capture) interface. |
| `V4L2_CAP_VIDEO_OUTPUT` | 0x00000002 | The device supports the single-planar API through the [Video Output](dev-output.html#output) interface. |
| `V4L2_CAP_VIDEO_OUTPUT_MPLANE` | 0x00002000 | The device supports the [multi-planar API](planar-apis.html#planar-apis) through the [Video Output](dev-output.html#output) interface. |
| `V4L2_CAP_VIDEO_M2M` | 0x00008000 | The device supports the single-planar API through the Video Memory-To-Memory interface. |
| `V4L2_CAP_VIDEO_M2M_MPLANE` | 0x00004000 | The device supports the [multi-planar API](planar-apis.html#planar-apis) through the Video Memory-To-Memory interface. |
| `V4L2_CAP_VIDEO_OVERLAY` | 0x00000004 | The device supports the [Video Overlay](dev-overlay.html#overlay) interface. A video overlay device typically stores captured images directly in the video memory of a graphics card, with hardware clipping and scaling. |
| `V4L2_CAP_VBI_CAPTURE` | 0x00000010 | The device supports the [Raw VBI Capture](dev-raw-vbi.html#raw-vbi) interface, providing Teletext and Closed Caption data. |
| `V4L2_CAP_VBI_OUTPUT` | 0x00000020 | The device supports the [Raw VBI Output](dev-raw-vbi.html#raw-vbi) interface. |
| `V4L2_CAP_SLICED_VBI_CAPTURE` | 0x00000040 | The device supports the [Sliced VBI Capture](dev-sliced-vbi.html#sliced) interface. |
| `V4L2_CAP_SLICED_VBI_OUTPUT` | 0x00000080 | The device supports the [Sliced VBI Output](dev-sliced-vbi.html#sliced) interface. |
| `V4L2_CAP_RDS_CAPTURE` | 0x00000100 | The device supports the [RDS](dev-rds.html#rds) capture interface. |
| `V4L2_CAP_VIDEO_OUTPUT_OVERLAY` | 0x00000200 | The device supports the [Video Output Overlay](dev-osd.html#osd) (OSD) interface. Unlike the *Video Overlay* interface, this is a secondary function of video output devices and overlays an image onto an outgoing video signal. When the driver sets this flag, it must clear the `V4L2_CAP_VIDEO_OVERLAY` flag and vice versa. [[1]](#f1) |
| `V4L2_CAP_HW_FREQ_SEEK` | 0x00000400 | The device supports the [ioctl VIDIOC\_S\_HW\_FREQ\_SEEK](vidioc-s-hw-freq-seek.html#vidioc-s-hw-freq-seek) ioctl for hardware frequency seeking. |
| `V4L2_CAP_RDS_OUTPUT` | 0x00000800 | The device supports the [RDS](dev-rds.html#rds) output interface. |
| `V4L2_CAP_TUNER` | 0x00010000 | The device has some sort of tuner to receive RF-modulated video signals. For more information about tuner programming see [Tuners and Modulators](tuner.html#tuner). |
| `V4L2_CAP_AUDIO` | 0x00020000 | The device has audio inputs or outputs. It may or may not support audio recording or playback, in PCM or compressed formats. PCM audio support must be implemented as ALSA or OSS interface. For more information on audio inputs and outputs see [Audio Inputs and Outputs](audio.html#audio). |
| `V4L2_CAP_RADIO` | 0x00040000 | This is a radio receiver. |
| `V4L2_CAP_MODULATOR` | 0x00080000 | The device has some sort of modulator to emit RF-modulated video/audio signals. For more information about modulator programming see [Tuners and Modulators](tuner.html#tuner). |
| `V4L2_CAP_SDR_CAPTURE` | 0x00100000 | The device supports the [SDR Capture](dev-sdr.html#sdr) interface. |
| `V4L2_CAP_EXT_PIX_FORMAT` | 0x00200000 | The device supports the struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") extended fields. |
| `V4L2_CAP_SDR_OUTPUT` | 0x00400000 | The device supports the [SDR Output](dev-sdr.html#sdr) interface. |
| `V4L2_CAP_META_CAPTURE` | 0x00800000 | The device supports the [Metadata Interface](dev-meta.html#metadata) capture interface. |
| `V4L2_CAP_READWRITE` | 0x01000000 | The device supports the [`read()`](func-read.html#c.V4L.read "read") and/or [`write()`](func-write.html#c.V4L.write "write") I/O methods. |
| `V4L2_CAP_EDID` | 0x02000000 | The device stores the EDID for a video input, or retrieves the EDID for a video output. It is a standalone EDID device, so no video streaming etc. will take place.  For a video input this is typically an eeprom that supports the [VESA Enhanced Display Data Channel Standard](biblio.html#vesaeddc). It can be something else as well, for example a micro controller.  For a video output this is typically read from an external device such as an HDMI splitter accessed by a serial port. |
| `V4L2_CAP_STREAMING` | 0x04000000 | The device supports the [streaming](mmap.html#mmap) I/O method. |
| `V4L2_CAP_META_OUTPUT` | 0x08000000 | The device supports the [Metadata Interface](dev-meta.html#metadata) output interface. |
| `V4L2_CAP_TOUCH` | 0x10000000 | This is a touch device. |
| `V4L2_CAP_IO_MC` | 0x20000000 | There is only one input and/or output seen from userspace. The whole video topology configuration, including which I/O entity is routed to the input/output, is configured by userspace via the Media Controller. See [Part IV - Media Controller API](../mediactl/media-controller.html#media-controller). |
| `V4L2_CAP_DEVICE_CAPS` | 0x80000000 | The driver fills the `device_caps` field. This capability can only appear in the `capabilities` field and never in the `device_caps` field. |

## 7.48.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

[[1](#id1)]

The struct [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "v4l2_framebuffer") lacks an
enum [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type") field, therefore the
type of overlay is implied by the driver capabilities.
