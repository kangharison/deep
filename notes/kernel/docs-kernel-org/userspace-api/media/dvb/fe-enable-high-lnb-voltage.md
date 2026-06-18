# 2.4.12.ioctl FE_ENABLE_HIGH_LNB_VOLTAGE

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-enable-high-lnb-voltage.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.12. ioctl FE\_ENABLE\_HIGH\_LNB\_VOLTAGE

## 2.4.12.1. Name

FE\_ENABLE\_HIGH\_LNB\_VOLTAGE - Select output DC level between normal LNBf voltages or higher LNBf - voltages.

## 2.4.12.2. Synopsis

FE\_ENABLE\_HIGH\_LNB\_VOLTAGE

`int ioctl(int fd, FE_ENABLE_HIGH_LNB_VOLTAGE, unsigned int high)`

## 2.4.12.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`high`
:   Valid flags:

    * 0 - normal 13V and 18V.
    * >0 - enables slightly higher voltages instead of 13/18V, in order
      to compensate for long antenna cables.

## 2.4.12.4. Description

Select output DC level between normal LNBf voltages or higher LNBf
voltages between 0 (normal) or a value grater than 0 for higher
voltages.

## 2.4.12.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
