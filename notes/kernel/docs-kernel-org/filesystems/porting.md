# Changes since 2.5.0:

> 출처(원문): https://docs.kernel.org/filesystems/porting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Changes since 2.5.0:

---

**recommended**

New helpers: `sb_bread()`, `sb_getblk()`, `sb_find_get_block()`, `set_bh()`,
`sb_set_blocksize()` and `sb_min_blocksize()`.

Use them.

(`sb_find_get_block()` replaces 2.4’s `get_hash_table()`)

---

**recommended**

New methods: ->`alloc_inode()` and ->`destroy_inode()`.

Remove inode->u.foo\_inode\_i

Declare:

```
struct foo_inode_info {
        /* fs-private stuff */
        struct inode vfs_inode;
};
static inline struct foo_inode_info *FOO_I(struct inode *inode)
{
        return list_entry(inode, struct foo_inode_info, vfs_inode);
}
```

Use FOO\_I(inode) instead of &inode->u.foo\_inode\_i;

Add `foo_alloc_inode()` and `foo_destroy_inode()` - the former should allocate
foo\_inode\_info and return the address of ->vfs\_inode, the latter should free
FOO\_I(inode) (see in-tree filesystems for examples).

Make them ->alloc\_inode and ->destroy\_inode in your super\_operations.

Keep in mind that now you need explicit initialization of private data
typically between calling [`iget_locked()`](api-summary.html#c.iget_locked "iget_locked") and unlocking the inode.

At some point that will become mandatory.

**mandatory**

The foo\_inode\_info should always be allocated through `alloc_inode_sb()` rather
than [`kmem_cache_alloc()`](../core-api/mm-api.html#c.kmem_cache_alloc "kmem_cache_alloc") or [`kmalloc()`](../core-api/mm-api.html#c.kmalloc "kmalloc") related to set up the inode reclaim context
correctly.

---

**mandatory**

Change of file\_system\_type method (->read\_super to ->get\_sb)

->`read_super()` is no more. Ditto for DECLARE\_FSTYPE and DECLARE\_FSTYPE\_DEV.

Turn your `foo_read_super()` into a function that would return 0 in case of
success and negative number in case of error (-EINVAL unless you have more
informative error value to report). Call it `foo_fill_super()`. Now declare:

```
int foo_get_sb(struct file_system_type *fs_type,
      int flags, const char *dev_name, void *data, struct vfsmount *mnt)
{
      return get_sb_bdev(fs_type, flags, dev_name, data, foo_fill_super,
                         mnt);
}
```

(or similar with s/bdev/nodev/ or s/bdev/single/, depending on the kind of
filesystem).

Replace DECLARE\_FSTYPE... with explicit initializer and have ->get\_sb set as
foo\_get\_sb.

---

**mandatory**

Locking change: ->s\_vfs\_rename\_sem is taken only by cross-directory renames.
Most likely there is no need to change anything, but if you relied on
global exclusion between renames for some internal purpose - you need to
change your internal locking. Otherwise exclusion warranties remain the
same (i.e. parents and victim are locked, etc.).

---

**informational**

Now we have the exclusion between ->`lookup()` and directory removal (by
->`rmdir()` and ->`rename()`). If you used to need that exclusion and do
it by internal locking (most of filesystems couldn’t care less) - you
can relax your locking.

---

**mandatory**

->`lookup()`, ->`truncate()`, ->`create()`, ->`unlink()`, ->`mknod()`, ->`mkdir()`,
->`rmdir()`, ->`link()`, ->`lseek()`, ->`symlink()`, ->`rename()`
and ->`readdir()` are called without BKL now. Grab it on entry, drop upon return
- that will guarantee the same locking you used to have. If your method or its
parts do not need BKL - better yet, now you can shift `lock_kernel()` and
`unlock_kernel()` so that they would protect exactly what needs to be
protected.

---

**mandatory**

BKL is also moved from around sb operations. BKL should have been shifted into
individual fs sb\_op functions. If you don’t need it, remove it.

---

**informational**

check for ->`link()` target not being a directory is done by callers. Feel
free to drop it...

---

**informational**

->`link()` callers hold ->i\_mutex on the object we are linking to. Some of your
problems might be over...

---

**mandatory**

new file\_system\_type method - kill\_sb(superblock). If you are converting
an existing filesystem, set it according to ->fs\_flags:

```
FS_REQUIRES_DEV         -       kill_block_super
FS_LITTER               -       kill_litter_super
neither                 -       kill_anon_super
```

FS\_LITTER is gone - just remove it from fs\_flags.

---

**mandatory**

FS\_SINGLE is gone (actually, that had happened back when ->`get_sb()`
went in - and hadn’t been documented ;-/). Just remove it from fs\_flags
(and see ->`get_sb()` entry for other actions).

---

**mandatory**

->`setattr()` is called without BKL now. Caller \_always\_ holds ->i\_mutex, so
watch for ->i\_mutex-grabbing code that might be used by your ->`setattr()`.
Callers of [`notify_change()`](api-summary.html#c.notify_change "notify_change") need ->i\_mutex now.

---

**recommended**

New super\_block field `struct export_operations *s_export_op` for
explicit support for exporting, e.g. via NFS. The structure is fully
documented at its declaration in include/linux/fs.h, and in
[Making Filesystems Exportable](nfs/exporting.html).

Briefly it allows for the definition of decode\_fh and encode\_fh operations
to encode and decode filehandles, and allows the filesystem to use
a standard helper function for decode\_fh, and provide file-system specific
support for this helper, particularly get\_parent.

It is planned that this will be required for exporting once the code
settles down a bit.

**mandatory**

s\_export\_op is now required for exporting a filesystem.
isofs, ext2, ext3, fat
can be used as examples of very different filesystems.

---

**mandatory**

`iget4()` and the read\_inode2 callback have been superseded by [`iget5_locked()`](api-summary.html#c.iget5_locked "iget5_locked")
which has the following prototype:

```
struct inode *iget5_locked(struct super_block *sb, unsigned long ino,
                            int (*test)(struct inode *, void *),
                            int (*set)(struct inode *, void *),
                            void *data);
```

‘test’ is an additional function that can be used when the inode
number is not sufficient to identify the actual file object. ‘set’
should be a non-blocking function that initializes those parts of a
newly created inode to allow the test function to succeed. ‘data’ is
passed as an opaque value to both test and set functions.

When the inode has been created by [`iget5_locked()`](api-summary.html#c.iget5_locked "iget5_locked"), it will be returned with the
I\_NEW flag set and will still be locked. The filesystem then needs to finalize
the initialization. Once the inode is initialized it must be unlocked by
calling [`unlock_new_inode()`](api-summary.html#c.unlock_new_inode "unlock_new_inode").

The filesystem is responsible for setting (and possibly testing) i\_ino
when appropriate. There is also a simpler iget\_locked function that
just takes the superblock and inode number as arguments and does the
test and set for you.

e.g.:

```
inode = iget_locked(sb, ino);
if (inode_state_read_once(inode) & I_NEW) {
        err = read_inode_from_disk(inode);
        if (err < 0) {
                iget_failed(inode);
                return err;
        }
        unlock_new_inode(inode);
}
```

Note that if the process of setting up a new inode fails, then [`iget_failed()`](api-summary.html#c.iget_failed "iget_failed")
should be called on the inode to render it dead, and an appropriate error
should be passed back to the caller.

---

**recommended**

->`getattr()` finally getting used. See instances in nfs, minix, etc.

---

**mandatory**

->`revalidate()` is gone. If your filesystem had it - provide ->`getattr()`
and let it call whatever you had as ->`revlidate()` + (for symlinks that
had ->`revalidate()`) add calls in ->`follow_link()`/->`readlink()`.

---

**mandatory**

->d\_parent changes are not protected by BKL anymore. Read access is safe
if at least one of the following is true:

> * filesystem has no cross-directory `rename()`
> * we know that parent had been locked (e.g. we are looking at
>   ->d\_parent of ->`lookup()` argument).
> * we are called from ->`rename()`.
> * the child’s ->d\_lock is held

Audit your code and add locking if needed. Notice that any place that is
not protected by the conditions above is risky even in the old tree - you
had been relying on BKL and that’s prone to screwups. Old tree had quite
a few holes of that kind - unprotected access to ->d\_parent leading to
anything from oops to silent memory corruption.

---

**mandatory**

FS\_NOMOUNT is gone. If you use it - just set SB\_NOUSER in flags
(see rootfs for one kind of solution and bdev/socket/pipe for another).

---

**recommended**

Use bdev\_read\_only(bdev) instead of is\_read\_only(kdev). The latter
is still alive, but only because of the mess in drivers/s390/block/dasd.c.
As soon as it gets fixed `is_read_only()` will die.

---

**mandatory**

->`permission()` is called without BKL now. Grab it on entry, drop upon
return - that will guarantee the same locking you used to have. If
your method or its parts do not need BKL - better yet, now you can
shift `lock_kernel()` and `unlock_kernel()` so that they would protect
exactly what needs to be protected.

---

**mandatory**

->`statfs()` is now called without BKL held. BKL should have been
shifted into individual fs sb\_op functions where it’s not clear that
it’s safe to remove it. If you don’t need it, remove it.

---

**mandatory**

`is_read_only()` is gone; use `bdev_read_only()` instead.

---

**mandatory**

`destroy_buffers()` is gone; use `invalidate_bdev()`.

---

**mandatory**

`fsync_dev()` is gone; use `fsync_bdev()`. NOTE: lvm breakage is
deliberate; as soon as `struct block_device` \* is propagated in a reasonable
way by that code fixing will become trivial; until then nothing can be
done.

**mandatory**

block truncation on error exit from ->write\_begin, and ->direct\_IO
moved from generic methods (block\_write\_begin, cont\_write\_begin,
nobh\_write\_begin, blockdev\_direct\_IO\*) to callers. Take a look at
ext2\_write\_failed and callers for an example.

**mandatory**

->truncate is gone. The whole truncate sequence needs to be
implemented in ->setattr, which is now mandatory for filesystems
implementing on-disk size changes. Start with a copy of the old inode\_setattr
and vmtruncate, and the reorder the vmtruncate + foofs\_vmtruncate sequence to
be in order of zeroing blocks using block\_truncate\_page or similar helpers,
size update and on finally on-disk truncation which should not fail.
setattr\_prepare (which used to be inode\_change\_ok) now includes the size checks
for ATTR\_SIZE and must be called in the beginning of ->setattr unconditionally.

**mandatory**

->`clear_inode()` and ->`delete_inode()` are gone; ->`evict_inode()` should
be used instead. It gets called whenever the inode is evicted, whether it has
remaining links or not. Caller does *not* evict the pagecache or inode-associated
metadata buffers; the method has to use [`truncate_inode_pages_final()`](../core-api/mm-api.html#c.truncate_inode_pages_final "truncate_inode_pages_final") to get rid
of those. Caller makes sure async writeback cannot be running for the inode while
(or after) ->`evict_inode()` is called.

->`drop_inode()` returns int now; it’s called on final [`iput()`](api-summary.html#c.iput "iput") with
inode->i\_lock held and it returns true if filesystems wants the inode to be
dropped. As before, `inode_generic_drop()` is still the default and it’s been
updated appropriately. `inode_just_drop()` is also alive and it consists
simply of return 1. Note that all actual eviction work is done by caller after
->`drop_inode()` returns.

As before, `clear_inode()` must be called exactly once on each call of
->`evict_inode()` (as it used to be for each call of ->`delete_inode()`). Unlike
before, if you are using inode-associated metadata buffers (i.e.
`mark_buffer_dirty_inode()`), it’s your responsibility to call
`invalidate_inode_buffers()` before `clear_inode()`.

NOTE: checking i\_nlink in the beginning of ->`write_inode()` and bailing out
if it’s zero is not *and* *never* *had* *been* enough. Final `unlink()` and [`iput()`](api-summary.html#c.iput "iput")
may happen while the inode is in the middle of ->`write_inode()`; e.g. if you blindly
free the on-disk inode, you may end up doing that while ->`write_inode()` is writing
to it.

---

**mandatory**

.[`d_delete()`](api-summary.html#c.d_delete "d_delete") now only advises the dcache as to whether or not to cache
unreferenced dentries, and is now only called when the dentry refcount goes to
0. Even on 0 refcount transition, it must be able to tolerate being called 0,
1, or more times (eg. constant, idempotent).

---

**mandatory**

.`d_compare()` calling convention and locking rules are significantly
changed. Read updated documentation in [Overview of the Linux Virtual File System](vfs.html) (and
look at examples of other filesystems) for guidance.

---

**mandatory**

.`d_hash()` calling convention and locking rules are significantly
changed. Read updated documentation in [Overview of the Linux Virtual File System](vfs.html) (and
look at examples of other filesystems) for guidance.

---

**mandatory**

dcache\_lock is gone, replaced by fine grained locks. See fs/dcache.c
for details of what locks to replace dcache\_lock with in order to protect
particular things. Most of the time, a filesystem only needs ->d\_lock, which
protects *all* the dcache state of a given dentry.

---

**mandatory**

Filesystems must RCU-free their inodes, if they can have been accessed
via rcu-walk path walk (basically, if the file can have had a path name in the
vfs namespace).

Even though i\_dentry and i\_rcu share storage in a union, we will
initialize the former in `inode_init_always()`, so just leave it alone in
the callback. It used to be necessary to clean it there, but not anymore
(starting at 3.2).

---

**recommended**

vfs now tries to do path walking in “rcu-walk mode”, which avoids
atomic operations and scalability hazards on dentries and inodes (see
[Pathname lookup](path-lookup.html)). d\_hash and d\_compare changes
(above) are examples of the changes required to support this. For more complex
filesystem callbacks, the vfs drops out of rcu-walk mode before the fs call, so
no changes are required to the filesystem. However, this is costly and loses
the benefits of rcu-walk mode. We will begin to add filesystem callbacks that
are rcu-walk aware, shown below. Filesystems should take advantage of this
where possible.

---

**mandatory**

d\_revalidate is a callback that is made on every path element (if
the filesystem provides it), which requires dropping out of rcu-walk mode. This
may now be called in rcu-walk mode (nd->flags & LOOKUP\_RCU). -ECHILD should be
returned if the filesystem cannot handle rcu-walk. See
[Overview of the Linux Virtual File System](vfs.html) for more details.

permission is an inode permission check that is called on many or all
directory inodes on the way down a path walk (to check for exec permission). It
must now be rcu-walk aware (mask & MAY\_NOT\_BLOCK). See
[Overview of the Linux Virtual File System](vfs.html) for more details.

---

**mandatory**

In ->`fallocate()` you must check the mode option passed in. If your
filesystem does not support hole punching (deallocating space in the middle of a
file) you must return -EOPNOTSUPP if FALLOC\_FL\_PUNCH\_HOLE is set in mode.
Currently you can only have FALLOC\_FL\_PUNCH\_HOLE with FALLOC\_FL\_KEEP\_SIZE set,
so the i\_size should not change when hole punching, even when puching the end of
a file off.

---

**mandatory**

->`get_sb()` and ->`mount()` are gone. Switch to using the new mount API. See
[Filesystem Mount API](mount_api.html) for more details.

---

**mandatory**

->`permission()` and [`generic_permission()`](api-summary.html#c.generic_permission "generic_permission")have lost flags
argument; instead of passing IPERM\_FLAG\_RCU we add MAY\_NOT\_BLOCK into mask.

[`generic_permission()`](api-summary.html#c.generic_permission "generic_permission") has also lost the check\_acl argument; ACL checking
has been taken to VFS and filesystems need to provide a non-NULL
->i\_op->get\_inode\_acl to read an ACL from disk.

---

**mandatory**

If you implement your own ->`llseek()` you must handle SEEK\_HOLE and
SEEK\_DATA. You can handle this by returning -EINVAL, but it would be nicer to
support it in some way. The generic handler assumes that the entire file is
data and there is a virtual hole at the end of the file. So if the provided
offset is less than i\_size and SEEK\_DATA is specified, return the same offset.
If the above is true for the offset and you are given SEEK\_HOLE, return the end
of the file. If the offset is i\_size or greater return -ENXIO in either case.

**mandatory**

If you have your own ->`fsync()` you must make sure to call
[`filemap_write_and_wait_range()`](../core-api/mm-api.html#c.filemap_write_and_wait_range "filemap_write_and_wait_range") so that all dirty pages are synced out properly.
You must also keep in mind that ->`fsync()` is not called with i\_mutex held
anymore, so if you require i\_mutex locking you must make sure to take it and
release it yourself.

---

**mandatory**

`d_alloc_root()` is gone, along with a lot of bugs caused by code
misusing it. Replacement: d\_make\_root(inode). On success d\_make\_root(inode)
allocates and returns a new dentry instantiated with the passed in inode.
On failure NULL is returned and the passed in inode is dropped so the reference
to inode is consumed in all cases and failure handling need not do any cleanup
for the inode. If d\_make\_root(inode) is passed a NULL inode it returns NULL
and also requires no further error handling. Typical usage is:

```
inode = foofs_new_inode(....);
s->s_root = d_make_root(inode);
if (!s->s_root)
        /* Nothing needed for the inode cleanup */
        return -ENOMEM;
...
```

---

**mandatory**

The witch is dead! Well, 2/3 of it, anyway. ->`d_revalidate()` and
->`lookup()` do *not* take `struct nameidata` anymore; just the flags.

---

**mandatory**

->`create()` doesn’t take `struct nameidata *`; unlike the previous
two, it gets “is it an O\_EXCL or equivalent?” boolean argument. Note that
local filesystems can ignore this argument - they are guaranteed that the
object doesn’t exist. It’s remote/distributed ones that might care...

---

**mandatory**

FS\_REVAL\_DOT is gone; if you used to have it, add ->`d_weak_revalidate()`
in your dentry operations instead.

---

**mandatory**

`vfs_readdir()` is gone; switch to `iterate_dir()` instead

---

**mandatory**

->`readdir()` is gone now; switch to ->`iterate_shared()`

**mandatory**

vfs\_follow\_link has been removed. Filesystems must use nd\_set\_link
from ->follow\_link for normal symlinks, or nd\_jump\_link for magic
/proc/<pid> style links.

---

**mandatory**

[`iget5_locked()`](api-summary.html#c.iget5_locked "iget5_locked")/[`ilookup5()`](api-summary.html#c.ilookup5 "ilookup5")/[`ilookup5_nowait()`](api-summary.html#c.ilookup5_nowait "ilookup5_nowait") `test()` callback used to be
called with both ->i\_lock and inode\_hash\_lock held; the former is *not*
taken anymore, so verify that your callbacks do not rely on it (none
of the in-tree instances did). inode\_hash\_lock is still held,
of course, so they are still serialized wrt removal from inode hash,
as well as wrt `set()` callback of [`iget5_locked()`](api-summary.html#c.iget5_locked "iget5_locked").

---

**mandatory**

`d_materialise_unique()` is gone; [`d_splice_alias()`](api-summary.html#c.d_splice_alias "d_splice_alias") does everything you
need now. Remember that they have opposite orders of arguments ;-/

---

**mandatory**

f\_dentry is gone; use f\_path.dentry, or, better yet, see if you can avoid
it entirely.

---

**mandatory**

never call ->read() and ->write() directly; use \_\_vfs\_{read,write} or
wrappers; instead of checking for ->write or ->read being NULL, look for
FMODE\_CAN\_{WRITE,READ} in file->f\_mode.

---

**mandatory**

do \_not\_ use new\_sync\_{read,write} for ->read/->write; leave it NULL
instead.

---

**mandatory**
:   ->aio\_read/->aio\_write are gone. Use ->read\_iter/->write\_iter.

---

**recommended**

for embedded (“fast”) symlinks just set inode->i\_link to wherever the
symlink body is and use `simple_follow_link()` as ->`follow_link()`.

---

**mandatory**

calling conventions for ->`follow_link()` have changed. Instead of returning
cookie and using `nd_set_link()` to store the body to traverse, we return
the body to traverse and store the cookie using explicit void \*\* argument.
nameidata isn’t passed at all - `nd_jump_link()` doesn’t need it and
nd\_[gs]`et_link()` is gone.

---

**mandatory**

calling conventions for ->`put_link()` have changed. It gets inode instead of
dentry, it does not get nameidata at all and it gets called only when cookie
is non-NULL. Note that link body isn’t available anymore, so if you need it,
store it as cookie.

---

**mandatory**

any symlink that might use page\_follow\_link\_light/[`page_put_link()`](api-summary.html#c.page_put_link "page_put_link") must
have inode\_nohighmem(inode) called before anything might start playing with
its pagecache. No highmem pages should end up in the pagecache of such
symlinks. That includes any preseeding that might be done during symlink
creation. `page_symlink()` will honour the mapping gfp flags, so once
you’ve done `inode_nohighmem()` it’s safe to use, but if you allocate and
insert the page manually, make sure to use the right gfp flags.

---

**mandatory**

->`follow_link()` is replaced with ->`get_link()`; same API, except that

> * ->`get_link()` gets inode as a separate argument
> * ->`get_link()` may be called in RCU mode - in that case NULL
>   dentry is passed

---

**mandatory**

->`get_link()` gets `struct delayed_call` `*done` now, and should do
`set_delayed_call()` where it used to set `*cookie`.

->`put_link()` is gone - just give the destructor to `set_delayed_call()`
in ->`get_link()`.

---

**mandatory**

->`getxattr()` and xattr\_handler.`get()` get dentry and inode passed separately.
dentry might be yet to be attached to inode, so do \_not\_ use its ->d\_inode
in the instances. Rationale: !@#!@# [`security_d_instantiate()`](../security/lsm-development.html#c.security_d_instantiate "security_d_instantiate") needs to be
called before we attach dentry to inode.

---

**mandatory**

symlinks are no longer the only inodes that do *not* have i\_bdev/i\_cdev/
i\_pipe/i\_link `union zeroed` out at inode eviction. As the result, you can’t
assume that non-NULL value in ->i\_nlink at ->`destroy_inode()` implies that
it’s a symlink. Checking ->i\_mode is really needed now. In-tree we had
to fix `shmem_destroy_callback()` that used to take that kind of shortcut;
watch out, since that shortcut is no longer valid.

---

**mandatory**

->i\_mutex is replaced with ->i\_rwsem now. `inode_lock()` et.al. work as
they used to - they just take it exclusive. However, ->`lookup()` may be
called with parent locked shared. Its instances must not

> * use d\_instantiate) and [`d_rehash()`](api-summary.html#c.d_rehash "d_rehash") separately - use [`d_add()`](api-summary.html#c.d_add "d_add") or
>   [`d_splice_alias()`](api-summary.html#c.d_splice_alias "d_splice_alias") instead.
> * use [`d_rehash()`](api-summary.html#c.d_rehash "d_rehash") alone - call d\_add(new\_dentry, NULL) instead.
> * in the unlikely case when (read-only) access to filesystem
>   data structures needs exclusion for some reason, arrange it
>   yourself. None of the in-tree filesystems needed that.
> * rely on ->d\_parent and ->d\_name not changing after dentry has
>   been fed to [`d_add()`](api-summary.html#c.d_add "d_add") or [`d_splice_alias()`](api-summary.html#c.d_splice_alias "d_splice_alias"). Again, none of the
>   in-tree instances relied upon that.

We are guaranteed that lookups of the same name in the same directory
will not happen in parallel (“same” in the sense of your ->`d_compare()`).
Lookups on different names in the same directory can and do happen in
parallel now.

---

**mandatory**

->`iterate_shared()` is added.
Exclusion on [`struct file`](api-summary.html#c.file "file") level is still provided (as well as that
between it and lseek on the same [`struct file`](api-summary.html#c.file "file")), but if your directory
has been opened several times, you can get these called in parallel.
Exclusion between that method and all directory-modifying ones is
still provided, of course.

If you have any per-inode or per-dentry in-core data structures modified
by ->`iterate_shared()`, you might need something to serialize the access
to them. If you do dcache pre-seeding, you’ll need to switch to
`d_alloc_parallel()` for that; look for in-tree examples.

---

**mandatory**

->`atomic_open()` calls without O\_CREAT may happen in parallel.

---

**mandatory**

->`setxattr()` and xattr\_handler.`set()` get dentry and inode passed separately.
The xattr\_handler.`set()` gets passed the user namespace of the mount the inode
is seen from so filesystems can idmap the i\_uid and i\_gid accordingly.
dentry might be yet to be attached to inode, so do \_not\_ use its ->d\_inode
in the instances. Rationale: !@#!@# [`security_d_instantiate()`](../security/lsm-development.html#c.security_d_instantiate "security_d_instantiate") needs to be
called before we attach dentry to inode and !@#!@##!@$!$#!@#$!@$!@$ smack
->[`d_instantiate()`](api-summary.html#c.d_instantiate "d_instantiate") uses not just ->`getxattr()` but ->`setxattr()` as well.

---

**mandatory**

->`d_compare()` doesn’t get parent as a separate argument anymore. If you
used it for finding the `struct super_block` involved, dentry->d\_sb will
work just as well; if it’s something more complicated, use dentry->d\_parent.
Just be careful not to assume that fetching it more than once will yield
the same value - in RCU mode it could change under you.

---

**mandatory**

->`rename()` has an added flags argument. Any flags not handled by the
filesystem should result in EINVAL being returned.

---

**recommended**

->readlink is optional for symlinks. Don’t set, unless filesystem needs
to fake something for readlink(2).

---

**mandatory**

->`getattr()` is now passed a `struct path` rather than a vfsmount and
dentry separately, and it now has request\_mask and query\_flags arguments
to specify the fields and sync type requested by statx. Filesystems not
supporting any statx-specific features may ignore the new arguments.

---

**mandatory**

->`atomic_open()` calling conventions have changed. Gone is `int *opened`,
along with FILE\_OPENED/FILE\_CREATED. In place of those we have
FMODE\_OPENED/FMODE\_CREATED, set in file->f\_mode. Additionally, return
value for ‘called [`finish_no_open()`](api-summary.html#c.finish_no_open "finish_no_open"), open it yourself’ case has become
0, not 1. Since [`finish_no_open()`](api-summary.html#c.finish_no_open "finish_no_open") itself is returning 0 now, that part
does not need any changes in ->`atomic_open()` instances.

---

**mandatory**

`alloc_file()` has become static now; two wrappers are to be used instead.
alloc\_file\_pseudo(inode, vfsmount, name, flags, ops) is for the cases
when dentry needs to be created; that’s the majority of old `alloc_file()`
users. Calling conventions: on success a reference to new [`struct file`](api-summary.html#c.file "file")
is returned and callers reference to inode is subsumed by that. On
failure, [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") is returned and no caller’s references are affected,
so the caller needs to drop the inode reference it held.
alloc\_file\_clone(file, flags, ops) does not affect any caller’s references.
On success you get a new [`struct file`](api-summary.html#c.file "file") sharing the mount/dentry with the
original, on failure - [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR").

---

**mandatory**

->`clone_file_range()` and ->dedupe\_file\_range have been replaced with
->`remap_file_range()`. See [Overview of the Linux Virtual File System](vfs.html) for more
information.

---

**recommended**

->`lookup()` instances doing an equivalent of:

```
if (IS_ERR(inode))
        return ERR_CAST(inode);
return d_splice_alias(inode, dentry);
```

don’t need to bother with the check - [`d_splice_alias()`](api-summary.html#c.d_splice_alias "d_splice_alias") will do the
right thing when given ERR\_PTR(...) as inode. Moreover, passing NULL
inode to [`d_splice_alias()`](api-summary.html#c.d_splice_alias "d_splice_alias") will also do the right thing (equivalent of
d\_add(dentry, NULL); return NULL;), so that kind of special cases
also doesn’t need a separate treatment.

---

**strongly recommended**

take the RCU-delayed parts of ->`destroy_inode()` into a new method -
->`free_inode()`. If ->`destroy_inode()` becomes empty - all the better,
just get rid of it. Synchronous work (e.g. the stuff that can’t
be done from an RCU callback, or any `WARN_ON()` where we want the
stack trace) *might* be movable to ->`evict_inode()`; however,
that goes only for the things that are not needed to balance something
done by ->`alloc_inode()`. IOW, if it’s cleaning up the stuff that
might have accumulated over the life of in-core inode, ->`evict_inode()`
might be a fit.

Rules for inode destruction:

> * if ->`destroy_inode()` is non-NULL, it gets called
> * if ->`free_inode()` is non-NULL, it gets scheduled by [`call_rcu()`](../core-api/kernel-api.html#c.call_rcu "call_rcu")
> * combination of NULL ->destroy\_inode and NULL ->free\_inode is
>   treated as NULL/free\_inode\_nonrcu, to preserve the compatibility.

Note that the callback (be it via ->`free_inode()` or explicit [`call_rcu()`](../core-api/kernel-api.html#c.call_rcu "call_rcu")
in ->`destroy_inode()`) is *NOT* ordered wrt superblock destruction;
as the matter of fact, the superblock and all associated structures
might be already gone. The filesystem driver is guaranteed to be still
there, but that’s it. Freeing memory in the callback is fine; doing
more than that is possible, but requires a lot of care and is best
avoided.

---

**mandatory**

DCACHE\_RCUACCESS is gone; having an RCU delay on dentry freeing is the
default. DCACHE\_NORCU opts out, and only `d_alloc_pseudo()` has any
business doing so.

---

**mandatory**

`d_alloc_pseudo()` is internal-only; uses outside of `alloc_file_pseudo()` are
very suspect (and won’t work in modules). Such uses are very likely to
be misspelled `d_alloc_anon()`.

---

**mandatory**

[should’ve been added in 2016] stale comment in [`finish_open()`](api-summary.html#c.finish_open "finish_open") notwithstanding,
failure exits in ->`atomic_open()` instances should *NOT* `fput()` the file,
no matter what. Everything is handled by the caller.

---

**mandatory**

[`clone_private_mount()`](api-summary.html#c.clone_private_mount "clone_private_mount") returns a longterm mount now, so the proper destructor of
its result is `kern_unmount()` or `kern_unmount_array()`.

---

**mandatory**

zero-length bvec segments are disallowed, they must be filtered out before
passed on to an iterator.

---

**mandatory**

For bvec based itererators `bio_iov_iter_get_pages()` now doesn’t copy bvecs but
uses the one provided. Anyone issuing kiocb-I/O should ensure that the bvec and
page references stay until I/O has completed, i.e. until ->`ki_complete()` has
been called or returned with non -EIOCBQUEUED code.

---

**mandatory**

[`mnt_want_write_file()`](api-summary.html#c.mnt_want_write_file "mnt_want_write_file") can now only be paired with `mnt_drop_write_file()`,
whereas previously it could be paired with [`mnt_drop_write()`](api-summary.html#c.mnt_drop_write "mnt_drop_write") as well.

---

**mandatory**

`iov_iter_copy_from_user_atomic()` is gone; use `copy_page_from_iter_atomic()`.
The difference is `copy_page_from_iter_atomic()` advances the iterator and
you don’t need `iov_iter_advance()` after it. However, if you decide to use
only a part of obtained data, you should do `iov_iter_revert()`.

---

**mandatory**

Calling conventions for `file_open_root()` changed; now it takes `struct path` \*
instead of passing mount and dentry separately. For callers that used to
pass <mnt, mnt->mnt\_root> pair (i.e. the root of given mount), a new helper
is provided - `file_open_root_mnt()`. In-tree users adjusted.

---

**mandatory**

no\_llseek is gone; don’t set .llseek to that - just leave it NULL instead.
Checks for “does that file have llseek(2), or should it fail with ESPIPE”
should be done by looking at FMODE\_LSEEK in file->f\_mode.

---

*mandatory*

filldir\_t (readdir callbacks) calling conventions have changed. Instead of
returning 0 or -E... it returns bool now. false means “no more” (as -E... used
to) and true - “keep going” (as 0 in old calling conventions). Rationale:
callers never looked at specific -E... values anyway. -> `iterate_shared()`
instances require no changes at all, all filldir\_t ones in the tree
converted.

---

**mandatory**

Calling conventions for ->`tmpfile()` have changed. It now takes a [`struct
file`](api-summary.html#c.file "file") pointer instead of `struct dentry` pointer. `d_tmpfile()` is similarly
changed to simplify callers. The passed file is in a non-open state and on
success must be opened before returning (e.g. by calling
`finish_open_simple()`).

---

**mandatory**

Calling convention for ->huge\_fault has changed. It now takes a page
order instead of an `enum page_entry_size`, and it may be called without the
mmap\_lock held. All in-tree users have been audited and do not seem to
depend on the mmap\_lock being held, but out of tree users should verify
for themselves. If they do need it, they can return VM\_FAULT\_RETRY to
be called with the mmap\_lock held.

---

**mandatory**

The order of opening block devices and matching or creating superblocks has
changed.

The old logic opened block devices first and then tried to find a
suitable superblock to reuse based on the block device pointer.

The new logic tries to find a suitable superblock first based on the device
number, and opening the block device afterwards.

Since opening block devices cannot happen under s\_umount because of lock
ordering requirements s\_umount is now dropped while opening block devices and
reacquired before calling `fill_super()`.

In the old logic concurrent mounters would find the superblock on the list of
superblocks for the filesystem type. Since the first opener of the block device
would hold s\_umount they would wait until the superblock became either born or
was discarded due to initialization failure.

Since the new logic drops s\_umount concurrent mounters could grab s\_umount and
would spin. Instead they are now made to wait using an explicit wait-wake
mechanism without having to hold s\_umount.

---

**mandatory**

The holder of a block device is now the superblock.

The holder of a block device used to be the file\_system\_type which wasn’t
particularly useful. It wasn’t possible to go from block device to owning
superblock without matching on the device pointer stored in the superblock.
This mechanism would only work for a single device so the block layer couldn’t
find the owning superblock of any additional devices.

In the old mechanism reusing or creating a superblock for a racing mount(2) and
umount(2) relied on the file\_system\_type as the holder. This was severely
underdocumented however:

1. Any concurrent mounter that managed to grab an active reference on an
   existing superblock was made to wait until the superblock either became
   ready or until the superblock was removed from the list of superblocks of
   the filesystem type. If the superblock is ready the caller would simple
   reuse it.
2. If the mounter came after [`deactivate_locked_super()`](api-summary.html#c.deactivate_locked_super "deactivate_locked_super") but before
   the superblock had been removed from the list of superblocks of the
   filesystem type the mounter would wait until the superblock was shutdown,
   reuse the block device and allocate a new superblock.
3. If the mounter came after [`deactivate_locked_super()`](api-summary.html#c.deactivate_locked_super "deactivate_locked_super") and after
   the superblock had been removed from the list of superblocks of the
   filesystem type the mounter would reuse the block device and allocate a new
   superblock (the bd\_holder point may still be set to the filesystem type).

Because the holder of the block device was the file\_system\_type any concurrent
mounter could open the block devices of any superblock of the same
file\_system\_type without risking seeing EBUSY because the block device was
still in use by another superblock.

Making the superblock the owner of the block device changes this as the holder
is now a unique superblock and thus block devices associated with it cannot be
reused by concurrent mounters. So a concurrent mounter in (2) could suddenly
see EBUSY when trying to open a block device whose holder was a different
superblock.

The new logic thus waits until the superblock and the devices are shutdown in
->`kill_sb()`. Removal of the superblock from the list of superblocks of the
filesystem type is now moved to a later point when the devices are closed:

1. Any concurrent mounter managing to grab an active reference on an existing
   superblock is made to wait until the superblock is either ready or until
   the superblock and all devices are shutdown in ->`kill_sb()`. If the
   superblock is ready the caller will simply reuse it.
2. If the mounter comes after [`deactivate_locked_super()`](api-summary.html#c.deactivate_locked_super "deactivate_locked_super") but before
   the superblock has been removed from the list of superblocks of the
   filesystem type the mounter is made to wait until the superblock and the
   devices are shut down in ->`kill_sb()` and the superblock is removed from the
   list of superblocks of the filesystem type. The mounter will allocate a new
   superblock and grab ownership of the block device (the bd\_holder pointer of
   the block device will be set to the newly allocated superblock).
3. This case is now collapsed into (2) as the superblock is left on the list
   of superblocks of the filesystem type until all devices are shutdown in
   ->`kill_sb()`. In other words, if the superblock isn’t on the list of
   superblock of the filesystem type anymore then it has given up ownership of
   all associated block devices (the bd\_holder pointer is NULL).

As this is a VFS level change it has no practical consequences for filesystems
other than that all of them must use one of the provided `kill_litter_super()`,
`kill_anon_super()`, or `kill_block_super()` helpers.

---

**mandatory**

Lock ordering has been changed so that s\_umount ranks above open\_mutex again.
All places where s\_umount was taken under open\_mutex have been fixed up.

---

**mandatory**

export\_operations ->`encode_fh()` no longer has a default implementation to
encode FILEID\_INO32\_GEN\* file handles.
Filesystems that used the default implementation may use the generic helper
[`generic_encode_ino32_fh()`](api-summary.html#c.generic_encode_ino32_fh "generic_encode_ino32_fh") explicitly.

---

**mandatory**

If ->`rename()` update of .. on cross-directory move needs an exclusion with
directory modifications, do *not* lock the subdirectory in question in your
->`rename()` - it’s done by the caller now [that item should’ve been added in
28eceeda130f “fs: Lock moved directories”].

---

**mandatory**

On same-directory ->`rename()` the (tautological) update of .. is not protected
by any locks; just don’t do it if the old parent is the same as the new one.
We really can’t lock two subdirectories in same-directory rename - not without
deadlocks.

---

**mandatory**

`lock_rename()` and `lock_rename_child()` may fail in cross-directory case, if
their arguments do not have a common ancestor. In that case ERR\_PTR(-EXDEV)
is returned, with no locks taken. In-tree users updated; out-of-tree ones
would need to do so.

---

**mandatory**

The list of children anchored in parent dentry got turned into hlist now.
Field names got changed (->d\_children/->d\_sib instead of ->d\_subdirs/->d\_child
for anchor/entries resp.), so any affected places will be immediately caught
by compiler.

---

**mandatory**

->[`d_delete()`](api-summary.html#c.d_delete "d_delete") instances are now called for dentries with ->d\_lock held
and refcount equal to 0. They are not permitted to drop/regain ->d\_lock.
None of in-tree instances did anything of that sort. Make sure yours do not...

---

**mandatory**

->`d_prune()` instances are now called without ->d\_lock held on the parent.
->d\_lock on dentry itself is still held; if you need per-parent exclusions (none
of the in-tree instances did), use your own spinlock.

->`d_iput()` and ->`d_release()` are called with victim dentry still in the
list of parent’s children. It is still unhashed, marked killed, etc., just not
removed from parent’s ->d\_children yet.

Anyone iterating through the list of children needs to be aware of the
half-killed dentries that might be seen there; taking ->d\_lock on those will
see them negative, unhashed and with negative refcount, which means that most
of the in-kernel users would’ve done the right thing anyway without any adjustment.

---

**recommended**

Block device freezing and thawing have been moved to holder operations.

Before this change, `get_active_super()` would only be able to find the
superblock of the main block device, i.e., the one stored in sb->s\_bdev. Block
device freezing now works for any block device owned by a given superblock, not
just the main block device. The `get_active_super()` helper and bd\_fsfreeze\_sb
pointer are gone.

---

**mandatory**

`set_blocksize()` takes opened [`struct file`](api-summary.html#c.file "file") instead of `struct block_device` now
and it *must* be opened exclusive.

---

**mandatory**

->`d_revalidate()` gets two extra arguments - inode of parent directory and
name our dentry is expected to have. Both are stable (dir is pinned in
non-RCU case and will stay around during the call in RCU case, and name
is guaranteed to stay unchanging). Your instance doesn’t have to use
either, but it often helps to avoid a lot of painful boilerplate.
Note that while name->name is stable and NUL-terminated, it may (and
often will) have name->name[name->len] equal to ‘/’ rather than ‘0’ -
in normal case it points into the pathname being looked up.
NOTE: if you need something like full path from the root of filesystem,
you are still on your own - this assists with simple cases, but it’s not
magic.

---

**recommended**

`kern_path_locked()` and `user_path_locked()` no longer return a negative
dentry so this doesn’t need to be checked. If the name cannot be found,
ERR\_PTR(-ENOENT) is returned.

---

**recommended**

`lookup_one_qstr_excl()` is changed to return errors in more cases, so
these conditions don’t require explicit checks:

> * if LOOKUP\_CREATE is NOT given, then the dentry won’t be negative,
>   ERR\_PTR(-ENOENT) is returned instead
> * if LOOKUP\_EXCL IS given, then the dentry won’t be positive,
>   ERR\_PTR(-EEXIST) is rreturned instread

LOOKUP\_EXCL now means “target must not exist”. It can be combined with
LOOK\_CREATE or LOOKUP\_RENAME\_TARGET.

---

**mandatory**
`invalidate_inodes()` is gone use [`evict_inodes()`](api-summary.html#c.evict_inodes "evict_inodes") instead.

---

**mandatory**

->`mkdir()` now returns a dentry. If the created inode is found to
already be in cache and have a dentry (often `IS_ROOT()`), it will need to
be spliced into the given name in place of the given dentry. That dentry
now needs to be returned. If the original dentry is used, NULL should
be returned. Any error should be returned with [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR").

In general, filesystems which use `d_instantiate_new()` to install the new
inode can safely return NULL. Filesystems which may not have an I\_NEW inode
should use [`d_drop()`](api-summary.html#c.d_drop "d_drop");[`d_splice_alias()`](api-summary.html#c.d_splice_alias "d_splice_alias") and return the result of the latter.

If a positive dentry cannot be returned for some reason, in-kernel
clients such as cachefiles, nfsd, smb/server may not perform ideally but
will fail-safe.

---

\*\* mandatory\*\*

[`lookup_one()`](api-summary.html#c.lookup_one "lookup_one"), [`lookup_one_unlocked()`](api-summary.html#c.lookup_one_unlocked "lookup_one_unlocked"), [`lookup_one_positive_unlocked()`](api-summary.html#c.lookup_one_positive_unlocked "lookup_one_positive_unlocked") now
take a qstr instead of a name and len. These, not the “one\_len”
versions, should be used whenever accessing a filesystem from outside
that filesysmtem, through a mount point - which will have a mnt\_idmap.

---

\*\* mandatory\*\*

Functions `try_lookup_one_len()`, `lookup_one_len()`,
`lookup_one_len_unlocked()` and `lookup_positive_unlocked()` have been
renamed to [`try_lookup_noperm()`](api-summary.html#c.try_lookup_noperm "try_lookup_noperm"), [`lookup_noperm()`](api-summary.html#c.lookup_noperm "lookup_noperm"),
[`lookup_noperm_unlocked()`](api-summary.html#c.lookup_noperm_unlocked "lookup_noperm_unlocked"), `lookup_noperm_positive_unlocked()`. They now
take a qstr instead of separate name and length. `QSTR()` can be used
when [`strlen()`](../core-api/kernel-api.html#c.strlen "strlen") is needed for the length.

These function no longer do any permission checking - they previously
checked that the caller has ‘X’ permission on the parent. They must
ONLY be used internally by a filesystem on itself when it knows that
permissions are irrelevant or in a context where permission checks have
already been performed such as after [`vfs_path_parent_lookup()`](api-summary.html#c.vfs_path_parent_lookup "vfs_path_parent_lookup")

---

\*\* mandatory\*\*

`d_hash_and_lookup()` is no longer exported or available outside the VFS.
Use [`try_lookup_noperm()`](api-summary.html#c.try_lookup_noperm "try_lookup_noperm") instead. This adds name validation and takes
arguments in the opposite order but is otherwise identical.

Using [`try_lookup_noperm()`](api-summary.html#c.try_lookup_noperm "try_lookup_noperm") will require linux/namei.h to be included.

---

**mandatory**

Calling conventions for ->`d_automount()` have changed; we should *not* grab
an extra reference to new mount - it should be returned with refcount 1.

---

`collect_mounts()`/`drop_collected_mounts()`/`iterate_mounts()` are gone now.
Replacement is `collect_paths()`/`drop_collected_path()`, with no special
iterator needed. Instead of a cloned mount tree, the new interface returns
an array of `struct path`, one for each mount `collect_mounts()` would’ve
created. These `struct path` point to locations in the caller’s namespace
that would be roots of the cloned mounts.

---

**mandatory**

If your filesystem sets the default dentry\_operations, use `set_default_d_op()`
rather than manually setting sb->s\_d\_op.

---

**mandatory**

`d_set_d_op()` is no longer exported (or public, for that matter); \_if\_
your filesystem really needed that, make use of `d_splice_alias_ops()`
to have them set. Better yet, think hard whether you need different
->d\_op for different dentries - if not, just use `set_default_d_op()`
at mount time and be done with that. Currently procfs is the only
thing that really needs ->d\_op varying between dentries.

---

**highly recommended**

The file operations mmap() callback is deprecated in favour of
`mmap_prepare()`. This passes a pointer to a vm\_area\_desc to the callback
rather than a VMA, as the VMA at this stage is not yet valid.

The vm\_area\_desc provides the minimum required information for a filesystem
to initialise state upon memory mapping of a file-backed region, and output
parameters for the file system to set this state.

In nearly all cases, this is all that is required for a filesystem. However, if
a filesystem needs to perform an operation such a pre-population of page tables,
then that action can be specified in the vm\_area\_desc->action field, which can
be configured using the mmap\_action\_\*() helpers.

---

**mandatory**

Several functions are renamed:

* kern\_path\_locked -> start\_removing\_path
* kern\_path\_create -> start\_creating\_path
* user\_path\_create -> start\_creating\_user\_path
* user\_path\_locked\_at -> start\_removing\_user\_path\_at
* done\_path\_create -> end\_creating\_path

---

**mandatory**

Calling conventions for `vfs_parse_fs_string()` have changed; it does *not*
take length anymore (value ? strlen(value) : 0 is used). If you want
a different length, use

> vfs\_parse\_fs\_qstr(fc, key, &QSTR\_LEN(value, len))

instead.

---

**mandatory**

[`vfs_mkdir()`](api-summary.html#c.vfs_mkdir "vfs_mkdir") now returns a dentry - the one returned by ->`mkdir()`. If
that dentry is different from the dentry passed in, including if it is
an [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") dentry pointer, the original dentry is `dput()`.

When [`vfs_mkdir()`](api-summary.html#c.vfs_mkdir "vfs_mkdir") returns an error, and so both `dputs()` the original
dentry and doesn’t provide a replacement, it also unlocks the parent.
Consequently the return value from [`vfs_mkdir()`](api-summary.html#c.vfs_mkdir "vfs_mkdir") can be passed to
`end_creating()` and the parent will be unlocked precisely when necessary.

---

**mandatory**

`kill_litter_super()` is gone; convert to DCACHE\_PERSISTENT use (as all
in-tree filesystems have done).

---

**mandatory**

The ->`setlease()` file\_operation must now be explicitly set in order to provide
support for leases. When set to NULL, the kernel will now return -EINVAL to
attempts to set a lease. Filesystems that wish to use the kernel-internal lease
implementation should set it to [`generic_setlease()`](api-summary.html#c.generic_setlease "generic_setlease").

---

**mandatory**

fs/namei.c primitives that consume filesystem references (`do_renameat2()`,
`do_linkat()`, `do_symlinkat()`, `do_mkdirat()`, `do_mknodat()`, `do_unlinkat()`
and `do_rmdir()`) are gone; they are replaced with non-consuming analogues
(`filename_renameat2()`, etc.)
Callers are adjusted - responsibility for dropping the filenames belongs
to them now.

---

**mandatory**

`readlink_copy()` now requires link length as the 4th argument. Said length needs
to match what [`strlen()`](../core-api/kernel-api.html#c.strlen "strlen") would return if it was ran on the string.

However, if the string is freely accessible for the duration of inode’s
lifetime, consider using `inode_set_cached_link()` instead.

---

**mandatory**

`lookup_one_qstr_excl()` is no longer exported - use [`start_creating()`](api-summary.html#c.start_creating "start_creating") or
similar.

---

\*\* mandatory\*\*

`lock_rename()`, `lock_rename_child()`, `unlock_rename()` are no
longer available. Use [`start_renaming()`](api-summary.html#c.start_renaming "start_renaming") or similar.

---

**recommended**

If you really need to iterate through dentries for given inode, use
for\_each\_alias(dentry, inode) instead of hlist\_for\_each\_entry; better
yet, see if any of the exported primitives could be used instead of
the entire loop. You still need to hold ->i\_lock of the inode over
either form of manual loop.
