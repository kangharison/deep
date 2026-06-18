# 4.2.1.Digital TV CA open()

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-fopen.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.1. Digital TV CA open()

## 4.2.1.1. Name

Digital TV CA [`open()`](#c.DTV.ca.open "DTV.ca.open")

## 4.2.1.2. Synopsis

int open(const char \*name, int flags)

## 4.2.1.3. Arguments

`name`
:   Name of specific Digital TV CA device.

`flags`
:   A bit-wise OR of the following flags:

|  |  |
| --- | --- |
| `O_RDONLY` | read-only access |
| `O_RDWR` | read/write access |
| `O_NONBLOCK` | open in non-blocking mode (blocking mode is the default) |

## 4.2.1.4. Description

This system call opens a named ca device (e.g. `/dev/dvb/adapter?/ca?`)
for subsequent use.

When an `open()` call has succeeded, the device will be ready for use. The
significance of blocking or non-blocking mode is described in the
documentation for functions where there is a difference. It does not
affect the semantics of the `open()` call itself. A device opened in
blocking mode can later be put into non-blocking mode (and vice versa)
using the `F_SETFL` command of the `fcntl` system call. This is a
standard system call, documented in the Linux manual page for fcntl.
Only one user can open the CA Device in `O_RDWR` mode. All other
attempts to open the device in this mode will fail, and an error code
will be returned.

## 4.2.1.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
