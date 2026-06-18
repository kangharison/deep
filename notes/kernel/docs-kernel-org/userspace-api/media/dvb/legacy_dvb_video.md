# 6.2.3.1.DVB Video Device

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/legacy_dvb_video.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.2.3.1. DVB Video Device

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

The DVB video device controls the MPEG2 video decoder of the DVB
hardware. It can be accessed through `/dev/dvb/adapter0/video0`. Data
types and ioctl definitions can be accessed by including
`linux/dvb/video.h` in your application.

Note that the DVB video device only controls decoding of the MPEG video
stream, not its presentation on the TV or computer screen. On PCs this
is typically handled by an associated video4linux device, e.g.
`/dev/video`, which allows scaling and defining output windows.

Most DVB cards don’t have their own MPEG decoder, which results in the
omission of the audio and video device as well as the video4linux
device.

These ioctls were also used by V4L2 to control MPEG decoders implemented
in V4L2. The use of these ioctls for that purpose has been made obsolete
and proper V4L2 ioctls or controls have been created to replace that
functionality. Use [V4L2 ioctls](../v4l/video.html#video) for new drivers!

## 6.2.3.1.1. Video Data Types

### 6.2.3.1.1.1. video\_format\_t

#### 6.2.3.1.1.1.1. Synopsis

```
typedef enum {
    VIDEO_FORMAT_4_3,
    VIDEO_FORMAT_16_9,
    VIDEO_FORMAT_221_1
} video_format_t;
```

#### 6.2.3.1.1.1.2. Constants

|  |  |
| --- | --- |
| `VIDEO_FORMAT_4_3` | Select 4:3 format. |
| `VIDEO_FORMAT_16_9` | Select 16:9 format. |
| `VIDEO_FORMAT_221_1` | Select 2.21:1 format. |

#### 6.2.3.1.1.1.3. Description

The `video_format_t` data type
is used in the [VIDEO\_SET\_FORMAT](#video-set-format) function to tell the driver which
aspect ratio the output hardware (e.g. TV) has. It is also used in the
data structures [video\_status](#video-status) returned by [VIDEO\_GET\_STATUS](#video-get-status)
and [video\_event](#video-event) returned by [VIDEO\_GET\_EVENT](#video-get-event) which report
about the display format of the current video stream.

---

### 6.2.3.1.1.2. video\_displayformat\_t

#### 6.2.3.1.1.2.1. Synopsis

```
typedef enum {
    VIDEO_PAN_SCAN,
    VIDEO_LETTER_BOX,
    VIDEO_CENTER_CUT_OUT
} video_displayformat_t;
```

#### 6.2.3.1.1.2.2. Constants

|  |  |
| --- | --- |
| `VIDEO_PAN_SCAN` | Use pan and scan format. |
| `VIDEO_LETTER_BOX` | Use letterbox format. |
| `VIDEO_CENTER_CUT_OUT` | Use center cut out format. |

#### 6.2.3.1.1.2.3. Description

In case the display format of the video stream and of the display
hardware differ the application has to specify how to handle the
cropping of the picture. This can be done using the
[VIDEO\_SET\_DISPLAY\_FORMAT](#video-set-display-format) call which accepts this `enum as` argument.

---

### 6.2.3.1.1.3. video\_size\_t

#### 6.2.3.1.1.3.1. Synopsis

```
typedef struct {
    int w;
    int h;
    video_format_t aspect_ratio;
} video_size_t;
```

#### 6.2.3.1.1.3.2. Variables

|  |  |
| --- | --- |
| `int w` | Video width in pixels. |
| `int h` | Video height in pixels. |
| [video\_format\_t](#video-format-t) `aspect_ratio` | Aspect ratio. |

#### 6.2.3.1.1.3.3. Description

Used in the struct [video\_event](#video-event). It stores the resolution and
aspect ratio of the video.

---

### 6.2.3.1.1.4. video\_stream\_source\_t

#### 6.2.3.1.1.4.1. Synopsis

```
typedef enum {
    VIDEO_SOURCE_DEMUX,
    VIDEO_SOURCE_MEMORY
} video_stream_source_t;
```

#### 6.2.3.1.1.4.2. Constants

|  |  |  |
| --- | --- | --- |
| `VIDEO_SOURCE_DEMUX` | Select the demux as the main source. | |
| `VIDEO_SOURCE_MEMORY` | If this source is selected, the stream comes from the user through the write system call. | |

#### 6.2.3.1.1.4.3. Description

The video stream source is set through the [VIDEO\_SELECT\_SOURCE](#video-select-source) call
and can take the following values, depending on whether we are replaying
from an internal (demuxer) or external (user write) source.
VIDEO\_SOURCE\_DEMUX selects the demultiplexer (fed either by the
frontend or the DVR device) as the source of the video stream. If
VIDEO\_SOURCE\_MEMORY is selected the stream comes from the application
through the [write()](#write) system call.

---

### 6.2.3.1.1.5. video\_play\_state\_t

#### 6.2.3.1.1.5.1. Synopsis

```
typedef enum {
    VIDEO_STOPPED,
    VIDEO_PLAYING,
    VIDEO_FREEZED
} video_play_state_t;
```

#### 6.2.3.1.1.5.2. Constants

|  |  |
| --- | --- |
| `VIDEO_STOPPED` | Video is stopped. |
| `VIDEO_PLAYING` | Video is currently playing. |
| `VIDEO_FREEZED` | Video is frozen. |

#### 6.2.3.1.1.5.3. Description

This values can be returned by the [VIDEO\_GET\_STATUS](#video-get-status) call
representing the state of video playback.

---

### 6.2.3.1.1.6. struct video\_command

#### 6.2.3.1.1.6.1. Synopsis

```
struct video_command {
    __u32 cmd;
    __u32 flags;
    union {
        struct {
            __u64 pts;
        } stop;

        struct {
            __s32 speed;
            __u32 format;
        } play;

        struct {
            __u32 data[16];
        } raw;
    };
};
```

#### 6.2.3.1.1.6.2. Variables

|  |  |  |
| --- | --- | --- |
| `__u32 cmd` | [Decoder command](#decoder-command) | |
| `__u32 flags` | Flags for the [Decoder command](#decoder-command). | |
| `struct stop` | `__u64 pts` | MPEG PTS |
| `stuct play` | `__s32 speed` | 0 or 1000 specifies normal speed, |
| 1: specifies forward single stepping, |
| -1: specifies backward single stepping, |
| >1: playback at speed / 1000 of the normal speed |
| <-1: reverse playback at ( -speed / 1000 ) of the normal speed. |
| `__u32 format` | [Play input formats](#play-input-formats) |
| `__u32 data[16]` | Reserved | |

#### 6.2.3.1.1.6.3. Description

The structure must be zeroed before use by the application. This ensures
it can be extended safely in the future.

---

### 6.2.3.1.1.7. Predefined decoder commands and flags

#### 6.2.3.1.1.7.1. Synopsis

```
#define VIDEO_CMD_PLAY                      (0)
#define VIDEO_CMD_STOP                      (1)
#define VIDEO_CMD_FREEZE                    (2)
#define VIDEO_CMD_CONTINUE                  (3)

#define VIDEO_CMD_FREEZE_TO_BLACK      (1 << 0)

#define VIDEO_CMD_STOP_TO_BLACK        (1 << 0)
#define VIDEO_CMD_STOP_IMMEDIATELY     (1 << 1)

#define VIDEO_PLAY_FMT_NONE                 (0)
#define VIDEO_PLAY_FMT_GOP                  (1)

#define VIDEO_VSYNC_FIELD_UNKNOWN           (0)
#define VIDEO_VSYNC_FIELD_ODD               (1)
#define VIDEO_VSYNC_FIELD_EVEN              (2)
#define VIDEO_VSYNC_FIELD_PROGRESSIVE       (3)
```

#### 6.2.3.1.1.7.2. Constants

|  |  |  |
| --- | --- | --- |
| Decoder command | `VIDEO_CMD_PLAY` | Start playback. |
| `VIDEO_CMD_STOP` | Stop playback. |
| `VIDEO_CMD_FREEZE` | Freeze playback. |
| `VIDEO_CMD_CONTINUE` | Continue playback after freeze. |
| Flags for `VIDEO_CMD_FREEZE` | `VIDEO_CMD_FREEZE_TO_BLACK` | Show black picture on freeze. |
| Flags for `VIDEO_CMD_STOP` | `VIDEO_CMD_STOP_TO_BLACK` | Show black picture on stop. |
| `VIDEO_CMD_STOP_IMMEDIATELY` | Stop immediately, without emptying buffers. |
| Play input formats | `VIDEO_PLAY_FMT_NONE` | The decoder has no special format requirements |
| `VIDEO_PLAY_FMT_GOP` | The decoder requires full GOPs |
| Field order | `VIDEO_VSYNC_FIELD_UNKNOWN` | FIELD\_UNKNOWN can be used if the hardware does not know whether the Vsync is for an odd, even or progressive (i.e. non-interlaced) field. |
| `VIDEO_VSYNC_FIELD_ODD` | Vsync is for an odd field. |
| `VIDEO_VSYNC_FIELD_EVEN` | Vsync is for an even field. |
| `VIDEO_VSYNC_FIELD_PROGRESSIVE` | progressive (i.e. non-interlaced) |

---

### 6.2.3.1.1.8. video\_event

#### 6.2.3.1.1.8.1. Synopsis

```
struct video_event {
    __s32 type;
#define VIDEO_EVENT_SIZE_CHANGED        1
#define VIDEO_EVENT_FRAME_RATE_CHANGED  2
#define VIDEO_EVENT_DECODER_STOPPED     3
#define VIDEO_EVENT_VSYNC               4
    long timestamp;
    union {
        video_size_t size;
        unsigned int frame_rate;
        unsigned char vsync_field;
    } u;
};
```

#### 6.2.3.1.1.8.2. Variables

|  |  |  |
| --- | --- | --- |
| `__s32 type` | Event type. | |
| `VIDEO_EVENT_SIZE_CHANGED` | Size changed. |
| `VIDEO_EVENT_FRAME_RATE_CHANGED` | Framerate changed. |
| `VIDEO_EVENT_DECODER_STOPPED` | Decoder stopped. |
| `VIDEO_EVENT_VSYNC` | Vsync occurred. |
| `long timestamp` | MPEG PTS at occurrence. | |
| `union u` | [video\_size\_t](#video-size-t) size | Resolution and aspect ratio of the video. |
| `unsigned int frame_rate` | in frames per 1000sec |
| `unsigned char vsync_field` | unknown / odd / even / progressive  See: [Predefined decoder commands and flags](#predefined-decoder-commands-and-flags) |

#### 6.2.3.1.1.8.3. Description

This is the structure of a video event as it is returned by the
[VIDEO\_GET\_EVENT](#video-get-event) call. See there for more details.

---

### 6.2.3.1.1.9. video\_status

#### 6.2.3.1.1.9.1. Synopsis

The [VIDEO\_GET\_STATUS](#video-get-status) call returns the following structure informing
about various states of the playback operation.

```
struct video_status {
    int                    video_blank;
    video_play_state_t     play_state;
    video_stream_source_t  stream_source;
    video_format_t         video_format;
    video_displayformat_t  display_format;
};
```

#### 6.2.3.1.1.9.2. Variables

|  |  |  |
| --- | --- | --- |
| `int video_blank` | Show blank video on freeze? | |
| TRUE ( != 0 ) | Blank screen when freeze. |
| FALSE ( == 0 ) | Show last decoded frame. |
| [video\_play\_state\_t](#video-play-state-t) `play_state` | Current state of playback. | |
| [video\_stream\_source\_t](#video-stream-source-t) `stream_source` | Current source (demux/memory). | |
| [video\_format\_t](#video-format-t) `video_format` | Current aspect ratio of stream. | |
| [video\_displayformat\_t](#video-displayformat-t) `display_format` | Applied cropping mode. | |

#### 6.2.3.1.1.9.3. Description

If `video_blank` is set `TRUE` video will be blanked out if the
channel is changed or if playback is stopped. Otherwise, the last picture
will be displayed. `play_state` indicates if the video is currently
frozen, stopped, or being played back. The `stream_source` corresponds
to the selected source for the video stream. It can come either from the
demultiplexer or from memory. The `video_format` indicates the aspect
ratio (one of 4:3 or 16:9) of the currently played video stream.
Finally, `display_format` corresponds to the applied cropping mode in
case the source video format is not the same as the format of the output
device.

---

### 6.2.3.1.1.10. video\_still\_picture

#### 6.2.3.1.1.10.1. Synopsis

```
struct video_still_picture {
char *iFrame;
int32_t size;
};
```

#### 6.2.3.1.1.10.2. Variables

|  |  |
| --- | --- |
| `char *iFrame` | Pointer to a single iframe in memory. |
| `int32_t size` | Size of the iframe. |

#### 6.2.3.1.1.10.3. Description

An I-frame displayed via the [VIDEO\_STILLPICTURE](#video-stillpicture) call is passed on
within this structure.

---

### 6.2.3.1.1.11. video capabilities

#### 6.2.3.1.1.11.1. Synopsis

```
#define VIDEO_CAP_MPEG1   1
#define VIDEO_CAP_MPEG2   2
#define VIDEO_CAP_SYS     4
#define VIDEO_CAP_PROG    8
```

#### 6.2.3.1.1.11.2. Constants

Bit definitions for capabilities:

|  |  |  |
| --- | --- | --- |
| `VIDEO_CAP_MPEG1` | The hardware can decode MPEG1. | |
| `VIDEO_CAP_MPEG2` | The hardware can decode MPEG2. | |
| `VIDEO_CAP_SYS` | The video device accepts system stream.  You still have to open the video and the audio device but only send the stream to the video device. | |
| `VIDEO_CAP_PROG` | The video device accepts program stream.  You still have to open the video and the audio device but only send the stream to the video device. | |

#### 6.2.3.1.1.11.3. Description

A call to [VIDEO\_GET\_CAPABILITIES](#video-get-capabilities) returns an unsigned integer with the
following bits set according to the hardware’s capabilities.

---

## 6.2.3.1.2. Video Function Calls

### 6.2.3.1.2.1. VIDEO\_STOP

#### 6.2.3.1.2.1.1. Synopsis

VIDEO\_STOP

```
int ioctl(fd, VIDEO_STOP, int mode)
```

#### 6.2.3.1.2.1.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_STOP` for this command. | |
| `int mode` | Indicates how the screen shall be handled. | |
| TRUE ( != 0 ) | Blank screen when stop. |
| FALSE ( == 0 ) | Show last decoded frame. |

#### 6.2.3.1.2.1.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for Digital TV devices only. To control a V4L2 decoder use
the V4L2 [ioctl VIDIOC\_DECODER\_CMD, VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) instead.

This ioctl call asks the Video Device to stop playing the current
stream. Depending on the input parameter, the screen can be blanked out
or displaying the last decoded frame.

#### 6.2.3.1.2.1.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.2. VIDEO\_PLAY

#### 6.2.3.1.2.2.1. Synopsis

VIDEO\_PLAY

```
int ioctl(fd, VIDEO_PLAY)
```

#### 6.2.3.1.2.2.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_PLAY` for this command. | |

#### 6.2.3.1.2.2.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for Digital TV devices only. To control a V4L2 decoder use
the V4L2 [ioctl VIDIOC\_DECODER\_CMD, VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) instead.

This ioctl call asks the Video Device to start playing a video stream
from the selected source.

#### 6.2.3.1.2.2.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.3. VIDEO\_FREEZE

#### 6.2.3.1.2.3.1. Synopsis

VIDEO\_FREEZE

```
int ioctl(fd, VIDEO_FREEZE)
```

#### 6.2.3.1.2.3.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_FREEZE` for this command. | |

#### 6.2.3.1.2.3.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for Digital TV devices only. To control a V4L2 decoder use
the V4L2 [ioctl VIDIOC\_DECODER\_CMD, VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) instead.

This ioctl call suspends the live video stream being played, if
VIDEO\_SOURCE\_DEMUX is selected. Decoding and playing are frozen.
It is then possible to restart the decoding and playing process of the
video stream using the [VIDEO\_CONTINUE](#video-continue) command.
If VIDEO\_SOURCE\_MEMORY is selected in the ioctl call
[VIDEO\_SELECT\_SOURCE](#video-select-source), the Digital TV subsystem will not decode any more
data until the ioctl call [VIDEO\_CONTINUE](#video-continue) or [VIDEO\_PLAY](#video-play) is performed.

#### 6.2.3.1.2.3.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.4. VIDEO\_CONTINUE

#### 6.2.3.1.2.4.1. Synopsis

VIDEO\_CONTINUE

```
int ioctl(fd, VIDEO_CONTINUE)
```

#### 6.2.3.1.2.4.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_CONTINUE` for this command. | |

#### 6.2.3.1.2.4.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for Digital TV devices only. To control a V4L2 decoder use
the V4L2 [ioctl VIDIOC\_DECODER\_CMD, VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) instead.

This ioctl call restarts decoding and playing processes of the video
stream which was played before a call to [VIDEO\_FREEZE](#video-freeze) was made.

#### 6.2.3.1.2.4.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.5. VIDEO\_SELECT\_SOURCE

#### 6.2.3.1.2.5.1. Synopsis

VIDEO\_SELECT\_SOURCE

```
int ioctl(fd, VIDEO_SELECT_SOURCE, video_stream_source_t source)
```

#### 6.2.3.1.2.5.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_SELECT_SOURCE` for this command. | |
| [video\_stream\_source\_t](#video-stream-source-t) `source` | Indicates which source shall be used for the Video stream. | |

#### 6.2.3.1.2.5.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for Digital TV devices only. This ioctl was also supported
by the V4L2 ivtv driver, but that has been replaced by the ivtv-specific
`IVTV_IOC_PASSTHROUGH_MODE` ioctl.

This ioctl call informs the video device which source shall be used for
the input data. The possible sources are demux or memory. If memory is
selected, the data is fed to the video device through the write command
using the struct [video\_stream\_source\_t](#video-stream-source-t). If demux is selected, the data
is directly transferred from the onboard demux-device to the decoder.

The data fed to the decoder is also controlled by the PID-filter.
Output selection: `dmx_output` `DMX_OUT_DECODER`.

#### 6.2.3.1.2.5.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.6. VIDEO\_SET\_BLANK

#### 6.2.3.1.2.6.1. Synopsis

VIDEO\_SET\_BLANK

```
int ioctl(fd, VIDEO_SET_BLANK, int mode)
```

#### 6.2.3.1.2.6.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_SET_BLANK` for this command. | |
| `int mode` | Indicates if the screen shall be blanked. | |
| TRUE ( != 0 ) | Blank screen when stop. |
| FALSE ( == 0 ) | Show last decoded frame. |

#### 6.2.3.1.2.6.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Video Device to blank out the picture.

#### 6.2.3.1.2.6.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.7. VIDEO\_GET\_STATUS

#### 6.2.3.1.2.7.1. Synopsis

VIDEO\_GET\_STATUS

```
int ioctl(fd, int request = VIDEO_GET_STATUS,
struct video_status *status)
```

#### 6.2.3.1.2.7.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_GET_STATUS` for this command. | |
| `struct` [video\_status](#video-status) `*status` | Returns the current status of the Video Device. | |

#### 6.2.3.1.2.7.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Video Device to return the current status of
the device.

#### 6.2.3.1.2.7.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.8. VIDEO\_GET\_EVENT

#### 6.2.3.1.2.8.1. Synopsis

VIDEO\_GET\_EVENT

```
int ioctl(fd, int request = VIDEO_GET_EVENT,
struct video_event *ev)
```

#### 6.2.3.1.2.8.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_GET_EVENT` for this command. | |
| `struct` [video\_event](#video-event) `*ev` | Points to the location where the event, if any, is to be stored. | |

#### 6.2.3.1.2.8.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is for DVB devices only. To get events from a V4L2 decoder
use the V4L2 [ioctl VIDIOC\_DQEVENT](../v4l/vidioc-dqevent.html#vidioc-dqevent) ioctl instead.

This ioctl call returns an event of type [video\_event](#video-event) if available. A
certain number of the latest events will be cued and returned in order of
occurrence. Older events may be discarded if not fetched in time. If
an event is not available, the behavior depends on whether the device is
in blocking or non-blocking mode. In the latter case, the call fails
immediately with errno set to `EWOULDBLOCK`. In the former case, the
call blocks until an event becomes available. The standard Linux `poll()`
and/or `select()` system calls can be used with the device file descriptor
to watch for new events. For `select()`, the file descriptor should be
included in the exceptfds argument, and for `poll()`, POLLPRI should be
specified as the wake-up condition. Read-only permissions are sufficient
for this ioctl call.

#### 6.2.3.1.2.8.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

|  |  |  |
| --- | --- | --- |
| `EWOULDBLOCK` | There is no event pending, and the device is in non-blocking mode. | |
| `EOVERFLOW` | Overflow in event queue - one or more events were lost. | |

---

### 6.2.3.1.2.9. VIDEO\_SET\_DISPLAY\_FORMAT

#### 6.2.3.1.2.9.1. Synopsis

VIDEO\_SET\_DISPLAY\_FORMAT

```
int ioctl(fd, int request = VIDEO_SET_DISPLAY_FORMAT,
video_display_format_t format)
```

#### 6.2.3.1.2.9.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_SET_DISPLAY_FORMAT` for this command. | |
| [video\_displayformat\_t](#video-displayformat-t) `format` | Selects the video format to be used. | |

#### 6.2.3.1.2.9.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Video Device to select the video format to be
applied by the MPEG chip on the video.

#### 6.2.3.1.2.9.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.10. VIDEO\_STILLPICTURE

#### 6.2.3.1.2.10.1. Synopsis

VIDEO\_STILLPICTURE

```
int ioctl(fd, int request = VIDEO_STILLPICTURE,
struct video_still_picture *sp)
```

#### 6.2.3.1.2.10.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_STILLPICTURE` for this command. | |
| `struct` [video\_still\_picture](#video-still-picture) `*sp` | Pointer to the location where the `struct with` the I-frame and size is stored. | |

#### 6.2.3.1.2.10.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Video Device to display a still picture
(I-frame). The input data shall be the section of an elementary video
stream containing an I-frame. Typically this section is extracted from a
TS or PES recording. Resolution and codec (see [video capabilities](#video-capabilities)) must
be supported by the device. If the pointer is NULL, then the current
displayed still picture is blanked.

e.g. The AV7110 supports MPEG1 and MPEG2 with the common PAL-SD
resolutions.

#### 6.2.3.1.2.10.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.11. VIDEO\_FAST\_FORWARD

#### 6.2.3.1.2.11.1. Synopsis

VIDEO\_FAST\_FORWARD

```
int ioctl(fd, int request = VIDEO_FAST_FORWARD, int nFrames)
```

#### 6.2.3.1.2.11.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_FAST_FORWARD` for this command. | |
| `int nFrames` | The number of frames to skip. | |

#### 6.2.3.1.2.11.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the Video Device to skip decoding of N number of
I-frames. This call can only be used if `VIDEO_SOURCE_MEMORY` is
selected.

#### 6.2.3.1.2.11.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

|  |  |
| --- | --- |
| `EPERM` | Mode `VIDEO_SOURCE_MEMORY` not selected. |

---

### 6.2.3.1.2.12. VIDEO\_SLOWMOTION

#### 6.2.3.1.2.12.1. Synopsis

VIDEO\_SLOWMOTION

```
int ioctl(fd, int request = VIDEO_SLOWMOTION, int nFrames)
```

#### 6.2.3.1.2.12.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_SLOWMOTION` for this command. | |
| `int nFrames` | The number of times to repeat each frame. | |

#### 6.2.3.1.2.12.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the video device to repeat decoding frames N number
of times. This call can only be used if `VIDEO_SOURCE_MEMORY` is
selected.

#### 6.2.3.1.2.12.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

|  |  |
| --- | --- |
| `EPERM` | Mode `VIDEO_SOURCE_MEMORY` not selected. |

---

### 6.2.3.1.2.13. VIDEO\_GET\_CAPABILITIES

#### 6.2.3.1.2.13.1. Synopsis

VIDEO\_GET\_CAPABILITIES

```
int ioctl(fd, int request = VIDEO_GET_CAPABILITIES, unsigned int *cap)
```

#### 6.2.3.1.2.13.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_GET_CAPABILITIES` for this command. | |
| `unsigned int *cap` | Pointer to a location where to store the capability information. | |

#### 6.2.3.1.2.13.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call asks the video device about its decoding capabilities.
On success it returns an integer which has bits set according to the
defines in [video capabilities](#video-capabilities).

#### 6.2.3.1.2.13.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.14. VIDEO\_CLEAR\_BUFFER

#### 6.2.3.1.2.14.1. Synopsis

VIDEO\_CLEAR\_BUFFER

```
int ioctl(fd, int request = VIDEO_CLEAR_BUFFER)
```

#### 6.2.3.1.2.14.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_CLEAR_BUFFER` for this command. | |

#### 6.2.3.1.2.14.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl call clears all video buffers in the driver and in the
decoder hardware.

#### 6.2.3.1.2.14.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.15. VIDEO\_SET\_STREAMTYPE

#### 6.2.3.1.2.15.1. Synopsis

VIDEO\_SET\_STREAMTYPE

```
int ioctl(fd, int request = VIDEO_SET_STREAMTYPE, int type)
```

#### 6.2.3.1.2.15.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_SET_STREAMTYPE` for this command. | |
| `int type` | Stream type. | |

#### 6.2.3.1.2.15.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl tells the driver which kind of stream to expect being written
to it.
Intelligent decoder might also not support or ignore (like the AV7110)
this call and determine the stream type themselves.

Currently used stream types:

| Codec | Stream type |
| --- | --- |
| MPEG2 | 0 |
| MPEG4 h.264 | 1 |
| VC1 | 3 |
| MPEG4 Part2 | 4 |
| VC1 SM | 5 |
| MPEG1 | 6 |
| HEVC h.265 | 7  DREAMBOX: 22 |
| AVS | 16 |
| AVS2 | 40 |

Not every decoder supports all stream types.

#### 6.2.3.1.2.15.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.16. VIDEO\_SET\_FORMAT

#### 6.2.3.1.2.16.1. Synopsis

VIDEO\_SET\_FORMAT

```
int ioctl(fd, int request = VIDEO_SET_FORMAT, video_format_t format)
```

#### 6.2.3.1.2.16.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_SET_FORMAT` for this command. | |
| [video\_format\_t](#video-format-t) `format` | Video format of TV as defined in section [video\_format\_t](#video-format-t). | |

#### 6.2.3.1.2.16.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl sets the screen format (aspect ratio) of the connected output
device (TV) so that the output of the decoder can be adjusted
accordingly.

#### 6.2.3.1.2.16.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.17. VIDEO\_GET\_SIZE

#### 6.2.3.1.2.17.1. Synopsis

VIDEO\_GET\_SIZE

```
int ioctl(int fd, int request = VIDEO_GET_SIZE, video_size_t *size)
```

#### 6.2.3.1.2.17.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call, to [open()](#open). | |
| `int request` | Equals `VIDEO_GET_SIZE` for this command. | |
| [video\_size\_t](#video-size-t) `*size` | Returns the size and aspect ratio. | |

#### 6.2.3.1.2.17.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl returns the size and aspect ratio.

#### 6.2.3.1.2.17.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.18. VIDEO\_GET\_PTS

#### 6.2.3.1.2.18.1. Synopsis

VIDEO\_GET\_PTS

```
int ioctl(int fd, int request = VIDEO_GET_PTS, __u64 *pts)
```

#### 6.2.3.1.2.18.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_GET_PTS` for this command. | |
| `__u64 *pts` | Returns the 33-bit timestamp as defined in ITU T-REC-H.222.0 / ISO/IEC 13818-1.  The PTS should belong to the currently played frame if possible, but may also be a value close to it like the PTS of the last decoded frame or the last PTS extracted by the PES parser. | |

#### 6.2.3.1.2.18.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

For V4L2 decoders this ioctl has been replaced by the
`V4L2_CID_MPEG_VIDEO_DEC_PTS` control.

This ioctl call asks the Video Device to return the current PTS
timestamp.

#### 6.2.3.1.2.18.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.19. VIDEO\_GET\_FRAME\_COUNT

#### 6.2.3.1.2.19.1. Synopsis

VIDEO\_GET\_FRAME\_COUNT

```
int ioctl(int fd, VIDEO_GET_FRAME_COUNT, __u64 *pts)
```

#### 6.2.3.1.2.19.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_GET_FRAME_COUNT` for this command. | |
| `__u64 *pts` | Returns the number of frames displayed since the decoder was started. | |

#### 6.2.3.1.2.19.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

For V4L2 decoders this ioctl has been replaced by the
`V4L2_CID_MPEG_VIDEO_DEC_FRAME` control.

This ioctl call asks the Video Device to return the number of displayed
frames since the decoder was started.

#### 6.2.3.1.2.19.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.20. VIDEO\_COMMAND

#### 6.2.3.1.2.20.1. Synopsis

VIDEO\_COMMAND

```
int ioctl(int fd, int request = VIDEO_COMMAND,
struct video_command *cmd)
```

#### 6.2.3.1.2.20.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_COMMAND` for this command. | |
| [struct video\_command](#struct-video-command) `*cmd` | Commands the decoder. | |

#### 6.2.3.1.2.20.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

For V4L2 decoders this ioctl has been replaced by the
[ioctl VIDIOC\_DECODER\_CMD, VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) ioctl.

This ioctl commands the decoder. The [struct video\_command](#struct-video-command) is a
subset of the `v4l2_decoder_cmd` struct, so refer to the
[ioctl VIDIOC\_DECODER\_CMD, VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) documentation for
more information.

#### 6.2.3.1.2.20.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.21. VIDEO\_TRY\_COMMAND

#### 6.2.3.1.2.21.1. Synopsis

VIDEO\_TRY\_COMMAND

```
int ioctl(int fd, int request = VIDEO_TRY_COMMAND,
struct video_command *cmd)
```

#### 6.2.3.1.2.21.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `VIDEO_TRY_COMMAND` for this command. | |
| [struct video\_command](#struct-video-command) `*cmd` | Try a decoder command. | |

#### 6.2.3.1.2.21.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

For V4L2 decoders this ioctl has been replaced by the
[VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) ioctl.

This ioctl tries a decoder command. The [struct video\_command](#struct-video-command) is a
subset of the `v4l2_decoder_cmd` struct, so refer to the
[VIDIOC\_TRY\_DECODER\_CMD](../v4l/vidioc-decoder-cmd.html#vidioc-decoder-cmd) documentation
for more information.

#### 6.2.3.1.2.21.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

---

### 6.2.3.1.2.22. open()

#### 6.2.3.1.2.22.1. Synopsis

```
#include <fcntl.h>
```

int open(const char \*deviceName, int flags)

#### 6.2.3.1.2.22.2. Arguments

|  |  |  |
| --- | --- | --- |
| `const char *deviceName` | Name of specific video device. | |
| `int flags` | A bit-wise OR of the following flags: | |
| `O_RDONLY` | read-only access |
| `O_RDWR` | read/write access |
| `O_NONBLOCK` | Open in non-blocking mode  (blocking mode is the default) |

#### 6.2.3.1.2.22.3. Description

This system call opens a named video device (e.g.
/dev/dvb/adapter?/video?) for subsequent use.

When an [`open()`](#c.dtv.legacy.video.open "dtv.legacy.video.open") call has succeeded, the device will be ready for use. The
significance of blocking or non-blocking mode is described in the
documentation for functions where there is a difference. It does not
affect the semantics of the [`open()`](#c.dtv.legacy.video.open "dtv.legacy.video.open") call itself. A device opened in
blocking mode can later be put into non-blocking mode (and vice versa)
using the F\_SETFL command of the fcntl system call. This is a standard
system call, documented in the Linux manual page for fcntl. Only one
user can open the Video Device in O\_RDWR mode. All other attempts to
open the device in this mode will fail, and an error-code will be
returned. If the Video Device is opened in O\_RDONLY mode, the only
ioctl call that can be used is [VIDEO\_GET\_STATUS](#video-get-status). All other call will
return an error code.

#### 6.2.3.1.2.22.4. Return Value

|  |  |  |
| --- | --- | --- |
| `ENODEV` | Device driver not loaded/available. | |
| `EINTERNAL` | Internal error. | |
| `EBUSY` | Device or resource busy. | |
| `EINVAL` | Invalid argument. | |

---

### 6.2.3.1.2.23. close()

#### 6.2.3.1.2.23.1. Synopsis

int close(int fd)

#### 6.2.3.1.2.23.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |

#### 6.2.3.1.2.23.3. Description

This system call closes a previously opened video device.

#### 6.2.3.1.2.23.4. Return Value

|  |  |
| --- | --- |
| `EBADF` | fd is not a valid open file descriptor. |

---

### 6.2.3.1.2.24. write()

#### 6.2.3.1.2.24.1. Synopsis

size\_t write(int fd, const void \*buf, size\_t count)

#### 6.2.3.1.2.24.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `void *buf` | Pointer to the buffer containing the PES data. | |
| `size_t count` | Size of buf. | |

#### 6.2.3.1.2.24.3. Description

This system call can only be used if VIDEO\_SOURCE\_MEMORY is selected
in the ioctl call [VIDEO\_SELECT\_SOURCE](#video-select-source). The data provided shall be in
PES format, unless the capability allows other formats. TS is the
most common format for storing DVB-data, it is usually supported too.
If O\_NONBLOCK is not specified the function will block until buffer space
is available. The amount of data to be transferred is implied by count.

Note

See: [DVB Data Formats](legacy_dvb_decoder_api.html#legacy-dvb-decoder-formats)

#### 6.2.3.1.2.24.4. Return Value

|  |  |  |
| --- | --- | --- |
| `EPERM` | Mode `VIDEO_SOURCE_MEMORY` not selected. | |
| `ENOMEM` | Attempted to write more data than the internal buffer can hold. | |
| `EBADF` | fd is not a valid open file descriptor. | |
