# 4.2.6.CA_GET_DESCR_INFO

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-get-descr-info.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.6. CA\_GET\_DESCR\_INFO

## 4.2.6.1. Name

CA\_GET\_DESCR\_INFO

## 4.2.6.2. Synopsis

CA\_GET\_DESCR\_INFO

`int ioctl(fd, CA_GET_DESCR_INFO, struct ca_descr_info *desc)`

## 4.2.6.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](ca-fopen.html#c.DTV.ca.open "open").

`desc`
:   Pointer to struct [`ca_descr_info`](ca_data_types.html#c.ca_descr_info "ca_descr_info").

## 4.2.6.4. Description

Returns information about all descrambler slots.

## 4.2.6.5. Return Value

On success 0 is returned, and [`ca_descr_info`](ca_data_types.html#c.ca_descr_info "ca_descr_info") is filled.

On error -1 is returned, and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
