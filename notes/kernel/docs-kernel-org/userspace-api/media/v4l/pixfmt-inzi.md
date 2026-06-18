# 2.9.1.V4L2_PIX_FMT_INZI (‘INZI’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-inzi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.9.1. V4L2\_PIX\_FMT\_INZI (‘INZI’)

Infrared 10-bit linked with Depth 16-bit images

## 2.9.1.1. Description

Proprietary multi-planar format used by Intel SR300 Depth cameras, comprise of
Infrared image followed by Depth data. The pixel definition is 32-bpp,
with the Depth and Infrared Data split into separate continuous planes of
identical dimensions.

The first plane - Infrared data - is stored according to
[V4L2\_PIX\_FMT\_Y10](pixfmt-yuv-luma.html#v4l2-pix-fmt-y10) greyscale format.
Each pixel is 16-bit cell, with actual data stored in the 10 LSBs
with values in range 0 to 1023.
The six remaining MSBs are padded with zeros.

The second plane provides 16-bit per-pixel Depth data arranged in
[V4L2-PIX-FMT-Z16](pixfmt-z16.html#v4l2-pix-fmt-z16) format.

**Frame Structure.**
Each cell is a 16-bit word with more significant data stored at higher
memory address (byte order is little-endian).

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| Ir0,0 | Ir0,1 | Ir0,2 | ... | ... | ... |
| ... | | | | | |
| Infrared Data | | | | | |
| ... | | | | | |
| ... | ... | ... | Irn-1,n-3 | Irn-1,n-2 | Irn-1,n-1 |
| Depth0,0 | Depth0,1 | Depth0,2 | ... | ... | ... |
| ... | | | | | |
| Depth Data | | | | | |
| ... | | | | | |
| ... | ... | ... | Depthn-1,n-3 | Depthn-1,n-2 | Depthn-1,n-1 |
