# ID Allocation

> 출처(원문): https://docs.kernel.org/core-api/idr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ID Allocation

Author:
:   Matthew Wilcox

## Overview

A common problem to solve is allocating identifiers (IDs); generally
small numbers which identify a thing. Examples include file descriptors,
process IDs, packet identifiers in networking protocols, SCSI tags
and device instance numbers. The IDR and the IDA provide a reasonable
solution to the problem to avoid everybody inventing their own. The IDR
provides the ability to map an ID to a pointer, while the IDA provides
only ID allocation, and as a result is much more memory-efficient.

The IDR interface is deprecated; please use the [XArray](xarray.html)
instead.

## IDR usage

Start by initialising an IDR, either with [`DEFINE_IDR()`](#c.DEFINE_IDR "DEFINE_IDR")
for statically allocated IDRs or [`idr_init()`](#c.idr_init "idr_init") for dynamically
allocated IDRs.

You can call [`idr_alloc()`](#c.idr_alloc "idr_alloc") to allocate an unused ID. Look up
the pointer you associated with the ID by calling [`idr_find()`](#c.idr_find "idr_find")
and free the ID by calling [`idr_remove()`](#c.idr_remove "idr_remove").

If you need to change the pointer associated with an ID, you can call
[`idr_replace()`](#c.idr_replace "idr_replace"). One common reason to do this is to reserve an
ID by passing a `NULL` pointer to the allocation function; initialise the
object with the reserved ID and finally insert the initialised object
into the IDR.

Some users need to allocate IDs larger than `INT_MAX`. So far all of
these users have been content with a `UINT_MAX` limit, and they use
[`idr_alloc_u32()`](#c.idr_alloc_u32 "idr_alloc_u32"). If you need IDs that will not fit in a u32,
we will work with you to address your needs.

If you need to allocate IDs sequentially, you can use
[`idr_alloc_cyclic()`](#c.idr_alloc_cyclic "idr_alloc_cyclic"). The IDR becomes less efficient when dealing
with larger IDs, so using this function comes at a slight cost.

To perform an action on all pointers used by the IDR, you can
either use the callback-based [`idr_for_each()`](#c.idr_for_each "idr_for_each") or the
iterator-style [`idr_for_each_entry()`](#c.idr_for_each_entry "idr_for_each_entry"). You may need to use
[`idr_for_each_entry_continue()`](#c.idr_for_each_entry_continue "idr_for_each_entry_continue") to continue an iteration. You can
also use [`idr_get_next()`](#c.idr_get_next "idr_get_next") if the iterator doesn’t fit your needs.

When you have finished using an IDR, you can call `idr_destroy()`
to release the memory used by the IDR. This will not free the objects
pointed to from the IDR; if you want to do that, use one of the iterators
to do it.

You can use [`idr_is_empty()`](#c.idr_is_empty "idr_is_empty") to find out whether there are any
IDs currently allocated.

If you need to take a lock while allocating a new ID from the IDR,
you may need to pass a restrictive set of GFP flags, which can lead
to the IDR being unable to allocate memory. To work around this,
you can call `idr_preload()` before taking the lock, and then
[`idr_preload_end()`](#c.idr_preload_end "idr_preload_end") after the allocation.

idr synchronization (stolen from radix-tree.h)

[`idr_find()`](#c.idr_find "idr_find") is able to be called locklessly, using RCU. The caller must
ensure calls to this function are made within [`rcu_read_lock()`](kernel-api.html#c.rcu_read_lock "rcu_read_lock") regions.
Other readers (lock-free or otherwise) and modifications may be running
concurrently.

It is still required that the caller manage the synchronization and
lifetimes of the items. So if RCU lock-free lookups are used, typically
this would mean that the items have their own locks, or are amenable to
lock-free access; and that the items are freed by RCU (or only freed after
having been deleted from the idr tree *and* a [`synchronize_rcu()`](kernel-api.html#c.synchronize_rcu "synchronize_rcu") grace
period).

## IDA usage

The IDA is an ID allocator which does not provide the ability to
associate an ID with a pointer. As such, it only needs to store one
bit per ID, and so is more space efficient than an IDR. To use an IDA,
define it using `DEFINE_IDA()` (or embed a `struct ida` in a data structure,
then initialise it using `ida_init()`). To allocate a new ID, call
[`ida_alloc()`](#c.ida_alloc "ida_alloc"), [`ida_alloc_min()`](#c.ida_alloc_min "ida_alloc_min"), [`ida_alloc_max()`](#c.ida_alloc_max "ida_alloc_max") or [`ida_alloc_range()`](#c.ida_alloc_range "ida_alloc_range").
To free an ID, call [`ida_free()`](#c.ida_free "ida_free").

[`ida_destroy()`](#c.ida_destroy "ida_destroy") can be used to dispose of an IDA without needing to
free the individual IDs in it. You can use `ida_is_empty()` to find
out whether the IDA has any IDs currently allocated.

The IDA handles its own locking. It is safe to call any of the IDA
functions without synchronisation in your code.

IDs are currently limited to the range [0-INT\_MAX]. If this is an awkward
limitation, it should be quite straightforward to raise the maximum.

## Functions and structures

IDR\_INIT

`IDR_INIT (name)`

> Initialise an IDR.

**Parameters**

`name`
:   Name of IDR.

**Description**

A freshly-initialised IDR contains no IDs.

DEFINE\_IDR

`DEFINE_IDR (name)`

> Define a statically-allocated IDR.

**Parameters**

`name`
:   Name of IDR.

**Description**

An IDR defined using this macro is ready for use with no additional
initialisation required. It contains no IDs.

unsigned int idr\_get\_cursor(const struct [idr](#c.idr_get_cursor "idr") \*idr)
:   Return the current position of the cyclic allocator

**Parameters**

`const struct idr *idr`
:   idr handle

**Description**

The value returned is the value that will be next returned from
[`idr_alloc_cyclic()`](#c.idr_alloc_cyclic "idr_alloc_cyclic") if it is free (otherwise the search will start from
this position).

void idr\_set\_cursor(struct [idr](#c.idr_set_cursor "idr") \*idr, unsigned int val)
:   Set the current position of the cyclic allocator

**Parameters**

`struct idr *idr`
:   idr handle

`unsigned int val`
:   new position

**Description**

The next call to [`idr_alloc_cyclic()`](#c.idr_alloc_cyclic "idr_alloc_cyclic") will return **val** if it is free
(otherwise the search will start from this position).

void idr\_init\_base(struct [idr](#c.idr_init_base "idr") \*idr, int base)
:   Initialise an IDR.

**Parameters**

`struct idr *idr`
:   IDR handle.

`int base`
:   The base value for the IDR.

**Description**

This variation of [`idr_init()`](#c.idr_init "idr_init") creates an IDR which will allocate IDs
starting at `base`.

void idr\_init(struct [idr](#c.idr_init "idr") \*idr)
:   Initialise an IDR.

**Parameters**

`struct idr *idr`
:   IDR handle.

**Description**

Initialise a dynamically allocated IDR. To initialise a
statically allocated IDR, use [`DEFINE_IDR()`](#c.DEFINE_IDR "DEFINE_IDR").

bool idr\_is\_empty(const struct [idr](#c.idr_is_empty "idr") \*idr)
:   Are there any IDs allocated?

**Parameters**

`const struct idr *idr`
:   IDR handle.

**Return**

`true` if any IDs have been allocated from this IDR.

void idr\_preload\_end(void)
:   end preload section started with `idr_preload()`

**Parameters**

`void`
:   no arguments

**Description**

Each `idr_preload()` should be matched with an invocation of this
function. See `idr_preload()` for details.

idr\_for\_each\_entry

`idr_for_each_entry (idr, entry, id)`

> Iterate over an IDR’s elements of a given type.

**Parameters**

`idr`
:   IDR handle.

`entry`
:   The type \* to use as cursor

`id`
:   Entry ID.

**Description**

**entry** and **id** do not need to be initialized before the loop, and
after normal termination **entry** is left with the value NULL. This
is convenient for a “not found” value.

idr\_for\_each\_entry\_ul

`idr_for_each_entry_ul (idr, entry, tmp, id)`

> Iterate over an IDR’s elements of a given type.

**Parameters**

`idr`
:   IDR handle.

`entry`
:   The type \* to use as cursor.

`tmp`
:   A temporary placeholder for ID.

`id`
:   Entry ID.

**Description**

**entry** and **id** do not need to be initialized before the loop, and
after normal termination **entry** is left with the value NULL. This
is convenient for a “not found” value.

idr\_for\_each\_entry\_continue

`idr_for_each_entry_continue (idr, entry, id)`

> Continue iteration over an IDR’s elements of a given type

**Parameters**

`idr`
:   IDR handle.

`entry`
:   The type \* to use as a cursor.

`id`
:   Entry ID.

**Description**

Continue to iterate over entries, continuing after the current position.

idr\_for\_each\_entry\_continue\_ul

`idr_for_each_entry_continue_ul (idr, entry, tmp, id)`

> Continue iteration over an IDR’s elements of a given type

**Parameters**

`idr`
:   IDR handle.

`entry`
:   The type \* to use as a cursor.

`tmp`
:   A temporary placeholder for ID.

`id`
:   Entry ID.

**Description**

Continue to iterate over entries, continuing after the current position.
After normal termination **entry** is left with the value NULL. This
is convenient for a “not found” value.

int ida\_alloc(struct [ida](#c.ida_alloc "ida") \*ida, gfp\_t gfp)
:   Allocate an unused ID.

**Parameters**

`struct ida *ida`
:   IDA handle.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Allocate an ID between 0 and `INT_MAX`, inclusive.

**Context**

Any context. It is safe to call this function without
locking in your code.

**Return**

The allocated ID, or `-ENOMEM` if memory could not be allocated,
or `-ENOSPC` if there are no free IDs.

int ida\_alloc\_min(struct [ida](#c.ida_alloc_min "ida") \*ida, unsigned int min, gfp\_t gfp)
:   Allocate an unused ID.

**Parameters**

`struct ida *ida`
:   IDA handle.

`unsigned int min`
:   Lowest ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Allocate an ID between **min** and `INT_MAX`, inclusive.

**Context**

Any context. It is safe to call this function without
locking in your code.

**Return**

The allocated ID, or `-ENOMEM` if memory could not be allocated,
or `-ENOSPC` if there are no free IDs.

int ida\_alloc\_max(struct [ida](#c.ida_alloc_max "ida") \*ida, unsigned int max, gfp\_t gfp)
:   Allocate an unused ID.

**Parameters**

`struct ida *ida`
:   IDA handle.

`unsigned int max`
:   Highest ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Allocate an ID between 0 and **max**, inclusive.

**Context**

Any context. It is safe to call this function without
locking in your code.

**Return**

The allocated ID, or `-ENOMEM` if memory could not be allocated,
or `-ENOSPC` if there are no free IDs.

int idr\_alloc\_u32(struct [idr](#c.idr_alloc_u32 "idr") \*idr, void \*ptr, u32 \*nextid, unsigned long max, gfp\_t gfp)
:   Allocate an ID.

**Parameters**

`struct idr *idr`
:   IDR handle.

`void *ptr`
:   Pointer to be associated with the new ID.

`u32 *nextid`
:   Pointer to an ID.

`unsigned long max`
:   The maximum ID to allocate (inclusive).

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Allocates an unused ID in the range specified by **nextid** and **max**.
Note that **max** is inclusive whereas the **end** parameter to [`idr_alloc()`](#c.idr_alloc "idr_alloc")
is exclusive. The new ID is assigned to **nextid** before the pointer
is inserted into the IDR, so if **nextid** points into the object pointed
to by **ptr**, a concurrent lookup will not find an uninitialised ID.

The caller should provide their own locking to ensure that two
concurrent modifications to the IDR are not possible. Read-only
accesses to the IDR may be done under the RCU read lock or may
exclude simultaneous writers.

**Return**

0 if an ID was allocated, -ENOMEM if memory allocation failed,
or -ENOSPC if no free IDs could be found. If an error occurred,
**nextid** is unchanged.

int idr\_alloc(struct [idr](#c.idr_alloc "idr") \*idr, void \*ptr, int start, int end, gfp\_t gfp)
:   Allocate an ID.

**Parameters**

`struct idr *idr`
:   IDR handle.

`void *ptr`
:   Pointer to be associated with the new ID.

`int start`
:   The minimum ID (inclusive).

`int end`
:   The maximum ID (exclusive).

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Allocates an unused ID in the range specified by **start** and **end**. If
**end** is <= 0, it is treated as one larger than `INT_MAX`. This allows
callers to use **start** + N as **end** as long as N is within integer range.

The caller should provide their own locking to ensure that two
concurrent modifications to the IDR are not possible. Read-only
accesses to the IDR may be done under the RCU read lock or may
exclude simultaneous writers.

**Return**

The newly allocated ID, -ENOMEM if memory allocation failed,
or -ENOSPC if no free IDs could be found.

int idr\_alloc\_cyclic(struct [idr](#c.idr_alloc_cyclic "idr") \*idr, void \*ptr, int start, int end, gfp\_t gfp)
:   Allocate an ID cyclically.

**Parameters**

`struct idr *idr`
:   IDR handle.

`void *ptr`
:   Pointer to be associated with the new ID.

`int start`
:   The minimum ID (inclusive).

`int end`
:   The maximum ID (exclusive).

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Allocates an unused ID in the range specified by **start** and **end**. If
**end** is <= 0, it is treated as one larger than `INT_MAX`. This allows
callers to use **start** + N as **end** as long as N is within integer range.
The search for an unused ID will start at the last ID allocated and will
wrap around to **start** if no free IDs are found before reaching **end**.

The caller should provide their own locking to ensure that two
concurrent modifications to the IDR are not possible. Read-only
accesses to the IDR may be done under the RCU read lock or may
exclude simultaneous writers.

**Return**

The newly allocated ID, -ENOMEM if memory allocation failed,
or -ENOSPC if no free IDs could be found.

void \*idr\_remove(struct [idr](#c.idr_remove "idr") \*idr, unsigned long id)
:   Remove an ID from the IDR.

**Parameters**

`struct idr *idr`
:   IDR handle.

`unsigned long id`
:   Pointer ID.

**Description**

Removes this ID from the IDR. If the ID was not previously in the IDR,
this function returns `NULL`.

Since this function modifies the IDR, the caller should provide their
own locking to ensure that concurrent modification of the same IDR is
not possible.

**Return**

The pointer formerly associated with this ID.

void \*idr\_find(const struct [idr](#c.idr_find "idr") \*idr, unsigned long id)
:   Return pointer for given ID.

**Parameters**

`const struct idr *idr`
:   IDR handle.

`unsigned long id`
:   Pointer ID.

**Description**

Looks up the pointer associated with this ID. A `NULL` pointer may
indicate that **id** is not allocated or that the `NULL` pointer was
associated with this ID.

This function can be called under [`rcu_read_lock()`](kernel-api.html#c.rcu_read_lock "rcu_read_lock"), given that the leaf
pointers lifetimes are correctly managed.

**Return**

The pointer associated with this ID.

int idr\_for\_each(const struct [idr](#c.idr_for_each "idr") \*idr, int (\*fn)(int id, void \*p, void \*data), void \*data)
:   Iterate through all stored pointers.

**Parameters**

`const struct idr *idr`
:   IDR handle.

`int (*fn)(int id, void *p, void *data)`
:   Function to be called for each pointer.

`void *data`
:   Data passed to callback function.

**Description**

The callback function will be called for each entry in **idr**, passing
the ID, the entry and **data**.

If **fn** returns anything other than `0`, the iteration stops and that
value is returned from this function.

[`idr_for_each()`](#c.idr_for_each "idr_for_each") can be called concurrently with [`idr_alloc()`](#c.idr_alloc "idr_alloc") and
[`idr_remove()`](#c.idr_remove "idr_remove") if protected by RCU. Newly added entries may not be
seen and deleted entries may be seen, but adding and removing entries
will not cause other entries to be skipped, nor spurious ones to be seen.

void \*idr\_get\_next\_ul(struct [idr](#c.idr_get_next_ul "idr") \*idr, unsigned long \*nextid)
:   Find next populated entry.

**Parameters**

`struct idr *idr`
:   IDR handle.

`unsigned long *nextid`
:   Pointer to an ID.

**Description**

Returns the next populated entry in the tree with an ID greater than
or equal to the value pointed to by **nextid**. On exit, **nextid** is updated
to the ID of the found value. To use in a loop, the value pointed to by
nextid must be incremented by the user.

void \*idr\_get\_next(struct [idr](#c.idr_get_next "idr") \*idr, int \*nextid)
:   Find next populated entry.

**Parameters**

`struct idr *idr`
:   IDR handle.

`int *nextid`
:   Pointer to an ID.

**Description**

Returns the next populated entry in the tree with an ID greater than
or equal to the value pointed to by **nextid**. On exit, **nextid** is updated
to the ID of the found value. To use in a loop, the value pointed to by
nextid must be incremented by the user.

void \*idr\_replace(struct [idr](#c.idr_replace "idr") \*idr, void \*ptr, unsigned long id)
:   replace pointer for given ID.

**Parameters**

`struct idr *idr`
:   IDR handle.

`void *ptr`
:   New pointer to associate with the ID.

`unsigned long id`
:   ID to change.

**Description**

Replace the pointer registered with an ID and return the old value.
This function can be called under the RCU read lock concurrently with
[`idr_alloc()`](#c.idr_alloc "idr_alloc") and [`idr_remove()`](#c.idr_remove "idr_remove") (as long as the ID being removed is not
the one being replaced!).

**Return**

the old value on success. `-ENOENT` indicates that **id** was not
found. `-EINVAL` indicates that **ptr** was not valid.

int ida\_alloc\_range(struct [ida](#c.ida_alloc_range "ida") \*ida, unsigned int min, unsigned int max, gfp\_t gfp)
:   Allocate an unused ID.

**Parameters**

`struct ida *ida`
:   IDA handle.

`unsigned int min`
:   Lowest ID to allocate.

`unsigned int max`
:   Highest ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Allocate an ID between **min** and **max**, inclusive. The allocated ID will
not exceed `INT_MAX`, even if **max** is larger.

**Context**

Any context. It is safe to call this function without
locking in your code.

**Return**

The allocated ID, or `-ENOMEM` if memory could not be allocated,
or `-ENOSPC` if there are no free IDs.

int ida\_find\_first\_range(struct [ida](#c.ida_find_first_range "ida") \*ida, unsigned int min, unsigned int max)
:   Get the lowest used ID.

**Parameters**

`struct ida *ida`
:   IDA handle.

`unsigned int min`
:   Lowest ID to get.

`unsigned int max`
:   Highest ID to get.

**Description**

Get the lowest used ID between **min** and **max**, inclusive. The returned
ID will not exceed `INT_MAX`, even if **max** is larger.

**Context**

Any context. Takes and releases the xa\_lock.

**Return**

The lowest used ID, or errno if no used ID is found.

void ida\_free(struct [ida](#c.ida_free "ida") \*ida, unsigned int id)
:   Release an allocated ID.

**Parameters**

`struct ida *ida`
:   IDA handle.

`unsigned int id`
:   Previously allocated ID.

**Context**

Any context. It is safe to call this function without
locking in your code.

void ida\_destroy(struct [ida](#c.ida_destroy "ida") \*ida)
:   Free all IDs.

**Parameters**

`struct ida *ida`
:   IDA handle.

**Description**

Calling this function frees all IDs and releases all resources used
by an IDA. When this call returns, the IDA is empty and can be reused
or freed. If the IDA is already empty, there is no need to call this
function.

**Context**

Any context. It is safe to call this function without
locking in your code.
