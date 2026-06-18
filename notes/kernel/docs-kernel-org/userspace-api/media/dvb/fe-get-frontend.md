# 6.1.2.6.FE_GET_FRONTEND

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-get-frontend.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.2.6. FE\_GET\_FRONTEND

## 6.1.2.6.1. Name

FE\_GET\_FRONTEND

Attention

This ioctl is deprecated.

## 6.1.2.6.2. Synopsis

FE\_GET\_FRONTEND

`int ioctl(int fd, FE_GET_FRONTEND, struct dvb_frontend_parameters *p)`

## 6.1.2.6.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`p`
:   Points to parameters for tuning operation.

## 6.1.2.6.4. Description

This ioctl call queries the currently effective frontend parameters. For
this command, read-only access to the device is sufficient.

## 6.1.2.6.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

|  |  |
| --- | --- |
| `EINVAL` | Maximum supported symbol rate reached. |

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
