# Sequence counters and sequential locks

> 출처(원문): https://docs.kernel.org/locking/seqlock.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Sequence counters and sequential locks

## Introduction

Sequence counters are a reader-writer consistency mechanism with
lockless readers (read-only retry loops), and no writer starvation. They
are used for data that’s rarely written to (e.g. system time), where the
reader wants a consistent set of information and is willing to retry if
that information changes.

A data set is consistent when the sequence count at the beginning of the
read side critical section is even and the same sequence count value is
read again at the end of the critical section. The data in the set must
be copied out inside the read side critical section. If the sequence
count has changed between the start and the end of the critical section,
the reader must retry.

Writers increment the sequence count at the start and the end of their
critical section. After starting the critical section the sequence count
is odd and indicates to the readers that an update is in progress. At
the end of the write side critical section the sequence count becomes
even again which lets readers make progress.

A sequence counter write side critical section must never be preempted
or interrupted by read side sections. Otherwise the reader will spin for
the entire scheduler tick due to the odd sequence count value and the
interrupted writer. If that reader belongs to a real-time scheduling
class, it can spin forever and the kernel will livelock.

This mechanism cannot be used if the protected data contains pointers,
as the writer can invalidate a pointer that the reader is following.

## Sequence counters (`seqcount_t`)

This is the raw counting mechanism, which does not protect against
multiple writers. Write side critical sections must thus be serialized
by an external lock.

If the write serialization primitive is not implicitly disabling
preemption, preemption must be explicitly disabled before entering the
write side section. If the read section can be invoked from hardirq or
softirq contexts, interrupts or bottom halves must also be respectively
disabled before entering the write section.

If it’s desired to automatically handle the sequence counter
requirements of writer serialization and non-preemptibility, use
[Sequential locks (seqlock\_t)](#seqlock-t) instead.

Initialization:

```
/* dynamic */
seqcount_t foo_seqcount;
seqcount_init(&foo_seqcount);

/* static */
static seqcount_t foo_seqcount = SEQCNT_ZERO(foo_seqcount);

/* C99 struct init */
struct {
        .seq   = SEQCNT_ZERO(foo.seq),
} foo;
```

Write path:

```
/* Serialized context with disabled preemption */

write_seqcount_begin(&foo_seqcount);

/* ... [[write-side critical section]] ... */

write_seqcount_end(&foo_seqcount);
```

Read path:

```
do {
        seq = read_seqcount_begin(&foo_seqcount);

        /* ... [[read-side critical section]] ... */

} while (read_seqcount_retry(&foo_seqcount, seq));
```

### Sequence counters with associated locks (`seqcount_LOCKNAME_t`)

As discussed at [Sequence counters (seqcount\_t)](#seqcount-t), sequence count write side critical
sections must be serialized and non-preemptible. This variant of
sequence counters associate the lock used for writer serialization at
initialization time, which enables lockdep to validate that the write
side critical sections are properly serialized.

This lock association is a NOOP if lockdep is disabled and has neither
storage nor runtime overhead. If lockdep is enabled, the lock pointer is
stored in `struct seqcount` and lockdep’s “lock is held” assertions are
injected at the beginning of the write side critical section to validate
that it is properly protected.

For lock types which do not implicitly disable preemption, preemption
protection is enforced in the write side function.

The following sequence counters with associated locks are defined:

> * `seqcount_spinlock_t`
> * `seqcount_raw_spinlock_t`
> * `seqcount_rwlock_t`
> * `seqcount_mutex_t`
> * `seqcount_ww_mutex_t`

The sequence counter read and write APIs can take either a plain
seqcount\_t or any of the seqcount\_LOCKNAME\_t variants above.

Initialization (replace “LOCKNAME” with one of the supported locks):

```
/* dynamic */
seqcount_LOCKNAME_t foo_seqcount;
seqcount_LOCKNAME_init(&foo_seqcount, &lock);

/* static */
static seqcount_LOCKNAME_t foo_seqcount =
        SEQCNT_LOCKNAME_ZERO(foo_seqcount, &lock);

/* C99 struct init */
struct {
        .seq   = SEQCNT_LOCKNAME_ZERO(foo.seq, &lock),
} foo;
```

Write path: same as in [Sequence counters (seqcount\_t)](#seqcount-t), while running from a context
with the associated write serialization lock acquired.

Read path: same as in [Sequence counters (seqcount\_t)](#seqcount-t).

### Latch sequence counters (`seqcount_latch_t`)

Latch sequence counters are a multiversion concurrency control mechanism
where the embedded seqcount\_t counter even/odd value is used to switch
between two copies of protected data. This allows the sequence counter
read path to safely interrupt its own write side critical section.

Use seqcount\_latch\_t when the write side sections cannot be protected
from interruption by readers. This is typically the case when the read
side can be invoked from NMI handlers.

Check [`write_seqcount_latch()`](#c.write_seqcount_latch "write_seqcount_latch") for more information.

## Sequential locks (`seqlock_t`)

This contains the [Sequence counters (seqcount\_t)](#seqcount-t) mechanism earlier discussed, plus an
embedded spinlock for writer serialization and non-preemptibility.

If the read side section can be invoked from hardirq or softirq context,
use the write side function variants which disable interrupts or bottom
halves respectively.

Initialization:

```
/* dynamic */
seqlock_t foo_seqlock;
seqlock_init(&foo_seqlock);

/* static */
static DEFINE_SEQLOCK(foo_seqlock);

/* C99 struct init */
struct {
        .seql   = __SEQLOCK_UNLOCKED(foo.seql)
} foo;
```

Write path:

```
write_seqlock(&foo_seqlock);

/* ... [[write-side critical section]] ... */

write_sequnlock(&foo_seqlock);
```

Read path, three categories:

1. Normal Sequence readers which never block a writer but they must
   retry if a writer is in progress by detecting change in the sequence
   number. Writers do not wait for a sequence reader:

   ```
   do {
           seq = read_seqbegin(&foo_seqlock);

           /* ... [[read-side critical section]] ... */

   } while (read_seqretry(&foo_seqlock, seq));
   ```
2. Locking readers which will wait if a writer or another locking reader
   is in progress. A locking reader in progress will also block a writer
   from entering its critical section. This read lock is
   exclusive. Unlike rwlock\_t, only one locking reader can acquire it:

   ```
   read_seqlock_excl(&foo_seqlock);

   /* ... [[read-side critical section]] ... */

   read_sequnlock_excl(&foo_seqlock);
   ```
3. Conditional lockless reader (as in 1), or locking reader (as in 2),
   according to a passed marker. This is used to avoid lockless readers
   starvation (too much retry loops) in case of a sharp spike in write
   activity. First, a lockless read is tried (even marker passed). If
   that trial fails (sequence counter doesn’t match), make the marker
   odd for the next iteration, the lockless read is transformed to a
   full locking read and no retry loop is necessary, for example:

   ```
   /* marker; even initialization */
   int seq = 1;
   do {
           seq++; /* 2 on the 1st/lockless path, otherwise odd */
           read_seqbegin_or_lock(&foo_seqlock, &seq);

           /* ... [[read-side critical section]] ... */

   } while (need_seqretry(&foo_seqlock, seq));
   done_seqretry(&foo_seqlock, seq);
   ```

## API documentation

seqcount\_init

`seqcount_init (s)`

> runtime initializer for seqcount\_t

**Parameters**

`s`
:   Pointer to the seqcount\_t instance

SEQCNT\_ZERO

`SEQCNT_ZERO (name)`

> static initializer for seqcount\_t

**Parameters**

`name`
:   Name of the seqcount\_t instance

\_\_read\_seqcount\_begin

`__read_seqcount_begin (s)`

> begin a seqcount\_t read section

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Return**

count to be passed to [`read_seqcount_retry()`](#c.read_seqcount_retry "read_seqcount_retry")

raw\_read\_seqcount\_begin

`raw_read_seqcount_begin (s)`

> begin a seqcount\_t read section w/o lockdep

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Return**

count to be passed to [`read_seqcount_retry()`](#c.read_seqcount_retry "read_seqcount_retry")

read\_seqcount\_begin

`read_seqcount_begin (s)`

> begin a seqcount\_t read critical section

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Return**

count to be passed to [`read_seqcount_retry()`](#c.read_seqcount_retry "read_seqcount_retry")

raw\_read\_seqcount

`raw_read_seqcount (s)`

> read the raw seqcount\_t counter value

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Description**

raw\_read\_seqcount opens a read critical section of the given
seqcount\_t, without any lockdep checking, and without checking or
masking the sequence counter LSB. Calling code is responsible for
handling that.

**Return**

count to be passed to [`read_seqcount_retry()`](#c.read_seqcount_retry "read_seqcount_retry")

raw\_seqcount\_try\_begin

`raw_seqcount_try_begin (s, start)`

> begin a seqcount\_t read critical section w/o lockdep and w/o counter stabilization

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

`start`
:   count to be passed to [`read_seqcount_retry()`](#c.read_seqcount_retry "read_seqcount_retry")

**Description**

Similar to [`raw_seqcount_begin()`](#c.raw_seqcount_begin "raw_seqcount_begin"), except it enables eliding the critical
section entirely if odd, instead of doing the speculation knowing it will
fail.

Useful when counter stabilization is more or less equivalent to taking
the lock and there is a slowpath that does that.

If true, start will be set to the (even) sequence count read.

**Return**

true when a read critical section is started.

raw\_seqcount\_begin

`raw_seqcount_begin (s)`

> begin a seqcount\_t read critical section w/o lockdep and w/o counter stabilization

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Description**

raw\_seqcount\_begin opens a read critical section of the given
seqcount\_t. Unlike [`read_seqcount_begin()`](#c.read_seqcount_begin "read_seqcount_begin"), this function will not wait
for the count to stabilize. If a writer is active when it begins, it
will fail the [`read_seqcount_retry()`](#c.read_seqcount_retry "read_seqcount_retry") at the end of the read critical
section instead of stabilizing at the beginning of it.

Use this only in special kernel hot paths where the read section is
small and has a high probability of success through other external
means. It will save a single branching instruction.

**Return**

count to be passed to [`read_seqcount_retry()`](#c.read_seqcount_retry "read_seqcount_retry")

\_\_read\_seqcount\_retry

`__read_seqcount_retry (s, start)`

> end a seqcount\_t read section w/o barrier

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

`start`
:   count, from [`read_seqcount_begin()`](#c.read_seqcount_begin "read_seqcount_begin")

**Description**

\_\_read\_seqcount\_retry is like read\_seqcount\_retry, but has no `smp_rmb()`
barrier. Callers should ensure that `smp_rmb()` or equivalent ordering is
provided before actually loading any of the variables that are to be
protected in this critical section.

Use carefully, only in critical code, and comment how the barrier is
provided.

**Return**

true if a read section retry is required, else false

read\_seqcount\_retry

`read_seqcount_retry (s, start)`

> end a seqcount\_t read critical section

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

`start`
:   count, from [`read_seqcount_begin()`](#c.read_seqcount_begin "read_seqcount_begin")

**Description**

read\_seqcount\_retry closes the read critical section of given
seqcount\_t. If the critical section was invalid, it must be ignored
(and typically retried).

**Return**

true if a read section retry is required, else false

raw\_write\_seqcount\_begin

`raw_write_seqcount_begin (s)`

> start a seqcount\_t write section w/o lockdep

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Context**

check [`write_seqcount_begin()`](#c.write_seqcount_begin "write_seqcount_begin")

raw\_write\_seqcount\_end

`raw_write_seqcount_end (s)`

> end a seqcount\_t write section w/o lockdep

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Context**

check [`write_seqcount_end()`](#c.write_seqcount_end "write_seqcount_end")

write\_seqcount\_begin\_nested

`write_seqcount_begin_nested (s, subclass)`

> start a seqcount\_t write section with custom lockdep nesting level

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

`subclass`
:   lockdep nesting level

**Description**

See [Runtime locking correctness validator](lockdep-design.html)

**Context**

check [`write_seqcount_begin()`](#c.write_seqcount_begin "write_seqcount_begin")

write\_seqcount\_begin

`write_seqcount_begin (s)`

> start a seqcount\_t write side critical section

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Context**

sequence counter write side sections must be serialized and
non-preemptible. Preemption will be automatically disabled if and
only if the seqcount write serialization lock is associated, and
preemptible. If readers can be invoked from hardirq or softirq
context, interrupts or bottom halves must be respectively disabled.

write\_seqcount\_end

`write_seqcount_end (s)`

> end a seqcount\_t write side critical section

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Context**

Preemption will be automatically re-enabled if and only if
the seqcount write serialization lock is associated, and preemptible.

raw\_write\_seqcount\_barrier

`raw_write_seqcount_barrier (s)`

> do a seqcount\_t write barrier

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Description**

This can be used to provide an ordering guarantee instead of the usual
consistency guarantee. It is one wmb cheaper, because it can collapse
the two back-to-back `wmb()`s.

Note that writes surrounding the barrier should be declared atomic (e.g.
via WRITE\_ONCE): a) to ensure the writes become visible to other threads
atomically, avoiding compiler optimizations; b) to document which writes are
meant to propagate to the reader critical section. This is necessary because
neither writes before nor after the barrier are enclosed in a seq-writer
critical section that would ensure readers are aware of ongoing writes:

```
seqcount_t seq;
bool X = true, Y = false;

void read(void)
{
        bool x, y;

        do {
                int s = read_seqcount_begin(&seq);

                x = X; y = Y;

        } while (read_seqcount_retry(&seq, s));

        BUG_ON(!x && !y);
}

void write(void)
{
        WRITE_ONCE(Y, true);

        raw_write_seqcount_barrier(seq);

        WRITE_ONCE(X, false);
}
```

write\_seqcount\_invalidate

`write_seqcount_invalidate (s)`

> invalidate in-progress seqcount\_t read side operations

**Parameters**

`s`
:   Pointer to seqcount\_t or any of the seqcount\_LOCKNAME\_t variants

**Description**

After write\_seqcount\_invalidate, no seqcount\_t read side operations
will complete successfully and see data older than this.

SEQCNT\_LATCH\_ZERO

`SEQCNT_LATCH_ZERO (seq_name)`

> static initializer for seqcount\_latch\_t

**Parameters**

`seq_name`
:   Name of the seqcount\_latch\_t instance

seqcount\_latch\_init

`seqcount_latch_init (s)`

> runtime initializer for seqcount\_latch\_t

**Parameters**

`s`
:   Pointer to the seqcount\_latch\_t instance

unsigned raw\_read\_seqcount\_latch(const seqcount\_latch\_t \*s)
:   pick even/odd latch data copy

**Parameters**

`const seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

**Description**

See [`raw_write_seqcount_latch()`](#c.raw_write_seqcount_latch "raw_write_seqcount_latch") for details and a full reader/writer
usage example.

**Return**

sequence counter raw value. Use the lowest bit as an index for
picking which data copy to read. The full counter must then be checked
with [`raw_read_seqcount_latch_retry()`](#c.raw_read_seqcount_latch_retry "raw_read_seqcount_latch_retry").

unsigned read\_seqcount\_latch(const seqcount\_latch\_t \*s)
:   pick even/odd latch data copy

**Parameters**

`const seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

**Description**

See [`write_seqcount_latch()`](#c.write_seqcount_latch "write_seqcount_latch") for details and a full reader/writer usage
example.

**Return**

sequence counter raw value. Use the lowest bit as an index for
picking which data copy to read. The full counter must then be checked
with [`read_seqcount_latch_retry()`](#c.read_seqcount_latch_retry "read_seqcount_latch_retry").

int raw\_read\_seqcount\_latch\_retry(const seqcount\_latch\_t \*s, unsigned start)
:   end a seqcount\_latch\_t read section

**Parameters**

`const seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

`unsigned start`
:   count, from [`raw_read_seqcount_latch()`](#c.raw_read_seqcount_latch "raw_read_seqcount_latch")

**Return**

true if a read section retry is required, else false

int read\_seqcount\_latch\_retry(const seqcount\_latch\_t \*s, unsigned start)
:   end a seqcount\_latch\_t read section

**Parameters**

`const seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

`unsigned start`
:   count, from [`read_seqcount_latch()`](#c.read_seqcount_latch "read_seqcount_latch")

**Return**

true if a read section retry is required, else false

void raw\_write\_seqcount\_latch(seqcount\_latch\_t \*s)
:   redirect latch readers to even/odd copy

**Parameters**

`seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

void write\_seqcount\_latch\_begin(seqcount\_latch\_t \*s)
:   redirect latch readers to odd copy

**Parameters**

`seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

**Description**

The latch technique is a multiversion concurrency control method that allows
queries during non-atomic modifications. If you can guarantee queries never
interrupt the modification -- e.g. the concurrency is strictly between CPUs
-- you most likely do not need this.

Where the traditional RCU/lockless data structures rely on atomic
modifications to ensure queries observe either the old or the new state the
latch allows the same for non-atomic updates. The trade-off is doubling the
cost of storage; we have to maintain two copies of the entire data
structure.

Very simply put: we first modify one copy and then the other. This ensures
there is always one copy in a stable state, ready to give us an answer.

The basic form is a data structure like:

```
struct latch_struct {
        seqcount_latch_t        seq;
        struct data_struct      data[2];
};
```

Where a modification, which is assumed to be externally serialized, does the
following:

```
void latch_modify(struct latch_struct *latch, ...)
{
        write_seqcount_latch_begin(&latch->seq);
        modify(latch->data[0], ...);
        write_seqcount_latch(&latch->seq);
        modify(latch->data[1], ...);
        write_seqcount_latch_end(&latch->seq);
}
```

The query will have a form like:

```
struct entry *latch_query(struct latch_struct *latch, ...)
{
        struct entry *entry;
        unsigned seq, idx;

        do {
                seq = read_seqcount_latch(&latch->seq);

                idx = seq & 0x01;
                entry = data_query(latch->data[idx], ...);

        // This includes needed smp_rmb()
        } while (read_seqcount_latch_retry(&latch->seq, seq));

        return entry;
}
```

So during the modification, queries are first redirected to data[1]. Then we
modify data[0]. When that is complete, we redirect queries back to data[0]
and we can modify data[1].

**NOTE**

> The non-requirement for atomic modifications does \_NOT\_ include
> the publishing of new entries in the case where data is a dynamic
> data structure.
>
> An iteration might start in data[0] and get suspended long enough
> to miss an entire modification sequence, once it resumes it might
> observe the new entry.

NOTE2:

> When data is a dynamic data structure; one should use regular RCU
> patterns to manage the lifetimes of the objects within.

void write\_seqcount\_latch(seqcount\_latch\_t \*s)
:   redirect latch readers to even copy

**Parameters**

`seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

void write\_seqcount\_latch\_end(seqcount\_latch\_t \*s)
:   end a seqcount\_latch\_t write section

**Parameters**

`seqcount_latch_t *s`
:   Pointer to seqcount\_latch\_t

**Description**

Marks the end of a seqcount\_latch\_t writer section, after all copies of the
latch-protected data have been updated.

seqlock\_init

`seqlock_init (sl)`

> dynamic initializer for seqlock\_t

**Parameters**

`sl`
:   Pointer to the seqlock\_t instance

DEFINE\_SEQLOCK

`DEFINE_SEQLOCK (sl)`

> Define a statically allocated seqlock\_t

**Parameters**

`sl`
:   Name of the seqlock\_t instance

unsigned read\_seqbegin(const seqlock\_t \*sl)
:   start a seqlock\_t read side critical section

**Parameters**

`const seqlock_t *sl`
:   Pointer to seqlock\_t

**Return**

count, to be passed to [`read_seqretry()`](#c.read_seqretry "read_seqretry")

unsigned read\_seqretry(const seqlock\_t \*sl, unsigned start)
:   end a seqlock\_t read side section

**Parameters**

`const seqlock_t *sl`
:   Pointer to seqlock\_t

`unsigned start`
:   count, from [`read_seqbegin()`](#c.read_seqbegin "read_seqbegin")

**Description**

read\_seqretry closes the read side critical section of given seqlock\_t.
If the critical section was invalid, it must be ignored (and typically
retried).

**Return**

true if a read section retry is required, else false

void write\_seqlock(seqlock\_t \*sl)
:   start a seqlock\_t write side critical section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

write\_seqlock opens a write side critical section for the given
seqlock\_t. It also implicitly acquires the spinlock\_t embedded inside
that sequential lock. All seqlock\_t write side sections are thus
automatically serialized and non-preemptible.

**Context**

if the seqlock\_t read section, or other write side critical
sections, can be invoked from hardirq or softirq contexts, use the
\_irqsave or \_bh variants of this function instead.

void write\_sequnlock(seqlock\_t \*sl)
:   end a seqlock\_t write side critical section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

write\_sequnlock closes the (serialized and non-preemptible) write side
critical section of given seqlock\_t.

void write\_seqlock\_bh(seqlock\_t \*sl)
:   start a softirqs-disabled seqlock\_t write section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

\_bh variant of [`write_seqlock()`](#c.write_seqlock "write_seqlock"). Use only if the read side section, or
other write side sections, can be invoked from softirq contexts.

void write\_sequnlock\_bh(seqlock\_t \*sl)
:   end a softirqs-disabled seqlock\_t write section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

write\_sequnlock\_bh closes the serialized, non-preemptible, and
softirqs-disabled, seqlock\_t write side critical section opened with
[`write_seqlock_bh()`](#c.write_seqlock_bh "write_seqlock_bh").

void write\_seqlock\_irq(seqlock\_t \*sl)
:   start a non-interruptible seqlock\_t write section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

\_irq variant of [`write_seqlock()`](#c.write_seqlock "write_seqlock"). Use only if the read side section, or
other write sections, can be invoked from hardirq contexts.

void write\_sequnlock\_irq(seqlock\_t \*sl)
:   end a non-interruptible seqlock\_t write section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

write\_sequnlock\_irq closes the serialized and non-interruptible
seqlock\_t write side section opened with [`write_seqlock_irq()`](#c.write_seqlock_irq "write_seqlock_irq").

write\_seqlock\_irqsave

`write_seqlock_irqsave (lock, flags)`

> start a non-interruptible seqlock\_t write section

**Parameters**

`lock`
:   Pointer to seqlock\_t

`flags`
:   Stack-allocated storage for saving caller’s local interrupt
    state, to be passed to [`write_sequnlock_irqrestore()`](#c.write_sequnlock_irqrestore "write_sequnlock_irqrestore").

**Description**

\_irqsave variant of [`write_seqlock()`](#c.write_seqlock "write_seqlock"). Use it only if the read side
section, or other write sections, can be invoked from hardirq context.

void write\_sequnlock\_irqrestore(seqlock\_t \*sl, unsigned long flags)
:   end non-interruptible seqlock\_t write section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

`unsigned long flags`
:   Caller’s saved interrupt state, from [`write_seqlock_irqsave()`](#c.write_seqlock_irqsave "write_seqlock_irqsave")

**Description**

write\_sequnlock\_irqrestore closes the serialized and non-interruptible
seqlock\_t write section previously opened with [`write_seqlock_irqsave()`](#c.write_seqlock_irqsave "write_seqlock_irqsave").

void read\_seqlock\_excl(seqlock\_t \*sl)
:   begin a seqlock\_t locking reader section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

read\_seqlock\_excl opens a seqlock\_t locking reader critical section. A
locking reader exclusively locks out *both* other writers *and* other
locking readers, but it does not update the embedded sequence number.

Locking readers act like a normal `spin_lock()`/`spin_unlock()`.

The opened read section must be closed with [`read_sequnlock_excl()`](#c.read_sequnlock_excl "read_sequnlock_excl").

**Context**

if the seqlock\_t write section, *or other read sections*, can
be invoked from hardirq or softirq contexts, use the \_irqsave or \_bh
variant of this function instead.

void read\_sequnlock\_excl(seqlock\_t \*sl)
:   end a seqlock\_t locking reader critical section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

void read\_seqlock\_excl\_bh(seqlock\_t \*sl)
:   start a seqlock\_t locking reader section with softirqs disabled

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

\_bh variant of [`read_seqlock_excl()`](#c.read_seqlock_excl "read_seqlock_excl"). Use this variant only if the
seqlock\_t write side section, *or other read sections*, can be invoked
from softirq contexts.

void read\_sequnlock\_excl\_bh(seqlock\_t \*sl)
:   stop a seqlock\_t softirq-disabled locking reader section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

void read\_seqlock\_excl\_irq(seqlock\_t \*sl)
:   start a non-interruptible seqlock\_t locking reader section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

**Description**

\_irq variant of [`read_seqlock_excl()`](#c.read_seqlock_excl "read_seqlock_excl"). Use this only if the seqlock\_t
write side section, *or other read sections*, can be invoked from a
hardirq context.

void read\_sequnlock\_excl\_irq(seqlock\_t \*sl)
:   end an interrupts-disabled seqlock\_t locking reader section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

read\_seqlock\_excl\_irqsave

`read_seqlock_excl_irqsave (lock, flags)`

> start a non-interruptible seqlock\_t locking reader section

**Parameters**

`lock`
:   Pointer to seqlock\_t

`flags`
:   Stack-allocated storage for saving caller’s local interrupt
    state, to be passed to [`read_sequnlock_excl_irqrestore()`](#c.read_sequnlock_excl_irqrestore "read_sequnlock_excl_irqrestore").

**Description**

\_irqsave variant of [`read_seqlock_excl()`](#c.read_seqlock_excl "read_seqlock_excl"). Use this only if the seqlock\_t
write side section, *or other read sections*, can be invoked from a
hardirq context.

void read\_sequnlock\_excl\_irqrestore(seqlock\_t \*sl, unsigned long flags)
:   end non-interruptible seqlock\_t locking reader section

**Parameters**

`seqlock_t *sl`
:   Pointer to seqlock\_t

`unsigned long flags`
:   Caller saved interrupt state, from [`read_seqlock_excl_irqsave()`](#c.read_seqlock_excl_irqsave "read_seqlock_excl_irqsave")

void read\_seqbegin\_or\_lock(seqlock\_t \*lock, int \*seq)
:   begin a seqlock\_t lockless or locking reader

**Parameters**

`seqlock_t *lock`
:   Pointer to seqlock\_t

`int *seq`
:   Marker and return parameter. If the passed value is even, the
    reader will become a *lockless* seqlock\_t reader as in [`read_seqbegin()`](#c.read_seqbegin "read_seqbegin").
    If the passed value is odd, the reader will become a *locking* reader
    as in [`read_seqlock_excl()`](#c.read_seqlock_excl "read_seqlock_excl"). In the first call to this function, the
    caller *must* initialize and pass an even value to **seq**; this way, a
    lockless read can be optimistically tried first.

**Description**

read\_seqbegin\_or\_lock is an API designed to optimistically try a normal
lockless seqlock\_t read section first. If an odd counter is found, the
lockless read trial has failed, and the next read iteration transforms
itself into a full seqlock\_t locking reader.

This is typically used to avoid seqlock\_t lockless readers starvation
(too much retry loops) in the case of a sharp spike in write side
activity.

Check [Sequence counters and sequential locks](#) for template example code.

**Context**

if the seqlock\_t write section, *or other read sections*, can
be invoked from hardirq or softirq contexts, use the \_irqsave or \_bh
variant of this function instead.

**Return**

the encountered sequence counter value, through the **seq**
parameter, which is overloaded as a return parameter. This returned
value must be checked with [`need_seqretry()`](#c.need_seqretry "need_seqretry"). If the read section need to
be retried, this returned value must also be passed as the **seq**
parameter of the next [`read_seqbegin_or_lock()`](#c.read_seqbegin_or_lock "read_seqbegin_or_lock") iteration.

int need\_seqretry(seqlock\_t \*lock, int seq)
:   validate seqlock\_t “locking or lockless” read section

**Parameters**

`seqlock_t *lock`
:   Pointer to seqlock\_t

`int seq`
:   sequence count, from [`read_seqbegin_or_lock()`](#c.read_seqbegin_or_lock "read_seqbegin_or_lock")

**Return**

true if a read section retry is required, false otherwise

void done\_seqretry(seqlock\_t \*lock, int seq)
:   end seqlock\_t “locking or lockless” reader section

**Parameters**

`seqlock_t *lock`
:   Pointer to seqlock\_t

`int seq`
:   count, from [`read_seqbegin_or_lock()`](#c.read_seqbegin_or_lock "read_seqbegin_or_lock")

**Description**

done\_seqretry finishes the seqlock\_t read side critical section started
with [`read_seqbegin_or_lock()`](#c.read_seqbegin_or_lock "read_seqbegin_or_lock") and validated by [`need_seqretry()`](#c.need_seqretry "need_seqretry").

unsigned long read\_seqbegin\_or\_lock\_irqsave(seqlock\_t \*lock, int \*seq)
:   begin a seqlock\_t lockless reader, or a non-interruptible locking reader

**Parameters**

`seqlock_t *lock`
:   Pointer to seqlock\_t

`int *seq`
:   Marker and return parameter. Check [`read_seqbegin_or_lock()`](#c.read_seqbegin_or_lock "read_seqbegin_or_lock").

**Description**

This is the \_irqsave variant of [`read_seqbegin_or_lock()`](#c.read_seqbegin_or_lock "read_seqbegin_or_lock"). Use it only if
the seqlock\_t write section, *or other read sections*, can be invoked
from hardirq context.

> 1. The saved local interrupts state in case of a locking reader, to
>    be passed to [`done_seqretry_irqrestore()`](#c.done_seqretry_irqrestore "done_seqretry_irqrestore").
> 2. The encountered sequence counter value, returned through **seq**
>    overloaded as a return parameter. Check [`read_seqbegin_or_lock()`](#c.read_seqbegin_or_lock "read_seqbegin_or_lock").

**Note**

Interrupts will be disabled only for “locking reader” mode.

void done\_seqretry\_irqrestore(seqlock\_t \*lock, int seq, unsigned long flags)
:   end a seqlock\_t lockless reader, or a non-interruptible locking reader section

**Parameters**

`seqlock_t *lock`
:   Pointer to seqlock\_t

`int seq`
:   Count, from [`read_seqbegin_or_lock_irqsave()`](#c.read_seqbegin_or_lock_irqsave "read_seqbegin_or_lock_irqsave")

`unsigned long flags`
:   Caller’s saved local interrupt state in case of a locking
    reader, also from [`read_seqbegin_or_lock_irqsave()`](#c.read_seqbegin_or_lock_irqsave "read_seqbegin_or_lock_irqsave")

**Description**

This is the \_irqrestore variant of [`done_seqretry()`](#c.done_seqretry "done_seqretry"). The read section
must’ve been opened with [`read_seqbegin_or_lock_irqsave()`](#c.read_seqbegin_or_lock_irqsave "read_seqbegin_or_lock_irqsave"), and validated
by [`need_seqretry()`](#c.need_seqretry "need_seqretry").

scoped\_seqlock\_read

`scoped_seqlock_read (_seqlock, _target)`

> execute the read-side critical section without manual sequence counter handling or calls to other helpers

**Parameters**

`_seqlock`
:   pointer to seqlock\_t protecting the data

`_target`
:   an `enum ss_state`: one of {ss\_lock, ss\_lock\_irqsave, ss\_lockless}
    indicating the type of critical read section

**Description**

Example:

```
scoped_seqlock_read (&lock, ss_lock) {
    // read-side critical section
}
```

Starts with a lockess pass first. If it fails, restarts the critical
section with the lock held.
