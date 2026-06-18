# TTY IOCTL Helpers

> 출처(원문): https://docs.kernel.org/driver-api/tty/tty_ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY IOCTL Helpers

unsigned int tty\_chars\_in\_buffer(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   characters pending

**Parameters**

`struct tty_struct *tty`
:   terminal

**Return**

the number of bytes of data in the device private output queue. If
no private method is supplied there is assumed to be no queue on the device.

unsigned int tty\_write\_room(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   write queue space

**Parameters**

`struct tty_struct *tty`
:   terminal

**Return**

the number of bytes that can be queued to this device at the present
time. The result should be treated as a guarantee and the driver cannot
offer a value it later shrinks by more than the number of bytes written. If
no method is provided, 2K is always returned and data may be lost as there
will be no flow control.

void tty\_driver\_flush\_buffer(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   discard internal buffer

**Parameters**

`struct tty_struct *tty`
:   terminal

**Description**

Discard the internal output buffer for this device. If no method is provided,
then either the buffer cannot be hardware flushed or there is no buffer
driver side.

void tty\_unthrottle(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   flow control

**Parameters**

`struct tty_struct *tty`
:   terminal

**Description**

Indicate that a **tty** may continue transmitting data down the stack. Takes
the [`tty_struct->termios_rwsem`](tty_struct.html#c.tty_struct "tty_struct") to protect against parallel
throttle/unthrottle and also to ensure the driver can consistently reference
its own termios data at this point when implementing software flow control.

Drivers should however remember that the stack can issue a throttle, then
change flow control method, then unthrottle.

bool tty\_throttle\_safe(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   flow control

**Parameters**

`struct tty_struct *tty`
:   terminal

**Description**

Indicate that a **tty** should stop transmitting data down the stack.
[`tty_throttle_safe()`](#c.tty_throttle_safe "tty_throttle_safe") will only attempt throttle if **tty->flow\_change** is
`TTY_THROTTLE_SAFE`. Prevents an accidental throttle due to race conditions
when throttling is conditional on factors evaluated prior to throttling.

**Return**

`true` if **tty** is throttled (or was already throttled)

bool tty\_unthrottle\_safe(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   flow control

**Parameters**

`struct tty_struct *tty`
:   terminal

**Description**

Similar to [`tty_unthrottle()`](#c.tty_unthrottle "tty_unthrottle") but will only attempt unthrottle if
**tty->flow\_change** is `TTY_UNTHROTTLE_SAFE`. Prevents an accidental unthrottle
due to race conditions when unthrottling is conditional on factors evaluated
prior to unthrottling.

**Return**

`true` if **tty** is unthrottled (or was already unthrottled)

void tty\_wait\_until\_sent(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, long timeout)
:   wait for I/O to finish

**Parameters**

`struct tty_struct *tty`
:   tty we are waiting for

`long timeout`
:   how long we will wait

**Description**

Wait for characters pending in a tty driver to hit the wire, or for a
timeout to occur (eg due to flow control).

Locking: none

void tty\_termios\_copy\_hw(struct ktermios \*new, const struct ktermios \*old)
:   copy hardware settings

**Parameters**

`struct ktermios *new`
:   new termios

`const struct ktermios *old`
:   old termios

**Description**

Propagate the hardware specific terminal setting bits from the **old** termios
structure to the **new** one. This is used in cases where the hardware does not
support reconfiguration or as a helper in some cases where only minimal
reconfiguration is supported.

bool tty\_termios\_hw\_change(const struct ktermios \*a, const struct ktermios \*b)
:   check for setting change

**Parameters**

`const struct ktermios *a`
:   termios

`const struct ktermios *b`
:   termios to compare

**Description**

Check if any of the bits that affect a dumb device have changed between the
two termios structures, or a speed change is needed.

**Return**

`true` if change is needed

unsigned char tty\_get\_char\_size(unsigned int cflag)
:   get size of a character

**Parameters**

`unsigned int cflag`
:   termios cflag value

**Return**

size (in bits) of a character depending on **cflag**’s `CSIZE` setting

unsigned char tty\_get\_frame\_size(unsigned int cflag)
:   get size of a frame

**Parameters**

`unsigned int cflag`
:   termios cflag value

**Description**

Get the size (in bits) of a frame depending on **cflag**’s `CSIZE`, `CSTOPB`, and
`PARENB` setting. The result is a sum of character size, start and stop bits
-- one bit each -- second stop bit (if set), and parity bit (if set).

**Return**

size (in bits) of a frame depending on **cflag**’s setting.

int tty\_set\_termios(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct ktermios \*new\_termios)
:   update termios values

**Parameters**

`struct tty_struct *tty`
:   tty to update

`struct ktermios *new_termios`
:   desired new value

**Description**

Perform updates to the termios values set on this **tty**. A master pty’s
termios should never be set.

Locking: [`tty_struct->termios_rwsem`](tty_struct.html#c.tty_struct "tty_struct")

int set\_termios(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, void \_\_user \*arg, int opt)
:   set termios values for a tty

**Parameters**

`struct tty_struct *tty`
:   terminal device

`void __user *arg`
:   user data

`int opt`
:   option information

**Description**

Helper function to prepare termios data and run necessary other functions
before using [`tty_set_termios()`](#c.tty_set_termios "tty_set_termios") to do the actual changes.

Locking: called functions take [`tty_struct->ldisc_sem`](tty_struct.html#c.tty_struct "tty_struct") and
[`tty_struct->termios_rwsem`](tty_struct.html#c.tty_struct "tty_struct") locks

**Return**

0 on success, an error otherwise

int set\_sgttyb(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [sgttyb](#c.set_sgttyb "sgttyb") \_\_user \*sgttyb)
:   set legacy terminal values

**Parameters**

`struct tty_struct *tty`
:   tty structure

`struct sgttyb __user *sgttyb`
:   pointer to old style terminal structure

**Description**

Updates a terminal from the legacy BSD style terminal information structure.

Locking: [`tty_struct->termios_rwsem`](tty_struct.html#c.tty_struct "tty_struct")

**Return**

0 on success, an error otherwise

int tty\_change\_softcar(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, bool enable)
:   carrier change ioctl helper

**Parameters**

`struct tty_struct *tty`
:   tty to update

`bool enable`
:   enable/disable `CLOCAL`

**Description**

Perform a change to the `CLOCAL` state and call into the driver layer to make
it visible.

Locking: [`tty_struct->termios_rwsem`](tty_struct.html#c.tty_struct "tty_struct").

**Return**

0 on success, an error otherwise

int tty\_mode\_ioctl(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, unsigned int cmd, unsigned long arg)
:   mode related ioctls

**Parameters**

`struct tty_struct *tty`
:   tty for the ioctl

`unsigned int cmd`
:   command

`unsigned long arg`
:   ioctl argument

**Description**

Perform non-line discipline specific mode control ioctls. This is designed
to be called by line disciplines to ensure they provide consistent mode
setting.

speed\_t tty\_get\_baud\_rate(const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   get tty bit rates

**Parameters**

`const struct tty_struct *tty`
:   tty to query

**Return**

the baud rate as an integer for this terminal

**Description**

Locking: The termios lock must be held by the caller.
