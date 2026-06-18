# 1.26.Single- and multi-planar APIs

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/planar-apis.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.26. Single- and multi-planar APIs

Some devices require data for each input or output video frame to be
placed in discontiguous memory buffers. In such cases, one video frame
has to be addressed using more than one memory address, i.e. one pointer
per “plane”. A plane is a sub-buffer of the current frame. For examples
of such formats see [Image Formats](pixfmt.html#pixfmt).

Initially, V4L2 API did not support multi-planar buffers and a set of
extensions has been introduced to handle them. Those extensions
constitute what is being referred to as the “multi-planar API”.

Some of the V4L2 API calls and structures are interpreted differently,
depending on whether single- or multi-planar API is being used. An
application can choose whether to use one or the other by passing a
corresponding buffer type to its ioctl calls. Multi-planar versions of
buffer types are suffixed with an `_MPLANE` string. For a list of
available multi-planar buffer types see enum
[`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type").

## 1.26.1. Multi-planar formats

Multi-planar API introduces new multi-planar formats. Those formats use
a separate set of FourCC codes. It is important to distinguish between
the multi-planar API and a multi-planar format. Multi-planar API calls
can handle all single-planar formats as well (as long as they are passed
in multi-planar API structures), while the single-planar API cannot
handle multi-planar formats.

## 1.26.2. Calls that distinguish between single and multi-planar APIs

[VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap)
:   Two additional multi-planar capabilities are added. They can be set
    together with non-multi-planar ones for devices that handle both
    single- and multi-planar formats.

[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt), [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt), [VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
:   New structures for describing multi-planar formats are added: struct
    [`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane") and
    struct [`v4l2_plane_pix_format`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_plane_pix_format "v4l2_plane_pix_format").
    Drivers may define new multi-planar formats, which have distinct
    FourCC codes from the existing single-planar ones.

[VIDIOC\_QBUF](vidioc-qbuf.html#vidioc-qbuf), [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf), [VIDIOC\_QUERYBUF](vidioc-querybuf.html#vidioc-querybuf)
:   A new struct [`v4l2_plane`](buffer.html#c.V4L.v4l2_plane "v4l2_plane") structure for
    describing planes is added. Arrays of this structure are passed in
    the new `m.planes` field of struct
    [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer").

[VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs)
:   Will allocate multi-planar buffers as requested.
