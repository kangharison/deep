# Familydev-energymodelnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/dev-energymodel.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `dev-energymodel` netlink specification](#id4)

## [Summary](#id5)

Energy model netlink interface to notify its changes.

## [Operations](#id6)

### [get-perf-domains](#id7)

Get the list of information for all performance domains.

attribute-set:
:   [perf-domain](#dev-energymodel-attribute-set-perf-domain)

do:
:   **request**
    :   attributes:
        :   [`perf-domain-id`]

    **reply**
    :   attributes:
        :   [`pad`, `perf-domain-id`, `flags`, `cpus`]

dump:
:   **reply**
    :   attributes:
        :   [`pad`, `perf-domain-id`, `flags`, `cpus`]

### [get-perf-table](#id8)

Get the energy model table of a performance domain.

attribute-set:
:   [perf-table](#dev-energymodel-attribute-set-perf-table)

do:
:   **request**
    :   attributes:
        :   [`perf-domain-id`]

    **reply**
    :   attributes:
        :   [`perf-domain-id`, `perf-state`]

### [perf-domain-created](#id9)

A performance domain is created.

notify:
:   get-perf-table

mcgrp:
:   event

### [perf-domain-updated](#id10)

A performance domain is updated.

notify:
:   get-perf-table

mcgrp:
:   event

### [perf-domain-deleted](#id11)

A performance domain is deleted.

attribute-set:
:   [perf-table](#dev-energymodel-attribute-set-perf-table)

mcgrp:
:   event

event:
:   attributes:
    :   [`perf-domain-id`]

## [Multicast groups](#id12)

* event

## [Definitions](#id13)

### [perf-state-flags](#id14)

type:
:   flags

entries:
:   perf-state-inefficient:
    :   The performance state is inefficient. There is in this perf-domain, another performance state with a higher frequency but a lower or equal power cost.

### [perf-domain-flags](#id15)

type:
:   flags

entries:
:   perf-domain-microwatts:
    :   The power values are in micro-Watts or some other scale.

    perf-domain-skip-inefficiencies:
    :   Skip inefficient states when estimating energy consumption.

    perf-domain-artificial:
    :   The power values are artificial and might be created by platform missing real power information.

## [Attribute sets](#id16)

### [perf-domain](#id17)

Information on a single performance domains.

#### pad (`pad`)

#### perf-domain-id (`u32`)

doc:
:   A unique ID number for each performance domain.

#### flags (`u64`)

doc:
:   Bitmask of performance domain flags.

enum:
:   [perf-domain-flags](#dev-energymodel-definition-perf-domain-flags)

#### cpus (`u64`)

multi-attr:
:   True

doc:
:   CPUs that belong to this performance domain.

### [perf-table](#id18)

Performance states table.

#### perf-domain-id (`u32`)

doc:
:   A unique ID number for each performance domain.

#### perf-state (`nest`)

nested-attributes:
:   [perf-state](#dev-energymodel-attribute-set-perf-state)

multi-attr:
:   True

### [perf-state](#id19)

Performance state of a performance domain.

#### pad (`pad`)

#### performance (`u64`)

doc:
:   CPU performance (capacity) at a given frequency.

#### frequency (`u64`)

doc:
:   The frequency in KHz, for consistency with CPUFreq.

#### power (`u64`)

doc:
:   The power consumed at this level (by 1 CPU or by a registered device). It can be a total power: static and dynamic.

#### cost (`u64`)

doc:
:   The cost coefficient associated with this level, used during energy calculation. Equal to: power \* max\_frequency / frequency.

#### flags (`u64`)

doc:
:   Bitmask of performance state flags.

enum:
:   [perf-state-flags](#dev-energymodel-definition-perf-state-flags)
