# Generic Error Codes

> 출처(원문): https://docs.kernel.org/userspace-api/media/gen-errors.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Generic Error Codes

Generic error codes

|  |  |
| --- | --- |
| `EAGAIN` (aka `EWOULDBLOCK`) | The ioctl can’t be handled because the device is in state where it can’t perform it. This could happen for example in case where device is sleeping and ioctl is performed to query statistics. It is also returned when the ioctl would need to wait for an event, but the device was opened in non-blocking mode. |
| `EBADF` | The file descriptor is not a valid. |
| `EBUSY` | The ioctl can’t be handled because the device is busy. This is typically return while device is streaming, and an ioctl tried to change something that would affect the stream, or would require the usage of a hardware resource that was already allocated. The ioctl must not be retried without performing another action to fix the problem first (typically: stop the stream before retrying). |
| `EFAULT` | There was a failure while copying data from/to userspace, probably caused by an invalid pointer reference. |
| `EINVAL` | One or more of the ioctl parameters are invalid or out of the allowed range. This is a widely used error code. See the individual ioctl requests for specific causes. |
| `ENODEV` | Device not found or was removed. |
| `ENOMEM` | There’s not enough memory to handle the desired operation. |
| `ENOTTY` | The ioctl is not supported by the file descriptor. |
| `ENOSPC` | On USB devices, the stream ioctl’s can return this error, meaning that this request would overcommit the usb bandwidth reserved for periodic transfers (up to 80% of the USB bandwidth). |
| `EPERM` | Permission denied. Can be returned if the device needs write permission, or some special capabilities is needed (e. g. root) |
| `EIO` | I/O error. Typically used when there are problems communicating with a hardware device. This could indicate broken or flaky hardware. It’s a ‘Something is wrong, I give up!’ type of error. |
| `ENXIO` | No device corresponding to this device special file exists. |

Note

1. This list is not exhaustive; ioctls may return other error codes.
   Since errors may have side effects such as a driver reset,
   applications should abort on unexpected errors, or otherwise
   assume that the device is in a bad state.
2. Request-specific error codes are listed in the individual
   requests descriptions.
