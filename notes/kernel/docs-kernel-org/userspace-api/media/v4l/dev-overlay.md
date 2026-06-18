# 4.2.Video Overlay Interface

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-overlay.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2. Video Overlay Interface

**Also known as Framebuffer Overlay or Previewing.**

Video overlay devices have the ability to genlock (TV-)video into the
(VGA-)video signal of a graphics card, or to store captured images
directly in video memory of a graphics card, typically with clipping.
This can be considerable more efficient than capturing images and
displaying them by other means. In the old days when only nuclear power
plants needed cooling towers this used to be the only way to put live
video into a window.

Video overlay devices are accessed through the same character special
files as [video capture](dev-capture.html#capture) devices.

Note

The default function of a `/dev/video` device is video
capturing. The overlay function is only available after calling
the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl.

The driver may support simultaneous overlay and capturing using the
read/write and streaming I/O methods. If so, operation at the nominal
frame rate of the video standard is not guaranteed. Frames may be
directed away from overlay to capture, or one field may be used for
overlay and the other for capture if the capture parameters permit this.

Applications should use different file descriptors for capturing and
overlay. This must be supported by all drivers capable of simultaneous
capturing and overlay. Optionally these drivers may also permit
capturing and overlay with a single file descriptor for compatibility
with V4L and earlier versions of V4L2. [[1]](#f1)

A common application of two file descriptors is the X11
[Xv/V4L](hist-v4l2.html#xvideo) interface driver and a V4L2 application.
While the X server controls video overlay, the application can take
advantage of memory mapping and DMA.

## 4.2.1. Querying Capabilities

Devices supporting the video overlay interface set the
`V4L2_CAP_VIDEO_OVERLAY` flag in the `capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. The overlay I/O
method specified below must be supported. Tuners and audio inputs are
optional.

## 4.2.2. Supplemental Functions

Video overlay devices shall support [audio input](audio.html#audio),
[Tuners and Modulators](tuner.html#tuner), [controls](control.html#control),
[cropping and scaling](crop.html#crop) and
[streaming parameter](streaming-par.html#streaming-par) ioctls as needed. The
[video input](video.html#video) and [video standard](standard.html#standard)
ioctls must be supported by all video overlay devices.

## 4.2.3. Setup

*Note: support for this has been removed.*
Before overlay can commence applications must program the driver with
frame buffer parameters, namely the address and size of the frame buffer
and the image format, for example RGB 5:6:5. The
[VIDIOC\_G\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) and
[VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) ioctls are available to get and
set these parameters, respectively. The [VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) ioctl is
privileged because it allows to set up DMA into physical memory,
bypassing the memory protection mechanisms of the kernel. Only the
superuser can change the frame buffer address and size. Users are not
supposed to run TV applications as root or with SUID bit set. A small
helper application with suitable privileges should query the graphics
system and program the V4L2 driver at the appropriate time.

Some devices add the video overlay to the output signal of the graphics
card. In this case the frame buffer is not modified by the video device,
and the frame buffer address and pixel format are not needed by the
driver. The [VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) ioctl is not privileged. An application
can check for this type of device by calling the [VIDIOC\_G\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf)
ioctl.

A driver may support any (or none) of five clipping/blending methods:

1. Chroma-keying displays the overlaid image only where pixels in the
   primary graphics surface assume a certain color.
2. *Note: support for this has been removed.*
   A bitmap can be specified where each bit corresponds to a pixel in
   the overlaid image. When the bit is set, the corresponding video
   pixel is displayed, otherwise a pixel of the graphics surface.
3. *Note: support for this has been removed.*
   A list of clipping rectangles can be specified. In these regions *no*
   video is displayed, so the graphics surface can be seen here.
4. The framebuffer has an alpha channel that can be used to clip or
   blend the framebuffer with the video.
5. A global alpha value can be specified to blend the framebuffer
   contents with video images.

When simultaneous capturing and overlay is supported and the hardware
prohibits different image and frame buffer formats, the format requested
first takes precedence. The attempt to capture
([VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)) or overlay
([VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf)) may fail with an `EBUSY` error
code or return accordingly modified parameters..

## 4.2.4. Overlay Window

The overlaid image is determined by cropping and overlay window
parameters. The former select an area of the video picture to capture,
the latter how images are overlaid and clipped. Cropping initialization
at minimum requires to reset the parameters to defaults. An example is
given in [Image Cropping, Insertion and Scaling -- the CROP API](crop.html#crop).

The overlay window is described by a struct
[`v4l2_window`](#c.V4L.v4l2_window "v4l2_window"). It defines the size of the image,
its position over the graphics surface and the clipping to be applied.
To get the current parameters applications set the `type` field of a
struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") to
`V4L2_BUF_TYPE_VIDEO_OVERLAY` and call the
[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl. The driver fills the
struct [`v4l2_window`](#c.V4L.v4l2_window "v4l2_window") substructure named `win`. It is not
possible to retrieve a previously programmed clipping list or bitmap.

To program the overlay window applications set the `type` field of a
struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") to
`V4L2_BUF_TYPE_VIDEO_OVERLAY`, initialize the `win` substructure and
call the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl. The driver
adjusts the parameters against hardware limits and returns the actual
parameters as [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) does. Like [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt), the
[VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl can be used to learn
about driver capabilities without actually changing driver state. Unlike
[VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) this also works after the overlay has been enabled.

The scaling factor of the overlaid image is implied by the width and
height given in struct [`v4l2_window`](#c.V4L.v4l2_window "v4l2_window") and the size
of the cropping rectangle. For more information see [Image Cropping, Insertion and Scaling -- the CROP API](crop.html#crop).

When simultaneous capturing and overlay is supported and the hardware
prohibits different image and window sizes, the size requested first
takes precedence. The attempt to capture or overlay as well
([VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)) may fail with an `EBUSY` error
code or return accordingly modified parameters.

type v4l2\_window

### 4.2.4.1. struct v4l2\_window

`struct v4l2_rect w`
:   Size and position of the window relative to the top, left corner of
    the frame buffer defined with
    [VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf). The window can extend the
    frame buffer width and height, the `x` and `y` coordinates can
    be negative, and it can lie completely outside the frame buffer. The
    driver clips the window accordingly, or if that is not possible,
    modifies its size and/or position.

`enum v4l2_field field`
:   Applications set this field to determine which video field shall be
    overlaid, typically one of `V4L2_FIELD_ANY` (0),
    `V4L2_FIELD_TOP`, `V4L2_FIELD_BOTTOM` or
    `V4L2_FIELD_INTERLACED`. Drivers may have to choose a different
    field order and return the actual setting here.

`__u32 chromakey`
:   When chroma-keying has been negotiated with
    [VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) applications set this field
    to the desired pixel value for the chroma key. The format is the
    same as the pixel format of the framebuffer (struct
    [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "v4l2_framebuffer") `fmt.pixelformat`
    field), with bytes in host order. E. g. for
    [V4L2\_PIX\_FMT\_BGR24](pixfmt-rgb.html#v4l2-pix-fmt-bgr32) the value should
    be 0xRRGGBB on a little endian, 0xBBGGRR on a big endian host.

`struct v4l2_clip * clips`
:   *Note: support for this has been removed.*
    When chroma-keying has *not* been negotiated and
    [VIDIOC\_G\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) indicated this capability,
    applications can set this field to point to an array of clipping
    rectangles.

    Like the window coordinates w, clipping rectangles are defined
    relative to the top, left corner of the frame buffer. However
    clipping rectangles must not extend the frame buffer width and
    height, and they must not overlap. If possible applications
    should merge adjacent rectangles. Whether this must create
    x-y or y-x bands, or the order of rectangles, is not defined. When
    clip lists are not supported the driver ignores this field. Its
    contents after calling [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
    are undefined.

`__u32 clipcount`
:   *Note: support for this has been removed.*
    When the application set the `clips` field, this field must
    contain the number of clipping rectangles in the list. When clip
    lists are not supported the driver ignores this field, its contents
    after calling [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) are undefined. When clip lists are
    supported but no clipping is desired this field must be set to zero.

`void * bitmap`
:   *Note: support for this has been removed.*
    When chroma-keying has *not* been negotiated and
    [VIDIOC\_G\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf) indicated this capability,
    applications can set this field to point to a clipping bit mask.

It must be of the same size as the window, `w.width` and `w.height`.
Each bit corresponds to a pixel in the overlaid image, which is
displayed only when the bit is *set*. Pixel coordinates translate to
bits like:

```
((__u8 *) bitmap)[w.width * y + x / 8] & (1 << (x & 7))
```

where `0` ≤ x < `w.width` and `0` ≤ y <`w.height`. [[2]](#f2)

When a clipping bit mask is not supported the driver ignores this field,
its contents after calling [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) are
undefined. When a bit mask is supported but no clipping is desired this
field must be set to `NULL`.

Applications need not create a clip list or bit mask. When they pass
both, or despite negotiating chroma-keying, the results are undefined.
Regardless of the chosen method, the clipping abilities of the hardware
may be limited in quantity or quality. The results when these limits are
exceeded are undefined. [[3]](#f3)

`__u8 global_alpha`
:   The global alpha value used to blend the framebuffer with video
    images, if global alpha blending has been negotiated
    (`V4L2_FBUF_FLAG_GLOBAL_ALPHA`, see
    [VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf),
    [Frame Buffer Flags](vidioc-g-fbuf.html#framebuffer-flags)).

Note

This field was added in Linux 2.6.23, extending the
structure. However the [VIDIOC\_[G|S|TRY]\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
ioctls, which take a pointer to a [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format")
parent structure with padding bytes at the end, are not affected.

type v4l2\_clip

### 4.2.4.2. struct v4l2\_clip [[4]](#f4)

`struct v4l2_rect c`
:   Coordinates of the clipping rectangle, relative to the top, left
    corner of the frame buffer. Only window pixels *outside* all
    clipping rectangles are displayed.

`struct v4l2_clip * next`
:   Pointer to the next clipping rectangle, `NULL` when this is the last
    rectangle. Drivers ignore this field, it cannot be used to pass a
    linked list of clipping rectangles.

type v4l2\_rect

### 4.2.4.3. struct v4l2\_rect

`__s32 left`
:   Horizontal offset of the top, left corner of the rectangle, in
    pixels.

`__s32 top`
:   Vertical offset of the top, left corner of the rectangle, in pixels.
    Offsets increase to the right and down.

`__u32 width`
:   Width of the rectangle, in pixels.

`__u32 height`
:   Height of the rectangle, in pixels.

## 4.2.5. Enabling Overlay

To start or stop the frame buffer overlay applications call the
[ioctl VIDIOC\_OVERLAY](vidioc-overlay.html#vidioc-overlay) ioctl.

[[1](#id1)]

In the opinion of the designers of this API, no driver writer taking
the efforts to support simultaneous capturing and overlay will
restrict this ability by requiring a single file descriptor, as in
V4L and earlier versions of V4L2. Making this optional means
applications depending on two file descriptors need backup routines
to be compatible with all drivers, which is considerable more work
than using two fds in applications which do not. Also two fd’s fit
the general concept of one file descriptor for each logical stream.
Hence as a complexity trade-off drivers *must* support two file
descriptors and *may* support single fd operation.


[[2](#id2)]

Should we require `w.width` to be a multiple of eight?


[[3](#id3)]

When the image is written into frame buffer memory it will be
undesirable if the driver clips out less pixels than expected,
because the application and graphics system are not aware these
regions need to be refreshed. The driver should clip out more pixels
or not write the image at all.


[[4](#id4)]

The X Window system defines “regions” which are vectors of `struct
BoxRec { short x1, y1, x2, y2; }` with `width = x2 - x1` and
`height = y2 - y1`, so one cannot pass X11 clip lists directly.
