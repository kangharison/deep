# Maple Tree

> 출처(원문): https://docs.kernel.org/core-api/maple_tree.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Maple Tree

Author:
:   Liam R. Howlett

## Overview

The Maple Tree is a B-Tree data type which is optimized for storing
non-overlapping ranges, including ranges of size 1. The tree was designed to
be simple to use and does not require a user written search method. It
supports iterating over a range of entries and going to the previous or next
entry in a cache-efficient manner. The tree can also be put into an RCU-safe
mode of operation which allows reading and writing concurrently. Writers must
synchronize on a lock, which can be the default spinlock, or the user can set
the lock to an external lock of a different type.

The Maple Tree maintains a small memory footprint and was designed to use
modern processor cache efficiently. The majority of the users will be able to
use the normal API. An [Advanced API](#maple-tree-advanced-api) exists for more complex
scenarios. The most important usage of the Maple Tree is the tracking of the
virtual memory areas.

The Maple Tree can store values between `0` and `ULONG_MAX`. The Maple
Tree reserves values with the bottom two bits set to ‘10’ which are below 4096
(ie 2, 6, 10 .. 4094) for internal use. If the entries may use reserved
entries then the users can convert the entries using [`xa_mk_value()`](xarray.html#c.xa_mk_value "xa_mk_value") and convert
them back by calling [`xa_to_value()`](xarray.html#c.xa_to_value "xa_to_value"). If the user needs to use a reserved
value, then the user can convert the value when using the
[Advanced API](#maple-tree-advanced-api), but are blocked by the normal API.

The Maple Tree can also be configured to support searching for a gap of a given
size (or larger).

Pre-allocating of nodes is also supported using the
[Advanced API](#maple-tree-advanced-api). This is useful for users who must guarantee a
successful store operation within a given
code segment when allocating cannot be done. Allocations of nodes are
relatively small at around 256 bytes.

## Normal API

Start by initialising a maple tree, either with `DEFINE_MTREE()` for statically
allocated maple trees or [`mt_init()`](#c.mt_init "mt_init") for dynamically allocated ones. A
freshly-initialised maple tree contains a `NULL` pointer for the range `0`
- `ULONG_MAX`. There are currently two types of maple trees supported: the
allocation tree and the regular tree. The regular tree has a higher branching
factor for internal nodes. The allocation tree has a lower branching factor
but allows the user to search for a gap of a given size or larger from either
`0` upwards or `ULONG_MAX` down. An allocation tree can be used by
passing in the `MT_FLAGS_ALLOC_RANGE` flag when initialising the tree.

You can then set entries using [`mtree_store()`](#c.mtree_store "mtree_store") or [`mtree_store_range()`](#c.mtree_store_range "mtree_store_range").
[`mtree_store()`](#c.mtree_store "mtree_store") will overwrite any entry with the new entry and return 0 on
success or an error code otherwise. [`mtree_store_range()`](#c.mtree_store_range "mtree_store_range") works in the same way
but takes a range. [`mtree_load()`](#c.mtree_load "mtree_load") is used to retrieve the entry stored at a
given index. You can use [`mtree_erase()`](#c.mtree_erase "mtree_erase") to erase an entire range by only
knowing one value within that range, or [`mtree_store()`](#c.mtree_store "mtree_store") call with an entry of
NULL may be used to partially erase a range or many ranges at once.

If you want to only store a new entry to a range (or index) if that range is
currently `NULL`, you can use [`mtree_insert_range()`](#c.mtree_insert_range "mtree_insert_range") or [`mtree_insert()`](#c.mtree_insert "mtree_insert") which
return -EEXIST if the range is not empty.

You can search for an entry from an index upwards by using [`mt_find()`](#c.mt_find "mt_find").

You can walk each entry within a range by calling [`mt_for_each()`](#c.mt_for_each "mt_for_each"). You must
provide a temporary variable to store a cursor. If you want to walk each
element of the tree then `0` and `ULONG_MAX` may be used as the range. If
the caller is going to hold the lock for the duration of the walk then it is
worth looking at the [`mas_for_each()`](#c.mas_for_each "mas_for_each") API in the [Advanced API](#maple-tree-advanced-api)
section.

Sometimes it is necessary to ensure the next call to store to a maple tree does
not allocate memory, please see [Advanced API](#maple-tree-advanced-api) for this use case.

You can use [`mtree_dup()`](#c.mtree_dup "mtree_dup") to duplicate an entire maple tree. It is a more
efficient way than inserting all elements one by one into a new tree.

Finally, you can remove all entries from a maple tree by calling
[`mtree_destroy()`](#c.mtree_destroy "mtree_destroy"). If the maple tree entries are pointers, you may wish to free
the entries first.

### Allocating Nodes

The allocations are handled by the internal tree code. See
[Advanced Allocating Nodes](#maple-tree-advanced-alloc) for other options.

### Locking

You do not have to worry about locking. See [Advanced Locking](#maple-tree-advanced-locks)
for other options.

The Maple Tree uses RCU and an internal spinlock to synchronise access:

Takes RCU read lock:
:   * [`mtree_load()`](#c.mtree_load "mtree_load")
    * [`mt_find()`](#c.mt_find "mt_find")
    * [`mt_for_each()`](#c.mt_for_each "mt_for_each")
    * [`mt_next()`](#c.mt_next "mt_next")
    * [`mt_prev()`](#c.mt_prev "mt_prev")

Takes ma\_lock internally:
:   * [`mtree_store()`](#c.mtree_store "mtree_store")
    * [`mtree_store_range()`](#c.mtree_store_range "mtree_store_range")
    * [`mtree_insert()`](#c.mtree_insert "mtree_insert")
    * [`mtree_insert_range()`](#c.mtree_insert_range "mtree_insert_range")
    * [`mtree_erase()`](#c.mtree_erase "mtree_erase")
    * [`mtree_dup()`](#c.mtree_dup "mtree_dup")
    * [`mtree_destroy()`](#c.mtree_destroy "mtree_destroy")
    * [`mt_set_in_rcu()`](#c.mt_set_in_rcu "mt_set_in_rcu")
    * [`mt_clear_in_rcu()`](#c.mt_clear_in_rcu "mt_clear_in_rcu")

If you want to take advantage of the internal lock to protect the data
structures that you are storing in the Maple Tree, you can call `mtree_lock()`
before calling [`mtree_load()`](#c.mtree_load "mtree_load"), then take a reference count on the object you
have found before calling `mtree_unlock()`. This will prevent stores from
removing the object from the tree between looking up the object and
incrementing the refcount. You can also use RCU to avoid dereferencing
freed memory, but an explanation of that is beyond the scope of this
document.

## Advanced API

The advanced API offers more flexibility and better performance at the
cost of an interface which can be harder to use and has fewer safeguards.
You must take care of your own locking while using the advanced API.
You can use the ma\_lock, RCU or an external lock for protection.
You can mix advanced and normal operations on the same array, as long
as the locking is compatible. The [Normal API](#maple-tree-normal-api) is implemented
in terms of the advanced API.

The advanced API is based around the ma\_state, this is where the ‘mas’
prefix originates. The ma\_state `struct keeps` track of tree operations to make
life easier for both internal and external tree users.

Initialising the maple tree is the same as in the [Normal API](#maple-tree-normal-api).
Please see above.

The maple state keeps track of the range start and end in mas->index and
mas->last, respectively.

[`mas_walk()`](#c.mas_walk "mas_walk") will walk the tree to the location of mas->index and set the
mas->index and mas->last according to the range for the entry.

You can set entries using [`mas_store()`](#c.mas_store "mas_store"). [`mas_store()`](#c.mas_store "mas_store") will overwrite any entry
with the new entry and return the first existing entry that is overwritten.
The range is passed in as members of the maple state: index and last.

You can use [`mas_erase()`](#c.mas_erase "mas_erase") to erase an entire range by setting index and
last of the maple state to the desired range to erase. This will erase
the first range that is found in that range, set the maple state index
and last as the range that was erased and return the entry that existed
at that location.

You can walk each entry within a range by using [`mas_for_each()`](#c.mas_for_each "mas_for_each"). If you want
to walk each element of the tree then `0` and `ULONG_MAX` may be used as
the range. If the lock needs to be periodically dropped, see the locking
section [`mas_pause()`](#c.mas_pause "mas_pause").

Using a maple state allows [`mas_next()`](#c.mas_next "mas_next") and [`mas_prev()`](#c.mas_prev "mas_prev") to function as if the
tree was a linked list. With such a high branching factor the amortized
performance penalty is outweighed by cache optimization. [`mas_next()`](#c.mas_next "mas_next") will
return the next entry which occurs after the entry at index. [`mas_prev()`](#c.mas_prev "mas_prev")
will return the previous entry which occurs before the entry at index.

[`mas_find()`](#c.mas_find "mas_find") will find the first entry which exists at or above index on
the first call, and the next entry from every subsequent calls.

[`mas_find_rev()`](#c.mas_find_rev "mas_find_rev") will find the first entry which exists at or below the last on
the first call, and the previous entry from every subsequent calls.

If the user needs to yield the lock during an operation, then the maple state
must be paused using [`mas_pause()`](#c.mas_pause "mas_pause").

There are a few extra interfaces provided when using an allocation tree.
If you wish to search for a gap within a range, then `mas_empty_area()`
or `mas_empty_area_rev()` can be used. `mas_empty_area()` searches for a gap
starting at the lowest index given up to the maximum of the range.
`mas_empty_area_rev()` searches for a gap starting at the highest index given
and continues downward to the lower bound of the range.

### Advanced Allocating Nodes

Allocations are usually handled internally to the tree, however if allocations
need to occur before a write occurs then calling `mas_expected_entries()` will
allocate the worst-case number of needed nodes to insert the provided number of
ranges. This also causes the tree to enter mass insertion mode. Once
insertions are complete calling `mas_destroy()` on the maple state will free the
unused allocations.

### Advanced Locking

The maple tree uses a spinlock by default, but external locks can be used for
tree updates as well. To use an external lock, the tree must be initialized
with the `MT_FLAGS_LOCK_EXTERN flag`, this is usually done with the
[`MTREE_INIT_EXT()`](#c.MTREE_INIT_EXT "MTREE_INIT_EXT") #define, which takes an external lock as an argument.

## Functions and structures

**Maple tree flags**

* MT\_FLAGS\_ALLOC\_RANGE - Track gaps in this tree
* MT\_FLAGS\_USE\_RCU - Operate in RCU mode
* MT\_FLAGS\_HEIGHT\_OFFSET - The position of the tree height in the flags
* MT\_FLAGS\_HEIGHT\_MASK - The mask for the maple tree height value
* MT\_FLAGS\_LOCK\_MASK - How the mt\_lock is used
* MT\_FLAGS\_LOCK\_IRQ - Acquired irq-safe
* MT\_FLAGS\_LOCK\_BH - Acquired bh-safe
* MT\_FLAGS\_LOCK\_EXTERN - mt\_lock is not used

MAPLE\_HEIGHT\_MAX The largest height that can be stored

MTREE\_INIT

`MTREE_INIT (name, __flags)`

> Initialize a maple tree

**Parameters**

`name`
:   The maple tree name

`__flags`
:   The maple tree flags

MTREE\_INIT\_EXT

`MTREE_INIT_EXT (name, __flags, __lock)`

> Initialize a maple tree with an external lock.

**Parameters**

`name`
:   The tree name

`__flags`
:   The maple tree flags

`__lock`
:   The external lock

bool mtree\_empty(const struct maple\_tree \*mt)
:   Determine if a tree has any present entries.

**Parameters**

`const struct maple_tree *mt`
:   Maple Tree.

**Context**

Any context.

**Return**

`true` if the tree contains only NULL pointers.

void mas\_reset(struct ma\_state \*mas)
:   Reset a Maple Tree operation state.

**Parameters**

`struct ma_state *mas`
:   Maple Tree operation state.

**Description**

Resets the error or walk state of the **mas** so future walks of the
array will start from the root. Use this if you have dropped the
lock and want to reuse the ma\_state.

**Context**

Any context.

mas\_for\_each

`mas_for_each (__mas, __entry, __max)`

> Iterate over a range of the maple tree.

**Parameters**

`__mas`
:   Maple Tree operation state (maple\_state)

`__entry`
:   Entry retrieved from the tree

`__max`
:   maximum index to retrieve from the tree

**Description**

When returned, mas->index and mas->last will hold the entire range for the
entry.

**Note**

may return the zero entry.

mas\_for\_each\_rev

`mas_for_each_rev (__mas, __entry, __min)`

> Iterate over a range of the maple tree in reverse order.

**Parameters**

`__mas`
:   Maple Tree operation state (maple\_state)

`__entry`
:   Entry retrieved from the tree

`__min`
:   minimum index to retrieve from the tree

**Description**

When returned, mas->index and mas->last will hold the entire range for the
entry.

**Note**

may return the zero entry.

void \_\_mas\_set\_range(struct ma\_state \*mas, unsigned long start, unsigned long last)
:   Set up Maple Tree operation state to a sub-range of the current location.

**Parameters**

`struct ma_state *mas`
:   Maple Tree operation state.

`unsigned long start`
:   New start of range in the Maple Tree.

`unsigned long last`
:   New end of range in the Maple Tree.

**Description**

set the internal maple state values to a sub-range.
Please use [`mas_set_range()`](#c.mas_set_range "mas_set_range") if you do not know where you are in the tree.

void mas\_set\_range(struct ma\_state \*mas, unsigned long start, unsigned long last)
:   Set up Maple Tree operation state for a different index.

**Parameters**

`struct ma_state *mas`
:   Maple Tree operation state.

`unsigned long start`
:   New start of range in the Maple Tree.

`unsigned long last`
:   New end of range in the Maple Tree.

**Description**

Move the operation state to refer to a different range. This will
have the effect of starting a walk from the top; see [`mas_next()`](#c.mas_next "mas_next")
to move to an adjacent index.

void mas\_set(struct ma\_state \*mas, unsigned long index)
:   Set up Maple Tree operation state for a different index.

**Parameters**

`struct ma_state *mas`
:   Maple Tree operation state.

`unsigned long index`
:   New index into the Maple Tree.

**Description**

Move the operation state to refer to a different index. This will
have the effect of starting a walk from the top; see [`mas_next()`](#c.mas_next "mas_next")
to move to an adjacent index.

void mt\_init\_flags(struct maple\_tree \*mt, unsigned int flags)
:   Initialise an empty maple tree with flags.

**Parameters**

`struct maple_tree *mt`
:   Maple Tree

`unsigned int flags`
:   maple tree flags.

**Description**

If you need to initialise a Maple Tree with special flags (eg, an
allocation tree), use this function.

**Context**

Any context.

void mt\_init(struct maple\_tree \*mt)
:   Initialise an empty maple tree.

**Parameters**

`struct maple_tree *mt`
:   Maple Tree

**Description**

An empty Maple Tree.

**Context**

Any context.

void mt\_clear\_in\_rcu(struct maple\_tree \*mt)
:   Switch the tree to non-RCU mode.

**Parameters**

`struct maple_tree *mt`
:   The Maple Tree

void mt\_set\_in\_rcu(struct maple\_tree \*mt)
:   Switch the tree to RCU safe mode.

**Parameters**

`struct maple_tree *mt`
:   The Maple Tree

mt\_for\_each

`mt_for_each (__tree, __entry, __index, __max)`

> Iterate over each entry starting at index until max.

**Parameters**

`__tree`
:   The Maple Tree

`__entry`
:   The current entry

`__index`
:   The index to start the search from. Subsequently used as iterator.

`__max`
:   The maximum limit for **index**

**Description**

This iterator skips all entries, which resolve to a NULL pointer,
e.g. entries which has been reserved with XA\_ZERO\_ENTRY.

void mas\_prealloc\_calc(struct ma\_wr\_state \*wr\_mas, void \*entry)
:   Calculate number of nodes needed for a given store oepration

**Parameters**

`struct ma_wr_state *wr_mas`
:   The maple write state

`void *entry`
:   The entry to store into the tree

**Return**

Number of nodes required for preallocation.

void mas\_wr\_preallocate(struct ma\_wr\_state \*wr\_mas, void \*entry)
:   Preallocate enough nodes for a store operation

**Parameters**

`struct ma_wr_state *wr_mas`
:   The maple write state

`void *entry`
:   The entry that will be stored

void \*mas\_insert(struct ma\_state \*mas, void \*entry)
:   Internal call to insert a value

**Parameters**

`struct ma_state *mas`
:   The maple state

`void *entry`
:   The entry to store

**Return**

`NULL` or the contents that already exists at the requested index
otherwise. The maple state needs to be checked for error conditions.

int mas\_alloc\_cyclic(struct ma\_state \*mas, unsigned long \*startp, void \*entry, unsigned long range\_lo, unsigned long range\_hi, unsigned long \*next, gfp\_t gfp)
:   Internal call to find somewhere to store an entry

**Parameters**

`struct ma_state *mas`
:   The maple state.

`unsigned long *startp`
:   Pointer to ID.

`void *entry`
:   The entry to store.

`unsigned long range_lo`
:   Lower bound of range to search.

`unsigned long range_hi`
:   Upper bound of range to search.

`unsigned long *next`
:   Pointer to next ID to allocate.

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations.

**Return**

0 if the allocation succeeded without wrapping, 1 if the
allocation succeeded after wrapping, or -EBUSY if there are no
free entries.

void \*mas\_walk(struct ma\_state \*mas)
:   Search for **mas->index** in the tree.

**Parameters**

`struct ma_state *mas`
:   The maple state.

**Description**

mas->index and mas->last will be set to the range if there is a value. If
mas->status is ma\_none, reset to ma\_start

**Return**

the entry at the location or `NULL`.

void \_\_rcu \*\*mte\_dead\_walk(struct maple\_enode \*\*enode, unsigned char offset)
:   Walk down a dead tree to just before the leaves

**Parameters**

`struct maple_enode **enode`
:   The maple encoded node

`unsigned char offset`
:   The starting offset

**Note**

This can only be used from the RCU callback context.

void mt\_free\_walk(struct rcu\_head \*head)
:   Walk & free a tree in the RCU callback context

**Parameters**

`struct rcu_head *head`
:   The RCU head that’s within the node.

**Note**

This can only be used from the RCU callback context.

void \*mas\_store(struct ma\_state \*mas, void \*entry)
:   Store an **entry**.

**Parameters**

`struct ma_state *mas`
:   The maple state.

`void *entry`
:   The entry to store.

**Description**

The **mas->index** and **mas->last** is used to set the range for the **entry**.

**Return**

the first entry between mas->index and mas->last or `NULL`.

int mas\_store\_gfp(struct ma\_state \*mas, void \*entry, gfp\_t gfp)
:   Store a value into the tree.

**Parameters**

`struct ma_state *mas`
:   The maple state

`void *entry`
:   The entry to store

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations if necessary.

**Return**

0 on success, -EINVAL on invalid request, -ENOMEM if memory could not
be allocated.

void mas\_store\_prealloc(struct ma\_state \*mas, void \*entry)
:   Store a value into the tree using memory preallocated in the maple state.

**Parameters**

`struct ma_state *mas`
:   The maple state

`void *entry`
:   The entry to store.

int mas\_preallocate(struct ma\_state \*mas, void \*entry, gfp\_t gfp)
:   Preallocate enough nodes for a store operation

**Parameters**

`struct ma_state *mas`
:   The maple state

`void *entry`
:   The entry that will be stored

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations.

**Return**

0 on success, -ENOMEM if memory could not be allocated.

void \*mas\_next(struct ma\_state \*mas, unsigned long max)
:   Get the next entry.

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long max`
:   The maximum index to check.

**Description**

Returns the next entry after **mas->index**.
Must hold rcu\_read\_lock or the write lock.
Can return the zero entry.

**Return**

The next entry or `NULL`

void \*mas\_next\_range(struct ma\_state \*mas, unsigned long max)
:   Advance the maple state to the next range

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long max`
:   The maximum index to check.

**Description**

Sets **mas->index** and **mas->last** to the range.
Must hold rcu\_read\_lock or the write lock.
Can return the zero entry.

**Return**

The next entry or `NULL`

void \*mt\_next(struct maple\_tree \*mt, unsigned long index, unsigned long max)
:   get the next value in the maple tree

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long index`
:   The start index

`unsigned long max`
:   The maximum index to check

**Description**

Takes RCU read lock internally to protect the search, which does not
protect the returned pointer after dropping RCU read lock.
See also: [Maple Tree](#)

**Return**

The entry higher than **index** or `NULL` if nothing is found.

void \*mas\_prev(struct ma\_state \*mas, unsigned long min)
:   Get the previous entry

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long min`
:   The minimum value to check.

**Description**

Must hold rcu\_read\_lock or the write lock.
Will reset mas to ma\_start if the status is ma\_none. Will stop on not
searchable nodes.

**Return**

the previous value or `NULL`.

void \*mas\_prev\_range(struct ma\_state \*mas, unsigned long min)
:   Advance to the previous range

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long min`
:   The minimum value to check.

**Description**

Sets **mas->index** and **mas->last** to the range.
Must hold rcu\_read\_lock or the write lock.
Will reset mas to ma\_start if the node is ma\_none. Will stop on not
searchable nodes.

**Return**

the previous value or `NULL`.

void \*mt\_prev(struct maple\_tree \*mt, unsigned long index, unsigned long min)
:   get the previous value in the maple tree

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long index`
:   The start index

`unsigned long min`
:   The minimum index to check

**Description**

Takes RCU read lock internally to protect the search, which does not
protect the returned pointer after dropping RCU read lock.
See also: [Maple Tree](#)

**Return**

The entry before **index** or `NULL` if nothing is found.

void mas\_pause(struct ma\_state \*mas)
:   Pause a mas\_find/mas\_for\_each to drop the lock.

**Parameters**

`struct ma_state *mas`
:   The maple state to pause

**Description**

Some users need to pause a walk and drop the lock they’re holding in
order to yield to a higher priority thread or carry out an operation
on an entry. Those users should call this function before they drop
the lock. It resets the **mas** to be suitable for the next iteration
of the loop after the user has reacquired the lock. If most entries
found during a walk require you to call [`mas_pause()`](#c.mas_pause "mas_pause"), the [`mt_for_each()`](#c.mt_for_each "mt_for_each")
iterator may be more appropriate.

bool mas\_find\_setup(struct ma\_state \*mas, unsigned long max, void \*\*entry)
:   Internal function to set up mas\_find\*().

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long max`
:   The maximum index

`void **entry`
:   Pointer to the entry

**Return**

True if entry is the answer, false otherwise.

void \*mas\_find(struct ma\_state \*mas, unsigned long max)
:   On the first call, find the entry at or after mas->index up to `max`. Otherwise, find the entry after mas->index.

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long max`
:   The maximum value to check.

**Description**

Must hold rcu\_read\_lock or the write lock.
If an entry exists, last and index are updated accordingly.
May set **mas->status** to ma\_overflow.

**Return**

The entry or `NULL`.

void \*mas\_find\_range(struct ma\_state \*mas, unsigned long max)
:   On the first call, find the entry at or after mas->index up to `max`. Otherwise, advance to the next slot mas->index.

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long max`
:   The maximum value to check.

**Description**

Must hold rcu\_read\_lock or the write lock.
If an entry exists, last and index are updated accordingly.
May set **mas->status** to ma\_overflow.

**Return**

The entry or `NULL`.

bool mas\_find\_rev\_setup(struct ma\_state \*mas, unsigned long min, void \*\*entry)
:   Internal function to set up mas\_find\_\*`_rev()`

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long min`
:   The minimum index

`void **entry`
:   Pointer to the entry

**Return**

True if entry is the answer, false otherwise.

void \*mas\_find\_rev(struct ma\_state \*mas, unsigned long min)
:   On the first call, find the first non-null entry at or below mas->index down to `min`. Otherwise find the first non-null entry below mas->index down to `min`.

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long min`
:   The minimum value to check.

**Description**

Must hold rcu\_read\_lock or the write lock.
If an entry exists, last and index are updated accordingly.
May set **mas->status** to ma\_underflow.

**Return**

The entry or `NULL`.

void \*mas\_find\_range\_rev(struct ma\_state \*mas, unsigned long min)
:   On the first call, find the first non-null entry at or below mas->index down to `min`. Otherwise advance to the previous slot after mas->index down to `min`.

**Parameters**

`struct ma_state *mas`
:   The maple state

`unsigned long min`
:   The minimum value to check.

**Description**

Must hold rcu\_read\_lock or the write lock.
If an entry exists, last and index are updated accordingly.
May set **mas->status** to ma\_underflow.

**Return**

The entry or `NULL`.

void \*mas\_erase(struct ma\_state \*mas)
:   Find the range in which index resides and erase the entire range.

**Parameters**

`struct ma_state *mas`
:   The maple state

**Description**

Must hold the write lock.
Searches for **mas->index**, sets **mas->index** and **mas->last** to the range and
erases that range.

**Return**

the entry that was erased or `NULL`, **mas->index** and **mas->last** are updated.

bool mas\_nomem(struct ma\_state \*mas, gfp\_t gfp)
:   Check if there was an error allocating and do the allocation if necessary If there are allocations, then free them.

**Parameters**

`struct ma_state *mas`
:   The maple state

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations

**Return**

true on allocation, false otherwise.

void \*mtree\_load(struct maple\_tree \*mt, unsigned long index)
:   Load a value stored in a maple tree

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long index`
:   The index to load

**Return**

the entry or `NULL`

int mtree\_store\_range(struct maple\_tree \*mt, unsigned long index, unsigned long last, void \*entry, gfp\_t gfp)
:   Store an entry at a given range.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long index`
:   The start of the range

`unsigned long last`
:   The end of the range

`void *entry`
:   The entry to store

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations

**Return**

0 on success, -EINVAL on invalid request, -ENOMEM if memory could not
be allocated.

int mtree\_store(struct maple\_tree \*mt, unsigned long index, void \*entry, gfp\_t gfp)
:   Store an entry at a given index.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long index`
:   The index to store the value

`void *entry`
:   The entry to store

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations

**Return**

0 on success, -EINVAL on invalid request, -ENOMEM if memory could not
be allocated.

int mtree\_insert\_range(struct maple\_tree \*mt, unsigned long first, unsigned long last, void \*entry, gfp\_t gfp)
:   Insert an entry at a given range if there is no value.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long first`
:   The start of the range

`unsigned long last`
:   The end of the range

`void *entry`
:   The entry to store

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations.

**Return**

0 on success, -EEXISTS if the range is occupied, -EINVAL on invalid
request, -ENOMEM if memory could not be allocated.

int mtree\_insert(struct maple\_tree \*mt, unsigned long index, void \*entry, gfp\_t gfp)
:   Insert an entry at a given index if there is no value.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long index`
:   The index to store the value

`void *entry`
:   The entry to store

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations.

**Return**

0 on success, -EEXISTS if the range is occupied, -EINVAL on invalid
request, -ENOMEM if memory could not be allocated.

int mtree\_alloc\_cyclic(struct maple\_tree \*mt, unsigned long \*startp, void \*entry, unsigned long range\_lo, unsigned long range\_hi, unsigned long \*next, gfp\_t gfp)
:   Find somewhere to store this entry in the tree.

**Parameters**

`struct maple_tree *mt`
:   The maple tree.

`unsigned long *startp`
:   Pointer to ID.

`void *entry`
:   The entry to store.

`unsigned long range_lo`
:   Lower bound of range to search.

`unsigned long range_hi`
:   Upper bound of range to search.

`unsigned long *next`
:   Pointer to next ID to allocate.

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations.

**Description**

Finds an empty entry in **mt** after **next**, stores the new index into
the **id** pointer, stores the entry at that index, then updates **next**.

**mt** must be initialized with the MT\_FLAGS\_ALLOC\_RANGE flag.

**Context**

Any context. Takes and releases the mt.lock. May sleep if
the **gfp** flags permit.

**Return**

0 if the allocation succeeded without wrapping, 1 if the
allocation succeeded after wrapping, -ENOMEM if memory could not be
allocated, -EINVAL if **mt** cannot be used, or -EBUSY if there are no
free entries.

void \*mtree\_erase(struct maple\_tree \*mt, unsigned long index)
:   Find an index and erase the entire range.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long index`
:   The index to erase

**Description**

Erasing is the same as a walk to an entry then a store of a NULL to that
ENTIRE range. In fact, it is implemented as such using the advanced API.

**Return**

The entry stored at the **index** or `NULL`

int \_\_mt\_dup(struct maple\_tree \*mt, struct maple\_tree \*new, gfp\_t gfp)
:   Duplicate an entire maple tree

**Parameters**

`struct maple_tree *mt`
:   The source maple tree

`struct maple_tree *new`
:   The new maple tree

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations

**Description**

This function duplicates a maple tree in Depth-First Search (DFS) pre-order
traversal. It uses [`memcpy()`](kernel-api.html#c.memcpy "memcpy") to copy nodes in the source tree and allocate
new child nodes in non-leaf nodes. The new node is exactly the same as the
source node except for all the addresses stored in it. It will be faster than
traversing all elements in the source tree and inserting them one by one into
the new tree.
The user needs to ensure that the attributes of the source tree and the new
tree are the same, and the new tree needs to be an empty tree, otherwise
-EINVAL will be returned.
Note that the user needs to manually lock the source tree and the new tree.

**Return**

0 on success, -ENOMEM if memory could not be allocated, -EINVAL If
the attributes of the two trees are different or the new tree is not an empty
tree.

int mtree\_dup(struct maple\_tree \*mt, struct maple\_tree \*new, gfp\_t gfp)
:   Duplicate an entire maple tree

**Parameters**

`struct maple_tree *mt`
:   The source maple tree

`struct maple_tree *new`
:   The new maple tree

`gfp_t gfp`
:   The GFP\_FLAGS to use for allocations

**Description**

This function duplicates a maple tree in Depth-First Search (DFS) pre-order
traversal. It uses [`memcpy()`](kernel-api.html#c.memcpy "memcpy") to copy nodes in the source tree and allocate
new child nodes in non-leaf nodes. The new node is exactly the same as the
source node except for all the addresses stored in it. It will be faster than
traversing all elements in the source tree and inserting them one by one into
the new tree.
The user needs to ensure that the attributes of the source tree and the new
tree are the same, and the new tree needs to be an empty tree, otherwise
-EINVAL will be returned.

**Return**

0 on success, -ENOMEM if memory could not be allocated, -EINVAL If
the attributes of the two trees are different or the new tree is not an empty
tree.

void \_\_mt\_destroy(struct maple\_tree \*mt)
:   Walk and free all nodes of a locked maple tree.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

**Note**

Does not handle locking.

void mtree\_destroy(struct maple\_tree \*mt)
:   Destroy a maple tree

**Parameters**

`struct maple_tree *mt`
:   The maple tree

**Description**

Frees all resources used by the tree. Handles locking.

void \*mt\_find(struct maple\_tree \*mt, unsigned long \*index, unsigned long max)
:   Search from the start up until an entry is found.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long *index`
:   Pointer which contains the start location of the search

`unsigned long max`
:   The maximum value of the search range

**Description**

Takes RCU read lock internally to protect the search, which does not
protect the returned pointer after dropping RCU read lock.
See also: [Maple Tree](#)

In case that an entry is found **index** is updated to point to the next
possible entry independent whether the found entry is occupying a
single index or a range if indices.

**Return**

The entry at or after the **index** or `NULL`

void \*mt\_find\_after(struct maple\_tree \*mt, unsigned long \*index, unsigned long max)
:   Search from the start up until an entry is found.

**Parameters**

`struct maple_tree *mt`
:   The maple tree

`unsigned long *index`
:   Pointer which contains the start location of the search

`unsigned long max`
:   The maximum value to check

**Description**

Same as [`mt_find()`](#c.mt_find "mt_find") except that it checks **index** for 0 before
searching. If **index** == 0, the search is aborted. This covers a wrap
around of **index** to 0 in an iterator loop.

**Return**

The entry at or after the **index** or `NULL`
