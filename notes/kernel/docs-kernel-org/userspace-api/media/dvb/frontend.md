# 2.Digital TV Frontend API

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/frontend.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. Digital TV Frontend API

The Digital TV frontend API was designed to support three groups of delivery
systems: Terrestrial, cable and Satellite. Currently, the following
delivery systems are supported:

* Terrestrial systems: DVB-T, DVB-T2, ATSC, ATSC M/H, ISDB-T, DVB-H,
  DTMB, CMMB
* Cable systems: DVB-C Annex A/C, ClearQAM (DVB-C Annex B)
* Satellite systems: DVB-S, DVB-S2, DVB Turbo, ISDB-S, DSS

The Digital TV frontend controls several sub-devices including:

* Tuner
* Digital TV demodulator
* Low noise amplifier (LNA)
* Satellite Equipment Control (SEC) [[1]](#f1).

The frontend can be accessed through `/dev/dvb/adapter?/frontend?`.
Data types and ioctl definitions can be accessed by including
`linux/dvb/frontend.h` in your application.

Note

Transmission via the internet (DVB-IP) and MMT (MPEG Media Transport)
is not yet handled by this API but a future extension is possible.

[[1](#id1)]

On Satellite systems, the API support for the Satellite Equipment
Control (SEC) allows to power control and to send/receive signals to
control the antenna subsystem, selecting the polarization and choosing
the Intermediate Frequency IF) of the Low Noise Block Converter Feed
Horn (LNBf). It supports the DiSEqC and V-SEC protocols. The DiSEqC
(digital SEC) specification is available at
[Eutelsat](http://www.eutelsat.com/satellites/4_5_5.html).

* [2.1. Querying frontend information](query-dvb-frontend-info.html)
* [2.2. Querying frontend status and statistics](dvb-fe-read-status.html)
* [2.3. Property types](dvbproperty.html)
* [2.4. Frontend Function Calls](frontend_fcalls.html)
