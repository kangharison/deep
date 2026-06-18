# Familynfsdnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/nfsd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `nfsd` netlink specification](#id1)

## [Summary](#id2)

NFSD configuration over generic netlink.

## [Operations](#id3)

### [rpc-status-get](#id4)

dump pending nfsd rpc

attribute-set:
:   [rpc-status](#nfsd-attribute-set-rpc-status)

dump:
:   **reply**
    :   attributes:
        :   [`xid`, `flags`, `prog`, `version`, `proc`, `service-time`, `saddr4`, `daddr4`, `saddr6`, `daddr6`, `sport`, `dport`, `compound-ops`]

### [threads-set](#id5)

set the maximum number of running threads

attribute-set:
:   [server](#nfsd-attribute-set-server)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`threads`, `gracetime`, `leasetime`, `scope`, `min-threads`, `fh-key`]

### [threads-get](#id6)

get the maximum number of running threads

attribute-set:
:   [server](#nfsd-attribute-set-server)

do:
:   **reply**
    :   attributes:
        :   [`threads`, `gracetime`, `leasetime`, `scope`, `min-threads`]

### [version-set](#id7)

set nfs enabled versions

attribute-set:
:   [server-proto](#nfsd-attribute-set-server-proto)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`version`]

### [version-get](#id8)

get nfs enabled versions

attribute-set:
:   [server-proto](#nfsd-attribute-set-server-proto)

do:
:   **reply**
    :   attributes:
        :   [`version`]

### [listener-set](#id9)

set nfs running sockets

attribute-set:
:   [server-sock](#nfsd-attribute-set-server-sock)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr`]

### [listener-get](#id10)

get nfs running listeners

attribute-set:
:   [server-sock](#nfsd-attribute-set-server-sock)

do:
:   **reply**
    :   attributes:
        :   [`addr`]

### [pool-mode-set](#id11)

set the current server pool-mode

attribute-set:
:   [pool-mode](#nfsd-attribute-set-pool-mode)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`mode`]

### [pool-mode-get](#id12)

get info about server pool-mode

attribute-set:
:   [pool-mode](#nfsd-attribute-set-pool-mode)

do:
:   **reply**
    :   attributes:
        :   [`mode`, `npools`]

## [Attribute sets](#id13)

### [rpc-status](#id14)

#### xid (`u32`)

byte-order:
:   big-endian

#### flags (`u32`)

#### prog (`u32`)

#### version (`u8`)

#### proc (`u32`)

#### service-time (`s64`)

#### pad (`pad`)

#### saddr4 (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### daddr4 (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### saddr6 (`binary`)

display-hint:
:   ipv6

#### daddr6 (`binary`)

display-hint:
:   ipv6

#### sport (`u16`)

byte-order:
:   big-endian

#### dport (`u16`)

byte-order:
:   big-endian

#### compound-ops (`u32`)

multi-attr:
:   True

### [server](#id15)

#### threads (`u32`)

multi-attr:
:   True

#### gracetime (`u32`)

#### leasetime (`u32`)

#### scope (`string`)

#### min-threads (`u32`)

#### fh-key (`binary`)

### [version](#id16)

#### major (`u32`)

#### minor (`u32`)

#### enabled (`flag`)

### [server-proto](#id17)

#### version (`nest`)

nested-attributes:
:   [version](#nfsd-attribute-set-version)

multi-attr:
:   True

### [sock](#id18)

#### addr (`binary`)

#### transport-name (`string`)

### [server-sock](#id19)

#### addr (`nest`)

nested-attributes:
:   [sock](#nfsd-attribute-set-sock)

multi-attr:
:   True

### [pool-mode](#id20)

#### mode (`string`)

#### npools (`u32`)
