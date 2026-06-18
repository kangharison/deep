# 2.1.Querying frontend information

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/query-dvb-frontend-info.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.1. Querying frontend information

Usually, the first thing to do when the frontend is opened is to check
the frontend capabilities. This is done using
[ioctl FE\_GET\_INFO](fe-get-info.html#fe-get-info). This ioctl will enumerate the
Digital TV API version and other characteristics about the frontend, and can
be opened either in read only or read/write mode.
