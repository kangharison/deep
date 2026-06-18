# 6.1.2.8.FE_DISHNETWORK_SEND_LEGACY_CMD

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-dishnetwork-send-legacy-cmd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.2.8. FE\_DISHNETWORK\_SEND\_LEGACY\_CMD

## 6.1.2.8.1. Name

FE\_DISHNETWORK\_SEND\_LEGACY\_CMD

## 6.1.2.8.2. Synopsis

FE\_DISHNETWORK\_SEND\_LEGACY\_CMD

`int ioctl(int fd, FE_DISHNETWORK_SEND_LEGACY_CMD, unsigned long cmd)`

## 6.1.2.8.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`cmd`
:   Sends the specified raw cmd to the dish via DISEqC.

## 6.1.2.8.4. Description

Warning

This is a very obscure legacy command, used only at stv0299
driver. Should not be used on newer drivers.

It provides a non-standard method for selecting Diseqc voltage on the
frontend, for Dish Network legacy switches.

As support for this ioctl were added in 2004, this means that such
dishes were already legacy in 2004.

## 6.1.2.8.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
