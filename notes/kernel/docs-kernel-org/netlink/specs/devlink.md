# Familydevlinknetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/devlink.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `devlink` netlink specification](#id21)

## [Summary](#id22)

Partial family for Devlink.

## [Operations](#id23)

### [get](#id24)

Get devlink instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’, ‘dump’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `reload-failed`, `dev-stats`]

dump:
:   **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `reload-failed`, `dev-stats`]

### [port-get](#id25)

Get devlink port instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

### [port-set](#id26)

Set devlink port instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `port-type`, `port-function`]

### [port-new](#id27)

Create devlink port instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `port-flavour`, `port-pci-pf-number`, `port-pci-sf-number`, `port-controller-number`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

### [port-del](#id28)

Delete devlink port instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

### [port-split](#id29)

Split devlink port instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `port-split-count`]

### [port-unsplit](#id30)

Unplit devlink port instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

### [sb-get](#id31)

Get shared buffer instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`]

### [sb-pool-get](#id32)

Get shared buffer pool instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`, `sb-pool-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`, `sb-pool-index`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`, `sb-pool-index`]

### [sb-pool-set](#id33)

Set shared buffer pool instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`, `sb-pool-index`, `sb-pool-threshold-type`, `sb-pool-size`]

### [sb-port-pool-get](#id34)

Get shared buffer port-pool combinations and threshold.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-index`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-index`]

### [sb-port-pool-set](#id35)

Set shared buffer port-pool combinations and threshold.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-index`, `sb-threshold`]

### [sb-tc-pool-bind-get](#id36)

Get shared buffer port-TC to pool bindings and threshold.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-type`, `sb-tc-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-type`, `sb-tc-index`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-type`, `sb-tc-index`]

### [sb-tc-pool-bind-set](#id37)

Set shared buffer port-TC to pool bindings and threshold.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `sb-index`, `sb-pool-index`, `sb-pool-type`, `sb-tc-index`, `sb-threshold`]

### [sb-occ-snapshot](#id38)

Take occupancy snapshot of shared buffer.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`]

### [sb-occ-max-clear](#id39)

Clear occupancy watermarks of shared buffer.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `sb-index`]

### [eswitch-get](#id40)

Get eswitch attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `eswitch-mode`, `eswitch-inline-mode`, `eswitch-encap-mode`]

### [eswitch-set](#id41)

Set eswitch attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `eswitch-mode`, `eswitch-inline-mode`, `eswitch-encap-mode`]

### [dpipe-table-get](#id42)

Get dpipe table attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `dpipe-table-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `dpipe-tables`]

### [dpipe-entries-get](#id43)

Get dpipe entries attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `dpipe-table-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `dpipe-entries`]

### [dpipe-headers-get](#id44)

Get dpipe headers attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `dpipe-headers`]

### [dpipe-table-counters-set](#id45)

Set dpipe counter attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `dpipe-table-name`, `dpipe-table-counters-enabled`]

### [resource-set](#id46)

Set resource attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `resource-id`, `resource-size`]

### [resource-dump](#id47)

Get resource attributes.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `resource-list`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `resource-scope-mask`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `resource-list`]

### [reload](#id48)

Reload devlink.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-dev-lock

    **post**
    :   devlink-nl-post-doit-dev-lock

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `reload-action`, `reload-limits`, `netns-pid`, `netns-fd`, `netns-id`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `reload-actions-performed`]

### [param-get](#id49)

Get param instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `param-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `param-name`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `param-name`]

### [param-set](#id50)

Set param instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `param-name`, `param-type`, `param-value-cmode`, `param-reset-default`]

### [region-get](#id51)

Get region instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`]

### [region-new](#id52)

Create region snapshot.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`, `region-snapshot-id`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`, `region-snapshot-id`]

### [region-del](#id53)

Delete region snapshot.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`, `region-snapshot-id`]

### [region-read](#id54)

Read region data.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘dump-strict’]

flags:
:   [`admin-perm`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`, `region-snapshot-id`, `region-direct`, `region-chunk-addr`, `region-chunk-len`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `region-name`]

### [port-param-get](#id55)

Get port param instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’, ‘dump-strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

dump:
:   **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

### [port-param-set](#id56)

Set port param instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

### [info-get](#id57)

Get device information, like driver name, hardware and firmware versions
etc.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’, ‘dump’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `info-driver-name`, `info-serial-number`, `info-version-fixed`, `info-version-running`, `info-version-stored`, `info-board-serial-number`]

dump:
:   **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `info-driver-name`, `info-serial-number`, `info-version-fixed`, `info-version-running`, `info-version-stored`, `info-board-serial-number`]

### [health-reporter-get](#id58)

Get health reporter instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

### [health-reporter-set](#id59)

Set health reporter instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`, `health-reporter-graceful-period`, `health-reporter-auto-recover`, `health-reporter-auto-dump`, `health-reporter-burst-period`]

### [health-reporter-recover](#id60)

Recover health reporter instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

### [health-reporter-diagnose](#id61)

Diagnose health reporter instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

### [health-reporter-dump-get](#id62)

Dump health reporter instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘dump-strict’]

flags:
:   [`admin-perm`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

    **reply**
    :   attributes:
        :   [`fmsg`]

### [health-reporter-dump-clear](#id63)

Clear dump of health reporter instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

### [flash-update](#id64)

Flash update devlink instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `flash-update-file-name`, `flash-update-component`, `flash-update-overwrite-mask`]

### [trap-get](#id65)

Get trap instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-name`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-name`]

### [trap-set](#id66)

Set trap instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-name`, `trap-action`]

### [trap-group-get](#id67)

Get trap group instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-group-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-group-name`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-group-name`]

### [trap-group-set](#id68)

Set trap group instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-group-name`, `trap-action`, `trap-policer-id`]

### [trap-policer-get](#id69)

Get trap policer instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-policer-id`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-policer-id`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-policer-id`]

### [trap-policer-set](#id70)

Get trap policer instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `trap-policer-id`, `trap-policer-rate`, `trap-policer-burst`]

### [health-reporter-test](#id71)

Test health reporter instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit-port-optional

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `health-reporter-name`]

### [rate-get](#id72)

Get rate instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `rate-node-name`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `rate-node-name`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`, `rate-node-name`]

### [rate-set](#id73)

Set rate instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `rate-node-name`, `rate-tx-share`, `rate-tx-max`, `rate-tx-priority`, `rate-tx-weight`, `rate-parent-node-name`, `rate-tc-bws`]

### [rate-new](#id74)

Create rate instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `rate-node-name`, `rate-tx-share`, `rate-tx-max`, `rate-tx-priority`, `rate-tx-weight`, `rate-parent-node-name`, `rate-tc-bws`]

### [rate-del](#id75)

Delete rate instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `rate-node-name`]

### [linecard-get](#id76)

Get line card instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `linecard-index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `linecard-index`]

dump:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `linecard-index`]

### [linecard-set](#id77)

Set line card instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `linecard-index`, `linecard-type`]

### [selftests-get](#id78)

Get device selftest instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’, ‘dump’]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

    **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

dump:
:   **reply**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`]

### [selftests-run](#id79)

Run device selftest instances.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   devlink-nl-pre-doit

    **post**
    :   devlink-nl-post-doit

    **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `selftests`]

### [notify-filter-set](#id80)

Set notification messages socket filter.

attribute-set:
:   [devlink](#devlink-attribute-set-devlink)

do:
:   **request**
    :   attributes:
        :   [`bus-name`, `dev-name`, `index`, `port-index`]

## [Definitions](#id81)

### [sb-pool-type](#id82)

type:
:   enum

entries:
:   ingress:


    egress:

### [port-type](#id83)

type:
:   enum

entries:
:   notset:


    auto:


    eth:


    ib:

### [port-flavour](#id84)

type:
:   enum

entries:
:   physical:


    cpu:


    dsa:


    pci-pf:


    pci-vf:


    virtual:


    unused:


    pci-sf:

### [port-fn-state](#id85)

type:
:   enum

entries:
:   inactive:


    active:

### [port-fn-opstate](#id86)

type:
:   enum

entries:
:   detached:


    attached:

### [port-fn-attr-cap](#id87)

type:
:   enum

entries:
:   roce-bit:


    migratable-bit:


    ipsec-crypto-bit:


    ipsec-packet-bit:

### [rate-type](#id88)

type:
:   enum

entries:
:   leaf:


    node:

### [sb-threshold-type](#id89)

type:
:   enum

entries:
:   static:


    dynamic:

### [eswitch-mode](#id90)

type:
:   enum

entries:
:   legacy:


    switchdev:


    switchdev-inactive:

### [eswitch-inline-mode](#id91)

type:
:   enum

entries:
:   none:


    link:


    network:


    transport:

### [eswitch-encap-mode](#id92)

type:
:   enum

entries:
:   none:


    basic:

### [dpipe-header-id](#id93)

type:
:   enum

entries:
:   ethernet:


    ipv4:


    ipv6:

### [dpipe-match-type](#id94)

type:
:   enum

entries:
:   field-exact:

### [dpipe-action-type](#id95)

type:
:   enum

entries:
:   field-modify:

### [dpipe-field-mapping-type](#id96)

type:
:   enum

entries:
:   none:


    ifindex:

### [resource-unit](#id97)

type:
:   enum

entries:
:   entry:

### [resource-scope](#id98)

type:
:   enum

entries:
:   dev:


    port:

### [reload-action](#id99)

type:
:   enum

entries:
:   driver-reinit:


    fw-activate:

### [param-cmode](#id100)

type:
:   enum

entries:
:   runtime:


    driverinit:


    permanent:

### [flash-overwrite](#id101)

type:
:   enum

entries:
:   settings-bit:


    identifiers-bit:

### [trap-action](#id102)

type:
:   enum

entries:
:   drop:


    trap:


    mirror:

### [trap-type](#id103)

type:
:   enum

entries:
:   drop:


    exception:


    control:

### [var-attr-type](#id104)

type:
:   enum

entries:
:   u8:


    u16:


    u32:


    u64:


    string:


    flag:


    nul-string:


    binary:

### [rate-tc-index-max](#id105)

type:
:   const

value:
:   7

## [Attribute sets](#id106)

### [devlink](#id107)

#### bus-name (`string`)

value:
:   1

#### dev-name (`string`)

#### port-index (`u32`)

#### port-type (`u16`)

enum:
:   [port-type](#devlink-definition-port-type)

#### port-desired-type (`u16`)

#### port-netdev-ifindex (`u32`)

#### port-netdev-name (`string`)

#### port-ibdev-name (`string`)

#### port-split-count (`u32`)

#### port-split-group (`u32`)

#### sb-index (`u32`)

#### sb-size (`u32`)

#### sb-ingress-pool-count (`u16`)

#### sb-egress-pool-count (`u16`)

#### sb-ingress-tc-count (`u16`)

#### sb-egress-tc-count (`u16`)

#### sb-pool-index (`u16`)

#### sb-pool-type (`u8`)

enum:
:   [sb-pool-type](#devlink-definition-sb-pool-type)

#### sb-pool-size (`u32`)

#### sb-pool-threshold-type (`u8`)

enum:
:   [sb-threshold-type](#devlink-definition-sb-threshold-type)

#### sb-threshold (`u32`)

#### sb-tc-index (`u16`)

#### sb-occ-cur (`u32`)

#### sb-occ-max (`u32`)

#### eswitch-mode (`u16`)

enum:
:   [eswitch-mode](#devlink-definition-eswitch-mode)

#### eswitch-inline-mode (`u8`)

enum:
:   [eswitch-inline-mode](#devlink-definition-eswitch-inline-mode)

#### dpipe-tables (`nest`)

nested-attributes:
:   [dl-dpipe-tables](#devlink-attribute-set-dl-dpipe-tables)

#### dpipe-table (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-table](#devlink-attribute-set-dl-dpipe-table)

#### dpipe-table-name (`string`)

#### dpipe-table-size (`u64`)

#### dpipe-table-matches (`nest`)

nested-attributes:
:   [dl-dpipe-table-matches](#devlink-attribute-set-dl-dpipe-table-matches)

#### dpipe-table-actions (`nest`)

nested-attributes:
:   [dl-dpipe-table-actions](#devlink-attribute-set-dl-dpipe-table-actions)

#### dpipe-table-counters-enabled (`u8`)

#### dpipe-entries (`nest`)

nested-attributes:
:   [dl-dpipe-entries](#devlink-attribute-set-dl-dpipe-entries)

#### dpipe-entry (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-entry](#devlink-attribute-set-dl-dpipe-entry)

#### dpipe-entry-index (`u64`)

#### dpipe-entry-match-values (`nest`)

nested-attributes:
:   [dl-dpipe-entry-match-values](#devlink-attribute-set-dl-dpipe-entry-match-values)

#### dpipe-entry-action-values (`nest`)

nested-attributes:
:   [dl-dpipe-entry-action-values](#devlink-attribute-set-dl-dpipe-entry-action-values)

#### dpipe-entry-counter (`u64`)

#### dpipe-match (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-match](#devlink-attribute-set-dl-dpipe-match)

#### dpipe-match-value (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-match-value](#devlink-attribute-set-dl-dpipe-match-value)

#### dpipe-match-type (`u32`)

enum:
:   [dpipe-match-type](#devlink-definition-dpipe-match-type)

#### dpipe-action (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-action](#devlink-attribute-set-dl-dpipe-action)

#### dpipe-action-value (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-action-value](#devlink-attribute-set-dl-dpipe-action-value)

#### dpipe-action-type (`u32`)

enum:
:   [dpipe-action-type](#devlink-definition-dpipe-action-type)

#### dpipe-value (`binary`)

#### dpipe-value-mask (`binary`)

#### dpipe-value-mapping (`u32`)

#### dpipe-headers (`nest`)

nested-attributes:
:   [dl-dpipe-headers](#devlink-attribute-set-dl-dpipe-headers)

#### dpipe-header (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-header](#devlink-attribute-set-dl-dpipe-header)

#### dpipe-header-name (`string`)

#### dpipe-header-id (`u32`)

enum:
:   [dpipe-header-id](#devlink-definition-dpipe-header-id)

#### dpipe-header-fields (`nest`)

nested-attributes:
:   [dl-dpipe-header-fields](#devlink-attribute-set-dl-dpipe-header-fields)

#### dpipe-header-global (`u8`)

#### dpipe-header-index (`u32`)

#### dpipe-field (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-dpipe-field](#devlink-attribute-set-dl-dpipe-field)

#### dpipe-field-name (`string`)

#### dpipe-field-id (`u32`)

#### dpipe-field-bitwidth (`u32`)

#### dpipe-field-mapping-type (`u32`)

enum:
:   [dpipe-field-mapping-type](#devlink-definition-dpipe-field-mapping-type)

#### pad (`pad`)

#### eswitch-encap-mode (`u8`)

enum:
:   [eswitch-encap-mode](#devlink-definition-eswitch-encap-mode)

#### resource-list (`nest`)

nested-attributes:
:   [dl-resource-list](#devlink-attribute-set-dl-resource-list)

#### resource (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-resource](#devlink-attribute-set-dl-resource)

#### resource-name (`string`)

#### resource-id (`u64`)

#### resource-size (`u64`)

#### resource-size-new (`u64`)

#### resource-size-valid (`u8`)

#### resource-size-min (`u64`)

#### resource-size-max (`u64`)

#### resource-size-gran (`u64`)

#### resource-unit (`u8`)

enum:
:   [resource-unit](#devlink-definition-resource-unit)

#### resource-occ (`u64`)

#### dpipe-table-resource-id (`u64`)

#### dpipe-table-resource-units (`u64`)

#### port-flavour (`u16`)

enum:
:   [port-flavour](#devlink-definition-port-flavour)

#### port-number (`u32`)

#### port-split-subport-number (`u32`)

#### param (`nest`)

nested-attributes:
:   [dl-param](#devlink-attribute-set-dl-param)

#### param-name (`string`)

#### param-generic (`flag`)

#### param-type (`u8`)

enum:
:   [var-attr-type](#devlink-definition-var-attr-type)

#### param-value-cmode (`u8`)

enum:
:   [param-cmode](#devlink-definition-param-cmode)

value:
:   87

#### region-name (`string`)

#### region-size (`u64`)

#### region-snapshots (`nest`)

nested-attributes:
:   [dl-region-snapshots](#devlink-attribute-set-dl-region-snapshots)

#### region-snapshot (`nest`)

nested-attributes:
:   [dl-region-snapshot](#devlink-attribute-set-dl-region-snapshot)

#### region-snapshot-id (`u32`)

#### region-chunks (`nest`)

nested-attributes:
:   [dl-region-chunks](#devlink-attribute-set-dl-region-chunks)

#### region-chunk (`nest`)

nested-attributes:
:   [dl-region-chunk](#devlink-attribute-set-dl-region-chunk)

#### region-chunk-data (`binary`)

#### region-chunk-addr (`u64`)

#### region-chunk-len (`u64`)

#### info-driver-name (`string`)

#### info-serial-number (`string`)

#### info-version-fixed (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-info-version](#devlink-attribute-set-dl-info-version)

#### info-version-running (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-info-version](#devlink-attribute-set-dl-info-version)

#### info-version-stored (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-info-version](#devlink-attribute-set-dl-info-version)

#### info-version-name (`string`)

#### info-version-value (`string`)

#### sb-pool-cell-size (`u32`)

#### fmsg (`nest`)

nested-attributes:
:   [dl-fmsg](#devlink-attribute-set-dl-fmsg)

#### fmsg-obj-nest-start (`flag`)

#### fmsg-pair-nest-start (`flag`)

#### fmsg-arr-nest-start (`flag`)

#### fmsg-nest-end (`flag`)

#### fmsg-obj-name (`string`)

#### fmsg-obj-value-type (`u8`)

enum:
:   [var-attr-type](#devlink-definition-var-attr-type)

#### health-reporter (`nest`)

value:
:   114

nested-attributes:
:   [dl-health-reporter](#devlink-attribute-set-dl-health-reporter)

#### health-reporter-name (`string`)

#### health-reporter-state (`u8`)

#### health-reporter-err-count (`u64`)

#### health-reporter-recover-count (`u64`)

#### health-reporter-dump-ts (`u64`)

#### health-reporter-graceful-period (`u64`)

#### health-reporter-auto-recover (`u8`)

#### flash-update-file-name (`string`)

#### flash-update-component (`string`)

#### flash-update-status-msg (`string`)

#### flash-update-status-done (`u64`)

#### flash-update-status-total (`u64`)

#### port-pci-pf-number (`u16`)

#### port-pci-vf-number (`u16`)

#### stats (`nest`)

nested-attributes:
:   [dl-attr-stats](#devlink-attribute-set-dl-attr-stats)

#### trap-name (`string`)

#### trap-action (`u8`)

enum:
:   [trap-action](#devlink-definition-trap-action)

#### trap-type (`u8`)

enum:
:   [trap-type](#devlink-definition-trap-type)

#### trap-generic (`flag`)

#### trap-metadata (`nest`)

nested-attributes:
:   [dl-trap-metadata](#devlink-attribute-set-dl-trap-metadata)

#### trap-group-name (`string`)

#### reload-failed (`u8`)

#### health-reporter-dump-ts-ns (`u64`)

#### netns-fd (`u32`)

#### netns-pid (`u32`)

#### netns-id (`u32`)

#### health-reporter-auto-dump (`u8`)

#### trap-policer-id (`u32`)

#### trap-policer-rate (`u64`)

#### trap-policer-burst (`u64`)

#### port-function (`nest`)

nested-attributes:
:   [dl-port-function](#devlink-attribute-set-dl-port-function)

#### info-board-serial-number (`string`)

#### port-lanes (`u32`)

#### port-splittable (`u8`)

#### port-external (`u8`)

#### port-controller-number (`u32`)

#### flash-update-status-timeout (`u64`)

#### flash-update-overwrite-mask (`bitfield32`)

enum:
:   [flash-overwrite](#devlink-definition-flash-overwrite)

enum-as-flags:
:   True

#### reload-action (`u8`)

enum:
:   [reload-action](#devlink-definition-reload-action)

#### reload-actions-performed (`bitfield32`)

enum:
:   [reload-action](#devlink-definition-reload-action)

enum-as-flags:
:   True

#### reload-limits (`bitfield32`)

enum:
:   [reload-action](#devlink-definition-reload-action)

enum-as-flags:
:   True

#### dev-stats (`nest`)

nested-attributes:
:   [dl-dev-stats](#devlink-attribute-set-dl-dev-stats)

#### reload-stats (`nest`)

nested-attributes:
:   [dl-reload-stats](#devlink-attribute-set-dl-reload-stats)

#### reload-stats-entry (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-reload-stats-entry](#devlink-attribute-set-dl-reload-stats-entry)

#### reload-stats-limit (`u8`)

#### reload-stats-value (`u32`)

#### remote-reload-stats (`nest`)

nested-attributes:
:   [dl-reload-stats](#devlink-attribute-set-dl-reload-stats)

#### reload-action-info (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-reload-act-info](#devlink-attribute-set-dl-reload-act-info)

#### reload-action-stats (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-reload-act-stats](#devlink-attribute-set-dl-reload-act-stats)

#### port-pci-sf-number (`u32`)

#### rate-type (`u16`)

enum:
:   [rate-type](#devlink-definition-rate-type)

#### rate-tx-share (`u64`)

#### rate-tx-max (`u64`)

#### rate-node-name (`string`)

#### rate-parent-node-name (`string`)

#### region-max-snapshots (`u32`)

#### linecard-index (`u32`)

#### linecard-state (`u8`)

#### linecard-type (`string`)

#### linecard-supported-types (`nest`)

nested-attributes:
:   [dl-linecard-supported-types](#devlink-attribute-set-dl-linecard-supported-types)

#### selftests (`nest`)

value:
:   176

nested-attributes:
:   [dl-selftest-id](#devlink-attribute-set-dl-selftest-id)

#### rate-tx-priority (`u32`)

#### rate-tx-weight (`u32`)

#### region-direct (`flag`)

#### rate-tc-bws (`nest`)

multi-attr:
:   True

nested-attributes:
:   [dl-rate-tc-bws](#devlink-attribute-set-dl-rate-tc-bws)

#### health-reporter-burst-period (`u64`)

doc:
:   Time (in msec) for recoveries before starting the grace period.

#### param-reset-default (`flag`)

doc:
:   Request restoring parameter to its default value.

value:
:   183

#### index (`uint`)

doc:
:   Unique devlink instance index.

#### resource-scope-mask (`u32`)

enum:
:   [resource-scope](#devlink-definition-resource-scope)

enum-as-flags:
:   True

doc:
:   Bitmask selecting which resource classes to include in a resource-dump response. Bit 0 (dev) selects device-level resources; bit 1 (port) selects port-level resources. When absent all classes are returned.

### [dl-dev-stats](#id108)

#### reload-stats

#### remote-reload-stats

### [dl-reload-stats](#id109)

#### reload-action-info

### [dl-reload-act-info](#id110)

#### reload-action

#### reload-action-stats

### [dl-reload-act-stats](#id111)

#### reload-stats-entry

### [dl-reload-stats-entry](#id112)

#### reload-stats-limit

#### reload-stats-value

### [dl-info-version](#id113)

#### info-version-name

#### info-version-value

### [dl-port-function](#id114)

#### hw-addr (`binary`)

name-prefix:
:   devlink-port-function-attr-

value:
:   1

#### state (`u8`)

enum:
:   [port-fn-state](#devlink-definition-port-fn-state)

#### opstate (`u8`)

enum:
:   [port-fn-opstate](#devlink-definition-port-fn-opstate)

#### caps (`bitfield32`)

enum:
:   [port-fn-attr-cap](#devlink-definition-port-fn-attr-cap)

enum-as-flags:
:   True

### [dl-dpipe-tables](#id115)

#### dpipe-table

### [dl-dpipe-table](#id116)

#### dpipe-table-name

#### dpipe-table-size

#### dpipe-table-name

#### dpipe-table-size

#### dpipe-table-matches

#### dpipe-table-actions

#### dpipe-table-counters-enabled

#### dpipe-table-resource-id

#### dpipe-table-resource-units

### [dl-dpipe-table-matches](#id117)

#### dpipe-match

### [dl-dpipe-table-actions](#id118)

#### dpipe-action

### [dl-dpipe-entries](#id119)

#### dpipe-entry

### [dl-dpipe-entry](#id120)

#### dpipe-entry-index

#### dpipe-entry-match-values

#### dpipe-entry-action-values

#### dpipe-entry-counter

### [dl-dpipe-entry-match-values](#id121)

#### dpipe-match-value

### [dl-dpipe-entry-action-values](#id122)

#### dpipe-action-value

### [dl-dpipe-match](#id123)

#### dpipe-match-type

#### dpipe-header-id

#### dpipe-header-global

#### dpipe-header-index

#### dpipe-field-id

### [dl-dpipe-match-value](#id124)

#### dpipe-match

#### dpipe-value

#### dpipe-value-mask

#### dpipe-value-mapping

### [dl-dpipe-action](#id125)

#### dpipe-action-type

#### dpipe-header-id

#### dpipe-header-global

#### dpipe-header-index

#### dpipe-field-id

### [dl-dpipe-action-value](#id126)

#### dpipe-action

#### dpipe-value

#### dpipe-value-mask

#### dpipe-value-mapping

### [dl-dpipe-headers](#id127)

#### dpipe-header

### [dl-dpipe-header](#id128)

#### dpipe-header-name

#### dpipe-header-id

#### dpipe-header-global

#### dpipe-header-fields

### [dl-dpipe-header-fields](#id129)

#### dpipe-field

### [dl-dpipe-field](#id130)

#### dpipe-field-name

#### dpipe-field-id

#### dpipe-field-bitwidth

#### dpipe-field-mapping-type

### [dl-resource](#id131)

#### resource-name

#### resource-id

#### resource-size

#### resource-size-new

#### resource-size-valid

#### resource-size-min

#### resource-size-max

#### resource-size-gran

#### resource-unit

#### resource-occ

### [dl-resource-list](#id132)

#### resource

### [dl-param](#id133)

#### param-name

#### param-generic

#### param-type

### [dl-region-snapshots](#id134)

#### region-snapshot

### [dl-region-snapshot](#id135)

#### region-snapshot-id

### [dl-region-chunks](#id136)

#### region-chunk

### [dl-region-chunk](#id137)

#### region-chunk-data

#### region-chunk-addr

### [dl-fmsg](#id138)

#### fmsg-obj-nest-start

#### fmsg-pair-nest-start

#### fmsg-arr-nest-start

#### fmsg-nest-end

#### fmsg-obj-name

### [dl-health-reporter](#id139)

#### health-reporter-name

#### health-reporter-state

#### health-reporter-err-count

#### health-reporter-recover-count

#### health-reporter-graceful-period

#### health-reporter-auto-recover

#### health-reporter-dump-ts

#### health-reporter-dump-ts-ns

#### health-reporter-auto-dump

#### health-reporter-burst-period

### [dl-attr-stats](#id140)

#### stats-rx-packets (`u64`)

value:
:   0

#### stats-rx-bytes (`u64`)

#### stats-rx-dropped (`u64`)

### [dl-trap-metadata](#id141)

#### trap-metadata-type-in-port (`flag`)

value:
:   0

#### trap-metadata-type-fa-cookie (`flag`)

### [dl-linecard-supported-types](#id142)

#### linecard-type

### [dl-selftest-id](#id143)

#### flash (`flag`)

### [dl-rate-tc-bws](#id144)

#### index (`u8`)

#### bw (`u32`)

doc:
:   Specifies the bandwidth share assigned to the Traffic Class. The bandwidth for the traffic class is determined in proportion to the sum of the shares of all configured classes.
