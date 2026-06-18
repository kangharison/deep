# 3.5.Buffers

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/buffer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.5. Buffers

A buffer contains data exchanged by application and driver using one of
the Streaming I/O methods. In the multi-planar API, the data is held in
planes, while the buffer structure acts as a container for the planes.
Only pointers to buffers (planes) are exchanged, the data itself is not
copied. These pointers, together with meta-information like timestamps
or field parity, are stored in a struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer"),
argument to the [ioctl VIDIOC\_QUERYBUF](vidioc-querybuf.html#vidioc-querybuf),
[VIDIOC\_QBUF](vidioc-qbuf.html#vidioc-qbuf) and
[VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl. In the multi-planar API,
some plane-specific members of struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer"),
such as pointers and sizes for each plane, are stored in
struct [`v4l2_plane`](#c.V4L.v4l2_plane "v4l2_plane") instead. In that case,
struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer") contains an array of plane structures.

Dequeued video buffers come with timestamps. The driver decides at which
part of the frame and with which clock the timestamp is taken. Please
see flags in the masks `V4L2_BUF_FLAG_TIMESTAMP_MASK` and
`V4L2_BUF_FLAG_TSTAMP_SRC_MASK` in [Buffer Flags](#buffer-flags). These flags
are always valid and constant across all buffers during the whole video
stream. Changes in these flags may take place as a side effect of
[VIDIOC\_S\_INPUT](vidioc-g-input.html#vidioc-g-input) or
[VIDIOC\_S\_OUTPUT](vidioc-g-output.html#vidioc-g-output) however. The
`V4L2_BUF_FLAG_TIMESTAMP_COPY` timestamp type which is used by e.g. on
mem-to-mem devices is an exception to the rule: the timestamp source
flags are copied from the OUTPUT video buffer to the CAPTURE video
buffer.

## 3.5.1. Interactions between formats, controls and buffers

V4L2 exposes parameters that influence the buffer size, or the way data is
laid out in the buffer. Those parameters are exposed through both formats and
controls. One example of such a control is the `V4L2_CID_ROTATE` control
that modifies the direction in which pixels are stored in the buffer, as well
as the buffer size when the selected format includes padding at the end of
lines.

The set of information needed to interpret the content of a buffer (e.g. the
pixel format, the line stride, the tiling orientation or the rotation) is
collectively referred to in the rest of this section as the buffer layout.

Controls that can modify the buffer layout shall set the
`V4L2_CTRL_FLAG_MODIFY_LAYOUT` flag.

Modifying formats or controls that influence the buffer size or layout require
the stream to be stopped. Any attempt at such a modification while the stream
is active shall cause the ioctl setting the format or the control to return
the `EBUSY` error code. In that case drivers shall also set the
`V4L2_CTRL_FLAG_GRABBED` flag when calling
`VIDIOC_QUERYCTRL()` or [`VIDIOC_QUERY_EXT_CTRL()`](vidioc-queryctrl.html#c.V4L.VIDIOC_QUERY_EXT_CTRL "VIDIOC_QUERY_EXT_CTRL") for such a
control while the stream is active.

Note

The [`VIDIOC_S_SELECTION()`](vidioc-g-selection.html#c.V4L.VIDIOC_S_SELECTION "VIDIOC_S_SELECTION") ioctl can, depending on the hardware (for
instance if the device doesn’t include a scaler), modify the format in
addition to the selection rectangle. Similarly, the
[`VIDIOC_S_INPUT()`](vidioc-g-input.html#c.V4L.VIDIOC_S_INPUT "VIDIOC_S_INPUT"), [`VIDIOC_S_OUTPUT()`](vidioc-g-output.html#c.V4L.VIDIOC_S_OUTPUT "VIDIOC_S_OUTPUT"), [`VIDIOC_S_STD()`](vidioc-g-std.html#c.V4L.VIDIOC_S_STD "VIDIOC_S_STD")
and [`VIDIOC_S_DV_TIMINGS()`](vidioc-g-dv-timings.html#c.V4L.VIDIOC_S_DV_TIMINGS "VIDIOC_S_DV_TIMINGS") ioctls can also modify the format and
selection rectangles. When those ioctls result in a buffer size or layout
change, drivers shall handle that condition as they would handle it in the
[`VIDIOC_S_FMT()`](vidioc-g-fmt.html#c.V4L.VIDIOC_S_FMT "VIDIOC_S_FMT") ioctl in all cases described in this section.

Controls that only influence the buffer layout can be modified at any time
when the stream is stopped. As they don’t influence the buffer size, no
special handling is needed to synchronize those controls with buffer
allocation and the `V4L2_CTRL_FLAG_GRABBED` flag is cleared once the
stream is stopped.

Formats and controls that influence the buffer size interact with buffer
allocation. The simplest way to handle this is for drivers to always require
buffers to be reallocated in order to change those formats or controls. In
that case, to perform such changes, userspace applications shall first stop
the video stream with the [`VIDIOC_STREAMOFF()`](vidioc-streamon.html#c.V4L.VIDIOC_STREAMOFF "VIDIOC_STREAMOFF") ioctl if it is running
and free all buffers with the [`VIDIOC_REQBUFS()`](vidioc-reqbufs.html#c.V4L.VIDIOC_REQBUFS "VIDIOC_REQBUFS") ioctl if they are
allocated. After freeing all buffers the `V4L2_CTRL_FLAG_GRABBED` flag
for controls is cleared. The format or controls can then be modified, and
buffers shall then be reallocated and the stream restarted. A typical ioctl
sequence is

> 1. VIDIOC\_STREAMOFF
> 2. VIDIOC\_REQBUFS(0)
> 3. VIDIOC\_S\_EXT\_CTRLS
> 4. VIDIOC\_S\_FMT
> 5. VIDIOC\_REQBUFS(n)
> 6. VIDIOC\_QBUF
> 7. VIDIOC\_STREAMON

The second [`VIDIOC_REQBUFS()`](vidioc-reqbufs.html#c.V4L.VIDIOC_REQBUFS "VIDIOC_REQBUFS") call will take the new format and control
value into account to compute the buffer size to allocate. Applications can
also retrieve the size by calling the [`VIDIOC_G_FMT()`](vidioc-g-fmt.html#c.V4L.VIDIOC_G_FMT "VIDIOC_G_FMT") ioctl if needed.

Note

The API doesn’t mandate the above order for control (3.) and format (4.)
changes. Format and controls can be set in a different order, or even
interleaved, depending on the device and use case. For instance some
controls might behave differently for different pixel formats, in which
case the format might need to be set first.

When reallocation is required, any attempt to modify format or controls that
influences the buffer size while buffers are allocated shall cause the format
or control set ioctl to return the `EBUSY` error. Any attempt to queue a
buffer too small for the current format or controls shall cause the
[`VIDIOC_QBUF()`](vidioc-qbuf.html#c.V4L.VIDIOC_QBUF "VIDIOC_QBUF") ioctl to return a `EINVAL` error.

Buffer reallocation is an expensive operation. To avoid that cost, drivers can
(and are encouraged to) allow format or controls that influence the buffer
size to be changed with buffers allocated. In that case, a typical ioctl
sequence to modify format and controls is

> 1. VIDIOC\_STREAMOFF
> 2. VIDIOC\_S\_EXT\_CTRLS
> 3. VIDIOC\_S\_FMT
> 4. VIDIOC\_QBUF
> 5. VIDIOC\_STREAMON

For this sequence to operate correctly, queued buffers need to be large enough
for the new format or controls. Drivers shall return a `ENOSPC` error in
response to format change ([`VIDIOC_S_FMT()`](vidioc-g-fmt.html#c.V4L.VIDIOC_S_FMT "VIDIOC_S_FMT")) or control changes
([`VIDIOC_S_CTRL()`](vidioc-g-ctrl.html#c.V4L.VIDIOC_S_CTRL "VIDIOC_S_CTRL") or [`VIDIOC_S_EXT_CTRLS()`](vidioc-g-ext-ctrls.html#c.V4L.VIDIOC_S_EXT_CTRLS "VIDIOC_S_EXT_CTRLS")) if buffers too small
for the new format are currently queued. As a simplification, drivers are
allowed to return a `EBUSY` error from these ioctls if any buffer is
currently queued, without checking the queued buffers sizes.

Additionally, drivers shall return a `EINVAL` error from the
[`VIDIOC_QBUF()`](vidioc-qbuf.html#c.V4L.VIDIOC_QBUF "VIDIOC_QBUF") ioctl if the buffer being queued is too small for the
current format or controls. Together, these requirements ensure that queued
buffers will always be large enough for the configured format and controls.

Userspace applications can query the buffer size required for a given format
and controls by first setting the desired control values and then trying the
desired format. The [`VIDIOC_TRY_FMT()`](vidioc-g-fmt.html#c.V4L.VIDIOC_TRY_FMT "VIDIOC_TRY_FMT") ioctl will return the required
buffer size.

> 1. VIDIOC\_S\_EXT\_CTRLS(x)
> 2. [`VIDIOC_TRY_FMT()`](vidioc-g-fmt.html#c.V4L.VIDIOC_TRY_FMT "V4L.VIDIOC_TRY_FMT")
> 3. VIDIOC\_S\_EXT\_CTRLS(y)
> 4. [`VIDIOC_TRY_FMT()`](vidioc-g-fmt.html#c.V4L.VIDIOC_TRY_FMT "V4L.VIDIOC_TRY_FMT")

The [`VIDIOC_CREATE_BUFS()`](vidioc-create-bufs.html#c.V4L.VIDIOC_CREATE_BUFS "VIDIOC_CREATE_BUFS") ioctl can then be used to allocate buffers
based on the queried sizes (for instance by allocating a set of buffers large
enough for all the desired formats and controls, or by allocating separate set
of appropriately sized buffers for each use case).

type v4l2\_buffer

## 3.5.2. struct v4l2\_buffer

struct v4l2\_buffer

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | Number of the buffer, set by the application except when calling [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf), then it is set by the driver. This field can range from zero to the number of buffers allocated with the [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) ioctl (struct [`v4l2_requestbuffers`](vidioc-reqbufs.html#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers") `count`), plus any buffers allocated with [ioctl VIDIOC\_CREATE\_BUFS](vidioc-create-bufs.html#vidioc-create-bufs) minus one. |
| \_\_u32 | `type` | Type of the buffer, same as struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") `type` or struct [`v4l2_requestbuffers`](vidioc-reqbufs.html#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers") `type`, set by the application. See [`v4l2_buf_type`](#c.V4L.v4l2_buf_type "v4l2_buf_type") |
| \_\_u32 | `bytesused` | The number of bytes occupied by the data in the buffer. It depends on the negotiated data format and may change with each buffer for compressed variable size data like JPEG images. Drivers must set this field when `type` refers to a capture stream, applications when it refers to an output stream. For multiplanar formats this field is ignored and the `planes` pointer is used instead. |
| \_\_u32 | `flags` | Flags set by the application or driver, see [Buffer Flags](#buffer-flags). |
| \_\_u32 | `field` | Indicates the field order of the image in the buffer, see [`v4l2_field`](field-order.html#c.V4L.v4l2_field "v4l2_field"). This field is not used when the buffer contains VBI data. Drivers must set it when `type` refers to a capture stream, applications when it refers to an output stream. |
| `struct timeval` | `timestamp` | For capture streams this is time when the first data byte was captured, as returned by the `clock_gettime()` function for the relevant clock id; see `V4L2_BUF_FLAG_TIMESTAMP_*` in [Buffer Flags](#buffer-flags). For output streams the driver stores the time at which the last data byte was actually sent out in the `timestamp` field. This permits applications to monitor the drift between the video and system clock. For output streams that use `V4L2_BUF_FLAG_TIMESTAMP_COPY` the application has to fill in the timestamp which will be copied by the driver to the capture stream. |
| struct [`v4l2_timecode`](#c.V4L.v4l2_timecode "v4l2_timecode") | `timecode` | When the `V4L2_BUF_FLAG_TIMECODE` flag is set in `flags`, this structure contains a frame timecode. In [`V4L2_FIELD_ALTERNATE`](field-order.html#c.V4L.v4l2_field "v4l2_field") mode the top and bottom field contain the same timecode. Timecodes are intended to help video editing and are typically recorded on video tapes, but also embedded in compressed formats like MPEG. This field is independent of the `timestamp` and `sequence` fields. |
| \_\_u32 | `sequence` | Set by the driver, counting the frames (not fields!) in sequence. This field is set for both input and output devices. |
| In [`V4L2_FIELD_ALTERNATE`](field-order.html#c.V4L.v4l2_field "v4l2_field") mode the top and bottom field have the same sequence number. The count starts at zero and includes dropped or repeated frames. A dropped frame was received by an input device but could not be stored due to lack of free buffer space. A repeated frame was displayed again by an output device because the application did not pass new data in time.  Note  This may count the frames received e.g. over USB, without taking into account the frames dropped by the remote hardware due to limited compression throughput or bus bandwidth. These devices identify by not enumerating any video standards, see [Video Standards](standard.html#standard). | | |
| \_\_u32 | `memory` | This field must be set by applications and/or drivers in accordance with the selected I/O method. See [`v4l2_memory`](#c.V4L.v4l2_memory "v4l2_memory") |
| union { | `m` | |
| \_\_u32 | `offset` | For the single-planar API and when `memory` is `V4L2_MEMORY_MMAP` this is the offset of the buffer from the start of the device memory. The value is returned by the driver and apart of serving as parameter to the [`mmap()`](func-mmap.html#c.V4L.mmap "mmap") function not useful for applications. See [Streaming I/O (Memory Mapping)](mmap.html#mmap) for details |
| unsigned long | `userptr` | For the single-planar API and when `memory` is `V4L2_MEMORY_USERPTR` this is a pointer to the buffer (casted to unsigned long type) in virtual memory, set by the application. See [Streaming I/O (User Pointers)](userp.html#userp) for details. |
| [`struct v4l2_plane`](#c.V4L.v4l2_plane "V4L.v4l2_plane") | `*planes` | When using the multi-planar API, contains a userspace pointer to an array of struct [`v4l2_plane`](#c.V4L.v4l2_plane "v4l2_plane"). The size of the array should be put in the `length` field of this struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer") structure. |
| int | `fd` | For the single-plane API and when `memory` is `V4L2_MEMORY_DMABUF` this is the file descriptor associated with a DMABUF buffer. |
| } |  | |
| \_\_u32 | `length` | Size of the buffer (not the payload) in bytes for the single-planar API. This is set by the driver based on the calls to [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) and/or [ioctl VIDIOC\_CREATE\_BUFS](vidioc-create-bufs.html#vidioc-create-bufs). For the multi-planar API the application sets this to the number of elements in the `planes` array. The driver will fill in the actual number of valid elements in that array. |
| \_\_u32 | `reserved2` | A place holder for future extensions. Drivers and applications must set this to 0. |
| \_\_u32 | `request_fd` | The file descriptor of the request to queue the buffer to. If the flag `V4L2_BUF_FLAG_REQUEST_FD` is set, then the buffer will be queued to this request. If the flag is not set, then this field will be ignored.  The `V4L2_BUF_FLAG_REQUEST_FD` flag and this field are only used by [ioctl VIDIOC\_QBUF](vidioc-qbuf.html#vidioc-qbuf) and ignored by other ioctls that take a [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer") as argument.  Applications should not set `V4L2_BUF_FLAG_REQUEST_FD` for any ioctls other than [VIDIOC\_QBUF](vidioc-qbuf.html#vidioc-qbuf).  If the device does not support requests, then `EBADR` will be returned. If requests are supported but an invalid request file descriptor is given, then `EINVAL` will be returned. |

type v4l2\_plane

## 3.5.3. struct v4l2\_plane

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `bytesused` | The number of bytes occupied by data in the plane (its payload). Drivers must set this field when `type` refers to a capture stream, applications when it refers to an output stream.  Note  Note that the actual image data starts at `data_offset` which may not be 0. |
| \_\_u32 | `length` | Size in bytes of the plane (not its payload). This is set by the driver based on the calls to [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) and/or [ioctl VIDIOC\_CREATE\_BUFS](vidioc-create-bufs.html#vidioc-create-bufs). |
| union { | `m` | |
| \_\_u32 | `mem_offset` | When the memory type in the containing struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer") is `V4L2_MEMORY_MMAP`, this is the value that should be passed to [`mmap()`](func-mmap.html#c.V4L.mmap "mmap"), similar to the `offset` field in struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer"). |
| unsigned long | `userptr` | When the memory type in the containing struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer") is `V4L2_MEMORY_USERPTR`, this is a userspace pointer to the memory allocated for this plane by an application. |
| int | `fd` | When the memory type in the containing struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer") is `V4L2_MEMORY_DMABUF`, this is a file descriptor associated with a DMABUF buffer, similar to the `fd` field in struct [`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer"). |
| } |  | |
| \_\_u32 | `data_offset` | Offset in bytes to video data in the plane. Drivers must set this field when `type` refers to a capture stream, applications when it refers to an output stream.  Note  That data\_offset is included in `bytesused`. So the size of the image in the plane is `bytesused`-`data_offset` at offset `data_offset` from the start of the plane. |
| \_\_u32 | `reserved[11]` | Reserved for future use. Should be zeroed by drivers and applications. |

type v4l2\_buf\_type

## 3.5.4. enum v4l2\_buf\_type

|  |  |  |
| --- | --- | --- |
| `V4L2_BUF_TYPE_VIDEO_CAPTURE` | 1 | Buffer of a single-planar video capture stream, see [Video Capture Interface](dev-capture.html#capture). |
| `V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE` | 9 | Buffer of a multi-planar video capture stream, see [Video Capture Interface](dev-capture.html#capture). |
| `V4L2_BUF_TYPE_VIDEO_OUTPUT` | 2 | Buffer of a single-planar video output stream, see [Video Output Interface](dev-output.html#output). |
| `V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE` | 10 | Buffer of a multi-planar video output stream, see [Video Output Interface](dev-output.html#output). |
| `V4L2_BUF_TYPE_VIDEO_OVERLAY` | 3 | Buffer for video overlay, see [Video Overlay Interface](dev-overlay.html#overlay). |
| `V4L2_BUF_TYPE_VBI_CAPTURE` | 4 | Buffer of a raw VBI capture stream, see [Raw VBI Data Interface](dev-raw-vbi.html#raw-vbi). |
| `V4L2_BUF_TYPE_VBI_OUTPUT` | 5 | Buffer of a raw VBI output stream, see [Raw VBI Data Interface](dev-raw-vbi.html#raw-vbi). |
| `V4L2_BUF_TYPE_SLICED_VBI_CAPTURE` | 6 | Buffer of a sliced VBI capture stream, see [Sliced VBI Data Interface](dev-sliced-vbi.html#sliced). |
| `V4L2_BUF_TYPE_SLICED_VBI_OUTPUT` | 7 | Buffer of a sliced VBI output stream, see [Sliced VBI Data Interface](dev-sliced-vbi.html#sliced). |
| `V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY` | 8 | Buffer for video output overlay (OSD), see [Video Output Overlay Interface](dev-osd.html#osd). |
| `V4L2_BUF_TYPE_SDR_CAPTURE` | 11 | Buffer for Software Defined Radio (SDR) capture stream, see [Software Defined Radio Interface (SDR)](dev-sdr.html#sdr). |
| `V4L2_BUF_TYPE_SDR_OUTPUT` | 12 | Buffer for Software Defined Radio (SDR) output stream, see [Software Defined Radio Interface (SDR)](dev-sdr.html#sdr). |
| `V4L2_BUF_TYPE_META_CAPTURE` | 13 | Buffer for metadata capture, see [Metadata Interface](dev-meta.html#metadata). |
| `V4L2_BUF_TYPE_META_OUTPUT` | 14 | Buffer for metadata output, see [Metadata Interface](dev-meta.html#metadata). |

## 3.5.5. Buffer Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_BUF_FLAG_MAPPED` | 0x00000001 | The buffer resides in device memory and has been mapped into the application’s address space, see [Streaming I/O (Memory Mapping)](mmap.html#mmap) for details. Drivers set or clear this flag when the [ioctl VIDIOC\_QUERYBUF](vidioc-querybuf.html#vidioc-querybuf), [ioctl VIDIOC\_QBUF, VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) or [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl is called. Set by the driver. |
| `V4L2_BUF_FLAG_QUEUED` | 0x00000002 | Internally drivers maintain two buffer queues, an incoming and outgoing queue. When this flag is set, the buffer is currently on the incoming queue. It automatically moves to the outgoing queue after the buffer has been filled (capture devices) or displayed (output devices). Drivers set or clear this flag when the `VIDIOC_QUERYBUF` ioctl is called. After (successful) calling the `VIDIOC_QBUF`ioctl it is always set and after `VIDIOC_DQBUF` always cleared. |
| `V4L2_BUF_FLAG_DONE` | 0x00000004 | When this flag is set, the buffer is currently on the outgoing queue, ready to be dequeued from the driver. Drivers set or clear this flag when the `VIDIOC_QUERYBUF` ioctl is called. After calling the `VIDIOC_QBUF` or `VIDIOC_DQBUF` it is always cleared. Of course a buffer cannot be on both queues at the same time, the `V4L2_BUF_FLAG_QUEUED` and `V4L2_BUF_FLAG_DONE` flag are mutually exclusive. They can be both cleared however, then the buffer is in “dequeued” state, in the application domain so to say. |
| `V4L2_BUF_FLAG_ERROR` | 0x00000040 | When this flag is set, the buffer has been dequeued successfully, although the data might have been corrupted. This is recoverable, streaming may continue as normal and the buffer may be reused normally. Drivers set this flag when the `VIDIOC_DQBUF` ioctl is called. |
| `V4L2_BUF_FLAG_IN_REQUEST` | 0x00000080 | This buffer is part of a request that hasn’t been queued yet. |
| `V4L2_BUF_FLAG_KEYFRAME` | 0x00000008 | Drivers set or clear this flag when calling the `VIDIOC_DQBUF` ioctl. It may be set by video capture devices when the buffer contains a compressed image which is a key frame (or field), i. e. can be decompressed on its own. Also known as an I-frame. Applications can set this bit when `type` refers to an output stream. |
| `V4L2_BUF_FLAG_PFRAME` | 0x00000010 | Similar to `V4L2_BUF_FLAG_KEYFRAME` this flags predicted frames or fields which contain only differences to a previous key frame. Applications can set this bit when `type` refers to an output stream. |
| `V4L2_BUF_FLAG_BFRAME` | 0x00000020 | Similar to `V4L2_BUF_FLAG_KEYFRAME` this flags a bi-directional predicted frame or field which contains only the differences between the current frame and both the preceding and following key frames to specify its content. Applications can set this bit when `type` refers to an output stream. |
| `V4L2_BUF_FLAG_TIMECODE` | 0x00000100 | The `timecode` field is valid. Drivers set or clear this flag when the `VIDIOC_DQBUF` ioctl is called. Applications can set this bit and the corresponding `timecode` structure when `type` refers to an output stream. |
| `V4L2_BUF_FLAG_PREPARED` | 0x00000400 | The buffer has been prepared for I/O and can be queued by the application. Drivers set or clear this flag when the [VIDIOC\_QUERYBUF](vidioc-querybuf.html#vidioc-querybuf), [VIDIOC\_PREPARE\_BUF](vidioc-qbuf.html#vidioc-qbuf), [VIDIOC\_QBUF](vidioc-qbuf.html#vidioc-qbuf) or [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl is called. |
| `V4L2_BUF_FLAG_NO_CACHE_INVALIDATE` | 0x00000800 | Caches do not have to be invalidated for this buffer. Typically applications shall use this flag if the data captured in the buffer is not going to be touched by the CPU, instead the buffer will, probably, be passed on to a DMA-capable hardware unit for further processing or output. This flag is ignored unless the queue is used for [memory mapping](mmap.html#mmap) streaming I/O and reports [V4L2\_BUF\_CAP\_SUPPORTS\_MMAP\_CACHE\_HINTS](vidioc-reqbufs.html#v4l2-buf-cap-supports-mmap-cache-hints) capability. |
| `V4L2_BUF_FLAG_NO_CACHE_CLEAN` | 0x00001000 | Caches do not have to be cleaned for this buffer. Typically applications shall use this flag for output buffers if the data in this buffer has not been created by the CPU but by some DMA-capable unit, in which case caches have not been used. This flag is ignored unless the queue is used for [memory mapping](mmap.html#mmap) streaming I/O and reports [V4L2\_BUF\_CAP\_SUPPORTS\_MMAP\_CACHE\_HINTS](vidioc-reqbufs.html#v4l2-buf-cap-supports-mmap-cache-hints) capability. |
| `V4L2_BUF_FLAG_M2M_HOLD_CAPTURE_BUF` | 0x00000200 | Only valid if struct [`v4l2_requestbuffers`](vidioc-reqbufs.html#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers") flag `V4L2_BUF_CAP_SUPPORTS_M2M_HOLD_CAPTURE_BUF` is set. It is typically used with stateless decoders where multiple output buffers each decode to a slice of the decoded frame. Applications can set this flag when queueing the output buffer to prevent the driver from dequeueing the capture buffer after the output buffer has been decoded (i.e. the capture buffer is ‘held’). If the timestamp of this output buffer differs from that of the previous output buffer, then that indicates the start of a new frame and the previously held capture buffer is dequeued. |
| `V4L2_BUF_FLAG_LAST` | 0x00100000 | Last buffer produced by the hardware. mem2mem codec drivers set this flag on the capture queue for the last buffer when the [ioctl VIDIOC\_QUERYBUF](vidioc-querybuf.html#vidioc-querybuf) or [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl is called. Due to hardware limitations, the last buffer may be empty. In this case the driver will set the `bytesused` field to 0, regardless of the format. Any subsequent call to the [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl will not block anymore, but return an `EPIPE` error code. |
| `V4L2_BUF_FLAG_REQUEST_FD` | 0x00800000 | The `request_fd` field contains a valid file descriptor. |
| `V4L2_BUF_FLAG_TIMESTAMP_MASK` | 0x0000e000 | Mask for timestamp types below. To test the timestamp type, mask out bits not belonging to timestamp type by performing a logical and operation with buffer flags and timestamp mask. |
| `V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN` | 0x00000000 | Unknown timestamp type. This type is used by drivers before Linux 3.9 and may be either monotonic (see below) or realtime (wall clock). Monotonic clock has been favoured in embedded systems whereas most of the drivers use the realtime clock. Either kinds of timestamps are available in user space via `clock_gettime()` using clock IDs `CLOCK_MONOTONIC` and `CLOCK_REALTIME`, respectively. |
| `V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC` | 0x00002000 | The buffer timestamp has been taken from the `CLOCK_MONOTONIC` clock. To access the same clock outside V4L2, use `clock_gettime()`. |
| `V4L2_BUF_FLAG_TIMESTAMP_COPY` | 0x00004000 | The CAPTURE buffer timestamp has been taken from the corresponding OUTPUT buffer. This flag applies only to mem2mem devices. |
| `V4L2_BUF_FLAG_TSTAMP_SRC_MASK` | 0x00070000 | Mask for timestamp sources below. The timestamp source defines the point of time the timestamp is taken in relation to the frame. Logical ‘and’ operation between the `flags` field and `V4L2_BUF_FLAG_TSTAMP_SRC_MASK` produces the value of the timestamp source. Applications must set the timestamp source when `type` refers to an output stream and `V4L2_BUF_FLAG_TIMESTAMP_COPY` is set. |
| `V4L2_BUF_FLAG_TSTAMP_SRC_EOF` | 0x00000000 | End Of Frame. The buffer timestamp has been taken when the last pixel of the frame has been received or the last pixel of the frame has been transmitted. In practice, software generated timestamps will typically be read from the clock a small amount of time after the last pixel has been received or transmitten, depending on the system and other activity in it. |
| `V4L2_BUF_FLAG_TSTAMP_SRC_SOE` | 0x00010000 | Start Of Exposure. The buffer timestamp has been taken when the exposure of the frame has begun. This is only valid for the `V4L2_BUF_TYPE_VIDEO_CAPTURE` buffer type. |

enum v4l2\_memory

## 3.5.6. enum v4l2\_memory

|  |  |  |
| --- | --- | --- |
| `V4L2_MEMORY_MMAP` | 1 | The buffer is used for [memory mapping](mmap.html#mmap) I/O. |
| `V4L2_MEMORY_USERPTR` | 2 | The buffer is used for [user pointer](userp.html#userp) I/O. |
| `V4L2_MEMORY_OVERLAY` | 3 | [to do] |
| `V4L2_MEMORY_DMABUF` | 4 | The buffer is used for [DMA shared buffer](dmabuf.html#dmabuf) I/O. |

## 3.5.7. Timecodes

The `v4l2_buffer_timecode` structure is designed to hold a
[SMPTE 12M](biblio.html#smpte12m) or similar timecode.
(struct `timeval` timestamps are stored in the struct
[`v4l2_buffer`](#c.V4L.v4l2_buffer "v4l2_buffer") `timestamp` field.)

type v4l2\_timecode

### 3.5.7.1. struct v4l2\_timecode

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | Frame rate the timecodes are based on, see [Timecode Types](#timecode-type). |
| \_\_u32 | `flags` | Timecode flags, see [Timecode Flags](#timecode-flags). |
| \_\_u8 | `frames` | Frame count, 0 ... 23/24/29/49/59, depending on the type of timecode. |
| \_\_u8 | `seconds` | Seconds count, 0 ... 59. This is a binary, not BCD number. |
| \_\_u8 | `minutes` | Minutes count, 0 ... 59. This is a binary, not BCD number. |
| \_\_u8 | `hours` | Hours count, 0 ... 29. This is a binary, not BCD number. |
| \_\_u8 | `userbits`[4] | The “user group” bits from the timecode. |

### 3.5.7.2. Timecode Types

|  |  |  |
| --- | --- | --- |
| `V4L2_TC_TYPE_24FPS` | 1 | 24 frames per second, i. e. film. |
| `V4L2_TC_TYPE_25FPS` | 2 | 25 frames per second, i. e. PAL or SECAM video. |
| `V4L2_TC_TYPE_30FPS` | 3 | 30 frames per second, i. e. NTSC video. |
| `V4L2_TC_TYPE_50FPS` | 4 |  |
| `V4L2_TC_TYPE_60FPS` | 5 |  |

### 3.5.7.3. Timecode Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_TC_FLAG_DROPFRAME` | 0x0001 | Indicates “drop frame” semantics for counting frames in 29.97 fps material. When set, frame numbers 0 and 1 at the start of each minute, except minutes 0, 10, 20, 30, 40, 50 are omitted from the count. |
| `V4L2_TC_FLAG_COLORFRAME` | 0x0002 | The “color frame” flag. |
| `V4L2_TC_USERBITS_field` | 0x000C | Field mask for the “binary group flags”. |
| `V4L2_TC_USERBITS_USERDEFINED` | 0x0000 | Unspecified format. |
| `V4L2_TC_USERBITS_8BITCHARS` | 0x0008 | 8-bit ISO characters. |
