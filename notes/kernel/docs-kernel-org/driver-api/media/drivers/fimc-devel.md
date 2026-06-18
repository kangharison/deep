# 10.1.4.The Samsung S5P/EXYNOS4 FIMC driver

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/fimc-devel.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.1.4. The Samsung S5P/EXYNOS4 FIMC driver

Copyright © 2012 - 2013 Samsung Electronics Co., Ltd.

## 10.1.4.1. Files partitioning

* media device driver

  drivers/media/platform/samsung/exynos4-is/media-dev.[ch]
* camera capture video device driver

  drivers/media/platform/samsung/exynos4-is/fimc-capture.c
* MIPI-CSI2 receiver subdev

  drivers/media/platform/samsung/exynos4-is/mipi-csis.[ch]
* video post-processor (mem-to-mem)

  drivers/media/platform/samsung/exynos4-is/fimc-core.c
* common files

  drivers/media/platform/samsung/exynos4-is/fimc-core.h
  drivers/media/platform/samsung/exynos4-is/fimc-reg.h
  drivers/media/platform/samsung/exynos4-is/regs-fimc.h
