# Familyovpnnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/ovpn.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `ovpn` netlink specification](#id29)

## [Summary](#id30)

Netlink protocol to control OpenVPN network devices

## [Operations](#id31)

### [peer-new](#id32)

Add a remote peer

attribute-set:
:   [ovpn-peer-new-input](#ovpn-attribute-set-ovpn-peer-new-input)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `peer`]

### [peer-set](#id33)

modify a remote peer

attribute-set:
:   [ovpn-peer-set-input](#ovpn-attribute-set-ovpn-peer-set-input)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `peer`]

### [peer-get](#id34)

Retrieve data about existing remote peers (or a specific one)

attribute-set:
:   [ovpn](#ovpn-attribute-set-ovpn)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `peer`]

    **reply**
    :   attributes:
        :   [`peer`]

dump:
:   **request**
    :   attributes:
        :   [`ifindex`]

    **reply**
    :   attributes:
        :   [`peer`]

### [peer-del](#id35)

Delete existing remote peer

attribute-set:
:   [ovpn-peer-del-input](#ovpn-attribute-set-ovpn-peer-del-input)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `peer`]

### [peer-del-ntf](#id36)

Notification about a peer being deleted

notify:
:   peer-get

mcgrp:
:   peers

### [key-new](#id37)

Add a cipher key for a specific peer

attribute-set:
:   [ovpn](#ovpn-attribute-set-ovpn)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `keyconf`]

### [key-get](#id38)

Retrieve non-sensitive data about peer key and cipher

attribute-set:
:   [ovpn-keyconf-get](#ovpn-attribute-set-ovpn-keyconf-get)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `keyconf`]

    **reply**
    :   attributes:
        :   [`keyconf`]

### [key-swap](#id39)

Swap primary and secondary session keys for a specific peer

attribute-set:
:   [ovpn-keyconf-swap-input](#ovpn-attribute-set-ovpn-keyconf-swap-input)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `keyconf`]

### [key-swap-ntf](#id40)

Notification about key having exhausted its IV space and requiring renegotiation

notify:
:   key-get

mcgrp:
:   peers

### [key-del](#id41)

Delete cipher key for a specific peer

attribute-set:
:   [ovpn-keyconf-del-input](#ovpn-attribute-set-ovpn-keyconf-del-input)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   ovpn-nl-pre-doit

    **post**
    :   ovpn-nl-post-doit

    **request**
    :   attributes:
        :   [`ifindex`, `keyconf`]

### [peer-float-ntf](#id42)

Notification about a peer floating (changing its remote UDP endpoint)

notify:
:   peer-get

mcgrp:
:   peers

## [Multicast groups](#id43)

* peers

## [Definitions](#id44)

### [nonce-tail-size](#id45)

type:
:   const

value:
:   8

### [cipher-alg](#id46)

type:
:   enum

entries:
:   * `none`
    * `aes-gcm`
    * `chacha20-poly1305`

### [del-peer-reason](#id47)

type:
:   enum

entries:
:   * `teardown`
    * `userspace`
    * `expired`
    * `transport-error`
    * `transport-disconnect`

### [key-slot](#id48)

type:
:   enum

entries:
:   * `primary`
    * `secondary`

## [Attribute sets](#id49)

### [peer](#id50)

#### id (`u32`)

doc:
:   The unique ID of the peer in the device context. To be used to identify peers during operations for a specific device. Also used to match packets received from this peer.

#### remote-ipv4 (`u32`)

doc:
:   The remote IPv4 address of the peer

byte-order:
:   big-endian

display-hint:
:   ipv4

#### remote-ipv6 (`binary`)

doc:
:   The remote IPv6 address of the peer

display-hint:
:   ipv6

#### remote-ipv6-scope-id (`u32`)

doc:
:   The scope id of the remote IPv6 address of the peer (RFC2553)

#### remote-port (`u16`)

doc:
:   The remote port of the peer

byte-order:
:   big-endian

#### socket (`u32`)

doc:
:   The socket to be used to communicate with the peer

#### socket-netnsid (`s32`)

doc:
:   The ID of the netns the socket assigned to this peer lives in

#### vpn-ipv4 (`u32`)

doc:
:   The IPv4 address assigned to the peer by the server

byte-order:
:   big-endian

display-hint:
:   ipv4

#### vpn-ipv6 (`binary`)

doc:
:   The IPv6 address assigned to the peer by the server

display-hint:
:   ipv6

#### local-ipv4 (`u32`)

doc:
:   The local IPv4 to be used to send packets to the peer (UDP only)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### local-ipv6 (`binary`)

doc:
:   The local IPv6 to be used to send packets to the peer (UDP only)

display-hint:
:   ipv6

#### local-port (`u16`)

doc:
:   The local port to be used to send packets to the peer (UDP only)

byte-order:
:   big-endian

#### keepalive-interval (`u32`)

doc:
:   The number of seconds after which a keep alive message is sent to the peer

#### keepalive-timeout (`u32`)

doc:
:   The number of seconds from the last activity after which the peer is assumed dead

#### del-reason (`u32`)

doc:
:   The reason why a peer was deleted

enum:
:   [del-peer-reason](#ovpn-definition-del-peer-reason)

#### vpn-rx-bytes (`uint`)

doc:
:   Number of bytes received over the tunnel

#### vpn-tx-bytes (`uint`)

doc:
:   Number of bytes transmitted over the tunnel

#### vpn-rx-packets (`uint`)

doc:
:   Number of packets received over the tunnel

#### vpn-tx-packets (`uint`)

doc:
:   Number of packets transmitted over the tunnel

#### link-rx-bytes (`uint`)

doc:
:   Number of bytes received at the transport level

#### link-tx-bytes (`uint`)

doc:
:   Number of bytes transmitted at the transport level

#### link-rx-packets (`uint`)

doc:
:   Number of packets received at the transport level

#### link-tx-packets (`uint`)

doc:
:   Number of packets transmitted at the transport level

#### tx-id (`u32`)

doc:
:   The ID value used when transmitting packets to this peer. This way outgoing packets can have a different ID than incoming ones. Useful in multipeer-to-multipeer connections, where each peer will advertise the tx-id to be used on the link.

### [peer-new-input](#id51)

#### id

#### remote-ipv4

#### remote-ipv6

#### remote-ipv6-scope-id

#### remote-port

#### socket

#### vpn-ipv4

#### vpn-ipv6

#### local-ipv4

#### local-ipv6

#### keepalive-interval

#### keepalive-timeout

#### tx-id

### [peer-set-input](#id52)

#### id

#### remote-ipv4

#### remote-ipv6

#### remote-ipv6-scope-id

#### remote-port

#### vpn-ipv4

#### vpn-ipv6

#### local-ipv4

#### local-ipv6

#### keepalive-interval

#### keepalive-timeout

#### tx-id

### [peer-del-input](#id53)

#### id

### [keyconf](#id54)

#### peer-id (`u32`)

doc:
:   The unique ID of the peer in the device context. To be used to identify peers during key operations

#### slot (`u32`)

doc:
:   The slot where the key should be stored

enum:
:   [key-slot](#ovpn-definition-key-slot)

#### key-id (`u32`)

doc:
:   The unique ID of the key in the peer context. Used to fetch the correct key upon decryption

#### cipher-alg (`u32`)

doc:
:   The cipher to be used when communicating with the peer

enum:
:   [cipher-alg](#ovpn-definition-cipher-alg)

#### encrypt-dir (`nest`)

doc:
:   Key material for encrypt direction

nested-attributes:
:   [keydir](#ovpn-attribute-set-keydir)

#### decrypt-dir (`nest`)

doc:
:   Key material for decrypt direction

nested-attributes:
:   [keydir](#ovpn-attribute-set-keydir)

### [keydir](#id55)

#### cipher-key (`binary`)

doc:
:   The actual key to be used by the cipher

#### nonce-tail (`binary`)

doc:
:   Random nonce to be concatenated to the packet ID, in order to obtain the actual cipher IV

### [keyconf-get](#id56)

#### peer-id

#### slot

#### key-id

#### cipher-alg

### [keyconf-swap-input](#id57)

#### peer-id

### [keyconf-del-input](#id58)

#### peer-id

#### slot

### [ovpn](#id59)

#### ifindex (`u32`)

doc:
:   Index of the ovpn interface to operate on

#### peer (`nest`)

doc:
:   The peer object containing the attributed of interest for the specific operation

nested-attributes:
:   [peer](#ovpn-attribute-set-peer)

#### keyconf (`nest`)

doc:
:   Peer specific cipher configuration

nested-attributes:
:   [keyconf](#ovpn-attribute-set-keyconf)

### [ovpn-peer-new-input](#id60)

#### ifindex

#### peer

nested-attributes:
:   [peer-new-input](#ovpn-attribute-set-peer-new-input)

### [ovpn-peer-set-input](#id61)

#### ifindex

#### peer

nested-attributes:
:   [peer-set-input](#ovpn-attribute-set-peer-set-input)

### [ovpn-peer-del-input](#id62)

#### ifindex

#### peer

nested-attributes:
:   [peer-del-input](#ovpn-attribute-set-peer-del-input)

### [ovpn-keyconf-get](#id63)

#### ifindex

#### keyconf

nested-attributes:
:   [keyconf-get](#ovpn-attribute-set-keyconf-get)

### [ovpn-keyconf-swap-input](#id64)

#### ifindex

#### keyconf

nested-attributes:
:   [keyconf-swap-input](#ovpn-attribute-set-keyconf-swap-input)

### [ovpn-keyconf-del-input](#id65)

#### ifindex

#### keyconf

nested-attributes:
:   [keyconf-del-input](#ovpn-attribute-set-keyconf-del-input)
