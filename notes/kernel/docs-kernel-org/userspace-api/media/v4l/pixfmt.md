# 2.Image Formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. Image Formats

The V4L2 API was primarily designed for devices exchanging image data
with applications. The struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") and
struct [`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane") structures define the
format and layout of an image in memory. The former is used with the
single-planar API, while the latter is used with the multi-planar
version (see [Single- and multi-planar APIs](planar-apis.html#planar-apis)). Image formats are negotiated with
the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl. (The explanations here
focus on video capturing and output, for overlay frame buffer formats
see also [VIDIOC\_G\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf).)

* [2.1. Single-planar format structure](pixfmt-v4l2.html)
* [2.2. Multi-planar format structures](pixfmt-v4l2-mplane.html)
* [2.3. Standard Image Formats](pixfmt-intro.html)
* [2.4. Indexed Format](pixfmt-indexed.html)
* [2.5. RGB Formats](pixfmt-rgb.html)
* [2.6. Raw Bayer Formats](pixfmt-bayer.html)
* [2.7. YUV Formats](yuv-formats.html)
* [2.8. HSV Formats](hsv-formats.html)
* [2.9. Depth Formats](depth-formats.html)
* [2.10. Compressed Formats](pixfmt-compressed.html)
* [2.11. SDR Formats](sdr-formats.html)
* [2.12. Touch Formats](tch-formats.html)
* [2.13. Metadata Formats](meta-formats.html)
* [2.14. Reserved Format Identifiers](pixfmt-reserved.html)
* [2.15. Colorspaces](colorspaces.html)
* [2.16. Defining Colorspaces in V4L2](colorspaces-defs.html)
* [2.17. Detailed Colorspace Descriptions](colorspaces-details.html)
* [2.18. Detailed Transfer Function Descriptions](colorspaces-details.html#detailed-transfer-function-descriptions)
