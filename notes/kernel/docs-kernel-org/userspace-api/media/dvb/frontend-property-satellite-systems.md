# 2.3.5.Properties used on satellite delivery systems

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/frontend-property-satellite-systems.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3.5. Properties used on satellite delivery systems

## 2.3.5.1. DVB-S delivery system

The following parameters are valid for DVB-S:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_SYMBOL\_RATE](fe_property_parameters.html#dtv-symbol-rate)
* [DTV\_INNER\_FEC](fe_property_parameters.html#dtv-inner-fec)
* [DTV\_VOLTAGE](fe_property_parameters.html#dtv-voltage)
* [DTV\_TONE](fe_property_parameters.html#dtv-tone)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

Future implementations might add those two missing parameters:

* [DTV\_DISEQC\_MASTER](fe_property_parameters.html#dtv-diseqc-master)
* [DTV\_DISEQC\_SLAVE\_REPLY](fe_property_parameters.html#dtv-diseqc-slave-reply)

## 2.3.5.2. DVB-S2 delivery system

In addition to all parameters valid for DVB-S, DVB-S2 supports the
following parameters:

* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)
* [DTV\_PILOT](fe_property_parameters.html#dtv-pilot)
* [DTV\_ROLLOFF](fe_property_parameters.html#dtv-rolloff)
* [DTV\_STREAM\_ID](fe_property_parameters.html#dtv-stream-id)
* [DTV\_SCRAMBLING\_SEQUENCE\_INDEX](fe_property_parameters.html#dtv-scrambling-sequence-index)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

## 2.3.5.3. Turbo code delivery system

In addition to all parameters valid for DVB-S, turbo code supports the
following parameters:

* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)

## 2.3.5.4. ISDB-S delivery system

The following parameters are valid for ISDB-S:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_SYMBOL\_RATE](fe_property_parameters.html#dtv-symbol-rate)
* [DTV\_INNER\_FEC](fe_property_parameters.html#dtv-inner-fec)
* [DTV\_VOLTAGE](fe_property_parameters.html#dtv-voltage)
* [DTV\_STREAM\_ID](fe_property_parameters.html#dtv-stream-id)
