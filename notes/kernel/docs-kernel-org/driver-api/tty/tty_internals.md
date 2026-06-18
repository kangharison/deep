# TTY Internals

> 출처(원문): https://docs.kernel.org/driver-api/tty/tty_internals.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY Internals

## [Kopen](#id1)

These functions serve for opening a TTY from the kernelspace:

void tty\_kclose(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   closes tty opened by tty\_kopen

**Parameters**

`struct tty_struct *tty`
:   tty device

**Description**

Performs the final steps to release and free a tty device. It is the same as
[`tty_release_struct()`](#c.tty_release_struct "tty_release_struct") except that it also resets `TTY_PORT_KOPENED` flag on
**tty->port**.

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty\_kopen\_exclusive(dev\_t device)
:   open a tty device for kernel

**Parameters**

`dev_t device`
:   dev\_t of device to open

**Description**

Opens tty exclusively for kernel. Performs the driver lookup, makes sure
it’s not already opened and performs the first-time tty initialization.

Claims the global `tty_mutex` to serialize:
:   * concurrent first-time tty initialization
    * concurrent tty driver removal w/ lookup
    * concurrent tty removal from driver table

**Return**

the locked initialized [`tty_struct`](tty_struct.html#c.tty_struct "tty_struct")

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty\_kopen\_shared(dev\_t device)
:   open a tty device for shared in-kernel use

**Parameters**

`dev_t device`
:   dev\_t of device to open

**Description**

Opens an already existing tty for in-kernel use. Compared to
[`tty_kopen_exclusive()`](#c.tty_kopen_exclusive "tty_kopen_exclusive") above it doesn’t ensure to be the only user.

Locking: identical to `tty_kopen()` above.

---

## [Exported Internal Functions](#id2)

int tty\_dev\_name\_to\_number(const char \*name, dev\_t \*number)
:   return dev\_t for device name

**Parameters**

`const char *name`
:   user space name of device under /dev

`dev_t *number`
:   pointer to dev\_t that this function will populate

**Description**

This function converts device names like ttyS0 or ttyUSB1 into dev\_t like
(4, 64) or (188, 1). If no corresponding driver is registered then the
function returns -`ENODEV`.

Locking: this acquires tty\_mutex to protect the tty\_drivers list from
:   being modified while we are traversing it, and makes sure to
    release it before exiting.

void tty\_release\_struct(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int idx)
:   release a tty struct

**Parameters**

`struct tty_struct *tty`
:   tty device

`int idx`
:   index of the tty

**Description**

Performs the final steps to release and free a tty device. It is roughly the
reverse of [`tty_init_dev()`](#c.tty_init_dev "tty_init_dev").

int tty\_get\_icount(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct serial\_icounter\_struct \*icount)
:   get tty statistics

**Parameters**

`struct tty_struct *tty`
:   tty device

`struct serial_icounter_struct *icount`
:   output parameter

**Description**

Gets a copy of the **tty**’s icount statistics.

Locking: none (up to the driver)

---

## [Internal Functions](#id3)

void free\_tty\_struct(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   free a disused tty

**Parameters**

`struct tty_struct *tty`
:   tty `struct to` free

**Description**

Free the write buffers, tty queue and tty memory itself.

Locking: none. Must be called after tty is definitely unused

void tty\_free\_file(struct [file](#c.tty_free_file "file") \*file)
:   free file->private\_data

**Parameters**

`struct file *file`
:   to free private\_data of

**Description**

This shall be used only for fail path handling when tty\_add\_file was not
called yet.

struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*get\_tty\_driver(dev\_t device, int \*index)
:   find device of a tty

**Parameters**

`dev_t device`
:   device identifier

`int *index`
:   returns the index of the tty

**Description**

This routine returns a tty driver structure, given a device number and also
passes back the index number.

Locking: caller must hold tty\_mutex

struct [file](../../filesystems/api-summary.html#c.file "file") \*tty\_release\_redirect(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   Release a redirect on a pty if present

**Parameters**

`struct tty_struct *tty`
:   tty device

**Description**

This is available to the pty code so if the master closes, if the slave is a
redirect it can release the redirect.

void \_\_tty\_hangup(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int exit\_session)
:   actual handler for hangup events

**Parameters**

`struct tty_struct *tty`
:   tty device

`int exit_session`
:   if non-zero, signal all foreground group processes

**Description**

This can be called by a “kworker” kernel thread. That is process synchronous
but doesn’t hold any locks, so we need to make sure we have the appropriate
locks for what we’re doing.

The hangup event clears any pending redirections onto the hung up device. It
ensures future writes will error and it does the needed line discipline
hangup and signal delivery. The tty object itself remains intact.

Locking:
:   * BTM

    > * redirect lock for undoing redirection
    > * file list lock for manipulating list of ttys
    > * tty\_ldiscs\_lock from called functions
    > * termios\_rwsem resetting termios data
    > * tasklist\_lock to walk task list for hangup event
    >
    > > * ->siglock to protect ->signal/->sighand

void tty\_vhangup\_self(void)
:   process vhangup for own ctty

**Parameters**

`void`
:   no arguments

**Description**

Perform a vhangup on the current controlling tty

void tty\_vhangup\_session(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   hangup session leader exit

**Parameters**

`struct tty_struct *tty`
:   tty to hangup

**Description**

The session leader is exiting and hanging up its controlling terminal.
Every process in the foreground process group is signalled `SIGHUP`.

We do this synchronously so that when the syscall returns the process is
complete. That guarantee is necessary for security reasons.

ssize\_t tty\_read(struct kiocb \*iocb, struct iov\_iter \*to)
:   read method for tty device files

**Parameters**

`struct kiocb *iocb`
:   kernel I/O control block

`struct iov_iter *to`
:   destination for the data read

**Description**

Perform the read system call function on this terminal device. Checks
for hung up devices before calling the line discipline method.

Locking:
:   Locks the line discipline internally while needed. Multiple read calls
    may be outstanding in parallel.

void tty\_write\_message(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, char \*msg)
:   write a message to a certain tty, not just the console.

**Parameters**

`struct tty_struct *tty`
:   the destination tty\_struct

`char *msg`
:   the message to write

**Description**

This is used for messages that need to be redirected to a specific tty. We
don’t put it into the syslog queue right now maybe in the future if really
needed.

We must still hold the BTM and test the CLOSING flag for the moment.

This function is DEPRECATED, do not use in new code.

ssize\_t tty\_write(struct kiocb \*iocb, struct iov\_iter \*from)
:   write method for tty device file

**Parameters**

`struct kiocb *iocb`
:   kernel I/O control block

`struct iov_iter *from`
:   iov\_iter with data to write

**Description**

Write data to a tty device via the line discipline.

Locking:
:   Locks the line discipline as required
    Writes to the tty driver are serialized by the atomic\_write\_lock
    and are then processed in chunks to the device. The line
    discipline write method will not be invoked in parallel for
    each device.

int tty\_send\_xchar(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, u8 ch)
:   send priority character

**Parameters**

`struct tty_struct *tty`
:   the tty to send to

`u8 ch`
:   xchar to send

**Description**

Send a high priority character to the tty even if stopped.

Locking: none for xchar method, write ordering for write method.

void pty\_line\_name(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, int index, char \*p)
:   generate name for a pty

**Parameters**

`struct tty_driver *driver`
:   the tty driver in use

`int index`
:   the minor number

`char *p`
:   output buffer of at least 6 bytes

**Description**

Generate a name from a **driver** reference and write it to the output buffer
**p**.

Locking: None

ssize\_t tty\_line\_name(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, int index, char \*p)
:   generate name for a tty

**Parameters**

`struct tty_driver *driver`
:   the tty driver in use

`int index`
:   the minor number

`char *p`
:   output buffer of at least 7 bytes

**Description**

Generate a name from a **driver** reference and write it to the output buffer
**p**.

Locking: None

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty\_driver\_lookup\_tty(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, struct [file](#c.tty_driver_lookup_tty "file") \*file, int idx)
:   find an existing tty, if any

**Parameters**

`struct tty_driver *driver`
:   the driver for the tty

`struct file *file`
:   file object

`int idx`
:   the minor number

**Return**

the tty, if found. If not found, return `NULL` or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") if the
driver `lookup()` method returns an error.

**Description**

Locking: tty\_mutex must be held. If the tty is found, bump the tty kref.

int tty\_driver\_install\_tty(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   install a tty entry in the driver

**Parameters**

`struct tty_driver *driver`
:   the driver for the tty

`struct tty_struct *tty`
:   the tty

**Description**

Install a tty object into the driver tables. The **tty->index** field will be
set by the time this is called. This method is responsible for ensuring any
need additional structures are allocated and configured.

Locking: tty\_mutex for now

void tty\_driver\_remove\_tty(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   remove a tty from the driver tables

**Parameters**

`struct tty_driver *driver`
:   the driver for the tty

`struct tty_struct *tty`
:   tty to remove

**Description**

Remove a tty object from the driver tables. The tty->index field will be set
by the time this is called.

Locking: tty\_mutex for now

int tty\_reopen(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   fast re-open of an open tty

**Parameters**

`struct tty_struct *tty`
:   the tty to open

**Description**

Re-opens on master ptys are not allowed and return -`EIO`.

Locking: Caller must hold tty\_lock

**Return**

0 on success, -errno on error.

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty\_init\_dev(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, int idx)
:   initialise a tty device

**Parameters**

`struct tty_driver *driver`
:   tty driver we are opening a device on

`int idx`
:   device index

**Description**

Prepare a tty device. This may not be a “new” clean device but could also be
an active device. The pty drivers require special handling because of this.

Locking:
:   The function is called under the tty\_mutex, which protects us from the
    tty `struct or` driver itself going away.

On exit the tty device has the line discipline attached and a reference
count of 1. If a pair was created for pty/tty use and the other was a pty
master then it too has a reference count of 1.

WSH 06/09/97: Rewritten to remove races and properly clean up after a failed
open. The new code protects the open with a mutex, so it’s really quite
straightforward. The mutex locking can probably be relaxed for the (most
common) case of reopening a tty.

**Return**

new tty structure

void tty\_flush\_works(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   flush all works of a tty/pty pair

**Parameters**

`struct tty_struct *tty`
:   tty device to flush works for (or either end of a pty pair)

**Description**

Sync flush all works belonging to **tty** (and the ‘other’ tty).

void release\_one\_tty(struct work\_struct \*work)
:   release tty structure memory

**Parameters**

`struct work_struct *work`
:   work of tty we are obliterating

**Description**

Releases memory associated with a tty structure, and clears out the
driver table slots. This function is called when a device is no longer
in use. It also gets called when setup of a device fails.

Locking:
:   takes the file list lock internally when working on the list of ttys
    that the driver keeps.

This method gets called from a work queue so that the driver private
cleanup ops can sleep (needed for USB at least)

void release\_tty(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int idx)
:   release tty structure memory

**Parameters**

`struct tty_struct *tty`
:   tty device release

`int idx`
:   index of the tty device release

**Description**

Release both **tty** and a possible linked partner (think pty pair),
and decrement the refcount of the backing module.

Locking:
:   tty\_mutex
    takes the file list lock internally when working on the list of ttys
    that the driver keeps.

int tty\_release\_checks(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int idx)
:   check a tty before real release

**Parameters**

`struct tty_struct *tty`
:   tty to check

`int idx`
:   index of the tty

**Description**

Performs some paranoid checking before true release of the **tty**. This is a
no-op unless `TTY_PARANOIA_CHECK` is defined.

int tty\_release(struct [inode](#c.tty_release "inode") \*inode, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   vfs callback for close

**Parameters**

`struct inode *inode`
:   inode of tty

`struct file *filp`
:   file pointer for handle to tty

**Description**

Called the last time each file handle is closed that references this tty.
There may however be several such references.

Locking:
:   Takes BKL. See `tty_release_dev()`.

Even releasing the tty structures is a tricky business. We have to be very
careful that the structures are all released at the same time, as interrupts
might otherwise get the wrong pointers.

WSH 09/09/97: rewritten to avoid some nasty race conditions that could
lead to double frees or releasing memory still in use.

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty\_open\_current\_tty(dev\_t device, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   get locked tty of current task

**Parameters**

`dev_t device`
:   device number

`struct file *filp`
:   file pointer to tty

**Return**

locked tty of the current task iff **device** is /dev/tty

**Description**

Performs a re-open of the current task’s controlling tty.

We cannot return driver and index like for the other nodes because devpts
will not work then. It expects inodes to be from devpts FS.

struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*tty\_lookup\_driver(dev\_t device, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, int \*index)
:   lookup a tty driver for a given device file

**Parameters**

`dev_t device`
:   device number

`struct file *filp`
:   file pointer to tty

`int *index`
:   index for the device in the **return** driver

**Description**

If returned value is not erroneous, the caller is responsible to decrement
the refcount by [`tty_driver_kref_put()`](tty_driver.html#c.tty_driver_kref_put "tty_driver_kref_put").

Locking: `tty_mutex` protects [`get_tty_driver()`](#c.get_tty_driver "get_tty_driver")

**Return**

driver for this inode (with increased refcount)

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty\_open\_by\_driver(dev\_t device, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   open a tty device

**Parameters**

`dev_t device`
:   dev\_t of device to open

`struct file *filp`
:   file pointer to tty

**Description**

Performs the driver lookup, checks for a reopen, or otherwise performs the
first-time tty initialization.

Claims the global tty\_mutex to serialize:
:   * concurrent first-time tty initialization
    * concurrent tty driver removal w/ lookup
    * concurrent tty removal from driver table

**Return**

the locked initialized or re-opened [`tty_struct`](tty_struct.html#c.tty_struct "tty_struct")

int tty\_open(struct [inode](#c.tty_open "inode") \*inode, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   open a tty device

**Parameters**

`struct inode *inode`
:   inode of device file

`struct file *filp`
:   file pointer to tty

**Description**

[`tty_open()`](#c.tty_open "tty_open") and [`tty_release()`](#c.tty_release "tty_release") keep up the tty count that contains the number
of opens done on a tty. We cannot use the inode-count, as different inodes
might point to the same tty.

Open-counting is needed for pty masters, as well as for keeping track of
serial lines: DTR is dropped when the last close happens.
(This is not done solely through tty->count, now. - Ted 1/27/92)

The termios state of a pty is reset on the first open so that settings don’t
persist across reuse.

Locking:
:   * `tty_mutex` protects tty, [`tty_lookup_driver()`](#c.tty_lookup_driver "tty_lookup_driver") and [`tty_init_dev()`](#c.tty_init_dev "tty_init_dev").
    * **tty->count** should protect the rest.
    * ->siglock protects ->signal/->sighand

**Note**

the tty\_unlock/lock cases without a ref are only safe due to `tty_mutex`

\_\_poll\_t tty\_poll(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, poll\_table \*wait)
:   check tty status

**Parameters**

`struct file *filp`
:   file being polled

`poll_table *wait`
:   poll wait structures to update

**Description**

Call the line discipline polling method to obtain the poll status of the
device.

Locking: locks called line discipline but ldisc poll method may be
re-entered freely by other callers.

int tiocsti(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, u8 \_\_user \*p)
:   fake input character

**Parameters**

`struct tty_struct *tty`
:   tty to fake input into

`u8 __user *p`
:   pointer to character

**Description**

Fake input to a tty device. Does the necessary locking and input management.

FIXME: does not honour flow control ??

Locking:
:   * Called functions take tty\_ldiscs\_lock
    * current->signal->tty check is safe without locks

int tiocgwinsz(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct winsize \_\_user \*arg)
:   implement window query ioctl

**Parameters**

`struct tty_struct *tty`
:   tty

`struct winsize __user *arg`
:   user buffer for result

**Description**

Copies the kernel idea of the window size into the user buffer.

Locking: **tty->winsize\_mutex** is taken to ensure the winsize data is
consistent.

int tiocswinsz(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct winsize \_\_user \*arg)
:   implement window size set ioctl

**Parameters**

`struct tty_struct *tty`
:   tty side of tty

`struct winsize __user *arg`
:   user buffer for result

**Description**

Copies the user idea of the window size to the kernel. Traditionally this is
just advisory information but for the Linux console it actually has driver
level meaning and triggers a VC resize.

Locking:
:   Driver dependent. The default do\_resize method takes the tty termios
    mutex and ctrl.lock. The console takes its own lock then calls into the
    default method.

int tioccons(struct [file](#c.tioccons "file") \*file)
:   allow admin to move logical console

**Parameters**

`struct file *file`
:   the file to become console

**Description**

Allow the administrator to move the redirected console device.

Locking: uses redirect\_lock to guard the redirect information

int tiocsetd(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int \_\_user \*p)
:   set line discipline

**Parameters**

`struct tty_struct *tty`
:   tty device

`int __user *p`
:   pointer to user data

**Description**

Set the line discipline according to user request.

Locking: see [`tty_set_ldisc()`](tty_ldisc.html#c.tty_set_ldisc "tty_set_ldisc"), this function is just a helper

int tiocgetd(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int \_\_user \*p)
:   get line discipline

**Parameters**

`struct tty_struct *tty`
:   tty device

`int __user *p`
:   pointer to user data

**Description**

Retrieves the line discipline id directly from the ldisc.

Locking: waits for ldisc reference (in case the line discipline is changing
or the **tty** is being hungup)

int send\_break(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, unsigned int duration)
:   performed time break

**Parameters**

`struct tty_struct *tty`
:   device to break on

`unsigned int duration`
:   timeout in mS

**Description**

Perform a timed break on hardware that lacks its own driver level timed
break functionality.

Locking:
:   **tty->atomic\_write\_lock** serializes

int tty\_tiocmget(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int \_\_user \*p)
:   get modem status

**Parameters**

`struct tty_struct *tty`
:   tty device

`int __user *p`
:   pointer to result

**Description**

Obtain the modem status bits from the tty driver if the feature is
supported. Return -`ENOTTY` if it is not available.

Locking: none (up to the driver)

int tty\_tiocmset(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, unsigned int cmd, unsigned \_\_user \*p)
:   set modem status

**Parameters**

`struct tty_struct *tty`
:   tty device

`unsigned int cmd`
:   command - clear bits, set bits or set all

`unsigned __user *p`
:   pointer to desired bits

**Description**

Set the modem status bits from the tty driver if the feature
is supported. Return -`ENOTTY` if it is not available.

Locking: none (up to the driver)

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*alloc\_tty\_struct(struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, int idx)
:   allocate a new tty

**Parameters**

`struct tty_driver *driver`
:   driver which will handle the returned tty

`int idx`
:   minor of the tty

**Description**

This subroutine allocates and initializes a tty structure.

Locking: none - **tty** in question is not exposed at this point
