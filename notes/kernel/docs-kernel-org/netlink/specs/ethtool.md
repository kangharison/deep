# Familyethtoolnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/ethtool.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `ethtool` netlink specification](#id120)

## [Summary](#id121)

Partial family for Ethtool Netlink.

## [Operations](#id122)

### [strset-get](#id123)

Get string set from the kernel.

attribute-set:
:   [strset](#ethtool-attribute-set-strset)

do:
:   **request**
    :   attributes:
        :   [`header`, `stringsets`, `counts-only`]

    **reply**
    :   attributes:
        :   [`header`, `stringsets`]

dump:
:   **request**
    :   attributes:
        :   [`header`, `stringsets`, `counts-only`]

    **reply**
    :   attributes:
        :   [`header`, `stringsets`]

### [linkinfo-get](#id124)

Get link info.

attribute-set:
:   [linkinfo](#ethtool-attribute-set-linkinfo)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `port`, `phyaddr`, `tp-mdix`, `tp-mdix-ctrl`, `transceiver`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `port`, `phyaddr`, `tp-mdix`, `tp-mdix-ctrl`, `transceiver`]

### [linkinfo-set](#id125)

Set link info.

attribute-set:
:   [linkinfo](#ethtool-attribute-set-linkinfo)

do:
:   **request**
    :   attributes:
        :   [`header`, `port`, `phyaddr`, `tp-mdix`, `tp-mdix-ctrl`, `transceiver`]

### [linkinfo-ntf](#id126)

Notification for change in link info.

notify:
:   linkinfo-get

### [linkmodes-get](#id127)

Get link modes.

attribute-set:
:   [linkmodes](#ethtool-attribute-set-linkmodes)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `autoneg`, `ours`, `peer`, `speed`, `duplex`, `master-slave-cfg`, `master-slave-state`, `lanes`, `rate-matching`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `autoneg`, `ours`, `peer`, `speed`, `duplex`, `master-slave-cfg`, `master-slave-state`, `lanes`, `rate-matching`]

### [linkmodes-set](#id128)

Set link modes.

attribute-set:
:   [linkmodes](#ethtool-attribute-set-linkmodes)

do:
:   **request**
    :   attributes:
        :   [`header`, `autoneg`, `ours`, `peer`, `speed`, `duplex`, `master-slave-cfg`, `master-slave-state`, `lanes`, `rate-matching`]

### [linkmodes-ntf](#id129)

Notification for change in link modes.

notify:
:   linkmodes-get

### [linkstate-get](#id130)

Get link state.

attribute-set:
:   [linkstate](#ethtool-attribute-set-linkstate)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `link`, `sqi`, `sqi-max`, `ext-state`, `ext-substate`, `ext-down-cnt`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `link`, `sqi`, `sqi-max`, `ext-state`, `ext-substate`, `ext-down-cnt`]

### [debug-get](#id131)

Get debug message mask.

attribute-set:
:   [debug](#ethtool-attribute-set-debug)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `msgmask`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `msgmask`]

### [debug-set](#id132)

Set debug message mask.

attribute-set:
:   [debug](#ethtool-attribute-set-debug)

do:
:   **request**
    :   attributes:
        :   [`header`, `msgmask`]

### [debug-ntf](#id133)

Notification for change in debug message mask.

notify:
:   debug-get

### [wol-get](#id134)

Get WOL params.

attribute-set:
:   [wol](#ethtool-attribute-set-wol)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `modes`, `sopass`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `modes`, `sopass`]

### [wol-set](#id135)

Set WOL params.

attribute-set:
:   [wol](#ethtool-attribute-set-wol)

do:
:   **request**
    :   attributes:
        :   [`header`, `modes`, `sopass`]

### [wol-ntf](#id136)

Notification for change in WOL params.

notify:
:   wol-get

### [features-get](#id137)

Get features.

attribute-set:
:   [features](#ethtool-attribute-set-features)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `hw`, `wanted`, `active`, `nochange`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `hw`, `wanted`, `active`, `nochange`]

### [features-set](#id138)

Set features.

attribute-set:
:   [features](#ethtool-attribute-set-features)

do:
:   **request**
    :   attributes:
        :   [`header`, `hw`, `wanted`, `active`, `nochange`]

    **reply**
    :   attributes:
        :   [`header`, `hw`, `wanted`, `active`, `nochange`]

### [features-ntf](#id139)

Notification for change in features.

notify:
:   features-get

### [privflags-get](#id140)

Get device private flags.

attribute-set:
:   [privflags](#ethtool-attribute-set-privflags)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `flags`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `flags`]

### [privflags-set](#id141)

Set device private flags.

attribute-set:
:   [privflags](#ethtool-attribute-set-privflags)

do:
:   **request**
    :   attributes:
        :   [`header`, `flags`]

### [privflags-ntf](#id142)

Notification for change in device private flags.

notify:
:   privflags-get

### [rings-get](#id143)

Get ring params.

attribute-set:
:   [rings](#ethtool-attribute-set-rings)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `rx-max`, `rx-mini-max`, `rx-jumbo-max`, `tx-max`, `rx`, `rx-mini`, `rx-jumbo`, `tx`, `rx-buf-len`, `tcp-data-split`, `cqe-size`, `tx-push`, `rx-push`, `tx-push-buf-len`, `tx-push-buf-len-max`, `hds-thresh`, `hds-thresh-max`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `rx-max`, `rx-mini-max`, `rx-jumbo-max`, `tx-max`, `rx`, `rx-mini`, `rx-jumbo`, `tx`, `rx-buf-len`, `tcp-data-split`, `cqe-size`, `tx-push`, `rx-push`, `tx-push-buf-len`, `tx-push-buf-len-max`, `hds-thresh`, `hds-thresh-max`]

### [rings-set](#id144)

Set ring params.

attribute-set:
:   [rings](#ethtool-attribute-set-rings)

do:
:   **request**
    :   attributes:
        :   [`header`, `rx-max`, `rx-mini-max`, `rx-jumbo-max`, `tx-max`, `rx`, `rx-mini`, `rx-jumbo`, `tx`, `rx-buf-len`, `tcp-data-split`, `cqe-size`, `tx-push`, `rx-push`, `tx-push-buf-len`, `tx-push-buf-len-max`, `hds-thresh`, `hds-thresh-max`]

### [rings-ntf](#id145)

Notification for change in ring params.

notify:
:   rings-get

### [channels-get](#id146)

Get channel params.

attribute-set:
:   [channels](#ethtool-attribute-set-channels)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `rx-max`, `tx-max`, `other-max`, `combined-max`, `rx-count`, `tx-count`, `other-count`, `combined-count`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `rx-max`, `tx-max`, `other-max`, `combined-max`, `rx-count`, `tx-count`, `other-count`, `combined-count`]

### [channels-set](#id147)

Set channel params.

attribute-set:
:   [channels](#ethtool-attribute-set-channels)

do:
:   **request**
    :   attributes:
        :   [`header`, `rx-max`, `tx-max`, `other-max`, `combined-max`, `rx-count`, `tx-count`, `other-count`, `combined-count`]

### [channels-ntf](#id148)

Notification for change in channel params.

notify:
:   channels-get

### [coalesce-get](#id149)

Get coalesce params.

attribute-set:
:   [coalesce](#ethtool-attribute-set-coalesce)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `rx-usecs`, `rx-max-frames`, `rx-usecs-irq`, `rx-max-frames-irq`, `tx-usecs`, `tx-max-frames`, `tx-usecs-irq`, `tx-max-frames-irq`, `stats-block-usecs`, `use-adaptive-rx`, `use-adaptive-tx`, `pkt-rate-low`, `rx-usecs-low`, `rx-max-frames-low`, `tx-usecs-low`, `tx-max-frames-low`, `pkt-rate-high`, `rx-usecs-high`, `rx-max-frames-high`, `tx-usecs-high`, `tx-max-frames-high`, `rate-sample-interval`, `use-cqe-mode-tx`, `use-cqe-mode-rx`, `tx-aggr-max-bytes`, `tx-aggr-max-frames`, `tx-aggr-time-usecs`, `rx-profile`, `tx-profile`, `rx-cqe-frames`, `rx-cqe-nsecs`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `rx-usecs`, `rx-max-frames`, `rx-usecs-irq`, `rx-max-frames-irq`, `tx-usecs`, `tx-max-frames`, `tx-usecs-irq`, `tx-max-frames-irq`, `stats-block-usecs`, `use-adaptive-rx`, `use-adaptive-tx`, `pkt-rate-low`, `rx-usecs-low`, `rx-max-frames-low`, `tx-usecs-low`, `tx-max-frames-low`, `pkt-rate-high`, `rx-usecs-high`, `rx-max-frames-high`, `tx-usecs-high`, `tx-max-frames-high`, `rate-sample-interval`, `use-cqe-mode-tx`, `use-cqe-mode-rx`, `tx-aggr-max-bytes`, `tx-aggr-max-frames`, `tx-aggr-time-usecs`, `rx-profile`, `tx-profile`, `rx-cqe-frames`, `rx-cqe-nsecs`]

### [coalesce-set](#id150)

Set coalesce params.

attribute-set:
:   [coalesce](#ethtool-attribute-set-coalesce)

do:
:   **request**
    :   attributes:
        :   [`header`, `rx-usecs`, `rx-max-frames`, `rx-usecs-irq`, `rx-max-frames-irq`, `tx-usecs`, `tx-max-frames`, `tx-usecs-irq`, `tx-max-frames-irq`, `stats-block-usecs`, `use-adaptive-rx`, `use-adaptive-tx`, `pkt-rate-low`, `rx-usecs-low`, `rx-max-frames-low`, `tx-usecs-low`, `tx-max-frames-low`, `pkt-rate-high`, `rx-usecs-high`, `rx-max-frames-high`, `tx-usecs-high`, `tx-max-frames-high`, `rate-sample-interval`, `use-cqe-mode-tx`, `use-cqe-mode-rx`, `tx-aggr-max-bytes`, `tx-aggr-max-frames`, `tx-aggr-time-usecs`, `rx-profile`, `tx-profile`, `rx-cqe-frames`, `rx-cqe-nsecs`]

### [coalesce-ntf](#id151)

Notification for change in coalesce params.

notify:
:   coalesce-get

### [pause-get](#id152)

Get pause params.

attribute-set:
:   [pause](#ethtool-attribute-set-pause)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `autoneg`, `rx`, `tx`, `stats`, `stats-src`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `autoneg`, `rx`, `tx`, `stats`, `stats-src`]

### [pause-set](#id153)

Set pause params.

attribute-set:
:   [pause](#ethtool-attribute-set-pause)

do:
:   **request**
    :   attributes:
        :   [`header`, `autoneg`, `rx`, `tx`, `stats`, `stats-src`]

### [pause-ntf](#id154)

Notification for change in pause params.

notify:
:   pause-get

### [eee-get](#id155)

Get eee params.

attribute-set:
:   [eee](#ethtool-attribute-set-eee)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `modes-ours`, `modes-peer`, `active`, `enabled`, `tx-lpi-enabled`, `tx-lpi-timer`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `modes-ours`, `modes-peer`, `active`, `enabled`, `tx-lpi-enabled`, `tx-lpi-timer`]

### [eee-set](#id156)

Set eee params.

attribute-set:
:   [eee](#ethtool-attribute-set-eee)

do:
:   **request**
    :   attributes:
        :   [`header`, `modes-ours`, `modes-peer`, `active`, `enabled`, `tx-lpi-enabled`, `tx-lpi-timer`]

### [eee-ntf](#id157)

Notification for change in eee params.

notify:
:   eee-get

### [tsinfo-get](#id158)

Get tsinfo params.

attribute-set:
:   [tsinfo](#ethtool-attribute-set-tsinfo)

do:
:   **request**
    :   attributes:
        :   [`header`, `hwtstamp-provider`]

    **reply**
    :   attributes:
        :   [`header`, `timestamping`, `tx-types`, `rx-filters`, `phc-index`, `stats`, `hwtstamp-provider`, `hwtstamp-source`, `hwtstamp-phyindex`]

dump:
:   **request**
    :   attributes:
        :   [`header`, `hwtstamp-provider`]

    **reply**
    :   attributes:
        :   [`header`, `timestamping`, `tx-types`, `rx-filters`, `phc-index`, `stats`, `hwtstamp-provider`, `hwtstamp-source`, `hwtstamp-phyindex`]

### [cable-test-act](#id159)

Cable test.

attribute-set:
:   [cable-test](#ethtool-attribute-set-cable-test)

do:
:   **request**
    :   attributes:
        :   [`header`]

### [cable-test-ntf](#id160)

Cable test notification.

attribute-set:
:   [cable-test-ntf](#ethtool-attribute-set-cable-test-ntf)

event:
:   attributes:
    :   [`header`, `status`]

### [cable-test-tdr-act](#id161)

Cable test TDR.

attribute-set:
:   [cable-test-tdr](#ethtool-attribute-set-cable-test-tdr)

do:
:   **request**
    :   attributes:
        :   [`header`]

### [cable-test-tdr-ntf](#id162)

Cable test TDR notification.

attribute-set:
:   [cable-test-tdr-ntf](#ethtool-attribute-set-cable-test-tdr-ntf)

event:
:   attributes:
    :   [`header`, `status`, `nest`]

### [tunnel-info-get](#id163)

Get tsinfo params.

attribute-set:
:   [tunnel-info](#ethtool-attribute-set-tunnel-info)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `udp-ports`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `udp-ports`]

### [fec-get](#id164)

Get FEC params.

attribute-set:
:   [fec](#ethtool-attribute-set-fec)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `modes`, `auto`, `active`, `stats`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `modes`, `auto`, `active`, `stats`]

### [fec-set](#id165)

Set FEC params.

attribute-set:
:   [fec](#ethtool-attribute-set-fec)

do:
:   **request**
    :   attributes:
        :   [`header`, `modes`, `auto`, `active`, `stats`]

### [fec-ntf](#id166)

Notification for change in FEC params.

notify:
:   fec-get

### [module-eeprom-get](#id167)

Get module EEPROM params.

attribute-set:
:   [module-eeprom](#ethtool-attribute-set-module-eeprom)

do:
:   **request**
    :   attributes:
        :   [`header`, `offset`, `length`, `page`, `bank`, `i2c-address`]

    **reply**
    :   attributes:
        :   [`header`, `data`]

dump:
:   **request**
    :   attributes:
        :   [`header`, `offset`, `length`, `page`, `bank`, `i2c-address`]

    **reply**
    :   attributes:
        :   [`header`, `data`]

### [stats-get](#id168)

Get statistics.

attribute-set:
:   [stats](#ethtool-attribute-set-stats)

do:
:   **request**
    :   attributes:
        :   [`header`, `groups`]

    **reply**
    :   attributes:
        :   [`header`, `groups`, `grp`, `src`]

dump:
:   **request**
    :   attributes:
        :   [`header`, `groups`]

    **reply**
    :   attributes:
        :   [`header`, `groups`, `grp`, `src`]

### [phc-vclocks-get](#id169)

Get PHC VCLOCKs.

attribute-set:
:   [phc-vclocks](#ethtool-attribute-set-phc-vclocks)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `num`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `num`]

### [module-get](#id170)

Get module params.

attribute-set:
:   [module](#ethtool-attribute-set-module)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `power-mode-policy`, `power-mode`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `power-mode-policy`, `power-mode`]

### [module-set](#id171)

Set module params.

attribute-set:
:   [module](#ethtool-attribute-set-module)

do:
:   **request**
    :   attributes:
        :   [`header`, `power-mode-policy`, `power-mode`]

### [module-ntf](#id172)

Notification for change in module params.

notify:
:   module-get

### [pse-get](#id173)

Get Power Sourcing Equipment params.

attribute-set:
:   [pse](#ethtool-attribute-set-pse)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `podl-pse-admin-state`, `podl-pse-admin-control`, `podl-pse-pw-d-status`, `c33-pse-admin-state`, `c33-pse-admin-control`, `c33-pse-pw-d-status`, `c33-pse-pw-class`, `c33-pse-actual-pw`, `c33-pse-ext-state`, `c33-pse-ext-substate`, `c33-pse-avail-pw-limit`, `c33-pse-pw-limit-ranges`, `pse-pw-d-id`, `pse-prio-max`, `pse-prio`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `podl-pse-admin-state`, `podl-pse-admin-control`, `podl-pse-pw-d-status`, `c33-pse-admin-state`, `c33-pse-admin-control`, `c33-pse-pw-d-status`, `c33-pse-pw-class`, `c33-pse-actual-pw`, `c33-pse-ext-state`, `c33-pse-ext-substate`, `c33-pse-avail-pw-limit`, `c33-pse-pw-limit-ranges`, `pse-pw-d-id`, `pse-prio-max`, `pse-prio`]

### [pse-set](#id174)

Set Power Sourcing Equipment params.

attribute-set:
:   [pse](#ethtool-attribute-set-pse)

do:
:   **request**
    :   attributes:
        :   [`header`, `podl-pse-admin-control`, `c33-pse-admin-control`, `c33-pse-avail-pw-limit`, `pse-prio`]

### [rss-get](#id175)

Get RSS params.

attribute-set:
:   [rss](#ethtool-attribute-set-rss)

do:
:   **request**
    :   attributes:
        :   [`header`, `context`]

    **reply**
    :   attributes:
        :   [`header`, `context`, `hfunc`, `indir`, `hkey`, `input-xfrm`, `flow-hash`]

dump:
:   **request**
    :   attributes:
        :   [`header`, `start-context`]

    **reply**
    :   attributes:
        :   [`header`, `context`, `hfunc`, `indir`, `hkey`, `input-xfrm`, `flow-hash`]

### [plca-get-cfg](#id176)

Get PLCA params.

attribute-set:
:   [plca](#ethtool-attribute-set-plca)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `version`, `enabled`, `status`, `node-cnt`, `node-id`, `to-tmr`, `burst-cnt`, `burst-tmr`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `version`, `enabled`, `status`, `node-cnt`, `node-id`, `to-tmr`, `burst-cnt`, `burst-tmr`]

### [plca-set-cfg](#id177)

Set PLCA params.

attribute-set:
:   [plca](#ethtool-attribute-set-plca)

do:
:   **request**
    :   attributes:
        :   [`header`, `version`, `enabled`, `status`, `node-cnt`, `node-id`, `to-tmr`, `burst-cnt`, `burst-tmr`]

### [plca-get-status](#id178)

Get PLCA status params.

attribute-set:
:   [plca](#ethtool-attribute-set-plca)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `version`, `enabled`, `status`, `node-cnt`, `node-id`, `to-tmr`, `burst-cnt`, `burst-tmr`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `version`, `enabled`, `status`, `node-cnt`, `node-id`, `to-tmr`, `burst-cnt`, `burst-tmr`]

### [plca-ntf](#id179)

Notification for change in PLCA params.

notify:
:   plca-get-cfg

### [mm-get](#id180)

Get MAC Merge configuration and state

attribute-set:
:   [mm](#ethtool-attribute-set-mm)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `pmac-enabled`, `tx-enabled`, `tx-active`, `tx-min-frag-size`, `rx-min-frag-size`, `verify-enabled`, `verify-time`, `max-verify-time`, `stats`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `pmac-enabled`, `tx-enabled`, `tx-active`, `tx-min-frag-size`, `rx-min-frag-size`, `verify-enabled`, `verify-time`, `max-verify-time`, `stats`]

### [mm-set](#id181)

Set MAC Merge configuration

attribute-set:
:   [mm](#ethtool-attribute-set-mm)

do:
:   **request**
    :   attributes:
        :   [`header`, `verify-enabled`, `verify-time`, `tx-enabled`, `pmac-enabled`, `tx-min-frag-size`]

### [mm-ntf](#id182)

Notification for change in MAC Merge configuration.

notify:
:   mm-get

### [module-fw-flash-act](#id183)

Flash transceiver module firmware.

attribute-set:
:   [module-fw-flash](#ethtool-attribute-set-module-fw-flash)

do:
:   **request**
    :   attributes:
        :   [`header`, `file-name`, `password`]

### [module-fw-flash-ntf](#id184)

Notification for firmware flashing progress and status.

attribute-set:
:   [module-fw-flash](#ethtool-attribute-set-module-fw-flash)

event:
:   attributes:
    :   [`header`, `status`, `status-msg`, `done`, `total`]

### [phy-get](#id185)

Get PHY devices attached to an interface

attribute-set:
:   [phy](#ethtool-attribute-set-phy)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `index`, `drvname`, `name`, `upstream-type`, `upstream-index`, `upstream-sfp-name`, `downstream-sfp-name`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `index`, `drvname`, `name`, `upstream-type`, `upstream-index`, `upstream-sfp-name`, `downstream-sfp-name`]

### [phy-ntf](#id186)

Notification for change in PHY devices.

notify:
:   phy-get

### [tsconfig-get](#id187)

Get hwtstamp config.

attribute-set:
:   [tsconfig](#ethtool-attribute-set-tsconfig)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `hwtstamp-provider`, `tx-types`, `rx-filters`, `hwtstamp-flags`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `hwtstamp-provider`, `tx-types`, `rx-filters`, `hwtstamp-flags`]

### [tsconfig-set](#id188)

Set hwtstamp config.

attribute-set:
:   [tsconfig](#ethtool-attribute-set-tsconfig)

do:
:   **request**
    :   attributes:
        :   [`header`, `hwtstamp-provider`, `tx-types`, `rx-filters`, `hwtstamp-flags`]

    **reply**
    :   attributes:
        :   [`header`, `hwtstamp-provider`, `tx-types`, `rx-filters`, `hwtstamp-flags`]

### [pse-ntf](#id189)

Notification for PSE events.

attribute-set:
:   [pse-ntf](#ethtool-attribute-set-pse-ntf)

event:
:   attributes:
    :   [`header`, `events`]

### [rss-set](#id190)

Set RSS params.

attribute-set:
:   [rss](#ethtool-attribute-set-rss)

do:
:   **request**
    :   attributes:
        :   [`header`, `context`, `hfunc`, `indir`, `hkey`, `input-xfrm`, `flow-hash`]

### [rss-ntf](#id191)

Notification for change in RSS configuration.
For additional contexts only modifications use this notification,
creation and deletion have dedicated messages.

notify:
:   rss-get

### [rss-create-act](#id192)

Create an RSS context.

attribute-set:
:   [rss](#ethtool-attribute-set-rss)

do:
:   **request**
    :   attributes:
        :   [`header`, `context`, `hfunc`, `indir`, `hkey`, `input-xfrm`]

    **reply**
    :   attributes:
        :   [`header`, `context`, `hfunc`, `indir`, `hkey`, `input-xfrm`]

### [rss-create-ntf](#id193)

Notification for creation of an additional RSS context.

notify:
:   rss-create-act

### [rss-delete-act](#id194)

Delete an RSS context.

attribute-set:
:   [rss](#ethtool-attribute-set-rss)

do:
:   **request**
    :   attributes:
        :   [`header`, `context`]

### [rss-delete-ntf](#id195)

Notification for deletion of an additional RSS context.

attribute-set:
:   [rss](#ethtool-attribute-set-rss)

event:
:   attributes:
    :   [`header`, `context`]

### [mse-get](#id196)

Get PHY MSE measurement data and capabilities.

attribute-set:
:   [mse](#ethtool-attribute-set-mse)

do:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `capabilities`, `channel-a`, `channel-b`, `channel-c`, `channel-d`, `worst-channel`, `link`]

dump:
:   **request**
    :   attributes:
        :   [`header`]

    **reply**
    :   attributes:
        :   [`header`, `capabilities`, `channel-a`, `channel-b`, `channel-c`, `channel-d`, `worst-channel`, `link`]

## [Multicast groups](#id197)

* monitor

## [Definitions](#id198)

### [udp-tunnel-type](#id199)

enum-name:
:   None

type:
:   enum

enum-cnt-name:
:   \_\_ethtool-udp-tunnel-type-cnt

entries:
:   * `vxlan`
    * `geneve`
    * `vxlan-gpe`

### [stringset](#id200)

type:
:   enum

header:
:   linux/ethtool.h

entries:

### [header-flags](#id201)

type:
:   flags

name-prefix:
:   ethtool-flag-

doc:
:   common ethtool header flags

entries:
:   compact-bitsets:
    :   use compact bitsets in reply

    omit-reply:
    :   provide optional reply for SET or ACT requests

    stats:
    :   request statistics, if supported by the driver

### [module-fw-flash-status](#id202)

type:
:   enum

doc:
:   plug-in module firmware flashing status

header:
:   linux/ethtool.h

entries:
:   started:
    :   The firmware flashing process has started.

    in-progress:
    :   The firmware flashing process is in progress.

    completed:
    :   The firmware flashing process was completed successfully.

    error:
    :   The firmware flashing process was stopped due to an error.

### [c33-pse-ext-state](#id203)

doc:
:   “groups of PSE extended states functions. IEEE 802.3-2022 33.2.4.4 Variables”

type:
:   enum

name-prefix:
:   ethtool-c33-pse-ext-state-

header:
:   linux/ethtool.h

entries:
:   none:
    :   none

    error-condition:
    :   Group of error\_condition states

    mr-mps-valid:
    :   Group of mr\_mps\_valid states

    mr-pse-enable:
    :   Group of mr\_pse\_enable states

    option-detect-ted:
    :   Group of option\_detect\_ted states

    option-vport-lim:
    :   Group of option\_vport\_lim states

    ovld-detected:
    :   Group of ovld\_detected states

    power-not-available:
    :   Group of power\_not\_available states

    short-detected:
    :   Group of short\_detected states

### [phy-upstream-type](#id204)

enum-name:
:   phy-upstream

header:
:   linux/ethtool.h

type:
:   enum

name-prefix:
:   phy-upstream

entries:
:   * `mac`
    * `phy`

### [tcp-data-split](#id205)

type:
:   enum

entries:
:   * `unknown`
    * `disabled`
    * `enabled`

### [hwtstamp-source](#id206)

doc:
:   Source of the hardware timestamp

enum-name:
:   hwtstamp-source

name-prefix:
:   hwtstamp-source-

type:
:   enum

entries:
:   netdev:
    :   Hardware timestamp comes from a MAC or a device which has MAC and PHY integrated

    phylib:
    :   Hardware timestamp comes from one PHY device of the network topology

### [pse-event](#id207)

doc:
:   PSE event list for the PSE controller

type:
:   flags

name-prefix:
:   ethtool-

entries:
:   pse-event-over-current:
    :   PSE output current is too high

    pse-event-over-temp:
    :   PSE in over temperature state

    c33-pse-event-detection:
    :   detection process occur on the PSE. IEEE 802.3-2022 33.2.5 and 145.2.6 PSE detection of PDs. IEEE 802.3-202 30.9.1.1.5 aPSEPowerDetectionStatus

    c33-pse-event-classification:
    :   classification process occur on the PSE. IEEE 802.3-2022 33.2.6 and 145.2.8 classification of PDs mutual identification. IEEE 802.3-2022 30.9.1.1.8 aPSEPowerClassification.

    c33-pse-event-disconnection:
    :   PD has been disconnected on the PSE. IEEE 802.3-2022 33.3.8 and 145.3.9 PD Maintain Power Signature. IEEE 802.3-2022 33.5.1.2.9 MPS Absent. IEEE 802.3-2022 30.9.1.1.20 aPSEMPSAbsentCounter.

    pse-event-over-budget:
    :   PSE turned off due to over budget situation

    pse-event-sw-pw-control-error:
    :   PSE faced an error managing the power control from software

### [input-xfrm](#id208)

doc:
:   RSS hash function transformations.

type:
:   flags

enum-name:
:   None

name-prefix:
:   rxh-xfrm-

header:
:   linux/ethtool.h

entries:
:   sym-xor:
    :   XOR the corresponding source and destination fields of each specified protocol. Both copies of the XOR’ed fields are fed into the RSS and RXHASH calculation. Note that this XORing reduces the input set entropy and could be exploited to reduce the RSS queue spread.

    sym-or-xor:
    :   Similar to SYM\_XOR, except that one copy of the XOR’ed fields is replaced by an OR of the same fields.

### [rxfh-fields](#id209)

name-prefix:
:   rxh-

enum-name:
:   None

header:
:   linux/ethtool.h

type:
:   flags

entries:
:   l2da:


    vlan:


    l3-proto:


    ip-src:


    ip-dst:


    l4-b-0-1:
    :   src port in case of TCP/UDP/SCTP

    l4-b-2-3:
    :   dst port in case of TCP/UDP/SCTP

    gtp-teid:


    ip6-fl:
    :   IPv6 Flow Label

    discard:

## [Attribute sets](#id210)

### [header](#id211)

#### unspec (`unused`)

value:
:   0

#### dev-index (`u32`)

#### dev-name (`string`)

#### flags (`u32`)

enum:
:   [header-flags](#ethtool-definition-header-flags)

#### phy-index (`u32`)

### [bitset-bit](#id212)

#### unspec (`unused`)

value:
:   0

#### index (`u32`)

#### name (`string`)

#### value (`flag`)

### [bitset-bits](#id213)

#### unspec (`unused`)

value:
:   0

#### bit (`nest`)

multi-attr:
:   True

nested-attributes:
:   [bitset-bit](#ethtool-attribute-set-bitset-bit)

### [bitset](#id214)

#### unspec (`unused`)

value:
:   0

#### nomask (`flag`)

#### size (`u32`)

#### bits (`nest`)

nested-attributes:
:   [bitset-bits](#ethtool-attribute-set-bitset-bits)

#### value (`binary`)

#### mask (`binary`)

### [string](#id215)

#### unspec (`unused`)

value:
:   0

#### index (`u32`)

#### value (`string`)

### [strings](#id216)

#### unspec (`unused`)

value:
:   0

#### string (`nest`)

multi-attr:
:   True

nested-attributes:
:   [string](#ethtool-attribute-set-string)

### [stringset](#id217)

#### unspec (`unused`)

value:
:   0

#### id (`u32`)

#### count (`u32`)

#### strings (`nest`)

multi-attr:
:   True

nested-attributes:
:   [strings](#ethtool-attribute-set-strings)

### [stringsets](#id218)

#### unspec (`unused`)

value:
:   0

#### stringset (`nest`)

multi-attr:
:   True

nested-attributes:
:   [stringset](#ethtool-attribute-set-stringset)

### [strset](#id219)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### stringsets (`nest`)

nested-attributes:
:   [stringsets](#ethtool-attribute-set-stringsets)

#### counts-only (`flag`)

### [privflags](#id220)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### flags (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

### [rings](#id221)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### rx-max (`u32`)

#### rx-mini-max (`u32`)

#### rx-jumbo-max (`u32`)

#### tx-max (`u32`)

#### rx (`u32`)

#### rx-mini (`u32`)

#### rx-jumbo (`u32`)

#### tx (`u32`)

#### rx-buf-len (`u32`)

#### tcp-data-split (`u8`)

enum:
:   [tcp-data-split](#ethtool-definition-tcp-data-split)

#### cqe-size (`u32`)

#### tx-push (`u8`)

#### rx-push (`u8`)

#### tx-push-buf-len (`u32`)

#### tx-push-buf-len-max (`u32`)

#### hds-thresh (`u32`)

#### hds-thresh-max (`u32`)

### [mm-stat](#id222)

MAC Merge (802.3)

#### unspec (`unused`)

value:
:   0

#### pad (`pad`)

#### reassembly-errors (`u64`)

doc:
:   aMACMergeFrameAssErrorCount

#### smd-errors (`u64`)

doc:
:   aMACMergeFrameSmdErrorCount

#### reassembly-ok (`u64`)

doc:
:   aMACMergeFrameAssOkCount

#### rx-frag-count (`u64`)

doc:
:   aMACMergeFragCountRx

#### tx-frag-count (`u64`)

doc:
:   aMACMergeFragCountTx

#### hold-count (`u64`)

doc:
:   aMACMergeHoldCount

### [mm](#id223)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### pmac-enabled (`u8`)

#### tx-enabled (`u8`)

#### tx-active (`u8`)

#### tx-min-frag-size (`u32`)

#### rx-min-frag-size (`u32`)

#### verify-enabled (`u8`)

#### verify-status (`u8`)

#### verify-time (`u32`)

#### max-verify-time (`u32`)

#### stats (`nest`)

nested-attributes:
:   [mm-stat](#ethtool-attribute-set-mm-stat)

### [linkinfo](#id224)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### port (`u8`)

#### phyaddr (`u8`)

#### tp-mdix (`u8`)

#### tp-mdix-ctrl (`u8`)

#### transceiver (`u8`)

### [linkmodes](#id225)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### autoneg (`u8`)

#### ours (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### peer (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### speed (`u32`)

#### duplex (`u8`)

#### master-slave-cfg (`u8`)

#### master-slave-state (`u8`)

#### lanes (`u32`)

#### rate-matching (`u8`)

### [linkstate](#id226)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### link (`u8`)

#### sqi (`u32`)

#### sqi-max (`u32`)

#### ext-state (`u8`)

#### ext-substate (`u8`)

#### ext-down-cnt (`u32`)

### [debug](#id227)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### msgmask (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

### [wol](#id228)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### modes (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### sopass (`binary`)

### [features](#id229)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### hw (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### wanted (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### active (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### nochange (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

### [channels](#id230)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### rx-max (`u32`)

#### tx-max (`u32`)

#### other-max (`u32`)

#### combined-max (`u32`)

#### rx-count (`u32`)

#### tx-count (`u32`)

#### other-count (`u32`)

#### combined-count (`u32`)

### [irq-moderation](#id231)

#### unspec (`unused`)

value:
:   0

#### usec (`u32`)

#### pkts (`u32`)

#### comps (`u32`)

### [profile](#id232)

#### unspec (`unused`)

value:
:   0

#### irq-moderation (`nest`)

multi-attr:
:   True

nested-attributes:
:   [irq-moderation](#ethtool-attribute-set-irq-moderation)

### [coalesce](#id233)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### rx-usecs (`u32`)

#### rx-max-frames (`u32`)

#### rx-usecs-irq (`u32`)

#### rx-max-frames-irq (`u32`)

#### tx-usecs (`u32`)

#### tx-max-frames (`u32`)

#### tx-usecs-irq (`u32`)

#### tx-max-frames-irq (`u32`)

#### stats-block-usecs (`u32`)

#### use-adaptive-rx (`u8`)

#### use-adaptive-tx (`u8`)

#### pkt-rate-low (`u32`)

#### rx-usecs-low (`u32`)

#### rx-max-frames-low (`u32`)

#### tx-usecs-low (`u32`)

#### tx-max-frames-low (`u32`)

#### pkt-rate-high (`u32`)

#### rx-usecs-high (`u32`)

#### rx-max-frames-high (`u32`)

#### tx-usecs-high (`u32`)

#### tx-max-frames-high (`u32`)

#### rate-sample-interval (`u32`)

#### use-cqe-mode-tx (`u8`)

#### use-cqe-mode-rx (`u8`)

#### tx-aggr-max-bytes (`u32`)

#### tx-aggr-max-frames (`u32`)

#### tx-aggr-time-usecs (`u32`)

#### rx-profile (`nest`)

nested-attributes:
:   [profile](#ethtool-attribute-set-profile)

#### tx-profile (`nest`)

nested-attributes:
:   [profile](#ethtool-attribute-set-profile)

#### rx-cqe-frames (`u32`)

#### rx-cqe-nsecs (`u32`)

### [pause-stat](#id234)

#### unspec (`unused`)

value:
:   0

#### pad (`pad`)

#### tx-frames (`u64`)

#### rx-frames (`u64`)

#### tx-pause-storm-events (`u64`)

doc:
:   TX pause storm event count. Increments each time device detects that its pause assertion condition has been true for too long for normal operation. As a result, the device has temporarily disabled its own Pause TX function to protect the network from itself. This counter should never increment under normal overload conditions; it indicates catastrophic failure like an OS crash. The rate of incrementing is implementation specific.

### [pause](#id235)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### autoneg (`u8`)

#### rx (`u8`)

#### tx (`u8`)

#### stats (`nest`)

nested-attributes:
:   [pause-stat](#ethtool-attribute-set-pause-stat)

#### stats-src (`u32`)

### [eee](#id236)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### modes-ours (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### modes-peer (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### active (`u8`)

#### enabled (`u8`)

#### tx-lpi-enabled (`u8`)

#### tx-lpi-timer (`u32`)

### [ts-stat](#id237)

#### unspec (`unused`)

value:
:   0

#### tx-pkts (`uint`)

#### tx-lost (`uint`)

#### tx-err (`uint`)

#### tx-onestep-pkts-unconfirmed (`uint`)

### [ts-hwtstamp-provider](#id238)

#### unspec (`unused`)

value:
:   0

#### index (`u32`)

#### qualifier (`u32`)

### [tsinfo](#id239)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### timestamping (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### tx-types (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### rx-filters (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### phc-index (`u32`)

#### stats (`nest`)

nested-attributes:
:   [ts-stat](#ethtool-attribute-set-ts-stat)

#### hwtstamp-provider (`nest`)

nested-attributes:
:   [ts-hwtstamp-provider](#ethtool-attribute-set-ts-hwtstamp-provider)

#### hwtstamp-source (`u32`)

enum:
:   [hwtstamp-source](#ethtool-definition-hwtstamp-source)

#### hwtstamp-phyindex (`u32`)

### [cable-result](#id240)

#### unspec (`unused`)

value:
:   0

#### pair (`u8`)

doc:
:   ETHTOOL\_A\_CABLE\_PAIR

#### code (`u8`)

doc:
:   ETHTOOL\_A\_CABLE\_RESULT\_CODE

#### src (`u32`)

doc:
:   ETHTOOL\_A\_CABLE\_INF\_SRC

### [cable-fault-length](#id241)

#### unspec (`unused`)

value:
:   0

#### pair (`u8`)

#### cm (`u32`)

#### src (`u32`)

### [cable-nest](#id242)

#### unspec (`unused`)

value:
:   0

#### result (`nest`)

nested-attributes:
:   [cable-result](#ethtool-attribute-set-cable-result)

#### fault-length (`nest`)

nested-attributes:
:   [cable-fault-length](#ethtool-attribute-set-cable-fault-length)

### [cable-test](#id243)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

### [cable-test-ntf](#id244)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### status (`u8`)

doc:
:   \_STARTED/\_COMPLETE

#### nest (`nest`)

nested-attributes:
:   [cable-nest](#ethtool-attribute-set-cable-nest)

### [cable-test-tdr-cfg](#id245)

#### unspec (`unused`)

value:
:   0

#### first (`u32`)

#### last (`u32`)

#### step (`u32`)

#### pair (`u8`)

### [cable-test-tdr-ntf](#id246)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### status (`u8`)

#### nest (`nest`)

nested-attributes:
:   [cable-nest](#ethtool-attribute-set-cable-nest)

### [cable-test-tdr](#id247)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### cfg (`nest`)

nested-attributes:
:   [cable-test-tdr-cfg](#ethtool-attribute-set-cable-test-tdr-cfg)

### [tunnel-udp-entry](#id248)

#### unspec (`unused`)

value:
:   0

#### port (`u16`)

byte-order:
:   big-endian

#### type (`u32`)

enum:
:   [udp-tunnel-type](#ethtool-definition-udp-tunnel-type)

### [tunnel-udp-table](#id249)

#### unspec (`unused`)

value:
:   0

#### size (`u32`)

#### types (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### entry (`nest`)

multi-attr:
:   True

nested-attributes:
:   [tunnel-udp-entry](#ethtool-attribute-set-tunnel-udp-entry)

### [tunnel-udp](#id250)

#### unspec (`unused`)

value:
:   0

#### table (`nest`)

nested-attributes:
:   [tunnel-udp-table](#ethtool-attribute-set-tunnel-udp-table)

### [tunnel-info](#id251)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### udp-ports (`nest`)

nested-attributes:
:   [tunnel-udp](#ethtool-attribute-set-tunnel-udp)

### [fec-hist](#id252)

#### pad (`pad`)

#### bin-low (`u32`)

doc:
:   Low bound of FEC bin (inclusive)

#### bin-high (`u32`)

doc:
:   High bound of FEC bin (inclusive)

#### bin-val (`uint`)

doc:
:   Error count in the bin (optional if per-lane values exist)

#### bin-val-per-lane (`binary`)

sub-type:
:   u64

doc:
:   An array of per-lane error counters in the bin (optional)

### [fec-stat](#id253)

#### unspec (`unused`)

value:
:   0

#### pad (`pad`)

#### corrected (`binary`)

sub-type:
:   u64

#### uncorr (`binary`)

sub-type:
:   u64

#### corr-bits (`binary`)

sub-type:
:   u64

#### hist (`nest`)

multi-attr:
:   True

nested-attributes:
:   [fec-hist](#ethtool-attribute-set-fec-hist)

### [fec](#id254)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### modes (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### auto (`u8`)

#### active (`u32`)

#### stats (`nest`)

nested-attributes:
:   [fec-stat](#ethtool-attribute-set-fec-stat)

### [module-eeprom](#id255)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### offset (`u32`)

#### length (`u32`)

#### page (`u8`)

#### bank (`u8`)

#### i2c-address (`u8`)

#### data (`binary`)

### [stats-grp](#id256)

#### unspec (`unused`)

value:
:   0

#### pad (`pad`)

#### id (`u32`)

#### ss-id (`u32`)

#### stat (`u64`)

type-value:
:   [‘id’]

#### hist-rx (`nest`)

nested-attributes:
:   [stats-grp-hist](#ethtool-attribute-set-stats-grp-hist)

#### hist-tx (`nest`)

nested-attributes:
:   [stats-grp-hist](#ethtool-attribute-set-stats-grp-hist)

#### hist-bkt-low (`u32`)

#### hist-bkt-hi (`u32`)

#### hist-val (`u64`)

### [stats-grp-hist](#id257)

#### hist-bkt-low

#### hist-bkt-hi

#### hist-val

### [stats](#id258)

#### unspec (`unused`)

value:
:   0

#### pad (`pad`)

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### groups (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### grp (`nest`)

nested-attributes:
:   [stats-grp](#ethtool-attribute-set-stats-grp)

#### src (`u32`)

### [phc-vclocks](#id259)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### num (`u32`)

#### index (`binary`)

sub-type:
:   s32

### [module](#id260)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### power-mode-policy (`u8`)

#### power-mode (`u8`)

### [c33-pse-pw-limit](#id261)

#### unspec (`unused`)

value:
:   0

#### min (`u32`)

#### max (`u32`)

### [pse](#id262)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### podl-pse-admin-state (`u32`)

name-prefix:
:   ethtool-a-

#### podl-pse-admin-control (`u32`)

name-prefix:
:   ethtool-a-

#### podl-pse-pw-d-status (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-admin-state (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-admin-control (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-pw-d-status (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-pw-class (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-actual-pw (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-ext-state (`u32`)

name-prefix:
:   ethtool-a-

enum:
:   [c33-pse-ext-state](#ethtool-definition-c33-pse-ext-state)

#### c33-pse-ext-substate (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-avail-pw-limit (`u32`)

name-prefix:
:   ethtool-a-

#### c33-pse-pw-limit-ranges (`nest`)

name-prefix:
:   ethtool-a-

multi-attr:
:   True

nested-attributes:
:   [c33-pse-pw-limit](#ethtool-attribute-set-c33-pse-pw-limit)

#### pse-pw-d-id (`u32`)

name-prefix:
:   ethtool-a-

#### pse-prio-max (`u32`)

name-prefix:
:   ethtool-a-

#### pse-prio (`u32`)

name-prefix:
:   ethtool-a-

### [flow](#id263)

Flow types, corresponding to those defined in the old
ethtool header for RXFH and RXNFC as ${PROTO}\_FLOW.
The values are not matching the old ones to avoid carrying
into Netlink the IP\_USER\_FLOW vs IPV4\_FLOW vs IPV4\_USER\_FLOW confusion.

#### ether (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### ip4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### ip6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### tcp4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### tcp6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### udp4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### udp6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### sctp4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### sctp6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### ah4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### ah6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### esp4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### esp6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### ah-esp4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### ah-esp6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpc4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpc6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpc-teid4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpc-teid6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu-eh4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu-eh6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu-ul4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu-ul6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu-dl4 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

#### gtpu-dl6 (`uint`)

enum:
:   [rxfh-fields](#ethtool-definition-rxfh-fields)

### [rss](#id264)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### context (`u32`)

#### hfunc (`u32`)

#### indir (`binary`)

sub-type:
:   u32

#### hkey (`binary`)

#### input-xfrm (`u32`)

enum:
:   [input-xfrm](#ethtool-definition-input-xfrm)

#### start-context (`u32`)

#### flow-hash (`nest`)

nested-attributes:
:   [flow](#ethtool-attribute-set-flow)

### [plca](#id265)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### version (`u16`)

#### enabled (`u8`)

#### status (`u8`)

#### node-cnt (`u32`)

#### node-id (`u32`)

#### to-tmr (`u32`)

#### burst-cnt (`u32`)

#### burst-tmr (`u32`)

### [module-fw-flash](#id266)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### file-name (`string`)

#### password (`u32`)

#### status (`u32`)

enum:
:   [module-fw-flash-status](#ethtool-definition-module-fw-flash-status)

#### status-msg (`string`)

#### done (`uint`)

#### total (`uint`)

### [phy](#id267)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### index (`u32`)

#### drvname (`string`)

#### name (`string`)

#### upstream-type (`u32`)

enum:
:   [phy-upstream-type](#ethtool-definition-phy-upstream-type)

#### upstream-index (`u32`)

#### upstream-sfp-name (`string`)

#### downstream-sfp-name (`string`)

### [tsconfig](#id268)

#### unspec (`unused`)

value:
:   0

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### hwtstamp-provider (`nest`)

nested-attributes:
:   [ts-hwtstamp-provider](#ethtool-attribute-set-ts-hwtstamp-provider)

#### tx-types (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### rx-filters (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

#### hwtstamp-flags (`nest`)

nested-attributes:
:   [bitset](#ethtool-attribute-set-bitset)

### [pse-ntf](#id269)

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### events (`uint`)

enum:
:   [pse-event](#ethtool-definition-pse-event)

doc:
:   List of events reported by the PSE controller

### [mse-capabilities](#id270)

MSE capabilities attribute set

#### max-average-mse (`uint`)

#### max-peak-mse (`uint`)

#### refresh-rate-ps (`uint`)

#### num-symbols (`uint`)

### [mse-snapshot](#id271)

MSE snapshot attribute set

#### average-mse (`uint`)

#### peak-mse (`uint`)

#### worst-peak-mse (`uint`)

### [mse](#id272)

#### header (`nest`)

nested-attributes:
:   [header](#ethtool-attribute-set-header)

#### capabilities (`nest`)

nested-attributes:
:   [mse-capabilities](#ethtool-attribute-set-mse-capabilities)

#### channel-a (`nest`)

nested-attributes:
:   [mse-snapshot](#ethtool-attribute-set-mse-snapshot)

#### channel-b (`nest`)

nested-attributes:
:   [mse-snapshot](#ethtool-attribute-set-mse-snapshot)

#### channel-c (`nest`)

nested-attributes:
:   [mse-snapshot](#ethtool-attribute-set-mse-snapshot)

#### channel-d (`nest`)

nested-attributes:
:   [mse-snapshot](#ethtool-attribute-set-mse-snapshot)

#### worst-channel (`nest`)

nested-attributes:
:   [mse-snapshot](#ethtool-attribute-set-mse-snapshot)

#### link (`nest`)

nested-attributes:
:   [mse-snapshot](#ethtool-attribute-set-mse-snapshot)
