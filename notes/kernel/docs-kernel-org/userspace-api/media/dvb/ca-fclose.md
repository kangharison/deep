# 4.2.2.Digital TV CA close()

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-fclose.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.2. Digital TV CA close()

## 4.2.2.1. Name

Digital TV CA [`close()`](#c.DTV.ca.close "DTV.ca.close")

## 4.2.2.2. Synopsis

int close(int fd)

## 4.2.2.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](ca-fopen.html#c.DTV.ca.open "open").

## 4.2.2.4. Description

This system call closes a previously opened CA device.

## 4.2.2.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
