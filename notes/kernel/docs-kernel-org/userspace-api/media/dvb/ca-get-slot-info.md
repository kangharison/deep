# 4.2.5.CA_GET_SLOT_INFO

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca-get-slot-info.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.2.5. CA\_GET\_SLOT\_INFO

## 4.2.5.1. Name

CA\_GET\_SLOT\_INFO

## 4.2.5.2. Synopsis

CA\_GET\_SLOT\_INFO

`int ioctl(fd, CA_GET_SLOT_INFO, struct ca_slot_info *info)`

## 4.2.5.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](ca-fopen.html#c.DTV.ca.open "open").

`info`
:   Pointer to struct [`ca_slot_info`](ca_data_types.html#c.ca_slot_info "ca_slot_info").

## 4.2.5.4. Description

Returns information about a CA slot identified by
[`ca_slot_info`](ca_data_types.html#c.ca_slot_info "ca_slot_info").slot\_num.

## 4.2.5.5. Return Value

On success 0 is returned, and [`ca_slot_info`](ca_data_types.html#c.ca_slot_info "ca_slot_info") is filled.

On error -1 is returned, and the `errno` variable is set
appropriately.

|  |  |
| --- | --- |
| `ENODEV` | the slot is not available. |

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
