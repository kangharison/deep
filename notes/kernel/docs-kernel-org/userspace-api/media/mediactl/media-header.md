# 6.Media controller uAPI symbols

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-header.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6. Media controller uAPI symbols

## 6.1. IOCTL Commands

* [MEDIA\_IOC\_DEVICE\_INFO](media-ioc-device-info.html#media-ioc-device-info): include/uapi/linux/media.h#371
* [MEDIA\_IOC\_ENUM\_ENTITIES](media-ioc-enum-entities.html#media-ioc-enum-entities): include/uapi/linux/media.h#372
* [MEDIA\_IOC\_ENUM\_LINKS](media-ioc-enum-links.html#media-ioc-enum-links): include/uapi/linux/media.h#373
* [MEDIA\_IOC\_G\_TOPOLOGY](media-ioc-g-topology.html#media-ioc-g-topology): include/uapi/linux/media.h#375
* [MEDIA\_IOC\_REQUEST\_ALLOC](media-ioc-request-alloc.html#media-ioc-request-alloc): include/uapi/linux/media.h#376
* [MEDIA\_IOC\_SETUP\_LINK](media-ioc-setup-link.html#media-ioc-setup-link): include/uapi/linux/media.h#374
* [MEDIA\_REQUEST\_IOC\_QUEUE](media-request-ioc-queue.html#media-request-ioc-queue): include/uapi/linux/media.h#382
* [MEDIA\_REQUEST\_IOC\_REINIT](media-request-ioc-reinit.html#media-request-ioc-reinit): include/uapi/linux/media.h#383

## 6.2. Macros and Definitions

* [MEDIA\_ENT\_FL\_CONNECTOR](media-types.html#media-ent-fl-connector): include/uapi/linux/media.h#144
* [MEDIA\_ENT\_FL\_DEFAULT](media-types.html#media-ent-fl-default): include/uapi/linux/media.h#143
* [MEDIA\_ENT\_F\_ATV\_DECODER](media-types.html#media-ent-f-atv-decoder): include/uapi/linux/media.h#138
* [MEDIA\_ENT\_F\_AUDIO\_CAPTURE](media-types.html#media-ent-f-audio-capture): include/uapi/linux/media.h#112
* [MEDIA\_ENT\_F\_AUDIO\_MIXER](media-types.html#media-ent-f-audio-mixer): include/uapi/linux/media.h#114
* [MEDIA\_ENT\_F\_AUDIO\_PLAYBACK](media-types.html#media-ent-f-audio-playback): include/uapi/linux/media.h#113
* [MEDIA\_ENT\_F\_CAM\_SENSOR](media-types.html#media-ent-f-cam-sensor): include/uapi/linux/media.h#84
* [MEDIA\_ENT\_F\_CONN\_COMPOSITE](media-types.html#media-ent-f-conn-composite): include/uapi/linux/media.h#281
* [MEDIA\_ENT\_F\_CONN\_RF](media-types.html#media-ent-f-conn-rf): include/uapi/linux/media.h#279
* [MEDIA\_ENT\_F\_CONN\_SVIDEO](media-types.html#media-ent-f-conn-svideo): include/uapi/linux/media.h#280
* [MEDIA\_ENT\_F\_DTV\_CA](media-types.html#media-ent-f-dtv-ca): include/uapi/linux/media.h#70
* [MEDIA\_ENT\_F\_DTV\_DEMOD](media-types.html#media-ent-f-dtv-demod): include/uapi/linux/media.h#68
* [MEDIA\_ENT\_F\_DTV\_NET\_DECAP](media-types.html#media-ent-f-dtv-net-decap): include/uapi/linux/media.h#71
* [MEDIA\_ENT\_F\_DV\_DECODER](media-types.html#media-ent-f-dv-decoder): include/uapi/linux/media.h#139
* [MEDIA\_ENT\_F\_DV\_ENCODER](media-types.html#media-ent-f-dv-encoder): include/uapi/linux/media.h#140
* [MEDIA\_ENT\_F\_FLASH](media-types.html#media-ent-f-flash): include/uapi/linux/media.h#85
* [MEDIA\_ENT\_F\_IF\_AUD\_DECODER](media-types.html#media-ent-f-if-aud-decoder): include/uapi/linux/media.h#107
* [MEDIA\_ENT\_F\_IF\_VID\_DECODER](media-types.html#media-ent-f-if-vid-decoder): include/uapi/linux/media.h#106
* [MEDIA\_ENT\_F\_IO\_DTV](media-types.html#media-ent-f-io-dtv): include/uapi/linux/media.h#77
* [MEDIA\_ENT\_F\_IO\_SWRADIO](media-types.html#media-ent-f-io-swradio): include/uapi/linux/media.h#79
* [MEDIA\_ENT\_F\_IO\_V4L](media-types.html#media-ent-f-io-v4l): include/uapi/linux/media.h#76
* [MEDIA\_ENT\_F\_IO\_VBI](media-types.html#media-ent-f-io-vbi): include/uapi/linux/media.h#78
* [MEDIA\_ENT\_F\_LENS](media-types.html#media-ent-f-lens): include/uapi/linux/media.h#86
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_COMPOSER](media-types.html#media-ent-f-proc-video-composer): include/uapi/linux/media.h#119
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_DECODER](media-types.html#media-ent-f-proc-video-decoder): include/uapi/linux/media.h#126
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_ENCODER](media-types.html#media-ent-f-proc-video-encoder): include/uapi/linux/media.h#125
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_ISP](media-types.html#media-ent-f-proc-video-isp): include/uapi/linux/media.h#127
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_LUT](media-types.html#media-ent-f-proc-video-lut): include/uapi/linux/media.h#122
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_PIXEL\_ENC\_CONV](media-types.html#media-ent-f-proc-video-pixel-enc-conv): include/uapi/linux/media.h#121
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_PIXEL\_FORMATTER](media-types.html#media-ent-f-proc-video-pixel-formatter): include/uapi/linux/media.h#120
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_SCALER](media-types.html#media-ent-f-proc-video-scaler): include/uapi/linux/media.h#123
* [MEDIA\_ENT\_F\_PROC\_VIDEO\_STATISTICS](media-types.html#media-ent-f-proc-video-statistics): include/uapi/linux/media.h#124
* [MEDIA\_ENT\_F\_TS\_DEMUX](media-types.html#media-ent-f-ts-demux): include/uapi/linux/media.h#69
* [MEDIA\_ENT\_F\_TUNER](media-types.html#media-ent-f-tuner): include/uapi/linux/media.h#98
* [MEDIA\_ENT\_F\_UNKNOWN](media-types.html#media-ent-f-unknown): include/uapi/linux/media.h#56
* [MEDIA\_ENT\_F\_V4L2\_SUBDEV\_UNKNOWN](media-types.html#media-ent-f-v4l2-subdev-unknown): include/uapi/linux/media.h#63
* [MEDIA\_ENT\_F\_VID\_IF\_BRIDGE](media-types.html#media-ent-f-vid-if-bridge): include/uapi/linux/media.h#133
* [MEDIA\_ENT\_F\_VID\_MUX](media-types.html#media-ent-f-vid-mux): include/uapi/linux/media.h#132
* [MEDIA\_ENT\_ID\_FLAG\_NEXT](media-ioc-enum-entities.html#media-ent-id-flag-next): include/uapi/linux/media.h#147
* [MEDIA\_INTF\_T\_ALSA\_COMPRESS](media-types.html#media-intf-t-alsa-compress): include/uapi/linux/media.h#428
* [MEDIA\_INTF\_T\_ALSA\_CONTROL](media-types.html#media-intf-t-alsa-control): include/uapi/linux/media.h#267
* [MEDIA\_INTF\_T\_ALSA\_HWDEP](media-types.html#media-intf-t-alsa-hwdep): include/uapi/linux/media.h#430
* [MEDIA\_INTF\_T\_ALSA\_PCM\_CAPTURE](media-types.html#media-intf-t-alsa-pcm-capture): include/uapi/linux/media.h#265
* [MEDIA\_INTF\_T\_ALSA\_PCM\_PLAYBACK](media-types.html#media-intf-t-alsa-pcm-playback): include/uapi/linux/media.h#266
* [MEDIA\_INTF\_T\_ALSA\_RAWMIDI](media-types.html#media-intf-t-alsa-rawmidi): include/uapi/linux/media.h#429
* [MEDIA\_INTF\_T\_ALSA\_SEQUENCER](media-types.html#media-intf-t-alsa-sequencer): include/uapi/linux/media.h#431
* [MEDIA\_INTF\_T\_ALSA\_TIMER](media-types.html#media-intf-t-alsa-timer): include/uapi/linux/media.h#432
* [MEDIA\_INTF\_T\_DVB\_CA](media-types.html#media-intf-t-dvb-ca): include/uapi/linux/media.h#254
* [MEDIA\_INTF\_T\_DVB\_DEMUX](media-types.html#media-intf-t-dvb-demux): include/uapi/linux/media.h#252
* [MEDIA\_INTF\_T\_DVB\_DVR](media-types.html#media-intf-t-dvb-dvr): include/uapi/linux/media.h#253
* [MEDIA\_INTF\_T\_DVB\_FE](media-types.html#media-intf-t-dvb-fe): include/uapi/linux/media.h#251
* [MEDIA\_INTF\_T\_DVB\_NET](media-types.html#media-intf-t-dvb-net): include/uapi/linux/media.h#255
* [MEDIA\_INTF\_T\_V4L\_RADIO](media-types.html#media-intf-t-v4l-radio): include/uapi/linux/media.h#259
* [MEDIA\_INTF\_T\_V4L\_SUBDEV](media-types.html#media-intf-t-v4l-subdev): include/uapi/linux/media.h#260
* [MEDIA\_INTF\_T\_V4L\_SWRADIO](media-types.html#media-intf-t-v4l-swradio): include/uapi/linux/media.h#261
* [MEDIA\_INTF\_T\_V4L\_TOUCH](media-types.html#media-intf-t-v4l-touch): include/uapi/linux/media.h#262
* [MEDIA\_INTF\_T\_V4L\_VBI](media-types.html#media-intf-t-v4l-vbi): include/uapi/linux/media.h#258
* [MEDIA\_INTF\_T\_V4L\_VIDEO](media-types.html#media-intf-t-v4l-video): include/uapi/linux/media.h#257
* [MEDIA\_LNK\_FL\_ANCILLARY\_LINK](media-types.html#media-lnk-fl-ancillary-link): include/uapi/linux/media.h#226
* [MEDIA\_LNK\_FL\_DATA\_LINK](media-types.html#media-lnk-fl-data-link): include/uapi/linux/media.h#224
* [MEDIA\_LNK\_FL\_DYNAMIC](media-types.html#media-lnk-fl-dynamic): include/uapi/linux/media.h#221
* [MEDIA\_LNK\_FL\_ENABLED](media-types.html#media-lnk-fl-enabled): include/uapi/linux/media.h#219
* [MEDIA\_LNK\_FL\_IMMUTABLE](media-types.html#media-lnk-fl-immutable): include/uapi/linux/media.h#220
* [MEDIA\_LNK\_FL\_INTERFACE\_LINK](media-types.html#media-lnk-fl-interface-link): include/uapi/linux/media.h#225
* [MEDIA\_LNK\_FL\_LINK\_TYPE](media-types.html#media-lnk-fl-link-type): include/uapi/linux/media.h#223
* [MEDIA\_PAD\_FL\_MUST\_CONNECT](media-types.html#media-pad-fl-must-connect): include/uapi/linux/media.h#210
* [MEDIA\_PAD\_FL\_SINK](media-types.html#media-pad-fl-sink): include/uapi/linux/media.h#208
* [MEDIA\_PAD\_FL\_SOURCE](media-types.html#media-pad-fl-source): include/uapi/linux/media.h#209

## 6.3. Structures

* [`media_device_info`](media-ioc-device-info.html#c.MC.media_device_info "MC.media_device_info"): include/uapi/linux/media.h#26
* [`media_entity_desc`](media-ioc-enum-entities.html#c.MC.media_entity_desc "MC.media_entity_desc"): include/uapi/linux/media.h#149
* [`media_link_desc`](media-ioc-enum-links.html#c.MC.media_link_desc "MC.media_link_desc"): include/uapi/linux/media.h#228
* [`media_links_enum`](media-ioc-enum-links.html#c.MC.media_links_enum "MC.media_links_enum"): include/uapi/linux/media.h#235
* [`media_pad_desc`](media-ioc-enum-links.html#c.MC.media_pad_desc "MC.media_pad_desc"): include/uapi/linux/media.h#212
* [`media_v2_entity`](media-ioc-g-topology.html#c.MC.media_v2_entity "MC.media_v2_entity"): include/uapi/linux/media.h#298
* [`media_v2_interface`](media-ioc-g-topology.html#c.MC.media_v2_interface "MC.media_v2_interface"): include/uapi/linux/media.h#312
* [`media_v2_intf_devnode`](media-ioc-g-topology.html#c.MC.media_v2_intf_devnode "MC.media_v2_intf_devnode"): include/uapi/linux/media.h#307
* [`media_v2_link`](media-ioc-g-topology.html#c.MC.media_v2_link "MC.media_v2_link"): include/uapi/linux/media.h#341
* [`media_v2_pad`](media-ioc-g-topology.html#c.MC.media_v2_pad "MC.media_v2_pad"): include/uapi/linux/media.h#333
* [`media_v2_topology`](media-ioc-g-topology.html#c.MC.media_v2_topology "MC.media_v2_topology"): include/uapi/linux/media.h#349
