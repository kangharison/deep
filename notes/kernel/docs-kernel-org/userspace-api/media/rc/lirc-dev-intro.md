# 6.1.Introduction

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-dev-intro.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1. Introduction

LIRC stands for Linux Infrared Remote Control. The LIRC device interface is
a bi-directional interface for transporting raw IR and decoded scancodes
data between userspace and kernelspace. Fundamentally, it is just a chardev
(/dev/lircX, for X = 0, 1, 2, ...), with a number of standard `struct
file_operations` defined on it. With respect to transporting raw IR and
decoded scancodes to and fro, the essential fops are read, write and ioctl.

It is also possible to attach a BPF program to a LIRC device for decoding
raw IR into scancodes.

Example dmesg output upon a driver registering w/LIRC:

```
$ dmesg |grep lirc_dev
rc rc0: lirc_dev: driver mceusb registered at minor = 0, raw IR receiver, raw IR transmitter
```

What you should see for a chardev:

```
$ ls -l /dev/lirc*
crw-rw---- 1 root root 248, 0 Jul 2 22:20 /dev/lirc0
```

Note that the package [v4l-utils](https://git.linuxtv.org/v4l-utils.git/)
contains tools for working with LIRC devices:

> * ir-ctl: can receive raw IR and transmit IR, as well as query LIRC
>   device features.
> * ir-keytable: can load keymaps; allows you to set IR kernel protocols; load
>   BPF IR decoders and test IR decoding. Some BPF IR decoders are also
>   provided.

# 6.2. LIRC modes

LIRC supports some modes of receiving and sending IR codes, as shown
on the following table.

`LIRC_MODE_SCANCODE`

> This mode is for both sending and receiving IR.
>
> For transmitting (aka sending), create a [`struct lirc_scancode`](#c.lirc_scancode "lirc_scancode") with
> the desired scancode set in the `scancode` member, [`rc_proto`](#c.rc_proto "rc_proto")
> set to the [IR protocol](rc-protos.html#remote-controllers-protocols), and all other
> members set to 0. Write this `struct to` the lirc device.
>
> For receiving, you read [`struct lirc_scancode`](#c.lirc_scancode "lirc_scancode") from the LIRC device.
> The `scancode` field is set to the received scancode and the
> [IR protocol](rc-protos.html#remote-controllers-protocols) is set in
> [`rc_proto`](#c.rc_proto "rc_proto"). If the scancode maps to a valid key code, this is set
> in the `keycode` field, else it is set to `KEY_RESERVED`.
>
> The `flags` can have `LIRC_SCANCODE_FLAG_TOGGLE` set if the toggle
> bit is set in protocols that support it (e.g. rc-5 and rc-6), or
> `LIRC_SCANCODE_FLAG_REPEAT` for when a repeat is received for protocols
> that support it (e.g. nec).
>
> In the Sanyo and NEC protocol, if you hold a button on remote, rather than
> repeating the entire scancode, the remote sends a shorter message with
> no scancode, which just means button is held, a “repeat”. When this is
> received, the `LIRC_SCANCODE_FLAG_REPEAT` is set and the scancode and
> keycode is repeated.
>
> With nec, there is no way to distinguish “button hold” from “repeatedly
> pressing the same button”. The rc-5 and rc-6 protocols have a toggle bit.
> When a button is released and pressed again, the toggle bit is inverted.
> If the toggle bit is set, the `LIRC_SCANCODE_FLAG_TOGGLE` is set.
>
> The `timestamp` field is filled with the time nanoseconds
> (in `CLOCK_MONOTONIC`) when the scancode was decoded.

`LIRC_MODE_MODE2`

> The driver returns a sequence of pulse and space codes to userspace,
> as a series of u32 values.
>
> This mode is used only for IR receive.
>
> The upper 8 bits determine the packet type, and the lower 24 bits
> the payload. Use `LIRC_VALUE()` macro to get the payload, and
> the macro `LIRC_MODE2()` will give you the type, which
> is one of:
>
> `LIRC_MODE2_PULSE`
>
> > Signifies the presence of IR in microseconds, also known as *flash*.
>
> `LIRC_MODE2_SPACE`
>
> > Signifies absence of IR in microseconds, also known as *gap*.
>
> `LIRC_MODE2_FREQUENCY`
>
> > If measurement of the carrier frequency was enabled with
> > [ioctl LIRC\_SET\_MEASURE\_CARRIER\_MODE](lirc-set-measure-carrier-mode.html#lirc-set-measure-carrier-mode) then this packet gives you
> > the carrier frequency in Hertz.
>
> `LIRC_MODE2_TIMEOUT`
>
> > When the timeout set with [ioctl LIRC\_GET\_REC\_TIMEOUT and LIRC\_SET\_REC\_TIMEOUT](lirc-set-rec-timeout.html#lirc-set-rec-timeout) expires due
> > to no IR being detected, this packet will be sent, with the number
> > of microseconds with no IR.
>
> `LIRC_MODE2_OVERFLOW`
>
> > Signifies that the IR receiver encounter an overflow, and some IR
> > is missing. The IR data after this should be correct again. The
> > actual value is not important, but this is set to 0xffffff by the
> > kernel for compatibility with lircd.

`LIRC_MODE_PULSE`

> In pulse mode, a sequence of pulse/space integer values are written to the
> lirc device using [LIRC write()](lirc-write.html#lirc-write).
>
> The values are alternating pulse and space lengths, in microseconds. The
> first and last entry must be a pulse, so there must be an odd number
> of entries.
>
> This mode is used only for IR send.

# 6.3. Data types used by LIRC\_MODE\_SCANCODE

struct lirc\_scancode
:   decoded scancode with protocol for use with LIRC\_MODE\_SCANCODE

**Definition**:

```
struct lirc_scancode {
    __u64 timestamp;
    __u16 flags;
    __u16 rc_proto;
    __u32 keycode;
    __u64 scancode;
};
```

**Members**

`timestamp`
:   Timestamp in nanoseconds using CLOCK\_MONOTONIC when IR
    was decoded.

`flags`
:   should be 0 for transmit. When receiving scancodes,
    LIRC\_SCANCODE\_FLAG\_TOGGLE or LIRC\_SCANCODE\_FLAG\_REPEAT can be set
    depending on the protocol

`rc_proto`
:   see [`enum rc_proto`](#c.rc_proto "rc_proto")

`keycode`
:   the translated keycode. Set to 0 for transmit.

`scancode`
:   the scancode received or to be sent

enum rc\_proto
:   the Remote Controller protocol

**Constants**

`RC_PROTO_UNKNOWN`
:   Protocol not known

`RC_PROTO_OTHER`
:   Protocol known but proprietary

`RC_PROTO_RC5`
:   Philips RC5 protocol

`RC_PROTO_RC5X_20`
:   Philips RC5x 20 bit protocol

`RC_PROTO_RC5_SZ`
:   StreamZap variant of RC5

`RC_PROTO_JVC`
:   JVC protocol

`RC_PROTO_SONY12`
:   Sony 12 bit protocol

`RC_PROTO_SONY15`
:   Sony 15 bit protocol

`RC_PROTO_SONY20`
:   Sony 20 bit protocol

`RC_PROTO_NEC`
:   NEC protocol

`RC_PROTO_NECX`
:   Extended NEC protocol

`RC_PROTO_NEC32`
:   NEC 32 bit protocol

`RC_PROTO_SANYO`
:   Sanyo protocol

`RC_PROTO_MCIR2_KBD`
:   RC6-ish MCE keyboard

`RC_PROTO_MCIR2_MSE`
:   RC6-ish MCE mouse

`RC_PROTO_RC6_0`
:   Philips RC6-0-16 protocol

`RC_PROTO_RC6_6A_20`
:   Philips RC6-6A-20 protocol

`RC_PROTO_RC6_6A_24`
:   Philips RC6-6A-24 protocol

`RC_PROTO_RC6_6A_32`
:   Philips RC6-6A-32 protocol

`RC_PROTO_RC6_MCE`
:   MCE (Philips RC6-6A-32 subtype) protocol

`RC_PROTO_SHARP`
:   Sharp protocol

`RC_PROTO_XMP`
:   XMP protocol

`RC_PROTO_CEC`
:   CEC protocol

`RC_PROTO_IMON`
:   iMon Pad protocol

`RC_PROTO_RCMM12`
:   RC-MM protocol 12 bits

`RC_PROTO_RCMM24`
:   RC-MM protocol 24 bits

`RC_PROTO_RCMM32`
:   RC-MM protocol 32 bits

`RC_PROTO_XBOX_DVD`
:   Xbox DVD Movie Playback Kit protocol

`RC_PROTO_MAX`
:   Maximum value of [`enum rc_proto`](#c.rc_proto "rc_proto")

# 6.4. BPF based IR decoder

The kernel has support for decoding the most common
[IR protocols](rc-protos.html#remote-controllers-protocols), but there
are many protocols which are not supported. To support these, it is possible
to load an BPF program which does the decoding. This can only be done on
LIRC devices which support reading raw IR.

First, using the [bpf(2)](http://man7.org/linux/man-pages/man2/bpf.2.html) syscall with the `BPF_LOAD_PROG` argument,
program must be loaded of type `BPF_PROG_TYPE_LIRC_MODE2`. Once attached
to the LIRC device, this program will be called for each pulse, space or
timeout event on the LIRC device. The context for the BPF program is a
pointer to a unsigned int, which is a [LIRC\_MODE\_MODE2](#lirc-mode-mode2)
value. When the program has decoded the scancode, it can be submitted using
the BPF functions `bpf_rc_keydown()` or `bpf_rc_repeat()`. Mouse or pointer
movements can be reported using `bpf_rc_pointer_rel()`.

Once you have the file descriptor for the `BPF_PROG_TYPE_LIRC_MODE2` BPF
program, it can be attached to the LIRC device using the [bpf(2)](http://man7.org/linux/man-pages/man2/bpf.2.html) syscall.
The target must be the file descriptor for the LIRC device, and the
attach type must be `BPF_LIRC_MODE2`. No more than 64 BPF programs can be
attached to a single LIRC device at a time.
