# Familywireguardnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/wireguard.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `wireguard` netlink specification](#id6)

## [Summary](#id7)

**Netlink protocol to control WireGuard network devices.**

The below enums and macros are for interfacing with WireGuard, using generic
netlink, with family `WG_GENL_NAME` and version `WG_GENL_VERSION`. It
defines two commands: get and set. Note that while they share many common
attributes, these two commands actually accept a slightly different set of
inputs and outputs. These differences are noted under the individual
attributes.

## [Operations](#id8)

### [get-device](#id9)

#### Retrieve WireGuard device

The command should be called with one but not both of:

* `WGDEVICE_A_IFINDEX`
* `WGDEVICE_A_IFNAME`

The kernel will then return several messages (`NLM_F_MULTI`). It is
possible that all of the allowed IPs of a single peer will not fit
within a single netlink message. In that case, the same peer will be
written in the following message, except it will only contain
`WGPEER_A_PUBLIC_KEY` and `WGPEER_A_ALLOWEDIPS`. This may occur
several times in a row for the same peer. It is then up to the receiver
to coalesce adjacent peers. Likewise, it is possible that all peers will
not fit within a single message. So, subsequent peers will be sent in
following messages, except those will only contain `WGDEVICE_A_IFNAME`
and `WGDEVICE_A_PEERS`. It is then up to the receiver to coalesce
these messages to form the complete list of peers.

Since this is an `NLA_F_DUMP` command, the final message will always
be `NLMSG_DONE`, even if an error occurs. However, this `NLMSG_DONE`
message contains an integer error code. It is either zero or a negative
error code corresponding to the errno.

value:
:   0

attribute-set:
:   [wgdevice](#wireguard-attribute-set-wgdevice)

flags:
:   [`uns-admin-perm`]

dump:
:   **pre**
    :   wg-get-device-start

    **post**
    :   wg-get-device-done

    **request**
    :   attributes:
        :   [`ifindex`, `ifname`]

    **reply**
    :   attributes:
        :   [`ifindex`, `ifname`, `private-key`, `public-key`, `flags`, `listen-port`, `fwmark`, `peers`]

### [set-device](#id10)

#### Set WireGuard device

This command should be called with a wgdevice set, containing one but
not both of `WGDEVICE_A_IFINDEX` and `WGDEVICE_A_IFNAME`.

It is possible that the amount of configuration data exceeds that of the
maximum message length accepted by the kernel. In that case, several
messages should be sent one after another, with each successive one
filling in information not contained in the prior. Note that if
`WGDEVICE_F_REPLACE_PEERS` is specified in the first message, it
probably should not be specified in fragments that come after, so that
the list of peers is only cleared the first time but appended after.
Likewise for peers, if `WGPEER_F_REPLACE_ALLOWEDIPS` is specified in
the first message of a peer, it likely should not be specified in
subsequent fragments.

If an error occurs, `NLMSG_ERROR` will reply containing an errno.

value:
:   1

attribute-set:
:   [wgdevice](#wireguard-attribute-set-wgdevice)

flags:
:   [`uns-admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`ifindex`, `ifname`, `private-key`, `public-key`, `flags`, `listen-port`, `fwmark`, `peers`]

## [Definitions](#id11)

### [key-len](#id12)

name-prefix:
:   wg-

type:
:   const

value:
:   32

### [--kernel-timespec](#id13)

type:
:   struct

header:
:   linux/time\_types.h

members:
:   sec (`u64`):
    :   Number of seconds, since UNIX epoch.

    nsec (`u64`):
    :   Number of nanoseconds, after the second began.

### [wgdevice-flags](#id14)

name-prefix:
:   wgdevice-f-

enum-name:
:   wgdevice-flag

type:
:   flags

entries:
:   * `replace-peers`

### [wgpeer-flags](#id15)

name-prefix:
:   wgpeer-f-

enum-name:
:   wgpeer-flag

type:
:   flags

entries:
:   * `remove-me`
    * `replace-allowedips`
    * `update-only`

### [wgallowedip-flags](#id16)

name-prefix:
:   wgallowedip-f-

enum-name:
:   wgallowedip-flag

type:
:   flags

entries:
:   * `remove-me`

## [Attribute sets](#id17)

### [wgdevice](#id18)

#### unspec (`unused`)

value:
:   0

#### ifindex (`u32`)

#### ifname (`string`)

#### private-key (`binary`)

doc:
:   Set to all zeros to remove.

display-hint:
:   hex

#### public-key (`binary`)

display-hint:
:   hex

#### flags (`u32`)

doc:
:   `0` or `WGDEVICE_F_REPLACE_PEERS` if all current peers should be removed prior to adding the list below.

enum:
:   [wgdevice-flags](#wireguard-definition-wgdevice-flags)

#### listen-port (`u16`)

doc:
:   Set as `0` to choose randomly.

#### fwmark (`u32`)

doc:
:   Set as `0` to disable.

#### peers (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [wgpeer](#wireguard-attribute-set-wgpeer)

doc:
:   The index/type parameter is unused on `SET_DEVICE` operations and is zero on `GET_DEVICE` operations.

### [wgpeer](#id19)

#### unspec (`unused`)

value:
:   0

#### public-key (`binary`)

display-hint:
:   hex

#### preshared-key (`binary`)

doc:
:   Set as all zeros to remove.

display-hint:
:   hex

#### flags (`u32`)

doc:
:   `0` and/or `WGPEER_F_REMOVE_ME` if the specified peer should not exist at the end of the operation, rather than added/updated and/or `WGPEER_F_REPLACE_ALLOWEDIPS` if all current allowed IPs of this peer should be removed prior to adding the list below and/or `WGPEER_F_UPDATE_ONLY` if the peer should only be set if it already exists.

enum:
:   [wgpeer-flags](#wireguard-definition-wgpeer-flags)

#### endpoint (`binary`)

doc:
:   `struct sockaddr_in` or `struct sockaddr_in6`

#### persistent-keepalive-interval (`u16`)

doc:
:   Set as `0` to disable.

#### last-handshake-time (`binary`)

struct:
:   [--kernel-timespec](#wireguard-definition-kernel-timespec)

#### rx-bytes (`u64`)

#### tx-bytes (`u64`)

#### allowedips (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [wgallowedip](#wireguard-attribute-set-wgallowedip)

doc:
:   The index/type parameter is unused on `SET_DEVICE` operations and is zero on `GET_DEVICE` operations.

#### protocol-version (`u32`)

doc:
:   Should not be set or used at all by most users of this API, as the most recent protocol will be used when this is unset. Otherwise, must be set to `1`.

### [wgallowedip](#id20)

#### unspec (`unused`)

value:
:   0

#### family (`u16`)

doc:
:   IP family, either `AF_INET` or `AF_INET6`.

#### ipaddr (`binary`)

doc:
:   Either `struct in_addr` or `struct in6_addr`.

display-hint:
:   ipv4-or-v6

#### cidr-mask (`u8`)

#### flags (`u32`)

doc:
:   `WGALLOWEDIP_F_REMOVE_ME` if the specified IP should be removed; otherwise, this IP will be added if it is not already present.

enum:
:   [wgallowedip-flags](#wireguard-definition-wgallowedip-flags)
