# 3.2.9.DMX_SET_FILTER

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-set-filter.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.9. DMX\_SET\_FILTER

## 3.2.9.1. Name

DMX\_SET\_FILTER

## 3.2.9.2. Synopsis

DMX\_SET\_FILTER

`int ioctl(int fd, DMX_SET_FILTER, struct dmx_sct_filter_params *params)`

## 3.2.9.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`params`

> Pointer to structure containing filter parameters.

## 3.2.9.4. Description

This ioctl call sets up a filter according to the filter and mask
parameters provided. A timeout may be defined stating number of seconds
to wait for a section to be loaded. A value of 0 means that no timeout
should be applied. Finally there is a flag field where it is possible to
state whether a section should be CRC-checked, whether the filter should
be a “one-shot” filter, i.e. if the filtering operation should be
stopped after the first section is received, and whether the filtering
operation should be started immediately (without waiting for a
[DMX\_START](dmx-start.html#dmx-start) ioctl call). If a filter was previously set-up, this
filter will be canceled, and the receive buffer will be flushed.

## 3.2.9.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
