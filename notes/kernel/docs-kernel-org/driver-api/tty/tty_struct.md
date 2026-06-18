# TTY Struct

> 출처(원문): https://docs.kernel.org/driver-api/tty/tty_struct.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY Struct

[`struct tty_struct`](#c.tty_struct "tty_struct") is allocated by the TTY layer upon the first open of the TTY
device and released after the last close. The TTY layer passes this structure
to most of `struct tty_operation`’s hooks. Members of tty\_struct are documented
in [TTY Struct Reference](#tty-struct-reference) at the bottom.

## [Initialization](#id1)

void tty\_init\_termios(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   helper for termios setup

**Parameters**

`struct tty_struct *tty`
:   the tty to set up

**Description**

Initialise the termios structure for this tty. This runs under the
`tty_mutex` currently so we can be relaxed about ordering.

## [Name](#id2)

const char \*tty\_name(const struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   return tty naming

**Parameters**

`const struct tty_struct *tty`
:   tty structure

**Description**

Convert a tty structure into a name. The name reflects the kernel naming
policy and if udev is in use may not reflect user space

Locking: none

## [Reference counting](#id3)

struct [tty\_struct](#c.tty_struct "tty_struct") \*tty\_kref\_get(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   get a tty reference

**Parameters**

`struct tty_struct *tty`
:   tty device

**Return**

a new reference to a tty object

**Description**

Locking: The caller must hold sufficient locks/counts to ensure that their
existing reference cannot go away.

void tty\_kref\_put(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   release a tty kref

**Parameters**

`struct tty_struct *tty`
:   tty device

**Description**

Release a reference to the **tty** device and if need be let the kref layer
destruct the object for us.

## [Install](#id4)

int tty\_standard\_install(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   usual tty->ops->install

**Parameters**

`struct tty_driver *driver`
:   the driver for the tty

`struct tty_struct *tty`
:   the tty

**Description**

If the **driver** overrides **tty->ops->install**, it still can call this function
to perform the standard install operations.

## [Read & Write](#id5)

int tty\_put\_char(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty, u8 ch)
:   write one character to a tty

**Parameters**

`struct tty_struct *tty`
:   tty

`u8 ch`
:   character to write

**Description**

Write one byte to the **tty** using the provided **tty->ops->`put_char()`** method
if present.

**Note**

the specific put\_char operation in the driver layer may go
away soon. Don’t call it directly, use this method

**Return**

the number of characters successfully output.

## [Start & Stop](#id6)

void stop\_tty(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   propagate flow control

**Parameters**

`struct tty_struct *tty`
:   tty to stop

**Description**

Perform flow control to the driver. May be called on an already stopped
device and will not re-call the [`tty_driver->stop()`](tty_driver.html#c.tty_driver "tty_driver") method.

This functionality is used by both the line disciplines for halting incoming
flow and by the driver. It may therefore be called from any context, may be
under the tty `atomic_write_lock` but not always.

Locking:
:   flow.lock

void start\_tty(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   propagate flow control

**Parameters**

`struct tty_struct *tty`
:   tty to start

**Description**

Start a tty that has been stopped if at all possible. If **tty** was previously
stopped and is now being started, the [`tty_driver->start()`](tty_driver.html#c.tty_driver "tty_driver") method is invoked
and the line discipline woken.

Locking:
:   flow.lock

## [Wakeup](#id7)

void tty\_wakeup(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   request more data

**Parameters**

`struct tty_struct *tty`
:   terminal

**Description**

Internal and external helper for wakeups of tty. This function informs the
line discipline if present that the driver is ready to receive more output
data.

## [Hangup](#id8)

void tty\_hangup(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   trigger a hangup event

**Parameters**

`struct tty_struct *tty`
:   tty to hangup

**Description**

A carrier loss (virtual or otherwise) has occurred on **tty**. Schedule a
hangup sequence to run after this event.

void tty\_vhangup(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty)
:   process vhangup

**Parameters**

`struct tty_struct *tty`
:   tty to hangup

**Description**

The user has asked via system call for the terminal to be hung up. We do
this synchronously so that when the syscall returns the process is complete.
That guarantee is necessary for security reasons.

int tty\_hung\_up\_p(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   was tty hung up

**Parameters**

`struct file *filp`
:   file pointer of tty

**Return**

true if the tty has been subject to a vhangup or a carrier loss

## [Misc](#id9)

int tty\_do\_resize(struct [tty\_struct](#c.tty_struct "tty_struct") \*tty, struct winsize \*ws)
:   resize event

**Parameters**

`struct tty_struct *tty`
:   tty being resized

`struct winsize *ws`
:   new dimensions

**Description**

Update the termios variables and send the necessary signals to peform a
terminal resize correctly.

## [TTY Struct Flags](#id10)

enum tty\_struct\_flags
:   TTY Struct Flags

**Constants**

`TTY_THROTTLED`
:   Driver input is throttled. The ldisc should call
    `tty_driver.unthrottle()` in order to resume reception when
    it is ready to process more data (at threshold min).

`TTY_IO_ERROR`
:   If set, causes all subsequent userspace read/write calls on the tty to
    fail, returning -`EIO`. (May be no ldisc too.)

`TTY_OTHER_CLOSED`
:   Device is a pty and the other side has closed.

`TTY_EXCLUSIVE`
:   Exclusive open mode (a single opener).

`TTY_DO_WRITE_WAKEUP`
:   If set, causes the driver to call the
    `tty_ldisc_ops.write_wakeup()` method in order to resume
    transmission when it can accept more data to transmit.

`TTY_LDISC_OPEN`
:   Indicates that a line discipline is open. For debugging purposes only.

`TTY_PTY_LOCK`
:   A flag private to pty code to implement `TIOCSPTLCK`/`TIOCGPTLCK` logic.

`TTY_NO_WRITE_SPLIT`
:   Prevent driver from splitting up writes into smaller chunks (preserve
    write boundaries to driver).

`TTY_HUPPED`
:   The TTY was hung up. This is set post `tty_driver.hangup()`.

`TTY_HUPPING`
:   The TTY is in the process of hanging up to abort potential readers.

`TTY_LDISC_CHANGING`
:   Line discipline for this TTY is being changed. I/O should not block
    when this is set. Use `tty_io_nonblock()` to check.

`TTY_LDISC_HALTED`
:   Line discipline for this TTY was stopped. No work should be queued to
    this ldisc.

**Description**

These bits are used in the `tty_struct.flags` field.

So that interrupts won’t be able to mess up the queues,
copy\_to\_cooked must be atomic with respect to itself, as must
tty->write. Thus, you must use the inline functions [`set_bit()`](../../core-api/kernel-api.html#c.set_bit "set_bit") and
[`clear_bit()`](../../core-api/kernel-api.html#c.clear_bit "clear_bit") to make things atomic.

## [TTY Struct Reference](#id11)

struct tty\_struct
:   state associated with a tty while open

**Definition**:

```
struct tty_struct {
    struct kref kref;
    int index;
    struct device *dev;
    struct tty_driver *driver;
    struct tty_port *port;
    const struct tty_operations *ops;
    struct tty_ldisc *ldisc;
    struct ld_semaphore ldisc_sem;
    struct mutex atomic_write_lock;
    struct mutex legacy_mutex;
    struct mutex throttle_mutex;
    struct rw_semaphore termios_rwsem;
    struct mutex winsize_mutex;
    struct ktermios termios, termios_locked;
    char name[64];
    unsigned long flags;
    int count;
    unsigned int receive_room;
    struct winsize winsize;
    struct {
        spinlock_t lock;
        bool stopped;
        bool tco_stopped;
    } flow;
    struct {
        struct pid *pgrp;
        struct pid *session;
        spinlock_t lock;
        unsigned char pktstatus;
        bool packet;
    } ctrl;
    bool hw_stopped;
    bool closing;
    int flow_change;
    struct tty_struct *link;
    struct fasync_struct *fasync;
    wait_queue_head_t write_wait;
    wait_queue_head_t read_wait;
    struct work_struct hangup_work;
    void *disc_data;
    void *driver_data;
    spinlock_t files_lock;
    int write_cnt;
    u8 *write_buf;
    struct list_head tty_files;
    struct work_struct SAK_work;
};
```

**Members**

`kref`
:   reference counting by [`tty_kref_get()`](#c.tty_kref_get "tty_kref_get") and [`tty_kref_put()`](#c.tty_kref_put "tty_kref_put"), reaching zero
    frees the structure

`index`
:   index of this tty (e.g. to construct **name** like tty12)

`dev`
:   class device or `NULL` (e.g. ptys, serdev)

`driver`
:   [`struct tty_driver`](tty_driver.html#c.tty_driver "tty_driver") operating this tty

`port`
:   persistent storage for this device (i.e. [`struct tty_port`](tty_port.html#c.tty_port "tty_port"))

`ops`
:   [`struct tty_operations`](tty_driver.html#c.tty_operations "tty_operations") of **driver** for this tty (open, close, etc.)

`ldisc`
:   the current line discipline for this tty (n\_tty by default)

`ldisc_sem`
:   protects line discipline changes (**ldisc**) -- lock tty not pty

`atomic_write_lock`
:   protects against concurrent writers, i.e. locks
    **write\_cnt**, **write\_buf** and similar

`legacy_mutex`
:   leftover from history (BKL -> BTM -> **legacy\_mutex**),
    protecting several operations on this tty

`throttle_mutex`
:   protects against concurrent [`tty_throttle_safe()`](tty_ioctl.html#c.tty_throttle_safe "tty_throttle_safe") and
    [`tty_unthrottle_safe()`](tty_ioctl.html#c.tty_unthrottle_safe "tty_unthrottle_safe") (but not [`tty_unthrottle()`](tty_ioctl.html#c.tty_unthrottle "tty_unthrottle"))

`termios_rwsem`
:   protects **termios** and **termios\_locked**

`winsize_mutex`
:   protects **winsize**

`termios`
:   termios for the current tty, copied from/to **driver.termios**

`termios_locked`
:   locked termios (by `TIOCGLCKTRMIOS` and `TIOCSLCKTRMIOS`
    ioctls)

`name`
:   name of the tty constructed by [`tty_line_name()`](tty_internals.html#c.tty_line_name "tty_line_name") (e.g. ttyS3)

`flags`
:   bitwise OR of `TTY_THROTTLED`, `TTY_IO_ERROR`, ...

`count`
:   count of open processes, reaching zero cancels all the work for
    this tty and drops a **kref** too (but does not free this tty)

`receive_room`
:   bytes permitted to feed to **ldisc** without any being lost

`winsize`
:   size of the terminal “window” (cf. **winsize\_mutex**)

`flow`
:   flow settings grouped together

`flow.lock`
:   lock for **flow** members

`flow.stopped`
:   tty stopped/started by [`stop_tty()`](#c.stop_tty "stop_tty")/[`start_tty()`](#c.start_tty "start_tty")

`flow.tco_stopped`
:   tty stopped/started by `TCOOFF`/`TCOON` ioctls (it has
    precedence over **flow.stopped**)

`ctrl`
:   control settings grouped together

`ctrl.pgrp`
:   process group of this tty (setpgrp(2))

`ctrl.session`
:   session of this tty (setsid(2)). Writes are protected by both
    **ctrl.lock** and **legacy\_mutex**, readers must use at least one of
    them.

`ctrl.lock`
:   lock for **ctrl** members

`ctrl.pktstatus`
:   packet mode status (bitwise OR of `TIOCPKT_` constants)

`ctrl.packet`
:   packet mode enabled

`hw_stopped`
:   not controlled by the tty layer, under **driver**’s control for CTS
    handling

`closing`
:   when set during close, n\_tty processes only START & STOP chars

`flow_change`
:   controls behavior of throttling, see [`tty_throttle_safe()`](tty_ioctl.html#c.tty_throttle_safe "tty_throttle_safe") and
    [`tty_unthrottle_safe()`](tty_ioctl.html#c.tty_unthrottle_safe "tty_unthrottle_safe")

`link`
:   link to another pty (master -> slave and vice versa)

`fasync`
:   state for `O_ASYNC` (for `SIGIO`); managed by `fasync_helper()`

`write_wait`
:   concurrent writers are waiting in this queue until they are
    allowed to write

`read_wait`
:   readers wait for data in this queue

`hangup_work`
:   normally a work to perform a hangup (`do_tty_hangup()`); while
    freeing the tty, (re)used to [`release_one_tty()`](tty_internals.html#c.release_one_tty "release_one_tty")

`disc_data`
:   pointer to **ldisc**’s private data (e.g. to `struct n_tty_data`)

`driver_data`
:   pointer to **driver**’s private data (e.g. `struct uart_state`)

`files_lock`
:   protects **tty\_files** list

`write_cnt`
:   count of bytes written in [`tty_write()`](tty_internals.html#c.tty_write "tty_write") to **write\_buf**

`write_buf`
:   temporary buffer used during [`tty_write()`](tty_internals.html#c.tty_write "tty_write") to copy user data to

`tty_files`
:   list of (re)openers of this tty (i.e. linked `struct
    tty_file_private`)

`SAK_work`
:   if the tty has a pending do\_SAK, it is queued here

**Description**

All of the state associated with a tty while the tty is open. Persistent
storage for tty devices is referenced here as **port** and is documented in
[`struct tty_port`](tty_port.html#c.tty_port "tty_port").
