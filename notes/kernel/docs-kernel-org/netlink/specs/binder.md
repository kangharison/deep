# Familybindernetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/binder.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `binder` netlink specification](#id2)

## [Summary](#id3)

Binder interface over generic netlink

## [Operations](#id4)

### [report](#id5)

A multicast event sent to userspace subscribers to notify them about
binder transaction failures. The generated report provides the full
details of the specific transaction that failed. The intention is for
programs to monitor these events and react to the failures as needed.

attribute-set:
:   [report](#binder-attribute-set-report)

mcgrp:
:   report

event:
:   attributes:
    :   [`error`, `context`, `from-pid`, `from-tid`, `to-pid`, `to-tid`, `is-reply`, `flags`, `code`, `data-size`]

## [Multicast groups](#id6)

* report

## [Attribute sets](#id7)

### [report](#id8)

Attributes included within a transaction failure report. The elements
correspond directly with the specific transaction that failed, along
with the error returned to the sender e.g. BR\_DEAD\_REPLY.

#### error (`u32`)

doc:
:   The `enum binder_driver_return_protocol` returned to the sender.

#### context (`string`)

doc:
:   The binder context where the transaction occurred.

#### from-pid (`u32`)

doc:
:   The PID of the sender process.

#### from-tid (`u32`)

doc:
:   The TID of the sender thread.

#### to-pid (`u32`)

doc:
:   The PID of the recipient process. This attribute may not be present if the target could not be determined.

#### to-tid (`u32`)

doc:
:   The TID of the recipient thread. This attribute may not be present if the target could not be determined.

#### is-reply (`flag`)

doc:
:   When present, indicates the failed transaction is a reply.

#### flags (`u32`)

doc:
:   The bitmask of `enum transaction_flags` from the transaction.

#### code (`u32`)

doc:
:   The application-defined code from the transaction.

#### data-size (`u32`)

doc:
:   The transaction payload size in bytes.
