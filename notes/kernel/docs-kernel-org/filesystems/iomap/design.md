# 1.Library Design

> 출처(원문): https://docs.kernel.org/filesystems/iomap/design.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1. Library Design

## [1.1. Introduction](#id1)

iomap is a filesystem library for handling common file operations.
The library has two layers:

> 1. A lower layer that provides an iterator over ranges of file offsets.
>    This layer tries to obtain mappings of each file ranges to storage
>    from the filesystem, but the storage information is not necessarily
>    required.
> 2. An upper layer that acts upon the space mappings provided by the
>    lower layer iterator.

The iteration can involve mappings of file’s logical offset ranges to
physical extents, but the storage layer information is not necessarily
required, e.g. for walking cached file information.
The library exports various APIs for implementing file operations such
as:

> * Pagecache reads and writes
> * Folio write faults to the pagecache
> * Writeback of dirty folios
> * Direct I/O reads and writes
> * fsdax I/O reads, writes, loads, and stores
> * FIEMAP
> * lseek `SEEK_DATA` and `SEEK_HOLE`
> * swapfile activation

This origins of this library is the file I/O path that XFS once used; it
has now been extended to cover several other operations.

## [1.2. Who Should Read This?](#id2)

The target audience for this document are filesystem, storage, and
pagecache programmers and code reviewers.

If you are working on PCI, machine architectures, or device drivers, you
are most likely in the wrong place.

## [1.3. How Is This Better?](#id3)

Unlike the classic Linux I/O model which breaks file I/O into small
units (generally memory pages or blocks) and looks up space mappings on
the basis of that unit, the iomap model asks the filesystem for the
largest space mappings that it can create for a given file operation and
initiates operations on that basis.
This strategy improves the filesystem’s visibility into the size of the
operation being performed, which enables it to combat fragmentation with
larger space allocations when possible.
Larger space mappings improve runtime performance by amortizing the cost
of mapping function calls into the filesystem across a larger amount of
data.

At a high level, an iomap operation [looks like this](https://lore.kernel.org/all/ZGbVaewzcCysclPt@dread.disaster.area/):

1. For each byte in the operation range...

   1. Obtain a space mapping via `->iomap_begin`
   2. For each sub-unit of work...

      1. Revalidate the mapping and go back to (1) above, if necessary.
         So far only the pagecache operations need to do this.
      2. Do the work
   3. Increment operation cursor
   4. Release the mapping via `->iomap_end`, if necessary

Each iomap operation will be covered in more detail below.
This library was covered previously by an [LWN article](https://lwn.net/Articles/935934/) and a [KernelNewbies page](https://kernelnewbies.org/KernelProjects/iomap).

The goal of this document is to provide a brief discussion of the
design and capabilities of iomap, followed by a more detailed catalog
of the interfaces presented by iomap.
If you change iomap, please update this design document.

## [1.4. File Range Iterator](#id4)

### [1.4.1. Definitions](#id5)

> * **buffer head**: Shattered remnants of the old buffer cache.
> * `fsblock`: The block size of a file, also known as `i_blocksize`.
> * `i_rwsem`: The VFS `struct inode` rwsemaphore.
>   Processes hold this in shared mode to read file state and contents.
>   Some filesystems may allow shared mode for writes.
>   Processes often hold this in exclusive mode to change file state and
>   contents.
> * `invalidate_lock`: The pagecache `struct address_space`
>   rwsemaphore that protects against folio insertion and removal for
>   filesystems that support punching out folios below EOF.
>   Processes wishing to insert folios must hold this lock in shared
>   mode to prevent removal, though concurrent insertion is allowed.
>   Processes wishing to remove folios must hold this lock in exclusive
>   mode to prevent insertions.
>   Concurrent removals are not allowed.
> * `dax_read_lock`: The RCU read lock that dax takes to prevent a
>   device pre-shutdown hook from returning before other threads have
>   released resources.
> * **filesystem mapping lock**: This synchronization primitive is
>   internal to the filesystem and must protect the file mapping data
>   from updates while a mapping is being sampled.
>   The filesystem author must determine how this coordination should
>   happen; it does not need to be an actual lock.
> * **iomap internal operation lock**: This is a general term for
>   synchronization primitives that iomap functions take while holding a
>   mapping.
>   A specific example would be taking the folio lock while reading or
>   writing the pagecache.
> * **pure overwrite**: A write operation that does not require any
>   metadata or zeroing operations to perform during either submission
>   or completion.
>   This implies that the filesystem must have already allocated space
>   on disk as `IOMAP_MAPPED` and the filesystem must not place any
>   constraints on IO alignment or size.
>   The only constraints on I/O alignment are device level (minimum I/O
>   size and alignment, typically sector size).

### [1.4.2. `struct iomap`](#id6)

The filesystem communicates to the iomap iterator the mapping of
byte ranges of a file to byte ranges of a storage device with the
structure below:

```
struct iomap {
    u64                 addr;
    loff_t              offset;
    u64                 length;
    u16                 type;
    u16                 flags;
    struct block_device *bdev;
    struct dax_device   *dax_dev;
    void                *inline_data;
    void                *private;
    u64                 validity_cookie;
};
```

The fields are as follows:

> * `offset` and `length` describe the range of file offsets, in
>   bytes, covered by this mapping.
>   These fields must always be set by the filesystem.
> * `type` describes the type of the space mapping:
>
>   + **IOMAP\_HOLE**: No storage has been allocated.
>     This type must never be returned in response to an `IOMAP_WRITE`
>     operation because writes must allocate and map space, and return
>     the mapping.
>     The `addr` field must be set to `IOMAP_NULL_ADDR`.
>     iomap does not support writing (whether via pagecache or direct
>     I/O) to a hole.
>   + **IOMAP\_DELALLOC**: A promise to allocate space at a later time
>     (“delayed allocation”).
>     If the filesystem returns IOMAP\_F\_NEW here and the write fails, the
>     `->iomap_end` function must delete the reservation.
>     The `addr` field must be set to `IOMAP_NULL_ADDR`.
>   + **IOMAP\_MAPPED**: The file range maps to specific space on the
>     storage device.
>     The device is returned in `bdev` or `dax_dev`.
>     The device address, in bytes, is returned via `addr`.
>   + **IOMAP\_UNWRITTEN**: The file range maps to specific space on the
>     storage device, but the space has not yet been initialized.
>     The device is returned in `bdev` or `dax_dev`.
>     The device address, in bytes, is returned via `addr`.
>     Reads from this type of mapping will return zeroes to the caller.
>     For a write or writeback operation, the ioend should update the
>     mapping to MAPPED.
>     Refer to the sections about ioends for more details.
>   + **IOMAP\_INLINE**: The file range maps to the memory buffer
>     specified by `inline_data`.
>     For write operation, the `->iomap_end` function presumably
>     handles persisting the data.
>     The `addr` field must be set to `IOMAP_NULL_ADDR`.
> * `flags` describe the status of the space mapping.
>   These flags should be set by the filesystem in `->iomap_begin`:
>
>   + **IOMAP\_F\_NEW**: The space under the mapping is newly allocated.
>     Areas that will not be written to must be zeroed.
>     If a write fails and the mapping is a space reservation, the
>     reservation must be deleted.
>   + **IOMAP\_F\_DIRTY**: The inode will have uncommitted metadata needed
>     to access any data written.
>     fdatasync is required to commit these changes to persistent
>     storage.
>     This needs to take into account metadata changes that *may* be made
>     at I/O completion, such as file size updates from direct I/O.
>   + **IOMAP\_F\_SHARED**: The space under the mapping is shared.
>     Copy on write is necessary to avoid corrupting other file data.
>   + **IOMAP\_F\_BUFFER\_HEAD**: This mapping requires the use of buffer
>     heads for pagecache operations.
>     Do not add more uses of this.
>   + **IOMAP\_F\_MERGED**: Multiple contiguous block mappings were
>     coalesced into this single mapping.
>     This is only useful for FIEMAP.
>   + **IOMAP\_F\_XATTR**: The mapping is for extended attribute data, not
>     regular file data.
>     This is only useful for FIEMAP.
>   + **IOMAP\_F\_BOUNDARY**: This indicates I/O and its completion must not be
>     merged with any other I/O or completion. Filesystems must use this when
>     submitting I/O to devices that cannot handle I/O crossing certain LBAs
>     (e.g. ZNS devices). This flag applies only to buffered I/O writeback; all
>     other functions ignore it.
>   + **IOMAP\_F\_PRIVATE**: This flag is reserved for filesystem private use.
>   + **IOMAP\_F\_ANON\_WRITE**: Indicates that (write) I/O does not have a target
>     block assigned to it yet and the file system will do that in the bio
>     submission handler, splitting the I/O as needed.
>   + **IOMAP\_F\_ATOMIC\_BIO**: This indicates write I/O must be submitted with the
>     `REQ_ATOMIC` flag set in the bio. Filesystems need to set this flag to
>     inform iomap that the write I/O operation requires torn-write protection
>     based on HW-offload mechanism. They must also ensure that mapping updates
>     upon the completion of the I/O must be performed in a single metadata
>     update.
>
>   These flags can be set by iomap itself during file operations.
>   The filesystem should supply an `->iomap_end` function if it needs
>   to observe these flags:
>
>   + **IOMAP\_F\_SIZE\_CHANGED**: The file size has changed as a result of
>     using this mapping.
>   + **IOMAP\_F\_STALE**: The mapping was found to be stale.
>     iomap will call `->iomap_end` on this mapping and then
>     `->iomap_begin` to obtain a new mapping.
>
>   Currently, these flags are only set by pagecache operations.
> * `addr` describes the device address, in bytes.
> * `bdev` describes the block device for this mapping.
>   This only needs to be set for mapped or unwritten operations.
> * `dax_dev` describes the DAX device for this mapping.
>   This only needs to be set for mapped or unwritten operations, and
>   only for a fsdax operation.
> * `inline_data` points to a memory buffer for I/O involving
>   `IOMAP_INLINE` mappings.
>   This value is ignored for all other mapping types.
> * `private` is a pointer to [filesystem-private information](https://lore.kernel.org/all/20180619164137.13720-7-hch@lst.de/).
>   This value will be passed unchanged to `->iomap_end`.
> * `validity_cookie` is a magic freshness value set by the filesystem
>   that should be used to detect stale mappings.
>   For pagecache operations this is critical for correct operation
>   because page faults can occur, which implies that filesystem locks
>   should not be held between `->iomap_begin` and `->iomap_end`.
>   Filesystems with completely static mappings need not set this value.
>   Only pagecache operations revalidate mappings; see the section about
>   `iomap_valid` for details.

### [1.4.3. `struct iomap_ops`](#id7)

Every iomap function requires the filesystem to pass an operations
structure to obtain a mapping and (optionally) to release the mapping:

```
struct iomap_ops {
    int (*iomap_begin)(struct inode *inode, loff_t pos, loff_t length,
                       unsigned flags, struct iomap *iomap,
                       struct iomap *srcmap);

    int (*iomap_end)(struct inode *inode, loff_t pos, loff_t length,
                     ssize_t written, unsigned flags,
                     struct iomap *iomap);
};
```

#### [1.4.3.1. `->iomap_begin`](#id8)

iomap operations call `->iomap_begin` to obtain one file mapping for
the range of bytes specified by `pos` and `length` for the file
`inode`.
This mapping should be returned through the `iomap` pointer.
The mapping must cover at least the first byte of the supplied file
range, but it does not need to cover the entire requested range.

Each iomap operation describes the requested operation through the
`flags` argument.
The exact value of `flags` will be documented in the
operation-specific sections below.
These flags can, at least in principle, apply generally to iomap
operations:

> * `IOMAP_DIRECT` is set when the caller wishes to issue file I/O to
>   block storage.
> * `IOMAP_DAX` is set when the caller wishes to issue file I/O to
>   memory-like storage.
> * `IOMAP_NOWAIT` is set when the caller wishes to perform a best
>   effort attempt to avoid any operation that would result in blocking
>   the submitting task.
>   This is similar in intent to `O_NONBLOCK` for network APIs - it is
>   intended for asynchronous applications to keep doing other work
>   instead of waiting for the specific unavailable filesystem resource
>   to become available.
>   Filesystems implementing `IOMAP_NOWAIT` semantics need to use
>   trylock algorithms.
>   They need to be able to satisfy the entire I/O request range with a
>   single iomap mapping.
>   They need to avoid reading or writing metadata synchronously.
>   They need to avoid blocking memory allocations.
>   They need to avoid waiting on transaction reservations to allow
>   modifications to take place.
>   They probably should not be allocating new space.
>   And so on.
>   If there is any doubt in the filesystem developer’s mind as to
>   whether any specific `IOMAP_NOWAIT` operation may end up blocking,
>   then they should return `-EAGAIN` as early as possible rather than
>   start the operation and force the submitting task to block.
>   `IOMAP_NOWAIT` is often set on behalf of `IOCB_NOWAIT` or
>   `RWF_NOWAIT`.
> * `IOMAP_DONTCACHE` is set when the caller wishes to perform a
>   buffered file I/O and would like the kernel to drop the pagecache
>   after the I/O completes, if it isn’t already being used by another
>   thread.

If it is necessary to read existing file contents from a [different](https://lore.kernel.org/all/20191008071527.29304-9-hch@lst.de/)
device or address range on a device, the filesystem should return that
information via `srcmap`.
Only pagecache and fsdax operations support reading from one mapping and
writing to another.

#### [1.4.3.2. `->iomap_end`](#id9)

After the operation completes, the `->iomap_end` function, if present,
is called to signal that iomap is finished with a mapping.
Typically, implementations will use this function to tear down any
context that were set up in `->iomap_begin`.
For example, a write might wish to commit the reservations for the bytes
that were operated upon and unreserve any space that was not operated
upon.
`written` might be zero if no bytes were touched.
`flags` will contain the same value passed to `->iomap_begin`.
iomap ops for reads are not likely to need to supply this function.

Both functions should return a negative errno code on error, or zero on
success.

## [1.5. Preparing for File Operations](#id10)

iomap only handles mapping and I/O.
Filesystems must still call out to the VFS to check input parameters
and file state before initiating an I/O operation.
It does not handle obtaining filesystem freeze protection, updating of
timestamps, stripping privileges, or access control.

## [1.6. Locking Hierarchy](#id11)

iomap requires that filesystems supply their own locking model.
There are three categories of synchronization primitives, as far as
iomap is concerned:

> * The **upper** level primitive is provided by the filesystem to
>   coordinate access to different iomap operations.
>   The exact primitive is specific to the filesystem and operation,
>   but is often a VFS inode, pagecache invalidation, or folio lock.
>   For example, a filesystem might take `i_rwsem` before calling
>   `iomap_file_buffered_write` and `iomap_file_unshare` to prevent
>   these two file operations from clobbering each other.
>   Pagecache writeback may lock a folio to prevent other threads from
>   accessing the folio until writeback is underway.
>
>   + The **lower** level primitive is taken by the filesystem in the
>     `->iomap_begin` and `->iomap_end` functions to coordinate
>     access to the file space mapping information.
>     The fields of the iomap object should be filled out while holding
>     this primitive.
>     The upper level synchronization primitive, if any, remains held
>     while acquiring the lower level synchronization primitive.
>     For example, XFS takes `ILOCK_EXCL` and ext4 takes `i_data_sem`
>     while sampling mappings.
>     Filesystems with immutable mapping information may not require
>     synchronization here.
>   + The **operation** primitive is taken by an iomap operation to
>     coordinate access to its own internal data structures.
>     The upper level synchronization primitive, if any, remains held
>     while acquiring this primitive.
>     The lower level primitive is not held while acquiring this
>     primitive.
>     For example, pagecache write operations will obtain a file mapping,
>     then grab and lock a folio to copy new contents.
>     It may also lock an internal folio state object to update metadata.

The exact locking requirements are specific to the filesystem; for
certain operations, some of these locks can be elided.
All further mentions of locking are *recommendations*, not mandates.
Each filesystem author must figure out the locking for themself.

## [1.7. Bugs and Limitations](#id12)

> * No support for fscrypt.
> * No support for compression.
> * No support for fsverity yet.
> * Strong assumptions that IO should work the way it does on XFS.
> * Does iomap *actually* work for non-regular file data?

Patches welcome!
