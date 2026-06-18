# 5.1.2.ioctl NET_ADD_IF

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/net-add-if.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.1.2. ioctl NET\_ADD\_IF

## 5.1.2.1. Name

NET\_ADD\_IF - Creates a new network interface for a given Packet ID.

## 5.1.2.2. Synopsis

NET\_ADD\_IF

`int ioctl(int fd, NET_ADD_IF, struct dvb_net_if *net_if)`

## 5.1.2.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`net_if`
:   pointer to struct [`dvb_net_if`](net-types.html#c.dvb_net_if "dvb_net_if")

## 5.1.2.4. Description

The NET\_ADD\_IF ioctl system call selects the Packet ID (PID) that
contains a TCP/IP traffic, the type of encapsulation to be used (MPE or
ULE) and the interface number for the new interface to be created. When
the system call successfully returns, a new virtual network interface is
created.

The struct [`dvb_net_if`](net-types.html#c.dvb_net_if "dvb_net_if")::ifnum field will be
filled with the number of the created interface.

## 5.1.2.5. Return Value

On success 0 is returned, and [`ca_slot_info`](ca_data_types.html#c.ca_slot_info "ca_slot_info") is filled.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
