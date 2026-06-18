# 4.2.9.CA_SET_DESCR

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-set-descr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.9. CA\_SET\_DESCR

## 4.2.9.1. Name

CA\_SET\_DESCR

## 4.2.9.2. Synopsis

CA\_SET\_DESCR

`int ioctl(fd, CA_SET_DESCR, struct ca_descr *desc)`

## 4.2.9.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](ca-fopen.html#c.DTV.ca.open "open").

`msg`
:   Pointer to struct [`ca_descr`](ca_data_types.html#c.ca_descr "ca_descr").

## 4.2.9.4. Description

CA\_SET\_DESCR is used for feeding descrambler CA slots with descrambling
keys (referred as control words).

## 4.2.9.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
