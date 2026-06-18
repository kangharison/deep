# 2.3.1.Digital TV property parameters

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe_property_parameters.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3.1. Digital TV property parameters

There are several different Digital TV parameters that can be used by
[FE\_SET\_PROPERTY and FE\_GET\_PROPERTY ioctls](fe-get-property.html#fe-get-property).
This section describes each of them. Please notice, however, that only
a subset of them are needed to setup a frontend.

## 2.3.1.1. DTV\_UNDEFINED

Used internally. A GET/SET operation for it won’t change or return
anything.

## 2.3.1.2. DTV\_TUNE

Interpret the cache of data, build either a traditional frontend
tunerequest so we can pass validation in the `FE_SET_FRONTEND` ioctl.

## 2.3.1.3. DTV\_CLEAR

Reset a cache of data specific to the frontend here. This does not
effect hardware.

## 2.3.1.4. DTV\_FREQUENCY

Frequency of the digital TV transponder/channel.

Note

1. For satellite delivery systems, the frequency is in kHz.
2. For cable and terrestrial delivery systems, the frequency is in
   Hz.
3. On most delivery systems, the frequency is the center frequency
   of the transponder/channel. The exception is for ISDB-T, where
   the main carrier has a 1/7 offset from the center.
4. For ISDB-T, the channels are usually transmitted with an offset of
   about 143kHz. E.g. a valid frequency could be 474,143 kHz. The
   stepping is bound to the bandwidth of the channel which is
   typically 6MHz.
5. In ISDB-Tsb, the channel consists of only one or three segments the
   frequency step is 429kHz, 3\*429 respectively.

## 2.3.1.5. DTV\_MODULATION

Specifies the frontend modulation type for delivery systems that
support multiple modulations.

The modulation can be one of the types defined by enum [`fe_modulation`](frontend-header.html#c.fe_modulation "fe_modulation").

Most of the digital TV standards offer more than one possible
modulation type.

The table below presents a summary of the types of modulation types
supported by each delivery system, as currently defined by specs.

| Standard | Modulation types |
| --- | --- |
| ATSC (version 1) | 8-VSB and 16-VSB. |
| DMTB | 4-QAM, 16-QAM, 32-QAM, 64-QAM and 4-QAM-NR. |
| DVB-C Annex A/C | 16-QAM, 32-QAM, 64-QAM and 256-QAM. |
| DVB-C Annex B | 64-QAM. |
| DVB-C2 | QPSK, 16-QAM, 64-QAM, 256-QAM, 1024-QAM and 4096-QAM. |
| DVB-T | QPSK, 16-QAM and 64-QAM. |
| DVB-T2 | QPSK, 16-QAM, 64-QAM and 256-QAM. |
| DVB-S | No need to set. It supports only QPSK. |
| DVB-S2 | QPSK, 8-PSK, 16-APSK and 32-APSK. |
| DVB-S2X | 8-APSK-L, 16-APSK-L, 32-APSK-L, 64-APSK and 64-APSK-L. |
| ISDB-T | QPSK, DQPSK, 16-QAM and 64-QAM. |
| ISDB-S | 8-PSK, QPSK and BPSK. |

Note

As DVB-S2X specifies extensions to the DVB-S2 standard, the same
delivery system `enum value` is used (SYS\_DVBS2).

Please notice that some of the above modulation types may not be
defined currently at the Kernel. The reason is simple: no driver
needed such definition yet.

## 2.3.1.6. DTV\_BANDWIDTH\_HZ

Bandwidth for the channel, in HZ.

Should be set only for terrestrial delivery systems.

Possible values: `1712000`, `5000000`, `6000000`, `7000000`,
`8000000`, `10000000`.

| Terrestrial Standard | Possible values for bandwidth |
| --- | --- |
| ATSC (version 1) | No need to set. It is always 6MHz. |
| DMTB | No need to set. It is always 8MHz. |
| DVB-T | 6MHz, 7MHz and 8MHz. |
| DVB-T2 | 1.172 MHz, 5MHz, 6MHz, 7MHz, 8MHz and 10MHz |
| ISDB-T | 5MHz, 6MHz, 7MHz and 8MHz, although most places use 6MHz. |

Note

1. For ISDB-Tsb, the bandwidth can vary depending on the number of
   connected segments.

   It can be easily derived from other parameters
   (DTV\_ISDBT\_SB\_SEGMENT\_IDX, DTV\_ISDBT\_SB\_SEGMENT\_COUNT).
2. On Satellite and Cable delivery systems, the bandwidth depends on
   the symbol rate. The kernel will silently ignore any [DTV\_BANDWIDTH\_HZ](#dtv-bandwidth-hz)
   setting and overwrites it with bandwidth estimation.

   Such bandwidth estimation takes into account the symbol rate set with
   [DTV\_SYMBOL\_RATE](#dtv-symbol-rate), and the rolloff factor, with is fixed for
   DVB-C and DVB-S.

   For DVB-S2, the rolloff should also be set via [DTV\_ROLLOFF](#dtv-rolloff).

## 2.3.1.7. DTV\_INVERSION

Specifies if the frontend should do spectral inversion or not.

The acceptable values are defined by [`fe_spectral_inversion`](frontend-header.html#c.fe_spectral_inversion "fe_spectral_inversion").

## 2.3.1.8. DTV\_DISEQC\_MASTER

Currently not implemented.

## 2.3.1.9. DTV\_SYMBOL\_RATE

Used on cable and satellite delivery systems.

Digital TV symbol rate, in bauds (symbols/second).

## 2.3.1.10. DTV\_INNER\_FEC

Used on cable and satellite delivery systems.

The acceptable values are defined by [`fe_code_rate`](frontend-header.html#c.fe_code_rate "fe_code_rate").

## 2.3.1.11. DTV\_VOLTAGE

Used on satellite delivery systems.

The voltage is usually used with non-DiSEqC capable LNBs to switch the
polarization (horizontal/vertical). When using DiSEqC equipment this
voltage has to be switched consistently to the DiSEqC commands as
described in the DiSEqC spec.

The acceptable values are defined by [`fe_sec_voltage`](frontend-header.html#c.fe_sec_voltage "fe_sec_voltage").

## 2.3.1.12. DTV\_TONE

Currently not used.

## 2.3.1.13. DTV\_PILOT

Used on DVB-S2.

Sets DVB-S2 pilot.

The acceptable values are defined by [`fe_pilot`](frontend-header.html#c.fe_pilot "fe_pilot").

## 2.3.1.14. DTV\_ROLLOFF

Used on DVB-S2.

Sets DVB-S2 rolloff.

The acceptable values are defined by [`fe_rolloff`](frontend-header.html#c.fe_rolloff "fe_rolloff").

## 2.3.1.15. DTV\_DISEQC\_SLAVE\_REPLY

Currently not implemented.

## 2.3.1.16. DTV\_FE\_CAPABILITY\_COUNT

Currently not implemented.

## 2.3.1.17. DTV\_FE\_CAPABILITY

Currently not implemented.

## 2.3.1.18. DTV\_DELIVERY\_SYSTEM

Specifies the type of the delivery system.

The acceptable values are defined by [`fe_delivery_system`](frontend-header.html#c.fe_delivery_system "fe_delivery_system").

## 2.3.1.19. DTV\_ISDBT\_PARTIAL\_RECEPTION

Used only on ISDB.

If `DTV_ISDBT_SOUND_BROADCASTING` is ‘0’ this bit field represents
whether the channel is in partial reception mode or not.

If ‘1’ `DTV_ISDBT_LAYERA_*` values are assigned to the center segment
and `DTV_ISDBT_LAYERA_SEGMENT_COUNT` has to be ‘1’.

If in addition `DTV_ISDBT_SOUND_BROADCASTING` is ‘1’
`DTV_ISDBT_PARTIAL_RECEPTION` represents whether this ISDB-Tsb channel
is consisting of one segment and layer or three segments and two layers.

Possible values: 0, 1, -1 (AUTO)

## 2.3.1.20. DTV\_ISDBT\_SOUND\_BROADCASTING

Used only on ISDB.

This field represents whether the other DTV\_ISDBT\_\*-parameters are
referring to an ISDB-T and an ISDB-Tsb channel. (See also
`DTV_ISDBT_PARTIAL_RECEPTION`).

Possible values: 0, 1, -1 (AUTO)

## 2.3.1.21. DTV\_ISDBT\_SB\_SUBCHANNEL\_ID

Used only on ISDB.

This field only applies if `DTV_ISDBT_SOUND_BROADCASTING` is ‘1’.

(Note of the author: This might not be the correct description of the
`SUBCHANNEL-ID` in all details, but it is my understanding of the
technical background needed to program a device)

An ISDB-Tsb channel (1 or 3 segments) can be broadcasted alone or in a
set of connected ISDB-Tsb channels. In this set of channels every
channel can be received independently. The number of connected ISDB-Tsb
segment can vary, e.g. depending on the frequency spectrum bandwidth
available.

Example: Assume 8 ISDB-Tsb connected segments are broadcasted. The
broadcaster has several possibilities to put those channels in the air:
Assuming a normal 13-segment ISDB-T spectrum he can align the 8 segments
from position 1-8 to 5-13 or anything in between.

The underlying layer of segments are sub-channels: each segment is
consisting of several sub-channels with a predefined IDs. A sub-channel
is used to help the demodulator to synchronize on the channel.

An ISDB-T channel is always centered over all sub-channels. As for the
example above, in ISDB-Tsb it is no longer as simple as that.

`The DTV_ISDBT_SB_SUBCHANNEL_ID` parameter is used to give the
sub-channel ID of the segment to be demodulated.

Possible values: 0 .. 41, -1 (AUTO)

## 2.3.1.22. DTV\_ISDBT\_SB\_SEGMENT\_IDX

Used only on ISDB.

This field only applies if `DTV_ISDBT_SOUND_BROADCASTING` is ‘1’.

`DTV_ISDBT_SB_SEGMENT_IDX` gives the index of the segment to be
demodulated for an ISDB-Tsb channel where several of them are
transmitted in the connected manner.

Possible values: 0 .. `DTV_ISDBT_SB_SEGMENT_COUNT` - 1

Note: This value cannot be determined by an automatic channel search.

## 2.3.1.23. DTV\_ISDBT\_SB\_SEGMENT\_COUNT

Used only on ISDB.

This field only applies if `DTV_ISDBT_SOUND_BROADCASTING` is ‘1’.

`DTV_ISDBT_SB_SEGMENT_COUNT` gives the total count of connected
ISDB-Tsb channels.

Possible values: 1 .. 13

Note: This value cannot be determined by an automatic channel search.

## 2.3.1.24. DTV-ISDBT-LAYER[A-C] parameters

Used only on ISDB.

ISDB-T channels can be coded hierarchically. As opposed to DVB-T in
ISDB-T hierarchical layers can be decoded simultaneously. For that
reason a ISDB-T demodulator has 3 Viterbi and 3 Reed-Solomon decoders.

ISDB-T has 3 hierarchical layers which each can use a part of the
available segments. The total number of segments over all layers has to
13 in ISDB-T.

There are 3 parameter sets, for Layers A, B and C.

### 2.3.1.24.1. DTV\_ISDBT\_LAYER\_ENABLED

Used only on ISDB.

Hierarchical reception in ISDB-T is achieved by enabling or disabling
layers in the decoding process. Setting all bits of
`DTV_ISDBT_LAYER_ENABLED` to ‘1’ forces all layers (if applicable) to
be demodulated. This is the default.

If the channel is in the partial reception mode
(`DTV_ISDBT_PARTIAL_RECEPTION` = 1) the central segment can be decoded
independently of the other 12 segments. In that mode layer A has to have
a `SEGMENT_COUNT` of 1.

In ISDB-Tsb only layer A is used, it can be 1 or 3 in ISDB-Tsb according
to `DTV_ISDBT_PARTIAL_RECEPTION`. `SEGMENT_COUNT` must be filled
accordingly.

Only the values of the first 3 bits are used. Other bits will be silently ignored:

`DTV_ISDBT_LAYER_ENABLED` bit 0: layer A enabled

`DTV_ISDBT_LAYER_ENABLED` bit 1: layer B enabled

`DTV_ISDBT_LAYER_ENABLED` bit 2: layer C enabled

`DTV_ISDBT_LAYER_ENABLED` bits 3-31: unused

### 2.3.1.24.2. DTV\_ISDBT\_LAYER[A-C]\_FEC

Used only on ISDB.

The Forward Error Correction mechanism used by a given ISDB Layer, as
defined by [`fe_code_rate`](frontend-header.html#c.fe_code_rate "fe_code_rate").

Possible values are: `FEC_AUTO`, `FEC_1_2`, `FEC_2_3`, `FEC_3_4`,
`FEC_5_6`, `FEC_7_8`

### 2.3.1.24.3. DTV\_ISDBT\_LAYER[A-C]\_MODULATION

Used only on ISDB.

The modulation used by a given ISDB Layer, as defined by
[`fe_modulation`](frontend-header.html#c.fe_modulation "fe_modulation").

Possible values are: `QAM_AUTO`, `QPSK`, `QAM_16`, `QAM_64`, `DQPSK`

Note

1. If layer C is `DQPSK`, then layer B has to be `DQPSK`.
2. If layer B is `DQPSK` and `DTV_ISDBT_PARTIAL_RECEPTION`= 0,
   then layer has to be `DQPSK`.

### 2.3.1.24.4. DTV\_ISDBT\_LAYER[A-C]\_SEGMENT\_COUNT

Used only on ISDB.

Possible values: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1 (AUTO)

Note: Truth table for `DTV_ISDBT_SOUND_BROADCASTING` and
`DTV_ISDBT_PARTIAL_RECEPTION` and `LAYER[A-C]_SEGMENT_COUNT`

Truth table for ISDB-T Sound Broadcasting

| Partial Reception | Sound Broadcasting | Layer A width | Layer B width | Layer C width | total width |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 | 1 .. 13 | 1 .. 13 | 1 .. 13 | 13 |
| 1 | 0 | 1 | 1 .. 13 | 1 .. 13 | 13 |
| 0 | 1 | 1 | 0 | 0 | 1 |
| 1 | 1 | 1 | 2 | 0 | 13 |

### 2.3.1.24.5. DTV\_ISDBT\_LAYER[A-C]\_TIME\_INTERLEAVING

Used only on ISDB.

Valid values: 0, 1, 2, 4, -1 (AUTO)

when DTV\_ISDBT\_SOUND\_BROADCASTING is active, value 8 is also valid.

Note: The real time interleaving length depends on the mode (fft-size).
The values here are referring to what can be found in the
TMCC-structure, as shown in the table below.

type isdbt\_layer\_interleaving\_table

ISDB-T time interleaving modes

| `DTV_ISDBT_LAYER[A-C]_TIME_INTERLEAVING` | Mode 1 (2K FFT) | Mode 2 (4K FFT) | Mode 3 (8K FFT) |
| --- | --- | --- | --- |
| 0 | 0 | 0 | 0 |
| 1 | 4 | 2 | 1 |
| 2 | 8 | 4 | 2 |
| 4 | 16 | 8 | 4 |

### 2.3.1.24.6. DTV\_ATSCMH\_FIC\_VER

Used only on ATSC-MH.

Version number of the FIC (Fast Information Channel) signaling data.

FIC is used for relaying information to allow rapid service acquisition
by the receiver.

Possible values: 0, 1, 2, 3, ..., 30, 31

### 2.3.1.24.7. DTV\_ATSCMH\_PARADE\_ID

Used only on ATSC-MH.

Parade identification number

A parade is a collection of up to eight MH groups, conveying one or two
ensembles.

Possible values: 0, 1, 2, 3, ..., 126, 127

### 2.3.1.24.8. DTV\_ATSCMH\_NOG

Used only on ATSC-MH.

Number of MH groups per MH subframe for a designated parade.

Possible values: 1, 2, 3, 4, 5, 6, 7, 8

### 2.3.1.24.9. DTV\_ATSCMH\_TNOG

Used only on ATSC-MH.

Total number of MH groups including all MH groups belonging to all MH
parades in one MH subframe.

Possible values: 0, 1, 2, 3, ..., 30, 31

### 2.3.1.24.10. DTV\_ATSCMH\_SGN

Used only on ATSC-MH.

Start group number.

Possible values: 0, 1, 2, 3, ..., 14, 15

### 2.3.1.24.11. DTV\_ATSCMH\_PRC

Used only on ATSC-MH.

Parade repetition cycle.

Possible values: 1, 2, 3, 4, 5, 6, 7, 8

### 2.3.1.24.12. DTV\_ATSCMH\_RS\_FRAME\_MODE

Used only on ATSC-MH.

Reed Solomon (RS) frame mode.

The acceptable values are defined by [`atscmh_rs_frame_mode`](frontend-header.html#c.atscmh_rs_frame_mode "atscmh_rs_frame_mode").

### 2.3.1.24.13. DTV\_ATSCMH\_RS\_FRAME\_ENSEMBLE

Used only on ATSC-MH.

Reed Solomon (RS) frame ensemble.

The acceptable values are defined by [`atscmh_rs_frame_ensemble`](frontend-header.html#c.atscmh_rs_frame_ensemble "atscmh_rs_frame_ensemble").

### 2.3.1.24.14. DTV\_ATSCMH\_RS\_CODE\_MODE\_PRI

Used only on ATSC-MH.

Reed Solomon (RS) code mode (primary).

The acceptable values are defined by [`atscmh_rs_code_mode`](frontend-header.html#c.atscmh_rs_code_mode "atscmh_rs_code_mode").

### 2.3.1.24.15. DTV\_ATSCMH\_RS\_CODE\_MODE\_SEC

Used only on ATSC-MH.

Reed Solomon (RS) code mode (secondary).

The acceptable values are defined by [`atscmh_rs_code_mode`](frontend-header.html#c.atscmh_rs_code_mode "atscmh_rs_code_mode").

### 2.3.1.24.16. DTV\_ATSCMH\_SCCC\_BLOCK\_MODE

Used only on ATSC-MH.

Series Concatenated Convolutional Code Block Mode.

The acceptable values are defined by [`atscmh_sccc_block_mode`](frontend-header.html#c.atscmh_sccc_block_mode "atscmh_sccc_block_mode").

### 2.3.1.24.17. DTV\_ATSCMH\_SCCC\_CODE\_MODE\_A

Used only on ATSC-MH.

Series Concatenated Convolutional Code Rate.

The acceptable values are defined by [`atscmh_sccc_code_mode`](frontend-header.html#c.atscmh_sccc_code_mode "atscmh_sccc_code_mode").

### 2.3.1.24.18. DTV\_ATSCMH\_SCCC\_CODE\_MODE\_B

Used only on ATSC-MH.

Series Concatenated Convolutional Code Rate.

Possible values are the same as documented on enum
[`atscmh_sccc_code_mode`](frontend-header.html#c.atscmh_sccc_code_mode "atscmh_sccc_code_mode").

### 2.3.1.24.19. DTV\_ATSCMH\_SCCC\_CODE\_MODE\_C

Used only on ATSC-MH.

Series Concatenated Convolutional Code Rate.

Possible values are the same as documented on enum
[`atscmh_sccc_code_mode`](frontend-header.html#c.atscmh_sccc_code_mode "atscmh_sccc_code_mode").

### 2.3.1.24.20. DTV\_ATSCMH\_SCCC\_CODE\_MODE\_D

Used only on ATSC-MH.

Series Concatenated Convolutional Code Rate.

Possible values are the same as documented on enum
[`atscmh_sccc_code_mode`](frontend-header.html#c.atscmh_sccc_code_mode "atscmh_sccc_code_mode").

## 2.3.1.25. DTV\_API\_VERSION

Returns the major/minor version of the Digital TV API

## 2.3.1.26. DTV\_CODE\_RATE\_HP

Used on terrestrial transmissions.

The acceptable values are defined by [`fe_transmit_mode`](frontend-header.html#c.fe_transmit_mode "fe_transmit_mode").

## 2.3.1.27. DTV\_CODE\_RATE\_LP

Used on terrestrial transmissions.

The acceptable values are defined by [`fe_transmit_mode`](frontend-header.html#c.fe_transmit_mode "fe_transmit_mode").

## 2.3.1.28. DTV\_GUARD\_INTERVAL

The acceptable values are defined by [`fe_guard_interval`](frontend-header.html#c.fe_guard_interval "fe_guard_interval").

Note

1. If `DTV_GUARD_INTERVAL` is set the `GUARD_INTERVAL_AUTO` the
   hardware will try to find the correct guard interval (if capable) and
   will use TMCC to fill in the missing parameters.
2. Interval `GUARD_INTERVAL_1_64` is used only for DVB-C2.
3. Interval `GUARD_INTERVAL_1_128` is used for both DVB-C2 and DVB\_T2.
4. Intervals `GUARD_INTERVAL_19_128` and `GUARD_INTERVAL_19_256` are
   used only for DVB-T2.
5. Intervals `GUARD_INTERVAL_PN420`, `GUARD_INTERVAL_PN595` and
   `GUARD_INTERVAL_PN945` are used only for DMTB at the present.
   On such standard, only those intervals and `GUARD_INTERVAL_AUTO`
   are valid.

## 2.3.1.29. DTV\_TRANSMISSION\_MODE

Used only on OFTM-based standards, e. g. DVB-T/T2, ISDB-T, DTMB.

Specifies the FFT size (with corresponds to the approximate number of
carriers) used by the standard.

The acceptable values are defined by [`fe_transmit_mode`](frontend-header.html#c.fe_transmit_mode "fe_transmit_mode").

Note

1. ISDB-T supports three carrier/symbol-size: 8K, 4K, 2K. It is called
   **mode** on such standard, and are numbered from 1 to 3:

   | Mode | FFT size | Transmission mode |
   | --- | --- | --- |
   | 1 | 2K | `TRANSMISSION_MODE_2K` |
   | 2 | 4K | `TRANSMISSION_MODE_4K` |
   | 3 | 8K | `TRANSMISSION_MODE_8K` |
2. If `DTV_TRANSMISSION_MODE` is set the `TRANSMISSION_MODE_AUTO`
   the hardware will try to find the correct FFT-size (if capable) and
   will use TMCC to fill in the missing parameters.
3. DVB-T specifies 2K and 8K as valid sizes.
4. DVB-T2 specifies 1K, 2K, 4K, 8K, 16K and 32K.
5. DTMB specifies C1 and C3780.

## 2.3.1.30. DTV\_HIERARCHY

Used only on DVB-T and DVB-T2.

Frontend hierarchy.

The acceptable values are defined by [`fe_hierarchy`](frontend-header.html#c.fe_hierarchy "fe_hierarchy").

## 2.3.1.31. DTV\_STREAM\_ID

Used on DVB-C2, DVB-S2, DVB-T2 and ISDB-S.

DVB-C2, DVB-S2, DVB-T2 and ISDB-S support the transmission of several
streams on a single transport stream. This property enables the digital
TV driver to handle substream filtering, when supported by the hardware.
By default, substream filtering is disabled.

For DVB-C2, DVB-S2 and DVB-T2, the valid substream id range is from 0 to
255.

For ISDB, the valid substream id range is from 1 to 65535.

To disable it, you should use the special macro NO\_STREAM\_ID\_FILTER.

Note: any value outside the id range also disables filtering.

## 2.3.1.32. DTV\_DVBT2\_PLP\_ID\_LEGACY

Obsolete, replaced with DTV\_STREAM\_ID.

## 2.3.1.33. DTV\_ENUM\_DELSYS

A Multi standard frontend needs to advertise the delivery systems
provided. Applications need to enumerate the provided delivery systems,
before using any other operation with the frontend. Prior to its
introduction, FE\_GET\_INFO was used to determine a frontend type. A
frontend which provides more than a single delivery system,
FE\_GET\_INFO doesn’t help much. Applications which intends to use a
multistandard frontend must enumerate the delivery systems associated
with it, rather than trying to use FE\_GET\_INFO. In the case of a
legacy frontend, the result is just the same as with FE\_GET\_INFO, but
in a more structured format.

The acceptable values are defined by [`fe_delivery_system`](frontend-header.html#c.fe_delivery_system "fe_delivery_system").

## 2.3.1.34. DTV\_INTERLEAVING

Time interleaving to be used.

The acceptable values are defined by [`fe_interleaving`](frontend-header.html#c.fe_interleaving "fe_interleaving").

## 2.3.1.35. DTV\_LNA

Low-noise amplifier.

Hardware might offer controllable LNA which can be set manually using
that parameter. Usually LNA could be found only from terrestrial devices
if at all.

Possible values: 0, 1, LNA\_AUTO

0, LNA off

1, LNA on

use the special macro LNA\_AUTO to set LNA auto

## 2.3.1.36. DTV\_SCRAMBLING\_SEQUENCE\_INDEX

Used on DVB-S2.

This 18 bit field, when present, carries the index of the DVB-S2 physical
layer scrambling sequence as defined in clause 5.5.4 of EN 302 307.
There is no explicit signalling method to convey scrambling sequence index
to the receiver. If S2 satellite delivery system descriptor is available
it can be used to read the scrambling sequence index (EN 300 468 table 41).

By default, gold scrambling sequence index 0 is used.

The valid scrambling sequence index range is from 0 to 262142.
