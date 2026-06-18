# 7.38.ioctl VIDIOC_G_PRIORITY, VIDIOC_S_PRIORITY

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-priority.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.38. ioctl VIDIOC\_G\_PRIORITY, VIDIOC\_S\_PRIORITY

## 7.38.1. Name

VIDIOC\_G\_PRIORITY - VIDIOC\_S\_PRIORITY - Query or request the access priority associated with a file descriptor

## 7.38.2. Synopsis

VIDIOC\_G\_PRIORITY

`int ioctl(int fd, VIDIOC_G_PRIORITY, enum v4l2_priority *argp)`

VIDIOC\_S\_PRIORITY

`int ioctl(int fd, VIDIOC_S_PRIORITY, const enum v4l2_priority *argp)`

## 7.38.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to an enum [`v4l2_priority`](#c.V4L.v4l2_priority "v4l2_priority") type.

## 7.38.4. Description

To query the current access priority applications call the
[VIDIOC\_G\_PRIORITY](#vidioc-g-priority) ioctl with a pointer to an [`enum v4l2_priority`](#c.V4L.v4l2_priority "V4L.v4l2_priority")
variable where the driver stores the current priority.

To request an access priority applications store the desired priority in
an [`enum v4l2_priority`](#c.V4L.v4l2_priority "V4L.v4l2_priority") variable and call [VIDIOC\_S\_PRIORITY](#vidioc-g-priority) ioctl
with a pointer to this variable.

type v4l2\_priority

enum v4l2\_priority

|  |  |  |
| --- | --- | --- |
| `V4L2_PRIORITY_UNSET` | 0 |  |
| `V4L2_PRIORITY_BACKGROUND` | 1 | Lowest priority, usually applications running in background, for example monitoring VBI transmissions. A proxy application running in user space will be necessary if multiple applications want to read from a device at this priority. |
| `V4L2_PRIORITY_INTERACTIVE` | 2 |  |
| `V4L2_PRIORITY_DEFAULT` | 2 | Medium priority, usually applications started and interactively controlled by the user. For example TV viewers, Teletext browsers, or just “panel” applications to change the channel or video controls. This is the default priority unless an application requests another. |
| `V4L2_PRIORITY_RECORD` | 3 | Highest priority. Only one file descriptor can have this priority, it blocks any other fd from changing device properties. Usually applications which must not be interrupted, like video recording. |

## 7.38.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The requested priority value is invalid.

EBUSY
:   Another application already requested higher priority.
