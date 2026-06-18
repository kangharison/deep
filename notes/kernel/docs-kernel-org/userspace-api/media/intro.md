# Introduction

> 출처(원문): https://docs.kernel.org/userspace-api/media/intro.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Introduction

This document covers the Linux Kernel to Userspace API’s used by video
and radio streaming devices, including video cameras, analog and digital
TV receiver cards, AM/FM receiver cards, Software Defined Radio (SDR),
streaming capture and output devices, codec devices and remote controllers.

A typical media device hardware is shown at [Typical Media Device](#typical-media-device).

![typical_media_device.svg](../../_images/typical_media_device.svg)

Typical Media Device

The media infrastructure API was designed to control such devices. It is
divided into five parts.

1. The [first part](v4l/v4l2.html#v4l2spec) covers radio, video capture and output,
   cameras, analog TV devices and codecs.
2. The [second part](dvb/dvbapi.html#dvbapi) covers the API used for digital TV and
   Internet reception via one of the several digital tv standards. While it is
   called as DVB API, in fact it covers several different video standards
   including DVB-T/T2, DVB-S/S2, DVB-C, ATSC, ISDB-T, ISDB-S, DTMB, etc. The
   complete list of supported standards can be found at
   [`fe_delivery_system`](dvb/frontend-header.html#c.fe_delivery_system "fe_delivery_system").
3. The [third part](rc/remote_controllers.html#remote-controllers) covers the Remote Controller API.
4. The [fourth part](mediactl/media-controller.html#media-controller) covers the Media Controller API.
5. The [fifth part](cec/cec-api.html#cec) covers the CEC (Consumer Electronics Control) API.

It should also be noted that a media device may also have audio components, like
mixers, PCM capture, PCM playback, etc, which are controlled via ALSA API. For
additional information and for the latest development code, see:
<https://linuxtv.org>. For discussing improvements,
reporting troubles, sending new drivers, etc, please mail to: [Linux Media
Mailing List (LMML)](http://vger.kernel.org/vger-lists.html#linux-media).
