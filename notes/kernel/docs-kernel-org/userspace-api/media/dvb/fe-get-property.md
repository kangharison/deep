# 2.4.5.ioctl FE_SET_PROPERTY, FE_GET_PROPERTY

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-get-property.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.5. ioctl FE\_SET\_PROPERTY, FE\_GET\_PROPERTY

## 2.4.5.1. Name

FE\_SET\_PROPERTY - FE\_GET\_PROPERTY - FE\_SET\_PROPERTY sets one or more frontend properties. - FE\_GET\_PROPERTY returns one or more frontend properties.

## 2.4.5.2. Synopsis

FE\_GET\_PROPERTY

`int ioctl(int fd, FE_GET_PROPERTY, struct dtv_properties *argp)`

FE\_SET\_PROPERTY

`int ioctl(int fd, FE_SET_PROPERTY, struct dtv_properties *argp)`

## 2.4.5.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`argp`
:   Pointer to struct [`dtv_properties`](frontend-header.html#c.dtv_properties "dtv_properties").

## 2.4.5.4. Description

All Digital TV frontend devices support the `FE_SET_PROPERTY` and
`FE_GET_PROPERTY` ioctls. The supported properties and statistics
depends on the delivery system and on the device:

* `FE_SET_PROPERTY:`

  + This ioctl is used to set one or more frontend properties.
  + This is the basic command to request the frontend to tune into
    some frequency and to start decoding the digital TV signal.
  + This call requires read/write access to the device.

Note

At return, the values aren’t updated to reflect the actual
parameters used. If the actual parameters are needed, an explicit
call to `FE_GET_PROPERTY` is needed.

* `FE_GET_PROPERTY:`

  + This ioctl is used to get properties and statistics from the
    frontend.
  + No properties are changed, and statistics aren’t reset.
  + This call only requires read-only access to the device.

## 2.4.5.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
