# TTY

> 출처(원문): https://docs.kernel.org/driver-api/tty/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY

Teletypewriter (TTY) layer takes care of all those serial devices. Including
the virtual ones like pseudoterminal (PTY).

## TTY structures

There are several major TTY structures. Every TTY device in a system has a
corresponding [`struct tty_port`](tty_port.html#c.tty_port "tty_port"). These devices are maintained by a TTY driver
which is [`struct tty_driver`](tty_driver.html#c.tty_driver "tty_driver"). This structure describes the driver but also
contains a reference to operations which could be performed on the TTYs. It is
[`struct tty_operations`](tty_driver.html#c.tty_operations "tty_operations"). Then, upon open, a [`struct tty_struct`](tty_struct.html#c.tty_struct "tty_struct") is allocated and
lives until the final close. During this time, several callbacks from [`struct
tty_operations`](tty_driver.html#c.tty_operations "tty_operations") are invoked by the TTY layer.

Every character received by the kernel (both from devices and users) is passed
through a preselected [TTY Line Discipline](tty_ldisc.html) (in
short ldisc; in C, [`struct tty_ldisc_ops`](tty_ldisc.html#c.tty_ldisc_ops "tty_ldisc_ops")). Its task is to transform characters
as defined by a particular ldisc or by user too. The default one is n\_tty,
implementing echoes, signal handling, jobs control, special characters
processing, and more. The transformed characters are passed further to
user/device, depending on the source.

In-detail description of the named TTY structures is in separate documents:

* [TTY Driver and TTY Operations](tty_driver.html)
  + [Allocation](tty_driver.html#allocation)
  + [Registration](tty_driver.html#registration)
  + [TTY Driver Reference](tty_driver.html#tty-driver-reference)
  + [TTY Operations Reference](tty_driver.html#tty-operations-reference)
* [TTY Port](tty_port.html)
  + [TTY Port Functions](tty_port.html#tty-port-functions)
  + [TTY Port Reference](tty_port.html#tty-port-reference)
  + [TTY Port Operations Reference](tty_port.html#tty-port-operations-reference)
* [TTY Struct](tty_struct.html)
  + [Initialization](tty_struct.html#initialization)
  + [Name](tty_struct.html#name)
  + [Reference counting](tty_struct.html#reference-counting)
  + [Install](tty_struct.html#install)
  + [Read & Write](tty_struct.html#read-write)
  + [Start & Stop](tty_struct.html#start-stop)
  + [Wakeup](tty_struct.html#wakeup)
  + [Hangup](tty_struct.html#hangup)
  + [Misc](tty_struct.html#misc)
  + [TTY Struct Flags](tty_struct.html#tty-struct-flags)
  + [TTY Struct Reference](tty_struct.html#tty-struct-reference)
* [TTY Line Discipline](tty_ldisc.html)
  + [Registration](tty_ldisc.html#registration)
  + [Other Functions](tty_ldisc.html#other-functions)
  + [Line Discipline Operations Reference](tty_ldisc.html#line-discipline-operations-reference)
  + [Driver Access](tty_ldisc.html#driver-access)
  + [TTY Flags](tty_ldisc.html#tty-flags)
  + [Locking](tty_ldisc.html#locking)
  + [Internal Functions](tty_ldisc.html#internal-functions)
* [TTY Buffer](tty_buffer.html)
  + [Flip Buffer Management](tty_buffer.html#flip-buffer-management)
  + [Other Functions](tty_buffer.html#other-functions)
  + [Buffer Locking](tty_buffer.html#buffer-locking)
  + [Internal Functions](tty_buffer.html#internal-functions)
* [TTY IOCTL Helpers](tty_ioctl.html)
* [TTY Internals](tty_internals.html)
  + [Kopen](tty_internals.html#kopen)
  + [Exported Internal Functions](tty_internals.html#exported-internal-functions)
  + [Internal Functions](tty_internals.html#internal-functions)
* [Console](console.html)
  + [Struct Console](console.html#struct-console)
  + [Struct Consw](console.html#struct-consw)
  + [Console functions](console.html#console-functions)

## Writing TTY Driver

Before one starts writing a TTY driver, they must consider
[Serial](../serial/driver.html) and [USB Serial](../../usb/usb-serial.html)
layers first. Drivers for serial devices can often use one of these specific
layers to implement a serial driver. Only special devices should be handled
directly by the TTY Layer. If you are about to write such a driver, read on.

A *typical* sequence a TTY driver performs is as follows:

1. Allocate and register a TTY driver (module init)
2. Create and register TTY devices as they are probed (probe function)
3. Handle TTY operations and events like interrupts (TTY core invokes the
   former, the device the latter)
4. Remove devices as they are going away (remove function)
5. Unregister and free the TTY driver (module exit)

Steps regarding driver, i.e. 1., 3., and 5. are described in detail in
[TTY Driver and TTY Operations](tty_driver.html). For the other two (devices handling), look into
[TTY Port](tty_port.html).

## Other Documentation

Miscellaneous documentation can be further found in these documents:

* [MOXA Smartio/Industio Family Device Driver Installation Guide](moxa-smartio.html)
  + [1. Introduction](moxa-smartio.html#introduction)
  + [2. System Requirement](moxa-smartio.html#system-requirement)
  + [3. Installation](moxa-smartio.html#installation)
  + [4. Utilities](moxa-smartio.html#utilities)
  + [5. Setserial](moxa-smartio.html#setserial)
  + [6. Troubleshooting](moxa-smartio.html#troubleshooting)
* [GSM 0710 tty multiplexor HOWTO](n_gsm.html)
  + [How to use it](n_gsm.html#how-to-use-it)
* [N\_TTY](n_tty.html)
  + [External Functions](n_tty.html#external-functions)
  + [Internal Functions](n_tty.html#internal-functions)
