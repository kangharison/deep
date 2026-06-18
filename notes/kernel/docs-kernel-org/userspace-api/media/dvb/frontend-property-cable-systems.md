# 2.3.4.Properties used on cable delivery systems

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/frontend-property-cable-systems.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3.4. Properties used on cable delivery systems

## 2.3.4.1. DVB-C delivery system

The DVB-C Annex-A is the widely used cable standard. Transmission uses
QAM modulation.

The DVB-C Annex-C is optimized for 6MHz, and is used in Japan. It
supports a subset of the Annex A modulation types, and a roll-off of
0.13, instead of 0.15

The following parameters are valid for DVB-C Annex A/C:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_SYMBOL\_RATE](fe_property_parameters.html#dtv-symbol-rate)
* [DTV\_INNER\_FEC](fe_property_parameters.html#dtv-inner-fec)
* [DTV\_LNA](fe_property_parameters.html#dtv-lna)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

## 2.3.4.2. DVB-C Annex B delivery system

The DVB-C Annex-B is only used on a few Countries like the United
States.

The following parameters are valid for DVB-C Annex B:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_LNA](fe_property_parameters.html#dtv-lna)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.
