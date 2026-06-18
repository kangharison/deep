# Linux Filesystems API summary

> 출처(원문): https://docs.kernel.org/filesystems/api-summary.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Filesystems API summary

This section contains API-level documentation, mostly taken from the source
code itself.

## The Linux VFS

### The Filesystem types

enum positive\_aop\_returns
:   aop return codes with specific semantics

**Constants**

`AOP_WRITEPAGE_ACTIVATE`
:   Informs the caller that page writeback has
    completed, that the page is still locked, and
    should be considered active. The VM uses this hint
    to return the page to the active list -- it won’t
    be a candidate for writeback again in the near
    future. Other callers must be careful to unlock
    the page if they get this return. Returned by
    `writepage()`;

`AOP_TRUNCATED_PAGE`
:   The AOP method that was handed a locked page has
    unlocked it and the page might have been truncated.
    The caller should back up to acquiring a new page and
    trying again. The aop will be taking reasonable
    precautions not to livelock. If the caller held a page
    reference, it should drop it before retrying. Returned
    by `read_folio()`.

**Description**

address\_space\_operation functions return these large constants to indicate
special semantics to the caller. These are much larger than the bytes in a
page to allow for functions that return the number of bytes operated on in a
given page.

struct address\_space
:   Contents of a cacheable, mappable object.

**Definition**:

```
struct address_space {
    struct inode            *host;
    struct xarray           i_pages;
    struct rw_semaphore     invalidate_lock;
    gfp_t gfp_mask;
    atomic_t i_mmap_writable;
#ifdef CONFIG_READ_ONLY_THP_FOR_FS;
    atomic_t nr_thps;
#endif;
    struct rb_root_cached   i_mmap;
    unsigned long           nrpages;
    pgoff_t writeback_index;
    const struct address_space_operations *a_ops;
    unsigned long           flags;
    errseq_t wb_err;
    spinlock_t i_private_lock;
    struct rw_semaphore     i_mmap_rwsem;
};
```

**Members**

`host`
:   Owner, either the inode or the block\_device.

`i_pages`
:   Cached pages.

`invalidate_lock`
:   Guards coherency between page cache contents and
    file offset->disk block mappings in the filesystem during invalidates.
    It is also used to block modification of page cache contents through
    memory mappings.

`gfp_mask`
:   Memory allocation flags to use for allocating pages.

`i_mmap_writable`
:   Number of VM\_SHARED, VM\_MAYWRITE mappings.

`nr_thps`
:   Number of THPs in the pagecache (non-shmem only).

`i_mmap`
:   Tree of private and shared mappings.

`nrpages`
:   Number of page entries, protected by the i\_pages lock.

`writeback_index`
:   Writeback starts here.

`a_ops`
:   Methods.

`flags`
:   Error bits and flags (AS\_\*).

`wb_err`
:   The most recent error which has occurred.

`i_private_lock`
:   For use by the owner of the address\_space.

`i_mmap_rwsem`
:   Protects **i\_mmap** and **i\_mmap\_writable**.

struct file\_ra\_state
:   Track a file’s readahead state.

**Definition**:

```
struct file_ra_state {
    pgoff_t start;
    unsigned int size;
    unsigned int async_size;
    unsigned int ra_pages;
    unsigned short order;
    unsigned short mmap_miss;
    loff_t prev_pos;
};
```

**Members**

`start`
:   Where the most recent readahead started.

`size`
:   Number of pages read in the most recent readahead.

`async_size`
:   Numer of pages that were/are not needed immediately
    and so were/are genuinely “ahead”. Start next readahead when
    the first of these pages is accessed.

`ra_pages`
:   Maximum size of a readahead request, copied from the bdi.

`order`
:   Preferred folio order used for most recent readahead.

`mmap_miss`
:   How many mmap accesses missed in the page cache.

`prev_pos`
:   The last byte in the most recent read request.

**Description**

When this structure is passed to ->`readahead()`, the “most recent”
readahead means the current readahead.

struct file
:   Represents a file

**Definition**:

```
struct file {
    spinlock_t f_lock;
    fmode_t f_mode;
    const struct file_operations    *f_op;
    struct address_space            *f_mapping;
    void *private_data;
    struct inode                    *f_inode;
    unsigned int                    f_flags;
    unsigned int                    f_iocb_flags;
    const struct cred               *f_cred;
    struct fown_struct              *f_owner;
    union {
        const struct path       f_path;
        struct path             __f_path;
    };
    union {
        struct mutex            f_pos_lock;
        u64 f_pipe;
    };
    loff_t f_pos;
#ifdef CONFIG_SECURITY;
    void *f_security;
#endif;
    errseq_t f_wb_err;
    errseq_t f_sb_err;
#ifdef CONFIG_EPOLL;
    struct hlist_head               *f_ep;
#endif;
    union {
        struct callback_head    f_task_work;
        struct llist_node       f_llist;
        struct file_ra_state    f_ra;
        freeptr_t f_freeptr;
    };
    file_ref_t f_ref;
};
```

**Members**

`f_lock`
:   Protects f\_ep, f\_flags. Must not be taken from IRQ context.

`f_mode`
:   FMODE\_\* flags often used in hotpaths

`f_op`
:   file operations

`f_mapping`
:   Contents of a cacheable, mappable object.

`private_data`
:   filesystem or driver specific data

`f_inode`
:   cached inode

`f_flags`
:   file flags

`f_iocb_flags`
:   iocb flags

`f_cred`
:   stashed credentials of creator/opener

`f_owner`
:   file owner

`{unnamed_union}`
:   anonymous

`f_path`
:   path of the file

`__f_path`
:   writable alias for **f\_path**; *ONLY* for core VFS and only before
    the file gets open

`{unnamed_union}`
:   anonymous

`f_pos_lock`
:   lock protecting file position

`f_pipe`
:   specific to pipes

`f_pos`
:   file position

`f_security`
:   LSM security context of this file

`f_wb_err`
:   writeback error

`f_sb_err`
:   per sb writeback errors

`f_ep`
:   link of all epoll hooks for this file

`{unnamed_union}`
:   anonymous

`f_task_work`
:   task work entry point

`f_llist`
:   work queue entrypoint

`f_ra`
:   file’s readahead state

`f_freeptr`
:   Pointer used by SLAB\_TYPESAFE\_BY\_RCU file cache (don’t touch.)

`f_ref`
:   reference count

vfsuid\_t i\_uid\_into\_vfsuid(struct mnt\_idmap \*idmap, const struct [inode](#c.i_uid_into_vfsuid "inode") \*inode)
:   map an inode’s i\_uid down according to an idmapping

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct inode *inode`
:   inode to map

**Return**

whe inode’s i\_uid mapped down according to **idmap**.
If the inode’s i\_uid has no mapping INVALID\_VFSUID is returned.

bool i\_uid\_needs\_update(struct mnt\_idmap \*idmap, const struct iattr \*attr, const struct [inode](#c.i_uid_needs_update "inode") \*inode)
:   check whether inode’s i\_uid needs to be updated

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct iattr *attr`
:   the new attributes of **inode**

`const struct inode *inode`
:   the inode to update

**Description**

Check whether the $inode’s i\_uid field needs to be updated taking idmapped
mounts into account if the filesystem supports it.

**Return**

true if **inode**’s i\_uid field needs to be updated, false if not.

void i\_uid\_update(struct mnt\_idmap \*idmap, const struct iattr \*attr, struct [inode](#c.i_uid_update "inode") \*inode)
:   update **inode**’s i\_uid field

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct iattr *attr`
:   the new attributes of **inode**

`struct inode *inode`
:   the inode to update

**Description**

Safely update **inode**’s i\_uid field translating the vfsuid of any idmapped
mount into the filesystem kuid.

vfsgid\_t i\_gid\_into\_vfsgid(struct mnt\_idmap \*idmap, const struct [inode](#c.i_gid_into_vfsgid "inode") \*inode)
:   map an inode’s i\_gid down according to an idmapping

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct inode *inode`
:   inode to map

**Return**

the inode’s i\_gid mapped down according to **idmap**.
If the inode’s i\_gid has no mapping INVALID\_VFSGID is returned.

bool i\_gid\_needs\_update(struct mnt\_idmap \*idmap, const struct iattr \*attr, const struct [inode](#c.i_gid_needs_update "inode") \*inode)
:   check whether inode’s i\_gid needs to be updated

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct iattr *attr`
:   the new attributes of **inode**

`const struct inode *inode`
:   the inode to update

**Description**

Check whether the $inode’s i\_gid field needs to be updated taking idmapped
mounts into account if the filesystem supports it.

**Return**

true if **inode**’s i\_gid field needs to be updated, false if not.

void i\_gid\_update(struct mnt\_idmap \*idmap, const struct iattr \*attr, struct [inode](#c.i_gid_update "inode") \*inode)
:   update **inode**’s i\_gid field

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct iattr *attr`
:   the new attributes of **inode**

`struct inode *inode`
:   the inode to update

**Description**

Safely update **inode**’s i\_gid field translating the vfsgid of any idmapped
mount into the filesystem kgid.

void inode\_fsuid\_set(struct [inode](#c.inode_fsuid_set "inode") \*inode, struct mnt\_idmap \*idmap)
:   initialize inode’s i\_uid field with callers fsuid

**Parameters**

`struct inode *inode`
:   inode to initialize

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

**Description**

Initialize the i\_uid field of **inode**. If the inode was found/created via
an idmapped mount map the caller’s fsuid according to **idmap**.

void inode\_fsgid\_set(struct [inode](#c.inode_fsgid_set "inode") \*inode, struct mnt\_idmap \*idmap)
:   initialize inode’s i\_gid field with callers fsgid

**Parameters**

`struct inode *inode`
:   inode to initialize

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

**Description**

Initialize the i\_gid field of **inode**. If the inode was found/created via
an idmapped mount map the caller’s fsgid according to **idmap**.

bool fsuidgid\_has\_mapping(struct super\_block \*sb, struct mnt\_idmap \*idmap)
:   check whether caller’s fsuid/fsgid is mapped

**Parameters**

`struct super_block *sb`
:   the superblock we want a mapping in

`struct mnt_idmap *idmap`
:   idmap of the relevant mount

**Description**

Check whether the caller’s fsuid and fsgid have a valid mapping in the
s\_user\_ns of the superblock **sb**. If the caller is on an idmapped mount map
the caller’s fsuid and fsgid according to the **idmap** first.

**Return**

true if fsuid and fsgid is mapped, false if not.

struct timespec64 inode\_set\_ctime(struct [inode](#c.inode_set_ctime "inode") \*inode, time64\_t sec, long nsec)
:   set the ctime in the inode

**Parameters**

`struct inode *inode`
:   inode in which to set the ctime

`time64_t sec`
:   tv\_sec value to set

`long nsec`
:   tv\_nsec value to set

**Description**

Set the ctime in **inode** to { **sec**, **nsec** }

bool file\_write\_started(const struct [file](#c.file_write_started "file") \*file)
:   check if SB\_FREEZE\_WRITE is held

**Parameters**

`const struct file *file`
:   the file we write to

**Description**

May be false positive with !CONFIG\_LOCKDEP/LOCK\_STATE\_UNKNOWN.
May be false positive with !S\_ISREG, because [`file_start_write()`](#c.file_start_write "file_start_write") has
no effect on !S\_ISREG.

bool file\_write\_not\_started(const struct [file](#c.file_write_not_started "file") \*file)
:   check if SB\_FREEZE\_WRITE is not held

**Parameters**

`const struct file *file`
:   the file we write to

**Description**

May be false positive with !CONFIG\_LOCKDEP/LOCK\_STATE\_UNKNOWN.
May be false positive with !S\_ISREG, because [`file_start_write()`](#c.file_start_write "file_start_write") has
no effect on !S\_ISREG.

struct renamedata
:   contains all information required for renaming

**Definition**:

```
struct renamedata {
    struct mnt_idmap *mnt_idmap;
    struct dentry *old_parent;
    struct dentry *old_dentry;
    struct dentry *new_parent;
    struct dentry *new_dentry;
    struct delegated_inode *delegated_inode;
    unsigned int flags;
};
```

**Members**

`mnt_idmap`
:   idmap of the mount in which the rename is happening.

`old_parent`
:   parent of source

`old_dentry`
:   source

`new_parent`
:   parent of destination

`new_dentry`
:   destination

`delegated_inode`
:   returns an inode needing a delegation break

`flags`
:   rename flags

bool is\_mgtime(const struct [inode](#c.is_mgtime "inode") \*inode)
:   is this inode using multigrain timestamps

**Parameters**

`const struct inode *inode`
:   inode to test for multigrain timestamps

**Description**

Return true if the inode uses multigrain timestamps, false otherwise.

bool is\_idmapped\_mnt(const struct vfsmount \*mnt)
:   check whether a mount is mapped

**Parameters**

`const struct vfsmount *mnt`
:   the mount to check

**Description**

If **mnt** has an non **nop\_mnt\_idmap** attached to it then **mnt** is mapped.

**Return**

true if mount is mapped, false if not.

void file\_start\_write(struct [file](#c.file_start_write "file") \*file)
:   get write access to a superblock for regular file io

**Parameters**

`struct file *file`
:   the file we want to write to

**Description**

This is a variant of `sb_start_write()` which is a noop on non-regular file.
Should be matched with a call to [`file_end_write()`](#c.file_end_write "file_end_write").

void file\_end\_write(struct [file](#c.file_end_write "file") \*file)
:   drop write access to a superblock of a regular file

**Parameters**

`struct file *file`
:   the file we wrote to

**Description**

Should be matched with a call to [`file_start_write()`](#c.file_start_write "file_start_write").

void kiocb\_start\_write(struct kiocb \*iocb)
:   get write access to a superblock for async file io

**Parameters**

`struct kiocb *iocb`
:   the io context we want to submit the write with

**Description**

This is a variant of `sb_start_write()` for async io submission.
Should be matched with a call to [`kiocb_end_write()`](#c.kiocb_end_write "kiocb_end_write").

void kiocb\_end\_write(struct kiocb \*iocb)
:   drop write access to a superblock after async file io

**Parameters**

`struct kiocb *iocb`
:   the io context we sumbitted the write with

**Description**

Should be matched with a call to [`kiocb_start_write()`](#c.kiocb_start_write "kiocb_start_write").

bool name\_is\_dot\_dotdot(const char \*name, size\_t len)
:   returns true only if **name** is “.” or “..”

**Parameters**

`const char *name`
:   file name to check

`size_t len`
:   length of file name, in bytes

bool name\_contains\_dotdot(const char \*name)
:   check if a file name contains “..” path components

**Parameters**

`const char *name`
:   File path string to check
    Search for “..” surrounded by either ‘/’ or start/end of string.

void inode\_dio\_begin(struct [inode](#c.inode_dio_begin "inode") \*inode)
:   signal start of a direct I/O requests

**Parameters**

`struct inode *inode`
:   inode the direct I/O happens on

**Description**

This is called once we’ve finished processing a direct I/O request,
and is used to wake up callers waiting for direct I/O to be quiesced.

void inode\_dio\_end(struct [inode](#c.inode_dio_end "inode") \*inode)
:   signal finish of a direct I/O requests

**Parameters**

`struct inode *inode`
:   inode the direct I/O happens on

**Description**

This is called once we’ve finished processing a direct I/O request,
and is used to wake up callers waiting for direct I/O to be quiesced.

bool generic\_ci\_validate\_strict\_name(struct inode \*dir, const struct qstr \*name)
:   Check if a given name is suitable for a directory

**Parameters**

`struct inode *dir`
:   inode of the directory where the new file will be created

`const struct qstr *name`
:   name of the new file

**Description**

This functions checks if the proposed filename is valid for the
parent directory. That means that only valid UTF-8 filenames will be
accepted for casefold directories from filesystems created with the
strict encoding flag. That also means that any name will be
accepted for directories that doesn’t have casefold enabled, or
aren’t being strict with the encoding.

**Return**

* True: if the filename is suitable for this directory. It can be
  true if a given name is not suitable for a strict encoding
  directory, but the directory being used isn’t strict
* False if the filename isn’t suitable for this directory. This only
  happens when a directory is casefolded and the filesystem is strict
  about its encoding.

### The Directory Cache

void d\_drop(struct [dentry](#c.d_drop "dentry") \*dentry)
:   drop a dentry

**Parameters**

`struct dentry *dentry`
:   dentry to drop

**Description**

[`d_drop()`](#c.d_drop "d_drop") unhashes the entry from the parent dentry hashes, so that it won’t
be found through a VFS lookup any more. Note that this is different from
deleting the dentry - d\_delete will try to mark the dentry negative if
possible, giving a successful \_negative\_ lookup, while d\_drop will
just make the cache lookup fail.

[`d_drop()`](#c.d_drop "d_drop") is used mainly for stuff that wants to invalidate a dentry for some
reason (NFS timeouts or autofs deletes).

\_\_d\_drop requires dentry->d\_lock

\_\_\_d\_drop doesn’t mark dentry as “unhashed”
(dentry->d\_hash.pprev will be LIST\_POISON2, not NULL).

struct dentry \*d\_find\_any\_alias(struct [inode](#c.d_find_any_alias "inode") \*inode)
:   find any alias for a given inode

**Parameters**

`struct inode *inode`
:   inode to find an alias for

**Description**

If any aliases exist for the given inode, take and return a
reference for one of them. If no aliases exist, return `NULL`.

struct dentry \*d\_find\_alias(struct [inode](#c.d_find_alias "inode") \*inode)
:   grab a hashed alias of inode

**Parameters**

`struct inode *inode`
:   inode in question

**Description**

If inode has a hashed alias, or is a directory and has any alias,
acquire the reference to alias and return it. Otherwise return NULL.
Notice that if inode is a directory there can be only one alias and
it can be unhashed only if it has no children, or if it is the root
of a filesystem, or if the directory was renamed and d\_revalidate
was the first vfs operation to notice.

If the inode has an IS\_ROOT, DCACHE\_DISCONNECTED alias, then prefer
any other hashed alias over that one.

void d\_dispose\_if\_unused(struct [dentry](#c.d_dispose_if_unused "dentry") \*dentry, struct list\_head \*dispose)
:   move unreferenced dentries to shrink list

**Parameters**

`struct dentry *dentry`
:   dentry in question

`struct list_head *dispose`
:   head of shrink list

**Description**

If dentry has no external references, move it to shrink list.

NOTE!!! The caller is responsible for preventing eviction of the dentry by
holding dentry->d\_inode->i\_lock or equivalent.

void shrink\_dcache\_sb(struct super\_block \*sb)
:   shrink dcache for a superblock

**Parameters**

`struct super_block *sb`
:   superblock

**Description**

Shrink the dcache for the specified super block. This is used to free
the dcache before unmounting a file system.

int path\_has\_submounts(const struct path \*parent)
:   check for mounts over a dentry in the current namespace.

**Parameters**

`const struct path *parent`
:   path to check.

**Description**

Return true if the parent or its subdirectories contain
a mount point in the current namespace.

void d\_invalidate(struct [dentry](#c.d_invalidate "dentry") \*dentry)
:   detach submounts, prune dcache, and drop

**Parameters**

`struct dentry *dentry`
:   dentry to invalidate (aka detach, prune and drop)

struct dentry \*d\_alloc(struct dentry \*parent, const struct qstr \*name)
:   allocate a dcache entry

**Parameters**

`struct dentry * parent`
:   parent of entry to allocate

`const struct qstr *name`
:   qstr of the name

**Description**

Allocates a dentry. It returns `NULL` if there is insufficient memory
available. On a success the dentry is returned. The name passed in is
copied and the copy passed in may be reused after this call.

void d\_instantiate(struct dentry \*entry, struct [inode](#c.d_instantiate "inode") \*inode)
:   fill in inode information for a dentry

**Parameters**

`struct dentry *entry`
:   dentry to complete

`struct inode * inode`
:   inode to attach to this dentry

**Description**

Fill in inode information in the entry.

This turns negative dentries into productive full members
of society.

NOTE! This assumes that the inode count has been incremented
(or otherwise set) by the caller to indicate that it is now
in use by the dcache.

struct dentry \*d\_obtain\_alias(struct [inode](#c.d_obtain_alias "inode") \*inode)
:   find or allocate a DISCONNECTED dentry for a given inode

**Parameters**

`struct inode *inode`
:   inode to allocate the dentry for

**Description**

Obtain a dentry for an inode resulting from NFS filehandle conversion or
similar open by handle operations. The returned dentry may be anonymous,
or may have a full name (if the inode was already in the cache).

When called on a directory inode, we must ensure that the inode only ever
has one dentry. If a dentry is found, that is returned instead of
allocating a new one.

On successful return, the reference to the inode has been transferred
to the dentry. In case of an error the reference on the inode is released.
To make it easier to use in export operations a `NULL` or IS\_ERR inode may
be passed in and the error will be propagated to the return value,
with a `NULL` **inode** replaced by ERR\_PTR(-ESTALE).

struct dentry \*d\_obtain\_root(struct [inode](#c.d_obtain_root "inode") \*inode)
:   find or allocate a dentry for a given inode

**Parameters**

`struct inode *inode`
:   inode to allocate the dentry for

**Description**

Obtain an IS\_ROOT dentry for the root of a filesystem.

We must ensure that directory inodes only ever have one dentry. If a
dentry is found, that is returned instead of allocating a new one.

On successful return, the reference to the inode has been transferred
to the dentry. In case of an error the reference on the inode is
released. A `NULL` or IS\_ERR inode may be passed in and will be the
error will be propagate to the return value, with a `NULL` **inode**
replaced by ERR\_PTR(-ESTALE).

struct [dentry](#c.d_add_ci "dentry") \*d\_add\_ci(struct [dentry](#c.d_add_ci "dentry") \*dentry, struct [inode](#c.d_add_ci "inode") \*inode, struct qstr \*name)
:   lookup or allocate new dentry with case-exact name

**Parameters**

`struct dentry *dentry`
:   the negative dentry that was passed to the parent’s lookup func

`struct inode *inode`
:   the inode case-insensitive lookup has found

`struct qstr *name`
:   the case-exact name to be associated with the returned dentry

**Description**

This is to avoid filling the dcache with case-insensitive names to the
same inode, only the actual correct case is stored in the dcache for
case-insensitive filesystems.

For a case-insensitive lookup match and if the case-exact dentry
already exists in the dcache, use it and return it.

If no entry exists with the exact case name, allocate new dentry with
the exact case, and return the spliced entry.

bool d\_same\_name(const struct [dentry](#c.d_same_name "dentry") \*dentry, const struct [dentry](#c.d_same_name "dentry") \*parent, const struct qstr \*name)
:   compare dentry name with case-exact name

**Parameters**

`const struct dentry *dentry`
:   the negative dentry that was passed to the parent’s lookup func

`const struct dentry *parent`
:   parent dentry

`const struct qstr *name`
:   the case-exact name to be associated with the returned dentry

**Return**

true if names are same, or false

struct dentry \*d\_lookup(const struct dentry \*parent, const struct qstr \*name)
:   search for a dentry

**Parameters**

`const struct dentry *parent`
:   parent dentry

`const struct qstr *name`
:   qstr of name we wish to find

**Return**

dentry, or NULL

**Description**

d\_lookup searches the children of the parent dentry for the name in
question. If the dentry is found its reference count is incremented and the
dentry is returned. The caller must use dput to free the entry when it has
finished using it. `NULL` is returned if the dentry does not exist.

void d\_delete(struct [dentry](#c.d_delete "dentry") \*dentry)
:   delete a dentry

**Parameters**

`struct dentry * dentry`
:   The dentry to delete

**Description**

Turn the dentry into a negative dentry if possible, otherwise
remove it from the hash queues so it can be deleted later

void d\_rehash(struct dentry \*entry)
:   add an entry back to the hash

**Parameters**

`struct dentry * entry`
:   dentry to add to the hash

**Description**

Adds a dentry to the hash according to its name.

void d\_add(struct dentry \*entry, struct [inode](#c.d_add "inode") \*inode)
:   add dentry to hash queues

**Parameters**

`struct dentry *entry`
:   dentry to add

`struct inode *inode`
:   The inode to attach to this dentry

**Description**

This adds the entry to the hash queues and initializes **inode**.
The entry was actually filled in earlier during [`d_alloc()`](#c.d_alloc "d_alloc").

struct [dentry](#c.d_splice_alias "dentry") \*d\_splice\_alias(struct [inode](#c.d_splice_alias "inode") \*inode, struct [dentry](#c.d_splice_alias "dentry") \*dentry)
:   splice a disconnected dentry into the tree if one exists

**Parameters**

`struct inode *inode`
:   the inode which may have a disconnected dentry

`struct dentry *dentry`
:   a negative dentry which we want to point to the inode.

**Description**

If inode is a directory and has an IS\_ROOT alias, then d\_move that in
place of the given dentry and return it, else simply d\_add the inode
to the dentry and return NULL.

If a non-IS\_ROOT directory is found, the filesystem is corrupt, and
we should error out: directories can’t have multiple aliases.

This is needed in the lookup routine of any filesystem that is exportable
(via knfsd) so that we can build dcache paths to directories effectively.

If a dentry was found and moved, then it is returned. Otherwise NULL
is returned. This matches the expected return value of ->lookup.

Cluster filesystems may call this function with a negative, hashed dentry.
In that case, we know that the inode will be a regular file, and also this
will only occur during atomic\_open. So we need to check for the dentry
being already hashed only in the final case.

bool is\_subdir(struct dentry \*new\_dentry, struct dentry \*old\_dentry)
:   is new dentry a subdirectory of old\_dentry

**Parameters**

`struct dentry *new_dentry`
:   new dentry

`struct dentry *old_dentry`
:   old dentry

**Description**

Returns true if new\_dentry is a subdirectory of the parent (at any depth).
Returns false otherwise.
Caller must ensure that “new\_dentry” is pinned before calling [`is_subdir()`](#c.is_subdir "is_subdir")

struct [dentry](#c.dget_dlock "dentry") \*dget\_dlock(struct [dentry](#c.dget_dlock "dentry") \*dentry)
:   get a reference to a dentry

**Parameters**

`struct dentry *dentry`
:   dentry to get a reference to

**Description**

Given a live dentry, increment the reference count and return the dentry.
Caller must hold **dentry->d\_lock**. Making sure that dentry is alive is
caller’s resonsibility. There are many conditions sufficient to guarantee
that; e.g. anything with non-negative refcount is alive, so’s anything
hashed, anything positive, anyone’s parent, etc.

struct [dentry](#c.dget "dentry") \*dget(struct [dentry](#c.dget "dentry") \*dentry)
:   get a reference to a dentry

**Parameters**

`struct dentry *dentry`
:   dentry to get a reference to

**Description**

Given a dentry or `NULL` pointer increment the reference count
if appropriate and return the dentry. A dentry will not be
destroyed when it has references. Conversely, a dentry with
no references can disappear for any number of reasons, starting
with memory pressure. In other words, that primitive is
used to clone an existing reference; using it on something with
zero refcount is a bug.

**NOTE**

it will spin if **dentry->d\_lock** is held. From the deadlock
avoidance point of view it is equivalent to `spin_lock()`/increment
refcount/`spin_unlock()`, so calling it under **dentry->d\_lock** is
always a bug; so’s calling it under ->d\_lock on any of its descendents.

int d\_unhashed(const struct [dentry](#c.d_unhashed "dentry") \*dentry)
:   is dentry hashed

**Parameters**

`const struct dentry *dentry`
:   entry to check

**Description**

Returns true if the dentry passed is not currently hashed.

bool d\_really\_is\_negative(const struct [dentry](#c.d_really_is_negative "dentry") \*dentry)
:   Determine if a dentry is really negative (ignoring fallthroughs)

**Parameters**

`const struct dentry *dentry`
:   The dentry in question

**Description**

Returns true if the dentry represents either an absent name or a name that
doesn’t map to an inode (ie. ->d\_inode is NULL). The dentry could represent
a true miss, a whiteout that isn’t represented by a 0,0 chardev or a
fallthrough marker in an opaque directory.

Note! (1) This should be used *only* by a filesystem to examine its own
dentries. It should not be used to look at some other filesystem’s
dentries. (2) It should also be used in combination with [`d_inode()`](#c.d_inode "d_inode") to get
the inode. (3) The dentry may have something attached to ->d\_lower and the
type field of the flags may be set to something other than miss or whiteout.

bool d\_really\_is\_positive(const struct [dentry](#c.d_really_is_positive "dentry") \*dentry)
:   Determine if a dentry is really positive (ignoring fallthroughs)

**Parameters**

`const struct dentry *dentry`
:   The dentry in question

**Description**

Returns true if the dentry represents a name that maps to an inode
(ie. ->d\_inode is not NULL). The dentry might still represent a whiteout if
that is represented on medium as a 0,0 chardev.

Note! (1) This should be used *only* by a filesystem to examine its own
dentries. It should not be used to look at some other filesystem’s
dentries. (2) It should also be used in combination with [`d_inode()`](#c.d_inode "d_inode") to get
the inode.

struct inode \*d\_inode(const struct [dentry](#c.d_inode "dentry") \*dentry)
:   Get the actual inode of this dentry

**Parameters**

`const struct dentry *dentry`
:   The dentry to query

**Description**

This is the helper normal filesystems should use to get at their own inodes
in their own dentries and ignore the layering superimposed upon them.

struct inode \*d\_inode\_rcu(const struct [dentry](#c.d_inode_rcu "dentry") \*dentry)
:   Get the actual inode of this dentry with `READ_ONCE()`

**Parameters**

`const struct dentry *dentry`
:   The dentry to query

**Description**

This is the helper normal filesystems should use to get at their own inodes
in their own dentries and ignore the layering superimposed upon them.

struct inode \*d\_backing\_inode(const struct dentry \*upper)
:   Get upper or lower inode we should be using

**Parameters**

`const struct dentry *upper`
:   The upper layer

**Description**

This is the helper that should be used to get at the inode that will be used
if this dentry were to be opened as a file. The inode may be on the upper
dentry or it may be on a lower dentry pinned by the upper.

Normal filesystems should not use this to access their own inodes.

struct [dentry](#c.d_real "dentry") \*d\_real(struct [dentry](#c.d_real "dentry") \*dentry, enum d\_real\_type type)
:   Return the real dentry

**Parameters**

`struct dentry *dentry`
:   the dentry to query

`enum d_real_type type`
:   the type of real dentry (data or metadata)

**Description**

If dentry is on a union/overlay, then return the underlying, real dentry.
Otherwise return the dentry itself.

See also: [Overview of the Linux Virtual File System](vfs.html)

struct inode \*d\_real\_inode(const struct [dentry](#c.d_real_inode "dentry") \*dentry)
:   Return the real inode hosting the data

**Parameters**

`const struct dentry *dentry`
:   The dentry to query

**Description**

If dentry is on a union/overlay, then return the underlying, real inode.
Otherwise return [`d_inode()`](#c.d_inode "d_inode").

### Inode Handling

int inode\_init\_always\_gfp(struct super\_block \*sb, struct [inode](#c.inode_init_always_gfp "inode") \*inode, gfp\_t gfp)
:   perform inode structure initialisation

**Parameters**

`struct super_block *sb`
:   superblock inode belongs to

`struct inode *inode`
:   inode to initialise

`gfp_t gfp`
:   allocation flags

**Description**

These are initializations that need to be done on every inode
allocation as the fields are not initialised by slab allocation.
If there are additional allocations required **gfp** is used.

void drop\_nlink(struct [inode](#c.drop_nlink "inode") \*inode)
:   directly drop an inode’s link count

**Parameters**

`struct inode *inode`
:   inode

**Description**

This is a low-level filesystem helper to replace any
direct filesystem manipulation of i\_nlink. In cases
where we are attempting to track writes to the
filesystem, a decrement to zero means an imminent
write when the file is truncated and actually unlinked
on the filesystem.

void clear\_nlink(struct [inode](#c.clear_nlink "inode") \*inode)
:   directly zero an inode’s link count

**Parameters**

`struct inode *inode`
:   inode

**Description**

This is a low-level filesystem helper to replace any
direct filesystem manipulation of i\_nlink. See
[`drop_nlink()`](#c.drop_nlink "drop_nlink") for why we care about i\_nlink hitting zero.

void set\_nlink(struct [inode](#c.set_nlink "inode") \*inode, unsigned int nlink)
:   directly set an inode’s link count

**Parameters**

`struct inode *inode`
:   inode

`unsigned int nlink`
:   new nlink (should be non-zero)

**Description**

This is a low-level filesystem helper to replace any
direct filesystem manipulation of i\_nlink.

void inc\_nlink(struct [inode](#c.inc_nlink "inode") \*inode)
:   directly increment an inode’s link count

**Parameters**

`struct inode *inode`
:   inode

**Description**

This is a low-level filesystem helper to replace any
direct filesystem manipulation of i\_nlink. Currently,
it is only here for parity with `dec_nlink()`.

void inode\_sb\_list\_add(struct [inode](#c.inode_sb_list_add "inode") \*inode)
:   add inode to the superblock list of inodes

**Parameters**

`struct inode *inode`
:   inode to add

void \_\_insert\_inode\_hash(struct [inode](#c.__insert_inode_hash "inode") \*inode, u64 hashval)
:   hash an inode

**Parameters**

`struct inode *inode`
:   unhashed inode

`u64 hashval`
:   u64 value used to locate this object in the
    inode\_hashtable.

**Description**

> Add an inode to the inode hash for this superblock.

void \_\_remove\_inode\_hash(struct [inode](#c.__remove_inode_hash "inode") \*inode)
:   remove an inode from the hash

**Parameters**

`struct inode *inode`
:   inode to unhash

**Description**

> Remove an inode from the superblock.

void evict\_inodes(struct super\_block \*sb)
:   evict all evictable inodes for a superblock

**Parameters**

`struct super_block *sb`
:   superblock to operate on

**Description**

Make sure that no inodes with zero refcount are retained. This is
called by superblock shutdown after having SB\_ACTIVE flag removed,
so any inode reaching zero refcount during or after that call will
be immediately evicted.

struct inode \*new\_inode(struct super\_block \*sb)
:   obtain an inode

**Parameters**

`struct super_block *sb`
:   superblock

**Description**

> Allocates a new inode for given superblock. The default gfp\_mask
> for allocations related to inode->i\_mapping is GFP\_HIGHUSER\_MOVABLE.
> If HIGHMEM pages are unsuitable or it is known that pages allocated
> for the page cache are not reclaimable or migratable,
> `mapping_set_gfp_mask()` must be called with suitable flags on the
> newly created inode’s mapping

void unlock\_new\_inode(struct [inode](#c.unlock_new_inode "inode") \*inode)
:   clear the I\_NEW state and wake up any waiters

**Parameters**

`struct inode *inode`
:   new inode to unlock

**Description**

Called when the inode is fully initialised to clear the new state of the
inode and wake up anyone waiting for the inode to finish initialisation.

void lock\_two\_nondirectories(struct inode \*inode1, struct inode \*inode2)
:   take two i\_mutexes on non-directory objects

**Parameters**

`struct inode *inode1`
:   first inode to lock

`struct inode *inode2`
:   second inode to lock

**Description**

Lock any non-NULL argument. Passed objects must not be directories.
Zero, one or two objects may be locked by this function.

void unlock\_two\_nondirectories(struct inode \*inode1, struct inode \*inode2)
:   release locks from [`lock_two_nondirectories()`](#c.lock_two_nondirectories "lock_two_nondirectories")

**Parameters**

`struct inode *inode1`
:   first inode to unlock

`struct inode *inode2`
:   second inode to unlock

struct [inode](#c.inode_insert5 "inode") \*inode\_insert5(struct [inode](#c.inode_insert5 "inode") \*inode, u64 hashval, int (\*test)(struct [inode](#c.inode_insert5 "inode")\*, void\*), int (\*set)(struct [inode](#c.inode_insert5 "inode")\*, void\*), void \*data)
:   obtain an inode from a mounted file system

**Parameters**

`struct inode *inode`
:   pre-allocated inode to use for insert to cache

`u64 hashval`
:   hash value (usually inode number) to get

`int (*test)(struct inode *, void *)`
:   callback used for comparisons between inodes

`int (*set)(struct inode *, void *)`
:   callback used to initialize a new `struct inode`

`void *data`
:   opaque data pointer to pass to **test** and **set**

**Description**

Search for the inode specified by **hashval** and **data** in the inode cache,
and if present return it with an increased reference count. This is a
variant of [`iget5_locked()`](#c.iget5_locked "iget5_locked") that doesn’t allocate an inode.

If the inode is not present in the cache, insert the pre-allocated inode and
return it locked, hashed, and with the I\_NEW flag set. The file system gets
to fill it in before unlocking it via [`unlock_new_inode()`](#c.unlock_new_inode "unlock_new_inode").

Note that both **test** and **set** are called with the inode\_hash\_lock held, so
they can’t sleep.

struct inode \*iget5\_locked(struct super\_block \*sb, u64 hashval, int (\*test)(struct inode\*, void\*), int (\*set)(struct inode\*, void\*), void \*data)
:   obtain an inode from a mounted file system

**Parameters**

`struct super_block *sb`
:   super block of file system

`u64 hashval`
:   hash value (usually inode number) to get

`int (*test)(struct inode *, void *)`
:   callback used for comparisons between inodes

`int (*set)(struct inode *, void *)`
:   callback used to initialize a new `struct inode`

`void *data`
:   opaque data pointer to pass to **test** and **set**

**Description**

Search for the inode specified by **hashval** and **data** in the inode cache,
and if present return it with an increased reference count. This is a
generalized version of [`iget_locked()`](#c.iget_locked "iget_locked") for file systems where the inode
number is not sufficient for unique identification of an inode.

If the inode is not present in the cache, allocate and insert a new inode
and return it locked, hashed, and with the I\_NEW flag set. The file system
gets to fill it in before unlocking it via [`unlock_new_inode()`](#c.unlock_new_inode "unlock_new_inode").

Note that both **test** and **set** are called with the inode\_hash\_lock held, so
they can’t sleep.

struct inode \*iget5\_locked\_rcu(struct super\_block \*sb, u64 hashval, int (\*test)(struct inode\*, void\*), int (\*set)(struct inode\*, void\*), void \*data)
:   obtain an inode from a mounted file system

**Parameters**

`struct super_block *sb`
:   super block of file system

`u64 hashval`
:   hash value (usually inode number) to get

`int (*test)(struct inode *, void *)`
:   callback used for comparisons between inodes

`int (*set)(struct inode *, void *)`
:   callback used to initialize a new `struct inode`

`void *data`
:   opaque data pointer to pass to **test** and **set**

**Description**

This is equivalent to iget5\_locked, except the **test** callback must
tolerate the inode not being stable, including being mid-teardown.

struct inode \*iget\_locked(struct super\_block \*sb, u64 ino)
:   obtain an inode from a mounted file system

**Parameters**

`struct super_block *sb`
:   super block of file system

`u64 ino`
:   inode number to get

**Description**

Search for the inode specified by **ino** in the inode cache and if present
return it with an increased reference count. This is for file systems
where the inode number is sufficient for unique identification of an inode.

If the inode is not in cache, allocate a new inode and return it locked,
hashed, and with the I\_NEW flag set. The file system gets to fill it in
before unlocking it via [`unlock_new_inode()`](#c.unlock_new_inode "unlock_new_inode").

ino\_t iunique(struct super\_block \*sb, ino\_t max\_reserved)
:   get a unique inode number

**Parameters**

`struct super_block *sb`
:   superblock

`ino_t max_reserved`
:   highest reserved inode number

**Description**

> Obtain an inode number that is unique on the system for a given
> superblock. This is used by file systems that have no natural
> permanent inode numbering system. An inode number is returned that
> is higher than the reserved limit but unique.
>
> BUGS:
> With a large number of inodes live on the file system this function
> currently becomes quite slow.

struct inode \*ilookup5\_nowait(struct super\_block \*sb, u64 hashval, int (\*test)(struct inode\*, void\*), void \*data, bool \*isnew)
:   search for an inode in the inode cache

**Parameters**

`struct super_block *sb`
:   super block of file system to search

`u64 hashval`
:   hash value (usually inode number) to search for

`int (*test)(struct inode *, void *)`
:   callback used for comparisons between inodes

`void *data`
:   opaque data pointer to pass to **test**

`bool *isnew`
:   return argument telling whether I\_NEW was set when
    the inode was found in hash (the caller needs to
    wait for I\_NEW to clear)

**Description**

Search for the inode specified by **hashval** and **data** in the inode cache.
If the inode is in the cache, the inode is returned with an incremented
reference count.

**Note**

I\_NEW is not waited upon so you have to be very careful what you do
with the returned inode. You probably should be using [`ilookup5()`](#c.ilookup5 "ilookup5") instead.

Note2: **test** is called with the inode\_hash\_lock held, so can’t sleep.

struct inode \*ilookup5(struct super\_block \*sb, u64 hashval, int (\*test)(struct inode\*, void\*), void \*data)
:   search for an inode in the inode cache

**Parameters**

`struct super_block *sb`
:   super block of file system to search

`u64 hashval`
:   hash value (usually inode number) to search for

`int (*test)(struct inode *, void *)`
:   callback used for comparisons between inodes

`void *data`
:   opaque data pointer to pass to **test**

**Description**

Search for the inode specified by **hashval** and **data** in the inode cache,
and if the inode is in the cache, return the inode with an incremented
reference count. Waits on I\_NEW before returning the inode.
returned with an incremented reference count.

This is a generalized version of [`ilookup()`](#c.ilookup "ilookup") for file systems where the
inode number is not sufficient for unique identification of an inode.

**Note**

**test** is called with the inode\_hash\_lock held, so can’t sleep.

struct inode \*ilookup(struct super\_block \*sb, u64 ino)
:   search for an inode in the inode cache

**Parameters**

`struct super_block *sb`
:   super block of file system to search

`u64 ino`
:   inode number to search for

**Description**

Search for the inode **ino** in the inode cache, and if the inode is in the
cache, the inode is returned with an incremented reference count.

struct inode \*find\_inode\_nowait(struct super\_block \*sb, u64 hashval, int (\*match)(struct inode\*, u64, void\*), void \*data)
:   find an inode in the inode cache

**Parameters**

`struct super_block *sb`
:   super block of file system to search

`u64 hashval`
:   hash value (usually inode number) to search for

`int (*match)(struct inode *, u64, void *)`
:   callback used for comparisons between inodes

`void *data`
:   opaque data pointer to pass to **match**

**Description**

Search for the inode specified by **hashval** and **data** in the inode
cache, where the helper function **match** will return 0 if the inode
does not match, 1 if the inode does match, and -1 if the search
should be stopped. The **match** function must be responsible for
taking the i\_lock spin\_lock and checking i\_state for an inode being
freed or being initialized, and incrementing the reference count
before returning 1. It also must not sleep, since it is called with
the inode\_hash\_lock spinlock held.

This is a even more generalized version of [`ilookup5()`](#c.ilookup5 "ilookup5") when the
function must never block --- `find_inode()` can block in
`__wait_on_freeing_inode()` --- or when the caller can not increment
the reference count because the resulting [`iput()`](#c.iput "iput") might cause an
inode eviction. The tradeoff is that the **match** funtion must be
very carefully implemented.

struct inode \*find\_inode\_rcu(struct super\_block \*sb, u64 hashval, int (\*test)(struct inode\*, void\*), void \*data)
:   find an inode in the inode cache

**Parameters**

`struct super_block *sb`
:   Super block of file system to search

`u64 hashval`
:   Key to hash

`int (*test)(struct inode *, void *)`
:   Function to test match on an inode

`void *data`
:   Data for test function

**Description**

Search for the inode specified by **hashval** and **data** in the inode cache,
where the helper function **test** will return 0 if the inode does not match
and 1 if it does. The **test** function must be responsible for taking the
i\_lock spin\_lock and checking i\_state for an inode being freed or being
initialized.

If successful, this will return the inode for which the **test** function
returned 1 and NULL otherwise.

The **test** function is not permitted to take a ref on any inode presented.
It is also not permitted to sleep.

The caller must hold the RCU read lock.

struct inode \*find\_inode\_by\_ino\_rcu(struct super\_block \*sb, u64 ino)
:   Find an inode in the inode cache

**Parameters**

`struct super_block *sb`
:   Super block of file system to search

`u64 ino`
:   The inode number to match

**Description**

Search for the inode specified by **hashval** and **data** in the inode cache,
where the helper function **test** will return 0 if the inode does not match
and 1 if it does. The **test** function must be responsible for taking the
i\_lock spin\_lock and checking i\_state for an inode being freed or being
initialized.

If successful, this will return the inode for which the **test** function
returned 1 and NULL otherwise.

The **test** function is not permitted to take a ref on any inode presented.
It is also not permitted to sleep.

The caller must hold the RCU read lock.

void iput(struct [inode](#c.iput "inode") \*inode)
:   put an inode

**Parameters**

`struct inode *inode`
:   inode to put

**Description**

> Puts an inode, dropping its usage count. If the inode use count hits
> zero, the inode is then freed and may also be destroyed.
>
> Consequently, [`iput()`](#c.iput "iput") can sleep.

void iput\_not\_last(struct [inode](#c.iput_not_last "inode") \*inode)
:   put an inode assuming this is not the last reference

**Parameters**

`struct inode *inode`
:   inode to put

int bmap(struct [inode](#c.bmap "inode") \*inode, sector\_t \*block)
:   find a block number in a file

**Parameters**

`struct inode *inode`
:   inode owning the block number being requested

`sector_t *block`
:   pointer containing the block to find

**Description**

> Replaces the value in `*block` with the block number on the device holding
> corresponding to the requested block number in the file.
> That is, asked for block 4 of inode 1 the function will replace the
> 4 in `*block`, with disk block relative to the disk start that holds that
> block of the file.
>
> Returns -EINVAL in case of error, 0 otherwise. If mapping falls into a
> hole, returns 0 and `*block` is also set to 0.

int inode\_update\_time(struct [inode](#c.inode_update_time "inode") \*inode, enum fs\_update\_time type, unsigned int flags)
:   update either atime or c/mtime and i\_version on the inode

**Parameters**

`struct inode *inode`
:   inode to be updated

`enum fs_update_time type`
:   timestamp to be updated

`unsigned int flags`
:   flags for the update

**Description**

Update either atime or c/mtime and version in a inode if needed for a file
access or modification. It is up to the caller to mark the inode dirty
appropriately.

Returns the positive I\_DIRTY\_\* flags for [`__mark_inode_dirty()`](#c.__mark_inode_dirty "__mark_inode_dirty") if the inode
needs to be marked dirty, 0 if it did not, or a negative errno if an error
happened.

int generic\_update\_time(struct [inode](#c.generic_update_time "inode") \*inode, enum fs\_update\_time type, unsigned int flags)
:   update the timestamps on the inode

**Parameters**

`struct inode *inode`
:   inode to be updated

`enum fs_update_time type`
:   timestamp to be updated

`unsigned int flags`
:   flags for the update

**Description**

Returns a negative error value on error, else 0.

int file\_remove\_privs(struct [file](#c.file_remove_privs "file") \*file)
:   remove special file privileges (suid, capabilities)

**Parameters**

`struct file *file`
:   file to remove privileges from

**Description**

When file is modified by a write or truncation ensure that special
file privileges are removed.

**Return**

0 on success, negative errno on failure.

struct timespec64 current\_time(struct [inode](#c.current_time "inode") \*inode)
:   Return FS time (possibly fine-grained)

**Parameters**

`struct inode *inode`
:   inode.

**Description**

Return the current time truncated to the time granularity supported by
the fs, as suitable for a ctime/mtime change. If the ctime is flagged
as having been QUERIED, get a fine-grained timestamp, but don’t update
the floor.

For a multigrain inode, this is effectively an estimate of the timestamp
that a file would receive. An actual update must go through
[`inode_set_ctime_current()`](#c.inode_set_ctime_current "inode_set_ctime_current").

int file\_update\_time(struct [file](#c.file_update_time "file") \*file)
:   update mtime and ctime time

**Parameters**

`struct file *file`
:   file accessed

**Description**

Update the mtime and ctime members of an inode and mark the inode for
writeback. Note that this function is meant exclusively for usage in
the file write path of filesystems, and filesystems may choose to
explicitly ignore updates via this function with the \_NOCMTIME inode
flag, e.g. for network filesystem where these imestamps are handled
by the server. This can return an error for file systems who need to
allocate space in order to update an inode.

**Return**

0 on success, negative errno on failure.

int file\_modified(struct [file](#c.file_modified "file") \*file)
:   handle mandated vfs changes when modifying a file

**Parameters**

`struct file *file`
:   file that was modified

**Description**

When file has been modified ensure that special
file privileges are removed and time settings are updated.

**Context**

Caller must hold the file’s inode lock.

**Return**

0 on success, negative errno on failure.

int kiocb\_modified(struct kiocb \*iocb)
:   handle mandated vfs changes when modifying a file

**Parameters**

`struct kiocb *iocb`
:   iocb that was modified

**Description**

When file has been modified ensure that special
file privileges are removed and time settings are updated.

**Context**

Caller must hold the file’s inode lock.

**Return**

0 on success, negative errno on failure.

void inode\_init\_owner(struct mnt\_idmap \*idmap, struct [inode](#c.inode_init_owner "inode") \*inode, const struct [inode](#c.inode_init_owner "inode") \*dir, umode\_t mode)
:   Init uid,gid,mode for new inode according to posix standards

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was created from

`struct inode *inode`
:   New inode

`const struct inode *dir`
:   Directory inode

`umode_t mode`
:   mode of the new inode

**Description**

If the inode has been created through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions
and initializing i\_uid and i\_gid. On non-idmapped mounts or if permission
checking is to be performed on the raw inode simply pass **nop\_mnt\_idmap**.

bool inode\_owner\_or\_capable(struct mnt\_idmap \*idmap, const struct [inode](#c.inode_owner_or_capable "inode") \*inode)
:   check current task permissions to inode

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct inode *inode`
:   inode being checked

**Description**

Return true if current either has CAP\_FOWNER in a namespace with the
inode owner uid mapped, or owns the file.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

void inode\_dio\_wait(struct [inode](#c.inode_dio_wait "inode") \*inode)
:   wait for outstanding DIO requests to finish

**Parameters**

`struct inode *inode`
:   inode to wait for

**Description**

Waits for all pending direct I/O requests to finish so that we can
proceed with a truncate or equivalent operation.

Must be called under a lock that serializes taking new references
to i\_dio\_count, usually by inode->i\_rwsem.

struct timespec64 timestamp\_truncate(struct timespec64 t, struct [inode](#c.timestamp_truncate "inode") \*inode)
:   Truncate timespec to a granularity

**Parameters**

`struct timespec64 t`
:   Timespec

`struct inode *inode`
:   inode being updated

**Description**

Truncate a timespec to the granularity supported by the fs
containing the inode. Always rounds down. gran must
not be 0 nor greater than a second (NSEC\_PER\_SEC, or 10^9 ns).

struct timespec64 inode\_set\_ctime\_current(struct [inode](#c.inode_set_ctime_current "inode") \*inode)
:   set the ctime to current\_time

**Parameters**

`struct inode *inode`
:   inode

**Description**

Set the inode’s ctime to the current value for the inode. Returns the
current value that was assigned. If this is not a multigrain inode, then we
set it to the later of the coarse time and floor value.

If it is multigrain, then we first see if the coarse-grained timestamp is
distinct from what is already there. If so, then use that. Otherwise, get a
fine-grained timestamp.

After that, try to swap the new value into i\_ctime\_nsec. Accept the
resulting ctime, regardless of the outcome of the swap. If it has
already been replaced, then that timestamp is later than the earlier
unacceptable one, and is thus acceptable.

struct timespec64 inode\_set\_ctime\_deleg(struct [inode](#c.inode_set_ctime_deleg "inode") \*inode, struct timespec64 update)
:   try to update the ctime on a delegated inode

**Parameters**

`struct inode *inode`
:   inode to update

`struct timespec64 update`
:   timespec64 to set the ctime

**Description**

Attempt to atomically update the ctime on behalf of a delegation holder.

The nfs server can call back the holder of a delegation to get updated
inode attributes, including the mtime. When updating the mtime, update
the ctime to a value at least equal to that.

This can race with concurrent updates to the inode, in which
case the update is skipped.

Note that this works even when multigrain timestamps are not enabled,
so it is used in either case.

bool in\_group\_or\_capable(struct mnt\_idmap \*idmap, const struct [inode](#c.in_group_or_capable "inode") \*inode, vfsgid\_t vfsgid)
:   check whether caller is CAP\_FSETID privileged

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount **inode** was found from

`const struct inode *inode`
:   inode to check

`vfsgid_t vfsgid`
:   the new/current vfsgid of **inode**

**Description**

Check whether **vfsgid** is in the caller’s group list or if the caller is
privileged with CAP\_FSETID over **inode**. This can be used to determine
whether the setgid bit can be kept or must be dropped.

**Return**

true if the caller is sufficiently privileged, false if not.

umode\_t mode\_strip\_sgid(struct mnt\_idmap \*idmap, const struct inode \*dir, umode\_t mode)
:   handle the sgid bit for non-directories

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was created from

`const struct inode *dir`
:   parent directory inode

`umode_t mode`
:   mode of the file to be created in **dir**

**Description**

If the **mode** of the new file has both the S\_ISGID and S\_IXGRP bit
raised and **dir** has the S\_ISGID bit raised ensure that the caller is
either in the group of the parent directory or they have CAP\_FSETID
in their user namespace and are privileged over the parent directory.
In all other cases, strip the S\_ISGID bit from **mode**.

**Return**

the new mode to use for the file

void dump\_inode(struct [inode](#c.dump_inode "inode") \*inode, const char \*reason)
:   dump an inode.

**Parameters**

`struct inode *inode`
:   inode to dump

`const char *reason`
:   reason for dumping

**Description**

If inode is an invalid pointer, we don’t want to crash accessing it,
so probe everything depending on it carefully with `get_kernel_nofault()`.

void make\_bad\_inode(struct [inode](#c.make_bad_inode "inode") \*inode)
:   mark an inode bad due to an I/O error

**Parameters**

`struct inode *inode`
:   Inode to mark bad

**Description**

> When an inode cannot be read due to a media or remote network
> failure this function makes the inode “bad” and causes I/O operations
> on it to fail from this point on.

bool is\_bad\_inode(struct [inode](#c.is_bad_inode "inode") \*inode)
:   is an inode errored

**Parameters**

`struct inode *inode`
:   inode to test

**Description**

> Returns true if the inode in question has been marked as bad.

void iget\_failed(struct [inode](#c.iget_failed "inode") \*inode)
:   Mark an under-construction inode as dead and release it

**Parameters**

`struct inode *inode`
:   The inode to discard

**Description**

Mark an under-construction inode as dead and release it.

### Registration and Superblocks

void deactivate\_locked\_super(struct super\_block \*s)
:   drop an active reference to superblock

**Parameters**

`struct super_block *s`
:   superblock to deactivate

**Description**

> Drops an active reference to superblock, converting it into a temporary
> one if there is no other active references left. In that case we
> tell fs driver to shut it down and drop the temporary reference we
> had just acquired.
>
> Caller holds exclusive lock on superblock; that lock is released.

void deactivate\_super(struct super\_block \*s)
:   drop an active reference to superblock

**Parameters**

`struct super_block *s`
:   superblock to deactivate

**Description**

> Variant of [`deactivate_locked_super()`](#c.deactivate_locked_super "deactivate_locked_super"), except that superblock is *not*
> locked by caller. If we are going to drop the final active reference,
> lock will be acquired prior to that.

void retire\_super(struct super\_block \*sb)
:   prevents superblock from being reused

**Parameters**

`struct super_block *sb`
:   superblock to retire

**Description**

> The function marks superblock to be ignored in superblock test, which
> prevents it from being reused for any new mounts. If the superblock has
> a private bdi, it also unregisters it, but doesn’t reduce the refcount
> of the superblock to prevent potential races. The refcount is reduced
> by [`generic_shutdown_super()`](#c.generic_shutdown_super "generic_shutdown_super"). The function can not be called
> concurrently with [`generic_shutdown_super()`](#c.generic_shutdown_super "generic_shutdown_super"). It is safe to call the
> function multiple times, subsequent calls have no effect.
>
> The marker will affect the re-use only for block-device-based
> superblocks. Other superblocks will still get marked if this function
> is used, but that will not affect their reusability.

void generic\_shutdown\_super(struct super\_block \*sb)
:   common helper for ->`kill_sb()`

**Parameters**

`struct super_block *sb`
:   superblock to kill

**Description**

> [`generic_shutdown_super()`](#c.generic_shutdown_super "generic_shutdown_super") does all fs-independent work on superblock
> shutdown. Typical ->`kill_sb()` should pick all fs-specific objects
> that need destruction out of superblock, call [`generic_shutdown_super()`](#c.generic_shutdown_super "generic_shutdown_super")
> and release aforementioned objects. Note: dentries and inodes \_are\_
> taken care of and do not need specific handling.
>
> Upon calling this function, the filesystem may no longer alter or
> rearrange the set of dentries belonging to this super\_block, nor may it
> change the attachments of dentries to inodes.

struct super\_block \*sget\_fc(struct fs\_context \*fc, int (\*test)(struct super\_block\*, struct fs\_context\*), int (\*set)(struct super\_block\*, struct fs\_context\*))
:   Find or create a superblock

**Parameters**

`struct fs_context *fc`
:   Filesystem context.

`int (*test)(struct super_block *, struct fs_context *)`
:   Comparison callback

`int (*set)(struct super_block *, struct fs_context *)`
:   Setup callback

**Description**

Create a new superblock or find an existing one.

The **test** callback is used to find a matching existing superblock.
Whether or not the requested parameters in **fc** are taken into account
is specific to the **test** callback that is used. They may even be
completely ignored.

If an extant superblock is matched, it will be returned unless:

1. the namespace the filesystem context **fc** and the extant
   superblock’s namespace differ
2. the filesystem context **fc** has requested that reusing an extant
   superblock is not allowed

In both cases EBUSY will be returned.

If no match is made, a new superblock will be allocated and basic
initialisation will be performed (s\_type, s\_fs\_info and s\_id will be
set and the **set** callback will be invoked), the superblock will be
published and it will be returned in a partially constructed state
with SB\_BORN and SB\_ACTIVE as yet unset.

**Return**

On success, an extant or newly created superblock is
returned. On failure an error pointer is returned.

struct super\_block \*sget(struct file\_system\_type \*type, int (\*test)(struct super\_block\*, void\*), int (\*set)(struct super\_block\*, void\*), int flags, void \*data)
:   find or create a superblock

**Parameters**

`struct file_system_type *type`
:   filesystem type superblock should belong to

`int (*test)(struct super_block *,void *)`
:   comparison callback

`int (*set)(struct super_block *,void *)`
:   setup callback

`int flags`
:   mount flags

`void *data`
:   argument to each of them

void iterate\_supers\_type(struct file\_system\_type \*type, void (\*f)(struct super\_block\*, void\*), void \*arg)
:   call function for superblocks of given type

**Parameters**

`struct file_system_type *type`
:   fs type

`void (*f)(struct super_block *, void *)`
:   function to call

`void *arg`
:   argument to pass to it

**Description**

> Scans the superblock list and calls given function, passing it
> locked superblock and given argument.

int get\_anon\_bdev(dev\_t \*p)
:   Allocate a block device for filesystems which don’t have one.

**Parameters**

`dev_t *p`
:   Pointer to a dev\_t.

**Description**

Filesystems which don’t use real block devices can call this function
to allocate a virtual block device.

**Context**

Any context. Frequently called while holding sb\_lock.

**Return**

0 on success, -EMFILE if there are no anonymous bdevs left
or -ENOMEM if memory allocation failed.

struct super\_block \*sget\_dev(struct fs\_context \*fc, dev\_t dev)
:   Find or create a superblock by device number

**Parameters**

`struct fs_context *fc`
:   Filesystem context.

`dev_t dev`
:   device number

**Description**

Find or create a superblock using the provided device number that
will be stored in fc->sget\_key.

If an extant superblock is matched, then that will be returned with
an elevated reference count that the caller must transfer or discard.

If no match is made, a new superblock will be allocated and basic
initialisation will be performed (s\_type, s\_fs\_info, s\_id, s\_dev will
be set). The superblock will be published and it will be returned in
a partially constructed state with SB\_BORN and SB\_ACTIVE as yet
unset.

**Return**

an existing or newly created superblock on success, an error
pointer on failure.

int get\_tree\_bdev\_flags(struct fs\_context \*fc, int (\*fill\_super)(struct super\_block \*sb, struct fs\_context \*fc), unsigned int flags)
:   Get a superblock based on a single block device

**Parameters**

`struct fs_context *fc`
:   The filesystem context holding the parameters

`int (*fill_super)(struct super_block *sb, struct fs_context *fc)`
:   Helper to initialise a new superblock

`unsigned int flags`
:   GET\_TREE\_BDEV\_\* flags

int get\_tree\_bdev(struct fs\_context \*fc, int (\*fill\_super)(struct super\_block\*, struct fs\_context\*))
:   Get a superblock based on a single block device

**Parameters**

`struct fs_context *fc`
:   The filesystem context holding the parameters

`int (*fill_super)(struct super_block *, struct fs_context *)`
:   Helper to initialise a new superblock

int vfs\_get\_tree(struct fs\_context \*fc)
:   Get the mountable root

**Parameters**

`struct fs_context *fc`
:   The superblock configuration context.

**Description**

The filesystem is invoked to get or create a superblock which can then later
be used for mounting. The filesystem places a pointer to the root to be
used for mounting in **fc->root**.

int freeze\_super(struct super\_block \*sb, enum freeze\_holder who, const void \*freeze\_owner)
:   lock the filesystem and force it into a consistent state

**Parameters**

`struct super_block *sb`
:   the super to lock

`enum freeze_holder who`
:   context that wants to freeze

`const void *freeze_owner`
:   owner of the freeze

**Description**

Syncs the super to make sure the filesystem is consistent and calls the fs’s
freeze\_fs. Subsequent calls to this without first thawing the fs may return
-EBUSY.

**who** should be:
\* `FREEZE_HOLDER_USERSPACE` if userspace wants to freeze the fs;
\* `FREEZE_HOLDER_KERNEL` if the kernel wants to freeze the fs.
\* `FREEZE_MAY_NEST` whether nesting freeze and thaw requests is allowed.

The **who** argument distinguishes between the kernel and userspace trying to
freeze the filesystem. Although there cannot be multiple kernel freezes or
multiple userspace freezes in effect at any given time, the kernel and
userspace can both hold a filesystem frozen. The filesystem remains frozen
until there are no kernel or userspace freezes in effect.

A filesystem may hold multiple devices and thus a filesystems may be
frozen through the block layer via multiple block devices. In this
case the request is marked as being allowed to nest by passing
FREEZE\_MAY\_NEST. The filesystem remains frozen until all block
devices are unfrozen. If multiple freezes are attempted without
FREEZE\_MAY\_NEST -EBUSY will be returned.

During this function, sb->s\_writers.frozen goes through these values:

SB\_UNFROZEN: File system is normal, all writes progress as usual.

SB\_FREEZE\_WRITE: The file system is in the process of being frozen. New
writes should be blocked, though page faults are still allowed. We wait for
all writes to complete and then proceed to the next stage.

SB\_FREEZE\_PAGEFAULT: Freezing continues. Now also page faults are blocked
but internal fs threads can still modify the filesystem (although they
should not dirty new pages or inodes), writeback can run etc. After waiting
for all running page faults we sync the filesystem which will clean all
dirty pages and inodes (no new dirty pages or inodes can be created when
sync is running).

SB\_FREEZE\_FS: The file system is frozen. Now all internal sources of fs
modification are blocked (e.g. XFS preallocation truncation on inode
reclaim). This is usually implemented by blocking new transactions for
filesystems that have them and need this additional guard. After all
internal writers are finished we call ->`freeze_fs()` to finish filesystem
freezing. Then we transition to SB\_FREEZE\_COMPLETE state. This state is
mostly auxiliary for filesystems to verify they do not modify frozen fs.

sb->s\_writers.frozen is protected by sb->s\_umount.

**Return**

If the freeze was successful zero is returned. If the freeze
failed a negative error code is returned.

int thaw\_super(struct super\_block \*sb, enum freeze\_holder who, const void \*freeze\_owner)
:   * unlock filesystem

**Parameters**

`struct super_block *sb`
:   the super to thaw

`enum freeze_holder who`
:   context that wants to freeze

`const void *freeze_owner`
:   owner of the freeze

**Description**

Unlocks the filesystem and marks it writeable again after [`freeze_super()`](#c.freeze_super "freeze_super")
if there are no remaining freezes on the filesystem.

**who** should be:
\* `FREEZE_HOLDER_USERSPACE` if userspace wants to thaw the fs;
\* `FREEZE_HOLDER_KERNEL` if the kernel wants to thaw the fs.
\* `FREEZE_MAY_NEST` whether nesting freeze and thaw requests is allowed

A filesystem may hold multiple devices and thus a filesystems may
have been frozen through the block layer via multiple block devices.
The filesystem remains frozen until all block devices are unfrozen.

### File Locks

bool locks\_owner\_has\_blockers(struct file\_lock\_context \*flctx, fl\_owner\_t owner)
:   Check for blocking lock requests

**Parameters**

`struct file_lock_context *flctx`
:   file lock context

`fl_owner_t owner`
:   lock owner

**Description**

Return values:
:   `true`: **owner** has at least one blocker
    `false`: **owner** has no blockers

int locks\_delete\_block(struct file\_lock \*waiter)
:   stop waiting for a file lock

**Parameters**

`struct file_lock *waiter`
:   the lock which was waiting

**Description**

> lockd/nfsd need to disconnect the lock while working on it.

int posix\_lock\_file(struct [file](#c.file "file") \*filp, struct file\_lock \*fl, struct file\_lock \*conflock)
:   Apply a POSIX-style lock to a file

**Parameters**

`struct file *filp`
:   The file to apply the lock to

`struct file_lock *fl`
:   The lock to be applied

`struct file_lock *conflock`
:   Place to return a copy of the conflicting lock, if found.

**Description**

Add a POSIX style lock to a file.
We merge adjacent & overlapping locks whenever possible.
POSIX locks are sorted by owner task, then by starting address

Note that if called with an FL\_EXISTS argument, the caller may determine
whether or not a lock was successfully freed by testing the return
value for -ENOENT.

int \_\_break\_lease(struct [inode](#c.__break_lease "inode") \*inode, unsigned int flags)
:   revoke all outstanding leases on file

**Parameters**

`struct inode *inode`
:   the inode of the file to return

`unsigned int flags`
:   LEASE\_BREAK\_\* flags

**Description**

> break\_lease (inlined for speed) has checked there already is at least
> some kind of lock (maybe a lease) on this file. Leases are broken on
> a call to open() or `truncate()`. This function can block waiting for the
> lease break unless you specify LEASE\_BREAK\_NONBLOCK.

void lease\_get\_mtime(struct [inode](#c.lease_get_mtime "inode") \*inode, struct timespec64 \*time)
:   update modified time of an inode with exclusive lease

**Parameters**

`struct inode *inode`
:   the inode

`struct timespec64 *time`
:   pointer to a timespec which contains the last modified time

**Description**

This is to force NFS clients to flush their caches for files with
exclusive leases. The justification is that if someone has an
exclusive lease, then they could be modifying it.

int generic\_setlease(struct [file](#c.file "file") \*filp, int arg, struct file\_lease \*\*flp, void \*\*priv)
:   sets a lease on an open file

**Parameters**

`struct file *filp`
:   file pointer

`int arg`
:   type of lease to obtain

`struct file_lease **flp`
:   input - file\_lock to use, output - file\_lock inserted

`void **priv`
:   private data for lm\_setup (may be NULL if lm\_setup
    doesn’t require it)

**Description**

> The (input) flp->fl\_lmops->lm\_break function is required
> by `break_lease()`.

int vfs\_setlease(struct [file](#c.file "file") \*filp, int arg, struct file\_lease \*\*lease, void \*\*priv)
:   sets a lease on an open file

**Parameters**

`struct file *filp`
:   file pointer

`int arg`
:   type of lease to obtain

`struct file_lease **lease`
:   file\_lock to use when adding a lease

`void **priv`
:   private info for lm\_setup when adding a lease (may be
    NULL if lm\_setup doesn’t require it)

**Description**

Call this to establish a lease on the file. The “lease” argument is not
used for F\_UNLCK requests and may be NULL. For commands that set or alter
an existing lease, the `(*lease)->fl_lmops->lm_break` operation must be
set; if not, this function will return -ENOLCK (and generate a scary-looking
stack trace).

The “priv” pointer is passed directly to the lm\_setup function as-is. It
may be NULL if the lm\_setup operation doesn’t require it.

int locks\_lock\_inode\_wait(struct [inode](#c.locks_lock_inode_wait "inode") \*inode, struct file\_lock \*fl)
:   Apply a lock to an inode

**Parameters**

`struct inode *inode`
:   inode of the file to apply to

`struct file_lock *fl`
:   The lock to be applied

**Description**

Apply a POSIX or FLOCK style lock request to an inode.

int vfs\_test\_lock(struct [file](#c.file "file") \*filp, struct file\_lock \*fl)
:   test file byte range lock

**Parameters**

`struct file *filp`
:   The file to test lock for

`struct file_lock *fl`
:   The byte-range in the file to test; also used to hold result

**Description**

On entry, **fl** does not contain a lock, but identifies a range (fl\_start, fl\_end)
in the file (c.flc\_file), and an owner (c.flc\_owner) for whom existing locks
should be ignored. c.flc\_type and c.flc\_flags are ignored.
Both fl\_lmops and fl\_ops in **fl** must be NULL.
Returns -ERRNO on failure. Indicates presence of conflicting lock by
setting fl->fl\_type to something other than F\_UNLCK.

If [`vfs_test_lock()`](#c.vfs_test_lock "vfs_test_lock") does find a lock and return it, the caller must
use `locks_free_lock()` or `locks_release_private()` on the returned lock.

int vfs\_lock\_file(struct [file](#c.file "file") \*filp, unsigned int cmd, struct file\_lock \*fl, struct file\_lock \*conf)
:   file byte range lock

**Parameters**

`struct file *filp`
:   The file to apply the lock to

`unsigned int cmd`
:   type of locking operation (F\_SETLK, F\_GETLK, etc.)

`struct file_lock *fl`
:   The lock to be applied

`struct file_lock *conf`
:   Place to return a copy of the conflicting lock, if found.

**Description**

A caller that doesn’t care about the conflicting lock may pass NULL
as the final argument.

If the filesystem defines a private ->`lock()` method, then **conf** will
be left unchanged; so a caller that cares should initialize it to
some acceptable default.

To avoid blocking kernel daemons, such as lockd, that need to acquire POSIX
locks, the ->`lock()` interface may return asynchronously, before the lock has
been granted or denied by the underlying filesystem, if (and only if)
lm\_grant is set. Additionally FOP\_ASYNC\_LOCK in file\_operations fop\_flags
need to be set.

Callers expecting ->`lock()` to return asynchronously will only use F\_SETLK,
not F\_SETLKW; they will set FL\_SLEEP if (and only if) the request is for a
blocking lock. When ->`lock()` does return asynchronously, it must return
FILE\_LOCK\_DEFERRED, and call ->`lm_grant()` when the lock request completes.
If the request is for non-blocking lock the file system should return
FILE\_LOCK\_DEFERRED then try to get the lock and call the callback routine
with the result. If the request timed out the callback routine will return a
nonzero return code and the file system should release the lock. The file
system is also responsible to keep a corresponding posix lock when it
grants a lock so the VFS can find out which locks are locally held and do
the correct lock cleanup when required.
The underlying filesystem must not drop the kernel lock or call
->`lm_grant()` before returning to the caller with a FILE\_LOCK\_DEFERRED
return code.

int vfs\_cancel\_lock(struct [file](#c.file "file") \*filp, struct file\_lock \*fl)
:   file byte range unblock lock

**Parameters**

`struct file *filp`
:   The file to apply the unblock to

`struct file_lock *fl`
:   The lock to be unblocked

**Description**

Used by lock managers to cancel blocked requests

bool vfs\_inode\_has\_locks(struct [inode](#c.vfs_inode_has_locks "inode") \*inode)
:   are any file locks held on **inode**?

**Parameters**

`struct inode *inode`
:   inode to check for locks

**Description**

Return true if there are any FL\_POSIX or FL\_FLOCK locks currently
set on **inode**.

int lease\_open\_conflict(struct [file](#c.file "file") \*filp, const int arg)
:   see if the given file points to an inode that has an existing open that would conflict with the desired lease.

**Parameters**

`struct file *filp`
:   file to check

`const int arg`
:   type of lease that we’re trying to acquire

**Description**

Check to see if there’s an existing open fd on this file that would
conflict with the lease we’re trying to set.

int posix\_lock\_inode\_wait(struct [inode](#c.posix_lock_inode_wait "inode") \*inode, struct file\_lock \*fl)
:   Apply a POSIX-style lock to a file

**Parameters**

`struct inode *inode`
:   inode of file to which lock request should be applied

`struct file_lock *fl`
:   The lock to be applied

**Description**

Apply a POSIX style lock request to an inode.

int \_\_fcntl\_getlease(struct [file](#c.file "file") \*filp, unsigned int flavor)
:   Enquire what lease is currently active

**Parameters**

`struct file *filp`
:   the file

`unsigned int flavor`
:   type of lease flags to check

**Description**

> The value returned by this function will be one of
> (if no lease break is pending):
>
> `F_RDLCK` to indicate a shared lease is held.
>
> `F_WRLCK` to indicate an exclusive lease is held.
>
> `F_UNLCK` to indicate no lease is held.
>
> (if a lease break is pending):
>
> `F_RDLCK` to indicate an exclusive lease needs to be
> :   changed to a shared lease (or removed).
>
> `F_UNLCK` to indicate the lease needs to be removed.
>
> XXX: sfr & willy disagree over whether F\_INPROGRESS
> should be returned to userspace.

int fcntl\_setlease(unsigned int fd, struct [file](#c.file "file") \*filp, int arg)
:   sets a lease on an open file

**Parameters**

`unsigned int fd`
:   open file descriptor

`struct file *filp`
:   file pointer

`int arg`
:   type of lease to obtain

**Description**

> Call this fcntl to establish a lease on the file.
> Note that you also need to call `F_SETSIG` to
> receive a signal when the lease is broken.

int fcntl\_setdeleg(unsigned int fd, struct [file](#c.file "file") \*filp, struct delegation \*deleg)
:   sets a delegation on an open file

**Parameters**

`unsigned int fd`
:   open file descriptor

`struct file *filp`
:   file pointer

`struct delegation *deleg`
:   delegation request from userland

**Description**

> Call this fcntl to establish a delegation on the file.
> Note that you also need to call `F_SETSIG` to
> receive a signal when the lease is broken.

int flock\_lock\_inode\_wait(struct [inode](#c.flock_lock_inode_wait "inode") \*inode, struct file\_lock \*fl)
:   Apply a FLOCK-style lock to a file

**Parameters**

`struct inode *inode`
:   inode of the file to apply to

`struct file_lock *fl`
:   The lock to be applied

**Description**

Apply a FLOCK style lock request to an inode.

long sys\_flock(unsigned int fd, unsigned int cmd)
:   * `flock()` system call.

**Parameters**

`unsigned int fd`
:   the file descriptor to lock.

`unsigned int cmd`
:   the type of lock to apply.

**Description**

> Apply a `FL_FLOCK` style lock to an open file descriptor.
> The **cmd** can be one of:
>
> * `LOCK_SH` -- a shared lock.
> * `LOCK_EX` -- an exclusive lock.
> * `LOCK_UN` -- remove an existing lock.
> * `LOCK_MAND` -- a ‘mandatory’ flock. (DEPRECATED)
>
> `LOCK_MAND` support has been removed from the kernel.

pid\_t locks\_translate\_pid(struct file\_lock\_core \*fl, struct pid\_namespace \*ns)
:   translate a file\_lock’s fl\_pid number into a namespace

**Parameters**

`struct file_lock_core *fl`
:   The file\_lock who’s fl\_pid should be translated

`struct pid_namespace *ns`
:   The namespace into which the pid should be translated

**Description**

Used to translate a fl\_pid into a namespace virtual pid number

### Other Functions

void mpage\_readahead(struct [readahead\_control](../core-api/mm-api.html#c.readahead_control "readahead_control") \*rac, get\_block\_t get\_block)
:   start reads against pages

**Parameters**

`struct readahead_control *rac`
:   Describes which pages to read.

`get_block_t get_block`
:   The filesystem’s block mapper function.

**Description**

This function walks the pages and the blocks within each page, building and
emitting large BIOs.

If anything unusual happens, such as:

* encountering a page which has buffers
* encountering a page which has a non-hole after a hole
* encountering a page with non-contiguous blocks

then this code just gives up and calls the buffer\_head-based read function.
It does handle a page which has holes at the end - that is a common case:
the end-of-file on blocksize < PAGE\_SIZE setups.

BH\_Boundary explanation:

There is a problem. The mpage read code assembles several pages, gets all
their disk mappings, and then submits them all. That’s fine, but obtaining
the disk mappings may require I/O. Reads of indirect blocks, for example.

So an mpage read of the first 16 blocks of an ext2 file will cause I/O to be
submitted in the following order:

> 12 0 1 2 3 4 5 6 7 8 9 10 11 13 14 15 16

because the indirect block has to be read to get the mappings of blocks
13,14,15,16. Obviously, this impacts performance.

So what we do it to allow the filesystem’s `get_block()` function to set
BH\_Boundary when it maps block 11. BH\_Boundary says: mapping of the block
after this one will require I/O against a block which is probably close to
this one. So you should push what I/O you have currently accumulated.

This all causes the disk requests to be issued in the correct order.

int \_\_mpage\_writepages(struct [address\_space](#c.address_space "address_space") \*mapping, struct writeback\_control \*wbc, get\_block\_t get\_block, int (\*write\_folio)(struct [folio](../core-api/mm-api.html#c.folio "folio") \*folio, struct writeback\_control \*wbc))
:   walk the list of dirty pages of the given address space & `writepage()` all of them

**Parameters**

`struct address_space *mapping`
:   address space structure to write

`struct writeback_control *wbc`
:   subtract the number of written pages from **\*wbc->nr\_to\_write**

`get_block_t get_block`
:   the filesystem’s block mapper function.

`int (*write_folio)(struct folio *folio, struct writeback_control *wbc)`
:   handler to call for each folio before calling
    `mpage_write_folio()`

**Description**

This is a library function, which implements the `writepages()`
address\_space\_operation. It calls **write\_folio** handler for each folio. If
the handler returns value > 0, it calls `mpage_write_folio()` to do the
folio writeback.

int generic\_permission(struct mnt\_idmap \*idmap, struct [inode](#c.generic_permission "inode") \*inode, int mask)
:   check for access rights on a Posix-like filesystem

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *inode`
:   inode to check access rights for

`int mask`
:   right to check for (`MAY_READ`, `MAY_WRITE`, `MAY_EXEC`,
    `MAY_NOT_BLOCK` ...)

**Description**

Used to check for read/write/execute permissions on a file.
We use “fsuid” for this, letting us set arbitrary permissions
for filesystem access without changing the “normal” uids which
are used for other things.

generic\_permission is rcu-walk aware. It returns -ECHILD in case an rcu-walk
request cannot be satisfied (eg. requires blocking or too much complexity).
It would then be called again in ref-walk mode.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

int inode\_permission(struct mnt\_idmap \*idmap, struct [inode](#c.inode_permission "inode") \*inode, int mask)
:   Check for access rights to a given inode

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *inode`
:   Inode to check permission on

`int mask`
:   Right to check for (`MAY_READ`, `MAY_WRITE`, `MAY_EXEC`)

**Description**

Check for read/write/execute permissions on an inode. We use fs[ug]id for
this, letting us set arbitrary permissions for filesystem access without
changing the “normal” UIDs which are used for other things.

When checking for MAY\_APPEND, MAY\_WRITE must also be set in **mask**.

void path\_get(const struct [path](#c.path_get "path") \*path)
:   get a reference to a path

**Parameters**

`const struct path *path`
:   path to get the reference to

**Description**

Given a path increment the reference count to the dentry and the vfsmount.

void path\_put(const struct [path](#c.path_put "path") \*path)
:   put a reference to a path

**Parameters**

`const struct path *path`
:   path to put the reference to

**Description**

Given a path decrement the reference count to the dentry and the vfsmount.

void end\_dirop(struct dentry \*de)
:   signal completion of a dirop

**Parameters**

`struct dentry *de`
:   the dentry which was returned by start\_dirop or similar.

**Description**

If the de is an error, nothing happens. Otherwise any lock taken to
protect the dentry is dropped and the dentry itself is release (`dput()`).

int vfs\_path\_parent\_lookup(struct [filename](#c.vfs_path_parent_lookup "filename") \*filename, unsigned int flags, struct path \*parent, struct qstr \*last, int \*type, const struct path \*root)
:   lookup a parent path relative to a dentry-vfsmount pair

**Parameters**

`struct filename *filename`
:   filename structure

`unsigned int flags`
:   lookup flags

`struct path *parent`
:   pointer to `struct path` to fill

`struct qstr *last`
:   last component

`int *type`
:   type of the last component

`const struct path *root`
:   pointer to `struct path` of the base directory

int vfs\_path\_lookup(struct [dentry](#c.vfs_path_lookup "dentry") \*dentry, struct vfsmount \*mnt, const char \*name, unsigned int flags, struct [path](#c.vfs_path_lookup "path") \*path)
:   lookup a file path relative to a dentry-vfsmount pair

**Parameters**

`struct dentry *dentry`
:   pointer to dentry of the base directory

`struct vfsmount *mnt`
:   pointer to vfs mount of the base directory

`const char *name`
:   pointer to file name

`unsigned int flags`
:   lookup flags

`struct path *path`
:   pointer to `struct path` to fill

struct dentry \*try\_lookup\_noperm(struct qstr \*name, struct dentry \*base)
:   filesystem helper to lookup single pathname component

**Parameters**

`struct qstr *name`
:   qstr storing pathname component to lookup

`struct dentry *base`
:   base directory to lookup from

**Description**

Look up a dentry by name in the dcache, returning NULL if it does not
currently exist or an error if there is a problem with the name.
The function does not try to create a dentry and if one
is found it doesn’t try to revalidate it.

Note that this routine is purely a helper for filesystem usage and should
not be called by generic code. It does no permission checking.

No locks need be held - only a counted reference to **base** is needed.

**Return**

* ref-counted dentry on success, or
* `NULL` if name could not be found, or
* ERR\_PTR(-EACCES) if name is dot or dotdot or contains a slash or nul, or
* [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") if fs provide ->d\_hash, and this returned an error.

struct dentry \*lookup\_noperm(struct qstr \*name, struct dentry \*base)
:   filesystem helper to lookup single pathname component

**Parameters**

`struct qstr *name`
:   qstr storing pathname component to lookup

`struct dentry *base`
:   base directory to lookup from

**Description**

Note that this routine is purely a helper for filesystem usage and should
not be called by generic code. It does no permission checking.

The caller must hold base->i\_rwsem.

struct dentry \*lookup\_one(struct mnt\_idmap \*idmap, struct qstr \*name, struct dentry \*base)
:   lookup single pathname component

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the lookup is performed from

`struct qstr *name`
:   qstr holding pathname component to lookup

`struct dentry *base`
:   base directory to lookup from

**Description**

This can be used for in-kernel filesystem clients such as file servers.

The caller must hold base->i\_rwsem.

struct dentry \*lookup\_one\_unlocked(struct mnt\_idmap \*idmap, struct qstr \*name, struct dentry \*base)
:   lookup single pathname component

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the lookup is performed from

`struct qstr *name`
:   qstr olding pathname component to lookup

`struct dentry *base`
:   base directory to lookup from

**Description**

This can be used for in-kernel filesystem clients such as file servers.

Unlike lookup\_one, it should be called without the parent
i\_rwsem held, and will take the i\_rwsem itself if necessary.

**Return**

* A dentry, possibly negative, or
* same errors as [`try_lookup_noperm()`](#c.try_lookup_noperm "try_lookup_noperm") or
* ERR\_PTR(-ENOENT) if parent has been removed, or
* ERR\_PTR(-EACCES) if parent directory is not searchable.

struct dentry \*lookup\_one\_positive\_killable(struct mnt\_idmap \*idmap, struct qstr \*name, struct dentry \*base)
:   lookup single pathname component

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the lookup is performed from

`struct qstr *name`
:   qstr olding pathname component to lookup

`struct dentry *base`
:   base directory to lookup from

**Description**

This helper will yield ERR\_PTR(-ENOENT) on negatives. The helper returns
known positive or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR"). This is what most of the users want.

Note that pinned negative with unlocked parent \_can\_ become positive at any
time, so callers of [`lookup_one_unlocked()`](#c.lookup_one_unlocked "lookup_one_unlocked") need to be very careful; pinned
positives have >d\_inode stable, so this one avoids such problems.

This can be used for in-kernel filesystem clients such as file servers.

It should be called without the parent i\_rwsem held, and will take
the i\_rwsem itself if necessary. If a fatal signal is pending or
delivered, it will return `-EINTR` if the lock is needed.

**Return**

A dentry, possibly negative, or
- same errors as [`lookup_one_unlocked()`](#c.lookup_one_unlocked "lookup_one_unlocked") or
- ERR\_PTR(-EINTR) if a fatal signal is pending.

struct dentry \*lookup\_one\_positive\_unlocked(struct mnt\_idmap \*idmap, struct qstr \*name, struct dentry \*base)
:   lookup single pathname component

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the lookup is performed from

`struct qstr *name`
:   qstr holding pathname component to lookup

`struct dentry *base`
:   base directory to lookup from

**Description**

This helper will yield ERR\_PTR(-ENOENT) on negatives. The helper returns
known positive or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR"). This is what most of the users want.

Note that pinned negative with unlocked parent \_can\_ become positive at any
time, so callers of [`lookup_one_unlocked()`](#c.lookup_one_unlocked "lookup_one_unlocked") need to be very careful; pinned
positives have >d\_inode stable, so this one avoids such problems.

This can be used for in-kernel filesystem clients such as file servers.

The helper should be called without i\_rwsem held.

**Return**

A positive dentry, or
- ERR\_PTR(-ENOENT) if the name could not be found, or
- same errors as [`lookup_one_unlocked()`](#c.lookup_one_unlocked "lookup_one_unlocked").

struct dentry \*lookup\_noperm\_unlocked(struct qstr \*name, struct dentry \*base)
:   filesystem helper to lookup single pathname component

**Parameters**

`struct qstr *name`
:   pathname component to lookup

`struct dentry *base`
:   base directory to lookup from

**Description**

Note that this routine is purely a helper for filesystem usage and should
not be called by generic code. It does no permission checking.

Unlike [`lookup_noperm()`](#c.lookup_noperm "lookup_noperm"), it should be called without the parent
i\_rwsem held, and will take the i\_rwsem itself if necessary.

Unlike [`try_lookup_noperm()`](#c.try_lookup_noperm "try_lookup_noperm") it *does* revalidate the dentry if it already
existed.

**Return**

A dentry, possibly negative, or
- ERR\_PTR(-ENOENT) if parent has been removed, or
- same errors as [`try_lookup_noperm()`](#c.try_lookup_noperm "try_lookup_noperm")

struct dentry \*start\_creating(struct mnt\_idmap \*idmap, struct dentry \*parent, struct qstr \*name)
:   prepare to create a given name with permission checking

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *parent`
:   directory in which to prepare to create the name

`struct qstr *name`
:   the name to be created

**Description**

Locks are taken and a lookup is performed prior to creating
an object in a directory. Permission checking (MAY\_EXEC) is performed
against **idmap**.

If the name already exists, a positive dentry is returned, so
behaviour is similar to O\_CREAT without O\_EXCL, which doesn’t fail
with -EEXIST.

**Return**

a negative or positive dentry, or an error.

struct dentry \*start\_removing(struct mnt\_idmap \*idmap, struct dentry \*parent, struct qstr \*name)
:   prepare to remove a given name with permission checking

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *parent`
:   directory in which to find the name

`struct qstr *name`
:   the name to be removed

**Description**

Locks are taken and a lookup in performed prior to removing
an object from a directory. Permission checking (MAY\_EXEC) is performed
against **idmap**.

If the name doesn’t exist, an error is returned.

`end_removing()` should be called when removal is complete, or aborted.

**Return**

a positive dentry, or an error.

struct dentry \*start\_creating\_killable(struct mnt\_idmap \*idmap, struct dentry \*parent, struct qstr \*name)
:   prepare to create a given name with permission checking

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *parent`
:   directory in which to prepare to create the name

`struct qstr *name`
:   the name to be created

**Description**

Locks are taken and a lookup in performed prior to creating
an object in a directory. Permission checking (MAY\_EXEC) is performed
against **idmap**.

If the name already exists, a positive dentry is returned.

If a signal is received or was already pending, the function aborts
with -EINTR;

**Return**

a negative or positive dentry, or an error.

struct dentry \*start\_removing\_killable(struct mnt\_idmap \*idmap, struct dentry \*parent, struct qstr \*name)
:   prepare to remove a given name with permission checking

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *parent`
:   directory in which to find the name

`struct qstr *name`
:   the name to be removed

**Description**

Locks are taken and a lookup in performed prior to removing
an object from a directory. Permission checking (MAY\_EXEC) is performed
against **idmap**.

If the name doesn’t exist, an error is returned.

`end_removing()` should be called when removal is complete, or aborted.

If a signal is received or was already pending, the function aborts
with -EINTR;

**Return**

a positive dentry, or an error.

struct dentry \*start\_creating\_noperm(struct dentry \*parent, struct qstr \*name)
:   prepare to create a given name without permission checking

**Parameters**

`struct dentry *parent`
:   directory in which to prepare to create the name

`struct qstr *name`
:   the name to be created

**Description**

Locks are taken and a lookup in performed prior to creating
an object in a directory.

If the name already exists, a positive dentry is returned.

**Return**

a negative or positive dentry, or an error.

struct dentry \*start\_removing\_noperm(struct dentry \*parent, struct qstr \*name)
:   prepare to remove a given name without permission checking

**Parameters**

`struct dentry *parent`
:   directory in which to find the name

`struct qstr *name`
:   the name to be removed

**Description**

Locks are taken and a lookup in performed prior to removing
an object from a directory.

If the name doesn’t exist, an error is returned.

`end_removing()` should be called when removal is complete, or aborted.

**Return**

a positive dentry, or an error.

struct dentry \*start\_creating\_dentry(struct dentry \*parent, struct dentry \*child)
:   prepare to create a given dentry

**Parameters**

`struct dentry *parent`
:   directory from which dentry should be removed

`struct dentry *child`
:   the dentry to be removed

**Description**

A lock is taken to protect the dentry again other dirops and
the validity of the dentry is checked: correct parent and still hashed.

If the dentry is valid and negative a reference is taken and
returned. If not an error is returned.

`end_creating()` should be called when creation is complete, or aborted.

**Return**

the valid dentry, or an error.

struct dentry \*start\_removing\_dentry(struct dentry \*parent, struct dentry \*child)
:   prepare to remove a given dentry

**Parameters**

`struct dentry *parent`
:   directory from which dentry should be removed

`struct dentry *child`
:   the dentry to be removed

**Description**

A lock is taken to protect the dentry again other dirops and
the validity of the dentry is checked: correct parent and still hashed.

If the dentry is valid and positive, a reference is taken and
returned. If not an error is returned.

`end_removing()` should be called when removal is complete, or aborted.

**Return**

the valid dentry, or an error.

int start\_renaming(struct [renamedata](#c.renamedata "renamedata") \*rd, int lookup\_flags, struct qstr \*old\_last, struct qstr \*new\_last)
:   lookup and lock names for rename with permission checking

**Parameters**

`struct renamedata *rd`
:   rename data containing parents and flags, and
    for receiving found dentries

`int lookup_flags`
:   extra flags to pass to ->lookup (e.g. LOOKUP\_REVAL,
    LOOKUP\_NO\_SYMLINKS etc).

`struct qstr *old_last`
:   name of object in **rd.old\_parent**

`struct qstr *new_last`
:   name of object in **rd.new\_parent**

**Description**

Look up two names and ensure locks are in place for
rename.

On success the found dentries are stored in **rd.old\_dentry**,
**rd.new\_dentry**. Also the refcount on **rd->old\_parent** is increased.
These references and the lock are dropped by `end_renaming()`.

The passed in qstrs need not have the hash calculated, and basic
eXecute permission checking is performed against **rd.mnt\_idmap**.

**Return**

zero or an error.

int start\_renaming\_dentry(struct [renamedata](#c.renamedata "renamedata") \*rd, int lookup\_flags, struct dentry \*old\_dentry, struct qstr \*new\_last)
:   lookup and lock name for rename with permission checking

**Parameters**

`struct renamedata *rd`
:   rename data containing parents and flags, and
    for receiving found dentries

`int lookup_flags`
:   extra flags to pass to ->lookup (e.g. LOOKUP\_REVAL,
    LOOKUP\_NO\_SYMLINKS etc).

`struct dentry *old_dentry`
:   dentry of name to move

`struct qstr *new_last`
:   name of target in **rd.new\_parent**

**Description**

Look up target name and ensure locks are in place for
rename.

On success the found dentry is stored in **rd.new\_dentry** and
**rd.old\_parent** is confirmed to be the parent of **old\_dentry**. If it
was originally `NULL`, it is set. In either case a reference is taken
so that `end_renaming()` can have a stable reference to unlock.

References and the lock can be dropped with `end_renaming()`

The passed in qstr need not have the hash calculated, and basic
eXecute permission checking is performed against **rd.mnt\_idmap**.

**Return**

zero or an error.

int start\_renaming\_two\_dentries(struct [renamedata](#c.renamedata "renamedata") \*rd, struct dentry \*old\_dentry, struct dentry \*new\_dentry)
:   Lock to dentries in given parents for rename

**Parameters**

`struct renamedata *rd`
:   rename data containing parent

`struct dentry *old_dentry`
:   dentry of name to move

`struct dentry *new_dentry`
:   dentry to move to

**Description**

Ensure locks are in place for rename and check parentage is still correct.

On success the two dentries are stored in **rd.old\_dentry** and
**rd.new\_dentry** and **rd.old\_parent** and **rd.new\_parent** are confirmed to
be the parents of the dentries.

References and the lock can be dropped with `end_renaming()`

**Return**

zero or an error.

int vfs\_create(struct mnt\_idmap \*idmap, struct [dentry](#c.vfs_create "dentry") \*dentry, umode\_t mode, struct delegated\_inode \*di)
:   create new file

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct dentry *dentry`
:   dentry of the child file

`umode_t mode`
:   mode of the child file

`struct delegated_inode *di`
:   returns parent inode, if the inode is delegated.

**Description**

Create a new file.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

struct [file](#c.file "file") \*kernel\_tmpfile\_open(struct mnt\_idmap \*idmap, const struct path \*parentpath, umode\_t mode, int open\_flag, const struct [cred](#c.kernel_tmpfile_open "cred") \*cred)
:   open a tmpfile for kernel internal use

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`const struct path *parentpath`
:   path of the base directory

`umode_t mode`
:   mode of the new tmpfile

`int open_flag`
:   flags

`const struct cred *cred`
:   credentials for open

**Description**

Create and open a temporary file. The file is not accounted in nr\_files,
hence this is only for kernel internal use, and must not be installed into
file tables or such.

void end\_creating\_path(const struct [path](#c.end_creating_path "path") \*path, struct [dentry](#c.end_creating_path "dentry") \*dentry)
:   finish a code section started by `start_creating_path()`

**Parameters**

`const struct path *path`
:   the path instantiated by `start_creating_path()`

`struct dentry *dentry`
:   the dentry returned by `start_creating_path()`

**Description**

[`end_creating_path()`](#c.end_creating_path "end_creating_path") will unlock and locks taken by `start_creating_path()`
and drop an references that were taken. It should only be called
if `start_creating_path()` returned a non-error.
If [`vfs_mkdir()`](#c.vfs_mkdir "vfs_mkdir") was called and it returned an error, that error *should*
be passed to [`end_creating_path()`](#c.end_creating_path "end_creating_path") together with the path.

struct [file](#c.file "file") \*dentry\_create(struct [path](#c.dentry_create "path") \*path, int flags, umode\_t mode, const struct [cred](#c.dentry_create "cred") \*cred)
:   Create and open a file

**Parameters**

`struct path *path`
:   path to create

`int flags`
:   O\_ flags

`umode_t mode`
:   mode bits for new file

`const struct cred *cred`
:   credentials to use

**Description**

Caller must hold the parent directory’s lock, and have prepared
a negative dentry, placed in **path->dentry**, for the new file.

Caller sets **path->mnt** to the vfsmount of the filesystem where
the new file is to be created. The parent directory and the
negative dentry must reside on the same filesystem instance.

On success, returns a `struct file *`. Otherwise an ERR\_PTR
is returned.

int vfs\_mknod(struct mnt\_idmap \*idmap, struct inode \*dir, struct [dentry](#c.vfs_mknod "dentry") \*dentry, umode\_t mode, dev\_t dev, struct [delegated\_inode](#c.vfs_mknod "delegated_inode") \*delegated\_inode)
:   create device node or file

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *dir`
:   inode of the parent directory

`struct dentry *dentry`
:   dentry of the child device node

`umode_t mode`
:   mode of the child device node

`dev_t dev`
:   device number of device to create

`struct delegated_inode *delegated_inode`
:   returns parent inode, if the inode is delegated.

**Description**

Create a device node or file.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

struct [dentry](#c.vfs_mkdir "dentry") \*vfs\_mkdir(struct mnt\_idmap \*idmap, struct inode \*dir, struct [dentry](#c.vfs_mkdir "dentry") \*dentry, umode\_t mode, struct [delegated\_inode](#c.vfs_mkdir "delegated_inode") \*delegated\_inode)
:   create directory returning correct dentry if possible

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *dir`
:   inode of the parent directory

`struct dentry *dentry`
:   dentry of the child directory

`umode_t mode`
:   mode of the child directory

`struct delegated_inode *delegated_inode`
:   returns parent inode, if the inode is delegated.

**Description**

Create a directory.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

In the event that the filesystem does not use the **\*dentry** but leaves it
negative or unhashes it and possibly splices a different one returning it,
the original dentry is `dput()` and the alternate is returned.

In case of an error the dentry is `dput()` and an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") is returned.

int vfs\_rmdir(struct mnt\_idmap \*idmap, struct inode \*dir, struct [dentry](#c.vfs_rmdir "dentry") \*dentry, struct [delegated\_inode](#c.vfs_rmdir "delegated_inode") \*delegated\_inode)
:   remove directory

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *dir`
:   inode of the parent directory

`struct dentry *dentry`
:   dentry of the child directory

`struct delegated_inode *delegated_inode`
:   returns parent inode, if it’s delegated.

**Description**

Remove a directory.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

int vfs\_unlink(struct mnt\_idmap \*idmap, struct inode \*dir, struct [dentry](#c.vfs_unlink "dentry") \*dentry, struct [delegated\_inode](#c.vfs_unlink "delegated_inode") \*delegated\_inode)
:   unlink a filesystem object

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *dir`
:   parent directory

`struct dentry *dentry`
:   victim

`struct delegated_inode *delegated_inode`
:   returns victim inode, if the inode is delegated.

**Description**

The caller must hold dir->i\_rwsem exclusively.

If vfs\_unlink discovers a delegation, it will return -EWOULDBLOCK and
return a reference to the inode in delegated\_inode. The caller
should then break the delegation on that inode and retry. Because
breaking a delegation may take a long time, the caller should drop
dir->i\_rwsem before doing so.

Alternatively, a caller may pass NULL for delegated\_inode. This may
be appropriate for callers that expect the underlying filesystem not
to be NFS exported.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

int vfs\_symlink(struct mnt\_idmap \*idmap, struct inode \*dir, struct [dentry](#c.vfs_symlink "dentry") \*dentry, const char \*oldname, struct [delegated\_inode](#c.vfs_symlink "delegated_inode") \*delegated\_inode)
:   create symlink

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *dir`
:   inode of the parent directory

`struct dentry *dentry`
:   dentry of the child symlink file

`const char *oldname`
:   name of the file to link to

`struct delegated_inode *delegated_inode`
:   returns victim inode, if the inode is delegated.

**Description**

Create a symlink.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

int vfs\_link(struct dentry \*old\_dentry, struct mnt\_idmap \*idmap, struct inode \*dir, struct dentry \*new\_dentry, struct [delegated\_inode](#c.vfs_link "delegated_inode") \*delegated\_inode)
:   create a new link

**Parameters**

`struct dentry *old_dentry`
:   object to be linked

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct inode *dir`
:   new parent

`struct dentry *new_dentry`
:   where to create the new link

`struct delegated_inode *delegated_inode`
:   returns inode needing a delegation break

**Description**

The caller must hold dir->i\_rwsem exclusively.

If vfs\_link discovers a delegation on the to-be-linked file in need
of breaking, it will return -EWOULDBLOCK and return a reference to the
inode in delegated\_inode. The caller should then break the delegation
and retry. Because breaking a delegation may take a long time, the
caller should drop the i\_rwsem before doing so.

Alternatively, a caller may pass NULL for delegated\_inode. This may
be appropriate for callers that expect the underlying filesystem not
to be NFS exported.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then take
care to map the inode according to **idmap** before checking permissions.
On non-idmapped mounts or if permission checking is to be performed on the
raw inode simply pass **nop\_mnt\_idmap**.

int vfs\_rename(struct [renamedata](#c.renamedata "renamedata") \*rd)
:   rename a filesystem object

**Parameters**

`struct renamedata *rd`
:   pointer to [`struct renamedata`](#c.renamedata "renamedata") info

**Description**

The caller must hold multiple mutexes--see `lock_rename()`).

If vfs\_rename discovers a delegation in need of breaking at either
the source or destination, it will return -EWOULDBLOCK and return a
reference to the inode in delegated\_inode. The caller should then
break the delegation and retry. Because breaking a delegation may
take a long time, the caller should drop all locks before doing
so.

Alternatively, a caller may pass NULL for delegated\_inode. This may
be appropriate for callers that expect the underlying filesystem not
to be NFS exported.

The worst of all namespace operations - renaming directory. “Perverted”
doesn’t even start to describe it. Somebody in UCB had a heck of a trip...
Problems:

> 1. we can get into loop creation.
> 2. race potential - two innocent renames can create a loop together.
>    That’s where 4.4BSD screws up. Current fix: serialization on
>    sb->s\_vfs\_rename\_mutex. We might be more accurate, but that’s another
>    story.
> 3. we may have to lock up to \_four\_ objects - parents and victim (if it exists),
>    and source (if it’s a non-directory or a subdirectory that moves to
>    different parent).
>    And that - after we got ->i\_rwsem on parents (until then we don’t know
>    whether the target exists). Solution: try to be smart with locking
>    order for inodes. We rely on the fact that tree topology may change
>    only under ->s\_vfs\_rename\_mutex \_and\_ that parent of the object we
>    move will be locked. Thus we can rank directories by the tree
>    (ancestors first) and rank all non-directories after them.
>    That works since everybody except rename does “lock parent, lookup,
>    lock child” and rename is under ->s\_vfs\_rename\_mutex.
>    HOWEVER, it relies on the assumption that any object with ->`lookup()`
>    has no more than 1 dentry. If “hybrid” objects will ever appear,
>    we’d better make sure that there’s no link(2) for them.
> 4. conversion from fhandle to dentry may come in the wrong moment - when
>    we are removing the target. Solution: we will have to grab ->i\_rwsem
>    in the fhandle\_to\_dentry code. [FIXME - current nfsfh.c relies on
>    ->i\_rwsem on parents, which works but leads to some truly excessive
>    locking].

int vfs\_readlink(struct [dentry](#c.vfs_readlink "dentry") \*dentry, char \_\_user \*buffer, int buflen)
:   copy symlink body into userspace buffer

**Parameters**

`struct dentry *dentry`
:   dentry on which to get symbolic link

`char __user *buffer`
:   user memory pointer

`int buflen`
:   size of buffer

**Description**

Does not touch atime. That’s up to the caller if necessary

Does not call security hook.

const char \*vfs\_get\_link(struct [dentry](#c.vfs_get_link "dentry") \*dentry, struct delayed\_call \*done)
:   get symlink body

**Parameters**

`struct dentry *dentry`
:   dentry on which to get symbolic link

`struct delayed_call *done`
:   caller needs to free returned data with this

**Description**

Calls security hook and i\_op->`get_link()` on the supplied inode.

It does not touch atime. That’s up to the caller if necessary.

Does not work on “special” symlinks like /proc/$$/fd/N

const char \*page\_get\_link(struct [dentry](#c.page_get_link "dentry") \*dentry, struct [inode](#c.page_get_link "inode") \*inode, struct delayed\_call \*callback)
:   An implementation of the get\_link inode\_operation.

**Parameters**

`struct dentry *dentry`
:   The directory entry which is the symlink.

`struct inode *inode`
:   The inode for the symlink.

`struct delayed_call *callback`
:   Used to drop the reference to the symlink.

**Description**

Filesystems which store their symlinks in the page cache should use
this to implement the `get_link()` member of their inode\_operations.

**Return**

A pointer to the NUL-terminated symlink.

void page\_put\_link(void \*arg)
:   Drop the reference to the symlink.

**Parameters**

`void *arg`
:   The folio which contains the symlink.

**Description**

This is used internally by [`page_get_link()`](#c.page_get_link "page_get_link"). It is exported for use
by filesystems which need to implement a variant of [`page_get_link()`](#c.page_get_link "page_get_link")
themselves. Despite the apparent symmetry, filesystems which use
[`page_get_link()`](#c.page_get_link "page_get_link") do not need to call [`page_put_link()`](#c.page_put_link "page_put_link").

The argument, while it has a void pointer type, must be a pointer to
the folio which was retrieved from the page cache. The delayed\_call
infrastructure is used to drop the reference count once the caller
is done with the symlink.

int finish\_open(struct [file](#c.finish_open "file") \*file, struct [dentry](#c.finish_open "dentry") \*dentry, int (\*open)(struct inode\*, struct [file](#c.finish_open "file")\*))
:   finish opening a file

**Parameters**

`struct file *file`
:   file pointer

`struct dentry *dentry`
:   pointer to dentry

`int (*open)(struct inode *, struct file *)`
:   open callback

**Description**

This can be used to finish opening a file passed to i\_op->`atomic_open()`.

If the open callback is set to NULL, then the standard f\_op->open()
filesystem callback is substituted.

NB: the dentry reference is \_not\_ consumed. If, for example, the dentry is
the return value of [`d_splice_alias()`](#c.d_splice_alias "d_splice_alias"), then the caller needs to perform `dput()`
on it after [`finish_open()`](#c.finish_open "finish_open").

Returns zero on success or -errno if the open failed.

int finish\_no\_open(struct [file](#c.finish_no_open "file") \*file, struct [dentry](#c.finish_no_open "dentry") \*dentry)
:   finish ->`atomic_open()` without opening the file

**Parameters**

`struct file *file`
:   file pointer

`struct dentry *dentry`
:   dentry, ERR\_PTR(-E...) or NULL (as returned from ->`lookup()`)

**Description**

This can be used to set the result of a lookup in ->`atomic_open()`.

NB: unlike [`finish_open()`](#c.finish_open "finish_open") this function does consume the dentry reference and
the caller need not `dput()` it.

Returns 0 or -E..., which must be the return value of ->`atomic_open()` after
having called this function.

struct [file](#c.file "file") \*kernel\_file\_open(const struct [path](#c.kernel_file_open "path") \*path, int flags, const struct [cred](#c.kernel_file_open "cred") \*cred)
:   open a file for kernel internal use

**Parameters**

`const struct path *path`
:   path of the file to open

`int flags`
:   open flags

`const struct cred *cred`
:   credentials for open

**Description**

Open a file for use by in-kernel consumers. The file is not accounted
against nr\_files and must not be installed into the file descriptor
table.

**Return**

Opened file on success, an error pointer on failure.

struct [file](#c.file "file") \*filp\_open(const char \*filename, int flags, umode\_t mode)
:   open file and return file pointer

**Parameters**

`const char *filename`
:   path to open

`int flags`
:   open flags as per the open(2) second argument

`umode_t mode`
:   mode for the new file if O\_CREAT is set, else ignored

**Description**

This is the helper to open a file from kernelspace if you really
have to. But in generally you should not do this, so please move
along, nothing to see here..

void bio\_reset(struct [bio](#c.bio_reset "bio") \*bio, struct block\_device \*bdev, blk\_opf\_t opf)
:   reinitialize a bio

**Parameters**

`struct bio *bio`
:   bio to reset

`struct block_device *bdev`
:   block device to use the bio for

`blk_opf_t opf`
:   operation and flags for bio

**Description**

> After calling [`bio_reset()`](#c.bio_reset "bio_reset"), **bio** will be in the same state as a freshly
> allocated bio returned bio [`bio_alloc_bioset()`](#c.bio_alloc_bioset "bio_alloc_bioset") - the only fields that are
> preserved are the ones that are initialized by [`bio_alloc_bioset()`](#c.bio_alloc_bioset "bio_alloc_bioset"). See
> comment in `struct bio`.

void bio\_reuse(struct [bio](#c.bio_reuse "bio") \*bio, blk\_opf\_t opf)
:   reuse a bio with the payload left intact

**Parameters**

`struct bio *bio`
:   bio to reuse

`blk_opf_t opf`
:   operation and flags for the next I/O

**Description**

Allow reusing an existing bio for another operation with all set up
fields including the payload, device and end\_io handler left intact.

Typically used when **bio** is first used to read data which is then written
to another location without modification. **bio** must not be in-flight and
owned by the caller. Can’t be used for cloned bios.

**Note**

Can’t be used when **bio** has integrity or blk-crypto contexts for now.
Feel free to add that support when you need it, though.

void bio\_chain(struct [bio](#c.bio_chain "bio") \*bio, struct [bio](#c.bio_chain "bio") \*parent)
:   chain bio completions

**Parameters**

`struct bio *bio`
:   the target bio

`struct bio *parent`
:   the parent bio of **bio**

**Description**

The caller won’t have a bi\_end\_io called when **bio** completes - instead,
**parent**’s bi\_end\_io won’t be called until both **parent** and **bio** have
completed; the chained bio will also be freed when it completes.

The caller must not set bi\_private or bi\_end\_io in **bio**.

struct bio \*bio\_alloc\_bioset(struct block\_device \*bdev, unsigned short nr\_vecs, blk\_opf\_t opf, gfp\_t gfp, struct bio\_set \*bs)
:   allocate a bio for I/O

**Parameters**

`struct block_device *bdev`
:   block device to allocate the bio for (can be `NULL`)

`unsigned short nr_vecs`
:   number of bvecs to pre-allocate

`blk_opf_t opf`
:   operation and flags for bio

`gfp_t gfp`
:   the GFP\_\* mask given to the slab allocator

`struct bio_set *bs`
:   the bio\_set to allocate from.

**Description**

Allocate a bio from the mempools in **bs**.

If `__GFP_DIRECT_RECLAIM` is set then bio\_alloc will always be able to
allocate a bio. This is due to the mempool guarantees. To make this work,
callers must never allocate more than 1 bio at a time from the general pool.
Callers that need to allocate more than 1 bio must always submit the
previously allocated bio for IO before attempting to allocate a new one.
Failure to do so can cause deadlocks under memory pressure.

Note that when running under [`submit_bio_noacct()`](../core-api/kernel-api.html#c.submit_bio_noacct "submit_bio_noacct") (i.e. any block driver),
bios are not submitted until after you return - see the code in
[`submit_bio_noacct()`](../core-api/kernel-api.html#c.submit_bio_noacct "submit_bio_noacct") that converts recursion into iteration, to prevent
stack overflows.

This would normally mean allocating multiple bios under [`submit_bio_noacct()`](../core-api/kernel-api.html#c.submit_bio_noacct "submit_bio_noacct")
would be susceptible to deadlocks, but we have
deadlock avoidance code that resubmits any blocked bios from a rescuer
thread.

However, we do not guarantee forward progress for allocations from other
mempools. Doing multiple allocations from the same mempool under
[`submit_bio_noacct()`](../core-api/kernel-api.html#c.submit_bio_noacct "submit_bio_noacct") should be avoided - instead, use bio\_set’s front\_pad
for per bio allocations.

**Return**

Pointer to new bio on success, NULL on failure.

struct bio \*bio\_kmalloc(unsigned short nr\_vecs, gfp\_t gfp\_mask)
:   kmalloc a bio

**Parameters**

`unsigned short nr_vecs`
:   number of bio\_vecs to allocate

`gfp_t gfp_mask`
:   the GFP\_\* mask given to the slab allocator

**Description**

Use kmalloc to allocate a bio (including bvecs). The bio must be initialized
using `bio_init()` before use. To free a bio returned from this function use
[`kfree()`](../core-api/mm-api.html#c.kfree "kfree") after calling `bio_uninit()`. A bio returned from this function can
be reused by calling `bio_uninit()` before calling `bio_init()` again.

Note that unlike `bio_alloc()` or [`bio_alloc_bioset()`](#c.bio_alloc_bioset "bio_alloc_bioset") allocations from this
function are not backed by a mempool can fail. Do not use this function
for allocations in the file system I/O path.

**Return**

Pointer to new bio on success, NULL on failure.

void bio\_put(struct [bio](#c.bio_put "bio") \*bio)
:   release a reference to a bio

**Parameters**

`struct bio *bio`
:   bio to release reference to

**Description**

> Put a reference to a `struct bio`, either one you have gotten with
> bio\_alloc, bio\_get or bio\_clone\_\*. The last put of a bio will free it.

struct bio \*bio\_alloc\_clone(struct block\_device \*bdev, struct bio \*bio\_src, gfp\_t gfp, struct bio\_set \*bs)
:   clone a bio that shares the original bio’s biovec

**Parameters**

`struct block_device *bdev`
:   block\_device to clone onto

`struct bio *bio_src`
:   bio to clone from

`gfp_t gfp`
:   allocation priority

`struct bio_set *bs`
:   bio\_set to allocate from

**Description**

Allocate a new bio that is a clone of **bio\_src**. This reuses the bio\_vecs
pointed to by **bio\_src->bi\_io\_vec**, and clones the iterator pointing to
the current position in it. The caller owns the returned bio, but not
the bio\_vecs, and must ensure the bio is freed before the memory
pointed to by **bio\_Src->bi\_io\_vecs**.

int bio\_init\_clone(struct block\_device \*bdev, struct [bio](#c.bio_init_clone "bio") \*bio, struct [bio](#c.bio_init_clone "bio") \*bio\_src, gfp\_t gfp)
:   clone a bio that shares the original bio’s biovec

**Parameters**

`struct block_device *bdev`
:   block\_device to clone onto

`struct bio *bio`
:   bio to clone into

`struct bio *bio_src`
:   bio to clone from

`gfp_t gfp`
:   allocation priority

**Description**

Initialize a new bio in caller provided memory that is a clone of **bio\_src**.
The same bio\_vecs reuse and bio lifetime rules as [`bio_alloc_clone()`](#c.bio_alloc_clone "bio_alloc_clone") apply.

void \_\_bio\_add\_page(struct [bio](#c.__bio_add_page "bio") \*bio, struct [page](#c.__bio_add_page "page") \*page, unsigned int len, unsigned int off)
:   add page(s) to a bio in a new segment

**Parameters**

`struct bio *bio`
:   destination bio

`struct page *page`
:   start page to add

`unsigned int len`
:   length of the data to add, may cross pages

`unsigned int off`
:   offset of the data relative to **page**, may cross pages

**Description**

Add the data at **page** + **off** to **bio** as a new bvec. The caller must ensure
that **bio** has space for another bvec.

void bio\_add\_virt\_nofail(struct [bio](#c.bio_add_virt_nofail "bio") \*bio, void \*vaddr, unsigned len)
:   add data in the direct kernel mapping to a bio

**Parameters**

`struct bio *bio`
:   destination bio

`void *vaddr`
:   data to add

`unsigned len`
:   length of the data to add, may cross pages

**Description**

Add the data at **vaddr** to **bio**. The caller must have ensure a segment
is available for the added data. No merging into an existing segment
will be performed.

int bio\_add\_page(struct [bio](#c.bio_add_page "bio") \*bio, struct [page](#c.bio_add_page "page") \*page, unsigned int len, unsigned int offset)
:   attempt to add page(s) to bio

**Parameters**

`struct bio *bio`
:   destination bio

`struct page *page`
:   start page to add

`unsigned int len`
:   vec entry length, may cross pages

`unsigned int offset`
:   vec entry offset relative to **page**, may cross pages

**Description**

> Attempt to add page(s) to the bio\_vec maplist. This will only fail
> if either bio->bi\_vcnt == bio->bi\_max\_vecs or it’s a cloned bio.

bool bio\_add\_folio(struct [bio](#c.bio_add_folio "bio") \*bio, struct [folio](#c.bio_add_folio "folio") \*folio, size\_t len, size\_t off)
:   Attempt to add part of a folio to a bio.

**Parameters**

`struct bio *bio`
:   BIO to add to.

`struct folio *folio`
:   Folio to add.

`size_t len`
:   How many bytes from the folio to add.

`size_t off`
:   First byte in this folio to add.

**Description**

Filesystems that use folios can call this function instead of calling
[`bio_add_page()`](#c.bio_add_page "bio_add_page") for each page in the folio. If **off** is bigger than
PAGE\_SIZE, this function can create a bio\_vec that starts in a page
after the bv\_page. BIOs do not support folios that are 4GiB or larger.

**Return**

Whether the addition was successful.

unsigned int bio\_add\_vmalloc\_chunk(struct [bio](#c.bio_add_vmalloc_chunk "bio") \*bio, void \*vaddr, unsigned len)
:   add a vmalloc chunk to a bio

**Parameters**

`struct bio *bio`
:   destination bio

`void *vaddr`
:   vmalloc address to add

`unsigned len`
:   total length in bytes of the data to add

**Description**

Add data starting at **vaddr** to **bio** and return how many bytes were added.
This may be less than the amount originally asked. Returns 0 if no data
could be added to **bio**.

This helper calls `flush_kernel_vmap_range()` for the range added. For reads
the caller still needs to manually call `invalidate_kernel_vmap_range()` in
the completion handler.

bool bio\_add\_vmalloc(struct [bio](#c.bio_add_vmalloc "bio") \*bio, void \*vaddr, unsigned int len)
:   add a vmalloc region to a bio

**Parameters**

`struct bio *bio`
:   destination bio

`void *vaddr`
:   vmalloc address to add

`unsigned int len`
:   total length in bytes of the data to add

**Description**

Add data starting at **vaddr** to **bio**. Return `true` on success or `false` if
**bio** does not have enough space for the payload.

This helper calls `flush_kernel_vmap_range()` for the range added. For reads
the caller still needs to manually call `invalidate_kernel_vmap_range()` in
the completion handler.

void bio\_await(struct [bio](#c.bio_await "bio") \*bio, void \*priv, void (\*submit)(struct [bio](#c.bio_await "bio") \*bio, void \*priv))
:   call a function on a bio, and wait until it completes

**Parameters**

`struct bio *bio`
:   the bio which describes the I/O

`void *priv`
:   private data passed to **submit**

`void (*submit)(struct bio *bio, void *priv)`
:   function called to submit the bio

**Description**

Wait for the bio as well as any bio chained off it after executing the
passed in callback **submit**. The wait for the bio is set up before calling
**submit** to ensure that the completion is captured. If **submit** is `NULL`,
[`submit_bio()`](../core-api/kernel-api.html#c.submit_bio "submit_bio") is used instead to submit the bio.

**Note**

this overrides the bi\_private and bi\_end\_io fields in the bio.

int submit\_bio\_wait(struct [bio](#c.submit_bio_wait "bio") \*bio)
:   submit a bio, and wait until it completes

**Parameters**

`struct bio *bio`
:   The `struct bio` which describes the I/O

**Description**

Simple wrapper around [`submit_bio()`](../core-api/kernel-api.html#c.submit_bio "submit_bio"). Returns 0 on success, or the error from
[`bio_endio()`](#c.bio_endio "bio_endio") on failure.

WARNING: Unlike to how [`submit_bio()`](../core-api/kernel-api.html#c.submit_bio "submit_bio") is usually used, this function does not
result in bio reference to be consumed. The caller must drop the reference
on his own.

int bdev\_rw\_virt(struct block\_device \*bdev, sector\_t sector, void \*data, size\_t len, enum req\_op op)
:   synchronously read into / write from kernel mapping

**Parameters**

`struct block_device *bdev`
:   block device to access

`sector_t sector`
:   sector to access

`void *data`
:   data to read/write

`size_t len`
:   length in byte to read/write

`enum req_op op`
:   operation (e.g. REQ\_OP\_READ/REQ\_OP\_WRITE)

**Description**

Performs synchronous I/O to **bdev** for **data**/**len**. **data** must be in
the kernel direct mapping and not a vmalloc address.

void bio\_copy\_data(struct bio \*dst, struct bio \*src)
:   copy contents of data buffers from one bio to another

**Parameters**

`struct bio *dst`
:   destination bio

`struct bio *src`
:   source bio

**Description**

Stops when it reaches the end of either **src** or **dst** - that is, copies
min(src->bi\_size, dst->bi\_size) bytes (or the equivalent for lists of bios).

void bio\_endio(struct [bio](#c.bio_endio "bio") \*bio)
:   end I/O on a bio

**Parameters**

`struct bio *bio`
:   bio

**Description**

> [`bio_endio()`](#c.bio_endio "bio_endio") will end I/O on the whole bio. [`bio_endio()`](#c.bio_endio "bio_endio") is the preferred
> way to end I/O on a bio. No one should call `bi_end_io()` directly on a
> bio unless they own it and thus know that it has an end\_io function.
>
> [`bio_endio()`](#c.bio_endio "bio_endio") can be called several times on a bio that has been chained
> using [`bio_chain()`](#c.bio_chain "bio_chain"). The ->`bi_end_io()` function will only be called the
> last time.

struct [bio](#c.bio_split "bio") \*bio\_split(struct [bio](#c.bio_split "bio") \*bio, int sectors, gfp\_t gfp, struct bio\_set \*bs)
:   split a bio

**Parameters**

`struct bio *bio`
:   bio to split

`int sectors`
:   number of sectors to split from the front of **bio**

`gfp_t gfp`
:   gfp mask

`struct bio_set *bs`
:   bio set to allocate from

**Description**

Allocates and returns a new bio which represents **sectors** from the start of
**bio**, and updates **bio** to represent the remaining sectors.

Unless this is a discard request the newly allocated bio will point
to **bio**’s bi\_io\_vec. It is the caller’s responsibility to ensure that
neither **bio** nor **bs** are freed before the split bio.

void bio\_trim(struct [bio](#c.bio_trim "bio") \*bio, sector\_t offset, sector\_t size)
:   trim a bio

**Parameters**

`struct bio *bio`
:   bio to trim

`sector_t offset`
:   number of sectors to trim from the front of **bio**

`sector_t size`
:   size we want to trim **bio** to, in sectors

**Description**

This function is typically used for bios that are cloned and submitted
to the underlying device in parts.

int bioset\_init(struct bio\_set \*bs, unsigned int pool\_size, unsigned int front\_pad, int flags)
:   Initialize a bio\_set

**Parameters**

`struct bio_set *bs`
:   pool to initialize

`unsigned int pool_size`
:   Number of bio and bio\_vecs to cache in the mempool

`unsigned int front_pad`
:   Number of bytes to allocate in front of the returned bio

`int flags`
:   Flags to modify behavior, currently `BIOSET_NEED_BVECS`
    and `BIOSET_NEED_RESCUER`

**Description**

> Set up a bio\_set to be used with **bio\_alloc\_bioset**. Allows the caller
> to ask for a number of bytes to be allocated in front of the bio.
> Front pad allocation is useful for embedding the bio inside
> another structure, to avoid allocating extra data to go with the bio.
> Note that the bio must be embedded at the END of that structure always,
> or things will break badly.
> If `BIOSET_NEED_BVECS` is set in **flags**, a separate pool will be allocated
> for allocating iovecs. This pool is not needed e.g. for [`bio_init_clone()`](#c.bio_init_clone "bio_init_clone").
> If `BIOSET_NEED_RESCUER` is set, a workqueue is created which can be used
> to dispatch queued requests when the mempool runs out of space.

int seq\_open(struct [file](#c.seq_open "file") \*file, const struct seq\_operations \*op)
:   initialize sequential file

**Parameters**

`struct file *file`
:   file we initialize

`const struct seq_operations *op`
:   method table describing the sequence

**Description**

> [`seq_open()`](#c.seq_open "seq_open") sets **file**, associating it with a sequence described
> by **op**. **op->[`start()`](../networking/ieee802154.html#c.start "start")** sets the iterator up and returns the first
> element of sequence. **op->[`stop()`](../networking/ieee802154.html#c.stop "stop")** shuts it down. **op->`next()`**
> returns the next element of sequence. **op->`show()`** prints element
> into the buffer. In case of error ->[`start()`](../networking/ieee802154.html#c.start "start") and ->`next()` return
> ERR\_PTR(error). In the end of sequence they return `NULL`. ->`show()`
> returns 0 in case of success and negative number in case of error.
> Returning SEQ\_SKIP means “discard this element and move on”.

**Note**

seq\_open() will allocate a struct seq\_file and store its
:   pointer in **file->private\_data**. This pointer should not be modified.

ssize\_t seq\_read(struct [file](#c.seq_read "file") \*file, char \_\_user \*buf, size\_t size, loff\_t \*ppos)
:   ->read() method for sequential files.

**Parameters**

`struct file *file`
:   the file to read from

`char __user *buf`
:   the buffer to read to

`size_t size`
:   the maximum number of bytes to read

`loff_t *ppos`
:   the current position in the file

**Description**

> Ready-made ->f\_op->read()

loff\_t seq\_lseek(struct [file](#c.seq_lseek "file") \*file, loff\_t offset, int whence)
:   ->`llseek()` method for sequential files.

**Parameters**

`struct file *file`
:   the file in question

`loff_t offset`
:   new position

`int whence`
:   0 for absolute, 1 for relative position

**Description**

> Ready-made ->f\_op->`llseek()`

int seq\_release(struct [inode](#c.seq_release "inode") \*inode, struct [file](#c.seq_release "file") \*file)
:   free the structures associated with sequential file.

**Parameters**

`struct inode *inode`
:   its inode

`struct file *file`
:   file in question

**Description**

> Frees the structures associated with sequential file; can be used
> as ->f\_op->`release()` if you don’t have private data to destroy.

void seq\_escape\_mem(struct seq\_file \*m, const char \*src, size\_t len, unsigned int flags, const char \*esc)
:   print data into buffer, escaping some characters

**Parameters**

`struct seq_file *m`
:   target buffer

`const char *src`
:   source buffer

`size_t len`
:   size of source buffer

`unsigned int flags`
:   flags to pass to [`string_escape_mem()`](../core-api/kernel-api.html#c.string_escape_mem "string_escape_mem")

`const char *esc`
:   set of characters that need escaping

**Description**

Puts data into buffer, replacing each occurrence of character from
given class (defined by **flags** and **esc**) with printable escaped sequence.

Use `seq_has_overflowed()` to check for errors.

char \*mangle\_path(char \*s, const char \*p, const char \*esc)
:   mangle and copy path to buffer beginning

**Parameters**

`char *s`
:   buffer start

`const char *p`
:   beginning of path in above buffer

`const char *esc`
:   set of characters that need escaping

**Description**

> Copy the path from **p** to **s**, replacing each occurrence of character from
> **esc** with usual octal escape.
> Returns pointer past last written character in **s**, or NULL in case of
> failure.

int seq\_path(struct seq\_file \*m, const struct [path](#c.seq_path "path") \*path, const char \*esc)
:   seq\_file interface to print a pathname

**Parameters**

`struct seq_file *m`
:   the seq\_file handle

`const struct path *path`
:   the `struct path` to print

`const char *esc`
:   set of characters to escape in the output

**Description**

return the absolute path of ‘path’, as represented by the
dentry / mnt pair in the path parameter.

int seq\_file\_path(struct seq\_file \*m, struct [file](#c.seq_file_path "file") \*file, const char \*esc)
:   seq\_file interface to print a pathname of a file

**Parameters**

`struct seq_file *m`
:   the seq\_file handle

`struct file *file`
:   the [`struct file`](#c.file "file") to print

`const char *esc`
:   set of characters to escape in the output

**Description**

return the absolute path to the file.

int seq\_write(struct seq\_file \*seq, const void \*data, size\_t len)
:   write arbitrary data to buffer

**Parameters**

`struct seq_file *seq`
:   seq\_file identifying the buffer to which data should be written

`const void *data`
:   data address

`size_t len`
:   number of bytes

**Description**

Return 0 on success, non-zero otherwise.

void seq\_pad(struct seq\_file \*m, char c)
:   write padding spaces to buffer

**Parameters**

`struct seq_file *m`
:   seq\_file identifying the buffer to which data should be written

`char c`
:   the byte to append after padding if non-zero

struct hlist\_node \*seq\_hlist\_start(struct hlist\_head \*head, loff\_t pos)
:   start an iteration of a hlist

**Parameters**

`struct hlist_head *head`
:   the head of the hlist

`loff_t pos`
:   the start position of the sequence

**Description**

Called at seq\_file->op->[`start()`](../networking/ieee802154.html#c.start "start").

struct hlist\_node \*seq\_hlist\_start\_head(struct hlist\_head \*head, loff\_t pos)
:   start an iteration of a hlist

**Parameters**

`struct hlist_head *head`
:   the head of the hlist

`loff_t pos`
:   the start position of the sequence

**Description**

Called at seq\_file->op->[`start()`](../networking/ieee802154.html#c.start "start"). Call this function if you want to
print a header at the top of the output.

struct hlist\_node \*seq\_hlist\_next(void \*v, struct hlist\_head \*head, loff\_t \*ppos)
:   move to the next position of the hlist

**Parameters**

`void *v`
:   the current iterator

`struct hlist_head *head`
:   the head of the hlist

`loff_t *ppos`
:   the current position

**Description**

Called at seq\_file->op->`next()`.

struct hlist\_node \*seq\_hlist\_start\_rcu(struct hlist\_head \*head, loff\_t pos)
:   start an iteration of a hlist protected by RCU

**Parameters**

`struct hlist_head *head`
:   the head of the hlist

`loff_t pos`
:   the start position of the sequence

**Description**

Called at seq\_file->op->[`start()`](../networking/ieee802154.html#c.start "start").

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`hlist_add_head_rcu()`](../core-api/kernel-api.html#c.hlist_add_head_rcu "hlist_add_head_rcu")
as long as the traversal is guarded by [`rcu_read_lock()`](../core-api/kernel-api.html#c.rcu_read_lock "rcu_read_lock").

struct hlist\_node \*seq\_hlist\_start\_head\_rcu(struct hlist\_head \*head, loff\_t pos)
:   start an iteration of a hlist protected by RCU

**Parameters**

`struct hlist_head *head`
:   the head of the hlist

`loff_t pos`
:   the start position of the sequence

**Description**

Called at seq\_file->op->[`start()`](../networking/ieee802154.html#c.start "start"). Call this function if you want to
print a header at the top of the output.

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`hlist_add_head_rcu()`](../core-api/kernel-api.html#c.hlist_add_head_rcu "hlist_add_head_rcu")
as long as the traversal is guarded by [`rcu_read_lock()`](../core-api/kernel-api.html#c.rcu_read_lock "rcu_read_lock").

struct hlist\_node \*seq\_hlist\_next\_rcu(void \*v, struct hlist\_head \*head, loff\_t \*ppos)
:   move to the next position of the hlist protected by RCU

**Parameters**

`void *v`
:   the current iterator

`struct hlist_head *head`
:   the head of the hlist

`loff_t *ppos`
:   the current position

**Description**

Called at seq\_file->op->`next()`.

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`hlist_add_head_rcu()`](../core-api/kernel-api.html#c.hlist_add_head_rcu "hlist_add_head_rcu")
as long as the traversal is guarded by [`rcu_read_lock()`](../core-api/kernel-api.html#c.rcu_read_lock "rcu_read_lock").

struct hlist\_node \*seq\_hlist\_start\_percpu(struct hlist\_head \_\_percpu \*head, int \*cpu, loff\_t pos)
:   start an iteration of a percpu hlist array

**Parameters**

`struct hlist_head __percpu *head`
:   pointer to percpu array of `struct hlist_heads`

`int *cpu`
:   pointer to cpu “cursor”

`loff_t pos`
:   start position of sequence

**Description**

Called at seq\_file->op->[`start()`](../networking/ieee802154.html#c.start "start").

struct hlist\_node \*seq\_hlist\_next\_percpu(void \*v, struct hlist\_head \_\_percpu \*head, int \*cpu, loff\_t \*pos)
:   move to the next position of the percpu hlist array

**Parameters**

`void *v`
:   pointer to current hlist\_node

`struct hlist_head __percpu *head`
:   pointer to percpu array of `struct hlist_heads`

`int *cpu`
:   pointer to cpu “cursor”

`loff_t *pos`
:   start position of sequence

**Description**

Called at seq\_file->op->`next()`.

int register\_filesystem(struct file\_system\_type \*fs)
:   register a new filesystem

**Parameters**

`struct file_system_type * fs`
:   the file system structure

**Description**

> Adds the file system passed to the list of file systems the kernel
> is aware of for mount and other syscalls. Returns 0 on success,
> or a negative errno code on an error.
>
> The `struct file_system_type` that is passed is linked into the kernel
> structures and must not be freed until the file system has been
> unregistered.

int unregister\_filesystem(struct file\_system\_type \*fs)
:   unregister a file system

**Parameters**

`struct file_system_type * fs`
:   filesystem to unregister

**Description**

> Remove a file system that was previously successfully registered
> with the kernel. An error is returned if the file system is not found.
> Zero is returned on a success.
>
> Once this function has returned the `struct file_system_type` structure
> may be freed or reused.

void wbc\_attach\_fdatawrite\_inode(struct writeback\_control \*wbc, struct [inode](#c.wbc_attach_fdatawrite_inode "inode") \*inode)
:   associate wbc and inode for fdatawrite

**Parameters**

`struct writeback_control *wbc`
:   writeback\_control of interest

`struct inode *inode`
:   target inode

**Description**

This function is to be used by `filemap_writeback()`, which is an alternative
entry point into writeback code, and first ensures **inode** is associated with
a bdi\_writeback and attaches it to **wbc**.

void wbc\_detach\_inode(struct writeback\_control \*wbc)
:   disassociate wbc from inode and perform foreign detection

**Parameters**

`struct writeback_control *wbc`
:   writeback\_control of the just finished writeback

**Description**

To be called after a writeback attempt of an inode finishes and undoes
`wbc_attach_and_unlock_inode()`. Can be called under any context.

As concurrent write sharing of an inode is expected to be very rare and
memcg only tracks page ownership on first-use basis severely confining
the usefulness of such sharing, cgroup writeback tracks ownership
per-inode. While the support for concurrent write sharing of an inode
is deemed unnecessary, an inode being written to by different cgroups at
different points in time is a lot more common, and, more importantly,
charging only by first-use can too readily lead to grossly incorrect
behaviors (single foreign page can lead to gigabytes of writeback to be
incorrectly attributed).

To resolve this issue, cgroup writeback detects the majority dirtier of
an inode and transfers the ownership to it. To avoid unnecessary
oscillation, the detection mechanism keeps track of history and gives
out the switch verdict only if the foreign usage pattern is stable over
a certain amount of time and/or writeback attempts.

On each writeback attempt, **wbc** tries to detect the majority writer
using Boyer-Moore majority vote algorithm. In addition to the byte
count from the majority voting, it also counts the bytes written for the
current wb and the last round’s winner wb (max of last round’s current
wb, the winner from two rounds ago, and the last round’s majority
candidate). Keeping track of the historical winner helps the algorithm
to semi-reliably detect the most active writer even when it’s not the
absolute majority.

Once the winner of the round is determined, whether the winner is
foreign or not and how much IO time the round consumed is recorded in
inode->i\_wb\_frn\_history. If the amount of recorded foreign IO time is
over a certain threshold, the switch verdict is given.

void wbc\_account\_cgroup\_owner(struct writeback\_control \*wbc, struct [folio](#c.wbc_account_cgroup_owner "folio") \*folio, size\_t bytes)
:   account writeback to update inode cgroup ownership

**Parameters**

`struct writeback_control *wbc`
:   writeback\_control of the writeback in progress

`struct folio *folio`
:   folio being written out

`size_t bytes`
:   number of bytes being written out

**Description**

**bytes** from **folio** are about to written out during the writeback
controlled by **wbc**. Keep the book for foreign inode detection. See
[`wbc_detach_inode()`](#c.wbc_detach_inode "wbc_detach_inode").

void \_\_mark\_inode\_dirty(struct [inode](#c.__mark_inode_dirty "inode") \*inode, int flags)
:   internal function to mark an inode dirty

**Parameters**

`struct inode *inode`
:   inode to mark

`int flags`
:   what kind of dirty, e.g. I\_DIRTY\_SYNC. This can be a combination of
    multiple I\_DIRTY\_\* flags, except that I\_DIRTY\_TIME can’t be combined
    with I\_DIRTY\_PAGES.

**Description**

Mark an inode as dirty. We notify the filesystem, then update the inode’s
dirty flags. Then, if needed we add the inode to the appropriate dirty list.

Most callers should use `mark_inode_dirty()` or `mark_inode_dirty_sync()`
instead of calling this directly.

CAREFUL! We only add the inode to the dirty list if it is hashed or if it
refers to a blockdev. Unhashed inodes will never be added to the dirty list
even if they are later hashed, as they will have been marked dirty already.

In short, ensure you hash any inodes \_before\_ you start marking them dirty.

Note that for blockdevs, inode->dirtied\_when represents the dirtying time of
the block-special inode (/dev/hda1) itself. And the ->dirtied\_when field of
the kernel-internal blockdev inode represents the dirtying time of the
blockdev’s pages. This is why for I\_DIRTY\_PAGES we always use
page->mapping->host, so the page-dirtying time is recorded in the internal
blockdev inode.

void writeback\_inodes\_sb\_nr(struct super\_block \*sb, unsigned long nr, enum wb\_reason reason)
:   writeback dirty inodes from given super\_block

**Parameters**

`struct super_block *sb`
:   the superblock

`unsigned long nr`
:   the number of pages to write

`enum wb_reason reason`
:   reason why some writeback work initiated

**Description**

Start writeback on some inodes on this super\_block. No guarantees are made
on how many (if any) will be written, and this function does not wait
for IO completion of submitted IO.

void writeback\_inodes\_sb(struct super\_block \*sb, enum wb\_reason reason)
:   writeback dirty inodes from given super\_block

**Parameters**

`struct super_block *sb`
:   the superblock

`enum wb_reason reason`
:   reason why some writeback work was initiated

**Description**

Start writeback on some inodes on this super\_block. No guarantees are made
on how many (if any) will be written, and this function does not wait
for IO completion of submitted IO.

void try\_to\_writeback\_inodes\_sb(struct super\_block \*sb, enum wb\_reason reason)
:   try to start writeback if none underway

**Parameters**

`struct super_block *sb`
:   the superblock

`enum wb_reason reason`
:   reason why some writeback work was initiated

**Description**

Invoke \_\_writeback\_inodes\_sb\_nr if no writeback is currently underway.

void sync\_inodes\_sb(struct super\_block \*sb)
:   sync sb inode pages

**Parameters**

`struct super_block *sb`
:   the superblock

**Description**

This function writes and waits on any dirty inode belonging to this
super\_block.

int write\_inode\_now(struct [inode](#c.write_inode_now "inode") \*inode, int sync)
:   write an inode to disk

**Parameters**

`struct inode *inode`
:   inode to write to disk

`int sync`
:   whether the write should be synchronous or not

**Description**

This function commits an inode to disk immediately if it is dirty. This is
primarily needed by knfsd.

The caller must either have a ref on the inode or must have set I\_WILL\_FREE.

int sync\_inode\_metadata(struct [inode](#c.sync_inode_metadata "inode") \*inode, int wait)
:   write an inode to disk

**Parameters**

`struct inode *inode`
:   the inode to sync

`int wait`
:   wait for I/O to complete.

**Description**

Write an inode to disk and adjust its dirty state after completion.

**Note**

only writes the actual inode, no associated data or other metadata.

struct [file](#c.file "file") \*anon\_inode\_getfile(const char \*name, const struct file\_operations \*fops, void \*priv, int flags)
:   creates a new file instance by hooking it up to an anonymous inode, and a dentry that describe the “class” of the file

**Parameters**

`const char *name`
:   [in] name of the “class” of the new file

`const struct file_operations *fops`
:   [in] file operations for the new file

`void *priv`
:   [in] private data for the new file (will be file’s private\_data)

`int flags`
:   [in] flags

**Description**

Creates a new file by hooking it on a single inode. This is useful for files
that do not need to have a full-fledged inode in order to operate correctly.
All the files created with [`anon_inode_getfile()`](#c.anon_inode_getfile "anon_inode_getfile") will share a single inode,
hence saving memory and avoiding code duplication for the file/inode/dentry
setup. Returns the newly created file\* or an error pointer.

struct [file](#c.file "file") \*anon\_inode\_getfile\_fmode(const char \*name, const struct file\_operations \*fops, void \*priv, int flags, fmode\_t f\_mode)
:   creates a new file instance by hooking it up to an anonymous inode, and a dentry that describe the “class” of the file

**Parameters**

`const char *name`
:   [in] name of the “class” of the new file

`const struct file_operations *fops`
:   [in] file operations for the new file

`void *priv`
:   [in] private data for the new file (will be file’s private\_data)

`int flags`
:   [in] flags

`fmode_t f_mode`
:   [in] fmode

**Description**

Creates a new file by hooking it on a single inode. This is useful for files
that do not need to have a full-fledged inode in order to operate correctly.
All the files created with [`anon_inode_getfile()`](#c.anon_inode_getfile "anon_inode_getfile") will share a single inode,
hence saving memory and avoiding code duplication for the file/inode/dentry
setup. Allows setting the fmode. Returns the newly created file\* or an error
pointer.

struct [file](#c.file "file") \*anon\_inode\_create\_getfile(const char \*name, const struct file\_operations \*fops, void \*priv, int flags, const struct inode \*context\_inode)
:   Like [`anon_inode_getfile()`](#c.anon_inode_getfile "anon_inode_getfile"), but creates a new !S\_PRIVATE anon inode rather than reuse the singleton anon inode and calls the `inode_init_security_anon()` LSM hook.

**Parameters**

`const char *name`
:   [in] name of the “class” of the new file

`const struct file_operations *fops`
:   [in] file operations for the new file

`void *priv`
:   [in] private data for the new file (will be file’s private\_data)

`int flags`
:   [in] flags

`const struct inode *context_inode`
:   [in] the logical relationship with the new inode (optional)

**Description**

Create a new anonymous inode and file pair. This can be done for two
reasons:

* for the inode to have its own security context, so that LSMs can enforce
  policy on the inode’s creation;
* if the caller needs a unique inode, for example in order to customize
  the size returned by `fstat()`

The LSM may use **context\_inode** in `inode_init_security_anon()`, but a
reference to it is not held.

Returns the newly created file\* or an error pointer.

int anon\_inode\_getfd(const char \*name, const struct file\_operations \*fops, void \*priv, int flags)
:   creates a new file instance by hooking it up to an anonymous inode and a dentry that describe the “class” of the file

**Parameters**

`const char *name`
:   [in] name of the “class” of the new file

`const struct file_operations *fops`
:   [in] file operations for the new file

`void *priv`
:   [in] private data for the new file (will be file’s private\_data)

`int flags`
:   [in] flags

**Description**

Creates a new file by hooking it on a single inode. This is
useful for files that do not need to have a full-fledged inode in
order to operate correctly. All the files created with
[`anon_inode_getfd()`](#c.anon_inode_getfd "anon_inode_getfd") will use the same singleton inode, reducing
memory use and avoiding code duplication for the file/inode/dentry
setup. Returns a newly created file descriptor or an error code.

int setattr\_should\_drop\_sgid(struct mnt\_idmap \*idmap, const struct [inode](#c.setattr_should_drop_sgid "inode") \*inode)
:   determine whether the setgid bit needs to be removed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount **inode** was found from

`const struct inode *inode`
:   inode to check

**Description**

This function determines whether the setgid bit needs to be removed.
We retain backwards compatibility and require setgid bit to be removed
unconditionally if S\_IXGRP is set. Otherwise we have the exact same
requirements as [`setattr_prepare()`](#c.setattr_prepare "setattr_prepare") and [`setattr_copy()`](#c.setattr_copy "setattr_copy").

**Return**

ATTR\_KILL\_SGID if setgid bit needs to be removed, 0 otherwise.

int setattr\_should\_drop\_suidgid(struct mnt\_idmap \*idmap, struct [inode](#c.setattr_should_drop_suidgid "inode") \*inode)
:   determine whether the set{g,u}id bit needs to be dropped

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount **inode** was found from

`struct inode *inode`
:   inode to check

**Description**

This function determines whether the set{g,u}id bits need to be removed.
If the setuid bit needs to be removed ATTR\_KILL\_SUID is returned. If the
setgid bit needs to be removed ATTR\_KILL\_SGID is returned. If both
set{g,u}id bits need to be removed the corresponding mask of both flags is
returned.

**Return**

A mask of ATTR\_KILL\_S{G,U}ID indicating which - if any - setid bits
to remove, 0 otherwise.

int setattr\_prepare(struct mnt\_idmap \*idmap, struct [dentry](#c.setattr_prepare "dentry") \*dentry, struct iattr \*attr)
:   check if attribute changes to a dentry are allowed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct dentry *dentry`
:   dentry to check

`struct iattr *attr`
:   attributes to change

**Description**

Check if we are allowed to change the attributes contained in **attr**
in the given dentry. This includes the normal unix access permission
checks, as well as checks for rlimits and others. The function also clears
SGID bit from mode if user is not allowed to set it. Also file capabilities
and IMA extended attributes are cleared if ATTR\_KILL\_PRIV is set.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then
take care to map the inode according to **idmap** before checking
permissions. On non-idmapped mounts or if permission checking is to be
performed on the raw inode simply pass **nop\_mnt\_idmap**.

Should be called as the first thing in ->setattr implementations,
possibly after taking additional locks.

int inode\_newsize\_ok(const struct [inode](#c.inode_newsize_ok "inode") \*inode, loff\_t offset)
:   may this inode be truncated to a given size

**Parameters**

`const struct inode *inode`
:   the inode to be truncated

`loff_t offset`
:   the new size to assign to the inode

**Description**

inode\_newsize\_ok must be called with i\_rwsem held exclusively.

inode\_newsize\_ok will check filesystem limits and ulimits to check that the
new inode size is within limits. inode\_newsize\_ok will also send SIGXFSZ
when necessary. Caller must not proceed with inode size change if failure is
returned. **inode** must be a file (not directory), with appropriate
permissions to allow truncate (inode\_newsize\_ok does NOT check these
conditions).

**Return**

0 on success, -ve errno on failure

void setattr\_copy(struct mnt\_idmap \*idmap, struct [inode](#c.setattr_copy "inode") \*inode, const struct iattr \*attr)
:   copy simple metadata updates into the generic inode

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct inode *inode`
:   the inode to be updated

`const struct iattr *attr`
:   the new attributes

**Description**

setattr\_copy must be called with i\_rwsem held exclusively.

setattr\_copy updates the inode’s metadata with that specified
in attr on idmapped mounts. Necessary permission checks to determine
whether or not the S\_ISGID property needs to be removed are performed with
the correct idmapped mount permission helpers.
Noticeably missing is inode size update, which is more complex
as it requires pagecache updates.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then
take care to map the inode according to **idmap** before checking
permissions. On non-idmapped mounts or if permission checking is to be
performed on the raw inode simply pass **nop\_mnt\_idmap**.

The inode is not marked as dirty after this operation. The rationale is
that for “simple” filesystems, the `struct inode` is the inode storage.
The caller is free to mark the inode dirty afterwards if needed.

int notify\_change(struct mnt\_idmap \*idmap, struct [dentry](#c.notify_change "dentry") \*dentry, struct iattr \*attr, struct [delegated\_inode](#c.notify_change "delegated_inode") \*delegated\_inode)
:   modify attributes of a filesystem object

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`struct dentry *dentry`
:   object affected

`struct iattr *attr`
:   new attributes

`struct delegated_inode *delegated_inode`
:   returns inode, if the inode is delegated

**Description**

The caller must hold the i\_rwsem exclusively on the affected object.

If notify\_change discovers a delegation in need of breaking,
it will return -EWOULDBLOCK and return a reference to the inode in
delegated\_inode. The caller should then break the delegation and
retry. Because breaking a delegation may take a long time, the
caller should drop the i\_rwsem before doing so.

Alternatively, a caller may pass NULL for delegated\_inode. This may
be appropriate for callers that expect the underlying filesystem not
to be NFS exported. Also, passing NULL is fine for callers holding
the file open for write, as there can be no conflicting delegation in
that case.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then
take care to map the inode according to **idmap** before checking
permissions. On non-idmapped mounts or if permission checking is to be
performed on the raw inode simply pass **nop\_mnt\_idmap**.

char \*d\_path(const struct [path](#c.d_path "path") \*path, char \*buf, int buflen)
:   return the path of a dentry

**Parameters**

`const struct path *path`
:   path to report

`char *buf`
:   buffer to return value in

`int buflen`
:   buffer length

**Description**

Convert a dentry into an ASCII path name. If the entry has been deleted
the string “ (deleted)” is appended. Note that this is ambiguous.

Returns a pointer into the buffer or an error code if the path was
too long. Note: Callers should use the returned pointer, not the passed
in buffer, to use the name! The implementation often starts at an offset
into the buffer, and may leave 0 bytes at the start.

“buflen” should be positive.

int dax\_folio\_reset\_order(struct [folio](#c.dax_folio_reset_order "folio") \*folio)
:   Reset a compound DAX folio to order-0 pages

**Parameters**

`struct folio *folio`
:   The folio to reset

**Description**

Splits a compound folio back into individual order-0 pages,
clearing compound state and restoring pgmap pointers.

**Return**

the original folio order (0 if already order-0)

struct page \*dax\_layout\_busy\_page\_range(struct [address\_space](#c.address_space "address_space") \*mapping, loff\_t start, loff\_t end)
:   find first pinned page in **mapping**

**Parameters**

`struct address_space *mapping`
:   address space to scan for a page with ref count > 1

`loff_t start`
:   Starting offset. Page containing ‘start’ is included.

`loff_t end`
:   End offset. Page containing ‘end’ is included. If ‘end’ is LLONG\_MAX,
    pages from ‘start’ till the end of file are included.

**Description**

DAX requires ZONE\_DEVICE mapped pages. These pages are never
‘onlined’ to the page allocator so they are considered idle when
page->count == 1. A filesystem uses this interface to determine if
any page in the mapping is busy, i.e. for DMA, or other
`get_user_pages()` usages.

It is expected that the filesystem is holding locks to block the
establishment of new mappings in this address\_space. I.e. it expects
to be able to run [`unmap_mapping_range()`](../core-api/mm-api.html#c.unmap_mapping_range "unmap_mapping_range") and subsequently not race
`mapping_mapped()` becoming true.

ssize\_t dax\_iomap\_rw(struct kiocb \*iocb, struct iov\_iter \*iter, const struct iomap\_ops \*ops)
:   Perform I/O to a DAX file

**Parameters**

`struct kiocb *iocb`
:   The control block for this I/O

`struct iov_iter *iter`
:   The addresses to do I/O from or to

`const struct iomap_ops *ops`
:   iomap ops passed from the file system

**Description**

This function performs read and write operations to directly mapped
persistent memory. The callers needs to take care of read/write exclusion
and evicting any page cache pages in the region under I/O.

[vm\_fault\_t](../core-api/mm-api.html#c.vm_fault_t "vm_fault_t") dax\_iomap\_fault(struct vm\_fault \*vmf, unsigned int order, unsigned long \*pfnp, int \*iomap\_errp, const struct iomap\_ops \*ops)
:   handle a page fault on a DAX file

**Parameters**

`struct vm_fault *vmf`
:   The description of the fault

`unsigned int order`
:   Order of the page to fault in

`unsigned long *pfnp`
:   PFN to insert for synchronous faults if fsync is required

`int *iomap_errp`
:   Storage for detailed error code in case of error

`const struct iomap_ops *ops`
:   Iomap ops passed from the file system

**Description**

When a page fault occurs, filesystems may call this helper in
their fault handler for DAX files. [`dax_iomap_fault()`](#c.dax_iomap_fault "dax_iomap_fault") assumes the caller
has done all the necessary locking for page fault to proceed
successfully.

[vm\_fault\_t](../core-api/mm-api.html#c.vm_fault_t "vm_fault_t") dax\_finish\_sync\_fault(struct vm\_fault \*vmf, unsigned int order, unsigned long pfn)
:   finish synchronous page fault

**Parameters**

`struct vm_fault *vmf`
:   The description of the fault

`unsigned int order`
:   Order of entry to be inserted

`unsigned long pfn`
:   PFN to insert

**Description**

This function ensures that the file range touched by the page fault is
stored persistently on the media and handles inserting of appropriate page
table entry.

void simple\_rename\_timestamp(struct inode \*old\_dir, struct dentry \*old\_dentry, struct inode \*new\_dir, struct dentry \*new\_dentry)
:   update the various inode timestamps for rename

**Parameters**

`struct inode *old_dir`
:   old parent directory

`struct dentry *old_dentry`
:   dentry that is being renamed

`struct inode *new_dir`
:   new parent directory

`struct dentry *new_dentry`
:   target for rename

**Description**

POSIX mandates that the old and new parent directories have their ctime and
mtime updated, and that inodes of **old\_dentry** and **new\_dentry** (if any), have
their ctime updated.

int simple\_setattr(struct mnt\_idmap \*idmap, struct [dentry](#c.simple_setattr "dentry") \*dentry, struct [iattr](#c.simple_setattr "iattr") \*iattr)
:   setattr for simple filesystem

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the target mount

`struct dentry *dentry`
:   dentry

`struct iattr *iattr`
:   iattr structure

**Description**

Returns 0 on success, -error on failure.

simple\_setattr is a simple ->setattr implementation without a proper
implementation of size changes.

It can either be used for in-memory filesystems or special files
on simple regular filesystems. Anything that needs to change on-disk
or wire state on size changes needs its own setattr method.

ssize\_t simple\_read\_from\_buffer(void \_\_user \*to, size\_t count, loff\_t \*ppos, const void \*from, size\_t available)
:   copy data from the buffer to user space

**Parameters**

`void __user *to`
:   the user space buffer to read to

`size_t count`
:   the maximum number of bytes to read

`loff_t *ppos`
:   the current position in the buffer

`const void *from`
:   the buffer to read from

`size_t available`
:   the size of the buffer

**Description**

The [`simple_read_from_buffer()`](#c.simple_read_from_buffer "simple_read_from_buffer") function reads up to **count** bytes from the
buffer **from** at offset **ppos** into the user space address starting at **to**.

On success, the number of bytes read is returned and the offset **ppos** is
advanced by this number, or negative value is returned on error.

ssize\_t simple\_write\_to\_buffer(void \*to, size\_t available, loff\_t \*ppos, const void \_\_user \*from, size\_t count)
:   copy data from user space to the buffer

**Parameters**

`void *to`
:   the buffer to write to

`size_t available`
:   the size of the buffer

`loff_t *ppos`
:   the current position in the buffer

`const void __user *from`
:   the user space buffer to read from

`size_t count`
:   the maximum number of bytes to read

**Description**

The [`simple_write_to_buffer()`](#c.simple_write_to_buffer "simple_write_to_buffer") function reads up to **count** bytes from the user
space address starting at **from** into the buffer **to** at offset **ppos**.

On success, the number of bytes written is returned and the offset **ppos** is
advanced by this number, or negative value is returned on error.

ssize\_t memory\_read\_from\_buffer(void \*to, size\_t count, loff\_t \*ppos, const void \*from, size\_t available)
:   copy data from the buffer

**Parameters**

`void *to`
:   the kernel space buffer to read to

`size_t count`
:   the maximum number of bytes to read

`loff_t *ppos`
:   the current position in the buffer

`const void *from`
:   the buffer to read from

`size_t available`
:   the size of the buffer

**Description**

The [`memory_read_from_buffer()`](#c.memory_read_from_buffer "memory_read_from_buffer") function reads up to **count** bytes from the
buffer **from** at offset **ppos** into the kernel space address starting at **to**.

On success, the number of bytes read is returned and the offset **ppos** is
advanced by this number, or negative value is returned on error.

int generic\_encode\_ino32\_fh(struct [inode](#c.generic_encode_ino32_fh "inode") \*inode, \_\_u32 \*fh, int \*max\_len, struct [inode](#c.generic_encode_ino32_fh "inode") \*parent)
:   generic export\_operations->encode\_fh function

**Parameters**

`struct inode *inode`
:   the object to encode

`__u32 *fh`
:   where to store the file handle fragment

`int *max_len`
:   maximum length to store there (in 4 byte units)

`struct inode *parent`
:   parent directory inode, if wanted

**Description**

This generic encode\_fh function assumes that the 32 inode number
is suitable for locating an inode, and that the generation number
can be used to check that it is still valid. It places them in the
filehandle fragment where export\_decode\_fh expects to find them.

struct dentry \*generic\_fh\_to\_dentry(struct super\_block \*sb, struct [fid](#c.generic_fh_to_dentry "fid") \*fid, int fh\_len, int fh\_type, struct inode \*(\*get\_inode)(struct super\_block \*sb, u64 ino, u32 gen))
:   generic helper for the fh\_to\_dentry export operation

**Parameters**

`struct super_block *sb`
:   filesystem to do the file handle conversion on

`struct fid *fid`
:   file handle to convert

`int fh_len`
:   length of the file handle in bytes

`int fh_type`
:   type of file handle

`struct inode *(*get_inode) (struct super_block *sb, u64 ino, u32 gen)`
:   filesystem callback to retrieve inode

**Description**

This function decodes **fid** as long as it has one of the well-known
Linux filehandle types and calls **get\_inode** on it to retrieve the
inode for the object specified in the file handle.

struct dentry \*generic\_fh\_to\_parent(struct super\_block \*sb, struct [fid](#c.generic_fh_to_parent "fid") \*fid, int fh\_len, int fh\_type, struct inode \*(\*get\_inode)(struct super\_block \*sb, u64 ino, u32 gen))
:   generic helper for the fh\_to\_parent export operation

**Parameters**

`struct super_block *sb`
:   filesystem to do the file handle conversion on

`struct fid *fid`
:   file handle to convert

`int fh_len`
:   length of the file handle in bytes

`int fh_type`
:   type of file handle

`struct inode *(*get_inode) (struct super_block *sb, u64 ino, u32 gen)`
:   filesystem callback to retrieve inode

**Description**

This function decodes **fid** as long as it has one of the well-known
Linux filehandle types and calls **get\_inode** on it to retrieve the
inode for the \_parent\_ object specified in the file handle if it
is specified in the file handle, or NULL otherwise.

int simple\_fsync\_noflush(struct [file](#c.simple_fsync_noflush "file") \*file, loff\_t start, loff\_t end, int datasync)
:   generic fsync implementation for simple filesystems

**Parameters**

`struct file *file`
:   file to synchronize

`loff_t start`
:   start offset in bytes

`loff_t end`
:   end offset in bytes (inclusive)

`int datasync`
:   only synchronize essential metadata if true

**Description**

This function is an fsync handler for simple filesystems. It writes out
dirty data, inode (if dirty), but does not issue a cache flush.

int simple\_fsync(struct [file](#c.simple_fsync "file") \*file, loff\_t start, loff\_t end, int datasync)
:   fsync implementation for simple filesystems with flush

**Parameters**

`struct file *file`
:   file to synchronize

`loff_t start`
:   start offset in bytes

`loff_t end`
:   end offset in bytes (inclusive)

`int datasync`
:   only synchronize essential metadata if true

**Description**

This function is an fsync handler for simple filesystems. It writes out
dirty data, inode (if dirty), and issues a cache flush.

int generic\_check\_addressable(unsigned blocksize\_bits, u64 num\_blocks)
:   Check addressability of file system

**Parameters**

`unsigned blocksize_bits`
:   log of file system block size

`u64 num_blocks`
:   number of blocks in file system

**Description**

Determine whether a file system with **num\_blocks** blocks (and a
block size of 2\*\*\*\*blocksize\_bits\*\*) is addressable by the sector\_t
and page cache of the system. Return 0 if so and -EFBIG otherwise.

const char \*simple\_get\_link(struct [dentry](#c.simple_get_link "dentry") \*dentry, struct [inode](#c.simple_get_link "inode") \*inode, struct delayed\_call \*done)
:   generic helper to get the target of “fast” symlinks

**Parameters**

`struct dentry *dentry`
:   not used here

`struct inode *inode`
:   the symlink inode

`struct delayed_call *done`
:   not used here

**Description**

Generic helper for filesystems to use for symlink inodes where a pointer to
the symlink target is stored in ->i\_link. NOTE: this isn’t normally called,
since as an optimization the path lookup code uses any non-NULL ->i\_link
directly, without calling ->`get_link()`. But ->`get_link()` still must be set,
to mark the inode\_operations as being for a symlink.

**Return**

the symlink target

int generic\_ci\_d\_compare(const struct [dentry](#c.generic_ci_d_compare "dentry") \*dentry, unsigned int len, const char \*str, const struct qstr \*name)
:   generic d\_compare implementation for casefolding filesystems

**Parameters**

`const struct dentry *dentry`
:   dentry whose name we are checking against

`unsigned int len`
:   len of name of dentry

`const char *str`
:   str pointer to name of dentry

`const struct qstr *name`
:   Name to compare against

**Return**

0 if names match, 1 if mismatch, or -ERRNO

int generic\_ci\_d\_hash(const struct [dentry](#c.generic_ci_d_hash "dentry") \*dentry, struct qstr \*str)
:   generic d\_hash implementation for casefolding filesystems

**Parameters**

`const struct dentry *dentry`
:   dentry of the parent directory

`struct qstr *str`
:   qstr of name whose hash we should fill in

**Return**

0 if hash was successful or unchanged, and -EINVAL on error

int generic\_ci\_match(const struct inode \*parent, const struct qstr \*name, const struct qstr \*folded\_name, const u8 \*de\_name, u32 de\_name\_len)
:   Match a name (case-insensitively) with a dirent. This is a filesystem helper for comparison with directory entries. generic\_ci\_d\_compare should be used in VFS’ ->d\_compare instead.

**Parameters**

`const struct inode *parent`
:   Inode of the parent of the dirent under comparison

`const struct qstr *name`
:   name under lookup.

`const struct qstr *folded_name`
:   Optional pre-folded name under lookup

`const u8 *de_name`
:   Dirent name.

`u32 de_name_len`
:   dirent name length.

**Description**

Test whether a case-insensitive directory entry matches the filename
being searched. If **folded\_name** is provided, it is used instead of
recalculating the casefold of **name**.

**Return**

> 0 if the directory entry matches, 0 if it doesn’t match, or
< 0 on error.

void generic\_set\_sb\_d\_ops(struct super\_block \*sb)
:   helper for choosing the set of filesystem-wide dentry operations for the enabled features

**Parameters**

`struct super_block *sb`
:   superblock to be configured

**Description**

Filesystems supporting casefolding and/or fscrypt can call this
helper at mount-time to configure default dentry\_operations to the
best set of dentry operations required for the enabled features.
The helper must be called after these have been configured, but
before the root dentry is created.

bool inode\_maybe\_inc\_iversion(struct [inode](#c.inode_maybe_inc_iversion "inode") \*inode, bool force)
:   increments i\_version

**Parameters**

`struct inode *inode`
:   inode with the i\_version that should be updated

`bool force`
:   increment the counter even if it’s not necessary?

**Description**

Every time the inode is modified, the i\_version field must be seen to have
changed by any observer.

If “force” is set or the QUERIED flag is set, then ensure that we increment
the value, and clear the queried flag.

In the common case where neither is set, then we can return “false” without
updating i\_version.

If this function returns false, and no other metadata has changed, then we
can avoid logging the metadata.

u64 inode\_query\_iversion(struct [inode](#c.inode_query_iversion "inode") \*inode)
:   read i\_version for later use

**Parameters**

`struct inode *inode`
:   inode from which i\_version should be read

**Description**

Read the inode i\_version counter. This should be used by callers that wish
to store the returned i\_version for later comparison. This will guarantee
that a later query of the i\_version will result in a different value if
anything has changed.

In this implementation, we fetch the current value, set the QUERIED flag and
then try to swap it into place with a cmpxchg, if it wasn’t already set. If
that fails, we try again with the newly fetched value from the cmpxchg.

struct timespec64 simple\_inode\_init\_ts(struct [inode](#c.simple_inode_init_ts "inode") \*inode)
:   initialize the timestamps for a new inode

**Parameters**

`struct inode *inode`
:   inode to be initialized

**Description**

When a new inode is created, most filesystems set the timestamps to the
current time. Add a helper to do this.

struct dentry \*simple\_start\_creating(struct dentry \*parent, const char \*name)
:   prepare to create a given name

**Parameters**

`struct dentry *parent`
:   directory in which to prepare to create the name

`const char *name`
:   the name to be created

**Description**

Required lock is taken and a lookup in performed prior to creating an
object in a directory. No permission checking is performed.

**Return**

a negative dentry on which [`vfs_create()`](#c.vfs_create "vfs_create") or similar may
be attempted, or an error.

int posix\_acl\_chmod(struct mnt\_idmap \*idmap, struct [dentry](#c.posix_acl_chmod "dentry") \*dentry, umode\_t mode)
:   chmod a posix acl

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount **inode** was found from

`struct dentry *dentry`
:   dentry to check permissions on

`umode_t mode`
:   the new mode of **inode**

**Description**

If the dentry has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then
take care to map the inode according to **idmap** before checking
permissions. On non-idmapped mounts or if permission checking is to be
performed on the raw inode simply pass **nop\_mnt\_idmap**.

int posix\_acl\_update\_mode(struct mnt\_idmap \*idmap, struct [inode](#c.posix_acl_update_mode "inode") \*inode, umode\_t \*mode\_p, struct posix\_acl \*\*acl)
:   update mode in set\_acl

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount **inode** was found from

`struct inode *inode`
:   target inode

`umode_t *mode_p`
:   mode (pointer) for update

`struct posix_acl **acl`
:   acl pointer

**Description**

Update the file mode when setting an ACL: compute the new file permission
bits based on the ACL. In addition, if the ACL is equivalent to the new
file mode, set **\*acl** to NULL to indicate that no ACL should be set.

As with chmod, clear the setgid bit if the caller is not in the owning group
or capable of CAP\_FSETID (see inode\_change\_ok).

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then
take care to map the inode according to **idmap** before checking
permissions. On non-idmapped mounts or if permission checking is to be
performed on the raw inode simply pass **nop\_mnt\_idmap**.

Called from set\_acl inode operations.

struct posix\_acl \*posix\_acl\_from\_xattr(struct user\_namespace \*userns, const void \*value, size\_t size)
:   convert POSIX ACLs from backing store to VFS format

**Parameters**

`struct user_namespace *userns`
:   the filesystem’s idmapping

`const void *value`
:   the uapi representation of POSIX ACLs

`size_t size`
:   the size of **void**

**Description**

Filesystems that store POSIX ACLs in the unaltered uapi format should use
[`posix_acl_from_xattr()`](#c.posix_acl_from_xattr "posix_acl_from_xattr") when reading them from the backing store and
converting them into the `struct posix_acl` VFS format. The helper is
specifically intended to be called from the acl inode operation.

The [`posix_acl_from_xattr()`](#c.posix_acl_from_xattr "posix_acl_from_xattr") function will map the raw {g,u}id values stored
in ACL\_{GROUP,USER} entries into idmapping in **userns**.

Note that [`posix_acl_from_xattr()`](#c.posix_acl_from_xattr "posix_acl_from_xattr") does not take idmapped mounts into account.
If it did it calling it from the get acl inode operation would return POSIX
ACLs mapped according to an idmapped mount which would mean that the value
couldn’t be cached for the filesystem. Idmapped mounts are taken into
account on the fly during permission checking or right at the VFS -
userspace boundary before reporting them to the user.

**Return**

Allocated `struct posix_acl` on success, NULL for a valid header but
without actual POSIX ACL entries, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") encoded error code.

int vfs\_set\_acl(struct mnt\_idmap \*idmap, struct [dentry](#c.vfs_set_acl "dentry") \*dentry, const char \*acl\_name, struct posix\_acl \*kacl)
:   set posix acls

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   the dentry based on which to set the posix acls

`const char *acl_name`
:   the name of the posix acl

`struct posix_acl *kacl`
:   the posix acls in the appropriate VFS format

**Description**

This function sets **kacl**. The caller must all `posix_acl_release()` on **kacl**
afterwards.

**Return**

On success 0, on error negative errno.

struct posix\_acl \*vfs\_get\_acl(struct mnt\_idmap \*idmap, struct [dentry](#c.vfs_get_acl "dentry") \*dentry, const char \*acl\_name)
:   get posix acls

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   the dentry based on which to retrieve the posix acls

`const char *acl_name`
:   the name of the posix acl

**Description**

This function retrieves **kacl** from the filesystem. The caller must all
`posix_acl_release()` on **kacl**.

**Return**

On success POSIX ACLs in VFS format, on error negative errno.

int vfs\_remove\_acl(struct mnt\_idmap \*idmap, struct [dentry](#c.vfs_remove_acl "dentry") \*dentry, const char \*acl\_name)
:   remove posix acls

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   the dentry based on which to retrieve the posix acls

`const char *acl_name`
:   the name of the posix acl

**Description**

This function removes posix acls.

**Return**

On success 0, on error negative errno.

void fill\_mg\_cmtime(struct kstat \*stat, u32 request\_mask, struct [inode](#c.fill_mg_cmtime "inode") \*inode)
:   Fill in the mtime and ctime and flag ctime as QUERIED

**Parameters**

`struct kstat *stat`
:   where to store the resulting values

`u32 request_mask`
:   STATX\_\* values requested

`struct inode *inode`
:   inode from which to grab the c/mtime

**Description**

Given **inode**, grab the ctime and mtime out if it and store the result
in **stat**. When fetching the value, flag it as QUERIED (if not already)
so the next write will record a distinct timestamp.

NB: The QUERIED flag is tracked in the ctime, but we set it there even
if only the mtime was requested, as that ensures that the next mtime
change will be distinct.

void generic\_fillattr(struct mnt\_idmap \*idmap, u32 request\_mask, struct [inode](#c.generic_fillattr "inode") \*inode, struct kstat \*stat)
:   Fill in the basic attributes from the inode struct

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount the inode was found from

`u32 request_mask`
:   statx request\_mask

`struct inode *inode`
:   Inode to use as the source

`struct kstat *stat`
:   Where to fill in the attributes

**Description**

Fill in the basic attributes in the kstat structure from data that’s to be
found on the VFS inode structure. This is the default if no getattr inode
operation is supplied.

If the inode has been found through an idmapped mount the idmap of
the vfsmount must be passed through **idmap**. This function will then
take care to map the inode according to **idmap** before filling in the
uid and gid filds. On non-idmapped mounts or if permission checking is to be
performed on the raw inode simply pass **nop\_mnt\_idmap**.

void generic\_fill\_statx\_attr(struct [inode](#c.generic_fill_statx_attr "inode") \*inode, struct kstat \*stat)
:   Fill in the statx attributes from the inode flags

**Parameters**

`struct inode *inode`
:   Inode to use as the source

`struct kstat *stat`
:   Where to fill in the attribute flags

**Description**

Fill in the STATX\_ATTR\_\* flags in the kstat structure for properties of the
inode that are published on i\_flags and enforced by the VFS.

void generic\_fill\_statx\_atomic\_writes(struct kstat \*stat, unsigned int unit\_min, unsigned int unit\_max, unsigned int unit\_max\_opt)
:   Fill in atomic writes statx attributes

**Parameters**

`struct kstat *stat`
:   Where to fill in the attribute flags

`unsigned int unit_min`
:   Minimum supported atomic write length in bytes

`unsigned int unit_max`
:   Maximum supported atomic write length in bytes

`unsigned int unit_max_opt`
:   Optimised maximum supported atomic write length in bytes

**Description**

Fill in the STATX{\_ATTR}\_WRITE\_ATOMIC flags in the kstat structure from
atomic write unit\_min and unit\_max values.

int vfs\_getattr\_nosec(const struct [path](#c.vfs_getattr_nosec "path") \*path, struct kstat \*stat, u32 request\_mask, unsigned int query\_flags)
:   getattr without security checks

**Parameters**

`const struct path *path`
:   file to get attributes from

`struct kstat *stat`
:   structure to return attributes in

`u32 request_mask`
:   STATX\_xxx flags indicating what the caller wants

`unsigned int query_flags`
:   Query mode (AT\_STATX\_SYNC\_TYPE)

**Description**

Get attributes without calling security\_inode\_getattr.

Currently the only caller other than vfs\_getattr is internal to the
filehandle lookup code, which uses only the inode number and returns no
attributes to any user. Any other code probably wants vfs\_getattr.

int vfs\_fsync\_range(struct [file](#c.vfs_fsync_range "file") \*file, loff\_t start, loff\_t end, int datasync)
:   helper to sync a range of data & metadata to disk

**Parameters**

`struct file *file`
:   file to sync

`loff_t start`
:   offset in bytes of the beginning of data range to sync

`loff_t end`
:   offset in bytes of the end of data range (inclusive)

`int datasync`
:   perform only datasync

**Description**

Write back data in range **start**..\*\*end\*\* and metadata for **file** to disk. If
**datasync** is set only metadata needed to access modified file data is
written.

int vfs\_fsync(struct [file](#c.vfs_fsync "file") \*file, int datasync)
:   perform a fsync or fdatasync on a file

**Parameters**

`struct file *file`
:   file to sync

`int datasync`
:   only perform a fdatasync operation

**Description**

Write back data and metadata for **file** to disk. If **datasync** is
set only metadata needed to access modified file data is written.

int \_\_vfs\_setxattr\_locked(struct mnt\_idmap \*idmap, struct [dentry](#c.__vfs_setxattr_locked "dentry") \*dentry, const char \*name, const void \*value, size\_t size, int flags, struct [delegated\_inode](#c.__vfs_setxattr_locked "delegated_inode") \*delegated\_inode)
:   set an extended attribute while holding the inode lock

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount of the target inode

`struct dentry *dentry`
:   object to perform setxattr on

`const char *name`
:   xattr name to set

`const void *value`
:   value to set **name** to

`size_t size`
:   size of **value**

`int flags`
:   flags to pass into filesystem operations

`struct delegated_inode *delegated_inode`
:   on return, will contain an inode pointer that
    a delegation was broken on, NULL if none.

ssize\_t vfs\_listxattr(struct [dentry](#c.vfs_listxattr "dentry") \*dentry, char \*list, size\_t size)
:   retrieve 0 separated list of xattr names

**Parameters**

`struct dentry *dentry`
:   the dentry from whose inode the xattr names are retrieved

`char *list`
:   buffer to store xattr names into

`size_t size`
:   size of the buffer

**Description**

This function returns the names of all xattrs associated with the
inode of **dentry**.

Note, for legacy reasons the [`vfs_listxattr()`](#c.vfs_listxattr "vfs_listxattr") function lists POSIX
ACLs as well. Since POSIX ACLs are decoupled from IOP\_XATTR the
[`vfs_listxattr()`](#c.vfs_listxattr "vfs_listxattr") function doesn’t check for this flag since a
filesystem could implement POSIX ACLs without implementing any other
xattrs.

However, since all codepaths that remove IOP\_XATTR also assign of
inode operations that either don’t implement or implement a stub
->`listxattr()` operation.

**Return**

On success, the size of the buffer that was used. On error a
negative error code.

int \_\_vfs\_removexattr\_locked(struct mnt\_idmap \*idmap, struct [dentry](#c.__vfs_removexattr_locked "dentry") \*dentry, const char \*name, struct [delegated\_inode](#c.__vfs_removexattr_locked "delegated_inode") \*delegated\_inode)
:   set an extended attribute while holding the inode lock

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount of the target inode

`struct dentry *dentry`
:   object to perform setxattr on

`const char *name`
:   name of xattr to remove

`struct delegated_inode *delegated_inode`
:   on return, will contain an inode pointer that
    a delegation was broken on, NULL if none.

ssize\_t generic\_listxattr(struct [dentry](#c.generic_listxattr "dentry") \*dentry, char \*buffer, size\_t buffer\_size)
:   run through a dentry’s xattr `list()` operations

**Parameters**

`struct dentry *dentry`
:   dentry to list the xattrs

`char *buffer`
:   result buffer

`size_t buffer_size`
:   size of **buffer**

**Description**

Combine the results of the `list()` operation from every xattr\_handler in the
xattr\_handler stack.

Note that this will not include the entries for POSIX ACLs.

const char \*xattr\_full\_name(const struct xattr\_handler \*handler, const char \*name)
:   Compute full attribute name from suffix

**Parameters**

`const struct xattr_handler *handler`
:   handler of the xattr\_handler operation

`const char *name`
:   name passed to the xattr\_handler operation

**Description**

The get and set xattr handler operations are called with the remainder of
the attribute name after skipping the handler’s prefix: for example, “foo”
is passed to the get operation of a handler with prefix “user.” to get
attribute “user.foo”. The full name is still “there” in the name though.

**Note**

the list xattr handler operation when called from the vfs is passed a
NULL name; some file systems use this operation internally, with varying
semantics.

int mnt\_get\_write\_access(struct vfsmount \*m)
:   get write access to a mount without freeze protection

**Parameters**

`struct vfsmount *m`
:   the mount on which to take a write

**Description**

This tells the low-level filesystem that a write is about to be performed to
it, and makes sure that writes are allowed (mnt it read-write) before
returning success. This operation does not protect against filesystem being
frozen. When the write operation is finished, [`mnt_put_write_access()`](#c.mnt_put_write_access "mnt_put_write_access") must be
called. This is effectively a refcount.

int mnt\_want\_write(struct vfsmount \*m)
:   get write access to a mount

**Parameters**

`struct vfsmount *m`
:   the mount on which to take a write

**Description**

This tells the low-level filesystem that a write is about to be performed to
it, and makes sure that writes are allowed (mount is read-write, filesystem
is not frozen) before returning success. When the write operation is
finished, [`mnt_drop_write()`](#c.mnt_drop_write "mnt_drop_write") must be called. This is effectively a refcount.

int mnt\_want\_write\_file(struct [file](#c.mnt_want_write_file "file") \*file)
:   get write access to a file’s mount

**Parameters**

`struct file *file`
:   the file who’s mount on which to take a write

**Description**

This is like mnt\_want\_write, but if the file is already open for writing it
skips incrementing mnt\_writers (since the open file already has a reference)
and instead only does the freeze protection and the check for emergency r/o
remounts. This must be paired with mnt\_drop\_write\_file.

void mnt\_put\_write\_access(struct vfsmount \*mnt)
:   give up write access to a mount

**Parameters**

`struct vfsmount *mnt`
:   the mount on which to give up write access

**Description**

Tells the low-level filesystem that we are done
performing writes to it. Must be matched with
[`mnt_get_write_access()`](#c.mnt_get_write_access "mnt_get_write_access") call above.

void mnt\_drop\_write(struct vfsmount \*mnt)
:   give up write access to a mount

**Parameters**

`struct vfsmount *mnt`
:   the mount on which to give up write access

**Description**

Tells the low-level filesystem that we are done performing writes to it and
also allows filesystem to be frozen again. Must be matched with
[`mnt_want_write()`](#c.mnt_want_write "mnt_want_write") call above.

struct vfsmount \*vfs\_create\_mount(struct fs\_context \*fc)
:   Create a mount for a configured superblock

**Parameters**

`struct fs_context *fc`
:   The configuration context with the superblock attached

**Description**

Create a mount to an already configured superblock. If necessary, the
caller should invoke [`vfs_get_tree()`](#c.vfs_get_tree "vfs_get_tree") before calling this.

Note that this does not attach the mount to anything.

bool path\_is\_mountpoint(const struct [path](#c.path_is_mountpoint "path") \*path)
:   Check if path is a mount in the current namespace.

**Parameters**

`const struct path *path`
:   path to check

**Description**

> `d_mountpoint()` can only be used reliably to establish if a dentry is
> not mounted in any namespace and that common case is handled inline.
> `d_mountpoint()` isn’t aware of the possibility there may be multiple
> mounts using a given dentry in a different namespace. This function
> checks if the passed in path is a mountpoint rather than the dentry
> alone.

int may\_umount\_tree(struct vfsmount \*m)
:   check if a mount tree is busy

**Parameters**

`struct vfsmount *m`
:   root of mount tree

**Description**

This is called to check if a tree of mounts has any
open files, pwds, chroots or sub mounts that are
busy.

int may\_umount(struct vfsmount \*mnt)
:   check if a mount point is busy

**Parameters**

`struct vfsmount *mnt`
:   root of mount

**Description**

This is called to check if a mount point has any
open files, pwds, chroots or sub mounts. If the
mount has sub mounts this will return busy
regardless of whether the sub mounts are busy.

Doesn’t take quota and stuff into account. IOW, in some cases it will
give false negatives. The main reason why it’s here is that we need
a non-destructive way to look for easily umountable filesystems.

struct vfsmount \*clone\_private\_mount(const struct [path](#c.clone_private_mount "path") \*path)
:   create a private clone of a path

**Parameters**

`const struct path *path`
:   path to clone

**Description**

This creates a new vfsmount, which will be the clone of **path**. The new mount
will not be attached anywhere in the namespace and will be private (i.e.
changes to the originating mount won’t be propagated into this).

This assumes caller has called or done the equivalent of `may_mount()`.

Release with `mntput()`.

void mnt\_set\_expiry(struct vfsmount \*mnt, struct list\_head \*expiry\_list)
:   Put a mount on an expiration list

**Parameters**

`struct vfsmount *mnt`
:   The mount to list.

`struct list_head *expiry_list`
:   The list to add the mount to.

## The proc filesystem

### sysctl interface

int proc\_dostring(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read a string sysctl

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes a string from/to the user buffer. If the kernel
buffer provided is not large enough to hold the string, the
string is truncated. The copied string is `NULL-terminated`.
If the string is being read by the user process, it is copied
and a newline ‘n’ is added. It is truncated if the buffer is
not large enough.

Returns 0 on success.

int proc\_dobool(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read/write a bool

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes one integer value from/to the user buffer,
treated as an ASCII string.

table->data must point to a bool variable and table->maxlen must
be sizeof(bool).

Returns 0 on success.

int proc\_dointvec(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read a vector of integers

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes up to table->maxlen/sizeof(unsigned int) integer
values from/to the user buffer, treated as an ASCII string.

Returns 0 on success.

int proc\_douintvec(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read a vector of unsigned integers

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes up to table->maxlen/sizeof(unsigned int) unsigned integer
values from/to the user buffer, treated as an ASCII string.

Returns 0 on success.

int proc\_dointvec\_minmax(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read a vector of integers with min/max values

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes up to table->maxlen/sizeof(unsigned int) integer
values from/to the user buffer, treated as an ASCII string.

This routine will ensure the values are within the range specified by
table->extra1 (min) and table->extra2 (max).

Returns 0 on success or -EINVAL when the range check fails and
SYSCTL\_USER\_TO\_KERN(dir) == true

int proc\_douintvec\_minmax(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read a vector of unsigned ints with min/max values

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes up to table->maxlen/sizeof(unsigned int) unsigned integer
values from/to the user buffer, treated as an ASCII string. Negative
strings are not allowed.

When changing the kernel variable, this routine will ensure the values
are within the range specified by table->extra1 (min) and table->extra2
(max). And Check that the values are less than UINT\_MAX to avoid having to
support wrap around uses from userspace.

Returns 0 on success or -ERANGE when range check failes and
SYSCTL\_USER\_TO\_KERN(dir) == true

int proc\_dou8vec\_minmax(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read a vector of unsigned chars with min/max values

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes up to table->maxlen/sizeof(u8) unsigned chars
values from/to the user buffer, treated as an ASCII string. Negative
strings are not allowed.

This routine will ensure the values are within the range specified by
table->extra1 (min) and table->extra2 (max).

Returns 0 on success or an error on SYSCTL\_USER\_TO\_KERN(dir) == true
and the range check fails.

int proc\_doulongvec\_minmax(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read a vector of long integers with min/max values

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

Reads/writes up to table->maxlen/sizeof(unsigned long) unsigned long
values from/to the user buffer, treated as an ASCII string.

This routine will ensure the values are within the range specified by
table->extra1 (min) and table->extra2 (max).

Returns 0 on success.

int proc\_do\_large\_bitmap(const struct ctl\_table \*table, int dir, void \*buffer, size\_t \*lenp, loff\_t \*ppos)
:   read/write from/to a large bitmap

**Parameters**

`const struct ctl_table *table`
:   the sysctl table

`int dir`
:   `TRUE` if this is a write to the sysctl file

`void *buffer`
:   the user buffer

`size_t *lenp`
:   the size of the user buffer

`loff_t *ppos`
:   file position

**Description**

The bitmap is stored at table->data and the bitmap length (in bits)
in table->maxlen.

We use a range comma separated format (e.g. 1,3-4,10-10) so that
large bitmaps may be represented in a compact manner. Writing into
the file will clear the bitmap then update it with the given input.

Returns 0 on success.

### proc filesystem interface

void proc\_flush\_pid(struct [pid](#c.proc_flush_pid "pid") \*pid)
:   Remove dcache entries for **pid** from the /proc dcache.

**Parameters**

`struct pid *pid`
:   pid that should be flushed.

**Description**

This function walks a list of inodes (that belong to any proc
filesystem) that are attached to the pid and flushes them from
the dentry cache.

It is safe and reasonable to cache /proc entries for a task until
that task exits. After that they just clog up the dcache with
useless entries, possibly causing useful dcache entries to be
flushed instead. This routine is provided to flush those useless
dcache entries when a process is reaped.

**NOTE**

This routine is just an optimization so it does not guarantee
:   that no dcache entries will exist after a process is reaped
    it just makes it very unlikely that any will persist.

## Events based on file descriptors

void eventfd\_signal\_mask(struct eventfd\_ctx \*ctx, \_\_poll\_t mask)
:   Increment the event counter

**Parameters**

`struct eventfd_ctx *ctx`
:   [in] Pointer to the eventfd context.

`__poll_t mask`
:   [in] poll mask

**Description**

This function is supposed to be called by the kernel in paths that do not
allow sleeping. In this function we allow the counter to reach the ULLONG\_MAX
value, and we signal this as overflow condition by returning a EPOLLERR
to poll(2).

void eventfd\_ctx\_put(struct eventfd\_ctx \*ctx)
:   Releases a reference to the internal eventfd context.

**Parameters**

`struct eventfd_ctx *ctx`
:   [in] Pointer to eventfd context.

**Description**

The eventfd context reference must have been previously acquired either
with [`eventfd_ctx_fdget()`](#c.eventfd_ctx_fdget "eventfd_ctx_fdget") or [`eventfd_ctx_fileget()`](#c.eventfd_ctx_fileget "eventfd_ctx_fileget").

int eventfd\_ctx\_remove\_wait\_queue(struct eventfd\_ctx \*ctx, wait\_queue\_entry\_t \*wait, \_\_u64 \*cnt)
:   Read the current counter and removes wait queue.

**Parameters**

`struct eventfd_ctx *ctx`
:   [in] Pointer to eventfd context.

`wait_queue_entry_t *wait`
:   [in] Wait queue to be removed.

`__u64 *cnt`
:   [out] Pointer to the 64-bit counter value.

**Description**

Returns `0` if successful, or the following error codes:

`-EAGAIN`
:   : The operation would have blocked.

This is used to atomically remove a wait queue entry from the eventfd wait
queue head, and read/reset the counter value.

struct [file](#c.file "file") \*eventfd\_fget(int fd)
:   Acquire a reference of an eventfd file descriptor.

**Parameters**

`int fd`
:   [in] Eventfd file descriptor.

**Description**

Returns a pointer to the eventfd file structure in case of success, or the
following error pointer:

`-EBADF`
:   : Invalid **fd** file descriptor.

`-EINVAL`
:   : The **fd** file descriptor is not an eventfd file.

struct eventfd\_ctx \*eventfd\_ctx\_fdget(int fd)
:   Acquires a reference to the internal eventfd context.

**Parameters**

`int fd`
:   [in] Eventfd file descriptor.

**Description**

Returns a pointer to the internal eventfd context, otherwise the error
pointers returned by the following functions:

eventfd\_fget

struct eventfd\_ctx \*eventfd\_ctx\_fileget(struct [file](#c.eventfd_ctx_fileget "file") \*file)
:   Acquires a reference to the internal eventfd context.

**Parameters**

`struct file *file`
:   [in] Eventfd file pointer.

**Description**

Returns a pointer to the internal eventfd context, otherwise the error
pointer:

`-EINVAL`
:   : The **fd** file descriptor is not an eventfd file.

## eventpoll (epoll) interfaces

int ep\_events\_available(struct eventpoll \*ep)
:   Checks if ready events might be available.

**Parameters**

`struct eventpoll *ep`
:   Pointer to the eventpoll context.

**Return**

a value different than `zero` if ready events are available,
or `zero` otherwise.

bool busy\_loop\_ep\_timeout(unsigned long start\_time, struct eventpoll \*ep)
:   check if busy poll has timed out. The timeout value from the epoll instance ep is preferred, but if it is not set fallback to the system-wide global via busy\_loop\_timeout.

**Parameters**

`unsigned long start_time`
:   The start time used to compute the remaining time until timeout.

`struct eventpoll *ep`
:   Pointer to the eventpoll context.

**Return**

true if the timeout has expired, false otherwise.

int reverse\_path\_check(void)
:   The tfile\_check\_list is list of epitem\_head, which have links that are proposed to be newly added. We need to make sure that those added links don’t add too many paths such that we will spend all our time waking up eventpoll objects.

**Parameters**

`void`
:   no arguments

**Return**

`zero` if the proposed links don’t create too many paths,
`-1` otherwise.

int ep\_poll(struct eventpoll \*ep, struct epoll\_event \_\_user \*events, int maxevents, struct timespec64 \*timeout)
:   Retrieves ready events, and delivers them to the caller-supplied event buffer.

**Parameters**

`struct eventpoll *ep`
:   Pointer to the eventpoll context.

`struct epoll_event __user *events`
:   Pointer to the userspace buffer where the ready events should be
    stored.

`int maxevents`
:   Size (in terms of number of events) of the caller event buffer.

`struct timespec64 *timeout`
:   Maximum timeout for the ready events fetch operation, in
    timespec. If the timeout is zero, the function will not block,
    while if the **timeout** ptr is NULL, the function will block
    until at least one event has been retrieved (or an error
    occurred).

**Return**

the number of ready events which have been fetched, or an
error code, in case of error.

int ep\_loop\_check\_proc(struct eventpoll \*ep, int depth)
:   verify that adding an epoll file **ep** inside another epoll file does not create closed loops, and determine the depth of the subtree starting at **ep**

**Parameters**

`struct eventpoll *ep`
:   the `struct eventpoll` to be currently checked.

`int depth`
:   Current depth of the path being checked.

**Return**

depth of the subtree, or a value bigger than EP\_MAX\_NESTS if we found
a loop or went too deep.

int ep\_loop\_check(struct eventpoll \*ep, struct eventpoll \*to)
:   Performs a check to verify that adding an epoll file (**to**) into another epoll file (represented by **ep**) does not create closed loops or too deep chains.

**Parameters**

`struct eventpoll *ep`
:   Pointer to the epoll we are inserting into.

`struct eventpoll *to`
:   Pointer to the epoll to be inserted.

**Return**

`zero` if adding the epoll **to** inside the epoll **from**
does not violate the constraints, or `-1` otherwise.

## The Filesystem for Exporting Kernel Objects

int sysfs\_create\_file\_ns(struct kobject \*kobj, const struct attribute \*attr, const struct ns\_common \*ns)
:   create an attribute file for an object with custom ns

**Parameters**

`struct kobject *kobj`
:   object we’re creating for

`const struct attribute *attr`
:   attribute descriptor

`const struct ns_common *ns`
:   namespace the new file should belong to

int sysfs\_add\_file\_to\_group(struct kobject \*kobj, const struct attribute \*attr, const char \*group)
:   add an attribute file to a pre-existing group.

**Parameters**

`struct kobject *kobj`
:   object we’re acting for.

`const struct attribute *attr`
:   attribute descriptor.

`const char *group`
:   group name.

int sysfs\_chmod\_file(struct kobject \*kobj, const struct attribute \*attr, umode\_t mode)
:   update the modified mode value on an object attribute.

**Parameters**

`struct kobject *kobj`
:   object we’re acting for.

`const struct attribute *attr`
:   attribute descriptor.

`umode_t mode`
:   file permissions.

struct kernfs\_node \*sysfs\_break\_active\_protection(struct kobject \*kobj, const struct attribute \*attr)
:   break “active” protection

**Parameters**

`struct kobject *kobj`
:   The kernel object **attr** is associated with.

`const struct attribute *attr`
:   The attribute to break the “active” protection for.

**Description**

With sysfs, just like kernfs, deletion of an attribute is postponed until
all active .`show()` and .`store()` callbacks have finished unless this function
is called. Hence this function is useful in methods that implement self
deletion.

void sysfs\_unbreak\_active\_protection(struct kernfs\_node \*kn)
:   restore “active” protection

**Parameters**

`struct kernfs_node *kn`
:   Pointer returned by [`sysfs_break_active_protection()`](#c.sysfs_break_active_protection "sysfs_break_active_protection").

**Description**

Undo the effects of [`sysfs_break_active_protection()`](#c.sysfs_break_active_protection "sysfs_break_active_protection"). Since this function
calls `kernfs_put()` on the kernfs node that corresponds to the ‘attr’
argument passed to [`sysfs_break_active_protection()`](#c.sysfs_break_active_protection "sysfs_break_active_protection") that attribute may have
been removed between the [`sysfs_break_active_protection()`](#c.sysfs_break_active_protection "sysfs_break_active_protection") and
[`sysfs_unbreak_active_protection()`](#c.sysfs_unbreak_active_protection "sysfs_unbreak_active_protection") calls, it is not safe to access **kn** after
this function has returned.

void sysfs\_remove\_file\_ns(struct kobject \*kobj, const struct attribute \*attr, const struct ns\_common \*ns)
:   remove an object attribute with a custom ns tag

**Parameters**

`struct kobject *kobj`
:   object we’re acting for

`const struct attribute *attr`
:   attribute descriptor

`const struct ns_common *ns`
:   namespace tag of the file to remove

**Description**

Hash the attribute name and namespace tag and kill the victim.

bool sysfs\_remove\_file\_self(struct kobject \*kobj, const struct attribute \*attr)
:   remove an object attribute from its own method

**Parameters**

`struct kobject *kobj`
:   object we’re acting for

`const struct attribute *attr`
:   attribute descriptor

**Description**

See `kernfs_remove_self()` for details.

void sysfs\_remove\_file\_from\_group(struct kobject \*kobj, const struct attribute \*attr, const char \*group)
:   remove an attribute file from a group.

**Parameters**

`struct kobject *kobj`
:   object we’re acting for.

`const struct attribute *attr`
:   attribute descriptor.

`const char *group`
:   group name.

int sysfs\_create\_bin\_file(struct kobject \*kobj, const struct bin\_attribute \*attr)
:   create binary file for object.

**Parameters**

`struct kobject *kobj`
:   object.

`const struct bin_attribute *attr`
:   attribute descriptor.

void sysfs\_remove\_bin\_file(struct kobject \*kobj, const struct bin\_attribute \*attr)
:   remove binary file for object.

**Parameters**

`struct kobject *kobj`
:   object.

`const struct bin_attribute *attr`
:   attribute descriptor.

int sysfs\_emit(char \*buf, const char \*fmt, ...)
:   scnprintf equivalent, aware of PAGE\_SIZE buffer.

**Parameters**

`char *buf`
:   start of PAGE\_SIZE buffer.

`const char *fmt`
:   format

`...`
:   optional arguments to **format**

**Description**

Returns number of characters written to **buf**.

int sysfs\_emit\_at(char \*buf, int at, const char \*fmt, ...)
:   scnprintf equivalent, aware of PAGE\_SIZE buffer.

**Parameters**

`char *buf`
:   start of PAGE\_SIZE buffer.

`int at`
:   offset in **buf** to start write in bytes
    **at** must be >= 0 && < PAGE\_SIZE

`const char *fmt`
:   format

`...`
:   optional arguments to **fmt**

**Description**

Returns number of characters written starting at &\*\*buf\*\*[**at**].

ssize\_t sysfs\_bin\_attr\_simple\_read(struct [file](#c.sysfs_bin_attr_simple_read "file") \*file, struct kobject \*kobj, const struct bin\_attribute \*attr, char \*buf, loff\_t off, size\_t count)
:   read callback to simply copy from memory.

**Parameters**

`struct file *file`
:   attribute file which is being read.

`struct kobject *kobj`
:   object to which the attribute belongs.

`const struct bin_attribute *attr`
:   attribute descriptor.

`char *buf`
:   destination buffer.

`loff_t off`
:   offset in bytes from which to read.

`size_t count`
:   maximum number of bytes to read.

**Description**

Simple ->read() callback for bin\_attributes backed by a buffer in memory.
The **private** and **size** members in `struct bin_attribute` must be set to the
buffer’s location and size before the bin\_attribute is created in sysfs.

Bounds check for **off** and **count** is done in `sysfs_kf_bin_read()`.
Negative value check for **off** is done in `vfs_setpos()` and `default_llseek()`.

Returns number of bytes written to **buf**.

int sysfs\_create\_link(struct kobject \*kobj, struct kobject \*target, const char \*name)
:   create symlink between two objects.

**Parameters**

`struct kobject *kobj`
:   object whose directory we’re creating the link in.

`struct kobject *target`
:   object we’re pointing to.

`const char *name`
:   name of the symlink.

int sysfs\_create\_link\_nowarn(struct kobject \*kobj, struct kobject \*target, const char \*name)
:   create symlink between two objects.

**Parameters**

`struct kobject *kobj`
:   object whose directory we’re creating the link in.

`struct kobject *target`
:   object we’re pointing to.

`const char *name`
:   name of the symlink.

**Description**

> This function does the same as [`sysfs_create_link()`](#c.sysfs_create_link "sysfs_create_link"), but it
> doesn’t warn if the link already exists.

void sysfs\_remove\_link(struct kobject \*kobj, const char \*name)
:   remove symlink in object’s directory.

**Parameters**

`struct kobject *kobj`
:   object we’re acting for.

`const char *name`
:   name of the symlink to remove.

int sysfs\_rename\_link\_ns(struct kobject \*kobj, struct kobject \*targ, const char \*old, const char \*new, const struct ns\_common \*new\_ns)
:   rename symlink in object’s directory.

**Parameters**

`struct kobject *kobj`
:   object we’re acting for.

`struct kobject *targ`
:   object we’re pointing to.

`const char *old`
:   previous name of the symlink.

`const char *new`
:   new name of the symlink.

`const struct ns_common *new_ns`
:   new namespace of the symlink.

**Description**

> A helper function for the common rename symlink idiom.

## The debugfs filesystem

### debugfs interface

struct dentry \*debugfs\_lookup(const char \*name, struct dentry \*parent)
:   look up an existing debugfs file

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to look up.

`struct dentry *parent`
:   a pointer to the parent dentry of the file.

**Description**

This function will return a pointer to a dentry if it succeeds. If the file
doesn’t exist or an error occurs, `NULL` will be returned. The returned
dentry must be passed to `dput()` when it is no longer needed.

If debugfs is not enabled in the kernel, the value -`ENODEV` will be
returned.

struct dentry \*debugfs\_create\_file\_unsafe(const char \*name, umode\_t mode, struct dentry \*parent, void \*data, const struct file\_operations \*fops)
:   create a file in the debugfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have.

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is NULL, then the
    file will be created in the root of the debugfs filesystem.

`void *data`
:   a pointer to something that the caller will want to get to later
    on. The inode.i\_private pointer will point to this value on
    the open() call.

`const struct file_operations *fops`
:   a pointer to a `struct file_operations` that should be used for
    this file.

**Description**

[`debugfs_create_file_unsafe()`](#c.debugfs_create_file_unsafe "debugfs_create_file_unsafe") is completely analogous to
`debugfs_create_file()`, the only difference being that the fops
handed it will not get protected against file removals by the
debugfs core.

It is your responsibility to protect your `struct file_operation`
methods against file removals by means of [`debugfs_file_get()`](#c.debugfs_file_get "debugfs_file_get")
and [`debugfs_file_put()`](#c.debugfs_file_put "debugfs_file_put"). ->open() is still protected by
debugfs though.

Any `struct file_operations` defined by means of
`DEFINE_DEBUGFS_ATTRIBUTE()` is protected against file removals and
thus, may be used here.

void debugfs\_create\_file\_size(const char \*name, umode\_t mode, struct dentry \*parent, void \*data, const struct file\_operations \*fops, loff\_t file\_size)
:   create a file in the debugfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have.

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is NULL, then the
    file will be created in the root of the debugfs filesystem.

`void *data`
:   a pointer to something that the caller will want to get to later
    on. The inode.i\_private pointer will point to this value on
    the open() call.

`const struct file_operations *fops`
:   a pointer to a `struct file_operations` that should be used for
    this file.

`loff_t file_size`
:   initial file size

**Description**

This is the basic “create a file” function for debugfs. It allows for a
wide range of flexibility in creating a file, or a directory (if you want
to create a directory, the [`debugfs_create_dir()`](#c.debugfs_create_dir "debugfs_create_dir") function is
recommended to be used instead.)

struct dentry \*debugfs\_create\_dir(const char \*name, struct dentry \*parent)
:   create a directory in the debugfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the directory to
    create.

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is NULL, then the
    directory will be created in the root of the debugfs filesystem.

**Description**

This function creates a directory in debugfs with the given name.

This function will return a pointer to a dentry if it succeeds. This
pointer must be passed to the [`debugfs_remove()`](#c.debugfs_remove "debugfs_remove") function when the file is
to be removed (no automatic cleanup happens if your module is unloaded,
you are responsible here.) If an error occurs, ERR\_PTR(-ERROR) will be
returned.

If debugfs is not enabled in the kernel, the value -`ENODEV` will be
returned.

**NOTE**

it’s expected that most callers should \_ignore\_ the errors returned
by this function. Other debugfs functions handle the fact that the “dentry”
passed to them could be an error and they don’t crash in that case.
Drivers should generally work fine even if debugfs fails to init anyway.

struct dentry \*debugfs\_create\_automount(const char \*name, struct dentry \*parent, debugfs\_automount\_t f, void \*data)
:   create automount point in the debugfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is NULL, then the
    file will be created in the root of the debugfs filesystem.

`debugfs_automount_t f`
:   function to be called when pathname resolution steps on that one.

`void *data`
:   opaque argument to pass to f().

**Description**

**f** should return what ->`d_automount()` would.

struct dentry \*debugfs\_create\_symlink(const char \*name, struct dentry \*parent, const char \*target)
:   create a symbolic link in the debugfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the symbolic link to
    create.

`struct dentry *parent`
:   a pointer to the parent dentry for this symbolic link. This
    should be a directory dentry if set. If this parameter is NULL,
    then the symbolic link will be created in the root of the debugfs
    filesystem.

`const char *target`
:   a pointer to a string containing the path to the target of the
    symbolic link.

**Description**

This function creates a symbolic link with the given name in debugfs that
links to the given target path.

This function will return a pointer to a dentry if it succeeds. This
pointer must be passed to the [`debugfs_remove()`](#c.debugfs_remove "debugfs_remove") function when the symbolic
link is to be removed (no automatic cleanup happens if your module is
unloaded, you are responsible here.) If an error occurs, ERR\_PTR(-ERROR)
will be returned.

If debugfs is not enabled in the kernel, the value -`ENODEV` will be
returned.

void debugfs\_remove(struct [dentry](#c.debugfs_remove "dentry") \*dentry)
:   recursively removes a directory

**Parameters**

`struct dentry *dentry`
:   a pointer to a the dentry of the directory to be removed. If this
    parameter is NULL or an error value, nothing will be done.

**Description**

This function recursively removes a directory tree in debugfs that
was previously created with a call to another debugfs function
(like `debugfs_create_file()` or variants thereof.)

This function is required to be called in order for the file to be
removed, no automatic cleanup of files will happen when a module is
removed, you are responsible here.

void debugfs\_lookup\_and\_remove(const char \*name, struct dentry \*parent)
:   lookup a directory or file and recursively remove it

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the item to look up.

`struct dentry *parent`
:   a pointer to the parent dentry of the item.

**Description**

This is the equlivant of doing something like
debugfs\_remove(debugfs\_lookup(..)) but with the proper reference counting
handled for the directory being looked up.

int debugfs\_change\_name(struct [dentry](#c.debugfs_change_name "dentry") \*dentry, const char \*fmt, ...)
:   rename a file/directory in the debugfs filesystem

**Parameters**

`struct dentry *dentry`
:   dentry of an object to be renamed.

`const char *fmt`
:   format for new name

`...`
:   variable arguments

**Description**

This function renames a file/directory in debugfs. The target must not
exist for rename to succeed.

This function will return 0 on success and -E... on failure.

If debugfs is not enabled in the kernel, the value -`ENODEV` will be
returned.

bool debugfs\_initialized(void)
:   Tells whether debugfs has been registered

**Parameters**

`void`
:   no arguments

int debugfs\_file\_get(struct [dentry](#c.debugfs_file_get "dentry") \*dentry)
:   mark the beginning of file data access

**Parameters**

`struct dentry *dentry`
:   the dentry object whose data is being accessed.

**Description**

Up to a matching call to [`debugfs_file_put()`](#c.debugfs_file_put "debugfs_file_put"), any successive call
into the file removing functions [`debugfs_remove()`](#c.debugfs_remove "debugfs_remove") and
`debugfs_remove_recursive()` will block. Since associated private
file data may only get freed after a successful return of any of
the removal functions, you may safely access it after a successful
call to [`debugfs_file_get()`](#c.debugfs_file_get "debugfs_file_get") without worrying about lifetime issues.

If -`EIO` is returned, the file has already been removed and thus,
it is not safe to access any of its data. If, on the other hand,
it is allowed to access the file data, zero is returned.

void debugfs\_file\_put(struct [dentry](#c.debugfs_file_put "dentry") \*dentry)
:   mark the end of file data access

**Parameters**

`struct dentry *dentry`
:   the dentry object formerly passed to
    [`debugfs_file_get()`](#c.debugfs_file_get "debugfs_file_get").

**Description**

Allow any ongoing concurrent call into [`debugfs_remove()`](#c.debugfs_remove "debugfs_remove") or
`debugfs_remove_recursive()` blocked by a former call to
[`debugfs_file_get()`](#c.debugfs_file_get "debugfs_file_get") to proceed and return to its caller.

void debugfs\_enter\_cancellation(struct [file](#c.debugfs_enter_cancellation "file") \*file, struct debugfs\_cancellation \*cancellation)
:   enter a debugfs cancellation

**Parameters**

`struct file *file`
:   the file being accessed

`struct debugfs_cancellation *cancellation`
:   the cancellation object, the cancel callback
    inside of it must be initialized

**Description**

When a debugfs file is removed it needs to wait for all active
operations to complete. However, the operation itself may need
to wait for hardware or completion of some asynchronous process
or similar. As such, it may need to be cancelled to avoid long
waits or even deadlocks.

This function can be used inside a debugfs handler that may
need to be cancelled. As soon as this function is called, the
cancellation’s ‘cancel’ callback may be called, at which point
the caller should proceed to call [`debugfs_leave_cancellation()`](#c.debugfs_leave_cancellation "debugfs_leave_cancellation")
and leave the debugfs handler function as soon as possible.
Note that the ‘cancel’ callback is only ever called in the
context of some kind of [`debugfs_remove()`](#c.debugfs_remove "debugfs_remove").

This function must be paired with [`debugfs_leave_cancellation()`](#c.debugfs_leave_cancellation "debugfs_leave_cancellation").

void debugfs\_leave\_cancellation(struct [file](#c.debugfs_leave_cancellation "file") \*file, struct debugfs\_cancellation \*cancellation)
:   leave cancellation section

**Parameters**

`struct file *file`
:   the file being accessed

`struct debugfs_cancellation *cancellation`
:   the cancellation previously registered with
    [`debugfs_enter_cancellation()`](#c.debugfs_enter_cancellation "debugfs_enter_cancellation")

**Description**

See the documentation of [`debugfs_enter_cancellation()`](#c.debugfs_enter_cancellation "debugfs_enter_cancellation").

void debugfs\_create\_u8(const char \*name, umode\_t mode, struct dentry \*parent, u8 \*value)
:   create a debugfs file that is used to read and write an unsigned 8-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u8 *value`
:   a pointer to the variable that the file should read to and write
    from.

**Description**

This function creates a file in debugfs with the given name that
contains the value of the variable **value**. If the **mode** variable is so
set, it can be read from, and written to.

void debugfs\_create\_u16(const char \*name, umode\_t mode, struct dentry \*parent, u16 \*value)
:   create a debugfs file that is used to read and write an unsigned 16-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u16 *value`
:   a pointer to the variable that the file should read to and write
    from.

**Description**

This function creates a file in debugfs with the given name that
contains the value of the variable **value**. If the **mode** variable is so
set, it can be read from, and written to.

void debugfs\_create\_u32(const char \*name, umode\_t mode, struct dentry \*parent, u32 \*value)
:   create a debugfs file that is used to read and write an unsigned 32-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u32 *value`
:   a pointer to the variable that the file should read to and write
    from.

**Description**

This function creates a file in debugfs with the given name that
contains the value of the variable **value**. If the **mode** variable is so
set, it can be read from, and written to.

void debugfs\_create\_u64(const char \*name, umode\_t mode, struct dentry \*parent, u64 \*value)
:   create a debugfs file that is used to read and write an unsigned 64-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u64 *value`
:   a pointer to the variable that the file should read to and write
    from.

**Description**

This function creates a file in debugfs with the given name that
contains the value of the variable **value**. If the **mode** variable is so
set, it can be read from, and written to.

void debugfs\_create\_ulong(const char \*name, umode\_t mode, struct dentry \*parent, unsigned long \*value)
:   create a debugfs file that is used to read and write an unsigned long value.

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`unsigned long *value`
:   a pointer to the variable that the file should read to and write
    from.

**Description**

This function creates a file in debugfs with the given name that
contains the value of the variable **value**. If the **mode** variable is so
set, it can be read from, and written to.

void debugfs\_create\_x8(const char \*name, umode\_t mode, struct dentry \*parent, u8 \*value)
:   create a debugfs file that is used to read and write an unsigned 8-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u8 *value`
:   a pointer to the variable that the file should read to and write
    from.

void debugfs\_create\_x16(const char \*name, umode\_t mode, struct dentry \*parent, u16 \*value)
:   create a debugfs file that is used to read and write an unsigned 16-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u16 *value`
:   a pointer to the variable that the file should read to and write
    from.

void debugfs\_create\_x32(const char \*name, umode\_t mode, struct dentry \*parent, u32 \*value)
:   create a debugfs file that is used to read and write an unsigned 32-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u32 *value`
:   a pointer to the variable that the file should read to and write
    from.

void debugfs\_create\_x64(const char \*name, umode\_t mode, struct dentry \*parent, u64 \*value)
:   create a debugfs file that is used to read and write an unsigned 64-bit value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`u64 *value`
:   a pointer to the variable that the file should read to and write
    from.

void debugfs\_create\_size\_t(const char \*name, umode\_t mode, struct dentry \*parent, size\_t \*value)
:   create a debugfs file that is used to read and write an size\_t value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`size_t *value`
:   a pointer to the variable that the file should read to and write
    from.

void debugfs\_create\_atomic\_t(const char \*name, umode\_t mode, struct dentry \*parent, atomic\_t \*value)
:   create a debugfs file that is used to read and write an atomic\_t value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`atomic_t *value`
:   a pointer to the variable that the file should read to and write
    from.

void debugfs\_create\_bool(const char \*name, umode\_t mode, struct dentry \*parent, bool \*value)
:   create a debugfs file that is used to read and write a boolean value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`bool *value`
:   a pointer to the variable that the file should read to and write
    from.

**Description**

This function creates a file in debugfs with the given name that
contains the value of the variable **value**. If the **mode** variable is so
set, it can be read from, and written to.

void debugfs\_create\_str(const char \*name, umode\_t mode, struct dentry \*parent, char \*\*value)
:   create a debugfs file that is used to read and write a string value

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`char **value`
:   a pointer to the variable that the file should read to and write
    from. This pointer and the string it points to must not be `NULL`.

**Description**

This function creates a file in debugfs with the given name that
contains the value of the variable **value**. If the **mode** variable is so
set, it can be read from, and written to.

struct dentry \*debugfs\_create\_blob(const char \*name, umode\_t mode, struct dentry \*parent, struct debugfs\_blob\_wrapper \*blob)
:   create a debugfs file that is used to read and write a binary blob

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`struct debugfs_blob_wrapper *blob`
:   a pointer to a `struct debugfs_blob_wrapper` which contains a pointer
    to the blob data and the size of the data.

**Description**

This function creates a file in debugfs with the given name that exports
**blob->data** as a binary blob. If the **mode** variable is so set it can be
read from and written to.

This function will return a pointer to a dentry if it succeeds. This
pointer must be passed to the [`debugfs_remove()`](#c.debugfs_remove "debugfs_remove") function when the file is
to be removed (no automatic cleanup happens if your module is unloaded,
you are responsible here.) If an error occurs, ERR\_PTR(-ERROR) will be
returned.

If debugfs is not enabled in the kernel, the value ERR\_PTR(-ENODEV) will
be returned.

void debugfs\_create\_u32\_array(const char \*name, umode\_t mode, struct dentry \*parent, struct debugfs\_u32\_array \*array)
:   create a debugfs file that is used to read u32 array.

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have.

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`struct debugfs_u32_array *array`
:   wrapper `struct containing` data pointer and size of the array.

**Description**

This function creates a file in debugfs with the given name that exports
**array** as data. If the **mode** variable is so set it can be read from.
Writing is not supported. Seek within the file is also not supported.
Once array is created its size can not be changed.

void debugfs\_print\_regs32(struct seq\_file \*s, const struct debugfs\_reg32 \*regs, int nregs, void \_\_iomem \*base, char \*prefix)
:   use seq\_print to describe a set of registers

**Parameters**

`struct seq_file *s`
:   the seq\_file structure being used to generate output

`const struct debugfs_reg32 *regs`
:   an array if `struct debugfs_reg32` structures

`int nregs`
:   the length of the above array

`void __iomem *base`
:   the base address to be used in reading the registers

`char *prefix`
:   a string to be prefixed to every output line

**Description**

This function outputs a text block describing the current values of
some 32-bit hardware registers. It is meant to be used within debugfs
files based on seq\_file that need to show registers, intermixed with other
information. The prefix argument may be used to specify a leading string,
because some peripherals have several blocks of identical registers,
for example configuration of dma channels

void debugfs\_create\_regset32(const char \*name, umode\_t mode, struct dentry \*parent, struct debugfs\_regset32 \*regset)
:   create a debugfs file that returns register values

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`struct debugfs_regset32 *regset`
:   a pointer to a `struct debugfs_regset32`, which contains a pointer
    to an array of register definitions, the array size and the base
    address where the register bank is to be found.

**Description**

This function creates a file in debugfs with the given name that reports
the names and values of a set of 32-bit registers. If the **mode** variable
is so set it can be read from. Writing is not supported.

void debugfs\_create\_devm\_seqfile(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*name, struct dentry \*parent, int (\*read\_fn)(struct seq\_file \*s, void \*data))
:   create a debugfs file that is bound to device.

**Parameters**

`struct device *dev`
:   device related to this debugfs file.

`const char *name`
:   name of the debugfs file.

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the debugfs filesystem.

`int (*read_fn)(struct seq_file *s, void *data)`
:   function pointer called to print the seq\_file content.
