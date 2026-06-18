# Making Filesystems Exportable

> 출처(원문): https://docs.kernel.org/filesystems/nfs/exporting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Making Filesystems Exportable

## Overview

All filesystem operations require a dentry (or two) as a starting
point. Local applications have a reference-counted hold on suitable
dentries via open file descriptors or cwd/root. However remote
applications that access a filesystem via a remote filesystem protocol
such as NFS may not be able to hold such a reference, and so need a
different way to refer to a particular dentry. As the alternative
form of reference needs to be stable across renames, truncates, and
server-reboot (among other things, though these tend to be the most
problematic), there is no simple answer like ‘filename’.

The mechanism discussed here allows each filesystem implementation to
specify how to generate an opaque (outside of the filesystem) byte
string for any dentry, and how to find an appropriate dentry for any
given opaque byte string.
This byte string will be called a “filehandle fragment” as it
corresponds to part of an NFS filehandle.

A filesystem which supports the mapping between filehandle fragments
and dentries will be termed “exportable”.

## Dcache Issues

The dcache normally contains a proper prefix of any given filesystem
tree. This means that if any filesystem object is in the dcache, then
all of the ancestors of that filesystem object are also in the dcache.
As normal access is by filename this prefix is created naturally and
maintained easily (by each object maintaining a reference count on
its parent).

However when objects are included into the dcache by interpreting a
filehandle fragment, there is no automatic creation of a path prefix
for the object. This leads to two related but distinct features of
the dcache that are not needed for normal filesystem access.

1. The dcache must sometimes contain objects that are not part of the
   proper prefix. i.e that are not connected to the root.
2. The dcache must be prepared for a newly found (via ->lookup) directory
   to already have a (non-connected) dentry, and must be able to move
   that dentry into place (based on the parent and name in the
   ->lookup). This is particularly needed for directories as
   it is a dcache invariant that directories only have one dentry.

To implement these features, the dcache has:

1. A dentry flag DCACHE\_DISCONNECTED which is set on
   any dentry that might not be part of the proper prefix.
   This is set when anonymous dentries are created, and cleared when a
   dentry is noticed to be a child of a dentry which is in the proper
   prefix. If the refcount on a dentry with this flag set
   becomes zero, the dentry is immediately discarded, rather than being
   kept in the dcache. If a dentry that is not already in the dcache
   is repeatedly accessed by filehandle (as NFSD might do), an new dentry
   will be a allocated for each access, and discarded at the end of
   the access.

   Note that such a dentry can acquire children, name, ancestors, etc.
   without losing DCACHE\_DISCONNECTED - that flag is only cleared when
   subtree is successfully reconnected to root. Until then dentries
   in such subtree are retained only as long as there are references;
   refcount reaching zero means immediate eviction, same as for unhashed
   dentries. That guarantees that we won’t need to hunt them down upon
   umount.
2. A primitive for creation of secondary roots - d\_obtain\_root(inode).
   Those do \_not\_ bear DCACHE\_DISCONNECTED. They are placed on the
   per-superblock list (->s\_roots), so they can be located at umount
   time for eviction purposes.
3. Helper routines to allocate anonymous dentries, and to help attach
   loose directory dentries at lookup time. They are:

   > d\_obtain\_alias(inode) will return a dentry for the given inode.
   > :   If the inode already has a dentry, one of those is returned.
   >
   >     If it doesn’t, a new anonymous (IS\_ROOT and
   >     DCACHE\_DISCONNECTED) dentry is allocated and attached.
   >
   >     In the case of a directory, care is taken that only one dentry
   >     can ever be attached.
   >
   > d\_splice\_alias(inode, dentry) will introduce a new dentry into the tree;
   > :   either the passed-in dentry or a preexisting alias for the given inode
   >     (such as an anonymous one created by d\_obtain\_alias), if appropriate.
   >     It returns NULL when the passed-in dentry is used, following the calling
   >     convention of ->lookup.

## Filesystem Issues

For a filesystem to be exportable it must:

> 1. provide the filehandle fragment routines described below.
> 2. make sure that d\_splice\_alias is used rather than d\_add
>    when ->lookup finds an inode for a given parent and name.
>
>    If inode is NULL, d\_splice\_alias(inode, dentry) is equivalent to:
>
>    ```
>    d_add(dentry, inode), NULL
>    ```
>
>    Similarly, d\_splice\_alias(ERR\_PTR(err), dentry) = ERR\_PTR(err)
>
>    Typically the ->lookup routine will simply end with a:
>
>    ```
>            return d_splice_alias(inode, dentry);
>    }
>    ```

A file system implementation declares that instances of the filesystem
are exportable by setting the s\_export\_op field in the `struct
super_block`. This field must point to a [`struct export_operations`](#c.export_operations "export_operations")
which has the following members:

struct export\_operations
:   for nfsd to communicate with file systems

**Definition**:

```
struct export_operations {
    int (*encode_fh)(struct inode *inode, __u32 *fh, int *max_len, struct inode *parent);
    struct dentry * (*fh_to_dentry)(struct super_block *sb, struct fid *fid, int fh_len, int fh_type);
    struct dentry * (*fh_to_parent)(struct super_block *sb, struct fid *fid, int fh_len, int fh_type);
    int (*get_name)(struct dentry *parent, char *name, struct dentry *child);
    struct dentry * (*get_parent)(struct dentry *child);
    int (*commit_metadata)(struct inode *inode);
    int (*get_uuid)(struct super_block *sb, u8 *buf, u32 *len, u64 *offset);
    int (*map_blocks)(struct inode *inode, loff_t offset, u64 len, struct iomap *iomap, bool write, u32 *device_generation);
    int (*commit_blocks)(struct inode *inode, struct iomap *iomaps, int nr_iomaps, struct iattr *iattr);
    int (*permission)(struct handle_to_path_ctx *ctx, unsigned int oflags);
    struct file * (*open)(const struct path *path, unsigned int oflags);
#define EXPORT_OP_NOWCC                 (0x1) ;
#define EXPORT_OP_NOSUBTREECHK          (0x2) ;
#define EXPORT_OP_CLOSE_BEFORE_UNLINK   (0x4) ;
#define EXPORT_OP_REMOTE_FS             (0x8) ;
#define EXPORT_OP_NOATOMIC_ATTR         (0x10)  #define EXPORT_OP_FLUSH_ON_CLOSE        (0x20) ;
#define EXPORT_OP_NOLOCKS               (0x40) ;
    unsigned long   flags;
};
```

**Members**

`encode_fh`
:   **encode\_fh** should store in the file handle fragment **fh** (using at most
    **max\_len** bytes) information that can be used by **decode\_fh** to recover the
    file referred to by the `struct dentry` **de**. If **flag** has CONNECTABLE bit
    set, the `encode_fh()` should store sufficient information so that a good
    attempt can be made to find not only the file but also it’s place in the
    filesystem. This typically means storing a reference to de->d\_parent in
    the filehandle fragment. `encode_fh()` should return the fileid\_type on
    success and on error returns 255 (if the space needed to encode fh is
    greater than **max\_len\*\*\*4 bytes). On error \*\*max\_len** contains the minimum
    size(in 4 byte unit) needed to encode the file handle.

`fh_to_dentry`
:   **fh\_to\_dentry** is given a `struct super_block` (**sb**) and a file handle
    fragment (**fh**, **fh\_len**). It should return a `struct dentry` which refers
    to the same file that the file handle fragment refers to. If it cannot,
    it should return a `NULL` pointer if the file cannot be found, or an
    `ERR_PTR` error code of `ENOMEM` if a memory allocation failure occurred.
    Any other error code is treated like `NULL`, and will cause an `ESTALE` error
    for callers of `exportfs_decode_fh()`.
    Any suitable dentry can be returned including, if necessary, a new dentry
    created with d\_alloc\_root. The caller can then find any other extant
    dentries by following the d\_alias links.

`fh_to_parent`
:   Same as **fh\_to\_dentry**, except that it returns a pointer to the parent
    dentry if it was encoded into the filehandle fragment by **encode\_fh**.

`get_name`
:   **get\_name** should find a name for the given **child** in the given **parent**
    directory. The name should be stored in the **name** (with the
    understanding that it is already pointing to a `NAME_MAX` + 1 sized
    buffer. `get_name()` should return `0` on success, a negative error code
    or error. **get\_name** will be called without **parent->i\_rwsem** held.

`get_parent`
:   **get\_parent** should find the parent directory for the given **child** which
    is also a directory. In the event that it cannot be found, or storage
    space cannot be allocated, a `ERR_PTR` should be returned.

`commit_metadata`
:   **commit\_metadata** should commit metadata changes to stable storage.

`get_uuid`
:   Get a filesystem unique signature exposed to clients.

`map_blocks`
:   Map and, if necessary, allocate blocks for a layout.

`commit_blocks`
:   Commit blocks in a layout once the client is done with them.

`permission`
:   Allow filesystems to specify a custom permission function for the
    open\_by\_handle\_at(2) syscall instead of the default permission check.
    This custom permission function is not respected by nfsd.

`open`
:   Allow filesystems to specify a custom open function for the
    open\_by\_handle\_at(2) syscall instead of the default `file_open_root()`.
    This custom open function is not respected by nfsd.

`flags`
:   Allows the filesystem to communicate to nfsd that it may want to do things
    differently when dealing with it.

**Description**

Methods for open\_by\_handle(2) syscall with special kernel file systems:

See [Making Filesystems Exportable](#) for details on how to use
this interface correctly and the definition of the flags.

Locking rules:
:   get\_parent is called with child->d\_inode->i\_rwsem down
    get\_name is not (which is possibly inconsistent)

A filehandle fragment consists of an array of 1 or more 4byte words,
together with a one byte “type”.
The decode\_fh routine should not depend on the stated size that is
passed to it. This size may be larger than the original filehandle
generated by encode\_fh, in which case it will have been padded with
nuls. Rather, the encode\_fh routine should choose a “type” which
indicates the decode\_fh how much of the filehandle is valid, and how
it should be interpreted.

## Export Operations Flags

In addition to the operation vector pointers, [`struct export_operations`](#c.export_operations "export_operations") also
contains a “flags” field that allows the filesystem to communicate to nfsd
that it may want to do things differently when dealing with it. The
following flags are defined:

> EXPORT\_OP\_NOWCC - disable NFSv3 WCC attributes on this filesystem
> :   RFC 1813 recommends that servers always send weak cache consistency
>     (WCC) data to the client after each operation. The server should
>     atomically collect attributes about the inode, do an operation on it,
>     and then collect the attributes afterward. This allows the client to
>     skip issuing GETATTRs in some situations but means that the server
>     is calling vfs\_getattr for almost all RPCs. On some filesystems
>     (particularly those that are clustered or networked) this is expensive
>     and atomicity is difficult to guarantee. This flag indicates to nfsd
>     that it should skip providing WCC attributes to the client in NFSv3
>     replies when doing operations on this filesystem. Consider enabling
>     this on filesystems that have an expensive ->getattr inode operation,
>     or when atomicity between pre and post operation attribute collection
>     is impossible to guarantee.
>
> EXPORT\_OP\_NOSUBTREECHK - disallow subtree checking on this fs
> :   Many NFS operations deal with filehandles, which the server must then
>     vet to ensure that they live inside of an exported tree. When the
>     export consists of an entire filesystem, this is trivial. nfsd can just
>     ensure that the filehandle live on the filesystem. When only part of a
>     filesystem is exported however, then nfsd must walk the ancestors of the
>     inode to ensure that it’s within an exported subtree. This is an
>     expensive operation and not all filesystems can support it properly.
>     This flag exempts the filesystem from subtree checking and causes
>     exportfs to get back an error if it tries to enable subtree checking
>     on it.
>
> EXPORT\_OP\_CLOSE\_BEFORE\_UNLINK - always close cached files before unlinking
> :   On some exportable filesystems (such as NFS) unlinking a file that
>     is still open can cause a fair bit of extra work. For instance,
>     the NFS client will do a “sillyrename” to ensure that the file
>     sticks around while it’s still open. When reexporting, that open
>     file is held by nfsd so we usually end up doing a sillyrename, and
>     then immediately deleting the sillyrenamed file just afterward when
>     the link count actually goes to zero. Sometimes this delete can race
>     with other operations (for instance an rmdir of the parent directory).
>     This flag causes nfsd to close any open files for this inode \_before\_
>     calling into the vfs to do an unlink or a rename that would replace
>     an existing file.
>
> EXPORT\_OP\_REMOTE\_FS - Backing storage for this filesystem is remote
> :   PF\_LOCAL\_THROTTLE exists for loopback NFSD, where a thread needs to
>     write to one bdi (the final bdi) in order to free up writes queued
>     to another bdi (the client bdi). Such threads get a private balance
>     of dirty pages so that dirty pages for the client bdi do not imact
>     the daemon writing to the final bdi. For filesystems whose durable
>     storage is not local (such as exported NFS filesystems), this
>     constraint has negative consequences. EXPORT\_OP\_REMOTE\_FS enables
>     an export to disable writeback throttling.
>
> EXPORT\_OP\_NOATOMIC\_ATTR - Filesystem does not update attributes atomically
> :   EXPORT\_OP\_NOATOMIC\_ATTR indicates that the exported filesystem
>     cannot provide the semantics required by the “atomic” boolean in
>     NFSv4’s change\_info4. This boolean indicates to a client whether the
>     returned before and after change attributes were obtained atomically
>     with the respect to the requested metadata operation (UNLINK,
>     OPEN/CREATE, MKDIR, etc).
>
> EXPORT\_OP\_FLUSH\_ON\_CLOSE - Filesystem flushes file data on close(2)
> :   On most filesystems, inodes can remain under writeback after the
>     file is closed. NFSD relies on client activity or local flusher
>     threads to handle writeback. Certain filesystems, such as NFS, flush
>     all of an inode’s dirty data on last close. Exports that behave this
>     way should set EXPORT\_OP\_FLUSH\_ON\_CLOSE so that NFSD knows to skip
>     waiting for writeback when closing such files.

## Signed Filehandles

To protect against filehandle guessing attacks, the Linux NFS server can be
configured to sign filehandles with a Message Authentication Code (MAC).

Standard NFS filehandles are often predictable. If an attacker can guess
a valid filehandle for a file they do not have permission to access via
directory traversal, they may be able to bypass path-based permissions
(though they still remain subject to inode-level permissions).

Signed filehandles prevent this by appending a MAC to the filehandle
before it is sent to the client. Upon receiving a filehandle back from a
client, the server re-calculates the MAC using its internal key and
verifies it against the one provided. If the signatures do not match,
the server treats the filehandle as invalid (returning NFS[34]ERR\_STALE).

Note that signing filehandles provides integrity and authenticity but
not confidentiality. The contents of the filehandle remain visible to
the client; they simply cannot be forged or modified.

### Configuration

To enable signed filehandles, the administrator must provide a signing
key to the kernel and enable the “sign\_fh” export option.

1. Providing a Key
   The signing key is managed via the nfsd netlink interface. This key
   is per-network-namespace and must be set before any exports using
   “sign\_fh” become active.
2. Export Options
   The feature is controlled on a per-export basis in /etc/exports:

   sign\_fh
   :   Enables signing for all filehandles generated under this export.

   no\_sign\_fh
   :   (Default) Disables signing.

### Key Management and Rotation

The security of this mechanism relies entirely on the secrecy of the
signing key.

Initial Setup:
:   The key should be generated using a high-quality random source and
    loaded early in the boot process or during the nfs-server startup
    sequence.

Changing Keys:
:   If a key is changed while clients have active mounts, existing
    filehandles held by those clients will become invalid, resulting in
    “Stale file handle” errors on the client side.

Safe Rotation:
:   Currently, there is no mechanism for “graceful” key rotation
    (maintaining multiple valid keys). Changing the key is an atomic
    operation that immediately invalidates all previous signatures.

### Transitioning Exports

When adding or removing the “sign\_fh” flag from an active export, the
following behaviors should be expected:

| Change | Result for Existing Clients |
| --- | --- |
| Adding sign\_fh | Clients holding unsigned filehandles will find them rejected, as the server now expects a signature. |
| Removing sign\_fh | Clients holding signed filehandles will find them rejected, as the server now expects the filehandle to end at its traditional boundary without a MAC. |

Because filehandles are often cached persistently by clients, adding or
removing this option should generally be done during a scheduled maintenance
window involving a NFS client unmount/remount.
