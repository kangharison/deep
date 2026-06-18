# 2.3.6.Frontend uAPI data types

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/frontend-header.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3.6. Frontend uAPI data types

enum fe\_caps
:   Frontend capabilities

**Constants**

`FE_IS_STUPID`
:   There’s something wrong at the
    frontend, and it can’t report its
    capabilities.

`FE_CAN_INVERSION_AUTO`
:   Can auto-detect frequency spectral
    band inversion

`FE_CAN_FEC_1_2`
:   Supports FEC 1/2

`FE_CAN_FEC_2_3`
:   Supports FEC 2/3

`FE_CAN_FEC_3_4`
:   Supports FEC 3/4

`FE_CAN_FEC_4_5`
:   Supports FEC 4/5

`FE_CAN_FEC_5_6`
:   Supports FEC 5/6

`FE_CAN_FEC_6_7`
:   Supports FEC 6/7

`FE_CAN_FEC_7_8`
:   Supports FEC 7/8

`FE_CAN_FEC_8_9`
:   Supports FEC 8/9

`FE_CAN_FEC_AUTO`
:   Can auto-detect FEC

`FE_CAN_QPSK`
:   Supports QPSK modulation

`FE_CAN_QAM_16`
:   Supports 16-QAM modulation

`FE_CAN_QAM_32`
:   Supports 32-QAM modulation

`FE_CAN_QAM_64`
:   Supports 64-QAM modulation

`FE_CAN_QAM_128`
:   Supports 128-QAM modulation

`FE_CAN_QAM_256`
:   Supports 256-QAM modulation

`FE_CAN_QAM_AUTO`
:   Can auto-detect QAM modulation

`FE_CAN_TRANSMISSION_MODE_AUTO`
:   Can auto-detect transmission mode

`FE_CAN_BANDWIDTH_AUTO`
:   Can auto-detect bandwidth

`FE_CAN_GUARD_INTERVAL_AUTO`
:   Can auto-detect guard interval

`FE_CAN_HIERARCHY_AUTO`
:   Can auto-detect hierarchy

`FE_CAN_8VSB`
:   Supports 8-VSB modulation

`FE_CAN_16VSB`
:   Supporta 16-VSB modulation

`FE_HAS_EXTENDED_CAPS`
:   Unused

`FE_CAN_MULTISTREAM`
:   Supports multistream filtering

`FE_CAN_TURBO_FEC`
:   Supports “turbo FEC” modulation

`FE_CAN_2G_MODULATION`
:   Supports “2nd generation” modulation,
    e. g. DVB-S2, DVB-T2, DVB-C2

`FE_NEEDS_BENDING`
:   Unused

`FE_CAN_RECOVER`
:   Can recover from a cable unplug
    automatically

`FE_CAN_MUTE_TS`
:   Can stop spurious TS data output

struct dvb\_frontend\_info
:   Frontend properties and capabilities

**Definition**:

```
struct dvb_frontend_info {
    char name[128];
    enum fe_type type;
    __u32 frequency_min;
    __u32 frequency_max;
    __u32 frequency_stepsize;
    __u32 frequency_tolerance;
    __u32 symbol_rate_min;
    __u32 symbol_rate_max;
    __u32 symbol_rate_tolerance;
    __u32 notifier_delay;
    enum fe_caps caps;
};
```

**Members**

`name`
:   Name of the frontend

`type`
:   **DEPRECATED**.
    Should not be used on modern programs,
    as a frontend may have more than one type.
    In order to get the support types of a given
    frontend, use `DTV_ENUM_DELSYS`
    instead.

`frequency_min`
:   Minimal frequency supported by the frontend.

`frequency_max`
:   Minimal frequency supported by the frontend.

`frequency_stepsize`
:   All frequencies are multiple of this value.

`frequency_tolerance`
:   Frequency tolerance.

`symbol_rate_min`
:   Minimal symbol rate, in bauds
    (for Cable/Satellite systems).

`symbol_rate_max`
:   Maximal symbol rate, in bauds
    (for Cable/Satellite systems).

`symbol_rate_tolerance`
:   Maximal symbol rate tolerance, in ppm
    (for Cable/Satellite systems).

`notifier_delay`
:   **DEPRECATED**. Not used by any driver.

`caps`
:   Capabilities supported by the frontend,
    as specified in [`enum fe_caps`](#c.fe_caps "fe_caps").

**Description**

struct dvb\_diseqc\_master\_cmd
:   DiSEqC master command

**Definition**:

```
struct dvb_diseqc_master_cmd {
    __u8 msg[6];
    __u8 msg_len;
};
```

**Members**

`msg`
:   DiSEqC message to be sent. It contains a 3 bytes header with:
    framing + address + command, and an optional argument
    of up to 3 bytes of data.

`msg_len`
:   Length of the DiSEqC message. Valid values are 3 to 6.

**Description**

Check out the DiSEqC bus spec available on <http://www.eutelsat.org/> for
the possible messages that can be used.

struct dvb\_diseqc\_slave\_reply
:   DiSEqC received data

**Definition**:

```
struct dvb_diseqc_slave_reply {
    __u8 msg[4];
    __u8 msg_len;
    int timeout;
};
```

**Members**

`msg`
:   DiSEqC message buffer to store a message received via DiSEqC.
    It contains one byte header with: framing and
    an optional argument of up to 3 bytes of data.

`msg_len`
:   Length of the DiSEqC message. Valid values are 0 to 4,
    where 0 means no message.

`timeout`
:   Return from ioctl after timeout ms with errorcode when
    no message was received.

**Description**

Check out the DiSEqC bus spec available on <http://www.eutelsat.org/> for
the possible messages that can be used.

enum fe\_sec\_voltage
:   DC Voltage used to feed the LNBf

**Constants**

`SEC_VOLTAGE_13`
:   Output 13V to the LNBf

`SEC_VOLTAGE_18`
:   Output 18V to the LNBf

`SEC_VOLTAGE_OFF`
:   Don’t feed the LNBf with a DC voltage

enum fe\_sec\_tone\_mode
:   Type of tone to be send to the LNBf.

**Constants**

`SEC_TONE_ON`
:   Sends a 22kHz tone burst to the antenna.

`SEC_TONE_OFF`
:   Don’t send a 22kHz tone to the antenna (except
    if the `FE_DISEQC_*` ioctls are called).

enum fe\_sec\_mini\_cmd
:   Type of mini burst to be sent

**Constants**

`SEC_MINI_A`
:   Sends a mini-DiSEqC 22kHz ‘0’ Tone Burst to select
    satellite-A

`SEC_MINI_B`
:   Sends a mini-DiSEqC 22kHz ‘1’ Data Burst to select
    satellite-B

enum fe\_status
:   Enumerates the possible frontend status.

**Constants**

`FE_NONE`
:   The frontend doesn’t have any kind of lock.
    That’s the initial frontend status

`FE_HAS_SIGNAL`
:   Has found something above the noise level.

`FE_HAS_CARRIER`
:   Has found a signal.

`FE_HAS_VITERBI`
:   FEC inner coding (Viterbi, LDPC or other inner code).
    is stable.

`FE_HAS_SYNC`
:   Synchronization bytes was found.

`FE_HAS_LOCK`
:   Digital TV were locked and everything is working.

`FE_TIMEDOUT`
:   Fo lock within the last about 2 seconds.

`FE_REINIT`
:   Frontend was reinitialized, application is recommended
    to reset DiSEqC, tone and parameters.

enum fe\_spectral\_inversion
:   Type of inversion band

**Constants**

`INVERSION_OFF`
:   Don’t do spectral band inversion.

`INVERSION_ON`
:   Do spectral band inversion.

`INVERSION_AUTO`
:   Autodetect spectral band inversion.

**Description**

This parameter indicates if spectral inversion should be presumed or
not. In the automatic setting (`INVERSION_AUTO`) the hardware will try
to figure out the correct setting by itself. If the hardware doesn’t
support, the `dvb_frontend` will try to lock at the carrier first with
inversion off. If it fails, it will try to enable inversion.

enum fe\_code\_rate
:   Type of Forward Error Correction (FEC)

**Constants**

`FEC_NONE`
:   No Forward Error Correction Code

`FEC_1_2`
:   Forward Error Correction Code 1/2

`FEC_2_3`
:   Forward Error Correction Code 2/3

`FEC_3_4`
:   Forward Error Correction Code 3/4

`FEC_4_5`
:   Forward Error Correction Code 4/5

`FEC_5_6`
:   Forward Error Correction Code 5/6

`FEC_6_7`
:   Forward Error Correction Code 6/7

`FEC_7_8`
:   Forward Error Correction Code 7/8

`FEC_8_9`
:   Forward Error Correction Code 8/9

`FEC_AUTO`
:   Autodetect Error Correction Code

`FEC_3_5`
:   Forward Error Correction Code 3/5

`FEC_9_10`
:   Forward Error Correction Code 9/10

`FEC_2_5`
:   Forward Error Correction Code 2/5

`FEC_1_3`
:   Forward Error Correction Code 1/3

`FEC_1_4`
:   Forward Error Correction Code 1/4

`FEC_5_9`
:   Forward Error Correction Code 5/9

`FEC_7_9`
:   Forward Error Correction Code 7/9

`FEC_8_15`
:   Forward Error Correction Code 8/15

`FEC_11_15`
:   Forward Error Correction Code 11/15

`FEC_13_18`
:   Forward Error Correction Code 13/18

`FEC_9_20`
:   Forward Error Correction Code 9/20

`FEC_11_20`
:   Forward Error Correction Code 11/20

`FEC_23_36`
:   Forward Error Correction Code 23/36

`FEC_25_36`
:   Forward Error Correction Code 25/36

`FEC_13_45`
:   Forward Error Correction Code 13/45

`FEC_26_45`
:   Forward Error Correction Code 26/45

`FEC_28_45`
:   Forward Error Correction Code 28/45

`FEC_32_45`
:   Forward Error Correction Code 32/45

`FEC_77_90`
:   Forward Error Correction Code 77/90

`FEC_11_45`
:   Forward Error Correction Code 11/45

`FEC_4_15`
:   Forward Error Correction Code 4/15

`FEC_14_45`
:   Forward Error Correction Code 14/45

`FEC_7_15`
:   Forward Error Correction Code 7/15

**Description**

Please note that not all FEC types are supported by a given standard.

enum fe\_modulation
:   Type of modulation/constellation

**Constants**

`QPSK`
:   QPSK modulation

`QAM_16`
:   16-QAM modulation

`QAM_32`
:   32-QAM modulation

`QAM_64`
:   64-QAM modulation

`QAM_128`
:   128-QAM modulation

`QAM_256`
:   256-QAM modulation

`QAM_AUTO`
:   Autodetect QAM modulation

`VSB_8`
:   8-VSB modulation

`VSB_16`
:   16-VSB modulation

`PSK_8`
:   8-PSK modulation

`APSK_16`
:   16-APSK modulation

`APSK_32`
:   32-APSK modulation

`DQPSK`
:   DQPSK modulation

`QAM_4_NR`
:   4-QAM-NR modulation

`QAM_1024`
:   1024-QAM modulation

`QAM_4096`
:   4096-QAM modulation

`APSK_8_L`
:   8APSK-L modulation

`APSK_16_L`
:   16APSK-L modulation

`APSK_32_L`
:   32APSK-L modulation

`APSK_64`
:   64APSK modulation

`APSK_64_L`
:   64APSK-L modulation

**Description**

Please note that not all modulations are supported by a given standard.

enum fe\_transmit\_mode
:   Transmission mode

**Constants**

`TRANSMISSION_MODE_2K`
:   Transmission mode 2K

`TRANSMISSION_MODE_8K`
:   Transmission mode 8K

`TRANSMISSION_MODE_AUTO`
:   Autodetect transmission mode. The hardware will try to find the
    correct FFT-size (if capable) to fill in the missing parameters.

`TRANSMISSION_MODE_4K`
:   Transmission mode 4K

`TRANSMISSION_MODE_1K`
:   Transmission mode 1K

`TRANSMISSION_MODE_16K`
:   Transmission mode 16K

`TRANSMISSION_MODE_32K`
:   Transmission mode 32K

`TRANSMISSION_MODE_C1`
:   Single Carrier (C=1) transmission mode (DTMB only)

`TRANSMISSION_MODE_C3780`
:   Multi Carrier (C=3780) transmission mode (DTMB only)

**Description**

Please note that not all transmission modes are supported by a given
standard.

enum fe\_guard\_interval
:   Guard interval

**Constants**

`GUARD_INTERVAL_1_32`
:   Guard interval 1/32

`GUARD_INTERVAL_1_16`
:   Guard interval 1/16

`GUARD_INTERVAL_1_8`
:   Guard interval 1/8

`GUARD_INTERVAL_1_4`
:   Guard interval 1/4

`GUARD_INTERVAL_AUTO`
:   Autodetect the guard interval

`GUARD_INTERVAL_1_128`
:   Guard interval 1/128

`GUARD_INTERVAL_19_128`
:   Guard interval 19/128

`GUARD_INTERVAL_19_256`
:   Guard interval 19/256

`GUARD_INTERVAL_PN420`
:   PN length 420 (1/4)

`GUARD_INTERVAL_PN595`
:   PN length 595 (1/6)

`GUARD_INTERVAL_PN945`
:   PN length 945 (1/9)

`GUARD_INTERVAL_1_64`
:   Guard interval 1/64

**Description**

Please note that not all guard intervals are supported by a given standard.

enum fe\_hierarchy
:   Hierarchy

**Constants**

`HIERARCHY_NONE`
:   No hierarchy

`HIERARCHY_1`
:   Hierarchy 1

`HIERARCHY_2`
:   Hierarchy 2

`HIERARCHY_4`
:   Hierarchy 4

`HIERARCHY_AUTO`
:   Autodetect hierarchy (if supported)

**Description**

Please note that not all hierarchy types are supported by a given standard.

enum fe\_interleaving
:   Interleaving

**Constants**

`INTERLEAVING_NONE`
:   No interleaving.

`INTERLEAVING_AUTO`
:   Auto-detect interleaving.

`INTERLEAVING_240`
:   Interleaving of 240 symbols.

`INTERLEAVING_720`
:   Interleaving of 720 symbols.

**Description**

Please note that, currently, only DTMB uses it.

enum fe\_pilot
:   Type of pilot tone

**Constants**

`PILOT_ON`
:   Pilot tones enabled

`PILOT_OFF`
:   Pilot tones disabled

`PILOT_AUTO`
:   Autodetect pilot tones

enum fe\_rolloff
:   Rolloff factor

**Constants**

`ROLLOFF_35`
:   Roloff factor: α=35%

`ROLLOFF_20`
:   Roloff factor: α=20%

`ROLLOFF_25`
:   Roloff factor: α=25%

`ROLLOFF_AUTO`
:   Auto-detect the roloff factor.

`ROLLOFF_15`
:   Rolloff factor: α=15%

`ROLLOFF_10`
:   Rolloff factor: α=10%

`ROLLOFF_5`
:   Rolloff factor: α=5%

**Description**

enum fe\_delivery\_system
:   Type of the delivery system

**Constants**

`SYS_UNDEFINED`
:   Undefined standard. Generally, indicates an error

`SYS_DVBC_ANNEX_A`
:   Cable TV: DVB-C following ITU-T J.83 Annex A spec

`SYS_DVBC_ANNEX_B`
:   Cable TV: DVB-C following ITU-T J.83 Annex B spec (ClearQAM)

`SYS_DVBT`
:   Terrestrial TV: DVB-T

`SYS_DSS`
:   Satellite TV: DSS (not fully supported)

`SYS_DVBS`
:   Satellite TV: DVB-S

`SYS_DVBS2`
:   Satellite TV: DVB-S2 and DVB-S2X

`SYS_DVBH`
:   Terrestrial TV (mobile): DVB-H (standard deprecated)

`SYS_ISDBT`
:   Terrestrial TV: ISDB-T

`SYS_ISDBS`
:   Satellite TV: ISDB-S

`SYS_ISDBC`
:   Cable TV: ISDB-C (no drivers yet)

`SYS_ATSC`
:   Terrestrial TV: ATSC

`SYS_ATSCMH`
:   Terrestrial TV (mobile): ATSC-M/H

`SYS_DTMB`
:   Terrestrial TV: DTMB

`SYS_CMMB`
:   Terrestrial TV (mobile): CMMB (not fully supported)

`SYS_DAB`
:   Digital audio: DAB (not fully supported)

`SYS_DVBT2`
:   Terrestrial TV: DVB-T2

`SYS_TURBO`
:   Satellite TV: DVB-S Turbo

`SYS_DVBC_ANNEX_C`
:   Cable TV: DVB-C following ITU-T J.83 Annex C spec

`SYS_DVBC2`
:   Cable TV: DVB-C2

enum atscmh\_sccc\_block\_mode
:   Type of Series Concatenated Convolutional Code Block Mode.

**Constants**

`ATSCMH_SCCC_BLK_SEP`
:   Separate SCCC: the SCCC outer code mode shall be set independently
    for each Group Region (A, B, C, D)

`ATSCMH_SCCC_BLK_COMB`
:   Combined SCCC: all four Regions shall have the same SCCC outer
    code mode.

`ATSCMH_SCCC_BLK_RES`
:   Reserved. Shouldn’t be used.

enum atscmh\_sccc\_code\_mode
:   Type of Series Concatenated Convolutional Code Rate.

**Constants**

`ATSCMH_SCCC_CODE_HLF`
:   The outer code rate of a SCCC Block is 1/2 rate.

`ATSCMH_SCCC_CODE_QTR`
:   The outer code rate of a SCCC Block is 1/4 rate.

`ATSCMH_SCCC_CODE_RES`
:   Reserved. Should not be used.

enum atscmh\_rs\_frame\_ensemble
:   Reed Solomon(RS) frame ensemble.

**Constants**

`ATSCMH_RSFRAME_ENS_PRI`
:   Primary Ensemble.

`ATSCMH_RSFRAME_ENS_SEC`
:   Secondary Ensemble.

enum atscmh\_rs\_frame\_mode
:   Reed Solomon (RS) frame mode.

**Constants**

`ATSCMH_RSFRAME_PRI_ONLY`
:   Single Frame: There is only a primary RS Frame for all Group
    Regions.

`ATSCMH_RSFRAME_PRI_SEC`
:   Dual Frame: There are two separate RS Frames: Primary RS Frame for
    Group Region A and B and Secondary RS Frame for Group Region C and
    D.

`ATSCMH_RSFRAME_RES`
:   Reserved. Shouldn’t be used.

enum atscmh\_rs\_code\_mode
:   ATSC-M/H Reed Solomon modes

**Constants**

`ATSCMH_RSCODE_211_187`
:   Reed Solomon code (211,187).

`ATSCMH_RSCODE_223_187`
:   Reed Solomon code (223,187).

`ATSCMH_RSCODE_235_187`
:   Reed Solomon code (235,187).

`ATSCMH_RSCODE_RES`
:   Reserved. Shouldn’t be used.

enum fecap\_scale\_params
:   scale types for the quality parameters.

**Constants**

`FE_SCALE_NOT_AVAILABLE`
:   That QoS measure is not available. That
    could indicate a temporary or a permanent
    condition.

`FE_SCALE_DECIBEL`
:   The scale is measured in 0.001 dB steps, typically
    used on signal measures.

`FE_SCALE_RELATIVE`
:   The scale is a relative percentual measure,
    ranging from 0 (0%) to 0xffff (100%).

`FE_SCALE_COUNTER`
:   The scale counts the occurrence of an event, like
    bit error, block error, lapsed time.

struct dtv\_stats
:   Used for reading a DTV status property

**Definition**:

```
struct dtv_stats {
    __u8 scale;
    union {
        __u64 uvalue;
        __s64 svalue;
    };
};
```

**Members**

`scale`
:   Filled with [`enum fecap_scale_params`](#c.fecap_scale_params "fecap_scale_params") - the scale in usage
    for that parameter

`{unnamed_union}`
:   anonymous

`uvalue`
:   unsigned integer value of the measure, used when **scale** is
    either `FE_SCALE_RELATIVE` or `FE_SCALE_COUNTER`.

`svalue`
:   integer value of the measure, for `FE_SCALE_DECIBEL`,
    used for dB measures. The unit is 0.001 dB.

**Description**

For most delivery systems, this will return a single value for each
parameter.

It should be noticed, however, that new OFDM delivery systems like
ISDB can use different modulation types for each group of carriers.
On such standards, up to 8 groups of statistics can be provided, one
for each carrier group (called “layer” on ISDB).

In order to be consistent with other delivery systems, the first
value refers to the entire set of carriers (“global”).

**scale** should use the value `FE_SCALE_NOT_AVAILABLE` when
the value for the entire group of carriers or from one specific layer
is not provided by the hardware.

**len** should be filled with the latest filled status + 1.

In other words, for ISDB, those values should be filled like:

```
u.st.stat.svalue[0] = global statistics;
u.st.stat.scale[0] = FE_SCALE_DECIBEL;
u.st.stat.value[1] = layer A statistics;
u.st.stat.scale[1] = FE_SCALE_NOT_AVAILABLE (if not available);
u.st.stat.svalue[2] = layer B statistics;
u.st.stat.scale[2] = FE_SCALE_DECIBEL;
u.st.stat.svalue[3] = layer C statistics;
u.st.stat.scale[3] = FE_SCALE_DECIBEL;
u.st.len = 4;
```

struct dtv\_fe\_stats
:   store Digital TV frontend statistics

**Definition**:

```
struct dtv_fe_stats {
    __u8 len;
    struct dtv_stats stat[MAX_DTV_STATS];
};
```

**Members**

`len`
:   length of the statistics - if zero, stats is disabled.

`stat`
:   array with digital TV statistics.

**Description**

On most standards, **len** can either be 0 or 1. However, for ISDB, each
layer is modulated in separate. So, each layer may have its own set
of statistics. If so, stat[0] carries on a global value for the property.
Indexes 1 to 3 means layer A to B.

struct dtv\_property
:   store one of frontend command and its value

**Definition**:

```
struct dtv_property {
    __u32 cmd;
    __u32 reserved[3];
    union {
        __u32 data;
        struct dtv_fe_stats st;
        struct {
            __u8 data[32];
            __u32 len;
            __u32 reserved1[3];
            void *reserved2;
        } buffer;
    } u;
    int result;
};
```

**Members**

`cmd`
:   Digital TV command.

`reserved`
:   Not used.

`u`
:   Union with the values for the command.

`u.data`
:   A unsigned 32 bits integer with command value.

`u.st`
:   a [`struct dtv_fe_stats`](#c.dtv_fe_stats "dtv_fe_stats") array of statistics.

`u.buffer`
:   Struct to store bigger properties.
    Currently unused.

`u.buffer.data`
:   an unsigned 32-bits array.

`u.buffer.len`
:   number of elements of the buffer.

`u.buffer.reserved1`
:   Reserved.

`u.buffer.reserved2`
:   Reserved.

`result`
:   Currently unused.

struct dtv\_properties
:   a set of command/value pairs.

**Definition**:

```
struct dtv_properties {
    __u32 num;
    struct dtv_property *props;
};
```

**Members**

`num`
:   amount of commands stored at the struct.

`props`
:   a pointer to [`struct dtv_property`](#c.dtv_property "dtv_property").
