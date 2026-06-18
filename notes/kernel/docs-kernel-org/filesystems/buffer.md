# Buffer Heads

> 출처(원문): https://docs.kernel.org/filesystems/buffer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Buffer Heads

Linux uses buffer heads to maintain state about individual filesystem blocks.
Buffer heads are deprecated and new filesystems should use iomap instead.

## Functions

void brelse(struct buffer\_head \*bh)
:   Release a buffer.

**Parameters**

`struct buffer_head *bh`
:   The buffer to release.

**Description**

Decrement a buffer\_head’s reference count. If **bh** is NULL, this
function is a no-op.

If all buffers on a folio have zero reference count, are clean
and unlocked, and if the folio is unlocked and not under writeback
then [`try_to_free_buffers()`](#c.try_to_free_buffers "try_to_free_buffers") may strip the buffers from the folio in
preparation for freeing it (sometimes, rarely, buffers are removed
from a folio but it ends up not being freed, and buffers may later
be reattached).

**Context**

Any context.

void bforget(struct buffer\_head \*bh)
:   Discard any dirty data in a buffer.

**Parameters**

`struct buffer_head *bh`
:   The buffer to forget.

**Description**

Call this function instead of [`brelse()`](#c.brelse "brelse") if the data written to a buffer
no longer needs to be written back. It will clear the buffer’s dirty
flag so writeback of this buffer will be skipped.

**Context**

Any context.

struct buffer\_head \*\_\_bread(struct block\_device \*bdev, sector\_t block, unsigned size)
:   Read a block.

**Parameters**

`struct block_device *bdev`
:   The block device to read from.

`sector_t block`
:   Block number in units of block size.

`unsigned size`
:   The block size of this device in bytes.

**Description**

Read a specified block, and return the buffer head that refers
to it. The memory is allocated from the movable area so that it can
be migrated. The returned buffer head has its refcount increased.
The caller should call [`brelse()`](#c.brelse "brelse") when it has finished with the buffer.

**Context**

May sleep waiting for I/O.

**Return**

NULL if the block was unreadable.

struct buffer\_head \*get\_nth\_bh(struct buffer\_head \*bh, unsigned int count)
:   Get a reference on the n’th buffer after this one.

**Parameters**

`struct buffer_head *bh`
:   The buffer to start counting from.

`unsigned int count`
:   How many buffers to skip.

**Description**

This is primarily useful for finding the nth buffer in a folio; in
that case you pass the head buffer and the byte offset in the folio
divided by the block size. It can be used for other purposes, but
it will wrap at the end of the folio rather than returning NULL or
proceeding to the next folio for you.

**Return**

The requested buffer with an elevated refcount.

int mmb\_sync(struct mapping\_metadata\_bhs \*mmb)
:   write out & wait upon all buffers in a list

**Parameters**

`struct mapping_metadata_bhs *mmb`
:   the list of buffers to write

**Description**

Starts I/O against the buffers in the given list and waits upon
that I/O. Basically, this is a convenience function for `fsync()`. **mmb** is
for a file or directory which needs those buffers to be written for a
successful `fsync()`.

We have conflicting pressures: we want to make sure that all
initially dirty buffers get waited on, but that any subsequently
dirtied buffers don’t. After all, we don’t want fsync to last
forever if somebody is actively writing to the file.

Do this in two main stages: first we copy dirty buffers to a
temporary inode list, queueing the writes as we go. Then we clean
up, waiting for those writes to complete. `mark_buffer_dirty_inode()`
doesn’t touch b\_assoc\_buffers list if b\_mmb is not NULL so we are sure the
buffer stays on our list until IO completes (at which point it can be
reaped).

int mmb\_fsync\_noflush(struct [file](#c.mmb_fsync_noflush "file") \*file, struct mapping\_metadata\_bhs \*mmb, loff\_t start, loff\_t end, bool datasync)
:   fsync implementation for simple filesystems with metadata buffers list

**Parameters**

`struct file *file`
:   file to synchronize

`struct mapping_metadata_bhs *mmb`
:   list of metadata bhs to flush

`loff_t start`
:   start offset in bytes

`loff_t end`
:   end offset in bytes (inclusive)

`bool datasync`
:   only synchronize essential metadata if true

**Description**

This is an implementation of the fsync method for simple filesystems which
track all non-inode metadata in the buffers list hanging off the **mmb**
structure.

int mmb\_fsync(struct [file](#c.mmb_fsync "file") \*file, struct mapping\_metadata\_bhs \*mmb, loff\_t start, loff\_t end, bool datasync)
:   fsync implementation for simple filesystems with metadata buffers list

**Parameters**

`struct file *file`
:   file to synchronize

`struct mapping_metadata_bhs *mmb`
:   list of metadata bhs to flush

`loff_t start`
:   start offset in bytes

`loff_t end`
:   end offset in bytes (inclusive)

`bool datasync`
:   only synchronize essential metadata if true

**Description**

This is an implementation of the fsync method for simple filesystems which
track all non-inode metadata in the buffers list hanging off the **mmb**
structure. This also makes sure that a device cache flush operation is
called at the end.

bool block\_dirty\_folio(struct [address\_space](api-summary.html#c.address_space "address_space") \*mapping, struct [folio](#c.block_dirty_folio "folio") \*folio)
:   Mark a folio as dirty.

**Parameters**

`struct address_space *mapping`
:   The address space containing this folio.

`struct folio *folio`
:   The folio to mark dirty.

**Description**

Filesystems which use buffer\_heads can use this function as their
->dirty\_folio implementation. Some filesystems need to do a little
work before calling this function. Filesystems which do not use
buffer\_heads should call [`filemap_dirty_folio()`](../core-api/mm-api.html#c.filemap_dirty_folio "filemap_dirty_folio") instead.

If the folio has buffers, the uptodate buffers are set dirty, to
preserve dirty-state coherency between the folio and the buffers.
Buffers added to a dirty folio are created dirty.

The buffers are dirtied before the folio is dirtied. There’s a small
race window in which writeback may see the folio cleanness but not the
buffer dirtiness. That’s fine. If this code were to set the folio
dirty before the buffers, writeback could clear the folio dirty flag,
see a bunch of clean buffers and we’d end up with dirty buffers/clean
folio on the dirty folio list.

We use i\_private\_lock to lock against [`try_to_free_buffers()`](#c.try_to_free_buffers "try_to_free_buffers") while
using the folio’s buffer list. This also prevents clean buffers
being added to the folio after it was set dirty.

**Context**

May only be called from process context. Does not sleep.
Caller must ensure that **folio** cannot be truncated during this call,
typically by holding the folio lock or having a page in the folio
mapped and holding the page table lock.

**Return**

True if the folio was dirtied; false if it was already dirtied.

void mark\_buffer\_dirty(struct buffer\_head \*bh)
:   mark a buffer\_head as needing writeout

**Parameters**

`struct buffer_head *bh`
:   the buffer\_head to mark dirty

**Description**

[`mark_buffer_dirty()`](#c.mark_buffer_dirty "mark_buffer_dirty") will set the dirty bit against the buffer, then set
its backing page dirty, then tag the page as dirty in the page cache
and then attach the address\_space’s inode to its superblock’s dirty
inode list.

[`mark_buffer_dirty()`](#c.mark_buffer_dirty "mark_buffer_dirty") is atomic. It takes bh->b\_folio->mapping->i\_private\_lock,
i\_pages lock and mapping->host->i\_lock.

void \_\_brelse(struct buffer\_head \*bh)
:   Release a buffer.

**Parameters**

`struct buffer_head *bh`
:   The buffer to release.

**Description**

This variant of [`brelse()`](#c.brelse "brelse") can be called if **bh** is guaranteed to not be NULL.

void \_\_bforget(struct buffer\_head \*bh)
:   Discard any dirty data in a buffer.

**Parameters**

`struct buffer_head *bh`
:   The buffer to forget.

**Description**

This variant of [`bforget()`](#c.bforget "bforget") can be called if **bh** is guaranteed to not
be NULL.

struct buffer\_head \*bdev\_getblk(struct block\_device \*bdev, sector\_t block, unsigned size, gfp\_t gfp)
:   Get a buffer\_head in a block device’s buffer cache.

**Parameters**

`struct block_device *bdev`
:   The block device.

`sector_t block`
:   The block number.

`unsigned size`
:   The size of buffer\_heads for this **bdev**.

`gfp_t gfp`
:   The memory allocation flags to use.

**Description**

The returned buffer head has its reference count incremented, but is
not locked. The caller should call [`brelse()`](#c.brelse "brelse") when it has finished
with the buffer. The buffer may not be uptodate. If needed, the
caller can bring it uptodate either by reading it or overwriting it.

**Return**

The buffer head, or NULL if memory could not be allocated.

struct buffer\_head \*\_\_bread\_gfp(struct block\_device \*bdev, sector\_t block, unsigned size, gfp\_t gfp)
:   Read a block.

**Parameters**

`struct block_device *bdev`
:   The block device to read from.

`sector_t block`
:   Block number in units of block size.

`unsigned size`
:   The block size of this device in bytes.

`gfp_t gfp`
:   Not page allocation flags; see below.

**Description**

You are not expected to call this function. You should use one of
`sb_bread()`, `sb_bread_unmovable()` or [`__bread()`](#c.__bread "__bread").

Read a specified block, and return the buffer head that refers to it.
If **gfp** is 0, the memory will be allocated using the block device’s
default GFP flags. If **gfp** is \_\_GFP\_MOVABLE, the memory may be
allocated from a movable area. Do not pass in a complete set of
GFP flags.

The returned buffer head has its refcount increased. The caller should
call [`brelse()`](#c.brelse "brelse") when it has finished with the buffer.

**Context**

May sleep waiting for I/O.

**Return**

NULL if the block was unreadable.

void block\_invalidate\_folio(struct [folio](#c.block_invalidate_folio "folio") \*folio, size\_t offset, size\_t length)
:   Invalidate part or all of a buffer-backed folio.

**Parameters**

`struct folio *folio`
:   The folio which is affected.

`size_t offset`
:   start of the range to invalidate

`size_t length`
:   length of the range to invalidate

**Description**

[`block_invalidate_folio()`](#c.block_invalidate_folio "block_invalidate_folio") is called when all or part of the folio has been
invalidated by a truncate operation.

[`block_invalidate_folio()`](#c.block_invalidate_folio "block_invalidate_folio") does not have to release all buffers, but it must
ensure that no dirty buffer is left outside **offset** and that no I/O
is underway against any of the blocks which are outside the truncation
point. Because the caller is about to free (and possibly reuse) those
blocks on-disk.

void clean\_bdev\_aliases(struct block\_device \*bdev, sector\_t block, sector\_t len)
:   clean a range of buffers in block device

**Parameters**

`struct block_device *bdev`
:   Block device to clean buffers in

`sector_t block`
:   Start of a range of blocks to clean

`sector_t len`
:   Number of blocks to clean

**Description**

We are taking a range of blocks for data and we don’t want writeback of any
buffer-cache aliases starting from return from this function and until the
moment when something will explicitly mark the buffer dirty (hopefully that
will not happen until we will free that block ;-) We don’t even need to mark
it not-uptodate - nobody can expect anything from a newly allocated buffer
anyway. We used to use `unmap_buffer()` for such invalidation, but that was
wrong. We definitely don’t want to mark the alias unmapped, for example - it
would confuse anyone who might pick it with `bread()` afterwards...

Also.. Note that [`bforget()`](#c.bforget "bforget") doesn’t lock the buffer. So there can be
writeout I/O going on against recently-freed buffers. We don’t wait on that
I/O in [`bforget()`](#c.bforget "bforget") - it’s more efficient to wait on the I/O only if we really
need to. That happens here.

bool try\_to\_free\_buffers(struct [folio](#c.try_to_free_buffers "folio") \*folio)
:   Release buffers attached to this folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

If any buffers are in use (dirty, under writeback, elevated refcount),
no buffers will be freed.

If the folio is dirty but all the buffers are clean then we need to
be sure to mark the folio clean as well. This is because the folio
may be against a block device, and a later reattachment of buffers
to a dirty folio will set *all* buffers dirty. Which would corrupt
filesystem data on the same device.

The same applies to regular filesystem folios: if all the buffers are
clean then we set the folio clean and proceed. To do that, we require
total exclusion from [`block_dirty_folio()`](#c.block_dirty_folio "block_dirty_folio"). That is obtained with
i\_private\_lock.

Exclusion against try\_to\_free\_buffers may be obtained by either
locking the folio or by holding its mapping’s i\_private\_lock.

**Context**

Process context. **folio** must be locked. Will not sleep.

**Return**

true if all buffers attached to this folio were freed.

int bh\_uptodate\_or\_lock(struct buffer\_head \*bh)
:   Test whether the buffer is uptodate

**Parameters**

`struct buffer_head *bh`
:   `struct buffer_head`

**Description**

Return true if the buffer is up-to-date and false,
with the buffer locked, if not.

int \_\_bh\_read(struct buffer\_head \*bh, blk\_opf\_t op\_flags, bool wait)
:   Submit read for a locked buffer

**Parameters**

`struct buffer_head *bh`
:   `struct buffer_head`

`blk_opf_t op_flags`
:   appending REQ\_OP\_\* flags besides REQ\_OP\_READ

`bool wait`
:   wait until reading finish

**Description**

Returns zero on success or don’t wait, and -EIO on error.

void \_\_bh\_read\_batch(int nr, struct buffer\_head \*bhs[], blk\_opf\_t op\_flags, bool force\_lock)
:   Submit read for a batch of unlocked buffers

**Parameters**

`int nr`
:   entry number of the buffer batch

`struct buffer_head *bhs[]`
:   a batch of `struct buffer_head`

`blk_opf_t op_flags`
:   appending REQ\_OP\_\* flags besides REQ\_OP\_READ

`bool force_lock`
:   force to get a lock on the buffer if set, otherwise drops any
    buffer that cannot lock.

**Description**

Returns zero on success or don’t wait, and -EIO on error.
