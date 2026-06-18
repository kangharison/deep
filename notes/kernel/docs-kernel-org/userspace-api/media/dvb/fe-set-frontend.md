# 6.1.2.5.FE_SET_FRONTEND

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-set-frontend.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.2.5. FE\_SET\_FRONTEND

Attention

This ioctl is deprecated.

## 6.1.2.5.1. Name

FE\_SET\_FRONTEND

## 6.1.2.5.2. Synopsis

FE\_SET\_FRONTEND

`int ioctl(int fd, FE_SET_FRONTEND, struct dvb_frontend_parameters *p)`

## 6.1.2.5.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`p`
:   Points to parameters for tuning operation.

## 6.1.2.5.4. Description

This ioctl call starts a tuning operation using specified parameters.
The result of this call will be successful if the parameters were valid
and the tuning could be initiated. The result of the tuning operation in
itself, however, will arrive asynchronously as an event (see
documentation for [FE\_GET\_EVENT](fe-get-event.html#fe-get-event) and
FrontendEvent.) If a new [FE\_SET\_FRONTEND](#fe-set-frontend)
operation is initiated before the previous one was completed, the
previous operation will be aborted in favor of the new one. This command
requires read/write access to the device.

## 6.1.2.5.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

|  |  |
| --- | --- |
| `EINVAL` | Maximum supported symbol rate reached. |

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
