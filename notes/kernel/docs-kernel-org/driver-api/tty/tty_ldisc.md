# TTY Line Discipline

> 출처(원문): https://docs.kernel.org/driver-api/tty/tty_ldisc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY Line Discipline

TTY line discipline process all incoming and outgoing character from/to a tty
device. The default line discipline is [N\_TTY](n_tty.html). It is also a
fallback if establishing any other discipline for a tty fails. If even N\_TTY
fails, N\_NULL takes over. That never fails, but also does not process any
characters -- it throws them away.

## [Registration](#id1)

Line disciplines are registered with [`tty_register_ldisc()`](#c.tty_register_ldisc "tty_register_ldisc") passing the ldisc
structure. At the point of registration the discipline must be ready to use and
it is possible it will get used before the call returns success. If the call
returns an error then it won’t get called. Do not reuse ldisc numbers as they
are part of the userspace ABI and writing over an existing ldisc will cause
demons to eat your computer. You must not re-register over the top of the line
discipline even with the same data or your computer again will be eaten by
demons. In order to remove a line discipline call [`tty_unregister_ldisc()`](#c.tty_unregister_ldisc "tty_unregister_ldisc").

Heed this warning: the reference count field of the registered copies of the
tty\_ldisc structure in the ldisc table counts the number of lines using this
discipline. The reference count of the tty\_ldisc structure within a tty counts
the number of active users of the ldisc at this instant. In effect it counts
the number of threads of execution within an ldisc method (plus those about to
enter and exit although this detail matters not).

int tty\_register\_ldisc(const struct [tty\_ldisc\_ops](#c.tty_ldisc_ops "tty_ldisc_ops") \*new\_ldisc)
:   install a line discipline

**Parameters**

`const struct tty_ldisc_ops *new_ldisc`
:   pointer to the ldisc object

**Description**

Installs a new line discipline into the kernel. The discipline is set up as
unreferenced and then made available to the kernel from this point onwards.

Locking: takes `tty_ldiscs_lock` to guard against ldisc races

void tty\_unregister\_ldisc(const struct [tty\_ldisc\_ops](#c.tty_ldisc_ops "tty_ldisc_ops") \*ldisc)
:   unload a line discipline

**Parameters**

`const struct tty_ldisc_ops *ldisc`
:   ldisc number

**Description**

Remove a line discipline from the kernel providing it is not currently in
use.

Locking: takes `tty_ldiscs_lock` to guard against ldisc races

## [Other Functions](#id2)

void tty\_ldisc\_flush(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   flush line discipline queue

**Parameters**

`struct tty_struct *tty`
:   tty to flush ldisc for

**Description**

Flush the line discipline queue (if any) and the tty flip buffers for this
**tty**.

int tty\_set\_ldisc(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int disc)
:   set line discipline

**Parameters**

`struct tty_struct *tty`
:   the terminal to set

`int disc`
:   the line discipline number

**Description**

Set the discipline of a tty line. Must be called from a process context. The
ldisc change logic has to protect itself against any overlapping ldisc
change (including on the other end of pty pairs), the close of one side of a
tty/pty pair, and eventually hangup.

## [Line Discipline Operations Reference](#id3)

struct tty\_ldisc\_ops
:   ldisc operations

**Definition**:

```
struct tty_ldisc_ops {
    char *name;
    int num;
    int (*open)(struct tty_struct *tty);
    void (*close)(struct tty_struct *tty);
    void (*flush_buffer)(struct tty_struct *tty);
    ssize_t (*read)(struct tty_struct *tty, struct file *file, u8 *buf, size_t nr, void **cookie, unsigned long offset);
    ssize_t (*write)(struct tty_struct *tty, struct file *file, const u8 *buf, size_t nr);
    int (*ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);
    int (*compat_ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);
    void (*set_termios)(struct tty_struct *tty, const struct ktermios *old);
    __poll_t (*poll)(struct tty_struct *tty, struct file *file, struct poll_table_struct *wait);
    void (*hangup)(struct tty_struct *tty);
    void (*receive_buf)(struct tty_struct *tty, const u8 *cp, const u8 *fp, size_t count);
    void (*write_wakeup)(struct tty_struct *tty);
    void (*dcd_change)(struct tty_struct *tty, bool active);
    size_t (*receive_buf2)(struct tty_struct *tty, const u8 *cp, const u8 *fp, size_t count);
    void (*lookahead_buf)(struct tty_struct *tty, const u8 *cp, const u8 *fp, size_t count);
    struct module *owner;
};
```

**Members**

`name`
:   name of this ldisc rendered in /proc/tty/ldiscs

`num`
:   `N_*` number (`N_TTY`, `N_HDLC`, ...) reserved to this ldisc

`open`
:   [TTY] `int ()(struct tty_struct *tty)`

`close`
:   [TTY] `void ()(struct tty_struct *tty)`

`flush_buffer`
:   [TTY] `void ()(struct tty_struct *tty)`

`read`
:   [TTY] `ssize_t ()(struct tty_struct *tty, struct file *file, u8 *buf,
    size_t nr)`

`write`
:   [TTY] `ssize_t ()(struct tty_struct *tty, struct file *file,
    const u8 *buf, size_t nr)`

`ioctl`
:   [TTY] `int ()(struct tty_struct *tty, unsigned int cmd,
    unsigned long arg)`

`compat_ioctl`
:   [TTY] `int ()(struct tty_struct *tty, unsigned int cmd,
    unsigned long arg)`

`set_termios`
:   [TTY] `void ()(struct tty_struct *tty, const struct ktermios *old)`

`poll`
:   [TTY] `int ()(struct tty_struct *tty, struct file *file,
    struct poll_table_struct *wait)`

`hangup`
:   [TTY] `void ()(struct tty_struct *tty)`

`receive_buf`
:   [DRV] `void ()(struct tty_struct *tty, const u8 *cp,
    const u8 *fp, size_t count)`

`write_wakeup`
:   [DRV] `void ()(struct tty_struct *tty)`

`dcd_change`
:   [DRV] `void ()(struct tty_struct *tty, bool active)`

`receive_buf2`
:   [DRV] `ssize_t ()(struct tty_struct *tty, const u8 *cp,
    const u8 *fp, size_t count)`

`lookahead_buf`
:   [DRV] `void ()(struct tty_struct *tty, const u8 *cp,
    const u8 *fp, size_t count)`

`owner`
:   module containting this ldisc (for reference counting)

**Description**

> This function is called when the line discipline is associated with the
> **tty**. No other call into the line discipline for this tty will occur
> until it completes successfully. It should initialize any state needed
> by the ldisc, and set **tty->receive\_room** to the maximum amount of data
> the line discipline is willing to accept from the driver with a single
> call to **`receive_buf()`**. Returning an error will prevent the ldisc from
> being attached.
>
> Optional. Can sleep.
>
> This function is called when the line discipline is being shutdown,
> either because the **tty** is being closed or because the **tty** is being
> changed to use a new line discipline. At the point of execution no
> further users will enter the ldisc code for this tty.
>
> Optional. Can sleep.
>
> This function instructs the line discipline to clear its buffers of any
> input characters it may have queued to be delivered to the user mode
> process. It may be called at any point between open and close.
>
> Optional.
>
> This function is called when the user requests to read from the **tty**.
> The line discipline will return whatever characters it has buffered up
> for the user. If this function is not defined, the user will receive
> an `EIO` error. Multiple read calls may occur in parallel and the ldisc
> must deal with serialization issues.
>
> Optional: `EIO` unless provided. Can sleep.
>
> This function is called when the user requests to write to the **tty**.
> The line discipline will deliver the characters to the low-level tty
> device for transmission, optionally performing some processing on the
> characters first. If this function is not defined, the user will
> receive an `EIO` error.
>
> Optional: `EIO` unless provided. Can sleep.
>
> This function is called when the user requests an ioctl which is not
> handled by the tty layer or the low-level tty driver. It is intended
> for ioctls which affect line discpline operation. Note that the search
> order for ioctls is (1) tty layer, (2) tty low-level driver, (3) line
> discpline. So a low-level driver can “grab” an ioctl request before
> the line discpline has a chance to see it.
>
> Optional.
>
> Process ioctl calls from 32-bit process on 64-bit system.
>
> Note that only ioctls that are neither “pointer to compatible
> structure” nor tty-generic. Something private that takes an integer or
> a pointer to wordsize-sensitive structure belongs here, but most of
> ldiscs will happily leave it `NULL`.
>
> Optional.
>
> This function notifies the line discpline that a change has been made
> to the termios structure.
>
> Optional.
>
> This function is called when a user attempts to select/poll on a **tty**
> device. It is solely the responsibility of the line discipline to
> handle poll requests.
>
> Optional.
>
> Called on a hangup. Tells the discipline that it should cease I/O to
> the tty driver. The driver should seek to perform this action quickly
> but should wait until any pending driver I/O is completed. No further
> calls into the ldisc code will occur.
>
> Optional. Can sleep.
>
> This function is called by the low-level tty driver to send characters
> received by the hardware to the line discpline for processing. **cp** is
> a pointer to the buffer of input character received by the device. **fp**
> is a pointer to an array of flag bytes which indicate whether a
> character was received with a parity error, etc. **fp** may be `NULL` to
> indicate all data received is `TTY_NORMAL`.
>
> Optional.
>
> This function is called by the low-level tty driver to signal that line
> discpline should try to send more characters to the low-level driver
> for transmission. If the line discpline does not have any more data to
> send, it can just return. If the line discipline does have some data to
> send, please arise a tasklet or workqueue to do the real data transfer.
> Do not send data in this hook, it may lead to a deadlock.
>
> Optional.
>
> Tells the discipline that the DCD pin has changed its status. Used
> exclusively by the `N_PPS` (Pulse-Per-Second) line discipline.
>
> Optional.
>
> This function is called by the low-level tty driver to send characters
> received by the hardware to the line discpline for processing. **cp** is a
> pointer to the buffer of input character received by the device. **fp**
> is a pointer to an array of flag bytes which indicate whether a
> character was received with a parity error, etc. **fp** may be `NULL` to
> indicate all data received is `TTY_NORMAL`. If assigned, prefer this
> function for automatic flow control.
>
> Optional.
>
> This function is called by the low-level tty driver for characters
> not eaten by ->`receive_buf()` or ->`receive_buf2()`. It is useful for
> processing high-priority characters such as software flow-control
> characters that could otherwise get stuck into the intermediate
> buffer until tty has room to receive them. Ldisc must be able to
> handle later a ->`receive_buf()` or ->`receive_buf2()` call for the
> same characters (e.g. by skipping the actions for high-priority
> characters already handled by ->`lookahead_buf()`).
>
> Optional.

This structure defines the interface between the tty line discipline
implementation and the tty routines. The above routines can be defined.
Unless noted otherwise, they are optional, and can be filled in with a `NULL`
pointer.

Hooks marked [TTY] are invoked from the TTY core, the [DRV] ones from the
tty\_driver side.

## [Driver Access](#id4)

Line discipline methods can call the methods of the underlying hardware driver.
These are documented as a part of [`struct tty_operations`](tty_driver.html#c.tty_operations "tty_operations").

## [TTY Flags](#id5)

Line discipline methods have access to `tty_struct.flags` field. See
[TTY Struct](tty_struct.html).

## [Locking](#id6)

Callers to the line discipline functions from the tty layer are required to
take line discipline locks. The same is true of calls from the driver side
but not yet enforced.

struct tty\_ldisc \*tty\_ldisc\_ref\_wait(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   wait for the tty ldisc

**Parameters**

`struct tty_struct *tty`
:   tty device

**Description**

Dereference the line discipline for the terminal and take a reference to it.
If the line discipline is in flux then wait patiently until it changes.

Note 1: Must not be called from an IRQ/timer context. The caller must also
be careful not to hold other locks that will deadlock against a discipline
change, such as an existing ldisc reference (which we check for).

Note 2: a file\_operations routine (read/poll/write) should use this function
to wait for any ldisc lifetime events to finish.

**Return**

`NULL` if the tty has been hungup and not re-opened with a new file
descriptor, otherwise valid ldisc reference

struct tty\_ldisc \*tty\_ldisc\_ref(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   get the tty ldisc

**Parameters**

`struct tty_struct *tty`
:   tty device

**Description**

Dereference the line discipline for the terminal and take a reference to it.
If the line discipline is in flux then return `NULL`. Can be called from IRQ
and timer functions.

void tty\_ldisc\_deref(struct tty\_ldisc \*ld)
:   free a tty ldisc reference

**Parameters**

`struct tty_ldisc *ld`
:   reference to free up

**Description**

Undoes the effect of [`tty_ldisc_ref()`](#c.tty_ldisc_ref "tty_ldisc_ref") or [`tty_ldisc_ref_wait()`](#c.tty_ldisc_ref_wait "tty_ldisc_ref_wait"). May be called
in IRQ context.

While these functions are slightly slower than the old code they should have
minimal impact as most receive logic uses the flip buffers and they only
need to take a reference when they push bits up through the driver.

A caution: The `tty_ldisc_ops.open()`,
`tty_ldisc_ops.close()` and `tty_driver.set_ldisc()`
functions are called with the ldisc unavailable. Thus [`tty_ldisc_ref()`](#c.tty_ldisc_ref "tty_ldisc_ref") will fail
in this situation if used within these functions. Ldisc and driver code
calling its own functions must be careful in this case.

## [Internal Functions](#id7)

struct tty\_ldisc \*tty\_ldisc\_get(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int disc)
:   take a reference to an ldisc

**Parameters**

`struct tty_struct *tty`
:   tty device

`int disc`
:   ldisc number

**Description**

Takes a reference to a line discipline. Deals with refcounts and module
locking counts. If the discipline is not available, its module loaded, if
possible.

Locking: takes `tty_ldiscs_lock` to guard against ldisc races

**Return**

* -`EINVAL` if the discipline index is not [`N_TTY` .. `NR_LDISCS`] or if the
  discipline is not registered
* -`EAGAIN` if `request_module()` failed to load or register the discipline
* -`ENOMEM` if allocation failure
* Otherwise, returns a pointer to the discipline and bumps the ref count

void tty\_ldisc\_put(struct tty\_ldisc \*ld)
:   release the ldisc

**Parameters**

`struct tty_ldisc *ld`
:   lisdsc to release

**Description**

Complement of [`tty_ldisc_get()`](#c.tty_ldisc_get "tty_ldisc_get").

void tty\_set\_termios\_ldisc(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int disc)
:   set ldisc field

**Parameters**

`struct tty_struct *tty`
:   tty structure

`int disc`
:   line discipline number

**Description**

This is probably overkill for real world processors but they are not on hot
paths so a little discipline won’t do any harm.

The line discipline-related tty\_struct fields are reset to prevent the ldisc
driver from re-using stale information for the new ldisc instance.

Locking: takes termios\_rwsem

int tty\_ldisc\_open(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct tty\_ldisc \*ld)
:   open a line discipline

**Parameters**

`struct tty_struct *tty`
:   tty we are opening the ldisc on

`struct tty_ldisc *ld`
:   discipline to open

**Description**

A helper opening method. Also a convenient debugging and check point.

Locking: always called with BTM already held.

void tty\_ldisc\_close(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct tty\_ldisc \*ld)
:   close a line discipline

**Parameters**

`struct tty_struct *tty`
:   tty we are opening the ldisc on

`struct tty_ldisc *ld`
:   discipline to close

**Description**

A helper close method. Also a convenient debugging and check point.

int tty\_ldisc\_failto(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int ld)
:   helper for ldisc failback

**Parameters**

`struct tty_struct *tty`
:   tty to open the ldisc on

`int ld`
:   ldisc we are trying to fail back to

**Description**

Helper to try and recover a tty when switching back to the old ldisc fails
and we need something attached.

void tty\_ldisc\_restore(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct tty\_ldisc \*old)
:   helper for tty ldisc change

**Parameters**

`struct tty_struct *tty`
:   tty to recover

`struct tty_ldisc *old`
:   previous ldisc

**Description**

Restore the previous line discipline or `N_TTY` when a line discipline change
fails due to an open error

void tty\_ldisc\_kill(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   teardown ldisc

**Parameters**

`struct tty_struct *tty`
:   tty being released

**Description**

Perform final close of the ldisc and reset **tty->ldisc**

void tty\_reset\_termios(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   reset terminal state

**Parameters**

`struct tty_struct *tty`
:   tty to reset

**Description**

Restore a terminal to the driver default state.

int tty\_ldisc\_reinit(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int disc)
:   reinitialise the tty ldisc

**Parameters**

`struct tty_struct *tty`
:   tty to reinit

`int disc`
:   line discipline to reinitialize

**Description**

Completely reinitialize the line discipline state, by closing the current
instance, if there is one, and opening a new instance. If an error occurs
opening the new non-`N_TTY` instance, the instance is dropped and **tty->ldisc**
reset to `NULL`. The caller can then retry with `N_TTY` instead.

**Return**

0 if successful, otherwise error code < 0

void tty\_ldisc\_hangup(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, bool reinit)
:   hangup ldisc reset

**Parameters**

`struct tty_struct *tty`
:   tty being hung up

`bool reinit`
:   whether to re-initialise the tty

**Description**

Some tty devices reset their termios when they receive a hangup event. In
that situation we must also switch back to `N_TTY` properly before we reset
the termios data.

Locking: We can take the ldisc mutex as the rest of the code is careful to
allow for this.

In the pty pair case this occurs in the close() path of the tty itself so we
must be careful about locking rules.

int tty\_ldisc\_setup(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*o\_tty)
:   open line discipline

**Parameters**

`struct tty_struct *tty`
:   tty being shut down

`struct tty_struct *o_tty`
:   pair tty for pty/tty pairs

**Description**

Called during the initial open of a tty/pty pair in order to set up the line
disciplines and bind them to the **tty**. This has no locking issues as the
device isn’t yet active.

void tty\_ldisc\_release(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   release line discipline

**Parameters**

`struct tty_struct *tty`
:   tty being shut down (or one end of pty pair)

**Description**

Called during the final close of a tty or a pty pair in order to shut down
the line discpline layer. On exit, each tty’s ldisc is `NULL`.

int tty\_ldisc\_init(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   ldisc setup for new tty

**Parameters**

`struct tty_struct *tty`
:   tty being allocated

**Description**

Set up the line discipline objects for a newly allocated tty. Note that the
tty structure is not completely set up when this call is made.

void tty\_ldisc\_deinit(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   ldisc cleanup for new tty

**Parameters**

`struct tty_struct *tty`
:   tty that was allocated recently

**Description**

The tty structure must not be completely set up ([`tty_ldisc_setup()`](#c.tty_ldisc_setup "tty_ldisc_setup")) when
this call is made.
