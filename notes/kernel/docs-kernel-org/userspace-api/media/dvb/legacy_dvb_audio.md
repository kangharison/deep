# 6.2.3.2.DVB Audio Device

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/legacy_dvb_audio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.2.3.2. DVB Audio Device

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

The DVB audio device controls the MPEG2 audio decoder of the DVB
hardware. It can be accessed through `/dev/dvb/adapter?/audio?`. Data
types and ioctl definitions can be accessed by including
`linux/dvb/audio.h` in your application.

Please note that most DVB cards don’t have their own MPEG decoder, which
results in the omission of the audio and video device.

These ioctls were also used by V4L2 to control MPEG decoders implemented
in V4L2. The use of these ioctls for that purpose has been made obsolete
and proper V4L2 ioctls or controls have been created to replace that
functionality. Use [V4L2 ioctls](../v4l/audio.html#audio) for new drivers!

## 6.2.3.2.1. Audio Data Types

This section describes the structures, data types and defines used when
talking to the audio device.

---

### 6.2.3.2.1.1. audio\_stream\_source\_t

#### 6.2.3.2.1.1.1. Synopsis

enum audio\_stream\_source\_t

```
typedef enum {
AUDIO_SOURCE_DEMUX,
AUDIO_SOURCE_MEMORY
} audio_stream_source_t;
```

#### 6.2.3.2.1.1.2. Constants

|  |  |  |
| --- | --- | --- |
| `AUDIO_SOURCE_DEMUX` | Selects the demultiplexer (fed either by the frontend or the DVR device) as the source of the video stream. | |
| `AUDIO_SOURCE_MEMORY` | Selects the stream from the application that comes through the [write()](#write) system call. | |

#### 6.2.3.2.1.1.3. Description

The audio stream source is set through the [AUDIO\_SELECT\_SOURCE](#audio-select-source) call
and can take the following values, depending on whether we are replaying
from an internal (demux) or external (user write) source.

The data fed to the decoder is also controlled by the PID-filter.
Output selection: `dmx_output` `DMX_OUT_DECODER`.

---

### 6.2.3.2.1.2. audio\_play\_state\_t

#### 6.2.3.2.1.2.1. Synopsis

enum audio\_play\_state\_t

```
typedef enum {
    AUDIO_STOPPED,
    AUDIO_PLAYING,
    AUDIO_PAUSED
} audio_play_state_t;
```

#### 6.2.3.2.1.2.2. Constants

|  |  |
| --- | --- |
| `AUDIO_STOPPED` | Audio is stopped. |
| `AUDIO_PLAYING` | Audio is currently playing. |
| `AUDIO_PAUSE` | Audio is frozen. |

#### 6.2.3.2.1.2.3. Description

This values can be returned by the [AUDIO\_GET\_STATUS](#audio-get-status) call
representing the state of audio playback.

---

### 6.2.3.2.1.3. audio\_channel\_select\_t

#### 6.2.3.2.1.3.1. Synopsis

enum audio\_channel\_select\_t

```
typedef enum {
    AUDIO_STEREO,
    AUDIO_MONO_LEFT,
    AUDIO_MONO_RIGHT,
    AUDIO_MONO,
    AUDIO_STEREO_SWAPPED
} audio_channel_select_t;
```

#### 6.2.3.2.1.3.2. Constants

|  |  |
| --- | --- |
| `AUDIO_STEREO` | Stereo. |
| `AUDIO_MONO_LEFT` | Mono, select left stereo channel as source. |
| `AUDIO_MONO_RIGHT` | Mono, select right stereo channel as source. |
| `AUDIO_MONO` | Mono source only. |
| `AUDIO_STEREO_SWAPPED` | Stereo, swap L & R. |

#### 6.2.3.2.1.3.3. Description

The audio channel selected via [AUDIO\_CHANNEL\_SELECT](#audio-channel-select) is determined by
this value.

---

### 6.2.3.2.1.4. audio\_mixer\_t

#### 6.2.3.2.1.4.1. Synopsis

struct audio\_mixer

```
typedef struct audio_mixer {
    unsigned int volume_left;
    unsigned int volume_right;
} audio_mixer_t;
```

#### 6.2.3.2.1.4.2. Variables

|  |  |
| --- | --- |
| `unsigned int volume_left` | Volume left channel. Valid range: 0 ... 255 |
| `unsigned int volume_right` | Volume right channel. Valid range: 0 ... 255 |

#### 6.2.3.2.1.4.3. Description

This structure is used by the [AUDIO\_SET\_MIXER](#audio-set-mixer) call to set the
audio volume.

---

### 6.2.3.2.1.5. audio\_status

#### 6.2.3.2.1.5.1. Synopsis

struct audio\_status

```
typedef struct audio_status {
    int AV_sync_state;
    int mute_state;
    audio_play_state_t play_state;
    audio_stream_source_t stream_source;
    audio_channel_select_t channel_select;
    int bypass_mode;
    audio_mixer_t mixer_state;
} audio_status_t;
```

#### 6.2.3.2.1.5.2. Variables

|  |  |  |
| --- | --- | --- |
| `int AV_sync_state` | Shows if A/V synchronization is ON or OFF. | |
| TRUE ( != 0 ) | AV-sync ON. |
| FALSE ( == 0 ) | AV-sync OFF. |
| `int mute_state` | Indicates if audio is muted or not. | |
| TRUE ( != 0 ) | mute audio |
| FALSE ( == 0 ) | unmute audio |
| [audio\_play\_state\_t](#audio-play-state-t) `play_state` | Current playback state. | |
| [audio\_stream\_source\_t](#audio-stream-source-t) `stream_source` | Current source of the data. | |
| `int bypass_mode` | Is the decoding of the current Audio stream in the DVB subsystem enabled or disabled. | |
| TRUE ( != 0 ) | Bypass disabled. |
| FALSE ( == 0 ) | Bypass enabled. |
| [audio\_mixer\_t](#audio-mixer-t) `mixer_state` | Current volume settings. | |

#### 6.2.3.2.1.5.3. Description

The [AUDIO\_GET\_STATUS](#audio-get-status) call returns this structure as information
about various states of the playback operation.

---

### 6.2.3.2.1.6. audio encodings

#### 6.2.3.2.1.6.1. Synopsis

```
#define AUDIO_CAP_DTS    1
#define AUDIO_CAP_LPCM   2
#define AUDIO_CAP_MP1    4
#define AUDIO_CAP_MP2    8
#define AUDIO_CAP_MP3   16
#define AUDIO_CAP_AAC   32
#define AUDIO_CAP_OGG   64
#define AUDIO_CAP_SDDS 128
#define AUDIO_CAP_AC3  256
```

#### 6.2.3.2.1.6.2. Constants

|  |  |  |
| --- | --- | --- |
| `AUDIO_CAP_DTS` | The hardware accepts DTS audio tracks. | |
| `AUDIO_CAP_LPCM` | The hardware accepts uncompressed audio with Linear Pulse-Code Modulation (LPCM) | |
| `AUDIO_CAP_MP1` | The hardware accepts MPEG-1 Audio Layer 1. | |
| `AUDIO_CAP_MP2` | The hardware accepts MPEG-1 Audio Layer 2. Also known as MUSICAM. | |
| `AUDIO_CAP_MP3` | The hardware accepts MPEG-1 Audio Layer III. Commonly known as .mp3. | |
| `AUDIO_CAP_AAC` | The hardware accepts AAC (Advanced Audio Coding). | |
| `AUDIO_CAP_OGG` | The hardware accepts Vorbis audio tracks. | |
| `AUDIO_CAP_SDDS` | The hardware accepts Sony Dynamic Digital Sound (SDDS). | |
| `AUDIO_CAP_AC3` | The hardware accepts Dolby Digital ATSC A/52 audio. Also known as AC-3. | |

#### 6.2.3.2.1.6.3. Description

A call to [AUDIO\_GET\_CAPABILITIES](#audio-get-capabilities) returns an unsigned integer with the
following bits set according to the hardware’s capabilities.

---

## 6.2.3.2.2. Audio Function Calls

### 6.2.3.2.2.1. AUDIO\_STOP

#### 6.2.3.2.2.1.1. Synopsis

AUDIO\_STOP

```
int ioctl(int fd, int request = AUDIO_STOP)
```

#### 6.2.3.2.2.1.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_STOP` for this command. | |

#### 6.2.3.2.2.1.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Audio Device to stop playing the current
stream.

#### 6.2.3.2.2.1.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.2. AUDIO\_PLAY

#### 6.2.3.2.2.2.1. Synopsis

AUDIO\_PLAY

```
int  ioctl(int fd, int request = AUDIO_PLAY)
```

#### 6.2.3.2.2.2.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_PLAY` for this command. | |

#### 6.2.3.2.2.2.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Audio Device to start playing an audio stream
from the selected source.

#### 6.2.3.2.2.2.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.3. AUDIO\_PAUSE

#### 6.2.3.2.2.3.1. Synopsis

AUDIO\_PAUSE

```
int  ioctl(int fd, int request = AUDIO_PAUSE)
```

#### 6.2.3.2.2.3.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_PAUSE` for this command. | |

#### 6.2.3.2.2.3.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call suspends the audio stream being played. Decoding and
playing are paused. It is then possible to restart again decoding and
playing process of the audio stream using [AUDIO\_CONTINUE](#audio-continue) command.

#### 6.2.3.2.2.3.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.4. AUDIO\_CONTINUE

#### 6.2.3.2.2.4.1. Synopsis

AUDIO\_CONTINUE

```
int  ioctl(int fd, int request = AUDIO_CONTINUE)
```

#### 6.2.3.2.2.4.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_CONTINUE` for this command. | |

#### 6.2.3.2.2.4.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl restarts the decoding and playing process previously paused
with [AUDIO\_PAUSE](#audio-pause) command.

#### 6.2.3.2.2.4.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.5. AUDIO\_SELECT\_SOURCE

#### 6.2.3.2.2.5.1. Synopsis

AUDIO\_SELECT\_SOURCE

```
int ioctl(int fd, int request = AUDIO_SELECT_SOURCE,
audio_stream_source_t source)
```

#### 6.2.3.2.2.5.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_SELECT_SOURCE` for this command. | |
| [audio\_stream\_source\_t](#audio-stream-source-t) `source` | Indicates the source that shall be used for the Audio stream. | |

#### 6.2.3.2.2.5.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call informs the audio device which source shall be used for
the input data. The possible sources are demux or memory. If
`AUDIO_SOURCE_MEMORY` is selected, the data is fed to the Audio Device
through the write command. If `AUDIO_SOURCE_DEMUX` is selected, the data
is directly transferred from the onboard demux-device to the decoder.
Note: This only supports DVB-devices with one demux and one decoder so far.

#### 6.2.3.2.2.5.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.6. AUDIO\_SET\_MUTE

#### 6.2.3.2.2.6.1. Synopsis

AUDIO\_SET\_MUTE

```
int  ioctl(int fd, int request = AUDIO_SET_MUTE, int state)
```

#### 6.2.3.2.2.6.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_SET_MUTE` for this command. | |
| `int state` | Indicates if audio device shall mute or not. | |
| TRUE ( != 0 ) | mute audio |
| FALSE ( == 0 ) | unmute audio |

#### 6.2.3.2.2.6.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for DVB devices only. To control a V4L2 decoder use the
V4L2 [ioctl VIDIOC\_DECODER\_CMD, VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) with the
`V4L2_DEC_CMD_START_MUTE_AUDIO` flag instead.

This ioctl call asks the audio device to mute the stream that is
currently being played.

#### 6.2.3.2.2.6.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.7. AUDIO\_SET\_AV\_SYNC

#### 6.2.3.2.2.7.1. Synopsis

AUDIO\_SET\_AV\_SYNC

```
int  ioctl(int fd, int request = AUDIO_SET_AV_SYNC, int state)
```

#### 6.2.3.2.2.7.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_AV_SYNC` for this command. | |
| `int state` | Tells the DVB subsystem if A/V synchronization shall be ON or OFF. | |
| TRUE ( != 0 ) | AV-sync ON. |
| FALSE ( == 0 ) | AV-sync OFF. |

#### 6.2.3.2.2.7.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Audio Device to turn ON or OFF A/V
synchronization.

#### 6.2.3.2.2.7.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.8. AUDIO\_SET\_BYPASS\_MODE

#### 6.2.3.2.2.8.1. Synopsis

AUDIO\_SET\_BYPASS\_MODE

```
int ioctl(int fd, int request = AUDIO_SET_BYPASS_MODE, int mode)
```

#### 6.2.3.2.2.8.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_SET_BYPASS_MODE` for this command. | |
| `int mode` | Enables or disables the decoding of the current Audio stream in the DVB subsystem. | |
| TRUE ( != 0 ) | Disable bypass |
| FALSE ( == 0 ) | Enable bypass |

#### 6.2.3.2.2.8.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Audio Device to bypass the Audio decoder and
forward the stream without decoding. This mode shall be used if streams
that can’t be handled by the DVB system shall be decoded. Dolby
DigitalTM streams are automatically forwarded by the DVB subsystem if
the hardware can handle it.

#### 6.2.3.2.2.8.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.9. AUDIO\_CHANNEL\_SELECT

#### 6.2.3.2.2.9.1. Synopsis

AUDIO\_CHANNEL\_SELECT

```
int ioctl(int fd, int request = AUDIO_CHANNEL_SELECT,
audio_channel_select_t)
```

#### 6.2.3.2.2.9.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_CHANNEL_SELECT` for this command. | |
| [audio\_channel\_select\_t](#audio-channel-select-t) `ch` | Select the output format of the audio (mono left/right, stereo). | |

#### 6.2.3.2.2.9.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for DVB devices only. To control a V4L2 decoder use the
V4L2 `V4L2_CID_MPEG_AUDIO_DEC_PLAYBACK` control instead.

This ioctl call asks the Audio Device to select the requested channel if
possible.

#### 6.2.3.2.2.9.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.10. AUDIO\_GET\_STATUS

#### 6.2.3.2.2.10.1. Synopsis

AUDIO\_GET\_STATUS

```
int ioctl(int fd, int request = AUDIO_GET_STATUS,
struct audio_status *status)
```

#### 6.2.3.2.2.10.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals AUDIO\_GET\_STATUS for this command. | |
| `struct` [audio\_status](#audio-status) `*status` | Returns the current state of Audio Device. | |

#### 6.2.3.2.2.10.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Audio Device to return the current state of the
Audio Device.

#### 6.2.3.2.2.10.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.11. AUDIO\_GET\_CAPABILITIES

#### 6.2.3.2.2.11.1. Synopsis

AUDIO\_GET\_CAPABILITIES

```
int ioctl(int fd, int request = AUDIO_GET_CAPABILITIES,
unsigned int *cap)
```

#### 6.2.3.2.2.11.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_GET_CAPABILITIES` for this command. | |
| `unsigned int *cap` | Returns a bit array of supported sound formats. Bits are defined in [audio encodings](#audio-encodings). | |

#### 6.2.3.2.2.11.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Audio Device to tell us about the decoding
capabilities of the audio hardware.

#### 6.2.3.2.2.11.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.12. AUDIO\_CLEAR\_BUFFER

#### 6.2.3.2.2.12.1. Synopsis

AUDIO\_CLEAR\_BUFFER

```
int  ioctl(int fd, int request = AUDIO_CLEAR_BUFFER)
```

#### 6.2.3.2.2.12.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_CLEAR_BUFFER` for this command. | |

#### 6.2.3.2.2.12.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Audio Device to clear all software and hardware
buffers of the audio decoder device.

#### 6.2.3.2.2.12.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.13. AUDIO\_SET\_ID

#### 6.2.3.2.2.13.1. Synopsis

AUDIO\_SET\_ID

```
int  ioctl(int fd, int request = AUDIO_SET_ID, int id)
```

#### 6.2.3.2.2.13.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_SET_ID` for this command. | |
| `int id` | Audio sub-stream id. | |

#### 6.2.3.2.2.13.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl selects which sub-stream is to be decoded if a program or
system stream is sent to the video device.

If no audio stream type is set the id has to be in range [0xC0,0xDF]
for MPEG sound, in [0x80,0x87] for AC3 and in [0xA0,0xA7] for LPCM.
See ITU-T H.222.0 | ISO/IEC 13818-1 for further description.

If the stream type is set with [AUDIO\_SET\_STREAMTYPE](#audio-set-streamtype), specifies the
id just the sub-stream id of the audio stream and only the first 5 bits
(& 0x1F) are recognized.

#### 6.2.3.2.2.13.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.14. AUDIO\_SET\_MIXER

#### 6.2.3.2.2.14.1. Synopsis

AUDIO\_SET\_MIXER

```
int ioctl(int fd, int request = AUDIO_SET_MIXER, audio_mixer_t *mix)
```

#### 6.2.3.2.2.14.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_SET_MIXER` for this command. | |
| `audio_mixer_t *mix` | Mixer settings. | |

#### 6.2.3.2.2.14.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl lets you adjust the mixer settings of the audio decoder.

#### 6.2.3.2.2.14.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.15. AUDIO\_SET\_STREAMTYPE

#### 6.2.3.2.2.15.1. Synopsis

AUDIO\_SET\_STREAMTYPE

```
int  ioctl(fd, int request = AUDIO_SET_STREAMTYPE, int type)
```

#### 6.2.3.2.2.15.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_SET_STREAMTYPE` for this command. | |
| `int type` | Stream type. | |

#### 6.2.3.2.2.15.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl tells the driver which kind of audio stream to expect. This
is useful if the stream offers several audio sub-streams like LPCM and
AC3.

Stream types defined in ITU-T H.222.0 | ISO/IEC 13818-1 are used.

#### 6.2.3.2.2.15.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

|  |  |
| --- | --- |
| `EINVAL` | Type is not a valid or supported stream type. |

---

### 6.2.3.2.2.16. AUDIO\_BILINGUAL\_CHANNEL\_SELECT

#### 6.2.3.2.2.16.1. Synopsis

AUDIO\_BILINGUAL\_CHANNEL\_SELECT

```
int ioctl(int fd, int request = AUDIO_BILINGUAL_CHANNEL_SELECT,
audio_channel_select_t)
```

#### 6.2.3.2.2.16.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `AUDIO_BILINGUAL_CHANNEL_SELECT` for this command. | |
| `audio_channel_select_t ch` | Select the output format of the audio (mono left/right, stereo). | |

#### 6.2.3.2.2.16.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl has been replaced by the V4L2
`V4L2_CID_MPEG_AUDIO_DEC_MULTILINGUAL_PLAYBACK` control
for MPEG decoders controlled through V4L2.

This ioctl call asks the Audio Device to select the requested channel
for bilingual streams if possible.

#### 6.2.3.2.2.16.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.2.2.17. open()

#### 6.2.3.2.2.17.1. Synopsis

```
#include <fcntl.h>
```

int open(const char \*deviceName, int flags)

#### 6.2.3.2.2.17.2. Arguments

|  |  |  |
| --- | --- | --- |
| `const char *deviceName` | Name of specific audio device. | |
| `int flags` | A bit-wise OR of the following flags: | |
| `O_RDONLY` | read-only access |
| `O_RDWR` | read/write access |
| `O_NONBLOCK` | Open in non-blocking mode  (blocking mode is the default) |

#### 6.2.3.2.2.17.3. Description

This system call opens a named audio device (e.g.
`/dev/dvb/adapter0/audio0`) for subsequent use. When an [`open()`](#c.dtv.legacy.audio.open "dtv.legacy.audio.open") call has
succeeded, the device will be ready for use. The significance of
blocking or non-blocking mode is described in the documentation for
functions where there is a difference. It does not affect the semantics
of the [`open()`](#c.dtv.legacy.audio.open "dtv.legacy.audio.open") call itself. A device opened in blocking mode can later be
put into non-blocking mode (and vice versa) using the F\_SETFL command
of the fcntl system call. This is a standard system call, documented in
the Linux manual page for fcntl. Only one user can open the Audio Device
in O\_RDWR mode. All other attempts to open the device in this mode will
fail, and an error code will be returned. If the Audio Device is opened
in O\_RDONLY mode, the only ioctl call that can be used is
[AUDIO\_GET\_STATUS](#audio-get-status). All other call will return with an error code.

#### 6.2.3.2.2.17.4. Return Value

|  |  |
| --- | --- |
| `ENODEV` | Device driver not loaded/available. |
| `EBUSY` | Device or resource busy. |
| `EINVAL` | Invalid argument. |

---

### 6.2.3.2.2.18. close()

#### 6.2.3.2.2.18.1. Synopsis

int close(int fd)

#### 6.2.3.2.2.18.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |

#### 6.2.3.2.2.18.3. Description

This system call closes a previously opened audio device.

#### 6.2.3.2.2.18.4. Return Value

|  |  |
| --- | --- |
| `EBADF` | Fd is not a valid open file descriptor. |

---

### 6.2.3.2.2.19. write()

#### 6.2.3.2.2.19.1. Synopsis

```
size_t write(int fd, const void *buf, size_t count)
```

#### 6.2.3.2.2.19.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `void *buf` | Pointer to the buffer containing the PES data. | |
| `size_t count` | Size of buf. | |

#### 6.2.3.2.2.19.3. Description

This system call can only be used if `AUDIO_SOURCE_MEMORY` is selected
in the ioctl call [AUDIO\_SELECT\_SOURCE](#audio-select-source). The data provided shall be in
PES format. If `O_NONBLOCK` is not specified the function will block
until buffer space is available. The amount of data to be transferred is
implied by count.

#### 6.2.3.2.2.19.4. Return Value

|  |  |  |
| --- | --- | --- |
| `EPERM` | Mode `AUDIO_SOURCE_MEMORY` not selected. | |
| `ENOMEM` | Attempted to write more data than the internal buffer can hold. | |
| `EBADF` | Fd is not a valid open file descriptor. | |
