# 2.7.ioctls CEC_ADAP_G_PHYS_ADDR and CEC_ADAP_S_PHYS_ADDR

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-ioc-adap-g-phys-addr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7. ioctls CEC\_ADAP\_G\_PHYS\_ADDR and CEC\_ADAP\_S\_PHYS\_ADDR

## 2.7.1. Name

CEC\_ADAP\_G\_PHYS\_ADDR, CEC\_ADAP\_S\_PHYS\_ADDR - Get or set the physical address

## 2.7.2. Synopsis

CEC\_ADAP\_G\_PHYS\_ADDR

`int ioctl(int fd, CEC_ADAP_G_PHYS_ADDR, __u16 *argp)`

CEC\_ADAP\_S\_PHYS\_ADDR

`int ioctl(int fd, CEC_ADAP_S_PHYS_ADDR, __u16 *argp)`

## 2.7.3. Arguments

`fd`
:   File descriptor returned by [`open()`](cec-func-open.html#c.CEC.open "open").

`argp`
:   Pointer to the CEC address.

## 2.7.4. Description

To query the current physical address applications call
[ioctl CEC\_ADAP\_G\_PHYS\_ADDR](#cec-adap-g-phys-addr) with a pointer to a \_\_u16 where the
driver stores the physical address.

To set a new physical address applications store the physical address in
a \_\_u16 and call [ioctl CEC\_ADAP\_S\_PHYS\_ADDR](#cec-adap-s-phys-addr) with a pointer to
this integer. The [ioctl CEC\_ADAP\_S\_PHYS\_ADDR](#cec-adap-s-phys-addr) is only available if
`CEC_CAP_PHYS_ADDR` is set (the `ENOTTY` error code will be returned
otherwise). The [ioctl CEC\_ADAP\_S\_PHYS\_ADDR](#cec-adap-s-phys-addr) can only be called
by a file descriptor in initiator mode (see [ioctls CEC\_G\_MODE and CEC\_S\_MODE](cec-ioc-g-mode.html#cec-s-mode)), if not
the `EBUSY` error code will be returned.

To clear an existing physical address use `CEC_PHYS_ADDR_INVALID`.
The adapter will go to the unconfigured state.

If logical address types have been defined (see [ioctl CEC\_ADAP\_S\_LOG\_ADDRS](cec-ioc-adap-g-log-addrs.html#cec-adap-s-log-addrs)),
then this ioctl will block until all
requested logical addresses have been claimed. If the file descriptor is in non-blocking mode
then it will not wait for the logical addresses to be claimed, instead it just returns 0.

A [CEC\_EVENT\_STATE\_CHANGE](cec-ioc-dqevent.html#cec-event-state-change) event is sent when the physical address
changes.

The physical address is a 16-bit number where each group of 4 bits
represent a digit of the physical address a.b.c.d where the most
significant 4 bits represent ‘a’. The CEC root device (usually the TV)
has address 0.0.0.0. Every device that is hooked up to an input of the
TV has address a.0.0.0 (where ‘a’ is ≥ 1), devices hooked up to those in
turn have addresses a.b.0.0, etc. So a topology of up to 5 devices deep
is supported. The physical address a device shall use is stored in the
EDID of the sink.

For example, the EDID for each HDMI input of the TV will have a
different physical address of the form a.0.0.0 that the sources will
read out and use as their physical address.

## 2.7.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

The [ioctl CEC\_ADAP\_S\_PHYS\_ADDR](#cec-adap-s-phys-addr) can return the following
error codes:

ENOTTY
:   The `CEC_CAP_PHYS_ADDR` capability wasn’t set, so this ioctl is not supported.

EBUSY
:   Another filehandle is in exclusive follower or initiator mode, or the filehandle
    is in mode `CEC_MODE_NO_INITIATOR`.

EINVAL
:   The physical address is malformed.
