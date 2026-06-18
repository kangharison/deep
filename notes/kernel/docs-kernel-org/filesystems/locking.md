# Locking

> 출처(원문): https://docs.kernel.org/filesystems/locking.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Locking

The text below describes the locking rules for VFS-related methods.
It is (believed to be) up-to-date. *Please*, if you change anything in
prototypes or locking protocols - update this file. And update the relevant
instances in the tree, don’t leave that to maintainers of filesystems/devices/
etc. At the very least, put the list of dubious cases in the end of this file.
Don’t turn it into log - maintainers of out-of-the-tree code are supposed to
be able to use diff(1).

Thing currently missing here: socket operations. Alexey?

## dentry\_operations

prototypes:

```
int (*d_revalidate)(struct inode *, const struct qstr *,
                    struct dentry *, unsigned int);
int (*d_weak_revalidate)(struct dentry *, unsigned int);
int (*d_hash)(const struct dentry *, struct qstr *);
int (*d_compare)(const struct dentry *,
                unsigned int, const char *, const struct qstr *);
int (*d_delete)(struct dentry *);
int (*d_init)(struct dentry *);
void (*d_release)(struct dentry *);
void (*d_iput)(struct dentry *, struct inode *);
char *(*d_dname)((struct dentry *dentry, char *buffer, int buflen);
struct vfsmount *(*d_automount)(struct path *path);
int (*d_manage)(const struct path *, bool);
struct dentry *(*d_real)(struct dentry *, enum d_real_type type);
bool (*d_unalias_trylock)(const struct dentry *);
void (*d_unalias_unlock)(const struct dentry *);
```

locking rules:

| ops | rename\_lock | ->d\_lock | may block | rcu-walk |
| --- | --- | --- | --- | --- |
| d\_revalidate: | no | no | yes (ref-walk) | maybe |
| d\_weak\_revalidate: | no | no | yes | no |
| d\_hash | no | no | no | maybe |
| d\_compare: | yes | no | no | maybe |
| d\_delete: | no | yes | no | no |
| d\_init: | no | no | yes | no |
| d\_release: | no | no | yes | no |
| d\_prune: | no | yes | no | no |
| d\_iput: | no | no | yes | no |
| d\_dname: | no | no | no | no |
| d\_automount: | no | no | yes | no |
| d\_manage: | no | no | yes (ref-walk) | maybe |
| d\_real | no | no | yes | no |
| d\_unalias\_trylock | yes | no | no | no |
| d\_unalias\_unlock | yes | no | no | no |

## inode\_operations

prototypes:

```
int (*create) (struct mnt_idmap *, struct inode *,struct dentry *,umode_t, bool);
struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
int (*link) (struct dentry *,struct inode *,struct dentry *);
int (*unlink) (struct inode *,struct dentry *);
int (*symlink) (struct mnt_idmap *, struct inode *,struct dentry *,const char *);
struct dentry *(*mkdir) (struct mnt_idmap *, struct inode *,struct dentry *,umode_t);
int (*rmdir) (struct inode *,struct dentry *);
int (*mknod) (struct mnt_idmap *, struct inode *,struct dentry *,umode_t,dev_t);
int (*rename) (struct mnt_idmap *, struct inode *, struct dentry *,
                struct inode *, struct dentry *, unsigned int);
int (*readlink) (struct dentry *, char __user *,int);
const char *(*get_link) (struct dentry *, struct inode *, struct delayed_call *);
void (*truncate) (struct inode *);
int (*permission) (struct mnt_idmap *, struct inode *, int, unsigned int);
struct posix_acl * (*get_inode_acl)(struct inode *, int, bool);
int (*setattr) (struct mnt_idmap *, struct dentry *, struct iattr *);
int (*getattr) (struct mnt_idmap *, const struct path *, struct kstat *, u32, unsigned int);
ssize_t (*listxattr) (struct dentry *, char *, size_t);
int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start, u64 len);
void (*update_time)(struct inode *inode, enum fs_update_time type,
                    int flags);
void (*sync_lazytime)(struct inode *inode);
int (*atomic_open)(struct inode *, struct dentry *,
                        struct file *, unsigned open_flag,
                        umode_t create_mode);
int (*tmpfile) (struct mnt_idmap *, struct inode *,
                struct file *, umode_t);
int (*fileattr_set)(struct mnt_idmap *idmap,
                    struct dentry *dentry, struct file_kattr *fa);
int (*fileattr_get)(struct dentry *dentry, struct file_kattr *fa);
struct posix_acl * (*get_acl)(struct mnt_idmap *, struct dentry *, int);
struct offset_ctx *(*get_offset_ctx)(struct inode *inode);
```

locking rules:
:   all may block

| ops | i\_rwsem(inode) |
| --- | --- |
| lookup: | shared |
| create: | exclusive |
| link: | exclusive (both) |
| mknod: | exclusive |
| symlink: | exclusive |
| mkdir: | exclusive |
| unlink: | exclusive (both) |
| rmdir: | exclusive (both)(see below) |
| rename: | exclusive (both parents, some children) (see below) |
| readlink: | no |
| get\_link: | no |
| setattr: | exclusive |
| permission: | no (may not block if called in rcu-walk mode) |
| get\_inode\_acl: | no |
| get\_acl: | no |
| getattr: | no |
| listxattr: | no |
| fiemap: | no |
| update\_time: | no |
| sync\_lazytime: | no |
| atomic\_open: | shared (exclusive if O\_CREAT is set in open flags) |
| tmpfile: | no |
| fileattr\_get: | no or exclusive |
| fileattr\_set: | exclusive |
| get\_offset\_ctx | no |

> Additionally, ->`rmdir()`, ->`unlink()` and ->`rename()` have ->i\_rwsem
> exclusive on victim.
> cross-directory ->`rename()` has (per-superblock) ->s\_vfs\_rename\_sem.
> ->`unlink()` and ->`rename()` have ->i\_rwsem exclusive on all non-directories
> involved.
> ->`rename()` has ->i\_rwsem exclusive on any subdirectory that changes parent.

See [Directory Locking](directory-locking.html) for more detailed discussion
of the locking scheme for directory operations.

## xattr\_handler operations

prototypes:

```
bool (*list)(struct dentry *dentry);
int (*get)(const struct xattr_handler *handler, struct dentry *dentry,
           struct inode *inode, const char *name, void *buffer,
           size_t size);
int (*set)(const struct xattr_handler *handler,
           struct mnt_idmap *idmap,
           struct dentry *dentry, struct inode *inode, const char *name,
           const void *buffer, size_t size, int flags);
```

locking rules:
:   all may block

| ops | i\_rwsem(inode) |
| --- | --- |
| list: | no |
| get: | no |
| set: | exclusive |

## super\_operations

prototypes:

```
struct inode *(*alloc_inode)(struct super_block *sb);
void (*free_inode)(struct inode *);
void (*destroy_inode)(struct inode *);
void (*dirty_inode) (struct inode *, int flags);
int (*write_inode) (struct inode *, struct writeback_control *wbc);
int (*drop_inode) (struct inode *);
void (*evict_inode) (struct inode *);
void (*put_super) (struct super_block *);
int (*sync_fs)(struct super_block *sb, int wait);
int (*freeze_fs) (struct super_block *);
int (*unfreeze_fs) (struct super_block *);
int (*statfs) (struct dentry *, struct kstatfs *);
void (*umount_begin) (struct super_block *);
int (*show_options)(struct seq_file *, struct dentry *);
ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
```

locking rules:
:   All may block [not true, see below]

| ops | s\_umount | note |
| --- | --- | --- |
| alloc\_inode: |  |  |
| free\_inode: |  | called from RCU callback |
| destroy\_inode: |  |  |
| dirty\_inode: |  |  |
| write\_inode: |  |  |
| drop\_inode: |  | !!!inode->i\_lock!!! |
| evict\_inode: |  |  |
| put\_super: | write |  |
| sync\_fs: | read |  |
| freeze\_fs: | write |  |
| unfreeze\_fs: | write |  |
| statfs: | maybe(read) | (see below) |
| umount\_begin: | no |  |
| show\_options: | no | (namespace\_sem) |
| quota\_read: | no | (see below) |
| quota\_write: | no | (see below) |

->`statfs()` has s\_umount (shared) when called by ustat(2) (native or
compat), but that’s an accident of bad API; s\_umount is used to pin
the superblock down when we only have dev\_t given us by userland to
identify the superblock. Everything else (`statfs()`, `fstatfs()`, etc.)
doesn’t hold it when calling ->`statfs()` - superblock is pinned down
by resolving the pathname passed to syscall.

->`quota_read()` and ->`quota_write()` functions are both guaranteed to
be the only ones operating on the quota file by the quota code (via
dqio\_sem) (unless an admin really wants to screw up something and
writes to quota files with quotas on). For other details about locking
see also dquot\_operations section.

## file\_system\_type

prototypes:

```
void (*kill_sb) (struct super_block *);
```

locking rules:

| ops | may block |
| --- | --- |
| kill\_sb | yes |

->`kill_sb()` takes a write-locked superblock, does all shutdown work on it,
unlocks and drops the reference.

## address\_space\_operations

prototypes:

```
int (*read_folio)(struct file *, struct folio *);
int (*writepages)(struct address_space *, struct writeback_control *);
bool (*dirty_folio)(struct address_space *, struct folio *folio);
void (*readahead)(struct readahead_control *);
int (*write_begin)(const struct kiocb *, struct address_space *mapping,
                        loff_t pos, unsigned len,
                        struct folio **foliop, void **fsdata);
int (*write_end)(const struct kiocb *, struct address_space *mapping,
                        loff_t pos, unsigned len, unsigned copied,
                        struct folio *folio, void *fsdata);
sector_t (*bmap)(struct address_space *, sector_t);
void (*invalidate_folio) (struct folio *, size_t start, size_t len);
bool (*release_folio)(struct folio *, gfp_t);
void (*free_folio)(struct folio *);
int (*direct_IO)(struct kiocb *, struct iov_iter *iter);
int (*migrate_folio)(struct address_space *, struct folio *dst,
                struct folio *src, enum migrate_mode);
int (*launder_folio)(struct folio *);
bool (*is_partially_uptodate)(struct folio *, size_t from, size_t count);
int (*error_remove_folio)(struct address_space *, struct folio *);
int (*swap_activate)(struct swap_info_struct *sis, struct file *f, sector_t *span)
int (*swap_deactivate)(struct file *);
int (*swap_rw)(struct kiocb *iocb, struct iov_iter *iter);
```

locking rules:
:   All except dirty\_folio and free\_folio may block

| ops | folio locked | i\_rwsem | invalidate\_lock |
| --- | --- | --- | --- |
| read\_folio: | yes, unlocks |  | shared |
| writepages: |  |  |  |
| dirty\_folio: | maybe |  |  |
| readahead: | yes, unlocks |  | shared |
| write\_begin: | locks the folio | exclusive |  |
| write\_end: | yes, unlocks | exclusive |  |
| bmap: |  |  |  |
| invalidate\_folio: | yes |  | exclusive |
| release\_folio: | yes |  |  |
| free\_folio: | yes |  |  |
| direct\_IO: |  |  |  |
| migrate\_folio: | yes (both) |  |  |
| launder\_folio: | yes |  |  |
| is\_partially\_uptodate: | yes |  |  |
| error\_remove\_folio: | yes |  |  |
| swap\_activate: | no |  |  |
| swap\_deactivate: | no |  |  |
| swap\_rw: | yes, unlocks |  |  |

->`write_begin()`, ->`write_end()` and ->`read_folio()` may be called from
the request handler (/dev/loop).

->`read_folio()` unlocks the folio, either synchronously or via I/O
completion.

->`readahead()` unlocks the folios that I/O is attempted on like ->`read_folio()`.

->`writepages()` is used for periodic writeback and for syscall-initiated
sync operations. The address\_space should start I/O against at least
`*nr_to_write` pages. `*nr_to_write` must be decremented for each page
which is written. The address\_space implementation may write more (or less)
pages than `*nr_to_write` asks for, but it should try to be reasonably close.
If nr\_to\_write is NULL, all dirty pages must be written.

writepages should \_only\_ write pages which are present in
mapping->i\_pages.

->`dirty_folio()` is called from various places in the kernel when
the target folio is marked as needing writeback. The folio cannot be
truncated because either the caller holds the folio lock, or the caller
has found the folio while holding the page table lock which will block
truncation.

->[`bmap()`](api-summary.html#c.bmap "bmap") is currently used by legacy ioctl() (FIBMAP) provided by some
filesystems and by the swapper. The latter will eventually go away. Please,
keep it that way and don’t breed new callers.

->`invalidate_folio()` is called when the filesystem must attempt to drop
some or all of the buffers from the page when it is being truncated. It
returns zero on success. The filesystem must exclusively acquire
invalidate\_lock before invalidating page cache in truncate / hole punch
path (and thus calling into ->invalidate\_folio) to block races between page
cache invalidation and page cache filling functions (fault, read, ...).

->`release_folio()` is called when the MM wants to make a change to the
folio that would invalidate the filesystem’s private data. For example,
it may be about to be removed from the address\_space or split. The folio
is locked and not under writeback. It may be dirty. The gfp parameter
is not usually used for allocation, but rather to indicate what the
filesystem may do to attempt to free the private data. The filesystem may
return false to indicate that the folio’s private data cannot be freed.
If it returns true, it should have already removed the private data from
the folio. If a filesystem does not provide a ->release\_folio method,
the pagecache will assume that private data is buffer\_heads and call
[`try_to_free_buffers()`](buffer.html#c.try_to_free_buffers "try_to_free_buffers").

->`free_folio()` is called when the kernel has dropped the folio
from the page cache.

->`launder_folio()` may be called prior to releasing a folio if
it is still found to be dirty. It returns zero if the folio was successfully
cleaned, or an error value if not. Note that in order to prevent the folio
getting mapped back in and redirtied, it needs to be kept locked
across the entire operation.

->`swap_activate()` will be called to prepare the given file for swap. It
should perform any validation and preparation necessary to ensure that
writes can be performed with minimal memory allocation. It should call
`add_swap_extent()`, or the helper `iomap_swapfile_activate()`, and return
the number of extents added. If IO should be submitted through
->`swap_rw()`, it should set SWP\_FS\_OPS, otherwise IO will be submitted
directly to the block device `sis->bdev`.

->`swap_deactivate()` will be called in the `sys_swapoff()`
path after ->`swap_activate()` returned success.

->swap\_rw will be called for swap IO if SWP\_FS\_OPS was set by ->`swap_activate()`.

## file\_lock\_operations

prototypes:

```
void (*fl_copy_lock)(struct file_lock *, struct file_lock *);
void (*fl_release_private)(struct file_lock *);
```

locking rules:

| ops | inode->i\_lock | may block |
| --- | --- | --- |
| fl\_copy\_lock: | yes | no |
| fl\_release\_private: | maybe | maybe[1]\_ |

## lock\_manager\_operations

prototypes:

```
void (*lm_notify)(struct file_lock *);  /* unblock callback */
int (*lm_grant)(struct file_lock *, struct file_lock *, int);
void (*lm_break)(struct file_lock *); /* break_lease callback */
int (*lm_change)(struct file_lock **, int);
bool (*lm_breaker_owns_lease)(struct file_lock *);
bool (*lm_lock_expirable)(struct file_lock *);
void (*lm_expire_lock)(void);
bool (*lm_breaker_timedout)(struct file_lease *);
```

locking rules:

| ops | flc\_lock | blocked\_lock\_lock | may block |
| --- | --- | --- | --- |
| lm\_notify: | no | yes | no |
| lm\_grant: | no | no | no |
| lm\_break: | yes | no | no |
| lm\_change | yes | no | no |
| lm\_breaker\_owns\_lease: | yes | no | no |
| lm\_lock\_expirable | yes | no | no |
| lm\_expire\_lock | no | no | yes |
| lm\_open\_conflict | yes | no | no |
| lm\_breaker\_timedout | yes | no | no |

## buffer\_head

prototypes:

```
void (*b_end_io)(struct buffer_head *bh, int uptodate);
```

locking rules:

called from interrupts. In other words, extreme care is needed here.
bh is locked, but that’s all warranties we have here. Currently only RAID1,
highmem, fs/buffer.c, and fs/ntfs/aops.c are providing these. Block devices
call this method upon the IO completion.

## block\_device\_operations

prototypes:

```
int (*open) (struct block_device *, fmode_t);
int (*release) (struct gendisk *, fmode_t);
int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
int (*direct_access) (struct block_device *, sector_t, void **,
                        unsigned long *);
void (*unlock_native_capacity) (struct gendisk *);
int (*getgeo)(struct gendisk *, struct hd_geometry *);
void (*swap_slot_free_notify) (struct block_device *, unsigned long);
```

locking rules:

| ops | open\_mutex |
| --- | --- |
| open: | yes |
| release: | yes |
| ioctl: | no |
| compat\_ioctl: | no |
| direct\_access: | no |
| unlock\_native\_capacity: | no |
| getgeo: | no |
| swap\_slot\_free\_notify: | no (see below) |

swap\_slot\_free\_notify is called with swap\_lock and sometimes the page lock
held.

## file\_operations

prototypes:

```
loff_t (*llseek) (struct file *, loff_t, int);
ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
int (*iopoll) (struct kiocb *kiocb, bool spin);
int (*iterate_shared) (struct file *, struct dir_context *);
__poll_t (*poll) (struct file *, struct poll_table_struct *);
long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
int (*mmap) (struct file *, struct vm_area_struct *);
int (*open) (struct inode *, struct file *);
int (*flush) (struct file *);
int (*release) (struct inode *, struct file *);
int (*fsync) (struct file *, loff_t start, loff_t end, int datasync);
int (*fasync) (int, struct file *, int);
int (*lock) (struct file *, int, struct file_lock *);
unsigned long (*get_unmapped_area)(struct file *, unsigned long,
                unsigned long, unsigned long, unsigned long);
int (*check_flags)(int);
int (*flock) (struct file *, int, struct file_lock *);
ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *,
                size_t, unsigned int);
ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *,
                size_t, unsigned int);
int (*setlease)(struct file *, long, struct file_lock **, void **);
long (*fallocate)(struct file *, int, loff_t, loff_t);
void (*show_fdinfo)(struct seq_file *m, struct file *f);
unsigned (*mmap_capabilities)(struct file *);
ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
                loff_t, size_t, unsigned int);
loff_t (*remap_file_range)(struct file *file_in, loff_t pos_in,
                struct file *file_out, loff_t pos_out,
                loff_t len, unsigned int remap_flags);
int (*fadvise)(struct file *, loff_t, loff_t, int);
```

locking rules:
:   All may block.

->`llseek()` locking has moved from llseek to the individual llseek
implementations. If your fs is not using generic\_file\_llseek, you
need to acquire and release the appropriate locks in your ->`llseek()`.
For many filesystems, it is probably safe to acquire the inode
mutex or just to use `i_size_read()` instead.
Note: this does not protect the file->f\_pos against concurrent modifications
since this is something the userspace has to take care about.

->`iterate_shared()` is called with i\_rwsem held for reading, and with the
file f\_pos\_lock held exclusively

->`fasync()` is responsible for maintaining the FASYNC bit in filp->f\_flags.
Most instances call `fasync_helper()`, which does that maintenance, so it’s
not normally something one needs to worry about. Return values > 0 will be
mapped to zero in the VFS layer.

->`readdir()` and ->ioctl() on directories must be changed. Ideally we would
move ->`readdir()` to inode\_operations and use a separate method for directory
->ioctl() or kill the latter completely. One of the problems is that for
anything that resembles union-mount we won’t have a [`struct file`](api-summary.html#c.file "file") for all
components. And there are other reasons why the current interface is a mess...

->read on directories probably must go away - we should just enforce -EISDIR
in `sys_read()` and friends.

->setlease operations should call [`generic_setlease()`](api-summary.html#c.generic_setlease "generic_setlease") before or after setting
the lease within the individual filesystem to record the result of the
operation

->fallocate implementation must be really careful to maintain page cache
consistency when punching holes or performing other operations that invalidate
page cache contents. Usually the filesystem needs to call
[`truncate_inode_pages_range()`](../core-api/mm-api.html#c.truncate_inode_pages_range "truncate_inode_pages_range") to invalidate relevant range of the page cache.
However the filesystem usually also needs to update its internal (and on disk)
view of file offset -> disk block mapping. Until this update is finished, the
filesystem needs to block page faults and reads from reloading now-stale page
cache contents from the disk. Since VFS acquires mapping->invalidate\_lock in
shared mode when loading pages from disk ([`filemap_fault()`](../core-api/mm-api.html#c.filemap_fault "filemap_fault"), [`filemap_read()`](../core-api/mm-api.html#c.filemap_read "filemap_read"),
readahead paths), the fallocate implementation must take the invalidate\_lock to
prevent reloading.

->copy\_file\_range and ->remap\_file\_range implementations need to serialize
against modifications of file data while the operation is running. For
blocking changes through write(2) and similar operations inode->i\_rwsem can be
used. To block changes to file contents via a memory mapping during the
operation, the filesystem must take mapping->invalidate\_lock to coordinate
with ->page\_mkwrite.

## dquot\_operations

prototypes:

```
int (*write_dquot) (struct dquot *);
int (*acquire_dquot) (struct dquot *);
int (*release_dquot) (struct dquot *);
int (*mark_dirty) (struct dquot *);
int (*write_info) (struct super_block *, int);
```

These operations are intended to be more or less wrapping functions that ensure
a proper locking wrt the filesystem and call the generic quota operations.

What filesystem should expect from the generic quota functions:

| ops | FS recursion | Held locks when called |
| --- | --- | --- |
| write\_dquot: | yes | dqonoff\_sem or dqptr\_sem |
| acquire\_dquot: | yes | dqonoff\_sem or dqptr\_sem |
| release\_dquot: | yes | dqonoff\_sem or dqptr\_sem |
| mark\_dirty: | no |  |
| write\_info: | yes | dqonoff\_sem |

FS recursion means calling ->`quota_read()` and ->`quota_write()` from superblock
operations.

More details about quota locking can be found in fs/dquot.c.

## vm\_operations\_struct

prototypes:

```
void (*open)(struct vm_area_struct *);
void (*close)(struct vm_area_struct *);
vm_fault_t (*fault)(struct vm_fault *);
vm_fault_t (*huge_fault)(struct vm_fault *, unsigned int order);
vm_fault_t (*map_pages)(struct vm_fault *, pgoff_t start, pgoff_t end);
vm_fault_t (*page_mkwrite)(struct vm_area_struct *, struct vm_fault *);
vm_fault_t (*pfn_mkwrite)(struct vm_area_struct *, struct vm_fault *);
int (*access)(struct vm_area_struct *, unsigned long, void*, int, int);
```

locking rules:

| ops | mmap\_lock | PageLocked(page) |
| --- | --- | --- |
| open: | write |  |
| close: | read/write |  |
| fault: | read | can return with page locked |
| huge\_fault: | maybe-read |  |
| map\_pages: | maybe-read |  |
| page\_mkwrite: | read | can return with page locked |
| pfn\_mkwrite: | read |  |
| access: | read |  |

->`fault()` is called when a previously not present pte is about to be faulted
in. The filesystem must find and return the page associated with the passed in
“pgoff” in the vm\_fault structure. If it is possible that the page may be
truncated and/or invalidated, then the filesystem must lock invalidate\_lock,
then ensure the page is not already truncated (invalidate\_lock will block
subsequent truncate), and then return with VM\_FAULT\_LOCKED, and the page
locked. The VM will unlock the page.

->`huge_fault()` is called when there is no PUD or PMD entry present. This
gives the filesystem the opportunity to install a PUD or PMD sized page.
Filesystems can also use the ->fault method to return a PMD sized page,
so implementing this function may not be necessary. In particular,
filesystems should not call [`filemap_fault()`](../core-api/mm-api.html#c.filemap_fault "filemap_fault") from ->`huge_fault()`.
The mmap\_lock may not be held when this method is called.

->`map_pages()` is called when VM asks to map easy accessible pages.
Filesystem should find and map pages associated with offsets from “start\_pgoff”
till “end\_pgoff”. ->`map_pages()` is called with the RCU lock held and must
not block. If it’s not possible to reach a page without blocking,
filesystem should skip it. Filesystem should use `set_pte_range()` to setup
page table entry. Pointer to entry associated with the page is passed in
“pte” field in vm\_fault structure. Pointers to entries for other offsets
should be calculated relative to “pte”.

->`page_mkwrite()` is called when a previously read-only pte is about to become
writeable. The filesystem again must ensure that there are no
truncate/invalidate races or races with operations such as ->remap\_file\_range
or ->copy\_file\_range, and then return with the page locked. Usually
mapping->invalidate\_lock is suitable for proper serialization. If the page has
been truncated, the filesystem should not look up a new page like the ->`fault()`
handler, but simply return with VM\_FAULT\_NOPAGE, which will cause the VM to
retry the fault.

->`pfn_mkwrite()` is the same as page\_mkwrite but when the pte is
VM\_PFNMAP or VM\_MIXEDMAP with a page-less entry. Expected return is
VM\_FAULT\_NOPAGE. Or one of the VM\_FAULT\_ERROR types. The default behavior
after this call is to make the pte read-write, unless pfn\_mkwrite returns
an error.

->`access()` is called when `get_user_pages()` fails in
`access_process_vm()`, typically used to debug a process through
/proc/pid/mem or ptrace. This function is needed only for
VM\_IO | VM\_PFNMAP VMAs.

---

> Dubious stuff

(if you break something or notice that it is broken and do not fix it yourself
- at least put it here)
