# 8.1.Common selection definitions

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/selections-common.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 8.1. Common selection definitions

While the [V4L2 selection API](selection-api.html#selection-api) and
[V4L2 subdev selection APIs](dev-subdev.html#v4l2-subdev-selections) are very
similar, there’s one fundamental difference between the two. On
sub-device API, the selection rectangle refers to the media bus format,
and is bound to a sub-device’s pad. On the V4L2 interface the selection
rectangles refer to the in-memory pixel format.

This section defines the common definitions of the selection interfaces
on the two APIs.

* [8.1.1. Selection targets](v4l2-selection-targets.html)
* [8.1.2. Selection flags](v4l2-selection-flags.html)
