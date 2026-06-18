# Linux Security Module Development

> 출처(원문): https://docs.kernel.org/security/lsm-development.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Security Module Development

Based on <https://lore.kernel.org/r/20071026073721.618b4778@laptopd505.fenrus.org>,
a new LSM is accepted into the kernel when its intent (a description of
what it tries to protect against and in what cases one would expect to
use it) has been appropriately documented in `Documentation/admin-guide/LSM/`.
This allows an LSM’s code to be easily compared to its goals, and so
that end users and distros can make a more informed decision about which
LSMs suit their requirements.

For extensive documentation on the available LSM hook interfaces, please
see `security/security.c` and associated structures:

void security\_free\_mnt\_opts(void \*\*mnt\_opts)
:   Free memory associated with mount options

**Parameters**

`void **mnt_opts`
:   LSM processed mount options

**Description**

Free memory associated with **mnt\_ops**.

int security\_sb\_eat\_lsm\_opts(char \*options, void \*\*mnt\_opts)
:   Consume LSM mount options

**Parameters**

`char *options`
:   mount options

`void **mnt_opts`
:   LSM processed mount options

**Description**

Eat (scan **options**) and save them in **mnt\_opts**.

**Return**

Returns 0 on success, negative values on failure.

int security\_sb\_mnt\_opts\_compat(struct super\_block \*sb, void \*mnt\_opts)
:   Check if new mount options are allowed

**Parameters**

`struct super_block *sb`
:   filesystem superblock

`void *mnt_opts`
:   new mount options

**Description**

Determine if the new mount options in **mnt\_opts** are allowed given the
existing mounted filesystem at **sb**. **sb** superblock being compared.

**Return**

Returns 0 if options are compatible.

int security\_sb\_remount(struct super\_block \*sb, void \*mnt\_opts)
:   Verify no incompatible mount changes during remount

**Parameters**

`struct super_block *sb`
:   filesystem superblock

`void *mnt_opts`
:   (re)mount options

**Description**

Extracts security system specific mount options and verifies no changes are
being made to those options.

**Return**

Returns 0 if permission is granted.

int security\_sb\_set\_mnt\_opts(struct super\_block \*sb, void \*mnt\_opts, unsigned long kern\_flags, unsigned long \*set\_kern\_flags)
:   Set the mount options for a filesystem

**Parameters**

`struct super_block *sb`
:   filesystem superblock

`void *mnt_opts`
:   binary mount options

`unsigned long kern_flags`
:   kernel flags (in)

`unsigned long *set_kern_flags`
:   kernel flags (out)

**Description**

Set the security relevant mount options used for a superblock.

**Return**

Returns 0 on success, error on failure.

int security\_sb\_clone\_mnt\_opts(const struct super\_block \*oldsb, struct super\_block \*newsb, unsigned long kern\_flags, unsigned long \*set\_kern\_flags)
:   Duplicate superblock mount options

**Parameters**

`const struct super_block *oldsb`
:   source superblock

`struct super_block *newsb`
:   destination superblock

`unsigned long kern_flags`
:   kernel flags (in)

`unsigned long *set_kern_flags`
:   kernel flags (out)

**Description**

Copy all security options from a given superblock to another.

**Return**

Returns 0 on success, error on failure.

int security\_dentry\_init\_security(struct [dentry](#c.security_dentry_init_security "dentry") \*dentry, int mode, const struct qstr \*name, const char \*\*xattr\_name, struct lsm\_context \*lsmctx)
:   Perform dentry initialization

**Parameters**

`struct dentry *dentry`
:   the dentry to initialize

`int mode`
:   mode used to determine resource type

`const struct qstr *name`
:   name of the last path component

`const char **xattr_name`
:   name of the security/LSM xattr

`struct lsm_context *lsmctx`
:   pointer to the resulting LSM context

**Description**

Compute a context for a dentry as the inode is not yet available since NFSv4
has no label backed by an EA anyway. It is important to note that
**xattr\_name** does not need to be free’d by the caller, it is a static string.

**Return**

Returns 0 on success, negative values on failure.

int security\_dentry\_create\_files\_as(struct [dentry](#c.security_dentry_create_files_as "dentry") \*dentry, int mode, const struct qstr \*name, const struct cred \*old, struct cred \*new)
:   Perform dentry initialization

**Parameters**

`struct dentry *dentry`
:   the dentry to initialize

`int mode`
:   mode used to determine resource type

`const struct qstr *name`
:   name of the last path component

`const struct cred *old`
:   creds to use for LSM context calculations

`struct cred *new`
:   creds to modify

**Description**

Compute a context for a dentry as the inode is not yet available and set
that context in passed in creds so that new files are created using that
context. Context is calculated using the passed in creds and not the creds
of the caller.

**Return**

Returns 0 on success, error on failure.

int security\_inode\_init\_security(struct [inode](#c.security_inode_init_security "inode") \*inode, struct [inode](#c.security_inode_init_security "inode") \*dir, const struct [qstr](#c.security_inode_init_security "qstr") \*qstr, const [initxattrs](#c.security_inode_init_security "initxattrs") initxattrs, void \*fs\_data)
:   Initialize an inode’s LSM context

**Parameters**

`struct inode *inode`
:   the inode

`struct inode *dir`
:   parent directory

`const struct qstr *qstr`
:   last component of the pathname

`const initxattrs initxattrs`
:   callback function to write xattrs

`void *fs_data`
:   filesystem specific data

**Description**

Obtain the security attribute name suffix and value to set on a newly
created inode and set up the incore security field for the new inode. This
hook is called by the fs code as part of the inode creation transaction and
provides for atomic labeling of the inode, unlike the post\_create/mkdir/...
hooks called by the VFS.

The hook function is expected to populate the xattrs array, by calling
`lsm_get_xattr_slot()` to retrieve the slots reserved by the security module
with the lbs\_xattr\_count field of the lsm\_blob\_sizes structure. For each
slot, the hook function should set ->name to the attribute name suffix
(e.g. selinux), to allocate ->value (will be freed by the caller) and set it
to the attribute value, to set ->value\_len to the length of the value. If
the security module does not use security attributes or does not wish to put
a security attribute on this particular inode, then it should return
-EOPNOTSUPP to skip this processing.

**Return**

Returns 0 if the LSM successfully initialized all of the inode
security attributes that are required, negative values otherwise.

int security\_path\_mknod(const struct path \*dir, struct [dentry](#c.security_path_mknod "dentry") \*dentry, umode\_t mode, unsigned int dev)
:   Check if creating a special file is allowed

**Parameters**

`const struct path *dir`
:   parent directory

`struct dentry *dentry`
:   new file

`umode_t mode`
:   new file mode

`unsigned int dev`
:   device number

**Description**

Check permissions when creating a file. Note that this hook is called even
if mknod operation is being done for a regular file.

**Return**

Returns 0 if permission is granted.

int security\_path\_mkdir(const struct path \*dir, struct [dentry](#c.security_path_mkdir "dentry") \*dentry, umode\_t mode)
:   Check if creating a new directory is allowed

**Parameters**

`const struct path *dir`
:   parent directory

`struct dentry *dentry`
:   new directory

`umode_t mode`
:   new directory mode

**Description**

Check permissions to create a new directory in the existing directory.

**Return**

Returns 0 if permission is granted.

int security\_path\_unlink(const struct path \*dir, struct [dentry](#c.security_path_unlink "dentry") \*dentry)
:   Check if removing a hard link is allowed

**Parameters**

`const struct path *dir`
:   parent directory

`struct dentry *dentry`
:   file

**Description**

Check the permission to remove a hard link to a file.

**Return**

Returns 0 if permission is granted.

int security\_path\_rename(const struct path \*old\_dir, struct dentry \*old\_dentry, const struct path \*new\_dir, struct dentry \*new\_dentry, unsigned int flags)
:   Check if renaming a file is allowed

**Parameters**

`const struct path *old_dir`
:   parent directory of the old file

`struct dentry *old_dentry`
:   the old file

`const struct path *new_dir`
:   parent directory of the new file

`struct dentry *new_dentry`
:   the new file

`unsigned int flags`
:   flags

**Description**

Check for permission to rename a file or directory.

**Return**

Returns 0 if permission is granted.

int security\_inode\_create(struct inode \*dir, struct [dentry](#c.security_inode_create "dentry") \*dentry, umode\_t mode)
:   Check if creating a file is allowed

**Parameters**

`struct inode *dir`
:   the parent directory

`struct dentry *dentry`
:   the file being created

`umode_t mode`
:   requested file mode

**Description**

Check permission to create a regular file.

**Return**

Returns 0 if permission is granted.

int security\_inode\_mkdir(struct inode \*dir, struct [dentry](#c.security_inode_mkdir "dentry") \*dentry, umode\_t mode)
:   Check if creating a new directory is allowed

**Parameters**

`struct inode *dir`
:   parent directory

`struct dentry *dentry`
:   new directory

`umode_t mode`
:   new directory mode

**Description**

Check permissions to create a new directory in the existing directory
associated with inode structure **dir**.

**Return**

Returns 0 if permission is granted.

int security\_inode\_setattr(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_setattr "dentry") \*dentry, struct iattr \*attr)
:   Check if setting file attributes is allowed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`struct iattr *attr`
:   new attributes

**Description**

Check permission before setting file attributes. Note that the kernel call
to notify\_change is performed from several locations, whenever file
attributes change (such as when a file is truncated, chown/chmod operations,
transferring disk quotas, etc).

**Return**

Returns 0 if permission is granted.

int security\_inode\_listsecurity(struct [inode](#c.security_inode_listsecurity "inode") \*inode, char \*buffer, size\_t buffer\_size)
:   List the xattr security label names

**Parameters**

`struct inode *inode`
:   inode

`char *buffer`
:   buffer

`size_t buffer_size`
:   size of buffer

**Description**

Copy the extended attribute names for the security labels associated with
**inode** into **buffer**. The maximum size of **buffer** is specified by
**buffer\_size**. **buffer** may be NULL to request the size of the buffer
required.

**Return**

Returns number of bytes used/required on success.

int security\_inode\_copy\_up(struct dentry \*src, struct cred \*\*new)
:   Create new creds for an overlayfs copy-up op

**Parameters**

`struct dentry *src`
:   `union dentry` of copy-up file

`struct cred **new`
:   newly created creds

**Description**

A file is about to be copied up from lower layer to upper layer of overlay
filesystem. Security module can prepare a set of new creds and modify as
need be and return new creds. Caller will switch to new creds temporarily to
create new file and release newly allocated creds.

**Return**

Returns 0 on success or a negative error code on error.

int security\_inode\_copy\_up\_xattr(struct dentry \*src, const char \*name)
:   Filter xattrs in an overlayfs copy-up op

**Parameters**

`struct dentry *src`
:   `union dentry` of copy-up file

`const char *name`
:   xattr name

**Description**

Filter the xattrs being copied up when a unioned file is copied up from a
lower layer to the union/overlay layer. The caller is responsible for
reading and writing the xattrs, this hook is merely a filter.

**Return**

Returns 0 to accept the xattr, -ECANCELED to discard the xattr,
-EOPNOTSUPP if the security module does not know about attribute,
or a negative error code to abort the copy up.

int security\_inode\_setintegrity(const struct [inode](#c.security_inode_setintegrity "inode") \*inode, enum lsm\_integrity\_type type, const void \*value, size\_t size)
:   Set the inode’s integrity data

**Parameters**

`const struct inode *inode`
:   inode

`enum lsm_integrity_type type`
:   type of integrity, e.g. hash digest, signature, etc

`const void *value`
:   the integrity value

`size_t size`
:   size of the integrity value

**Description**

Register a verified integrity measurement of a inode with LSMs.
LSMs should free the previously saved data if **value** is NULL.

**Return**

Returns 0 on success, negative values on failure.

int security\_file\_ioctl(struct [file](#c.security_file_ioctl "file") \*file, unsigned int cmd, unsigned long arg)
:   Check if an ioctl is allowed

**Parameters**

`struct file *file`
:   associated file

`unsigned int cmd`
:   ioctl cmd

`unsigned long arg`
:   ioctl arguments

**Description**

Check permission for an ioctl operation on **file**. Note that **arg** sometimes
represents a user space pointer; in other cases, it may be a simple integer
value. When **arg** represents a user space pointer, it should never be used
by the security module.

**Return**

Returns 0 if permission is granted.

int security\_file\_ioctl\_compat(struct [file](#c.security_file_ioctl_compat "file") \*file, unsigned int cmd, unsigned long arg)
:   Check if an ioctl is allowed in compat mode

**Parameters**

`struct file *file`
:   associated file

`unsigned int cmd`
:   ioctl cmd

`unsigned long arg`
:   ioctl arguments

**Description**

Compat version of [`security_file_ioctl()`](#c.security_file_ioctl "security_file_ioctl") that correctly handles 32-bit
processes running on 64-bit kernels.

**Return**

Returns 0 if permission is granted.

int security\_mmap\_backing\_file(struct vm\_area\_struct \*vma, struct [file](../filesystems/api-summary.html#c.file "file") \*backing\_file, struct [file](../filesystems/api-summary.html#c.file "file") \*user\_file)
:   Check if mmap’ing a backing file is allowed

**Parameters**

`struct vm_area_struct *vma`
:   the vm\_area\_struct for the mmap’d region

`struct file *backing_file`
:   the backing file being mmap’d

`struct file *user_file`
:   the user file being mmap’d

**Description**

Check permissions for a mmap operation on a stacked filesystem. This hook
is called after the [`security_mmap_file()`](../core-api/kernel-api.html#c.security_mmap_file "security_mmap_file") and is responsible for authorizing
the mmap on **backing\_file**. It is important to note that the mmap operation
on **user\_file** has already been authorized and the **vma->vm\_file** has been
set to **backing\_file**.

**Return**

Returns 0 if permission is granted.

int security\_file\_post\_open(struct [file](#c.security_file_post_open "file") \*file, int mask)
:   Evaluate a file after it has been opened

**Parameters**

`struct file *file`
:   the file

`int mask`
:   access mask

**Description**

Evaluate an opened file and the access mask requested with open(). The hook
is useful for LSMs that require the file content to be available in order to
make decisions.

**Return**

Returns 0 if permission is granted.

void security\_cred\_getsecid(const struct cred \*c, u32 \*secid)
:   Get the secid from a set of credentials

**Parameters**

`const struct cred *c`
:   credentials

`u32 *secid`
:   secid value

**Description**

Retrieve the security identifier of the cred structure **c**. In case of
failure, **secid** will be set to zero.

void security\_cred\_getlsmprop(const struct cred \*c, struct lsm\_prop \*prop)
:   Get the LSM data from a set of credentials

**Parameters**

`const struct cred *c`
:   credentials

`struct lsm_prop *prop`
:   destination for the LSM data

**Description**

Retrieve the security data of the cred structure **c**. In case of
failure, **prop** will be cleared.

int security\_kernel\_read\_file(struct [file](#c.security_kernel_read_file "file") \*file, enum kernel\_read\_file\_id id, bool contents)
:   Read a file specified by userspace

**Parameters**

`struct file *file`
:   file

`enum kernel_read_file_id id`
:   file identifier

`bool contents`
:   trust if [`security_kernel_post_read_file()`](#c.security_kernel_post_read_file "security_kernel_post_read_file") will be called

**Description**

Read a file specified by userspace.

**Return**

Returns 0 if permission is granted.

int security\_kernel\_post\_read\_file(struct [file](#c.security_kernel_post_read_file "file") \*file, char \*buf, loff\_t size, enum kernel\_read\_file\_id id)
:   Read a file specified by userspace

**Parameters**

`struct file *file`
:   file

`char *buf`
:   file contents

`loff_t size`
:   size of file contents

`enum kernel_read_file_id id`
:   file identifier

**Description**

Read a file specified by userspace. This must be paired with a prior call
to [`security_kernel_read_file()`](#c.security_kernel_read_file "security_kernel_read_file") call that indicated this hook would also be
called, see [`security_kernel_read_file()`](#c.security_kernel_read_file "security_kernel_read_file") for more information.

**Return**

Returns 0 if permission is granted.

int security\_kernel\_load\_data(enum kernel\_load\_data\_id id, bool contents)
:   Load data provided by userspace

**Parameters**

`enum kernel_load_data_id id`
:   data identifier

`bool contents`
:   true if [`security_kernel_post_load_data()`](#c.security_kernel_post_load_data "security_kernel_post_load_data") will be called

**Description**

Load data provided by userspace.

**Return**

Returns 0 if permission is granted.

int security\_kernel\_post\_load\_data(char \*buf, loff\_t size, enum kernel\_load\_data\_id id, char \*description)
:   Load userspace data from a non-file source

**Parameters**

`char *buf`
:   data

`loff_t size`
:   size of data

`enum kernel_load_data_id id`
:   data identifier

`char *description`
:   text description of data, specific to the id value

**Description**

Load data provided by a non-file source (usually userspace buffer). This
must be paired with a prior [`security_kernel_load_data()`](#c.security_kernel_load_data "security_kernel_load_data") call that indicated
this hook would also be called, see [`security_kernel_load_data()`](#c.security_kernel_load_data "security_kernel_load_data") for more
information.

**Return**

Returns 0 if permission is granted.

void security\_current\_getlsmprop\_subj(struct lsm\_prop \*prop)
:   Current task’s subjective LSM data

**Parameters**

`struct lsm_prop *prop`
:   lsm specific information

**Description**

Retrieve the subjective security identifier of the current task and return
it in **prop**.

void security\_task\_getlsmprop\_obj(struct task\_struct \*p, struct lsm\_prop \*prop)
:   Get a task’s objective LSM data

**Parameters**

`struct task_struct *p`
:   target task

`struct lsm_prop *prop`
:   lsm specific information

**Description**

Retrieve the objective security identifier of the task\_struct in **p** and
return it in **prop**.

void security\_d\_instantiate(struct [dentry](#c.security_d_instantiate "dentry") \*dentry, struct [inode](#c.security_d_instantiate "inode") \*inode)
:   Populate an inode’s LSM state based on a dentry

**Parameters**

`struct dentry *dentry`
:   dentry

`struct inode *inode`
:   inode

**Description**

Fill in **inode** security information for a **dentry** if allowed.

int security\_ismaclabel(const char \*name)
:   Check if the named attribute is a MAC label

**Parameters**

`const char *name`
:   full extended attribute name

**Description**

Check if the extended attribute specified by **name** represents a MAC label.

**Return**

Returns 1 if name is a MAC attribute otherwise returns 0.

int security\_secid\_to\_secctx(u32 secid, struct lsm\_context \*cp)
:   Convert a secid to a secctx

**Parameters**

`u32 secid`
:   secid

`struct lsm_context *cp`
:   the LSM context

**Description**

Convert secid to security context. If **cp** is NULL the length of the
result will be returned, but no data will be returned. This
does mean that the length could change between calls to check the length and
the next call which actually allocates and returns the data.

**Return**

Return length of data on success, error on failure.

int security\_lsmprop\_to\_secctx(struct lsm\_prop \*prop, struct lsm\_context \*cp, int lsmid)
:   Convert a lsm\_prop to a secctx

**Parameters**

`struct lsm_prop *prop`
:   lsm specific information

`struct lsm_context *cp`
:   the LSM context

`int lsmid`
:   which security module to report

**Description**

Convert a **prop** entry to security context. If **cp** is NULL the
length of the result will be returned. This does mean that the
length could change between calls to check the length and the
next call which actually allocates and returns the **cp**.

**lsmid** identifies which LSM should supply the context.
A value of LSM\_ID\_UNDEF indicates that the first LSM suppling
the hook should be used. This is used in cases where the
ID of the supplying LSM is unambiguous.

**Return**

Return length of data on success, error on failure.

int security\_secctx\_to\_secid(const char \*secdata, u32 seclen, u32 \*secid)
:   Convert a secctx to a secid

**Parameters**

`const char *secdata`
:   secctx

`u32 seclen`
:   length of secctx

`u32 *secid`
:   secid

**Description**

Convert security context to secid.

**Return**

Returns 0 on success, error on failure.

void security\_release\_secctx(struct lsm\_context \*cp)
:   Free a secctx buffer

**Parameters**

`struct lsm_context *cp`
:   the security context

**Description**

Release the security context.

void security\_inode\_invalidate\_secctx(struct [inode](#c.security_inode_invalidate_secctx "inode") \*inode)
:   Invalidate an inode’s security label

**Parameters**

`struct inode *inode`
:   inode

**Description**

Notify the security module that it must revalidate the security context of
an inode.

int security\_inode\_notifysecctx(struct [inode](#c.security_inode_notifysecctx "inode") \*inode, void \*ctx, u32 ctxlen)
:   Notify the LSM of an inode’s security label

**Parameters**

`struct inode *inode`
:   inode

`void *ctx`
:   secctx

`u32 ctxlen`
:   length of secctx

**Description**

Notify the security module of what the security context of an inode should
be. Initializes the incore security context managed by the security module
for this inode. Example usage: NFS client invokes this hook to initialize
the security context in its incore inode to the value provided by the server
for the file when the server returned the file’s attributes to the client.
Must be called with inode->i\_mutex locked.

**Return**

Returns 0 on success, error on failure.

int security\_inode\_setsecctx(struct [dentry](#c.security_inode_setsecctx "dentry") \*dentry, void \*ctx, u32 ctxlen)
:   Change the security label of an inode

**Parameters**

`struct dentry *dentry`
:   inode

`void *ctx`
:   secctx

`u32 ctxlen`
:   length of secctx

**Description**

Change the security context of an inode. Updates the incore security
context managed by the security module and invokes the fs code as needed
(via \_\_vfs\_setxattr\_noperm) to update any backing xattrs that represent the
context. Example usage: NFS server invokes this hook to change the security
context in its incore inode and on the backing filesystem to a value
provided by the client on a SETATTR operation. Must be called with
inode->i\_mutex locked.

**Return**

Returns 0 on success, error on failure.

int security\_inode\_getsecctx(struct [inode](#c.security_inode_getsecctx "inode") \*inode, struct lsm\_context \*cp)
:   Get the security label of an inode

**Parameters**

`struct inode *inode`
:   inode

`struct lsm_context *cp`
:   security context

**Description**

On success, returns 0 and fills out **cp** with the security context
for the given **inode**.

**Return**

Returns 0 on success, error on failure.

int security\_unix\_stream\_connect(struct [sock](#c.security_unix_stream_connect "sock") \*sock, struct [sock](#c.security_unix_stream_connect "sock") \*other, struct [sock](#c.security_unix_stream_connect "sock") \*newsk)
:   Check if a AF\_UNIX stream is allowed

**Parameters**

`struct sock *sock`
:   originating sock

`struct sock *other`
:   peer sock

`struct sock *newsk`
:   new sock

**Description**

Check permissions before establishing a Unix domain stream connection
between **sock** and **other**.

The **unix\_stream\_connect** and **unix\_may\_send** hooks were necessary because
Linux provides an alternative to the conventional file name space for Unix
domain sockets. Whereas binding and connecting to sockets in the file name
space is mediated by the typical file permissions (and caught by the mknod
and permission hooks in inode\_security\_ops), binding and connecting to
sockets in the abstract name space is completely unmediated. Sufficient
control of Unix domain sockets in the abstract name space isn’t possible
using only the socket layer hooks, since we need to know the actual target
socket, which is not looked up until we are inside the af\_unix code.

**Return**

Returns 0 if permission is granted.

int security\_unix\_may\_send(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, struct [socket](../networking/kapi.html#c.socket "socket") \*other)
:   Check if AF\_UNIX socket can send datagrams

**Parameters**

`struct socket *sock`
:   originating sock

`struct socket *other`
:   peer sock

**Description**

Check permissions before connecting or sending datagrams from **sock** to
**other**.

The **unix\_stream\_connect** and **unix\_may\_send** hooks were necessary because
Linux provides an alternative to the conventional file name space for Unix
domain sockets. Whereas binding and connecting to sockets in the file name
space is mediated by the typical file permissions (and caught by the mknod
and permission hooks in inode\_security\_ops), binding and connecting to
sockets in the abstract name space is completely unmediated. Sufficient
control of Unix domain sockets in the abstract name space isn’t possible
using only the socket layer hooks, since we need to know the actual target
socket, which is not looked up until we are inside the af\_unix code.

**Return**

Returns 0 if permission is granted.

int security\_socket\_socketpair(struct [socket](../networking/kapi.html#c.socket "socket") \*socka, struct [socket](../networking/kapi.html#c.socket "socket") \*sockb)
:   Check if creating a socketpair is allowed

**Parameters**

`struct socket *socka`
:   first socket

`struct socket *sockb`
:   second socket

**Description**

Check permissions before creating a fresh pair of sockets.

**Return**

Returns 0 if permission is granted and the connection was
established.

int security\_sock\_rcv\_skb(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb)
:   Check if an incoming network packet is allowed

**Parameters**

`struct sock *sk`
:   destination sock

`struct sk_buff *skb`
:   incoming packet

**Description**

Check permissions on incoming network packets. This hook is distinct from
Netfilter’s IP input hooks since it is the first time that the incoming
sk\_buff **skb** has been associated with a particular socket, **sk**. Must not
sleep inside this hook because some callers hold spinlocks.

**Return**

Returns 0 if permission is granted.

int security\_socket\_getpeersec\_dgram(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, u32 \*secid)
:   Get the remote peer label

**Parameters**

`struct socket *sock`
:   socket

`struct sk_buff *skb`
:   datagram packet

`u32 *secid`
:   remote peer label secid

**Description**

This hook allows the security module to provide peer socket security state
for udp sockets on a per-packet basis to userspace via getsockopt
SO\_GETPEERSEC. The application must first have indicated the IP\_PASSSEC
option via getsockopt. It can then retrieve the security state returned by
this hook for a packet via the SCM\_SECURITY ancillary message type.

**Return**

Returns 0 on success, error on failure.

void security\_sk\_clone(const struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [sock](../networking/kapi.html#c.sock "sock") \*newsk)
:   Clone a sock’s LSM state

**Parameters**

`const struct sock *sk`
:   original sock

`struct sock *newsk`
:   target sock

**Description**

Clone/copy security structure.

void security\_sk\_classify\_flow(const struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct flowi\_common \*flic)
:   Set a flow’s secid based on socket

**Parameters**

`const struct sock *sk`
:   original socket

`struct flowi_common *flic`
:   target flow

**Description**

Set the target flow’s secid to socket’s secid.

void security\_req\_classify\_flow(const struct request\_sock \*req, struct flowi\_common \*flic)
:   Set a flow’s secid based on request\_sock

**Parameters**

`const struct request_sock *req`
:   request\_sock

`struct flowi_common *flic`
:   target flow

**Description**

Sets **flic**’s secid to **req**’s secid.

void security\_sock\_graft(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [socket](../networking/kapi.html#c.socket "socket") \*parent)
:   Reconcile LSM state when grafting a sock on a socket

**Parameters**

`struct sock *sk`
:   sock being grafted

`struct socket *parent`
:   target parent socket

**Description**

Sets **parent**’s inode secid to **sk**’s secid and update **sk** with any necessary
LSM state from **parent**.

int security\_inet\_conn\_request(const struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, struct request\_sock \*req)
:   Set request\_sock state using incoming connect

**Parameters**

`const struct sock *sk`
:   parent listening sock

`struct sk_buff *skb`
:   incoming connection

`struct request_sock *req`
:   new request\_sock

**Description**

Initialize the **req** LSM state based on **sk** and the incoming connect in **skb**.

**Return**

Returns 0 if permission is granted.

void security\_inet\_conn\_established(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb)
:   Update sock’s LSM state with connection

**Parameters**

`struct sock *sk`
:   sock

`struct sk_buff *skb`
:   connection packet

**Description**

Update **sock**’s LSM state to represent a new connection from **skb**.

int security\_secmark\_relabel\_packet(u32 secid)
:   Check if setting a secmark is allowed

**Parameters**

`u32 secid`
:   new secmark value

**Description**

Check if the process should be allowed to relabel packets to **secid**.

**Return**

Returns 0 if permission is granted.

void security\_secmark\_refcount\_inc(void)
:   Increment the secmark labeling rule count

**Parameters**

`void`
:   no arguments

**Description**

Tells the LSM to increment the number of secmark labeling rules loaded.

void security\_secmark\_refcount\_dec(void)
:   Decrement the secmark labeling rule count

**Parameters**

`void`
:   no arguments

**Description**

Tells the LSM to decrement the number of secmark labeling rules loaded.

int security\_tun\_dev\_alloc\_security(void \*\*security)
:   Allocate a LSM blob for a TUN device

**Parameters**

`void **security`
:   pointer to the LSM blob

**Description**

This hook allows a module to allocate a security structure for a TUN device,
returning the pointer in **security**.

**Return**

Returns a zero on success, negative values on failure.

void security\_tun\_dev\_free\_security(void \*security)
:   Free a TUN device LSM blob

**Parameters**

`void *security`
:   LSM blob

**Description**

This hook allows a module to free the security structure for a TUN device.

int security\_tun\_dev\_create(void)
:   Check if creating a TUN device is allowed

**Parameters**

`void`
:   no arguments

**Description**

Check permissions prior to creating a new TUN device.

**Return**

Returns 0 if permission is granted.

int security\_tun\_dev\_attach\_queue(void \*security)
:   Check if attaching a TUN queue is allowed

**Parameters**

`void *security`
:   TUN device LSM blob

**Description**

Check permissions prior to attaching to a TUN device queue.

**Return**

Returns 0 if permission is granted.

int security\_tun\_dev\_attach(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, void \*security)
:   Update TUN device LSM state on attach

**Parameters**

`struct sock *sk`
:   associated sock

`void *security`
:   TUN device LSM blob

**Description**

This hook can be used by the module to update any security state associated
with the TUN device’s sock structure.

**Return**

Returns 0 if permission is granted.

int security\_tun\_dev\_open(void \*security)
:   Update TUN device LSM state on open

**Parameters**

`void *security`
:   TUN device LSM blob

**Description**

This hook can be used by the module to update any security state associated
with the TUN device’s security structure.

**Return**

Returns 0 if permission is granted.

int security\_sctp\_assoc\_request(struct sctp\_association \*asoc, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb)
:   Update the LSM on a SCTP association req

**Parameters**

`struct sctp_association *asoc`
:   SCTP association

`struct sk_buff *skb`
:   packet requesting the association

**Description**

Passes the **asoc** and **chunk->skb** of the association INIT packet to the LSM.

**Return**

Returns 0 on success, error on failure.

int security\_sctp\_bind\_connect(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, int optname, struct sockaddr \*address, int addrlen)
:   Validate a list of addrs for a SCTP option

**Parameters**

`struct sock *sk`
:   socket

`int optname`
:   SCTP option to validate

`struct sockaddr *address`
:   list of IP addresses to validate

`int addrlen`
:   length of the address list

**Description**

Validiate permissions required for each address associated with sock **sk**.
Depending on **optname**, the addresses will be treated as either a connect or
bind service. The **addrlen** is calculated on each IPv4 and IPv6 address using
sizeof(`struct sockaddr_in`) or sizeof(`struct sockaddr_in6`).

**Return**

Returns 0 on success, error on failure.

void security\_sctp\_sk\_clone(struct sctp\_association \*asoc, struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [sock](../networking/kapi.html#c.sock "sock") \*newsk)
:   Clone a SCTP sock’s LSM state

**Parameters**

`struct sctp_association *asoc`
:   SCTP association

`struct sock *sk`
:   original sock

`struct sock *newsk`
:   target sock

**Description**

Called whenever a new socket is created by accept(2) (i.e. a TCP style
socket) or when a socket is ‘peeled off’ e.g userspace calls
sctp\_peeloff(3).

int security\_sctp\_assoc\_established(struct sctp\_association \*asoc, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb)
:   Update LSM state when assoc established

**Parameters**

`struct sctp_association *asoc`
:   SCTP association

`struct sk_buff *skb`
:   packet establishing the association

**Description**

Passes the **asoc** and **chunk->skb** of the association COOKIE\_ACK packet to the
security module.

**Return**

Returns 0 if permission is granted.

int security\_unix\_find(const struct [path](#c.security_unix_find "path") \*path, struct [sock](../networking/kapi.html#c.sock "sock") \*other, int flags)
:   Check if a named AF\_UNIX socket can connect

**Parameters**

`const struct path *path`
:   path of the socket being connected to

`struct sock *other`
:   peer sock

`int flags`
:   flags associated with the socket

**Description**

This hook is called to check permissions before connecting to a named
AF\_UNIX socket. The caller does not hold any locks on **other**.

**Return**

Returns 0 if permission is granted.

int security\_ib\_pkey\_access(void \*sec, u64 subnet\_prefix, u16 pkey)
:   Check if access to an IB pkey is allowed

**Parameters**

`void *sec`
:   LSM blob

`u64 subnet_prefix`
:   subnet prefix of the port

`u16 pkey`
:   IB pkey

**Description**

Check permission to access a pkey when modifying a QP.

**Return**

Returns 0 if permission is granted.

int security\_ib\_endport\_manage\_subnet(void \*sec, const char \*dev\_name, u8 port\_num)
:   Check if SMPs traffic is allowed

**Parameters**

`void *sec`
:   LSM blob

`const char *dev_name`
:   IB device name

`u8 port_num`
:   port number

**Description**

Check permissions to send and receive SMPs on a end port.

**Return**

Returns 0 if permission is granted.

int security\_ib\_alloc\_security(void \*\*sec)
:   Allocate an Infiniband LSM blob

**Parameters**

`void **sec`
:   LSM blob

**Description**

Allocate a security structure for Infiniband objects.

**Return**

Returns 0 on success, non-zero on failure.

void security\_ib\_free\_security(void \*sec)
:   Free an Infiniband LSM blob

**Parameters**

`void *sec`
:   LSM blob

**Description**

Deallocate an Infiniband security structure.

int security\_xfrm\_policy\_alloc(struct xfrm\_sec\_ctx \*\*ctxp, struct xfrm\_user\_sec\_ctx \*sec\_ctx, gfp\_t gfp)
:   Allocate a xfrm policy LSM blob

**Parameters**

`struct xfrm_sec_ctx **ctxp`
:   xfrm security context being added to the SPD

`struct xfrm_user_sec_ctx *sec_ctx`
:   security label provided by userspace

`gfp_t gfp`
:   gfp flags

**Description**

Allocate a security structure to the xp->security field; the security field
is initialized to NULL when the xfrm\_policy is allocated.

**Return**

Return 0 if operation was successful.

void security\_xfrm\_policy\_free(struct xfrm\_sec\_ctx \*ctx)
:   Free a xfrm security context

**Parameters**

`struct xfrm_sec_ctx *ctx`
:   xfrm security context

**Description**

Free LSM resources associated with **ctx**.

int security\_xfrm\_state\_alloc(struct xfrm\_state \*x, struct xfrm\_user\_sec\_ctx \*sec\_ctx)
:   Allocate a xfrm state LSM blob

**Parameters**

`struct xfrm_state *x`
:   xfrm state being added to the SAD

`struct xfrm_user_sec_ctx *sec_ctx`
:   security label provided by userspace

**Description**

Allocate a security structure to the **x->security** field; the security field
is initialized to NULL when the xfrm\_state is allocated. Set the context to
correspond to **sec\_ctx**.

**Return**

Return 0 if operation was successful.

int security\_xfrm\_state\_delete(struct xfrm\_state \*x)
:   Check if deleting a xfrm state is allowed

**Parameters**

`struct xfrm_state *x`
:   xfrm state

**Description**

Authorize deletion of x->security.

**Return**

Returns 0 if permission is granted.

int security\_locked\_down(enum lockdown\_reason what)
:   Check if a kernel feature is allowed

**Parameters**

`enum lockdown_reason what`
:   requested kernel feature

**Description**

Determine whether a kernel feature that potentially enables arbitrary code
execution in kernel space should be permitted.

**Return**

Returns 0 if permission is granted.

int security\_bdev\_alloc(struct block\_device \*bdev)
:   Allocate a block device LSM blob

**Parameters**

`struct block_device *bdev`
:   block device

**Description**

Allocate and attach a security structure to **bdev->bd\_security**. The
security field is initialized to NULL when the bdev structure is
allocated.

**Return**

Return 0 if operation was successful.

void security\_bdev\_free(struct block\_device \*bdev)
:   Free a block device’s LSM blob

**Parameters**

`struct block_device *bdev`
:   block device

**Description**

Deallocate the bdev security structure and set **bdev->bd\_security** to NULL.

int security\_bdev\_setintegrity(struct block\_device \*bdev, enum lsm\_integrity\_type type, const void \*value, size\_t size)
:   Set the device’s integrity data

**Parameters**

`struct block_device *bdev`
:   block device

`enum lsm_integrity_type type`
:   type of integrity, e.g. hash digest, signature, etc

`const void *value`
:   the integrity value

`size_t size`
:   size of the integrity value

**Description**

Register a verified integrity measurement of a bdev with LSMs.
LSMs should free the previously saved data if **value** is NULL.
Please note that the new hook should be invoked every time the security
information is updated to keep these data current. For example, in dm-verity,
if the mapping table is reloaded and configured to use a different dm-verity
target with a new roothash and signing information, the previously stored
data in the LSM blob will become obsolete. It is crucial to re-invoke the
hook to refresh these data and ensure they are up to date. This necessity
arises from the design of device-mapper, where a device-mapper device is
first created, and then targets are subsequently loaded into it. These
targets can be modified multiple times during the device’s lifetime.
Therefore, while the LSM blob is allocated during the creation of the block
device, its actual contents are not initialized at this stage and can change
substantially over time. This includes alterations from data that the LSMs
‘trusts’ to those they do not, making it essential to handle these changes
correctly. Failure to address this dynamic aspect could potentially allow
for bypassing LSM checks.

**Return**

Returns 0 on success, negative values on failure.
