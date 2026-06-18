# 2.4.13.ioctl FE_SET_FRONTEND_TUNE_MODE

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-set-frontend-tune-mode.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.13. ioctl FE\_SET\_FRONTEND\_TUNE\_MODE

## 2.4.13.1. Name

FE\_SET\_FRONTEND\_TUNE\_MODE - Allow setting tuner mode flags to the frontend.

## 2.4.13.2. Synopsis

FE\_SET\_FRONTEND\_TUNE\_MODE

`int ioctl(int fd, FE_SET_FRONTEND_TUNE_MODE, unsigned int flags)`

## 2.4.13.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`flags`
:   Valid flags:

    * 0 - normal tune mode
    * `FE_TUNE_MODE_ONESHOT` - When set, this flag will disable any
      zigzagging or other “normal” tuning behaviour. Additionally,
      there will be no automatic monitoring of the lock status, and
      hence no frontend events will be generated. If a frontend device
      is closed, this flag will be automatically turned off when the
      device is reopened read-write.

## 2.4.13.4. Description

Allow setting tuner mode flags to the frontend, between 0 (normal) or
`FE_TUNE_MODE_ONESHOT` mode

## 2.4.13.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
