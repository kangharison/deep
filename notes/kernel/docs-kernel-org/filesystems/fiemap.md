# Fiemap Ioctl

> 출처(원문): https://docs.kernel.org/filesystems/fiemap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Fiemap Ioctl

The fiemap ioctl is an efficient method for userspace to get file
extent mappings. Instead of block-by-block mapping (such as bmap), fiemap
returns a list of extents.

## Request Basics

A fiemap request is encoded within [`struct fiemap`](#c.fiemap "fiemap"):

struct fiemap
:   file extent mappings

**Definition**:

```
struct fiemap {
    __u64 fm_start;
    __u64 fm_length;
    __u32 fm_flags;
    __u32 fm_mapped_extents;
    __u32 fm_extent_count;
    struct fiemap_extent fm_extents[];
};
```

**Members**

`fm_start`
:   byte offset (inclusive) at which to start mapping (in)

`fm_length`
:   logical length of mapping which userspace wants (in)

`fm_flags`
:   FIEMAP\_FLAG\_\* flags for request (in/out)

`fm_mapped_extents`
:   number of extents that were mapped (out)

`fm_extent_count`
:   size of fm\_extents array (in)

`fm_extents`
:   array of mapped extents (out)

fm\_start, and fm\_length specify the logical range within the file
which the process would like mappings for. Extents returned mirror
those on disk - that is, the logical offset of the 1st returned extent
may start before fm\_start, and the range covered by the last returned
extent may end after fm\_length. All offsets and lengths are in bytes.

Certain flags to modify the way in which mappings are looked up can be
set in fm\_flags. If the kernel doesn’t understand some particular
flags, it will return EBADR and the contents of fm\_flags will contain
the set of flags which caused the error. If the kernel is compatible
with all flags passed, the contents of fm\_flags will be unmodified.
It is up to userspace to determine whether rejection of a particular
flag is fatal to its operation. This scheme is intended to allow the
fiemap interface to grow in the future but without losing
compatibility with old software.

fm\_extent\_count specifies the number of elements in the fm\_extents[] array
that can be used to return extents. If fm\_extent\_count is zero, then the
fm\_extents[] array is ignored (no extents will be returned), and the
fm\_mapped\_extents count will hold the number of extents needed in
fm\_extents[] to hold the file’s current mapping. Note that there is
nothing to prevent the file from changing between calls to FIEMAP.

The following flags can be set in fm\_flags:

FIEMAP\_FLAG\_SYNC
:   If this flag is set, the kernel will sync the file before mapping extents.

FIEMAP\_FLAG\_XATTR
:   If this flag is set, the extents returned will describe the inodes
    extended attribute lookup tree, instead of its data tree.

FIEMAP\_FLAG\_CACHE
:   This flag requests caching of the extents.

## Extent Mapping

Extent information is returned within the embedded fm\_extents array
which userspace must allocate along with the fiemap structure. The
number of elements in the fiemap\_extents[] array should be passed via
fm\_extent\_count. The number of extents mapped by kernel will be
returned via fm\_mapped\_extents. If the number of fiemap\_extents
allocated is less than would be required to map the requested range,
the maximum number of extents that can be mapped in the fm\_extent[]
array will be returned and fm\_mapped\_extents will be equal to
fm\_extent\_count. In that case, the last extent in the array will not
complete the requested range and will not have the FIEMAP\_EXTENT\_LAST
flag set (see the next section on extent flags).

Each extent is described by a single fiemap\_extent structure as
returned in fm\_extents:

struct fiemap\_extent
:   description of one fiemap extent

**Definition**:

```
struct fiemap_extent {
    __u64 fe_logical;
    __u64 fe_physical;
    __u64 fe_length;
    __u32 fe_flags;
};
```

**Members**

`fe_logical`
:   byte offset of the extent in the file

`fe_physical`
:   byte offset of extent on disk

`fe_length`
:   length in bytes for this extent

`fe_flags`
:   FIEMAP\_EXTENT\_\* flags for this extent

All offsets and lengths are in bytes and mirror those on disk. It is valid
for an extents logical offset to start before the request or its logical
length to extend past the request. Unless FIEMAP\_EXTENT\_NOT\_ALIGNED is
returned, fe\_logical, fe\_physical, and fe\_length will be aligned to the
block size of the file system. With the exception of extents flagged as
FIEMAP\_EXTENT\_MERGED, adjacent extents will not be merged.

The fe\_flags field contains flags which describe the extent returned.
A special flag, FIEMAP\_EXTENT\_LAST is always set on the last extent in
the file so that the process making fiemap calls can determine when no
more extents are available, without having to call the ioctl again.

Some flags are intentionally vague and will always be set in the
presence of other more specific flags. This way a program looking for
a general property does not have to know all existing and future flags
which imply that property.

For example, if FIEMAP\_EXTENT\_DATA\_INLINE or FIEMAP\_EXTENT\_DATA\_TAIL
are set, FIEMAP\_EXTENT\_NOT\_ALIGNED will also be set. A program looking
for inline or tail-packed data can key on the specific flag. Software
which simply cares not to try operating on non-aligned extents
however, can just key on FIEMAP\_EXTENT\_NOT\_ALIGNED, and not have to
worry about all present and future flags which might imply unaligned
data. Note that the opposite is not true - it would be valid for
FIEMAP\_EXTENT\_NOT\_ALIGNED to appear alone.

FIEMAP\_EXTENT\_LAST
:   This is generally the last extent in the file. A mapping attempt past
    this extent may return nothing. Some implementations set this flag to
    indicate this extent is the last one in the range queried by the user
    (via fiemap->fm\_length).

FIEMAP\_EXTENT\_UNKNOWN
:   The location of this extent is currently unknown. This may indicate
    the data is stored on an inaccessible volume or that no storage has
    been allocated for the file yet.

FIEMAP\_EXTENT\_DELALLOC
:   This will also set FIEMAP\_EXTENT\_UNKNOWN.

    Delayed allocation - while there is data for this extent, its
    physical location has not been allocated yet.

FIEMAP\_EXTENT\_ENCODED
:   This extent does not consist of plain filesystem blocks but is
    encoded (e.g. encrypted or compressed). Reading the data in this
    extent via I/O to the block device will have undefined results.

Note that it is *always* undefined to try to update the data
in-place by writing to the indicated location without the
assistance of the filesystem, or to access the data using the
information returned by the FIEMAP interface while the filesystem
is mounted. In other words, user applications may only read the
extent data via I/O to the block device while the filesystem is
unmounted, and then only if the FIEMAP\_EXTENT\_ENCODED flag is
clear; user applications must not try reading or writing to the
filesystem via the block device under any other circumstances.

FIEMAP\_EXTENT\_DATA\_ENCRYPTED
:   This will also set FIEMAP\_EXTENT\_ENCODED
    The data in this extent has been encrypted by the file system.

FIEMAP\_EXTENT\_NOT\_ALIGNED
:   Extent offsets and length are not guaranteed to be block aligned.

FIEMAP\_EXTENT\_DATA\_INLINE
:   This will also set FIEMAP\_EXTENT\_NOT\_ALIGNED
    Data is located within a meta data block.

FIEMAP\_EXTENT\_DATA\_TAIL
:   This will also set FIEMAP\_EXTENT\_NOT\_ALIGNED
    Data is packed into a block with data from other files.

FIEMAP\_EXTENT\_UNWRITTEN
:   Unwritten extent - the extent is allocated but its data has not been
    initialized. This indicates the extent’s data will be all zero if read
    through the filesystem but the contents are undefined if read directly from
    the device.

FIEMAP\_EXTENT\_MERGED
:   This will be set when a file does not support extents, i.e., it uses a block
    based addressing scheme. Since returning an extent for each block back to
    userspace would be highly inefficient, the kernel will try to merge most
    adjacent blocks into ‘extents’.

FIEMAP\_EXTENT\_SHARED
:   This flag is set to request that space be shared with other files.

## VFS -> File System Implementation

File systems wishing to support fiemap must implement a ->fiemap callback on
their inode\_operations structure. The fs ->fiemap call is responsible for
defining its set of supported fiemap flags, and calling a helper function on
each discovered extent:

```
struct inode_operations {
     ...

     int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start,
                   u64 len);
```

->fiemap is passed [`struct fiemap_extent_info`](#c.fiemap_extent_info "fiemap_extent_info") which describes the
fiemap request:

struct fiemap\_extent\_info
:   fiemap request to a filesystem

**Definition**:

```
struct fiemap_extent_info {
    unsigned int fi_flags;
    unsigned int fi_extents_mapped;
    unsigned int fi_extents_max;
    struct fiemap_extent __user *fi_extents_start;
};
```

**Members**

`fi_flags`
:   Flags as passed from user

`fi_extents_mapped`
:   Number of mapped extents

`fi_extents_max`
:   Size of fiemap\_extent array

`fi_extents_start`
:   Start of fiemap\_extent array

It is intended that the file system should not need to access any of this
structure directly. Filesystem handlers should be tolerant to signals and return
EINTR once fatal signal received.

Flag checking should be done at the beginning of the ->fiemap callback via the
`fiemap_prep()` helper:

```
int fiemap_prep(struct inode *inode, struct fiemap_extent_info *fieinfo,
                u64 start, u64 *len, u32 supported_flags);
```

The `struct fieinfo` should be passed in as received from `ioctl_fiemap()`. The
set of fiemap flags which the fs understands should be passed via fs\_flags. If
fiemap\_prep finds invalid user flags, it will place the bad values in
fieinfo->fi\_flags and return -EBADR. If the file system gets -EBADR, from
`fiemap_prep()`, it should immediately exit, returning that error back to
`ioctl_fiemap()`. Additionally the range is validate against the supported
maximum file size.

For each extent in the request range, the file system should call
the helper function, `fiemap_fill_next_extent()`:

```
int fiemap_fill_next_extent(struct fiemap_extent_info *info, u64 logical,
                            u64 phys, u64 len, u32 flags, u32 dev);
```

`fiemap_fill_next_extent()` will use the passed values to populate the
next free extent in the fm\_extents array. ‘General’ extent flags will
automatically be set from specific flags on behalf of the calling file
system so that the userspace API is not broken.

`fiemap_fill_next_extent()` returns 0 on success, and 1 when the
user-supplied fm\_extents array is full. If an error is encountered
while copying the extent to user memory, -EFAULT will be returned.
