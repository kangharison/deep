# 4.Digital TV (DVB) devices

> 출처(원문): https://docs.kernel.org/driver-api/media/dtv-core.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4. Digital TV (DVB) devices

Digital TV devices are implemented by several different drivers:

* A bridge driver that is responsible to talk with the bus where the other
  devices are connected (PCI, USB, SPI), bind to the other drivers and
  implement the digital demux logic (either in software or in hardware);
* Frontend drivers that are usually implemented as two separate drivers:

  + A tuner driver that implements the logic which commands the part of
    the hardware responsible for tuning into a digital TV transponder or
    physical channel. The output of a tuner is usually a baseband or
    Intermediate Frequency (IF) signal;
  + A demodulator driver (a.k.a “demod”) that implements the logic which
    commands the digital TV decoding hardware. The output of a demod is
    a digital stream, with multiple audio, video and data channels typically
    multiplexed using MPEG Transport Stream [[1]](#f1).

On most hardware, the frontend drivers talk with the bridge driver using an
I2C bus.

[[1](#id1)]

Some standards use TCP/IP for multiplexing data, like DVB-H (an
abandoned standard, not used anymore) and ATSC version 3.0 current
proposals. Currently, the DVB subsystem doesn’t implement those standards.

* [4.1. Digital TV Common functions](dtv-common.html)
* [4.2. Digital TV Frontend kABI](dtv-frontend.html)
* [4.3. Digital TV Demux kABI](dtv-demux.html)
* [4.4. Digital TV Conditional Access kABI](dtv-ca.html)
* [4.5. Digital TV Network kABI](dtv-net.html)
