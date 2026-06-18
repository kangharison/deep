# 2.3.3.Properties used on terrestrial delivery systems

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/frontend-property-terrestrial-systems.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3.3. Properties used on terrestrial delivery systems

## 2.3.3.1. DVB-T delivery system

The following parameters are valid for DVB-T:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)
* [DTV\_BANDWIDTH\_HZ](fe_property_parameters.html#dtv-bandwidth-hz)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_CODE\_RATE\_HP](fe_property_parameters.html#dtv-code-rate-hp)
* [DTV\_CODE\_RATE\_LP](fe_property_parameters.html#dtv-code-rate-lp)
* [DTV\_GUARD\_INTERVAL](fe_property_parameters.html#dtv-guard-interval)
* [DTV\_TRANSMISSION\_MODE](fe_property_parameters.html#dtv-transmission-mode)
* [DTV\_HIERARCHY](fe_property_parameters.html#dtv-hierarchy)
* [DTV\_LNA](fe_property_parameters.html#dtv-lna)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

## 2.3.3.2. DVB-T2 delivery system

DVB-T2 support is currently in the early stages of development, so
expect that this section may grow and become more detailed with time.

The following parameters are valid for DVB-T2:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)
* [DTV\_BANDWIDTH\_HZ](fe_property_parameters.html#dtv-bandwidth-hz)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_CODE\_RATE\_HP](fe_property_parameters.html#dtv-code-rate-hp)
* [DTV\_CODE\_RATE\_LP](fe_property_parameters.html#dtv-code-rate-lp)
* [DTV\_GUARD\_INTERVAL](fe_property_parameters.html#dtv-guard-interval)
* [DTV\_TRANSMISSION\_MODE](fe_property_parameters.html#dtv-transmission-mode)
* [DTV\_HIERARCHY](fe_property_parameters.html#dtv-hierarchy)
* [DTV\_STREAM\_ID](fe_property_parameters.html#dtv-stream-id)
* [DTV\_LNA](fe_property_parameters.html#dtv-lna)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

## 2.3.3.3. ISDB-T delivery system

This ISDB-T/ISDB-Tsb API extension should reflect all information needed
to tune any ISDB-T/ISDB-Tsb hardware. Of course it is possible that some
very sophisticated devices won’t need certain parameters to tune.

The information given here should help application writers to know how
to handle ISDB-T and ISDB-Tsb hardware using the Linux Digital TV API.

The details given here about ISDB-T and ISDB-Tsb are just enough to
basically show the dependencies between the needed parameter values, but
surely some information is left out. For more detailed information see
the following documents:

ARIB STD-B31 - “Transmission System for Digital Terrestrial Television
Broadcasting” and

ARIB TR-B14 - “Operational Guidelines for Digital Terrestrial Television
Broadcasting”.

In order to understand the ISDB specific parameters, one has to have
some knowledge the channel structure in ISDB-T and ISDB-Tsb. I.e. it has
to be known to the reader that an ISDB-T channel consists of 13
segments, that it can have up to 3 layer sharing those segments, and
things like that.

The following parameters are valid for ISDB-T:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_BANDWIDTH\_HZ](fe_property_parameters.html#dtv-bandwidth-hz)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_GUARD\_INTERVAL](fe_property_parameters.html#dtv-guard-interval)
* [DTV\_TRANSMISSION\_MODE](fe_property_parameters.html#dtv-transmission-mode)
* [DTV\_ISDBT\_LAYER\_ENABLED](fe_property_parameters.html#dtv-isdbt-layer-enabled)
* [DTV\_ISDBT\_PARTIAL\_RECEPTION](fe_property_parameters.html#dtv-isdbt-partial-reception)
* [DTV\_ISDBT\_SOUND\_BROADCASTING](fe_property_parameters.html#dtv-isdbt-sound-broadcasting)
* [DTV\_ISDBT\_SB\_SUBCHANNEL\_ID](fe_property_parameters.html#dtv-isdbt-sb-subchannel-id)
* [DTV\_ISDBT\_SB\_SEGMENT\_IDX](fe_property_parameters.html#dtv-isdbt-sb-segment-idx)
* [DTV\_ISDBT\_SB\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-sb-segment-count)
* [DTV\_ISDBT\_LAYERA\_FEC](fe_property_parameters.html#dtv-isdbt-layer-fec)
* [DTV\_ISDBT\_LAYERA\_MODULATION](fe_property_parameters.html#dtv-isdbt-layer-modulation)
* [DTV\_ISDBT\_LAYERA\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-layer-segment-count)
* [DTV\_ISDBT\_LAYERA\_TIME\_INTERLEAVING](fe_property_parameters.html#dtv-isdbt-layer-time-interleaving)
* [DTV\_ISDBT\_LAYERB\_FEC](fe_property_parameters.html#dtv-isdbt-layer-fec)
* [DTV\_ISDBT\_LAYERB\_MODULATION](fe_property_parameters.html#dtv-isdbt-layer-modulation)
* [DTV\_ISDBT\_LAYERB\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-layer-segment-count)
* [DTV\_ISDBT\_LAYERB\_TIME\_INTERLEAVING](fe_property_parameters.html#dtv-isdbt-layer-time-interleaving)
* [DTV\_ISDBT\_LAYERC\_FEC](fe_property_parameters.html#dtv-isdbt-layer-fec)
* [DTV\_ISDBT\_LAYERC\_MODULATION](fe_property_parameters.html#dtv-isdbt-layer-modulation)
* [DTV\_ISDBT\_LAYERC\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-layer-segment-count)
* [DTV\_ISDBT\_LAYERC\_TIME\_INTERLEAVING](fe_property_parameters.html#dtv-isdbt-layer-time-interleaving)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

## 2.3.3.4. ATSC delivery system

The following parameters are valid for ATSC:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)
* [DTV\_BANDWIDTH\_HZ](fe_property_parameters.html#dtv-bandwidth-hz)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

## 2.3.3.5. ATSC-MH delivery system

The following parameters are valid for ATSC-MH:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_BANDWIDTH\_HZ](fe_property_parameters.html#dtv-bandwidth-hz)
* [DTV\_ATSCMH\_FIC\_VER](fe_property_parameters.html#dtv-atscmh-fic-ver)
* [DTV\_ATSCMH\_PARADE\_ID](fe_property_parameters.html#dtv-atscmh-parade-id)
* [DTV\_ATSCMH\_NOG](fe_property_parameters.html#dtv-atscmh-nog)
* [DTV\_ATSCMH\_TNOG](fe_property_parameters.html#dtv-atscmh-tnog)
* [DTV\_ATSCMH\_SGN](fe_property_parameters.html#dtv-atscmh-sgn)
* [DTV\_ATSCMH\_PRC](fe_property_parameters.html#dtv-atscmh-prc)
* [DTV\_ATSCMH\_RS\_FRAME\_MODE](fe_property_parameters.html#dtv-atscmh-rs-frame-mode)
* [DTV\_ATSCMH\_RS\_FRAME\_ENSEMBLE](fe_property_parameters.html#dtv-atscmh-rs-frame-ensemble)
* [DTV\_ATSCMH\_RS\_CODE\_MODE\_PRI](fe_property_parameters.html#dtv-atscmh-rs-code-mode-pri)
* [DTV\_ATSCMH\_RS\_CODE\_MODE\_SEC](fe_property_parameters.html#dtv-atscmh-rs-code-mode-sec)
* [DTV\_ATSCMH\_SCCC\_BLOCK\_MODE](fe_property_parameters.html#dtv-atscmh-sccc-block-mode)
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_A](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-a)
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_B](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-b)
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_C](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-c)
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_D](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-d)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.

## 2.3.3.6. DTMB delivery system

The following parameters are valid for DTMB:

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version)
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system)
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune)
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear)
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency)
* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation)
* [DTV\_BANDWIDTH\_HZ](fe_property_parameters.html#dtv-bandwidth-hz)
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion)
* [DTV\_INNER\_FEC](fe_property_parameters.html#dtv-inner-fec)
* [DTV\_GUARD\_INTERVAL](fe_property_parameters.html#dtv-guard-interval)
* [DTV\_TRANSMISSION\_MODE](fe_property_parameters.html#dtv-transmission-mode)
* [DTV\_INTERLEAVING](fe_property_parameters.html#dtv-interleaving)
* [DTV\_LNA](fe_property_parameters.html#dtv-lna)

In addition, the [DTV QoS statistics](frontend-stat-properties.html#frontend-stat-properties)
are also valid.
