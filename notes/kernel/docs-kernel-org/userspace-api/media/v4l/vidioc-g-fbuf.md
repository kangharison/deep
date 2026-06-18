# 7.30.ioctl VIDIOC_G_FBUF, VIDIOC_S_FBUF

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-fbuf.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.30. ioctl VIDIOC\_G\_FBUF, VIDIOC\_S\_FBUF

## 7.30.1. Name

VIDIOC\_G\_FBUF - VIDIOC\_S\_FBUF - Get or set frame buffer overlay parameters

## 7.30.2. Synopsis

VIDIOC\_G\_FBUF

`int ioctl(int fd, VIDIOC_G_FBUF, struct v4l2_framebuffer *argp)`

VIDIOC\_S\_FBUF

`int ioctl(int fd, VIDIOC_S_FBUF, const struct v4l2_framebuffer *argp)`

## 7.30.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_framebuffer`](#c.V4L.v4l2_framebuffer "v4l2_framebuffer").

## 7.30.4. Description

Applications can use the [VIDIOC\_G\_FBUF](#vidioc-g-fbuf) and [VIDIOC\_S\_FBUF](#vidioc-g-fbuf) ioctl
to get and set the framebuffer parameters for a
[Video Overlay](dev-overlay.html#overlay) or [Video Output Overlay](dev-osd.html#osd)
(OSD). The type of overlay is implied by the device type (capture or
output device) and can be determined with the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. One `/dev/videoN`
device must not support both kinds of overlay.

The V4L2 API distinguishes destructive and non-destructive overlays. A
destructive overlay copies captured video images into the video memory
of a graphics card. A non-destructive overlay blends video images into a
VGA signal or graphics into a video signal. *Video Output Overlays* are
always non-destructive.

Destructive overlay support has been removed: with modern GPUs and CPUs
this is no longer needed, and it was always a very dangerous feature.

To get the current parameters applications call the [VIDIOC\_G\_FBUF](#vidioc-g-fbuf)
ioctl with a pointer to a struct [`v4l2_framebuffer`](#c.V4L.v4l2_framebuffer "v4l2_framebuffer")
structure. The driver fills all fields of the structure or returns an
EINVAL error code when overlays are not supported.

To set the parameters for a *Video Output Overlay*, applications must
initialize the `flags` field of a struct
[`v4l2_framebuffer`](#c.V4L.v4l2_framebuffer "v4l2_framebuffer"). Since the framebuffer is
implemented on the TV card all other parameters are determined by the
driver. When an application calls [VIDIOC\_S\_FBUF](#vidioc-g-fbuf) with a pointer to
this structure, the driver prepares for the overlay and returns the
framebuffer parameters as [VIDIOC\_G\_FBUF](#vidioc-g-fbuf) does, or it returns an error
code.

To set the parameters for a *Video Capture Overlay*
applications must initialize the `flags` field, the `fmt`
substructure, and call [VIDIOC\_S\_FBUF](#vidioc-g-fbuf). Again the driver prepares for
the overlay and returns the framebuffer parameters as [VIDIOC\_G\_FBUF](#vidioc-g-fbuf)
does, or it returns an error code.

type v4l2\_framebuffer

struct v4l2\_framebuffer

|  |  |  |  |
| --- | --- | --- | --- |
| \_\_u32 | `capability` |  | Overlay capability flags set by the driver, see [Frame Buffer Capability Flags](#framebuffer-cap). |
| \_\_u32 | `flags` |  | Overlay control flags set by application and driver, see [Frame Buffer Flags](#framebuffer-flags) |
| void \* | `base` |  | Physical base address of the framebuffer, that is the address of the pixel in the top left corner of the framebuffer. For [VIDIOC\_S\_FBUF](#vidioc-g-fbuf) this field is no longer supported and the kernel will always set this to NULL. For *Video Output Overlays* the driver will return a valid base address, so applications can find the corresponding Linux framebuffer device (see [Video Output Overlay Interface](dev-osd.html#osd)). For *Video Capture Overlays* this field will always be NULL. |
| struct | `fmt` |  | Layout of the frame buffer. |
|  | \_\_u32 | `width` | Width of the frame buffer in pixels. |
|  | \_\_u32 | `height` | Height of the frame buffer in pixels. |
|  | \_\_u32 | `pixelformat` | The pixel format of the framebuffer. |
|  |  |  | For *non-destructive Video Overlays* this field only defines a format for the struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window") `chromakey` field. |
|  |  |  | For *Video Output Overlays* the driver must return a valid format. |
|  |  |  | Usually this is an RGB format (for example [V4L2\_PIX\_FMT\_RGB565](pixfmt-rgb.html#v4l2-pix-fmt-rgb565)) but YUV formats (only packed YUV formats when chroma keying is used, not including `V4L2_PIX_FMT_YUYV` and `V4L2_PIX_FMT_UYVY`) and the `V4L2_PIX_FMT_PAL8` format are also permitted. The behavior of the driver when an application requests a compressed format is undefined. See [Image Formats](pixfmt.html#pixfmt) for information on pixel formats. |
|  | enum [`v4l2_field`](field-order.html#c.V4L.v4l2_field "v4l2_field") | `field` | Drivers and applications shall ignore this field. If applicable, the field order is selected with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, using the `field` field of struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window"). |
|  | \_\_u32 | `bytesperline` | Distance in bytes between the leftmost pixels in two adjacent lines. |
| This field is irrelevant to *non-destructive Video Overlays*.  For *Video Output Overlays* the driver must return a valid value.  Video hardware may access padding bytes, therefore they must reside in accessible memory. Consider for example the case where padding bytes after the last line of an image cross a system page boundary. Capture devices may write padding bytes, the value is undefined. Output devices ignore the contents of padding bytes.  When the image format is planar the `bytesperline` value applies to the first plane and is divided by the same factor as the `width` field for the other planes. For example the Cb and Cr planes of a YUV 4:2:0 image have half as many padding bytes following each line as the Y plane. To avoid ambiguities drivers must return a `bytesperline` value rounded up to a multiple of the scale factor. | | | |
|  | \_\_u32 | `sizeimage` | This field is irrelevant to *non-destructive Video Overlays*. For *Video Output Overlays* the driver must return a valid format.  Together with `base` it defines the framebuffer memory accessible by the driver. |
|  | enum [`v4l2_colorspace`](colorspaces-defs.html#c.V4L.v4l2_colorspace "v4l2_colorspace") | `colorspace` | This information supplements the `pixelformat` and must be set by the driver, see [Colorspaces](colorspaces.html#colorspaces). |
|  | \_\_u32 | `priv` | Reserved. Drivers and applications must set this field to zero. |

Frame Buffer Capability Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_FBUF_CAP_EXTERNOVERLAY` | 0x0001 | The device is capable of non-destructive overlays. When the driver clears this flag, only destructive overlays are supported. There are no drivers yet which support both destructive and non-destructive overlays. Video Output Overlays are in practice always non-destructive. |
| `V4L2_FBUF_CAP_CHROMAKEY` | 0x0002 | The device supports clipping by chroma-keying the images. That is, image pixels replace pixels in the VGA or video signal only where the latter assume a certain color. Chroma-keying makes no sense for destructive overlays. |
| `V4L2_FBUF_CAP_LIST_CLIPPING` | 0x0004 | The device supports clipping using a list of clip rectangles. Note that this is no longer supported. |
| `V4L2_FBUF_CAP_BITMAP_CLIPPING` | 0x0008 | The device supports clipping using a bit mask. Note that this is no longer supported. |
| `V4L2_FBUF_CAP_LOCAL_ALPHA` | 0x0010 | The device supports clipping/blending using the alpha channel of the framebuffer or VGA signal. Alpha blending makes no sense for destructive overlays. |
| `V4L2_FBUF_CAP_GLOBAL_ALPHA` | 0x0020 | The device supports alpha blending using a global alpha value. Alpha blending makes no sense for destructive overlays. |
| `V4L2_FBUF_CAP_LOCAL_INV_ALPHA` | 0x0040 | The device supports clipping/blending using the inverted alpha channel of the framebuffer or VGA signal. Alpha blending makes no sense for destructive overlays. |
| `V4L2_FBUF_CAP_SRC_CHROMAKEY` | 0x0080 | The device supports Source Chroma-keying. Video pixels with the chroma-key colors are replaced by framebuffer pixels, which is exactly opposite of `V4L2_FBUF_CAP_CHROMAKEY` |

Frame Buffer Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_FBUF_FLAG_PRIMARY` | 0x0001 | The framebuffer is the primary graphics surface. In other words, the overlay is destructive. This flag is typically set by any driver that doesn’t have the `V4L2_FBUF_CAP_EXTERNOVERLAY` capability and it is cleared otherwise. |
| `V4L2_FBUF_FLAG_OVERLAY` | 0x0002 | If this flag is set for a video capture device, then the driver will set the initial overlay size to cover the full framebuffer size, otherwise the existing overlay size (as set by [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)) will be used. Only one video capture driver (bttv) supports this flag. The use of this flag for capture devices is deprecated. There is no way to detect which drivers support this flag, so the only reliable method of setting the overlay size is through [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt). If this flag is set for a video output device, then the video output overlay window is relative to the top-left corner of the framebuffer and restricted to the size of the framebuffer. If it is cleared, then the video output overlay window is relative to the video output display. |
| `V4L2_FBUF_FLAG_CHROMAKEY` | 0x0004 | Use chroma-keying. The chroma-key color is determined by the `chromakey` field of struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window") and negotiated with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, see [Video Overlay Interface](dev-overlay.html#overlay) and [Video Output Overlay Interface](dev-osd.html#osd). |
| There are no flags to enable clipping using a list of clip rectangles or a bitmap. These methods are negotiated with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, see [Video Overlay Interface](dev-overlay.html#overlay) and [Video Output Overlay Interface](dev-osd.html#osd). | | |
| `V4L2_FBUF_FLAG_LOCAL_ALPHA` | 0x0008 | Use the alpha channel of the framebuffer to clip or blend framebuffer pixels with video images. The blend function is: output = framebuffer pixel \* alpha + video pixel \* (1 - alpha). The actual alpha depth depends on the framebuffer pixel format. |
| `V4L2_FBUF_FLAG_GLOBAL_ALPHA` | 0x0010 | Use a global alpha value to blend the framebuffer with video images. The blend function is: output = (framebuffer pixel \* alpha + video pixel \* (255 - alpha)) / 255. The alpha value is determined by the `global_alpha` field of struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window") and negotiated with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, see [Video Overlay Interface](dev-overlay.html#overlay) and [Video Output Overlay Interface](dev-osd.html#osd). |
| `V4L2_FBUF_FLAG_LOCAL_INV_ALPHA` | 0x0020 | Like `V4L2_FBUF_FLAG_LOCAL_ALPHA`, use the alpha channel of the framebuffer to clip or blend framebuffer pixels with video images, but with an inverted alpha value. The blend function is: output = framebuffer pixel \* (1 - alpha) + video pixel \* alpha. The actual alpha depth depends on the framebuffer pixel format. |
| `V4L2_FBUF_FLAG_SRC_CHROMAKEY` | 0x0040 | Use source chroma-keying. The source chroma-key color is determined by the `chromakey` field of struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window") and negotiated with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, see [Video Overlay Interface](dev-overlay.html#overlay) and [Video Output Overlay Interface](dev-osd.html#osd). Both chroma-keying are mutual exclusive to each other, so same `chromakey` field of struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window") is being used. |

## 7.30.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EPERM
:   [VIDIOC\_S\_FBUF](#vidioc-g-fbuf) can only be called by a privileged user to
    negotiate the parameters for a destructive overlay.

EINVAL
:   The [VIDIOC\_S\_FBUF](#vidioc-g-fbuf) parameters are unsuitable.
