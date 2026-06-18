# 2.3.Property types

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dvbproperty.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3. Property types

Tuning into a Digital TV physical channel and starting decoding it
requires changing a set of parameters, in order to control the tuner,
the demodulator, the Linear Low-noise Amplifier (LNA) and to set the
antenna subsystem via Satellite Equipment Control - SEC (on satellite
systems). The actual parameters are specific to each particular digital
TV standards, and may change as the digital TV specs evolves.

In the past (up to DVB API version 3 - DVBv3), the strategy used was to have a
`union with` the parameters needed to tune for DVB-S, DVB-C, DVB-T and
ATSC delivery systems grouped there. The problem is that, as the second
generation standards appeared, the size of such `union was` not big
enough to group the structs that would be required for those new
standards. Also, extending it would break userspace.

So, the legacy union/`struct based` approach was deprecated, in favor
of a properties set approach. On such approach,
[FE\_GET\_PROPERTY and FE\_SET\_PROPERTY](fe-get-property.html#fe-get-property) are used
to setup the frontend and read its status.

The actual action is determined by a set of dtv\_property cmd/data pairs.
With one single ioctl, is possible to get/set up to 64 properties.

This section describes the new and recommended way to set the frontend,
with supports all digital TV delivery systems.

Note

1. On Linux DVB API version 3, setting a frontend was done via
   struct [`dvb_frontend_parameters`](dvb-frontend-parameters.html#c.dvb_frontend_parameters "dvb_frontend_parameters").
2. Don’t use DVB API version 3 calls on hardware with supports
   newer standards. Such API provides no support or a very limited
   support to new standards and/or new hardware.
3. Nowadays, most frontends support multiple delivery systems.
   Only with DVB API version 5 calls it is possible to switch between
   the multiple delivery systems supported by a frontend.
4. DVB API version 5 is also called *S2API*, as the first
   new standard added to it was DVB-S2.

**Example**: in order to set the hardware to tune into a DVB-C channel
at 651 kHz, modulated with 256-QAM, FEC 3/4 and symbol rate of 5.217
Mbauds, those properties should be sent to
[FE\_SET\_PROPERTY](fe-get-property.html#fe-get-property) ioctl:

> [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system) = SYS\_DVBC\_ANNEX\_A
>
> [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency) = 651000000
>
> [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation) = QAM\_256
>
> [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion) = INVERSION\_AUTO
>
> [DTV\_SYMBOL\_RATE](fe_property_parameters.html#dtv-symbol-rate) = 5217000
>
> [DTV\_INNER\_FEC](fe_property_parameters.html#dtv-inner-fec) = FEC\_3\_4
>
> [DTV\_TUNE](fe_property_parameters.html#dtv-tune)

The code that would that would do the above is show in
[Example: Setting digital TV frontend properties](#dtv-prop-example).

Example: Setting digital TV frontend properties

```
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/dvb/frontend.h>

static struct dtv_property props[] = {
    { .cmd = DTV_DELIVERY_SYSTEM, .u.data = SYS_DVBC_ANNEX_A },
    { .cmd = DTV_FREQUENCY,       .u.data = 651000000 },
    { .cmd = DTV_MODULATION,      .u.data = QAM_256 },
    { .cmd = DTV_INVERSION,       .u.data = INVERSION_AUTO },
    { .cmd = DTV_SYMBOL_RATE,     .u.data = 5217000 },
    { .cmd = DTV_INNER_FEC,       .u.data = FEC_3_4 },
    { .cmd = DTV_TUNE }
};

static struct dtv_properties dtv_prop = {
    .num = 6, .props = props
};

int main(void)
{
    int fd = open("/dev/dvb/adapter0/frontend0", O_RDWR);

    if (!fd) {
        perror ("open");
        return -1;
    }
    if (ioctl(fd, FE_SET_PROPERTY, &dtv_prop) == -1) {
        perror("ioctl");
        return -1;
    }
    printf("Frontend set\\n");
    return 0;
}
```

Attention

While it is possible to directly call the Kernel code like the
above example, it is strongly recommended to use
[libdvbv5](https://linuxtv.org/docs/libdvbv5/index.html), as it
provides abstraction to work with the supported digital TV standards and
provides methods for usual operations like program scanning and to
read/write channel descriptor files.

* [2.3.1. Digital TV property parameters](fe_property_parameters.html)
* [2.3.2. Frontend statistics indicators](frontend-stat-properties.html)
* [2.3.3. Properties used on terrestrial delivery systems](frontend-property-terrestrial-systems.html)
* [2.3.4. Properties used on cable delivery systems](frontend-property-cable-systems.html)
* [2.3.5. Properties used on satellite delivery systems](frontend-property-satellite-systems.html)
* [2.3.6. Frontend uAPI data types](frontend-header.html)
