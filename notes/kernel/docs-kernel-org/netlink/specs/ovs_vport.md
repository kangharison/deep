# Familyovs_vportnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/ovs_vport.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `ovs_vport` netlink specification](#id1)

## [Summary](#id2)

OVS vport configuration over generic netlink.

## [Operations](#id3)

### [new](#id4)

Create a new OVS vport

attribute-set:
:   [vport](#ovs-vport-attribute-set-vport)

do:
:   **request**
    :   attributes:
        :   [`name`, `type`, `upcall-pid`, `ifindex`, `options`]

### [del](#id5)

Delete existing OVS vport from a data path

attribute-set:
:   [vport](#ovs-vport-attribute-set-vport)

do:
:   **request**
    :   attributes:
        :   [`port-no`, `type`, `name`]

### [get](#id6)

Get / dump OVS vport configuration and state

attribute-set:
:   [vport](#ovs-vport-attribute-set-vport)

do:
:   **request**
    :   attributes:
        :   [`name`]

    **reply**
    :   attributes:
        :   [`port-no`, `type`, `name`, `upcall-pid`, `stats`, `ifindex`, `netnsid`, `upcall-stats`]

dump:
:   **request**
    :   attributes:
        :   [`name`]

    **reply**
    :   attributes:
        :   [`port-no`, `type`, `name`, `upcall-pid`, `stats`, `ifindex`, `netnsid`, `upcall-stats`]

## [Multicast groups](#id7)

* ovs\_vport

## [Definitions](#id8)

### [ovs-header](#id9)

type:
:   struct

members:
:   dp-ifindex (`u32`):

### [vport-type](#id10)

type:
:   enum

enum-name:
:   ovs-vport-type

name-prefix:
:   ovs-vport-type-

entries:
:   * `unspec`
    * `netdev`
    * `internal`
    * `gre`
    * `vxlan`
    * `geneve`

### [ovs-vport-stats](#id11)

type:
:   struct

members:
:   rx-packets (`u64`):


    tx-packets (`u64`):


    rx-bytes (`u64`):


    tx-bytes (`u64`):


    rx-errors (`u64`):


    tx-errors (`u64`):


    rx-dropped (`u64`):


    tx-dropped (`u64`):

## [Attribute sets](#id12)

### [vport-options](#id13)

#### dst-port (`u32`)

#### extension (`u32`)

### [upcall-stats](#id14)

#### success (`u64`)

value:
:   0

#### fail (`u64`)

### [vport](#id15)

#### unspec (`unused`)

value:
:   0

#### port-no (`u32`)

#### type (`u32`)

enum:
:   [vport-type](#ovs-vport-definition-vport-type)

#### name (`string`)

#### options (`nest`)

nested-attributes:
:   [vport-options](#ovs-vport-attribute-set-vport-options)

#### upcall-pid (`binary`)

sub-type:
:   u32

#### stats (`binary`)

struct:
:   [ovs-vport-stats](#ovs-vport-definition-ovs-vport-stats)

#### pad (`unused`)

#### ifindex (`u32`)

#### netnsid (`u32`)

#### upcall-stats (`nest`)

nested-attributes:
:   [upcall-stats](#ovs-vport-attribute-set-upcall-stats)
