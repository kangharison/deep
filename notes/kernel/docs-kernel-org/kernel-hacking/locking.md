# Unreliable Guide To Locking

> 출처(원문): https://docs.kernel.org/kernel-hacking/locking.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Unreliable Guide To Locking

Author:
:   Rusty Russell

## Introduction

Welcome, to Rusty’s Remarkably Unreliable Guide to Kernel Locking
issues. This document describes the locking systems in the Linux Kernel
in 2.6.

With the wide availability of HyperThreading, and preemption in the
Linux Kernel, everyone hacking on the kernel needs to know the
fundamentals of concurrency and locking for SMP.

## The Problem With Concurrency

(Skip this if you know what a Race Condition is).

In a normal program, you can increment a counter like so:

```
very_important_count++;
```

This is what they would expect to happen:

Expected Results

| Instance 1 | Instance 2 |
| --- | --- |
| read very\_important\_count (5) |  |
| add 1 (6) |  |
| write very\_important\_count (6) |  |
|  | read very\_important\_count (6) |
|  | add 1 (7) |
|  | write very\_important\_count (7) |

This is what might happen:

Possible Results

| Instance 1 | Instance 2 |
| --- | --- |
| read very\_important\_count (5) |  |
|  | read very\_important\_count (5) |
| add 1 (6) |  |
|  | add 1 (6) |
| write very\_important\_count (6) |  |
|  | write very\_important\_count (6) |

### Race Conditions and Critical Regions

This overlap, where the result depends on the relative timing of
multiple tasks, is called a race condition. The piece of code containing
the concurrency issue is called a critical region. And especially since
Linux starting running on SMP machines, they became one of the major
issues in kernel design and implementation.

Preemption can have the same effect, even if there is only one CPU: by
preempting one task during the critical region, we have exactly the same
race condition. In this case the thread which preempts might run the
critical region itself.

The solution is to recognize when these simultaneous accesses occur, and
use locks to make sure that only one instance can enter the critical
region at any time. There are many friendly primitives in the Linux
kernel to help you do this. And then there are the unfriendly
primitives, but I’ll pretend they don’t exist.

## Locking in the Linux Kernel

If I could give you one piece of advice on locking: **keep it simple**.

Be reluctant to introduce new locks.

### Two Main Types of Kernel Locks: Spinlocks and Mutexes

There are two main types of kernel locks. The fundamental type is the
spinlock (`include/asm/spinlock.h`), which is a very simple
single-holder lock: if you can’t get the spinlock, you keep trying
(spinning) until you can. Spinlocks are very small and fast, and can be
used anywhere.

The second type is a mutex (`include/linux/mutex.h`): it is like a
spinlock, but you may block holding a mutex. If you can’t lock a mutex,
your task will suspend itself, and be woken up when the mutex is
released. This means the CPU can do something else while you are
waiting. There are many cases when you simply can’t sleep (see
[What Functions Are Safe To Call From Interrupts?](#what-functions-are-safe-to-call-from-interrupts)),
and so have to use a spinlock instead.

Neither type of lock is recursive: see
[Deadlock: Simple and Advanced](#deadlock-simple-and-advanced).

### Locks and Uniprocessor Kernels

For kernels compiled without `CONFIG_SMP`, and without
`CONFIG_PREEMPT` spinlocks do not exist at all. This is an excellent
design decision: when no-one else can run at the same time, there is no
reason to have a lock.

If the kernel is compiled without `CONFIG_SMP`, but `CONFIG_PREEMPT`
is set, then spinlocks simply disable preemption, which is sufficient to
prevent any races. For most purposes, we can think of preemption as
equivalent to SMP, and not worry about it separately.

You should always test your locking code with `CONFIG_SMP` and
`CONFIG_PREEMPT` enabled, even if you don’t have an SMP test box,
because it will still catch some kinds of locking bugs.

Mutexes still exist, because they are required for synchronization
between user contexts, as we will see below.

### Locking Only In User Context

If you have a data structure which is only ever accessed from user
context, then you can use a simple mutex (`include/linux/mutex.h`) to
protect it. This is the most trivial case: you initialize the mutex.
Then you can call [`mutex_lock_interruptible()`](#c.mutex_lock_interruptible "mutex_lock_interruptible") to grab the
mutex, and [`mutex_unlock()`](#c.mutex_unlock "mutex_unlock") to release it. There is also a
[`mutex_lock()`](#c.mutex_lock "mutex_lock"), which should be avoided, because it will
not return if a signal is received.

Example: `net/netfilter/nf_sockopt.c` allows registration of new
`setsockopt()` and `getsockopt()` calls, with
`nf_register_sockopt()`. Registration and de-registration
are only done on module load and unload (and boot time, where there is
no concurrency), and the list of registrations is only consulted for an
unknown `setsockopt()` or `getsockopt()` system
call. The `nf_sockopt_mutex` is perfect to protect this, especially
since the setsockopt and getsockopt calls may well sleep.

### Locking Between User Context and Softirqs

If a softirq shares data with user context, you have two problems.
Firstly, the current user context can be interrupted by a softirq, and
secondly, the critical region could be entered from another CPU. This is
where `spin_lock_bh()` (`include/linux/spinlock.h`) is
used. It disables softirqs on that CPU, then grabs the lock.
`spin_unlock_bh()` does the reverse. (The ‘\_bh’ suffix is
a historical reference to “Bottom Halves”, the old name for software
interrupts. It should really be called `spin_lock_softirq()`’ in a
perfect world).

Note that you can also use `spin_lock_irq()` or
`spin_lock_irqsave()` here, which stop hardware interrupts
as well: see [Hard IRQ Context](#hard-irq-context).

This works perfectly for UP as well: the spin lock vanishes, and this
macro simply becomes `local_bh_disable()`
(`include/linux/interrupt.h`), which protects you from the softirq
being run.

### Locking Between User Context and Tasklets

This is exactly the same as above, because tasklets are actually run
from a softirq.

### Locking Between User Context and Timers

This, too, is exactly the same as above, because timers are actually run
from a softirq. From a locking point of view, tasklets and timers are
identical.

### Locking Between Tasklets/Timers

Sometimes a tasklet or timer might want to share data with another
tasklet or timer.

#### The Same Tasklet/Timer

Since a tasklet is never run on two CPUs at once, you don’t need to
worry about your tasklet being reentrant (running twice at once), even
on SMP.

#### Different Tasklets/Timers

If another tasklet/timer wants to share data with your tasklet or timer
, you will both need to use `spin_lock()` and
`spin_unlock()` calls. `spin_lock_bh()` is
unnecessary here, as you are already in a tasklet, and none will be run
on the same CPU.

### Locking Between Softirqs

Often a softirq might want to share data with itself or a tasklet/timer.

#### The Same Softirq

The same softirq can run on the other CPUs: you can use a per-CPU array
(see [Per-CPU Data](#per-cpu-data)) for better performance. If you’re
going so far as to use a softirq, you probably care about scalable
performance enough to justify the extra complexity.

You’ll need to use `spin_lock()` and
`spin_unlock()` for shared data.

#### Different Softirqs

You’ll need to use `spin_lock()` and
`spin_unlock()` for shared data, whether it be a timer,
tasklet, different softirq or the same or another softirq: any of them
could be running on a different CPU.

## Hard IRQ Context

Hardware interrupts usually communicate with a tasklet or softirq.
Frequently this involves putting work in a queue, which the softirq will
take out.

### Locking Between Hard IRQ and Softirqs/Tasklets

If a hardware irq handler shares data with a softirq, you have two
concerns. Firstly, the softirq processing can be interrupted by a
hardware interrupt, and secondly, the critical region could be entered
by a hardware interrupt on another CPU. This is where
`spin_lock_irq()` is used. It is defined to disable
interrupts on that cpu, then grab the lock.
`spin_unlock_irq()` does the reverse.

The irq handler does not need to use `spin_lock_irq()`, because
the softirq cannot run while the irq handler is running: it can use
`spin_lock()`, which is slightly faster. The only exception
would be if a different hardware irq handler uses the same lock:
`spin_lock_irq()` will stop that from interrupting us.

This works perfectly for UP as well: the spin lock vanishes, and this
macro simply becomes `local_irq_disable()`
(`include/asm/smp.h`), which protects you from the softirq/tasklet/BH
being run.

`spin_lock_irqsave()` (`include/linux/spinlock.h`) is a
variant which saves whether interrupts were on or off in a flags word,
which is passed to `spin_unlock_irqrestore()`. This means
that the same code can be used inside an hard irq handler (where
interrupts are already off) and in softirqs (where the irq disabling is
required).

Note that softirqs (and hence tasklets and timers) are run on return
from hardware interrupts, so `spin_lock_irq()` also stops
these. In that sense, `spin_lock_irqsave()` is the most
general and powerful locking function.

### Locking Between Two Hard IRQ Handlers

It is rare to have to share data between two IRQ handlers, but if you
do, `spin_lock_irqsave()` should be used: it is
architecture-specific whether all interrupts are disabled inside irq
handlers themselves.

## Cheat Sheet For Locking

Pete Zaitcev gives the following summary:

* If you are in a process context (any syscall) and want to lock other
  process out, use a mutex. You can take a mutex and sleep
  (`copy_from_user()` or `kmalloc(x,GFP_KERNEL)`).
* Otherwise (== data can be touched in an interrupt), use
  `spin_lock_irqsave()` and
  `spin_unlock_irqrestore()`.
* Avoid holding spinlock for more than 5 lines of code and across any
  function call (except accessors like `readb()`).

### Table of Minimum Requirements

The following table lists the **minimum** locking requirements between
various contexts. In some cases, the same context can only be running on
one CPU at a time, so no locking is required for that context (eg. a
particular thread can only run on one CPU at a time, but if it needs
shares data with another thread, locking is required).

Remember the advice above: you can always use
`spin_lock_irqsave()`, which is a superset of all other
spinlock primitives.

| . | IRQ Handler A | IRQ Handler B | Softirq A | Softirq B | Tasklet A | Tasklet B | Timer A | Timer B | User Context A | User Context B |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| IRQ Handler A | None |  |  |  |  |  |  |  |  |  |
| IRQ Handler B | SLIS | None |  |  |  |  |  |  |  |  |
| Softirq A | SLI | SLI | SL |  |  |  |  |  |  |  |
| Softirq B | SLI | SLI | SL | SL |  |  |  |  |  |  |
| Tasklet A | SLI | SLI | SL | SL | None |  |  |  |  |  |
| Tasklet B | SLI | SLI | SL | SL | SL | None |  |  |  |  |
| Timer A | SLI | SLI | SL | SL | SL | SL | None |  |  |  |
| Timer B | SLI | SLI | SL | SL | SL | SL | SL | None |  |  |
| User Context A | SLI | SLI | SLBH | SLBH | SLBH | SLBH | SLBH | SLBH | None |  |
| User Context B | SLI | SLI | SLBH | SLBH | SLBH | SLBH | SLBH | SLBH | MLI | None |

Table: Table of Locking Requirements

|  |  |
| --- | --- |
| SLIS | spin\_lock\_irqsave |
| SLI | spin\_lock\_irq |
| SL | spin\_lock |
| SLBH | spin\_lock\_bh |
| MLI | mutex\_lock\_interruptible |

Table: Legend for Locking Requirements Table

## The trylock Functions

There are functions that try to acquire a lock only once and immediately
return a value telling about success or failure to acquire the lock.
They can be used if you need no access to the data protected with the
lock when some other thread is holding the lock. You should acquire the
lock later if you then need access to the data protected with the lock.

`spin_trylock()` does not spin but returns non-zero if it
acquires the spinlock on the first try or 0 if not. This function can be
used in all contexts like `spin_lock()`: you must have
disabled the contexts that might interrupt you and acquire the spin
lock.

[`mutex_trylock()`](#c.mutex_trylock "mutex_trylock") does not suspend your task but returns
non-zero if it could lock the mutex on the first try or 0 if not. This
function cannot be safely used in hardware or software interrupt
contexts despite not sleeping.

## Common Examples

Let’s step through a simple example: a cache of number to name mappings.
The cache keeps a count of how often each of the objects is used, and
when it gets full, throws out the least used one.

### All In User Context

For our first example, we assume that all operations are in user context
(ie. from system calls), so we can sleep. This means we can use a mutex
to protect the cache and all the objects within it. Here’s the code:

```
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <asm/errno.h>

struct object
{
        struct list_head list;
        int id;
        char name[32];
        int popularity;
};

/* Protects the cache, cache_num, and the objects within it */
static DEFINE_MUTEX(cache_lock);
static LIST_HEAD(cache);
static unsigned int cache_num = 0;
#define MAX_CACHE_SIZE 10

/* Must be holding cache_lock */
static struct object *__cache_find(int id)
{
        struct object *i;

        list_for_each_entry(i, &cache, list)
                if (i->id == id) {
                        i->popularity++;
                        return i;
                }
        return NULL;
}

/* Must be holding cache_lock */
static void __cache_delete(struct object *obj)
{
        BUG_ON(!obj);
        list_del(&obj->list);
        kfree(obj);
        cache_num--;
}

/* Must be holding cache_lock */
static void __cache_add(struct object *obj)
{
        list_add(&obj->list, &cache);
        if (++cache_num > MAX_CACHE_SIZE) {
                struct object *i, *outcast = NULL;
                list_for_each_entry(i, &cache, list) {
                        if (!outcast || i->popularity < outcast->popularity)
                                outcast = i;
                }
                __cache_delete(outcast);
        }
}

int cache_add(int id, const char *name)
{
        struct object *obj;

        if ((obj = kmalloc(sizeof(*obj), GFP_KERNEL)) == NULL)
                return -ENOMEM;

        strscpy(obj->name, name, sizeof(obj->name));
        obj->id = id;
        obj->popularity = 0;

        mutex_lock(&cache_lock);
        __cache_add(obj);
        mutex_unlock(&cache_lock);
        return 0;
}

void cache_delete(int id)
{
        mutex_lock(&cache_lock);
        __cache_delete(__cache_find(id));
        mutex_unlock(&cache_lock);
}

int cache_find(int id, char *name)
{
        struct object *obj;
        int ret = -ENOENT;

        mutex_lock(&cache_lock);
        obj = __cache_find(id);
        if (obj) {
                ret = 0;
                strcpy(name, obj->name);
        }
        mutex_unlock(&cache_lock);
        return ret;
}
```

Note that we always make sure we have the cache\_lock when we add,
delete, or look up the cache: both the cache infrastructure itself and
the contents of the objects are protected by the lock. In this case it’s
easy, since we copy the data for the user, and never let them access the
objects directly.

There is a slight (and common) optimization here: in
`cache_add()` we set up the fields of the object before
grabbing the lock. This is safe, as no-one else can access it until we
put it in cache.

### Accessing From Interrupt Context

Now consider the case where `cache_find()` can be called
from interrupt context: either a hardware interrupt or a softirq. An
example would be a timer which deletes object from the cache.

The change is shown below, in standard patch format: the `-` are lines
which are taken away, and the `+` are lines which are added.

```
--- cache.c.usercontext 2003-12-09 13:58:54.000000000 +1100
+++ cache.c.interrupt   2003-12-09 14:07:49.000000000 +1100
@@ -12,7 +12,7 @@
         int popularity;
 };

-static DEFINE_MUTEX(cache_lock);
+static DEFINE_SPINLOCK(cache_lock);
 static LIST_HEAD(cache);
 static unsigned int cache_num = 0;
 #define MAX_CACHE_SIZE 10
@@ -55,6 +55,7 @@
 int cache_add(int id, const char *name)
 {
         struct object *obj;
+        unsigned long flags;

         if ((obj = kmalloc(sizeof(*obj), GFP_KERNEL)) == NULL)
                 return -ENOMEM;
@@ -63,30 +64,33 @@
         obj->id = id;
         obj->popularity = 0;

-        mutex_lock(&cache_lock);
+        spin_lock_irqsave(&cache_lock, flags);
         __cache_add(obj);
-        mutex_unlock(&cache_lock);
+        spin_unlock_irqrestore(&cache_lock, flags);
         return 0;
 }

 void cache_delete(int id)
 {
-        mutex_lock(&cache_lock);
+        unsigned long flags;
+
+        spin_lock_irqsave(&cache_lock, flags);
         __cache_delete(__cache_find(id));
-        mutex_unlock(&cache_lock);
+        spin_unlock_irqrestore(&cache_lock, flags);
 }

 int cache_find(int id, char *name)
 {
         struct object *obj;
         int ret = -ENOENT;
+        unsigned long flags;

-        mutex_lock(&cache_lock);
+        spin_lock_irqsave(&cache_lock, flags);
         obj = __cache_find(id);
         if (obj) {
                 ret = 0;
                 strcpy(name, obj->name);
         }
-        mutex_unlock(&cache_lock);
+        spin_unlock_irqrestore(&cache_lock, flags);
         return ret;
 }
```

Note that the `spin_lock_irqsave()` will turn off
interrupts if they are on, otherwise does nothing (if we are already in
an interrupt handler), hence these functions are safe to call from any
context.

Unfortunately, `cache_add()` calls [`kmalloc()`](../core-api/mm-api.html#c.kmalloc "kmalloc")
with the `GFP_KERNEL` flag, which is only legal in user context. I
have assumed that `cache_add()` is still only called in
user context, otherwise this should become a parameter to
`cache_add()`.

### Exposing Objects Outside This File

If our objects contained more information, it might not be sufficient to
copy the information in and out: other parts of the code might want to
keep pointers to these objects, for example, rather than looking up the
id every time. This produces two problems.

The first problem is that we use the `cache_lock` to protect objects:
we’d need to make this non-static so the rest of the code can use it.
This makes locking trickier, as it is no longer all in one place.

The second problem is the lifetime problem: if another structure keeps a
pointer to an object, it presumably expects that pointer to remain
valid. Unfortunately, this is only guaranteed while you hold the lock,
otherwise someone might call `cache_delete()` and even
worse, add another object, re-using the same address.

As there is only one lock, you can’t hold it forever: no-one else would
get any work done.

The solution to this problem is to use a reference count: everyone who
has a pointer to the object increases it when they first get the object,
and drops the reference count when they’re finished with it. Whoever
drops it to zero knows it is unused, and can actually delete it.

Here is the code:

```
--- cache.c.interrupt   2003-12-09 14:25:43.000000000 +1100
+++ cache.c.refcnt  2003-12-09 14:33:05.000000000 +1100
@@ -7,6 +7,7 @@
 struct object
 {
         struct list_head list;
+        unsigned int refcnt;
         int id;
         char name[32];
         int popularity;
@@ -17,6 +18,35 @@
 static unsigned int cache_num = 0;
 #define MAX_CACHE_SIZE 10

+static void __object_put(struct object *obj)
+{
+        if (--obj->refcnt == 0)
+                kfree(obj);
+}
+
+static void __object_get(struct object *obj)
+{
+        obj->refcnt++;
+}
+
+void object_put(struct object *obj)
+{
+        unsigned long flags;
+
+        spin_lock_irqsave(&cache_lock, flags);
+        __object_put(obj);
+        spin_unlock_irqrestore(&cache_lock, flags);
+}
+
+void object_get(struct object *obj)
+{
+        unsigned long flags;
+
+        spin_lock_irqsave(&cache_lock, flags);
+        __object_get(obj);
+        spin_unlock_irqrestore(&cache_lock, flags);
+}
+
 /* Must be holding cache_lock */
 static struct object *__cache_find(int id)
 {
@@ -35,6 +65,7 @@
 {
         BUG_ON(!obj);
         list_del(&obj->list);
+        __object_put(obj);
         cache_num--;
 }

@@ -63,6 +94,7 @@
         strscpy(obj->name, name, sizeof(obj->name));
         obj->id = id;
         obj->popularity = 0;
+        obj->refcnt = 1; /* The cache holds a reference */

         spin_lock_irqsave(&cache_lock, flags);
         __cache_add(obj);
@@ -79,18 +111,15 @@
         spin_unlock_irqrestore(&cache_lock, flags);
 }

-int cache_find(int id, char *name)
+struct object *cache_find(int id)
 {
         struct object *obj;
-        int ret = -ENOENT;
         unsigned long flags;

         spin_lock_irqsave(&cache_lock, flags);
         obj = __cache_find(id);
-        if (obj) {
-                ret = 0;
-                strcpy(name, obj->name);
-        }
+        if (obj)
+                __object_get(obj);
         spin_unlock_irqrestore(&cache_lock, flags);
-        return ret;
+        return obj;
 }
```

We encapsulate the reference counting in the standard ‘get’ and ‘put’
functions. Now we can return the object itself from
`cache_find()` which has the advantage that the user can
now sleep holding the object (eg. to `copy_to_user()` to
name to userspace).

The other point to note is that I said a reference should be held for
every pointer to the object: thus the reference count is 1 when first
inserted into the cache. In some versions the framework does not hold a
reference count, but they are more complicated.

#### Using Atomic Operations For The Reference Count

In practice, `atomic_t` would usually be used for refcnt. There are a
number of atomic operations defined in `include/asm/atomic.h`: these
are guaranteed to be seen atomically from all CPUs in the system, so no
lock is required. In this case, it is simpler than using spinlocks,
although for anything non-trivial using spinlocks is clearer. The
[`atomic_inc()`](../driver-api/basics.html#c.atomic_inc "atomic_inc") and [`atomic_dec_and_test()`](../driver-api/basics.html#c.atomic_dec_and_test "atomic_dec_and_test")
are used instead of the standard increment and decrement operators, and
the lock is no longer used to protect the reference count itself.

```
--- cache.c.refcnt  2003-12-09 15:00:35.000000000 +1100
+++ cache.c.refcnt-atomic   2003-12-11 15:49:42.000000000 +1100
@@ -7,7 +7,7 @@
 struct object
 {
         struct list_head list;
-        unsigned int refcnt;
+        atomic_t refcnt;
         int id;
         char name[32];
         int popularity;
@@ -18,33 +18,15 @@
 static unsigned int cache_num = 0;
 #define MAX_CACHE_SIZE 10

-static void __object_put(struct object *obj)
-{
-        if (--obj->refcnt == 0)
-                kfree(obj);
-}
-
-static void __object_get(struct object *obj)
-{
-        obj->refcnt++;
-}
-
 void object_put(struct object *obj)
 {
-        unsigned long flags;
-
-        spin_lock_irqsave(&cache_lock, flags);
-        __object_put(obj);
-        spin_unlock_irqrestore(&cache_lock, flags);
+        if (atomic_dec_and_test(&obj->refcnt))
+                kfree(obj);
 }

 void object_get(struct object *obj)
 {
-        unsigned long flags;
-
-        spin_lock_irqsave(&cache_lock, flags);
-        __object_get(obj);
-        spin_unlock_irqrestore(&cache_lock, flags);
+        atomic_inc(&obj->refcnt);
 }

 /* Must be holding cache_lock */
@@ -65,7 +47,7 @@
 {
         BUG_ON(!obj);
         list_del(&obj->list);
-        __object_put(obj);
+        object_put(obj);
         cache_num--;
 }

@@ -94,7 +76,7 @@
         strscpy(obj->name, name, sizeof(obj->name));
         obj->id = id;
         obj->popularity = 0;
-        obj->refcnt = 1; /* The cache holds a reference */
+        atomic_set(&obj->refcnt, 1); /* The cache holds a reference */

         spin_lock_irqsave(&cache_lock, flags);
         __cache_add(obj);
@@ -119,7 +101,7 @@
         spin_lock_irqsave(&cache_lock, flags);
         obj = __cache_find(id);
         if (obj)
-                __object_get(obj);
+                object_get(obj);
         spin_unlock_irqrestore(&cache_lock, flags);
         return obj;
 }
```

### Protecting The Objects Themselves

In these examples, we assumed that the objects (except the reference
counts) never changed once they are created. If we wanted to allow the
name to change, there are three possibilities:

* You can make `cache_lock` non-static, and tell people to grab that
  lock before changing the name in any object.
* You can provide a `cache_obj_rename()` which grabs this
  lock and changes the name for the caller, and tell everyone to use
  that function.
* You can make the `cache_lock` protect only the cache itself, and
  use another lock to protect the name.

Theoretically, you can make the locks as fine-grained as one lock for
every field, for every object. In practice, the most common variants
are:

* One lock which protects the infrastructure (the `cache` list in
  this example) and all the objects. This is what we have done so far.
* One lock which protects the infrastructure (including the list
  pointers inside the objects), and one lock inside the object which
  protects the rest of that object.
* Multiple locks to protect the infrastructure (eg. one lock per hash
  chain), possibly with a separate per-object lock.

Here is the “lock-per-object” implementation:

```
--- cache.c.refcnt-atomic   2003-12-11 15:50:54.000000000 +1100
+++ cache.c.perobjectlock   2003-12-11 17:15:03.000000000 +1100
@@ -6,11 +6,17 @@

 struct object
 {
+        /* These two protected by cache_lock. */
         struct list_head list;
+        int popularity;
+
         atomic_t refcnt;
+
+        /* Doesn't change once created. */
         int id;
+
+        spinlock_t lock; /* Protects the name */
         char name[32];
-        int popularity;
 };

 static DEFINE_SPINLOCK(cache_lock);
@@ -77,6 +84,7 @@
         obj->id = id;
         obj->popularity = 0;
         atomic_set(&obj->refcnt, 1); /* The cache holds a reference */
+        spin_lock_init(&obj->lock);

         spin_lock_irqsave(&cache_lock, flags);
         __cache_add(obj);
```

Note that I decide that the popularity count should be protected by the
`cache_lock` rather than the per-object lock: this is because it (like
the `struct list_head` inside the object)
is logically part of the infrastructure. This way, I don’t need to grab
the lock of every object in `__cache_add()` when seeking
the least popular.

I also decided that the id member is unchangeable, so I don’t need to
grab each object lock in `__cache_find()` to examine the
id: the object lock is only used by a caller who wants to read or write
the name field.

Note also that I added a comment describing what data was protected by
which locks. This is extremely important, as it describes the runtime
behavior of the code, and can be hard to gain from just reading. And as
Alan Cox says, “Lock data, not code”.

## Common Problems

### Deadlock: Simple and Advanced

There is a coding bug where a piece of code tries to grab a spinlock
twice: it will spin forever, waiting for the lock to be released
(spinlocks, rwlocks and mutexes are not recursive in Linux). This is
trivial to diagnose: not a
stay-up-five-nights-talk-to-fluffy-code-bunnies kind of problem.

For a slightly more complex case, imagine you have a region shared by a
softirq and user context. If you use a `spin_lock()` call
to protect it, it is possible that the user context will be interrupted
by the softirq while it holds the lock, and the softirq will then spin
forever trying to get the same lock.

Both of these are called deadlock, and as shown above, it can occur even
with a single CPU (although not on UP compiles, since spinlocks vanish
on kernel compiles with `CONFIG_SMP`=n. You’ll still get data
corruption in the second example).

This complete lockup is easy to diagnose: on SMP boxes the watchdog
timer or compiling with `DEBUG_SPINLOCK` set
(`include/linux/spinlock.h`) will show this up immediately when it
happens.

A more complex problem is the so-called ‘deadly embrace’, involving two
or more locks. Say you have a hash table: each entry in the table is a
spinlock, and a chain of hashed objects. Inside a softirq handler, you
sometimes want to alter an object from one place in the hash to another:
you grab the spinlock of the old hash chain and the spinlock of the new
hash chain, and delete the object from the old one, and insert it in the
new one.

There are two problems here. First, if your code ever tries to move the
object to the same chain, it will deadlock with itself as it tries to
lock it twice. Secondly, if the same softirq on another CPU is trying to
move another object in the reverse direction, the following could
happen:

| CPU 1 | CPU 2 |
| --- | --- |
| Grab lock A -> OK | Grab lock B -> OK |
| Grab lock B -> spin | Grab lock A -> spin |

Table: Consequences

The two CPUs will spin forever, waiting for the other to give up their
lock. It will look, smell, and feel like a crash.

### Preventing Deadlock

Textbooks will tell you that if you always lock in the same order, you
will never get this kind of deadlock. Practice will tell you that this
approach doesn’t scale: when I create a new lock, I don’t understand
enough of the kernel to figure out where in the 5000 lock hierarchy it
will fit.

The best locks are encapsulated: they never get exposed in headers, and
are never held around calls to non-trivial functions outside the same
file. You can read through this code and see that it will never
deadlock, because it never tries to grab another lock while it has that
one. People using your code don’t even need to know you are using a
lock.

A classic problem here is when you provide callbacks or hooks: if you
call these with the lock held, you risk simple deadlock, or a deadly
embrace (who knows what the callback will do?).

#### Overzealous Prevention Of Deadlocks

Deadlocks are problematic, but not as bad as data corruption. Code which
grabs a read lock, searches a list, fails to find what it wants, drops
the read lock, grabs a write lock and inserts the object has a race
condition.

### Racing Timers: A Kernel Pastime

Timers can produce their own special problems with races. Consider a
collection of objects (list, hash, etc) where each object has a timer
which is due to destroy it.

If you want to destroy the entire collection (say on module removal),
you might do the following:

```
/* THIS CODE BAD BAD BAD BAD: IF IT WAS ANY WORSE IT WOULD USE
   HUNGARIAN NOTATION */
spin_lock_bh(&list_lock);

while (list) {
        struct foo *next = list->next;
        timer_delete(&list->timer);
        kfree(list);
        list = next;
}

spin_unlock_bh(&list_lock);
```

Sooner or later, this will crash on SMP, because a timer can have just
gone off before the `spin_lock_bh()`, and it will only get
the lock after we `spin_unlock_bh()`, and then try to free
the element (which has already been freed!).

This can be avoided by checking the result of
[`timer_delete()`](../driver-api/basics.html#c.timer_delete "timer_delete"): if it returns 1, the timer has been deleted.
If 0, it means (in this case) that it is currently running, so we can
do:

```
retry:
        spin_lock_bh(&list_lock);

        while (list) {
                struct foo *next = list->next;
                if (!timer_delete(&list->timer)) {
                        /* Give timer a chance to delete this */
                        spin_unlock_bh(&list_lock);
                        goto retry;
                }
                kfree(list);
                list = next;
        }

        spin_unlock_bh(&list_lock);
```

Another common problem is deleting timers which restart themselves (by
calling [`add_timer()`](../driver-api/basics.html#c.add_timer "add_timer") at the end of their timer function).
Because this is a fairly common case which is prone to races, you should
use [`timer_delete_sync()`](../driver-api/basics.html#c.timer_delete_sync "timer_delete_sync") (`include/linux/timer.h`) to handle this case.

Before freeing a timer, [`timer_shutdown()`](../driver-api/basics.html#c.timer_shutdown "timer_shutdown") or [`timer_shutdown_sync()`](../driver-api/basics.html#c.timer_shutdown_sync "timer_shutdown_sync") should be
called which will keep it from being rearmed. Any subsequent attempt to
rearm the timer will be silently ignored by the core code.

## Locking Speed

There are three main things to worry about when considering speed of
some code which does locking. First is concurrency: how many things are
going to be waiting while someone else is holding a lock. Second is the
time taken to actually acquire and release an uncontended lock. Third is
using fewer, or smarter locks. I’m assuming that the lock is used fairly
often: otherwise, you wouldn’t be concerned about efficiency.

Concurrency depends on how long the lock is usually held: you should
hold the lock for as long as needed, but no longer. In the cache
example, we always create the object without the lock held, and then
grab the lock only when we are ready to insert it in the list.

Acquisition times depend on how much damage the lock operations do to
the pipeline (pipeline stalls) and how likely it is that this CPU was
the last one to grab the lock (ie. is the lock cache-hot for this CPU):
on a machine with more CPUs, this likelihood drops fast. Consider a
700MHz Intel Pentium III: an instruction takes about 0.7ns, an atomic
increment takes about 58ns, a lock which is cache-hot on this CPU takes
160ns, and a cacheline transfer from another CPU takes an additional 170
to 360ns. (These figures from Paul McKenney’s [Linux Journal RCU
article](http://www.linuxjournal.com/article.php?sid=6993)).

These two aims conflict: holding a lock for a short time might be done
by splitting locks into parts (such as in our final per-object-lock
example), but this increases the number of lock acquisitions, and the
results are often slower than having a single lock. This is another
reason to advocate locking simplicity.

The third concern is addressed below: there are some methods to reduce
the amount of locking which needs to be done.

### Read/Write Lock Variants

Both spinlocks and mutexes have read/write variants: `rwlock_t` and
`struct rw_semaphore`. These divide
users into two classes: the readers and the writers. If you are only
reading the data, you can get a read lock, but to write to the data you
need the write lock. Many people can hold a read lock, but a writer must
be sole holder.

If your code divides neatly along reader/writer lines (as our cache code
does), and the lock is held by readers for significant lengths of time,
using these locks can help. They are slightly slower than the normal
locks though, so in practice `rwlock_t` is not usually worthwhile.

### Avoiding Locks: Read Copy Update

There is a special method of read/write locking called Read Copy Update.
Using RCU, the readers can avoid taking a lock altogether: as we expect
our cache to be read more often than updated (otherwise the cache is a
waste of time), it is a candidate for this optimization.

How do we get rid of read locks? Getting rid of read locks means that
writers may be changing the list underneath the readers. That is
actually quite simple: we can read a linked list while an element is
being added if the writer adds the element very carefully. For example,
adding `new` to a single linked list called `list`:

```
new->next = list->next;
wmb();
list->next = new;
```

The `wmb()` is a write memory barrier. It ensures that the
first operation (setting the new element’s `next` pointer) is complete
and will be seen by all CPUs, before the second operation is (putting
the new element into the list). This is important, since modern
compilers and modern CPUs can both reorder instructions unless told
otherwise: we want a reader to either not see the new element at all, or
see the new element with the `next` pointer correctly pointing at the
rest of the list.

Fortunately, there is a function to do this for standard
`struct list_head` lists:
[`list_add_rcu()`](../core-api/kernel-api.html#c.list_add_rcu "list_add_rcu") (`include/linux/list.h`).

Removing an element from the list is even simpler: we replace the
pointer to the old element with a pointer to its successor, and readers
will either see it, or skip over it.

```
list->next = old->next;
```

There is [`list_del_rcu()`](../core-api/kernel-api.html#c.list_del_rcu "list_del_rcu") (`include/linux/list.h`) which
does this (the normal version poisons the old object, which we don’t
want).

The reader must also be careful: some CPUs can look through the `next`
pointer to start reading the contents of the next element early, but
don’t realize that the pre-fetched contents is wrong when the `next`
pointer changes underneath them. Once again, there is a
[`list_for_each_entry_rcu()`](../core-api/kernel-api.html#c.list_for_each_entry_rcu "list_for_each_entry_rcu") (`include/linux/list.h`)
to help you. Of course, writers can just use
[`list_for_each_entry()`](../core-api/list.html#c.list_for_each_entry "list_for_each_entry"), since there cannot be two
simultaneous writers.

Our final dilemma is this: when can we actually destroy the removed
element? Remember, a reader might be stepping through this element in
the list right now: if we free this element and the `next` pointer
changes, the reader will jump off into garbage and crash. We need to
wait until we know that all the readers who were traversing the list
when we deleted the element are finished. We use
[`call_rcu()`](../core-api/kernel-api.html#c.call_rcu "call_rcu") to register a callback which will actually
destroy the object once all pre-existing readers are finished.
Alternatively, [`synchronize_rcu()`](../core-api/kernel-api.html#c.synchronize_rcu "synchronize_rcu") may be used to block
until all pre-existing are finished.

But how does Read Copy Update know when the readers are finished? The
method is this: firstly, the readers always traverse the list inside
[`rcu_read_lock()`](../core-api/kernel-api.html#c.rcu_read_lock "rcu_read_lock")/[`rcu_read_unlock()`](../core-api/kernel-api.html#c.rcu_read_unlock "rcu_read_unlock") pairs:
these simply disable preemption so the reader won’t go to sleep while
reading the list.

RCU then waits until every other CPU has slept at least once: since
readers cannot sleep, we know that any readers which were traversing the
list during the deletion are finished, and the callback is triggered.
The real Read Copy Update code is a little more optimized than this, but
this is the fundamental idea.

```
--- cache.c.perobjectlock   2003-12-11 17:15:03.000000000 +1100
+++ cache.c.rcupdate    2003-12-11 17:55:14.000000000 +1100
@@ -1,15 +1,18 @@
 #include <linux/list.h>
 #include <linux/slab.h>
 #include <linux/string.h>
+#include <linux/rcupdate.h>
 #include <linux/mutex.h>
 #include <asm/errno.h>

 struct object
 {
-        /* These two protected by cache_lock. */
+        /* This is protected by RCU */
         struct list_head list;
         int popularity;

+        struct rcu_head rcu;
+
         atomic_t refcnt;

         /* Doesn't change once created. */
@@ -40,7 +43,7 @@
 {
         struct object *i;

-        list_for_each_entry(i, &cache, list) {
+        list_for_each_entry_rcu(i, &cache, list) {
                 if (i->id == id) {
                         i->popularity++;
                         return i;
@@ -49,19 +52,25 @@
         return NULL;
 }

+/* Final discard done once we know no readers are looking. */
+static void cache_delete_rcu(void *arg)
+{
+        object_put(arg);
+}
+
 /* Must be holding cache_lock */
 static void __cache_delete(struct object *obj)
 {
         BUG_ON(!obj);
-        list_del(&obj->list);
-        object_put(obj);
+        list_del_rcu(&obj->list);
         cache_num--;
+        call_rcu(&obj->rcu, cache_delete_rcu);
 }

 /* Must be holding cache_lock */
 static void __cache_add(struct object *obj)
 {
-        list_add(&obj->list, &cache);
+        list_add_rcu(&obj->list, &cache);
         if (++cache_num > MAX_CACHE_SIZE) {
                 struct object *i, *outcast = NULL;
                 list_for_each_entry(i, &cache, list) {
@@ -104,12 +114,11 @@
 struct object *cache_find(int id)
 {
         struct object *obj;
-        unsigned long flags;

-        spin_lock_irqsave(&cache_lock, flags);
+        rcu_read_lock();
         obj = __cache_find(id);
         if (obj)
                 object_get(obj);
-        spin_unlock_irqrestore(&cache_lock, flags);
+        rcu_read_unlock();
         return obj;
 }
```

Note that the reader will alter the popularity member in
`__cache_find()`, and now it doesn’t hold a lock. One
solution would be to make it an `atomic_t`, but for this usage, we
don’t really care about races: an approximate result is good enough, so
I didn’t change it.

The result is that `cache_find()` requires no
synchronization with any other functions, so is almost as fast on SMP as
it would be on UP.

There is a further optimization possible here: remember our original
cache code, where there were no reference counts and the caller simply
held the lock whenever using the object? This is still possible: if you
hold the lock, no one can delete the object, so you don’t need to get
and put the reference count.

Now, because the ‘read lock’ in RCU is simply disabling preemption, a
caller which always has preemption disabled between calling
`cache_find()` and `object_put()` does not
need to actually get and put the reference count: we could expose
`__cache_find()` by making it non-static, and such
callers could simply call that.

The benefit here is that the reference count is not written to: the
object is not altered in any way, which is much faster on SMP machines
due to caching.

### Per-CPU Data

Another technique for avoiding locking which is used fairly widely is to
duplicate information for each CPU. For example, if you wanted to keep a
count of a common condition, you could use a spin lock and a single
counter. Nice and simple.

If that was too slow (it’s usually not, but if you’ve got a really big
machine to test on and can show that it is), you could instead use a
counter for each CPU, then none of them need an exclusive lock. See
`DEFINE_PER_CPU()`, `get_cpu_var()` and
`put_cpu_var()` (`include/linux/percpu.h`).

Of particular use for simple per-cpu counters is the `local_t` type,
and the `cpu_local_inc()` and related functions, which are
more efficient than simple code on some architectures
(`include/asm/local.h`).

Note that there is no simple, reliable way of getting an exact value of
such a counter, without introducing more locks. This is not a problem
for some uses.

### Data Which Mostly Used By An IRQ Handler

If data is always accessed from within the same IRQ handler, you don’t
need a lock at all: the kernel already guarantees that the irq handler
will not run simultaneously on multiple CPUs.

Manfred Spraul points out that you can still do this, even if the data
is very occasionally accessed in user context or softirqs/tasklets. The
irq handler doesn’t use a lock, and all other accesses are done as so:

```
mutex_lock(&lock);
disable_irq(irq);
...
enable_irq(irq);
mutex_unlock(&lock);
```

The [`disable_irq()`](../core-api/genericirq.html#c.disable_irq "disable_irq") prevents the irq handler from running
(and waits for it to finish if it’s currently running on other CPUs).
The spinlock prevents any other accesses happening at the same time.
Naturally, this is slower than just a `spin_lock_irq()`
call, so it only makes sense if this type of access happens extremely
rarely.

## What Functions Are Safe To Call From Interrupts?

Many functions in the kernel sleep (ie. call `schedule()`) directly or
indirectly: you can never call them while holding a spinlock, or with
preemption disabled. This also means you need to be in user context:
calling them from an interrupt is illegal.

### Some Functions Which Sleep

The most common ones are listed below, but you usually have to read the
code to find out if other calls are safe. If everyone else who calls it
can sleep, you probably need to be able to sleep, too. In particular,
registration and deregistration functions usually expect to be called
from user context, and can sleep.

* Accesses to userspace:

  + `copy_from_user()`
  + `copy_to_user()`
  + [`get_user()`](../core-api/mm-api.html#c.get_user "get_user")
  + [`put_user()`](../core-api/mm-api.html#c.put_user "put_user")
* kmalloc(GP\_KERNEL) <kmalloc>`
* [`mutex_lock_interruptible()`](#c.mutex_lock_interruptible "mutex_lock_interruptible") and
  [`mutex_lock()`](#c.mutex_lock "mutex_lock")

  There is a [`mutex_trylock()`](#c.mutex_trylock "mutex_trylock") which does not sleep.
  Still, it must not be used inside interrupt context since its
  implementation is not safe for that. [`mutex_unlock()`](#c.mutex_unlock "mutex_unlock")
  will also never sleep. It cannot be used in interrupt context either
  since a mutex must be released by the same task that acquired it.

### Some Functions Which Don’t Sleep

Some functions are safe to call from any context, or holding almost any
lock.

* [`printk()`](../core-api/printk-basics.html#c.printk "printk")
* [`kfree()`](../core-api/mm-api.html#c.kfree "kfree")
* [`add_timer()`](../driver-api/basics.html#c.add_timer "add_timer") and [`timer_delete()`](../driver-api/basics.html#c.timer_delete "timer_delete")

## Mutex API reference

mutex\_init

`mutex_init (mutex)`

> initialize the mutex

**Parameters**

`mutex`
:   the mutex to be initialized

**Description**

Initialize the mutex to unlocked state.

It is not allowed to initialize an already locked mutex.

mutex\_init\_with\_key

`mutex_init_with_key (mutex, key)`

> initialize a mutex with a given lockdep key

**Parameters**

`mutex`
:   the mutex to be initialized

`key`
:   the lockdep key to be associated with the mutex

**Description**

Initialize the mutex to the unlocked state.

It is not allowed to initialize an already locked mutex.

bool mutex\_is\_locked(struct mutex \*lock)
:   is the mutex locked

**Parameters**

`struct mutex *lock`
:   the mutex to be queried

**Description**

Returns true if the mutex is locked, false if unlocked.

void mutex\_lock(struct mutex \*lock)
:   acquire the mutex

**Parameters**

`struct mutex *lock`
:   the mutex to be acquired

**Description**

Lock the mutex exclusively for this task. If the mutex is not
available right now, it will sleep until it can get it.

The mutex must later on be released by the same task that
acquired it. Recursive locking is not allowed. The task
may not exit without first unlocking the mutex. Also, kernel
memory where the mutex resides must not be freed with
the mutex still locked. The mutex must first be initialized
(or statically defined) before it can be locked. [`memset()`](../core-api/kernel-api.html#c.memset "memset")-ing
the mutex to 0 is not allowed.

(The CONFIG\_DEBUG\_MUTEXES .config option turns on debugging
checks that will enforce the restrictions and will also do
deadlock debugging)

This function is similar to (but not equivalent to) `down()`.

void mutex\_unlock(struct mutex \*lock)
:   release the mutex

**Parameters**

`struct mutex *lock`
:   the mutex to be released

**Description**

Unlock a mutex that has been locked by this task previously.

This function must not be used in interrupt context. Unlocking
of a not locked mutex is not allowed.

The caller must ensure that the mutex stays alive until this function has
returned - [`mutex_unlock()`](#c.mutex_unlock "mutex_unlock") can NOT directly be used to release an object such
that another concurrent task can free it.
Mutexes are different from spinlocks & refcounts in this aspect.

This function is similar to (but not equivalent to) `up()`.

void ww\_mutex\_unlock(struct ww\_mutex \*lock)
:   release the w/w mutex

**Parameters**

`struct ww_mutex *lock`
:   the mutex to be released

**Description**

Unlock a mutex that has been locked by this task previously with any of the
ww\_mutex\_lock\* functions (with or without an acquire context). It is
forbidden to release the locks after releasing the acquire context.

This function must not be used in interrupt context. Unlocking
of a unlocked mutex is not allowed.

int ww\_mutex\_trylock(struct ww\_mutex \*ww, struct ww\_acquire\_ctx \*ww\_ctx)
:   tries to acquire the w/w mutex with optional acquire context

**Parameters**

`struct ww_mutex *ww`
:   mutex to lock

`struct ww_acquire_ctx *ww_ctx`
:   optional w/w acquire context

**Description**

Trylocks a mutex with the optional acquire context; no deadlock detection is
possible. Returns 1 if the mutex has been acquired successfully, 0 otherwise.

Unlike ww\_mutex\_lock, no deadlock handling is performed. However, if a **ctx** is
specified, -EALREADY handling may happen in calls to ww\_mutex\_trylock.

A mutex acquired with this function must be released with ww\_mutex\_unlock.

int mutex\_lock\_interruptible(struct mutex \*lock)
:   Acquire the mutex, interruptible by signals.

**Parameters**

`struct mutex *lock`
:   The mutex to be acquired.

**Description**

Lock the mutex like [`mutex_lock()`](#c.mutex_lock "mutex_lock"). If a signal is delivered while the
process is sleeping, this function will return without acquiring the
mutex.

**Context**

Process context.

**Return**

0 if the lock was successfully acquired or `-EINTR` if a
signal arrived.

int mutex\_lock\_killable(struct mutex \*lock)
:   Acquire the mutex, interruptible by fatal signals.

**Parameters**

`struct mutex *lock`
:   The mutex to be acquired.

**Description**

Lock the mutex like [`mutex_lock()`](#c.mutex_lock "mutex_lock"). If a signal which will be fatal to
the current process is delivered while the process is sleeping, this
function will return without acquiring the mutex.

**Context**

Process context.

**Return**

0 if the lock was successfully acquired or `-EINTR` if a
fatal signal arrived.

void mutex\_lock\_io(struct mutex \*lock)
:   Acquire the mutex and mark the process as waiting for I/O

**Parameters**

`struct mutex *lock`
:   The mutex to be acquired.

**Description**

Lock the mutex like [`mutex_lock()`](#c.mutex_lock "mutex_lock"). While the task is waiting for this
mutex, it will be accounted as being in the IO wait state by the
scheduler.

**Context**

Process context.

int mutex\_trylock(struct mutex \*lock)
:   try to acquire the mutex, without waiting

**Parameters**

`struct mutex *lock`
:   the mutex to be acquired

**Description**

Try to acquire the mutex atomically. Returns 1 if the mutex
has been acquired successfully, and 0 on contention.

**NOTE**

this function follows the `spin_trylock()` convention, so
it is negated from the `down_trylock()` return values! Be careful
about this when converting semaphore users to mutexes.

This function must not be used in interrupt context. The
mutex must be released by the same task that acquired it.

int atomic\_dec\_and\_mutex\_lock(atomic\_t \*cnt, struct mutex \*lock)
:   return holding mutex if we dec to 0

**Parameters**

`atomic_t *cnt`
:   the atomic which we are to dec

`struct mutex *lock`
:   the mutex to return holding if we dec to 0

**Description**

return true and hold lock if we dec to 0, return false otherwise

## Futex API reference

void futex\_hash\_get(struct futex\_hash\_bucket \*hb)
:   Get an additional reference for the local hash.

**Parameters**

`struct futex_hash_bucket *hb`
:   ptr to the private local hash.

**Description**

Obtain an additional reference for the already obtained hash bucket. The
caller must already own an reference.

struct futex\_hash\_bucket \*\_\_futex\_hash(union futex\_key \*key, struct futex\_private\_hash \*fph)
:   Return the hash bucket

**Parameters**

`union futex_key *key`
:   Pointer to the futex key for which the hash is calculated

`struct futex_private_hash *fph`
:   Pointer to private hash if known

**Description**

We hash on the keys returned from get\_futex\_key (see below) and return the
corresponding hash bucket.
If the FUTEX is PROCESS\_PRIVATE then a per-process hash bucket (from the
private hash) is returned if existing. Otherwise a hash bucket from the
global hash is returned.

struct [hrtimer\_sleeper](../driver-api/basics.html#c.hrtimer_sleeper "hrtimer_sleeper") \*futex\_setup\_timer(ktime\_t \*time, struct [hrtimer\_sleeper](../driver-api/basics.html#c.hrtimer_sleeper "hrtimer_sleeper") \*timeout, int flags, u64 range\_ns)
:   set up the sleeping hrtimer.

**Parameters**

`ktime_t *time`
:   ptr to the given timeout value

`struct hrtimer_sleeper *timeout`
:   the hrtimer\_sleeper structure to be set up

`int flags`
:   futex flags

`u64 range_ns`
:   optional range in ns

**Return**

Initialized hrtimer\_sleeper structure or NULL if no timeout
value given

int get\_futex\_key(u32 \_\_user \*uaddr, unsigned int flags, union futex\_key \*key, enum futex\_access rw)
:   Get parameters which are the keys for a futex

**Parameters**

`u32 __user *uaddr`
:   virtual address of the futex

`unsigned int flags`
:   FLAGS\_\*

`union futex_key *key`
:   address where result is stored.

`enum futex_access rw`
:   mapping needs to be read/write (values: FUTEX\_READ,
    FUTEX\_WRITE)

**Return**

a negative error code or 0

**Description**

The key words are stored in **key** on success.

For shared mappings (when **fshared**), the key is:

> ( inode->i\_sequence, page offset within mapping, offset\_within\_page )

[ also see `get_inode_sequence_number()` ]

For private mappings (or when **!fshared**), the key is:

> ( current->mm, address, 0 )

This allows (cross process, where applicable) identification of the futex
without keeping the page pinned for the duration of the FUTEX\_WAIT.

[`lock_page()`](../core-api/mm-api.html#c.lock_page "lock_page") might sleep, the caller should not hold a spinlock.

int fault\_in\_user\_writeable(u32 \_\_user \*uaddr)
:   Fault in user address and verify RW access

**Parameters**

`u32 __user *uaddr`
:   pointer to faulting user space address

**Description**

Slow path to fixup the fault we just took in the atomic write
access to **uaddr**.

We have no generic implementation of a non-destructive write to the
user address. We know that we faulted in the atomic pagefault
disabled section so we can as well avoid the #PF overhead by
calling `get_user_pages()` right away.

struct [futex\_q](#c.futex_q "futex_q") \*futex\_top\_waiter(struct futex\_hash\_bucket \*hb, union futex\_key \*key)
:   Return the highest priority waiter on a futex

**Parameters**

`struct futex_hash_bucket *hb`
:   the hash bucket the futex\_q’s reside in

`union futex_key *key`
:   the futex key (to distinguish it from other futex futex\_q’s)

**Description**

Must be called with the hb lock held.

void wait\_for\_owner\_exiting(int ret, struct task\_struct \*exiting)
:   Block until the owner has exited

**Parameters**

`int ret`
:   owner’s current futex lock status

`struct task_struct *exiting`
:   Pointer to the exiting task

**Description**

Caller must hold a refcount on **exiting**.

void \_\_futex\_unqueue(struct [futex\_q](#c.futex_q "futex_q") \*q)
:   Remove the futex\_q from its futex\_hash\_bucket

**Parameters**

`struct futex_q *q`
:   The futex\_q to unqueue

**Description**

The q->lock\_ptr must not be NULL and must be held by the caller.

int futex\_unqueue(struct [futex\_q](#c.futex_q "futex_q") \*q)
:   Remove the futex\_q from its futex\_hash\_bucket

**Parameters**

`struct futex_q *q`
:   The futex\_q to unqueue

**Description**

The q->lock\_ptr must not be held by the caller. A call to [`futex_unqueue()`](#c.futex_unqueue "futex_unqueue") must
be paired with exactly one earlier call to [`futex_queue()`](#c.futex_queue "futex_queue").

**Return**

* 1 - if the futex\_q was still queued (and we removed unqueued it);
* 0 - if the futex\_q was already removed by the waking thread

void futex\_exit\_recursive(struct task\_struct \*tsk)
:   Set the tasks futex state to FUTEX\_STATE\_DEAD

**Parameters**

`struct task_struct *tsk`
:   task to set the state on

**Description**

Set the futex exit state of the task lockless. The futex waiter code
observes that state when a task is exiting and loops until the task has
actually finished the futex cleanup. The worst case for this is that the
waiter runs through the wait loop until the state becomes visible.

This is called from the recursive fault handling path in `make_task_dead()`.

This is best effort. Either the futex exit code has run already or
not. If the OWNER\_DIED bit has been set on the futex then the waiter can
take it over. If not, the problem is pushed back to user space. If the
futex exit code did not run yet, then an already queued waiter might
block forever, but there is nothing which can be done about that.

struct futex\_q
:   The hashed futex queue entry, one per waiting task

**Definition**:

```
struct futex_q {
    struct plist_node list;
    struct task_struct *task;
    spinlock_t *lock_ptr;
    futex_wake_fn *wake;
    void *wake_data;
    union futex_key key;
    struct futex_pi_state *pi_state;
    struct rt_mutex_waiter *rt_waiter;
    union futex_key *requeue_pi_key;
    u32 bitset;
    atomic_t requeue_state;
    bool drop_hb_ref;
#ifdef CONFIG_PREEMPT_RT;
    struct rcuwait requeue_wait;
#endif;
};
```

**Members**

`list`
:   priority-sorted list of tasks waiting on this futex

`task`
:   the task waiting on the futex

`lock_ptr`
:   the hash bucket lock

`wake`
:   the wake handler for this queue

`wake_data`
:   data associated with the wake handler

`key`
:   the key the futex is hashed on

`pi_state`
:   optional priority inheritance state

`rt_waiter`
:   rt\_waiter storage for use with requeue\_pi

`requeue_pi_key`
:   the requeue\_pi target futex key

`bitset`
:   bitset for the optional bitmasked wakeup

`requeue_state`
:   State field for `futex_requeue_pi()`

`drop_hb_ref`
:   Waiter should drop the extra hash bucket reference if true

`requeue_wait`
:   RCU wait for `futex_requeue_pi()` (RT only)

**Description**

We use this hashed waitqueue, instead of a normal wait\_queue\_entry\_t, so
we can wake only the relevant ones (hashed queues may be shared).

A futex\_q has a woken state, just like tasks have TASK\_RUNNING.
It is considered woken when plist\_node\_empty(`q->list`) || q->lock\_ptr == 0.
The order of wakeup is always to make the first condition true, then
the second.

PI futexes are typically woken before they are removed from the hash list via
the rt\_mutex code. See `futex_unqueue_pi()`.

int futex\_match(union futex\_key \*key1, union futex\_key \*key2)
:   Check whether two futex keys are equal

**Parameters**

`union futex_key *key1`
:   Pointer to key1

`union futex_key *key2`
:   Pointer to key2

**Description**

Return 1 if two futex\_keys are equal, 0 otherwise.

void futex\_queue(struct [futex\_q](#c.futex_q "futex_q") \*q, struct futex\_hash\_bucket \*hb, struct task\_struct \*task)
:   Enqueue the futex\_q on the futex\_hash\_bucket

**Parameters**

`struct futex_q *q`
:   The futex\_q to enqueue

`struct futex_hash_bucket *hb`
:   The destination hash bucket

`struct task_struct *task`
:   Task queueing this futex

**Description**

The hb->lock must be held by the caller, and is released here. A call to
[`futex_queue()`](#c.futex_queue "futex_queue") is typically paired with exactly one call to [`futex_unqueue()`](#c.futex_unqueue "futex_unqueue"). The
exceptions involve the PI related operations, which may use `futex_unqueue_pi()`
or nothing if the unqueue is done as part of the wake process and the unqueue
state is implicit in the state of woken task (see [`futex_wait_requeue_pi()`](#c.futex_wait_requeue_pi "futex_wait_requeue_pi") for
an example).

Note that **task** may be NULL, for async usage of futexes.

struct futex\_vector
:   Auxiliary struct for `futex_waitv()`

**Definition**:

```
struct futex_vector {
    struct futex_waitv w;
    struct futex_q q;
};
```

**Members**

`w`
:   Userspace provided data

`q`
:   Kernel side data

**Description**

Struct used to build an array with all data need for `futex_waitv()`

int futex\_lock\_pi\_atomic(u32 \_\_user \*uaddr, struct futex\_hash\_bucket \*hb, union futex\_key \*key, struct futex\_pi\_state \*\*ps, struct task\_struct \*task, struct task\_struct \*\*exiting, int set\_waiters)
:   Atomic work required to acquire a pi aware futex

**Parameters**

`u32 __user *uaddr`
:   the pi futex user address

`struct futex_hash_bucket *hb`
:   the pi futex hash bucket

`union futex_key *key`
:   the futex key associated with uaddr and hb

`struct futex_pi_state **ps`
:   the pi\_state pointer where we store the result of the
    lookup

`struct task_struct *task`
:   the task to perform the atomic lock work for. This will
    be “current” except in the case of requeue pi.

`struct task_struct **exiting`
:   Pointer to store the task pointer of the owner task
    which is in the middle of exiting

`int set_waiters`
:   force setting the FUTEX\_WAITERS bit (1) or not (0)

**Return**

* 0 - ready to wait;
* 1 - acquired the lock;
* <0 - error

**Description**

The hb->lock must be held by the caller.

**exiting** is only set when the return value is -EBUSY. If so, this holds
a refcount on the exiting task on return and the caller needs to drop it
after waiting for the exit to complete.

int fixup\_pi\_owner(u32 \_\_user \*uaddr, struct [futex\_q](#c.futex_q "futex_q") \*q, int locked)
:   Post lock pi\_state and corner case management

**Parameters**

`u32 __user *uaddr`
:   user address of the futex

`struct futex_q *q`
:   futex\_q (contains pi\_state and access to the rt\_mutex)

`int locked`
:   if the attempt to take the rt\_mutex succeeded (1) or not (0)

**Description**

After attempting to lock an rt\_mutex, this function is called to cleanup
the pi\_state owner as well as handle race conditions that may allow us to
acquire the lock. Must be called with the hb lock held.

**Return**

* 1 - success, lock taken;
* 0 - success, lock not taken;
* <0 - on error (-EFAULT)

void requeue\_futex(struct [futex\_q](#c.futex_q "futex_q") \*q, struct futex\_hash\_bucket \*hb1, struct futex\_hash\_bucket \*hb2, union futex\_key \*key2)
:   Requeue a futex\_q from one hb to another

**Parameters**

`struct futex_q *q`
:   the futex\_q to requeue

`struct futex_hash_bucket *hb1`
:   the source hash\_bucket

`struct futex_hash_bucket *hb2`
:   the target hash\_bucket

`union futex_key *key2`
:   the new key for the requeued futex\_q

void requeue\_pi\_wake\_futex(struct [futex\_q](#c.futex_q "futex_q") \*q, union futex\_key \*key, struct futex\_hash\_bucket \*hb)
:   Wake a task that acquired the lock during requeue

**Parameters**

`struct futex_q *q`
:   the futex\_q

`union futex_key *key`
:   the key of the requeue target futex

`struct futex_hash_bucket *hb`
:   the hash\_bucket of the requeue target futex

**Description**

During futex\_requeue, with requeue\_pi=1, it is possible to acquire the
target futex if it is uncontended or via a lock steal.

1. Set **q**::key to the requeue target futex key so the waiter can detect
   the wakeup on the right futex.
2. Dequeue **q** from the hash bucket.
3. Set **q**::rt\_waiter to NULL so the woken up task can detect atomic lock
   acquisition.
4. Set the q->lock\_ptr to the requeue target hb->lock for the case that
   the waiter has to fixup the pi state.
5. Complete the requeue state so the waiter can make progress. After
   this point the waiter task can return from the syscall immediately in
   case that the pi state does not have to be fixed up.
6. Wake the waiter task.

Must be called with both q->lock\_ptr and hb->lock held.

int futex\_proxy\_trylock\_atomic(u32 \_\_user \*pifutex, struct futex\_hash\_bucket \*hb1, struct futex\_hash\_bucket \*hb2, union futex\_key \*key1, union futex\_key \*key2, struct futex\_pi\_state \*\*ps, struct task\_struct \*\*exiting, int set\_waiters)
:   Attempt an atomic lock for the top waiter

**Parameters**

`u32 __user *pifutex`
:   the user address of the to futex

`struct futex_hash_bucket *hb1`
:   the from futex hash bucket, must be locked by the caller

`struct futex_hash_bucket *hb2`
:   the to futex hash bucket, must be locked by the caller

`union futex_key *key1`
:   the from futex key

`union futex_key *key2`
:   the to futex key

`struct futex_pi_state **ps`
:   address to store the pi\_state pointer

`struct task_struct **exiting`
:   Pointer to store the task pointer of the owner task
    which is in the middle of exiting

`int set_waiters`
:   force setting the FUTEX\_WAITERS bit (1) or not (0)

**Description**

Try and get the lock on behalf of the top waiter if we can do it atomically.
Wake the top waiter if we succeed. If the caller specified set\_waiters,
then direct [`futex_lock_pi_atomic()`](#c.futex_lock_pi_atomic "futex_lock_pi_atomic") to force setting the FUTEX\_WAITERS bit.
hb1 and hb2 must be held by the caller.

**exiting** is only set when the return value is -EBUSY. If so, this holds
a refcount on the exiting task on return and the caller needs to drop it
after waiting for the exit to complete.

**Return**

* 0 - failed to acquire the lock atomically;
* >0 - acquired the lock, return value is vpid of the top\_waiter
* <0 - error

int futex\_requeue(u32 \_\_user \*uaddr1, unsigned int flags1, u32 \_\_user \*uaddr2, unsigned int flags2, int nr\_wake, int nr\_requeue, u32 \*cmpval, int requeue\_pi)
:   Requeue waiters from uaddr1 to uaddr2

**Parameters**

`u32 __user *uaddr1`
:   source futex user address

`unsigned int flags1`
:   futex flags (FLAGS\_SHARED, etc.)

`u32 __user *uaddr2`
:   target futex user address

`unsigned int flags2`
:   futex flags (FLAGS\_SHARED, etc.)

`int nr_wake`
:   number of waiters to wake (must be 1 for requeue\_pi)

`int nr_requeue`
:   number of waiters to requeue (0-INT\_MAX)

`u32 *cmpval`
:   **uaddr1** expected value (or `NULL`)

`int requeue_pi`
:   if we are attempting to requeue from a non-pi futex to a
    pi futex (pi to pi requeue is not supported)

**Description**

Requeue waiters on uaddr1 to uaddr2. In the requeue\_pi case, try to acquire
uaddr2 atomically on behalf of the top waiter.

**Return**

* >=0 - on success, the number of tasks requeued or woken;
* <0 - on error

int handle\_early\_requeue\_pi\_wakeup(struct futex\_hash\_bucket \*hb, struct [futex\_q](#c.futex_q "futex_q") \*q, struct [hrtimer\_sleeper](../driver-api/basics.html#c.hrtimer_sleeper "hrtimer_sleeper") \*timeout)
:   Handle early wakeup on the initial futex

**Parameters**

`struct futex_hash_bucket *hb`
:   the hash\_bucket futex\_q was original enqueued on

`struct futex_q *q`
:   the futex\_q woken while waiting to be requeued

`struct hrtimer_sleeper *timeout`
:   the timeout associated with the wait (NULL if none)

**Description**

Determine the cause for the early wakeup.

**Return**

-EWOULDBLOCK or -ETIMEDOUT or -ERESTARTNOINTR

int futex\_wait\_requeue\_pi(u32 \_\_user \*uaddr, unsigned int flags, u32 val, ktime\_t \*abs\_time, u32 bitset, u32 \_\_user \*uaddr2)
:   Wait on uaddr and take uaddr2

**Parameters**

`u32 __user *uaddr`
:   the futex we initially wait on (non-pi)

`unsigned int flags`
:   futex flags (FLAGS\_SHARED, FLAGS\_CLOCKRT, etc.), they must be
    the same type, no requeueing from private to shared, etc.

`u32 val`
:   the expected value of uaddr

`ktime_t *abs_time`
:   absolute timeout

`u32 bitset`
:   32 bit wakeup bitset set by userspace, defaults to all

`u32 __user *uaddr2`
:   the pi futex we will take prior to returning to user-space

**Description**

The caller will wait on uaddr and will be requeued by [`futex_requeue()`](#c.futex_requeue "futex_requeue") to
uaddr2 which must be PI aware and unique from uaddr. Normal wakeup will wake
on uaddr2 and complete the acquisition of the rt\_mutex prior to returning to
userspace. This ensures the rt\_mutex maintains an owner when it has waiters;
without one, the pi logic would not know which task to boost/deboost, if
there was a need to.

We call schedule in `futex_wait_queue()` when we enqueue and return there
via the following--
1) wakeup on uaddr2 after an atomic lock acquisition by [`futex_requeue()`](#c.futex_requeue "futex_requeue")
2) wakeup on uaddr2 after a requeue
3) signal
4) timeout

If 3, cleanup and return -ERESTARTNOINTR.

If 2, we may then block on trying to take the rt\_mutex and return via:
5) successful lock
6) signal
7) timeout
8) other lock acquisition failure

If 6, return -EWOULDBLOCK (restarting the syscall would do the same).

If 4 or 7, we cleanup and return with -ETIMEDOUT.

**Return**

* 0 - On success;
* <0 - On error

void futex\_do\_wait(struct [futex\_q](#c.futex_q "futex_q") \*q, struct [hrtimer\_sleeper](../driver-api/basics.html#c.hrtimer_sleeper "hrtimer_sleeper") \*timeout)
:   wait for wakeup, timeout, or signal

**Parameters**

`struct futex_q *q`
:   the futex\_q to queue up on

`struct hrtimer_sleeper *timeout`
:   the prepared hrtimer\_sleeper, or null for no timeout

int futex\_unqueue\_multiple(struct [futex\_vector](#c.futex_vector "futex_vector") \*v, int count)
:   Remove various futexes from their hash bucket

**Parameters**

`struct futex_vector *v`
:   The list of futexes to unqueue

`int count`
:   Number of futexes in the list

**Description**

Helper to unqueue a list of futexes. This can’t fail.

**Return**

* >=0 - Index of the last futex that was awoken;
* `-1`
  :   + No futex was awoken

int futex\_wait\_multiple\_setup(struct [futex\_vector](#c.futex_vector "futex_vector") \*vs, int count, int \*woken)
:   Prepare to wait and enqueue multiple futexes

**Parameters**

`struct futex_vector *vs`
:   The futex list to wait on

`int count`
:   The size of the list

`int *woken`
:   Index of the last woken futex, if any. Used to notify the
    caller that it can return this index to userspace (return parameter)

**Description**

Prepare multiple futexes in a single step and enqueue them. This may fail if
the futex list is invalid or if any futex was already awoken. On success the
task is ready to interruptible sleep.

**Return**

* 1 - One of the futexes was woken by another thread
* 0 - Success
* <0 - -EFAULT, -EWOULDBLOCK or -EINVAL

void futex\_sleep\_multiple(struct [futex\_vector](#c.futex_vector "futex_vector") \*vs, unsigned int count, struct [hrtimer\_sleeper](../driver-api/basics.html#c.hrtimer_sleeper "hrtimer_sleeper") \*to)
:   Check sleeping conditions and sleep

**Parameters**

`struct futex_vector *vs`
:   List of futexes to wait for

`unsigned int count`
:   Length of vs

`struct hrtimer_sleeper *to`
:   Timeout

**Description**

Sleep if and only if the timeout hasn’t expired and no futex on the list has
been woken up.

int futex\_wait\_multiple(struct [futex\_vector](#c.futex_vector "futex_vector") \*vs, unsigned int count, struct [hrtimer\_sleeper](../driver-api/basics.html#c.hrtimer_sleeper "hrtimer_sleeper") \*to)
:   Prepare to wait on and enqueue several futexes

**Parameters**

`struct futex_vector *vs`
:   The list of futexes to wait on

`unsigned int count`
:   The number of objects

`struct hrtimer_sleeper *to`
:   Timeout before giving up and returning to userspace

**Description**

Entry point for the FUTEX\_WAIT\_MULTIPLE futex operation, this function
sleeps on a group of futexes and returns on the first futex that is
wake, or after the timeout has elapsed.

**Return**

* >=0 - Hint to the futex that was awoken
* <0 - On error

int futex\_wait\_setup(u32 \_\_user \*uaddr, u32 val, unsigned int flags, struct [futex\_q](#c.futex_q "futex_q") \*q, union futex\_key \*key2, struct task\_struct \*task)
:   Prepare to wait on a futex

**Parameters**

`u32 __user *uaddr`
:   the futex userspace address

`u32 val`
:   the expected value

`unsigned int flags`
:   futex flags (FLAGS\_SHARED, etc.)

`struct futex_q *q`
:   the associated futex\_q

`union futex_key *key2`
:   the second futex\_key if used for requeue PI

`struct task_struct *task`
:   Task queueing this futex

**Description**

Setup the futex\_q and locate the hash\_bucket. Get the futex value and
compare it with the expected value. Handle atomic faults internally.
Return with the hb lock held on success, and unlocked on failure.

**Return**

* 0 - uaddr contains val and hb has been locked;
* <0 - On error and the hb is unlocked. A possible reason: the uaddr can not
  :   be read, does not contain the expected value or is not properly aligned.

## Further reading

* `Documentation/locking/spinlocks.rst`: Linus Torvalds’ spinlocking
  tutorial in the kernel sources.
* Unix Systems for Modern Architectures: Symmetric Multiprocessing and
  Caching for Kernel Programmers:

  Curt Schimmel’s very good introduction to kernel level locking (not
  written for Linux, but nearly everything applies). The book is
  expensive, but really worth every penny to understand SMP locking.
  [ISBN: 0201633388]

## Thanks

Thanks to Telsa Gwynne for DocBooking, neatening and adding style.

Thanks to Martin Pool, Philipp Rumpf, Stephen Rothwell, Paul Mackerras,
Ruedi Aschwanden, Alan Cox, Manfred Spraul, Tim Waugh, Pete Zaitcev,
James Morris, Robert Love, Paul McKenney, John Ashby for proofreading,
correcting, flaming, commenting.

Thanks to the cabal for having no influence on this document.

## Glossary

preemption
:   Prior to 2.5, or when `CONFIG_PREEMPT` is unset, processes in user
    context inside the kernel would not preempt each other (ie. you had that
    CPU until you gave it up, except for interrupts). With the addition of
    `CONFIG_PREEMPT` in 2.5.4, this changed: when in user context, higher
    priority tasks can “cut in”: spinlocks were changed to disable
    preemption, even on UP.

bh
:   Bottom Half: for historical reasons, functions with ‘\_bh’ in them often
    now refer to any software interrupt, e.g. `spin_lock_bh()`
    blocks any software interrupt on the current CPU. Bottom halves are
    deprecated, and will eventually be replaced by tasklets. Only one bottom
    half will be running at any time.

Hardware Interrupt / Hardware IRQ
:   Hardware interrupt request. `in_hardirq()` returns true in a
    hardware interrupt handler.

Interrupt Context
:   Not user context: processing a hardware irq or software irq. Indicated
    by the `in_interrupt()` macro returning true.

SMP
:   Symmetric Multi-Processor: kernels compiled for multiple-CPU machines.
    (`CONFIG_SMP=y`).

Software Interrupt / softirq
:   Software interrupt handler. `in_hardirq()` returns false;
    `in_softirq()` returns true. Tasklets and softirqs both
    fall into the category of ‘software interrupts’.

    Strictly speaking a softirq is one of up to 32 enumerated software
    interrupts which can run on multiple CPUs at once. Sometimes used to
    refer to tasklets as well (ie. all software interrupts).

tasklet
:   A dynamically-registrable software interrupt, which is guaranteed to
    only run on one CPU at a time.

timer
:   A dynamically-registrable software interrupt, which is run at (or close
    to) a given time. When running, it is just like a tasklet (in fact, they
    are called from the `TIMER_SOFTIRQ`).

UP
:   Uni-Processor: Non-SMP. (`CONFIG_SMP=n`).

User Context
:   The kernel executing on behalf of a particular process (ie. a system
    call or trap) or kernel thread. You can tell which process with the
    `current` macro.) Not to be confused with userspace. Can be
    interrupted by software or hardware interrupts.

Userspace
:   A process executing its own code outside the kernel.
