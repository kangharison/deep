# 2.4.3.ioctl FE_GET_INFO

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-get-info.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.3. ioctl FE\_GET\_INFO

## 2.4.3.1. Name

FE\_GET\_INFO - Query Digital TV frontend capabilities and returns information
about the - front-end. This call only requires read-only access to the device.

## 2.4.3.2. Synopsis

FE\_GET\_INFO

`int ioctl(int fd, FE_GET_INFO, struct dvb_frontend_info *argp)`

## 2.4.3.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`argp`
:   pointer to struct [`dvb_frontend_info`](frontend-header.html#c.dvb_frontend_info "dvb_frontend_info")

## 2.4.3.4. Description

All Digital TV frontend devices support the [ioctl FE\_GET\_INFO](#fe-get-info) ioctl. It is
used to identify kernel devices compatible with this specification and to
obtain information about driver and hardware capabilities. The ioctl
takes a pointer to dvb\_frontend\_info which is filled by the driver.
When the driver is not compatible with this specification the ioctl
returns an error.

## 2.4.3.5. frontend capabilities

Capabilities describe what a frontend can do. Some capabilities are
supported only on some specific frontend types.

The frontend capabilities are described at [`fe_caps`](frontend-header.html#c.fe_caps "fe_caps").

## 2.4.3.6. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
