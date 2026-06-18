# 4.2.4.CA_GET_CAP

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-get-cap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.4. CA\_GET\_CAP

## 4.2.4.1. Name

CA\_GET\_CAP

## 4.2.4.2. Synopsis

CA\_GET\_CAP

`int ioctl(fd, CA_GET_CAP, struct ca_caps *caps)`

## 4.2.4.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](ca-fopen.html#c.DTV.ca.open "open").

`caps`
:   Pointer to struct [`ca_caps`](ca_data_types.html#c.ca_caps "ca_caps").

## 4.2.4.4. Description

Queries the Kernel for information about the available CA and descrambler
slots, and their types.

## 4.2.4.5. Return Value

On success 0 is returned and [`ca_caps`](ca_data_types.html#c.ca_caps "ca_caps") is filled.

On error, -1 is returned and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
