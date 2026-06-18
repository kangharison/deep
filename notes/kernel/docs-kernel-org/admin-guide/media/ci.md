# 5.2.Digital TV Conditional Access Interface

> 출처(원문): https://docs.kernel.org/admin-guide/media/ci.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.2. Digital TV Conditional Access Interface

Note

This documentation is outdated.

This document describes the usage of the high level CI API as
in accordance to the Linux DVB API. This is a not a documentation for the,
existing low level CI API.

Note

For the Twinhan/Twinhan clones, the dst\_ca module handles the CI
hardware handling. This module is loaded automatically if a CI
(Common Interface, that holds the CAM (Conditional Access Module)
is detected.

## 5.2.1. ca\_zap

A userspace application, like `ca_zap` is required to handle encrypted
MPEG-TS streams.

The `ca_zap` userland application is in charge of sending the
descrambling related information to the Conditional Access Module (CAM).

This application requires the following to function properly as of now.

1. Tune to a valid channel, with szap.

> eg: $ szap -c channels.conf -r “TMC” -x

2. a channels.conf containing a valid PMT PID

> eg: TMC:11996:h:0:27500:278:512:650:321
>
> here 278 is a valid PMT PID. the rest of the values are the
> same ones that szap uses.

3. after running a szap, you have to run ca\_zap, for the
   descrambler to function,

> eg: $ ca\_zap channels.conf “TMC”

4. Hopefully enjoy your favourite subscribed channel as you do with
   a FTA card.

Note

Currently ca\_zap, and dst\_test, both are meant for demonstration
purposes only, they can become full fledged applications if necessary.

## 5.2.2. Cards that fall in this category

At present the cards that fall in this category are the Twinhan and its
clones, these cards are available as VVMER, Tomato, Hercules, Orange and
so on.

## 5.2.3. CI modules that are supported

The CI module support is largely dependent upon the firmware on the cards
Some cards do support almost all of the available CI modules. There is
nothing much that can be done in order to make additional CI modules
working with these cards.

Modules that have been tested by this driver at present are

1. Irdeto 1 and 2 from SCM
2. Viaccess from SCM
3. Dragoncam
