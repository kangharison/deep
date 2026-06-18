# Introduction

> 출처(원문): https://docs.kernel.org/driver-api/iio/intro.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Introduction

The main purpose of the Industrial I/O subsystem (IIO) is to provide support
for devices that in some sense perform either
analog-to-digital conversion (ADC) or digital-to-analog conversion (DAC)
or both. The aim is to fill the gap between the somewhat similar hwmon and
[input](../input.html) subsystems. Hwmon is directed at low sample rate
sensors used to monitor and control the system itself, like fan speed control
or temperature measurement. [Input](../input.html) is, as its name suggests,
focused on human interaction input devices (keyboard, mouse, touchscreen).
In some cases there is considerable overlap between these and IIO.

Devices that fall into this category include:

* analog to digital converters (ADCs)
* accelerometers
* capacitance to digital converters (CDCs)
* digital to analog converters (DACs)
* gyroscopes
* inertial measurement units (IMUs)
* color and light sensors
* magnetometers
* pressure sensors
* proximity sensors
* temperature sensors

Usually these sensors are connected via [SPI](../spi.html) or
[I2C](../i2c.html). A common use case of the sensors devices is to have
combined functionality (e.g. light plus proximity sensor).
