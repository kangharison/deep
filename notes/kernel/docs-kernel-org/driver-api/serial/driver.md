# Low Level Serial API

> 출처(원문): https://docs.kernel.org/driver-api/serial/driver.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Low Level Serial API

This document is meant as a brief overview of some aspects of the new serial
driver. It is not complete, any questions you have should be directed to
<[rmk@arm.linux.org.uk](mailto:rmk%40arm.linux.org.uk)>

The reference implementation is contained within amba-pl011.c.

## Low Level Serial Hardware Driver

The low level serial hardware driver is responsible for supplying port
information (defined by uart\_port) and a set of control methods (defined
by uart\_ops) to the core serial driver. The low level driver is also
responsible for handling interrupts for the port, and providing any
console support.

## Console Support

The serial core provides a few helper functions. This includes
decoding command line arguments ([`uart_parse_options()`](#c.uart_parse_options "uart_parse_options")).

There is also a helper function ([`uart_console_write()`](#c.uart_console_write "uart_console_write")) which performs a
character by character write, translating newlines to CRLF sequences.
Driver writers are recommended to use this function rather than implementing
their own version.

## Locking

It is the responsibility of the low level hardware driver to perform the
necessary locking using port->lock. There are some exceptions (which
are described in the [`struct uart_ops`](#c.uart_ops "uart_ops") listing below.)

There are two locks. A per-port spinlock, and an overall semaphore.

From the core driver perspective, the port->lock locks the following
data:

```
port->mctrl
port->icount
port->state->xmit.head (circ_buf->head)
port->state->xmit.tail (circ_buf->tail)
```

The low level driver is free to use this lock to provide any additional
locking.

The port\_sem semaphore is used to protect against ports being added/
removed or reconfigured at inappropriate times. Since v2.6.27, this
semaphore has been the ‘mutex’ member of the tty\_port struct, and
commonly referred to as the port mutex.

## uart\_ops

struct uart\_ops
:   * interface between serial\_core and the driver

**Definition**:

```
struct uart_ops {
    unsigned int    (*tx_empty)(struct uart_port *);
    void (*set_mctrl)(struct uart_port *, unsigned int mctrl);
    unsigned int    (*get_mctrl)(struct uart_port *);
    void (*stop_tx)(struct uart_port *);
    void (*start_tx)(struct uart_port *);
    void (*throttle)(struct uart_port *);
    void (*unthrottle)(struct uart_port *);
    void (*send_xchar)(struct uart_port *, char ch);
    void (*stop_rx)(struct uart_port *);
    void (*start_rx)(struct uart_port *);
    void (*enable_ms)(struct uart_port *);
    void (*break_ctl)(struct uart_port *, int ctl);
    int (*startup)(struct uart_port *);
    void (*shutdown)(struct uart_port *);
    void (*flush_buffer)(struct uart_port *);
    void (*set_termios)(struct uart_port *, struct ktermios *new, const struct ktermios *old);
    void (*set_ldisc)(struct uart_port *, struct ktermios *);
    void (*pm)(struct uart_port *, unsigned int state, unsigned int oldstate);
    const char      *(*type)(struct uart_port *);
    void (*release_port)(struct uart_port *);
    int (*request_port)(struct uart_port *);
    void (*config_port)(struct uart_port *, int);
    int (*verify_port)(struct uart_port *, struct serial_struct *);
    int (*ioctl)(struct uart_port *, unsigned int, unsigned long);
#ifdef CONFIG_CONSOLE_POLL;
    int (*poll_init)(struct uart_port *);
    void (*poll_put_char)(struct uart_port *, unsigned char);
    int (*poll_get_char)(struct uart_port *);
#endif;
};
```

**Members**

`tx_empty`
:   `unsigned int ()(struct uart_port *port)`

`set_mctrl`
:   `void ()(struct uart_port *port, unsigned int mctrl)`

`get_mctrl`
:   `unsigned int ()(struct uart_port *port)`

`stop_tx`
:   `void ()(struct uart_port *port)`

`start_tx`
:   `void ()(struct uart_port *port)`

`throttle`
:   `void ()(struct uart_port *port)`

`unthrottle`
:   `void ()(struct uart_port *port)`

`send_xchar`
:   `void ()(struct uart_port *port, char ch)`

`stop_rx`
:   `void ()(struct uart_port *port)`

`start_rx`
:   `void ()(struct uart_port *port)`

`enable_ms`
:   `void ()(struct uart_port *port)`

`break_ctl`
:   `void ()(struct uart_port *port, int ctl)`

`startup`
:   `int ()(struct uart_port *port)`

`shutdown`
:   `void ()(struct uart_port *port)`

`flush_buffer`
:   `void ()(struct uart_port *port)`

`set_termios`
:   `void ()(struct uart_port *port, struct ktermios *new,
    struct ktermios *old)`

`set_ldisc`
:   `void ()(struct uart_port *port, struct ktermios *termios)`

`pm`
:   `void ()(struct uart_port *port, unsigned int state,
    unsigned int oldstate)`

`type`
:   `const char *()(struct uart_port *port)`

`release_port`
:   `void ()(struct uart_port *port)`

`request_port`
:   `int ()(struct uart_port *port)`

`config_port`
:   `void ()(struct uart_port *port, int type)`

`verify_port`
:   `int ()(struct uart_port *port,
    struct serial_struct *serinfo)`

`ioctl`
:   `int ()(struct uart_port *port, unsigned int cmd,
    unsigned long arg)`

`poll_init`
:   `int ()(struct uart_port *port)`

`poll_put_char`
:   `void ()(struct uart_port *port, unsigned char ch)`

`poll_get_char`
:   `int ()(struct uart_port *port)`

**Description**

This structure describes all the operations that can be done on the
physical hardware.

> This function tests whether the transmitter fifo and shifter for the
> **port** is empty. If it is empty, this function should return
> `TIOCSER_TEMT`, otherwise return 0. If the port does not support this
> operation, then it should return `TIOCSER_TEMT`.
>
> Locking: none.
> Interrupts: caller dependent.
> This call must not sleep
>
> This function sets the modem control lines for **port** to the state
> described by **mctrl**. The relevant bits of **mctrl** are:
>
> > * `TIOCM_RTS` RTS signal.
> > * `TIOCM_DTR` DTR signal.
> > * `TIOCM_OUT1` OUT1 signal.
> > * `TIOCM_OUT2` OUT2 signal.
> > * `TIOCM_LOOP` Set the port into loopback mode.
>
> If the appropriate bit is set, the signal should be driven
> active. If the bit is clear, the signal should be driven
> inactive.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Returns the current state of modem control inputs of **port**. The state
> of the outputs should not be returned, since the core keeps track of
> their state. The state information should include:
>
> > * `TIOCM_CAR` state of DCD signal
> > * `TIOCM_CTS` state of CTS signal
> > * `TIOCM_DSR` state of DSR signal
> > * `TIOCM_RI` state of RI signal
>
> The bit is set if the signal is currently driven active. If
> the port does not support CTS, DCD or DSR, the driver should
> indicate that the signal is permanently active. If RI is
> not available, the signal should not be indicated as active.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Stop transmitting characters. This might be due to the CTS line
> becoming inactive or the tty layer indicating we want to stop
> transmission due to an `XOFF` character.
>
> The driver should stop transmitting characters as soon as possible.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Start transmitting characters.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Notify the serial driver that input buffers for the line discipline are
> close to full, and it should somehow signal that no more characters
> should be sent to the serial port.
> This will be called only if hardware assisted flow control is enabled.
>
> Locking: serialized with **`unthrottle()`** and termios modification by the
> tty layer.
>
> Notify the serial driver that characters can now be sent to the serial
> port without fear of overrunning the input buffers of the line
> disciplines.
>
> This will be called only if hardware assisted flow control is enabled.
>
> Locking: serialized with **`throttle()`** and termios modification by the
> tty layer.
>
> Transmit a high priority character, even if the port is stopped. This
> is used to implement XON/XOFF flow control and `tcflow()`. If the serial
> driver does not implement this function, the tty core will append the
> character to the circular buffer and then call `start_tx()` / `stop_tx()`
> to flush the data out.
>
> Do not transmit if **ch** == ‘0’ (`__DISABLED_CHAR`).
>
> Locking: none.
> Interrupts: caller dependent.
>
> Start receiving characters.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Stop receiving characters; the **port** is in the process of being closed.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Enable the modem status interrupts.
>
> This method may be called multiple times. Modem status interrupts
> should be disabled when the **`shutdown()`** method is called.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Control the transmission of a break signal. If **ctl** is nonzero, the
> break signal should be transmitted. The signal should be terminated
> when another call is made with a zero **ctl**.
>
> Locking: caller holds tty\_port->mutex
>
> Grab any interrupt resources and initialise any low level driver state.
> Enable the port for reception. It should not activate RTS nor DTR;
> this will be done via a separate call to **`set_mctrl()`**.
>
> This method will only be called when the port is initially opened.
>
> Locking: port\_sem taken.
> Interrupts: globally disabled.
>
> Disable the **port**, disable any break condition that may be in effect,
> and free any interrupt resources. It should not disable RTS nor DTR;
> this will have already been done via a separate call to **`set_mctrl()`**.
>
> Drivers must not access **port->state** once this call has completed.
>
> This method will only be called when there are no more users of this
> **port**.
>
> Locking: port\_sem taken.
> Interrupts: caller dependent.
>
> Flush any write buffers, reset any DMA state and stop any ongoing DMA
> transfers.
>
> This will be called whenever the **port->state->xmit** circular buffer is
> cleared.
>
> Locking: **port->lock** taken.
> Interrupts: locally disabled.
> This call must not sleep
>
> Change the **port** parameters, including word length, parity, stop bits.
> Update **port->read\_status\_mask** and **port->ignore\_status\_mask** to
> indicate the types of events we are interested in receiving. Relevant
> ktermios::c\_cflag bits are:
>
> * `CSIZE` - word size
> * `CSTOPB` - 2 stop bits
> * `PARENB` - parity enable
> * `PARODD` - odd parity (when `PARENB` is in force)
> * `ADDRB` - address bit (changed through uart\_port::`rs485_config()`).
> * `CREAD` - enable reception of characters (if not set, still receive
>   characters from the port, but throw them away).
> * `CRTSCTS` - if set, enable CTS status change reporting.
> * `CLOCAL` - if not set, enable modem status change reporting.
>
> Relevant ktermios::c\_iflag bits are:
>
> * `INPCK` - enable frame and parity error events to be passed to the TTY
>   layer.
> * `BRKINT` / `PARMRK` - both of these enable break events to be passed to
>   the TTY layer.
> * `IGNPAR` - ignore parity and framing errors.
> * `IGNBRK` - ignore break errors. If `IGNPAR` is also set, ignore overrun
>   errors as well.
>
> The interaction of the ktermios::c\_iflag bits is as follows (parity
> error given as an example):
>
> | Parity error | INPCK | IGNPAR |  |
> | --- | --- | --- | --- |
> | n/a | 0 | n/a | character received, marked as `TTY_NORMAL` |
> | None | 1 | n/a | character received, marked as `TTY_NORMAL` |
> | Yes | 1 | 0 | character received, marked as `TTY_PARITY` |
> | Yes | 1 | 1 | character discarded |
>
> Other flags may be used (eg, xon/xoff characters) if your hardware
> supports hardware “soft” flow control.
>
> Locking: caller holds tty\_port->mutex
> Interrupts: caller dependent.
> This call must not sleep
>
> Notifier for discipline change. See
> [TTY Line Discipline](../tty/tty_ldisc.html).
>
> Locking: caller holds tty\_port->mutex
>
> Perform any power management related activities on the specified **port**.
> **state** indicates the new state (defined by `enum uart_pm_state`),
> **oldstate** indicates the previous state.
>
> This function should not be used to grab any resources.
>
> This will be called when the **port** is initially opened and finally
> closed, except when the **port** is also the system console. This will
> occur even if `CONFIG_PM` is not set.
>
> Locking: none.
> Interrupts: caller dependent.
>
> Return a pointer to a string constant describing the specified **port**,
> or return `NULL`, in which case the string ‘unknown’ is substituted.
>
> Locking: none.
> Interrupts: caller dependent.
>
> Release any memory and IO region resources currently in use by the
> **port**.
>
> Locking: none.
> Interrupts: caller dependent.
>
> Request any memory and IO region resources required by the port. If any
> fail, no resources should be registered when this function returns, and
> it should return -`EBUSY` on failure.
>
> Locking: none.
> Interrupts: caller dependent.
>
> Perform any autoconfiguration steps required for the **port**. **type**
> contains a bit mask of the required configuration. `UART_CONFIG_TYPE`
> indicates that the port requires detection and identification.
> **port->type** should be set to the type found, or `PORT_UNKNOWN` if no
> port was detected.
>
> `UART_CONFIG_IRQ` indicates autoconfiguration of the interrupt signal,
> which should be probed using standard kernel autoprobing techniques.
> This is not necessary on platforms where ports have interrupts
> internally hard wired (eg, system on a chip implementations).
>
> Locking: none.
> Interrupts: caller dependent.
>
> Verify the new serial port information contained within **serinfo** is
> suitable for this port type.
>
> Locking: none.
> Interrupts: caller dependent.
>
> Perform any port specific IOCTLs. IOCTL commands must be defined using
> the standard numbering system found in <asm/ioctl.h>.
>
> Locking: none.
> Interrupts: caller dependent.
>
> Called by kgdb to perform the minimal hardware initialization needed to
> support **`poll_put_char()`** and **`poll_get_char()`**. Unlike **`startup()`**, this
> should not request interrupts.
>
> Locking: `tty_mutex` and tty\_port->mutex taken.
> Interrupts: n/a.
>
> Called by kgdb to write a single character **ch** directly to the serial
> **port**. It can and should block until there is space in the TX FIFO.
>
> Locking: none.
> Interrupts: caller dependent.
> This call must not sleep
>
> Called by kgdb to read a single character directly from the serial
> port. If data is available, it should be returned; otherwise the
> function should return `NO_POLL_CHAR` immediately.
>
> Locking: none.
> Interrupts: caller dependent.
> This call must not sleep

## Other functions

void uart\_write\_wakeup(struct uart\_port \*port)
:   schedule write processing

**Parameters**

`struct uart_port *port`
:   port to be processed

**Description**

This routine is used by the interrupt handler to schedule processing in the
software interrupt portion of the driver. A driver is expected to call this
function when the number of characters in the transmit buffer have dropped
below a threshold.

Locking: **port->lock** should be held

void uart\_update\_timeout(struct uart\_port \*port, unsigned int cflag, unsigned int baud)
:   update per-port frame timing information

**Parameters**

`struct uart_port *port`
:   uart\_port structure describing the port

`unsigned int cflag`
:   termios cflag value

`unsigned int baud`
:   speed of the port

**Description**

Set the **port** frame timing information from which the FIFO timeout value is
derived. The **cflag** value should reflect the actual hardware settings as
number of bits, parity, stop bits and baud rate is taken into account here.

Locking: caller is expected to take **port->lock**

unsigned int uart\_get\_baud\_rate(struct uart\_port \*port, struct ktermios \*termios, const struct ktermios \*old, unsigned int min, unsigned int max)
:   return baud rate for a particular port

**Parameters**

`struct uart_port *port`
:   uart\_port structure describing the port in question.

`struct ktermios *termios`
:   desired termios settings

`const struct ktermios *old`
:   old termios (or `NULL`)

`unsigned int min`
:   minimum acceptable baud rate

`unsigned int max`
:   maximum acceptable baud rate

**Description**

Decode the termios structure into a numeric baud rate, taking account of the
magic 38400 baud rate (with spd\_\* flags), and mapping the `B0` rate to 9600
baud.

If the new baud rate is invalid, try the **old** termios setting. If it’s still
invalid, we try 9600 baud. If that is also invalid 0 is returned.

The **termios** structure is updated to reflect the baud rate we’re actually
going to be using. Don’t do this for the case where B0 is requested (“hang
up”).

Locking: caller dependent

unsigned int uart\_get\_divisor(struct uart\_port \*port, unsigned int baud)
:   return uart clock divisor

**Parameters**

`struct uart_port *port`
:   uart\_port structure describing the port

`unsigned int baud`
:   desired baud rate

**Description**

Calculate the divisor (baud\_base / baud) for the specified **baud**,
appropriately rounded.

If 38400 baud and custom divisor is selected, return the custom divisor
instead.

Locking: caller dependent

int uart\_get\_lsr\_info(struct [tty\_struct](../tty/tty_struct.html#c.tty_struct "tty_struct") \*tty, struct uart\_state \*state, unsigned int \_\_user \*value)
:   get line status register info

**Parameters**

`struct tty_struct *tty`
:   tty associated with the UART

`struct uart_state *state`
:   UART being queried

`unsigned int __user *value`
:   returned modem value

void uart\_console\_write(struct uart\_port \*port, const char \*s, unsigned int count, void (\*putchar)(struct uart\_port\*, unsigned char))
:   write a console message to a serial port

**Parameters**

`struct uart_port *port`
:   the port to write the message

`const char *s`
:   array of characters

`unsigned int count`
:   number of characters in string to write

`void (*putchar)(struct uart_port *, unsigned char)`
:   function to write character to port

int uart\_parse\_earlycon(char \*p, enum uart\_iotype \*iotype, resource\_size\_t \*addr, char \*\*options)
:   Parse earlycon options

**Parameters**

`char *p`
:   ptr to 2nd field (ie., just beyond ‘<name>,’)

`enum uart_iotype *iotype`
:   ptr for decoded iotype (out)

`resource_size_t *addr`
:   ptr for decoded mapbase/iobase (out)

`char **options`
:   ptr for <options> field; `NULL` if not present (out)

**Description**

Decodes earlycon kernel command line parameters of the form:
:   * earlycon=<name>,io|mmio|mmio16|mmio32|mmio32be|mmio32native,<addr>,<options>
    * console=<name>,io|mmio|mmio16|mmio32|mmio32be|mmio32native,<addr>,<options>

The optional form:
:   * earlycon=<name>,0x<addr>,<options>
    * console=<name>,0x<addr>,<options>

is also accepted; the returned **iotype** will be `UPIO_MEM`.

**Return**

0 on success or -`EINVAL` on failure

void uart\_parse\_options(const char \*options, int \*baud, int \*parity, int \*bits, int \*flow)
:   Parse serial port baud/parity/bits/flow control.

**Parameters**

`const char *options`
:   pointer to option string

`int *baud`
:   pointer to an ‘int’ variable for the baud rate.

`int *parity`
:   pointer to an ‘int’ variable for the parity.

`int *bits`
:   pointer to an ‘int’ variable for the number of data bits.

`int *flow`
:   pointer to an ‘int’ variable for the flow control character.

**Description**

[`uart_parse_options()`](#c.uart_parse_options "uart_parse_options") decodes a string containing the serial console
options. The format of the string is <baud><parity><bits><flow>,
eg: 115200n8r

int uart\_set\_options(struct uart\_port \*port, struct [console](../tty/console.html#c.console "console") \*co, int baud, int parity, int bits, int flow)
:   setup the serial console parameters

**Parameters**

`struct uart_port *port`
:   pointer to the serial ports uart\_port structure

`struct console *co`
:   console pointer

`int baud`
:   baud rate

`int parity`
:   parity character - ‘n’ (none), ‘o’ (odd), ‘e’ (even)

`int bits`
:   number of data bits

`int flow`
:   flow control character - ‘r’ (rts)

**Description**

Locking: Caller must hold console\_list\_lock in order to serialize
early initialization of the serial-console lock.

int uart\_register\_driver(struct uart\_driver \*drv)
:   register a driver with the uart core layer

**Parameters**

`struct uart_driver *drv`
:   low level driver structure

**Description**

Register a uart driver with the core driver. We in turn register with the
tty layer, and initialise the core driver per-port state.

We have a proc file in /proc/tty/driver which is named after the normal
driver.

**drv->port** should be `NULL`, and the per-port structures should be registered
using `uart_add_one_port()` after this call has succeeded.

Locking: none, Interrupts: enabled

void uart\_unregister\_driver(struct uart\_driver \*drv)
:   remove a driver from the uart core layer

**Parameters**

`struct uart_driver *drv`
:   low level driver structure

**Description**

Remove all references to a driver from the core driver. The low level
driver must have removed all its ports via the `uart_remove_one_port()` if it
registered them with `uart_add_one_port()`. (I.e. **drv->port** is `NULL`.)

Locking: none, Interrupts: enabled

bool uart\_match\_port(const struct uart\_port \*port1, const struct uart\_port \*port2)
:   are the two ports equivalent?

**Parameters**

`const struct uart_port *port1`
:   first port

`const struct uart_port *port2`
:   second port

**Description**

This utility function can be used to determine whether two uart\_port
structures describe the same port.

void uart\_handle\_dcd\_change(struct uart\_port \*uport, bool active)
:   handle a change of carrier detect state

**Parameters**

`struct uart_port *uport`
:   uart\_port structure for the open port

`bool active`
:   new carrier detect status

**Description**

Caller must hold uport->lock.

void uart\_handle\_cts\_change(struct uart\_port \*uport, bool active)
:   handle a change of clear-to-send state

**Parameters**

`struct uart_port *uport`
:   uart\_port structure for the open port

`bool active`
:   new clear-to-send status

**Description**

Caller must hold uport->lock.

bool uart\_try\_toggle\_sysrq(struct uart\_port \*port, u8 ch)
:   Enables SysRq from serial line

**Parameters**

`struct uart_port *port`
:   uart\_port structure where char(s) after BREAK met

`u8 ch`
:   new character in the sequence after received BREAK

**Description**

Enables magic SysRq when the required sequence is met on port
(see CONFIG\_MAGIC\_SYSRQ\_SERIAL\_SEQUENCE).

**Return**

`false` if **ch** is out of enabling sequence and should be
handled some other way, `true` if **ch** was consumed.

uart\_port\_tx\_limited

`uart_port_tx_limited (port, ch, count, tx_ready, put_char, tx_done)`

> * transmit helper for uart\_port with count limiting

**Parameters**

`port`
:   uart port

`ch`
:   variable to store a character to be written to the HW

`count`
:   a limit of characters to send

`tx_ready`
:   can HW accept more data function

`put_char`
:   function to write a character

`tx_done`
:   function to call after the loop is done

**Description**

This helper transmits characters from the xmit buffer to the hardware using
**`put_char()`**. It does so until **count** characters are sent and while **tx\_ready**
evaluates to true.

The expression in macro parameters shall be designed as follows:
:   * **tx\_ready:** should evaluate to true if the HW can accept more data to
      be sent. This parameter can be `true`, which means the HW is always ready.
    * **put\_char:** shall write **ch** to the device of **port**.
    * **tx\_done:** when the write loop is done, this can perform arbitrary
      action before potential invocation of ops->`stop_tx()` happens. If the
      driver does not need to do anything, use e.g. ({}).

For all of them, **port->lock** is held, interrupts are locally disabled and
the expressions must not sleep.

**Return**

the number of characters in the xmit buffer when done.

uart\_port\_tx

`uart_port_tx (port, ch, tx_ready, put_char)`

> * transmit helper for uart\_port

**Parameters**

`port`
:   uart port

`ch`
:   variable to store a character to be written to the HW

`tx_ready`
:   can HW accept more data function

`put_char`
:   function to write a character

**Description**

See [`uart_port_tx_limited()`](#c.uart_port_tx_limited "uart_port_tx_limited") for more details.

## Other notes

It is intended some day to drop the ‘unused’ entries from uart\_port, and
allow low level drivers to register their own individual uart\_port’s with
the core. This will allow drivers to use uart\_port as a pointer to a
structure containing both the uart\_port entry with their own extensions,
thus:

```
struct my_port {
        struct uart_port        port;
        int                     my_stuff;
};
```

## Modem control lines via GPIO

Some helpers are provided in order to set/get modem control lines via GPIO.

void mctrl\_gpio\_set(struct mctrl\_gpios \*gpios, unsigned int mctrl)
:   set gpios according to mctrl state

**Parameters**

`struct mctrl_gpios *gpios`
:   gpios to set

`unsigned int mctrl`
:   state to set

**Description**

Set the gpios according to the mctrl state.

struct gpio\_desc \*mctrl\_gpio\_to\_gpiod(struct mctrl\_gpios \*gpios, enum mctrl\_gpio\_idx gidx)
:   obtain gpio\_desc of modem line index

**Parameters**

`struct mctrl_gpios *gpios`
:   gpios to look into

`enum mctrl_gpio_idx gidx`
:   index of the modem line

**Return**

the gpio\_desc structure associated to the modem line index

unsigned int mctrl\_gpio\_get(struct mctrl\_gpios \*gpios, unsigned int \*mctrl)
:   update mctrl with the gpios values.

**Parameters**

`struct mctrl_gpios *gpios`
:   gpios to get the info from

`unsigned int *mctrl`
:   mctrl to set

**Return**

modified mctrl (the same value as in **mctrl**)

**Description**

Update mctrl with the gpios values.

struct mctrl\_gpios \*mctrl\_gpio\_init(struct uart\_port \*port, unsigned int idx)
:   initialize uart gpios

**Parameters**

`struct uart_port *port`
:   port to initialize gpios for

`unsigned int idx`
:   index of the gpio in the **port**’s device

**Description**

This will get the {cts,rts,...}-gpios from device tree if they are present
and request them, set direction etc, and return an allocated structure.
devm\_\* functions are used, so there’s no need to explicitly free.
As this sets up the irq handling, make sure to not handle changes to the
gpio input lines in your driver, too.

void mctrl\_gpio\_enable\_ms(struct mctrl\_gpios \*gpios)
:   enable irqs and handling of changes to the ms lines

**Parameters**

`struct mctrl_gpios *gpios`
:   gpios to enable

void mctrl\_gpio\_disable\_ms\_sync(struct mctrl\_gpios \*gpios)
:   disable irqs and handling of changes to the ms lines, and wait for any pending IRQ to be processed

**Parameters**

`struct mctrl_gpios *gpios`
:   gpios to disable

void mctrl\_gpio\_disable\_ms\_no\_sync(struct mctrl\_gpios \*gpios)
:   disable irqs and handling of changes to the ms lines, and return immediately

**Parameters**

`struct mctrl_gpios *gpios`
:   gpios to disable
