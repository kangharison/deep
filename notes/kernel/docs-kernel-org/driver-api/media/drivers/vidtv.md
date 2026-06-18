# 10.2.3.vidtv: Virtual Digital TV driver

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/vidtv.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.2.3. vidtv: Virtual Digital TV driver

Author: Daniel W. S. Almeida <[dwlsalmeida@gmail.com](mailto:dwlsalmeida%40gmail.com)>, June 2020.

## 10.2.3.1. Background

Vidtv is a virtual DVB driver that aims to serve as a reference for driver
writers by serving as a template. It also validates the existing media DVB
APIs, thus helping userspace application writers.

Currently, it consists of:

* A fake tuner driver, which will report a bad signal quality if the chosen
  frequency is too far away from a table of valid frequencies for a
  particular delivery system.
* A fake demod driver, which will constantly poll the fake signal quality
  returned by the tuner, simulating a device that can lose/reacquire a lock
  on the signal depending on the CNR levels.
* A fake bridge driver, which is the module responsible for modprobing the
  fake tuner and demod modules and implementing the demux logic. This module
  takes parameters at initialization that will dictate how the simulation
  behaves.
* Code responsible for encoding a valid MPEG Transport Stream, which is then
  passed to the bridge driver. This fake stream contains some hardcoded content.
  For now, we have a single, audio-only channel containing a single MPEG
  Elementary Stream, which in turn contains a SMPTE 302m encoded sine-wave.
  Note that this particular encoder was chosen because it is the easiest
  way to encode PCM audio data in a MPEG Transport Stream.

## 10.2.3.2. Building vidtv

vidtv is a test driver and thus is **not** enabled by default when
compiling the kernel.

In order to enable compilation of vidtv:

* Enable **DVB\_TEST\_DRIVERS**, then
* Enable **DVB\_VIDTV**

When compiled as a module, expect the following .ko files:

* dvb\_vidtv\_tuner.ko
* dvb\_vidtv\_demod.ko
* dvb\_vidtv\_bridge.ko

## 10.2.3.3. Running vidtv

When compiled as a module, run:

```
modprobe vidtv
```

That’s it! The bridge driver will initialize the tuner and demod drivers as
part of its own initialization.

By default, it will accept the following frequencies:

> * 474 MHz for DVB-T/T2/C;
> * 11,362 GHz for DVB-S/S2.

For satellite systems, the driver simulates an universal extended
LNBf, with frequencies at Ku-Band, ranging from 10.7 GHz to 12.75 GHz.

You can optionally define some command-line arguments to vidtv.

## 10.2.3.4. Command-line arguments to vidtv

Below is a list of all arguments that can be supplied to vidtv:

drop\_tslock\_prob\_on\_low\_snr
:   Probability of losing the TS lock if the signal quality is bad.
    This probability be used by the fake demodulator driver to
    eventually return a status of 0 when the signal quality is not
    good.

recover\_tslock\_prob\_on\_good\_snr:
:   Probability recovering the TS lock when the signal improves. This
    probability be used by the fake demodulator driver to eventually
    return a status of 0x1f when/if the signal quality improves.

mock\_power\_up\_delay\_msec
:   Simulate a power up delay. Default: 0.

mock\_tune\_delay\_msec
:   Simulate a tune delay. Default 0.

vidtv\_valid\_dvb\_t\_freqs
:   Valid DVB-T frequencies to simulate, in Hz.

vidtv\_valid\_dvb\_c\_freqs
:   Valid DVB-C frequencies to simulate, in Hz.

vidtv\_valid\_dvb\_s\_freqs
:   Valid DVB-S/S2 frequencies to simulate at Ku-Band, in kHz.

max\_frequency\_shift\_hz,
:   Maximum shift in HZ allowed when tuning in a channel.

si\_period\_msec
:   How often to send SI packets. Default: 40ms.

pcr\_period\_msec
:   How often to send PCR packets. Default: 40ms.

mux\_rate\_kbytes\_sec
:   Attempt to maintain this bit rate by inserting TS null packets, if
    necessary. Default: 4096.

pcr\_pid,
:   PCR PID for all channels. Default: 0x200.

mux\_buf\_sz\_pkts,
:   Size for the mux buffer in multiples of 188 bytes.

## 10.2.3.5. vidtv internal structure

The kernel modules are split in the following way:

vidtv\_tuner.[ch]
:   Implements a fake tuner DVB driver.

vidtv\_demod.[ch]
:   Implements a fake demodulator DVB driver.

vidtv\_bridge.[ch]
:   Implements a bridge driver.

The MPEG related code is split in the following way:

vidtv\_ts.[ch]
:   Code to work with MPEG TS packets, such as TS headers, adaptation
    fields, PCR packets and NULL packets.

vidtv\_psi.[ch]
:   This is the PSI generator. PSI packets contain general information
    about a MPEG Transport Stream. A PSI generator is needed so
    userspace apps can retrieve information about the Transport Stream
    and eventually tune into a (dummy) channel.

    Because the generator is implemented in a separate file, it can be
    reused elsewhere in the media subsystem.

    Currently vidtv supports working with 5 PSI tables: PAT, PMT,
    SDT, NIT and EIT.

    The specification for PAT and PMT can be found in *ISO 13818-1:
    Systems*, while the specification for the SDT, NIT, EIT can be found in *ETSI
    EN 300 468: Specification for Service Information (SI) in DVB
    systems*.

    It isn’t strictly necessary, but using a real TS file helps when
    debugging PSI tables. Vidtv currently tries to replicate the PSI
    structure found in this file: [TS1Globo.ts](https://tsduck.io/streams/brazil-isdb-tb/TS1globo.ts).

    A good way to visualize the structure of streams is by using
    [DVBInspector](https://sourceforge.net/projects/dvbinspector/).

vidtv\_pes.[ch]
:   Implements the PES logic to convert encoder data into MPEG TS
    packets. These can then be fed into a TS multiplexer and eventually
    into userspace.

vidtv\_encoder.h
:   An interface for vidtv encoders. New encoders can be added to this
    driver by implementing the calls in this file.

vidtv\_s302m.[ch]
:   Implements a S302M encoder to make it possible to insert PCM audio
    data in the generated MPEG Transport Stream. The relevant
    specification is available online as *SMPTE 302M-2007: Television -
    Mapping of AES3 Data into MPEG-2 Transport Stream*.

    The resulting MPEG Elementary Stream is conveyed in a private
    stream with a S302M registration descriptor attached.

    This shall enable passing an audio signal into userspace so it can
    be decoded and played by media software. The corresponding decoder
    in ffmpeg is located in ‘libavcodec/s302m.c’ and is experimental.

vidtv\_channel.[ch]
:   Implements a ‘channel’ abstraction.

    When vidtv boots, it will create some hardcoded channels:

    1. Their services will be concatenated to populate the SDT.
    2. Their programs will be concatenated to populate the PAT
    3. Their events will be concatenated to populate the EIT
    4. For each program in the PAT, a PMT section will be created
    5. The PMT section for a channel will be assigned its streams.
    6. Every stream will have its corresponding encoder polled in a
       loop to produce TS packets.
       These packets may be interleaved by the muxer and then delivered
       to the bridge.

vidtv\_mux.[ch]
:   Implements a MPEG TS mux, loosely based on the ffmpeg
    implementation in “libavcodec/mpegtsenc.c”

    The muxer runs a loop which is responsible for:

    1. Keeping track of the amount of time elapsed since the last
       iteration.
    2. Polling encoders in order to fetch ‘elapsed\_time’ worth of data.
    3. Inserting PSI and/or PCR packets, if needed.
    4. Padding the resulting stream with NULL packets if
       necessary in order to maintain the chosen bit rate.
    5. Delivering the resulting TS packets to the bridge
       driver so it can pass them to the demux.

## 10.2.3.6. Testing vidtv with v4l-utils

Using the tools in v4l-utils is a great way to test and inspect the output of
vidtv. It is hosted here: [v4l-utils Documentation](https://linuxtv.org/wiki/index.php/V4l-utils).

From its webpage:

```
The v4l-utils are a series of packages for handling media devices.

It is hosted at http://git.linuxtv.org/v4l-utils.git, and packaged
on most distributions.

It provides a series of libraries and utilities to be used to
control several aspect of the media boards.
```

Start by installing v4l-utils and then modprobing vidtv:

```
modprobe dvb_vidtv_bridge
```

If the driver is OK, it should load and its probing code will run. This will
pull in the tuner and demod drivers.

### 10.2.3.6.1. Using dvb-fe-tool

The first step to check whether the demod loaded successfully is to run:

```
$ dvb-fe-tool
Device Dummy demod for DVB-T/T2/C/S/S2 (/dev/dvb/adapter0/frontend0) capabilities:
    CAN_FEC_1_2
    CAN_FEC_2_3
    CAN_FEC_3_4
    CAN_FEC_4_5
    CAN_FEC_5_6
    CAN_FEC_6_7
    CAN_FEC_7_8
    CAN_FEC_8_9
    CAN_FEC_AUTO
    CAN_GUARD_INTERVAL_AUTO
    CAN_HIERARCHY_AUTO
    CAN_INVERSION_AUTO
    CAN_QAM_16
    CAN_QAM_32
    CAN_QAM_64
    CAN_QAM_128
    CAN_QAM_256
    CAN_QAM_AUTO
    CAN_QPSK
    CAN_TRANSMISSION_MODE_AUTO
DVB API Version 5.11, Current v5 delivery system: DVBC/ANNEX_A
Supported delivery systems:
    DVBT
    DVBT2
    [DVBC/ANNEX_A]
    DVBS
    DVBS2
Frequency range for the current standard:
From:            51.0 MHz
To:              2.15 GHz
Step:            62.5 kHz
Tolerance:       29.5 MHz
Symbol rate ranges for the current standard:
From:            1.00 MBauds
To:              45.0 MBauds
```

This should return what is currently set up at the demod struct, i.e.:

```
static const struct dvb_frontend_ops vidtv_demod_ops = {
        .delsys = {
                SYS_DVBT,
                SYS_DVBT2,
                SYS_DVBC_ANNEX_A,
                SYS_DVBS,
                SYS_DVBS2,
        },

        .info = {
                .name                   = "Dummy demod for DVB-T/T2/C/S/S2",
                .frequency_min_hz       = 51 * MHz,
                .frequency_max_hz       = 2150 * MHz,
                .frequency_stepsize_hz  = 62500,
                .frequency_tolerance_hz = 29500 * kHz,
                .symbol_rate_min        = 1000000,
                .symbol_rate_max        = 45000000,

                .caps = FE_CAN_FEC_1_2 |
                        FE_CAN_FEC_2_3 |
                        FE_CAN_FEC_3_4 |
                        FE_CAN_FEC_4_5 |
                        FE_CAN_FEC_5_6 |
                        FE_CAN_FEC_6_7 |
                        FE_CAN_FEC_7_8 |
                        FE_CAN_FEC_8_9 |
                        FE_CAN_QAM_16 |
                        FE_CAN_QAM_64 |
                        FE_CAN_QAM_32 |
                        FE_CAN_QAM_128 |
                        FE_CAN_QAM_256 |
                        FE_CAN_QAM_AUTO |
                        FE_CAN_QPSK |
                        FE_CAN_FEC_AUTO |
                        FE_CAN_INVERSION_AUTO |
                        FE_CAN_TRANSMISSION_MODE_AUTO |
                        FE_CAN_GUARD_INTERVAL_AUTO |
                        FE_CAN_HIERARCHY_AUTO,
        }

        ....
```

For more information on dvb-fe-tools check its online documentation here:
[dvb-fe-tool Documentation](https://www.linuxtv.org/wiki/index.php/Dvb-fe-tool).

### 10.2.3.6.2. Using dvb-scan

In order to tune into a channel and read the PSI tables, we can use dvb-scan.

For this, one should provide a configuration file known as a ‘scan file’,
here’s an example:

```
[Channel]
FREQUENCY = 474000000
MODULATION = QAM/AUTO
SYMBOL_RATE = 6940000
INNER_FEC = AUTO
DELIVERY_SYSTEM = DVBC/ANNEX_A
```

Note

The parameters depend on the video standard you’re testing.

Note

Vidtv is a fake driver and does not validate much of the information
in the scan file. Just specifying ‘FREQUENCY’ and ‘DELIVERY\_SYSTEM’
should be enough for DVB-T/DVB-T2. For DVB-S/DVB-C however, you
should also provide ‘SYMBOL\_RATE’.

You can browse scan tables online here: [dvb-scan-tables](https://git.linuxtv.org/dtv-scan-tables.git).

Assuming this channel is named ‘channel.conf’, you can then run:

```
$ dvbv5-scan channel.conf
dvbv5-scan ~/vidtv.conf
ERROR    command BANDWIDTH_HZ (5) not found during retrieve
Cannot calc frequency shift. Either bandwidth/symbol-rate is unavailable (yet).
Scanning frequency #1 330000000
    (0x00) Signal= -68.00dBm
Scanning frequency #2 474000000
Lock   (0x1f) Signal= -34.45dBm C/N= 33.74dB UCB= 0
Service Beethoven, provider LinuxTV.org: digital television
```

For more information on dvb-scan, check its documentation online here:
[dvb-scan Documentation](https://www.linuxtv.org/wiki/index.php/Dvbscan).

### 10.2.3.6.3. Using dvb-zap

dvbv5-zap is a command line tool that can be used to record MPEG-TS to disk. The
typical use is to tune into a channel and put it into record mode. The example
below - which is taken from the documentation - illustrates that[[1]](#id2):

```
$ dvbv5-zap -c dvb_channel.conf "beethoven" -o music.ts -P -t 10
using demux 'dvb0.demux0'
reading channels from file 'dvb_channel.conf'
tuning to 474000000 Hz
pass all PID's to TS
dvb_set_pesfilter 8192
dvb_dev_set_bufsize: buffer set to 6160384
Lock   (0x1f) Quality= Good Signal= -34.66dBm C/N= 33.41dB UCB= 0 postBER= 0 preBER= 1.05x10^-3 PER= 0
Lock   (0x1f) Quality= Good Signal= -34.57dBm C/N= 33.46dB UCB= 0 postBER= 0 preBER= 1.05x10^-3 PER= 0
Record to file 'music.ts' started
received 24587768 bytes (2401 Kbytes/sec)
Lock   (0x1f) Quality= Good Signal= -34.42dBm C/N= 33.89dB UCB= 0 postBER= 0 preBER= 2.44x10^-3 PER= 0
```

[[1](#id1)]

In this example, it records 10 seconds with all program ID’s stored
at the music.ts file.

The channel can be watched by playing the contents of the stream with some
player that recognizes the MPEG-TS format, such as `mplayer` or `vlc`.

By playing the contents of the stream one can visually inspect the workings of
vidtv, e.g., to play a recorded TS file with:

```
$ mplayer music.ts
```

or, alternatively, running this command on one terminal:

```
$ dvbv5-zap -c dvb_channel.conf "beethoven" -P -r &
```

And, on a second terminal, playing the contents from DVR interface with:

```
$ mplayer /dev/dvb/adapter0/dvr0
```

For more information on dvb-zap check its online documentation here:
[dvb-zap Documentation](https://www.linuxtv.org/wiki/index.php/Dvbv5-zap).
See also: [zap](https://www.linuxtv.org/wiki/index.php/Zap).

## 10.2.3.7. What can still be improved in vidtv

### 10.2.3.7.1. Add *debugfs* integration

Although frontend drivers provide DVBv5 statistics via the .read\_status
call, a nice addition would be to make additional statistics available to
userspace via debugfs, which is a simple-to-use, RAM-based filesystem
specifically designed for debug purposes.

The logic for this would be implemented on a separate file so as not to
pollute the frontend driver. These statistics are driver-specific and can
be useful during tests.

The Siano driver is one example of a driver using
debugfs to convey driver-specific statistics to userspace and it can be
used as a reference.

This should be further enabled and disabled via a Kconfig
option for convenience.

### 10.2.3.7.2. Add a way to test video

Currently, vidtv can only encode PCM audio. It would be great to implement
a barebones version of MPEG-2 video encoding so we can also test video. The
first place to look into is *ISO 13818-2: Information technology — Generic
coding of moving pictures and associated audio information — Part 2: Video*,
which covers the encoding of compressed video in MPEG Transport Streams.

This might optionally use the Video4Linux2 Test Pattern Generator, v4l2-tpg,
which resides at:

```
drivers/media/common/v4l2-tpg/
```

### 10.2.3.7.3. Add white noise simulation

The vidtv tuner already has code to identify whether the chosen frequency
is too far away from a table of valid frequencies. For now, this means that
the demodulator can eventually lose the lock on the signal, since the tuner will
report a bad signal quality.

A nice addition is to simulate some noise when the signal quality is bad by:

* Randomly dropping some TS packets. This will trigger a continuity error if the
  continuity counter is updated but the packet is not passed on to the demux.
* Updating the error statistics accordingly (e.g. BER, etc).
* Simulating some noise in the encoded data.

## 10.2.3.8. Functions and structs used within vidtv

struct vidtv\_dvb
:   Vidtv bridge state

**Definition**:

```
struct vidtv_dvb {
    struct platform_device *pdev;
    struct dvb_frontend *fe[NUM_FE];
    struct dvb_adapter adapter;
    struct dvb_demux demux;
    struct dmxdev dmx_dev;
    struct dmx_frontend dmx_fe[NUM_FE];
    struct i2c_adapter i2c_adapter;
    struct i2c_client *i2c_client_demod[NUM_FE];
    struct i2c_client *i2c_client_tuner[NUM_FE];
    u32 nfeeds;
    struct mutex feed_lock;
    bool streaming;
    struct vidtv_mux *mux;
#ifdef CONFIG_MEDIA_CONTROLLER_DVB;
    struct media_device mdev;
#endif ;
};
```

**Members**

`pdev`
:   The platform device. Obtained when the bridge is probed.

`fe`
:   The frontends. Obtained when probing the demodulator modules.

`adapter`
:   Represents a DTV adapter. See ‘dvb\_register\_adapter’.

`demux`
:   The demux used by the [`dvb_dmx_swfilter_packets()`](../dtv-demux.html#c.dvb_dmx_swfilter_packets "dvb_dmx_swfilter_packets") call.

`dmx_dev`
:   Represents a demux device.

`dmx_fe`
:   The frontends associated with the demux.

`i2c_adapter`
:   The i2c\_adapter associated with the bridge driver.

`i2c_client_demod`
:   The i2c\_clients associated with the demodulator modules.

`i2c_client_tuner`
:   The i2c\_clients associated with the tuner modules.

`nfeeds`
:   The number of feeds active.

`feed_lock`
:   Protects access to the start/stop stream logic/data.

`streaming`
:   Whether we are streaming now.

`mux`
:   The abstraction responsible for delivering MPEG TS packets to the bridge.

`mdev`
:   The media\_device struct for media controller support.

struct vidtv\_channel
:   A ‘channel’ abstraction

**Definition**:

```
struct vidtv_channel {
    char *name;
    u16 transport_stream_id;
    struct vidtv_psi_table_sdt_service *service;
    u16 program_num;
    struct vidtv_psi_table_pat_program *program;
    struct vidtv_psi_table_pmt_stream *streams;
    struct vidtv_encoder *encoders;
    struct vidtv_psi_table_eit_event *events;
    struct vidtv_channel *next;
};
```

**Members**

`name`
:   name of the channel

`transport_stream_id`
:   a number to identify the TS, chosen at will.

`service`
:   A \_single\_ service. Will be concatenated into the SDT.

`program_num`
:   The link between PAT, PMT and SDT.

`program`
:   A \_single\_ program with one or more streams associated with it.
    Will be concatenated into the PAT.

`streams`
:   A stream loop used to populate the PMT section for ‘program’

`encoders`
:   A encoder loop. There must be one encoder for each stream.

`events`
:   Optional event information. This will feed into the EIT.

`next`
:   Optionally chain this channel.

**Description**

When vidtv boots, it will create some hardcoded channels.
Their services will be concatenated to populate the SDT.
Their programs will be concatenated to populate the PAT
For each program in the PAT, a PMT section will be created
The PMT section for a channel will be assigned its streams.
Every stream will have its corresponding encoder polled to produce TS packets
These packets may be interleaved by the mux and then delivered to the bridge

int vidtv\_channel\_si\_init(struct [vidtv\_mux](#c.vidtv_mux "vidtv_mux") \*m)
:   Init the PSI tables from the channels in the mux

**Parameters**

`struct vidtv_mux *m`
:   The mux containing the channels.

int vidtv\_channels\_init(struct [vidtv\_mux](#c.vidtv_mux "vidtv_mux") \*m)
:   Init hardcoded, fake ‘channels’.

**Parameters**

`struct vidtv_mux *m`
:   The mux to store the channels into.

struct vidtv\_demod\_cnr\_to\_qual\_s
:   Map CNR values to a given combination of modulation and fec\_inner

**Definition**:

```
struct vidtv_demod_cnr_to_qual_s {
    u32 modulation;
    u32 fec;
    u32 cnr_ok;
    u32 cnr_good;
};
```

**Members**

`modulation`
:   see [`enum fe_modulation`](../../../userspace-api/media/dvb/frontend-header.html#c.fe_modulation "fe_modulation")

`fec`
:   see `enum fe_fec_rate`

`cnr_ok`
:   S/N threshold to consider the signal as OK. Below that, there’s
    a chance of losing sync.

`cnr_good`
:   S/N threshold to consider the signal strong.

**Description**

This `struct matches` values for ‘good’ and ‘ok’ CNRs given the combination
of modulation and fec\_inner in use. We might simulate some noise if the
signal quality is not too good.

The values were taken from libdvbv5.

struct vidtv\_demod\_config
:   Configuration used to init the demod

**Definition**:

```
struct vidtv_demod_config {
    u8 drop_tslock_prob_on_low_snr;
    u8 recover_tslock_prob_on_good_snr;
};
```

**Members**

`drop_tslock_prob_on_low_snr`
:   probability of losing the lock due to low snr

`recover_tslock_prob_on_good_snr`
:   probability of recovering when the signal
    improves

**Description**

The configuration used to init the demodulator module, usually filled
by a bridge driver. For vidtv, this is filled by vidtv\_bridge before the
demodulator module is probed.

struct vidtv\_demod\_state
:   The demodulator state

**Definition**:

```
struct vidtv_demod_state {
    struct dvb_frontend frontend;
    struct vidtv_demod_config config;
    enum fe_status status;
    u16 tuner_cnr;
};
```

**Members**

`frontend`
:   The frontend structure allocated by the demod.

`config`
:   The config used to init the demod.

`status`
:   the demod status.

`tuner_cnr`
:   current S/N ratio for the signal carrier

struct vidtv\_encoder
:   A generic encoder type.

**Definition**:

```
struct vidtv_encoder {
    enum vidtv_encoder_id id;
    char *name;
    u8 *encoder_buf;
    u32 encoder_buf_sz;
    u32 encoder_buf_offset;
    u64 sample_count;
    struct vidtv_access_unit *access_units;
    void *src_buf;
    u32 src_buf_sz;
    u32 src_buf_offset;
    bool is_video_encoder;
    void *ctx;
    __be16 stream_id;
    __be16 es_pid;
    void *(*encode)(struct vidtv_encoder *e);
    u32 (*clear)(struct vidtv_encoder *e);
    struct vidtv_encoder *sync;
    u32 sampling_rate_hz;
    void (*last_sample_cb)(u32 sample_no);
    void (*destroy)(struct vidtv_encoder *e);
    struct vidtv_encoder *next;
};
```

**Members**

`id`
:   So we can cast to a concrete implementation when needed.

`name`
:   Usually the same as the stream name.

`encoder_buf`
:   The encoder internal buffer for the access units.

`encoder_buf_sz`
:   The encoder buffer size, in bytes

`encoder_buf_offset`
:   Our byte position in the encoder buffer.

`sample_count`
:   How many samples we have encoded in total.

`access_units`
:   encoder payload units, used for clock references

`src_buf`
:   The source of raw data to be encoded, encoder might set a
    default if null.

`src_buf_sz`
:   size of **src\_buf**.

`src_buf_offset`
:   Our position in the source buffer.

`is_video_encoder`
:   Whether this a video encoder (as opposed to audio)

`ctx`
:   Encoder-specific state.

`stream_id`
:   Examples: Audio streams (0xc0-0xdf), Video streams
    (0xe0-0xef).

`es_pid`
:   The TS PID to use for the elementary stream in this encoder.

`encode`
:   Prepare enough AUs for the given amount of time.

`clear`
:   Clear the encoder output.

`sync`
:   Attempt to synchronize with this encoder.

`sampling_rate_hz`
:   The sampling rate (or fps, if video) used.

`last_sample_cb`
:   Called when the encoder runs out of data.This is
    so the source can read data in a
    piecemeal fashion instead of having to
    provide it all at once.

`destroy`
:   Destroy this encoder, freeing allocated resources.

`next`
:   Next in the chain

struct vidtv\_mux\_timing
:   Timing related information

**Definition**:

```
struct vidtv_mux_timing {
    u64 start_jiffies;
    u64 current_jiffies;
    u64 past_jiffies;
    u64 clk;
    u64 pcr_period_usecs;
    u64 si_period_usecs;
};
```

**Members**

`start_jiffies`
:   The value of ‘jiffies’ when we started the mux thread.

`current_jiffies`
:   The value of ‘jiffies’ for the current iteration.

`past_jiffies`
:   The value of ‘jiffies’ for the past iteration.

`clk`
:   A 27Mhz clock from which we will drive the PCR. Updated proportionally
    on every iteration.

`pcr_period_usecs`
:   How often we should send PCR packets.

`si_period_usecs`
:   How often we should send PSI packets.

**Description**

This is used to decide when PCR or PSI packets should be sent. This will also
provide storage for the clock, which is used to compute the value for the PCR.

struct vidtv\_mux\_si
:   Store the PSI context.

**Definition**:

```
struct vidtv_mux_si {
    struct vidtv_psi_table_pat *pat;
    struct vidtv_psi_table_pmt **pmt_secs;
    struct vidtv_psi_table_sdt *sdt;
    struct vidtv_psi_table_nit *nit;
    struct vidtv_psi_table_eit *eit;
};
```

**Members**

`pat`
:   The PAT in use by the muxer.

`pmt_secs`
:   The PMT sections in use by the muxer. One for each program in the PAT.

`sdt`
:   The SDT in use by the muxer.

`nit`
:   The NIT in use by the muxer.

`eit`
:   the EIT in use by the muxer.

**Description**

This is used to store the PAT, PMT sections and SDT in use by the muxer.

The muxer acquire these by looking into the hardcoded channels in
vidtv\_channel and then periodically sends the TS packets for them>

struct vidtv\_mux\_pid\_ctx
:   Store the context for a given TS PID.

**Definition**:

```
struct vidtv_mux_pid_ctx {
    u16 pid;
    u8 cc;
    struct hlist_node h;
};
```

**Members**

`pid`
:   The TS PID.

`cc`
:   The continuity counter for this PID. It is incremented on every TS
    pack and it will wrap around at 0xf0. If the decoder notices a sudden jump in
    this counter this will trigger a discontinuity state.

`h`
:   This is embedded in a hash table, mapping pid -> vidtv\_mux\_pid\_ctx

struct vidtv\_mux
:   A muxer abstraction loosely based in libavcodec/mpegtsenc.c

**Definition**:

```
struct vidtv_mux {
    struct dvb_frontend *fe;
    struct device *dev;
    struct vidtv_mux_timing timing;
    u32 mux_rate_kbytes_sec;
    unsigned long pid_ctx[1 << (( 3) - 1)];
    void (*on_new_packets_available_cb)(void *priv, u8 *buf, u32 npackets);
    u8 *mux_buf;
    u32 mux_buf_sz;
    u32 mux_buf_offset;
    struct vidtv_channel  *channels;
    struct vidtv_mux_si si;
    u64 num_streamed_pcr;
    u64 num_streamed_si;
    struct work_struct mpeg_thread;
    bool streaming;
    u16 pcr_pid;
    u16 transport_stream_id;
    u16 network_id;
    char *network_name;
    void *priv;
};
```

**Members**

`fe`
:   The frontend structure allocated by the muxer.

`dev`
:   pointer to [`struct device`](../../infrastructure.html#c.device "device").

`timing`
:   Keeps track of timing related information.

`mux_rate_kbytes_sec`
:   The bit rate for the TS, in kbytes.

`pid_ctx`
:   A hash table to keep track of per-PID metadata.

`on_new_packets_available_cb`
:   A callback to inform of new TS packets ready.

`mux_buf`
:   A pointer to a buffer for this muxer. TS packets are stored there
    and then passed on to the bridge driver.

`mux_buf_sz`
:   The size for ‘mux\_buf’.

`mux_buf_offset`
:   The current offset into ‘mux\_buf’.

`channels`
:   The channels associated with this muxer.

`si`
:   Keeps track of the PSI context.

`num_streamed_pcr`
:   Number of PCR packets streamed.

`num_streamed_si`
:   The number of PSI packets streamed.

`mpeg_thread`
:   Thread responsible for the muxer loop.

`streaming`
:   whether ‘mpeg\_thread’ is running.

`pcr_pid`
:   The TS PID used for the PSI packets. All channels will share the
    same PCR.

`transport_stream_id`
:   The transport stream ID

`network_id`
:   The network ID

`network_name`
:   The network name

`priv`
:   Private data.

struct vidtv\_mux\_init\_args
:   Arguments used to inix the muxer.

**Definition**:

```
struct vidtv_mux_init_args {
    u32 mux_rate_kbytes_sec;
    void (*on_new_packets_available_cb)(void *priv, u8 *buf, u32 npackets);
    u32 mux_buf_sz;
    u64 pcr_period_usecs;
    u64 si_period_usecs;
    u16 pcr_pid;
    u16 transport_stream_id;
    struct vidtv_channel *channels;
    u16 network_id;
    char *network_name;
    void *priv;
};
```

**Members**

`mux_rate_kbytes_sec`
:   The bit rate for the TS, in kbytes.

`on_new_packets_available_cb`
:   A callback to inform of new TS packets ready.

`mux_buf_sz`
:   The size for ‘mux\_buf’.

`pcr_period_usecs`
:   How often we should send PCR packets.

`si_period_usecs`
:   How often we should send PSI packets.

`pcr_pid`
:   The TS PID used for the PSI packets. All channels will share the
    same PCR.

`transport_stream_id`
:   The transport stream ID

`channels`
:   an optional list of channels to use

`network_id`
:   The network ID

`network_name`
:   The network name

`priv`
:   Private data.

struct pes\_header\_write\_args
:   Arguments to write a PES header.

**Definition**:

```
struct pes_header_write_args {
    void *dest_buf;
    u32 dest_offset;
    u32 dest_buf_sz;
    u32 encoder_id;
    bool send_pts;
    u64 pts;
    bool send_dts;
    u64 dts;
    u16 stream_id;
    u32 n_pes_h_s_bytes;
    u32 access_unit_len;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`dest_offset`
:   where to start writing in the dest\_buffer.

`dest_buf_sz`
:   The size of the dest\_buffer

`encoder_id`
:   Encoder id (see vidtv\_encoder.h)

`send_pts`
:   Should we send PTS?

`pts`
:   PTS value to send.

`send_dts`
:   Should we send DTS?

`dts`
:   DTS value to send.

`stream_id`
:   The stream id to use. Ex: Audio streams (0xc0-0xdf), Video
    streams (0xe0-0xef).

`n_pes_h_s_bytes`
:   Padding bytes. Might be used by an encoder if needed, gets
    discarded by the decoder.

`access_unit_len`
:   The size of \_one\_ access unit (with any headers it might need)

struct pes\_ts\_header\_write\_args
:   Arguments to write a TS header.

**Definition**:

```
struct pes_ts_header_write_args {
    void *dest_buf;
    u32 dest_offset;
    u32 dest_buf_sz;
    u16 pid;
    u8 *continuity_counter;
    bool wrote_pes_header;
    u32 n_stuffing_bytes;
    u64 pcr;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`dest_offset`
:   where to start writing in the dest\_buffer.

`dest_buf_sz`
:   The size of the dest\_buffer

`pid`
:   The PID to use for the TS packets.

`continuity_counter`
:   Incremented on every new TS packet.

`wrote_pes_header`
:   Flag to indicate that the PES header was written

`n_stuffing_bytes`
:   Padding bytes. Might be used by an encoder if needed, gets
    discarded by the decoder.

`pcr`
:   counter driven by a 27Mhz clock.

struct pes\_write\_args
:   Arguments for the packetizer.

**Definition**:

```
struct pes_write_args {
    void *dest_buf;
    void *from;
    u32 access_unit_len;
    u32 dest_offset;
    u32 dest_buf_sz;
    u16 pid;
    u32 encoder_id;
    u8 *continuity_counter;
    u16 stream_id;
    bool send_pts;
    u64 pts;
    bool send_dts;
    u64 dts;
    u32 n_pes_h_s_bytes;
    u64 pcr;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`from`
:   A pointer to the encoder buffer containing one access unit.

`access_unit_len`
:   The size of \_one\_ access unit (with any headers it might need)

`dest_offset`
:   where to start writing in the dest\_buffer.

`dest_buf_sz`
:   The size of the dest\_buffer

`pid`
:   The PID to use for the TS packets.

`encoder_id`
:   Encoder id (see vidtv\_encoder.h)

`continuity_counter`
:   Incremented on every new TS packet.

`stream_id`
:   The stream id to use. Ex: Audio streams (0xc0-0xdf), Video
    streams (0xe0-0xef).

`send_pts`
:   Should we send PTS?

`pts`
:   PTS value to send.

`send_dts`
:   Should we send DTS?

`dts`
:   DTS value to send.

`n_pes_h_s_bytes`
:   Padding bytes. Might be used by an encoder if needed, gets
    discarded by the decoder.

`pcr`
:   counter driven by a 27Mhz clock.

u32 vidtv\_pes\_write\_into(struct [pes\_write\_args](#c.pes_write_args "pes_write_args") \*args)
:   Write a PES packet as MPEG-TS packets into a buffer.

**Parameters**

`struct pes_write_args *args`
:   The args to use when writing

**Description**

This function translate the ES data for one access unit
from an encoder into MPEG TS packets. It does so by first encapsulating it
with a PES header and then splitting it into TS packets.

The data is then written into the buffer pointed to by ‘args.buf’

**Return**

The number of bytes written into the buffer. This is usually NOT
equal to the size of the access unit, since we need space for PES headers, TS headers
and padding bytes, if any.

struct psi\_write\_args
:   Arguments for the PSI packetizer.

**Definition**:

```
struct psi_write_args {
    void *dest_buf;
    void *from;
    size_t len;
    u32 dest_offset;
    u16 pid;
    bool new_psi_section;
    u8 *continuity_counter;
    bool is_crc;
    u32 dest_buf_sz;
    u32 *crc;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`from`
:   PSI data to be copied.

`len`
:   How much to write.

`dest_offset`
:   where to start writing in the dest\_buffer.

`pid`
:   TS packet ID.

`new_psi_section`
:   Set when starting a table section.

`continuity_counter`
:   Incremented on every new packet.

`is_crc`
:   Set when writing the CRC at the end.

`dest_buf_sz`
:   The size of the dest\_buffer

`crc`
:   a pointer to store the crc for this chunk

struct desc\_write\_args
:   Arguments in order to write a descriptor.

**Definition**:

```
struct desc_write_args {
    void *dest_buf;
    u32 dest_offset;
    struct vidtv_psi_desc *desc;
    u16 pid;
    u8 *continuity_counter;
    u32 dest_buf_sz;
    u32 *crc;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`dest_offset`
:   where to start writing in the dest\_buffer.

`desc`
:   A pointer to the descriptor

`pid`
:   TS packet ID.

`continuity_counter`
:   Incremented on every new packet.

`dest_buf_sz`
:   The size of the dest\_buffer

`crc`
:   a pointer to store the crc for this chunk

struct crc32\_write\_args
:   Arguments in order to write the CRC at the end of the PSI tables.

**Definition**:

```
struct crc32_write_args {
    void *dest_buf;
    u32 dest_offset;
    __be32 crc;
    u16 pid;
    u8 *continuity_counter;
    u32 dest_buf_sz;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`dest_offset`
:   where to start writing in the dest\_buffer.

`crc`
:   the CRC value to write

`pid`
:   TS packet ID.

`continuity_counter`
:   Incremented on every new packet.

`dest_buf_sz`
:   The size of the dest\_buffer

struct header\_write\_args
:   Arguments in order to write the common table header

**Definition**:

```
struct header_write_args {
    void *dest_buf;
    u32 dest_offset;
    struct vidtv_psi_table_header *h;
    u16 pid;
    u8 *continuity_counter;
    u32 dest_buf_sz;
    u32 *crc;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`dest_offset`
:   where to start writing in the dest\_buffer.

`h`
:   a pointer to the header.

`pid`
:   TS packet ID.

`continuity_counter`
:   Incremented on every new packet.

`dest_buf_sz`
:   The size of the dest\_buffer

`crc`
:   a pointer to store the crc for this chunk

void vidtv\_psi\_sdt\_service\_assign(struct vidtv\_psi\_table\_sdt \*sdt, struct vidtv\_psi\_table\_sdt\_service \*service)
:   Assigns the service loop to the SDT.

**Parameters**

`struct vidtv_psi_table_sdt *sdt`
:   The SDT to assign to.

`struct vidtv_psi_table_sdt_service *service`
:   The service loop (one or more services)

**Description**

This will free the previous service loop in the table.
This will assign ownership of the service loop to the table, i.e. the table
will free this service loop when a call to its destroy function is made.

void vidtv\_psi\_desc\_assign(struct vidtv\_psi\_desc \*\*to, struct vidtv\_psi\_desc \*desc)
:   Assigns a descriptor loop at some point

**Parameters**

`struct vidtv_psi_desc **to`
:   Where to assign this descriptor loop to

`struct vidtv_psi_desc *desc`
:   The descriptor loop that will be assigned.

**Description**

This will free the loop in ‘to’, if any.

void vidtv\_pmt\_desc\_assign(struct vidtv\_psi\_table\_pmt \*pmt, struct vidtv\_psi\_desc \*\*to, struct vidtv\_psi\_desc \*desc)
:   Assigns a descriptor loop at some point in a PMT section.

**Parameters**

`struct vidtv_psi_table_pmt *pmt`
:   The PMT section that will contain the descriptor loop

`struct vidtv_psi_desc **to`
:   Where in the PMT to assign this descriptor loop to

`struct vidtv_psi_desc *desc`
:   The descriptor loop that will be assigned.

**Description**

This will free the loop in ‘to’, if any.
This will assign ownership of the loop to the table, i.e. the table
will free this loop when a call to its destroy function is made.

void vidtv\_sdt\_desc\_assign(struct vidtv\_psi\_table\_sdt \*sdt, struct vidtv\_psi\_desc \*\*to, struct vidtv\_psi\_desc \*desc)
:   Assigns a descriptor loop at some point in a SDT.

**Parameters**

`struct vidtv_psi_table_sdt *sdt`
:   The SDT that will contain the descriptor loop

`struct vidtv_psi_desc **to`
:   Where in the PMT to assign this descriptor loop to

`struct vidtv_psi_desc *desc`
:   The descriptor loop that will be assigned.

**Description**

This will free the loop in ‘to’, if any.
This will assign ownership of the loop to the table, i.e. the table
will free this loop when a call to its destroy function is made.

void vidtv\_psi\_pat\_program\_assign(struct vidtv\_psi\_table\_pat \*pat, struct vidtv\_psi\_table\_pat\_program \*p)
:   Assigns the program loop to the PAT.

**Parameters**

`struct vidtv_psi_table_pat *pat`
:   The PAT to assign to.

`struct vidtv_psi_table_pat_program *p`
:   The program loop (one or more programs)

**Description**

This will free the previous program loop in the table.
This will assign ownership of the program loop to the table, i.e. the table
will free this program loop when a call to its destroy function is made.

void vidtv\_psi\_pmt\_stream\_assign(struct vidtv\_psi\_table\_pmt \*pmt, struct vidtv\_psi\_table\_pmt\_stream \*s)
:   Assigns the stream loop to the PAT.

**Parameters**

`struct vidtv_psi_table_pmt *pmt`
:   The PMT to assign to.

`struct vidtv_psi_table_pmt_stream *s`
:   The stream loop (one or more streams)

**Description**

This will free the previous stream loop in the table.
This will assign ownership of the stream loop to the table, i.e. the table
will free this stream loop when a call to its destroy function is made.

struct vidtv\_psi\_table\_pmt \*\*vidtv\_psi\_pmt\_create\_sec\_for\_each\_pat\_entry(struct vidtv\_psi\_table\_pat \*pat, u16 pcr\_pid)
:   Create a PMT section for each program found in the PAT

**Parameters**

`struct vidtv_psi_table_pat *pat`
:   The PAT to look for programs.

`u16 pcr_pid`
:   packet ID for the PCR to be used for the program described in this
    PMT section

u16 vidtv\_psi\_pmt\_get\_pid(struct vidtv\_psi\_table\_pmt \*section, struct vidtv\_psi\_table\_pat \*pat)
:   Get the TS PID for a PMT section.

**Parameters**

`struct vidtv_psi_table_pmt *section`
:   The PMT section whose PID we want to retrieve.

`struct vidtv_psi_table_pat *pat`
:   The PAT table to look into.

**Return**

the TS PID for ‘section’

void vidtv\_psi\_pat\_table\_update\_sec\_len(struct vidtv\_psi\_table\_pat \*pat)
:   Recompute and update the PAT section length.

**Parameters**

`struct vidtv_psi_table_pat *pat`
:   The PAT whose length is to be updated.

**Description**

This will traverse the table and accumulate the length of its components,
which is then used to replace the ‘section\_length’ field.

If section\_length > MAX\_SECTION\_LEN, the operation fails.

void vidtv\_psi\_pmt\_table\_update\_sec\_len(struct vidtv\_psi\_table\_pmt \*pmt)
:   Recompute and update the PMT section length.

**Parameters**

`struct vidtv_psi_table_pmt *pmt`
:   The PMT whose length is to be updated.

**Description**

This will traverse the table and accumulate the length of its components,
which is then used to replace the ‘section\_length’ field.

If section\_length > MAX\_SECTION\_LEN, the operation fails.

void vidtv\_psi\_sdt\_table\_update\_sec\_len(struct vidtv\_psi\_table\_sdt \*sdt)
:   Recompute and update the SDT section length.

**Parameters**

`struct vidtv_psi_table_sdt *sdt`
:   The SDT whose length is to be updated.

**Description**

This will traverse the table and accumulate the length of its components,
which is then used to replace the ‘section\_length’ field.

If section\_length > MAX\_SECTION\_LEN, the operation fails.

struct vidtv\_psi\_pat\_write\_args
:   Arguments for writing a PAT table

**Definition**:

```
struct vidtv_psi_pat_write_args {
    char *buf;
    u32 offset;
    struct vidtv_psi_table_pat *pat;
    u32 buf_sz;
    u8 *continuity_counter;
};
```

**Members**

`buf`
:   The destination buffer.

`offset`
:   The offset into the destination buffer.

`pat`
:   A pointer to the PAT.

`buf_sz`
:   The size of the destination buffer.

`continuity_counter`
:   A pointer to the CC. Incremented on every new packet.

u32 vidtv\_psi\_pat\_write\_into(struct [vidtv\_psi\_pat\_write\_args](#c.vidtv_psi_pat_write_args "vidtv_psi_pat_write_args") \*args)
:   Write PAT as MPEG-TS packets into a buffer.

**Parameters**

`struct vidtv_psi_pat_write_args *args`
:   An instance of [`struct vidtv_psi_pat_write_args`](#c.vidtv_psi_pat_write_args "vidtv_psi_pat_write_args")

**Description**

This function writes the MPEG TS packets for a PAT table into a buffer.
Calling code will usually generate the PAT via a call to its init function
and thus is responsible for freeing it.

**Return**

The number of bytes written into the buffer. This is NOT
equal to the size of the PAT, since more space is needed for TS headers during TS
encapsulation.

struct vidtv\_psi\_sdt\_write\_args
:   Arguments for writing a SDT table

**Definition**:

```
struct vidtv_psi_sdt_write_args {
    char *buf;
    u32 offset;
    struct vidtv_psi_table_sdt *sdt;
    u32 buf_sz;
    u8 *continuity_counter;
};
```

**Members**

`buf`
:   The destination buffer.

`offset`
:   The offset into the destination buffer.

`sdt`
:   A pointer to the SDT.

`buf_sz`
:   The size of the destination buffer.

`continuity_counter`
:   A pointer to the CC. Incremented on every new packet.

u32 vidtv\_psi\_sdt\_write\_into(struct [vidtv\_psi\_sdt\_write\_args](#c.vidtv_psi_sdt_write_args "vidtv_psi_sdt_write_args") \*args)
:   Write SDT as MPEG-TS packets into a buffer.

**Parameters**

`struct vidtv_psi_sdt_write_args *args`
:   an instance of [`struct vidtv_psi_sdt_write_args`](#c.vidtv_psi_sdt_write_args "vidtv_psi_sdt_write_args")

**Description**

This function writes the MPEG TS packets for a SDT table into a buffer.
Calling code will usually generate the SDT via a call to its init function
and thus is responsible for freeing it.

**Return**

The number of bytes written into the buffer. This is NOT
equal to the size of the SDT, since more space is needed for TS headers during TS
encapsulation.

struct vidtv\_psi\_pmt\_write\_args
:   Arguments for writing a PMT section

**Definition**:

```
struct vidtv_psi_pmt_write_args {
    char *buf;
    u32 offset;
    struct vidtv_psi_table_pmt *pmt;
    u16 pid;
    u32 buf_sz;
    u8 *continuity_counter;
    u16 pcr_pid;
};
```

**Members**

`buf`
:   The destination buffer.

`offset`
:   The offset into the destination buffer.

`pmt`
:   A pointer to the PMT.

`pid`
:   Program ID

`buf_sz`
:   The size of the destination buffer.

`continuity_counter`
:   A pointer to the CC. Incremented on every new packet.

`pcr_pid`
:   The TS PID used for the PSI packets. All channels will share the
    same PCR.

u32 vidtv\_psi\_pmt\_write\_into(struct [vidtv\_psi\_pmt\_write\_args](#c.vidtv_psi_pmt_write_args "vidtv_psi_pmt_write_args") \*args)
:   Write PMT as MPEG-TS packets into a buffer.

**Parameters**

`struct vidtv_psi_pmt_write_args *args`
:   an instance of [`struct vidtv_psi_pmt_write_args`](#c.vidtv_psi_pmt_write_args "vidtv_psi_pmt_write_args")

**Description**

This function writes the MPEG TS packets for a PMT section into a buffer.
Calling code will usually generate the PMT section via a call to its init function
and thus is responsible for freeing it.

**Return**

The number of bytes written into the buffer. This is NOT
equal to the size of the PMT section, since more space is needed for TS headers
during TS encapsulation.

struct vidtv\_psi\_table\_pmt \*vidtv\_psi\_find\_pmt\_sec(struct vidtv\_psi\_table\_pmt \*\*pmt\_sections, u16 nsections, u16 program\_num)
:   Finds the PMT section for ‘program\_num’

**Parameters**

`struct vidtv_psi_table_pmt **pmt_sections`
:   The sections to look into.

`u16 nsections`
:   The number of sections.

`u16 program_num`
:   The ‘program\_num’ from PAT pointing to a PMT section.

**Return**

A pointer to the PMT, if found, or NULL.

struct vidtv\_psi\_table\_transport
:   A entry in the TS loop for the NIT and/or other tables. See ETSI 300 468 section 5.2.1

**Definition**:

```
struct vidtv_psi_table_transport {
    __be16 transport_id;
    __be16 network_id;
    __be16 bitfield;
    struct vidtv_psi_desc *descriptor;
    struct vidtv_psi_table_transport *next;
};
```

**Members**

`transport_id`
:   The TS ID being described

`network_id`
:   The network\_id that contains the TS ID

`bitfield`
:   Contains the descriptor loop length

`descriptor`
:   A descriptor loop

`next`
:   Pointer to the next entry

struct vidtv\_psi\_table\_nit
:   A Network Information Table (NIT). See ETSI 300 468 section 5.2.1

**Definition**:

```
struct vidtv_psi_table_nit {
    struct vidtv_psi_table_header header;
    __be16 bitfield;
    struct vidtv_psi_desc *descriptor;
    __be16 bitfield2;
    struct vidtv_psi_table_transport *transport;
};
```

**Members**

`header`
:   A PSI table header

`bitfield`
:   Contains the network descriptor length

`descriptor`
:   A descriptor loop describing the network

`bitfield2`
:   Contains the transport stream loop length

`transport`
:   The transport stream loop

struct vidtv\_psi\_nit\_write\_args
:   Arguments for writing a NIT section

**Definition**:

```
struct vidtv_psi_nit_write_args {
    char *buf;
    u32 offset;
    struct vidtv_psi_table_nit *nit;
    u32 buf_sz;
    u8 *continuity_counter;
};
```

**Members**

`buf`
:   The destination buffer.

`offset`
:   The offset into the destination buffer.

`nit`
:   A pointer to the NIT

`buf_sz`
:   The size of the destination buffer.

`continuity_counter`
:   A pointer to the CC. Incremented on every new packet.

u32 vidtv\_psi\_nit\_write\_into(struct [vidtv\_psi\_nit\_write\_args](#c.vidtv_psi_nit_write_args "vidtv_psi_nit_write_args") \*args)
:   Write NIT as MPEG-TS packets into a buffer.

**Parameters**

`struct vidtv_psi_nit_write_args *args`
:   an instance of [`struct vidtv_psi_nit_write_args`](#c.vidtv_psi_nit_write_args "vidtv_psi_nit_write_args")

**Description**

This function writes the MPEG TS packets for a NIT table into a buffer.
Calling code will usually generate the NIT via a call to its init function
and thus is responsible for freeing it.

**Return**

The number of bytes written into the buffer. This is NOT
equal to the size of the NIT, since more space is needed for TS headers during TS
encapsulation.

struct vidtv\_psi\_eit\_write\_args
:   Arguments for writing an EIT section

**Definition**:

```
struct vidtv_psi_eit_write_args {
    char *buf;
    u32 offset;
    struct vidtv_psi_table_eit *eit;
    u32 buf_sz;
    u8 *continuity_counter;
};
```

**Members**

`buf`
:   The destination buffer.

`offset`
:   The offset into the destination buffer.

`eit`
:   A pointer to the EIT

`buf_sz`
:   The size of the destination buffer.

`continuity_counter`
:   A pointer to the CC. Incremented on every new packet.

u32 vidtv\_psi\_eit\_write\_into(struct [vidtv\_psi\_eit\_write\_args](#c.vidtv_psi_eit_write_args "vidtv_psi_eit_write_args") \*args)
:   Write EIT as MPEG-TS packets into a buffer.

**Parameters**

`struct vidtv_psi_eit_write_args *args`
:   an instance of [`struct vidtv_psi_nit_write_args`](#c.vidtv_psi_nit_write_args "vidtv_psi_nit_write_args")

**Description**

This function writes the MPEG TS packets for a EIT table into a buffer.
Calling code will usually generate the EIT via a call to its init function
and thus is responsible for freeing it.

**Return**

The number of bytes written into the buffer. This is NOT
equal to the size of the EIT, since more space is needed for TS headers during TS
encapsulation.

void vidtv\_psi\_eit\_table\_update\_sec\_len(struct vidtv\_psi\_table\_eit \*eit)
:   Recompute and update the EIT section length.

**Parameters**

`struct vidtv_psi_table_eit *eit`
:   The EIT whose length is to be updated.

**Description**

This will traverse the table and accumulate the length of its components,
which is then used to replace the ‘section\_length’ field.

If section\_length > EIT\_MAX\_SECTION\_LEN, the operation fails.

void vidtv\_psi\_eit\_event\_assign(struct vidtv\_psi\_table\_eit \*eit, struct vidtv\_psi\_table\_eit\_event \*e)
:   Assigns the event loop to the EIT.

**Parameters**

`struct vidtv_psi_table_eit *eit`
:   The EIT to assign to.

`struct vidtv_psi_table_eit_event *e`
:   The event loop

**Description**

This will free the previous event loop in the table.
This will assign ownership of the stream loop to the table, i.e. the table
will free this stream loop when a call to its destroy function is made.

struct vidtv\_s302m\_ctx
:   s302m encoder context.

**Definition**:

```
struct vidtv_s302m_ctx {
    struct vidtv_encoder *enc;
    u32 frame_index;
    u32 au_count;
    int last_duration;
    unsigned int note_offset;
    enum musical_notes last_tone;
};
```

**Members**

`enc`
:   A pointer to the containing encoder structure.

`frame_index`
:   The current frame in a block

`au_count`
:   The total number of access units encoded up to now

`last_duration`
:   Duration of the tone currently being played

`note_offset`
:   Position at the music tone array

`last_tone`
:   Tone currently being played

struct vidtv\_s302m\_encoder\_init\_args
:   Args for the s302m encoder.

**Definition**:

```
struct vidtv_s302m_encoder_init_args {
    char *name;
    void *src_buf;
    u32 src_buf_sz;
    u16 es_pid;
    struct vidtv_encoder *sync;
    void (*last_sample_cb)(u32 sample_no);
    struct vidtv_encoder *head;
};
```

**Members**

`name`
:   A name to identify this particular instance

`src_buf`
:   The source buffer, encoder will default to a sine wave if this is NULL.

`src_buf_sz`
:   The size of the source buffer.

`es_pid`
:   The MPEG Elementary Stream PID to use.

`sync`
:   Attempt to synchronize audio with this video encoder, if not NULL.

`last_sample_cb`
:   A callback called when the encoder runs out of data.

`head`
:   Add to this chain

struct pcr\_write\_args
:   Arguments for the pcr\_write\_into function.

**Definition**:

```
struct pcr_write_args {
    void *dest_buf;
    u32 dest_offset;
    u16 pid;
    u32 buf_sz;
    u8 *continuity_counter;
    u64 pcr;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`dest_offset`
:   The byte offset into the buffer.

`pid`
:   The TS PID for the PCR packets.

`buf_sz`
:   The size of the buffer in bytes.

`continuity_counter`
:   The TS continuity\_counter.

`pcr`
:   A sample from the system clock.

struct null\_packet\_write\_args
:   Arguments for the null\_write\_into function

**Definition**:

```
struct null_packet_write_args {
    void *dest_buf;
    u32 dest_offset;
    u32 buf_sz;
    u8 *continuity_counter;
};
```

**Members**

`dest_buf`
:   The buffer to write into.

`dest_offset`
:   The byte offset into the buffer.

`buf_sz`
:   The size of the buffer in bytes.

`continuity_counter`
:   The TS continuity\_counter.

u32 vidtv\_ts\_null\_write\_into(const struct [null\_packet\_write\_args](#c.null_packet_write_args "null_packet_write_args") \*args)
:   Write a TS null packet into a buffer.

**Parameters**

`const struct null_packet_write_args *args`
:   the arguments to use when writing.

**Description**

This function will write a null packet into a buffer. This is usually used to
pad TS streams.

**Return**

The number of bytes written into the buffer.

u32 vidtv\_ts\_pcr\_write\_into(const struct [pcr\_write\_args](#c.pcr_write_args "pcr_write_args") \*args)
:   Write a PCR packet into a buffer.

**Parameters**

`const struct pcr_write_args *args`
:   the arguments to use when writing.

**Description**

This function will write a PCR packet into a buffer. This is used to
synchronize the clocks between encoders and decoders.

**Return**

The number of bytes written into the buffer.

struct vidtv\_tuner\_config
:   Configuration used to init the tuner.

**Definition**:

```
struct vidtv_tuner_config {
    struct dvb_frontend *fe;
    u32 mock_power_up_delay_msec;
    u32 mock_tune_delay_msec;
    u32 vidtv_valid_dvb_t_freqs[NUM_VALID_TUNER_FREQS];
    u32 vidtv_valid_dvb_c_freqs[NUM_VALID_TUNER_FREQS];
    u32 vidtv_valid_dvb_s_freqs[NUM_VALID_TUNER_FREQS];
    u8 max_frequency_shift_hz;
};
```

**Members**

`fe`
:   A pointer to the dvb\_frontend structure allocated by vidtv\_demod.

`mock_power_up_delay_msec`
:   Simulate a power-up delay.

`mock_tune_delay_msec`
:   Simulate a tune delay.

`vidtv_valid_dvb_t_freqs`
:   The valid DVB-T frequencies to simulate.

`vidtv_valid_dvb_c_freqs`
:   The valid DVB-C frequencies to simulate.

`vidtv_valid_dvb_s_freqs`
:   The valid DVB-S frequencies to simulate.

`max_frequency_shift_hz`
:   The maximum frequency shift in HZ allowed when
    tuning in a channel

**Description**

The configuration used to init the tuner module, usually filled
by a bridge driver. For vidtv, this is filled by vidtv\_bridge before the
tuner module is probed.

u32 vidtv\_memcpy(void \*to, size\_t to\_offset, size\_t to\_size, const void \*from, size\_t len)
:   wrapper routine to be used by MPEG-TS generator, in order to avoid going past the output buffer.

**Parameters**

`void *to`
:   Starting element to where a MPEG-TS packet will
    be copied.

`size_t to_offset`
:   Starting position of the **to** buffer to be filled.

`size_t to_size`
:   Size of the **to** buffer.

`const void *from`
:   Starting element of the buffer to be copied.

`size_t len`
:   Number of elements to be copy from **from** buffer
    into **to\*\*+ \*\*to\_offset** buffer.

**Note**

> Real digital TV demod drivers should not have memcpy
> wrappers. We use it here because emulating MPEG-TS
> generation at kernelspace requires some extra care.

**Return**

Returns the number of bytes written

u32 vidtv\_memset(void \*to, size\_t to\_offset, size\_t to\_size, const int c, size\_t len)
:   wrapper routine to be used by MPEG-TS generator, in order to avoid going past the output buffer.

**Parameters**

`void *to`
:   Starting element to set

`size_t to_offset`
:   Starting position of the **to** buffer to be filled.

`size_t to_size`
:   Size of the **to** buffer.

`const int c`
:   The value to set the memory to.

`size_t len`
:   Number of elements to be copy from **from** buffer
    into **to\*\*+ \*\*to\_offset** buffer.

**Note**

> Real digital TV demod drivers should not have memset
> wrappers. We use it here because emulating MPEG-TS
> generation at kernelspace requires some extra care.

**Return**

Returns the number of bytes written

struct vidtv\_tuner\_hardware\_state
:   Simulate the tuner hardware status

**Definition**:

```
struct vidtv_tuner_hardware_state {
    bool asleep;
    u32 lock_status;
    u32 if_frequency;
    u32 tuned_frequency;
    u32 bandwidth;
};
```

**Members**

`asleep`
:   whether the tuner is asleep, i.e whether `_sleep()` or `_suspend()` was
    called.

`lock_status`
:   Whether the tuner has managed to lock on the requested
    frequency.

`if_frequency`
:   The tuner’s intermediate frequency. Hardcoded for the purposes
    of simulation.

`tuned_frequency`
:   The actual tuned frequency.

`bandwidth`
:   The actual bandwidth.

**Description**

This structure is meant to simulate the status of the tuner hardware, as if
we had a physical tuner hardware.

struct vidtv\_tuner\_dev
:   The tuner struct

**Definition**:

```
struct vidtv_tuner_dev {
    struct dvb_frontend *fe;
    struct vidtv_tuner_hardware_state hw_state;
    struct vidtv_tuner_config config;
};
```

**Members**

`fe`
:   A pointer to the dvb\_frontend structure allocated by vidtv\_demod

`hw_state`
:   A `struct to` simulate the tuner’s hardware state as if we had a
    physical tuner hardware.

`config`
:   The configuration used to start the tuner module, usually filled
    by a bridge driver. For vidtv, this is filled by vidtv\_bridge before the
    tuner module is probed.
