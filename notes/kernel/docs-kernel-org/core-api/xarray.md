# XArray

> 출처(원문): https://docs.kernel.org/core-api/xarray.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# XArray

Author:
:   Matthew Wilcox

## Overview

The XArray is an abstract data type which behaves like a very large array
of pointers. It meets many of the same needs as a hash or a conventional
resizable array. Unlike a hash, it allows you to sensibly go to the
next or previous entry in a cache-efficient manner. In contrast to a
resizable array, there is no need to copy data or change MMU mappings in
order to grow the array. It is more memory-efficient, parallelisable
and cache friendly than a doubly-linked list. It takes advantage of
RCU to perform lookups without locking.

The XArray implementation is efficient when the indices used are densely
clustered; hashing the object and using the hash as the index will not
perform well. The XArray is optimised for small indices, but still has
good performance with large indices. If your index can be larger than
`ULONG_MAX` then the XArray is not the data type for you. The most
important user of the XArray is the page cache.

Normal pointers may be stored in the XArray directly. They must be 4-byte
aligned, which is true for any pointer returned from [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc") and
`alloc_page()`. It isn’t true for arbitrary user-space pointers,
nor for function pointers. You can store pointers to statically allocated
objects, as long as those objects have an alignment of at least 4.

You can also store integers between 0 and `LONG_MAX` in the XArray.
You must first convert it into an entry using [`xa_mk_value()`](#c.xa_mk_value "xa_mk_value").
When you retrieve an entry from the XArray, you can check whether it is
a value entry by calling [`xa_is_value()`](#c.xa_is_value "xa_is_value"), and convert it back to
an integer by calling [`xa_to_value()`](#c.xa_to_value "xa_to_value").

Some users want to tag the pointers they store in the XArray. You can
call [`xa_tag_pointer()`](#c.xa_tag_pointer "xa_tag_pointer") to create an entry with a tag, [`xa_untag_pointer()`](#c.xa_untag_pointer "xa_untag_pointer")
to turn a tagged entry back into an untagged pointer and [`xa_pointer_tag()`](#c.xa_pointer_tag "xa_pointer_tag")
to retrieve the tag of an entry. Tagged pointers use the same bits that
are used to distinguish value entries from normal pointers, so you must
decide whether you want to store value entries or tagged pointers in any
particular XArray.

The XArray does not support storing [`IS_ERR()`](kernel-api.html#c.IS_ERR "IS_ERR") pointers as some
conflict with value entries or internal entries.

An unusual feature of the XArray is the ability to create entries which
occupy a range of indices. Once stored to, looking up any index in
the range will return the same entry as looking up any other index in
the range. Storing to any index will store to all of them. Multi-index
entries can be explicitly split into smaller entries. Unsetting (using
[`xa_erase()`](#c.xa_erase "xa_erase") or [`xa_store()`](#c.xa_store "xa_store") with `NULL`) any entry will cause the XArray
to forget about the range.

## Normal API

Start by initialising an XArray, either with [`DEFINE_XARRAY()`](#c.DEFINE_XARRAY "DEFINE_XARRAY")
for statically allocated XArrays or [`xa_init()`](#c.xa_init "xa_init") for dynamically
allocated ones. A freshly-initialised XArray contains a `NULL`
pointer at every index.

You can then set entries using [`xa_store()`](#c.xa_store "xa_store") and get entries using
[`xa_load()`](#c.xa_load "xa_load"). [`xa_store()`](#c.xa_store "xa_store") will overwrite any entry with the new entry and
return the previous entry stored at that index. You can unset entries
using [`xa_erase()`](#c.xa_erase "xa_erase") or by setting the entry to `NULL` using [`xa_store()`](#c.xa_store "xa_store").
There is no difference between an entry that has never been stored to
and one that has been erased with [`xa_erase()`](#c.xa_erase "xa_erase"); an entry that has most
recently had `NULL` stored to it is also equivalent except if the
XArray was initialized with `XA_FLAGS_ALLOC`.

You can conditionally replace an entry at an index by using
[`xa_cmpxchg()`](#c.xa_cmpxchg "xa_cmpxchg"). Like `cmpxchg()`, it will only succeed if
the entry at that index has the ‘old’ value. It also returns the entry
which was at that index; if it returns the same entry which was passed as
‘old’, then [`xa_cmpxchg()`](#c.xa_cmpxchg "xa_cmpxchg") succeeded.

If you want to only store a new entry to an index if the current entry
at that index is `NULL`, you can use [`xa_insert()`](#c.xa_insert "xa_insert") which
returns `-EBUSY` if the entry is not empty.

You can copy entries out of the XArray into a plain array by calling
[`xa_extract()`](#c.xa_extract "xa_extract"). Or you can iterate over the present entries in the XArray
by calling [`xa_for_each()`](#c.xa_for_each "xa_for_each"), [`xa_for_each_start()`](#c.xa_for_each_start "xa_for_each_start") or [`xa_for_each_range()`](#c.xa_for_each_range "xa_for_each_range").
You may prefer to use [`xa_find()`](#c.xa_find "xa_find") or [`xa_find_after()`](#c.xa_find_after "xa_find_after") to move to the next
present entry in the XArray.

Calling [`xa_store_range()`](#c.xa_store_range "xa_store_range") stores the same entry in a range
of indices. If you do this, some of the other operations will behave
in a slightly odd way. For example, marking the entry at one index
may result in the entry being marked at some, but not all of the other
indices. Storing into one index may result in the entry retrieved by
some, but not all of the other indices changing.

Sometimes you need to ensure that a subsequent call to [`xa_store()`](#c.xa_store "xa_store")
will not need to allocate memory. The [`xa_reserve()`](#c.xa_reserve "xa_reserve") function
will store a reserved entry at the indicated index. Users of the
normal API will see this entry as containing `NULL`. If you do
not need to use the reserved entry, you can call [`xa_release()`](#c.xa_release "xa_release")
to remove the unused entry. If another user has stored to the entry
in the meantime, [`xa_release()`](#c.xa_release "xa_release") will do nothing; if instead you
want the entry to become `NULL`, you should use [`xa_erase()`](#c.xa_erase "xa_erase").
Using [`xa_insert()`](#c.xa_insert "xa_insert") on a reserved entry will fail.

If all entries in the array are `NULL`, the [`xa_empty()`](#c.xa_empty "xa_empty") function
will return `true`.

Finally, you can remove all entries from an XArray by calling
[`xa_destroy()`](#c.xa_destroy "xa_destroy"). If the XArray entries are pointers, you may wish
to free the entries first. You can do this by iterating over all present
entries in the XArray using the [`xa_for_each()`](#c.xa_for_each "xa_for_each") iterator.

### Search Marks

Each entry in the array has three bits associated with it called marks.
Each mark may be set or cleared independently of the others. You can
iterate over marked entries by using the [`xa_for_each_marked()`](#c.xa_for_each_marked "xa_for_each_marked") iterator.

You can enquire whether a mark is set on an entry by using
[`xa_get_mark()`](#c.xa_get_mark "xa_get_mark"). If the entry is not `NULL`, you can set a mark on it
by using [`xa_set_mark()`](#c.xa_set_mark "xa_set_mark") and remove the mark from an entry by calling
[`xa_clear_mark()`](#c.xa_clear_mark "xa_clear_mark"). You can ask whether any entry in the XArray has a
particular mark set by calling [`xa_marked()`](#c.xa_marked "xa_marked"). Erasing an entry from the
XArray causes all marks associated with that entry to be cleared.

Setting or clearing a mark on any index of a multi-index entry will
affect all indices covered by that entry. Querying the mark on any
index will return the same result.

There is no way to iterate over entries which are not marked; the data
structure does not allow this to be implemented efficiently. There are
not currently iterators to search for logical combinations of bits (eg
iterate over all entries which have both `XA_MARK_1` and `XA_MARK_2`
set, or iterate over all entries which have `XA_MARK_0` or `XA_MARK_2`
set). It would be possible to add these if a user arises.

### Allocating XArrays

If you use [`DEFINE_XARRAY_ALLOC()`](#c.DEFINE_XARRAY_ALLOC "DEFINE_XARRAY_ALLOC") to define the XArray, or
initialise it by passing `XA_FLAGS_ALLOC` to [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags"),
the XArray changes to track whether entries are in use or not.

You can call [`xa_alloc()`](#c.xa_alloc "xa_alloc") to store the entry at an unused index
in the XArray. If you need to modify the array from interrupt context,
you can use [`xa_alloc_bh()`](#c.xa_alloc_bh "xa_alloc_bh") or [`xa_alloc_irq()`](#c.xa_alloc_irq "xa_alloc_irq") to disable
interrupts while allocating the ID.

Using [`xa_store()`](#c.xa_store "xa_store"), [`xa_cmpxchg()`](#c.xa_cmpxchg "xa_cmpxchg") or [`xa_insert()`](#c.xa_insert "xa_insert") will
also mark the entry as being allocated. Unlike a normal XArray, storing
`NULL` will mark the entry as being in use, like [`xa_reserve()`](#c.xa_reserve "xa_reserve").
To free an entry, use [`xa_erase()`](#c.xa_erase "xa_erase") (or [`xa_release()`](#c.xa_release "xa_release") if
you only want to free the entry if it’s `NULL`).

By default, the lowest free entry is allocated starting from 0. If you
want to allocate entries starting at 1, it is more efficient to use
[`DEFINE_XARRAY_ALLOC1()`](#c.DEFINE_XARRAY_ALLOC1 "DEFINE_XARRAY_ALLOC1") or `XA_FLAGS_ALLOC1`. If you want to
allocate IDs up to a maximum, then wrap back around to the lowest free
ID, you can use [`xa_alloc_cyclic()`](#c.xa_alloc_cyclic "xa_alloc_cyclic").

You cannot use `XA_MARK_0` with an allocating XArray as this mark
is used to track whether an entry is free or not. The other marks are
available for your use.

### Memory allocation

The [`xa_store()`](#c.xa_store "xa_store"), [`xa_cmpxchg()`](#c.xa_cmpxchg "xa_cmpxchg"), [`xa_alloc()`](#c.xa_alloc "xa_alloc"),
[`xa_reserve()`](#c.xa_reserve "xa_reserve") and [`xa_insert()`](#c.xa_insert "xa_insert") functions take a gfp\_t
parameter in case the XArray needs to allocate memory to store this entry.
If the entry is being deleted, no memory allocation needs to be performed,
and the GFP flags specified will be ignored.

It is possible for no memory to be allocatable, particularly if you pass
a restrictive set of GFP flags. In that case, the functions return a
special value which can be turned into an errno using [`xa_err()`](#c.xa_err "xa_err").
If you don’t need to know exactly which error occurred, using
[`xa_is_err()`](#c.xa_is_err "xa_is_err") is slightly more efficient.

### Locking

When using the Normal API, you do not have to worry about locking.
The XArray uses RCU and an internal spinlock to synchronise access:

No lock needed:
:   * [`xa_empty()`](#c.xa_empty "xa_empty")
    * [`xa_marked()`](#c.xa_marked "xa_marked")

Takes RCU read lock:
:   * [`xa_load()`](#c.xa_load "xa_load")
    * [`xa_for_each()`](#c.xa_for_each "xa_for_each")
    * [`xa_for_each_start()`](#c.xa_for_each_start "xa_for_each_start")
    * [`xa_for_each_range()`](#c.xa_for_each_range "xa_for_each_range")
    * [`xa_find()`](#c.xa_find "xa_find")
    * [`xa_find_after()`](#c.xa_find_after "xa_find_after")
    * [`xa_extract()`](#c.xa_extract "xa_extract")
    * [`xa_get_mark()`](#c.xa_get_mark "xa_get_mark")

Takes xa\_lock internally:
:   * [`xa_store()`](#c.xa_store "xa_store")
    * [`xa_store_bh()`](#c.xa_store_bh "xa_store_bh")
    * [`xa_store_irq()`](#c.xa_store_irq "xa_store_irq")
    * [`xa_insert()`](#c.xa_insert "xa_insert")
    * [`xa_insert_bh()`](#c.xa_insert_bh "xa_insert_bh")
    * [`xa_insert_irq()`](#c.xa_insert_irq "xa_insert_irq")
    * [`xa_erase()`](#c.xa_erase "xa_erase")
    * [`xa_erase_bh()`](#c.xa_erase_bh "xa_erase_bh")
    * [`xa_erase_irq()`](#c.xa_erase_irq "xa_erase_irq")
    * [`xa_cmpxchg()`](#c.xa_cmpxchg "xa_cmpxchg")
    * [`xa_cmpxchg_bh()`](#c.xa_cmpxchg_bh "xa_cmpxchg_bh")
    * [`xa_cmpxchg_irq()`](#c.xa_cmpxchg_irq "xa_cmpxchg_irq")
    * [`xa_store_range()`](#c.xa_store_range "xa_store_range")
    * [`xa_alloc()`](#c.xa_alloc "xa_alloc")
    * [`xa_alloc_bh()`](#c.xa_alloc_bh "xa_alloc_bh")
    * [`xa_alloc_irq()`](#c.xa_alloc_irq "xa_alloc_irq")
    * [`xa_reserve()`](#c.xa_reserve "xa_reserve")
    * [`xa_reserve_bh()`](#c.xa_reserve_bh "xa_reserve_bh")
    * [`xa_reserve_irq()`](#c.xa_reserve_irq "xa_reserve_irq")
    * [`xa_destroy()`](#c.xa_destroy "xa_destroy")
    * [`xa_set_mark()`](#c.xa_set_mark "xa_set_mark")
    * [`xa_clear_mark()`](#c.xa_clear_mark "xa_clear_mark")

Assumes xa\_lock held on entry:
:   * [`__xa_store()`](#c.__xa_store "__xa_store")
    * [`__xa_insert()`](#c.__xa_insert "__xa_insert")
    * [`__xa_erase()`](#c.__xa_erase "__xa_erase")
    * [`__xa_cmpxchg()`](#c.__xa_cmpxchg "__xa_cmpxchg")
    * [`__xa_alloc()`](#c.__xa_alloc "__xa_alloc")
    * [`__xa_set_mark()`](#c.__xa_set_mark "__xa_set_mark")
    * [`__xa_clear_mark()`](#c.__xa_clear_mark "__xa_clear_mark")

If you want to take advantage of the lock to protect the data structures
that you are storing in the XArray, you can call `xa_lock()`
before calling [`xa_load()`](#c.xa_load "xa_load"), then take a reference count on the
object you have found before calling `xa_unlock()`. This will
prevent stores from removing the object from the array between looking
up the object and incrementing the refcount. You can also use RCU to
avoid dereferencing freed memory, but an explanation of that is beyond
the scope of this document.

The XArray does not disable interrupts or softirqs while modifying
the array. It is safe to read the XArray from interrupt or softirq
context as the RCU lock provides enough protection.

If, for example, you want to store entries in the XArray in process
context and then erase them in softirq context, you can do that this way:

```
void foo_init(struct foo *foo)
{
    xa_init_flags(&foo->array, XA_FLAGS_LOCK_BH);
}

int foo_store(struct foo *foo, unsigned long index, void *entry)
{
    int err;

    xa_lock_bh(&foo->array);
    err = xa_err(__xa_store(&foo->array, index, entry, GFP_KERNEL));
    if (!err)
        foo->count++;
    xa_unlock_bh(&foo->array);
    return err;
}

/* foo_erase() is only called from softirq context */
void foo_erase(struct foo *foo, unsigned long index)
{
    xa_lock(&foo->array);
    __xa_erase(&foo->array, index);
    foo->count--;
    xa_unlock(&foo->array);
}
```

If you are going to modify the XArray from interrupt or softirq context,
you need to initialise the array using [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags"), passing
`XA_FLAGS_LOCK_IRQ` or `XA_FLAGS_LOCK_BH`.

The above example also shows a common pattern of wanting to extend the
coverage of the xa\_lock on the store side to protect some statistics
associated with the array.

Sharing the XArray with interrupt context is also possible, either
using `xa_lock_irqsave()` in both the interrupt handler and process
context, or `xa_lock_irq()` in process context and `xa_lock()`
in the interrupt handler. Some of the more common patterns have helper
functions such as [`xa_store_bh()`](#c.xa_store_bh "xa_store_bh"), [`xa_store_irq()`](#c.xa_store_irq "xa_store_irq"),
[`xa_erase_bh()`](#c.xa_erase_bh "xa_erase_bh"), [`xa_erase_irq()`](#c.xa_erase_irq "xa_erase_irq"), [`xa_cmpxchg_bh()`](#c.xa_cmpxchg_bh "xa_cmpxchg_bh")
and [`xa_cmpxchg_irq()`](#c.xa_cmpxchg_irq "xa_cmpxchg_irq").

Sometimes you need to protect access to the XArray with a mutex because
that lock sits above another mutex in the locking hierarchy. That does
not entitle you to use functions like [`__xa_erase()`](#c.__xa_erase "__xa_erase") without taking
the xa\_lock; the xa\_lock is used for lockdep validation and will be used
for other purposes in the future.

The [`__xa_set_mark()`](#c.__xa_set_mark "__xa_set_mark") and [`__xa_clear_mark()`](#c.__xa_clear_mark "__xa_clear_mark") functions are also
available for situations where you look up an entry and want to atomically
set or clear a mark. It may be more efficient to use the advanced API
in this case, as it will save you from walking the tree twice.

## Advanced API

The advanced API offers more flexibility and better performance at the
cost of an interface which can be harder to use and has fewer safeguards.
No locking is done for you by the advanced API, and you are required
to use the xa\_lock while modifying the array. You can choose whether
to use the xa\_lock or the RCU lock while doing read-only operations on
the array. You can mix advanced and normal operations on the same array;
indeed the normal API is implemented in terms of the advanced API. The
advanced API is only available to modules with a GPL-compatible license.

The advanced API is based around the xa\_state. This is an opaque data
structure which you declare on the stack using the [`XA_STATE()`](#c.XA_STATE "XA_STATE") macro.
This macro initialises the xa\_state ready to start walking around the
XArray. It is used as a cursor to maintain the position in the XArray
and let you compose various operations together without having to restart
from the top every time. The contents of the xa\_state are protected by
the [`rcu_read_lock()`](kernel-api.html#c.rcu_read_lock "rcu_read_lock") or the `xas_lock()`. If you need to drop whichever of
those locks is protecting your state and tree, you must call [`xas_pause()`](#c.xas_pause "xas_pause")
so that future calls do not rely on the parts of the state which were
left unprotected.

The xa\_state is also used to store errors. You can call
[`xas_error()`](#c.xas_error "xas_error") to retrieve the error. All operations check whether
the xa\_state is in an error state before proceeding, so there’s no need
for you to check for an error after each call; you can make multiple
calls in succession and only check at a convenient point. The only
errors currently generated by the XArray code itself are `ENOMEM` and
`EINVAL`, but it supports arbitrary errors in case you want to call
[`xas_set_err()`](#c.xas_set_err "xas_set_err") yourself.

If the xa\_state is holding an `ENOMEM` error, calling [`xas_nomem()`](#c.xas_nomem "xas_nomem")
will attempt to allocate more memory using the specified gfp flags and
cache it in the xa\_state for the next attempt. The idea is that you take
the xa\_lock, attempt the operation and drop the lock. The operation
attempts to allocate memory while holding the lock, but it is more
likely to fail. Once you have dropped the lock, [`xas_nomem()`](#c.xas_nomem "xas_nomem")
can try harder to allocate more memory. It will return `true` if it
is worth retrying the operation (i.e. that there was a memory error *and*
more memory was allocated). If it has previously allocated memory, and
that memory wasn’t used, and there is no error (or some error that isn’t
`ENOMEM`), then it will free the memory previously allocated.

### Internal Entries

The XArray reserves some entries for its own purposes. These are never
exposed through the normal API, but when using the advanced API, it’s
possible to see them. Usually the best way to handle them is to pass them
to [`xas_retry()`](#c.xas_retry "xas_retry"), and retry the operation if it returns `true`.

|  |  |  |
| --- | --- | --- |
| Name | Test | Usage |
| Node | `xa_is_node()` | An XArray node. May be visible when using a multi-index xa\_state. |
| Sibling | [`xa_is_sibling()`](#c.xa_is_sibling "xa_is_sibling") | A non-canonical entry for a multi-index entry. The value indicates which slot in this node has the canonical entry. |
| Retry | [`xa_is_retry()`](#c.xa_is_retry "xa_is_retry") | This entry is currently being modified by a thread which has the xa\_lock. The node containing this entry may be freed at the end of this RCU period. You should restart the lookup from the head of the array. |
| Zero | [`xa_is_zero()`](#c.xa_is_zero "xa_is_zero") | Zero entries appear as `NULL` through the Normal API, but occupy an entry in the XArray which can be used to reserve the index for future use. This is used by allocating XArrays for allocated entries which are `NULL`. |

Other internal entries may be added in the future. As far as possible, they
will be handled by [`xas_retry()`](#c.xas_retry "xas_retry").

### Additional functionality

The [`xas_create_range()`](#c.xas_create_range "xas_create_range") function allocates all the necessary memory
to store every entry in a range. It will set ENOMEM in the xa\_state if
it cannot allocate memory.

You can use [`xas_init_marks()`](#c.xas_init_marks "xas_init_marks") to reset the marks on an entry
to their default state. This is usually all marks clear, unless the
XArray is marked with `XA_FLAGS_TRACK_FREE`, in which case mark 0 is set
and all other marks are clear. Replacing one entry with another using
[`xas_store()`](#c.xas_store "xas_store") will not reset the marks on that entry; if you want
the marks reset, you should do that explicitly.

The [`xas_load()`](#c.xas_load "xas_load") will walk the xa\_state as close to the entry
as it can. If you know the xa\_state has already been walked to the
entry and need to check that the entry hasn’t changed, you can use
[`xas_reload()`](#c.xas_reload "xas_reload") to save a function call.

If you need to move to a different index in the XArray, call
[`xas_set()`](#c.xas_set "xas_set"). This resets the cursor to the top of the tree, which
will generally make the next operation walk the cursor to the desired
spot in the tree. If you want to move to the next or previous index,
call [`xas_next()`](#c.xas_next "xas_next") or [`xas_prev()`](#c.xas_prev "xas_prev"). Setting the index does
not walk the cursor around the array so does not require a lock to be
held, while moving to the next or previous index does.

You can search for the next present entry using [`xas_find()`](#c.xas_find "xas_find"). This
is the equivalent of both [`xa_find()`](#c.xa_find "xa_find") and [`xa_find_after()`](#c.xa_find_after "xa_find_after");
if the cursor has been walked to an entry, then it will find the next
entry after the one currently referenced. If not, it will return the
entry at the index of the xa\_state. Using [`xas_next_entry()`](#c.xas_next_entry "xas_next_entry") to
move to the next present entry instead of [`xas_find()`](#c.xas_find "xas_find") will save
a function call in the majority of cases at the expense of emitting more
inline code.

The [`xas_find_marked()`](#c.xas_find_marked "xas_find_marked") function is similar. If the xa\_state has
not been walked, it will return the entry at the index of the xa\_state,
if it is marked. Otherwise, it will return the first marked entry after
the entry referenced by the xa\_state. The [`xas_next_marked()`](#c.xas_next_marked "xas_next_marked")
function is the equivalent of [`xas_next_entry()`](#c.xas_next_entry "xas_next_entry").

When iterating over a range of the XArray using [`xas_for_each()`](#c.xas_for_each "xas_for_each")
or [`xas_for_each_marked()`](#c.xas_for_each_marked "xas_for_each_marked"), it may be necessary to temporarily stop
the iteration. The [`xas_pause()`](#c.xas_pause "xas_pause") function exists for this purpose.
After you have done the necessary work and wish to resume, the xa\_state
is in an appropriate state to continue the iteration after the entry
you last processed. If you have interrupts disabled while iterating,
then it is good manners to pause the iteration and reenable interrupts
every `XA_CHECK_SCHED` entries.

The [`xas_get_mark()`](#c.xas_get_mark "xas_get_mark"), [`xas_set_mark()`](#c.xas_set_mark "xas_set_mark") and [`xas_clear_mark()`](#c.xas_clear_mark "xas_clear_mark") functions require
the xa\_state cursor to have been moved to the appropriate location in the
XArray; they will do nothing if you have called [`xas_pause()`](#c.xas_pause "xas_pause") or [`xas_set()`](#c.xas_set "xas_set")
immediately before.

You can call [`xas_set_update()`](#c.xas_set_update "xas_set_update") to have a callback function
called each time the XArray updates a node. This is used by the page
cache workingset code to maintain its list of nodes which contain only
shadow entries.

### Multi-Index Entries

The XArray has the ability to tie multiple indices together so that
operations on one index affect all indices. For example, storing into
any index will change the value of the entry retrieved from any index.
Setting or clearing a mark on any index will set or clear the mark
on every index that is tied together. The current implementation
only allows tying ranges which are aligned powers of two together;
eg indices 64-127 may be tied together, but 2-6 may not be. This may
save substantial quantities of memory; for example tying 512 entries
together will save over 4kB.

You can create a multi-index entry by using [`XA_STATE_ORDER()`](#c.XA_STATE_ORDER "XA_STATE_ORDER")
or [`xas_set_order()`](#c.xas_set_order "xas_set_order") followed by a call to [`xas_store()`](#c.xas_store "xas_store").
Calling [`xas_load()`](#c.xas_load "xas_load") with a multi-index xa\_state will walk the
xa\_state to the right location in the tree, but the return value is not
meaningful, potentially being an internal entry or `NULL` even when there
is an entry stored within the range. Calling [`xas_find_conflict()`](#c.xas_find_conflict "xas_find_conflict")
will return the first entry within the range or `NULL` if there are no
entries in the range. The [`xas_for_each_conflict()`](#c.xas_for_each_conflict "xas_for_each_conflict") iterator will
iterate over every entry which overlaps the specified range.

If [`xas_load()`](#c.xas_load "xas_load") encounters a multi-index entry, the xa\_index
in the xa\_state will not be changed. When iterating over an XArray
or calling [`xas_find()`](#c.xas_find "xas_find"), if the initial index is in the middle
of a multi-index entry, it will not be altered. Subsequent calls
or iterations will move the index to the first index in the range.
Each entry will only be returned once, no matter how many indices it
occupies.

Using [`xas_next()`](#c.xas_next "xas_next") or [`xas_prev()`](#c.xas_prev "xas_prev") with a multi-index xa\_state is not
supported. Using either of these functions on a multi-index entry will
reveal sibling entries; these should be skipped over by the caller.

Storing `NULL` into any index of a multi-index entry will set the
entry at every index to `NULL` and dissolve the tie. A multi-index
entry can be split into entries occupying smaller ranges by calling
[`xas_split_alloc()`](#c.xas_split_alloc "xas_split_alloc") without the xa\_lock held, followed by taking the lock
and calling [`xas_split()`](#c.xas_split "xas_split") or calling [`xas_try_split()`](#c.xas_try_split "xas_try_split") with xa\_lock. The
difference between [`xas_split_alloc()`](#c.xas_split_alloc "xas_split_alloc")+[`xas_split()`](#c.xas_split "xas_split") and `xas_try_alloc()` is
that [`xas_split_alloc()`](#c.xas_split_alloc "xas_split_alloc") + [`xas_split()`](#c.xas_split "xas_split") split the entry from the original
order to the new order in one shot uniformly, whereas [`xas_try_split()`](#c.xas_try_split "xas_try_split")
iteratively splits the entry containing the index non-uniformly.
For example, to split an order-9 entry, which takes 2^(9-6)=8 slots,
assuming `XA_CHUNK_SHIFT` is 6, [`xas_split_alloc()`](#c.xas_split_alloc "xas_split_alloc") + [`xas_split()`](#c.xas_split "xas_split") need
8 xa\_node. [`xas_try_split()`](#c.xas_try_split "xas_try_split") splits the order-9 entry into
2 order-8 entries, then split one order-8 entry, based on the given index,
to 2 order-7 entries, ..., and split one order-1 entry to 2 order-0 entries.
When splitting the order-6 entry and a new xa\_node is needed, [`xas_try_split()`](#c.xas_try_split "xas_try_split")
will try to allocate one if possible. As a result, [`xas_try_split()`](#c.xas_try_split "xas_try_split") would only
need 1 xa\_node instead of 8.

## Functions and structures

void \*xa\_mk\_value(unsigned long v)
:   Create an XArray entry from an integer.

**Parameters**

`unsigned long v`
:   Value to store in XArray.

**Context**

Any context.

**Return**

An entry suitable for storing in the XArray.

unsigned long xa\_to\_value(const void \*entry)
:   Get value stored in an XArray entry.

**Parameters**

`const void *entry`
:   XArray entry.

**Context**

Any context.

**Return**

The value stored in the XArray entry.

bool xa\_is\_value(const void \*entry)
:   Determine if an entry is a value.

**Parameters**

`const void *entry`
:   XArray entry.

**Context**

Any context.

**Return**

True if the entry is a value, false if it is a pointer.

void \*xa\_tag\_pointer(void \*p, unsigned long tag)
:   Create an XArray entry for a tagged pointer.

**Parameters**

`void *p`
:   Plain pointer.

`unsigned long tag`
:   Tag value (0, 1 or 3).

**Description**

If the user of the XArray prefers, they can tag their pointers instead
of storing value entries. Three tags are available (0, 1 and 3).
These are distinct from the xa\_mark\_t as they are not replicated up
through the array and cannot be searched for.

**Context**

Any context.

**Return**

An XArray entry.

void \*xa\_untag\_pointer(void \*entry)
:   Turn an XArray entry into a plain pointer.

**Parameters**

`void *entry`
:   XArray entry.

**Description**

If you have stored a tagged pointer in the XArray, call this function
to get the untagged version of the pointer.

**Context**

Any context.

**Return**

A pointer.

unsigned int xa\_pointer\_tag(void \*entry)
:   Get the tag stored in an XArray entry.

**Parameters**

`void *entry`
:   XArray entry.

**Description**

If you have stored a tagged pointer in the XArray, call this function
to get the tag of that pointer.

**Context**

Any context.

**Return**

A tag.

bool xa\_is\_zero(const void \*entry)
:   Is the entry a zero entry?

**Parameters**

`const void *entry`
:   Entry retrieved from the XArray

**Description**

The normal API will return NULL as the contents of a slot containing
a zero entry. You can only see zero entries by using the advanced API.

**Return**

`true` if the entry is a zero entry.

bool xa\_is\_err(const void \*entry)
:   Report whether an XArray operation returned an error

**Parameters**

`const void *entry`
:   Result from calling an XArray function

**Description**

If an XArray operation cannot complete an operation, it will return
a special value indicating an error. This function tells you
whether an error occurred; [`xa_err()`](#c.xa_err "xa_err") tells you which error occurred.

**Context**

Any context.

**Return**

`true` if the entry indicates an error.

int xa\_err(void \*entry)
:   Turn an XArray result into an errno.

**Parameters**

`void *entry`
:   Result from calling an XArray function.

**Description**

If an XArray operation cannot complete an operation, it will return
a special pointer value which encodes an errno. This function extracts
the errno from the pointer value, or returns 0 if the pointer does not
represent an errno.

**Context**

Any context.

**Return**

A negative errno or 0.

struct xa\_limit
:   Represents a range of IDs.

**Definition**:

```
struct xa_limit {
    u32 max;
    u32 min;
};
```

**Members**

`max`
:   The maximum ID to allocate (inclusive).

`min`
:   The lowest ID to allocate (inclusive).

**Description**

This structure is used either directly or via the `XA_LIMIT()` macro
to communicate the range of IDs that are valid for allocation.
Three common ranges are predefined for you:
\* xa\_limit\_32b - [0 - UINT\_MAX]
\* xa\_limit\_31b - [0 - INT\_MAX]
\* xa\_limit\_16b - [0 - USHRT\_MAX]

struct xarray
:   The anchor of the XArray.

**Definition**:

```
struct xarray {
    spinlock_t xa_lock;
};
```

**Members**

`xa_lock`
:   Lock that protects the contents of the XArray.

**Description**

To use the xarray, define it statically or embed it in your data structure.
It is a very small data structure, so it does not usually make sense to
allocate it separately and keep a pointer to it in your data structure.

You may use the xa\_lock to protect your own data structures as well.

DEFINE\_XARRAY\_FLAGS

`DEFINE_XARRAY_FLAGS (name, flags)`

> Define an XArray with custom flags.

**Parameters**

`name`
:   A string that names your XArray.

`flags`
:   XA\_FLAG values.

**Description**

This is intended for file scope definitions of XArrays. It declares
and initialises an empty XArray with the chosen name and flags. It is
equivalent to calling [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags") on the array, but it does the
initialisation at compiletime instead of runtime.

DEFINE\_XARRAY

`DEFINE_XARRAY (name)`

> Define an XArray.

**Parameters**

`name`
:   A string that names your XArray.

**Description**

This is intended for file scope definitions of XArrays. It declares
and initialises an empty XArray with the chosen name. It is equivalent
to calling [`xa_init()`](#c.xa_init "xa_init") on the array, but it does the initialisation at
compiletime instead of runtime.

DEFINE\_XARRAY\_ALLOC

`DEFINE_XARRAY_ALLOC (name)`

> Define an XArray which allocates IDs starting at 0.

**Parameters**

`name`
:   A string that names your XArray.

**Description**

This is intended for file scope definitions of allocating XArrays.
See also [`DEFINE_XARRAY()`](#c.DEFINE_XARRAY "DEFINE_XARRAY").

DEFINE\_XARRAY\_ALLOC1

`DEFINE_XARRAY_ALLOC1 (name)`

> Define an XArray which allocates IDs starting at 1.

**Parameters**

`name`
:   A string that names your XArray.

**Description**

This is intended for file scope definitions of allocating XArrays.
See also [`DEFINE_XARRAY()`](#c.DEFINE_XARRAY "DEFINE_XARRAY").

void xa\_init\_flags(struct [xarray](#c.xarray "xarray") \*xa, gfp\_t flags)
:   Initialise an empty XArray with flags.

**Parameters**

`struct xarray *xa`
:   XArray.

`gfp_t flags`
:   XA\_FLAG values.

**Description**

If you need to initialise an XArray with special flags (eg you need
to take the lock from interrupt context), use this function instead
of [`xa_init()`](#c.xa_init "xa_init").

**Context**

Any context.

void xa\_init(struct [xarray](#c.xarray "xarray") \*xa)
:   Initialise an empty XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

**Description**

An empty XArray is full of NULL entries.

**Context**

Any context.

bool xa\_empty(const struct [xarray](#c.xarray "xarray") \*xa)
:   Determine if an array has any present entries.

**Parameters**

`const struct xarray *xa`
:   XArray.

**Context**

Any context.

**Return**

`true` if the array contains only NULL pointers.

bool xa\_marked(const struct [xarray](#c.xarray "xarray") \*xa, xa\_mark\_t mark)
:   Inquire whether any entry in this array has a mark set

**Parameters**

`const struct xarray *xa`
:   Array

`xa_mark_t mark`
:   Mark value

**Context**

Any context.

**Return**

`true` if any entry has this mark set.

xa\_for\_each\_range

`xa_for_each_range (xa, index, entry, start, last)`

> Iterate over a portion of an XArray.

**Parameters**

`xa`
:   XArray.

`index`
:   Index of **entry**.

`entry`
:   Entry retrieved from array.

`start`
:   First index to retrieve from array.

`last`
:   Last index to retrieve from array.

**Description**

During the iteration, **entry** will have the value of the entry stored
in **xa** at **index**. You may modify **index** during the iteration if you
want to skip or reprocess indices. It is safe to modify the array
during the iteration. At the end of the iteration, **entry** will be set
to NULL and **index** will have a value less than or equal to max.

[`xa_for_each_range()`](#c.xa_for_each_range "xa_for_each_range") is O(n.log(n)) while [`xas_for_each()`](#c.xas_for_each "xas_for_each") is O(n). You have
to handle your own locking with [`xas_for_each()`](#c.xas_for_each "xas_for_each"), and if you have to unlock
after each iteration, it will also end up being O(n.log(n)).
[`xa_for_each_range()`](#c.xa_for_each_range "xa_for_each_range") will spin if it hits a retry entry; if you intend to
see retry entries, you should use the [`xas_for_each()`](#c.xas_for_each "xas_for_each") iterator instead.
The [`xas_for_each()`](#c.xas_for_each "xas_for_each") iterator will expand into more inline code than
[`xa_for_each_range()`](#c.xa_for_each_range "xa_for_each_range").

**Context**

Any context. Takes and releases the RCU lock.

xa\_for\_each\_start

`xa_for_each_start (xa, index, entry, start)`

> Iterate over a portion of an XArray.

**Parameters**

`xa`
:   XArray.

`index`
:   Index of **entry**.

`entry`
:   Entry retrieved from array.

`start`
:   First index to retrieve from array.

**Description**

During the iteration, **entry** will have the value of the entry stored
in **xa** at **index**. You may modify **index** during the iteration if you
want to skip or reprocess indices. It is safe to modify the array
during the iteration. At the end of the iteration, **entry** will be set
to NULL and **index** will have a value less than or equal to max.

[`xa_for_each_start()`](#c.xa_for_each_start "xa_for_each_start") is O(n.log(n)) while [`xas_for_each()`](#c.xas_for_each "xas_for_each") is O(n). You have
to handle your own locking with [`xas_for_each()`](#c.xas_for_each "xas_for_each"), and if you have to unlock
after each iteration, it will also end up being O(n.log(n)).
[`xa_for_each_start()`](#c.xa_for_each_start "xa_for_each_start") will spin if it hits a retry entry; if you intend to
see retry entries, you should use the [`xas_for_each()`](#c.xas_for_each "xas_for_each") iterator instead.
The [`xas_for_each()`](#c.xas_for_each "xas_for_each") iterator will expand into more inline code than
[`xa_for_each_start()`](#c.xa_for_each_start "xa_for_each_start").

**Context**

Any context. Takes and releases the RCU lock.

xa\_for\_each

`xa_for_each (xa, index, entry)`

> Iterate over present entries in an XArray.

**Parameters**

`xa`
:   XArray.

`index`
:   Index of **entry**.

`entry`
:   Entry retrieved from array.

**Description**

During the iteration, **entry** will have the value of the entry stored
in **xa** at **index**. You may modify **index** during the iteration if you want
to skip or reprocess indices. It is safe to modify the array during the
iteration. At the end of the iteration, **entry** will be set to NULL and
**index** will have a value less than or equal to max.

[`xa_for_each()`](#c.xa_for_each "xa_for_each") is O(n.log(n)) while [`xas_for_each()`](#c.xas_for_each "xas_for_each") is O(n). You have
to handle your own locking with [`xas_for_each()`](#c.xas_for_each "xas_for_each"), and if you have to unlock
after each iteration, it will also end up being O(n.log(n)). [`xa_for_each()`](#c.xa_for_each "xa_for_each")
will spin if it hits a retry entry; if you intend to see retry entries,
you should use the [`xas_for_each()`](#c.xas_for_each "xas_for_each") iterator instead. The [`xas_for_each()`](#c.xas_for_each "xas_for_each")
iterator will expand into more inline code than [`xa_for_each()`](#c.xa_for_each "xa_for_each").

**Context**

Any context. Takes and releases the RCU lock.

xa\_for\_each\_marked

`xa_for_each_marked (xa, index, entry, filter)`

> Iterate over marked entries in an XArray.

**Parameters**

`xa`
:   XArray.

`index`
:   Index of **entry**.

`entry`
:   Entry retrieved from array.

`filter`
:   Selection criterion.

**Description**

During the iteration, **entry** will have the value of the entry stored
in **xa** at **index**. The iteration will skip all entries in the array
which do not match **filter**. You may modify **index** during the iteration
if you want to skip or reprocess indices. It is safe to modify the array
during the iteration. At the end of the iteration, **entry** will be set to
NULL and **index** will have a value less than or equal to max.

[`xa_for_each_marked()`](#c.xa_for_each_marked "xa_for_each_marked") is O(n.log(n)) while [`xas_for_each_marked()`](#c.xas_for_each_marked "xas_for_each_marked") is O(n).
You have to handle your own locking with [`xas_for_each()`](#c.xas_for_each "xas_for_each"), and if you have
to unlock after each iteration, it will also end up being O(n.log(n)).
[`xa_for_each_marked()`](#c.xa_for_each_marked "xa_for_each_marked") will spin if it hits a retry entry; if you intend to
see retry entries, you should use the [`xas_for_each_marked()`](#c.xas_for_each_marked "xas_for_each_marked") iterator
instead. The [`xas_for_each_marked()`](#c.xas_for_each_marked "xas_for_each_marked") iterator will expand into more inline
code than [`xa_for_each_marked()`](#c.xa_for_each_marked "xa_for_each_marked").

**Context**

Any context. Takes and releases the RCU lock.

void \*xa\_store\_bh(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

This function is like calling [`xa_store()`](#c.xa_store "xa_store") except it disables softirqs
while holding the array lock.

**Context**

Any context. Takes and releases the xa\_lock while
disabling softirqs.

**Return**

The old entry at this index or [`xa_err()`](#c.xa_err "xa_err") if an error happened.

void \*xa\_store\_irq(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

This function is like calling [`xa_store()`](#c.xa_store "xa_store") except it disables interrupts
while holding the array lock.

**Context**

Process context. Takes and releases the xa\_lock while
disabling interrupts.

**Return**

The old entry at this index or [`xa_err()`](#c.xa_err "xa_err") if an error happened.

void \*xa\_erase\_bh(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index)
:   Erase this entry from the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

**Description**

After this function returns, loading from **index** will return `NULL`.
If the index is part of a multi-index entry, all indices will be erased
and none of the entries will be part of a multi-index entry.

**Context**

Any context. Takes and releases the xa\_lock while
disabling softirqs.

**Return**

The entry which used to be at this index.

void \*xa\_erase\_irq(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index)
:   Erase this entry from the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

**Description**

After this function returns, loading from **index** will return `NULL`.
If the index is part of a multi-index entry, all indices will be erased
and none of the entries will be part of a multi-index entry.

**Context**

Process context. Takes and releases the xa\_lock while
disabling interrupts.

**Return**

The entry which used to be at this index.

void \*xa\_cmpxchg(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*old, void \*entry, gfp\_t gfp)
:   Conditionally replace an entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *old`
:   Old value to test against.

`void *entry`
:   New value to place in array.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

If the entry at **index** is the same as **old**, replace it with **entry**.
If the return value is equal to **old**, then the exchange was successful.

**Context**

Any context. Takes and releases the xa\_lock. May sleep
if the **gfp** flags permit.

**Return**

The old value at this index or [`xa_err()`](#c.xa_err "xa_err") if an error happened.

void \*xa\_cmpxchg\_bh(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*old, void \*entry, gfp\_t gfp)
:   Conditionally replace an entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *old`
:   Old value to test against.

`void *entry`
:   New value to place in array.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

This function is like calling [`xa_cmpxchg()`](#c.xa_cmpxchg "xa_cmpxchg") except it disables softirqs
while holding the array lock.

**Context**

Any context. Takes and releases the xa\_lock while
disabling softirqs. May sleep if the **gfp** flags permit.

**Return**

The old value at this index or [`xa_err()`](#c.xa_err "xa_err") if an error happened.

void \*xa\_cmpxchg\_irq(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*old, void \*entry, gfp\_t gfp)
:   Conditionally replace an entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *old`
:   Old value to test against.

`void *entry`
:   New value to place in array.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

This function is like calling [`xa_cmpxchg()`](#c.xa_cmpxchg "xa_cmpxchg") except it disables interrupts
while holding the array lock.

**Context**

Process context. Takes and releases the xa\_lock while
disabling interrupts. May sleep if the **gfp** flags permit.

**Return**

The old value at this index or [`xa_err()`](#c.xa_err "xa_err") if an error happened.

int xa\_insert(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray unless another entry is already present.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Inserting a NULL entry will store a reserved entry (like [`xa_reserve()`](#c.xa_reserve "xa_reserve"))
if no entry is present. Inserting will fail if a reserved entry is
present, even though loading from this index will return NULL.

**Context**

Any context. Takes and releases the xa\_lock. May sleep if
the **gfp** flags permit.

**Return**

0 if the store succeeded. -EBUSY if another entry was present.
-ENOMEM if memory could not be allocated.

int xa\_insert\_bh(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray unless another entry is already present.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Inserting a NULL entry will store a reserved entry (like [`xa_reserve()`](#c.xa_reserve "xa_reserve"))
if no entry is present. Inserting will fail if a reserved entry is
present, even though loading from this index will return NULL.

**Context**

Any context. Takes and releases the xa\_lock while
disabling softirqs. May sleep if the **gfp** flags permit.

**Return**

0 if the store succeeded. -EBUSY if another entry was present.
-ENOMEM if memory could not be allocated.

int xa\_insert\_irq(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray unless another entry is already present.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Inserting a NULL entry will store a reserved entry (like [`xa_reserve()`](#c.xa_reserve "xa_reserve"))
if no entry is present. Inserting will fail if a reserved entry is
present, even though loading from this index will return NULL.

**Context**

Process context. Takes and releases the xa\_lock while
disabling interrupts. May sleep if the **gfp** flags permit.

**Return**

0 if the store succeeded. -EBUSY if another entry was present.
-ENOMEM if memory could not be allocated.

int xa\_alloc(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range of ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

**Context**

Any context. Takes and releases the xa\_lock. May sleep if
the **gfp** flags permit.

**Return**

0 on success, -ENOMEM if memory could not be allocated or
-EBUSY if there are no free entries in **limit**.

int xa\_alloc\_bh(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range of ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

**Context**

Any context. Takes and releases the xa\_lock while
disabling softirqs. May sleep if the **gfp** flags permit.

**Return**

0 on success, -ENOMEM if memory could not be allocated or
-EBUSY if there are no free entries in **limit**.

int xa\_alloc\_irq(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range of ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

**Context**

Process context. Takes and releases the xa\_lock while
disabling interrupts. May sleep if the **gfp** flags permit.

**Return**

0 on success, -ENOMEM if memory could not be allocated or
-EBUSY if there are no free entries in **limit**.

int xa\_alloc\_cyclic(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, u32 \*next, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range of allocated ID.

`u32 *next`
:   Pointer to next ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.
The search for an empty entry will start at **next** and will wrap
around if necessary.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

Note that callers interested in whether wrapping has occurred should
use [`__xa_alloc_cyclic()`](#c.__xa_alloc_cyclic "__xa_alloc_cyclic") instead.

**Context**

Any context. Takes and releases the xa\_lock. May sleep if
the **gfp** flags permit.

**Return**

0 if the allocation succeeded, -ENOMEM if memory could not be
allocated or -EBUSY if there are no free entries in **limit**.

int xa\_alloc\_cyclic\_bh(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, u32 \*next, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range of allocated ID.

`u32 *next`
:   Pointer to next ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.
The search for an empty entry will start at **next** and will wrap
around if necessary.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

Note that callers interested in whether wrapping has occurred should
use [`__xa_alloc_cyclic()`](#c.__xa_alloc_cyclic "__xa_alloc_cyclic") instead.

**Context**

Any context. Takes and releases the xa\_lock while
disabling softirqs. May sleep if the **gfp** flags permit.

**Return**

0 if the allocation succeeded, -ENOMEM if memory could not be
allocated or -EBUSY if there are no free entries in **limit**.

int xa\_alloc\_cyclic\_irq(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, u32 \*next, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range of allocated ID.

`u32 *next`
:   Pointer to next ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.
The search for an empty entry will start at **next** and will wrap
around if necessary.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

Note that callers interested in whether wrapping has occurred should
use [`__xa_alloc_cyclic()`](#c.__xa_alloc_cyclic "__xa_alloc_cyclic") instead.

**Context**

Process context. Takes and releases the xa\_lock while
disabling interrupts. May sleep if the **gfp** flags permit.

**Return**

0 if the allocation succeeded, -ENOMEM if memory could not be
allocated or -EBUSY if there are no free entries in **limit**.

int xa\_reserve(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, gfp\_t gfp)
:   Reserve this index in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Ensures there is somewhere to store an entry at **index** in the array.
If there is already something stored at **index**, this function does
nothing. If there was nothing there, the entry is marked as reserved.
Loading from a reserved entry returns a `NULL` pointer.

If you do not use the entry that you have reserved, call [`xa_release()`](#c.xa_release "xa_release")
or [`xa_erase()`](#c.xa_erase "xa_erase") to free any unnecessary memory.

**Context**

Any context. Takes and releases the xa\_lock.
May sleep if the **gfp** flags permit.

**Return**

0 if the reservation succeeded or -ENOMEM if it failed.

int xa\_reserve\_bh(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, gfp\_t gfp)
:   Reserve this index in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

A softirq-disabling version of [`xa_reserve()`](#c.xa_reserve "xa_reserve").

**Context**

Any context. Takes and releases the xa\_lock while
disabling softirqs.

**Return**

0 if the reservation succeeded or -ENOMEM if it failed.

int xa\_reserve\_irq(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, gfp\_t gfp)
:   Reserve this index in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

An interrupt-disabling version of [`xa_reserve()`](#c.xa_reserve "xa_reserve").

**Context**

Process context. Takes and releases the xa\_lock while
disabling interrupts.

**Return**

0 if the reservation succeeded or -ENOMEM if it failed.

void xa\_release(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index)
:   Release a reserved entry.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

**Description**

After calling [`xa_reserve()`](#c.xa_reserve "xa_reserve"), you can call this function to release the
reservation. If the entry at **index** has been stored to, this function
will do nothing.

bool xa\_is\_sibling(const void \*entry)
:   Is the entry a sibling entry?

**Parameters**

`const void *entry`
:   Entry retrieved from the XArray

**Return**

`true` if the entry is a sibling entry.

bool xa\_is\_retry(const void \*entry)
:   Is the entry a retry entry?

**Parameters**

`const void *entry`
:   Entry retrieved from the XArray

**Return**

`true` if the entry is a retry entry.

bool xa\_is\_advanced(const void \*entry)
:   Is the entry only permitted for the advanced API?

**Parameters**

`const void *entry`
:   Entry to be stored in the XArray.

**Return**

`true` if the entry cannot be stored by the normal API.

xa\_update\_node\_t
:   **Typedef**: A callback function from the XArray.

**Syntax**

> `void xa_update_node_t (struct xa_node *node)`

**Parameters**

`struct xa_node *node`
:   The node which is being processed

**Description**

This function is called every time the XArray updates the count of
present and value entries in a node. It allows advanced users to
maintain the private\_list in the node.

**Context**

The xa\_lock is held and interrupts may be disabled.
Implementations should not drop the xa\_lock, nor re-enable
interrupts.

XA\_STATE

`XA_STATE (name, array, index)`

> Declare an XArray operation state.

**Parameters**

`name`
:   Name of this operation state (usually xas).

`array`
:   Array to operate on.

`index`
:   Initial index of interest.

**Description**

Declare and initialise an xa\_state on the stack.

XA\_STATE\_ORDER

`XA_STATE_ORDER (name, array, index, order)`

> Declare an XArray operation state.

**Parameters**

`name`
:   Name of this operation state (usually xas).

`array`
:   Array to operate on.

`index`
:   Initial index of interest.

`order`
:   Order of entry.

**Description**

Declare and initialise an xa\_state on the stack. This variant of
[`XA_STATE()`](#c.XA_STATE "XA_STATE") allows you to specify the ‘order’ of the element you
want to operate on.`

int xas\_error(const struct xa\_state \*xas)
:   Return an errno stored in the xa\_state.

**Parameters**

`const struct xa_state *xas`
:   XArray operation state.

**Return**

0 if no error has been noted. A negative errno if one has.

void xas\_set\_err(struct xa\_state \*xas, long err)
:   Note an error in the xa\_state.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`long err`
:   Negative error number.

**Description**

Only call this function with a negative **err**; zero or positive errors
will probably not behave the way you think they should. If you want
to clear the error from an xa\_state, use [`xas_reset()`](#c.xas_reset "xas_reset").

bool xas\_invalid(const struct xa\_state \*xas)
:   Is the xas in a retry or error state?

**Parameters**

`const struct xa_state *xas`
:   XArray operation state.

**Return**

`true` if the xas cannot be used for operations.

bool xas\_valid(const struct xa\_state \*xas)
:   Is the xas a valid cursor into the array?

**Parameters**

`const struct xa_state *xas`
:   XArray operation state.

**Return**

`true` if the xas can be used for operations.

bool xas\_is\_node(const struct xa\_state \*xas)
:   Does the xas point to a node?

**Parameters**

`const struct xa_state *xas`
:   XArray operation state.

**Return**

`true` if the xas currently references a node.

void xas\_reset(struct xa\_state \*xas)
:   Reset an XArray operation state.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

Resets the error or walk state of the **xas** so future walks of the
array will start from the root. Use this if you have dropped the
xarray lock and want to reuse the xa\_state.

**Context**

Any context.

bool xas\_retry(struct xa\_state \*xas, const void \*entry)
:   Retry the operation if appropriate.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`const void *entry`
:   Entry from xarray.

**Description**

The advanced functions may sometimes return an internal entry, such as
a retry entry or a zero entry. This function sets up the **xas** to restart
the walk from the head of the array if needed.

**Context**

Any context.

**Return**

true if the operation needs to be retried.

void \*xas\_reload(struct xa\_state \*xas)
:   Refetch an entry from the xarray.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

Use this function to check that a previously loaded entry still has
the same value. This is useful for the lockless pagecache lookup where
we walk the array with only the RCU lock to protect us, lock the page,
then check that the page hasn’t moved since we looked it up.

The caller guarantees that **xas** is still valid. If it may be in an
error or restart state, call [`xas_load()`](#c.xas_load "xas_load") instead.

**Return**

The entry at this location in the xarray.

void xas\_set(struct xa\_state \*xas, unsigned long index)
:   Set up XArray operation state for a different index.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`unsigned long index`
:   New index into the XArray.

**Description**

Move the operation state to refer to a different index. This will
have the effect of starting a walk from the top; see [`xas_next()`](#c.xas_next "xas_next")
to move to an adjacent index.

void xas\_advance(struct xa\_state \*xas, unsigned long index)
:   Skip over sibling entries.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`unsigned long index`
:   Index of last sibling entry.

**Description**

Move the operation state to refer to the last sibling entry.
This is useful for loops that normally want to see sibling
entries but sometimes want to skip them. Use [`xas_set()`](#c.xas_set "xas_set") if you
want to move to an index which is not part of this entry.

void xas\_set\_order(struct xa\_state \*xas, unsigned long index, unsigned int order)
:   Set up XArray operation state for a multislot entry.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`unsigned long index`
:   Target of the operation.

`unsigned int order`
:   Entry occupies 2^\*\*order\*\* indices.

void xas\_set\_update(struct xa\_state \*xas, [xa\_update\_node\_t](#c.xa_update_node_t "xa_update_node_t") update)
:   Set up XArray operation state for a callback.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`xa_update_node_t update`
:   Function to call when updating a node.

**Description**

The XArray can notify a caller after it has updated an xa\_node.
This is advanced functionality and is only needed by the page
cache and swap cache.

void \*xas\_next\_entry(struct xa\_state \*xas, unsigned long max)
:   Advance iterator to next present entry.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`unsigned long max`
:   Highest index to return.

**Description**

[`xas_next_entry()`](#c.xas_next_entry "xas_next_entry") is an inline function to optimise xarray traversal for
speed. It is equivalent to calling [`xas_find()`](#c.xas_find "xas_find"), and will call [`xas_find()`](#c.xas_find "xas_find")
for all the hard cases.

**Return**

The next present entry after the one currently referred to by **xas**.

void \*xas\_next\_marked(struct xa\_state \*xas, unsigned long max, xa\_mark\_t mark)
:   Advance iterator to next marked entry.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`unsigned long max`
:   Highest index to return.

`xa_mark_t mark`
:   Mark to search for.

**Description**

[`xas_next_marked()`](#c.xas_next_marked "xas_next_marked") is an inline function to optimise xarray traversal for
speed. It is equivalent to calling [`xas_find_marked()`](#c.xas_find_marked "xas_find_marked"), and will call
[`xas_find_marked()`](#c.xas_find_marked "xas_find_marked") for all the hard cases.

**Return**

The next marked entry after the one currently referred to by **xas**.

xas\_for\_each

`xas_for_each (xas, entry, max)`

> Iterate over a range of an XArray.

**Parameters**

`xas`
:   XArray operation state.

`entry`
:   Entry retrieved from the array.

`max`
:   Maximum index to retrieve from array.

**Description**

The loop body will be executed for each entry present in the xarray
between the current xas position and **max**. **entry** will be set to
the entry retrieved from the xarray. It is safe to delete entries
from the array in the loop body. You should hold either the RCU lock
or the xa\_lock while iterating. If you need to drop the lock, call
[`xas_pause()`](#c.xas_pause "xas_pause") first.

xas\_for\_each\_marked

`xas_for_each_marked (xas, entry, max, mark)`

> Iterate over a range of an XArray.

**Parameters**

`xas`
:   XArray operation state.

`entry`
:   Entry retrieved from the array.

`max`
:   Maximum index to retrieve from array.

`mark`
:   Mark to search for.

**Description**

The loop body will be executed for each marked entry in the xarray
between the current xas position and **max**. **entry** will be set to
the entry retrieved from the xarray. It is safe to delete entries
from the array in the loop body. You should hold either the RCU lock
or the xa\_lock while iterating. If you need to drop the lock, call
[`xas_pause()`](#c.xas_pause "xas_pause") first.

xas\_for\_each\_conflict

`xas_for_each_conflict (xas, entry)`

> Iterate over a range of an XArray.

**Parameters**

`xas`
:   XArray operation state.

`entry`
:   Entry retrieved from the array.

**Description**

The loop body will be executed for each entry in the XArray that
lies within the range specified by **xas**. If the loop terminates
normally, **entry** will be `NULL`. The user may break out of the loop,
which will leave **entry** set to the conflicting entry. The caller
may also call `xa_set_err()` to exit the loop while setting an error
to record the reason.

void \*xas\_prev(struct xa\_state \*xas)
:   Move iterator to previous index.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

If the **xas** was in an error state, it will remain in an error state
and this function will return `NULL`. If the **xas** has never been walked,
it will have the effect of calling [`xas_load()`](#c.xas_load "xas_load"). Otherwise one will be
subtracted from the index and the state will be walked to the correct
location in the array for the next operation.

If the iterator was referencing index 0, this function wraps
around to `ULONG_MAX`.

**Return**

The entry at the new index. This may be `NULL` or an internal
entry.

void \*xas\_next(struct xa\_state \*xas)
:   Move state to next index.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

If the **xas** was in an error state, it will remain in an error state
and this function will return `NULL`. If the **xas** has never been walked,
it will have the effect of calling [`xas_load()`](#c.xas_load "xas_load"). Otherwise one will be
added to the index and the state will be walked to the correct
location in the array for the next operation.

If the iterator was referencing index `ULONG_MAX`, this function wraps
around to 0.

**Return**

The entry at the new index. This may be `NULL` or an internal
entry.

void \*xas\_load(struct xa\_state \*xas)
:   Load an entry from the XArray (advanced).

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

Usually walks the **xas** to the appropriate state to load the entry
stored at xa\_index. However, it will do nothing and return `NULL` if
**xas** is in an error state. [`xas_load()`](#c.xas_load "xas_load") will never expand the tree.

If the xa\_state is set up to operate on a multi-index entry, [`xas_load()`](#c.xas_load "xas_load")
may return `NULL` or an internal entry, even if there are entries
present within the range specified by **xas**.

**Context**

Any context. The caller should hold the xa\_lock or the RCU lock.

**Return**

Usually an entry in the XArray, but see description for exceptions.

bool xas\_nomem(struct xa\_state \*xas, gfp\_t gfp)
:   Allocate memory if needed.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

If we need to add new nodes to the XArray, we try to allocate memory
with GFP\_NOWAIT while holding the lock, which will usually succeed.
If it fails, **xas** is flagged as needing memory to continue. The caller
should drop the lock and call [`xas_nomem()`](#c.xas_nomem "xas_nomem"). If [`xas_nomem()`](#c.xas_nomem "xas_nomem") succeeds,
the caller should retry the operation.

Forward progress is guaranteed as one node is allocated here and
stored in the xa\_state where it will be found by `xas_alloc()`. More
nodes will likely be found in the slab allocator, but we do not tie
them up here.

**Return**

true if memory was needed, and was successfully allocated.

void xas\_free\_nodes(struct xa\_state \*xas, struct xa\_node \*top)
:   Free this node and all nodes that it references

**Parameters**

`struct xa_state *xas`
:   Array operation state.

`struct xa_node *top`
:   Node to free

**Description**

This node has been removed from the tree. We must now free it and all
of its subnodes. There may be RCU walkers with references into the tree,
so we must replace all entries with retry markers.

void xas\_create\_range(struct xa\_state \*xas)
:   Ensure that stores to this range will succeed

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

Creates all of the slots in the range covered by **xas**. Sets **xas** to
create single-index entries and positions it at the beginning of the
range. This is for the benefit of users which have not yet been
converted to use multi-index entries.

void \*xas\_store(struct xa\_state \*xas, void \*entry)
:   Store this entry in the XArray.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`void *entry`
:   New entry.

**Description**

If **xas** is operating on a multi-index entry, the entry returned by this
function is essentially meaningless (it may be an internal entry or it
may be `NULL`, even if there are non-NULL entries at some of the indices
covered by the range). This is not a problem for any current users,
and can be changed if needed.

**Return**

The old entry at this index.

bool xas\_get\_mark(const struct xa\_state \*xas, xa\_mark\_t mark)
:   Returns the state of this mark.

**Parameters**

`const struct xa_state *xas`
:   XArray operation state.

`xa_mark_t mark`
:   Mark number.

**Return**

true if the mark is set, false if the mark is clear or **xas**
is in an error state.

void xas\_set\_mark(const struct xa\_state \*xas, xa\_mark\_t mark)
:   Sets the mark on this entry and its parents.

**Parameters**

`const struct xa_state *xas`
:   XArray operation state.

`xa_mark_t mark`
:   Mark number.

**Description**

Sets the specified mark on this entry, and walks up the tree setting it
on all the ancestor entries. Does nothing if **xas** has not been walked to
an entry, or is in an error state.

void xas\_clear\_mark(const struct xa\_state \*xas, xa\_mark\_t mark)
:   Clears the mark on this entry and its parents.

**Parameters**

`const struct xa_state *xas`
:   XArray operation state.

`xa_mark_t mark`
:   Mark number.

**Description**

Clears the specified mark on this entry, and walks back to the head
attempting to clear it on all the ancestor entries. Does nothing if
**xas** has not been walked to an entry, or is in an error state.

void xas\_init\_marks(const struct xa\_state \*xas)
:   Initialise all marks for the entry

**Parameters**

`const struct xa_state *xas`
:   Array operations state.

**Description**

Initialise all marks for the entry specified by **xas**. If we’re tracking
free entries with a mark, we need to set it on all entries. All other
marks are cleared.

This implementation is not as efficient as it could be; we may walk
up the tree multiple times.

void xas\_split\_alloc(struct xa\_state \*xas, void \*entry, unsigned int order, gfp\_t gfp)
:   Allocate memory for splitting an entry.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`void *entry`
:   New entry which will be stored in the array.

`unsigned int order`
:   Current entry order.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

This function should be called before calling [`xas_split()`](#c.xas_split "xas_split").
If necessary, it will allocate new nodes (and fill them with **entry**)
to prepare for the upcoming split of an entry of **order** size into
entries of the order stored in the **xas**.

**Context**

May sleep if **gfp** flags permit.

void xas\_split(struct xa\_state \*xas, void \*entry, unsigned int order)
:   Split a multi-index entry into smaller entries.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`void *entry`
:   New entry to store in the array.

`unsigned int order`
:   Current entry order.

**Description**

The size of the new entries is set in **xas**. The value in **entry** is
copied to all the replacement entries.

**Context**

Any context. The caller should hold the xa\_lock.

unsigned int xas\_try\_split\_min\_order(unsigned int order)
:   Minimal split order [`xas_try_split()`](#c.xas_try_split "xas_try_split") can accept

**Parameters**

`unsigned int order`
:   Current entry order.

**Description**

[`xas_try_split()`](#c.xas_try_split "xas_try_split") can split a multi-index entry to smaller than **order** - 1 if
no new xa\_node is needed. This function provides the minimal order
[`xas_try_split()`](#c.xas_try_split "xas_try_split") supports.

**Return**

the minimal order [`xas_try_split()`](#c.xas_try_split "xas_try_split") supports

**Context**

Any context.

void xas\_try\_split(struct xa\_state \*xas, void \*entry, unsigned int order)
:   Try to split a multi-index entry.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`void *entry`
:   New entry to store in the array.

`unsigned int order`
:   Current entry order.

**Description**

The size of the new entries is set in **xas**. The value in **entry** is
copied to all the replacement entries. If and only if one new xa\_node is
needed, the function will use GFP\_NOWAIT to get one if xas->xa\_alloc is
NULL. If more new xa\_node are needed, the function gives EINVAL error.

**NOTE**

use [`xas_try_split_min_order()`](#c.xas_try_split_min_order "xas_try_split_min_order") to get next split order instead of
**order** - 1 if you want to minmize [`xas_try_split()`](#c.xas_try_split "xas_try_split") calls.

**Context**

Any context. The caller should hold the xa\_lock.

void xas\_pause(struct xa\_state \*xas)
:   Pause a walk to drop a lock.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

Some users need to pause a walk and drop the lock they’re holding in
order to yield to a higher priority thread or carry out an operation
on an entry. Those users should call this function before they drop
the lock. It resets the **xas** to be suitable for the next iteration
of the loop after the user has reacquired the lock. If most entries
found during a walk require you to call [`xas_pause()`](#c.xas_pause "xas_pause"), the [`xa_for_each()`](#c.xa_for_each "xa_for_each")
iterator may be more appropriate.

Note that [`xas_pause()`](#c.xas_pause "xas_pause") only works for forward iteration. If a user needs
to pause a reverse iteration, we will need a `xas_pause_rev()`.

void \*xas\_find(struct xa\_state \*xas, unsigned long max)
:   Find the next present entry in the XArray.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`unsigned long max`
:   Highest index to return.

**Description**

If the **xas** has not yet been walked to an entry, return the entry
which has an index >= xas.xa\_index. If it has been walked, the entry
currently being pointed at has been processed, and so we move to the
next entry.

If no entry is found and the array is smaller than **max**, the iterator
is set to the smallest index not yet in the array. This allows **xas**
to be immediately passed to [`xas_store()`](#c.xas_store "xas_store").

**Return**

The entry, if found, otherwise `NULL`.

void \*xas\_find\_marked(struct xa\_state \*xas, unsigned long max, xa\_mark\_t mark)
:   Find the next marked entry in the XArray.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

`unsigned long max`
:   Highest index to return.

`xa_mark_t mark`
:   Mark number to search for.

**Description**

If the **xas** has not yet been walked to an entry, return the marked entry
which has an index >= xas.xa\_index. If it has been walked, the entry
currently being pointed at has been processed, and so we return the
first marked entry with an index > xas.xa\_index.

If no marked entry is found and the array is smaller than **max**, **xas** is
set to the bounds state and xas->xa\_index is set to the smallest index
not yet in the array. This allows **xas** to be immediately passed to
[`xas_store()`](#c.xas_store "xas_store").

If no entry is found before **max** is reached, **xas** is set to the restart
state.

**Return**

The entry, if found, otherwise `NULL`.

void \*xas\_find\_conflict(struct xa\_state \*xas)
:   Find the next present entry in a range.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

The **xas** describes both a range and a position within that range.

**Context**

Any context. Expects xa\_lock to be held.

**Return**

The next entry in the range covered by **xas** or `NULL`.

void \*xa\_load(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index)
:   Load an entry from an XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   index into array.

**Context**

Any context. Takes and releases the RCU lock.

**Return**

The entry at **index** in **xa**.

void \*\_\_xa\_erase(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index)
:   Erase this entry from the XArray while locked.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

**Description**

After this function returns, loading from **index** will return `NULL`.
If the index is part of a multi-index entry, all indices will be erased
and none of the entries will be part of a multi-index entry.

**Context**

Any context. Expects xa\_lock to be held on entry.

**Return**

The entry which used to be at this index.

void \*xa\_erase(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index)
:   Erase this entry from the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

**Description**

After this function returns, loading from **index** will return `NULL`.
If the index is part of a multi-index entry, all indices will be erased
and none of the entries will be part of a multi-index entry.

**Context**

Any context. Takes and releases the xa\_lock.

**Return**

The entry which used to be at this index.

void \*\_\_xa\_store(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

You must already be holding the xa\_lock when calling this function.
It will drop the lock if needed to allocate memory, and then reacquire
it afterwards.

**Context**

Any context. Expects xa\_lock to be held on entry. May
release and reacquire xa\_lock if **gfp** flags permit.

**Return**

The old entry at this index or [`xa_err()`](#c.xa_err "xa_err") if an error happened.

void \*xa\_store(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

After this function returns, loads from this index will return **entry**.
Storing into an existing multi-index entry updates the entry of every index.
The marks associated with **index** are unaffected unless **entry** is `NULL`.

**Context**

Any context. Takes and releases the xa\_lock.
May sleep if the **gfp** flags permit.

**Return**

The old entry at this index on success, xa\_err(-EINVAL) if **entry**
cannot be stored in an XArray, or xa\_err(-ENOMEM) if memory allocation
failed.

void \*\_\_xa\_cmpxchg(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*old, void \*entry, gfp\_t gfp)
:   Conditionally replace an entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *old`
:   Old value to test against.

`void *entry`
:   New value to place in array.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

You must already be holding the xa\_lock when calling this function.
It will drop the lock if needed to allocate memory, and then reacquire
it afterwards.

If the entry at **index** is the same as **old**, replace it with **entry**.
If the return value is equal to **old**, then the exchange was successful.

**Context**

Any context. Expects xa\_lock to be held on entry. May
release and reacquire xa\_lock if **gfp** flags permit.

**Return**

The old value at this index or [`xa_err()`](#c.xa_err "xa_err") if an error happened.

int \_\_xa\_insert(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, void \*entry, gfp\_t gfp)
:   Store this entry in the XArray if no entry is present.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index into array.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Inserting a NULL entry will store a reserved entry (like [`xa_reserve()`](#c.xa_reserve "xa_reserve"))
if no entry is present. Inserting will fail if a reserved entry is
present, even though loading from this index will return NULL.

**Context**

Any context. Expects xa\_lock to be held on entry. May
release and reacquire xa\_lock if **gfp** flags permit.

**Return**

0 if the store succeeded. -EBUSY if another entry was present.
-ENOMEM if memory could not be allocated.

void \*xa\_store\_range(struct [xarray](#c.xarray "xarray") \*xa, unsigned long first, unsigned long last, void \*entry, gfp\_t gfp)
:   Store this entry at a range of indices in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long first`
:   First index to affect.

`unsigned long last`
:   Last index to affect.

`void *entry`
:   New entry.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

After this function returns, loads from any index between **first** and **last**,
inclusive will return **entry**.
Storing into an existing multi-index entry updates the entry of every index.
The marks associated with **index** are unaffected unless **entry** is `NULL`.

**Context**

Process context. Takes and releases the xa\_lock. May sleep
if the **gfp** flags permit.

**Return**

`NULL` on success, xa\_err(-EINVAL) if **entry** cannot be stored in
an XArray, or xa\_err(-ENOMEM) if memory allocation failed.

int xas\_get\_order(struct xa\_state \*xas)
:   Get the order of an entry.

**Parameters**

`struct xa_state *xas`
:   XArray operation state.

**Description**

Called after xas\_load, the xas should not be in an error state.
The xas should not be pointing to a sibling entry.

**Return**

A number between 0 and 63 indicating the order of the entry.

int xa\_get\_order(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index)
:   Get the order of an entry.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of the entry.

**Return**

A number between 0 and 63 indicating the order of the entry.

int \_\_xa\_alloc(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range for allocated ID.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

**Context**

Any context. Expects xa\_lock to be held on entry. May
release and reacquire xa\_lock if **gfp** flags permit.

**Return**

0 on success, -ENOMEM if memory could not be allocated or
-EBUSY if there are no free entries in **limit**.

int \_\_xa\_alloc\_cyclic(struct [xarray](#c.xarray "xarray") \*xa, u32 \*id, void \*entry, struct [xa\_limit](#c.xa_limit "xa_limit") limit, u32 \*next, gfp\_t gfp)
:   Find somewhere to store this entry in the XArray.

**Parameters**

`struct xarray *xa`
:   XArray.

`u32 *id`
:   Pointer to ID.

`void *entry`
:   New entry.

`struct xa_limit limit`
:   Range of allocated ID.

`u32 *next`
:   Pointer to next ID to allocate.

`gfp_t gfp`
:   Memory allocation flags.

**Description**

Finds an empty entry in **xa** between **limit.min** and **limit.max**,
stores the index into the **id** pointer, then stores the entry at
that index. A concurrent lookup will not see an uninitialised **id**.
The search for an empty entry will start at **next** and will wrap
around if necessary.

Must only be operated on an xarray initialized with flag XA\_FLAGS\_ALLOC set
in [`xa_init_flags()`](#c.xa_init_flags "xa_init_flags").

**Context**

Any context. Expects xa\_lock to be held on entry. May
release and reacquire xa\_lock if **gfp** flags permit.

**Return**

0 if the allocation succeeded without wrapping. 1 if the
allocation succeeded after wrapping, -ENOMEM if memory could not be
allocated or -EBUSY if there are no free entries in **limit**.

void \_\_xa\_set\_mark(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, xa\_mark\_t mark)
:   Set this mark on this entry while locked.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

`xa_mark_t mark`
:   Mark number.

**Description**

Attempting to set a mark on a `NULL` entry does not succeed.

**Context**

Any context. Expects xa\_lock to be held on entry.

void \_\_xa\_clear\_mark(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, xa\_mark\_t mark)
:   Clear this mark on this entry while locked.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

`xa_mark_t mark`
:   Mark number.

**Context**

Any context. Expects xa\_lock to be held on entry.

bool xa\_get\_mark(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, xa\_mark\_t mark)
:   Inquire whether this mark is set on this entry.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

`xa_mark_t mark`
:   Mark number.

**Description**

This function uses the RCU read lock, so the result may be out of date
by the time it returns. If you need the result to be stable, use a lock.

**Context**

Any context. Takes and releases the RCU lock.

**Return**

True if the entry at **index** has this mark set, false if it doesn’t.

void xa\_set\_mark(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, xa\_mark\_t mark)
:   Set this mark on this entry.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

`xa_mark_t mark`
:   Mark number.

**Description**

Attempting to set a mark on a `NULL` entry does not succeed.

**Context**

Process context. Takes and releases the xa\_lock.

void xa\_clear\_mark(struct [xarray](#c.xarray "xarray") \*xa, unsigned long index, xa\_mark\_t mark)
:   Clear this mark on this entry.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long index`
:   Index of entry.

`xa_mark_t mark`
:   Mark number.

**Description**

Clearing a mark always succeeds.

**Context**

Process context. Takes and releases the xa\_lock.

void \*xa\_find(struct [xarray](#c.xarray "xarray") \*xa, unsigned long \*indexp, unsigned long max, xa\_mark\_t filter)
:   Search the XArray for an entry.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long *indexp`
:   Pointer to an index.

`unsigned long max`
:   Maximum index to search to.

`xa_mark_t filter`
:   Selection criterion.

**Description**

Finds the entry in **xa** which matches the **filter**, and has the lowest
index that is at least **indexp** and no more than **max**.
If an entry is found, **indexp** is updated to be the index of the entry.
This function is protected by the RCU read lock, so it may not find
entries which are being simultaneously added. It will not return an
`XA_RETRY_ENTRY`; if you need to see retry entries, use [`xas_find()`](#c.xas_find "xas_find").

**Context**

Any context. Takes and releases the RCU lock.

**Return**

The entry, if found, otherwise `NULL`.

void \*xa\_find\_after(struct [xarray](#c.xarray "xarray") \*xa, unsigned long \*indexp, unsigned long max, xa\_mark\_t filter)
:   Search the XArray for a present entry.

**Parameters**

`struct xarray *xa`
:   XArray.

`unsigned long *indexp`
:   Pointer to an index.

`unsigned long max`
:   Maximum index to search to.

`xa_mark_t filter`
:   Selection criterion.

**Description**

Finds the entry in **xa** which matches the **filter** and has the lowest
index that is above **indexp** and no more than **max**.
If an entry is found, **indexp** is updated to be the index of the entry.
This function is protected by the RCU read lock, so it may miss entries
which are being simultaneously added. It will not return an
`XA_RETRY_ENTRY`; if you need to see retry entries, use [`xas_find()`](#c.xas_find "xas_find").

**Context**

Any context. Takes and releases the RCU lock.

**Return**

The pointer, if found, otherwise `NULL`.

unsigned int xa\_extract(struct [xarray](#c.xarray "xarray") \*xa, void \*\*dst, unsigned long start, unsigned long max, unsigned int n, xa\_mark\_t filter)
:   Copy selected entries from the XArray into a normal array.

**Parameters**

`struct xarray *xa`
:   The source XArray to copy from.

`void **dst`
:   The buffer to copy entries into.

`unsigned long start`
:   The first index in the XArray eligible to be selected.

`unsigned long max`
:   The last index in the XArray eligible to be selected.

`unsigned int n`
:   The maximum number of entries to copy.

`xa_mark_t filter`
:   Selection criterion.

**Description**

Copies up to **n** entries that match **filter** from the XArray. The
copied entries will have indices between **start** and **max**, inclusive.

The **filter** may be an XArray mark value, in which case entries which are
marked with that mark will be copied. It may also be `XA_PRESENT`, in
which case all entries which are not `NULL` will be copied.

The entries returned may not represent a snapshot of the XArray at a
moment in time. For example, if another thread stores to index 5, then
index 10, calling [`xa_extract()`](#c.xa_extract "xa_extract") may return the old contents of index 5
and the new contents of index 10. Indices not modified while this
function is running will not be skipped.

If you need stronger guarantees, holding the xa\_lock across calls to this
function will prevent concurrent modification.

**Context**

Any context. Takes and releases the RCU lock.

**Return**

The number of entries copied.

void xa\_delete\_node(struct xa\_node \*node, [xa\_update\_node\_t](#c.xa_update_node_t "xa_update_node_t") update)
:   Private interface for workingset code.

**Parameters**

`struct xa_node *node`
:   Node to be removed from the tree.

`xa_update_node_t update`
:   Function to call to update ancestor nodes.

**Context**

xa\_lock must be held on entry and will not be released.

void xa\_destroy(struct [xarray](#c.xarray "xarray") \*xa)
:   Free all internal data structures.

**Parameters**

`struct xarray *xa`
:   XArray.

**Description**

After calling this function, the XArray is empty and has freed all memory
allocated for its internal data structures. You are responsible for
freeing the objects referenced by the XArray.

**Context**

Any context. Takes and releases the xa\_lock, interrupt-safe.
