# 2.4.Indexed Format

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-indexed.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4. Indexed Format

In this format each pixel is represented by an 8 bit index into a 256
entry ARGB palette. It is intended for
[Video Output Overlays](dev-osd.html#osd) only. There are no ioctls to access
the palette, this must be done with ioctls of the Linux framebuffer API.

Indexed Image Format

| Identifier | Code |  | Byte 0 | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| `V4L2_PIX_FMT_PAL8` | ‘PAL8’ |  | i7 | i6 | i5 | i4 | i3 | i2 | i1 | i0 |
