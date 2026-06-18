# 7.36.ioctl VIDIOC_G_OUTPUT, VIDIOC_S_OUTPUT

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-output.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.36. ioctl VIDIOC\_G\_OUTPUT, VIDIOC\_S\_OUTPUT

## 7.36.1. Name

VIDIOC\_G\_OUTPUT - VIDIOC\_S\_OUTPUT - Query or select the current video output

## 7.36.2. Synopsis

VIDIOC\_G\_OUTPUT

`int ioctl(int fd, VIDIOC_G_OUTPUT, int *argp)`

VIDIOC\_S\_OUTPUT

`int ioctl(int fd, VIDIOC_S_OUTPUT, int *argp)`

## 7.36.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to an integer with output index.

## 7.36.4. Description

To query the current video output applications call the
[VIDIOC\_G\_OUTPUT](#vidioc-g-output) ioctl with a pointer to an integer where the driver
stores the number of the output, as in the struct
[`v4l2_output`](vidioc-enumoutput.html#c.V4L.v4l2_output "v4l2_output") `index` field. This ioctl will
fail only when there are no video outputs, returning the `EINVAL` error
code.

To select a video output applications store the number of the desired
output in an integer and call the [VIDIOC\_S\_OUTPUT](#vidioc-g-output) ioctl with a
pointer to this integer. Side effects are possible. For example outputs
may support different video standards, so the driver may implicitly
switch the current standard. Because of these possible side
effects applications must select an output before querying or
negotiating any other parameters.

Information about video outputs is available using the
[ioctl VIDIOC\_ENUMOUTPUT](vidioc-enumoutput.html#vidioc-enumoutput) ioctl.

## 7.36.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The number of the video output is out of bounds, or there are no
    video outputs at all.
