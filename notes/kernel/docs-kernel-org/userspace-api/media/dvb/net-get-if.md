# 5.1.4.ioctl NET_GET_IF

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/net-get-if.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.1.4. ioctl NET\_GET\_IF

## 5.1.4.1. Name

NET\_GET\_IF - Read the configuration data of an interface created via - [NET\_ADD\_IF](net.html#net).

## 5.1.4.2. Synopsis

NET\_GET\_IF

`int ioctl(int fd, NET_GET_IF, struct dvb_net_if *net_if)`

## 5.1.4.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`net_if`
:   pointer to struct [`dvb_net_if`](net-types.html#c.dvb_net_if "dvb_net_if")

## 5.1.4.4. Description

The NET\_GET\_IF ioctl uses the interface number given by the struct
[`dvb_net_if`](net-types.html#c.dvb_net_if "dvb_net_if")::ifnum field and fills the content of
struct [`dvb_net_if`](net-types.html#c.dvb_net_if "dvb_net_if") with the packet ID and
encapsulation type used on such interface. If the interface was not
created yet with [NET\_ADD\_IF](net.html#net), it will return -1 and fill
the `errno` with `EINVAL` error code.

## 5.1.4.5. Return Value

On success 0 is returned, and [`ca_slot_info`](ca_data_types.html#c.ca_slot_info "ca_slot_info") is filled.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
