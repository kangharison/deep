# TTY Buffer

> 출처(원문): https://docs.kernel.org/driver-api/tty/tty_buffer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY Buffer

Here, we document functions for taking care of tty buffer and their flipping.
Drivers are supposed to fill the buffer by one of those functions below and
then flip the buffer, so that the data are passed to [line discipline](tty_ldisc.html) for further processing.

## [Flip Buffer Management](#id1)

size\_t tty\_prepare\_flip\_string(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, u8 \*\*chars, size\_t size)
:   make room for characters

**Parameters**

`struct tty_port *port`
:   tty port

`u8 **chars`
:   return pointer for character write area

`size_t size`
:   desired size

**Description**

Prepare a block of space in the buffer for data.

This is used for drivers that need their own block copy routines into the
buffer. There is no guarantee the buffer is a DMA target!

**Return**

the length available and buffer pointer (**chars**) to the space which
is now allocated and accounted for as ready for normal characters.

size\_t tty\_ldisc\_receive\_buf(struct tty\_ldisc \*ld, const u8 \*p, const u8 \*f, size\_t count)
:   forward data to line discipline

**Parameters**

`struct tty_ldisc *ld`
:   line discipline to process input

`const u8 *p`
:   char buffer

`const u8 *f`
:   `TTY_NORMAL`, `TTY_BREAK`, etc. flags buffer

`size_t count`
:   number of bytes to process

**Description**

Callers other than [`flush_to_ldisc()`](#c.flush_to_ldisc "flush_to_ldisc") need to exclude the kworker from
concurrent use of the line discipline, see `paste_selection()`.

**Return**

the number of bytes processed.

void tty\_flip\_buffer\_push(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port)
:   push terminal buffers

**Parameters**

`struct tty_port *port`
:   tty port to push

**Description**

Queue a push of the terminal flip buffers to the line discipline. Can be
called from IRQ/atomic context.

In the event of the queue being busy for flipping the work will be held off
and retried later.

size\_t tty\_insert\_flip\_string\_fixed\_flag(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, const u8 \*chars, u8 flag, size\_t size)
:   add characters to the tty buffer

**Parameters**

`struct tty_port *port`
:   tty port

`const u8 *chars`
:   characters

`u8 flag`
:   flag value for each character

`size_t size`
:   size

**Description**

Queue a series of bytes to the tty buffering. All the characters passed are
marked with the supplied flag.

**Return**

the number added.

size\_t tty\_insert\_flip\_string\_flags(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, const u8 \*chars, const u8 \*flags, size\_t size)
:   add characters to the tty buffer

**Parameters**

`struct tty_port *port`
:   tty port

`const u8 *chars`
:   characters

`const u8 *flags`
:   flag bytes

`size_t size`
:   size

**Description**

Queue a series of bytes to the tty buffering. For each character the flags
array indicates the status of the character.

**Return**

the number added.

size\_t tty\_insert\_flip\_char(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, u8 ch, u8 flag)
:   add one character to the tty buffer

**Parameters**

`struct tty_port *port`
:   tty port

`u8 ch`
:   character

`u8 flag`
:   flag byte

**Description**

Queue a single byte **ch** to the tty buffering, with an optional flag.

---

## [Other Functions](#id2)

unsigned int tty\_buffer\_space\_avail(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port)
:   return unused buffer space

**Parameters**

`struct tty_port *port`
:   tty port owning the flip buffer

**Return**

the # of bytes which can be written by the driver without reaching
the buffer limit.

**Note**

this does not guarantee that memory is available to write the returned
# of bytes (use [`tty_prepare_flip_string()`](#c.tty_prepare_flip_string "tty_prepare_flip_string") to pre-allocate if memory
guarantee is required).

int tty\_buffer\_set\_limit(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, int limit)
:   change the tty buffer memory limit

**Parameters**

`struct tty_port *port`
:   tty port to change

`int limit`
:   memory limit to set

**Description**

Change the tty buffer memory limit.

Must be called before the other tty buffer functions are used.

---

## [Buffer Locking](#id3)

These are used only in special circumstances. Avoid them.

void tty\_buffer\_lock\_exclusive(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port)
:   gain exclusive access to buffer

**Parameters**

`struct tty_port *port`
:   tty port owning the flip buffer

**Description**

Guarantees safe use of the [`tty_ldisc_ops.receive_buf()`](tty_ldisc.html#c.tty_ldisc_ops "tty_ldisc_ops") method by excluding
the buffer work and any pending flush from using the flip buffer. Data can
continue to be added concurrently to the flip buffer from the driver side.

See also [`tty_buffer_unlock_exclusive()`](#c.tty_buffer_unlock_exclusive "tty_buffer_unlock_exclusive").

void tty\_buffer\_unlock\_exclusive(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port)
:   release exclusive access

**Parameters**

`struct tty_port *port`
:   tty port owning the flip buffer

**Description**

The buffer work is restarted if there is data in the flip buffer.

See also [`tty_buffer_lock_exclusive()`](#c.tty_buffer_lock_exclusive "tty_buffer_lock_exclusive").

---

## [Internal Functions](#id4)

void tty\_buffer\_free\_all(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port)
:   free buffers used by a tty

**Parameters**

`struct tty_port *port`
:   tty port to free from

**Description**

Remove all the buffers pending on a tty whether queued with data or in the
free ring. Must be called when the tty is no longer in use.

struct tty\_buffer \*tty\_buffer\_alloc(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, size\_t size)
:   allocate a tty buffer

**Parameters**

`struct tty_port *port`
:   tty port

`size_t size`
:   desired size (characters)

**Description**

Allocate a new tty buffer to hold the desired number of characters. We
round our buffers off in 256 character chunks to get better allocation
behaviour.

**Return**

`NULL` if out of memory or the allocation would exceed the per
device queue.

void tty\_buffer\_free(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, struct tty\_buffer \*b)
:   free a tty buffer

**Parameters**

`struct tty_port *port`
:   tty port owning the buffer

`struct tty_buffer *b`
:   the buffer to free

**Description**

Free a tty buffer, or add it to the free list according to our internal
strategy.

void tty\_buffer\_flush(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct tty\_ldisc \*ld)
:   flush full tty buffers

**Parameters**

`struct tty_struct *tty`
:   tty to flush

`struct tty_ldisc *ld`
:   optional ldisc ptr (must be referenced)

**Description**

Flush all the buffers containing receive data. If **ld** != `NULL`, flush the
ldisc input buffer.

Locking: takes buffer lock to ensure single-threaded flip buffer ‘consumer’.

int \_\_tty\_buffer\_request\_room(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, size\_t size, bool flags)
:   grow tty buffer if needed

**Parameters**

`struct tty_port *port`
:   tty port

`size_t size`
:   size desired

`bool flags`
:   buffer has to store flags along character data

**Description**

Make at least **size** bytes of linear space available for the tty buffer.

Will change over to a new buffer if the current buffer is encoded as
`TTY_NORMAL` (so has no flags buffer) and the new buffer requires a flags
buffer.

**Return**

the size we managed to find.

void flush\_to\_ldisc(struct work\_struct \*work)
:   flush data from buffer to ldisc

**Parameters**

`struct work_struct *work`
:   tty structure passed from work queue.

**Description**

This routine is called out of the software interrupt to flush data from the
buffer chain to the line discipline.

The `receive_buf()` method is single threaded for each tty instance.

Locking: takes buffer lock to ensure single-threaded flip buffer ‘consumer’.

int tty\_insert\_flip\_string\_and\_push\_buffer(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, const u8 \*chars, size\_t size)
:   add characters to the tty buffer and push

**Parameters**

`struct tty_port *port`
:   tty port

`const u8 *chars`
:   characters

`size_t size`
:   size

**Description**

The function combines `tty_insert_flip_string()` and [`tty_flip_buffer_push()`](#c.tty_flip_buffer_push "tty_flip_buffer_push")
with the exception of properly holding the **port->lock**.

To be used only internally (by pty currently).

**Return**

the number added.

void tty\_buffer\_init(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port)
:   prepare a tty buffer structure

**Parameters**

`struct tty_port *port`
:   tty port to initialise

**Description**

Set up the initial state of the buffer management for a tty device. Must be
called before the other tty buffer functions are used.
