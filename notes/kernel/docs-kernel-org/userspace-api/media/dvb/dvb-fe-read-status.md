# 2.2.Querying frontend status and statistics

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dvb-fe-read-status.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.2. Querying frontend status and statistics

Once [FE\_SET\_PROPERTY](fe-get-property.html#fe-get-property) is called, the
frontend will run a kernel thread that will periodically check for the
tuner lock status and provide statistics about the quality of the
signal.

The information about the frontend tuner locking status can be queried
using [ioctl FE\_READ\_STATUS](fe-read-status.html#fe-read-status).

Signal statistics are provided via
[ioctl FE\_SET\_PROPERTY, FE\_GET\_PROPERTY](fe-get-property.html#fe-get-property).

Note

Most statistics require the demodulator to be fully locked
(e. g. with [`FE_HAS_LOCK`](frontend-header.html#c.fe_status "fe_status") bit set). See
[Frontend statistics indicators](frontend-stat-properties.html#frontend-stat-properties) for
more details.
