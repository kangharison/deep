# 3.Digital TV Demux Device

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/demux.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3. Digital TV Demux Device

The Digital TV demux device controls the MPEG-TS filters for the
digital TV. If the driver and hardware supports, those filters are
implemented at the hardware. Otherwise, the Kernel provides a software
emulation.

It can be accessed through `/dev/adapter?/demux?`. Data types and
ioctl definitions can be accessed by including `linux/dvb/dmx.h` in
your application.

* [3.1. Demux Data Types](dmx_types.html)
* [3.2. Demux Function Calls](dmx_fcalls.html)
