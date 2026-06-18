# 7.3.ioctl VIDIOC_CREATE_BUFS

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-create-bufs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.3. ioctl VIDIOC\_CREATE\_BUFS

## 7.3.1. Name

VIDIOC\_CREATE\_BUFS - Create buffers for Memory Mapped or User Pointer or DMA Buffer I/O

## 7.3.2. Synopsis

VIDIOC\_CREATE\_BUFS

`int ioctl(int fd, VIDIOC_CREATE_BUFS, struct v4l2_create_buffers *argp)`

## 7.3.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_create_buffers`](#c.V4L.v4l2_create_buffers "v4l2_create_buffers").

## 7.3.4. Description

This ioctl is used to create buffers for [memory mapped](mmap.html#mmap)
or [user pointer](userp.html#userp) or [DMA buffer](dmabuf.html#dmabuf) I/O. It
can be used as an alternative or in addition to the
[ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) ioctl, when a tighter control
over buffers is required. This ioctl can be called multiple times to
create buffers of different sizes.

To allocate the device buffers applications must initialize the relevant
fields of the struct [`v4l2_create_buffers`](#c.V4L.v4l2_create_buffers "v4l2_create_buffers") structure. The
`count` field must be set to the number of requested buffers, the
`memory` field specifies the requested I/O method and the `reserved`
array must be zeroed.

The `format` field specifies the image format that the buffers must be
able to handle. The application has to fill in this struct
[`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format"). Usually this will be done using the
[VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) or
[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctls to ensure that the
requested format is supported by the driver. Based on the format’s
`type` field the requested buffer size (for single-planar) or plane
sizes (for multi-planar formats) will be used for the allocated buffers.
The driver may return an error if the size(s) are not supported by the
hardware (usually because they are too small).

The buffers created by this ioctl will have as minimum size the size
defined by the `format.pix.sizeimage` field (or the corresponding
fields for other format types). Usually if the `format.pix.sizeimage`
field is less than the minimum required for the given format, then an
error will be returned since drivers will typically not allow this. If
it is larger, then the value will be used as-is. In other words, the
driver may reject the requested size, but if it is accepted the driver
will use it unchanged.

When the ioctl is called with a pointer to this structure the driver
will attempt to allocate up to the requested number of buffers and store
the actual number allocated and the starting index in the `count` and
the `index` fields respectively. On return `count` can be smaller
than the number requested.

type v4l2\_create\_buffers

struct v4l2\_create\_buffers

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | The starting buffer index, returned by the driver. |
| \_\_u32 | `count` | The number of buffers requested or granted. If count == 0, then [ioctl VIDIOC\_CREATE\_BUFS](#vidioc-create-bufs) will set `index` to the current number of created buffers, and it will check the validity of `memory` and `format.type`. If those are invalid -1 is returned and errno is set to `EINVAL` error code, otherwise [ioctl VIDIOC\_CREATE\_BUFS](#vidioc-create-bufs) returns 0. It will never set errno to `EBUSY` error code in this particular case. |
| \_\_u32 | `memory` | Applications set this field to `V4L2_MEMORY_MMAP`, `V4L2_MEMORY_DMABUF` or `V4L2_MEMORY_USERPTR`. See [`v4l2_memory`](buffer.html#c.V4L.v4l2_memory "v4l2_memory") |
| struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") | `format` | Filled in by the application, preserved by the driver. |
| \_\_u32 | `capabilities` | Set by the driver. If 0, then the driver doesn’t support capabilities. In that case all you know is that the driver is guaranteed to support `V4L2_MEMORY_MMAP` and *might* support other [`v4l2_memory`](buffer.html#c.V4L.v4l2_memory "v4l2_memory") types. It will not support any other capabilities. See [here](vidioc-reqbufs.html#v4l2-buf-capabilities) for a list of the capabilities.  If you want to just query the capabilities without making any other changes, then set `count` to 0, `memory` to `V4L2_MEMORY_MMAP` and `format.type` to the buffer type. |
| \_\_u32 | `flags` | Specifies additional buffer management attributes. See [Memory Consistency Flags](vidioc-reqbufs.html#memory-flags). |
| \_\_u32 | `max_num_buffers` | If the V4L2\_BUF\_CAP\_SUPPORTS\_MAX\_NUM\_BUFFERS capability flag is set this field indicates the maximum possible number of buffers for this queue. |
| \_\_u32 | `reserved`[5] | A place holder for future extensions. Drivers and applications must set the array to zero. |

## 7.3.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

ENOMEM
:   No memory to allocate buffers for [memory mapped](mmap.html#mmap) I/O.

EINVAL
:   The buffer type (`format.type` field), requested I/O method
    (`memory`) or format (`format` field) is not valid.
