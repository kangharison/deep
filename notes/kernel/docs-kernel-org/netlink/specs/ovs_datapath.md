# Familyovs_datapathnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/ovs_datapath.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `ovs_datapath` netlink specification](#id1)

## [Summary](#id2)

OVS datapath configuration over generic netlink.

## [Operations](#id3)

### [get](#id4)

Get / dump OVS data path configuration and state

value:
:   3

attribute-set:
:   [datapath](#ovs-datapath-attribute-set-datapath)

do:
:   **request**
    :   attributes:
        :   [`name`]

    **reply**
    :   attributes:
        :   [`name`, `upcall-pid`, `stats`, `megaflow-stats`, `user-features`, `masks-cache-size`, `per-cpu-pids`]

dump:
:   **request**
    :   attributes:
        :   [`name`]

    **reply**
    :   attributes:
        :   [`name`, `upcall-pid`, `stats`, `megaflow-stats`, `user-features`, `masks-cache-size`, `per-cpu-pids`]

### [new](#id5)

Create new OVS data path

value:
:   1

attribute-set:
:   [datapath](#ovs-datapath-attribute-set-datapath)

do:
:   **request**
    :   attributes:
        :   [`name`, `upcall-pid`, `user-features`]

### [del](#id6)

Delete existing OVS data path

value:
:   2

attribute-set:
:   [datapath](#ovs-datapath-attribute-set-datapath)

do:
:   **request**
    :   attributes:
        :   [`name`]

## [Multicast groups](#id7)

* ovs\_datapath

## [Definitions](#id8)

### [ovs-header](#id9)

type:
:   struct

members:
:   dp-ifindex (`u32`):

### [user-features](#id10)

type:
:   flags

name-prefix:
:   ovs-dp-f-

enum-name:
:   None

entries:
:   unaligned:
    :   Allow last Netlink attribute to be unaligned

    vport-pids:
    :   Allow datapath to associate multiple Netlink PIDs to each vport

    tc-recirc-sharing:
    :   Allow tc offload recirc sharing

    dispatch-upcall-per-cpu:
    :   Allow per-cpu dispatch of upcalls

### [ovs-dp-stats](#id11)

type:
:   struct

members:
:   n-hit (`u64`):


    n-missed (`u64`):


    n-lost (`u64`):


    n-flows (`u64`):

### [ovs-dp-megaflow-stats](#id12)

type:
:   struct

members:
:   n-mask-hit (`u64`):


    n-masks (`u32`):


    padding (`u32`):


    n-cache-hit (`u64`):


    pad1 (`u64`):

## [Attribute sets](#id13)

### [datapath](#id14)

#### name (`string`)

#### upcall-pid (`u32`)

doc:
:   upcall pid

#### stats (`binary`)

struct:
:   [ovs-dp-stats](#ovs-datapath-definition-ovs-dp-stats)

#### megaflow-stats (`binary`)

struct:
:   [ovs-dp-megaflow-stats](#ovs-datapath-definition-ovs-dp-megaflow-stats)

#### user-features (`u32`)

enum:
:   [user-features](#ovs-datapath-definition-user-features)

enum-as-flags:
:   True

#### pad (`unused`)

#### masks-cache-size (`u32`)

#### per-cpu-pids (`binary`)

sub-type:
:   u32

#### ifindex (`u32`)
