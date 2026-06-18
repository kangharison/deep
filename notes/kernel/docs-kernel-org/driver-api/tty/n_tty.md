# N_TTY

> 출처(원문): https://docs.kernel.org/driver-api/tty/n_tty.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# N\_TTY

The default (and fallback) [TTY line discipline](tty_ldisc.html). It tries to
handle characters as per POSIX.

## [External Functions](#id1)

void n\_tty\_inherit\_ops(struct [tty\_ldisc\_ops](tty_ldisc.html#c.tty_ldisc_ops "tty_ldisc_ops") \*ops)
:   inherit N\_TTY methods

**Parameters**

`struct tty_ldisc_ops *ops`
:   [`struct tty_ldisc_ops`](tty_ldisc.html#c.tty_ldisc_ops "tty_ldisc_ops") where to save N\_TTY methods

**Description**

> Enables a ‘subclass’ line discipline to ‘inherit’ N\_TTY methods.

## [Internal Functions](#id2)

void n\_tty\_kick\_worker(const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   start input worker (if required)

**Parameters**

`const struct tty_struct *tty`
:   terminal

**Description**

Re-schedules the flip buffer work if it may have stopped.

Locking:
:   * Caller holds exclusive `termios_rwsem`, or
    * n\_tty\_read()/consumer path:
      :   holds non-exclusive `termios_rwsem`

void n\_tty\_write\_wakeup(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   asynchronous I/O notifier

**Parameters**

`struct tty_struct *tty`
:   tty device

**Description**

Required for the ptys, serial driver etc. since processes that attach
themselves to the master and rely on ASYNC IO must be woken up.

void put\_tty\_queue(u8 c, struct n\_tty\_data \*ldata)
:   add character to tty

**Parameters**

`u8 c`
:   character

`struct n_tty_data *ldata`
:   n\_tty data

**Description**

Add a character to the tty read\_buf queue.

Locking:
:   * n\_tty\_receive\_buf()/producer path:
      :   caller holds non-exclusive `termios_rwsem`

void reset\_buffer\_flags(struct n\_tty\_data \*ldata)
:   reset buffer state

**Parameters**

`struct n_tty_data *ldata`
:   line disc data to reset

**Description**

Reset the read buffer counters and clear the flags. Called from
[`n_tty_open()`](#c.n_tty_open "n_tty_open") and [`n_tty_flush_buffer()`](#c.n_tty_flush_buffer "n_tty_flush_buffer").

Locking:
:   * caller holds exclusive `termios_rwsem`, or
    * (locking is not required)

void n\_tty\_flush\_buffer(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   clean input queue

**Parameters**

`struct tty_struct *tty`
:   terminal device

**Description**

Flush the input buffer. Called when the tty layer wants the buffer flushed
(eg at hangup) or when the `N_TTY` line discipline internally has to clean
the pending queue (for example some signals).

Holds `termios_rwsem` to exclude producer/consumer while buffer indices are
reset.

Locking: `ctrl`.lock, exclusive `termios_rwsem`

int is\_utf8\_continuation(u8 c)
:   utf8 multibyte check

**Parameters**

`u8 c`
:   byte to check

**Return**

true if the utf8 character **c** is a multibyte continuation
character. We use this to correctly compute the on-screen size of the
character when printing.

int is\_continuation(u8 c, const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   multibyte check

**Parameters**

`u8 c`
:   byte to check

`const struct tty_struct *tty`
:   terminal device

**Return**

true if the utf8 character **c** is a multibyte continuation character
and the terminal is in unicode mode.

int do\_output\_char(u8 c, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, int space)
:   output one character

**Parameters**

`u8 c`
:   character (or partial unicode symbol)

`struct tty_struct *tty`
:   terminal device

`int space`
:   space available in tty driver write buffer

**Description**

This is a helper function that handles one output character (including
special characters like TAB, CR, LF, etc.), doing OPOST processing and
putting the results in the tty driver’s write buffer.

Note that Linux currently ignores TABDLY, CRDLY, VTDLY, FFDLY and NLDLY.
They simply aren’t relevant in the world today. If you ever need them, add
them here.

Locking: should be called under the `output_lock` to protect the column state
and space left in the buffer.

**Return**

the number of bytes of buffer space used or -1 if no space left.

int process\_output(u8 c, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   output post processor

**Parameters**

`u8 c`
:   character (or partial unicode symbol)

`struct tty_struct *tty`
:   terminal device

**Description**

Output one character with OPOST processing.

Locking: `output_lock` to protect column state and space left (also, this is
called from [`n_tty_write()`](#c.n_tty_write "n_tty_write") under the tty layer write lock).

**Return**

-1 when the output device is full and the character must be
retried.

ssize\_t process\_output\_block(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, const u8 \*buf, unsigned int nr)
:   block post processor

**Parameters**

`struct tty_struct *tty`
:   terminal device

`const u8 *buf`
:   character buffer

`unsigned int nr`
:   number of bytes to output

**Description**

Output a block of characters with OPOST processing.

This path is used to speed up block console writes, among other things when
processing blocks of output data. It handles only the simple cases normally
found and helps to generate blocks of symbols for the console driver and
thus improve performance.

Locking: `output_lock` to protect column state and space left (also, this is
called from [`n_tty_write()`](#c.n_tty_write "n_tty_write") under the tty layer write lock).

**Return**

the number of characters output.

size\_t \_\_process\_echoes(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   write pending echo characters

**Parameters**

`struct tty_struct *tty`
:   terminal device

**Description**

Write previously buffered echo (and other ldisc-generated) characters to the
tty.

Characters generated by the ldisc (including echoes) need to be buffered
because the driver’s write buffer can fill during heavy program output.
Echoing straight to the driver will often fail under these conditions,
causing lost characters and resulting mismatches of ldisc state information.

Since the ldisc state must represent the characters actually sent to the
driver at the time of the write, operations like certain changes in column
state are also saved in the buffer and executed here.

A circular fifo buffer is used so that the most recent characters are
prioritized. Also, when control characters are echoed with a prefixed “^”,
the pair is treated atomically and thus not separated.

Locking: callers must hold `output_lock`.

void add\_echo\_byte(u8 c, struct n\_tty\_data \*ldata)
:   add a byte to the echo buffer

**Parameters**

`u8 c`
:   unicode byte to echo

`struct n_tty_data *ldata`
:   n\_tty data

**Description**

Add a character or operation byte to the echo buffer.

void echo\_move\_back\_col(struct n\_tty\_data \*ldata)
:   add operation to move back a column

**Parameters**

`struct n_tty_data *ldata`
:   n\_tty data

**Description**

Add an operation to the echo buffer to move back one column.

void echo\_set\_canon\_col(struct n\_tty\_data \*ldata)
:   add operation to set the canon column

**Parameters**

`struct n_tty_data *ldata`
:   n\_tty data

**Description**

Add an operation to the echo buffer to set the canon column to the current
column.

void echo\_erase\_tab(unsigned int num\_chars, int after\_tab, struct n\_tty\_data \*ldata)
:   add operation to erase a tab

**Parameters**

`unsigned int num_chars`
:   number of character columns already used

`int after_tab`
:   true if num\_chars starts after a previous tab

`struct n_tty_data *ldata`
:   n\_tty data

**Description**

Add an operation to the echo buffer to erase a tab.

Called by the eraser function, which knows how many character columns have
been used since either a previous tab or the start of input. This
information will be used later, along with canon column (if applicable), to
go back the correct number of columns.

void echo\_char\_raw(u8 c, struct n\_tty\_data \*ldata)
:   echo a character raw

**Parameters**

`u8 c`
:   unicode byte to echo

`struct n_tty_data *ldata`
:   line disc data

**Description**

Echo user input back onto the screen. This must be called only when
L\_ECHO(tty) is true. Called from the [`tty_driver.receive_buf()`](tty_driver.html#c.tty_driver "tty_driver") path.

This variant does not treat control characters specially.

void echo\_char(u8 c, const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   echo a character

**Parameters**

`u8 c`
:   unicode byte to echo

`const struct tty_struct *tty`
:   terminal device

**Description**

Echo user input back onto the screen. This must be called only when
L\_ECHO(tty) is true. Called from the [`tty_driver.receive_buf()`](tty_driver.html#c.tty_driver "tty_driver") path.

This variant tags control characters to be echoed as “^X” (where X is the
letter representing the control char).

void finish\_erasing(struct n\_tty\_data \*ldata)
:   complete erase

**Parameters**

`struct n_tty_data *ldata`
:   n\_tty data

void eraser(u8 c, const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   handle erase function

**Parameters**

`u8 c`
:   character input

`const struct tty_struct *tty`
:   terminal device

**Description**

Perform erase and necessary output when an erase character is present in the
stream from the driver layer. Handles the complexities of UTF-8 multibyte
symbols.

Locking: n\_tty\_receive\_buf()/producer path:
:   caller holds non-exclusive `termios_rwsem`

void isig(int sig, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   handle the ISIG optio

**Parameters**

`int sig`
:   signal

`struct tty_struct *tty`
:   terminal

**Description**

Called when a signal is being sent due to terminal input. Called from the
[`tty_driver.receive_buf()`](tty_driver.html#c.tty_driver "tty_driver") path, so serialized.

Performs input and output flush if !NOFLSH. In this context, the echo
buffer is ‘output’. The signal is processed first to alert any current
readers or writers to discontinue and exit their i/o loops.

Locking: `ctrl`.lock

void n\_tty\_receive\_break(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   handle break

**Parameters**

`struct tty_struct *tty`
:   terminal

**Description**

An RS232 break event has been hit in the incoming bitstream. This can cause
a variety of events depending upon the termios settings.

Locking: n\_tty\_receive\_buf()/producer path:
:   caller holds non-exclusive termios\_rwsem

**Note**

may get exclusive `termios_rwsem` if flushing input buffer

void n\_tty\_receive\_overrun(const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   handle overrun reporting

**Parameters**

`const struct tty_struct *tty`
:   terminal

**Description**

Data arrived faster than we could process it. While the tty driver has
flagged this the bits that were missed are gone forever.

Called from the receive\_buf path so single threaded. Does not need locking
as num\_overrun and overrun\_time are function private.

void n\_tty\_receive\_parity\_error(const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, u8 c)
:   error notifier

**Parameters**

`const struct tty_struct *tty`
:   terminal device

`u8 c`
:   character

**Description**

Process a parity error and queue the right data to indicate the error case
if necessary.

Locking: n\_tty\_receive\_buf()/producer path:
:   caller holds non-exclusive `termios_rwsem`

bool n\_tty\_receive\_char\_flow\_ctrl(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, u8 c, bool lookahead\_done)
:   receive flow control chars

**Parameters**

`struct tty_struct *tty`
:   terminal device

`u8 c`
:   character

`bool lookahead_done`
:   lookahead has processed this character already

**Description**

Receive and process flow control character actions.

In case lookahead for flow control chars already handled the character in
advance to the normal receive, the actions are skipped during normal
receive.

Returns true if **c** is consumed as flow-control character, the character
must not be treated as normal character.

void n\_tty\_receive\_char(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, u8 c)
:   perform processing

**Parameters**

`struct tty_struct *tty`
:   terminal device

`u8 c`
:   character

**Description**

Process an individual character of input received from the driver. This is
serialized with respect to itself by the rules for the driver above.

Locking: n\_tty\_receive\_buf()/producer path:
:   caller holds non-exclusive `termios_rwsem`
    publishes canon\_head if canonical mode is active

size\_t n\_tty\_receive\_buf\_common(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, const u8 \*cp, const u8 \*fp, size\_t count, bool flow)
:   process input

**Parameters**

`struct tty_struct *tty`
:   device to receive input

`const u8 *cp`
:   input chars

`const u8 *fp`
:   flags for each char (if `NULL`, all chars are `TTY_NORMAL`)

`size_t count`
:   number of input chars in **cp**

`bool flow`
:   enable flow control

**Description**

Called by the terminal driver when a block of characters has been received.
This function must be called from soft contexts not from interrupt context.
The driver is responsible for making calls one at a time and in order (or
using [`flush_to_ldisc()`](tty_buffer.html#c.flush_to_ldisc "flush_to_ldisc")).

In canonical mode, the maximum line length is 4096 chars (including the line
termination char); lines longer than 4096 chars are truncated. After 4095
chars, input data is still processed but not stored. Overflow processing
ensures the tty can always receive more input until at least one line can be
read.

In non-canonical mode, the read buffer will only accept 4095 chars; this
provides the necessary space for a newline char if the input mode is
switched to canonical.

Note it is possible for the read buffer to \_contain\_ 4096 chars in
non-canonical mode: the read buffer could already contain the maximum canon
line of 4096 chars when the mode is switched to non-canonical.

Locking: n\_tty\_receive\_buf()/producer path:
:   claims non-exclusive `termios_rwsem`
    publishes commit\_head or canon\_head

**Return**

the # of input chars from **cp** which were processed.

void n\_tty\_set\_termios(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, const struct ktermios \*old)
:   termios data changed

**Parameters**

`struct tty_struct *tty`
:   terminal

`const struct ktermios *old`
:   previous data

**Description**

Called by the tty layer when the user changes termios flags so that the line
discipline can plan ahead. This function cannot sleep and is protected from
re-entry by the tty layer. The user is guaranteed that this function will
not be re-entered or in progress when the ldisc is closed.

Locking: Caller holds **tty->termios\_rwsem**

void n\_tty\_close(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   close the ldisc for this tty

**Parameters**

`struct tty_struct *tty`
:   device

**Description**

Called from the terminal layer when this line discipline is being shut down,
either because of a close or becsuse of a discipline change. The function
will not be called while other ldisc methods are in progress.

int n\_tty\_open(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   open an ldisc

**Parameters**

`struct tty_struct *tty`
:   terminal to open

**Description**

Called when this line discipline is being attached to the terminal device.
Can sleep. Called serialized so that no other events will occur in parallel.
No further open will occur until a close.

bool copy\_from\_read\_buf(const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, u8 \*\*kbp, size\_t \*nr)
:   copy read data directly

**Parameters**

`const struct tty_struct *tty`
:   terminal device

`u8 **kbp`
:   data

`size_t *nr`
:   size of data

**Description**

Helper function to speed up [`n_tty_read()`](#c.n_tty_read "n_tty_read"). It is only called when `ICANON` is
off; it copies characters straight from the tty queue.

Locking:
:   * called under the **ldata->atomic\_read\_lock** sem
    * n\_tty\_read()/consumer path:
      :   caller holds non-exclusive `termios_rwsem`;
          read\_tail published

**Return**

true if it successfully copied data, but there is still more data
to be had.

bool canon\_copy\_from\_read\_buf(const struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, u8 \*\*kbp, size\_t \*nr)
:   copy read data in canonical mode

**Parameters**

`const struct tty_struct *tty`
:   terminal device

`u8 **kbp`
:   data

`size_t *nr`
:   size of data

**Description**

Helper function for [`n_tty_read()`](#c.n_tty_read "n_tty_read"). It is only called when `ICANON` is on; it
copies one line of input up to and including the line-delimiting character
into the result buffer.

**Note**

When termios is changed from non-canonical to canonical mode and the
read buffer contains data, [`n_tty_set_termios()`](#c.n_tty_set_termios "n_tty_set_termios") simulates an EOF push (as if
C-d were input) \_without\_ the `DISABLED_CHAR` in the buffer. This causes data
already processed as input to be immediately available as input although a
newline has not been received.

Locking:
:   * called under the `atomic_read_lock` mutex
    * n\_tty\_read()/consumer path:
      :   caller holds non-exclusive `termios_rwsem`;
          read\_tail published

int job\_control(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](#c.job_control "file") \*file)
:   check job control

**Parameters**

`struct tty_struct *tty`
:   tty

`struct file *file`
:   file handle

**Description**

Perform job control management checks on this **file**/**tty** descriptor and if
appropriate send any needed signals and return a negative error code if
action should be taken.

Locking:
:   * redirected write test is safe
    * current->signal->tty check is safe
    * ctrl.lock to safely reference **tty->ctrl.pgrp**

ssize\_t n\_tty\_read(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](#c.n_tty_read "file") \*file, u8 \*kbuf, size\_t nr, void \*\*cookie, unsigned long offset)
:   read function for tty

**Parameters**

`struct tty_struct *tty`
:   tty device

`struct file *file`
:   file object

`u8 *kbuf`
:   kernelspace buffer pointer

`size_t nr`
:   size of I/O

`void **cookie`
:   if non-`NULL`, this is a continuation read

`unsigned long offset`
:   where to continue reading from (unused in n\_tty)

**Description**

Perform reads for the line discipline. We are guaranteed that the line
discipline will not be closed under us but we may get multiple parallel
readers and must handle this ourselves. We may also get a hangup. Always
called in user context, may sleep.

This code must be sure never to sleep through a hangup.

Locking: n\_tty\_read()/consumer path:
:   claims non-exclusive termios\_rwsem;
    publishes read\_tail

ssize\_t n\_tty\_write(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](#c.n_tty_write "file") \*file, const u8 \*buf, size\_t nr)
:   write function for tty

**Parameters**

`struct tty_struct *tty`
:   tty device

`struct file *file`
:   file object

`const u8 *buf`
:   userspace buffer pointer

`size_t nr`
:   size of I/O

**Description**

Write function of the terminal device. This is serialized with respect to
other write callers but not to termios changes, reads and other such events.
Since the receive code will echo characters, thus calling driver write
methods, the `output_lock` is used in the output processing functions called
here as well as in the echo processing function to protect the column state
and space left in the buffer.

This code must be sure never to sleep through a hangup.

Locking: output\_lock to protect column state and space left
:   (note that the process\_output\*() functions take this lock themselves)

\_\_poll\_t n\_tty\_poll(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](#c.n_tty_poll "file") \*file, poll\_table \*wait)
:   poll method for N\_TTY

**Parameters**

`struct tty_struct *tty`
:   terminal device

`struct file *file`
:   file accessing it

`poll_table *wait`
:   poll table

**Description**

Called when the line discipline is asked to poll() for data or for special
events. This code is not serialized with respect to other events save
open/close.

This code must be sure never to sleep through a hangup.

Locking: called without the kernel lock held -- fine.
