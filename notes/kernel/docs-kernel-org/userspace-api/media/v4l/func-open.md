# 7.69.V4L2 open()

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/func-open.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.69. V4L2 open()

## 7.69.1. Name

v4l2-open - Open a V4L2 device

## 7.69.2. Synopsis

```
#include <fcntl.h>
```

int open(const char \*device\_name, int flags)

## 7.69.3. Arguments

`device_name`
:   Device to be opened.

`flags`
:   Open flags. Access mode must be `O_RDWR`. This is just a
    technicality, input devices still support only reading and output
    devices only writing.

    When the `O_NONBLOCK` flag is given, the [`read()`](func-read.html#c.V4L.read "read")
    function and the [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl will
    return the `EAGAIN` error code when no data is available or no
    buffer is in the driver outgoing queue, otherwise these functions
    block until data becomes available. All V4L2 drivers exchanging data
    with applications must support the `O_NONBLOCK` flag.

    Other flags have no effect.

## 7.69.4. Description

To open a V4L2 device applications call [`open()`](#c.V4L.open "open") with the
desired device name. This function has no side effects; all data format
parameters, current input or output, control values or other properties
remain unchanged. At the first [`open()`](#c.V4L.open "open") call after loading the
driver they will be reset to default values, drivers are never in an
undefined state.

## 7.69.5. Return Value

On success [`open()`](#c.V4L.open "open") returns the new file descriptor. On error
-1 is returned, and the `errno` variable is set appropriately.
Possible error codes are:

EACCES
:   The caller has no permission to access the device.

EBUSY
:   The driver does not support multiple opens and the device is already
    in use.

ENODEV
:   Device not found or was removed.

ENOMEM
:   Not enough kernel memory was available to complete the request.

EMFILE
:   The process already has the maximum number of files open.

ENFILE
:   The limit on the total number of files open on the system has been
    reached.
