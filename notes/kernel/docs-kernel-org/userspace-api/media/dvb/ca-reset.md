# 4.2.3.CA_RESET

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-reset.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.3. CA\_RESET

## 4.2.3.1. Name

CA\_RESET

## 4.2.3.2. Synopsis

CA\_RESET

`int ioctl(fd, CA_RESET)`

## 4.2.3.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](ca-fopen.html#c.DTV.ca.open "open").

## 4.2.3.4. Description

Puts the Conditional Access hardware on its initial state. It should
be called before start using the CA hardware.

## 4.2.3.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
