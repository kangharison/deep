# 7.52.ioctl VIDIOC_REQBUFS

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-reqbufs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.52. ioctl VIDIOC\_REQBUFS

## 7.52.1. Name

VIDIOC\_REQBUFS - Initiate Memory Mapping, User Pointer I/O or DMA buffer I/O

## 7.52.2. Synopsis

VIDIOC\_REQBUFS

`int ioctl(int fd, VIDIOC_REQBUFS, struct v4l2_requestbuffers *argp)`

## 7.52.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_requestbuffers`](#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers").

## 7.52.4. Description

This ioctl is used to initiate [memory mapped](mmap.html#mmap),
[user pointer](userp.html#userp) or [DMABUF](dmabuf.html#dmabuf) based I/O.
Memory mapped buffers are located in device memory and must be allocated
with this ioctl before they can be mapped into the application’s address
space. User buffers are allocated by applications themselves, and this
ioctl is merely used to switch the driver into user pointer I/O mode and
to setup some internal structures. Similarly, DMABUF buffers are
allocated by applications through a device driver, and this ioctl only
configures the driver into DMABUF I/O mode without performing any direct
allocation.

To allocate device buffers applications initialize all fields of the
struct [`v4l2_requestbuffers`](#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers") structure. They set the `type`
field to the respective stream or buffer type, the `count` field to
the desired number of buffers, `memory` must be set to the requested
I/O method and the `reserved` array must be zeroed. When the ioctl is
called with a pointer to this structure the driver will attempt to
allocate the requested number of buffers and it stores the actual number
allocated in the `count` field. It can be smaller than the number
requested, even zero, when the driver runs out of free memory. A larger
number is also possible when the driver requires more buffers to
function correctly. For example video output requires at least two
buffers, one displayed and one filled by the application.

When the I/O method is not supported the ioctl returns an `EINVAL` error
code.

Applications can call [ioctl VIDIOC\_REQBUFS](#vidioc-reqbufs) again to change the number of
buffers. Note that if any buffers are still mapped or exported via DMABUF,
then [ioctl VIDIOC\_REQBUFS](#vidioc-reqbufs) can only succeed if the
`V4L2_BUF_CAP_SUPPORTS_ORPHANED_BUFS` capability is set. Otherwise
[ioctl VIDIOC\_REQBUFS](#vidioc-reqbufs) will return the `EBUSY` error code.
If `V4L2_BUF_CAP_SUPPORTS_ORPHANED_BUFS` is set, then these buffers are
orphaned and will be freed when they are unmapped or when the exported DMABUF
fds are closed. A `count` value of zero frees or orphans all buffers, after
aborting or finishing any DMA in progress, an implicit
[VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon).

type v4l2\_requestbuffers

struct v4l2\_requestbuffers

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `count` | The number of buffers requested or granted. |
| \_\_u32 | `type` | Type of the stream or buffers, this is the same as the struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") `type` field. See [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type") for valid values. |
| \_\_u32 | `memory` | Applications set this field to `V4L2_MEMORY_MMAP`, `V4L2_MEMORY_DMABUF` or `V4L2_MEMORY_USERPTR`. See [`v4l2_memory`](buffer.html#c.V4L.v4l2_memory "v4l2_memory"). |
| \_\_u32 | `capabilities` | Set by the driver. If 0, then the driver doesn’t support capabilities. In that case all you know is that the driver is guaranteed to support `V4L2_MEMORY_MMAP` and *might* support other [`v4l2_memory`](buffer.html#c.V4L.v4l2_memory "v4l2_memory") types. It will not support any other capabilities.  If you want to query the capabilities with a minimum of side-effects, then this can be called with `count` set to 0, `memory` set to `V4L2_MEMORY_MMAP` and `type` set to the buffer type. This will free any previously allocated buffers, so this is typically something that will be done at the start of the application. |
| \_\_u8 | `flags` | Specifies additional buffer management attributes. See [Memory Consistency Flags](#memory-flags). |
| \_\_u8 | `reserved`[3] | Reserved for future extensions. |

V4L2 Buffer Capabilities Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_BUF_CAP_SUPPORTS_MMAP` | 0x00000001 | This buffer type supports the `V4L2_MEMORY_MMAP` streaming mode. |
| `V4L2_BUF_CAP_SUPPORTS_USERPTR` | 0x00000002 | This buffer type supports the `V4L2_MEMORY_USERPTR` streaming mode. |
| `V4L2_BUF_CAP_SUPPORTS_DMABUF` | 0x00000004 | This buffer type supports the `V4L2_MEMORY_DMABUF` streaming mode. |
| `V4L2_BUF_CAP_SUPPORTS_REQUESTS` | 0x00000008 | This buffer type supports [requests](../mediactl/request-api.html#media-request-api). |
| `V4L2_BUF_CAP_SUPPORTS_ORPHANED_BUFS` | 0x00000010 | The kernel allows calling [ioctl VIDIOC\_REQBUFS](#vidioc-reqbufs) while buffers are still mapped or exported via DMABUF. These orphaned buffers will be freed when they are unmapped or when the exported DMABUF fds are closed. |
| `V4L2_BUF_CAP_SUPPORTS_M2M_HOLD_CAPTURE_BUF` | 0x00000020 | Only valid for stateless decoders. If set, then userspace can set the `V4L2_BUF_FLAG_M2M_HOLD_CAPTURE_BUF` flag to hold off on returning the capture buffer until the OUTPUT timestamp changes. |
| `V4L2_BUF_CAP_SUPPORTS_MMAP_CACHE_HINTS` | 0x00000040 | This capability is set by the driver to indicate that the queue supports cache and memory management hints. However, it’s only valid when the queue is used for [memory mapping](mmap.html#mmap) streaming I/O. See [V4L2\_BUF\_FLAG\_NO\_CACHE\_INVALIDATE](buffer.html#v4l2-buf-flag-no-cache-invalidate), [V4L2\_BUF\_FLAG\_NO\_CACHE\_CLEAN](buffer.html#v4l2-buf-flag-no-cache-clean) and [V4L2\_MEMORY\_FLAG\_NON\_COHERENT](#v4l2-memory-flag-non-coherent). |
| `V4L2_BUF_CAP_SUPPORTS_MAX_NUM_BUFFERS` | 0x00000080 | If set, then the `max_num_buffers` field in `struct v4l2_create_buffers` is valid. If not set, then the maximum is `VIDEO_MAX_FRAME` buffers. |
| `V4L2_BUF_CAP_SUPPORTS_REMOVE_BUFS` | 0x00000100 | If set, then `VIDIOC_REMOVE_BUFS` is supported. |

Memory Consistency Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_MEMORY_FLAG_NON_COHERENT` | 0x00000001 | A buffer is allocated either in coherent (it will be automatically coherent between the CPU and the bus) or non-coherent memory. The latter can provide performance gains, for instance the CPU cache sync/flush operations can be avoided if the buffer is accessed by the corresponding device only and the CPU does not read/write to/from that buffer. However, this requires extra care from the driver -- it must guarantee memory consistency by issuing a cache flush/sync when consistency is needed. If this flag is set V4L2 will attempt to allocate the buffer in non-coherent memory. The flag takes effect only if the buffer is used for [memory mapping](mmap.html#mmap) I/O and the queue reports the [V4L2\_BUF\_CAP\_SUPPORTS\_MMAP\_CACHE\_HINTS](#v4l2-buf-cap-supports-mmap-cache-hints) capability. |

## 7.52.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The buffer type (`type` field) or the requested I/O method
    (`memory`) is not supported.
