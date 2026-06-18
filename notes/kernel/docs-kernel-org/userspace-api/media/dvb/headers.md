# 8.Digital TV uAPI symbols

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/headers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 8. Digital TV uAPI symbols

## [8.1. Frontend](#id14)

### [8.1.1. Enumeration values](#id15)

* [BANDWIDTH\_10\_MHZ](fe-bandwidth-t.html#bandwidth-10-mhz): include/uapi/linux/dvb/frontend.h#987
* [BANDWIDTH\_1\_712\_MHZ](fe-bandwidth-t.html#bandwidth-1-712-mhz): include/uapi/linux/dvb/frontend.h#988
* [BANDWIDTH\_5\_MHZ](fe-bandwidth-t.html#bandwidth-5-mhz): include/uapi/linux/dvb/frontend.h#986
* [BANDWIDTH\_6\_MHZ](fe-bandwidth-t.html#bandwidth-6-mhz): include/uapi/linux/dvb/frontend.h#984
* [BANDWIDTH\_7\_MHZ](fe-bandwidth-t.html#bandwidth-7-mhz): include/uapi/linux/dvb/frontend.h#983
* [BANDWIDTH\_8\_MHZ](fe-bandwidth-t.html#bandwidth-8-mhz): include/uapi/linux/dvb/frontend.h#982
* [BANDWIDTH\_AUTO](fe-bandwidth-t.html#bandwidth-auto): include/uapi/linux/dvb/frontend.h#985
* [FE\_ATSC](fe-type-t.html#fe-atsc): include/uapi/linux/dvb/frontend.h#98
* [FE\_OFDM](fe-type-t.html#fe-ofdm): include/uapi/linux/dvb/frontend.h#97
* [FE\_QAM](fe-type-t.html#fe-qam): include/uapi/linux/dvb/frontend.h#96
* [FE\_QPSK](fe-type-t.html#fe-qpsk): include/uapi/linux/dvb/frontend.h#95

### [8.1.2. Enumerations](#id16)

* [`atscmh_rs_code_mode`](frontend-header.html#c.atscmh_rs_code_mode "atscmh_rs_code_mode"): include/uapi/linux/dvb/frontend.h#778
* [`atscmh_rs_frame_ensemble`](frontend-header.html#c.atscmh_rs_frame_ensemble "atscmh_rs_frame_ensemble"): include/uapi/linux/dvb/frontend.h#747
* [`atscmh_rs_frame_mode`](frontend-header.html#c.atscmh_rs_frame_mode "atscmh_rs_frame_mode"): include/uapi/linux/dvb/frontend.h#765
* [`atscmh_sccc_block_mode`](frontend-header.html#c.atscmh_sccc_block_mode "atscmh_sccc_block_mode"): include/uapi/linux/dvb/frontend.h#718
* [`atscmh_sccc_code_mode`](frontend-header.html#c.atscmh_sccc_code_mode "atscmh_sccc_code_mode"): include/uapi/linux/dvb/frontend.h#735
* [`fe_bandwidth`](fe-bandwidth-t.html#c.fe_bandwidth "fe_bandwidth"): include/uapi/linux/dvb/frontend.h#981
* [`fe_caps`](frontend-header.html#c.fe_caps "fe_caps"): include/uapi/linux/dvb/frontend.h#57
* [`fe_code_rate`](frontend-header.html#c.fe_code_rate "fe_code_rate"): include/uapi/linux/dvb/frontend.h#306
* [`fe_delivery_system`](frontend-header.html#c.fe_delivery_system "fe_delivery_system"): include/uapi/linux/dvb/frontend.h#676
* [`fe_guard_interval`](frontend-header.html#c.fe_guard_interval "fe_guard_interval"): include/uapi/linux/dvb/frontend.h#449
* [`fe_hierarchy`](frontend-header.html#c.fe_hierarchy "fe_hierarchy"): include/uapi/linux/dvb/frontend.h#474
* [`fe_interleaving`](frontend-header.html#c.fe_interleaving "fe_interleaving"): include/uapi/linux/dvb/frontend.h#491
* [`fe_modulation`](frontend-header.html#c.fe_modulation "fe_modulation"): include/uapi/linux/dvb/frontend.h#369
* [`fe_pilot`](frontend-header.html#c.fe_pilot "fe_pilot"): include/uapi/linux/dvb/frontend.h#602
* [`fe_rolloff`](frontend-header.html#c.fe_rolloff "fe_rolloff"): include/uapi/linux/dvb/frontend.h#622
* [`fe_sec_mini_cmd`](frontend-header.html#c.fe_sec_mini_cmd "fe_sec_mini_cmd"): include/uapi/linux/dvb/frontend.h#218
* [`fe_sec_tone_mode`](frontend-header.html#c.fe_sec_tone_mode "fe_sec_tone_mode"): include/uapi/linux/dvb/frontend.h#205
* [`fe_sec_voltage`](frontend-header.html#c.fe_sec_voltage "fe_sec_voltage"): include/uapi/linux/dvb/frontend.h#193
* [`fe_spectral_inversion`](frontend-header.html#c.fe_spectral_inversion "fe_spectral_inversion"): include/uapi/linux/dvb/frontend.h#261
* [`fe_status`](frontend-header.html#c.fe_status "fe_status"): include/uapi/linux/dvb/frontend.h#237
* [`fe_transmit_mode`](frontend-header.html#c.fe_transmit_mode "fe_transmit_mode"): include/uapi/linux/dvb/frontend.h#419
* [`fe_type`](fe-type-t.html#c.fe_type "fe_type"): include/uapi/linux/dvb/frontend.h#94
* [`fecap_scale_params`](frontend-header.html#c.fecap_scale_params "fecap_scale_params"): include/uapi/linux/dvb/frontend.h#801

### [8.1.3. IOCTL Commands](#id17)

* [FE\_DISEQC\_RECV\_SLAVE\_REPLY](fe-diseqc-recv-slave-reply.html#fe-diseqc-recv-slave-reply): include/uapi/linux/dvb/frontend.h#941
* [FE\_DISEQC\_RESET\_OVERLOAD](fe-diseqc-reset-overload.html#fe-diseqc-reset-overload): include/uapi/linux/dvb/frontend.h#939
* [FE\_DISEQC\_SEND\_BURST](fe-diseqc-send-burst.html#fe-diseqc-send-burst): include/uapi/linux/dvb/frontend.h#942
* [FE\_DISEQC\_SEND\_MASTER\_CMD](fe-diseqc-send-master-cmd.html#fe-diseqc-send-master-cmd): include/uapi/linux/dvb/frontend.h#940
* [FE\_DISHNETWORK\_SEND\_LEGACY\_CMD](fe-dishnetwork-send-legacy-cmd.html#fe-dishnetwork-send-legacy-cmd): include/uapi/linux/dvb/frontend.h#957
* [FE\_ENABLE\_HIGH\_LNB\_VOLTAGE](fe-enable-high-lnb-voltage.html#fe-enable-high-lnb-voltage): include/uapi/linux/dvb/frontend.h#946
* [FE\_GET\_EVENT](fe-get-event.html#fe-get-event): include/uapi/linux/dvb/frontend.h#955
* [FE\_GET\_FRONTEND](fe-get-frontend.html#fe-get-frontend): include/uapi/linux/dvb/frontend.h#1056
* [FE\_GET\_INFO](fe-get-info.html#fe-get-info): include/uapi/linux/dvb/frontend.h#937
* [FE\_GET\_PROPERTY](fe-get-property.html#fe-get-property): include/uapi/linux/dvb/frontend.h#960
* [FE\_READ\_BER](fe-read-ber.html#fe-read-ber): include/uapi/linux/dvb/frontend.h#949
* [FE\_READ\_SIGNAL\_STRENGTH](fe-read-signal-strength.html#fe-read-signal-strength): include/uapi/linux/dvb/frontend.h#950
* [FE\_READ\_SNR](fe-read-snr.html#fe-read-snr): include/uapi/linux/dvb/frontend.h#951
* [FE\_READ\_STATUS](fe-read-status.html#fe-read-status): include/uapi/linux/dvb/frontend.h#948
* [FE\_READ\_UNCORRECTED\_BLOCKS](fe-read-uncorrected-blocks.html#fe-read-uncorrected-blocks): include/uapi/linux/dvb/frontend.h#952
* [FE\_SET\_FRONTEND](fe-set-frontend.html#fe-set-frontend): include/uapi/linux/dvb/frontend.h#1055
* [FE\_SET\_FRONTEND\_TUNE\_MODE](fe-set-frontend-tune-mode.html#fe-set-frontend-tune-mode): include/uapi/linux/dvb/frontend.h#954
* [FE\_SET\_PROPERTY](fe-get-property.html#fe-get-property): include/uapi/linux/dvb/frontend.h#959
* [FE\_SET\_TONE](fe-set-tone.html#fe-set-tone): include/uapi/linux/dvb/frontend.h#944
* [FE\_SET\_VOLTAGE](fe-set-voltage.html#fe-set-voltage): include/uapi/linux/dvb/frontend.h#945

### [8.1.4. Macros and Definitions](#id18)

* [DTV\_API\_VERSION](fe_property_parameters.html#dtv-api-version): include/uapi/linux/dvb/frontend.h#544
* [DTV\_ATSCMH\_FIC\_VER](fe_property_parameters.html#dtv-atscmh-fic-ver): include/uapi/linux/dvb/frontend.h#561
* [DTV\_ATSCMH\_NOG](fe_property_parameters.html#dtv-atscmh-nog): include/uapi/linux/dvb/frontend.h#563
* [DTV\_ATSCMH\_PARADE\_ID](fe_property_parameters.html#dtv-atscmh-parade-id): include/uapi/linux/dvb/frontend.h#562
* [DTV\_ATSCMH\_PRC](fe_property_parameters.html#dtv-atscmh-prc): include/uapi/linux/dvb/frontend.h#566
* [DTV\_ATSCMH\_RS\_CODE\_MODE\_PRI](fe_property_parameters.html#dtv-atscmh-rs-code-mode-pri): include/uapi/linux/dvb/frontend.h#569
* [DTV\_ATSCMH\_RS\_CODE\_MODE\_SEC](fe_property_parameters.html#dtv-atscmh-rs-code-mode-sec): include/uapi/linux/dvb/frontend.h#570
* [DTV\_ATSCMH\_RS\_FRAME\_ENSEMBLE](fe_property_parameters.html#dtv-atscmh-rs-frame-ensemble): include/uapi/linux/dvb/frontend.h#568
* [DTV\_ATSCMH\_RS\_FRAME\_MODE](fe_property_parameters.html#dtv-atscmh-rs-frame-mode): include/uapi/linux/dvb/frontend.h#567
* [DTV\_ATSCMH\_SCCC\_BLOCK\_MODE](fe_property_parameters.html#dtv-atscmh-sccc-block-mode): include/uapi/linux/dvb/frontend.h#571
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_A](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-a): include/uapi/linux/dvb/frontend.h#572
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_B](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-b): include/uapi/linux/dvb/frontend.h#573
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_C](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-c): include/uapi/linux/dvb/frontend.h#574
* [DTV\_ATSCMH\_SCCC\_CODE\_MODE\_D](fe_property_parameters.html#dtv-atscmh-sccc-code-mode-d): include/uapi/linux/dvb/frontend.h#575
* [DTV\_ATSCMH\_SGN](fe_property_parameters.html#dtv-atscmh-sgn): include/uapi/linux/dvb/frontend.h#565
* [DTV\_ATSCMH\_TNOG](fe_property_parameters.html#dtv-atscmh-tnog): include/uapi/linux/dvb/frontend.h#564
* [DTV\_BANDWIDTH\_HZ](fe_property_parameters.html#dtv-bandwidth-hz): include/uapi/linux/dvb/frontend.h#505
* [DTV\_CLEAR](fe_property_parameters.html#dtv-clear): include/uapi/linux/dvb/frontend.h#502
* [DTV\_CODE\_RATE\_HP](fe_property_parameters.html#dtv-code-rate-hp): include/uapi/linux/dvb/frontend.h#546
* [DTV\_CODE\_RATE\_LP](fe_property_parameters.html#dtv-code-rate-lp): include/uapi/linux/dvb/frontend.h#547
* [DTV\_DELIVERY\_SYSTEM](fe_property_parameters.html#dtv-delivery-system): include/uapi/linux/dvb/frontend.h#519
* [DTV\_DISEQC\_MASTER](fe_property_parameters.html#dtv-diseqc-master): include/uapi/linux/dvb/frontend.h#507
* [DTV\_DISEQC\_SLAVE\_REPLY](fe_property_parameters.html#dtv-diseqc-slave-reply): include/uapi/linux/dvb/frontend.h#514
* [DTV\_DVBT2\_PLP\_ID\_LEGACY](fe_property_parameters.html#dtv-dvbt2-plp-id-legacy): include/uapi/linux/dvb/frontend.h#556
* [DTV\_ENUM\_DELSYS](fe_property_parameters.html#dtv-enum-delsys): include/uapi/linux/dvb/frontend.h#558
* [DTV\_FE\_CAPABILITY](fe_property_parameters.html#dtv-fe-capability): include/uapi/linux/dvb/frontend.h#518
* [DTV\_FE\_CAPABILITY\_COUNT](fe_property_parameters.html#dtv-fe-capability-count): include/uapi/linux/dvb/frontend.h#517
* [DTV\_FREQUENCY](fe_property_parameters.html#dtv-frequency): include/uapi/linux/dvb/frontend.h#503
* [DTV\_GUARD\_INTERVAL](fe_property_parameters.html#dtv-guard-interval): include/uapi/linux/dvb/frontend.h#548
* [DTV\_HIERARCHY](fe_property_parameters.html#dtv-hierarchy): include/uapi/linux/dvb/frontend.h#550
* [DTV\_INNER\_FEC](fe_property_parameters.html#dtv-inner-fec): include/uapi/linux/dvb/frontend.h#509
* [DTV\_INTERLEAVING](fe_property_parameters.html#dtv-interleaving): include/uapi/linux/dvb/frontend.h#577
* [DTV\_INVERSION](fe_property_parameters.html#dtv-inversion): include/uapi/linux/dvb/frontend.h#506
* [DTV\_ISDBT\_LAYERA\_FEC](fe_property_parameters.html#dtv-isdbt-layer-fec): include/uapi/linux/dvb/frontend.h#529
* [DTV\_ISDBT\_LAYERA\_MODULATION](fe_property_parameters.html#dtv-isdbt-layer-modulation): include/uapi/linux/dvb/frontend.h#530
* [DTV\_ISDBT\_LAYERA\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-layer-segment-count): include/uapi/linux/dvb/frontend.h#531
* [DTV\_ISDBT\_LAYERA\_TIME\_INTERLEAVING](fe_property_parameters.html#dtv-isdbt-layer-time-interleaving): include/uapi/linux/dvb/frontend.h#532
* [DTV\_ISDBT\_LAYERB\_FEC](fe_property_parameters.html#dtv-isdbt-layer-fec): include/uapi/linux/dvb/frontend.h#534
* [DTV\_ISDBT\_LAYERB\_MODULATION](fe_property_parameters.html#dtv-isdbt-layer-modulation): include/uapi/linux/dvb/frontend.h#535
* [DTV\_ISDBT\_LAYERB\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-layer-segment-count): include/uapi/linux/dvb/frontend.h#536
* [DTV\_ISDBT\_LAYERB\_TIME\_INTERLEAVING](fe_property_parameters.html#dtv-isdbt-layer-time-interleaving): include/uapi/linux/dvb/frontend.h#537
* [DTV\_ISDBT\_LAYERC\_FEC](fe_property_parameters.html#dtv-isdbt-layer-fec): include/uapi/linux/dvb/frontend.h#539
* [DTV\_ISDBT\_LAYERC\_MODULATION](fe_property_parameters.html#dtv-isdbt-layer-modulation): include/uapi/linux/dvb/frontend.h#540
* [DTV\_ISDBT\_LAYERC\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-layer-segment-count): include/uapi/linux/dvb/frontend.h#541
* [DTV\_ISDBT\_LAYERC\_TIME\_INTERLEAVING](fe_property_parameters.html#dtv-isdbt-layer-time-interleaving): include/uapi/linux/dvb/frontend.h#542
* [DTV\_ISDBT\_LAYER\_ENABLED](fe_property_parameters.html#dtv-isdbt-layer-enabled): include/uapi/linux/dvb/frontend.h#552
* [DTV\_ISDBT\_PARTIAL\_RECEPTION](fe_property_parameters.html#dtv-isdbt-partial-reception): include/uapi/linux/dvb/frontend.h#522
* [DTV\_ISDBT\_SB\_SEGMENT\_COUNT](fe_property_parameters.html#dtv-isdbt-sb-segment-count): include/uapi/linux/dvb/frontend.h#527
* [DTV\_ISDBT\_SB\_SEGMENT\_IDX](fe_property_parameters.html#dtv-isdbt-sb-segment-idx): include/uapi/linux/dvb/frontend.h#526
* [DTV\_ISDBT\_SB\_SUBCHANNEL\_ID](fe_property_parameters.html#dtv-isdbt-sb-subchannel-id): include/uapi/linux/dvb/frontend.h#525
* [DTV\_ISDBT\_SOUND\_BROADCASTING](fe_property_parameters.html#dtv-isdbt-sound-broadcasting): include/uapi/linux/dvb/frontend.h#523
* [DTV\_LNA](fe_property_parameters.html#dtv-lna): include/uapi/linux/dvb/frontend.h#578
* [DTV\_MODULATION](fe_property_parameters.html#dtv-modulation): include/uapi/linux/dvb/frontend.h#504
* [DTV\_PILOT](fe_property_parameters.html#dtv-pilot): include/uapi/linux/dvb/frontend.h#512
* [DTV\_ROLLOFF](fe_property_parameters.html#dtv-rolloff): include/uapi/linux/dvb/frontend.h#513
* [DTV\_SCRAMBLING\_SEQUENCE\_INDEX](fe_property_parameters.html#dtv-scrambling-sequence-index): include/uapi/linux/dvb/frontend.h#591
* [DTV\_STAT\_CNR](frontend-stat-properties.html#dtv-stat-cnr): include/uapi/linux/dvb/frontend.h#582
* [DTV\_STAT\_ERROR\_BLOCK\_COUNT](frontend-stat-properties.html#dtv-stat-error-block-count): include/uapi/linux/dvb/frontend.h#587
* [DTV\_STAT\_POST\_ERROR\_BIT\_COUNT](frontend-stat-properties.html#dtv-stat-post-error-bit-count): include/uapi/linux/dvb/frontend.h#585
* [DTV\_STAT\_POST\_TOTAL\_BIT\_COUNT](frontend-stat-properties.html#dtv-stat-post-total-bit-count): include/uapi/linux/dvb/frontend.h#586
* [DTV\_STAT\_PRE\_ERROR\_BIT\_COUNT](frontend-stat-properties.html#dtv-stat-pre-error-bit-count): include/uapi/linux/dvb/frontend.h#583
* [DTV\_STAT\_PRE\_TOTAL\_BIT\_COUNT](frontend-stat-properties.html#dtv-stat-pre-total-bit-count): include/uapi/linux/dvb/frontend.h#584
* [DTV\_STAT\_SIGNAL\_STRENGTH](frontend-stat-properties.html#dtv-stat-signal-strength): include/uapi/linux/dvb/frontend.h#581
* [DTV\_STAT\_TOTAL\_BLOCK\_COUNT](frontend-stat-properties.html#dtv-stat-total-block-count): include/uapi/linux/dvb/frontend.h#588
* [DTV\_STREAM\_ID](fe_property_parameters.html#dtv-stream-id): include/uapi/linux/dvb/frontend.h#554
* [DTV\_SYMBOL\_RATE](fe_property_parameters.html#dtv-symbol-rate): include/uapi/linux/dvb/frontend.h#508
* [DTV\_TONE](fe_property_parameters.html#dtv-tone): include/uapi/linux/dvb/frontend.h#511
* [DTV\_TRANSMISSION\_MODE](fe_property_parameters.html#dtv-transmission-mode): include/uapi/linux/dvb/frontend.h#549
* [DTV\_TUNE](fe_property_parameters.html#dtv-tune): include/uapi/linux/dvb/frontend.h#501
* [DTV\_UNDEFINED](fe_property_parameters.html#dtv-undefined): include/uapi/linux/dvb/frontend.h#500
* [DTV\_VOLTAGE](fe_property_parameters.html#dtv-voltage): include/uapi/linux/dvb/frontend.h#510
* [FE\_TUNE\_MODE\_ONESHOT](fe-set-frontend-tune-mode.html#fe-set-frontend-tune-mode): include/uapi/linux/dvb/frontend.h#933
* [LNA\_AUTO](fe_property_parameters.html#dtv-lna): include/uapi/linux/dvb/frontend.h#786
* [NO\_STREAM\_ID\_FILTER](fe_property_parameters.html#dtv-stream-id): include/uapi/linux/dvb/frontend.h#785

### [8.1.5. Structures](#id19)

* [`dtv_fe_stats`](frontend-header.html#c.dtv_fe_stats "dtv_fe_stats"): include/uapi/linux/dvb/frontend.h#874
* [`dtv_properties`](frontend-header.html#c.dtv_properties "dtv_properties"): include/uapi/linux/dvb/frontend.h#921
* [`dtv_property`](frontend-header.html#c.dtv_property "dtv_property"): include/uapi/linux/dvb/frontend.h#896
* [`dtv_stats`](frontend-header.html#c.dtv_stats "dtv_stats"): include/uapi/linux/dvb/frontend.h#852
* [`dvb_diseqc_master_cmd`](frontend-header.html#c.dvb_diseqc_master_cmd "dvb_diseqc_master_cmd"): include/uapi/linux/dvb/frontend.h#158
* [`dvb_diseqc_slave_reply`](frontend-header.html#c.dvb_diseqc_slave_reply "dvb_diseqc_slave_reply"): include/uapi/linux/dvb/frontend.h#180
* [`dvb_frontend_event`](dvb-frontend-event.html#c.dvb_frontend_event "dvb_frontend_event"): include/uapi/linux/dvb/frontend.h#1048
* [`dvb_frontend_info`](frontend-header.html#c.dvb_frontend_info "dvb_frontend_info"): include/uapi/linux/dvb/frontend.h#131
* [`dvb_frontend_parameters`](dvb-frontend-parameters.html#c.dvb_frontend_parameters "dvb_frontend_parameters"): include/uapi/linux/dvb/frontend.h#1036
* [`dvb_ofdm_parameters`](dvb-frontend-parameters.html#c.dvb_ofdm_parameters "dvb_ofdm_parameters"): include/uapi/linux/dvb/frontend.h#1026
* [`dvb_qam_parameters`](dvb-frontend-parameters.html#c.dvb_qam_parameters "dvb_qam_parameters"): include/uapi/linux/dvb/frontend.h#1016
* [`dvb_qpsk_parameters`](dvb-frontend-parameters.html#c.dvb_qpsk_parameters "dvb_qpsk_parameters"): include/uapi/linux/dvb/frontend.h#1011
* [`dvb_vsb_parameters`](dvb-frontend-parameters.html#c.dvb_vsb_parameters "dvb_vsb_parameters"): include/uapi/linux/dvb/frontend.h#1022

### [8.1.6. Type Definitions](#id20)

* [`fe_bandwidth_t`](fe-bandwidth-t.html#c.fe_bandwidth "fe_bandwidth"): include/uapi/linux/dvb/frontend.h#1002
* [`fe_caps_t`](frontend-header.html#c.fe_caps "fe_caps"): include/uapi/linux/dvb/frontend.h#993
* [`fe_code_rate_t`](frontend-header.html#c.fe_code_rate "fe_code_rate"): include/uapi/linux/dvb/frontend.h#999
* [`fe_delivery_system_t`](frontend-header.html#c.fe_delivery_system "fe_delivery_system"): include/uapi/linux/dvb/frontend.h#1007
* [`fe_guard_interval_t`](frontend-header.html#c.fe_guard_interval "fe_guard_interval"): include/uapi/linux/dvb/frontend.h#1003
* [`fe_hierarchy_t`](frontend-header.html#c.fe_hierarchy "fe_hierarchy"): include/uapi/linux/dvb/frontend.h#1004
* [`fe_modulation_t`](frontend-header.html#c.fe_modulation "fe_modulation"): include/uapi/linux/dvb/frontend.h#1000
* [`fe_pilot_t`](frontend-header.html#c.fe_pilot "fe_pilot"): include/uapi/linux/dvb/frontend.h#1005
* [`fe_rolloff_t`](frontend-header.html#c.fe_rolloff "fe_rolloff"): include/uapi/linux/dvb/frontend.h#1006
* [`fe_sec_mini_cmd_t`](frontend-header.html#c.fe_sec_mini_cmd "fe_sec_mini_cmd"): include/uapi/linux/dvb/frontend.h#996
* [`fe_sec_tone_mode_t`](frontend-header.html#c.fe_sec_tone_mode "fe_sec_tone_mode"): include/uapi/linux/dvb/frontend.h#995
* [`fe_sec_voltage_t`](frontend-header.html#c.fe_sec_voltage "fe_sec_voltage"): include/uapi/linux/dvb/frontend.h#992
* [`fe_spectral_inversion_t`](frontend-header.html#c.fe_spectral_inversion "fe_spectral_inversion"): include/uapi/linux/dvb/frontend.h#998
* [`fe_status_t`](frontend-header.html#c.fe_status "fe_status"): include/uapi/linux/dvb/frontend.h#997
* [`fe_transmit_mode_t`](frontend-header.html#c.fe_transmit_mode "fe_transmit_mode"): include/uapi/linux/dvb/frontend.h#1001
* [`fe_type_t`](fe-type-t.html#c.fe_type "fe_type"): include/uapi/linux/dvb/frontend.h#994

## [8.2. Demux](#id21)

### [8.2.1. Enumeration values](#id22)

* [`DMX_BUFFER_FLAG_DISCONTINUITY_DETECTED`](dmx_types.html#c.DTV.dmx.dmx_buffer_flags "DTV.dmx.dmx_buffer_flags"): include/uapi/linux/dvb/dmx.h#221
* [`DMX_BUFFER_FLAG_DISCONTINUITY_INDICATOR`](dmx_types.html#c.DTV.dmx.dmx_buffer_flags "DTV.dmx.dmx_buffer_flags"): include/uapi/linux/dvb/dmx.h#222
* [`DMX_BUFFER_FLAG_HAD_CRC32_DISCARD`](dmx_types.html#c.DTV.dmx.dmx_buffer_flags "DTV.dmx.dmx_buffer_flags"): include/uapi/linux/dvb/dmx.h#218
* [`DMX_BUFFER_FLAG_TEI`](dmx_types.html#c.DTV.dmx.dmx_buffer_flags "DTV.dmx.dmx_buffer_flags"): include/uapi/linux/dvb/dmx.h#219
* [`DMX_BUFFER_PKT_COUNTER_MISMATCH`](dmx_types.html#c.DTV.dmx.dmx_buffer_flags "DTV.dmx.dmx_buffer_flags"): include/uapi/linux/dvb/dmx.h#220
* [`DMX_IN_DVR`](dmx_types.html#c.DTV.dmx.dmx_input "DTV.dmx.dmx_input"): include/uapi/linux/dvb/dmx.h#54
* [`DMX_IN_FRONTEND`](dmx_types.html#c.DTV.dmx.dmx_input "DTV.dmx.dmx_input"): include/uapi/linux/dvb/dmx.h#53
* [`DMX_OUT_DECODER`](dmx_types.html#c.DTV.dmx.dmx_output "DTV.dmx.dmx_output"): include/uapi/linux/dvb/dmx.h#39
* [`DMX_OUT_TAP`](dmx_types.html#c.DTV.dmx.dmx_output "DTV.dmx.dmx_output"): include/uapi/linux/dvb/dmx.h#40
* [`DMX_OUT_TSDEMUX_TAP`](dmx_types.html#c.DTV.dmx.dmx_output "DTV.dmx.dmx_output"): include/uapi/linux/dvb/dmx.h#42
* [`DMX_OUT_TS_TAP`](dmx_types.html#c.DTV.dmx.dmx_output "DTV.dmx.dmx_output"): include/uapi/linux/dvb/dmx.h#41
* [`DMX_PES_AUDIO0`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#89
* [`DMX_PES_AUDIO1`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#95
* [`DMX_PES_AUDIO2`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#101
* [`DMX_PES_AUDIO3`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#107
* [`DMX_PES_OTHER`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#113
* [`DMX_PES_PCR0`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#93
* [`DMX_PES_PCR1`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#99
* [`DMX_PES_PCR2`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#105
* [`DMX_PES_PCR3`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#111
* [`DMX_PES_SUBTITLE0`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#92
* [`DMX_PES_SUBTITLE1`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#98
* [`DMX_PES_SUBTITLE2`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#104
* [`DMX_PES_SUBTITLE3`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#110
* [`DMX_PES_TELETEXT0`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#91
* [`DMX_PES_TELETEXT1`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#97
* [`DMX_PES_TELETEXT2`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#103
* [`DMX_PES_TELETEXT3`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#109
* [`DMX_PES_VIDEO0`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#90
* [`DMX_PES_VIDEO1`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#96
* [`DMX_PES_VIDEO2`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#102
* [`DMX_PES_VIDEO3`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#108

### [8.2.2. Enumerations](#id23)

* [`dmx_buffer_flags`](dmx_types.html#c.DTV.dmx.dmx_buffer_flags "DTV.dmx.dmx_buffer_flags"): include/uapi/linux/dvb/dmx.h#217
* [`dmx_input`](dmx_types.html#c.DTV.dmx.dmx_input "DTV.dmx.dmx_input"): include/uapi/linux/dvb/dmx.h#52
* [`dmx_output`](dmx_types.html#c.DTV.dmx.dmx_output "DTV.dmx.dmx_output"): include/uapi/linux/dvb/dmx.h#38
* [`dmx_ts_pes`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#88

### [8.2.3. IOCTL Commands](#id24)

* [DMX\_ADD\_PID](dmx-add-pid.html#dmx-add-pid): include/uapi/linux/dvb/dmx.h#296
* [DMX\_DQBUF](dmx-qbuf.html#dmx-qbuf): include/uapi/linux/dvb/dmx.h#313
* [DMX\_EXPBUF](dmx-expbuf.html#dmx-expbuf): include/uapi/linux/dvb/dmx.h#311
* [DMX\_GET\_PES\_PIDS](dmx-get-pes-pids.html#dmx-get-pes-pids): include/uapi/linux/dvb/dmx.h#294
* [DMX\_GET\_STC](dmx-get-stc.html#dmx-get-stc): include/uapi/linux/dvb/dmx.h#295
* [DMX\_QBUF](dmx-qbuf.html#dmx-qbuf): include/uapi/linux/dvb/dmx.h#312
* [DMX\_QUERYBUF](dmx-querybuf.html#dmx-querybuf): include/uapi/linux/dvb/dmx.h#310
* [DMX\_REMOVE\_PID](dmx-remove-pid.html#dmx-remove-pid): include/uapi/linux/dvb/dmx.h#297
* [DMX\_REQBUFS](dmx-reqbufs.html#dmx-reqbufs): include/uapi/linux/dvb/dmx.h#309
* [DMX\_SET\_BUFFER\_SIZE](dmx-set-buffer-size.html#dmx-set-buffer-size): include/uapi/linux/dvb/dmx.h#293
* [DMX\_SET\_FILTER](dmx-set-filter.html#dmx-set-filter): include/uapi/linux/dvb/dmx.h#291
* [DMX\_SET\_PES\_FILTER](dmx-set-pes-filter.html#dmx-set-pes-filter): include/uapi/linux/dvb/dmx.h#292
* [DMX\_START](dmx-start.html#dmx-start): include/uapi/linux/dvb/dmx.h#289
* [DMX\_STOP](dmx-stop.html#dmx-stop): include/uapi/linux/dvb/dmx.h#290

### [8.2.4. Macros and Definitions](#id25)

* [`DMX_CHECK_CRC`](dmx_types.html#c.DTV.dmx.dmx_sct_filter_params "DTV.dmx.dmx_sct_filter_params"): include/uapi/linux/dvb/dmx.h#163
* [`DMX_IMMEDIATE_START`](dmx_types.html#c.DTV.dmx.dmx_sct_filter_params "DTV.dmx.dmx_sct_filter_params"): include/uapi/linux/dvb/dmx.h#165
* [`DMX_ONESHOT`](dmx_types.html#c.DTV.dmx.dmx_sct_filter_params "DTV.dmx.dmx_sct_filter_params"): include/uapi/linux/dvb/dmx.h#164

### [8.2.5. Structures](#id26)

* [`dmx_buffer`](dmx_types.html#c.DTV.dmx.dmx_buffer "DTV.dmx.dmx_buffer"): include/uapi/linux/dvb/dmx.h#245
* [`dmx_exportbuffer`](dmx_types.html#c.DTV.dmx.dmx_exportbuffer "DTV.dmx.dmx_exportbuffer"): include/uapi/linux/dvb/dmx.h#283
* [`dmx_filter`](dmx_types.html#c.DTV.dmx.dmx_filter "DTV.dmx.dmx_filter"): include/uapi/linux/dvb/dmx.h#134
* [`dmx_pes_filter_params`](dmx_types.html#c.DTV.dmx.dmx_pes_filter_params "DTV.dmx.dmx_pes_filter_params"): include/uapi/linux/dvb/dmx.h#178
* [`dmx_requestbuffers`](dmx_types.html#c.DTV.dmx.dmx_requestbuffers "DTV.dmx.dmx_requestbuffers"): include/uapi/linux/dvb/dmx.h#263
* [`dmx_sct_filter_params`](dmx_types.html#c.DTV.dmx.dmx_sct_filter_params "DTV.dmx.dmx_sct_filter_params"): include/uapi/linux/dvb/dmx.h#158
* [`dmx_stc`](dmx_types.html#c.DTV.dmx.dmx_stc "DTV.dmx.dmx_stc"): include/uapi/linux/dvb/dmx.h#193

### [8.2.6. Type Definitions](#id27)

* [`dmx_filter_t`](dmx_types.html#c.DTV.dmx.dmx_filter "DTV.dmx.dmx_filter"): include/uapi/linux/dvb/dmx.h#305
* [`dmx_input_t`](dmx_types.html#c.DTV.dmx.dmx_input "DTV.dmx.dmx_input"): include/uapi/linux/dvb/dmx.h#303
* [`dmx_output_t`](dmx_types.html#c.DTV.dmx.dmx_output "DTV.dmx.dmx_output"): include/uapi/linux/dvb/dmx.h#302
* [`dmx_pes_type_t`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "DTV.dmx.dmx_ts_pes"): include/uapi/linux/dvb/dmx.h#304

## [8.3. Conditional Access](#id28)

### [8.3.1. IOCTL Commands](#id29)

* [CA\_GET\_CAP](ca-get-cap.html#ca-get-cap): include/uapi/linux/dvb/ca.h#121
* [CA\_GET\_DESCR\_INFO](ca-get-descr-info.html#ca-get-descr-info): include/uapi/linux/dvb/ca.h#123
* [CA\_GET\_MSG](ca-get-msg.html#ca-get-msg): include/uapi/linux/dvb/ca.h#124
* [CA\_GET\_SLOT\_INFO](ca-get-slot-info.html#ca-get-slot-info): include/uapi/linux/dvb/ca.h#122
* [CA\_RESET](ca-reset.html#ca-reset): include/uapi/linux/dvb/ca.h#120
* [CA\_SEND\_MSG](ca-send-msg.html#ca-send-msg): include/uapi/linux/dvb/ca.h#125
* [CA\_SET\_DESCR](ca-set-descr.html#ca-set-descr): include/uapi/linux/dvb/ca.h#126

### [8.3.2. Macros and Definitions](#id30)

* [`CA_CI`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#39
* [`CA_CI_LINK`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#40
* [`CA_CI_MODULE_PRESENT`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#46
* [`CA_CI_MODULE_READY`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#47
* [`CA_CI_PHYS`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#41
* [`CA_DESCR`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#42
* [`CA_DSS`](ca_data_types.html#c.ca_descr_info "ca_descr_info"): include/uapi/linux/dvb/ca.h#70
* [`CA_ECD`](ca_data_types.html#c.ca_descr_info "ca_descr_info"): include/uapi/linux/dvb/ca.h#68
* [`CA_NDS`](ca_data_types.html#c.ca_descr_info "ca_descr_info"): include/uapi/linux/dvb/ca.h#69
* [`CA_SC`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#43

### [8.3.3. Structures](#id31)

* [`ca_caps`](ca_data_types.html#c.ca_caps "ca_caps"): include/uapi/linux/dvb/ca.h#83
* [`ca_descr`](ca_data_types.html#c.ca_descr "ca_descr"): include/uapi/linux/dvb/ca.h#114
* [`ca_descr_info`](ca_data_types.html#c.ca_descr_info "ca_descr_info"): include/uapi/linux/dvb/ca.h#65
* [`ca_msg`](ca_data_types.html#c.ca_msg "ca_msg"): include/uapi/linux/dvb/ca.h#100
* [`ca_slot_info`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#36

### [8.3.4. Type Definitions](#id32)

* [`ca_caps_t`](ca_data_types.html#c.ca_caps "ca_caps"): include/uapi/linux/dvb/ca.h#133
* [`ca_descr_info_t`](ca_data_types.html#c.ca_descr_info "ca_descr_info"): include/uapi/linux/dvb/ca.h#132
* [`ca_descr_t`](ca_data_types.html#c.ca_descr "ca_descr"): include/uapi/linux/dvb/ca.h#135
* [`ca_msg_t`](ca_data_types.html#c.ca_msg "ca_msg"): include/uapi/linux/dvb/ca.h#134
* [`ca_slot_info_t`](ca_data_types.html#c.ca_slot_info "ca_slot_info"): include/uapi/linux/dvb/ca.h#131

## [8.4. Network](#id33)

### [8.4.1. IOCTL Commands](#id34)

* [NET\_ADD\_IF](net-add-if.html#net-add-if): include/uapi/linux/dvb/net.h#39
* [NET\_GET\_IF](net-get-if.html#net-get-if): include/uapi/linux/dvb/net.h#41
* [NET\_REMOVE\_IF](net-remove-if.html#net-remove-if): include/uapi/linux/dvb/net.h#40

### [8.4.2. Macros and Definitions](#id35)

* [`DVB_NET_FEEDTYPE_MPE`](net-types.html#c.dvb_net_if "dvb_net_if"): include/uapi/linux/dvb/net.h#34
* [`DVB_NET_FEEDTYPE_ULE`](net-types.html#c.dvb_net_if "dvb_net_if"): include/uapi/linux/dvb/net.h#35

### [8.4.3. Structures](#id36)

* [`dvb_net_if`](net-types.html#c.dvb_net_if "dvb_net_if"): include/uapi/linux/dvb/net.h#30
