# 4.2.7.CA_GET_MSG

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-get-msg.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.7. CA\_GET\_MSG

## 4.2.7.1. Name

CA\_GET\_MSG

## 4.2.7.2. Synopsis

CA\_GET\_MSG

`int ioctl(fd, CA_GET_MSG, struct ca_msg *msg)`

## 4.2.7.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](ca-fopen.html#c.DTV.ca.open "open").

`msg`
:   Pointer to struct [`ca_msg`](ca_data_types.html#c.ca_msg "ca_msg").

## 4.2.7.4. Description

Receives a message via a CI CA module.

Note

Please notice that, on most drivers, this is done by reading from
the /dev/adapter?/ca? device node.

## 4.2.7.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
