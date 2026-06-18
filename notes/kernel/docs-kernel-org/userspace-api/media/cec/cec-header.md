# 4.CEC uAPI Symbols

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-header.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4. CEC uAPI Symbols

## 4.1. IOCTL Commands

* [CEC\_ADAP\_G\_CAPS](cec-ioc-adap-g-caps.html#cec-adap-g-caps): include/uapi/linux/cec.h#503
* [CEC\_ADAP\_G\_CONNECTOR\_INFO](cec-ioc-adap-g-conn-info.html#cec-adap-g-connector-info): include/uapi/linux/cec.h#545
* [CEC\_ADAP\_G\_LOG\_ADDRS](cec-ioc-adap-g-log-addrs.html#cec-adap-g-log-addrs): include/uapi/linux/cec.h#528
* [CEC\_ADAP\_G\_PHYS\_ADDR](cec-ioc-adap-g-phys-addr.html#cec-adap-g-phys-addr): include/uapi/linux/cec.h#517
* [CEC\_ADAP\_S\_LOG\_ADDRS](cec-ioc-adap-g-log-addrs.html#cec-adap-s-log-addrs): include/uapi/linux/cec.h#529
* [CEC\_ADAP\_S\_PHYS\_ADDR](cec-ioc-adap-g-phys-addr.html#cec-adap-s-phys-addr): include/uapi/linux/cec.h#518
* [CEC\_DQEVENT](cec-ioc-dqevent.html#cec-dqevent): include/uapi/linux/cec.h#536
* [CEC\_G\_MODE](cec-ioc-g-mode.html#cec-g-mode): include/uapi/linux/cec.h#541
* [CEC\_RECEIVE](cec-ioc-receive.html#cec-receive): include/uapi/linux/cec.h#533
* [CEC\_S\_MODE](cec-ioc-g-mode.html#cec-s-mode): include/uapi/linux/cec.h#542
* [CEC\_TRANSMIT](cec-ioc-receive.html#cec-transmit): include/uapi/linux/cec.h#532

## 4.2. Macros and Definitions

* [CEC\_CAP\_CONNECTOR\_INFO](cec-ioc-adap-g-caps.html#cec-cap-connector-info): include/uapi/linux/cec.h#346
* [CEC\_CAP\_LOG\_ADDRS](cec-ioc-adap-g-caps.html#cec-cap-log-addrs): include/uapi/linux/cec.h#330
* [CEC\_CAP\_MONITOR\_ALL](cec-ioc-adap-g-caps.html#cec-cap-monitor-all): include/uapi/linux/cec.h#340
* [CEC\_CAP\_MONITOR\_PIN](cec-ioc-adap-g-caps.html#cec-cap-monitor-pin): include/uapi/linux/cec.h#344
* [CEC\_CAP\_NEEDS\_HPD](cec-ioc-adap-g-caps.html#cec-cap-needs-hpd): include/uapi/linux/cec.h#342
* [CEC\_CAP\_PASSTHROUGH](cec-ioc-adap-g-caps.html#cec-cap-passthrough): include/uapi/linux/cec.h#336
* [CEC\_CAP\_PHYS\_ADDR](cec-ioc-adap-g-caps.html#cec-cap-phys-addr): include/uapi/linux/cec.h#328
* [CEC\_CAP\_RC](cec-ioc-adap-g-caps.html#cec-cap-rc): include/uapi/linux/cec.h#338
* [CEC\_CAP\_REPLY\_VENDOR\_ID](cec-ioc-adap-g-caps.html#cec-cap-reply-vendor-id): include/uapi/linux/cec.h#348
* [CEC\_CAP\_TRANSMIT](cec-ioc-adap-g-caps.html#cec-cap-transmit): include/uapi/linux/cec.h#332
* [CEC\_CONNECTOR\_TYPE\_DRM](cec-ioc-adap-g-conn-info.html#cec-connector-type-drm): include/uapi/linux/cec.h#419
* [CEC\_CONNECTOR\_TYPE\_NO\_CONNECTOR](cec-ioc-adap-g-conn-info.html#cec-connector-type-no-connector): include/uapi/linux/cec.h#418
* [CEC\_EVENT\_FL\_DROPPED\_EVENTS](cec-ioc-dqevent.html#cec-event-fl-dropped-events): include/uapi/linux/cec.h#453
* [CEC\_EVENT\_FL\_INITIAL\_STATE](cec-ioc-dqevent.html#cec-event-fl-initial-state): include/uapi/linux/cec.h#452
* [CEC\_EVENT\_LOST\_MSGS](cec-ioc-dqevent.html#cec-event-lost-msgs): include/uapi/linux/cec.h#444
* [CEC\_EVENT\_PIN\_5V\_HIGH](cec-ioc-dqevent.html#cec-event-pin-5v-high): include/uapi/linux/cec.h#450
* [CEC\_EVENT\_PIN\_5V\_LOW](cec-ioc-dqevent.html#cec-event-pin-5v-low): include/uapi/linux/cec.h#449
* [CEC\_EVENT\_PIN\_CEC\_HIGH](cec-ioc-dqevent.html#cec-event-pin-cec-high): include/uapi/linux/cec.h#446
* [CEC\_EVENT\_PIN\_CEC\_LOW](cec-ioc-dqevent.html#cec-event-pin-cec-low): include/uapi/linux/cec.h#445
* [CEC\_EVENT\_PIN\_HPD\_HIGH](cec-ioc-dqevent.html#cec-event-pin-hpd-high): include/uapi/linux/cec.h#448
* [CEC\_EVENT\_PIN\_HPD\_LOW](cec-ioc-dqevent.html#cec-event-pin-hpd-low): include/uapi/linux/cec.h#447
* [CEC\_EVENT\_STATE\_CHANGE](cec-ioc-dqevent.html#cec-event-state-change): include/uapi/linux/cec.h#439
* [CEC\_LOG\_ADDRS\_FL\_ALLOW\_RC\_PASSTHRU](cec-ioc-adap-g-log-addrs.html#cec-log-addrs-fl-allow-rc-passthru): include/uapi/linux/cec.h#403
* [CEC\_LOG\_ADDRS\_FL\_ALLOW\_UNREG\_FALLBACK](cec-ioc-adap-g-log-addrs.html#cec-log-addrs-fl-allow-unreg-fallback): include/uapi/linux/cec.h#401
* [CEC\_LOG\_ADDRS\_FL\_CDC\_ONLY](cec-ioc-adap-g-log-addrs.html#cec-log-addrs-fl-cdc-only): include/uapi/linux/cec.h#405
* [CEC\_LOG\_ADDR\_TYPE\_AUDIOSYSTEM](cec-ioc-adap-g-log-addrs.html#cec-log-addr-type-audiosystem): include/uapi/linux/cec.h#234
* [CEC\_LOG\_ADDR\_TYPE\_PLAYBACK](cec-ioc-adap-g-log-addrs.html#cec-log-addr-type-playback): include/uapi/linux/cec.h#233
* [CEC\_LOG\_ADDR\_TYPE\_RECORD](cec-ioc-adap-g-log-addrs.html#cec-log-addr-type-record): include/uapi/linux/cec.h#231
* [CEC\_LOG\_ADDR\_TYPE\_SPECIFIC](cec-ioc-adap-g-log-addrs.html#cec-log-addr-type-specific): include/uapi/linux/cec.h#235
* [CEC\_LOG\_ADDR\_TYPE\_TUNER](cec-ioc-adap-g-log-addrs.html#cec-log-addr-type-tuner): include/uapi/linux/cec.h#232
* [CEC\_LOG\_ADDR\_TYPE\_TV](cec-ioc-adap-g-log-addrs.html#cec-log-addr-type-tv): include/uapi/linux/cec.h#230
* [CEC\_LOG\_ADDR\_TYPE\_UNREGISTERED](cec-ioc-adap-g-log-addrs.html#cec-log-addr-type-unregistered): include/uapi/linux/cec.h#236
* [CEC\_MODE\_EXCL\_FOLLOWER](cec-ioc-g-mode.html#cec-mode-excl-follower): include/uapi/linux/cec.h#320
* [CEC\_MODE\_EXCL\_FOLLOWER\_PASSTHRU](cec-ioc-g-mode.html#cec-mode-excl-follower-passthru): include/uapi/linux/cec.h#321
* [CEC\_MODE\_EXCL\_INITIATOR](cec-ioc-g-mode.html#cec-mode-excl-initiator): include/uapi/linux/cec.h#314
* [CEC\_MODE\_FOLLOWER](cec-ioc-g-mode.html#cec-mode-follower): include/uapi/linux/cec.h#319
* [CEC\_MODE\_INITIATOR](cec-ioc-g-mode.html#cec-mode-initiator): include/uapi/linux/cec.h#313
* [CEC\_MODE\_MONITOR](cec-ioc-g-mode.html#cec-mode-monitor): include/uapi/linux/cec.h#323
* [CEC\_MODE\_MONITOR\_ALL](cec-ioc-g-mode.html#cec-mode-monitor-all): include/uapi/linux/cec.h#324
* [CEC\_MODE\_MONITOR\_PIN](cec-ioc-g-mode.html#cec-mode-monitor-pin): include/uapi/linux/cec.h#322
* [CEC\_MODE\_NO\_FOLLOWER](cec-ioc-g-mode.html#cec-mode-no-follower): include/uapi/linux/cec.h#318
* [CEC\_MODE\_NO\_INITIATOR](cec-ioc-g-mode.html#cec-mode-no-initiator): include/uapi/linux/cec.h#312
* [CEC\_MSG\_FL\_RAW](cec-ioc-receive.html#cec-msg-fl-raw): include/uapi/linux/cec.h#171
* [CEC\_MSG\_FL\_REPLY\_TO\_FOLLOWERS](cec-ioc-receive.html#cec-msg-fl-reply-to-followers): include/uapi/linux/cec.h#170
* [CEC\_MSG\_FL\_REPLY\_VENDOR\_ID](cec-ioc-receive.html#cec-msg-fl-reply-vendor-id): include/uapi/linux/cec.h#172
* [CEC\_RX\_STATUS\_ABORTED](cec-ioc-receive.html#cec-rx-status-aborted): include/uapi/linux/cec.h#187
* [CEC\_RX\_STATUS\_FEATURE\_ABORT](cec-ioc-receive.html#cec-rx-status-feature-abort): include/uapi/linux/cec.h#186
* [CEC\_RX\_STATUS\_OK](cec-ioc-receive.html#cec-rx-status-ok): include/uapi/linux/cec.h#184
* [CEC\_RX\_STATUS\_TIMEOUT](cec-ioc-receive.html#cec-rx-status-timeout): include/uapi/linux/cec.h#185
* [CEC\_TX\_STATUS\_ABORTED](cec-ioc-receive.html#cec-tx-status-aborted): include/uapi/linux/cec.h#181
* [CEC\_TX\_STATUS\_ARB\_LOST](cec-ioc-receive.html#cec-tx-status-arb-lost): include/uapi/linux/cec.h#176
* [CEC\_TX\_STATUS\_ERROR](cec-ioc-receive.html#cec-tx-status-error): include/uapi/linux/cec.h#179
* [CEC\_TX\_STATUS\_LOW\_DRIVE](cec-ioc-receive.html#cec-tx-status-low-drive): include/uapi/linux/cec.h#178
* [CEC\_TX\_STATUS\_MAX\_RETRIES](cec-ioc-receive.html#cec-tx-status-max-retries): include/uapi/linux/cec.h#180
* [CEC\_TX\_STATUS\_NACK](cec-ioc-receive.html#cec-tx-status-nack): include/uapi/linux/cec.h#177
* [CEC\_TX\_STATUS\_OK](cec-ioc-receive.html#cec-tx-status-ok): include/uapi/linux/cec.h#175
* [CEC\_TX\_STATUS\_TIMEOUT](cec-ioc-receive.html#cec-tx-status-timeout): include/uapi/linux/cec.h#182

## 4.3. Structures

* [`cec_caps`](cec-ioc-adap-g-caps.html#c.CEC.cec_caps "CEC.cec_caps"): include/uapi/linux/cec.h#358
* [`cec_connector_info`](cec-ioc-adap-g-conn-info.html#c.CEC.cec_connector_info "CEC.cec_connector_info"): include/uapi/linux/cec.h#428
* [`cec_drm_connector_info`](cec-ioc-adap-g-conn-info.html#c.CEC.cec_drm_connector_info "CEC.cec_drm_connector_info"): include/uapi/linux/cec.h#413
* [`cec_event`](cec-ioc-dqevent.html#c.CEC.cec_event "CEC.cec_event"): include/uapi/linux/cec.h#489
* [`cec_event_lost_msgs`](cec-ioc-dqevent.html#c.CEC.cec_event_lost_msgs "CEC.cec_event_lost_msgs"): include/uapi/linux/cec.h#476
* [`cec_event_state_change`](cec-ioc-dqevent.html#c.CEC.cec_event_state_change "CEC.cec_event_state_change"): include/uapi/linux/cec.h#466
* [`cec_log_addrs`](cec-ioc-adap-g-log-addrs.html#c.CEC.cec_log_addrs "CEC.cec_log_addrs"): include/uapi/linux/cec.h#384
* [`cec_msg`](cec-ioc-receive.html#c.CEC.cec_msg "CEC.cec_msg"): include/uapi/linux/cec.h#57
