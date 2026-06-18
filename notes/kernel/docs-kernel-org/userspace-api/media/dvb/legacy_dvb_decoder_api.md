# 6.2.Legacy DVB MPEG Decoder APIs

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/legacy_dvb_decoder_api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.2. Legacy DVB MPEG Decoder APIs

## 6.2.1. General Notes

This API has originally been designed for DVB only and is therefore limited to
the [Data Formats](#legacy-dvb-decoder-formats) used in such digital TV-broadcastsystems.

To circumvent this limitations the more versatile [V4L2](../v4l/v4l2.html#v4l2spec) API has
been designed. Which replaces this part of the DVB API.

Nevertheless there have been projects build around this API.
To ensure compatibility this API is kept as it is.

Attention

Do **not** use this API in new drivers!

For audio and video use the [V4L2](../v4l/v4l2.html#v4l2spec) and ALSA APIs.

Pipelines should be set up using the [Media Controller API](../mediactl/media-controller.html#media-controller).

Practically the decoders seem to be treated differently. The application typically
knows which decoder is in use or it is specially written for one decoder type.
Querying capabilities are rarely used because they are already known.

## 6.2.2. Data Formats

The API has been designed for DVB and compatible broadcastsystems.
Because of that fact the only supported data formats are ISO/IEC 13818-1
compatible MPEG streams. The supported payloads may vary depending on the
used decoder.

Timestamps are always MPEG PTS as defined in ITU T-REC-H.222.0 /
ISO/IEC 13818-1, if not otherwise noted.

For storing recordings typically TS streams are used, in lesser extent PES.
Both variants are commonly accepted for playback, but it may be driver dependent.

## 6.2.3. Table of Contents

* [6.2.3.1. DVB Video Device](legacy_dvb_video.html)
  + [6.2.3.1.1. Video Data Types](legacy_dvb_video.html#video-data-types)
  + [6.2.3.1.2. Video Function Calls](legacy_dvb_video.html#video-function-calls)
* [6.2.3.2. DVB Audio Device](legacy_dvb_audio.html)
  + [6.2.3.2.1. Audio Data Types](legacy_dvb_audio.html#audio-data-types)
  + [6.2.3.2.2. Audio Function Calls](legacy_dvb_audio.html#audio-function-calls)
* [6.2.3.3. DVB OSD Device](legacy_dvb_osd.html)
  + [6.2.3.3.1. OSD Data Types](legacy_dvb_osd.html#osd-data-types)
  + [6.2.3.3.2. OSD Function Calls](legacy_dvb_osd.html#osd-function-calls)
