# Network Filesystem Caching API

> 출처(원문): https://docs.kernel.org/filesystems/caching/netfs-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Network Filesystem Caching API

Fscache provides an API by which a network filesystem can make use of local
caching facilities. The API is arranged around a number of principles:

> 1. A cache is logically organised into volumes and data storage objects
>    within those volumes.
> 2. Volumes and data storage objects are represented by various types of
>    cookie.
> 3. Cookies have keys that distinguish them from their peers.
> 4. Cookies have coherency data that allows a cache to determine if the
>    cached data is still valid.
> 5. I/O is done asynchronously where possible.

This API is used by:

```
#include <linux/fscache.h>.
```

## Overview

The fscache hierarchy is organised on two levels from a network filesystem’s
point of view. The upper level represents “volumes” and the lower level
represents “data storage objects”. These are represented by two types of
cookie, hereafter referred to as “volume cookies” and “cookies”.

A network filesystem acquires a volume cookie for a volume using a volume key,
which represents all the information that defines that volume (e.g. cell name
or server address, volume ID or share name). This must be rendered as a
printable string that can be used as a directory name (ie. no ‘/’ characters
and shouldn’t begin with a ‘.’). The maximum name length is one less than the
maximum size of a filename component (allowing the cache backend one char for
its own purposes).

A filesystem would typically have a volume cookie for each superblock.

The filesystem then acquires a cookie for each file within that volume using an
object key. Object keys are binary blobs and only need to be unique within
their parent volume. The cache backend is responsible for rendering the binary
blob into something it can use and may employ hash tables, trees or whatever to
improve its ability to find an object. This is transparent to the network
filesystem.

A filesystem would typically have a cookie for each inode, and would acquire it
in iget and relinquish it when evicting the cookie.

Once it has a cookie, the filesystem needs to mark the cookie as being in use.
This causes fscache to send the cache backend off to look up/create resources
for the cookie in the background, to check its coherency and, if necessary, to
mark the object as being under modification.

A filesystem would typically “use” the cookie in its file open routine and
unuse it in file release and it needs to use the cookie around calls to
truncate the cookie locally. It *also* needs to use the cookie when the
pagecache becomes dirty and unuse it when writeback is complete. This is
slightly tricky, and provision is made for it.

When performing a read, write or resize on a cookie, the filesystem must first
begin an operation. This copies the resources into a holding `struct and` puts
extra pins into the cache to stop cache withdrawal from tearing down the
structures being used. The actual operation can then be issued and conflicting
invalidations can be detected upon completion.

The filesystem is expected to use netfslib to access the cache, but that’s not
actually required and it can use the fscache I/O API directly.

## Volume Registration

The first step for a network filesystem is to acquire a volume cookie for the
volume it wants to access:

```
struct fscache_volume *
fscache_acquire_volume(const char *volume_key,
                       const char *cache_name,
                       const void *coherency_data,
                       size_t coherency_len);
```

This function creates a volume cookie with the specified volume key as its name
and notes the coherency data.

The volume key must be a printable string with no ‘/’ characters in it. It
should begin with the name of the filesystem and should be no longer than 254
characters. It should uniquely represent the volume and will be matched with
what’s stored in the cache.

The caller may also specify the name of the cache to use. If specified,
fscache will look up or create a cache cookie of that name and will use a cache
of that name if it is online or comes online. If no cache name is specified,
it will use the first cache that comes to hand and set the name to that.

The specified coherency data is stored in the cookie and will be matched
against coherency data stored on disk. The data pointer may be NULL if no data
is provided. If the coherency data doesn’t match, the entire cache volume will
be invalidated.

This function can return errors such as EBUSY if the volume key is already in
use by an acquired volume or ENOMEM if an allocation failure occurred. It may
also return a NULL volume cookie if fscache is not enabled. It is safe to
pass a NULL cookie to any function that takes a volume cookie. This will
cause that function to do nothing.

When the network filesystem has finished with a volume, it should relinquish it
by calling:

```
void fscache_relinquish_volume(struct fscache_volume *volume,
                               const void *coherency_data,
                               bool invalidate);
```

This will cause the volume to be committed or removed, and if sealed the
coherency data will be set to the value supplied. The amount of coherency data
must match the length specified when the volume was acquired. Note that all
data cookies obtained in this volume must be relinquished before the volume is
relinquished.

## Data File Registration

Once it has a volume cookie, a network filesystem can use it to acquire a
cookie for data storage:

```
struct fscache_cookie *
fscache_acquire_cookie(struct fscache_volume *volume,
                       u8 advice,
                       const void *index_key,
                       size_t index_key_len,
                       const void *aux_data,
                       size_t aux_data_len,
                       loff_t object_size)
```

This creates the cookie in the volume using the specified index key. The index
key is a binary blob of the given length and must be unique for the volume.
This is saved into the cookie. There are no restrictions on the content, but
its length shouldn’t exceed about three quarters of the maximum filename length
to allow for encoding.

The caller should also pass in a piece of coherency data in aux\_data. A buffer
of size aux\_data\_len will be allocated and the coherency data copied in. It is
assumed that the size is invariant over time. The coherency data is used to
check the validity of data in the cache. Functions are provided by which the
coherency data can be updated.

The file size of the object being cached should also be provided. This may be
used to trim the data and will be stored with the coherency data.

This function never returns an error, though it may return a NULL cookie on
allocation failure or if fscache is not enabled. It is safe to pass in a NULL
volume cookie and pass the NULL cookie returned to any function that takes it.
This will cause that function to do nothing.

When the network filesystem has finished with a cookie, it should relinquish it
by calling:

```
void fscache_relinquish_cookie(struct fscache_cookie *cookie,
                               bool retire);
```

This will cause fscache to either commit the storage backing the cookie or
delete it.

## Marking A Cookie In-Use

Once a cookie has been acquired by a network filesystem, the filesystem should
tell fscache when it intends to use the cookie (typically done on file open)
and should say when it has finished with it (typically on file close):

```
void fscache_use_cookie(struct fscache_cookie *cookie,
                        bool will_modify);
void fscache_unuse_cookie(struct fscache_cookie *cookie,
                          const void *aux_data,
                          const loff_t *object_size);
```

The *use* function tells fscache that it will use the cookie and, additionally,
indicate if the user is intending to modify the contents locally. If not yet
done, this will trigger the cache backend to go and gather the resources it
needs to access/store data in the cache. This is done in the background, and
so may not be complete by the time the function returns.

The *unuse* function indicates that a filesystem has finished using a cookie.
It optionally updates the stored coherency data and object size and then
decreases the in-use counter. When the last user unuses the cookie, it is
scheduled for garbage collection. If not reused within a short time, the
resources will be released to reduce system resource consumption.

A cookie must be marked in-use before it can be accessed for read, write or
resize - and an in-use mark must be kept whilst there is dirty data in the
pagecache in order to avoid an oops due to trying to open a file during process
exit.

Note that in-use marks are cumulative. For each time a cookie is marked
in-use, it must be unused.

## Resizing A Data File (Truncation)

If a network filesystem file is resized locally by truncation, the following
should be called to notify the cache:

```
void fscache_resize_cookie(struct fscache_cookie *cookie,
                           loff_t new_size);
```

The caller must have first marked the cookie in-use. The cookie and the new
size are passed in and the cache is synchronously resized. This is expected to
be called from `->setattr()` inode operation under the inode lock.

## Data I/O API

To do data I/O operations directly through a cookie, the following functions
are available:

```
int fscache_begin_read_operation(struct netfs_cache_resources *cres,
                                 struct fscache_cookie *cookie);
int fscache_read(struct netfs_cache_resources *cres,
                 loff_t start_pos,
                 struct iov_iter *iter,
                 enum netfs_read_from_hole read_hole,
                 netfs_io_terminated_t term_func,
                 void *term_func_priv);
int fscache_write(struct netfs_cache_resources *cres,
                  loff_t start_pos,
                  struct iov_iter *iter,
                  netfs_io_terminated_t term_func,
                  void *term_func_priv);
```

The *begin* function sets up an operation, attaching the resources required to
the cache resources block from the cookie. Assuming it doesn’t return an error
(for instance, it will return -ENOBUFS if given a NULL cookie, but otherwise do
nothing), then one of the other two functions can be issued.

The *read* and *write* functions initiate a direct-IO operation. Both take the
previously set up cache resources block, an indication of the start file
position, and an I/O iterator that describes buffer and indicates the amount of
data.

The read function also takes a parameter to indicate how it should handle a
partially populated region (a hole) in the disk content. This may be to ignore
it, skip over an initial hole and place zeros in the buffer or give an error.

The read and write functions can be given an optional termination function that
will be run on completion:

```
typedef
void (*netfs_io_terminated_t)(void *priv, ssize_t transferred_or_error,
                              bool was_async);
```

If a termination function is given, the operation will be run asynchronously
and the termination function will be called upon completion. If not given, the
operation will be run synchronously. Note that in the asynchronous case, it is
possible for the operation to complete before the function returns.

Both the read and write functions end the operation when they complete,
detaching any pinned resources.

The read operation will fail with ESTALE if invalidation occurred whilst the
operation was ongoing.

## Data File Coherency

To request an update of the coherency data and file size on a cookie, the
following should be called:

```
void fscache_update_cookie(struct fscache_cookie *cookie,
                           const void *aux_data,
                           const loff_t *object_size);
```

This will update the cookie’s coherency data and/or file size.

## Data File Invalidation

Sometimes it will be necessary to invalidate an object that contains data.
Typically this will be necessary when the server informs the network filesystem
of a remote third-party change - at which point the filesystem has to throw
away the state and cached data that it had for an file and reload from the
server.

To indicate that a cache object should be invalidated, the following should be
called:

```
void fscache_invalidate(struct fscache_cookie *cookie,
                        const void *aux_data,
                        loff_t size,
                        unsigned int flags);
```

This increases the invalidation counter in the cookie to cause outstanding
reads to fail with -ESTALE, sets the coherency data and file size from the
information supplied, blocks new I/O on the cookie and dispatches the cache to
go and get rid of the old data.

Invalidation runs asynchronously in a worker thread so that it doesn’t block
too much.

## Write-Back Resource Management

To write data to the cache from network filesystem writeback, the cache
resources required need to be pinned at the point the modification is made (for
instance when the page is marked dirty) as it’s not possible to open a file in
a thread that’s exiting.

The following facilities are provided to manage this:

> * An inode flag, `I_PINNING_FSCACHE_WB`, is provided to indicate that an
>   in-use is held on the cookie for this inode. It can only be changed if the
>   the inode lock is held.
> * A flag, `unpinned_fscache_wb` is placed in the `writeback_control`
>   struct that gets set if `__writeback_single_inode()` clears
>   `I_PINNING_FSCACHE_WB` because all the dirty pages were cleared.

To support this, the following functions are provided:

```
bool fscache_dirty_folio(struct address_space *mapping,
                         struct folio *folio,
                         struct fscache_cookie *cookie);
void fscache_unpin_writeback(struct writeback_control *wbc,
                             struct fscache_cookie *cookie);
void fscache_clear_inode_writeback(struct fscache_cookie *cookie,
                                   struct inode *inode,
                                   const void *aux);
```

The *set* function is intended to be called from the filesystem’s
`dirty_folio` address space operation. If `I_PINNING_FSCACHE_WB` is not
set, it sets that flag and increments the use count on the cookie (the caller
must already have called `fscache_use_cookie()`).

The *unpin* function is intended to be called from the filesystem’s
`write_inode` superblock operation. It cleans up after writing by unusing
the cookie if unpinned\_fscache\_wb is set in the writeback\_control struct.

The *clear* function is intended to be called from the netfs’s `evict_inode`
superblock operation. It must be called *after*
`truncate_inode_pages_final()`, but *before* `clear_inode()`. This cleans
up any hanging `I_PINNING_FSCACHE_WB`. It also allows the coherency data to
be updated.

## Caching of Local Modifications

If a network filesystem has locally modified data that it wants to write to the
cache, it needs to mark the pages to indicate that a write is in progress, and
if the mark is already present, it needs to wait for it to be removed first
(presumably due to an already in-progress operation). This prevents multiple
competing DIO writes to the same storage in the cache.

Firstly, the netfs should determine if caching is available by doing something
like:

```
bool caching = fscache_cookie_enabled(cookie);
```

If caching is to be attempted, pages should be waited for and then marked using
the following functions provided by the netfs helper library:

```
void set_page_fscache(struct page *page);
void wait_on_page_fscache(struct page *page);
int wait_on_page_fscache_killable(struct page *page);
```

Once all the pages in the span are marked, the netfs can ask fscache to
schedule a write of that region:

```
void fscache_write_to_cache(struct fscache_cookie *cookie,
                            struct address_space *mapping,
                            loff_t start, size_t len, loff_t i_size,
                            netfs_io_terminated_t term_func,
                            void *term_func_priv,
                            bool caching)
```

And if an error occurs before that point is reached, the marks can be removed
by calling:

```
void fscache_clear_page_bits(struct address_space *mapping,
                             loff_t start, size_t len,
                             bool caching)
```

In these functions, a pointer to the mapping to which the source pages are
attached is passed in and start and len indicate the size of the region that’s
going to be written (it doesn’t have to align to page boundaries necessarily,
but it does have to align to DIO boundaries on the backing filesystem). The
caching parameter indicates if caching should be skipped, and if false, the
functions do nothing.

The write function takes some additional parameters: the cookie representing
the cache object to be written to, i\_size indicates the size of the netfs file
and term\_func indicates an optional completion function, to which
term\_func\_priv will be passed, along with the error or amount written.

Note that the write function will always run asynchronously and will unmark all
the pages upon completion before calling term\_func.

## Page Release and Invalidation

Fscache keeps track of whether we have any data in the cache yet for a cache
object we’ve just created. It knows it doesn’t have to do any reading until it
has done a write and then the page it wrote from has been released by the VM,
after which it *has* to look in the cache.

To inform fscache that a page might now be in the cache, the following function
should be called from the `release_folio` address space op:

```
void fscache_note_page_release(struct fscache_cookie *cookie);
```

if the page has been released (ie. release\_folio returned true).

Page release and page invalidation should also wait for any mark left on the
page to say that a DIO write is underway from that page:

```
void wait_on_page_fscache(struct page *page);
int wait_on_page_fscache_killable(struct page *page);
```

## API Function Reference

struct fscache\_volume \*fscache\_acquire\_volume(const char \*volume\_key, const char \*cache\_name, const void \*coherency\_data, size\_t coherency\_len)
:   Register a volume as desiring caching services

**Parameters**

`const char *volume_key`
:   An identification string for the volume

`const char *cache_name`
:   The name of the cache to use (or NULL for the default)

`const void *coherency_data`
:   Piece of arbitrary coherency data to check (or NULL)

`size_t coherency_len`
:   The size of the coherency data

**Description**

Register a volume as desiring caching services if they’re available. The
caller must provide an identifier for the volume and may also indicate which
cache it should be in. If a preexisting volume entry is found in the cache,
the coherency data must match otherwise the entry will be invalidated.

Returns a cookie pointer on success, -ENOMEM if out of memory or -EBUSY if a
cache volume of that name is already acquired. Note that “NULL” is a valid
cookie pointer and can be returned if caching is refused.

void fscache\_relinquish\_volume(struct fscache\_volume \*volume, const void \*coherency\_data, bool invalidate)
:   Cease caching a volume

**Parameters**

`struct fscache_volume *volume`
:   The volume cookie

`const void *coherency_data`
:   Piece of arbitrary coherency data to set (or NULL)

`bool invalidate`
:   True if the volume should be invalidated

**Description**

Indicate that a filesystem no longer desires caching services for a volume.
The caller must have relinquished all file cookies prior to calling this.
The stored coherency data is updated.

struct fscache\_cookie \*fscache\_acquire\_cookie(struct fscache\_volume \*volume, u8 advice, const void \*index\_key, size\_t index\_key\_len, const void \*aux\_data, size\_t aux\_data\_len, loff\_t object\_size)
:   Acquire a cookie to represent a cache object

**Parameters**

`struct fscache_volume *volume`
:   The volume in which to locate/create this cookie

`u8 advice`
:   Advice flags (FSCACHE\_COOKIE\_ADV\_\*)

`const void *index_key`
:   The index key for this cookie

`size_t index_key_len`
:   Size of the index key

`const void *aux_data`
:   The auxiliary data for the cookie (may be NULL)

`size_t aux_data_len`
:   Size of the auxiliary data buffer

`loff_t object_size`
:   The initial size of object

**Description**

Acquire a cookie to represent a data file within the given cache volume.

See [Network Filesystem Caching API](#) for a complete
description.

void fscache\_use\_cookie(struct fscache\_cookie \*cookie, bool will\_modify)
:   Request usage of cookie attached to an object

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

`bool will_modify`
:   If cache is expected to be modified locally

**Description**

Request usage of the cookie attached to an object. The caller should tell
the cache if the object’s contents are about to be modified locally and then
the cache can apply the policy that has been set to handle this case.

void fscache\_unuse\_cookie(struct fscache\_cookie \*cookie, const void \*aux\_data, const loff\_t \*object\_size)
:   Cease usage of cookie attached to an object

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

`const void *aux_data`
:   Updated auxiliary data (or NULL)

`const loff_t *object_size`
:   Revised size of the object (or NULL)

**Description**

Cease usage of the cookie attached to an object. When the users count
reaches zero then the cookie relinquishment will be permitted to proceed.

void fscache\_relinquish\_cookie(struct fscache\_cookie \*cookie, bool retire)
:   Return the cookie to the cache, maybe discarding it

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie being returned

`bool retire`
:   True if the cache object the cookie represents is to be discarded

**Description**

This function returns a cookie to the cache, forcibly discarding the
associated cache object if retire is set to true.

See [Network Filesystem Caching API](#) for a complete
description.

void fscache\_update\_cookie(struct fscache\_cookie \*cookie, const void \*aux\_data, const loff\_t \*object\_size)
:   Request that a cache object be updated

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

`const void *aux_data`
:   The updated auxiliary data for the cookie (may be NULL)

`const loff_t *object_size`
:   The current size of the object (may be NULL)

**Description**

Request an update of the index data for the cache object associated with the
cookie. The auxiliary data on the cookie will be updated first if **aux\_data**
is set and the object size will be updated and the object possibly trimmed
if **object\_size** is set.

See [Network Filesystem Caching API](#) for a complete
description.

void fscache\_resize\_cookie(struct fscache\_cookie \*cookie, loff\_t new\_size)
:   Request that a cache object be resized

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

`loff_t new_size`
:   The new size of the object (may be NULL)

**Description**

Request that the size of an object be changed.

See [Network Filesystem Caching API](#) for a complete
description.

void fscache\_invalidate(struct fscache\_cookie \*cookie, const void \*aux\_data, loff\_t size, unsigned int flags)
:   Notify cache that an object needs invalidation

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

`const void *aux_data`
:   The updated auxiliary data for the cookie (may be NULL)

`loff_t size`
:   The revised size of the object.

`unsigned int flags`
:   Invalidation flags (FSCACHE\_INVAL\_\*)

**Description**

Notify the cache that an object is needs to be invalidated and that it
should abort any retrievals or stores it is doing on the cache. This
increments inval\_counter on the cookie which can be used by the caller to
reconsider I/O requests as they complete.

If **flags** has FSCACHE\_INVAL\_DIO\_WRITE set, this indicates that this is due
to a direct I/O write and will cause caching to be disabled on this cookie
until it is completely unused.

See [Network Filesystem Caching API](#) for a complete
description.

const struct netfs\_cache\_ops \*fscache\_operation\_valid(const struct netfs\_cache\_resources \*cres)
:   Return true if operations resources are usable

**Parameters**

`const struct netfs_cache_resources *cres`
:   The resources to check.

**Description**

Returns a pointer to the operations table if usable or NULL if not.

int fscache\_begin\_read\_operation(struct netfs\_cache\_resources \*cres, struct fscache\_cookie \*cookie)
:   Begin a read operation for the netfs lib

**Parameters**

`struct netfs_cache_resources *cres`
:   The cache resources for the read being performed

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

**Description**

Begin a read operation on behalf of the netfs helper library. **cres**
indicates the cache resources to which the operation state should be
attached; **cookie** indicates the cache object that will be accessed.

**cres->inval\_counter** is set from **cookie->inval\_counter** for comparison at
the end of the operation. This allows invalidation during the operation to
be detected by the caller.

**Return**

* 0 - Success
* `-ENOBUFS`
  :   + No caching available
* Other error code from the cache, such as -ENOMEM.

void fscache\_end\_operation(struct netfs\_cache\_resources \*cres)
:   End the read operation for the netfs lib

**Parameters**

`struct netfs_cache_resources *cres`
:   The cache resources for the read operation

**Description**

Clean up the resources at the end of the read request.

int fscache\_read(struct netfs\_cache\_resources \*cres, loff\_t start\_pos, struct iov\_iter \*iter, enum netfs\_read\_from\_hole read\_hole, netfs\_io\_terminated\_t term\_func, void \*term\_func\_priv)
:   Start a read from the cache.

**Parameters**

`struct netfs_cache_resources *cres`
:   The cache resources to use

`loff_t start_pos`
:   The beginning file offset in the cache file

`struct iov_iter *iter`
:   The buffer to fill - and also the length

`enum netfs_read_from_hole read_hole`
:   How to handle a hole in the data.

`netfs_io_terminated_t term_func`
:   The function to call upon completion

`void *term_func_priv`
:   The private data for **term\_func**

**Description**

Start a read from the cache. **cres** indicates the cache object to read from
and must be obtained by a call to `fscache_begin_operation()` beforehand.

The data is read into the iterator, **iter**, and that also indicates the size
of the operation. **start\_pos** is the start position in the file, though if
**seek\_data** is set appropriately, the cache can use SEEK\_DATA to find the
next piece of data, writing zeros for the hole into the iterator.

Upon termination of the operation, **term\_func** will be called and supplied
with **term\_func\_priv** plus the amount of data written, if successful, or the
error code otherwise.

**read\_hole** indicates how a partially populated region in the cache should be
handled. It can be one of a number of settings:

> NETFS\_READ\_HOLE\_IGNORE - Just try to read (may return a short read).
>
> NETFS\_READ\_HOLE\_FAIL - Give ENODATA if we encounter a hole.

int fscache\_begin\_write\_operation(struct netfs\_cache\_resources \*cres, struct fscache\_cookie \*cookie)
:   Begin a write operation for the netfs lib

**Parameters**

`struct netfs_cache_resources *cres`
:   The cache resources for the write being performed

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

**Description**

Begin a write operation on behalf of the netfs helper library. **cres**
indicates the cache resources to which the operation state should be
attached; **cookie** indicates the cache object that will be accessed.

**cres->inval\_counter** is set from **cookie->inval\_counter** for comparison at
the end of the operation. This allows invalidation during the operation to
be detected by the caller.

**Return**

* 0 - Success
* `-ENOBUFS`
  :   + No caching available
* Other error code from the cache, such as -ENOMEM.

int fscache\_write(struct netfs\_cache\_resources \*cres, loff\_t start\_pos, struct iov\_iter \*iter, netfs\_io\_terminated\_t term\_func, void \*term\_func\_priv)
:   Start a write to the cache.

**Parameters**

`struct netfs_cache_resources *cres`
:   The cache resources to use

`loff_t start_pos`
:   The beginning file offset in the cache file

`struct iov_iter *iter`
:   The data to write - and also the length

`netfs_io_terminated_t term_func`
:   The function to call upon completion

`void *term_func_priv`
:   The private data for **term\_func**

**Description**

Start a write to the cache. **cres** indicates the cache object to write to and
must be obtained by a call to `fscache_begin_operation()` beforehand.

The data to be written is obtained from the iterator, **iter**, and that also
indicates the size of the operation. **start\_pos** is the start position in
the file.

Upon termination of the operation, **term\_func** will be called and supplied
with **term\_func\_priv** plus the amount of data written, if successful, or the
error code otherwise.

void fscache\_clear\_page\_bits(struct [address\_space](../api-summary.html#c.address_space "address_space") \*mapping, loff\_t start, size\_t len, bool caching)
:   Clear the PG\_fscache bits from a set of pages

**Parameters**

`struct address_space *mapping`
:   The netfs inode to use as the source

`loff_t start`
:   The start position in **mapping**

`size_t len`
:   The amount of data to unlock

`bool caching`
:   If PG\_fscache has been set

**Description**

Clear the PG\_fscache flag from a sequence of pages and wake up anyone who’s
waiting.

void fscache\_write\_to\_cache(struct fscache\_cookie \*cookie, struct [address\_space](../api-summary.html#c.address_space "address_space") \*mapping, loff\_t start, size\_t len, loff\_t i\_size, netfs\_io\_terminated\_t term\_func, void \*term\_func\_priv, bool using\_pgpriv2, bool caching)
:   Save a write to the cache and clear PG\_fscache

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie representing the cache object

`struct address_space *mapping`
:   The netfs inode to use as the source

`loff_t start`
:   The start position in **mapping**

`size_t len`
:   The amount of data to write back

`loff_t i_size`
:   The new size of the inode

`netfs_io_terminated_t term_func`
:   The function to call upon completion

`void *term_func_priv`
:   The private data for **term\_func**

`bool using_pgpriv2`
:   If we’re using PG\_private\_2 to mark in-progress write

`bool caching`
:   If we actually want to do the caching

**Description**

Helper function for a netfs to write dirty data from an inode into the cache
object that’s backing it.

**start** and **len** describe the range of the data. This does not need to be
page-aligned, but to satisfy DIO requirements, the cache may expand it up to
the page boundaries on either end. All the pages covering the range must be
marked with PG\_fscache.

If given, **term\_func** will be called upon completion and supplied with
**term\_func\_priv**. Note that if **using\_pgpriv2** is set, the PG\_private\_2 flags
will have been cleared by this point, so the netfs must retain its own pin
on the mapping.

void fscache\_note\_page\_release(struct fscache\_cookie \*cookie)
:   Note that a netfs page got released

**Parameters**

`struct fscache_cookie *cookie`
:   The cookie corresponding to the file

**Description**

Note that a page that has been copied to the cache has been released. This
means that future reads will need to look in the cache to see if it’s there.
