# 2.1.cec open()

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-func-open.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.1. cec open()

## 2.1.1. Name

cec-open - Open a cec device

## 2.1.2. Synopsis

```
#include <fcntl.h>
```

int open(const char \*device\_name, int flags)

## 2.1.3. Arguments

`device_name`
:   Device to be opened.

`flags`
:   Open flags. Access mode must be `O_RDWR`.

    When the `O_NONBLOCK` flag is given, the
    [CEC\_RECEIVE](cec-ioc-receive.html#cec-receive) and [CEC\_DQEVENT](cec-ioc-dqevent.html#cec-dqevent) ioctls
    will return the `EAGAIN` error code when no message or event is available, and
    ioctls [CEC\_TRANSMIT](cec-ioc-receive.html#cec-transmit),
    [CEC\_ADAP\_S\_PHYS\_ADDR](cec-ioc-adap-g-phys-addr.html#cec-adap-s-phys-addr) and
    [CEC\_ADAP\_S\_LOG\_ADDRS](cec-ioc-adap-g-log-addrs.html#cec-adap-s-log-addrs)
    all return 0.

    Other flags have no effect.

## 2.1.4. Description

To open a cec device applications call [`open()`](#c.CEC.open "open") with the
desired device name. The function has no side effects; the device
configuration remain unchanged.

When the device is opened in read-only mode, attempts to modify its
configuration will result in an error, and `errno` will be set to
EBADF.

## 2.1.5. Return Value

[`open()`](#c.CEC.open "open") returns the new file descriptor on success. On error,
-1 is returned, and `errno` is set appropriately. Possible error codes
include:

`EACCES`
:   The requested access to the file is not allowed.

`EMFILE`
:   The process already has the maximum number of files open.

`ENFILE`
:   The system limit on the total number of open files has been reached.

`ENOMEM`
:   Insufficient kernel memory was available.

`ENODEV`
:   Device not found or was removed.
