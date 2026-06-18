# IIO Interfacing Tools

> 출처(원문): https://docs.kernel.org/iio/iio_tools.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# IIO Interfacing Tools

## 1. Linux Kernel Tools

Linux Kernel provides some userspace tools that can be used to retrieve data
from IIO sysfs:

* lsiio: example application that provides a list of IIO devices and triggers
* iio\_event\_monitor: example application that reads events from an IIO device
  and prints them
* iio\_generic\_buffer: example application that reads data from buffer
* iio\_utils: set of APIs, typically used to access sysfs files.

## 2. LibIIO

LibIIO is a C/C++ library that provides generic access to IIO devices. The
library abstracts the low-level details of the hardware, and provides a simple
yet complete programming interface that can be used for advanced projects.

For more information about LibIIO, please see:
<https://github.com/analogdevicesinc/libiio>
