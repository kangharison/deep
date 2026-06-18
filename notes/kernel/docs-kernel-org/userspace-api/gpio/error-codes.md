# GPIO Error Codes

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/error-codes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO Error Codes

Common GPIO error codes

|  |  |
| --- | --- |
| `EAGAIN` (aka `EWOULDBLOCK`) | The device was opened in non-blocking mode and a read can’t be performed as there is no data available. |
| `EBADF` | The file descriptor is not valid. |
| `EBUSY` | The ioctl can’t be handled because the device is busy. Typically returned when an ioctl attempts something that would require the usage of a resource that was already allocated. The ioctl must not be retried without performing another action to fix the problem first. |
| `EFAULT` | There was a failure while copying data from/to userspace, probably caused by an invalid pointer reference. |
| `EINVAL` | One or more of the ioctl parameters are invalid or out of the allowed range. This is a widely used error code. |
| `ENODEV` | Device not found or was removed. |
| `ENOMEM` | There’s not enough memory to handle the desired operation. |
| `EPERM` | Permission denied. Typically returned in response to an attempt to perform an action incompatible with the current line configuration. |
| `EIO` | I/O error. Typically returned when there are problems communicating with a hardware device or requesting features that hardware does not support. This could indicate broken or flaky hardware. It’s a ‘Something is wrong, I give up!’ type of error. |
| `ENXIO` | Typically returned when a feature requiring interrupt support was requested, but the line does not support interrupts. |

Note

1. This list is not exhaustive; ioctls may return other error codes.
   Since errors may have side effects such as a driver reset,
   applications should abort on unexpected errors, or otherwise
   assume that the device is in a bad state.
2. Request-specific error codes are listed in the individual
   requests descriptions.
