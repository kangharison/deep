# Familymptcp_pmnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/mptcp_pm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `mptcp_pm` netlink specification](#id10)

## [Summary](#id11)

Multipath TCP.

## [Operations](#id12)

### [unspec](#id13)

unused

value:
:   0

### [add-addr](#id14)

Add endpoint

attribute-set:
:   [endpoint](#mptcp-pm-attribute-set-endpoint)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`]

### [del-addr](#id15)

Delete endpoint

attribute-set:
:   [endpoint](#mptcp-pm-attribute-set-endpoint)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`]

### [get-addr](#id16)

Get endpoint information

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

do:
:   **request**
    :   attributes:
        :   [`addr`, `token`]

    **reply**
    :   attributes:
        :   [`addr`]

dump:
:   **reply**
    :   attributes:
        :   [`addr`]

### [flush-addrs](#id17)

Flush addresses

attribute-set:
:   [endpoint](#mptcp-pm-attribute-set-endpoint)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`]

### [set-limits](#id18)

Set protocol limits

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`rcv-add-addrs`, `subflows`]

### [get-limits](#id19)

Get protocol limits

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

do:
:   **request**
    :   attributes:
        :   [`rcv-add-addrs`, `subflows`]

    **reply**
    :   attributes:
        :   [`rcv-add-addrs`, `subflows`]

### [set-flags](#id20)

Change endpoint flags

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`, `token`, `addr-remote`]

### [announce](#id21)

Announce new address

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`, `token`]

### [remove](#id22)

Announce removal

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`token`, `loc-id`]

### [subflow-create](#id23)

Create subflow

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`, `token`, `addr-remote`]

### [subflow-destroy](#id24)

Destroy subflow

attribute-set:
:   [attr](#mptcp-pm-attribute-set-attr)

dont-validate:
:   [‘strict’]

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`, `token`, `addr-remote`]

## [Definitions](#id25)

### [event-type](#id26)

type:
:   enum

enum-name:
:   mptcp-event-type

doc:
:   Netlink MPTCP event types

name-prefix:
:   mptcp-event-

entries:
:   unspec:
    :   unused event

    created:
    :   A new MPTCP connection has been created. It is the good time to allocate memory and send ADD\_ADDR if needed. Depending on the traffic-patterns it can take a long time until the MPTCP\_EVENT\_ESTABLISHED is sent. Attributes: token, family, saddr4 | saddr6, daddr4 | daddr6, sport, dport, [server-side], [flags].

    established:
    :   A MPTCP connection is established (can start new subflows). Attributes: token, family, saddr4 | saddr6, daddr4 | daddr6, sport, dport, [server-side], [flags].

    closed:
    :   A MPTCP connection has stopped. Attribute: token.

    announced:
    :   A new address has been announced by the peer. Attributes: token, rem\_id, family, daddr4 | daddr6 [, dport].

    removed:
    :   An address has been lost by the peer. Attributes: token, rem\_id.

    sub-established:
    :   A new subflow has been established. ‘error’ should not be set. Attributes: token, family, loc\_id, rem\_id, saddr4 | saddr6, daddr4 | daddr6, sport, dport, backup, if-idx [, error].

    sub-closed:
    :   A subflow has been closed. An error (copy of sk\_err) could be set if an error has been detected for this subflow. Attributes: token, family, loc\_id, rem\_id, saddr4 | saddr6, daddr4 | daddr6, sport, dport, backup, if-idx [, error].

    sub-priority:
    :   The priority of a subflow has changed. ‘error’ should not be set. Attributes: token, family, loc\_id, rem\_id, saddr4 | saddr6, daddr4 | daddr6, sport, dport, backup, if-idx [, error].

    listener-created:
    :   A new PM listener is created. Attributes: family, sport, saddr4 | saddr6.

    listener-closed:
    :   A PM listener is closed. Attributes: family, sport, saddr4 | saddr6.

## [Attribute sets](#id27)

### [address](#id28)

#### unspec (`unused`)

value:
:   0

#### family (`u16`)

#### id (`u8`)

#### addr4 (`u32`)

byte-order:
:   big-endian

#### addr6 (`binary`)

#### port (`u16`)

#### flags (`u32`)

#### if-idx (`s32`)

### [subflow-attribute](#id29)

#### unspec (`unused`)

value:
:   0

#### token-rem (`u32`)

#### token-loc (`u32`)

#### relwrite-seq (`u32`)

#### map-seq (`u64`)

#### map-sfseq (`u32`)

#### ssn-offset (`u32`)

#### map-datalen (`u16`)

#### flags (`u32`)

#### id-rem (`u8`)

#### id-loc (`u8`)

#### pad (`pad`)

### [endpoint](#id30)

#### addr (`nest`)

nested-attributes:
:   [address](#mptcp-pm-attribute-set-address)

### [attr](#id31)

#### unspec (`unused`)

value:
:   0

#### addr (`nest`)

nested-attributes:
:   [address](#mptcp-pm-attribute-set-address)

#### rcv-add-addrs (`u32`)

#### subflows (`u32`)

#### token (`u32`)

#### loc-id (`u8`)

#### addr-remote (`nest`)

nested-attributes:
:   [address](#mptcp-pm-attribute-set-address)

### [event-attr](#id32)

#### unspec (`unused`)

value:
:   0

#### token (`u32`)

#### family (`u16`)

#### loc-id (`u8`)

#### rem-id (`u8`)

#### saddr4 (`u32`)

byte-order:
:   big-endian

#### saddr6 (`binary`)

#### daddr4 (`u32`)

byte-order:
:   big-endian

#### daddr6 (`binary`)

#### sport (`u16`)

byte-order:
:   big-endian

#### dport (`u16`)

byte-order:
:   big-endian

#### backup (`u8`)

#### error (`u8`)

#### flags (`u16`)

#### timeout (`u32`)

#### if-idx (`s32`)

#### reset-reason (`u32`)

#### reset-flags (`u32`)

#### server-side (`u8`)

doc:
:   Deprecated: use ‘flags’
