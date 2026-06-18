# GPIO_GET_CHIPINFO_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-get-chipinfo-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_GET\_CHIPINFO\_IOCTL

## Name

GPIO\_GET\_CHIPINFO\_IOCTL - Get the publicly available information for a chip.

## Synopsis

GPIO\_GET\_CHIPINFO\_IOCTL

`int ioctl(int chip_fd, GPIO_GET_CHIPINFO_IOCTL, struct gpiochip_info *info)`

## Arguments

`chip_fd`
:   The file descriptor of the GPIO character device returned by open().

`info`
:   The [`chip_info`](chardev.html#c.gpiochip_info "gpiochip_info") to be populated.

## Description

Gets the publicly available information for a particular GPIO chip.

## Return Value

On success 0 and `info` is populated with the chip info.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
