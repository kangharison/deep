# 2.13.7.V4L2_META_FMT_RPI_FE_CFG

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-pisp-fe.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.7. V4L2\_META\_FMT\_RPI\_FE\_CFG

## 2.13.7.1. Raspberry Pi PiSP Front End configuration format

The Raspberry Pi PiSP Front End image signal processor is configured by
userspace by providing a buffer of configuration parameters to the
rp1-cfe-fe-config output video device node using the
[`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") interface.

The [Raspberry Pi PiSP technical specification](https://datasheets.raspberrypi.com/camera/raspberry-pi-image-signal-processor-specification.pdf)
provide detailed description of the Front End configuration and programming
model.

# 2.13.8. V4L2\_META\_FMT\_RPI\_FE\_STATS

## 2.13.8.1. Raspberry Pi PiSP Front End statistics format

The Raspberry Pi PiSP Front End image signal processor provides statistics data
by writing to a buffer provided via the rp1-cfe-fe-stats capture video device
node using the
[`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") interface.

The [Raspberry Pi PiSP technical specification](https://datasheets.raspberrypi.com/camera/raspberry-pi-image-signal-processor-specification.pdf)
provide detailed description of the Front End configuration and programming
model.
