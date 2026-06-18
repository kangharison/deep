# 5.1.3.ioctl NET_REMOVE_IF

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/net-remove-if.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.1.3. ioctl NET\_REMOVE\_IF

## 5.1.3.1. Name

NET\_REMOVE\_IF - Removes a network interface.

## 5.1.3.2. Synopsis

NET\_REMOVE\_IF

`int ioctl(int fd, NET_REMOVE_IF, int ifnum)`

## 5.1.3.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`net_if`
:   number of the interface to be removed

## 5.1.3.4. Description

The NET\_REMOVE\_IF ioctl deletes an interface previously created via
[NET\_ADD\_IF](net.html#net).

## 5.1.3.5. Return Value

On success 0 is returned, and [`ca_slot_info`](ca_data_types.html#c.ca_slot_info "ca_slot_info") is filled.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
